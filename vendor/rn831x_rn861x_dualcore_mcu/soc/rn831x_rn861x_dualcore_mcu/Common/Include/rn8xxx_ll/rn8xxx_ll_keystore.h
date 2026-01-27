/**
 ******************************************************************************
 * @file   rn_rv_hal_keystore.h
 * @author Renergy Software Team
 * @brief  Header file of KeyStore HAL module.
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
#ifndef __RN_RV_HAL_KEYSTORE_H
#define __RN_RV_HAL_KEYSTORE_H

#if defined(RN831x_RN861x_MCU_V4)

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn8xxx_ll_utils.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup KEYSTORE
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup KEYSTORE_Exported_Types KEYSTORE Exported Types
 * @{
 */

/**
 * @brief KeyStore init structure.
 */
typedef struct
{
    uint8_t  Readable;     /*!< Whether the key is readable to CPU or not.*/

    uint8_t  Encrypted;    /*!< Whether the key is encrypted and stored or not.*/

    uint8_t  Reserved;

    uint8_t  Programmed;   /*!< Whether the key slot is programmed or not.*/

    uint8_t  EncryptMode;  /*!< Set the encrypt mode: KEYSTORE_AES_ECB or KEYSTORE_XOR_ENC.*/

    uint8_t  RootKeyIdx;   /*!< Set the root key index, which is generally 80.*/

    uint8_t  Reserved1[2];

    uint32_t KeyLen;       /*!< Set the key length: 128,160,192,224,256,384,512,521,576.*/

} KeyStore_InitTypeDef;

/**
 * @brief KeyStore key encryption method.
 */
typedef enum __attribute__((__packed__))
{
    KEYSTORE_AES_ECB = 0U, /*!< AES_ECB 128bits. */
    KEYSTORE_XOR_ENC = 1U
} KEYSTORE_ENCRYPT_MODE;
/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup KEYSTORE_Exported_Constants KEYSTORE Exported Constants
 * @{
 */

/**
 * @brief KEYSTORE_HAL_ES_STA KeyStore Reture Value
 */
typedef enum __attribute__((__packed__))
{
    KEYSTORE_STA_SUCCESS                = 0U,  /*!< Success state. */
    KEYSTORE_STA_ERROR                  = 1U,  /*!< Error state. */
    KEYSTORE_STA_TIMEOUT_FAILURE        = 2U,  /*!< Timeout error. */
    KEYSTORE_STA_SEA_BUSY_FAILURE       = 3U,  /*!< SEA busy error. */
    KEYSTORE_STA_KINDEX_FAILURE         = 4U,  /*!< Invalid key index error. */
    KEYSTORE_STA_KEY_NULL_FAILURE       = 5U,  /*!< Keystore is full error. */
    KEYSTORE_STA_SECTOR_INDEX_FAILURE   = 6U,  /*!< Invalid sector index error. */
    KEYSTORE_STA_RD_OR_CS_FAILURE       = 7U,  /*!< Read or Modify operation error. */
    KEYSTORE_STA_NVR_NOEMPTY_FAILURE    = 8U,  /*!< Nvr is not empty error. */
    KEYSTORE_STA_PARM_FAILURE           = 9U,  /*!< Init parameters error. */
    KEYSTORE_STA_ROOTKY_FAILURE         = 10U, /*!< Rootkey error. */
	KEYSTORE_STA_RESERVED               = 11U,
    KEYSTORE_STA_ERR_PWD_FAILURE        = 12U, /*!< Password error. */
    KEYSTORE_STA_STA_CLR_FAILURE        = 13U, /*!< Clear status error. */
    KEYSTORE_STA_FLASH_CLK_FREQ_FAILURE = 14U, /*!< Flash clock frequency error. */
    KEYSTORE_STA_FLASH_RD_ONLY_FAILURE  = 15U  /*!< Flash write permission error. */
} KeyStore_Status;
/**
 * @}
 */

/* Exported macros ----------------------------------------------------------*/
/** @defgroup KEYSTORE_Exported_Macros KEYSTORE Exported Macros
 * @{
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup KEYSTORE_Exported_Functions KEYSTORE Exported Functions
 * @{
 */

/** @addtogroup KEYSTORE_Exported_Functions_Group1 Control functions
 * @{
 */
KeyStore_Status LL_KEYSTORE_Unlock(void);
KeyStore_Status LL_KEYSTORE_ReadKey(uint32_t *PKey, uint32_t KeyLen, const uint8_t KeyIdx);
KeyStore_Status LL_KEYSTORE_ReadKeyProperty(KeyStore_InitTypeDef *kp, const uint8_t KeyIdx);
KeyStore_Status LL_KEYSTORE_WriteKey(KeyStore_InitTypeDef *kp, const uint32_t *PKey, const uint8_t KeyIdx);
KeyStore_Status LL_KEYSTORE_Erase(uint32_t Passwd);
/**
 * @}
 */

/** @addtogroup KEYSTORE_Exported_Functions_Group2 State functions
 * @{
 */
uint32_t LL_KEYSTORE__GetState(void);
/**
 * @}
 */

/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/** @defgroup KEYSTORE_Private_Types KEYSTORE Private Types
 * @{
 */

/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/** @defgroup KEYSTORE_Private_Variables KEYSTORE Private Variables
 * @{
 */

/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup KEYSTORE_Private_Constants KEYSTORE Private Constants
 * @{
 */

/**
 * @}
 */

/* Private Macros -----------------------------------------------------------*/
/** @defgroup KEYSTORE_Private_Macros KEYSTORE Private Macros
 * @{
 */

/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup KEYSTORE_Private_Functions KEYSTORE Private Functions
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

#endif /* __RN_RV_HAL_KEYSTORE_H */
#endif
