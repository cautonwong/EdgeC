#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "net_buf.h"
#include "datahub.h"

typedef struct {
    uint8_t addr_bcd[6];
} dlt645_ctx_t;

static uint8_t _calc_cs(const uint8_t *buf, size_t len) {
    uint8_t cs = 0;
    for (size_t i = 0; i < len; i++) cs += buf[i];
    return cs;
}

static int dlt645_v3_on_init(edge_driver_ctx_t *ctx) {
    dlt645_ctx_t *priv = (dlt645_ctx_t *)ctx->priv;
    memset(priv->addr_bcd, 0x99, 6);  // 默认广播地址
    return 0;
}

static int dlt645_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int tag_count,
                                    net_buf_t *out) {
    if (tag_count == 0) return -1;
    dlt645_ctx_t *priv = (dlt645_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &out->b;
    net_buf_simple_add_u8(b, 0x68);
    net_buf_simple_add_mem(b, priv->addr_bcd, 6);
    net_buf_simple_add_u8(b, 0x68);
    net_buf_simple_add_u8(b, 0x11);
    net_buf_simple_add_u8(b, 0x04);
    uint32_t di = tag_ids[0];
    net_buf_simple_add_u8(b, (uint8_t)((di & 0xFF) + 0x33));
    net_buf_simple_add_u8(b, (uint8_t)(((di >> 8) & 0xFF) + 0x33));
    net_buf_simple_add_u8(b, (uint8_t)(((di >> 16) & 0xFF) + 0x33));
    net_buf_simple_add_u8(b, (uint8_t)(((di >> 24) & 0xFF) + 0x33));
    net_buf_simple_add_u8(b, _calc_cs(b->data, b->len));
    net_buf_simple_add_u8(b, 0x16);
    return 0;
}

static int dlt645_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                     pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    (void)resp_out;
    struct net_buf_simple *b = &in->b;
    *consumed = 0;
    *msg_count = 0;

    /* 流同步: 寻找起始字节 0x68 */
    size_t skip = 0;
    while (skip < b->len && b->data[skip] != 0x68) skip++;
    if (skip > 0) { *consumed = skip; return 1; }

    if (b->len < 12) return EDGE_ERR_INCOMPLETE;

    uint8_t ctrl = b->data[8];
    uint8_t data_len = b->data[9];

    /* 校验帧长度 */
    if (b->len < (size_t)(data_len + 12)) return EDGE_ERR_INCOMPLETE;
    *consumed = (size_t)(data_len + 12);

    /* 校验和 */
    uint8_t rx_cs = b->data[data_len + 10];
    uint8_t calc_cs = _calc_cs(b->data, (size_t)(data_len + 10));
    if (rx_cs != calc_cs) return -1;

    /* 解析主站响应 (0x91 = 读数据应答) */
    if (ctrl == 0x91 && data_len >= 8) {
        uint32_t di = (uint32_t)((b->data[10]-0x33) | ((b->data[11]-0x33) << 8) |
                                 ((b->data[12]-0x33) << 16) | ((b->data[13]-0x33) << 24));

        double val = 0;
        bool found = false;

        if (ctx->lookup_by_key) {
            const dlt645_map_t *map = (const dlt645_map_t *)ctx->lookup_by_key(di);
            if (map) {
                /* BCD 解码: 每个字节减去 0x33 后按 BCD 排列 */
                int32_t raw = 0;
                int digit_pos = 0;
                for (int idx = 14; idx < (int)(10 + data_len - 2); idx++) {
                    uint8_t bcd = (uint8_t)(b->data[idx] - 0x33);
                    raw += (int32_t)((bcd >> 4) * 10 + (bcd & 0x0F)) * (int32_t)pow(100, digit_pos);
                    digit_pos++;
                }
                val = (double)raw * map->scaler;
                found = true;
            }
        }

        if (found) {
            edge_tag_id_t tid = ((const dlt645_map_t *)ctx->lookup_by_key(di))->tag_id;
            pipe_msg_init_scalar(&out_msgs[0], tid, val, uv_hrtime());
            *msg_count = 1;
        }
    }

    return 0;
}

