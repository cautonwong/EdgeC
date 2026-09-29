/**
 * @file test_caller_allocated_storage.c
 * @brief Unit Tests for Caller-Allocated Storage, Opaque Encapsulation & Aggregate Roots (AEP-752 /
 * Issue #340)
 *
 * Verifies:
 * 1. Physical Opaque Encapsulation (Opaque struct pointer + aligned storage placeholder);
 * 2. In-place placement construction (*_create_in) with zero heap allocation;
 * 3. Consumer-Defined Interfaces & Ports injection across Aggregate Roots;
 * 4. Symmetric Composition Root lifecycle with Caller-Allocated Storage;
 * 5. State invariant preservation and deterministic step execution.
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battery_balancer.h"
#include "battery_eis.h"
#include "battery_fo_ecm_ekf.h"
#include "battery_gas_early_warning.h"
#include "battery_micro_isc.h"
#include "battery_pulse_heating.h"
#include "battery_soh_fusion.h"
#include "battery_thermal_mesh.h"
#include "bus_orchestrator.h"
#include "datahub.h"
#include "driver_hotplug.h"
#include "driver_manager.h"
#include "ebpf_guard.h"
#include "edge_acl.h"
#include "edge_agent.h"
#include "edge_cep_engine.h"
#include "edge_composition_root.h"
#include "edge_guard.h"
#include "edge_sched_deadline.h"
#include "iec104_apci.h"
#include "impedance_control.h"
#include "mpc_dispatcher.h"
#include "persistence_mgr.h"
#include "protocol_bridge.h"
#include "shmring.h"
#include "snf_manager.h"
#include "sqlite_service.h"
#include "vsg_control.h"

/* -------------------------------------------------------------------------- */
/* Test Cases                                                                 */
/* -------------------------------------------------------------------------- */

static void test_battery_ekf_caller_allocated_storage(void **state) {
    (void)state;

    /* 1. Allocate storage on stack (0 malloc) */
    fo_ecm_ekf_storage_t ekf_store;
    memset(&ekf_store, 0xCC, sizeof(ekf_store));

    fo_ecm_params_t params = {.alpha = 0.75,
                              .r0 = 0.005,
                              .rp = 0.008,
                              .c_alpha = 1200.0,
                              .capacity_ah = 100.0,
                              .coulombic_eff = 0.998,
                              .temp_coeff_r0 = 0.002,
                              .temp_ref_c = 25.0};

    /* 2. In-place construct aggregate root */
    fo_ecm_ekf_t *ekf = fo_ecm_ekf_create_in(&ekf_store, &params, 0.85, 25.0, 1.0);
    assert_non_null(ekf);
    assert_ptr_equal(ekf, (void *)ekf_store.raw);

    /* 3. Verify state invariants via opaque accessors */
    assert_float_equal(fo_ecm_ekf_get_soc(ekf), 0.85, 0.001);
    assert_float_equal(fo_ecm_ekf_get_up(ekf), 0.0, 0.001);
    assert_float_equal(fo_ecm_ekf_get_gl_weight(ekf, 0), 1.0, 0.001);

    /* 4. Execute time step */
    assert_true(fo_ecm_ekf_step(ekf, -20.0, 3.30, 25.0, 1.0));
    assert_true(fo_ecm_ekf_get_soc(ekf) > 0.0 && fo_ecm_ekf_get_soc(ekf) <= 1.0);
    assert_int_equal(fo_ecm_ekf_get_step_count(ekf), 1);

    /* 5. Reset */
    fo_ecm_ekf_reset(ekf, 0.50, 30.0);
    assert_float_equal(fo_ecm_ekf_get_soc(ekf), 0.50, 0.001);
}

