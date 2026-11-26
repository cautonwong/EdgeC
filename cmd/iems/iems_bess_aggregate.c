/**
 * @file iems_bess_aggregate.c
 * @brief BESS cabinet aggregate implementation.
 */

#include "iems_bess_aggregate.h"

#include <limits.h>
#include <math.h>
#include <string.h>

#define BESS_MAGIC 0xBE55A66Du
#define BESS_EPS 1e-9
/* Internal latch for repeated PCS submit failures; adapters use bits 0..30. */
#define BESS_CMD_FAILED_BIT (1u << 31)

typedef enum {
    BESS_SOC_UNKNOWN = 0,
    BESS_SOC_OR,
    BESS_SOC_FULL,
    BESS_SOC_EMPTY
} bess_soc_state_t;

typedef enum {
    BESS_CMD_NONE = 0,
    BESS_CMD_POWER,
    BESS_CMD_ONOFF,
    BESS_CMD_CONNECT
} bess_cmd_kind_t;

typedef struct {
    iems_bess_pcs_status_t pcs;
    iems_bess_bms_sample_t bms;
    bool have_pcs;
    bool have_bms;
    bool pcs_fresh;
    bool bms_fresh;
    bess_soc_state_t soc_state;
    uint32_t active_faults;
    uint32_t latched_faults;
    double last_submitted_kw;
    uint64_t last_submit_ts_ns;
    uint64_t last_attempt_ts_ns;
    iems_bess_port_status_t last_submit_status;
    bool ever_submitted;
    uint8_t consecutive_failures;
} bess_cabinet_t;

struct iems_bess_s {
    uint32_t magic;
    iems_bess_config_t cfg;
    iems_bess_ports_t ports;
    uint8_t count;
    iems_bess_state_t state;

    bool site_tripped;
    uint32_t site_fault_bits;
    bool desired_on;
    bool desired_connected;
    uint32_t config_epoch;
    uint32_t command_seq;

    bool have_command;
    uint64_t command_id;
    bess_cmd_kind_t command_kind;
    double command_value;
    iems_bess_result_t command_result;

    bool have_power;
    double last_power_kw;
    uint64_t last_power_id;

    bool have_result;
    iems_bess_result_t last_result;

    bess_cabinet_t cabinets[IEMS_BESS_MAX_CABINETS];
};

_Static_assert(IEMS_BESS_MAX_CABINETS == 20, "BESS aggregate is fixed at 20 cabinets");
_Static_assert(sizeof(struct iems_bess_s) <= sizeof(iems_bess_storage_t),
               "iems_bess_s exceeds iems_bess_storage_t");

static bool bess_handle_ok(const iems_bess_t *bess) {
    return bess != NULL && bess->magic == BESS_MAGIC;
}

static uint64_t bess_now(const iems_bess_t *bess) {
    return bess->ports.clock.now_ns(bess->ports.clock.ctx);
}

static uint64_t bess_ms_to_ns(double ms) {
    double ns = ms * 1000000.0;
    if (ns >= 1.8446744073709552e19) return UINT64_MAX;
    if (ns <= 0.0) return 0;
    return (uint64_t)ns;
}

static bool bess_ts_fresh(uint64_t now, uint64_t ts, uint64_t stale_ns) {
    return ts <= now && now - ts <= stale_ns;
}

static bool bess_lc_no_valid(const char *lc_no) {
    for (size_t i = 0; i < 16; i++) {
        if (lc_no[i] == '\0') return i > 0;
    }
    return false;
}

static bool bess_soc_window_valid(const iems_bess_soc_window_t *w) {
    return isfinite(w->min) && isfinite(w->min_back) && isfinite(w->max_back) &&
           isfinite(w->max) && w->min >= 0.0 && w->min < w->min_back &&
           w->min_back <= w->max_back && w->max_back < w->max && w->max <= 100.0;
}

static bool bess_config_valid(const iems_bess_config_t *cfg) {
    if (!cfg || cfg->cabinet_count < 1 || cfg->cabinet_count > IEMS_BESS_MAX_CABINETS) {
        return false;
    }
    for (uint8_t i = 0; i < cfg->cabinet_count; i++) {
        if (!bess_lc_no_valid(cfg->cabinet[i].lc_no)) return false;
        for (uint8_t j = 0; j < i; j++) {
            if (strcmp(cfg->cabinet[i].lc_no, cfg->cabinet[j].lc_no) == 0) return false;
        }
    }
    if (!bess_soc_window_valid(&cfg->soc_window)) return false;
    if (!isfinite(cfg->low_soc_threshold_pct) || cfg->low_soc_threshold_pct < 0.0 ||
        cfg->low_soc_threshold_pct > 100.0) {
        return false;
    }
    if (!isfinite(cfg->low_soc_power_kw) || cfg->low_soc_power_kw < 0.0) return false;
    if (!isfinite(cfg->heartbeat_ms) || cfg->heartbeat_ms <= 0.0) return false;
    if (!isfinite(cfg->deadband_kw) || cfg->deadband_kw < 0.0) return false;
    if (!isfinite(cfg->bms_stale_ms) || cfg->bms_stale_ms <= 0.0) return false;
    if (!isfinite(cfg->command_timeout_ms) || cfg->command_timeout_ms <= 0.0) return false;
    if (cfg->command_fail_limit < 1) return false;
    if (cfg->policy != IEMS_BESS_POLICY_REDISTRIBUTE &&
        cfg->policy != IEMS_BESS_POLICY_EQUAL_SPLIT_NO_REMAINDER) {
        return false;
    }
    return true;
}

