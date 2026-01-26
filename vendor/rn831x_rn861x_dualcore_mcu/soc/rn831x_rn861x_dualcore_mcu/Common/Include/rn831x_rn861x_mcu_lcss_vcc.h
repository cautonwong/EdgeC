/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_lcss_vcc.h
 * @author  Renergy Software Team
 * @brief   LCSS_VCC module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_LCSS_VCC_H
#define _RN831x_RN861x_MCU_LCSS_VCC_H

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

/* LCSS_VCC */
typedef struct
{
    __IO uint32_t PSW0;                      /*!< offset: 0x000 info: Security key register 0 */
    __IO uint32_t PSW1;                      /*!< offset: 0x004 info: Security key register 1 */
    __IO uint32_t EN;                        /*!< offset: 0x008 info: Enable register */
    __IO uint32_t ASWH;                      /*!< offset: 0x00C info: Auto switch configuration register */
    __IO uint32_t CTRL;                      /*!< offset: 0x010 info: Control register */
    __IO uint32_t RCL_OS;                    /*!< offset: 0x014 info: RCL32K frequency offset register */
    __IO uint32_t HFDLA_TH;                  /*!< offset: 0x018 info: High frequency abnormal offset threshold configuration register */
    __IO uint32_t LFDLA_TH;                  /*!< offset: 0x01C info: Low frequency abnormal offset threshold configuration register */
    __IO uint32_t CLKSWH;                    /*!< offset: 0x020 info: Clock configuration register for software switch */
    __IO uint32_t CLKSEL;                    /*!< offset: 0x024 info: Clock selected register, indicate the selected clock */
    __IO uint32_t IE;                        /*!< offset: 0x028 info: Interrupt enable register */
    __IO uint32_t IF;                        /*!< offset: 0x02C info: Status register */
    __IO uint32_t HFREQ;                     /*!< offset: 0x030 info: High frequency abnormal frequency register */
    __IO uint32_t LFREQ;                     /*!< offset: 0x034 info: Low frequency abnormal frequency register */
} LCSS_VCC_TypeDef;

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
#define LCSS_VCC_CORE0_BASE (0x2010C000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define LCSS_VCC_CORE0 ((LCSS_VCC_TypeDef *) LCSS_VCC_CORE0_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define LCSS_VCC  LCSS_VCC_CORE0
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** LCSS_VCC Instances *******************************/
#define IS_LCSS_VCC_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == LCSS_VCC_CORE0))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* LCSS_VCC_PSW0 */
#define LCSS_VCC_PSW0_PSW                    (BITS(0,15))                                        /*!< Managed registers include LCSS_EN/LCSS_ASWH/LCSS_CTRL/LCSS_RCL_OS/LCSS_HFDLA_TH/LCSS_LFDLA_TH/LCSS_IE */

/* LCSS_VCC_PSW1 */
#define LCSS_VCC_PSW1_PSW                    (BITS(0,15))                                        /*!< Managed registers include LCSS_CLKSWH */

/* LCSS_VCC_EN */
#define LCSS_VCC_EN_HFDT_EN                  (BIT(0))                                            /*!< High frequency abnormal detection enable */
#define LCSS_VCC_EN_LFDT_EN                  (BIT(1))                                            /*!< Low frequency abnormal detection enable */

