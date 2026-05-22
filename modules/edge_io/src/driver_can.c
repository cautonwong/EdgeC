/**
 * @file driver_can.c
 * @brief 工业级 CAN 规约驱动 (基于 SocketCAN 归一化)
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "can_codec.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "net_buf.h"

typedef struct {
    uint32_t last_can_id;
} can_ctx_t;

static int can_v4_on_init(edge_driver_ctx_t *ctx) {
    can_ctx_t *priv = calloc(1, sizeof(can_ctx_t));
    ctx->priv = priv;
    return 0;
}

static int can_v4_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int count,
                                  net_buf_t *out) {
    // 对于 CAN 来说，Request 通常是发送一个特定 ID 的帧
    if (count == 0) return -1;
    
    // [Nexus] 获取契约映射：TagID -> CAN ID
    const modbus_map_t *map = NULL; 
    if (ctx->lookup_by_tag) {
        map = (const modbus_map_t *)ctx->lookup_by_tag(tag_ids[0]);
    }
    if (!map) return -1;

    edge_can_frame_t frame = {
        .can_id = (uint32_t)map->key,
        .can_dlc = 0 // 远程请求帧 RTR
    };
    frame.can_id |= 0x40000000U; // CAN_RTR_FLAG
    
    return can_encode_frame(&frame, out);
}

static int can_v4_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                   pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    (void)resp_out;
    edge_can_frame_t frame;
    if (can_decode_frame(&in->b, &frame) != 0) {
        return EDGE_ERR_INCOMPLETE;
    }
    *consumed = 16;

    // [Aetheris] 根据 CAN ID 反查 TagID
    if (ctx->lookup_by_key) {
        const modbus_map_t *map = (const modbus_map_t *)ctx->lookup_by_key(frame.can_id & 0x1FFFFFFFU);
        if (map) {
            // 简单解析：取前 4 字节作为 float 或 int
            float val = 0;
            memcpy(&val, frame.data, 4);
            pipe_msg_init_scalar(&out_msgs[0], map->tag_id, (double)val * map->scaler, uv_hrtime());
            *msg_count = 1;
        }
    }
    return 0;
}

edge_driver_t can_driver = {
    .name = "CAN",
    .ctx_size = sizeof(can_ctx_t),
    .on_init = can_v4_on_init,
    .encode_request = can_v4_encode_request,
    .decode_response = can_v4_decode_response,
    .handle_rx = can_v4_decode_response, // CAN 的响应和自发数据解析逻辑一致
};

int can_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &can_driver);
}
