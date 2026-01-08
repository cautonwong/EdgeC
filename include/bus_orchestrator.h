#ifndef BUS_ORCHESTRATOR_H
#define BUS_ORCHESTRATOR_H

#include <stdint.h>

#include "arbiter.h"
#include "datahub.h"

// 前向声明驱动管理器，避免循环包含
struct driver_manager_s;
typedef struct driver_manager_s driver_manager_t;

#include "io_executor.h"

typedef struct bus_orchestrator_s bus_orchestrator_t;

bus_orchestrator_t *bus_orchestrator_create(datahub_t *hub, arbiter_t *arb, driver_manager_t *mgr,
                                            int max_buses, uint32_t pool_size, uint32_t buf_size);

int bus_orchestrator_attach_io(bus_orchestrator_t *ctx, int bus_id, io_handle_t *io);

/**
 * @brief 配置总线为 Master 模式
 */
int bus_orchestrator_setup_master(bus_orchestrator_t *ctx, int bus_id, const char *protocol,
                                  uint32_t device_id);

int bus_orchestrator_setup_slave(bus_orchestrator_t *ctx, int bus_id, const char *protocol,
                                 uint32_t device_id);

/**
 * @brief 提交业务指令到仲裁器
 */
int bus_orchestrator_submit_cmd(bus_orchestrator_t *ctx, int bus_id, cmd_msg_t *cmd);

int bus_orchestrator_run(bus_orchestrator_t *ctx);

void bus_orchestrator_destroy(bus_orchestrator_t *ctx);

#endif