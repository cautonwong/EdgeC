/**
 * @file dfu_dualbank.h
 * @brief Dual-Bank A/B OTA Firmware DFU Engine with Auto-Rollback Guard
 */

#ifndef DFU_DUALBANK_H
#define DFU_DUALBANK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DFU_BANK_A = 0,
    DFU_BANK_B = 1
} dfu_bank_id_t;

typedef enum {
    DFU_STATE_STABLE = 0,
    DFU_STATE_TESTING = 1,
    DFU_STATE_REVERTED = 2
} dfu_state_t;

typedef struct {
    dfu_bank_id_t active_bank;
    dfu_bank_id_t pending_bank;
    dfu_state_t state;
    uint32_t active_version;
    uint32_t pending_version;
    bool health_confirmed;
    uint32_t watchdog_timeout_s;
} dfu_dualbank_t;

/**
 * @brief Initializes dual-bank DFU engine
 * @param dfu Pointer to dual-bank structure
 * @return 0 on success, negative error code on failure
 */
int dfu_dualbank_init(dfu_dualbank_t *dfu);

/**
 * @brief Stages a new firmare image into the alternate bank with signature check
 * @param dfu Pointer to dual-bank structure
 * @param image_data Pointer to binary firmware image
 * @param image_len Length of binary image
 * @param new_version Target firmware version number
 * @param signature Magic signature bytes
 * @return 0 on success, negative error code on failure
 */
int dfu_dualbank_stage_image(dfu_dualbank_t *dfu, const uint8_t *image_data, size_t image_len, uint32_t new_version, uint32_t signature);

/**
 * @brief Confirms boot health within 30s window to finalize A/B swap
 * @param dfu Pointer to dual-bank structure
 * @return 0 on success, negative error code on failure
 */
int dfu_dualbank_confirm_health(dfu_dualbank_t *dfu);

/**
 * @brief Triggers automatic rollback if health check fails or watchdog expires
 * @param dfu Pointer to dual-bank structure
 * @return 0 on success, negative error code on failure
 */
int dfu_dualbank_trigger_rollback(dfu_dualbank_t *dfu);

#ifdef __cplusplus
}
#endif

#endif /* DFU_DUALBANK_H */
