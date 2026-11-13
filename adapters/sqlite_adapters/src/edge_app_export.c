#include "edgeos/edge_app_export.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "app_points_sqlite.h"
#include "app_repo_sqlite.h"
#include "edge_authz_sqlite.h"
#include "edge_digest.h"
#include "edge_tar.h"
#include "edgeos/edge_authz.h"

#define EXPORT_MAX_POINTS 256
#define EXPORT_MAX_GRANTS 64

static int fail(char *reason, size_t size, const char *text) {
    if (reason && size) snprintf(reason, size, "%s", text);
    return -1;
}

static char *manifest_json(const app_manifest_t *app) {
    cJSON *o = cJSON_CreateObject();
    if (!o) return NULL;
    cJSON_AddStringToObject(o, "id", app->id);
    cJSON_AddStringToObject(o, "name", app->name);
    cJSON_AddStringToObject(o, "version", app->version);
    cJSON_AddStringToObject(o, "author", app->author);
    cJSON_AddNumberToObject(o, "state", (double)app->state);
    cJSON_AddBoolToObject(o, "desired_state", app->desired_state);
    cJSON_AddNumberToObject(o, "fault_policy", (double)app->fault_policy);
    cJSON_AddNumberToObject(o, "priority", (double)app->priority);
    cJSON_AddBoolToObject(o, "keep_alive", app->keep_alive);
    cJSON_AddNumberToObject(o, "created_at", (double)app->created_at);
    char *json = cJSON_PrintUnformatted(o);
    cJSON_Delete(o);
    return json;
}

static char *points_json(sqlite_svc_t *db, const char *app_id, char *reason, size_t reason_size) {
    app_point_row_t *rows = calloc(EXPORT_MAX_POINTS, sizeof(*rows));
    if (!rows) return NULL;
    size_t count = 0;
    if (app_points_list(db, app_id, rows, EXPORT_MAX_POINTS, &count, reason, reason_size) != 0) {
        free(rows);
        return NULL;
    }
    cJSON *array = cJSON_CreateArray();
    if (!array) {
        free(rows);
        return NULL;
    }
    for (size_t i = 0; i < count; i++) {
        cJSON *o = cJSON_CreateObject();
        cJSON_AddStringToObject(o, "name", rows[i].name);
        cJSON_AddStringToObject(o, "unit", rows[i].unit);
        cJSON_AddNumberToObject(o, "slot", rows[i].slot);
        cJSON_AddNumberToObject(o, "type", rows[i].type);
        cJSON_AddNumberToObject(o, "contract_version", rows[i].contract_version);
        cJSON_AddNumberToObject(o, "retention", rows[i].retention);
        cJSON_AddNumberToObject(o, "stale_after_ms", rows[i].stale_after_ms);
        cJSON_AddNumberToObject(o, "stability", rows[i].stability);
        cJSON_AddNumberToObject(o, "source", rows[i].source);
        cJSON_AddItemToArray(array, o);
    }
    free(rows);
    char *json = cJSON_PrintUnformatted(array);
    cJSON_Delete(array);
    return json;
}

static char *grants_json(sqlite_svc_t *db, const char *app_id, char *reason,
                         size_t reason_size) {
    edge_app_grant_t *grants = calloc(EXPORT_MAX_GRANTS, sizeof(*grants));
    if (!grants) return NULL;
    edge_authz_store_t store = edge_authz_sqlite_store(db);
    size_t count = 0;
    if (!store.list_grants ||
        store.list_grants(store.ctx, app_id, grants, EXPORT_MAX_GRANTS, &count) != 0) {
        free(grants);
        if (reason && reason_size) snprintf(reason, reason_size, "grant list failed");
        return NULL;
    }
    cJSON *array = cJSON_CreateArray();
    if (!array) {
        free(grants);
        return NULL;
    }
    for (size_t i = 0; i < count; i++) {
        cJSON *o = cJSON_CreateObject();
        cJSON_AddStringToObject(o, "capability", grants[i].capability);
        cJSON_AddStringToObject(o, "scope", grants[i].scope_json);
        cJSON_AddNumberToObject(o, "state", (double)grants[i].state);
        cJSON_AddStringToObject(o, "purpose", grants[i].purpose);
        cJSON_AddNumberToObject(o, "expires_at", (double)grants[i].expires_at);
        cJSON_AddNumberToObject(o, "sensitivity_max", (double)grants[i].sensitivity_max);
        cJSON_AddStringToObject(o, "granted_by", grants[i].granted_by);
        cJSON_AddNumberToObject(o, "granted_at", (double)grants[i].granted_at);
        cJSON_AddNumberToObject(o, "revoked_at", (double)grants[i].revoked_at);
        cJSON_AddItemToArray(array, o);
    }
    free(grants);
    char *json = cJSON_PrintUnformatted(array);
    cJSON_Delete(array);
    return json;
}

int edge_app_export_archive(sqlite_svc_t *db, const char *app_id, const char *out_path,
                            char *reason, size_t reason_size) {
    if (!db || !app_id || !out_path) return fail(reason, reason_size, "missing argument");
    edge_app_repo_t repo = app_repo_sqlite(db);
    app_manifest_t app;
    if (!repo.get_app || repo.get_app(repo.ctx, app_id, &app) != 0) {
        return fail(reason, reason_size, "app not found");
    }
    if (app.wasm_path[0] == '\0') return fail(reason, reason_size, "package path is empty");

    char digest[65];
    unsigned long long size = 0;
    if (edge_sha256_file(app.wasm_path, digest, &size) != 0) {
        return fail(reason, reason_size, "package is unreadable");
    }

    char *manifest = manifest_json(&app);
    char *points = points_json(db, app_id, reason, reason_size);
    char *grants = grants_json(db, app_id, reason, reason_size);
    cJSON *package = cJSON_CreateObject();
    char *package_text = NULL;
    if (package) {
        cJSON_AddNumberToObject(package, "size", (double)size);
        cJSON_AddStringToObject(package, "sha256", digest);
        package_text = cJSON_PrintUnformatted(package);
        cJSON_Delete(package);
    }
    if (!manifest || !points || !grants || !package_text) {
        free(manifest);
        free(points);
        free(grants);
        free(package_text);
        return fail(reason, reason_size, "json build failed");
    }

    char tmp[512];
    if (snprintf(tmp, sizeof(tmp), "%s.tmp", out_path) >= (int)sizeof(tmp)) {
        free(manifest);
        free(points);
        free(grants);
        free(package_text);
        return fail(reason, reason_size, "path too long");
    }
    FILE *out = fopen(tmp, "wb");
    int rc = -1;
    if (out) {
        rc = 0;
        if (edge_tar_write_entry(out, "manifest.json", manifest, strlen(manifest)) != 0 ||
            edge_tar_write_entry(out, "points.json", points, strlen(points)) != 0 ||
            edge_tar_write_entry(out, "grants.json", grants, strlen(grants)) != 0 ||
            edge_tar_write_entry(out, "package.json", package_text, strlen(package_text)) != 0 ||
            edge_tar_finish(out) != 0 || fflush(out) != 0 || fsync(fileno(out)) != 0) {
            rc = -1;
        }
        fclose(out);
    }
    free(manifest);
    free(points);
    free(grants);
    free(package_text);
    if (rc != 0) {
        unlink(tmp);
        return fail(reason, reason_size, "archive write failed");
    }
    if (rename(tmp, out_path) != 0) {
        unlink(tmp);
        return fail(reason, reason_size, "archive rename failed");
    }
    return 0;
}
