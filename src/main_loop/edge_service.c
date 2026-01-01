/**
 * @file edge_service.c
 * @brief 核心控制面实现 (支持 S14 掉电保护与 S19 链路冗余)
 */

#include "edge_service.h"

#include <stdlib.h>
#include <string.h>

#include "edge_log.h"

struct service_s {
    datahub_t *hub;
    sqlite_svc_t *db;
    stream_engine_t *engine;
    arbiter_t *arb;
    edge_params_t *params;

    char *contract_path;
    char *mode;

    emergency_hook_fn emergency_hook;
    void *emergency_user_data;

    // S19: 链路管理
    net_link_t current_link;
    bool link_status[2];  // PRIMARY, SECONDARY
    link_monitor_fn link_cb;
    void *link_user_data;
};

service_t *service_new(service_options_t *opts) {
    service_t *svc = calloc(1, sizeof(service_t));
    if (opts && opts->mode) svc->mode = strdup(opts->mode);

    // 默认主链路可用
    svc->link_status[LINK_PRIMARY] = true;
    svc->link_status[LINK_SECONDARY] = false;
    svc->current_link = LINK_PRIMARY;

    return svc;
}

void service_delete(service_t *svc) {
    if (!svc) return;
    if (svc->mode) free(svc->mode);
    if (svc->contract_path) free(svc->contract_path);
    free(svc);
}

// --- Setters ---
void service_set_datahub(service_t *svc, datahub_t *hub) {
    svc->hub = hub;
}
void service_set_storage(service_t *svc, sqlite_svc_t *storage) {
    svc->db = storage;
}
void service_set_stream_engine(service_t *svc, stream_engine_t *engine) {
    svc->engine = engine;
}
void service_set_arbiter(service_t *svc, arbiter_t *arb) {
    svc->arb = arb;
}
void service_set_params(service_t *svc, edge_params_t *params) {
    svc->params = params;
}
void service_set_contract_path(service_t *svc, const char *path) {
    if (svc->contract_path) free(svc->contract_path);
    svc->contract_path = strdup(path);
}
void service_set_emergency_hook(service_t *svc, emergency_hook_fn hook, void *user_data) {
    svc->emergency_hook = hook;
    svc->emergency_user_data = user_data;
}

void service_set_link_monitor(service_t *svc, link_monitor_fn cb, void *user_data) {
    svc->link_cb = cb;
    svc->link_user_data = user_data;
}

void service_report_link_status(service_t *svc, net_link_t link, bool is_up) {
    if (!svc || link > LINK_SECONDARY) return;

    bool changed = (svc->link_status[link] != is_up);
    svc->link_status[link] = is_up;

    net_link_t target_link = svc->current_link;

    if (!svc->link_status[LINK_PRIMARY]) {
        target_link = LINK_SECONDARY;
    } else {
        target_link = LINK_PRIMARY;
    }

    if (target_link != svc->current_link || changed) {
        svc->current_link = target_link;
        LOG_INFO(LOG_CAT_SCHED, "Link State Changed: Current Active Link is %s",
                 (svc->current_link == LINK_PRIMARY) ? "PRIMARY" : "SECONDARY");
        if (svc->link_cb) {
            svc->link_cb(svc->current_link, svc->link_user_data);
        }
    }
}

// --- Getters ---
uv_loop_t *service_get_loop(service_t *svc) {
    return svc->hub ? svc->hub->loop : NULL;
}
datahub_t *service_get_datahub(service_t *svc) {
    return svc->hub;
}
sqlite_svc_t *service_get_storage(service_t *svc) {
    return svc->db;
}
stream_engine_t *service_get_stream_engine(service_t *svc) {
    return svc->engine;
}
arbiter_t *service_get_arbiter(service_t *svc) {
    return svc->arb;
}
edge_params_t *service_get_params(service_t *svc) {
    return svc->params;
}
const char *service_get_contract_path(service_t *svc) {
    return svc->contract_path ? svc->contract_path
                              : "/workspaces/edgeos/EdgeC/products/iems/generated/contracts.yaml";
}

int service_post_cmd(service_t *svc, cmd_msg_t *cmd) {
    if (!svc || !svc->arb) return -1;
    return arbiter_submit_cmd(svc->arb, 0, cmd);
}

int service_run(service_t *svc) {
    if (!svc || !svc->hub) return -1;
    LOG_INFO(LOG_CAT_SCHED, "EdgeC Service Starting...");
    return uv_run(svc->hub->loop, UV_RUN_DEFAULT);
}

void service_stop(service_t *svc) {
    if (!svc) return;
    LOG_INFO(LOG_CAT_SCHED, "Service stopping... Triggering emergency hooks (S14).");
    if (svc->emergency_hook) svc->emergency_hook(svc->emergency_user_data);
    if (svc->hub && svc->hub->loop) uv_stop(svc->hub->loop);
}