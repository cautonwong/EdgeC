#include "edge_cep_engine.h"
#include <string.h>

struct cep_engine {
    cep_rule_t rules[CEP_MAX_RULES];
    uint32_t rule_count;
    uint64_t total_events;
    uint64_t total_alerts;
};

_Static_assert(sizeof(struct cep_engine) <= sizeof(cep_engine_storage_t),
               "cep_engine_storage_t size is insufficient");

cep_engine_t *cep_engine_create_in(cep_engine_storage_t *store) {
    if (!store) return NULL;
    cep_engine_t *engine = (cep_engine_t *)store->raw;
    cep_engine_init(engine);
    return engine;
}

void cep_engine_init(cep_engine_t *engine) {
    if (!engine) return;
    memset(engine, 0, sizeof(struct cep_engine));
}

int cep_engine_add_rule(cep_engine_t *engine, uint32_t rule_id, uint16_t tag_a, double val_a_min, uint16_t tag_b, double val_b_min, uint64_t window_ms, uint16_t alert_tag) {
    if (!engine || engine->rule_count >= CEP_MAX_RULES) return -1;
    cep_rule_t *r = &engine->rules[engine->rule_count++];
    r->rule_id = rule_id;
    r->tag_a = tag_a;
    r->val_a_min = val_a_min;
    r->tag_b = tag_b;
    r->val_b_min = val_b_min;
    r->max_time_window_ms = window_ms;
    r->last_event_a_time = 0;
    r->event_a_active = false;
    r->alert_tag = alert_tag;
    return 0;
}

int cep_engine_feed_event(cep_engine_t *engine, const cep_event_t *event, uint16_t *out_alert_tag, bool *triggered) {
    if (!engine || !event || !out_alert_tag || !triggered) return -1;
    *triggered = false;
    engine->total_events++;

    for (uint32_t i = 0; i < engine->rule_count; i++) {
        cep_rule_t *r = &engine->rules[i];

        /* Check for Event A */
        if (event->tag_id == r->tag_a && event->val >= r->val_a_min) {
            r->event_a_active = true;
            r->last_event_a_time = event->timestamp_ms;
        }

        /* Check for Event B after Event A within window */
        if (event->tag_id == r->tag_b && event->val >= r->val_b_min) {
            if (r->event_a_active) {
                if (event->timestamp_ms >= r->last_event_a_time &&
                    (event->timestamp_ms - r->last_event_a_time) <= r->max_time_window_ms) {
                    *out_alert_tag = r->alert_tag;
                    *triggered = true;
                    r->event_a_active = false; /* Reset sequence trigger */
                    engine->total_alerts++;
                    return 0;
                } else {
                    /* Window expired */
                    r->event_a_active = false;
                }
            }
        }
    }

    return 0;
}
