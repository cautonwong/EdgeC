/**
 * @file dcu_node.c
 * @brief DCU Edge Collector Domain Aggregate Root Implementation (Issue #375)
 */

#include "dcu_driver_adapter.h"
#include "dcu_node.h"

#include <stdlib.h>
#include <string.h>

#include "edge_log.h"


struct dcu_node_s {
    uv_loop_t *loop;
    dcu_node_config_t cfg;
    edge_composition_container_storage_t comp_storage;
    edge_composition_container_t *comp_root;
    datahub_t *hub;
    dcu_inventory_t inventory;
    dcu_poller_t poller;
    dcu_storage_t storage;
    dcu_event_obj_t evt_3000;
    dcu_event_obj_t evt_3005;
    uv_timer_t *poller_timer;
    dcu_event_handler_t event_handler;
    void *event_user_data;
    bool running;
    uint32_t saved_since_take; /* 上一轮以来 record_reading 成功落库的条数 */
};

static void _timer_close_cb(uv_handle_t *handle) {
    free(handle);
}

static void _dcu_node_poller_timer_cb(uv_timer_t *handle) {
    dcu_node_t *node = (dcu_node_t *)handle->data;
    if (node && node->running) {
        dcu_node_pump(node);
    }
}

static int _node_publish_realtime(void *ctx, uint32_t tag_id, uint32_t meter_idx, double val,
                                  uint64_t timestamp_ns) {
    dcu_node_t *node = (dcu_node_t *)ctx;
    if (!node || !node->hub) return -1;
    pipe_msg_t bus_msg;
    pipe_msg_init_scalar(&bus_msg, tag_id, val, timestamp_ns);
    bus_msg.device_id = meter_idx;
    return datahub_publish(node->hub, &bus_msg);
}

static int _node_read_shadow(void *ctx, uint32_t tag_id, uint32_t device_id, void *out_msg) {
    dcu_node_t *node = (dcu_node_t *)ctx;
    if (!node || !node->hub) return -1;
    return datahub_read_shadow_device(node->hub, tag_id, device_id, (pipe_msg_t *)out_msg);
}

static int _node_count_freezes(void *ctx, uint16_t *out_count) {
    dcu_node_t *node = (dcu_node_t *)ctx;
    if (!node) return -1;
    return dcu_storage_count_freezes(&node->storage, out_count);
}

extern int dcu_tag_map_fn(uint32_t tag_id, uint32_t device_id);

