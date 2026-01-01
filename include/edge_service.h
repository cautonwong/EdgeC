#ifndef EDGE_SERVICE_H
#define EDGE_SERVICE_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "arbiter.h"
#include "datahub.h"
#include "edge_params.h"
#include "sqlite_service.h"
#include "stream_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct service_s service_t;

typedef struct {
    const char *mode;
} service_options_t;

typedef void (*emergency_hook_fn)(void *user_data);

// [S19] 链路定义
typedef enum {
    LINK_PRIMARY = 0,  /**< 以太网/主链路 */
    LINK_SECONDARY = 1 /**< 4G/5G/备用链路 */
} net_link_t;

typedef void (*link_monitor_fn)(net_link_t new_link, void *user_data);

service_t *service_new(service_options_t *opts);
void service_delete(service_t *svc);

// --- Setters ---
void service_set_datahub(service_t *svc, datahub_t *hub);
void service_set_storage(service_t *svc, sqlite_svc_t *storage);
void service_set_stream_engine(service_t *svc, stream_engine_t *engine);
void service_set_arbiter(service_t *svc, arbiter_t *arb);
void service_set_params(service_t *svc, edge_params_t *params);
void service_set_contract_path(service_t *svc, const char *path);
void service_set_emergency_hook(service_t *svc, emergency_hook_fn hook, void *user_data);

/**
 * @brief [S19] 设置链路状态监控回调
 */
void service_set_link_monitor(service_t *svc, link_monitor_fn cb, void *user_data);

/**
 * @brief [S19] 上报链路物理状态变化 (通常由驱动层调用)
 */
void service_report_link_status(service_t *svc, net_link_t link, bool is_up);

// --- Getters ---
uv_loop_t *service_get_loop(service_t *svc);
datahub_t *service_get_datahub(service_t *svc);
sqlite_svc_t *service_get_storage(service_t *svc);
stream_engine_t *service_get_stream_engine(service_t *svc);
arbiter_t *service_get_arbiter(service_t *svc);
edge_params_t *service_get_params(service_t *svc);
const char *service_get_contract_path(service_t *svc);

int service_post_cmd(service_t *svc, cmd_msg_t *cmd);
int service_run(service_t *svc);
void service_stop(service_t *svc);

#ifdef __cplusplus
}
#endif

#endif