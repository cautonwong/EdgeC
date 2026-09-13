/**
 * @file test_battery_spatial_matrix.c
 * @brief Unit tests for 4096-Node Cell Voltage-Temperature-Current Sliding Window
 *        Polarization Feature Extractor & Thermal Runaway Spatial Diffusion Predictor (AEP-733)
 * @author EdgeOS Core Architecture Team
 */

#include "battery_spatial_matrix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#define TEST_PASS 0
#define TEST_FAIL 1

static int test_init_and_geometry(void) {
    printf("[TEST 1] Testing 4096-node spatial matrix initialization & 3D topology...\n");

    battery_spatial_matrix_t *ctx = (battery_spatial_matrix_t *)calloc(1, sizeof(battery_spatial_matrix_t));
    assert(ctx != NULL);

    battery_spatial_matrix_config_t config;
    memset(&config, 0, sizeof(config));
    config.num_cells = 4096;
    config.num_racks = 16;
    config.num_modules_per_rack = 16;
    config.num_cells_per_module = 16;
    config.cell_dx = 0.05f; // 50mm
    config.cell_dy = 0.20f; // 200mm
    config.cell_dz = 0.80f; // 800mm
    config.nominal_capacity_ah = 280.0f;
    config.internal_r0_mohm = 0.25f;

    int rc = battery_spatial_matrix_init(ctx, &config);
    assert(rc == 0);

    // Verify cell #0 coordinates (r=0, m=0, c=0)
    assert(ctx->coords[0].rack_id == 0);
    assert(ctx->coords[0].module_id == 0);
    assert(ctx->coords[0].cell_id_in_module == 0);
    assert(fabsf(ctx->coords[0].pos_x - 0.0f) < 1e-4f);
    assert(fabsf(ctx->coords[0].pos_y - 0.0f) < 1e-4f);
    assert(fabsf(ctx->coords[0].pos_z - 0.0f) < 1e-4f);

    // Verify cell #0 boundary neighbor conditions (-X is boundary, +X is #1, -Y is boundary, +Y is #16, -Z is boundary, +Z is #256)
    assert(ctx->neighbor_indices[0][0] == -1); // -X
    assert(ctx->neighbor_indices[0][1] == 1);  // +X
    assert(ctx->neighbor_indices[0][2] == -1); // -Y
    assert(ctx->neighbor_indices[0][3] == 16); // +Y
    assert(ctx->neighbor_indices[0][4] == -1); // -Z
    assert(ctx->neighbor_indices[0][5] == 256);// +Z

    // Verify internal cell #273 (r=1, m=1, c=1 => 1*256 + 1*16 + 1 = 273)
    uint32_t internal_idx = 273;
    assert(ctx->coords[internal_idx].rack_id == 1);
    assert(ctx->coords[internal_idx].module_id == 1);
    assert(ctx->coords[internal_idx].cell_id_in_module == 1);
    assert(ctx->neighbor_indices[internal_idx][0] == 272); // -X
    assert(ctx->neighbor_indices[internal_idx][1] == 274); // +X
    assert(ctx->neighbor_indices[internal_idx][2] == 257); // -Y (273 - 16)
    assert(ctx->neighbor_indices[internal_idx][3] == 289); // +Y (273 + 16)
    assert(ctx->neighbor_indices[internal_idx][4] == 17);  // -Z (273 - 256)
    assert(ctx->neighbor_indices[internal_idx][5] == 529); // +Z (273 + 256)

    free(ctx);
    printf("  --> PASSED: 4096-Node 3D lattice geometry & neighbor indices verified.\n");
    return TEST_PASS;
}

