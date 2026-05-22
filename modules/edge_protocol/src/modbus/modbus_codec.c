#include "modbus_codec.h"
#include <string.h>

uint16_t modbus_calculate_crc(const uint8_t *buf, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static int _modbus_rtu_encode_generic(uint8_t slave_id, uint8_t fc, uint16_t p1, uint16_t p2, net_buf_t *out) {
    uint8_t *p = net_buf_add(out, 8);
    p[0] = slave_id;
    p[1] = fc;
    p[2] = (uint8_t)(p1 >> 8);
    p[3] = (uint8_t)(p1 & 0xFF);
    p[4] = (uint8_t)(p2 >> 8);
    p[5] = (uint8_t)(p2 & 0xFF);

    uint16_t crc = modbus_calculate_crc(p, 6);
    p[6] = (uint8_t)(crc & 0xFF);
    p[7] = (uint8_t)(crc >> 8);
    return 0;
}

int modbus_rtu_encode_read(uint8_t slave_id, uint8_t fc, uint16_t addr, uint16_t quantity, net_buf_t *out) {
    return _modbus_rtu_encode_generic(slave_id, fc, addr, quantity, out);
}

int modbus_rtu_encode_write_single(uint8_t slave_id, uint16_t addr, uint16_t value, net_buf_t *out) {
    return _modbus_rtu_encode_generic(slave_id, MODBUS_FC_WRITE_SINGLE_REG, addr, value, out);
}

int modbus_rtu_encode_exception(uint8_t slave_id, uint8_t fc, uint8_t ex_code, net_buf_t *out) {
    uint8_t *p = net_buf_add(out, 5);
    p[0] = slave_id;
    p[1] = (uint8_t)(fc | 0x80);
    p[2] = ex_code;
    uint16_t crc = modbus_calculate_crc(p, 3);
    p[3] = (uint8_t)(crc & 0xFF);
    p[4] = (uint8_t)(crc >> 8);
    return 0;
}

int modbus_rtu_decode_frame(struct net_buf_simple *in, uint8_t *slave_id, uint8_t *fc,
                            uint8_t **data_out, size_t *data_len) {
    if (in->len < 4) return 1; // Incomplete

    uint8_t *p = in->data;
    uint8_t func = p[1];
    size_t frame_len = 0;

    if (func & 0x80) {
        frame_len = 5; // Exception frame
    } else {
        switch (func) {
            case MODBUS_FC_READ_COILS:
            case MODBUS_FC_READ_DISCRETE_INPUTS:
            case MODBUS_FC_READ_HOLDING_REGS:
            case MODBUS_FC_READ_INPUT_REGS:
                /* 
                 * 【双端兼容自适应解码】
                 * 1. 优先尝试 8 字节固定长度（符合标准请求帧格式）。若在 8 字节下 CRC 校验成功，则帧长定为 8。
                 * 2. 否则，如果缓冲区长度足够响应帧，且在响应帧长度 (p[2] + 5) 下 CRC 校验成功，则帧长定为 p[2] + 5。
                 * 3. 否则，默认设定帧长为 8，交由后续通用 CRC 校验逻辑处理。
                 */
                if (in->len >= 8) {
                    uint16_t crc_calc = modbus_calculate_crc(p, 6);
                    uint16_t crc_rx = (uint16_t)(p[6] | (p[7] << 8));
                    if (crc_calc == crc_rx) {
                        frame_len = 8;
                        break;
                    }
                }
                if (in->len >= 3) {
                    size_t resp_len = (size_t)(p[2] + 5);
                    if (in->len >= resp_len) {
                        uint16_t crc_calc = modbus_calculate_crc(p, resp_len - 2);
                        uint16_t crc_rx = (uint16_t)(p[resp_len - 2] | (p[resp_len - 1] << 8));
                        if (crc_calc == crc_rx) {
                            frame_len = resp_len;
                            break;
                        }
                    }
                }
                frame_len = 8;
                break;
            case MODBUS_FC_WRITE_SINGLE_COIL:
            case MODBUS_FC_WRITE_SINGLE_REG:
                frame_len = 8;
                break;
            case MODBUS_FC_WRITE_MULTIPLE_REGS:
                /* 
                 * 【双端兼容自适应解码】
                 * 1. 优先尝试 8 字节固定长度（符合写多个寄存器的响应帧格式）。若在 8 字节下 CRC 校验成功，则帧长定为 8。
                 * 2. 否则，如果缓冲区长度足够请求帧，且在请求帧长度 (p[6] + 9) 下 CRC 校验成功，则帧长定为 p[6] + 9。
                 * 3. 否则，默认设定帧长为 8，交由后续通用 CRC 校验逻辑处理。
                 */
                if (in->len >= 8) {
                    uint16_t crc_calc = modbus_calculate_crc(p, 6);
                    uint16_t crc_rx = (uint16_t)(p[6] | (p[7] << 8));
                    if (crc_calc == crc_rx) {
                        frame_len = 8;
                        break;
                    }
                }
                if (in->len >= 7) {
                    size_t req_len = (size_t)(p[6] + 9);
                    if (in->len >= req_len) {
                        uint16_t crc_calc = modbus_calculate_crc(p, req_len - 2);
                        uint16_t crc_rx = (uint16_t)(p[req_len - 2] | (p[req_len - 1] << 8));
                        if (crc_calc == crc_rx) {
                            frame_len = req_len;
                            break;
                        }
                    }
                }
                frame_len = 8;
                break;
            default:
                return -2; // Unknown function code
        }
    }

    if (in->len < frame_len) return 1;

    uint16_t crc_calc = modbus_calculate_crc(p, frame_len - 2);
    uint16_t crc_rx = (uint16_t)(p[frame_len - 2] | (p[frame_len - 1] << 8));
    if (crc_calc != crc_rx) return -1;

    *slave_id = p[0];
    *fc = func;

    if (func & 0x80) {
        *data_out = &p[2];
        *data_len = 1;
    } else {
        *data_out = &p[2];
        *data_len = frame_len - 4; // Skip ID, FC and CRC
    }

    return 0;
}

/* ================================================================
 * Modbus TCP / MBAP Codec
 * ================================================================ */

int modbus_tcp_encode_mbap(uint16_t tid, uint8_t slave_id, uint16_t pdu_len, net_buf_t *out) {
    uint8_t *p = net_buf_add(out, MODBUS_TCP_HEADER_LEN);
    p[0] = (uint8_t)(tid >> 8);
    p[1] = (uint8_t)(tid & 0xFF);
    p[2] = 0;
    p[3] = 0; /* Protocol ID = 0 (Modbus) */
    p[4] = (uint8_t)((pdu_len + 1) >> 8);
    p[5] = (uint8_t)((pdu_len + 1) & 0xFF);
    p[6] = slave_id;
    return 0;
}

int modbus_tcp_decode_mbap(struct net_buf_simple *in, uint16_t *tid, uint8_t *slave_id, size_t *pdu_len) {
    if (in->len < MODBUS_TCP_HEADER_LEN) return 1;

    uint8_t *p = in->data;
    *tid = (uint16_t)((p[0] << 8) | p[1]);
    uint16_t proto = (uint16_t)((p[2] << 8) | p[3]);
    if (proto != MODBUS_TCP_PROTOCOL_ID) return -1;

    uint16_t total_len = (uint16_t)((p[4] << 8) | p[5]);
    if (total_len < 1) return -1;
    if (in->len < (size_t)(MODBUS_TCP_HEADER_LEN + total_len)) return 1;

    *slave_id = p[6];
    *pdu_len = (size_t)(total_len - 1);
    return 0;
}

int modbus_tcp_encode_read(uint16_t tid, uint8_t slave_id, uint8_t fc, uint16_t addr, uint16_t quantity, net_buf_t *out) {
    net_buf_t *pdu = net_buf_alloc(out->pool, 0);
    uint8_t *p = net_buf_add(pdu, 5);
    p[0] = fc;
    p[1] = (uint8_t)(addr >> 8);
    p[2] = (uint8_t)(addr & 0xFF);
    p[3] = (uint8_t)(quantity >> 8);
    p[4] = (uint8_t)(quantity & 0xFF);

    modbus_tcp_encode_mbap(tid, slave_id, 5, out);
    net_buf_simple_add_mem(&out->b, pdu->b.data, pdu->b.len);
    net_buf_unref(pdu);
    return 0;
}

int modbus_tcp_encode_write_single(uint16_t tid, uint8_t slave_id, uint16_t addr, uint16_t value, net_buf_t *out) {
    net_buf_t *pdu = net_buf_alloc(out->pool, 0);
    uint8_t *p = net_buf_add(pdu, 5);
    p[0] = MODBUS_FC_WRITE_SINGLE_REG;
    p[1] = (uint8_t)(addr >> 8);
    p[2] = (uint8_t)(addr & 0xFF);
    p[3] = (uint8_t)(value >> 8);
    p[4] = (uint8_t)(value & 0xFF);

    modbus_tcp_encode_mbap(tid, slave_id, 5, out);
    net_buf_simple_add_mem(&out->b, pdu->b.data, pdu->b.len);
    net_buf_unref(pdu);
    return 0;
}

int modbus_tcp_encode_exception(uint16_t tid, uint8_t slave_id, uint8_t fc, uint8_t ex_code, net_buf_t *out) {
    net_buf_t *pdu = net_buf_alloc(out->pool, 0);
    uint8_t *p = net_buf_add(pdu, 2);
    p[0] = (uint8_t)(fc | 0x80);
    p[1] = ex_code;

    modbus_tcp_encode_mbap(tid, slave_id, 2, out);
    net_buf_simple_add_mem(&out->b, pdu->b.data, pdu->b.len);
    net_buf_unref(pdu);
    return 0;
}

int modbus_tcp_encode_read_resp(uint16_t tid, uint8_t slave_id, const uint8_t *data, size_t data_len, net_buf_t *out) {
    size_t pdu_len = 2 + data_len;
    modbus_tcp_encode_mbap(tid, slave_id, (uint16_t)pdu_len, out);

    uint8_t *p = net_buf_add(out, 2 + data_len);
    p[0] = MODBUS_FC_READ_HOLDING_REGS;
    p[1] = (uint8_t)data_len;
    if (data_len > 0) memcpy(p + 2, data, data_len);
    return 0;
}

int modbus_tcp_encode_write_resp(uint16_t tid, uint8_t slave_id, uint16_t addr, uint16_t value, net_buf_t *out) {
    net_buf_t *pdu = net_buf_alloc(out->pool, 0);
    uint8_t *p = net_buf_add(pdu, 5);
    p[0] = MODBUS_FC_WRITE_SINGLE_REG;
    p[1] = (uint8_t)(addr >> 8);
    p[2] = (uint8_t)(addr & 0xFF);
    p[3] = (uint8_t)(value >> 8);
    p[4] = (uint8_t)(value & 0xFF);

    modbus_tcp_encode_mbap(tid, slave_id, 5, out);
    net_buf_simple_add_mem(&out->b, pdu->b.data, pdu->b.len);
    net_buf_unref(pdu);
    return 0;
}
