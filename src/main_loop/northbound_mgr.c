#include "northbound_mgr.h"

#include <MQTTAsync.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contract_data_types.h"
#include "edge_log.h"
#include "edge_service.h"
#include "rest_server.h"
#include "web_server.h"

// --- 外部契约接口声明 (符合解耦架构 v2.0) ---

extern const tag_meta_t *edge_get_meta(edge_tag_id_t id);
extern char *rest_dispatch(service_t *svc, const char *path, const char *body);

// #include "rest_server.h"  <-- This was already here but maybe out of order or commented?
// I see it was after the extern. Let's make it clean.

// --- MQTT Context ---
static MQTTAsync g_mqtt_client = NULL;
static bool g_mqtt_connected = false;

static void on_connect(void *context, MQTTAsync_successData *response) {
    (void)context;
    (void)response;
    LOG_INFO(LOG_CAT_CORE, "MQTT Connected to broker");
    g_mqtt_connected = true;
}

static void on_connect_failure(void *context, MQTTAsync_failureData *response) {
    (void)context;
    (void)response;
    printf("[MQTT] Connect failed, rc %d\n", response ? response->code : 0);
    g_mqtt_connected = false;
}

static void mqtt_init_and_connect(const char *url, const char *client_id) {
    MQTTAsync_create(&g_mqtt_client, url, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = on_connect;
    conn_opts.onFailure = on_connect_failure;
    conn_opts.context = g_mqtt_client;
    MQTTAsync_connect(g_mqtt_client, &conn_opts);
}

// --- DataHub Listener ---
static void on_data_notify(uv_async_t *handle) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)handle->data;
    shm_ring_t *ring = mgr->ring;
    uint64_t seq = 0;  // TODO: 应该维护上次读取的 seq，避免重复
    // 简化：这里演示原理，实际应该在 mgr 中维护 read_seq

    // 我们需要一个非阻塞的读取循环。
    // 由于 shm_ring 是为轮询设计的，这里我们假设 mgr->last_seq 已维护
    PipeMsg msg;
    while (shm_ring_read(ring, &mgr->last_seq, &msg) == 0) {
        // 1. 查契约
        const tag_meta_t *meta = edge_get_meta(msg.tag_id);
        if (!meta) continue;

        // 2. 检查上送标志
        if (!meta->upload) continue;

        // 3. 构造 JSON 并上送
        if (g_mqtt_connected) {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "n", meta->name);
            cJSON_AddNumberToObject(root, "v", msg.payload.val.f);
            cJSON_AddNumberToObject(root, "t", (double)msg.ts);

            char *json_str = cJSON_PrintUnformatted(root);

            char topic[128];
            snprintf(topic, sizeof(topic), "telemetry/%s", meta->name);

            MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
            MQTTAsync_send(g_mqtt_client, topic, (int)strlen(json_str), json_str, 0, 0, &opts);

            free(json_str);  // cJSON_Print 分配的内存
            cJSON_Delete(root);
        }
    }
}

// --- Lifecycle ---

static void on_walk_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

static void on_north_stop(uv_async_t *handle) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)handle->data;
    uv_stop(&mgr->loop);
}

static void datahub_cb(datahub_t *hub, void *user_data) {
    (void)hub;
    uv_async_t *async = (uv_async_t *)user_data;
    uv_async_send(async);
}

static void north_thread_entry(void *arg) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)arg;
    service_t *svc = (service_t *)mgr->user_data;

    uv_loop_init(&mgr->loop);

    // 初始化 MQTT (异步库，自带线程池，不占用 Loop)
    mqtt_init_and_connect("tcp://localhost:1883", "edgeos_gw");

    // 初始化 DataHub 监听
    uv_async_init(&mgr->loop, &mgr->data_async, on_data_notify);
    mgr->data_async.data = mgr;

    // 注册到 DataHub (注意：DataHub 需要支持多 Listener，或者我们通过 service 层转发)
    // 目前 DataHub 简单实现可能只支持单 Listener。
    // 正确做法：DataHub 应该是一个广播总线。
    // 这里的实现假设 DataHub 支持回调链，或者我们通过某种方式挂载。
    // 由于 shm_ring 是多读者的，我们只需要在这里注册 async 句柄即可。
    // 实际上，datahub.c 中的 datahub_init 只接受一个 on_data。
    // 这意味着我们需要一个 EventBus 或者多播机制。
    // *临时方案*：为了演示，我们假设 Northbound 是唯一的 DataHub 监听者，或者我们修改 DataHub
    // 支持多播。 实际上，Stream Engine 也是监听者。所以 DataHub 必须支持多播。 现在的 DataHub 实现
    // (src/core/datahub.c) 确实只存了一个 user_data。
    // TODO: 升级 DataHub 支持多 Listener。

    // 暂时：我们不注册回调，而是通过定时器轮询 SHM (这也是一种常见的解耦方式)
    // 或者，我们在 service 层做一个分发。

    uv_timer_init(&mgr->loop, &mgr->poll_timer);
    mgr->poll_timer.data = mgr;
    // 100ms 轮询一次 SHM，作为临时方案
    uv_timer_start(&mgr->poll_timer, (uv_timer_cb)on_data_notify, 100, 100);

    uv_async_init(&mgr->loop, &mgr->stop_async, on_north_stop);
    mgr->stop_async.data = mgr;

    mgr->web_srv = web_server_create(&mgr->loop, mgr->web_port, mgr->web_root);
    web_server_set_api_handler(mgr->web_srv, (web_api_handler_pt)rest_dispatch, svc);

    mgr->rest_srv = rest_server_create(&mgr->loop, svc, mgr->rest_port);

    printf("[Northbound] Stack running. UI:%d, API:%d, MQTT:Active\n", mgr->web_port,
           mgr->rest_port);

    uv_run(&mgr->loop, UV_RUN_DEFAULT);

    if (g_mqtt_client) {
        MQTTAsync_disconnect(g_mqtt_client, NULL);
        MQTTAsync_destroy(&g_mqtt_client);
    }

    web_server_destroy(mgr->web_srv);
    rest_server_destroy(mgr->rest_srv);

    uv_walk(&mgr->loop, on_walk_close, NULL);
    while (uv_run(&mgr->loop, UV_RUN_ONCE))
        ;

    uv_loop_close(&mgr->loop);
}

int northbound_mgr_init(northbound_mgr_t *mgr, uv_loop_t *main_loop, shm_ring_t *ring) {
    (void)main_loop;
    mgr->ring = ring;
    mgr->running = 0;
    mgr->last_seq = 0;  // 必须初始化 seq
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