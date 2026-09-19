/**
 * @file edge_mcp_industry_tools.h
 * @brief Industry Specialized Algorithm MCP Tools (AEP-508 / Issue #192)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 *
 * Exposes specialized industrial domain algorithms as standard MCP tools:
 * 1. DCU Power Quality & Phase Commutation (GB/T 15543, Harmonics up to 31st)
 * 2. SmartBox Transformer Thermal Hot-spot & Life Aging (IEEE C57.91 / IEC 60076-7)
 * 3. IEMS Photovoltaic + ESS TOU Arbitrage Scheduler (24h Peak-Valley Arbitrage)
 * 4. Electrical Five-Anti Safety Interlocking Simulator (DL/T 687 Rules Engine)
 */

#ifndef EDGE_MCP_INDUSTRY_TOOLS_H
#define EDGE_MCP_INDUSTRY_TOOLS_H

#include "edgeos/edge_mcp_dispatcher.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handler for 'dcu_analyze_power_quality'
 * Analyzes 3-phase voltage unbalance and harmonics, provides commutation guidance.
 */
int handle_dcu_analyze_power_quality(edge_mcp_dispatcher_t *disp,
                                     const cJSON *params,
                                     cJSON **out_result,
                                     char *err_msg,
                                     size_t err_len);

/**
 * @brief Handler for 'smartbox_eval_transformer_life'
 * Computes hot-spot temperature rise, aging factor FAA, and remaining lifespan.
 */
int handle_smartbox_eval_transformer_life(edge_mcp_dispatcher_t *disp,
                                         const cJSON *params,
                                         cJSON **out_result,
                                         char *err_msg,
                                         size_t err_len);

/**
 * @brief Handler for 'iems_optimize_tou_schedule'
 * Optimizes 24h battery charge/discharge schedule against peak/valley tariffs.
 */
int handle_iems_optimize_tou_schedule(edge_mcp_dispatcher_t *disp,
                                      const cJSON *params,
                                      cJSON **out_result,
                                      char *err_msg,
                                      size_t err_len);

/**
 * @brief Handler for 'security_simulate_interlock'
 * Evaluates breaker/disconnector/earth-switch switching against Electrical Five-Anti rules.
 */
int handle_security_simulate_interlock(edge_mcp_dispatcher_t *disp,
                                       const cJSON *params,
                                       cJSON **out_result,
                                       char *err_msg,
                                       size_t err_len);

/**
 * @brief Handler for 'ess_emergency_stop'
 * Triggers emergency shutdown of energy storage system with Five-Anti interlock verification.
 */
int handle_ess_emergency_stop(edge_mcp_dispatcher_t *disp,
                              const cJSON *params,
                              cJSON **out_result,
                              char *err_msg,
                              size_t err_len);

/**
 * @brief Handler for 'iems_get_battery_status'
 * Queries real-time battery cluster status with Fast-Track SHM zero-copy handle.
 */
int handle_iems_get_battery_status(edge_mcp_dispatcher_t *disp,
                                   const cJSON *params,
                                   cJSON **out_result,
                                   char *err_msg,
                                   size_t err_len);

/**
 * @brief Handler for 'smartbox_get_topology'
 * Queries transformer distribution network topology and identified branch phases.
 */
int handle_smartbox_get_topology(edge_mcp_dispatcher_t *disp,
                                 const cJSON *params,
                                 cJSON **out_result,
                                 char *err_msg,
                                 size_t err_len);

/**
 * @brief Handler for 'tag_batch_read_scaled'
 * Batch reads DataHub tags with ABCD/CDAB/BADC/DCBA byte swapping, scaling and deadband filtering.
 */
int handle_tag_batch_read_scaled(edge_mcp_dispatcher_t *disp,
                                 const cJSON *params,
                                 cJSON **out_result,
                                 char *err_msg,
                                 size_t err_len);

/**
 * @brief Register all specialized industry algorithm MCP tools to dispatcher
 * @param disp Dispatcher instance
 * @return 0 on success, negative error code on failure
 */
int edge_mcp_register_industry_tools(edge_mcp_dispatcher_t *disp);

/* Forward declaration for Hub */
struct mcp_bus_hub_s;

/**
 * @brief Register IEMS ESS microservice provider into the shared MCP bus hub
 */
int edge_mcp_register_iems_provider(struct mcp_bus_hub_s *hub, const edge_mcp_env_t *env);

/**
 * @brief Register SmartBox transformer microservice provider into the shared MCP bus hub
 */
int edge_mcp_register_smartbox_provider(struct mcp_bus_hub_s *hub, const edge_mcp_env_t *env);

/**
 * @brief Register DCU metering microservice provider into the shared MCP bus hub
 */
int edge_mcp_register_dcu_provider(struct mcp_bus_hub_s *hub, const edge_mcp_env_t *env);

/**
 * @brief Register all 3 microservices (iems, smartbox, dcu) into the shared MCP bus hub
 */
int edge_mcp_register_all_microservices(struct mcp_bus_hub_s *hub, const edge_mcp_env_t *env);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_MCP_INDUSTRY_TOOLS_H */