static void test_vsg_controller_caller_allocated_storage(void **state) {
    (void)state;

    /* 1. Allocate storage on stack (0 malloc) */
    vsg_controller_storage_t vsg_store;
    memset(&vsg_store, 0xAA, sizeof(vsg_store));

    vsg_config_t cfg = {.j_inertia = 2.0,
                        .d_damping = 20.0,
                        .v_nominal = 220.0,
                        .f_nominal = 50.0,
                        .p_ref = 50000.0, /* 50 kW */
                        .q_ref = 0.0,
                        .droop_p = 1e-4,
                        .droop_q = 1e-3};

    /* 2. Setup mock consumer ports */
    edge_consumer_ports_t mock_ports;
    assert_int_equal(edge_port_create_mock_ports(&mock_ports, 0.0), 0);
    vsg_ports_t ports = {
        .reader = mock_ports.reader, .writer = mock_ports.writer, .clock = mock_ports.clock};

    /* 3. In-place construct VSG aggregate root */
    vsg_controller_t *vsg = vsg_controller_create_in(&vsg_store, &cfg, &ports);
    assert_non_null(vsg);
    assert_ptr_equal(vsg, (void *)vsg_store.raw);

    assert_float_equal(vsg_controller_get_frequency(vsg), 50.0, 0.001);
    assert_float_equal(vsg_controller_get_voltage(vsg), 220.0, 0.001);

    /* 4. Step VSG swing equation under load step (P_meas = 55 kW) */
    double out_f = 0.0, out_v = 0.0;
    assert_int_equal(vsg_controller_step(vsg, 55000.0, 5000.0, 0.01, &out_f, &out_v), 0);
    assert_true(out_f > 45.0 && out_f < 55.0);
    assert_true(out_v > 180.0 && out_v < 260.0);

    /* 5. Step via Consumer Ports */
    uint32_t tag_p = 301;
    uint32_t tag_q = 302;
    uint32_t tag_f_out = 303;
    uint32_t tag_v_out = 304;

    mock_ports.writer.write_tag(mock_ports.writer.ctx, tag_p, 50000.0, 1000ULL);
    mock_ports.writer.write_tag(mock_ports.writer.ctx, tag_q, 0.0, 1000ULL);

    assert_int_equal(vsg_controller_step_ports(vsg, tag_p, tag_q, tag_f_out, tag_v_out, 0.01), 0);

    double read_freq = 0.0;
    mock_ports.reader.read_tag(mock_ports.reader.ctx, tag_f_out, &read_freq, NULL);
    assert_true(read_freq > 45.0 && read_freq < 55.0);
}

static void test_mpc_dispatcher_caller_allocated_storage(void **state) {
    (void)state;

    /* 1. Allocate storage on stack (0 malloc) */
    mpc_dispatcher_storage_t mpc_store;
    memset(&mpc_store, 0x55, sizeof(mpc_store));

    mpc_dispatcher_config_t cfg = {.horizon = 5,
                                   .dt_s = 1.0,
                                   .capacity_ah = 100.0,
                                   .target_soc = 0.60,
                                   .q_soc_weight = 1000.0,
                                   .r_power_weight = 0.1,
                                   .max_power_kw = 50.0,
                                   .min_power_kw = -50.0};

    /* 2. Setup mock ports */
    edge_consumer_ports_t mock_ports;
    assert_int_equal(edge_port_create_mock_ports(&mock_ports, 0.0), 0);
    mpc_dispatcher_ports_t ports = {
        .reader = mock_ports.reader, .writer = mock_ports.writer, .clock = mock_ports.clock};

    /* 3. In-place construct MPC aggregate root */
    mpc_dispatcher_t *mpc = mpc_dispatcher_create_in(&mpc_store, &cfg, &ports);
    assert_non_null(mpc);
    assert_ptr_equal(mpc, (void *)mpc_store.raw);

    /* 4. Test when SOC is above target (0.80 > 0.60) -> should discharge (optimal power > 0 or
     * negative sign depending on convention) */
    double opt_p = 0.0;
    assert_int_equal(mpc_dispatcher_step(mpc, 0.80, &opt_p), 0);
    assert_true(opt_p != 0.0);
    assert_int_equal(mpc_dispatcher_get_step_count(mpc), 1);

    /* 5. Test step via ports */
    uint32_t tag_soc = 401;
    uint32_t tag_p_out = 402;
    mock_ports.writer.write_tag(mock_ports.writer.ctx, tag_soc, 75.0, 1000ULL); /* 75% */

    assert_int_equal(mpc_dispatcher_step_ports(mpc, tag_soc, tag_p_out), 0);

    double read_p = 0.0;
    mock_ports.reader.read_tag(mock_ports.reader.ctx, tag_p_out, &read_p, NULL);
    assert_true(read_p != 0.0);
}

