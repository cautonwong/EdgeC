#include <cjson/cJSON.h>
#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "app_repo_sqlite.h"
#include "edge_authz_sqlite.h"
#include "edge_secret_sqlite.h"
#include "edge_time_trust.h"
#include "app_points_sqlite.h"
#include "edge_app_mgr.h"
#include "iems_app_manager_adapter.h"
#include "edge_service.h"
#include "rest_server.h"
#include "sqlite_service.h"
#include "stream_engine.h"
#include "datahub_priv.h"

#define TEST_APPS_DIR "edgeos_test_apps"

static const char *valid_manifest =
    "{\"abi\":1,\"id\":\"voltage-guard\",\"version\":\"1.0.0\",\"language\":\"c\","
    "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
    "\"limits\":{\"heapKb\":256,\"storageKb\":64,\"cpuMsPerSec\":50,"
    "\"callsPerSec\":100,\"publishPerSec\":10},"
    "\"capabilities\":{\"tags\":{\"read\":[\"U_A\"],\"write\":[]},\"log\":true}}";

static void _on_close_cb(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

static int setup(void **state) {
    setenv("EDGE_APPS_DIR", TEST_APPS_DIR, 1);
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    *state = loop;
    return 0;
}

static int teardown(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    /* Bounded drain: never block forever if a timer/handle remains active */
    uv_walk(loop, _on_close_cb, NULL);
    int close_rc = uv_loop_close(loop);
    for (int i = 0; i < 100000 && close_rc == UV_EBUSY; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        close_rc = uv_loop_close(loop);
    }
    free(loop);
    return close_rc == 0 ? 0 : -1;
}

static void _trust_clock(sqlite_svc_t *db) {
    char reason[EDGE_TIME_TRUST_REASON_SIZE];
    assert_int_equal(edge_time_trust_mark(db, edge_time_now_wall_ms(),
                                          edge_time_now_monotonic_ms(), reason, sizeof(reason)),
                     0);
}

static void _drain_app_jobs(edge_app_mgr_t *mgr, uv_loop_t *loop) {
    while (edge_app_mgr_is_busy(mgr)) uv_run(loop, UV_RUN_ONCE);
}

static char *_upload_body(const char *name, const char *manifest, const char *payload_hex,
                         const char *idempotency_key) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;
    cJSON_AddStringToObject(root, "name", name);
    if (manifest) cJSON_AddStringToObject(root, "manifest", manifest);
    if (payload_hex) cJSON_AddStringToObject(root, "payload_hex", payload_hex);
    if (idempotency_key) cJSON_AddStringToObject(root, "idempotency_key", idempotency_key);
    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return body;
}

static char *_upgrade_manifest(const char *version, const char *read_scopes) {
    char *buf = malloc(1024);
    if (!buf) return NULL;
    snprintf(buf, 1024,
             "{\"abi\":1,\"id\":\"voltage-guard\",\"version\":\"%s\",\"language\":\"c\","
             "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
             "\"limits\":{\"heapKb\":256,\"storageKb\":64,\"cpuMsPerSec\":50,"
             "\"callsPerSec\":100,\"publishPerSec\":10},"
             "\"capabilities\":{\"tags\":{\"read\":%s,\"write\":[]},\"log\":true}}",
             version, read_scopes);
    return buf;
}

static char *_upgrade_body(const char *manifest, const char *payload_hex,
                           const char *idempotency_key) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;
    cJSON_AddStringToObject(root, "manifest", manifest);
    if (payload_hex) cJSON_AddStringToObject(root, "payload_hex", payload_hex);
    if (idempotency_key) cJSON_AddStringToObject(root, "idempotency_key", idempotency_key);
    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return body;
}

