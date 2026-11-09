#ifndef EDGE_HAL_CRYPTO_H
#define EDGE_HAL_CRYPTO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief 密码学后端能力集 (Capabilities)
 */
typedef enum {
    CRYPTO_CAP_AES_GCM = 1 << 0,
    CRYPTO_CAP_ECDSA = 1 << 1,
    CRYPTO_CAP_ECDH = 1 << 2,
    CRYPTO_CAP_TRNG = 1 << 3,
    CRYPTO_CAP_MD5 = 1 << 4,
    CRYPTO_CAP_SHA = 1 << 5,
    CRYPTO_CAP_CERT = 1 << 6,    // 证书解析与管理
    CRYPTO_CAP_HW_KEY = 1 << 7,  // 支持硬件隔离密钥
    CRYPTO_CAP_HMAC = 1 << 8     // 支持 HMAC
} crypto_cap_t;

/**
 * @brief 摘要算法类型
 */
typedef enum {
    CRYPTO_HASH_MD5,
    CRYPTO_HASH_SHA1,
    CRYPTO_HASH_SHA256,
    CRYPTO_HASH_SHA384,
    CRYPTO_HASH_SHA512
} crypto_hash_algo_t;

/**
 * @brief 密钥容器 (支持硬件隔离)
 */
typedef struct {
    const uint8_t *raw_data;  // 明文密钥数据 (软件后端使用)
    size_t len;               // 密钥长度
    uint32_t id;              // 硬件密钥 ID/槽位 (硬件后端使用)
    bool is_hardware;         // 是否使用硬件 ID
} crypto_key_t;

/**
 * @brief 证书类型 (X.509)
 */
typedef enum {
    CERT_TYPE_OWN,      // 设备自身证书
    CERT_TYPE_TRUSTED,  // 信任的根证书/中间证书
    CERT_TYPE_PEER      // 对方(主站)证书
} crypto_cert_type_t;

/**
 * @brief 密码学算法抽象层接口 (Ops Table)
 */
typedef struct crypto_ops_s {
    const char *name;

    /**
     * @brief 初始化后端
     */
    int (*init)(void);

    /**
     * @brief 获取当前后端支持的能力位图
     */
    uint32_t (*get_capabilities)(void);

    /**
     * @brief AEAD 对称加密 (DLMS Suite 0/1/2 核心)
     */
    int (*aes_gcm_process)(const crypto_key_t *key, const uint8_t *iv, size_t iv_len,
                           const uint8_t *aad, size_t aad_len, uint8_t *payload, size_t payload_len,
                           uint8_t *tag, bool encrypt);

    /**
     * @brief 摘要计算 (全量模式)
     */
    int (*hash_calculate)(crypto_hash_algo_t algo, const uint8_t *input, size_t ilen,
                          uint8_t *output);

    /**
     * @brief 流式摘要接口 (针对大块数据)
     */
    void *(*hash_init)(crypto_hash_algo_t algo);
    int (*hash_update)(void *ctx, const uint8_t *input, size_t ilen);
    int (*hash_finish)(void *ctx, uint8_t *output);

    /**
     * @brief HMAC 计算
     */
    int (*hmac_calculate)(crypto_hash_algo_t algo, const crypto_key_t *key, const uint8_t *input,
                          size_t ilen, uint8_t *output);

    /**
     * @brief 非对称签名 (ECDSA)
     */
    int (*ecdsa_sign)(const crypto_key_t *priv_key, const uint8_t *hash, uint8_t *sig);

    /**
     * @brief 非对称验签
     */
    int (*ecdsa_verify)(const crypto_key_t *pub_key, const uint8_t *hash, const uint8_t *sig);

    /**
     * @brief 密钥协商 (ECDH)
     */
    int (*ecdh_compute_shared)(const crypto_key_t *priv_key, const uint8_t *peer_pub,
                               uint8_t *shared_secret);

    /**
     * @brief 证书解析与验证 (X.509)
     */
    void *(*cert_parse)(const uint8_t *cert_der, size_t der_len);
    int (*cert_verify)(void *cert, void *trust_anchor);
    int (*cert_get_pubkey)(void *cert, uint8_t *pubkey_out, size_t *klen);
    void (*cert_free)(void *cert);

    /**
     * @brief 密钥解包 (NIST SP 800-38F / AES Key Wrap)
     * 用于处理主站下发的加密密钥
     */
    int (*aes_key_unwrap)(const crypto_key_t *kek, const uint8_t *input, size_t ilen,
                          uint8_t *output);

    /**
     * @brief 真随机数生成
     */
    int (*trng_generate)(uint8_t *out, size_t len);
} crypto_ops_t;

/**
 * @brief 注册密码学后端实现
 */
void hal_crypto_register(const crypto_ops_t *ops);

/**
 * @brief 获取当前活动的密码学后端
 */
const crypto_ops_t *hal_crypto_get_ops(void);

#endif  // EDGE_HAL_CRYPTO_H