static void test_composition_root_caller_allocated_storage(void **state) {
    (void)state;

    /* 1. Allocate Composition Root Container storage on stack (0 malloc) */
    edge_composition_container_storage_t root_store;
    memset(&root_store, 0x00, sizeof(root_store));

    edge_composition_config_t cfg = {.product_name = "IEMS-BESS-Container",
                                     .datahub_capacity = 256,
                                     .memory_pool_buffer_size = 2048,
                                     .memory_pool_capacity = 4,
                                     .enable_dma_buf = true,
                                     .enable_anti_corruption = true};

    /* 2. In-place placement construct Single Composition Root */
    edge_composition_container_t *root = edge_composition_root_create_in(&root_store, &cfg);
    assert_non_null(root);
    assert_ptr_equal(root, (void *)root_store.raw);

    /* 3. Retrieve injected ports bundle */
    edge_consumer_ports_t ports;
    assert_int_equal(edge_composition_root_get_ports(root, &ports), 0);
    assert_non_null(ports.reader.read_tag);
    assert_non_null(ports.writer.write_tag);
    assert_non_null(ports.clock.now_us);

    /* 4. Telemetry write and read through DataHub shadow */
    uint32_t test_tag = 50;
    assert_int_equal(ports.writer.write_tag(ports.writer.ctx, test_tag, 380.5, 2000000ULL), 0);

    double val = 0.0;
    uint64_t ts = 0;
    assert_int_equal(ports.reader.read_tag(ports.reader.ctx, test_tag, &val, &ts), 0);
    assert_float_equal(val, 380.5, 0.001);

    /* 5. Verify Metrics */
    edge_composition_metrics_t metrics;
    assert_int_equal(edge_composition_root_get_metrics(root, &metrics), 0);
    assert_string_equal(metrics.product_name, "IEMS-BESS-Container");
    assert_true(metrics.initialized);
    assert_true(metrics.zero_alloc_runtime);

    /* 6. Clean symmetric destruction */
    assert_int_equal(edge_composition_root_destroy(root), 0);
}

static void test_micro_isc_caller_allocated_storage(void **state) {
    (void)state;
    pack_micro_isc_detector_storage_t store;
    memset(&store, 0xEE, sizeof(store));

    pack_micro_isc_detector_t *det = battery_micro_isc_create_in(&store, NULL, 64, NULL);
    assert_non_null(det);
    assert_ptr_equal(det, (void *)store.raw);

    assert_int_equal(battery_micro_isc_get_cell_count(det), 64);
    assert_float_equal(battery_micro_isc_get_pack_avg_voltage(det), 0.0f, 0.001f);

    /* Feed cell */
    for (uint16_t c = 0; c < 64; c++) {
        assert_int_equal(battery_micro_isc_feed_cell(det, c, 3.300f, 0.0f, 25.0f, 0.50f, 0.0), 0);
    }
}