static int dlt645_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                               pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    (void)async_msgs; (void)async_count;
    struct net_buf_simple *b = &in->b;
    size_t skip = 0;
    while (skip < b->len && b->data[skip] != 0x68) skip++;
    if (skip > 0) { *consumed = skip; return 1; }
    if (b->len < 12) return EDGE_ERR_INCOMPLETE;

    dlt645_ctx_t *priv = (dlt645_ctx_t *)ctx->priv;
    if (memcmp(&b->data[1], priv->addr_bcd, 6) != 0 &&
        memcmp(&b->data[1], "\x99\x99\x99\x99\x99\x99", 6) != 0) {
        *consumed = 1;
        return 1;
    }

    uint8_t len = b->data[9];
    if (b->len < (size_t)(len + 12)) return EDGE_ERR_INCOMPLETE;
    *consumed = (size_t)(len + 12);

    uint8_t ctrl = b->data[8];
    if (ctrl == 0x11) { // 读数据
        uint32_t di = (uint32_t)((b->data[10]-0x33) | ((b->data[11]-0x33) << 8) | 
                                 ((b->data[12]-0x33) << 16) | ((b->data[13]-0x33) << 24));
        
        double val = 0;
        bool found = false;
        if (ctx->lookup_by_key) {
            const modbus_map_t *map = (const modbus_map_t *)ctx->lookup_by_key(di);
            if (map && ctx->user_data) {
                pipe_msg_t shadow;
                if (datahub_read_shadow((datahub_t *)ctx->user_data, map->tag_id, &shadow) == 0) {
                    val = shadow.payload.val.f / (map->scaler ? map->scaler : 1.0);
                    found = true;
                }
            }
        }

        if (found) {
            struct net_buf_simple *rb = &resp_out->b;
            net_buf_simple_add_u8(rb, 0x68);
            net_buf_simple_add_mem(rb, priv->addr_bcd, 6);
            net_buf_simple_add_u8(rb, 0x68);
            net_buf_simple_add_u8(rb, 0x91); // 读数据响应
            net_buf_simple_add_u8(rb, 0x08); // 4(DI) + 4(DATA)
            // DI
            net_buf_simple_add_u8(rb, b->data[10]);
            net_buf_simple_add_u8(rb, b->data[11]);
            net_buf_simple_add_u8(rb, b->data[12]);
            net_buf_simple_add_u8(rb, b->data[13]);
            // Data (假设 4 字节 BCD, 0.01 精度)
            uint32_t raw_val = (uint32_t)(val * 100);
            net_buf_simple_add_u8(rb, (uint8_t)(((raw_val % 100) / 10 << 4 | (raw_val % 10)) + 0x33));
            net_buf_simple_add_u8(rb, (uint8_t)(((raw_val / 100 % 100) / 10 << 4 | (raw_val / 100 % 10)) + 0x33));
            net_buf_simple_add_u8(rb, (uint8_t)(((raw_val / 10000 % 100) / 10 << 4 | (raw_val / 10000 % 10)) + 0x33));
            net_buf_simple_add_u8(rb, (uint8_t)(((raw_val / 1000000 % 100) / 10 << 4 | (raw_val / 1000000 % 10)) + 0x33));
            net_buf_simple_add_u8(rb, _calc_cs(rb->data, rb->len));
            net_buf_simple_add_u8(rb, 0x16);
        }
        return 0;
    }

    return -1;
}

edge_driver_t dlt645_driver = {.name = "DLT645",
                               .ctx_size = sizeof(dlt645_ctx_t),
                               .on_init = dlt645_v3_on_init,
                               .encode_request = dlt645_v3_encode_request,
                               .decode_response = dlt645_v3_decode_response,
                               .handle_rx = dlt645_v3_handle_rx};

int dlt645_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &dlt645_driver);
}