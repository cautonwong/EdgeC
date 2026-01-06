#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"

// --- Global Test State ---
typedef struct {
    edge_driver_ctx_t *ctx;
    net_buf_pool_t pool;
} test_state_t;

uint32_t edge_lookup_dlt645_num(uint32_t di, float *out_scale) {
    if (di == 0x00010000) {
        *out_scale = 0.01f;
        return 1001;
    }
    return 0;
}

extern int dlt645_driver_register(void);

static int group_setup(void **state) {
    (void)state;
    driver_manager_init();
    dlt645_driver_register();
    return 0;
}

static int group_teardown(void **state) {
    (void)state;
    driver_manager_fini();
    return 0;
}

static int test_setup(void **state) {
    test_state_t *ts = calloc(1, sizeof(test_state_t));
    ts->ctx = driver_instance_create("DLT645-2007", 123456789);
    net_buf_pool_init(&ts->pool, 8, 512, 0, NULL, NULL);
    *state = ts;
    return 0;
}

static int test_teardown(void **state) {
    test_state_t *ts = (test_state_t *)*state;
    driver_instance_destroy(ts->ctx, "DLT645-2007");
    net_buf_pool_destroy(&ts->pool);
    free(ts);
    return 0;
}

static void test_dlt645_v3_cycle(void **state) {
    test_state_t *ts = (test_state_t *)*state;
    edge_driver_t *drv = driver_manager_find("DLT645-2007");

    uint32_t tags[] = {0x00010000};
    net_buf_t *tx = net_buf_alloc(&ts->pool, 0);
    drv->encode_request(ts->ctx, tags, 1, tx);
    assert_int_equal(tx->b.data[0], 0x68);

    uint8_t rx_raw[] = {0x68, 0x89, 0x67, 0x45, 0x23, 0x01, 0x00, 0x68, 0x91, 0x08,
                        0x33, 0x33, 0x34, 0x33, 0x68, 0x39, 0x33, 0x33, 0x00, 0x16};
    uint8_t cs = 0;
    for (int i = 0; i < 18; i++) cs += rx_raw[i];
    rx_raw[18] = cs;

    net_buf_t *rx = net_buf_alloc(&ts->pool, 0);
    net_buf_simple_add_mem(&rx->b, rx_raw, sizeof(rx_raw));

    pipe_msg_t msgs[4];
    int count = 0;
    drv->decode_response(ts->ctx, rx, msgs, &count);
    assert_int_equal(count, 1);
    assert_double_equal(msgs[0].payload.val.f, 6.35, 0.001);

    net_buf_unref(tx);
    net_buf_unref(rx);
}

static void test_dlt645_v3_write_cycle(void **state) {
    test_state_t *ts = (test_state_t *)*state;
    edge_driver_t *drv = driver_manager_find("DLT645-2007");

    uint32_t write_tags[] = {0x02010100, 1234};
    net_buf_t *req = net_buf_alloc(&ts->pool, 0);
    drv->encode_request(ts->ctx, write_tags, 2, req);

    // 验证产生了一个合法的 DLT645 帧 (以 0x68 开头)
    assert_int_equal(req->b.data[0], 0x68);
    assert_true(req->b.len >= 12);

    net_buf_t *resp = net_buf_alloc(&ts->pool, 0);
    drv->handle_rx(ts->ctx, req, resp, NULL, NULL);

    net_buf_unref(req);
    net_buf_unref(resp);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_dlt645_v3_cycle, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dlt645_v3_write_cycle, test_setup, test_teardown),
    };
    return cmocka_run_group_tests(tests, group_setup, group_teardown);
}