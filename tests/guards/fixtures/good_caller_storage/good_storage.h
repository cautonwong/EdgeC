#ifndef GOOD_CALLER_STORAGE_H
#define GOOD_CALLER_STORAGE_H

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

typedef struct good_entity good_entity_t;

typedef struct {
    alignas(max_align_t) uint8_t raw[128];
} good_entity_storage_t;

good_entity_t *good_entity_create_in(good_entity_storage_t *store, int id, double val);

#endif
