/**
 * @file driver.h
 * @brief EdgeOS Public SDK Secondary Developer Driver Registration & Ops Interface
 */

#ifndef EDGEOS_SDK_DRIVER_H
#define EDGEOS_SDK_DRIVER_H

#include "edgeos/types.h"
#include "../../modules/edge_io/include/edge_driver.h"
#include "../../modules/edge_io/include/driver_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef edge_driver_t edgeos_driver_t;
typedef edge_driver_ctx_t edgeos_driver_ctx_t;
typedef edge_point_config_t edgeos_point_config_t;
typedef edge_poll_policy_t edgeos_poll_policy_t;
typedef edge_orchestrator_cfg_t edgeos_orchestrator_cfg_t;
typedef edge_poll_stats_t edgeos_poll_stats_t;

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_SDK_DRIVER_H */
