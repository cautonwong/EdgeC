/**
 * @file driver_dlms.c
 * @brief 工业级 DLMS 驱动 (契约化重塑最终版)
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

// 引入外部契约查找接口 (由 contract-compiler 生成)
// 使用 weak 属性防止链接错误
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

/* --- Master: 编码请求 --- */
static int dlms_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int count,
                                  net_buf_t *out) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;

    if (priv->hdlc.state == HDLC_STATE_DISCONNECTED) {
        dlms_hdlc_build_snrm(&priv->hdlc, out);
        priv->hdlc.state = HDLC_STATE_CONNECTING;
        return 0;
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
            net_buf_t *get_req = net_buf_alloc(out->pool, 0);
            // 修正：obis 是 uint8_t 数组，此处需要转换或修改接口
            dlms_cosem_build_get_request(&priv->cosem, map->class_id, (const char *)map->obis,
                                         map->attribute_index, get_req);
            dlms_hdlc_send_iframe(&priv->hdlc, get_req, out);
            net_buf_unref(get_req);
        }
    }
    return 0;
}

/* --- Master: 解析响应 --- */
static int dlms_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, pipe_msg_t *out_msgs,
                                   int *msg_count) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    net_buf_t *apdu = NULL;

    int ret = dlms_hdlc_depacketize(&priv->hdlc, &in->b, in->pool, &apdu);
    if (ret != 0 || !apdu) return (ret == 1) ? 1 : -1;

    uint8_t tag = apdu->b.data[0];
    if (tag == DLMS_SERVICE_AARE) {
        if (dlms_cosem_parse_aare(&priv->cosem, &apdu->b) == 0) {
            priv->association_pending = false;
            LOG_INFO(LOG_CAT_IO, "DLMS: Associated.");
        }
    } else if (tag == DLMS_SERVICE_GET_RESPONSE) {
        dlms_variant_t var;
        if (dlms_cosem_decode_get_response(&priv->cosem, &apdu->b, &var) == 0) {
            const dlms_map_t *map = edge_lookup_dlms_by_tag(priv->current_tag_id);
            float scale = map ? map->scaler : 1.0f;

            double final_val = 0;
            if (var.type == DLMS_TYPE_DOUBLE_LONG_UNSIGNED && var.length == 4) {
                uint32_t val = (uint32_t)((var.payload[0] << 24) | (var.payload[1] << 16) |
                                          (var.payload[2] << 8) | var.payload[3]);
                final_val = (double)val;
            } else if (var.type == DLMS_TYPE_LONG_UNSIGNED && var.length == 2) {
                uint16_t val = (uint16_t)((var.payload[0] << 8) | var.payload[1]);
                final_val = (double)val;
            }

            pipe_msg_init_scalar(&out_msgs[0], priv->current_tag_id, final_val * (double)scale,
                                 (uint64_t)time(NULL) * 1000);
            *msg_count = 1;
        }
    }

    net_buf_unref(apdu);
    return 0;
}

const edge_driver_t dlms_driver = {.name = "DLMS",
                                   .ctx_size = sizeof(dlms_driver_ctx_t),
                                   .on_init = dlms_v3_on_init,
                                   .encode_request = dlms_v3_encode_request,
                                   .decode_response = dlms_v3_decode_response};

int dlms_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &dlms_driver);
}