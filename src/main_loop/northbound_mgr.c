#include "northbound_mgr.h"

#include <MQTTAsync.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contract_data_types.h"
#include "edge_log.h"
#include "edge_service.h"

// --- 外部契约接口声明 ---
extern const edge_tag_meta_t *edge_get_meta(edge_tag_id_t id);
extern char *rest_dispatch(service_t *svc, const char *path, const char *body);

// --- MQTT 内部回调 (通过 context 传递实例指针) ---

static void on_connect(void *context, MQTTAsync_successData *response) {
    (void)response;
    northbound_mgr_t *mgr = (northbound_mgr_t *)context;
    LOG_INFO(LOG_CAT_CORE, "MQTT Instance %p Connected.", mgr);
    mgr->mqtt_connected = true;
}

static void on_connect_failure(void *context, MQTTAsync_failureData *response) {
    (void)response;
    northbound_mgr_t *mgr = (northbound_mgr_t *)context;
    mgr->mqtt_connected = false;
}

static void mqtt_instance_init(northbound_mgr_t *mgr, const char *url, const char *client_id) {
    MQTTAsync client;
    MQTTAsync_create(&client, url, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    mgr->mqtt_client = client;

    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = on_connect;
    conn_opts.onFailure = on_connect_failure;
    conn_opts.context = mgr;  // 关键：注入实例指针
    MQTTAsync_connect(client, &conn_opts);
}

// --- DataHub Listener ---
static void on_data_notify_impl(northbound_mgr_t *mgr) {
    shm_ring_t *ring = mgr->ring;
    PipeMsg msg;
    while (shm_ring_read(ring, &mgr->last_seq, &msg) == 0) {
        const edge_tag_meta_t *meta = edge_get_meta(msg.tag_id);
        if (!meta || !meta->name || !meta->upload) continue;

        if (mgr->mqtt_connected) {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "n", meta->name);
            cJSON_AddNumberToObject(root, "v", msg.payload.val.f);
            cJSON_AddNumberToObject(root, "t", (double)msg.ts);

            char *json_str = cJSON_PrintUnformatted(root);
            char topic[128];
            snprintf(topic, sizeof(topic), "telemetry/%s", meta->name);

            MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
            MQTTAsync_send((MQTTAsync)mgr->mqtt_client, topic, (int)strlen(json_str), json_str, 0,
                           0, &opts);

            free(json_str);
            cJSON_Delete(root);
        }
    }
}

static void on_data_notify_timer(uv_timer_t *handle) {
    on_data_notify_impl((northbound_mgr_t *)handle->data);
}

static void on_north_stop(uv_async_t *handle) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)handle->data;
    uv_stop(&mgr->loop);
}

static void north_thread_entry(void *arg) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)arg;
    service_t *svc = (service_t *)mgr->user_data;
    uv_loop_init(&mgr->loop);

    mqtt_instance_init(mgr, "tcp://localhost:1883", "edgeos_gw");

    uv_timer_init(&mgr->loop, &mgr->poll_timer);
    mgr->poll_timer.data = mgr;
    uv_timer_start(&mgr->poll_timer, on_data_notify_timer, 100, 100);

    uv_async_init(&mgr->loop, &mgr->stop_async, on_north_stop);
    mgr->stop_async.data = mgr;

    mgr->web_srv = web_server_create(&mgr->loop, mgr->web_port, mgr->web_root);
    web_server_set_api_handler(mgr->web_srv, (web_api_handler_pt)rest_dispatch, svc);
    mgr->rest_srv = rest_server_create(&mgr->loop, svc, mgr->rest_port);

    uv_run(&mgr->loop, UV_RUN_DEFAULT);

    if (mgr->mqtt_client) {
        MQTTAsync_disconnect((MQTTAsync)mgr->mqtt_client, NULL);
        MQTTAsync_destroy((MQTTAsync *)&mgr->mqtt_client);
    }
    web_server_destroy(mgr->web_srv);
    rest_server_destroy(mgr->rest_srv);
    uv_loop_close(&mgr->loop);
}

int northbound_mgr_init(northbound_mgr_t *mgr, uv_loop_t *main_loop, shm_ring_t *ring) {
    (void)main_loop;
    memset(mgr, 0, sizeof(*mgr));
    mgr->ring = ring;
    return 0;
}

int northbound_mgr_start(northbound_mgr_t *mgr) {
    return uv_thread_create(&mgr->thread, north_thread_entry, mgr);
}

void northbound_mgr_stop(northbound_mgr_t *mgr) {
    uv_async_send(&mgr->stop_async);
    uv_thread_join(&mgr->thread);
}