#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "../src/stream_engine/stream_engine_priv.h"  // 允许访问 engine->pipelines
#include "datahub.h"
#include "stream_engine.h"

// --- Helper: 生成测试配置 ---
static void write_config(const char *path, uint32_t trigger_tag) {
    FILE *f = fopen(path, "w");
    fprintf(f,
            "{\n"
            "  \"pipelines\": [\n"
            "    {\n"
            "      \"id\": \"hot_reload_pipe\",\n"
            "      \"trigger_tag\": %u,\n"
            "      \"stages\": [\n"
            "        { \"type\": \"accumulate\", \"config\": {} }\n"
            "      ]\n"
            "    }\n"
            "  ]\n"
            "}",
            trigger_tag);
    fclose(f);
}

// 模拟外部探针：探查某个算子是否被执行
typedef struct {
    int last_processed_tag;
} reload_probe_t;

static stage_ret_t probe_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    reload_probe_t *probe = (reload_probe_t *)stage->priv_state;
    if (ev->type == EVT_DATA_POINT) {
        probe->last_processed_tag = (int)ev->tag_id;
    }
    return STAGE_OK;
}

static void test_pipeline_hot_reload_logic(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "reload_bus", 64, NULL, NULL);
    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    pipeline_loader_ctx_t ctx = {0};

    const char *config_file = "hot_reload.json";

    // 1. 部署 V1: 监听 1001
    write_config(config_file, 1001);
    assert_int_equal(stream_engine_load_config(engine, config_file, &ctx), 0);

    // 注入探针 (手动注入，因为 JSON 加载器目前不支持自定义 probe 算子)
    reload_probe_t probe = {.last_processed_tag = 0};
    pipeline_add_stage(engine->pipelines[0], "probe", probe_proc, &probe, NULL, NULL, NULL);

    pipe_event_t ev1 = {.type = EVT_DATA_POINT, .tag_id = 1001};
    stream_engine_push_event(engine, &ev1);
    assert_int_equal(probe.last_processed_tag, 1001);

    // 2. 执行热重载: 切换到 2002
    stream_engine_clear_pipelines(engine);
    write_config(config_file, 2002);
    assert_int_equal(stream_engine_load_config(engine, config_file, &ctx), 0);

    // 再次注入探针到新 Pipeline
    probe.last_processed_tag = 0;
    pipeline_add_stage(engine->pipelines[0], "probe", probe_proc, &probe, NULL, NULL, NULL);

    // 验证：旧 Tag 应该失效 (由于 trigger_tag 过滤)
    pipe_event_t ev_old = {.type = EVT_DATA_POINT, .tag_id = 1001};
    stream_engine_push_event(engine, &ev_old);
    assert_int_equal(probe.last_processed_tag, 0);  // 没过 Filter

    // 验证：新 Tag 生效
    pipe_event_t ev_new = {.type = EVT_DATA_POINT, .tag_id = 2002};
    stream_engine_push_event(engine, &ev_new);
    assert_int_equal(probe.last_processed_tag, 2002);

    // 3. 清理
    stream_engine_destroy(engine);
    datahub_destroy(&hub);
    remove(config_file);
    while (uv_run(loop, UV_RUN_NOWAIT))
        ;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pipeline_hot_reload_logic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
