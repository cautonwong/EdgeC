#include "dlt645_codec.h"
#include <string.h>
#include <math.h>

uint8_t dlt645_calculate_cs(const uint8_t *buf, size_t len) {
    uint8_t cs = 0;
    for (size_t i = 0; i < len; i++) {
        cs += buf[i];
    }
    return cs;
}

void dlt645_u64_to_bcd_addr(uint64_t addr, uint8_t *out_bcd) {
    for (int i = 0; i < 6; i++) {
        uint8_t byte_val = (uint8_t)(addr % 100);
        out_bcd[i] = ((byte_val / 10) << 4) | (byte_val % 10);
        addr /= 100;
    }
}

double dlt645_bcd_to_f64(const uint8_t *bcd, int bytes, int decimals) {
    double val = 0;
    double factor = 1.0;
    for (int i = 0; i < bytes; i++) {
        uint8_t low = bcd[i] & 0x0F;
        uint8_t high = (bcd[i] >> 4) & 0x0F;
        val += (double)(low + high * 10) * factor;
        factor *= 100.0;
    }
    return val / pow(10, decimals);
}

int dlt645_encode_frame(const uint8_t *addr_bcd, uint8_t ctrl, uint32_t di, 
                        const uint8_t *data, size_t data_len, net_buf_t *out) {
    // FE FE FE FE 68 A0 A1 A2 A3 A4 A5 68 C L DI0 DI1 DI2 DI3 D0 .. Dn CS 16
    size_t total_payload_len = 4 + data_len; // DI(4) + DATA(N)
    size_t frame_len = 1 + 6 + 1 + 1 + 1 + total_payload_len + 1 + 1;
    
    uint8_t *p = net_buf_add(out, 4 + frame_len);
    memset(p, 0xFE, 4);
    uint8_t *f = p + 4;
    
    f[0] = 0x68;
    memcpy(&f[1], addr_bcd, 6);
    f[7] = 0x68;
    f[8] = ctrl;
    f[9] = (uint8_t)total_payload_len;
    
    // DI with +0x33 (Little Endian in 645)
    f[10] = (uint8_t)((di & 0xFF) + 0x33);
    f[11] = (uint8_t)(((di >> 8) & 0xFF) + 0x33);
    f[12] = (uint8_t)(((di >> 16) & 0xFF) + 0x33);
    f[13] = (uint8_t)(((di >> 24) & 0xFF) + 0x33);
    
    for (size_t i = 0; i < data_len; i++) {
        f[14 + i] = data[i] + 0x33;
    }
    
    f[14 + data_len] = dlt645_calculate_cs(f, 14 + data_len);
    f[15 + data_len] = 0x16;
    
    return 0;
}

int dlt645_build_ctrl_request(const uint8_t *addr_bcd, uint8_t cmd_type,
                              const uint8_t deadline_bcd[6], const uint8_t password[4],
                              const uint8_t operator_code[4], net_buf_t *out) {
    if (!addr_bcd || !password || !operator_code || !out) return -1;

    /* 数据域（未加 33H 前）：密码4 | 操作者代码4 | N1 N2 N3..N8 = 16 字节
     * 帧：68 A0..A5 68 1C L ... CS 16，L = 8 + 8 = 16 */
    const size_t data_len = 16;
    uint8_t data[16];
    memcpy(data, password, 4);
    memcpy(data + 4, operator_code, 4);
    data[8] = cmd_type; /* N1：命令类型 */
    data[9] = 0x00;     /* N2：保留（标准 7.13 注 1） */
    for (size_t i = 0; i < 6; i++) {
        data[10 + i] = deadline_bcd ? deadline_bcd[i] : 0x00;
    }

    const size_t frame_len = 1 + 6 + 1 + 1 + 1 + data_len + 1 + 1;
    uint8_t *p = net_buf_add(out, 4 + frame_len);
    if (!p) return -1;
    memset(p, 0xFE, 4); /* 前导 FE×4：唤醒接收方（标准 5.3.1） */
    uint8_t *f = p + 4;

    f[0] = 0x68;
    memcpy(&f[1], addr_bcd, 6);
    f[7] = 0x68;
    f[8] = DLT645_CTRL_TRIP_CONTROL;
    f[9] = (uint8_t)data_len;

    /* 数据域每字节 +33H（标准 5.2.5，发送方加、接收方减） */
    for (size_t i = 0; i < data_len; i++) {
        f[10 + i] = (uint8_t)(data[i] + 0x33);
    }

    f[10 + data_len] = dlt645_calculate_cs(f, 10 + data_len);
    f[11 + data_len] = 0x16;
    return 0;
}

int dlt645_decode_frame(struct net_buf_simple *in, uint8_t *addr_bcd, uint8_t *ctrl,
                        uint32_t *di, uint8_t **payload_out, size_t *payload_len) {
    // Skip FE
    while (in->len > 0 && in->data[0] == 0xFE) {
        net_buf_simple_pull(in, 1);
    }
    
    if (in->len < 12) return 1; // ID(1) + ADDR(6) + ID(1) + CTRL(1) + LEN(1) + ... + CS(1) + END(1)
    if (in->data[0] != 0x68 || in->data[7] != 0x68) return -2;
    
    uint8_t len = in->data[9];
    size_t frame_full_len = 10 + len + 2;
    if (in->len < frame_full_len) return 1;
    
    if (in->data[frame_full_len - 1] != 0x16) return -2;
    
    uint8_t cs_calc = dlt645_calculate_cs(in->data, frame_full_len - 2);
    if (cs_calc != in->data[frame_full_len - 2]) return -1;
    
    if (addr_bcd) memcpy(addr_bcd, &in->data[1], 6);
    if (ctrl) *ctrl = in->data[8];
    
    if (len >= 4) {
        // DI with -0x33
        uint32_t res_di = (uint32_t)(in->data[10] - 0x33);
        res_di |= (uint32_t)(in->data[11] - 0x33) << 8;
        res_di |= (uint32_t)(in->data[12] - 0x33) << 16;
        res_di |= (uint32_t)(in->data[13] - 0x33) << 24;
        if (di) *di = res_di;
        
        if (payload_out) *payload_out = &in->data[14];
        if (payload_len) *payload_len = len - 4;
    } else {
        if (payload_out) *payload_out = &in->data[10];
        if (payload_len) *payload_len = len;
    }
    
    return 0;
}
