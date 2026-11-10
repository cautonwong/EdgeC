#ifndef RADAR_DATAHUB_ADAPTER_H
#define RADAR_DATAHUB_ADAPTER_H

#include "radar_bus_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct datahub_s datahub_t;

radar_bus_port_t radar_datahub_port(datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif /* RADAR_DATAHUB_ADAPTER_H */
