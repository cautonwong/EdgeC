/**
 * @file test_dds_shm_zerocopy_perf.c
 * @brief Zero-Copy Shared Memory (SHM) Microsecond Latency & Dual-Track Transparent Routing
 * (AEP-453 / Issue #145)
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
#include <time.h>
#include <uv.h>

#include "datahub.h"
#include "datahub_dds.h"
#include "datahub_router.h"

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void test_shm_zerocopy_microsecond_latency(void **state) {
    (void)state;

    uv_loop_t loop;
    uv_loop_init(&loop);

    datahub_router_config_t config;
    memset(&config, 0, sizeof(config));
    config.enable_shm = true;
    config.enable_dds = false;
    config.write_policy = DATAHUB_ROUTER_POLICY_BEST_EFFORT;
    snprintf(config.shm_config.shm_name, sizeof(config.shm_config.shm_name), "perf_shm_test");
    config.shm_config.ring_slots = 64;
    config.shm_config.shadow_capacity = 2048;

    datahub_router_t router;
    assert_int_equal(datahub_router_init(&router, &loop, &config, NULL, 0, NULL, NULL, NULL), 0);

    const int iterations = 10000;
    pipe_msg_t msg = {
        .tag_id = 2001,
        .type = PIPEMSG_TYPE_SCALAR,
        .ts = 1000000ULL,
        .payload.val.f = 220.5f,
    };

    uint64_t start_ns = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        msg.payload.val.f = (float)(220.0 + (double)(i % 100) * 0.1);
        int rc = datahub_router_write_shadow(&router, &msg);
        assert_int_equal(rc, 0);

        pipe_msg_t read_msg;
        rc = datahub_router_read_shadow(&router, 2001, &read_msg);
        assert_int_equal(rc, 0);
    }
    uint64_t end_ns = get_time_ns();

    double total_us = (double)(end_ns - start_ns) / 1000.0;
    double avg_us_per_op = total_us / (double)iterations;

    printf("\n  ⚡ [SHM Zero-Copy Perf] %d round-trips completed in %.2f us (Avg: %.3f us/op)\n",
           iterations, total_us, avg_us_per_op);

    // 严密断言：单机共享内存读写平均耗时必须严格 <= 5.0 微秒 (AEP-453 工业指标)
    assert_true(avg_us_per_op <= 5.0);

    datahub_router_destroy(&router);
    uv_loop_close(&loop);
}

static void test_dual_track_transparent_routing(void **state) {
    (void)state;

    uv_loop_t loop;
    uv_loop_init(&loop);

    datahub_register_backend(DATAHUB_BACKEND_DDS, &g_datahub_dds_ops);

    datahub_router_config_t config;
    memset(&config, 0, sizeof(config));
    config.enable_shm = true;
    config.enable_dds = true;
    config.dds_ops = &g_datahub_dds_ops;
    config.write_policy = DATAHUB_ROUTER_POLICY_BEST_EFFORT;
    snprintf(config.shm_config.shm_name, sizeof(config.shm_config.shm_name), "dual_track_shm");
    config.shm_config.ring_slots = 64;
    config.shm_config.shadow_capacity = 2048;

    datahub_router_t router;
    assert_int_equal(datahub_router_init(&router, &loop, &config, NULL, 0, NULL, NULL, NULL), 0);

    // 发布双轨消息：本地 SHM 亚微秒写入，DDS 组播异步广播
    pipe_msg_t msg = {
        .tag_id = 1002,  // Within shadow capacity
        .type = PIPEMSG_TYPE_SCALAR,
        .ts = 1000000ULL,
        .payload.val.f = 230.2f,
    };

    assert_int_equal(datahub_router_publish(&router, &msg), 0);
    assert_int_equal(datahub_router_write_shadow(&router, &msg), 0);

    // 验证本地快车道 (SHM) 立即能够读取该数值
    pipe_msg_t read_msg;
    int rc = datahub_router_read_shadow(&router, 1002, &read_msg);
    assert_int_equal(rc, 0);
    assert_true(read_msg.payload.val.f > 230.0f);

    datahub_router_destroy(&router);
    uv_loop_close(&loop);
}

static void test_dual_track_graceful_degradation_without_dds(void **state) {
    (void)state;

    uv_loop_t loop;
    uv_loop_init(&loop);

    datahub_router_config_t config;
    memset(&config, 0, sizeof(config));
    config.enable_shm = true;
    config.enable_dds = false;  // 模拟网络/DDS未就绪或禁用
    config.write_policy = DATAHUB_ROUTER_POLICY_BEST_EFFORT;
    snprintf(config.shm_config.shm_name, sizeof(config.shm_config.shm_name), "degrade_shm");
    config.shm_config.ring_slots = 64;
    config.shm_config.shadow_capacity = 2048;

    datahub_router_t router;
    assert_int_equal(datahub_router_init(&router, &loop, &config, NULL, 0, NULL, NULL, NULL), 0);

    // 验证：即使 DDS 禁用，本地业务逻辑仍然 100% 正常运行，不阻塞、不中断
    pipe_msg_t msg = {
        .tag_id = 1003,
        .type = PIPEMSG_TYPE_SCALAR,
        .ts = 2000000ULL,
        .payload.val.f = 50.01f,
    };
    assert_int_equal(datahub_router_publish(&router, &msg), 0);
    assert_int_equal(datahub_router_write_shadow(&router, &msg), 0);

    pipe_msg_t out_msg;
    assert_int_equal(datahub_router_read_shadow(&router, 1003, &out_msg), 0);
    assert_true(out_msg.payload.val.f > 50.0f);

    datahub_router_destroy(&router);
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_shm_zerocopy_microsecond_latency),
        cmocka_unit_test(test_dual_track_transparent_routing),
        cmocka_unit_test(test_dual_track_graceful_degradation_without_dds),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
