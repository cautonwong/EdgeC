/**
 * @file dcu_peripheral_telecontrol.c
 * @brief Production SGCC/CSG Peripheral Control & "Four-Remotes" (四遥: YC/YX/YK/YT) Interface Engine Implementation
 */

#include "dcu_peripheral_telecontrol.h"
#include <string.h>

void dcu_telecontrol_init(dcu_telecontrol_manager_t* mgr, const dcu_io_ops_t* ops) {
    if (!mgr) return;
    memset(mgr, 0, sizeof(dcu_telecontrol_manager_t));
    if (ops) {
        mgr->ops = *ops;
    }
    mgr->breaker_state = BREAKER_STATE_CLOSED;
}

int dcu_telecontrol_scan_yx(dcu_telecontrol_manager_t* mgr) {
    if (!mgr) return -1;

    for (uint8_t i = 0; i < DCU_MAX_DI_CHANNELS; i++) {
        if (mgr->ops.read_di) {
            bool state = false;
            if (mgr->ops.read_di(i, &state, mgr->ops.user_data) == 0) {
                mgr->di_states[i] = state;
            }
        }
    }

    // Map standard DI channels to semantic states
    mgr->breaker_state = mgr->di_states[0] ? BREAKER_STATE_CLOSED : BREAKER_STATE_OPEN;
    mgr->surge_protector_tripped = mgr->di_states[1];
    mgr->cabinet_door_open = mgr->di_states[2];
    mgr->smoke_alarm_active = mgr->di_states[3];

    return 0;
}

int dcu_telecontrol_exec_yk(dcu_telecontrol_manager_t* mgr, uint8_t do_channel, dcu_yk_action_t action) {
    if (!mgr || do_channel >= DCU_MAX_DO_CHANNELS) return -1;

    bool target_state = (action == YK_ACTION_CLOSE);
    if (mgr->ops.set_do) {
        int rc = mgr->ops.set_do(do_channel, target_state, mgr->ops.user_data);
        if (rc != 0) return rc;
    }

    mgr->do_states[do_channel] = target_state;
    if (do_channel == 0) {
        mgr->breaker_state = target_state ? BREAKER_STATE_CLOSED : BREAKER_STATE_OPEN;
    }
    return 0;
}

int dcu_telecontrol_switch_capacitor(dcu_telecontrol_manager_t* mgr, uint8_t bank_idx, bool enable) {
    if (!mgr || bank_idx >= DCU_MAX_CAPACITOR_BANKS) return -1;

    uint8_t do_channel = (uint8_t)(4 + bank_idx); // Channels 4..7 map to capacitor banks 0..3
    if (do_channel < DCU_MAX_DO_CHANNELS && mgr->ops.set_do) {
        mgr->ops.set_do(do_channel, enable, mgr->ops.user_data);
    }
    mgr->capacitor_bank_states[bank_idx] = enable;
    return 0;
}

int dcu_telecontrol_regulate_oltc(dcu_telecontrol_manager_t* mgr, int8_t target_tap) {
    if (!mgr) return -1;
    if (target_tap < -8 || target_tap > 8) return -2; // Out of range

    if (mgr->ops.set_oltc_tap) {
        int rc = mgr->ops.set_oltc_tap(target_tap, mgr->ops.user_data);
        if (rc != 0) return rc;
    }
    mgr->current_oltc_tap = target_tap;
    return 0;
}
