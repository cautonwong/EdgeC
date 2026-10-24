/**
 * @file iems_app.h
 * @brief IEMS Application Assembler and Lifecycle Coordinator (AEP-406)
 *
 * Implements Composition Root Exclusivity & Assembler Pattern:
 * 1. Encapsulates full assembly sequence (Node, Orchestrator, StreamEngine, Northbound, Timers)
 * 2. Owns iems_node_t aggregate root and exposes safe domain operations
 * 3. Enforces domain boundary for control write operations (AEP-400)
 */

#ifndef IEMS_APP_H
#define IEMS_APP_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "iems_node.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iems_app_s iems_app_t;

typedef struct {
    const char *db_path;
    const char *shm_name;
    const char *wal_path;
    const char *snap_path;
    const char *mqtt_url;
    int rest_port;
} iems_app_config_t;

/**
 * @brief 装配 IEMS 应用程序各个组件 (聚合根、总线编排器、流计算引擎、北向服务)
 *
 * @param loop libuv 事件循环
 * @param cfg 应用程序配置
 * @return 装配好的 iems_app_t 句柄，失败返回 NULL
 */
iems_app_t *iems_app_assemble(uv_loop_t *loop, const iems_app_config_t *cfg);

/**
 * @brief 启动 IEMS 应用程序 (自律业务循环、总线连接定时器、任务加载等)
 *
 * @param app 应用程序句柄
 * @return 0 成功, <0 失败
 */
int iems_app_start(iems_app_t *app);

/**
 * @brief 运行 IEMS 应用程序主服务循环
 *
 * @param app 应用程序句柄
 * @return 0 成功
 */
int iems_app_run(iems_app_t *app);

/**
 * @brief 停止并销毁 IEMS 应用程序，释放所有持有资源
 *
 * @param app 应用程序句柄
 */
void iems_app_destroy(iems_app_t *app);

/**
 * @brief 获取 IEMS 聚合根
 */
iems_node_t *iems_app_get_node(iems_app_t *app);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_APP_H
