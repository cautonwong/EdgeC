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
            // 修正：增加 access_params 参数 (传 NULL 表示 Normal GET)
            dlms_cosem_build_get_request(&priv->cosem, map->class_id, (const char *)map->obis,
                                         map->attribute_index, NULL, get_req);
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

    // 1. HDLC 层重组 (处理 Segmentation)
    int ret = dlms_hdlc_depacketize(&priv->hdlc, &in->b, in->pool, &apdu);
    if (ret != 0 || !apdu) return (ret == 1) ? 1 : -1;

    uint8_t tag = apdu->b.data[0];
    if (tag == DLMS_SERVICE_AARE) {
        if (dlms_cosem_parse_aare(&priv->cosem, &apdu->b) == 0) {
            priv->association_pending = false;
            LOG_INFO(LOG_CAT_IO, "DLMS: Associated.");
        }
    } else {
        // 2. COSEM 层语义化解析 (支持 Normal, Block Transfer, 和 Push)
        const dlms_map_t *map = edge_lookup_dlms_by_tag(priv->current_tag_id);
        if (!map) return -1;

        double val = 0;
        int cosem_ret =
            dlms_cosem_decode_response_semantic(&priv->cosem, &apdu->b, map->expected_type, &val);

        if (cosem_ret == 1) {
            LOG_INFO(LOG_CAT_IO, "DLMS: Block Transfer in progress...");
            net_buf_unref(apdu);
            return 1;
        }

        if (cosem_ret == 0) {
            pipe_msg_init_scalar(&out_msgs[0], priv->current_tag_id, val * (double)map->scaler,
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