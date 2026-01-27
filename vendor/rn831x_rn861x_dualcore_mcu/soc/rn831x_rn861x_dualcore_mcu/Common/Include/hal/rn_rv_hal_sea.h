/**
 ******************************************************************************
 * @file   rn_rv_hal_sea.h
 * @author Renergy Software Team
 * @brief  Header file of SEA HAL module.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RN_RV_HAL_SEA_H
#define __RN_RV_HAL_SEA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal_def.h"
#include "rn831x_rn861x_mcu_sea.h"
#include "rn_rv_hal_aes.h"
#include "rn_rv_hal_keystore.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup SEA
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup SEA_Exported_Types SEA Exported Types
 * @{
 */

/**
 * @brief SHA output length in words.
 */
#define NOUT_SHA_1                     (5U)
#define NOUT_SHA_224                   (7U)
#define NOUT_SHA_256                   (8U)
#define NOUT_SHA_384                   (12U)
#define NOUT_SHA_512                   (16U)
#define NOUT_SHA_MAX                   (NOUT_SHA_512)

/**
 * @brief SHA IV length in words.
 */
#define NIV_SHA_160                    (5U)
#define NIV_SHA_224_256                (8U)
#define NIV_SHA_384_256                (16U)

/**
 * @brief HASH IV length in words.
 */
#define HASH_SHA_160_256_IV_BSIZE      (16U * NB)
#define HASH_SHA_384_512_IV_BSIZE      (32U * NB)

/**
 * @brief SHA Number of 32-bit words comprising the block data.
 */
#define NBLOCK_DATA_SHA_160_256        (16U)
#define NBLOCK_DATA_SHA_384_512        (32U)
#define NBLOCK_DATA_SHA_160_256_BSIZE  (4U * NBLOCK_DATA_SHA_160_256)
#define NBLOCK_DATA_SHA_384_512_BSIZE  (4U * NBLOCK_DATA_SHA_384_512)
#define NBLOCK_DATA_SHA_MAX            (uint8_t)(NBLOCK_DATA_SHA_384_512)

/**
 * @brief SHA Number of byte comprising the block data shift.
 */
#define BSHIFT_BLOCK_DATA_SHA_160_256  (6U)
#define BSHIFT_BLOCK_DATA_SHA_384_512  (7U)

/**
 * @brief HASH init configuration.
 */
typedef struct
{
    uint8_t  bitWideMode;
    uint8_t  runningMode;
    uint8_t  reserved[2];
} HASH_InitTypeDef;

/**
 * @brief HASH Mode defination.
 */
typedef enum __attribute__((__packed__)) {
    SEA_SHA_1   = 0U,
    SEA_SHA_224 = 1U,
    SEA_SHA_256 = 2U,
    SEA_SHA_NC3 = 3U,
    SEA_SHA_384 = 4U,
    SEA_SHA_512 = 5U,
} SEA_SHA_MODE;

typedef enum __attribute__((__packed__)) {
    HASH_GENERAL_MODE_EN = 0U,
    HASH_DMA_MODE_EN     = 1U,
} HASH_RUNNING_MODE;

typedef enum __attribute__((__packed__)) {
    ECC_GENERAL_MODE_EN  = 0U,
    ECC_SECURE_MODE_EN   = 1U,
} ECC_RUNNING_MODE;

/**
 * @brief HASH engine parameters defination.
 */
typedef struct
{
    uint32_t         *pIv;
    uint8_t           ivLen;
    uint8_t           blockSize;
    uint8_t           blockShift;
    uint8_t           MsgAddLenSize;
    uint8_t           dataOutLen;
    uint8_t           ctr_ir;
    uint8_t           is_start;
    uint8_t           is_last;
    uint32_t          messageSize;
    AES_KernelTypeDef kernelRegion;
    uint8_t           dma_enable;
    uint8_t           reserved0[2];
} hashEngineParameters;

/**
 * @brief HASH datain parameters defination.
 */
typedef struct
{
    uint32_t datas[NBLOCK_DATA_SHA_MAX];
    uint8_t  size_last;
    uint8_t  reserved[3];
} SEA_HASH_DATAIN_MANAGE;

/**
 * @brief SHA block datatype defination.
 */
typedef enum __attribute__((__packed__))
{
    SHA_MSG_ONLY            = 0U,
    SHA_MSG_FILL_ADD        = 1U,
    SHA_MSG_FILL            = 2U,
    SHA_ZERO_ADD            = 3U
} SHA_BLOCK_DATA_TPYE;

/**
 * @brief HASH other parameters defination.
 */
typedef struct
{
    uint32_t               DATA[NOUT_SHA_MAX];
    SEA_HASH_DATAIN_MANAGE datain;
    SHA_BLOCK_DATA_TPYE    block_type;
    uint8_t                last_block;
} SEA_HASH_G_PARAS;

