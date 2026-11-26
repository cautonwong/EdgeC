/**
 * @file iems_alarm_state.c
 * @brief P5-04a: pure state transitions for the real-time alarm table.
 */

#include "iems_alarm_state.h"

#include <string.h>

static int _find_index(const iems_alarm_table_t *table, uint32_t key, uint32_t code,
                       int32_t reason_code) {
    for (uint32_t i = 0; i < table->count; i++) {
        if (table->rows[i].key == key && table->rows[i].code == code &&
            table->rows[i].reason_code == reason_code) {
            return (int)i;
        }
    }
    return -1;
}

static iems_alarm_state_t *_append(iems_alarm_table_t *table, uint32_t key, uint32_t code,
                                   int32_t reason_code) {
    if (table->count >= IEMS_ALARM_STATE_MAX) return NULL;
    iems_alarm_state_t *row = &table->rows[table->count];
    memset(row, 0, sizeof(*row));
    row->key = key;
    row->code = code;
    row->reason_code = reason_code;
    table->count++;
    return row;
}

const iems_alarm_state_t *iems_alarm_state_find(const iems_alarm_table_t *table, uint32_t key,
                                                uint32_t code, int32_t reason_code) {
    if (!table) return NULL;
    int index = _find_index(table, key, code, reason_code);
    return index < 0 ? NULL : &table->rows[index];
}

int iems_alarm_state_raise(iems_alarm_table_t *table, uint32_t key, uint32_t code,
                           int32_t reason_code) {
    if (!table) return -1;
    int index = _find_index(table, key, code, reason_code);
    if (index >= 0) {
        iems_alarm_state_t *row = &table->rows[index];
        if (row->suppressed || row->active) return 1;
        row->active = true;
        row->acked = false;
        row->recovered_at = 0;
        return 0;
    }
    iems_alarm_state_t *row = _append(table, key, code, reason_code);
    if (!row) return -1;
    row->active = true;
    return 0;
}

int iems_alarm_state_ack(iems_alarm_table_t *table, uint32_t key, uint32_t code,
                         int32_t reason_code) {
    if (!table) return -1;
    int index = _find_index(table, key, code, reason_code);
    if (index < 0) return 1;
    iems_alarm_state_t *row = &table->rows[index];
    if (!row->active || row->acked) return 1;
    row->acked = true;
    return 0;
}

int iems_alarm_state_recover(iems_alarm_table_t *table, uint32_t key, uint32_t code,
                             int32_t reason_code, uint64_t now_ms) {
    if (!table) return -1;
    int index = _find_index(table, key, code, reason_code);
    if (index < 0) return 1;
    iems_alarm_state_t *row = &table->rows[index];
    if (!row->active) return 1;
    row->active = false;
    row->recovered_at = now_ms;
    return 0;
}

int iems_alarm_state_suppress(iems_alarm_table_t *table, uint32_t key, uint32_t code,
                              int32_t reason_code) {
    if (!table) return -1;
    int index = _find_index(table, key, code, reason_code);
    if (index >= 0) {
        iems_alarm_state_t *row = &table->rows[index];
        if (row->suppressed) return 1;
        row->suppressed = true;
        return 0;
    }
    iems_alarm_state_t *row = _append(table, key, code, reason_code);
    if (!row) return -1;
    row->suppressed = true;
    return 0;
}

int iems_alarm_state_resume(iems_alarm_table_t *table, uint32_t key, uint32_t code,
                            int32_t reason_code) {
    if (!table) return -1;
    int index = _find_index(table, key, code, reason_code);
    if (index < 0) return 1;
    iems_alarm_state_t *row = &table->rows[index];
    if (!row->suppressed) return 1;
    row->suppressed = false;
    return 0;
}

int iems_alarm_state_raise_mapped(iems_alarm_table_t *table, const iems_alarm_tag_map_t *map,
                                  const char *class_type, const char *vendor, const char *model,
                                  const char *tag, uint32_t code, int32_t reason_code) {
    if (!table || !map || !map->map_tag) return -1;
    if (!class_type || !vendor || !model || !tag) return -1;
    if (class_type[0] == '\0' || vendor[0] == '\0' || model[0] == '\0' || tag[0] == '\0') {
        return -1;
    }
    uint32_t edgec_tag = 0;
    if (map->map_tag(map->ctx, class_type, vendor, model, tag, &edgec_tag) != 0) return -1;
    if (edgec_tag == 0) return -1; /* 0 is the unmapped sentinel in iems_point_model.h */
    return iems_alarm_state_raise(table, edgec_tag, code, reason_code);
}
