#include "edge_acl.h"
#include <string.h>

struct edge_acl_engine {
    acl_rule_t rules[MAX_ACL_RULES];
    size_t rule_count;
    acl_action_t default_action;
};

_Static_assert(sizeof(struct edge_acl_engine) <= sizeof(edge_acl_engine_storage_t),
               "edge_acl_engine_storage_t size is insufficient");

edge_acl_engine_t *edge_acl_engine_create_in(edge_acl_engine_storage_t *store, acl_action_t default_action) {
    if (!store) {
        return NULL;
    }
    edge_acl_engine_t *acl = (edge_acl_engine_t *)store->raw;
    memset(acl, 0, sizeof(*acl));
    acl->default_action = default_action;
    return acl;
}

void edge_acl_init(edge_acl_engine_t *acl, acl_action_t default_action) {
    if (!acl) return;
    memset(acl, 0, sizeof(struct edge_acl_engine));
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
