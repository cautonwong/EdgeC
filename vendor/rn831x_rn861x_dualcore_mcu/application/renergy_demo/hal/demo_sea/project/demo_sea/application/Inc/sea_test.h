/******************************************************************************
 * @file        sea_test.h
 * @brief
 * @author      Renergy Technology
 * @note
 *
 * Copyright (C), Renergy Technology Inc. All rights reserved.
 *
 *****************************************************************************/

#ifndef __SEA_TEST_H__
#define __SEA_TEST_H__

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "rn_rv_hal.h"
#include "rn_rv_ll_wdt.h"
#include "unity.h"

#define SEA_HAL_FULL_TEST

#define WDT_FEED 		 (WDT->EN = 0xbb)

#define GET_CYCLE(cycle) (__ASM volatile ("rdcycle %0" : "=r"(cycle)))

static uint32_t AES_GCM_DMA_TEST[100];

#define HASH_CRYPT_ITEMS_IN_BYTES
#define ECC_CRYPT_ITEMS_IN_BYTES

#define HASH_PROCESS_LATCH_RECOVER_SUPPORT
#define AES_DECRYPT_GCM_SPECIAL_MOD_TYPE 0

#ifdef HASH_CRYPT_ITEMS_IN_BYTES
typedef uint8_t hashData_t; /* input/output items are in bytes */
#else
typedef uint32_t hashData_t;
#endif

typedef uint32_t regData_t;

#ifdef ECC_CRYPT_ITEMS_IN_BYTES
typedef uint8_t ECCdata_t;
#else
typedef uint32_t ECCdata_t;
#endif

#ifdef AES_PROCESS_LATCH_RECOVER_SUPPORT
extern void aes_128_gcm_demo_init(aesProcessParameters *pParas, AES_ENCRYPT_SEL encrypt_sel);
extern void aes_128_gcm_demo_crypt_sign(aesProcessParameters *pParas);
extern void aes_128_gcm_demo_decrypt_verify(aesProcessParameters *pParas);
#endif
extern void aes_gmac_demo_test(void);

#ifdef HASH_PROCESS_LATCH_RECOVER_SUPPORT
extern void hash_384_demo_single_calc(void);
#endif

#define WDT_EN_ADDR (0x20100000U)
void 	 word2str(uint32_t *words, uint32_t bytelen);
void	 WdtFeed(void);
void 	 str2hex(const uint8_t *input_str, uint8_t *output_hex, uint32_t byte_len);
uint32_t getHashOutSize(const HASH_InitTypeDef *pParas);

#define SEA_STACK_MAX_SIZE (2048 + 4)

typedef struct
{
    uint32_t           *key;
    uint32_t            keylen;
    uint32_t           *pt;
    uint32_t            ptlen;
    uint32_t           *ct;
    AES_BITWIDE_MODE    bitWideMode;
    AES_CRYPT_MODE      cryptMode;
} SEA_AES_ECB_TESTLIST;

typedef struct
{
    uint32_t           *kek;
    uint32_t           *key;
    uint32_t            keylen;
    uint32_t           *ct;
    uint32_t            ctlen;
    AES_BITWIDE_MODE    bitWideMode;
    AES_CRYPT_MODE      cryptMode;
    AES_KW_MODE         keyWrapMode;
} SEA_AES_KW_TESTLIST;

typedef struct
{
    uint32_t           *key;
    uint32_t            keylen;
    uint32_t           *pt;
    uint32_t            ptlen;
    uint32_t           *iv;
    uint32_t            ivlen;
    uint32_t           *ct;
    AES_BITWIDE_MODE    bitWideMode;
    AES_CRYPT_MODE      cryptMode;
} SEA_AES_OTHERS_TESTLIST;

typedef struct
{
    uint32_t           *key;
    uint32_t            keylen;
    uint32_t           *pt;
    uint32_t            ptlen;
    uint32_t           *aad;
    uint32_t            aadlen;
    uint32_t           *iv;
    uint32_t            ivlen;
    uint32_t           *ct;
    uint32_t           *tag;
    uint32_t            taglen;
    AES_BITWIDE_MODE    bitWideMode;
    AES_CRYPT_MODE      cryptMode;
} SEA_AES_GCM_TESTLIST;

typedef struct
{
    uint32_t           *pt;
    uint32_t            ptlen;
    uint32_t           *key;
    uint32_t            keylen;
    uint32_t           *iv;
    uint32_t            ivlen;
    uint32_t           *aad;
    uint32_t            aadlen;
    uint32_t           *ct;
    uint32_t           *tag;
    uint32_t            taglen;
    AES_BITWIDE_MODE    bitWideMode;
    AES_CRYPT_MODE      cryptMode;
} SEA_AES_AAD_TESTLIST;

