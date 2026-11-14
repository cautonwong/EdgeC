/**
 * @file edge_registry_snapshot.c
 * @brief Snapshot publication and status JSON for the app registry.
 */

#include "edge_registry_snapshot.h"

#include <errno.h>
#include <fcntl.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <cjson/cJSON.h>

#include "sqlite_service.h"

static const char *state_name(int state) {
    switch (state) {
    case 0: return "uninstalled";
    case 1: return "installed";
    case 2: return "running";
    case 3: return "stopped";
    case 4: return "faulted";
    default: return "unknown";
    }
}

static int mkdir_p(const char *path) {
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "%s", path);
    size_t length = strlen(buffer);
    if (length == 0 || length >= sizeof(buffer)) return -1;
    for (size_t i = 1; i < length; i++) {
        if (buffer[i] != '/') continue;
        buffer[i] = '\0';
        if (mkdir(buffer, 0755) != 0 && errno != EEXIST) return -1;
        buffer[i] = '/';
    }
    if (mkdir(buffer, 0755) != 0 && errno != EEXIST) return -1;
    return 0;
}

static int read_generation(sqlite3 *db, long long *out) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "SELECT value FROM registry_meta WHERE key = 'generation';", -1,
                           &stmt, NULL) != SQLITE_OK)
        return -1;
    int rc = sqlite3_step(stmt);
    long long value = 0;
    if (rc == SQLITE_ROW) value = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_ROW) return -1;
    *out = value;
    return 0;
}

static cJSON *build_grants(sqlite3 *db, const char *app_id) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,
                           "SELECT capability, scope_json, state, expires_at, revoked_at "
                           "FROM app_grant WHERE app_id = ? ORDER BY capability;",
                           -1, &stmt, NULL) != SQLITE_OK)
        return NULL;

    cJSON *grants = cJSON_CreateArray();
    if (!grants) {
        sqlite3_finalize(stmt);
        return NULL;
    }
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        cJSON *grant = cJSON_CreateObject();
        if (!grant) continue;
        const unsigned char *capability = sqlite3_column_text(stmt, 0);
        const unsigned char *scope = sqlite3_column_text(stmt, 1);
        cJSON_AddStringToObject(grant, "capability",
                                capability ? (const char *)capability : "");
        cJSON *scopes = scope ? cJSON_Parse((const char *)scope) : NULL;
        if (scopes) {
            cJSON_AddItemToObject(grant, "scopes", scopes);
        } else {
            cJSON_AddStringToObject(grant, "scope_json", scope ? (const char *)scope : "");
        }
        cJSON_AddNumberToObject(grant, "state", sqlite3_column_int(stmt, 2));
        cJSON_AddNumberToObject(grant, "expires_at", sqlite3_column_int64(stmt, 3));
        cJSON_AddNumberToObject(grant, "revoked_at", sqlite3_column_int64(stmt, 4));
        cJSON_AddItemToArray(grants, grant);
    }
    sqlite3_finalize(stmt);
    return grants;
}

static const char *point_type_name(int type) {
    switch (type) {
    case 1: return "integer";
    case 2: return "boolean";
    case 3: return "text";
    default: return "number";
    }
}

static const char *point_retention_name(int retention) {
    return retention == 1 ? "latest+history" : "latest";
}

static const char *point_stability_name(int stability) {
    return stability == 1 ? "experimental" : "stable";
}

static const char *point_source_name(int source) {
    return source == 1 ? "derived" : "measured";
}

