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

int modbus_encode_read_holding(uint8_t slave_id, uint16_t addr, uint16_t quantity, net_buf_t *out) {
    struct net_buf_simple *b = &out->b;
    uint8_t *p = net_buf_simple_add(b, 8);

    p[0] = slave_id;
    p[1] = 0x03;
    p[2] = (uint8_t)((addr >> 8) & 0xFF);
    p[3] = (uint8_t)(addr & 0xFF);
    p[4] = (uint8_t)((quantity >> 8) & 0xFF);
    p[5] = (uint8_t)(quantity & 0xFF);

    uint16_t crc = modbus_calculate_crc(p, 6);
    p[6] = (uint8_t)(crc & 0xFF);
    p[7] = (uint8_t)(crc >> 8);

    return 0;
}

int modbus_encode_write_single_resp(uint8_t slave_id, uint16_t addr, uint16_t value,
                                    net_buf_t *out) {
    struct net_buf_simple *b = &out->b;
    uint8_t *p = net_buf_simple_add(b, 8);

    p[0] = slave_id;
    p[1] = 0x06;
    p[2] = (uint8_t)((addr >> 8) & 0xFF);
    p[3] = (uint8_t)(addr & 0xFF);
    p[4] = (uint8_t)((value >> 8) & 0xFF);
    p[5] = (uint8_t)(value & 0xFF);

    uint16_t crc = modbus_calculate_crc(p, 6);
    p[6] = (uint8_t)(crc & 0xFF);
    p[7] = (uint8_t)(crc >> 8);

    return 0;
}

int modbus_decode_rtu_header(struct net_buf_simple *in, uint8_t *slave_id, uint8_t *func_code) {
    if (in->len < 4) return 1;

    // 这里仅做基础长度检查，具体校验由调用者决定或在后续完善
    *slave_id = in->data[0];
    *func_code = in->data[1];

    return 0;
}
