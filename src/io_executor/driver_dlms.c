/**
 * @file driver_dlms.c
 * @brief 工业级 DLMS 驱动 (基于 libedge-proto 专家库重塑 - EP_OK 对齐版)
 */

#include <string.h>
#include <time.h>

#include "driver_manager.h"
#include "edge_common.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "protocols/edge_dlms.h"

typedef struct {
    edge_dlms_context_t proto;
    uv_timer_t t1_timer;
    bool association_pending;
} dlms_driver_ctx_t;

static void _on_t1_timeout(uv_timer_t *handle) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)handle->data;
    LOG_WARN(LOG_CAT_IO, "DLMS: T1 Timeout [LLC:%d, COSEM:%d]", priv->proto.hdlc.state,
             priv->proto.state);
    priv->association_pending = false;
}

static int dlms_v3_on_init(edge_driver_ctx_t *ctx) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    memset(priv, 0, sizeof(dlms_driver_ctx_t));

    // 初始化 libedge-proto 上下文 (使用子模块原语)
    edge_hdlc_init(&priv->proto.hdlc, 0x01, 0x03);
    priv->proto.state = EDGE_COSEM_STATE_IDLE;

    uv_timer_init(uv_default_loop(), &priv->t1_timer);
    priv->t1_timer.data = priv;
    return 0;
}

static int dlms_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int tag_count,
                                  net_buf_t *out) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    struct iovec iov[8];
    edge_vector_t v;
    edge_vector_init(&v, iov, 8);

    // 1. 链路层握手
    if (priv->proto.hdlc.state == HDLC_STATE_DISCONNECTED) {
        edge_hdlc_build_snrm(&priv->proto.hdlc, &v);
    }
    // 2. 应用层关联
    else if (priv->proto.state != EDGE_COSEM_STATE_ASSOCIATED) {
        if (!priv->association_pending) {
            struct iovec apdu_iov[4];
            edge_vector_t apdu_v;
            edge_vector_init(&apdu_v, apdu_iov, 4);
            edge_dlms_encoder_t enc;
            edge_dlms_encoder_init(&enc, &apdu_v);
            edge_dlms_build_aarq(&enc);

            // 分帧并打包到 HDLC
            edge_hdlc_build_iframe(&priv->proto.hdlc, &v, apdu_iov[0].iov_base, apdu_v.total_len,
                                   true);
            priv->association_pending = true;
            uv_timer_start(&priv->t1_timer, _on_t1_timeout, 3000, 0);
        }
    }
    // 3. 业务数据请求
    else if (tag_count > 0) {
        struct iovec apdu_iov[4];
        edge_vector_t apdu_v;
        edge_vector_init(&apdu_v, apdu_iov, 4);
        edge_dlms_encoder_t enc;
        edge_dlms_encoder_init(&enc, &apdu_v);

        edge_dlms_object_t obj = {
            .class_id = 3, .obis = {1, 0, 1, 8, 0, 255}, .attribute_index = 2};
        edge_dlms_build_get_request(&enc, DLMS_GET_NORMAL, 0x01, &obj);

        edge_hdlc_build_iframe(&priv->proto.hdlc, &v, apdu_iov[0].iov_base, apdu_v.total_len, true);
        uv_timer_start(&priv->t1_timer, _on_t1_timeout, 3000, 0);
    }

    // 胶水层：线性化输出到主系统 net_buf
    if (v.total_len > 0) {
        for (int i = 0; i < v.used_count; i++) {
            net_buf_simple_add_mem(&out->b, v.iovs[i].iov_base, v.iovs[i].iov_len);
        }
    }
    (void)tag_ids;
    return 0;
}

static int dlms_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, pipe_msg_t *out_msgs,
                                   int *msg_count) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;

    // 零拷贝包裹
    struct iovec rx_iov = {.iov_base = in->b.data, .iov_len = in->b.len};
    edge_cursor_t c;
    edge_cursor_init(&c, &rx_iov, 1);

    uint8_t apdu_raw[1024];
    size_t apdu_len = sizeof(apdu_raw);

    // 1. 调用内核进行 HDLC 解析
    edge_error_t err = edge_hdlc_parse(&priv->proto.hdlc, &c, apdu_raw, &apdu_len);
    if (err != EP_OK) return (err == EP_ERR_INCOMPLETE_DATA) ? 1 : -1;

    uv_timer_stop(&priv->t1_timer);

    // 2. 解析 APDU
    struct iovec apdu_iov = {.iov_base = apdu_raw, .iov_len = apdu_len};
    edge_cursor_t ac;
    edge_cursor_init(&ac, &apdu_iov, 1);

    if (apdu_len == 0) return 0;

    uint8_t tag;
    edge_cursor_read_u8(&ac, &tag);
    if (tag == (uint8_t)DLMS_SERVICE_AARE) {
        priv->proto.state = EDGE_COSEM_STATE_ASSOCIATED;
        priv->association_pending = false;
        LOG_INFO(LOG_CAT_IO, "DLMS: Associated via libedge-proto kernel.");
    } else if (tag == (uint8_t)DLMS_SERVICE_GET_RESPONSE) {
        edge_dlms_variant_t var;
        if (edge_dlms_decode_variant(&ac, &var) == EP_OK) {
            double val = (var.tag == DLMS_TAG_LONG_UNSIGNED)
                             ? (double)((var.data[0] << 8) | var.data[1])
                             : 0;
            pipe_msg_init_scalar(&out_msgs[0], 1001, val / 10.0, (uint64_t)time(NULL) * 1000);
            *msg_count = 1;
        }
    }

    return 0;
}

edge_driver_t dlms_driver = {.name = "DLMS",
                             .ctx_size = sizeof(dlms_driver_ctx_t),
                             .on_init = dlms_v3_on_init,
                             .encode_request = dlms_v3_encode_request,
                             .decode_response = dlms_v3_decode_response};

int dlms_driver_register(void) {
    return driver_manager_register(&dlms_driver);
}