static void test_battery_balancer_caller_allocated_storage(void **state) {
    (void)state;
    battery_balancer_storage_t store;
    memset(&store, 0xDD, sizeof(store));

    battery_balancer_t *bal = battery_balancer_create_in(&store, NULL, 16, NULL);
    assert_non_null(bal);
    assert_ptr_equal(bal, (void *)store.raw);

    assert_int_equal(battery_balancer_get_cell_count(bal), 16);
    const battery_bal_config_t *cfg = battery_balancer_get_config(bal);
    assert_non_null(cfg);
    assert_int_equal(cfg->mode, BATTERY_BAL_MODE_ACTIVE_PAIRWISE);

    for (uint16_t i = 0; i < 16; i++) {
        battery_bal_cell_input_t in = {.cell_id = i,
                                       .voltage_v = 3.300,
                                       .soc = 0.50,
                                       .temperature_c = 25.0,
                                       .soh = 1.0,
                                       .r0_mohm = 0.5};
        assert_int_equal(battery_balancer_update_cell(bal, i, &in), 0);
    }
    assert_int_equal(battery_balancer_plan_step(bal, 0.0), 0);
    const battery_bal_plan_t *plan = battery_balancer_get_plan(bal);
    assert_non_null(plan);
    assert_true(plan->valid);
}

static void test_gas_early_warning_caller_allocated_storage(void **state) {
    (void)state;
    battery_gas_ew_storage_t store;
    memset(&store, 0xBB, sizeof(store));

    battery_gas_ew_detector_t *ew = battery_gas_ew_create_in(&store, NULL, NULL);
    assert_non_null(ew);
    assert_ptr_equal(ew, (void *)store.raw);

    assert_int_equal(battery_gas_ew_get_sample_count(ew), 0);

    battery_gas_sensor_sample_t s = {.pressure_pa = 101325.0,
                                     .max_cell_temp_c = 25.0,
                                     .co_ppm = 0.0,
                                     .h2_ppm = 0.0,
                                     .voc_ppb = 10.0,
                                     .smoke_density = 0.0,
                                     .min_cell_volt_v = 3.300,
                                     .timestamp_s = 100.0};
    assert_int_equal(battery_gas_ew_push_sample(ew, &s), 0);
    assert_int_equal(battery_gas_ew_get_sample_count(ew), 1);
    assert_int_equal(battery_gas_ew_process_step(ew, 100.0), 0);

    const battery_gas_ew_report_t *rep = battery_gas_ew_get_report(ew);
    assert_non_null(rep);
    assert_int_equal(rep->stage, BATTERY_GAS_STAGE_NORMAL);
}

static void test_battery_soh_caller_allocated_storage(void **state) {
    (void)state;
    battery_soh_storage_t store;
    memset(&store, 0xAA, sizeof(store));

    battery_soh_estimator_t *est = battery_soh_create_in(&store, NULL);
    assert_non_null(est);
    assert_ptr_equal(est, (void *)store.raw);

    assert_int_equal(battery_soh_feed_sample(est, 0.0, 3.295, 25.0, 0.50, 0.0), 0);
    assert_int_equal(battery_soh_process_fusion(est, 1.0), 0);

    const battery_soh_result_t *res = battery_soh_get_result(est);
    assert_non_null(res);
    assert_true(res->valid);
    assert_true(res->soh_capacity >= 0.98);
}

static void test_battery_eis_caller_allocated_storage(void **state) {
    (void)state;
    battery_eis_storage_t store;
    memset(&store, 0x99, sizeof(store));

    battery_eis_analyzer_t *eis = battery_eis_create_in(&store, NULL);
    assert_non_null(eis);
    assert_ptr_equal(eis, (void *)store.raw);

    assert_false(battery_eis_is_ready(eis));
    double pert = battery_eis_next_prbs_excitation(eis);
    assert_true(fabs(fabs(pert) - 1.0) < 1e-6);
}

static void test_battery_pulse_heating_caller_allocated_storage(void **state) {
    (void)state;
    pulse_heat_engine_storage_t store;
    memset(&store, 0x88, sizeof(store));

    pulse_heat_engine_t *engine = pulse_heat_engine_create_in(&store, NULL);
    assert_non_null(engine);
    assert_ptr_equal(engine, (void *)store.raw);

    assert_int_equal(pulse_heat_engine_get_state(engine), PULSE_HEAT_STATE_IDLE);
    assert_int_equal(pulse_heat_engine_get_fault(engine), PULSE_HEAT_FAULT_NONE);
}

