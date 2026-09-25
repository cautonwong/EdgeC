#ifndef EDGE_CEP_ENGINE_H
#define EDGE_CEP_ENGINE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CEP_MAX_RULES 32

typedef struct {
    uint16_t tag_id;
    double val;
    uint64_t timestamp_ms;
} cep_event_t;

typedef struct {
    uint32_t rule_id;
    uint16_t tag_a;
    double val_a_min;
    uint16_t tag_b;
    double val_b_min;
    uint64_t max_time_window_ms;
    uint64_t last_event_a_time;
    bool event_a_active;
    uint16_t alert_tag;
} cep_rule_t;

typedef struct cep_engine cep_engine_t;

typedef struct {
    alignas(max_align_t) uint8_t raw[2048];
} cep_engine_storage_t;

cep_engine_t *cep_engine_create_in(cep_engine_storage_t *store);
void cep_engine_init(cep_engine_t *engine);
int cep_engine_add_rule(cep_engine_t *engine, uint32_t rule_id, uint16_t tag_a, double val_a_min, uint16_t tag_b, double val_b_min, uint64_t window_ms, uint16_t alert_tag);
int cep_engine_feed_event(cep_engine_t *engine, const cep_event_t *event, uint16_t *out_alert_tag, bool *triggered);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_CEP_ENGINE_H */
