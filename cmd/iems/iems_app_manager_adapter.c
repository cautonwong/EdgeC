/**
 * @file iems_app_manager_adapter.c
 * @brief Composition-side wiring for the IEMS app manager and its sqlite repo.
 */

#include "iems_app_manager_adapter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <cjson/cJSON.h>

#include "app_repo_sqlite.h"
#include "app_points_sqlite.h"
#include "iems_app_runtime.h"
#include "edge_app_task_sqlite.h"
#include "edge_authz_sqlite.h"
#include "edge_registry_snapshot.h"
#include "edge_secret_sqlite.h"
#include "edge_log.h"
#include "edgeos/app_manifest.h"
#include "edgeos/app_secret.h"
#include "edgeos/app_task.h"
#include "edge_time_trust.h"
#include "edgeos/edge_app_export.h"
#include "edgeos/edge_app_mgr.h"
#include "edgeos/edge_authz.h"
#include "edgeos/edge_eapp.h"
#include "sqlite_service.h"
#include "stream_engine.h"

static void iems_app_log(void *ctx, int level, const char *message) {
    (void)ctx;
    edge_log_out(NULL, LOG_CAT_CORE, level, "%s", message);
}

static int64_t _now_ms(void);
static int _control_upgrade_app(void *ctx, const char *manifest_json, const void *wasm,
                                size_t wasm_len, const char *idempotency_key);

edge_app_mgr_t *iems_app_manager_create(uv_loop_t *loop, sqlite_svc_t *db,
                                        stream_engine_t *engine, void **storage_out) {
    if (storage_out) *storage_out = NULL;
    if (!db) return NULL;

    size_t storage_size = edge_app_mgr_storage_size();
    void *storage = malloc(storage_size);
    if (!storage) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: storage allocation failed; app API disabled");
        return NULL;
    }

    edge_app_repo_t repo = app_repo_sqlite(db);
    edge_app_mgr_t *mgr =
        edge_app_mgr_create_with_repo_in(storage, storage_size, loop, &repo, engine);
    if (!mgr) {
        free(storage);
        return NULL;
    }

    edge_app_logger_t logger = {.log = iems_app_log, .ctx = NULL};
    edge_app_mgr_set_logger(mgr, &logger);
    const char *apps_dir = getenv("EDGE_APPS_DIR");
    if (apps_dir && apps_dir[0]) edge_app_mgr_set_apps_dir(mgr, apps_dir);
    if (getenv("EDGEC_TEST_CRASH_AFTER_PACKAGE_WRITE")) {
        edge_app_mgr_set_crash_after_package_write(mgr, true);
    }
    LOG_INFO(LOG_CAT_CORE, "AppMgr: Initialized with repository successfully");

    if (storage_out) *storage_out = storage;
    return mgr;
}

void iems_app_manager_destroy(edge_app_mgr_t *mgr, void *storage) {
    edge_app_mgr_destroy(mgr);
    free(storage);
}

static char *_control_apps_json(void *ctx) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->mgr) return strdup("[]");

    const edge_app_repo_t *repo = edge_app_mgr_get_repo(control->mgr);
    app_manifest_t apps[64];
    int count = (repo && repo->list_apps) ? repo->list_apps(repo->ctx, apps, 64) : 0;

    cJSON *arr = cJSON_CreateArray();
    if (!arr) return strdup("[]");
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_CreateObject();
        if (!item) continue;
        cJSON_AddStringToObject(item, "id", apps[i].id);
        cJSON_AddStringToObject(item, "name", apps[i].name);
        cJSON_AddStringToObject(item, "version", apps[i].version);
        cJSON_AddStringToObject(item, "author", apps[i].author);
        cJSON_AddStringToObject(item, "wasm_path", apps[i].wasm_path);
        cJSON_AddNumberToObject(item, "trigger_tag", apps[i].trigger_tag);
        cJSON_AddStringToObject(item, "entry_func", apps[i].entry_func);
        cJSON_AddStringToObject(
            item, "fault_policy",
            apps[i].fault_policy == FAULT_POLICY_ISOLATE_AND_ALARM ? "isolate_and_alarm"
                                                                  : "auto_restart");
        cJSON_AddNumberToObject(item, "state", apps[i].state);
        cJSON_AddNumberToObject(item, "restart_count", apps[i].restart_count);
        cJSON_AddItemToArray(arr, item);
    }

    char *json = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    return json;
}

typedef struct {
    iems_control_ctx_t *control;
    app_manifest_t manifest;
    app_spec_t spec;
    char task_id[EDGE_APP_TASK_ID_SIZE];
} iems_install_job_ctx_t;

/* 台账终态；无 task_id 表示该请求没有台账行。 */
static void _finish_task(iems_control_ctx_t *control, const char *task_id,
                         edge_app_task_state_t state, const char *reason) {
    if (!control || !control->db || !task_id || !task_id[0]) return;
    char *result = NULL;
    cJSON *root = cJSON_CreateObject();
    if (root) {
        cJSON_AddStringToObject(root, "reason", reason ? reason : "");
        result = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
    }
    edge_app_task_store_t store = edge_app_task_sqlite(control->db);
    edge_app_task_finish(&store, task_id, state, result, _now_ms());
    free(result);
}

typedef enum {
    TASK_BEGIN_RUN = 0,
    TASK_BEGIN_REPLAY_OK,
    TASK_BEGIN_REPLAY_FAIL,
    TASK_BEGIN_ERROR,
} task_begin_t;

/* 提交或重放幂等键；缺省键由适配器生成，保证每次操作都有台账行。 */
static task_begin_t _task_begin(iems_control_ctx_t *control, edge_app_task_kind_t kind,
                                const char *idempotency_key, edge_app_task_t *task) {
    if (!control || !control->db) return TASK_BEGIN_ERROR;
    static unsigned int auto_counter;
    char generated[EDGE_APP_TASK_KEY_SIZE];
    if (!idempotency_key || !idempotency_key[0]) {
        snprintf(generated, sizeof(generated), "auto-%lld-%ld-%u", (long long)time(NULL),
                 (long)getpid(), ++auto_counter);
        idempotency_key = generated;
    }

    edge_app_task_store_t store = edge_app_task_sqlite(control->db);
    int replayed = 0;
    if (edge_app_task_submit(&store, kind, idempotency_key, _now_ms(), task, &replayed) !=
        EDGE_APP_TASK_OK)
        return TASK_BEGIN_ERROR;
    if (!replayed) return TASK_BEGIN_RUN;

    switch (task->state) {
    case EDGE_APP_TASK_SUCCEEDED: return TASK_BEGIN_REPLAY_OK;
    case EDGE_APP_TASK_FAILED: return TASK_BEGIN_REPLAY_FAIL;
    default: return TASK_BEGIN_REPLAY_OK; /* PENDING/RUNNING: already in progress */
    }
}

