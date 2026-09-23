#include <stdlib.h>

void control_step(void) {
    // VIOLATION: Calling malloc in real-time control step loop
    void *ptr = malloc(64);
    free(ptr);
}
