/**
 * @file iems_driver_adapter.c
 * @brief IEMS 专属南向工业协议驱动防腐适配层与静态点表注册实现
 */

#include "iems_driver_adapter.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "contract_data_types.h"
#include "modbus_codec.h"

/* 外部协议驱动模板声明 */
extern edge_driver_t modbus_rtu_driver;
extern edge_driver_t modbus_tcp_driver;
extern edge_driver_t iec104_driver;
extern edge_driver_t can_driver;
extern const edge_driver_t dlt645_driver;

/* ==============================================================================
 * 1. IEMS 专属静态驱动注册表 (替代生成的 edge_driver_registry.c)
 * ============================================================================== */
static const edge_driver_reg_entry_t s_iems_driver_registry[] = {
    EDGE_DRIVER_ENTRY("ModbusRTU", modbus_rtu_driver),
    EDGE_DRIVER_ENTRY("ModbusTCP", modbus_tcp_driver),
    EDGE_DRIVER_ENTRY("IEC104", iec104_driver),
    EDGE_DRIVER_ENTRY("CAN", can_driver),
    EDGE_DRIVER_ENTRY("DLT645", dlt645_driver),
};

static const size_t s_iems_driver_registry_count =
    sizeof(s_iems_driver_registry) / sizeof(s_iems_driver_registry[0]);

int iems_driver_registry_init(driver_manager_t *mgr) {
    if (!mgr) return -1;
    return driver_manager_register_table(mgr, s_iems_driver_registry, s_iems_driver_registry_count);
}

/* ==============================================================================
 * 2. Modbus RTU 静态测点映射表
 * ============================================================================== */
static const modbus_map_t s_modbus_points[] = {
    {.key = 0,   .tag_id = IEMS_TAG_VOLTAGE_PHASE_A,   .dense_idx = 0,  .scaler = 0.1f,  .length = 1, .access = 0x01},
    {.key = 12,  .tag_id = IEMS_TAG_GRID_POWER,        .dense_idx = 2,  .scaler = 1.0f,  .length = 2, .access = 0x01},
    {.key = 15,  .tag_id = IEMS_TAG_BATTERY_POWER,     .dense_idx = 9,  .scaler = 0.01f, .length = 1, .access = 0x01},
    {.key = 52,  .tag_id = IEMS_TAG_GRID_FREQUENCY,    .dense_idx = 3,  .scaler = 0.01f, .length = 1, .access = 0x01},
    {.key = 255, .tag_id = IEMS_TAG_SOLAR_POWER,       .dense_idx = 10, .scaler = 1.0f,  .length = 1, .access = 0x01},
    {.key = 256, .tag_id = IEMS_TAG_SOLAR_VOLTS,       .dense_idx = 11, .scaler = 0.1f,  .length = 1, .access = 0x01},
    {.key = 258, .tag_id = IEMS_TAG_SOLAR_RELAY,       .dense_idx = 12, .scaler = 1.0f,  .length = 1, .access = 0x01},
    {.key = 261, .tag_id = IEMS_TAG_BATTERY_SOC,       .dense_idx = 7,  .scaler = 1.0f,  .length = 1, .access = 0x01},
};
static const size_t s_modbus_points_count = sizeof(s_modbus_points) / sizeof(s_modbus_points[0]);

const void *iems_lookup_modbus_by_key(uint64_t key) {
    for (size_t i = 0; i < s_modbus_points_count; i++) {
        if (s_modbus_points[i].key == key) {
            return &s_modbus_points[i];
        }
    }
    return NULL;
}

const void *iems_lookup_modbus_by_tag(uint32_t tag_id) {
    for (size_t i = 0; i < s_modbus_points_count; i++) {
        if (s_modbus_points[i].tag_id == tag_id) {
            return &s_modbus_points[i];
        }
    }
    return NULL;
}

static const driver_contract_injection_t s_modbus_injection = {
    .lookup_by_key = iems_lookup_modbus_by_key,
    .lookup_by_tag = iems_lookup_modbus_by_tag,
};

