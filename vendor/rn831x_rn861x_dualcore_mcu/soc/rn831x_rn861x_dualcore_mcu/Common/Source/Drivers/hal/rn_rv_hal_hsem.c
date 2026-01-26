/**
 ******************************************************************************
 * @file   rn_rv_hal_hsem.c
 * @author Renergy Software Team
 * @brief  HSEM HAL module driver.
 *          This file provides firmware functions to manage the following
 *          functionalities of the internal HSEM memory:
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

/** @defgroup HSEM HSEM
 * @brief     HSEM driver modules
 * @{
 */
#ifdef HAL_HSEM_MODULE_ENABLED

/* Private typedef ----------------------------------------------------------*/
/** @defgroup HSEM_Private_Typedef HSEM Private Typedef
 * @{
 */

/**
 * @}
 */
/* Private define -----------------------------------------------------------*/
/** @defgroup HSEM_Private_Constants HSEM Private Constants
 * @{
 */

/**
 * @}
 */
/* Private macro ------------------------------------------------------------*/
/** @defgroup HSEM_Private_Macros HSEM Private Macros
 * @{
 */

/**
 * @}
 */
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/** @defgroup HSEM_Private_Functions HSEM Private Functions
 * @{
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup HSEM_Exported_Functions HSEM Exported Functions
 * @{
 */

/** @defgroup HSEM_Exported_Functions_Group1 Take and Release functions
 *  @brief    HSEM Take and Release functions
 *
@verbatim
 ==============================================================================
              ##### HSEM Take and Release functions #####
 ==============================================================================
    [..] This subsection provides a set of functions allowing to initialize and
          deinitialize the HSEM peripheral:
      (+) enable clock

@endverbatim
 * @{
 */

/**
 * @brief  Initializes the HSEM.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_HSEM_Init(void)
{
    /* HSEM clock enable */
    LL_SYSTEM_CORE1_UnlockSystemPSW();
    LL_SYSTEM_CORE1_PMOD0_EnableClock(LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_HSEM);
    LL_SYSTEM_CORE1_LockSystemPSW();
    return HAL_OK;
}

/**
 * @brief  DeInitialize the HSEM peripheral (in core1).
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_HSEM_DeInit(void)
{
#ifdef USE_FULL_LL_DRIVER
    if ((HAL_StatusTypeDef)LL_HSEM_DeInit(HSEM) != HAL_OK) {
        return HAL_ERROR;
    }
#endif
    /* Peripheral clock disable */
    LL_SYSTEM_CORE1_PMOD0_DisableClock(LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_HSEM);

    return HAL_OK;
}

/**
 * @}
 */

/** @defgroup HSEM_Exported_Functions_Group2 Take and Release functions
 *  @brief    HSEM Take and Release functions
 *
@verbatim

@endverbatim
 * @{
 */

/**
 * @brief  Take a semaphore, register bits8~9 is Current CoreID, bit0~7 is ProcessID.
 * @param  SemID: semaphore ID from 0 to HSEM_SEMID_MAX (31U)
 * @param  ProcessID: Process ID from 0 to HSEM_PROCESSID_MAX (0xFFU)
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_HSEM_Take(uint32_t SemID, uint32_t ProcessID)
{
    /* Check the parameters */
    assert_param(IS_HSEM_SEMID(SemID));
    assert_param(IS_HSEM_PROCESSID(ProcessID));

    uint32_t reg, coreID;

    coreID = HAL_GetCoreId();
    reg = ProcessID << HSEM_PROCESSID_POS;

    /* Current if not core0 */
    if (coreID != 0U) {
        reg |= coreID << HSEM_COREID_POS;
    }

    if (LL_HSEM_TryLockSemaphore(SemID, reg) == 1U) {
        return HAL_OK;
    }

    /* Semaphore take fails*/
    return HAL_ERROR;
}

/**
 * @brief  Check semaphore state Taken or not.
 * @param  SemID: semaphore ID from 0 to HSEM_SEMID_MAX (31U)
 * @retval HAL HSEM state (1: semaphore is taken  -  0: semaphore is free)
 */
uint32_t HAL_HSEM_IsSemTaken(uint32_t SemID)
{
    /* Check the parameters */
    assert_param(IS_HSEM_SEMID(SemID));

    return LL_HSEM_IsSemaphoreLocked(SemID);
}

/**
 * @brief  Release a semaphore.
 * @param  SemID: semaphore ID from 0 to HSEM_SEMID_MAX (31U)
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_HSEM_Release(uint32_t SemID)
{
    /* Check the parameters */
    assert_param(IS_HSEM_SEMID(SemID));

    LL_HSEM_ReleaseLockSemaphore(SemID);

    if (LL_HSEM_GetSemaphoreStatus(SemID) != HSEM_UNLOCKED_MSK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief  Release all semaphores.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_HSEM_ReleaseAll(void)
{
    uint32_t i;

    for (i = 0; i <= HSEM_SEMID_MAX; i++) {
        LL_HSEM_ReleaseLockSemaphore(i);

        if (LL_HSEM_GetSemaphoreStatus(i) != HSEM_UNLOCKED_MSK) {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}

/**
 * @}
 */

/** @defgroup HSEM_Exported_Functions_Group3
 *  @brief    HSEM Group3 functions
 *
@verbatim
================================================================================
                    ##### HSEM management functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+)                   :

@endverbatim
 * @{
 */

/**
 * @}
 */

/**
 * @}
 */
#endif /* HAL_HSEM_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
