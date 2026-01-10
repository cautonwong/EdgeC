#ifndef NORTHBOUND_MGR_H
#define NORTHBOUND_MGR_H

#include <uv.h>

#include "rest_server.h"
#include "shmring.h"
#include "web_server.h"

typedef struct {
    uv_loop_t *loop; /**< [Phase 8] 指向私有事件循环 */
    uv_thread_t thread;
    shm_ring_t *ring;
    struct sqlite_svc_s *db; /**< [Phase 8] 数据库句柄，用于断线缓存 */
    uint64_t last_seq;
    int running;

    char product_name[32]; /**< [Phase 8] 产品名称，用于动态 Topic 拼装 */

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
    uv_timer_t resync_timer; /**< [Phase 8] 续传监控定时器 */

    web_server_t *web_srv;
    rest_server_t *rest_srv;
    void *user_data;
} northbound_mgr_t;

/**
 * @brief 初始化北向管理器实例
 */
int northbound_mgr_init(northbound_mgr_t *mgr, uv_loop_t *main_loop, shm_ring_t *ring,
                        const char *product_name, struct sqlite_svc_s *db);

/**
 * @brief 启动独立服务线程
 */
int northbound_mgr_start(northbound_mgr_t *mgr);

/**
 * @brief 优雅停止并释放
 */
void northbound_mgr_stop(northbound_mgr_t *mgr);

#endif