/* 兼容外部全栈测试符号 */
const modbus_map_t *edge_get_map_modbus(uint64_t key) {
    return (const modbus_map_t *)iems_lookup_modbus_by_key(key);
}
const modbus_map_t *edge_get_map_modbus_by_tag(edge_tag_id_t tag_id) {
    return (const modbus_map_t *)iems_lookup_modbus_by_tag((uint32_t)tag_id);
}

/* ==============================================================================
 * 3. IEC 104 静态测点映射表
 * ============================================================================== */
static const iec104_map_t s_iec104_points[] = {
    {.key = 16385, .tag_id = IEMS_TAG_GRID_POWER,        .dense_idx = 2, .scaler = 1.0f, .access = 0x01},
    {.key = 16386, .tag_id = IEMS_TAG_VOLTAGE_PHASE_A,   .dense_idx = 0, .scaler = 1.0f, .access = 0x01},
    {.key = 16387, .tag_id = IEMS_TAG_CURRENT_PHASE_A,   .dense_idx = 1, .scaler = 1.0f, .access = 0x01},
    {.key = 16388, .tag_id = IEMS_TAG_BATTERY_POWER,     .dense_idx = 9, .scaler = 1.0f, .access = 0x01},
};
static const size_t s_iec104_points_count = sizeof(s_iec104_points) / sizeof(s_iec104_points[0]);

const void *iems_lookup_iec104_by_key(uint64_t key) {
    for (size_t i = 0; i < s_iec104_points_count; i++) {
        if (s_iec104_points[i].key == key) {
            return &s_iec104_points[i];
        }
    }
    return NULL;
}

const void *iems_lookup_iec104_by_tag(uint32_t tag_id) {
    for (size_t i = 0; i < s_iec104_points_count; i++) {
        if (s_iec104_points[i].tag_id == tag_id) {
            return &s_iec104_points[i];
        }
    }
    return NULL;
}

static const driver_contract_injection_t s_iec104_injection = {
    .lookup_by_key = iems_lookup_iec104_by_key,
    .lookup_by_tag = iems_lookup_iec104_by_tag,
};

/* 兼容外部全栈测试符号 */
const iec104_map_t *edge_get_map_iec104(uint64_t key) {
    return (const iec104_map_t *)iems_lookup_iec104_by_key(key);
}
const iec104_map_t *edge_get_map_iec104_by_tag(edge_tag_id_t tag_id) {
    return (const iec104_map_t *)iems_lookup_iec104_by_tag((uint32_t)tag_id);
}

/* ==============================================================================
 * 4. CAN 静态测点映射表 (CATL BMS)
 * ============================================================================== */
static const modbus_map_t s_can_points[] = {
    {.key = 0x18FF50E5, .tag_id = IEMS_TAG_BATTERY_SOC,     .dense_idx = 7, .scaler = 0.1f, .length = 1, .access = 0x01},
    {.key = 0x18FF50E6, .tag_id = IEMS_TAG_BATTERY_CURRENT, .dense_idx = 8, .scaler = 0.1f, .length = 2, .access = 0x01},
};
static const size_t s_can_points_count = sizeof(s_can_points) / sizeof(s_can_points[0]);

static const void *iems_lookup_can_by_key(uint64_t key) {
    for (size_t i = 0; i < s_can_points_count; i++) {
        if (s_can_points[i].key == key) {
            return &s_can_points[i];
        }
    }
    return NULL;
}

static const void *iems_lookup_can_by_tag(uint32_t tag_id) {
    for (size_t i = 0; i < s_can_points_count; i++) {
        if (s_can_points[i].tag_id == tag_id) {
            return &s_can_points[i];
        }
    }
    return NULL;
}

static const driver_contract_injection_t s_can_injection = {
    .lookup_by_key = iems_lookup_can_by_key,
    .lookup_by_tag = iems_lookup_can_by_tag,
};

/* [ADR-002] 产品的点位标识 → 驱动认识的原生标识。
 * 645 的 DI 已由 dcu_driver_adapter_oad_to_proto 换好；CAN 的 CAN ID 在这里换。
 * 翻译必须在产品侧完成——驱动拿不到点位表，也不该拿到。 */
