/**
 ******************************************************************************
 * @file   rn_rv_hal_hsem.h
 * @author Renergy Software Team
 * @brief  Header file of HSEM HAL module.
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
#ifndef __RN_RV_HAL_HSEM_H
#define __RN_RV_HAL_HSEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal_def.h"
#include "rn_rv_ll_hsem.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup HSEM
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup HSEM_Exported_Types HSEM Exported Types
 * @{
 */

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup HSEM_Exported_Constants HSEM Exported Constants
 * @{
 */

/**
 * @}
 */
/* Exported macros ----------------------------------------------------------*/
/** @defgroup HSEM_Exported_Macros HSEM Exported Macros
 * @{
 */

/**
 * @}
 */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup HSEM_Exported_Functions HSEM Exported Functions
 * @{
 */

/** @addtogroup HSEM_Exported_Functions_Group1
 * @{
 */
/* Initialization and de-initialization functions ****************************/
HAL_StatusTypeDef  HAL_HSEM_Init(void);
HAL_StatusTypeDef  HAL_HSEM_DeInit(void);
/**
 * @}
 */

/** @addtogroup HSEM_Exported_Functions_Group2
 * @{
 */
/* HSEM semaphore take (lock) using 2-Step method ****************************/
HAL_StatusTypeDef  HAL_HSEM_Take(uint32_t SemID, uint32_t ProcessID);
/* HSEM Check semaphore state Taken or not ***********************************/
uint32_t           HAL_HSEM_IsSemTaken(uint32_t SemID);
/* HSEM Release **************************************************************/
HAL_StatusTypeDef  HAL_HSEM_Release(uint32_t SemID);
HAL_StatusTypeDef  HAL_HSEM_ReleaseAll(void);
/**
 * @}
 */

/** @addtogroup HSEM_Exported_Functions_Group3
 * @{
 */

/**
 * @}
 */

/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/** @defgroup HSEM_Private_Types HSEM Private Types
 * @{
 */

/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/** @defgroup HSEM_Private_Variables HSEM Private Variables
 * @{
 */

/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup HSEM_Private_Constants HSEM Private Constants
 * @{
 */
#define HSEM_SEMID_MIN       (0U)       /* HSEM ID Min */
#define HSEM_SEMID_MAX       (31U)      /* HSEM ID Max */

#define HSEM_PROCESSID_MIN   (0U)       /* HSEM Process ID Min */
#define HSEM_PROCESSID_MAX   (255U)     /* HSEM Process ID Max */

#define HSEM_UNLOCKED_MSK    (0x3FFU)   /* HSEM Unlocked MASK */

#define HSEM_PROCESSID_POS   (0U)
#define HSEM_COREID_POS      (8U)
/**
 * @}
 */

/* Private Macros -----------------------------------------------------------*/
/** @defgroup HSEM_Private_Macros HSEM Private Macros
 * @{
 */
#define IS_HSEM_SEMID(__SEMID__)         ((__SEMID__) <= HSEM_SEMID_MAX)
#define IS_HSEM_PROCESSID(__PROCESSID__) ((__PROCESSID__) <= HSEM_PROCESSID_MAX)
/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup CEC_Private_Functions CEC Private Functions
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

#endif /* __RN_RV_HAL_HSEM_H */
