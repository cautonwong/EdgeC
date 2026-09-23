/**
 * @file test_domain_scenarios.c
 * @brief AEP-354: Domain-Driven Design (DDD) business scenario end-to-end integration tests
 *        Covering DCU (LineLoss, Outage 3011) and IEMS (Anti-Reverse Flow, TOU Arbitrage & SOC
 * Envelope)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "datahub.h"
#include "edge_anti_corruption.h"
#include "shmring.h"

/* -------------------------------------------------------------------------- */
/* Tag ID and Mapping Helper                                                  */
/* -------------------------------------------------------------------------- */

#define TAG_DCU_TRANSFORMER_LOAD 65537  // 0x010001
#define TAG_DCU_LINELOSS_PCT 65538      // 0x010002
#define TAG_DCU_OUTAGE_STATUS 65539     // 0x010003
#define TAG_DCU_DI_BITMAP 65540         // 0x010004
#define TAG_DCU_DO_BITMAP 65541         // 0x010005

#define TAG_IEMS_PCS_MODE 131073         // 0x020001
#define TAG_IEMS_PCS_POWER_SET 131074    // 0x020002
#define TAG_IEMS_BMS_MAX_CELL_V 131075   // 0x020003
#define TAG_IEMS_BMS_MIN_CELL_V 131076   // 0x020004
#define TAG_IEMS_BMS_MAX_TEMP 131077     // 0x020005
#define TAG_IEMS_ANTI_REV_ACTIVE 131078  // 0x020006
#define TAG_IEMS_BMS_SOC 11001           // Standard Metrology Tag

static int domain_scenario_map_fn(uint32_t tag_id) {
    return (int)(tag_id % 1024);
}

/* -------------------------------------------------------------------------- */
/* Scenario 1: DCU Topology & 16-Feeder Dynamic Line Loss Settlement          */
/* -------------------------------------------------------------------------- */

static void test_dcu_lineloss_topology_scenario(void **state) {
    (void)state;

    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    assert_int_equal(datahub_init(&hub, &loop, "dcu_lineloss_hub", 1024, 1024, NULL, 0, NULL,
                                  domain_scenario_map_fn, NULL),
                     0);

    edge_anti_corruption_engine_t acl;
    assert_int_equal(edge_anti_corruption_init(&acl, 32), 0);

    // Register DCU points
    edge_anti_corruption_entry_t entry_loss = {.tag_id = TAG_DCU_LINELOSS_PCT,
                                               .domain_name = "dcu_distribution",
                                               .entity_prop = "topology.lineloss_pct",
                                               .proto = EDGE_PROTO_MODBUS,
                                               .raw_address = 0x4002,
                                               .scaler = 0.01,
                                               .offset = 0.0,
                                               .data_type = EDGE_ACL_TYPE_FLOAT};
    assert_int_equal(edge_anti_corruption_register(&acl, &entry_loss), 0);

    // 1. Simulate incoming transformer meter power: Pin = 120.0 kW
    double p_in_kw = 120.0;

    // 2. Simulate 16 feeder branches summing up to Pout = 115.8 kW
    double branch_powers[16] = {8.0, 7.5, 6.2, 9.1, 5.0, 8.3, 7.2, 6.5,
                                8.0, 7.5, 6.2, 9.1, 5.0, 8.3, 7.2, 6.7};
    double p_out_kw = 0.0;
    for (int i = 0; i < 16; i++) {
        p_out_kw += branch_powers[i];
    }
    assert_float_equal((float)p_out_kw, 115.8f, 0.01f);

    // 3. Domain algorithm: Calculate loss rate
    double loss_kw = p_in_kw - p_out_kw;                 // 4.2 kW
    double loss_rate_pct = (loss_kw / p_in_kw) * 100.0;  // 3.5%
    assert_float_equal((float)loss_rate_pct, 3.5f, 0.01f);

    // 4. Translate and write to DataHub via ACL
    pipe_msg_t loss_msg;
    // Raw value in register = 350 (3.5% / 0.01)
    assert_int_equal(
        edge_anti_corruption_translate_inbound(&acl, EDGE_PROTO_MODBUS, 0x4002, 350.0, &loss_msg),
        0);
    assert_int_equal(datahub_write_shadow(&hub, &loss_msg), 0);

    // 5. Read back from DataHub and verify settlement status
    pipe_msg_t readback;
    assert_int_equal(datahub_read_shadow(&hub, TAG_DCU_LINELOSS_PCT, &readback), 0);
    assert_float_equal(readback.payload.val.f, 3.5f, 0.01f);

    // Verify loss is within healthy threshold (< 7.0%)
    bool is_loss_alarm = (readback.payload.val.f > 7.0f);
    assert_false(is_loss_alarm);

    edge_anti_corruption_destroy(&acl);
    datahub_destroy(&hub);
    while (uv_run(&loop, UV_RUN_NOWAIT))
        ;
    uv_loop_close(&loop);
}

