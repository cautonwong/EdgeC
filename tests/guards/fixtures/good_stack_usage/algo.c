#include <stdint.h>

void task_loop(void) {
    // Compliant: small stack buffer <= 512 bytes
    char small_buffer[64];
    small_buffer[0] = 'a';
}