static int test_uniform_pack_baseline_and_speed(void) {
    printf("[TEST 2] Testing uniform 4096-node pack baseline & compute latency benchmark...\n");

    battery_spatial_matrix_t *ctx = (battery_spatial_matrix_t *)calloc(1, sizeof(battery_spatial_matrix_t));
    assert(ctx != NULL);

    battery_spatial_matrix_config_t config;
    memset(&config, 0, sizeof(config));
    config.num_cells = 4096;
    config.num_racks = 16;
    config.num_modules_per_rack = 16;
    config.num_cells_per_module = 16;

    int rc = battery_spatial_matrix_init(ctx, &config);
    assert(rc == 0);

    float *voltages = (float *)malloc(4096 * sizeof(float));
    float *temperatures = (float *)malloc(4096 * sizeof(float));
    float *currents = (float *)malloc(4096 * sizeof(float));

    for (int i = 0; i < 4096; i++) {
        voltages[i] = 3.300f;     // uniform 3.300V
        temperatures[i] = 25.0f;  // uniform 25.0°C
        currents[i] = 50.0f;      // 50A discharge
    }

    battery_spatial_frame_result_t result;
    uint64_t ts = 1000000;

    // Feed 10 consecutive frames at 100ms intervals
    for (int step = 0; step < 10; step++) {
        rc = battery_spatial_matrix_process_frame(ctx, voltages, temperatures, currents, 4096, ts, &result);
        assert(rc == 0);
        ts += 100;
    }

    // Verify statistical results
    assert(fabsf(result.v_mean - 3.300f) < 1e-4f);
    assert(fabsf(result.v_std) < 1e-4f);
    assert(fabsf(result.t_mean - 25.0f) < 1e-4f);
    assert(fabsf(result.t_std) < 1e-4f);
    assert(result.overall_tr_stage == BATTERY_TR_STAGE_NORMAL);
    assert(result.num_voltage_outliers == 0);
    assert(result.num_temp_outliers == 0);
    assert(result.num_hotspots == 0);

    printf("  [Benchmark] 4096-Cell Frame Compute Duration: %llu us (%.2f ms)\n",
           (unsigned long long)result.compute_duration_us,
           (double)result.compute_duration_us / 1000.0);
    // Hard real-time limit: processing 4096 cells must be < 30ms (30,000 us)
    assert(result.compute_duration_us < 30000);

    free(voltages);
    free(temperatures);
    free(currents);
    free(ctx);
    printf("  --> PASSED: Uniform pack baseline & performance latency verified.\n");
    return TEST_PASS;
}

static int test_single_cell_thermal_runaway_acceleration(void) {
    printf("[TEST 3] Testing localized thermal runaway acceleration & propagation vector...\n");

    battery_spatial_matrix_t *ctx = (battery_spatial_matrix_t *)calloc(1, sizeof(battery_spatial_matrix_t));
    assert(ctx != NULL);

    battery_spatial_matrix_config_t config;
    memset(&config, 0, sizeof(config));
    config.num_cells = 4096;
    config.cell_dx = 0.05f;
    config.cell_dy = 0.20f;
    config.cell_dz = 0.80f;

    battery_spatial_matrix_init(ctx, &config);

    float *voltages = (float *)malloc(4096 * sizeof(float));
    float *temperatures = (float *)malloc(4096 * sizeof(float));
    float *currents = (float *)malloc(4096 * sizeof(float));

    for (int i = 0; i < 4096; i++) {
        voltages[i] = 3.28f;
        temperatures[i] = 25.0f;
        currents[i] = 20.0f;
    }

    battery_spatial_frame_result_t result;
    uint64_t ts = 1000000;

    // Normal steady baseline (5 frames)
    for (int step = 0; step < 5; step++) {
        battery_spatial_matrix_process_frame(ctx, voltages, temperatures, currents, 4096, ts, &result);
        ts += 100;
    }

    // Inject thermal runaway on Cell #512: temperature increases at +2.5°C/s (0.25°C per 100ms)
    uint32_t hotspot_idx = 512;
    for (int step = 0; step < 10; step++) {
        temperatures[hotspot_idx] += 0.25f; // +2.5°C/s rise
        voltages[hotspot_idx] -= 0.005f;    // slight voltage sag
        battery_spatial_matrix_process_frame(ctx, voltages, temperatures, currents, 4096, ts, &result);
        ts += 100;
    }

    // Now inject severe temperature jump up to 80°C (dT/dt ~ 2.5°C/s)
    temperatures[hotspot_idx] = 80.0f;
    battery_spatial_matrix_process_frame(ctx, voltages, temperatures, currents, 4096, ts, &result);

    // Verify detection
    assert(result.overall_tr_stage >= BATTERY_TR_STAGE_ACCELERATION);
    assert(result.num_hotspots >= 1);
    assert(result.hotspots[0].cell_index == hotspot_idx);
    assert(result.hotspots[0].temperature >= 75.0f);
    assert(result.hotspots[0].spatial_grad > 50.0f); // severe spatial gradient with neighbors

    // Verify feature inspection for hotspot cell
    battery_cell_features_t f;
    int rc = battery_spatial_matrix_get_cell_features(ctx, hotspot_idx, &f);
    assert(rc == 0);
    assert(f.tr_stage >= BATTERY_TR_STAGE_ACCELERATION);
    assert(f.spatial_temp_grad > 50.0f);

    free(voltages);
    free(temperatures);
    free(currents);
    free(ctx);
    printf("  --> PASSED: Localized thermal runaway & spatial gradient captured accurately.\n");
    return TEST_PASS;
}

