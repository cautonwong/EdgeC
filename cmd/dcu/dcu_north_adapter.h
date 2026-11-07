#ifndef DCU_NORTH_ADAPTER_H
#define DCU_NORTH_ADAPTER_H

#include <uv.h>

#include "northbound_mgr.h"

#define DCU_NORTH_BUS 10

typedef struct datahub_s datahub_t;
typedef struct bus_orchestrator_s bus_orchestrator_t;

void dcu_north_slave_attach(uv_loop_t *loop, datahub_t *hub, bus_orchestrator_t *orch);
void dcu_north_cloud_attach(datahub_t *hub);
void dcu_north_stop(void);
void dcu_north_destroy(void);
northbound_mgr_t *dcu_north_mgr(void);

#endif /* DCU_NORTH_ADAPTER_H */
