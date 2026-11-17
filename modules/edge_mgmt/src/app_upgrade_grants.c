#include "app_upgrade_grants.h"

#include <cjson/cJSON.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GRANT_SNAPSHOT_MAX (64 * 1024)

static void set_reason(char *reason, size_t reason_cap, const char *text) {
    if (reason && reason_cap) snprintf(reason, reason_cap, "%s", text);
}

static int write_file_sync(const char *path, const void *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t written = fwrite(data, 1, len, f);
    int rc = fflush(f);
    if (rc == 0) rc = fsync(fileno(f));
    if (fclose(f) != 0) rc = -1;
    return (written == len && rc == 0) ? 0 : -1;
}

static void fsync_parent(const char *path) {
    char dir[512];
    int n = snprintf(dir, sizeof(dir), "%s", path);
    if (n < 0 || (size_t)n >= sizeof(dir)) return;
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
    } else {
        snprintf(dir, sizeof(dir), ".");
    }
    int fd = open(dir, O_RDONLY | O_DIRECTORY);
    if (fd >= 0) {
        (void)fsync(fd);
        close(fd);
    }
}

static void json_read_string(const cJSON *o, const char *key, char *out, size_t cap) {
    const cJSON *item = cJSON_GetObjectItem(o, key);
    if (cJSON_IsString(item) && item->valuestring) {
        snprintf(out, cap, "%s", item->valuestring);
    }
}

static void json_read_uint64(const cJSON *o, const char *key, uint64_t *out) {
    const cJSON *item = cJSON_GetObjectItem(o, key);
    if (cJSON_IsNumber(item)) *out = (uint64_t)item->valuedouble;
}

void app_upgrade_grants_path(const char *target, char *out, size_t cap) {
    int room = cap > 16 ? (int)(cap - 16) : 0; /* room for ".pending.grants" + NUL */
    snprintf(out, cap, "%.*s.pending.grants", room, target);
}

void app_upgrade_grants_done_path(const char *target, char *out, size_t cap) {
    int room = cap > 21 ? (int)(cap - 21) : 0; /* room for ".pending.grants.done" + NUL */
    snprintf(out, cap, "%.*s.pending.grants.done", room, target);
}

void app_upgrade_markers_remove(const char *target) {
    char pending[272];
    char grants[288];
    char done[296];
    int room = sizeof(pending) > 9 ? (int)(sizeof(pending) - 9) : 0;
    snprintf(pending, sizeof(pending), "%.*s.pending", room, target);
    app_upgrade_grants_path(target, grants, sizeof(grants));
    app_upgrade_grants_done_path(target, done, sizeof(done));
    unlink(pending);
    unlink(grants);
    unlink(done);
    fsync_parent(pending);
}

static cJSON *grant_json(const edge_app_grant_t *g) {
    cJSON *o = cJSON_CreateObject();
    if (!o) return NULL;
    cJSON_AddStringToObject(o, "app_id", g->app_id);
    cJSON_AddStringToObject(o, "capability", g->capability);
    cJSON_AddStringToObject(o, "scope_json", g->scope_json);
    cJSON_AddNumberToObject(o, "state", (double)g->state);
    cJSON_AddStringToObject(o, "purpose", g->purpose);
    cJSON_AddNumberToObject(o, "expires_at", (double)g->expires_at);
    cJSON_AddNumberToObject(o, "sensitivity_max", (double)g->sensitivity_max);
    cJSON_AddStringToObject(o, "granted_by", g->granted_by);
    cJSON_AddNumberToObject(o, "granted_at", (double)g->granted_at);
    cJSON_AddNumberToObject(o, "revoked_at", (double)g->revoked_at);
    return o;
}

static int grant_from_json(const cJSON *o, edge_app_grant_t *g) {
    memset(g, 0, sizeof(*g));
    json_read_string(o, "app_id", g->app_id, sizeof(g->app_id));
    json_read_string(o, "capability", g->capability, sizeof(g->capability));
    json_read_string(o, "scope_json", g->scope_json, sizeof(g->scope_json));
    json_read_string(o, "purpose", g->purpose, sizeof(g->purpose));
    json_read_string(o, "granted_by", g->granted_by, sizeof(g->granted_by));
    uint64_t number = 0;
    json_read_uint64(o, "state", &number);
    g->state = (edge_grant_state_t)number;
    json_read_uint64(o, "expires_at", &g->expires_at);
    json_read_uint64(o, "sensitivity_max", &number);
    g->sensitivity_max = (uint8_t)number;
    json_read_uint64(o, "granted_at", &g->granted_at);
    json_read_uint64(o, "revoked_at", &g->revoked_at);
    return (g->app_id[0] && g->capability[0]) ? 0 : -1;
}

