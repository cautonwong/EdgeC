#include "probe3.h"

#include <stdlib.h>

struct probe3_s {
    int id;
};

_Static_assert(sizeof(struct probe3_s) <= sizeof(probe3_storage_t), "probe3 storage too small");

/* #411 probe #4: single-line placement constructor that allocates.
 * The old guard did `continue` right after matching the signature, so the
 * malloc on the same line was never inspected. */
probe3_t *probe3_create_in(probe3_storage_t *store) { (void)store; return (probe3_t *)malloc(sizeof(struct probe3_s)); }
