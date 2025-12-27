#include "northbound_mgr.h"
#include "web_server.h"
#include "rest_server.h"
#include <stdlib.h>
#include <stdio.h>

static void north_thread_entry(void *arg) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)arg;
    
    uv_loop_init(&mgr->loop);
    
    // Start Web Server (Static HMI) on 8080
    mgr->web_srv = web_server_create(&mgr->loop, 8080);
    
    // Start REST Server (API) on 8081
    mgr->rest_srv = rest_server_create(&mgr->loop, mgr->ring, 8081);
    
    printf("[Northbound] Stack running. UI:8080, API:8081\n");
    
    // Run loop
    uv_run(&mgr->loop, UV_RUN_DEFAULT);
    
    // Cleanup
    web_server_destroy(mgr->web_srv);
    rest_server_destroy(mgr->rest_srv);
    uv_loop_close(&mgr->loop);
}

int northbound_mgr_init(northbound_mgr_t *mgr, uv_loop_t *main_loop, shm_ring_t *ring) {
    (void)main_loop; 
    mgr->ring = ring;
    mgr->running = 0;
    mgr->web_srv = NULL;
    mgr->rest_srv = NULL;
    return 0;
}

int northbound_mgr_start(northbound_mgr_t *mgr) {
    if (!mgr) return -1;
    // Start the dedicated thread
    if (uv_thread_create(&mgr->thread, north_thread_entry, mgr) != 0) {
        return -1;
    }
    mgr->running = 1;
    return 0;
}

void northbound_mgr_stop(northbound_mgr_t *mgr) {
    if (!mgr || !mgr->running) return;
    // In real implementation: send async signal to loop to stop
    // Simple join for MVP
    uv_thread_join(&mgr->thread);
    mgr->running = 0;
}
