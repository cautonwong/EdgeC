/**
 * @file dcu_698_devmgr.c
 * @brief Class 19 (0x4300 电气设备) 设备管理软件面领域引擎实现
 */

#include "dcu_698_devmgr.h"

#include <string.h>

#include "dlt698_codec.h"
#include "edge_slave_plane.h"

static const dcu_devmgr_info_t k_default_info = {
    .descriptor = "EdgeC-DCU-4300",
    .vendor_code = "EDGC",
    .sw_version = "V1.0",
    .sw_date = "261128",
    .hw_version = "H1.0",
    .hw_date = "260101",
    .vendor_ext = "EDGC0001",
    .prod_year = 2026,
    .prod_month = 11,
    .prod_day = 28,
    .prod_hour = 0,
    .prod_min = 0,
    .prod_sec = 0,
    .protocol_count = 2,
    .protocols = {"DL/T698.45", "DL/T645-2007"},
};

static dcu_devmgr_info_t g_devmgr_info = {
    .descriptor = "EdgeC-DCU-4300",
    .vendor_code = "EDGC",
    .sw_version = "V1.0",
    .sw_date = "261128",
    .hw_version = "H1.0",
    .hw_date = "260101",
    .vendor_ext = "EDGC0001",
    .prod_year = 2026,
    .prod_month = 11,
    .prod_day = 28,
    .prod_hour = 0,
    .prod_min = 0,
    .prod_sec = 0,
    .protocol_count = 2,
    .protocols = {"DL/T698.45", "DL/T645-2007"},
};

const dcu_devmgr_info_t *dcu_698_devmgr_get_info(void) {
    return &g_devmgr_info;
}

void dcu_698_devmgr_set_info(const dcu_devmgr_info_t *info) {
    if (!info) return;
    g_devmgr_info = *info;
    if (g_devmgr_info.protocol_count > DCU_DEVMGR_MAX_PROTOCOLS) {
        g_devmgr_info.protocol_count = DCU_DEVMGR_MAX_PROTOCOLS;
    }
}

void dcu_698_devmgr_reset_info(void) {
    g_devmgr_info = k_default_info;
}

static int _put_visible_string(uint8_t *buf, size_t cap, size_t *off, const char *s,
                               size_t max_chars) {
    size_t slen = s ? strnlen(s, max_chars) : 0;
    if (slen > 127u || *off + 2u + slen > cap) return -1;
    buf[(*off)++] = (uint8_t)DLT698_TAG_VISIBLE_STRING;
    buf[(*off)++] = (uint8_t)slen;
    if (slen > 0) {
        memcpy(buf + *off, s, slen);
        *off += slen;
    }
    return 0;
}

static int _put_container_head(uint8_t *buf, size_t cap, size_t *off, uint8_t tag, uint8_t count) {
    if (*off + 2u > cap) return -1;
    buf[(*off)++] = tag;
    buf[(*off)++] = count;
    return 0;
}

static int _encode_a3_version(const dcu_devmgr_info_t *info, uint8_t *buf, size_t cap,
                              size_t *off) {
    if (_put_container_head(buf, cap, off, (uint8_t)DLT698_TAG_STRUCTURE, 6u) != 0) return -1;
    if (_put_visible_string(buf, cap, off, info->vendor_code, 4u) != 0) return -1;
    if (_put_visible_string(buf, cap, off, info->sw_version, 4u) != 0) return -1;
    if (_put_visible_string(buf, cap, off, info->sw_date, 6u) != 0) return -1;
    if (_put_visible_string(buf, cap, off, info->hw_version, 4u) != 0) return -1;
    if (_put_visible_string(buf, cap, off, info->hw_date, 6u) != 0) return -1;
    if (_put_visible_string(buf, cap, off, info->vendor_ext, 8u) != 0) return -1;
    return 0;
}

static int _encode_a4_prod_date(const dcu_devmgr_info_t *info, uint8_t *buf, size_t cap,
                                size_t *off) {
    if (*off + 8u > cap) return -1;
    buf[(*off)++] = (uint8_t)DLT698_TAG_DATE_TIME_S;
    buf[(*off)++] = (uint8_t)(info->prod_year >> 8);
    buf[(*off)++] = (uint8_t)(info->prod_year & 0xFFu);
    buf[(*off)++] = info->prod_month;
    buf[(*off)++] = info->prod_day;
    buf[(*off)++] = info->prod_hour;
    buf[(*off)++] = info->prod_min;
    buf[(*off)++] = info->prod_sec;
    return 0;
}

static int _encode_a5_protocols(const dcu_devmgr_info_t *info, uint8_t *buf, size_t cap,
                                size_t *off) {
    uint8_t count = info->protocol_count;
    if (count > DCU_DEVMGR_MAX_PROTOCOLS) count = DCU_DEVMGR_MAX_PROTOCOLS;
    if (_put_container_head(buf, cap, off, (uint8_t)DLT698_TAG_ARRAY, count) != 0) return -1;
    for (uint8_t i = 0; i < count; i++) {
        if (_put_visible_string(buf, cap, off, info->protocols[i], DCU_DEVMGR_PROTOCOL_LEN - 1u) !=
            0) {
            return -1;
        }
    }
    return 0;
}

int dcu_698_devmgr_encode_attr(uint8_t attr_id, uint8_t *buf, size_t cap, size_t *out_len) {
    if (out_len) *out_len = 0;
    if (!buf || !out_len || cap == 0) return -1;

    const dcu_devmgr_info_t *info = &g_devmgr_info;
    size_t off = 0;
    int rc = 0;

    switch (attr_id) {
        case 2:
            rc = _put_visible_string(buf, cap, &off, info->descriptor,
                                     sizeof(info->descriptor) - 1u);
            break;
        case 3:
            rc = _encode_a3_version(info, buf, cap, &off);
            break;
        case 4:
            rc = _encode_a4_prod_date(info, buf, cap, &off);
            break;
        case 5:
            rc = _encode_a5_protocols(info, buf, cap, &off);
            break;
        default:
            return EDGE_SLAVE_SRC_UNDEFINED;
    }

    if (rc != 0) return -1;
    *out_len = off;
    return 0;
}

int dcu_698_devmgr_call(uint8_t method_id, const dcu_devmgr_ops_t *ops, void *ctx) {
    switch (method_id) {
        case 1:
        case 2:
        case 4:
            /* 无硬件/平台复位与出厂镜像端口时诚实返回 DAR 5，绝不伪造成功。 */
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
        case 3:
            if (!ops || !ops->data_init) return EDGE_SLAVE_SRC_CLASS_MISMATCH;
            return ops->data_init(ctx);
        case 5:
            if (!ops || !ops->event_init) return EDGE_SLAVE_SRC_CLASS_MISMATCH;
            return ops->event_init(ctx);
        case 6:
            if (!ops || !ops->demand_init) return EDGE_SLAVE_SRC_CLASS_MISMATCH;
            return ops->demand_init(ctx);
        default:
            return EDGE_SLAVE_SRC_UNDEFINED;
    }
}
