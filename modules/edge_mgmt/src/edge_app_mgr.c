/**
 * @file edge_app_mgr.c
 * @brief EdgeOS WASM 应用管理器实现
 */

#include "edgeos/edge_app_mgr.h"
#include "app_upgrade_grants.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cjson/cJSON.h>
#include <stdarg.h>
#include "edge_wasm_imports.h"
#include <uv.h>

static int _write_file_sync(const char *path, const void *data, size_t len);
static void _fsync_parent(const char *path);
static int _write_pending(const char *target, const char *new_version, char *reason,
                          size_t reason_cap);
static void _remove_pending(const char *target);
static void _json_read_string(const cJSON *o, const char *key, char *out, size_t cap);
static void _json_read_uint64(const cJSON *o, const char *key, uint64_t *out);

typedef enum {
    APP_JOB_INSTALL = 0,
    APP_JOB_START,
    APP_JOB_STOP,
    APP_JOB_UNINSTALL,
    APP_JOB_RECLAIM_STOP,
    APP_JOB_UPGRADE,
    APP_JOB_ROLLBACK,
} app_job_kind_t;

/* 单槽任务：持有 install/upgrade 的 manifest 与 wasm 副本，生命周期到 after_work 为止。 */
#define APP_JOB_MAX_CAPABILITIES 16

typedef struct {
    uv_work_t req;
    edge_app_mgr_t *mgr;
    app_job_kind_t kind;
    app_manifest_t manifest;
    void *wasm;
    size_t wasm_len;
    app_capability_scope_t capabilities[APP_JOB_MAX_CAPABILITIES];
    size_t capability_count;
    edge_authz_store_t grants;
    bool has_grants;
    char app_id[sizeof(((app_manifest_t *)0)->id)];
    app_uninstall_opts_t uninstall;
    char version[sizeof(((app_manifest_t *)0)->version)];
    int rc;
    char reason[160];
    edge_app_job_cb_t cb;
    void *cb_ctx;
} app_job_t;

struct edge_app_mgr_s {
    uv_loop_t *loop;
    edge_app_repo_t repo;
    void *stream_engine;
    edge_app_logger_t logger;
    uint64_t fault_window_ms;
    char apps_dir[256];
    bool crash_after_package_write; /**< 仅测试注入使用 */
    bool crash_after_upgrade_swap;  /**< 仅测试注入使用 */
    bool crash_after_registry_commit; /**< 仅测试注入使用 */
    app_job_t job; /**< 单槽；job_active 表示已入槽，只在 loop 线程读写。 */
    bool job_active;
};

static void app_log(edge_app_mgr_t *mgr, int level, const char *fmt, ...) {
    if (!mgr || !mgr->logger.log) return;
    char message[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    mgr->logger.log(mgr->logger.ctx, level, message);
}

void edge_app_mgr_set_logger(edge_app_mgr_t *mgr, const edge_app_logger_t *logger) {
    if (!mgr) return;
    if (logger) {
        mgr->logger = *logger;
    } else {
        memset(&mgr->logger, 0, sizeof(mgr->logger));
    }
}


static bool _valid_app_id(const char *id) {
    if (!id || !id[0]) return false;
    size_t len = strlen(id);
    if (len >= sizeof(((app_manifest_t *)0)->id)) return false;
    if (strstr(id, "..")) return false;
    for (size_t i = 0; i < len; i++) {
        char c = id[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '.' || c == '_' || c == '-') {
            continue;
        }
        return false;
    }
    return true;
}

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

/* 应用目录与 wasm 落盘路径只有这一处定义：安装和默认清单必须算出同一条路径。 */
static int _default_wasm_path(const char *apps_dir, const char *app_id, char *out, size_t cap) {
    if (!apps_dir || apps_dir[0] == '\0') apps_dir = "/var/edgeos/apps";
    int n = snprintf(out, cap, "%s/%s/app.wasm", apps_dir, app_id);
    return (n >= 0 && (size_t)n < cap) ? 0 : -1;
}

int edge_app_mgr_parse_manifest_json(const char *json_str, app_manifest_t *out_manifest) {
    if (!json_str || !out_manifest) return -1;
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        fprintf(stderr, "AppMgr: Invalid JSON manifest\n");
        return -1;
    }

    memset(out_manifest, 0, sizeof(app_manifest_t));

    cJSON *item = cJSON_GetObjectItem(root, "id");
    if (!item || !item->valuestring || !_valid_app_id(item->valuestring)) {
        cJSON_Delete(root);
        return -1;
    }
    snprintf(out_manifest->id, sizeof(out_manifest->id), "%s", item->valuestring);

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

    item = cJSON_GetObjectItem(root, "tick_ms");
    if (item) {
        double value = cJSON_IsNumber(item) ? item->valuedouble : -1.0;
        if (!cJSON_IsNumber(item) || value < 0.0 || value > 60000.0 ||
            value != (double)(uint32_t)value || (value != 0.0 && value < 50.0)) {
            cJSON_Delete(root);
            return -1;
        }
        out_manifest->tick_ms = (uint32_t)value;
    }

    out_manifest->state = APP_STATE_STOPPED;
    out_manifest->restart_count = 0;
    out_manifest->created_at = (uint64_t)time(NULL);

    cJSON *resources = cJSON_GetObjectItem(root, "resources");
    cJSON *priority = resources ? cJSON_GetObjectItem(resources, "priority") : NULL;
    if (cJSON_IsNumber(priority)) {
        int p = priority->valueint;
        out_manifest->priority = (uint32_t)(p < 0 ? 0 : (p > 10 ? 10 : p));
    }
    out_manifest->keep_alive = false;
    out_manifest->desired_state = false;
    out_manifest->last_active_ms = 0;

    cJSON_Delete(root);
    return 0;
}

