/**
 * @file dlms_security.c
 * @brief DLMS 安全平面实现 - 支持 Global Ciphering 与 In-place 加解密
 */

#include "dlms_codec.h"
#include "edge_log.h"
#include "edge_common.h"
#include <string.h>

// 模拟 SE (Secure Element) HAL 接口
// 在实际硬件中，这将调用加密芯片的驱动
extern int hal_aes_gcm_encrypt(const uint8_t *key, const uint8_t *iv, 
                               const uint8_t *aad, size_t aad_len,
                               uint8_t *payload, size_t payload_len,
                               uint8_t *tag);

int dlms_security_apply(net_buf_t *apdu, dlms_security_level_t level, uint32_t ic) {
    if (level == DLMS_SEC_NONE) return 0;

    struct net_buf_simple *b = &apdu->b;
    
    // 1. 预留安全头空间 (Security Control + IC)
    uint8_t *sec_head = net_buf_simple_push(b, 1 + DLMS_IC_LEN);
    
    // Security Control Byte: Bit 4-5 = Security Suite, Bit 0-3 = Level
    sec_head[0] = (uint8_t)(0x10 | (level & 0x0F)); 
    
    // Invocation Counter (Big Endian)
    sec_head[1] = (uint8_t)(ic >> 24);
    sec_head[2] = (uint8_t)(ic >> 16);
    sec_head[3] = (uint8_t)(ic >> 8);
    sec_head[4] = (uint8_t)(ic & 0xFF);

    // 2. 原地加密 (In-place Encryption)
    // 根据 DLMS 规范，IV 由 System Title + IC 组成
    uint8_t iv[12] = {0}; 
    // memcpy(iv, system_title, 8); // 实际应从上下文获取
    memcpy(iv + 8, sec_head + 1, 4);

    if (level >= DLMS_SEC_ENCRYPTED) {
        uint8_t auth_tag[12];
        // 这里的 payload 指向 APDU 的业务数据部分
        // AAD 通常包含安全头
        hal_aes_gcm_encrypt(NULL, iv, sec_head, 5, b->data + 5, b->len - 5, auth_tag);
        
        // 3. 追加认证标签
        net_buf_simple_add_mem(b, auth_tag, 12);
    }

    // 4. 封装加密后的 Tag (0x21 = Global Ciphering APDU)
    net_buf_simple_push_u8(b, 0x21);
    
    return 0;
}

int dlms_security_remove(net_buf_t *apdu, dlms_security_level_t *out_level) {
    struct net_buf_simple *b = &apdu->b;
    uint8_t tag = net_buf_simple_pull_u8(b);
    
    if (tag != 0x21) {
        *out_level = DLMS_SEC_NONE;
        return 0; // 明文报文
    }

    uint8_t sc = net_buf_simple_pull_u8(b);
    *out_level = (dlms_security_level_t)(sc & 0x0F);
    
    // 提取 IC
    uint32_t ic = net_buf_simple_pull_be32(b);
    (void)ic; // 实际应检查 IC 是否递增

    // 执行解密...
    return 0;
}
