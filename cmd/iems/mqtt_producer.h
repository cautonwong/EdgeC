#ifndef MQTT_PRODUCER_H
#define MQTT_PRODUCER_H

#include "northbound_mgr.h"

// 物理实现层对外暴露的唯一接口
nb_transport_ops_t mqtt_producer_create(const char *url, const char *client_id);
void mqtt_producer_destroy(nb_transport_ops_t *ops);

#endif
