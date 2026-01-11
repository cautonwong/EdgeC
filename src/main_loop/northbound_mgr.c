#include "northbound_mgr.h"

#include <MQTTAsync.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contract_data_types.h"
#include "edge_log.h"
#include "edge_service.h"

extern char *rest_dispatch(service_t *svc, const char *path, const char *body);

static void on_data_notify_timer(uv_timer_t *handle) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)handle->data;
    if (!mgr || !mgr->user_data) return;

    service_t *svc = (service_t *)mgr->user_data;
    datahub_t *hub = service_get_datahub(svc);
    if (!hub) return;

    PipeMsg msg;
    while (shm_ring_read(mgr->ring, &mgr->last_seq, &msg) == 0) {
        // Observability metrics collection
    }
}

static void on_north_stop(uv_async_t *handle) {
    uv_stop(handle->loop);
}

static void north_thread_entry(void *arg) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)arg;
    if (!mgr) return;

    // 等待 user_data (svc) 注入，最长等待 1 秒
    int retry = 0;
    while (!mgr->user_data && retry++ < 10) {
        usleep(100000);
    }

    if (!mgr->user_data) {
        fprintf(stderr, "[NORTH] FATAL: Northbound thread started without service context!\n");
        return;
    }

    mgr->loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(mgr->loop);

    // 关键：在这里完成所有 Handle 初始化
    uv_timer_init(mgr->loop, &mgr->poll_timer);
    mgr->poll_timer.data = mgr;
    uv_timer_start(&mgr->poll_timer, on_data_notify_timer, 100, 100);

    uv_async_init(mgr->loop, &mgr->stop_async, on_north_stop);
    mgr->stop_async.data = mgr;

    if (mgr->rest_port > 0 && mgr->user_data) {
        mgr->rest_srv = rest_server_create(mgr->loop, (service_t *)mgr->user_data, mgr->rest_port);
    }

    uv_run(mgr->loop, UV_RUN_DEFAULT);

    if (mgr->rest_srv) rest_server_destroy(mgr->rest_srv);

    uv_loop_close(mgr->loop);
    free(mgr->loop);
    mgr->loop = NULL;
}

int northbound_mgr_init(northbound_mgr_t *mgr, uv_loop_t *main_loop, shm_ring_t *ring,
                        const char *product_name, struct sqlite_svc_s *db) {
    (void)main_loop;
    memset(mgr, 0, sizeof(*mgr));
    mgr->ring = ring;
    mgr->db = db;
    strncpy(mgr->product_name, product_name, sizeof(mgr->product_name) - 1);
    return 0;
}

int northbound_mgr_start(northbound_mgr_t *mgr) {
    if (!mgr || mgr->loop) return -1;
    return uv_thread_create(&mgr->thread, north_thread_entry, mgr);
}

void northbound_mgr_stop(northbound_mgr_t *mgr) {
    if (mgr && mgr->loop) {
        uv_async_send(&mgr->stop_async);
        uv_thread_join(&mgr->thread);
    }
}