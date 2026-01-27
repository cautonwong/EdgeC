/**
 ******************************************************************************
 * @file   rn_rv_hal.h
 * @author Renergy Software Team
 * @brief  This file contains all the functions prototypes for the HAL
 *         module driver.
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
#ifndef __RN_HAL_H
#define __RN_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal_conf.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup HAL
 * @{
 */

#ifdef HAL_MODULE_ENABLED

/* Exported constants -------------------------------------------------------*/
/** @defgroup HAL_Exported_Constants HAL Exported Constants
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup HAL_Exported_Types HAL Exported Types
 * @{
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup HAL_Exported_Macros HAL Exported Macros
 * @{
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup HAL_Exported_Functions
 * @{
 */

/** @addtogroup HAL_Exported_Functions_Group1
 * @{
 */
HAL_StatusTypeDef HAL_Init(void);
HAL_StatusTypeDef HAL_DeInit(void);
void              HAL_MspInit(void);
void              HAL_MspDeInit(void);
HAL_StatusTypeDef HAL_InitTick(void);
/**
 * @}
 */

/** @addtogroup HAL_Exported_Functions_Group2
 * @{
 */
uint64_t          HAL_ReadCycles(void);
uint32_t          HAL_GetMillis(void);
uint32_t          HAL_GetMicros(void);
uint32_t          HAL_GetCoreId(void);
void              HAL_Delay(uint32_t Delay);
uint64_t          HAL_GetTicks(void);
uint32_t          HAL_GetHalVersion(void);
/**
 * @}
 */

/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/** @defgroup HAL_Private_Variables HAL Private Variables
 * @{
 */

/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup HAL_Private_Constants HAL Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup HAL_Private_Macros HAL Private Macros
 * @{
 */

/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @addtogroup HAL_Private_Functions HAL Private Functions
 * @{
 */
#ifdef CORE0
HAL_StatusTypeDef GetIAPHeaderAddress(uint32_t *Address);
#endif
/**
 * @}
 */

#endif /* HAL_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_HAL_H */