static bool bess_ports_valid(const iems_bess_ports_t *ports) {
    return ports->pcs.submit != NULL && ports->pcs.read_status != NULL &&
           ports->bms.read_all != NULL && ports->clock.now_ns != NULL;
}

iems_bess_t *iems_bess_create_in(iems_bess_storage_t *storage,
                                 const iems_bess_config_t *cfg,
                                 iems_bess_ports_t ports) {
    if (!storage || !bess_config_valid(cfg) || !bess_ports_valid(&ports)) return NULL;

    iems_bess_t *bess = (iems_bess_t *)storage->raw;
    memset(bess, 0, sizeof(*bess));
    bess->magic = BESS_MAGIC;
    bess->cfg = *cfg;
    bess->ports = ports;
    bess->count = cfg->cabinet_count;
    bess->state = IEMS_BESS_STATE_INIT;
    bess->desired_on = true;
    bess->desired_connected = true;
    for (uint8_t i = 0; i < bess->count; i++) {
        bess->cabinets[i].soc_state = BESS_SOC_UNKNOWN;
        bess->cabinets[i].last_submit_status = IEMS_BESS_PORT_OK;
    }
    return bess;
}

void iems_bess_destroy(iems_bess_t *bess) {
    if (!bess_handle_ok(bess)) return;
    bess->magic = 0;
}

static bool bess_pcs_finite(const iems_bess_pcs_status_t *s) {
    return isfinite(s->measured_active_kw) && isfinite(s->max_charge_kw) &&
           isfinite(s->max_discharge_kw);
}

static bool bess_bms_finite(const iems_bess_bms_sample_t *s) {
    return isfinite(s->soc_pct) && s->soc_pct >= 0.0 && s->soc_pct <= 100.0 &&
           isfinite(s->max_charge_kw) && isfinite(s->max_discharge_kw);
}

static bess_soc_state_t bess_next_soc_state(bess_soc_state_t prev, double soc,
                                            const iems_bess_soc_window_t *w) {
    if (!isfinite(soc) || soc < 0.0 || soc > 100.0) return BESS_SOC_UNKNOWN;
    if (prev == BESS_SOC_FULL) {
        return soc >= w->max_back ? BESS_SOC_FULL : BESS_SOC_OR;
    }
    if (prev == BESS_SOC_EMPTY) {
        return soc <= w->min_back ? BESS_SOC_EMPTY : BESS_SOC_OR;
    }
    if (soc >= w->max) return BESS_SOC_FULL;
    if (soc <= w->min) return BESS_SOC_EMPTY;
    return BESS_SOC_OR;
}

