/**
 * @file fake_app_host.c
 * @brief Child for runtime stop tests: holds the inherited ABI fd, never acks.
 */

#include <unistd.h>

int main(void) {
    for (;;) pause();
}