/* LCSS_VCC_ASWH */
#define LCSS_VCC_ASWH_HF_ASWH0               (BIT(0))                                            /*!< (System clock: PLL to RCH) Enable automatic switch to backup clock in high frequency abnormality */
#define LCSS_VCC_ASWH_HF_ASWH1               (BIT(1))                                            /*!< (PLL source: LOSC to RCL) Enable automatic switch to backup clock in high frequency abnormality */
#define LCSS_VCC_ASWH_HF_ASWH2               (BIT(2))                                            /*!< (KBI Clock: LOSC to RCL) Enable automatic switch to backup clock in high frequency abnormality */
#define LCSS_VCC_ASWH_HF_ASWH3               (BIT(3))                                            /*!< (LPUART Clock: LOSC to RCL) Enable automatic switch to backup clock in high frequency abnormality */
#define LCSS_VCC_ASWH_HF_ASWH4               (BIT(4))                                            /*!< (LCD Clock: LOSC to RCL) Enable automatic switch to backup clock in high frequency abnormality */
#define LCSS_VCC_ASWH_HF_ASWH5               (BIT(5))                                            /*!< (Infrared Clock: LOSC to RCL) Enable automatic switch to backup clock in high frequency abnormality */
#define LCSS_VCC_ASWH_HF_ASWH6               (BIT(6))                                            /*!< (System clock: LOSC to RCL) Enable automatic switch to backup clock in high frequency abnormality */
#define LCSS_VCC_ASWH_LF_ASWH0               (BIT(8))                                            /*!< (System clock: PLL to RCH) Enable automatic switch to backup clock in low frequency abnormality */
#define LCSS_VCC_ASWH_LF_ASWH1               (BIT(9))                                            /*!< (PLL source: LOSC to RCL) Enable automatic switch to backup clock in low frequency abnormality */
#define LCSS_VCC_ASWH_LF_ASWH2               (BIT(10))                                           /*!< (KBI Clock: LOSC to RCL) Enable automatic switch to backup clock in low frequency abnormality */
#define LCSS_VCC_ASWH_LF_ASWH3               (BIT(11))                                           /*!< (LPUART Clock: LOSC to RCL) Enable automatic switch to backup clock in low frequency abnormality */
#define LCSS_VCC_ASWH_LF_ASWH4               (BIT(12))                                           /*!< (LCD Clock: LOSC to RCL) Enable automatic switch to backup clock in low frequency abnormality */
#define LCSS_VCC_ASWH_LF_ASWH5               (BIT(13))                                           /*!< (Infrared Clock: LOSC to RCL) Enable automatic switch to backup clock in low frequency abnormality */
#define LCSS_VCC_ASWH_LF_ASWH6               (BIT(14))                                           /*!< (System clock: LOSC to RCL) Enable automatic switch to backup clock in low frequency abnormality */
#define LCSS_VCC_ASWH_CK_ARCV0               (BIT(16))                                           /*!< (System clock: PLL to RCH) Enable automatic switch to backup clock in low frequency abnormality */
#define LCSS_VCC_ASWH_CK_ARCV1               (BIT(17))                                           /*!< (PLL source: LOSC to RCL)  Enable automatic switch to source clock in normal frequency */
#define LCSS_VCC_ASWH_CK_ARCV2               (BIT(18))                                           /*!< (KBI Clock: LOSC to RCL)  Enable automatic switch to source clock in normal frequency */
#define LCSS_VCC_ASWH_CK_ARCV3               (BIT(19))                                           /*!< (LPUART Clock: LOSC to RCL)  Enable automatic switch to source clock in normal frequency */
#define LCSS_VCC_ASWH_CK_ARCV4               (BIT(20))                                           /*!< (LCD Clock: LOSC to RCL)  Enable automatic switch to source clock in normal frequency */
#define LCSS_VCC_ASWH_CK_ARCV5               (BIT(21))                                           /*!< (Infrared Clock: LOSC to RCL)  Enable automatic switch to source clock in normal frequency */
#define LCSS_VCC_ASWH_CK_ARCV6               (BIT(22))                                           /*!< (System clock: LOSC to RCL)  Enable automatic switch to source clock in normal frequency */

/* LCSS_VCC_CTRL */
#define LCSS_VCC_CTRL_DETD_DIV               (BITS(0,1))                                         /*!< Clock frequency division configuration */
#define LCSS_VCC_CTRL_HF_CYC                 (BITS(4,7))                                         /*!< High frequency/low frequency abnormality threshold */
#define LCSS_VCC_CTRL_RCV_CYC                (BITS(16,31))                                       /*!< LOSC recovery time configuration */

/* LCSS_VCC_RCL_OS */
#define LCSS_VCC_RCL_OS_RCL_OS               (BITS(0,7))                                         /*!< This register is configurable only when LCSS_EN=0 */

/* LCSS_VCC_HFDLA_TH */
#define LCSS_VCC_HFDLA_TH_HFDLA_TH           (BITS(0,7))                                         /*!< This register is configurable only when LCSS_EN=0 */

