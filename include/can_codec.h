#ifndef EDGE_CAN_CODEC_H
#define EDGE_CAN_CODEC_H

#include <stdint.h>
#include "net_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 归一化 CAN 帧结构 (兼容 SocketCAN)
 */
typedef struct {
    uint32_t can_id;  /**< 32位标识符 (含 EFF/RTR/ERR 标志) */
    uint8_t  can_dlc; /**< 数据长度 (0..8) */
    uint8_t  data[8]; /**< 数据内容 */
} edge_can_frame_t;

/**
 * @brief 从 net_buf 中提取 CAN 帧
 */
int can_decode_frame(struct net_buf_simple *in, edge_can_frame_t *out);

/**
 * @brief 将 CAN 帧封装进 net_buf
 */
int can_encode_frame(const edge_can_frame_t *in, net_buf_t *out);

#ifdef __cplusplus
}
#endif

#endif
