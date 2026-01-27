/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_lcss_vbat.h
 * @author  Renergy Software Team
 * @brief   LCSS_VBAT module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_LCSS_VBAT_H
#define _RN831x_RN861x_MCU_LCSS_VBAT_H

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

/* LCSS_VBAT */
typedef struct
{
    __IO uint32_t PSW0;                      /*!< offset: 0x000 info: Security key register 0 */
    __IO uint32_t PSW1;                      /*!< offset: 0x004 info: Security key register 1 */
    __IO uint32_t EN;                        /*!< offset: 0x008 info: Enable register */
    __IO uint32_t CTRL;                      /*!< offset: 0x00C info: Control register */
    __IO uint32_t CLKSWH;                    /*!< offset: 0x010 info: Clock configuration register for software switch */
    __IO uint32_t CLKSEL;                    /*!< offset: 0x014 info: Clock selected register, indicate the selected clock */
    __IO uint32_t IE;                        /*!< offset: 0x018 info: Interrupt enable register */
    __IO uint32_t IF;                        /*!< offset: 0x01C info: Status register */
} LCSS_VBAT_TypeDef;

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
#define LCSS_VBAT_CORE0_BASE (0x200B8200U)
#define LCSS_VBAT_CORE1_BASE (0xB00B8200U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define LCSS_VBAT_CORE0 ((LCSS_VBAT_TypeDef *)LCSS_VBAT_CORE0_BASE)
#define LCSS_VBAT_CORE1 ((LCSS_VBAT_TypeDef *)LCSS_VBAT_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define LCSS_VBAT  LCSS_VBAT_CORE0
#endif
#if defined(CORE1)
#define LCSS_VBAT  LCSS_VBAT_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** LCSS_VBAT Instances *******************************/
#define IS_LCSS_VBAT_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == LCSS_VBAT_CORE0) || \
                                                ((INSTANCE) == LCSS_VBAT_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* LCSS_VBAT_PSW0 */
#define LCSS_VBAT_PSW0_PSW0                  (BITS(0,15))                                        /*!< Managed registers include LCSS_VBAT_EN/LCSS_CTRL/LCSS_IE */

/* LCSS_VBAT_PSW1 */
#define LCSS_VBAT_PSW1_PSW1                  (BITS(0,15))                                        /*!< Managed registers include LCSS_CLKSWH */

/* LCSS_VBAT_EN */
#define LCSS_VBAT_EN_EN                      (BIT(0))                                            /*!< LCSS VBAT enable */

/* LCSS_VBAT_CTRL */
#define LCSS_VBAT_CTRL_STOP_TH               (BITS(0,4))                                         /*!< Clock stop threshold configuration */
#define LCSS_VBAT_CTRL_RCV_CYC               (BITS(5,11))                                        /*!< LOSC recovery cycles configuration */
#define LCSS_VBAT_CTRL_STOP_ASWH             (BIT(12))                                           /*!< Auto switch when clock stopped */
#define LCSS_VBAT_CTRL_RCV_ASWH              (BIT(13))                                           /*!< Auto switch when clock recovery */
#define LCSS_VBAT_CTRL_TC_CFG                (BITS(14,15))                                       /*!< Timing detection function configuration */

/* LCSS_VBAT_CLKSWH */
#define LCSS_VBAT_CLKSWH_RTC_CLK_SWH         (BITS(0,2))                                         /*!< Clock source selection */

/* LCSS_VBAT_CLKSEL */
#define LCSS_VBAT_CLKSEL_RTC_CLK_SEL         (BITS(0,2))                                         /*!< Clock source selection, this register must be written to 0 before reading to read the latest value. */

/* LCSS_VBAT_IE */
#define LCSS_VBAT_IE_LOSC_STOP_IE            (BIT(0))                                            /*!< LOSC stopped interrupt enable */
#define LCSS_VBAT_IE_LOSC_OK_IE              (BIT(1))                                            /*!< LOSC is ok interrupt enable */
#define LCSS_VBAT_IE_RCL_STOP_IE             (BIT(2))                                            /*!< RCL stopped interrupt enable */
#define LCSS_VBAT_IE_RCL_OK_IE               (BIT(3))                                            /*!< RCL is ok interrupt enable */

/* LCSS_VBAT_IF */
#define LCSS_VBAT_IF_LOSC_STOP_IF            (BIT(0))                                            /*!< LOSC stopped status, when LCSS_VBAT_EN=0, the state will not be automatically updated (this bit must be written 1 twice to clear) */
#define LCSS_VBAT_IF_LOSC_OK                 (BIT(1))                                            /*!< LOSC is ok status, when LCSS_VBAT_EN=0, the state will not be automatically updated (this bit must be written 1 twice to clear) */
#define LCSS_VBAT_IF_RCL_STOP_IF             (BIT(2))                                            /*!< RCL stopped status, when LCSS_VBAT_EN=0, the state will not be automatically updated (this bit must be written 1 twice to clear) */
#define LCSS_VBAT_IF_RCL_OK                  (BIT(3))                                            /*!< RCL is ok status, when LCSS_VBAT_EN=0, the state will not be automatically updated (this bit must be written 1 twice to clear) */

/* constants definitions */
/* LCSS_VBAT_PSW0_PSW0 */
#define LCSS_VBAT_PSW0_PSW0_POS              (0U)                                                /*!< LCSS_VBAT_PSW0_PSW0 bit offset */

/* LCSS_VBAT_PSW1_PSW1 */
#define LCSS_VBAT_PSW1_PSW1_POS              (0U)                                                /*!< LCSS_VBAT_PSW1_PSW1 bit offset */

/* LCSS_VBAT_EN_EN */
#define LCSS_VBAT_EN_EN_POS                  (0U)                                                /*!< LCSS_VBAT_EN_EN bit offset */

/* LCSS_VBAT_CTRL_STOP_TH */
#define LCSS_VBAT_CTRL_STOP_TH_POS           (0U)                                                /*!< LCSS_VBAT_CTRL_STOP_TH bit offset */

/* LCSS_VBAT_CTRL_RCV_CYC */
#define LCSS_VBAT_CTRL_RCV_CYC_POS           (5U)                                                /*!< LCSS_VBAT_CTRL_RCV_CYC bit offset */

/* LCSS_VBAT_CTRL_STOP_ASWH */
#define LCSS_VBAT_CTRL_STOP_ASWH_POS         (12U)                                               /*!< LCSS_VBAT_CTRL_STOP_ASWH bit offset */

/* LCSS_VBAT_CTRL_RCV_ASWH */
#define LCSS_VBAT_CTRL_RCV_ASWH_POS          (13U)                                               /*!< LCSS_VBAT_CTRL_RCV_ASWH bit offset */

/* LCSS_VBAT_CTRL_TC_CFG */
#define LCSS_VBAT_CTRL_TC_CFG_POS            (14U)                                               /*!< LCSS_VBAT_CTRL_TC_CFG bit offset */
#define LCSS_VBAT_CTRL_TC_CFG_CFG(regval)  (LCSS_VBAT_CTRL_TC_CFG & ((uint32_t)(regval) << LCSS_VBAT_CTRL_TC_CFG_POS))
#define LCSS_VBAT_CTRL_TC_CFG_TIMING_NONE    (LCSS_VBAT_CTRL_TC_CFG_CFG(0x0))                    /*!< No timing detection */
#define LCSS_VBAT_CTRL_TC_CFG_TIMING_10MS    (LCSS_VBAT_CTRL_TC_CFG_CFG(0x1))                    /*!< 10ms timing detection, start detection every 10ms */
#define LCSS_VBAT_CTRL_TC_CFG_TIMING_20MS    (LCSS_VBAT_CTRL_TC_CFG_CFG(0x2))                    /*!< 20ms timing detection, start detection every 20ms */
#define LCSS_VBAT_CTRL_TC_CFG_TIMING_40MS    (LCSS_VBAT_CTRL_TC_CFG_CFG(0x3))                    /*!< 40ms timing detection, start detection every 40ms */

/* LCSS_VBAT_CLKSWH_RTC_CLK_SWH */
#define LCSS_VBAT_CLKSWH_RTC_CLK_SWH_POS     (0U)                                                /*!< LCSS_VBAT_CLKSWH_RTC_CLK_SWH bit offset */
#define LCSS_VBAT_CLKSWH_RTC_CLK_SWH_CFG(regval)  (LCSS_VBAT_CLKSWH_RTC_CLK_SWH & ((uint32_t)(regval) << LCSS_VBAT_CLKSWH_RTC_CLK_SWH_POS))
#define LCSS_VBAT_CLKSWH_RTC_CLK_SWH_LOSC    (LCSS_VBAT_CLKSWH_RTC_CLK_SWH_CFG(0x1))             /*!< Selected LOSC clock */
#define LCSS_VBAT_CLKSWH_RTC_CLK_SWH_RCL     (LCSS_VBAT_CLKSWH_RTC_CLK_SWH_CFG(0x2))             /*!< Selected RCL32k clock */
#define LCSS_VBAT_CLKSWH_RTC_CLK_SWH_RCO     (LCSS_VBAT_CLKSWH_RTC_CLK_SWH_CFG(0x4))             /*!< Selected RCO clock */

/* LCSS_VBAT_CLKSEL_RTC_CLK_SEL */
#define LCSS_VBAT_CLKSEL_RTC_CLK_SEL_POS     (0U)                                                /*!< LCSS_VBAT_CLKSEL_RTC_CLK_SEL bit offset */
#define LCSS_VBAT_CLKSEL_RTC_CLK_SEL_CFG(regval)  (LCSS_VBAT_CLKSEL_RTC_CLK_SEL & ((uint32_t)(regval) << LCSS_VBAT_CLKSEL_RTC_CLK_SEL_POS))
#define LCSS_VBAT_CLKSEL_RTC_CLK_SEL_LOSC    (LCSS_VBAT_CLKSEL_RTC_CLK_SEL_CFG(0x1))             /*!< Selected LOSC clock */
#define LCSS_VBAT_CLKSEL_RTC_CLK_SEL_RCL     (LCSS_VBAT_CLKSEL_RTC_CLK_SEL_CFG(0x2))             /*!< Selected RCL32k clock */
#define LCSS_VBAT_CLKSEL_RTC_CLK_SEL_RCO     (LCSS_VBAT_CLKSEL_RTC_CLK_SEL_CFG(0x4))             /*!< Selected RCO clock */

/* LCSS_VBAT_IE_LOSC_STOP_IE */
#define LCSS_VBAT_IE_LOSC_STOP_IE_POS        (0U)                                                /*!< LCSS_VBAT_IE_LOSC_STOP_IE bit offset */

/* LCSS_VBAT_IE_LOSC_OK_IE */
#define LCSS_VBAT_IE_LOSC_OK_IE_POS          (1U)                                                /*!< LCSS_VBAT_IE_LOSC_OK_IE bit offset */

/* LCSS_VBAT_IE_RCL_STOP_IE */
#define LCSS_VBAT_IE_RCL_STOP_IE_POS         (2U)                                                /*!< LCSS_VBAT_IE_RCL_STOP_IE bit offset */

/* LCSS_VBAT_IE_RCL_OK_IE */
#define LCSS_VBAT_IE_RCL_OK_IE_POS           (3U)                                                /*!< LCSS_VBAT_IE_RCL_OK_IE bit offset */

/* LCSS_VBAT_IF_LOSC_STOP_IF */
#define LCSS_VBAT_IF_LOSC_STOP_IF_POS        (0U)                                                /*!< LCSS_VBAT_IF_LOSC_STOP_IF bit offset */

/* LCSS_VBAT_IF_LOSC_OK */
#define LCSS_VBAT_IF_LOSC_OK_POS             (1U)                                                /*!< LCSS_VBAT_IF_LOSC_OK bit offset */

/* LCSS_VBAT_IF_RCL_STOP_IF */
#define LCSS_VBAT_IF_RCL_STOP_IF_POS         (2U)                                                /*!< LCSS_VBAT_IF_RCL_STOP_IF bit offset */

/* LCSS_VBAT_IF_RCL_OK */
#define LCSS_VBAT_IF_RCL_OK_POS              (3U)                                                /*!< LCSS_VBAT_IF_RCL_OK bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_LCSS_VBAT_H */
