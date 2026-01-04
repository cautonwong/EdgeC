/**
 * @file driver_dlt645.c
 * @brief 工业级 DLT645-2007 协议栈 (V3.0 Master & Slave 全功能版)
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "net_buf.h"

// 协议私有状态
typedef struct {
    uint8_t addr_bcd[6]; /**< 缓存的 BCD 格式地址 */
    uint32_t last_di;    /**< 上次请求的 DI */
    uint16_t seq;        /**< 事务序列号 */
} dlt645_ctx_t;

// --- 基础工具 ---

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
        uint8_t low = (uint8_t)(v % 10);
        v /= 10;
        uint8_t high = (uint8_t)(v % 10);
        v /= 10;
        bcd[i] = (uint8_t)((high << 4) | low);
    }
}

static void dlt645_encode_addr(uint64_t addr, uint8_t *out_bcd) {
    for (int i = 0; i < 6; i++) {
        out_bcd[i] = (uint8_t)(((addr / 10 % 10) << 4) | (addr % 10));
        addr /= 100;
    }
}

extern uint32_t edge_lookup_dlt645_num(uint32_t di, float *out_scale);

/* --- V3 Master 接口 --- */

static int dlt645_v3_on_init(edge_driver_ctx_t *ctx) {
    dlt645_ctx_t *priv = (dlt645_ctx_t *)ctx->priv;
    dlt645_encode_addr((uint64_t)ctx->device_id, priv->addr_bcd);
    return 0;
}

static int dlt645_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int tag_count,
                                    net_buf_t *out) {
    if (tag_count == 0) return -1;
    dlt645_ctx_t *priv = (dlt645_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &out->b;

    uint32_t di = tag_ids[0];
    priv->last_di = di;

    // 公共帧头
    net_buf_simple_add_u8(b, 0x68);
    net_buf_simple_add_mem(b, priv->addr_bcd, 6);
    net_buf_simple_add_u8(b, 0x68);

    if (tag_count == 1) { // 读请求
        net_buf_simple_add_u8(b, 0x11);  // 读数据
        net_buf_simple_add_u8(b, 0x04);  // 长度
        for (int i = 0; i < 4; i++) {
            net_buf_simple_add_u8(b, (uint8_t)((di & 0xFF) + 0x33));
            di >>= 8;
        }
    } else if (tag_count >= 2) { // 写请求
        // 实际场景应根据 DI 判断数据类型和长度, 此处简化为2字节
        uint8_t data_body_len = 2;
        uint8_t total_data_len = 4 + 4 + 4 + data_body_len; // DI+PWD+OP+Data

        net_buf_simple_add_u8(b, 0x14); // 写数据
        net_buf_simple_add_u8(b, total_data_len);
        
        for (int i = 0; i < 4; i++) {
            net_buf_simple_add_u8(b, (uint8_t)((di & 0xFF) + 0x33));
            di >>= 8;
        }
        // 密码 + 操作员代码 (mock)
        for (int i = 0; i < 8; i++) net_buf_simple_add_u8(b, 0x33);

        uint32_t val_to_write = tag_ids[1];
        uint8_t bcd_buf[2];
        dlt645_double_to_bcd((double)val_to_write, bcd_buf, 2);
        net_buf_simple_add_u8(b, bcd_buf[0] + 0x33);
        net_buf_simple_add_u8(b, bcd_buf[1] + 0x33);
    }

    net_buf_simple_add_u8(b, dlt645_calc_cs(b->data, b->len));
    net_buf_simple_add_u8(b, 0x16);
    return 0;
}

static int dlt645_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, pipe_msg_t *out_msgs,
                                     int *msg_count) {
    (void)ctx;
    struct net_buf_simple *b = &in->b;
    while (b->len > 0 && b->data[0] == 0xFE) net_buf_simple_pull(b, 1);
    if (b->len < 12 || b->data[0] != 0x68) return -1;
    if (dlt645_calc_cs(b->data, b->len - 2) != b->data[b->len - 2]) return -2;

    uint8_t ctrl = b->data[8];

    // 处理异常响应帧
    if (ctrl & 0x80 && ctrl != 0x91 && ctrl != 0x94) {
        LOG_WARN("SIM", "DLT645 error response, ctrl=0x%02X", (unsigned int)ctrl);
        return -5; // 返回特定错误码表示设备拒绝
    }

    if(ctrl == 0x94) { // 写成功响应
        *msg_count = 0;
        return 0;
    }

    net_buf_simple_pull(b, 9);
    uint8_t data_len = net_buf_simple_pull_u8(b);
    if (data_len < 4) return -3;

    uint32_t resp_di = 0;
    for (int i = 0; i < 4; i++) {
        resp_di |= (uint32_t)(net_buf_simple_pull_u8(b) - 0x33) << (i * 8);
    }

    float scaler = 1.0f;
    uint32_t tid = edge_lookup_dlt645_num(resp_di, &scaler);
    if (tid == 0) return -4;

    uint8_t val_buf[16];
    int val_len = (int)data_len - 4;
    net_buf_simple_pull_mem(b, val_buf, (size_t)val_len);
    for (int i = 0; i < val_len; i++) val_buf[i] -= 0x33;

    double val = dlt645_bcd_to_double(val_buf, val_len);
    pipe_msg_init_scalar(&out_msgs[0], tid, val * (double)scaler, (uint64_t)time(NULL) * 1000);
    *msg_count = 1;
    return 0;
}

