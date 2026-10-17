// VIOLATION: Business domain/algo acts as a pseudo-composition-root
#include "edge_composition_root.h"

void algo_pseudo_root(void) {
    edge_composition_root_t root;
    edge_composition_root_init(&root, NULL);
}
