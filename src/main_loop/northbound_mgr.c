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

// --- 独立句柄结构体 (线程内部使用) ---
typedef struct {
    northbound_mgr_t *mgr;
    uv_timer_t poll_timer;
    uv_timer_t resync_timer;
    uv_async_t stop_async;
} north_worker_ctx_t;

static void on_data_notify_timer(uv_timer_t *handle) {
    north_worker_ctx_t *worker = (north_worker_ctx_t *)handle->data;
    northbound_mgr_t *mgr = worker->mgr;
    shm_ring_t *ring = mgr->ring;
    service_t *svc = (service_t *)mgr->user_data;
    if (!svc) return;

    datahub_t *hub = service_get_datahub(svc);
    if (!hub) return;

    PipeMsg msg;
    while (shm_ring_read(ring, &mgr->last_seq, &msg) == 0) {
        const edge_tag_meta_t *meta = NULL;
        uint32_t count = datahub_get_metadata_count(hub);
        for (uint32_t i = 0; i < count; i++) {
            const edge_tag_meta_t *m = datahub_get_metadata_by_index(hub, i);
            if (m && m->id == msg.tag_id) {
                meta = m;
                break;
            }
        }
        if (!meta || !meta->name || !meta->upload) continue;

        if (mgr->mqtt_connected) {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddNumberToObject(root, "v", msg.payload.val.f);
            cJSON_AddNumberToObject(root, "t", (double)msg.ts);
            char *json_str = cJSON_PrintUnformatted(root);
            char topic[128];
            snprintf(topic, sizeof(topic), "edge/%s/%s", mgr->product_name, meta->name);
            MQTTAsync_send((MQTTAsync)mgr->mqtt_client, topic, (int)strlen(json_str), json_str, 0,
                           0, NULL);
            free(json_str);
            cJSON_Delete(root);
        }
    }
}

static void on_north_stop(uv_async_t *handle) {
    north_worker_ctx_t *worker = (north_worker_ctx_t *)handle->data;
    uv_stop(worker->mgr->loop);
}

static void north_thread_entry(void *arg) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)arg;
    if (!mgr) return;

    north_worker_ctx_t *worker = calloc(1, sizeof(north_worker_ctx_t));
    worker->mgr = mgr;

    mgr->loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(mgr->loop);

    uv_timer_init(mgr->loop, &worker->poll_timer);
    worker->poll_timer.data = worker;
    uv_timer_start(&worker->poll_timer, on_data_notify_timer, 100, 100);

    uv_async_init(mgr->loop, &worker->stop_async, on_north_stop);
    worker->stop_async.data = worker;
    // 将 stop_async 存回 mgr 供外部控制
    memcpy(&mgr->stop_async, &worker->stop_async, sizeof(uv_async_t));

    if (mgr->rest_port > 0 && mgr->user_data) {
        mgr->rest_srv = rest_server_create(mgr->loop, (service_t *)mgr->user_data, mgr->rest_port);
    }

    uv_run(mgr->loop, UV_RUN_DEFAULT);

    if (mgr->rest_srv) rest_server_destroy(mgr->rest_srv);
    uv_loop_close(mgr->loop);
    free(mgr->loop);
    free(worker);
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
    return uv_thread_create(&mgr->thread, north_thread_entry, mgr);
}

void northbound_mgr_stop(northbound_mgr_t *mgr) {
    // 物理发送异步信号
    uv_async_send(&mgr->stop_async);
    uv_thread_join(&mgr->thread);
}