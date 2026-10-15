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

#include "ai_awq_gptq_gemm.h"
#include "ai_dynamic_tensor_pool.h"
#include "ai_fast_gemm.h"
#include "ai_graph_pruner.h"
#include "ai_hetero_pipeline.h"
#include "ai_pinn_evaluator.h"
#include "ai_quant_calibrator.h"
#include "ai_slm_kv_cache.h"
#include "ai_temporal_fault_net.h"
#include "battery_balancer.h"
#include "battery_eis.h"
#include "battery_fo_ecm_ekf.h"
#include "battery_gas_early_warning.h"
#include "battery_micro_isc.h"
#include "battery_pulse_heating.h"
#include "battery_soh_fusion.h"
#include "battery_thermal_mesh.h"
#include "battery_thermal_pinn.h"
#include "bus_orchestrator.h"
#include "datahub.h"
#include "driver_hotplug.h"
#include "driver_manager.h"
#include "edge_acl.h"
#include "edge_agent.h"
#include "edge_amp_ipc.h"
#include "edge_cep_engine.h"
#include "edge_composition_root.h"
#include "edge_coop_guard.h"
#include "edge_cpu_affinity.h"
#include "edge_ebpf_telemetry.h"
#include "edge_edf_scheduler.h"
#include "edge_guard.h"
#include "edge_mixed_criticality.h"
#include "edge_operator_fusion.h"
#include "edge_pcp_mutex.h"
#include "edge_prio_booster.h"
#include "edge_rms_scheduler.h"
#include "edge_sched_deadline.h"
#include "edge_swarm_offload.h"
#include "edge_tensor_pipeline.h"
#include "edge_thermal_scheduler.h"
#include "edge_tmr_voter.h"
#include "edge_tsn_engine.h"
#include "edge_uns_engine.h"
#include "edge_vsg_engine.h"
#include "edge_wasm_nn.h"
#include "iec104_apci.h"
#include "impedance_control.h"
#include "mpc_dispatcher.h"
#include "npu_enclave.h"
#include "npu_pmu.h"
#include "npu_power_manager.h"
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

static void test_edge_pcp_mutex_caller_allocated_storage(void **state) {
    (void)state;
    edge_pcp_mutex_storage_t store;
    memset(&store, 0x55, sizeof(store));

    edge_pcp_mutex_config_t cfg = {
        .mutex_id = 11009,
        .name = "test_pcp_caller_storage",
        .protocol = EDGE_PCP_PROTOCOL_PROTECT,
        .prio_ceiling = 80,
        .max_expected_hold_us = 10000,
        .recursive = false,
    };

    edge_pcp_mutex_t *m = edge_pcp_mutex_create_in(&store, &cfg, NULL);
    assert_non_null(m);
    assert_ptr_equal(m, (void *)store.raw);

    assert_int_equal(edge_pcp_mutex_lock(m), 0);
    assert_int_equal(edge_pcp_mutex_unlock(m), 0);

    edge_pcp_mutex_stats_t stats;
    assert_int_equal(edge_pcp_mutex_get_stats(m, &stats), 0);
    assert_int_equal(stats.total_lock_count, 1);
    assert_int_equal(stats.mutex_id, 11009);

    assert_int_equal(edge_pcp_mutex_destroy(m), 0);
}

static void test_edge_edf_scheduler_caller_allocated_storage(void **state) {
    (void)state;
    edge_edf_scheduler_storage_t store;
    memset(&store, 0x44, sizeof(store));

    edge_edf_config_t cfg = {
        .tick_resolution_us = 1000,
        .enable_dynamic_preemption = true,
        .drop_on_deadline_miss = true,
    };

    edge_edf_scheduler_t *sched = edge_edf_scheduler_create_in(&store, &cfg, NULL);
    assert_non_null(sched);
    assert_ptr_equal(sched, (void *)store.raw);

    edge_edf_task_config_t t_cfg = {
        .task_id = 77,
        .name = "caller_edf_task",
        .period_us = 10000,
        .wcet_us = 2000,
    };
    assert_int_equal(edge_edf_scheduler_register_task(sched, &t_cfg), 0);
    assert_true(edge_edf_scheduler_get_utilization(sched) > 0.199);

    edge_edf_dispatch_result_t res;
    assert_int_equal(edge_edf_scheduler_step(sched, 0, 1000, &res), 0);
    assert_true(res.task_dispatched);
    assert_int_equal(res.active_task_id, 77);
}

