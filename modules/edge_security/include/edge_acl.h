#ifndef EDGE_ACL_H
#define EDGE_ACL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>

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

typedef struct edge_acl_engine edge_acl_engine_t;

/**
 * @brief ACL 引擎调用方预分配存储容器 (Caller-Allocated Storage - AEP-752)
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[1024];
} edge_acl_engine_storage_t;

/**
 * @brief 在调用方提供的内存就地构造 ACL 引擎聚合根 (Zero-Alloc)
 * @param store 调用方分配的存储块
 * @param default_action 默认规则动作
 * @return 构造成功的聚合根指针, 失败返回 NULL
 */
edge_acl_engine_t *edge_acl_engine_create_in(edge_acl_engine_storage_t *store, acl_action_t default_action);

void edge_acl_init(edge_acl_engine_t *acl, acl_action_t default_action);

int edge_acl_add_rule(edge_acl_engine_t *acl, uint32_t src_ip, uint32_t src_mask, uint16_t port, uint8_t protocol, acl_action_t action);

acl_action_t edge_acl_match(const edge_acl_engine_t *acl, uint32_t src_ip, uint16_t port, uint8_t protocol);

#ifdef __cplusplus
}
#endif

#endif // EDGE_ACL_H
