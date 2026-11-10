/**
 * @file edge_esam_driver.h
 * @brief Production SGCC/CSG Hardware ESAM Security Module & APDU Cryptographic Driver
 */

#ifndef EDGE_ESAM_DRIVER_H
#define EDGE_ESAM_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define ESAM_APDU_MAX_LEN 260

typedef enum {
    ESAM_ALGO_SM2 = 1, // SM2 非对称算法 (签名/验签/密钥交换)
    ESAM_ALGO_SM3 = 2, // SM3 密码杂凑算法 (256-bit Hash)
    ESAM_ALGO_SM4 = 3  // SM4 分组密码算法 (128-bit CBC/ECB)
} esam_crypto_algo_t;

typedef struct {
    uint8_t cla;
    uint8_t ins;
    uint8_t p1;
    uint8_t p2;
    uint8_t lc;
    uint8_t data[ESAM_APDU_MAX_LEN];
    uint8_t le;
} esam_apdu_cmd_t;

typedef struct {
    uint8_t data[ESAM_APDU_MAX_LEN];
    uint16_t len;
    uint16_t sw; // Status Word: 0x9000 = Success, 0x6A82 = File Not Found, etc.
} esam_apdu_resp_t;

typedef struct {
    bool is_initialized;
    uint8_t esam_chip_id[8];
    uint32_t session_counter;
    int (*spi_transmit)(const uint8_t* tx, uint8_t* rx, uint16_t len, void* user_data);
    void* user_data;
} esam_driver_t;

/**
 * @brief 初始化硬件 ESAM 国密安全单元
 */
int esam_driver_init(esam_driver_t* drv, int (*spi_fn)(const uint8_t*, uint8_t*, uint16_t, void*), void* user_data);

/**
 * @brief 发送标准 APDU 指令并接收响应
 */
int esam_send_apdu(esam_driver_t* drv, const esam_apdu_cmd_t* cmd, esam_apdu_resp_t* resp);

/**
 * @brief 指令 0x0084: 生成真随机数 (Get Random)
 */
int esam_get_random(esam_driver_t* drv, uint8_t* out_rand, uint8_t len);

/**
 * @brief 指令 0x803C: 计算 SM3 密码杂凑值 (SM3 Hash)
 */
int esam_compute_sm3_hash(esam_driver_t* drv, const uint8_t* msg, size_t msg_len, uint8_t out_hash[32]);

/**
 * @brief 指令 0x803A: 执行 SM4 CBC 加密/解密 (SM4 Encrypt/Decrypt)
 */
int esam_sm4_cbc_encrypt(esam_driver_t* drv, uint8_t key_index, const uint8_t* iv,
                         const uint8_t* in_data, size_t data_len, uint8_t* out_data);

/**
 * @brief 指令 0x802A: 执行 SM2 报文验签 (SM2 Verify Signature)
 */
int esam_sm2_verify_signature(esam_driver_t* drv, const uint8_t* hash, const uint8_t* sig_r, const uint8_t* sig_s);

#ifdef __cplusplus
}
#endif

#endif // EDGE_ESAM_DRIVER_H
