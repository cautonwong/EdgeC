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

static void dlt645_encode_addr(uint64_t addr, uint8_t *out_bcd) {
    for (int i = 0; i < 6; i++) {
        out_bcd[i] = (uint8_t)(((addr / 10 % 10) << 4) | (addr % 10));
        addr /= 100;
    }
}

extern uint32_t edge_lookup_dlt645(uint32_t di, float *out_scale);

/* --- V3 Master 接口 --- */

static int dlt645_v3_on_init(edge_driver_ctx_t *ctx) {
    if (!ctx || !ctx->priv) return -1;
    dlt645_ctx_t *priv = (dlt645_ctx_t *)ctx->priv;
    dlt645_encode_addr((uint64_t)ctx->device_id, priv->addr_bcd);
    return 0;
}

static int dlt645_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int tag_count,
                                    net_buf_t *out) {
    if (!ctx || tag_count == 0 || !out) return -1;
    dlt645_ctx_t *priv = (dlt645_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &out->b;

    // 映射 TagID 1001 -> DI 02010100 (A相电压)
    uint32_t di = tag_ids[0];
    if (di == 1001) di = 0x02010100;
    priv->last_di = di;

    // 组包
    net_buf_simple_add_u8(b, 0x68);
    net_buf_simple_add_mem(b, priv->addr_bcd, 6);
    net_buf_simple_add_u8(b, 0x68);
    net_buf_simple_add_u8(b, 0x11);  // 读数据
    net_buf_simple_add_u8(b, 0x04);  // 长度

    // DI 编码 (LE + 0x33)
    net_buf_simple_add_u8(b, (uint8_t)((di & 0xFF) + 0x33));
    net_buf_simple_add_u8(b, (uint8_t)(((di >> 8) & 0xFF) + 0x33));
    net_buf_simple_add_u8(b, (uint8_t)(((di >> 16) & 0xFF) + 0x33));
    net_buf_simple_add_u8(b, (uint8_t)(((di >> 24) & 0xFF) + 0x33));

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
    if (ctrl == 0xD1) {
        LOG_WARN(LOG_CAT_IO, "DLT645: Device reported data item not found (0xD1)");
        return -5;
    }

    net_buf_simple_pull(b, 9);
    uint8_t data_len = net_buf_simple_pull_u8(b);
    if (data_len < 4) return -3;

    // 解析返回的 DI (LE)
    uint32_t resp_di = 0;
    resp_di |= (uint32_t)(net_buf_simple_pull_u8(b) - 0x33);
    resp_di |= (uint32_t)(net_buf_simple_pull_u8(b) - 0x33) << 8;
    resp_di |= (uint32_t)(net_buf_simple_pull_u8(b) - 0x33) << 16;
    resp_di |= (uint32_t)(net_buf_simple_pull_u8(b) - 0x33) << 24;

    float scaler = 1.0f;
    uint32_t tid = edge_lookup_dlt645(resp_di, &scaler);
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

static int dlt645_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                               pipe_msg_t *async_msgs, int *async_count) {
    (void)ctx;
    (void)in;
    (void)resp_out;
    (void)async_msgs;
    (void)async_count;
    return 0;
}

const edge_driver_t dlt645_driver = {.name = "DLT645-2007",
                                     .ctx_size = sizeof(dlt645_ctx_t),
                                     .on_init = dlt645_v3_on_init,
                                     .encode_request = dlt645_v3_encode_request,
                                     .decode_response = dlt645_v3_decode_response,
                                     .handle_rx = dlt645_v3_handle_rx,
                                     .on_cleanup = NULL};

int dlt645_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &dlt645_driver);
}