static void bess_collect(iems_bess_t *bess, uint64_t now, uint64_t *out_epoch,
                         bool *out_any_data) {
    iems_bess_bms_sample_t samples[IEMS_BESS_MAX_CABINETS];
    memset(samples, 0, sizeof(samples));
    uint8_t sample_count = 0;
    uint64_t epoch = 0;
    iems_bess_port_status_t bms_rc =
        bess->ports.bms.read_all(bess->ports.bms.ctx, samples,
                                 (uint8_t)IEMS_BESS_MAX_CABINETS, &sample_count, &epoch);
    bool snapshot_ok = bms_rc == IEMS_BESS_PORT_OK && sample_count == bess->count;
    *out_epoch = snapshot_ok ? epoch : 0;

    uint64_t stale_ns = bess_ms_to_ns(bess->cfg.bms_stale_ms);
    bool any_data = false;
    for (uint8_t i = 0; i < bess->count; i++) {
        bess_cabinet_t *cab = &bess->cabinets[i];
        iems_bess_pcs_status_t pcs;
        memset(&pcs, 0, sizeof(pcs));
        iems_bess_port_status_t pcs_rc =
            bess->ports.pcs.read_status(bess->ports.pcs.ctx, (uint8_t)(i + 1u), &pcs);
        bool pcs_ok = pcs_rc == IEMS_BESS_PORT_OK && pcs.valid && bess_pcs_finite(&pcs);
        if (pcs_ok) {
            if (!cab->have_pcs || pcs.sample_ts_ns >= cab->pcs.sample_ts_ns) {
                cab->pcs = pcs;
                cab->have_pcs = true;
            }
            cab->pcs_fresh = bess_ts_fresh(now, cab->pcs.sample_ts_ns, stale_ns);
        } else {
            cab->pcs_fresh = false;
        }

        bool bms_ok = snapshot_ok && samples[i].valid && bess_bms_finite(&samples[i]);
        if (bms_ok) {
            if (!cab->have_bms || samples[i].sample_ts_ns >= cab->bms.sample_ts_ns) {
                cab->bms = samples[i];
                cab->have_bms = true;
            }
            cab->bms_fresh = bess_ts_fresh(now, cab->bms.sample_ts_ns, stale_ns);
            if (cab->bms_fresh) {
                cab->soc_state =
                    bess_next_soc_state(cab->soc_state, cab->bms.soc_pct, &bess->cfg.soc_window);
                any_data = true;
            }
        } else {
            cab->bms_fresh = false;
        }

        uint32_t pcs_faults = (pcs_ok && cab->pcs_fresh) ? cab->pcs.fault_bits : 0u;
        uint32_t bms_faults = (bms_ok && cab->bms_fresh) ? cab->bms.fault_bits : 0u;
        cab->active_faults = pcs_faults | bms_faults;
        cab->latched_faults |= cab->active_faults;
    }
    *out_any_data = any_data;
}

static void bess_classify_cabinet(const iems_bess_t *bess, uint8_t index,
                                  iems_bess_cab_state_t *state,
                                  iems_bess_reason_t *reason, bool *usable,
                                  double *cap_charge, double *cap_discharge) {
    const bess_cabinet_t *cab = &bess->cabinets[index];
    *usable = false;
    *cap_charge = 0.0;
    *cap_discharge = 0.0;
    if (cab->active_faults != 0u) {
        *state = IEMS_BESS_CAB_FAULT;
        *reason = IEMS_BESS_REASON_FAULT;
        return;
    }
    if (cab->latched_faults != 0u) {
        *state = IEMS_BESS_CAB_FAULT_LATCHED;
        *reason = IEMS_BESS_REASON_FAULT;
        return;
    }
    if (!cab->have_pcs || !cab->have_bms) {
        *state = IEMS_BESS_CAB_STALE;
        *reason = IEMS_BESS_REASON_STALE;
        return;
    }
    if (!cab->pcs_fresh || !cab->bms_fresh) {
        *state = IEMS_BESS_CAB_STALE;
        *reason = IEMS_BESS_REASON_STALE;
        return;
    }
    if (!cab->pcs.online || !cab->bms.online) {
        *state = IEMS_BESS_CAB_OFFLINE;
        *reason = IEMS_BESS_REASON_OFFLINE;
        return;
    }
    if (!cab->pcs.onoff_acked) {
        *state = IEMS_BESS_CAB_OFF;
        *reason = IEMS_BESS_REASON_OFF;
        return;
    }

    double charge = fmin(cab->bms.max_charge_kw, cab->pcs.max_charge_kw);
    double discharge = fmin(cab->bms.max_discharge_kw, cab->pcs.max_discharge_kw);
    if (!(charge > 0.0)) charge = 0.0;
    if (!(discharge > 0.0)) discharge = 0.0;
    if (cab->soc_state == BESS_SOC_FULL) charge = 0.0;
    if (cab->soc_state == BESS_SOC_EMPTY) discharge = 0.0;
    *cap_charge = charge;
    *cap_discharge = discharge;
    *usable = true;
    *state = IEMS_BESS_CAB_READY;
    *reason = IEMS_BESS_REASON_OK;
}

static void bess_mark_blocked(const iems_bess_t *bess, uint8_t index, int direction,
                              iems_bess_cab_state_t *state, iems_bess_reason_t *reason) {
    bess_soc_state_t soc_state = bess->cabinets[index].soc_state;
    if (direction < 0) {
        *state = IEMS_BESS_CAB_BLOCKED_CHARGE;
        *reason = soc_state == BESS_SOC_FULL ? IEMS_BESS_REASON_SOC_FULL
                                            : IEMS_BESS_REASON_LIMIT;
    } else {
        *state = IEMS_BESS_CAB_BLOCKED_DISCHARGE;
        *reason = soc_state == BESS_SOC_EMPTY ? IEMS_BESS_REASON_SOC_EMPTY
                                             : IEMS_BESS_REASON_LIMIT;
    }
}

