#include "northbound_driver_adapter.h"

#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "net_buf.h"

#define NB_DRIVER_POOL_BLOCKS 32
#define NB_DRIVER_POOL_BLOCK_SIZE 1024

int northbound_attach_default_driver(northbound_mgr_t *mgr) {
    if (!mgr) return -1;

    static driver_manager_storage_t dm_storage;
    driver_manager_t *dm = driver_manager_create_in(&dm_storage);
    if (!dm) {
        LOG_ERROR(LOG_CAT_NB, "Northbound: driver manager storage unusable");
        return -1;
    }

    const char *proto = northbound_mgr_get_protocol(mgr);
    if (northbound_mgr_register_codec(proto, dm) != 0) {
        driver_manager_destroy(dm);
        return -1;
    }

    edge_driver_ctx_t *driver = driver_instance_create(dm, proto, 1);
    if (!driver) {
        LOG_ERROR(LOG_CAT_NB, "Northbound: cannot create driver instance '%s'", proto);
        driver_manager_destroy(dm);
        return -1;
    }

    driver->pool = net_buf_pool_new(NB_DRIVER_POOL_BLOCKS, NB_DRIVER_POOL_BLOCK_SIZE, 0);
    if (!driver->pool) {
        driver_instance_destroy(driver, proto);
        driver_manager_destroy(dm);
        return -1;
    }

    return northbound_mgr_attach_driver(mgr, dm, driver);
}
