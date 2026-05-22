/**
 * @file modbus_tcp_driver.c
 * @brief Modbus TCP 驱动 (主从双模, MBAP 封装, 无 CRC)
 *
 * 从站模式: 接受 TCP 客户端连接, 响应 FC03/FC04/FC06/FC10
 * 主站模式: 主动连接 TCP 服务器, 发起 FC03/FC04/FC06 读写请求
 */

#include <stdlib.h>
#include <string.h>

#include "contract_data_types.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "modbus_codec.h"
#include "net_buf.h"

typedef struct {
    uint16_t tid;       /* 事务标识符 (自增) */
    datahub_t *hub;
    uint8_t slave_id;   /* 从站模式下本机地址 */
} modbus_tcp_ctx_t;

/* ================================================================
 * 主站侧回调
 * ================================================================ */

static int modbus_tcp_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids,
                                      int tag_count, net_buf_t *out) {
    modbus_tcp_ctx_t *priv = (modbus_tcp_ctx_t *)ctx->priv;
    if (tag_count == 0 || !ctx->meta_list) return -1;

    const modbus_map_t *map = NULL;
    for (uint32_t i = 0; i < ctx->meta_list_len; i++) {
        const modbus_map_t *m = (const modbus_map_t *)ctx->meta_list + i;
        if (m->tag_id == tag_ids[0]) { map = m; break; }
    }
    if (!map) return -1;

    uint16_t tid = (uint16_t)(++priv->tid);
    return modbus_tcp_encode_read(tid, (uint8_t)ctx->device_id,
                                   MODBUS_FC_READ_HOLDING_REGS,
                                   (uint16_t)map->key, map->length, out);
}

static int modbus_tcp_encode_write(edge_driver_ctx_t *ctx, uint32_t tag_id,
                                    double value, net_buf_t *out) {
    modbus_tcp_ctx_t *priv = (modbus_tcp_ctx_t *)ctx->priv;
    if (!ctx->meta_list) return -1;

    const modbus_map_t *map = NULL;
    for (uint32_t i = 0; i < ctx->meta_list_len; i++) {
        const modbus_map_t *m = (const modbus_map_t *)ctx->meta_list + i;
        if (m->tag_id == tag_id) { map = m; break; }
    }
    if (!map) return -1;

    uint16_t tid = (uint16_t)(++priv->tid);
    uint16_t raw = (uint16_t)(value * map->scaler);
    return modbus_tcp_encode_write_single(tid, (uint8_t)ctx->device_id,
                                           (uint16_t)map->key, raw, out);
}

static int modbus_tcp_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *out,
                                       pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    (void)out;
    struct net_buf_simple *b = &in->b;
    *consumed = 0;
    *msg_count = 0;

    uint16_t tid;
    uint8_t slave_id;
    size_t pdu_len;
    int rc = modbus_tcp_decode_mbap(b, &tid, &slave_id, &pdu_len);
    if (rc != 0) {
        if (rc == 1) return EDGE_ERR_INCOMPLETE;
        return -1;
    }
    *consumed = MODBUS_TCP_HEADER_LEN + pdu_len + 1;

    if (pdu_len < 2) return -1;
    net_buf_simple_pull(b, MODBUS_TCP_HEADER_LEN);

    uint8_t fc = b->data[0];
    if (fc & 0x80) {
        LOG_WARN(LOG_CAT_IO, "ModbusTCP: Exception fc=0x%02X code=%d", fc, b->data[1]);
        return -1;
    }

    if ((fc == MODBUS_FC_READ_HOLDING_REGS || fc == MODBUS_FC_READ_INPUT_REGS) && pdu_len >= 3) {
        uint8_t byte_count = b->data[1];
        if (ctx->meta_list_len > 0 && byte_count >= 2) {
            const modbus_map_t *map = (const modbus_map_t *)ctx->meta_list;
            uint16_t raw = (uint16_t)((b->data[2] << 8) | b->data[3]);
            double val = (double)raw / map->scaler;
            pipe_msg_init_scalar(&out_msgs[*msg_count], map->tag_id, val, uv_hrtime());
            (*msg_count)++;
        }
    }

    return 0;
}

/* ================================================================
 * 从站侧回调
 * ================================================================ */

