#include "dlms_hdlc.h"

#include <string.h>

#include "edge_common.h"
#include "edge_log.h"

void dlms_hdlc_reset(dlms_hdlc_manager_t *mgr) {
    if (!mgr) return;
    if (mgr->last_tx_frame) {
        net_buf_unref(mgr->last_tx_frame);
        mgr->last_tx_frame = NULL;
    }
    if (mgr->reassembly_buf) {
        net_buf_unref(mgr->reassembly_buf);
        mgr->reassembly_buf = NULL;
    }
    mgr->state = HDLC_STATE_DISCONNECTED;
    mgr->ns = 0;
    mgr->nr = 0;
}

int dlms_hdlc_build_snrm(dlms_hdlc_manager_t *mgr, net_buf_t *out) {
    if (!mgr || !out) return -1;
    struct net_buf_simple *b = &out->b;

    net_buf_simple_add_u8(b, 0x7E);
    uint8_t *f_ptr = net_buf_simple_add(b, 2);
    net_buf_simple_add_u8(b, (uint8_t)((mgr->server_addr << 1) | 1));
    net_buf_simple_add_u8(b, (uint8_t)((mgr->client_addr << 1) | 1));
    net_buf_simple_add_u8(b, 0x93);  // SNRM control byte

    uint16_t frame_len = (uint16_t)(b->len + 2 - 1);
    f_ptr[0] = (uint8_t)(0xA0 | (frame_len >> 8));
    f_ptr[1] = (uint8_t)(frame_len & 0xFF);

    uint16_t hcs = edge_crc16_ccitt(b->data + 1, b->len - 1);
    net_buf_simple_add_le16(b, hcs);
    net_buf_simple_add_u8(b, 0x7E);

    return 0;
}

static int _parse_hdlc_addr(const uint8_t *ptr, uint32_t *addr) {
    uint32_t val = 0;
    int len = 0;
    do {
        if (len >= 4) return -1;
        val = (val << 7) | (ptr[len] >> 1);
        len++;
    } while (!(ptr[len - 1] & 0x01));
    *addr = val;
    return len;
}

int dlms_hdlc_packetize(dlms_hdlc_manager_t *mgr, net_buf_t *apdu, size_t offset, size_t mtu,
                        net_buf_t *out) {
    struct net_buf_simple *b = &out->b;
    size_t remaining = apdu->b.len - offset;
    size_t chunk = (remaining > (mtu - 13)) ? (mtu - 13) : remaining;

    if (net_buf_simple_tailroom(b) < chunk + 13) {
        LOG_ERROR(LOG_CAT_IO, "DLMS HDLC: Out buf too small");
        return -1;
    }

    net_buf_simple_add_u8(b, 0x7E);
    uint8_t *f_ptr = net_buf_simple_add(b, 2);
    net_buf_simple_add_u8(b, (uint8_t)((mgr->server_addr << 1) | 1));
    net_buf_simple_add_u8(b, (uint8_t)((mgr->client_addr << 1) | 1));

    // 控制域：编码 N(S) 和 N(R)
    uint8_t control = (uint8_t)((mgr->nr << 5) | (mgr->ns << 1));

    // Segmentation 处理：如果是最后一帧，设置 P/F 位 (0x10)
    bool is_last = (offset + chunk >= apdu->b.len);
    if (is_last) control |= 0x10;
    net_buf_simple_add_u8(b, control);

    uint16_t frame_len = (uint16_t)(b->len + chunk + 2 - 1);
    f_ptr[0] = (uint8_t)(0xA0 | (frame_len >> 8));
    f_ptr[1] = (uint8_t)(frame_len & 0xFF);

    uint16_t hcs = edge_crc16_ccitt(b->data + 1, b->len - 1);
    net_buf_simple_add_le16(b, hcs);

    net_buf_simple_add_mem(b, apdu->b.data + offset, chunk);
    uint16_t fcs = edge_crc16_ccitt(b->data + 1, b->len - 1);
    net_buf_simple_add_le16(b, fcs);
    net_buf_simple_add_u8(b, 0x7E);

    if (is_last) {
        mgr->ns = (uint8_t)((mgr->ns + 1) % 8);
    }
    return (int)chunk;
}

int dlms_hdlc_depacketize(dlms_hdlc_manager_t *mgr, struct net_buf_simple *in, net_buf_pool_t *pool,
                          net_buf_t **out_apdu) {
    while (in->len > 0 && in->data[0] == 0x7E) net_buf_simple_pull(in, 1);
    if (in->len < 9) return 1;

    uint8_t *f = in->data;
    uint16_t f_len = (uint16_t)(((f[0] & 0x07) << 8) | f[1]);
    if (in->len < f_len) return 1;  // 报文不全

    // 基础校验 (简化版：假设地址均为1字节)
    uint8_t control = f[4];
    int h_l = 5;  // 头长度 (68 LL CC SS Ctrl)

    // 检查 HCS
    uint16_t hcs_rx = (uint16_t)(f[h_l] | (f[h_l + 1] << 8));
    if (edge_crc16_ccitt(f, (size_t)h_l) != hcs_rx) return -2;

    size_t p_len = (size_t)f_len - (size_t)h_l - 4;  // 减去头、HCS、FCS、7E
    if (p_len > 0) {
        if (!mgr->reassembly_buf) {
            mgr->reassembly_buf = net_buf_alloc(pool, 0);
        }
        net_buf_simple_add_mem(&mgr->reassembly_buf->b, f + h_l + 2, p_len);
    }

    net_buf_simple_pull(in, f_len);

    // 如果设置了 P/F 位 (0x10)，说明是最后一段或单帧
    if (control & 0x10) {
        *out_apdu = mgr->reassembly_buf;
        mgr->reassembly_buf = NULL;
        return 0;
    }

    return 1;  // 还有后续帧
}

int dlms_hdlc_send_iframe(dlms_hdlc_manager_t *mgr, net_buf_t *apdu, net_buf_t *out) {
    return dlms_hdlc_packetize(mgr, apdu, 0, 128, out);
}

int dlms_hdlc_handle_ack(dlms_hdlc_manager_t *mgr, uint8_t control_byte) {
    uint8_t ack_seq = (uint8_t)((control_byte >> 5) & 0x07);
    if (ack_seq == ((mgr->ns + 1) & 0x07)) {
        mgr->ns = (uint8_t)((mgr->ns + 1) & 0x07);
        mgr->state = HDLC_STATE_CONNECTED;
        if (mgr->last_tx_frame) {
            net_buf_unref(mgr->last_tx_frame);
            mgr->last_tx_frame = NULL;
        }
        return 0;
    }
    return -1;
}