static void _reload_stage(iems_control_ctx_t *control, const app_manifest_t *manifest) {
    if (!control || !control->engine || !manifest) return;
    if (control->app_runtime) return; /* app-host owns execution when configured */
    pipeline_stage_t *stage = stage_wasm_create(manifest->wasm_path, manifest->entry_func);
    if (!stage) return;
    stream_engine_set_pipeline_trigger(control->engine, manifest->id, manifest->trigger_tag);
    stream_engine_replace_pipeline_stage(control->engine, manifest->id, stage);
    free(stage);
}

/* Registry snapshots are a cache over the database; a publish failure must not
 * fail the admin operation that triggered it. */
static void _publish_snapshot(iems_control_ctx_t *control) {
    if (!control || !control->db) return;
    const char *dir = getenv("EDGEC_REGISTRY_DIR");
    if (!dir || !dir[0]) dir = "/run/edgeos/registry";
    char path[512];
    char reason[160];
    if (edge_registry_snapshot_publish(control->db, dir, path, sizeof(path), reason,
                                       sizeof(reason)) != 0) {
        LOG_WARN(LOG_CAT_CORE, "Registry: snapshot publish failed: %s", reason);
        return;
    }
    LOG_INFO(LOG_CAT_CORE, "Registry: published %s", path);
}

typedef struct {
    iems_control_ctx_t *control;
    char app_id[64];
    char task_id[EDGE_APP_TASK_ID_SIZE];
} iems_start_ctx_t;

static void _iems_start_done(int rc, const char *app_id, const char *reason, void *ctx) {
    iems_start_ctx_t *start = (iems_start_ctx_t *)ctx;
    if (rc != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: async start failed for %s: %s", app_id, reason);
        if (start->task_id[0]) {
            _finish_task(start->control, start->task_id, EDGE_APP_TASK_FAILED, reason);
        }
        free(start);
        return;
    }

    app_manifest_t manifest;
    if (edge_app_mgr_get_app(start->control->mgr, app_id, &manifest) != 0) {
        if (start->task_id[0]) {
            _finish_task(start->control, start->task_id, EDGE_APP_TASK_FAILED,
                         "registry read failed");
        }
        free(start);
        return;
    }

    if (start->control->app_runtime) {
        uint32_t heap_kb = manifest.heap_limit_kb > 0 ? manifest.heap_limit_kb : 64;
        if (iems_app_runtime_start(start->control->app_runtime, app_id, manifest.wasm_path, heap_kb,
                                   64) != 0) {
            LOG_WARN(LOG_CAT_CORE, "AppHost: start failed for %s", app_id);
            edge_app_mgr_stop(start->control->mgr, app_id);
            if (start->task_id[0]) {
                _finish_task(start->control, start->task_id, EDGE_APP_TASK_FAILED,
                             "app-host start failed");
            }
            free(start);
            return;
        }
    } else {
        _reload_stage(start->control, &manifest);
    }

    _publish_snapshot(start->control);
    if (start->task_id[0]) {
        _finish_task(start->control, start->task_id, EDGE_APP_TASK_SUCCEEDED, NULL);
    }
    free(start);
}

static void _iems_install_done(int rc, const char *app_id, const char *reason, void *ctx) {
    iems_install_job_ctx_t *job = (iems_install_job_ctx_t *)ctx;
    if (rc != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: async install failed for %s: %s", app_id, reason);
        _finish_task(job->control, job->task_id, EDGE_APP_TASK_FAILED, reason);
        free(job);
        return;
    }

    char points_reason[160];
    if (app_points_sync(job->control->db, app_id, job->spec.points, job->spec.points_count,
                        points_reason, sizeof(points_reason)) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: point registration failed for %s: %s", app_id,
                 points_reason);
        _finish_task(job->control, job->task_id, EDGE_APP_TASK_FAILED, points_reason);
        free(job);
        return;
    }

    if (job->control->app_runtime) iems_app_runtime_stop(job->control->app_runtime, app_id);

    iems_start_ctx_t *start = calloc(1, sizeof(*start));
    if (!start) {
        _finish_task(job->control, job->task_id, EDGE_APP_TASK_FAILED, "out of memory");
        free(job);
        return;
    }
    start->control = job->control;
    snprintf(start->app_id, sizeof(start->app_id), "%s", app_id);
    snprintf(start->task_id, sizeof(start->task_id), "%s", job->task_id);
    if (edge_app_mgr_start_async(job->control->mgr, app_id, _iems_start_done, start) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: async start queue failed for %s", app_id);
        _finish_task(job->control, job->task_id, EDGE_APP_TASK_FAILED, "start queue failed");
        free(start);
    }
    free(job);
}

static void _iems_state_done(int rc, const char *app_id, const char *reason, void *ctx) {
    (void)ctx;
    if (rc != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: async state change failed for %s: %s", app_id, reason);
    }
}

static int _control_start_app(void *ctx, const char *app_id) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->mgr || !app_id) return -1;

    iems_start_ctx_t *start = calloc(1, sizeof(*start));
    if (!start) return -1;
    start->control = control;
    snprintf(start->app_id, sizeof(start->app_id), "%s", app_id);
    if (edge_app_mgr_start_async(control->mgr, app_id, _iems_start_done, start) != 0) {
        free(start);
        return -1;
    }
    return 0;
}

static int _control_stop_app(void *ctx, const char *app_id) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->mgr) return -1;
    if (control->app_runtime) iems_app_runtime_stop(control->app_runtime, app_id);
    return edge_app_mgr_stop_async(control->mgr, app_id, _iems_state_done, NULL);
}

typedef struct {
    iems_control_ctx_t *control;
    app_uninstall_disposition_t disposition;
} iems_uninstall_ctx_t;

/* PURGE 的私有数据层：secrets、授权、包目录。点位槽位在上面的回调里释放；
 * 历史与任务台账按保留策略留下。 */
