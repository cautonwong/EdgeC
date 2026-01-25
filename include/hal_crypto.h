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
    CRYPTO_CAP_CERT = 1 << 6,   // 证书解析与管理
    CRYPTO_CAP_HW_KEY = 1 << 7  // 支持硬件隔离密钥
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
    int (*aes_gcm_process)(const uint8_t *key, size_t key_len, const uint8_t *iv, size_t iv_len,
                           const uint8_t *aad, size_t aad_len, uint8_t *payload, size_t payload_len,
                           uint8_t *tag, bool encrypt);

    /**
     * @brief 摘要计算 (MD5/SHA系列)
     * @param algo 算法类型
     * @param input 输入数据
     * @param ilen 输入长度
     * @param output 输出缓冲区
     */
    int (*hash_calculate)(crypto_hash_algo_t algo, const uint8_t *input, size_t ilen,
                          uint8_t *output);

    /**
     * @brief 非对称签名 (ECDSA)
     */
    int (*ecdsa_sign)(const uint8_t *priv_key, const uint8_t *hash, uint8_t *sig);

    /**
     * @brief 非对称验签
     */
    int (*ecdsa_verify)(const uint8_t *pub_key, const uint8_t *hash, const uint8_t *sig);

    /**
     * @brief 密钥协商 (ECDH)
     */
    int (*ecdh_compute_shared)(const uint8_t *priv_key, const uint8_t *peer_pub,
                               uint8_t *shared_secret);

    /**
     * @brief 证书解析与验证 (X.509)
     * @param cert_der 证书原始数据 (DER格式)
     * @param der_len 长度
     * @return 证书句柄 (由实现分配，失败返回 NULL)
     */
    void *(*cert_parse)(const uint8_t *cert_der, size_t der_len);

    /**
     * @brief 验证证书链
     * @param cert 待验证证书句柄
     * @param trust_anchor 信任锚点句柄 (根证书)
     * @return 0 成功
     */
    int (*cert_verify)(void *cert, void *trust_anchor);

    /**
     * @brief 从证书中提取公钥
     * @param cert 证书句柄
     * @param pubkey_out 输出缓冲区
     * @param klen 缓冲区长度
     */
    int (*cert_get_pubkey)(void *cert, uint8_t *pubkey_out, size_t *klen);

    /**
     * @brief 释放证书句柄
     */
    void (*cert_free)(void *cert);

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