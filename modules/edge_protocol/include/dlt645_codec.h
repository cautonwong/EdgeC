#ifndef EDGE_DLT645_CODEC_H
#define EDGE_DLT645_CODEC_H

#include <stdint.h>
#include "net_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DLT645 报文控制码定义
 */
#define DLT645_CTRL_READ_DATA           0x11
#define DLT645_CTRL_READ_RESPONSE       0x91
#define DLT645_CTRL_WRITE_DATA          0x14
#define DLT645_CTRL_WRITE_RESPONSE      0x94
#define DLT645_CTRL_READ_ADDR           0x13
#define DLT645_CTRL_ERROR_RESPONSE      0xD1
/* 跳合闸/报警/保电（标准 7.13）。请求 1CH、正常应答 9CH、异常应答 DCH。 */
#define DLT645_CTRL_TRIP_CONTROL        0x1C
#define DLT645_CTRL_TRIP_RESPONSE       0x9C
#define DLT645_CTRL_TRIP_ERROR          0xDC
/* 数据域 N1：控制命令类型（标准 7.13 注 1） */
#define DLT645_CMD_TRIP                 0x1A /* 跳闸 */
#define DLT645_CMD_ALLOW_CLOSE          0x1B /* 合闸允许 */
#define DLT645_CMD_ALARM                0x2A /* 报警 */
#define DLT645_CMD_ALARM_CLEAR          0x2B /* 报警解除 */
#define DLT645_CMD_KEEP_POWER           0x3A /* 保电 */
#define DLT645_CMD_KEEP_CLEAR           0x3B /* 保电解除 */
/* 密码首字节是密级（标准 7.13 注 3）：02=不带安全认证（N1..Nm 明文），98=带安全认证 */
#define DLT645_PWD_LEVEL_PLAIN          0x02

/**
 * @brief DLT645 纯协议内核接口 (Zero-Dependency)
 */

/* --- 基础工具 --- */
uint8_t dlt645_calculate_cs(const uint8_t *buf, size_t len);

/**
 * @brief 将 64位地址转换为 6字节 BCD (Little Endian in 645)
 */
void dlt645_u64_to_bcd_addr(uint64_t addr, uint8_t *out_bcd);

/**
 * @brief BCD 转 浮点 (处理 0x33 偏移后的原始值)
 */
double dlt645_bcd_to_f64(const uint8_t *bcd, int bytes, int decimals);

/* --- 编解码逻辑 --- */

/**
 * @brief 构造 DLT645 请求帧 (自动处理 FEFE 前导符和 0x33 偏移)
 * @param addr_bcd 6字节从站地址
 * @param ctrl 控制码
 * @param di 数据标识 (4字节)
 * @param data 附加数据
 * @param data_len 附加数据长度
 * @param out 输出缓冲区
 */
int dlt645_encode_frame(const uint8_t *addr_bcd, uint8_t ctrl, uint32_t di, 
                        const uint8_t *data, size_t data_len, net_buf_t *out);

/**
 * @brief 构造跳合闸/报警/保电请求帧（控制码 1CH，标准 7.13）
 *
 * 帧：`68H A0…A5 68H 1CH L PA P0P1P2 C0…C3 N1…Nm CS 16H`，
 * 其中 `L = 8 + m`（密码 4 + 操作者代码 4 + 命令数据 m），数据域每字节 +33H。
 *
 * 为什么单独一个函数而不是复用 `dlt645_encode_frame`：那条路强制带 4 字节数据标识
 * （DI），而控制命令帧**没有 DI** —— 硬塞进去会让从站把 DI 当成密码的一部分。
 *
 * @param addr_bcd       6 字节 BCD 表地址
 * @param cmd_type       N1：DLT645_CMD_TRIP / _ALLOW_CLOSE / _ALARM / ...
 * @param deadline_bcd   N3…N8 命令有效截止时间，6 字节 BCD：ss mm hh DD MM YY。
 *                       传 NULL 表示不设截止时间（N3…N8 填 0）
 * @param password       4 字节密码，首字节是密级（DLT645_PWD_LEVEL_PLAIN=02 明文）
 * @param operator_code  4 字节操作者代码（标准要求从站记录是谁操作的）
 * @return 0 成功；<0 参数不合法
 */
int dlt645_build_ctrl_request(const uint8_t *addr_bcd, uint8_t cmd_type,
                              const uint8_t deadline_bcd[6], const uint8_t password[4],
                              const uint8_t operator_code[4], net_buf_t *out);

/**
 * @brief 解析 DLT645 响应帧
 * @param in 输入缓冲区
 * @param addr_bcd 输出解析到的地址
 * @param ctrl 输出解析到的控制码
 * @param di 输出解析到的数据标识
 * @param payload_out 输出数据区指针
 * @param payload_len 输出数据区长度
 * @return 0 成功, 1 数据不足, -1 校验错误, -2 格式错误
 */
int dlt645_decode_frame(struct net_buf_simple *in, uint8_t *addr_bcd, uint8_t *ctrl,
                        uint32_t *di, uint8_t **payload_out, size_t *payload_len);

#ifdef __cplusplus
}
#endif

#endif  // EDGE_DLT645_CODEC_H
