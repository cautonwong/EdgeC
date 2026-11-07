#include "iems_north_adapter.h"

#include <math.h>
#include <string.h>

#include "edge_log.h"
#include "iems_driver_adapter.h"
#include "modbus_codec.h"
#include "mqtt_json_codec.h"
#include "northbound_mgr.h"

static int _peek(void *ctx_v, uint32_t tag_id, pipe_msg_t *out) {
    iems_north_ctx_t *ctx = (iems_north_ctx_t *)ctx_v;
    if (!ctx || !ctx->hub || !out) return -1;
    return datahub_read_shadow(ctx->hub, tag_id, out);
}

static const char *_name(void *ctx_v, uint32_t tag_id) {
    (void)ctx_v;
    const edge_tag_meta_t *meta = iems_get_tag_meta(tag_id);
    return meta ? meta->name : NULL;
}

static int _resolve_name(void *ctx_v, const char *name, uint32_t *tag_id) {
    (void)ctx_v;
    if (!name || !tag_id) return -1;
    for (size_t i = 0; i < g_tag_metadata_len; i++) {
        if (g_tag_metadata[i].name && strcmp(g_tag_metadata[i].name, name) == 0) {
            *tag_id = (uint32_t)g_tag_metadata[i].id;
            return 0;
        }
    }
    return -1;
}

static int _on_downlink(void *ctx_v, const char *address, const uint8_t *payload, size_t len) {
    iems_north_ctx_t *ctx = (iems_north_ctx_t *)ctx_v;
    if (!ctx || !ctx->north) return -1;
    northbound_mgr_t *north = (northbound_mgr_t *)ctx->north;

    char name[64];
    double value = 0.0;
    if (mqtt_json_decode_downlink(payload, len, name, sizeof(name), &value) != 0) {
        LOG_WARN(LOG_CAT_NB, "IEMS downlink on '%s': 不是 {p,v} 帧", address);
        (void)northbound_mgr_publish_ack(north, 0, NULL, false);
        return -1;
    }

    uint32_t tag_id = 0;
    if (_resolve_name(ctx, name, &tag_id) != 0) {
        LOG_WARN(LOG_CAT_NB, "IEMS downlink on '%s': 点位 '%s' 不在点位表里，拒绝", address,
                 name);
        (void)northbound_mgr_publish_ack(north, 0, name, false);
        return -1;
    }

    int rc = ctx->submit_control(ctx->submit_ctx, 0, tag_id, value, "ModbusRTU");
    (void)northbound_mgr_publish_ack(north, tag_id, name, rc == 0);
    return rc;
}

static int _accept(void *ctx_v, uint32_t point_id, double value) {
    iems_north_ctx_t *ctx = (iems_north_ctx_t *)ctx_v;
    if (!ctx || !ctx->node) return -1;

    uint16_t addr = MODBUS_NATIVE_ADDR(point_id);
    const modbus_map_t *map = edge_get_map_modbus(addr);
    if (!map) {
        LOG_WARN(LOG_CAT_NB, "IEMS north write: 寄存器 %u 没有点位映射，拒绝", (unsigned)addr);
        return -1;
    }

    double sc = (map->scaler > 0.0) ? (double)map->scaler : 1.0;
    double eng = value * sc;
    int rc = ctx->submit_control(ctx->submit_ctx, 0, map->tag_id, eng, "ModbusRTU");
    LOG_INFO(LOG_CAT_NB, "IEMS north write: addr=%u tag=%u raw=%.0f eng=%.6g -> rc=%d",
             (unsigned)addr, map->tag_id, value, eng, rc);
    return rc;
}

static int _resolve(void *ctx_v, uint32_t point_id, const void *point_desc,
                    size_t point_desc_len, edge_point_value_t *out) {
    (void)point_desc;
    (void)point_desc_len;
    iems_north_ctx_t *ctx = (iems_north_ctx_t *)ctx_v;
    if (!ctx || !ctx->hub || !out) return -1;

    uint16_t addr = MODBUS_NATIVE_ADDR(point_id);
    const modbus_map_t *map = edge_get_map_modbus(addr);
    if (!map) return -1;

    pipe_msg_t msg;
    if (datahub_read_shadow(ctx->hub, map->tag_id, &msg) != 0) return -1;

    double eng = msg.payload.val.f;
    double sc = (map->scaler > 0.0) ? (double)map->scaler : 1.0;
    out->raw = (int32_t)lround(eng / sc);
    out->width = 1;
    out->value = eng;
    return 0;
}

const edge_north_source_ops_t *iems_north_source_ops(void) {
    static const edge_north_source_ops_t ops = {
        .peek = _peek,
        .name = _name,
        .resolve_name = _resolve_name,
        .accept = NULL,
        .on_downlink = _on_downlink,
    };
    return &ops;
}

const edge_slave_source_ops_t *iems_north_slave_ops(void) {
    static const edge_slave_source_ops_t ops = {
        .resolve = _resolve,
        .accept = _accept,
    };
    return &ops;
}