/* --- V3 Slave / Spontaneous 接口 --- */

static int dlt645_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                               pipe_msg_t *async_msgs, int *async_count) {
    dlt645_ctx_t *priv = (dlt645_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;

    // 1. 校验请求帧
    while (b->len > 0 && b->data[0] == 0xFE) net_buf_simple_pull(b, 1);
    if (b->len < 12 || b->data[0] != 0x68 || b->data[b->len - 1] != 0x16) return -1;
    if (dlt645_calc_cs(b->data, b->len - 2) != b->data[b->len - 2]) return -2;

    // 2. 匹配地址 (工业级严谨：非本设备地址不回)
    if (memcmp(b->data + 1, priv->addr_bcd, 6) != 0) return 0;  // 静默忽略

    uint8_t ctrl = b->data[8];
    
    // 3. 构建响应
    if (resp_out) {
        struct net_buf_simple *rb = &resp_out->b;
        net_buf_simple_add_u8(rb, 0x68);
        net_buf_simple_add_mem(rb, priv->addr_bcd, 6);
        net_buf_simple_add_u8(rb, 0x68);

        if (ctrl == 0x11) { // 响应读
            net_buf_simple_add_u8(rb, 0x91);  // 读成功响应
            // 模拟回复数据：220.0V -> DI: 02010100
            net_buf_simple_add_u8(rb, 6);  // DI(4) + Data(2)
            net_buf_simple_add_u8(rb, 0x00 + 0x33);
            net_buf_simple_add_u8(rb, 0x01 + 0x33);
            net_buf_simple_add_u8(rb, 0x01 + 0x33);
            net_buf_simple_add_u8(rb, 0x02 + 0x33);
            net_buf_simple_add_u8(rb, 0x00 + 0x33);
            net_buf_simple_add_u8(rb, 0x22 + 0x33);
        } else if (ctrl == 0x14) { // 响应写
            net_buf_simple_add_u8(rb, 0x94); // 写成功响应
            net_buf_simple_add_u8(rb, 0); // 无数据体
        } else {
             // 不支持的控制码
             return -3;
        }

        net_buf_simple_add_u8(rb, dlt645_calc_cs(rb->data, rb->len));
        net_buf_simple_add_u8(rb, 0x16);
    }

    (void)async_msgs;
    (void)async_count;
    (void)dlt645_double_to_bcd;  // 抑制警告
    return 0;
}

edge_driver_t dlt645_driver = {.name = "DLT645-2007",
                               .ctx_size = sizeof(dlt645_ctx_t),
                               .on_init = dlt645_v3_on_init,
                               .encode_request = dlt645_v3_encode_request,
                               .decode_response = dlt645_v3_decode_response,
                               .handle_rx = dlt645_v3_handle_rx,
                               .on_cleanup = NULL};

int dlt645_driver_register(void) {
    return driver_manager_register(&dlt645_driver);
}