/**
 * @file dcu_driver_adapter.c
 * @brief DCU 专属南向驱动防腐适配层与静态点表注册实现
 */

#include "dcu_driver_adapter.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>

#include "contract_data_types.h"
#include "edge_api.h"
#include "dcu_698_rules.h"
#include "dcu_inventory.h"
#include "dcu_router.h"
#include "modbus_codec.h"

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

int dcu_driver_registry_init(const edge_driver_registry_port_t *registry) {
    if (!registry || !registry->register_table) return -1;
    return registry->register_table(registry->ctx, s_dcu_driver_registry, s_dcu_driver_registry_count);
}

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

static uint8_t _proto_of(const char *proto) {
    if (strcasecmp(proto, "DLT645") == 0) return DCU_RULE_DLT645_2007;
    if (strcasecmp(proto, "DLT698") == 0) return DCU_RULE_DLT698;
    if (strcasecmp(proto, "ModbusRTU") == 0 || strcasecmp(proto, "Modbus") == 0) {
        return DCU_RULE_MODBUS;
    }
    return 0;
}

uint32_t dcu_driver_adapter_oad_to_proto(const char *proto, uint32_t oad) {
    if (!proto) return 0;
    uint8_t p = _proto_of(proto);
    if (!p) return 0;
    dcu_rule_row_t row;
    if (dcu_698_rules_find_by_oad(p, oad, &row) != 0) return 0;
    if (p == DCU_RULE_MODBUS) return MODBUS_NATIVE(row.protocol_id, row.length ? row.length : 1);
    return row.protocol_id;
}

uint32_t dcu_driver_adapter_proto_to_oad(const char *proto, uint32_t proto_id) {
    if (!proto) return 0;
    uint8_t p = _proto_of(proto);
    if (!p) return 0;
    if (p == DCU_RULE_MODBUS) proto_id = MODBUS_NATIVE_ADDR(proto_id);
    dcu_rule_row_t row;
    if (dcu_698_rules_find_by_proto(p, proto_id, &row) != 0) return 0;
    return row.oad;
}

double dcu_driver_adapter_scale(const char *proto, uint32_t proto_id) {
    if (!proto) return 0.0;
    uint8_t p = _proto_of(proto);
    if (!p) return 0.0;
    if (p == DCU_RULE_MODBUS) proto_id = MODBUS_NATIVE_ADDR(proto_id);
    dcu_rule_row_t row;
    if (dcu_698_rules_find_by_proto(p, proto_id, &row) != 0) return 0.0;
    return (double)row.scale;
}
