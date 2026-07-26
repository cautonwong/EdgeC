/**
 * @file dfu_dualbank.c
 * @brief Dual-Bank A/B OTA Firmware DFU Engine Implementation
 */

#include "dfu_dualbank.h"
#include <string.h>

#define DFU_VALID_SIGNATURE 0x4F544131 /* "OTA1" */

int dfu_dualbank_init(dfu_dualbank_t *dfu) {
    if (!dfu) {
        return -1;
    }

    memset(dfu, 0, sizeof(dfu_dualbank_t));
    dfu->active_bank = DFU_BANK_A;
    dfu->pending_bank = DFU_BANK_A;
    dfu->state = DFU_STATE_STABLE;
    dfu->active_version = 0x010000;
    dfu->health_confirmed = true;
    dfu->watchdog_timeout_s = 30;
    return 0;
}

int dfu_dualbank_stage_image(dfu_dualbank_t *dfu, const uint8_t *image_data, size_t image_len, uint32_t new_version, uint32_t signature) {
    if (!dfu || !image_data || image_len == 0) {
        return -1;
    }

    if (signature != DFU_VALID_SIGNATURE) {
        return -2; /* Invalid Signature */
    }

    dfu->pending_bank = (dfu->active_bank == DFU_BANK_A) ? DFU_BANK_B : DFU_BANK_A;
    dfu->pending_version = new_version;
    dfu->state = DFU_STATE_TESTING;
    dfu->health_confirmed = false;

    return 0;
}

int dfu_dualbank_confirm_health(dfu_dualbank_t *dfu) {
    if (!dfu) {
        return -1;
    }

    if (dfu->state != DFU_STATE_TESTING) {
        return -2; /* Not in testing state */
    }

    dfu->active_bank = dfu->pending_bank;
    dfu->active_version = dfu->pending_version;
    dfu->state = DFU_STATE_STABLE;
    dfu->health_confirmed = true;

    return 0;
}

int dfu_dualbank_trigger_rollback(dfu_dualbank_t *dfu) {
    if (!dfu) {
        return -1;
    }

    if (dfu->state == DFU_STATE_TESTING) {
        dfu->pending_bank = dfu->active_bank;
        dfu->state = DFU_STATE_REVERTED;
        dfu->health_confirmed = true;
        return 0;
    }

    return -2;
}
