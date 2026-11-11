#include "mqtt_producer.h"

#include <MQTTAsync.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"

/** 最多同时订阅几个下行主题（一个 NCMD + 预留） */
#define MQTT_MAX_SUBS 4
/** 主题长度上限 */
#define MQTT_SUB_TOPIC_MAX 128

typedef struct {
    char topic[MQTT_SUB_TOPIC_MAX];
    void (*on_msg)(void* user, const char* topic, const uint8_t* p, size_t len);
    void* user;
} mqtt_sub_t;

typedef struct {
    MQTTAsync client;
    volatile bool connected;
    volatile uint32_t inflight; /* 已入队、尚未被 broker 确认的条数 */
    volatile uint32_t failures; /* PUBACK 失败、尚未被上层取走的条数 */
    volatile uint32_t epoch;    /* 会话代数：每次重连 +1 */

    /* 订阅表。Paho 的 cleansession=1 会让服务端在重连后丢掉订阅关系，
     * 所以必须自己记住，重连后重订一遍。 */
    mqtt_sub_t subs[MQTT_MAX_SUBS];
    uint32_t sub_count;
    pthread_mutex_t sub_lock;
} mqtt_ctx_t;

/* ---- 订阅表（写方：管理器线程；读方：Paho 回调线程） ---- */

static int _sub_match(const char* sub_topic, const char* topic) {
    size_t n = strlen(sub_topic);
    if (n > 0 && sub_topic[n - 1] == '#') {
        /* 只支持尾随 #（最常见的用法）；+ 通配留到有需要再加 */
        return strncmp(sub_topic, topic, n - 1) == 0;
    }
    return strcmp(sub_topic, topic) == 0;
}

static void _resubscribe_all(mqtt_ctx_t* ctx) {
    if (!ctx || !ctx->client) return;
    pthread_mutex_lock(&ctx->sub_lock);
    for (uint32_t i = 0; i < ctx->sub_count; i++) {
        MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
        if (MQTTAsync_subscribe(ctx->client, ctx->subs[i].topic, 1, &opts) != MQTTASYNC_SUCCESS) {
            LOG_WARN(LOG_CAT_NB, "[PRODUCER] re-subscribe failed: %s", ctx->subs[i].topic);
        }
    }
    pthread_mutex_unlock(&ctx->sub_lock);
}

/* ---- 连接生命周期 ---- */

static void _on_success(void* context, MQTTAsync_successData* response) {
    (void)response;
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)context;
    if (!ctx) return;
    ctx->connected = true;
    LOG_INFO(LOG_CAT_NB, "[PRODUCER] MQTT connected.");
    _resubscribe_all(ctx); /* 首次连接也要订：subscribe 可能在连接前就调用了 */
}

static void _on_failure(void* context, MQTTAsync_failureData* response) {
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)context;
    if (!ctx) return;
    ctx->connected = false;
    LOG_ERROR(LOG_CAT_NB, "[PRODUCER] MQTT connect failed (code %d).",
              response ? response->code : -1);
}

/**
 * 断连回调。
 *
 * 不注册这个回调，`connected` 会一直停在 true —— 上层 `is_connected()` 以为还连着，
 * 于是一条数据都不进 SnF，全部在 Paho 的队列里随进程一起消失。
 */
static void _on_connection_lost(void* context, char* cause) {
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)context;
    if (!ctx) return;
    ctx->connected = false;
    LOG_ERROR(LOG_CAT_NB, "[PRODUCER] MQTT connection lost: %s", cause ? cause : "unknown");
}

/**
 * 重连成功回调。
 *
 * Paho 文档（MQTTAsync.h:1848）：重连成功后只会调用 `MQTTAsync_setConnected`
 * 注册的回调，`conn_opts.onSuccess` **不会**再触发一次。所以"重连后能做什么"
 * 完全取决于有没有注册这里。
 */
static void _on_reconnected(void* context, char* cause) {
    (void)cause;
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)context;
    if (!ctx) return;
    ctx->connected = true;
    __atomic_add_fetch(&ctx->epoch, 1, __ATOMIC_SEQ_CST);
    LOG_INFO(LOG_CAT_NB, "[PRODUCER] MQTT session re-established (epoch %u).",
             (unsigned)__atomic_load_n(&ctx->epoch, __ATOMIC_SEQ_CST));
    _resubscribe_all(ctx);
}

