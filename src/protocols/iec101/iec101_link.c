/**
 * @file iec101_link.c
 * @brief IEC 101 链路层 FT1.2 帧格式实现 (Phase 5 Task 4 - Parsing & Validation)
 */

#include "iec101_link.h"

#include <string.h>

uint8_t iec101_calc_sum8(const uint8_t *data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}

int iec101_link_build_fixed(iec101_link_mgr_t *mgr, iec101_ctrl_t ctrl, net_buf_t *out) {
    struct net_buf_simple *b = &out->b;
    net_buf_simple_reset(b);

    net_buf_simple_add_u8(b, 0x10);
    uint8_t *start = net_buf_simple_add(b, 0);

    net_buf_simple_add_u8(b, *(uint8_t *)&ctrl);

    if (mgr->addr_len == 1) {
        net_buf_simple_add_u8(b, (uint8_t)(mgr->link_addr & 0xFF));
    } else {
        net_buf_simple_add_le16(b, mgr->link_addr);
    }

    size_t data_len = (size_t)(b->data + b->len - start);
    net_buf_simple_add_u8(b, iec101_calc_sum8(start, data_len));
    net_buf_simple_add_u8(b, 0x16);

    return 0;
}

int iec101_link_wrap_asdu(iec101_link_mgr_t *mgr, iec101_ctrl_t ctrl, net_buf_t *asdu) {
    size_t asdu_len = asdu->b.len;
    uint8_t header_len = (uint8_t)(1 + mgr->addr_len);
    uint8_t total_user_len = (uint8_t)(header_len + asdu_len);

    uint8_t *p = net_buf_push(asdu, (size_t)(4 + header_len));

    p[0] = 0x68;
    p[1] = total_user_len;
    p[2] = total_user_len;
    p[3] = 0x68;
    p[4] = *(uint8_t *)&ctrl;

    if (mgr->addr_len == 1) {
        p[5] = (uint8_t)(mgr->link_addr & 0xFF);
    } else {
        uint16_t le_addr = mgr->link_addr;
        memcpy(&p[5], &le_addr, 2);
    }

    uint8_t cs = iec101_calc_sum8(&p[4], total_user_len);
    net_buf_simple_add_u8(&asdu->b, cs);
    net_buf_simple_add_u8(&asdu->b, 0x16);

    return 0;
}

int iec101_link_parse(iec101_link_mgr_t *mgr, struct net_buf_simple *in, size_t *consumed,
                      iec101_ctrl_t *out_ctrl, struct net_buf_simple *out_asdu) {
    *consumed = 0;
    if (in->len < 1) return IEC101_FRAME_INCOMPLETE;

    uint8_t start_char = in->data[0];

    // --- 1. 固定长度帧 (0x10) ---
    if (start_char == 0x10) {
        size_t expected_len = (size_t)(1 + 1 + mgr->addr_len + 1 + 1);
        if (in->len < expected_len) return IEC101_FRAME_INCOMPLETE;

        *consumed = expected_len;
        uint8_t cs_calc = iec101_calc_sum8(&in->data[1], 1 + mgr->addr_len);
        if (in->data[expected_len - 2] != cs_calc) return IEC101_FRAME_ERR_CS;
        if (in->data[expected_len - 1] != 0x16) return IEC101_FRAME_ERR_FORMAT;

        if (out_ctrl) *out_ctrl = *(iec101_ctrl_t *)&in->data[1];
        if (out_asdu) {
            out_asdu->data = NULL;
            out_asdu->len = 0;
        }
        return IEC101_FRAME_OK;
    }

    // --- 2. 可变长度帧 (0x68) ---
    if (start_char == 0x68) {
        if (in->len < 4) return IEC101_FRAME_INCOMPLETE;
        uint8_t len1 = in->data[1];
        uint8_t len2 = in->data[2];
        if (len1 != len2 || in->data[3] != 0x68) {
            *consumed = 1;  // 搜寻下一个起始位
            return IEC101_FRAME_ERR_FORMAT;
        }

        size_t total_len = (size_t)(len1 + 6);
        if (in->len < total_len) return IEC101_FRAME_INCOMPLETE;

        *consumed = total_len;
        uint8_t cs_calc = iec101_calc_sum8(&in->data[4], len1);
        if (in->data[total_len - 2] != cs_calc) return IEC101_FRAME_ERR_CS;
        if (in->data[total_len - 1] != 0x16) return IEC101_FRAME_ERR_FORMAT;

        if (out_ctrl) *out_ctrl = *(iec101_ctrl_t *)&in->data[4];
        if (out_asdu) {
            size_t header_len = (size_t)(1 + mgr->addr_len);
            out_asdu->data = &in->data[4 + header_len];
            out_asdu->len = (uint16_t)(len1 - header_len);
        }
        return IEC101_FRAME_OK;
    }

    // 非法起始符
    *consumed = 1;
    return IEC101_FRAME_ERR_FORMAT;
}