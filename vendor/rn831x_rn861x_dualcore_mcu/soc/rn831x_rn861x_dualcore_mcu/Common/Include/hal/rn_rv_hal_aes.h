/**
 ******************************************************************************
 * @file   rn_rv_hal_aes.h
 * @author Renergy Software Team
 * @brief  Header file of AES HAL module.
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
#ifndef __RN_RV_HAL_AES_H
#define __RN_RV_HAL_AES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_ll_aes.h"

#define AES_CRYPT_ITEMS_IN_BYTES

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup AES
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup AES_Exported_Types AES Exported Types
 * @{
 */
#ifdef AES_CRYPT_ITEMS_IN_BYTES
typedef uint8_t PlainData_t;
typedef uint8_t CryptData_t;
typedef uint8_t aesAAD_t;
typedef uint8_t GHashData_t;
typedef uint8_t aesIV_t;
#define AES_REGS_DIV_DATA_ITEM 16U
#else
typedef uint32_t PlainData_t;
typedef uint32_t CryptData_t;
typedef uint32_t aesAAD_t;
typedef uint32_t GHashData_t;
typedef uint32_t aesIV_t;
#define AES_REGS_DIV_DATA_ITEM 4U
#endif
typedef uint8_t aesKey_t;
typedef uint8_t aesTag_t;

typedef enum __attribute__((__packed__))
{
    AES_RUNNING_IN_CORE0    = 0U,
    AES_RUNNING_IN_CORE1    = 1U
} AES_KernelTypeDef;

typedef enum __attribute__((__packed__))
{
    AES_BIG_ENDIAN          = 0U,
    AES_LITTLE_ENDIAN       = 1U
} AES_ENDIAN_MODE;

typedef enum __attribute__((__packed__))
{
    CypherDatasNotTheLast   = 0U,
    CypherDatasIsTheLast    = 1U
} CYPHER_DATA_STA;

typedef enum __attribute__((__packed__))
{
    AES_PLAIN_ONLY          = 0U,
    AES_IV_ONLY             = 1U,
    AES_IV_PLAIN3           = 2U,
    AES_PLAIN_IV3           = 3U
} AES_CRYPT_STRC;

typedef enum __attribute__((__packed__))
{
    AES_SPC_NONE            = 0U,
    AES_SPC_GCM_VERIFY_ONLY = 1U,
    AES_SPC_GMAC_CryptData  = 2U
} AES_SPECIAL_MOD;

typedef enum __attribute__((__packed__))
{
    AES_enCrypt             = 0U,
    AES_deCrypt             = 1U
} AES_ENCRYPT_SEL;

typedef enum __attribute__((__packed__))
{
    AES_128                 = 0U,
    AES_192                 = 1U,
    AES_256                 = 2U
} AES_BITWIDE_MODE;

typedef enum __attribute__((__packed__))
{
    AES_ECB                 = 0U,
    AES_CBC                 = 1U,
    AES_CTR                 = 2U,
    AES_CFB128              = 3U,
    AES_OFB                 = 4U,
    AES_GCM                 = 5U
} AES_CRYPT_MODE;

typedef enum __attribute__((__packed__))
{
    AES_VERIFY_SUCCESS      = 0U,
    AES_VERIFY_FAIL         = 1U
} AES_VERIFY_STA;

typedef enum __attribute__((__packed__)) {
    AES_GENERAL_MODE_EN     = 0U,
    AES_SECURE_MODE_EN      = 1U
} AES_RUNNING_MODE;

typedef enum __attribute__((__packed__))
{
    AES_KW_MODE_KW          = 0U,
    AES_KW_MODE_KWP         = 1U
} AES_KW_MODE;

typedef struct
{
    AES_CRYPT_MODE    cryptMode;
    AES_CRYPT_STRC    crypt_kernelMode;
    AES_KernelTypeDef kernelRegion;
    uint8_t           is_last;
    uint32_t          cryptSize;
    uint32_t          aadSize;
    uint8_t           crypt_ir;
    uint8_t           is_start_ghash;
    uint8_t           is_start_cryptKernel;
    uint8_t           is_start_crypt;
    uint32_t          dma_addr;
} aesEngineParameters;

typedef struct
{
    uint32_t h_y0[4];
    uint32_t h_iv[4];
    uint32_t h_aad[4];
    uint32_t h_predata[4];
    uint32_t h_default[4];
    uint32_t iv[4];
    uint32_t cypher_predata[4];
    uint32_t key[8];
    uint32_t C_SIZE;
} SEA_AES_G_PARAS;

