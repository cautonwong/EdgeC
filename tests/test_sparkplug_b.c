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

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_spb_encoding),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
