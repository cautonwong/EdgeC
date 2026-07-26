/**
 * @file driver_hotplug.c
 * @brief Dynamic Driver Hot-Plug & ABI Guard Sandbox Manager Implementation
 */

#include "driver_hotplug.h"
#include <string.h>

int driver_hotplug_init(driver_hotplug_mgr_t *mgr) {
    if (!mgr) {
        return -1;
    }

    memset(mgr, 0, sizeof(driver_hotplug_mgr_t));
    return 0;
}

int driver_hotplug_load(driver_hotplug_mgr_t *mgr, const char *path, const driver_plugin_ops_t *ops, uint32_t *out_slot_id) {
    if (!mgr || !path || !ops || !out_slot_id) {
        return -1;
    }

    /* ABI Validation */
    if (ops->abi_magic != DRIVER_ABI_MAGIC || ops->abi_version != DRIVER_ABI_VERSION) {
        return -2; /* ABI Mismatch */
    }

    for (uint32_t i = 0; i < DRIVER_HOTPLUG_MAX_SLOTS; i++) {
        if (!mgr->slots[i].loaded) {
            mgr->slots[i].loaded = true;
            mgr->slots[i].slot_id = i + 1;
            strncpy(mgr->slots[i].driver_path, path, sizeof(mgr->slots[i].driver_path) - 1);
            mgr->slots[i].ops = *ops;
            mgr->slots[i].crash_count = 0;
            mgr->active_count++;

            if (ops->init_fn) {
                ops->init_fn();
            }

            *out_slot_id = mgr->slots[i].slot_id;
            return 0;
        }
    }

    return -3; /* No free slot */
}

int driver_hotplug_unload(driver_hotplug_mgr_t *mgr, uint32_t slot_id) {
    if (!mgr || slot_id == 0 || slot_id > DRIVER_HOTPLUG_MAX_SLOTS) {
        return -1;
    }

    uint32_t idx = slot_id - 1;
    if (mgr->slots[idx].loaded) {
        if (mgr->slots[idx].ops.cleanup_fn) {
            mgr->slots[idx].ops.cleanup_fn();
        }
        mgr->slots[idx].loaded = false;
        if (mgr->active_count > 0) {
            mgr->active_count--;
        }
        return 0;
    }

    return -2;
}
