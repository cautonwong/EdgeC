/**
 * @file dcu_driver_adapter.c
 * @brief DCU 专属南向驱动防腐适配层与静态点表注册实现
 */

#include "dcu_driver_adapter.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "contract_data_types.h"
#include "edge_api.h"
#include "dcu_inventory.h"
#include "dcu_router.h"
#include "modbus_codec.h"
#include "dcu_point_map_generated.h"

/* 外部协议驱动模板声明 */
extern const edge_driver_t modbus_rtu_driver;
extern const edge_driver_t dlt645_driver;
extern const edge_driver_t dlt698_driver;

/* ==============================================================================
 * 1. DCU 专属静态驱动注册表 (替代生成的 edge_driver_registry.c)
 * ============================================================================== */
static const edge_driver_reg_entry_t s_dcu_driver_registry[] = {
    EDGE_DRIVER_ENTRY("ModbusRTU", modbus_rtu_driver),
    EDGE_DRIVER_ENTRY("DLT645", dlt645_driver),
    EDGE_DRIVER_ENTRY("DLT698", dlt698_driver),
};

static const size_t s_dcu_driver_registry_count =
    sizeof(s_dcu_driver_registry) / sizeof(s_dcu_driver_registry[0]);

int dcu_driver_registry_init(driver_manager_t *mgr) {
    if (!mgr) return -1;
    return driver_manager_register_table(mgr, s_dcu_driver_registry, s_dcu_driver_registry_count);
}

/* ==============================================================================
 * 2. DL/T 698.45 静态测点映射表 (以 32-bit OAD 为核心，tag_id 即 oad)
 * ============================================================================== */

/* ==============================================================================
 * 3. DL/T 645-2007 静态测点映射表 (DI 直接映射至对应 698 OAD)
 * ============================================================================== */

/* 台架/现场表地址 (6 字节 BCD, 低字节先), 按 device_id 索引。由 main 从 inventory 填充。 */
static uint8_t s_meter_addr_bcd[MAX_METERS][6];

/* 645 不再向驱动暴露任何查找：驱动只认 DI(协议号)，OAD↔DI 与 scale 都在 cmd 层。
 * 表地址 BCD 走 edge_api 的 meta 登记（物理寻址），也不再经过驱动注入。 */

void dcu_driver_adapter_set_meters(const production_meter_t *meters, uint32_t count) {
    if (!meters) return;
    if (count > MAX_METERS) count = MAX_METERS;
    for (uint32_t i = 0; i < count; i++) {
        const char *s = meters[i].address;
        size_t len = strlen(s);
        uint8_t pair[3] = {0, 0, 0};  // pair[0] 最高两位, pair[2] 最低两位
        for (int k = 0; k < 3; k++) {
            uint8_t hi = 0, lo = 0;
            size_t off = len - 6 + 2 * (size_t)k;
            if (len >= (size_t)(6 - 2 * k)) {
                char ch = s[off];
                char cl = s[off + 1];
                if (ch >= '0' && ch <= '9') hi = (uint8_t)(ch - '0');
                if (cl >= '0' && cl <= '9') lo = (uint8_t)(cl - '0');
            }
            pair[k] = (uint8_t)((hi << 4) | lo);
        }
        for (int k = 0; k < 3; k++) s_meter_addr_bcd[i][k] = pair[2 - k];  // A0 = 最低字节
        s_meter_addr_bcd[i][3] = s_meter_addr_bcd[i][4] = s_meter_addr_bcd[i][5] = 0;
    }
    /* [ADR-002] 表地址是物理寻址信息，不是语义映射：走 edge_api 的 meta 登记，
     * 由请求层在 req_prepare 时交给驱动，不再经过 ctx 注入。 */
    edge_api_set_protocol_meta("DLT645", s_meter_addr_bcd, count);
}

/* ==============================================================================
 * 4. Modbus RTU 静态测点映射表
 * ============================================================================== */

