/**
 * @file driver_dlms.c
 * @brief DLMS/COSEM over HDLC 驱动 —— 纯 codec（ADR-002 S3-c3）
 *
 * ## 点位标识：为什么用点位描述而不是 native_id
 *
 * DLMS 的一个点位是 **OBIS(6 字节) + class_id(2) + attribute_index(1)**，
 * 光 OBIS 就 48 位，`native_id`（uint32）装不下；扩到 64 位也不够（要 72 位）。
 *
 * 所以走 `point_desc` 通道：调用方把**这一个点**的描述递进来，
 * 驱动不查表、不反查、也不需要知道表里还有什么。
 *
 * ## 会话
 *
 * HDLC 建链 → COSEM 关联 → READY 是跨帧的状态机，放在驱动的持久上下文里
 * （`ctx_size`），`req_tick` 驱动它推进。这与 IEC104 的「请求级会话」不同。
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "contract_data_types.h"
#include "dlms_codec.h"
#include "dlms_cosem.h"
#include "dlms_hdlc.h"
#include "driver_dlms.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "net_buf.h"

typedef enum {
    DLMS_SESSION_IDLE,
    DLMS_SESSION_HDLC_CONNECTING,
    DLMS_SESSION_COSEM_ASSOCIATING,
    DLMS_SESSION_READY,
    DLMS_SESSION_ERROR
} dlms_session_state_t;

/* 设备级会话（ctx_size）：跨帧状态，随 SessionManager 的会话存活 */
typedef struct {
    dlms_hdlc_manager_t hdlc;
    dlms_cosem_ctx_t cosem;
    dlms_session_state_t session_state;
    uint64_t last_step_ts;
} dlms_session_t;

/* 请求级 scratch（req_priv_size）：只放"这一帧要读/写哪个点" */
typedef struct {
    dlms_point_t cur;
    bool cur_valid;
    bool is_write;
    uint8_t write_type;   /**< Data 类型标签（dlms_write_type_t） */
    double write_value;   /**< 写入值（raw） */
} dlms_req_t;

static int dlms_on_init(edge_driver_ctx_t *ctx) {
    dlms_session_t *priv = (dlms_session_t *)ctx->priv;
    memset(priv, 0, sizeof(*priv));

    priv->hdlc.client_addr = 0x01;
    priv->hdlc.server_addr = 0x03;

    dlms_hdlc_reset(&priv->hdlc);
    dlms_cosem_reset(&priv->cosem);
    priv->session_state = DLMS_SESSION_IDLE;
    return 0;
}

/* ==================== 主站：组请求 ==================== */

static int dlms_req_prepare(void *priv_v, uint32_t slave_id, uint32_t point_id,
                            const void *meta_list, uint32_t meta_list_len,
                            const void *point_desc, size_t point_desc_len,
                              bool is_write, double value) {
    (void)slave_id;
    (void)point_id;
    (void)meta_list;
    (void)meta_list_len;
    dlms_req_t *priv = (dlms_req_t *)priv_v;
    if (!priv) return -1;

    /* 点位描述是必须的：DLMS 没有 native_id 可用的 32 位标识。
     * 尺寸不对就拒绝——void* 不校验长度就是隐患。 */
    if (!point_desc || point_desc_len != sizeof(dlms_point_t)) return -1;
    memcpy(&priv->cur, point_desc, sizeof(dlms_point_t));
    priv->cur_valid = true;

    priv->is_write = is_write;
    priv->write_type = 0;
    priv->write_value = 0.0;
    if (is_write) {
        /* 写还要知道值的 Data 类型：同一个 OBIS 在不同表上类型可能不同，
         * 驱动猜不了。类型/值域不合法就拒绝。 */
        if (priv->cur.class_id == 0) return -1;
        /* 类型是点位描述的一部分（同一个 OBIS 在不同表上类型可能不同），
         * 所以放在 point_desc 里而不是 meta_list（meta_list 是设备附加信息）。 */
        uint8_t type = priv->cur.data_type;
        if (type == 0) {
            LOG_WARN(LOG_CAT_IO, "DLMS: 写请求缺少 Data 类型描述");
            return -1;
        }
        if (type == DLMS_WRITE_LONG_UNSIGNED) {
            if (value < 0.0 || value > 65535.0) {
                LOG_WARN(LOG_CAT_IO, "DLMS: long-unsigned 越界 %.0f", value);
                return -1;
            }
        } else if (type == DLMS_WRITE_DOUBLE_LONG_UNSIGNED) {
            if (value < 0.0 || value > 4294967295.0) {
                LOG_WARN(LOG_CAT_IO, "DLMS: double-long-unsigned 越界 %.0f", value);
                return -1;
            }
        } else {
            LOG_WARN(LOG_CAT_IO, "DLMS: 不支持的 Data 类型 0x%02X", type);
            return -1;
        }
        priv->write_type = type;
        priv->write_value = value;
    }
    return 0;
}

