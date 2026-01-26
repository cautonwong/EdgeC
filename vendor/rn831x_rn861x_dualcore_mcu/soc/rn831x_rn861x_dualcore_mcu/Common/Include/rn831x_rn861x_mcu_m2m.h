/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_m2m.h
 * @author  Renergy Software Team
 * @brief   M2M module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_M2M_H
#define _RN831x_RN861x_MCU_M2M_H

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

/* M2M */
typedef struct
{
    __IO uint32_t MODE;                      /*!< offset: 0x000 info: M2M Mode Register. */
    __IO uint32_t CTRL;                      /*!< offset: 0x004 info: M2M Control Register. */
    __IO uint32_t DUMMY;                     /*!< offset: 0x008 info: M2M invalid address Dummy value Register. */
    __IO uint32_t SADDR;                     /*!< offset: 0x00C info: M2M Source Data Address Register. */
    __IO uint32_t DADDR;                     /*!< offset: 0x010 info: M2M Destination Data Address Register. */
    __IO uint32_t ILEN;                      /*!< offset: 0x014 info: M2M Source Data Length Register. */
    __IO uint32_t IE;                        /*!< offset: 0x018 info: M2M Interrupt Enable Register. */
    __IO uint32_t IF;                        /*!< offset: 0x01C info: M2M Interrupt Flags Register. */
} M2M_TypeDef;

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
#define M2M_CORE0_BASE  (0x20078000U)
#define M2M_CORE1_BASE  (0xA0078000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define M2M_CORE0  ((M2M_TypeDef *)      M2M_CORE0_BASE)
#define M2M_CORE1  ((M2M_TypeDef *)      M2M_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define M2M  M2M_CORE0
#endif
#if defined(CORE1)
#define M2M  M2M_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** M2M Instances *******************************/
#define IS_M2M_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == M2M_CORE0) || \
                                          ((INSTANCE) == M2M_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* M2M_MODE */
#define M2M_MODE_IVLD                        (BITS(0,3))                                         /*!< Set input data valid in byte. */
#define M2M_MODE_OVLD                        (BITS(4,7))                                         /*!< Set output data valid in byte. */
#define M2M_MODE_ORV                         (BIT(8))                                            /*!< Reverse output data in byte. */
#define M2M_MODE_DUMMY                       (BIT(9))                                            /*!< Enable set dummy value. */
#define M2M_MODE_ADDRREV                     (BIT(10))                                           /*!< Reverse data by address. */

/* M2M_CTRL */
#define M2M_CTRL_EN                          (BIT(0))                                            /*!< Enable M2M running */

/* M2M_DUMMY */
#define M2M_DUMMY_VAL                        (BITS(0,31))                                        /*!< Set dummy value */

/* M2M_SADDR */
#define M2M_SADDR_ADDR                       (BITS(0,16))                                        /*!< Set source data address */

/* M2M_DADDR */
#define M2M_DADDR_ADDR                       (BITS(0,16))                                        /*!< Set destination data address */

/* M2M_ILEN */
#define M2M_ILEN_LEN                         (BITS(0,16))                                        /*!< Set data length */

/* M2M_IE */
#define M2M_IE_DONE                          (BIT(0))                                            /*!< Enable M2M interrupt */

/* M2M_IF */
#define M2M_IF_DONE                          (BIT(0))                                            /*!< Check M2M interrupt flag */

/* constants definitions */
/* M2M_MODE_IVLD */
#define M2M_MODE_IVLD_POS                    (0U)                                                /*!< M2M_MODE_IVLD bit offset */

/* M2M_MODE_OVLD */
#define M2M_MODE_OVLD_POS                    (4U)                                                /*!< M2M_MODE_OVLD bit offset */

/* M2M_MODE_ORV */
#define M2M_MODE_ORV_POS                     (8U)                                                /*!< M2M_MODE_ORV bit offset */

/* M2M_MODE_DUMMY */
#define M2M_MODE_DUMMY_POS                   (9U)                                                /*!< M2M_MODE_DUMMY bit offset */

/* M2M_MODE_ADDRREV */
#define M2M_MODE_ADDRREV_POS                 (10U)                                               /*!< M2M_MODE_ADDRREV bit offset */

/* M2M_CTRL_EN */
#define M2M_CTRL_EN_POS                      (0U)                                                /*!< M2M_CTRL_EN bit offset */

/* M2M_DUMMY_VAL */
#define M2M_DUMMY_VAL_POS                    (0U)                                                /*!< M2M_DUMMY_VAL bit offset */

/* M2M_SADDR_ADDR */
#define M2M_SADDR_ADDR_POS                   (0U)                                                /*!< M2M_SADDR_ADDR bit offset */

/* M2M_DADDR_ADDR */
#define M2M_DADDR_ADDR_POS                   (0U)                                                /*!< M2M_DADDR_ADDR bit offset */

/* M2M_ILEN_LEN */
#define M2M_ILEN_LEN_POS                     (0U)                                                /*!< M2M_ILEN_LEN bit offset */

/* M2M_IE_DONE */
#define M2M_IE_DONE_POS                      (0U)                                                /*!< M2M_IE_DONE bit offset */

/* M2M_IF_DONE */
#define M2M_IF_DONE_POS                      (0U)                                                /*!< M2M_IF_DONE bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_M2M_H */
