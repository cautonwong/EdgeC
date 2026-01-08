#include "dlt645_codec.h"

#include <string.h>

uint8_t dlt645_calculate_cs(const uint8_t *buf, size_t len) {
    uint8_t cs = 0;
    for (size_t i = 0; i < len; i++) cs += buf[i];
    return cs;
}

double dlt645_bcd_to_f64(const uint8_t *bcd, int bytes) {
    double val = 0;
    double factor = 1.0;
    for (int i = 0; i < bytes; i++) {
        val += (double)(bcd[i] & 0x0F) * factor;
        factor *= 10.0;
        val += (double)((bcd[i] >> 4) & 0x0F) * factor;
        factor *= 10.0;
    }
    return val;
}

void dlt645_u64_to_bcd_addr(uint64_t addr, uint8_t *out_bcd) {
    for (int i = 0; i < 6; i++) {
        out_bcd[i] = (uint8_t)(((addr / 10 % 10) << 4) | (addr % 10));
        addr /= 100;
    }
}

int dlt645_encode_read_request(const uint8_t *addr_bcd, uint32_t di, net_buf_t *out) {
    struct net_buf_simple *b = &out->b;

    net_buf_simple_add_u8(b, 0x68);
    net_buf_simple_add_mem(b, addr_bcd, 6);
    net_buf_simple_add_u8(b, 0x68);
    net_buf_simple_add_u8(b, DLT645_CTRL_READ_DATA);
    net_buf_simple_add_u8(b, 0x04);  // DI 长度

    // DI 编码 (+0x33)
    net_buf_simple_add_u8(b, (uint8_t)((di & 0xFF) + 0x33));
    net_buf_simple_add_u8(b, (uint8_t)(((di >> 8) & 0xFF) + 0x33));
    net_buf_simple_add_u8(b, (uint8_t)(((di >> 16) & 0xFF) + 0x33));
    net_buf_simple_add_u8(b, (uint8_t)(((di >> 24) & 0xFF) + 0x33));

    net_buf_simple_add_u8(b, dlt645_calculate_cs(b->data, b->len));
    net_buf_simple_add_u8(b, 0x16);
    return 0;
}

int dlt645_decode_response(struct net_buf_simple *in, uint32_t *out_di, double *out_val) {
    // 跳过前导 FE
    while (in->len > 0 && in->data[0] == 0xFE) net_buf_simple_pull(in, 1);

    if (in->len < 12) return 1;
    if (in->data[0] != 0x68) return -1;

    // 检查 CS (倒数第二字节)
    uint16_t frame_len = (uint16_t)(10 + in->data[9] + 2);
    if (in->len < frame_len) return 1;

    if (dlt645_calculate_cs(in->data, frame_len - 2) != in->data[frame_len - 2]) return -2;

    // 解析 DI (-0x33)
    uint32_t di = 0;
    di |= (uint32_t)(in->data[10] - 0x33);
    di |= (uint32_t)(in->data[11] - 0x33) << 8;
    di |= (uint32_t)(in->data[12] - 0x33) << 16;
    di |= (uint32_t)(in->data[13] - 0x33) << 24;
    *out_di = di;

    // 数据域处理 (-0x33)
    int data_len = in->data[9] - 4;
    if (data_len > 0) {
        uint8_t val_buf[16];
        if (data_len > (int)sizeof(val_buf)) return -3;

        for (int i = 0; i < data_len; i++) {
            val_buf[i] = (uint8_t)(in->data[14 + i] - 0x33);
        }
        *out_val = dlt645_bcd_to_f64(val_buf, data_len);
    } else {
        *out_val = 0;
    }

    // 注意：这里不拉取 buffer，交给调用者决定
    return 0;
}
