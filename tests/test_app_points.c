/**
 * @file test_app_points.c
 * @brief App point contracts: slot allocation, upgrades, reclamation, reconcile.
 */

#include <cmocka.h>
#include <sqlite3.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "app_points_sqlite.h"
#include "sqlite_service.h"

typedef struct {
    uv_loop_t loop;
} fixture_t;

static fixture_t g_fixture;

static sqlite_svc_t *make_db(uv_loop_t *loop, const char *path) {
    unlink(path);
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    return sqlite_svc_create(loop, &cfg);
}

static void add_app(sqlite_svc_t *db, const char *app_id, int state) {
    sqlite3 *handle = (sqlite3 *)sqlite_svc_get_db_handle(db);
    sqlite3_stmt *stmt;
    assert_int_equal(sqlite3_prepare_v2(handle,
                                        "INSERT INTO app_registry(id, version, state) "
                                        "VALUES(?1, '1.0.0', ?2);",
                                        -1, &stmt, NULL),
                     SQLITE_OK);
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, state);
    assert_int_equal(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);
}

static void make_point(app_point_spec_t *point, const char *name, int type, int version,
                       const char *unit) {
    memset(point, 0, sizeof(*point));
    snprintf(point->name, sizeof(point->name), "%s", name);
    point->type = (uint8_t)type;
    point->contract_version = (uint16_t)version;
    snprintf(point->unit, sizeof(point->unit), "%s", unit);
    point->stale_after_ms = APP_SPEC_POINT_STALE_DEFAULT_MS;
}

