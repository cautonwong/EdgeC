#ifndef EDGE_MCP_DATAHUB_ADAPTER_H
#define EDGE_MCP_DATAHUB_ADAPTER_H

#include "edgeos/edge_mcp_bus_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct datahub_s datahub_t;

edge_mcp_bus_port_t edge_mcp_datahub_port(datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_MCP_DATAHUB_ADAPTER_H */
