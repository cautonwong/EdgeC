#ifndef NORTHBOUND_MGR_H
#define NORTHBOUND_MGR_H

#include <uv.h>

#include "rest_server.h"
#include "shmring.h"
#include "web_server.h"

typedef struct {
    uv_loop_t loop;
    uv_thread_t thread;
    shm_ring_t *ring;
    uint64_t last_seq;
    int running;

    // 配置项
    int rest_port;
    int web_port;
    char web_root[256];

    // [补强] 内部状态移入结构体，杜绝全局变量
    void *mqtt_client;
    bool mqtt_connected;

    uv_async_t data_async;
    uv_async_t stop_async;
    uv_timer_t poll_timer;

    web_server_t *web_srv;
    rest_server_t *rest_srv;
    void *user_data;
} northbound_mgr_t;

/**
 * @brief 初始化北向管理器实例
 */
int northbound_mgr_init(northbound_mgr_t *mgr, uv_loop_t *main_loop, shm_ring_t *ring);

/**
 * @brief 启动独立服务线程
 */
int northbound_mgr_start(northbound_mgr_t *mgr);

/**
 * @brief 优雅停止并释放
 */
void northbound_mgr_stop(northbound_mgr_t *mgr);

#endif