static double bess_water_fill(const uint8_t *idx, uint8_t n, const double *cap,
                              double demand, double *alloc, bool *pinned) {
    bool active[IEMS_BESS_MAX_CABINETS] = {false};
    for (uint8_t k = 0; k < n; k++) active[k] = true;
    double remaining = demand;
    uint8_t active_count = n;
    while (active_count > 0 && remaining > BESS_EPS) {
        double share = remaining / (double)active_count;
        uint8_t saturated = 0;
        for (uint8_t k = 0; k < n; k++) {
            if (!active[k]) continue;
            uint8_t i = idx[k];
            if (cap[i] < share - BESS_EPS) {
                alloc[i] = cap[i];
                pinned[i] = true;
                remaining -= cap[i];
                active[k] = false;
                active_count--;
                saturated++;
            }
        }
        if (saturated == 0) {
            for (uint8_t k = 0; k < n; k++) {
                if (active[k]) alloc[idx[k]] = share;
            }
            remaining = 0.0;
            break;
        }
        if (remaining < 0.0) remaining = 0.0;
    }
    return remaining > BESS_EPS ? remaining : 0.0;
}

static bool bess_should_submit(const iems_bess_t *bess, const bess_cabinet_t *cab,
                               double target, uint64_t now) {
    if (cab->consecutive_failures > 0) {
        uint64_t timeout_ns = bess_ms_to_ns(bess->cfg.command_timeout_ms);
        if (now < cab->last_attempt_ts_ns) return false;
        return now - cab->last_attempt_ts_ns >= timeout_ns;
    }
    if (!cab->ever_submitted) return target != 0.0;
    if (target == 0.0 && cab->last_submitted_kw != 0.0) return true;
    double diff = target - cab->last_submitted_kw;
    if (diff < 0.0) diff = -diff;
    uint64_t elapsed =
        now >= cab->last_submit_ts_ns ? now - cab->last_submit_ts_ns : 0u;
    return diff > bess->cfg.deadband_kw ||
           elapsed >= bess_ms_to_ns(bess->cfg.heartbeat_ms);
}

static void bess_apply_plan(iems_bess_t *bess, uint64_t now, const double *targets,
                            const iems_bess_cab_state_t *states,
                            const iems_bess_reason_t *reasons, bool fault_mode,
                            bool force_submit, bool allow_partial, iems_bess_result_t *r) {
    uint32_t seq = ++bess->command_seq;
    r->command_seq = seq;
    iems_bess_pcs_mode_t mode = IEMS_BESS_PCS_MODE_PQ;
    if (fault_mode) {
        mode = IEMS_BESS_PCS_MODE_FAULT;
    } else if (!bess->desired_on || !bess->desired_connected) {
        mode = IEMS_BESS_PCS_MODE_STANDBY;
    }

    bool any_failed = false;
    double allocated = 0.0;
    double reserved = 0.0;
    for (uint8_t i = 0; i < bess->count; i++) {
        bess_cabinet_t *cab = &bess->cabinets[i];
        double target = targets[i];
        iems_bess_cab_state_t state = states[i];
        iems_bess_reason_t reason = reasons[i];
        if (force_submit || bess_should_submit(bess, cab, target, now)) {
            iems_bess_pcs_command_t cmd;
            memset(&cmd, 0, sizeof(cmd));
            cmd.cabinet_id = (uint8_t)(i + 1u);
            cmd.active_power_kw = target;
            cmd.reactive_power_kvar = 0.0;
            cmd.mode = mode;
            cmd.command_seq = seq;
            cmd.issued_ts_ns = now;
            iems_bess_pcs_ack_t ack;
            memset(&ack, 0, sizeof(ack));
            iems_bess_port_status_t prc =
                bess->ports.pcs.submit(bess->ports.pcs.ctx, &cmd, &ack);
            cab->last_attempt_ts_ns = now;
            cab->last_submit_status = prc;
            if (prc == IEMS_BESS_PORT_OK) {
                cab->last_submitted_kw = target;
                cab->last_submit_ts_ns = now;
                cab->ever_submitted = true;
                cab->consecutive_failures = 0;
            } else {
                if (cab->consecutive_failures < UINT8_MAX) cab->consecutive_failures++;
                if (cab->consecutive_failures >= bess->cfg.command_fail_limit) {
                    cab->latched_faults |= BESS_CMD_FAILED_BIT;
                }
                target = 0.0;
                state = IEMS_BESS_CAB_COMMAND_FAILED;
                reason = IEMS_BESS_REASON_CMD_FAILED;
                any_failed = true;
            }
        }
        r->cabinet[i].cabinet_id = (uint8_t)(i + 1u);
        r->cabinet[i].target_kw = target;
        r->cabinet[i].state = state;
        r->cabinet[i].reason = reason;
        r->cabinet[i].submit_status = cab->last_submit_status;
        allocated += target;
        if (state == IEMS_BESS_CAB_LOW_SOC && target < 0.0) reserved += -target;
    }
    r->allocated_kw = allocated;
    r->reserved_low_soc_kw = reserved;
    if (any_failed && allow_partial) r->status = IEMS_BESS_PARTIAL;
}

