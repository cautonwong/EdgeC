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
    net_buf_reserve(out, DLMS_MAX_LINK_HEADER);
    dlms_encoder_t enc;
    dlms_encoder_init(&enc, &out->b);
    net_buf_simple_add_u8(enc.buf, (uint8_t)DLMS_SERVICE_AARQ);
    
    // [Aetheris] 构造符合 BlueBook 标准的 AARQ
    // 应用上下文: LN without ciphering
    uint8_t app_ctx[] = {0x60, 0x85, 0x74, 0x05, 0x08, 0x01, 0x01};
    dlms_encode_octet_string(&enc, app_ctx, sizeof(app_ctx));
    
    // User Information: xDLMS InitiatRequest
    dlms_encode_begin(&enc, DLMS_TYPE_OCTET_STRING);
    net_buf_simple_add_u8(enc.buf, 0x01); // tag: initiateRequest
    net_buf_simple_add_u8(enc.buf, 0x00); // dedicated-key: none
    net_buf_simple_add_u8(enc.buf, 0x00); // response-allowed: false
    net_buf_simple_add_u8(enc.buf, 0x06); // proposed-quality-of-service: none
    net_buf_simple_add_u8(enc.buf, 0x5F); // proposed-dlms-version-number: 6
    
    // Proposed conformance (GET, SET, ACTION, etc)
    uint8_t conformance[] = {0x1F, 0x04, 0x00}; // 举例：支持基本读写
    net_buf_simple_add_mem(enc.buf, conformance, sizeof(conformance));
    
    net_buf_simple_add_be16(enc.buf, 1024); // client-max-receive-pdu-size
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
                                 uint8_t attr_id, net_buf_t *access_params, net_buf_t *out) {
    if (ctx->state != COSEM_STATE_ASSOCIATED) return -1;
    net_buf_reserve(out, DLMS_MAX_LINK_HEADER);
    struct net_buf_simple *b = &out->b;
    net_buf_simple_add_u8(b, (uint8_t)DLMS_SERVICE_GET_REQUEST);
    net_buf_simple_add_u8(b, 0x01);  // Normal
    net_buf_simple_add_u8(b, 0xC1);  // Invoke ID
    net_buf_simple_add_be16(b, class_id);

    uint8_t obis_bytes[6];
    int p[6];
    if (sscanf(obis, "%d.%d.%d.%d.%d.%d", &p[0], &p[1], &p[2], &p[3], &p[4], &p[5]) == 6) {
        for (int i = 0; i < 6; i++) obis_bytes[i] = (uint8_t)p[i];
        net_buf_simple_add_mem(b, obis_bytes, 6);
    }
    net_buf_simple_add_u8(b, attr_id);

    if (access_params && access_params->b.len > 0) {
        net_buf_simple_add_u8(b, 0x01);
        net_buf_simple_add_mem(b, access_params->b.data, access_params->b.len);
    } else {
        net_buf_simple_add_u8(b, 0x00);
    }
    return 0;
}

int dlms_cosem_build_set_request(dlms_cosem_ctx_t *ctx, uint16_t class_id, const char *obis,
                                 uint8_t attr_id, const uint8_t *data, size_t dlen,
                                 net_buf_t *out) {
    if (ctx->state != COSEM_STATE_ASSOCIATED) return -1;
    if (dlen > 0 && !data) return -1;

    net_buf_reserve(out, DLMS_MAX_LINK_HEADER);
    struct net_buf_simple *b = &out->b;

    /* SET-Request ::= SEQUENCE { invoke-id, class-id, OBIS, attribute-id,
     *                            access-selection, value } —— 与 GET 同构，尾部换成 value */
    net_buf_simple_add_u8(b, (uint8_t)DLMS_SERVICE_SET_REQUEST);
    net_buf_simple_add_u8(b, 0x01); /* Normal */
    net_buf_simple_add_u8(b, 0xC1); /* Invoke ID */
    net_buf_simple_add_be16(b, class_id);

    uint8_t obis_bytes[6];
    int p[6];
    if (sscanf(obis, "%d.%d.%d.%d.%d.%d", &p[0], &p[1], &p[2], &p[3], &p[4], &p[5]) != 6) {
        return -1;
    }
    for (int i = 0; i < 6; i++) obis_bytes[i] = (uint8_t)p[i];
    net_buf_simple_add_mem(b, obis_bytes, 6);
    net_buf_simple_add_u8(b, attr_id);

    /* 无选择访问；值由调用方给（含类型标签），codec 不猜类型 */
    net_buf_simple_add_u8(b, 0x00);
    if (dlen > 0) net_buf_simple_add_mem(b, data, dlen);
    return 0;
}

