#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>
#include <cjson/cJSON.h>

#include "sqlite_service.h"
#include "edge_app_mgr.h"
#include "edge_service.h"
#include "rest_server.h"

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

static void test_rest_apps_api_endpoints(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_rest_apps.db";
    unlink(db_path);

    sqlite_config_t cfg = {
        .strategy = STRATEGY_LITE,
        .db_path = db_path
    };
    sqlite_svc_t *db = sqlite_svc_create(loop, &cfg);
    assert_non_null(db);

    edge_app_mgr_t *app_mgr = edge_app_mgr_create(loop, NULL, db, NULL);
    assert_non_null(app_mgr);

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    arbiter_t *arb = (arbiter_t *)0x1234;

    service_t *svc = service_new(loop, hub, arb, db, NULL, NULL, NULL);
    assert_non_null(svc);
    service_set_app_mgr(svc, app_mgr);

    // 1. Test POST /api/v1/apps/upload
    const char *upload_body =
        "{"
        "\"name\": \"voltage_guard\","
        "\"trigger_tag\": 1001,"
        "\"entry_func\": \"on_event\","
        "\"payload_hex\": \"0061736d01000000\""
        "}";

    char *resp = rest_dispatch(svc, "POST", "/api/v1/apps/upload", upload_body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    // 2. Test GET /api/v1/apps/list
    resp = rest_dispatch(svc, "GET", "/api/v1/apps/list", "");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "voltage_guard"));
    free(resp);

    // 3. Test POST /api/v1/apps/stop
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/stop", "{\"app_id\":\"voltage_guard\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    // Verify stopped state
    app_manifest_t m;
    int rc = edge_app_mgr_get_app(app_mgr, "voltage_guard", &m);
    assert_int_equal(rc, 0);
    assert_int_equal(m.state, APP_STATE_STOPPED);

    // 4. Test POST /api/v1/apps/start
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/start", "{\"app_id\":\"voltage_guard\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    // Verify running state
    rc = edge_app_mgr_get_app(app_mgr, "voltage_guard", &m);
    assert_int_equal(rc, 0);
    assert_int_equal(m.state, APP_STATE_RUNNING);

    service_delete(svc);
    edge_app_mgr_destroy(app_mgr);
    sqlite_svc_destroy(db);
    free(hub);
    unlink(db_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_rest_apps_api_endpoints, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