static long long generation(sqlite_svc_t *db) {
    sqlite3 *handle = (sqlite3 *)sqlite_svc_get_db_handle(db);
    sqlite3_stmt *stmt;
    assert_int_equal(
        sqlite3_prepare_v2(handle, "SELECT value FROM registry_meta WHERE key='generation';", -1,
                           &stmt, NULL),
        SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    long long value = atoll((const char *)sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    return value;
}

static void test_allocates_contiguous_slots(void **state) {
    (void)state;
    sqlite_svc_t *db = make_db(&g_fixture.loop, "/tmp/edgeos_points_a.db");
    assert_non_null(db);

    app_point_spec_t points[2];
    make_point(&points[0], "sag_count", APP_POINT_NUMBER, 1, "count");
    make_point(&points[1], "last_sag_ms", APP_POINT_INTEGER, 1, "ms");

    char reason[160];
    long long before = generation(db);
    assert_int_equal(app_points_sync(db, "demo", points, 2, reason, sizeof(reason)), 0);
    assert_true(generation(db) > before);

    app_point_row_t rows[4];
    size_t count = 0;
    assert_int_equal(app_points_list(db, "demo", rows, 4, &count, reason, sizeof(reason)), 0);
    assert_int_equal(count, 2);
    assert_string_equal(rows[0].name, "sag_count");
    assert_int_equal(rows[0].slot, 0);
    assert_string_equal(rows[0].unit, "count");
    assert_int_equal(rows[0].type, APP_POINT_NUMBER);
    assert_int_equal(rows[1].slot, 1);

    app_point_spec_t other[1];
    make_point(&other[0], "value", APP_POINT_NUMBER, 1, "");
    assert_int_equal(app_points_sync(db, "second", other, 1, reason, sizeof(reason)), 0);
    assert_int_equal(app_points_list(db, "second", rows, 4, &count, reason, sizeof(reason)), 0);
    assert_int_equal(count, 1);
    assert_int_equal(rows[0].slot, APP_POINT_SLOT_BLOCK);

    sqlite_svc_destroy(db);
    unlink("/tmp/edgeos_points_a.db");
}

static void test_upgrade_preserves_and_reuses_slots(void **state) {
    (void)state;
    sqlite_svc_t *db = make_db(&g_fixture.loop, "/tmp/edgeos_points_b.db");
    assert_non_null(db);

    app_point_spec_t first[3];
    make_point(&first[0], "a", APP_POINT_NUMBER, 1, "");
    make_point(&first[1], "b", APP_POINT_NUMBER, 1, "");
    make_point(&first[2], "c", APP_POINT_NUMBER, 1, "");
    char reason[160];
    assert_int_equal(app_points_sync(db, "demo", first, 3, reason, sizeof(reason)), 0);

    app_point_spec_t second[3];
    make_point(&second[0], "a", APP_POINT_NUMBER, 1, "");
    make_point(&second[1], "d", APP_POINT_NUMBER, 1, "");
    make_point(&second[2], "b", APP_POINT_NUMBER, 1, "");
    assert_int_equal(app_points_sync(db, "demo", second, 3, reason, sizeof(reason)), 0);

    app_point_row_t rows[4];
    size_t count = 0;
    assert_int_equal(app_points_list(db, "demo", rows, 4, &count, reason, sizeof(reason)), 0);
    assert_int_equal(count, 3);
    assert_string_equal(rows[0].name, "a");
    assert_int_equal(rows[0].slot, 0);
    assert_string_equal(rows[1].name, "b");
    assert_int_equal(rows[1].slot, 1);
    assert_string_equal(rows[2].name, "d");
    assert_int_equal(rows[2].slot, 2); /* c freed slot 2 */

    sqlite_svc_destroy(db);
    unlink("/tmp/edgeos_points_b.db");
}

static void test_rejects_contract_change(void **state) {
    (void)state;
    sqlite_svc_t *db = make_db(&g_fixture.loop, "/tmp/edgeos_points_c.db");
    assert_non_null(db);

    app_point_spec_t point[1];
    make_point(&point[0], "sag_count", APP_POINT_NUMBER, 1, "count");
    char reason[160];
    assert_int_equal(app_points_sync(db, "demo", point, 1, reason, sizeof(reason)), 0);

    make_point(&point[0], "sag_count", APP_POINT_INTEGER, 1, "count");
    assert_int_equal(app_points_sync(db, "demo", point, 1, reason, sizeof(reason)), -1);
    assert_non_null(strstr(reason, "point contract changed"));

    make_point(&point[0], "sag_count", APP_POINT_NUMBER, 2, "count");
    assert_int_equal(app_points_sync(db, "demo", point, 1, reason, sizeof(reason)), -1);
    assert_non_null(strstr(reason, "point contract changed"));

    make_point(&point[0], "sag_count", APP_POINT_NUMBER, 1, "events");
    assert_int_equal(app_points_sync(db, "demo", point, 1, reason, sizeof(reason)), -1);
    assert_non_null(strstr(reason, "point contract changed"));

    sqlite_svc_destroy(db);
    unlink("/tmp/edgeos_points_c.db");
}

static void test_release_and_reuse(void **state) {
    (void)state;
    sqlite_svc_t *db = make_db(&g_fixture.loop, "/tmp/edgeos_points_d.db");
    assert_non_null(db);

    app_point_spec_t point[1];
    make_point(&point[0], "value", APP_POINT_NUMBER, 1, "");
    char reason[160];
    assert_int_equal(app_points_sync(db, "demo", point, 1, reason, sizeof(reason)), 0);
    assert_int_equal(app_points_release(db, "demo"), 0);

    app_point_row_t rows[1];
    size_t count = 0;
    assert_int_equal(app_points_list(db, "demo", rows, 1, &count, reason, sizeof(reason)), 0);
    assert_int_equal(count, 0);

    assert_int_equal(app_points_sync(db, "other", point, 1, reason, sizeof(reason)), 0);
    assert_int_equal(app_points_list(db, "other", rows, 1, &count, reason, sizeof(reason)), 0);
    assert_int_equal(rows[0].slot, 0);

    sqlite_svc_destroy(db);
    unlink("/tmp/edgeos_points_d.db");
}

static void test_slot_region_exhaustion(void **state) {
    (void)state;
    sqlite_svc_t *db = make_db(&g_fixture.loop, "/tmp/edgeos_points_e.db");
    assert_non_null(db);

    app_point_spec_t point[1];
    make_point(&point[0], "value", APP_POINT_NUMBER, 1, "");
    char reason[160];
    char app_id[32];
    for (int i = 0; i < APP_POINT_SLOT_BLOCKS; i++) {
        snprintf(app_id, sizeof(app_id), "app-%d", i);
        assert_int_equal(app_points_sync(db, app_id, point, 1, reason, sizeof(reason)), 0);
    }
    assert_int_equal(app_points_sync(db, "app-overflow", point, 1, reason, sizeof(reason)), -1);
    assert_string_equal(reason, "slot region exhausted");

    sqlite_svc_destroy(db);
    unlink("/tmp/edgeos_points_e.db");
}

static void test_reconcile_uninstalled(void **state) {
    (void)state;
    sqlite_svc_t *db = make_db(&g_fixture.loop, "/tmp/edgeos_points_f.db");
    assert_non_null(db);

    add_app(db, "kept", 1);
    add_app(db, "gone", 0);
    app_point_spec_t point[1];
    make_point(&point[0], "value", APP_POINT_NUMBER, 1, "");
    char reason[160];
    assert_int_equal(app_points_sync(db, "kept", point, 1, reason, sizeof(reason)), 0);
    assert_int_equal(app_points_sync(db, "gone", point, 1, reason, sizeof(reason)), 0);
    assert_int_equal(app_points_sync(db, "orphan", point, 1, reason, sizeof(reason)), 0);

    size_t released = 0;
    assert_int_equal(app_points_reconcile(db, &released, reason, sizeof(reason)), 0);
    assert_true(released >= 2);

    app_point_row_t rows[2];
    size_t count = 0;
    assert_int_equal(app_points_list(db, "kept", rows, 1, &count, reason, sizeof(reason)), 0);
    assert_int_equal(count, 1);
    assert_int_equal(app_points_list(db, "gone", rows, 1, &count, reason, sizeof(reason)), 0);
    assert_int_equal(count, 0);
    assert_int_equal(app_points_list(db, "orphan", rows, 1, &count, reason, sizeof(reason)), 0);
    assert_int_equal(count, 0);

    sqlite_svc_destroy(db);
    unlink("/tmp/edgeos_points_f.db");
}

static void test_check_is_side_effect_free(void **state) {
    (void)state;
    sqlite_svc_t *db = make_db(&g_fixture.loop, "/tmp/edgeos_points_g.db");
    assert_non_null(db);

    app_point_spec_t point[1];
    make_point(&point[0], "sag_count", APP_POINT_NUMBER, 1, "count");
    char reason[160];
    assert_int_equal(app_points_check(db, "demo", point, 1, reason, sizeof(reason)), 0);
    app_point_row_t rows[1];
    size_t count = 0;
    assert_int_equal(app_points_list(db, "demo", rows, 1, &count, reason, sizeof(reason)), 0);
    assert_int_equal(count, 0);

    assert_int_equal(app_points_sync(db, "demo", point, 1, reason, sizeof(reason)), 0);
    make_point(&point[0], "sag_count", APP_POINT_TEXT, 1, "count");
    assert_int_equal(app_points_check(db, "demo", point, 1, reason, sizeof(reason)), -1);
    assert_non_null(strstr(reason, "point contract changed"));

    sqlite_svc_destroy(db);
    unlink("/tmp/edgeos_points_g.db");
}

int main(void) {
    memset(&g_fixture, 0, sizeof(g_fixture));
    assert_int_equal(uv_loop_init(&g_fixture.loop), 0);

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_allocates_contiguous_slots),
        cmocka_unit_test(test_upgrade_preserves_and_reuses_slots),
        cmocka_unit_test(test_rejects_contract_change),
        cmocka_unit_test(test_release_and_reuse),
        cmocka_unit_test(test_slot_region_exhaustion),
        cmocka_unit_test(test_reconcile_uninstalled),
        cmocka_unit_test(test_check_is_side_effect_free),
    };
    int rc = cmocka_run_group_tests(tests, NULL, NULL);
    uv_run(&g_fixture.loop, UV_RUN_DEFAULT);
    uv_loop_close(&g_fixture.loop);
    return rc;
}