/* ---- 消费侧 OAD <-> 线上协议号 的映射：都在 cmd 层，驱动不认识 OAD ----
 * shortcut: 三种协议各写了三段同构循环（约 90 行）。点表结构体类型不同没法直接复用；
 * 等点位再增或出现第四种协议时，抽成一张统一的 (key, tag_id, scaler) 只读视图。 */
uint32_t dcu_driver_adapter_oad_to_proto(const char *proto, uint32_t oad) {
    if (!proto) return 0;
    if (strcasecmp(proto, "DLT645") == 0) {
        for (size_t i = 0; i < s_dlt645_points_count; i++)
            if (s_dlt645_points[i].tag_id == oad) return s_dlt645_points[i].key;
        return 0;
    }
    if (strcasecmp(proto, "DLT698") == 0) {
        for (size_t i = 0; i < sizeof(s_dlt698_points) / sizeof(s_dlt698_points[0]); i++)
            if (s_dlt698_points[i].tag_id == oad) return s_dlt698_points[i].key;
        return 0;
    }
    if (strcasecmp(proto, "ModbusRTU") == 0 || strcasecmp(proto, "Modbus") == 0) {
        /* [ADR-002] Modbus 的点位标识 = 地址 + 寄存器个数，驱动只认这个。
         * 长度不能丢：电流/功率/电量都是 2 个寄存器的 32 位量。 */
        for (size_t i = 0; i < sizeof(s_modbus_points) / sizeof(s_modbus_points[0]); i++)
            if (s_modbus_points[i].tag_id == oad)
                return MODBUS_NATIVE(s_modbus_points[i].key,
                                     s_modbus_points[i].length ? s_modbus_points[i].length : 1);
        return 0;
    }
    return 0;
}

uint32_t dcu_driver_adapter_proto_to_oad(const char *proto, uint32_t proto_id) {
    if (!proto) return 0;
    if (strcasecmp(proto, "DLT645") == 0) {
        for (size_t i = 0; i < s_dlt645_points_count; i++)
            if (s_dlt645_points[i].key == proto_id) return (uint32_t)s_dlt645_points[i].tag_id;
        return 0;
    }
    if (strcasecmp(proto, "DLT698") == 0) {
        for (size_t i = 0; i < sizeof(s_dlt698_points) / sizeof(s_dlt698_points[0]); i++)
            if (s_dlt698_points[i].key == proto_id) return (uint32_t)s_dlt698_points[i].tag_id;
        return 0;
    }
    if (strcasecmp(proto, "ModbusRTU") == 0 || strcasecmp(proto, "Modbus") == 0) {
        uint16_t maddr = MODBUS_NATIVE_ADDR(proto_id);
        for (size_t i = 0; i < sizeof(s_modbus_points) / sizeof(s_modbus_points[0]); i++)
            if (s_modbus_points[i].key == maddr) return (uint32_t)s_modbus_points[i].tag_id;
        return 0;
    }
    return 0;
}

double dcu_driver_adapter_scale(const char *proto, uint32_t proto_id) {
    if (!proto) return 0.0;
    if (strcasecmp(proto, "DLT645") == 0) {
        for (size_t i = 0; i < s_dlt645_points_count; i++)
            if (s_dlt645_points[i].key == proto_id) return (double)s_dlt645_points[i].scaler;
        return 0.0;
    }
    if (strcasecmp(proto, "DLT698") == 0) {
        for (size_t i = 0; i < sizeof(s_dlt698_points) / sizeof(s_dlt698_points[0]); i++)
            if (s_dlt698_points[i].key == proto_id) return (double)s_dlt698_points[i].scaler;
        return 0.0;
    }
    if (strcasecmp(proto, "ModbusRTU") == 0 || strcasecmp(proto, "Modbus") == 0) {
        uint16_t saddr = MODBUS_NATIVE_ADDR(proto_id);
        for (size_t i = 0; i < sizeof(s_modbus_points) / sizeof(s_modbus_points[0]); i++)
            if (s_modbus_points[i].key == saddr) return (double)s_modbus_points[i].scaler;
        return 0.0;
    }
    return 0.0;
}

