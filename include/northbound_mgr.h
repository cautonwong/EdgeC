#ifndef NORTHBOUND_MGR_H
#define NORTHBOUND_MGR_H

#include <stdint.h>
#include <stdbool.h>
#include <uv.h>
#include "shmring.h"
#include "edge_request.h"

// 💡 [Golang 原则] 使用者定义传输契约
typedef struct {
    void *ctx;
    int (*publish)(void *ctx, const char *topic, const uint8_t *payload, size_t len);
    int (*subscribe)(void *ctx, const char *topic, void (*on_msg)(void *ctx, const char *topic, const uint8_t *p, size_t len));
    bool (*is_connected)(void *ctx);
} nb_transport_ops_t;

typedef struct {
    uv_thread_t thread;
    uv_loop_t *loop;
    uv_timer_t poll_timer;
    uv_async_t stop_async;
    uv_async_t req_async;
    
    shm_ring_t *ring;
    uint64_t last_seq;
    
    void *user_data;
    struct sqlite_svc_s *db;
    struct snf_manager_s *snf;
    
    char product_name[64];
    int rest_port;
    void *rest_srv;

    // 💡 物理注入的传输接口 (不再硬连 MQTT)
    nb_transport_ops_t transport;
    
    struct driver_manager_s *drv_mgr;
    struct edge_driver_ctx_s *nb_driver;

    struct uv__queue req_queue;
    uv_mutex_t req_lock;
} northbound_mgr_t;

typedef struct {
    edge_req_t *req;
    struct uv__queue node;
} nb_req_entry_t;

// 物理闭环初始化：注入 transport 实现
int northbound_mgr_init_ex(northbound_mgr_t *mgr, shm_ring_t *ring, 
                          const char *product_name, struct sqlite_svc_s *db,
                          nb_transport_ops_t transport);

int northbound_mgr_start(northbound_mgr_t *mgr);
void northbound_mgr_stop(northbound_mgr_t *mgr);
void northbound_mgr_destroy(northbound_mgr_t *mgr);

int northbound_mgr_submit_req(northbound_mgr_t *mgr, edge_req_t *req);
void northbound_mgr_set_rest_port(northbound_mgr_t *mgr, int port);

#endif
