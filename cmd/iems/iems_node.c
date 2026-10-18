/**
 * @file iems_node.c
 * @brief IEMS Commercial Energy Storage Aggregate Root Implementation (No-Contract)
 */

#include "iems_node.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"
#include "iems_driver_adapter.h"

struct iems_node_s {
    uv_loop_t *loop;
    iems_node_config_t cfg;
    edge_composition_container_storage_t comp_storage;
    edge_composition_container_t *comp_root;
    datahub_t *hub;

    /* 领域子聚合 (Sub-Aggregates) */
    iems_bms_cluster_engine_t bms_cluster;
    iems_pcs_controller_t pcs_ctrl;
    iems_microgrid_coordinator_t coordinator;
    iems_fire_safety_interlock_t fire_safety;
    iems_multi_pcs_dispatcher_t pcs_dispatcher;

    bool running;
};

iems_node_t *iems_node_create(uv_loop_t *loop, const iems_node_config_t *cfg) {
    if (!loop || !cfg) return NULL;

    iems_node_t *node = (iems_node_t *)calloc(1, sizeof(iems_node_t));
    if (!node) return NULL;

    node->loop = loop;
    node->cfg = *cfg;
    if (!node->cfg.product_name) node->cfg.product_name = "IEMS";
    if (!node->cfg.shm_name) node->cfg.shm_name = "IEMS";
    if (!node->cfg.datahub_capacity) node->cfg.datahub_capacity = 1024;
    if (node->cfg.max_pcs_power_kw <= 0.0) node->cfg.max_pcs_power_kw = 500.0;
    if (node->cfg.transformer_capacity_kva <= 0.0) node->cfg.transformer_capacity_kva = 630.0;
    if (node->cfg.demand_limit_kw <= 0.0) node->cfg.demand_limit_kw = 500.0;

    /* 1. 基础组合根装配 (Composition Root) */
    edge_composition_config_t comp_cfg = {
        .product_name = node->cfg.product_name,
        .loop = node->loop,
        .shm_name = node->cfg.shm_name,
        .ring_capacity = node->cfg.datahub_capacity * 4,
        .datahub_capacity = node->cfg.datahub_capacity,
        .map_fn_v2 = iems_tag_map_fn,
        .memory_pool_buffer_size = 2048,
        .memory_pool_capacity = 64,
        .enable_dma_buf = false,
        .enable_anti_corruption = true
    };
    node->comp_root = edge_composition_root_create_in(&node->comp_storage, &comp_cfg);
    if (!node->comp_root) {
        LOG_ERROR(LOG_CAT_CORE, "IEMS Node: Failed to instantiate Composition Root");
        free(node);
        return NULL;
    }
    node->hub = edge_composition_root_get_datahub(node->comp_root);

    /* 2. 初始化核心领域子聚合 */
    iems_bms_cluster_init(&node->bms_cluster);
    iems_pcs_init(&node->pcs_ctrl, node->cfg.max_pcs_power_kw);
    iems_coordinator_init(&node->coordinator, node->cfg.transformer_capacity_kva,
                          node->cfg.demand_limit_kw);
    iems_fire_interlock_init(&node->fire_safety);
    iems_pcs_dispatcher_init(&node->pcs_dispatcher, 1000.0);

    /* 默认注册 2 台 250kW PCS */
    iems_pcs_dispatcher_register_unit(&node->pcs_dispatcher, 1, "PCS_01",
                                      node->cfg.max_pcs_power_kw * 0.5);
    iems_pcs_dispatcher_register_unit(&node->pcs_dispatcher, 2, "PCS_02",
                                      node->cfg.max_pcs_power_kw * 0.5);

    LOG_INFO(LOG_CAT_CORE, "IEMS Node Aggregate Root assembled successfully (Max Power: %.1f kW)",
             node->cfg.max_pcs_power_kw);
    return node;
}

void iems_node_destroy(iems_node_t *node) {
    if (!node) return;
    if (node->running) {
        iems_node_stop(node);
    }
    if (node->comp_root) {
        edge_composition_root_destroy(node->comp_root);
        node->comp_root = NULL;
        node->hub = NULL;
    }
    free(node);
}

int iems_node_start(iems_node_t *node) {
    if (!node || node->running) return -1;
    node->running = true;
    LOG_INFO(LOG_CAT_CORE, "IEMS Node aggregate root started successfully");
    return 0;
}

int iems_node_stop(iems_node_t *node) {
    if (!node || !node->running) return -1;
    node->running = false;
    iems_pcs_set_mode(&node->pcs_ctrl, PCS_MODE_STANDBY);
    iems_pcs_exec_pq_control(&node->pcs_ctrl, 0.0, 0.0);
    LOG_INFO(LOG_CAT_CORE, "IEMS Node aggregate root stopped");
    return 0;
}

int iems_node_step(iems_node_t *node) {
    if (!node || !node->running) return -1;
    return 0;
}

iems_bms_cluster_engine_t *iems_node_get_bms(iems_node_t *node) {
    return node ? &node->bms_cluster : NULL;
}

iems_pcs_controller_t *iems_node_get_pcs(iems_node_t *node) {
    return node ? &node->pcs_ctrl : NULL;
}

iems_microgrid_coordinator_t *iems_node_get_coordinator(iems_node_t *node) {
    return node ? &node->coordinator : NULL;
}

iems_fire_safety_interlock_t *iems_node_get_fire_safety(iems_node_t *node) {
    return node ? &node->fire_safety : NULL;
}

iems_multi_pcs_dispatcher_t *iems_node_get_pcs_dispatcher(iems_node_t *node) {
    return node ? &node->pcs_dispatcher : NULL;
}

