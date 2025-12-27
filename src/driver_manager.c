#include "driver_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <uv.h>

struct driver_manager_s {
    int fds[BUS_MAX];
};

driver_manager_t* driver_manager_create(void) {
    driver_manager_t* dm = malloc(sizeof(driver_manager_t));
    if (!dm) return NULL;

    for (int i = 0; i < BUS_MAX; ++i) {
        dm->fds[i] = -1;
    }

    // Use libuv synchronous FS API
    uv_fs_t req;
    
    // RS485_1
    int fd1 = uv_fs_open(NULL, &req, "/dev/null", UV_FS_O_RDWR | UV_FS_O_NONBLOCK, 0, NULL);
    dm->fds[BUS_RS485_1] = fd1;
    uv_fs_req_cleanup(&req);

    // RS485_2
    int fd2 = uv_fs_open(NULL, &req, "/dev/null", UV_FS_O_RDWR | UV_FS_O_NONBLOCK, 0, NULL);
    dm->fds[BUS_RS485_2] = fd2;
    uv_fs_req_cleanup(&req);

    printf("[DriverManager] Manager created using Libuv FS API.\n");
    return dm;
}

void driver_manager_destroy(driver_manager_t* dm) {
    if (!dm) return;

    uv_fs_t req;
    for (int i = 0; i < BUS_MAX; ++i) {
        if (dm->fds[i] >= 0) {
            uv_fs_close(NULL, &req, dm->fds[i], NULL);
            uv_fs_req_cleanup(&req);
            dm->fds[i] = -1;
        }
    }
    free(dm);
}

int driver_manager_get_fd(driver_manager_t* dm, arbiter_bus_t bus) {
    if (!dm || (int)bus >= BUS_MAX) return -1;
    return dm->fds[bus];
}