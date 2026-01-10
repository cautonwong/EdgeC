#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../src/stream_engine/stream_engine_priv.h"
#include "datahub.h"
#include "stream_engine.h"

typedef struct {
    int hit_count;
} probe_state_t;
static stage_ret_t probe_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    probe_state_t *s = (probe_state_t *)stage->priv_state;
    s->hit_count++;
    return STAGE_OK;
}

static void test_pipeline_hot_reload_logic(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "hot_reload_bus", 64, 1024, NULL, 0, NULL, NULL);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    pipeline_t *p1 = stream_engine_add_pipeline(engine, "pipe_v1", 1001);
    probe_state_t probe = {0};
    pipeline_add_stage(p1, "probe", probe_proc, &probe, NULL, NULL, NULL);

    pipe_msg_t ev1 = {0};
    ev1.type = PIPEMSG_TYPE_SCALAR;
    ev1.tag_id = 1001;
    ev1.payload.val.f = 1.0;
    stream_engine_push(engine, &ev1);
    assert_int_equal(probe.hit_count, 1);

    stream_engine_destroy(engine);

    // 重新创建引擎模拟热重载
    engine = stream_engine_create(&hub, NULL);
    stream_engine_add_pipeline(engine, "pipe_v2", 2002);

    pipe_msg_t ev_new = {0};
    ev_new.type = PIPEMSG_TYPE_SCALAR;
    ev_new.tag_id = 2002;
    stream_engine_push(engine, &ev_new);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);

    while (uv_loop_close(loop) == UV_EBUSY) {
        uv_run(loop, UV_RUN_ONCE);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pipeline_hot_reload_logic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}