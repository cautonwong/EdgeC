#include <string.h>
#include <time.h>

#include "driver_manager.h"
#include "edge_common.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "modbus_codec.h"
#include "net_buf.h"

// 引入生成的契约查找接口
extern uint32_t edge_lookup_modbus(uint32_t addr, float *out_scale);

typedef struct {
    uint8_t slave_id;
    uint32_t last_requested_tag;
    // 仿真用的本地寄存器
    uint16_t local_regs[128];
} modbus_ctx_t;

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

    uint32_t addr = tag_ids[0];  // 简化：直接把 tag_id 当作地址用
    priv->last_requested_tag = addr;

    return modbus_encode_read_holding(priv->slave_id, (uint16_t)addr, 1, out);
}

/* --- Master 模式: 解析响应 --- */
static int modbus_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, pipe_msg_t *out_msgs,
                                     int *msg_count) {
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;

    // 使用协议内核计算校验 (保持零拷贝读取)
    if (in->b.len < 5) return 1;
    uint16_t crc_calc = modbus_calculate_crc(in->b.data, in->b.len - 2);
    uint16_t crc_rx = (uint16_t)(in->b.data[in->b.len - 2] | (in->b.data[in->b.len - 1] << 8));
    if (crc_calc != crc_rx) return -1;

    uint8_t fc = in->b.data[1];
    if (fc == 0x03) {
        uint8_t bytes = in->b.data[2];
        if (bytes + 5 != in->b.len) return -1;

        uint16_t val = (uint16_t)((in->b.data[3] << 8) | in->b.data[4]);

        uint32_t tid = priv->last_requested_tag;
        if (tid == 0) tid = 1001;

        pipe_msg_init_scalar(&out_msgs[0], tid, (double)val, (uint64_t)time(NULL) * 1000);
        *msg_count = 1;
        return 0;
    }

    return -1;
}

/* --- Slave 模式: 处理接收并生成响应 --- */
static int modbus_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                               pipe_msg_t *async_msgs, int *async_count) {
    modbus_ctx_t *priv = (modbus_ctx_t *)ctx->priv;
    if (in->b.len < 8) return -1;

    uint16_t crc_calc = modbus_calculate_crc(in->b.data, in->b.len - 2);
    uint16_t crc_rx = (uint16_t)(in->b.data[in->b.len - 2] | (in->b.data[in->b.len - 1] << 8));
    if (crc_calc != crc_rx) return -1;

    uint8_t sid = in->b.data[0];
    if (sid != priv->slave_id && sid != 0x00) return 1;

    uint8_t fc = in->b.data[1];
    uint16_t addr = (uint16_t)((in->b.data[2] << 8) | in->b.data[3]);

    if (fc == 0x03) {
        modbus_encode_read_holding(priv->slave_id, (uint16_t)addr, 1, resp_out);
    } else if (fc == 0x06) {
        uint16_t val = (uint16_t)((in->b.data[4] << 8) | in->b.data[5]);
        priv->local_regs[addr % 128] = val;

        float scale = 1.0f;
        uint32_t tag_id = edge_lookup_modbus(addr, &scale);
        if (tag_id != 0) {
            pipe_msg_init_scalar(&async_msgs[0], tag_id, (double)val * (double)scale,
                                 (uint64_t)time(NULL) * 1000);
            *async_count = 1;
        }
        net_buf_simple_add_mem(&resp_out->b, in->b.data, 8);
    }

    return 0;
}

edge_driver_t modbus_rtu_driver = {.name = "ModbusRTU",
                                   .ctx_size = sizeof(modbus_ctx_t),
                                   .on_init = modbus_v3_on_init,
                                   .encode_request = modbus_v3_encode_request,
                                   .decode_response = modbus_v3_decode_response,
                                   .handle_rx = modbus_v3_handle_rx};

int modbus_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &modbus_rtu_driver);
}
