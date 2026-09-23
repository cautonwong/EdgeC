/**
 * @file storage.h
 * @brief EdgeOS Public SDK Unified Telemetry Storage Interface (ARCH-DEEP-03)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#ifndef EDGEOS_SDK_STORAGE_H
#define EDGEOS_SDK_STORAGE_H

#include "../../modules/edge_infra/include/edge_telemetry_store.h"
#include "../../modules/edge_infra/include/storage_ops.h"
#include "edgeos/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef edge_telemetry_store_t edgeos_telemetry_store_t;
typedef edge_telemetry_store_cfg_t edgeos_telemetry_store_cfg_t;
typedef edge_telemetry_store_stats_t edgeos_telemetry_store_stats_t;
typedef telemetry_forward_fn edgeos_telemetry_forward_fn;
typedef telemetry_query_cb edgeos_telemetry_query_cb;

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_SDK_STORAGE_H */
