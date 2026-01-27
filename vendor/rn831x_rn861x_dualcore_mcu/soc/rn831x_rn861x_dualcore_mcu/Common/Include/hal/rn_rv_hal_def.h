/**
 ******************************************************************************
 * @file   rn_rv_hal_def.h
 * @author Renergy Software Team
 * @brief  This file contains HAL common defines, enumeration, macros and
 *         structures definitions.
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
#ifndef __RN_HAL_DEF
#define __RN_HAL_DEF

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu.h"
#include <stddef.h>

/* Exported types -----------------------------------------------------------*/
/**
 * @brief  HAL Status structures definition
 */
typedef enum __attribute__((__packed__))
{
    HAL_OK       = 0x00U,
    HAL_ERROR    = 0x01U,
    HAL_BUSY     = 0x02U,
    HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

/**
 * @brief  HAL Lock structures definition
 */
typedef enum __attribute__((__packed__))
{
    HAL_UNLOCKED = 0x00U,
    HAL_LOCKED   = 0x01U
} HAL_LockTypeDef;

/* Exported macro -----------------------------------------------------------*/
#define HAL_MAX_DELAY      0xFFFFFFFFU

#define HAL_IS_BIT_SET(REG, BIT)         (((REG) & (BIT)) != 0U)
#define HAL_IS_BIT_CLR(REG, BIT)         (((REG) & (BIT)) == 0U)

#define __HAL_LINKDMA(__HANDLE__, __PPP_DMA_FIELD__, __DMA_HANDLE__)             \
                        do{                                                      \
                            (__HANDLE__)->__PPP_DMA_FIELD__ = &(__DMA_HANDLE__); \
                            (__DMA_HANDLE__).Parent = (__HANDLE__);              \
                        } while(0U)

#if !defined(UNUSED)
#define UNUSED(X) (void)X      /* To avoid gcc/g++ warnings */
#endif /* UNUSED */

/**
 * @brief  Reset the Handle's State field.
 * @param  __HANDLE__ specifies the Peripheral Handle.
 * @note   This macro can be used for the following purpose:
 *          - When the Handle is declared as local variable; before passing it as parameter
 *            to HAL_PPP_Init() for the first time, it is mandatory to use this macro
 *            to set to 0 the Handle's "State" field.
 *            Otherwise, "State" field may have any random value and the first time the function
 *            HAL_PPP_Init() is called, the low level hardware initialization will be missed
 *            (i.e. HAL_PPP_MspInit() will not be executed).
 *          - When there is a need to reconfigure the low level hardware: instead of calling
 *            HAL_PPP_DeInit() then HAL_PPP_Init(), user can make a call to this macro then HAL_PPP_Init().
 *            In this later function, when the Handle's "State" field is set to 0, it will execute the function
 *            HAL_PPP_MspInit() which will reconfigure the low level hardware.
 * @retval None
 */
#define __HAL_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = 0U)

#define __HAL_LOCK(__HANDLE__)                                             \
                                do{                                        \
                                    if((__HANDLE__)->Lock == HAL_LOCKED)   \
                                    {                                      \
                                       return HAL_BUSY;                    \
                                    }                                      \
                                    else                                   \
                                    {                                      \
                                       (__HANDLE__)->Lock = HAL_LOCKED;    \
                                    }                                      \
                                  }while (0U)

#define __HAL_UNLOCK(__HANDLE__)                                           \
                                  do{                                      \
                                      (__HANDLE__)->Lock = HAL_UNLOCKED;   \
                                    }while (0U)

#ifdef __cplusplus
}
#endif

#endif /* ___RN_HAL_DEF */