int dlms_cosem_decode_response(dlms_cosem_ctx_t *ctx, struct net_buf_simple *apdu,
                               dlms_variant_t *out) {
    if (apdu->len < 1) return -1;
    uint8_t service_tag = net_buf_simple_pull_u8(apdu);

    if (service_tag == DLMS_SERVICE_GET_RESPONSE) {
        uint8_t response_type = net_buf_simple_pull_u8(apdu);
        net_buf_simple_pull_u8(apdu);  // Invoke ID

        if (response_type == 0x01) {  // Normal
            uint8_t result = net_buf_simple_pull_u8(apdu);
            if (result != 0x00) return -2;
            dlms_decoder_t dec;
            dlms_decoder_init(&dec, apdu);
            return dlms_decode_variant(&dec, out);
        } else if (response_type == 0x02) {  // with datablock
            uint8_t last_block = net_buf_simple_pull_u8(apdu);
            uint32_t block_num = net_buf_simple_pull_be32(apdu);
            uint8_t result = net_buf_simple_pull_u8(apdu);
            if (result != 0x00) return -3;

            dlms_decoder_t dec;
            dlms_decoder_init(&dec, apdu);
            dlms_variant_t block_var;
            if (dlms_decode_variant(&dec, &block_var) != 0) return -4;

            if (block_num == 1) {
                if (ctx->accumulated_buf) net_buf_unref(ctx->accumulated_buf);
                ctx->accumulated_buf = net_buf_alloc(NULL, 0);
            }
            if (ctx->accumulated_buf) {
                net_buf_simple_add_mem(&ctx->accumulated_buf->b, block_var.payload,
                                       block_var.length);
            }

            ctx->classic_block.active = !last_block;
            ctx->classic_block.current_block = block_num;

            if (last_block) {
                dlms_decoder_t final_dec;
                dlms_decoder_init(&final_dec, &ctx->accumulated_buf->b);
                return dlms_decode_variant(&final_dec, out);
            } else {
                return 1;  // Need GET-Next
            }
        }
    } else if (service_tag == DLMS_SERVICE_SET_RESPONSE) {
        /* SET-Response ::= SEQUENCE { invoke-id, result } —— 没有值，成功即 0 */
        net_buf_simple_pull_u8(apdu); /* response type: 01 = normal */
        net_buf_simple_pull_u8(apdu); /* invoke id */
        uint8_t result = net_buf_simple_pull_u8(apdu);
        if (result != 0x00) return -2;
        memset(out, 0, sizeof(*out));
        return 0;
    } else if (service_tag == 194) {  // Event-Notification
        uint8_t time_present = net_buf_simple_pull_u8(apdu);
        if (time_present) net_buf_simple_pull(apdu, 12);
        net_buf_simple_pull_be16(apdu);  // Class ID
        net_buf_simple_pull(apdu, 6);    // OBIS
        net_buf_simple_pull_u8(apdu);    // Attr ID
        dlms_decoder_t dec;
        dlms_decoder_init(&dec, apdu);
        return dlms_decode_variant(&dec, out);
    }

    return -5;
}

static int _variant_to_double(const dlms_variant_t *var, double *out) {
    if (var->type == DLMS_TYPE_UINT8)
        *out = (double)var->payload[0];
    else if (var->type == DLMS_TYPE_UINT16)
        *out = (double)((var->payload[0] << 8) | var->payload[1]);
    else if (var->type == DLMS_TYPE_UINT32)
        *out = (double)((var->payload[0] << 24) | (var->payload[1] << 16) | (var->payload[2] << 8) |
                        var->payload[3]);
    else if (var->type == DLMS_TYPE_INT8)
        *out = (double)((int8_t)var->payload[0]);
    else if (var->type == DLMS_TYPE_INT16)
        *out = (double)((int16_t)((var->payload[0] << 8) | var->payload[1]));
    else if (var->type == DLMS_TYPE_INT32)
        *out = (double)((int32_t)((var->payload[0] << 24) | (var->payload[1] << 16) |
                                  (var->payload[2] << 8) | var->payload[3]));
    else
        return -1;
    return 0;
}

int dlms_cosem_decode_response_semantic(dlms_cosem_ctx_t *ctx, struct net_buf_simple *apdu,
                                        uint8_t expected_type, double *out_val) {
    dlms_variant_t var;
    int ret = dlms_cosem_decode_response(ctx, apdu, &var);
    if (ret != 0) return ret;

    if (expected_type != 0 && var.type != expected_type) return -10;
    return _variant_to_double(&var, out_val);
}

int dlms_cosem_build_get_next(dlms_cosem_ctx_t *ctx, net_buf_t *out) {
    net_buf_reserve(out, DLMS_MAX_LINK_HEADER);
    struct net_buf_simple *b = &out->b;
    net_buf_simple_add_u8(b, (uint8_t)DLMS_SERVICE_GET_REQUEST);
    net_buf_simple_add_u8(b, 0x02);
    net_buf_simple_add_u8(b, 0x01);
    net_buf_simple_add_be32(b, ctx->classic_block.current_block + 1);
    return 0;
}

