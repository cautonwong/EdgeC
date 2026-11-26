#include "modbus_codec.h"
#include <float.h>
#include <math.h>
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

static int _write_type_regs(uint8_t type) {
    switch (type) {
        case MODBUS_WRITE_U16:
        case MODBUS_WRITE_I16:
            return 1;
        case MODBUS_WRITE_U32:
        case MODBUS_WRITE_I32:
        case MODBUS_WRITE_F32:
            return 2;
        case MODBUS_WRITE_U64:
        case MODBUS_WRITE_I64:
        case MODBUS_WRITE_F64:
            return 4;
        default:
            return 0;
    }
}

/* 规范形式按大端字序（A B, C D, ...）生成，再按消费者的字序重排。
 * CDAB 是 16 位字序反转，BADC 是字内字节交换，DCBA 两者都做。
 * v3 ORDER_* 名字按寄存器值语义命名，wire 字节与名字交叉（见头文件）。 */
static void _order_write_regs(uint16_t *regs, int n, uint8_t order) {
    bool reverse = order == MODBUS_WORD_ORDER_CDAB || order == MODBUS_WORD_ORDER_DCBA;
    bool swap = order == MODBUS_WORD_ORDER_BADC || order == MODBUS_WORD_ORDER_DCBA;
    if (swap) {
        for (int i = 0; i < n; i++) {
            regs[i] = (uint16_t)((regs[i] >> 8) | (regs[i] << 8));
        }
    }
    if (reverse) {
        for (int i = 0, j = n - 1; i < j; i++, j--) {
            uint16_t tmp = regs[i];
            regs[i] = regs[j];
            regs[j] = tmp;
        }
    }
}

int modbus_encode_write_regs(double value, const modbus_write_desc_t *desc, uint16_t quantity,
                             uint16_t *regs) {
    if (!regs || !isfinite(value)) return -1;

    uint8_t type;
    uint8_t order;
    if (desc) {
        type = desc->type;
        order = desc->order;
        if (_write_type_regs(type) != quantity) return -1;
        if (order > MODBUS_WORD_ORDER_DCBA) return -1;
    } else {
        /* 旧默认：1 个寄存器 u16，2 个寄存器 u32 ABCD；其余数量拒绝 */
        order = MODBUS_WORD_ORDER_ABCD;
        if (quantity == 1) {
            type = MODBUS_WRITE_U16;
        } else if (quantity == 2) {
            type = MODBUS_WRITE_U32;
        } else {
            return -1;
        }
    }

    /* 整数先按 double 区间检查再向零截断；越界值直接拒绝，不回绕也不静默截断。 */
    uint64_t bits = 0;
    switch (type) {
        case MODBUS_WRITE_U16:
            if (value < 0.0 || value > 65535.0) return -1;
            bits = (uint16_t)value;
            break;
        case MODBUS_WRITE_I16:
            if (value < -32768.0 || value > 32767.0) return -1;
            bits = (uint16_t)(int16_t)value;
            break;
        case MODBUS_WRITE_U32:
            if (value < 0.0 || value > 4294967295.0) return -1;
            bits = (uint32_t)value;
            break;
        case MODBUS_WRITE_I32:
            if (value < -2147483648.0 || value > 2147483647.0) return -1;
            bits = (uint32_t)(int32_t)value;
            break;
        case MODBUS_WRITE_F32: {
            /* double→float 超范围是 UB；先挡住，cast 溢出成 inf 的帧不发给从站 */
            if (value > FLT_MAX || value < -FLT_MAX) return -1;
            float f = (float)value;
            uint32_t u = 0;
            memcpy(&u, &f, sizeof(u));
            bits = u;
            break;
        }
        case MODBUS_WRITE_U64:
            if (value < 0.0 || value >= 18446744073709551616.0) return -1; /* 2^64 */
            bits = (uint64_t)value;
            break;
        case MODBUS_WRITE_I64:
            if (value < -9223372036854775808.0 || value >= 9223372036854775808.0) return -1;
            bits = (uint64_t)(int64_t)value;
            break;
        case MODBUS_WRITE_F64: {
            uint64_t u = 0;
            memcpy(&u, &value, sizeof(u));
            bits = u;
            break;
        }
        default:
            return -1;
    }

    int n = _write_type_regs(type);
    for (int i = 0; i < n; i++) {
        regs[i] = (uint16_t)(bits >> (16 * (n - 1 - i)));
    }
    _order_write_regs(regs, n, order);
    return 0;
}