static void test_edge_rms_scheduler_caller_allocated_storage(void **state) {
    (void)state;
    edge_rms_scheduler_storage_t store;
    memset(&store, 0x33, sizeof(store));

    edge_rms_config_t cfg = {
        .tick_resolution_us = 1000,
        .auto_assign_priorities = true,
        .enable_budget_watchdog = true,
    };

    edge_rms_scheduler_t *sched = edge_rms_scheduler_create_in(&store, &cfg, NULL);
    assert_non_null(sched);
    assert_ptr_equal(sched, (void *)store.raw);

    edge_rms_task_config_t t_cfg = {
        .task_id = 88,
        .name = "caller_rms_task",
        .period_us = 10000,
        .wcet_budget_us = 2000,
    };
    assert_int_equal(edge_rms_scheduler_register_task(sched, &t_cfg), 0);
    assert_true(edge_rms_scheduler_get_utilization(sched) > 0.199);

    edge_rms_dispatch_result_t res;
    assert_int_equal(edge_rms_scheduler_step(sched, 0, 1000, &res), 0);
    assert_true(res.task_dispatched);
    assert_int_equal(res.active_task_id, 88);
}

static void test_edge_cpu_affinity_caller_allocated_storage(void **state) {
    (void)state;
    edge_cpu_affinity_storage_t store;
    memset(&store, 0x22, sizeof(store));

    edge_cpu_affinity_config_t cfg = {
        .rt_core_mask = 0x02,
        .housekeeping_core_mask = 0x01,
        .auto_isolate_irqs = false,
        .enable_mlockall = false,
    };

    edge_cpu_affinity_t *aff = edge_cpu_affinity_create_in(&store, &cfg, NULL);
    assert_non_null(aff);
    assert_ptr_equal(aff, (void *)store.raw);

    edge_cpu_topology_t topo;
    assert_int_equal(edge_cpu_affinity_get_topology(aff, &topo), 0);
    assert_true(topo.total_online_cores >= 1);

    assert_int_equal(edge_cpu_affinity_pin_current_thread(aff, 0, "caller_affinity_task"), 0);

    edge_cpu_affinity_stats_t stats;
    assert_int_equal(edge_cpu_affinity_get_stats(aff, &stats), 0);
    assert_int_equal(stats.pinned_thread_count, 1);
    assert_int_equal(stats.rt_core_mask, 0x02);

    assert_int_equal(edge_cpu_affinity_destroy(aff), 0);
}

static void test_edge_prio_booster_caller_allocated_storage(void **state) {
    (void)state;
    edge_prio_booster_storage_t store;
    memset(&store, 0x77, sizeof(store));

    edge_prio_booster_config_t cfg = {
        .tick_resolution_us = 1000,
        .default_boost_hold_us = 50000,
        .max_boosted_prio = 99,
        .anneal_curve = EDGE_ANNEAL_STEP,
    };

    edge_prio_booster_t *booster = edge_prio_booster_create_in(&store, &cfg, NULL);
    assert_non_null(booster);
    assert_ptr_equal(booster, (void *)store.raw);

    edge_boost_task_config_t t_cfg = {
        .task_id = 99,
        .name = "caller_booster_task",
        .base_priority = 25,
        .thread_handle = (pthread_t)0,
    };
    assert_int_equal(edge_prio_booster_register_task(booster, &t_cfg), 0);

    assert_int_equal(edge_prio_booster_trigger_boost(booster, 99, 99, 10000, "trip"), 0);

    int prio = 0;
    assert_int_equal(edge_prio_booster_get_task_priority(booster, 99, &prio), 0);
    assert_int_equal(prio, 99);

    assert_int_equal(edge_prio_booster_destroy(booster), 0);
}

