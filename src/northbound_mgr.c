#include "northbound_mgr.h"
#include "web_server.h"
#include "rest_server.h"
#include "edge_service.h"
#include <stdlib.h>
#include <stdio.h>

static void on_walk_close(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

static void on_north_stop(uv_async_t *handle) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)handle->data;
    uv_stop(&mgr->loop);
}

static void north_thread_entry(void *arg) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)arg;
    service_t *svc = (service_t *)mgr->user_data; 
    
    uv_loop_init(&mgr->loop);
    uv_async_init(&mgr->loop, &mgr->stop_async, on_north_stop);
    mgr->stop_async.data = mgr;
    
    mgr->web_srv = web_server_create(&mgr->loop, mgr->web_port);
    mgr->rest_srv = rest_server_create(&mgr->loop, svc, mgr->rest_port);
    
    printf("[Northbound] Stack running. UI:%d, API:%d\n", mgr->web_port, mgr->rest_port);
    
    uv_run(&mgr->loop, UV_RUN_DEFAULT);
    
    // Cleanup sequence
    web_server_destroy(mgr->web_srv);
    rest_server_destroy(mgr->rest_srv);
    
    // 2. 强制清理 loop 上的所有残留句柄 (包括未完成的 TCP 连接)
    uv_walk(&mgr->loop, on_walk_close, NULL);
    while (uv_loop_close(&mgr->loop) == UV_EBUSY) {
        uv_run(&mgr->loop, UV_RUN_ONCE);
    }

    // 3. 此时 loop 已关闭，所有 close_cb (包含 web_srv/rest_srv 的释放) 已处理
}

int northbound_mgr_init(northbound_mgr_t *mgr, uv_loop_t *main_loop, shm_ring_t *ring) {
    (void)main_loop;
    mgr->ring = ring;
    mgr->running = 0;
    return 0;
}

int northbound_mgr_start(northbound_mgr_t *mgr) {
    if (uv_thread_create(&mgr->thread, north_thread_entry, mgr) != 0) {
        return -1;
    }
    return 0;
}

void northbound_mgr_stop(northbound_mgr_t *mgr) {
    uv_async_send(&mgr->stop_async);
    uv_thread_join(&mgr->thread);
}