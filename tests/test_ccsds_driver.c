#include <arpa/inet.h>
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
#include "edge_slave_plane.h"

/* [ADR-002] 北向生产者端口：测试扮演"组合根/消费方"，从本地镜像取状态。
 * 驱动不再自己 read_shadow —— 这正是这次改造要验的东西。 */
static int _test_north_peek(void* ctx, uint32_t tag_id, pipe_msg_t* out) {
    return datahub_read_shadow((datahub_t*)ctx, tag_id, out);
}

extern int ccsds_driver_register(driver_manager_t* mgr);

static void test_ccsds_packet_format(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();

    datahub_t hub;
    datahub_init(&hub, loop, "test_ccsds", 64, 10000, NULL, 0, NULL, NULL, NULL);

    driver_manager_t* mgr = driver_manager_create();
    ccsds_driver_register(mgr);

    // Node ID 1024 (APID)
    edge_driver_ctx_t* ctx = driver_instance_create(mgr, "CCSDS", 1024);
    assert_non_null(ctx);
    ctx->user_data = &hub;
    ctx->pool = net_buf_pool_new(10, 2048, 0);

    // 1. Simulate Telemetry in DataHub (Tag 200, Scalar)
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 200, 3.14159, uv_hrtime());
    datahub_publish(&hub, &msg);

    // 2. Trigger driver tick to formulate CCSDS packet
    net_buf_t* out_tx = NULL;
    static edge_north_source_ops_t s_src = {.peek = _test_north_peek};
    int rc = ctx->driver->req_tick(ctx->priv, &s_src, &hub, NULL, NULL, ctx->pool, &out_tx);

    assert_int_equal(rc, 0);
    assert_non_null(out_tx);

    // 3. Verify CCSDS Primary Header
    uint8_t* packet = out_tx->b.data;
    size_t packet_len = out_tx->b.len;

    // Header should be 6 bytes
    // Data is 8 bytes (double)
    assert_int_equal(packet_len, 6 + 8);

    uint16_t stream_id = ntohs(*(uint16_t*)&packet[0]);
    uint16_t seq = ntohs(*(uint16_t*)&packet[2]);
    uint16_t len = ntohs(*(uint16_t*)&packet[4]);

    // Check Stream ID
    // Version = 000, Type = 0, SecHdr = 0, APID = 1024 (0x400)
    // 0000 0100 0000 0000 = 0x0400
    assert_int_equal(stream_id, 0x0400);

    // Check Sequence
    // SeqFlags = 11 (3) = 0xC000, SeqCount = 0
    // 1100 0000 0000 0000 = 0xC000
    assert_int_equal(seq, 0xC000);

    // Check Length
    // Length = 8 bytes - 1 = 7
    assert_int_equal(len, 7);

    // Cleanup
    if (out_tx) net_buf_unref(out_tx);
    net_buf_pool_unref(ctx->pool);
    driver_instance_destroy(ctx, "CCSDS");
    driver_manager_destroy(mgr);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ccsds_packet_format),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
