/**
 * @file dcu_driver_adapter.c
 * @brief DCU 专属南向驱动防腐适配层与静态点表注册实现
 */

#include "dcu_driver_adapter.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "contract_data_types.h"
#include "dcu_inventory.h"
#include "dcu_router.h"

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
static const modbus_map_t s_dlt698_points[] = {
    {.key = OAD_VOLTAGE,              .tag_id = OAD_VOLTAGE,              .scaler = 0.1f,    .length = 2, .access = 0x01},
    {.key = OAD_VOLTAGE_B,            .tag_id = OAD_VOLTAGE_B,            .scaler = 0.1f,    .length = 2, .access = 0x01},
    {.key = OAD_VOLTAGE_C,            .tag_id = OAD_VOLTAGE_C,            .scaler = 0.1f,    .length = 2, .access = 0x01},
    {.key = OAD_CURRENT,              .tag_id = OAD_CURRENT,              .scaler = 0.01f,   .length = 3, .access = 0x01},
    {.key = OAD_CURRENT_B,            .tag_id = OAD_CURRENT_B,            .scaler = 0.01f,   .length = 3, .access = 0x01},
    {.key = OAD_CURRENT_C,            .tag_id = OAD_CURRENT_C,            .scaler = 0.01f,   .length = 3, .access = 0x01},
    {.key = OAD_ACTIVE_ENERGY,        .tag_id = OAD_ACTIVE_ENERGY,        .scaler = 0.01f,   .length = 4, .access = 0x01},
    {.key = OAD_REACTIVE_ENERGY,      .tag_id = OAD_REACTIVE_ENERGY,      .scaler = 0.01f,   .length = 4, .access = 0x01},
    {.key = OAD_ACTIVE_POWER_TOTAL,   .tag_id = OAD_ACTIVE_POWER_TOTAL,   .scaler = 0.0001f, .length = 3, .access = 0x01},
    {.key = OAD_ACTIVE_POWER_A,       .tag_id = OAD_ACTIVE_POWER_A,       .scaler = 0.0001f, .length = 3, .access = 0x01},
    {.key = OAD_ACTIVE_POWER_B,       .tag_id = OAD_ACTIVE_POWER_B,       .scaler = 0.0001f, .length = 3, .access = 0x01},
    {.key = OAD_ACTIVE_POWER_C,       .tag_id = OAD_ACTIVE_POWER_C,       .scaler = 0.0001f, .length = 3, .access = 0x01},
    {.key = OAD_REACTIVE_POWER_TOTAL, .tag_id = OAD_REACTIVE_POWER_TOTAL, .scaler = 0.0001f, .length = 3, .access = 0x01},
    {.key = OAD_POWER_FACTOR_TOTAL,   .tag_id = OAD_POWER_FACTOR_TOTAL,   .scaler = 0.001f,  .length = 2, .access = 0x01},
    {.key = OAD_GRID_FREQUENCY,       .tag_id = OAD_GRID_FREQUENCY,       .scaler = 0.01f,   .length = 2, .access = 0x01},
    {.key = OAD_ZERO_SEQ_CURRENT,     .tag_id = OAD_ZERO_SEQ_CURRENT,     .scaler = 0.01f,   .length = 3, .access = 0x01},
};
static const size_t s_dlt698_points_count = sizeof(s_dlt698_points) / sizeof(s_dlt698_points[0]);

static const void *dcu_lookup_698_by_key(uint64_t key) {
    for (size_t i = 0; i < s_dlt698_points_count; i++) {
        if (s_dlt698_points[i].key == key) {
            return &s_dlt698_points[i];
        }
    }
    return NULL;
}

static const void *dcu_lookup_698_by_tag(uint32_t tag_id) {
    for (size_t i = 0; i < s_dlt698_points_count; i++) {
        if (s_dlt698_points[i].tag_id == tag_id) {
            return &s_dlt698_points[i];
        }
    }
    return NULL;
}

static const driver_contract_injection_t s_dlt698_injection = {
    .lookup_by_key = dcu_lookup_698_by_key,
    .lookup_by_tag = dcu_lookup_698_by_tag,
};

/* ==============================================================================
 * 3. DL/T 645-2007 静态测点映射表 (DI 直接映射至对应 698 OAD)
 * ============================================================================== */
