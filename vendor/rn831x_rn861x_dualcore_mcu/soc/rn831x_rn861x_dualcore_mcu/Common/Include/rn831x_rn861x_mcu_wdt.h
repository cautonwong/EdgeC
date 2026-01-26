/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_wdt.h
 * @author  Renergy Software Team
 * @brief   WDT module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_WDT_H
#define _RN831x_RN861x_MCU_WDT_H

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

/* WDT */
typedef struct
{
    __IO uint32_t EN;                        /*!< offset: 0x000 info: Watchdog enable register */
    __IO uint32_t CTL;                       /*!< offset: 0x004 info: Watchdog control register */
    __IO uint32_t PASS;                      /*!< offset: 0x008 info: Password register */
    __IO uint32_t RESERVED0[2];              /*!< offset: 0x00C ~ 010 */
    __IO uint32_t HALT;                      /*!< offset: 0x014 info: Control whether to stop counting when the CPU is in halt state */
    __IO uint32_t STBY;                      /*!< offset: 0x018 info: Control whether to stop counting when the CPU is in sleep state */
    __IO uint32_t STOP;                      /*!< offset: 0x01C info: Control whether to stop counting when the CPU is in stop state */
    __IO uint32_t CNT;                       /*!< offset: 0x020 info: Counting of WDT */
} WDT_TypeDef;

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
#define WDT_CORE0_BASE  (0x20100000U)
#define WDT_CORE1_BASE  (0xA0100000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define WDT_CORE0  ((WDT_TypeDef *)      WDT_CORE0_BASE)
#define WDT_CORE1  ((WDT_TypeDef *)      WDT_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define WDT  WDT_CORE0
#endif
#if defined(CORE1)
#define WDT  WDT_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** WDT Instances *******************************/
#define IS_WDT_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == WDT_CORE0) || \
                                          ((INSTANCE) == WDT_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* WDT_EN */
#define WDT_EN_WDTE                          (BITS(0,7))                                         /*!< Reload bits of WDT */
#define WDT_EN_WR_BUSY                       (BIT(8))                                            /*!< Write busy state of WDT */

/* WDT_CTL */
#define WDT_CTL_WDCS                         (BITS(0,2))                                         /*!< Overflow time configuration */
#define WDT_CTL_WINDOW                       (BITS(5,6))                                         /*!< Valid reload window */
#define WDT_CTL_WDTINT                       (BIT(7))                                            /*!< Early wake interrup of WDT */
#define WDT_CTL_CLKDIV                       (BITS(8,10))                                        /*!< Clock division of WDT */
#define WDT_CTL_RAD_WAKE                     (BIT(11))                                           /*!< Enable bit of random wake */
#define WDT_CTL_WAKE_CNT                     (BITS(12,30))                                       /*!< Random wake counting bis */

/* WDT_PASS */
#define WDT_PASS_L                           (BITS(0,3))                                         /*!< Password low bits */
#define WDT_PASS_H                           (BITS(4,7))                                         /*!< Password high bits */

/* WDT_HALT */
#define WDT_HALT_HALT                        (BITS(0,31))                                        /*!< Control whether to stop counting when the CPU is in halt state */

/* WDT_STBY */
#define WDT_STBY_STBY                        (BITS(0,31))                                        /*!< Control whether to stop counting when the CPU is in sleep state */

/* WDT_STOP */
#define WDT_STOP_STOP                        (BITS(0,31))                                        /*!< Control whether to stop counting when the CPU is in stop state */

/* WDT_CNT */
#define WDT_CNT_CNT                          (BITS(0,18))                                        /*!< Counting bits of WDT */

/* constants definitions */
/* WDT_EN_WDTE */
#define WDT_EN_WDTE_POS                      (0U)                                                /*!< WDT_EN_WDTE bit offset */

/* WDT_EN_WR_BUSY */
#define WDT_EN_WR_BUSY_POS                   (8U)                                                /*!< WDT_EN_WR_BUSY bit offset */

/* WDT_CTL_WDCS */
#define WDT_CTL_WDCS_POS                     (0U)                                                /*!< WDT_CTL_WDCS bit offset */
#define WDT_CTL_WDCS_CFG(regval)  (WDT_CTL_WDCS & ((uint32_t)(regval) << WDT_CTL_WDCS_POS))
#define WDT_CTL_WDCS_0                       (WDT_CTL_WDCS_CFG(0))                               /*!< Overflow time 15.625ms */
#define WDT_CTL_WDCS_1                       (WDT_CTL_WDCS_CFG(1))                               /*!< Overflow time 31.25ms */
#define WDT_CTL_WDCS_2                       (WDT_CTL_WDCS_CFG(2))                               /*!< Overflow time 125ms */
#define WDT_CTL_WDCS_3                       (WDT_CTL_WDCS_CFG(3))                               /*!< Overflow time 500ms */
#define WDT_CTL_WDCS_4                       (WDT_CTL_WDCS_CFG(4))                               /*!< Overflow time 1s */
#define WDT_CTL_WDCS_5                       (WDT_CTL_WDCS_CFG(5))                               /*!< Overflow time 2s */
#define WDT_CTL_WDCS_6                       (WDT_CTL_WDCS_CFG(6))                               /*!< Overflow time 4s */
#define WDT_CTL_WDCS_7                       (WDT_CTL_WDCS_CFG(7))                               /*!< Overflow time 8s */

/* WDT_CTL_WINDOW */
#define WDT_CTL_WINDOW_POS                   (5U)                                                /*!< WDT_CTL_WINDOW bit offset */
#define WDT_CTL_WINDOW_CFG(regval)  (WDT_CTL_WINDOW & ((uint32_t)(regval) << WDT_CTL_WINDOW_POS))
#define WDT_CTL_WINDOW_ONEQUARTER            (WDT_CTL_WINDOW_CFG(0))                             /*!< One quarter of overflow time */
#define WDT_CTL_WINDOW_HALF                  (WDT_CTL_WINDOW_CFG(1))                             /*!< Half of overflow time */
#define WDT_CTL_WINDOW_THREEQUARTERS         (WDT_CTL_WINDOW_CFG(2))                             /*!< Three quarters of overflow time */
#define WDT_CTL_WINDOW_FULL                  (WDT_CTL_WINDOW_CFG(3))                             /*!< Full of overflow time */

/* WDT_CTL_WDTINT */
#define WDT_CTL_WDTINT_POS                   (7U)                                                /*!< WDT_CTL_WDTINT bit offset */

/* WDT_CTL_CLKDIV */
#define WDT_CTL_CLKDIV_POS                   (8U)                                                /*!< WDT_CTL_CLKDIV bit offset */
#define WDT_CTL_CLKDIV_CFG(regval)  (WDT_CTL_CLKDIV & ((uint32_t)(regval) << WDT_CTL_CLKDIV_POS))
#define WDT_CTL_CLKDIV_DIV0                  (WDT_CTL_CLKDIV_CFG(0))                             /*!< Clock no divider */
#define WDT_CTL_CLKDIV_DIV2                  (WDT_CTL_CLKDIV_CFG(1))                             /*!< Clock divider by 2 */
#define WDT_CTL_CLKDIV_DIV4                  (WDT_CTL_CLKDIV_CFG(2))                             /*!< Clock divider by 4 */
#define WDT_CTL_CLKDIV_DIV8                  (WDT_CTL_CLKDIV_CFG(3))                             /*!< Clock divider by 8 */
#define WDT_CTL_CLKDIV_DIV16                 (WDT_CTL_CLKDIV_CFG(4))                             /*!< Clock divider by 16 */
#define WDT_CTL_CLKDIV_DIV32                 (WDT_CTL_CLKDIV_CFG(5))                             /*!< Clock divider by 32 */
#define WDT_CTL_CLKDIV_DIV64                 (WDT_CTL_CLKDIV_CFG(6))                             /*!< Clock divider by 64 */
#define WDT_CTL_CLKDIV_DIV128                (WDT_CTL_CLKDIV_CFG(7))                             /*!< Clock divider by 128 */

/* WDT_CTL_RAD_WAKE */
#define WDT_CTL_RAD_WAKE_POS                 (11U)                                               /*!< WDT_CTL_RAD_WAKE bit offset */

/* WDT_CTL_WAKE_CNT */
#define WDT_CTL_WAKE_CNT_POS                 (12U)                                               /*!< WDT_CTL_WAKE_CNT bit offset */

/* WDT_PASS_L */
#define WDT_PASS_L_POS                       (0U)                                                /*!< WDT_PASS_L bit offset */

/* WDT_PASS_H */
#define WDT_PASS_H_POS                       (4U)                                                /*!< WDT_PASS_H bit offset */

/* WDT_HALT_HALT */
#define WDT_HALT_HALT_POS                    (0U)                                                /*!< WDT_HALT_HALT bit offset */

/* WDT_STBY_STBY */
#define WDT_STBY_STBY_POS                    (0U)                                                /*!< WDT_STBY_STBY bit offset */

/* WDT_STOP_STOP */
#define WDT_STOP_STOP_POS                    (0U)                                                /*!< WDT_STOP_STOP bit offset */

/* WDT_CNT_CNT */
#define WDT_CNT_CNT_POS                      (0U)                                                /*!< WDT_CNT_CNT bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_WDT_H */
