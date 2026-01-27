/**
 ******************************************************************************
 * @file   rn_rv_hal_scu.c
 * @author Renergy Software Team
 * @brief  SCU HAL module driver.
 *          This file provides firmware functions to manage the following
 *          functionalities of the internal SCU memory:
 *           + Program operations functions
 *           + Memory Control functions
 *           + Peripheral State functions
 *
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

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @defgroup SCU SCU
 * @brief     SCU driver modules
 * @{
 */

#ifdef HAL_SCU_MODULE_ENABLED

/* Private typedef ----------------------------------------------------------*/
/** @defgroup SCU_Private_Typedef SCU Private Typedef
 * @{
 */
/**
 * @}
 */

/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @defgroup SCU_Exported_Functions SCU Exported Functions
 * @{
 */

/** @defgroup SCU_Exported_Functions_Group1 Initialization and de-initialization functions
 *  @brief    Initialization and Configuration functions
 *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
    [..]
      This subsection provides a set of functions allowing to control the SCU
      operations.

@endverbatim
 * @{
 */

/**
 * @}
 */

/** @defgroup SCU_Exported_Functions_Group2 Peripheral Control functions
 *  @brief     management functions.
 *
@verbatim
================================================================================
                    ##### Peripheral Control functions #####
================================================================================
    [..]
      This subsection provides a set of functions allowing to control the SCU
      operations.

@endverbatim
 * @{
 */

/**
 * @brief  Set the secure SRAM size.
 * @note   If secure SRAM size is SRAMSize(KB), then Non-secure SRAM size
 *         is 160KB-SRAMSize(KB).
 * @note   Only can set in core0.
 * @param  SRAMSize Specify the SRAM size to set, see @ref HAL_SCU_SRAMSecLenTypeDef.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_SCU_SetSecureSRAMSize(HAL_SCU_SRAMSecLenTypeDef SRAMSize)
{
    /* Check the parameters */
    if ((SRAMSize == 0x0) || (SRAMSize > HAL_SCU_SRAM_SIZE_160KB)) {
        return HAL_ERROR;
    }

    __HAL_SCU_WKEY_UNLOCK();
    LL_SCU_SetSecuritySRAMWatermark(SRAMSize);
    __HAL_SCU_WKEY_LOCK();

    return HAL_OK;
}

/**
 * @brief  Set the Non-secure SRAM size.
 * @note   If Nonsecure SRAM size is SRAMSize(KB), then secure SRAM size
 *         is 160KB-SRAMSize(KB).
 * @note   Only can set in core0.
 *         Can not set to HAL_SCU_SRAM_SECURE_160KB.
 * @param  SRAMSize Specify the SRAM size to set, see @ref HAL_SCU_SRAMSecLenTypeDef.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_SCU_SetNonSecureSRAMSize(HAL_SCU_SRAMSecLenTypeDef SRAMSize)
{
    /* Check the parameters */
    if (SRAMSize >= HAL_SCU_SRAM_SIZE_160KB) {
        return HAL_ERROR;
    }
    __HAL_SCU_WKEY_UNLOCK();
    LL_SCU_SetSecuritySRAMWatermark(HAL_SCU_SRAM_SIZE_160KB - SRAMSize);
    __HAL_SCU_WKEY_LOCK();

    return HAL_OK;
}

/**
 * @brief  Get the secure SRAM size.
 * @retval Secure SRAM Size (KB).
 */
uint32_t HAL_SCU_GetSecureSRAMSize(void)
{
    return LL_SCU_GetSecuritySRAMWatermark(SCU) * 16U;
}

/**
 * @brief  Get the secure SRAM size.
 * @retval Secure SRAM Size (KB).
 */
uint32_t HAL_SCU_GetNonSecureSRAMSize(void)
{
    return (10 - LL_SCU_GetSecuritySRAMWatermark(SCU)) * 16U;
}

/**
 * @brief  This function handles SCU interrupt request.
 * @note   This API should be called under the SCU_HANDLER().
 * @retval None
 */
void HAL_SCU_IRQHandler(void)
{
    /* Check Peripheral0 Illegal Access interrupt flag */
    if (__HAL_SCU_PERIPH0_GET_IT()) {
        /* Peripheral0 Illegal Access interrupt user callback */
        HAL_SCU_Periph0IllegalAccessCallback();

        /* Clear Peripheral0 Illegal Access interrupt ALL pending bit */
        __HAL_SCU_PERIPH0_CLEAR_IT();
    }

    /* Check Peripheral1 Illegal Access interrupt flag */
    if (__HAL_SCU_PERIPH1_GET_IT()) {
        /* Peripheral1 Illegal Access interrupt user callback */
        HAL_SCU_Periph1IllegalAccessCallback();

        /* Clear Peripheral1 Illegal Access interrupt ALL pending bit */
        __HAL_SCU_PERIPH1_CLEAR_IT();
    }

    /* Check Peripheral2 Illegal Access interrupt flag */
    if (__HAL_SCU_PERIPH2_GET_IT()) {
        /* Peripheral2 Illegal Access interrupt user callback */
        HAL_SCU_Periph2IllegalAccessCallback();

        /* Clear Peripheral2 Illegal Access interrupt ALL pending bit */
        __HAL_SCU_PERIPH2_CLEAR_IT();
    }
}

/**
 * @brief  Peripheral0 Illegal Access interrupt callback
 * @retval none
 */
__WEAK void HAL_SCU_Periph0IllegalAccessCallback(void)
{
    /* NOTE : This function Should not be modified, when the callback is needed,
        the HAL_SCU_Periph0IllegalAccessCallback could be implemented in the user file
    */
}

/**
 * @brief  Peripheral1 Illegal Access interrupt callback
 * @retval none
 */
__WEAK void HAL_SCU_Periph1IllegalAccessCallback(void)
{
    /* NOTE : This function Should not be modified, when the callback is needed,
        the HAL_SCU_Periph1IllegalAccessCallback could be implemented in the user file
    */
}

/**
 * @brief  Peripheral2 Illegal Access interrupt callback
 * @retval none
 */
__WEAK void HAL_SCU_Periph2IllegalAccessCallback(void)
{
    /* NOTE : This function Should not be modified, when the callback is needed,
        the HAL_SCU_Periph2IllegalAccessCallback could be implemented in the user file
    */
}

/**
 * @}
 */


/** @defgroup SCU_Exported_Functions_Group3 Set and Get IRQ target functions
 *  @brief     Set and Get IRQ target functions
 *
@verbatim
@endverbatim
 * @{
 */

/**
 * @}
 */

/**
 * @}
 */

#endif /* HAL_SCU_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
