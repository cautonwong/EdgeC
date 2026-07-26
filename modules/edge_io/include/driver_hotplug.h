/**
 * @file driver_hotplug.h
 * @brief Dynamic Driver Hot-Plug & ABI Guard Sandbox Manager for EdgeC
 */

#ifndef DRIVER_HOTPLUG_H
#define DRIVER_HOTPLUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define DRIVER_ABI_MAGIC      0x45444745 /* "EDGE" */
#define DRIVER_ABI_VERSION    0x02010000 /* V2.1.0 */
#define DRIVER_HOTPLUG_MAX_SLOTS 8

typedef struct {
    uint32_t abi_magic;
    uint32_t abi_version;
    const char *driver_name;
    int (*init_fn)(void);
    int (*poll_fn)(uint8_t *out_data, size_t *out_len);
    void (*cleanup_fn)(void);
} driver_plugin_ops_t;

typedef struct {
    uint32_t slot_id;
    bool loaded;
    char driver_path[128];
    driver_plugin_ops_t ops;
    uint32_t crash_count;
} driver_slot_t;

typedef struct {
    size_t active_count;
    driver_slot_t slots[DRIVER_HOTPLUG_MAX_SLOTS];
} driver_hotplug_mgr_t;

/**
 * @brief Initializes dynamic driver hotplug manager
 * @param mgr Pointer to hotplug manager instance
 * @return 0 on success, negative error code on failure
 */
int driver_hotplug_init(driver_hotplug_mgr_t *mgr);

/**
 * @brief Loads a driver plugin struct with strict ABI validation
 * @param mgr Pointer to hotplug manager instance
 * @param path File system path to .so driver plugin
 * @param ops Pointer to driver plugin ops structure
 * @param out_slot_id Output assigned slot ID
 * @return 0 on success, negative error code on failure (ABI mismatch or full)
 */
int driver_hotplug_load(driver_hotplug_mgr_t *mgr, const char *path, const driver_plugin_ops_t *ops, uint32_t *out_slot_id);

/**
 * @brief Unloads an active driver plugin
 * @param mgr Pointer to hotplug manager instance
 * @param slot_id Slot ID to unload
 * @return 0 on success, negative error code on failure
 */
int driver_hotplug_unload(driver_hotplug_mgr_t *mgr, uint32_t slot_id);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_HOTPLUG_H */
