#include <stdio.h>
#include <string.h>
#include <time.h>

#include "contract_data_types.h"
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
    float scale = 1.0f;

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

static int modbus_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                     pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    (void)resp_out;
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
    if (in->b.len < 5) return EDGE_ERR_INCOMPLETE;
    uint8_t *data = in->b.data;
    if (data[0] != priv->slave_id) return -1;

    size_t expected = 3 + (size_t)data[2] + 2;
    if (in->b.len < expected) return EDGE_ERR_INCOMPLETE;
    *consumed = expected;

    uint16_t crc_calc = _calc_crc(data, expected - 2);
    uint16_t crc_rx = (uint16_t)(data[expected - 2] | (data[expected - 1] << 8));
    if (crc_calc != crc_rx) return -1;

    if (data[1] == 0x03) {
        uint16_t val = (uint16_t)((data[3] << 8) | data[4]);
        float scale = 1.0f;
        if (ctx->lookup_by_tag) {
            const modbus_map_t *map =
                (const modbus_map_t *)ctx->lookup_by_tag(priv->last_requested_tag);
            if (map) scale = map->scaler;
        }
        pipe_msg_init_scalar(&out_msgs[0], priv->last_requested_tag, (double)val * (double)scale,
                             uv_hrtime());
        *msg_count = 1;
        // printf(" [Modbus] RX Tag %u = %.2f\n", priv->last_requested_tag, (double)val *
        // (double)scale);
        return 0;
    }
    return 0;
}

edge_driver_t modbus_rtu_driver = {.name = "ModbusRTU",
                                   .ctx_size = sizeof(modbus_ctx_t),
                                   .on_init = modbus_v3_on_init,
                                   .on_cleanup = NULL,
                                   .encode_request = modbus_v3_encode_request,
                                   .decode_response = modbus_v3_decode_response,
                                   .handle_rx = NULL};

int modbus_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &modbus_rtu_driver);
}