#ifndef BUS_ORCHESTRATOR_H
#define BUS_ORCHESTRATOR_H

#include "datahub.h"
#include "edge_driver.h"

struct bus_orchestrator_s;
typedef struct bus_orchestrator_s bus_orchestrator_t;

bus_orchestrator_t *bus_orchestrator_create(edge_driver_t *driver, datahub_t *hub);
void bus_orchestrator_run(bus_orchestrator_t *ctx);
void bus_orchestrator_destroy(bus_orchestrator_t *ctx);

#endif