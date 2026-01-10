#ifndef NORTHBOUND_MGR_H
#define NORTHBOUND_MGR_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "rest_server.h"
#include "shmring.h"
#include "web_server.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sqlite_svc_s;

typedef struct {
    uv_loop_t *loop;    /**< [Phase 8] 指向私有事件循环 */
    uv_thread_t thread; /**< 后台线程句柄 */

    // 线程内 Handle
    uv_async_t stop_async;
    uv_timer_t poll_timer;

    shm_ring_t *ring;
    uint64_t last_seq;
    char product_name[64];
    bool mqtt_connected;
    void *mqtt_client;
    struct sqlite_svc_s *db;

    void *user_data; /**< 注入 service_t 引用 */

    int rest_port;
    int web_port;
    char web_root[256];

    web_server_t *web_srv;
    rest_server_t *rest_srv;
} northbound_mgr_t;

int northbound_mgr_init(northbound_mgr_t *mgr, uv_loop_t *main_loop, shm_ring_t *ring,
                        const char *product_name, struct sqlite_svc_s *db);

int northbound_mgr_start(northbound_mgr_t *mgr);
void northbound_mgr_stop(northbound_mgr_t *mgr);

#ifdef __cplusplus
}
#endif

#endif