static void test_rest_apps_api_endpoints(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_rest_apps.db";
    unlink(db_path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(loop, &cfg);
    assert_non_null(db);
    _trust_clock(db);

    size_t app_mgr_size = edge_app_mgr_storage_size();
    void *app_mgr_mem = malloc(app_mgr_size);
    edge_app_repo_t app_repo = app_repo_sqlite(db);
    edge_app_mgr_t *app_mgr = edge_app_mgr_create_with_repo_in(app_mgr_mem, app_mgr_size, loop,
                                                              &app_repo, NULL);
    assert_non_null(app_mgr);
    edge_app_mgr_set_apps_dir(app_mgr, TEST_APPS_DIR);

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    arbiter_t *arb = (arbiter_t *)0x1234;

    service_t *svc = service_new(loop, hub, arb, NULL, NULL);
    assert_non_null(svc);
    iems_control_ctx_t control_ctx;
    rest_control_port_t control;
    iems_control_port_init(&control, &control_ctx, app_mgr, NULL, db);
    service_set_control_port(svc, &control);

    // 1. Legacy upload is closed by default: 410 points the caller at the signed package endpoint.
    const char *legacy_upload_body = "{\"name\":\"voltage_guard\",\"payload_hex\":\"0061736d01000000\"}";
    int status = 0;
    char *resp = rest_dispatch_ex(svc, "POST", "/api/v1/apps/upload", legacy_upload_body, &status);
    assert_non_null(resp);
    assert_int_equal(status, 410);
    assert_non_null(strstr(resp, "/api/v1/apps/package"));
    free(resp);

    // Enabled explicitly for migration, the manifest-backed upload path still works.
    rest_server_set_legacy_upload(true);
    unlink(TEST_APPS_DIR "/voltage-guard/app.wasm");
    char *body = _upload_body("voltage-guard", valid_manifest, "0061736d01000000", NULL);
    assert_non_null(body);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/upload", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    _drain_app_jobs(app_mgr, loop);
    assert_int_equal(access(TEST_APPS_DIR "/voltage-guard/app.wasm", F_OK), 0);

    // 1b. Invalid field types and odd-length hex must be rejected, not crash.
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/upload",
                         "{\"name\":\"bad\",\"payload_hex\":123}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "invalid_payload"));
    free(resp);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/upload",
                         "{\"name\":\"bad\",\"payload_hex\":\"abc\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "invalid_payload"));
    free(resp);

    // 2. Test GET /api/v1/apps/list
    resp = rest_dispatch(svc, "GET", "/api/v1/apps/list", "");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "voltage-guard"));
    free(resp);

    // 3. Test POST /api/v1/apps/stop
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/stop", "{\"app_id\":\"voltage-guard\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    _drain_app_jobs(app_mgr, loop);

    // Verify stopped state
    app_manifest_t m;
    int rc = edge_app_mgr_get_app(app_mgr, "voltage-guard", &m);
    assert_int_equal(rc, 0);
    assert_int_equal(m.state, APP_STATE_STOPPED);

    // 4. Test POST /api/v1/apps/start
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/start", "{\"app_id\":\"voltage-guard\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    _drain_app_jobs(app_mgr, loop);

    // Verify running state
    rc = edge_app_mgr_get_app(app_mgr, "voltage-guard", &m);
    assert_int_equal(rc, 0);
    assert_int_equal(m.state, APP_STATE_RUNNING);

    // 5. Grant admin API: approve, list, revoke, then revoke again fails.
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/grant",
                         "{\"app_id\":\"voltage-guard\",\"capability\":\"tags.read\","
                         "\"scope\":\"[\\\"U_A\\\"]\",\"sensitivity_max\":1,"
                         "\"purpose\":\"display\",\"actor\":\"admin\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/grants", "{\"app_id\":\"voltage-guard\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "tags.read"));
    assert_non_null(strstr(resp, "U_A"));
    free(resp);

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/grants", "{}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "tags.read"));
    free(resp);

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/revoke_grant",
                         "{\"app_id\":\"voltage-guard\",\"capability\":\"tags.read\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    // Revocation is idempotent: the second call succeeds and keeps the first timestamp.
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/revoke_grant",
                         "{\"app_id\":\"voltage-guard\",\"capability\":\"tags.read\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    // Sensitive grants need a purpose; the REST path must surface the rejection.
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/grant",
                         "{\"app_id\":\"voltage-guard\",\"capability\":\"tags.read\","
                         "\"scope\":\"[\\\"U_A\\\"]\",\"sensitivity_max\":2}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "grant_rejected"));
    free(resp);

    // 6. Upgrade: grants inherit or go pending, and downgrades are rejected.
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/grant",
                         "{\"app_id\":\"voltage-guard\",\"capability\":\"tags.read\","
                         "\"scope\":\"[\\\"U_A\\\"]\",\"purpose\":\"display\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    char *manifest_up = _upgrade_manifest("1.1.0", "[\"U_A\"]");
    char *upgrade_up = _upgrade_body(manifest_up, NULL, NULL);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/upgrade", upgrade_up);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    free(upgrade_up);
    free(manifest_up);
    _drain_app_jobs(app_mgr, loop);

    rc = edge_app_mgr_get_app(app_mgr, "voltage-guard", &m);
    assert_int_equal(rc, 0);
    assert_string_equal(m.version, "1.1.0");

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/grants", "{\"app_id\":\"voltage-guard\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"state\":1"));
    free(resp);

    manifest_up = _upgrade_manifest("1.2.0", "[\"U_A\",\"U_B\"]");
    upgrade_up = _upgrade_body(manifest_up, NULL, NULL);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/upgrade", upgrade_up);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    free(upgrade_up);
    free(manifest_up);
    _drain_app_jobs(app_mgr, loop);

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/grants", "{\"app_id\":\"voltage-guard\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"state\":0"));
    free(resp);

    /* A downgrade is accepted for execution and rejected by the worker; the
     * registry stays on 1.2.0. */
    manifest_up = _upgrade_manifest("1.1.0", "[\"U_A\"]");
    upgrade_up = _upgrade_body(manifest_up, NULL, NULL);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/upgrade", upgrade_up);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    free(upgrade_up);
    free(manifest_up);
    _drain_app_jobs(app_mgr, loop);

    rc = edge_app_mgr_get_app(app_mgr, "voltage-guard", &m);
    assert_int_equal(rc, 0);
    assert_string_equal(m.version, "1.2.0");

    // 7. Task ledger: submit, replay, status, finish, terminal rejection.
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/task",
                         "{\"kind\":\"install\",\"idempotency_key\":\"rest-task-1\"}");
    assert_non_null(resp);
    cJSON *task_root = cJSON_Parse(resp);
    assert_non_null(task_root);
    cJSON *task_item = cJSON_GetObjectItem(task_root, "task");
    cJSON *task_id_item = cJSON_GetObjectItem(task_item, "id");
    assert_true(cJSON_IsString(task_id_item));
    char task_id[64];
    snprintf(task_id, sizeof(task_id), "%s", task_id_item->valuestring);
    cJSON_Delete(task_root);
    free(resp);

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/task",
                         "{\"kind\":\"install\",\"idempotency_key\":\"rest-task-1\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"replayed\":true"));
    assert_non_null(strstr(resp, task_id));
    free(resp);

    char task_status_body[128];
    snprintf(task_status_body, sizeof(task_status_body), "{\"id\":\"%s\"}", task_id);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/task/status", task_status_body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"state\":\"pending\""));
    free(resp);

    char task_finish_body[192];
    snprintf(task_finish_body, sizeof(task_finish_body),
             "{\"id\":\"%s\",\"state\":\"succeeded\",\"result_json\":\"{}\"}", task_id);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/task/finish", task_finish_body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/task/status", task_status_body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"state\":\"succeeded\""));
    free(resp);

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/task/finish", task_finish_body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "task_update_rejected"));
    free(resp);

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/task",
                         "{\"kind\":\"purge\",\"idempotency_key\":\"bad\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "task_rejected"));
    free(resp);

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/task/purge", "");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    // 8. Registry status and snapshot publication.
    setenv("EDGEC_REGISTRY_DIR", "test_rest_registry", 1);
    resp = rest_dispatch(svc, "GET", "/api/v1/apps/status", "");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"generation\""));
    assert_non_null(strstr(resp, "\"converged\":null"));
    assert_non_null(strstr(resp, "no_consumers"));
    free(resp);

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/snapshot", "");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    assert_non_null(strstr(resp, "gen-"));
    free(resp);
    unsetenv("EDGEC_REGISTRY_DIR");
    assert_int_equal(system("rm -rf test_rest_registry"), 0);

    // 9. Consumer heartbeats drive the convergence verdict.
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/consumers/heartbeat",
                         "{\"name\":\"appctl\",\"applied_generation\":0}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    resp = rest_dispatch(svc, "GET", "/api/v1/apps/status", "");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"name\":\"appctl\""));
    assert_non_null(strstr(resp, "\"converged\":false"));
    free(resp);

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/consumers/heartbeat",
                         "{\"name\":\"appctl\",\"applied_generation\":999999}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    resp = rest_dispatch(svc, "GET", "/api/v1/apps/status", "");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"converged\":true"));
    free(resp);

    service_delete(svc);
    edge_app_mgr_destroy(app_mgr);
    free(app_mgr_mem);
    sqlite_svc_destroy(db);
    free(hub);
    unlink(db_path);
}

