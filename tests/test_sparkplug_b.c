#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sparkplug_b.h"
#include "edge_driver.h"
#include "driver_manager.h"
#include "datahub.h"

// Forward declaration
int sparkplug_b_driver_register(driver_manager_t *mgr);

static void test_spb_encoding(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "test_spb", 64, 128, NULL, 0, NULL, NULL, NULL);

    driver_manager_t *mgr = driver_manager_create();
    sparkplug_b_driver_register(mgr);

    edge_driver_ctx_t *ctx = driver_instance_create(mgr, "SparkplugB", 1);
    assert_non_null(ctx);
    ctx->user_data = &hub;
    ctx->pool = net_buf_pool_new(10, 1024, 0);

    // Test NDATA encoding
    net_buf_t *nb = net_buf_alloc(ctx->pool, 0);
    assert_non_null(nb);

    // Tag 1001, Value 23.5
    int rc = ctx->driver->encode_write(ctx, 1001, 23.5, nb);
    assert_int_equal(rc, 0);
    assert_true(nb->b.len > 0);

    printf("Encoded SPB NDATA (%u bytes): ", nb->b.len);
    for(size_t i=0; i<nb->b.len; i++) printf("%02X ", nb->b.data[i]);
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
