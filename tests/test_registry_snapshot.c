/**
 * @file test_registry_snapshot.c
 * @brief Registry generation, snapshot publication, and status JSON.
 */

#include <cmocka.h>
#include <cjson/cJSON.h>
#include <sqlite3.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "app_repo_sqlite.h"
#include "edge_registry_snapshot.h"
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

static char *_read_file(const char *path, size_t *length) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) {
        fclose(file);
        return NULL;
    }
    char *buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    size_t read = fread(buffer, 1, (size_t)size, file);
    buffer[read] = '\0';
    fclose(file);
    if (length) *length = read;
    return buffer;
}

static void test_snapshot_generation_and_status(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_registry_snapshot.db";
    const char *dir = "test_registry_snapshot_dir";
    unlink(db_path);
    char command[256];
    snprintf(command, sizeof(command), "rm -rf %s", dir);
    assert_int_equal(system(command), 0);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(loop, &cfg);
    assert_non_null(db);

    edge_app_repo_t repo = app_repo_sqlite(db);
    app_manifest_t app;
    memset(&app, 0, sizeof(app));
    snprintf(app.id, sizeof(app.id), "snap-app");
    snprintf(app.version, sizeof(app.version), "1.0.0");
    app.state = APP_STATE_RUNNING;
    app.desired_state = true;
    assert_int_equal(repo.save_app(repo.ctx, &app), 0);

    sqlite3 *handle = (sqlite3 *)sqlite_svc_get_db_handle(db);
    assert_non_null(handle);
    char *err = NULL;
    assert_int_equal(sqlite3_exec(handle,
                                  "INSERT INTO app_grant (app_id, capability, scope_json, state, "
                                  "purpose, expires_at, sensitivity_max, granted_by, granted_at, "
                                  "revoked_at) VALUES ('snap-app', 'tags.read', '[\"U_A\"]', 1, "
                                  "'display', 0, 1, 'admin', 1, 0);",
                                  NULL, NULL, &err),
                     SQLITE_OK);
    sqlite3_free(err);

    assert_int_equal(sqlite3_exec(handle,
                                  "INSERT INTO app_point (app_id, name, slot, type, unit, "
                                  "contract_version, retention, stale_after_ms, stability, source) "
                                  "VALUES ('snap-app', 'sag_count', 7, 0, 'count', 1, 0, 5000, 0, 1);",
                                  NULL, NULL, &err),
                     SQLITE_OK);
    sqlite3_free(err);

    char path[512];
    char reason[160];
    assert_int_equal(edge_registry_snapshot_publish(db, dir, path, sizeof(path), reason,
                                                    sizeof(reason)),
                     0);
    size_t length = 0;
    char *snapshot = _read_file(path, &length);
    assert_non_null(snapshot);
    cJSON *root = cJSON_Parse(snapshot);
    free(snapshot);
    assert_non_null(root);
    cJSON *generation = cJSON_GetObjectItem(root, "generation");
    assert_true(cJSON_IsNumber(generation));
    int first_generation = generation->valueint;
    assert_true(first_generation >= 3); /* app row, grant row, point row */
    cJSON *apps = cJSON_GetObjectItem(root, "apps");
    assert_true(cJSON_IsArray(apps));
    cJSON *first_app = cJSON_GetArrayItem(apps, 0);
    assert_string_equal(cJSON_GetObjectItem(first_app, "id")->valuestring, "snap-app");
    cJSON *points = cJSON_GetObjectItem(first_app, "points");
    assert_true(cJSON_IsArray(points));
    cJSON *point = cJSON_GetArrayItem(points, 0);
    assert_string_equal(cJSON_GetObjectItem(point, "name")->valuestring, "sag_count");
    assert_int_equal(cJSON_GetObjectItem(point, "slot")->valueint, 7);
    assert_string_equal(cJSON_GetObjectItem(point, "type")->valuestring, "number");
    assert_string_equal(cJSON_GetObjectItem(point, "unit")->valuestring, "count");
    assert_string_equal(cJSON_GetObjectItem(point, "retention")->valuestring, "latest");
    assert_string_equal(cJSON_GetObjectItem(point, "source")->valuestring, "derived");
    cJSON *grants = cJSON_GetObjectItem(first_app, "grants");
    assert_true(cJSON_IsArray(grants));
    assert_string_equal(cJSON_GetObjectItem(cJSON_GetArrayItem(grants, 0), "capability")->valuestring,
                        "tags.read");
    cJSON_Delete(root);

    char current_path[512];
    snprintf(current_path, sizeof(current_path), "%s/current", dir);
    char *current = _read_file(current_path, NULL);
    assert_non_null(current);
    assert_non_null(strstr(current, "gen-"));
    free(current);

    /* Any registry write advances the generation and the next publish. */
    app.state = APP_STATE_STOPPED;
    assert_int_equal(repo.save_app(repo.ctx, &app), 0);
    assert_int_equal(edge_registry_snapshot_publish(db, dir, path, sizeof(path), reason,
                                                    sizeof(reason)),
                     0);
    char *second = _read_file(path, NULL);
    assert_non_null(second);
    cJSON *second_root = cJSON_Parse(second);
    free(second);
    assert_non_null(second_root);
    int second_generation = cJSON_GetObjectItem(second_root, "generation")->valueint;
    assert_true(second_generation > first_generation);
    cJSON_Delete(second_root);

    char *status = edge_registry_status_json(db, NULL, 0);
    assert_non_null(status);
    cJSON *status_root = cJSON_Parse(status);
    free(status);
    assert_non_null(status_root);
    assert_int_equal(cJSON_GetObjectItem(status_root, "generation")->valueint, second_generation);
    assert_true(cJSON_IsNull(cJSON_GetObjectItem(status_root, "converged")));
    assert_string_equal(cJSON_GetObjectItem(status_root, "reason")->valuestring, "no_consumers");
    cJSON_Delete(status_root);

    /* A consumer behind the current generation is pending, not converged. */
    edge_registry_consumer_t behind = {0};
    snprintf(behind.name, sizeof(behind.name), "appctl");
    behind.applied_generation = 0;
    status = edge_registry_status_json(db, &behind, 1);
    assert_non_null(status);
    status_root = cJSON_Parse(status);
    free(status);
    assert_non_null(status_root);
    assert_false(cJSON_IsTrue(cJSON_GetObjectItem(status_root, "converged")));
    assert_string_equal(cJSON_GetObjectItem(status_root, "reason")->valuestring, "pending");
    cJSON *consumer = cJSON_GetArrayItem(cJSON_GetObjectItem(status_root, "consumers"), 0);
    assert_string_equal(cJSON_GetObjectItem(consumer, "name")->valuestring, "appctl");
    cJSON_Delete(status_root);

    /* Caught up and fresh converges; a stale consumer never does. */
    behind.applied_generation = (unsigned long long)second_generation;
    status = edge_registry_status_json(db, &behind, 1);
    assert_non_null(status);
    status_root = cJSON_Parse(status);
    free(status);
    assert_non_null(status_root);
    assert_true(cJSON_IsTrue(cJSON_GetObjectItem(status_root, "converged")));
    assert_string_equal(cJSON_GetObjectItem(status_root, "reason")->valuestring, "converged");
    cJSON_Delete(status_root);

    behind.stale = 1;
    status = edge_registry_status_json(db, &behind, 1);
    assert_non_null(status);
    status_root = cJSON_Parse(status);
    free(status);
    assert_non_null(status_root);
    assert_false(cJSON_IsTrue(cJSON_GetObjectItem(status_root, "converged")));
    cJSON_Delete(status_root);

    snprintf(command, sizeof(command), "rm -rf %s", dir);
    assert_int_equal(system(command), 0);
    sqlite_svc_destroy(db);
    unlink(db_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_snapshot_generation_and_status, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
