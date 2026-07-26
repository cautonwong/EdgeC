#ifndef EDGE_ACL_H
#define EDGE_ACL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ACL_RULES 32

typedef enum {
    ACL_ACTION_DROP = 0,
    ACL_ACTION_ALLOW = 1
} acl_action_t;

typedef struct {
    uint32_t src_ip;
    uint32_t src_mask;
    uint16_t port;
    uint8_t protocol; // 6=TCP, 17=UDP, 0=ANY
    acl_action_t action;
} acl_rule_t;

typedef struct {
    acl_rule_t rules[MAX_ACL_RULES];
    size_t rule_count;
    acl_action_t default_action;
} edge_acl_engine_t;

void edge_acl_init(edge_acl_engine_t *acl, acl_action_t default_action);

int edge_acl_add_rule(edge_acl_engine_t *acl, uint32_t src_ip, uint32_t src_mask, uint16_t port, uint8_t protocol, acl_action_t action);

acl_action_t edge_acl_match(const edge_acl_engine_t *acl, uint32_t src_ip, uint16_t port, uint8_t protocol);

#ifdef __cplusplus
}
#endif

#endif // EDGE_ACL_H
