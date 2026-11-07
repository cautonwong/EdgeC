#ifndef MULTIBUS_BRIDGE_H
#define MULTIBUS_BRIDGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MULTIBUS_MAX_RULES 64

typedef enum {
    BUS_TYPE_UNKNOWN = 0,
    BUS_TYPE_MODBUS_TCP = 1,
    BUS_TYPE_MODBUS_RTU = 2,
    BUS_TYPE_CAN = 3,
    BUS_TYPE_IEC104 = 4,
    BUS_TYPE_DATAHUB_SHM = 5
} multibus_type_t;

typedef struct {
    uint32_t rule_id;
    multibus_type_t src_bus;
    uint32_t src_address;   /* Register addr / CAN ID / ASDU point */
    multibus_type_t dst_bus;
    uint32_t dst_address;
    double scale;
    double offset;
    bool enabled;
} bus_route_rule_t;

typedef struct {
    bus_route_rule_t rules[MULTIBUS_MAX_RULES];
    uint32_t rule_count;
    uint64_t total_routed;
    uint64_t route_errors;
} multibus_bridge_t;

void multibus_bridge_init(multibus_bridge_t *bridge);
int multibus_bridge_add_rule(multibus_bridge_t *bridge, uint32_t rule_id, multibus_type_t src_bus, uint32_t src_addr, multibus_type_t dst_bus, uint32_t dst_addr, double scale, double offset);
int multibus_bridge_route_signal(multibus_bridge_t *bridge, multibus_type_t src_bus, uint32_t src_addr, double raw_val, multibus_type_t *out_dst_bus, uint32_t *out_dst_addr, double *out_translated_val);
void multibus_bridge_get_stats(const multibus_bridge_t *bridge, uint64_t *total_routed, uint64_t *route_errors);

#ifdef __cplusplus
}
#endif

#endif /* MULTIBUS_BRIDGE_H */
