#ifndef BUS_ORCHESTRATOR_H
#define BUS_ORCHESTRATOR_H

#include <stdint.h>

#include "arbiter.h"
#include "contract_data_types.h"
#include "datahub.h"
#include "io_executor.h"

// 前向声明驱动管理器，避免循环包含
struct driver_manager_s;
typedef struct driver_manager_s driver_manager_t;

typedef struct bus_orchestrator_s bus_orchestrator_t;

typedef struct {
    uint32_t tag_netbuf_total;
    uint32_t tag_netbuf_free;
    uint32_t tag_netbuf_peak;
    uint32_t tag_netbuf_leak;
} bus_orch_monitor_tags_t;

bus_orchestrator_t *bus_orchestrator_create(datahub_t *hub, arbiter_t *arb, driver_manager_t *mgr,
                                            int max_buses, uint32_t pool_size, uint32_t buf_size);

void bus_orchestrator_set_monitor_tags(bus_orchestrator_t *ctx,
                                       const bus_orch_monitor_tags_t *tags);

int bus_orchestrator_attach_io(bus_orchestrator_t *ctx, int bus_id, io_handle_t *io);

io_handle_t *bus_orchestrator_get_io(bus_orchestrator_t *ctx, int bus_id);

/**
 * @brief 配置总线为 Master 模式并注入契约
 */
int bus_orchestrator_setup_master(bus_orchestrator_t *ctx, int bus_id, const char *protocol,
                                  uint32_t device_id, const driver_contract_injection_t *inject);

/**
 * @brief 配置总线为 Slave 模式并注入契约
 */
int bus_orchestrator_setup_slave(bus_orchestrator_t *ctx, int bus_id, const char *protocol,
                                 uint32_t device_id, const driver_contract_injection_t *inject);

/**
 * @brief 向已有的从站总线添加额外的协议支持 (多协议混跑)
 */
int bus_orchestrator_add_slave_protocol(bus_orchestrator_t *ctx, int bus_id, const char *protocol,
                                        uint32_t device_id,
                                        const driver_contract_injection_t *inject);

/**
 * @brief 提交业务指令到仲裁器
 */
int bus_orchestrator_submit_cmd(bus_orchestrator_t *ctx, int bus_id, cmd_msg_t *cmd);

int bus_orchestrator_run(bus_orchestrator_t *ctx);

/**
 * @brief [Aetheris] 触发编排器重载配置
 */
void bus_orchestrator_trigger_reload(bus_orchestrator_t *ctx);

/**
 * @brief 获取总线管理的内存池 (用于注入 DataHub 监控)
 */
struct net_buf_pool *bus_orchestrator_get_pool(bus_orchestrator_t *ctx);

void bus_orchestrator_destroy(bus_orchestrator_t *ctx);

#endif