/* 标准 Modbus 低字节先发；hi_crc 是从 iems_v3 默认 modbus_rtu 继承的高字节先发 */
static void _put_crc(uint8_t *p, size_t body_len, bool hi_crc) {
    uint16_t crc = modbus_calculate_crc(p, body_len);
    p[body_len] = (uint8_t)(hi_crc ? (crc >> 8) : (crc & 0xFF));
    p[body_len + 1] = (uint8_t)(hi_crc ? (crc & 0xFF) : (crc >> 8));
}

static uint16_t _get_crc(const uint8_t *p, size_t frame_len, bool hi_crc) {
    return hi_crc ? (uint16_t)((p[frame_len - 2] << 8) | p[frame_len - 1])
                  : (uint16_t)(p[frame_len - 2] | (p[frame_len - 1] << 8));
}

static int _modbus_rtu_encode_generic(uint8_t slave_id, uint8_t fc, uint16_t p1, uint16_t p2,
                                      bool hi_crc, net_buf_t *out) {
    if (!out || net_buf_tailroom(out) < 8) return -1;
    uint8_t *p = net_buf_add(out, 8);
    p[0] = slave_id;
    p[1] = fc;
    p[2] = (uint8_t)(p1 >> 8);
    p[3] = (uint8_t)(p1 & 0xFF);
    p[4] = (uint8_t)(p2 >> 8);
    p[5] = (uint8_t)(p2 & 0xFF);
    _put_crc(p, 6, hi_crc);
    return 0;
}

int modbus_rtu_encode_read_ex(uint8_t slave_id, uint8_t fc, uint16_t addr, uint16_t quantity,
                              bool hi_crc, net_buf_t *out) {
    return _modbus_rtu_encode_generic(slave_id, fc, addr, quantity, hi_crc, out);
}

int modbus_rtu_encode_write_single_ex(uint8_t slave_id, uint16_t addr, uint16_t value, bool hi_crc,
                                      net_buf_t *out) {
    return _modbus_rtu_encode_generic(slave_id, MODBUS_FC_WRITE_SINGLE_REG, addr, value, hi_crc,
                                      out);
}

int modbus_rtu_encode_read(uint8_t slave_id, uint8_t fc, uint16_t addr, uint16_t quantity, net_buf_t *out) {
    return modbus_rtu_encode_read_ex(slave_id, fc, addr, quantity, false, out);
}

int modbus_rtu_encode_write_single(uint8_t slave_id, uint16_t addr, uint16_t value, net_buf_t *out) {
    return modbus_rtu_encode_write_single_ex(slave_id, addr, value, false, out);
}

int modbus_rtu_encode_write_multiple_ex(uint8_t slave_id, uint16_t addr, uint16_t quantity,
                                        const uint16_t *values, bool hi_crc, net_buf_t *out) {
    if (!out || !values || quantity == 0 || quantity > 123) return -1;
    size_t body_len = 7 + 2 * (size_t)quantity; /* slave/fc/addr/qty/byte_count + values */
    if (net_buf_tailroom(out) < body_len + 2) return -1;
    uint8_t *p = net_buf_add(out, body_len + 2);
    p[0] = slave_id;
    p[1] = MODBUS_FC_WRITE_MULTIPLE_REGS;
    p[2] = (uint8_t)(addr >> 8);
    p[3] = (uint8_t)(addr & 0xFF);
    p[4] = (uint8_t)(quantity >> 8);
    p[5] = (uint8_t)(quantity & 0xFF);
    p[6] = (uint8_t)(quantity * 2);
    for (uint16_t i = 0; i < quantity; i++) {
        p[7 + 2 * i] = (uint8_t)(values[i] >> 8);
        p[8 + 2 * i] = (uint8_t)(values[i] & 0xFF);
    }
    _put_crc(p, body_len, hi_crc);
    return 0;
}

