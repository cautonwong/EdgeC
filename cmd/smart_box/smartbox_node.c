/**
 * @file smartbox_node.c
 * @brief SmartBox Edge Controller Aggregate Root Implementation (No-Contract)
 */

#include "smartbox_node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"
#include "smartbox_driver_adapter.h"

struct smartbox_node_s {
    uv_loop_t *loop;
    smartbox_node_config_t cfg;
    edge_composition_container_storage_t comp_storage;
    edge_composition_container_t *comp_root;
    datahub_t *hub;

    /* 领域子聚合 (Sub-Aggregates) */
    smartbox_thermal_model_t thermal_model;
    smartbox_switchgear_bay_t bay;

    bool running;
};

smartbox_node_t *smartbox_node_create(uv_loop_t *loop, const smartbox_node_config_t *cfg) {
    if (!loop || !cfg) return NULL;

    smartbox_node_t *node = (smartbox_node_t *)calloc(1, sizeof(smartbox_node_t));
    if (!node) return NULL;

    node->loop = loop;
    node->cfg = *cfg;
    if (!node->cfg.product_name) node->cfg.product_name = "smart_box";
    if (!node->cfg.shm_name) node->cfg.shm_name = "smart_box";
    if (!node->cfg.datahub_capacity) node->cfg.datahub_capacity = 1024;
    if (node->cfg.transformer_rated_kva <= 0.0) node->cfg.transformer_rated_kva = 630.0;

    /* 1. 基础组合根装配 (Composition Root) */
    edge_composition_config_t comp_cfg = {
        .product_name = node->cfg.product_name,
        .loop = node->loop,
        .shm_name = node->cfg.shm_name,
        .ring_capacity = node->cfg.datahub_capacity * 4,
        .datahub_capacity = node->cfg.datahub_capacity,
        .map_fn_v2 = smartbox_tag_map_fn,
        .memory_pool_buffer_size = 2048,
        .memory_pool_capacity = 64,
        .enable_dma_buf = false,
        .enable_anti_corruption = true
    };
    node->comp_root = edge_composition_root_create_in(&node->comp_storage, &comp_cfg);
    if (!node->comp_root) {
        LOG_ERROR(LOG_CAT_CORE, "SmartBox Node: Failed to instantiate Composition Root");
        free(node);
        return NULL;
    }
    node->hub = edge_composition_root_get_datahub(node->comp_root);

    /* 2. 初始化核心领域子聚合 */
    smartbox_thermal_init(&node->thermal_model, node->cfg.transformer_rated_kva);
    smartbox_interlock_bay_init(&node->bay, 1);

    LOG_INFO(LOG_CAT_CORE, "SmartBox Node Aggregate Root assembled successfully (Capacity: %.1f kVA)",
             node->cfg.transformer_rated_kva);
    return node;
}

void smartbox_node_destroy(smartbox_node_t *node) {
    if (!node) return;
    if (node->running) {
        smartbox_node_stop(node);
    }
    if (node->comp_root) {
        edge_composition_root_destroy(node->comp_root);
        node->comp_root = NULL;
        node->hub = NULL;
    }
    free(node);
}

int smartbox_node_start(smartbox_node_t *node) {
    if (!node || node->running) return -1;
    node->running = true;
    LOG_INFO(LOG_CAT_CORE, "SmartBox Node aggregate root started successfully");
    return 0;
}

int smartbox_node_stop(smartbox_node_t *node) {
    if (!node || !node->running) return -1;
    node->running = false;
    LOG_INFO(LOG_CAT_CORE, "SmartBox Node aggregate root stopped");
    return 0;
}

smartbox_thermal_model_t *smartbox_node_get_thermal_model(smartbox_node_t *node) {
    return node ? &node->thermal_model : NULL;
}

smartbox_switchgear_bay_t *smartbox_node_get_switchgear_bay(smartbox_node_t *node) {
    return node ? &node->bay : NULL;
}

datahub_t *smartbox_node_get_datahub(smartbox_node_t *node) {
    return node ? node->hub : NULL;
}

edge_composition_container_t *smartbox_node_get_container(smartbox_node_t *node) {
    return node ? node->comp_root : NULL;
}

int smartbox_node_evaluate_thermal(smartbox_node_t *node, double ambient_temp_c,
                                  double top_oil_temp_c, double load_kva, double dt_s) {
    if (!node) return -1;
    int rc = smartbox_thermal_evaluate_cycle(&node->thermal_model, ambient_temp_c,
                                            top_oil_temp_c, load_kva, dt_s);
    if (rc == 0 && node->hub) {
        pipe_msg_t msg;
        pipe_msg_init_scalar(&msg, TAG_BOX_SYSTEM_STATUS,
                             node->thermal_model.is_thermal_alarm ? 2.0 : 1.0,
                             uv_hrtime());
        datahub_publish(node->hub, &msg);
    }
    return rc;
}

int smartbox_node_operate_breaker(smartbox_node_t *node, bool close_cmd, const char **out_reason) {
    if (!node) return -1;
    interlock_operation_type_t op = close_cmd ? INTERLOCK_OP_CLOSE_BREAKER : INTERLOCK_OP_OPEN_BREAKER;
    interlock_result_t res = smartbox_interlock_evaluate(&node->bay, op, out_reason);
    if (res == INTERLOCK_PERMITTED) {
        node->bay.is_breaker_closed = close_cmd;
        return 0;
    }
    return (int)res;
}

int smartbox_node_operate_earth_switch(smartbox_node_t *node, bool close_cmd, const char **out_reason) {
    if (!node) return -1;
    interlock_operation_type_t op = close_cmd ? INTERLOCK_OP_CLOSE_EARTH_SWITCH : INTERLOCK_OP_OPEN_EARTH_SWITCH;
    interlock_result_t res = smartbox_interlock_evaluate(&node->bay, op, out_reason);
    if (res == INTERLOCK_PERMITTED) {
        node->bay.is_earth_switch_closed = close_cmd;
        return 0;
    }
    return (int)res;
}
