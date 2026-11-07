#ifndef PROTOCOL_BRIDGE_DATAHUB_ADAPTER_H
#define PROTOCOL_BRIDGE_DATAHUB_ADAPTER_H

#include "protocol_bridge_bus_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct datahub_s datahub_t;

protocol_bridge_bus_port_t protocol_bridge_datahub_port(datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_BRIDGE_DATAHUB_ADAPTER_H */
