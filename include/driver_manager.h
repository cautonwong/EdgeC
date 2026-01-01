#ifndef DRIVER_MANAGER_H
#define DRIVER_MANAGER_H

#include "arbiter.h"
#include "edge_driver.h"

struct driver_manager_s;
typedef struct driver_manager_s driver_manager_t;

// 生命周期管理
driver_manager_t* driver_manager_create(void);
void driver_manager_destroy(driver_manager_t* dm);

// 物理总线 FD 管理
int driver_manager_get_fd(driver_manager_t* dm, arbiter_bus_t bus);

// 驱动程序注册管理 (全局静态)
int driver_manager_init(void);
int driver_manager_register(edge_driver_t* driver);
edge_driver_t* driver_manager_find(const char* name);
void driver_manager_fini(void);

#endif