typedef struct
{
    uint32_t           *msg;
    uint32_t            msglen;
    uint32_t           *expected;
} SEA_HASH_TESTLIST;

typedef enum __attribute__((__packed__))
{
    MOD2EN_AES_CKE      = 8,
    MOD2EN_KEYSTORE_CKE = 7,
    MOD2EN_SEA_SHA_CKE  = 6,
    MOD2EN_SEA_TRNG_CKE = 5,
    MOD2EN_SEA_AES_CKE  = 4,
    MOD2EN_SEA_PKE_CKE  = 3,
    MOD2EN_SEA_BUS_CKE  = 2,
    MOD2EN_QSPI1_CKE    = 1,
    MOD2EN_QSPI0_CKE    = 0,
} MOD2EN_TypeDef;

void aes_ecb_test(void);
void aes_gcm_test(void);
void aes_gmac_test(void);
void aes_others_test(void);
void aes_keywrap_test(void);
void aes_updateAAD_test(void);
void hash_test(void);
void rsa_test(void);

void ECC_sign_test(void);
void ECC_keygen_test(void);
void ECDH_NIST256_test(void);
void ECDH_NIST384_test(void);
void ECDSA_Ver_NIST256(void);
void ECDSA_Ver_NIST384(void);
void ECDSA_Ver_NIST521(void);

uint32_t sea_update_stat(void);
void sea_init_all(AES_KernelTypeDef version);
void sea_memset_word(uint32_t dst[], uint32_t data, uint32_t len);
void sea_memset(uint8_t dst[], const uint8_t data, uint32_t len);
void sea_memcpy_word(uint32_t dst[], const uint32_t src[], uint32_t len);
void sea_memcpy(uint8_t dst[], const uint8_t src[], uint32_t len);

#define MEM_U32(addr)                (*((__IOM uint32_t *)(addr)))
#define SYS_PSW_ADDR(ver)            (((ver) == AES_RUNNING_IN_CORE0) ? 0x20080000U : 0xA0080000U)
#define SYS_PMOD0_ADDR(ver)          (((ver) == AES_RUNNING_IN_CORE0) ? 0x2008012CU : 0xA008012CU)
#define SYS_MOD0_ADDR(ver)           (((ver) == AES_RUNNING_IN_CORE0) ? 0x2008013CU : 0xA008013CU)
#define SYS_MOD1_ADDR(ver)           (((ver) == AES_RUNNING_IN_CORE0) ? 0x20080140U : 0xA0080140U)
#define SYS_MOD2_ADDR(ver)           (((ver) == AES_RUNNING_IN_CORE0) ? 0x20080144U : 0xA0080144U)
#define SYS_FREQ_STA_ADDR(ver)       (((ver) == AES_RUNNING_IN_CORE0) ? 0x20080170U : 0xA0080170U)
#define SYS_CLK_STA_ADDR(ver)        (((ver) == AES_RUNNING_IN_CORE0) ? 0x20080174U : 0xA0080174U)
#define SCU_PERIPH_SEC_CFG2(ver)     (((ver) == AES_RUNNING_IN_CORE0) ? 0x30018010U : 0xB0018010U)
#define SCU_SRAM_SEC_WM_CFG(ver)     (((ver) == AES_RUNNING_IN_CORE0) ? 0x3001805CU : 0xB001805CU)

#define SEA_EMB_BASEADDR(ver)        (((ver) == AES_RUNNING_IN_CORE0) ? 0x30009000U : 0xB0009000U)
#define SEA_CYPHER_ADDR(ver)         (((ver) == AES_RUNNING_IN_CORE0) ? 0x30008000U : 0xB0008000U)
#define SEA_AES_IV_ADDR(ver)         (((ver) == AES_RUNNING_IN_CORE0) ? 0x30008150U : 0xB0008150U)
#define SEA_AES_EXP_KEY_ADDR(ver)    (SEA_EMB_BASEADDR(ver) + 0x760U)
#define SEA_AES_CYPHER_IN_ADDR(ver)  (SEA_EMB_BASEADDR(ver) + 0x740U)
#define SEA_AES_CYPHER_OUT_ADDR(ver) (SEA_EMB_BASEADDR(ver) + 0xF90U)
#define SEA_HASH_ADDR(ver)           SEA_EMB_BASEADDR(ver)