static int dlms_req_encode(void *priv_v, void *session, net_buf_t *out) {
    dlms_req_t *req = (dlms_req_t *)priv_v;
    dlms_session_t *priv = (dlms_session_t *)session;
    if (!req || !priv || !out || !req->cur_valid) return -1;
    if (priv->session_state != DLMS_SESSION_READY) return -1;

    char obis_str[32];
    snprintf(obis_str, sizeof(obis_str), "%d.%d.%d.%d.%d.%d", req->cur.obis[0],
             req->cur.obis[1], req->cur.obis[2], req->cur.obis[3], req->cur.obis[4],
             req->cur.obis[5]);

    net_buf_t *apdu = net_buf_alloc(out->pool, 0);
    if (!apdu) return -1;

    int rc;
    if (!req->is_write) {
        rc = dlms_cosem_build_get_request(&priv->cosem, req->cur.class_id, obis_str,
                                          req->cur.attribute_index, NULL, apdu);
    } else {
        /* SET：Data = 类型标签 + 大端值。只编 prepare 校验过的两种类型。 */
        uint8_t data[8];
        size_t n = 0;
        data[n++] = req->write_type;
        if (req->write_type == DLMS_WRITE_LONG_UNSIGNED) {
            uint16_t v = (uint16_t)req->write_value;
            data[n++] = (uint8_t)(v >> 8);
            data[n++] = (uint8_t)(v & 0xFF);
        } else {
            uint32_t v = (uint32_t)req->write_value;
            data[n++] = (uint8_t)(v >> 24);
            data[n++] = (uint8_t)(v >> 16);
            data[n++] = (uint8_t)(v >> 8);
            data[n++] = (uint8_t)(v & 0xFF);
        }
        rc = dlms_cosem_build_set_request(&priv->cosem, req->cur.class_id, obis_str,
                                          req->cur.attribute_index, data, n, apdu);
    }

    if (rc == 0) dlms_hdlc_send_iframe(&priv->hdlc, apdu, out);
    net_buf_unref(apdu);
    return rc == 0 ? 0 : -1;
}

/* ==================== 主站：解应答 ==================== */

static int dlms_req_decode(void *priv_v, void *session, const uint8_t *buf, size_t len,
                           size_t *consumed, double *out_value) {
    (void)priv_v; /* 应答解析不需要"读哪个点"——序号与关联状态都在 session 里 */
    dlms_session_t *priv = (dlms_session_t *)session;
    if (!priv || !buf || !consumed) return -1;
    *consumed = 0;

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, (void *)buf, len);
    nb.len = (uint16_t)len; /* init_with_data 只设 size */

    /* HDLC 解包需要 pool；这里借用不到 ctx->pool，用调用方给的 out 缓冲的池
     * —— req_decode 没有 pool 参数，但上层传进来的 buf 一定来自某个池。
     * 简化：用栈上的临时池不可行，改为不分配（depacketize 传入 NULL 表示复用入参）。 */
    net_buf_t *apdu = NULL;
    int rc = dlms_hdlc_depacketize(&priv->hdlc, &nb, NULL, &apdu);
    if (rc == 1) {
        *consumed = nb.len;
        return EDGE_ERR_INCOMPLETE;
    }
    if (rc != 0 || !apdu) return -1;
    *consumed = nb.len;

    if (priv->session_state == DLMS_SESSION_COSEM_ASSOCIATING) {
        dlms_cosem_parse_aare(&priv->cosem, &apdu->b);
        return -1; /* 关联应答不是测量值 */
    }

    dlms_variant_t var;
    if (dlms_cosem_decode_response(&priv->cosem, &apdu->b, &var) != 0) return -1;

    double val = 0;
    if (var.type == DLMS_TYPE_UINT16) {
        val = (double)((var.payload[0] << 8) | var.payload[1]);
    } else if (var.type == DLMS_TYPE_UINT32) {
        val = (double)(((uint32_t)var.payload[0] << 24) | ((uint32_t)var.payload[1] << 16) |
                       ((uint32_t)var.payload[2] << 8) | var.payload[3]);
    }
    if (out_value) *out_value = val; /* 标定归调用方 */
    return 0;
}

