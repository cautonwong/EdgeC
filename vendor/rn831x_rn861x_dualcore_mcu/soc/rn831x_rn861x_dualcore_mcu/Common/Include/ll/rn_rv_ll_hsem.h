/**
 *****************************************************************************
 * @file   rn_rv_ll_hsem.h
 * @author Renergy Software Team
 * @brief  Header file of HSEM LL module.
 *
 *****************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 *****************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RN_LL_HSEM_H
#define __RN_LL_HSEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_hsem.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (HSEM)

/** @defgroup HSEM_LL HSEM
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup HSEM_LL_Private_Constants HSEM Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup HSEM_LL_Private_Macros HSEM Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup HSEM_LL_ES_INIT HSEM Exported Init structure
 * @{
 */

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup HSEM_LL_Exported_Constants HSEM Exported Constants
 * @{
 */

/** @defgroup HSEM_LL_EC_SEMAPHORE_ID SEMAPHORE_ID
 * @{
 */
#define LL_HSEM_SEMAPHORE_0              0U            /*!< Semaphore 0  */
#define LL_HSEM_SEMAPHORE_1              1U            /*!< Semaphore 1  */
#define LL_HSEM_SEMAPHORE_2              2U            /*!< Semaphore 2  */
#define LL_HSEM_SEMAPHORE_3              3U            /*!< Semaphore 3  */
#define LL_HSEM_SEMAPHORE_4              4U            /*!< Semaphore 4  */
#define LL_HSEM_SEMAPHORE_5              5U            /*!< Semaphore 5  */
#define LL_HSEM_SEMAPHORE_6              6U            /*!< Semaphore 6  */
#define LL_HSEM_SEMAPHORE_7              7U            /*!< Semaphore 7  */
#define LL_HSEM_SEMAPHORE_8              8U            /*!< Semaphore 8  */
#define LL_HSEM_SEMAPHORE_9              9U            /*!< Semaphore 9  */
#define LL_HSEM_SEMAPHORE_10             10U           /*!< Semaphore 10 */
#define LL_HSEM_SEMAPHORE_11             11U           /*!< Semaphore 11 */
#define LL_HSEM_SEMAPHORE_12             12U           /*!< Semaphore 12 */
#define LL_HSEM_SEMAPHORE_13             13U           /*!< Semaphore 13 */
#define LL_HSEM_SEMAPHORE_14             14U           /*!< Semaphore 14 */
#define LL_HSEM_SEMAPHORE_15             15U           /*!< Semaphore 15 */
#define LL_HSEM_SEMAPHORE_16             16U           /*!< Semaphore 16 */
#define LL_HSEM_SEMAPHORE_17             17U           /*!< Semaphore 17 */
#define LL_HSEM_SEMAPHORE_18             18U           /*!< Semaphore 18 */
#define LL_HSEM_SEMAPHORE_19             19U           /*!< Semaphore 19 */
#define LL_HSEM_SEMAPHORE_20             20U           /*!< Semaphore 20 */
#define LL_HSEM_SEMAPHORE_21             21U           /*!< Semaphore 21 */
#define LL_HSEM_SEMAPHORE_22             22U           /*!< Semaphore 22 */
#define LL_HSEM_SEMAPHORE_23             23U           /*!< Semaphore 23 */
#define LL_HSEM_SEMAPHORE_24             24U           /*!< Semaphore 24 */
#define LL_HSEM_SEMAPHORE_25             25U           /*!< Semaphore 25 */
#define LL_HSEM_SEMAPHORE_26             26U           /*!< Semaphore 26 */
#define LL_HSEM_SEMAPHORE_27             27U           /*!< Semaphore 27 */
#define LL_HSEM_SEMAPHORE_28             28U           /*!< Semaphore 28 */
#define LL_HSEM_SEMAPHORE_29             29U           /*!< Semaphore 29 */
#define LL_HSEM_SEMAPHORE_30             30U           /*!< Semaphore 30 */
#define LL_HSEM_SEMAPHORE_31             31U           /*!< Semaphore 31 */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup HSEM_LL_Exported_Macros HSEM Exported Macros
 * @{
 */
/** @defgroup HSEM_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in HSEM register
 * @param  __INSTANCE__ HSEM Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_HSEM_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in HSEM register
 * @param  __INSTANCE__ HSEM Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_HSEM_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup HSEM_LL_Exported_Functions HSEM Exported Functions
 * @{
 */

/** @defgroup HSEM_LL_EF_Data_Management Data_Management
 * @{
 */

/**
 * @brief  Return 1 if the semaphore is locked, else return 0.
 * @rmtoll SEMAPHOREx          SEMA_STATUS           LL_HSEM_IsLockedSemaphore
 * @param  Semaphore This parameter can be one of the following values:
 *         @arg @ref LL_HSEM_SEMAPHORE_0
 *         @arg @ref LL_HSEM_SEMAPHORE_1
 *         @arg @ref LL_HSEM_SEMAPHORE_2
 *         @arg @ref LL_HSEM_SEMAPHORE_3
 *         @arg @ref LL_HSEM_SEMAPHORE_4
 *         @arg @ref LL_HSEM_SEMAPHORE_5
 *         @arg @ref LL_HSEM_SEMAPHORE_6
 *         @arg @ref LL_HSEM_SEMAPHORE_7
 *         @arg @ref LL_HSEM_SEMAPHORE_8
 *         @arg @ref LL_HSEM_SEMAPHORE_9
 *         @arg @ref LL_HSEM_SEMAPHORE_10
 *         @arg @ref LL_HSEM_SEMAPHORE_11
 *         @arg @ref LL_HSEM_SEMAPHORE_12
 *         @arg @ref LL_HSEM_SEMAPHORE_13
 *         @arg @ref LL_HSEM_SEMAPHORE_14
 *         @arg @ref LL_HSEM_SEMAPHORE_15
 *         @arg @ref LL_HSEM_SEMAPHORE_16
 *         @arg @ref LL_HSEM_SEMAPHORE_17
 *         @arg @ref LL_HSEM_SEMAPHORE_18
 *         @arg @ref LL_HSEM_SEMAPHORE_19
 *         @arg @ref LL_HSEM_SEMAPHORE_20
 *         @arg @ref LL_HSEM_SEMAPHORE_21
 *         @arg @ref LL_HSEM_SEMAPHORE_22
 *         @arg @ref LL_HSEM_SEMAPHORE_23
 *         @arg @ref LL_HSEM_SEMAPHORE_24
 *         @arg @ref LL_HSEM_SEMAPHORE_25
 *         @arg @ref LL_HSEM_SEMAPHORE_26
 *         @arg @ref LL_HSEM_SEMAPHORE_27
 *         @arg @ref LL_HSEM_SEMAPHORE_28
 *         @arg @ref LL_HSEM_SEMAPHORE_29
 *         @arg @ref LL_HSEM_SEMAPHORE_30
 *         @arg @ref LL_HSEM_SEMAPHORE_31
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_HSEM_IsSemaphoreLocked(uint32_t Semaphore)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&HSEM->SEMAPHORE[0]) + (4 * Semaphore)));
    return (READ_BIT(*pReg, HSEM_SEMAPHORE_SEMA_STATUS) != 0x3FFU);
}

/**
 * @brief  Release Semaphore, write to 0x3FFU.
 * @rmtoll SEMAPHOREx          SEMA_STATUS           LL_HSEM_TryLockSemaphore
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_HSEM_SEMAPHORE_0
 *         @arg @ref LL_HSEM_SEMAPHORE_1
 *         @arg @ref LL_HSEM_SEMAPHORE_2
 *         @arg @ref LL_HSEM_SEMAPHORE_3
 *         @arg @ref LL_HSEM_SEMAPHORE_4
 *         @arg @ref LL_HSEM_SEMAPHORE_5
 *         @arg @ref LL_HSEM_SEMAPHORE_6
 *         @arg @ref LL_HSEM_SEMAPHORE_7
 *         @arg @ref LL_HSEM_SEMAPHORE_8
 *         @arg @ref LL_HSEM_SEMAPHORE_9
 *         @arg @ref LL_HSEM_SEMAPHORE_10
 *         @arg @ref LL_HSEM_SEMAPHORE_11
 *         @arg @ref LL_HSEM_SEMAPHORE_12
 *         @arg @ref LL_HSEM_SEMAPHORE_13
 *         @arg @ref LL_HSEM_SEMAPHORE_14
 *         @arg @ref LL_HSEM_SEMAPHORE_15
 *         @arg @ref LL_HSEM_SEMAPHORE_16
 *         @arg @ref LL_HSEM_SEMAPHORE_17
 *         @arg @ref LL_HSEM_SEMAPHORE_18
 *         @arg @ref LL_HSEM_SEMAPHORE_19
 *         @arg @ref LL_HSEM_SEMAPHORE_20
 *         @arg @ref LL_HSEM_SEMAPHORE_21
 *         @arg @ref LL_HSEM_SEMAPHORE_22
 *         @arg @ref LL_HSEM_SEMAPHORE_23
 *         @arg @ref LL_HSEM_SEMAPHORE_24
 *         @arg @ref LL_HSEM_SEMAPHORE_25
 *         @arg @ref LL_HSEM_SEMAPHORE_26
 *         @arg @ref LL_HSEM_SEMAPHORE_27
 *         @arg @ref LL_HSEM_SEMAPHORE_28
 *         @arg @ref LL_HSEM_SEMAPHORE_29
 *         @arg @ref LL_HSEM_SEMAPHORE_30
 *         @arg @ref LL_HSEM_SEMAPHORE_31
 * @retval None
 */
__STATIC_INLINE void LL_HSEM_ReleaseLockSemaphore(uint32_t Semaphore)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&HSEM->SEMAPHORE[0]) + (4 * Semaphore)));
    MODIFY_REG(*pReg, HSEM_SEMAPHORE_SEMA_STATUS, 0x3FFU);
}

