#include <stdio.h>
#include <string.h>
#include <time.h>

#include "contract_data_types.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_common.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "net_buf.h"

typedef struct {
    uint8_t slave_id;
    uint32_t last_requested_tag;
    uint16_t local_regs[128];
} modbus_ctx_t;

static uint16_t _calc_crc(const uint8_t *buf, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < len; pos++) {
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
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
    priv->slave_id = (uint8_t)(ctx->device_id & 0xFF);
    memset(priv->local_regs, 0, sizeof(priv->local_regs));
    return 0;
}

static int modbus_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int count,
                                    net_buf_t *out) {
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
    if (count == 0) return -1;
    uint32_t tag_id = tag_ids[0];
    uint16_t reg_addr = 0;

    if (ctx->lookup_by_tag) {
        const modbus_map_t *map = (const modbus_map_t *)ctx->lookup_by_tag(tag_id);
        if (map)
            reg_addr = (uint16_t)map->key;
        else
            return -1;
    } else
        return -1;

    priv->last_requested_tag = tag_id;
    uint8_t *p = net_buf_add(out, 8);
    p[0] = priv->slave_id;
    p[1] = 0x03;
    p[2] = (uint8_t)(reg_addr >> 8);
    p[3] = (uint8_t)(reg_addr & 0xFF);
    p[4] = 0x00;
    p[5] = 0x01;
    uint16_t crc = _calc_crc(p, 6);
    p[6] = (uint8_t)(crc & 0xFF);
    p[7] = (uint8_t)(crc >> 8);
    return 0;
}

static int modbus_v3_encode_write(edge_driver_ctx_t *ctx, uint32_t tag_id, double value,
                                  net_buf_t *out) {
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
    uint16_t reg_addr = 0;
    float scale = 1.0f;

    if (ctx->lookup_by_tag) {
        const modbus_map_t *map = (const modbus_map_t *)ctx->lookup_by_tag(tag_id);
        if (map) {
            reg_addr = (uint16_t)map->key;
            scale = map->scaler;
        } else
            return -1;
    } else
        return -1;

    uint16_t raw_val = (uint16_t)(value / (double)scale);
    priv->last_requested_tag = tag_id;

    uint8_t *p = net_buf_add(out, 8);
    p[0] = priv->slave_id;
    p[1] = 0x06;  // Write Single Register
    p[2] = (uint8_t)(reg_addr >> 8);
    p[3] = (uint8_t)(reg_addr & 0xFF);
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
    if (data[0] != priv->slave_id) return -1;

    size_t expected = 0;
    uint8_t func = data[1];

    if (func == 0x03) {
        // Read Holding Registers Response: SlaveID(1) + Func(1) + ByteCount(1) + Data(2*N) + CRC(2)

        // Since we only request 1 register (N=1), ByteCount must be 2.

        if (in->b.len < 3) return EDGE_ERR_INCOMPLETE;

        if (data[2] == 0x00 && in->b.len >= 8) {
            // [Omnis Special] This is almost certainly a mirrored REQUEST, not a RESPONSE.

            // In a request for addr 0, data[2] is the high byte of addr (0).

            // In a response for 1 register, data[2] is ByteCount (2).

            *consumed = 8;

            return EDGE_ERR_INCOMPLETE;
        }

        if (data[2] != 2) {
            // Malformed or different register count response

            return -1;
        }

        expected = 3 + 2 + 2;  // 7 bytes

    } else if (func == 0x06) {
        // Write Single Register Response: mirrors the request (8 bytes)

        expected = 8;

    } else if (func == 0x83) {
        // Exception Response: SlaveID(1) + ErrorFunc(1) + ErrorCode(1) + CRC(2)
        expected = 5;
    } else {
        // Unsupported function code or garbage
        return -1;
    }

    if (in->b.len < expected) return EDGE_ERR_INCOMPLETE;
    *consumed = expected;

    uint16_t crc_calc = _calc_crc(data, expected - 2);
    uint16_t crc_rx = (uint16_t)(data[expected - 2] | (data[expected - 1] << 8));
    if (crc_calc != crc_rx) {
        printf("[MODBUS-ERR] CRC Fail! Calc=0x%04X Rx=0x%04X\n", crc_calc, crc_rx);
        return -1;
    }

    if (func == 0x03) {
        uint16_t val = (uint16_t)((data[3] << 8) | data[4]);
        float scale = 1.0f;
        if (ctx->lookup_by_tag) {
            const modbus_map_t *map =
                (const modbus_map_t *)ctx->lookup_by_tag(priv->last_requested_tag);
            if (map) scale = map->scaler;
        }
        double final_val = (double)val * (double)scale;

        // [Trace] 关键节点日志
        printf("\033[96m[MODBUS] 📥 RX: Tag=%u Raw=0x%04X Val=%.2f\033[0m\n",
               priv->last_requested_tag, val, final_val);

        pipe_msg_init_scalar(&out_msgs[0], priv->last_requested_tag, final_val, uv_hrtime());
        *msg_count = 1;
    } else {
        printf("[MODBUS-ERR] Exception Response: 0x%02X, ErrorCode: 0x%02X\n", func, data[2]);
    }
    return 0;
}

