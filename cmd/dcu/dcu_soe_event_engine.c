/**
 * @file dcu_soe_event_engine.c
 * @brief Production SGCC/CSG DL/T 698.45 3xxx Event Alarms & SOE Engine Implementation
 */

#include "dcu_soe_event_engine.h"

#include <string.h>

void dcu_soe_init(dcu_soe_event_engine_t* engine) {
    if (!engine) return;
    memset(engine, 0, sizeof(dcu_soe_event_engine_t));
}

int dcu_soe_record_event(dcu_soe_event_engine_t* engine, uint16_t oad_code, const char* addr,
                         uint32_t dev_id, dcu_event_state_t state, double val,
                         uint64_t timestamp_ms, const char* desc) {
    if (!engine) return -1;

    uint32_t idx = engine->head % DCU_MAX_SOE_EVENTS;
    dcu_soe_event_record_t* rec = &engine->records[idx];

    rec->event_id = ++engine->total_events_logged;
    rec->oad_event_code = oad_code;
    if (addr) {
        strncpy(rec->meter_addr, addr, sizeof(rec->meter_addr) - 1);
    } else {
        rec->meter_addr[0] = '\0';
    }
    rec->device_id = dev_id;
    rec->state = state;
    rec->soe_timestamp_ms = timestamp_ms;
    if (state == EVT_STATE_TRIGGERED) {
        rec->trigger_value = val;
        rec->restore_value = 0.0;
        engine->active_alarm_count++;
    } else {
        rec->restore_value = val;
        if (engine->active_alarm_count > 0) {
            engine->active_alarm_count--;
        }
    }
    if (desc) {
        strncpy(rec->description, desc, sizeof(rec->description) - 1);
    } else {
        rec->description[0] = '\0';
    }

    engine->head = (engine->head + 1) % DCU_MAX_SOE_EVENTS;
    return 0;
}

uint32_t dcu_soe_get_active_alarms(const dcu_soe_event_engine_t* engine) {
    if (!engine) return 0;
    return engine->active_alarm_count;
}
