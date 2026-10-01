/**
 * @file npu_power_manager.c
 * @brief NPU Dynamic Clock Gating, DVFS & Ultra-low Power Wakeup Driver (AEP-750 / Issue #280)
 */

#include "npu_power_manager.h"

#include <string.h>
#include <time.h>

#define TAG_NPU_POWER_STATE         262157
#define TAG_NPU_DYNAMIC_POWER_MW    262158
#define TAG_NPU_WAKEUP_LATENCY_US   262159

/* Standard OPP (Operating Performance Points) table */
static const npu_opp_point_t g_opp_table[] = {
    { .freq_mhz = 300,  .voltage_mv = 700,  .active_power_mw = 2200.0f },
    { .freq_mhz = 600,  .voltage_mv = 800,  .active_power_mw = 4800.0f },
    { .freq_mhz = 1000, .voltage_mv = 950,  .active_power_mw = 8500.0f },
    { .freq_mhz = 1200, .voltage_mv = 1050, .active_power_mw = 11500.0f }
};
#define NUM_OPP_POINTS (sizeof(g_opp_table) / sizeof(g_opp_table[0]))

/* Static power consumption per low-power state */
#define PWR_IDLE_GATE_MW   450.0f
#define PWR_STANDBY_MW     80.0f
#define PWR_DEEP_SLEEP_MW  5.0f

typedef struct npu_power_mgr_internal {
    npu_power_config_t config;

    npu_power_state_t current_state;
    uint32_t current_freq_mhz;
    uint32_t current_opp_idx;
    float current_power_mw;

    uint32_t idle_accumulator_ms;
    float last_wakeup_latency_us;

    /* Timing and Energy Counters */
    uint64_t total_active_time_us;
    uint64_t total_gated_time_us;
    uint64_t total_sleep_time_us;
    uint32_t state_transition_count;

    double cumulative_energy_mj;
} npu_power_mgr_internal_t;

_Static_assert(sizeof(npu_power_mgr_internal_t) <= sizeof(npu_power_mgr_storage_t),
               "npu_power_mgr_storage_t buffer size is insufficient for internal state");

/* -------------------------------------------------------------------------- */
/* Public API Implementation                                                  */
/* -------------------------------------------------------------------------- */

npu_power_mgr_t *npu_power_mgr_create_in(npu_power_mgr_storage_t *storage, const npu_power_config_t *config) {
    if (!storage) {
        return NULL;
    }

    npu_power_mgr_internal_t *self = (npu_power_mgr_internal_t *)storage->raw;
    memset(self, 0, sizeof(*self));

    if (config) {
        self->config = *config;
    } else {
        self->config.governor = NPU_DVFS_ONDEMAND;
        self->config.idle_gate_timeout_ms = 5;
        self->config.standby_timeout_ms = 50;
        self->config.deep_sleep_timeout_ms = 500;
        self->config.thermal_warning_temp_c = 75.0f;
        self->config.thermal_critical_temp_c = 85.0f;
        self->config.max_freq_mhz = 1000;
        self->config.min_freq_mhz = 300;
    }

    if (self->config.idle_gate_timeout_ms == 0) self->config.idle_gate_timeout_ms = 5;
    if (self->config.standby_timeout_ms == 0) self->config.standby_timeout_ms = 50;
    if (self->config.deep_sleep_timeout_ms == 0) self->config.deep_sleep_timeout_ms = 500;
    if (self->config.thermal_warning_temp_c <= 0.0f) self->config.thermal_warning_temp_c = 75.0f;
    if (self->config.thermal_critical_temp_c <= 0.0f) self->config.thermal_critical_temp_c = 85.0f;

    /* Start in ACTIVE mode with nominal OPP point (1000MHz) */
    self->current_state = NPU_PWR_ACTIVE;
    self->current_opp_idx = 2; /* 1000 MHz */
    self->current_freq_mhz = g_opp_table[self->current_opp_idx].freq_mhz;
    self->current_power_mw = g_opp_table[self->current_opp_idx].active_power_mw;

    return (npu_power_mgr_t *)self;
}