static void test_raw_wasm_and_legacy_abi_rejected(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_rest_apps_reject.db";
    unlink(db_path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(loop, &cfg);
    assert_non_null(db);
    _trust_clock(db);

    size_t mgr_size = edge_app_mgr_storage_size();
    void *mgr_mem = malloc(mgr_size);
    edge_app_repo_t repo = app_repo_sqlite(db);
    edge_app_mgr_t *mgr = edge_app_mgr_create_with_repo_in(mgr_mem, mgr_size, loop, &repo, NULL);
    assert_non_null(mgr);
    edge_app_mgr_set_apps_dir(mgr, TEST_APPS_DIR);

    stream_engine_storage_t engine_storage;
    stream_engine_t *engine = stream_engine_create_in(&engine_storage, NULL, loop, NULL, NULL);
    assert_non_null(engine);

    iems_control_ctx_t control_ctx;
    rest_control_port_t control;
    iems_control_port_init(&control, &control_ctx, mgr, engine, db);

    const unsigned char wasm[] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};
    char raw_id[64];
    char legacy_id[64];
    snprintf(raw_id, sizeof(raw_id), "raw-wasm-%d", (int)getpid());
    snprintf(legacy_id, sizeof(legacy_id), "legacy-abi-%d", (int)getpid());

    // Raw wasm (no manifest) is rejected before any file write or stage attach.
    assert_int_equal(control.install_app(control.ctx, NULL, raw_id, 1001, "on_event", wasm,
                                         sizeof(wasm), NULL),
                     -1);

    // A manifest without abi=1 is rejected for the same reason.
    char legacy_manifest[512];
    snprintf(legacy_manifest, sizeof(legacy_manifest),
             "{\"id\":\"%s\",\"version\":\"1.0.0\",\"language\":\"c\","
             "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
             "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,"
             "\"callsPerSec\":1,\"publishPerSec\":1}}",
             legacy_id);
    assert_int_equal(control.install_app(control.ctx, legacy_manifest, legacy_id, 1001, "on_event",
                                         wasm, sizeof(wasm), NULL),
                     -1);

    char path[256];
    snprintf(path, sizeof(path), TEST_APPS_DIR "/%s/app.wasm", raw_id);
    assert_int_not_equal(access(path, F_OK), 0);
    snprintf(path, sizeof(path), TEST_APPS_DIR "/%s/app.wasm", legacy_id);
    assert_int_not_equal(access(path, F_OK), 0);

    app_manifest_t m;
    assert_int_equal(edge_app_mgr_get_app(mgr, raw_id, &m), -1);
    assert_int_equal(edge_app_mgr_get_app(mgr, legacy_id, &m), -1);

    char *topology = stream_engine_dump_json(engine);
    assert_non_null(topology);
    assert_null(strstr(topology, raw_id));
    assert_null(strstr(topology, legacy_id));
    free(topology);

    stream_engine_destroy(engine);
    edge_app_mgr_destroy(mgr);
    free(mgr_mem);
    sqlite_svc_destroy(db);
    unlink(db_path);
}

