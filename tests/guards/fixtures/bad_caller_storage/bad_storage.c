#include "bad_storage.h"

#include <stdlib.h>

struct bad_entity {
    int id;
};

/* VIOLATION: Lacks _Static_assert and calls malloc in _create_in */
bad_entity_t *bad_entity_create_in(bad_entity_storage_t *store, int id) {
    bad_entity_t *ent = (bad_entity_t *)malloc(sizeof(bad_entity_t));
    if (ent) ent->id = id;
    return ent;
}