int npu_power_mgr_notify_tensor_arrival(npu_power_mgr_t *mgr, float *out_wakeup_latency_us) {
    if (!mgr) {
        return -1;
    }

    npu_power_mgr_internal_t *self = (npu_power_mgr_internal_t *)mgr;
    self->idle_accumulator_ms = 0;

    float wakeup_lat_us = 0.0f;
    if (self->current_state != NPU_PWR_ACTIVE) {
        switch (self->current_state) {
            case NPU_PWR_IDLE_GATE:
                wakeup_lat_us = 8.5f; /* 8.5us fast PLL lock */
                break;
            case NPU_PWR_STANDBY_SLEEP:
                wakeup_lat_us = 32.0f; /* 32us core power rail ramp */
                break;
            case NPU_PWR_DEEP_SLEEP:
                wakeup_lat_us = 145.0f; /* 145us full power-on reset */
                break;
            case NPU_PWR_OFF:
                wakeup_lat_us = 5000.0f; /* 5ms cold boot */
                break;
            default:
                wakeup_lat_us = 1.0f;
                break;
        }

        self->current_state = NPU_PWR_ACTIVE;
        self->current_power_mw = g_opp_table[self->current_opp_idx].active_power_mw;
        self->state_transition_count++;
        self->last_wakeup_latency_us = wakeup_lat_us;
    }

    if (out_wakeup_latency_us) {
        *out_wakeup_latency_us = self->last_wakeup_latency_us;
    }
    return 0;
}

int npu_power_mgr_tick(npu_power_mgr_t *mgr, uint32_t delta_time_ms,
                       float current_core_temp_c, uint32_t pending_queue_depth) {
    if (!mgr) {
        return -1;
    }

    npu_power_mgr_internal_t *self = (npu_power_mgr_internal_t *)mgr;

    /* 1. Closed-Loop Thermal Throttling Protection */
    if (current_core_temp_c >= self->config.thermal_critical_temp_c) {
        /* Force clock gate to cool down immediately */
        if (self->current_state == NPU_PWR_ACTIVE) {
            self->current_state = NPU_PWR_IDLE_GATE;
            self->current_power_mw = PWR_IDLE_GATE_MW;
            self->state_transition_count++;
        }
        self->current_opp_idx = 0; /* Min frequency 300MHz */
        self->current_freq_mhz = g_opp_table[0].freq_mhz;
    } else if (current_core_temp_c >= self->config.thermal_warning_temp_c) {
        /* Thermal warning: Step down OPP */
        if (self->current_opp_idx > 0) {
            self->current_opp_idx = 1; /* Drop to 600MHz */
            self->current_freq_mhz = g_opp_table[1].freq_mhz;
        }
    } else {
        /* 2. DVFS Governor Execution */
        switch (self->config.governor) {
            case NPU_DVFS_POWERSAVE:
                self->current_opp_idx = 0;
                self->current_freq_mhz = g_opp_table[0].freq_mhz;
                break;
            case NPU_DVFS_PERFORMANCE:
                self->current_opp_idx = 2; /* 1000 MHz */
                self->current_freq_mhz = g_opp_table[2].freq_mhz;
                break;
            case NPU_DVFS_ONDEMAND:
            case NPU_DVFS_THERMAL_ADAPTIVE:
                if (pending_queue_depth > 4) {
                    self->current_opp_idx = 3; /* Turbo 1200 MHz */
                } else if (pending_queue_depth > 0) {
                    self->current_opp_idx = 2; /* Nominal 1000 MHz */
                } else {
                    self->current_opp_idx = 1; /* Relaxed 600 MHz */
                }
                self->current_freq_mhz = g_opp_table[self->current_opp_idx].freq_mhz;
                break;
        }
    }

    /* 3. Idle Timeout State Machine Progression */
    if (pending_queue_depth == 0) {
        self->idle_accumulator_ms += delta_time_ms;

        if (self->idle_accumulator_ms >= self->config.deep_sleep_timeout_ms) {
            if (self->current_state != NPU_PWR_DEEP_SLEEP) {
                self->current_state = NPU_PWR_DEEP_SLEEP;
                self->current_power_mw = PWR_DEEP_SLEEP_MW;
                self->state_transition_count++;
            }
        } else if (self->idle_accumulator_ms >= self->config.standby_timeout_ms) {
            if (self->current_state != NPU_PWR_STANDBY_SLEEP && self->current_state != NPU_PWR_DEEP_SLEEP) {
                self->current_state = NPU_PWR_STANDBY_SLEEP;
                self->current_power_mw = PWR_STANDBY_MW;
                self->state_transition_count++;
            }
        } else if (self->idle_accumulator_ms >= self->config.idle_gate_timeout_ms) {
            if (self->current_state == NPU_PWR_ACTIVE) {
                self->current_state = NPU_PWR_IDLE_GATE;
                self->current_power_mw = PWR_IDLE_GATE_MW;
                self->state_transition_count++;
            }
        }
    } else if (current_core_temp_c < self->config.thermal_critical_temp_c) {
        self->idle_accumulator_ms = 0;
        if (self->current_state != NPU_PWR_ACTIVE) {
            npu_power_mgr_notify_tensor_arrival(mgr, NULL);
        }
    }

    /* Update current power based on active state / OPP */
    if (self->current_state == NPU_PWR_ACTIVE) {
        self->current_power_mw = g_opp_table[self->current_opp_idx].active_power_mw;
        self->total_active_time_us += (uint64_t)delta_time_ms * 1000ULL;
    } else if (self->current_state == NPU_PWR_IDLE_GATE) {
        self->current_power_mw = PWR_IDLE_GATE_MW;
        self->total_gated_time_us += (uint64_t)delta_time_ms * 1000ULL;
    } else {
        self->total_sleep_time_us += (uint64_t)delta_time_ms * 1000ULL;
    }

    /* 4. Energy Integration: E = P * dt */
    /* P in mW, dt in seconds = delta_time_ms / 1000 => E in mJ */
    double delta_energy_mj = (double)self->current_power_mw * ((double)delta_time_ms / 1000.0);
    self->cumulative_energy_mj += delta_energy_mj;

    return 0;
}