/* LCSS_VCC_LFDLA_TH */
#define LCSS_VCC_LFDLA_TH_LFDLA_TH           (BITS(0,7))                                         /*!< This register is configurable only when LCSS_EN=0 */

/* LCSS_VCC_CLKSWH */
#define LCSS_VCC_CLKSWH_CLK0_SWH1            (BIT(0))                                            /*!< (System clock: PLL to RCH) software switch to backup clock */
#define LCSS_VCC_CLKSWH_CLK1_SWH1            (BIT(1))                                            /*!< (PLL source: LOSC to RCL) software switch to backup clock */
#define LCSS_VCC_CLKSWH_CLK2_SWH1            (BIT(2))                                            /*!< (KBI Clock: LOSC to RCL) software switch to backup clock */
#define LCSS_VCC_CLKSWH_CLK3_SWH1            (BIT(3))                                            /*!< (LPUART Clock: LOSC to RCL) software switch to backup clock */
#define LCSS_VCC_CLKSWH_CLK4_SWH1            (BIT(4))                                            /*!< (LCD Clock: LOSC to RCL) software switch to backup clock */
#define LCSS_VCC_CLKSWH_CLK5_SWH1            (BIT(5))                                            /*!< (Infrared Clock: LOSC to RCL) software switch to backup clock */
#define LCSS_VCC_CLKSWH_CLK6_SWH1            (BIT(6))                                            /*!< (System clock: LOSC to RCL) software switch to backup clock */
#define LCSS_VCC_CLKSWH_CLK0_SWH0            (BIT(8))                                            /*!< (System clock: RCH to PLL) software switch to source clock */
#define LCSS_VCC_CLKSWH_CLK1_SWH0            (BIT(9))                                            /*!< (PLL source: RCL to LOSC) software switch to source clock */
#define LCSS_VCC_CLKSWH_CLK2_SWH0            (BIT(10))                                           /*!< (KBI Clock: RCL to LOSC) software switch to source clock */
#define LCSS_VCC_CLKSWH_CLK3_SWH0            (BIT(11))                                           /*!< (LPUART Clock: RCL to LOSC) software switch to source clock */
#define LCSS_VCC_CLKSWH_CLK4_SWH0            (BIT(12))                                           /*!< (LCD Clock: RCL to LOSC) software switch to source clock */
#define LCSS_VCC_CLKSWH_CLK5_SWH0            (BIT(13))                                           /*!< (Infrared Clock: RCL to LOSC) software switch to source clock */
#define LCSS_VCC_CLKSWH_CLK6_SWH0            (BIT(14))                                           /*!< (System clock: RCL to LOSC) software switch to source clock */

/* LCSS_VCC_CLKSEL */
#define LCSS_VCC_CLKSEL_CLKSEL0              (BIT(0))                                            /*!< (System clock: PLL to RCH) Indicate the current clock selection, source clock or backup clock */
#define LCSS_VCC_CLKSEL_CLKSEL1              (BIT(1))                                            /*!< (PLL source: LOSC to RCL) Indicate the current clock selection, source clock or backup clock */
#define LCSS_VCC_CLKSEL_CLKSEL2              (BIT(2))                                            /*!< (KBI Clock: LOSC to RCL) Indicate the current clock selection, source clock or backup clock */
#define LCSS_VCC_CLKSEL_CLKSEL3              (BIT(3))                                            /*!< (LPUART Clock: LOSC to RCL) Indicate the current clock selection, source clock or backup clock */
#define LCSS_VCC_CLKSEL_CLKSEL4              (BIT(4))                                            /*!< (LCD Clock: LOSC to RCL) Indicate the current clock selection, source clock or backup clock */
#define LCSS_VCC_CLKSEL_CLKSEL5              (BIT(5))                                            /*!< (Infrared Clock: LOSC to RCL) Indicate the current clock selection, source clock or backup clock */
#define LCSS_VCC_CLKSEL_CLKSEL6              (BIT(6))                                            /*!< (System clock: LOSC to RCL) Indicate the current clock selection, source clock or backup clock */

/* LCSS_VCC_IE */
#define LCSS_VCC_IE_HFREQ_IE                 (BIT(0))                                            /*!< Clock high frequency abnormal interrupt enable */
#define LCSS_VCC_IE_LFREQ_IE                 (BIT(1))                                            /*!< Clock low frequency abnormal interrupt enable */
#define LCSS_VCC_IE_LOSCOK_IE                (BIT(2))                                            /*!< LOSC clock is OK interrupt enable */

