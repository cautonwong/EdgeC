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
 * @brief 启动时连一次南向物理总线(组合根调用,只连一次、不重试)
 *
 * 谁发起:组合根。哪个总线没连上,启动日志会明确报出来,不再无限重连。
 *
 * @param app 应用程序句柄
 * @param configured 出参:契约表里配置的南向总线数(可为 NULL)。用来区分「没配南向总线」和「配了但全连不上」
 * @return 连上的南向总线数(含此前已连上的);<0 参数非法或装配契约表缺失
 *
 * 必须在 iems_app_start() 之前调用一次,iems_app_start() 会拒绝未连过总线的半装配启动。
 * 语义:配了南向总线就要求至少连上一路;部分连上照常启动,缺的那几路只 WARN 不致命
 *       (缺 CAN 的机器不该起不来)。全都没连上才算启动失败,由组合根 abort。
 *       能拿到装配契约表时打一行摘要:[IEMS-INIT] southbound: <connected>/<configured> connected
 *       (契约表本身取不到属于装配坏了,由组合根打 FATAL,这里不出摘要)
 */
int iems_app_connect_buses(iems_app_t *app, int *configured);

/**
 * @brief 启动 IEMS 应用程序 (自律业务循环、内部总线、任务加载等;南向总线由组合根先 connect_buses)
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
