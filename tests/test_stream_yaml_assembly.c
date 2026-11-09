#include <cjson/cJSON.h>
#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "datahub.h"
#include "stream_engine.h"
#include "stream_bus_datahub_adapter.h"
#include "datahub_priv.h"

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

static pipeline_stage_t *custom_gain_factory(const char *name, void *config_json,
                                             pipeline_loader_ctx_t *ctx) {
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

static int stub_sqlite_exec(void *ctx, const char *sql) {
    (void)ctx;
    (void)sql;
    return 0;
}

static int stub_submit_control(void *ctx, int bus_id, uint32_t tag_id, double value,
                               const char *proto) {
    (void)ctx;
    (void)bus_id;
    (void)tag_id;
    (void)value;
    (void)proto;
    return 0;
}

// --- Test Cases ---

static void test_declarative_dag_assembly_and_execution(void **state) {
    (void)state;
    datahub_t hub;
    int rc = datahub_init(&hub, NULL, "stream_dag_test", 64, 0x10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    stream_engine_t *engine = stream_engine_create_datahub(&hub, NULL, NULL, NULL);
    assert_non_null(engine);

    const char *contract_json =
        "{"
        "  \"pipelines\": ["
        "    {"
        "      \"id\": \"grid_analytics_dag\","
        "      \"trigger_tag\": 268435457,"
        "      \"nodes\": ["
        "        { \"name\": \"clamp_1\", \"type\": \"clamp\", \"params\": { \"min\": 100.0, "
        "\"max\": 300.0 } },"
        "        { \"name\": \"ema_1\", \"type\": \"ema\", \"inputs\": [\"clamp_1\"], \"params\": "
        "{ \"alpha\": 0.5 } },"
        "        { \"name\": \"transform_1\", \"type\": \"transform\", \"inputs\": [\"ema_1\"], "
        "\"params\": { \"alpha\": 1.1, \"beta\": 5.0 } }"
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

    stream_engine_t *engine = stream_engine_create_datahub(&hub, NULL, NULL, NULL);
    assert_non_null(engine);

    // 构造循环依赖有向图：node_A -> node_B -> node_A
    const char *cyclic_contract =
        "{"
        "  \"pipelines\": ["
        "    {"
        "      \"id\": \"invalid_cycle_pipe\","
        "      \"trigger_tag\": 1001,"
        "      \"nodes\": ["
        "        { \"name\": \"node_A\", \"type\": \"clamp\", \"inputs\": [\"node_B\"], "
        "\"params\": { \"min\": 0.0, \"max\": 100.0 } },"
        "        { \"name\": \"node_B\", \"type\": \"ema\", \"inputs\": [\"node_A\"], \"params\": "
        "{ \"alpha\": 0.5 } }"
        "      ]"
        "    }"
        "  ]"
        "}";

    pipeline_loader_ctx_t ctx = {0};
    rc = stream_engine_load_yaml_contract(engine, cyclic_contract, &ctx);
    assert_int_equal(rc, -22);  // 拒绝循环依赖拓扑

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
}

static void test_custom_stage_factory_registration(void **state) {
    (void)state;
    datahub_t hub;
    int rc =
        datahub_init(&hub, NULL, "stream_factory_test", 64, 0x10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    stream_engine_t *engine = stream_engine_create_datahub(&hub, NULL, NULL, NULL);
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
        "        { \"name\": \"gain_x3\", \"type\": \"custom_gain\", \"params\": { \"gain\": 3.0 } "
        "}"
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

static void test_builtin_stage_types_load(void **state) {
    (void)state;
    datahub_t hub;
    int rc = datahub_init(&hub, NULL, "stream_builtin_types", 64, 0x10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    stream_engine_t *engine = stream_engine_create_datahub(&hub, NULL, NULL, NULL);
    assert_non_null(engine);

    static const struct {
        const char *type;
        const char *params;
    } cases[] = {
        {"energy_integrator", "{\"state_tag\": 101}"},
        {"dsp_rms", "{\"state_tag\": 102}"},
        {"filter", "{\"tag_id\": 103}"},
        {"periodic_freeze", "{\"kind\": 1}"},
        {"time_guard", "{}"},
        {"battery_guard", "{\"max_soc\": 95.0, \"min_soc\": 10.0, \"max_charge_w\": 5000.0, "
                          "\"max_discharge_w\": 5000.0}"},
        {"transform", "{\"alpha\": 1.1, \"beta\": 5.0}"},
        {"ema", "{\"alpha\": 0.5}"},
        {"clamp", "{\"min\": 0.0, \"max\": 100.0}"},
        {"peak_shave", "{\"threshold\": 5000.0, \"ctrl_tag\": 104}"},
        {"accumulate", "{}"},
        {"sqlite_sink", "{\"table\": \"history\"}"},
        {"reflex", "{\"bus_id\": 1, \"protocol\": \"modbus\", \"target_tag\": 105, "
                   "\"threshold\": 10.0, \"greater_than\": true, \"action_value\": 1.0}"},
    };

    pipeline_loader_ctx_t ctx = {0};
    ctx.db.exec = stub_sqlite_exec;
    ctx.control_ops.submit_control = stub_submit_control;

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char contract[1024];
        snprintf(contract, sizeof(contract),
                 "{ \"pipelines\": [ { \"id\": \"builtin_%s\", \"trigger_tag\": %u, "
                 "\"nodes\": [ { \"name\": \"node_%s\", \"type\": \"%s\", \"params\": %s } ] } ] }",
                 cases[i].type, 3001u + (unsigned)i, cases[i].type, cases[i].type, cases[i].params);
        rc = stream_engine_load_yaml_contract(engine, contract, &ctx);
        assert_int_equal(rc, 0);

        char *dot = stream_engine_export_dot(engine);
        assert_non_null(dot);
        char node_name[128];
        snprintf(node_name, sizeof(node_name), "node_%s", cases[i].type);
        assert_non_null(strstr(dot, node_name));
        free(dot);
    }

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
}

static void test_context_dependent_stage_types_skip_without_ports(void **state) {
    (void)state;
    datahub_t hub;
    int rc = datahub_init(&hub, NULL, "stream_builtin_skip", 64, 0x10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    stream_engine_t *engine = stream_engine_create_datahub(&hub, NULL, NULL, NULL);
    assert_non_null(engine);

    const char *contract =
        "{"
        "  \"pipelines\": ["
        "    {"
        "      \"id\": \"context_skip\","
        "      \"trigger_tag\": 3002,"
        "      \"nodes\": ["
        "        { \"name\": \"js_rule_1\", \"type\": \"js_rule\", \"params\": "
        "{ \"script_path\": \"algo/js/normal.js\" } },"
        "        { \"name\": \"wasm_executor_1\", \"type\": \"wasm_executor\", \"params\": "
        "{ \"module\": \"missing.wasm\", \"entry\": \"on_event\" } }"
        "      ]"
        "    }"
        "  ]"
        "}";

    pipeline_loader_ctx_t ctx = {0};
    rc = stream_engine_load_yaml_contract(engine, contract, &ctx);
    assert_int_equal(rc, 0);

    char *dot = stream_engine_export_dot(engine);
    assert_non_null(dot);
    assert_null(strstr(dot, "js_rule_1"));
    assert_null(strstr(dot, "wasm_executor_1"));
    free(dot);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_declarative_dag_assembly_and_execution),
        cmocka_unit_test(test_dag_cycle_detection_rejected),
        cmocka_unit_test(test_custom_stage_factory_registration),
        cmocka_unit_test(test_builtin_stage_types_load),
        cmocka_unit_test(test_context_dependent_stage_types_skip_without_ports),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
