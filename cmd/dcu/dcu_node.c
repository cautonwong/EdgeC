/**
 * @file dcu_node.c
 * @brief DCU Edge Collector Domain Aggregate Root Implementation (Issue #375)
 */

#include "dcu_node.h"

#include <stdlib.h>
#include <string.h>

#include "contracts.h"
#include "edge_log.h"

/* Global fallback for legacy modules referencing g_dcu_inventory */
dcu_inventory_t g_dcu_inventory = {0};

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
    bool running;
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

static int _node_read_shadow(void *ctx, uint32_t idx, void *out_msg) {
    dcu_node_t *node = (dcu_node_t *)ctx;
    if (!node || !node->hub) return -1;
    return datahub_read_shadow_idx(node->hub, idx, (pipe_msg_t *)out_msg);
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
    if (!node->cfg.datahub_capacity) node->cfg.datahub_capacity = 32768;

    /* 1. Assemble Foundation via Edge Composition Root */
    edge_composition_config_t comp_cfg = {.product_name = node->cfg.product_name,
                                          .loop = node->loop,
                                          .shm_name = node->cfg.product_name,
                                          .ring_capacity = node->cfg.datahub_capacity,
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
        g_dcu_inventory = node->inventory;
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
    dcu_poller_start(&node->poller, (poller_io_ops_t){0}, poller_bus, poller_sink);

    router_sink_ops_t router_sink = {.ctx = &node->storage,
                                     .record_telemetry = (void *)dcu_storage_write_telemetry,
                                     .count_freezes = _node_count_freezes};
    router_bus_ops_t router_bus = {.ctx = node,
                                   .msg_size = sizeof(pipe_msg_t),
                                   .publish_realtime = _node_publish_realtime,
                                   .read_shadow_idx = _node_read_shadow};
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
