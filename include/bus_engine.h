#ifndef EDGE_BUS_ENGINE_H
#define EDGE_BUS_ENGINE_H

typedef struct uv_loop_s uv_loop_t;

#include "contract_data_types.h"
#include "edge_slave_plane.h"
#include "net_buf.h"

// --- Forward Declarations (Decoupling Planes) ---
typedef struct bus_engine_s bus_engine_t;
typedef struct arbiter arbiter_t;
typedef struct cmd_msg cmd_msg_t;
typedef struct io_handle_s io_handle_t;
typedef struct driver_manager_s driver_manager_t;
typedef struct datahub_s datahub_t;

struct bus_engine_s {
    // 绑定物理层。Engine 在这里注册 io_callback。
    void (*attach)(bus_engine_t* self, io_handle_t* io);

    // 收到数据。Engine 必须负责 unref 这个 rx
    void (*on_data)(bus_engine_t* self, net_buf_t* rx);

    // 执行指令
    void (*run_command)(bus_engine_t* self, cmd_msg_t* cmd);

    // [新增] 注入协议契约
    void (*inject)(bus_engine_t* self, const void* proto_contract);

    // [ADR-002 S3-0] 注入从站面端口（取当前值 / 收写入值）。
    // master 引擎为 NULL；从站引擎把它转交给 session_manager。
    int (*set_slave_source)(bus_engine_t* self, const edge_slave_source_ops_t* ops, void* ctx);

    // 销毁
    void (*detach)(bus_engine_t* self);

    // Context Data
    void* user_data;
};

// Factory functions
// [ADR-002] 不再注入点位表：驱动是纯 codec，点位翻译在产品侧完成
bus_engine_t* engine_create_master(driver_manager_t* mgr, int bus_id, const char* protocol,
                                   uint32_t device_id, uv_loop_t* loop, arbiter_t* arb,
                                   datahub_t* hub, net_buf_pool_t* pool);

bus_engine_t* engine_create_slave(driver_manager_t* mgr, int bus_id, const char* protocol,
                                  uint32_t device_id, uv_loop_t* loop, arbiter_t* arb,
                                  datahub_t* hub, net_buf_pool_t* pool);

/**
 * @brief 向从站引擎添加额外的协议支持 (Phase 4: 多协议混跑)
 */
int engine_slave_add_protocol(bus_engine_t* self, const char* protocol, uint32_t device_id);

#endif
