/**
 * @file driver_dlms.c
 * @brief 工业级 DLMS/COSEM V3 驱动 - 专家级最终闭环版 (Peek 修正)
 */

#include "edge_driver.h"
#include "driver_manager.h"
#include "dlms_codec.h"
#include "dlms_hdlc.h"
#include "dlms_cosem.h"
#include "edge_log.h"
#include "edge_common.h"
#include <string.h>
#include <time.h>

typedef struct {
    dlms_hdlc_manager_t hdlc;
    dlms_cosem_ctx_t cosem;
    uv_timer_t t1_timer;
    bool association_pending;
} dlms_driver_ctx_t;

static void _on_t1_timeout(uv_timer_t *handle) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)handle->data;
    LOG_WARN(LOG_CAT_IO, "DLMS: T1 Timeout [LLC:%d, COSEM:%d]", priv->hdlc.state, priv->cosem.state);
}

static int dlms_v3_on_init(edge_driver_ctx_t *ctx) {
    if(!ctx) return -1;
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    memset(priv, 0, sizeof(dlms_driver_ctx_t));
    priv->hdlc.client_addr = 0x01;
    priv->hdlc.server_addr = 0x03;
    dlms_hdlc_reset(&priv->hdlc);
    dlms_cosem_reset(&priv->cosem);
    uv_timer_init(uv_default_loop(), &priv->t1_timer);
    priv->t1_timer.data = priv;
    return 0;
}

static int dlms_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int tag_count, net_buf_t *out) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    if (priv->hdlc.state == HDLC_STATE_WAIT_ACK) return 0;

    if (priv->hdlc.state == HDLC_STATE_DISCONNECTED) {
        struct net_buf_simple *b = &out->b;
        net_buf_simple_add_u8(b, 0x7E);
        net_buf_simple_add_be16(b, 0xA007);
        net_buf_simple_add_u8(b, 0x03<<1|1); net_buf_simple_add_u8(b, 0x01<<1|1);
        net_buf_simple_add_u8(b, 0x83); 
        uint16_t fcs = edge_crc16_ccitt(b->data+1, 5);
        net_buf_simple_add_le16(b, fcs); net_buf_simple_add_u8(b, 0x7E);
        priv->hdlc.state = HDLC_STATE_CONNECTING;
        return 0;
    }

    if (priv->cosem.state != COSEM_STATE_ASSOCIATED) {
        if (!priv->association_pending) {
            net_buf_t *aarq = net_buf_alloc(out->pool, 0);
            if (aarq) {
                dlms_cosem_build_aarq(&priv->cosem, aarq);
                dlms_hdlc_send_iframe(&priv->hdlc, aarq, out);
                net_buf_unref(aarq);
                priv->association_pending = true;
                uv_timer_start(&priv->t1_timer, _on_t1_timeout, 3000, 0);
            }
        }
        return 0;
    }

    if (tag_count > 0) {
        net_buf_t *get_req = net_buf_alloc(out->pool, 0);
        if (get_req) {
            dlms_cosem_build_get_request(&priv->cosem, 3, "1.0.1.8.0.255", 2, get_req);
            dlms_hdlc_send_iframe(&priv->hdlc, get_req, out);
            net_buf_unref(get_req);
            uv_timer_start(&priv->t1_timer, _on_t1_timeout, 3000, 0);
        }
    }
    (void)tag_ids;
    return 0;
}

static int dlms_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, pipe_msg_t *out_msgs, int *msg_count) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;

    if (b->len >= 10 && (b->data[5] == 0x63 || b->data[5] == 0x73)) {
        priv->hdlc.state = HDLC_STATE_CONNECTED;
        return 0;
    }

    net_buf_t *apdu = NULL;
    int ret = dlms_hdlc_depacketize(&priv->hdlc, b, in->pool, &apdu);
    if (ret != 0 || !apdu) return (ret == 1) ? 0 : -1;

    uv_timer_stop(&priv->t1_timer);
    struct net_buf_simple *ab = &apdu->b;
    uint8_t tag = ab->data[0]; // [高质量] 只读不取，让中间件去消耗
    
    if (tag == (uint8_t)DLMS_SERVICE_AARE) {
        if (dlms_cosem_parse_aare(&priv->cosem, ab) == 0) priv->association_pending = false;
    } else if (tag == (uint8_t)DLMS_SERVICE_GET_RESPONSE) {
        if (dlms_cosem_handle_get_response(&priv->cosem, ab) == 0) {
            // [高质量解析] 消耗掉剩余的 Result 头 (0x00)
            if (ab->len > 0) net_buf_simple_pull_u8(ab);
            dlms_decoder_t dec; dlms_decoder_init(&dec, ab);
            dlms_variant_t var;
            if (dlms_decode_variant(&dec, &var) == 0) {
                double val = (var.type == DLMS_TYPE_UINT16) ? (double)((var.payload[0]<<8)|var.payload[1]) : 0;
                pipe_msg_init_scalar(&out_msgs[0], 1001, val / 10.0, (uint64_t)time(NULL)*1000);
                *msg_count = 1;
            }
            priv->cosem.classic_block.active = false;
        }
    }

    net_buf_unref(apdu);
    return 0;
}

edge_driver_t dlms_driver = {
    .name = "DLMS",
    .ctx_size = sizeof(dlms_driver_ctx_t),
    .on_init = dlms_v3_on_init,
    .encode_request = dlms_v3_encode_request,
    .decode_response = dlms_v3_decode_response,
    .handle_rx = NULL,
    .on_cleanup = NULL
};

int dlms_driver_register(void) {
    return driver_manager_register(&dlms_driver);
}
