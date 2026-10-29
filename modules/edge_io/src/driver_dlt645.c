/**
 * @file driver_dlt645.c
 * @brief DL/T 645 纯 codec（ADR-002 S1 样板）
 *
 * 这个文件里**没有** edge_driver_ctx_t、没有 tag_ids、没有 pipe_msg_t、
 * 没有 lookup_by_*、没有 read_shadow。只有"结构体 ↔ 字节"。
 *
 * 之前的三处泄漏（见 docs/design/DLT645_REQUEST_CODEC.md §1）：
 *   L1 用 TagID 冒充 DI 写进报文 → 现在 di 是一等公民，来自点位模型
 *   L2 把 DI 当 tag_id 抛出去   → 驱动不产生消息，值回填给调用方
 *   L3 从站读 datahub 未接入    → decode_slave_req / encode_slave_rsp 只做编解码，
 *                                 取值由调用方经 edge_slave_source_ops_t 提供
 */

#include <stdint.h>
#include <string.h>

#include "driver_dlt645.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"

/* ==================== 帧级工具 ==================== */

static uint8_t _cs(const uint8_t *p, size_t n) {
    uint8_t s = 0;
    for (size_t i = 0; i < n; i++) s += p[i];
    return s;
}

/* 跳过前导垃圾找到帧首。返回 0=已对齐；>0=跳过字节数 */
static size_t _sync(const uint8_t *buf, size_t len) {
    size_t skip = 0;
    while (skip < len && buf[skip] != 0x68) skip++;
    return skip;
}

/* 帧总长 = L + 12；L 在偏移 9。返回 0=够整帧 */
static int _frame_len(const uint8_t *buf, size_t len, uint8_t *out_l, size_t *out_total) {
    if (len < 10) return EDGE_ERR_INCOMPLETE;
    uint8_t l = buf[9];
    size_t total = (size_t)l + 12;
    if (len < total) return EDGE_ERR_INCOMPLETE;
    *out_l = l;
    *out_total = total;
    return 0;
}

/* DI 低字节先，逐字节 +0x33 */
static void _put_di(struct net_buf_simple *b, uint32_t di) {
    for (int i = 0; i < DLT645_DI_LEN; i++)
        net_buf_simple_add_u8(b, (uint8_t)(((di >> (8 * i)) & 0xFF) + 0x33));
}

static uint32_t _get_di(const uint8_t *p) {
    uint32_t di = 0;
    for (int i = 0; i < DLT645_DI_LEN; i++)
        di |= (uint32_t)(uint8_t)(p[i] - 0x33) << (8 * i);
    return di;
}

/* BCD 解码：低字节先，每字节两位十进制 */
static int32_t _bcd_to_int(const uint8_t *p, size_t n) {
    int32_t v = 0;
    int32_t mul = 1;
    for (size_t i = 0; i < n; i++) {
        uint8_t bcd = (uint8_t)(p[i] - 0x33);
        v += (int32_t)((bcd >> 4) * 10 + (bcd & 0x0F)) * mul;
        mul *= 100;
    }
    return v;
}

static void _int_to_bcd(struct net_buf_simple *b, int32_t v, size_t n) {
    for (size_t i = 0; i < n; i++) {
        int32_t d = v % 100;
        if (d < 0) d = 0;
        uint8_t bcd = (uint8_t)(((d / 10) << 4) | (d % 10));
        net_buf_simple_add_u8(b, (uint8_t)(bcd + 0x33));
        v /= 100;
    }
}

/* 收尾：CS + 0x16 */
static void _seal(struct net_buf_simple *b) {
    net_buf_simple_add_u8(b, _cs(b->data, b->len));
    net_buf_simple_add_u8(b, 0x16);
}

/* 公共帧头：68 + 地址(6) + 68 + 控制码 + L */
static void _put_head(struct net_buf_simple *b, const uint8_t *addr, uint8_t ctrl, uint8_t l) {
    net_buf_simple_add_u8(b, 0x68);
    net_buf_simple_add_mem(b, addr, DLT645_ADDR_LEN);
    net_buf_simple_add_u8(b, 0x68);
    net_buf_simple_add_u8(b, ctrl);
    net_buf_simple_add_u8(b, l);
}