/**
 * @brief  Try Get Semaphore, 1 lock successful, 0 lock successful or already locked by same core
 * @note   When a Core get the semaphore by writing Non 0x3FF, and must be released by current Core by writing 0x3FF,
 *         writing 0x3FF by other core is invalid.
 * @rmtoll SEMAPHOREx          SEMA_STATUS           LL_HSEM_TryLockSemaphore
 * @param  Semaphore This parameter can be one of the following values:
 *         @arg @ref LL_HSEM_SEMAPHORE_0
 *         @arg @ref LL_HSEM_SEMAPHORE_1
 *         @arg @ref LL_HSEM_SEMAPHORE_2
 *         @arg @ref LL_HSEM_SEMAPHORE_3
 *         @arg @ref LL_HSEM_SEMAPHORE_4
 *         @arg @ref LL_HSEM_SEMAPHORE_5
 *         @arg @ref LL_HSEM_SEMAPHORE_6
 *         @arg @ref LL_HSEM_SEMAPHORE_7
 *         @arg @ref LL_HSEM_SEMAPHORE_8
 *         @arg @ref LL_HSEM_SEMAPHORE_9
 *         @arg @ref LL_HSEM_SEMAPHORE_10
 *         @arg @ref LL_HSEM_SEMAPHORE_11
 *         @arg @ref LL_HSEM_SEMAPHORE_12
 *         @arg @ref LL_HSEM_SEMAPHORE_13
 *         @arg @ref LL_HSEM_SEMAPHORE_14
 *         @arg @ref LL_HSEM_SEMAPHORE_15
 *         @arg @ref LL_HSEM_SEMAPHORE_16
 *         @arg @ref LL_HSEM_SEMAPHORE_17
 *         @arg @ref LL_HSEM_SEMAPHORE_18
 *         @arg @ref LL_HSEM_SEMAPHORE_19
 *         @arg @ref LL_HSEM_SEMAPHORE_20
 *         @arg @ref LL_HSEM_SEMAPHORE_21
 *         @arg @ref LL_HSEM_SEMAPHORE_22
 *         @arg @ref LL_HSEM_SEMAPHORE_23
 *         @arg @ref LL_HSEM_SEMAPHORE_24
 *         @arg @ref LL_HSEM_SEMAPHORE_25
 *         @arg @ref LL_HSEM_SEMAPHORE_26
 *         @arg @ref LL_HSEM_SEMAPHORE_27
 *         @arg @ref LL_HSEM_SEMAPHORE_28
 *         @arg @ref LL_HSEM_SEMAPHORE_29
 *         @arg @ref LL_HSEM_SEMAPHORE_30
 *         @arg @ref LL_HSEM_SEMAPHORE_31
 * @param  Value between Min_Data=0x00 and Max_Data=0x1FF (Non 0x3FF).
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_HSEM_TryLockSemaphore(uint32_t Semaphore, uint32_t Value)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&HSEM->SEMAPHORE[0]) + (4 * Semaphore)));
    MODIFY_REG(*pReg, HSEM_SEMAPHORE_SEMA_STATUS, Value);
    return (READ_BIT(*pReg, HSEM_SEMAPHORE_SEMA_STATUS) == Value);
}

/**
 * @brief  get semaphore status.
 * @rmtoll SEMAPHOREx          SEMA_STATUS           LL_HSEM_GetSemaphoreStatus
 * @param  Semaphore This parameter can be one of the following values:
 *         @arg @ref LL_HSEM_SEMAPHORE_0
 *         @arg @ref LL_HSEM_SEMAPHORE_1
 *         @arg @ref LL_HSEM_SEMAPHORE_2
 *         @arg @ref LL_HSEM_SEMAPHORE_3
 *         @arg @ref LL_HSEM_SEMAPHORE_4
 *         @arg @ref LL_HSEM_SEMAPHORE_5
 *         @arg @ref LL_HSEM_SEMAPHORE_6
 *         @arg @ref LL_HSEM_SEMAPHORE_7
 *         @arg @ref LL_HSEM_SEMAPHORE_8
 *         @arg @ref LL_HSEM_SEMAPHORE_9
 *         @arg @ref LL_HSEM_SEMAPHORE_10
 *         @arg @ref LL_HSEM_SEMAPHORE_11
 *         @arg @ref LL_HSEM_SEMAPHORE_12
 *         @arg @ref LL_HSEM_SEMAPHORE_13
 *         @arg @ref LL_HSEM_SEMAPHORE_14
 *         @arg @ref LL_HSEM_SEMAPHORE_15
 *         @arg @ref LL_HSEM_SEMAPHORE_16
 *         @arg @ref LL_HSEM_SEMAPHORE_17
 *         @arg @ref LL_HSEM_SEMAPHORE_18
 *         @arg @ref LL_HSEM_SEMAPHORE_19
 *         @arg @ref LL_HSEM_SEMAPHORE_20
 *         @arg @ref LL_HSEM_SEMAPHORE_21
 *         @arg @ref LL_HSEM_SEMAPHORE_22
 *         @arg @ref LL_HSEM_SEMAPHORE_23
 *         @arg @ref LL_HSEM_SEMAPHORE_24
 *         @arg @ref LL_HSEM_SEMAPHORE_25
 *         @arg @ref LL_HSEM_SEMAPHORE_26
 *         @arg @ref LL_HSEM_SEMAPHORE_27
 *         @arg @ref LL_HSEM_SEMAPHORE_28
 *         @arg @ref LL_HSEM_SEMAPHORE_29
 *         @arg @ref LL_HSEM_SEMAPHORE_30
 *         @arg @ref LL_HSEM_SEMAPHORE_31
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FF.
 */
__STATIC_INLINE uint32_t LL_HSEM_GetSemaphoreStatus(uint32_t Semaphore)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&HSEM->SEMAPHORE[0]) + (4 * Semaphore)));
    return (READ_BIT(*pReg, HSEM_SEMAPHORE_SEMA_STATUS));
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup HSEM_LL_EF_Init Initialization and de-initialization functions
 * @{
 */

// The initialization function can have more than one argument.
ErrorStatus LL_HSEM_DeInit(HSEM_TypeDef *HSEMx);

/**
 * @}
 */
#endif /* USE_FULL_LL_DRIVER */

/**
 * @}
 */

/**
 * @}
 */

#endif /* HSEM */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_HSEM_H */