static void test_edge_sched_deadline_caller_allocated_storage(void **state) {
    (void)state;
    edge_sched_deadline_storage_t store;
    memset(&store, 0x77, sizeof(store));

    edge_sched_deadline_monitor_t *mon = edge_sched_deadline_create_in(&store, NULL);
    assert_non_null(mon);
    assert_ptr_equal(mon, (void *)store.raw);

    assert_int_equal(edge_sched_deadline_get_task_count(mon), 0);

    edge_sched_task_config_t cfg = {.task_id = 99,
                                    .name = "caller_storage_task",
                                    .period_us = 10000,
                                    .deadline_us = 2000,
                                    .max_allowed_jitter_us = 500,
                                    .max_consecutive_overruns = 3,
                                    .fallback_action = EDGE_SCHED_ACTION_SAFE_FALLBACK};
    assert_int_equal(edge_sched_deadline_register_task(mon, &cfg), 0);
    assert_int_equal(edge_sched_deadline_get_task_count(mon), 1);
}

static void test_thermal_mesh_caller_allocated_storage(void **state) {
    (void)state;
    thermal_mesh_engine_storage_t store;
    memset(&store, 0x66, sizeof(store));

    thermal_mesh_engine_t *engine = thermal_mesh_engine_create_in(&store, NULL);
    assert_non_null(engine);
    assert_ptr_equal(engine, (void *)store.raw);

    assert_true(thermal_mesh_is_initialized(engine));
    assert_float_equal(thermal_mesh_get_temperature(engine, THERMAL_NODE_CORE_CENTER), 25.0f,
                       0.001f);

    thermal_mesh_telemetry_t telem = {
        .cell_terminal_voltage_v = 3.25f,
        .load_current_a = 50.0f,
        .ambient_temp_c = 25.0f,
        .coolant_temp_c = 20.0f,
        .cell_soc = 0.5f,
    };
    thermal_mesh_output_t out;
    assert_int_equal(thermal_mesh_engine_step(engine, &telem, 0.1f, &out), 0);
    assert_true(out.core_hotspot_temp_c >= 25.0f);
}

static void test_cep_engine_caller_allocated_storage(void **state) {
    (void)state;
    cep_engine_storage_t store;
    memset(&store, 0x55, sizeof(store));

    cep_engine_t *engine = cep_engine_create_in(&store);
    assert_non_null(engine);
    assert_ptr_equal(engine, (void *)store.raw);

    assert_int_equal(cep_engine_add_rule(engine, 101, 1, 50.0, 2, 80.0, 5000, 999), 0);

    cep_event_t ev1 = {.tag_id = 1, .val = 55.0, .timestamp_ms = 1000};
    uint16_t alert = 0;
    bool trig = false;
    assert_int_equal(cep_engine_feed_event(engine, &ev1, &alert, &trig), 0);
    assert_false(trig);

    cep_event_t ev2 = {.tag_id = 2, .val = 85.0, .timestamp_ms = 2000};
    assert_int_equal(cep_engine_feed_event(engine, &ev2, &alert, &trig), 0);
    assert_true(trig);
    assert_int_equal(alert, 999);
}

static void test_impedance_controller_caller_allocated_storage(void **state) {
    (void)state;
    impedance_controller_storage_t store;
    memset(&store, 0x44, sizeof(store));

    impedance_params_t params = {
        .mass = 2.0f,
        .damping = 10.0f,
        .stiffness = 50.0f,
        .target_pos = 0.5f,
        .max_torque_limit = 100.0f,
    };
    impedance_controller_t *ctrl = impedance_controller_create_in(&store, &params);
    assert_non_null(ctrl);
    assert_ptr_equal(ctrl, (void *)store.raw);

    assert_float_equal(impedance_controller_get_torque(ctrl), 0.0f, 0.001f);
    assert_int_equal(impedance_controller_step(ctrl, 0.4f, 0.0f, 0.0f, 0.001f), 0);
    assert_true(impedance_controller_get_torque(ctrl) != 0.0f);
}

