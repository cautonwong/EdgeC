/**
 * @file test_npu_pmu.c
 * @brief Unit Tests for NPU PMU Hardware Performance Counter & Hotspot Profiler (AEP-748 / Issue
 * #278)
 */

#include <cmocka.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "datahub.h"
#include "npu_pmu.h"

static void test_npu_pmu_lifecycle_and_profiling(void **state) {
    (void)state;

    npu_pmu_storage_t storage;
    memset(&storage, 0xAA, sizeof(storage));

    npu_pmu_config_t cfg = {.peak_macs_per_cycle = 512,
                            .core_clock_ghz = 1.0f,
                            .jitter_threshold_sigma = 3.0f,
                            .enable_ring_buffer = true};

    npu_pmu_t *pmu = npu_pmu_create_in(&storage, &cfg);
    assert_non_null(pmu);

    /* 1. Profile Conv2D Layer */
    uint64_t token1 =
        npu_pmu_op_begin(pmu, 101, NPU_OP_CONV2D, "conv2d_stem", 5000000ULL, 1024 * 1024);
    assert_true(token1 > 0);

    /* Simulate computation */
    usleep(500); /* 500us */

    npu_pmu_record_t rec1;
    int rc = npu_pmu_op_end(pmu, token1, 10000, 200, 5000, 100, &rec1);
    assert_int_equal(rc, 0);
    assert_int_equal(rec1.op_id, 101);
    assert_int_equal(rec1.op_type, NPU_OP_CONV2D);
    assert_string_equal(rec1.op_name, "conv2d_stem");
    assert_true(rec1.latency_ns >= 400000); /* >= 400us */
    assert_true(rec1.cycles >= 400000);
    assert_true(rec1.l1_miss_pct > 0.0f && rec1.l1_miss_pct < 5.0f);
    assert_true(rec1.l2_miss_pct > 0.0f && rec1.l2_miss_pct < 5.0f);
    assert_true(rec1.mac_util_pct > 0.0f);
    assert_true(rec1.memory_bw_gbps > 0.0f);
    assert_false(rec1.is_jitter_anomaly);

    /* 2. Profile MatMul GEMM Layer */
    uint64_t token2 =
        npu_pmu_op_begin(pmu, 102, NPU_OP_MATMUL_GEMM, "matmul_head", 10000000ULL, 2048 * 1024);
    assert_true(token2 > token1);
    usleep(3000); /* 3000us */

    npu_pmu_record_t rec2;
    rc = npu_pmu_op_end(pmu, token2, 20000, 500, 10000, 250, &rec2);
    assert_int_equal(rc, 0);
    assert_int_equal(rec2.op_id, 102);

    /* 3. Check Aggregated Stats */
    npu_pmu_stats_t stats;
    rc = npu_pmu_get_stats(pmu, &stats);
    assert_int_equal(rc, 0);
    assert_int_equal(stats.total_ops_profiled, 2);
    assert_true(stats.total_cycles > 0);
    assert_true(stats.total_latency_ns > 0);
    assert_true(stats.avg_inference_latency_us > 0.0f);
    assert_true(stats.overall_ipc > 0.0f);

    /* 4. Top-K Hotspot Ranking */
    npu_pmu_hotspot_t hotspots[4];
    size_t count = npu_pmu_get_top_hotspots(pmu, hotspots, 4);
    assert_int_equal(count, 2);
    /* matmul_head (3000us) should rank higher than conv2d_stem (500us) */
    assert_int_equal(hotspots[0].op_id, 102);
    assert_string_equal(hotspots[0].op_name, "matmul_head");
    assert_int_equal(hotspots[1].op_id, 101);
    assert_string_equal(hotspots[1].op_name, "conv2d_stem");
    assert_true(hotspots[0].time_share_pct > hotspots[1].time_share_pct);
}

static void test_npu_pmu_jitter_anomaly_and_datahub_sync(void **state) {
    (void)state;

    npu_pmu_storage_t storage;
    npu_pmu_config_t cfg = {.peak_macs_per_cycle = 512,
                            .core_clock_ghz = 1.0f,
                            .jitter_threshold_sigma = 4.0f,
                            .enable_ring_buffer = true};

    npu_pmu_t *pmu = npu_pmu_create_in(&storage, &cfg);
    assert_non_null(pmu);

    /* Run 10 steady baseline invocations */
    for (int i = 0; i < 10; i++) {
        uint64_t tok =
            npu_pmu_op_begin(pmu, 201, NPU_OP_SOFTMAX, "softmax_layer", 50000ULL, 64 * 1024);
        usleep(300); /* ~300us steady baseline */
        npu_pmu_record_t rec;
        int rc = npu_pmu_op_end(pmu, tok, 1000, 10, 500, 5, &rec);
        assert_int_equal(rc, 0);
        assert_false(rec.is_jitter_anomaly);
    }

    /* Trigger a massive outlier jitter spike */
    uint64_t spike_tok =
        npu_pmu_op_begin(pmu, 201, NPU_OP_SOFTMAX, "softmax_layer", 50000ULL, 64 * 1024);
    usleep(5000); /* 5000us spike (>16x jump) */
    npu_pmu_record_t spike_rec;
    int rc = npu_pmu_op_end(pmu, spike_tok, 1000, 50, 500, 25, &spike_rec);
    assert_int_equal(rc, 0);
    assert_true(spike_rec.is_jitter_anomaly);

    npu_pmu_stats_t stats;
    npu_pmu_get_stats(pmu, &stats);
    assert_true(stats.jitter_anomaly_count >= 1);

    /* DataHub Sync Test */
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    shm_unlink("/test_npu_pmu_hub");
    assert_int_equal(
        datahub_init(&hub, &loop, "/test_npu_pmu_hub", 32, 64, NULL, 0, NULL, NULL, NULL), 0);

    rc = npu_pmu_sync_datahub(pmu, &hub);
    assert_int_equal(rc, 0);

    datahub_destroy(&hub);
    uv_loop_close(&loop);
    shm_unlink("/test_npu_pmu_hub");

    /* Reset */
    npu_pmu_reset(pmu);
    npu_pmu_get_stats(pmu, &stats);
    assert_int_equal(stats.total_ops_profiled, 0);
    assert_int_equal(stats.jitter_anomaly_count, 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_npu_pmu_lifecycle_and_profiling),
        cmocka_unit_test(test_npu_pmu_jitter_anomaly_and_datahub_sync),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