static void _purge_private_data(iems_control_ctx_t *control, const char *app_id) {
    edge_secret_store_t secrets = edge_secret_sqlite(control->db);
    char names[64][EDGE_SECRET_NAME_SIZE];
    size_t count = 0;
    if (edge_secret_list(&secrets, app_id, names, 64, &count) == EDGE_SECRET_OK) {
        for (size_t i = 0; i < count; i++) edge_secret_delete(&secrets, app_id, names[i]);
    }

    edge_authz_store_t authz = edge_authz_sqlite_store(control->db);
    edge_app_grant_t grants[32];
    size_t grants_count = 0;
    if (authz.list_grants &&
        authz.list_grants(authz.ctx, app_id, grants, 32, &grants_count) == 0) {
        for (size_t i = 0; i < grants_count; i++) {
            authz.delete_grant(authz.ctx, app_id, grants[i].capability);
        }
    }

    const char *apps_dir = getenv("EDGE_APPS_DIR");
    if (!apps_dir || !apps_dir[0]) apps_dir = "/var/edgeos/apps";
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/app-host", apps_dir, app_id);
    unlink(path);
    snprintf(path, sizeof(path), "%s/%s", apps_dir, app_id);
    if (rmdir(path) != 0 && errno != ENOENT) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: purge left %s: %s", path, strerror(errno));
    }
}

static void _iems_uninstall_done(int rc, const char *app_id, const char *reason, void *ctx) {
    iems_uninstall_ctx_t *job = (iems_uninstall_ctx_t *)ctx;
    iems_control_ctx_t *control = job->control;
    if (rc != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: uninstall failed for %s: %s", app_id, reason);
    } else {
        if (control->app_runtime) iems_app_runtime_stop(control->app_runtime, app_id);
        app_points_release(control->db, app_id);
        if (job->disposition == APP_UNINSTALL_PURGE ||
            job->disposition == APP_UNINSTALL_EXPORT) {
            _purge_private_data(control, app_id);
        }
    }
    _publish_snapshot(control);
    free(job);
}

static int _control_uninstall_app(void *ctx, const char *app_id, const char *disposition,
                                  const char *export_path) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->mgr || !app_id || !app_id[0]) return -1;

    app_uninstall_opts_t opts;
    memset(&opts, 0, sizeof(opts));
    if (disposition && strcmp(disposition, "purge") == 0) {
        opts.disposition = APP_UNINSTALL_PURGE;
    } else if (disposition && strcmp(disposition, "export") == 0) {
        opts.disposition = APP_UNINSTALL_EXPORT;
    }
    snprintf(opts.actor, sizeof(opts.actor), "appctl");
    if (opts.disposition == APP_UNINSTALL_EXPORT) {
        char reason[EDGE_APP_EXPORT_REASON_SIZE];
        if (!export_path || !export_path[0]) {
            LOG_WARN(LOG_CAT_CORE, "AppMgr: export needs an export path");
            return -1;
        }
        if (edge_app_export_archive(control->db, app_id, export_path, reason, sizeof(reason)) !=
            0) {
            LOG_WARN(LOG_CAT_CORE, "AppMgr: export failed for %s: %s", app_id, reason);
            return -1;
        }
    }
    if (control->app_runtime) iems_app_runtime_stop(control->app_runtime, app_id);
    iems_uninstall_ctx_t *job = calloc(1, sizeof(*job));
    if (!job) return -1;
    job->control = control;
    job->disposition = opts.disposition;
    int rc = edge_app_mgr_uninstall_async_ex(control->mgr, app_id, &opts, _iems_uninstall_done,
                                             job);
    if (rc != 0) free(job);
    return rc;
}

static int _clock_allows_install(iems_control_ctx_t *control, char *reason, size_t cap) {
    edge_time_info_t info;
    if (edge_time_trust_get(control->db, edge_time_now_wall_ms(), edge_time_now_monotonic_ms(),
                            &info, reason, cap) != 0) {
        return -1;
    }
    if (info.state == EDGE_TIME_UNTRUSTED) {
        if (reason && cap) snprintf(reason, cap, "clock is untrusted");
        return -1;
    }
    if (info.state == EDGE_TIME_DEGRADED) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: clock is degraded; install proceeds as an admin action");
    }
    return 0;
}

static char *_control_time_status_json(void *ctx) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->db) return NULL;
    edge_time_info_t info;
    char reason[EDGE_TIME_TRUST_REASON_SIZE];
    if (edge_time_trust_get(control->db, edge_time_now_wall_ms(), edge_time_now_monotonic_ms(),
                            &info, reason, sizeof(reason)) != 0) {
        return NULL;
    }
    cJSON *o = cJSON_CreateObject();
    if (!o) return NULL;
    cJSON_AddStringToObject(o, "state", edge_time_state_name(info.state));
    cJSON_AddBoolToObject(o, "has_rtc", info.has_rtc);
    cJSON_AddNumberToObject(o, "wall_ms", (double)info.wall_ms);
    cJSON_AddNumberToObject(o, "monotonic_ms", (double)info.monotonic_ms);
    cJSON_AddNumberToObject(o, "trusted_floor_ms", (double)info.trusted_floor_ms);
    cJSON_AddNumberToObject(o, "last_sync_ms", (double)info.last_sync_ms);
    char *json = cJSON_PrintUnformatted(o);
    cJSON_Delete(o);
    return json;
}

static int _control_time_mark(void *ctx, uint64_t epoch_ms, char *reason, size_t reason_size) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->db) return -1;
    uint64_t wall = epoch_ms ? epoch_ms : edge_time_now_wall_ms();
    int rc = edge_time_trust_mark(control->db, wall, edge_time_now_monotonic_ms(), reason,
                                  reason_size);
    if (rc == 0) {
        LOG_INFO(LOG_CAT_CORE, "AppMgr: clock marked trusted at %llu",
                 (unsigned long long)wall);
    }
    return rc;
}

static int _control_install_app(void *ctx, const char *manifest_json, const char *name,
                                uint32_t trigger_tag, const char *entry_func,
                                const void *wasm, size_t wasm_len,
                                const char *idempotency_key) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->mgr || !control->db) return -1;

    char clock_reason[EDGE_TIME_TRUST_REASON_SIZE];
    if (_clock_allows_install(control, clock_reason, sizeof(clock_reason)) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: install denied: %s", clock_reason);
        return -1;
    }

    /* T0: raw wasm has no manifest. Legacy request fields only existed for the
     * removed default-manifest path; the manifest is the sole install entry. */
    (void)name;
    (void)trigger_tag;
    (void)entry_func;

    if (!manifest_json || !manifest_json[0]) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: install denied: raw wasm upload has no manifest");
        return -1;
    }

    iems_install_job_ctx_t *job = calloc(1, sizeof(*job));
    if (!job) return -1;
    job->control = control;

    char reason[160];
    if (edge_app_manifest_parse(manifest_json, &job->spec, reason, sizeof(reason)) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: install denied: %s", reason);
        free(job);
        return -1;
    }

    if (edge_app_mgr_parse_manifest_json(manifest_json, &job->manifest) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: install denied: manifest has no usable id");
        free(job);
        return -1;
    }
    if (app_points_check(control->db, job->manifest.id, job->spec.points, job->spec.points_count,
                         reason, sizeof(reason)) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: install denied: %s", reason);
        free(job);
        return -1;
    }

    edge_app_task_t task;
    task_begin_t begin = _task_begin(control, EDGE_APP_TASK_INSTALL, idempotency_key, &task);
    if (begin == TASK_BEGIN_ERROR) {
        free(job);
        return -1;
    }
    if (begin == TASK_BEGIN_REPLAY_FAIL) {
        free(job);
        return -1;
    }
    if (begin == TASK_BEGIN_REPLAY_OK) {
        free(job);
        return 0;
    }
    snprintf(job->task_id, sizeof(job->task_id), "%s", task.id);

    int rc = edge_app_mgr_install_async(control->mgr, &job->manifest, wasm, wasm_len,
                                        _iems_install_done, job);
    if (rc != 0) {
        _finish_task(control, job->task_id, EDGE_APP_TASK_FAILED, "install queue failed");
        free(job);
    }
    return rc;
}

