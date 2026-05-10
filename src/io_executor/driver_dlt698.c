/**
 * @file driver_dlt698.c
 * @brief 面向对象规约 DLT698.45 驱动 (Codec 分离 + 有状态会话)
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dlt698_codec.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "net_buf.h"

typedef struct {
    dlt698_addr_t sa;
    uint8_t piid;
    uint32_t last_oad;
    bool connected;
} dlt698_ctx_t;

static int dlt698_v4_on_init(edge_driver_ctx_t *ctx) {
    dlt698_ctx_t *priv = (dlt698_ctx_t *)ctx->priv;
    memset(priv, 0, sizeof(dlt698_ctx_t));
    
    // 从 device_id 恢复 BCD 地址
    dlt645_u64_to_bcd_addr(ctx->device_id, priv->sa.addr);
    priv->sa.len = 6;
    priv->sa.type = 0; // Single
    priv->piid = 0;
    return 0;
}

static int dlt698_v4_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int count,
                                    net_buf_t *out) {
    dlt698_ctx_t *priv = (dlt698_ctx_t *)ctx->priv;
    if (count == 0) return -1;

    const modbus_map_t *map = NULL; // 698 也复用 key 结构存储 OAD
    if (ctx->lookup_by_tag) {
        map = (const modbus_map_t *)ctx->lookup_by_tag(tag_ids[0]);
    }
    if (!map) return -1;

    priv->last_oad = (uint32_t)map->key;
    return dlt698_encode_get_request(&priv->sa, ++priv->piid, priv->last_oad, out);
}

static int dlt698_v4_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                     pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    (void)resp_out;
    dlt698_ctx_t *priv = (dlt698_ctx_t *)ctx->priv;
    
    dlt698_addr_t sa_rx;
    uint8_t svc, piid;
    uint8_t *payload;
    size_t payload_len;

    int ret = dlt698_decode_frame(&in->b, &sa_rx, &svc, &piid, &payload, &payload_len);
    if (ret == 1) return EDGE_ERR_INCOMPLETE;
    if (ret < 0) {
        *consumed = 1;
        return -1;
    }

    // 计算消耗 (简单近似：到 payload + len + 2(FCS) + 1(16))
    *consumed = (size_t)(payload + payload_len + 3 - in->b.data);

    if (svc == DLT698_SVC_GET_RES && payload_len >= 4) {
        // [Aetheris] 解析 OAD 后的数据 (假设为简单数值类型)
        // 实际上 698 的数据解析非常复杂，涉及 Data 类型的嵌套递归
        // 此处先打通链路，后续可通过 dlms_decoder 风格重构 698_decoder
        uint32_t oad_rx = (uint32_t)((payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3]);
        if (oad_rx == priv->last_oad) {
            float scaler = 1.0f;
            uint32_t tid = 0;
            if (ctx->lookup_by_key) {
                const modbus_map_t *map = (const modbus_map_t *)ctx->lookup_by_key(oad_rx);
                if (map) {
                    tid = map->tag_id;
                    scaler = map->scaler;
                }
            }
            
            if (tid != 0 && payload_len >= 6) {
                // 简化：取 OAD 之后的前2字节作为数值 (如 long-unsigned)
                uint16_t raw_val = (uint16_t)((payload[4] << 8) | payload[5]);
                pipe_msg_init_scalar(&out_msgs[0], tid, (double)raw_val * scaler, uv_hrtime());
                *msg_count = 1;
            }
        }
    }

    return 0;
}

edge_driver_t dlt698_driver = {
    .name = "DLT698",
    .ctx_size = sizeof(dlt698_ctx_t),
    .on_init = dlt698_v4_on_init,
    .encode_request = dlt698_v4_encode_request,
    .decode_response = dlt698_v4_decode_response,
    .handle_rx = NULL,
};

int dlt698_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &dlt698_driver);
}
