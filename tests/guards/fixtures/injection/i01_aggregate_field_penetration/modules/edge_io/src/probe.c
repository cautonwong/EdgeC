#include "datahub.h"

/* #411 probe #1: reach into an aggregate root's internals from another module.
 * `shadow_table` was absent from the old hard-coded field list, so this passed. */
void probe_penetrate(datahub_t *hub) {
    hub->shadow_table = 0;
    hub->shadow_capacity = 0;
}
