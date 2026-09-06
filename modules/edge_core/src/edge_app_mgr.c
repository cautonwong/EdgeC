/**
 * @file edge_app_mgr.c
 * @brief EdgeOS WASM 应用管理器实现
 */

#include "edge_app_mgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cjson/cJSON.h>

#include "edge_log.h"

struct edge_app_mgr_s {
    uv_loop_t *loop;
    datahub_t *hub;
    sqlite_svc_t *db;
    void *stream_engine;
};

static void _mkdir_p(const char *dir) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

int edge_app_mgr_parse_manifest_json(const char *json_str, app_manifest_t *out_manifest) {
    if (!json_str || !out_manifest) return -1;
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        LOG_ERROR(LOG_CAT_CORE, "AppMgr: Invalid JSON manifest");
        return -1;
    }

    memset(out_manifest, 0, sizeof(app_manifest_t));

    cJSON *item = cJSON_GetObjectItem(root, "id");
    if (item && item->valuestring) snprintf(out_manifest->id, sizeof(out_manifest->id), "%s", item->valuestring);

    item = cJSON_GetObjectItem(root, "name");
    if (item && item->valuestring) snprintf(out_manifest->name, sizeof(out_manifest->name), "%s", item->valuestring);
    else snprintf(out_manifest->name, sizeof(out_manifest->name), "%s", out_manifest->id);

    item = cJSON_GetObjectItem(root, "version");
    if (item && item->valuestring) snprintf(out_manifest->version, sizeof(out_manifest->version), "%s", item->valuestring);
    else snprintf(out_manifest->version, sizeof(out_manifest->version), "1.0.0");

    item = cJSON_GetObjectItem(root, "author");
    if (item && item->valuestring) snprintf(out_manifest->author, sizeof(out_manifest->author), "%s", item->valuestring);
    else snprintf(out_manifest->author, sizeof(out_manifest->author), "Unknown");

    item = cJSON_GetObjectItem(root, "trigger_tag");
    if (item) out_manifest->trigger_tag = (uint32_t)item->valueint;
    else out_manifest->trigger_tag = 1001;

    item = cJSON_GetObjectItem(root, "entry_func");
    if (item && item->valuestring) snprintf(out_manifest->entry_func, sizeof(out_manifest->entry_func), "%s", item->valuestring);
    else snprintf(out_manifest->entry_func, sizeof(out_manifest->entry_func), "on_event");

    item = cJSON_GetObjectItem(root, "fault_policy");
    if (item && item->valuestring && strcmp(item->valuestring, "isolate_and_alarm") == 0) {
        out_manifest->fault_policy = FAULT_POLICY_ISOLATE_AND_ALARM;
    } else {
        out_manifest->fault_policy = FAULT_POLICY_AUTO_RESTART;
    }

    item = cJSON_GetObjectItem(root, "heap_limit_kb");
    if (item) out_manifest->heap_limit_kb = (uint32_t)item->valueint;
    else out_manifest->heap_limit_kb = 256;

    item = cJSON_GetObjectItem(root, "timeout_ms");
    if (item) out_manifest->timeout_ms = (uint32_t)item->valueint;
    else out_manifest->timeout_ms = 100;

    snprintf(out_manifest->wasm_path, sizeof(out_manifest->wasm_path),
             "/var/edgeos/apps/%s/app.wasm", out_manifest->id);

    out_manifest->state = APP_STATE_STOPPED;
    out_manifest->restart_count = 0;
    out_manifest->created_at = (uint64_t)time(NULL);

    cJSON_Delete(root);
    return 0;
}

int edge_app_mgr_construct_default_manifest(const char *filename_or_id, app_manifest_t *out_manifest) {
    if (!filename_or_id || !out_manifest) return -1;
    memset(out_manifest, 0, sizeof(app_manifest_t));

    const char *apps_dir = getenv("EDGE_APPS_DIR");
    if (!apps_dir || apps_dir[0] == '\0') {
        apps_dir = "/var/edgeos/apps";
    }

    char clean_id[64];
    snprintf(clean_id, sizeof(clean_id), "%s", filename_or_id);
    char *dot = strrchr(clean_id, '.');
    if (dot && strcmp(dot, ".wasm") == 0) *dot = '\0';

    snprintf(out_manifest->id, sizeof(out_manifest->id), "%s", clean_id);
    snprintf(out_manifest->name, sizeof(out_manifest->name), "%s App", clean_id);
    snprintf(out_manifest->version, sizeof(out_manifest->version), "1.0.0");
    snprintf(out_manifest->author, sizeof(out_manifest->author), "EdgeOS Auto");
    snprintf(out_manifest->wasm_path, sizeof(out_manifest->wasm_path), "%s/%s/app.wasm", apps_dir, clean_id);
    out_manifest->trigger_tag = 1001;
    snprintf(out_manifest->entry_func, sizeof(out_manifest->entry_func), "on_event");
    out_manifest->fault_policy = FAULT_POLICY_AUTO_RESTART;
    out_manifest->heap_limit_kb = 256;
    out_manifest->timeout_ms = 100;
    out_manifest->state = APP_STATE_STOPPED;
    out_manifest->restart_count = 0;
    out_manifest->created_at = (uint64_t)time(NULL);

    return 0;
}

