#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    if (ctx && ctx->priv) {
        modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
        priv->slave_id = ctx->device_id;
    }
    return 0;
}

static void modbus_v3_on_cleanup(edge_driver_ctx_t *ctx) {
    // The memory is freed by driver_instance_destroy, so we don't free it here
    // unless we allocated dynamic structures inside priv.
    (void)ctx;
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
    (void)ctx;
    
    uint8_t slave_id, fc;
    uint8_t *data;
    size_t data_len;
    
    int ret = modbus_rtu_decode_frame(&in->b, &slave_id, &fc, &data, &data_len);
    if (ret == 1) return EDGE_ERR_INCOMPLETE;
    if (ret < 0) {
        *consumed = 1;
        return EDGE_ERR_INCOMPLETE;
    }

    *consumed = (size_t)(data + data_len + 2 - in->b.data);
    
    if (fc & 0x80) {
        LOG_ERROR(IO, "Modbus Slave %u Exception: 0x%02X", slave_id, data[0]);
        return -2;
    }

    if (fc == MODBUS_FC_READ_HOLDING_REGS || fc == MODBUS_FC_READ_INPUT_REGS) {
        /*
         * 【物理闭环】根据契约定义的长短动态解析字节。
         * data[0] 是从站返回的 Byte Count。
         */
        modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
        uint32_t tag_id = priv->last_requested_tag;
        const modbus_map_t *map = NULL;
        if (ctx->lookup_by_tag) {
            typedef const modbus_map_t *(*lookup_fn_t)(uint32_t);
            map = ((lookup_fn_t)ctx->lookup_by_tag)(tag_id);
        }

        if (map) {
            printf("🔍 [MAP-AUDIT] Tag: %u, MapPtr: %p, MapTag: %u, MapLen: %u, MapScale: %f\n",
                   tag_id, (void*)map, map->tag_id, map->length, map->scaler);
        } else {
            printf("⚠️ [MAP-AUDIT] NO MAP FOUND for Tag: %u\n", tag_id);
        }

        double val_double = 0;
        double scale = map ? (double)map->scaler : 1.0;
        int reg_count = map ? map->length : 1;
        int byte_count = (int)data[0];

        if (reg_count == 2 && byte_count >= 4) {
            // 💡 [32-bit ABCD] 物理合包：data[1,2] 是高16位，data[3,4] 是低16位
            uint32_t high = (uint32_t)((data[1] << 8) | data[2]);
            uint32_t low = (uint32_t)((data[3] << 8) | data[4]);
            uint32_t v32 = (high << 16) | low;
            val_double = (double)v32;
            printf("  [32-BIT] Tag: %u, High16: 0x%04X, Low16: 0x%04X, Combined: %u\n", 
                   tag_id, high, low, v32);
        } else if (byte_count >= 2) {
            // 💡 [16-bit] 正常 2 字节解析
            uint16_t v16 = (uint16_t)((data[1] << 8) | data[2]);
            val_double = (double)v16;
        }

        pipe_msg_init_scalar(&out_msgs[0], tag_id, val_double * scale,
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
    
    int ret = modbus_rtu_decode_req_frame(&in->b, &slave_id, &fc, &data, &data_len);
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
                if (!map) {
                    // 如果带 slave_id 的复合键查找不到，则回退到 bare 寄存器地址作为 key（兼容单设备/默认 slave_id 为 0 的场景）
                    map = ((lookup_fn_t)ctx->lookup_by_key)((uint64_t)cur_addr);
                }
            }

            if (map) {
                pipe_msg_t shadow;
                if (edge_driver_read_shadow(ctx, map->tag_id, &shadow) == 0) {
                    rv = (uint16_t)(shadow.payload.val.f / (map->scaler ? map->scaler : 1.0f));
                    LOG_INFO(LOG_CAT_IO, "Modbus Slave: Read Tag %u (Shadow: %.2f) -> Raw %u (Scale: %.4f)", 
                             map->tag_id, shadow.payload.val.f, rv, (double)map->scaler);
                } else {
                    LOG_WARN(LOG_CAT_IO, "Modbus Slave: Failed to read shadow for Tag %u", map->tag_id);
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
            if (!map) {
                // 回退到 bare 寄存器地址作为 key 查找
                map = ((lookup_fn_t)ctx->lookup_by_key)((uint64_t)addr);
            }
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
                if (!map) {
                    // 回退到 bare 寄存器地址作为 key 查找
                    map = ((lookup_fn_t)ctx->lookup_by_key)((uint64_t)(addr + i));
                }
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