static const dlt645_map_t s_dlt645_points[] = {
    {.key = 0x02010100, .tag_id = OAD_VOLTAGE,              .scaler = 0.1f,    .length = 2, .access = 0x01},
    {.key = 0x02010200, .tag_id = OAD_VOLTAGE_B,            .scaler = 0.1f,    .length = 2, .access = 0x01},
    {.key = 0x02010300, .tag_id = OAD_VOLTAGE_C,            .scaler = 0.1f,    .length = 2, .access = 0x01},
    {.key = 0x02020100, .tag_id = OAD_CURRENT,              .scaler = 0.001f,  .length = 3, .access = 0x01},
    {.key = 0x02020200, .tag_id = OAD_CURRENT_B,            .scaler = 0.001f,  .length = 3, .access = 0x01},
    {.key = 0x02020300, .tag_id = OAD_CURRENT_C,            .scaler = 0.001f,  .length = 3, .access = 0x01},
    {.key = 0x00010000, .tag_id = OAD_ACTIVE_ENERGY,        .scaler = 0.01f,   .length = 4, .access = 0x01},
    {.key = 0x00020000, .tag_id = OAD_REACTIVE_ENERGY,      .scaler = 0.01f,   .length = 4, .access = 0x01},
    {.key = 0x02030000, .tag_id = OAD_ACTIVE_POWER_TOTAL,   .scaler = 0.0001f, .length = 3, .access = 0x01},
    {.key = 0x02030100, .tag_id = OAD_ACTIVE_POWER_A,       .scaler = 0.0001f, .length = 3, .access = 0x01},
    {.key = 0x02030200, .tag_id = OAD_ACTIVE_POWER_B,       .scaler = 0.0001f, .length = 3, .access = 0x01},
    {.key = 0x02030300, .tag_id = OAD_ACTIVE_POWER_C,       .scaler = 0.0001f, .length = 3, .access = 0x01},
    {.key = 0x02040000, .tag_id = OAD_REACTIVE_POWER_TOTAL, .scaler = 0.0001f, .length = 3, .access = 0x01},
    {.key = 0x02060000, .tag_id = OAD_POWER_FACTOR_TOTAL,   .scaler = 0.001f,  .length = 2, .access = 0x01},
    {.key = 0x02800002, .tag_id = OAD_GRID_FREQUENCY,       .scaler = 0.01f,   .length = 2, .access = 0x01},
};
static const size_t s_dlt645_points_count = sizeof(s_dlt645_points) / sizeof(s_dlt645_points[0]);

static const void *dcu_lookup_645_by_key(uint64_t key) {
    for (size_t i = 0; i < s_dlt645_points_count; i++) {
        if (s_dlt645_points[i].key == key) {
            return &s_dlt645_points[i];
        }
    }
    return NULL;
}

static const void *dcu_lookup_645_by_tag(uint32_t tag_id) {
    for (size_t i = 0; i < s_dlt645_points_count; i++) {
        if (s_dlt645_points[i].tag_id == tag_id) {
            return &s_dlt645_points[i];
        }
    }
    return NULL;
}

/* 台架/现场表地址 (6 字节 BCD, 低字节先), 按 device_id 索引。由 main 从 inventory 填充。 */
static uint8_t s_meter_addr_bcd[MAX_METERS][6];

/* 645 不再向驱动暴露契约查找：驱动只认 DI(协议号)，OAD↔DI 与 scale 都在 cmd 层。
 * meta_list（表地址 BCD）仍由这里注入，那是物理寻址不是语义映射。 */
static driver_contract_injection_t s_dlt645_injection = {0};

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
    s_dlt645_injection.meta_list = s_meter_addr_bcd;
    s_dlt645_injection.meta_list_len = count;
}

/* ==============================================================================
 * 4. Modbus RTU 静态测点映射表
 * ============================================================================== */