dcu_node_t *dcu_node_create(uv_loop_t *loop, const dcu_node_config_t *cfg) {
    if (!loop || !cfg) return NULL;

    dcu_node_t *node = (dcu_node_t *)calloc(1, sizeof(dcu_node_t));
    if (!node) return NULL;

    node->loop = loop;
    node->cfg = *cfg;
    if (!node->cfg.product_name) node->cfg.product_name = "DCU";
    if (!node->cfg.poll_interval_ms) node->cfg.poll_interval_ms = 1000;
    /* 影子表按槽位公式索引：slot = device_id * DCU_SLOTS_PER_DEVICE + tag_slot。
     * 容量必须容得下 MAX_METERS 只表，否则 idx 越界、写入被静默丢弃
     * （32768 只够 1024 只表，而 MAX_METERS 是 4096）。 */
    if (!node->cfg.datahub_capacity) {
        node->cfg.datahub_capacity = MAX_METERS * DCU_SLOTS_PER_DEVICE;
    }
    if (!node->cfg.ring_capacity) node->cfg.ring_capacity = 32768;
    if (node->cfg.datahub_capacity < (size_t)MAX_METERS * DCU_SLOTS_PER_DEVICE) {
        LOG_WARN(LOG_CAT_SCHED,
                 "datahub_capacity=%zu < MAX_METERS*SLOTS=%d：高位表的影子值会被丢弃",
                 node->cfg.datahub_capacity, MAX_METERS * DCU_SLOTS_PER_DEVICE);
    }

    /* 1. Assemble Foundation via Edge Composition Root */
    edge_composition_config_t comp_cfg = {.product_name = node->cfg.product_name,
                                          .loop = node->loop,
                                          .shm_name = node->cfg.product_name,
                                          .ring_capacity = node->cfg.ring_capacity,
                                          .datahub_capacity = node->cfg.datahub_capacity,
                                          .map_fn_v2 = dcu_tag_map_fn,
                                          .memory_pool_buffer_size = 4096,
                                          .memory_pool_capacity = 16,
                                          .enable_dma_buf = false,
                                          .enable_anti_corruption = true};
    node->comp_root = edge_composition_root_create_in(&node->comp_storage, &comp_cfg);
    if (!node->comp_root) {
        LOG_ERROR(LOG_CAT_CORE, "DCU Node: Failed to instantiate Composition Root");
        free(node);
        return NULL;
    }

    node->hub = edge_composition_root_get_datahub(node->comp_root);

    /* 2. Initialize Encapsulated Storage Sink */
    if (node->cfg.db_path && strlen(node->cfg.db_path) > 0) {
        int rc = dcu_storage_init(&node->storage, loop, node->cfg.db_path);
        if (rc != 0) {
            LOG_ERROR(LOG_CAT_CORE, "DCU Node: Failed to initialize storage at %s",
                      node->cfg.db_path);
            edge_composition_root_destroy(node->comp_root);
            free(node);
            return NULL;
        }

        /* 3. Load Smart Meter Inventory from Storage */
        dcu_inventory_load_from_db(&node->inventory, node->cfg.db_path);
    }

    /* 4. Configure Domain Event Objects */
    memset(&node->evt_3000, 0, sizeof(dcu_event_obj_t));
    node->evt_3000.oi = OI_VOLTAGE_DROP;
    node->evt_3000.threshold = 176.0f;
    node->evt_3000.delay_s = 0;
    node->evt_3000.is_enabled = true;
    node->evt_3000.snapshot_count = 2;
    node->evt_3000.snapshot_oads[0] = OAD_VOLTAGE;
    node->evt_3000.snapshot_oads[1] = OAD_ACTIVE_ENERGY;

    memset(&node->evt_3005, 0, sizeof(dcu_event_obj_t));
    node->evt_3005.oi = OI_OVER_CURRENT;
    node->evt_3005.threshold = 5.0f;
    node->evt_3005.delay_s = 0;
    node->evt_3005.is_enabled = true;
    node->evt_3005.snapshot_count = 1;
    node->evt_3005.snapshot_oads[0] = OAD_CURRENT;

    /* 5. Initialize Poller and Wire Router Operations */
    dcu_poller_init(&node->poller);
    uint32_t oads[1] = {0x20000200};
    dcu_poller_add_scheme(&node->poller, 1, 0, oads, 1);
    dcu_poller_add_task(&node->poller, 1, 5, 1);

    poller_sink_ops_t poller_sink = {.ctx = &node->storage,
                                     .record_telemetry = (void *)dcu_storage_write_telemetry,
                                     .commit_batch = (void *)dcu_storage_flush};
    poller_bus_ops_t poller_bus = {
        .ctx = node, .publish_realtime = _node_publish_realtime, .push_event = NULL};
    dcu_poller_start(&node->poller, node->cfg.source_ops, poller_bus, poller_sink);

    router_sink_ops_t router_sink = {.ctx = &node->storage,
                                     .record_telemetry = (void *)dcu_storage_write_telemetry,
                                     .count_freezes = _node_count_freezes};
    router_bus_ops_t router_bus = {.ctx = node,
                                   .msg_size = sizeof(pipe_msg_t),
                                   .publish_realtime = _node_publish_realtime,
                                   .read_shadow = _node_read_shadow};
    dcu_router_init_ex(&node->inventory, &node->poller, router_sink, NULL, NULL, &node->evt_3000,
                       &node->evt_3005, router_bus);

    /* 6. Setup Poller Periodic Timer */
    node->poller_timer = (uv_timer_t *)calloc(1, sizeof(uv_timer_t));
    if (node->poller_timer) {
        uv_timer_init(loop, node->poller_timer);
        node->poller_timer->data = node;
    }

    LOG_INFO(LOG_CAT_CORE, "DCU Node Aggregate Root assembled successfully (Meters: %d)",
             node->inventory.count);
    return node;
}

