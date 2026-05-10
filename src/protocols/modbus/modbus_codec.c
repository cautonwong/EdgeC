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
    p[1] = fc | 0x80;
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
                if (in->len < 3) return 1;
                frame_len = 5 + p[2]; // 1(ID) + 1(FC) + 1(LEN) + N(DATA) + 2(CRC)
                break;
            case MODBUS_FC_WRITE_SINGLE_COIL:
            case MODBUS_FC_WRITE_SINGLE_REG:
            case MODBUS_FC_WRITE_MULTIPLE_REGS:
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
    } else if (func <= 0x04) {
        *data_out = &p[3];
        *data_len = p[2];
    } else {
        *data_out = &p[2];
        *data_len = 4;
    }

    return 0;
}
