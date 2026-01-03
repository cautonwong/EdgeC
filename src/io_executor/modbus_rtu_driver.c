/**
 * @file modbus_rtu_driver.c
 * @brief 工业级 Modbus RTU 协议栈实现 (V3.0 升维版)
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "net_buf.h"
#include "shmring.h"

typedef struct {
    uint8_t slave_id;
    uint16_t last_reg;
} modbus_rtu_ctx_t;

// --- 工业级 CRC16 计算 ---
static uint16_t modbus_crc16(const uint8_t *buf, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

extern uint32_t edge_lookup_modbus(uint16_t reg, float *out_scale);

static int modbus_v3_on_init(edge_driver_ctx_t *ctx) {
    modbus_rtu_ctx_t *priv = (modbus_rtu_ctx_t *)ctx->priv;
    priv->slave_id = (uint8_t)ctx->device_id;
    return 0;
}

static int modbus_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int tag_count,
                                    net_buf_t *out) {
    if (tag_count == 0) return -1;
    modbus_rtu_ctx_t *priv = (modbus_rtu_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &out->b;

    // 假设第一个 TagID 映射到寄存器地址
    uint16_t reg_addr = (uint16_t)tag_ids[0];
    priv->last_reg = reg_addr;

    net_buf_simple_add_u8(b, priv->slave_id);
    net_buf_simple_add_u8(b, 0x03);  // Read Holding Registers
    net_buf_simple_add_be16(b, reg_addr);
    net_buf_simple_add_be16(b, 1);  // 只读 1 个寄存器

    uint16_t crc = modbus_crc16(b->data, b->len);
    net_buf_simple_add_le16(b, crc);  // Modbus CRC 是小端

    return 0;
}

static int modbus_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, pipe_msg_t *out_msgs,
                                     int *msg_count) {
    struct net_buf_simple *b = &in->b;
    if (b->len < 5) return -1;

    // 1. CRC 校验
    uint16_t rx_crc = (uint16_t)(b->data[b->len - 2] | (b->data[b->len - 1] << 8));
    if (modbus_crc16(b->data, b->len - 2) != rx_crc) return -2;

    // 2. 解析
    net_buf_simple_pull_u8(b);  // Skip slave_id
    uint8_t func = net_buf_simple_pull_u8(b);
    if (func & 0x80) return -(int)(func & 0x7F);  // 错误响应码

    uint8_t bytes = net_buf_simple_pull_u8(b);
    if (b->len < bytes + 2) return -3;

    modbus_rtu_ctx_t *priv = (modbus_rtu_ctx_t *)ctx->priv;
    float scaler = 1.0f;
    uint32_t tid = edge_lookup_modbus(priv->last_reg, &scaler);
    if (tid == 0) return -4;

    uint16_t raw_val = net_buf_simple_pull_be16(b);

    pipe_msg_init_scalar(&out_msgs[0], tid, (double)raw_val * (double)scaler,
                         (uint64_t)time(NULL) * 1000);
    *msg_count = 1;
    return 0;
}

static int modbus_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                               pipe_msg_t *async_msgs, int *async_count) {
    modbus_rtu_ctx_t *priv = (modbus_rtu_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;

    if (b->len < 4) return -1;
    if (b->data[0] != priv->slave_id) return 0;  // 静默忽略

    uint8_t func = b->data[1];
    if (func == 0x03 && resp_out) {
        // Slave 响应读取请求
        struct net_buf_simple *rb = &resp_out->b;
        net_buf_simple_add_u8(rb, priv->slave_id);
        net_buf_simple_add_u8(rb, 0x03);
        net_buf_simple_add_u8(rb, 2);       // 字节数
        net_buf_simple_add_be16(rb, 1234);  // Mock 数据
        net_buf_simple_add_le16(rb, modbus_crc16(rb->data, rb->len));
    }

    (void)async_msgs;
    (void)async_count;
    return 0;
}

edge_driver_t modbus_rtu_driver = {.name = "ModbusRTU",
                                   .ctx_size = sizeof(modbus_rtu_ctx_t),
                                   .on_init = modbus_v3_on_init,
                                   .encode_request = modbus_v3_encode_request,
                                   .decode_response = modbus_v3_decode_response,
                                   .handle_rx = modbus_v3_handle_rx,
                                   .on_cleanup = NULL};

int modbus_rtu_driver_register(void) {
    return driver_manager_register(&modbus_rtu_driver);
}
