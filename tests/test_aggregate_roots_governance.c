/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
/**
 * @file test_aggregate_roots_governance.c
 * @brief 验证 Issue #369 与 Issue #370 聚合根封装与防跨层穿透治理测试
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "datahub.h"
#include "stream_engine.h"

static int g_cleanup_called = 0;

static stage_ret_t dummy_stage_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    (void)stage;
    (void)ev;
    return STAGE_OK;
}

static void dummy_cleanup_state(void *priv_data) {
    (void)priv_data;
    g_cleanup_called++;
}

static void test_datahub_aggregate_root(void) {
    printf("[TEST] Testing datahub_t aggregate root encapsulation (Issue #369)...\n");
    uv_loop_t loop;
    uv_loop_init(&loop);

    datahub_t hub;
    int rc = datahub_init(&hub, &loop, "test_hub_agg", 256, 0, NULL, 0, NULL, NULL, NULL);
    assert(rc == 0);

    // 1. Verify get_ring_fd
    int ring_fd = datahub_get_ring_fd(&hub);
    assert(ring_fd >= 0);

    // 2. Verify listener_count
    assert(datahub_get_listener_count(&hub) == 0);

    // 3. Verify bind_io_pool
    assert(datahub_bind_io_pool(&hub, NULL) == 0);

    // 4. Verify write_ring
    pipe_msg_t wmsg = {0};
    wmsg.tag_id = 9999;
    wmsg.type = PIPEMSG_TYPE_SCALAR;
    wmsg.payload.val.f = 123.456;
    rc = datahub_write_ring(&hub, &wmsg);
    assert(rc == 0);

    // 5. Verify total_writes and seq
    assert(datahub_get_ring_total_writes(&hub) >= 1);
    assert(datahub_get_ring_seq(&hub) >= 1);

    // 6. Verify read_ring
    uint64_t read_seq = 0;
    pipe_msg_t rmsg = {0};
    rc = datahub_read_ring(&hub, &read_seq, &rmsg);
    assert(rc == 0);
    assert(rmsg.tag_id == 9999);
    assert(rmsg.payload.val.f == 123.456);

    datahub_destroy(&hub);
    uv_loop_close(&loop);
    printf("  -> PASS: datahub_t encapsulation verified.\n");
}

static void test_stream_engine_aggregate_root(void) {
    printf("[TEST] Testing stream_engine_t aggregate root stage management (Issue #370)...\n");
    uv_loop_t loop;
    uv_loop_init(&loop);

    datahub_t hub;
    datahub_init(&hub, &loop, "test_se_hub", 256, 0, NULL, 0, NULL, NULL, NULL);

    stream_engine_storage_t engine_storage;
    stream_engine_t *engine = stream_engine_create_in(&engine_storage, &hub, &loop, NULL, NULL);
    assert(engine != NULL);

    // 1. Test trigger setting for new pipeline
    int rc = stream_engine_set_pipeline_trigger(engine, "pipe_wasm_dynamic", 8001);
    assert(rc == 0);

    // 2. Test replace stage with new stage
    g_cleanup_called = 0;
    pipeline_stage_t st1;
    memset(&st1, 0, sizeof(st1));
    strncpy(st1.name, "wasm_stage_v1", sizeof(st1.name) - 1);
    st1.process = dummy_stage_proc;
    st1.cleanup_state = dummy_cleanup_state;

    rc = stream_engine_replace_pipeline_stage(engine, "pipe_wasm_dynamic", &st1);
    assert(rc == 0);
    assert(g_cleanup_called == 0);

    // 3. Test atomic replacement with v2 (should call cleanup for v1)
    pipeline_stage_t st2;
    memset(&st2, 0, sizeof(st2));
    strncpy(st2.name, "wasm_stage_v2", sizeof(st2.name) - 1);
    st2.process = dummy_stage_proc;
    st2.cleanup_state = dummy_cleanup_state;

    stream_engine_set_pipeline_trigger(engine, "pipe_wasm_dynamic", 8002);
    rc = stream_engine_replace_pipeline_stage(engine, "pipe_wasm_dynamic", &st2);
    assert(rc == 0);
    assert(g_cleanup_called == 1);

    // 4. Test clear pipeline stages
    rc = stream_engine_clear_pipeline_stages(engine, "pipe_wasm_dynamic");
    assert(rc == 0);
    assert(g_cleanup_called == 2);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
    uv_loop_close(&loop);
    printf("  -> PASS: stream_engine_t stage management encapsulation verified.\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("  Running Aggregate Roots Governance Tests (Issue #369, #370)   \n");
    printf("=================================================================\n");

    test_datahub_aggregate_root();
    test_stream_engine_aggregate_root();

    printf("\nAll Aggregate Roots Governance tests PASSED!\n");
    return 0;
}
