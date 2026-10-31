/**
 * @file driver_iec101.c
 * @brief IEC 60870-5-101 驱动 —— 纯 codec（ADR-002 S3-c2）
 *
 * ## 点位标识
 *
 * IEC101 的 ASDU 需要 **类型标识 + 信息对象地址** 两个东西才能组帧，
 * 而 native_id 只有一个 32 位数，所以编码为：
 *
 *     native_id = (type_id << 24) | (ioa & 0xFFFFFF)
 *
 * 两者都是协议原生概念，产品侧用 IEC101_NATIVE() 编，驱动侧解。
 *
 * ## 三个槽位
 *
 * - `req_encode` / `req_decode`：主站点对点读
 * - `req_slave_exchange`：从站应答。总召唤（C_IC_NA_1）要遍历全部点位，
 *   靠端口新增的 `enumerate` 拿列表，再逐个 `resolve` 取值
 * - `req_tick`：主站周期请求链路状态
 */

#include <stdint.h>
#include <string.h>
#include <time.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "iec101_link.h"
#include "iec_asdu.h"
#include "net_buf.h"

/* [ADR-002] 点位标识 = 类型标识(高 8 位) + 信息对象地址(低 24 位) */
#define IEC101_NATIVE(type_id, ioa) \
    ((((uint32_t)(type_id) & 0xFFu) << 24) | ((uint32_t)(ioa) & 0xFFFFFFu))
#define IEC101_NATIVE_TYPE(id) ((uint8_t)(((id) >> 24) & 0xFFu))
#define IEC101_NATIVE_IOA(id) ((uint32_t)((id) & 0xFFFFFFu))

typedef struct {
    iec_addr_config_t addr_conf;
    uint16_t casdu;
    bool request_level1;
    iec101_link_mgr_t link;
    /* 当前请求（主站解码时用来校验 IOA） */
    uint32_t cur_type;
    uint32_t cur_ioa;
    bool is_write;
    double write_value;
} iec101_ctx_t;

static int iec101_on_init(edge_driver_ctx_t *ctx) {
    iec101_ctx_t *priv = (iec101_ctx_t *)ctx->priv;
    memset(priv, 0, sizeof(iec101_ctx_t));

    priv->addr_conf.cot_len = 1;
    priv->addr_conf.casdu_len = 1;
    priv->addr_conf.ioa_len = 2;

    priv->link.link_addr = (uint16_t)ctx->device_id;
    priv->link.addr_len = 1;
    priv->casdu = (uint16_t)ctx->device_id;
    return 0;
}

/* ==================== 主站：组请求 ==================== */

static int iec101_req_prepare(void *priv_v, uint32_t slave_id, uint32_t point_id,
                              const void *meta_list, uint32_t meta_list_len,
                              const void *point_desc, size_t point_desc_len,
                              bool is_write, double value) {
    (void)point_desc;
    (void)point_desc_len;
    (void)meta_list;
    (void)meta_list_len;
    iec101_ctx_t *priv = (iec101_ctx_t *)priv_v;
    if (!priv) return -1;

    uint8_t type = IEC_NATIVE_TYPE(point_id);
    if (is_write && !IEC_IS_COMMAND_TYPE(type)) {
        /* 写请求必须带命令类型。用测量类型去"写"是协议错误——
         * 不能拿一个 M_ME_NC_1 当控制命令发出去。 */
        LOG_WARN(LOG_CAT_IO, "IEC101: 写请求的 type_id=%u 不是命令类型", type);
        return -1;
    }

    priv->cur_type = type;
    priv->cur_ioa = IEC_NATIVE_IOA(point_id);
    priv->is_write = is_write;
    priv->write_value = value;
    priv->link.link_addr = (uint16_t)slave_id;
    priv->casdu = (uint16_t)slave_id;
    return 0;
}

static int iec101_req_encode(void *priv_v, void *session, net_buf_t *out) {
    (void)session; /* 无会话状态：这一帧自成一体 */
    iec101_ctx_t *priv = (iec101_ctx_t *)priv_v;
    if (!priv || !out) return -1;

    if (priv->is_write) {
        /* 控制方向用 ACT（激活），不是 REQ */
        iec_asdu_init(out, (iec_type_id_t)priv->cur_type, false, 1, IEC_COT_ACT, priv->casdu,
                      &priv->addr_conf);
        if (priv->cur_type == IEC_TYPE_C_SC_NA_1) {
            return iec_asdu_add_c_sc_na_1(out, priv->cur_ioa, priv->write_value > 0.0, false,
                                          &priv->addr_conf) == 0
                       ? 0
                       : -1;
        }
        if (priv->cur_type == IEC_TYPE_C_DC_NA_1) {
            /* 双点遥控：1=分闸，2=合闸（0 不允许） */
            uint8_t dcs = (priv->write_value > 0.0) ? 2 : 1;
            return iec_asdu_add_c_dc_na_1(out, priv->cur_ioa, dcs, false, &priv->addr_conf) == 0
                       ? 0
                       : -1;
        }
        return iec_asdu_add_c_se_nc_1(out, priv->cur_ioa, (float)priv->write_value, 0, false,
                                      &priv->addr_conf) == 0
                   ? 0
                   : -1;
    }

    iec_asdu_init(out, (iec_type_id_t)priv->cur_type, false, 1, IEC_COT_REQ, priv->casdu,
                  &priv->addr_conf);
    iec_asdu_add_any(out, (uint8_t)priv->cur_type, priv->cur_ioa, 0, 0, 0, (iec_qds_t){0},
                     &priv->addr_conf);
    return 0;
}

