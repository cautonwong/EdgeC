/**
 * @file edge_license.h
 * @brief SDK Tiering and License Engine Definitions
 */

#ifndef EDGE_LICENSE_H
#define EDGE_LICENSE_H

#include <stdbool.h>

/**
 * @brief License validation outcome
 */
typedef enum {
    LICENSE_VALID = 0,
    LICENSE_INVALID_SIGNATURE = -1,
    LICENSE_EXPIRED = -2,
    LICENSE_TIER_VIOLATION = -3
} license_status_t;

/**
 * @brief Current active tier based on build flags
 */
typedef enum {
    TIER_BASIC = 1,
    TIER_PROFESSIONAL = 2,
    TIER_ENTERPRISE = 3
} edgec_tier_t;

#include <string.h>
#include "autoconf.h"

#define CURRENT_SDK_TIER _edgec_get_compiled_tier()

static inline edgec_tier_t _edgec_get_compiled_tier(void) {
    if (strcmp(CONFIG_EDGEC_TIER, "ENTERPRISE") == 0) return TIER_ENTERPRISE;
    if (strcmp(CONFIG_EDGEC_TIER, "PROFESSIONAL") == 0) return TIER_PROFESSIONAL;
    return TIER_BASIC;
}

/**
 * @brief Initialize the Licensing Engine during boot sequence
 */
int license_engine_init(const char *license_key);

/**
 * @brief Verify if a specific feature is allowed by the current license
 */
bool license_engine_check_feature(const char *feature_name);

/**
 * @brief Check if the current binary tier supports WASM
 */
static inline bool license_supports_wasm(void) {
    return CURRENT_SDK_TIER >= TIER_ENTERPRISE;
}

/**
 * @brief Check if the current binary tier supports Advanced DSP/Linalg
 */
static inline bool license_supports_dsp(void) {
    return CURRENT_SDK_TIER >= TIER_PROFESSIONAL;
}

#endif // EDGE_LICENSE_H
