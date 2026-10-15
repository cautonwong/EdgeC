#ifndef NORTHBOUND_MGR_H
#define NORTHBOUND_MGR_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "edge_request.h"
#include "shmring.h"

// 💡 [Golang 原则] 使用者定义传输契约
typedef struct {
    void* ctx;
    int (*publish)(void* ctx, const char* topic, const uint8_t* payload, size_t len);
    int (*subscribe)(void* ctx, const char* topic,
                     void (*on_msg)(void* ctx, const char* topic, const uint8_t* p, size_t len));
    bool (*is_connected)(void* ctx);
} nb_transport_ops_t;

// 💡 [Consumer-Driven] 数据源抽象：Northbound 不再直接依赖 shm_ring_t
typedef struct {
    void* ctx;
    /** 读取下一条消息。返回 0=有数据, 1=无数据, <0=错误 */
    int (*read_next)(void* ctx, uint64_t* seq, pipe_msg_t* out);
} nb_source_ops_t;

typedef struct {
    uv_thread_t thread;
    uv_loop_t* loop;
    uv_timer_t poll_timer;
    uv_async_t stop_async;
    uv_async_t req_async;

    // 💡 [Consumer-Driven] 使用抽象数据源代替直接 shm_ring_t 依赖
    nb_source_ops_t source;
    uint64_t last_seq;

    void* user_data;
    struct sqlite_svc_s* db;
    struct snf_manager_s* snf;

    char product_name[64];
    int rest_port;
    void* rest_srv;

    // 💡 物理注入的传输接口 (不再硬连 MQTT)
    nb_transport_ops_t transport;

    struct driver_manager_s* drv_mgr;
    struct edge_driver_ctx_s* nb_driver;

    struct uv__queue req_queue;
    uv_mutex_t req_lock;
} northbound_mgr_t;

typedef struct {
    edge_req_t* req;
    struct uv__queue node;
} nb_req_entry_t;

struct datahub_s;

// 兼容性聚合根初始化：注入 datahub 聚合根安全适配
int northbound_mgr_init(northbound_mgr_t* mgr, uv_loop_t* loop, struct datahub_s* hub,
                        const char* product_name, struct sqlite_svc_s* db);

// 物理闭环初始化：注入 source 和 transport 实现
int northbound_mgr_init_ex(northbound_mgr_t* mgr, nb_source_ops_t source, const char* product_name,
                           struct sqlite_svc_s* db, nb_transport_ops_t transport);

int northbound_mgr_start(northbound_mgr_t* mgr);
void northbound_mgr_stop(northbound_mgr_t* mgr);
void northbound_mgr_destroy(northbound_mgr_t* mgr);

int northbound_mgr_submit_req(northbound_mgr_t* mgr, edge_req_t* req);
void northbound_mgr_set_rest_port(northbound_mgr_t* mgr, int port);

#endif