/* ==================== 主站：解应答 ==================== */

static int iec101_req_decode(void *priv_v, void *session, const uint8_t *buf, size_t len, size_t *consumed,
                             double *out_value) {
    (void)session; /* 无会话状态：这一帧自成一体 */
    iec101_ctx_t *priv = (iec101_ctx_t *)priv_v;
    if (!priv || !buf || !consumed) return -1;
    *consumed = 0;

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, (void *)buf, len);
    nb.len = (uint16_t)len; /* init_with_data 只设 size */

    /* 流同步：找 0x10 / 0x68 / 0xE5 */
    size_t skip = 0;
    while (skip < nb.len) {
        uint8_t h = nb.data[skip];
        if (h == 0x10 || h == 0x68 || h == 0xE5) break;
        skip++;
    }
    if (skip > 0) {
        *consumed = skip;
        return 1;
    }
    if (nb.len == 0) return EDGE_ERR_INCOMPLETE;

    uint8_t head = nb.data[0];
    if (head == 0xE5) { /* 单字符确认，无数据 */
        *consumed = 1;
        return -1;
    }
    if (head == 0x10) { /* 固定帧长：链路层，没有测量值 */
        if (nb.len < (size_t)(3 + priv->link.addr_len)) return EDGE_ERR_INCOMPLETE;
        *consumed = (size_t)(3 + priv->link.addr_len);
        return -1;
    }

    /* 可变帧长 */
    if (nb.len < 4) return EDGE_ERR_INCOMPLETE;
    uint8_t flen = nb.data[1];
    if (nb.len < (size_t)(flen + 6)) return EDGE_ERR_INCOMPLETE;
    *consumed = (size_t)(flen + 6);

    net_buf_simple_pull(&nb, (size_t)(4 + 1 + priv->link.addr_len));

    iec_type_id_t type;
    bool sq;
    uint8_t count;
    iec_cot_t cot;
    uint16_t casdu;
    if (iec_asdu_parse_header(&nb, &type, &sq, &count, &cot, &casdu, &priv->addr_conf) != 0)
        return -1;

    uint32_t ioa = 0;
    for (int i = 0; i < count; i++) {
        bool first = (i == 0);
        if (type == IEC_TYPE_M_SP_NA_1) {
            bool val;
            iec_qds_t qds;
            if (iec_asdu_parse_m_sp_na_1(&nb, &ioa, &val, &qds, sq, first, &priv->addr_conf) != 0)
                break;
            if (ioa == priv->cur_ioa) {
                if (out_value) *out_value = val ? 1.0 : 0.0;
                return 0;
            }
        } else if (type == IEC_TYPE_M_ME_NC_1) {
            float val;
            iec_qds_t qds;
            if (iec_asdu_parse_m_me_nc_1(&nb, &ioa, &val, &qds, sq, first, &priv->addr_conf) != 0)
                break;
            if (ioa == priv->cur_ioa) {
                if (out_value) *out_value = (double)val; /* 标定归调用方 */
                return 0;
            }
        } else {
            break; /* 不支持的类型 */
        }
    }
    return -1; /* 不是本请求的对象 */
}

/* ==================== 从站：应答 ==================== */

typedef struct {
    net_buf_t *out;
    iec101_ctx_t *priv;
    const edge_slave_source_ops_t *src;
    void *src_ctx;
    iec_cot_t cot;
    uint8_t count;
} iec101_gi_ctx_t;

static void _gi_emit(void *ud, uint32_t point_id) {
    iec101_gi_ctx_t *g = (iec101_gi_ctx_t *)ud;
    edge_point_value_t v;
    memset(&v, 0, sizeof(v));
    if (!g->src->resolve || g->src->resolve(g->src_ctx, point_id, NULL, 0, &v) != 0) return;

    uint8_t type = IEC101_NATIVE_TYPE(point_id);
    uint32_t ioa = IEC101_NATIVE_IOA(point_id);
    iec_asdu_add_any(g->out, type, ioa, (double)v.raw, 0, 0, (iec_qds_t){0}, &g->priv->addr_conf);
    g->count++;
}

