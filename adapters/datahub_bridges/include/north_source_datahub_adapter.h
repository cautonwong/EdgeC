#ifndef NORTH_SOURCE_DATAHUB_ADAPTER_H
#define NORTH_SOURCE_DATAHUB_ADAPTER_H

#include "northbound_mgr.h"

#ifdef __cplusplus
extern "C" {
#endif

struct datahub_s;

nb_source_ops_t northbound_source_from_datahub(struct datahub_s *hub);

#ifdef __cplusplus
}
#endif

#endif /* NORTH_SOURCE_DATAHUB_ADAPTER_H */