typedef enum __attribute__((__packed__)) {
    SEA_SUCCESS = 0,
    SEA_ERROR = !(SEA_SUCCESS),
    // AES_CRYPTKEY_ADDR_ALIGN_FAILURE = 2,
    // AES_CRYPT_FAILURE = 3,
    // AES_DECRYPT_FAILURE = 4,
    // AES_GMAC_FAILURE = 5,
    // AES_NULL_POINTER_FAILURE = 6,
    // AES_KW_MODE_FAILURE = 7,
    // AES_KW_INLEN_FAILURE = 8,
    // AES_KW_CHECK_ICV_FAILURE = 9,
    // AES_KW_CHECK_PADDING_FAILURE = 10,
    SHA_DATA_ALIGN_FAILURE = 11,
    SHA_DATASIGN_FAILURE = 12,
    SHA_DMA_TIMEOUT_FAILURE = 13,
    SHA_DMA_DATA_TRANS_FAILURE = 14,
    ECC_TIMEOUT_FAILURE = 15,
    ECC_PARASLEN_FAILURE = 16,
    ECC_LOOPTIMETOUT_FAILURE = 17,
    ECC_PKEY_FAILURE = 18,
    RSA_TIMEOUT_FAILURE = 19,
    RSA_PARASLEN_FAILURE = 20,
    RSA_KEY_FAILURE = 21,
    RSA_PADDING_FAILURE = 22,
    SEA_KS_WRKEY_FAILURE = 23,
    SEA_KS_RDKEY_FAILURE = 24,
    SEA_TRNG_KEYGEN_FAILURE = 25,
    SEA_TRNG_KLEN_FAILURE = 26,
    SEA_TRNG_TIMEOUT_FAILURE = 27,
    SEA_TRNG_RANDOM_FAILURE = 28,
    SEA_FLASH_CLK_FREQ_FAILURE = 29,
} RetStatus;

#ifdef SEA_HAL_FULL_TEST
#define LL_AES_gf128_mult(pParas, configuration)                              HAL_SEA_AES_GF128Mul(pParas, configuration)
#define LL_AES_crypt_init(pParas, configuration)                              HAL_SEA_AES_Init(pParas, configuration)
#define LL_AES_crypt_deinit()                                                 HAL_SEA_AES_DeInit()
#define LL_AES_crypt_sign(pParas, plainData, size, is_last, cryptedData)      HAL_SEA_AES_Encrypt(pParas, plainData, size, is_last, cryptedData)
#define LL_AES_decrypt_verify(pParas, cryptedData, size, is_last, plainData)  HAL_SEA_AES_Decrypt(pParas, cryptedData, size, is_last, plainData)
#define LL_AES_gmac(pParas, ghashData, size, is_last)                         HAL_SEA_AES_Gmac(pParas, ghashData, size, is_last)
#define LL_AES_process_status_latch(pParas, latch_buf)                        HAL_SEA_AES_LatchProcessStatus(pParas, latch_buf)
#define LL_AES_process_status_recover(recover_buf, pParas)                    HAL_SEA_AES_RecoverProcessStatus(recover_buf, pParas)
#define LL_AES_key_wrap(pParas, plainData, inlen, cryptedData, outlen)        HAL_SEA_AES_KeyWrapping(pParas, plainData, inlen, cryptedData, outlen)
#define LL_AES_key_unwrap(pParas, cryptedData, inlen, plainData, outlen)      HAL_SEA_AES_KeyUnwrapping(pParas, cryptedData, inlen, plainData, outlen)
#define LL_AES_update_aad(pParas, aadData, size)                              HAL_SEA_AES_SetAAD(pParas, aadData, size)

#else
#define LL_AES_gf128_mult(pParas, configuration)                              HAL_AES_GF128Mul(pParas, configuration)
#define LL_AES_crypt_init(pParas, configuration)                              HAL_AES_Init(pParas, configuration)
#define LL_AES_crypt_deinit()                                                 HAL_AES_DeInit()
#define LL_AES_crypt_sign(pParas, plainData, size, is_last, cryptedData)      HAL_AES_Encrypt(pParas, plainData, size, is_last, cryptedData)
#define LL_AES_decrypt_verify(pParas, cryptedData, size, is_last, plainData)  HAL_AES_Decrypt(pParas, cryptedData, size, is_last, plainData)
#define LL_AES_gmac(pParas, ghashData, size, is_last)                         HAL_AES_Gmac(pParas, ghashData, size, is_last)
#define LL_AES_process_status_latch(pParas, latch_buf)                        HAL_AES_LatchProcessStatus(pParas, latch_buf)
#define LL_AES_process_status_recover(recover_buf, pParas)                    HAL_AES_RecoverProcessStatus(recover_buf, pParas)
#define LL_AES_key_wrap(pParas, plainData, inlen, cryptedData, outlen)        HAL_AES_KeyWrapping(pParas, plainData, inlen, cryptedData, outlen)
#define LL_AES_key_unwrap(pParas, cryptedData, inlen, plainData, outlen)      HAL_AES_KeyUnwrapping(pParas, cryptedData, inlen, plainData, outlen)
#define LL_AES_update_aad(pParas, aadData, size)                              HAL_AES_SetAAD(pParas, aadData, size)
#endif

