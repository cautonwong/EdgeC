/**
 * @file test_iems_app_host_ports.c
 * @brief Host ports over real DataHub, app point slots and SQLite authz.
 */

#include <cjson/cJSON.h>
#include <cmocka.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sqlite3.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <uv.h>

#include "app_points_sqlite.h"
#include "app_settings_sqlite.h"
#include "datahub_priv.h"
#include "edge_app_host.h"
#include "edge_app_sdk.h"
#include "edge_authz_sqlite.h"
#include "edgeos/edge_authz.h"
#include "iems_app_host_ports.h"
#include "iems_driver_adapter.h"
#include "pipe_msg.h"
#include "sqlite_service.h"

#define APP_BASE 131072u

static const edge_tag_meta_t kProductTags[] = {
    {.id = 1001, .name = "U_A", .type = "number", .unit = "V"},
};

static int run_loop_until(uv_loop_t *loop, pid_t pid, int timeout_ms) {
    int status = 0;
    for (int waited = 0; waited < timeout_ms; waited += 10) {
        uv_run(loop, UV_RUN_NOWAIT);
        if (waitpid(pid, &status, WNOHANG) == pid) return status;
        usleep(10000);
    }
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);
    return -1;
}

static int run_loop_until_byte(uv_loop_t *loop, int fd, int timeout_ms) {
    for (int waited = 0; waited < timeout_ms; waited += 10) {
        uv_run(loop, UV_RUN_NOWAIT);
        char byte = 0;
        if (read(fd, &byte, 1) == 1) return (unsigned char)byte;
        usleep(10000);
    }
    return -1;
}

