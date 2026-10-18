/**
 * @file dcu_driver_adapter.c
 * @brief DCU 专属南向驱动防腐适配层与静态点表注册实现
 */

#include "dcu_driver_adapter.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "contract_data_types.h"
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
    {.key = OAD_VOLTAGE, .tag_id = OAD_VOLTAGE, .scaler = 0.1f, .length = 2, .access = 0x01},
    {.key = OAD_CURRENT, .tag_id = OAD_CURRENT, .scaler = 0.01f, .length = 3, .access = 0x01},
    {.key = OAD_ACTIVE_ENERGY, .tag_id = OAD_ACTIVE_ENERGY, .scaler = 0.01f, .length = 4, .access = 0x01},
    {.key = OAD_VOLTAGE_B, .tag_id = OAD_VOLTAGE_B, .scaler = 0.1f, .length = 2, .access = 0x01},
    {.key = OAD_CURRENT_B, .tag_id = OAD_CURRENT_B, .scaler = 0.01f, .length = 3, .access = 0x01},
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
    {.key = 0x02010100, .tag_id = OAD_VOLTAGE, .scaler = 0.1f, .length = 2, .access = 0x01},
    {.key = 0x02020100, .tag_id = OAD_CURRENT, .scaler = 0.001f, .length = 3, .access = 0x01},
    {.key = 0x00010000, .tag_id = OAD_ACTIVE_ENERGY, .scaler = 0.01f, .length = 4, .access = 0x01},
    {.key = 0x02010200, .tag_id = OAD_VOLTAGE_B, .scaler = 0.1f, .length = 2, .access = 0x01},
    {.key = 0x02020200, .tag_id = OAD_CURRENT_B, .scaler = 0.001f, .length = 3, .access = 0x01},
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

static const driver_contract_injection_t s_dlt645_injection = {
    .lookup_by_key = dcu_lookup_645_by_key,
    .lookup_by_tag = dcu_lookup_645_by_tag,
};

/* ==============================================================================
 * 4. Modbus RTU 静态测点映射表
 * ============================================================================== */
static const modbus_map_t s_modbus_points[] = {
    {.key = 0, .tag_id = OAD_VOLTAGE, .scaler = 0.1f, .length = 1, .access = 0x01},
    {.key = 12, .tag_id = OAD_CURRENT, .scaler = 0.01f, .length = 2, .access = 0x01},
    {.key = 52, .tag_id = OAD_ACTIVE_ENERGY, .scaler = 0.01f, .length = 2, .access = 0x01},
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
