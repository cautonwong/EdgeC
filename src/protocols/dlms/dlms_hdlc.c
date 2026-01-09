#include "dlms_hdlc.h"

#include <string.h>

#include "edge_common.h"
#include "edge_log.h"

// --- HDLC Control Field Definitions ---
#define HDLC_CTRL_RR 0x01
#define HDLC_CTRL_RNR 0x05
#define HDLC_CTRL_SNRM 0x93
#define HDLC_CTRL_DISC 0x53
#define HDLC_CTRL_UA 0x73
#define HDLC_CTRL_DM 0x1F
#define HDLC_CTRL_UI 0x03
#define HDLC_CTRL_FRMR 0x97

// --- 辅助：编码变长地址 (1, 2, 4 字节) ---
static int _encode_hdlc_addr(uint32_t addr, uint8_t *out) {
    if (addr <= 0x7F) {
        out[0] = (uint8_t)((addr << 1) | 1);
        return 1;
    } else if (addr <= 0x3FFF) {
        out[0] = (uint8_t)(addr >> 7 << 1);
        out[1] = (uint8_t)((addr & 0x7F) << 1 | 1);
        return 2;
    } else {
        out[0] = (uint8_t)(addr >> 21 << 1);
        out[1] = (uint8_t)((addr >> 14 & 0x7F) << 1);
        out[2] = (uint8_t)((addr >> 7 & 0x7F) << 1);
        out[3] = (uint8_t)((addr & 0x7F) << 1 | 1);
        return 4;
    }
}

/**
 * @brief 构建基础 HDLC 帧头部 (包含 7E, Format, Addr, Ctrl)
 */
static uint8_t *_build_hdlc_header(dlms_hdlc_manager_t *mgr, uint8_t ctrl, uint16_t info_len,
                                   bool segment, net_buf_t *out) {
    struct net_buf_simple *b = &out->b;
    net_buf_simple_add_u8(b, 0x7E);
    uint8_t *f_ptr = net_buf_simple_add(b, 2);

    // 写入地址 (此处简化为 1B)
    net_buf_simple_add_u8(b, (uint8_t)((mgr->server_addr << 1) | 1));
    net_buf_simple_add_u8(b, (uint8_t)((mgr->client_addr << 1) | 1));
    net_buf_simple_add_u8(b, ctrl);

    // 计算 Frame Format (Bit 11 是 S 位)
    uint16_t frame_len = (uint16_t)(b->len + info_len + 2 - 1);
    uint16_t format = 0xA000 | frame_len;
    if (segment) format |= 0x0800;

    f_ptr[0] = (uint8_t)(format >> 8);
    f_ptr[1] = (uint8_t)(format & 0xFF);

    uint16_t hcs = edge_crc16_ccitt(b->data + 1, b->len - 1);
    net_buf_simple_add_le16(b, hcs);
    return f_ptr;
}

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
    _build_hdlc_header(mgr, HDLC_CTRL_SNRM, 0, false, out);
    net_buf_simple_add_u8(&out->b, 0x7E);
    return 0;
}

int dlms_hdlc_build_disc(dlms_hdlc_manager_t *mgr, net_buf_t *out) {
    _build_hdlc_header(mgr, HDLC_CTRL_DISC | 0x10, 0, false, out);
    net_buf_simple_add_u8(&out->b, 0x7E);
    return 0;
}

int dlms_hdlc_build_rr(dlms_hdlc_manager_t *mgr, net_buf_t *out) {
    uint8_t ctrl = HDLC_CTRL_RR | (uint8_t)(mgr->nr << 5);
    _build_hdlc_header(mgr, ctrl, 0, false, out);
    net_buf_simple_add_u8(&out->b, 0x7E);
    return 0;
}

int dlms_hdlc_send_ui_frame(dlms_hdlc_manager_t *mgr, net_buf_t *apdu, net_buf_t *out) {
    _build_hdlc_header(mgr, HDLC_CTRL_UI | 0x10, apdu->b.len, false, out);
    net_buf_simple_add_mem(&out->b, apdu->b.data, apdu->b.len);
    uint16_t fcs = edge_crc16_ccitt(out->b.data + 1, out->b.len - 1);
    net_buf_simple_add_le16(&out->b, fcs);
    net_buf_simple_add_u8(&out->b, 0x7E);
    return 0;
}