static void child_phase_one(int fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);

    char setting[128];
    const char *err = edge_app_setting_get(sdk, "threshold", setting, sizeof(setting));
    if (err || strcmp(setting, "250") != 0) _exit(50);
    err = edge_app_setting_get(sdk, "mode", setting, sizeof(setting));
    if (err || strcmp(setting, "\"trip\"") != 0) _exit(51);
    err = edge_app_setting_get(sdk, "level", setting, sizeof(setting));
    if (err || strcmp(setting, "5") != 0) _exit(52);
    err = edge_app_setting_get(sdk, "optional", setting, sizeof(setting));
    if (!err || strcmp(err, "not_set") != 0) _exit(53);
    err = edge_app_setting_get(sdk, "missing", setting, sizeof(setting));
    if (!err || strcmp(err, "not_found") != 0) _exit(54);

    uint32_t handle = 0;
    const char *access = NULL;
    err = edge_app_point_resolve(sdk, "U_A", &handle, &access);
    if (err || handle != 1001 || strcmp(access, "read") != 0) _exit(11);

    edge_app_sample_t sample;
    memset(&sample, 0, sizeof(sample));
    err = edge_app_point_read(sdk, 1001, &sample);
    if (err || sample.value.number != 220.5 || strcmp(sample.quality, "good") != 0) _exit(12);
    if (sample.source_ts != 5000ULL) _exit(13);

    err = edge_app_point_resolve(sdk, "app.demo.value", &handle, &access);
    if (err || handle != APP_BASE || strcmp(access, "publish") != 0) _exit(14);

    edge_app_sample_t publish;
    memset(&publish, 0, sizeof(publish));
    publish.value.kind = EDGE_APP_VALUE_FLOAT;
    publish.value.number = 42.25;
    publish.quality = "good";
    err = edge_app_point_publish(sdk, APP_BASE, &publish);
    if (err) _exit(15);

    err = edge_app_point_publish(sdk, 1001, &publish);
    if (!err || strcmp(err, "denied_scope") != 0) _exit(16);

    err = edge_app_point_read(sdk, APP_BASE, &sample);
    if (!err || strcmp(err, "denied_scope") != 0) _exit(17);

    err = edge_app_point_resolve(sdk, "app.demo.nope", &handle, &access);
    if (!err || strcmp(err, "not_found") != 0) _exit(18);

    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void child_phase_two(int fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(20);

    uint32_t handle = 0;
    const char *access = NULL;
    const char *err = edge_app_point_resolve(sdk, "U_A", &handle, &access);
    if (err || handle != 1001) _exit(21);

    edge_app_sample_t sample;
    memset(&sample, 0, sizeof(sample));
    err = edge_app_point_read(sdk, 1001, &sample);
    if (!err || strcmp(err, "denied_revoked") != 0) _exit(22);

    edge_app_sample_t publish;
    memset(&publish, 0, sizeof(publish));
    publish.value.kind = EDGE_APP_VALUE_FLOAT;
    publish.value.number = 1.0;
    err = edge_app_point_publish(sdk, APP_BASE, &publish);
    if (!err || strcmp(err, "denied_revoked") != 0) _exit(23);

    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void child_subscribe(int fd, int sync_fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(30);

    uint32_t handle = 0;
    const char *access = NULL;
    const char *err = edge_app_point_resolve(sdk, "U_A", &handle, &access);
    if (err || handle != 1001) _exit(31);

    uint32_t sub = 0;
    err = edge_app_point_subscribe(sdk, 1001, 0.0, 0, 1000, &sub);
    if (err || sub == 0) _exit(32);
    char byte = 'S';
    if (write(sync_fd, &byte, 1) != 1) _exit(33);

    char frame[4096];
    if (edge_app_sdk_next_frame(sdk, frame, sizeof(frame)) != 0) _exit(34);
    cJSON *root = cJSON_Parse(frame);
    if (!root) _exit(35);
    const cJSON *fn = cJSON_GetObjectItemCaseSensitive(root, "fn");
    const cJSON *args = cJSON_GetObjectItemCaseSensitive(root, "args");
    const cJSON *kind = cJSON_GetObjectItemCaseSensitive(args, "kind");
    const cJSON *point = cJSON_GetObjectItemCaseSensitive(args, "point");
    const cJSON *payload_text = cJSON_GetObjectItemCaseSensitive(args, "payload");
    int ok = cJSON_IsString(fn) && strcmp(fn->valuestring, "app_on_event") == 0 &&
             cJSON_IsString(kind) && strcmp(kind->valuestring, "point_change") == 0 &&
             cJSON_IsNumber(point) && (uint32_t)point->valuedouble == 1001 &&
             cJSON_IsString(payload_text);
    if (ok) {
        cJSON *payload = cJSON_Parse(payload_text->valuestring);
        ok = payload != NULL;
        if (ok) {
            const cJSON *value = cJSON_GetObjectItemCaseSensitive(payload, "value");
            const cJSON *number = cJSON_GetObjectItemCaseSensitive(value, "float");
            const cJSON *reason = cJSON_GetObjectItemCaseSensitive(payload, "reason");
            ok = cJSON_IsNumber(number) && number->valuedouble == 244.75 &&
                 cJSON_IsString(reason) && strcmp(reason->valuestring, "change") == 0;
            cJSON_Delete(payload);
        }
    }
    cJSON_Delete(root);
    if (!ok) _exit(36);

    char response[256];
    if (edge_app_sdk_dispatch(sdk, frame, response, sizeof(response)) != 0) _exit(37);
    if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(38);
    byte = 'A';
    if (write(sync_fd, &byte, 1) != 1) _exit(39);

    /* The parent revokes tags.read and publishes again; the queued change fails
     * the allow re-check, so grant_changed arrives before the subscription drops. */
    struct timeval timeout = {2, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (edge_app_sdk_next_frame(sdk, frame, sizeof(frame)) != 0) _exit(42);
    cJSON *revoked_root = cJSON_Parse(frame);
    if (!revoked_root) _exit(43);
    const cJSON *revoked_fn = cJSON_GetObjectItemCaseSensitive(revoked_root, "fn");
    const cJSON *revoked_args = cJSON_GetObjectItemCaseSensitive(revoked_root, "args");
    const cJSON *revoked_kind = cJSON_GetObjectItemCaseSensitive(revoked_args, "kind");
    const cJSON *revoked_point = cJSON_GetObjectItemCaseSensitive(revoked_args, "point");
    const cJSON *revoked_payload_text = cJSON_GetObjectItemCaseSensitive(revoked_args, "payload");
    ok = cJSON_IsString(revoked_fn) && strcmp(revoked_fn->valuestring, "app_on_event") == 0 &&
         cJSON_IsString(revoked_kind) &&
         strcmp(revoked_kind->valuestring, "grant_changed") == 0 && cJSON_IsNull(revoked_point) &&
         cJSON_IsString(revoked_payload_text);
    if (ok) {
        cJSON *payload = cJSON_Parse(revoked_payload_text->valuestring);
        ok = payload != NULL;
        if (ok) {
            const cJSON *handle_item = cJSON_GetObjectItemCaseSensitive(payload, "handle");
            const cJSON *why = cJSON_GetObjectItemCaseSensitive(payload, "reason");
            ok = cJSON_IsNumber(handle_item) && (uint32_t)handle_item->valuedouble == 1001 &&
                 cJSON_IsString(why) && strcmp(why->valuestring, "denied_revoked") == 0;
            cJSON_Delete(payload);
        }
    }
    cJSON_Delete(revoked_root);
    if (!ok) _exit(44);

    if (edge_app_sdk_dispatch(sdk, frame, response, sizeof(response)) != 0) _exit(45);
    if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(46);
    byte = 'B';
    if (write(sync_fd, &byte, 1) != 1) _exit(47);

    /* The revoked grant must not produce point events after grant_changed. */
    timeout.tv_sec = 0;
    timeout.tv_usec = 200 * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (edge_app_sdk_next_frame(sdk, frame, sizeof(frame)) == 0) _exit(48);
    byte = 'C';
    if (write(sync_fd, &byte, 1) != 1) _exit(49);

    /* Wait for teardown to close the host end. */
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    (void)edge_app_sdk_next_frame(sdk, frame, sizeof(frame));
    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void child_settings_watch(int fd, int sync_fd, int watch, int expect_event) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(60);
    if (watch) {
        const char *err = edge_app_setting_watch(sdk);
        if (err) _exit(61);
        /* The watch is idempotent per connection. */
        err = edge_app_setting_watch(sdk);
        if (err) _exit(61);
    }
    char byte = watch ? 'W' : 'N';
    if (write(sync_fd, &byte, 1) != 1) _exit(62);

    struct timeval timeout = {0, 500 * 1000};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    char frame[4096];
    int got = edge_app_sdk_next_frame(sdk, frame, sizeof(frame)) == 0;
    if (!expect_event) {
        if (got) _exit(63);
        byte = 'X';
        if (write(sync_fd, &byte, 1) != 1) _exit(64);
        edge_app_sdk_destroy(sdk);
        _exit(0);
    }
    if (!got) _exit(65);

    cJSON *root = cJSON_Parse(frame);
    if (!root) _exit(66);
    const cJSON *fn = cJSON_GetObjectItemCaseSensitive(root, "fn");
    const cJSON *args = cJSON_GetObjectItemCaseSensitive(root, "args");
    const cJSON *kind = cJSON_GetObjectItemCaseSensitive(args, "kind");
    const cJSON *point = cJSON_GetObjectItemCaseSensitive(args, "point");
    const cJSON *payload_text = cJSON_GetObjectItemCaseSensitive(args, "payload");
    int ok = cJSON_IsString(fn) && strcmp(fn->valuestring, "app_on_event") == 0 &&
             cJSON_IsString(kind) && strcmp(kind->valuestring, "settings_changed") == 0 &&
             cJSON_IsNull(point) && cJSON_IsString(payload_text);
    if (ok) {
        cJSON *payload = cJSON_Parse(payload_text->valuestring);
        ok = payload != NULL && cJSON_GetArraySize(payload) == 1;
        if (ok) {
            const cJSON *key = cJSON_GetObjectItemCaseSensitive(payload, "key");
            ok = cJSON_IsString(key) && strcmp(key->valuestring, "threshold") == 0;
        }
        cJSON_Delete(payload);
    }
    cJSON_Delete(root);
    if (!ok) _exit(67);

    char response[256];
    if (edge_app_sdk_dispatch(sdk, frame, response, sizeof(response)) != 0) _exit(68);
    if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(69);
    byte = 'A';
    if (write(sync_fd, &byte, 1) != 1) _exit(70);
    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void grant_app(sqlite_svc_t *db, const char *app_id, const char *capability,
                      const char *scope_json) {
    edge_authz_store_t store = edge_authz_sqlite_store(db);
    edge_app_grant_t grant = {0};
    snprintf(grant.app_id, sizeof(grant.app_id), "%s", app_id);
    snprintf(grant.capability, sizeof(grant.capability), "%s", capability);
    snprintf(grant.scope_json, sizeof(grant.scope_json), "%s", scope_json);
    snprintf(grant.purpose, sizeof(grant.purpose), "test");
    assert_int_equal(edge_authz_approve_grant(&store, &grant, 1000), EDGE_AUTHZ_OK);
}

static void grant(sqlite_svc_t *db, const char *capability, const char *scope_json) {
    grant_app(db, "demo", capability, scope_json);
}

static void port_event(void *event_ctx, uint32_t handle, const app_host_sample_t *sample) {
    (void)event_ctx;
    (void)handle;
    (void)sample;
}

static void hub_event(datahub_t *hub, const pipe_msg_t *msg, void *user_ctx) {
    (void)hub;
    (void)msg;
    (void)user_ctx;
}

static void test_host_quota_denies_the_33rd(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    const char *db_path = "test_iems_app_host_ports_quota.db";
    unlink(db_path);
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(&loop, &cfg);
    assert_non_null(db);
    sqlite3 *raw = (sqlite3 *)sqlite_svc_get_db_handle(db);
    sqlite3_stmt *stmt;
    assert_int_equal(sqlite3_prepare_v2(
                         raw,
                         "INSERT INTO app_registry(id, version, state) VALUES('demo','1.0.0',1);",
                         -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);
    grant(db, "tags.read", "[\"U_A\"]");

    datahub_t hub;
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "test_host_quota_%d", (int)getpid());
    assert_int_equal(datahub_init(&hub, NULL, shm_name, 64, APP_BASE + 4096, NULL, 0, NULL, NULL,
                                  NULL),
                     0);

    iems_app_host_ctx_t ctx = {
        .app_id = "demo",
        .hub = &hub,
        .db = db,
        .product_tags = kProductTags,
        .product_tag_count = 1,
        .app_base = APP_BASE,
    };
    app_host_ports_t ports;
    iems_app_host_ports_init(&ports, &ctx);

    int32_t ids[32];
    char reason[160];
    for (int i = 0; i < 32; i++) {
        assert_int_equal(ports.subscribe(&ctx, 1001, port_event, &ctx, &ids[i], reason,
                                         sizeof(reason)),
                         0);
        assert_true(ids[i] > 0);
    }
    int32_t denied = 0;
    assert_int_equal(
        ports.subscribe(&ctx, 1001, port_event, &ctx, &denied, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "denied_quota");
    assert_int_equal(denied, 0);

    int extra = datahub_subscribe_tag(&hub, 1001, hub_event, &ctx);
    assert_true(extra > 0);
    assert_int_equal(datahub_unsubscribe(&hub, extra), 0);

    for (int i = 0; i < 32; i++) assert_int_equal(ports.unsubscribe(&ctx, ids[i]), 0);
    for (int i = 0; i < DATAHUB_MAX_SUBSCRIBERS; i++) assert_false(hub.subscribers[i].active);

    datahub_destroy(&hub);
    sqlite_svc_destroy(db);
    unlink(db_path);
    assert_int_equal(uv_loop_close(&loop), 0);
}

/* P1-06b A1-A3：两个应用各 32 个订阅正好填满 DataHub 的 64 槽；第三个应用
 * 第一次订阅就被拒，原因必须是 hub 满（denied_quota），且它自己的记录槽仍空；
 * 前两个应用全部退订后槽可回收，第三个应用能再订阅成功。 */
static void test_two_apps_fill_hub_then_third_denied(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    const char *db_path = "test_iems_app_host_ports_two_apps.db";
    unlink(db_path);
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(&loop, &cfg);
    assert_non_null(db);
    grant_app(db, "app_a", "tags.read", "[\"U_A\"]");
    grant_app(db, "app_b", "tags.read", "[\"U_A\"]");
    grant_app(db, "app_c", "tags.read", "[\"U_A\"]");

    datahub_t hub;
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "test_host_two_apps_%d", (int)getpid());
    assert_int_equal(datahub_init(&hub, NULL, shm_name, 64, APP_BASE + 4096, NULL, 0, NULL, NULL,
                                  NULL),
                     0);

    iems_app_host_ctx_t ctx_a = {
        .app_id = "app_a",
        .hub = &hub,
        .db = db,
        .product_tags = kProductTags,
        .product_tag_count = 1,
        .app_base = APP_BASE,
    };
    iems_app_host_ctx_t ctx_b = ctx_a;
    ctx_b.app_id = "app_b";
    iems_app_host_ctx_t ctx_c = ctx_a;
    ctx_c.app_id = "app_c";

    app_host_ports_t ports_a, ports_b, ports_c;
    iems_app_host_ports_init(&ports_a, &ctx_a);
    iems_app_host_ports_init(&ports_b, &ctx_b);
    iems_app_host_ports_init(&ports_c, &ctx_c);

    char reason[160];
    int32_t ids_a[IEMS_MAX_POINT_SUBSCRIPTIONS];
    int32_t ids_b[IEMS_MAX_POINT_SUBSCRIPTIONS];
    for (int i = 0; i < IEMS_MAX_POINT_SUBSCRIPTIONS; i++) {
        assert_int_equal(ports_a.subscribe(&ctx_a, 1001, port_event, &ctx_a, &ids_a[i], reason,
                                           sizeof(reason)),
                         0);
        assert_true(ids_a[i] > 0);
    }
    for (int i = 0; i < IEMS_MAX_POINT_SUBSCRIPTIONS; i++) {
        assert_int_equal(ports_b.subscribe(&ctx_b, 1001, port_event, &ctx_b, &ids_b[i], reason,
                                           sizeof(reason)),
                         0);
        assert_true(ids_b[i] > 0);
    }
    int active = 0;
    for (int i = 0; i < DATAHUB_MAX_SUBSCRIBERS; i++) {
        if (hub.subscribers[i].active) active++;
    }
    assert_int_equal(active, DATAHUB_MAX_SUBSCRIBERS);

    /* 第三个应用：第一次订阅即被拒，原因是 hub 满而不是它自己的 32 槽满。 */
    int32_t denied = 0;
    assert_int_equal(
        ports_c.subscribe(&ctx_c, 1001, port_event, &ctx_c, &denied, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "denied_quota");
    assert_int_equal(denied, 0);
    for (uint32_t i = 0; i < IEMS_MAX_POINT_SUBSCRIPTIONS; i++) {
        assert_int_equal(ctx_c.subscriptions[i].sub_id, 0);
    }

    for (int i = 0; i < IEMS_MAX_POINT_SUBSCRIPTIONS; i++) {
        assert_int_equal(ports_a.unsubscribe(&ctx_a, ids_a[i]), 0);
        assert_int_equal(ports_b.unsubscribe(&ctx_b, ids_b[i]), 0);
    }
    for (int i = 0; i < DATAHUB_MAX_SUBSCRIBERS; i++) assert_false(hub.subscribers[i].active);

    int32_t sub_c = 0;
    assert_int_equal(
        ports_c.subscribe(&ctx_c, 1001, port_event, &ctx_c, &sub_c, reason, sizeof(reason)), 0);
    assert_true(sub_c > 0);
    assert_int_equal(ports_c.unsubscribe(&ctx_c, sub_c), 0);

    datahub_destroy(&hub);
    sqlite_svc_destroy(db);
    unlink(db_path);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_host_ports_end_to_end(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    const char *db_path = "test_iems_app_host_ports.db";
    unlink(db_path);
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(&loop, &cfg);
    assert_non_null(db);

    sqlite3 *raw = (sqlite3 *)sqlite_svc_get_db_handle(db);
    sqlite3_stmt *stmt;
    assert_int_equal(sqlite3_prepare_v2(
                         raw, "INSERT INTO app_registry(id, version, state) VALUES('demo','1.0.0',1);",
                         -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);

    app_point_spec_t point;
    memset(&point, 0, sizeof(point));
    snprintf(point.name, sizeof(point.name), "value");
    point.type = APP_POINT_NUMBER;
    point.contract_version = 1;
    char reason[160];
    assert_int_equal(app_points_sync(db, "demo", &point, 1, reason, sizeof(reason)), 0);
    grant(db, "tags.read", "[\"U_A\"]");
    grant(db, "points.publish", "[\"app.demo.value\"]");

    static const char *settings_schema =
        "{\"schemaVersion\":1,\"fields\":{"
        "\"threshold\":{\"type\":\"number\",\"default\":240},"
        "\"mode\":{\"type\":\"string\",\"enum\":[\"warn\",\"trip\"],\"default\":\"warn\"},"
        "\"level\":{\"type\":\"integer\",\"default\":5},"
        "\"optional\":{\"type\":\"string\"}}}";
    assert_int_equal(app_settings_schema_save(db, "demo", settings_schema), 0);
    assert_int_equal(app_settings_value_set(db, "demo", "threshold", "250", reason,
                                            sizeof(reason)),
                     0);
    assert_int_equal(app_settings_value_set(db, "demo", "mode", "\"trip\"", reason,
                                            sizeof(reason)),
                     0);

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    assert_non_null(hub);
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "test_host_ports_%d", (int)getpid());
    char shm_path[96];
    snprintf(shm_path, sizeof(shm_path), "/edgeos_shadow_%s", shm_name);
    shm_unlink(shm_path);
    assert_int_equal(datahub_init(hub, &loop, shm_name, 64, APP_BASE + 4096, NULL, 0, NULL, NULL,
                                  NULL),
                     0);
    pipe_msg_t seed;
    pipe_msg_init_scalar(&seed, 1001, 220.5, 5000000000ULL);
    assert_int_equal(datahub_write_shadow(hub, &seed), 0);

    iems_app_host_ctx_t ctx = {
        .app_id = "demo",
        .hub = hub,
        .db = db,
        .product_tags = kProductTags,
        .product_tag_count = 1,
        .app_base = APP_BASE,
    };
    app_host_ports_t ports;
    iems_app_host_ports_init(&ports, &ctx);

    int fds[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    app_host_server_t *server = app_host_server_create(&loop, fds[0], &ports);
    assert_non_null(server);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        child_phase_one(fds[1]);
    }
    close(fds[1]);
    int status = run_loop_until(&loop, pid, 5000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);

    pipe_msg_t published;
    memset(&published, 0, sizeof(published));
    assert_int_equal(datahub_read_shadow(hub, APP_BASE, &published), 0);
    assert_true(published.payload.val.f == 42.25);

    /* Subscribe through the real port, publish through DataHub, ack the event. */
    int sub_fds[2];
    int sync[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, sub_fds), 0);
    assert_int_equal(pipe(sync), 0);
    int sync_flags = fcntl(sync[0], F_GETFL, 0);
    assert_true(sync_flags >= 0);
    assert_int_equal(fcntl(sync[0], F_SETFL, sync_flags | O_NONBLOCK), 0);
    app_host_server_t *sub_server = app_host_server_create(&loop, sub_fds[0], &ports);
    assert_non_null(sub_server);

    pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(sub_fds[0]);
        close(sync[0]);
        child_subscribe(sub_fds[1], sync[1]);
    }
    close(sub_fds[1]);
    close(sync[1]);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), 'S');
    pipe_msg_t change;
    pipe_msg_init_scalar(&change, 1001, 244.75, uv_hrtime());
    assert_int_equal(datahub_write_shadow(hub, &change), 0);
    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), 'A');

    /* Revoking tags.read must stop delivery and release the DataHub slot. */
    edge_authz_store_t store = edge_authz_sqlite_store(db);
    assert_int_equal(edge_authz_revoke_grant(&store, "demo", "tags.read", 3000), EDGE_AUTHZ_OK);
    pipe_msg_t revoked;
    pipe_msg_init_scalar(&revoked, 1001, 250.0, uv_hrtime());
    assert_int_equal(datahub_write_shadow(hub, &revoked), 0);
    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), 'B');
    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), 'C');

    app_host_server_destroy(sub_server);
    for (int i = 0; i < DATAHUB_MAX_SUBSCRIBERS; i++) {
        assert_false(hub->subscribers[i].active);
    }
    status = run_loop_until(&loop, pid, 5000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);
    close(sync[0]);

    pipe_msg_t after;
    pipe_msg_init_scalar(&after, 1001, 250.0, uv_hrtime());
    assert_int_equal(datahub_write_shadow(hub, &after), 0);

    assert_int_equal(edge_authz_revoke_grant(&store, "demo", "points.publish", 2000),
                     EDGE_AUTHZ_OK);

    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    app_host_server_t *server2 = app_host_server_create(&loop, fds[0], &ports);
    assert_non_null(server2);

    pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        child_phase_two(fds[1]);
    }
    close(fds[1]);
    status = run_loop_until(&loop, pid, 5000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);

    /* settings_changed: watched + successful write, unwatched, and a failed write. */
    static const struct {
        int watch;
        int expect_event;
        int fail_write;
    } cases[] = {{1, 1, 0}, {0, 0, 0}, {1, 0, 1}};
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        int settings_fds[2];
        int settings_sync[2];
        assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, settings_fds), 0);
        assert_int_equal(pipe(settings_sync), 0);
        int flags = fcntl(settings_sync[0], F_GETFL, 0);
        assert_true(flags >= 0);
        assert_int_equal(fcntl(settings_sync[0], F_SETFL, flags | O_NONBLOCK), 0);
        app_host_server_t *settings_server = app_host_server_create(&loop, settings_fds[0], &ports);
        assert_non_null(settings_server);

        pid = fork();
        assert_true(pid >= 0);
        if (pid == 0) {
            close(settings_fds[0]);
            close(settings_sync[0]);
            child_settings_watch(settings_fds[1], settings_sync[1], cases[i].watch,
                                 cases[i].expect_event);
        }
        close(settings_fds[1]);
        close(settings_sync[1]);

        assert_int_equal(run_loop_until_byte(&loop, settings_sync[0], 5000),
                         cases[i].watch ? 'W' : 'N');
        int write_rc = app_settings_value_set(db, "demo", "threshold",
                                              cases[i].fail_write ? "\"oops\"" : "251", reason,
                                              sizeof(reason));
        assert_int_equal(write_rc, cases[i].fail_write ? -1 : 0);
        if (write_rc == 0) {
            assert_int_equal(app_host_server_post_settings_changed(settings_server, "threshold"),
                             cases[i].watch ? 0 : -1);
        }
        assert_int_equal(run_loop_until_byte(&loop, settings_sync[0], 5000),
                         cases[i].expect_event ? 'A' : 'X');
        app_host_server_destroy(settings_server);
        status = run_loop_until(&loop, pid, 5000);
        assert_true(status != -1);
        assert_int_equal(WEXITSTATUS(status), 0);
        close(settings_sync[0]);
    }

    app_host_server_destroy(server);
    app_host_server_destroy(server2);
    for (int i = 0; i < 20; i++) uv_run(&loop, UV_RUN_NOWAIT);

    datahub_destroy(hub);
    free(hub);
    sqlite_svc_destroy(db);
    unlink(db_path);
    assert_int_equal(uv_loop_close(&loop), 0);
}

