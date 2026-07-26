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
#include "edge_app_mgr.h"

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

static void test_sqlite_app_registry_crud(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_app_registry.db";
    unlink(db_path);

    sqlite_config_t cfg = {
        .strategy = STRATEGY_LITE,
        .db_path = db_path
    };
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);

    // 1. Save App Manifest
    app_manifest_t manifest_in;
    memset(&manifest_in, 0, sizeof(manifest_in));
    snprintf(manifest_in.id, sizeof(manifest_in.id), "voltage_guard");
    snprintf(manifest_in.name, sizeof(manifest_in.name), "Voltage Guard App");
    snprintf(manifest_in.version, sizeof(manifest_in.version), "1.0.0");
    snprintf(manifest_in.author, sizeof(manifest_in.author), "EdgeOS Team");
    snprintf(manifest_in.wasm_path, sizeof(manifest_in.wasm_path), "/var/edgeos/apps/voltage_guard/app.wasm");
    manifest_in.trigger_tag = 1001;
    snprintf(manifest_in.entry_func, sizeof(manifest_in.entry_func), "on_event");
    manifest_in.fault_policy = FAULT_POLICY_AUTO_RESTART;
    manifest_in.heap_limit_kb = 512;
    manifest_in.timeout_ms = 50;
    manifest_in.state = APP_STATE_RUNNING;
    manifest_in.restart_count = 2;
    manifest_in.created_at = 1770000000000ULL;

    int rc = sqlite_svc_save_app(svc, &manifest_in);
    assert_int_equal(rc, 0);

    // 2. Get App Manifest
    app_manifest_t manifest_out;
    rc = sqlite_svc_get_app(svc, "voltage_guard", &manifest_out);
    assert_int_equal(rc, 0);
    assert_string_equal(manifest_out.id, "voltage_guard");
    assert_string_equal(manifest_out.name, "Voltage Guard App");
    assert_string_equal(manifest_out.version, "1.0.0");
    assert_string_equal(manifest_out.author, "EdgeOS Team");
    assert_string_equal(manifest_out.wasm_path, "/var/edgeos/apps/voltage_guard/app.wasm");
    assert_int_equal(manifest_out.trigger_tag, 1001);
    assert_string_equal(manifest_out.entry_func, "on_event");
    assert_int_equal(manifest_out.fault_policy, FAULT_POLICY_AUTO_RESTART);
    assert_int_equal(manifest_out.heap_limit_kb, 512);
    assert_int_equal(manifest_out.timeout_ms, 50);
    assert_int_equal(manifest_out.state, APP_STATE_RUNNING);
    assert_int_equal(manifest_out.restart_count, 2);

    // 3. List Apps
    app_manifest_t app_list[10];
    int count = sqlite_svc_list_apps(svc, app_list, 10);
    assert_int_equal(count, 1);
    assert_string_equal(app_list[0].id, "voltage_guard");

    // 4. Delete App
    rc = sqlite_svc_delete_app(svc, "voltage_guard");
    assert_int_equal(rc, 0);

    // Verify deletion
    rc = sqlite_svc_get_app(svc, "voltage_guard", &manifest_out);
    assert_int_equal(rc, -1);

    count = sqlite_svc_list_apps(svc, app_list, 10);
    assert_int_equal(count, 0);

    sqlite_svc_destroy(svc);
    unlink(db_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_sqlite_app_registry_crud, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
