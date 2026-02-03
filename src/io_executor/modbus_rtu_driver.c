#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "driver_manager.h"
#include "edge_common.h"
#include "edge_driver.h"
#include "modbus_codec.h"
#include "net_buf.h"

typedef struct {
    uint32_t slave_id;
    uint32_t last_requested_tag;
} modbus_ctx_t;

static uint16_t _calc_crc(uint8_t *buf, int len) {
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else
                crc >>= 1;
        }
    }
    return crc;
}

static int modbus_v3_on_init(edge_driver_ctx_t *ctx) {
    modbus_ctx_t *priv = calloc(1, sizeof(modbus_ctx_t));
    priv->slave_id = ctx->device_id;
    ctx->priv = priv;
    return 0;
}

static void modbus_v3_on_cleanup(edge_driver_ctx_t *ctx) {
    if (ctx && ctx->priv) {
        free(ctx->priv);
        ctx->priv = NULL;
    }
}

// --- MASTER PLANE ---

static int modbus_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int tag_count,
                                    net_buf_t *out) {
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
    if (tag_count == 0) return -1;
    uint32_t tag_id = tag_ids[0];
    priv->last_requested_tag = tag_id;

    const modbus_map_t *map = NULL;
    if (ctx->lookup_by_tag) {
        typedef const modbus_map_t *(*lookup_fn_t)(uint32_t);
        map = ((lookup_fn_t)ctx->lookup_by_tag)(tag_id);
    }
    if (!map) return -1;

    uint8_t *p = net_buf_add(out, 8);
    p[0] = (uint8_t)priv->slave_id;
    p[1] = 0x03;
    p[2] = (uint8_t)(map->key >> 8);
    p[3] = (uint8_t)(map->key & 0xFF);
    p[4] = (uint8_t)(map->length >> 8);
    p[5] = (uint8_t)(map->length & 0xFF);
    uint16_t crc = _calc_crc(p, 6);
    p[6] = (uint8_t)(crc & 0xFF);
    p[7] = (uint8_t)(crc >> 8);
    return 0;
}

static int modbus_v3_encode_write(edge_driver_ctx_t *ctx, uint32_t tag_id, double val,
                                  net_buf_t *out) {
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
    const modbus_map_t *map = NULL;
    if (ctx->lookup_by_tag) {
        typedef const modbus_map_t *(*lookup_fn_t)(uint32_t);
        map = ((lookup_fn_t)ctx->lookup_by_tag)(tag_id);
    }
    if (!map) return -1;

    uint16_t raw_val = (uint16_t)(val / (map->scaler ? map->scaler : 1.0));
    uint8_t *p = net_buf_add(out, 8);
    p[0] = (uint8_t)priv->slave_id;
    p[1] = 0x06;
    p[2] = (uint8_t)(map->key >> 8);
    p[3] = (uint8_t)(map->key & 0xFF);
    p[4] = (uint8_t)(raw_val >> 8);
    p[5] = (uint8_t)(raw_val & 0xFF);
    uint16_t crc = _calc_crc(p, 6);
    p[6] = (uint8_t)(crc & 0xFF);
    p[7] = (uint8_t)(crc >> 8);
    return 0;
}

static int modbus_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                     pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    (void)resp_out;
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
    if (in->b.len < 5) return EDGE_ERR_INCOMPLETE;

    uint8_t *data = in->b.data;
    uint8_t func = data[1];
    size_t expected = (func == 0x03 || func == 0x04) ? (size_t)(5 + data[2]) : 8;
    if (in->b.len < expected) return EDGE_ERR_INCOMPLETE;

    uint16_t crc_rx = (uint16_t)(data[expected - 2] | (data[expected - 1] << 8));
    if (_calc_crc(data, (int)expected - 2) != crc_rx) {
        *consumed = 1;
        return -1;
    }

    *consumed = expected;
    if (func == 0x03 || func == 0x04) {
        uint16_t val = (uint16_t)((data[3] << 8) | data[4]);
        double scale = 1.0;
        if (ctx->lookup_by_tag) {
            typedef const modbus_map_t *(*lookup_fn_t)(uint32_t);
            const modbus_map_t *map = ((lookup_fn_t)ctx->lookup_by_tag)(priv->last_requested_tag);
            if (map) scale = (double)map->scaler;
        }
        pipe_msg_init_scalar(&out_msgs[0], priv->last_requested_tag, (double)val * scale,
                             uv_hrtime());
        *msg_count = 1;
    }
    return 0;
}

// --- SLAVE PLANE ---

int modbus_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                        pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    (void)async_msgs;
    (void)async_count;
    if (!in || in->b.len < 8) return EDGE_ERR_INCOMPLETE;

    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
    uint8_t *data = in->b.data;

    if (data[0] != priv->slave_id) {
        *consumed = 1;
        return -1;
    }
    uint16_t crc_rx = (uint16_t)(data[6] | (data[7] << 8));
    if (_calc_crc(data, 6) != crc_rx) {
        *consumed = 1;
        return -1;
    }

    *consumed = 8;
    uint8_t func = data[1];
    uint16_t addr = (uint16_t)((data[2] << 8) | data[3]);
    uint16_t count = (uint16_t)((data[4] << 8) | data[5]);

    if (func == 0x03 || func == 0x04) {
        uint8_t *p = net_buf_add(resp_out, 3 + (size_t)count * 2 + 2);
        p[0] = (uint8_t)priv->slave_id;
        p[1] = func;
        p[2] = (uint8_t)(count * 2);
        for (uint16_t i = 0; i < count; i++) {
            uint16_t rv = 0;
            const modbus_map_t *map = NULL;
            uint32_t cur_addr = (uint32_t)(addr + i);

            // [Nexus Fix] 增强查找逻辑：优先哈希查找，其次列表遍历
            if (ctx->lookup_by_key) {
                typedef const modbus_map_t *(*lookup_fn_t)(uint32_t);
                map = ((lookup_fn_t)ctx->lookup_by_key)(cur_addr);
            }
            if (!map && ctx->meta_list) {
                for (uint32_t m = 0; m < ctx->meta_list_len; m++) {
                    const modbus_map_t *m_e = (const modbus_map_t *)ctx->meta_list + m;
                    if (m_e->key == cur_addr) {
                        map = m_e;
                        break;
                    }
                }
            }

            if (map && ctx->user_data) {
                pipe_msg_t shadow;
                if (datahub_read_shadow((datahub_t *)ctx->user_data, map->tag_id, &shadow) == 0) {
                    rv = (uint16_t)(shadow.payload.val.f / (map->scaler ? map->scaler : 1.0f));
                }
            }
            p[3 + i * 2] = (uint8_t)(rv >> 8);
            p[4 + i * 2] = (uint8_t)(rv & 0xFF);
        }
        uint16_t crc_tx = _calc_crc(p, 3 + count * 2);
        p[3 + count * 2] = (uint8_t)(crc_tx & 0xFF);
        p[4 + count * 2] = (uint8_t)(crc_tx >> 8);
        return 0;
    }
    return -1;
}

edge_driver_t modbus_rtu_driver = {.name = "ModbusRTU",
                                   .ctx_size = sizeof(modbus_ctx_t),
                                   .on_init = modbus_v3_on_init,
                                   .on_cleanup = modbus_v3_on_cleanup,
                                   .encode_request = modbus_v3_encode_request,
                                   .encode_write = modbus_v3_encode_write,
                                   .decode_response = modbus_v3_decode_response,
                                   .handle_rx = modbus_v3_handle_rx};

int modbus_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &modbus_rtu_driver);
}
