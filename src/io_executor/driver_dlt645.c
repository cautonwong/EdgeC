/**
 * @file driver_dlt645.c
 * @brief 工业级 DLT645-2007 协议栈 (Master & Slave 重塑版)
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "net_buf.h"

extern uint32_t edge_lookup_dlt645(uint32_t di, float *out_scale);

typedef struct {
    uint8_t addr_bcd[6];
} dlt645_ctx_t;

// --- DLT645 核心工具 ---

static uint8_t _calc_cs(const uint8_t *buf, size_t len) {
    uint8_t cs = 0;
    for (size_t i = 0; i < len; i++) cs += buf[i];
    return cs;
}

static double _bcd_to_f64(const uint8_t *bcd, int bytes) {
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

static void _u64_to_bcd_addr(uint64_t addr, uint8_t *out) {
    for (int i = 0; i < 6; i++) {
        out[i] = (uint8_t)(((addr / 10 % 10) << 4) | (addr % 10));
        addr /= 100;
    }
}

static int dlt645_v3_on_init(edge_driver_ctx_t *ctx) {
    dlt645_ctx_t *priv = (dlt645_ctx_t *)ctx->priv;
    _u64_to_bcd_addr((uint64_t)ctx->device_id, priv->addr_bcd);
    return 0;
}

/* --- Master: 编码请求 --- */
static int dlt645_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int count,
                                    net_buf_t *out) {
    if (count == 0) return -1;
    dlt645_ctx_t *priv = (dlt645_ctx_t *)ctx->priv;

    // 假设 tag_ids[0] 已经是 DI 格式 (由契约编译器保证)
    uint32_t di = tag_ids[0];

    struct net_buf_simple *b = &out->b;
    net_buf_simple_add_u8(b, 0x68);
    net_buf_simple_add_mem(b, priv->addr_bcd, 6);
    net_buf_simple_add_u8(b, 0x68);
    net_buf_simple_add_u8(b, 0x11);  // 读数据
    net_buf_simple_add_u8(b, 0x04);  // 长度 (DI 4字节)

    // DI 编码 (LE + 0x33)
    net_buf_simple_add_u8(b, (uint8_t)((di & 0xFF) + 0x33));
    net_buf_simple_add_u8(b, (uint8_t)(((di >> 8) & 0xFF) + 0x33));
    net_buf_simple_add_u8(b, (uint8_t)(((di >> 16) & 0xFF) + 0x33));
    net_buf_simple_add_u8(b, (uint8_t)(((di >> 24) & 0xFF) + 0x33));

    net_buf_simple_add_u8(b, _calc_cs(b->data, b->len));
    net_buf_simple_add_u8(b, 0x16);
    return 0;
}

/* --- Master: 解析响应 --- */
static int dlt645_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, pipe_msg_t *out_msgs,
                                     int *msg_count) {
    struct net_buf_simple *b = &in->b;
    // 跳过前导 FE
    while (b->len > 0 && b->data[0] == 0xFE) net_buf_simple_pull(b, 1);

    if (b->len < 12 || b->data[0] != 0x68) return 1;  // 继续等待
    if (_calc_cs(b->data, b->len - 2) != b->data[b->len - 2]) return -1;

    // 解析 DI
    uint32_t resp_di = 0;
    resp_di |= (uint32_t)(b->data[10] - 0x33);
    resp_di |= (uint32_t)(b->data[11] - 0x33) << 8;
    resp_di |= (uint32_t)(b->data[12] - 0x33) << 16;
    resp_di |= (uint32_t)(b->data[13] - 0x33) << 24;

    float scale = 1.0f;
    uint32_t tid = edge_lookup_dlt645(resp_di, &scale);
    if (tid == 0) return -1;

    // 数据域处理 (-0x33)
    int data_len = b->data[9] - 4;  // 减去 DI 长度
    if (data_len <= 0) return -1;

    uint8_t val_buf[16];
    memcpy(val_buf, &b->data[14], (size_t)data_len);
    for (int i = 0; i < data_len; i++) val_buf[i] -= 0x33;

    double val = _bcd_to_f64(val_buf, data_len);
    pipe_msg_init_scalar(&out_msgs[0], tid, val * (double)scale, (uint64_t)time(NULL) * 1000);
    *msg_count = 1;

    return 0;
}

/* --- Slave: 被动响应 (仿真支持) --- */

static int dlt645_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,

                               pipe_msg_t *async_msgs, int *async_count) {
    (void)async_msgs;
    (void)async_count;

    dlt645_ctx_t *priv = (dlt645_ctx_t *)ctx->priv;

    struct net_buf_simple *b = &in->b;

    if (b->len < 12 || b->data[0] != 0x68) return 1;

    if (memcmp(&b->data[1], priv->addr_bcd, 6) != 0) return 1;  // 非本站

    uint8_t ctrl = b->data[8];

    if (ctrl == 0x11) {  // 读数据

        uint32_t di = 0;

        di |= (uint32_t)(b->data[10] - 0x33);

        di |= (uint32_t)(b->data[11] - 0x33) << 8;

        di |= (uint32_t)(b->data[12] - 0x33) << 16;

        di |= (uint32_t)(b->data[13] - 0x33) << 24;

        // 构造响应 (固定返回一个模拟值)

        struct net_buf_simple *rb = &resp_out->b;

        net_buf_simple_add_u8(rb, 0x68);

        net_buf_simple_add_mem(rb, priv->addr_bcd, 6);

        net_buf_simple_add_u8(rb, 0x68);

        net_buf_simple_add_u8(rb, 0x91);  // 读成功响应

        net_buf_simple_add_u8(rb, 0x08);  // 长度 (DI 4 + Data 4)

        // DI

        net_buf_simple_add_u8(rb, (uint8_t)((di & 0xFF) + 0x33));

        net_buf_simple_add_u8(rb, (uint8_t)(((di >> 8) & 0xFF) + 0x33));

        net_buf_simple_add_u8(rb, (uint8_t)(((di >> 16) & 0xFF) + 0x33));

        net_buf_simple_add_u8(rb, (uint8_t)(((di >> 24) & 0xFF) + 0x33));

        // Data: 230.5V -> 05 30 20 00

        net_buf_simple_add_u8(rb, 0x05 + 0x33);

        net_buf_simple_add_u8(rb, 0x30 + 0x33);

        net_buf_simple_add_u8(rb, 0x20 + 0x33);

        net_buf_simple_add_u8(rb, 0x00 + 0x33);

        net_buf_simple_add_u8(rb, _calc_cs(rb->data, rb->len));

        net_buf_simple_add_u8(rb, 0x16);
    }

    return 0;
}

const edge_driver_t dlt645_driver = {.name = "DLT645-2007",
                                     .ctx_size = sizeof(dlt645_ctx_t),
                                     .on_init = dlt645_v3_on_init,
                                     .encode_request = dlt645_v3_encode_request,
                                     .decode_response = dlt645_v3_decode_response,
                                     .handle_rx = dlt645_v3_handle_rx};

int dlt645_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &dlt645_driver);
}