void dcu_node_set_source_ops(dcu_node_t *node, poller_source_ops_t source_ops) {
    if (!node) return;
    node->cfg.source_ops = source_ops;
    node->poller.source_ops = source_ops;
}

int dcu_node_start(dcu_node_t *node) {
    if (!node || node->running) return -1;
    node->running = true;
    if (node->poller_timer) {
        uv_timer_start(node->poller_timer, _dcu_node_poller_timer_cb, 0,
                       node->cfg.poll_interval_ms);
    }
    LOG_INFO(LOG_CAT_CORE, "DCU Node started with polling interval %u ms",
             node->cfg.poll_interval_ms);
    return 0;
}

int dcu_node_stop(dcu_node_t *node) {
    if (!node || !node->running) return -1;
    node->running = false;
    if (node->poller_timer) {
        uv_timer_stop(node->poller_timer);
    }
    dcu_storage_flush(&node->storage);
    LOG_INFO(LOG_CAT_CORE, "DCU Node stopped successfully");
    return 0;
}

int dcu_node_pump(dcu_node_t *node) {
    if (!node) return -1;
    dcu_poller_pump(&node->poller, &node->inventory);
    return 0;
}

int dcu_node_record_reading(dcu_node_t *node, uint32_t device_id, uint32_t proto_id,
                            const uint8_t *raw, size_t raw_len) {
    if (!node || !raw || raw_len < sizeof(double)) return -1;
    if (device_id >= node->inventory.count) return -1;

    const production_meter_t *m = &node->inventory.meters[device_id];

    /* 协议号(线上 DI/寄存器) → OAD 与量纲：驱动只还原整数，换算全在 cmd 层。
     * 未注册的协议号一律拒收，不静默塞 0。 */
    uint32_t oad = dcu_driver_adapter_proto_to_oad(m->protocol, proto_id);
    double scale = dcu_driver_adapter_scale(m->protocol, proto_id);
    if (oad == 0 || scale <= 0.0) return -1;

    double value = (*(const double *)raw) * scale;

    if (dcu_storage_write_telemetry(&node->storage, m->address, device_id, oad, value) != 0)
        return -1;
    node->saved_since_take++;
    return 0;
}

uint32_t dcu_node_take_saved(dcu_node_t *node) {
    if (!node) return 0;
    uint32_t n = node->saved_since_take;
    node->saved_since_take = 0;
    return n;
}

dcu_inventory_t *dcu_node_get_inventory(dcu_node_t *node) {
    return node ? &node->inventory : NULL;
}

dcu_poller_t *dcu_node_get_poller(dcu_node_t *node) {
    return node ? &node->poller : NULL;
}

dcu_storage_t *dcu_node_get_storage(dcu_node_t *node) {
    return node ? &node->storage : NULL;
}

datahub_t *dcu_node_get_datahub(dcu_node_t *node) {
    return node ? node->hub : NULL;
}

edge_composition_container_t *dcu_node_get_container(dcu_node_t *node) {
    return node ? node->comp_root : NULL;
}

dcu_event_obj_t *dcu_node_get_event_3000(dcu_node_t *node) {
    return node ? &node->evt_3000 : NULL;
}

dcu_event_obj_t *dcu_node_get_event_3005(dcu_node_t *node) {
    return node ? &node->evt_3005 : NULL;
}

int dcu_node_set_event_handler(dcu_node_t *node, dcu_event_handler_t handler, void *user_data) {
    if (!node) return -1;
    node->event_handler = handler;
    node->event_user_data = user_data;
    return 0;
}