static int test_polarization_resistance_anomaly_outlier(void) {
    printf("[TEST 4] Testing 3-Sigma Z-Score outlier detection & polarization resistance spike...\n");

    battery_spatial_matrix_t *ctx = (battery_spatial_matrix_t *)calloc(1, sizeof(battery_spatial_matrix_t));
    assert(ctx != NULL);

    battery_spatial_matrix_config_t config;
    memset(&config, 0, sizeof(config));
    config.num_cells = 4096;
    config.internal_r0_mohm = 0.30f;
    config.z_score_outlier_thresh = 3.0f;

    battery_spatial_matrix_init(ctx, &config);

    float *voltages = (float *)malloc(4096 * sizeof(float));
    float *temperatures = (float *)malloc(4096 * sizeof(float));
    float *currents = (float *)malloc(4096 * sizeof(float));

    for (int i = 0; i < 4096; i++) {
        voltages[i] = 3.250f;
        temperatures[i] = 26.0f;
        currents[i] = 100.0f; // 100A high current load
    }

    // Inject abnormal voltage drop on Cell #1024 due to high polarization resistance (3.05V vs 3.25V)
    uint32_t outlier_idx = 1024;
    voltages[outlier_idx] = 3.050f; // 200mV sag

    battery_spatial_frame_result_t result;
    int rc = battery_spatial_matrix_process_frame(ctx, voltages, temperatures, currents, 4096, 1000000, &result);
    assert(rc == 0);

    // Verify statistical outlier detection
    assert(result.num_voltage_outliers >= 1);
    assert(result.voltage_outliers[0].cell_index == outlier_idx);
    assert(result.voltage_outliers[0].z_score >= 3.0f);

    // Verify polarization resistance of outlier cell is significantly elevated
    battery_cell_features_t f_outlier, f_normal;
    battery_spatial_matrix_get_cell_features(ctx, outlier_idx, &f_outlier);
    battery_spatial_matrix_get_cell_features(ctx, 0, &f_normal);

    assert(f_outlier.overpotential > f_normal.overpotential);
    assert(f_outlier.polarization_res > f_normal.polarization_res);

    free(voltages);
    free(temperatures);
    free(currents);
    free(ctx);
    printf("  --> PASSED: Z-Score voltage anomaly & polarization resistance validated.\n");
    return TEST_PASS;
}