typedef struct
{
    AES_ENCRYPT_SEL   encrypt_sel;
    AES_BITWIDE_MODE  bitWideMode;
    AES_CRYPT_MODE    cryptMode;
    AES_RUNNING_MODE  runningMode;
    aesKey_t         *keyLocation;
    aesIV_t          *ivLocation;
    uint32_t          ivSize;
    aesAAD_t         *aadLocation;
    uint32_t          aadSize;
    aesTag_t         *tagSignBuf;
    uint32_t          tagSize;
    aesTag_t         *tagVerifyBuf;
    AES_VERIFY_STA   *tagVerifySta;
    AES_SPECIAL_MOD   specialMode;
    AES_KW_MODE       keyWrapMode;
    uint8_t           keyIndex;
    uint8_t           reserved;
    uint32_t          assignedAddress; /*!< Valid only in the stand-alone AES Module. */
} aesCypherConfigStr;

typedef struct
{
    aesCypherConfigStr  config;
    aesEngineParameters engine;
    SEA_AES_G_PARAS     proc;
} aesProcessParameters;

typedef enum __attribute__((__packed__))
{
    AES_SUCCESS                     = 0U,
    AES_ERROR                       = 1U,
    AES_CRYPTKEY_ADDR_ALIGN_FAILURE = 2U,
    AES_CRYPT_FAILURE               = 3U,
    AES_DECRYPT_FAILURE             = 4U,
    AES_GMAC_FAILURE                = 5U,
    AES_NULL_POINTER_FAILURE        = 6U,
    AES_KW_MODE_FAILURE             = 7U,
    AES_KW_INLEN_FAILURE            = 8U,
    AES_KW_CHECK_ICV_FAILURE        = 9U,
    AES_KW_CHECK_PADDING_FAILURE    = 10U,
    AES_REMAP_ADDR_INVALID_FAILURE  = 11U,
    AES_TIMEOUT_FAILURE             = 12U,
} HAL_AES_StatusTypeDef;

typedef aesCypherConfigStr   AES_InitTypeDef;
typedef aesProcessParameters AES_HandleTypeDef;
/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup AES_Exported_Constants AES Exported Constants
 * @{
 */

/**
 * @}
 */

/* Exported macros ----------------------------------------------------------*/
/** @defgroup AES_Exported_Macros AES Exported Macros
 * @{
 */

/**
 * @brief AES_HAL_MODE_EN MODE_EN
 */
/* TODO: here should be from SYS_CTRL.h */
#define SYSCTRL_MOD2_AES_EN_POS         (1UL)
#define SYSCTRL_MOD2_AES_EN_MSK         BIT(8U)

#define SYSCTRL_BASE_ADDR_CORE0         (0x30018000U)
#define SYSCTRL_BASE_ADDR_CORE1         (0xB0018000U)
#define SYSCTRL_ADDR_MAP1_CORE0         (0x10000000U)
#define SYSCTRL_ADDR_MAP1_CORE1         (0x80000000U)

#define SCU_PERIPH_SEC_CFG2(ver)        (((ver) == AES_RUNNING_IN_CORE0) ? 0x30018010U : 0xB0018010U)
#define SCU_SRAM_SEC_WM_CFG(ver)        (((ver) == AES_RUNNING_IN_CORE0) ? 0x3001805CU : 0xB001805CU)

#define AES_MODULE_REMAP_ADDR(ver)      (((ver) == AES_RUNNING_IN_CORE0) ? \
                                        (AES_CORE0_BASE + 0x0U) : \
                                        (AES_CORE1_BASE + 0x0U))
#define AES_IV_ADDR(ver)                (((ver) == AES_RUNNING_IN_CORE0) ? \
                                        (AES_CORE0_BASE + 0x8U) : \
                                        (AES_CORE1_BASE + 0x8U))
#define AES_PT_ADDR(ver, addr)          (((ver) == AES_RUNNING_IN_CORE0) ? \
                                        (addr + 0x00U) : \
                                        (addr + 0x00U))
#define AES_CT_ADDR(ver, addr)          (((ver) == AES_RUNNING_IN_CORE0) ? \
                                        (addr + 0x10U) : \
                                        (addr + 0x10U))
#define AES_KEYEXP_ADDR(ver, addr)      (((ver) == AES_RUNNING_IN_CORE0) ? \
                                        (addr + 0x20U) : \
                                        (addr + 0x20U))

/**
 * @brief AES_HAL_IR_TYPE IR_TYPE
 */
#define HAL_AES_IR_KEYEXP               AES_CTRL_IR_KEYEXP
#define HAL_AES_IR_ENCRYPT              AES_CTRL_IR_ENC
#define HAL_AES_IR_DECRYPT              AES_CTRL_IR_DEC
#define HAL_AES_IR_GF128                AES_CTRL_IR_GF128

/**
 * @brief AES_HAL_KEY_LEN KEY_LEN
 */