static cJSON *build_points(sqlite3 *db, const char *app_id) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,
                           "SELECT name, slot, type, unit, contract_version, retention, "
                           "stale_after_ms, stability, source FROM app_point "
                           "WHERE app_id = ?1 ORDER BY slot;",
                           -1, &stmt, NULL) != SQLITE_OK)
        return NULL;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);

    cJSON *points = cJSON_CreateArray();
    if (!points) {
        sqlite3_finalize(stmt);
        return NULL;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(stmt, 0);
        const unsigned char *unit = sqlite3_column_text(stmt, 3);
        cJSON *point = cJSON_CreateObject();
        if (!point) continue;
        cJSON_AddStringToObject(point, "name", name ? (const char *)name : "");
        cJSON_AddNumberToObject(point, "slot", sqlite3_column_int(stmt, 1));
        cJSON_AddStringToObject(point, "type", point_type_name(sqlite3_column_int(stmt, 2)));
        cJSON_AddStringToObject(point, "unit", unit ? (const char *)unit : "");
        cJSON_AddNumberToObject(point, "contract_version", sqlite3_column_int(stmt, 4));
        cJSON_AddStringToObject(point, "retention",
                                point_retention_name(sqlite3_column_int(stmt, 5)));
        cJSON_AddNumberToObject(point, "stale_after_ms", sqlite3_column_int(stmt, 6));
        cJSON_AddStringToObject(point, "stability",
                                point_stability_name(sqlite3_column_int(stmt, 7)));
        cJSON_AddStringToObject(point, "source", point_source_name(sqlite3_column_int(stmt, 8)));
        cJSON_AddItemToArray(points, point);
    }
    sqlite3_finalize(stmt);
    return points;
}

static cJSON *build_apps(sqlite3 *db, int with_grants, const char *app_id) {
    sqlite3_stmt *stmt;
    const char *sql = app_id ? "SELECT id, version, state, priority, desired_state, prev_version "
                               "FROM app_registry WHERE id = ?1 ORDER BY id;"
                             : "SELECT id, version, state, priority, desired_state, prev_version "
                               "FROM app_registry ORDER BY id;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return NULL;
    if (app_id) sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    cJSON *apps = cJSON_CreateArray();
    if (!apps) {
        sqlite3_finalize(stmt);
        return NULL;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *id = sqlite3_column_text(stmt, 0);
        const unsigned char *version = sqlite3_column_text(stmt, 1);
        cJSON *app = cJSON_CreateObject();
        if (!app) continue;
        cJSON_AddStringToObject(app, "id", id ? (const char *)id : "");
        cJSON_AddStringToObject(app, "version", version ? (const char *)version : "");
        cJSON_AddStringToObject(app, "state", state_name(sqlite3_column_int(stmt, 2)));
        cJSON_AddNumberToObject(app, "priority", sqlite3_column_int(stmt, 3));
        cJSON_AddBoolToObject(app, "desired_state", sqlite3_column_int(stmt, 4) != 0);
        const unsigned char *prev = sqlite3_column_text(stmt, 5);
        cJSON_AddStringToObject(app, "prev_version", prev ? (const char *)prev : "");
        if (with_grants && id) {
            cJSON *grants = build_grants(db, (const char *)id);
            if (grants) cJSON_AddItemToObject(app, "grants", grants);
        }
        if (id) {
            cJSON *points = build_points(db, (const char *)id);
            if (points) cJSON_AddItemToObject(app, "points", points);
        }
        cJSON_AddItemToArray(apps, app);
    }
    sqlite3_finalize(stmt);
    return apps;
}

static int write_file_atomic(const char *path, const char *data, size_t length) {
    char tmp_path[512];
    if (strlen(path) + 5 > sizeof(tmp_path)) return -1;
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
    FILE *file = fopen(tmp_path, "wb");
    if (!file) return -1;
    size_t written = fwrite(data, 1, length, file);
    int rc = written == length ? 0 : -1;
    if (rc == 0 && fflush(file) != 0) rc = -1;
    if (rc == 0 && fsync(fileno(file)) != 0) rc = -1;
    if (fclose(file) != 0) rc = -1;
    if (rc != 0) {
        unlink(tmp_path);
        return -1;
    }
    if (rename(tmp_path, path) != 0) {
        unlink(tmp_path);
        return -1;
    }
    return 0;
}