edge_app_mgr_t *edge_app_mgr_create(uv_loop_t *loop, datahub_t *hub, sqlite_svc_t *db, void *stream_engine) {
    edge_app_mgr_t *mgr = calloc(1, sizeof(edge_app_mgr_t));
    if (!mgr) return NULL;
    mgr->loop = loop;
    mgr->hub = hub;
    mgr->db = db;
    mgr->stream_engine = stream_engine;
    LOG_INFO(LOG_CAT_CORE, "AppMgr: Initialized successfully");
    return mgr;
}

void edge_app_mgr_destroy(edge_app_mgr_t *mgr) {
    if (!mgr) return;
    free(mgr);
}

int edge_app_mgr_install(edge_app_mgr_t *mgr, const app_manifest_t *manifest, const void *wasm_data, size_t wasm_len) {
    if (!mgr || !manifest) return -1;

    app_manifest_t m = *manifest;
    if (m.wasm_path[0] == '\0') {
        const char *apps_dir = getenv("EDGE_APPS_DIR");
        if (!apps_dir || apps_dir[0] == '\0') {
            apps_dir = "/var/edgeos/apps";
        }
        snprintf(m.wasm_path, sizeof(m.wasm_path), "%s/%s/app.wasm", apps_dir, m.id);
    }

    char dir_path[256];
    snprintf(dir_path, sizeof(dir_path), "%s", m.wasm_path);
    char *slash = strrchr(dir_path, '/');
    if (slash) {
        *slash = '\0';
        _mkdir_p(dir_path);
    }

    if (wasm_data && wasm_len > 0) {
        FILE *f = fopen(m.wasm_path, "wb");
        if (!f) {
            LOG_ERROR(LOG_CAT_CORE, "AppMgr: Failed to write WASM file to %s", m.wasm_path);
            return -1;
        }
        fwrite(wasm_data, 1, wasm_len, f);
        fclose(f);
    }

    m.state = APP_STATE_INSTALLED;
    int rc = sqlite_svc_save_app(mgr->db, &m);
    if (rc == 0) {
        LOG_INFO(LOG_CAT_CORE, "AppMgr: Installed app %s successfully at %s", m.id, m.wasm_path);
    }
    return rc;
}

int edge_app_mgr_start(edge_app_mgr_t *mgr, const char *app_id) {
    if (!mgr || !app_id) return -1;
    app_manifest_t m;
    if (sqlite_svc_get_app(mgr->db, app_id, &m) != 0) {
        LOG_ERROR(LOG_CAT_CORE, "AppMgr: App %s not found in registry", app_id);
        return -1;
    }

    m.state = APP_STATE_RUNNING;
    sqlite_svc_save_app(mgr->db, &m);
    LOG_INFO(LOG_CAT_CORE, "AppMgr: Started app %s", app_id);
    return 0;
}

int edge_app_mgr_stop(edge_app_mgr_t *mgr, const char *app_id) {
    if (!mgr || !app_id) return -1;
    app_manifest_t m;
    if (sqlite_svc_get_app(mgr->db, app_id, &m) != 0) {
        return -1;
    }

    m.state = APP_STATE_STOPPED;
    sqlite_svc_save_app(mgr->db, &m);
    LOG_INFO(LOG_CAT_CORE, "AppMgr: Stopped app %s", app_id);
    return 0;
}

int edge_app_mgr_uninstall(edge_app_mgr_t *mgr, const char *app_id) {
    if (!mgr || !app_id) return -1;
    edge_app_mgr_stop(mgr, app_id);

    app_manifest_t m;
    if (sqlite_svc_get_app(mgr->db, app_id, &m) == 0) {
        unlink(m.wasm_path);
    }

    sqlite_svc_delete_app(mgr->db, app_id);
    LOG_INFO(LOG_CAT_CORE, "AppMgr: Uninstalled app %s", app_id);
    return 0;
}

int edge_app_mgr_boot_restore(edge_app_mgr_t *mgr) {
    if (!mgr || !mgr->db) return 0;
    app_manifest_t apps[64];
    int count = sqlite_svc_list_apps(mgr->db, apps, 64);
    int restored = 0;

    for (int i = 0; i < count; i++) {
        if (apps[i].state == APP_STATE_RUNNING) {
            LOG_INFO(LOG_CAT_CORE, "AppMgr: Boot restoring RUNNING app %s", apps[i].id);
            edge_app_mgr_start(mgr, apps[i].id);
            restored++;
        }
    }
    LOG_INFO(LOG_CAT_CORE, "AppMgr: Boot restore complete, restored %d apps", restored);
    return restored;
}

int edge_app_mgr_get_app(edge_app_mgr_t *mgr, const char *app_id, app_manifest_t *out_manifest) {
    if (!mgr || !mgr->db || !app_id || !out_manifest) return -1;
    return sqlite_svc_get_app(mgr->db, app_id, out_manifest);
}
