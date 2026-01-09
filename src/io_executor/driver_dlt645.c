#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "net_buf.h"

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

static int dlt645_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, pipe_msg_t *out_msgs,
                                     int *msg_count, size_t *consumed) {
    (void)ctx;
    (void)out_msgs;
    struct net_buf_simple *b = &in->b;
    if (b->len < 12) return EDGE_ERR_INCOMPLETE;
    *consumed = b->len;
    *msg_count = 0;
    return 0;
}

static int dlt645_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                               pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    (void)ctx;
    (void)resp_out;
    (void)async_msgs;
    (void)async_count;
    if (in->b.len < 12) return EDGE_ERR_INCOMPLETE;
    *consumed = in->b.len;
    return 0;
}

edge_driver_t dlt645_driver = {.name = "DLT645-2007",
                               .ctx_size = sizeof(dlt645_ctx_t),
                               .on_init = dlt645_v3_on_init,
                               .encode_request = dlt645_v3_encode_request,
                               .decode_response = dlt645_v3_decode_response,
                               .handle_rx = dlt645_v3_handle_rx};

int dlt645_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &dlt645_driver);
}