static const modbus_map_t s_modbus_points[] = {
    {.key = 0,  .tag_id = OAD_VOLTAGE,              .scaler = 0.1f,    .length = 1, .access = 0x01},
    {.key = 1,  .tag_id = OAD_VOLTAGE_B,            .scaler = 0.1f,    .length = 1, .access = 0x01},
    {.key = 2,  .tag_id = OAD_VOLTAGE_C,            .scaler = 0.1f,    .length = 1, .access = 0x01},
    {.key = 6,  .tag_id = OAD_CURRENT,              .scaler = 0.01f,   .length = 2, .access = 0x01},
    {.key = 7,  .tag_id = OAD_CURRENT_B,            .scaler = 0.01f,   .length = 2, .access = 0x01},
    {.key = 8,  .tag_id = OAD_CURRENT_C,            .scaler = 0.01f,   .length = 2, .access = 0x01},
    {.key = 12, .tag_id = OAD_ACTIVE_POWER_TOTAL,   .scaler = 0.001f,  .length = 2, .access = 0x01},
    {.key = 13, .tag_id = OAD_ACTIVE_POWER_A,       .scaler = 0.001f,  .length = 2, .access = 0x01},
    {.key = 14, .tag_id = OAD_ACTIVE_POWER_B,       .scaler = 0.001f,  .length = 2, .access = 0x01},
    {.key = 15, .tag_id = OAD_ACTIVE_POWER_C,       .scaler = 0.001f,  .length = 2, .access = 0x01},
    {.key = 18, .tag_id = OAD_REACTIVE_POWER_TOTAL, .scaler = 0.001f,  .length = 2, .access = 0x01},
    {.key = 24, .tag_id = OAD_POWER_FACTOR_TOTAL,   .scaler = 0.001f,  .length = 1, .access = 0x01},
    {.key = 28, .tag_id = OAD_GRID_FREQUENCY,       .scaler = 0.01f,   .length = 1, .access = 0x01},
    {.key = 52, .tag_id = OAD_ACTIVE_ENERGY,        .scaler = 0.01f,   .length = 2, .access = 0x01},
    {.key = 56, .tag_id = OAD_REACTIVE_ENERGY,      .scaler = 0.01f,   .length = 2, .access = 0x01},
};
static const size_t s_modbus_points_count = sizeof(s_modbus_points) / sizeof(s_modbus_points[0]);

static const void *dcu_lookup_modbus_by_key(uint64_t key) {
    for (size_t i = 0; i < s_modbus_points_count; i++) {
        if (s_modbus_points[i].key == key) {
            return &s_modbus_points[i];
        }
    }
    return NULL;
}

static const void *dcu_lookup_modbus_by_tag(uint32_t tag_id) {
    for (size_t i = 0; i < s_modbus_points_count; i++) {
        if (s_modbus_points[i].tag_id == tag_id) {
            return &s_modbus_points[i];
        }
    }
    return NULL;
}

static const driver_contract_injection_t s_modbus_injection = {
    .lookup_by_key = dcu_lookup_modbus_by_key,
    .lookup_by_tag = dcu_lookup_modbus_by_tag,
};

/* ==============================================================================
 * 5. DCU 驱动注入查找调度
 * ============================================================================== */
/* IO 层的弱符号默认返回 NULL（modules/edge_io/src/driver_manager.c:70）。弱定义在这里被顶掉，
 * 否则 edge_api_read_tag 在 edge_api.c:40 查不到契约直接返回 -1，总线一帧都发不出去。 */
const driver_contract_injection_t *edge_model_find(const char *name) {
    return dcu_get_driver_injection(name);
}


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
        for (size_t i = 0; i < sizeof(s_modbus_points) / sizeof(s_modbus_points[0]); i++)
            if (s_modbus_points[i].tag_id == oad) return s_modbus_points[i].key;
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
        for (size_t i = 0; i < sizeof(s_modbus_points) / sizeof(s_modbus_points[0]); i++)
            if (s_modbus_points[i].key == proto_id) return (uint32_t)s_modbus_points[i].tag_id;
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
        for (size_t i = 0; i < sizeof(s_modbus_points) / sizeof(s_modbus_points[0]); i++)
            if (s_modbus_points[i].key == proto_id) return (double)s_modbus_points[i].scaler;
        return 0.0;
    }
    return 0.0;
}

const driver_contract_injection_t *dcu_get_driver_injection(const char *proto) {
    if (!proto) return NULL;
    if (strcasecmp(proto, "DLT698") == 0) {
        return &s_dlt698_injection;
    } else if (strcasecmp(proto, "DLT645") == 0) {
        return &s_dlt645_injection;
    } else if (strcasecmp(proto, "ModbusRTU") == 0 || strcasecmp(proto, "Modbus") == 0) {
        return &s_modbus_injection;
    }
    return NULL;
}