/* ==================== 从站：应答 ==================== */

static int dlms_req_slave_exchange(void *priv_v, void *session, const uint8_t *in, size_t in_len,
                                   const edge_slave_source_ops_t *src, void *src_ctx,
                                   net_buf_t *out, size_t *consumed) {
    (void)priv_v; /* HDLC 帧序号是跨帧状态，只能放 session */
    dlms_session_t *priv = (dlms_session_t *)session;
    if (!priv || !in || !out || !consumed) return -1;
    *consumed = 0;

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, (void *)in, in_len);
    nb.len = (uint16_t)in_len;

    net_buf_t *apdu_in = NULL;
    int rc = dlms_hdlc_depacketize(&priv->hdlc, &nb, out->pool, &apdu_in);
    if (rc == 1) {
        *consumed = nb.len;
        dlms_hdlc_build_rr(&priv->hdlc, out);
        return EDGE_ERR_INCOMPLETE;
    }
    if (rc != 0 || !apdu_in) return -1;
    *consumed = nb.len;

    uint8_t tag = apdu_in->b.data[0];
    net_buf_t *apdu_out = net_buf_alloc(out->pool, 0);
    if (!apdu_out) {
        net_buf_unref(apdu_in);
        return -1;
    }

    if (tag == DLMS_SERVICE_AARQ) {
        dlms_cosem_parse_aarq(&priv->cosem, &apdu_in->b);
        dlms_cosem_build_aare(&priv->cosem, apdu_out);
    } else if (tag == DLMS_SERVICE_GET_REQUEST || tag == DLMS_SERVICE_SET_REQUEST) {
        uint16_t cid = 0;
        uint8_t obis[6] = {0};
        uint8_t attr = 0;
        dlms_cosem_parse_get_request(&apdu_in->b, &cid, obis, &attr);

        /* 安全策略：OBIS 首字节为 1 表示受保护对象，未认证不给读 */
        if (obis[0] == 1 && priv->cosem.security_level < DLMS_SEC_AUTHENTICATED) {
            LOG_WARN(LOG_CAT_IO, "DLMS: 未认证访问受保护对象 %d.%d...", obis[0], obis[1]);
            net_buf_unref(apdu_in);
            net_buf_unref(apdu_out);
            return -1;
        }

        dlms_point_t pt;
        memset(&pt, 0, sizeof(pt));
        memcpy(pt.obis, obis, 6);
        pt.class_id = cid;
        pt.attribute_index = attr;

        if (tag == DLMS_SERVICE_GET_REQUEST) {
            edge_point_value_t v;
            memset(&v, 0, sizeof(v));
            uint16_t rv = 0;
            /* point_id 传 0：DLMS 的点位标识全在描述里 */
            if (src && src->resolve && src->resolve(src_ctx, 0, &pt, sizeof(pt), &v) == 0) {
                rv = (uint16_t)v.raw;
            }
            dlms_cosem_build_get_response(&priv->cosem, DLMS_TYPE_UINT16, &rv, 2, apdu_out);
        } else {
            /* SET：从数据段取原始值交给端口（简化：跳过类型与长度，取前 2 字节） */
            if (src && src->accept && apdu_in->b.len > 6) {
                uint16_t raw = (uint16_t)(((uint8_t)apdu_in->b.data[5] << 8) |
                                          (uint8_t)apdu_in->b.data[6]);
                (void)src->accept(src_ctx, 0, (double)raw);
            }
            dlms_cosem_build_get_response(&priv->cosem, DLMS_TYPE_UINT16, NULL, 0, apdu_out);
        }
    }

    if (apdu_out->b.len > 0) {
        dlms_hdlc_send_iframe(&priv->hdlc, apdu_out, out);
    }

    net_buf_unref(apdu_in);
    net_buf_unref(apdu_out);
    return 0;
}

