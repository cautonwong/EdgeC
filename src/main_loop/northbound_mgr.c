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
#include "driver_manager.h"
#include "edge_driver.h"
#include "sparkplug_b.h"

extern int sparkplug_b_driver_register(driver_manager_t *mgr);
typedef struct {
    char group_id[64];
    char edge_node_id[64];
    uint64_t seq;
    void *hub;
    bool birth_sent;
} spb_ctx_t;

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
        if (res == CURLE_OK) {
            printf("\033[95m[NORTH] ☁️  Push to Matrix: Tag=%u Val=%.2f\033[0m\n", tag_id, val);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

static void _push_to_mqtt(northbound_mgr_t *mgr, uint32_t tag_id, double val, uint64_t ts) {
    if (!mgr || !mgr->mqtt_connected || !mgr->nb_driver) {
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

    // [Nexus] 使用 Sparkplug B 驱动进行编码
    edge_driver_ctx_t *ctx = (edge_driver_ctx_t *)mgr->nb_driver;
    net_buf_t *nb = net_buf_alloc(ctx->pool, 0);
    if (nb) {
        if (ctx->driver->encode_write(ctx, tag_id, val, nb) == 0) {
            spb_ctx_t *spb = (spb_ctx_t *)ctx->priv;
            char topic[256];
            snprintf(topic, sizeof(topic), "spBv1.0/%s/NDATA/%s", spb->group_id, spb->edge_node_id);
            
            MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
            pubmsg.payload = nb->b.data;
            pubmsg.payloadlen = (int)nb->b.len;
            pubmsg.qos = 1;
            pubmsg.retained = 0;

            MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
            MQTTAsync_sendMessage(mgr->mqtt_client, topic, &pubmsg, &opts);
            LOG_DEBUG(LOG_CAT_NB, "Sparkplug B: Published NDATA to %s", topic);
        }
        net_buf_unref(nb);
    }
}

static void on_data_notify_timer(uv_timer_t *handle) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)handle->data;
    if (!mgr || !mgr->user_data) return;

    // [Nexus] 处理驱动 Tick (用于 NBIRTH 等)
    if (mgr->mqtt_connected && mgr->nb_driver) {
        edge_driver_ctx_t *ctx = (edge_driver_ctx_t *)mgr->nb_driver;
        if (ctx->driver->on_tick) {
            net_buf_t *tick_buf = NULL;
            if (ctx->driver->on_tick(ctx, &tick_buf) == 0 && tick_buf) {
                 spb_ctx_t *spb = (spb_ctx_t *)ctx->priv;
                 char topic[256];
                 snprintf(topic, sizeof(topic), "spBv1.0/%s/NBIRTH/%s", spb->group_id, spb->edge_node_id);
                 
                 MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
                 pubmsg.payload = tick_buf->b.data;
                 pubmsg.payloadlen = (int)tick_buf->b.len;
                 pubmsg.qos = 1;
                 pubmsg.retained = 0;

                 MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
                 MQTTAsync_sendMessage(mgr->mqtt_client, topic, &pubmsg, &opts);
                 LOG_INFO(LOG_CAT_NB, "Sparkplug B: Published NBIRTH to %s", topic);
                 net_buf_unref(tick_buf);
            }
        }
    }

    PipeMsg msg;
    while (shm_ring_read(mgr->ring, &mgr->last_seq, &msg) == 0) {
        if (msg.type == PIPEMSG_TYPE_SCALAR) {
            _push_to_mqtt(mgr, msg.tag_id, msg.payload.val.f, msg.ts);
            _push_to_matrix(msg.tag_id, msg.payload.val.f, msg.ts);
        }
    }

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

static void _on_mqtt_connect_success(void *context, MQTTAsync_successData *response) {
    (void)response;
    northbound_mgr_t *mgr = (northbound_mgr_t *)context;
    mgr->mqtt_connected = true;
    LOG_INFO(LOG_CAT_NB, "Sparkplug B: Connected to MQTT Broker.");
}

static void _on_mqtt_connect_failure(void *context, MQTTAsync_failureData *response) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)context;
    mgr->mqtt_connected = false;
    LOG_ERROR(LOG_CAT_NB, "Sparkplug B: Failed to connect to MQTT Broker (Code: %d).", response ? response->code : -1);
}

static void north_thread_entry(void *arg) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)arg;
    if (!mgr) return;

    const char *msg = "\n\033[33m>>> NORTHBOUND THREAD STARTING <<<\033[0m\n";
    write(2, msg, strlen(msg));
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\033[33m[NORTH-BOOT] Thread Entry Point Reached.\033[0m\n");
    fflush(stdout);

    int retry = 0;
    while (!mgr->user_data && retry++ < 10) {
        usleep(100000);
    }

    if (!mgr->user_data) {
        fprintf(stderr, "[NORTH] FATAL: Northbound thread started without service context!\n");
        return;
    }

    if (mgr->nb_driver) {
        ((edge_driver_ctx_t *)mgr->nb_driver)->user_data = service_get_datahub((service_t *)mgr->user_data);
    }

    curl_global_init(CURL_GLOBAL_ALL);

    mgr->loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(mgr->loop);

    const char *mqtt_url = getenv("MQTT_BROKER");
    if (!mqtt_url) mqtt_url = "tcp://localhost:1883";
    
    MQTTAsync_create((MQTTAsync *)&mgr->mqtt_client, mqtt_url, mgr->product_name, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = _on_mqtt_connect_success;
    conn_opts.onFailure = _on_mqtt_connect_failure;
    conn_opts.context = mgr;
    conn_opts.automaticReconnect = 1;

    if (MQTTAsync_connect(mgr->mqtt_client, &conn_opts) != MQTTASYNC_SUCCESS) {
        LOG_ERROR(LOG_CAT_NB, "Sparkplug B: Failed to start MQTT connect process.");
    }

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

    printf("[NORTH] Thread started. Polling DataHub ring...\n");
    uv_run(mgr->loop, UV_RUN_DEFAULT);

    if (mgr->rest_srv) rest_server_destroy(mgr->rest_srv);

    if (mgr->mqtt_client) {
        MQTTAsync_disconnect(mgr->mqtt_client, NULL);
        MQTTAsync_destroy((MQTTAsync *)&mgr->mqtt_client);
        mgr->mqtt_client = NULL;
    }

    uv_loop_close(mgr->loop);
    free(mgr->loop);
    mgr->loop = NULL;

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

    // [Nexus] 初始化北向驱动
    mgr->drv_mgr = driver_manager_create();
    sparkplug_b_driver_register(mgr->drv_mgr);
    mgr->nb_driver = driver_instance_create(mgr->drv_mgr, "SparkplugB", 1);

    if (mgr->nb_driver) {
        mgr->nb_driver->pool = net_buf_pool_new(32, 1024, 0);
    }

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

void northbound_mgr_destroy(northbound_mgr_t *mgr) {
    if (!mgr) return;
    if (mgr->nb_driver) {
        if (mgr->nb_driver->pool) net_buf_pool_unref(mgr->nb_driver->pool);
        driver_instance_destroy(mgr->nb_driver, "SparkplugB");
        mgr->nb_driver = NULL;
    }
    if (mgr->drv_mgr) {
        driver_manager_destroy(mgr->drv_mgr);
        mgr->drv_mgr = NULL;
    }
    uv_mutex_destroy(&mgr->req_lock);
}