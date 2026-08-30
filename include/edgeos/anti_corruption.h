/**
 * @file anti_corruption.h
 * @brief EdgeOS Public SDK Anti-Corruption Layer (ACL) Decoupling Interface
 */

#ifndef EDGEOS_SDK_ANTI_CORRUPTION_H
#define EDGEOS_SDK_ANTI_CORRUPTION_H

#include "edgeos/types.h"
#include "edge_anti_corruption.h"

#ifdef __cplusplus
extern "C" {
#endif

// Public ACL Type Aliases
typedef edge_anti_corruption_entry_t edgeos_acl_entry_t;
typedef edge_anti_corruption_engine_t edgeos_acl_engine_t;

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_SDK_ANTI_CORRUPTION_H */