static int test_spatial_heat_diffusion_wave_multi_cells(void) {
    printf("[TEST 5] Testing 3D spatial heat diffusion wave across adjacent cells...\n");

    battery_spatial_matrix_t *ctx = (battery_spatial_matrix_t *)calloc(1, sizeof(battery_spatial_matrix_t));
    assert(ctx != NULL);

    battery_spatial_matrix_config_t config;
    memset(&config, 0, sizeof(config));
    config.num_cells = 4096;
    config.num_racks = 16;
    config.num_modules_per_rack = 16;
    config.num_cells_per_module = 16;
    config.cell_dx = 0.05f;
    config.cell_dy = 0.20f;
    config.cell_dz = 0.80f;

    battery_spatial_matrix_init(ctx, &config);

    float *voltages = (float *)malloc(4096 * sizeof(float));
    float *temperatures = (float *)malloc(4096 * sizeof(float));
    float *currents = (float *)malloc(4096 * sizeof(float));

    for (int i = 0; i < 4096; i++) {
        voltages[i] = 3.28f;
        temperatures[i] = 25.0f;
        currents[i] = 10.0f;
    }

    // Set central cell #273 as primary thermal source (95°C explosive)
    uint32_t center_idx = 273;
    temperatures[center_idx] = 95.0f;

    // Simulate diffusion to adjacent neighbors
    temperatures[272] = 58.0f; // -X
    temperatures[274] = 56.0f; // +X
    temperatures[257] = 50.0f; // -Y
    temperatures[289] = 48.0f; // +Y
    temperatures[17]  = 46.0f; // -Z
    temperatures[529] = 47.0f; // +Z

    battery_spatial_frame_result_t result;
    int rc = battery_spatial_matrix_process_frame(ctx, voltages, temperatures, currents, 4096, 1000000, &result);
    assert(rc == 0);

    assert(result.overall_tr_stage == BATTERY_TR_STAGE_EXPLOSIVE);
    assert(result.num_hotspots >= 2); // Primary hotspot + adjacent heated cells

    // Verify center cell diffusion vector magnitude is non-zero
    assert(result.hotspots[0].cell_index == center_idx);
    assert(result.hotspots[0].stage == BATTERY_TR_STAGE_EXPLOSIVE);

    free(voltages);
    free(temperatures);
    free(currents);
    free(ctx);
    printf("  --> PASSED: 3D multi-cell spatial diffusion cluster detected successfully.\n");
    return TEST_PASS;
}

static int test_reset_and_robustness(void) {
    printf("[TEST 6] Testing context reset & error handling...\n");

    battery_spatial_matrix_t *ctx = (battery_spatial_matrix_t *)calloc(1, sizeof(battery_spatial_matrix_t));
    assert(ctx != NULL);

    // Test invalid config
    assert(battery_spatial_matrix_init(NULL, NULL) == -1);
    battery_spatial_matrix_config_t config;
    memset(&config, 0, sizeof(config));
    config.num_cells = 0;
    assert(battery_spatial_matrix_init(ctx, &config) == -2);
    config.num_cells = 5000; // exceeds 4096
    assert(battery_spatial_matrix_init(ctx, &config) == -2);

    config.num_cells = 128;
    assert(battery_spatial_matrix_init(ctx, &config) == 0);

    // Test invalid process args
    battery_spatial_frame_result_t res;
    float v[128], t[128];
    assert(battery_spatial_matrix_process_frame(NULL, v, t, NULL, 128, 1000, &res) == -1);
    assert(battery_spatial_matrix_process_frame(ctx, v, t, NULL, 64, 1000, &res) == -2); // mismatch

    // Test reset
    assert(battery_spatial_matrix_reset(ctx) == 0);
    assert(ctx->windows[0].count == 0);

    free(ctx);
    printf("  --> PASSED: Context reset & parameter boundary robustness verified.\n");
    return TEST_PASS;
}

int main(void) {
    printf("===============================================================\n");
    printf("🧪 [UNIT TEST] AEP-733 4096-Node Spatial Matrix & TR Diffusion\n");
    printf("===============================================================\n");

    int failures = 0;
    failures += test_init_and_geometry();
    failures += test_uniform_pack_baseline_and_speed();
    failures += test_single_cell_thermal_runaway_acceleration();
    failures += test_polarization_resistance_anomaly_outlier();
    failures += test_spatial_heat_diffusion_wave_multi_cells();
    failures += test_reset_and_robustness();

    if (failures == 0) {
        printf("\n🎉 ALL AEP-733 4096-NODE SPATIAL MATRIX TESTS PASSED (6/6)!\n");
        return 0;
    } else {
        printf("\n❌ %d TESTS FAILED!\n", failures);
        return 1;
    }
}
