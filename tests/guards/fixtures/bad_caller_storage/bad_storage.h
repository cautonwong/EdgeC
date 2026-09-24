#ifndef BAD_CALLER_STORAGE_H
#define BAD_CALLER_STORAGE_H

#include <stddef.h>
#include <stdint.h>

typedef struct bad_entity bad_entity_t;

/* VIOLATION: Non-aligned storage placeholder */
typedef struct {
    uint8_t raw[128];
} bad_entity_storage_t;

bad_entity_t *bad_entity_create_in(bad_entity_storage_t *store, int id);

#endif
