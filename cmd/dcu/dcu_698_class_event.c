#include "dcu_698_object_internal.h"

#include <stdio.h>
#include <string.h>

int dcu_698_event_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                       void *out_val) {
    (void)elem_idx;
    dcu_event_obj_t *evt = (dcu_event_obj_t *)obj->instance;
    if (!evt || !out_val) return -1;

    // 7.3.24 表193：A4 最大记录数、A11 上报标识、A12 有效标识。
    if (attr_id == 4) {
        *(uint16_t *)out_val = MAX_EVENT_RECORDS;
        return 0;
    }
    if (attr_id == 11) {
        *(uint8_t *)out_val = evt->report_flag;
        return 0;
    }
    if (attr_id == 12) {
        *(bool *)out_val = evt->is_enabled;
        return 0;
    }
    return -2;
}

int dcu_698_event_write(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                        const void *in_val) {
    (void)elem_idx;
    dcu_event_obj_t *evt = (dcu_event_obj_t *)obj->instance;
    if (!evt || !in_val) return -1;

    if (attr_id == 11) {
        uint8_t v = *(const uint8_t *)in_val;
        if (v > 3) return -2;
        evt->report_flag = v;
        return 0;
    }
    if (attr_id == 12) {
        evt->is_enabled = *(const bool *)in_val;
        return 0;
    }
    return -2;
}

int dcu_698_event_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                          void *result) {
    (void)param;
    dcu_event_obj_t *evt = (dcu_event_obj_t *)obj->instance;
    if (!evt) return -1;

    if (method_id == 1) {
        evt->record_count = 0;
        memset(evt->records, 0, sizeof(evt->records));
        evt->last_start_time = 0;
        evt->last_end_time = 0;
        if (result) *(int *)result = 0;
        printf("🧹 [EVENT] ACTION: 成功复位事件 %04X 的历史事件记录表。\n", evt->oi);
        return 0;
    }
    return -2;
}
