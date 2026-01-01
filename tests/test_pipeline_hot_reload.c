#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "stream_engine.h"

// Private header for clearing
#include "../src/stream_engine/stream_engine_priv.h"

int stream_engine_push_event(stream_engine_t *engine, pipe_msg_t *ev);
static void stream_engine_clear_pipelines(stream_engine_t *engine);

// Mock probe
typedef struct {
    int hit_count;
} probe_state_t;

static stage_ret_t probe_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    probe_state_t *s = (probe_state_t *)stage->priv_state;
    s->hit_count++;
    return STAGE_OK;
}

// Copy of static function from stream_engine.c for testing purposes
static void stream_engine_clear_pipelines(stream_engine_t *engine) {
    for (int i = 0; i < engine->pipe_count; i++) {
        pipeline_t *p = engine->pipelines[i];
        pipeline_stage_t *curr = p->head;
        while (curr) {
            pipeline_stage_t *next = curr->next;
            if (curr->cleanup_state) curr->cleanup_state(curr->priv_state);
            if (curr->cleanup_cfg) curr->cleanup_cfg(curr->priv_cfg);
            free(curr);
            curr = next;
        }
        free(p);
    }
    engine->pipe_count = 0;
}

static void test_pipeline_hot_reload_logic(void **state) {
    (void)state;
    datahub_t *hub = calloc(1, sizeof(datahub_t));
    stream_engine_t *engine = stream_engine_create(hub, NULL);

    // 1. Load initial config
    stream_engine_add_pipeline(engine, "pipe_v1", 1001);
    probe_state_t probe = {0};
    pipeline_add_stage(engine->pipelines[0], "probe", probe_proc, &probe, NULL, NULL, NULL);

    pipe_msg_t ev1 = {0};
    ev1.type = PIPEMSG_TYPE_SCALAR;
    ev1.tag_id = 1001;
    ev1.payload.val.f = 1.0;

    stream_engine_push_event(engine, &ev1);
    assert_int_equal(probe.hit_count, 1);

    // 2. Reload (Clear + Load New)
    stream_engine_clear_pipelines(engine);
    assert_int_equal(engine->pipe_count, 0);

    // 3. Load new config
    stream_engine_add_pipeline(engine, "pipe_v2", 2002);

    pipe_msg_t ev_old = {0};
    ev_old.type = PIPEMSG_TYPE_SCALAR;
    ev_old.tag_id = 1001;

    stream_engine_push_event(engine, &ev_old);

    pipe_msg_t ev_new = {0};
    ev_new.type = PIPEMSG_TYPE_SCALAR;
    ev_new.tag_id = 2002;

    stream_engine_push_event(engine, &ev_new);

    stream_engine_destroy(engine);
    free(hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pipeline_hot_reload_logic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