static void test_edge_coop_guard_caller_allocated_storage(void **state) {
    (void)state;
    edge_coop_guard_storage_t store;
    memset(&store, 0x88, sizeof(store));

    edge_coop_guard_config_t cfg = {
        .default_budget_us = 10000,
        .default_degrade_threshold = 3,
        .default_trip_threshold = 5,
        .default_cooldown_us = 50000,
    };

    edge_coop_guard_t *guard = edge_coop_guard_create_in(&store, &cfg, NULL);
    assert_non_null(guard);
    assert_ptr_equal(guard, (void *)store.raw);

    edge_coop_task_config_t t_cfg = {
        .task_id = 777,
        .name = "caller_coop_task",
        .timeslice_budget_us = 10000,
        .max_consecutive_overruns_to_degrade = 2,
        .max_consecutive_overruns_to_trip = 4,
        .recovery_cooldown_us = 50000,
    };
    assert_int_equal(edge_coop_guard_register_task(guard, &t_cfg), 0);

    assert_true(edge_coop_guard_can_execute(guard, 777, 1000000ULL));
    assert_int_equal(edge_coop_guard_enter_slice(guard, 777, 1000000ULL), 0);
    assert_int_equal(edge_coop_guard_exit_slice(guard, 777, 1005000ULL), 0);

    edge_coop_task_status_t status;
    assert_int_equal(edge_coop_guard_get_task_status(guard, 777, &status), 0);
    assert_int_equal(status.state, EDGE_COOP_STATE_HEALTHY);

    assert_int_equal(edge_coop_guard_destroy(guard), 0);
}

static void test_edge_mixed_crit_caller_allocated_storage(void **state) {
    (void)state;
    edge_mixed_crit_storage_t store;
    memset(&store, 0x99, sizeof(store));

    edge_mixed_crit_config_t cfg = {
        .tick_resolution_us = 1000,
        .thermal_warning_temp_c = 70.0f,
        .thermal_critical_temp_c = 85.0f,
        .enable_thermal_throttling = true,
        .l3_decimation_factor = 2,
    };

    edge_mixed_crit_dispatcher_t *disp = edge_mixed_crit_create_in(&store, &cfg, NULL);
    assert_non_null(disp);
    assert_ptr_equal(disp, (void *)store.raw);

    edge_mixed_crit_task_config_t t_cfg = {
        .task_id = 111,
        .name = "caller_l1_task",
        .level = CRITICALITY_L1_SAFETY,
        .period_us = 10000,
        .wcet_budget_us = 1000,
    };
    assert_int_equal(edge_mixed_crit_register_task(disp, &t_cfg), 0);

    edge_mixed_crit_dispatch_result_t res;
    assert_int_equal(edge_mixed_crit_step(disp, 0, &res), 0);
    assert_true(res.task_dispatched);
    assert_int_equal(res.active_task_id, 111);
    assert_int_equal(res.active_level, CRITICALITY_L1_SAFETY);

    assert_int_equal(edge_mixed_crit_destroy(disp), 0);
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
    int rc = dpi_engine_init_in(&storage, NULL);
    assert_int_equal(rc, 0);
}

static void test_bus_orchestrator_caller_allocated_storage(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub = {0};
    hub.loop = &loop;
    arbiter_t *arb = NULL;

    driver_manager_storage_t dm_storage;
    driver_manager_t *mgr = driver_manager_create_in(&dm_storage);
    assert_non_null(mgr);

    bus_orchestrator_storage_t storage;
    memset(&storage, 0xDD, sizeof(storage));
    bus_orchestrator_t *orch = bus_orchestrator_create_in(&storage, &hub, arb, mgr, 4, 32, 256);
    assert_non_null(orch);
    assert_ptr_equal(orch, (void *)storage.raw);
    assert_ptr_equal(bus_orchestrator_get_hub(orch), &hub);

    bus_orchestrator_destroy(orch);
    driver_manager_destroy(mgr);
    uv_loop_close(&loop);
}

