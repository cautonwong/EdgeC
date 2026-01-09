#include <string.h>
#include <time.h>

#include "driver_manager.h"
#include "edge_common.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "net_buf.h"

// 引入生成的契约查找接口
extern uint32_t edge_lookup_modbus(uint32_t addr, float *out_scale);

typedef struct {
    uint8_t slave_id;
    uint32_t last_requested_tag;
    // 仿真用的本地寄存器
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
            } else {
                crc >>= 1;
            }
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

/* --- Master 模式: 编码请求 --- */
static int modbus_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int count,
                                    net_buf_t *out) {
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
    if (count == 0) return -1;

    uint32_t addr = tag_ids[0];
    priv->last_requested_tag = addr;

    uint8_t *p = net_buf_add(out, 8);
    p[0] = priv->slave_id;
    p[1] = 0x03;
    p[2] = (uint8_t)((addr >> 8) & 0xFF);
    p[3] = (uint8_t)(addr & 0xFF);
    p[4] = 0x00;
    p[5] = 0x01;  // Count=1

    uint16_t crc = _calc_crc(p, 6);
    p[6] = (uint8_t)(crc & 0xFF);
    p[7] = (uint8_t)(crc >> 8);

    return 0;
}

/* --- Master 模式: 解析响应 --- */
static int modbus_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                     pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    (void)resp_out;
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
    *consumed = 0;

    if (in->b.len < 5) return EDGE_ERR_INCOMPLETE;

    uint8_t *data = in->b.data;
    uint8_t fc = data[1];
    size_t expected_len = 0;

    if (fc == 0x03) {
        expected_len = 3 + (size_t)data[2] + 2;
    } else if (fc == 0x06 || fc == 0x10) {
        expected_len = 8;
    } else if (fc > 0x80) {
        expected_len = 5;  // Error response
    } else {
        return -1;  // Unsupported
    }

    if (in->b.len < expected_len) return EDGE_ERR_INCOMPLETE;
    *consumed = expected_len;

    uint16_t crc_calc = _calc_crc(data, expected_len - 2);
    uint16_t crc_rx = (uint16_t)(data[expected_len - 2] | (data[expected_len - 1] << 8));
    if (crc_calc != crc_rx) return -1;

    if (fc == 0x03) {
        uint16_t val = (uint16_t)((data[3] << 8) | data[4]);
        uint32_t tid = priv->last_requested_tag;
        if (tid == 0) tid = 1001;
        pipe_msg_init_scalar(&out_msgs[0], tid, (double)val, (uint64_t)time(NULL) * 1000);
        *msg_count = 1;
        return 0;
    }

    return 0;
}

/* --- Slave 模式: 处理接收并生成响应 --- */
static int modbus_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                               pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
    *consumed = 0;

    if (in->b.len < 8) return EDGE_ERR_INCOMPLETE;

    uint8_t *data = in->b.data;
    uint8_t fc = data[1];
    size_t expected_len = 0;

    if (fc == 0x03 || fc == 0x06) {
        expected_len = 8;
    } else if (fc == 0x10) {
        if (in->b.len < 7) return EDGE_ERR_INCOMPLETE;
        expected_len = 7 + (size_t)data[6] + 2;
    } else {
        return -1;
    }

    if (in->b.len < expected_len) return EDGE_ERR_INCOMPLETE;
    *consumed = expected_len;

    uint16_t crc_calc = _calc_crc(data, expected_len - 2);
    uint16_t crc_rx = (uint16_t)(data[expected_len - 2] | (data[expected_len - 1] << 8));
    if (crc_calc != crc_rx) return -1;

    uint8_t sid = data[0];
    if (sid != priv->slave_id && sid != 0x00) {
        *consumed = 0;  // 不匹配，不消耗，让下一个协议尝试
        return 1;
    }

    uint16_t addr = (uint16_t)((data[2] << 8) | data[3]);

    if (fc == 0x03) {
        uint8_t *res = net_buf_add(resp_out, 7);
        res[0] = priv->slave_id;
        res[1] = 0x03;
        res[2] = 2;
        uint16_t val = priv->local_regs[addr % 128];
        res[3] = (uint8_t)(val >> 8);
        res[4] = (uint8_t)(val & 0xFF);
        uint16_t crc = _calc_crc(res, 5);
        res[5] = (uint8_t)(crc & 0xFF);
        res[6] = (uint8_t)(crc >> 8);
    } else if (fc == 0x06) {
        uint16_t val = (uint16_t)((data[4] << 8) | data[5]);
        priv->local_regs[addr % 128] = val;
        float scale = 1.0f;
        uint32_t tag_id = edge_lookup_modbus(addr, &scale);
        if (tag_id != 0) {
            pipe_msg_init_scalar(&async_msgs[0], tag_id, (double)val * (double)scale,
                                 (uint64_t)time(NULL) * 1000);
            *async_count = 1;
        }
        net_buf_simple_add_mem(&resp_out->b, data, 8);
    }

    return 0;
}

edge_driver_t modbus_rtu_driver = {.name = "ModbusRTU",
                                   .ctx_size = sizeof(modbus_ctx_t),
                                   .on_init = modbus_v3_on_init,
                                   .on_cleanup = NULL,
                                   .encode_request = modbus_v3_encode_request,
                                   .decode_response = modbus_v3_decode_response,
                                   .handle_rx = modbus_v3_handle_rx};

int modbus_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &modbus_rtu_driver);
}