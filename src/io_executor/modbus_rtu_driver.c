#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "driver_manager.h"
#include "edge_common.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "modbus_codec.h"
#include "net_buf.h"

typedef struct {
    uint32_t slave_id;
    uint32_t last_requested_tag;
} modbus_ctx_t;

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

    uint8_t fc = MODBUS_FC_READ_HOLDING_REGS;
    return modbus_rtu_encode_read((uint8_t)priv->slave_id, fc, (uint16_t)map->key, (uint16_t)map->length, out);
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
    return modbus_rtu_encode_write_single((uint8_t)priv->slave_id, (uint16_t)map->key, raw_val, out);
}

static int modbus_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                     pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    (void)resp_out;
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
    
    uint8_t slave_id, fc;
    uint8_t *data;
    size_t data_len;
    
    int ret = modbus_rtu_decode_frame(&in->b, &slave_id, &fc, &data, &data_len);
    if (ret == 1) return EDGE_ERR_INCOMPLETE;
    if (ret < 0) {
        *consumed = 1;
        return -1;
    }

    *consumed = (size_t)(data + data_len + 2 - in->b.data);
    
    if (fc & 0x80) {
        LOG_ERROR(IO, "Modbus Slave %u Exception: 0x%02X", slave_id, data[0]);
        return -2;
    }

    if (fc == MODBUS_FC_READ_HOLDING_REGS || fc == MODBUS_FC_READ_INPUT_REGS) {
        uint16_t val = (uint16_t)((data[0] << 8) | data[1]);
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
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
    uint8_t slave_id, fc;
    uint8_t *data;
    size_t data_len;
    
    int ret = modbus_rtu_decode_frame(&in->b, &slave_id, &fc, &data, &data_len);
    if (ret == 1) return EDGE_ERR_INCOMPLETE;
    if (ret < 0) {
        *consumed = 1;
        return -1;
    }

    *consumed = (size_t)(data + data_len + 2 - in->b.data);

    if (slave_id != priv->slave_id) {
        return -1;
    }

    if (fc == MODBUS_FC_READ_HOLDING_REGS || fc == MODBUS_FC_READ_INPUT_REGS) {
        uint16_t addr = (uint16_t)((data[0] << 8) | data[1]);
        uint16_t count = (uint16_t)((data[2] << 8) | data[3]);

        uint8_t *p = net_buf_add(resp_out, 3 + (size_t)count * 2 + 2);
        p[0] = (uint8_t)priv->slave_id;
        p[1] = fc;
        p[2] = (uint8_t)(count * 2);
        
        for (uint16_t i = 0; i < count; i++) {
            uint16_t rv = 0;
            const modbus_map_t *map = NULL;
            uint32_t cur_addr = (uint32_t)(addr + i);
            uint64_t composite_key = ((uint64_t)priv->slave_id << 32) | cur_addr;

            if (ctx->lookup_by_key) {
                typedef const modbus_map_t *(*lookup_fn_t)(uint64_t);
                map = ((lookup_fn_t)ctx->lookup_by_key)(composite_key);
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
        uint16_t crc_tx = modbus_calculate_crc(p, 3 + (size_t)count * 2);
        p[3 + count * 2] = (uint8_t)(crc_tx & 0xFF);
        p[4 + count * 2] = (uint8_t)(crc_tx >> 8);
        return 0;
    } else if (fc == MODBUS_FC_WRITE_SINGLE_REG) {
        uint16_t addr = (uint16_t)((data[0] << 8) | data[1]);
        uint16_t val = (uint16_t)((data[2] << 8) | data[3]);
        
        uint64_t composite_key = ((uint64_t)priv->slave_id << 32) | addr;
        const modbus_map_t *map = NULL;
        if (ctx->lookup_by_key) {
            typedef const modbus_map_t *(*lookup_fn_t)(uint64_t);
            map = ((lookup_fn_t)ctx->lookup_by_key)(composite_key);
        }
        
        if (map && ctx->user_data) {
            double v = (double)val * (map->scaler ? map->scaler : 1.0);
            pipe_msg_init_scalar(&async_msgs[0], map->tag_id, v, uv_hrtime());
            *async_count = 1;
        }
        
        uint8_t *p = net_buf_add(resp_out, 8);
        memcpy(p, in->b.data, 6);
        uint16_t crc_tx = modbus_calculate_crc(p, 6);
        p[6] = (uint8_t)(crc_tx & 0xFF);
        p[7] = (uint8_t)(crc_tx >> 8);
        return 0;
    } else if (fc == MODBUS_FC_WRITE_MULTIPLE_REGS) {
        uint16_t addr = (uint16_t)((data[0] << 8) | data[1]);
        uint16_t count = (uint16_t)((data[2] << 8) | data[3]);
        uint8_t byte_count = data[4];
        
        if (data_len < (size_t)(5 + byte_count)) return -2;

        for (uint16_t i = 0; i < count; i++) {
            uint16_t val = (uint16_t)((data[5 + i*2] << 8) | data[5 + i*2 + 1]);
            uint64_t composite_key = ((uint64_t)priv->slave_id << 32) | (addr + i);
            const modbus_map_t *map = NULL;
            if (ctx->lookup_by_key) {
                typedef const modbus_map_t *(*lookup_fn_t)(uint64_t);
                map = ((lookup_fn_t)ctx->lookup_by_key)(composite_key);
            }
            if (map && ctx->user_data && *async_count < 16) {
                double v = (double)val * (map->scaler ? map->scaler : 1.0);
                pipe_msg_init_scalar(&async_msgs[(*async_count)++], map->tag_id, v, uv_hrtime());
            }
        }

        uint8_t *p = net_buf_add(resp_out, 8);
        memcpy(p, in->b.data, 6); // 响应格式与请求前6字节相同
        uint16_t crc_tx = modbus_calculate_crc(p, 6);
        p[6] = (uint8_t)(crc_tx & 0xFF);
        p[7] = (uint8_t)(crc_tx >> 8);
        return 0;
    }
    
    modbus_rtu_encode_exception((uint8_t)priv->slave_id, fc, MODBUS_EX_ILLEGAL_FUNCTION, resp_out);
    return 0;
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
