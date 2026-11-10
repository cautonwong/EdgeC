/**
 * @file iems_app_manager_adapter.c
 * @brief Composition-side wiring for the IEMS app manager and its sqlite repo.
 */

#include "iems_app_manager_adapter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "app_repo_sqlite.h"
#include "edge_log.h"
#include "edgeos/edge_app_mgr.h"
#include "sqlite_service.h"
#include "stream_engine.h"

static void iems_app_log(void *ctx, int level, const char *message) {
    (void)ctx;
    edge_log_out(NULL, LOG_CAT_CORE, level, "%s", message);
}

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

static int _control_start_app(void *ctx, const char *app_id) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->mgr) return -1;
    return edge_app_mgr_start(control->mgr, app_id);
}

static int _control_stop_app(void *ctx, const char *app_id) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->mgr) return -1;
    return edge_app_mgr_stop(control->mgr, app_id);
}

static int _control_install_app(void *ctx, const char *manifest_json, const char *name,
                                uint32_t trigger_tag, const char *entry_func,
                                const void *wasm, size_t wasm_len) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->mgr) return -1;

    app_manifest_t manifest;
    if (manifest_json && manifest_json[0]) {
        if (edge_app_mgr_parse_manifest_json(manifest_json, &manifest) != 0) return -1;
    } else {
        if (!name) return -1;
        edge_app_mgr_construct_default_manifest(name, &manifest);
        if (trigger_tag != 0) manifest.trigger_tag = trigger_tag;
        if (entry_func && entry_func[0]) {
            snprintf(manifest.entry_func, sizeof(manifest.entry_func), "%s", entry_func);
        }
    }

    if (edge_app_mgr_install(control->mgr, &manifest, wasm, wasm_len) != 0) return -1;
    edge_app_mgr_start(control->mgr, manifest.id);

    if (control->engine) {
        pipeline_stage_t *stage = stage_wasm_create(manifest.wasm_path, manifest.entry_func);
        if (stage) {
            stream_engine_set_pipeline_trigger(control->engine, manifest.id, manifest.trigger_tag);
            stream_engine_replace_pipeline_stage(control->engine, manifest.id, stage);
            free(stage);
        }
    }
    return 0;
}

static char *_control_stream_topology_json(void *ctx) {
    iems_control_ctx_t *control = (iems_control_ctx_t *)ctx;
    if (!control || !control->engine) return NULL;
    return stream_engine_dump_json(control->engine);
}

void iems_control_port_init(rest_control_port_t *port, iems_control_ctx_t *ctx,
                            edge_app_mgr_t *mgr, stream_engine_t *engine) {
    if (!port || !ctx) return;
    ctx->mgr = mgr;
    ctx->engine = engine;
    port->ctx = ctx;
    port->apps_json = _control_apps_json;
    port->start_app = _control_start_app;
    port->stop_app = _control_stop_app;
    port->install_app = _control_install_app;
    port->stream_topology_json = _control_stream_topology_json;
}
