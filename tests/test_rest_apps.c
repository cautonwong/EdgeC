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

static FILE *g_trace = NULL;

static void trace_phase(const char *phase) {
    if (!g_trace) {
        g_trace = fopen("rest_apps_trace.txt", "a");
    }
    if (g_trace) {
        fprintf(g_trace, "%s\n", phase);
        fflush(g_trace);
        fsync(fileno(g_trace));
    }
    fprintf(stderr, "[rest_apps] %s\n", phase);
}

static void _on_close_cb(uv_handle_t *handle) {
    (void)handle;
}

static int setup(void **state) {
    trace_phase("setup");
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    *state = loop;
    return 0;
}

static int teardown(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    trace_phase("teardown: uv_run DEFAULT enter");
    uv_run(loop, UV_RUN_DEFAULT);
    trace_phase("teardown: uv_run DEFAULT exit, closing handles");
    uv_walk(loop, (uv_walk_cb)_on_close_cb, NULL);
    int close_rc = 0;
    while ((close_rc = uv_loop_close(loop)) == UV_EBUSY) uv_run(loop, UV_RUN_ONCE);
    trace_phase("teardown: loop closed");
    free(loop);
    return 0;
}

static void test_rest_apps_api_endpoints(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_rest_apps.db";
    unlink(db_path);
    trace_phase("setup db");

    sqlite_config_t cfg = {
        .strategy = STRATEGY_LITE,
        .db_path = db_path
    };
    sqlite_svc_t *db = sqlite_svc_create(loop, &cfg);
    assert_non_null(db);
    trace_phase("db created");

    edge_app_mgr_t *app_mgr = edge_app_mgr_create(loop, NULL, db, NULL);
    assert_non_null(app_mgr);
    trace_phase("app_mgr created");

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    arbiter_t *arb = (arbiter_t *)0x1234;

    service_t *svc = service_new(loop, hub, arb, db, NULL, NULL, NULL);
    assert_non_null(svc);
    trace_phase("service created");
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
    trace_phase("upload dispatched");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    // 2. Test GET /api/v1/apps/list
    resp = rest_dispatch(svc, "GET", "/api/v1/apps/list", "");
    trace_phase("list dispatched");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "voltage_guard"));
    free(resp);

    // 3. Test POST /api/v1/apps/stop
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/stop", "{\"app_id\":\"voltage_guard\"}");
    trace_phase("stop dispatched");
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
    trace_phase("start dispatched");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    // Verify running state
    rc = edge_app_mgr_get_app(app_mgr, "voltage_guard", &m);
    assert_int_equal(rc, 0);
    assert_int_equal(m.state, APP_STATE_RUNNING);

    service_delete(svc);
    trace_phase("service deleted");
    edge_app_mgr_destroy(app_mgr);
    trace_phase("app_mgr destroyed");
    sqlite_svc_destroy(db);
    trace_phase("db destroyed");
    free(hub);
    unlink(db_path);
}

int main(void) {
    remove("rest_apps_trace.txt");
    trace_phase("main start");
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_rest_apps_api_endpoints, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