/* LCSS_VCC_IF */
#define LCSS_VCC_IF_HFREQ_IF                 (BIT(0))                                            /*!< The flag for the high-frequency abnormal event of the clock */
#define LCSS_VCC_IF_LFREQ_IF                 (BIT(1))                                            /*!< The flag for the low-frequency abnormal event of the clock */
#define LCSS_VCC_IF_FRCV_IF                  (BIT(2))                                            /*!< The flag for the normal (recovery) frequency event of the clock */
#define LCSS_VCC_IF_LOSC_OK                  (BIT(3))                                            /*!< Indicates the current LOSC clock frequency status */

/* LCSS_VCC_HFREQ */
#define LCSS_VCC_HFREQ_HFREQ                 (BITS(0,7))                                         /*!< Set maximum high frequency abnormal frequency value */

/* LCSS_VCC_LFREQ */
#define LCSS_VCC_LFREQ_LFREQ                 (BITS(0,7))                                         /*!< Set minimum low frequency abnormal frequency value */

/* constants definitions */
/* LCSS_VCC_PSW0_PSW */
#define LCSS_VCC_PSW0_PSW_POS                (0U)                                                /*!< LCSS_VCC_PSW0_PSW bit offset */

/* LCSS_VCC_PSW1_PSW */
#define LCSS_VCC_PSW1_PSW_POS                (0U)                                                /*!< LCSS_VCC_PSW1_PSW bit offset */

/* LCSS_VCC_EN_HFDT_EN */
#define LCSS_VCC_EN_HFDT_EN_POS              (0U)                                                /*!< LCSS_VCC_EN_HFDT_EN bit offset */

/* LCSS_VCC_EN_LFDT_EN */
#define LCSS_VCC_EN_LFDT_EN_POS              (1U)                                                /*!< LCSS_VCC_EN_LFDT_EN bit offset */

/* LCSS_VCC_ASWH_HF_ASWH0 */
#define LCSS_VCC_ASWH_HF_ASWH0_POS           (0U)                                                /*!< LCSS_VCC_ASWH_HF_ASWH0 bit offset */

/* LCSS_VCC_ASWH_HF_ASWH1 */
#define LCSS_VCC_ASWH_HF_ASWH1_POS           (1U)                                                /*!< LCSS_VCC_ASWH_HF_ASWH1 bit offset */

/* LCSS_VCC_ASWH_HF_ASWH2 */
#define LCSS_VCC_ASWH_HF_ASWH2_POS           (2U)                                                /*!< LCSS_VCC_ASWH_HF_ASWH2 bit offset */

/* LCSS_VCC_ASWH_HF_ASWH3 */
#define LCSS_VCC_ASWH_HF_ASWH3_POS           (3U)                                                /*!< LCSS_VCC_ASWH_HF_ASWH3 bit offset */

/* LCSS_VCC_ASWH_HF_ASWH4 */
#define LCSS_VCC_ASWH_HF_ASWH4_POS           (4U)                                                /*!< LCSS_VCC_ASWH_HF_ASWH4 bit offset */

/* LCSS_VCC_ASWH_HF_ASWH5 */
#define LCSS_VCC_ASWH_HF_ASWH5_POS           (5U)                                                /*!< LCSS_VCC_ASWH_HF_ASWH5 bit offset */

/* LCSS_VCC_ASWH_HF_ASWH6 */
#define LCSS_VCC_ASWH_HF_ASWH6_POS           (6U)                                                /*!< LCSS_VCC_ASWH_HF_ASWH6 bit offset */

/* LCSS_VCC_ASWH_LF_ASWH0 */
#define LCSS_VCC_ASWH_LF_ASWH0_POS           (8U)                                                /*!< LCSS_VCC_ASWH_LF_ASWH0 bit offset */

/* LCSS_VCC_ASWH_LF_ASWH1 */
#define LCSS_VCC_ASWH_LF_ASWH1_POS           (9U)                                                /*!< LCSS_VCC_ASWH_LF_ASWH1 bit offset */

