#ifndef EDGE_DATAHUB_ROUTER_H
#define EDGE_DATAHUB_ROUTER_H

#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DATAHUB_ROUTER_POLICY_BEST_EFFORT = 0, /**< 默认尽力而为：至少一个后端成功即放行 */
    DATAHUB_ROUTER_POLICY_ALL_REQUIRED = 1 /**< 强一致性策略：要求所有活跃后端全部写入成功，否则返回错误码 */
} datahub_router_policy_t;

typedef struct {
    bool enable_shm;
    bool enable_dds;
    bool enable_mqtt;
    datahub_router_policy_t write_policy;
    const datahub_ops_t *dds_ops;   /**< 可选注入 DDS Ops 虚表 */
    const datahub_ops_t *mqtt_ops;  /**< 可选注入 MQTT Ops 虚表 */
    datahub_config_t shm_config;
    datahub_config_t dds_config;
    datahub_config_t mqtt_config;
} datahub_router_config_t;

typedef struct datahub_router_s datahub_router_t;

struct datahub_router_s {
    datahub_t shm_hub;
    datahub_t dds_hub;
    datahub_t mqtt_hub;
    bool shm_active;
    bool dds_active;
    bool mqtt_active;
    datahub_router_policy_t write_policy;
};

/**
 * @brief Initialize Three-Backend Concurrent DataHub Router
 */
int datahub_router_init(datahub_router_t *router, uv_loop_t *loop, const datahub_router_config_t *config,
                       const edge_tag_meta_t *metadata, uint32_t metadata_len,
                       struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                       datahub_wakeup_filter_cb wakeup_fn);

/**
 * @brief Concurrent Write Fan-Out across active backends (SHM + DDS + MQTT)
 */
int datahub_router_publish(datahub_router_t *router, const pipe_msg_t *msg);

/**
 * @brief Concurrent Write Shadow Fan-Out across active backends
 */
int datahub_router_write_shadow(datahub_router_t *router, const pipe_msg_t *msg);

/**
 * @brief Read Shadow from primary low-latency backend (SHM)
 */
int datahub_router_read_shadow(datahub_router_t *router, uint32_t tag_id, pipe_msg_t *out_msg);

/**
 * @brief Destroy DataHub Router
 */
void datahub_router_destroy(datahub_router_t *router);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_DATAHUB_ROUTER_H */
