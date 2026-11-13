/**
 * @file test_iems_app_host_ports.c
 * @brief Host ports over real DataHub, app point slots and SQLite authz.
 */

#include <cmocka.h>
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
#include "datahub_priv.h"
#include "edge_app_host.h"
#include "edge_app_sdk.h"
#include "edge_authz_sqlite.h"
#include "edgeos/edge_authz.h"
#include "iems_app_host_ports.h"
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

static void child_phase_one(int fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);

    uint32_t handle = 0;
    const char *access = NULL;
    const char *err = edge_app_point_resolve(sdk, "U_A", &handle, &access);
    if (err || handle != 1001 || strcmp(access, "read") != 0) _exit(11);

    edge_app_sample_t sample;
    memset(&sample, 0, sizeof(sample));
    err = edge_app_point_read(sdk, 1001, &sample);
    if (err || sample.value.number != 220.5 || strcmp(sample.quality, "good") != 0) _exit(12);
    if (sample.source_ts != 1720000000000ULL) _exit(13);

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

static void grant(sqlite_svc_t *db, const char *capability, const char *scope_json) {
    edge_authz_store_t store = edge_authz_sqlite_store(db);
    edge_app_grant_t grant = {0};
    snprintf(grant.app_id, sizeof(grant.app_id), "demo");
    snprintf(grant.capability, sizeof(grant.capability), "%s", capability);
    snprintf(grant.scope_json, sizeof(grant.scope_json), "%s", scope_json);
    snprintf(grant.purpose, sizeof(grant.purpose), "test");
    assert_int_equal(edge_authz_approve_grant(&store, &grant, 1000), EDGE_AUTHZ_OK);
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
    pipe_msg_init_scalar(&seed, 1001, 220.5, 1720000000000ULL);
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

    edge_authz_store_t store = edge_authz_sqlite_store(db);
    assert_int_equal(edge_authz_revoke_grant(&store, "demo", "tags.read", 2000), EDGE_AUTHZ_OK);
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

    app_host_server_destroy(server);
    app_host_server_destroy(server2);
    for (int i = 0; i < 20; i++) uv_run(&loop, UV_RUN_NOWAIT);

    datahub_destroy(hub);
    free(hub);
    sqlite_svc_destroy(db);
    unlink(db_path);
    assert_int_equal(uv_loop_close(&loop), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_host_ports_end_to_end),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
