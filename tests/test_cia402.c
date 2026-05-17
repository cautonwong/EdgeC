#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cia402.h"
#include "edge_driver.h"
#include "driver_manager.h"
#include "datahub.h"
#include "can_codec.h"

// Forward declaration
int cia402_driver_register(driver_manager_t *mgr);

static void test_cia402_state_machine(void **state) {
    (void)state;
    
    // Test Statusword parsing
    assert_int_equal(cia402_get_state(0x0000), CIA402_STATE_NOT_READY_TO_SWITCH_ON);
    assert_int_equal(cia402_get_state(0x0040), CIA402_STATE_SWITCH_ON_DISABLED);
    assert_int_equal(cia402_get_state(0x0021), CIA402_STATE_READY_TO_SWITCH_ON);
    assert_int_equal(cia402_get_state(0x0023), CIA402_STATE_SWITCHED_ON);
    assert_int_equal(cia402_get_state(0x0027), CIA402_STATE_OPERATION_ENABLED);
    assert_int_equal(cia402_get_state(0x0007), CIA402_STATE_QUICK_STOP_ACTIVE);
    assert_int_equal(cia402_get_state(0x000F), CIA402_STATE_FAULT_REACTION_ACTIVE);
    assert_int_equal(cia402_get_state(0x0008), CIA402_STATE_FAULT);
}

static void test_cia402_pdo_flow(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    // Capacity 8192 to support tags 6001-6004
    datahub_init(&hub, loop, "test_cia", 64, 8192, NULL, 0, NULL, NULL, NULL);

    driver_manager_t *mgr = driver_manager_create();
    cia402_driver_register(mgr);

    // Node ID 5
    edge_driver_ctx_t *ctx = driver_instance_create(mgr, "CiA402", 5);
    assert_non_null(ctx);
    ctx->user_data = &hub;
    ctx->pool = net_buf_pool_new(10, 1024, 0);

    // 1. Simulate RX of RPDO1 (Statusword from Servo)
    // COB-ID = 0x180 + 5 = 0x185
    edge_can_frame_t frame_in = {
        .can_id = 0x185,
        .can_dlc = 2,
        .data = {0x27, 0x00} // Statusword 0x0027 (Operation Enabled)
    };
    net_buf_t *in = net_buf_alloc(ctx->pool, 0);
    can_encode_frame(&frame_in, in);
    
    pipe_msg_t msgs[4];
    int msg_count = 0;
    size_t consumed = 0;
    int rc = ctx->driver->handle_rx(ctx, in, NULL, msgs, &msg_count, &consumed);
    assert_int_equal(rc, 0);
    assert_int_equal(msg_count, 1);
    assert_int_equal(msgs[0].tag_id, 6001); // tag_statusword default
    assert_int_equal((int)msgs[0].payload.val.f, 0x0027);

    // 2. Simulate DataHub update for Target Position (tag 6004)
    pipe_msg_t pos_msg;
    pipe_msg_init_scalar(&pos_msg, 6004, 123456.0, uv_hrtime());
    datahub_publish(&hub, &pos_msg);
    
    // Process many turns to ensure sync
    for(int i=0; i<10; i++) uv_run(loop, UV_RUN_NOWAIT);

    // 3. Trigger on_tick to generate TPDO2 (Target Position to Servo)
    net_buf_t *out_tx = NULL;
    rc = ctx->driver->on_tick(ctx, &out_tx);
    assert_int_equal(rc, 0);
    assert_non_null(out_tx);
    
    edge_can_frame_t frame_out;
    can_decode_frame(&out_tx->b, &frame_out);
    assert_int_equal(frame_out.can_id, 0x305); // TPDO2: 0x300 + 5
    assert_int_equal(frame_out.can_dlc, 4);
    
    int32_t val_out = 0;
    memcpy(&val_out, frame_out.data, 4);
    assert_int_equal(val_out, 123456);

    // Cleanup
    if (out_tx) net_buf_unref(out_tx);
    net_buf_unref(in);
    net_buf_pool_unref(ctx->pool);
    driver_instance_destroy(ctx, "CiA402");
    driver_manager_destroy(mgr);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_cia402_state_machine),
        cmocka_unit_test(test_cia402_pdo_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