int dcu_node_feed_meter_reading(dcu_node_t *node, const char *meter_addr, uint32_t oad,
                                double val, uint64_t ts_ns) {
    if (!node || !meter_addr) return -1;

    /* 1. 领域不变量检查：电表是否在当前档案中 */
    production_meter_t *meter = dcu_inventory_get(&node->inventory, meter_addr);
    if (!meter) {
        LOG_WARN(LOG_CAT_CORE, "DCU Feed Reading: Meter '%s' not registered in inventory", meter_addr);
        return -2;
    }

    uint32_t dev_id = (uint32_t)(meter - node->inventory.meters);
    uint64_t now_ns = ts_ns ? ts_ns : (node->loop ? uv_hrtime() : 0);

    /* 2. 持久化至存储聚合 */
    dcu_storage_write_telemetry(&node->storage, meter_addr, dev_id, oad, val);

    /* 3. 推入 DataHub 出站通道 (Outbound Channel) */
    if (node->hub) {
        pipe_msg_t bus_msg;
        pipe_msg_init_scalar(&bus_msg, oad, val, now_ns);
        bus_msg.device_id = dev_id;
        datahub_publish(node->hub, &bus_msg);
    }

    /* 4. 越限异常评估 (失压 / 过流领域事件) */
    bool alarm_triggered = false;
    dcu_event_type_t evt_type = DCU_EVENT_TELEMETRY;

    if (oad == OAD_VOLTAGE && val < (double)node->evt_3000.threshold) {
        evt_type = DCU_EVENT_VOLTAGE_DROP;
        alarm_triggered = true;
    } else if (oad == OAD_CURRENT && val > (double)node->evt_3005.threshold) {
        evt_type = DCU_EVENT_OVER_CURRENT;
        alarm_triggered = true;
    }

    /* 5. 广播领域事件 (微服务架构消息出口) */
    if (node->event_handler) {
        dcu_domain_event_t evt = {
            .type = evt_type,
            .oad = oad,
            .value = val,
            .timestamp_ns = now_ns,
            .is_alarm_active = alarm_triggered,
        };
        strncpy(evt.meter_addr, meter_addr, sizeof(evt.meter_addr) - 1);
        node->event_handler(&evt, node->event_user_data);
    }

    return 0;
}

int dcu_node_execute_control(dcu_node_t *node, const char *meter_addr, uint16_t control_oi) {
    if (!node || !meter_addr) return -1;

    /* 1. 领域前置检查：电表是否存在且在线 */
    production_meter_t *meter = dcu_inventory_get(&node->inventory, meter_addr);
    if (!meter || !meter->is_online) {
        LOG_WARN(LOG_CAT_CORE, "DCU Control: Meter '%s' offline or not found", meter_addr);
        return -2;
    }

    /* 2. 遥控动作白名单校验 (防误操作) */
    if (control_oi != OI_REMOTE_CONTROL) {
        LOG_WARN(LOG_CAT_CORE, "DCU Control: Unauthorized control OI 0x%04X", control_oi);
        return -3;
    }

    /* 3. 广播动作事件至微服务事件总线 */
    if (node->event_handler) {
        dcu_domain_event_t evt = {
            .type = DCU_EVENT_REMOTE_TRIP,
            .oad = ((uint32_t)control_oi << 16) | 0x0200,
            .value = 1.0,
            .timestamp_ns = node->loop ? uv_hrtime() : 0,
            .is_alarm_active = false,
        };
        strncpy(evt.meter_addr, meter_addr, sizeof(evt.meter_addr) - 1);
        node->event_handler(&evt, node->event_user_data);
    }

    LOG_INFO(LOG_CAT_CORE, "DCU Control: Remote trip command dispatched for meter %s", meter_addr);
    return 0;
}

void dcu_node_destroy(dcu_node_t *node) {
    if (!node) return;
    if (node->running) {
        dcu_node_stop(node);
    }
    if (node->poller_timer) {
        uv_timer_stop(node->poller_timer);
        uv_close((uv_handle_t *)node->poller_timer, _timer_close_cb);
        node->poller_timer = NULL;
    }
    dcu_storage_destroy(&node->storage);
    edge_composition_root_destroy(node->comp_root);
    if (node->loop) {
        uv_run(node->loop, UV_RUN_NOWAIT);
    }
    free(node);
}