static char *_secret_body(const char *app_id, const char *name, const char *hex) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;
    cJSON_AddStringToObject(root, "app_id", app_id);
    if (name) cJSON_AddStringToObject(root, "name", name);
    if (hex) cJSON_AddStringToObject(root, "value", hex);
    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return body;
}

static void test_rest_apps_secret_admin(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_rest_apps_secret.db";
    unlink(db_path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(loop, &cfg);
    assert_non_null(db);
    _trust_clock(db);

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    arbiter_t *arb = (arbiter_t *)0x1234;
    service_t *svc = service_new(loop, hub, arb, NULL, NULL);
    assert_non_null(svc);

    iems_control_ctx_t control_ctx;
    rest_control_port_t control;
    iems_control_port_init(&control, &control_ctx, NULL, NULL, db);
    service_set_control_port(svc, &control);

    // Set and overwrite the same name: both are writes, neither returns the value.
    char *body = _secret_body("app.demo", "apiKey", "00ff10");
    assert_non_null(body);
    char *resp = rest_dispatch(svc, "POST", "/api/v1/apps/secret", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    body = _secret_body("app.demo", "apiKey", "a1b2c3");
    assert_non_null(body);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/secret", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    body = _secret_body("app.demo", "second", "01");
    assert_non_null(body);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/secret", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    // List returns names only, never values.
    body = _secret_body("app.demo", NULL, NULL);
    assert_non_null(body);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/secret/list", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "apiKey"));
    assert_non_null(strstr(resp, "second"));
    assert_null(strstr(resp, "00ff10"));
    assert_null(strstr(resp, "a1b2c3"));
    free(resp);

    // There is no admin read-back endpoint.
    resp = rest_dispatch(svc, "GET", "/api/v1/apps/secret/list", "");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "not_found"));
    free(resp);

    // Over 4 KB is refused.
    size_t hex_len = 4097 * 2;
    char *oversize = malloc(hex_len + 1);
    assert_non_null(oversize);
    memset(oversize, 'a', hex_len);
    oversize[hex_len] = '\0';
    body = _secret_body("app.demo", "tooBig", oversize);
    free(oversize);
    assert_non_null(body);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/secret", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "too_large"));
    free(resp);

    // Odd-length and non-hex payloads are rejected.
    body = _secret_body("app.demo", "bad", "abc");
    assert_non_null(body);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/secret", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "invalid_payload"));
    free(resp);

    body = _secret_body("app.demo", "bad", "zz");
    assert_non_null(body);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/secret", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "invalid_payload"));
    free(resp);

    // Delete one name, then confirm only the other remains.
    body = _secret_body("app.demo", "apiKey", NULL);
    assert_non_null(body);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/secret/delete", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    body = _secret_body("app.demo", NULL, NULL);
    assert_non_null(body);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/secret/list", body);
    free(body);
    assert_non_null(resp);
    assert_null(strstr(resp, "apiKey"));
    assert_non_null(strstr(resp, "second"));
    free(resp);

    service_delete(svc);
    sqlite_svc_destroy(db);
    free(hub);
    unlink(db_path);
}

static char *_manifest_for(const char *id, const char *version) {
    char *buf = malloc(1024);
    if (!buf) return NULL;
    snprintf(buf, 1024,
             "{\"abi\":1,\"id\":\"%s\",\"version\":\"%s\",\"language\":\"c\","
             "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
             "\"limits\":{\"heapKb\":256,\"storageKb\":64,\"cpuMsPerSec\":50,"
             "\"callsPerSec\":100,\"publishPerSec\":10},"
             "\"capabilities\":{\"tags\":{\"read\":[\"U_A\"],\"write\":[]},\"log\":true}}",
             id, version);
    return buf;
}

