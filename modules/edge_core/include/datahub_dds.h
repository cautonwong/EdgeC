#ifndef EDGE_DATAHUB_DDS_H
#define EDGE_DATAHUB_DDS_H

#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief [ADR-0005] Cyclone DDS Backend VTable Structure
 */
extern const datahub_ops_t g_datahub_dds_ops;

/**
 * @brief Initialize Cyclone DDS DataHub Backend Instance
 */
int datahub_dds_init(datahub_t *hub, const datahub_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_DATAHUB_DDS_H */
