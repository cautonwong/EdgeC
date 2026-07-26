#include <stdio.h>
#include <assert.h>
#include "edge_acl.h"

int main(void) {
    printf("[TEST] Testing Edge Firewall ACL Rule Engine...\n");

    edge_acl_engine_t acl;
    edge_acl_init(&acl, ACL_ACTION_DROP);

    // Allow subnet 192.168.1.0/24 on port 8080 TCP
    uint32_t ip_sub = (192 << 24) | (168 << 16) | (1 << 8) | 0;
    uint32_t mask   = (255 << 24) | (255 << 16) | (255 << 8) | 0;

    int ret = edge_acl_add_rule(&acl, ip_sub, mask, 8080, 6, ACL_ACTION_ALLOW);
    assert(ret == 0);

    // Test match 192.168.1.50:8080 TCP -> ALLOW
    uint32_t client1 = (192 << 24) | (168 << 16) | (1 << 8) | 50;
    acl_action_t act1 = edge_acl_match(&acl, client1, 8080, 6);
    assert(act1 == ACL_ACTION_ALLOW);

    // Test match 10.0.0.1:8080 TCP -> DROP (default)
    uint32_t client2 = (10 << 24) | (0 << 16) | (0 << 8) | 1;
    acl_action_t act2 = edge_acl_match(&acl, client2, 8080, 6);
    assert(act2 == ACL_ACTION_DROP);

    printf("[TEST] Edge Firewall ACL Rule Engine PASSED!\n");
    return 0;
}