static void test_dpi_engine_caller_allocated_storage(void **state) {
    (void)state;
    edge_guard_dpi_storage_t storage;
    memset(&storage, 0xEE, sizeof(storage));
    dpi_engine_ctx_t *ctx = dpi_engine_init_in(&storage, NULL);
    assert_non_null(ctx);
    assert_ptr_equal(ctx, (void *)storage.raw);
}

static void test_bus_orchestrator_caller_allocated_storage(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub = {0};

    bus_orchestrator_storage_t storage;
    memset(&storage, 0xDD, sizeof(storage));
    bus_orchestrator_t *orch = bus_orchestrator_create_in(&storage, &loop, &hub);
    assert_non_null(orch);
    assert_ptr_equal(orch, (void *)storage.raw);
    assert_ptr_equal(bus_orchestrator_get_hub(orch), &hub);

    bus_orchestrator_destroy(orch);
    uv_loop_close(&loop);
}

static void test_driver_manager_caller_allocated_storage(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub = {0};
    bus_orchestrator_storage_t orch_storage;
    bus_orchestrator_t *orch = bus_orchestrator_create_in(&orch_storage, &loop, &hub);
    assert_non_null(orch);

    driver_manager_storage_t dm_storage;
    memset(&dm_storage, 0xBB, sizeof(dm_storage));
    driver_manager_t *mgr = driver_manager_create_in(&dm_storage, &hub, orch);
    assert_non_null(mgr);
    assert_ptr_equal(mgr, (void *)dm_storage.raw);

    driver_manager_destroy(mgr);
    bus_orchestrator_destroy(orch);
    uv_loop_close(&loop);
}

static void test_edge_agent_caller_allocated_storage(void **state) {
    (void)state;
    edge_agent_storage_t agent_storage;
    memset(&agent_storage, 0x11, sizeof(agent_storage));
    edge_agent_config_t cfg = {.broker_url = "tcp://127.0.0.1:1883",
                               .device_id = "EDGE_TEST_001",
                               .product_type = "IEMS_BOX"};
    edge_agent_t *agent = edge_agent_create_in(&agent_storage, cfg);
    assert_non_null(agent);
    assert_ptr_equal(agent, (void *)agent_storage.raw);

    edge_agent_destroy(agent);
}

static void test_protocol_bridge_caller_allocated_storage(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub = {0};
    bus_orchestrator_storage_t orch_storage;
    bus_orchestrator_t *orch = bus_orchestrator_create_in(&orch_storage, &loop, &hub);
    assert_non_null(orch);

    protocol_bridge_storage_t bridge_storage;
    memset(&bridge_storage, 0x22, sizeof(bridge_storage));
    protocol_bridge_t *bridge = protocol_bridge_create_in(&bridge_storage, &loop, &hub, orch);
    assert_non_null(bridge);
    assert_ptr_equal(bridge, (void *)bridge_storage.raw);

    protocol_bridge_destroy(bridge);
    bus_orchestrator_destroy(orch);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
}

static int _dummy_snf_pub(void *user_data, const PipeMsg *msg) {
    (void)user_data;
    (void)msg;
    return 0;
}

static void test_snf_manager_caller_allocated_storage(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    sqlite_config_t scfg = {.db_path = ":memory:",
                            .strategy = STRATEGY_SERIES,
                            .batch_size = 10,
                            .flush_interval_ms = 1000};
    sqlite_svc_t *db = sqlite_svc_create(&loop, &scfg);
    assert_non_null(db);

    snf_manager_storage_t snf_storage;
    memset(&snf_storage, 0x33, sizeof(snf_storage));
    snf_config_t cfg = {.db = db,
                        .loop = &loop,
                        .publish_fn = _dummy_snf_pub,
                        .sync_batch_size = 50,
                        .sync_interval_ms = 1000};
    snf_manager_t *mgr = snf_manager_create_in(&snf_storage, &cfg);
    assert_non_null(mgr);
    assert_ptr_equal(mgr, (void *)snf_storage.raw);

    snf_manager_destroy(mgr);
    sqlite_svc_destroy(db);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
}

