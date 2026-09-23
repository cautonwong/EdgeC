#include <stdint.h>

void task_loop(void) {
    // VIOLATION: Oversized stack buffer 2048 bytes > 512B threshold
    char oversized_buffer[2048];
    oversized_buffer[0] = 'b';
}