int dlms_cosem_encode_range_descriptor(const uint8_t *start_time, const uint8_t *end_time,
                                       net_buf_t *out) {
    dlms_encoder_t enc;
    dlms_encoder_init(&enc, &out->b);
    net_buf_simple_add_u8(enc.buf, 0x01);
    dlms_encode_begin(&enc, DLMS_TYPE_STRUCTURE);
    net_buf_simple_add_u8(enc.buf, 4);
    dlms_encode_begin(&enc, DLMS_TYPE_STRUCTURE);
    net_buf_simple_add_u8(enc.buf, 4);
    dlms_encode_u16(&enc, 8);
    uint8_t clock_obis[] = {0, 0, 1, 0, 0, 255};
    dlms_encode_octet_string(&enc, clock_obis, 6);
    net_buf_simple_add_u8(enc.buf, (uint8_t)DLMS_TYPE_INT8);
    net_buf_simple_add_u8(enc.buf, 2);
    dlms_encode_u16(&enc, 0);
    dlms_encode_end(&enc);
    dlms_encode_octet_string(&enc, start_time, 12);
    dlms_encode_octet_string(&enc, end_time, 12);
    dlms_encode_begin(&enc, DLMS_TYPE_ARRAY);
    net_buf_simple_add_u8(enc.buf, 0);
    dlms_encode_end(&enc);
    dlms_encode_end(&enc);
    return 0;
}

/* --- 从站逻辑 (Slave Role) --- */

int dlms_cosem_parse_aarq(dlms_cosem_ctx_t *ctx, struct net_buf_simple *in) {
    if (in->len < 8) return -1;
    uint8_t tag = net_buf_simple_pull_u8(in);
    if (tag != DLMS_SERVICE_AARQ) return -1;
    
    // 简化的 AARQ 解析，查找 Application Context
    // 预期：[0xA1] [len] [0x06] [len] [OID]
    // 正常的 OID 长度为 7
    uint8_t *p = in->data;
    for (int i = 0; i <= (int)in->len - 7; i++) {
        if (p[i] == 0x60 && p[i+1] == 0x85 && p[i+2] == 0x74 && p[i+3] == 0x05 && p[i+4] == 0x08 && p[i+5] == 0x01) {
            if (p[i+6] == 0x01) ctx->security_level = DLMS_SEC_NONE;
            else if (p[i+6] == 0x02) ctx->security_level = DLMS_SEC_AUTHENTICATED_ENCRYPTED;
            break;
        }
    }

    ctx->state = COSEM_STATE_ASSOCIATED;
    return 0;
}

int dlms_cosem_build_aare(dlms_cosem_ctx_t *ctx, net_buf_t *out) {
    (void)ctx;
    net_buf_reserve(out, DLMS_MAX_LINK_HEADER);
    dlms_encoder_t enc;
    dlms_encoder_init(&enc, &out->b);
    net_buf_simple_add_u8(enc.buf, (uint8_t)DLMS_SERVICE_AARE);
    dlms_encode_begin(&enc, DLMS_TYPE_STRUCTURE);
    dlms_encode_u8(&enc, 0);  // result: success
    dlms_encode_end(&enc);
    return 0;
}

int dlms_cosem_parse_get_request(struct net_buf_simple *in, uint16_t *class_id, uint8_t *obis,
                                 uint8_t *attr_id) {
    if (in->len < 10) return -1;
    uint8_t tag = net_buf_simple_pull_u8(in);
    if (tag != DLMS_SERVICE_GET_REQUEST) return -1;
    net_buf_simple_pull_u8(in);
    net_buf_simple_pull_u8(in);
    *class_id = net_buf_simple_pull_be16(in);
    memcpy(obis, net_buf_simple_pull(in, 6), 6);
    *attr_id = net_buf_simple_pull_u8(in);
    return 0;
}

int dlms_cosem_build_get_response(dlms_cosem_ctx_t *ctx, dlms_data_type_t type, const void *val,
                                  size_t val_len, net_buf_t *out) {
    (void)ctx;
    net_buf_reserve(out, DLMS_MAX_LINK_HEADER);
    struct net_buf_simple *b = &out->b;
    net_buf_simple_add_u8(b, (uint8_t)DLMS_SERVICE_GET_RESPONSE);
    net_buf_simple_add_u8(b, 0x01);
    net_buf_simple_add_u8(b, 0xC1);
    net_buf_simple_add_u8(b, 0x00);
    dlms_encoder_t enc;
    dlms_encoder_init(&enc, b);
    if (type == DLMS_TYPE_OCTET_STRING)
        dlms_encode_octet_string(&enc, val, val_len);
    else if (type == DLMS_TYPE_UINT16)
        dlms_encode_u16(&enc, *(uint16_t *)val);
    return 0;
}
