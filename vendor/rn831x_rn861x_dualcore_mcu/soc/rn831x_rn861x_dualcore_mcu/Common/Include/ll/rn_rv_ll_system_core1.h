/**
 *****************************************************************************
 * @file   rn_rv_ll_system_core1.h
 * @author Renergy Software Team
 * @brief  Header file of SYSTEM_CORE1 LL module.
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
#ifndef __RN_LL_SYSTEM_CORE1_H
#define __RN_LL_SYSTEM_CORE1_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_system_core1.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

/** @defgroup SYSTEM_CORE1_LL SYSTEM_CORE1
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup SYSTEM_CORE1_LL_Private_Constants SYSTEM_CORE1 Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup SYSTEM_CORE1_LL_Private_Macros SYSTEM_CORE1 Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup SYSTEM_CORE1_LL_ES_INIT SYSTEM_CORE1 Exported Init structure
 * @{
 */

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup SYSTEM_CORE1_LL_Exported_Constants SYSTEM_CORE1 Exported Constants
 * @{
 */

/** @defgroup SYSTEM_CORE1_LL_EC_SYSCLOCKMODE System Clock Mode
 * @{
 */
#define LL_SYSTEM_CORE1_SYSCLOCKMODE_HCM                (SYSTEM_CORE1_SYS_MODE_SYS_MODE_HCM)              /*!< High-frequency mode HCM, read value 4'b0100 */
#define LL_SYSTEM_CORE1_SYSCLOCKMODE_RCM                (SYSTEM_CORE1_SYS_MODE_SYS_MODE_RCM)              /*!< RC mode RCM, read value 4'b0010             */
#define LL_SYSTEM_CORE1_SYSCLOCKMODE_LCM                (SYSTEM_CORE1_SYS_MODE_SYS_MODE_LCM)              /*!< 32.768KHz mode LCM, read value 4'b0001      */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_PMOD0_CKE_PERIPH  PMOD0_CKE PERIPH
 * @{
 */
#define LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_ALL            (uint32_t)0x00001F1FU
#define LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_WDT            SYSTEM_CORE1_PMOD0_CKE_WDT
#define LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_MAILBOX        SYSTEM_CORE1_PMOD0_CKE_MAILBOX
#define LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_AOS            SYSTEM_CORE1_PMOD0_CKE_AOS
#define LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_SCU            SYSTEM_CORE1_PMOD0_CKE_SCU
#define LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GIC            SYSTEM_CORE1_PMOD0_CKE_GIC
#define LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GDMA10         SYSTEM_CORE1_PMOD0_CKE_GDMA10
#define LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GDMA11         SYSTEM_CORE1_PMOD0_CKE_GDMA11
#define LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_UART38K        SYSTEM_CORE1_PMOD0_CKE_UART38K
#define LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GPIO           SYSTEM_CORE1_PMOD0_CKE_GPIO
#define LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_HSEM           SYSTEM_CORE1_PMOD0_CKE_HSEM
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_PMOD0_RST_PERIPH  PMOD0_RST PERIPH
 * @{
 */
#define LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_ALL            (uint32_t)0x0000001FU
#define LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_MAILBOX        SYSTEM_CORE1_PMOD0_RST_MAILBOX
#define LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_AOS            SYSTEM_CORE1_PMOD0_RST_AOS
#define LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_GDMA10         SYSTEM_CORE1_PMOD0_RST_GDMA10
#define LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_GDMA11         SYSTEM_CORE1_PMOD0_RST_GDMA11
#define LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_HSEM           SYSTEM_CORE1_PMOD0_RST_HSEM
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_KBI0_CHANNEL  KBI0 CHANNEL
 * @{
 */
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_ALL          (uint32_t)0x0000FFFFU
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_0            SYSTEM_CORE1_KBI0_CKE_KBI00
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_1            SYSTEM_CORE1_KBI0_CKE_KBI01
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_2            SYSTEM_CORE1_KBI0_CKE_KBI02
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_3            SYSTEM_CORE1_KBI0_CKE_KBI03
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_4            SYSTEM_CORE1_KBI0_CKE_KBI04
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_5            SYSTEM_CORE1_KBI0_CKE_KBI05
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_6            SYSTEM_CORE1_KBI0_CKE_KBI06
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_7            SYSTEM_CORE1_KBI0_CKE_KBI07
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_8            SYSTEM_CORE1_KBI0_CKE_KBI08
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_9            SYSTEM_CORE1_KBI0_CKE_KBI09
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_10           SYSTEM_CORE1_KBI0_CKE_KBI010
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_11           SYSTEM_CORE1_KBI0_CKE_KBI011
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_12           SYSTEM_CORE1_KBI0_CKE_KBI012
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_13           SYSTEM_CORE1_KBI0_CKE_KBI013
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_14           SYSTEM_CORE1_KBI0_CKE_KBI014
#define LL_SYSTEM_CORE1_KBI0_CHANNEL_15           SYSTEM_CORE1_KBI0_CKE_KBI015
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_KBI1_CHANNEL  KBI1 CHANNEL
 * @{
 */
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_ALL          (uint32_t)0x0000FFFFU
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_0            SYSTEM_CORE1_KBI1_CKE_KBI10
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_1            SYSTEM_CORE1_KBI1_CKE_KBI11
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_2            SYSTEM_CORE1_KBI1_CKE_KBI12
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_3            SYSTEM_CORE1_KBI1_CKE_KBI13
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_4            SYSTEM_CORE1_KBI1_CKE_KBI14
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_5            SYSTEM_CORE1_KBI1_CKE_KBI15
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_6            SYSTEM_CORE1_KBI1_CKE_KBI16
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_7            SYSTEM_CORE1_KBI1_CKE_KBI17
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_8            SYSTEM_CORE1_KBI1_CKE_KBI18
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_9            SYSTEM_CORE1_KBI1_CKE_KBI19
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_10           SYSTEM_CORE1_KBI1_CKE_KBI110
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_11           SYSTEM_CORE1_KBI1_CKE_KBI111
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_12           SYSTEM_CORE1_KBI1_CKE_KBI112
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_13           SYSTEM_CORE1_KBI1_CKE_KBI113
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_14           SYSTEM_CORE1_KBI1_CKE_KBI114
#define LL_SYSTEM_CORE1_KBI1_CHANNEL_15           SYSTEM_CORE1_KBI1_CKE_KBI115
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_MOD0_PERIPH  MOD0 PERIPH
 * @{
 */
#define LL_SYSTEM_CORE1_MOD0_PERIPH_ALL            (uint32_t)0x3F7FFFFFU
#define LL_SYSTEM_CORE1_MOD0_PERIPH_UART0          SYSTEM_CORE1_MOD0_CKE_UART0
#define LL_SYSTEM_CORE1_MOD0_PERIPH_UART1          SYSTEM_CORE1_MOD0_CKE_UART1
#define LL_SYSTEM_CORE1_MOD0_PERIPH_UART2          SYSTEM_CORE1_MOD0_CKE_UART2
#define LL_SYSTEM_CORE1_MOD0_PERIPH_UART3          SYSTEM_CORE1_MOD0_CKE_UART3
#define LL_SYSTEM_CORE1_MOD0_PERIPH_UART4          SYSTEM_CORE1_MOD0_CKE_UART4
#define LL_SYSTEM_CORE1_MOD0_PERIPH_UART5          SYSTEM_CORE1_MOD0_CKE_UART5
#define LL_SYSTEM_CORE1_MOD0_PERIPH_UART6          SYSTEM_CORE1_MOD0_CKE_UART6
#define LL_SYSTEM_CORE1_MOD0_PERIPH_UART7          SYSTEM_CORE1_MOD0_CKE_UART7
#define LL_SYSTEM_CORE1_MOD0_PERIPH_LPUART         SYSTEM_CORE1_MOD0_CKE_LPUART
#define LL_SYSTEM_CORE1_MOD0_PERIPH_USART0         SYSTEM_CORE1_MOD0_CKE_USART0
#define LL_SYSTEM_CORE1_MOD0_PERIPH_USART1         SYSTEM_CORE1_MOD0_CKE_USART1
#define LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78160       SYSTEM_CORE1_MOD0_CKE_ISO78160
#define LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78161       SYSTEM_CORE1_MOD0_CKE_ISO78161
#define LL_SYSTEM_CORE1_MOD0_PERIPH_SPI0           SYSTEM_CORE1_MOD0_CKE_SPI0
#define LL_SYSTEM_CORE1_MOD0_PERIPH_SPI1           SYSTEM_CORE1_MOD0_CKE_SPI1
#define LL_SYSTEM_CORE1_MOD0_PERIPH_SPI2           SYSTEM_CORE1_MOD0_CKE_SPI2
#define LL_SYSTEM_CORE1_MOD0_PERIPH_SPI3           SYSTEM_CORE1_MOD0_CKE_SPI3
#define LL_SYSTEM_CORE1_MOD0_PERIPH_I2C0           SYSTEM_CORE1_MOD0_CKE_I2C0
#define LL_SYSTEM_CORE1_MOD0_PERIPH_I2C1           SYSTEM_CORE1_MOD0_CKE_I2C1
#define LL_SYSTEM_CORE1_MOD0_PERIPH_CAN            SYSTEM_CORE1_MOD0_CKE_CAN
#define LL_SYSTEM_CORE1_MOD0_PERIPH_IOCNT          SYSTEM_CORE1_MOD0_CKE_IOCNT
#define LL_SYSTEM_CORE1_MOD0_PERIPH_LCD            SYSTEM_CORE1_MOD0_CKE_LCD
#define LL_SYSTEM_CORE1_MOD0_PERIPH_DSP            SYSTEM_CORE1_MOD0_CKE_DSP
#define LL_SYSTEM_CORE1_MOD0_PERIPH_M2M            SYSTEM_CORE1_MOD0_CKE_M2M
#define LL_SYSTEM_CORE1_MOD0_PERIPH_D2F            SYSTEM_CORE1_MOD0_CKE_D2F
#define LL_SYSTEM_CORE1_MOD0_PERIPH_ADC0           SYSTEM_CORE1_MOD0_CKE_ADC0
#define LL_SYSTEM_CORE1_MOD0_PERIPH_DAC            SYSTEM_CORE1_MOD0_CKE_DAC
#define LL_SYSTEM_CORE1_MOD0_PERIPH_TC0            SYSTEM_CORE1_MOD0_CKE_TC0
#define LL_SYSTEM_CORE1_MOD0_PERIPH_TC1            SYSTEM_CORE1_MOD0_CKE_TC1
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_MOD1_PERIPH  MOD1 PERIPH
 * @{
 */
#define LL_SYSTEM_CORE1_MOD1_PERIPH_ALL             (uint32_t)0x000007DFU
#define LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC0         SYSTEM_CORE1_MOD1_CKE_SIMPTC0
#define LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC1         SYSTEM_CORE1_MOD1_CKE_SIMPTC1
#define LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC2         SYSTEM_CORE1_MOD1_CKE_SIMPTC2
#define LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC3         SYSTEM_CORE1_MOD1_CKE_SIMPTC3
#define LL_SYSTEM_CORE1_MOD1_PERIPH_RS_VDD          SYSTEM_CORE1_MOD1_CKE_RS_VDD                /*!< Inlcude RTC, SARADC, LCSS_VBAT and GPIOL[3:0] clock */
#define LL_SYSTEM_CORE1_MOD1_PERIPH_LSRX            SYSTEM_CORE1_MOD1_CKE_LSRX
#define LL_SYSTEM_CORE1_MOD1_PERIPH_KBI0            SYSTEM_CORE1_MOD1_RST_KBI0
#define LL_SYSTEM_CORE1_MOD1_PERIPH_KBI1            SYSTEM_CORE1_MOD1_RST_KBI1
#define LL_SYSTEM_CORE1_MOD1_PERIPH_LVDCMP          SYSTEM_CORE1_MOD1_CKE_LVDCMP
#define LL_SYSTEM_CORE1_MOD1_PERIPH_CRC0            SYSTEM_CORE1_MOD1_CKE_CRC0
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_MOD2_PERIPH  MOD2 PERIPH
 * @{
 */
#define LL_SYSTEM_CORE1_MOD2_PERIPH_ALL            (uint32_t)0x000001FFU
#define LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI0          SYSTEM_CORE1_MOD2_CKE_QSPI0
#define LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI1          SYSTEM_CORE1_MOD2_CKE_QSPI1
#define LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_BUS        SYSTEM_CORE1_MOD2_CKE_SEA_BUS
#define LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_PKE        SYSTEM_CORE1_MOD2_CKE_SEA_PKE
#define LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_AES        SYSTEM_CORE1_MOD2_CKE_SEA_AES
#define LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_TRNG       SYSTEM_CORE1_MOD2_CKE_SEA_TRNG
#define LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_SHA        SYSTEM_CORE1_MOD2_CKE_SEA_SHA
#define LL_SYSTEM_CORE1_MOD2_PERIPH_KEY_STORE      SYSTEM_CORE1_MOD2_CKE_KEY_STORE
#define LL_SYSTEM_CORE1_MOD2_PERIPH_AES            SYSTEM_CORE1_MOD2_CKE_AES
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_CORE0FREQ CORE0 System Frequency
 * @{
 */
#define LL_SYSTEM_CORE1_CORE0FREQ_32K768              (SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_32K768)             /*!< Current system main clock is 32.768KHz */
#define LL_SYSTEM_CORE1_CORE0FREQ_1M8432              (SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_1M8432)             /*!< Current system main clock is 1.8432MHz */
#define LL_SYSTEM_CORE1_CORE0FREQ_3M6864              (SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_3M6864)             /*!< Current system main clock is 3.6864MHz */
#define LL_SYSTEM_CORE1_CORE0FREQ_7M3728              (SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_7M3728)             /*!< Current system main clock is 7.3728MHz */
#define LL_SYSTEM_CORE1_CORE0FREQ_14M7456             (SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_14M7456)            /*!< Current system main clock is 14.7456MHz */
#define LL_SYSTEM_CORE1_CORE0FREQ_29M4912             (SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_29M4912)            /*!< Current system main clock is 29.4912MHz */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_CORE1FREQ CORE1 System Frequency
 * @{
 */
#define LL_SYSTEM_CORE1_CORE1FREQ_32K768              (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_32K768)              /*!< Current system main clock is 32.768KHz */
#define LL_SYSTEM_CORE1_CORE1FREQ_1M8432              (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_1M8432)              /*!< Current system main clock is 1.8432MHz */
#define LL_SYSTEM_CORE1_CORE1FREQ_3M6864              (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_3M6864)              /*!< Current system main clock is 3.6864MHz */
#define LL_SYSTEM_CORE1_CORE1FREQ_7M3728              (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_7M3728)              /*!< Current system main clock is 7.3728MHz */
#define LL_SYSTEM_CORE1_CORE1FREQ_14M7456             (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_14M7456)             /*!< Current system main clock is 14.7456MHz */
#define LL_SYSTEM_CORE1_CORE1FREQ_29M4912             (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_29M4912)             /*!< Current system main clock is 29.4912MHz */
#define LL_SYSTEM_CORE1_CORE1FREQ_58M9824             (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_58M9824)             /*!< Current system main clock is 58.9824MHz */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_FLASHFREQ Flash Frequency
 * @{
 */
#define LL_SYSTEM_CORE1_FLASHFREQ_32K768              (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_32K768)              /*!< Current Flash clock is 32.768KHz */
#define LL_SYSTEM_CORE1_FLASHFREQ_1M8432              (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_1M8432)              /*!< Current Flash clock is 1.8432MHz */
#define LL_SYSTEM_CORE1_FLASHFREQ_3M6864              (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_3M6864)              /*!< Current Flash clock is 3.6864MHz */
#define LL_SYSTEM_CORE1_FLASHFREQ_7M3728              (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_7M3728)              /*!< Current Flash clock is 7.3728MHz */
#define LL_SYSTEM_CORE1_FLASHFREQ_14M7456             (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_14M7456)             /*!< Current Flash clock is 14.7456MHz */
#define LL_SYSTEM_CORE1_FLASHFREQ_29M4912             (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_29M4912)             /*!< Current Flash clock is 29.4912MHz */
#define LL_SYSTEM_CORE1_FLASHFREQ_58M9824             (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_58M9824)             /*!< Current Flash clock is 58.9824MHz */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_CORE0REMAP Core0 Remap
 * @{
 */
#define LL_SYSTEM_CORE1_CORE0REMAP_0            (SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0_0)               /*!< CORE0 FLASH mapped to address 0x0020_0000 */
#define LL_SYSTEM_CORE1_CORE0REMAP_1            (SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0_1)               /*!< CORE0 FLASH mapped to address 0x0000_0000 */
#define LL_SYSTEM_CORE1_CORE0REMAP_2            (SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0_2)               /*!< CORE0 FLASH mapped to address 0x1020_0000 */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_CORE1REMAP Core1 Remap
 * @{
 */
#define LL_SYSTEM_CORE1_CORE1REMAP_0            (SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1_0)               /*!< CORE1 FLASH mapped to address 0x8000_0000 */
#define LL_SYSTEM_CORE1_CORE1REMAP_1            (SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1_1)               /*!< CORE1 FLASH mapped to address 0x9000_0000 */
#define LL_SYSTEM_CORE1_CORE1REMAP_2            (SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1_2)               /*!< CORE1 FLASH mapped to address 0x8100_0000 */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_CORE0FLASHREMAP Core0 Flash Remap
 * @{
 */
#define LL_SYSTEM_CORE1_CORE0FLASHREMAP_LOW            0x00000000U                                             /*!< CORE0 program start address mapped to lower half of CORE0 FLASH */
#define LL_SYSTEM_CORE1_CORE0FLASHREMAP_HIGH           (SYSTEM_CORE1_FLASH_MAP_CTRL_CORE0_FLASH)               /*!< CORE0 program start address mapped to upper half of CORE0 FLASH */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_CORE0FLASHREMAP Core0 Flash Remap
 * @{
 */
#define LL_SYSTEM_CORE1_CORE1FLASHREMAP_LOW            0x00000000U                                             /*!< CORE1 program start address mapped to lower half of CORE1 FLASH */
#define LL_SYSTEM_CORE1_CORE1FLASHREMAP_HIGH           (SYSTEM_CORE1_FLASH_MAP_CTRL_CORE1_FLASH)               /*!< CORE1 program start address mapped to upper half of CORE1 FLASH */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_DMACHANNEL DMA Channel
 * @{
 */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_0            0x00000000U               /*!< Channel 0: SPI0 channel access SRAM.     */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_1            0x00000001U               /*!< Channel 1: SPI1 channel access SRAM.     */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_2            0x00000002U               /*!< Channel 2: SPI2 channel access SRAM.     */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_3            0x00000003U               /*!< Channel 3: SPI3 channel access SRAM.     */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_4            0x00000004U               /*!< Channel 4: SARADC channel access SRAM.   */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_5            0x00000005U               /*!< Channel 5: DAC channel access SRAM.      */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_6            0x00000006U               /*!< Channel 6: UART0 channel access SRAM.    */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_7            0x00000007U               /*!< Channel 7: UART1 channel access SRAM.    */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_8            0x00000008U               /*!< Channel 8: UART2 channel access SRAM.    */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_9            0x00000009U               /*!< Channel 9: UART3 channel access SRAM.    */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_10           0x0000000AU               /*!< Channel 10: UART4 channel access SRAM.   */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_11           0x0000000BU               /*!< Channel 11: UART5 channel access SRAM.   */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_12           0x0000000CU               /*!< Channel 12: UART6 channel access SRAM.   */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_13           0x0000000DU               /*!< Channel 13: UART7 channel access SRAM.   */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_14           0x0000000EU               /*!< Channel 14: CAN channel access SRAM.     */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_15           0x0000000FU               /*!< Channel 15: TC0_PWM channel access SRAM. */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_16           0x00000010U               /*!< Channel 16: TC1_PWM channel access SRAM. */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_17           0x00000011U               /*!< Channel 17: AES channel access SRAM.     */
#define LL_SYSTEM_CORE1_DMAPRI0CHANNEL_18           0x00000012U               /*!< Channel 18: M2M channel access SRAM.     */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_DMAPRIORITY0 Transfer Priority level1
 * @{
 */
#define LL_SYSTEM_CORE1_DMAPRIORITY_LOW               0x00000000U            /*!< Priority level : Low       */
#define LL_SYSTEM_CORE1_DMAPRIORITY_MEDIUM            0x00000001U            /*!< Priority level : Medium    */
#define LL_SYSTEM_CORE1_DMAPRIORITY_HIGH              0x00000002U            /*!< Priority level : High      */
#define LL_SYSTEM_CORE1_DMAPRIORITY_VERYHIGH          0x00000003U            /*!< Priority level : Very_High */
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EC_DMACHANNEL DMA Channel
 * @{
 */
#define LL_SYSTEM_CORE1_DMAPRI1CHANNEL_0            0x00000000U              /*!< Channel 0: First level arbitration result channel access SRAM.          */
#define LL_SYSTEM_CORE1_DMAPRI1CHANNEL_1            0x00000001U              /*!< Channel 1: General DMA read channel (MDMA0x_RD/MDMA1x_RD) access SRAM.  */
#define LL_SYSTEM_CORE1_DMAPRI1CHANNEL_2            0x00000002U              /*!< Channel 2: General DMA write channel (MDMA0x_WR/MDMA1x_WR) access SRAM. */
#define LL_SYSTEM_CORE1_DMAPRI1CHANNEL_3            0x00000003U              /*!< Channel 3: DSP channel access SRAM.                                     */
#define LL_SYSTEM_CORE1_DMAPRI1CHANNEL_4            0x00000004U              /*!< Channel 4: CORE0/CORE1 channel access SRAM.                             */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup SYSTEM_CORE1_LL_Exported_Macros SYSTEM_CORE1 Exported Macros
 * @{
 */
/** @defgroup SYSTEM_CORE1_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in SYSTEM_CORE1 register
 * @param  __INSTANCE__ SYSTEM_CORE1 Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_SYSTEM_CORE1_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in SYSTEM_CORE1 register
 * @param  __INSTANCE__ SYSTEM_CORE1 Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_SYSTEM_CORE1_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EM_Exported_Macros_Helper Exported Macros Helper
 * @{
 */

/**
 * @brief  Helper macro to calculate the Core0 CLK frequency ((in Hz))
 * @note:  __SYSCLKFREQ__ be retrieved by @ref LL_SYSTEM_CORE1_GetCore0SystemClockFreq
 * @param  __SYSCLKFREQ__: This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_CORE0FREQ_32K768
 *         @arg @ref LL_SYSTEM_CORE1_CORE0FREQ_1M8432
 *         @arg @ref LL_SYSTEM_CORE1_CORE0FREQ_3M6864
 *         @arg @ref LL_SYSTEM_CORE1_CORE0FREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE1_CORE0FREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE1_CORE0FREQ_29M4912
 * @retval HCLK clock frequency (in Hz)
 */
#define __LL_SYSTEM_CORE1_CALC_CORE0_CLK_FREQ(__SYSCLKFREQ__) \
                        (((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_CORE0FREQ_32K768) ? 32768U : \
                            ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_CORE0FREQ_1M8432) ? 1843200U : \
                                ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_CORE0FREQ_3M6864) ? 3686400U : \
                                    ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_CORE0FREQ_7M3728) ? 7372800 : \
                                        ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_CORE0FREQ_14M7456) ? 14745600 : \
                                            ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_CORE0FREQ_29M4912) ? 29491200 : 0x00U)

/**
 * @brief  Helper macro to calculate the Core1 CLK frequency ((in Hz))
 * @note:  __SYSCLKFREQ__ be retrieved by @ref LL_SYSTEM_CORE1_GetCore1SystemClockFreq
 * @param  __SYSCLKFREQ__: This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FREQ_32K768
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FREQ_1M8432
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FREQ_3M6864
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FREQ_29M4912
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FREQ_58M9824
 * @retval HCLK clock frequency (in Hz)
 */
#define __LL_SYSTEM_CORE1_CALC_CORE1_CLK_FREQ(__SYSCLKFREQ__) \
                        (((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_CORE1FREQ_32K768) ? 32768U : \
                            ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_CORE1FREQ_1M8432) ? 1843200U : \
                                ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_CORE1FREQ_3M6864) ? 3686400U : \
                                    ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_CORE1FREQ_7M3728) ? 7372800U : \
                                        ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_CORE1FREQ_14M7456) ? 14745600U : \
                                            ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_CORE1FREQ_29M4912) ? 29491200U : \
                                                ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_CORE1FREQ_58M9824) ? 58982400U : 0x00U)

/**
 * @brief  Helper macro to calculate the Flash CLK frequency ((in Hz))
 * @note:  __SYSCLKFREQ__ be retrieved by @ref LL_SYSTEM_CORE1_GetFlashClockFreq
 * @param  __SYSCLKFREQ__: This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_FLASHFREQ_32K768
 *         @arg @ref LL_SYSTEM_CORE1_FLASHFREQ_1M8432
 *         @arg @ref LL_SYSTEM_CORE1_FLASHFREQ_3M6864
 *         @arg @ref LL_SYSTEM_CORE1_FLASHFREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE1_FLASHFREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE1_FLASHFREQ_29M4912
 *         @arg @ref LL_SYSTEM_CORE1_FLASHFREQ_58M9824
 * @retval HCLK clock frequency (in Hz)
 */
#define __LL_SYSTEM_CORE1_CALC_FLASH_CLK_FREQ(__SYSCLKFREQ__) \
                        (((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_FLASHFREQ_32K768) ? 32768U : \
                            ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_FLASHFREQ_1M8432) ? 1843200U : \
                                ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_FLASHFREQ_3M6864) ? 3686400U : \
                                    ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_FLASHFREQ_7M3728) ? 7372800U : \
                                        ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_FLASHFREQ_14M7456) ? 14745600U : \
                                            ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_FLASHFREQ_29M4912) ? 29491200U : \
                                                ((__SYSCLKFREQ__) == LL_SYSTEM_CORE1_FLASHFREQ_58M9824) ? 58982400U : 0x00U)

/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup SYSTEM_CORE1_LL_Exported_Functions SYSTEM_CORE1 Exported Functions
 * @{
 */

/** @defgroup SYSTEM_CORE1_LL_EF_Configuration Configuration functions
 * @{
 */

/**
 * @brief  Unlock System PSW
 * @rmtoll SYS_PSW1         SYS_PSW1         LL_SYSTEM_CORE1_UnlockSystemPSW
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_UnlockSystemPSW(void)
{
    MODIFY_REG(SYSTEM_CORE1->SYS_PSW1, SYSTEM_CORE1_SYS_PSW1_SYS_PSW1, 0x82U);
}

/**
 * @brief  Lock System PSW
 * @rmtoll SYS_PSW1         SYS_PSW1         LL_SYSTEM_CORE1_LockSystemPSW
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_LockSystemPSW(void)
{
    MODIFY_REG(SYSTEM_CORE1->SYS_PSW1, SYSTEM_CORE1_SYS_PSW1_SYS_PSW1, 0x00U);
}

/**
 * @brief  Check if System PSW has been Unloced or not
 * @rmtoll SYS_PSW1         SYS_PSW1         LL_SYSTEM_CORE1_IsUnlockedSystemPSW
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsUnlockedSystemPSW(void)
{
    return (READ_BIT(SYSTEM_CORE1->SYS_PSW1, SYSTEM_CORE1_SYS_PSW1_SYS_PSW1) == (0x82U));
}

/**
 * @brief  Get the System Clock Mode (read only).
 * @rmtoll SYS_MODE         SYS_MODE        LL_SYSTEM_CORE1_GetSystemClockMode
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_SYSCLOCKMODE_HCM
 *         @arg @ref LL_SYSTEM_CORE1_SYSCLOCKMODE_RCM
 *         @arg @ref LL_SYSTEM_CORE1_SYSCLOCKMODE_LCM
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_GetSystemClockMode(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE1->SYS_MODE, SYSTEM_CORE1_SYS_MODE_SYS_MODE));
}

/**
 * @brief  Get the CORE0 System Main Clock Frequency (read only).
 * @rmtoll SYSFREQ_STA         CORE0_SYS_FREQ        LL_SYSTEM_CORE1_GetCore0SystemClockFreq
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_CORE0FREQ_32K768
 *         @arg @ref LL_SYSTEM_CORE1_CORE0FREQ_1M8432
 *         @arg @ref LL_SYSTEM_CORE1_CORE0FREQ_3M6864
 *         @arg @ref LL_SYSTEM_CORE1_CORE0FREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE1_CORE0FREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE1_CORE0FREQ_29M4912
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_GetCore0SystemClockFreq(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE1->SYSFREQ_STA, SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS));
}

/**
 * @brief  Get the CORE1 System Main Clock Frequency (read only).
 * @rmtoll SYSFREQ_STA         CORE1_SYS_FREQ        LL_SYSTEM_CORE1_GetCore1SystemClockFreq
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FREQ_32K768
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FREQ_1M8432
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FREQ_3M6864
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FREQ_29M4912
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FREQ_58M9824
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_GetCore1SystemClockFreq(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE1->SYSFREQ_STA, SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS));
}

/**
 * @brief  Get the Flash Clock Frequency (read only).
 * @rmtoll SYSFREQ_STA         FLASH_CLK_FREQ        LL_SYSTEM_CORE1_GetFlashClockFreq
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_FLASHFREQ_32K768
 *         @arg @ref LL_SYSTEM_CORE1_FLASHFREQ_1M8432
 *         @arg @ref LL_SYSTEM_CORE1_FLASHFREQ_3M6864
 *         @arg @ref LL_SYSTEM_CORE1_FLASHFREQ_7M3728
 *         @arg @ref LL_SYSTEM_CORE1_FLASHFREQ_14M7456
 *         @arg @ref LL_SYSTEM_CORE1_FLASHFREQ_29M4912
 *         @arg @ref LL_SYSTEM_CORE1_FLASHFREQ_58M9824
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_GetFlashSystemClockFreq(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE1->SYSFREQ_STA, SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK));
}

/**
 * @brief  Get the CORE0 Remap.
 * @rmtoll ADDR_MAP_CTRL         CORE0_REMAP        LL_SYSTEM_CORE1_GetCore0AddressRemap
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_CORE0REMAP_0
 *         @arg @ref LL_SYSTEM_CORE1_CORE0REMAP_1
 *         @arg @ref LL_SYSTEM_CORE1_CORE0REMAP_2
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_GetCore0AddressRemap(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE1->ADDR_MAP_CTRL, SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0));
}

/**
 * @brief  Configure CORE1 Remap.
 * @rmtoll ADDR_MAP_CTRL      CORE1_REMAP        LL_SYSTEM_CORE1_SetCore1AddressRemap
 * @param Remap This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_CORE1REMAP_0
 *         @arg @ref LL_SYSTEM_CORE1_CORE1REMAP_1
 *         @arg @ref LL_SYSTEM_CORE1_CORE1REMAP_2
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_SetCore1AddressRemap(uint32_t Remap)
{
    MODIFY_REG(SYSTEM_CORE1->ADDR_MAP_CTRL, SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1, Remap);
}

/**
 * @brief  Get the CORE1 Remap.
 * @rmtoll ADDR_MAP_CTRL         CORE1_REMAP        LL_SYSTEM_CORE1_GetCore1AddressRemap
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_CORE1REMAP_0
 *         @arg @ref LL_SYSTEM_CORE1_CORE1REMAP_1
 *         @arg @ref LL_SYSTEM_CORE1_CORE1REMAP_2
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_GetCore1AddressRemap(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE1->ADDR_MAP_CTRL, SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1));
}

/**
 * @brief  Get the CORE0 Flash Remap.
 * @rmtoll FLASH_MAP_CTRL         CORE0_FLASH_REMAP        LL_SYSTEM_CORE1_GetCore0FlashRemap
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_CORE0FLASHREMAP_LOW
 *         @arg @ref LL_SYSTEM_CORE1_CORE0FLASHREMAP_HIGH
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_GetCore0FlashRemap(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE1->FLASH_MAP_CTRL, SYSTEM_CORE1_FLASH_MAP_CTRL_CORE0_FLASH));
}

/**
 * @brief  Configure CORE1 Flash Remap.
 * @rmtoll FLASH_MAP_CTRL      CORE1_FLASH_REMAP        LL_SYSTEM_CORE1_SetCore1FlashRemap
 * @param Remap This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FLASHREMAP_LOW
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FLASHREMAP_HIGH
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_SetCore1FlashRemap(uint32_t Remap)
{
    MODIFY_REG(SYSTEM_CORE1->FLASH_MAP_CTRL, SYSTEM_CORE1_FLASH_MAP_CTRL_CORE1_FLASH, Remap);
}

/**
 * @brief  Get the CORE1 Flash Remap.
 * @rmtoll FLASH_MAP_CTRL         CORE1_FLASH_REMAP        LL_SYSTEM_CORE1_GetCore1FlashRemap
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FLASHREMAP_LOW
 *         @arg @ref LL_SYSTEM_CORE1_CORE1FLASHREMAP_HIGH
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_GetCore1FlashRemap(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE1->FLASH_MAP_CTRL, SYSTEM_CORE1_FLASH_MAP_CTRL_CORE1_FLASH));
}

/**
 * @brief  Configure DMA Access SRAM Priority First Level.
 * @rmtoll DMA_PRI0_CTRL      DMA_CH0_PRI0        LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH1_PRI0        LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH2_PRI0        LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH3_PRI0        LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH4_PRI0        LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH5_PRI0        LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH6_PRI0        LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH7_PRI0        LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH8_PRI0        LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH9_PRI0        LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH10_PRI0       LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH11_PRI0       LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH12_PRI0       LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH13_PRI0       LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH14_PRI0       LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH15_PRI0       LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI1_CTRL      DMA_CH16_PRI0       LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI1_CTRL      DMA_CH17_PRI0       LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel\n
            DMA_PRI1_CTRL      DMA_CH18_PRI0       LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_15
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_16
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_17
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_18
 * @param Priority This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_LOW
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_MEDIUM
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_HIGH
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_VERYHIGH
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_DMA_SetChannelPriorityFirstLevel(uint32_t Channel, uint32_t Priority)
{
    if (Channel <= LL_SYSTEM_CORE1_DMAPRI0CHANNEL_15) {
        MODIFY_REG(SYSTEM_CORE1->DMA_PRI0_CTRL, SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH0 << (Channel * 2), Priority << (Channel * 2));
    } else {
        MODIFY_REG(SYSTEM_CORE1->DMA_PRI1_CTRL, SYSTEM_CORE1_DMA_PRI1_CTRL_DMA_CH16 << ((Channel - 16) * 2), Priority << ((Channel - 16) * 2));
    }
}

/**
 * @brief  Get the DMA Access SRAM Priority First Level.
 * @rmtoll DMA_PRI0_CTRL      DMA_CH0_PRI0        LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH1_PRI0        LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH2_PRI0        LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH3_PRI0        LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH4_PRI0        LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH5_PRI0        LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH6_PRI0        LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH7_PRI0        LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH8_PRI0        LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH9_PRI0        LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH10_PRI0       LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH11_PRI0       LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH12_PRI0       LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH13_PRI0       LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH14_PRI0       LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI0_CTRL      DMA_CH15_PRI0       LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI1_CTRL      DMA_CH16_PRI0       LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI1_CTRL      DMA_CH17_PRI0       LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel\n
            DMA_PRI1_CTRL      DMA_CH18_PRI0       LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_15
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_16
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_17
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI0CHANNEL_18
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_LOW
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_MEDIUM
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_HIGH
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_VERYHIGH
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_DMA_GetChannelPriorityFirstLevel(uint32_t Channel)
{
    if (Channel <= LL_SYSTEM_CORE1_DMAPRI0CHANNEL_15) {
        return (uint32_t)(READ_BIT(SYSTEM_CORE1->DMA_PRI0_CTRL, SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH0 << (Channel * 2)) >> (Channel * 2));
    } else {
        return (uint32_t)(READ_BIT(SYSTEM_CORE1->DMA_PRI1_CTRL, SYSTEM_CORE1_DMA_PRI1_CTRL_DMA_CH16 << ((Channel - 16) * 2)) >> ((Channel - 16) * 2));
    }
}

/**
 * @brief  Configure DMA Access SRAM Priority Second Level.
 * @rmtoll DMA_PRI2_CTRL      DMA_CH0_PRI1        LL_SYSTEM_CORE1_DMA_SetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH1_PRI1        LL_SYSTEM_CORE1_DMA_SetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH2_PRI1        LL_SYSTEM_CORE1_DMA_SetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH3_PRI1        LL_SYSTEM_CORE1_DMA_SetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH4_PRI1        LL_SYSTEM_CORE1_DMA_SetChannelPrioritySecondLevel
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI1CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI1CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI1CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI1CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI1CHANNEL_4
 * @param Priority This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_LOW
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_MEDIUM
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_HIGH
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_VERYHIGH
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_DMA_SetChannelPrioritySecondLevel(uint32_t Channel, uint32_t Priority)
{
    MODIFY_REG(SYSTEM_CORE1->DMA_PRI2_CTRL, SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH0 << (Channel * 2), Priority << (Channel * 2));
}

/**
 * @brief  Get the DMA Access SRAM Priority Second Level.
 * @rmtoll DMA_PRI2_CTRL      DMA_CH0_PRI1        LL_SYSTEM_CORE1_DMA_GetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH1_PRI1        LL_SYSTEM_CORE1_DMA_GetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH2_PRI1        LL_SYSTEM_CORE1_DMA_GetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH3_PRI1        LL_SYSTEM_CORE1_DMA_GetChannelPrioritySecondLevel\n
            DMA_PRI2_CTRL      DMA_CH4_PRI1        LL_SYSTEM_CORE1_DMA_GetChannelPrioritySecondLevel
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI1CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI1CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI1CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI1CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRI1CHANNEL_4
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_LOW
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_MEDIUM
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_HIGH
 *         @arg @ref LL_SYSTEM_CORE1_DMAPRIORITY_VERYHIGH
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_DMA_GetChannelPrioritySecondLevel(uint32_t Channel)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE1->DMA_PRI2_CTRL, SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH0 << (Channel * 2)) >> (Channel * 2));
}

/**
 * @brief  Get the CHIP_ID.
 * @rmtoll CHIP_ID      CHIP_ID        LL_SYSTEM_CORE1_GetChipID
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFF.
 */
__STATIC_INLINE uint16_t LL_SYSTEM_CORE1_GetChipID(void)
{
    return (uint16_t)(READ_BIT(SYSTEM_CORE1->CHIP_ID, SYSTEM_CORE1_CHIP_ID_CHIP_ID) >> SYSTEM_CORE1_CHIP_ID_CHIP_ID_POS);
}

/**
 * @brief  Get the Version Number.
 * @rmtoll DIE_VER      VERSION        LL_SYSTEM_CORE1_GetChipVersion
 * @retval Value between Min_Data=0x00 and Max_Data=0x0F.
 */
__STATIC_INLINE uint8_t LL_SYSTEM_CORE1_GetChipVersion(void)
{
    return (uint8_t)(READ_BIT(SYSTEM_CORE1->DIE_VER, SYSTEM_CORE1_DIE_VER_VERSION) >> SYSTEM_CORE1_DIE_VER_VERSION_POS);
}

/**
 * @brief  Get the Chip Model, Encoded in ASCII.
 * @rmtoll DIE_VER      CHIP_MODEL        LL_SYSTEM_CORE1_GetChipType
 * @retval Value between Min_Data=0x00 and Max_Data=0x0FFFFFFF.
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_GetChipType(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE1->DIE_VER, SYSTEM_CORE1_DIE_VER_CHIP_MODEL) >> SYSTEM_CORE1_DIE_VER_CHIP_MODEL_POS);
}

/**
 * @brief  Get the the wafer LOT NUM information.
 * @rmtoll FAB_UID0      LOT_NUM        LL_SYSTEM_CORE1_GetLOTNumber
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_GetLOTNumber(void)
{
    return (uint32_t)(READ_BIT(SYSTEM_CORE1->DIE_VER, SYSTEM_CORE1_FAB_UID0_LOT_NUM) >> SYSTEM_CORE1_DIE_VER_CHIP_MODEL_POS);
}

/**
 * @brief  Get the the wafer LOT NUM information.
 * @rmtoll FAB_UID1      WAFER_ID        LL_SYSTEM_CORE1_GetWaferID
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF.
 */
__STATIC_INLINE uint8_t LL_SYSTEM_CORE1_GetWaferID(void)
{
    return (uint8_t)(READ_BIT(SYSTEM_CORE1->DIE_VER, SYSTEM_CORE1_FAB_UID1_WAFER_ID) >> SYSTEM_CORE1_FAB_UID1_WAFER_ID_POS);
}

/**
 * @brief  Get the the DIE X-coordinate information.
 * @rmtoll FAB_UID1      X_LABEL        LL_SYSTEM_CORE1_GetXLabel
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFF.
 */
__STATIC_INLINE uint16_t LL_SYSTEM_CORE1_GetXLabel(void)
{
    return (uint8_t)(READ_BIT(SYSTEM_CORE1->DIE_VER, SYSTEM_CORE1_FAB_UID1_X_LABEL) >> SYSTEM_CORE1_FAB_UID1_X_LABEL_POS);
}

/**
 * @brief  Get the the DIE Y-coordinate information.
 * @rmtoll FAB_UID1      Y_LABEL        LL_SYSTEM_CORE1_GetYLabel
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFF.
 */
__STATIC_INLINE uint16_t LL_SYSTEM_CORE1_GetYLabel(void)
{
    return (uint8_t)(READ_BIT(SYSTEM_CORE1->DIE_VER, SYSTEM_CORE1_FAB_UID1_Y_LABEL) >> SYSTEM_CORE1_FAB_UID1_Y_LABEL_POS);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EF_Reset Reset functions
 * @{
 */

/**
 * @brief  Enable CORE0 Update Reset.
 * @rmtoll UPDATE_RST_CTRL          CORE0_RST_EN       LL_SYSTEM_CORE1_EnableUpdateReset
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_EnableUpdateReset(void)
{
    WRITE_REG(SYSTEM_CORE1->UPDATE_RST_CTRL, SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_EN | 0x000035E2U);
}

/**
 * @brief  Disable CORE0 Update Reset.
 * @rmtoll UPDATE_RST_CTRL          CORE0_RST_EN       LL_SYSTEM_CORE1_DisableUpdateReset
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_DisableUpdateReset(void)
{
    WRITE_REG(SYSTEM_CORE1->UPDATE_RST_CTRL, (~SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_EN) & SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_EN);
}

/**
 * @brief  Check if CORE0 Update Reset are enabled or disabled.
 * @rmtoll UPDATE_RST_CTRL          CORE0_RST_EN       LL_SYSTEM_CORE1_IsEnabledUpdateReset
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsEnabledUpdateReset(void)
{
    return (READ_BIT(SYSTEM_CORE1->UPDATE_RST_CTRL, SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_EN) == (0x35E2U));
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EF_PMOD0 PMOD0
 * @{
 */

/**
 * @brief  Enable PMOD0 peripherals clock.
 * @rmtoll PMOD0_CKE      WDT_CKE           LL_SYSTEM_CORE1_PMOD0_EnableClock\n
 *         PMOD0_CKE      MAILBOX_CKE       LL_SYSTEM_CORE1_PMOD0_EnableClock\n
 *         PMOD0_CKE      AOS_CKE           LL_SYSTEM_CORE1_PMOD0_EnableClock\n
 *         PMOD0_CKE      SCU_CKE           LL_SYSTEM_CORE1_PMOD0_EnableClock\n
 *         PMOD0_CKE      GIC_CKE           LL_SYSTEM_CORE1_PMOD0_EnableClock\n
 *         PMOD0_CKE      GDMA10_CKE        LL_SYSTEM_CORE1_PMOD0_EnableClock\n
 *         PMOD0_CKE      GDMA11_CKE        LL_SYSTEM_CORE1_PMOD0_EnableClock\n
 *         PMOD0_CKE      UART38K_CKE       LL_SYSTEM_CORE1_PMOD0_EnableClock\n
 *         PMOD0_CKE      GPIO_CKE          LL_SYSTEM_CORE1_PMOD0_EnableClock\n
 *         PMOD0_CKE      HSEM_CKE          LL_SYSTEM_CORE1_PMOD0_EnableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_WDT
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_MAILBOX
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_AOS
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_SCU
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GIC
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GDMA10
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GDMA11
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_UART38K
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GPIO
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_HSEM
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_PMOD0_EnableClock(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE1->PMOD0_CKE, Periphs);
}

/**
 * @brief  Disable PMOD0 peripherals clock.
 * @rmtoll PMOD0_CKE      WDT_CKE           LL_SYSTEM_CORE1_PMOD0_DisableClock\n
 *         PMOD0_CKE      MAILBOX_CKE       LL_SYSTEM_CORE1_PMOD0_DisableClock\n
 *         PMOD0_CKE      AOS_CKE           LL_SYSTEM_CORE1_PMOD0_DisableClock\n
 *         PMOD0_CKE      SCU_CKE           LL_SYSTEM_CORE1_PMOD0_DisableClock\n
 *         PMOD0_CKE      GIC_CKE           LL_SYSTEM_CORE1_PMOD0_DisableClock\n
 *         PMOD0_CKE      GDMA10_CKE        LL_SYSTEM_CORE1_PMOD0_DisableClock\n
 *         PMOD0_CKE      GDMA11_CKE        LL_SYSTEM_CORE1_PMOD0_DisableClock\n
 *         PMOD0_CKE      UART38K_CKE       LL_SYSTEM_CORE1_PMOD0_DisableClock\n
 *         PMOD0_CKE      GPIO_CKE          LL_SYSTEM_CORE1_PMOD0_DisableClock\n
 *         PMOD0_CKE      HSEM_CKE          LL_SYSTEM_CORE1_PMOD0_DisableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_WDT
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_MAILBOX
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_AOS
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_SCU
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GIC
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GDMA10
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GDMA11
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_UART38K
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GPIO
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_HSEM
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_PMOD0_DisableClock(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE1->PMOD0_CKE, Periphs);
}

/**
 * @brief  Check if PMOD0 peripheral clock is enabled or not
 * @rmtoll PMOD0_CKE      WDT_CKE           LL_SYSTEM_CORE1_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      MAILBOX_CKE       LL_SYSTEM_CORE1_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      AOS_CKE           LL_SYSTEM_CORE1_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      SCU_CKE           LL_SYSTEM_CORE1_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      GIC_CKE           LL_SYSTEM_CORE1_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      GDMA10_CKE        LL_SYSTEM_CORE1_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      GDMA11_CKE        LL_SYSTEM_CORE1_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      UART38K_CKE       LL_SYSTEM_CORE1_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      GPIO_CKE          LL_SYSTEM_CORE1_PMOD0_IsEnabledClock\n
 *         PMOD0_CKE      HSEM_CKE          LL_SYSTEM_CORE1_PMOD0_IsEnabledClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_WDT
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_MAILBOX
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_AOS
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_SCU
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GIC
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GDMA10
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GDMA11
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_UART38K
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GPIO
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_HSEM
 * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_PMOD0_IsEnabledClock(uint32_t Periphs)
{
    return (READ_BIT(SYSTEM_CORE1->PMOD0_CKE, Periphs) == Periphs);
}

/**
 * @brief  Force PMOD0 peripherals reset.
 * @note   For all module-level resets, first disable the module clock,
 *         then force the reset, then release the module reset,
 *         finally enable the module clock for normal operation.
 * @rmtoll PMOD0_RST     MAILBOX_RST_REQ       LL_SYSTEM_CORE1_PMOD0_ForceReset\n
 *         PMOD0_RST     AOS_RST_REQ           LL_SYSTEM_CORE1_PMOD0_ForceReset\n
 *         PMOD0_RST     GDMA10_RST_REQ        LL_SYSTEM_CORE1_PMOD0_ForceReset\n
 *         PMOD0_RST     GDMA11_RST_REQ        LL_SYSTEM_CORE1_PMOD0_ForceReset\n
 *         PMOD0_RST     HSEM_RST_REQ          LL_SYSTEM_CORE1_PMOD0_ForceReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_MAILBOX
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_AOS
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_GDMA10
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_GDMA11
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_HSEM
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_PMOD0_ForceReset(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE1->PMOD0_RST, Periphs);
}

/**
 * @brief  Release PMOD0 peripherals reset.
 * @rmtoll PMOD0_RST     MAILBOX_RST_REQ       LL_SYSTEM_CORE1_PMOD0_ReleaseReset\n
 *         PMOD0_RST     AOS_RST_REQ           LL_SYSTEM_CORE1_PMOD0_ReleaseReset\n
 *         PMOD0_RST     GDMA10_RST_REQ        LL_SYSTEM_CORE1_PMOD0_ReleaseReset\n
 *         PMOD0_RST     GDMA11_RST_REQ        LL_SYSTEM_CORE1_PMOD0_ReleaseReset\n
 *         PMOD0_RST     HSEM_RST_REQ          LL_SYSTEM_CORE1_PMOD0_ReleaseReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_MAILBOX
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_AOS
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_GDMA10
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_GDMA11
 *         @arg @ref LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_HSEM
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_PMOD0_ReleaseReset(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE1->PMOD0_RST, Periphs);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EF_KBI_CKE KBI_CKE
 * @{
 */

/**
 * @brief  Enable KBI0 APB module clock.
 * @rmtoll KBI0_CKE      KBIC0_CKE           LL_SYSTEM_CORE1_KBI0_EnableClock
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_KBI0_EnableClock(void)
{
    SET_BIT(SYSTEM_CORE1->KBI0_CKE, SYSTEM_CORE1_KBI0_CKE_KBIC0);
}

/**
 * @brief  Disable KBI0 APB module clock.
 * @rmtoll KBI0_CKE      KBIC0_CKE           LL_SYSTEM_CORE1_KBI0_DisableClock\
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_KBI0_DisableClock(void)
{
    CLEAR_BIT(SYSTEM_CORE1->KBI0_CKE, SYSTEM_CORE1_KBI0_CKE_KBIC0);
}

/**
 * @brief  Check if KBI0 APB module clock is enabled or not
 * @rmtoll KBI0_CKE      KBIC0_CKE           LL_SYSTEM_CORE1_KBI0_IsEnabledClock
 * @retval State of KBI0 Periph (1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_KBI0_IsEnabledClock(void)
{
    return (READ_BIT(SYSTEM_CORE1->KBI0_CKE, SYSTEM_CORE1_KBI0_CKE_KBIC0) == SYSTEM_CORE1_KBI0_CKE_KBIC0);
}

/**
 * @brief  Enable KBI0 channels clock.
 * @rmtoll KBI0_CKE      KBI00_CKE           LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI01_CKE           LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI02_CKE           LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI03_CKE           LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI04_CKE           LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI05_CKE           LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI06_CKE           LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI07_CKE           LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI08_CKE           LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI09_CKE           LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI010_CKE          LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI011_CKE          LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI012_CKE          LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI013_CKE          LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI014_CKE          LL_SYSTEM_CORE1_KBI0_EnableChannelClock\n
 *         KBI0_CKE      KBI015_CKE          LL_SYSTEM_CORE1_KBI0_EnableChannelClock
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_ALL
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_15
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_KBI0_EnableChannelClock(uint32_t Channels)
{
    SET_BIT(SYSTEM_CORE1->KBI0_CKE, Channels);
}

/**
 * @brief  Disable KBI0 channels clock.
 * @rmtoll KBI0_CKE      KBI00_CKE           LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI01_CKE           LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI02_CKE           LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI03_CKE           LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI04_CKE           LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI05_CKE           LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI06_CKE           LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI07_CKE           LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI08_CKE           LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI09_CKE           LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI010_CKE          LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI011_CKE          LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI012_CKE          LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI013_CKE          LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI014_CKE          LL_SYSTEM_CORE1_KBI0_DisableChannelClock\n
 *         KBI0_CKE      KBI015_CKE          LL_SYSTEM_CORE1_KBI0_DisableChannelClock
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_ALL
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_15
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_KBI0_DisableChannelClock(uint32_t Channels)
{
    CLEAR_BIT(SYSTEM_CORE1->KBI0_CKE, Channels);
}

/**
 * @brief  Check if KBI0 channels clock is enabled or not
 * @rmtoll KBI0_CKE      KBI00_CKE           LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI01_CKE           LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI02_CKE           LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI03_CKE           LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI04_CKE           LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI05_CKE           LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI06_CKE           LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI07_CKE           LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI08_CKE           LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI09_CKE           LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI010_CKE          LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI011_CKE          LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI012_CKE          LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI013_CKE          LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI014_CKE          LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock\n
 *         KBI0_CKE      KBI015_CKE          LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_ALL
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE1_KBI0_CHANNEL_15
 * @retval State of KBI0 Channels(1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_KBI0_IsEnabledChannelClock(uint32_t Channels)
{
    return (READ_BIT(SYSTEM_CORE1->KBI0_CKE, Channels) == Channels);
}

/**
 * @brief  Enable KBI1 APB module clock.
 * @rmtoll KBI1_CKE      KBIC0_CKE           LL_SYSTEM_CORE1_KBI1_EnableClock
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_KBI1_EnableClock(void)
{
    SET_BIT(SYSTEM_CORE1->KBI1_CKE, SYSTEM_CORE1_KBI1_CKE_KBIC1);
}

/**
 * @brief  Disable KBI1 APB module clock.
 * @rmtoll KBI1_CKE      KBIC0_CKE           LL_SYSTEM_CORE1_KBI1_DisableClock\
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_KBI1_DisableClock(void)
{
    CLEAR_BIT(SYSTEM_CORE1->KBI1_CKE, SYSTEM_CORE1_KBI1_CKE_KBIC1);
}

/**
 * @brief  Check if KBI1 APB module clock is enabled or not
 * @rmtoll KBI1_CKE      KBIC0_CKE           LL_SYSTEM_CORE1_KBI1_IsEnabledClock
 * @retval State of KBI1 Periph (1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_KBI1_IsEnabledClock(void)
{
    return (READ_BIT(SYSTEM_CORE1->KBI1_CKE, SYSTEM_CORE1_KBI1_CKE_KBIC1) == SYSTEM_CORE1_KBI1_CKE_KBIC1);
}

/**
 * @brief  Enable KBI1 channels clock.
 * @rmtoll KBI1_CKE      KBI10_CKE           LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI11_CKE           LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI12_CKE           LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI13_CKE           LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI14_CKE           LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI15_CKE           LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI16_CKE           LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI17_CKE           LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI18_CKE           LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI19_CKE           LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI110_CKE          LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI111_CKE          LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI112_CKE          LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI113_CKE          LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI114_CKE          LL_SYSTEM_CORE1_KBI1_EnableChannelClock\n
 *         KBI1_CKE      KBI115_CKE          LL_SYSTEM_CORE1_KBI1_EnableChannelClock
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_ALL
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_15
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_KBI1_EnableChannelClock(uint32_t Channels)
{
    SET_BIT(SYSTEM_CORE1->KBI1_CKE, Channels);
}

/**
 * @brief  Disable KBI1 channels clock.
 * @rmtoll KBI1_CKE      KBI10_CKE           LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI11_CKE           LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI12_CKE           LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI13_CKE           LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI14_CKE           LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI15_CKE           LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI16_CKE           LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI17_CKE           LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI18_CKE           LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI19_CKE           LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI110_CKE          LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI111_CKE          LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI112_CKE          LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI113_CKE          LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI114_CKE          LL_SYSTEM_CORE1_KBI1_DisableChannelClock\n
 *         KBI1_CKE      KBI115_CKE          LL_SYSTEM_CORE1_KBI1_DisableChannelClock
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_ALL
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_15
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_KBI1_DisableChannelClock(uint32_t Channels)
{
    CLEAR_BIT(SYSTEM_CORE1->KBI1_CKE, Channels);
}

/**
 * @brief  Check if KBI1 channels clock is enabled or not
 * @rmtoll KBI1_CKE      KBI10_CKE           LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI11_CKE           LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI12_CKE           LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI13_CKE           LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI14_CKE           LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI15_CKE           LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI16_CKE           LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI17_CKE           LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI18_CKE           LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI19_CKE           LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI110_CKE          LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI111_CKE          LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI112_CKE          LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI113_CKE          LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI114_CKE          LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock\n
 *         KBI1_CKE      KBI115_CKE          LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_ALL
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_0
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_1
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_2
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_3
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_4
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_5
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_6
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_7
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_8
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_9
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_10
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_11
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_12
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_13
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_14
 *         @arg @ref LL_SYSTEM_CORE1_KBI1_CHANNEL_15
 * @retval State of KBI1 Channels(1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_KBI1_IsEnabledChannelClock(uint32_t Channels)
{
    return (READ_BIT(SYSTEM_CORE1->KBI1_CKE, Channels) == Channels);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EF_MOD0 MOD0
 * @{
 */

/**
 * @brief  Enable MOD0 peripherals clock.
 * @rmtoll MOD0_CKE      UART0_CKE           LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      UART1_CKE           LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      UART2_CKE           LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      UART3_CKE           LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      UART4_CKE           LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      UART5_CKE           LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      UART6_CKE           LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      UART7_CKE           LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      LPUART_CKE          LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      USART0_CKE          LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      USART1_CKE          LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      ISO78160_CKE        LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      ISO78161_CKE        LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      SPI0_CKE            LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      SPI1_CKE            LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      SPI2_CKE            LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      SPI3_CKE            LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      I2C0_CKE            LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      I2C1_CKE            LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      CAN_CKE             LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      IOCNT_CKE           LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      LCD_CKE             LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      DSP_CKE             LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      M2M_CKE             LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      D2F_CKE             LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      ADC0_CKE            LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      DAC_CKE             LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      TC0_CKE             LL_SYSTEM_CORE1_MOD0_EnableClock\n
 *         MOD0_CKE      TC1_CKE             LL_SYSTEM_CORE1_MOD0_EnableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART2
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART3
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART4
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART5
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART6
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART7
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_LPUART
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_USART0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_USART1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78160
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78161
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI2
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI3
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_I2C0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_I2C1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_CAN
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_IOCNT
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_LCD
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_DSP
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_M2M
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_D2F
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ADC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_DAC
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_TC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_TC1
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_MOD0_EnableClock(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE1->MOD0_CKE, Periphs);
}

/**
 * @brief  Disable MOD0 peripherals clock.
 * @rmtoll MOD0_CKE      UART0_CKE           LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      UART1_CKE           LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      UART2_CKE           LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      UART3_CKE           LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      UART4_CKE           LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      UART5_CKE           LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      UART6_CKE           LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      UART7_CKE           LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      LPUART_CKE          LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      USART0_CKE          LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      USART1_CKE          LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      ISO78160_CKE        LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      ISO78161_CKE        LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      SPI0_CKE            LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      SPI1_CKE            LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      SPI2_CKE            LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      SPI3_CKE            LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      I2C0_CKE            LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      I2C1_CKE            LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      CAN_CKE             LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      IOCNT_CKE           LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      LCD_CKE             LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      DSP_CKE             LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      M2M_CKE             LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      D2F_CKE             LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      ADC0_CKE            LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      DAC_CKE             LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      TC0_CKE             LL_SYSTEM_CORE1_MOD0_DisableClock\n
 *         MOD0_CKE      TC1_CKE             LL_SYSTEM_CORE1_MOD0_DisableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART2
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART3
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART4
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART5
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART6
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART7
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_LPUART
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_USART0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_USART1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78160
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78161
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI2
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI3
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_I2C0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_I2C1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_CAN
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_IOCNT
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_LCD
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_DSP
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_M2M
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_D2F
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ADC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_DAC
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_TC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_TC1
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_MOD0_DisableClock(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE1->MOD0_CKE, Periphs);
}

/**
 * @brief  Check if MOD0 peripheral clock is enabled or not
 * @rmtoll MOD0_CKE      UART0_CKE           LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      UART1_CKE           LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      UART2_CKE           LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      UART3_CKE           LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      UART4_CKE           LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      UART5_CKE           LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      UART6_CKE           LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      UART7_CKE           LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      LPUART_CKE          LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      USART0_CKE          LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      USART1_CKE          LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      ISO78160_CKE        LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      ISO78161_CKE        LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      SPI0_CKE            LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      SPI1_CKE            LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      SPI2_CKE            LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      SPI3_CKE            LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      I2C0_CKE            LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      I2C1_CKE            LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      CAN_CKE             LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      IOCNT_CKE           LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      LCD_CKE             LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      DSP_CKE             LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      M2M_CKE             LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      D2F_CKE             LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      ADC0_CKE            LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      DAC_CKE             LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      TC0_CKE             LL_SYSTEM_CORE1_MOD0_IsEnabledClock\n
 *         MOD0_CKE      TC1_CKE             LL_SYSTEM_CORE1_MOD0_IsEnabledClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART2
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART3
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART4
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART5
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART6
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART7
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_LPUART
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_USART0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_USART1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78160
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78161
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI2
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI3
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_I2C0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_I2C1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_CAN
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_IOCNT
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_LCD
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_DSP
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_M2M
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_D2F
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ADC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_DAC
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_TC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_TC1
 * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_MOD0_IsEnabledClock(uint32_t Periphs)
{
    return (READ_BIT(SYSTEM_CORE1->MOD0_CKE, Periphs) == Periphs);
}

/**
 * @brief  Force MOD0 peripherals reset.
 * @note   For all module-level resets, first disable the module clock,
 *         then force the reset, then release the module reset,
 *         finally enable the module clock for normal operation.
 * @rmtoll MOD0_RST      UART0_RST           LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      UART1_RST           LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      UART2_RST           LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      UART3_RST           LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      UART4_RST           LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      UART5_RST           LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      UART6_RST           LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      UART7_RST           LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      LPUART_RST          LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      USART0_RST          LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      USART1_RST          LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      ISO78160_RST        LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      ISO78161_RST        LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      SPI0_RST            LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      SPI1_RST            LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      SPI2_RST            LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      SPI3_RST            LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      I2C0_RST            LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      I2C1_RST            LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      CAN_RST             LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      IOCNT_RST           LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      LCD_RST             LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      DSP_RST             LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      M2M_RST             LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      D2F_RST             LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      ADC0_RST            LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      DAC_RST             LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      TC0_RST             LL_SYSTEM_CORE1_MOD0_ForceReset\n
 *         MOD0_RST      TC1_RST             LL_SYSTEM_CORE1_MOD0_ForceReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART2
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART3
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART4
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART5
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART6
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART7
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_LPUART
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_USART0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_USART1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78160
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78161
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI2
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI3
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_I2C0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_I2C1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_CAN
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_IOCNT
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_LCD
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_DSP
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_M2M
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_D2F
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ADC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_DAC
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_TC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_TC1
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_MOD0_ForceReset(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE1->MOD0_RST, Periphs);
}

/**
 * @brief  Release MOD0 peripherals reset.
 * @rmtoll MOD0_RST      UART0_RST           LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      UART1_RST           LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      UART2_RST           LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      UART3_RST           LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      UART4_RST           LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      UART5_RST           LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      UART6_RST           LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      UART7_RST           LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      LPUART_RST          LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      USART0_RST          LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      USART1_RST          LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      ISO78160_RST        LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      ISO78161_RST        LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      SPI0_RST            LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      SPI1_RST            LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      SPI2_RST            LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      SPI3_RST            LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      I2C0_RST            LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      I2C1_RST            LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      CAN_RST             LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      IOCNT_RST           LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      LCD_RST             LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      DSP_RST             LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      M2M_RST             LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      D2F_RST             LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      ADC0_RST            LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      DAC_RST             LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      TC0_RST             LL_SYSTEM_CORE1_MOD0_ReleaseReset\n
 *         MOD0_RST      TC1_RST             LL_SYSTEM_CORE1_MOD0_ReleaseReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART2
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART3
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART4
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART5
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART6
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_UART7
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_LPUART
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_USART0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_USART1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78160
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78161
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI2
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_SPI3
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_I2C0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_I2C1
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_CAN
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_IOCNT
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_LCD
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_DSP
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_M2M
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_D2F
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_ADC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_DAC
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_TC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD0_PERIPH_TC1
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_MOD0_ReleaseReset(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE1->MOD0_RST, Periphs);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EF_MOD1 MOD1
 * @{
 */

/**
 * @brief  Enable MOD1 peripherals clock.
 * @rmtoll MOD1_CKE      SIMPTC0_CKE            LL_SYSTEM_CORE1_MOD1_EnableClock\n
 *         MOD1_CKE      SIMPTC1_CKE            LL_SYSTEM_CORE1_MOD1_EnableClock\n
 *         MOD1_CKE      SIMPTC2_CKE            LL_SYSTEM_CORE1_MOD1_EnableClock\n
 *         MOD1_CKE      SIMPTC3_CKE            LL_SYSTEM_CORE1_MOD1_EnableClock\n
 *         MOD1_CKE      RS_VDD_CKE             LL_SYSTEM_CORE1_MOD1_EnableClock\n
 *         MOD1_CKE      LSRX_CKE               LL_SYSTEM_CORE1_MOD1_EnableClock\n
 *         MOD1_CKE      LVDCMP_CKE             LL_SYSTEM_CORE1_MOD1_EnableClock\n
 *         MOD1_CKE      CRC0_CKE               LL_SYSTEM_CORE1_MOD1_EnableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC1
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC2
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC3
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_RS_VDD
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_LSRX
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_LVDCMP
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_CRC0
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_MOD1_EnableClock(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE1->MOD1_CKE, Periphs);
}

/**
 * @brief  Disable MOD1 peripherals clock.
 * @rmtoll MOD1_CKE      SIMPTC0_CKE            LL_SYSTEM_CORE1_MOD1_DisableClock\n
 *         MOD1_CKE      SIMPTC1_CKE            LL_SYSTEM_CORE1_MOD1_DisableClock\n
 *         MOD1_CKE      SIMPTC2_CKE            LL_SYSTEM_CORE1_MOD1_DisableClock\n
 *         MOD1_CKE      SIMPTC3_CKE            LL_SYSTEM_CORE1_MOD1_DisableClock\n
 *         MOD1_CKE      RS_VDD_CKE             LL_SYSTEM_CORE1_MOD1_DisableClock\n
 *         MOD1_CKE      LSRX_CKE               LL_SYSTEM_CORE1_MOD1_DisableClock\n
 *         MOD1_CKE      LVDCMP_CKE             LL_SYSTEM_CORE1_MOD1_DisableClock\n
 *         MOD1_CKE      CRC0_CKE               LL_SYSTEM_CORE1_MOD1_DisableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC1
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC2
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC3
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_RS_VDD
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_LSRX
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_LVDCMP
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_CRC0
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_MOD1_DisableClock(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE1->MOD1_CKE, Periphs);
}

/**
 * @brief  Check if MOD1 peripheral clock is enabled or not
 * @rmtoll MOD1_CKE      SIMPTC0_CKE            LL_SYSTEM_CORE1_MOD1_IsEnabledClock\n
 *         MOD1_CKE      SIMPTC1_CKE            LL_SYSTEM_CORE1_MOD1_IsEnabledClock\n
 *         MOD1_CKE      SIMPTC2_CKE            LL_SYSTEM_CORE1_MOD1_IsEnabledClock\n
 *         MOD1_CKE      SIMPTC3_CKE            LL_SYSTEM_CORE1_MOD1_IsEnabledClock\n
 *         MOD1_CKE      RS_VDD_CKE             LL_SYSTEM_CORE1_MOD1_IsEnabledClock\n
 *         MOD1_CKE      LSRX_CKE               LL_SYSTEM_CORE1_MOD1_IsEnabledClock\n
 *         MOD1_CKE      LVDCMP_CKE             LL_SYSTEM_CORE1_MOD1_IsEnabledClock\n
 *         MOD1_CKE      CRC0_CKE               LL_SYSTEM_CORE1_MOD1_IsEnabledClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC1
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC2
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC3
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_RS_VDD
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_LSRX
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_LVDCMP
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_CRC0
 * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_MOD1_IsEnabledClock(uint32_t Periphs)
{
    return (READ_BIT(SYSTEM_CORE1->MOD1_CKE, Periphs) == Periphs);
}

/**
 * @brief  Force MOD1 peripherals reset.
 * @note   For all module-level resets, first disable the module clock,
 *         then force the reset, then release the module reset,
 *         finally enable the module clock for normal operation.
 * @rmtoll MOD1_RST      SIMPTC0_RST            LL_SYSTEM_CORE1_MOD1_ForceReset\n
 *         MOD1_RST      SIMPTC1_RST            LL_SYSTEM_CORE1_MOD1_ForceReset\n
 *         MOD1_RST      SIMPTC2_RST            LL_SYSTEM_CORE1_MOD1_ForceReset\n
 *         MOD1_RST      SIMPTC3_RST            LL_SYSTEM_CORE1_MOD1_ForceReset\n
 *         MOD1_RST      RS_VDD_RST             LL_SYSTEM_CORE1_MOD1_ForceReset\n
 *         MOD1_RST      LSRX_RST               LL_SYSTEM_CORE1_MOD1_ForceReset\n
 *         MOD1_RST      KBI0_RST               LL_SYSTEM_CORE1_MOD1_ForceReset\n
 *         MOD1_RST      KBI1_RST               LL_SYSTEM_CORE1_MOD1_ForceReset\n
 *         MOD1_RST      LVDCMP_RST             LL_SYSTEM_CORE1_MOD1_ForceReset\n
 *         MOD1_RST      CRC0_RST               LL_SYSTEM_CORE1_MOD1_ForceReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC1
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC2
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC3
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_RS_VDD
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_LSRX
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_KBI0
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_KBI1
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_LVDCMP
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_CRC0
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_MOD1_ForceReset(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE1->MOD1_RST, Periphs);
}

/**
 * @brief  Release MOD1 peripherals reset.
 * @rmtoll MOD1_RST      SIMPTC0_RST            LL_SYSTEM_CORE1_MOD1_ReleaseReset\n
 *         MOD1_RST      SIMPTC1_RST            LL_SYSTEM_CORE1_MOD1_ReleaseReset\n
 *         MOD1_RST      SIMPTC2_RST            LL_SYSTEM_CORE1_MOD1_ReleaseReset\n
 *         MOD1_RST      SIMPTC3_RST            LL_SYSTEM_CORE1_MOD1_ReleaseReset\n
 *         MOD1_RST      RS_VDD_RST             LL_SYSTEM_CORE1_MOD1_ReleaseReset\n
 *         MOD1_RST      LSRX_RST               LL_SYSTEM_CORE1_MOD1_ReleaseReset\n
 *         MOD1_RST      KBI0_RST               LL_SYSTEM_CORE1_MOD1_ReleaseReset\n
 *         MOD1_RST      KBI1_RST               LL_SYSTEM_CORE1_MOD1_ReleaseReset\n
 *         MOD1_RST      LVDCMP_RST             LL_SYSTEM_CORE1_MOD1_ReleaseReset\n
 *         MOD1_RST      CRC0_RST               LL_SYSTEM_CORE1_MOD1_ReleaseReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC0
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC1
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC2
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC3
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_RS_VDD
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_LSRX
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_KBI0
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_KBI1
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_LVDCMP
 *         @arg @ref LL_SYSTEM_CORE1_MOD1_PERIPH_CRC0
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_MOD1_ReleaseReset(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE1->MOD1_RST, Periphs);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EF_MOD2 MOD2
 * @{
 */

/**
 * @brief  Enable MOD2 peripherals clock.
 * @rmtoll MOD2_CKE      QSPI0_CKE           LL_SYSTEM_CORE1_MOD2_EnableClock\n
 *         MOD2_CKE      QSPI1_CKE           LL_SYSTEM_CORE1_MOD2_EnableClock\n
 *         MOD2_CKE      SEA_BUS_CKE         LL_SYSTEM_CORE1_MOD2_EnableClock\n
 *         MOD2_CKE      SEA_PKE_CKE         LL_SYSTEM_CORE1_MOD2_EnableClock\n
 *         MOD2_CKE      SEA_AES_CKE         LL_SYSTEM_CORE1_MOD2_EnableClock\n
 *         MOD2_CKE      SEA_TRNG_CKE        LL_SYSTEM_CORE1_MOD2_EnableClock\n
 *         MOD2_CKE      SEA_SHA_CKE         LL_SYSTEM_CORE1_MOD2_EnableClock\n
 *         MOD2_CKE      KEY_STORE_CKE       LL_SYSTEM_CORE1_MOD2_EnableClock\n
 *         MOD2_CKE      AES_CKE             LL_SYSTEM_CORE1_MOD2_EnableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI0
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI1
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_BUS
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_PKE
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_AES
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_TRNG
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_SHA
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_KEY_STORE
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_AES
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_MOD2_EnableClock(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE1->MOD2_CKE, Periphs);
}

/**
 * @brief  Disable MOD2 peripherals clock.
 * @rmtoll MOD2_CKE      QSPI0_CKE           LL_SYSTEM_CORE1_MOD2_DisableClock\n
 *         MOD2_CKE      QSPI1_CKE           LL_SYSTEM_CORE1_MOD2_DisableClock\n
 *         MOD2_CKE      SEA_BUS_CKE         LL_SYSTEM_CORE1_MOD2_DisableClock\n
 *         MOD2_CKE      SEA_PKE_CKE         LL_SYSTEM_CORE1_MOD2_DisableClock\n
 *         MOD2_CKE      SEA_AES_CKE         LL_SYSTEM_CORE1_MOD2_DisableClock\n
 *         MOD2_CKE      SEA_TRNG_CKE        LL_SYSTEM_CORE1_MOD2_DisableClock\n
 *         MOD2_CKE      SEA_SHA_CKE         LL_SYSTEM_CORE1_MOD2_DisableClock\n
 *         MOD2_CKE      KEY_STORE_CKE       LL_SYSTEM_CORE1_MOD2_DisableClock\n
 *         MOD2_CKE      AES_CKE             LL_SYSTEM_CORE1_MOD2_DisableClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI0
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI1
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_BUS
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_PKE
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_AES
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_TRNG
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_SHA
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_KEY_STORE
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_AES
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_MOD2_DisableClock(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE1->MOD2_CKE, Periphs);
}

/**
 * @brief  Check if MOD2 peripheral clock is enabled or not
 * @rmtoll MOD2_CKE      QSPI0_CKE           LL_SYSTEM_CORE1_MOD2_IsEnabledClock\n
 *         MOD2_CKE      QSPI1_CKE           LL_SYSTEM_CORE1_MOD2_IsEnabledClock\n
 *         MOD2_CKE      SEA_BUS_CKE         LL_SYSTEM_CORE1_MOD2_IsEnabledClock\n
 *         MOD2_CKE      SEA_PKE_CKE         LL_SYSTEM_CORE1_MOD2_IsEnabledClock\n
 *         MOD2_CKE      SEA_AES_CKE         LL_SYSTEM_CORE1_MOD2_IsEnabledClock\n
 *         MOD2_CKE      SEA_TRNG_CKE        LL_SYSTEM_CORE1_MOD2_IsEnabledClock\n
 *         MOD2_CKE      SEA_SHA_CKE         LL_SYSTEM_CORE1_MOD2_IsEnabledClock\n
 *         MOD2_CKE      KEY_STORE_CKE       LL_SYSTEM_CORE1_MOD2_IsEnabledClock\n
 *         MOD2_CKE      AES_CKE             LL_SYSTEM_CORE1_MOD2_IsEnabledClock
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI0
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI1
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_BUS
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_PKE
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_AES
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_TRNG
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_SHA
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_KEY_STORE
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_AES
 * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_MOD2_IsEnabledClock(uint32_t Periphs)
{
    return (READ_BIT(SYSTEM_CORE1->MOD2_CKE, Periphs) == Periphs);
}

/**
 * @brief  Force MOD2 peripherals reset.
 * @note   For all module-level resets, first disable the module clock,
 *         then force the reset, then release the module reset,
 *         finally enable the module clock for normal operation.
 * @rmtoll MOD2_RST      QSPI0_RST           LL_SYSTEM_CORE1_MOD2_ForceReset\n
 *         MOD2_RST      QSPI1_RST           LL_SYSTEM_CORE1_MOD2_ForceReset\n
 *         MOD2_RST      SEA_BUS_RST         LL_SYSTEM_CORE1_MOD2_ForceReset\n
 *         MOD2_RST      AES_RST             LL_SYSTEM_CORE1_MOD2_ForceReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI0
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI1
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_BUS
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_AES
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_MOD2_ForceReset(uint32_t Periphs)
{
    SET_BIT(SYSTEM_CORE1->MOD2_RST, Periphs);
}

/**
 * @brief  Release MOD2 peripherals reset.
 * @rmtoll MOD2_RST      QSPI0_RST           LL_SYSTEM_CORE1_MOD2_ReleaseReset\n
 *         MOD2_RST      QSPI1_RST           LL_SYSTEM_CORE1_MOD2_ReleaseReset\n
 *         MOD2_RST      SEA_BUS_RST         LL_SYSTEM_CORE1_MOD2_ReleaseReset\n
 *         MOD2_RST      AES_RST             LL_SYSTEM_CORE1_MOD2_ReleaseReset
 * @param  Periphs This parameter can be a combination of the following values:
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_ALL
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI0
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI1
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_BUS
 *         @arg @ref LL_SYSTEM_CORE1_MOD2_PERIPH_AES
 * @retval None
*/
__STATIC_INLINE void LL_SYSTEM_CORE1_MOD2_ReleaseReset(uint32_t Periphs)
{
    CLEAR_BIT(SYSTEM_CORE1->MOD2_RST, Periphs);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EF_IT_MANAGEMENT IT_Management
 * @{
 */

/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EF_RESET_IT_MANAGEMENT Reset IT_Management
 * @{
 */

/**
 * @brief  Enable BOI_VBAT_RST Interrupt.
 * @rmtoll BOI_RST_CTRL          BOI_VBAT_RST_IE       LL_SYSTEM_CORE1_EnableIT_BOIVBATRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_EnableIT_BOIVBATRST(void)
{
    __IO uint32_t tmpreg;
    tmpreg = READ_BIT(SYSTEM_CORE1->BOI_RST_CTRL, SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_IE);
    WRITE_REG(SYSTEM_CORE1->BOI_RST_CTRL, SYSTEM_CORE1_BOI_RST_CTRL_BOI_VBAT_RST_IE | tmpreg);
}

/**
 * @brief  Disable BOI_VBAT_RST Interrupt.
 * @rmtoll BOI_RST_CTRL          BOI_VBAT_RST_IE       LL_SYSTEM_CORE1_DisableIT_BOIVBATRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_DisableIT_BOIVBATRST(void)
{
    __IO uint32_t tmpreg;
    tmpreg = READ_BIT(SYSTEM_CORE1->BOI_RST_CTRL, SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_IE);
    WRITE_REG(SYSTEM_CORE1->BOI_RST_CTRL, ((~SYSTEM_CORE1_BOI_RST_CTRL_BOI_VBAT_RST_IE) & SYSTEM_CORE1_BOI_RST_CTRL_BOI_VBAT_RST_IE) | tmpreg);
}

/**
 * @brief  Check if BOI_VBAT_RST Interrupt are enabled or disabled.
 * @rmtoll BOI_RST_CTRL          BOI_VBAT_RST_IE       LL_SYSTEM_CORE1_IsEnabledIT_BOIVBATRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsEnabledIT_BOIVBATRST(void)
{
    return (READ_BIT(SYSTEM_CORE1->BOI_RST_CTRL, SYSTEM_CORE1_BOI_RST_CTRL_BOI_VBAT_RST_IE) == (SYSTEM_CORE1_BOI_RST_CTRL_BOI_VBAT_RST_IE));
}

/**
 * @brief  Enable CORE1_RST Interrupt.
 * @rmtoll BOI_RST_CTRL          CORE1_RST_IE       LL_SYSTEM_CORE1_EnableIT_CORE1RST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_EnableIT_CORE1RST(void)
{
    __IO uint32_t tmpreg;
    tmpreg = READ_BIT(SYSTEM_CORE1->BOI_RST_CTRL, SYSTEM_CORE1_BOI_RST_CTRL_BOI_VBAT_RST_IE);
    WRITE_REG(SYSTEM_CORE1->BOI_RST_CTRL, SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_IE | tmpreg);
}

/**
 * @brief  Disable CORE1_RST Interrupt.
 * @rmtoll BOI_RST_CTRL          CORE1_RST_IE       LL_SYSTEM_CORE1_DisableIT_CORE1RST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_DisableIT_CORE1RST(void)
{
    uint32_t tmpreg;
    tmpreg = READ_BIT(SYSTEM_CORE1->BOI_RST_CTRL, SYSTEM_CORE1_BOI_RST_CTRL_BOI_VBAT_RST_IE);
    WRITE_REG(SYSTEM_CORE1->BOI_RST_CTRL, ((~SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_IE) & SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_IE) | tmpreg);
}

/**
 * @brief  Check if CORE1_RST Interrupt are enabled or disabled.
 * @rmtoll BOI_RST_CTRL          CORE1_RST_IE       LL_SYSTEM_CORE1_IsEnabledIT_CORE1RST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsEnabledIT_CORE1RST(void)
{
    return (READ_BIT(SYSTEM_CORE1->BOI_RST_CTRL, SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_IE) == (SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_IE));
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EF_RESET_FLAG_management Reset FLAG_management
 * @{
 */

/**
 * @brief  Indicate the status of POWEROK Reset Flag.
 * @rmtoll SYS_RST_FLAG          POWEROK_RST_FLAG          LL_SYSTEM_CORE1_IsActiveFlag_POWEROKRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsActiveFlag_POWEROKRST(void)
{
    return (READ_BIT(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_POWEROK) == (SYSTEM_CORE1_SYS_RST_FLAG_POWEROK));
}

/**
 * @brief  Clear POWEROK Reset Flag.
 * @rmtoll SYS_RST_FLAG          POWEROK_RST_FLAG          LL_SYSTEM_CORE1_ClearFlag_POWEROKRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_ClearFlag_POWEROKRST(void)
{
    WRITE_REG(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_POWEROK);
}

/**
 * @brief  Indicate the status of PIN Reset Flag.
 * @rmtoll SYS_RST_FLAG          PIN_RST_FLAG          LL_SYSTEM_CORE1_IsActiveFlag_PINRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsActiveFlag_PINRST(void)
{
    return (READ_BIT(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_PIN) == (SYSTEM_CORE1_SYS_RST_FLAG_PIN));
}

/**
 * @brief  Clear PIN Reset Flag.
 * @rmtoll SYS_RST_FLAG          PIN_RST_FLAG          LL_SYSTEM_CORE1_ClearFlag_PINRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_ClearFlag_PINRST(void)
{
    WRITE_REG(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_PIN);
}

/**
 * @brief  Indicate the status of CORE0 WDT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE0_WDT_RST_FLAG          LL_SYSTEM_CORE1_IsActiveFlag_CORE0WDTRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsActiveFlag_CORE0WDTRST(void)
{
    return (READ_BIT(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_CORE0_WDT) == (SYSTEM_CORE1_SYS_RST_FLAG_CORE0_WDT));
}

/**
 * @brief  Clear CORE0 WDT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE0_WDT_RST_FLAG          LL_SYSTEM_CORE1_ClearFlag_CORE0WDTRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_ClearFlag_CORE0WDTRST(void)
{
    WRITE_REG(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_CORE0_WDT);
}

/**
 * @brief  Indicate the status of CORE1T0 WDT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE1T0_WDT_RST_FLAG          LL_SYSTEM_CORE1_IsActiveFlag_CORE1T0WDTRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsActiveFlag_CORE1T0WDTRST(void)
{
    return (READ_BIT(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_CORE1T0_WDT) == (SYSTEM_CORE1_SYS_RST_FLAG_CORE1T0_WDT));
}

/**
 * @brief  Clear CORE1T0 WDT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE1T0_WDT_RST_FLAG          LL_SYSTEM_CORE1_ClearFlag_CORE1T0WDTRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_ClearFlag_CORE1T0WDTRST(void)
{
    WRITE_REG(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_CORE1T0_WDT);
}

/**
 * @brief  Indicate the status of CORE0_SOFT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE0_SOFT_RST_FLAG          LL_SYSTEM_CORE1_IsActiveFlag_CORE0SOFTRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsActiveFlag_CORE0SOFTRST(void)
{
    return (READ_BIT(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_CORE0_SOFT) == (SYSTEM_CORE1_SYS_RST_FLAG_CORE0_SOFT));
}

/**
 * @brief  Clear CORE0_SOFT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE0_SOFT_RST_FLAG          LL_SYSTEM_CORE1_ClearFlag_CORE0SOFTRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_ClearFlag_CORE0SOFTRST(void)
{
    WRITE_REG(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_CORE0_SOFT);
}

/**
 * @brief  Indicate the status of CORE1T0_SOFT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE1T0_SOFT_RST_FLAG          LL_SYSTEM_CORE1_IsActiveFlag_CORE1T0SOFTRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsActiveFlag_CORE1T0SOFTRST(void)
{
    return (READ_BIT(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_CORE1T0_SOFT) == (SYSTEM_CORE1_SYS_RST_FLAG_CORE1T0_SOFT));
}

/**
 * @brief  Clear CORE1T0_SOFT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE1T0_SOFT_RST_FLAG          LL_SYSTEM_CORE1_ClearFlag_CORE1T0SOFTRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_ClearFlag_CORE1T0SOFTRST(void)
{
    WRITE_REG(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_CORE1T0_SOFT);
}

/**
 * @brief  Indicate the status of VBAT BOI Power-Down Reset Flag.
 * @note   When using this flag, after initial power-on,
 *         software must first clear this flag before application.
 * @rmtoll SYS_RST_FLAG          BOI_VBAT_RST_FLAG          LL_SYSTEM_CORE1_IsActiveFlag_BOIVBATRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsActiveFlag_BOIVBATRST(void)
{
    return (READ_BIT(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_BOI_VBAT) == (SYSTEM_CORE1_SYS_RST_FLAG_BOI_VBAT));
}

/**
 * @brief  Clear VBAT BOI Power-Down Reset Flag.
 * @rmtoll SYS_RST_FLAG          BOI_VBAT_RST_FLAG          LL_SYSTEM_CORE1_ClearFlag_BOIVBATRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_ClearFlag_BOIVBATRST(void)
{
    WRITE_REG(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_BOI_VBAT);
}

/**
 * @brief  Indicate the status of CORE1_WDT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE1_WDT_RST_FLAG          LL_SYSTEM_CORE1_IsActiveFlag_CORE1WDTRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsActiveFlag_CORE1WDTRST(void)
{
    return (READ_BIT(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_CORE1_WDT) == (SYSTEM_CORE1_SYS_RST_FLAG_CORE1_WDT));
}

/**
 * @brief  Clear CORE1_WDT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE1_WDT_RST_FLAG          LL_SYSTEM_CORE1_ClearFlag_CORE1WDTRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_ClearFlag_CORE1WDTRST(void)
{
    WRITE_REG(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_CORE1_WDT);
}

/**
 * @brief  Indicate the status of CORE1_SOFT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE1_SOFT_RST_FLAG          LL_SYSTEM_CORE1_IsActiveFlag_CORE1SOFTRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsActiveFlag_CORE1SOFTRST(void)
{
    return (READ_BIT(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_CORE1_SOFT) == (SYSTEM_CORE1_SYS_RST_FLAG_CORE1_SOFT));
}

/**
 * @brief  Clear CORE1_SOFT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE1_SOFT_RST_FLAG          LL_SYSTEM_CORE1_ClearFlag_CORE1SOFTRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_ClearFlag_CORE1SOFTRST(void)
{
    WRITE_REG(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_CORE1_SOFT);
}

/**
 * @brief  Indicate the status of CORE0T1_SOFT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE0T1_SOFT_RST_FLAG          LL_SYSTEM_CORE1_IsActiveFlag_CORE0T1SOFTRST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsActiveFlag_CORE0T1SOFTRST(void)
{
    return (READ_BIT(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_CORE0T1_SOFT) == (SYSTEM_CORE1_SYS_RST_FLAG_CORE0T1_SOFT));
}

/**
 * @brief  Clear CORE0T1_SOFT Reset Flag.
 * @rmtoll SYS_RST_FLAG          CORE0T1_SOFT_RST_FLAG          LL_SYSTEM_CORE1_ClearFlag_CORE0T1SOFTRST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_ClearFlag_CORE0T1SOFTRST(void)
{
    WRITE_REG(SYSTEM_CORE1->SYS_RST_FLAG, SYSTEM_CORE1_SYS_RST_FLAG_CORE0T1_SOFT);
}

/**
 * @brief  Indicate the status of CORE1_RST Reset Flag.
 * @note   This bit can be reset by all reset sources except the CORE0 update reset itself.
 * @rmtoll BOI_RST_CTRL          CORE1_RST_PENDING          LL_SYSTEM_CORE1_IsActiveFlag_CORE1RST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsActiveFlag_CORE1RST(void)
{
    return (READ_BIT(SYSTEM_CORE1->BOI_RST_CTRL, SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_PENDING) == (SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_PENDING));
}

/**
 * @brief  Clear CORE0 Update Reset Flag.
 * @rmtoll BOI_RST_CTRL          CORE1_RST_PENDING          LL_SYSTEM_CORE1_ClearFlag_CORE1RST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_ClearFlag_CORE1RST(void)
{
    SET_BIT(SYSTEM_CORE1->BOI_RST_CTRL, SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_PENDING);
}

/**
 * @brief  Indicate the status of CORE0 Update Reset Flag.
 * @note   This bit can be reset by all reset sources except the CORE0 update reset itself.
 * @rmtoll UPDATE_RST_CTRL          CORE1_RST_FLAG          LL_SYSTEM_CORE1_IsActiveFlag_UPDATERST
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsActiveFlag_UPDATERST(void)
{
    return (READ_BIT(SYSTEM_CORE1->UPDATE_RST_CTRL, SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_FLAG) == (SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_FLAG));
}

/**
 * @brief  Clear CORE0 Update Reset Flag.
 * @rmtoll UPDATE_RST_CTRL          CORE1_RST_FLAG          LL_SYSTEM_CORE1_ClearFlag_UPDATERST
 * @retval None
 */
__STATIC_INLINE void LL_SYSTEM_CORE1_ClearFlag_UPDATERST(void)
{
    SET_BIT(SYSTEM_CORE1->UPDATE_RST_CTRL, SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_FLAG);
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EF_FLAG_management FLAG_management
 * @{
 */

/**
 * @brief  Indicate the status of Flash Busy flag (read only).
 * @rmtoll SYS_MODE          FLASH_BUSY          LL_SYSTEM_CORE1_IsActiveFlag_FLASHBUSY
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsActiveFlag_FLASHBUSY(void)
{
    return (READ_BIT(SYSTEM_CORE1->SYS_MODE, SYSTEM_CORE1_SYS_MODE_FLASH_BUSY) == (SYSTEM_CORE1_SYS_MODE_FLASH_BUSY));
}

/**
 * @brief  Indicate the status of Core1 Stop flag (read only).
 * @rmtoll CORE1_STA          CORE1_STOP_FLAG          LL_SYSTEM_CORE1_IsActiveFlag_CORE1STOP
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SYSTEM_CORE1_IsActiveFlag_CORE1STOP(void)
{
    return (READ_BIT(SYSTEM_CORE1->CORE1_STA, SYSTEM_CORE1_CORE1_STA_CORE1_STOP_FLAG) == (SYSTEM_CORE1_CORE1_STA_CORE1_STOP_FLAG));
}

/**
 * @}
 */

/** @defgroup SYSTEM_CORE1_LL_EF_Init Initialization and de-initialization functions
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

#endif /* __RN_LL_SYSTEM_CORE1_H */