/* LCSS_VCC_ASWH_LF_ASWH2 */
#define LCSS_VCC_ASWH_LF_ASWH2_POS           (10U)                                               /*!< LCSS_VCC_ASWH_LF_ASWH2 bit offset */

/* LCSS_VCC_ASWH_LF_ASWH3 */
#define LCSS_VCC_ASWH_LF_ASWH3_POS           (11U)                                               /*!< LCSS_VCC_ASWH_LF_ASWH3 bit offset */

/* LCSS_VCC_ASWH_LF_ASWH4 */
#define LCSS_VCC_ASWH_LF_ASWH4_POS           (12U)                                               /*!< LCSS_VCC_ASWH_LF_ASWH4 bit offset */

/* LCSS_VCC_ASWH_LF_ASWH5 */
#define LCSS_VCC_ASWH_LF_ASWH5_POS           (13U)                                               /*!< LCSS_VCC_ASWH_LF_ASWH5 bit offset */

/* LCSS_VCC_ASWH_LF_ASWH6 */
#define LCSS_VCC_ASWH_LF_ASWH6_POS           (14U)                                               /*!< LCSS_VCC_ASWH_LF_ASWH6 bit offset */

/* LCSS_VCC_ASWH_CK_ARCV0 */
#define LCSS_VCC_ASWH_CK_ARCV0_POS           (16U)                                               /*!< LCSS_VCC_ASWH_CK_ARCV0 bit offset */

/* LCSS_VCC_ASWH_CK_ARCV1 */
#define LCSS_VCC_ASWH_CK_ARCV1_POS           (17U)                                               /*!< LCSS_VCC_ASWH_CK_ARCV1 bit offset */

/* LCSS_VCC_ASWH_CK_ARCV2 */
#define LCSS_VCC_ASWH_CK_ARCV2_POS           (18U)                                               /*!< LCSS_VCC_ASWH_CK_ARCV2 bit offset */

/* LCSS_VCC_ASWH_CK_ARCV3 */
#define LCSS_VCC_ASWH_CK_ARCV3_POS           (19U)                                               /*!< LCSS_VCC_ASWH_CK_ARCV3 bit offset */

/* LCSS_VCC_ASWH_CK_ARCV4 */
#define LCSS_VCC_ASWH_CK_ARCV4_POS           (20U)                                               /*!< LCSS_VCC_ASWH_CK_ARCV4 bit offset */

/* LCSS_VCC_ASWH_CK_ARCV5 */
#define LCSS_VCC_ASWH_CK_ARCV5_POS           (21U)                                               /*!< LCSS_VCC_ASWH_CK_ARCV5 bit offset */

/* LCSS_VCC_ASWH_CK_ARCV6 */
#define LCSS_VCC_ASWH_CK_ARCV6_POS           (22U)                                               /*!< LCSS_VCC_ASWH_CK_ARCV6 bit offset */

/* LCSS_VCC_CTRL_DETD_DIV */
#define LCSS_VCC_CTRL_DETD_DIV_POS           (0U)                                                /*!< LCSS_VCC_CTRL_DETD_DIV bit offset */
#define LCSS_VCC_CTRL_DETD_DIV_CFG(regval)  (LCSS_VCC_CTRL_DETD_DIV & ((uint32_t)(regval) << LCSS_VCC_CTRL_DETD_DIV_POS))
#define LCSS_VCC_CTRL_DETD_DIV_NONE          (LCSS_VCC_CTRL_DETD_DIV_CFG(0x0))                   /*!< No Frequency division */
#define LCSS_VCC_CTRL_DETD_DIV_2             (LCSS_VCC_CTRL_DETD_DIV_CFG(0x1))                   /*!< 2 Frequency division */
#define LCSS_VCC_CTRL_DETD_DIV_4             (LCSS_VCC_CTRL_DETD_DIV_CFG(0x2))                   /*!< 4 Frequency division */
#define LCSS_VCC_CTRL_DETD_DIV_8             (LCSS_VCC_CTRL_DETD_DIV_CFG(0x3))                   /*!< 8 Frequency division */

/* LCSS_VCC_CTRL_HF_CYC */
#define LCSS_VCC_CTRL_HF_CYC_POS             (4U)                                                /*!< LCSS_VCC_CTRL_HF_CYC bit offset */

