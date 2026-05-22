/**
 * @file edge_guard.h
 * @brief Edge-Guard 工业协议防火墙与 DPI 校验引擎
 */

#ifndef EDGE_GUARD_H
#define EDGE_GUARD_H

#include <stdbool.h>
#include <stdint.h>
#include "datahub.h"
#include "edge_request.h"
#include "arbiter.h" // Add for cmd_msg_t

/**
 * @brief DPI 校验结果
 */
typedef enum {
    DPI_PASS = 0,
    DPI_BLOCK_RANGE = -1,         /**< 超出安全量程 */
    DPI_BLOCK_RATE = -2,          /**< 变化率异常 (逻辑炸弹) */
    DPI_BLOCK_MALFORMED = -3,     /**< 协议格式畸形 */
    DPI_BLOCK_UNAUTHORIZED = -4   /**< 未经授权的操作 */
} dpi_result_t;

/**
 * @brief 动态安全约束状态 (用于计算变化率)
 */
typedef struct {
    uint32_t tag_id;
    double last_value;
    uint64_t last_ts;
} dpi_state_t;

/**
 * @brief 初始化 DPI 校验引擎
 */
int dpi_engine_init(datahub_t *hub);

/**
 * @brief 校验写入指令 (Master 模式下发)
 */
dpi_result_t dpi_validate_command(cmd_msg_t *cmd);

/**
 * @brief 校验遥测响应 (Slave 模式上送)
 */
dpi_result_t dpi_validate_telemetry(uint32_t tag_id, double val);

/**
 * @brief 审计日志记录 (加密签名版)
 */
void dpi_audit_log(dpi_result_t res, uint32_t tag_id, double val, const char *reason);

#endif // EDGE_GUARD_H