#define LL_HASH_init(pParas, configuration)                                   HAL_SEA_HASH_Init(pParas, configuration)
#define LL_HASH_deinit()                                                      HAL_SEA_HASH_DeInit()
#define LL_HASH(pParas, verifyMessage, size, is_last, verifyOut)              HAL_SEA_HASH(pParas, verifyMessage, size, is_last, verifyOut)
#define LL_HASH_process_status_latch(pParas, latch_buf)                       HAL_SEA_HASH_LatchProcessStatus(pParas, latch_buf)
#define LL_HASH_process_status_recover(recover_buf, pParas)                   HAL_SEA_HASH_RecoverProcessStatus(recover_buf, pParas)

#define LL_ECC_keygen(curve, Q, pkey)                                         HAL_SEA_ECC_GenerateKey(curve, Q, pkey)
#define LL_ECC_public_keygen(curve, Q, pkey)                                  HAL_SEA_ECC_GeneratePublicKey(curve, Q, pkey)
#define LL_ECC_public_keygen_uncompressed(curve, Q, pkey)                     HAL_SEA_ECC_GenerateUncompPublicKey(curve, Q, pkey)
#define LL_ECC_keyver(curve, Q, ver_flag)                                     HAL_SEA_ECC_VerifyPublicKey(curve, Q, ver_flag)
#define LL_ECC_DHkeygen(curve, Q, pkey, DHkey)                                HAL_SEA_ECC_GenerateDHKeyPairs(curve, Q, pkey, DHkey)
#define LL_ECC_DHkeygen_x(curve, Q, pkey, DHkey)                              HAL_SEA_ECC_GenerateDHKeyX(curve, Q, pkey, DHkey)
#define LL_ECC_sign_gen(curve, Hm, len, Q,pkey, r, s)                         HAL_SEA_ECC_GenerateSignature(curve, Hm, len, Q,pkey, r, s)
#define LL_ECC_specify_pkey_sign_gen(curve, Hm, len, Q, pkey, r, s)           HAL_SEA_ECC_GenerateSignaturewithPKey(curve, Hm, len, Q, pkey, r, s)
#define LL_ECC_sign_ver(curve, Hm, len, Q, r, s, ver_flag)                    HAL_SEA_ECC_VerifySignature(curve, Hm, len, Q, r, s, ver_flag)
#define LL_ECC_keygen_sec(curve, pkIdx, QxIdx, QyIdx)                         HAL_SEA_ECC_GenerateKeySecurely(curve, pkIdx, QxIdx, QyIdx)
#define LL_ECC_sign_gen_sec(curve, Hm, len, pkIdx, r, s)                      HAL_SEA_ECC_GenerateSignatureSecurely(curve, Hm, len, pkIdx, r, s)
#define LL_ECC_sign_ver_sec(curve, Hm, len, QxIdx, QyIdx, r, s, ver_flag)     HAL_SEA_ECC_VerifySignatureSecurely(curve, Hm, len, QxIdx, QyIdx, r, s, ver_flag)

#define LL_TRNG_gen(random, data_len)                                         HAL_SEA_TRNG_GenerateRandom(random, data_len)
#define LL_TRNG_keygen(kp, keyIdx)                                            HAL_SEA_TRNG_GenerateKey(kp, keyIdx)

#define LL_RSA_keygen(rsa)                                                    HAL_SEA_RSA_GenerateKey(rsa)
#define LL_RSA_sign_gen(rsa, msg, mbits, sign)                                HAL_SEA_RSA_GenerateSignature(rsa, msg, mbits, sign)
#define LL_RSA_sign_ver(rsa, msg, mbits, sign)                                HAL_SEA_RSA_VerifySignature(rsa, msg, mbits, sign)
#define LL_RSA_pub_encrypt(rsa, plain, plainlen, cipher)                      HAL_SEA_RSA_EncryptwithPublicKey(rsa, plain, plainlen, cipher)
#define LL_RSA_pub_decrypt(rsa, cipher, cipherlen, plain)                     HAL_SEA_RSA_DecryptwithPublicKey(rsa, cipher, cipherlen, plain)
#define LL_RSA_pvt_encrypt(rsa, plain, plainlen, cipher)                      HAL_SEA_RSA_EncryptwithPrivateKey(rsa, plain, plainlen, cipher)
#define LL_RSA_pvt_decrypt(rsa, cipher, cipherlen, plain)                     HAL_SEA_RSA_DecryptwithPrivateKey(rsa, cipher, cipherlen, plain)

#endif
