#ifndef EDGE_DATAHUB_MQTT_H
#define EDGE_DATAHUB_MQTT_H

#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief [ADR-0005] Paho MQTT DataHub Backend VTable Structure
 */
extern const datahub_ops_t g_datahub_mqtt_ops;

/**
 * @brief Initialize Paho MQTT DataHub Backend Instance
 */
int datahub_mqtt_init(datahub_t *hub, const datahub_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_DATAHUB_MQTT_H */
