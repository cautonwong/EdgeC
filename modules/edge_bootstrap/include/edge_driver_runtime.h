#ifndef EDGE_DRIVER_RUNTIME_H
#define EDGE_DRIVER_RUNTIME_H

#include "driver_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 组合根持有的驱动运行时。
 *
 * manager 的存储、创建和销毁集中在这里；产品只接收注册端口和会话端口。
 */
typedef struct {
    driver_manager_storage_t manager_storage;
    driver_manager_t *manager;
    edge_driver_registry_port_t registry;
    edge_driver_session_port_t session;
} edge_driver_runtime_t;

int edge_driver_runtime_init(edge_driver_runtime_t *runtime);
void edge_driver_runtime_destroy(edge_driver_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif
