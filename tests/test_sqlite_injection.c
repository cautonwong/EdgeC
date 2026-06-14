#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "sqlite_service.h"
#include "stream_engine.h"

// standard_stages.c 中的算子工厂
extern pipeline_stage_t* stage_sqlite_sink_create_v2(const edge_stage_def_t* def,
                                                     pipeline_loader_ctx_t* ctx);

typedef struct {
    uv_loop_t* loop;
    sqlite_svc_t* svc;
} test_state_t;

static int setup(void** state) {
    test_state_t* ts = malloc(sizeof(test_state_t));
    ts->loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(ts->loop);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = ":memory:"};
    ts->svc = sqlite_svc_create(ts->loop, &cfg);

    *state = ts;
    return 0;
}

static int teardown(void** state) {
    test_state_t* ts = (test_state_t*)*state;
    sqlite_svc_destroy(ts->svc);

    uv_run(ts->loop, UV_RUN_DEFAULT);
    uv_loop_close(ts->loop);
    free(ts->loop);
    free(ts);
    return 0;
}

static void test_sqlite_sink_injection_blocked(void** state) {
    test_state_t* ts = (test_state_t*)*state;
    sqlite_svc_t* svc = ts->svc;

    // 构造一个合法的注入 (现在应该被拦截)
    const char* malicious_table = "pwned (id INT); --";

    edge_stage_def_t def = {0};
    def.params.s1 = malicious_table;

    pipeline_loader_ctx_t ctx = {0};
    ctx.db = svc;

    pipeline_stage_t* sink = stage_sqlite_sink_create_v2(&def, &ctx);
    // 预期创建失败
    assert_null(sink);

    // 等待一段时间确保没有后台执行
    for (int i = 0; i < 50; i++) {
        uv_run(ts->loop, UV_RUN_NOWAIT);
        usleep(1000);
    }

    // 检查 pwned 表是否被创建 (不应该存在)
    sqlite3* db = (sqlite3*)sqlite_svc_get_db_handle(svc);
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(
        db, "SELECT name FROM sqlite_master WHERE type='table' AND name='pwned';", -1, &stmt, NULL);
    assert_int_equal(rc, SQLITE_OK);

    rc = sqlite3_step(stmt);
    // 预期为 SQLITE_DONE (未找到)
    assert_int_equal(rc, SQLITE_DONE);

    sqlite3_finalize(stmt);
}

static void test_sqlite_sink_valid_works(void** state) {
    test_state_t* ts = (test_state_t*)*state;
    sqlite_svc_t* svc = ts->svc;

    const char* valid_table = "telemetry_safe";

    edge_stage_def_t def = {0};
    def.params.s1 = valid_table;

    pipeline_loader_ctx_t ctx = {0};
    ctx.db = svc;

    pipeline_stage_t* sink = stage_sqlite_sink_create_v2(&def, &ctx);
    assert_non_null(sink);

    // 等待异步执行完成
    for (int i = 0; i < 100; i++) {
        uv_run(ts->loop, UV_RUN_NOWAIT);
        usleep(10000);
    }

    sqlite3* db = (sqlite3*)sqlite_svc_get_db_handle(svc);
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(
        db, "SELECT name FROM sqlite_master WHERE type='table' AND name='telemetry_safe';", -1,
        &stmt, NULL);
    assert_int_equal(rc, SQLITE_OK);

    rc = sqlite3_step(stmt);
    assert_int_equal(rc, SQLITE_ROW);

    sqlite3_finalize(stmt);

    // 清理
    if (sink->cleanup_cfg) sink->cleanup_cfg(sink->priv_cfg);
    free(sink);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_sqlite_sink_injection_blocked, setup, teardown),
        cmocka_unit_test_setup_teardown(test_sqlite_sink_valid_works, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
