/**
 * @file device_twin.h
 * @brief State-Reconciled Device Twin & Desired/Reported Dual-State Shadow Engine
 */

#ifndef DEVICE_TWIN_H
#define DEVICE_TWIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define DEVICE_TWIN_MAX_PROPERTIES 32

typedef struct {
    uint32_t property_id;
    double desired_val;
    double reported_val;
    uint64_t desired_version;
    uint64_t reported_version;
    bool in_sync;
} device_twin_prop_t;

typedef struct {
    uint32_t device_id;
    size_t prop_count;
    device_twin_prop_t properties[DEVICE_TWIN_MAX_PROPERTIES];
} device_twin_t;

/**
 * @brief Initializes device twin instance
 * @param twin Pointer to device twin instance
 * @param device_id Numerical ID of target device
 * @return 0 on success, negative error code on failure
 */
int device_twin_init(device_twin_t *twin, uint32_t device_id);

/**
 * @brief Sets cloud or HMI desired property state
 * @param twin Pointer to device twin instance
 * @param prop_id Property ID
 * @param val Desired float/double value
 * @return 0 on success, negative error code on failure
 */
int device_twin_set_desired(device_twin_t *twin, uint32_t prop_id, double val);

/**
 * @brief Sets physical device reported property state
 * @param twin Pointer to device twin instance
 * @param prop_id Property ID
 * @param val Reported float/double value
 * @return 0 on success, negative error code on failure
 */
int device_twin_set_reported(device_twin_t *twin, uint32_t prop_id, double val);

/**
 * @brief Reconciles desired and reported state differences
 * @param twin Pointer to device twin instance
 * @param out_unsynced_count Output count of unsynchronized properties
 * @return 0 on success, negative error code on failure
 */
int device_twin_reconcile(device_twin_t *twin, size_t *out_unsynced_count);

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_TWIN_H */