/* P1-04a 切片 1：旧名只读解析。期望值来自 iems_point_model.c 的 27 条名表与
 * iems_v3 dev.node 规则（tag-config-map.md §1.1）：5 个健康旧名 9001..9005，
 * 8 条旧名样例 tag_id=NONE 不可读，匹配按字节精确、不做归一化。 */
static void test_host_ports_old_name_resolution(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    const char *db_path = "test_iems_app_host_ports_old_name.db";
    unlink(db_path);
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(&loop, &cfg);
    assert_non_null(db);
    sqlite3 *raw = (sqlite3 *)sqlite_svc_get_db_handle(db);
    sqlite3_stmt *stmt;
    assert_int_equal(sqlite3_prepare_v2(
                         raw,
                         "INSERT INTO app_registry(id, version, state) VALUES('demo','1.0.0',1);",
                         -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);
    grant(db, "tags.read",
          "[\"EMS_POWER\",\"EMS_CPU_HIGH\",\"EMS_MEM_HIGH\",\"EMS_EMMC_HIGH\",\"EMS_SSD_HIGH\"]");

    datahub_t hub;
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "test_host_old_name_%d", (int)getpid());
    char shm_path[96];
    snprintf(shm_path, sizeof(shm_path), "/edgeos_shadow_%s", shm_name);
    shm_unlink(shm_path);
    assert_int_equal(datahub_init(&hub, NULL, shm_name, 64, APP_BASE + 4096, NULL, 0, NULL, NULL,
                                  NULL),
                     0);
    datahub_set_tag_mapper_v2(&hub, iems_tag_map_fn);

    uint32_t tag_count = 0;
    const edge_tag_meta_t *product_tags = iems_get_tag_metadata(&tag_count);
    iems_app_host_ctx_t ctx = {
        .app_id = "demo",
        .hub = &hub,
        .db = db,
        .product_tags = product_tags,
        .product_tag_count = tag_count,
        .app_base = APP_BASE,
    };
    app_host_ports_t ports;
    iems_app_host_ports_init(&ports, &ctx);

    static const struct {
        const char *name;
        uint32_t tag_id;
        double value;
    } health[] = {
        {"EMS.EMSPower", IEMS_TAG_EMS_POWER, 1.0},
        {"EMS.EMSCpuHigh", IEMS_TAG_EMS_CPU_HIGH, 2.0},
        {"EMS.EMSMemHigh", IEMS_TAG_EMS_MEM_HIGH, 3.0},
        {"EMS.EmmcDiskHigh", IEMS_TAG_EMS_EMMC_HIGH, 4.0},
        {"EMS.SsdDiskHigh", IEMS_TAG_EMS_SSD_HIGH, 5.0},
    };
    char reason[160];
    for (size_t i = 0; i < sizeof(health) / sizeof(health[0]); i++) {
        pipe_msg_t seed;
        pipe_msg_init_scalar(&seed, health[i].tag_id, health[i].value,
                             7000000000ULL + (uint64_t)i * 1000000ULL);
        assert_int_equal(datahub_write_shadow(&hub, &seed), 0);
    }
    for (size_t i = 0; i < sizeof(health) / sizeof(health[0]); i++) {
        uint32_t handle = 0;
        uint8_t access = 0;
        assert_int_equal(ports.resolve(&ctx, health[i].name, &handle, &access, reason,
                                       sizeof(reason)),
                         0);
        assert_int_equal(handle, health[i].tag_id);
        assert_int_equal(access, APP_HOST_ACCESS_READ);

        app_host_sample_t sample;
        memset(&sample, 0, sizeof(sample));
        assert_int_equal(ports.read(&ctx, handle, &sample, reason, sizeof(reason)), 0);
        assert_float_equal(sample.number, health[i].value, 0.0);
        assert_int_equal(sample.quality, APP_HOST_QUALITY_GOOD);
        assert_int_equal(sample.source_ts, 7000ULL + i);
    }

    /* 14 条 canonical 短名解析优先级与 handle 不变。 */
    static const struct {
        const char *name;
        uint32_t tag_id;
    } short_names[] = {
        {"VOLTAGE_PHASE_A", 1001},   {"CURRENT_PHASE_A", 1002}, {"GRID_POWER", 1003},
        {"GRID_FREQUENCY", 1004},    {"ADC_RAW_DATA", 2001},    {"ADC_VOLTAGE_RMS", 2002},
        {"ENERGY_ACTIVE_IMP", 3001}, {"BATTERY_SOC", 4001},     {"BATTERY_CURRENT", 4002},
        {"BATTERY_POWER", 4003},     {"SOLAR_POWER", 8001},     {"SOLAR_VOLTS", 8003},
        {"SOLAR_RELAY", 8004},       {"PCS_CONTROL", 8005},
    };
    for (size_t i = 0; i < sizeof(short_names) / sizeof(short_names[0]); i++) {
        uint32_t handle = 0;
        uint8_t access = 0;
        assert_int_equal(ports.resolve(&ctx, short_names[i].name, &handle, &access, reason,
                                       sizeof(reason)),
                         0);
        assert_int_equal(handle, short_names[i].tag_id);
        assert_int_equal(access, APP_HOST_ACCESS_READ);
    }

    /* 旧名 handle 只读：publish 仍 denied_scope。 */
    app_host_sample_t publish;
    memset(&publish, 0, sizeof(publish));
    publish.kind = APP_HOST_VALUE_FLOAT;
    publish.number = 1.0;
    assert_int_equal(ports.publish(&ctx, IEMS_TAG_EMS_POWER, &publish, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "denied_scope");

    /* 8 条未映射旧名、未知名与大小写/空格/点号变体都 not_found，且不写输出。 */
    static const char *not_found[] = {
        "LC1.PCS.SmokeDetectStatus", "EMS.EMS.DcTotalVolUnusual", "EMS.ActivePower",
        "EMS.SetConnect",            "PCS.AphaseApparentPower",  "PCS_METER.AphaseActivePower",
        "EMS.EnIoProtect",           "pcs456.constant vol_chg_vol_setting",
        "EMS.NotARealPoint",         "ems.emspower",             "EMS.EMSPower ",
        " EMS.EMSPower",             "EMS_EMSPower",             "EMS EMSPower",
    };
    for (size_t i = 0; i < sizeof(not_found) / sizeof(not_found[0]); i++) {
        uint32_t handle = 0xDEADBEEFu;
        uint8_t access = 0xFF;
        assert_int_equal(ports.resolve(&ctx, not_found[i], &handle, &access, reason,
                                       sizeof(reason)),
                         -1);
        assert_string_equal(reason, "not_found");
        assert_int_equal(handle, 0xDEADBEEFu);
        assert_int_equal(access, 0xFF);
    }

    /* tag 在点模型里可查但产品元数据不存在时也不返回 handle。 */
    static const edge_tag_meta_t kUnrelatedTag[] = {{.id = 9999, .name = "OTHER"}};
    iems_app_host_ctx_t limited = ctx;
    limited.product_tags = kUnrelatedTag;
    limited.product_tag_count = 1;
    uint32_t handle = 0xDEADBEEFu;
    uint8_t access = 0xFF;
    assert_int_equal(ports.resolve(&limited, "VOLTAGE_PHASE_A", &handle, &access, reason,
                                   sizeof(reason)),
                     -1);
    assert_string_equal(reason, "not_found");
    assert_int_equal(handle, 0xDEADBEEFu);
    assert_int_equal(access, 0xFF);

    datahub_destroy(&hub);
    sqlite_svc_destroy(db);
    unlink(db_path);
    assert_int_equal(uv_loop_close(&loop), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_host_quota_denies_the_33rd),
        cmocka_unit_test(test_two_apps_fill_hub_then_third_denied),
        cmocka_unit_test(test_host_ports_old_name_resolution),
        cmocka_unit_test(test_host_ports_end_to_end),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