static int bess_finish(iems_bess_t *bess, iems_bess_result_t *r,
                       iems_bess_result_t *out) {
    r->state = bess->state;
    bess->last_result = *r;
    bess->have_result = true;
    *out = *r;
    return r->status < 0 ? (int)r->status : 0;
}

static void bess_fill_cabinet_defaults(iems_bess_t *bess, iems_bess_cab_state_t *states,
                                       iems_bess_reason_t *reasons,
                                       iems_bess_cab_state_t state,
                                       iems_bess_reason_t reason) {
    for (uint8_t i = 0; i < bess->count; i++) {
        states[i] = state;
        reasons[i] = reason;
    }
}

static iems_bess_status_t bess_site_status(uint32_t fault_bits) {
    const uint32_t watchdog = IEMS_BESS_SITE_FAULT_WATCHDOG;
    if ((fault_bits & watchdog) != 0u && (fault_bits & ~watchdog) == 0u) {
        return IEMS_BESS_WATCHDOG;
    }
    return IEMS_BESS_TRIPPED;
}

typedef struct {
    double target[IEMS_BESS_MAX_CABINETS];
    iems_bess_cab_state_t states[IEMS_BESS_MAX_CABINETS];
    iems_bess_reason_t reasons[IEMS_BESS_MAX_CABINETS];
    double shortfall;
    bool low_soc_override;
    bool no_eligible;
} bess_plan_t;