static void test_ebpf_guard_caller_allocated_storage(void **state) {
    (void)state;
    ebpf_guard_mgr_storage_t store;
    memset(&store, 0x11, sizeof(store));

    ebpf_guard_mgr_t *mgr = ebpf_guard_mgr_create_in(&store);
    assert_non_null(mgr);
    assert_ptr_equal(mgr, (void *)store.raw);
    assert_true(ebpf_guard_is_enforce_mode(mgr));

    uint32_t probe_id = 0;
    assert_int_equal(ebpf_guard_attach_probe(mgr, EBPF_PROBE_SYSCALL_MONITOR, &probe_id), 0);
    assert_int_equal(probe_id, 1);

    assert_int_equal(ebpf_guard_audit_event(mgr, probe_id, 100, false), 0);
    assert_int_equal(ebpf_guard_audit_event(mgr, probe_id, 200, true), 1);
    assert_int_equal(ebpf_guard_get_probe_events_processed(mgr, probe_id), 2);
    assert_int_equal(ebpf_guard_get_probe_violations(mgr, probe_id), 1);
}

static void test_edge_acl_caller_allocated_storage(void **state) {
    (void)state;
    edge_acl_engine_storage_t store;
    memset(&store, 0x22, sizeof(store));

    edge_acl_engine_t *acl = edge_acl_engine_create_in(&store, ACL_ACTION_DROP);
    assert_non_null(acl);
    assert_ptr_equal(acl, (void *)store.raw);

    uint32_t ip = (192 << 24) | (168 << 16) | (1 << 8) | 10;
    uint32_t mask = (255 << 24) | (255 << 16) | (255 << 8) | 0;
    assert_int_equal(edge_acl_add_rule(acl, ip, mask, 443, 6, ACL_ACTION_ALLOW), 0);

    assert_int_equal(edge_acl_match(acl, ip, 443, 6), ACL_ACTION_ALLOW);
    assert_int_equal(edge_acl_match(acl, (10 << 24) | 1, 443, 6), ACL_ACTION_DROP);
}

static void test_persistence_mgr_caller_allocated_storage(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    shm_ring_t ring;
    assert_int_equal(shm_ring_init(&ring, 16), 0);

    persistence_mgr_storage_t store;
    memset(&store, 0x33, sizeof(store));

    persistence_mgr_t *mgr = persistence_mgr_create_in(&store, &loop, &ring, 100);
    assert_non_null(mgr);
    assert_ptr_equal(mgr, (void *)store.raw);

    persistence_mgr_close(mgr);
    shm_ring_destroy(&ring);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
}

static int _dummy_hp_init(void) { return 0; }
static void _dummy_hp_cleanup(void) {}

static void test_driver_hotplug_caller_allocated_storage(void **state) {
    (void)state;
    driver_hotplug_mgr_storage_t store;
    memset(&store, 0x44, sizeof(store));

    driver_hotplug_mgr_t *mgr = driver_hotplug_mgr_create_in(&store);
    assert_non_null(mgr);
    assert_ptr_equal(mgr, (void *)store.raw);
    assert_int_equal(driver_hotplug_get_active_count(mgr), 0);

    driver_plugin_ops_t ops = {
        .abi_magic = DRIVER_ABI_MAGIC,
        .abi_version = DRIVER_ABI_VERSION,
        .driver_name = "HotplugStorageTest",
        .init_fn = _dummy_hp_init,
        .cleanup_fn = _dummy_hp_cleanup,
    };
    uint32_t slot = 0;
    assert_int_equal(driver_hotplug_load(mgr, "/lib/hp_test.so", &ops, &slot), 0);
    assert_int_equal(slot, 1);
    assert_int_equal(driver_hotplug_get_active_count(mgr), 1);

    assert_int_equal(driver_hotplug_unload(mgr, slot), 0);
    assert_int_equal(driver_hotplug_get_active_count(mgr), 0);
}

