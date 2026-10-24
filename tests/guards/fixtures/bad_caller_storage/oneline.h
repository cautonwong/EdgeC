#ifndef BAD_CALLER_STORAGE_ONELINE_H
#define BAD_CALLER_STORAGE_ONELINE_H

/* Regression fixture for #411: the placement-constructor scanner used to skip a body
 * that opened and closed on the signature line. The header itself is compliant, so the
 * only thing this directory can trip is the one-line body below. */
#include <stdalign.h>
#include <stdint.h>

typedef struct oneline_entity oneline_entity_t;

typedef struct {
    alignas(max_align_t) uint8_t raw[128];
} oneline_entity_storage_t;

oneline_entity_t *oneline_entity_create_in(oneline_entity_storage_t *store);

#endif
