/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_hsem.h
 * @author  Renergy Software Team
 * @brief   HSEM module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_HSEM_H
#define _RN831x_RN861x_MCU_HSEM_H

#include "rn831x_rn861x_mcu.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/*                Device Specific Peripheral registers structures             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Peripherals rn831x_rn861x_mcu Peripherals
 * @brief rn831x_rn861x_mcu Device Specific Peripheral registers structures
 * @{
 */

/* HSEM */
typedef struct
{
    __IO uint32_t SEMAPHORE[32];                /*!< offset: 0x000 ~ 0x07C info: Hardware Semaphore Register 0 */
} HSEM_TypeDef;

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_Peripherals */

/******************************************************************************/
/*                         Peripheral memory map                              */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_MemoryMap rn831x_rn861x_mcu Memory Mapping
 * @{
 */

/* base address */
#define HSEM_BASE       (0xB0084000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define HSEM       ((HSEM_TypeDef *)     HSEM_BASE)


/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** HSEM Instances *******************************/
#define IS_HSEM_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == HSEM))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* HSEM_SEMAPHORE0 */
#define HSEM_SEMAPHORE_SEMA_STATUS          (BITS(0,9))                                         /*!< Writing a non-0x3FF value indicates that a specific CORE get a semaphore */

/* constants definitions */
/* HSEM_SEMAPHORE0_SEMA_STATUS */
#define HSEM_SEMAPHORE_SEMA_STATUS_POS      (0U)                                                /*!< HSEM_SEMAPHORE0_SEMA_STATUS bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_HSEM_H */