int dlms_hdlc_packetize(dlms_hdlc_manager_t *mgr, net_buf_t *apdu, size_t offset, size_t mtu,
                        net_buf_t *out) {
    size_t remaining = apdu->b.len - offset;
    size_t header_est = 13;
    size_t chunk = (remaining > (mtu - header_est)) ? (mtu - header_est) : remaining;
    bool has_more = (offset + chunk < apdu->b.len);

    uint8_t ctrl = (uint8_t)((mgr->nr << 5) | (mgr->ns << 1));
    if (!has_more) ctrl |= 0x10;  // P/F bit

    _build_hdlc_header(mgr, ctrl, (uint16_t)chunk, has_more, out);
    net_buf_simple_add_mem(&out->b, apdu->b.data + offset, chunk);

    uint16_t fcs = edge_crc16_ccitt(out->b.data + 1, out->b.len - 1);
    net_buf_simple_add_le16(&out->b, fcs);
    net_buf_simple_add_u8(&out->b, 0x7E);

    return (int)chunk;
}

int dlms_hdlc_send_iframe(dlms_hdlc_manager_t *mgr, net_buf_t *apdu, net_buf_t *out) {
    // 简易重发支持：如果 apdu 为空且有备份，重发上一帧
    if (!apdu && mgr->last_tx_frame) {
        LOG_INFO(LOG_CAT_IO, "DLMS HDLC: Retransmitting last frame (N(S)=%d)", mgr->ns);
        net_buf_simple_add_mem(&out->b, mgr->last_tx_frame->b.data, mgr->last_tx_frame->b.len);
        return 0;
    }

    int n = dlms_hdlc_packetize(mgr, apdu, 0, 128, out);

    // 备份当前发送帧用于重发
    if (mgr->last_tx_frame) net_buf_unref(mgr->last_tx_frame);
    mgr->last_tx_frame = net_buf_ref(out);

    return n;
}

int dlms_hdlc_handle_ack(dlms_hdlc_manager_t *mgr, uint8_t control_byte) {
    // 处理 RR 或 I 帧中的确认 N(R)
    uint8_t nr = (uint8_t)((control_byte >> 5) & 0x07);
    if (nr == ((mgr->ns + 1) & 0x07)) {
        mgr->ns = nr;
        if (mgr->last_tx_frame) {
            net_buf_unref(mgr->last_tx_frame);
            mgr->last_tx_frame = NULL;
        }
        return 0;
    }
    // 如果收到要求重发的信号 (通常通过控制域判断，此处简化)
    return -1;
}

int dlms_hdlc_depacketize(dlms_hdlc_manager_t *mgr, struct net_buf_simple *in, net_buf_pool_t *pool,
                          net_buf_t **out_apdu) {
    while (in->len > 0 && in->data[0] == 0x7E) net_buf_simple_pull(in, 1);
    if (in->len < 9) return 1;

    uint8_t *f = in->data;
    uint16_t f_format = (uint16_t)((f[0] << 8) | f[1]);
    uint16_t f_len = f_format & 0x07FF;
    bool is_segment = (f_format & 0x0800) != 0;

    if (in->len < f_len) return 1;

    uint8_t control = f[4];
    int h_l = 5;
    uint16_t hcs_rx = (uint16_t)(f[h_l] | (f[h_l + 1] << 8));
    if (edge_crc16_ccitt(f, (size_t)h_l) != hcs_rx) return -2;

    size_t p_len = (size_t)f_len - (size_t)h_l - 4;
    if (p_len > 0) {
        if (!mgr->reassembly_buf) mgr->reassembly_buf = net_buf_alloc(pool, 0);
        net_buf_simple_add_mem(&mgr->reassembly_buf->b, f + h_l + 2, p_len);
    }

    // 更新 N(R)：收到的 N(S) + 1
    uint8_t ns_rx = (uint8_t)((control >> 1) & 0x07);
    mgr->nr = (uint8_t)((ns_rx + 1) % 8);

    net_buf_simple_pull(in, f_len);

    if (!is_segment) {
        *out_apdu = mgr->reassembly_buf;
        mgr->reassembly_buf = NULL;
        return 0;
    }
    return 1;  // 还有分帧
}
