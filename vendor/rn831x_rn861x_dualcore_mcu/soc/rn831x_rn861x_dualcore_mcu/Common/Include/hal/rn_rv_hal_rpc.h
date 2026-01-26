/**
 ******************************************************************************
 * @file   rn_rv_hal_rpc.h
 * @author Renergy Software Team
 * @brief  Header file of RPC HAL module.
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
#ifndef __RN_RV_HAL_RPC_H
#define __RN_RV_HAL_RPC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include <string.h>
#include <stdbool.h>
#include "rn_rv_hal_def.h"
#include "rn_rv_ll_gic.h"
#include "rn831x_rn861x_mcu_mailbox.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup RPC
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup RPC_Exported_Types RPC Exported Types
 * @{
 */

/**
 * @brief MAILBOX Init structure definition
 */
typedef struct
{
    FunctionalState InputFull_ITCommand;      /*!< Specifies the new state of the input mailbox Full interrupt for Core0.
                                                    This parameter can be set either to ENABLE or DISABLE */

    FunctionalState OutputEmpty_ITCommand;    /*!< Specifies the new state of the Output mailbox Empty interrupt for Core0.
                                                    This parameter can be set either to ENABLE or DISABLE */

    FunctionalState InputEmpty_ITCommand;     /*!< Specifies the new state of the input mailbox Empty interrupt for Core1.
                                                    This parameter can be set either to ENABLE or DISABLE */

    FunctionalState OutputFull_ITCommand;     /*!< Specifies the new state of the Output mailbox Full interrupt for Core1.
                                                    This parameter can be set either to ENABLE or DISABLE */

    FunctionalState Linkable_ITCommand;       /*!< Specifies the new state of the mailbox Linkable interrupt for Core1.
                                                    This parameter can be set either to ENABLE or DISABLE */
} MAILBOX_InitTypeDef;

/**
 * @brief  ICC Core Command enumeration
 *         ICC Core Command use 8bit ici Message.
 *         Format:
 *             [7:6]: Type:    0x1(CORE_STA), 0x2(SHARE_MEM), 0 is reserved.
 *             [5:0]: Command: 0x1(CORE_STARTUP), 0x2(CORE_HANDSHAKE),
 *                          0x3(CORE_SLEEP), 0x4(CORE_WAKEUP)
 */
typedef enum __attribute__((__packed__))
{
    HAL_CORE_STARTUP   = (0x1U << 6U) | 0x1U,
    HAL_CORE_HANDSHAKE = (0x1U << 6U) | 0x2U,
    HAL_CORE_SLEEP     = (0x1U << 6U) | 0x3U,
    HAL_CORE_WAKEUP    = (0x1U << 6U) | 0x4U
} HAL_ICCCommandTypeDef;

/**
 * @brief  Share Memory Command enumeration
 *         Share Memory Command use 8bit ici Message.
 *         Format:
 *             [7:6]: Type:    0x1(CORE_STA), 0x2(SHARE_MEM), 0 is reserved.
 *             [5:0]: Command: 0x1(Transmit), 0x2(Receive)
 */
typedef enum __attribute__((__packed__))
{
    HAL_SHMEM_TRANSMITD = (0x2U << 6U) | 0x1U,
    HAL_SHMEM_RECEIVED  = (0x2U << 6U) | 0x2U
} HAL_ShareMemoryCommandTypeDef;

/**
 * @brief  Mailbox Message Type enumeration
 */
typedef enum __attribute__((__packed__))
{
    HAL_MAILBOX_API_CALL     = 0x1U,
    HAL_MAILBOX_SHMEM_CONFIG = 0x2U
} HAL_MailboxMessageTypeDef;

/**
 * @brief  HAL RPC API ID enumeration definition
 */
