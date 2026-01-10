#include "northbound_mgr.h"

#include <MQTTAsync.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contract_data_types.h"
#include "edge_log.h"
#include "edge_service.h"

// --- 外部接口声明 ---
extern char *rest_dispatch(service_t *svc, const char *path, const char *body);

// --- MQTT 内部回调 ---

static int on_message_arrived(void *context, char *topicName, int topicLen,
                              MQTTAsync_message *message) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)context;
    service_t *svc = (service_t *)mgr->user_data;
    (void)topicLen;

    LOG_INFO(LOG_CAT_CORE, "MQTT: Message arrived on topic %s", topicName);

    // [Task 5] 指令注入：解析 edge/{product}/cmd
    char cmd_topic[64];
    snprintf(cmd_topic, sizeof(cmd_topic), "edge/%s/cmd", mgr->product_name);

    if (strcmp(topicName, cmd_topic) == 0) {
        cJSON *root = cJSON_Parse(message->payload);
        if (root) {
            uint32_t tag_id = (uint32_t)cJSON_GetObjectItem(root, "tag_id")->valueint;
            double val = cJSON_GetObjectItem(root, "value")->valuedouble;

            arbiter_t *arb = service_get_arbiter(svc);
            if (arb) {
                cmd_msg_t cmd = {0};
                cmd_msg_init_write(&cmd, tag_id, val, ARBITER_PRIO_HIGH);
                strncpy(cmd.req_id, "MQTT_CMD", sizeof(cmd.req_id) - 1);
                // 默认路由到总线 0 (实际应根据配置映射)
                arbiter_submit_cmd(arb, 0, &cmd);
                LOG_WARN(LOG_CAT_CORE, "MQTT: Injected command for Tag %u val %f", tag_id, val);
            }
            cJSON_Delete(root);
        }
    }

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

static void on_connect(void *context, MQTTAsync_successData *response) {
    (void)response;
    northbound_mgr_t *mgr = (northbound_mgr_t *)context;
    LOG_INFO(LOG_CAT_CORE, "MQTT Connected. Subscribing to commands...");

    char cmd_topic[64];
    snprintf(cmd_topic, sizeof(cmd_topic), "edge/%s/cmd", mgr->product_name);
    MQTTAsync_subscribe((MQTTAsync)mgr->mqtt_client, cmd_topic, 1, NULL);

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

    MQTTAsync_setCallbacks(client, mgr, NULL, on_message_arrived, NULL);

    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = on_connect;
    conn_opts.onFailure = on_connect_failure;
    conn_opts.context = mgr;
    MQTTAsync_connect(client, &conn_opts);
}

// --- DataHub Listener ---
static void on_data_notify_impl(northbound_mgr_t *mgr) {
    shm_ring_t *ring = mgr->ring;
    service_t *svc = (service_t *)mgr->user_data;
    datahub_t *hub = service_get_datahub(svc);
    if (!hub) return;

    PipeMsg msg;
    while (shm_ring_read(ring, &mgr->last_seq, &msg) == 0) {
        // [Task 4] 动态主题映射：通过注入的元数据获取名称
        const edge_tag_meta_t *meta = NULL;
        uint32_t count = datahub_get_metadata_count(hub);
        // 此处为了性能，通常应使用 hash 查找，暂且遍历
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
            // 契约驱动主题：edge/{product}/{tag}
            snprintf(topic, sizeof(topic), "edge/%s/%s", mgr->product_name, meta->name);

            MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
            MQTTAsync_send((MQTTAsync)mgr->mqtt_client, topic, (int)strlen(json_str), json_str, 0,
                           0, &opts);

            free(json_str);
            cJSON_Delete(root);
        } else if (mgr->db) {
            // [Task 6] 断线缓存：写入本地 SQLite
            char sql[256];
            snprintf(sql, sizeof(sql),
                     "INSERT INTO offline_cache (tag_id, value, ts) VALUES (%u, %f, %lu)",
                     msg.tag_id, msg.payload.val.f, msg.ts);
            sqlite_svc_exec(mgr->db, sql, NULL, NULL);
        }
    }
}

static void on_data_notify_timer(uv_timer_t *handle) {
    on_data_notify_impl((northbound_mgr_t *)handle->data);
}

static void on_resync_timer(uv_timer_t *handle) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)handle->data;
    if (!mgr->mqtt_connected || !mgr->db) return;

    // [Task 6] 断线续传：从数据库读取一批数据并上报 (简化实现)
    // 实际应使用 sqlite_svc_query 处理回调
    // 此处仅作为架构占位
}

static void on_north_stop(uv_async_t *handle) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)handle->data;
    uv_stop(&mgr->loop);
}

static void north_thread_entry(void *arg) {
    northbound_mgr_t *mgr = (northbound_mgr_t *)arg;
    service_t *svc = (service_t *)mgr->user_data;
    uv_loop_init(&mgr->loop);

    char client_id[64];
    snprintf(client_id, sizeof(client_id), "edgeos_%s", mgr->product_name);
    mqtt_instance_init(mgr, "tcp://localhost:1883", client_id);

    uv_timer_init(&mgr->loop, &mgr->poll_timer);
    mgr->poll_timer.data = mgr;
    uv_timer_start(&mgr->poll_timer, on_data_notify_timer, 100, 100);

    // [Task 6] 初始化续传定时器
    uv_timer_t resync_timer;
    uv_timer_init(&mgr->loop, &resync_timer);
    resync_timer.data = mgr;
    uv_timer_start(&resync_timer, on_resync_timer, 5000, 5000);

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
    uv_async_send(&mgr->stop_async);
    uv_thread_join(&mgr->thread);
}
