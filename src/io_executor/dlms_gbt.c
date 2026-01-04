/**
 * @file dlms_gbt.c
 * @brief DLMS General Block Transfer (GBT) 引擎 - 支持滑动窗口流控
 */

#include "dlms_cosem.h"
#include "edge_log.h"
#include <string.h>

typedef struct {
    uint32_t block_index;
    uint8_t window_size;
    bool is_last;
} dlms_gbt_ctx_t;

/**
 * @brief 构建 GBT 确认帧
 */
int dlms_gbt_build_ack(dlms_gbt_ctx_t *gbt, net_buf_t *out) {
    struct net_buf_simple *b = &out->b;
    net_buf_simple_add_u8(b, 0xE6); // GBT Service Tag
    net_buf_simple_add_u8(b, 0x01); // ACK subtype
    net_buf_simple_add_be32(b, gbt->block_index);
    return 0;
}

/**
 * @brief 处理接收到的 GBT 数据块
 */
int dlms_gbt_handle_block(dlms_gbt_ctx_t *gbt, struct net_buf_simple *block_data) {
    uint32_t received_idx = net_buf_simple_pull_be32(block_data);
    
    if (received_idx != gbt->block_index + 1) {
        LOG_WARN(LOG_CAT_IO, "GBT: Out of sequence block %u", received_idx);
        return -1;
    }
    
    gbt->block_index = received_idx;
    // 逻辑：如果收到窗口上限，则触发 ACK 任务
    return 0;
}
