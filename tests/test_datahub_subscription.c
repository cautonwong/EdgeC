/**
 * @file test_datahub_subscription.c
 * @brief CMocka Unit Tests and Benchmark for DataHub Fine-Grained Subscription Engine
 * (ARCH-DEEP-01)
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
#include <time.h>

#include "datahub.h"

typedef struct {
    int call_count;
    pipe_msg_t last_msg;
    void *user_ctx;
} test_subscriber_cb_ctx_t;

static void test_sub_callback(datahub_t *hub, const pipe_msg_t *msg, void *user_ctx) {
    (void)hub;
    test_subscriber_cb_ctx_t *ctx = (test_subscriber_cb_ctx_t *)user_ctx;
    if (ctx && msg) {
        ctx->call_count++;
        ctx->last_msg = *msg;
    }
}

static void test_exact_tag_subscription(void **state) {
    (void)state;
    datahub_t hub;
    int rc = datahub_init(&hub, NULL, "sub_exact_test", 64, 0x40000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    test_subscriber_cb_ctx_t ctx_a = {0};
    test_subscriber_cb_ctx_t ctx_b = {0};

    int sub_a = datahub_subscribe_tag(&hub, 0x2001, test_sub_callback, &ctx_a);
    int sub_b = datahub_subscribe_tag(&hub, 0x2002, test_sub_callback, &ctx_b);

    assert_true(sub_a > 0);
    assert_true(sub_b > 0);

    pipe_msg_t msg1 = {0};
    msg1.tag_id = 0x2001;
    msg1.type = PIPEMSG_TYPE_SCALAR;
    msg1.payload.val.f = 220.5;
    datahub_write_shadow(&hub, &msg1);

    assert_int_equal(ctx_a.call_count, 1);
    assert_int_equal(ctx_b.call_count, 0);
    assert_int_equal(ctx_a.last_msg.tag_id, 0x2001);

    pipe_msg_t msg2 = {0};
    msg2.tag_id = 0x2002;
    msg2.type = PIPEMSG_TYPE_SCALAR;
    msg2.payload.val.f = 50.0;
    datahub_write_shadow(&hub, &msg2);

    assert_int_equal(ctx_a.call_count, 1);
    assert_int_equal(ctx_b.call_count, 1);
    assert_int_equal(ctx_b.last_msg.tag_id, 0x2002);

    pipe_msg_t msg3 = {0};
    msg3.tag_id = 0x3000;
    msg3.type = PIPEMSG_TYPE_SCALAR;
    msg3.payload.val.f = 100.0;
    datahub_write_shadow(&hub, &msg3);

    assert_int_equal(ctx_a.call_count, 1);
    assert_int_equal(ctx_b.call_count, 1);

    datahub_destroy(&hub);
}

static void test_domain_and_device_filter(void **state) {
    (void)state;
    datahub_t hub;
    int rc = datahub_init(&hub, NULL, "sub_domain_test", 64, 0x40000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    test_subscriber_cb_ctx_t ctx_alarm = {0};
    test_subscriber_cb_ctx_t ctx_dev42 = {0};

    // 订阅报警域 (前缀 0x0002xxxx)
    int sub_alarm = datahub_subscribe_domain(&hub, 0x0002, test_sub_callback, &ctx_alarm);
    assert_true(sub_alarm > 0);

    // 订阅指定物理设备 Device 42
    datahub_sub_filter_t filter_dev = {.tag_id = 0,
                                       .tag_mask = 0,
                                       .domain_mask = 0,
                                       .device_id = 42,
                                       .notify_on_change_only = false,
                                       .deadband = 0.0};
    int sub_dev42 = datahub_subscribe_filtered(&hub, &filter_dev, test_sub_callback, &ctx_dev42);
    assert_true(sub_dev42 > 0);

    // 消息 1: 报警域消息，设备 10 -> 仅 Alarm 接收
    pipe_msg_t msg1 = {0};
    msg1.tag_id = 0x00020010;
    msg1.device_id = 10;
    msg1.type = PIPEMSG_TYPE_SCALAR;
    msg1.payload.val.f = 1.0;
    datahub_write_shadow(&hub, &msg1);

    assert_int_equal(ctx_alarm.call_count, 1);
    assert_int_equal(ctx_dev42.call_count, 0);

    // 消息 2: 遥测域消息，设备 42 -> 仅 Dev42 接收
    pipe_msg_t msg2 = {0};
    msg2.tag_id = 0x00010020;
    msg2.device_id = 42;
    msg2.type = PIPEMSG_TYPE_SCALAR;
    msg2.payload.val.f = 380.0;
    datahub_write_shadow(&hub, &msg2);

    assert_int_equal(ctx_alarm.call_count, 1);
    assert_int_equal(ctx_dev42.call_count, 1);

    // 消息 3: 报警域消息，设备 42 -> 两者均接收
    pipe_msg_t msg3 = {0};
    msg3.tag_id = 0x00020099;
    msg3.device_id = 42;
    msg3.type = PIPEMSG_TYPE_SCALAR;
    msg3.payload.val.f = 99.0;
    datahub_write_shadow(&hub, &msg3);

    assert_int_equal(ctx_alarm.call_count, 2);
    assert_int_equal(ctx_dev42.call_count, 2);

    datahub_destroy(&hub);
}

static void test_deadband_and_rbe(void **state) {
    (void)state;
    datahub_t hub;
    int rc = datahub_init(&hub, NULL, "sub_deadband_test", 64, 0x40000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    test_subscriber_cb_ctx_t ctx = {0};
    datahub_sub_filter_t filter = {.tag_id = 0x1000,
                                   .tag_mask = 0,
                                   .domain_mask = 0,
                                   .device_id = EDGE_DEVICE_ANY,
                                   .notify_on_change_only = true,
                                   .deadband = 0.5};

    int sub_id = datahub_subscribe_filtered(&hub, &filter, test_sub_callback, &ctx);
    assert_true(sub_id > 0);

    pipe_msg_t msg = {0};
    msg.tag_id = 0x1000;
    msg.type = PIPEMSG_TYPE_SCALAR;

    // 首次写入 -> 必须触发通知 (count = 1)
    msg.payload.val.f = 10.0;
    datahub_write_shadow(&hub, &msg);
    assert_int_equal(ctx.call_count, 1);

    // 写入 10.2 (变化 0.2 < 0.5 死区) -> 必须抑制 (count = 1)
    msg.payload.val.f = 10.2;
    datahub_write_shadow(&hub, &msg);
    assert_int_equal(ctx.call_count, 1);

    // 写入 10.4 (变化 0.4 < 0.5 死区) -> 必须抑制 (count = 1)
    msg.payload.val.f = 10.4;
    datahub_write_shadow(&hub, &msg);
    assert_int_equal(ctx.call_count, 1);

    // 写入 10.6 (相较于上次通知值 10.0 变化 0.6 > 0.5) -> 必须触发通知 (count = 2)
    msg.payload.val.f = 10.6;
    datahub_write_shadow(&hub, &msg);
    assert_int_equal(ctx.call_count, 2);

    // 重复写入 10.6 -> 必须抑制 (count = 2)
    datahub_write_shadow(&hub, &msg);
    assert_int_equal(ctx.call_count, 2);

    // 写入 11.2 (变化 0.6 > 0.5) -> 必须触发通知 (count = 3)
    msg.payload.val.f = 11.2;
    datahub_write_shadow(&hub, &msg);
    assert_int_equal(ctx.call_count, 3);

    datahub_destroy(&hub);
}

static void test_unsubscribe_and_slot_reuse(void **state) {
    (void)state;
    datahub_t hub;
    int rc = datahub_init(&hub, NULL, "sub_unsub_test", 64, 0x40000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    test_subscriber_cb_ctx_t ctx = {0};
    int sub_id = datahub_subscribe_tag(&hub, 0x5000, test_sub_callback, &ctx);
    assert_true(sub_id > 0);

    pipe_msg_t msg = {0};
    msg.tag_id = 0x5000;
    msg.type = PIPEMSG_TYPE_SCALAR;
    msg.payload.val.f = 1.0;
    datahub_write_shadow(&hub, &msg);
    assert_int_equal(ctx.call_count, 1);

    // 注销订阅
    rc = datahub_unsubscribe(&hub, sub_id);
    assert_int_equal(rc, 0);

    // 再次写入，验证回调不再被触发
    datahub_write_shadow(&hub, &msg);
    assert_int_equal(ctx.call_count, 1);

    // 再次注销相同 ID 应返回 -1
    rc = datahub_unsubscribe(&hub, sub_id);
    assert_int_equal(rc, -1);

    datahub_destroy(&hub);
}

static void test_subscription_dispatch_microbenchmark(void **state) {
    (void)state;
    datahub_t hub;
    int rc = datahub_init(&hub, NULL, "sub_bench_test", 64, 0x40000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    test_subscriber_cb_ctx_t ctx_arr[64];
    memset(ctx_arr, 0, sizeof(ctx_arr));

    // 满载注册 64 个活跃订阅者
    for (int i = 0; i < 64; i++) {
        int sub_id =
            datahub_subscribe_tag(&hub, (uint32_t)(0x1000 + i), test_sub_callback, &ctx_arr[i]);
        assert_true(sub_id > 0);
    }

    const int iterations = 10000;
    pipe_msg_t msg = {0};
    msg.type = PIPEMSG_TYPE_SCALAR;
    msg.payload.val.f = 123.456;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < iterations; i++) {
        msg.tag_id = (uint32_t)(0x1000 + (i % 64));
        datahub_write_shadow(&hub, &msg);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    uint64_t elapsed_ns = (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL +
                          (uint64_t)(end.tv_nsec - start.tv_nsec);
    double ns_per_write = (double)elapsed_ns / (double)iterations;

    printf("\n  [BENCHMARK] 64 Active Subscribers: %d writes executed in %.3f ms (%.2f ns/write)\n",
           iterations, (double)elapsed_ns / 1000000.0, ns_per_write);

    // 验证每个订阅者精确接收到通知
    for (int i = 0; i < 64; i++) {
        int expected = (iterations / 64) + (i < (iterations % 64) ? 1 : 0);
        assert_int_equal(ctx_arr[i].call_count, expected);
    }

    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_exact_tag_subscription),
        cmocka_unit_test(test_domain_and_device_filter),
        cmocka_unit_test(test_deadband_and_rbe),
        cmocka_unit_test(test_unsubscribe_and_slot_reuse),
        cmocka_unit_test(test_subscription_dispatch_microbenchmark),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
