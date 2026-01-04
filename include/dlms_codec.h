#ifndef DLMS_CODEC_H
#define DLMS_CODEC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "net_buf.h"

#define DLMS_MAX_NESTING 16

typedef enum {
    DLMS_TYPE_NULL = 0,
    DLMS_TYPE_ARRAY = 1,
    DLMS_TYPE_STRUCTURE = 2,
    DLMS_TYPE_BOOLEAN = 3,
    DLMS_TYPE_BIT_STRING = 4,
    DLMS_TYPE_INT32 = 5,
    DLMS_TYPE_UINT32 = 6,
    DLMS_TYPE_OCTET_STRING = 9,
    DLMS_TYPE_VISIBLE_STRING = 10,
    DLMS_TYPE_UTF8_STRING = 12,
    DLMS_TYPE_INT8 = 15,
    DLMS_TYPE_INT16 = 16,
    DLMS_TYPE_UINT8 = 17,
    DLMS_TYPE_UINT16 = 18,
    DLMS_TYPE_INT64 = 20,
    DLMS_TYPE_UINT64 = 21,
    DLMS_TYPE_ENUM = 22,
    DLMS_TYPE_FLOAT32 = 23,
    DLMS_TYPE_FLOAT64 = 24
} dlms_data_type_t;

typedef struct {
    uint16_t head_offset;
    uint16_t data_offset;
} dlms_enc_frame_t;

typedef struct {
    struct net_buf_simple *buf;
    dlms_enc_frame_t stack[DLMS_MAX_NESTING];
    int depth;
} dlms_encoder_t;

typedef struct {
    dlms_data_type_t type;
    size_t length;
    uint8_t *payload;
} dlms_variant_t;

typedef struct {
    struct net_buf_simple *buf;
} dlms_decoder_t;

// --- APIs ---

void dlms_encoder_init(dlms_encoder_t *enc, struct net_buf_simple *buf);
void dlms_encode_begin(dlms_encoder_t *enc, dlms_data_type_t type);
void dlms_encode_end(dlms_encoder_t *enc);
void dlms_encode_u8(dlms_encoder_t *enc, uint8_t val);
void dlms_encode_u16(dlms_encoder_t *enc, uint16_t val);
void dlms_encode_octet_string(dlms_encoder_t *enc, const uint8_t *data, size_t len);

void dlms_decoder_init(dlms_decoder_t *dec, struct net_buf_simple *buf);
int dlms_decode_variant(dlms_decoder_t *dec, dlms_variant_t *out);

#endif
