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
 * @return 成功加载的任务数量，未就绪返回 0，错误返回 -1
 */
int iems_poll_tasks_load(sqlite_svc_t *db_svc, iems_task_registered_cb on_task, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_TASK_LOADER_H */
