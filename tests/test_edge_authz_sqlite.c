/**
 * @file test_edge_authz_sqlite.c
 * @brief Round-trip tests for the SQLite authorization repository.
 */

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

#include "edge_authz_sqlite.h"
#include "sqlite_service.h"

static void _on_close_cb(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int setup(void **state) {
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    *state = loop;
    return 0;
}

static int teardown(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    uv_walk(loop, _on_close_cb, NULL);
    while (uv_loop_close(loop) == UV_EBUSY) uv_run(loop, UV_RUN_ONCE);
    free(loop);
    return 0;
}

static void _exec(sqlite3 *db, const char *sql) {
    char *err = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) fprintf(stderr, "sqlite exec failed: %s\n", err ? err : sql);
    assert_int_equal(rc, SQLITE_OK);
    sqlite3_free(err);
}

static sqlite_svc_t *_open(uv_loop_t *loop, const char *path) {
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    return svc;
}

static void test_sqlite_authz_roundtrip_and_revoke(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_edge_authz.db";
    unlink(path);

    sqlite_svc_t *svc = _open(loop, path);
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    _exec(db,
          "INSERT INTO app_grant (app_id, capability, scope_json, state, purpose, expires_at, "
          "sensitivity_max, granted_by, granted_at, revoked_at) "
          "VALUES ('app.demo', 'tags.read', '[\"U_A\"]', 1, 'display', 0, 3, 'admin', 100, 0);"
          "INSERT INTO point_profile VALUES ('U_A', 2);"
          "INSERT INTO publisher_trust VALUES ('app.demo', 2, 1);");

    edge_authz_repo_t repo = edge_authz_sqlite(svc);
    edge_authz_engine_t engine;
    edge_authz_engine_init(&engine, &repo);
    edge_authz_engine_set_enforcement(&engine, true);

    edge_app_grant_t grant;
    assert_int_equal(repo.get_grant(repo.ctx, "app.demo", "tags.read", &grant), 1);
    assert_string_equal(grant.app_id, "app.demo");
    assert_string_equal(grant.scope_json, "[\"U_A\"]");
    assert_int_equal(grant.state, EDGE_GRANT_GRANTED);
    assert_string_equal(grant.purpose, "display");
    assert_int_equal(grant.sensitivity_max, 3);
    assert_int_equal(grant.granted_at, 100);

    edge_point_profile_t point;
    assert_int_equal(repo.get_point_profile(repo.ctx, "U_A", &point), 1);
    assert_int_equal(point.sensitivity, 2);

    edge_publisher_trust_t trust;
    assert_int_equal(repo.get_publisher_trust(repo.ctx, "app.demo", &trust), 1);
    assert_int_equal(trust.trust_level, 2);
    assert_true(trust.verified);

    assert_int_equal(edge_authz_check(&engine, "app.demo", "tags.read", "U_A", 200),
                     EDGE_AUTHZ_OK);
    assert_int_equal(edge_authz_check(&engine, "app.demo", "tags.read", "U_B", 200),
                     EDGE_AUTHZ_DENIED_SCOPE);

    _exec(db, "UPDATE app_grant SET revoked_at = 300 WHERE app_id = 'app.demo';");
    assert_int_equal(edge_authz_check(&engine, "app.demo", "tags.read", "U_A", 301),
                     EDGE_AUTHZ_DENIED_REVOKED);

    assert_int_equal(repo.get_grant(repo.ctx, "app.demo", "missing", &grant), 0);

    sqlite_svc_destroy(svc);
    unlink(path);
}

static void test_sqlite_authz_absent_rows(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_edge_authz_absent.db";
    unlink(path);

    sqlite_svc_t *svc = _open(loop, path);
    edge_authz_repo_t repo = edge_authz_sqlite(svc);

    edge_authz_engine_t engine;
    edge_authz_engine_init(&engine, &repo);
    edge_authz_engine_set_enforcement(&engine, true);
    assert_int_equal(edge_authz_check(&engine, "app.demo", "tags.read", "U_A", 100),
                     EDGE_AUTHZ_DENIED_NOT_GRANTED);

    edge_app_grant_t grant;
    assert_int_equal(repo.get_grant(repo.ctx, "app.demo", "tags.read", &grant), 0);
    edge_point_profile_t point;
    assert_int_equal(repo.get_point_profile(repo.ctx, "U_A", &point), 0);
    edge_publisher_trust_t trust;
    assert_int_equal(repo.get_publisher_trust(repo.ctx, "app.demo", &trust), 0);

    sqlite_svc_destroy(svc);
    unlink(path);
}

static void test_sqlite_store_admin(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_edge_authz_store.db";
    unlink(path);

    sqlite_svc_t *svc = _open(loop, path);
    edge_authz_store_t store = edge_authz_sqlite_store(svc);

    edge_app_grant_t g;
    memset(&g, 0, sizeof(g));
    snprintf(g.app_id, sizeof(g.app_id), "app.demo");
    snprintf(g.capability, sizeof(g.capability), "tags.read");
    snprintf(g.scope_json, sizeof(g.scope_json), "[\"U_A\",\"U_B\"]");
    snprintf(g.purpose, sizeof(g.purpose), "display");
    g.sensitivity_max = 1;

    assert_int_equal(edge_authz_approve_grant(&store, &g, 100), EDGE_AUTHZ_OK);

    edge_app_grant_t out;
    assert_int_equal(store.get_grant(store.ctx, "app.demo", "tags.read", &out), 1);
    assert_int_equal(out.state, EDGE_GRANT_GRANTED);
    assert_int_equal(out.granted_at, 100);

    edge_app_grant_t listed[4];
    size_t count = 0;
    assert_int_equal(store.list_grants(store.ctx, "app.demo", listed, 4, &count), 0);
    assert_int_equal(count, 1);
    assert_string_equal(listed[0].capability, "tags.read");

    assert_int_equal(edge_authz_revoke_grant(&store, "app.demo", "tags.read", 200),
                     EDGE_AUTHZ_OK);
    assert_int_equal(store.get_grant(store.ctx, "app.demo", "tags.read", &out), 1);
    assert_int_equal(out.revoked_at, 200);
    assert_int_equal(edge_authz_revoke_grant(&store, "app.demo", "tags.read", 300),
                     EDGE_AUTHZ_OK);
    assert_int_equal(store.get_grant(store.ctx, "app.demo", "tags.read", &out), 1);
    assert_int_equal(out.revoked_at, 200);

    assert_int_equal(
        edge_authz_inherit_upgrade(&store, "app.demo", "tags.read", "[\"U_A\"]", 400),
        EDGE_AUTHZ_INHERIT_REVOKED);
    assert_int_equal(store.get_grant(store.ctx, "app.demo", "tags.read", &out), 1);
    assert_int_equal(out.revoked_at, 200);

    assert_int_equal(
        edge_authz_inherit_upgrade(&store, "app.demo", "alarms.raise", "[\"power\"]", 500),
        EDGE_AUTHZ_INHERIT_AWAITING);
    assert_int_equal(store.get_grant(store.ctx, "app.demo", "alarms.raise", &out), 1);
    assert_int_equal(out.state, EDGE_GRANT_PENDING);

    sqlite_svc_destroy(svc);
    unlink(path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_sqlite_authz_roundtrip_and_revoke, setup, teardown),
        cmocka_unit_test_setup_teardown(test_sqlite_authz_absent_rows, setup, teardown),
        cmocka_unit_test_setup_teardown(test_sqlite_store_admin, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
