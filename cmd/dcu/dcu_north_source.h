/**
 * @file dcu_north_source.h
 * @brief DCU 北向取数端口（ADR-0011 族 A）
 */

#ifndef DCU_NORTH_SOURCE_H
#define DCU_NORTH_SOURCE_H

#include "edge_north_plane.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 取 DCU 的北向取数端口，由组合根注入给 `northbound_mgr`。
 *
 * 上下文传 `datahub_t*`（`northbound_mgr_set_north_source(&g_north, ops, hub)`）。
 *
 * 为什么归 cmd 而不是 edge_io：点位名是 DCU 的点位模型（698 OAD）的一部分，
 * 驱动不认识它。这与 ADR-0010「产品用行业原生标识」是同一条。
 */
const edge_north_source_ops_t *dcu_north_source_ops(void);

#ifdef __cplusplus
}
#endif

#endif /* DCU_NORTH_SOURCE_H */
