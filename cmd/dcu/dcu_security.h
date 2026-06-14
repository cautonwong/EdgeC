#ifndef DCU_SECURITY_H
#define DCU_SECURITY_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief ESAM 临时鉴权会话状态结构体 (模拟 ESAM 状态机)
 */
typedef struct {
    uint8_t random_challenge[8];  // 主站下发的随机质询数
    bool is_authenticated;        // 当前是否已通过身份鉴权
    uint8_t session_key[16];      // 动态生成的会话密钥 (Session Key)
} dcu_esam_t;

/**
 * @brief 外设 RS485/UART 信道属性结构体 (OI F200 参数)
 */
typedef struct {
    uint32_t baudrate;  // 波特率 (例如 9600, 2400)
    uint8_t parity;     // 校验位 (0: 无校验, 1: 奇校验, 2: 偶校验)
} dcu_channel_cfg_t;

/**
 * @brief 初始化 ESAM 安全芯片状态
 */
void dcu_security_init(dcu_esam_t* esam);

/**
 * @brief 身份鉴权动作处理方法 1 (Authenticate)
 * 输入随机数，计算产生加密质询应答，并协商 Session Key
 * @param param_in 主站下发的随机质询数字节流 (8字节)
 * @param result_out 返回给主站的 ESAM 密文应答 (8字节)
 * @return 0 成功，负数失败
 */
int dcu_security_authenticate(dcu_esam_t* esam, const uint8_t* param_in, uint8_t* result_out);

/**
 * @brief 密文数据 MAC 签名校验方法 2 (Verify MAC)
 * 验证主站下发报文的签名合法性，防篡改
 * @param data 报文数据
 * @param data_len 数据长度
 * @param mac 待验证的 4 字节 MAC
 * @return true 验证通过，false 签名非法
 */
bool dcu_security_verify_mac(dcu_esam_t* esam, const uint8_t* data, uint32_t data_len,
                             const uint8_t* mac);

#endif  // DCU_SECURITY_H
