#include <stdint.h>

void control_step(void) {
    // Compliant: Zero heap allocation in control step
    static uint8_t prealloc[128];
    prealloc[0] = 1;
}
