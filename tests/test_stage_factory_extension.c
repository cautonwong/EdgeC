#include <cjson/cJSON.h>
#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"
#include "datahub_priv.h"
#include "stream_bus_datahub_adapter.h"
#include "stream_engine.h"

typedef struct {
    int seen;
    double value;
} capture_t;

static capture_t g_capture;

static stage_ret_t scale_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    double *factor = (double *)stage->priv_state;
    if (ev->type == PIPEMSG_TYPE_SCALAR) ev->payload.val.f *= *factor;
    return STAGE_OK;
}

static pipeline_stage_t *scale_factory(const char *name, void *config_json,
                                       pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    cJSON *config = (cJSON *)config_json;
    cJSON *factor_item = config ? cJSON_GetObjectItem(config, "factor") : NULL;
    pipeline_stage_t *stage = stream_stage_alloc_node();
    double *factor = (double *)stage->storage_state;
    *factor = factor_item ? factor_item->valuedouble : 1.0;
    snprintf(stage->name, sizeof(stage->name), "%s", name ? name : "test_scale");
    stage->process = scale_proc;
    stage->priv_state = factor;
    return stage;
}

static stage_ret_t capture_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    (void)stage;
    if (ev->type == PIPEMSG_TYPE_SCALAR) {
        g_capture.value = ev->payload.val.f;
        g_capture.seen++;
    }
    return STAGE_OK;
}

static pipeline_stage_t *capture_factory(const char *name, void *config_json,
                                         pipeline_loader_ctx_t *ctx) {
    (void)config_json;
    (void)ctx;
    pipeline_stage_t *stage = stream_stage_alloc_node();
    snprintf(stage->name, sizeof(stage->name), "%s", name ? name : "test_capture");
    stage->process = capture_proc;
    return stage;
}

static void test_custom_stage_from_json(void** state) {
    (void)state;
    char path[] = "/tmp/edge_stage_factory_XXXXXX";
    int fd = mkstemp(path);
    assert_true(fd >= 0);
    const char *json =
        "{\"pipelines\":[{\"id\":\"custom\",\"trigger_tag\":1001,\"stages\":["
        "{\"type\":\"test_scale\",\"config\":{\"factor\":3.0}},"
        "{\"type\":\"test_capture\"}]}]}";
    assert_int_equal(write(fd, json, strlen(json)), (ssize_t)strlen(json));
    close(fd);

    assert_int_equal(stream_engine_register_stage_factory("test_scale", scale_factory), 0);
    assert_int_equal(stream_engine_register_stage_factory("test_capture", capture_factory), 0);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    datahub_t hub;
    assert_int_equal(datahub_init(&hub, &loop, "test_stage_factory", 64, 1024, NULL, 0, NULL, NULL,
                                  NULL),
                     0);
    stream_engine_t *engine = stream_engine_create_datahub(&hub, &loop, NULL, NULL);
    assert_non_null(engine);
    stream_engine_start(engine);

    memset(&g_capture, 0, sizeof(g_capture));
    assert_int_equal(stream_engine_load_config(engine, path, NULL), 0);
    pipe_msg_t msg = {0};
    msg.type = PIPEMSG_TYPE_SCALAR;
    msg.tag_id = 1001;
    msg.payload.val.f = 2.0;
    stream_engine_push(engine, &msg);

    assert_int_equal(g_capture.seen, 1);
    assert_true(g_capture.value == 6.0);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
    while (uv_run(&loop, UV_RUN_NOWAIT)) {
    }
    uv_loop_close(&loop);
    unlink(path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_custom_stage_from_json),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