uint32_t iems_driver_adapter_native_id(const char *proto, uint32_t tag_id) {
    if (!proto) return tag_id;
    if (strcasecmp(proto, "CAN") == 0) {
        const modbus_map_t *m = (const modbus_map_t *)iems_lookup_can_by_tag(tag_id);
        if (m) return (uint32_t)m->key;
    }
    /* ModbusTCP 已 request-native：点位 = 寄存器地址 + 寄存器个数。
     * ModbusRTU 仍是 legacy（驱动自己查表），这里保持等值不动。 */
    if (strcasecmp(proto, "ModbusTCP") == 0) {
        const modbus_map_t *m = (const modbus_map_t *)iems_lookup_modbus_by_tag(tag_id);
        if (m) return MODBUS_NATIVE((uint32_t)m->key, m->length ? m->length : 1);
    }
    return tag_id;
}

/* [ADR-002] 量纲：request-native 驱动不再自己乘 scaler，改由调用方给。
 * legacy 驱动仍在驱动内部标定，这里返回 1.0，避免双重相乘。 */
double iems_driver_adapter_tag_scale(const char *proto, uint32_t tag_id) {
    if (!proto) return 1.0;
    if (strcasecmp(proto, "CAN") == 0) {
        const modbus_map_t *m = (const modbus_map_t *)iems_lookup_can_by_tag(tag_id);
        if (m && m->scaler > 0.0) return m->scaler;   /* CAN：工程值 = raw × scaler */
    }
    if (strcasecmp(proto, "ModbusTCP") == 0) {
        const modbus_map_t *m = (const modbus_map_t *)iems_lookup_modbus_by_tag(tag_id);
        /* Modbus：工程值 = raw / scaler，所以这里给倒数 */
        if (m && m->scaler > 0.0) return 1.0 / m->scaler;
    }
    return 1.0;
}

/* ==============================================================================
 * 5. DL/T 645-2007 静态测点映射表
 * ============================================================================== */
