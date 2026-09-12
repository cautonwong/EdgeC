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
 * @brief Register all 4 specialized industry algorithm MCP tools to dispatcher
 * @param disp Dispatcher instance
 * @return 0 on success, negative error code on failure
 */
int edge_mcp_register_industry_tools(edge_mcp_dispatcher_t *disp);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_MCP_INDUSTRY_TOOLS_H */
