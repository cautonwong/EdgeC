#ifndef EDGE_DRIVER_H
#define EDGE_DRIVER_H

#include <stdint.h>
#include "io_executor.h"
#include "shmring.h"

// 测点元数据 (Profile)
typedef struct {
    uint32_t tag_id;
    uint16_t reg_addr;
    uint16_t reg_count;
    double scale;
    double offset;
} edge_point_config_t;

// 驱动程序标准接口 (面向对象 C)
typedef struct edge_driver_s {
    const char *name;
    int (*init)(void *driver_ctx);
    
    // 核心职责：将业务 Job 翻译为物理字节流
    int (*build_request)(uint32_t slave_id, edge_point_config_t *point, uint8_t *tx_buf, size_t *tx_len);
    
    // 核心职责：将物理字节流解析为业务数据，并写入 PipeMsg
    int (*parse_response)(edge_point_config_t *point, uint8_t *rx_buf, size_t rx_len, PipeMsg *out_msg);
    
    void (*cleanup)(void *driver_ctx);
} edge_driver_t;

#endif
