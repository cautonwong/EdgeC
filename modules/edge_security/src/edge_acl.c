#include "edge_acl.h"
#include <string.h>

void edge_acl_init(edge_acl_engine_t *acl, acl_action_t default_action) {
    if (!acl) return;
    memset(acl, 0, sizeof(edge_acl_engine_t));
    acl->default_action = default_action;
}

int edge_acl_add_rule(edge_acl_engine_t *acl, uint32_t src_ip, uint32_t src_mask, uint16_t port, uint8_t protocol, acl_action_t action) {
    if (!acl || acl->rule_count >= MAX_ACL_RULES) {
        return -1;
    }

    acl_rule_t *rule = &acl->rules[acl->rule_count++];
    rule->src_ip = src_ip;
    rule->src_mask = src_mask;
    rule->port = port;
    rule->protocol = protocol;
    rule->action = action;

    return 0;
}

acl_action_t edge_acl_match(const edge_acl_engine_t *acl, uint32_t src_ip, uint16_t port, uint8_t protocol) {
    if (!acl) return ACL_ACTION_DROP;

    for (size_t i = 0; i < acl->rule_count; i++) {
        const acl_rule_t *rule = &acl->rules[i];

        if (rule->src_mask != 0) {
            if ((src_ip & rule->src_mask) != (rule->src_ip & rule->src_mask)) {
                continue;
            }
        }

        if (rule->port != 0 && rule->port != port) {
            continue;
        }

        if (rule->protocol != 0 && rule->protocol != protocol) {
            continue;
        }

        return rule->action;
    }

    return acl->default_action;
}