static int _read_bytes(const char *path, unsigned char *out, size_t cap) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    size_t len = fread(out, 1, cap, f);
    fclose(f);
    return (int)len;
}

static void test_rest_apps_upgrade_swap_rollback(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_rest_apps_lifecycle.db";
    const char *wasm_path = TEST_APPS_DIR "/swap-app/app.wasm";
    unlink(db_path);
    unlink(wasm_path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(loop, &cfg);
    assert_non_null(db);
    _trust_clock(db);

    size_t mgr_size = edge_app_mgr_storage_size();
    void *mgr_mem = malloc(mgr_size);
    edge_app_repo_t app_repo = app_repo_sqlite(db);
    edge_app_mgr_t *mgr = edge_app_mgr_create_with_repo_in(mgr_mem, mgr_size, loop, &app_repo,
                                                           NULL);
    assert_non_null(mgr);
    edge_app_mgr_set_apps_dir(mgr, TEST_APPS_DIR);

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    service_t *svc = service_new(loop, hub, (arbiter_t *)0x1234, NULL, NULL);
    assert_non_null(svc);
    iems_control_ctx_t control_ctx;
    rest_control_port_t control;
    iems_control_port_init(&control, &control_ctx, mgr, NULL, db);
    service_set_control_port(svc, &control);
    rest_server_set_legacy_upload(true);

    const unsigned char bytes_a[] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00,
                                     0x00, 0x00, 0x00, 0x01, 'A'};
    const unsigned char bytes_b[] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00,
                                     0x00, 0x00, 0x00, 0x01, 'B'};
    unsigned char got[16];

    char *manifest_v1 = _manifest_for("swap-app", "1.0.0");
    assert_non_null(manifest_v1);
    char *body = _upload_body("swap-app", manifest_v1, "0061736d01000000000141", "install-key-1");
    assert_non_null(body);
    char *resp = rest_dispatch(svc, "POST", "/api/v1/apps/upload", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    _drain_app_jobs(mgr, loop);
    assert_int_equal(_read_bytes(wasm_path, got, sizeof(got)), (int)sizeof(bytes_a));
    assert_memory_equal(got, bytes_a, sizeof(bytes_a));

    char *manifest_v2 = _manifest_for("swap-app", "1.1.0");
    body = _upgrade_body(manifest_v2, "0061736d01000000000142", "upgrade-key-1");
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/upgrade", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    _drain_app_jobs(mgr, loop);

    assert_int_equal(_read_bytes(wasm_path, got, sizeof(got)), (int)sizeof(bytes_b));
    assert_memory_equal(got, bytes_b, sizeof(bytes_b));
    app_manifest_t m;
    assert_int_equal(edge_app_mgr_get_app(mgr, "swap-app", &m), 0);
    assert_string_equal(m.version, "1.1.0");
    assert_string_equal(m.prev_version, "1.0.0");

    /* Replaying the key must not execute the 1.2.0 upgrade again. */
    char *manifest_v3 = _manifest_for("swap-app", "1.2.0");
    body = _upgrade_body(manifest_v3, "0061736d01000000000143", "upgrade-key-1");
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/upgrade", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    _drain_app_jobs(mgr, loop);
    assert_int_equal(edge_app_mgr_get_app(mgr, "swap-app", &m), 0);
    assert_string_equal(m.version, "1.1.0");
    assert_int_equal(_read_bytes(wasm_path, got, sizeof(got)), (int)sizeof(bytes_b));
    assert_memory_equal(got, bytes_b, sizeof(bytes_b));

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/rollback", "{\"app_id\":\"swap-app\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    _drain_app_jobs(mgr, loop);
    assert_int_equal(_read_bytes(wasm_path, got, sizeof(got)), (int)sizeof(bytes_a));
    assert_memory_equal(got, bytes_a, sizeof(bytes_a));
    assert_int_equal(edge_app_mgr_get_app(mgr, "swap-app", &m), 0);
    assert_string_equal(m.version, "1.0.0");
    assert_string_equal(m.prev_version, "");

    /* A task that already failed replays as a failure without rerunning. */
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/task",
                         "{\"kind\":\"install\",\"idempotency_key\":\"install-fail-1\"}");
    assert_non_null(resp);
    cJSON *task_root = cJSON_Parse(resp);
    assert_non_null(task_root);
    cJSON *task_id_item = cJSON_GetObjectItem(cJSON_GetObjectItem(task_root, "task"), "id");
    assert_true(cJSON_IsString(task_id_item));
    char failed_task_id[64];
    snprintf(failed_task_id, sizeof(failed_task_id), "%s", task_id_item->valuestring);
    cJSON_Delete(task_root);
    free(resp);

    char finish_body[192];
    snprintf(finish_body, sizeof(finish_body),
             "{\"id\":\"%s\",\"state\":\"failed\",\"result_json\":\"boom\"}", failed_task_id);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/task/finish", finish_body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    body = _upload_body("swap-app", manifest_v1, "0061736d0100000041", "install-fail-1");
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/upload", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "install_failed"));
    free(resp);
    assert_int_equal(edge_app_mgr_get_app(mgr, "swap-app", &m), 0);
    assert_string_equal(m.version, "1.0.0");

    free(manifest_v1);
    free(manifest_v2);
    free(manifest_v3);
    unlink(wasm_path);
    service_delete(svc);
    edge_app_mgr_destroy(mgr);
    free(mgr_mem);
    sqlite_svc_destroy(db);
    free(hub);
    unlink(db_path);
}

