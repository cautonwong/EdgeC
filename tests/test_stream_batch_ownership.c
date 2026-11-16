#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "datahub.h"
#include "datahub_priv.h"
#include "stream_bus_datahub_adapter.h"
#include "stream_bus_port.h"
#include "stream_engine.h"

pipeline_stage_t *stage_compactor_create(uint32_t target_tag, uint32_t max_elements,
                                         uint32_t output_tag);

typedef struct {
    int batches;
    int total;
    int counts[4];
    double values[32];
    void *ptrs[4];
} capture_t;

static stage_ret_t capture_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    capture_t *cap = (capture_t *)stage->priv_state;
    if (ev->type != PIPEMSG_TYPE_BATCH || cap->batches >= 4) return STAGE_DROP;
    int index = cap->batches;
    cap->counts[index] = ev->payload.batch.count;
    cap->ptrs[index] = ev->payload.batch.ptr;
    const double *data = ev->payload.batch.ptr;
    for (int i = 0; i < ev->payload.batch.count; i++) cap->values[cap->total + i] = data[i];
    cap->total += ev->payload.batch.count;
    cap->batches++;
    return STAGE_OK;
}

static stage_ret_t async_batch_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    (void)stage;
    return ev->type == PIPEMSG_TYPE_BATCH ? STAGE_ASYNC : STAGE_OK;
}

static int g_publish_calls;

static int count_publish(void *ctx, const pipe_msg_t *msg) {
    (void)ctx;
    (void)msg;
    g_publish_calls++;
    return 0;
}

static void test_compactor_reuses_batch_buffer(void** state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    datahub_t hub;
    assert_int_equal(datahub_init(&hub, &loop, "test_batch_owner", 64, 1024, NULL, 0, NULL, NULL,
                                  NULL),
                     0);
    stream_engine_t *engine = stream_engine_create_datahub(&hub, &loop, NULL, NULL);
    assert_non_null(engine);
    stream_engine_start(engine);

    pipeline_t *producer = stream_engine_add_pipeline(engine, "compactor", 1001);
    pipeline_stage_t *compactor = stage_compactor_create(1001, 4, 5001);
    assert_non_null(compactor);
    assert_non_null(pipeline_add_stage_node(producer, compactor));

    capture_t cap = {0};
    pipeline_t *consumer = stream_engine_add_pipeline(engine, "capture", 5001);
    assert_non_null(pipeline_add_stage(consumer, "capture", capture_proc, &cap, NULL, NULL, NULL));

    for (int i = 0; i < 8; i++) {
        pipe_msg_t msg = {0};
        msg.type = PIPEMSG_TYPE_SCALAR;
        msg.tag_id = 1001;
        msg.payload.val.f = (double)(i + 1);
        stream_engine_push(engine, &msg);
    }

    assert_int_equal(cap.batches, 2);
    assert_int_equal(cap.counts[0], 4);
    assert_int_equal(cap.counts[1], 4);
    for (int i = 0; i < 8; i++) assert_true(cap.values[i] == (double)(i + 1));
    assert_true(cap.ptrs[0] == cap.ptrs[1]);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
    while (uv_run(&loop, UV_RUN_NOWAIT)) {
    }
    uv_loop_close(&loop);
}

static void test_bus_rejects_batch(void** state) {
    (void)state;
    g_publish_calls = 0;
    stream_bus_port_t bus = {.publish = count_publish};

    pipe_msg_t batch = {0};
    batch.type = PIPEMSG_TYPE_BATCH;
    assert_int_equal(stream_bus_publish(&bus, &batch), -1);
    assert_int_equal(g_publish_calls, 0);

    pipe_msg_t scalar = {0};
    scalar.type = PIPEMSG_TYPE_SCALAR;
    assert_int_equal(stream_bus_publish(&bus, &scalar), 0);
    assert_int_equal(g_publish_calls, 1);
}

static void test_async_batch_is_rejected(void** state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    datahub_t hub;
    assert_int_equal(datahub_init(&hub, &loop, "test_batch_async", 64, 1024, NULL, 0, NULL, NULL,
                                  NULL),
                     0);
    stream_engine_t *engine = stream_engine_create_datahub(&hub, &loop, NULL, NULL);
    assert_non_null(engine);
    stream_engine_start(engine);

    capture_t cap = {0};
    pipeline_t *pipeline = stream_engine_add_pipeline(engine, "async_batch", 6001);
    assert_non_null(pipeline_add_stage(pipeline, "async", async_batch_proc, NULL, NULL, NULL, NULL));
    assert_non_null(pipeline_add_stage(pipeline, "capture", capture_proc, &cap, NULL, NULL, NULL));

    double data[4] = {1.0, 2.0, 3.0, 4.0};
    pipe_msg_t batch = {0};
    batch.type = PIPEMSG_TYPE_BATCH;
    batch.tag_id = 6001;
    batch.payload.batch.ptr = data;
    batch.payload.batch.count = 4;
    stream_engine_push(engine, &batch);
    stream_engine_resume(pipeline);

    assert_int_equal(cap.batches, 0);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
    while (uv_run(&loop, UV_RUN_NOWAIT)) {
    }
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_compactor_reuses_batch_buffer),
        cmocka_unit_test(test_bus_rejects_batch),
        cmocka_unit_test(test_async_batch_is_rejected),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