static void bess_build_plan(iems_bess_t *bess, double requested_kw, bess_plan_t *plan) {
    memset(plan, 0, sizeof(*plan));
    double cap_charge[IEMS_BESS_MAX_CABINETS] = {0.0};
    double cap_discharge[IEMS_BESS_MAX_CABINETS] = {0.0};
    double reserved_i[IEMS_BESS_MAX_CABINETS] = {0.0};
    bool usable[IEMS_BESS_MAX_CABINETS] = {false};
    bool is_low[IEMS_BESS_MAX_CABINETS] = {false};

    double reserved = 0.0;
    for (uint8_t i = 0; i < bess->count; i++) {
        bool u = false;
        bess_classify_cabinet(bess, i, &plan->states[i], &plan->reasons[i], &u,
                              &cap_charge[i], &cap_discharge[i]);
        usable[i] = u;
        if (!u) continue;
        is_low[i] = bess->cfg.low_soc_enable &&
                    bess->cabinets[i].bms.soc_pct < bess->cfg.low_soc_threshold_pct;
        if (!is_low[i]) continue;
        double reserve = fmin(bess->cfg.low_soc_power_kw, cap_charge[i]);
        if (!(reserve > 0.0)) reserve = 0.0;
        reserved_i[i] = reserve;
        reserved += reserve;
        plan->target[i] = -reserve;
        plan->states[i] = IEMS_BESS_CAB_LOW_SOC;
        plan->reasons[i] = IEMS_BESS_REASON_LOW_SOC;
    }

    bool override = false;
    double normal_demand = requested_kw;
    if (bess->cfg.policy == IEMS_BESS_POLICY_REDISTRIBUTE && reserved > BESS_EPS) {
        double signed_remaining =
            requested_kw < 0.0 ? requested_kw + reserved : requested_kw;
        if (signed_remaining > BESS_EPS) {
            signed_remaining = 0.0;
            override = true;
        } else if (requested_kw >= 0.0) {
            override = true;
        }
        normal_demand = signed_remaining;
    } else if (bess->cfg.policy == IEMS_BESS_POLICY_EQUAL_SPLIT_NO_REMAINDER &&
               requested_kw >= 0.0 && reserved > BESS_EPS) {
        override = true;
    }

    uint8_t candidates[IEMS_BESS_MAX_CABINETS];
    uint8_t candidate_count = 0;
    double alloc_mag[IEMS_BESS_MAX_CABINETS] = {0.0};
    bool pinned[IEMS_BESS_MAX_CABINETS] = {false};
    double shortfall = 0.0;
    bool no_eligible = false;
    int requested_direction =
        requested_kw < 0.0 ? -1 : (requested_kw > 0.0 ? 1 : 0);

    if (bess->cfg.policy == IEMS_BESS_POLICY_REDISTRIBUTE) {
        double demand = fabs(normal_demand);
        if (requested_direction != 0) {
            for (uint8_t i = 0; i < bess->count; i++) {
                if (!usable[i] || is_low[i]) continue;
                double cap =
                    requested_direction < 0 ? cap_charge[i] : cap_discharge[i];
                if (cap <= BESS_EPS) {
                    bess_mark_blocked(bess, i, requested_direction, &plan->states[i],
                                      &plan->reasons[i]);
                    continue;
                }
                if (demand > BESS_EPS) {
                    candidates[candidate_count] = i;
                    candidate_count++;
                }
            }
        }
        if (demand > BESS_EPS) {
            if (candidate_count == 0) {
                no_eligible = true;
                shortfall = demand;
            } else {
                shortfall = bess_water_fill(candidates, candidate_count,
                                            requested_direction < 0 ? cap_charge
                                                                    : cap_discharge,
                                            demand, alloc_mag, pinned);
                for (uint8_t k = 0; k < candidate_count; k++) {
                    uint8_t i = candidates[k];
                    plan->target[i] =
                        requested_direction < 0 ? -alloc_mag[i] : alloc_mag[i];
                    if (pinned[i]) plan->reasons[i] = IEMS_BESS_REASON_LIMIT;
                }
            }
        }
    } else if (requested_direction != 0) {
        /* v3 parity: split, no redistribution (bess.c:175-219, V1/V5). */
        double demand = fabs(requested_kw);
        for (uint8_t i = 0; i < bess->count; i++) {
            if (!usable[i]) continue;
            double cap = requested_direction < 0 ? cap_charge[i] : cap_discharge[i];
            if (cap <= BESS_EPS) {
                bess_mark_blocked(bess, i, requested_direction, &plan->states[i],
                                  &plan->reasons[i]);
                continue;
            }
            if (demand > BESS_EPS) {
                candidates[candidate_count] = i;
                candidate_count++;
            }
        }
        if (demand > BESS_EPS) {
            if (candidate_count == 0) {
                no_eligible = true;
                shortfall = demand;
            } else {
                double share = requested_kw / (double)candidate_count;
                double share_mag = fabs(share);
                for (uint8_t k = 0; k < candidate_count; k++) {
                    uint8_t i = candidates[k];
                    double cap =
                        requested_direction < 0 ? cap_charge[i] : cap_discharge[i];
                    bool hit_limit = cap < share_mag - BESS_EPS;
                    double magnitude = hit_limit ? cap : share_mag;
                    plan->target[i] = share < 0.0 ? -magnitude : magnitude;
                    if (hit_limit) plan->reasons[i] = IEMS_BESS_REASON_LIMIT;
                }
                for (uint8_t i = 0; i < bess->count; i++) {
                    if (is_low[i]) plan->target[i] = -reserved_i[i];
                }
                double same_direction = 0.0;
                for (uint8_t i = 0; i < bess->count; i++) {
                    if (requested_direction < 0 && plan->target[i] < 0.0) {
                        same_direction += -plan->target[i];
                    }
                    if (requested_direction > 0 && plan->target[i] > 0.0) {
                        same_direction += plan->target[i];
                    }
                }
                shortfall = demand - same_direction;
                if (shortfall < BESS_EPS) shortfall = 0.0;
                /* A reserve larger than the replaced share makes total charging
                 * exceed the request: the battery-protection override. */
                if (same_direction > demand + BESS_EPS) override = true;
            }
        }
    }

    plan->shortfall = shortfall;
    plan->low_soc_override = override;
    plan->no_eligible = no_eligible;
}