static void test_driver_manager_caller_allocated_storage(void **state) {
    (void)state;
    driver_manager_storage_t dm_storage;
    memset(&dm_storage, 0xBB, sizeof(dm_storage));
    driver_manager_t *mgr = driver_manager_create_in(&dm_storage);
    assert_non_null(mgr);
    assert_ptr_equal(mgr, (void *)dm_storage.raw);

    driver_manager_destroy(mgr);
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
    hub.loop = &loop;
    arbiter_t *arb = NULL;

    driver_manager_storage_t dm_storage;
    driver_manager_t *mgr = driver_manager_create_in(&dm_storage);
    assert_non_null(mgr);

    bus_orchestrator_storage_t orch_storage;
    bus_orchestrator_t *orch =
        bus_orchestrator_create_in(&orch_storage, &hub, arb, mgr, 4, 32, 256);
    assert_non_null(orch);

    protocol_bridge_storage_t bridge_storage;
    memset(&bridge_storage, 0x22, sizeof(bridge_storage));
    protocol_bridge_t *bridge = protocol_bridge_create_in(&bridge_storage, &loop, &hub, orch);
    assert_non_null(bridge);
    assert_ptr_equal(bridge, (void *)bridge_storage.raw);

    protocol_bridge_destroy(bridge);
    bus_orchestrator_destroy(orch);
    driver_manager_destroy(mgr);
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

static void test_edge_acl_caller_allocated_storage(void **state) {
    (void)state;
    edge_acl_engine_storage_t store;
    memset(&store, 0x22, sizeof(store));

    edge_acl_engine_t *acl = edge_acl_engine_create_in(&store, ACL_ACTION_DROP);
    assert_non_null(acl);
    assert_ptr_equal(acl, (void *)store.raw);

    uint32_t ip = (uint32_t)((192U << 24) | (168U << 16) | (1U << 8) | 10U);
    uint32_t mask = (uint32_t)((255U << 24) | (255U << 16) | (255U << 8) | 0U);
    assert_int_equal(edge_acl_add_rule(acl, ip, mask, 443, 6, ACL_ACTION_ALLOW), 0);

    assert_int_equal(edge_acl_match(acl, ip, 443, 6), ACL_ACTION_ALLOW);
    assert_int_equal(edge_acl_match(acl, (10 << 24) | 1, 443, 6), ACL_ACTION_DROP);
}

static void test_persistence_mgr_caller_allocated_storage(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    shm_ring_t ring;
    assert_int_equal(shmring_create(&ring, "persistence_ring", 16), 0);

    persistence_mgr_storage_t store;
    memset(&store, 0x33, sizeof(store));

    persistence_mgr_t *mgr = persistence_mgr_create_in(&store, &loop, &ring, 100);
    assert_non_null(mgr);
    assert_ptr_equal(mgr, (void *)store.raw);

    persistence_mgr_close(mgr);
    shmring_destroy(&ring);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
}

static int _dummy_hp_init(void) {
    return 0;
}
static void _dummy_hp_cleanup(void) {
}

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

static void test_npu_pmu_caller_allocated_storage(void **state) {
    (void)state;

    npu_pmu_storage_t pmu_store;
    memset(&pmu_store, 0xCC, sizeof(pmu_store));

    npu_pmu_config_t cfg = {.peak_macs_per_cycle = 512,
                            .core_clock_ghz = 1.0f,
                            .jitter_threshold_sigma = 3.0f,
                            .enable_ring_buffer = true};

    npu_pmu_t *pmu = npu_pmu_create_in(&pmu_store, &cfg);
    assert_non_null(pmu);
    assert_ptr_equal(pmu, (void *)pmu_store.raw);

    uint64_t tok = npu_pmu_op_begin(pmu, 1, NPU_OP_CONV2D, "layer1", 1000ULL, 1024);
    assert_true(tok > 0);

    npu_pmu_record_t rec;
    assert_int_equal(npu_pmu_op_end(pmu, tok, 100, 5, 50, 2, &rec), 0);
    assert_int_equal(rec.op_id, 1);
}

static void test_npu_enclave_caller_allocated_storage(void **state) {
    (void)state;

    npu_enclave_storage_t enc_store;
    memset(&enc_store, 0xDD, sizeof(enc_store));

    npu_enclave_config_t cfg = {.enforce_signature_check = false,
                                .enforce_integrity_sha256 = true,
                                .zeroize_on_unload = true};

    npu_enclave_t *enclave = npu_enclave_create_in(&enc_store, &cfg);
    assert_non_null(enclave);
    assert_ptr_equal(enclave, (void *)enc_store.raw);

    npu_enclave_auth_status_t status;
    uint32_t tamper = 0;
    assert_int_equal(npu_enclave_get_status(enclave, &status, NULL, &tamper), 0);
    assert_int_equal(status, NPU_ENCLAVE_AUTH_UNINITIALIZED);
}

static void test_npu_power_mgr_caller_allocated_storage(void **state) {
    (void)state;

    npu_power_mgr_storage_t pwr_store;
    memset(&pwr_store, 0xEE, sizeof(pwr_store));

    npu_power_config_t cfg = {.governor = NPU_DVFS_ONDEMAND,
                              .idle_gate_timeout_ms = 5,
                              .standby_timeout_ms = 50,
                              .deep_sleep_timeout_ms = 500,
                              .thermal_warning_temp_c = 75.0f,
                              .thermal_critical_temp_c = 85.0f,
                              .max_freq_mhz = 1000,
                              .min_freq_mhz = 300};

    npu_power_mgr_t *mgr = npu_power_mgr_create_in(&pwr_store, &cfg);
    assert_non_null(mgr);
    assert_ptr_equal(mgr, (void *)pwr_store.raw);

    npu_power_metrics_t metrics;
    assert_int_equal(npu_power_mgr_get_metrics(mgr, &metrics), 0);
    assert_int_equal(metrics.current_state, NPU_PWR_ACTIVE);
}

static void test_ai_quant_calib_caller_allocated_storage(void **state) {
    (void)state;

    ai_quant_calib_storage_t store;
    memset(&store, 0x11, sizeof(store));

    ai_quant_config_t cfg = {.scheme = AI_QUANT_SCHEME_INT8_SYMMETRIC,
                             .method = AI_QUANT_CALIB_MINMAX};

    ai_quant_calibrator_t *calib = ai_quant_calibrator_create_in(&store, &cfg);
    assert_non_null(calib);
    assert_ptr_equal(calib, (void *)store.raw);

    float samples[16] = {1.0f, -2.0f, 3.0f, -4.0f};
    assert_int_equal(ai_quant_calibrator_collect_sample(calib, samples, 4), 0);

    ai_quant_params_t p;
    assert_int_equal(ai_quant_calibrator_compute_params(calib, &p), 0);
    assert_true(p.scale > 0.0f);
}

static void test_ai_awq_gptq_caller_allocated_storage(void **state) {
    (void)state;

    ai_awq_gptq_storage_t store;
    memset(&store, 0x22, sizeof(store));

    ai_awq_gptq_engine_t *engine = ai_awq_gptq_create_in(&store);
    assert_non_null(engine);
    assert_ptr_equal(engine, (void *)store.raw);
}

static void test_ai_temporal_fault_caller_allocated_storage(void **state) {
    (void)state;

    ai_temporal_fault_storage_t store;
    memset(&store, 0x33, sizeof(store));

    ai_fault_net_config_t cfg = {
        .classification_threshold = 75.0f, .enable_bilstm = true, .cell_state_clip = 10.0f};

    ai_temporal_fault_net_t *net = ai_temporal_fault_net_create_in(&store, &cfg);
    assert_non_null(net);
    assert_ptr_equal(net, (void *)store.raw);
}

static void test_ai_dyn_pool_caller_allocated_storage(void **state) {
    (void)state;

    ai_dyn_pool_storage_t store;
    memset(&store, 0x44, sizeof(store));

    ai_dyn_pool_t *pool = ai_dyn_pool_create_in(&store, 32768);
    assert_non_null(pool);
    assert_ptr_equal(pool, (void *)store.raw);
}

static void test_ai_slm_kv_cache_caller_allocated_storage(void **state) {
    (void)state;

    ai_slm_kv_cache_storage_t store;
    memset(&store, 0x55, sizeof(store));

    ai_slm_kv_config_t cfg = {.num_layers = 2,
                              .num_heads = 4,
                              .num_kv_heads = 2,
                              .head_dim = 16,
                              .sliding_window_size = 32,
                              .rope_theta_base = 10000.0f};

    ai_slm_kv_cache_t *cache = ai_slm_kv_cache_create_in(&store, &cfg);
    assert_non_null(cache);
    assert_ptr_equal(cache, (void *)store.raw);
}

static void test_ai_graph_pruner_caller_allocated_storage(void **state) {
    (void)state;

    ai_graph_pruner_storage_t store;
    memset(&store, 0x66, sizeof(store));

    ai_graph_pruner_t *pruner = ai_graph_pruner_create_in(&store);
    assert_non_null(pruner);
    assert_ptr_equal(pruner, (void *)store.raw);
}

static void test_ai_fast_gemm_caller_allocated_storage(void **state) {
    (void)state;

    ai_fast_gemm_storage_t store;
    memset(&store, 0x77, sizeof(store));

    ai_gemm_config_t cfg = {.m = 2,
                            .k = 2,
                            .n = 2,
                            .precision = AI_GEMM_PRECISION_INT8,
                            .activation = AI_GEMM_ACT_NONE};

    ai_fast_gemm_t *gemm = ai_fast_gemm_create_in(&store, &cfg);
    assert_non_null(gemm);
    assert_ptr_equal(gemm, (void *)store.raw);
}

static void test_ai_pinn_evaluator_caller_allocated_storage(void **state) {
    (void)state;

    ai_pinn_evaluator_storage_t store;
    memset(&store, 0x88, sizeof(store));

    ai_pinn_config_t cfg = {.lambda_mass = 1.0f, .lambda_energy = 1.0f, .lambda_circuit = 1.0f};

    ai_pinn_evaluator_t *pinn = ai_pinn_evaluator_create_in(&store, &cfg);
    assert_non_null(pinn);
    assert_ptr_equal(pinn, (void *)store.raw);
}

static void test_ai_hetero_pipeline_caller_allocated_storage(void **state) {
    (void)state;

    ai_hetero_pipeline_storage_t store;
    memset(&store, 0x99, sizeof(store));

    ai_hetero_pipeline_t *pipe = ai_hetero_pipeline_create_in(&store);
    assert_non_null(pipe);
    assert_ptr_equal(pipe, (void *)store.raw);
}

static void test_tmr_voter_caller_allocated_storage(void **state) {
    (void)state;

    tmr_voter_context_t voter_store;
    memset(&voter_store, 0x77, sizeof(voter_store));

    tmr_voter_config_t cfg = {.tolerance_band = 0.05f,
                              .suspect_cycles_threshold = 3,
                              .failover_rate_limit = 50.0f,
                              .cycle_period_sec = 0.001f,
                              .fail_safe_value = 0.0f};

    assert_int_equal(edge_tmr_voter_init(&voter_store, &cfg), 0);
    assert_int_equal(edge_tmr_voter_update_channel(&voter_store, 0, 100.0f, TMR_NODE_HEALTHY), 0);
    assert_int_equal(edge_tmr_voter_update_channel(&voter_store, 1, 100.0f, TMR_NODE_HEALTHY), 0);
    assert_int_equal(edge_tmr_voter_update_channel(&voter_store, 2, 100.0f, TMR_NODE_HEALTHY), 0);

    float voted = 0.0f;
    assert_int_equal(edge_tmr_voter_step(&voter_store, &voted), 0);
    assert_float_equal(voted, 100.0f, 0.01f);
}

static void test_ebpf_telemetry_caller_allocated_storage(void **state) {
    (void)state;

    edge_ebpf_telemetry_ctx_t ebpf_store;
    memset(&ebpf_store, 0x88, sizeof(ebpf_store));

    ebpf_telemetry_config_t cfg = {.jitter_warn_us = 20,
                                   .jitter_trip_us = 50,
                                   .hardirq_trip_us = 100,
                                   .consecutive_spike_threshold = 3,
                                   .recovery_healthy_cycles = 5,
                                   .target_task_pid = 4321};

    assert_int_equal(edge_ebpf_telemetry_init(&ebpf_store, &cfg), 0);
    assert_int_equal(edge_ebpf_telemetry_record_event(&ebpf_store, 10, 5), 0);

    ebpf_latency_stats_t stats;
    assert_int_equal(edge_ebpf_telemetry_get_stats(&ebpf_store, &stats), 0);
    assert_int_equal(stats.sample_count, 1);
}

static void test_thermal_scheduler_caller_allocated_storage(void **state) {
    (void)state;

    edge_thermal_scheduler_ctx_t thermal_store;
    memset(&thermal_store, 0x99, sizeof(thermal_store));

    thermal_scheduler_config_t cfg = {.temp_warn_threshold_c = 70.0f,
                                      .temp_degraded_threshold_c = 85.0f,
                                      .temp_emergency_threshold_c = 95.0f,
                                      .thermal_resistance_r_th = 1.2f,
                                      .thermal_capacitance_c_th = 50.0f,
                                      .max_tdp_power_w = 35.0f,
                                      .base_power_idle_w = 5.0f};

    assert_int_equal(edge_thermal_scheduler_init(&thermal_store, &cfg), 0);
    assert_int_equal(
        edge_thermal_scheduler_update_sensors(&thermal_store, 50.0f, 48.0f, 28.0f, 0.1f), 0);

    thermal_task_quota_t quota;
    assert_int_equal(edge_thermal_scheduler_arbitrate(&thermal_store, &quota), 0);
    assert_true(quota.level_a_allowed);
}

static void test_edge_uns_caller_allocated_storage(void **state) {
    (void)state;
    _Static_assert(sizeof(edge_uns_engine_t) > 0, "edge_uns_engine_t must have static size");
    edge_uns_engine_t uns_engine;
    memset(&uns_engine, 0x55, sizeof(uns_engine));
    assert_int_equal(edge_uns_engine_init(&uns_engine, "Grid", "Node_01"), 0);
    assert_int_equal(uns_engine.node_count, 0);
}

static void test_edge_tsn_caller_allocated_storage(void **state) {
    (void)state;
    _Static_assert(sizeof(edge_tsn_engine_t) > 0, "edge_tsn_engine_t must have static size");
    edge_tsn_engine_t tsn_engine;
    memset(&tsn_engine, 0x44, sizeof(tsn_engine));
    tsn_schedule_config_t cfg = {.interface_name = "eth0",
                                 .cycle_time_ns = 1000000,
                                 .base_time_ns = 0,
                                 .entry_count = 1,
                                 .entries = {{.gate_mask = 0xFF, .time_interval_ns = 1000000}}};
    assert_int_equal(edge_tsn_engine_init(&tsn_engine, &cfg), 0);
    assert_true(tsn_engine.is_active);
}

static void test_edge_amp_caller_allocated_storage(void **state) {
    (void)state;
    _Static_assert(sizeof(edge_amp_ipc_t) > 0, "edge_amp_ipc_t must have static size");
    edge_amp_ipc_t amp_ipc;
    memset(&amp_ipc, 0x33, sizeof(amp_ipc));
    assert_int_equal(edge_amp_ipc_init(&amp_ipc, NULL, 0), 0);
    assert_int_equal(amp_ipc.remote_core_state, AMP_CORE_STATE_RUNNING);
}

static void test_edge_vsg_caller_allocated_storage(void **state) {
    (void)state;
    _Static_assert(sizeof(edge_vsg_engine_t) > 0, "edge_vsg_engine_t must have static size");
    edge_vsg_engine_t vsg;
    memset(&vsg, 0x55, sizeof(vsg));
    edge_vsg_config_t cfg = {
        .j_inertia = 5.0f, .d_damping = 20.0f, .droop_p_f = 100.0f, .droop_q_v = 50.0f};
    assert_int_equal(edge_vsg_engine_init(&vsg, &cfg), 0);
    assert_true(vsg.is_initialized);
}

static void test_edge_swarm_caller_allocated_storage(void **state) {
    (void)state;
    _Static_assert(sizeof(edge_swarm_engine_t) > 0, "edge_swarm_engine_t must have static size");
    edge_swarm_engine_t swarm;
    memset(&swarm, 0x44, sizeof(swarm));
    assert_int_equal(edge_swarm_engine_init(&swarm, "node-local"), 0);
    assert_int_equal(swarm.node_count, 1);
}

static void test_edge_operator_fusion_caller_allocated_storage(void **state) {
    (void)state;
    _Static_assert(sizeof(edge_model_manager_t) > 0, "edge_model_manager_t must have static size");
    edge_model_manager_t mgr;
    memset(&mgr, 0x33, sizeof(mgr));
    uint8_t w[32] = {1, 2, 3};
    assert_int_equal(edge_model_mgr_init(&mgr, 101, w, sizeof(w)), 0);
    edge_ai_model_t *m = edge_model_acquire_active(&mgr);
    assert_non_null(m);
    edge_model_release(m);
}

static void test_battery_thermal_pinn_caller_allocated_storage(void **state) {
    (void)state;
    _Static_assert(sizeof(battery_thermal_pinn_t) > 0,
                   "battery_thermal_pinn_t must have static size");
    battery_thermal_pinn_t pinn;
    memset(&pinn, 0x55, sizeof(pinn));
    pinn_config_t cfg = {.nominal_capacity_ah = 280.0f,
                         .c_th_joule_per_k = 1500.0f,
                         .r_th_k_per_w = 0.8f,
                         .temp_ambient_c = 25.0f};
    assert_int_equal(battery_thermal_pinn_init(&pinn, &cfg), 0);
    assert_int_equal(pinn.channel_count, 0);
}

static void test_edge_wasm_nn_caller_allocated_storage(void **state) {
    (void)state;
    _Static_assert(sizeof(edge_wasm_nn_sandbox_t) > 0,
                   "edge_wasm_nn_sandbox_t must have static size");
    edge_wasm_nn_sandbox_t sandbox;
    memset(&sandbox, 0x44, sizeof(sandbox));
    assert_int_equal(edge_wasm_nn_sandbox_init(&sandbox, 1001, 50000), 0);
    assert_true(sandbox.is_active);
}

static void test_edge_tensor_pipeline_caller_allocated_storage(void **state) {
    (void)state;
    _Static_assert(sizeof(edge_tensor_pipeline_t) > 0,
                   "edge_tensor_pipeline_t must have static size");
    edge_tensor_pipeline_t pipe;
    memset(&pipe, 0x33, sizeof(pipe));
    tensor_domain_def_t def = {
        .domain_name = "test_tensor",
        .domain_id = 1,
        .stage_count = 1,
        .stages = {{.stage_id = 1, .stage_type = TENSOR_STAGE_THRESHOLD_ALARM}}};
    assert_int_equal(edge_tensor_pipeline_init(&pipe, &def), 0);
    assert_true(pipe.is_valid_dag);
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
        cmocka_unit_test(test_edge_pcp_mutex_caller_allocated_storage),
        cmocka_unit_test(test_edge_edf_scheduler_caller_allocated_storage),
        cmocka_unit_test(test_edge_rms_scheduler_caller_allocated_storage),
        cmocka_unit_test(test_edge_cpu_affinity_caller_allocated_storage),
        cmocka_unit_test(test_edge_prio_booster_caller_allocated_storage),
        cmocka_unit_test(test_edge_coop_guard_caller_allocated_storage),
        cmocka_unit_test(test_edge_mixed_crit_caller_allocated_storage),
        cmocka_unit_test(test_thermal_mesh_caller_allocated_storage),
        cmocka_unit_test(test_cep_engine_caller_allocated_storage),
        cmocka_unit_test(test_impedance_controller_caller_allocated_storage),
        cmocka_unit_test(test_dpi_engine_caller_allocated_storage),
        cmocka_unit_test(test_bus_orchestrator_caller_allocated_storage),
        cmocka_unit_test(test_driver_manager_caller_allocated_storage),
        cmocka_unit_test(test_edge_agent_caller_allocated_storage),
        cmocka_unit_test(test_protocol_bridge_caller_allocated_storage),
        cmocka_unit_test(test_snf_manager_caller_allocated_storage),
        cmocka_unit_test(test_edge_acl_caller_allocated_storage),
        cmocka_unit_test(test_persistence_mgr_caller_allocated_storage),
        cmocka_unit_test(test_driver_hotplug_caller_allocated_storage),
        cmocka_unit_test(test_iec104_apci_caller_allocated_storage),
        cmocka_unit_test(test_npu_pmu_caller_allocated_storage),
        cmocka_unit_test(test_npu_enclave_caller_allocated_storage),
        cmocka_unit_test(test_npu_power_mgr_caller_allocated_storage),
        cmocka_unit_test(test_ai_quant_calib_caller_allocated_storage),
        cmocka_unit_test(test_ai_awq_gptq_caller_allocated_storage),
        cmocka_unit_test(test_ai_temporal_fault_caller_allocated_storage),
        cmocka_unit_test(test_ai_dyn_pool_caller_allocated_storage),
        cmocka_unit_test(test_ai_slm_kv_cache_caller_allocated_storage),
        cmocka_unit_test(test_ai_graph_pruner_caller_allocated_storage),
        cmocka_unit_test(test_ai_fast_gemm_caller_allocated_storage),
        cmocka_unit_test(test_ai_pinn_evaluator_caller_allocated_storage),
        cmocka_unit_test(test_ai_hetero_pipeline_caller_allocated_storage),
        cmocka_unit_test(test_tmr_voter_caller_allocated_storage),
        cmocka_unit_test(test_ebpf_telemetry_caller_allocated_storage),
        cmocka_unit_test(test_thermal_scheduler_caller_allocated_storage),
        cmocka_unit_test(test_edge_uns_caller_allocated_storage),
        cmocka_unit_test(test_edge_tsn_caller_allocated_storage),
        cmocka_unit_test(test_edge_amp_caller_allocated_storage),
        cmocka_unit_test(test_edge_vsg_caller_allocated_storage),
        cmocka_unit_test(test_edge_swarm_caller_allocated_storage),
        cmocka_unit_test(test_edge_operator_fusion_caller_allocated_storage),
        cmocka_unit_test(test_battery_thermal_pinn_caller_allocated_storage),
        cmocka_unit_test(test_edge_wasm_nn_caller_allocated_storage),
        cmocka_unit_test(test_edge_tensor_pipeline_caller_allocated_storage),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
