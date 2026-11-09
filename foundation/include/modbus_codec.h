#ifndef EDGE_MODBUS_CODEC_H
#define EDGE_MODBUS_CODEC_H

#include <stdint.h>
#include "net_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Modbus Function Codes --- */
#define MODBUS_FC_READ_COILS            0x01
#define MODBUS_FC_READ_DISCRETE_INPUTS  0x02
#define MODBUS_FC_READ_HOLDING_REGS     0x03
#define MODBUS_FC_READ_INPUT_REGS       0x04
#define MODBUS_FC_WRITE_SINGLE_COIL      0x05
#define MODBUS_FC_WRITE_SINGLE_REG       0x06
#define MODBUS_FC_WRITE_MULTIPLE_COILS   0x0F
#define MODBUS_FC_WRITE_MULTIPLE_REGS    0x10

/* --- Modbus Exception Codes --- */
#define MODBUS_EX_ILLEGAL_FUNCTION      0x01
#define MODBUS_EX_ILLEGAL_DATA_ADDRESS   0x02
#define MODBUS_EX_ILLEGAL_DATA_VALUE     0x03
#define MODBUS_EX_SERVER_DEVICE_FAILURE  0x04

/**
 * @brief Modbus 纯协议内核接口 (Zero-Dependency)
 */

/* --- CRC16 Modbus --- */
uint16_t modbus_calculate_crc(const uint8_t *buf, size_t len);

/* --- 封包逻辑 (RTU) --- */

int modbus_rtu_encode_read(uint8_t slave_id, uint8_t fc, uint16_t addr, uint16_t quantity, net_buf_t *out);
int modbus_rtu_encode_write_single(uint8_t slave_id, uint16_t addr, uint16_t value, net_buf_t *out);
int modbus_rtu_encode_exception(uint8_t slave_id, uint8_t fc, uint8_t ex_code, net_buf_t *out);

/* --- 解析逻辑 (RTU) --- */

/**
 * @brief 解析 RTU 帧并验证 CRC
 * @return 0 成功, 1 数据不足, -1 CRC 错误, -2 格式错误
 */
int modbus_rtu_decode_frame(struct net_buf_simple *in, uint8_t *slave_id, uint8_t *fc, 
                            uint8_t **data_out, size_t *data_len);

/**
 * @brief 解析 RTU 请求帧并验证 CRC (从站使用)
 * @return 0 成功, 1 数据不足, -1 CRC 错误, -2 格式错误
 */
int modbus_rtu_decode_req_frame(struct net_buf_simple *in, uint8_t *slave_id, uint8_t *fc, 
                                uint8_t **data_out, size_t *data_len);

/* --- Modbus TCP / MBAP --- */

/* [ADR-002] 驱动认识的点位标识：低 16 位地址 + 高 16 位寄存器个数。
 * 产品侧用这个宏编码后传给 edge_api_read_tag_ex 的 native_id；
 * 驱动只解这两个数，不知道 TagID 是什么。 */
#define MODBUS_NATIVE(addr, quantity) \
    ((((uint32_t)(quantity) & 0xFFFFu) << 16) | ((uint32_t)(addr) & 0xFFFFu))
#define MODBUS_NATIVE_ADDR(id)   ((uint16_t)((id) & 0xFFFFu))
#define MODBUS_NATIVE_QTY(id)    ((uint16_t)(((id) >> 16) & 0xFFFFu))

#define MODBUS_TCP_HEADER_LEN 7  /* Transaction ID(2) + Protocol ID(2) + Length(2) + Unit ID(1) */
#define MODBUS_TCP_PROTOCOL_ID 0x0000

/**
 * @brief 构建 Modbus TCP MBAP 头 (不含 CRC, 由 TCP 保证)
 * @param tid 事务标识符 (Transaction ID)
 * @param slave_id 单元标识符 (Unit ID)
 * @param pdu_len PDU 长度 (功能码 + 数据)
 */
int modbus_tcp_encode_mbap(uint16_t tid, uint8_t slave_id, uint16_t pdu_len, net_buf_t *out);

/**
 * @brief 解析 Modbus TCP MBAP 头
 * @return 0 成功, 1 数据不足, -1 格式错
 */
int modbus_tcp_decode_mbap(struct net_buf_simple *in, uint16_t *tid, uint8_t *slave_id, size_t *pdu_len);

/**
 * @brief Modbus TCP 响应编码 (MBAP + PDU, 无 CRC)
 */
int modbus_tcp_encode_read(uint16_t tid, uint8_t slave_id, uint8_t fc, uint16_t addr, uint16_t quantity, net_buf_t *out);
int modbus_tcp_encode_write_single(uint16_t tid, uint8_t slave_id, uint16_t addr, uint16_t value, net_buf_t *out);
int modbus_tcp_encode_exception(uint16_t tid, uint8_t slave_id, uint8_t fc, uint8_t ex_code, net_buf_t *out);

/**
 * @brief Modbus TCP 响应编码 (仅用于从站应答)
 */
int modbus_tcp_encode_read_resp(uint16_t tid, uint8_t slave_id, const uint8_t *data, size_t data_len, net_buf_t *out);
int modbus_tcp_encode_write_resp(uint16_t tid, uint8_t slave_id, uint16_t addr, uint16_t value, net_buf_t *out);

#ifdef __cplusplus
}
#endif

#endif  // EDGE_MODBUS_CODEC_H
