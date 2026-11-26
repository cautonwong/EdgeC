#ifndef EDGE_MODBUS_CODEC_H
#define EDGE_MODBUS_CODEC_H

#include <stdint.h>
#include <stdbool.h>
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

/* --- 写请求的点位描述 --- */

/**
 * @brief 写值的类型（点位模型的知识，由消费方填写，驱动不猜）。
 *
 * 同一个寄存器在不同设备上可能是 u16、i16、f32 或 u64；类型决定写入宽度
 * （1/2/4 个寄存器）。读请求忽略本描述，与 DLMS 的约定一致。
 */
typedef enum {
    MODBUS_WRITE_U16 = 0, /**< 1 寄存器，无符号 */
    MODBUS_WRITE_I16,     /**< 1 寄存器，有符号（二进制补码） */
    MODBUS_WRITE_U32,     /**< 2 寄存器，无符号 */
    MODBUS_WRITE_I32,     /**< 2 寄存器，有符号 */
    MODBUS_WRITE_F32,     /**< 2 寄存器，IEEE-754 单精度 */
    MODBUS_WRITE_U64,     /**< 4 寄存器，无符号 */
    MODBUS_WRITE_I64,     /**< 4 寄存器，有符号 */
    MODBUS_WRITE_F64,     /**< 4 寄存器，IEEE-754 双精度 */
} modbus_write_type_t;

/**
 * @brief 写入值的字序：名字就是值字节 A(MSB)..D(H) 在线上的顺序。
 *
 *   ABCD = 线上原序；CDAB = 16 位字序反转；BADC = 每个字内字节交换；
 *   DCBA = 字序反转 + 每个字内字节交换。
 * 注意：iems_v3 `port_transf.c` 的 ORDER_* 名字按寄存器值语义命名，实际
 * wire 字节与名字交叉（v3 ORDER_ABCD 发出的字节是 EdgeC 的 BADC）。
 * 模板映射必须按 v3 实际 wire 对照，不能按名字直连。
 * 32 位示例（值 0x12345678）：ABCD [0x1234,0x5678]，
 * CDAB [0x5678,0x1234]，BADC [0x3412,0x7856]，DCBA [0x7856,0x3412]。
 */
typedef enum {
    MODBUS_WORD_ORDER_ABCD = 0, /**< 高字节先（标准） */
    MODBUS_WORD_ORDER_CDAB,     /**< 16 位字序反转 */
    MODBUS_WORD_ORDER_BADC,     /**< 每个 16 位字内字节交换 */
    MODBUS_WORD_ORDER_DCBA,     /**< 字序反转 + 每个字内字节交换 */
} modbus_word_order_t;

/**
 * @brief 写请求的点位描述（随 edge_api_write_tag_ex 的 point_desc 传入）。
 *
 * 缺描述时保持旧默认：1 个寄存器按 u16、2 个寄存器按 u32 ABCD；其他数量拒绝。
 * 描述存在时 `point_desc_len` 必须等于 sizeof(modbus_write_desc_t)，类型宽度
 * 必须等于 native_id 里的寄存器数量（quantity），否则拒绝。
 *
 * 放在公开头是因为构造它的是消费方（产品侧），驱动只按同一布局读取。
 */
typedef struct {
    uint8_t type;  /**< modbus_write_type_t */
    uint8_t order; /**< modbus_word_order_t */
} modbus_write_desc_t;

/**
 * @brief 按写描述把 double 编码成寄存器序列。
 *
 * 整数类型先做 double 区间检查再向零截断（与既有默认和 v3 的隐式转换一致）；
 * 非有限值、超出类型范围、未知类型/字序、类型宽度与 quantity 不符都返回 -1。
 * `desc == NULL` 表示旧默认（quantity 1 → u16，2 → u32 ABCD，其余拒绝）。
 *
 * @param value    写入值（raw，未标定）
 * @param desc     写描述；NULL 走旧默认
 * @param quantity 本次写入的寄存器数量（native_id 的 quantity，0 已归一为 1）
 * @param regs     输出寄存器序列，至少 `quantity` 个
 * @return 0 成功；-1 参数或值非法
 */
int modbus_encode_write_regs(double value, const modbus_write_desc_t *desc, uint16_t quantity,
                             uint16_t *regs);

/* --- CRC16 Modbus --- */
uint16_t modbus_calculate_crc(const uint8_t *buf, size_t len);

/* --- 封包逻辑 (RTU) --- */

int modbus_rtu_encode_read(uint8_t slave_id, uint8_t fc, uint16_t addr, uint16_t quantity, net_buf_t *out);
int modbus_rtu_encode_write_single(uint8_t slave_id, uint16_t addr, uint16_t value, net_buf_t *out);
int modbus_rtu_encode_write_multiple(uint8_t slave_id, uint16_t addr, uint16_t quantity,
                                     const uint16_t *values, net_buf_t *out);

/* CRC 字节序变体：hi_crc = true 时高字节先发。iems_v3 的默认 modbus_rtu
 * 就是这个顺序（src/libmodbus/modbus-rtu.c:156-170），标准 Modbus 是低字节先发。 */
int modbus_rtu_encode_read_ex(uint8_t slave_id, uint8_t fc, uint16_t addr, uint16_t quantity,
                              bool hi_crc, net_buf_t *out);
int modbus_rtu_encode_write_single_ex(uint8_t slave_id, uint16_t addr, uint16_t value,
                                      bool hi_crc, net_buf_t *out);
int modbus_rtu_encode_write_multiple_ex(uint8_t slave_id, uint16_t addr, uint16_t quantity,
                                        const uint16_t *values, bool hi_crc, net_buf_t *out);
int modbus_rtu_encode_exception_ex(uint8_t slave_id, uint8_t fc, uint8_t ex_code, bool hi_crc,
                                   net_buf_t *out);
int modbus_rtu_encode_exception(uint8_t slave_id, uint8_t fc, uint8_t ex_code, net_buf_t *out);

/* --- 解析逻辑 (RTU) --- */

/**
 * @brief 解析 RTU 帧并验证 CRC
 * @return 0 成功, 1 数据不足, -1 CRC 错误, -2 格式错误
 */
int modbus_rtu_decode_frame(struct net_buf_simple *in, uint8_t *slave_id, uint8_t *fc, 
                            uint8_t **data_out, size_t *data_len);

/* 主站应答解码的 CRC 字节序变体，见上。 */
int modbus_rtu_decode_frame_ex(struct net_buf_simple *in, bool hi_crc, uint8_t *slave_id,
                               uint8_t *fc, uint8_t **data_out, size_t *data_len);

/**
 * @brief 解析 RTU 请求帧并验证 CRC (从站使用)
 * @return 0 成功, 1 数据不足, -1 CRC 错误, -2 格式错误
 */
/* 从站请求解码的 CRC 字节序变体，见上。 */
int modbus_rtu_decode_req_frame_ex(struct net_buf_simple *in, bool hi_crc, uint8_t *slave_id,
                                   uint8_t *fc, uint8_t **data_out, size_t *data_len);
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
int modbus_tcp_encode_write_multiple(uint16_t tid, uint8_t slave_id, uint16_t addr, uint16_t quantity,
                                     const uint16_t *values, net_buf_t *out);
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
