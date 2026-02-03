/**
 * @file dlms_security.c
 * @brief DLMS 工业级安全平面 (Compliant with Green Book 10+ / Blue Book 15)
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dlms_codec.h"
#include "dlms_cosem.h"
#include "edge_log.h"
#include "hal_crypto.h"
#include "net_buf.h"

/* --- 内部常量定义 --- */
#define DLMS_GCM_TAG_LEN 12
#define DLMS_GCM_IV_LEN 12

/* --- 内部类型补丁 (如果头文件还没更新) --- */
typedef enum { DLMS_SUITE_0 = 0, DLMS_SUITE_1 = 1, DLMS_SUITE_1_HMAC = 2 } dlms_security_suite_t;

typedef struct {
    dlms_security_suite_t suite;
    const uint8_t *my_sys_title;
    const uint8_t *peer_sys_title;
    const uint8_t *ek;  // Encryption Key
    const uint8_t *ak;  // Authentication Key
    size_t key_len;
    uint32_t tx_ic;
    bool is_hw;
} dlms_security_params_t;

// [Nexus] 为适配头文件中的旧签名增加的宏定义
#define DLMS_SEC_ENC_AUTH DLMS_SECURITY_AUTHENTICATION_ENCRYPTION

/* --- 核心逻辑：IV 构造 --- */
static void _dlms_make_iv(const uint8_t *st, uint32_t ic, uint8_t *iv) {
    if (!st) return;
    memcpy(iv, st, 8);
    iv[8] = (uint8_t)(ic >> 24);
    iv[9] = (uint8_t)(ic >> 16);
    iv[10] = (uint8_t)(ic >> 8);
    iv[11] = (uint8_t)(ic & 0xFF);
}

static int _dlms_validate_and_update_ic(uint32_t received_ic, uint32_t *stored_ic) {
    if (stored_ic && received_ic <= *stored_ic) {
        return -1;
    }
    if (stored_ic) *stored_ic = received_ic;
    return 0;
}

/* --- 适配旧接口的实现 --- */

int dlms_security_apply(net_buf_t *apdu, dlms_security_level_t level, uint32_t ic,
                        const uint8_t *key_data) {
    const crypto_ops_t *crypto = hal_crypto_get_ops();
    if (!crypto || level == DLMS_SEC_NONE) return 0;

    struct net_buf_simple *b = &apdu->b;
    size_t plain_len = b->len;

    // 1. 构造 AAD 元数据: [SCB] [IC]
    uint8_t scb = (uint8_t)(0x10 | (level & 0x0F));  // Default suite 0
    uint8_t *head = net_buf_simple_push(b, 5);
    head[0] = scb;
    head[1] = (uint8_t)(ic >> 24);
    head[2] = (uint8_t)(ic >> 16);
    head[3] = (uint8_t)(ic >> 8);
    head[4] = (uint8_t)(ic & 0xFF);

    uint8_t iv[DLMS_GCM_IV_LEN] = {0};
    // 这里因为旧接口没传 system title，先用全 0 占位
    _dlms_make_iv(NULL, ic, iv);

    uint8_t tag[DLMS_GCM_TAG_LEN];
    crypto_key_t key = {.is_hardware = false, .raw_data = key_data, .len = 16};

    if (level == DLMS_SEC_AUTHENTICATED) {
        crypto->aes_gcm_process(&key, iv, DLMS_GCM_IV_LEN, head, 5 + plain_len, NULL, 0, tag, true);
        net_buf_simple_add_mem(b, tag, DLMS_GCM_TAG_LEN);
    } else {
        int rc = crypto->aes_gcm_process(&key, iv, DLMS_GCM_IV_LEN, head, 5, b->data + 5, plain_len,
                                         tag, true);
        if (rc != 0) return -1;
        if (level == DLMS_SEC_AUTHENTICATED_ENCRYPTED) {
            net_buf_simple_add_mem(b, tag, DLMS_GCM_TAG_LEN);
        }
    }

    net_buf_simple_push_u8(b, 0x21);
    return 0;
}

int dlms_security_remove(net_buf_t *apdu, dlms_security_level_t *out_level,
                         const uint8_t *key_data) {
    const crypto_ops_t *crypto = hal_crypto_get_ops();
    if (!crypto) return -1;
    struct net_buf_simple *b = &apdu->b;

    if (b->len < 1) return -1;
    uint8_t tag = net_buf_simple_pull_u8(b);
    if (tag != 0x21 && tag != 0x28) return 0;

    if (b->len < 5) return -1;
    uint8_t *head_ptr = b->data;
    uint8_t scb = net_buf_simple_pull_u8(b);
    uint32_t received_ic = net_buf_simple_pull_be32(b);

    dlms_security_level_t level = (dlms_security_level_t)(scb & 0x0F);
    if (out_level) *out_level = level;

    uint8_t iv[DLMS_GCM_IV_LEN] = {0};
    _dlms_make_iv(NULL, received_ic, iv);

    crypto_key_t key = {.raw_data = key_data, .len = 16};

    if (level == DLMS_SEC_AUTHENTICATED) {
        if (b->len < DLMS_GCM_TAG_LEN) return -1;
        uint8_t *rx_tag = b->data + (b->len - DLMS_GCM_TAG_LEN);
        int rc = crypto->aes_gcm_process(&key, iv, DLMS_GCM_IV_LEN, head_ptr,
                                         b->len - DLMS_GCM_TAG_LEN + 5, NULL, 0, rx_tag, false);
        if (rc != 0) return -1;
        b->len -= DLMS_GCM_TAG_LEN;
    } else {
        size_t tag_len = (level == DLMS_SEC_AUTHENTICATED_ENCRYPTED) ? DLMS_GCM_TAG_LEN : 0;
        if (b->len < tag_len) return -1;
        uint8_t *rx_tag = tag_len ? (b->data + (b->len - tag_len)) : NULL;

        int rc = crypto->aes_gcm_process(&key, iv, DLMS_GCM_IV_LEN, head_ptr, 5, b->data,
                                         b->len - tag_len, rx_tag, false);
        if (rc != 0) return -1;
        b->len -= (uint16_t)tag_len;
    }

    return 0;
}

/* --- 高级扩展接口 (供后续升级使用) --- */

int dlms_security_update_key(const uint8_t *kek, size_t kek_len, const uint8_t *wrapped_key,
                             size_t wrapped_len, uint8_t *out_plain_key) {
    const crypto_ops_t *crypto = hal_crypto_get_ops();
    if (!crypto || !crypto->aes_key_unwrap) return -1;

    crypto_key_t master_key = {.raw_data = kek, .len = kek_len};
    return crypto->aes_key_unwrap(&master_key, wrapped_key, wrapped_len, out_plain_key);
}