static void test_iec104_apci_caller_allocated_storage(void **state) {
    (void)state;
    iec104_apci_mgr_storage_t store;
    memset(&store, 0x55, sizeof(store));

    iec104_apci_mgr_t *mgr = iec104_apci_mgr_create_in(&store);
    assert_non_null(mgr);
    assert_ptr_equal(mgr, (void *)store.raw);

    assert_int_equal(iec104_apci_get_state(mgr), IEC104_STATE_DISCONNECTED);
    iec104_apci_set_state(mgr, IEC104_STATE_CONNECTED);
    assert_int_equal(iec104_apci_get_state(mgr), IEC104_STATE_CONNECTED);

    assert_int_equal(iec104_apci_get_ns(mgr), 0);
    assert_int_equal(iec104_apci_get_nr(mgr), 0);
    iec104_apci_set_nr(mgr, 104);
    assert_int_equal(iec104_apci_get_nr(mgr), 104);

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 128, 0, NULL, NULL), 0);
    net_buf_t *u_buf = net_buf_alloc(&pool, 0);
    assert_non_null(u_buf);
    assert_int_equal(iec104_apci_build_u(IEC104_U_STARTDT_ACT, u_buf), 0);
    assert_int_equal(u_buf->b.len, 6);
    assert_int_equal(u_buf->b.data[0], 0x68);

    net_buf_t *s_buf = net_buf_alloc(&pool, 0);
    assert_non_null(s_buf);
    assert_int_equal(iec104_apci_build_s(mgr, s_buf), 0);
    assert_int_equal(s_buf->b.len, 6);

    net_buf_unref(u_buf);
    net_buf_unref(s_buf);
    net_buf_pool_destroy(&pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_battery_ekf_caller_allocated_storage),
        cmocka_unit_test(test_vsg_controller_caller_allocated_storage),
        cmocka_unit_test(test_mpc_dispatcher_caller_allocated_storage),
        cmocka_unit_test(test_composition_root_caller_allocated_storage),
        cmocka_unit_test(test_micro_isc_caller_allocated_storage),
        cmocka_unit_test(test_battery_balancer_caller_allocated_storage),
        cmocka_unit_test(test_gas_early_warning_caller_allocated_storage),
        cmocka_unit_test(test_battery_soh_caller_allocated_storage),
        cmocka_unit_test(test_battery_eis_caller_allocated_storage),
        cmocka_unit_test(test_battery_pulse_heating_caller_allocated_storage),
        cmocka_unit_test(test_edge_sched_deadline_caller_allocated_storage),
        cmocka_unit_test(test_thermal_mesh_caller_allocated_storage),
        cmocka_unit_test(test_cep_engine_caller_allocated_storage),
        cmocka_unit_test(test_impedance_controller_caller_allocated_storage),
        cmocka_unit_test(test_dpi_engine_caller_allocated_storage),
        cmocka_unit_test(test_bus_orchestrator_caller_allocated_storage),
        cmocka_unit_test(test_driver_manager_caller_allocated_storage),
        cmocka_unit_test(test_edge_agent_caller_allocated_storage),
        cmocka_unit_test(test_protocol_bridge_caller_allocated_storage),
        cmocka_unit_test(test_snf_manager_caller_allocated_storage),
        cmocka_unit_test(test_ebpf_guard_caller_allocated_storage),
        cmocka_unit_test(test_edge_acl_caller_allocated_storage),
        cmocka_unit_test(test_persistence_mgr_caller_allocated_storage),
        cmocka_unit_test(test_driver_hotplug_caller_allocated_storage),
        cmocka_unit_test(test_iec104_apci_caller_allocated_storage),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