int edge_registry_snapshot_publish(sqlite_svc_t *db, const char *dir, char *out_path,
                                   size_t path_cap, char *reason, size_t reason_cap) {
    if (reason && reason_cap) reason[0] = '\0';
    if (!db || !dir || !dir[0]) {
        if (reason && reason_cap) snprintf(reason, reason_cap, "invalid args");
        return -1;
    }
    sqlite3 *handle = (sqlite3 *)sqlite_svc_get_db_handle(db);
    if (!handle) {
        if (reason && reason_cap) snprintf(reason, reason_cap, "database unavailable");
        return -1;
    }
    if (strlen(dir) > 400) {
        if (reason && reason_cap) snprintf(reason, reason_cap, "snapshot directory too long");
        return -1;
    }

    long long generation = 0;
    if (read_generation(handle, &generation) != 0) {
        if (reason && reason_cap) snprintf(reason, reason_cap, "generation missing");
        return -1;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *apps = build_apps(handle, 1, NULL);
    if (!root || !apps) {
        cJSON_Delete(root);
        cJSON_Delete(apps);
        if (reason && reason_cap) snprintf(reason, reason_cap, "out of memory");
        return -1;
    }
    cJSON_AddNumberToObject(root, "generation", (double)generation);
    cJSON_AddNumberToObject(root, "generated_at", (double)time(NULL));
    cJSON_AddItemToObject(root, "apps", apps);
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) {
        if (reason && reason_cap) snprintf(reason, reason_cap, "out of memory");
        return -1;
    }

    if (mkdir_p(dir) != 0) {
        free(json);
        if (reason && reason_cap) snprintf(reason, reason_cap, "snapshot directory failed");
        return -1;
    }

    char name[128];
    char path[512];
    snprintf(name, sizeof(name), "gen-%lld.json", generation);
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    if (write_file_atomic(path, json, strlen(json)) != 0) {
        free(json);
        if (reason && reason_cap) snprintf(reason, reason_cap, "snapshot write failed");
        return -1;
    }
    free(json);

    char current[512];
    snprintf(current, sizeof(current), "%s/current", dir);
    if (write_file_atomic(current, name, strlen(name)) != 0) {
        if (reason && reason_cap) snprintf(reason, reason_cap, "current pointer failed");
        return -1;
    }

    if (out_path && path_cap) snprintf(out_path, path_cap, "%s", path);
    return 0;
}

char *edge_registry_status_json_for(sqlite_svc_t *db, const edge_registry_consumer_t *consumers,
                                    size_t consumer_count, const char *app_id) {
    if (!db) return NULL;
    sqlite3 *handle = (sqlite3 *)sqlite_svc_get_db_handle(db);
    if (!handle) return NULL;

    long long generation = 0;
    if (read_generation(handle, &generation) != 0) return NULL;

    cJSON *root = cJSON_CreateObject();
    cJSON *apps = build_apps(handle, 0, app_id);
    cJSON *consumer_array = cJSON_CreateArray();
    if (!root || !apps || !consumer_array) {
        cJSON_Delete(root);
        cJSON_Delete(apps);
        cJSON_Delete(consumer_array);
        return NULL;
    }

    int converged = 1;
    for (size_t i = 0; i < consumer_count; i++) {
        cJSON *item = cJSON_CreateObject();
        if (!item) continue;
        cJSON_AddStringToObject(item, "name", consumers[i].name);
        cJSON_AddNumberToObject(item, "applied_generation",
                                (double)consumers[i].applied_generation);
        cJSON_AddBoolToObject(item, "stale", consumers[i].stale != 0);
        cJSON_AddItemToArray(consumer_array, item);
        if (consumers[i].stale ||
            consumers[i].applied_generation < (unsigned long long)generation) {
            converged = 0;
        }
    }

    cJSON_AddNumberToObject(root, "generation", (double)generation);
    cJSON_AddItemToObject(root, "apps", apps);
    cJSON_AddItemToObject(root, "consumers", consumer_array);
    if (consumer_count == 0) {
        cJSON_AddNullToObject(root, "converged");
        cJSON_AddStringToObject(root, "reason", "no_consumers");
    } else {
        cJSON_AddBoolToObject(root, "converged", converged != 0);
        cJSON_AddStringToObject(root, "reason", converged ? "converged" : "pending");
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

char *edge_registry_status_json(sqlite_svc_t *db, const edge_registry_consumer_t *consumers,
                                size_t consumer_count) {
    return edge_registry_status_json_for(db, consumers, consumer_count, NULL);
}

int edge_registry_generation(sqlite_svc_t *db, unsigned long long *out) {
    if (!db || !out) return -1;
    sqlite3 *handle = (sqlite3 *)sqlite_svc_get_db_handle(db);
    if (!handle) return -1;
    long long generation = 0;
    if (read_generation(handle, &generation) != 0) return -1;
    *out = (unsigned long long)generation;
    return 0;
}
