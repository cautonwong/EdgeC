/**
 ******************************************************************************
 * @file   rn_rv_hal_gic.h
 * @author Renergy Software Team
 * @brief  Header file of GIC HAL module.
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
#ifndef __RN_RV_HAL_GIC_H
#define __RN_RV_HAL_GIC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal_def.h"
#include "rn_rv_ll_gic.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup GIC
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup GIC_Exported_Types GIC Exported Types
 * @{
 */
/**
 * @brief  IRQ Target Type enumeration
 */
typedef enum __attribute__((__packed__))
{
    HAL_GIC_IRQ_TARGET_NONE        = 0x00U,
    HAL_GIC_IRQ_TARGET_CORE0       = 0x01U,
    HAL_GIC_IRQ_TARGET_CORE1       = 0x02U,
    HAL_GIC_IRQ_TARGET_CORE0_CORE1 = 0x03U
} HAL_GIC_IRQTargetTypeDef;
/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup GIC_Exported_Constants GIC Exported Constants
 * @{
 */

/** @defgroup I2C_Flag_definition I2C Flag definition
 * @{
 */
#define GIC_ICI_FLAG_FROM_CORE0                   ((uint8_t)(0x00000000U))
#define GIC_ICI_FLAG_FROM_CORE1                   ((uint8_t)(0x00000004U))
#define GIC_ICI_FLAG_MASK                         ((uint8_t)(0x00000001U))
/**
 * @}
 */

/**
 * @}
 */
/* Exported macros ----------------------------------------------------------*/
/** @defgroup GIC_Exported_Macros GIC Exported Macros
 * @{
 */
/** @brief  Checks whether the specified ICI flag is set or not.
 *  @param  __FLAG__ specifies the ICI flag to check.
 *         This parameter can be one of the following values:
 *            @arg GIC_ICI_FLAG_FROM_CORE0: Inter-core interrupt Flag (sent from CORE0)
 *            @arg GIC_ICI_FLAG_FROM_CORE1: Inter-core interrupt Flag (sent from CORE1)
 * @retval The new state of __FLAG__ (TRUE or FALSE).
 */
#ifdef CORE0
#define __HAL_GIC_ICI_GET_FLAG(__FLAG__) (*(__IO uint8_t *)(GIC_ICI_BASE_ADDRESS + __FLAG__) == (GIC_ICI_FLAG_MASK))
#endif
#ifdef CORE1
#define __HAL_GIC_ICI_GET_FLAG(__FLAG__) (*(__IO uint8_t *)(GIC_ICI_BASE_ADDRESS + 0x10U + __FLAG__) == (GIC_ICI_FLAG_MASK))
#endif
/** @brief  Clears the I2C pending flags which are cleared by writing 0 in a specific bit.
 * @param  __FLAG__ specifies the flag to clear.
 *         This parameter can be one of the following values:
 *            @arg GIC_ICI_FLAG_FROM_CORE0: Inter-core interrupt Flag (sent from CORE0)
 *            @arg GIC_ICI_FLAG_FROM_CORE1: Inter-core interrupt Flag (sent from CORE1)
 * @retval None
 */
#ifdef CORE0
#define __HAL_GIC_ICI_CLEAR_FLAG(__FLAG__) (*(__IO uint8_t *)(GIC_ICI_BASE_ADDRESS + __FLAG__) = (GIC_ICI_FLAG_MASK))
#endif
#ifdef CORE1
#define __HAL_GIC_ICI_CLEAR_FLAG(__FLAG__) (*(__IO uint8_t *)(GIC_ICI_BASE_ADDRESS + 0x10U + __FLAG__) = (GIC_ICI_FLAG_MASK))
#endif
/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup GIC_Exported_Functions GIC Exported Functions
 * @{
 */

/** @addtogroup GIC_Exported_Functions_Group1
 * @{
 */
/* Initialization and de-initialization functions******************************/
HAL_StatusTypeDef HAL_GIC_ICI_Init(uint8_t Shv, ECLIC_TRIGGER_Type TriggerMode, uint8_t Level, uint8_t Priority);
void              HAL_GIC_ICI_DeInit(void);

/**
 * @}
 */

/** @addtogroup GIC_Exported_Functions_Group2
 * @{
 */
/* GIC Control functions ***************************************************/
HAL_StatusTypeDef HAL_GIC_ICI_Transmit(uint8_t CoreID, uint8_t Data);
HAL_StatusTypeDef HAL_GIC_ICI_Receive(uint8_t CoreID, uint8_t *pData);

/* GIC IRQHandler and Callbacks **************************************/
void              HAL_GIC_ICI_IRQHandler(void);
void              HAL_GIC_ICI_Core0ToCore0Callback(void);
void              HAL_GIC_ICI_Core1ToCore0Callback(void);
void              HAL_GIC_ICI_Core0ToCore1Callback(void);
void              HAL_GIC_ICI_Core1ToCore1Callback(void);
/**
 * @}
 */

/** @addtogroup GIC_Exported_Functions_Group3
 * @{
 */
HAL_StatusTypeDef        HAL_GIC_SetIrqTargetCore(IRQn_Type IRQn, HAL_GIC_IRQTargetTypeDef CoreTarget);
HAL_GIC_IRQTargetTypeDef HAL_GIC_GetIrqTargetCore(IRQn_Type IRQn);
HAL_StatusTypeDef        HAL_GIC_SetAllIrqTargetCore(HAL_GIC_IRQTargetTypeDef CoreTarget);
/**
 * @}
 */

/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/** @defgroup GIC_Private_Types GIC Private Types
 * @{
 */

/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/** @defgroup GIC_Private_Variables GIC Private Variables
 * @{
 */

/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup GIC_Private_Constants GIC Private Constants
 * @{
 */
#define GIC_ICI_BASE_ADDRESS          ((uint32_t)(GIC))
/**
 * @}
 */

/* Private Macros -----------------------------------------------------------*/
/** @defgroup GIC_Private_Macros GIC Private Macros
 * @{
 */

/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup GIC_Private_Functions GIC Private Functions
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

#endif /* __RN_RV_HAL_GIC_H */