static int _parse_hex_key(const char *hex, uint8_t out[32]) {
    if (!hex || strlen(hex) != 64) return -1;
    for (size_t i = 0; i < 32; i++) {
        char pair[3] = {hex[i * 2], hex[i * 2 + 1], '\0'};
        char *end = NULL;
        unsigned long value = strtoul(pair, &end, 16);
        if (end != pair + 2 || value > 0xff) return -1;
        out[i] = (uint8_t)value;
    }
    return 0;
}

static void _keyring_add(iems_control_ctx_t *ctx, const char *publisher,
                         const uint8_t key[EDGE_EAPP_PUBLIC_KEY_LEN], uint8_t trust,
                         bool verified) {
    if (ctx->package_key_count >= EDGE_EAPP_KEYRING_MAX) return;
    edge_eapp_key_t *entry = &ctx->package_keys[ctx->package_key_count++];
    snprintf(entry->publisher_id, sizeof(entry->publisher_id), "%s", publisher);
    memcpy(entry->public_key, key, EDGE_EAPP_PUBLIC_KEY_LEN);
    entry->trust_level = trust;
    entry->verified = verified;
}

/* Keyring line: <publisher> <trust 0..3> <verified 0|1> <64 hex>. A single
 * EDGEC_PACKAGE_PUBKEY or _FILE entry stays supported for one-publisher units. */
static void _load_package_keyring(iems_control_ctx_t *ctx) {
    ctx->package_key_count = 0;
    const char *ring = getenv("EDGEC_PACKAGE_KEYRING_FILE");
    if (ring && ring[0]) {
        FILE *f = fopen(ring, "r");
        if (f) {
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                char publisher[EDGE_EAPP_PUBLISHER_SIZE];
                char hex[65];
                unsigned trust = 0;
                unsigned verified = 0;
                char *p = line;
                while (*p == ' ' || *p == '\t') p++;
                if (*p == '#' || *p == '\n' || *p == '\0') continue;
                if (sscanf(p, "%63s %u %u %64s", publisher, &trust, &verified, hex) != 4) continue;
                uint8_t key[EDGE_EAPP_PUBLIC_KEY_LEN];
                if (_parse_hex_key(hex, key) != 0) continue;
                _keyring_add(ctx, publisher, key, (uint8_t)(trust > 3 ? 3 : trust),
                             verified != 0);
            }
            fclose(f);
        }
        if (ctx->package_key_count > 0) return;
    }

    const char *hex = getenv("EDGEC_PACKAGE_PUBKEY");
    if (hex && hex[0]) {
        uint8_t key[EDGE_EAPP_PUBLIC_KEY_LEN];
        if (_parse_hex_key(hex, key) == 0) _keyring_add(ctx, "local", key, 1, true);
        return;
    }
    const char *path = getenv("EDGEC_PACKAGE_PUBKEY_FILE");
    if (!path || !path[0]) return;

    FILE *f = fopen(path, "rb");
    if (!f) return;
    uint8_t raw[65];
    size_t n = fread(raw, 1, sizeof(raw), f);
    fclose(f);
    uint8_t key[EDGE_EAPP_PUBLIC_KEY_LEN];
    if (n == 32) {
        memcpy(key, raw, 32);
        _keyring_add(ctx, "local", key, 1, true);
    } else if (n == 64 || (n == 65 && raw[64] == '\n')) {
        char text[65];
        memcpy(text, raw, 64);
        text[64] = '\0';
        if (_parse_hex_key(text, key) == 0) _keyring_add(ctx, "local", key, 1, true);
    }
}

static int _control_install_package(void *ctx, const uint8_t *data, size_t len,
                                    const char *idempotency_key,
                                    const uint8_t expected_pubkey[32]) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->mgr || !control->db) return -1;
    if (control->package_key_count == 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: package rejected: no signing key configured");
        return -1;
    }

    edge_eapp_package_t package;
    char reason[EDGE_EAPP_REASON_SIZE];
    if (edge_eapp_validate(data, len, control->package_keys, control->package_key_count, &package,
                           reason, sizeof(reason)) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: package rejected: %s", reason);
        return -1;
    }
    if (expected_pubkey &&
        memcmp(expected_pubkey, package.signer_key, EDGE_EAPP_PUBLIC_KEY_LEN) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: package rejected: unexpected signer %s",
                 package.publisher_id);
        edge_eapp_package_free(&package);
        return -1;
    }
    LOG_INFO(LOG_CAT_CORE, "AppMgr: package signer %s trust=%u verified=%d",
             package.publisher_id, package.trust_level, package.verified ? 1 : 0);
    app_spec_t signer;
    char signer_reason[160];
    if (edge_app_manifest_parse(package.manifest_json, &signer, signer_reason,
                                sizeof(signer_reason)) == 0) {
        if (signer.publisher_id[0] && package.publisher_id[0] &&
            strcmp(package.publisher_id, "local") != 0 &&
            strcmp(signer.publisher_id, package.publisher_id) != 0) {
            LOG_WARN(LOG_CAT_CORE, "AppMgr: package rejected: manifest publisher %s != key %s",
                     signer.publisher_id, package.publisher_id);
            edge_eapp_package_free(&package);
            return -1;
        }
        edge_authz_store_t store = edge_authz_sqlite_store(control->db);
        if (store.set_publisher_trust) {
            store.set_publisher_trust(store.ctx, signer.id, package.trust_level,
                                      package.verified);
        }
    }
    int rc = _control_install_app(ctx, package.manifest_json, NULL, 0, NULL, package.wasm,
                                  package.wasm_len, idempotency_key);
    edge_eapp_package_free(&package);
    return rc;
}

