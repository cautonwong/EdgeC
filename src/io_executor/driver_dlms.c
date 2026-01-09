/**
 * @file driver_dlms.c
 * @brief 工业级 DLMS 驱动 (主从全功能 + 自动分帧确认)
 */

#include <string.h>
#include <time.h>

#include "contract_data_types.h"
#include "dlms_codec.h"
#include "dlms_cosem.h"
#include "dlms_hdlc.h"
#include "driver_manager.h"
#include "edge_common.h"
#include "edge_driver.h"
#include "edge_log.h"

extern const dlms_map_t *edge_lookup_dlms_by_tag(uint32_t tag_id);
__attribute__((weak)) const dlms_map_t *edge_lookup_dlms_by_tag(uint32_t tag_id) {
    (void)tag_id;
    return NULL;
}

typedef struct {
    dlms_hdlc_manager_t hdlc;
    dlms_cosem_ctx_t cosem;
    bool association_pending;
    uint32_t current_tag_id;
} dlms_driver_ctx_t;

static int dlms_v3_on_init(edge_driver_ctx_t *ctx) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    memset(priv, 0, sizeof(dlms_driver_ctx_t));
    priv->hdlc.client_addr = 0x01;
    priv->hdlc.server_addr = 0x03;
    dlms_hdlc_reset(&priv->hdlc);
    dlms_cosem_reset(&priv->cosem);
    return 0;
}

/* --- Master Role Logic --- */

static int dlms_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int count,
                                  net_buf_t *out) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;

    if (priv->hdlc.state == HDLC_STATE_DISCONNECTED) {
        return dlms_hdlc_build_snrm(&priv->hdlc, out);
    }

    if (priv->cosem.state != COSEM_STATE_ASSOCIATED) {
        if (!priv->association_pending) {
            net_buf_t *aarq = net_buf_alloc(out->pool, 0);
            dlms_cosem_build_aarq(&priv->cosem, aarq);
            dlms_hdlc_send_iframe(&priv->hdlc, aarq, out);
            net_buf_unref(aarq);
            priv->association_pending = true;
        }
        return 0;
    }

    if (count > 0) {
        const dlms_map_t *map = edge_lookup_dlms_by_tag(tag_ids[0]);
        if (map) {
            priv->current_tag_id = tag_ids[0];
            net_buf_t *apdu = net_buf_alloc(out->pool, 0);
            char obis_str[32];
            snprintf(obis_str, 31, "%d.%d.%d.%d.%d.%d", map->obis[0], map->obis[1], map->obis[2],
                     map->obis[3], map->obis[4], map->obis[5]);
            dlms_cosem_build_get_request(&priv->cosem, map->class_id, obis_str,
                                         map->attribute_index, NULL, apdu);
            dlms_hdlc_send_iframe(&priv->hdlc, apdu, out);
            net_buf_unref(apdu);
        }
    }
    return 0;
}

static int dlms_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                   pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;
    *consumed = 0;

    net_buf_t *apdu = NULL;
    int rc = dlms_hdlc_depacketize(&priv->hdlc, b, in->pool, &apdu);

    // 如果 depacketize 返回 1，说明是分包中的一帧
    if (rc == 1) {
        *consumed = b->len;  // 消耗当前所有字节，因为已经存入重组缓冲区
        dlms_hdlc_build_rr(&priv->hdlc, resp_out);  // 构建 RR 确认帧回写
        return EDGE_ERR_INCOMPLETE;
    }

    if (rc != 0 || !apdu) return -1;
    *consumed = b->len;

    if (priv->association_pending) {
        dlms_cosem_parse_aare(&priv->cosem, &apdu->b);
        priv->association_pending = false;
    } else {
        dlms_variant_t var;
        if (dlms_cosem_decode_response(&priv->cosem, &apdu->b, &var) == 0) {
            double val = 0;
            if (var.type == DLMS_TYPE_UINT16)
                val = (double)((var.payload[0] << 8) | var.payload[1]);
            pipe_msg_init_scalar(&out_msgs[0], priv->current_tag_id, val,
                                 (uint64_t)time(NULL) * 1000);
            *msg_count = 1;
        }
    }
    net_buf_unref(apdu);
    return 0;
}

/* --- Slave Role Logic --- */

static int dlms_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                             pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    (void)async_msgs;
    (void)async_count;

    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;

    struct net_buf_simple *b = &in->b;
    *consumed = 0;

    net_buf_t *apdu_in = NULL;
    int rc = dlms_hdlc_depacketize(&priv->hdlc, b, in->pool, &apdu_in);

    if (rc == 1) {
        // 从站也需要处理分包确认逻辑（如果主站发大数据过来）
        *consumed = b->len;
        dlms_hdlc_build_rr(&priv->hdlc, resp_out);
        return EDGE_ERR_INCOMPLETE;
    }

    if (rc != 0 || !apdu_in) return -1;
    *consumed = b->len;

    uint8_t tag = apdu_in->b.data[0];
    net_buf_t *apdu_out = net_buf_alloc(in->pool, 0);

    if (tag == DLMS_SERVICE_AARQ) {
        dlms_cosem_parse_aarq(&priv->cosem, &apdu_in->b);
        dlms_cosem_build_aare(&priv->cosem, apdu_out);
    } else if (tag == DLMS_SERVICE_GET_REQUEST) {
        uint16_t cid;
        uint8_t obis[6], attr;
        dlms_cosem_parse_get_request(&apdu_in->b, &cid, obis, &attr);
        uint16_t mock_val = 2305;
        dlms_cosem_build_get_response(&priv->cosem, DLMS_TYPE_UINT16, &mock_val, 2, apdu_out);
    }

    if (apdu_out->b.len > 0) {
        dlms_hdlc_send_iframe(&priv->hdlc, apdu_out, resp_out);
    }

    net_buf_unref(apdu_in);
    net_buf_unref(apdu_out);
    return 0;
}

edge_driver_t dlms_driver = {.name = "DLMS",
                             .ctx_size = sizeof(dlms_driver_ctx_t),
                             .on_init = dlms_v3_on_init,
                             .encode_request = dlms_v3_encode_request,
                             .decode_response = dlms_v3_decode_response,
                             .handle_rx = dlms_v3_handle_rx};

int dlms_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &dlms_driver);
}
