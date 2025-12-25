#include "driver_manager.h"
#include <stdlib.h>
#include <unistd.h> // For open, close
#include <fcntl.h>  // For O_RDWR etc.
#include <stdio.h>  // For printf, perror

struct driver_manager_s {
    int fds[BUS_MAX];
};

driver_manager_t* driver_manager_create(void) {
    driver_manager_t* dm = malloc(sizeof(driver_manager_t));
    if (!dm) {
        perror("driver_manager_create: malloc");
        return NULL;
    }

    // Initialize all fds to -1 (not available)
    for (int i = 0; i < BUS_MAX; ++i) {
        dm->fds[i] = -1;
    }

    // --- Open physical devices ---
    // This is where configuration would be read to get device paths (e.g., "/dev/ttyS0")
    // For this implementation, we use a mixture of /dev/null and pipes to have valid,
    // non-blocking FDs for testing purposes without real hardware.

    // Using /dev/null is safe for write/read, it just discards data or returns EOF.
    // O_NONBLOCK is important if the main loop ever tried to interact with these.
    // For the io_executor worker thread, it doesn't matter as much, but still good practice.
    dm->fds[BUS_RS485_1] = open("/dev/null", O_RDWR | O_NONBLOCK);
    if (dm->fds[BUS_RS485_1] < 0) {
        perror("open /dev/null for BUS_RS485_1");
        // continue, maybe some buses are not critical
    } else {
        printf("[DriverManager] Opened bus RS485_1 on fd %d\n", dm->fds[BUS_RS485_1]);
    }

    dm->fds[BUS_RS485_2] = open("/dev/null", O_RDWR | O_NONBLOCK);
    if (dm->fds[BUS_RS485_2] < 0) {
        perror("open /dev/null for BUS_RS485_2");
    } else {
        printf("[DriverManager] Opened bus RS485_2 on fd %d\n", dm->fds[BUS_RS485_2]);
    }

    // For other buses, we can leave them as -1 to indicate they are not configured.
    printf("[DriverManager] Manager created.\n");
    return dm;
}

void driver_manager_destroy(driver_manager_t* dm) {
    if (!dm) return;

    printf("[DriverManager] Destroying manager and closing fds.\n");
    for (int i = 0; i < BUS_MAX; ++i) {
        if (dm->fds[i] >= 0) {
            close(dm->fds[i]);
            dm->fds[i] = -1;
        }
    }
    free(dm);
}

int driver_manager_get_fd(driver_manager_t* dm, arbiter_bus_t bus) {
    if (!dm || bus >= BUS_MAX) {
        return -1;
    }
    return dm->fds[bus];
}
