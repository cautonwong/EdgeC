// Compliant: composition root is assembled only in main.c
#include "edge_composition_root.h"

int main(void) {
    edge_composition_root_t root;
    edge_composition_root_init(&root, NULL);
    return 0;
}
