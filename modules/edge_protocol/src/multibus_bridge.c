#include "multibus_bridge.h"
#include <string.h>

void multibus_bridge_init(multibus_bridge_t *bridge) {
    if (!bridge) return;
    memset(bridge, 0, sizeof(multibus_bridge_t));
}

int multibus_bridge_add_rule(multibus_bridge_t *bridge, uint32_t rule_id, multibus_type_t src_bus, uint32_t src_addr, multibus_type_t dst_bus, uint32_t dst_addr, double scale, double offset) {
    if (!bridge || bridge->rule_count >= MULTIBUS_MAX_RULES) return -1;

    bus_route_rule_t *r = &bridge->rules[bridge->rule_count++];
    r->rule_id = rule_id;
    r->src_bus = src_bus;
    r->src_address = src_addr;
    r->dst_bus = dst_bus;
    r->dst_address = dst_addr;
    r->scale = (scale == 0.0) ? 1.0 : scale;
    r->offset = offset;
    r->enabled = true;
    return 0;
}

int multibus_bridge_route_signal(multibus_bridge_t *bridge, multibus_type_t src_bus, uint32_t src_addr, double raw_val, multibus_type_t *out_dst_bus, uint32_t *out_dst_addr, double *out_translated_val) {
    if (!bridge || !out_dst_bus || !out_dst_addr || !out_translated_val) return -1;

    for (uint32_t i = 0; i < bridge->rule_count; i++) {
        const bus_route_rule_t *r = &bridge->rules[i];
        if (r->enabled && r->src_bus == src_bus && r->src_address == src_addr) {
            *out_dst_bus = r->dst_bus;
            *out_dst_addr = r->dst_address;
            *out_translated_val = (raw_val * r->scale) + r->offset;
            bridge->total_routed++;
            return 0;
        }
    }

    bridge->route_errors++;
    return -2;
}

void multibus_bridge_get_stats(const multibus_bridge_t *bridge, uint64_t *total_routed, uint64_t *route_errors) {
    if (!bridge) return;
    if (total_routed) *total_routed = bridge->total_routed;
    if (route_errors) *route_errors = bridge->route_errors;
}
