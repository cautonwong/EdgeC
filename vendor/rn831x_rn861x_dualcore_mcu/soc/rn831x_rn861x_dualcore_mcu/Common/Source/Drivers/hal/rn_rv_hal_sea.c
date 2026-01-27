/**
 ******************************************************************************
 * @file   rn_rv_hal_sea.c
 * @author Renergy Software Team
 * @brief  SEA HAL module driver.
 *         This file provides firmware functions to manage the following
 *         functionalities of the Security Encryption Accelerator (SEA) peripheral:
 *          + AES IO operation functions
 *          + HASH IO operation functions
 *          + ECC IO operation functions
 *          + RSA IO operation functions
 *          + TRNG IO operation functions
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */


/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal.h"
#include <stdio.h>
#include <stddef.h>

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @defgroup SEA SEA
 * @brief     SEA driver modules
 * @{
 */

#if defined(HAL_SEA_MODULE_ENABLED) && \
    defined(HAL_RPC_MODULE_ENABLED) && \
    defined(HAL_KEYSTORE_MODULE_ENABLED)

/* Private typedef ----------------------------------------------------------*/
/** @defgroup SEA_Private_Typedef SEA Private Typedef
 * @{
 */

/**
 * @brief SEA IAP interface defination.
 */
typedef struct
{
    uint32_t Reserved[11];                   /*!< Reserved. */
    uint32_t (*AES_GF128Mul)                 (AES_HandleTypeDef *haes, uint8_t *pDataX, uint8_t *pDataY, uint8_t *pDataZ);
    uint32_t (*AES_Init)                     (AES_HandleTypeDef *haes, AES_InitTypeDef *pConfig);
    uint32_t (*AES_DeInit)                   (void);
    uint32_t (*AES_Encrypt)                  (AES_HandleTypeDef *haes, uint8_t *Input, uint32_t InputLen, CYPHER_DATA_STA IsLast, uint8_t *Output);
    uint32_t (*AES_Decrypt)                  (AES_HandleTypeDef *haes, uint8_t *Input, uint32_t InputLen, CYPHER_DATA_STA IsLast, uint8_t *Output);
    uint32_t (*AES_Gmac)                     (AES_HandleTypeDef *haes, uint8_t *ghashData, uint32_t InputLen, CYPHER_DATA_STA IsLast);
    uint32_t (*AES_LatchProcessStatus)       (AES_HandleTypeDef *haes, AES_HandleTypeDef *LatchBuf);
    uint32_t (*AES_RecoverProcessStatus)     (AES_HandleTypeDef *RecoverBuf, AES_HandleTypeDef *haes);
    uint32_t (*AES_KeyWrapping)              (AES_HandleTypeDef *haes, uint8_t *Input, uint32_t InputLen, uint8_t *Output, uint32_t *OutputLen);
    uint32_t (*AES_KeyUnwrapping)            (AES_HandleTypeDef *haes, uint8_t *Input, uint32_t InputLen, uint8_t *Output, uint32_t *OutputLen);
    uint32_t (*AES_SetAAD)                   (AES_HandleTypeDef *haes, uint8_t *Input, uint32_t InputLen);
    uint32_t (*AES_RunOneTime)               (void);

    uint32_t (*HASH_Init)                    (HASH_HandleTypeDef *hhash, HASH_InitTypeDef *pConfig);
    uint32_t (*HASH_DeInit)                  (void);
    uint32_t (*HASH)                         (HASH_HandleTypeDef *hhash, uint8_t *Input, uint32_t InputLen, CYPHER_DATA_STA IsLast, uint8_t *Output);
    uint32_t (*HASH_LatchProcessStatus)      (HASH_HandleTypeDef *hhash, HASH_HandleTypeDef *LatchBuf);
    uint32_t (*HASH_RecoverProcessStatus)    (HASH_HandleTypeDef *RecoverBuf, HASH_HandleTypeDef *hhash);

    uint32_t (*ECC_GenerateSignature)        (ECC_HandleTypeDef *hecc, uint8_t *HashMessage, uint32_t MessageLen, uint8_t *Q, uint8_t *K, uint8_t *R, uint8_t *S);
    uint32_t (*ECC_GenerateSignaturewithPKey)(ECC_HandleTypeDef *hecc, uint8_t *HashMessage, uint32_t MessageLen, uint8_t *Q, uint8_t *K, uint8_t *R, uint8_t *S);
    uint32_t (*ECC_VerifySignature)          (ECC_HandleTypeDef *hecc, uint8_t *HashMessage, uint32_t MessageLen, uint8_t *Q, uint8_t *R, uint8_t *S, HAL_SEA_FlagTypeDef *pFlag);
    uint32_t (*ECC_GenerateDHKeyPairs)       (ECC_HandleTypeDef *hecc, uint8_t *Q, uint8_t *K, uint8_t *DHKey);
    uint32_t (*ECC_GenerateKey)              (ECC_HandleTypeDef *hecc, uint8_t *Q, uint8_t *K);
    uint32_t (*ECC_GeneratePublicKey)        (ECC_HandleTypeDef *hecc, uint8_t *Q, uint8_t *K);
    uint32_t (*ECC_VerifyPublicKey)          (ECC_HandleTypeDef *hecc, uint8_t *Q, HAL_SEA_FlagTypeDef *pFlag);
    uint32_t (*ECC_GenerateKeySecurely)      (ECC_HandleTypeDef *hecc, uint8_t PKeyIdx, uint8_t QxIdx, uint8_t QyIdx);
    uint32_t (*ECC_GenerateSignatureSecurely)(ECC_HandleTypeDef *hecc, uint8_t *HashMessage, uint32_t MessageLen, uint8_t PKeyIdx, uint8_t *R, uint8_t *S);
    uint32_t (*ECC_VerifySignatureSecurely)  (ECC_HandleTypeDef *hecc, uint8_t *HashMessage, uint32_t MessageLen, uint8_t QxIdx, uint8_t QyIdx, uint8_t *R, uint8_t *S, HAL_SEA_FlagTypeDef *pFlag);

    uint32_t (*RSA_GenerateKey)              (RSA_HandleTypeDef *hrsa);
    uint32_t (*RSA_GenerateSignature)        (RSA_HandleTypeDef *hrsa, uint32_t *HashMessage, uint32_t MessageLen, uint32_t *Signature);
    uint32_t (*RSA_VerifySignature)          (RSA_HandleTypeDef *hrsa, uint32_t *HashMessage, uint32_t MessageLen, uint32_t *Signature);
    uint32_t (*RSA_EncryptwithPublicKey)     (RSA_HandleTypeDef *hrsa, uint32_t *Input, uint32_t InputLen, uint32_t *Output);
    uint32_t (*RSA_DecryptwithPublicKey)     (RSA_HandleTypeDef *hrsa, uint32_t *Input, uint32_t InputLen, uint32_t *Output);
    uint32_t (*RSA_EncryptwithPrivateKey)    (RSA_HandleTypeDef *hrsa, uint32_t *Input, uint32_t InputLen, uint32_t *Output);
    uint32_t (*RSA_DecryptwithPrivateKey)    (RSA_HandleTypeDef *hrsa, uint32_t *Input, uint32_t InputLen, uint32_t *Output);

    void     (*TRNG_GenerateRandom)          (uint32_t *RandomData, uint8_t DataLen);
} HAL_SEA_ApiTypeDef;

/**
 * @brief SEA IAP function pointer.
 */
typedef uint32_t (*pAES_GF128Mul)                 (AES_HandleTypeDef *haes, const uint8_t *pDataX, const uint8_t *pDataY, uint8_t *pDataZ);
typedef uint32_t (*pAES_Init)                     (AES_HandleTypeDef *haes, const AES_InitTypeDef *pConfig);
typedef uint32_t (*pAES_DeInit)                   (void);
typedef uint32_t (*pAES_Encrypt)                  (AES_HandleTypeDef *haes, const uint8_t *Input, uint32_t InputLen, CYPHER_DATA_STA IsLast, uint8_t *Output);
typedef uint32_t (*pAES_Decrypt)                  (AES_HandleTypeDef *haes, const uint8_t *Input, uint32_t InputLen, CYPHER_DATA_STA IsLast, uint8_t *Output);
typedef uint32_t (*pAES_Gmac)                     (AES_HandleTypeDef *haes, const uint8_t *ghashData, uint32_t InputLen, CYPHER_DATA_STA IsLast);
typedef uint32_t (*pAES_LatchProcessStatus)       (AES_HandleTypeDef *haes, AES_HandleTypeDef *LatchBuf);
typedef uint32_t (*pAES_RecoverProcessStatus)     (AES_HandleTypeDef *RecoverBuf, AES_HandleTypeDef *haes);
typedef uint32_t (*pAES_KeyWrapping)              (AES_HandleTypeDef *haes, const uint8_t *Input, uint32_t InputLen, uint8_t *Output, uint32_t *OutputLen);
typedef uint32_t (*pAES_KeyUnwrapping)            (AES_HandleTypeDef *haes, const uint8_t *Input, uint32_t InputLen, uint8_t *Output, uint32_t *OutputLen);
typedef uint32_t (*pAES_SetAAD)                   (AES_HandleTypeDef *haes, const uint8_t *Input, uint32_t InputLen);
typedef uint32_t (*pAES_RunOneTime)               (void);

typedef uint32_t (*pHASH_Init)                    (HASH_HandleTypeDef *hhash, const HASH_InitTypeDef *pConfig);
typedef uint32_t (*pHASH_DeInit)                  (void);
typedef uint32_t (*pHASH)                         (HASH_HandleTypeDef *hhash, const uint8_t *Input, uint32_t InputLen, CYPHER_DATA_STA IsLast, uint8_t *Output);
typedef uint32_t (*pHASH_LatchProcessStatus)      (const HASH_HandleTypeDef *hhash, HASH_HandleTypeDef *LatchBuf);
typedef uint32_t (*pHASH_RecoverProcessStatus)    (const HASH_HandleTypeDef *RecoverBuf, HASH_HandleTypeDef *hhash);

typedef uint32_t (*pECC_GenerateSignature)        (const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, uint8_t *Q, uint8_t *K, uint8_t *R, uint8_t *S);
typedef uint32_t (*pECC_GenerateSignaturewithPKey)(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, uint8_t *Q, const uint8_t *K, uint8_t *R, uint8_t *S);
typedef uint32_t (*pECC_VerifySignature)          (const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t *Q, const uint8_t *R, const uint8_t *S, HAL_SEA_FlagTypeDef *pFlag);
typedef uint32_t (*pECC_GenerateDHKeyPairs)       (const ECC_HandleTypeDef *hecc, const uint8_t *Q, const uint8_t *K, uint8_t *DHKey);
typedef uint32_t (*pECC_GenerateKey)              (const ECC_HandleTypeDef *hecc, uint8_t *Q, uint8_t *K);
typedef uint32_t (*pECC_GeneratePublicKey)        (const ECC_HandleTypeDef *hecc, uint8_t *Q, const uint8_t *K);
typedef uint32_t (*pECC_VerifyPublicKey)          (const ECC_HandleTypeDef *hecc, const uint8_t *Q, HAL_SEA_FlagTypeDef *pFlag);
typedef uint32_t (*pECC_GenerateKeySecurely)      (const ECC_HandleTypeDef *hecc, uint8_t PKeyIdx, uint8_t QxIdx, uint8_t QyIdx);
typedef uint32_t (*pECC_GenerateSignatureSecurely)(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t PKeyIdx, uint8_t *R, uint8_t *S);
typedef uint32_t (*pECC_VerifySignatureSecurely)  (const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t QxIdx, const uint8_t QyIdx, const uint8_t *R, const uint8_t *S, HAL_SEA_FlagTypeDef *pFlag);

typedef uint32_t (*pRSA_GenerateKey)              (RSA_HandleTypeDef *hrsa);
typedef uint32_t (*pRSA_GenerateSignature)        (RSA_HandleTypeDef *hrsa, const uint32_t *HashMessage, uint32_t MessageLen, uint32_t *Signature);
typedef uint32_t (*pRSA_VerifySignature)          (RSA_HandleTypeDef *hrsa, const uint32_t *HashMessage, uint32_t MessageLen, uint32_t *Signature);
typedef uint32_t (*pRSA_EncryptwithPublicKey)     (RSA_HandleTypeDef *hrsa, const uint32_t *Input, uint32_t InputLen, uint32_t *Output);
typedef uint32_t (*pRSA_DecryptwithPublicKey)     (RSA_HandleTypeDef *hrsa, const uint32_t *Input, uint32_t InputLen, uint32_t *Output);
typedef uint32_t (*pRSA_EncryptwithPrivateKey)    (RSA_HandleTypeDef *hrsa, const uint32_t *Input, uint32_t InputLen, uint32_t *Output);
typedef uint32_t (*pRSA_DecryptwithPrivateKey)    (RSA_HandleTypeDef *hrsa, const uint32_t *Input, uint32_t InputLen, uint32_t *Output);

typedef void     (*pTRNG_GenerateRandom)          (uint32_t *RandomData, uint8_t DataLen);
/**
 * @}
 */

/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @defgroup SEA_Exported_Functions SEA Exported Functions
 * @{
 */

/** @defgroup SEA_Exported_Functions_Group1 AES IO operation functions
 * @brief     AES IO operation functions
 *
@verbatim
================================================================================
                    ##### AES IO operation functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) HAL_SEA_AES_Init                    : Init the AES module
        (+) HAL_SEA_AES_DeInit                  : Deinit the AES module
        (+) HAL_SEA_AES_Encrypt                 : Start the AES encryption
        (+) HAL_SEA_AES_Decrypt                 : Start the AES decryption
        (+) HAL_SEA_AES_Gmac                    : Start the AES gmac calculation
        (+) HAL_SEA_AES_GF128Mul                : Start the AES Galois Field multiplication
        (+) HAL_SEA_AES_KeyWrapping             : Start the AES keywarping
        (+) HAL_SEA_AES_KeyUnwrapping           : Start the AES keyunwarping
        (+) HAL_SEA_AES_SetAAD                  : Add the AES aad message
        (+) HAL_SEA_AES_LatchProcessStatus      : Latch the current process status
        (+) HAL_SEA_AES_RecoverProcessStatus    : Recover the last process status

@endverbatim
 * @{
 */

/**
 * @brief  128-bit Galois Field multiplication.
 * @param  haes SEA AES-processing instance
 * @param  pDataX Multiplicand X.
 * @param  pDataY Multiplier Y
 * @param  pDataZ Result Z.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_AES_STA_CRYPTKEY_ADDR_ALIGN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_CRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_DECRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_GMAC_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_NUHAL_POINTER_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_MODE_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_INLEN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_ICV_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_AES_GF128Mul(AES_HandleTypeDef *haes, const uint8_t *pDataX, const uint8_t *pDataY, uint8_t *pDataZ)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pAES_GF128Mul func_ptr = (pAES_GF128Mul)((uint32_t)Iap_Func->AES_GF128Mul + ROM_BASE_ADDR);
    return func_ptr(haes, pDataX, pDataY, pDataZ);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_AES_GF128MUL_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)haes;
    api_params[2] = (uint32_t)pDataX;
    api_params[3] = (uint32_t)pDataY;
    api_params[4] = (uint32_t)pDataZ;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 4, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Initialize the AES module and calculation.
 * @param  haes SEA AES-processing instance
 * @param  pConfig AES init instance
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_AES_STA_CRYPTKEY_ADDR_ALIGN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_CRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_DECRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_GMAC_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_NUHAL_POINTER_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_MODE_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_INLEN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_ICV_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_AES_Init(AES_HandleTypeDef *haes, const AES_InitTypeDef *pConfig)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pAES_Init func_ptr = (pAES_Init)((uint32_t)Iap_Func->AES_Init + ROM_BASE_ADDR);
    return func_ptr(haes, pConfig);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_AES_INIT_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)haes;
    api_params[2] = (uint32_t)pConfig;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 2, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  De-Initialize the AES module and calculation.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_AES_STA_CRYPTKEY_ADDR_ALIGN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_CRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_DECRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_GMAC_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_NUHAL_POINTER_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_MODE_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_INLEN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_ICV_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_AES_DeInit(void)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pAES_DeInit func_ptr = (pAES_DeInit)((uint32_t)Iap_Func->AES_DeInit + ROM_BASE_ADDR);
    return func_ptr();
#endif
#ifdef CORE1
    uint32_t api_id = SEA_AES_DEINIT_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 0, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Start AES encrypt calculation.
 * @param  haes SEA AES-processing instance
 * @param  PlainData pointer to plaintext buffer.
 * @param  PlainLen size of plaintext.
 * @param  CipherData pointer to ciphertext buffer.
 * @param  IsLast flag of current package.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_AES_STA_CRYPTKEY_ADDR_ALIGN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_CRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_DECRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_GMAC_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_NUHAL_POINTER_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_MODE_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_INLEN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_ICV_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_AES_Encrypt(AES_HandleTypeDef *haes, const uint8_t *PlainData, uint32_t PlainLen, CYPHER_DATA_STA IsLast, uint8_t *CipherData)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pAES_Encrypt func_ptr = (pAES_Encrypt)((uint32_t)Iap_Func->AES_Encrypt + ROM_BASE_ADDR);
    return func_ptr(haes, PlainData, PlainLen, IsLast, CipherData);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_AES_ENCRYPT_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)haes;
    api_params[2] = (uint32_t)PlainData;
    api_params[3] = PlainLen;
    api_params[4] = (uint32_t)IsLast;
    api_params[5] = (uint32_t)CipherData;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 5, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Start AES decrypt calculation.
 * @param  haes SEA AES-processing instance
 * @param  CipherData pointer to ciphertext buffer.
 * @param  CipherLen size of ciphertext.
 * @param  PlainData pointer to plaintext buffer.
 * @param  IsLast flag of current package.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_AES_STA_CRYPTKEY_ADDR_ALIGN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_CRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_DECRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_GMAC_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_NUHAL_POINTER_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_MODE_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_INLEN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_ICV_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_AES_Decrypt(AES_HandleTypeDef *haes, const uint8_t *CipherData, uint32_t CipherLen, CYPHER_DATA_STA IsLast, uint8_t *PlainData)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pAES_Decrypt func_ptr = (pAES_Decrypt)((uint32_t)Iap_Func->AES_Decrypt + ROM_BASE_ADDR);
    return func_ptr(haes, CipherData, CipherLen, IsLast, PlainData);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_AES_DECRYPT_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)haes;
    api_params[2] = (uint32_t)CipherData;
    api_params[3] = CipherLen;
    api_params[4] = (uint32_t)IsLast;
    api_params[5] = (uint32_t)PlainData;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 5, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Start AES GMAC calculation.
 * @param  haes SEA AES-processing instance
 * @param  GhashData pointer to GHashData.
 * @param  GhashDataLen size of GHashData.
 * @param  IsLast flag of current package.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_AES_STA_CRYPTKEY_ADDR_ALIGN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_CRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_DECRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_GMAC_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_NUHAL_POINTER_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_MODE_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_INLEN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_ICV_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_AES_Gmac(AES_HandleTypeDef *haes, const uint8_t *GhashData, uint32_t GhashDataLen, CYPHER_DATA_STA IsLast)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pAES_Gmac func_ptr = (pAES_Gmac)((uint32_t)Iap_Func->AES_Gmac + ROM_BASE_ADDR);
    return func_ptr(haes, GhashData, GhashDataLen, IsLast);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_AES_GMAC_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)haes;
    api_params[2] = (uint32_t)GhashData;
    api_params[3] = GhashDataLen;
    api_params[4] = (uint32_t)IsLast;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 4, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Save the module state and intermediate variables for this calculation.
 * @param  haes SEA AES-processing instance
 * @param  LatchBuf pointer to latch buffer.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_AES_STA_CRYPTKEY_ADDR_ALIGN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_CRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_DECRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_GMAC_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_NUHAL_POINTER_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_MODE_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_INLEN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_ICV_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_AES_LatchProcessStatus(AES_HandleTypeDef *haes, AES_HandleTypeDef *LatchBuf)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pAES_LatchProcessStatus func_ptr = (pAES_LatchProcessStatus)((uint32_t)Iap_Func->AES_LatchProcessStatus + ROM_BASE_ADDR);
    return func_ptr(haes, LatchBuf);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_AES_LATCH_PROCSTA_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)haes;
    api_params[2] = (uint32_t)LatchBuf;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 2, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Recover the last calculated module state and intermediate variables.
 * @param  RecoverBuf pointer to recover buffer.
 * @param  haes SEA AES-processing instance
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_AES_STA_CRYPTKEY_ADDR_ALIGN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_CRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_DECRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_GMAC_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_NUHAL_POINTER_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_MODE_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_INLEN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_ICV_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_AES_RecoverProcessStatus(AES_HandleTypeDef *RecoverBuf, AES_HandleTypeDef *haes)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pAES_RecoverProcessStatus func_ptr = (pAES_RecoverProcessStatus)((uint32_t)Iap_Func->AES_RecoverProcessStatus + ROM_BASE_ADDR);
    return func_ptr(RecoverBuf, haes);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_AES_RECOVER_PROCSTA_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)RecoverBuf;
    api_params[2] = (uint32_t)haes;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 2, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Start key warpping.
 * @param  haes SEA AES-processing instance
 * @param  PlainData pointer to plaintext buffer.
 * @param  PlainLen size of plaintext.
 * @param  CipherData pointer to ciphertext buffer.
 * @param  CipherLen size of ciphertext.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_AES_STA_CRYPTKEY_ADDR_ALIGN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_CRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_DECRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_GMAC_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_NUHAL_POINTER_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_MODE_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_INLEN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_ICV_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_AES_KeyWrapping(AES_HandleTypeDef *haes, const uint8_t *PlainData, uint32_t PlainLen, uint8_t *CipherData, uint32_t *CipherLen)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pAES_KeyWrapping func_ptr = (pAES_KeyWrapping)((uint32_t)Iap_Func->AES_KeyWrapping + ROM_BASE_ADDR);
    return func_ptr(haes, PlainData, PlainLen, CipherData, CipherLen);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_AES_KEYWRAP_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)haes;
    api_params[2] = (uint32_t)PlainData;
    api_params[3] = PlainLen;
    api_params[4] = (uint32_t)CipherData;
    api_params[5] = (uint32_t)CipherLen;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 5, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Start key upwrapping.
 * @param  haes SEA AES-processing instance
 * @param  CipherData pointer to ciphertext buffer.
 * @param  CipherLen size of ciphertext.
 * @param  PlainData pointer to plaintext buffer.
 * @param  PlainLen size of plaintext.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_AES_STA_CRYPTKEY_ADDR_ALIGN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_CRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_DECRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_GMAC_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_NUHAL_POINTER_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_MODE_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_INLEN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_ICV_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_AES_KeyUnwrapping(AES_HandleTypeDef *haes, const uint8_t *CipherData, uint32_t CipherLen, uint8_t *PlainData, uint32_t *PlainLen)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pAES_KeyUnwrapping func_ptr = (pAES_KeyUnwrapping)((uint32_t)Iap_Func->AES_KeyUnwrapping + ROM_BASE_ADDR);
    return func_ptr(haes, CipherData, CipherLen, PlainData, PlainLen);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_AES_KEYUNWARP_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)haes;
    api_params[2] = (uint32_t)CipherData;
    api_params[3] = CipherLen;
    api_params[4] = (uint32_t)PlainData;
    api_params[5] = (uint32_t)PlainLen;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 5, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Specify additional authenticated data.
 * @param  haes SEA AES-processing instance
 * @param  DataIn pointer to aad.
 * @param  DataLen size of aad.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_AES_STA_CRYPTKEY_ADDR_ALIGN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_CRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_DECRYPT_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_GMAC_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_NUHAL_POINTER_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_MODE_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_INLEN_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_ICV_FAILURE
 *         @arg @ref HAL_SEA_AES_STA_KW_CHECK_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_AES_SetAAD(AES_HandleTypeDef *haes, const uint8_t *DataIn, uint32_t DataLen)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pAES_SetAAD func_ptr = (pAES_SetAAD)((uint32_t)Iap_Func->AES_SetAAD + ROM_BASE_ADDR);
    return func_ptr(haes, DataIn, DataLen);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_AES_SET_AAD_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)haes;
    api_params[2] = (uint32_t)DataIn;
    api_params[3] = DataLen;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 3, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}
/**
 * @}
 */

/** @defgroup SEA_Exported_Functions_Group2 HASH IO operation functions
 * @brief     HASH IO operation functions
 *
@verbatim
================================================================================
                    ##### HASH IO operation functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) HAL_SEA_HASH_Init                   : Init the HASH module
        (+) HAL_SEA_HASH_DeInit                 : Deinit the HASH module
        (+) HAL_SEA_HASH                        : Start the HASH calculation
        (+) HAL_SEA_HASH_LatchProcessStatus     : Latch the current process status
        (+) HAL_SEA_HASH_RecoverProcessStatus   : Recover the last process status

@endverbatim
 * @{
 */

/**
 * @brief  Initialize the HASH module.
 * @param  hhash SEA HASH-processing instance
 * @param  pConfig HASH init instance
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_SHA_STA_DATA_ALIGN_FAILURE
 *         @arg @ref HAL_SEA_SHA_STA_DATASIGN_FAILURE
 *         @arg @ref HAL_SEA_SHA_STA_DMA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_SHA_STA_DMA_DATA_TRANS_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_HASH_Init(HASH_HandleTypeDef *hhash, const HASH_InitTypeDef *pConfig)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pHASH_Init func_ptr = (pHASH_Init)((uint32_t)Iap_Func->HASH_Init + ROM_BASE_ADDR);
    return func_ptr(hhash, pConfig);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_HASH_INIT_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hhash;
    api_params[2] = (uint32_t)pConfig;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 2, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  De-Initialize the HASH module.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SHA_STA_DATA_ALIGN_FAILURE
 *         @arg @ref HAL_SHA_STA_DATASIGN_FAILURE
 *         @arg @ref HAL_SHA_STA_DMA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SHA_STA_DMA_DATA_TRANS_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_HASH_DeInit(void)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pHASH_DeInit func_ptr = (pHASH_DeInit)((uint32_t)Iap_Func->HASH_DeInit + ROM_BASE_ADDR);
    return func_ptr();
#endif
#ifdef CORE1
    uint32_t api_id = SEA_HASH_DEINIT_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 0, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Do HASH calculation.
 * @param  hhash SEA HASH-processing instance
 * @param  Input pointer to input data buffer.
 * @param  InputLen size of input data.
 * @param  IsLast flag of current package.
 * @param  Output pointer to output data buffer.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SHA_STA_DATA_ALIGN_FAILURE
 *         @arg @ref HAL_SHA_STA_DATASIGN_FAILURE
 *         @arg @ref HAL_SHA_STA_DMA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SHA_STA_DMA_DATA_TRANS_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_HASH(HASH_HandleTypeDef *hhash, const uint8_t *Input, uint32_t InputLen, CYPHER_DATA_STA IsLast, uint8_t *Output)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pHASH func_ptr = (pHASH)((uint32_t)Iap_Func->HASH + ROM_BASE_ADDR);
    return func_ptr(hhash, Input, InputLen, IsLast, Output);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_HASH_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hhash;
    api_params[2] = (uint32_t)Input;
    api_params[3] = InputLen;
    api_params[4] = (uint32_t)IsLast;
    api_params[5] = (uint32_t)Output;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 5, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Latch the current process status of the Hash.
 * @param  hhash SEA HASH-processing instance
 * @param  LatchBuf pointer to latch buffer.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SHA_STA_DATA_ALIGN_FAILURE
 *         @arg @ref HAL_SHA_STA_DATASIGN_FAILURE
 *         @arg @ref HAL_SHA_STA_DMA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SHA_STA_DMA_DATA_TRANS_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_HASH_LatchProcessStatus(const HASH_HandleTypeDef *hhash, HASH_HandleTypeDef *LatchBuf)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pHASH_LatchProcessStatus func_ptr = (pHASH_LatchProcessStatus)((uint32_t)Iap_Func->HASH_LatchProcessStatus + ROM_BASE_ADDR);
    return func_ptr(hhash, LatchBuf);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_HASH_LATCH_PROCSTA_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hhash;
    api_params[2] = (uint32_t)LatchBuf;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 2, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Recover the Hash process status use the latch parameters before.
 * @param  RecoverBuf pointer to recover buffer.
 * @param  hhash SEA HASH-processing instance
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SHA_STA_DATA_ALIGN_FAILURE
 *         @arg @ref HAL_SHA_STA_DATASIGN_FAILURE
 *         @arg @ref HAL_SHA_STA_DMA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SHA_STA_DMA_DATA_TRANS_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_HASH_RecoverProcessStatus(const HASH_HandleTypeDef *RecoverBuf, HASH_HandleTypeDef *hhash)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pHASH_RecoverProcessStatus func_ptr = (pHASH_RecoverProcessStatus)((uint32_t)Iap_Func->HASH_RecoverProcessStatus + ROM_BASE_ADDR);
    return func_ptr(RecoverBuf, hhash);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_HASH_RECOVER_PROCSTA_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)RecoverBuf;
    api_params[2] = (uint32_t)hhash;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 2, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}
/**
 * @}
 */

/** @defgroup SEA_Exported_Functions_Group3 ECC IO operation functions
 * @brief     ECC IO operation functions
 *
@verbatim
================================================================================
                    ##### ECC IO operation functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) HAL_SEA_ECC_GenerateKey                 : Generate ecc key pairs
        (+) HAL_SEA_ECC_GeneratePublicKey           : Generate ecc public key
        (+) HAL_SEA_ECC_GenerateUncompPublicKey     : Generate ecc uncompressed public key
        (+) HAL_SEA_ECC_VerifyPublicKey             : Verify ecc public key
        (+) HAL_SEA_ECC_GenerateDHKeyPairs          : Generate DHKey pairs
        (+) HAL_SEA_ECC_GenerateDHKeyX              : Generate DHKey x
        (+) HAL_SEA_ECC_GenerateSignature           : Generate sign
        (+) HAL_SEA_ECC_GenerateSignaturewithPKey   : Generate sign with private key
        (+) HAL_SEA_ECC_VerifySignature             : Verify sign
        (+) HAL_SEA_ECC_GenerateKeySecurely         : Generate key and save it to keystore
        (+) HAL_SEA_ECC_GenerateSignatureSecurely   : Generate sign using keystore
        (+) HAL_SEA_ECC_VerifySignatureSecurely     : Verify sign using keystore

@endverbatim
 * @{
 */

/**
 * @brief  Generating ECC private key and public key pairs.
 * @param  hecc SEA ECC-processing instance
 * @param  Q pointer to public key buffer.
 * @param  K pointer to private key buffer.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_ECC_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_LOOPTIMETOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateKey(const ECC_HandleTypeDef *hecc, uint8_t *Q, uint8_t *K)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pECC_GenerateKey func_ptr = (pECC_GenerateKey)((uint32_t)Iap_Func->ECC_GenerateKey + ROM_BASE_ADDR);
    return func_ptr(hecc, Q, K);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_ECC_GEN_KEY_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hecc;
    api_params[2] = (uint32_t)Q;
    api_params[3] = (uint32_t)K;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 3, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Generating ECC public key pairs by using private key.
 * @param  hecc SEA ECC-processing instance
 * @param  Q pointer to public key buffer.
 * @param  K pointer to private key buffer.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_ECC_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_LOOPTIMETOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GeneratePublicKey(const ECC_HandleTypeDef *hecc, uint8_t *Q, const uint8_t *K)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pECC_GeneratePublicKey func_ptr = (pECC_GeneratePublicKey)((uint32_t)Iap_Func->ECC_GeneratePublicKey + ROM_BASE_ADDR);
    return func_ptr(hecc, Q, K);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_ECC_GEN_PUBKEY_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hecc;
    api_params[2] = (uint32_t)Q;
    api_params[3] = (uint32_t)K;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 3, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Generating uncompressed public key pairs using private key. Reference Specification RFC-5480.
 * @param  hecc SEA ECC-processing instance
 * @param  Q pointer to public key buffer.
 * @param  K pointer to private key buffer.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_ECC_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_LOOPTIMETOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateUncompPublicKey(const ECC_HandleTypeDef *hecc, uint8_t *Q, const uint8_t *K)
{
    uint32_t Q_tmp[34] = {0};
    uint32_t ret = HAL_SEA_ECC_GeneratePublicKey(hecc, (uint8_t *)Q_tmp, K);
    for (int i = 0; i < hecc->NumBytes; i++) {
        Q[i + 1] = ((uint8_t*)Q_tmp)[i];
    }
    *Q = 0x04U;
    return ret;
}

/**
 * @brief  Checking ECC public key pairs is valid.
 * @param  hecc SEA ECC-processing instance
 * @param  Q pointer to public key buffer.
 * @param  pFlag verify result.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_ECC_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_LOOPTIMETOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_ECC_VerifyPublicKey(const ECC_HandleTypeDef *hecc, const uint8_t *Q, HAL_SEA_FlagTypeDef *pFlag)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pECC_VerifyPublicKey func_ptr = (pECC_VerifyPublicKey)((uint32_t)Iap_Func->ECC_VerifyPublicKey + ROM_BASE_ADDR);
    return func_ptr(hecc, Q, pFlag);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_ECC_VERIFY_PUBKEY_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hecc;
    api_params[2] = (uint32_t)Q;
    api_params[3] = (uint32_t)pFlag;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 3, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  The (x,y) is used as the basis for ECDH. Reference Specification NIST SP 800-56A and SEC 1.
 * @param  hecc SEA ECC-processing instance
 * @param  Q pointer to public key buffer.
 * @param  K pointer to private key buffer.
 * @param  DHKey pointer to DHKey buffer.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_ECC_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_LOOPTIMETOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateDHKeyPairs(const ECC_HandleTypeDef *hecc, const uint8_t *Q, const uint8_t *K, uint8_t *DHKey)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pECC_GenerateDHKeyPairs func_ptr = (pECC_GenerateDHKeyPairs)((uint32_t)Iap_Func->ECC_GenerateDHKeyPairs + ROM_BASE_ADDR);
    return func_ptr(hecc, Q, K, DHKey);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_ECC_GEN_DHKEY_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hecc;
    api_params[2] = (uint32_t)Q;
    api_params[3] = (uint32_t)K;
    api_params[4] = (uint32_t)DHKey;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 4, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  The x coordinate is used as the basis for ECDH. Reference Specification NIST SP 800-56A and SEC 1.
 * @param  hecc SEA ECC-processing instance
 * @param  Q pointer to public key buffer.
 * @param  K pointer to private key buffer.
 * @param  DHKeyX pointer to DHKey X coordinate buffer.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_ECC_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_LOOPTIMETOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateDHKeyX(const ECC_HandleTypeDef *hecc, const uint8_t *Q, const uint8_t *K, uint8_t *DHKeyX)
{
    uint32_t DHKey_tmp[34] = {0};
    uint32_t ret = HAL_SEA_ECC_GenerateDHKeyPairs(hecc, Q, K, (uint8_t *)DHKey_tmp);
    for (int i = 0; i < hecc->NumBytes; i++) {
        DHKeyX[i] = ((uint8_t*)DHKey_tmp)[i];
    }
    return ret;
}

/**
 * @brief  Generating public key pairs, private key and corresponding signatures based on the input hash result.
 * @param  hecc SEA ECC-processing instance
 * @param  HashMessage pointer to Hash result buffer.
 * @param  MessageLen size of Hash result.
 * @param  Q pointer to public key buffer.
 * @param  K pointer to private key buffer.
 * @param  R pointer to r of the signature pairs.
 * @param  S pointer to s of the signature pairs.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_ECC_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_LOOPTIMETOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateSignature(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, uint8_t *Q, uint8_t *K, uint8_t *R, uint8_t *S)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pECC_GenerateSignature func_ptr = (pECC_GenerateSignature)((uint32_t)Iap_Func->ECC_GenerateSignature + ROM_BASE_ADDR);
    return func_ptr(hecc, HashMessage, MessageLen, Q, K, R, S);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_ECC_GEN_SIGN_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hecc;
    api_params[2] = (uint32_t)HashMessage;
    api_params[3] = MessageLen;
    api_params[4] = (uint32_t)Q;
    api_params[5] = (uint32_t)K;
    api_params[6] = (uint32_t)R;
    api_params[7] = (uint32_t)S;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 7, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Generating public key pairs if the pointer of Q is not NULL, and corresponding signatures based on the input hash result.
 * @param  hecc SEA ECC-processing instance
 * @param  HashMessage pointer to Hash result buffer.
 * @param  MessageLen size of Hash result.
 * @param  Q pointer to public key buffer.
 * @param  K pointer to private key buffer.
 * @param  R pointer to r of the signature pairs.
 * @param  S pointer to s of the signature pairs.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_ECC_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_LOOPTIMETOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateSignaturewithPKey(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, uint8_t *Q, const uint8_t *K, uint8_t *R, uint8_t *S)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pECC_GenerateSignaturewithPKey func_ptr = (pECC_GenerateSignaturewithPKey)((uint32_t)Iap_Func->ECC_GenerateSignaturewithPKey + ROM_BASE_ADDR);
    return func_ptr(hecc, HashMessage, MessageLen, Q, K, R, S);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_ECC_GEN_SIGN_PUBKEY_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hecc;
    api_params[2] = (uint32_t)HashMessage;
    api_params[3] = MessageLen;
    api_params[4] = (uint32_t)Q;
    api_params[5] = (uint32_t)K;
    api_params[6] = (uint32_t)R;
    api_params[7] = (uint32_t)S;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 7, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Verifying the input signature by using public key based on the input hash result.
 * @param  hecc SEA ECC-processing instance
 * @param  HashMessage pointer to Hash result buffer.
 * @param  MessageLen size of Hash result.
 * @param  Q pointer to public key buffer.
 * @param  R pointer to r of the signature pairs.
 * @param  S pointer to s of the signature pairs.
 * @param  pFlag
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_ECC_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_LOOPTIMETOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_ECC_VerifySignature(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t *Q, const uint8_t *R, const uint8_t *S, HAL_SEA_FlagTypeDef *pFlag)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pECC_VerifySignature func_ptr = (pECC_VerifySignature)((uint32_t)Iap_Func->ECC_VerifySignature + ROM_BASE_ADDR);
    return func_ptr(hecc, HashMessage, MessageLen, Q, R, S, pFlag);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_ECC_VERIFY_SIGN_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hecc;
    api_params[2] = (uint32_t)HashMessage;
    api_params[3] = MessageLen;
    api_params[4] = (uint32_t)Q;
    api_params[5] = (uint32_t)R;
    api_params[6] = (uint32_t)S;
    api_params[7] = (uint32_t)pFlag;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 7, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Generating public key pairs and private key, and save those into keystore.
 * @param  hecc SEA ECC-processing instance
 * @param  PKeyIdx index of private key.
 * @param  QxIdx index of Qx.
 * @param  QyIdx index of Qy.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_ECC_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_LOOPTIMETOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateKeySecurely(const ECC_HandleTypeDef *hecc, uint8_t PKeyIdx, uint8_t QxIdx, uint8_t QyIdx)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pECC_GenerateKeySecurely func_ptr = (pECC_GenerateKeySecurely)((uint32_t)Iap_Func->ECC_GenerateKeySecurely + ROM_BASE_ADDR);
    return func_ptr(hecc, PKeyIdx, QxIdx, QyIdx);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_ECC_GEN_KEY_SEC_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hecc;
    api_params[2] = (uint32_t)PKeyIdx;
    api_params[3] = (uint32_t)QxIdx;
    api_params[4] = (uint32_t)QyIdx;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 4, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Generating corresponding signatures with private key from keystore.
 * @param  hecc SEA ECC-processing instance
 * @param  HashMessage pointer to Hash result buffer.
 * @param  MessageLen size of Hash result.
 * @param  PKeyIdx index of private key.
 * @param  R pointer to r of the signature pairs.
 * @param  S pointer to s of the signature pairs.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_ECC_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_LOOPTIMETOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateSignatureSecurely(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t PKeyIdx, uint8_t *R, uint8_t *S)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pECC_GenerateSignatureSecurely func_ptr = (pECC_GenerateSignatureSecurely)((uint32_t)Iap_Func->ECC_GenerateSignatureSecurely + ROM_BASE_ADDR);
    return func_ptr(hecc, HashMessage, MessageLen, PKeyIdx, R, S);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_ECC_GEN_SIGN_SEC_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hecc;
    api_params[2] = (uint32_t)HashMessage;
    api_params[3] = MessageLen;
    api_params[4] = (uint32_t)PKeyIdx;
    api_params[5] = (uint32_t)R;
    api_params[6] = (uint32_t)S;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 6, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Verifying corresponding signatures with public key pairs from keystore.
 * @param  hecc SEA ECC-processing instance
 * @param  HashMessage pointer to Hash result buffer.
 * @param  MessageLen size of Hash result.
 * @param  QxIdx index of Qx.
 * @param  QyIdx index of Qy.
 * @param  R pointer to r of the signature pairs.
 * @param  S pointer to s of the signature pairs.
 * @param  pFlag verify result.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_ECC_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_LOOPTIMETOUT_FAILURE
 *         @arg @ref HAL_SEA_ECC_STA_PKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_ECC_VerifySignatureSecurely(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t QxIdx, const uint8_t QyIdx, const uint8_t *R, const uint8_t *S, HAL_SEA_FlagTypeDef *pFlag)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pECC_VerifySignatureSecurely func_ptr = (pECC_VerifySignatureSecurely)((uint32_t)Iap_Func->ECC_VerifySignatureSecurely + ROM_BASE_ADDR);
    return func_ptr(hecc, HashMessage, MessageLen, QxIdx, QyIdx, R, S, pFlag);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_ECC_VERIFY_SIGN_SEC_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hecc;
    api_params[2] = (uint32_t)HashMessage;
    api_params[3] = MessageLen;
    api_params[4] = (uint32_t)QxIdx;
    api_params[5] = (uint32_t)QyIdx;
    api_params[6] = (uint32_t)R;
    api_params[7] = (uint32_t)S;
    api_params[8] = (uint32_t)pFlag;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 8, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @}
 */

/** @defgroup SEA_Exported_Functions_Group4 RSA IO operation functions
 * @brief     RSA IO operation functions
 *
@verbatim
================================================================================
                    ##### RSA IO operation functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) HAL_SEA_RSA_GenerateKey                 : Generate rsa key
        (+) HAL_SEA_RSA_GenerateSignature           : Generate sign
        (+) HAL_SEA_RSA_VerifySignature             : Verify sign
        (+) HAL_SEA_RSA_EncryptwithPublicKey        : Start the RSA encryption with public key
        (+) HAL_SEA_RSA_DecryptwithPublicKey        : Start the RSA decryption with public key
        (+) HAL_SEA_RSA_EncryptwithPrivateKey       : Start the RSA encryption with private key
        (+) HAL_SEA_RSA_DecryptwithPrivateKey       : Start the RSA decryption with private key

@endverbatim
 * @{
 */

/**
 * @brief  Generate RSA key pairs.
 * @param  hrsa SEA RSA-processing instance
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_RSA_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_KEY_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_RSA_GenerateKey(RSA_HandleTypeDef *hrsa)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pRSA_GenerateKey func_ptr = (pRSA_GenerateKey)((uint32_t)Iap_Func->RSA_GenerateKey + ROM_BASE_ADDR);
    return func_ptr(hrsa);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_RSA_GEN_KEY_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hrsa;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 1, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Generating corresponding signatures.
 * @param  hrsa SEA RSA-processing instance
 * @param  HashMessage pointer to Hash result buffer.
 * @param  MessageLen size of Hash result.
 * @param  Signature pointer to Signature buffer.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_RSA_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_KEY_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_RSA_GenerateSignature(RSA_HandleTypeDef *hrsa, const uint32_t *HashMessage, uint32_t MessageLen, uint32_t *Signature)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pRSA_GenerateSignature func_ptr = (pRSA_GenerateSignature)((uint32_t)Iap_Func->RSA_GenerateSignature + ROM_BASE_ADDR);
    return func_ptr(hrsa, HashMessage, MessageLen, Signature);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_RSA_GEN_SIGN_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hrsa;
    api_params[2] = (uint32_t)HashMessage;
    api_params[3] = MessageLen;
    api_params[4] = (uint32_t)Signature;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 4, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Verifying corresponding signatures.
 * @param  hrsa SEA RSA-processing instance
 * @param  HashMessage pointer to Hash result buffer.
 * @param  MessageLen size of Hash result.
 * @param  Signature pointer to Signature buffer.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_RSA_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_KEY_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_RSA_VerifySignature(RSA_HandleTypeDef *hrsa, const uint32_t *HashMessage, uint32_t MessageLen, uint32_t *Signature)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pRSA_VerifySignature func_ptr = (pRSA_VerifySignature)((uint32_t)Iap_Func->RSA_VerifySignature + ROM_BASE_ADDR);
    return func_ptr(hrsa, HashMessage, MessageLen, Signature);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_RSA_VERIFY_SIGN_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hrsa;
    api_params[2] = (uint32_t)HashMessage;
    api_params[3] = MessageLen;
    api_params[4] = (uint32_t)Signature;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 4, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Encrypt input data by public key.
 * @param  hrsa SEA RSA-processing instance
 * @param  PlainData pointer to plaintext buffer.
 * @param  PlainLen size of plaintext.
 * @param  CipherData pointer to ciphertext buffer.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_RSA_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_KEY_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_RSA_EncryptwithPublicKey(RSA_HandleTypeDef *hrsa, const uint32_t *PlainData, uint32_t PlainLen, uint32_t *CipherData)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pRSA_EncryptwithPublicKey func_ptr = (pRSA_EncryptwithPublicKey)((uint32_t)Iap_Func->RSA_EncryptwithPublicKey + ROM_BASE_ADDR);
    return func_ptr(hrsa, PlainData, PlainLen, CipherData);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_RSA_ENC_PUBKEY_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hrsa;
    api_params[2] = (uint32_t)PlainData;
    api_params[3] = PlainLen;
    api_params[4] = (uint32_t)CipherData;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 4, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Decrypt input data by public key.
 * @param  hrsa SEA RSA-processing instance
 * @param  CipherData pointer to ciphertext buffer.
 * @param  CipherLen size of ciphertext.
 * @param  PlainData pointer to plaintext buffer.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_RSA_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_KEY_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_RSA_DecryptwithPublicKey(RSA_HandleTypeDef *hrsa, const uint32_t *CipherData, uint32_t CipherLen, uint32_t *PlainData)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pRSA_DecryptwithPublicKey func_ptr = (pRSA_DecryptwithPublicKey)((uint32_t)Iap_Func->RSA_DecryptwithPublicKey + ROM_BASE_ADDR);
    return func_ptr(hrsa, CipherData, CipherLen, PlainData);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_RSA_DEC_PUBKEY_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hrsa;
    api_params[2] = (uint32_t)CipherData;
    api_params[3] = CipherLen;
    api_params[4] = (uint32_t)PlainData;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 4, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Encrypt input data by private key.
 * @param  hrsa SEA RSA-processing instance
 * @param  PlainData pointer to plaintext buffer.
 * @param  PlainLen size of plaintext.
 * @param  CipherData pointer to ciphertext buffer.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_RSA_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_KEY_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_RSA_EncryptwithPrivateKey(RSA_HandleTypeDef *hrsa, const uint32_t *PlainData, uint32_t PlainLen, uint32_t *CipherData)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pRSA_EncryptwithPrivateKey func_ptr = (pRSA_EncryptwithPrivateKey)((uint32_t)Iap_Func->RSA_EncryptwithPrivateKey + ROM_BASE_ADDR);
    return func_ptr(hrsa, PlainData, PlainLen, CipherData);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_RSA_ENC_PRIKEY_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hrsa;
    api_params[2] = (uint32_t)PlainData;
    api_params[3] = PlainLen;
    api_params[4] = (uint32_t)CipherData;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 4, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Decrypt input data by private key.
 * @param  hrsa SEA RSA-processing instance
 * @param  CipherData pointer to ciphertext buffer.
 * @param  CipherLen size of ciphertext.
 * @param  PlainData pointer to plaintext buffer.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_RSA_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_PARASLEN_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_KEY_FAILURE
 *         @arg @ref HAL_SEA_RSA_STA_PADDING_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_RSA_DecryptwithPrivateKey(RSA_HandleTypeDef *hrsa, const uint32_t *CipherData, uint32_t CipherLen, uint32_t *PlainData)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pRSA_DecryptwithPrivateKey func_ptr = (pRSA_DecryptwithPrivateKey)((uint32_t)Iap_Func->RSA_DecryptwithPrivateKey + ROM_BASE_ADDR);
    return func_ptr(hrsa, CipherData, CipherLen, PlainData);
#endif
#ifdef CORE1
    uint32_t api_id = SEA_RSA_DEC_PRIKEY_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)hrsa;
    api_params[2] = (uint32_t)CipherData;
    api_params[3] = CipherLen;
    api_params[4] = (uint32_t)PlainData;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 4, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @}
 */

/** @defgroup SEA_Exported_Functions_Group5 TRNG IO operation functions
 * @brief     TRNG IO operation functions
 *
@verbatim
================================================================================
                    ##### TRNG IO operation functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) HAL_SEA_TRNG_GenerateRandom              : Generate random number
        (+) HAL_SEA_TRNG_GenerateKey                 : Generate random key

@endverbatim
 * @{
 */

/**
 * @brief  Generating random data.
 * @param  RandomData pointer to random data buffer.
 * @param  DataLen specified random data length.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 */
HAL_SEA_StatusTypeDef HAL_SEA_TRNG_GenerateRandom(uint32_t *RandomData, uint8_t DataLen)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_SEA_STA_ERROR;
    }
    HAL_SEA_ApiTypeDef *Iap_Func = (HAL_SEA_ApiTypeDef *)iap_addr;
    pTRNG_GenerateRandom func_ptr = (pTRNG_GenerateRandom)((uint32_t)Iap_Func->TRNG_GenerateRandom + ROM_BASE_ADDR);
    func_ptr(RandomData, DataLen);
    return HAL_SEA_STA_SUCCESS;
#endif
#ifdef CORE1
    uint32_t api_id = SEA_TRNG_GEN_RANDOM_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)RandomData;
    api_params[2] = (uint32_t)DataLen;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 2, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_SEA_StatusTypeDef)result;
    }

    return (HAL_SEA_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Generating random key and save it to keystore.
 * @param  KEYSTORE_InitStruct KeyStore init instance
 * @param  KeyIdx key index in KeyStore.
 * @retval This parameter can be one of the following values:
 *         @arg @ref HAL_SEA_STA_SUCCESS
 *         @arg @ref HAL_SEA_STA_ERROR
 *         @arg @ref HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE
 *         @arg @ref HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KEYGEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_KLEN_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_SEA_TRNG_STA_RANDOM_FAILURE
 *         @arg @ref HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE
 */
HAL_SEA_StatusTypeDef HAL_SEA_TRNG_GenerateKey(KEYSTORE_InitTypeDef *KEYSTORE_InitStruct, uint8_t KeyIdx)
{
    uint32_t key[18] = {0}, keyword = 0U;
    keyword = ((0U != (KEYSTORE_InitStruct->KeyLen & 0x1FU)) \
                ? ((KEYSTORE_InitStruct->KeyLen >> 5U) + 1U) \
                : (KEYSTORE_InitStruct->KeyLen >> 5U));
    HAL_SEA_TRNG_GenerateRandom(key, keyword);
    if (HAL_KEYSTORE_STA_SUCCESS != HAL_KEYSTORE_WriteKey(KEYSTORE_InitStruct, key, KeyIdx)) {
        return HAL_SEA_STA_ERROR;
    } else {
        return HAL_SEA_STA_SUCCESS;
    }
}

/**
 * @}
 */

/** @defgroup SEA_Exported_Functions_Group6 State functions
 * @brief     State functions
 *
@verbatim
================================================================================
                    ##### State functions #####
================================================================================
    [..]  This subsection provides functions allowing to :
        (+) HAL_SEA_GetState()  : Return the SEA state.

@endverbatim
 * @{
 */

/**
 * @brief  Return the SEA peripheral state.
 *
 * @retval Between 0 to 0xFFF.
 */
uint32_t HAL_SEA_GetState(void)
{
    return READ_REG(SEA->STA);
}

/**
 * @}
 */

/**
 * @}
 */

#endif /* HAL_SEA_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
