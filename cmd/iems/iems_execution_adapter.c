#include "iems_execution_adapter.h"

#include <math.h>
#include <strings.h>

#include "edge_api.h"
#include "edge_log.h"
#include "iems_driver_adapter.h"
#include "modbus_codec.h"

void iems_exec_translate_write(const char *proto, uint32_t tag_id, double value,
                               uint32_t *native_id, double *raw) {
    if (!native_id || !raw) return;
    /* 驱动是纯 codec，不认识点位模型。原生标识复用读路径的翻译：Modbus 带
     * 寄存器个数、CAN 换 CAN ID、DLT645 换 DI；标定沿用原有口径。 */
    *native_id = iems_driver_adapter_native_id(proto, tag_id);
    *raw = value;
    /* 只有 Modbus 家族的写值要按点位表标定；其它协议的原生值口径不同，
     * 混用 point 表会把 CAN/DLT645 的写值算错。 */
    bool modbus_family = proto && (strcasecmp(proto, "ModbusRTU") == 0 ||
                                   strcasecmp(proto, "ModbusRTUHiCRC") == 0 ||
                                   strcasecmp(proto, "ModbusTCP") == 0 ||
                                   strcasecmp(proto, "Modbus") == 0);
    const modbus_map_t *map = modbus_family ? edge_get_map_modbus_by_tag(tag_id) : NULL;
    if (map) {
        double sc = (map->scaler > 0.0) ? (double)map->scaler : 1.0;
        *raw = (double)lround(value / sc);
    } else if (*native_id == tag_id) {
        LOG_WARN(LOG_CAT_CORE,
                 "IEMS control: tag %u 不在点位表里，按原生 id 下发（地址可能与预期不符）",
                 tag_id);
    }
}

static int iems_exec_write(void *ctx, int bus_id, uint32_t tag_id, double value, const char *proto) {
    (void)ctx;
    uint32_t native_id = tag_id;
    double raw = value;
    iems_exec_translate_write(proto, tag_id, value, &native_id, &raw);
    return edge_api_write_tag(bus_id, proto, 1, native_id, raw, NULL, NULL);
}

iems_execution_port_t iems_execution_port_create(void *ctx) {
    return (iems_execution_port_t){.ctx = ctx, .write_tag = iems_exec_write};
}
