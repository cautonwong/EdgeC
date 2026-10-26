#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver_manager.h"
#include "net_buf.h"
#include "shmring.h"

extern edge_driver_t dlt645_driver;

typedef struct {
    driver_manager_t* mgr;
    edge_driver_ctx_t* ctx;
    net_buf_pool_t pool;
} test_state_t;

static int group_setup(void** state) {
    test_state_t* ts = calloc(1, sizeof(test_state_t));
    ts->mgr = driver_manager_create();
    driver_manager_register(ts->mgr, &dlt645_driver);
    net_buf_pool_init(&ts->pool, 16, 512, 0, NULL, NULL);
    *state = ts;
    return 0;
}

static int group_teardown(void** state) {
    test_state_t* ts = *state;
    net_buf_pool_destroy(&ts->pool);
    driver_manager_destroy(ts->mgr);
    free(ts);
    return 0;
}

static int test_setup(void** state) {
    test_state_t* ts = *state;
    ts->ctx = driver_instance_create(ts->mgr, "DLT645", 123456789);
    return 0;
}

static int test_teardown(void** state) {
    test_state_t* ts = *state;
    driver_instance_destroy(ts->ctx, "DLT645");
    return 0;
}

static void test_dlt645_v3_cycle(void** state) {
    test_state_t* ts = *state;
    assert_non_null(ts->ctx);
}

/* 验收台架的真实应答帧：68 010000000000 68 91 06 33343435 4a56 d8 16
 * L=0x06 = DI(4) + 数据(2)，数据 4a 56 减 0x33 得 BCD "17""23" → 2317。
 * 驱动层只还原整数(scale 归 cmd 层)：断言 raw=2317、consumed=18、msg_count=1。
 * 这条是回归钉：此前按 L>=8 判定，L=6 的帧会被静默丢弃（msg_count=0 却返回 DONE）。 */
static void test_dlt645_decode_acceptance_frame(void** state) {
    test_state_t* ts = *state;
    const uint8_t frame[] = {0x68, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x91,
                             0x06, 0x33, 0x34, 0x34, 0x35, 0x4a, 0x56, 0xd8, 0x16};

    net_buf_t* in = net_buf_alloc(&ts->pool, 0);
    net_buf_t* resp = net_buf_alloc(&ts->pool, 0);
    net_buf_simple_add_mem(&in->b, frame, sizeof(frame));

    pipe_msg_t out[2] = {0};
    int msg_count = 0;
    size_t consumed = 0;
    int rc = dlt645_driver.decode_response(ts->ctx, in, resp, out, &msg_count, &consumed);

    assert_int_equal(rc, 0);
    assert_int_equal(consumed, 18);
    assert_int_equal(msg_count, 1);
    assert_int_equal(out[0].tag_id, 0x02010100);   /* 协议层原样回 DI，不换成 OAD */
    assert_true(out[0].payload.val.f == 2317.0);   /* 未乘 scale */
    assert_int_equal(out[0].device_id, ts->ctx->device_id);

    net_buf_unref(in);
    net_buf_unref(resp);
}

/* 四字节数据段（L=0x08，如组合有功）同样要认 */
static void test_dlt645_decode_four_byte_payload(void** state) {
    test_state_t* ts = *state;
    uint8_t frame[] = {0x68, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x91,
                       0x08, 0x33, 0x34, 0x34, 0x35, 0x4a, 0x56, 0x33, 0x33, 0x00, 0x16};
    uint8_t cs = 0;
    for (size_t i = 0; i < 18; i++) cs = (uint8_t)(cs + frame[i]);
    frame[18] = cs;

    net_buf_t* in = net_buf_alloc(&ts->pool, 0);
    net_buf_t* resp = net_buf_alloc(&ts->pool, 0);
    net_buf_simple_add_mem(&in->b, frame, sizeof(frame));

    pipe_msg_t out[2] = {0};
    int msg_count = 0;
    size_t consumed = 0;
    int rc = dlt645_driver.decode_response(ts->ctx, in, resp, out, &msg_count, &consumed);

    assert_int_equal(rc, 0);
    assert_int_equal(consumed, 20);
    assert_int_equal(msg_count, 1);
    assert_true(out[0].payload.val.f == 2317.0);

    net_buf_unref(in);
    net_buf_unref(resp);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_dlt645_v3_cycle, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dlt645_decode_acceptance_frame, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_dlt645_decode_four_byte_payload, test_setup, test_teardown),
    };
    return cmocka_run_group_tests(tests, group_setup, group_teardown);
}
