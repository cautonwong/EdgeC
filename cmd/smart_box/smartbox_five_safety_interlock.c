/**
 * @file smartbox_five_safety_interlock.c
 * @brief SmartBox Switchgear Electrical Five-Safety Intelligent Interlocking Implementation
 */

#include "smartbox_five_safety_interlock.h"
#include <string.h>

void smartbox_interlock_bay_init(smartbox_switchgear_bay_t *bay, uint32_t bay_id) {
    if (!bay) return;
    memset(bay, 0, sizeof(smartbox_switchgear_bay_t));
    bay->bay_id = bay_id;
}

interlock_result_t smartbox_interlock_evaluate(const smartbox_switchgear_bay_t *bay,
                                              interlock_operation_type_t op,
                                              const char **out_reason) {
    if (!bay) {
        if (out_reason) *out_reason = "Null bay context";
        return INTERLOCK_BLOCK_INVALID_OP;
    }

    if (bay->is_lock_override_key_active) {
        if (out_reason) *out_reason = "Manual override key active";
        return INTERLOCK_PERMITTED;
    }

    switch (op) {
        // 1. 合断路器
        case INTERLOCK_OP_CLOSE_BREAKER:
            // 防带地线合断路器: 接地刀闸必须处于分位
            if (bay->is_earth_switch_closed) {
                if (out_reason) *out_reason = "Rule 4 Block: Earth switch is closed";
                return INTERLOCK_BLOCK_EARTHED_CLOSING;
            }
            if (out_reason) *out_reason = "Permitted: Breaker close safe";
            return INTERLOCK_PERMITTED;

        // 2. 分断路器 (正常分闸无条件允许)
        case INTERLOCK_OP_OPEN_BREAKER:
            if (out_reason) *out_reason = "Permitted: Breaker open allowed";
            return INTERLOCK_PERMITTED;

        // 3. 合母线/出线隔离开关
        case INTERLOCK_OP_CLOSE_BUS_ISOLATOR:
        case INTERLOCK_OP_CLOSE_LINE_ISOLATOR:
            // 防带负荷合刀闸: 断路器必须处于分闸状态
            if (bay->is_breaker_closed) {
                if (out_reason) *out_reason = "Rule 2 Block: Cannot close isolator while breaker is closed (load present)";
                return INTERLOCK_BLOCK_LOADED_ISOLATOR;
            }
            // 防带地线合刀闸: 接地刀闸必须处于分闸状态
            if (bay->is_earth_switch_closed) {
                if (out_reason) *out_reason = "Rule 4 Block: Cannot close isolator while earth switch is closed";
                return INTERLOCK_BLOCK_EARTHED_CLOSING;
            }
            if (out_reason) *out_reason = "Permitted: Isolator close safe";
            return INTERLOCK_PERMITTED;

        // 4. 分母线/出线隔离开关
        case INTERLOCK_OP_OPEN_BUS_ISOLATOR:
        case INTERLOCK_OP_OPEN_LINE_ISOLATOR:
            // 防带负荷拉刀闸: 断路器必须处于分闸状态
            if (bay->is_breaker_closed) {
                if (out_reason) *out_reason = "Rule 2 Block: Cannot open isolator while breaker is closed (load present)";
                return INTERLOCK_BLOCK_LOADED_ISOLATOR;
            }
            if (out_reason) *out_reason = "Permitted: Isolator open safe";
            return INTERLOCK_PERMITTED;

        // 5. 合接地刀闸
        case INTERLOCK_OP_CLOSE_EARTH_SWITCH:
            // 防带电合接地刀闸: 隔离开关与断路器必须全部分开，且高压带电指示器无电
            if (bay->is_breaker_closed || bay->is_bus_isolator_closed || bay->is_line_isolator_closed || bay->is_line_charged) {
                if (out_reason) *out_reason = "Rule 3 Block: Cannot close earth switch on charged line/closed switches";
                return INTERLOCK_BLOCK_CHARGED_EARTH;
            }
            if (out_reason) *out_reason = "Permitted: Earth switch close safe";
            return INTERLOCK_PERMITTED;

        // 6. 分接地刀闸
        case INTERLOCK_OP_OPEN_EARTH_SWITCH:
            if (out_reason) *out_reason = "Permitted: Earth switch open allowed";
            return INTERLOCK_PERMITTED;

        // 7. 开柜门/网门
        case INTERLOCK_OP_OPEN_CUBICLE_DOOR:
            // 防误入带电间隔: 必须回路不带电且接地刀闸已合闸
            if (bay->is_line_charged || !bay->is_earth_switch_closed) {
                if (out_reason) *out_reason = "Rule 5 Block: Cannot open cubicle door while line is charged or earth switch open";
                return INTERLOCK_BLOCK_CHARGED_DOOR_ENTRY;
            }
            if (out_reason) *out_reason = "Permitted: Cubicle door open safe";
            return INTERLOCK_PERMITTED;

        default:
            if (out_reason) *out_reason = "Unknown operation";
            return INTERLOCK_BLOCK_INVALID_OP;
    }
}
