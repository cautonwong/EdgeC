/**
 * @file test_driver_mqtt_json.c
 * @brief 裸 JSON 北向驱动（ADR-0011 N4-4）
 *
 * 这个驱动是"可插拔"的**对照样本**：地址规则、载荷格式、BIRTH 语义都与
 * Sparkplug B 不同。它和 sparkplug 用同一套 `req_*` 槽位跑通，就说明
 * 编排层确实不关心载荷格式。
 *
 * 重点锁两件事：
 *   1. 载荷是**合法且正确**的 JSON（名字/值都在，精度不丢）
 *   2. 名字里混进引号/反斜杠时**拒绝**，不拼出坏 JSON
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"

int mqtt_json_driver_register(driver_manager_t* mgr);

/* 把 net_buf 里的字节当 C 字符串用（驱动产出的都是 NUL 之外的可打印文本，
 * 这里额外补一个 NUL 便于断言） */
static const char* buf_text(net_buf_t* nb, char* dst, size_t dst_sz) {
    size_t n = nb->b.len < dst_sz - 1 ? nb->b.len : dst_sz - 1;
    memcpy(dst, nb->b.data, n);
    dst[n] = '\0';
    return dst;
}

static void test_encode_ndata(void** state) {
    (void)state;
    driver_manager_t* mgr = driver_manager_create();
    assert_non_null(mgr);
    assert_int_equal(mqtt_json_driver_register(mgr), 0);

    edge_driver_ctx_t* ctx = driver_instance_create(mgr, "MqttJson", 1);
    assert_non_null(ctx);
    ctx->pool = net_buf_pool_new(8, 512, 0);
    assert_non_null(ctx->pool);

    alignas(max_align_t) uint8_t scratch[REQ_DRV_PRIV_MAX];
    memset(scratch, 0, sizeof(scratch));

    const char* name = "VOLTAGE_A";
    assert_int_equal(ctx->driver->req_prepare(scratch, 1, 1001, NULL, 0, name, strlen(name) + 1,
                                              true, 23.5),
                     0);

    net_buf_t* nb = net_buf_alloc(ctx->pool, 0);
    assert_non_null(nb);
    assert_int_equal(ctx->driver->req_encode(scratch, ctx->priv, nb), 0);

    char text[256];
    buf_text(nb, text, sizeof(text));
    assert_string_equal(text, "{\"p\":\"VOLTAGE_A\",\"v\":23.5}");

    net_buf_unref(nb);
    net_buf_pool_unref(ctx->pool);
    driver_instance_destroy(ctx, "MqttJson");
    driver_manager_destroy(mgr);
}

static void test_encode_keeps_precision(void** state) {
    (void)state;
    driver_manager_t* mgr = driver_manager_create();
    mqtt_json_driver_register(mgr);
    edge_driver_ctx_t* ctx = driver_instance_create(mgr, "MqttJson", 1);
    assert_non_null(ctx);
    ctx->pool = net_buf_pool_new(8, 512, 0);

    alignas(max_align_t) uint8_t scratch[REQ_DRV_PRIV_MAX];
    memset(scratch, 0, sizeof(scratch));

    /* 0.123456 用 %.2f 会变成 0.12 —— 电量类数据不能这样截 */
    const char* name = "ENERGY";
    assert_int_equal(
        ctx->driver->req_prepare(scratch, 1, 1, NULL, 0, name, strlen(name) + 1, true, 0.123456),
        0);

    net_buf_t* nb = net_buf_alloc(ctx->pool, 0);
    assert_int_equal(ctx->driver->req_encode(scratch, ctx->priv, nb), 0);

    char text[256];
    buf_text(nb, text, sizeof(text));
    assert_non_null(strstr(text, "0.123456"));

    net_buf_unref(nb);
    net_buf_pool_unref(ctx->pool);
    driver_instance_destroy(ctx, "MqttJson");
    driver_manager_destroy(mgr);
}

static void test_rejects_unsafe_name(void** state) {
    (void)state;
    driver_manager_t* mgr = driver_manager_create();
    mqtt_json_driver_register(mgr);
    edge_driver_ctx_t* ctx = driver_instance_create(mgr, "MqttJson", 1);
    assert_non_null(ctx);

    alignas(max_align_t) uint8_t scratch[REQ_DRV_PRIV_MAX];
    memset(scratch, 0, sizeof(scratch));

    /* 带引号的名字会拼出语法正确的坏 JSON（主站解析失败但报错指向别处），
     * 必须在这里拒绝，而不是转义了事。 */
    const char* evil = "A\",\"v\":999,\"x\":\"";
    assert_int_not_equal(
        ctx->driver->req_prepare(scratch, 1, 1, NULL, 0, evil, strlen(evil) + 1, true, 1.0), 0);

    /* 反斜杠同样拒绝 */
    const char* bs = "A\\B";
    assert_int_not_equal(
        ctx->driver->req_prepare(scratch, 1, 1, NULL, 0, bs, strlen(bs) + 1, true, 1.0), 0);

    driver_instance_destroy(ctx, "MqttJson");
    driver_manager_destroy(mgr);
}

static void test_birth_once_per_session(void** state) {
    (void)state;
    driver_manager_t* mgr = driver_manager_create();
    mqtt_json_driver_register(mgr);
    edge_driver_ctx_t* ctx = driver_instance_create(mgr, "MqttJson", 1);
    assert_non_null(ctx);
    ctx->pool = net_buf_pool_new(8, 512, 0);

    net_buf_t* tx = NULL;
    assert_int_equal(ctx->driver->req_tick(ctx->priv, NULL, NULL, NULL, NULL, ctx->pool, &tx), 0);
    assert_non_null(tx);

    char text[128];
    buf_text(tx, text, sizeof(text));
    assert_non_null(strstr(text, "\"online\":true"));
    net_buf_unref(tx);
    tx = NULL;

    /* 同一会话内不再发 */
    assert_int_not_equal(ctx->driver->req_tick(ctx->priv, NULL, NULL, NULL, NULL, ctx->pool, &tx),
                         0);
    assert_null(tx);

    /* 会话重建后必须重发 —— 否则主站认为节点没上线 */
    assert_non_null(ctx->driver->req_session_reset);
    assert_int_equal(ctx->driver->req_session_reset(ctx->priv), 0);
    assert_int_equal(ctx->driver->req_tick(ctx->priv, NULL, NULL, NULL, NULL, ctx->pool, &tx), 0);
    assert_non_null(tx);
    net_buf_unref(tx);

    net_buf_pool_unref(ctx->pool);
    driver_instance_destroy(ctx, "MqttJson");
    driver_manager_destroy(mgr);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_encode_ndata),
        cmocka_unit_test(test_encode_keeps_precision),
        cmocka_unit_test(test_rejects_unsafe_name),
        cmocka_unit_test(test_birth_once_per_session),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
