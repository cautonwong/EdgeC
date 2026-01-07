/**
 * @file driver_iec104.c
 * @brief 超级高度版 IEC 60870-5-104 协议栈实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_driver.h"

// IEC104 私有上下文：体现“有状态”高度
typedef struct {
    uint16_t send_seq; /**< V(S) */
    uint16_t recv_seq; /**< V(R) */
    uint16_t ack_seq;  /**< 上次确认的序号 */
    bool gi_active;    /**< 总召唤状态标识 */
    uint64_t last_msg_ts;
} iec104_ctx_t;

// --- 基础工具 (内联) ---
static inline void _pack_i_frame(struct net_buf_simple *b, uint16_t vs, uint16_t vr) {
    net_buf_simple_add_u8(b, 0x68);
    net_buf_simple_add_u8(b, 0);  // 长度占位
    net_buf_simple_add_le16(b, (uint16_t)(vs << 1));
    net_buf_simple_add_le16(b, (uint16_t)(vr << 1));
}

static int iec104_v3_on_init(edge_driver_ctx_t *ctx) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    memset(priv, 0, sizeof(*priv));
    uv_loop_t *loop = (uv_loop_t *)ctx->user_data;
    priv->last_msg_ts = loop ? uv_now(loop) : 0;
    return 0;
}

static int iec104_v3_on_tick(edge_driver_ctx_t *ctx, net_buf_t **out_tx) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;

    // 逻辑补强：如果 10 秒没收到包，自动构造一个 U 帧心跳 (TESTFR act)
    // 此处简化，仅演示状态机如何被 tick 驱动
    if (priv->recv_seq > priv->ack_seq) {
        // 需要发送 S 帧确认
        return 0;
    }
    (void)out_tx;
    return 0;
}

static int iec104_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                               pipe_msg_t *async_msgs, int *async_count) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;

    if (b->len < 6 || b->data[0] != 0x68) return -1;

    uint8_t apdu_len = b->data[1];
    (void)apdu_len;
    uint8_t ctrl1 = b->data[2];

    // 1. 处理 I 帧 (数据交换)
    if ((ctrl1 & 0x01) == 0) {
        net_buf_simple_pull(b, 2);                       // 跳过 0x68 和长度
        uint16_t vs = net_buf_simple_pull_le16(b) >> 1;  // 对方的 V(S)
        priv->recv_seq = vs + 1;

        // 简单处理：如果是总召唤命令 (TypeID 100)
        // 这里的逻辑才是真正的“高度”：协议栈自主决定业务响应
        if (b->len >= 10 && b->data[4] == 100) {
            priv->gi_active = true;
            // 构造总召唤确认响应 (Mirror back)
            if (resp_out) {
                _pack_i_frame(&resp_out->b, priv->send_seq++, priv->recv_seq);
                net_buf_simple_add_mem(&resp_out->b, b->data, 10);  // 镜像 ASDU
                resp_out->b.data[1] = (uint8_t)(resp_out->b.len - 2);
            }
        }
    }
    // 2. 处理 U 帧 (链路管理)
    else if ((ctrl1 & 0x03) == 3) {
        // TESTFR act -> TESTFR con
    }

    (void)async_msgs;
    (void)async_count;
    return 0;
}

const edge_driver_t iec104_driver = {.name = "IEC104",
                                     .ctx_size = sizeof(iec104_ctx_t),
                                     .on_init = iec104_v3_on_init,
                                     .on_tick = iec104_v3_on_tick,
                                     .encode_request = NULL,  // 104 通常走突发和总召唤，不走轮询
                                     .decode_response = NULL,
                                     .handle_rx = iec104_v3_handle_rx,
                                     .on_cleanup = NULL};
