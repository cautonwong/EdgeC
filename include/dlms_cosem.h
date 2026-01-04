#ifndef DLMS_COSEM_H
#define DLMS_COSEM_H

#include <stdint.h>
#include <stdbool.h>
#include "net_buf.h"
#include "dlms_codec.h"

// --- Service Tags ---
#define DLMS_SERVICE_AARQ 96
#define DLMS_SERVICE_AARE 97
#define DLMS_SERVICE_GET_REQUEST 192
#define DLMS_SERVICE_GET_RESPONSE 196
#define DLMS_TAG_GBT 0xE0

typedef enum {
    COSEM_STATE_IDLE,
    COSEM_STATE_ASSOCIATING,
    COSEM_STATE_ASSOCIATED
} dlms_cosem_state_t;

typedef struct {
    dlms_cosem_state_t state;
    uint16_t max_pdu_send;
    struct { bool active; uint32_t current_block; } classic_block;
    struct { bool active; uint32_t expected_block; uint8_t window_size; } gbt;
    net_buf_t *accumulated_buf;
} dlms_cosem_ctx_t;

void dlms_cosem_reset(dlms_cosem_ctx_t *ctx);
int dlms_cosem_build_aarq(dlms_cosem_ctx_t *ctx, net_buf_t *out);
int dlms_cosem_parse_aare(dlms_cosem_ctx_t *ctx, struct net_buf_simple *in);
int dlms_cosem_build_get_request(dlms_cosem_ctx_t *ctx, uint16_t class_id, const char *obis, uint8_t attr_id, net_buf_t *out);
int dlms_cosem_process_apdu(dlms_cosem_ctx_t *ctx, net_buf_t *apdu);
int dlms_cosem_handle_get_response(dlms_cosem_ctx_t *ctx, struct net_buf_simple *apdu);
int dlms_cosem_build_get_next(dlms_cosem_ctx_t *ctx, net_buf_t *out);

#endif