/* -------------------------------------------------------------------------- */
/* Scenario 2: DCU Outage Proactive Detection & 0x3011 SOE Snapshot           */
/* -------------------------------------------------------------------------- */

static void test_dcu_outage_soe_3011_scenario(void **state) {
    (void)state;

    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    assert_int_equal(datahub_init(&hub, &loop, "dcu_outage_hub", 1024, 1024, NULL, 0, NULL,
                                  domain_scenario_map_fn, NULL),
                     0);

    // 1. Initial State: Normal 3-Phase Grid Voltage (220.0V)
    pipe_msg_t v_normal;
    pipe_msg_init_scalar(&v_normal, 1001, 220.0, 1000000);
    assert_int_equal(datahub_write_shadow(&hub, &v_normal), 0);

    // 2. Power Outage Occurrence: Voltage drops to 0V
    pipe_msg_t v_outage;
    pipe_msg_init_scalar(&v_outage, 1001, 0.0, 1000100);
    assert_int_equal(datahub_write_shadow(&hub, &v_outage), 0);

    // 3. Outage Detector triggers 0x3011 SOE Event with microsecond timestamp
    uint64_t outage_ts_us = 1000100;
    uint32_t outage_event_code = 0x3011;  // DLT698 Outage Event Identifier

    pipe_msg_t outage_event_msg;
    pipe_msg_init_scalar(&outage_event_msg, TAG_DCU_OUTAGE_STATUS, (double)outage_event_code,
                         outage_ts_us);
    assert_int_equal(datahub_write_shadow(&hub, &outage_event_msg), 0);

    // 4. Assert DataHub shadow state reflects active 0x3011 event
    pipe_msg_t out;
    assert_int_equal(datahub_read_shadow(&hub, TAG_DCU_OUTAGE_STATUS, &out), 0);
    assert_int_equal((uint32_t)out.payload.val.f, 0x3011);
    assert_int_equal(out.ts, outage_ts_us);

    datahub_destroy(&hub);
    while (uv_run(&loop, UV_RUN_NOWAIT))
        ;
    uv_loop_close(&loop);
}

/* -------------------------------------------------------------------------- */
/* Scenario 3: IEMS Anti-Reverse Flow (Zero Export) Closed-Loop Response       */
/* -------------------------------------------------------------------------- */

static void test_iems_anti_reverse_flow_scenario(void **state) {
    (void)state;

    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    assert_int_equal(datahub_init(&hub, &loop, "iems_anti_rev_hub", 1024, 1024, NULL, 0, NULL,
                                  domain_scenario_map_fn, NULL),
                     0);

    edge_anti_corruption_engine_t acl;
    assert_int_equal(edge_anti_corruption_init(&acl, 32), 0);

    edge_anti_corruption_entry_t entry_power = {.tag_id = TAG_IEMS_PCS_POWER_SET,
                                                .domain_name = "iems_microgrid",
                                                .entity_prop = "pcs.active_power_setpoint",
                                                .proto = EDGE_PROTO_MODBUS,
                                                .raw_address = 0x5001,
                                                .scaler = 0.1,
                                                .offset = 0.0,
                                                .data_type = EDGE_ACL_TYPE_FLOAT};
    assert_int_equal(edge_anti_corruption_register(&acl, &entry_power), 0);

    // 1. Initial State: PCS Discharging at 80.0 kW
    double current_pcs_discharge_kw = 80.0;
    pipe_msg_t msg_pcs;
    pipe_msg_init_scalar(&msg_pcs, TAG_IEMS_PCS_POWER_SET, current_pcs_discharge_kw, 0);
    assert_int_equal(datahub_write_shadow(&hub, &msg_pcs), 0);

    // 2. PCC Grid Meter detects reverse power flow: -40.0 kW (exceeds -5.0 kW deadband)
    double pcc_export_power_kw = -40.0;
    double anti_reverse_deadband_kw = -5.0;
    bool is_reverse_flow = (pcc_export_power_kw < anti_reverse_deadband_kw);
    assert_true(is_reverse_flow);

    // 3. IEMS ESSContainer executes Anti-Reverse Control Law:
    // Target PCS Power = Current Power + Reverse Power = 80.0 + (-40.0) = 40.0 kW
    double adjusted_pcs_power_kw = current_pcs_discharge_kw + pcc_export_power_kw;
    assert_float_equal((float)adjusted_pcs_power_kw, 40.0f, 0.01f);

    // 4. Translate Outbound via ACL to Modbus Register Value
    uint32_t raw_reg_addr = 0;
    double raw_reg_val = 0.0;
    assert_int_equal(
        edge_anti_corruption_translate_outbound(&acl, TAG_IEMS_PCS_POWER_SET, adjusted_pcs_power_kw,
                                                &raw_reg_addr, &raw_reg_val),
        0);
    assert_int_equal(raw_reg_addr, 0x5001);
    assert_float_equal((float)raw_reg_val, 400.0f, 0.01f);  // 40.0 kW / 0.1 = 400

    // 5. Update DataHub shadow bus with active flag and adjusted setpoint
    pipe_msg_t msg_flag, msg_new_power;
    pipe_msg_init_scalar(&msg_flag, TAG_IEMS_ANTI_REV_ACTIVE, 1.0, 0);
    pipe_msg_init_scalar(&msg_new_power, TAG_IEMS_PCS_POWER_SET, adjusted_pcs_power_kw, 0);
    assert_int_equal(datahub_write_shadow(&hub, &msg_flag), 0);
    assert_int_equal(datahub_write_shadow(&hub, &msg_new_power), 0);

    // 6. Assert DataHub state
    pipe_msg_t read_flag, read_power;
    assert_int_equal(datahub_read_shadow(&hub, TAG_IEMS_ANTI_REV_ACTIVE, &read_flag), 0);
    assert_int_equal(datahub_read_shadow(&hub, TAG_IEMS_PCS_POWER_SET, &read_power), 0);
    assert_float_equal(read_flag.payload.val.f, 1.0f, 0.01f);
    assert_float_equal(read_power.payload.val.f, 40.0f, 0.01f);

    edge_anti_corruption_destroy(&acl);
    datahub_destroy(&hub);
    while (uv_run(&loop, UV_RUN_NOWAIT))
        ;
    uv_loop_close(&loop);
}