typedef enum __attribute__((__packed__))
{
    SYSTEM_CLOCKMODECONFIG_ID      = 0U,    /*!< @ref HAL_SYSTEM_ClockModeConfig() RPC API ID            */
    SYSTEM_FASTCLOCKMODECONFIG_ID  = 1U,    /*!< @ref HAL_SYSTEM_FastClockModeConfig() RPC API ID        */
    SYSTEM_SETCOREDIVISION_ID      = 2U,    /*!< @ref HAL_SYSTEM_SetCoreDivision() RPC API ID            */
    SYSTEM_PLLCONFIG_ID            = 3U,    /*!< @ref HAL_SYSTEM_PLLConfig() RPC API ID                  */
    SYSTEM_OPTIONCONFIGCHECK_ID    = 4U,    /*!< @ref HAL_SYSTEM_OptionConfigCheck() RPC API ID          */
    SYSTEM_SETRTCDOTA_ID           = 5U,    /*!< @ref HAL_SYSTEM_SetRTCDota() RPC API ID                 */
    SYSTEM_GETRTCDOTA_ID           = 6U,    /*!< @ref HAL_SYSTEM_GetRTCDota() RPC API ID                 */
    FLASH_UNLOCK_ID                = 7U,    /*!< @ref HAL_FLASH_Unlock() RPC API ID                      */
    FLASH_PROGRAM_ID               = 8U,    /*!< @ref HAL_FLASH_Program() RPC API ID                     */
    FLASH_ERASE_ID                 = 9U,    /*!< @ref HAL_FLASH_Erase() RPC API ID                       */
    FLASH_CLEARSOFTISP_ID          = 10U,   /*!< @ref HAL_FLASH_ClearSoftISP() RPC API ID                */
    FLASH_ENABLESOFTISP_ID         = 11U,   /*!< @ref HAL_FLASH_EnableSoftISP() RPC API ID               */
    FLASH_CLEARCP_ID               = 12U,   /*!< @ref HAL_FLASH_ClearCP() RPC API ID                     */
    SEA_AES_GF128MUL_ID            = 13U,   /*!< @ref HAL_SEA_AES_GF128Mul() RPC API ID                  */
    SEA_AES_INIT_ID                = 14U,   /*!< @ref HAL_SEA_AES_Init() RPC API ID                      */
    SEA_AES_DEINIT_ID              = 15U,   /*!< @ref HAL_SEA_AES_DeInit() RPC API ID                    */
    SEA_AES_ENCRYPT_ID             = 16U,   /*!< @ref HAL_SEA_AES_Encrypt() RPC API ID                   */
    SEA_AES_DECRYPT_ID             = 17U,   /*!< @ref HAL_SEA_AES_Decrypt() RPC API ID                   */
    SEA_AES_GMAC_ID                = 18U,   /*!< @ref HAL_SEA_AES_Gmac() RPC API ID                      */
    SEA_AES_LATCH_PROCSTA_ID       = 19U,   /*!< @ref HAL_SEA_AES_LatchProcessStatus() RPC API ID        */
    SEA_AES_RECOVER_PROCSTA_ID     = 20U,   /*!< @ref HAL_SEA_AES_RecoverProcessStatus() RPC API ID      */
    SEA_AES_KEYWRAP_ID             = 21U,   /*!< @ref HAL_SEA_AES_KeyWrapping() RPC API ID               */
    SEA_AES_KEYUNWARP_ID           = 22U,   /*!< @ref HAL_SEA_AES_KeyUnwrapping() RPC API ID             */
    SEA_AES_SET_AAD_ID             = 23U,   /*!< @ref HAL_SEA_AES_SetAAD() RPC API ID                    */
    SEA_HASH_INIT_ID               = 24U,   /*!< @ref HAL_SEA_HASH_Init() RPC API ID                     */
    SEA_HASH_DEINIT_ID             = 25U,   /*!< @ref HAL_SEA_HASH_DeInit() RPC API ID                   */
    SEA_HASH_ID                    = 26U,   /*!< @ref HAL_SEA_HASH() RPC API ID                          */
    SEA_HASH_LATCH_PROCSTA_ID      = 27U,   /*!< @ref HAL_SEA_HASH_LatchProcessStatus() RPC API ID       */
    SEA_HASH_RECOVER_PROCSTA_ID    = 28U,   /*!< @ref HAL_SEA_HASH_RecoverProcessStatus() RPC API ID     */
    SEA_ECC_GEN_KEY_ID             = 29U,   /*!< @ref HAL_SEA_ECC_GenerateKey() RPC API ID               */
    SEA_ECC_GEN_PUBKEY_ID          = 30U,   /*!< @ref HAL_SEA_ECC_GeneratePublicKey() RPC API ID         */
    SEA_ECC_VERIFY_PUBKEY_ID       = 31U,   /*!< @ref HAL_SEA_ECC_VerifyPublicKey() RPC API ID           */
    SEA_ECC_GEN_DHKEY_ID           = 32U,   /*!< @ref HAL_SEA_ECC_GenerateDHKeyPairs() RPC API ID        */
    SEA_ECC_GEN_SIGN_ID            = 33U,   /*!< @ref HAL_SEA_ECC_GenerateSignature() RPC API ID         */
    SEA_ECC_GEN_SIGN_PUBKEY_ID     = 34U,   /*!< @ref HAL_SEA_ECC_GenerateSignaturewithPKey() RPC API ID */
    SEA_ECC_VERIFY_SIGN_ID         = 35U,   /*!< @ref HAL_SEA_ECC_VerifySignature() RPC API ID           */
    SEA_ECC_GEN_KEY_SEC_ID         = 36U,   /*!< @ref HAL_SEA_ECC_GenerateKeySecurely() RPC API ID       */
    SEA_ECC_GEN_SIGN_SEC_ID        = 37U,   /*!< @ref HAL_SEA_ECC_GenerateSignatureSecurely() RPC API ID */
    SEA_ECC_VERIFY_SIGN_SEC_ID     = 38U,   /*!< @ref HAL_SEA_ECC_VerifySignatureSecurely() RPC API ID   */
    SEA_RSA_GEN_KEY_ID             = 39U,   /*!< @ref HAL_SEA_RSA_GenerateKey() RPC API ID               */
    SEA_RSA_GEN_SIGN_ID            = 40U,   /*!< @ref HAL_SEA_RSA_GenerateSignature() RPC API ID         */
    SEA_RSA_VERIFY_SIGN_ID         = 41U,   /*!< @ref HAL_SEA_RSA_VerifySignature() RPC API ID           */
    SEA_RSA_ENC_PUBKEY_ID          = 42U,   /*!< @ref HAL_SEA_RSA_EncryptwithPublicKey() RPC API ID      */
    SEA_RSA_DEC_PUBKEY_ID          = 43U,   /*!< @ref HAL_SEA_RSA_DecryptwithPublicKey() RPC API ID      */
    SEA_RSA_ENC_PRIKEY_ID          = 44U,   /*!< @ref HAL_SEA_RSA_EncryptwithPrivateKey() RPC API ID     */
    SEA_RSA_DEC_PRIKEY_ID          = 45U,   /*!< @ref HAL_SEA_RSA_DecryptwithPrivateKey() RPC API ID     */
    SEA_TRNG_GEN_RANDOM_ID         = 46U,   /*!< @ref HAL_SEA_TRNG_GenerateRandom() RPC API ID           */
    KEYSTORE_UNLOCK_ID             = 47U,   /*!< @ref HAL_KEYSTORE_Unlock() RPC API ID                   */
    KEYSTORE_RD_KEY_ID             = 48U,   /*!< @ref HAL_KEYSTORE_ReadKey() RPC API ID                  */
    KEYSTORE_RD_KEY_INFO_ID        = 49U,   /*!< @ref HAL_KEYSTORE_ReadKeyProperty() RPC API ID          */
    KEYSTORE_WR_KEY_ID             = 50U,   /*!< @ref HAL_KEYSTORE_WriteKey() RPC API ID                 */
    KEYSTORE_ERASE_ID              = 51U,   /*!< @ref HAL_KEYSTORE_Erase() RPC API ID                    */
    KEYSTORE_GET_STA_ID            = 52U,   /*!< @ref HAL_KEYSTORE_GetState() RPC API ID                 */
    RPC_MAX_USED_API_ID
} RPC_APIUsedIDTypeDef;

