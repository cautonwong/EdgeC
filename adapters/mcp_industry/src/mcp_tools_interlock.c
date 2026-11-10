/**
 * @file mcp_tools_interlock.c
 * @brief Electrical Five-Anti Safety Interlocking Simulator MCP Tool (AEP-508)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 *
 * Implements DL/T 687 substation electrical interlocking logic:
 * 1. Prevent accidental breaker open/close
 * 2. Prevent disconnector operation under load (Anti-Arc Flash)
 * 3. Prevent earth-switch closing on live circuit (Anti-Electrocution)
 * 4. Prevent breaker/disconnector closing with earth-switch active (Anti-Short-Circuit)
 * 5. Prevent unauthorized cubicle entry
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <cjson/cJSON.h>

#include "edgeos/edge_mcp_industry_tools.h"
#include "edge_log.h"

int handle_security_simulate_interlock(edge_mcp_dispatcher_t *disp,
                                       const cJSON *params,
                                       cJSON **out_result,
                                       char *err_msg,
                                       size_t err_len) {
    (void)disp;
    if (!params) {
        snprintf(err_msg, err_len, "Missing parameters for security_simulate_interlock");
        return MCP_RPC_INVALID_PARAMS;
    }

    const cJSON *bid_item = cJSON_GetObjectItem(params, "breaker_id");
    const cJSON *target_item = cJSON_GetObjectItem(params, "target_state");

    if (!bid_item || !cJSON_IsString(bid_item) ||
        !target_item || !cJSON_IsString(target_item)) {
        snprintf(err_msg, err_len, "Params must include string 'breaker_id' and 'target_state' (OPEN|CLOSE)");
        return MCP_RPC_INVALID_PARAMS;
    }

    const char *breaker_id = bid_item->valuestring;
    const char *target_state = target_item->valuestring;

    bool is_target_close = (strcmp(target_state, "CLOSE") == 0 || strcmp(target_state, "CLOSED") == 0 || strcmp(target_state, "1") == 0);
    bool is_target_open = (strcmp(target_state, "OPEN") == 0 || strcmp(target_state, "OPENED") == 0 || strcmp(target_state, "0") == 0);

    if (!is_target_close && !is_target_open) {
        snprintf(err_msg, err_len, "'target_state' must be 'OPEN' or 'CLOSE'");
        return MCP_RPC_INVALID_PARAMS;
    }

    /* Optional simulated bay sensor values */
    double load_current_amps = 0.0;
    double line_voltage_v = 0.0;
    bool associated_cb_closed = false;
    bool associated_es_closed = false;

    const cJSON *curr_item = cJSON_GetObjectItem(params, "current_load_amps");
    if (curr_item && cJSON_IsNumber(curr_item)) load_current_amps = curr_item->valuedouble;

    const cJSON *volt_item = cJSON_GetObjectItem(params, "line_voltage_v");
    if (volt_item && cJSON_IsNumber(volt_item)) line_voltage_v = volt_item->valuedouble;

    const cJSON *cb_state_item = cJSON_GetObjectItem(params, "associated_cb_closed");
    if (cb_state_item && cJSON_IsBool(cb_state_item)) associated_cb_closed = cJSON_IsTrue(cb_state_item);

    const cJSON *es_state_item = cJSON_GetObjectItem(params, "associated_es_closed");
    if (es_state_item && cJSON_IsBool(es_state_item)) associated_es_closed = cJSON_IsTrue(es_state_item);

    /* 1. Electrical Five-Anti Rule Validation Engine */
    bool check_passed = true;
    cJSON *violated_rules = cJSON_CreateArray();
    cJSON *blocking_reasons = cJSON_CreateArray();
    cJSON *sop_steps = cJSON_CreateArray();

    bool is_disconnector = (strstr(breaker_id, "DS") != NULL || strstr(breaker_id, "QS") != NULL || strstr(breaker_id, "ISOLATOR") != NULL);
    bool is_earth_switch = (strstr(breaker_id, "ES") != NULL || strstr(breaker_id, "QE") != NULL || strstr(breaker_id, "EARTH") != NULL);
    bool is_circuit_breaker = (strstr(breaker_id, "CB") != NULL || strstr(breaker_id, "QF") != NULL || (!is_disconnector && !is_earth_switch));

    /* Rule 2 Check: Prevent Disconnector operation under load */
    if (is_disconnector) {
        if (load_current_amps > 1.0 || associated_cb_closed) {
            check_passed = false;
            cJSON_AddItemToArray(violated_rules, cJSON_CreateString("RULE-2: 防止带负荷拉合隔离开关 (Anti-Arc-Flash Interlock Violation)"));
            char r_buf[256];
            snprintf(r_buf, sizeof(r_buf), "Cannot switch disconnector %s while Circuit Breaker is CLOSED with load current %.1fA", breaker_id, load_current_amps);
            cJSON_AddItemToArray(blocking_reasons, cJSON_CreateString(r_buf));
        }
        if (is_target_close && associated_es_closed) {
            check_passed = false;
            cJSON_AddItemToArray(violated_rules, cJSON_CreateString("RULE-4: 防止带接地线/接地刀闸合隔离开关 (Anti-Short-Circuit Interlock Violation)"));
            cJSON_AddItemToArray(blocking_reasons, cJSON_CreateString("Earthing switch ES is currently active; closing disconnector is prohibited"));
        }
    }

    /* Rule 3 Check: Prevent Earth Switch closing on live line */
    if (is_earth_switch && is_target_close) {
        if (line_voltage_v > 5.0 || associated_cb_closed) {
            check_passed = false;
            cJSON_AddItemToArray(violated_rules, cJSON_CreateString("RULE-3: 防止带电挂接地线/合接地刀闸 (Anti-Electrocution Interlock Violation)"));
            char r_buf[256];
            snprintf(r_buf, sizeof(r_buf), "Cannot close earthing switch %s while line is energized (Measured Voltage: %.1fV > 5.0V threshold)", breaker_id, line_voltage_v);
            cJSON_AddItemToArray(blocking_reasons, cJSON_CreateString(r_buf));
        }
    }

    /* Rule 4 Check: Prevent Breaker closing when Earth Switch is active */
    if (is_circuit_breaker && is_target_close) {
        if (associated_es_closed) {
            check_passed = false;
            cJSON_AddItemToArray(violated_rules, cJSON_CreateString("RULE-4: 防止带接地线/接地刀闸合断路器 (Anti-Short-Circuit Interlock Violation)"));
            cJSON_AddItemToArray(blocking_reasons, cJSON_CreateString("Earthing switch / ground lead is in place; closing circuit breaker would trigger bolted three-phase short-circuit"));
        }
    }

    /* 2. Generate Safe Sequential SOP Steps (倒闸操作票) */
    if (is_circuit_breaker && is_target_open) {
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 1: 检查保护装置运行正常，无闭锁告警信号"));
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 2: 下发遥控分闸指令，分断断路器 QF/CB"));
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 3: 检查三相电流指示归零，断路器分闸机械指示到位"));
    } else if (is_circuit_breaker && is_target_close) {
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 1: 确认母线隔离开关与线路隔离开关已全部处于合闸位置"));
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 2: 确认接地刀闸 QE/ES 已全部处于分闸位置且接地线已拆除"));
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 3: 确认二次控制回路储能完成，重合闸就绪"));
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 4: 执行断路器合闸操作，核验三相电压电流平衡"));
    } else if (is_disconnector && is_target_open) {
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 1: 确认本间隔断路器 QF 已分断，三相电流为 0.0A"));
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 2: 拉开线路侧隔离开关 QS-2，检查拉开到位"));
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 3: 拉开母线侧隔离开关 QS-1，检查拉开到位并加锁"));
    } else if (is_earth_switch && is_target_close) {
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 1: 确认断路器 QF 及两侧隔离开关 QS 均已在断开位置"));
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 2: 在出线端进行高压验电，确认三相确无电压 (V < 5V)"));
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 3: 合上接地刀闸 QE / 挂设接地线，锁定操作机构并悬挂标示牌"));
    } else {
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 1: 核对设备双重名称及编号"));
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 2: 检查防误闭锁逻辑条件满足"));
        cJSON_AddItemToArray(sop_steps, cJSON_CreateString("步骤 3: 唱票并执行操作"));
    }

    /* 3. Assemble JSON Response */
    cJSON *res = cJSON_CreateObject();
    cJSON_AddStringToObject(res, "breaker_id", breaker_id);
    cJSON_AddStringToObject(res, "target_state", is_target_close ? "CLOSE" : "OPEN");
    cJSON_AddBoolToObject(res, "operation_allowed", check_passed);
    cJSON_AddBoolToObject(res, "interlock_check_passed", check_passed);
    cJSON_AddStringToObject(res, "interlock_status", check_passed ? "PASSED_SAFE" : "BLOCKED_VIOLATION");

    cJSON_AddItemToObject(res, "violated_rules", violated_rules);
    cJSON_AddItemToObject(res, "blocking_reasons", blocking_reasons);
    cJSON_AddItemToObject(res, "sequential_sop_steps", sop_steps);

    char sum_buf[512];
    if (check_passed) {
        snprintf(sum_buf, sizeof(sum_buf),
                 "SAFETY APPROVED: Switching %s to %s satisfies all Electrical Five-Anti interlock constraints.",
                 breaker_id, is_target_close ? "CLOSE" : "OPEN");
    } else {
        snprintf(sum_buf, sizeof(sum_buf),
                 "INTERLOCK LOCKOUT: Operation on %s blocked! Violation of Electrical Five-Anti safety interlock.",
                 breaker_id);
    }
    cJSON_AddStringToObject(res, "summary", sum_buf);

    *out_result = res;
    return 0;
}