/**
 * @brief HASH parameters defination.
 */
typedef struct
{
    HASH_InitTypeDef       Config;
    hashEngineParameters   Engine;
    SEA_HASH_G_PARAS       Process;
} HASH_HandleTypeDef;

/**
 * @brief ECC parameters defination.
 */
typedef struct
{
    uint8_t  NumWords; /*!< ECC curve bit width is rounded up in words.*/
    uint8_t  NumBytes; /*!< ECC curve bit width is rounded up in bytes.*/
    uint16_t NumBits;  /*!< ECC curve bit width.*/
    uint32_t p[17];    /*!< ECC curve modulus.*/
    uint32_t n[17];    /*!< ECC curve rank.*/
    uint32_t a[17];    /*!< ECC curve Weierstrass Equation coefficient a.*/
    uint32_t b[17];    /*!< ECC curve Weierstrass Equation coefficient b.*/
    uint32_t G[34];    /*!< ECC curve base point.*/
} ECC_HandleTypeDef;

/**
 * @brief RSA parameters defination.
 */
typedef struct
{
    uint32_t KeyLen;   /*!< The key length of RSA.*/
    uint32_t nLen;     /*!< The bit length of modulus N.*/
    uint32_t eLen;     /*!< The bit length of public exponent E.*/
    uint32_t dLen;     /*!< The bit length of private exponent D.*/
    uint32_t n[32];    /*!< The RSA modulus.*/
    uint32_t e[32];    /*!< The RSA public exponent.*/
    uint32_t d[32];    /*!< The RSA private exponent.*/
} RSA_HandleTypeDef;

/**
 * @brief SEA common flag defination.
 */
typedef enum __attribute__((__packed__))
{
    PASS = 0U,
    FAIL = 1U,
} HAL_SEA_FlagTypeDef;

/**
 * @brief SEA running status.
 */
