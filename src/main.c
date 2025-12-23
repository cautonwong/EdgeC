#include <stdio.h>
#include "edge_version.h"

int main(void) {
    printf("Starting %s...\n", edge_get_version());
    printf("Self-Test: 1+1=%d\n", edge_add(1, 1));
    return 0;
}