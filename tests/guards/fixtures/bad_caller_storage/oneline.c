#include "oneline.h"

#include <stdlib.h>

struct oneline_entity {
    int id;
};

_Static_assert(sizeof(struct oneline_entity) <= sizeof(oneline_entity_storage_t),
               "oneline_entity_t exceeds its storage");

/* VIOLATION: heap allocation inside a placement constructor, written as a one-liner. */
oneline_entity_t *oneline_entity_create_in(oneline_entity_storage_t *store) { (void)store; return malloc(sizeof(struct oneline_entity)); }