/* ---- 下行 ---- */

static int _on_message(void* context, char* topic, int topic_len, MQTTAsync_message* msg) {
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)context;
    (void)topic_len;

    if (ctx && topic && msg) {
        pthread_mutex_lock(&ctx->sub_lock);
        for (uint32_t i = 0; i < ctx->sub_count; i++) {
            if (ctx->subs[i].on_msg && _sub_match(ctx->subs[i].topic, topic)) {
                ctx->subs[i].on_msg(ctx->subs[i].user, topic, (const uint8_t*)msg->payload,
                                    (size_t)msg->payloadlen);
            }
        }
        pthread_mutex_unlock(&ctx->sub_lock);
    }

    /* Paho 要求应用自己释放 topic 与 message */
    if (msg) MQTTAsync_freeMessage(&msg);
    if (topic) MQTTAsync_free(topic);
    return 1;
}

/* ---- 上行 ---- */

/* PUBACK 到达 / 发送失败：无论哪种，这条都不再"在途"。 */
static void _pub_settled(void* context, MQTTAsync_successData* response) {
    (void)response;
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)context;
    if (ctx) __atomic_sub_fetch(&ctx->inflight, 1, __ATOMIC_SEQ_CST);
}

static void _pub_failed(void* context, MQTTAsync_failureData* response) {
    (void)response;
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)context;
    if (ctx) {
        __atomic_sub_fetch(&ctx->inflight, 1, __ATOMIC_SEQ_CST);
        __atomic_add_fetch(&ctx->failures, 1, __ATOMIC_SEQ_CST);
    }
}

/**
 * 发布。
 *
 * 返回值语义是**"已交给传输"**，不是"对端收到"——QoS1 的 PUBACK 是异步的。
 * 谁需要"真送达"的判断，看 `pending()`。SnF 就是这么用的。
 */
static int _mqtt_publish(void* ctx, const char* topic, const uint8_t* payload, size_t len) {
    mqtt_ctx_t* mctx = (mqtt_ctx_t*)ctx;
    if (!mctx || !mctx->connected || !mctx->client) return -1;

    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    pubmsg.payload = (void*)payload;
    pubmsg.payloadlen = (int)len;
    pubmsg.qos = 1;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.context = mctx;
    opts.onSuccess = _pub_settled;
    opts.onFailure = _pub_failed;

    /* 先加计数再发：回调可能在 sendMessage 返回之前就被调用（本地 broker 很快），
     * 顺序反了会让计数漏加、永久停在 1，SnF 就再也不敢确认任何一批。 */
    __atomic_add_fetch(&mctx->inflight, 1, __ATOMIC_SEQ_CST);
    if (MQTTAsync_sendMessage(mctx->client, topic, &pubmsg, &opts) != MQTTASYNC_SUCCESS) {
        __atomic_sub_fetch(&mctx->inflight, 1, __ATOMIC_SEQ_CST);
        return -1;
    }
    return 0;
}

static int _mqtt_subscribe(void* ctx, const char* topic,
                           void (*on_msg)(void* user, const char* topic, const uint8_t* p,
                                          size_t len),
                           void* user) {
    mqtt_ctx_t* mctx = (mqtt_ctx_t*)ctx;
    if (!mctx || !topic || !topic[0] || !on_msg) return -1;
    if (strlen(topic) >= MQTT_SUB_TOPIC_MAX) return -1;

    pthread_mutex_lock(&mctx->sub_lock);

    /* 同一主题重复订阅时更新回调，不要占两个槽位 */
    for (uint32_t i = 0; i < mctx->sub_count; i++) {
        if (strcmp(mctx->subs[i].topic, topic) == 0) {
            mctx->subs[i].on_msg = on_msg;
            mctx->subs[i].user = user;
            pthread_mutex_unlock(&mctx->sub_lock);
            return 0;
        }
    }

    if (mctx->sub_count >= MQTT_MAX_SUBS) {
        pthread_mutex_unlock(&mctx->sub_lock);
        LOG_ERROR(LOG_CAT_NB, "[PRODUCER] too many subscriptions (max %d)", MQTT_MAX_SUBS);
        return -1;
    }

    snprintf(mctx->subs[mctx->sub_count].topic, MQTT_SUB_TOPIC_MAX, "%s", topic);
    mctx->subs[mctx->sub_count].on_msg = on_msg;
    mctx->subs[mctx->sub_count].user = user;
    mctx->sub_count++;

    bool connected = mctx->connected && mctx->client;
    pthread_mutex_unlock(&mctx->sub_lock);

    if (connected) {
        MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
        if (MQTTAsync_subscribe(mctx->client, topic, 1, &opts) != MQTTASYNC_SUCCESS) {
            LOG_WARN(LOG_CAT_NB, "[PRODUCER] subscribe failed: %s", topic);
            return -1;
        }
    }
    /* 还没连上也没关系：连上时 `_on_success` 会统一订一遍 */
    return 0;
}

