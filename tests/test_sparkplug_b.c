#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "sparkplug_b.h"
#include "datahub_priv.h"

// Forward declaration
int sparkplug_b_driver_register(driver_manager_t* mgr);

static void test_spb_encoding(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "test_spb", 64, 128, NULL, 0, NULL, NULL, NULL);

    driver_manager_t* mgr = driver_manager_create();
    sparkplug_b_driver_register(mgr);

    edge_driver_ctx_t* ctx = driver_instance_create(mgr, "SparkplugB", 1);
    assert_non_null(ctx);
    ctx->user_data = &hub;
    ctx->pool = net_buf_pool_new(10, 1024, 0);

    // Test NDATA encoding
    net_buf_t* nb = net_buf_alloc(ctx->pool, 0);
    assert_non_null(nb);

    /* [ADR-002] 北向生产者走 native 槽位：metric 名由端口经 point_desc 传进来，
     * 驱动不再反查 datahub 元数据。 */
    alignas(max_align_t) uint8_t scratch[REQ_DRV_PRIV_MAX];
    memset(scratch, 0, sizeof(scratch));
    const char* metric = "voltage_a";
    assert_int_equal(ctx->driver->req_prepare(scratch, 1, 1001, NULL, 0, metric,
                                              strlen(metric) + 1, true, 23.5),
                     0);
    int rc = ctx->driver->req_encode(scratch, ctx->priv, nb);
    assert_int_equal(rc, 0);
    assert_true(nb->b.len > 0);

    /* metric 名必须出现在报文里（证明名字来自调用方，而不是驱动自己去查） */
    bool name_found = false;
    for (size_t i = 0; i + strlen(metric) <= nb->b.len; i++) {
        if (memcmp(nb->b.data + i, metric, strlen(metric)) == 0) {
            name_found = true;
            break;
        }
    }
    assert_true(name_found);

    printf("Encoded SPB NDATA (%u bytes): ", nb->b.len);
    for (size_t i = 0; i < nb->b.len; i++) printf("%02X ", nb->b.data[i]);
    printf("\n");

    // Verify Protobuf structure manually (Tracer Bullet)
    // First byte should be field 1 (timestamp) tag: (1<<3)|0 = 0x08
    assert_int_equal(nb->b.data[0], 0x08);

    net_buf_unref(nb);
    net_buf_pool_unref(ctx->pool);
    driver_instance_destroy(ctx, "SparkplugB");
    driver_manager_destroy(mgr);
    datahub_destroy(&hub);
}

/**
 * 重连之后必须重发 NBIRTH（ADR-002 北向 N1-1）。
 *
 * `req_tick` 用 `priv->birth_sent` 守住"每会话只发一次 BIRTH"。但 `priv` 是
 * **持久**上下文，重连不会让它复位 —— 结果就是主站眼里这个节点已经死了，
 * 之后发多少 NDATA 都不认。
 *
 * `req_session_reset` 就是传输层报告"新会话"时调用的复位入口。
 */
static void test_spb_session_reset_resends_birth(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "test_spb_birth", 64, 128, NULL, 0, NULL, NULL, NULL);

    driver_manager_t* mgr = driver_manager_create();
    sparkplug_b_driver_register(mgr);

    edge_driver_ctx_t* ctx = driver_instance_create(mgr, "SparkplugB", 1);
    assert_non_null(ctx);
    ctx->user_data = &hub;
    ctx->pool = net_buf_pool_new(10, 1024, 0);

    /* 第一次会话：tick 发出 NBIRTH */
    net_buf_t* tx = NULL;
    assert_int_equal(ctx->driver->req_tick(ctx->priv, NULL, NULL, NULL, NULL, ctx->pool, &tx), 0);
    assert_non_null(tx);
    net_buf_unref(tx);
    tx = NULL;

    /* 同一会话内再 tick：不该再发（协议要求每会话一次） */
    assert_int_not_equal(
        ctx->driver->req_tick(ctx->priv, NULL, NULL, NULL, NULL, ctx->pool, &tx), 0);
    assert_null(tx);

    /* 传输层报告会话重建 —— 驱动必须能重新发 BIRTH */
    assert_non_null(ctx->driver->req_session_reset);
    assert_int_equal(ctx->driver->req_session_reset(ctx->priv), 0);

    assert_int_equal(ctx->driver->req_tick(ctx->priv, NULL, NULL, NULL, NULL, ctx->pool, &tx), 0);
    assert_non_null(tx);
    net_buf_unref(tx);

    net_buf_pool_unref(ctx->pool);
    driver_instance_destroy(ctx, "SparkplugB");
    driver_manager_destroy(mgr);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_spb_encoding),
        cmocka_unit_test(test_spb_session_reset_resends_birth),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
