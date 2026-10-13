#ifndef EDGE_APP_MGR_H
#define EDGE_APP_MGR_H

#include <stdint.h>
#include <stdbool.h>
typedef struct uv_loop_s uv_loop_t;
#include "datahub.h"
#include "sqlite_service.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WASM 应用运行状态
 */
typedef enum {
    APP_STATE_UNINSTALLED = 0,
    APP_STATE_INSTALLED   = 1,
    APP_STATE_RUNNING     = 2,
    APP_STATE_STOPPED     = 3,
    APP_STATE_FAULTED     = 4,
} app_state_t;

/**
 * @brief WASM 应用故障恢复策略
 */
typedef enum {
    FAULT_POLICY_AUTO_RESTART      = 0,  /**< 默认：自动重载重启 */
    FAULT_POLICY_ISOLATE_AND_ALARM = 1,  /**< 隔离熔断并发送系统告警 */
} fault_policy_t;

/**
 * @brief WASM 应用描述符结构体 (App Manifest)
 */
typedef struct {
    char id[64];              /**< 应用唯一标识 (e.g. "voltage_guard") */
    char name[128];           /**< 应用显示名称 */
    char version[32];         /**< 语义化版本 (e.g. "1.0.0") */
    char author[64];          /**< 开发者或组织 */
    char wasm_path[256];      /**< 磁盘物理 .wasm 文件存储路径 */
    uint32_t trigger_tag;     /**< 绑定的 DataHub 触发点位 ID */
    char entry_func[64];      /**< 导出的算法入口函数名 (e.g. "on_event") */
    fault_policy_t fault_policy; /**< 故障处理策略 */
    uint32_t heap_limit_kb;   /**< 堆内存限制 (KB) */
    uint32_t timeout_ms;      /**< 单次演算看门狗超时 (ms) */
    app_state_t state;        /**< 当前运行状态 */
    uint32_t restart_count;   /**< 累计重启计数 */
    uint64_t created_at;      /**< 安装创建时间戳 */
} app_manifest_t;

typedef struct edge_app_mgr_s edge_app_mgr_t;

/**
 * @brief 创建并初始化应用管理器
 */
edge_app_mgr_t *edge_app_mgr_create(uv_loop_t *loop, datahub_t *hub, sqlite_svc_t *db, void *stream_engine);

/**
 * @brief 销毁应用管理器
 */
void edge_app_mgr_destroy(edge_app_mgr_t *mgr);

/**
 * @brief 安装/部署新应用 (自动解析或默认配置)
 */
int edge_app_mgr_install(edge_app_mgr_t *mgr, const app_manifest_t *manifest, const void *wasm_data, size_t wasm_len);

/**
 * @brief 启动应用
 */
int edge_app_mgr_start(edge_app_mgr_t *mgr, const char *app_id);

/**
 * @brief 停止应用
 */
int edge_app_mgr_stop(edge_app_mgr_t *mgr, const char *app_id);

/**
 * @brief 卸载应用
 */
int edge_app_mgr_uninstall(edge_app_mgr_t *mgr, const char *app_id);

/**
 * @brief 开机自动恢复所有持久化的 RUNNING 状态应用
 */
int edge_app_mgr_boot_restore(edge_app_mgr_t *mgr);

/**
 * @brief 获取应用状态与列表
 */
int edge_app_mgr_get_app(edge_app_mgr_t *mgr, const char *app_id, app_manifest_t *out_manifest);

/**
 * @brief 从 app.json 文本解析 Manifest 结构体
 */
int edge_app_mgr_parse_manifest_json(const char *json_str, app_manifest_t *out_manifest);

/**
 * @brief 为单 .wasm 字节码文件构造默认 Manifest 描述符
 */
int edge_app_mgr_construct_default_manifest(const char *filename_or_id, app_manifest_t *out_manifest);

#ifdef __cplusplus
}
#endif

#endif // EDGE_APP_MGR_H
