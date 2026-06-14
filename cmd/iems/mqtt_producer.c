#include "mqtt_producer.h"

#include <MQTTAsync.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"

typedef struct {
    MQTTAsync client;
    bool connected;
} mqtt_ctx_t;

static void _on_success(void* context, MQTTAsync_successData* response) {
    (void)response;
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)context;
    ctx->connected = true;
    LOG_INFO(LOG_CAT_NB, "[PRODUCER] MQTT Connected successfully.");
}

static void _on_failure(void* context, MQTTAsync_failureData* response) {
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)context;
    ctx->connected = false;
    LOG_ERROR(LOG_CAT_NB, "[PRODUCER] MQTT Connection failed (Code: %d).",
              response ? response->code : -1);
}

static int _mqtt_publish(void* ctx, const char* topic, const uint8_t* payload, size_t len) {
    mqtt_ctx_t* mctx = (mqtt_ctx_t*)ctx;
    if (!mctx || !mctx->connected) return -1;

    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    pubmsg.payload = (void*)payload;
    pubmsg.payloadlen = (int)len;
    pubmsg.qos = 1;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    return (MQTTAsync_sendMessage(mctx->client, topic, &pubmsg, &opts) == MQTTASYNC_SUCCESS) ? 0
                                                                                             : -1;
}

static bool _mqtt_is_connected(void* ctx) {
    mqtt_ctx_t* mctx = (mqtt_ctx_t*)ctx;
    return mctx ? mctx->connected : false;
}

nb_transport_ops_t mqtt_producer_create(const char* url, const char* client_id) {
    nb_transport_ops_t ops = {0};
    mqtt_ctx_t* ctx = calloc(1, sizeof(mqtt_ctx_t));

    MQTTAsync_create(&ctx->client, url, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);

    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = _on_success;
    conn_opts.onFailure = _on_failure;
    conn_opts.context = ctx;
    conn_opts.automaticReconnect = 1;

    MQTTAsync_connect(ctx->client, &conn_opts);

    ops.ctx = ctx;
    ops.publish = _mqtt_publish;
    ops.is_connected = _mqtt_is_connected;
    return ops;
}

void mqtt_producer_destroy(nb_transport_ops_t* ops) {
    if (ops && ops->ctx) {
        mqtt_ctx_t* ctx = (mqtt_ctx_t*)ops->ctx;
        if (ctx->client) {
            MQTTAsync_disconnect(ctx->client, NULL);
            MQTTAsync_destroy(&ctx->client);
        }
        free(ctx);
        ops->ctx = NULL;
    }
}
