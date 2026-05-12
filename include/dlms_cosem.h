#ifndef DLMS_COSEM_H
#define DLMS_COSEM_H

#include <stdbool.h>
#include <stdint.h>

#include "dlms_codec.h"
#include "net_buf.h"

// --- Phase 4: 架构解耦配置 ---
// 为链路层(HDLC/Wrapper)预留的最大头部空隙
#define DLMS_MAX_LINK_HEADER 16

// --- Service Tags ---
#define DLMS_SERVICE_AARQ 96
#define DLMS_SERVICE_AARE 97
#define DLMS_SERVICE_GET_REQUEST 192
#define DLMS_SERVICE_GET_RESPONSE 196
#define DLMS_SERVICE_SET_REQUEST 193
#define DLMS_SERVICE_SET_RESPONSE 197
#define DLMS_TAG_GBT 0xE0

typedef enum {
    COSEM_STATE_IDLE,
    COSEM_STATE_ASSOCIATING,
    COSEM_STATE_ASSOCIATED
} dlms_cosem_state_t;

typedef struct {
    dlms_cosem_state_t state;
    uint16_t max_pdu_send;
    struct {
        bool active;
        uint32_t current_block;
    } classic_block;
    struct {
        bool active;
        uint32_t expected_block;
        uint8_t window_size;
    } gbt;
    net_buf_t *accumulated_buf;
} dlms_cosem_ctx_t;

void dlms_cosem_reset(dlms_cosem_ctx_t *ctx);
int dlms_cosem_build_aarq(dlms_cosem_ctx_t *ctx, net_buf_t *out);
int dlms_cosem_parse_aare(dlms_cosem_ctx_t *ctx, struct net_buf_simple *in);
int dlms_cosem_build_get_request(dlms_cosem_ctx_t *ctx, uint16_t class_id, const char *obis,
                                 uint8_t attr_id, net_buf_t *access_params, net_buf_t *out);
int dlms_cosem_process_apdu(dlms_cosem_ctx_t *ctx, net_buf_t *apdu);
int dlms_cosem_decode_response(dlms_cosem_ctx_t *ctx, struct net_buf_simple *apdu,
                               dlms_variant_t *out);

/**
 * @brief 语义化响应解码：直接根据预期类型解析并返回数值
 */
int dlms_cosem_decode_response_semantic(dlms_cosem_ctx_t *ctx, struct net_buf_simple *apdu,
                                        uint8_t expected_type, double *out_val);
int dlms_cosem_build_get_next(dlms_cosem_ctx_t *ctx, net_buf_t *out);

/* --- 从站逻辑 (Slave Role) --- */
int dlms_cosem_parse_aarq(dlms_cosem_ctx_t *ctx, struct net_buf_simple *in);
int dlms_cosem_build_aare(dlms_cosem_ctx_t *ctx, net_buf_t *out);
int dlms_cosem_parse_get_request(struct net_buf_simple *in, uint16_t *class_id, uint8_t *obis,
                                 uint8_t *attr_id);
int dlms_cosem_build_get_response(dlms_cosem_ctx_t *ctx, dlms_data_type_t type, const void *val,
                                  size_t val_len, net_buf_t *out);

/**
 * @brief 构造 Selective Access 描述符 (目前支持 Selector 1: Range Descriptor)
 * @param start_time 起始时间 (DLMS DateTime 12字节格式)
 * @param end_time   结束时间
 * @param out        输出 buffer
 */
int dlms_cosem_encode_range_descriptor(const uint8_t *start_time, const uint8_t *end_time,
                                       net_buf_t *out);

#endif