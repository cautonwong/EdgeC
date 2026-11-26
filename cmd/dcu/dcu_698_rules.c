#include "dcu_698_rules.h"

#include <stddef.h>
#include <string.h>
#include <strings.h>

#include "contract_data_types.h"
#include "dcu_router.h"
#include "dcu_rule_store.h"

static const dcu_rule_row_t k_seed[] = {
    {OAD_VOLTAGE, 0x02010100, 0.1f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 2},
    {TAG_VOLTAGE_B, 0x02010200, 0.1f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 2},
    {TAG_VOLTAGE_C, 0x02010300, 0.1f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 2},
    {OAD_CURRENT, 0x02020100, 0.001f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 3},
    {TAG_CURRENT_B, 0x02020200, 0.001f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 3},
    {TAG_CURRENT_C, 0x02020300, 0.001f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 3},
    {OAD_ACTIVE_ENERGY, 0x00010000, 0.01f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 4},
    {OAD_REACTIVE_ENERGY, 0x00020000, 0.01f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 4},
    {OAD_ACTIVE_POWER_TOTAL, 0x02030000, 0.0001f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 3},
    {TAG_ACTIVE_POWER_A, 0x02030100, 0.0001f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 3},
    {TAG_ACTIVE_POWER_B, 0x02030200, 0.0001f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 3},
    {TAG_ACTIVE_POWER_C, 0x02030300, 0.0001f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 3},
    {OAD_REACTIVE_POWER_TOTAL, 0x02040000, 0.0001f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 3},
    {OAD_POWER_FACTOR_TOTAL, 0x02060000, 0.001f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 2},
    {OAD_GRID_FREQUENCY, 0x02800002, 0.01f, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, 2},
    {OAD_VOLTAGE, OAD_VOLTAGE, 0.1f, DCU_RULE_DLT698, DCU_RULE_MAIN, 2},
    {TAG_VOLTAGE_B, OAD_VOLTAGE_B_ELEM, 0.1f, DCU_RULE_DLT698, DCU_RULE_MAIN, 2},
    {TAG_VOLTAGE_C, OAD_VOLTAGE_C_ELEM, 0.1f, DCU_RULE_DLT698, DCU_RULE_MAIN, 2},
    {OAD_CURRENT, OAD_CURRENT, 0.01f, DCU_RULE_DLT698, DCU_RULE_MAIN, 3},
    {TAG_CURRENT_B, OAD_CURRENT_B_ELEM, 0.01f, DCU_RULE_DLT698, DCU_RULE_MAIN, 3},
    {TAG_CURRENT_C, OAD_CURRENT_C_ELEM, 0.01f, DCU_RULE_DLT698, DCU_RULE_MAIN, 3},
    {OAD_ACTIVE_ENERGY, OAD_ACTIVE_ENERGY, 0.01f, DCU_RULE_DLT698, DCU_RULE_MAIN, 4},
    {OAD_REACTIVE_ENERGY, OAD_REACTIVE_ENERGY, 0.01f, DCU_RULE_DLT698, DCU_RULE_MAIN, 4},
    {OAD_ACTIVE_POWER_TOTAL, OAD_ACTIVE_POWER_TOTAL, 0.0001f, DCU_RULE_DLT698, DCU_RULE_MAIN, 3},
    {TAG_ACTIVE_POWER_A, OAD_ACTIVE_POWER_A_ELEM, 0.0001f, DCU_RULE_DLT698, DCU_RULE_MAIN, 3},
    {TAG_ACTIVE_POWER_B, OAD_ACTIVE_POWER_B_ELEM, 0.0001f, DCU_RULE_DLT698, DCU_RULE_MAIN, 3},
    {TAG_ACTIVE_POWER_C, OAD_ACTIVE_POWER_C_ELEM, 0.0001f, DCU_RULE_DLT698, DCU_RULE_MAIN, 3},
    {OAD_REACTIVE_POWER_TOTAL, OAD_REACTIVE_POWER_TOTAL, 0.0001f, DCU_RULE_DLT698, DCU_RULE_MAIN, 3},
    {OAD_POWER_FACTOR_TOTAL, OAD_POWER_FACTOR_TOTAL, 0.001f, DCU_RULE_DLT698, DCU_RULE_MAIN, 2},
    {OAD_GRID_FREQUENCY, OAD_GRID_FREQUENCY, 0.01f, DCU_RULE_DLT698, DCU_RULE_MAIN, 2},
    {OAD_ZERO_SEQ_CURRENT, OAD_ZERO_SEQ_CURRENT, 0.01f, DCU_RULE_DLT698, DCU_RULE_MAIN, 3},
    {OAD_VOLTAGE, 0, 0.1f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 1},
    {TAG_VOLTAGE_B, 1, 0.1f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 1},
    {TAG_VOLTAGE_C, 2, 0.1f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 1},
    {OAD_CURRENT, 6, 0.01f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 2},
    {TAG_CURRENT_B, 7, 0.01f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 2},
    {TAG_CURRENT_C, 8, 0.01f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 2},
    {OAD_ACTIVE_POWER_TOTAL, 12, 0.001f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 2},
    {TAG_ACTIVE_POWER_A, 13, 0.001f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 2},
    {TAG_ACTIVE_POWER_B, 14, 0.001f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 2},
    {TAG_ACTIVE_POWER_C, 15, 0.001f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 2},
    {OAD_REACTIVE_POWER_TOTAL, 18, 0.001f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 2},
    {OAD_POWER_FACTOR_TOTAL, 24, 0.001f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 1},
    {OAD_GRID_FREQUENCY, 28, 0.01f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 1},
    {OAD_ACTIVE_ENERGY, 52, 0.01f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 2},
    {OAD_REACTIVE_ENERGY, 56, 0.01f, DCU_RULE_MODBUS, DCU_RULE_MAIN, 2},
};

