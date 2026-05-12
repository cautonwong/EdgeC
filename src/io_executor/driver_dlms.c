/**
 * @file driver_dlms.c
 * @brief 工业级 DLMS 驱动 (有状态会话 + 自动握手版)
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
#include "datahub.h"

typedef enum {
    DLMS_SESSION_IDLE,
    DLMS_SESSION_HDLC_CONNECTING,
    DLMS_SESSION_COSEM_ASSOCIATING,
    DLMS_SESSION_READY,
    DLMS_SESSION_ERROR
} dlms_session_state_t;

typedef struct {
    dlms_hdlc_manager_t hdlc;
    dlms_cosem_ctx_t cosem;
    dlms_session_state_t session_state;
    uint32_t current_tag_id;
    uint64_t last_step_ts;
} dlms_driver_ctx_t;

static int dlms_v4_on_init(edge_driver_ctx_t *ctx) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    memset(priv, 0, sizeof(dlms_driver_ctx_t));
    
    priv->hdlc.client_addr = 0x01;
    priv->hdlc.server_addr = 0x03;
    
    dlms_hdlc_reset(&priv->hdlc);
    dlms_cosem_reset(&priv->cosem);
    priv->session_state = DLMS_SESSION_IDLE;
    return 0;
}

static int dlms_v4_on_tick(edge_driver_ctx_t *ctx, net_buf_t **out_tx) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    uint64_t now = uv_hrtime() / 1000000;

    switch (priv->session_state) {
        case DLMS_SESSION_IDLE:
            *out_tx = net_buf_alloc(ctx->pool, 0);
            dlms_hdlc_build_snrm(&priv->hdlc, *out_tx);
            priv->session_state = DLMS_SESSION_HDLC_CONNECTING;
            priv->last_step_ts = now;
            return 0;

        case DLMS_SESSION_HDLC_CONNECTING:
            if (priv->hdlc.state == HDLC_STATE_CONNECTED) {
                *out_tx = net_buf_alloc(ctx->pool, 0);
                net_buf_t *apdu = net_buf_alloc(ctx->pool, 0);
                dlms_cosem_build_aarq(&priv->cosem, apdu);
                dlms_hdlc_send_iframe(&priv->hdlc, apdu, *out_tx);
                net_buf_unref(apdu);
                
                priv->session_state = DLMS_SESSION_COSEM_ASSOCIATING;
                priv->last_step_ts = now;
                return 0;
            } else if (now - priv->last_step_ts > 5000) {
                priv->session_state = DLMS_SESSION_IDLE;
            }
            break;

        case DLMS_SESSION_COSEM_ASSOCIATING:
            if (priv->cosem.state == COSEM_STATE_ASSOCIATED) {
                priv->session_state = DLMS_SESSION_READY;
                LOG_INFO(LOG_CAT_IO, "DLMS: Session for Device %u is READY.", ctx->device_id);
            } else if (now - priv->last_step_ts > 10000) {
                priv->session_state = DLMS_SESSION_IDLE;
            }
            break;

        case DLMS_SESSION_READY:
            if (now - ctx->last_activity_ts > 30000) {
                *out_tx = net_buf_alloc(ctx->pool, 0);
                dlms_hdlc_build_rr(&priv->hdlc, *out_tx);
                return 0;
            }
            break;
            
        default: break;
    }
    return -1;
}

static bool dlms_v4_is_ready(edge_driver_ctx_t *ctx) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    return priv->session_state == DLMS_SESSION_READY;
}

static int dlms_v4_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int count,
                                  net_buf_t *out) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    if (priv->session_state != DLMS_SESSION_READY) return -1;

    if (count > 0 && ctx->lookup_by_tag) {
        const dlms_map_t *map = (const dlms_map_t *)ctx->lookup_by_tag(tag_ids[0]);
        if (map) {
            priv->current_tag_id = tag_ids[0];
            net_buf_t *apdu = net_buf_alloc(ctx->pool, 0);
            char obis_str[32];
            snprintf(obis_str, 31, "%d.%d.%d.%d.%d.%d", map->obis[0], map->obis[1], map->obis[2],
                     map->obis[3], map->obis[4], map->obis[5]);
            dlms_cosem_build_get_request(&priv->cosem, map->class_id, obis_str,
                                         map->attribute_index, NULL, apdu);
            dlms_hdlc_send_iframe(&priv->hdlc, apdu, out);
            net_buf_unref(apdu);
            return 0;
        }
    }
    return -1;
}

static int dlms_v4_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                   pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;
    *consumed = 0;

    net_buf_t *apdu = NULL;
    int rc = dlms_hdlc_depacketize(&priv->hdlc, b, ctx->pool, &apdu);

    if (rc == 1) {
        *consumed = b->len;
        dlms_hdlc_build_rr(&priv->hdlc, resp_out);
        return EDGE_ERR_INCOMPLETE;
    }
    if (rc != 0 || !apdu) return -1;
    *consumed = b->len;

    if (priv->session_state == DLMS_SESSION_COSEM_ASSOCIATING) {
        dlms_cosem_parse_aare(&priv->cosem, &apdu->b);
    } else {
        dlms_variant_t var;
        if (dlms_cosem_decode_response(&priv->cosem, &apdu->b, &var) == 0) {
            double val = 0;
            if (var.type == DLMS_TYPE_UINT16)
                val = (double)((var.payload[0] << 8) | var.payload[1]);
            else if (var.type == DLMS_TYPE_UINT32)
                val = (double)((var.payload[0] << 24) | (var.payload[1] << 16) | (var.payload[2] << 8) | var.payload[3]);
            
            pipe_msg_init_scalar(&out_msgs[0], priv->current_tag_id, val, uv_hrtime());
            *msg_count = 1;
        }
    }
    net_buf_unref(apdu);
    return 0;
}

static int dlms_v4_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                             pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    (void)async_msgs; (void)async_count;
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;
    *consumed = 0;

    net_buf_t *apdu_in = NULL;
    int rc = dlms_hdlc_depacketize(&priv->hdlc, b, ctx->pool, &apdu_in);
    if (rc == 1) {
        *consumed = b->len;
        dlms_hdlc_build_rr(&priv->hdlc, resp_out);
        return EDGE_ERR_INCOMPLETE;
    }
    if (rc != 0 || !apdu_in) return -1;
    *consumed = b->len;

    uint8_t tag = apdu_in->b.data[0];
    net_buf_t *apdu_out = net_buf_alloc(ctx->pool, 0);

    if (tag == DLMS_SERVICE_AARQ) {
        dlms_cosem_parse_aarq(&priv->cosem, &apdu_in->b);
        dlms_cosem_build_aare(&priv->cosem, apdu_out);
    } else if (tag == DLMS_SERVICE_GET_REQUEST) {
        uint16_t cid; uint8_t obis[6], attr;
        dlms_cosem_parse_get_request(&apdu_in->b, &cid, obis, &attr);
        
        double val = 0;
        bool found = false;
        if (ctx->lookup_by_key) {
            uint64_t key = 0;
            for(int i=0; i<6; i++) key = (key << 8) | obis[i];
            const dlms_map_t *map = (const dlms_map_t *)ctx->lookup_by_key(key);
            if (map && ctx->user_data) {
                pipe_msg_t shadow;
                if (datahub_read_shadow((datahub_t *)ctx->user_data, map->tag_id, &shadow) == 0) {
                    val = shadow.payload.val.f;
                    found = true;
                }
            }
        }

        if (found) {
            uint16_t rv = (uint16_t)val;
            dlms_cosem_build_get_response(&priv->cosem, DLMS_TYPE_UINT16, &rv, 2, apdu_out);
        } else {
            uint16_t rv = 0;
            dlms_cosem_build_get_response(&priv->cosem, DLMS_TYPE_UINT16, &rv, 2, apdu_out);
        }
    } else if (tag == DLMS_SERVICE_SET_REQUEST) {
        /* [从站] 处理 SET 请求: 解析 OBIS + 值, 发布到 DataHub */
        uint16_t cid; uint8_t obis[6], attr;
        dlms_cosem_parse_get_request(&apdu_in->b, &cid, obis, &attr);

        if (ctx->lookup_by_key) {
            uint64_t key = 0;
            for (int i = 0; i < 6; i++) key = (key << 8) | obis[i];
            const dlms_map_t *map = (const dlms_map_t *)ctx->lookup_by_key(key);
            if (map && ctx->user_data) {
                /* 从 SET 数据中提取值 (简化: 跳过类型和长度, 取前2字节) */
                if (apdu_in->b.len > 6) {
                    uint16_t raw = (uint16_t)((uint8_t)apdu_in->b.data[5] << 8
                                              | (uint8_t)apdu_in->b.data[6]);
                    double scaled = (double)raw / map->scaler;
                    pipe_msg_t am;
                    pipe_msg_init_scalar(&am, map->tag_id, scaled, uv_hrtime());
                    datahub_publish((datahub_t *)ctx->user_data, &am);
                }
            }
        }
        /* SET 响应 (简化: 直接镜像) */
        dlms_cosem_build_get_response(&priv->cosem, DLMS_TYPE_UINT16, NULL, 0, apdu_out);
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
                             .on_init = dlms_v4_on_init,
                             .on_tick = dlms_v4_on_tick,
                             .is_ready = dlms_v4_is_ready,
                             .encode_request = dlms_v4_encode_request,
                             .decode_response = dlms_v4_decode_response,
                             .handle_rx = dlms_v4_handle_rx};

int dlms_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &dlms_driver);
}