static int _control_upgrade_package(void *ctx, const char *app_id, const uint8_t *data, size_t len,
                                    const char *idempotency_key) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->mgr || !app_id || !app_id[0]) return -1;
    if (control->package_key_count == 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: upgrade rejected: no signing key configured");
        return -1;
    }

    edge_eapp_package_t package;
    char reason[EDGE_EAPP_REASON_SIZE];
    if (edge_eapp_validate(data, len, control->package_keys, control->package_key_count, &package,
                           reason, sizeof(reason)) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: upgrade rejected: %s", reason);
        return -1;
    }
    if (strcmp(package.publisher_id, "local") != 0) {
        app_spec_t signer;
        char signer_reason[160];
        if (edge_app_manifest_parse(package.manifest_json, &signer, signer_reason,
                                    sizeof(signer_reason)) == 0) {
            if (signer.publisher_id[0] && package.publisher_id[0] &&
                strcmp(signer.publisher_id, package.publisher_id) != 0) {
                LOG_WARN(LOG_CAT_CORE,
                         "AppMgr: upgrade rejected: manifest publisher %s != key %s",
                         signer.publisher_id, package.publisher_id);
                edge_eapp_package_free(&package);
                return -1;
            }
            edge_authz_store_t store = edge_authz_sqlite_store(control->db);
            if (store.set_publisher_trust) {
                store.set_publisher_trust(store.ctx, signer.id, package.trust_level,
                                          package.verified);
            }
        }
    }

    app_manifest_t manifest;
    if (edge_app_mgr_parse_manifest_json(package.manifest_json, &manifest) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: upgrade rejected: manifest has no usable id");
        edge_eapp_package_free(&package);
        return -1;
    }
    if (strcmp(manifest.id, app_id) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: upgrade rejected: package id %s != %s", manifest.id, app_id);
        edge_eapp_package_free(&package);
        return -1;
    }

    int rc = _control_upgrade_app(ctx, package.manifest_json, package.wasm, package.wasm_len,
                                  idempotency_key);
    edge_eapp_package_free(&package);
    return rc;
}

static char *_control_stream_topology_json(void *ctx) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->engine) return NULL;
    return stream_engine_dump_json(control->engine);
}

static int _control_approve_grant(void *ctx, const char *app_id, const char *capability,
                                  const char *scope_json, const char *purpose, uint64_t expires_at,
                                  uint8_t sensitivity_max, const char *actor) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->db || !app_id || !capability || !scope_json) return -1;

    edge_app_grant_t grant;
    memset(&grant, 0, sizeof(grant));
    snprintf(grant.app_id, sizeof(grant.app_id), "%s", app_id);
    snprintf(grant.capability, sizeof(grant.capability), "%s", capability);
    snprintf(grant.scope_json, sizeof(grant.scope_json), "%s", scope_json);
    snprintf(grant.purpose, sizeof(grant.purpose), "%s", purpose ? purpose : "");
    snprintf(grant.granted_by, sizeof(grant.granted_by), "%s", actor ? actor : "admin");
    grant.expires_at = expires_at;
    grant.sensitivity_max = sensitivity_max;

    edge_authz_store_t store = edge_authz_sqlite_store(control->db);
    edge_authz_result_t rc = edge_authz_approve_grant(&store, &grant, (uint64_t)time(NULL));
    if (rc != EDGE_AUTHZ_OK) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: grant rejected for %s/%s: %s", app_id, capability,
                 edge_authz_result_code(rc));
        return -1;
    }
    LOG_INFO(LOG_CAT_CORE, "AppMgr: grant approved for %s/%s by %s", app_id, capability,
             grant.granted_by);
    _publish_snapshot(control);
    return 0;
}

static int _control_revoke_grant(void *ctx, const char *app_id, const char *capability) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->db || !app_id || !capability) return -1;

    edge_authz_store_t store = edge_authz_sqlite_store(control->db);
    edge_authz_result_t rc =
        edge_authz_revoke_grant(&store, app_id, capability, (uint64_t)time(NULL));
    if (rc != EDGE_AUTHZ_OK) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: revoke rejected for %s/%s: %s", app_id, capability,
                 edge_authz_result_code(rc));
        return -1;
    }
    LOG_INFO(LOG_CAT_CORE, "AppMgr: grant revoked for %s/%s", app_id, capability);
    _publish_snapshot(control);
    return 0;
}

static char *_control_grants_json(void *ctx, const char *app_id) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->db) return strdup("[]");

    edge_authz_store_t store = edge_authz_sqlite_store(control->db);
    edge_app_grant_t grants[64];
    size_t count = 0;
    if (store.list_grants(store.ctx, app_id ? app_id : "", grants, 64, &count) != 0)
        return strdup("[]");

    cJSON *arr = cJSON_CreateArray();
    if (!arr) return strdup("[]");
    for (size_t i = 0; i < count; i++) {
        cJSON *item = cJSON_CreateObject();
        if (!item) continue;
        cJSON_AddStringToObject(item, "app_id", grants[i].app_id);
        cJSON_AddStringToObject(item, "capability", grants[i].capability);
        cJSON_AddStringToObject(item, "scope_json", grants[i].scope_json);
        cJSON_AddNumberToObject(item, "state", (double)grants[i].state);
        cJSON_AddStringToObject(item, "purpose", grants[i].purpose);
        cJSON_AddNumberToObject(item, "expires_at", (double)grants[i].expires_at);
        cJSON_AddNumberToObject(item, "sensitivity_max", (double)grants[i].sensitivity_max);
        cJSON_AddStringToObject(item, "granted_by", grants[i].granted_by);
        cJSON_AddNumberToObject(item, "granted_at", (double)grants[i].granted_at);
        cJSON_AddNumberToObject(item, "revoked_at", (double)grants[i].revoked_at);
        cJSON_AddItemToArray(arr, item);
    }

    char *json = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    return json ? json : strdup("[]");
}

static void _scopes_to_json(const char scopes[][APP_SPEC_SCOPE_SIZE], size_t count, char *out,
                            size_t capacity) {
    out[0] = '\0';
    cJSON *array = cJSON_CreateArray();
    if (!array) return;
    for (size_t i = 0; i < count; i++) {
        cJSON_AddItemToArray(array, cJSON_CreateString(scopes[i]));
    }
    char *printed = cJSON_PrintUnformatted(array);
    cJSON_Delete(array);
    if (printed) {
        snprintf(out, capacity, "%s", printed);
        free(printed);
    }
}

