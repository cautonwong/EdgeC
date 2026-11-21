#ifndef DCU_NORTH_ADAPTER_H
#define DCU_NORTH_ADAPTER_H

#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "edge_slave_plane.h"
#include "northbound_mgr.h"

#define DCU_NORTH_BUS 10

typedef struct datahub_s datahub_t;
typedef struct bus_orchestrator_s bus_orchestrator_t;

/** 标量取数（同一张 OAD→类型表）；测试直接调用，运行时由 ops 注入。 */
int dcu_north_resolve_scalar(void *ctx, uint32_t oad, const void *desc, size_t desc_len,
                             edge_point_value_t *out);

void dcu_north_slave_attach(uv_loop_t *loop, datahub_t *hub, bus_orchestrator_t *orch);
void dcu_north_cloud_attach(datahub_t *hub);
void dcu_north_stop(void);
void dcu_north_destroy(void);
northbound_mgr_t *dcu_north_mgr(void);

#endif /* DCU_NORTH_ADAPTER_H */
