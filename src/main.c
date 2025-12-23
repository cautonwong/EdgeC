#include <stdio.h>
#include "edge_version.h"
#include "core_mem.h"

int main(void) {
    printf("Starting %s...\n", edge_get_version());
    printf("Self-Test: 1+1=%d\n", edge_add(1, 1));

    if (core_mem_init() != 0) {
        fprintf(stderr, "Failed to initialize memory manager!\n");
        return 1;
    }
    printf("Memory manager initialized successfully.\n");

    return 0;
}