typedef enum __attribute__((__packed__))
{
    HAL_SEA_STA_SUCCESS                         = 0U,   /*!< Success state */
    HAL_SEA_STA_ERROR                           = 1U,   /*!< Error state */
    HAL_SEA_AES_STA_CRYPTKEY_ADDR_ALIGN_FAILURE = 2U,   /*!< AES cryption address aligned invalid */
    HAL_SEA_AES_STA_CRYPT_FAILURE               = 3U,   /*!< AES encryption error */
    HAL_SEA_AES_STA_DECRYPT_FAILURE             = 4U,   /*!< AES decryption error */
    HAL_SEA_AES_STA_GMAC_FAILURE                = 5U,   /*!< AES gmac error */
    HAL_SEA_AES_STA_NULL_POINTER_FAILURE        = 6U,   /*!< Null pointer invalid */
    HAL_SEA_AES_STA_KW_MODE_FAILURE             = 7U,   /*!< AES keywraping mode invalid */
    HAL_SEA_AES_STA_KW_INLEN_FAILURE            = 8U,   /*!< AES keywraping inlen invalid */
    HAL_SEA_AES_STA_KW_CHECK_ICV_FAILURE        = 9U,   /*!< AES keywraping icv check invalid */
    HAL_SEA_AES_STA_KW_CHECK_PADDING_FAILURE    = 10U,  /*!< AES keywraping padding check invalid */
    HAL_SEA_SHA_STA_DATA_ALIGN_FAILURE          = 11U,  /*!< HASH data aligned invalid */
    HAL_SEA_SHA_STA_DATASIGN_FAILURE            = 12U,  /*!< HASH data sign invalid */
    HAL_SEA_SHA_STA_DMA_TIMEOUT_FAILURE         = 13U,  /*!< HASH DMA timeout */
    HAL_SEA_SHA_STA_DMA_DATA_TRANS_FAILURE      = 14U,  /*!< HASH DMA data transmit invalid */
    HAL_SEA_ECC_STA_TIMEOUT_FAILURE             = 15U,  /*!< ECC calculation timeout */
    HAL_SEA_ECC_STA_PARASLEN_FAILURE            = 16U,  /*!< ECC parameter length invalid */
    HAL_SEA_ECC_STA_LOOPTIMETOUT_FAILURE        = 17U,  /*!< ECC loop timeout */
    HAL_SEA_ECC_STA_PKEY_FAILURE                = 18U,  /*!< ECC private key invalid */
    HAL_SEA_RSA_STA_TIMEOUT_FAILURE             = 19U,  /*!< RSA calculation timeout */
    HAL_SEA_RSA_STA_PARASLEN_FAILURE            = 20U,  /*!< RSA parameter length invalid */
    HAL_SEA_RSA_STA_KEY_FAILURE                 = 21U,  /*!< RSA invalid key */
    HAL_SEA_RSA_STA_PADDING_FAILURE             = 22U,  /*!< RSA padding invalid */
    HAL_SEA_KEYSTORE_STA_WRKEY_FAILURE          = 23U,  /*!< Keystore write key failed */
    HAL_SEA_KEYSTORE_STA_RDKEY_FAILURE          = 24U,  /*!< Keystore read key failed */
    HAL_SEA_TRNG_STA_KEYGEN_FAILURE             = 25U,  /*!< TRNG generate key failed */
    HAL_SEA_TRNG_STA_KLEN_FAILURE               = 26U,  /*!< TRNG key length invalid */
    HAL_SEA_TRNG_STA_TIMEOUT_FAILURE            = 27U,  /*!< TRNG timeout */
    HAL_SEA_TRNG_STA_RANDOM_FAILURE             = 28U,  /*!< TRNG random check failed */
    HAL_SEA_FLASH_STA_CLK_FREQ_FAILURE          = 29U,  /*!< Flash clock frequency invalid */
} HAL_SEA_StatusTypeDef;

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup SEA_Exported_Constants SEA Exported Constants
 * @{
 */

/**
 * @}
 */

/* Exported macros ----------------------------------------------------------*/
/** @defgroup SEA_Exported_Macros SEA Exported Macros
 * @{
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup SEA_Exported_Functions SEA Exported Functions
 * @{
 */

/** @addtogroup SEA_Exported_Functions_Group1
 * @{
 */
/* AES IO operation functions ************************************************/
HAL_SEA_StatusTypeDef HAL_SEA_AES_GF128Mul(AES_HandleTypeDef *haes, const uint8_t *pDataX, const uint8_t *pDataY, uint8_t *pDataZ);
HAL_SEA_StatusTypeDef HAL_SEA_AES_Init(AES_HandleTypeDef *haes, const AES_InitTypeDef *pConfig);
HAL_SEA_StatusTypeDef HAL_SEA_AES_DeInit(void);
HAL_SEA_StatusTypeDef HAL_SEA_AES_Encrypt(AES_HandleTypeDef *haes, const uint8_t *PlainData, uint32_t PlainLen, CYPHER_DATA_STA IsLast, uint8_t *CipherData);
HAL_SEA_StatusTypeDef HAL_SEA_AES_Decrypt(AES_HandleTypeDef *haes, const uint8_t *CipherData, uint32_t CipherLen, CYPHER_DATA_STA IsLast, uint8_t *PlainData);
HAL_SEA_StatusTypeDef HAL_SEA_AES_Gmac(AES_HandleTypeDef *haes, const uint8_t *GhashData, uint32_t GhashDataLen, CYPHER_DATA_STA IsLast);
HAL_SEA_StatusTypeDef HAL_SEA_AES_LatchProcessStatus(AES_HandleTypeDef *haes, AES_HandleTypeDef *LatchBuf);
HAL_SEA_StatusTypeDef HAL_SEA_AES_RecoverProcessStatus(AES_HandleTypeDef *RecoverBuf, AES_HandleTypeDef *haes);
HAL_SEA_StatusTypeDef HAL_SEA_AES_KeyWrapping(AES_HandleTypeDef *haes, const uint8_t *PlainData, uint32_t PlainLen, uint8_t *CipherData, uint32_t *CipherLen);
HAL_SEA_StatusTypeDef HAL_SEA_AES_KeyUnwrapping(AES_HandleTypeDef *haes, const uint8_t *CipherData, uint32_t CipherLen, uint8_t *PlainData, uint32_t *PlainLen);
HAL_SEA_StatusTypeDef HAL_SEA_AES_SetAAD(AES_HandleTypeDef *haes, const uint8_t *DataIn, uint32_t DataLen);
/**
 * @}
 */

/** @addtogroup SEA_Exported_Functions_Group2
 * @{
 */
/* HASH IO operation functions ***********************************************/
HAL_SEA_StatusTypeDef HAL_SEA_HASH_Init(HASH_HandleTypeDef *hhash, const HASH_InitTypeDef *pConfig);
HAL_SEA_StatusTypeDef HAL_SEA_HASH_DeInit(void);
HAL_SEA_StatusTypeDef HAL_SEA_HASH(HASH_HandleTypeDef *hhash, const uint8_t *Input, uint32_t InputLen, CYPHER_DATA_STA IsLast, uint8_t *Output);
HAL_SEA_StatusTypeDef HAL_SEA_HASH_LatchProcessStatus(const HASH_HandleTypeDef *hhash, HASH_HandleTypeDef *LatchBuf);
HAL_SEA_StatusTypeDef HAL_SEA_HASH_RecoverProcessStatus(const HASH_HandleTypeDef *RecoverBuf, HASH_HandleTypeDef *hhash);
/**
 * @}
 */

/** @addtogroup SEA_Exported_Functions_Group3
 * @{
 */
/* ECC IO operation functions ************************************************/
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateKey(const ECC_HandleTypeDef *hecc, uint8_t *Q, uint8_t *K);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GeneratePublicKey(const ECC_HandleTypeDef *hecc, uint8_t *Q, const uint8_t *K);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateUncompPublicKey(const ECC_HandleTypeDef *hecc, uint8_t *Q, const uint8_t *K);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_VerifyPublicKey(const ECC_HandleTypeDef *hecc, const uint8_t *Q, HAL_SEA_FlagTypeDef *pFlag);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateDHKeyPairs(const ECC_HandleTypeDef *hecc, const uint8_t *Q, const uint8_t *K, uint8_t *DHKey);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateDHKeyX(const ECC_HandleTypeDef *hecc, const uint8_t *Q, const uint8_t *K, uint8_t *DHKeyX);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateSignature(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, uint8_t *Q, uint8_t *K, uint8_t *R, uint8_t *S);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateSignaturewithPKey(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, uint8_t *Q, const uint8_t *K, uint8_t *R, uint8_t *S);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_VerifySignature(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t *Q, const uint8_t *R, const uint8_t *S, HAL_SEA_FlagTypeDef *pFlag);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateKeySecurely(const ECC_HandleTypeDef *hecc, uint8_t PKeyIdx, uint8_t QxIdx, uint8_t QyIdx);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateSignatureSecurely(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t PKeyIdx, uint8_t *R, uint8_t *S);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_VerifySignatureSecurely(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t QxIdx, const uint8_t QyIdx, const uint8_t *R, const uint8_t *S, HAL_SEA_FlagTypeDef *pFlag);
/**
 * @}
 */

/** @addtogroup SEA_Exported_Functions_Group4 RSA IO operation functions
 * @{
 */
/* RSA IO operation functions ************************************************/
HAL_SEA_StatusTypeDef HAL_SEA_RSA_GenerateKey(RSA_HandleTypeDef *hrsa);
HAL_SEA_StatusTypeDef HAL_SEA_RSA_GenerateSignature(RSA_HandleTypeDef *hrsa, const uint32_t *HashMessage, uint32_t MessageLen, uint32_t *Signature);
HAL_SEA_StatusTypeDef HAL_SEA_RSA_VerifySignature(RSA_HandleTypeDef *hrsa, const uint32_t *HashMessage, uint32_t MessageLen, uint32_t *Signature);
HAL_SEA_StatusTypeDef HAL_SEA_RSA_EncryptwithPublicKey(RSA_HandleTypeDef *hrsa, const uint32_t *PlainData, uint32_t PlainLen, uint32_t *CipherData);
HAL_SEA_StatusTypeDef HAL_SEA_RSA_DecryptwithPublicKey(RSA_HandleTypeDef *hrsa, const uint32_t *CipherData, uint32_t CipherLen, uint32_t *PlainData);
HAL_SEA_StatusTypeDef HAL_SEA_RSA_EncryptwithPrivateKey(RSA_HandleTypeDef *hrsa, const uint32_t *PlainData, uint32_t PlainLen, uint32_t *CipherData);
HAL_SEA_StatusTypeDef HAL_SEA_RSA_DecryptwithPrivateKey(RSA_HandleTypeDef *hrsa, const uint32_t *CipherData, uint32_t CipherLen, uint32_t *PlainData);
/**
 * @}
 */

/** @addtogroup SEA_Exported_Functions_Group5
 * @{
 */
/* TRNG IO operation functions ***********************************************/
HAL_SEA_StatusTypeDef HAL_SEA_TRNG_GenerateRandom(uint32_t *RandomData, uint8_t DataLen);
HAL_SEA_StatusTypeDef HAL_SEA_TRNG_GenerateKey(KEYSTORE_InitTypeDef *KEYSTORE_InitStruct, uint8_t KeyIndex);
/**
 * @}
 */

/** @addtogroup SEA_Exported_Functions_Group6
 * @{
 */
/* State functions ***********************************************************/
uint32_t             HAL_SEA_GetState(void);
/**
 * @}
 */

/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/** @defgroup SEA_Private_Types SEA Private Types
 * @{
 */

/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/** @defgroup SEA_Private_Variables SEA Private Variables
 * @{
 */

/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup SEA_Private_Constants SEA Private Constants
 * @{
 */

/**
 * @}
 */

/* Private Macros -----------------------------------------------------------*/
/** @defgroup SEA_Private_Macros SEA Private Macros
 * @{
 */

/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup SEA_Private_Functions SEA Private Functions
 * @{
 */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_HAL_SEA_H */
