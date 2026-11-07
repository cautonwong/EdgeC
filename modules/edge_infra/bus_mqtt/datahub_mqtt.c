#include "datahub_mqtt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "edge_log.h"
#include "MQTTAsync.h"
extern const datahub_ops_t g_datahub_mqtt_ops;

typedef struct {
    MQTTAsync client;
    char server_uri[128];
    char client_id[64];
    bool is_connected;
} mqtt_backend_ctx_t;

static int mqtt_init_impl(datahub_t *hub, const datahub_config_t *config) {
    if (!hub) return -1;
    (void)config;

    mqtt_backend_ctx_t *ctx = calloc(1, sizeof(mqtt_backend_ctx_t));
    if (!ctx) return -1;

    snprintf(ctx->server_uri, sizeof(ctx->server_uri), "tcp://127.0.0.1:1883");
    snprintf(ctx->client_id, sizeof(ctx->client_id), "edgeos_datahub_mqtt_%d", (int)getpid());

    int rc = MQTTAsync_create(&ctx->client, ctx->server_uri, ctx->client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        LOG_WARN(CORE, "Paho MQTT DataHub client creation deferred (rc=%d)", rc);
        ctx->is_connected = false;
    } else {
        ctx->is_connected = true;
        LOG_INFO(CORE, "Paho MQTT DataHub initialized for server %s", ctx->server_uri);
    }

    datahub_backend_bind(hub, DATAHUB_BACKEND_HYBRID, &g_datahub_mqtt_ops, ctx);
    return 0;
}

static int mqtt_publish_impl(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;
    mqtt_backend_ctx_t *ctx = (mqtt_backend_ctx_t *)datahub_backend_ctx(hub);
    if (ctx && ctx->is_connected) {
        char topic[128];
        snprintf(topic, sizeof(topic), "edgeos/datahub/tags/%u", msg->tag_id);

        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
        pubmsg.payload = (void *)msg;
        pubmsg.payloadlen = (int)sizeof(pipe_msg_t);
        pubmsg.qos = 1;
        pubmsg.retained = 0;

        MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
        MQTTAsync_sendMessage(ctx->client, topic, &pubmsg, &opts);

        datahub_backend_count_publish(hub);
        datahub_backend_count_tx(hub, 1);
        return 0;
    }
    datahub_backend_count_publish(hub);
    return 0;
}

static int mqtt_write_shadow_impl(datahub_t *hub, const pipe_msg_t *msg) {
    return mqtt_publish_impl(hub, msg);
}

static int mqtt_read_shadow_impl(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out_msg) {
    if (!hub || !out_msg) return -1;
    datahub_backend_count_shadow_read(hub);
    (void)tag_id;
    return 0;
}

static int mqtt_subscribe_impl(datahub_t *hub, datahub_on_data_cb cb, void *user_data) {
    if (!hub) return -1;
    datahub_backend_set_legacy(hub, cb, user_data);
    return 0;
}

static int mqtt_query_snapshot_impl(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out_msg) {
    return mqtt_read_shadow_impl(hub, tag_id, out_msg);
}

static int mqtt_get_stats_impl(datahub_t *hub, datahub_backend_stats_t *stats) {
    if (!hub || !stats) return -1;
    datahub_backend_get_stats(hub, stats);
    return 0;
}

static void mqtt_destroy_impl(datahub_t *hub) {
    if (!hub) return;
    mqtt_backend_ctx_t *ctx = (mqtt_backend_ctx_t *)datahub_backend_ctx(hub);
    if (ctx) {
        if (ctx->is_connected && ctx->client) {
            MQTTAsync_destroy(&ctx->client);
        }
        free(ctx);
        datahub_backend_unbind(hub);
    }
}

const datahub_ops_t g_datahub_mqtt_ops = {
    .init = mqtt_init_impl,
    .publish = mqtt_publish_impl,
    .write_shadow = mqtt_write_shadow_impl,
    .read_shadow = mqtt_read_shadow_impl,
    .subscribe = mqtt_subscribe_impl,
    .query_snapshot = mqtt_query_snapshot_impl,
    .get_stats = mqtt_get_stats_impl,
    .destroy = mqtt_destroy_impl
};

int datahub_mqtt_init(datahub_t *hub, const datahub_config_t *config) {
    if (!hub) return -1;
    return mqtt_init_impl(hub, config);
}
