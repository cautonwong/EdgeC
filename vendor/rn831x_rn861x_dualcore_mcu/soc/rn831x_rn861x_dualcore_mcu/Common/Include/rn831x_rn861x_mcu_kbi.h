/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_kbi.h
 * @author  Renergy Software Team
 * @brief   KBI module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_KBI_H
#define _RN831x_RN861x_MCU_KBI_H

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

/* KBI */
typedef struct
{
    __IO uint32_t CTRL;                      /*!< offset: 0x000 info: Control Register */
    __IO uint32_t MODE;                      /*!< offset: 0x004 info: Mode Register */
    __IO uint32_t RESERVED0;                 /*!< offset: 0x008 */
    __IO uint32_t MASK;                      /*!< offset: 0x00C info: Mask Register */
    __IO uint32_t STA;                       /*!< offset: 0x010 info: Interrupt status Register */
    __IO uint32_t CH_FLT_TIME[16];           /*!< offset: 0x014 ~ 0x050 info: Channel 0 ~ 15 Filtering time configure Register */
} KBI_TypeDef;

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
#define KBI0_CORE0_BASE (0x20064000U)
#define KBI1_CORE0_BASE (0x20068000U)
#define KBI0_CORE1_BASE (0xA0064000U)
#define KBI1_CORE1_BASE (0xA0068000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define KBI0_CORE0 ((KBI_TypeDef *)      KBI0_CORE0_BASE)
#define KBI1_CORE0 ((KBI_TypeDef *)      KBI1_CORE0_BASE)
#define KBI0_CORE1 ((KBI_TypeDef *)      KBI0_CORE1_BASE)
#define KBI1_CORE1 ((KBI_TypeDef *)      KBI1_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define KBI0  KBI0_CORE0
#define KBI1  KBI1_CORE0
#endif
#if defined(CORE1)
#define KBI0  KBI0_CORE1
#define KBI1  KBI1_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** KBI Instances *******************************/
#define IS_KBI_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == KBI0_CORE0) || \
                                          ((INSTANCE) == KBI1_CORE0) || \
                                          ((INSTANCE) == KBI0_CORE1) || \
                                          ((INSTANCE) == KBI1_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* KBI_CTRL */
#define KBI_CTRL_EN                          (BITS(0,15))                                        /*!< Enable/Disable KBI channel */

/* KBI_MODE */
#define KBI_MODE_MODE0                       (BITS(0,1))                                         /*!< KBI channel 0 interrupt input valid mode selection */
#define KBI_MODE_MODE1                       (BITS(2,3))                                         /*!< KBI channel 1 interrupt input valid mode selection */
#define KBI_MODE_MODE2                       (BITS(4,5))                                         /*!< KBI channel 2 interrupt input valid mode selection */
#define KBI_MODE_MODE3                       (BITS(6,7))                                         /*!< KBI channel 3 interrupt input valid mode selection */
#define KBI_MODE_MODE4                       (BITS(8,9))                                         /*!< KBI channel 4 interrupt input valid mode selection */
#define KBI_MODE_MODE5                       (BITS(10,11))                                       /*!< KBI channel 5 interrupt input valid mode selection */
#define KBI_MODE_MODE6                       (BITS(12,13))                                       /*!< KBI channel 6 interrupt input valid mode selection */
#define KBI_MODE_MODE7                       (BITS(14,15))                                       /*!< KBI channel 7 interrupt input valid mode selection */
#define KBI_MODE_MODE8                       (BITS(16,17))                                       /*!< KBI channel 8 interrupt input valid mode selection */
#define KBI_MODE_MODE9                       (BITS(18,19))                                       /*!< KBI channel 9 interrupt input valid mode selection */
#define KBI_MODE_MODE10                      (BITS(20,21))                                       /*!< KBI channel 10 interrupt input valid mode selection */
#define KBI_MODE_MODE11                      (BITS(22,23))                                       /*!< KBI channel 11 interrupt input valid mode selection */
#define KBI_MODE_MODE12                      (BITS(24,25))                                       /*!< KBI channel 12 interrupt input valid mode selection */
#define KBI_MODE_MODE13                      (BITS(26,27))                                       /*!< KBI channel 13 interrupt input valid mode selection */
#define KBI_MODE_MODE14                      (BITS(28,29))                                       /*!< KBI channel 14 interrupt input valid mode selection */
#define KBI_MODE_MODE15                      (BITS(30,31))                                       /*!< KBI channel 15 interrupt input valid mode selection */

/* KBI_MASK */
#define KBI_MASK_MASK                        (BITS(0,15))                                        /*!< Enable/Disable KBI channel interrupt */

/* KBI_STA */
#define KBI_STA_STA                          (BITS(0,15))                                        /*!< Flag for KBI channel interrupt */

/* KBI_CH_FLT_TIME */
#define KBI_CH_FLT_TIME_CLK_DIV2             (BITS(0,4))                                         /*!< Filter clock second-level frequency division, if set to 0, it means turning off the channel input filter */
#define KBI_CH_FLT_TIME_CLK_DIV1             (BIT(5))                                            /*!< Filter clock first-level frequency division */
#define KBI_CH_FLT_TIME_CLK_SEL              (BIT(6))                                            /*!< Filter clock selection */

/* constants definitions */
/* KBI_CTRL_EN */
#define KBI_CTRL_EN_POS                      (0U)                                                /*!< KBI_CTRL_EN bit offset */

/* KBI_MODE_MODE0 */
#define KBI_MODE_MODE0_POS                   (0U)                                                /*!< KBI_MODE_MODE0 bit offset */
#define KBI_MODE_MODE0_CFG(regval)  (KBI_MODE_MODE0 & ((uint32_t)(regval) << KBI_MODE_MODE0_POS))
#define KBI_MODE_MODE0_RISING                (KBI_MODE_MODE0_CFG(0x0))                           /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE0_FALLING               (KBI_MODE_MODE0_CFG(0x1))                           /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE0_RISING_FALLING        (KBI_MODE_MODE0_CFG(0x2))                           /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE1 */
#define KBI_MODE_MODE1_POS                   (2U)                                                /*!< KBI_MODE_MODE1 bit offset */
#define KBI_MODE_MODE1_CFG(regval)  (KBI_MODE_MODE1 & ((uint32_t)(regval) << KBI_MODE_MODE1_POS))
#define KBI_MODE_MODE1_RISING                (KBI_MODE_MODE1_CFG(0x0))                           /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE1_FALLING               (KBI_MODE_MODE1_CFG(0x1))                           /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE1_RISING_FALLING        (KBI_MODE_MODE1_CFG(0x2))                           /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE2 */
#define KBI_MODE_MODE2_POS                   (4U)                                                /*!< KBI_MODE_MODE2 bit offset */
#define KBI_MODE_MODE2_CFG(regval)  (KBI_MODE_MODE2 & ((uint32_t)(regval) << KBI_MODE_MODE2_POS))
#define KBI_MODE_MODE2_RISING                (KBI_MODE_MODE2_CFG(0x0))                           /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE2_FALLING               (KBI_MODE_MODE2_CFG(0x1))                           /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE2_RISING_FALLING        (KBI_MODE_MODE2_CFG(0x2))                           /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE3 */
#define KBI_MODE_MODE3_POS                   (6U)                                                /*!< KBI_MODE_MODE3 bit offset */
#define KBI_MODE_MODE3_CFG(regval)  (KBI_MODE_MODE3 & ((uint32_t)(regval) << KBI_MODE_MODE3_POS))
#define KBI_MODE_MODE3_RISING                (KBI_MODE_MODE3_CFG(0x0))                           /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE3_FALLING               (KBI_MODE_MODE3_CFG(0x1))                           /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE3_RISING_FALLING        (KBI_MODE_MODE3_CFG(0x2))                           /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE4 */
#define KBI_MODE_MODE4_POS                   (8U)                                                /*!< KBI_MODE_MODE4 bit offset */
#define KBI_MODE_MODE4_CFG(regval)  (KBI_MODE_MODE4 & ((uint32_t)(regval) << KBI_MODE_MODE4_POS))
#define KBI_MODE_MODE4_RISING                (KBI_MODE_MODE4_CFG(0x0))                           /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE4_FALLING               (KBI_MODE_MODE4_CFG(0x1))                           /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE4_RISING_FALLING        (KBI_MODE_MODE4_CFG(0x2))                           /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE5 */
#define KBI_MODE_MODE5_POS                   (10U)                                               /*!< KBI_MODE_MODE5 bit offset */
#define KBI_MODE_MODE5_CFG(regval)  (KBI_MODE_MODE5 & ((uint32_t)(regval) << KBI_MODE_MODE5_POS))
#define KBI_MODE_MODE5_RISING                (KBI_MODE_MODE5_CFG(0x0))                           /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE5_FALLING               (KBI_MODE_MODE5_CFG(0x1))                           /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE5_RISING_FALLING        (KBI_MODE_MODE5_CFG(0x2))                           /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE6 */
#define KBI_MODE_MODE6_POS                   (12U)                                               /*!< KBI_MODE_MODE6 bit offset */
#define KBI_MODE_MODE6_CFG(regval)  (KBI_MODE_MODE6 & ((uint32_t)(regval) << KBI_MODE_MODE6_POS))
#define KBI_MODE_MODE6_RISING                (KBI_MODE_MODE6_CFG(0x0))                           /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE6_FALLING               (KBI_MODE_MODE6_CFG(0x1))                           /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE6_RISING_FALLING        (KBI_MODE_MODE6_CFG(0x2))                           /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE7 */
#define KBI_MODE_MODE7_POS                   (14U)                                               /*!< KBI_MODE_MODE7 bit offset */
#define KBI_MODE_MODE7_CFG(regval)  (KBI_MODE_MODE7 & ((uint32_t)(regval) << KBI_MODE_MODE7_POS))
#define KBI_MODE_MODE7_RISING                (KBI_MODE_MODE7_CFG(0x0))                           /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE7_FALLING               (KBI_MODE_MODE7_CFG(0x1))                           /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE7_RISING_FALLING        (KBI_MODE_MODE7_CFG(0x2))                           /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE8 */
#define KBI_MODE_MODE8_POS                   (16U)                                               /*!< KBI_MODE_MODE8 bit offset */
#define KBI_MODE_MODE8_CFG(regval)  (KBI_MODE_MODE8 & ((uint32_t)(regval) << KBI_MODE_MODE8_POS))
#define KBI_MODE_MODE8_RISING                (KBI_MODE_MODE8_CFG(0x0))                           /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE8_FALLING               (KBI_MODE_MODE8_CFG(0x1))                           /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE8_RISING_FALLING        (KBI_MODE_MODE8_CFG(0x2))                           /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE9 */
#define KBI_MODE_MODE9_POS                   (18U)                                               /*!< KBI_MODE_MODE9 bit offset */
#define KBI_MODE_MODE9_CFG(regval)  (KBI_MODE_MODE9 & ((uint32_t)(regval) << KBI_MODE_MODE9_POS))
#define KBI_MODE_MODE9_RISING                (KBI_MODE_MODE9_CFG(0x0))                           /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE9_FALLING               (KBI_MODE_MODE9_CFG(0x1))                           /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE9_RISING_FALLING        (KBI_MODE_MODE9_CFG(0x2))                           /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE10 */
#define KBI_MODE_MODE10_POS                  (20U)                                               /*!< KBI_MODE_MODE10 bit offset */
#define KBI_MODE_MODE10_CFG(regval)  (KBI_MODE_MODE10 & ((uint32_t)(regval) << KBI_MODE_MODE10_POS))
#define KBI_MODE_MODE10_RISING               (KBI_MODE_MODE10_CFG(0x0))                          /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE10_FALLING              (KBI_MODE_MODE10_CFG(0x1))                          /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE10_RISING_FALLING       (KBI_MODE_MODE10_CFG(0x2))                          /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE11 */
#define KBI_MODE_MODE11_POS                  (22U)                                               /*!< KBI_MODE_MODE11 bit offset */
#define KBI_MODE_MODE11_CFG(regval)  (KBI_MODE_MODE11 & ((uint32_t)(regval) << KBI_MODE_MODE11_POS))
#define KBI_MODE_MODE11_RISING               (KBI_MODE_MODE11_CFG(0x0))                          /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE11_FALLING              (KBI_MODE_MODE11_CFG(0x1))                          /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE11_RISING_FALLING       (KBI_MODE_MODE11_CFG(0x2))                          /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE12 */
#define KBI_MODE_MODE12_POS                  (24U)                                               /*!< KBI_MODE_MODE12 bit offset */
#define KBI_MODE_MODE12_CFG(regval)  (KBI_MODE_MODE12 & ((uint32_t)(regval) << KBI_MODE_MODE12_POS))
#define KBI_MODE_MODE12_RISING               (KBI_MODE_MODE12_CFG(0x0))                          /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE12_FALLING              (KBI_MODE_MODE12_CFG(0x1))                          /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE12_RISING_FALLING       (KBI_MODE_MODE12_CFG(0x2))                          /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE13 */
#define KBI_MODE_MODE13_POS                  (26U)                                               /*!< KBI_MODE_MODE13 bit offset */
#define KBI_MODE_MODE13_CFG(regval)  (KBI_MODE_MODE13 & ((uint32_t)(regval) << KBI_MODE_MODE13_POS))
#define KBI_MODE_MODE13_RISING               (KBI_MODE_MODE13_CFG(0x0))                          /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE13_FALLING              (KBI_MODE_MODE13_CFG(0x1))                          /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE13_RISING_FALLING       (KBI_MODE_MODE13_CFG(0x2))                          /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE14 */
#define KBI_MODE_MODE14_POS                  (28U)                                               /*!< KBI_MODE_MODE14 bit offset */
#define KBI_MODE_MODE14_CFG(regval)  (KBI_MODE_MODE14 & ((uint32_t)(regval) << KBI_MODE_MODE14_POS))
#define KBI_MODE_MODE14_RISING               (KBI_MODE_MODE14_CFG(0x0))                          /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE14_FALLING              (KBI_MODE_MODE14_CFG(0x1))                          /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE14_RISING_FALLING       (KBI_MODE_MODE14_CFG(0x2))                          /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MODE_MODE15 */
#define KBI_MODE_MODE15_POS                  (30U)                                               /*!< KBI_MODE_MODE15 bit offset */
#define KBI_MODE_MODE15_CFG(regval)  (KBI_MODE_MODE15 & ((uint32_t)(regval) << KBI_MODE_MODE15_POS))
#define KBI_MODE_MODE15_RISING               (KBI_MODE_MODE15_CFG(0x0))                          /*!< Trigger Rising Edge Mode */
#define KBI_MODE_MODE15_FALLING              (KBI_MODE_MODE15_CFG(0x1))                          /*!< Trigger Falling Edge Mode */
#define KBI_MODE_MODE15_RISING_FALLING       (KBI_MODE_MODE15_CFG(0x2))                          /*!< Trigger Rising & Falling Edge Mode */

/* KBI_MASK_MASK */
#define KBI_MASK_MASK_POS                    (0U)                                                /*!< KBI_MASK_MASK bit offset */

/* KBI_STA_STA */
#define KBI_STA_STA_POS                      (0U)                                                /*!< KBI_STA_STA bit offset */

/* KBI_CH_FLT_TIME_CLK_DIV2 */
#define KBI_CH_FLT_TIME_CLK_DIV2_POS        (0U)                                                /*!< KBI_CH_FLT_TIME_CLK_DIV2 bit offset */

/* KBI_CH_FLT_TIME_CLK_DIV1 */
#define KBI_CH_FLT_TIME_CLK_DIV1_POS        (5U)                                                /*!< KBI_CH_FLT_TIME_CLK_DIV1 bit offset */

/* KBI_CH_FLT_TIME_CLK_SEL */
#define KBI_CH_FLT_TIME_CLK_SEL_POS         (6U)                                                /*!< KBI_CH_FLT_TIME_CLK_SEL bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_KBI_H */