/* LCSS_VCC_CTRL_RCV_CYC */
#define LCSS_VCC_CTRL_RCV_CYC_POS            (16U)                                               /*!< LCSS_VCC_CTRL_RCV_CYC bit offset */

/* LCSS_VCC_RCL_OS_RCL_OS */
#define LCSS_VCC_RCL_OS_RCL_OS_POS           (0U)                                                /*!< LCSS_VCC_RCL_OS_RCL_OS bit offset */

/* LCSS_VCC_HFDLA_TH_HFDLA_TH */
#define LCSS_VCC_HFDLA_TH_HFDLA_TH_POS       (0U)                                                /*!< LCSS_VCC_HFDLA_TH_HFDLA_TH bit offset */

/* LCSS_VCC_LFDLA_TH_LFDLA_TH */
#define LCSS_VCC_LFDLA_TH_LFDLA_TH_POS       (0U)                                                /*!< LCSS_VCC_LFDLA_TH_LFDLA_TH bit offset */

/* LCSS_VCC_CLKSWH_CLK0_SWH1 */
#define LCSS_VCC_CLKSWH_CLK0_SWH1_POS        (0U)                                                /*!< LCSS_VCC_CLKSWH_CLK0_SWH1 bit offset */

/* LCSS_VCC_CLKSWH_CLK1_SWH1 */
#define LCSS_VCC_CLKSWH_CLK1_SWH1_POS        (1U)                                                /*!< LCSS_VCC_CLKSWH_CLK1_SWH1 bit offset */

/* LCSS_VCC_CLKSWH_CLK2_SWH1 */
#define LCSS_VCC_CLKSWH_CLK2_SWH1_POS        (2U)                                                /*!< LCSS_VCC_CLKSWH_CLK2_SWH1 bit offset */

/* LCSS_VCC_CLKSWH_CLK3_SWH1 */
#define LCSS_VCC_CLKSWH_CLK3_SWH1_POS        (3U)                                                /*!< LCSS_VCC_CLKSWH_CLK3_SWH1 bit offset */

/* LCSS_VCC_CLKSWH_CLK4_SWH1 */
#define LCSS_VCC_CLKSWH_CLK4_SWH1_POS        (4U)                                                /*!< LCSS_VCC_CLKSWH_CLK4_SWH1 bit offset */

/* LCSS_VCC_CLKSWH_CLK5_SWH1 */
#define LCSS_VCC_CLKSWH_CLK5_SWH1_POS        (5U)                                                /*!< LCSS_VCC_CLKSWH_CLK5_SWH1 bit offset */

/* LCSS_VCC_CLKSWH_CLK6_SWH1 */
#define LCSS_VCC_CLKSWH_CLK6_SWH1_POS        (6U)                                                /*!< LCSS_VCC_CLKSWH_CLK6_SWH1 bit offset */

/* LCSS_VCC_CLKSWH_CLK0_SWH0 */
#define LCSS_VCC_CLKSWH_CLK0_SWH0_POS        (8U)                                                /*!< LCSS_VCC_CLKSWH_CLK0_SWH0 bit offset */

/* LCSS_VCC_CLKSWH_CLK1_SWH0 */
#define LCSS_VCC_CLKSWH_CLK1_SWH0_POS        (9U)                                                /*!< LCSS_VCC_CLKSWH_CLK1_SWH0 bit offset */

/* LCSS_VCC_CLKSWH_CLK2_SWH0 */
#define LCSS_VCC_CLKSWH_CLK2_SWH0_POS        (10U)                                               /*!< LCSS_VCC_CLKSWH_CLK2_SWH0 bit offset */

/* LCSS_VCC_CLKSWH_CLK3_SWH0 */
#define LCSS_VCC_CLKSWH_CLK3_SWH0_POS        (11U)                                               /*!< LCSS_VCC_CLKSWH_CLK3_SWH0 bit offset */

/* LCSS_VCC_CLKSWH_CLK4_SWH0 */
#define LCSS_VCC_CLKSWH_CLK4_SWH0_POS        (12U)                                               /*!< LCSS_VCC_CLKSWH_CLK4_SWH0 bit offset */

