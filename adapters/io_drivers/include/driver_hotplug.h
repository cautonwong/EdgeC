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
#include <stdalign.h>

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

typedef struct driver_hotplug_mgr driver_hotplug_mgr_t;

/**
 * @brief 热插拔驱动管理器调用方分配存储容器 (Caller-Allocated Storage - AEP-752)
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[2048];
} driver_hotplug_mgr_storage_t;

/**
 * @brief 在调用方提供的内存就地构造驱动热插拔管理器聚合根 (Zero-Alloc)
 * @param store 调用方分配的存储块
 * @return 构造成功的聚合根指针, 失败返回 NULL
 */
driver_hotplug_mgr_t *driver_hotplug_mgr_create_in(driver_hotplug_mgr_storage_t *store);

/**
 * @brief 获取当前已加载驱动插件数量
 */
size_t driver_hotplug_get_active_count(const driver_hotplug_mgr_t *mgr);

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
