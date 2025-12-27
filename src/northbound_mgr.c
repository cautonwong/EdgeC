#include "northbound_mgr.h"
#include "web_server.h"
#include "rest_server.h"
#include "edge_service.h"
#include <stdlib.h>
#include <stdio.h>

static void north_thread_entry(void *arg) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)arg;
    service_t *svc = (service_t *)mgr->user_data; // We'll store svc here
    
    uv_loop_init(&mgr->loop);
    
    mgr->web_srv = web_server_create(&mgr->loop, mgr->web_port);
    mgr->rest_srv = rest_server_create(&mgr->loop, svc, mgr->rest_port);
    
    printf("[Northbound] Stack running. UI:%d, API:%d\n", mgr->web_port, mgr->rest_port);
    
    uv_run(&mgr->loop, UV_RUN_DEFAULT);
    
    web_server_destroy(mgr->web_srv);
    rest_server_destroy(mgr->rest_srv);
    uv_loop_close(&mgr->loop);
}

int northbound_mgr_init(northbound_mgr_t *mgr, uv_loop_t *main_loop, shm_ring_t *ring) {
    (void)main_loop;
    mgr->ring = ring;
    mgr->running = 0;
    return 0;
}

int northbound_mgr_start(northbound_mgr_t *mgr) {
    // Note: App must set web_port, rest_port and user_data (svc) before calling this
    if (uv_thread_create(&mgr->thread, north_thread_entry, mgr) != 0) {
        return -1;
    }
    return 0;
}

void northbound_mgr_stop(northbound_mgr_t *mgr) {
    uv_thread_join(&mgr->thread);
}