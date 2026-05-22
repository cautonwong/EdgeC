#include "dlms_codec.h"

#include <assert.h>
#include <string.h>

#include "edge_log.h"

void dlms_encoder_init(dlms_encoder_t *enc, struct net_buf_simple *buf) {
    memset(enc, 0, sizeof(dlms_encoder_t));
    enc->buf = buf;
}

static void _write_ber_len(uint8_t *ptr, size_t len, int sz) {
    if (sz == 1)
        *ptr = (uint8_t)len;
    else if (sz == 2) {
        *ptr++ = 0x81;
        *ptr = (uint8_t)len;
    } else {
        *ptr++ = 0x82;
        *ptr++ = (uint8_t)(len >> 8);
        *ptr = (uint8_t)(len & 0xFF);
    }
}

void dlms_encode_begin(dlms_encoder_t *enc, dlms_data_type_t type) {
    if (enc->depth >= DLMS_MAX_NESTING) return;
    if (net_buf_simple_tailroom(enc->buf) < 2) {
        LOG_ERROR(LOG_CAT_IO, "DLMS Encoder: Buffer overflow in begin");
        return;
    }
    net_buf_simple_add_u8(enc->buf, (uint8_t)type);
    dlms_enc_frame_t *f = &enc->stack[enc->depth++];
    f->head_offset = (uint16_t)(enc->buf->len - 1);
    net_buf_simple_add(enc->buf, 1);
    f->data_offset = (uint16_t)enc->buf->len;
}

void dlms_encode_end(dlms_encoder_t *enc) {
    if (enc->depth == 0) return;
    dlms_enc_frame_t *f = &enc->stack[--enc->depth];
    uint8_t *len_head_ptr = enc->buf->data + f->head_offset + 1;
    uint8_t *data_start_ptr = enc->buf->data + f->data_offset;
    size_t data_len = (size_t)(enc->buf->data + enc->buf->len - data_start_ptr);
    int sz = (data_len < 128) ? 1 : (data_len < 256 ? 2 : 3);
    if (sz > 1) {
        int shift = sz - 1;
        if (enc->buf->len + shift > enc->buf->size) {
            LOG_ERROR(LOG_CAT_IO, "DLMS Encoder: Buffer overflow during shifting!");
            return;
        }
        memmove(data_start_ptr + shift, data_start_ptr, data_len);
        enc->buf->len = (uint16_t)(enc->buf->len + shift);
    }
    _write_ber_len(len_head_ptr, data_len, sz);
}

void dlms_encode_u8(dlms_encoder_t *enc, uint8_t val) {
    if (net_buf_simple_tailroom(enc->buf) < 2) return;
    net_buf_simple_add_u8(enc->buf, (uint8_t)DLMS_TYPE_UINT8);
    net_buf_simple_add_u8(enc->buf, val);
}

void dlms_encode_u16(dlms_encoder_t *enc, uint16_t val) {
    if (net_buf_simple_tailroom(enc->buf) < 3) return;
    net_buf_simple_add_u8(enc->buf, (uint8_t)DLMS_TYPE_UINT16);
    net_buf_simple_add_be16(enc->buf, val);
}

void dlms_encode_octet_string(dlms_encoder_t *enc, const uint8_t *data, size_t len) {
    dlms_encode_begin(enc, DLMS_TYPE_OCTET_STRING);
    if (net_buf_simple_tailroom(enc->buf) < len) {
        LOG_ERROR(LOG_CAT_IO, "DLMS Encoder: OctetString too long");
        return;
    }
    net_buf_simple_add_mem(enc->buf, data, len);
    dlms_encode_end(enc);
}

void dlms_decoder_init(dlms_decoder_t *dec, struct net_buf_simple *buf) {
    dec->buf = buf;
}

int dlms_decode_variant(dlms_decoder_t *dec, dlms_variant_t *out) {
    if (dec->buf->len == 0) return -1;
    out->type = (dlms_data_type_t)net_buf_simple_pull_u8(dec->buf);
    switch (out->type) {
        case DLMS_TYPE_UINT8:
        case DLMS_TYPE_ENUM:
            out->length = 1;
            out->payload = net_buf_simple_pull(dec->buf, 1);
            break;
        case DLMS_TYPE_UINT16:
            out->length = 2;
            out->payload = net_buf_simple_pull(dec->buf, 2);
            break;
        case DLMS_TYPE_OCTET_STRING:
        case DLMS_TYPE_VISIBLE_STRING:
        case DLMS_TYPE_STRUCTURE:
        case DLMS_TYPE_ARRAY: {
            if (dec->buf->len == 0) return -1;
            uint8_t l = net_buf_simple_pull_u8(dec->buf);
            if (l < 128)
                out->length = l;
            else if (l == 0x81) {
                if (dec->buf->len < 1) return -1;
                out->length = net_buf_simple_pull_u8(dec->buf);
            } else if (l == 0x82) {
                if (dec->buf->len < 2) return -1;
                out->length = net_buf_simple_pull_be16(dec->buf);
            }
            if (out->type == DLMS_TYPE_OCTET_STRING || out->type == DLMS_TYPE_VISIBLE_STRING) {
                if (dec->buf->len < out->length) return -3;
                out->payload = net_buf_simple_pull(dec->buf, out->length);
            } else
                out->payload = dec->buf->data;
        } break;
        default:
            return -2;
    }
    return 0;
}