#define SEED_COUNT ((uint16_t)(sizeof(k_seed) / sizeof(k_seed[0])))

static dcu_rule_cache_t *s_cache;
static dcu_storage_t *s_storage;
static dcu_rule_op_t s_rule_op;

static const dcu_rule_row_t *_rows(uint16_t *count) {
    if (s_cache) {
        *count = s_cache->count;
        return s_cache->rows;
    }
    *count = SEED_COUNT;
    return k_seed;
}

int dcu_698_rules_init(dcu_storage_t *storage, dcu_rule_cache_t *cache) {
    if (!storage || !storage->db || !cache) return -1;
    if (dcu_rule_store_init(storage, k_seed, SEED_COUNT) != 0) return -1;
    if (dcu_rule_store_load(storage, cache) != 0) return -1;
    s_cache = cache;
    s_storage = storage;
    return 0;
}

void dcu_698_rules_detach(void) {
    s_cache = NULL;
    s_storage = NULL;
}

dcu_rule_op_t *dcu_698_rules_op(void) {
    return &s_rule_op;
}

int dcu_698_rules_apply(const dcu_rule_op_t *op) {
    if (!s_storage || !s_storage->db || !s_cache) return EDGE_SLAVE_SRC_TEMPORARY;
    int rc = dcu_rule_store_apply(s_storage, k_seed, SEED_COUNT, op);
    if (rc != 0) return rc;
    dcu_rule_cache_t reloaded;
    if (dcu_rule_store_load(s_storage, &reloaded) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
    *s_cache = reloaded;
    return 0;
}

int dcu_698_rules_projected_count(uint16_t *out) {
    if (!s_storage || !s_storage->db) return -1;
    return dcu_rule_store_projected_count(s_storage, out) == 0 ? 0 : -1;
}

int dcu_698_rules_projected_oad(uint16_t pos, uint32_t *out) {
    if (!s_storage || !s_storage->db) return -1;
    return dcu_rule_store_projected_oad(s_storage, pos, out);
}

int dcu_698_rules_unit_load(uint32_t oad, dcu_rule_unit_t *out) {
    if (!s_storage || !s_storage->db) return -1;
    return dcu_rule_store_unit_load(s_storage, oad, out);
}

int dcu_698_rules_find_by_oad(uint8_t protocol, uint32_t oad, dcu_rule_row_t *out) {
    if (!out) return -1;
    uint16_t count = 0;
    const dcu_rule_row_t *rows = _rows(&count);
    for (int pass = 0; pass < 2; pass++) {
        for (uint16_t i = 0; i < count; i++) {
            const dcu_rule_row_t *row = &rows[i];
            if (row->protocol != protocol || row->oad != oad) continue;
            if (pass == 0 && row->role != DCU_RULE_MAIN) continue;
            *out = *row;
            return 0;
        }
    }
    return -1;
}

int dcu_698_rules_find_by_proto(uint8_t protocol, uint32_t protocol_id, dcu_rule_row_t *out) {
    if (!out) return -1;
    uint16_t count = 0;
    const dcu_rule_row_t *rows = _rows(&count);
    for (uint16_t i = 0; i < count; i++) {
        const dcu_rule_row_t *row = &rows[i];
        if (row->protocol != protocol || row->protocol_id != protocol_id) continue;
        *out = *row;
        return 0;
    }
    return -1;
}

int dcu_698_rules_retry_with_spare(const char *protocol, uint32_t oad, int already_retried,
                                   uint32_t *spare_protocol_id) {
    if (!protocol || !spare_protocol_id || already_retried) return 0;
    if (strcasecmp(protocol, "DLT645") != 0) return 0;
    uint16_t count = 0;
    const dcu_rule_row_t *rows = _rows(&count);
    for (uint16_t i = 0; i < count; i++) {
        const dcu_rule_row_t *row = &rows[i];
        if (row->protocol != DCU_RULE_DLT645_2007 || row->role != DCU_RULE_SPARE) continue;
        if (row->oad != oad) continue;
        *spare_protocol_id = row->protocol_id;
        return 1;
    }
    return 0;
}
