/**
 * @file iec101_link.c
 * @brief IEC 101 链路层 FT1.2 帧格式实现
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
    uint8_t *start = net_buf_simple_add(b, 0);  // 记录校验起始点

    net_buf_simple_add_u8(b, *(uint8_t *)&ctrl);

    // 链路地址
    if (mgr->addr_len == 1) {
        net_buf_simple_add_u8(b, (uint8_t)(mgr->link_addr & 0xFF));
    } else {
        net_buf_simple_add_le16(b, mgr->link_addr);
    }

    size_t data_len = (size_t)(b->data + b->len - (uint8_t *)start);
    net_buf_simple_add_u8(b, iec101_calc_sum8((uint8_t *)start, data_len));
    net_buf_simple_add_u8(b, 0x16);

    return 0;
}

int iec101_link_wrap_asdu(iec101_link_mgr_t *mgr, iec101_ctrl_t ctrl, net_buf_t *asdu) {
    size_t asdu_len = asdu->b.len;
    uint8_t header_len = (uint8_t)(1 + mgr->addr_len);  // Ctrl + Addr
    uint8_t total_user_len = (uint8_t)(header_len + asdu_len);

    // 使用 net_buf_push 在头部“生长”：0x68 L L 0x68 Ctrl Addr
    uint8_t *p = net_buf_push(asdu, (size_t)(4 + header_len));

    p[0] = 0x68;
    p[1] = total_user_len;
    p[2] = total_user_len;
    p[3] = 0x68;
    p[4] = *(uint8_t *)&ctrl;

    if (mgr->addr_len == 1) {
        p[5] = (uint8_t)(mgr->link_addr & 0xFF);
    } else {
        uint16_t le_addr = mgr->link_addr;  // 简化处理
        memcpy(&p[5], &le_addr, 2);
    }

    // 尾部追加校验和 0x16
    uint8_t cs = iec101_calc_sum8(&p[4], total_user_len);
    net_buf_simple_add_u8(&asdu->b, cs);
    net_buf_simple_add_u8(&asdu->b, 0x16);

    return 0;
}
