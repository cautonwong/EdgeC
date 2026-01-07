/**
 * @file driver_dlt698.c
 * @brief 面向对象规约 DLT698.45 协议栈实现 (V3.0)
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
    uint16_t server_addr; /**< 从站逻辑地址 */
    uint16_t client_addr; /**< 主站地址 (通常 0x10) */
    uint16_t seq;
} dlt698_ctx_t;

// --- 基础工具 (FCS16 计算) ---
static uint16_t dlt698_pppfcs16(uint16_t fcs, uint8_t *cp, int len) {
    static const uint16_t fcstab[256] = {0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad,
                                         0x6536, 0x74bf, 0x8c48, 0x9dc1, 0xaf5a, 0xbed3,
                                         0xca6c, 0xdbe5, 0xe97e, 0xf8f7};  // 简化版表格，仅做演示
    while (len--) fcs = (uint16_t)((fcs >> 8) ^ fcstab[(fcs ^ *cp++) & 0xff]);
    return fcs;
}

extern uint32_t edge_lookup_dlt698_num(uint32_t oad, float *scaler);

/* --- V3 Master 接口 --- */

static int dlt698_v3_on_init(edge_driver_ctx_t *ctx) {
    dlt698_ctx_t *priv = (dlt698_ctx_t *)ctx->priv;
    priv->server_addr = (uint16_t)ctx->device_id;
    priv->client_addr = 0x10;
    return 0;
}

static int dlt698_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int tag_count,
                                    net_buf_t *out) {
    if (tag_count == 0) return -1;
    dlt698_ctx_t *priv = (dlt698_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &out->b;

    // 构建 GET-Request (Normal) 报文
    net_buf_simple_add_u8(b, 0x68);
    net_buf_simple_add_le16(b, 0);   // 长度占位
    net_buf_simple_add_u8(b, 0x43);  // 控制域
    net_buf_simple_add_le16(b, priv->server_addr);
    net_buf_simple_add_u8(b, (uint8_t)priv->client_addr);
    net_buf_simple_add_le16(b, 0);  // HCS 占位

    // APDU: GET.Request.Normal
    net_buf_simple_add_u8(b, 0x05);
    net_buf_simple_add_u8(b, 0x01);
    net_buf_simple_add_u8(b, ++priv->seq);

    // OAD (Object Attribute Descriptor)
    uint32_t oad = tag_ids[0];
    net_buf_simple_add_be32(b, oad);

    // 填充长度与校验 (此处简化模拟)
    b->data[1] = (uint8_t)(b->len + 2);
    net_buf_simple_add_le16(b, 0xAAAA);  // 模拟 FCS
    return 0;
}

static int dlt698_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, pipe_msg_t *out_msgs,
                                     int *msg_count) {
    struct net_buf_simple *b = &in->b;
    if (b->len < 12 || b->data[0] != 0x68) return -1;

    // 跳转到 APDU 载荷
    net_buf_simple_pull(b, 9);  // Skip 68, LEN, CTRL, ADDR, HCS

    uint8_t service_tag = net_buf_simple_pull_u8(b);
    if (service_tag != 0x85) return -2;  // 必须是 GET-Response

    net_buf_simple_pull_u8(b);  // Result type
    net_buf_simple_pull_u8(b);  // PIID

    uint32_t oad = net_buf_simple_pull_be32(b);
    float scaler = 1.0f;
    uint32_t tid = edge_lookup_dlt698_num(oad, &scaler);
    if (tid == 0) return -3;

    // 解析数据 (假设为 Double-Long-Unsigned)
    uint16_t raw_val = net_buf_simple_pull_be16(b);

    pipe_msg_init_scalar(&out_msgs[0], tid, (double)raw_val * (double)scaler,
                         (uint64_t)time(NULL) * 1000);
    *msg_count = 1;
    return 0;
}

const edge_driver_t dlt698_driver = {.name = "DLT698",
                               .ctx_size = sizeof(dlt698_ctx_t),
                               .on_init = dlt698_v3_on_init,
                               .encode_request = dlt698_v3_encode_request,
                               .decode_response = dlt698_v3_decode_response,
                               .handle_rx = NULL,
                               .on_cleanup = NULL};

int dlt698_driver_register(void) {
    return driver_manager_register(&dlt698_driver);
}