typedef void (*ProcessCallback)(uint32_t *, uint32_t, uint32_t);
/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup RPC_Exported_Constants RPC Exported Constants
 * @{
 */
#define HAL_RPC_TIMEOUT_FLAG          35U         /*!< Timeout 35 ms */
#define HAL_RPC_TIMEOUT_BUSY_FLAG     25U         /*!< Timeout 25 ms */
#define HAL_RPC_TIMEOUT_STOP_FLAG     5U          /*!< Timeout 5 ms  */
/**
 * @}
 */

/* Exported macros ----------------------------------------------------------*/
/** @defgroup RPC_Exported_Macros RPC Exported Macros
 * @{
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup RPC_Exported_Functions RPC Exported Functions
 * @{
 */

/** @addtogroup RPC_Exported_Functions_Group1 Initialization and de-initialization functions
 * @{
 */
/* Initialization and de-initialization functions ****************************/
HAL_StatusTypeDef HAL_ICC_Init(uint32_t Timeout);
void              HAL_ICC_DeInit(void);
/* Share Mempry **************************************************************/
#ifdef CORE0
HAL_StatusTypeDef HAL_SHMEM_Init(ProcessCallback Callback, uint32_t Timeout);
#endif
#ifdef CORE1
HAL_StatusTypeDef HAL_SHMEM_Init(uint32_t *SHMEMAddr, uint32_t SHMEMLength, uint32_t BufSize, ProcessCallback Callback, uint32_t Timeout);
#endif
HAL_StatusTypeDef HAL_SHMEM_DeInit(void);
/**
 * @}
 */

/** @addtogroup RPC_Exported_Functions_Group2 Send and Receive Message functions
 * @{
 */
/* Mailbox Message operation functions ***************************************/
HAL_StatusTypeDef HAL_SHMEM_Transmit(const uint8_t *pData, uint32_t Size);
/**
 * @}
 */

/** @addtogroup RPC_Exported_Functions_Group3 IRQ Handler and Callbacks
 * @{
 */
/* MAILBOX IRQHandler and Callbacks ******************************************/
void HAL_MAILBOX_CORE0_IRQHandler(void);
void HAL_MAILBOX_CORE1_IRQHandler(void);
void HAL_MAILBOX_MessageFullReadCallback(uint32_t *pData, uint16_t Size);
void HAL_SHMEM_MessageReceiveCallback(void);
/**
 * @}
 */

/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/** @defgroup RPC_Private_Types RPC Private Types
 * @{
 */

/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/** @defgroup RPC_Private_Variables RPC Private Variables
 * @{
 */

/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup RPC_Private_Constants RPC Private Constants
 * @{
 */

/**
 * @}
 */

/* Private Macros -----------------------------------------------------------*/
/** @defgroup RPC_Private_Macros RPC Private Macros
 * @{
 */

/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup RPC_Private_Functions RPC Private Functions
 * @{
 */
HAL_StatusTypeDef RPC_APICall(uint8_t API_ID, uint32_t *Params, uint32_t Size, uint32_t Timeout);
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

#endif /* __RN_RV_HAL_RPC_H */
