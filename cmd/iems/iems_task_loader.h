#ifndef IEMS_TASK_LOADER_H
#define IEMS_TASK_LOADER_H

#include <stdbool.h>
#include <stdint.h>

#include "sqlite_service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int bus_id;
    uint32_t slave_id;
    uint32_t tag_id;
    uint32_t dense_idx;
    char protocol[16];
    uint32_t interval_ms;
} iems_loaded_task_t;

typedef void (*iems_task_registered_cb)(const iems_loaded_task_t *task, void *user_data);

/**
 * @brief 从统一 SQLite 服务加载测点轮询任务
 * @param db_svc 统一 SQLite 服务实例
 * @param on_task 任务注册回调
 * @param user_data 回调透传参数
 * @return 成功加载的任务数量；库里有表但无数据返回 0，查询/错误返回 -1
 */
int iems_poll_tasks_load(sqlite_svc_t *db_svc, iems_task_registered_cb on_task, void *user_data);

/**
 * @brief 按 tag 反查轮询任务的路由（bus + 规范协议名）。
 *
 * 北向控制写用它决定“这个点在哪条总线上”，取代写死 bus 0/ModbusRTU。
 * bus 10 是北向镜像，不参与。
 *
 * @return 0 找到；-1 没有路由或参数/查询失败
 */
int iems_poll_task_route(sqlite_svc_t *db_svc, uint32_t tag_id, int *bus_id, char proto[16]);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_TASK_LOADER_H */
