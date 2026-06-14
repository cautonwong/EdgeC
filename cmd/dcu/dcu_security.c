#include "dcu_security.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief 初始化 ESAM 安全芯片状态
 * 电力终端上电后，ESAM 芯片会进行安全寄存器自检，并将鉴权状态复位为未认证
 */
void dcu_security_init(dcu_esam_t* esam) {
    if (!esam) return;

    memset(esam->random_challenge, 0, sizeof(esam->random_challenge));
    esam->is_authenticated = false;
    memset(esam->session_key, 0, sizeof(esam->session_key));
}

/**
 * @brief 身份鉴权动作处理方法 1 (Authenticate)
 * 【电力鉴权底层物理设计意图】：为了防止攻击者通过重放攻击或者伪造主站指令来下发非法拉闸控制，
 * 终端必须通过内置的国密/安全 ESAM 芯片与主站进行双向质询。
 * 主站下发一个 8 字节随机数，ESAM
 * 通过内部私钥加密生成密文响应，并利用该随机数协商出本次通信的动态会话密钥（Session Key）。
 */
int dcu_security_authenticate(dcu_esam_t* esam, const uint8_t* param_in, uint8_t* result_out) {
    if (!esam || !param_in || !result_out) return -1;

    // 1. 安全拷贝主站下发的 8 字节随机数质询
    memcpy(esam->random_challenge, param_in, 8);

    // 2. 模拟 ESAM 内部加密引擎计算（模拟国密算法，将随机数与终端固有的设备密钥混合异或）
    for (int i = 0; i < 8; i++) {
        result_out[i] = (uint8_t)(param_in[i] ^ 0xA5 ^ (uint8_t)i);
    }

    // 3. 动态计算本次 Session Key，用于后续报文的对称加解密与 MAC 校验
    for (int i = 0; i < 16; i++) {
        esam->session_key[i] = (uint8_t)((param_in[i % 8] ^ 0x5A) + (uint8_t)i);
    }

    // 4. 将鉴权标识置为有效，允许北向进行控制类动作（如拉闸）
    esam->is_authenticated = true;

    printf("🔐 [ESAM] 接收到随机质询！身份鉴权成功，动态会话密钥协商完成。\n");
    return 0;
}

/**
 * @brief 密文数据 MAC 签名校验方法 2 (Verify MAC)
 * 【电力防篡改设计意图】：主站在下发关键数据包时，必须附带一个由协商出的 Session Key 计算出的 4
 * 字节 MAC。 终端 ESAM 在物理底层对接收到的明文执行相同的 HMAC-like 运算，并与主站附带的 MAC
 * 进行逐字节强一致比对。 只有比对 100% 通过，才说明报文在信道传输中未被截获和篡改。
 */
bool dcu_security_verify_mac(dcu_esam_t* esam, const uint8_t* data, uint32_t data_len,
                             const uint8_t* mac) {
    if (!esam || !data || data_len == 0 || !mac) return false;

    // 只有已通过 Authenticate 身份鉴权的会话才允许进行 MAC 报文验证
    if (!esam->is_authenticated) {
        printf("⚠️ [ESAM] 鉴权状态未激活！拒绝执行 MAC 报文校验。\n");
        return false;
    }

    // 模拟 HMAC-like 4字节签名计算
    uint8_t local_mac[4] = {0};
    for (uint32_t i = 0; i < data_len; i++) {
        local_mac[i % 4] ^= (uint8_t)(data[i] ^ esam->session_key[i % 16]);
    }

    // 逐字节比对
    for (int i = 0; i < 4; i++) {
        if (local_mac[i] != mac[i]) {
            printf("❌ [ESAM] 报文签名校验失败！检测到非法包篡改，拒绝执行指令。\n");
            return false;
        }
    }

    printf("✅ [ESAM] 报文签名 MAC 验证成功，指令合法准予执行。\n");
    return true;
}