int modbus_rtu_encode_write_multiple(uint8_t slave_id, uint16_t addr, uint16_t quantity,
                                     const uint16_t *values, net_buf_t *out) {
    return modbus_rtu_encode_write_multiple_ex(slave_id, addr, quantity, values, false, out);
}

int modbus_rtu_encode_exception_ex(uint8_t slave_id, uint8_t fc, uint8_t ex_code, bool hi_crc,
                                   net_buf_t *out) {
    if (!out || net_buf_tailroom(out) < 5) return -1;
    uint8_t *p = net_buf_add(out, 5);
    p[0] = slave_id;
    p[1] = (uint8_t)(fc | 0x80);
    p[2] = ex_code;
    _put_crc(p, 3, hi_crc);
    return 0;
}

int modbus_rtu_encode_exception(uint8_t slave_id, uint8_t fc, uint8_t ex_code, net_buf_t *out) {
    return modbus_rtu_encode_exception_ex(slave_id, fc, ex_code, false, out);
}

int modbus_rtu_decode_frame_ex(struct net_buf_simple *in, bool hi_crc, uint8_t *slave_id,
                               uint8_t *fc, uint8_t **data_out, size_t *data_len) {
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
                    uint16_t crc_rx = _get_crc(p, resp_len, hi_crc);
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
                    uint16_t crc_rx = _get_crc(p, 8, hi_crc);
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
    uint16_t crc_rx = _get_crc(p, frame_len, hi_crc);
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

int modbus_rtu_decode_frame(struct net_buf_simple *in, uint8_t *slave_id, uint8_t *fc,
                            uint8_t **data_out, size_t *data_len) {
    return modbus_rtu_decode_frame_ex(in, false, slave_id, fc, data_out, data_len);
}

int modbus_rtu_decode_req_frame_ex(struct net_buf_simple *in, bool hi_crc, uint8_t *slave_id,
                                   uint8_t *fc, uint8_t **data_out, size_t *data_len) {
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
    uint16_t crc_rx = _get_crc(p, frame_len, hi_crc);
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

int modbus_rtu_decode_req_frame(struct net_buf_simple *in, uint8_t *slave_id, uint8_t *fc,
                                uint8_t **data_out, size_t *data_len) {
    return modbus_rtu_decode_req_frame_ex(in, false, slave_id, fc, data_out, data_len);
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
    /* length 从 unit id 起算，上限 = 1 + PDU 253。超长声明若放行会一直
     * INCOMPLETE，只能等事务超时重同步 */
    if (total_len > 254) return -1;
    /* MBAP length 字段从 unit id 起算，不含前 6 字节；帧长 = 6 + total_len */
    if (in->len < (size_t)(MODBUS_TCP_HEADER_LEN - 1 + total_len)) return 1;

    *slave_id = p[6];
    *pdu_len = (size_t)(total_len - 1);
    return 0;
}

int modbus_tcp_encode_read(uint16_t tid, uint8_t slave_id, uint8_t fc, uint16_t addr, uint16_t quantity, net_buf_t *out) {
    if (!out) return -1;
    const size_t pdu_len = 5;
    size_t frame_len = MODBUS_TCP_HEADER_LEN + pdu_len;
    if (net_buf_tailroom(out) < frame_len) return -1;

    /* MBAP 与 PDU 一次写入 out：临时 buffer 的分配失败会让 net_buf_add 解引用 NULL */
    uint8_t *p = net_buf_add(out, frame_len);
    p[0] = (uint8_t)(tid >> 8);
    p[1] = (uint8_t)(tid & 0xFF);
    p[2] = 0;
    p[3] = 0; /* Protocol ID = 0 (Modbus) */
    p[4] = 0;
    p[5] = (uint8_t)(pdu_len + 1); /* length 从 unit id 起算 */
    p[6] = slave_id;
    p[7] = fc;
    p[8] = (uint8_t)(addr >> 8);
    p[9] = (uint8_t)(addr & 0xFF);
    p[10] = (uint8_t)(quantity >> 8);
    p[11] = (uint8_t)(quantity & 0xFF);
    return 0;
}

int modbus_tcp_encode_write_single(uint16_t tid, uint8_t slave_id, uint16_t addr, uint16_t value, net_buf_t *out) {
    if (!out) return -1;
    const size_t pdu_len = 5;
    size_t frame_len = MODBUS_TCP_HEADER_LEN + pdu_len;
    if (net_buf_tailroom(out) < frame_len) return -1;

    uint8_t *p = net_buf_add(out, frame_len);
    p[0] = (uint8_t)(tid >> 8);
    p[1] = (uint8_t)(tid & 0xFF);
    p[2] = 0;
    p[3] = 0; /* Protocol ID = 0 (Modbus) */
    p[4] = 0;
    p[5] = (uint8_t)(pdu_len + 1); /* length 从 unit id 起算 */
    p[6] = slave_id;
    p[7] = MODBUS_FC_WRITE_SINGLE_REG;
    p[8] = (uint8_t)(addr >> 8);
    p[9] = (uint8_t)(addr & 0xFF);
    p[10] = (uint8_t)(value >> 8);
    p[11] = (uint8_t)(value & 0xFF);
    return 0;
}

int modbus_tcp_encode_write_multiple(uint16_t tid, uint8_t slave_id, uint16_t addr,
                                     uint16_t quantity, const uint16_t *values, net_buf_t *out) {
    if (!out || !values || quantity == 0 || quantity > 123) return -1;
    size_t pdu_len = 6 + 2 * (size_t)quantity;
    size_t frame_len = MODBUS_TCP_HEADER_LEN + pdu_len;
    if (net_buf_tailroom(out) < frame_len) return -1;

    /* MBAP 与 PDU 一次写入 out：临时 buffer 的分配失败会让 net_buf_add 解引用 NULL */
    uint8_t *p = net_buf_add(out, frame_len);
    p[0] = (uint8_t)(tid >> 8);
    p[1] = (uint8_t)(tid & 0xFF);
    p[2] = 0;
    p[3] = 0; /* Protocol ID = 0 (Modbus) */
    p[4] = (uint8_t)((pdu_len + 1) >> 8);
    p[5] = (uint8_t)((pdu_len + 1) & 0xFF);
    p[6] = slave_id;
    p[7] = MODBUS_FC_WRITE_MULTIPLE_REGS;
    p[8] = (uint8_t)(addr >> 8);
    p[9] = (uint8_t)(addr & 0xFF);
    p[10] = (uint8_t)(quantity >> 8);
    p[11] = (uint8_t)(quantity & 0xFF);
    p[12] = (uint8_t)(quantity * 2);
    for (uint16_t i = 0; i < quantity; i++) {
        p[13 + 2 * i] = (uint8_t)(values[i] >> 8);
        p[14 + 2 * i] = (uint8_t)(values[i] & 0xFF);
    }
    return 0;
}

int modbus_tcp_encode_exception(uint16_t tid, uint8_t slave_id, uint8_t fc, uint8_t ex_code, net_buf_t *out) {
    if (!out) return -1;
    size_t frame_len = MODBUS_TCP_HEADER_LEN + 2;
    if (net_buf_tailroom(out) < frame_len) return -1;

    uint8_t *p = net_buf_add(out, frame_len);
    p[0] = (uint8_t)(tid >> 8);
    p[1] = (uint8_t)(tid & 0xFF);
    p[2] = 0;
    p[3] = 0; /* Protocol ID = 0 (Modbus) */
    p[4] = 0;
    p[5] = 3; /* pdu_len(2) + unit id(1) */
    p[6] = slave_id;
    p[7] = (uint8_t)(fc | 0x80);
    p[8] = ex_code;
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
    /* FC06 应答就是请求回显，与写单寄存器帧逐字节相同 */
    return modbus_tcp_encode_write_single(tid, slave_id, addr, value, out);
}
