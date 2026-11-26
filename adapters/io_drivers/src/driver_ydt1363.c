/**
 * @file driver_ydt1363.c
 * @brief YD/T 1363-2014 只读 request-native 驱动（首切片）
 *
 * 驱动里没有 tag_id、没有点表、没有 datahub：帧参数与取值窗口全部来自
 * 请求描述，帧的校验与 INFO 解码复用 ydt1363_codec。
 */

#include "driver_ydt1363.h"

#include <stdbool.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_common.h"
#include "edge_driver.h"
#include "edge_log.h"

/* 首切片单点读的应答 INFO 只有几个字节；2047 字节的块读落地时再按帧长放宽。 */
#define YDT1363_MAX_RSP_INFO 64

typedef struct {
    ydt1363_point_desc_t desc; /**< req_prepare 时拷入，生命周期不依赖调用方 */
} ydt1363_req_t;

static int _order_width(uint8_t order) {
    switch (order) {
        case YDT1363_ORDER_A:
            return 1;
        case YDT1363_ORDER_AB:
        case YDT1363_ORDER_BA:
            return 2;
        case YDT1363_ORDER_ABCD:
        case YDT1363_ORDER_DCBA:
            return 4;
        default:
            return 0;
    }
}

/* codec 只回 INFO 字节；ADR 是应答归属校验，必须直接读已通过整帧校验的字段。 */
static int _hex_nibble(uint8_t c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static int _frame_u8(const uint8_t *p) {
    int hi = _hex_nibble(p[0]);
    int lo = _hex_nibble(p[1]);
    if (hi < 0 || lo < 0) return -1;
    return (hi << 4) | lo;
}

static int _decode_value(const ydt1363_point_desc_t *desc, const uint8_t *info, size_t info_len,
                         double *out_value) {
    int width = _order_width(desc->order);
    if (width == 0 || (size_t)desc->info_offset + (size_t)width > info_len) return -1;

    const uint8_t *p = info + desc->info_offset;
    uint32_t raw;
    switch (desc->order) {
        case YDT1363_ORDER_A:
            raw = p[0];
            break;
        case YDT1363_ORDER_AB:
            raw = ((uint32_t)p[0] << 8) | p[1];
            break;
        case YDT1363_ORDER_BA:
            raw = ((uint32_t)p[1] << 8) | p[0];
            break;
        case YDT1363_ORDER_ABCD:
            raw = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
            break;
        case YDT1363_ORDER_DCBA:
            raw = ((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16) | ((uint32_t)p[1] << 8) | p[0];
            break;
        default:
            return -1;
    }

    double value;
    if (desc->type == YDT1363_TYPE_INT) {
        if (width == 1 && (raw & 0x80u)) raw |= 0xFFFFFF00u;
        if (width == 2 && (raw & 0x8000u)) raw |= 0xFFFF0000u;
        value = (double)(int32_t)raw;
    } else if (desc->type == YDT1363_TYPE_UINT) {
        value = (double)raw;
    } else {
        return -1;
    }
    if (out_value) *out_value = value;
    return 0;
}

int ydt1363_encode_read(const ydt1363_point_desc_t *desc, net_buf_t *out) {
    if (!desc || !out) return -1;
    if (ydt1363_encode(desc->ver, desc->adr, desc->cid1, desc->cid2, desc->info, desc->info_len,
                       out) != YDT1363_OK) {
        return -1;
    }
    return 0;
}

int ydt1363_decode_rsp(const ydt1363_point_desc_t *desc, const uint8_t *buf, size_t len,
                       size_t *consumed, double *out_value) {
    if (!desc || !buf || !consumed) return -1;
    *consumed = 0;
    if (len == 0) return EDGE_ERR_INCOMPLETE;

    size_t so = 0;
    while (so < len && buf[so] != YDT1363_SOI) so++;
    if (so > 0) {
        *consumed = so; /* 无 SOI：整段垃圾都可丢 */
        return EDGE_ERR_INCOMPLETE;
    }

    int total = ydt1363_frame_len(buf, len);
    if (total < 0) {
        *consumed = 1; /* 头部已可判坏：只丢 1 字节重新扫 SOI */
        return EDGE_ERR_INCOMPLETE;
    }
    if (total == 0 || (size_t)total > len) return EDGE_ERR_INCOMPLETE;

    uint8_t info[YDT1363_MAX_RSP_INFO];
    size_t info_len = 0;
    uint8_t rtn = 0;
    int rc = ydt1363_decode(buf, (size_t)total, info, sizeof(info), &info_len, &rtn);
    if (rc == YDT1363_ERR_RTN) {
        LOG_WARN(LOG_CAT_IO, "YDT1363: RTN 0x%02X", rtn);
        *consumed = (size_t)total;
        return -1;
    }
    if (rc == YDT1363_ERR_SIZE) {
        *consumed = (size_t)total; /* 帧完好但超出首切片窗口，重试无益 */
        return -1;
    }
    if (rc != YDT1363_OK) {
        *consumed = 1; /* CHKSUM/格式坏帧：丢 1 字节，后面的好帧还能找回 */
        return EDGE_ERR_INCOMPLETE;
    }

    int adr = _frame_u8(&buf[3]);
    if (adr < 0 || (uint8_t)adr != desc->adr) {
        *consumed = 1; /* 不是本机的应答 */
        return EDGE_ERR_INCOMPLETE;
    }

    if (_decode_value(desc, info, info_len, out_value) != 0) {
        *consumed = (size_t)total; /* 帧合法但点位窗口越界：配置错，重试无益 */
        return -1;
    }
    *consumed = (size_t)total;
    return 0;
}

/* ==================== request-native 四件套（首切片只读） ==================== */

static int ydt1363_req_prepare(void *priv, uint32_t slave_id, uint32_t point_id,
                               const void *meta_list, uint32_t meta_list_len,
                               const void *point_desc, size_t point_desc_len, bool is_write,
                               double value) {
    (void)slave_id; /* ADR 由请求描述给，设备级参数不在首切片里拆表 */
    (void)point_id;
    (void)meta_list;
    (void)meta_list_len;
    (void)value;

    ydt1363_req_t *p = (ydt1363_req_t *)priv;
    if (!p) return -1;
    if (is_write) {
        LOG_WARN(LOG_CAT_IO, "YDT1363: 首切片只读，拒绝写请求");
        return -1;
    }
    if (!point_desc || point_desc_len != sizeof(ydt1363_point_desc_t)) {
        LOG_WARN(LOG_CAT_IO, "YDT1363: 缺少点位描述或尺寸不符");
        return -1;
    }

    const ydt1363_point_desc_t *desc = (const ydt1363_point_desc_t *)point_desc;
    int width = _order_width(desc->order);
    if (desc->adr == 0 || desc->adr == 0xFF || desc->info_len > YDT1363_MAX_REQ_INFO ||
        width == 0 || (desc->type != YDT1363_TYPE_UINT && desc->type != YDT1363_TYPE_INT) ||
        (size_t)desc->info_offset + (size_t)width > YDT1363_MAX_INFO) {
        LOG_WARN(LOG_CAT_IO, "YDT1363: 点位描述非法");
        return -1;
    }

    p->desc = *desc;
    return 0;
}

static int ydt1363_req_encode(void *priv, void *session, net_buf_t *out) {
    (void)session; /* 无会话状态：一帧自成一体 */
    ydt1363_req_t *p = (ydt1363_req_t *)priv;
    if (!p) return -1;
    return ydt1363_encode_read(&p->desc, out);
}

static int ydt1363_req_decode(void *priv, void *session, const uint8_t *buf, size_t len,
                              size_t *consumed, double *out_value) {
    (void)session; /* 无会话状态：一帧自成一体 */
    ydt1363_req_t *p = (ydt1363_req_t *)priv;
    if (!p) return -1;
    return ydt1363_decode_rsp(&p->desc, buf, len, consumed, out_value);
}

const edge_driver_t ydt1363_driver = {
    .name = "YDT1363",
    .ctx_size = 0, /* 无跨帧状态 */
    .req_priv_size = sizeof(ydt1363_req_t),
    .req_prepare = ydt1363_req_prepare,
    .req_encode = ydt1363_req_encode,
    .req_decode = ydt1363_req_decode,
};

/* File-local prototypes. */
int ydt1363_driver_register(driver_manager_t *mgr);
int ydt1363_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &ydt1363_driver);
}