datahub_t *iems_node_get_datahub(iems_node_t *node) {
    return node ? node->hub : NULL;
}

edge_composition_container_t *iems_node_get_container(iems_node_t *node) {
    return node ? node->comp_root : NULL;
}

int iems_node_dispatch_power(iems_node_t *node, double target_kw, double *out_actual_kw) {
    if (!node) return -1;

    /* 1. 消防联动闭锁断开不变量检查 (Fire Safety Invariant) */
    if (node->fire_safety.is_pcs_emergency_stop || node->fire_safety.is_hv_breaker_tripped ||
        node->fire_safety.current_stage >= FIRE_STAGE_2_ALARM) {
        LOG_WARN(LOG_CAT_CORE, "IEMS Dispatch: Rejected! Fire safety interlock active.");
        if (out_actual_kw) *out_actual_kw = 0.0;
        iems_pcs_exec_pq_control(&node->pcs_ctrl, 0.0, 0.0);
        return -2;
    }

    /* 2. BMS 热失控安全不变量检查 (Thermal Runaway Invariant) */
    iems_bms_alarm_level_t highest_alarm = BMS_ALARM_NONE;
    if (node->bms_cluster.is_emergency_trip) {
        highest_alarm = BMS_ALARM_LEVEL_3;
    } else {
        for (uint8_t i = 0; i < node->bms_cluster.rack_count; i++) {
            if (node->bms_cluster.racks[i].is_online &&
                node->bms_cluster.racks[i].alarm_level > highest_alarm) {
                highest_alarm = node->bms_cluster.racks[i].alarm_level;
            }
        }
    }

    if (highest_alarm >= BMS_ALARM_LEVEL_3) {
        LOG_WARN(LOG_CAT_CORE, "IEMS Dispatch: Rejected! BMS Level 3 Thermal Runaway alarm.");
        if (out_actual_kw) *out_actual_kw = 0.0;
        iems_pcs_exec_pq_control(&node->pcs_ctrl, 0.0, 0.0);
        return -3;
    }

    double permitted_kw = target_kw;

    /* 3. BMS 降功率约束 (Level 2 Derating) */
    if (highest_alarm == BMS_ALARM_LEVEL_2) {
        permitted_kw *= 0.5;  // 降半功率
    }

    /* 4. 充放电 SOC 边界约束 */
    if (permitted_kw < 0.0 && node->bms_cluster.sys_avg_soc_pct >= 98.0) {
        permitted_kw = 0.0;  // 充满不充
    } else if (permitted_kw > 0.0 && node->bms_cluster.sys_avg_soc_pct <= 5.0) {
        permitted_kw = 0.0;  // 放空不放
    }

    /* 5. 多 PCS 功率分摊调度 */
    int rc = iems_pcs_dispatcher_dispatch(&node->pcs_dispatcher, permitted_kw, 1.0);
    if (rc != 0) {
        permitted_kw = 0.0;
    }

    /* 6. 执行最终指令下发并同步影子寄存器 */
    iems_pcs_set_mode(&node->pcs_ctrl, PCS_MODE_GRID_PQ);
    iems_pcs_exec_pq_control(&node->pcs_ctrl, permitted_kw, 0.0);

    if (node->hub) {
        pipe_msg_t msg;
        pipe_msg_init_scalar(&msg, IEMS_TAG_PCS_CONTROL, permitted_kw, uv_hrtime());
        datahub_publish(node->hub, &msg);
    }

    if (out_actual_kw) *out_actual_kw = permitted_kw;
    return 0;
}

int iems_node_emergency_trip(iems_node_t *node, const char *reason) {
    if (!node) return -1;
    LOG_ERROR(LOG_CAT_CORE, "🚨 IEMS Emergency Trip Triggered! Reason: %s", reason ? reason : "Unknown");
    node->fire_safety.is_pcs_emergency_stop = true;
    node->fire_safety.is_hv_breaker_tripped = true;
    node->fire_safety.current_stage = FIRE_STAGE_3_RELEASE;
    iems_pcs_set_mode(&node->pcs_ctrl, PCS_MODE_FAULT);
    iems_pcs_exec_pq_control(&node->pcs_ctrl, 0.0, 0.0);

    if (node->hub) {
        pipe_msg_t msg;
        pipe_msg_init_scalar(&msg, IEMS_TAG_PCS_CONTROL, 0.0, uv_hrtime());
        datahub_publish(node->hub, &msg);
    }
    return 0;
}

int iems_node_update_bms_rack(iems_node_t *node, uint8_t rack_id, double voltage_v,
                              double current_a, double soc_pct, double max_temp_c) {
    if (!node) return -1;
    char sn[16];
    snprintf(sn, sizeof(sn), "RACK_%u", (unsigned int)rack_id);
    double cell_v = voltage_v > 0.0 ? (voltage_v / 240.0) : 3.2;
    int rc = iems_bms_update_rack(&node->bms_cluster, rack_id, sn,
                                  voltage_v, current_a, soc_pct, 100.0,
                                  cell_v + 0.01, cell_v - 0.01,
                                  max_temp_c, max_temp_c - 2.0, 0.1, 500.0);
    if (rc == 0) {
        iems_bms_evaluate_cluster_safety(&node->bms_cluster);
        if (node->hub) {
            pipe_msg_t msg;
            pipe_msg_init_scalar(&msg, IEMS_TAG_BATTERY_SOC, node->bms_cluster.sys_avg_soc_pct, uv_hrtime());
            datahub_publish(node->hub, &msg);
            pipe_msg_init_scalar(&msg, IEMS_TAG_BATTERY_POWER, node->bms_cluster.sys_total_power_kw, uv_hrtime());
            datahub_publish(node->hub, &msg);
        }
    }
    return rc;
}