typedef struct {
    iems_control_ctx_t *control;
    char task_id[EDGE_APP_TASK_ID_SIZE];
    app_manifest_t manifest;
    app_spec_t spec;
    bool was_running;
} iems_upgrade_job_ctx_t;

static void _iems_upgrade_done(int rc, const char *app_id, const char *reason, void *ctx) {
    iems_upgrade_job_ctx_t *job = (iems_upgrade_job_ctx_t *)ctx;
    if (rc != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: async upgrade failed for %s: %s", app_id, reason);
        _finish_task(job->control, job->task_id, EDGE_APP_TASK_FAILED, reason);
        free(job);
        return;
    }

    char points_reason[160];
    if (app_points_sync(job->control->db, app_id, job->spec.points, job->spec.points_count,
                        points_reason, sizeof(points_reason)) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: point registration failed for %s: %s", app_id,
                 points_reason);
        _finish_task(job->control, job->task_id, EDGE_APP_TASK_FAILED, points_reason);
        free(job);
        return;
    }

    if (job->was_running) {
        if (job->control->app_runtime) iems_app_runtime_stop(job->control->app_runtime, app_id);
        iems_start_ctx_t *start = calloc(1, sizeof(*start));
        if (!start) {
            _finish_task(job->control, job->task_id, EDGE_APP_TASK_FAILED, "out of memory");
            free(job);
            return;
        }
        start->control = job->control;
        snprintf(start->app_id, sizeof(start->app_id), "%s", app_id);
        snprintf(start->task_id, sizeof(start->task_id), "%s", job->task_id);
        if (edge_app_mgr_start_async(job->control->mgr, app_id, _iems_start_done, start) != 0) {
            LOG_WARN(LOG_CAT_CORE, "AppMgr: reload queue failed for %s", app_id);
            _finish_task(job->control, job->task_id, EDGE_APP_TASK_FAILED, "reload queue failed");
            free(start);
        }
        free(job);
        return;
    }
    LOG_INFO(LOG_CAT_CORE, "AppMgr: upgraded %s", app_id);
    _publish_snapshot(job->control);
    _finish_task(job->control, job->task_id, EDGE_APP_TASK_SUCCEEDED, NULL);
    free(job);
}

typedef struct {
    iems_control_ctx_t *control;
    char app_id[64];
} iems_rollback_ctx_t;

static void _iems_rollback_done(int rc, const char *app_id, const char *reason, void *ctx) {
    iems_rollback_ctx_t *job = (iems_rollback_ctx_t *)ctx;
    if (rc != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: rollback failed for %s: %s", app_id, reason);
        free(job);
        return;
    }

    app_manifest_t manifest;
    _publish_snapshot(job->control);
    if (edge_app_mgr_get_app(job->control->mgr, job->app_id, &manifest) != 0 ||
        manifest.state != APP_STATE_RUNNING) {
        LOG_INFO(LOG_CAT_CORE, "AppMgr: rolled back %s", app_id);
        free(job);
        return;
    }
    if (job->control->app_runtime) iems_app_runtime_stop(job->control->app_runtime, app_id);
    iems_start_ctx_t *start = calloc(1, sizeof(*start));
    if (!start) {
        free(job);
        return;
    }
    start->control = job->control;
    snprintf(start->app_id, sizeof(start->app_id), "%s", job->app_id);
    if (edge_app_mgr_start_async(job->control->mgr, job->app_id, _iems_start_done, start) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: reload queue after rollback failed for %s", app_id);
        free(start);
    }
    free(job);
}

static int _control_upgrade_app(void *ctx, const char *manifest_json, const void *wasm,
                                size_t wasm_len, const char *idempotency_key) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->mgr || !control->db || !manifest_json) return -1;

    char reason[160];
    if (edge_app_manifest_validate_json(manifest_json, reason, sizeof(reason)) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: upgrade denied: %s", reason);
        return -1;
    }
    app_spec_t spec;
    if (edge_app_manifest_parse(manifest_json, &spec, reason, sizeof(reason)) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: upgrade denied: %s", reason);
        return -1;
    }
    app_manifest_t manifest;
    if (edge_app_mgr_parse_manifest_json(manifest_json, &manifest) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: upgrade denied: manifest has no usable id");
        return -1;
    }
    if (app_points_check(control->db, manifest.id, spec.points, spec.points_count, reason,
                         sizeof(reason)) != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: upgrade denied: %s", reason);
        return -1;
    }

    edge_app_task_t task;
    task_begin_t begin = _task_begin(control, EDGE_APP_TASK_UPGRADE, idempotency_key, &task);
    if (begin == TASK_BEGIN_ERROR) return -1;
    if (begin == TASK_BEGIN_REPLAY_FAIL) return -1;
    if (begin == TASK_BEGIN_REPLAY_OK) return 0;

    app_capability_scope_t capabilities[2];
    memset(capabilities, 0, sizeof(capabilities));
    size_t count = 0;
    snprintf(capabilities[count].capability, sizeof(capabilities[count].capability), "tags.read");
    _scopes_to_json(spec.capabilities.read, spec.capabilities.read_count,
                    capabilities[count].scope_json, sizeof(capabilities[count].scope_json));
    count++;
    snprintf(capabilities[count].capability, sizeof(capabilities[count].capability),
             "points.publish");
    _scopes_to_json(spec.capabilities.publish, spec.capabilities.publish_count,
                    capabilities[count].scope_json, sizeof(capabilities[count].scope_json));
    count++;

    app_manifest_t current;
    bool was_running = false;
    if (edge_app_mgr_get_app(control->mgr, manifest.id, &current) == 0) {
        was_running = current.state == APP_STATE_RUNNING;
    }

    iems_upgrade_job_ctx_t *job = calloc(1, sizeof(*job));
    if (!job) {
        _finish_task(control, task.id, EDGE_APP_TASK_FAILED, "out of memory");
        return -1;
    }
    job->control = control;
    job->manifest = manifest;
    job->spec = spec;
    job->was_running = was_running;
    snprintf(job->task_id, sizeof(job->task_id), "%s", task.id);

    edge_authz_store_t store = edge_authz_sqlite_store(control->db);
    int rc = edge_app_mgr_upgrade_async(control->mgr, &manifest, capabilities, count, &store, wasm,
                                        wasm_len, _iems_upgrade_done, job);
    if (rc != 0) {
        LOG_WARN(LOG_CAT_CORE, "AppMgr: upgrade queue failed for %s: %s", manifest.id,
                 rc == EDGE_APP_MGR_BUSY ? "busy" : "error");
        _finish_task(control, task.id, EDGE_APP_TASK_FAILED, "upgrade queue failed");
        free(job);
    }
    return rc;
}

