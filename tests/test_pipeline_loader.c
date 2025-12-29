#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "datahub.h"
#include "sqlite_service.h"
#include "stream_engine.h"

static void create_test_config(const char *path) {
    FILE *f = fopen(path, "w");
    fprintf(f,
            "{\n"
            "  \"pipelines\": [\n"
            "    {\n"
            "      \"id\": \"test_json_pipe\",\n"
            "      \"trigger_tag\": 1001,\n"
            "      \"stages\": [\n"
            "        { \"type\": \"accumulate\", \"config\": {} },\n"
            "        { \"type\": \"sqlite_sink\", \"config\": { \"table\": \"test_table\" } }\n"
            "      ]\n"
            "    }\n"
            "  ]\n"
            "}");
    fclose(f);
}

static void test_loader_dynamic_build(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();

    datahub_t hub;
    datahub_init(&hub, loop, "loader_bus_v3", 64, NULL, NULL);

    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = ":memory:"};
    sqlite_svc_t *db = sqlite_svc_create(loop, &db_cfg);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);

    const char *config_path = "tmp_pipeline.json";
    create_test_config(config_path);

    int ret = stream_engine_load_config(engine, config_path, db);
    assert_int_equal(ret, 0);

    pipe_event_t ev = {
        .type = EVT_DATA_POINT, .tag_id = 1001, .payload.val = 100.0, .calendar_ms = 123456};
    stream_engine_push_event(engine, &ev);

    // 清理
    stream_engine_destroy(engine);
    sqlite_svc_destroy(db);
    datahub_destroy(&hub);
    remove(config_path);

    // 驱动 Loop 排空异步 close 任务，根治 LSan 报错
    for (int i = 0; i < 10; i++) uv_run(loop, UV_RUN_NOWAIT);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_loader_dynamic_build),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
