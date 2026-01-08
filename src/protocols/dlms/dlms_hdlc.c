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
        LOG_ERROR(LOG_CAT_IO, "DLMS HDLC: Out buf too small (Need %ld)", chunk + 13);
        return -1;
    }

    net_buf_simple_add_u8(b, 0x7E);
    uint8_t *f_ptr = net_buf_simple_add(b, 2);
    net_buf_simple_add_u8(b, (uint8_t)((mgr->server_addr << 1) | 1));
    net_buf_simple_add_u8(b, (uint8_t)((mgr->client_addr << 1) | 1));
    uint8_t control = (uint8_t)((mgr->nr << 5) | (mgr->ns << 1));
    if (offset + chunk >= apdu->b.len) control |= 0x10;
    net_buf_simple_add_u8(b, control);

    uint16_t frame_len = (uint16_t)(b->len + chunk + 2 - 1);
    f_ptr[0] = (uint8_t)(0xA0 | (frame_len >> 8));
    f_ptr[1] = (uint8_t)(frame_len & 0xFF);
    uint16_t hcs = edge_crc16_ccitt(b->data + 1, 5);
    net_buf_simple_add_le16(b, hcs);

    net_buf_simple_add_mem(b, apdu->b.data + offset, chunk);
    uint16_t fcs = edge_crc16_ccitt(b->data + 1, b->len - 1);
    net_buf_simple_add_le16(b, fcs);
    net_buf_simple_add_u8(b, 0x7E);

    if (offset + chunk >= apdu->b.len) mgr->ns = (uint8_t)((mgr->ns + 1) % 8);
    return (int)chunk;
}

int dlms_hdlc_depacketize(dlms_hdlc_manager_t *mgr, struct net_buf_simple *in, net_buf_pool_t *pool,
                          net_buf_t **out_apdu) {
    while (in->len > 0 && in->data[0] == 0x7E) net_buf_simple_pull(in, 1);
    if (in->len < 9) return 1;

    uint8_t *f = in->data;
    uint32_t d_addr, s_addr;
    int d_l = _parse_hdlc_addr(f + 2, &d_addr);
    int s_l = _parse_hdlc_addr(f + 2 + d_l, &s_addr);
    if (d_l < 0 || s_l < 0) return -1;

    int h_l = 2 + d_l + s_l + 1;
    if (in->len < (size_t)h_l + 2) return 1;

    uint16_t hcs_rx = (uint16_t)(f[h_l] | (f[h_l + 1] << 8));
    if (edge_crc16_ccitt(f, (size_t)h_l) != hcs_rx) return -2;

    uint8_t control = f[h_l - 1];
    mgr->nr = (uint8_t)(((control >> 1) + 1) % 8);

    if ((control & 0xEF) == 0x63 || (control & 0xEF) == 0x83) {
        net_buf_simple_pull(in, (size_t)h_l + 4);
        *out_apdu = NULL;
        return 0;
    }

    uint16_t f_len = (uint16_t)(((f[0] & 0x07) << 8) | f[1]);
    if (f_len < (uint16_t)h_l + 4 || in->len < f_len) return 1;

    size_t p_len = (size_t)f_len - (size_t)h_l - 4;
    if (p_len > 0) {
        if (!mgr->reassembly_buf) {
            mgr->reassembly_buf = net_buf_alloc(pool, 0);
            if (!mgr->reassembly_buf) return -3;
        }
        if (net_buf_simple_tailroom(&mgr->reassembly_buf->b) < p_len) {
            LOG_ERROR(LOG_CAT_IO, "DLMS HDLC: Reassembly buffer overflow!");
            return -5;
        }
        net_buf_simple_add_mem(&mgr->reassembly_buf->b, f + h_l + 2, p_len);
    }

    net_buf_simple_pull(in, f_len);
    if (control & 0x10) {
        *out_apdu = mgr->reassembly_buf;
        mgr->reassembly_buf = NULL;
        return 0;
    }
    return 1;
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