#ifndef BUS_ORCHESTRATOR_H
#define BUS_ORCHESTRATOR_H

#include <stdint.h>

#include "arbiter.h"
#include "datahub.h"

// 前向声明驱动管理器，避免循环包含
struct driver_manager_s;
typedef struct driver_manager_s driver_manager_t;

typedef struct bus_orchestrator_s bus_orchestrator_t;

/**
 * @brief 创建总线编排器实例
 * @param mgr 注入的驱动管理器对象 (解耦全局变量)
 */
bus_orchestrator_t *bus_orchestrator_create(datahub_t *hub, arbiter_t *arb, driver_manager_t *mgr,
                                            int max_buses, uint32_t pool_size, uint32_t buf_size);

void bus_orchestrator_destroy(bus_orchestrator_t *ctx);

/**
 * @brief 绑定物理描述符到虚拟总线
 */
int bus_orchestrator_attach_fd(bus_orchestrator_t *ctx, int bus_id, int fd);

/**
 * @brief 添加轮询任务
 */
int bus_orchestrator_add_poll(bus_orchestrator_t *ctx, const char *protocol, uint32_t device_id,
                              uint32_t tag_id, int interval_ms);

/**
 * @brief 运行编排器 (通常在主 Loop 或独立线程)
 */
int bus_orchestrator_run(bus_orchestrator_t *ctx);

#endif