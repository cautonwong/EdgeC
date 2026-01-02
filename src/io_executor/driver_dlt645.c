/**
 * @file driver_dlt645.c
 * @brief 工业级 DLT645-2007 协议驱动实现 (Master/Slave 全双工)
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "net_buf.h"
#include "shmring.h"

static uint8_t dlt645_calc_cs(const uint8_t *buf, size_t len) {
    uint8_t cs = 0;
    for (size_t i = 0; i < len; i++) cs += buf[i];
    return cs;
}

static double dlt645_bcd_to_double(const uint8_t *bcd, int bytes) {
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

static void dlt645_double_to_bcd(double val, uint8_t *bcd, int bytes) {
    long long v = (long long)(val + 0.5);
    for (int i = 0; i < bytes; i++) {
        bcd[i] = (uint8_t)(((v / 10 % 10) << 4) | (v % 10));
        v /= 100;
    }
}

static void dlt645_encode_addr(uint64_t addr, uint8_t *out_bcd) {
    for (int i = 0; i < 6; i++) {
        out_bcd[i] = (uint8_t)(((addr / 10 % 10) << 4) | (addr % 10));
        addr /= 100;
    }
}

extern uint32_t edge_lookup_dlt645(uint32_t di, float *out_scale);

static int dlt645_init(void *ctx) {
    (void)ctx;
    return 0;
}

static int dlt645_build_batch_req(edge_poll_group_t *group, uint8_t *tx_buf, size_t *tx_len) {
    int idx = 0;
    tx_buf[idx++] = 0x68;
    dlt645_encode_addr((uint64_t)group->slave_id, &tx_buf[idx]);
    idx += 6;
    tx_buf[idx++] = 0x68;
    tx_buf[idx++] = 0x11;
    tx_buf[idx++] = 0x04;
    uint32_t di = group->start_addr;
    tx_buf[idx++] = (uint8_t)((di & 0xFF) + 0x33);
    tx_buf[idx++] = (uint8_t)(((di >> 8) & 0xFF) + 0x33);
    tx_buf[idx++] = (uint8_t)(((di >> 16) & 0xFF) + 0x33);
    tx_buf[idx++] = (uint8_t)(((di >> 24) & 0xFF) + 0x33);
    uint8_t cs = dlt645_calc_cs(tx_buf, (size_t)idx);
    tx_buf[idx++] = cs;
    tx_buf[idx++] = 0x16;
    *tx_len = (size_t)idx;
    return 0;
}

static int dlt645_parse_batch_res(edge_poll_group_t *group, uint8_t *rx_buf, size_t rx_len,
                                  void (*on_msg)(pipe_msg_t *msg, void *user_data),
                                  void *user_data) {
    (void)group;
    struct net_buf_simple b;
    net_buf_simple_init_with_data(&b, rx_buf, rx_len);
    while (b.len > 0 && b.data[0] == 0xFE) net_buf_simple_pull(&b, 1);
    if (b.len < 12 || b.data[0] != 0x68) return -1;
    if (dlt645_calc_cs(b.data, b.len - 2) != b.data[b.len - 2]) return -2;

    net_buf_simple_pull(&b, 10);
    uint8_t data_len = b.data[-1];
    uint8_t unmask[256];
    for (int i = 0; i < data_len; i++) unmask[i] = (uint8_t)(b.data[i] - 0x33);

    uint32_t di;
    memcpy(&di, unmask, 4);
    float scale = 1.0f;
    uint32_t tid = edge_lookup_dlt645(di, &scale);
    if (tid == 0) return -3;

    double val = dlt645_bcd_to_double(&unmask[4], (int)data_len - 4);
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, tid, val * (double)scale, (uint64_t)time(NULL) * 1000);
    on_msg(&msg, user_data);
    return 0;
}

static int dlt645_parse_request(const uint8_t *rx_buf, size_t rx_len, uint32_t *out_slave_id,
                                uint16_t *out_di) {
    struct net_buf_simple b;
    net_buf_simple_init_with_data(&b, (void *)rx_buf, rx_len);
    while (b.len > 0 && b.data[0] == 0xFE) net_buf_simple_pull(&b, 1);
    if (b.len < 12 || b.data[0] != 0x68) return -1;

    net_buf_simple_pull(&b, 1);
    uint8_t *addr_bcd = net_buf_simple_pull(&b, 6);
    *out_slave_id = (uint32_t)dlt645_bcd_to_double(addr_bcd, 4);

    net_buf_simple_pull(&b, 1);
    uint8_t ctrl = net_buf_simple_pull_u8(&b);
    uint8_t len = net_buf_simple_pull_u8(&b);
    if (ctrl != 0x11 || len < 4) return -2;

    uint8_t di_unmask[4];
    for (int i = 0; i < 4; i++) di_unmask[i] = (uint8_t)(net_buf_simple_pull_u8(&b) - 0x33);
    *out_di = (uint16_t)((di_unmask[1] << 8) | di_unmask[0]);
    return 0;
}

static int dlt645_build_response(uint32_t slave_id, uint8_t ctrl_code, uint32_t di,
                                 const pipe_msg_t *data, uint8_t *tx_buf, size_t *tx_len) {
    struct net_buf_simple b;
    net_buf_simple_init_with_data(&b, tx_buf, 256);
    net_buf_simple_reset(&b);

    net_buf_simple_add_u8(&b, 0x68);
    uint8_t addr_bcd[6];
    dlt645_encode_addr(slave_id, addr_bcd);
    net_buf_simple_add_mem(&b, addr_bcd, 6);
    net_buf_simple_add_u8(&b, 0x68);
    net_buf_simple_add_u8(&b, (uint8_t)(ctrl_code | 0x80));

    net_buf_simple_add_u8(&b, 8);  // Len

    for (int i = 0; i < 4; i++) {
        net_buf_simple_add_u8(&b, (uint8_t)((di & 0xFF) + 0x33));
        di >>= 8;
    }

    uint8_t val_bcd[4];
    dlt645_double_to_bcd(data->payload.val.f * 100.0, val_bcd, 4);
    for (int i = 0; i < 4; i++) net_buf_simple_add_u8(&b, (uint8_t)(val_bcd[i] + 0x33));

    uint8_t cs = dlt645_calc_cs(b.data, b.len);
    net_buf_simple_add_u8(&b, cs);
    net_buf_simple_add_u8(&b, 0x16);

    *tx_len = b.len;
    return 0;
}

static edge_driver_t dlt645_driver = {.name = "DLT645-2007",
                                      .init = dlt645_init,
                                      .build_batch_req = dlt645_build_batch_req,
                                      .parse_batch_res = dlt645_parse_batch_res,
                                      .build_request = NULL,
                                      .parse_response = NULL,
                                      .parse_request = dlt645_parse_request,
                                      .build_response = dlt645_build_response,
                                      .cleanup = NULL};

int dlt645_driver_register(void) {
    return driver_manager_register(&dlt645_driver);
}