int edge_app_mgr_construct_default_manifest(const char *filename_or_id, app_manifest_t *out_manifest) {
    if (!filename_or_id || !out_manifest) return -1;
    memset(out_manifest, 0, sizeof(app_manifest_t));

    char clean_id[64];
    snprintf(clean_id, sizeof(clean_id), "%s", filename_or_id);
    char *dot = strrchr(clean_id, '.');
    if (dot && strcmp(dot, ".wasm") == 0) *dot = '\0';
    if (!_valid_app_id(clean_id)) return -1;

    snprintf(out_manifest->id, sizeof(out_manifest->id), "%s", clean_id);
    snprintf(out_manifest->name, sizeof(out_manifest->name), "%s App", clean_id);
    snprintf(out_manifest->version, sizeof(out_manifest->version), "1.0.0");
    snprintf(out_manifest->author, sizeof(out_manifest->author), "EdgeOS Auto");
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

size_t edge_app_mgr_storage_size(void) {
    return sizeof(struct edge_app_mgr_s);
}

edge_app_mgr_t *edge_app_mgr_create_with_repo_in(void *storage, size_t storage_size,
                                                 uv_loop_t *loop,
                                                 const edge_app_repo_t *repo,
                                                 void *stream_engine) {
    if (!storage || storage_size < sizeof(struct edge_app_mgr_s) || !repo) return NULL;
    edge_app_mgr_t *mgr = storage;
    memset(mgr, 0, sizeof(*mgr));
    mgr->loop = loop;
    mgr->repo = *repo;
    mgr->stream_engine = stream_engine;
    mgr->fault_window_ms = EDGE_APP_FAULT_WINDOW_MS;
    snprintf(mgr->apps_dir, sizeof(mgr->apps_dir), "%s", "/var/edgeos/apps");
    return mgr;
}

void edge_app_mgr_set_apps_dir(edge_app_mgr_t *mgr, const char *dir) {
    if (!mgr || !dir || !dir[0]) return;
    snprintf(mgr->apps_dir, sizeof(mgr->apps_dir), "%s", dir);
}

void edge_app_mgr_set_crash_after_package_write(edge_app_mgr_t *mgr, bool enabled) {
    if (mgr) mgr->crash_after_package_write = enabled;
}

void edge_app_mgr_set_crash_after_upgrade_swap(edge_app_mgr_t *mgr, bool enabled) {
    if (mgr) mgr->crash_after_upgrade_swap = enabled;
}

void edge_app_mgr_set_crash_after_registry_commit(edge_app_mgr_t *mgr, bool enabled) {
    if (mgr) mgr->crash_after_registry_commit = enabled;
}

void edge_app_mgr_set_fault_window_ms(edge_app_mgr_t *mgr, uint64_t window_ms) {
    if (mgr) mgr->fault_window_ms = window_ms;
}

const edge_app_repo_t *edge_app_mgr_get_repo(const edge_app_mgr_t *mgr) {
    return mgr ? &mgr->repo : NULL;
}

void edge_app_mgr_destroy(edge_app_mgr_t *mgr) {
    if (!mgr) return;
    if (mgr->job_active) {
        app_log(mgr, 4, "AppMgr: destroy refused while an async job is in flight");
    }
}

/* 文件写与仓储提交；同步 API 与 worker 线程共用。不在内部记日志。 */
static int _install_impl(edge_app_mgr_t *mgr, const app_manifest_t *manifest,
                         const void *wasm_data, size_t wasm_len,
                         char *reason, size_t reason_cap) {
    app_manifest_t m = *manifest;
    if (m.wasm_path[0] == '\0') {
        if (_default_wasm_path(mgr->apps_dir, m.id, m.wasm_path, sizeof(m.wasm_path)) != 0) {
            if (reason) snprintf(reason, reason_cap, "wasm path too long");
            return -1;
        }
    }

    if (wasm_data && wasm_len > 0) {
        char import_reason[EDGE_WASM_IMPORT_REASON_SIZE];
        if (edge_wasm_imports_validate(wasm_data, wasm_len, import_reason,
                                       sizeof(import_reason)) != 0) {
            if (reason) snprintf(reason, reason_cap, "%s", import_reason);
            return -1;
        }
    }

    char dir_path[256];
    snprintf(dir_path, sizeof(dir_path), "%s", m.wasm_path);
    char *slash = strrchr(dir_path, '/');
    if (slash) {
        *slash = '\0';
        _mkdir_p(dir_path);
    }

    if (wasm_data && wasm_len > 0) {
        char new_path[264];
        snprintf(new_path, sizeof(new_path), "%s.new", m.wasm_path);
        if (_write_file_sync(new_path, wasm_data, wasm_len) != 0) {
            if (strncmp(m.wasm_path, "/var/edgeos/apps", 16) == 0) {
                snprintf(m.wasm_path, sizeof(m.wasm_path), "/tmp/edgeos/apps/%s/app.wasm", m.id);
                snprintf(dir_path, sizeof(dir_path), "%s", m.wasm_path);
                char *s2 = strrchr(dir_path, '/');
                if (s2) {
                    *s2 = '\0';
                    _mkdir_p(dir_path);
                }
                snprintf(new_path, sizeof(new_path), "%s.new", m.wasm_path);
            }
            if (_write_file_sync(new_path, wasm_data, wasm_len) != 0) {
                unlink(new_path);
                if (reason) snprintf(reason, reason_cap, "wasm write failed: %.120s", m.wasm_path);
                return -1;
            }
        }
        if (_write_pending(m.wasm_path, m.version, reason, reason_cap) != 0) {
            unlink(new_path);
            return -1;
        }
        if (rename(new_path, m.wasm_path) != 0) {
            unlink(new_path);
            _remove_pending(m.wasm_path);
            if (reason) snprintf(reason, reason_cap, "wasm rename failed: %.120s", m.wasm_path);
            return -1;
        }
        _fsync_parent(m.wasm_path);
    }

    /* Crash-injection point for the install consistency test: the package is
     * on disk and the registry row is not. Only tests set this env var. */
    if (mgr->crash_after_package_write) _exit(70);

    m.state = APP_STATE_INSTALLED;
    int rc = mgr->repo.save_app ? mgr->repo.save_app(mgr->repo.ctx, &m) : -1;
    if (rc != 0) {
        _remove_pending(m.wasm_path);
        if (reason) snprintf(reason, reason_cap, "repository save failed");
        return rc;
    }
    _remove_pending(m.wasm_path);
    if (reason) reason[0] = '\0';
    return 0;
}

int edge_app_mgr_install(edge_app_mgr_t *mgr, const app_manifest_t *manifest, const void *wasm_data, size_t wasm_len) {
    if (!mgr || !manifest || !_valid_app_id(manifest->id)) return -1;

    char reason[160] = "";
    int rc = _install_impl(mgr, manifest, wasm_data, wasm_len, reason, sizeof(reason));
    if (rc == 0) {
        app_log(mgr, 1, "AppMgr: Installed app %s successfully", manifest->id);
    } else {
        app_log(mgr, 3, "AppMgr: install %s failed: %s", manifest->id, reason);
    }
    return rc;
}

static int _start_impl(edge_app_mgr_t *mgr, const char *app_id, char *reason, size_t reason_cap) {
    app_manifest_t m;
    if (!mgr->repo.get_app || mgr->repo.get_app(mgr->repo.ctx, app_id, &m) != 0) {
        if (reason) snprintf(reason, reason_cap, "app not found in registry");
        return -1;
    }
    if (m.state == APP_STATE_UNINSTALLED) {
        if (reason) snprintf(reason, reason_cap, "app is uninstalled; install again first");
        return -1;
    }

    m.state = APP_STATE_RUNNING;
    m.desired_state = true;
    m.last_active_ms = (uint64_t)uv_now(mgr->loop);
    if (!mgr->repo.save_app || mgr->repo.save_app(mgr->repo.ctx, &m) != 0) {
        if (reason) snprintf(reason, reason_cap, "registry save failed");
        return -1;
    }
    if (reason) reason[0] = '\0';
    return 0;
}

static void _set_reason(char *reason, size_t cap, const char *text) {
    if (reason && cap) snprintf(reason, cap, "%s", text);
}

static int _version_parse(const char *version, int out[3]) {
    int major = 0, minor = 0, patch = 0;
    if (!version || sscanf(version, "%d.%d.%d", &major, &minor, &patch) != 3) return -1;
    out[0] = major;
    out[1] = minor;
    out[2] = patch;
    return 0;
}

/* 1 when newv > oldv, 0 when equal, -1 when older or unparsable. */
static int _version_compare(const char *oldv, const char *newv) {
    int a[3], b[3];
    if (_version_parse(oldv, a) != 0 || _version_parse(newv, b) != 0) return -1;
    for (int i = 0; i < 3; i++) {
        if (b[i] != a[i]) return b[i] > a[i] ? 1 : -1;
    }
    return 0;
}

static int _write_file_sync(const char *path, const void *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t written = fwrite(data, 1, len, f);
    int rc = fflush(f);
    if (rc == 0) rc = fsync(fileno(f));
    if (fclose(f) != 0) rc = -1;
    return (written == len && rc == 0) ? 0 : -1;
}

static void _fsync_parent(const char *path) {
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

/* 换包和 Registry 提交之间的崩溃标记。写在换包之前并 fsync，提交后删除；
 * 恢复时按 Registry 版本决定保留新包还是回滚旧包。 */
static void _pending_path(const char *target, char *out, size_t cap) {
    /* 8 是 ".pending" 的长度；precision 保证结果总是 NUL 结尾且不超 cap。 */
    int room = cap > 9 ? (int)(cap - 9) : 0;
    snprintf(out, cap, "%.*s.pending", room, target);
}

static int _write_pending(const char *target, const char *new_version, char *reason,
                          size_t reason_cap) {
    char pending[272];
    char body[96];
    _pending_path(target, pending, sizeof(pending));
    int n = snprintf(body, sizeof(body), "new=%s\n", new_version);
    if (n <= 0 || (size_t)n >= sizeof(body)) {
        _set_reason(reason, reason_cap, "version too large for pending marker");
        return -1;
    }
    if (_write_file_sync(pending, body, (size_t)n) != 0) {
        unlink(pending);
        _set_reason(reason, reason_cap, "pending marker write failed");
        return -1;
    }
    _fsync_parent(pending);
    return 0;
}

static void _remove_pending(const char *target) {
    char pending[272];
    _pending_path(target, pending, sizeof(pending));
    if (unlink(pending) == 0) _fsync_parent(pending);
}

static int _read_pending_version(const char *pending, char *out, size_t cap) {
    char buf[128];
    FILE *f = fopen(pending, "r");
    if (!f) return -1;
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';
    const char *p = strstr(buf, "new=");
    if (!p) return -1;
    p += 4;
    size_t i = 0;
    while (p[i] && p[i] != '\n' && p[i] != '\r' && i + 1 < cap) {
        out[i] = p[i];
        i++;
    }
    out[i] = '\0';
    return out[0] ? 0 : -1;
}

/* Repair a package interrupted by an upgrade. True when no backup is left or the
 * old bytes are back in place. */
static bool _restore_package(const char *wasm_path) {
    char prev[264];
    snprintf(prev, sizeof(prev), "%s.prev", wasm_path);
    if (access(prev, F_OK) != 0) return true;
    return rename(prev, wasm_path) == 0;
}

/* Atomically replace target with wasm_data, keeping the old file at prev_path.
 * On any failure the old package is back in place and .new is gone. */
static int _swap_package(const char *target, const char *new_path, const char *prev_path,
                         const void *wasm_data, size_t wasm_len,
                         char *reason, size_t reason_cap) {
    if (_write_file_sync(new_path, wasm_data, wasm_len) != 0) {
        unlink(new_path);
        _set_reason(reason, reason_cap, "package write failed");
        return -1;
    }
    unlink(prev_path); /* leftover from an older upgrade */
    bool moved_old = false;
    if (rename(target, prev_path) == 0) {
        moved_old = true;
    } else if (errno != ENOENT) {
        unlink(new_path);
        _set_reason(reason, reason_cap, "package backup failed");
        return -1;
    }
    if (rename(new_path, target) != 0) {
        unlink(new_path);
        if (moved_old && access(target, F_OK) != 0) rename(prev_path, target);
        _set_reason(reason, reason_cap, "package swap failed");
        return -1;
    }
    _fsync_parent(target);
    return 0;
}

/* One flat object with every app_manifest_t field, including prev_*. */
static char *_manifest_to_json(const app_manifest_t *m) {
    cJSON *o = cJSON_CreateObject();
    if (!o) return NULL;
    cJSON_AddStringToObject(o, "id", m->id);
    cJSON_AddStringToObject(o, "name", m->name);
    cJSON_AddStringToObject(o, "version", m->version);
    cJSON_AddStringToObject(o, "author", m->author);
    cJSON_AddStringToObject(o, "wasm_path", m->wasm_path);
    cJSON_AddNumberToObject(o, "trigger_tag", (double)m->trigger_tag);
    cJSON_AddStringToObject(o, "entry_func", m->entry_func);
    cJSON_AddNumberToObject(o, "fault_policy", (double)m->fault_policy);
    cJSON_AddNumberToObject(o, "heap_limit_kb", (double)m->heap_limit_kb);
    cJSON_AddNumberToObject(o, "timeout_ms", (double)m->timeout_ms);
    cJSON_AddNumberToObject(o, "tick_ms", (double)m->tick_ms);
    cJSON_AddNumberToObject(o, "state", (double)m->state);
    cJSON_AddNumberToObject(o, "restart_count", (double)m->restart_count);
    cJSON_AddNumberToObject(o, "last_fault_ms", (double)m->last_fault_ms);
    cJSON_AddNumberToObject(o, "created_at", (double)m->created_at);
    cJSON_AddNumberToObject(o, "priority", (double)m->priority);
    cJSON_AddBoolToObject(o, "keep_alive", m->keep_alive);
    cJSON_AddBoolToObject(o, "desired_state", m->desired_state);
    cJSON_AddNumberToObject(o, "last_active_ms", (double)m->last_active_ms);
    cJSON_AddStringToObject(o, "prev_version", m->prev_version);
    cJSON_AddStringToObject(o, "prev_manifest_json", m->prev_manifest_json);
    cJSON_AddStringToObject(o, "prev_wasm_path", m->prev_wasm_path);
    char *json = cJSON_PrintUnformatted(o);
    cJSON_Delete(o);
    return json;
}

static void _json_read_string(const cJSON *o, const char *key, char *out, size_t cap) {
    const cJSON *item = cJSON_GetObjectItem(o, key);
    if (cJSON_IsString(item) && item->valuestring) {
        snprintf(out, cap, "%s", item->valuestring);
    }
}

static void _json_read_uint64(const cJSON *o, const char *key, uint64_t *out) {
    const cJSON *item = cJSON_GetObjectItem(o, key);
    if (cJSON_IsNumber(item)) *out = (uint64_t)item->valuedouble;
}

/* Restore a registry row from the JSON shape _manifest_to_json writes. */
static int _manifest_from_json(const char *json, app_manifest_t *m) {
    cJSON *o = cJSON_Parse(json);
    if (!o) return -1;
    memset(m, 0, sizeof(*m));
    _json_read_string(o, "id", m->id, sizeof(m->id));
    _json_read_string(o, "name", m->name, sizeof(m->name));
    _json_read_string(o, "version", m->version, sizeof(m->version));
    _json_read_string(o, "author", m->author, sizeof(m->author));
    _json_read_string(o, "wasm_path", m->wasm_path, sizeof(m->wasm_path));
    _json_read_string(o, "entry_func", m->entry_func, sizeof(m->entry_func));
    _json_read_string(o, "prev_version", m->prev_version, sizeof(m->prev_version));
    _json_read_string(o, "prev_manifest_json", m->prev_manifest_json,
                      sizeof(m->prev_manifest_json));
    _json_read_string(o, "prev_wasm_path", m->prev_wasm_path, sizeof(m->prev_wasm_path));

    uint64_t number = 0;
    _json_read_uint64(o, "trigger_tag", &number);
    m->trigger_tag = (uint32_t)number;
    _json_read_uint64(o, "fault_policy", &number);
    m->fault_policy = (fault_policy_t)number;
    _json_read_uint64(o, "heap_limit_kb", &number);
    m->heap_limit_kb = (uint32_t)number;
    _json_read_uint64(o, "timeout_ms", &number);
    m->timeout_ms = (uint32_t)number;
    _json_read_uint64(o, "tick_ms", &number);
    m->tick_ms = (uint32_t)number;
    _json_read_uint64(o, "state", &number);
    m->state = (app_state_t)number;
    _json_read_uint64(o, "restart_count", &number);
    m->restart_count = (uint32_t)number;
    _json_read_uint64(o, "last_fault_ms", &m->last_fault_ms);
    _json_read_uint64(o, "created_at", &m->created_at);
    _json_read_uint64(o, "priority", &number);
    m->priority = (uint32_t)number;
    _json_read_uint64(o, "last_active_ms", &m->last_active_ms);
    m->keep_alive = cJSON_IsTrue(cJSON_GetObjectItem(o, "keep_alive"));
    m->desired_state = cJSON_IsTrue(cJSON_GetObjectItem(o, "desired_state"));

    cJSON_Delete(o);
    return m->id[0] ? 0 : -1;
}

/* Recovery only scans registry rows with a wasm_path, so a metadata-only
 * upgrade needs one to leave its markers next to. */
static int _upgrade_marker_path(const app_manifest_t *old, const char *apps_dir, char *out,
                                size_t cap, char *reason, size_t reason_cap) {
    snprintf(out, cap, "%s", old->wasm_path);
    if (out[0]) return 0;
    if (_default_wasm_path(apps_dir, old->id, out, cap) != 0) {
        _set_reason(reason, reason_cap, "wasm path too long");
        return -1;
    }
    return 0;
}

/* 文件换包、旧行快照与仓储提交；同步 API 与 worker 线程共用。 */
static int _upgrade_impl(edge_app_mgr_t *mgr, const app_manifest_t *manifest,
                         const void *wasm_data, size_t wasm_len,
                         const app_capability_scope_t *capabilities, size_t capability_count,
                         const edge_authz_store_t *grants, char *reason, size_t reason_cap) {
    _set_reason(reason, reason_cap, "");
    if (!mgr || !manifest || !manifest->id[0]) {
        _set_reason(reason, reason_cap, "invalid manifest");
        return -1;
    }

    app_manifest_t old;
    if (!mgr->repo.get_app || mgr->repo.get_app(mgr->repo.ctx, manifest->id, &old) != 0) {
        _set_reason(reason, reason_cap, "app not found in registry");
        return -1;
    }
    if (old.state == APP_STATE_UNINSTALLED) {
        _set_reason(reason, reason_cap, "app is uninstalled; install again first");
        return -1;
    }
    if (_version_compare(old.version, manifest->version) <= 0) {
        _set_reason(reason, reason_cap, "version must increase");
        return -1;
    }

    if (grants && capabilities && capability_count > APP_JOB_MAX_CAPABILITIES) {
        _set_reason(reason, reason_cap, "too many capabilities");
        return -1;
    }
    /* Snapshot and validate without writing: a failed package swap must not
     * leave grants changed. The snapshot is what the recovery sidecar records. */
    edge_app_grant_t grant_snapshot[APP_JOB_MAX_CAPABILITIES];
    bool grant_found[APP_JOB_MAX_CAPABILITIES];
    memset(grant_found, 0, sizeof(grant_found));
    if (grants && capabilities) {
        if (!grants->get_grant) {
            _set_reason(reason, reason_cap, "grant store unavailable");
            return -1;
        }
        for (size_t i = 0; i < capability_count; i++) {
            edge_app_grant_t existing;
            memset(&existing, 0, sizeof(existing));
            snprintf(existing.app_id, sizeof(existing.app_id), "%s", manifest->id);
            snprintf(existing.capability, sizeof(existing.capability), "%s",
                     capabilities[i].capability);
            snprintf(existing.scope_json, sizeof(existing.scope_json), "[]");
            edge_app_grant_t row;
            int found = grants->get_grant(grants->ctx, manifest->id,
                                          capabilities[i].capability, &row);
            if (found < 0) {
                _set_reason(reason, reason_cap, "grant store unavailable");
                return -1;
            }
            if (found == 1) {
                existing = row;
                grant_snapshot[i] = row;
                grant_found[i] = true;
            }
            edge_app_grant_t probe;
            if (edge_authz_inherit_grant(&existing, capabilities[i].scope_json, &probe) ==
                EDGE_AUTHZ_INHERIT_INVALID) {
                _set_reason(reason, reason_cap, "invalid capability scope");
                return -1;
            }
        }
    }

    bool has_package = wasm_data && wasm_len > 0;
    if (has_package) {
        char import_reason[EDGE_WASM_IMPORT_REASON_SIZE];
        if (edge_wasm_imports_validate(wasm_data, wasm_len, import_reason,
                                       sizeof(import_reason)) != 0) {
            _set_reason(reason, reason_cap, import_reason);
            return -1;
        }
    }

    /* 快照里的 prev_* 置空：rollback 恢复后本就会清空，嵌套会让 4096 字段逐次溢出。 */
    app_manifest_t snapshot = old;
    memset(snapshot.prev_version, 0, sizeof(snapshot.prev_version));
    memset(snapshot.prev_manifest_json, 0, sizeof(snapshot.prev_manifest_json));
    memset(snapshot.prev_wasm_path, 0, sizeof(snapshot.prev_wasm_path));
    char *old_json = _manifest_to_json(&snapshot);
    if (!old_json || strlen(old_json) >= sizeof(((app_manifest_t *)0)->prev_manifest_json)) {
        free(old_json);
        _set_reason(reason, reason_cap, "previous manifest too large");
        return -1;
    }

    char target_path[256];
    if (_upgrade_marker_path(&old, mgr->apps_dir, target_path, sizeof(target_path),
                             reason, reason_cap) != 0) {
        free(old_json);
        return -1;
    }

    if (_write_pending(target_path, manifest->version, reason, reason_cap) != 0) {
        free(old_json);
        return -1;
    }
    if (has_package) {
        char new_path[264];
        char prev_path[264];
        snprintf(new_path, sizeof(new_path), "%s.new", target_path);
        snprintf(prev_path, sizeof(prev_path), "%s.prev", target_path);
        if (_swap_package(target_path, new_path, prev_path, wasm_data, wasm_len, reason,
                          reason_cap) != 0) {
            _remove_pending(target_path);
            free(old_json);
            return -1;
        }
        /* Crash-injection point: the new package is in place, the registry row
         * still holds the old version. Only tests set this env var. */
        if (mgr->crash_after_upgrade_swap) _exit(71);
    }
    /* Recovery replays this snapshot when the process stops before the grant
     * block; metadata-only upgrades need it to roll the registry row back too. */
    if (grants && capabilities &&
        app_upgrade_grants_write_snapshot(target_path, capabilities, grant_snapshot, grant_found,
                              capability_count, reason, reason_cap) != 0) {
        if (!has_package || _restore_package(target_path)) app_upgrade_markers_remove(target_path);
        free(old_json);
        return -1;
    }

    app_manifest_t updated = old;
    snprintf(updated.version, sizeof(updated.version), "%s", manifest->version);
    snprintf(updated.name, sizeof(updated.name), "%s", manifest->name);
    snprintf(updated.author, sizeof(updated.author), "%s", manifest->author);
    snprintf(updated.entry_func, sizeof(updated.entry_func), "%s", manifest->entry_func);
    updated.trigger_tag = manifest->trigger_tag;
    updated.fault_policy = manifest->fault_policy;
    updated.heap_limit_kb = manifest->heap_limit_kb;
    updated.timeout_ms = manifest->timeout_ms;
    updated.tick_ms = manifest->tick_ms;
    updated.priority = manifest->priority;
    updated.keep_alive = manifest->keep_alive;
    snprintf(updated.prev_version, sizeof(updated.prev_version), "%s", old.version);
    snprintf(updated.prev_manifest_json, sizeof(updated.prev_manifest_json), "%s", old_json);
    /* A metadata-only upgrade has no package to roll back. */
    updated.prev_wasm_path[0] = '\0';
    if (has_package) {
        snprintf(updated.wasm_path, sizeof(updated.wasm_path), "%s", target_path);
        snprintf(updated.prev_wasm_path, sizeof(updated.prev_wasm_path), "%s", target_path);
    }
    free(old_json);

    if (!mgr->repo.save_app || mgr->repo.save_app(mgr->repo.ctx, &updated) != 0) {
        /* Keep package and registry consistent: put the old bytes back. */
        bool rolled_back =
            updated.prev_wasm_path[0] == '\0' || _restore_package(updated.wasm_path);
        /* If the rollback failed, keep the markers so the recovery path
         * retries it on the next start. */
        if (rolled_back) app_upgrade_markers_remove(target_path);
        _set_reason(reason, reason_cap, "registry save failed");
        return -1;
    }
    /* Crash-injection point: the registry row is committed, the grant block has
     * not run. Only tests set this env var. */
    if (mgr->crash_after_registry_commit) _exit(72);

    if (grants && capabilities) {
        uint64_t now = (uint64_t)time(NULL);
        bool grants_ok = true;
        for (size_t i = 0; i < capability_count; i++) {
            edge_authz_inherit_t rc = edge_authz_inherit_upgrade(
                grants, manifest->id, capabilities[i].capability, capabilities[i].scope_json, now);
            if (rc == EDGE_AUTHZ_INHERIT_STORE_ERROR || rc == EDGE_AUTHZ_INHERIT_INVALID) {
                grants_ok = false;
                break;
            }
        }
        /* The done marker is the only evidence that the grant block finished. */
        if (grants_ok) {
            grants_ok = app_upgrade_grants_write_done(target_path) == 0;
        }
        if (!grants_ok) {
            bool rolled_back =
                app_upgrade_grants_restore(grants, manifest->id, capabilities, grant_snapshot,
                                           grant_found, capability_count) == 0;
            if (rolled_back && updated.prev_wasm_path[0]) {
                rolled_back = _restore_package(updated.wasm_path);
            }
            if (rolled_back) {
                rolled_back = mgr->repo.save_app && mgr->repo.save_app(mgr->repo.ctx, &old) == 0;
            }
            /* Any failed restore leaves the markers for recovery. */
            if (rolled_back) app_upgrade_markers_remove(target_path);
            _set_reason(reason, reason_cap, "grant inheritance failed");
            return -1;
        }
    }

    app_upgrade_markers_remove(target_path);
    _set_reason(reason, reason_cap, "");
    return 0;
}

int edge_app_mgr_upgrade(edge_app_mgr_t *mgr, const app_manifest_t *manifest,
                         const void *wasm_data, size_t wasm_len,
                         const app_capability_scope_t *capabilities, size_t capability_count,
                         const edge_authz_store_t *grants, char *reason, size_t reason_cap) {
    int rc = _upgrade_impl(mgr, manifest, wasm_data, wasm_len, capabilities, capability_count,
                           grants, reason, reason_cap);
    if (rc == 0) {
        app_log(mgr, 1, "AppMgr: upgraded app %s to %s", manifest->id, manifest->version);
    } else {
        app_log(mgr, 3, "AppMgr: upgrade %s failed: %s", manifest ? manifest->id : "-",
                reason ? reason : "");
    }
    return rc;
}

static int _rollback_impl(edge_app_mgr_t *mgr, const char *app_id,
                          char *reason, size_t reason_cap) {
    _set_reason(reason, reason_cap, "");
    if (!mgr || !app_id || !app_id[0]) {
        _set_reason(reason, reason_cap, "invalid app id");
        return -1;
    }

    app_manifest_t current;
    if (!mgr->repo.get_app || mgr->repo.get_app(mgr->repo.ctx, app_id, &current) != 0) {
        _set_reason(reason, reason_cap, "app not found in registry");
        return -1;
    }
    if (current.prev_version[0] == '\0') {
        _set_reason(reason, reason_cap, "no previous version");
        return -1;
    }

    app_manifest_t restored;
    if (_manifest_from_json(current.prev_manifest_json, &restored) != 0) {
        _set_reason(reason, reason_cap, "previous manifest is unreadable");
        return -1;
    }
    if (strcmp(restored.id, app_id) != 0) {
        _set_reason(reason, reason_cap, "previous manifest belongs to another app");
        return -1;
    }

    if (current.prev_wasm_path[0]) {
        char prev_path[264];
        snprintf(prev_path, sizeof(prev_path), "%s.prev", current.prev_wasm_path);
        if (rename(prev_path, current.wasm_path) != 0 && errno != ENOENT) {
            _set_reason(reason, reason_cap, "package restore failed");
            return -1;
        }
    }

    memset(restored.prev_version, 0, sizeof(restored.prev_version));
    memset(restored.prev_manifest_json, 0, sizeof(restored.prev_manifest_json));
    memset(restored.prev_wasm_path, 0, sizeof(restored.prev_wasm_path));
    if (!mgr->repo.save_app || mgr->repo.save_app(mgr->repo.ctx, &restored) != 0) {
        _set_reason(reason, reason_cap, "registry save failed");
        return -1;
    }
    return 0;
}

int edge_app_mgr_report_fault(edge_app_mgr_t *mgr, const char *app_id, const char *reason) {
    if (!mgr || !app_id) return -1;

    app_manifest_t m;
    if (!mgr->repo.get_app || mgr->repo.get_app(mgr->repo.ctx, app_id, &m) != 0) return -1;
    if (m.state == APP_STATE_UNINSTALLED) return -1;

    struct timespec now_ts;
    clock_gettime(CLOCK_REALTIME, &now_ts);
    uint64_t now_ms = (uint64_t)now_ts.tv_sec * 1000ULL + (uint64_t)(now_ts.tv_nsec / 1000000);
    if (m.last_fault_ms == 0 || now_ms - m.last_fault_ms > mgr->fault_window_ms) {
        m.restart_count = 1;
    } else {
        m.restart_count += 1;
    }
    m.last_fault_ms = now_ms;
    m.state = APP_STATE_FAULTED;
    if (!mgr->repo.save_app || mgr->repo.save_app(mgr->repo.ctx, &m) != 0) return -1;
    app_log(mgr, 3, "AppMgr: app %s faulted: %s", app_id, reason ? reason : "unknown");
    return 0;
}

int edge_app_mgr_start(edge_app_mgr_t *mgr, const char *app_id) {
    if (!mgr || !app_id) return -1;

    char reason[160] = "";
    int rc = _start_impl(mgr, app_id, reason, sizeof(reason));
    if (rc == 0) {
        app_log(mgr, 1, "AppMgr: Started app %s", app_id);
    } else {
        app_log(mgr, 3, "AppMgr: start %s failed: %s", app_id, reason);
    }
    return rc;
}

int edge_app_mgr_set_limits(edge_app_mgr_t *mgr, const char *app_id, const app_limits_t *limits) {
    if (!mgr || !app_id || !app_id[0] || !limits) return -1;
    if (!mgr->repo.save_limits) return 0;
    return mgr->repo.save_limits(mgr->repo.ctx, app_id, limits);
}

int edge_app_mgr_get_limits(edge_app_mgr_t *mgr, const char *app_id, app_limits_t *out_limits) {
    if (!mgr || !app_id || !out_limits) return -1;
    memset(out_limits, 0, sizeof(*out_limits));
    if (!mgr->repo.get_limits) return 0;
    return mgr->repo.get_limits(mgr->repo.ctx, app_id, out_limits);
}

static int _stop_impl(edge_app_mgr_t *mgr, const char *app_id, char *reason, size_t reason_cap,
                      bool clear_desired) {
    app_manifest_t m;
    if (!mgr->repo.get_app || mgr->repo.get_app(mgr->repo.ctx, app_id, &m) != 0) {
        if (reason) snprintf(reason, reason_cap, "app not found in registry");
        return -1;
    }
    if (m.state == APP_STATE_UNINSTALLED) {
        if (reason) snprintf(reason, reason_cap, "app is uninstalled; install again first");
        return -1;
    }

    m.state = APP_STATE_STOPPED;
    if (clear_desired) m.desired_state = false;
    if (!mgr->repo.save_app || mgr->repo.save_app(mgr->repo.ctx, &m) != 0) {
        if (reason) snprintf(reason, reason_cap, "registry save failed");
        return -1;
    }
    if (reason) reason[0] = '\0';
    return 0;
}

int edge_app_mgr_stop(edge_app_mgr_t *mgr, const char *app_id) {
    if (!mgr || !app_id) return -1;

    char reason[160] = "";
    int rc = _stop_impl(mgr, app_id, reason, sizeof(reason), true);
    if (rc == 0) {
        app_log(mgr, 1, "AppMgr: Stopped app %s", app_id);
    } else {
        app_log(mgr, 3, "AppMgr: stop %s failed: %s", app_id, reason);
    }
    return rc;
}

static const char *_disposition_name(app_uninstall_disposition_t disposition) {
    switch (disposition) {
    case APP_UNINSTALL_KEEP: return "keep";
    case APP_UNINSTALL_PURGE: return "purge";
    case APP_UNINSTALL_EXPORT: return "export";
    }
    return "unknown";
}

/* 审计在 loop 线程写；worker 只把 version 带回来，避免在 worker 调日志端口。 */
static void _audit_uninstall(edge_app_mgr_t *mgr, const char *app_id, const char *version,
                             const app_uninstall_opts_t *opts) {
    app_log(mgr, 1,
            "AppMgr: uninstall audit app=%s version=%s disposition=%s actor=%.*s at=%llu",
            app_id, version, _disposition_name(opts->disposition), (int)sizeof(opts->actor),
            opts->actor[0] ? opts->actor : "-", (unsigned long long)time(NULL));
}

static int _uninstall_impl(edge_app_mgr_t *mgr, const char *app_id,
                           const app_uninstall_opts_t *opts, char *version_out,
                           size_t version_cap, char *reason, size_t reason_cap) {
    (void)opts;
    app_manifest_t m;
    if (!mgr->repo.get_app || mgr->repo.get_app(mgr->repo.ctx, app_id, &m) != 0) {
        if (reason) snprintf(reason, reason_cap, "app not found in registry");
        return -1;
    }
    if (version_out && version_cap) snprintf(version_out, version_cap, "%s", m.version);

    /* 注册表层对所有处置相同：只删代码包并留只读墓碑。PURGE 与 EXPORT 的
     * 私有数据由组合根在卸载回调里删除，管理器不持有那些存储。 */
    m.state = APP_STATE_UNINSTALLED;
    if (!mgr->repo.save_app) {
        if (reason) snprintf(reason, reason_cap, "repository save failed");
        return -1;
    }
    int rc = mgr->repo.save_app(mgr->repo.ctx, &m);
    if (rc != 0) {
        if (reason) snprintf(reason, reason_cap, "repository save failed");
        return rc;
    }
    if (m.wasm_path[0] && unlink(m.wasm_path) != 0 && errno != ENOENT) {
        if (reason) snprintf(reason, reason_cap, "package delete failed: %.120s", m.wasm_path);
        return -1;
    }
    if (mgr->repo.delete_limits) mgr->repo.delete_limits(mgr->repo.ctx, app_id);
    if (reason) reason[0] = '\0';
    return 0;
}

int edge_app_mgr_uninstall_ex(edge_app_mgr_t *mgr, const char *app_id,
                              const app_uninstall_opts_t *opts) {
    if (!mgr || !app_id) return -1;

    static const app_uninstall_opts_t keep = {0};
    if (!opts) opts = &keep;

    char reason[160] = "";
    char version[sizeof(((app_manifest_t *)0)->version)] = "";
    int rc = _uninstall_impl(mgr, app_id, opts, version, sizeof(version), reason, sizeof(reason));
    if (rc == 0) {
        _audit_uninstall(mgr, app_id, version, opts);
    } else {
        app_log(mgr, 3, "AppMgr: uninstall %s failed: %s", app_id, reason);
    }
    return rc;
}

int edge_app_mgr_uninstall(edge_app_mgr_t *mgr, const char *app_id) {
    return edge_app_mgr_uninstall_ex(mgr, app_id, NULL);
}

static const char *_job_kind_name(app_job_kind_t kind) {
    switch (kind) {
    case APP_JOB_INSTALL: return "install";
    case APP_JOB_START: return "start";
    case APP_JOB_STOP: return "stop";
    case APP_JOB_UNINSTALL: return "uninstall";
    case APP_JOB_RECLAIM_STOP: return "reclaim_stop";
    case APP_JOB_UPGRADE: return "upgrade";
    case APP_JOB_ROLLBACK: return "rollback";
    }
    return "job";
}

static void _job_free(app_job_t *job) {
    if (!job) return;
    free(job->wasm);
    if (job->mgr) job->mgr->job_active = false;
    memset(job, 0, sizeof(*job));
}

/* 只在 loop 线程调用。槽已被占用时释放 job 并返回 BUSY，不排队。 */
static void _job_work_cb(uv_work_t *req);
static void _job_after_cb(uv_work_t *req, int status);

static int _queue_job(edge_app_mgr_t *mgr, app_job_t *job) {
    if (!mgr || !mgr->loop) {
        _job_free(job);
        return -1;
    }
    if (mgr->job_active) return EDGE_APP_MGR_BUSY;

    job->mgr = mgr;
    job->req.data = job;
    mgr->job_active = true;
    if (uv_queue_work(mgr->loop, &job->req, _job_work_cb, _job_after_cb) != 0) {
        _job_free(job);
        return -1;
    }
    return 0;
}

static void _job_work_cb(uv_work_t *req) {
    app_job_t *job = (app_job_t *)req->data;
    switch (job->kind) {
    case APP_JOB_INSTALL:
        job->rc = _install_impl(job->mgr, &job->manifest, job->wasm, job->wasm_len,
                                job->reason, sizeof(job->reason));
        break;
    case APP_JOB_START:
        job->rc = _start_impl(job->mgr, job->app_id, job->reason, sizeof(job->reason));
        break;
    case APP_JOB_STOP:
        job->rc = _stop_impl(job->mgr, job->app_id, job->reason, sizeof(job->reason), true);
        break;
    case APP_JOB_UNINSTALL:
        job->rc = _uninstall_impl(job->mgr, job->app_id, &job->uninstall, job->version,
                                  sizeof(job->version), job->reason, sizeof(job->reason));
        break;
    case APP_JOB_RECLAIM_STOP:
        job->rc = _stop_impl(job->mgr, job->app_id, job->reason, sizeof(job->reason), false);
        break;
    case APP_JOB_UPGRADE:
        job->rc = _upgrade_impl(job->mgr, &job->manifest, job->wasm, job->wasm_len,
                                job->capabilities, job->capability_count,
                                job->has_grants ? &job->grants : NULL, job->reason,
                                sizeof(job->reason));
        break;
    case APP_JOB_ROLLBACK:
        job->rc = _rollback_impl(job->mgr, job->app_id, job->reason, sizeof(job->reason));
        break;
    }
}

static void _job_after_cb(uv_work_t *req, int status) {
    app_job_t *job = (app_job_t *)req->data;
    edge_app_mgr_t *mgr = job->mgr;
    app_job_kind_t kind = job->kind;
    int rc = job->rc;
    char app_id[sizeof(job->app_id)];
    char reason[sizeof(job->reason)];
    char version[sizeof(job->version)];
    app_uninstall_opts_t uninstall = job->uninstall;
    edge_app_job_cb_t cb = job->cb;
    void *cb_ctx = job->cb_ctx;
    snprintf(app_id, sizeof(app_id), "%s", job->app_id);
    snprintf(reason, sizeof(reason), "%s", job->reason);
    snprintf(version, sizeof(version), "%s", job->version);

    /* 先清槽再回调：回调里可以立即提交下一个任务。 */
    free(job->wasm);
    memset(job, 0, sizeof(*job));
    mgr->job_active = false;

    if (status != 0) {
        rc = -1;
        snprintf(reason, sizeof(reason), "job cancelled");
    }

    if (rc == 0) {
        if (kind == APP_JOB_UNINSTALL) {
            _audit_uninstall(mgr, app_id, version, &uninstall);
        } else {
            app_log(mgr, 1, "AppMgr: async %s ok for %s", _job_kind_name(kind), app_id);
        }
    } else {
        app_log(mgr, 3, "AppMgr: async %s failed for %s: %s", _job_kind_name(kind), app_id,
                reason);
    }

    if (cb) cb(rc, app_id, reason, cb_ctx);
}

static app_job_t *_job_new(edge_app_mgr_t *mgr, app_job_kind_t kind, const char *app_id,
                           edge_app_job_cb_t cb, void *ctx) {
    if (!mgr || mgr->job_active) return NULL;
    app_job_t *job = &mgr->job;
    memset(job, 0, sizeof(*job));
    job->mgr = mgr;
    job->kind = kind;
    snprintf(job->app_id, sizeof(job->app_id), "%s", app_id);
    job->cb = cb;
    job->cb_ctx = ctx;
    return job;
}

int edge_app_mgr_install_async(edge_app_mgr_t *mgr, const app_manifest_t *manifest,
                               const void *wasm_data, size_t wasm_len,
                               edge_app_job_cb_t cb, void *ctx) {
    if (!mgr || !manifest || !_valid_app_id(manifest->id)) return -1;
    if (wasm_len > 0 && !wasm_data) return -1;
    if (mgr->job_active) return EDGE_APP_MGR_BUSY;

    app_job_t *job = _job_new(mgr, APP_JOB_INSTALL, manifest->id, cb, ctx);
    if (!job) return -1;

    if (wasm_len > 0) {
        job->wasm = malloc(wasm_len);
        if (!job->wasm) {
            _job_free(job);
            return -1;
        }
        memcpy(job->wasm, wasm_data, wasm_len);
        job->wasm_len = wasm_len;
    }
    job->manifest = *manifest;

    return _queue_job(mgr, job);
}

static int _queue_state_job(edge_app_mgr_t *mgr, app_job_kind_t kind, const char *app_id,
                            edge_app_job_cb_t cb, void *ctx);

int edge_app_mgr_upgrade_async(edge_app_mgr_t *mgr, const app_manifest_t *manifest,
                               const app_capability_scope_t *capabilities,
                               size_t capability_count, const edge_authz_store_t *grants,
                               const void *wasm_data, size_t wasm_len,
                               edge_app_job_cb_t cb, void *ctx) {
    if (!mgr || !manifest || !_valid_app_id(manifest->id)) return -1;
    if (wasm_len > 0 && !wasm_data) return -1;
    if (mgr->job_active) return EDGE_APP_MGR_BUSY;

    app_job_t *job = _job_new(mgr, APP_JOB_UPGRADE, manifest->id, cb, ctx);
    if (!job) return -1;
    job->manifest = *manifest;
    if (grants) {
        job->grants = *grants;
        job->has_grants = true;
    }

    if (capability_count > 0 && capabilities) {
        if (capability_count > APP_JOB_MAX_CAPABILITIES) {
            _job_free(job);
            return -1;
        }
        memcpy(job->capabilities, capabilities, sizeof(*capabilities) * capability_count);
        job->capability_count = capability_count;
    }
    if (wasm_len > 0) {
        job->wasm = malloc(wasm_len);
        if (!job->wasm) {
            _job_free(job);
            return -1;
        }
        memcpy(job->wasm, wasm_data, wasm_len);
        job->wasm_len = wasm_len;
    }
    return _queue_job(mgr, job);
}

int edge_app_mgr_rollback_async(edge_app_mgr_t *mgr, const char *app_id,
                                edge_app_job_cb_t cb, void *ctx) {
    return _queue_state_job(mgr, APP_JOB_ROLLBACK, app_id, cb, ctx);
}

static int _queue_state_job(edge_app_mgr_t *mgr, app_job_kind_t kind, const char *app_id,
                            edge_app_job_cb_t cb, void *ctx) {
    if (!mgr || !app_id || !app_id[0]) return -1;
    if (mgr->job_active) return EDGE_APP_MGR_BUSY;
    app_job_t *job = _job_new(mgr, kind, app_id, cb, ctx);
    if (!job) return -1;
    return _queue_job(mgr, job);
}

int edge_app_mgr_start_async(edge_app_mgr_t *mgr, const char *app_id,
                             edge_app_job_cb_t cb, void *ctx) {
    return _queue_state_job(mgr, APP_JOB_START, app_id, cb, ctx);
}

int edge_app_mgr_stop_async(edge_app_mgr_t *mgr, const char *app_id,
                            edge_app_job_cb_t cb, void *ctx) {
    return _queue_state_job(mgr, APP_JOB_STOP, app_id, cb, ctx);
}

int edge_app_mgr_reclaim_stop_async(edge_app_mgr_t *mgr, const char *app_id,
                                    edge_app_job_cb_t cb, void *ctx) {
    return _queue_state_job(mgr, APP_JOB_RECLAIM_STOP, app_id, cb, ctx);
}

int edge_app_mgr_uninstall_async_ex(edge_app_mgr_t *mgr, const char *app_id,
                                    const app_uninstall_opts_t *opts,
                                    edge_app_job_cb_t cb, void *ctx) {
    if (!mgr || !app_id || !app_id[0]) return -1;
    if (mgr->job_active) return EDGE_APP_MGR_BUSY;
    app_job_t *job = _job_new(mgr, APP_JOB_UNINSTALL, app_id, cb, ctx);
    if (!job) return -1;
    if (opts) job->uninstall = *opts;
    return _queue_job(mgr, job);
}

int edge_app_mgr_uninstall_async(edge_app_mgr_t *mgr, const char *app_id,
                                 edge_app_job_cb_t cb, void *ctx) {
    return edge_app_mgr_uninstall_async_ex(mgr, app_id, NULL, cb, ctx);
}

/* 安装崩溃留下的孤儿包没有 Registry 行，按目录扫描清理。升级标记对应的行
 * 会先在上面按版本处理，这里跳过仍被 Registry 引用的路径。 */
static int _recover_orphan_installs(edge_app_mgr_t *mgr, const app_manifest_t *apps, int count) {
    DIR *d = opendir(mgr->apps_dir);
    if (!d) return 0;
    int recovered = 0;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char dir[520];  // stack-ok: bounded by apps_dir[256] + NAME_MAX; startup recovery only
        char wasm[sizeof(dir) + 16];
        char side[sizeof(wasm) + 24];
        snprintf(dir, sizeof(dir), "%s/%s", mgr->apps_dir, entry->d_name);
        snprintf(wasm, sizeof(wasm), "%s/app.wasm", dir);
        snprintf(side, sizeof(side), "%s.pending", wasm);
        if (access(side, F_OK) != 0) continue;
        bool referenced = false;
        for (int i = 0; i < count; i++) {
            if (strcmp(apps[i].wasm_path, wasm) == 0) {
                referenced = true;
                break;
            }
        }
        if (referenced) continue;
        unlink(wasm);
        snprintf(side, sizeof(side), "%s.new", wasm);
        unlink(side);
        snprintf(side, sizeof(side), "%s.pending", wasm);
        unlink(side);
        snprintf(side, sizeof(side), "%s.pending.grants", wasm);
        unlink(side);
        snprintf(side, sizeof(side), "%s.pending.grants.done", wasm);
        unlink(side);
        rmdir(dir);
        app_log(mgr, 2, "AppMgr: removed orphan package %s", wasm);
        recovered++;
    }
    closedir(d);
    return recovered;
}

int edge_app_mgr_recover_packages(edge_app_mgr_t *mgr, const edge_authz_store_t *grants) {
    if (!mgr || !mgr->repo.list_apps) return 0;
    app_manifest_t apps[64];
    int count = mgr->repo.list_apps(mgr->repo.ctx, apps, 64);
    int recovered = 0;
    for (int i = 0; i < count; i++) {
        if (apps[i].wasm_path[0] == '\0') continue;
        char pending[272];
        _pending_path(apps[i].wasm_path, pending, sizeof(pending));
        bool pending_committed = false;
        if (access(pending, F_OK) == 0) {
            char pending_version[64];
            bool have_version =
                _read_pending_version(pending, pending_version, sizeof(pending_version)) == 0;
            char grants_path[288];
            char done_path[296];
            app_upgrade_grants_path(apps[i].wasm_path, grants_path, sizeof(grants_path));
            app_upgrade_grants_done_path(apps[i].wasm_path, done_path, sizeof(done_path));

            if (access(done_path, F_OK) == 0) {
                /* The grant block finished; the upgrade is committed. */
                pending_committed = true;
                app_upgrade_markers_remove(apps[i].wasm_path);
            } else if (access(grants_path, F_OK) == 0) {
                /* Registry committed, grant phase did not. Put grants, package,
                 * and registry row back; a failed step keeps the markers so the
                 * next start retries. */
                bool restored =
                    app_upgrade_grants_restore_snapshot(grants, apps[i].id, grants_path) == 0;
                if (restored) {
                    if (have_version && strcmp(apps[i].version, pending_version) == 0) {
                        char reason[160];
                        restored = _rollback_impl(mgr, apps[i].id, reason, sizeof(reason)) == 0;
                    } else {
                        restored = _restore_package(apps[i].wasm_path);
                    }
                }
                if (restored) {
                    app_upgrade_markers_remove(apps[i].wasm_path);
                    app_log(mgr, 2, "AppMgr: rolled back interrupted upgrade with grants %s",
                            apps[i].wasm_path);
                    recovered++;
                } else {
                    app_log(mgr, 4, "AppMgr: grant rollback incomplete for %s; markers kept",
                            apps[i].wasm_path);
                }
            } else {
                /* Install crash or grantless upgrade: the registry version decides. */
                bool committed = have_version && strcmp(apps[i].version, pending_version) == 0;
                if (committed) {
                    pending_committed = true;
                    app_upgrade_markers_remove(apps[i].wasm_path);
                } else if (_restore_package(apps[i].wasm_path)) {
                    app_upgrade_markers_remove(apps[i].wasm_path);
                    app_log(mgr, 2, "AppMgr: rolled back interrupted upgrade %s",
                            apps[i].wasm_path);
                    recovered++;
                }
            }
        }
        char side[264];
        snprintf(side, sizeof(side), "%s.new", apps[i].wasm_path);
        unlink(side);
        if (access(apps[i].wasm_path, F_OK) == 0) continue;
        if (pending_committed) {
            app_log(mgr, 4, "AppMgr: committed upgrade has no package at %s", apps[i].wasm_path);
            continue;
        }
        snprintf(side, sizeof(side), "%s.prev", apps[i].wasm_path);
        if (access(side, F_OK) == 0 && rename(side, apps[i].wasm_path) == 0) {
            app_log(mgr, 2, "AppMgr: recovered package %s after an interrupted upgrade",
                    apps[i].wasm_path);
            recovered++;
        }
    }
    recovered += _recover_orphan_installs(mgr, apps, count);
    return recovered;
}

int edge_app_mgr_reconcile(edge_app_mgr_t *mgr, edge_app_control_fn is_running,
                           edge_app_control_fn start, edge_app_control_fn stop, void *ctx,
                           int *acted) {
    if (acted) *acted = 0;
    if (!mgr || !mgr->repo.list_apps) return -1;
    app_manifest_t apps[64];
    int count = mgr->repo.list_apps(mgr->repo.ctx, apps, 64);
    int changes = 0;
    for (int i = 0; i < count; i++) {
        app_manifest_t *app = &apps[i];
        if (app->state == APP_STATE_UNINSTALLED || app->state == APP_STATE_FAULTED) continue;
        int running = is_running ? is_running(ctx, app->id)
                                 : (app->state == APP_STATE_RUNNING);
        if (app->desired_state && app->state == APP_STATE_RUNNING && !running) {
            if (start && start(ctx, app->id) == 0) changes++;
        } else if (!app->desired_state && running) {
            if (stop && stop(ctx, app->id) == 0) changes++;
        }
    }
    if (acted) *acted = changes;
    return 0;
}

bool edge_app_mgr_is_busy(const edge_app_mgr_t *mgr) {
    return mgr && mgr->job_active;
}

int edge_app_mgr_boot_restore(edge_app_mgr_t *mgr) {
    if (!mgr || !mgr->repo.list_apps) return 0;
    app_manifest_t apps[64];
    int count = mgr->repo.list_apps(mgr->repo.ctx, apps, 64);
    int restored = 0;

    for (int i = 0; i < count; i++) {
        if (apps[i].state == APP_STATE_RUNNING) {
            app_log(mgr, 1, "AppMgr: Boot restoring RUNNING app %s", apps[i].id);
            edge_app_mgr_start(mgr, apps[i].id);
            restored++;
        }
    }
    app_log(mgr, 1, "AppMgr: Boot restore complete, restored %d apps", restored);
    return restored;
}

int edge_app_mgr_get_app(edge_app_mgr_t *mgr, const char *app_id, app_manifest_t *out_manifest) {
    if (!mgr || !mgr->repo.get_app || !app_id || !out_manifest) return -1;
    return mgr->repo.get_app(mgr->repo.ctx, app_id, out_manifest);
}
