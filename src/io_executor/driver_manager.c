#include "driver_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "edge_log.h"

// 物理结构体定义
struct driver_manager_s {
    int fds[BUS_MAX];
};

driver_manager_t* driver_manager_create(void) {
    driver_manager_t* dm = malloc(sizeof(struct driver_manager_s));
    if (!dm) return NULL;

    for (int i = 0; i < BUS_MAX; i++) {
        dm->fds[i] = -1;
    }

    // 模拟总线初始化
    int fd1 = 10;  // Mock FD
    int fd2 = 11;

    dm->fds[BUS_RS485_1] = fd1;
    LOG_INFO(LOG_CAT_DRIVER, "Bus RS485-1 initialized at fd %d", fd1);

    dm->fds[BUS_RS485_2] = fd2;
    LOG_INFO(LOG_CAT_DRIVER, "Bus RS485-2 initialized at fd %d", fd2);

    return dm;
}

void driver_manager_destroy(driver_manager_t* dm) {
    if (dm) free(dm);
}

int driver_manager_get_fd(driver_manager_t* dm, arbiter_bus_t bus) {
    if (!dm || bus >= BUS_MAX) return -1;
    return dm->fds[bus];
}

// --- 驱动实例注册逻辑 (底层实现) ---
static edge_driver_t* g_drivers[32];
static int g_driver_count = 0;

int driver_manager_init(void) {
    g_driver_count = 0;
    return 0;
}

int driver_manager_register(edge_driver_t* driver) {
    if (g_driver_count >= 32) return -1;
    g_drivers[g_driver_count++] = driver;
    return 0;
}

edge_driver_t* driver_manager_find(const char* name) {
    for (int i = 0; i < g_driver_count; i++) {
        if (strcmp(g_drivers[i]->name, name) == 0) return g_drivers[i];
    }
    return NULL;
}

void driver_manager_fini(void) {
    g_driver_count = 0;
}