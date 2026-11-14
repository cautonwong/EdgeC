/**
 * @file test_app_export.c
 * @brief App export archive contents on a real migrated database.
 */

#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "app_points_sqlite.h"
#include "app_repo_sqlite.h"
#include "edge_authz_sqlite.h"
#include "edge_digest.h"
#include "edge_secret_sqlite.h"
#include "edgeos/edge_app_export.h"
#include "edgeos/edge_authz.h"
#include "sqlite_service.h"

static const char *kDb = "test_app_export.db";
static const char *kWasmb = "test_app_export.wasm";
static const char *kOut = "test_app_export.tar";

static char *entry_data(const char *archive, const char *wanted) {
    FILE *f = fopen(archive, "rb");
    if (!f) return NULL;
    char header[512];
    char *found = NULL;
    while (fread(header, 1, sizeof(header), f) == sizeof(header)) {
        if (header[0] == '\0') break;
        char name[101];
        memcpy(name, header, 100);
        name[100] = '\0';
        char size_text[13];
        memcpy(size_text, header + 124, 12);
        size_text[12] = '\0';
        unsigned long long size = strtoull(size_text, NULL, 8);
        if (strcmp(name, wanted) == 0) {
            found = malloc((size_t)size + 1);
            if (!found) break;
            if (fread(found, 1, (size_t)size, f) != (size_t)size) {
                free(found);
                found = NULL;
            } else {
                found[size] = '\0';
            }
            break;
        }
        fseek(f, (long)((size + 511) / 512 * 512), SEEK_CUR);
    }
    fclose(f);
    return found;
}

static int archive_contains(const char *archive, const char *needle) {
    FILE *f = fopen(archive, "rb");
    if (!f) return 0;
    char buf[512];
    size_t got;
    int found = 0;
    while ((got = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (memmem(buf, got, needle, strlen(needle))) {
            found = 1;
            break;
        }
    }
    fclose(f);
    return found;
}

static void test_export_archive(void **state) {
    (void)state;
    unlink(kDb);
    unlink(kOut);
    FILE *wasm = fopen(kWasmb, "wb");
    assert_non_null(wasm);
    fputs("WASM-BYTES", wasm);
    fclose(wasm);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = kDb};
    sqlite_svc_t *db = sqlite_svc_create(&loop, &cfg);
    assert_non_null(db);

    edge_app_repo_t repo = app_repo_sqlite(db);
    app_manifest_t app;
    memset(&app, 0, sizeof(app));
    snprintf(app.id, sizeof(app.id), "export-app");
    snprintf(app.name, sizeof(app.name), "export app");
    snprintf(app.version, sizeof(app.version), "2.0.0");
    snprintf(app.wasm_path, sizeof(app.wasm_path), "%s", kWasmb);
    app.state = APP_STATE_RUNNING;
    assert_int_equal(repo.save_app(repo.ctx, &app), 0);

    app_point_spec_t point;
    memset(&point, 0, sizeof(point));
    snprintf(point.name, sizeof(point.name), "voltage");
    snprintf(point.unit, sizeof(point.unit), "V");
    point.type = 1;
    point.retention = 1;
    point.contract_version = 1;
    point.stale_after_ms = 5000;
    char reason[160];
    assert_int_equal(app_points_sync(db, "export-app", &point, 1, reason, sizeof(reason)), 0);

    edge_authz_store_t authz = edge_authz_sqlite_store(db);
    edge_app_grant_t grant;
    memset(&grant, 0, sizeof(grant));
    snprintf(grant.app_id, sizeof(grant.app_id), "export-app");
    snprintf(grant.capability, sizeof(grant.capability), "tags.read");
    snprintf(grant.scope_json, sizeof(grant.scope_json), "[\"U_A\"]");
    snprintf(grant.purpose, sizeof(grant.purpose), "export test");
    snprintf(grant.granted_by, sizeof(grant.granted_by), "tester");
    assert_int_equal(edge_authz_approve_grant(&authz, &grant, 1000), EDGE_AUTHZ_OK);

    edge_secret_store_t secrets = edge_secret_sqlite(db);
    assert_int_equal(edge_secret_set(&secrets, "export-app", "token", "SECRET-MARKER-42", 16,
                                     reason, sizeof(reason)),
                     EDGE_SECRET_OK);

    assert_int_equal(edge_app_export_archive(db, "export-app", kOut, reason, sizeof(reason)), 0);

    char *manifest = entry_data(kOut, "manifest.json");
    char *points = entry_data(kOut, "points.json");
    char *grants = entry_data(kOut, "grants.json");
    char *package = entry_data(kOut, "package.json");
    assert_non_null(manifest);
    assert_non_null(points);
    assert_non_null(grants);
    assert_non_null(package);
    assert_non_null(strstr(manifest, "\"id\":\"export-app\""));
    assert_non_null(strstr(manifest, "\"version\":\"2.0.0\""));
    assert_non_null(strstr(points, "\"name\":\"voltage\""));
    assert_non_null(strstr(grants, "\"capability\":\"tags.read\""));
    char digest[65];
    assert_int_equal(edge_sha256_file(kWasmb, digest, NULL), 0);
    assert_non_null(strstr(package, digest));
    assert_int_equal(archive_contains(kOut, "SECRET-MARKER-42"), 0);
    free(manifest);
    free(points);
    free(grants);
    free(package);

    sqlite_svc_destroy(db);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
    unlink(kDb);
    unlink(kWasmb);
    unlink(kOut);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_export_archive),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
