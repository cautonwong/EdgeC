/**
 ******************************************************************************
 * @file   rn_rv_hal_flash.h
 * @author Renergy Software Team
 * @brief  Header file of FLASH HAL module.
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
#ifndef __RN_RV_HAL_FLASH_H
#define __RN_RV_HAL_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal_def.h"
#include "rn831x_rn861x_mcu_flash.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup FLASH
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup FLASH_Exported_Types FLASH Exported Types
 * @{
 */

/**
 * @brief  HAL FLASH ID enumeration definition
 */
typedef enum __attribute__((__packed__))
{
    HAL_FLASH_ID_FLASH0 = 0x00U,    /*!< FLASH0 ID (core0)              */
    HAL_FLASH_ID_FLASH1 = 0x01U,    /*!< FLASH1 ID (core1)              */
    HAL_FLASH_ID_NVR    = 0x02U,    /*!< NVR ID (only support NVR2/3/4) */
} HAL_FLASH_IDTypeDef;

/**
 * @brief  HAL FLASH Status enumeration definition
 */
typedef enum __attribute__((__packed__))
{
    HAL_FALSH_CMD_SUCCESS         = 0x00U,
    HAL_FALSH_INVALID_COMMAND     = 0x01U,
    HAL_FALSH_INVALID_PARAM       = 0x02U,
    HAL_FALSH_INVALID_BAUD_RATE   = 0x03U,
    HAL_FALSH_INVALID_STOP_BIT    = 0x04U,
    HAL_FALSH_ADDR_NOT_ALIGN      = 0x05U,
    HAL_FALSH_COUNT_ERROR         = 0x06U,
    HAL_FALSH_ADDR_NOT_MAPPED     = 0x07U,
    HAL_FALSH_INVALID_PAGES       = 0x08U,
    HAL_FALSH_SECTOR_NOT_BLANK    = 0x09U,
    HAL_FALSH_SRC_ADDR_NOT_ALIGN  = 0x0AU,
    HAL_FALSH_SRC_ADDR_NOT_MAPPED = 0x0BU,
    HAL_FALSH_DST_ADDR_NOT_ALIGN  = 0x0CU,
    HAL_FALSH_DST_ADDR_NOT_MAPPED = 0x0DU,
    HAL_FALSH_COMPARE_ERROR       = 0x0EU,
    HAL_FALSH_FM_MODE_ERROR       = 0x0FU,
    HAL_FALSH_ADDR_NOT_THUMB      = 0x10U,
    HAL_FALSH_INVALID_PASS        = 0x11U,
    HAL_FALSH_INVALID_DOTA0       = 0x12U,
    HAL_FALSH_INVALID_FOUT        = 0x13U,
    HAL_FALSH_LOWPWR_ERROR        = 0x14U
} HAL_FLASH_StatusTypeDef;

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup FLASH_Exported_Constants FLASH Exported Constants
 * @{
 */

/**
 * @}
 */
/* Exported macros ----------------------------------------------------------*/
/** @defgroup FLASH_Exported_Macros FLASH Exported Macros
 * @{
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup FLASH_Exported_Functions FLASH Exported Functions
 * @{
 */

/** @addtogroup FLASH_Exported_Functions_Group1
 * @{
 */

/* Flash Control functions ***************************************************/
HAL_FLASH_StatusTypeDef HAL_FLASH_Unlock(HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t ByteLength);

/**
 * @}
 */

/** @addtogroup FLASH_Exported_Functions_Group2
 * @{
 */

/* Program operation functions ***********************************************/
HAL_FLASH_StatusTypeDef HAL_FLASH_Program(HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t SrcAddress, uint32_t ByteLength);
/* Erase operation functions *************************************************/
HAL_FLASH_StatusTypeDef HAL_FLASH_Erase(HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t ByteLength);

/**
 * @}
 */

/** @addtogroup FLASH_Exported_Functions_Group3
 * @{
 */

/* ISP operation functions ***************************************************/
HAL_FLASH_StatusTypeDef HAL_FLASH_ClearSoftISP(uint32_t ISPProtect);
HAL_FLASH_StatusTypeDef HAL_FLASH_EnableSoftISP(uint32_t ISPCommand, uint32_t UartIOCommand);
/* CP operation functions ****************************************************/
HAL_FLASH_StatusTypeDef HAL_FLASH_ClearCP(uint32_t JtagMode, uint32_t Core0JtagEnable, uint32_t Core1JtagEnable);

/**
 * @}
 */

/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/** @defgroup FLASH_Private_Types FLASH Private Types
 * @{
 */

/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/** @defgroup FLASH_Private_Variables FLASH Private Variables
 * @{
 */

/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup FLASH_Private_Constants FLASH Private Constants
 * @{
 */

/**
 * @}
 */

/* Private Macros -----------------------------------------------------------*/
/** @defgroup FLASH_Private_Macros FLASH Private Macros
 * @{
 */

/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup FLASH_Private_Functions FLASH Private Functions
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

#endif /* __RN_RV_HAL_FLASH_H */
