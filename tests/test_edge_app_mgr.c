#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "edge_app_mgr.h"
#include "sqlite_service.h"

static void _on_close_cb(uv_handle_t *handle) {
    (void)handle;
}

static int setup(void **state) {
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    *state = loop;
    return 0;
}

static int teardown(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    uv_run(loop, UV_RUN_DEFAULT);
    uv_walk(loop, (uv_walk_cb)_on_close_cb, NULL);
    while (uv_loop_close(loop) == UV_EBUSY) uv_run(loop, UV_RUN_ONCE);
    free(loop);
    return 0;
}

static void test_manifest_parse_and_default(void **state) {
    (void)state;

    // 1. JSON Parser Test
    const char *json_str =
        "{"
        "\"id\": \"temp_monitor\","
        "\"name\": \"Temperature Monitor App\","
        "\"version\": \"2.1.0\","
        "\"author\": \"Energy Dept\","
        "\"trigger_tag\": 2002,"
        "\"entry_func\": \"process_temp\","
        "\"fault_policy\": \"isolate_and_alarm\","
        "\"heap_limit_kb\": 1024,"
        "\"timeout_ms\": 200"
        "}";

    app_manifest_t m1;
    int rc = edge_app_mgr_parse_manifest_json(json_str, &m1);
    assert_int_equal(rc, 0);
    assert_string_equal(m1.id, "temp_monitor");
    assert_string_equal(m1.name, "Temperature Monitor App");
    assert_string_equal(m1.version, "2.1.0");
    assert_string_equal(m1.author, "Energy Dept");
    assert_int_equal(m1.trigger_tag, 2002);
    assert_string_equal(m1.entry_func, "process_temp");
    assert_int_equal(m1.fault_policy, FAULT_POLICY_ISOLATE_AND_ALARM);
    assert_int_equal(m1.heap_limit_kb, 1024);
    assert_int_equal(m1.timeout_ms, 200);

    // 2. Default Manifest for raw .wasm
    app_manifest_t m2;
    rc = edge_app_mgr_construct_default_manifest("voltage_alarm.wasm", &m2);
    assert_int_equal(rc, 0);
    assert_string_equal(m2.id, "voltage_alarm");
    assert_string_equal(m2.name, "voltage_alarm App");
    assert_int_equal(m2.fault_policy, FAULT_POLICY_AUTO_RESTART);
}

static void test_app_lifecycle_and_boot_restore(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_app_mgr.db";
    unlink(db_path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(loop, &cfg);
    assert_non_null(db);

    edge_app_mgr_t *mgr = edge_app_mgr_create(loop, NULL, db, NULL);
    assert_non_null(mgr);

    // Install App
    app_manifest_t m;
    edge_app_mgr_construct_default_manifest("demo_app", &m);
    snprintf(m.wasm_path, sizeof(m.wasm_path), "/tmp/test_edgeos_apps/demo_app/app.wasm");

    const char dummy_wasm[] = "\x00\x61\x73\x6d\x01\x00\x00\x00";
    int rc = edge_app_mgr_install(mgr, &m, dummy_wasm, sizeof(dummy_wasm));
    assert_int_equal(rc, 0);

    // Start App
    rc = edge_app_mgr_start(mgr, "demo_app");
    assert_int_equal(rc, 0);

    app_manifest_t check_m;
    rc = edge_app_mgr_get_app(mgr, "demo_app", &check_m);
    assert_int_equal(rc, 0);
    assert_int_equal(check_m.state, APP_STATE_RUNNING);

    // Simulate Boot Restore
    int restored = edge_app_mgr_boot_restore(mgr);
    assert_int_equal(restored, 1);

    // Stop and Uninstall
    rc = edge_app_mgr_stop(mgr, "demo_app");
    assert_int_equal(rc, 0);

    rc = edge_app_mgr_uninstall(mgr, "demo_app");
    assert_int_equal(rc, 0);

    rc = edge_app_mgr_get_app(mgr, "demo_app", &check_m);
    assert_int_equal(rc, -1);

    edge_app_mgr_destroy(mgr);
    sqlite_svc_destroy(db);
    unlink(db_path);
}

typedef struct {
    app_manifest_t app;
    int has_app;
} mock_app_repo_ctx_t;

static int _mock_save_app(void *ctx, const app_manifest_t *manifest) {
    mock_app_repo_ctx_t *m = (mock_app_repo_ctx_t *)ctx;
    m->app = *manifest;
    m->has_app = 1;
    return 0;
}

static int _mock_get_app(void *ctx, const char *app_id, app_manifest_t *out_manifest) {
    mock_app_repo_ctx_t *m = (mock_app_repo_ctx_t *)ctx;
    if (m->has_app && strcmp(m->app.id, app_id) == 0) {
        *out_manifest = m->app;
        return 0;
    }
    return -1;
}

static int _mock_delete_app(void *ctx, const char *app_id) {
    mock_app_repo_ctx_t *m = (mock_app_repo_ctx_t *)ctx;
    if (m->has_app && strcmp(m->app.id, app_id) == 0) {
        m->has_app = 0;
        return 0;
    }
    return -1;
}

static int _mock_list_apps(void *ctx, app_manifest_t *out_apps, int max_count) {
    mock_app_repo_ctx_t *m = (mock_app_repo_ctx_t *)ctx;
    if (m->has_app && max_count > 0) {
        out_apps[0] = m->app;
        return 1;
    }
    return 0;
}

static void test_app_mgr_custom_repo(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    mock_app_repo_ctx_t mock_ctx = {0};
    edge_app_repo_t repo = {
        .ctx = &mock_ctx,
        .save_app = _mock_save_app,
        .get_app = _mock_get_app,
        .delete_app = _mock_delete_app,
        .list_apps = _mock_list_apps
    };

    edge_app_mgr_t *mgr = edge_app_mgr_create_with_repo(loop, NULL, &repo, NULL);
    assert_non_null(mgr);

    app_manifest_t m = {0};
    strncpy(m.id, "repo_app", sizeof(m.id) - 1);
    const char dummy_wasm[] = "\x00\x61\x73\x6d\x01\x00\x00\x00";
    int rc = edge_app_mgr_install(mgr, &m, dummy_wasm, sizeof(dummy_wasm));
    assert_int_equal(rc, 0);
    assert_int_equal(mock_ctx.has_app, 1);

    rc = edge_app_mgr_start(mgr, "repo_app");
    assert_int_equal(rc, 0);
    assert_int_equal(mock_ctx.app.state, APP_STATE_RUNNING);

    rc = edge_app_mgr_stop(mgr, "repo_app");
    assert_int_equal(rc, 0);
    assert_int_equal(mock_ctx.app.state, APP_STATE_STOPPED);

    rc = edge_app_mgr_uninstall(mgr, "repo_app");
    assert_int_equal(rc, 0);
    assert_int_equal(mock_ctx.has_app, 0);

    edge_app_mgr_destroy(mgr);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_manifest_parse_and_default),
        cmocka_unit_test_setup_teardown(test_app_lifecycle_and_boot_restore, setup, teardown),
        cmocka_unit_test_setup_teardown(test_app_mgr_custom_repo, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