static int modbus_tcp_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                 pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    (void)async_msgs;
    *async_count = 0;
    *consumed = 0;

    modbus_tcp_ctx_t *priv = (modbus_tcp_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;

    uint16_t tid;
    uint8_t slave_id;
    size_t pdu_len;
    int rc = modbus_tcp_decode_mbap(b, &tid, &slave_id, &pdu_len);
    if (rc != 0) {
        if (rc == 1) return EDGE_ERR_INCOMPLETE;
        return -1;
    }

    /* 地址过滤 */
    if (slave_id != 0 && slave_id != priv->slave_id) {
        *consumed = MODBUS_TCP_HEADER_LEN + pdu_len + 1;
        return 1;
    }

    *consumed = MODBUS_TCP_HEADER_LEN + pdu_len + 1;
    net_buf_simple_pull(b, MODBUS_TCP_HEADER_LEN);

    if (pdu_len < 2) return -1;
    uint8_t fc = b->data[0];

    switch (fc) {
        case MODBUS_FC_READ_HOLDING_REGS:
        case MODBUS_FC_READ_INPUT_REGS: {
            if (pdu_len < 5) {
                modbus_tcp_encode_exception(tid, slave_id, fc,
                                             MODBUS_EX_ILLEGAL_DATA_VALUE, resp_out);
                return 0;
            }
            uint16_t start_addr = (uint16_t)((b->data[1] << 8) | b->data[2]);
            uint16_t quantity = (uint16_t)((b->data[3] << 8) | b->data[4]);

            uint8_t resp_data[256];
            size_t resp_len = 0;

            for (uint16_t i = 0; i < quantity; i++) {
                uint16_t addr = (uint16_t)(start_addr + i);
                const modbus_map_t *map = ctx->lookup_by_key
                    ? (const modbus_map_t *)ctx->lookup_by_key(addr)
                    : NULL;

                if (map && priv->hub) {
                    pipe_msg_t shadow;
                    if (datahub_read_shadow(priv->hub, map->tag_id, &shadow) == 0) {
                        uint16_t raw = (uint16_t)(shadow.payload.val.f * map->scaler);
                        resp_data[resp_len++] = (uint8_t)(raw >> 8);
                        resp_data[resp_len++] = (uint8_t)(raw & 0xFF);
                    } else {
                        resp_data[resp_len++] = 0;
                        resp_data[resp_len++] = 0;
                    }
                } else {
                    resp_data[resp_len++] = 0;
                    resp_data[resp_len++] = 0;
                }
            }
            modbus_tcp_encode_read_resp(tid, slave_id, resp_data, resp_len, resp_out);
            return 0;
        }

        case MODBUS_FC_WRITE_SINGLE_REG: {
            if (pdu_len < 5) return -1;
            uint16_t addr = (uint16_t)((b->data[1] << 8) | b->data[2]);
            uint16_t value = (uint16_t)((b->data[3] << 8) | b->data[4]);

            const modbus_map_t *map = ctx->lookup_by_key
                ? (const modbus_map_t *)ctx->lookup_by_key(addr)
                : NULL;

            if (map) {
                double scaled = (double)value / map->scaler;
                pipe_msg_t am;
                pipe_msg_init_scalar(&am, map->tag_id, scaled, uv_hrtime());
                if (priv->hub) datahub_publish(priv->hub, &am);
            }

            modbus_tcp_encode_write_resp(tid, slave_id, addr, value, resp_out);
            return 0;
        }

        case MODBUS_FC_WRITE_MULTIPLE_REGS: {
            if (pdu_len < 8) return -1;
            uint16_t addr = (uint16_t)((b->data[1] << 8) | b->data[2]);
            uint16_t quantity = (uint16_t)((b->data[3] << 8) | b->data[4]);

            for (uint16_t i = 0; i < quantity; i++) {
                uint16_t reg_addr = (uint16_t)(addr + i);
                uint16_t raw = (uint16_t)((b->data[6 + 2*i] << 8) | b->data[7 + 2*i]);
                const modbus_map_t *map = ctx->lookup_by_key
                    ? (const modbus_map_t *)ctx->lookup_by_key(reg_addr)
                    : NULL;
                if (map && priv->hub) {
                    double scaled = (double)raw / map->scaler;
                    pipe_msg_t am;
                    pipe_msg_init_scalar(&am, map->tag_id, scaled, uv_hrtime());
                    datahub_publish(priv->hub, &am);
                }
            }

            net_buf_t *pdu = net_buf_alloc(resp_out->pool, 0);
            uint8_t *p = net_buf_add(pdu, 5);
            p[0] = MODBUS_FC_WRITE_MULTIPLE_REGS;
            p[1] = (uint8_t)(addr >> 8);
            p[2] = (uint8_t)(addr & 0xFF);
            p[3] = (uint8_t)(quantity >> 8);
            p[4] = (uint8_t)(quantity & 0xFF);
            modbus_tcp_encode_mbap(tid, slave_id, 5, resp_out);
            net_buf_simple_add_mem(&resp_out->b, pdu->b.data, pdu->b.len);
            net_buf_unref(pdu);
            return 0;
        }

        default:
            modbus_tcp_encode_exception(tid, slave_id, fc,
                                         MODBUS_EX_ILLEGAL_FUNCTION, resp_out);
            return 0;
    }
}

/* ================================================================
 * 生命周期
 * ================================================================ */

static int modbus_tcp_on_init(edge_driver_ctx_t *ctx) {
    modbus_tcp_ctx_t *priv = (modbus_tcp_ctx_t *)ctx->priv;
    memset(priv, 0, sizeof(modbus_tcp_ctx_t));
    priv->tid = 0;
    priv->hub = (datahub_t *)ctx->user_data;
    priv->slave_id = (uint8_t)ctx->device_id;
    return 0;
}

edge_driver_t modbus_tcp_driver = {
    .name = "ModbusTCP",
    .ctx_size = sizeof(modbus_tcp_ctx_t),
    .on_init = modbus_tcp_on_init,
    .encode_request = modbus_tcp_encode_request,
    .encode_write = modbus_tcp_encode_write,
    .decode_response = modbus_tcp_decode_response,
    .handle_rx = modbus_tcp_handle_rx,
};

int modbus_tcp_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &modbus_tcp_driver);
}
