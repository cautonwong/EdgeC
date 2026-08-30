/**
 * @file edgeos.h
 * @brief EdgeOS Public SDK Main Entry Point for Secondary Developers
 */

#ifndef EDGEOS_SDK_EDGEOS_H
#define EDGEOS_SDK_EDGEOS_H

#include "edgeos/version.h"
#include "edgeos/types.h"
#include "edgeos/datahub.h"
#include "edgeos/driver.h"
#include "edgeos/anti_corruption.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize EdgeOS SDK environment and logging
 * @return 0 on success, non-zero on failure
 */
static inline int edgeos_sdk_init(void) {
    return 0;
}

/**
 * @brief Get EdgeOS SDK Semantic Version String (e.g. "5.0.0")
 */
static inline const char *edgeos_sdk_get_version(void) {
    return EDGEOS_SDK_VERSION_STRING;
}

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_SDK_EDGEOS_H */