static void bess_run_cycle(iems_bess_t *bess, double requested_kw, uint64_t request_id,
                           iems_bess_result_t *r) {
    memset(r, 0, sizeof(*r));
    r->status = IEMS_BESS_OK;
    r->requested_kw = requested_kw;
    r->request_id = request_id;
    r->config_epoch = bess->config_epoch;
    r->cabinet_count = bess->count;

    double zero_targets[IEMS_BESS_MAX_CABINETS] = {0.0};
    iems_bess_cab_state_t states[IEMS_BESS_MAX_CABINETS];
    iems_bess_reason_t reasons[IEMS_BESS_MAX_CABINETS];

    if (bess->site_tripped) {
        r->status = bess_site_status(bess->site_fault_bits);
        bess_fill_cabinet_defaults(bess, states, reasons, IEMS_BESS_CAB_FAULT,
                                   IEMS_BESS_REASON_TRIPPED);
        bess_apply_plan(bess, bess_now(bess), zero_targets, states, reasons, true, false,
                        false, r);
        bess->state = IEMS_BESS_STATE_TRIPPED;
        return;
    }

    if (!bess->desired_on || !bess->desired_connected) {
        bess_fill_cabinet_defaults(bess, states, reasons, IEMS_BESS_CAB_OFF,
                                   IEMS_BESS_REASON_OFF);
        bess_apply_plan(bess, bess_now(bess), zero_targets, states, reasons, false, false,
                        true, r);
        bess->state = IEMS_BESS_STATE_READY;
        return;
    }

    uint64_t now = bess_now(bess);
    uint64_t input_epoch = 0;
    bool any_data = false;
    bess_collect(bess, now, &input_epoch, &any_data);
    r->input_epoch = input_epoch;

    if (!any_data) {
        r->status = IEMS_BESS_NO_DATA;
        bess_fill_cabinet_defaults(bess, states, reasons, IEMS_BESS_CAB_STALE,
                                   IEMS_BESS_REASON_STALE);
        bess_apply_plan(bess, now, zero_targets, states, reasons, false, false, false, r);
        bess->state = IEMS_BESS_STATE_INIT;
        return;
    }

    bess_plan_t plan;
    bess_build_plan(bess, requested_kw, &plan);
    r->low_soc_override = plan.low_soc_override;
    if (plan.no_eligible) {
        r->status = IEMS_BESS_NO_ELIGIBLE;
    } else if (plan.shortfall > BESS_EPS) {
        r->status = IEMS_BESS_SHORTFALL;
    } else {
        r->status = IEMS_BESS_OK;
    }
    r->shortfall_kw = plan.shortfall;

    bess_apply_plan(bess, now, plan.target, plan.states, plan.reasons, false, false, true,
                    r);

    bool degraded = false;
    for (uint8_t i = 0; i < bess->count; i++) {
        iems_bess_cab_state_t s = r->cabinet[i].state;
        if (s == IEMS_BESS_CAB_FAULT || s == IEMS_BESS_CAB_FAULT_LATCHED ||
            s == IEMS_BESS_CAB_OFFLINE || s == IEMS_BESS_CAB_STALE ||
            s == IEMS_BESS_CAB_COMMAND_FAILED) {
            degraded = true;
        }
    }
    if (fabs(requested_kw) <= BESS_EPS) {
        bess->state = r->status == IEMS_BESS_OK ? IEMS_BESS_STATE_READY : IEMS_BESS_STATE_DEGRADED;
    } else if (r->status == IEMS_BESS_OK && !plan.low_soc_override && !degraded) {
        bess->state = IEMS_BESS_STATE_DISPATCHING;
    } else {
        bess->state = IEMS_BESS_STATE_DEGRADED;
    }
}

static int bess_apply_cycle(iems_bess_t *bess, double requested_kw, uint64_t request_id,
                            iems_bess_result_t *out) {
    iems_bess_result_t r;
    bess_run_cycle(bess, requested_kw, request_id, &r);
    return bess_finish(bess, &r, out);
}

static int bess_command_gate(iems_bess_t *bess, bess_cmd_kind_t kind, double value,
                             uint64_t request_id, bool *cached) {
    *cached = false;
    if (!bess->have_command) return 0;
    if (request_id < bess->command_id) return IEMS_BESS_STALE_REQUEST;
    if (request_id == bess->command_id) {
        if (bess->command_kind == kind && bess->command_value == value) {
            *cached = true;
            return 0;
        }
        return IEMS_BESS_INVALID;
    }
    return 0;
}

static void bess_command_store(iems_bess_t *bess, bess_cmd_kind_t kind, double value,
                               uint64_t request_id, const iems_bess_result_t *result) {
    bess->have_command = true;
    bess->command_id = request_id;
    bess->command_kind = kind;
    bess->command_value = value;
    bess->command_result = *result;
}

int iems_bess_request_power(iems_bess_t *bess, double requested_kw,
                            uint64_t request_id, iems_bess_result_t *out) {
    if (!bess_handle_ok(bess) || !out) return IEMS_BESS_INVALID;
    if (!isfinite(requested_kw)) return IEMS_BESS_INVALID;
    bool cached = false;
    int rc = bess_command_gate(bess, BESS_CMD_POWER, requested_kw, request_id, &cached);
    if (rc < 0) return rc;
    if (cached) {
        *out = bess->command_result;
        return 0;
    }
    rc = bess_apply_cycle(bess, requested_kw, request_id, out);
    if (rc >= 0) {
        bess->have_power = true;
        bess->last_power_kw = requested_kw;
        bess->last_power_id = request_id;
        bess_command_store(bess, BESS_CMD_POWER, requested_kw, request_id, out);
    }
    return rc;
}

int iems_bess_tick(iems_bess_t *bess, iems_bess_result_t *out) {
    if (!bess_handle_ok(bess) || !out) return IEMS_BESS_INVALID;
    double requested = bess->have_power ? bess->last_power_kw : 0.0;
    uint64_t request_id = bess->have_power ? bess->last_power_id : 0;
    return bess_apply_cycle(bess, requested, request_id, out);
}

