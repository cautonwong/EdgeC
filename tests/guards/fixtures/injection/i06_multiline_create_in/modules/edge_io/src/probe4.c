#include "probe4.h"

#include <stdlib.h>

struct probe4_s {
    int id;
};

_Static_assert(sizeof(struct probe4_s) <= sizeof(probe4_storage_t), "probe4 storage too small");

/* #411 probe #6: multi-line placement constructor that allocates (already
 * caught by the old guard -- kept as a regression). */
probe4_t *probe4_create_in(probe4_storage_t *store) {
    (void)store;
    probe4_t *obj = (probe4_t *)malloc(sizeof(struct probe4_s));
    return obj;
}