int app_upgrade_grants_write_snapshot(const char *target,
                                      const app_capability_scope_t *capabilities,
                                      const edge_app_grant_t *snapshots, const bool *found,
                                      size_t count, char *reason, size_t reason_cap) {
    cJSON *root = cJSON_CreateObject();
    cJSON *list = root ? cJSON_AddArrayToObject(root, "grants") : NULL;
    if (!list) {
        cJSON_Delete(root);
        set_reason(reason, reason_cap, "grant snapshot allocation failed");
        return -1;
    }
    cJSON_AddNumberToObject(root, "version", 1);
    bool ok = true;
    for (size_t i = 0; ok && i < count; i++) {
        cJSON *entry = cJSON_CreateObject();
        if (!entry) {
            ok = false;
            break;
        }
        cJSON_AddStringToObject(entry, "capability", capabilities[i].capability);
        cJSON_AddBoolToObject(entry, "found", found[i]);
        if (found[i]) {
            cJSON *grant = grant_json(&snapshots[i]);
            if (!grant) {
                cJSON_Delete(entry);
                ok = false;
                break;
            }
            cJSON_AddItemToObject(entry, "grant", grant);
        }
        cJSON_AddItemToArray(list, entry);
    }
    char *json = ok ? cJSON_PrintUnformatted(root) : NULL;
    cJSON_Delete(root);
    if (!json) {
        set_reason(reason, reason_cap, "grant snapshot build failed");
        return -1;
    }
    char path[288];
    app_upgrade_grants_path(target, path, sizeof(path));
    int rc = write_file_sync(path, json, strlen(json));
    free(json);
    if (rc != 0) {
        unlink(path);
        set_reason(reason, reason_cap, "grant snapshot write failed");
        return -1;
    }
    fsync_parent(path);
    return 0;
}

int app_upgrade_grants_write_done(const char *target) {
    static const char body[] = "grants=done\n";
    char path[296];
    app_upgrade_grants_done_path(target, path, sizeof(path));
    if (write_file_sync(path, body, sizeof(body) - 1) != 0) {
        unlink(path);
        return -1;
    }
    fsync_parent(path);
    return 0;
}

int app_upgrade_grants_restore(const edge_authz_store_t *grants, const char *app_id,
                               const app_capability_scope_t *capabilities,
                               const edge_app_grant_t *snapshots, const bool *found, size_t count) {
    if (!grants) return -1;
    for (size_t i = 0; i < count; i++) {
        if (found[i]) {
            if (!grants->save_grant || grants->save_grant(grants->ctx, &snapshots[i]) != 0) {
                return -1;
            }
        } else if (!grants->delete_grant ||
                   grants->delete_grant(grants->ctx, app_id, capabilities[i].capability) != 0) {
            return -1;
        }
    }
    return 0;
}

/* Recovery runs once at startup and is single-threaded; a static buffer keeps
 * the snapshot reader out of the module heap ratchet. */
static char snapshot_text[GRANT_SNAPSHOT_MAX];

static char *read_text_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    size_t n = fread(snapshot_text, 1, sizeof(snapshot_text) - 1, f);
    bool ok = !ferror(f);
    fclose(f);
    if (!ok) return NULL;
    snapshot_text[n] = '\0';
    return snapshot_text;
}

int app_upgrade_grants_restore_snapshot(const edge_authz_store_t *grants, const char *app_id,
                                        const char *snapshot_path) {
    if (!grants) return -1;
    char *json = read_text_file(snapshot_path);
    if (!json) return -1;
    cJSON *root = cJSON_Parse(json);
    if (!root) return -1;

    const cJSON *entry = NULL;
    int rc = -1;
    const cJSON *version = cJSON_GetObjectItem(root, "version");
    const cJSON *list = cJSON_GetObjectItem(root, "grants");
    if (!cJSON_IsNumber(version) || version->valueint != 1 || !cJSON_IsArray(list)) goto out;
    rc = 0;
    cJSON_ArrayForEach(entry, list) {
        const cJSON *capability = cJSON_GetObjectItem(entry, "capability");
        if (!cJSON_IsString(capability) || !capability->valuestring[0]) {
            rc = -1;
            break;
        }
        if (cJSON_IsTrue(cJSON_GetObjectItem(entry, "found"))) {
            edge_app_grant_t grant;
            const cJSON *grant_object = cJSON_GetObjectItem(entry, "grant");
            if (!cJSON_IsObject(grant_object) || grant_from_json(grant_object, &grant) != 0 ||
                strcmp(grant.app_id, app_id) != 0 || !grants->save_grant ||
                grants->save_grant(grants->ctx, &grant) != 0) {
                rc = -1;
                break;
            }
        } else if (!grants->delete_grant ||
                   grants->delete_grant(grants->ctx, app_id, capability->valuestring) != 0) {
            rc = -1;
            break;
        }
    }
out:
    cJSON_Delete(root);
    return rc;
}
