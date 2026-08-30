/**
 * @file edge_anti_corruption.h
 * @brief Anti-Corruption Layer (ACL) translation engine for protocol-to-domain decoupling
 */

#ifndef EDGE_ANTI_CORRUPTION_H
#define EDGE_ANTI_CORRUPTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include "contract_data_types.h"
#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EDGE_PROTO_MODBUS = 0,
    EDGE_PROTO_DLT645 = 1,
    EDGE_PROTO_IEC104 = 2,
    EDGE_PROTO_CAN    = 3,
    EDGE_PROTO_CUSTOM = 4
} edge_protocol_type_t;

typedef enum {
    EDGE_ACL_TYPE_FLOAT  = 0,
    EDGE_ACL_TYPE_INT32  = 1,
    EDGE_ACL_TYPE_UINT32 = 2,
    EDGE_ACL_TYPE_BOOL   = 3,
    EDGE_ACL_TYPE_DOUBLE = 4
} edge_acl_data_type_t;

typedef struct {
    uint32_t tag_id;
    char domain_name[32];
    char entity_prop[64];
    edge_protocol_type_t proto;
    uint32_t raw_address;
    double scaler;
    double offset;
    edge_acl_data_type_t data_type;
} edge_anti_corruption_entry_t;

typedef struct {
    edge_anti_corruption_entry_t *entries;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
    bool initialized;
} edge_anti_corruption_engine_t;

int edge_anti_corruption_init(edge_anti_corruption_engine_t *acl, size_t capacity);
void edge_anti_corruption_destroy(edge_anti_corruption_engine_t *acl);

int edge_anti_corruption_register(edge_anti_corruption_engine_t *acl, const edge_anti_corruption_entry_t *entry);
int edge_anti_corruption_register_table(edge_anti_corruption_engine_t *acl, const edge_anti_corruption_entry_t *entries, size_t count);

const edge_anti_corruption_entry_t *edge_anti_corruption_lookup_by_tag(edge_anti_corruption_engine_t *acl, uint32_t tag_id);
const edge_anti_corruption_entry_t *edge_anti_corruption_lookup_by_address(edge_anti_corruption_engine_t *acl, edge_protocol_type_t proto, uint32_t raw_address);

int edge_anti_corruption_translate_inbound(edge_anti_corruption_engine_t *acl, edge_protocol_type_t proto, uint32_t raw_address, double raw_value, pipe_msg_t *out_msg);
int edge_anti_corruption_translate_outbound(edge_anti_corruption_engine_t *acl, uint32_t tag_id, double physical_value, uint32_t *out_raw_address, double *out_raw_value);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_ANTI_CORRUPTION_H */
