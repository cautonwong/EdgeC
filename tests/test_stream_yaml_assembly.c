#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <cmocka.h>
#include <cjson/cJSON.h>

#include "datahub.h"
#include "stream_engine.h"

// --- Custom Gain Stage for Dynamic Factory Test ---

typedef struct {
    double gain;
    int exec_count;
    double last_output;
} custom_gain_state_t;

static stage_ret_t custom_gain_proc(pipeline_stage_t *stage, pipe_msg_t *msg) {
    if (!stage || !msg) return STAGE_ERROR;
    custom_gain_state_t *st = (custom_gain_state_t *)stage->priv_state;
    if (st && msg->type == PIPEMSG_TYPE_SCALAR) {
        msg->payload.val.f = (float)((double)msg->payload.val.f * st->gain);
        st->exec_count++;
        st->last_output = (double)msg->payload.val.f;
    }
    return STAGE_OK;
}

static pipeline_stage_t *custom_gain_factory(const char *name, void *config_json, pipeline_loader_ctx_t *ctx) {
    (void)name;
    (void)ctx;
    cJSON *cfg = (cJSON *)config_json;
    double gain = 2.0;
    if (cfg) {
        cJSON *g_item = cJSON_GetObjectItem(cfg, "gain");
        if (g_item) gain = g_item->valuedouble;
    }

    pipeline_stage_t *stage = calloc(1, sizeof(pipeline_stage_t));
    if (!stage) return NULL;
    strncpy(stage->name, name ? name : "gain", sizeof(stage->name) - 1);
    stage->process = custom_gain_proc;

    custom_gain_state_t *st = calloc(1, sizeof(custom_gain_state_t));
    st->gain = gain;
    stage->priv_state = st;
    stage->cleanup_state = free;
    return stage;
}

// --- Test Cases ---

static void test_declarative_dag_assembly_and_execution(void **state) {
    (void)state;
    datahub_t hub;
    int rc = datahub_init(&hub, NULL, "stream_dag_test", 64, 0x10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    stream_engine_t *engine = stream_engine_create(&hub, NULL, NULL, NULL);
    assert_non_null(engine);

    const char *contract_json = 
    "{"
    "  \"pipelines\": ["
    "    {"
    "      \"id\": \"grid_analytics_dag\","
    "      \"trigger_tag\": 268435457,"
    "      \"nodes\": ["
    "        { \"name\": \"clamp_1\", \"type\": \"clamp\", \"params\": { \"min\": 100.0, \"max\": 300.0 } },"
    "        { \"name\": \"ema_1\", \"type\": \"ema\", \"inputs\": [\"clamp_1\"], \"params\": { \"alpha\": 0.5 } },"
    "        { \"name\": \"transform_1\", \"type\": \"transform\", \"inputs\": [\"ema_1\"], \"params\": { \"alpha\": 1.1, \"beta\": 5.0 } }"
    "      ]"
    "    }"
    "  ]"
    "}";

    pipeline_loader_ctx_t ctx = {0};
    rc = stream_engine_load_yaml_contract(engine, contract_json, &ctx);
    assert_int_equal(rc, 0);

    // 推送触发消息
    pipe_msg_t msg = {
        .tag_id = 268435457,
        .ts = 1000000,
        .type = PIPEMSG_TYPE_SCALAR,
        .payload.val.f = 200.0f,
    };
    stream_engine_push(engine, &msg);

    // 验证 DOT 导出
    char *dot = stream_engine_export_dot(engine);
    assert_non_null(dot);
    assert_non_null(strstr(dot, "grid_analytics_dag"));
    assert_non_null(strstr(dot, "clamp_1"));
    assert_non_null(strstr(dot, "ema_1"));
    free(dot);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
}

static void test_dag_cycle_detection_rejected(void **state) {
    (void)state;
    datahub_t hub;
    int rc = datahub_init(&hub, NULL, "stream_cycle_test", 64, 0x10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    stream_engine_t *engine = stream_engine_create(&hub, NULL, NULL, NULL);
    assert_non_null(engine);

    // 构造循环依赖有向图：node_A -> node_B -> node_A
    const char *cyclic_contract = 
    "{"
    "  \"pipelines\": ["
    "    {"
    "      \"id\": \"invalid_cycle_pipe\","
    "      \"trigger_tag\": 1001,"
    "      \"nodes\": ["
    "        { \"name\": \"node_A\", \"type\": \"clamp\", \"inputs\": [\"node_B\"], \"params\": { \"min\": 0.0, \"max\": 100.0 } },"
    "        { \"name\": \"node_B\", \"type\": \"ema\", \"inputs\": [\"node_A\"], \"params\": { \"alpha\": 0.5 } }"
    "      ]"
    "    }"
    "  ]"
    "}";

    pipeline_loader_ctx_t ctx = {0};
    rc = stream_engine_load_yaml_contract(engine, cyclic_contract, &ctx);
    assert_int_equal(rc, -22); // 拒绝循环依赖拓扑

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
}

static void test_custom_stage_factory_registration(void **state) {
    (void)state;
    datahub_t hub;
    int rc = datahub_init(&hub, NULL, "stream_factory_test", 64, 0x10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    stream_engine_t *engine = stream_engine_create(&hub, NULL, NULL, NULL);
    assert_non_null(engine);

    // 动态注册自定义算子工厂
    rc = stream_engine_register_stage_factory("custom_gain", custom_gain_factory);
    assert_int_equal(rc, 0);

    const char *custom_contract = 
    "{"
    "  \"pipelines\": ["
    "    {"
    "      \"id\": \"custom_gain_pipe\","
    "      \"trigger_tag\": 2001,"
    "      \"nodes\": ["
    "        { \"name\": \"gain_x3\", \"type\": \"custom_gain\", \"params\": { \"gain\": 3.0 } }"
    "      ]"
    "    }"
    "  ]"
    "}";

    pipeline_loader_ctx_t ctx = {0};
    rc = stream_engine_load_yaml_contract(engine, custom_contract, &ctx);
    assert_int_equal(rc, 0);

    pipe_msg_t msg = {
        .tag_id = 2001,
        .ts = 5000,
        .type = PIPEMSG_TYPE_SCALAR,
        .payload.val.f = 10.0f,
    };
    stream_engine_push(engine, &msg);
    // 10.0 * 3.0 = 30.0f
    assert_float_equal(msg.payload.val.f, 30.0f, 0.01f);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_declarative_dag_assembly_and_execution),
        cmocka_unit_test(test_dag_cycle_detection_rejected),
        cmocka_unit_test(test_custom_stage_factory_registration),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
