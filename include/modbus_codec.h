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

#ifdef __cplusplus
}
#endif

#endif  // EDGE_MODBUS_CODEC_H