typedef struct {
    uint8_t data[16];
    size_t len;
    char key[32];
    int calls;
} package_capture_t;

static int _capture_install_package(void *ctx, const uint8_t *data, size_t len, const char *key,
                                    const uint8_t expected_pubkey[32]) {
    package_capture_t *capture = (package_capture_t *)ctx;
    (void)expected_pubkey;
    capture->calls++;
    capture->len = len;
    if (len <= sizeof(capture->data)) memcpy(capture->data, data, len);
    snprintf(capture->key, sizeof(capture->key), "%s", key ? key : "");
    return 0;
}

static void test_rest_apps_package(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    datahub_t *hub = calloc(1, sizeof(datahub_t));
    service_t *svc = service_new(loop, hub, (arbiter_t *)0x1234, NULL, NULL);
    assert_non_null(svc);

    package_capture_t capture;
    memset(&capture, 0, sizeof(capture));
    rest_control_port_t control;
    memset(&control, 0, sizeof(control));
    control.ctx = &capture;
    control.install_package = _capture_install_package;
    service_set_control_port(svc, &control);

    char *resp = rest_dispatch(svc, "POST", "/api/v1/apps/package",
                               "{\"package_hex\":\"45444745\",\"idempotency_key\":\"req-7\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    assert_int_equal(capture.calls, 1);
    assert_int_equal(capture.len, 4);
    assert_memory_equal(capture.data, "EDGE", 4);
    assert_string_equal(capture.key, "req-7");

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/package", "{\"package_hex\":\"zz\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "invalid_payload"));
    free(resp);
    assert_int_equal(capture.calls, 1);

    rest_control_port_t empty;
    memset(&empty, 0, sizeof(empty));
    service_set_control_port(svc, &empty);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/package", "{\"package_hex\":\"4544\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "app_manager_unavailable"));
    free(resp);
}

static void test_rest_apps_package_key_env(void **state) {
    (void)state;
    iems_control_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    rest_control_port_t port;
    memset(&port, 0, sizeof(port));

    setenv("EDGEC_PACKAGE_PUBKEY",
           "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", 1);
    iems_control_port_init(&port, &ctx, NULL, NULL, NULL);
    assert_int_equal(ctx.package_key_count, 1);
    assert_string_equal(ctx.package_keys[0].publisher_id, "local");
    assert_int_equal(ctx.package_keys[0].public_key[0], 0x00);
    assert_int_equal(ctx.package_keys[0].public_key[15], 0x0f);
    assert_int_equal(ctx.package_keys[0].public_key[31], 0x1f);

    unsetenv("EDGEC_PACKAGE_PUBKEY");
    iems_control_port_init(&port, &ctx, NULL, NULL, NULL);
    assert_int_equal(ctx.package_key_count, 0);
}

static void test_rest_apps_package_key_file(void **state) {
    (void)state;
    char path[] = "/tmp/edgeos_pub_XXXXXX";
    int fd = mkstemp(path);
    assert_true(fd >= 0);
    const char *hex =
        "1f1e1d1c1b1a191817161514131211100f0e0d0c0b0a09080706050403020100\n";
    assert_int_equal(write(fd, hex, strlen(hex)), (ssize_t)strlen(hex));
    close(fd);

    iems_control_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    rest_control_port_t port;
    memset(&port, 0, sizeof(port));
    unsetenv("EDGEC_PACKAGE_PUBKEY");
    setenv("EDGEC_PACKAGE_PUBKEY_FILE", path, 1);
    iems_control_port_init(&port, &ctx, NULL, NULL, NULL);
    assert_int_equal(ctx.package_key_count, 1);
    assert_int_equal(ctx.package_keys[0].public_key[0], 0x1f);
    assert_int_equal(ctx.package_keys[0].public_key[31], 0x00);

    unsetenv("EDGEC_PACKAGE_PUBKEY_FILE");
    unlink(path);
}

static const char *points_manifest =
    "{\"abi\":1,\"id\":\"points-app\",\"version\":\"1.0.0\",\"language\":\"c\","
    "\"execution_profile\":\"wasm\",\"entry\":\"app.wasm\","
    "\"limits\":{\"heapKb\":1,\"storageKb\":1,\"cpuMsPerSec\":1,\"callsPerSec\":1,"
    "\"publishPerSec\":1},"
    "\"capabilities\":{\"points\":{\"publish\":[\"app.points-app.sag_count\"],"
    "\"reason\":\"own points\"}},"
    "\"contributes\":{\"telemetry\":{\"points\":[{\"name\":\"sag_count\","
    "\"type\":\"number\",\"unit\":\"count\",\"contractVersion\":1}]}}}";

static void test_rest_apps_contributed_points(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_rest_apps_points.db";
    unlink(db_path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(loop, &cfg);
    assert_non_null(db);
    _trust_clock(db);

    size_t app_mgr_size = edge_app_mgr_storage_size();
    void *app_mgr_mem = malloc(app_mgr_size);
    edge_app_repo_t app_repo = app_repo_sqlite(db);
    edge_app_mgr_t *app_mgr = edge_app_mgr_create_with_repo_in(app_mgr_mem, app_mgr_size, loop,
                                                              &app_repo, NULL);
    assert_non_null(app_mgr);
    edge_app_mgr_set_apps_dir(app_mgr, TEST_APPS_DIR);

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    service_t *svc = service_new(loop, hub, (arbiter_t *)0x1234, NULL, NULL);
    assert_non_null(svc);
    iems_control_ctx_t control_ctx;
    rest_control_port_t control;
    iems_control_port_init(&control, &control_ctx, app_mgr, NULL, db);
    service_set_control_port(svc, &control);

    const uint8_t wasm[8] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};
    int rc = control.install_app(control.ctx, points_manifest, "points-app", 0, NULL, wasm,
                                 sizeof(wasm), "pt-1");
    assert_int_equal(rc, 0);
    _drain_app_jobs(app_mgr, loop);

    app_point_row_t rows[2];
    size_t count = 0;
    char reason[160];
    assert_int_equal(
        app_points_list(db, "points-app", rows, 2, &count, reason, sizeof(reason)), 0);
    assert_int_equal(count, 1);
    assert_string_equal(rows[0].name, "sag_count");
    assert_int_equal(rows[0].slot, 0);
    assert_string_equal(rows[0].unit, "count");
    assert_int_equal(rows[0].contract_version, 1);

    char *resp = rest_dispatch(svc, "GET", "/api/v1/apps/status/points-app", "");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "sag_count"));
    assert_non_null(strstr(resp, "\"slot\":0"));
    free(resp);
}

static void test_rest_apps_uninstall_purge(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_rest_apps_purge.db";
    const char *app_dir = TEST_APPS_DIR "/purge-app";
    unlink(db_path);
    unlink(TEST_APPS_DIR "/purge-app/app.wasm");

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(loop, &cfg);
    assert_non_null(db);
    _trust_clock(db);

    size_t mgr_size = edge_app_mgr_storage_size();
    void *mgr_mem = malloc(mgr_size);
    edge_app_repo_t app_repo = app_repo_sqlite(db);
    edge_app_mgr_t *mgr = edge_app_mgr_create_with_repo_in(mgr_mem, mgr_size, loop, &app_repo,
                                                           NULL);
    assert_non_null(mgr);
    edge_app_mgr_set_apps_dir(mgr, TEST_APPS_DIR);

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    service_t *svc = service_new(loop, hub, (arbiter_t *)0x1234, NULL, NULL);
    assert_non_null(svc);
    iems_control_ctx_t control_ctx;
    rest_control_port_t control;
    iems_control_port_init(&control, &control_ctx, mgr, NULL, db);
    service_set_control_port(svc, &control);
    rest_server_set_legacy_upload(true);

    char *manifest = _manifest_for("purge-app", "1.0.0");
    char *body = _upload_body("purge-app", manifest, "0061736d01000000000141", "purge-install");
    assert_non_null(body);
    char *resp = rest_dispatch(svc, "POST", "/api/v1/apps/upload", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    _drain_app_jobs(mgr, loop);

    body = _secret_body("purge-app", "token", "00ff10");
    assert_non_null(body);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/secret", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    const char *grant =
        "{\"app_id\":\"purge-app\",\"capability\":\"tags.read\","
        "\"scope\":\"[\\\"U_A\\\"]\",\"purpose\":\"purge test\"}";
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/grant", grant);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);

    edge_secret_store_t secrets = edge_secret_sqlite(db);
    char names[8][EDGE_SECRET_NAME_SIZE];
    size_t secret_count = 0;
    assert_int_equal(edge_secret_list(&secrets, "purge-app", names, 8, &secret_count),
                     EDGE_SECRET_OK);
    assert_int_equal(secret_count, 1);
    edge_authz_store_t authz = edge_authz_sqlite_store(db);
    edge_app_grant_t grants[8];
    size_t grant_count = 0;
    assert_int_equal(authz.list_grants(authz.ctx, "purge-app", grants, 8, &grant_count), 0);
    assert_int_equal(grant_count, 1);

    resp = rest_dispatch(svc, "POST", "/api/v1/apps/uninstall",
                         "{\"app_id\":\"purge-app\",\"disposition\":\"purge\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    _drain_app_jobs(mgr, loop);

    app_manifest_t m;
    assert_int_equal(edge_app_mgr_get_app(mgr, "purge-app", &m), 0);
    assert_int_equal(m.state, APP_STATE_UNINSTALLED);
    secret_count = 0;
    assert_int_equal(edge_secret_list(&secrets, "purge-app", names, 8, &secret_count),
                     EDGE_SECRET_OK);
    assert_int_equal(secret_count, 0);
    grant_count = 0;
    assert_int_equal(authz.list_grants(authz.ctx, "purge-app", grants, 8, &grant_count), 0);
    assert_int_equal(grant_count, 0);
    assert_int_equal(access(app_dir, F_OK), -1);

    const char *export_path = "test_rest_apps_export.tar";
    unlink(export_path);
    body = _upload_body("purge-app", manifest, "0061736d01000000000141", "export-install");
    assert_non_null(body);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/upload", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    _drain_app_jobs(mgr, loop);

    char export_body[256];
    snprintf(export_body, sizeof(export_body),
             "{\"app_id\":\"purge-app\",\"disposition\":\"export\","
             "\"export_path\":\"%s\"}",
             export_path);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/uninstall", export_body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    _drain_app_jobs(mgr, loop);
    assert_int_equal(access(export_path, F_OK), 0);
    assert_int_equal(edge_app_mgr_get_app(mgr, "purge-app", &m), 0);
    assert_int_equal(m.state, APP_STATE_UNINSTALLED);
    unlink(export_path);
}

static void test_rest_apps_install_requires_trusted_clock(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_rest_apps_clock.db";
    unlink(db_path);
    unlink(TEST_APPS_DIR "/clock-app/app.wasm");

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(loop, &cfg);
    assert_non_null(db);

    size_t mgr_size = edge_app_mgr_storage_size();
    void *mgr_mem = malloc(mgr_size);
    edge_app_repo_t app_repo = app_repo_sqlite(db);
    edge_app_mgr_t *mgr = edge_app_mgr_create_with_repo_in(mgr_mem, mgr_size, loop, &app_repo,
                                                           NULL);
    assert_non_null(mgr);
    edge_app_mgr_set_apps_dir(mgr, TEST_APPS_DIR);
    datahub_t *hub = calloc(1, sizeof(datahub_t));
    service_t *svc = service_new(loop, hub, (arbiter_t *)0x1234, NULL, NULL);
    assert_non_null(svc);
    iems_control_ctx_t control_ctx;
    rest_control_port_t control;
    iems_control_port_init(&control, &control_ctx, mgr, NULL, db);
    service_set_control_port(svc, &control);
    rest_server_set_legacy_upload(true);

    char *manifest = _manifest_for("clock-app", "1.0.0");
    char *body = _upload_body("clock-app", manifest, "0061736d01000000000141", "clock-key-1");
    assert_non_null(body);
    char *resp = rest_dispatch(svc, "POST", "/api/v1/apps/upload", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"error\""));
    free(resp);
    _drain_app_jobs(mgr, loop);
    app_manifest_t m;
    assert_int_not_equal(edge_app_mgr_get_app(mgr, "clock-app", &m), 0);
    assert_int_not_equal(access(TEST_APPS_DIR "/clock-app/app.wasm", F_OK), 0);

    _trust_clock(db);
    body = _upload_body("clock-app", manifest, "0061736d01000000000141", "clock-key-2");
    assert_non_null(body);
    resp = rest_dispatch(svc, "POST", "/api/v1/apps/upload", body);
    free(body);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    _drain_app_jobs(mgr, loop);
    assert_int_equal(edge_app_mgr_get_app(mgr, "clock-app", &m), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_rest_apps_api_endpoints, setup, teardown),
        cmocka_unit_test_setup_teardown(test_raw_wasm_and_legacy_abi_rejected, setup, teardown),
        cmocka_unit_test_setup_teardown(test_rest_apps_secret_admin, setup, teardown),
        cmocka_unit_test_setup_teardown(test_rest_apps_upgrade_swap_rollback, setup, teardown),
        cmocka_unit_test_setup_teardown(test_rest_apps_package, setup, teardown),
        cmocka_unit_test(test_rest_apps_package_key_env),
        cmocka_unit_test_setup_teardown(test_rest_apps_uninstall_purge, setup, teardown),
        cmocka_unit_test_setup_teardown(test_rest_apps_install_requires_trusted_clock, setup,
                                        teardown),
        cmocka_unit_test(test_rest_apps_package_key_file),
        cmocka_unit_test_setup_teardown(test_rest_apps_contributed_points, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
