/**
 * @file dlms_security.c
 * @brief DLMS 安全平面实现 - 基于 hal_crypto 抽象层
 */

#include <string.h>

#include "dlms_codec.h"
#include "dlms_cosem.h"
#include "edge_common.h"
#include "edge_log.h"
#include "hal_crypto.h"

// 内部注册表指针
static const crypto_ops_t *g_crypto_ops = NULL;

void hal_crypto_register(const crypto_ops_t *ops) {
    g_crypto_ops = ops;
    if (g_crypto_ops && g_crypto_ops->init) {
        g_crypto_ops->init();
    }
}

const crypto_ops_t *hal_crypto_get_ops(void) {
    return g_crypto_ops;
}

/**
 * @brief 应用 DLMS 安全保护 (Global Ciphering)
 *
 * @param apdu APDU 缓冲区
 * @param level 安全等级
 * @param ic 调用计数器 (Invocation Counter)
 * @param key 密钥 (通常为 EK)
 * @return int 0 成功
 */
int dlms_security_apply(net_buf_t *apdu, dlms_security_level_t level, uint32_t ic,
                        const uint8_t *key) {
    if (level == DLMS_SEC_NONE) return 0;

    const crypto_ops_t *crypto = hal_crypto_get_ops();
    if (!crypto) {
        LOG_ERROR(LOG_CAT_CORE, "DLMS-SEC: No crypto backend registered!");
        return -1;
    }

    struct net_buf_simple *b = &apdu->b;

    // 1. 预留安全头空间 (Security Control Byte + IC)
    // 布局: [SCB(1)] [IC(4)] [Payload...]
    uint8_t *sec_head = net_buf_simple_push(b, 1 + DLMS_IC_LEN);

    // Security Control Byte: Bit 4-5 = Security Suite (0), Bit 0-3 = Level
    sec_head[0] = (uint8_t)(0x10 | (level & 0x0F));

    // Invocation Counter (Big Endian)
    sec_head[1] = (uint8_t)(ic >> 24);
    sec_head[2] = (uint8_t)(ic >> 16);
    sec_head[3] = (uint8_t)(ic >> 8);
    sec_head[4] = (uint8_t)(ic & 0xFF);

    // 2. 构造 IV (System Title 8 bytes + IC 4 bytes)
    uint8_t iv[12] = {0};
    // TODO: 从关联上下文获取真正的 System Title，此处占位
    // memcpy(iv, system_title, 8);
    memcpy(iv + 8, sec_head + 1, 4);

    if (level >= DLMS_SEC_ENCRYPTED) {
        uint8_t auth_tag[12];
        // 原地加密处理
        // AAD: Security Control Byte + IC (5 bytes)
        // Payload: 开始于 sec_head 之后
        int rc = crypto->aes_gcm_process(key, 16, iv, 12, sec_head, 5, b->data + 5, b->len - 5,
                                         auth_tag, true);

        if (rc != 0) {
            LOG_ERROR(LOG_CAT_CORE, "DLMS-SEC: Encryption failed (rc=%d)", rc);
            return -1;
        }

        // 3. 追加认证标签 (Authentication Tag)
        net_buf_simple_add_mem(b, auth_tag, 12);
    }

    // 4. 封装加密后的 PDU Tag (0x21 = Global Ciphering APDU)
    net_buf_simple_push_u8(b, 0x21);

    return 0;
}

/**
 * @brief 剥离 DLMS 安全保护并还原明文
 */
int dlms_security_remove(net_buf_t *apdu, dlms_security_level_t *out_level, const uint8_t *key) {
    const crypto_ops_t *crypto = hal_crypto_get_ops();
    if (!crypto) return -1;

    struct net_buf_simple *b = &apdu->b;
    uint8_t tag = net_buf_simple_pull_u8(b);

    if (tag != 0x21) {
        *out_level = DLMS_SEC_NONE;
        return 0;  // 明文报文，直接跳过
    }

    // 1. 解析安全头
    uint8_t *sec_head = b->data;
    uint8_t sc = net_buf_simple_pull_u8(b);
    *out_level = (dlms_security_level_t)(sc & 0x0F);

    uint32_t ic = net_buf_simple_pull_be32(b);
    (void)ic;

    // 2. 构造 IV
    uint8_t iv[12] = {0};
    memcpy(iv + 8, sec_head + 1, 4);

    if (*out_level >= DLMS_SEC_ENCRYPTED) {
        // 提取尾部的 Tag (12字节)
        if (b->len < 12) return -1;
        uint8_t *rx_tag = b->data + (b->len - 12);
        size_t payload_len = b->len - 12;

        // 原地解密并校验
        int rc = crypto->aes_gcm_process(key, 16, iv, 12, sec_head, 5, b->data, payload_len, rx_tag,
                                         false);

        if (rc != 0) {
            LOG_ERROR(LOG_CAT_CORE, "DLMS-SEC: Decryption/Auth failed!");
            return -1;
        }

        // 去掉尾部的 Tag
        b->len -= 12;
    }

    return 0;
}