#define HAL_AES_KEYLEN_128              AES_CTRL_SIZE_KLEN128
#define HAL_AES_KEYLEN_192              AES_CTRL_SIZE_KLEN192
#define HAL_AES_KEYLEN_256              AES_CTRL_SIZE_KLEN256

/**
 * @brief AES_HAL_DATA_STREAM_MODE DATA_STREAM_MODE
 */
#define HAL_AES_STREAM_MODE_ECB         AES_CTRL_MODE_ECB
#define HAL_AES_STREAM_MODE_CBC         AES_CTRL_MODE_CBC
#define HAL_AES_STREAM_MODE_CTR         AES_CTRL_MODE_CTR
#define HAL_AES_STREAM_MODE_CFB128      AES_CTRL_MODE_CFB128
#define HAL_AES_STREAM_MODE_OFB         AES_CTRL_MODE_OFB
/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup AES_Exported_Functions AES Exported Functions
 * @{
 */

/** @addtogroup AES_Exported_Functions_Group1
 * @{
 */
/* Initialization and Configuration functions ********************************/
HAL_AES_StatusTypeDef HAL_AES_Init(AES_HandleTypeDef *pParas, const AES_InitTypeDef *Configuration);
HAL_AES_StatusTypeDef HAL_AES_DeInit(void);
/**
 * @}
 */

/** @addtogroup AES_Exported_Functions_Group2
 * @{
 */
/* IO operation functions ****************************************************/
HAL_AES_StatusTypeDef HAL_AES_Encrypt(AES_HandleTypeDef *pParas, const uint8_t *Plaintext, uint32_t PlaintextLen, uint8_t IsLast, uint8_t *Ciphertext);
HAL_AES_StatusTypeDef HAL_AES_Decrypt(AES_HandleTypeDef *pParas, const uint8_t *Ciphertext, uint32_t CiphertextLen, uint8_t IsLast, uint8_t *Plaintext);
HAL_AES_StatusTypeDef HAL_AES_Gmac(AES_HandleTypeDef *pParas, const uint8_t *GhashData, uint32_t GhashDataLen, uint8_t IsLast);
HAL_AES_StatusTypeDef HAL_AES_GF128Mul(AES_HandleTypeDef *pParas, const uint8_t *pDataX, const uint8_t *pDataY, uint8_t *pDataZ);
HAL_AES_StatusTypeDef HAL_AES_KeyWrapping(AES_HandleTypeDef *pParas, const uint8_t *Plaintext, uint32_t PlaintextLen, uint8_t *Ciphertext, uint32_t *CiphertextLen);
HAL_AES_StatusTypeDef HAL_AES_KeyUnwrapping(AES_HandleTypeDef *pParas, const uint8_t *Ciphertext, uint32_t CiphertextLen, uint8_t *Plaintext, uint32_t *PlaintextLen);
HAL_AES_StatusTypeDef HAL_AES_SetAAD(AES_HandleTypeDef *pParas, const uint8_t *AAD, uint32_t AADLen);
HAL_AES_StatusTypeDef HAL_AES_LatchProcessStatus(AES_HandleTypeDef *pParas, AES_HandleTypeDef *LatchBuf);
HAL_AES_StatusTypeDef HAL_AES_RecoverProcessStatus(AES_HandleTypeDef *RecoverBuf, AES_HandleTypeDef *pParas);
/**
 * @}
 */

/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/** @defgroup AES_Private_Types AES Private Types
 * @{
 */

/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/** @defgroup AES_Private_Variables AES Private Variables
 * @{
 */

/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup AES_Private_Constants AES Private Constants
 * @{
 */
#define AES_CRYPT_ITEMS_IN_BYTES

#define AES_PROCESS_LATCH_RECOVER_SUPPORT

#define AES_DECRYPT_GCM_SPECIAL_MOD_TYPE 0

#define NB               4U
/* Number of 32-bit words comprising the cypher key */
#define NK_AES128        4U
#define NK_AES192        6U
#define NK_AES256        8U

/* Number of bytes comprising the cypher key */
#define AES128_KEYSIZE   ((uint8_t)(4U * NK_AES128))
#define AES192_KEYSIZE   ((uint8_t)(4U * NK_AES192))
#define AES256_KEYSIZE   ((uint8_t)(4U * NK_AES256))

/* input and output buffer blocks processed every turn */
#define AES_PLAIN_BSIZE  (4U * NB)
#define AES_CRYPT_BSIZE  (4U * NB)
#define AES_BLOCK_BSIZE  (4U * NB)
#define AES_BLOCK_BSHIFT (4U)
/**
 * @}
 */

/* Private Macros -----------------------------------------------------------*/
/** @defgroup AES_Private_Macros AES Private Macros
 * @{
 */

/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup AES_Private_Functions AES Private Functions
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

#endif /* __RN_RV_HAL_AES_H */
