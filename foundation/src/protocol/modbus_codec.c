#include "modbus_codec.h"
#include <string.h>

static const uint16_t s_modbus_crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
};

uint16_t modbus_calculate_crc(const uint8_t *buf, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < len; pos++) {
        crc = (uint16_t)((crc >> 8) ^ s_modbus_crc16_table[(crc ^ buf[pos]) & 0xFF]);
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

    fprintf(stderr, "🔍 [MODBUS_DECODE] Len: %u, Dump: ", (unsigned int)in->len);
    for(size_t i=0; i<in->len && i<16; i++) {
        fprintf(stderr, "%02X ", p[i]);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
    if (func & 0x80) {
        frame_len = 5; // Exception frame
    } else {
        switch (func) {
            case MODBUS_FC_READ_COILS:
            case MODBUS_FC_READ_DISCRETE_INPUTS:
            case MODBUS_FC_READ_HOLDING_REGS:
            case MODBUS_FC_READ_INPUT_REGS:
                /* 
                 * 仅接受从站响应帧格式：[ID][FC][ByteCount][Data...][CRC]
                 */
                if (in->len >= 3) {
                    size_t resp_len = (size_t)(p[2] + 5);
                    if (in->len < resp_len) {
                        return 1; // Incomplete
                    }
                    uint16_t crc_calc = modbus_calculate_crc(p, resp_len - 2);
                    uint16_t crc_rx = (uint16_t)(p[resp_len - 2] | (p[resp_len - 1] << 8));
                    if (crc_calc == crc_rx) {
                        frame_len = resp_len;
                    } else {
                        // CRC mismatch indicates garbage header, trigger sliding window
                        fprintf(stderr, "❌ [MODBUS_CRC] Resp Len: %zu, Calc: %04X, RX: %04X, trigger sliding\n", resp_len, crc_calc, crc_rx);
                        fflush(stderr);
                        return -1;
                    }
                } else {
                    return 1; // Incomplete
                }
                break;

            case MODBUS_FC_WRITE_SINGLE_COIL:
            case MODBUS_FC_WRITE_SINGLE_REG:
            case MODBUS_FC_WRITE_MULTIPLE_COILS:
            case MODBUS_FC_WRITE_MULTIPLE_REGS:
                if (in->len >= 8) {
                    uint16_t crc_calc = modbus_calculate_crc(p, 6);
                    uint16_t crc_rx = (uint16_t)(p[6] | (p[7] << 8));
                    if (crc_calc == crc_rx) {
                        frame_len = 8;
                    } else {
                        return -1; // Garbage
                    }
                } else {
                    return 1; // Incomplete
                }
                break;
                
            default:
                return -2; // Unknown function code, garbage
        }
    }

    if (frame_len == 0 || in->len < frame_len) return 1;

    uint16_t crc_calc = modbus_calculate_crc(p, frame_len - 2);
    uint16_t crc_rx = (uint16_t)(p[frame_len - 2] | (p[frame_len - 1] << 8));
    if (crc_calc != crc_rx) {
        fprintf(stderr, "❌ [MODBUS_GLOBAL_CRC] Len: %zu, Calc: %04X, RX: %04X\n", frame_len, crc_calc, crc_rx);
        return -1;
    }

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

int modbus_rtu_decode_req_frame(struct net_buf_simple *in, uint8_t *slave_id, uint8_t *fc, 
                                uint8_t **data_out, size_t *data_len) {
    if (in->len < 4) return 1;

    uint8_t *p = in->data;
    uint8_t func = p[1];
    size_t frame_len = 0;

    switch (func) {
        case MODBUS_FC_READ_HOLDING_REGS:
        case MODBUS_FC_READ_INPUT_REGS:
        case MODBUS_FC_WRITE_SINGLE_REG:
            frame_len = 8;
            break;
        case MODBUS_FC_WRITE_MULTIPLE_REGS:
            if (in->len >= 7) {
                uint8_t byte_count = p[6];
                frame_len = 7 + (size_t)byte_count + 2;
            } else {
                return 1;
            }
            break;
        default:
            return -2;
    }

    if (in->len < frame_len) return 1;

    uint16_t crc_calc = modbus_calculate_crc(p, frame_len - 2);
    uint16_t crc_rx = (uint16_t)(p[frame_len - 2] | (p[frame_len - 1] << 8));
    if (crc_calc != crc_rx) {
        fprintf(stderr, "❌ [MODBUS_REQ_CRC] Len: %zu, Calc: %04X, RX: %04X\n", frame_len, crc_calc, crc_rx);
        return -1;
    }

    *slave_id = p[0];
    *fc = p[1];
    *data_out = p + 2;
    *data_len = frame_len - 4;
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