/* LCSS_VCC_CLKSWH_CLK5_SWH0 */
#define LCSS_VCC_CLKSWH_CLK5_SWH0_POS        (13U)                                               /*!< LCSS_VCC_CLKSWH_CLK5_SWH0 bit offset */

/* LCSS_VCC_CLKSWH_CLK6_SWH0 */
#define LCSS_VCC_CLKSWH_CLK6_SWH0_POS        (14U)                                               /*!< LCSS_VCC_CLKSWH_CLK6_SWH0 bit offset */

/* LCSS_VCC_CLKSEL_CLKSEL0 */
#define LCSS_VCC_CLKSEL_CLKSEL0_POS          (0U)                                                /*!< LCSS_VCC_CLKSEL_CLKSEL0 bit offset */

/* LCSS_VCC_CLKSEL_CLKSEL1 */
#define LCSS_VCC_CLKSEL_CLKSEL1_POS          (1U)                                                /*!< LCSS_VCC_CLKSEL_CLKSEL1 bit offset */

/* LCSS_VCC_CLKSEL_CLKSEL2 */
#define LCSS_VCC_CLKSEL_CLKSEL2_POS          (2U)                                                /*!< LCSS_VCC_CLKSEL_CLKSEL2 bit offset */

/* LCSS_VCC_CLKSEL_CLKSEL3 */
#define LCSS_VCC_CLKSEL_CLKSEL3_POS          (3U)                                                /*!< LCSS_VCC_CLKSEL_CLKSEL3 bit offset */

/* LCSS_VCC_CLKSEL_CLKSEL4 */
#define LCSS_VCC_CLKSEL_CLKSEL4_POS          (4U)                                                /*!< LCSS_VCC_CLKSEL_CLKSEL4 bit offset */

/* LCSS_VCC_CLKSEL_CLKSEL5 */
#define LCSS_VCC_CLKSEL_CLKSEL5_POS          (5U)                                                /*!< LCSS_VCC_CLKSEL_CLKSEL5 bit offset */

/* LCSS_VCC_CLKSEL_CLKSEL6 */
#define LCSS_VCC_CLKSEL_CLKSEL6_POS          (6U)                                                /*!< LCSS_VCC_CLKSEL_CLKSEL6 bit offset */

/* LCSS_VCC_IE_HFREQ_IE */
#define LCSS_VCC_IE_HFREQ_IE_POS             (0U)                                                /*!< LCSS_VCC_IE_HFREQ_IE bit offset */

/* LCSS_VCC_IE_LFREQ_IE */
#define LCSS_VCC_IE_LFREQ_IE_POS             (1U)                                                /*!< LCSS_VCC_IE_LFREQ_IE bit offset */

/* LCSS_VCC_IE_LOSCOK_IE */
#define LCSS_VCC_IE_LOSCOK_IE_POS            (2U)                                                /*!< LCSS_VCC_IE_LOSCOK_IE bit offset */

/* LCSS_VCC_IF_HFREQ_IF */
#define LCSS_VCC_IF_HFREQ_IF_POS             (0U)                                                /*!< LCSS_VCC_IF_HFREQ_IF bit offset */

/* LCSS_VCC_IF_LFREQ_IF */
#define LCSS_VCC_IF_LFREQ_IF_POS             (1U)                                                /*!< LCSS_VCC_IF_LFREQ_IF bit offset */

/* LCSS_VCC_IF_FRCV_IF */
#define LCSS_VCC_IF_FRCV_IF_POS              (2U)                                                /*!< LCSS_VCC_IF_FRCV_IF bit offset */

/* LCSS_VCC_IF_LOSC_OK */
#define LCSS_VCC_IF_LOSC_OK_POS              (3U)                                                /*!< LCSS_VCC_IF_LOSC_OK bit offset */

/* LCSS_VCC_HFREQ_HFREQ */
#define LCSS_VCC_HFREQ_HFREQ_POS             (0U)                                                /*!< LCSS_VCC_HFREQ_HFREQ bit offset */

/* LCSS_VCC_LFREQ_LFREQ */
#define LCSS_VCC_LFREQ_LFREQ_POS             (0U)                                                /*!< LCSS_VCC_LFREQ_LFREQ bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_LCSS_VCC_H */
