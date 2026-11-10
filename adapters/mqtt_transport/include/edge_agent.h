#ifndef EDGE_AGENT_H
#define EDGE_AGENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>
typedef struct uv_loop_s uv_loop_t;

#define EDGE_AGENT_ID_MAX  64
#define EDGE_AGENT_TOPIC_MAX 128
#define EDGE_AGENT_BROKER_MAX 256

// Edge-Ops agent state
typedef enum {
    AGENT_STATE_DISCONNECTED = 0,
    AGENT_STATE_CONNECTING,
    AGENT_STATE_CONNECTED,
    AGENT_STATE_ERROR
} edge_agent_state_t;

// Agent heartbeat payload (JSON)
typedef struct {
    char device_id[EDGE_AGENT_ID_MAX];
    char product_type[32];
    uint64_t uptime_ms;
    double metrics_watermark;
    double metrics_drops;
    uint32_t active_buses;
} edge_agent_heartbeat_t;

// Agent configuration
typedef struct {
    char broker_url[EDGE_AGENT_BROKER_MAX];
    char device_id[EDGE_AGENT_ID_MAX];
    char product_type[32];
    uint32_t heartbeat_interval_ms;  // default 30000 (30s)
    void (*on_state_change)(edge_agent_state_t state, void *user_data);
    void *user_data;
} edge_agent_config_t;

// Opaque agent handle
typedef struct edge_agent_s edge_agent_t;

// Aligned storage placeholder for caller allocation
typedef struct {
    alignas(max_align_t) uint8_t raw[4096];
} edge_agent_storage_t;

/**
 * @brief Create Edge-Ops management agent using caller-allocated storage
 * @param storage Pre-allocated storage buffer
 * @param cfg Agent configuration
 * @return Agent handle, NULL on failure
 */
edge_agent_t *edge_agent_create_in(edge_agent_storage_t *storage, edge_agent_config_t cfg);

/**
 * @brief Create Edge-Ops management agent
 * @param cfg Agent configuration
 * @return Agent handle, NULL on failure
 */
edge_agent_t *edge_agent_create(edge_agent_config_t cfg);

/**
 * @brief Start agent (connects to MQTT broker, begins heartbeat)
 */
int edge_agent_start(edge_agent_t *agent, uv_loop_t *loop);

/**
 * @brief Update heartbeat metrics (call from metrics timer)
 */
void edge_agent_update_heartbeat(edge_agent_t *agent, edge_agent_heartbeat_t *hb);

/**
 * @brief Stop agent and disconnect
 */
void edge_agent_stop(edge_agent_t *agent);

/**
 * @brief Destroy agent and free resources
 */
void edge_agent_destroy(edge_agent_t *agent);

#endif
