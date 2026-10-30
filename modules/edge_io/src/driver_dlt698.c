/**
 * @file driver_dlt698.c
 * @brief 面向对象规约 DLT698.45 驱动 —— 纯 codec（ADR-002 S3-c）
 *
 * 与 645 同族，点位标识天然是 **OAD**（不需要像 Modbus 那样编码地址+长度），
 * 所以产品适配层不用改：`oad_to_proto("DLT698", oad)` 返回的就是 OAD。
 *
 * 这个文件里没有 lookup_by_tag / lookup_by_key / read_shadow / tag_id：
 *   - 主站：只认 oad，只回原始值；标识与量纲由调用方给
 *   - 从站：解 GET 请求 → src->resolve(oad) → 组应答，驱动不认识点位模型
 */

#include <stdint.h>
#include <string.h>

#include "dlt698_codec.h"
#include "dlt645_codec.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "net_buf.h"

#define DLT698_ADDR_LEN 6

/** 请求私有数据：驱动认识的全部信息 */
typedef struct {
    uint32_t oad;                  /**< 对象标识，698 原生点位 */
    uint8_t piid;                  /**< 服务序号 */
    uint8_t sa_addr[DLT698_ADDR_LEN];
    uint8_t sa_len;
    int32_t raw;                   /**< 解码出的原始值 */
} dlt698_req_t;

/* PIID 是协议层序号，不依赖任何全局服务 */
static uint8_t s_piid;

static void _fill_sa(dlt698_req_t *p, uint32_t slave_id) {
    memset(p->sa_addr, 0, sizeof(p->sa_addr));
    dlt645_u64_to_bcd_addr(slave_id, p->sa_addr);
    p->sa_len = DLT698_ADDR_LEN;
}

/* ==================== 主站侧 ==================== */

static int dlt698_req_prepare(void *priv, uint32_t slave_id, uint32_t point_id,
                              const void *meta_list, uint32_t meta_list_len) {
    (void)meta_list;
    (void)meta_list_len;
    dlt698_req_t *p = (dlt698_req_t *)priv;
    if (!p) return -1;
    p->oad = point_id;
    _fill_sa(p, slave_id);
    p->piid = ++s_piid;
    p->raw = 0;
    return 0;
}

static int dlt698_req_encode(void *priv, net_buf_t *out) {
    dlt698_req_t *p = (dlt698_req_t *)priv;
    if (!p || !out) return -1;
    dlt698_addr_t sa = {.len = p->sa_len, .type = 0};
    memcpy(sa.addr, p->sa_addr, p->sa_len);
    return dlt698_encode_get_request(&sa, p->piid, p->oad, out);
}

static int dlt698_req_decode(void *priv, const uint8_t *buf, size_t len, size_t *consumed,
                             double *out_value) {
    dlt698_req_t *p = (dlt698_req_t *)priv;
    if (!p || !buf || !consumed) return -1;
    *consumed = 0;

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, (void *)buf, len);
    nb.len = (uint16_t)len; /* init_with_data 只设 size，len 留给写入用 */

    dlt698_addr_t sa_rx;
    uint8_t svc = 0, piid = 0;
    uint8_t *payload = NULL;
    size_t payload_len = 0;

    int ret = dlt698_decode_frame(&nb, &sa_rx, &svc, &piid, &payload, &payload_len);
    if (ret == 1) return EDGE_ERR_INCOMPLETE;
    if (ret < 0) {
        *consumed = 1;
        return EDGE_ERR_INCOMPLETE;
    }
    *consumed = (size_t)(payload + payload_len + 3 - buf);

    if (svc != DLT698_SVC_GET_RES || payload_len < 5) return -1;

    uint32_t oad_rx = (uint32_t)((payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) |
                                 payload[3]);
    /* 不是本请求的对象：不认，让调用方继续找 */
    if (oad_rx != p->oad) return EDGE_ERR_INCOMPLETE;

    uint16_t offset = 4;
    dlt698_data_t res;
    if (dlt698_decode_data(payload, (uint16_t)payload_len, &offset, &res) != 0) return -1;

    double val = 0.0;
    switch (res.tag) {
        case DLT698_TAG_UNSIGNED: val = (double)res.val.u8; break;
        case DLT698_TAG_LONG_UNSIGNED: val = (double)res.val.u16; break;
        case DLT698_TAG_DOUBLE_LONG_UNSIGNED: val = (double)res.val.u32; break;
        case DLT698_TAG_INTEGER: val = (double)res.val.i8; break;
        case DLT698_TAG_LONG: val = (double)res.val.i16; break;
        case DLT698_TAG_DOUBLE_LONG: val = (double)res.val.i32; break;
        default:
            dlt698_free_data(&res);
            return -1;
    }
    dlt698_free_data(&res);

    /* 只回原始值；量纲由调用方按点位表给 */
    p->raw = (int32_t)val;
    if (out_value) *out_value = val;
    return 0;
}

/* ==================== 从站侧 ==================== */

static int dlt698_req_slave_exchange(void *priv, const uint8_t *in, size_t in_len,
                                     const edge_slave_source_ops_t *src, void *src_ctx,
                                     net_buf_t *out, size_t *consumed) {
    dlt698_req_t *p = (dlt698_req_t *)priv;
    if (!p || !in || !out || !consumed) return -1;
    *consumed = 0;

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, (void *)in, in_len);
    nb.len = (uint16_t)in_len;

    dlt698_addr_t sa_rx;
    uint8_t svc = 0, piid = 0;
    uint8_t *payload = NULL;
    size_t payload_len = 0;

    int ret = dlt698_decode_frame(&nb, &sa_rx, &svc, &piid, &payload, &payload_len);
    if (ret == 1) return EDGE_ERR_INCOMPLETE;
    if (ret < 0) {
        *consumed = 1;
        return -1;
    }
    *consumed = (size_t)(payload + payload_len + 3 - in);

    /* 只应答本机地址 */
    if (p->sa_len && (sa_rx.len != p->sa_len || memcmp(sa_rx.addr, p->sa_addr, p->sa_len) != 0)) {
        return 1;
    }

    if (svc != DLT698_SVC_GET_REQ || payload_len < 4) return -1;

    uint32_t oad = (uint32_t)((payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) |
                              payload[3]);
    edge_point_value_t v;
    memset(&v, 0, sizeof(v));
    if (!src || !src->resolve || src->resolve(src_ctx, oad, &v) != 0) {
        /* 无此对象：不应答（698 的异常应答格式复杂，先保持静默，由主站超时） */
        return -1;
    }

    uint16_t rv = (uint16_t)v.raw;
    uint8_t data[2] = {(uint8_t)(rv >> 8), (uint8_t)(rv & 0xFF)};
    dlt698_addr_t sa = {.len = p->sa_len, .type = 0};
    memcpy(sa.addr, p->sa_addr, p->sa_len);
    dlt698_encode_get_response(&sa, piid, oad, data, 2, out);
    return 0;
}

const edge_driver_t dlt698_driver = {
    .name = "DLT698",
    .ctx_size = 0, /* request-native：无 per-session 上下文 */
    .req_priv_size = sizeof(dlt698_req_t),
    .req_prepare = dlt698_req_prepare,
    .req_encode = dlt698_req_encode,
    .req_decode = dlt698_req_decode,
    .req_slave_exchange = dlt698_req_slave_exchange,
};

int dlt698_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &dlt698_driver);
}
