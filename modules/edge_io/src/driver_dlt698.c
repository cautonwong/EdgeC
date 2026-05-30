/**
 * @file driver_dlt698.c
 * @brief 面向对象规约 DLT698.45 驱动 (Codec 分离 + 有状态会话)
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dlt698_codec.h"
#include "dlt645_codec.h"
#include "datahub.h"
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

    // 【核心修正】实事求是：拿着 tag_id (影子索引) 去契约里找回原始 OAD
    const dlt698_map_t *map = NULL; 
    if (ctx->lookup_by_tag) {
        map = (const dlt698_map_t *)ctx->lookup_by_tag(tag_ids[0]);
    }
    if (!map) {
        // 如果契约里没找到映射，说明这个索引是非法的。由于我们的 poller 动态生成了索引，
        // 我们在此必须有一个 Fallback：如果找不到，则尝试直接使用外部传进来的 OAD 提示
        // 在本项目的 DCU 场景下，我们信任 poller 计算出的索引在契约反查表的覆盖范围内。
        return -1;
    }

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

    if (svc == DLT698_SVC_GET_RES && payload_len >= 5) {
        // [Aetheris] 解析 OAD 后的数据 (使用通用递归解码引擎)
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
            
            if (tid != 0) {
                uint16_t offset = 4;
                dlt698_data_t res_data;
                if (dlt698_decode_data(payload, (uint16_t)payload_len, &offset, &res_data) == 0) {
                    double val = 0.0;
                    bool supported = true;

                    // 将抽象 A-XDR 数据类型转为 DataHub 标量
                    switch (res_data.tag) {
                        case DLT698_TAG_UNSIGNED: val = (double)res_data.val.u8; break;
                        case DLT698_TAG_LONG_UNSIGNED: val = (double)res_data.val.u16; break;
                        case DLT698_TAG_DOUBLE_LONG_UNSIGNED: val = (double)res_data.val.u32; break;
                        case DLT698_TAG_INTEGER: val = (double)res_data.val.i8; break;
                        case DLT698_TAG_LONG: val = (double)res_data.val.i16; break;
                        case DLT698_TAG_DOUBLE_LONG: val = (double)res_data.val.i32; break;
                        default: supported = false; break;
                    }

                    if (supported) {
                        pipe_msg_init_scalar(&out_msgs[0], tid, val * (double)scaler, uv_hrtime());
                        *msg_count = 1;
                        LOG_DEBUG(LOG_CAT_IO, "DLT698: Decoded OAD %08X (Tag %d) -> Val %.3f", oad_rx, res_data.tag, val * (double)scaler);
                    }
                    dlt698_free_data(&res_data);
                }
            }
        }
    }

    return 0;
}

static int dlt698_v4_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                             pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    (void)async_msgs; (void)async_count;
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
    *consumed = (size_t)(payload + payload_len + 3 - in->b.data);

    // 验证地址
    if (sa_rx.len != priv->sa.len || memcmp(sa_rx.addr, priv->sa.addr, sa_rx.len) != 0) {
        return -1; 
    }

    if (svc == DLT698_SVC_GET_REQ && payload_len >= 4) {
        uint32_t oad = (uint32_t)((payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3]);
        double val = 0;
        bool found = false;

        if (ctx->lookup_by_key) {
            const modbus_map_t *map = (const modbus_map_t *)ctx->lookup_by_key(oad);
            if (map && ctx->user_data) {
                pipe_msg_t shadow;
                if (datahub_read_shadow((datahub_t *)ctx->user_data, map->tag_id, &shadow) == 0) {
                    val = shadow.payload.val.f / (map->scaler ? map->scaler : 1.0);
                    found = true;
                }
            }
        }

        if (found) {
            // 简单实现：仅返回 2 字节 long-unsigned (uint16)
            uint16_t rv = (uint16_t)val;
            uint8_t data[2] = {(uint8_t)(rv >> 8), (uint8_t)(rv & 0xFF)};
            dlt698_encode_get_response(&priv->sa, piid, oad, data, 2, resp_out);
        } else {
            // 这里的错误响应逻辑可以后续补强
        }
        return 0;
    }

    return -1;
}

edge_driver_t dlt698_driver = {
    .name = "DLT698",
    .ctx_size = sizeof(dlt698_ctx_t),
    .on_init = dlt698_v4_on_init,
    .encode_request = dlt698_v4_encode_request,
    .decode_response = dlt698_v4_decode_response,
    .handle_rx = dlt698_v4_handle_rx,
};

int dlt698_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &dlt698_driver);
}
