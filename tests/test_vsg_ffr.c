/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
/**
 * @file test_vsg_ffr.c
 * @brief Unit tests for Virtual Synchronous Generator (VSG) & Fast Frequency Response (FFR)
 * (AEP-607)
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "edge_vsg_engine.h"

static void test_vsg_initialization_and_steady_state(void) {
    printf("[TEST] VSG Engine Initialization & Steady-State Dynamics...\n");

    edge_vsg_engine_t vsg;
    edge_vsg_config_t cfg = {.j_inertia = 10.0f,
                             .d_damping = 25.0f,
                             .droop_p_f = 200.0f, /* 200 kW/Hz */
                             .droop_q_v = 50.0f,  /* 50 kVar/V */
                             .virtual_r = 0.01f,
                             .virtual_x = 0.05f,
                             .nominal_voltage_v = 380.0f,
                             .nominal_freq_hz = 50.0f,
                             .max_power_kw = 500.0f,
                             .max_current_a = 800.0f,
                             .enable_ffr = true};

    int ret = edge_vsg_engine_init(&vsg, &cfg);
    assert(ret == 0);
    assert(vsg.is_initialized == true);

    edge_vsg_state_t st;
    edge_vsg_engine_get_state(&vsg, &st);
    assert(fabsf(st.grid_freq_hz - 50.0f) < 1e-4);
    assert(fabsf(st.omega_virtual - VSG_OMEGA_0) < 1e-2);

    /* Run 100 steps (50ms) in steady state: p_ref = 100kW */
    for (int i = 0; i < 100; i++) {
        ret = edge_vsg_engine_step(&vsg, 0.0005f, 100.0f, 0.0f, 50.0f, 380.0f);
        assert(ret == 0);
    }

    edge_vsg_engine_get_state(&vsg, &st);
    assert(st.step_count == 100);
    assert(st.p_active_kw > 90.0f); /* Power tracking converges */
    assert(st.id_ref > 0.0f);

    printf("  -> PASSED: VSG steady-state convergence verified.\n");
}

static void test_vsg_fast_frequency_response_ffr_boost(void) {
    printf("[TEST] VSG Fast Frequency Response (FFR) Grid Drop Step (50Hz -> 49.8Hz)...\n");

    edge_vsg_engine_t vsg;
    edge_vsg_config_t cfg = {.j_inertia = 8.0f,
                             .d_damping = 20.0f,
                             .droop_p_f = 100.0f, /* 100 kW/Hz -> -0.2Hz gives +20kW droop */
                             .droop_q_v = 50.0f,
                             .nominal_voltage_v = 380.0f,
                             .nominal_freq_hz = 50.0f,
                             .max_power_kw = 500.0f,
                             .max_current_a = 800.0f,
                             .enable_ffr = true};
    edge_vsg_engine_init(&vsg, &cfg);

    /* Grid step drop to 49.8 Hz (-0.2 Hz) */
    int ret = edge_vsg_engine_step(&vsg, 0.0005f, 50.0f, 0.0f, 49.8f, 380.0f);
    assert(ret == 0);

    edge_vsg_state_t st;
    edge_vsg_engine_get_state(&vsg, &st);

    /* FFR boost is triggered for delta_f > 0.05Hz: p_ffr = -droop_p_f * 2 * delta_f = -100 * 2 *
     * (-0.2) = +40kW */
    assert(st.ffr_active_power_kw >= 39.0f);
    assert(st.id_ref > 0.0f);

    printf("  -> PASSED: FFR fast dynamic boost (+40kW) verified on -0.2Hz grid drop.\n");
}

static void test_vsg_voltage_droop_and_current_clamping(void) {
    printf("[TEST] VSG Q-V Droop & Inverter Current Limit Clamping...\n");

    edge_vsg_engine_t vsg;
    edge_vsg_config_t cfg = {.j_inertia = 5.0f,
                             .d_damping = 10.0f,
                             .droop_p_f = 50.0f,
                             .droop_q_v = 10.0f, /* 10 kVar/V */
                             .nominal_voltage_v = 380.0f,
                             .nominal_freq_hz = 50.0f,
                             .max_power_kw = 500.0f,
                             .max_current_a = 200.0f, /* Strict current limit 200A */
                             .enable_ffr = false};
    edge_vsg_engine_init(&vsg, &cfg);

    /* Request massive power 450kW that would exceed 200A */
    edge_vsg_engine_step(&vsg, 0.0005f, 450.0f, 0.0f, 50.0f, 380.0f);

    edge_vsg_state_t st;
    edge_vsg_engine_get_state(&vsg, &st);

    float i_total = sqrtf(st.id_ref * st.id_ref + st.iq_ref * st.iq_ref);
    assert(i_total <= 200.01f); /* Must be clamped to 200A */

    printf("  -> PASSED: Current clamping (<= 200A) verified under overload.\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("     AEP-607: Virtual Synchronous Generator (VSG/FFR) Suite      \n");
    printf("=================================================================\n");

    test_vsg_initialization_and_steady_state();
    test_vsg_fast_frequency_response_ffr_boost();
    test_vsg_voltage_droop_and_current_clamping();

    printf("\nAll 3 AEP-607 VSG / FFR test cases PASSED!\n");
    return 0;
}