static const dlt645_map_t s_dlt645_points[] = {
    {.key = 0x02010100, .tag_id = IEMS_TAG_VOLTAGE_PHASE_A,   .dense_idx = 0, .scaler = 0.1f,   .length = 2, .access = 0x01},
    {.key = 0x02020100, .tag_id = IEMS_TAG_CURRENT_PHASE_A,   .dense_idx = 1, .scaler = 0.001f, .length = 3, .access = 0x01},
    {.key = 0x02030000, .tag_id = IEMS_TAG_GRID_POWER,        .dense_idx = 2, .scaler = 0.0001f,.length = 3, .access = 0x01},
    {.key = 0x00010000, .tag_id = IEMS_TAG_ENERGY_ACTIVE_IMP, .dense_idx = 6, .scaler = 0.01f,  .length = 4, .access = 0x01},
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
 * 6. IEMS 统一驱动注入调度
 * ============================================================================== */
const driver_contract_injection_t *iems_get_driver_injection(const char *proto) {
    if (!proto) return NULL;
    if (strcasecmp(proto, "ModbusRTU") == 0 || strcasecmp(proto, "Modbus") == 0 || strcasecmp(proto, "ModbusTCP") == 0) {
        return &s_modbus_injection;
    } else if (strcasecmp(proto, "IEC104") == 0) {
        return &s_iec104_injection;
    } else if (strcasecmp(proto, "CAN") == 0) {
        return &s_can_injection;
    } else if (strcasecmp(proto, "DLT645") == 0) {
        return &s_dlt645_injection;
    }
    return NULL;
}

/* ==============================================================================
 * 7. IEMS 本地测点到 DataHub 影子槽位与轮询周期 (OCP 表驱动映射)
 * ============================================================================== */
typedef struct {
    uint32_t tag_id;
    uint32_t offset;
    uint32_t poll_interval_ms;
} iems_tag_slot_map_t;

static const iems_tag_slot_map_t s_iems_tag_slots[] = {
    {IEMS_TAG_VOLTAGE_PHASE_A,   0,  1000},
    {IEMS_TAG_CURRENT_PHASE_A,   1,  1000},
    {IEMS_TAG_GRID_POWER,        2,  1000},
    {IEMS_TAG_GRID_FREQUENCY,    3,  1000},
    {IEMS_TAG_ADC_RAW_DATA,      4,  10},
    {IEMS_TAG_ADC_VOLTAGE_RMS,   5,  100},
    {IEMS_TAG_ENERGY_ACTIVE_IMP, 6,  10000},
    {IEMS_TAG_BATTERY_SOC,       7,  1000},
    {IEMS_TAG_BATTERY_CURRENT,   8,  1000},
    {IEMS_TAG_BATTERY_POWER,     9,  1000},
    {IEMS_TAG_SOLAR_POWER,       10, 1000},
    {IEMS_TAG_SOLAR_VOLTS,       11, 1000},
    {IEMS_TAG_SOLAR_RELAY,       12, 1000},
    {IEMS_TAG_PCS_CONTROL,       13, 1000},
};
static const size_t s_iems_tag_slots_count = sizeof(s_iems_tag_slots) / sizeof(s_iems_tag_slots[0]);

int iems_tag_map_fn(uint32_t tag_id, uint32_t device_id) {
    for (size_t i = 0; i < s_iems_tag_slots_count; i++) {
        if (s_iems_tag_slots[i].tag_id == tag_id) {
            return (int)(device_id * 16 + s_iems_tag_slots[i].offset);
        }
    }
    if (tag_id < 64) return (int)tag_id;
    return -1;
}

uint32_t iems_get_tag_poll_interval_ms(uint32_t tag_id) {
    for (size_t i = 0; i < s_iems_tag_slots_count; i++) {
        if (s_iems_tag_slots[i].tag_id == tag_id) {
            return s_iems_tag_slots[i].poll_interval_ms;
        }
    }
    return 1000;
}

/* ==============================================================================
 * 9. IEMS 物理总线通道静态配置表
 * ============================================================================== */
static const edge_connectivity_t s_iems_connectivity[] = {
    {
        .id = 0,
        .label = "RS485_Main",
        .category = "SOUTHBOUND",
        .type = "UART",
        .simulation_path = "UDS:/tmp/vbus0.sock",
        .production_path = "/dev/ttyS1",
    },
    {
        .id = 1,
        .label = "CAN_Main",
        .category = "SOUTHBOUND",
        .type = "CAN",
        .simulation_path = "UDS:/tmp/vbus1.sock",
        .production_path = "can0",
    },
    {
        .id = 10,
        .label = "Northbound_SCADA",
        .category = "NORTHBOUND",
        .type = "UART",
        .simulation_path = "/tmp/iems_north_slave.sock",
        .production_path = "/dev/ttyS2",
    },
    {
        .id = 200,
        .label = "HighSpeed_ADC",
        .category = "INTERNAL",
        .type = "MEM_RING",
        .simulation_path = "/tmp/v_adc.sock",
        .production_path = "adc_raw",
    },
};

const edge_connectivity_t *g_connectivity_configs = s_iems_connectivity;
const int g_connectivity_configs_len = (int)(sizeof(s_iems_connectivity) / sizeof(s_iems_connectivity[0]));

const edge_connectivity_t *iems_get_connectivity(uint32_t *count) {
    if (count) *count = (uint32_t)g_connectivity_configs_len;
    return s_iems_connectivity;
}

const char *edge_resolve_bus_path(int bus_id) {
    char env_name[64];
    snprintf(env_name, sizeof(env_name), "OMNIS_BUS_%d_DEV", bus_id);
    const char *env_val = getenv(env_name);
    if (env_val) return env_val;
    for (int i = 0; i < g_connectivity_configs_len; i++) {
        if (g_connectivity_configs[i].id == bus_id) {
            const char *sp = g_connectivity_configs[i].simulation_path;
            if (sp && sp[0] != '\0') {
                if (strncmp(sp, "UDS:", 4) == 0) return sp + 4;
                return sp;
            }
            return g_connectivity_configs[i].production_path;
        }
    }
    return NULL;
}

/* ==============================================================================
 * 10. IEMS 数据流处理管道静态配置表
 * ============================================================================== */
static const edge_stage_def_t _stages_energy_manager[] = {
    { "integrator", "energy_integrator", { 0, 0, 0, 3001, "", "", "", 0, 0 }, { 1, -1, -1, -1 } },
    { "sink", "sqlite_sink", { 0, 0, 0, 0, "energy_history", "", "", 0, 0 }, { -1, -1, -1, -1 } },
};

static const edge_stage_def_t _stages_overvoltage_protection[] = {
    { "reflex", "reflex", { 264.0, 1.0, 0.0, 8004, "ModbusRTU", "", "", 0, 0 }, { -1, -1, -1, -1 } },
};

static const edge_stage_def_t _stages_dsp_processor[] = {
    { "dsp_rms", "dsp_rms", { 0, 0, 0, 2002, "", "", "", 0, 0 }, { -1, -1, -1, -1 } },
};

static const edge_stage_def_t _stages_island_protection[] = {
    { "mpc_dispatch", "mpc_dispatch", { 1.0, 100.0, 50.0, 4001, "", "", "", 0, 8005 }, { -1, -1, -1, -1 } },
};

static const edge_pipeline_def_t s_iems_pipelines[] = {
    { "energy_manager", 1003, NULL, 0, _stages_energy_manager, 2 },
    { "overvoltage_protection", 2002, NULL, 0, _stages_overvoltage_protection, 1 },
    { "dsp_processor", 2001, NULL, 0, _stages_dsp_processor, 1 },
    { "island_protection", 4001, NULL, 0, _stages_island_protection, 1 },
};

const edge_pipeline_def_t *g_pipeline_metadata = s_iems_pipelines;
const int g_pipeline_metadata_len = (int)(sizeof(s_iems_pipelines) / sizeof(s_iems_pipelines[0]));

const edge_pipeline_def_t *iems_get_pipelines(uint32_t *count) {
    if (count) *count = (uint32_t)g_pipeline_metadata_len;
    return s_iems_pipelines;
}

/* ==============================================================================
 * 11. IEMS 测点静态元数据表
 * ============================================================================== */
static const edge_tag_meta_t s_iems_tag_metadata[] = {
    { .id = 1001, .name = "VOLTAGE_PHASE_A",   .type = "FLOAT",  .unit = "V",   .category = "AC",       .min = 0,    .max = 500,     .persist = false, .upload = true,  .poll_interval_ms = 1000 },
    { .id = 1002, .name = "CURRENT_PHASE_A",   .type = "FLOAT",  .unit = "A",   .category = "AC",       .min = 0,    .max = 100,     .persist = false, .upload = true,  .poll_interval_ms = 1000 },
    { .id = 1003, .name = "GRID_POWER",        .type = "FLOAT",  .unit = "W",   .category = "AC",       .min = 0,    .max = 50000,   .persist = false, .upload = true,  .poll_interval_ms = 1000 },
    { .id = 1004, .name = "GRID_FREQUENCY",    .type = "FLOAT",  .unit = "Hz",  .category = "AC",       .min = 45,   .max = 65,      .persist = false, .upload = true,  .poll_interval_ms = 1000 },
    { .id = 2001, .name = "ADC_RAW_DATA",      .type = "FLOAT",  .unit = "RAW", .category = "FAST_ADC", .min = 0,    .max = 4096,    .persist = false, .upload = false, .poll_interval_ms = 10 },
    { .id = 2002, .name = "ADC_VOLTAGE_RMS",   .type = "FLOAT",  .unit = "V",   .category = "FAST_ADC", .min = 0,    .max = 500,     .persist = false, .upload = true,  .poll_interval_ms = 100 },
    { .id = 3001, .name = "ENERGY_ACTIVE_IMP", .type = "DOUBLE", .unit = "kWh", .category = "METER",    .min = 0,    .max = 1000000, .persist = true,  .upload = true,  .poll_interval_ms = 10000 },
    { .id = 4001, .name = "BATTERY_SOC",       .type = "FLOAT",  .unit = "%",   .category = "BATTERY",  .min = 0,    .max = 100,     .persist = true,  .upload = true,  .poll_interval_ms = 1000 },
    { .id = 4002, .name = "BATTERY_CURRENT",   .type = "FLOAT",  .unit = "A",   .category = "BATTERY",  .min = -500, .max = 500,     .persist = false, .upload = true,  .poll_interval_ms = 1000 },
    { .id = 4003, .name = "BATTERY_POWER",     .type = "FLOAT",  .unit = "kW",  .category = "BATTERY",  .min = -500, .max = 500,     .persist = false, .upload = true,  .poll_interval_ms = 1000 },
    { .id = 8001, .name = "SOLAR_POWER",       .type = "FLOAT",  .unit = "W",   .category = "SOLAR",    .min = 0,    .max = 50000,   .persist = false, .upload = true,  .poll_interval_ms = 1000 },
    { .id = 8003, .name = "SOLAR_VOLTS",       .type = "FLOAT",  .unit = "V",   .category = "SOLAR",    .min = 0,    .max = 1000,    .persist = false, .upload = true,  .poll_interval_ms = 1000 },
    { .id = 8004, .name = "SOLAR_RELAY",       .type = "INT32",  .unit = "",    .category = "SOLAR",    .min = 0,    .max = 1,       .persist = false, .upload = true,  .poll_interval_ms = 1000 },
    { .id = 8005, .name = "PCS_CONTROL",       .type = "FLOAT",  .unit = "kW",  .category = "PCS",      .min = -500, .max = 500,     .persist = false, .upload = true,  .poll_interval_ms = 1000 },
};

const edge_tag_meta_t *g_tag_metadata = s_iems_tag_metadata;
const size_t g_tag_metadata_len = sizeof(s_iems_tag_metadata) / sizeof(s_iems_tag_metadata[0]);

const edge_tag_meta_t *iems_get_tag_metadata(uint32_t *count) {
    if (count) *count = (uint32_t)g_tag_metadata_len;
    return s_iems_tag_metadata;
}

const edge_tag_meta_t *iems_get_tag_meta(uint32_t tag_id) {
    for (size_t i = 0; i < g_tag_metadata_len; i++) {
        if (s_iems_tag_metadata[i].id == tag_id) {
            return &s_iems_tag_metadata[i];
        }
    }
    return NULL;
}

const edge_tag_meta_t *contract_tag_get_meta(edge_tag_id_t tag_id) {
    return iems_get_tag_meta((uint32_t)tag_id);
}

int edge_tag_get_compact_index(uint32_t tag_id) {
    return iems_tag_map_fn(tag_id, 0);
}

const edge_driver_reg_entry_t *g_edge_driver_registry = s_iems_driver_registry;
const size_t g_edge_driver_registry_count = sizeof(s_iems_driver_registry) / sizeof(s_iems_driver_registry[0]);

int edge_driver_registry_init(driver_manager_t *mgr) {
    return iems_driver_registry_init(mgr);
}

edge_tag_id_t edge_lookup_modbus(uint64_t addr, float *scale) {
    const modbus_map_t *p = edge_get_map_modbus(addr);
    if (p) {
        if (scale) *scale = p->scaler;
        return (edge_tag_id_t)p->tag_id;
    }
    return 0;
}

edge_tag_id_t edge_lookup_dlt645(uint64_t di, float *scale) {
    for (size_t i = 0; i < s_dlt645_points_count; i++) {
        if (s_dlt645_points[i].key == di) {
            if (scale) *scale = s_dlt645_points[i].scaler;
            return (edge_tag_id_t)s_dlt645_points[i].tag_id;
        }
    }
    return 0;
}

int edge_contract_self_test(void) {
    return 0;
}

const driver_contract_injection_t *edge_model_find(const char *name) {
    return iems_get_driver_injection(name);
}