/* ==================== 主站侧：组请求 ==================== */

int dlt645_encode_read(const dlt645_req_t *req, net_buf_t *out) {
    if (!req || !out) return -1;
    struct net_buf_simple *b = &out->b;
    _put_head(b, req->addr_bcd, DLT645_CTRL_READ, DLT645_DI_LEN);
    _put_di(b, req->di);
    _seal(b);
    return 0;
}

int dlt645_encode_write(const dlt645_req_t *req, net_buf_t *out) {
    if (!req || !out) return -1;
    if (req->data_len != 2 && req->data_len != 4) return -1;

    struct net_buf_simple *b = &out->b;
    _put_head(b, req->addr_bcd, DLT645_CTRL_WRITE, (uint8_t)(DLT645_DI_LEN + req->data_len));
    _put_di(b, req->di);
    /* 写入值按 raw（未标定整数）编码；调用方负责把工程量换成 raw */
    _int_to_bcd(b, req->raw, req->data_len);
    _seal(b);
    return 0;
}

/* ==================== 主站侧：解应答 ==================== */

/* 主站应答的公共前半段：同步、验长、验和。成功时 req->ctrl/di 已填好 */
static int _decode_rsp_head(dlt645_req_t *req, const uint8_t *buf, size_t len, size_t *consumed,
                            uint8_t *out_l, uint8_t *out_ctrl) {
    size_t skip = _sync(buf, len);
    if (skip > 0) {
        *consumed = skip;
        return 1; /* 让调用方先丢掉垃圾字节 */
    }
    if (len < 12) return EDGE_ERR_INCOMPLETE;
    if (buf[7] != 0x68) { /* 第二个 68 不对：跳一格重找，不要当整帧丢掉 */
        *consumed = 1;
        return 1;
    }

    uint8_t l = 0;
    size_t total = 0;
    int rc = _frame_len(buf, len, &l, &total);
    if (rc != 0) return rc;

    *consumed = total;
    if (_cs(buf, (size_t)l + 10) != buf[l + 10]) return -1; /* 校验和错 */

    *out_l = l;
    *out_ctrl = buf[8];
    req->ctrl = buf[8];
    if (l >= DLT645_DI_LEN) req->di = _get_di(&buf[10]);
    return 0;
}

int dlt645_decode_read_rsp(dlt645_req_t *req, const uint8_t *buf, size_t len, size_t *consumed) {
    if (!req || !buf || !consumed) return -1;
    *consumed = 0;

    uint8_t l = 0, ctrl = 0;
    int rc = _decode_rsp_head(req, buf, len, consumed, &l, &ctrl);
    if (rc != 0) return rc;

    if (ctrl != DLT645_CTRL_READ_RSP) return -1;
    if (l < DLT645_DI_LEN) return -1;

    req->data_len = (uint8_t)(l - DLT645_DI_LEN);
    req->raw = req->data_len ? _bcd_to_int(&buf[14], req->data_len) : 0;
    req->value = (double)req->raw;
    return 0;
}

int dlt645_decode_write_rsp(dlt645_req_t *req, const uint8_t *buf, size_t len, size_t *consumed) {
    if (!req || !buf || !consumed) return -1;
    *consumed = 0;

    uint8_t l = 0, ctrl = 0;
    int rc = _decode_rsp_head(req, buf, len, consumed, &l, &ctrl);
    if (rc != 0) return rc;

    /* 写应答只表示"收到/成功"，不回值；异常应答单独识别 */
    if (ctrl == DLT645_CTRL_ABNORMAL) return -2;
    if (ctrl != DLT645_CTRL_WRITE_RSP) return -1;
    return 0;
}

/* ==================== 从站侧 ==================== */