static int _control_rollback_app(void *ctx, const char *app_id) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->mgr || !app_id || !app_id[0]) return -1;
    iems_rollback_ctx_t *job = calloc(1, sizeof(*job));
    if (!job) return -1;
    job->control = control;
    snprintf(job->app_id, sizeof(job->app_id), "%s", app_id);
    int rc = edge_app_mgr_rollback_async(control->mgr, app_id, _iems_rollback_done, job);
    if (rc != 0) free(job);
    return rc;
}

static int _control_secret_set(void *ctx, const char *app_id, const char *name,
                               const void *value, size_t len, char *reason,
                               size_t reason_size) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->db) return -1;
    edge_secret_store_t store = edge_secret_sqlite(control->db);
    edge_secret_result_t rc =
        edge_secret_set(&store, app_id, name, value, len, reason, reason_size);
    return rc == EDGE_SECRET_OK ? 0 : -1;
}

static int _control_secret_delete(void *ctx, const char *app_id, const char *name) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->db) return -1;
    edge_secret_store_t store = edge_secret_sqlite(control->db);
    return edge_secret_delete(&store, app_id, name) == EDGE_SECRET_OK ? 0 : -1;
}

static char *_control_secret_list(void *ctx, const char *app_id) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->db || !app_id) return strdup("[]");

    edge_secret_store_t store = edge_secret_sqlite(control->db);
    char names[EDGE_SECRET_PER_APP_MAX][EDGE_SECRET_NAME_SIZE];
    size_t count = 0;
    if (edge_secret_list(&store, app_id, names, EDGE_SECRET_PER_APP_MAX, &count) !=
        EDGE_SECRET_OK)
        return strdup("[]");

    cJSON *arr = cJSON_CreateArray();
    if (!arr) return strdup("[]");
    for (size_t i = 0; i < count; i++) {
        cJSON_AddItemToArray(arr, cJSON_CreateString(names[i]));
    }
    char *json = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    return json ? json : strdup("[]");
}

static int64_t _now_ms(void) {
    return (int64_t)time(NULL) * 1000;
}

static const char *_task_kind_name(edge_app_task_kind_t kind) {
    switch (kind) {
    case EDGE_APP_TASK_INSTALL: return "install";
    case EDGE_APP_TASK_UPGRADE: return "upgrade";
    case EDGE_APP_TASK_BACKUP: return "backup";
    default: return "unknown";
    }
}

static const char *_task_state_name(edge_app_task_state_t state) {
    switch (state) {
    case EDGE_APP_TASK_PENDING: return "pending";
    case EDGE_APP_TASK_RUNNING: return "running";
    case EDGE_APP_TASK_SUCCEEDED: return "succeeded";
    case EDGE_APP_TASK_FAILED: return "failed";
    default: return "unknown";
    }
}

static int _task_kind_from(const char *name) {
    if (!name) return -1;
    if (strcmp(name, "install") == 0) return EDGE_APP_TASK_INSTALL;
    if (strcmp(name, "upgrade") == 0) return EDGE_APP_TASK_UPGRADE;
    if (strcmp(name, "backup") == 0) return EDGE_APP_TASK_BACKUP;
    return -1;
}

static int _task_state_from(const char *name) {
    if (!name) return -1;
    if (strcmp(name, "running") == 0) return EDGE_APP_TASK_RUNNING;
    if (strcmp(name, "succeeded") == 0) return EDGE_APP_TASK_SUCCEEDED;
    if (strcmp(name, "failed") == 0) return EDGE_APP_TASK_FAILED;
    return -1;
}

static cJSON *_task_object(const edge_app_task_t *task) {
    cJSON *item = cJSON_CreateObject();
    if (!item) return NULL;
    cJSON_AddStringToObject(item, "id", task->id);
    cJSON_AddStringToObject(item, "kind", _task_kind_name(task->kind));
    cJSON_AddStringToObject(item, "idempotency_key", task->idempotency_key);
    cJSON_AddStringToObject(item, "state", _task_state_name(task->state));
    cJSON_AddNumberToObject(item, "created_at", (double)task->created_at);
    cJSON_AddNumberToObject(item, "updated_at", (double)task->updated_at);
    cJSON_AddStringToObject(item, "result_json", task->result_json);
    return item;
}

static char *_control_task_submit(void *ctx, const char *kind, const char *idempotency_key) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->db || !kind || !idempotency_key) return NULL;
    int kind_value = _task_kind_from(kind);
    if (kind_value < 0) return NULL;

    edge_app_task_store_t store = edge_app_task_sqlite(control->db);
    edge_app_task_t task;
    int replayed = 0;
    if (edge_app_task_submit(&store, (edge_app_task_kind_t)kind_value, idempotency_key, _now_ms(),
                             &task, &replayed) != EDGE_APP_TASK_OK)
        return NULL;

    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddBoolToObject(root, "replayed", replayed != 0);
    cJSON_AddItemToObject(root, "task", _task_object(&task));
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

static char *_control_task_status(void *ctx, const char *id) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->db || !id) return NULL;

    edge_app_task_store_t store = edge_app_task_sqlite(control->db);
    edge_app_task_t task;
    if (store.get(store.ctx, id, &task) != 1) return NULL;

    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddItemToObject(root, "task", _task_object(&task));
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

static int _control_task_finish(void *ctx, const char *id, const char *state,
                                const char *result_json) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->db || !id) return -1;
    int state_value = _task_state_from(state);
    if (state_value < 0) return -1;

    edge_app_task_store_t store = edge_app_task_sqlite(control->db);
    return edge_app_task_finish(&store, id, (edge_app_task_state_t)state_value, result_json,
                                _now_ms()) == EDGE_APP_TASK_OK
               ? 0
               : -1;
}

static char *_control_task_purge(void *ctx) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->db) return NULL;

    edge_app_task_store_t store = edge_app_task_sqlite(control->db);
    if (edge_app_task_purge(&store, _now_ms()) != EDGE_APP_TASK_OK) return NULL;
    return strdup("{\"status\":\"ok\"}");
}

static char *_control_registry_status(void *ctx, const char *app_id) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->db) return NULL;

    edge_registry_consumer_t view[IEMS_REGISTRY_CONSUMERS_MAX];
    uint64_t now = _now_ms();
    for (size_t i = 0; i < control->consumer_count; i++) {
        memset(&view[i], 0, sizeof(view[i]));
        snprintf(view[i].name, sizeof(view[i].name), "%s", control->consumers[i].name);
        view[i].applied_generation = control->consumers[i].applied_generation;
        if (now > control->consumers[i].last_seen_ms) {
            view[i].stale = now - control->consumers[i].last_seen_ms > IEMS_CONSUMER_STALE_MS;
        }
    }
    return edge_registry_status_json_for(control->db, view, control->consumer_count, app_id);
}