int npu_power_mgr_set_state(npu_power_mgr_t *mgr, npu_power_state_t target_state) {
    if (!mgr) {
        return -1;
    }

    npu_power_mgr_internal_t *self = (npu_power_mgr_internal_t *)mgr;
    if (self->current_state == target_state) {
        return 0;
    }

    self->current_state = target_state;
    self->state_transition_count++;

    switch (target_state) {
        case NPU_PWR_ACTIVE:
            self->current_power_mw = g_opp_table[self->current_opp_idx].active_power_mw;
            self->idle_accumulator_ms = 0;
            break;
        case NPU_PWR_IDLE_GATE:
            self->current_power_mw = PWR_IDLE_GATE_MW;
            break;
        case NPU_PWR_STANDBY_SLEEP:
            self->current_power_mw = PWR_STANDBY_MW;
            break;
        case NPU_PWR_DEEP_SLEEP:
            self->current_power_mw = PWR_DEEP_SLEEP_MW;
            break;
        case NPU_PWR_OFF:
            self->current_power_mw = 0.0f;
            break;
    }

    return 0;
}

int npu_power_mgr_get_metrics(const npu_power_mgr_t *mgr, npu_power_metrics_t *out_metrics) {
    if (!mgr || !out_metrics) {
        return -1;
    }

    const npu_power_mgr_internal_t *self = (const npu_power_mgr_internal_t *)mgr;
    memset(out_metrics, 0, sizeof(*out_metrics));

    out_metrics->current_state = self->current_state;
    out_metrics->current_freq_mhz = self->current_freq_mhz;
    out_metrics->dynamic_power_mw = self->current_power_mw;
    out_metrics->last_wakeup_latency_us = self->last_wakeup_latency_us;
    out_metrics->total_active_time_us = self->total_active_time_us;
    out_metrics->total_gated_time_us = self->total_gated_time_us;
    out_metrics->total_sleep_time_us = self->total_sleep_time_us;
    out_metrics->state_transition_count = self->state_transition_count;
    out_metrics->cumulative_energy_mj = self->cumulative_energy_mj;
    /* 1 mWh = 3600 mJ */
    out_metrics->cumulative_energy_mwh = self->cumulative_energy_mj / 3600.0;

    return 0;
}

int npu_power_mgr_sync_datahub(const npu_power_mgr_t *mgr, datahub_t *hub) {
    if (!mgr || !hub) {
        return -1;
    }

    const npu_power_mgr_internal_t *self = (const npu_power_mgr_internal_t *)mgr;

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, TAG_NPU_POWER_STATE, (double)self->current_state, 0);
    datahub_write_shadow(hub, &msg);

    pipe_msg_init_scalar(&msg, TAG_NPU_DYNAMIC_POWER_MW, (double)self->current_power_mw, 0);
    datahub_write_shadow(hub, &msg);

    pipe_msg_init_scalar(&msg, TAG_NPU_WAKEUP_LATENCY_US, (double)self->last_wakeup_latency_us, 0);
    datahub_write_shadow(hub, &msg);

    return 0;
}

void npu_power_mgr_reset(npu_power_mgr_t *mgr) {
    if (!mgr) {
        return;
    }

    npu_power_mgr_internal_t *self = (npu_power_mgr_internal_t *)mgr;
    npu_power_config_t cfg = self->config;
    memset(self, 0, sizeof(*self));
    self->config = cfg;
    self->current_state = NPU_PWR_ACTIVE;
    self->current_opp_idx = 2;
    self->current_freq_mhz = g_opp_table[2].freq_mhz;
    self->current_power_mw = g_opp_table[2].active_power_mw;
}
