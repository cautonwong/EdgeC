#include "iems_north_adapter.h"

#include <math.h>
#include <string.h>

#include "edge_log.h"
#include "iec_asdu.h"
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

/* 控制写要落在点位实际所在的设备上；没有唯一轮询路由就拒绝。 */
static int _route_for_tag(iems_north_ctx_t *ctx, uint32_t tag_id, iems_device_route_t *route) {
    if (!ctx || !ctx->route_for_tag || !route) return -1;
    return ctx->route_for_tag(ctx->route_ctx, tag_id, route);
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

    iems_device_route_t route;
    int route_rc = _route_for_tag(ctx, tag_id, &route);
    if (route_rc != 0) {
        LOG_WARN(LOG_CAT_NB, "IEMS downlink on '%s': tag %u 没有唯一轮询路由 (rc=%d)，拒绝", address,
                 tag_id, route_rc);
        (void)northbound_mgr_publish_ack(north, tag_id, name, false);
        return -1;
    }

    int rc = ctx->submit_control(ctx->submit_ctx, &route, tag_id, value);
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
    iems_device_route_t route;
    int route_rc = _route_for_tag(ctx, map->tag_id, &route);
    if (route_rc != 0) {
        LOG_WARN(LOG_CAT_NB, "IEMS north write: tag %u 没有唯一轮询路由 (rc=%d)，拒绝",
                 map->tag_id, route_rc);
        return -1;
    }
    int rc = ctx->submit_control(ctx->submit_ctx, &route, map->tag_id, eng);
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

int iems_control_route_for_tag(void *ctx, uint32_t tag_id, iems_device_route_t *route) {
    return iems_poll_task_route((sqlite_svc_t *)ctx, tag_id, route);
}

int iems_control_submit(void *ctx, const iems_device_route_t *route, uint32_t tag_id,
                        double value) {
    iems_node_t *node = (iems_node_t *)ctx;
    if (!node || !route) return -1;
    int rc = iems_node_write_tag(node, route->bus_id, route->slave_id, tag_id, value,
                                 route->protocol);
    if (rc < 0) {
        LOG_WARN(LOG_CAT_CORE, "IEMS control write rejected by aggregate root (rc=%d)", rc);
    }
    return rc;
}

int iems_control_submit_by_tag(void *ctx, int bus_id, uint32_t tag_id, double value,
                               const char *proto) {
    iems_control_write_ctx_t *c = (iems_control_write_ctx_t *)ctx;
    if (!c || !c->node) return -1;

    /* 反射链路只带 tag；目标设备必须查轮询路由，拿不到就拒绝，不猜从站地址。 */
    iems_device_route_t route;
    int route_rc = c->db ? iems_poll_task_route(c->db, tag_id, &route) : -1;
    if (route_rc != 0) {
        LOG_WARN(LOG_CAT_CORE,
                 "IEMS reflex control: tag %u 没有唯一轮询路由 (rc=%d, bus=%d, proto=%s)", tag_id,
                 route_rc, bus_id, proto ? proto : "?");
        return -1;
    }
    return iems_control_submit(c->node, &route, tag_id, value);
}

/* ---- IEC104 从站面：驱动只给协议原生标识，IOA → tag 的翻译留在产品侧 ---- */

static int _iec104_resolve(void *ctx_v, uint32_t point_id, const void *point_desc,
                           size_t point_desc_len, edge_point_value_t *out) {
    (void)point_desc;
    (void)point_desc_len;
    iems_iec104_source_ctx_t *ctx = (iems_iec104_source_ctx_t *)ctx_v;
    if (!ctx || !ctx->hub || !out) return -1;

    const iec104_map_t *map = edge_get_map_iec104(IEC_NATIVE_IOA(point_id));
    if (!map) return -1;

    pipe_msg_t msg;
    if (datahub_read_shadow(ctx->hub, map->tag_id, &msg) != 0) return -1;

    double eng = msg.payload.val.f;
    double sc = (map->scaler > 0.0f) ? (double)map->scaler : 1.0;
    out->value = eng;
    out->raw = (int32_t)lround(eng / sc);
    out->width = 1;
    return 0;
}

/* 遥控/遥调：先校验 IOA 落在 v3 的控制地址段并有控制点映射，再按 tag
 * 反查唯一设备路由；查不到就不猜从站地址。 */
static int _iec104_accept(void *ctx_v, uint32_t point_id, double value) {
    iems_iec104_source_ctx_t *ctx = (iems_iec104_source_ctx_t *)ctx_v;
    if (!ctx || !ctx->node || !ctx->db) return -1;

    edge_tag_id_t tag_id = 0;
    if (iems_iec104_control_tag(IEC_NATIVE_IOA(point_id), &tag_id) != 0) return -1;

    iems_device_route_t route;
    int rc = iems_control_route_for_tag(ctx->db, tag_id, &route);
    if (rc != 0) {
        LOG_WARN(LOG_CAT_NB, "IEMS IEC104 write: tag %u 没有唯一轮询路由 (rc=%d)，拒绝",
                 tag_id, rc);
        return -1;
    }
    if (route.bus_id == ctx->north_bus_id) {
        LOG_WARN(LOG_CAT_NB, "IEMS IEC104 write: tag %u 路由回北向总线 %d，拒绝", tag_id,
                 route.bus_id);
        return -1;
    }
    return iems_control_submit(ctx->node, &route, tag_id, value);
}

static int _iec104_enumerate(void *ctx_v, void (*fn)(void *ud, uint32_t point_id), void *ud) {
    (void)ctx_v;
    if (!fn) return -1;

    size_t count = 0;
    const iec104_map_t *table = iems_get_iec104_map(&count);
    for (size_t i = 0; i < count; i++) {
        uint8_t type = table[i].type_id ? table[i].type_id : (uint8_t)IEC_TYPE_M_ME_NC_1;
        fn(ud, IEC_NATIVE_ID(type, (uint32_t)table[i].key));
    }
    return 0;
}

/* 从站面端口：取数 / 收数 / 遍历 / 按点死区。 */
const edge_slave_source_ops_t *iems_north_iec104_source_ops(void) {
    static const edge_slave_source_ops_t ops = {
        .resolve = _iec104_resolve,
        .accept = _iec104_accept,
        .enumerate = _iec104_enumerate,
        .deadband = iems_iec104_deadband,
    };
    return &ops;
}