int iems_bess_set_onoff(iems_bess_t *bess, bool on, uint64_t request_id,
                        iems_bess_result_t *out) {
    if (!bess_handle_ok(bess) || !out) return IEMS_BESS_INVALID;
    double value = on ? 1.0 : 0.0;
    bool cached = false;
    int rc = bess_command_gate(bess, BESS_CMD_ONOFF, value, request_id, &cached);
    if (rc < 0) return rc;
    if (cached) {
        *out = bess->command_result;
        return 0;
    }
    bess->desired_on = on;
    rc = bess_apply_cycle(bess, bess->have_power ? bess->last_power_kw : 0.0, request_id,
                          out);
    if (rc >= 0) bess_command_store(bess, BESS_CMD_ONOFF, value, request_id, out);
    return rc;
}

int iems_bess_set_connect(iems_bess_t *bess, bool grid_connected, uint64_t request_id,
                          iems_bess_result_t *out) {
    if (!bess_handle_ok(bess) || !out) return IEMS_BESS_INVALID;
    double value = grid_connected ? 1.0 : 0.0;
    bool cached = false;
    int rc = bess_command_gate(bess, BESS_CMD_CONNECT, value, request_id, &cached);
    if (rc < 0) return rc;
    if (cached) {
        *out = bess->command_result;
        return 0;
    }
    bess->desired_connected = grid_connected;
    rc = bess_apply_cycle(bess, bess->have_power ? bess->last_power_kw : 0.0, request_id,
                          out);
    if (rc >= 0) bess_command_store(bess, BESS_CMD_CONNECT, value, request_id, out);
    return rc;
}

int iems_bess_set_site_fault(iems_bess_t *bess, uint32_t fault_bits) {
    if (!bess_handle_ok(bess)) return IEMS_BESS_INVALID;
    bool was_tripped = bess->site_tripped;
    bess->site_fault_bits = fault_bits;
    if (was_tripped || fault_bits == 0u) return 0;

    bess->site_tripped = true;
    iems_bess_result_t r;
    memset(&r, 0, sizeof(r));
    r.status = bess_site_status(fault_bits);
    r.config_epoch = bess->config_epoch;
    r.cabinet_count = bess->count;
    double zero_targets[IEMS_BESS_MAX_CABINETS] = {0.0};
    iems_bess_cab_state_t states[IEMS_BESS_MAX_CABINETS];
    iems_bess_reason_t reasons[IEMS_BESS_MAX_CABINETS];
    bess_fill_cabinet_defaults(bess, states, reasons, IEMS_BESS_CAB_FAULT,
                               IEMS_BESS_REASON_TRIPPED);
    bess_apply_plan(bess, bess_now(bess), zero_targets, states, reasons, true, true, false,
                    &r);
    bess->state = IEMS_BESS_STATE_TRIPPED;
    r.state = IEMS_BESS_STATE_TRIPPED;
    bess->last_result = r;
    bess->have_result = true;
    return 0;
}

int iems_bess_reset_faults(iems_bess_t *bess, uint8_t cabinet_id) {
    if (!bess_handle_ok(bess) || cabinet_id < 1 || cabinet_id > bess->count) {
        return IEMS_BESS_INVALID;
    }
    bess_cabinet_t *cab = &bess->cabinets[cabinet_id - 1u];
    cab->latched_faults = 0u;
    cab->consecutive_failures = 0;
    return 0;
}

int iems_bess_reset_site_faults(iems_bess_t *bess) {
    if (!bess_handle_ok(bess)) return IEMS_BESS_INVALID;
    if (bess->site_fault_bits != 0u) return IEMS_BESS_TRIPPED;
    bess->site_tripped = false;
    return 0;
}

int iems_bess_set_soc_window(iems_bess_t *bess, const iems_bess_soc_window_t *w) {
    if (!bess_handle_ok(bess) || !w || !bess_soc_window_valid(w)) return IEMS_BESS_INVALID;
    bess->cfg.soc_window = *w;
    bess->config_epoch++;
    return 0;
}

int iems_bess_set_low_soc(iems_bess_t *bess, bool enable, double threshold_pct,
                          double power_kw) {
    if (!bess_handle_ok(bess)) return IEMS_BESS_INVALID;
    if (!isfinite(threshold_pct) || threshold_pct < 0.0 || threshold_pct > 100.0) {
        return IEMS_BESS_INVALID;
    }
    if (!isfinite(power_kw) || power_kw < 0.0) return IEMS_BESS_INVALID;
    bess->cfg.low_soc_enable = enable;
    bess->cfg.low_soc_threshold_pct = threshold_pct;
    bess->cfg.low_soc_power_kw = power_kw;
    bess->config_epoch++;
    return 0;
}

int iems_bess_get_last_result(const iems_bess_t *bess, iems_bess_result_t *out) {
    if (!bess_handle_ok(bess) || !out || !bess->have_result) return IEMS_BESS_INVALID;
    *out = bess->last_result;
    return 0;
}
