/**
 * @file edge_bus_resilience.c
 * @brief Industrial Bus Resilience, Half-Duplex Flow Control & CAN Bus-Off Auto-Recovery Engine Implementation (AEP-462)
 */

#include "edge_bus_resilience.h"

#include <string.h>

/* --- CAN Auto-Recovery Implementation --- */

void edge_can_resilience_init(edge_can_resilience_mgr_t *mgr, bool auto_recovery) {
    if (!mgr) return;
    memset(mgr, 0, sizeof(*mgr));
    mgr->state = EDGE_CAN_STATE_ERROR_ACTIVE;
    mgr->auto_recovery_enabled = auto_recovery;
}

static void _update_can_state(edge_can_resilience_mgr_t *mgr) {
    if (mgr->state == EDGE_CAN_STATE_RECOVERING) {
        return; // In recovery sequence
    }

    if (mgr->tec > 255) {
        mgr->state = EDGE_CAN_STATE_BUS_OFF;
        mgr->bus_off_count++;
        if (mgr->auto_recovery_enabled) {
            mgr->state = EDGE_CAN_STATE_RECOVERING;
            mgr->consecutive_recessive_bits = 0;
        }
    } else if (mgr->tec > 127 || mgr->rec > 127) {
        mgr->state = EDGE_CAN_STATE_ERROR_PASSIVE;
    } else if (mgr->tec >= 96 || mgr->rec >= 96) {
        mgr->state = EDGE_CAN_STATE_ERROR_WARNING;
    } else {
        mgr->state = EDGE_CAN_STATE_ERROR_ACTIVE;
    }
}

void edge_can_record_tx_error(edge_can_resilience_mgr_t *mgr, uint8_t err_increment) {
    if (!mgr) return;
    mgr->tec += (uint16_t)err_increment;
    _update_can_state(mgr);
}

void edge_can_record_rx_error(edge_can_resilience_mgr_t *mgr, uint8_t err_increment) {
    if (!mgr) return;
    mgr->rec += (uint16_t)err_increment;
    _update_can_state(mgr);
}

void edge_can_record_tx_success(edge_can_resilience_mgr_t *mgr) {
    if (!mgr) return;
    if (mgr->tec > 0) mgr->tec--;
    _update_can_state(mgr);
}

void edge_can_record_rx_success(edge_can_resilience_mgr_t *mgr) {
    if (!mgr) return;
    if (mgr->rec > 0) mgr->rec--;
    _update_can_state(mgr);
}

bool edge_can_step_recovery(edge_can_resilience_mgr_t *mgr) {
    if (!mgr || mgr->state != EDGE_CAN_STATE_RECOVERING) return false;

    // ISO 11898-1: 128 occurrences of 11 consecutive recessive bits
    mgr->consecutive_recessive_bits++;
    if (mgr->consecutive_recessive_bits >= 128) {
        mgr->tec = 0;
        mgr->rec = 0;
        mgr->consecutive_recessive_bits = 0;
        mgr->state = EDGE_CAN_STATE_ERROR_ACTIVE;
        mgr->auto_recover_count++;
        return true; // Successfully recovered!
    }
    return false;
}

/* --- RS-485 Half-Duplex Flow Control Implementation --- */

uint32_t edge_rs485_compute_t35_us(uint32_t baud_rate) {
    if (baud_rate > 19200) {
        return 1750; // Modbus standard: 1.75ms for baud > 19200
    }
    // 3.5 characters * 11 bits/char * 1,000,000 / baud
    return (uint32_t)(38500000ULL / baud_rate);
}

void edge_rs485_flow_ctrl_init(edge_rs485_flow_ctrl_t *ctrl, uint32_t baud_rate) {
    if (!ctrl) return;
    memset(ctrl, 0, sizeof(*ctrl));
    ctrl->baud_rate = baud_rate;
    ctrl->post_tx_delay_us = edge_rs485_compute_t35_us(baud_rate);
    ctrl->pre_tx_delay_us = 50; // 50us transceiver stabilization
    ctrl->turnaround_us = 100;
}

void edge_rs485_start_tx(edge_rs485_flow_ctrl_t *ctrl) {
    if (!ctrl) return;
    ctrl->rts_asserted = true;
}

void edge_rs485_end_tx(edge_rs485_flow_ctrl_t *ctrl) {
    if (!ctrl) return;
    ctrl->rts_asserted = false;
}

/* --- Adaptive Slave Fault Backoff Implementation --- */

void edge_bus_backoff_init(edge_bus_backoff_mgr_t *mgr) {
    if (!mgr) return;
    memset(mgr, 0, sizeof(*mgr));
}

int edge_bus_backoff_register_slave(edge_bus_backoff_mgr_t *mgr, uint32_t slave_id,
                                    uint32_t base_interval_ms, uint32_t max_interval_ms) {
    if (!mgr || mgr->slave_count >= EDGE_BUS_MAX_SLAVES) return -1;
    for (uint32_t i = 0; i < mgr->slave_count; i++) {
        if (mgr->slaves[i].slave_id == slave_id) return 0; // already registered
    }

    edge_slave_backoff_t *s = &mgr->slaves[mgr->slave_count++];
    s->slave_id = slave_id;
    s->consecutive_timeouts = 0;
    s->base_poll_interval_ms = base_interval_ms;
    s->current_poll_interval_ms = base_interval_ms;
    s->max_poll_interval_ms = max_interval_ms ? max_interval_ms : base_interval_ms * 16;
    s->is_isolated = false;
    return 0;
}

void edge_bus_backoff_record_timeout(edge_bus_backoff_mgr_t *mgr, uint32_t slave_id) {
    if (!mgr) return;
    for (uint32_t i = 0; i < mgr->slave_count; i++) {
        edge_slave_backoff_t *s = &mgr->slaves[i];
        if (s->slave_id == slave_id) {
            s->consecutive_timeouts++;
            if (s->consecutive_timeouts >= 3) {
                // Exponential backoff
                s->current_poll_interval_ms *= 2;
                if (s->current_poll_interval_ms > s->max_poll_interval_ms) {
                    s->current_poll_interval_ms = s->max_poll_interval_ms;
                    s->is_isolated = true;
                }
            }
            return;
        }
    }
}

void edge_bus_backoff_record_success(edge_bus_backoff_mgr_t *mgr, uint32_t slave_id) {
    if (!mgr) return;
    for (uint32_t i = 0; i < mgr->slave_count; i++) {
        edge_slave_backoff_t *s = &mgr->slaves[i];
        if (s->slave_id == slave_id) {
            s->consecutive_timeouts = 0;
            s->current_poll_interval_ms = s->base_poll_interval_ms;
            s->is_isolated = false;
            return;
        }
    }
}

uint32_t edge_bus_backoff_get_interval(const edge_bus_backoff_mgr_t *mgr, uint32_t slave_id) {
    if (!mgr) return 1000;
    for (uint32_t i = 0; i < mgr->slave_count; i++) {
        if (mgr->slaves[i].slave_id == slave_id) {
            return mgr->slaves[i].current_poll_interval_ms;
        }
    }
    return 1000;
}
