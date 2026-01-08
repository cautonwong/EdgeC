#include "dlms_cosem.h"

#include <stdio.h>
#include <string.h>

#include "dlms_codec.h"
#include "edge_log.h"

void dlms_cosem_reset(dlms_cosem_ctx_t *ctx) {
    if (!ctx) return;
    if (ctx->accumulated_buf) {
        net_buf_unref(ctx->accumulated_buf);
        ctx->accumulated_buf = NULL;
    }
    ctx->state = COSEM_STATE_IDLE;
    ctx->classic_block.active = false;
    ctx->gbt.active = false;
}

int dlms_cosem_build_aarq(dlms_cosem_ctx_t *ctx, net_buf_t *out) {
    dlms_encoder_t enc;
    dlms_encoder_init(&enc, &out->b);
    net_buf_simple_add_u8(enc.buf, (uint8_t)DLMS_SERVICE_AARQ);
    dlms_encode_begin(&enc, DLMS_TYPE_STRUCTURE);
    uint8_t app_ctx[] = {0x60, 0x85, 0x74, 0x05, 0x08, 0x01, 0x01};
    dlms_encode_octet_string(&enc, app_ctx, sizeof(app_ctx));
    dlms_encode_begin(&enc, DLMS_TYPE_OCTET_STRING);
    net_buf_simple_add_u8(enc.buf, 0x01);
    net_buf_simple_add_u8(enc.buf, 0x00);
    net_buf_simple_add_u8(enc.buf, 0x00);
    net_buf_simple_add_u8(enc.buf, 0x06);
    uint8_t conf[] = {0x5F, 0x1F, 0x04, 0x00, 0x00, 0x7E, 0x1F, 0x04};
    net_buf_simple_add_mem(enc.buf, conf, sizeof(conf));
    net_buf_simple_add_be16(enc.buf, 1024);
    dlms_encode_end(&enc);
    dlms_encode_end(&enc);
    ctx->state = COSEM_STATE_ASSOCIATING;
    return 0;
}

int dlms_cosem_parse_aare(dlms_cosem_ctx_t *ctx, struct net_buf_simple *in) {
    if (in->len == 0) return -1;
    uint8_t tag = net_buf_simple_pull_u8(in);
    if (tag != DLMS_SERVICE_AARE) return -1;

    dlms_decoder_t dec;
    dlms_decoder_init(&dec, in);
    dlms_variant_t var;
    // AARE 顶层通常是 Structure (0x02)
    if (dlms_decode_variant(&dec, &var) != 0 || var.type != DLMS_TYPE_STRUCTURE) {
        LOG_WARN(LOG_CAT_IO, "DLMS: Invalid AARE structure (Tag=%d)", var.type);
        return -1;
    }

    ctx->state = COSEM_STATE_ASSOCIATED;
    ctx->max_pdu_send = 1024;
    LOG_INFO(LOG_CAT_IO, "DLMS: COSEM Associated.");
    return 0;
}

int dlms_cosem_build_get_request(dlms_cosem_ctx_t *ctx, uint16_t class_id, const char *obis,
                                 uint8_t attr_id, net_buf_t *out) {
    if (ctx->state != COSEM_STATE_ASSOCIATED) return -1;
    struct net_buf_simple *b = &out->b;
    net_buf_simple_add_u8(b, (uint8_t)DLMS_SERVICE_GET_REQUEST);
    net_buf_simple_add_u8(b, 0x01);
    net_buf_simple_add_u8(b, 0x01);
    net_buf_simple_add_be16(b, class_id);
    uint8_t obis_bytes[6];
    int p[6];
    if (sscanf(obis, "%d.%d.%d.%d.%d.%d", &p[0], &p[1], &p[2], &p[3], &p[4], &p[5]) == 6) {
        for (int i = 0; i < 6; i++) obis_bytes[i] = (uint8_t)p[i];
        net_buf_simple_add_mem(b, obis_bytes, 6);
    }
    net_buf_simple_add_u8(b, attr_id);
    net_buf_simple_add_u8(b, 0x00);
    return 0;
}

int dlms_cosem_decode_get_response(dlms_cosem_ctx_t *ctx, struct net_buf_simple *apdu,
                                   dlms_variant_t *out) {
    (void)ctx;
    dlms_decoder_t dec;
    dlms_decoder_init(&dec, apdu);
    net_buf_simple_pull_u8(apdu);  // Skip tag
    net_buf_simple_pull_u8(apdu);  // Skip invoke_id
    return dlms_decode_variant(&dec, out);
}

int dlms_cosem_build_get_next(dlms_cosem_ctx_t *ctx, net_buf_t *out) {
    struct net_buf_simple *b = &out->b;
    net_buf_simple_add_u8(b, (uint8_t)DLMS_SERVICE_GET_REQUEST);
    net_buf_simple_add_u8(b, 0x02);
    net_buf_simple_add_u8(b, 0x01);
    net_buf_simple_add_be32(b, ctx->classic_block.current_block + 1);
    return 0;
}