#include "northbound_mgr.h"

#include <MQTTAsync.h>
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "contract_data_types.h"
#include "edge_log.h"
#include "edge_service.h"

extern char *rest_dispatch(service_t *svc, const char *method, const char *path, const char *body);

// [Matrix] 简单的 HTTP POST 实现，用于回传数据给仿真服务器
static void _push_to_matrix(uint32_t tag_id, double val, uint64_t ts) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        // 获取 Matrix Server 地址，默认为 localhost:50051
        const char *srv = getenv("OMNIS_SERVER");
        char url[128];
        snprintf(url, sizeof(url), "http://%s/api/v1/monitor", srv ? srv : "localhost:50051");

        // 构造 JSON Payload
        char body[128];
        snprintf(body, sizeof(body), "{\"tag_id\": %u, \"value\": %f, \"ts\": %lu}", tag_id, val,
                 ts);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 200L);  // 快速超时，避免阻塞

        // 执行请求
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            // 仅在调试时打印连接错误，防止刷屏
            // fprintf(stderr, "[NORTH-ERR] Matrix Push Failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("\033[95m[NORTH] ☁️  Push to Matrix: Tag=%u Val=%.2f\033[0m\n", tag_id, val);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

static void _push_to_mqtt(northbound_mgr_t *mgr, uint32_t tag_id, double val, uint64_t ts) {
    if (!mgr || !mgr->mqtt_connected) {
        // [AET-2.3] 断网补传：如果 MQTT 未连接，写入本地 SQLite
        if (mgr->db) {
            PipeMsg msg = {0};
            msg.ts = ts;
            msg.tag_id = tag_id;
            msg.payload.val.f = val;
            msg.quality = 1;
            sqlite_svc_insert(mgr->db, &msg);
        }
        return;
    }

    // MQTT 实际发送逻辑 (调用 Paho MQTT)
    // char topic[128];
    // snprintf(topic, sizeof(topic), "edge/%s/shadow/%u", mgr->product_name, tag_id);
    // MQTTAsync_sendMessage(mgr->mqtt_client, topic, ...);
}

static void on_data_notify_timer(uv_timer_t *handle) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)handle->data;
    if (!mgr || !mgr->user_data) return;

    PipeMsg msg;
    while (shm_ring_read(mgr->ring, &mgr->last_seq, &msg) == 0) {
        if (msg.type == PIPEMSG_TYPE_SCALAR) {
            // [AET-1.2] 云端影子同步
            _push_to_mqtt(mgr, msg.tag_id, msg.payload.val.f, msg.ts);

            // [Matrix] 仿真回传
            _push_to_matrix(msg.tag_id, msg.payload.val.f, msg.ts);
        }
    }

    // [Trace] 北向心跳：每秒打印一次，确认为存活

    static uint64_t last_beat = 0;

    uint64_t now = uv_now(handle->loop);

    if (now - last_beat >= 1000) {
        printf("\033[35m[NORTH] Running. Ring Seq Cursor: %lu\033[0m\n", mgr->last_seq);

        fflush(stdout);

        last_beat = now;
    }
}

static void _on_req_notify(uv_async_t *handle) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)handle->data;
    struct uv__queue *q;

    uv_mutex_lock(&mgr->req_lock);
    while (!uv__queue_empty(&mgr->req_queue)) {
        q = uv__queue_head(&mgr->req_queue);
        nb_req_entry_t *entry = uv__queue_data(q, nb_req_entry_t, node);
        uv__queue_remove(q);
        uv_mutex_unlock(&mgr->req_lock);

        LOG_INFO(LOG_CAT_NB, "Processing NB Request (Tag: %u)",
                 ((edge_nb_req_t *)entry->req->priv_data)->tag_id);
        // 调度给 IO Executor 或内部状态机
        // [Aetheris] 这里是 AET-1.1 的核心：将 NB 指令转化为内部 Request
        if (entry->req->ops && entry->req->ops->setup) {
            entry->req->ops->setup(entry->req, mgr->user_data);
        }

        free(entry->req->priv_data);
        free(entry->req);
        free(entry);

        uv_mutex_lock(&mgr->req_lock);
    }
    uv_mutex_unlock(&mgr->req_lock);
}

static void on_north_stop(uv_async_t *handle) {
    uv_stop(handle->loop);
}

static void north_thread_entry(void *arg) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)arg;
    if (!mgr) return;

    // [System-level Trace] 使用无缓冲 write 直接写 stderr
    const char *msg = "\n\033[33m>>> NORTHBOUND THREAD STARTING <<<\033[0m\n";
    write(2, msg, strlen(msg));

    // 强制 stdout 无缓冲
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\033[33m[NORTH-BOOT] Thread Entry Point Reached.\033[0m\n");
    fflush(stdout);

    // 等待 user_data (svc) 注入，最长等待 1 秒
    int retry = 0;
    while (!mgr->user_data && retry++ < 10) {
        usleep(100000);
    }

    if (!mgr->user_data) {
        fprintf(stderr, "[NORTH] FATAL: Northbound thread started without service context!\n");
        return;
    }

    // [Init] 全局初始化 CURL (线程安全)
    curl_global_init(CURL_GLOBAL_ALL);

    mgr->loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(mgr->loop);

    // 关键：在这里完成所有 Handle 初始化
    uv_timer_init(mgr->loop, &mgr->poll_timer);
    mgr->poll_timer.data = mgr;
    uv_timer_start(&mgr->poll_timer, on_data_notify_timer, 100, 100);

    uv_async_init(mgr->loop, &mgr->stop_async, on_north_stop);
    mgr->stop_async.data = mgr;

    uv_async_init(mgr->loop, &mgr->req_async, _on_req_notify);
    mgr->req_async.data = mgr;

    if (mgr->rest_port > 0 && mgr->user_data) {
        mgr->rest_srv = rest_server_create(mgr->loop, (service_t *)mgr->user_data, mgr->rest_port);
    }

    // 启动北向事件循环
    printf("[NORTH] Thread started. Polling DataHub ring...\n");
    uv_run(mgr->loop, UV_RUN_DEFAULT);

    if (mgr->rest_srv) rest_server_destroy(mgr->rest_srv);

    uv_loop_close(mgr->loop);
    free(mgr->loop);
    mgr->loop = NULL;

    // [Cleanup] 清理 CURL
    curl_global_cleanup();
}

int northbound_mgr_init(northbound_mgr_t *mgr, uv_loop_t *main_loop, shm_ring_t *ring,
                        const char *product_name, struct sqlite_svc_s *db) {
    (void)main_loop;
    memset(mgr, 0, sizeof(*mgr));
    mgr->ring = ring;
    mgr->db = db;
    strncpy(mgr->product_name, product_name, sizeof(mgr->product_name) - 1);

    uv__queue_init(&mgr->req_queue);
    uv_mutex_init(&mgr->req_lock);

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