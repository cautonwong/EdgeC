#ifndef EDGE_DRIVER_H
#define EDGE_DRIVER_H

#include <stddef.h>
#include <stdint.h>

#include "shmring.h"  // 确证 pipe_msg_t 可见

/**
 * @brief 驱动测点物理与逻辑配置 (核心结构)
 * 必须包含 profile_loader.c 需要的所有字段
 */
typedef struct {
    uint32_t tag_id;     // 逻辑 ID
    uint32_t reg_addr;   // 物理地址 (如 Modbus 寄存器)
    uint16_t reg_count;  // 寄存器数量
    uint8_t func_code;   // 功能码
    double scale;        // 缩放系数
    double offset;       // 偏移量
} edge_point_config_t;

/**
 * @brief 轮询分组配置 (高性能 V2)
 */
typedef struct {
    uint16_t start_addr;
    uint16_t count;
    uint8_t func_code;
    uint32_t slave_id;
} edge_poll_group_t;

/**
 * @brief 驱动程序统一接口
 */
struct edge_driver_s {
    const char *name;
    int (*init)(void *ctx);

    // 批处理 API (聚合轮询)
    int (*build_batch_req)(edge_poll_group_t *group, uint8_t *tx_buf, size_t *tx_len);
    int (*parse_batch_res)(edge_poll_group_t *group, uint8_t *rx_buf, size_t rx_len,
                           void (*on_msg)(pipe_msg_t *msg, void *user_data), void *user_data);

    // 单点 API (Legacy & 随机读写)
    int (*build_request)(uint32_t slave_id, edge_point_config_t *point, uint8_t *tx_buf,
                         size_t *tx_len);
    int (*parse_response)(edge_point_config_t *point, uint8_t *rx_buf, size_t rx_len,
                          pipe_msg_t *out_msg);

    // 从站/服务端 API (新增)
    int (*parse_request)(const uint8_t *rx_buf, size_t rx_len, uint32_t *out_slave_id,
                         uint16_t *out_di);
    int (*build_response)(uint32_t slave_id, uint8_t ctrl_code, uint32_t di, const pipe_msg_t *data,
                          uint8_t *tx_buf, size_t *tx_len);

    void (*cleanup)(void *ctx);
};

typedef struct edge_driver_s edge_driver_t;

#endif
