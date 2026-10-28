#ifndef PROBE_DATAHUB_H
#define PROBE_DATAHUB_H

#include <stdint.h>

/* Minimal stand-in for modules/edge_core/include/datahub.h so the guard can
 * parse a real aggregate layout instead of a hand-written field list. */
typedef struct datahub_s {
    uint32_t slot_count;
    void *ring;
    void *shadow_table;
    uint32_t shadow_capacity;
} datahub_t;

int datahub_init(datahub_t *hub);

#endif