static int _control_consumer_heartbeat(void *ctx, const char *name,
                                       unsigned long long applied_generation) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !name || !name[0]) return -1;

    uint64_t now = _now_ms();
    for (size_t i = 0; i < control->consumer_count; i++) {
        if (strcmp(control->consumers[i].name, name) == 0) {
            control->consumers[i].applied_generation = applied_generation;
            control->consumers[i].last_seen_ms = now;
            return 0;
        }
    }
    if (control->consumer_count >= IEMS_REGISTRY_CONSUMERS_MAX) return -1;
    iems_registry_consumer_t *slot = &control->consumers[control->consumer_count++];
    memset(slot, 0, sizeof(*slot));
    snprintf(slot->name, sizeof(slot->name), "%s", name);
    slot->applied_generation = applied_generation;
    slot->last_seen_ms = now;
    return 0;
}

static char *_control_registry_snapshot(void *ctx) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->db) return NULL;

    const char *dir = getenv("EDGEC_REGISTRY_DIR");
    if (!dir || !dir[0]) dir = "/run/edgeos/registry";
    char path[512];
    char reason[160];
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;
    unsigned long long generation = 0;
    if (edge_registry_generation(control->db, &generation) == 0) {
        cJSON_AddNumberToObject(root, "generation", (double)generation);
    }
    if (edge_registry_snapshot_publish(control->db, dir, path, sizeof(path), reason,
                                       sizeof(reason)) == 0) {
        cJSON_AddStringToObject(root, "status", "ok");
        cJSON_AddStringToObject(root, "path", path);
    } else {
        cJSON_AddStringToObject(root, "status", "error");
        cJSON_AddStringToObject(root, "reason", reason);
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

void iems_control_port_init(rest_control_port_t *port, iems_control_ctx_t *ctx,
                            edge_app_mgr_t *mgr, stream_engine_t *engine, sqlite_svc_t *db) {
    if (!port || !ctx) return;
    ctx->mgr = mgr;
    ctx->engine = engine;
    ctx->db = db;
    ctx->app_runtime = NULL;
    memset(ctx->consumers, 0, sizeof(ctx->consumers));
    ctx->consumer_count = 0;
    _load_package_keyring(ctx);
    if (db) {
        size_t released = 0;
        char reason[160];
        if (app_points_reconcile(db, &released, reason, sizeof(reason)) == 0 && released > 0) {
            LOG_INFO(LOG_CAT_CORE, "AppMgr: released %zu stale app point rows", released);
        }
    }
    port->ctx = ctx;
    port->apps_json = _control_apps_json;
    port->time_status_json = _control_time_status_json;
    port->time_mark = _control_time_mark;
    port->start_app = _control_start_app;
    port->stop_app = _control_stop_app;
    port->uninstall_app = _control_uninstall_app;
    port->install_app = _control_install_app;
    port->install_package = _control_install_package;
    port->upgrade_package = _control_upgrade_package;
    port->stream_topology_json = _control_stream_topology_json;
    port->approve_grant = _control_approve_grant;
    port->revoke_grant = _control_revoke_grant;
    port->grants_json = _control_grants_json;
    port->upgrade_app = _control_upgrade_app;
    port->rollback_app = _control_rollback_app;
    port->secret_set = _control_secret_set;
    port->secret_delete = _control_secret_delete;
    port->secret_list = _control_secret_list;
    port->task_submit = _control_task_submit;
    port->task_status = _control_task_status;
    port->task_finish = _control_task_finish;
    port->task_purge = _control_task_purge;
    port->registry_status_json = _control_registry_status;
    port->registry_snapshot = _control_registry_snapshot;
    port->consumer_heartbeat = _control_consumer_heartbeat;
}

void iems_control_set_app_runtime(iems_control_ctx_t *ctx, iems_app_runtime_t *runtime) {
    if (ctx) ctx->app_runtime = runtime;
}

void iems_control_app_faulted(iems_control_ctx_t *ctx, const char *app_id, int64_t status,
                              int term_signal) {
    if (!ctx || !ctx->mgr || !app_id) return;
    char reason[96];
    snprintf(reason, sizeof(reason), "app-host exit status=%lld signal=%d", (long long)status,
             term_signal);
    if (edge_app_mgr_report_fault(ctx->mgr, app_id, reason) != 0) return;

    app_manifest_t manifest;
    if (ctx->app_runtime && edge_app_mgr_get_app(ctx->mgr, app_id, &manifest) == 0 &&
        manifest.fault_policy == FAULT_POLICY_AUTO_RESTART &&
        manifest.restart_count <= IEMS_APP_RESTART_CAP) {
        static const uint32_t k_delays_ms[IEMS_APP_RESTART_CAP] = {1000, 2000, 4000, 8000, 16000};
        uint32_t base = 1000;
        const char *env = getenv("EDGEC_APP_RESTART_DELAY_MS");
        if (env && env[0]) base = (uint32_t)strtoul(env, NULL, 10);
        uint32_t index = manifest.restart_count ? manifest.restart_count - 1u : 0u;
        if (index >= IEMS_APP_RESTART_CAP) index = IEMS_APP_RESTART_CAP - 1u;
        uint32_t delay = (uint32_t)((uint64_t)k_delays_ms[index] * base / 1000u);
        /* +/-20% so a host restart cannot bring every app up in the same tick */
        delay = (uint32_t)((uint64_t)delay * (80u + (uint32_t)rand() % 41u) / 100u);
        uint32_t heap_kb = manifest.heap_limit_kb > 0 ? manifest.heap_limit_kb : 64;
        if (iems_app_runtime_restart_later(ctx->app_runtime, app_id, manifest.wasm_path, delay,
                                           heap_kb, 64) == 0) {
            LOG_INFO(LOG_CAT_CORE, "AppHost: restarting %s in %u ms (attempt %d)", app_id, delay,
                     manifest.restart_count);
        } else {
            LOG_WARN(LOG_CAT_CORE, "AppHost: restart scheduling failed for %s", app_id);
        }
    }
    _publish_snapshot(ctx);
}

void iems_control_app_restarted(iems_control_ctx_t *ctx, const char *app_id) {
    if (!ctx || !ctx->mgr || !app_id) return;
    if (edge_app_mgr_start(ctx->mgr, app_id) == 0) {
        _publish_snapshot(ctx);
    } else if (ctx->app_runtime) {
        iems_app_runtime_stop(ctx->app_runtime, app_id);
    }
}
