#include "good_storage.h"

#include <string.h>

struct good_entity {
    int id;
    double val;
};

_Static_assert(sizeof(struct good_entity) <= sizeof(good_entity_storage_t),
               "good_entity_storage_t too small");
_Static_assert(_Alignof(struct good_entity) <= _Alignof(good_entity_storage_t),
               "good_entity_storage_t alignment mismatch");

good_entity_t *good_entity_create_in(good_entity_storage_t *store, int id, double val) {
    if (!store) return NULL;
    good_entity_t *ent = (good_entity_t *)store->raw;
    memset(ent, 0, sizeof(struct good_entity));
    ent->id = id;
    ent->val = val;
    return ent;
}
