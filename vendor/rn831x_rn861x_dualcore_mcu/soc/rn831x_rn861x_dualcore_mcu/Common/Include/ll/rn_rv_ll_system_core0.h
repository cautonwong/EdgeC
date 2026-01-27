/**
 *****************************************************************************
 * @file   rn_rv_ll_system_core0.h
 * @author Renergy Software Team
 * @brief  Header file of SYSTEM_CORE0 LL module.
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
#ifndef __RN_LL_SYSTEM_CORE0_H
#define __RN_LL_SYSTEM_CORE0_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_system_core0.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

/** @defgroup SYSTEM_CORE0_LL SYSTEM_CORE0
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup SYSTEM_CORE0_LL_Private_Constants SYSTEM_CORE0 Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup SYSTEM_CORE0_LL_Private_Macros SYSTEM_CORE0 Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup SYSTEM_CORE0_LL_ES_INIT SYSTEM_CORE0 Exported Init structure
 * @{
 */

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup SYSTEM_CORE0_LL_Exported_Constants SYSTEM_CORE0 Exported Constants
 * @{
 */

/** @defgroup SYSTEM_CORE0_LL_EC_RCHCLOCKDIV RCH Clcok Division
 * @{
 */
#define LL_SYSTEM_CORE0_RCHCLOCKDIV_1               (SYSTEM_CORE0_RCH_CTRL_DIV_1)                 /*!< RCH clock divided by 1  */
#define LL_SYSTEM_CORE0_RCHCLOCKDIV_2               (SYSTEM_CORE0_RCH_CTRL_DIV_2)                 /*!< RCH clock divided by 2  */
#define LL_SYSTEM_CORE0_RCHCLOCKDIV_4               (SYSTEM_CORE0_RCH_CTRL_DIV_4)                 /*!< RCH clock divided by 3  */
#define LL_SYSTEM_CORE0_RCHCLOCKDIV_8               (SYSTEM_CORE0_RCH_CTRL_DIV_8)                 /*!< RCH clock divided by 8  */
#define LL_SYSTEM_CORE0_RCHCLOCKDIV_16              (SYSTEM_CORE0_RCH_CTRL_DIV_16)                /*!< RCH clock divided by 16 */
#define LL_SYSTEM_CORE0_RCHCLOCKDIV_32              (SYSTEM_CORE0_RCH_CTRL_DIV_32)                /*!< RCH clock divided by 32 */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_PLLOUTPUTFREQ PLL Output Frequency
 * @{
 */
#define LL_SYSTEM_CORE0_PLLOUTPUTFREQ_7M3728                (SYSTEM_CORE0_PLL_CTRL_FREQ_7M3728)                 /*!< PLL output frequency selection is 7.3728MHz  */
#define LL_SYSTEM_CORE0_PLLOUTPUTFREQ_14M7456               (SYSTEM_CORE0_PLL_CTRL_FREQ_14M7456)                /*!< PLL output frequency selection is 14.7456MHz */
#define LL_SYSTEM_CORE0_PLLOUTPUTFREQ_29M4912               (SYSTEM_CORE0_PLL_CTRL_FREQ_29M4912)                /*!< PLL output frequency selection is 29.4912MHz */
#define LL_SYSTEM_CORE0_PLLOUTPUTFREQ_58M9824               (SYSTEM_CORE0_PLL_CTRL_FREQ_58M9824)                /*!< PLL output frequency selection is 58.9824MHz */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_PLLCLOCKDIV PLL Clcok Division
 * @{
 */
#define LL_SYSTEM_CORE0_PLLCLOCKDIV_1               (SYSTEM_CORE0_PLL_CTRL_DIV_1)                 /*!< PLL clock divided by 1  */
#define LL_SYSTEM_CORE0_PLLCLOCKDIV_2               (SYSTEM_CORE0_PLL_CTRL_DIV_2)                 /*!< PLL clock divided by 2  */
#define LL_SYSTEM_CORE0_PLLCLOCKDIV_4               (SYSTEM_CORE0_PLL_CTRL_DIV_4)                 /*!< PLL clock divided by 3  */
#define LL_SYSTEM_CORE0_PLLCLOCKDIV_8               (SYSTEM_CORE0_PLL_CTRL_DIV_8)                 /*!< PLL clock divided by 8  */
#define LL_SYSTEM_CORE0_PLLCLOCKDIV_16              (SYSTEM_CORE0_PLL_CTRL_DIV_16)                /*!< PLL clock divided by 16 */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_HOSCCRYSTALFREQ HOSC Crystal Frequency
 * @{
 */
#define LL_SYSTEM_CORE0_HOSCCRYSTALFREQ_7M3728                (SYSTEM_CORE0_HOSC_CTRL_FREQ_7M3728)                 /*!< HOSC Crystal frequency selection is 7.3728MHz  */
#define LL_SYSTEM_CORE0_HOSCCRYSTALFREQ_14M7456               (SYSTEM_CORE0_HOSC_CTRL_FREQ_14M7456)                /*!< HOSC Crystal frequency selection is 14.7456MHz */
#define LL_SYSTEM_CORE0_HOSCCRYSTALFREQ_29M4912               (SYSTEM_CORE0_HOSC_CTRL_FREQ_29M4912)                /*!< HOSC Crystal frequency selection is 29.4912MHz */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_HOSCCLOCKDIV HOSC Clcok Division
 * @{
 */
#define LL_SYSTEM_CORE0_HOSCCLOCKDIV_1               (SYSTEM_CORE0_HOSC_CTRL_DIV_1)                 /*!< HOSC clock divided by 1  */
#define LL_SYSTEM_CORE0_HOSCCLOCKDIV_2               (SYSTEM_CORE0_HOSC_CTRL_DIV_2)                 /*!< HOSC clock divided by 2  */
#define LL_SYSTEM_CORE0_HOSCCLOCKDIV_4               (SYSTEM_CORE0_HOSC_CTRL_DIV_4)                 /*!< HOSC clock divided by 3  */
#define LL_SYSTEM_CORE0_HOSCCLOCKDIV_8               (SYSTEM_CORE0_HOSC_CTRL_DIV_8)                 /*!< HOSC clock divided by 8  */
#define LL_SYSTEM_CORE0_HOSCCLOCKDIV_16              (SYSTEM_CORE0_HOSC_CTRL_DIV_16)                /*!< HOSC clock divided by 16 */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_HOSCGAIN HOSC Clcok Gain
 * @{
 */
#define LL_SYSTEM_CORE0_HOSCGAIN_1X_0_8MS              (SYSTEM_CORE0_HOSC_CTRL_PGA_1X_0_8MS)                /*!< 1x transconductance gain, 928uA current, 0.8mS  */
#define LL_SYSTEM_CORE0_HOSCGAIN_2X_2_4MS              (SYSTEM_CORE0_HOSC_CTRL_PGA_2X_2_4MS)                /*!< 2x transconductance gain, 1856uA current, 2.4mS */
#define LL_SYSTEM_CORE0_HOSCGAIN_4X_3_2MS              (SYSTEM_CORE0_HOSC_CTRL_PGA_4X_3_2MS)                /*!< 4x transconductance gain, 3712uA current, 3.2mS */
#define LL_SYSTEM_CORE0_HOSCGAIN_8X_5_6MS              (SYSTEM_CORE0_HOSC_CTRL_PGA_8X_5_6MS)                /*!< 8x transconductance gain, 7424uA current, 5.6mS */
#define LL_SYSTEM_CORE0_HOSCGAIN_1X_1_2MS              (SYSTEM_CORE0_HOSC_CTRL_PGA_1X_1_2MS)                /*!< 1x transconductance gain, enhanced gain, 1.2mS  */
#define LL_SYSTEM_CORE0_HOSCGAIN_2X_1_6MS              (SYSTEM_CORE0_HOSC_CTRL_PGA_2X_1_6MS)                /*!< 2x transconductance gain, enhanced gain, 1.6mS  */
#define LL_SYSTEM_CORE0_HOSCGAIN_4X_4_8MS              (SYSTEM_CORE0_HOSC_CTRL_PGA_4X_4_8MS)                /*!< 4x transconductance gain, enhanced gain, 4.8mS  */
#define LL_SYSTEM_CORE0_HOSCGAIN_8X_6_4MS              (SYSTEM_CORE0_HOSC_CTRL_PGA_8X_6_4MS)                /*!< 8x transconductance gain, enhanced gain, 6.4mS  */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_HOSCTIME HOSC Analog Filter Time
 * @{
 */
#define LL_SYSTEM_CORE0_HOSCTIME_25_5NS              (SYSTEM_CORE0_HOSC_CTRL_DSEL_25_5NS)                /*!< External high-frequency crystal is 7.3728MHz  */
#define LL_SYSTEM_CORE0_HOSCTIME_10_5NS              (SYSTEM_CORE0_HOSC_CTRL_DSEL_10_5NS)                /*!< External high-frequency crystal is 14.7456MHz */
#define LL_SYSTEM_CORE0_HOSCTIME_5NS                 (SYSTEM_CORE0_HOSC_CTRL_DSEL_5NS)                   /*!< External high-frequency crystal is 29.4912MHz */
#define LL_SYSTEM_CORE0_HOSCTIME_NONE                (SYSTEM_CORE0_HOSC_CTRL_DSEL_NONE)                  /*!< HOSC clock does not use analog filtering      */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_SYSTEMLOWFREQSOURCE System Low Frequence Source
 * @{
 */
#define LL_SYSTEM_CORE0_SYSTEMLOWFREQSOURCE_LOSC                   0x00000000U                                       /*!< System Low Frequence selects LOSC   */
#define LL_SYSTEM_CORE0_SYSTEMLOWFREQSOURCE_RCL32K                 (SYSTEM_CORE0_CLK_SEL_SYS_RCL_LOSC)               /*!< System Low Frequence selects RCL32K */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_SYSTEMHIGHFREQSOURCE System High Frequence Source
 * @{
 */
#define LL_SYSTEM_CORE0_SYSTEMHIGHFREQSOURCE_PLL                   0x00000000U                                       /*!< System High Frequence selects PLL   */
#define LL_SYSTEM_CORE0_SYSTEMHIGHFREQSOURCE_HOSC                  (SYSTEM_CORE0_CLK_SEL_SYS_PLL_HOSC)               /*!< System High Frequence selects HOSC */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_CORE1LOWFREQSOURCE Core1 Low Frequence Source
 * @{
 */
#define LL_SYSTEM_CORE0_CORE1LOWFREQSOURCE_LOSC                   0x00000000U                                       /*!< Filtered clock selects LOSC   */
#define LL_SYSTEM_CORE0_CORE1LOWFREQSOURCE_RCL32K                 (SYSTEM_CORE0_CLK_SEL_CORE1_RCL_LOSC)             /*!< Filtered clock selects RCL32K */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_CORE0LOWFREQSOURCE Core0 Low Frequence Source
 * @{
 */
#define LL_SYSTEM_CORE0_CORE0LOWFREQSOURCE_LOSC                   0x00000000U                                       /*!< Filtered clock selects LOSC   */
#define LL_SYSTEM_CORE0_CORE0LOWFREQSOURCE_RCL32K                 (SYSTEM_CORE0_CLK_SEL_CORE0_RCL_LOSC)             /*!< Filtered clock selects RCL32K */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_D2FLOWFREQSOURCE D2F Low Frequence Source
 * @{
 */
#define LL_SYSTEM_CORE0_D2FLOWFREQSOURCE_LOSC                     0x00000000U                                     /*!< Filtered clock selects LOSC   */
#define LL_SYSTEM_CORE0_D2FLOWFREQSOURCE_RCL32K                   (SYSTEM_CORE0_CLK_SEL_D2F_RCL_LOSC)             /*!< Filtered clock selects RCL32K */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_LPUARTLOWFREQSOURCE LPUART Low Frequence Source
 * @{
 */
#define LL_SYSTEM_CORE0_LPUARTLOWFREQSOURCE_LOSC                     0x00000000U                                     /*!< Filtered clock selects LOSC   */
#define LL_SYSTEM_CORE0_LPUARTLOWFREQSOURCE_RCL32K                   (SYSTEM_CORE0_CLK_SEL_LPUART_RCL_LOSC)             /*!< Filtered clock selects RCL32K */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_KBISOURCE KBI Source
 * @{
 */
#define LL_SYSTEM_CORE0_KBISOURCE_LOSC                     0x00000000U                                     /*!< Filtered clock selects LOSC   */
#define LL_SYSTEM_CORE0_KBISOURCE_RCL32K                   (SYSTEM_CORE0_CLK_SEL_KBI_RCL_LOSC)             /*!< Filtered clock selects RCL32K */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_IRFRSOURCE IRFR Source
 * @{
 */
#define LL_SYSTEM_CORE0_IRFRSOURCE_LOSC                     (SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC_LOSC)               /*!< IRFR selects LOSC external low-frequency crystal as clock source    */
#define LL_SYSTEM_CORE0_IRFRSOURCE_RCL32K                   (SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC_RCL32K)             /*!< IRFR selects RCL32k internal low-frequency crystal as clock source  */
#define LL_SYSTEM_CORE0_IRFRSOURCE_RCL307K                  (SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC_RCL307K)            /*!< IRFR selects RCL307k internal low-frequency crystal as clock source */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_LCDSOURCE LCD Source
 * @{
 */
#define LL_SYSTEM_CORE0_LCDSOURCE_LOSC                     (SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC_LOSC)               /*!< LCD selects LOSC external low-frequency crystal as clock source    */
#define LL_SYSTEM_CORE0_LCDSOURCE_RCL32K                   (SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC_RCL32K)             /*!< LCD selects RCL32k internal low-frequency crystal as clock source  */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_SYSCLOCKMODE System Clock Mode
 * @{
 */
#define LL_SYSTEM_CORE0_SYSCLOCKMODE_HCM                   (SYSTEM_CORE0_SYS_MODE_SYS_MODE_HCM)              /*!< Write D, set to enter high-frequency mode HCM, read value 4'b0100 */
#define LL_SYSTEM_CORE0_SYSCLOCKMODE_RCM                   (SYSTEM_CORE0_SYS_MODE_SYS_MODE_RCM)              /*!< Write E, set to enter RC mode RCM, read value 4'b0010 */
#define LL_SYSTEM_CORE0_SYSCLOCKMODE_LCM                   (SYSTEM_CORE0_SYS_MODE_SYS_MODE_LCM)              /*!< Write F, set to enter 32.768KHz mode LCM, read value 4'b0001 */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_COREDIV CORE1_CORE0_DIV
 * @{
 */
#define LL_SYSTEM_CORE0_COREDIV_CORE1_1_CORE0_1             (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_NONE_CORE0_NONE)        /*!< CORE1 no division, CORE0 no division */
#define LL_SYSTEM_CORE0_COREDIV_CORE1_1_CORE0_2             (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_NONE_CORE0_2)           /*!< CORE1 no division, CORE0 divide by 2 */
#define LL_SYSTEM_CORE0_COREDIV_CORE1_1_CORE0_4             (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_NONE_CORE0_4)           /*!< CORE1 no division, CORE0 divide by 4 */
#define LL_SYSTEM_CORE0_COREDIV_CORE1_1_CORE0_8             (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_NONE_CORE0_8)           /*!< CORE1 no division, CORE0 divide by 8 */
#define LL_SYSTEM_CORE0_COREDIV_CORE1_2_CORE0_2             (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_2_CORE0_2)              /*!< CORE1 divide by 2, CORE0 divide by 2 */
#define LL_SYSTEM_CORE0_COREDIV_CORE1_2_CORE0_4             (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_2_CORE0_4)              /*!< CORE1 divide by 2, CORE0 divide by 4 */
#define LL_SYSTEM_CORE0_COREDIV_CORE1_2_CORE0_8             (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_2_CORE0_8)              /*!< CORE1 divide by 2, CORE0 divide by 8 */
#define LL_SYSTEM_CORE0_COREDIV_CORE1_4_CORE0_4             (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_4_CORE0_4)              /*!< CORE1 divide by 4, CORE0 divide by 4 */
#define LL_SYSTEM_CORE0_COREDIV_CORE1_4_CORE0_8             (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_4_CORE0_8)              /*!< CORE1 divide by 4, CORE0 divide by 8 */
#define LL_SYSTEM_CORE0_COREDIV_CORE1_8_CORE0_8             (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_8_CORE0_8)              /*!< CORE1 divide by 8, CORE0 divide by 8 */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_PMOD0_CKE_PERIPH  PMOD0_CKE PERIPH
 * @{
 */
#define LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_ALL            (uint32_t)0x00007F1FU
#define LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_WDT            SYSTEM_CORE0_PMOD0_CKE_WDT
#define LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_MAILBOX        SYSTEM_CORE0_PMOD0_CKE_MAILBOX
#define LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_AOS            SYSTEM_CORE0_PMOD0_CKE_AOS
#define LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_SCU            SYSTEM_CORE0_PMOD0_CKE_SCU
#define LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GIC            SYSTEM_CORE0_PMOD0_CKE_GIC
#define LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GDMA00         SYSTEM_CORE0_PMOD0_CKE_GDMA00
#define LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GDMA01         SYSTEM_CORE0_PMOD0_CKE_GDMA01
#define LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_UART38K        SYSTEM_CORE0_PMOD0_CKE_UART38K
#define LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GPIO           SYSTEM_CORE0_PMOD0_CKE_GPIO
#define LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_LCSS           SYSTEM_CORE0_PMOD0_CKE_LCSS
#define LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_CRC1           SYSTEM_CORE0_PMOD0_CKE_CRC1
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_PMOD0_RST_PERIPH  PMOD0_RST PERIPH
 * @{
 */
#define LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_ALL            (uint32_t)0x0000007FU
#define LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_MAILBOX        SYSTEM_CORE0_PMOD0_RST_MAILBOX
#define LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_AOS            SYSTEM_CORE0_PMOD0_RST_AOS
#define LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GDMA00         SYSTEM_CORE0_PMOD0_RST_GDMA00
#define LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GDMA01         SYSTEM_CORE0_PMOD0_RST_GDMA01
#define LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GPIO           SYSTEM_CORE0_PMOD0_RST_GPIO
#define LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_CRC1           SYSTEM_CORE0_PMOD0_RST_CRC1
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_KBI0_CHANNEL  KBI0 CHANNEL
 * @{
 */
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_ALL          (uint32_t)0x0000FFFFU
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_0            SYSTEM_CORE0_KBI0_CKE_KBI00
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_1            SYSTEM_CORE0_KBI0_CKE_KBI01
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_2            SYSTEM_CORE0_KBI0_CKE_KBI02
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_3            SYSTEM_CORE0_KBI0_CKE_KBI03
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_4            SYSTEM_CORE0_KBI0_CKE_KBI04
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_5            SYSTEM_CORE0_KBI0_CKE_KBI05
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_6            SYSTEM_CORE0_KBI0_CKE_KBI06
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_7            SYSTEM_CORE0_KBI0_CKE_KBI07
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_8            SYSTEM_CORE0_KBI0_CKE_KBI08
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_9            SYSTEM_CORE0_KBI0_CKE_KBI09
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_10           SYSTEM_CORE0_KBI0_CKE_KBI010
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_11           SYSTEM_CORE0_KBI0_CKE_KBI011
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_12           SYSTEM_CORE0_KBI0_CKE_KBI012
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_13           SYSTEM_CORE0_KBI0_CKE_KBI013
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_14           SYSTEM_CORE0_KBI0_CKE_KBI014
#define LL_SYSTEM_CORE0_KBI0_CHANNEL_15           SYSTEM_CORE0_KBI0_CKE_KBI015
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_KBI1_CHANNEL  KBI1 CHANNEL
 * @{
 */
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_ALL          (uint32_t)0x0000FFFFU
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_0            SYSTEM_CORE0_KBI1_CKE_KBI10
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_1            SYSTEM_CORE0_KBI1_CKE_KBI11
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_2            SYSTEM_CORE0_KBI1_CKE_KBI12
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_3            SYSTEM_CORE0_KBI1_CKE_KBI13
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_4            SYSTEM_CORE0_KBI1_CKE_KBI14
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_5            SYSTEM_CORE0_KBI1_CKE_KBI15
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_6            SYSTEM_CORE0_KBI1_CKE_KBI16
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_7            SYSTEM_CORE0_KBI1_CKE_KBI17
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_8            SYSTEM_CORE0_KBI1_CKE_KBI18
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_9            SYSTEM_CORE0_KBI1_CKE_KBI19
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_10           SYSTEM_CORE0_KBI1_CKE_KBI110
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_11           SYSTEM_CORE0_KBI1_CKE_KBI111
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_12           SYSTEM_CORE0_KBI1_CKE_KBI112
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_13           SYSTEM_CORE0_KBI1_CKE_KBI113
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_14           SYSTEM_CORE0_KBI1_CKE_KBI114
#define LL_SYSTEM_CORE0_KBI1_CHANNEL_15           SYSTEM_CORE0_KBI1_CKE_KBI115
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_MOD0_PERIPH  MOD0 PERIPH
 * @{
 */
#define LL_SYSTEM_CORE0_MOD0_PERIPH_ALL            (uint32_t)0x3F7FFFFFU
#define LL_SYSTEM_CORE0_MOD0_PERIPH_UART0          SYSTEM_CORE0_MOD0_CKE_UART0
#define LL_SYSTEM_CORE0_MOD0_PERIPH_UART1          SYSTEM_CORE0_MOD0_CKE_UART1
#define LL_SYSTEM_CORE0_MOD0_PERIPH_UART2          SYSTEM_CORE0_MOD0_CKE_UART2
#define LL_SYSTEM_CORE0_MOD0_PERIPH_UART3          SYSTEM_CORE0_MOD0_CKE_UART3
#define LL_SYSTEM_CORE0_MOD0_PERIPH_UART4          SYSTEM_CORE0_MOD0_CKE_UART4
#define LL_SYSTEM_CORE0_MOD0_PERIPH_UART5          SYSTEM_CORE0_MOD0_CKE_UART5
#define LL_SYSTEM_CORE0_MOD0_PERIPH_UART6          SYSTEM_CORE0_MOD0_CKE_UART6
#define LL_SYSTEM_CORE0_MOD0_PERIPH_UART7          SYSTEM_CORE0_MOD0_CKE_UART7
#define LL_SYSTEM_CORE0_MOD0_PERIPH_LPUART         SYSTEM_CORE0_MOD0_CKE_LPUART
#define LL_SYSTEM_CORE0_MOD0_PERIPH_USART0         SYSTEM_CORE0_MOD0_CKE_USART0
#define LL_SYSTEM_CORE0_MOD0_PERIPH_USART1         SYSTEM_CORE0_MOD0_CKE_USART1
#define LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78160       SYSTEM_CORE0_MOD0_CKE_ISO78160
#define LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78161       SYSTEM_CORE0_MOD0_CKE_ISO78161
#define LL_SYSTEM_CORE0_MOD0_PERIPH_SPI0           SYSTEM_CORE0_MOD0_CKE_SPI0
#define LL_SYSTEM_CORE0_MOD0_PERIPH_SPI1           SYSTEM_CORE0_MOD0_CKE_SPI1
#define LL_SYSTEM_CORE0_MOD0_PERIPH_SPI2           SYSTEM_CORE0_MOD0_CKE_SPI2
#define LL_SYSTEM_CORE0_MOD0_PERIPH_SPI3           SYSTEM_CORE0_MOD0_CKE_SPI3
#define LL_SYSTEM_CORE0_MOD0_PERIPH_I2C0           SYSTEM_CORE0_MOD0_CKE_I2C0
#define LL_SYSTEM_CORE0_MOD0_PERIPH_I2C1           SYSTEM_CORE0_MOD0_CKE_I2C1
#define LL_SYSTEM_CORE0_MOD0_PERIPH_CAN            SYSTEM_CORE0_MOD0_CKE_CAN
#define LL_SYSTEM_CORE0_MOD0_PERIPH_IOCNT          SYSTEM_CORE0_MOD0_CKE_IOCNT
#define LL_SYSTEM_CORE0_MOD0_PERIPH_LCD            SYSTEM_CORE0_MOD0_CKE_LCD
#define LL_SYSTEM_CORE0_MOD0_PERIPH_DSP            SYSTEM_CORE0_MOD0_CKE_DSP
#define LL_SYSTEM_CORE0_MOD0_PERIPH_M2M            SYSTEM_CORE0_MOD0_CKE_M2M
#define LL_SYSTEM_CORE0_MOD0_PERIPH_D2F            SYSTEM_CORE0_MOD0_CKE_D2F
#define LL_SYSTEM_CORE0_MOD0_PERIPH_ADC0           SYSTEM_CORE0_MOD0_CKE_ADC0
#define LL_SYSTEM_CORE0_MOD0_PERIPH_DAC            SYSTEM_CORE0_MOD0_CKE_DAC
#define LL_SYSTEM_CORE0_MOD0_PERIPH_TC0            SYSTEM_CORE0_MOD0_CKE_TC0
#define LL_SYSTEM_CORE0_MOD0_PERIPH_TC1            SYSTEM_CORE0_MOD0_CKE_TC1
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_MOD1_PERIPH  MOD1 PERIPH
 * @{
 */
#define LL_SYSTEM_CORE0_MOD1_PERIPH_ALL            (uint32_t)0x000007DFU
#define LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC0         SYSTEM_CORE0_MOD1_CKE_SIMPTC0
#define LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC1         SYSTEM_CORE0_MOD1_CKE_SIMPTC1
#define LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC2         SYSTEM_CORE0_MOD1_CKE_SIMPTC2
#define LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC3         SYSTEM_CORE0_MOD1_CKE_SIMPTC3
#define LL_SYSTEM_CORE0_MOD1_PERIPH_RS_VDD          SYSTEM_CORE0_MOD1_CKE_RS_VDD                      /*!< Inlcude RTC, SARADC, LCSS_VBAT and GPIOL[3:0] clock */
#define LL_SYSTEM_CORE0_MOD1_PERIPH_LSRX            SYSTEM_CORE0_MOD1_CKE_LSRX
#define LL_SYSTEM_CORE0_MOD1_PERIPH_KBI0            SYSTEM_CORE0_MOD1_RST_KBI0
#define LL_SYSTEM_CORE0_MOD1_PERIPH_KBI1            SYSTEM_CORE0_MOD1_RST_KBI1
#define LL_SYSTEM_CORE0_MOD1_PERIPH_LVDCMP          SYSTEM_CORE0_MOD1_CKE_LVDCMP
#define LL_SYSTEM_CORE0_MOD1_PERIPH_CRC0            SYSTEM_CORE0_MOD1_CKE_CRC0
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_MOD2_PERIPH  MOD2 PERIPH
 * @{
 */
#define LL_SYSTEM_CORE0_MOD2_PERIPH_ALL            (uint32_t)0x000001FFU
#define LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI0          SYSTEM_CORE0_MOD2_CKE_QSPI0
#define LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI1          SYSTEM_CORE0_MOD2_CKE_QSPI1
#define LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_BUS        SYSTEM_CORE0_MOD2_CKE_SEA_BUS
#define LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_PKE        SYSTEM_CORE0_MOD2_CKE_SEA_PKE
#define LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_AES        SYSTEM_CORE0_MOD2_CKE_SEA_AES
#define LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_TRNG       SYSTEM_CORE0_MOD2_CKE_SEA_TRNG
#define LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_SHA        SYSTEM_CORE0_MOD2_CKE_SEA_SHA
#define LL_SYSTEM_CORE0_MOD2_PERIPH_KEY_STORE      SYSTEM_CORE0_MOD2_CKE_KEY_STORE
#define LL_SYSTEM_CORE0_MOD2_PERIPH_AES            SYSTEM_CORE0_MOD2_CKE_AES
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_CANSOURCE CAN Source
 * @{
 */
#define LL_SYSTEM_CORE0_CANSOURCE_PLL_HOSC              0x00000000U                                         /*!< CAN selects PLL or HOSC (determined by system clock) as clock source */
#define LL_SYSTEM_CORE0_CANSOURCE_RCH                   (SYSTEM_CORE0_CAN_CLK_CFG_CNA_CLK_SEL)              /*!< CAN selects RCH as clock source                                      */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_CANCLOCKTYPE CAN Clcok Type
 * @{
 */
#define LL_SYSTEM_CORE0_CANCLOCKTYPE_ASYN              0x00000000U                                         /*!< CAN Select asynchronous clock */
#define LL_SYSTEM_CORE0_CANCLOCKTYPE_SYN               (SYSTEM_CORE0_CAN_CLK_CFG_CAN_CLK_SRC)              /*!< CAN Select synchronous clock  */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_CANCLOCKDIV CAN Clcok Division
 * @{
 */
#define LL_SYSTEM_CORE0_CANCLOCKDIV_NONE              (SYSTEM_CORE0_CAN_CLK_CFG_CAN_CLK_DIV_NONE)             /*!< CAN Asynchronous clock not divided    */
#define LL_SYSTEM_CORE0_CANCLOCKDIV_1_5               (SYSTEM_CORE0_CAN_CLK_CFG_CAN_CLK_DIV_1_5)              /*!< CAN Asynchronous clock divided by 1.5 */
#define LL_SYSTEM_CORE0_CANCLOCKDIV_2                 (SYSTEM_CORE0_CAN_CLK_CFG_CAN_CLK_DIV_2)                /*!< CAN Asynchronous clock divided by 2   */
#define LL_SYSTEM_CORE0_CANCLOCKDIV_3                 (SYSTEM_CORE0_CAN_CLK_CFG_CAN_CLK_DIV_3)                /*!< CAN Asynchronous clock divided by 3   */
#define LL_SYSTEM_CORE0_CANCLOCKDIV_4                 (SYSTEM_CORE0_CAN_CLK_CFG_CAN_CLK_DIV_4)                /*!< CAN Asynchronous clock divided by 4   */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_CORE0FREQ CORE0 System Frequency
 * @{
 */
#define LL_SYSTEM_CORE0_CORE0FREQ_32K768              (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_32K768)             /*!< Current system main clock is 32.768KHz */
#define LL_SYSTEM_CORE0_CORE0FREQ_1M8432              (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_1M8432)             /*!< Current system main clock is 1.8432MHz */
#define LL_SYSTEM_CORE0_CORE0FREQ_3M6864              (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_3M6864)             /*!< Current system main clock is 3.6864MHz */
#define LL_SYSTEM_CORE0_CORE0FREQ_7M3728              (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_7M3728)             /*!< Current system main clock is 7.3728MHz */
#define LL_SYSTEM_CORE0_CORE0FREQ_14M7456             (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_14M7456)            /*!< Current system main clock is 14.7456MHz */
#define LL_SYSTEM_CORE0_CORE0FREQ_29M4912             (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_29M4912)            /*!< Current system main clock is 29.4912MHz */
#define LL_SYSTEM_CORE0_CORE0FREQ_OVERRANGE           (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_OVERRANGE)          /*!< Current system main clock configuration exceeds allowed operating frequency range */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_CORE1FREQ CORE1 System Frequency
 * @{
 */
#define LL_SYSTEM_CORE0_CORE1FREQ_32K768              (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_32K768)              /*!< Current system main clock is 32.768KHz */
#define LL_SYSTEM_CORE0_CORE1FREQ_1M8432              (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_1M8432)              /*!< Current system main clock is 1.8432MHz */
#define LL_SYSTEM_CORE0_CORE1FREQ_3M6864              (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_3M6864)              /*!< Current system main clock is 3.6864MHz */
#define LL_SYSTEM_CORE0_CORE1FREQ_7M3728              (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_7M3728)              /*!< Current system main clock is 7.3728MHz */
#define LL_SYSTEM_CORE0_CORE1FREQ_14M7456             (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_14M7456)             /*!< Current system main clock is 14.7456MHz */
#define LL_SYSTEM_CORE0_CORE1FREQ_29M4912             (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_29M4912)             /*!< Current system main clock is 29.4912MHz */
#define LL_SYSTEM_CORE0_CORE1FREQ_58M9824             (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_58M9824)             /*!< Current system main clock is 58.9824MHz */
#define LL_SYSTEM_CORE0_CORE1FREQ_OVERRANGE           (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_OVERRANGE)           /*!< Current system main clock configuration exceeds allowed operating frequency range */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_FLASHFREQ Flash Frequency
 * @{
 */
#define LL_SYSTEM_CORE0_FLASHFREQ_32K768              (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_32K768)              /*!< Current Flash clock is 32.768KHz */
#define LL_SYSTEM_CORE0_FLASHFREQ_1M8432              (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_1M8432)              /*!< Current Flash clock is 1.8432MHz */
#define LL_SYSTEM_CORE0_FLASHFREQ_3M6864              (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_3M6864)              /*!< Current Flash clock is 3.6864MHz */
#define LL_SYSTEM_CORE0_FLASHFREQ_7M3728              (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_7M3728)              /*!< Current Flash clock is 7.3728MHz */
#define LL_SYSTEM_CORE0_FLASHFREQ_14M7456             (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_14M7456)             /*!< Current Flash clock is 14.7456MHz */
#define LL_SYSTEM_CORE0_FLASHFREQ_29M4912             (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_29M4912)             /*!< Current Flash clock is 29.4912MHz */
#define LL_SYSTEM_CORE0_FLASHFREQ_58M9824             (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_58M9824)             /*!< Current Flash clock is 58.9824MHz */
#define LL_SYSTEM_CORE0_FLASHFREQ_OVERRANGE           (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_OVERRANGE)           /*!< Current Flash configuration exceeds allowed operating frequency range */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_CALTRIGGERSOURCE Calibration Trigger Source
 * @{
 */
#define LL_SYSTEM_CORE0_CALTRIGGERSOURCE_SOFTWAER          0x00000000U                                      /*!< CORE0 software triggers clock calibration */
#define LL_SYSTEM_CORE0_CALTRIGGERSOURCE_AOS               (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_SEL)              /*!< AOS triggers clock calibration            */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_REFSOURCE Reference Clock Source
 * @{
 */
#define LL_SYSTEM_CORE0_REFSOURCE_LOSC          (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_LOSC)              /*!< Select LOSC as reference clock     */
#define LL_SYSTEM_CORE0_REFSOURCE_RCL32K        (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_RCL32K)            /*!< Select RCL_32K as reference clock  */
#define LL_SYSTEM_CORE0_REFSOURCE_HOSC          (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_HOSC)              /*!< Select HOSC as reference clock     */
#define LL_SYSTEM_CORE0_REFSOURCE_RCHVCC        (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_RCHVCC)            /*!< Select RCH_VCC as reference clock  */
#define LL_SYSTEM_CORE0_REFSOURCE_PLL           (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_PLL)               /*!< Select PLL as reference clock      */
#define LL_SYSTEM_CORE0_REFSOURCE_RCHVBAT       (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_RCHVBAT)           /*!< Select RCH_VBAT as reference clock */
#define LL_SYSTEM_CORE0_REFSOURCE_RCL307K       (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_RCL307K)           /*!< Select RCL_307K as reference clock */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_CALSOURCE Calibration Clock Source
 * @{
 */
#define LL_SYSTEM_CORE0_CALSOURCE_LOSC          (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_LOSC)               /*!< Select LOSC as clock under test     */
#define LL_SYSTEM_CORE0_CALSOURCE_RCL32K        (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_RCL32K)             /*!< Select RCL_32K as clock under test  */
#define LL_SYSTEM_CORE0_CALSOURCE_HOSC          (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_HOSC)               /*!< Select HOSC as clock under test     */
#define LL_SYSTEM_CORE0_CALSOURCE_RCHVCC        (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_RCHVCC)             /*!< Select RCH_VCC as clock under test  */
#define LL_SYSTEM_CORE0_CALSOURCE_PLL           (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_PLL)                /*!< Select PLL as clock under test      */
#define LL_SYSTEM_CORE0_CALSOURCE_RCHVBAT       (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_RCHVBAT)            /*!< Select RCH_VBAT as clock under test */
#define LL_SYSTEM_CORE0_CALSOURCE_RCL307K       (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_RCL307K)            /*!< Select RCL_307K as clock under test */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_RSTTARGET Reset Target
 * @{
 */
#define LL_SYSTEM_CORE0_RSTTARGET_ONLY_CORE1             0x0000785AU                 /*!< Only reset core1 system                    */
#define LL_SYSTEM_CORE0_RSTTARGET_CORE0_CORE1            0x00000000U                 /*!< Reset both core0 and core1 system circuits */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_CORE1CLKCLOSEMODE Core1 Clock Close Mode
 * @{
 */
#define LL_SYSTEM_CORE0_CORE1CLKCLOSEMODE_CPU             0x000067A5U                 /*!< Only close core1 CPU clock    */
#define LL_SYSTEM_CORE0_CORE1CLKCLOSEMODE_CPU_BUS         0x0000675AU                 /*!< close core1 CPU and BUS clock */
#define LL_SYSTEM_CORE0_CORE1CLKCLOSEMODE_DISABLE         0x00000000U                 /*!< Disable close core1 clock     */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_BOR5THRESHOLD BOR5 Threshold
 * @{
 */
#define LL_SYSTEM_CORE0_BOR5THRESHOLD_2_1V_1_8V             0x000075B6U                 /*!< 2.1v/1.8v          */
#define LL_SYSTEM_CORE0_BOR5THRESHOLD_2_5V_2_2V             0x00000000U                 /*!< 2.5v/2.2v, default */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_CORE0REMAP Core0 Remap
 * @{
 */
#define LL_SYSTEM_CORE0_CORE0REMAP_0            (SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0_0)               /*!< CORE0 FLASH mapped to address 0x0020_0000 */
#define LL_SYSTEM_CORE0_CORE0REMAP_1            (SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0_1)               /*!< CORE0 FLASH mapped to address 0x0000_0000 */
#define LL_SYSTEM_CORE0_CORE0REMAP_2            (SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0_2)               /*!< CORE0 FLASH mapped to address 0x1020_0000 */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_CORE1REMAP Core1 Remap
 * @{
 */
#define LL_SYSTEM_CORE0_CORE1REMAP_0            (SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1_0)               /*!< CORE1 FLASH mapped to address 0x8000_0000 */
#define LL_SYSTEM_CORE0_CORE1REMAP_1            (SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1_1)               /*!< CORE1 FLASH mapped to address 0x9000_0000 */
#define LL_SYSTEM_CORE0_CORE1REMAP_2            (SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1_2)               /*!< CORE1 FLASH mapped to address 0x8100_0000 */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_CORE0FLASHREMAP Core0 Flash Remap
 * @{
 */
#define LL_SYSTEM_CORE0_CORE0FLASHREMAP_LOW            0x00000000U                                             /*!< CORE0 program start address mapped to lower half of CORE0 FLASH */
#define LL_SYSTEM_CORE0_CORE0FLASHREMAP_HIGH           (SYSTEM_CORE0_FLASH_MAP_CTRL_CORE0_FLASH)               /*!< CORE0 program start address mapped to upper half of CORE0 FLASH */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_CORE0FLASHREMAP Core0 Flash Remap
 * @{
 */
#define LL_SYSTEM_CORE0_CORE1FLASHREMAP_LOW            0x00000000U                                             /*!< CORE1 program start address mapped to lower half of CORE1 FLASH */
#define LL_SYSTEM_CORE0_CORE1FLASHREMAP_HIGH           (SYSTEM_CORE0_FLASH_MAP_CTRL_CORE1_FLASH)               /*!< CORE1 program start address mapped to upper half of CORE1 FLASH */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_DMACHANNEL DMA Channel
 * @{
 */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_0            0x00000000U               /*!< Channel 0: SPI0 channel access SRAM.     */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_1            0x00000001U               /*!< Channel 1: SPI1 channel access SRAM.     */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_2            0x00000002U               /*!< Channel 2: SPI2 channel access SRAM.     */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_3            0x00000003U               /*!< Channel 3: SPI3 channel access SRAM.     */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_4            0x00000004U               /*!< Channel 4: SARADC channel access SRAM.   */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_5            0x00000005U               /*!< Channel 5: DAC channel access SRAM.      */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_6            0x00000006U               /*!< Channel 6: UART0 channel access SRAM.    */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_7            0x00000007U               /*!< Channel 7: UART1 channel access SRAM.    */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_8            0x00000008U               /*!< Channel 8: UART2 channel access SRAM.    */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_9            0x00000009U               /*!< Channel 9: UART3 channel access SRAM.    */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_10           0x0000000AU               /*!< Channel 10: UART4 channel access SRAM.   */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_11           0x0000000BU               /*!< Channel 11: UART5 channel access SRAM.   */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_12           0x0000000CU               /*!< Channel 12: UART6 channel access SRAM.   */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_13           0x0000000DU               /*!< Channel 13: UART7 channel access SRAM.   */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_14           0x0000000EU               /*!< Channel 14: CAN channel access SRAM.     */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_15           0x0000000FU               /*!< Channel 15: TC0_PWM channel access SRAM. */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_16           0x00000010U               /*!< Channel 16: TC1_PWM channel access SRAM. */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_17           0x00000011U               /*!< Channel 17: AES channel access SRAM.     */
#define LL_SYSTEM_CORE0_DMAPRI0CHANNEL_18           0x00000012U               /*!< Channel 18: M2M channel access SRAM.     */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_DMAPRIORITY0 Transfer Priority level1
 * @{
 */
#define LL_SYSTEM_CORE0_DMAPRIORITY_LOW               0x00000000U            /*!< Priority level : Low       */
#define LL_SYSTEM_CORE0_DMAPRIORITY_MEDIUM            0x00000001U            /*!< Priority level : Medium    */
#define LL_SYSTEM_CORE0_DMAPRIORITY_HIGH              0x00000002U            /*!< Priority level : High      */
#define LL_SYSTEM_CORE0_DMAPRIORITY_VERYHIGH          0x00000003U            /*!< Priority level : Very_High */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EC_DMACHANNEL DMA Channel
 * @{
 */
#define LL_SYSTEM_CORE0_DMAPRI1CHANNEL_0            0x00000000U              /*!< Channel 0: First level arbitration result channel access SRAM.          */
#define LL_SYSTEM_CORE0_DMAPRI1CHANNEL_1            0x00000001U              /*!< Channel 1: General DMA read channel (MDMA0x_RD/MDMA1x_RD) access SRAM.  */
#define LL_SYSTEM_CORE0_DMAPRI1CHANNEL_2            0x00000002U              /*!< Channel 2: General DMA write channel (MDMA0x_WR/MDMA1x_WR) access SRAM. */
#define LL_SYSTEM_CORE0_DMAPRI1CHANNEL_3            0x00000003U              /*!< Channel 3: DSP channel access SRAM.                                     */
#define LL_SYSTEM_CORE0_DMAPRI1CHANNEL_4            0x00000004U              /*!< Channel 4: CORE0/CORE1 channel access SRAM.                             */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup SYSTEM_CORE0_LL_Exported_Macros SYSTEM_CORE0 Exported Macros
 * @{
 */
/** @defgroup SYSTEM_CORE0_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in SYSTEM_CORE0 register
 * @param  __INSTANCE__ SYSTEM_CORE0 Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_SYSTEM_CORE0_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in SYSTEM_CORE0 register
 * @param  __INSTANCE__ SYSTEM_CORE0 Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_SYSTEM_CORE0_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EM_Exported_Macros_Helper Exported Macros Helper
 * @{
 */

/**
 * @brief  Helper macro to calculate the Core0 CLK frequency ((in Hz))
 * @note:  __SYSCLKFREQ__ be retrieved by @ref LL_SYSTEM_CORE0_GetCore0SystemClockFreq
 * @param  __SYSCLKFREQ__: This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FREQ_32K768
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FREQ_1M8432
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FREQ_3M6864
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FREQ_29M4912
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FREQ_OVERRANGE
 * @retval HCLK clock frequency (in Hz)
 */
#define __LL_SYSTEM_CORE0_CALC_CORE0_CLK_FREQ(__SYSCLKFREQ__) \
                        (((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_CORE0FREQ_32K768) ? 32768U : \
                            ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_CORE0FREQ_1M8432) ? 1843200U : \
                                ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_CORE0FREQ_3M6864) ? 3686400U : \
                                    ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_CORE0FREQ_7M3728) ? 7372800U : \
                                        ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_CORE0FREQ_14M7456) ? 14745600U : \
                                            ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_CORE0FREQ_29M4912) ? 29491200U : 0x00U)

/**
 * @brief  Helper macro to calculate the Core1 CLK frequency ((in Hz))
 * @note:  __SYSCLKFREQ__ be retrieved by @ref LL_SYSTEM_CORE0_GetCore1SystemClockFreq
 * @param  __SYSCLKFREQ__: This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_32K768
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_1M8432
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_3M6864
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_29M4912
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_58M9824
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_OVERRANGE
 * @retval HCLK clock frequency (in Hz)
 */
#define __LL_SYSTEM_CORE0_CALC_CORE1_CLK_FREQ(__SYSCLKFREQ__) \
                        (((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_CORE1FREQ_32K768) ? 32768U : \
                            ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_CORE1FREQ_1M8432) ? 1843200U : \
                                ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_CORE1FREQ_3M6864) ? 3686400U : \
                                    ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_CORE1FREQ_7M3728) ? 7372800U : \
                                        ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_CORE1FREQ_14M7456) ? 14745600U : \
                                            ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_CORE1FREQ_29M4912) ? 29491200U : \
                                                ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_CORE1FREQ_58M9824) ? 58982400U : 0x00U)

/**
 * @brief  Helper macro to calculate the Flash CLK frequency ((in Hz))
 * @note:  __SYSCLKFREQ__ be retrieved by @ref LL_SYSTEM_CORE0_GetFlashClockFreq
 * @param  __SYSCLKFREQ__: This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_32K768
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_1M8432
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_3M6864
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_29M4912
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_58M9824
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_OVERRANGE
 * @retval HCLK clock frequency (in Hz)
 */
#define __LL_SYSTEM_CORE0_CALC_FLASH_CLK_FREQ(__SYSCLKFREQ__) \
                        (((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_FLASHFREQ_32K768) ? 32768U : \
                            ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_FLASHFREQ_1M8432) ? 1843200U : \
                                ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_FLASHFREQ_3M6864) ? 3686400U : \
                                    ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_FLASHFREQ_7M3728) ? 7372800U : \
                                        ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_FLASHFREQ_14M7456) ? 14745600U : \
                                            ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_FLASHFREQ_29M4912) ? 29491200U : \
                                                ((__SYSCLKFREQ__) == LL_SYSTEM_CORE0_FLASHFREQ_58M9824) ? 58982400U : 0x00U)

/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup SYSTEM_CORE0_LL_Exported_Functions SYSTEM_CORE0 Exported Functions
 * @{
 */

/** @defgroup SYSTEM_CORE0_LL_EF_Configuration Configuration functions
 * @{
 */

/**
 * @brief  Unlock System PSW
 * @rmtoll SYS_PSW1         SYS_PSW1         LL_SYSTEM_CORE0_UnlockSystemPSW
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_UnlockSystemPSW(void)
{
    MODIFY_REG(SYSTEM_CORE0->SYS_PSW1, SYSTEM_CORE0_SYS_PSW1_SYS_PSW1, 0x82U);
}

/**
 * @brief  Lock System PSW
 * @rmtoll SYS_PSW1         SYS_PSW1         LL_SYSTEM_CORE0_LockSystemPSW
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_LockSystemPSW(void)
{
    MODIFY_REG(SYSTEM_CORE0->SYS_PSW1, SYSTEM_CORE0_SYS_PSW1_SYS_PSW1, 0x00U);
}

/**
 * @brief  Check if System PSW has been Unloced or not
 * @rmtoll SYS_PSW1         SYS_PSW1         LL_SYSTEM_CORE0_IsUnlockedSystemPSW
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsUnlockedSystemPSW(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYS_PSW1, SYSTEM_CORE0_SYS_PSW1_SYS_PSW1) == (0x82U));
}

/**
 * @brief  Enable RCH PD
 * @rmtoll RCH_CTRL         RCH_PD         LL_SYSTEM_CORE0_EnableRCHClockPD
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_EnableRCHClockPD(void)
{
    CLEAR_BIT(SYSTEM_CORE0->RCH_CTRL, SYSTEM_CORE0_RCH_CTRL_PD);
}

/**
 * @brief  Disable RCH PD
 * @rmtoll RCH_CTRL         RCH_PD         LL_SYSTEM_CORE0_DisableRCHClockPD
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_DisableRCHClockPD(void)
{
    SET_BIT(SYSTEM_CORE0->RCH_CTRL, SYSTEM_CORE0_RCH_CTRL_PD);
}

/**
 * @brief  Check if RCH PD has been enabled or not
 * @rmtoll RCH_CTRL         RCH_PD         LL_SYSTEM_CORE0_IsEnabledRCHClockPD
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsEnabledRCHClockPD(void)
{
    return (READ_BIT(SYSTEM_CORE0->RCH_CTRL, SYSTEM_CORE0_RCH_CTRL_PD) == (SYSTEM_CORE0_RCH_CTRL_PD));
}

/**
 * @brief  Configure RCH Clock Division.
 * @rmtoll RCH_CTRL      RCH_DIV        LL_SYSTEM_CORE0_SetRCHClockDivision
 * @param ClockDivision This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_RCHCLOCKDIV_1
 *         @arg @ref LL_SYSTEM_CORE0_RCHCLOCKDIV_2
 *         @arg @ref LL_SYSTEM_CORE0_RCHCLOCKDIV_4
 *         @arg @ref LL_SYSTEM_CORE0_RCHCLOCKDIV_8
 *         @arg @ref LL_SYSTEM_CORE0_RCHCLOCKDIV_16
 *         @arg @ref LL_SYSTEM_CORE0_RCHCLOCKDIV_32
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetRCHClockDivision(uint32_t ClockDivision)
{
    MODIFY_REG(SYSTEM_CORE0->RCH_CTRL, SYSTEM_CORE0_RCH_CTRL_DIV, ClockDivision);
}

/**
 * @brief  Get the RCH Clock Division.
 * @rmtoll RCH_CTRL         RCH_DIV        LL_SYSTEM_CORE0_GetRCHClockDivision
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_RCHCLOCKDIV_1
 *         @arg @ref LL_SYSTEM_CORE0_RCHCLOCKDIV_2
 *         @arg @ref LL_SYSTEM_CORE0_RCHCLOCKDIV_4
 *         @arg @ref LL_SYSTEM_CORE0_RCHCLOCKDIV_8
 *         @arg @ref LL_SYSTEM_CORE0_RCHCLOCKDIV_16
 *         @arg @ref LL_SYSTEM_CORE0_RCHCLOCKDIV_32
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetRCHClockDivision(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->RCH_CTRL, SYSTEM_CORE0_RCH_CTRL_DIV));
}

/**
 * @brief  Enable PLL PD
 * @rmtoll PLL_CTRL         PLL_PD         LL_SYSTEM_CORE0_EnablePLLClockPD
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_EnablePLLClockPD(void)
{
    CLEAR_BIT(SYSTEM_CORE0->PLL_CTRL, SYSTEM_CORE0_PLL_CTRL_PD);
}

/**
 * @brief  Disable PLL PD
 * @rmtoll PLL_CTRL         PLL_PD         LL_SYSTEM_CORE0_DisablePLLClockPD
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_DisablePLLClockPD(void)
{
    SET_BIT(SYSTEM_CORE0->PLL_CTRL, SYSTEM_CORE0_PLL_CTRL_PD);
}

/**
 * @brief  Check if PLL PD has been enabled or not
 * @rmtoll PLL_CTRL         PLL_PD         LL_SYSTEM_CORE0_IsEnabledPLLClockPD
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsEnabledPLLClockPD(void)
{
    return (READ_BIT(SYSTEM_CORE0->PLL_CTRL, SYSTEM_CORE0_PLL_CTRL_PD) == (SYSTEM_CORE0_PLL_CTRL_PD));
}

/**
 * @brief  Configure PLL Output Frequency.
 * @rmtoll PLL_CTRL      PLL_FREQ        LL_SYSTEM_CORE0_SetPLLOutputFrequency
 * @param OutputFrequency This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_PLLOUTPUTFREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE0_PLLOUTPUTFREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE0_PLLOUTPUTFREQ_29M4912
 *         @arg @ref LL_SYSTEM_CORE0_PLLOUTPUTFREQ_58M9824
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetPLLOutputFrequency(uint32_t OutputFrequency)
{
    MODIFY_REG(SYSTEM_CORE0->PLL_CTRL, SYSTEM_CORE0_PLL_CTRL_FREQ, OutputFrequency);
}

/**
 * @brief  Get the PLL Output Frequency.
 * @rmtoll PLL_CTRL         PLL_FREQ        LL_SYSTEM_CORE0_GetPLLOutputFrequency
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_PLLOUTPUTFREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE0_PLLOUTPUTFREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE0_PLLOUTPUTFREQ_29M4912
 *         @arg @ref LL_SYSTEM_CORE0_PLLOUTPUTFREQ_58M9824
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetPLLOutputFrequency(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->PLL_CTRL, SYSTEM_CORE0_PLL_CTRL_FREQ));
}

/**
 * @brief  Configure PLL Clock Division.
 * @rmtoll PLL_CTRL      PLL_DIV        LL_SYSTEM_CORE0_SetPLLClockDivision
 * @param ClockDivision This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_PLLCLOCKDIV_1
 *         @arg @ref LL_SYSTEM_CORE0_PLLCLOCKDIV_2
 *         @arg @ref LL_SYSTEM_CORE0_PLLCLOCKDIV_4
 *         @arg @ref LL_SYSTEM_CORE0_PLLCLOCKDIV_8
 *         @arg @ref LL_SYSTEM_CORE0_PLLCLOCKDIV_16
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetPLLClockDivision(uint32_t ClockDivision)
{
    MODIFY_REG(SYSTEM_CORE0->PLL_CTRL, SYSTEM_CORE0_PLL_CTRL_DIV, ClockDivision);
}

/**
 * @brief  Get the PLL Clock Division.
 * @rmtoll PLL_CTRL         PLL_DIV        LL_SYSTEM_CORE0_GetPLLClockDivision
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_PLLCLOCKDIV_1
 *         @arg @ref LL_SYSTEM_CORE0_PLLCLOCKDIV_2
 *         @arg @ref LL_SYSTEM_CORE0_PLLCLOCKDIV_4
 *         @arg @ref LL_SYSTEM_CORE0_PLLCLOCKDIV_8
 *         @arg @ref LL_SYSTEM_CORE0_PLLCLOCKDIV_16
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetPLLClockDivision(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->PLL_CTRL, SYSTEM_CORE0_PLL_CTRL_DIV));
}

/**
 * @brief  Enable HOSC PD
 * @rmtoll HOSC_CTRL         HOSC_PD         LL_SYSTEM_CORE0_EnableHOSCClockPD
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_EnableHOSCClockPD(void)
{
    CLEAR_BIT(SYSTEM_CORE0->HOSC_CTRL, SYSTEM_CORE0_HOSC_CTRL_PD);
}

/**
 * @brief  Disable HOSC PD
 * @rmtoll HOSC_CTRL         HOSC_PD         LL_SYSTEM_CORE0_DisableHOSCClockPD
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_DisableHOSCClockPD(void)
{
    SET_BIT(SYSTEM_CORE0->HOSC_CTRL, SYSTEM_CORE0_HOSC_CTRL_PD);
}

/**
 * @brief  Check if HOSC PD has been enabled or not
 * @rmtoll HOSC_CTRL         HOSC_PD         LL_SYSTEM_CORE0_IsEnabledHOSCClockPD
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsEnabledHOSCClockPD(void)
{
    return (READ_BIT(SYSTEM_CORE0->HOSC_CTRL, SYSTEM_CORE0_HOSC_CTRL_PD) == (SYSTEM_CORE0_HOSC_CTRL_PD));
}

/**
 * @brief  Configure HOSC Crystal Frequency.
 * @rmtoll HOSC_CTRL      HOSC_FREQ        LL_SYSTEM_CORE0_SetHOSCCrystalFrequency
 * @param CrystalFrequency This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCRYSTALFREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCRYSTALFREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCRYSTALFREQ_29M4912
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetHOSCCrystalFrequency(uint32_t CrystalFrequency)
{
    MODIFY_REG(SYSTEM_CORE0->HOSC_CTRL, SYSTEM_CORE0_HOSC_CTRL_FREQ, CrystalFrequency);
}

/**
 * @brief  Get the HOSC Crystal Frequency.
 * @rmtoll HOSC_CTRL         HOSC_FREQ        LL_SYSTEM_CORE0_GetHOSCCrystalFrequency
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCRYSTALFREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCRYSTALFREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCRYSTALFREQ_29M4912
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetHOSCCrystalFrequency(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->HOSC_CTRL, SYSTEM_CORE0_HOSC_CTRL_FREQ));
}

/**
 * @brief  Configure HOSC Clock Division.
 * @rmtoll HOSC_CTRL      HOSC_DIV        LL_SYSTEM_CORE0_SetHOSCClockDivision
 * @param ClockDivision This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCLOCKDIV_1
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCLOCKDIV_2
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCLOCKDIV_4
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCLOCKDIV_8
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCLOCKDIV_16
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetHOSCClockDivision(uint32_t ClockDivision)
{
    MODIFY_REG(SYSTEM_CORE0->HOSC_CTRL, SYSTEM_CORE0_HOSC_CTRL_DIV, ClockDivision);
}

/**
 * @brief  Get the HOSC Clock Division.
 * @rmtoll HOSC_CTRL         HOSC_DIV        LL_SYSTEM_CORE0_GetHOSCClockDivision
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCLOCKDIV_1
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCLOCKDIV_2
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCLOCKDIV_4
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCLOCKDIV_8
 *         @arg @ref LL_SYSTEM_CORE0_HOSCCLOCKDIV_16
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetHOSCClockDivision(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->HOSC_CTRL, SYSTEM_CORE0_HOSC_CTRL_DIV));
}

/**
 * @brief  Configure HOSC Clock Gain.
 * @rmtoll HOSC_CTRL      HOSC_PGA        LL_SYSTEM_CORE0_SetHOSCClockGain
 * @param ClockGain This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_1X_0_8MS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_2X_2_4MS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_4X_3_2MS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_8X_5_6MS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_1X_1_2MS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_2X_1_6MS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_4X_4_8MS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_8X_6_4MS
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetHOSCClockGain(uint32_t ClockGain)
{
    MODIFY_REG(SYSTEM_CORE0->HOSC_CTRL, SYSTEM_CORE0_HOSC_CTRL_PGA, ClockGain);
}

/**
 * @brief  Get the HOSC Clock Gain.
 * @rmtoll HOSC_CTRL         HOSC_PGA        LL_SYSTEM_CORE0_GetHOSCClockGain
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_1X_0_8MS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_2X_2_4MS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_4X_3_2MS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_8X_5_6MS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_1X_1_2MS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_2X_1_6MS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_4X_4_8MS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCGAIN_8X_6_4MS
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetHOSCClockGain(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->HOSC_CTRL, SYSTEM_CORE0_HOSC_CTRL_PGA));
}

/**
 * @brief  Configure HOSC Clock Gain.
 * @rmtoll HOSC_CTRL      HOSC_DSEL        LL_SYSTEM_CORE0_SetHOSCAnalogFilterTime
 * @param Time This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_HOSCTIME_25_5NS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCTIME_10_5NS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCTIME_5NS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCTIME_NONE
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetHOSCAnalogFilterTime(uint32_t Time)
{
    MODIFY_REG(SYSTEM_CORE0->HOSC_CTRL, SYSTEM_CORE0_HOSC_CTRL_DSEL, Time);
}

/**
 * @brief  Get the HOSC Clock Gain.
 * @rmtoll HOSC_CTRL         HOSC_PGA        LL_SYSTEM_CORE0_GetAnalogFilterTime
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_HOSCTIME_25_5NS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCTIME_10_5NS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCTIME_5NS
 *         @arg @ref LL_SYSTEM_CORE0_HOSCTIME_NONE
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetAnalogFilterTime(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->HOSC_CTRL, SYSTEM_CORE0_HOSC_CTRL_DSEL));
}

/**
 * @brief  Enable RCL307K PD
 * @rmtoll RCL307K_CTRL         PD         LL_SYSTEM_CORE0_EnableRCL307ClockPD
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_EnableRCL307ClockPD(void)
{
    MODIFY_REG(SYSTEM_CORE0->RCL307K_CTRL, SYSTEM_CORE0_RCL307K_CTRL_PD, 0x00U);
}

/**
 * @brief  Disable RCL307K PD
 * @rmtoll RCL307K_CTRL         PD         LL_SYSTEM_CORE0_DisableRCL307ClockPD
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_DisableRCL307ClockPD(void)
{
    MODIFY_REG(SYSTEM_CORE0->RCL307K_CTRL, SYSTEM_CORE0_RCL307K_CTRL_PD, 0x75U);
}

/**
 * @brief  Check if RCL307K PD has been enabled or not
 * @rmtoll RCL307K_CTRL         PD         LL_SYSTEM_CORE0_IsEnabledRCL307ClockPD
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsEnabledRCL307ClockPD(void)
{
    return (READ_BIT(SYSTEM_CORE0->RCL307K_CTRL, SYSTEM_CORE0_RCL307K_CTRL_PD) != (0x75U));
}

/**
 * @brief  Configure System Low Frequency Clock Source.
 * @note   This register can only be modified in RC mode or high-frequency mode.
 * @rmtoll CLK_SEL      SYS_RCL_LOSC        LL_SYSTEM_CORE0_SetSystemLowFreqClockSource
 * @param SystemClockSource This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_SYSTEMLOWFREQSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_SYSTEMLOWFREQSOURCE_RCL32K
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetSystemLowFreqClockSource(uint32_t SystemClockSource)
{
    MODIFY_REG(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_SYS_RCL_LOSC, SystemClockSource);
}

/**
 * @brief  Get the System Low Frequency Clock Source.
 * @rmtoll CLK_SEL         SYS_RCL_LOSC        LL_SYSTEM_CORE0_GetSystemLowFreqClockSource
 * @note   This register can only be modified in RC mode or high-frequency mode.
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_SYSTEMLOWFREQSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_SYSTEMLOWFREQSOURCE_RCL32K
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetSystemLowFreqClockSource(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_SYS_RCL_LOSC));
}

/**
 * @brief  Configure System High Frequency Clock Source.
 * @note   This configuration can only be set in RC mode or low-frequency mode.
 * @rmtoll CLK_SEL      SYS_PLL_HOSC        LL_SYSTEM_CORE0_SetSystemHighFreqClockSource
 * @param SystemClockSource This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_SYSTEMHIGHFREQSOURCE_PLL
 *         @arg @ref LL_SYSTEM_CORE0_SYSTEMHIGHFREQSOURCE_HOSC
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetSystemHighFreqClockSource(uint32_t SystemClockSource)
{
    MODIFY_REG(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_SYS_PLL_HOSC, SystemClockSource);
}

/**
 * @brief  Get the System High Frequency Clock Source.
 * @rmtoll CLK_SEL         SYS_PLL_HOSC        LL_SYSTEM_CORE0_GetSystemHighFreqClockSource
 * @note   This configuration can only be set in RC mode or low-frequency mode.
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_SYSTEMHIGHFREQSOURCE_PLL
 *         @arg @ref LL_SYSTEM_CORE0_SYSTEMHIGHFREQSOURCE_HOSC
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetSystemHighFreqClockSource(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_SYS_PLL_HOSC));
}


/**
 * @brief  Configure LCD Clock Source.
 * @rmtoll CLK_SEL      LCD_RCL_LOSC        LL_SYSTEM_CORE0_SetLCDClockSource
 * @param LCDSource This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_LCDSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_LCDSOURCE_RCL32K
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetLCDClockSource(uint32_t LCDSource)
{
    MODIFY_REG(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC, LCDSource);
}

/**
 * @brief  Get the LCD Clock Source.
 * @rmtoll CLK_SEL         LCD_RCL_LOSC        LL_SYSTEM_CORE0_GetLCDClockSource
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_LCDSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_LCDSOURCE_RCL32K
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetLCDClockSource(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC));
}

/**
 * @brief  Configure IRFR Clock Source.
 * @rmtoll CLK_SEL      IRFR_RCL_LOSC        LL_SYSTEM_CORE0_SetIRFRClockSource
 * @param IRFRSource This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_IRFRSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_IRFRSOURCE_RCL32K
 *         @arg @ref LL_SYSTEM_CORE0_IRFRSOURCE_RCL307K
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetIRFRClockSource(uint32_t IRFRSource)
{
    MODIFY_REG(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC, IRFRSource);
}

/**
 * @brief  Get the IRFR Clock Source.
 * @rmtoll CLK_SEL         IRFR_RCL_LOSC        LL_SYSTEM_CORE0_GetIRFRClockSource
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_IRFRSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_IRFRSOURCE_RCL32K
 *         @arg @ref LL_SYSTEM_CORE0_IRFRSOURCE_RCL307K
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetIRFRClockSource(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC));
}

/**
 * @brief  Configure KBI Low Frequency Clock Source.
 * @rmtoll CLK_SEL      KBI_RCL_LOSC        LL_SYSTEM_CORE0_SetKBIClockSource
 * @param KBISource This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_KBISOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_KBISOURCE_RCL32K
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetKBIClockSource(uint32_t KBISource)
{
    MODIFY_REG(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_KBI_RCL_LOSC, KBISource);
}

/**
 * @brief  Get the KBI Low Frequency Clock Source.
 * @rmtoll CLK_SEL         KBI_RCL_LOSC        LL_SYSTEM_CORE0_GetKBIClockSource
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_KBISOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_KBISOURCE_RCL32K
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetKBIClockSource(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_KBI_RCL_LOSC));
}

/**
 * @brief  Configure LPUART Low Frequency Clock Source.
 * @rmtoll CLK_SEL      LPUART_RCL_LOSC        LL_SYSTEM_CORE0_SetLPUARTLowFreqClockSource
 * @param Source This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_LPUARTLOWFREQSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_LPUARTLOWFREQSOURCE_RCL32K
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetLPUARTLowFreqClockSource(uint32_t Source)
{
    MODIFY_REG(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_LPUART_RCL_LOSC, Source);
}

/**
 * @brief  Get the LPUART Low Frequency Clock Source.
 * @rmtoll CLK_SEL         LPUART_RCL_LOSC        LL_SYSTEM_CORE0_GetLPUARTLowFreqClockSource
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_LPUARTLOWFREQSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_LPUARTLOWFREQSOURCE_RCL32K
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetLPUARTLowFreqClockSource(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_LPUART_RCL_LOSC));
}

/**
 * @brief  Enable RCL32K clock channel to VBAT domain
 * @rmtoll CLK_SEL         VBAT_RCL         LL_SYSTEM_CORE0_EnableVBATRCL32KClockChannel
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_EnableVBATRCL32KClockChannel(void)
{
    SET_BIT(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_VBAT_RCL);
}

/**
 * @brief  Disable RCL32K clock channel to VBAT domain
 * @rmtoll CLK_SEL         VBAT_RCL         LL_SYSTEM_CORE0_DisableVBATRCL32KClockChannel
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_DisableVBATRCL32KClockChannel(void)
{
    CLEAR_BIT(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_VBAT_RCL);
}

/**
 * @brief  Check if RCL32K clock channel to VBAT domain has been enabled or not
 * @rmtoll CLK_SEL         VBAT_RCL         LL_SYSTEM_CORE0_IsEnabledVBATRCL32KClockChannel
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsEnabledVBATRCL32KClockChannel(void)
{
    return (READ_BIT(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_VBAT_RCL) == (SYSTEM_CORE0_CLK_SEL_VBAT_RCL));
}

/**
 * @brief  Configure D2F Low Frequency Clock Source.
 * @rmtoll CLK_SEL      D2F_RCL_LOSC        LL_SYSTEM_CORE0_SetD2FLowFreqClockSource
 * @param Source This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_D2FLOWFREQSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_D2FLOWFREQSOURCE_RCL32K
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetD2FLowFreqClockSource(uint32_t Source)
{
    MODIFY_REG(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_D2F_RCL_LOSC, Source);
}

/**
 * @brief  Get the D2F Low Frequency Clock Source.
 * @rmtoll CLK_SEL         D2F_RCL_LOSC        LL_SYSTEM_CORE0_GetD2FLowFreqClockSource
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_D2FLOWFREQSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_D2FLOWFREQSOURCE_RCL32K
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetD2FLowFreqClockSource(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_D2F_RCL_LOSC));
}

/**
 * @brief  Configure Core0 Low Frequency Clock Source.
 * @rmtoll CLK_SEL      CORE0_RCL_LOSC        LL_SYSTEM_CORE0_SetCore0LowFreqClockSource
 * @param Source This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE0LOWFREQSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_CORE0LOWFREQSOURCE_RCL32K
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetCore0LowFreqClockSource(uint32_t Source)
{
    MODIFY_REG(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_CORE0_RCL_LOSC, Source);
}

/**
 * @brief  Get the Core0 Low Frequency Clock Source.
 * @rmtoll CLK_SEL         CORE0_RCL_LOSC        LL_SYSTEM_CORE0_GetCore0LowFreqClockSource
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE0LOWFREQSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_CORE0LOWFREQSOURCE_RCL32K
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetCore0LowFreqClockSource(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_CORE0_RCL_LOSC));
}

/**
 * @brief  Configure Core1 Low Frequency Clock Source.
 * @rmtoll CLK_SEL      CORE1_RCL_LOSC        LL_SYSTEM_CORE0_SetCore1LowFreqClockSource
 * @param Source This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE1LOWFREQSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_CORE1LOWFREQSOURCE_RCL32K
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetCore1LowFreqClockSource(uint32_t Source)
{
    MODIFY_REG(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_CORE1_RCL_LOSC, Source);
}

/**
 * @brief  Get the Core1 Low Frequency Clock Source.
 * @rmtoll CLK_SEL         CORE1_RCL_LOSC        LL_SYSTEM_CORE0_GetCore1LowFreqClockSource
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE1LOWFREQSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_CORE1LOWFREQSOURCE_RCL32K
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetCore1LowFreqClockSource(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CLK_SEL, SYSTEM_CORE0_CLK_SEL_CORE1_RCL_LOSC));
}

/**
 * @brief  Configure System Clock Mode.
 * @rmtoll SYS_MODE      SYS_MODE        LL_SYSTEM_CORE0_SetSystemClockMode
 * @param Mode This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_SYSCLOCKMODE_HCM
 *         @arg @ref LL_SYSTEM_CORE0_SYSCLOCKMODE_RCM
 *         @arg @ref LL_SYSTEM_CORE0_SYSCLOCKMODE_LCM
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetSystemClockMode(uint32_t Mode)
{
    if (Mode == LL_SYSTEM_CORE0_SYSCLOCKMODE_HCM) {
        MODIFY_REG(SYSTEM_CORE0->SYS_MODE, SYSTEM_CORE0_SYS_MODE_SYS_MODE, 0xDU);
    } else if (Mode == LL_SYSTEM_CORE0_SYSCLOCKMODE_RCM) {
        MODIFY_REG(SYSTEM_CORE0->SYS_MODE, SYSTEM_CORE0_SYS_MODE_SYS_MODE, 0xEU);
    } else if (Mode == LL_SYSTEM_CORE0_SYSCLOCKMODE_LCM) {
        MODIFY_REG(SYSTEM_CORE0->SYS_MODE, SYSTEM_CORE0_SYS_MODE_SYS_MODE, 0xFU);
    } else {
        /* Nothing to do */
    }
}

/**
 * @brief  Get the System Clock Mode.
 * @rmtoll SYS_MODE         SYS_MODE        LL_SYSTEM_CORE0_GetSystemClockMode
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_SYSCLOCKMODE_HCM
 *         @arg @ref LL_SYSTEM_CORE0_SYSCLOCKMODE_RCM
 *         @arg @ref LL_SYSTEM_CORE0_SYSCLOCKMODE_LCM
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetSystemClockMode(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->SYS_MODE, SYSTEM_CORE0_SYS_MODE_SYS_MODE));
}

/**
 * @brief  Configure CORE0/CORE1 main clock division.
 * @note   CORE0 does not support the 58.9824MHz frequency.
 * @rmtoll CORE_DIV      CORE1_CORE0_DIV        LL_SYSTEM_CORE0_SetCore1Core0ClockDivision
 * @param ClockDivision This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_1_CORE0_1
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_1_CORE0_2
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_1_CORE0_4
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_1_CORE0_8
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_2_CORE0_2
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_2_CORE0_4
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_2_CORE0_8
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_4_CORE0_4
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_4_CORE0_8
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_8_CORE0_8
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetCore1Core0ClockDivision(uint32_t ClockDivision)
{
    MODIFY_REG(SYSTEM_CORE0->CORE_DIV, SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV, ClockDivision);
}

/**
 * @brief  Get the CORE0/CORE1 main clock division.
 * @note   CORE0 does not support the 58.9824MHz frequency.
 * @rmtoll CORE_DIV         CORE1_CORE0_DIV        LL_SYSTEM_CORE0_GetCore1Core0ClockDivision
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_1_CORE0_1
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_1_CORE0_2
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_1_CORE0_4
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_1_CORE0_8
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_2_CORE0_2
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_2_CORE0_4
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_2_CORE0_8
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_4_CORE0_4
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_4_CORE0_8
 *         @arg @ref LL_SYSTEM_CORE0_COREDIV_CORE1_8_CORE0_8
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetCore1Core0ClockDivision(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CORE_DIV, SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV));
}

/**
 * @brief  Get the CORE0 System Main Clock Frequency (read only).
 * @rmtoll SYSFREQ_STA         CORE0_SYS_FREQ        LL_SYSTEM_CORE0_GetCore0SystemClockFreq
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FREQ_32K768
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FREQ_1M8432
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FREQ_3M6864
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FREQ_29M4912
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FREQ_OVERRANGE
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetCore0SystemClockFreq(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->SYSFREQ_STA, SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS));
}

/**
 * @brief  Get the CORE1 System Main Clock Frequency (read only).
 * @rmtoll SYSFREQ_STA         CORE1_SYS_FREQ        LL_SYSTEM_CORE0_GetCore1SystemClockFreq
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_32K768
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_1M8432
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_3M6864
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_29M4912
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_58M9824
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FREQ_OVERRANGE
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetCore1SystemClockFreq(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->SYSFREQ_STA, SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS));
}

/**
 * @brief  Get the Flash Clock Frequency (read only).
 * @rmtoll SYSFREQ_STA         FLASH_CLK_FREQ        LL_SYSTEM_CORE0_GetFlashSystemClockFreq
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_32K768
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_1M8432
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_3M6864
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_29M4912
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_58M9824
 *         @arg @ref LL_SYSTEM_CORE0_FLASHFREQ_OVERRANGE
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetFlashSystemClockFreq(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->SYSFREQ_STA, SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK));
}

/**
 * @brief  Configure Core1 Clock Close Mode.
 * @rmtoll CORE1_CKE      CORE1_CKE        LL_SYSTEM_CORE0_SetCore1ClockCloseMode
 * @param CloseMode This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE1CLKCLOSEMODE_CPU
 *         @arg @ref LL_SYSTEM_CORE0_CORE1CLKCLOSEMODE_CPU_BUS
 *         @arg @ref LL_SYSTEM_CORE0_CORE1CLKCLOSEMODE_DISABLE
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetCore1ClockCloseMode(uint32_t CloseMode)
{
    MODIFY_REG(SYSTEM_CORE0->CORE1_CKE, SYSTEM_CORE0_CORE1_CKE_CORE1_CKE, CloseMode);
}

/**
 * @brief  Get the Core1 Clock Close Mode.
 * @rmtoll CORE1_CKE         CORE1_CKE        LL_SYSTEM_CORE0_GetCore1ClockCloseMode
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE1CLKCLOSEMODE_CPU
 *         @arg @ref LL_SYSTEM_CORE0_CORE1CLKCLOSEMODE_CPU_BUS
 *         @arg @ref LL_SYSTEM_CORE0_CORE1CLKCLOSEMODE_DISABLE
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetCore1ClockCloseMode(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CORE1_CKE, SYSTEM_CORE0_CORE1_CKE_CORE1_CKE));
}

/**
 * @brief  Configure BOR5 Threshold.
 * @rmtoll BOR5_CTRL      BOR5_SEL        LL_SYSTEM_CORE0_SetBOR5Threshold
 * @param Threshold This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_BOR5THRESHOLD_2_1V_1_8V
 *         @arg @ref LL_SYSTEM_CORE0_BOR5THRESHOLD_2_5V_2_2V
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetBOR5Threshold(uint32_t Threshold)
{
    MODIFY_REG(SYSTEM_CORE0->BOR5_CTRL, SYSTEM_CORE0_BOR5_CTRL_BOR5_SEL, Threshold);
}

/**
 * @brief  Get the BOR5 Threshold.
 * @rmtoll BOR5_CTRL         BOR5_SEL        LL_SYSTEM_CORE0_GetBOR5Threshold
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_BOR5THRESHOLD_2_1V_1_8V
 *         @arg @ref LL_SYSTEM_CORE0_BOR5THRESHOLD_2_5V_2_2V
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetBOR5Threshold(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->BOR5_CTRL, SYSTEM_CORE0_BOR5_CTRL_BOR5_SEL));
}

/**
 * @brief  Configure CORE0 Remap.
 * @rmtoll ADDR_MAP_CTRL      CORE0_REMAP        LL_SYSTEM_CORE0_SetCore0AddressRemap
 * @param Remap This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE0REMAP_0
 *         @arg @ref LL_SYSTEM_CORE0_CORE0REMAP_1
 *         @arg @ref LL_SYSTEM_CORE0_CORE0REMAP_2
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetCore0AddressRemap(uint32_t Remap)
{
    MODIFY_REG(SYSTEM_CORE0->ADDR_MAP_CTRL, SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0, Remap);
}

/**
 * @brief  Get the CORE0 Remap.
 * @rmtoll ADDR_MAP_CTRL         CORE0_REMAP        LL_SYSTEM_CORE0_GetCore0AddressRemap
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE0REMAP_0
 *         @arg @ref LL_SYSTEM_CORE0_CORE0REMAP_1
 *         @arg @ref LL_SYSTEM_CORE0_CORE0REMAP_2
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetCore0AddressRemap(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->ADDR_MAP_CTRL, SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0));
}

/**
 * @brief  Get the CORE1 Remap.
 * @rmtoll ADDR_MAP_CTRL         CORE1_REMAP        LL_SYSTEM_CORE0_GetCore1AddressRemap
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE1REMAP_0
 *         @arg @ref LL_SYSTEM_CORE0_CORE1REMAP_1
 *         @arg @ref LL_SYSTEM_CORE0_CORE1REMAP_2
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetCore1AddressRemap(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->ADDR_MAP_CTRL, SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1));
}

/**
 * @brief  Configure CORE0 Flash Remap.
 * @rmtoll FLASH_MAP_CTRL      CORE0_FLASH_REMAP        LL_SYSTEM_CORE0_SetCore0FlashRemap
 * @param Remap This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FLASHREMAP_LOW
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FLASHREMAP_HIGH
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetCore0FlashRemap(uint32_t Remap)
{
    MODIFY_REG(SYSTEM_CORE0->FLASH_MAP_CTRL, SYSTEM_CORE0_FLASH_MAP_CTRL_CORE0_FLASH, Remap);
}

/**
 * @brief  Get the CORE0 Flash Remap.
 * @rmtoll FLASH_MAP_CTRL         CORE0_FLASH_REMAP        LL_SYSTEM_CORE0_GetCore0FlashRemap
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FLASHREMAP_LOW
 *         @arg @ref LL_SYSTEM_CORE0_CORE0FLASHREMAP_HIGH
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetCore0FlashRemap(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->FLASH_MAP_CTRL, SYSTEM_CORE0_FLASH_MAP_CTRL_CORE0_FLASH));
}

/**
 * @brief  Get the CORE1 Flash Remap.
 * @rmtoll FLASH_MAP_CTRL         CORE1_FLASH_REMAP        LL_SYSTEM_CORE0_GetCore1FlashRemap
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FLASHREMAP_LOW
 *         @arg @ref LL_SYSTEM_CORE0_CORE1FLASHREMAP_HIGH
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetCore1FlashRemap(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->FLASH_MAP_CTRL, SYSTEM_CORE0_FLASH_MAP_CTRL_CORE1_FLASH));
}

/**
 * @brief  Configure DMA Access SRAM Priority First Level.
 * @rmtoll DMA_PRI0_CTRL      DMA_CH0_PRI0        LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH1_PRI0        LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH2_PRI0        LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH3_PRI0        LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH4_PRI0        LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH5_PRI0        LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH6_PRI0        LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH7_PRI0        LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH8_PRI0        LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH9_PRI0        LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH10_PRI0       LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH11_PRI0       LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH12_PRI0       LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH13_PRI0       LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH14_PRI0       LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH15_PRI0       LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI1_CTRL      DMA_CH16_PRI0       LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI1_CTRL      DMA_CH17_PRI0       LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI1_CTRL      DMA_CH18_PRI0       LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_15
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_16
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_17
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_18
 * @param Priority This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_LOW
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_MEDIUM
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_HIGH
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_VERYHIGH
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_DMA_SetChannelPriorityFirstLevel(uint32_t Channel, uint32_t Priority)
{
    if (Channel <= LL_SYSTEM_CORE0_DMAPRI0CHANNEL_15) {
        MODIFY_REG(SYSTEM_CORE0->DMA_PRI0_CTRL, SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH0 << (Channel * 2), Priority << (Channel * 2));
    } else {
        MODIFY_REG(SYSTEM_CORE0->DMA_PRI1_CTRL, SYSTEM_CORE0_DMA_PRI1_CTRL_DMA_CH16 << ((Channel - 16) * 2), Priority << ((Channel - 16) * 2));
    }
}

/**
 * @brief  Get the DMA Access SRAM Priority First Level.
 * @rmtoll DMA_PRI0_CTRL      DMA_CH0_PRI0        LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH1_PRI0        LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH2_PRI0        LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH3_PRI0        LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH4_PRI0        LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH5_PRI0        LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH6_PRI0        LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH7_PRI0        LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH8_PRI0        LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH9_PRI0        LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH10_PRI0       LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH11_PRI0       LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH12_PRI0       LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH13_PRI0       LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH14_PRI0       LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH15_PRI0       LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI1_CTRL      DMA_CH16_PRI0       LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI1_CTRL      DMA_CH17_PRI0       LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI1_CTRL      DMA_CH18_PRI0       LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_15
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_16
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_17
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI0CHANNEL_18
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_LOW
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_MEDIUM
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_HIGH
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_VERYHIGH
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_DMA_GetChannelPriorityFirstLevel(uint32_t Channel)
{
    if (Channel <= LL_SYSTEM_CORE0_DMAPRI0CHANNEL_15) {
        return (uint32_t)(READ_BIT(SYSTEM_CORE0->DMA_PRI0_CTRL, SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH0 << (Channel * 2)) >> (Channel * 2));
    } else {
        return (uint32_t)(READ_BIT(SYSTEM_CORE0->DMA_PRI1_CTRL, SYSTEM_CORE0_DMA_PRI1_CTRL_DMA_CH16 << ((Channel - 16) * 2)) >> ((Channel - 16) * 2));
    }
}

/**
 * @brief  Configure DMA Access SRAM Priority Second Level.
 * @rmtoll DMA_PRI2_CTRL      DMA_CH0_PRI1        LL_SYSTEM_CORE0_DMA_SetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH1_PRI1        LL_SYSTEM_CORE0_DMA_SetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH2_PRI1        LL_SYSTEM_CORE0_DMA_SetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH3_PRI1        LL_SYSTEM_CORE0_DMA_SetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH4_PRI1        LL_SYSTEM_CORE0_DMA_SetChannelPrioritySecondLevel
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI1CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI1CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI1CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI1CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI1CHANNEL_4
 * @param Priority This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_LOW
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_MEDIUM
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_HIGH
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_VERYHIGH
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_DMA_SetChannelPrioritySecondLevel(uint32_t Channel, uint32_t Priority)
{
    MODIFY_REG(SYSTEM_CORE0->DMA_PRI2_CTRL, SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH0 << (Channel * 2), Priority << (Channel * 2));
}

/**
 * @brief  Get the DMA Access SRAM Priority Second Level.
 * @rmtoll DMA_PRI2_CTRL      DMA_CH0_PRI1        LL_SYSTEM_CORE0_DMA_GetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH1_PRI1        LL_SYSTEM_CORE0_DMA_GetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH2_PRI1        LL_SYSTEM_CORE0_DMA_GetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH3_PRI1        LL_SYSTEM_CORE0_DMA_GetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH4_PRI1        LL_SYSTEM_CORE0_DMA_GetChannelPrioritySecondLevel
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI1CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI1CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI1CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI1CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRI1CHANNEL_4
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_LOW
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_MEDIUM
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_HIGH
 *         @arg @ref LL_SYSTEM_CORE0_DMAPRIORITY_VERYHIGH
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_DMA_GetChannelPrioritySecondLevel(uint32_t Channel)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->DMA_PRI2_CTRL, SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH0 << (Channel * 2)) >> (Channel * 2));
}

/**
 * @brief  Configure Infrared clock division.
 * @note   Cannot be configured as 0.
 * @rmtoll IRFR_CTRL      INFRARED_CYCLE        LL_SYSTEM_CORE0_SetInfraredClockDivison
 * @param  ClockDivison This parameter must be a value between Min_Data=00 and Max_Data=0xFF.
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetInfraredClockDivison(uint32_t ClockDivison)
{
    MODIFY_REG(SYSTEM_CORE0->IRFR_CTRL, SYSTEM_CORE0_IRFR_CTRL_INFRARED_CYCLE, ClockDivison << SYSTEM_CORE0_IRFR_CTRL_INFRARED_CYCLE_POS);
}

/**
 * @brief  Get the Infrared clock division.
 * @note   Cannot be configured as 0.
 * @rmtoll IRFR_CTRL      INFRARED_CYCLE        LL_SYSTEM_CORE0_GetInfraredClockDivison
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF.
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetInfraredClockDivison(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->IRFR_CTRL, SYSTEM_CORE0_IRFR_CTRL_INFRARED_CYCLE) >> SYSTEM_CORE0_IRFR_CTRL_INFRARED_CYCLE_POS);
}

/**
 * @brief  Get the CHIP_ID.
 * @rmtoll CHIP_ID      CHIP_ID        LL_SYSTEM_CORE0_GetChipID
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFF.
 */
__STATIC_INLINE uint16_t LL_SYSTEM_CORE0_GetChipID(void)
{
    return (uint16_t)(READ_BIT(SYSTEM_CORE0->CHIP_ID, SYSTEM_CORE0_CHIP_ID_CHIP_ID) >> SYSTEM_CORE0_CHIP_ID_CHIP_ID_POS);
}

/**
 * @brief  Get the Version Number.
 * @rmtoll DIE_VER      VERSION        LL_SYSTEM_CORE0_GetChipVersion
 * @retval Value between Min_Data=0x00 and Max_Data=0x0F.
 */
__STATIC_INLINE uint8_t LL_SYSTEM_CORE0_GetChipVersion(void)
{
    return (uint8_t)(READ_BIT(SYSTEM_CORE0->DIE_VER, SYSTEM_CORE0_DIE_VER_VERSION) >> SYSTEM_CORE0_DIE_VER_VERSION_POS);
}

/**
 * @brief  Get the Chip Model, Encoded in ASCII.
 * @rmtoll DIE_VER      CHIP_MODEL        LL_SYSTEM_CORE0_GetChipType
 * @retval Value between Min_Data=0x00 and Max_Data=0x0FFFFFFF.
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetChipType(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->DIE_VER, SYSTEM_CORE0_DIE_VER_CHIP_MODEL) >> SYSTEM_CORE0_DIE_VER_CHIP_MODEL_POS);
}

/**
 * @brief  Get the the wafer LOT NUM information.
 * @rmtoll FAB_UID0      LOT_NUM        LL_SYSTEM_CORE0_GetLOTNumber
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetLOTNumber(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->DIE_VER, SYSTEM_CORE0_FAB_UID0_LOT_NUM) >> SYSTEM_CORE0_DIE_VER_CHIP_MODEL_POS);
}

/**
 * @brief  Get the the wafer LOT NUM information.
 * @rmtoll FAB_UID1      WAFER_ID        LL_SYSTEM_CORE0_GetWaferID
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF.
 */
__STATIC_INLINE uint8_t LL_SYSTEM_CORE0_GetWaferID(void)
{
    return (uint8_t)(READ_BIT(SYSTEM_CORE0->DIE_VER, SYSTEM_CORE0_FAB_UID1_WAFER_ID) >> SYSTEM_CORE0_FAB_UID1_WAFER_ID_POS);
}

/**
 * @brief  Get the the DIE X-coordinate information.
 * @rmtoll FAB_UID1      X_LABEL        LL_SYSTEM_CORE0_GetXLabel
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFF.
 */
__STATIC_INLINE uint16_t LL_SYSTEM_CORE0_GetXLabel(void)
{
    return (uint8_t)(READ_BIT(SYSTEM_CORE0->DIE_VER, SYSTEM_CORE0_FAB_UID1_X_LABEL) >> SYSTEM_CORE0_FAB_UID1_X_LABEL_POS);
}

/**
 * @brief  Get the the DIE Y-coordinate information.
 * @rmtoll FAB_UID1      Y_LABEL        LL_SYSTEM_CORE0_GetYLabel
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFF.
 */
__STATIC_INLINE uint16_t LL_SYSTEM_CORE0_GetYLabel(void)
{
    return (uint8_t)(READ_BIT(SYSTEM_CORE0->DIE_VER, SYSTEM_CORE0_FAB_UID1_Y_LABEL) >> SYSTEM_CORE0_FAB_UID1_Y_LABEL_POS);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EF_Clock_Calibration Clock Calibration functions
 * @{
 */

/**
 * @brief  Clock Calibration Start.
 * @note   This bit is automatically cleared when calibration completes or is stopped.
 * @rmtoll CLK_CAL_CTRL         CAL_START         LL_SYSTEM_CORE0_StartClockCalibration=
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_StartClockCalibration(void)
{
    CLEAR_BIT(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_CAL_STOP);
    SET_BIT(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_CAL_START);
}

/**
 * @brief  Clock Calibration Stop.
 * @note   If stopping clock calibration,
 *         this bit must be written to 1 before restarting calibration.
 * @rmtoll CLK_CAL_CTRL         CAL_STOP         LL_SYSTEM_CORE0_StopClockCalibration
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_StopClockCalibration(void)
{
    SET_BIT(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_CAL_STOP);
}

/**
 * @brief  Configure Calibration Trigger Source.
 * @rmtoll CLK_CAL_CTRL      CAL_SEL        LL_SYSTEM_CORE0_SetClockCalibrationTriggerSource
 * @param Source This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CALTRIGGERSOURCE_SOFTWAER
 *         @arg @ref LL_SYSTEM_CORE0_CALTRIGGERSOURCE_AOS
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetClockCalibrationTriggerSource(uint32_t Source)
{
    MODIFY_REG(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_CAL_SEL, Source);
}

/**
 * @brief  Get the Calibration Trigger Source.
 * @rmtoll CLK_CAL_CTRL         CAL_SEL        LL_SYSTEM_CORE0_GetClockCalibrationTriggerSource
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CALTRIGGERSOURCE_SOFTWAER
 *         @arg @ref LL_SYSTEM_CORE0_CALTRIGGERSOURCE_AOS
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetClockCalibrationTriggerSource(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_CAL_SEL));
}

/**
 * @brief  Configure Reference Clock Source.
 * @rmtoll CLK_CAL_CTRL      REF_CLK_SEL        LL_SYSTEM_CORE0_SetReferenceClockSource
 * @param Source This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_REFSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_REFSOURCE_RCL32K
 *         @arg @ref LL_SYSTEM_CORE0_REFSOURCE_HOSC
 *         @arg @ref LL_SYSTEM_CORE0_REFSOURCE_RCHVCC
 *         @arg @ref LL_SYSTEM_CORE0_REFSOURCE_PLL
 *         @arg @ref LL_SYSTEM_CORE0_REFSOURCE_RCHVBAT
 *         @arg @ref LL_SYSTEM_CORE0_REFSOURCE_RCL307K
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetReferenceClockSource(uint32_t Source)
{
    MODIFY_REG(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL, Source);
}

/**
 * @brief  Get the Reference Clock Source.
 * @rmtoll CLK_CAL_CTRL         REF_CLK_SEL        LL_SYSTEM_CORE0_GetReferenceClockSource
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_REFSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_REFSOURCE_RCL32K
 *         @arg @ref LL_SYSTEM_CORE0_REFSOURCE_HOSC
 *         @arg @ref LL_SYSTEM_CORE0_REFSOURCE_RCHVCC
 *         @arg @ref LL_SYSTEM_CORE0_REFSOURCE_PLL
 *         @arg @ref LL_SYSTEM_CORE0_REFSOURCE_RCHVBAT
 *         @arg @ref LL_SYSTEM_CORE0_REFSOURCE_RCL307K
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetReferenceClockSource(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL));
}

/**
 * @brief  Configure be Calibrated Clock Source.
 * @rmtoll CLK_CAL_CTRL      CAL_CLK_SEL        LL_SYSTEM_CORE0_SetCalibratedClockSource
 * @param Source This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CALSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_CALSOURCE_RCL32K
 *         @arg @ref LL_SYSTEM_CORE0_CALSOURCE_HOSC
 *         @arg @ref LL_SYSTEM_CORE0_CALSOURCE_RCHVCC
 *         @arg @ref LL_SYSTEM_CORE0_CALSOURCE_PLL
 *         @arg @ref LL_SYSTEM_CORE0_CALSOURCE_RCHVBAT
 *         @arg @ref LL_SYSTEM_CORE0_CALSOURCE_RCL307K
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetCalibratedClockSource(uint32_t Source)
{
    MODIFY_REG(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL, Source);
}

/**
 * @brief  Get the be Calibrated Clock Source.
 * @rmtoll CLK_CAL_CTRL         CAL_CLK_SEL        LL_SYSTEM_CORE0_GetCalibratedClockSource
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_CALSOURCE_LOSC
 *         @arg @ref LL_SYSTEM_CORE0_CALSOURCE_RCL32K
 *         @arg @ref LL_SYSTEM_CORE0_CALSOURCE_HOSC
 *         @arg @ref LL_SYSTEM_CORE0_CALSOURCE_RCHVCC
 *         @arg @ref LL_SYSTEM_CORE0_CALSOURCE_PLL
 *         @arg @ref LL_SYSTEM_CORE0_CALSOURCE_RCHVBAT
 *         @arg @ref LL_SYSTEM_CORE0_CALSOURCE_RCL307K
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetCalibratedClockSource(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL));
}

/**
 * @brief  Configure Reference Clock Count.
 * @rmtoll CLK_REF_CNT      REF_CLK_CNT        LL_SYSTEM_CORE0_SetReferenceClockCount
 * @param  Count This parameter must be a value between Min_Data=00 and Max_Data=0xFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetReferenceClockCount(uint32_t Count)
{
    MODIFY_REG(SYSTEM_CORE0->CLK_REF_CNT, SYSTEM_CORE0_CLK_REF_CNT_REF_CLK_CNT, Count);
}

/**
 * @brief  Get the Reference Clock Count.
 * @rmtoll CLK_REF_CNT         REF_CLK_CNT        LL_SYSTEM_CORE0_GetReferenceClockCount
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFFFF.
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetReferenceClockCount(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CLK_REF_CNT, SYSTEM_CORE0_CLK_REF_CNT_REF_CLK_CNT));
}

/**
 * @brief  Get the be Calibrated Clock Count.
 * @rmtoll CLK_CAL_CNT         CAL_CLK_CNT        LL_SYSTEM_CORE0_GetCalibratedClockCount
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFFFF.
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetCalibratedClockCount(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->CLK_CAL_CNT, SYSTEM_CORE0_CLK_CAL_CNT_CAL_CLK_CNT));
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EF_Reset Reset functions
 * @{
 */

/**
 * @brief  Enable CORE0 Reset CORE1
 * @note   After useing Enable to reset core1, should disable this bits,
 *         so that the core1 reset will be effective.
 * @rmtoll RST_CTRL         CORE0T1_RST_EN         LL_SYSTEM_CORE0_EnableCore0ResetCore1
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_EnableCore0ResetCore1(void)
{
    MODIFY_REG(SYSTEM_CORE0->RST_CTRL, SYSTEM_CORE0_RST_CTRL_CORE0T1_RST_EN, 0x82F3U);
}

/**
 * @brief  Disable CORE0 Reset CORE1
 * @rmtoll RST_CTRL         CORE0T1_RST_EN         LL_SYSTEM_CORE0_DisableCore0ResetCore1
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_DisableCore0ResetCore1(void)
{
    MODIFY_REG(SYSTEM_CORE0->RST_CTRL, SYSTEM_CORE0_RST_CTRL_CORE0T1_RST_EN, 0x0000U);
}

/**
 * @brief  Check if CORE0 Reset CORE1 has been enabled or not
 * @rmtoll RST_CTRL         CORE0T1_RST_EN         LL_SYSTEM_CORE0_IsEnabledCore0ResetCore1
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsEnabledCore0ResetCore1(void)
{
    return (READ_BIT(SYSTEM_CORE0->RST_CTRL, SYSTEM_CORE0_RST_CTRL_CORE0T1_RST_EN) == (0x82F3U));
}

/**
 * @brief  Configure CORE1 WDT and Soft Reset Target.
 * @rmtoll RST_CTRL      CORE1_RST_SEL        LL_SYSTEM_CORE0_SetCore1ResetTarget
 * @param ResetTarget This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_RSTTARGET_ONLY_CORE1
 *         @arg @ref LL_SYSTEM_CORE0_RSTTARGET_CORE0_CORE1
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_SetCore1ResetTarget(uint32_t ResetTarget)
{
    MODIFY_REG(SYSTEM_CORE0->RST_CTRL, SYSTEM_CORE0_RST_CTRL_CORE1_RST_SEL, ResetTarget);
}

/**
 * @brief  Get the CORE1 WDT and Soft Reset Target.
 * @rmtoll RST_CTRL         CORE1_RST_SEL        LL_SYSTEM_CORE0_GetCore1ResetTarget
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_RSTTARGET_ONLY_CORE1
 *         @arg @ref LL_SYSTEM_CORE0_RSTTARGET_CORE0_CORE1
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_GetCore1ResetTarget(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE0->RST_CTRL, SYSTEM_CORE0_RST_CTRL_CORE1_RST_SEL));
}

/**
 * @brief  Force CORE1 Stop ON Reset.
 * @rmtoll RST_STOP_CTRL      CORE1_STOP        LL_SYSTEM_CORE0_ForceResetCore1
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ForceResetCore1(void)
{
    MODIFY_REG(SYSTEM_CORE0->RST_STOP_CTRL, SYSTEM_CORE0_RST_STOP_CTRL_CORE1_STOP, 0x87A3U);
}

/**
 * @brief  Release CORE1 Stop ON reset.
 * @rmtoll RST_STOP_CTRL         CORE1_STOP        LL_SYSTEM_CORE0_ReleaseResetCore1
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ReleaseResetCore1(void)
{
    MODIFY_REG(SYSTEM_CORE0->RST_STOP_CTRL, SYSTEM_CORE0_RST_STOP_CTRL_CORE1_STOP, 0x0000U);
}

/**
 * @brief  Enable CORE0 Update Reset.
 * @rmtoll UPDATE_RST_CTRL          CORE0_RST_EN       LL_SYSTEM_CORE0_EnableUpdateReset
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_EnableUpdateReset(void)
{
    WRITE_REG(SYSTEM_CORE0->UPDATE_RST_CTRL, SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_EN | 0x000035E2U);
}

/**
 * @brief  Disable CORE0 Update Reset.
 * @rmtoll UPDATE_RST_CTRL          CORE0_RST_EN       LL_SYSTEM_CORE0_DisableUpdateReset
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_DisableUpdateReset(void)
{
    WRITE_REG(SYSTEM_CORE0->UPDATE_RST_CTRL, (~SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_EN) & SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_EN);
}

/**
 * @brief  Check if CORE0 Update Reset are enabled or disabled.
 * @rmtoll UPDATE_RST_CTRL          CORE0_RST_EN       LL_SYSTEM_CORE0_IsEnabledUpdateReset
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsEnabledUpdateReset(void)
{
    return (READ_BIT(SYSTEM_CORE0->UPDATE_RST_CTRL, SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_EN) == (0x35E2U));
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EF_PMOD0 PMOD0
 * @{
 */

/**
 * @brief  Enable PMOD0 peripherals clock.
 * @rmtoll PMOD0_CKE      WDT_CKE           LL_SYSTEM_CORE0_PMOD0_EnableClock\n
 *         PMOD0_CKE      MAILBOX_CKE       LL_SYSTEM_CORE0_PMOD0_EnableClock\n
 *         PMOD0_CKE      AOS_CKE           LL_SYSTEM_CORE0_PMOD0_EnableClock\n
 *         PMOD0_CKE      SCU_CKE           LL_SYSTEM_CORE0_PMOD0_EnableClock\n
 *         PMOD0_CKE      GIC_CKE           LL_SYSTEM_CORE0_PMOD0_EnableClock\n
 *         PMOD0_CKE      GDMA00_CKE        LL_SYSTEM_CORE0_PMOD0_EnableClock\n
 *         PMOD0_CKE      GDMA01_CKE        LL_SYSTEM_CORE0_PMOD0_EnableClock\n
 *         PMOD0_CKE      UART38K_CKE       LL_SYSTEM_CORE0_PMOD0_EnableClock\n
 *         PMOD0_CKE      GPIO_CKE          LL_SYSTEM_CORE0_PMOD0_EnableClock\n
 *         PMOD0_CKE      LCSS_CKE          LL_SYSTEM_CORE0_PMOD0_EnableClock\n
 *         PMOD0_CKE      CRC1_CKE          LL_SYSTEM_CORE0_PMOD0_EnableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_WDT
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_MAILBOX
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_AOS
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_SCU
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GIC
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GDMA00
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GDMA01
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_UART38K
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GPIO
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_LCSS
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_CRC1
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_PMOD0_EnableClock(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE0->PMOD0_CKE, Periphs);
}

/**
 * @brief  Disable PMOD0 peripherals clock.
 * @rmtoll PMOD0_CKE      WDT_CKE           LL_SYSTEM_CORE0_PMOD0_DisableClock\n
 *         PMOD0_CKE      MAILBOX_CKE       LL_SYSTEM_CORE0_PMOD0_DisableClock\n
 *         PMOD0_CKE      AOS_CKE           LL_SYSTEM_CORE0_PMOD0_DisableClock\n
 *         PMOD0_CKE      SCU_CKE           LL_SYSTEM_CORE0_PMOD0_DisableClock\n
 *         PMOD0_CKE      GIC_CKE           LL_SYSTEM_CORE0_PMOD0_DisableClock\n
 *         PMOD0_CKE      GDMA00_CKE        LL_SYSTEM_CORE0_PMOD0_DisableClock\n
 *         PMOD0_CKE      GDMA01_CKE        LL_SYSTEM_CORE0_PMOD0_DisableClock\n
 *         PMOD0_CKE      UART38K_CKE       LL_SYSTEM_CORE0_PMOD0_DisableClock\n
 *         PMOD0_CKE      GPIO_CKE          LL_SYSTEM_CORE0_PMOD0_DisableClock\n
 *         PMOD0_CKE      LCSS_CKE          LL_SYSTEM_CORE0_PMOD0_DisableClock\n
 *         PMOD0_CKE      CRC1_CKE          LL_SYSTEM_CORE0_PMOD0_DisableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_WDT
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_MAILBOX
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_AOS
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_SCU
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GIC
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GDMA00
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GDMA01
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_UART38K
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GPIO
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_LCSS
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_CRC1
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_PMOD0_DisableClock(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE0->PMOD0_CKE, Periphs);
}

/**
 * @brief  Check if PMOD0 peripheral clock is enabled or not
 * @rmtoll PMOD0_CKE      WDT_CKE           LL_SYSTEM_CORE0_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      MAILBOX_CKE       LL_SYSTEM_CORE0_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      AOS_CKE           LL_SYSTEM_CORE0_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      SCU_CKE           LL_SYSTEM_CORE0_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      GIC_CKE           LL_SYSTEM_CORE0_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      GDMA00_CKE        LL_SYSTEM_CORE0_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      GDMA01_CKE        LL_SYSTEM_CORE0_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      UART38K_CKE       LL_SYSTEM_CORE0_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      GPIO_CKE          LL_SYSTEM_CORE0_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      LCSS_CKE          LL_SYSTEM_CORE0_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      CRC1_CKE          LL_SYSTEM_CORE0_PMOD0_IsEnabledClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_WDT
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_MAILBOX
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_AOS
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_SCU
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GIC
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GDMA00
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GDMA01
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_UART38K
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GPIO
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_LCSS
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_CRC1
 * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_PMOD0_IsEnabledClock(uint32_t Periphs)
{
    return (READ_BIT(SYSTEM_CORE0->PMOD0_CKE, Periphs) == Periphs);
}

/**
 * @brief  Force PMOD0 peripherals reset.
 * @note   For all module-level resets, first disable the module clock,
 *         then force the reset, then release the module reset,
 *         finally enable the module clock for normal operation.
 * @rmtoll PMOD0_RST     MAILBOX_RST_REQ       LL_SYSTEM_CORE0_PMOD0_ForceReset\n
 *         PMOD0_RST     AOS_RST_REQ           LL_SYSTEM_CORE0_PMOD0_ForceReset\n
 *         PMOD0_RST     GDMA00_RST_REQ        LL_SYSTEM_CORE0_PMOD0_ForceReset\n
 *         PMOD0_RST     GDMA01_RST_REQ        LL_SYSTEM_CORE0_PMOD0_ForceReset\n
 *         PMOD0_RST     GPIO_RST_REQ          LL_SYSTEM_CORE0_PMOD0_ForceReset\n
 *         PMOD0_RST     CRC1_RST_REQ          LL_SYSTEM_CORE0_PMOD0_ForceReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_MAILBOX
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_AOS
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GDMA00
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GDMA01
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GPIO
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_CRC1
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_PMOD0_ForceReset(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE0->PMOD0_RST, Periphs);
}

/**
 * @brief  Release PMOD0 peripherals reset.
 * @rmtoll PMOD0_RST     MAILBOX_RST_REQ       LL_SYSTEM_CORE0_PMOD0_ReleaseReset\n
 *         PMOD0_RST     AOS_RST_REQ           LL_SYSTEM_CORE0_PMOD0_ReleaseReset\n
 *         PMOD0_RST     GDMA00_RST_REQ        LL_SYSTEM_CORE0_PMOD0_ReleaseReset\n
 *         PMOD0_RST     GDMA01_RST_REQ        LL_SYSTEM_CORE0_PMOD0_ReleaseReset\n
 *         PMOD0_RST     GPIO_RST_REQ          LL_SYSTEM_CORE0_PMOD0_ReleaseReset\n
 *         PMOD0_RST     CRC1_RST_REQ          LL_SYSTEM_CORE0_PMOD0_ReleaseReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_MAILBOX
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_AOS
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GDMA00
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GDMA01
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GPIO
 *         @arg @ref LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_CRC1
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_PMOD0_ReleaseReset(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE0->PMOD0_RST, Periphs);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EF_KBI_CKE KBI_CKE
 * @{
 */

/**
 * @brief  Enable KBI0 APB module clock.
 * @rmtoll KBI0_CKE      KBIC0_CKE           LL_SYSTEM_CORE0_KBI0_EnableClock
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_KBI0_EnableClock(void)
{
    SET_BIT(SYSTEM_CORE0->KBI0_CKE, SYSTEM_CORE0_KBI0_CKE_KBIC0);
}

/**
 * @brief  Disable KBI0 APB module clock.
 * @rmtoll KBI0_CKE      KBIC0_CKE           LL_SYSTEM_CORE0_KBI0_DisableClock\
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_KBI0_DisableClock(void)
{
    CLEAR_BIT(SYSTEM_CORE0->KBI0_CKE, SYSTEM_CORE0_KBI0_CKE_KBIC0);
}

/**
 * @brief  Check if KBI0 APB module clock is enabled or not
 * @rmtoll KBI0_CKE      KBIC0_CKE           LL_SYSTEM_CORE0_KBI0_IsEnabledClock
 * @retval State of KBI0 Periph (1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_KBI0_IsEnabledClock(void)
{
    return (READ_BIT(SYSTEM_CORE0->KBI0_CKE, SYSTEM_CORE0_KBI0_CKE_KBIC0) == SYSTEM_CORE0_KBI0_CKE_KBIC0);
}

/**
 * @brief  Enable KBI0 channels clock.
 * @rmtoll KBI0_CKE      KBI00_CKE           LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI01_CKE           LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI02_CKE           LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI03_CKE           LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI04_CKE           LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI05_CKE           LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI06_CKE           LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI07_CKE           LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI08_CKE           LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI09_CKE           LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI010_CKE          LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI011_CKE          LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI012_CKE          LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI013_CKE          LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI014_CKE          LL_SYSTEM_CORE0_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI015_CKE          LL_SYSTEM_CORE0_KBI0_EnableChannelClock
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_ALL
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_15
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_KBI0_EnableChannelClock(uint32_t Channels)
{
    SET_BIT(SYSTEM_CORE0->KBI0_CKE, Channels);
}

/**
 * @brief  Disable KBI0 channels clock.
 * @rmtoll KBI0_CKE      KBI00_CKE           LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI01_CKE           LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI02_CKE           LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI03_CKE           LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI04_CKE           LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI05_CKE           LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI06_CKE           LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI07_CKE           LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI08_CKE           LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI09_CKE           LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI010_CKE          LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI011_CKE          LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI012_CKE          LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI013_CKE          LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI014_CKE          LL_SYSTEM_CORE0_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI015_CKE          LL_SYSTEM_CORE0_KBI0_DisableChannelClock
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_ALL
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_15
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_KBI0_DisableChannelClock(uint32_t Channels)
{
    CLEAR_BIT(SYSTEM_CORE0->KBI0_CKE, Channels);
}

/**
 * @brief  Check if KBI0 channels clock is enabled or not
 * @rmtoll KBI0_CKE      KBI00_CKE           LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI01_CKE           LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI02_CKE           LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI03_CKE           LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI04_CKE           LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI05_CKE           LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI06_CKE           LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI07_CKE           LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI08_CKE           LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI09_CKE           LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI010_CKE          LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI011_CKE          LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI012_CKE          LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI013_CKE          LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI014_CKE          LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI015_CKE          LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_ALL
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE0_KBI0_CHANNEL_15
 * @retval State of KBI0 Channels(1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_KBI0_IsEnabledChannelClock(uint32_t Channels)
{
    return (READ_BIT(SYSTEM_CORE0->KBI0_CKE, Channels) == Channels);
}

/**
 * @brief  Enable KBI1 APB module clock.
 * @rmtoll KBI1_CKE      KBIC0_CKE           LL_SYSTEM_CORE0_KBI1_EnableClock
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_KBI1_EnableClock(void)
{
    SET_BIT(SYSTEM_CORE0->KBI1_CKE, SYSTEM_CORE0_KBI1_CKE_KBIC1);
}

/**
 * @brief  Disable KBI1 APB module clock.
 * @rmtoll KBI1_CKE      KBIC0_CKE           LL_SYSTEM_CORE0_KBI1_DisableClock\
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_KBI1_DisableClock(void)
{
    CLEAR_BIT(SYSTEM_CORE0->KBI1_CKE, SYSTEM_CORE0_KBI1_CKE_KBIC1);
}

/**
 * @brief  Check if KBI1 APB module clock is enabled or not
 * @rmtoll KBI1_CKE      KBIC0_CKE           LL_SYSTEM_CORE0_KBI1_IsEnabledClock
 * @retval State of KBI1 Periph (1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_KBI1_IsEnabledClock(void)
{
    return (READ_BIT(SYSTEM_CORE0->KBI1_CKE, SYSTEM_CORE0_KBI1_CKE_KBIC1) == SYSTEM_CORE0_KBI1_CKE_KBIC1);
}

/**
 * @brief  Enable KBI1 channels clock.
 * @rmtoll KBI1_CKE      KBI10_CKE           LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI11_CKE           LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI12_CKE           LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI13_CKE           LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI14_CKE           LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI15_CKE           LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI16_CKE           LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI17_CKE           LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI18_CKE           LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI19_CKE           LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI110_CKE          LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI111_CKE          LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI112_CKE          LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI113_CKE          LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI114_CKE          LL_SYSTEM_CORE0_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI115_CKE          LL_SYSTEM_CORE0_KBI1_EnableChannelClock
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_ALL
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_15
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_KBI1_EnableChannelClock(uint32_t Channels)
{
    SET_BIT(SYSTEM_CORE0->KBI1_CKE, Channels);
}

/**
 * @brief  Disable KBI1 channels clock.
 * @rmtoll KBI1_CKE      KBI10_CKE           LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI11_CKE           LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI12_CKE           LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI13_CKE           LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI14_CKE           LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI15_CKE           LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI16_CKE           LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI17_CKE           LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI18_CKE           LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI19_CKE           LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI110_CKE          LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI111_CKE          LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI112_CKE          LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI113_CKE          LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI114_CKE          LL_SYSTEM_CORE0_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI115_CKE          LL_SYSTEM_CORE0_KBI1_DisableChannelClock
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_ALL
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_15
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_KBI1_DisableChannelClock(uint32_t Channels)
{
    CLEAR_BIT(SYSTEM_CORE0->KBI1_CKE, Channels);
}

/**
 * @brief  Check if KBI1 channels clock is enabled or not
 * @rmtoll KBI1_CKE      KBI10_CKE           LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI11_CKE           LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI12_CKE           LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI13_CKE           LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI14_CKE           LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI15_CKE           LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI16_CKE           LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI17_CKE           LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI18_CKE           LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI19_CKE           LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI110_CKE          LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI111_CKE          LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI112_CKE          LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI113_CKE          LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI114_CKE          LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI115_CKE          LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_ALL
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE0_KBI1_CHANNEL_15
 * @retval State of KBI1 Channels(1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_KBI1_IsEnabledChannelClock(uint32_t Channels)
{
    return (READ_BIT(SYSTEM_CORE0->KBI1_CKE, Channels) == Channels);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EF_MOD0 MOD0
 * @{
 */

/**
 * @brief  Enable MOD0 peripherals clock.
 * @rmtoll MOD0_CKE      UART0_CKE           LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      UART1_CKE           LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      UART2_CKE           LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      UART3_CKE           LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      UART4_CKE           LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      UART5_CKE           LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      UART6_CKE           LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      UART7_CKE           LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      LPUART_CKE          LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      USART0_CKE          LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      USART1_CKE          LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      ISO78160_CKE        LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      ISO78161_CKE        LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      SPI0_CKE            LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      SPI1_CKE            LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      SPI2_CKE            LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      SPI3_CKE            LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      I2C0_CKE            LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      I2C1_CKE            LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      CAN_CKE             LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      IOCNT_CKE           LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      LCD_CKE             LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      DSP_CKE             LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      M2M_CKE             LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      D2F_CKE             LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      ADC0_CKE            LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      DAC_CKE             LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      TC0_CKE             LL_SYSTEM_CORE0_MOD0_EnableClock\n
 *         MOD0_CKE      TC1_CKE             LL_SYSTEM_CORE0_MOD0_EnableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART2
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART3
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART4
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART5
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART6
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART7
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_LPUART
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_USART0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_USART1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78160
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78161
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI2
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI3
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_I2C0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_I2C1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_CAN
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_IOCNT
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_LCD
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_DSP
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_M2M
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_D2F
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ADC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_DAC
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_TC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_TC1
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_MOD0_EnableClock(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE0->MOD0_CKE, Periphs);
}

/**
 * @brief  Disable MOD0 peripherals clock.
 * @rmtoll MOD0_CKE      UART0_CKE           LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      UART1_CKE           LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      UART2_CKE           LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      UART3_CKE           LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      UART4_CKE           LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      UART5_CKE           LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      UART6_CKE           LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      UART7_CKE           LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      LPUART_CKE          LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      USART0_CKE          LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      USART1_CKE          LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      ISO78160_CKE        LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      ISO78161_CKE        LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      SPI0_CKE            LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      SPI1_CKE            LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      SPI2_CKE            LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      SPI3_CKE            LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      I2C0_CKE            LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      I2C1_CKE            LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      CAN_CKE             LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      IOCNT_CKE           LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      LCD_CKE             LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      DSP_CKE             LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      M2M_CKE             LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      D2F_CKE             LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      ADC0_CKE            LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      DAC_CKE             LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      TC0_CKE             LL_SYSTEM_CORE0_MOD0_DisableClock\n
 *         MOD0_CKE      TC1_CKE             LL_SYSTEM_CORE0_MOD0_DisableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART2
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART3
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART4
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART5
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART6
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART7
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_LPUART
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_USART0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_USART1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78160
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78161
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI2
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI3
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_I2C0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_I2C1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_CAN
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_IOCNT
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_LCD
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_DSP
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_M2M
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_D2F
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ADC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_DAC
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_TC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_TC1
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_MOD0_DisableClock(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE0->MOD0_CKE, Periphs);
}

/**
 * @brief  Check if MOD0 peripheral clock is enabled or not
 * @rmtoll MOD0_CKE      UART0_CKE           LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      UART1_CKE           LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      UART2_CKE           LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      UART3_CKE           LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      UART4_CKE           LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      UART5_CKE           LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      UART6_CKE           LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      UART7_CKE           LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      LPUART_CKE          LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      USART0_CKE          LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      USART1_CKE          LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      ISO78160_CKE        LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      ISO78161_CKE        LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      SPI0_CKE            LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      SPI1_CKE            LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      SPI2_CKE            LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      SPI3_CKE            LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      I2C0_CKE            LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      I2C1_CKE            LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      CAN_CKE             LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      IOCNT_CKE           LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      LCD_CKE             LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      DSP_CKE             LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      M2M_CKE             LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      D2F_CKE             LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      ADC0_CKE            LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      DAC_CKE             LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      TC0_CKE             LL_SYSTEM_CORE0_MOD0_IsEnabledClock\n
 *         MOD0_CKE      TC1_CKE             LL_SYSTEM_CORE0_MOD0_IsEnabledClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART2
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART3
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART4
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART5
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART6
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART7
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_LPUART
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_USART0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_USART1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78160
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78161
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI2
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI3
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_I2C0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_I2C1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_CAN
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_IOCNT
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_LCD
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_DSP
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_M2M
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_D2F
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ADC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_DAC
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_TC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_TC1
 * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_MOD0_IsEnabledClock(uint32_t Periphs)
{
    return (READ_BIT(SYSTEM_CORE0->MOD0_CKE, Periphs) == Periphs);
}

/**
 * @brief  Force MOD0 peripherals reset.
 * @note   For all module-level resets, first disable the module clock,
 *         then force the reset, then release the module reset,
 *         finally enable the module clock for normal operation.
 * @rmtoll MOD0_RST      UART0_RST           LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      UART1_RST           LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      UART2_RST           LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      UART3_RST           LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      UART4_RST           LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      UART5_RST           LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      UART6_RST           LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      UART7_RST           LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      LPUART_RST          LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      USART0_RST          LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      USART1_RST          LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      ISO78160_RST        LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      ISO78161_RST        LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      SPI0_RST            LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      SPI1_RST            LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      SPI2_RST            LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      SPI3_RST            LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      I2C0_RST            LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      I2C1_RST            LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      CAN_RST             LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      IOCNT_RST           LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      LCD_RST             LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      DSP_RST             LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      M2M_RST             LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      D2F_RST             LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      ADC0_RST            LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      DAC_RST             LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      TC0_RST             LL_SYSTEM_CORE0_MOD0_ForceReset\n
 *         MOD0_RST      TC1_RST             LL_SYSTEM_CORE0_MOD0_ForceReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART2
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART3
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART4
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART5
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART6
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART7
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_LPUART
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_USART0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_USART1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78160
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78161
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI2
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI3
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_I2C0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_I2C1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_CAN
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_IOCNT
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_LCD
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_DSP
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_M2M
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_D2F
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ADC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_DAC
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_TC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_TC1
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_MOD0_ForceReset(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE0->MOD0_RST, Periphs);
}

/**
 * @brief  Release MOD0 peripherals reset.
 * @rmtoll MOD0_RST      UART0_RST           LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      UART1_RST           LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      UART2_RST           LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      UART3_RST           LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      UART4_RST           LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      UART5_RST           LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      UART6_RST           LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      UART7_RST           LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      LPUART_RST          LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      USART0_RST          LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      USART1_RST          LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      ISO78160_RST        LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      ISO78161_RST        LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      SPI0_RST            LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      SPI1_RST            LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      SPI2_RST            LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      SPI3_RST            LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      I2C0_RST            LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      I2C1_RST            LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      CAN_RST             LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      IOCNT_RST           LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      LCD_RST             LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      DSP_RST             LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      M2M_RST             LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      D2F_RST             LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      ADC0_RST            LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      DAC_RST             LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      TC0_RST             LL_SYSTEM_CORE0_MOD0_ReleaseReset\n
 *         MOD0_RST      TC1_RST             LL_SYSTEM_CORE0_MOD0_ReleaseReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART2
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART3
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART4
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART5
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART6
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_UART7
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_LPUART
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_USART0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_USART1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78160
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78161
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI2
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_SPI3
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_I2C0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_I2C1
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_CAN
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_IOCNT
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_LCD
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_DSP
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_M2M
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_D2F
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_ADC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_DAC
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_TC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD0_PERIPH_TC1
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_MOD0_ReleaseReset(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE0->MOD0_RST, Periphs);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EF_MOD1 MOD1
 * @{
 */

/**
 * @brief  Enable MOD1 peripherals clock.
 * @rmtoll MOD1_CKE      SIMPTC0_CKE            LL_SYSTEM_CORE0_MOD1_EnableClock\n
 *         MOD1_CKE      SIMPTC1_CKE            LL_SYSTEM_CORE0_MOD1_EnableClock\n
 *         MOD1_CKE      SIMPTC2_CKE            LL_SYSTEM_CORE0_MOD1_EnableClock\n
 *         MOD1_CKE      SIMPTC3_CKE            LL_SYSTEM_CORE0_MOD1_EnableClock\n
 *         MOD1_CKE      RS_VDD_CKE             LL_SYSTEM_CORE0_MOD1_EnableClock\n
 *         MOD1_CKE      LSRX_CKE               LL_SYSTEM_CORE0_MOD1_EnableClock\n
 *         MOD1_CKE      LVDCMP_CKE             LL_SYSTEM_CORE0_MOD1_EnableClock\n
 *         MOD1_CKE      CRC0_CKE               LL_SYSTEM_CORE0_MOD1_EnableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC1
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC2
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC3
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_RS_VDD
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_LSRX
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_LVDCMP
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_CRC0
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_MOD1_EnableClock(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE0->MOD1_CKE, Periphs);
}

/**
 * @brief  Disable MOD1 peripherals clock.
 * @rmtoll MOD1_CKE      SIMPTC0_CKE            LL_SYSTEM_CORE0_MOD1_DisableClock\n
 *         MOD1_CKE      SIMPTC1_CKE            LL_SYSTEM_CORE0_MOD1_DisableClock\n
 *         MOD1_CKE      SIMPTC2_CKE            LL_SYSTEM_CORE0_MOD1_DisableClock\n
 *         MOD1_CKE      SIMPTC3_CKE            LL_SYSTEM_CORE0_MOD1_DisableClock\n
 *         MOD1_CKE      RS_VDD_CKE             LL_SYSTEM_CORE0_MOD1_DisableClock\n
 *         MOD1_CKE      LSRX_CKE               LL_SYSTEM_CORE0_MOD1_DisableClock\n
 *         MOD1_CKE      LVDCMP_CKE             LL_SYSTEM_CORE0_MOD1_DisableClock\n
 *         MOD1_CKE      CRC0_CKE               LL_SYSTEM_CORE0_MOD1_DisableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC1
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC2
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC3
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_RS_VDD
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_LSRX
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_LVDCMP
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_CRC0
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_MOD1_DisableClock(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE0->MOD1_CKE, Periphs);
}

/**
 * @brief  Check if MOD1 peripheral clock is enabled or not
 * @rmtoll MOD1_CKE      SIMPTC0_CKE            LL_SYSTEM_CORE0_MOD1_IsEnabledClock\n
 *         MOD1_CKE      SIMPTC1_CKE            LL_SYSTEM_CORE0_MOD1_IsEnabledClock\n
 *         MOD1_CKE      SIMPTC2_CKE            LL_SYSTEM_CORE0_MOD1_IsEnabledClock\n
 *         MOD1_CKE      SIMPTC3_CKE            LL_SYSTEM_CORE0_MOD1_IsEnabledClock\n
 *         MOD1_CKE      RS_VDD_CKE             LL_SYSTEM_CORE0_MOD1_IsEnabledClock\n
 *         MOD1_CKE      LSRX_CKE               LL_SYSTEM_CORE0_MOD1_IsEnabledClock\n
 *         MOD1_CKE      LVDCMP_CKE             LL_SYSTEM_CORE0_MOD1_IsEnabledClock\n
 *         MOD1_CKE      CRC0_CKE               LL_SYSTEM_CORE0_MOD1_IsEnabledClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC1
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC2
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC3
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_RS_VDD
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_LSRX
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_LVDCMP
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_CRC0
 * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_MOD1_IsEnabledClock(uint32_t Periphs)
{
    return (READ_BIT(SYSTEM_CORE0->MOD1_CKE, Periphs) == Periphs);
}

/**
 * @brief  Force MOD1 peripherals reset.
 * @note   For all module-level resets, first disable the module clock,
 *         then force the reset, then release the module reset,
 *         finally enable the module clock for normal operation.
 * @rmtoll MOD1_RST      SIMPTC0_RST            LL_SYSTEM_CORE0_MOD1_ForceReset\n
 *         MOD1_RST      SIMPTC1_RST            LL_SYSTEM_CORE0_MOD1_ForceReset\n
 *         MOD1_RST      SIMPTC2_RST            LL_SYSTEM_CORE0_MOD1_ForceReset\n
 *         MOD1_RST      SIMPTC3_RST            LL_SYSTEM_CORE0_MOD1_ForceReset\n
 *         MOD1_RST      RS_VDD_RST             LL_SYSTEM_CORE0_MOD1_ForceReset\n
 *         MOD1_RST      LSRX_RST               LL_SYSTEM_CORE0_MOD1_ForceReset\n
 *         MOD1_RST      KBI0_RST               LL_SYSTEM_CORE0_MOD1_ForceReset\n
 *         MOD1_RST      KBI1_RST               LL_SYSTEM_CORE0_MOD1_ForceReset\n
 *         MOD1_RST      LVDCMP_RST             LL_SYSTEM_CORE0_MOD1_ForceReset\n
 *         MOD1_RST      CRC0_RST               LL_SYSTEM_CORE0_MOD1_ForceReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC1
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC2
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC3
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_RS_VDD
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_LSRX
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_KBI0
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_KBI1
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_LVDCMP
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_CRC0
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_MOD1_ForceReset(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE0->MOD1_RST, Periphs);
}

/**
 * @brief  Release MOD1 peripherals reset.
 * @rmtoll MOD1_RST      SIMPTC0_RST            LL_SYSTEM_CORE0_MOD1_ReleaseReset\n
 *         MOD1_RST      SIMPTC1_RST            LL_SYSTEM_CORE0_MOD1_ReleaseReset\n
 *         MOD1_RST      SIMPTC2_RST            LL_SYSTEM_CORE0_MOD1_ReleaseReset\n
 *         MOD1_RST      SIMPTC3_RST            LL_SYSTEM_CORE0_MOD1_ReleaseReset\n
 *         MOD1_RST      RS_VDD_RST             LL_SYSTEM_CORE0_MOD1_ReleaseReset\n
 *         MOD1_RST      LSRX_RST               LL_SYSTEM_CORE0_MOD1_ReleaseReset\n
 *         MOD1_RST      KBI0_RST               LL_SYSTEM_CORE0_MOD1_ReleaseReset\n
 *         MOD1_RST      KBI1_RST               LL_SYSTEM_CORE0_MOD1_ReleaseReset\n
 *         MOD1_RST      LVDCMP_RST             LL_SYSTEM_CORE0_MOD1_ReleaseReset\n
 *         MOD1_RST      CRC0_RST               LL_SYSTEM_CORE0_MOD1_ReleaseReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC0
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC1
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC2
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC3
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_RS_VDD
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_LSRX
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_KBI0
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_KBI1
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_LVDCMP
 *         @arg @ref LL_SYSTEM_CORE0_MOD1_PERIPH_CRC0
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_MOD1_ReleaseReset(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE0->MOD1_RST, Periphs);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EF_MOD2 MOD2
 * @{
 */

/**
 * @brief  Enable MOD2 peripherals clock.
 * @rmtoll MOD2_CKE      QSPI0_CKE           LL_SYSTEM_CORE0_MOD2_EnableClock\n
 *         MOD2_CKE      QSPI1_CKE           LL_SYSTEM_CORE0_MOD2_EnableClock\n
 *         MOD2_CKE      SEA_BUS_CKE         LL_SYSTEM_CORE0_MOD2_EnableClock\n
 *         MOD2_CKE      SEA_PKE_CKE         LL_SYSTEM_CORE0_MOD2_EnableClock\n
 *         MOD2_CKE      SEA_AES_CKE         LL_SYSTEM_CORE0_MOD2_EnableClock\n
 *         MOD2_CKE      SEA_TRNG_CKE        LL_SYSTEM_CORE0_MOD2_EnableClock\n
 *         MOD2_CKE      SEA_SHA_CKE         LL_SYSTEM_CORE0_MOD2_EnableClock\n
 *         MOD2_CKE      KEY_STORE_CKE       LL_SYSTEM_CORE0_MOD2_EnableClock\n
 *         MOD2_CKE      AES_CKE             LL_SYSTEM_CORE0_MOD2_EnableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI0
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI1
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_BUS
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_PKE
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_AES
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_TRNG
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_SHA
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_KEY_STORE
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_AES
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_MOD2_EnableClock(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE0->MOD2_CKE, Periphs);
}

/**
 * @brief  Disable MOD2 peripherals clock.
 * @rmtoll MOD2_CKE      QSPI0_CKE           LL_SYSTEM_CORE0_MOD2_DisableClock\n
 *         MOD2_CKE      QSPI1_CKE           LL_SYSTEM_CORE0_MOD2_DisableClock\n
 *         MOD2_CKE      SEA_BUS_CKE         LL_SYSTEM_CORE0_MOD2_DisableClock\n
 *         MOD2_CKE      SEA_PKE_CKE         LL_SYSTEM_CORE0_MOD2_DisableClock\n
 *         MOD2_CKE      SEA_AES_CKE         LL_SYSTEM_CORE0_MOD2_DisableClock\n
 *         MOD2_CKE      SEA_TRNG_CKE        LL_SYSTEM_CORE0_MOD2_DisableClock\n
 *         MOD2_CKE      SEA_SHA_CKE         LL_SYSTEM_CORE0_MOD2_DisableClock\n
 *         MOD2_CKE      KEY_STORE_CKE       LL_SYSTEM_CORE0_MOD2_DisableClock\n
 *         MOD2_CKE      AES_CKE             LL_SYSTEM_CORE0_MOD2_DisableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI0
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI1
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_BUS
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_PKE
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_AES
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_TRNG
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_SHA
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_KEY_STORE
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_AES
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_MOD2_DisableClock(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE0->MOD2_CKE, Periphs);
}

/**
 * @brief  Check if MOD2 peripheral clock is enabled or not
 * @rmtoll MOD2_CKE      QSPI0_CKE           LL_SYSTEM_CORE0_MOD2_IsEnabledClock\n
 *         MOD2_CKE      QSPI1_CKE           LL_SYSTEM_CORE0_MOD2_IsEnabledClock\n
 *         MOD2_CKE      SEA_BUS_CKE         LL_SYSTEM_CORE0_MOD2_IsEnabledClock\n
 *         MOD2_CKE      SEA_PKE_CKE         LL_SYSTEM_CORE0_MOD2_IsEnabledClock\n
 *         MOD2_CKE      SEA_AES_CKE         LL_SYSTEM_CORE0_MOD2_IsEnabledClock\n
 *         MOD2_CKE      SEA_TRNG_CKE        LL_SYSTEM_CORE0_MOD2_IsEnabledClock\n
 *         MOD2_CKE      SEA_SHA_CKE         LL_SYSTEM_CORE0_MOD2_IsEnabledClock\n
 *         MOD2_CKE      KEY_STORE_CKE       LL_SYSTEM_CORE0_MOD2_IsEnabledClock\n
 *         MOD2_CKE      AES_CKE             LL_SYSTEM_CORE0_MOD2_IsEnabledClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI0
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI1
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_BUS
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_PKE
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_AES
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_TRNG
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_SHA
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_KEY_STORE
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_AES
 * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_MOD2_IsEnabledClock(uint32_t Periphs)
{
    return (READ_BIT(SYSTEM_CORE0->MOD2_CKE, Periphs) == Periphs);
}

/**
 * @brief  Force MOD2 peripherals reset.
 * @note   For all module-level resets, first disable the module clock,
 *         then force the reset, then release the module reset,
 *         finally enable the module clock for normal operation.
 * @rmtoll MOD2_RST      QSPI0_RST           LL_SYSTEM_CORE0_MOD2_ForceReset\n
 *         MOD2_RST      QSPI1_RST           LL_SYSTEM_CORE0_MOD2_ForceReset\n
 *         MOD2_RST      SEA_BUS_RST         LL_SYSTEM_CORE0_MOD2_ForceReset\n
 *         MOD2_RST      AES_RST             LL_SYSTEM_CORE0_MOD2_ForceReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI0
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI1
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_BUS
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_AES
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_MOD2_ForceReset(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE0->MOD2_RST, Periphs);
}

/**
 * @brief  Release MOD2 peripherals reset.
 * @rmtoll MOD2_RST      QSPI0_RST           LL_SYSTEM_CORE0_MOD2_ReleaseReset\n
 *         MOD2_RST      QSPI1_RST           LL_SYSTEM_CORE0_MOD2_ReleaseReset\n
 *         MOD2_RST      SEA_BUS_RST         LL_SYSTEM_CORE0_MOD2_ReleaseReset\n
 *         MOD2_RST      AES_RST             LL_SYSTEM_CORE0_MOD2_ReleaseReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI0
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI1
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_BUS
 *         @arg @ref LL_SYSTEM_CORE0_MOD2_PERIPH_AES
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE0_MOD2_ReleaseReset(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE0->MOD2_RST, Periphs);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EF_IT_MANAGEMENT IT_Management
 * @{
 */

/**
 * @brief  Enable PLVD5 Interrupt.
 * @rmtoll PLVD5_CTRL          PLVD5_IE       LL_SYSTEM_CORE0_EnableIT_PLVD5
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_EnableIT_PLVD5(void)
{
    WRITE_REG(SYSTEM_CORE0->PLVD5_CTRL, SYSTEM_CORE0_PLVD5_CTRL_PLVD5_IE);
}

/**
 * @brief  Disable PLVD5 Interrupt.
 * @rmtoll PLVD5_CTRL          PLVD5_IE       LL_SYSTEM_CORE0_DisableIT_PLVD5
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_DisableIT_PLVD5(void)
{
    WRITE_REG(SYSTEM_CORE0->PLVD5_CTRL, (~SYSTEM_CORE0_PLVD5_CTRL_PLVD5_IE) & SYSTEM_CORE0_PLVD5_CTRL_PLVD5_IE);
}

/**
 * @brief  Check if PLVD5 Interrupt are enabled or disabled.
 * @rmtoll PLVD5_CTRL          PLVD5_IE       LL_SYSTEM_CORE0_IsEnabledIT_PLVD5
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsEnabledIT_PLVD5(void)
{
    return (READ_BIT(SYSTEM_CORE0->PLVD5_CTRL, SYSTEM_CORE0_PLVD5_CTRL_PLVD5_IE) == (SYSTEM_CORE0_PLVD5_CTRL_PLVD5_IE));
}

/**
 * @brief  Enable CORE02CORE1 Bridge Illegal Access Interrupt.
 * @rmtoll DIG_REV_CTRL          CORE02CORE1_BRG_IE       LL_SYSTEM_CORE0_EnableIT_BRG
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_EnableIT_BRG(void)
{
    WRITE_REG(SYSTEM_CORE0->DIG_REV_CTRL, SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_IE);
}

/**
 * @brief  Disable CORE02CORE1 Bridge Illegal Access Interrupt.
 * @rmtoll DIG_REV_CTRL          CORE02CORE1_BRG_IE       LL_SYSTEM_CORE0_DisableIT_BRG
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_DisableIT_BRG(void)
{
    WRITE_REG(SYSTEM_CORE0->DIG_REV_CTRL, (~SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_IE) & SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_IE);
}

/**
 * @brief  Check if CORE02CORE1 Bridge Illegal Access Interrupt are enabled or disabled.
 * @rmtoll DIG_REV_CTRL          CORE02CORE1_BRG_IE       LL_SYSTEM_CORE0_IsEnabledIT_BRG
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsEnabledIT_BRG(void)
{
    return (READ_BIT(SYSTEM_CORE0->DIG_REV_CTRL, SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_IE) == (SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_IE));
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EF_CALIBRATION_IT_MANAGEMENT Calibration IT_Management
 * @{
 */

/**
 * @brief  Enable Clock Calibration Complete Interrupt.
 * @rmtoll CLK_CAL_CTRL          CAL_DONE_IE       LL_SYSTEM_CORE0_EnableIT_CALDONE
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_EnableIT_CALDONE(void)
{
    SET_BIT(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_CAL_DONE_IE);
}

/**
 * @brief  Disable Clock Calibration Complete Interrupt.
 * @rmtoll CLK_CAL_CTRL          CAL_DONE_IE       LL_SYSTEM_CORE0_DisableIT_CALDONE
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_DisableIT_CALDONE(void)
{
    CLEAR_BIT(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_CAL_DONE_IE);
}

/**
 * @brief  Check if Clock Calibration Complete Interrupt are enabled or disabled.
 * @rmtoll CLK_CAL_CTRL          CAL_DONE_IE       LL_SYSTEM_CORE0_IsEnabledIT_CALDONE
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsEnabledIT_CALDONE(void)
{
    return (READ_BIT(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_CAL_DONE_IE) == (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_DONE_IE));
}

/**
 * @brief  Enable Calibrated Clock Counter Overflow Interrupt.
 * @rmtoll CLK_CAL_CTRL          CAL_DONE_IE       LL_SYSTEM_CORE0_EnableIT_CALOV
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_EnableIT_CALOV(void)
{
    SET_BIT(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_CAL_OV_IE);
}

/**
 * @brief  Disable Calibrated Clock Counter Overflow Interrupt.
 * @rmtoll CLK_CAL_CTRL          CAL_DONE_IE       LL_SYSTEM_CORE0_DisableIT_CALOV
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_DisableIT_CALOV(void)
{
    CLEAR_BIT(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_CAL_OV_IE);
}

/**
 * @brief  Check if Calibrated Clock Counter Overflow Interrupt are enabled or disabled.
 * @rmtoll CLK_CAL_CTRL          CAL_DONE_IE       LL_SYSTEM_CORE0_IsEnabledIT_CALOV
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsEnabledIT_CALOV(void)
{
    return (READ_BIT(SYSTEM_CORE0->CLK_CAL_CTRL, SYSTEM_CORE0_CLK_CAL_CTRL_CAL_OV_IE) == (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_OV_IE));
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EF_RESET_IT_MANAGEMENT Reset IT_Management
 * @{
 */

/**
 * @brief  Enable BOI_VBAT_RST Interrupt.
 * @rmtoll BOI_RST_CTRL          BOI_VBAT_RST_IE       LL_SYSTEM_CORE0_EnableIT_BOIVBATRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_EnableIT_BOIVBATRST(void)
{
    SET_BIT(SYSTEM_CORE0->BOI_RST_CTRL, SYSTEM_CORE0_BOI_RST_CTRL_BOI_VBAT_RST_IE);
}

/**
 * @brief  Disable BOI_VBAT_RST Interrupt.
 * @rmtoll BOI_RST_CTRL          BOI_VBAT_RST_IE       LL_SYSTEM_CORE0_DisableIT_BOIVBATRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_DisableIT_BOIVBATRST(void)
{
    CLEAR_BIT(SYSTEM_CORE0->BOI_RST_CTRL, SYSTEM_CORE0_BOI_RST_CTRL_BOI_VBAT_RST_IE);
}

/**
 * @brief  Check if BOI_VBAT_RST Interrupt are enabled or disabled.
 * @rmtoll BOI_RST_CTRL          BOI_VBAT_RST_IE       LL_SYSTEM_CORE0_IsEnabledIT_BOIVBATRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsEnabledIT_BOIVBATRST(void)
{
    return (READ_BIT(SYSTEM_CORE0->BOI_RST_CTRL, SYSTEM_CORE0_BOI_RST_CTRL_BOI_VBAT_RST_IE) == (SYSTEM_CORE0_BOI_RST_CTRL_BOI_VBAT_RST_IE));
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EF_CALIBRATION_FLAG_management Calibration FLAG_management
 * @{
 */

/**
 * @brief  Indicate the status of Clock Calibration Complete Flag.
 * @rmtoll CLK_CAL_STA          CAL_DONE          LL_SYSTEM_CORE0_IsActiveFlag_CALDONE
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_CALDONE(void)
{
    return (READ_BIT(SYSTEM_CORE0->CLK_CAL_STA, SYSTEM_CORE0_CLK_CAL_STA_CAL_DONE) == (SYSTEM_CORE0_CLK_CAL_STA_CAL_DONE));
}

/**
 * @brief  Clear Clock Calibration Complete Flag.
 * @rmtoll CLK_CAL_STA          CAL_DONE          LL_SYSTEM_CORE0_ClearFlag_CALDONE
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_CALDONE(void)
{
    WRITE_REG(SYSTEM_CORE0->CLK_CAL_STA, SYSTEM_CORE0_CLK_CAL_STA_CAL_DONE);
}

/**
 * @brief  Indicate the status of Calibrated Clock Counter Overflow Flag.
 * @rmtoll CLK_CAL_STA          CAL_OV          LL_SYSTEM_CORE0_IsActiveFlag_CALOV
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_CALOV(void)
{
    return (READ_BIT(SYSTEM_CORE0->CLK_CAL_STA, SYSTEM_CORE0_CLK_CAL_STA_CAL_OV) == (SYSTEM_CORE0_CLK_CAL_STA_CAL_OV));
}

/**
 * @brief  Clear Calibrated Clock Counter Overflow Flag.
 * @rmtoll CLK_CAL_STA          CAL_OV          LL_SYSTEM_CORE0_ClearFlag_CALOV
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_CALOV(void)
{
    WRITE_REG(SYSTEM_CORE0->CLK_CAL_STA, SYSTEM_CORE0_CLK_CAL_STA_CAL_OV);
}

/**
 * @brief  Indicate the status of AOS Calibration Flag (Read Only).
 * @rmtoll CLK_CAL_STA          AOS_CAL_BUSY          LL_SYSTEM_CORE0_IsActiveFlag_AOSBUSY
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_AOSBUSY(void)
{
    return (READ_BIT(SYSTEM_CORE0->CLK_CAL_STA, SYSTEM_CORE0_CLK_CAL_STA_AOS_CAL_BUSY) == (SYSTEM_CORE0_CLK_CAL_STA_AOS_CAL_BUSY));
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EF_RESET_FLAG_management Reset FLAG_management
 * @{
 */

/**
 * @brief  Indicate the status of VDET5 Reset Flag.
 * @rmtoll POW_RST_FLAG          VDET5_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_VDET5RST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_VDET5RST(void)
{
    return (READ_BIT(SYSTEM_CORE0->POW_RST_FLAG, SYSTEM_CORE0_POW_RST_FLAG_VDET5) == (SYSTEM_CORE0_POW_RST_FLAG_VDET5));
}

/**
 * @brief  Clear VDET5 Reset Flag.
 * @rmtoll POW_RST_FLAG          VDET5_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_VDET5RST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_VDET5RST(void)
{
    WRITE_REG(SYSTEM_CORE0->POW_RST_FLAG, SYSTEM_CORE0_POW_RST_FLAG_VDET5);
}

/**
 * @brief  Indicate the status of POR5 Reset Flag.
 * @rmtoll POW_RST_FLAG          POR5_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_POR5RST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_POR5RST(void)
{
    return (READ_BIT(SYSTEM_CORE0->POW_RST_FLAG, SYSTEM_CORE0_POW_RST_FLAG_POR5) == (SYSTEM_CORE0_POW_RST_FLAG_POR5));
}

/**
 * @brief  Clear POR5 Reset Flag.
 * @rmtoll POW_RST_FLAG          POR5_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_POR5RST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_POR5RST(void)
{
    WRITE_REG(SYSTEM_CORE0->POW_RST_FLAG, SYSTEM_CORE0_POW_RST_FLAG_POR5);
}

/**
 * @brief  Indicate the status of BOR5 Reset Flag.
 * @rmtoll POW_RST_FLAG          BOR5_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_BOR5RST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_BOR5RST(void)
{
    return (READ_BIT(SYSTEM_CORE0->POW_RST_FLAG, SYSTEM_CORE0_POW_RST_FLAG_BOR5) == (SYSTEM_CORE0_POW_RST_FLAG_BOR5));
}

/**
 * @brief  Clear BOR5 Reset Flag.
 * @rmtoll POW_RST_FLAG          BOR5_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_BOR5RST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_BOR5RST(void)
{
    WRITE_REG(SYSTEM_CORE0->POW_RST_FLAG, SYSTEM_CORE0_POW_RST_FLAG_BOR5);
}

/**
 * @brief  Indicate the status of PROB5 Reset Flag.
 * @rmtoll POW_RST_FLAG          PROB5_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_PROB5RST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_PROB5RST(void)
{
    return (READ_BIT(SYSTEM_CORE0->POW_RST_FLAG, SYSTEM_CORE0_POW_RST_FLAG_PROB5) == (SYSTEM_CORE0_POW_RST_FLAG_PROB5));
}

/**
 * @brief  Clear PROB5 Reset Flag.
 * @rmtoll POW_RST_FLAG          PROB5_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_PROB5RST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_PROB5RST(void)
{
    WRITE_REG(SYSTEM_CORE0->POW_RST_FLAG, SYSTEM_CORE0_POW_RST_FLAG_PROB5);
}

/**
 * @brief  Indicate the status of POR15 Reset Flag.
 * @rmtoll POW_RST_FLAG          POR15_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_POR15RST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_POR15RST(void)
{
    return (READ_BIT(SYSTEM_CORE0->POW_RST_FLAG, SYSTEM_CORE0_POW_RST_FLAG_POR15) == (SYSTEM_CORE0_POW_RST_FLAG_POR15));
}

/**
 * @brief  Clear POR15 Reset Flag.
 * @rmtoll POW_RST_FLAG          POR15_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_POR15RST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_POR15RST(void)
{
    WRITE_REG(SYSTEM_CORE0->POW_RST_FLAG, SYSTEM_CORE0_POW_RST_FLAG_POR15);
}

/**
 * @brief  Indicate the status of BOR15 Reset Flag.
 * @rmtoll POW_RST_FLAG          BOR15_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_BOR15RST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_BOR15RST(void)
{
    return (READ_BIT(SYSTEM_CORE0->POW_RST_FLAG, SYSTEM_CORE0_POW_RST_FLAG_BOR15) == (SYSTEM_CORE0_POW_RST_FLAG_BOR15));
}

/**
 * @brief  Clear BOR15 Reset Flag.
 * @rmtoll POW_RST_FLAG          BOR15_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_BOR15RST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_BOR15RST(void)
{
    WRITE_REG(SYSTEM_CORE0->POW_RST_FLAG, SYSTEM_CORE0_POW_RST_FLAG_BOR15);
}

/**
 * @brief  Indicate the status of PROB15 Reset Flag.
 * @rmtoll POW_RST_FLAG          PROB15_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_PROB15RST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_PROB15RST(void)
{
    return (READ_BIT(SYSTEM_CORE0->POW_RST_FLAG, SYSTEM_CORE0_POW_RST_FLAG_PROB15) == (SYSTEM_CORE0_POW_RST_FLAG_PROB15));
}

/**
 * @brief  Clear PROB15 Reset Flag.
 * @rmtoll POW_RST_FLAG          PROB15_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_PROB15RST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_PROB15RST(void)
{
    WRITE_REG(SYSTEM_CORE0->POW_RST_FLAG, SYSTEM_CORE0_POW_RST_FLAG_PROB15);
}

/**
 * @brief  Indicate the status of POWEROK Reset Flag.
 * @rmtoll SYS_RST_FLAG          POWEROK_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_POWEROKRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_POWEROKRST(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYS_RST_FLAG, SYSTEM_CORE0_SYS_RST_FLAG_POWEROK) == (SYSTEM_CORE0_SYS_RST_FLAG_POWEROK));
}

/**
 * @brief  Clear POWEROK Reset Flag.
 * @rmtoll SYS_RST_FLAG          POWEROK_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_POWEROKRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_POWEROKRST(void)
{
    WRITE_REG(SYSTEM_CORE0->SYS_RST_FLAG, SYSTEM_CORE0_SYS_RST_FLAG_POWEROK);
}

/**
 * @brief  Indicate the status of PIN Reset Flag.
 * @rmtoll SYS_RST_FLAG          PIN_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_PINRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_PINRST(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYS_RST_FLAG, SYSTEM_CORE0_SYS_RST_FLAG_PIN) == (SYSTEM_CORE0_SYS_RST_FLAG_PIN));
}

/**
 * @brief  Clear PIN Reset Flag.
 * @rmtoll SYS_RST_FLAG          PIN_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_PINRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_PINRST(void)
{
    WRITE_REG(SYSTEM_CORE0->SYS_RST_FLAG, SYSTEM_CORE0_SYS_RST_FLAG_PIN);
}

/**
 * @brief  Indicate the status of CORE0 WDT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE0_WDT_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_CORE0WDTRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_CORE0WDTRST(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYS_RST_FLAG, SYSTEM_CORE0_SYS_RST_FLAG_CORE0_WDT) == (SYSTEM_CORE0_SYS_RST_FLAG_CORE0_WDT));
}

/**
 * @brief  Clear CORE0 WDT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE0_WDT_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_CORE0WDTRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_CORE0WDTRST(void)
{
    WRITE_REG(SYSTEM_CORE0->SYS_RST_FLAG, SYSTEM_CORE0_SYS_RST_FLAG_CORE0_WDT);
}

/**
 * @brief  Indicate the status of CORE1T0 WDT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE1T0_WDT_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_CORE1T0WDTRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_CORE1T0WDTRST(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYS_RST_FLAG, SYSTEM_CORE0_SYS_RST_FLAG_CORE1T0_WDT) == (SYSTEM_CORE0_SYS_RST_FLAG_CORE1T0_WDT));
}

/**
 * @brief  Clear CORE1T0 WDT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE1T0_WDT_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_CORE1T0WDTRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_CORE1T0WDTRST(void)
{
    WRITE_REG(SYSTEM_CORE0->SYS_RST_FLAG, SYSTEM_CORE0_SYS_RST_FLAG_CORE1T0_WDT);
}

/**
 * @brief  Indicate the status of CORE0_SOFT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE0_SOFT_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_CORE0SOFTRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_CORE0SOFTRST(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYS_RST_FLAG, SYSTEM_CORE0_SYS_RST_FLAG_CORE0_SOFT) == (SYSTEM_CORE0_SYS_RST_FLAG_CORE0_SOFT));
}

/**
 * @brief  Clear CORE0_SOFT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE0_SOFT_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_CORE0SOFTRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_CORE0SOFTRST(void)
{
    WRITE_REG(SYSTEM_CORE0->SYS_RST_FLAG, SYSTEM_CORE0_SYS_RST_FLAG_CORE0_SOFT);
}

/**
 * @brief  Indicate the status of CORE1T0_SOFT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE1T0_SOFT_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_CORE1T0SOFTRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_CORE1T0SOFTRST(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYS_RST_FLAG, SYSTEM_CORE0_SYS_RST_FLAG_CORE1T0_SOFT) == (SYSTEM_CORE0_SYS_RST_FLAG_CORE1T0_SOFT));
}

/**
 * @brief  Clear CORE1T0_SOFT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE1T0_SOFT_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_CORE1T0SOFTRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_CORE1T0SOFTRST(void)
{
    WRITE_REG(SYSTEM_CORE0->SYS_RST_FLAG, SYSTEM_CORE0_SYS_RST_FLAG_CORE1T0_SOFT);
}

/**
 * @brief  Indicate the status of VBAT BOI Power-Down Reset Flag.
 * @note   When using this flag, after initial power-on,
 *         software must first clear this flag before application.
 * @rmtoll SYS_RST_FLAG          BOI_VBAT_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_BOIVBATRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_BOIVBATRST(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYS_RST_FLAG, SYSTEM_CORE0_SYS_RST_FLAG_BOI_VBAT) == (SYSTEM_CORE0_SYS_RST_FLAG_BOI_VBAT));
}

/**
 * @brief  Clear VBAT BOI Power-Down Reset Flag.
 * @rmtoll SYS_RST_FLAG          BOI_VBAT_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_BOIVBATRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_BOIVBATRST(void)
{
    WRITE_REG(SYSTEM_CORE0->SYS_RST_FLAG, SYSTEM_CORE0_SYS_RST_FLAG_BOI_VBAT);
}

/**
 * @brief  Indicate the status of CORE0 Update Reset Flag.
 * @note   This bit can be reset by all reset sources except the CORE0 update reset itself.
 * @rmtoll UPDATE_RST_CTRL          CORE0_RST_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_UPDATERST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_UPDATERST(void)
{
    return (READ_BIT(SYSTEM_CORE0->UPDATE_RST_CTRL, SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_FLAG) == (SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_FLAG));
}

/**
 * @brief  Clear CORE0 Update Reset Flag.
 * @rmtoll UPDATE_RST_CTRL          CORE0_RST_FLAG          LL_SYSTEM_CORE0_ClearFlag_UPDATERST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_UPDATERST(void)
{
    SET_BIT(SYSTEM_CORE0->UPDATE_RST_CTRL, SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_FLAG);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EF_FLAG_management FLAG_management
 * @{
 */

/**
 * @brief  Indicate the status of PLL Lock flag (read only).
 * @rmtoll PLL_CTRL          PLL_LOCK          LL_SYSTEM_CORE0_IsActiveFlag_PLLLOCK
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_PLLLOCK(void)
{
    return (READ_BIT(SYSTEM_CORE0->PLL_CTRL, SYSTEM_CORE0_PLL_CTRL_LOCK) == (SYSTEM_CORE0_PLL_CTRL_LOCK));
}

/**
 * @brief  Indicate the status of Flash Busy flag (read only).
 * @rmtoll SYS_MODE          FLASH_BUSY          LL_SYSTEM_CORE0_IsActiveFlag_FLASHBUSY
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_FLASHBUSY(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYS_MODE, SYSTEM_CORE0_SYS_MODE_FLASH_BUSY) == (SYSTEM_CORE0_SYS_MODE_FLASH_BUSY));
}

/**
 * @brief  Indicate the status of LOSC or RCL32K clock flag (read only).
 * @rmtoll SYSCLK_STA          SYS_ON_LOSC_RCL          LL_SYSTEM_CORE0_IsActiveFlag_SYSONLOSCRCL
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_SYSONLOSCRCL(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYSCLK_STA, SYSTEM_CORE0_SYSCLK_STA_SYS_ON_LOSC_RCL) == (SYSTEM_CORE0_SYSCLK_STA_SYS_ON_LOSC_RCL));
}

/**
 * @brief  Indicate the status of RCH or Other clock flag (read only).
 * @rmtoll SYSCLK_STA          SYS_ON_RCH          LL_SYSTEM_CORE0_IsActiveFlag_SYSONRCH
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_SYSONRCH(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYSCLK_STA, SYSTEM_CORE0_SYSCLK_STA_SYS_ON_RCH) == (SYSTEM_CORE0_SYSCLK_STA_SYS_ON_RCH));
}

/**
 * @brief  Indicate the status of PLL/HOSC or Other clock flag (read only).
 * @rmtoll SYSCLK_STA          SYS_ON_PLL_HOSC          LL_SYSTEM_CORE0_IsActiveFlag_SYSONPLLHOSC
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_SYSONPLLHOSC(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYSCLK_STA, SYSTEM_CORE0_SYSCLK_STA_SYS_ON_PLL_HOSC) == (SYSTEM_CORE0_SYSCLK_STA_SYS_ON_PLL_HOSC));
}

/**
 * @brief  Indicate the status of LOSC ON flag (read only).
 * @rmtoll SYSCLK_STA          LOSC_ON_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_LOSCON
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_LOSCON(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYSCLK_STA, SYSTEM_CORE0_SYSCLK_STA_LOSC_ON_FLAG) == (SYSTEM_CORE0_SYSCLK_STA_LOSC_ON_FLAG));
}

/**
 * @brief  Indicate the status of PLL ON flag (read only).
 * @rmtoll SYSCLK_STA          PLL_ON_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_PLLON
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_PLLON(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYSCLK_STA, SYSTEM_CORE0_SYSCLK_STA_PLL_ON_FLAG) == (SYSTEM_CORE0_SYSCLK_STA_PLL_ON_FLAG));
}

/**
 * @brief  Indicate the status of RCH ON flag (read only).
 * @rmtoll SYSCLK_STA          RCH_ON_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_RCHON
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_RCHON(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYSCLK_STA, SYSTEM_CORE0_SYSCLK_STA_RCH_ON_FLAG) == (SYSTEM_CORE0_SYSCLK_STA_RCH_ON_FLAG));
}

/**
 * @brief  Indicate the status of RCL32K ON flag (read only).
 * @rmtoll SYSCLK_STA          RCL32K_ON_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_RCL32KON
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_RCL32KON(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYSCLK_STA, SYSTEM_CORE0_SYSCLK_STA_RCL32K_ON_FLAG) == (SYSTEM_CORE0_SYSCLK_STA_RCL32K_ON_FLAG));
}

/**
 * @brief  Indicate the status of HOSC ON flag (read only).
 * @rmtoll SYSCLK_STA          HOSC_ON_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_HOSCON
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_HOSCON(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYSCLK_STA, SYSTEM_CORE0_SYSCLK_STA_HOSC_ON_FLAG) == (SYSTEM_CORE0_SYSCLK_STA_HOSC_ON_FLAG));
}

/**
 * @brief  Indicate the status of RCL307LK ON flag (read only).
 * @rmtoll SYSCLK_STA          RCL307K_ON_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_RCL307KON
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_RCL307KON(void)
{
    return (READ_BIT(SYSTEM_CORE0->SYSCLK_STA, SYSTEM_CORE0_SYSCLK_STA_RCL307K_ON_FLAG) == (SYSTEM_CORE0_SYSCLK_STA_RCL307K_ON_FLAG));
}

/**
 * @brief  Indicate the status of PLVD5 Low Voltage Event Occurred Flag.
 * @rmtoll PLVD5_CTRL          PLVD5_FLAG          LL_SYSTEM_CORE0_IsActiveFlag_PLVD5
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_PLVD5(void)
{
    return (READ_BIT(SYSTEM_CORE0->PLVD5_CTRL, SYSTEM_CORE0_PLVD5_CTRL_PLVD5_FLAG) == (SYSTEM_CORE0_PLVD5_CTRL_PLVD5_FLAG));
}

/**
 * @brief  Clear PLVD5 Low Voltage Event Occurred Flag.
 * @rmtoll PLVD5_CTRL          PLVD5_FLAG          LL_SYSTEM_CORE0_ClearFlag_PLVD5
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_PLVD5(void)
{
    SET_BIT(SYSTEM_CORE0->PLVD5_CTRL, SYSTEM_CORE0_PLVD5_CTRL_PLVD5_FLAG);
}

/**
 * @brief  Indicate the status of CORE02CORE1 Bridge Illegal Access Flag.
 * @rmtoll DIG_REV_CTRL          CORE02CORE1_BRG_PENDING          LL_SYSTEM_CORE0_IsActiveFlag_BRG
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE0_IsActiveFlag_BRG(void)
{
    return (READ_BIT(SYSTEM_CORE0->DIG_REV_CTRL, SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_PENDING) == (SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_PENDING));
}

/**
 * @brief  Clear CORE02CORE1 Bridge Illegal Access Flag.
 * @rmtoll DIG_REV_CTRL          CORE02CORE1_BRG_PENDING          LL_SYSTEM_CORE0_ClearFlag_BRG
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE0_ClearFlag_BRG(void)
{
    SET_BIT(SYSTEM_CORE0->DIG_REV_CTRL, SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_PENDING);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE0_LL_EF_Init Initialization and de-initialization functions
 * @{
 */


/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_SYSTEM_CORE0_H */