static int modbus_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                               pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    (void)out_msgs;
    (void)msg_count;
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;

    if (in->b.len < 8) return EDGE_ERR_INCOMPLETE;

    uint8_t *data = in->b.data;

    // [Debug] 使用统一日志引擎打印所有收到的潜在报文

    LOG_INFO(LOG_CAT_BUS,
             "[MODBUS-RX-RAW] ID:%02X (Mine:%02X) data: %02X %02X %02X %02X %02X %02X %02X %02X",

             data[0], priv->slave_id, data[0], data[1], data[2], data[3], data[4], data[5], data[6],
             data[7]);

    // 1. 验证 Slave ID
    if (data[0] != priv->slave_id) {
        // [Nexus] 不是发给我的，返回错误但不设置 consumed，让 engine 决定是否丢弃头部
        return -1;
    }

    // 2. 校验 CRC
    uint16_t crc_rx = (uint16_t)(data[6] | (data[7] << 8));
    uint16_t crc_calc = _calc_crc(data, 6);
    if (crc_calc != crc_rx) {
        LOG_WARN(LOG_CAT_BUS, "MODBUS-SLAVE: CRC Error (Rx:0x%04X, Calc:0x%04X)", crc_rx, crc_calc);
        return -1;
    }

    *consumed = 8;
    uint8_t func = data[1];
    uint16_t addr = (uint16_t)((data[2] << 8) | data[3]);
    uint16_t count = (uint16_t)((data[4] << 8) | data[5]);

    printf(
        "\033[35m[MODBUS-SLAVE] Request Validated: Func=%d, Addr=%u, Count=%d "
        "(SlaveID:%d)\033[0m\n",
        func, addr, count, data[0]);

    if (func == 0x03) {
        // Read Holding Registers
        if (count > 32) {
            LOG_WARN(LOG_CAT_BUS, "MODBUS-SLAVE: Count %d too large", count);
            return -1;
        }

        uint8_t *p = net_buf_add(resp_out, 3 + (size_t)count * 2 + 2);
        p[0] = priv->slave_id;
        p[1] = 0x03;
        p[2] = (uint8_t)(count * 2);

        for (uint16_t i = 0; i < count; i++) {
            uint32_t tag_id = 0;
            float scale = 1.0f;
            if (ctx->lookup_by_key) {
                const modbus_map_t *map = (const modbus_map_t *)ctx->lookup_by_key(addr + i);
                if (map) {
                    tag_id = map->tag_id;
                    scale = map->scaler;
                }
            }

            uint16_t reg_val = 0;
            if (tag_id > 0 && ctx->user_data) {
                datahub_t *hub = (datahub_t *)ctx->user_data;
                pipe_msg_t shadow;
                if (datahub_read_shadow(hub, tag_id, &shadow) == 0) {
                    double val = (shadow.type == PIPEMSG_TYPE_SCALAR)
                                     ? shadow.payload.val.f
                                     : (double)shadow.payload.val.i;
                    reg_val = (uint16_t)(val / (double)scale);
                    LOG_INFO(LOG_CAT_BUS, "[MODBUS-SLAVE] Mirroring Tag %u -> %.2f (Reg %u: %u)",
                             tag_id, val, addr + i, reg_val);
                } else {
                    LOG_INFO(LOG_CAT_BUS, "[MODBUS-SLAVE] Tag %u not in shadow table yet", tag_id);
                }
            } else if (tag_id == 0) {
                LOG_INFO(LOG_CAT_BUS, "[MODBUS-SLAVE] Address %u NOT mapped in contract", addr + i);
            }

            p[3 + i * 2] = (uint8_t)(reg_val >> 8);
            p[4 + i * 2] = (uint8_t)(reg_val & 0xFF);
        }

        uint16_t crc_tx = _calc_crc(p, 3 + (size_t)count * 2);
        p[3 + count * 2] = (uint8_t)(crc_tx & 0xFF);
        p[4 + count * 2] = (uint8_t)(crc_tx >> 8);

        LOG_INFO(LOG_CAT_BUS, "[MODBUS-SLAVE] Replied %d registers starting from %u", count, addr);
        return 0;
    }

    LOG_WARN(LOG_CAT_BUS, "MODBUS-SLAVE: Unsupported function %d", func);
    return -1;
}

edge_driver_t modbus_rtu_driver = {.name = "ModbusRTU",
                                   .ctx_size = sizeof(modbus_ctx_t),
                                   .on_init = modbus_v3_on_init,
                                   .on_cleanup = NULL,
                                   .encode_request = modbus_v3_encode_request,
                                   .encode_write = modbus_v3_encode_write,
                                   .decode_response = modbus_v3_decode_response,
                                   .handle_rx = modbus_v3_handle_rx};

int modbus_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &modbus_rtu_driver);
}