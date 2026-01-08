#ifndef EDGE_BUS_ENGINE_H
#define EDGE_BUS_ENGINE_H

#include <uv.h>

#include "arbiter.h"
#include "datahub.h"  // 引入 DataHub 定义
#include "io_executor.h"

// Forward declaration
typedef struct bus_engine_s bus_engine_t;
typedef struct driver_manager_s driver_manager_t;

struct bus_engine_s {
    // 绑定物理层。Engine 在这里注册 io_callback。
    void (*attach)(bus_engine_t *self, io_handle_t *io);

    // 收到数据。Engine 必须负责 unref 这个 rx
    void (*on_data)(bus_engine_t *self, net_buf_t *rx);

    // 执行指令
    void (*run_command)(bus_engine_t *self, cmd_msg_t *cmd);

    // 销毁
    void (*detach)(bus_engine_t *self);

    // Context Data
    void *user_data;
};

// Factory functions
// [Updated] 注入必要的依赖：Loop, Arbiter, DataHub, Pool
bus_engine_t *engine_create_master(driver_manager_t *mgr, const char *protocol, uint32_t device_id,
                                   uv_loop_t *loop, arbiter_t *arb, datahub_t *hub,
                                   net_buf_pool_t *pool);

bus_engine_t *engine_create_slave(driver_manager_t *mgr, const char *protocol, uint32_t device_id,
                                  uv_loop_t *loop, arbiter_t *arb, datahub_t *hub,
                                  net_buf_pool_t *pool);

#endif
