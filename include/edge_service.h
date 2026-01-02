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
/**
 * @brief [S9] 添加 IPC 桥接，将内部总线转发至外部 UDS 地址
 */
void service_add_ipc_bridge(service_t *svc, const char *uds_path);

/**
 * @brief [S11] 触发总线扫描发现设备
 */
int service_scan_bus(service_t *svc, int bus_id);

/**
 * @brief [S11] 获取最近一次扫描的发现报告 (JSON 格式)
 */
char *service_get_discovery_report(service_t *svc);

/**
 * @brief [S16] 冷启动自举：根据清单文件自动生成并加载契约
 */
int service_bootstrap(service_t *svc, const char *manifest_path);

/**
 * @brief [S10] 同步系统时间 (ms)
 */
void service_sync_time(service_t *svc, uint64_t calendar_ms);

/**
 * @brief [S10] 获取校准后的当前时间戳 (ms)
 */
uint64_t service_get_now_ms(service_t *svc);

/**
 * @brief [S12] 创建远程维护隧道 (TCP 端口映射到物理总线)
 */
void service_create_tunnel(service_t *svc, int tcp_port, int bus_id);

/**
 * @brief [S4] 触发历史数据导出补发 (断网续传逻辑)
 * @param table 目标表名
 * @param cb 发送回调 (通常封装为 MQTT Publish)
 */
typedef void (*export_publish_fn)(uint32_t tag, double val);
int service_trigger_export(service_t *svc, const char *table, export_publish_fn cb);

int service_run(service_t *svc);
void service_stop(service_t *svc);

#ifdef __cplusplus
}
#endif

#endif