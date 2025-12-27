#ifndef BUS_ORCHESTRATOR_H
#define BUS_ORCHESTRATOR_H

#include "arbiter.h"
#include "edge_driver.h"
#include "shmring.h"

int edge_request_read(arbiter_t *arb, arbiter_bus_t bus, edge_driver_t *driver,
                      edge_point_config_t *point, uint32_t slave_id, shm_ring_t *ring);

int edge_request_write(arbiter_t *arb, arbiter_bus_t bus, edge_driver_t *driver,
                       edge_point_config_t *point, uint32_t slave_id, shm_ring_t *ring,
                       bool emergency);

#endif