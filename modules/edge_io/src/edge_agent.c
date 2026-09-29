#include "edge_agent.h"
#include "edge_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTAsync.h>

struct edge_agent_s {
    edge_agent_config_t cfg;
    edge_agent_state_t state;
    uv_loop_t *loop;
    uv_timer_t heartbeat_timer;
    MQTTAsync client;
    edge_agent_heartbeat_t hb_data;
    bool started;
};

// --- MQTT Callbacks ---

static void on_connect_success(void *context, MQTTAsync_successData *response) {
    (void)response;
    edge_agent_t *agent = (edge_agent_t *)context;
    agent->state = AGENT_STATE_CONNECTED;
    LOG_INFO(LOG_CAT_NB, "[Edge-Agent] Connected to %s", agent->cfg.broker_url);
    if (agent->cfg.on_state_change)
        agent->cfg.on_state_change(AGENT_STATE_CONNECTED, agent->cfg.user_data);
}

static void on_connect_failure(void *context, MQTTAsync_failureData *response) {
    edge_agent_t *agent = (edge_agent_t *)context;
    agent->state = AGENT_STATE_ERROR;
    LOG_ERROR(LOG_CAT_NB, "[Edge-Agent] Connection failed: %s",
              response ? MQTTAsync_strerror(response->code) : "unknown");
    if (agent->cfg.on_state_change)
        agent->cfg.on_state_change(AGENT_STATE_ERROR, agent->cfg.user_data);
}

static void on_disconnect(void *context, MQTTAsync_successData *response) {
    (void)response;
    edge_agent_t *agent = (edge_agent_t *)context;
    agent->state = AGENT_STATE_DISCONNECTED;
    LOG_INFO(LOG_CAT_NB, "[Edge-Agent] Disconnected");
}

static void on_publish_success(void *context, MQTTAsync_successData *response) {
    (void)context;
    (void)response;
}

// --- Heartbeat ---

static void send_heartbeat(edge_agent_t *agent) {
    if (agent->state != AGENT_STATE_CONNECTED) return;

    // Build JSON heartbeat payload
    char payload[512];
    int n = snprintf(payload, sizeof(payload),
        "{"
        "\"device_id\":\"%s\","
        "\"product\":\"%s\","
        "\"uptime_ms\":%lu,"
        "\"ring_watermark\":%.2f,"
        "\"ring_drops\":%.0f,"
        "\"active_buses\":%u"
        "}",
        agent->cfg.device_id,
        agent->cfg.product_type,
        (unsigned long)agent->hb_data.uptime_ms,
        agent->hb_data.metrics_watermark,
        agent->hb_data.metrics_drops,
        agent->hb_data.active_buses);

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.context = agent;
    opts.onSuccess = on_publish_success;

    char topic[EDGE_AGENT_TOPIC_MAX];
    snprintf(topic, sizeof(topic), "edgeops/v1/agents/%s/heartbeat", agent->cfg.device_id);

    int rc = MQTTAsync_send(agent->client, topic, (int)strlen(payload),
                            payload, 1, 0, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
        LOG_WARN(LOG_CAT_NB, "[Edge-Agent] Heartbeat publish failed: %d", rc);
    }
}

static void on_heartbeat_timer(uv_timer_t *handle) {
    edge_agent_t *agent = (edge_agent_t *)handle->data;
    if (!agent) return;
    agent->hb_data.uptime_ms += agent->cfg.heartbeat_interval_ms;
    send_heartbeat(agent);
}

_Static_assert(sizeof(struct edge_agent_s) <= sizeof(edge_agent_storage_t),
               "edge_agent_storage_t is too small for struct edge_agent_s");

static edge_agent_storage_t g_default_agent_storage;

edge_agent_t *edge_agent_create_in(edge_agent_storage_t *storage, edge_agent_config_t cfg) {
    if (!storage) return NULL;
    memset(storage, 0, sizeof(*storage));
    edge_agent_t *agent = (edge_agent_t *)storage;
    agent->cfg = cfg;
    agent->state = AGENT_STATE_DISCONNECTED;
    agent->started = false;
    return agent;
}

edge_agent_t *edge_agent_create(edge_agent_config_t cfg) {
    return edge_agent_create_in(&g_default_agent_storage, cfg);
}

int edge_agent_start(edge_agent_t *agent, uv_loop_t *loop) {
    if (!agent || !loop) return -1;
    if (agent->started) return 0;

    agent->loop = loop;

    // Create MQTT client
    int rc = MQTTAsync_create(&agent->client, agent->cfg.broker_url,
                              agent->cfg.device_id,
                              MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        LOG_ERROR(LOG_CAT_NB, "[Edge-Agent] Failed to create MQTT client: %d", rc);
        return -1;
    }

    MQTTAsync_setCallbacks(agent->client, agent, NULL, NULL, NULL);

    // Connect
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.onSuccess = on_connect_success;
    conn_opts.onFailure = on_connect_failure;
    conn_opts.context = agent;
    conn_opts.cleansession = 1;
    conn_opts.keepAliveInterval = 20;

    rc = MQTTAsync_connect(agent->client, &conn_opts);
    if (rc != MQTTASYNC_SUCCESS) {
        LOG_ERROR(LOG_CAT_NB, "[Edge-Agent] MQTT connect failed: %d", rc);
        MQTTAsync_destroy(&agent->client);
        return -1;
    }

    agent->state = AGENT_STATE_CONNECTING;
    agent->started = true;

    // Start heartbeat timer
    uv_timer_init(loop, &agent->heartbeat_timer);
    agent->heartbeat_timer.data = agent;
    uint64_t interval = agent->cfg.heartbeat_interval_ms > 0
                        ? agent->cfg.heartbeat_interval_ms : 30000;
    uv_timer_start(&agent->heartbeat_timer, on_heartbeat_timer, interval, interval);

    LOG_INFO(LOG_CAT_NB, "[Edge-Agent] Started (broker=%s, id=%s, heartbeat=%ums)",
             agent->cfg.broker_url, agent->cfg.device_id, (unsigned)interval);
    return 0;
}

void edge_agent_update_heartbeat(edge_agent_t *agent, edge_agent_heartbeat_t *hb) {
    if (!agent || !hb) return;
    agent->hb_data = *hb;
}

void edge_agent_stop(edge_agent_t *agent) {
    if (!agent || !agent->started) return;

    uv_timer_stop(&agent->heartbeat_timer);

    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    disc_opts.context = agent;
    disc_opts.onSuccess = on_disconnect;
    MQTTAsync_disconnect(agent->client, &disc_opts);

    agent->state = AGENT_STATE_DISCONNECTED;
    agent->started = false;
    LOG_INFO(LOG_CAT_NB, "[Edge-Agent] Stopped");
}

void edge_agent_destroy(edge_agent_t *agent) {
    if (!agent) return;

    if (agent->started) edge_agent_stop(agent);

    if (agent->client) {
        MQTTAsync_destroy(&agent->client);
        agent->client = NULL;
    }
}