static int iec101_req_slave_exchange(void *priv_v, void *session, const uint8_t *in, size_t in_len,
                                     const edge_slave_source_ops_t *src, void *src_ctx,
                                     net_buf_t *out, size_t *consumed) {
    (void)session; /* 无跨帧状态：一问一答自成一体 */
    iec101_ctx_t *priv = (iec101_ctx_t *)priv_v;
    if (!priv || !in || !out || !consumed) return -1;
    *consumed = 0;

    if (in_len < 1) return EDGE_ERR_INCOMPLETE;
    uint8_t head = in[0];

    /* 固定帧长：链路状态请求 → 回链路状态 */
    if (head == 0x10) {
        if (in_len < (size_t)(3 + priv->link.addr_len)) return EDGE_ERR_INCOMPLETE;
        *consumed = (size_t)(3 + priv->link.addr_len);
        uint8_t func = in[1] & 0x0F;
        if (func == 9) {
            iec101_ctrl_t resp = {.prm = 0, .func = 11};
            iec101_link_build_fixed(&priv->link, resp, out);
        }
        return 0;
    }

    if (head != 0x68) {
        *consumed = 1; /* 跳一格重找帧首 */
        return 1;
    }

    if (in_len < 4) return EDGE_ERR_INCOMPLETE;
    uint8_t flen = in[1];
    if (in_len < (size_t)(flen + 6)) return EDGE_ERR_INCOMPLETE;
    *consumed = (size_t)(flen + 6);

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, (void *)in, in_len);
    nb.len = (uint16_t)in_len;
    net_buf_simple_pull(&nb, (size_t)(4 + 1 + priv->link.addr_len));

    iec_type_id_t type;
    bool sq;
    uint8_t count;
    iec_cot_t cot;
    uint16_t casdu;
    if (iec_asdu_parse_header(&nb, &type, &sq, &count, &cot, &casdu, &priv->addr_conf) != 0)
        return -1;

    /* 总召唤：遍历全部点位，逐个 resolve 后组 ASDU */
    if (type == IEC_TYPE_C_IC_NA_1) {
        if (!src || !src->enumerate) return -1;
        /* 先写头，count 之后回填不了——所以先收集到临时 ASDU */
        net_buf_t *asdu = net_buf_alloc(out->pool, 0);
        if (!asdu) return -1;
        iec_asdu_init(asdu, IEC_TYPE_M_ME_NC_1, false, 1, IEC_COT_INTROGEN, priv->casdu,
                      &priv->addr_conf);
        iec101_gi_ctx_t g = {.out = asdu, .priv = priv, .src = src, .src_ctx = src_ctx,
                             .cot = IEC_COT_INTROGEN, .count = 0};
        src->enumerate(src_ctx, _gi_emit, &g);
        if (g.count > 0) {
            net_buf_simple_add_mem(&out->b, asdu->b.data, asdu->b.len);
        }
        net_buf_unref(asdu);
        return 0;
    }

    /* 其它类型：按 IOA 取一个值回单点 ASDU */
    uint32_t ioa = 0;
    if (iec_asdu_parse_m_me_nc_1(&nb, &ioa, &(float){0}, &(iec_qds_t){0}, false, true,
                                 &priv->addr_conf) != 0)
        return -1;

    edge_point_value_t v;
    memset(&v, 0, sizeof(v));
    uint32_t pid = IEC101_NATIVE((uint32_t)type, ioa);
    if (!src || !src->resolve || src->resolve(src_ctx, pid, NULL, 0, &v) != 0) return -1;

    iec_asdu_init(out, type, false, 1, IEC_COT_ACT_CON, priv->casdu, &priv->addr_conf);
    iec_asdu_add_any(out, (uint8_t)type, ioa, (double)v.raw, 0, 0, (iec_qds_t){0},
                     &priv->addr_conf);
    return 0;
}

/* ==================== 主站：周期链路状态 ==================== */

static int iec101_req_tick(void *priv_v, const edge_north_source_ops_t *src, void *src_ctx,
                    const edge_slave_source_ops_t *slave, void *slave_ctx,
                    net_buf_pool_t *pool, net_buf_t **out_tx) {
    (void)slave; /* 这一路只发北向报文，不用从站端口 */
    (void)slave_ctx;
    (void)src;
    (void)src_ctx;
    iec101_ctx_t *priv = (iec101_ctx_t *)priv_v;
    if (!priv || !pool || !out_tx) return -1;

    iec101_ctrl_t req = {.prm = 1, .func = 9}; /* 请求链路状态 */
    *out_tx = net_buf_alloc(pool, 0);
    if (!*out_tx) return -1;
    iec101_link_build_fixed(&priv->link, req, *out_tx);
    return 0;
}

const edge_driver_t iec101_driver = {
    .name = "IEC101",
    .ctx_size = sizeof(iec101_ctx_t),
    .on_init = iec101_on_init,
    .req_prepare = iec101_req_prepare,
    .req_encode = iec101_req_encode,
    .req_decode = iec101_req_decode,
    .req_slave_exchange = iec101_req_slave_exchange,
    .req_tick = iec101_req_tick,
};

int iec101_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &iec101_driver);
}