/* ==================== 会话推进 ==================== */

static int dlms_req_tick(void *priv_v, const edge_north_source_ops_t *src, void *src_ctx,
                    const edge_slave_source_ops_t *slave, void *slave_ctx,
                    net_buf_pool_t *pool, net_buf_t **out_tx) {
    (void)slave; /* 这一路只发北向报文，不用从站端口 */
    (void)slave_ctx;
    (void)src;
    (void)src_ctx;
    dlms_session_t *priv = (dlms_session_t *)priv_v;
    if (!priv || !pool || !out_tx) return -1;

    uint64_t now = uv_hrtime() / 1000000;

    switch (priv->session_state) {
        case DLMS_SESSION_IDLE:
            *out_tx = net_buf_alloc(pool, 0);
            if (!*out_tx) return -1;
            dlms_hdlc_build_snrm(&priv->hdlc, *out_tx);
            priv->session_state = DLMS_SESSION_HDLC_CONNECTING;
            priv->last_step_ts = now;
            return 0;

        case DLMS_SESSION_HDLC_CONNECTING:
            if (priv->hdlc.state == HDLC_STATE_CONNECTED) {
                net_buf_t *apdu = net_buf_alloc(pool, 0);
                if (!apdu) return -1;
                *out_tx = net_buf_alloc(pool, 0);
                if (!*out_tx) {
                    net_buf_unref(apdu);
                    return -1;
                }
                dlms_cosem_build_aarq(&priv->cosem, apdu);
                dlms_hdlc_send_iframe(&priv->hdlc, apdu, *out_tx);
                net_buf_unref(apdu);
                priv->session_state = DLMS_SESSION_COSEM_ASSOCIATING;
                priv->last_step_ts = now;
                return 0;
            }
            if (now - priv->last_step_ts > 5000) priv->session_state = DLMS_SESSION_IDLE;
            break;

        case DLMS_SESSION_COSEM_ASSOCIATING:
            if (priv->cosem.state == COSEM_STATE_ASSOCIATED) {
                priv->session_state = DLMS_SESSION_READY;
                LOG_INFO(LOG_CAT_IO, "DLMS: 会话就绪");
            } else if (now - priv->last_step_ts > 10000) {
                priv->session_state = DLMS_SESSION_IDLE;
            }
            break;

        case DLMS_SESSION_READY:
            /* 保活：空闲 30s 发 RR */
            if (now - priv->last_step_ts > 30000) {
                *out_tx = net_buf_alloc(pool, 0);
                if (!*out_tx) return -1;
                dlms_hdlc_build_rr(&priv->hdlc, *out_tx);
                priv->last_step_ts = now;
                return 0;
            }
            break;

        default:
            break;
    }
    return -1;
}

const edge_driver_t dlms_driver = {
    .name = "DLMS",
    .ctx_size = sizeof(dlms_session_t),      /* 会话是跨帧状态，必须持久 */
    .req_priv_size = sizeof(dlms_req_t),    /* 请求 scratch 只放这一个点 */
    .on_init = dlms_on_init,
    .req_prepare = dlms_req_prepare,
    .req_encode = dlms_req_encode,
    .req_decode = dlms_req_decode,
    .req_slave_exchange = dlms_req_slave_exchange,
    .req_tick = dlms_req_tick,
};

int dlms_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &dlms_driver);
}