int dlt645_decode_slave_req(dlt645_req_t *req, const uint8_t *buf, size_t len, size_t *consumed) {
    if (!req || !buf || !consumed) return -1;
    *consumed = 0;

    size_t skip = _sync(buf, len);
    if (skip > 0) {
        *consumed = skip;
        return 1;
    }
    if (len < 12) return EDGE_ERR_INCOMPLETE;
    if (buf[7] != 0x68) {
        *consumed = 1;
        return 1;
    }

    uint8_t l = 0;
    size_t total = 0;
    int rc = _frame_len(buf, len, &l, &total);
    if (rc != 0) return rc;

    *consumed = total;
    if (_cs(buf, (size_t)l + 10) != buf[l + 10]) return -1;

    req->ctrl = buf[8];
    if (l < DLT645_DI_LEN) return -1;
    req->di = _get_di(&buf[10]);
    memcpy(req->addr_bcd, &buf[1], DLT645_ADDR_LEN);
    req->data_len = (uint8_t)(l - DLT645_DI_LEN);
    if (req->data_len) req->raw = _bcd_to_int(&buf[14], req->data_len);
    return 0;
}

int dlt645_encode_slave_rsp(const dlt645_req_t *req, net_buf_t *out) {
    if (!req || !out) return -1;
    if (req->data_len != 2 && req->data_len != 4) return -1;

    struct net_buf_simple *b = &out->b;
    _put_head(b, req->addr_bcd, DLT645_CTRL_READ_RSP, (uint8_t)(DLT645_DI_LEN + req->data_len));
    _put_di(b, req->di);
    /* 从站侧的值由调用方（cmd 的点位模型）经 edge_slave_source_ops_t 给到 raw */
    _int_to_bcd(b, req->raw, req->data_len);
    _seal(b);
    return 0;
}

/* ==================== 挂到 edge_driver_t（request-native） ==================== */

static int dlt645_req_prepare(void *priv, uint32_t slave_id, uint32_t point_id,
                              const void *meta_list, uint32_t meta_list_len) {
    dlt645_req_t *p = (dlt645_req_t *)priv;
    if (!p) return -1;

    /* 表地址来自设备配置：meta_list 是按 slave_id 索引的 6 字节 BCD 数组。
     * 缺失时回落广播地址 0x99×6，与旧行为一致。 */
    memset(p->addr_bcd, 0x99, DLT645_ADDR_LEN);
    if (meta_list && slave_id < meta_list_len) {
        memcpy(p->addr_bcd, ((const uint8_t (*)[DLT645_ADDR_LEN])meta_list)[slave_id],
               DLT645_ADDR_LEN);
    }
    p->di = point_id;
    p->ctrl = DLT645_CTRL_READ;
    p->data_len = 0;
    p->raw = 0;
    p->value = 0.0;
    return 0;
}

static int dlt645_req_encode(void *priv, net_buf_t *out) {
    dlt645_req_t *p = (dlt645_req_t *)priv;
    if (!p) return -1;
    if (p->ctrl == DLT645_CTRL_WRITE) return dlt645_encode_write(p, out);
    return dlt645_encode_read(p, out);
}

static int dlt645_req_decode(void *priv, const uint8_t *buf, size_t len, size_t *consumed,
                             double *out_value) {
    dlt645_req_t *p = (dlt645_req_t *)priv;
    if (!p) return -1;

    int rc;
    if (p->ctrl == DLT645_CTRL_WRITE) {
        rc = dlt645_decode_write_rsp(p, buf, len, consumed);
        if (rc == 0 && out_value) *out_value = 0.0;
        return rc;
    }
    rc = dlt645_decode_read_rsp(p, buf, len, consumed);
    if (rc == 0 && out_value) *out_value = (double)p->raw; /* 标定归上层 */
    return rc;
}

const edge_driver_t dlt645_driver = {
    .name = "DLT645",
    .ctx_size = 0, /* request-native：驱动不再需要 per-session 上下文 */
    .req_priv_size = sizeof(dlt645_req_t),
    .req_prepare = dlt645_req_prepare,
    .req_encode = dlt645_req_encode,
    .req_decode = dlt645_req_decode,
};

int dlt645_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &dlt645_driver);
}