static bool _mqtt_is_connected(void* ctx) {
    mqtt_ctx_t* mctx = (mqtt_ctx_t*)ctx;
    return mctx ? mctx->connected : false;
}

static uint32_t _mqtt_pending(void* ctx) {
    mqtt_ctx_t* mctx = (mqtt_ctx_t*)ctx;
    return mctx ? __atomic_load_n(&mctx->inflight, __ATOMIC_SEQ_CST) : 0;
}

static uint32_t _mqtt_take_failures(void* ctx) {
    mqtt_ctx_t* mctx = (mqtt_ctx_t*)ctx;
    return mctx ? __atomic_exchange_n(&mctx->failures, 0, __ATOMIC_SEQ_CST) : 0;
}

static uint32_t _mqtt_session_epoch(void* ctx) {
    mqtt_ctx_t* mctx = (mqtt_ctx_t*)ctx;
    return mctx ? __atomic_load_n(&mctx->epoch, __ATOMIC_SEQ_CST) : 0;
}

nb_transport_ops_t mqtt_producer_create(const char* url, const char* client_id) {
    nb_transport_ops_t ops = {0};
    mqtt_ctx_t* ctx = calloc(1, sizeof(mqtt_ctx_t));
    if (!ctx) {
        LOG_ERROR(LOG_CAT_NB, "[PRODUCER] out of memory, MQTT transport unavailable.");
        return ops;
    }
    pthread_mutex_init(&ctx->sub_lock, NULL);

    if (MQTTAsync_create(&ctx->client, url, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL) !=
        MQTTASYNC_SUCCESS) {
        LOG_ERROR(LOG_CAT_NB, "[PRODUCER] MQTTAsync_create failed for %s.", url ? url : "(null)");
        pthread_mutex_destroy(&ctx->sub_lock);
        free(ctx);
        return ops;
    }

    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = _on_success;
    conn_opts.onFailure = _on_failure;
    conn_opts.context = ctx;
    conn_opts.automaticReconnect = 1;

    /* 断连回调不在 connectOptions 里，只能经 setCallbacks 注册。
     * 不注册它，connected 会一直停在 true —— 见 _on_connection_lost 的注释。 */
    MQTTAsync_setCallbacks(ctx->client, ctx, _on_connection_lost, _on_message, NULL);

    /* 重连成功只会走这个回调（见 _on_reconnected 的注释）。 */
    MQTTAsync_setConnected(ctx->client, ctx, _on_reconnected);

    MQTTAsync_connect(ctx->client, &conn_opts);

    ops.ctx = ctx;
    ops.publish = _mqtt_publish;
    ops.subscribe = _mqtt_subscribe;
    ops.is_connected = _mqtt_is_connected;
    ops.pending = _mqtt_pending;
    ops.take_failures = _mqtt_take_failures;
    ops.session_epoch = _mqtt_session_epoch;
    return ops;
}

void mqtt_producer_destroy(nb_transport_ops_t* ops) {
    if (ops && ops->ctx) {
        mqtt_ctx_t* ctx = (mqtt_ctx_t*)ops->ctx;
        if (ctx->client) {
            MQTTAsync_disconnect(ctx->client, NULL);
            MQTTAsync_destroy(&ctx->client);
        }
        pthread_mutex_destroy(&ctx->sub_lock);
        free(ctx);
        ops->ctx = NULL;
    }
}
