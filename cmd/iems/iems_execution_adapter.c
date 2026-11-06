#include "iems_execution_adapter.h"

#include <math.h>

#include "edge_api.h"
#include "edge_log.h"
#include "iems_driver_adapter.h"

static int iems_exec_write(void *ctx, int bus_id, uint32_t tag_id, double value, const char *proto) {
    (void)ctx;
    /* 驱动是纯 codec，不认识点位模型：tag → 原生标识 + 原始值只在这里换算。 */
    uint32_t native_id = tag_id;
    double raw = value;
    const modbus_map_t *map = edge_get_map_modbus_by_tag(tag_id);
    if (map) {
        native_id = (uint32_t)map->key;
        double sc = (map->scaler > 0.0) ? (double)map->scaler : 1.0;
        raw = (double)lround(value / sc);
    } else {
        LOG_WARN(LOG_CAT_CORE,
                 "IEMS control: tag %u 不在 modbus 点位表里，按原生 id 下发（地址可能与预期不符）",
                 tag_id);
    }
    return edge_api_write_tag(bus_id, proto, 1, native_id, raw, NULL, NULL);
}

iems_execution_port_t iems_execution_port_create(void *ctx) {
    return (iems_execution_port_t){.ctx = ctx, .write_tag = iems_exec_write};
}