/* -------------------------------------------------------------------------- */
/* Scenario 4: IEMS TOU Dynamic Tariff Arbitrage & BMS SOC Safety Envelope    */
/* -------------------------------------------------------------------------- */

static void test_iems_tou_arbitrage_and_soc_envelope_scenario(void **state) {
    (void)state;

    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    assert_int_equal(datahub_init(&hub, &loop, "iems_tou_hub", 1024, 1024, NULL, 0, NULL,
                                  domain_scenario_map_fn, NULL),
                     0);

    // 1. Initial State: Peak Tariff Period (1.35 RMB/kWh) & Battery SOC = 45.0%
    double current_tariff_rmb = 1.35;
    double current_soc_pct = 45.0;
    double soc_min_safe_pct = 15.0;
    double soc_max_safe_pct = 95.0;

    pipe_msg_t msg_soc;
    pipe_msg_init_scalar(&msg_soc, TAG_IEMS_BMS_SOC, current_soc_pct, 0);
    assert_int_equal(datahub_write_shadow(&hub, &msg_soc), 0);

    // 2. Decision: Peak Tariff + SOC in safe envelope -> Trigger Discharge Arbitrage (150.0 kW)
    bool is_peak_period = (current_tariff_rmb > 1.00);
    bool is_soc_healthy =
        (current_soc_pct > soc_min_safe_pct && current_soc_pct < soc_max_safe_pct);
    assert_true(is_peak_period && is_soc_healthy);

    double discharge_power_target_kw = 150.0;
    pipe_msg_t msg_setpoint;
    pipe_msg_init_scalar(&msg_setpoint, TAG_IEMS_PCS_POWER_SET, discharge_power_target_kw, 0);
    assert_int_equal(datahub_write_shadow(&hub, &msg_setpoint), 0);

    // 3. Simulate battery discharging until SOC drops to safety lower bound: 14.5%
    double depleted_soc_pct = 14.5;
    pipe_msg_t msg_depleted_soc;
    pipe_msg_init_scalar(&msg_depleted_soc, TAG_IEMS_BMS_SOC, depleted_soc_pct, 0);
    assert_int_equal(datahub_write_shadow(&hub, &msg_depleted_soc), 0);

    // 4. BMS Safety Envelope Protection Law: SOC <= 15% -> Force Shutdown Discharge (0 kW)
    if (depleted_soc_pct <= soc_min_safe_pct) {
        discharge_power_target_kw = 0.0;
    }
    assert_float_equal((float)discharge_power_target_kw, 0.0f, 0.01f);

    pipe_msg_t msg_stopped;
    pipe_msg_init_scalar(&msg_stopped, TAG_IEMS_PCS_POWER_SET, discharge_power_target_kw, 0);
    assert_int_equal(datahub_write_shadow(&hub, &msg_stopped), 0);

    // 5. Read back from DataHub and verify shutdown
    pipe_msg_t read_power;
    assert_int_equal(datahub_read_shadow(&hub, TAG_IEMS_PCS_POWER_SET, &read_power), 0);
    assert_float_equal(read_power.payload.val.f, 0.0f, 0.01f);

    datahub_destroy(&hub);
    while (uv_run(&loop, UV_RUN_NOWAIT))
        ;
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dcu_lineloss_topology_scenario),
        cmocka_unit_test(test_dcu_outage_soe_3011_scenario),
        cmocka_unit_test(test_iems_anti_reverse_flow_scenario),
        cmocka_unit_test(test_iems_tou_arbitrage_and_soc_envelope_scenario),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
