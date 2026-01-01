#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "contracts.h"
#include "sqlite_service.h"
#include "stream_engine.h"

static void create_test_config(const char *path) {
    FILE *f = fopen(path, "w");
    fprintf(f,
            "{\n"
            "  \"pipelines\": [\n"
            "    {\n"
            "      \"id\": \"test_json_pipe\",\n"
            "      \"trigger_tag\": %d,\n"
            "      \"stages\": [\n"
            "        { \"type\": \"accumulate\", \"config\": {} },\n"
            "        { \"type\": \"sqlite_sink\", \"config\": { \"table\": \"test_table\" } }\n"
            "      ]\n"
            "    }\n"
            "  ]\n"
            "}",
            TAG_GRID_POWER);
    fclose(f);
}

static void test_loader_dynamic_build(void **state) {
    (void)state;
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);

    datahub_t hub;
    datahub_init(&hub, loop, "loader_bus_v4", 64);

    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = ":memory:"};
    sqlite_svc_t *db = sqlite_svc_create(loop, &db_cfg);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);

    const char *config_path = "tmp_pipeline.json";
    create_test_config(config_path);

    pipeline_loader_ctx_t loader_ctx = {.db = db, .js_engine = NULL, .params = NULL};
    int ret = stream_engine_load_config(engine, config_path, &loader_ctx);
    assert_int_equal(ret, 0);

    pipe_msg_t ev = {0};
    ev.type = PIPEMSG_TYPE_SCALAR;
    ev.tag_id = TAG_GRID_POWER;
    ev.payload.val.f = 100.0;
    stream_engine_push(engine, &ev);

    unlink(config_path);
    stream_engine_destroy(engine);
    sqlite_svc_destroy(db);
    datahub_destroy(&hub);

    while (uv_loop_close(loop) == UV_EBUSY) {
        uv_run(loop, UV_RUN_ONCE);
    }
    free(loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_loader_dynamic_build),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
