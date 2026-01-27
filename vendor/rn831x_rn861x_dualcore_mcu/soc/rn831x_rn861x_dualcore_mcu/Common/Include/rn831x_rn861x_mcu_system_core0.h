/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_system_core0.h
 * @author  Renergy Software Team
 * @brief   SYSTEM_CORE0 module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_SYSTEM_CORE0_H
#define _RN831x_RN861x_MCU_SYSTEM_CORE0_H

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

/* SYSTEM_CORE0 */
typedef struct
{
    __IO uint32_t SYS_PSW1;                  /*!< offset: 0x000 info: Public Password Register */
    __IO uint32_t RESERVED0[63];             /*!< offset: 0x004 ~ 0FC */
    __IO uint32_t RCH_CTRL;                  /*!< offset: 0x100 info: RCH Control Register */
    __IO uint32_t PLL_CTRL;                  /*!< offset: 0x104 info: PLL Control Register */
    __IO uint32_t HOSC_CTRL;                 /*!< offset: 0x108 info: HOSC Control Register */
    __IO uint32_t RCL307K_CTRL;              /*!< offset: 0x10C info: RCL307K Control Register */
    __IO uint32_t RESERVED1[4];              /*!< offset: 0x110 ~ 11C */
    __IO uint32_t CLK_SEL;                   /*!< offset: 0x120 info: Clock Selection Register */
    __IO uint32_t SYS_MODE;                  /*!< offset: 0x124 info: System Mode Register */
    __IO uint32_t CORE_DIV;                  /*!< offset: 0x128 info: CORE Clock Division Register */
    __IO uint32_t PMOD0_CKE;                 /*!< offset: 0x12C info: Private Module Clock Enable Register */
    __IO uint32_t CORE1_CKE;                 /*!< offset: 0x130 info: Core1 Clock Enable Register */
    __IO uint32_t KBI0_CKE;                  /*!< offset: 0x134 info: KBI0 Clock Enable Register */
    __IO uint32_t KBI1_CKE;                  /*!< offset: 0x138 info: KBI1 Clock Enable Register */
    __IO uint32_t MOD0_CKE;                  /*!< offset: 0x13C info: Module Clock Enable Register 0 */
    __IO uint32_t MOD1_CKE;                  /*!< offset: 0x140 info: Module Clock Enable Register 1 */
    __IO uint32_t MOD2_CKE;                  /*!< offset: 0x144 info: Module Clock Enable Register 2 */
    __IO uint32_t RESERVED2[10];             /*!< offset: 0x148 ~ 16C */
    __IO uint32_t SYSFREQ_STA;               /*!< offset: 0x170 info: System Clock Frequency Indication Register */
    __IO uint32_t SYSCLK_STA;                /*!< offset: 0x174 info: System Clock Status Register */
    __IO uint32_t RESERVED3[34];             /*!< offset: 0x178 ~ 1FC */
    __IO uint32_t CLK_CAL_CTRL;              /*!< offset: 0x200 info: Clock Calibration Control Register */
    __IO uint32_t CLK_REF_CNT;               /*!< offset: 0x204 info: Clock Reference Count Register */
    __IO uint32_t CLK_CAL_CNT;               /*!< offset: 0x208 info: Clock Target Count Register */
    __IO uint32_t CLK_CAL_STA;               /*!< offset: 0x20C info: Clock Calibration Status Register */
    __IO uint32_t RESERVED4[60];             /*!< offset: 0x210 ~ 2FC */
    __IO uint32_t RST_CTRL;                  /*!< offset: 0x300 info: Reset Control Register */
    __IO uint32_t RST_STOP_CTRL;             /*!< offset: 0x304 info: Reset State Stop Control Register */
    __IO uint32_t RESERVED5[2];              /*!< offset: 0x308 ~ 30C */
    __IO uint32_t POW_RST_FLAG;              /*!< offset: 0x310 info: Power Reset Source Flag Register */
    __IO uint32_t SYS_RST_FLAG;              /*!< offset: 0x314 info: System Reset Flag Register */
    __IO uint32_t BOI_RST_CTRL;              /*!< offset: 0x318 info: BOI_VBAT Reset Control Register */
    __IO uint32_t UPDATE_RST_CTRL;           /*!< offset: 0x31C info: Update Reset Control Register */
    __IO uint32_t RESERVED6[4];              /*!< offset: 0x320 ~ 32C */
    __IO uint32_t PMOD0_RST;                 /*!< offset: 0x330 info: CORE0 Private Module Reset Register */
    __IO uint32_t MOD0_RST;                  /*!< offset: 0x334 info: Assignable Module 0 Reset Register */
    __IO uint32_t MOD1_RST;                  /*!< offset: 0x338 info: Assignable Module 1 Reset Register */
    __IO uint32_t MOD2_RST;                  /*!< offset: 0x33C info: Assignable Module 2 Reset Register */
    __IO uint32_t RESERVED7[48];             /*!< offset: 0x340 ~ 3FC */
    __IO uint32_t BOR5_CTRL;                 /*!< offset: 0x400 info: BOR5 Control Register */
    __IO uint32_t PLVD5_CTRL;                /*!< offset: 0x404 info: PLVD5 Control Register */
    __IO uint32_t RESERVED8[62];             /*!< offset: 0x408 ~ 4FC */
    __IO uint32_t ADDR_MAP_CTRL;             /*!< offset: 0x500 info: Address Mapping Control Register */
    __IO uint32_t FLASH_MAP_CTRL;            /*!< offset: 0x504 info: FLASH Mapping Control Register */
    __IO uint32_t DMA_PRI0_CTRL;             /*!< offset: 0x508 info: DMA Access SRAM Priority Configuration Register 0 */
    __IO uint32_t DMA_PRI1_CTRL;             /*!< offset: 0x50C info: DMA Access SRAM Priority Configuration Register 1 */
    __IO uint32_t DMA_PRI2_CTRL;             /*!< offset: 0x510 info: DMA Access SRAM Priority Configuration Register 2 */
    __IO uint32_t IRFR_CTRL;                 /*!< offset: 0x514 info: Infrared Control Register */
    __IO uint32_t DIG_REV_CTRL;              /*!< offset: 0x518 info: Reserved Digital Control Register */
    __IO uint32_t RESERVED9[185];            /*!< offset: 0x51C ~ 7FC */
    __IO uint32_t CHIP_ID;                   /*!< offset: 0x800 info: CHIP ID Register */
    __IO uint32_t DIE_VER;                   /*!< offset: 0x804 info: Chip Version Register */
    __IO uint32_t FAB_UID0;                  /*!< offset: 0x808 info: Chip Unique ID 0 Register */
    __IO uint32_t FAB_UID1;                  /*!< offset: 0x80C info: Chip Unique ID 1 Register */
} SYSTEM_CORE0_TypeDef;

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
#define SYSTEM_CORE0_BASE (0x20080000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define SYSTEM_CORE0 ((SYSTEM_CORE0_TypeDef *)SYSTEM_CORE0_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define SYSTEM  SYSTEM_CORE0
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** SYSTEM_CORE0 Instances *******************************/
#define IS_SYSTEM_CORE0_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == SYSTEM_CORE0))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* SYSTEM_CORE0_SYS_PSW1 */
#define SYSTEM_CORE0_SYS_PSW1_SYS_PSW1       (BITS(0,7))                                         /*!< When SYS_PSW1 = 8'h82, public registers are writable. */

/* SYSTEM_CORE0_RCH_CTRL */
#define SYSTEM_CORE0_RCH_CTRL_PD             (BIT(0))                                            /*!< Internal RCH enable bit, main frequency is 58.9824MHz */
#define SYSTEM_CORE0_RCH_CTRL_DIV            (BITS(1,3))                                         /*!< RCH source clock division selection */

/* SYSTEM_CORE0_PLL_CTRL */
#define SYSTEM_CORE0_PLL_CTRL_PD             (BIT(0))                                            /*!< PLL module enable bit */
#define SYSTEM_CORE0_PLL_CTRL_FREQ           (BITS(1,2))                                         /*!< PLL output frequency selection */
#define SYSTEM_CORE0_PLL_CTRL_DIV            (BITS(4,6))                                         /*!< PLL source clock division selection */
#define SYSTEM_CORE0_PLL_CTRL_LOCK           (BIT(8))                                            /*!< PLL lock status */

/* SYSTEM_CORE0_HOSC_CTRL */
#define SYSTEM_CORE0_HOSC_CTRL_PD            (BIT(0))                                            /*!< External High-Frequency Oscillator enable bit */
#define SYSTEM_CORE0_HOSC_CTRL_FREQ          (BITS(1,2))                                         /*!< External high-frequency crystal selection */
#define SYSTEM_CORE0_HOSC_CTRL_DIV           (BITS(4,6))                                         /*!< HOSC source clock division selection */
#define SYSTEM_CORE0_HOSC_CTRL_PGA           (BITS(8,10))                                        /*!< HOSC gain selection */
#define SYSTEM_CORE0_HOSC_CTRL_DSEL          (BITS(12,14))                                       /*!< HOSC analog filter time configuration */

/* SYSTEM_CORE0_RCL307K_CTRL */
#define SYSTEM_CORE0_RCL307K_CTRL_PD         (BITS(0,7))                                         /*!< RCL307K Enable Switch */

/* SYSTEM_CORE0_CLK_SEL */
#define SYSTEM_CORE0_CLK_SEL_SYS_RCL_LOSC    (BIT(0))                                            /*!< Low-frequency operation system main clock selection */
#define SYSTEM_CORE0_CLK_SEL_SYS_PLL_HOSC    (BIT(1))                                            /*!< High-frequency operation system main clock selection */
#define SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC    (BITS(2,3))                                         /*!< LCD Clock Source selection */
#define SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC   (BITS(4,5))                                         /*!< IRFR Clock Source selection */
#define SYSTEM_CORE0_CLK_SEL_KBI_RCL_LOSC    (BIT(6))                                            /*!< KBI Clock Source selection */
#define SYSTEM_CORE0_CLK_SEL_LPUART_RCL_LOSC  (BIT(7))                                            /*!< LPUART Low-Frequency Clock Source Selection */
#define SYSTEM_CORE0_CLK_SEL_VBAT_RCL        (BIT(8))                                            /*!< RCL32K clock channel switch to VBAT domain */
#define SYSTEM_CORE0_CLK_SEL_D2F_RCL_LOSC    (BIT(9))                                            /*!< D2F Low-Frequency Clock Source Selection */
#define SYSTEM_CORE0_CLK_SEL_CORE0_RCL_LOSC  (BIT(10))                                           /*!< CORE0 Low-Frequency Clock Source Selection */
#define SYSTEM_CORE0_CLK_SEL_CORE1_RCL_LOSC  (BIT(11))                                           /*!< CORE1 Low-Frequency Clock Source Selection */

/* SYSTEM_CORE0_SYS_MODE */
#define SYSTEM_CORE0_SYS_MODE_SYS_MODE       (BITS(0,3))                                         /*!< set to enter HCM/RC/LCM mode */
#define SYSTEM_CORE0_SYS_MODE_FLASH_BUSY     (BIT(4))                                            /*!< Flash busy status */

/* SYSTEM_CORE0_CORE_DIV */
#define SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV  (BITS(0,3))                                         /*!< CORE0/CORE1 main clock division configuration */

/* SYSTEM_CORE0_PMOD0_CKE */
#define SYSTEM_CORE0_PMOD0_CKE_WDT           (BIT(0))                                            /*!< CORE0_WDT APB clock enable */
#define SYSTEM_CORE0_PMOD0_CKE_MAILBOX       (BIT(1))                                            /*!< CORE0_MAILBOX module clock enable */
#define SYSTEM_CORE0_PMOD0_CKE_AOS           (BIT(2))                                            /*!< CORE0_AOS module clock enable */
#define SYSTEM_CORE0_PMOD0_CKE_SCU           (BIT(3))                                            /*!< CORE0_SCU module clock enable */
#define SYSTEM_CORE0_PMOD0_CKE_GIC           (BIT(4))                                            /*!< CORE0_GIC module clock enable */
#define SYSTEM_CORE0_PMOD0_CKE_GDMA00        (BIT(8))                                            /*!< GDMA00 module clock enable */
#define SYSTEM_CORE0_PMOD0_CKE_GDMA01        (BIT(9))                                            /*!< GDMA01 module clock enable */
#define SYSTEM_CORE0_PMOD0_CKE_UART38K       (BIT(10))                                           /*!< UART38K module clock enable */
#define SYSTEM_CORE0_PMOD0_CKE_GPIO          (BIT(11))                                           /*!< GPIO module enable */
#define SYSTEM_CORE0_PMOD0_CKE_LCSS          (BIT(12))                                           /*!< LCSS module enable */
#define SYSTEM_CORE0_PMOD0_CKE_CRC1          (BIT(14))                                           /*!< CRC1 module clock enable */

/* SYSTEM_CORE0_CORE1_CKE */
#define SYSTEM_CORE0_CORE1_CKE_CORE1_CKE     (BITS(0,15))                                        /*!< Disable/enable for CORE1 system */

/* SYSTEM_CORE0_KBI0_CKE */
#define SYSTEM_CORE0_KBI0_CKE_KBI00          (BIT(0))                                            /*!< KBI00 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI01          (BIT(1))                                            /*!< KBI01 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI02          (BIT(2))                                            /*!< KBI02 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI03          (BIT(3))                                            /*!< KBI03 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI04          (BIT(4))                                            /*!< KBI04 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI05          (BIT(5))                                            /*!< KBI05 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI06          (BIT(6))                                            /*!< KBI06 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI07          (BIT(7))                                            /*!< KBI07 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI08          (BIT(8))                                            /*!< KBI08 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI09          (BIT(9))                                            /*!< KBI09 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI010         (BIT(10))                                           /*!< KBI010 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI011         (BIT(11))                                           /*!< KBI011 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI012         (BIT(12))                                           /*!< KBI012 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI013         (BIT(13))                                           /*!< KBI013 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI014         (BIT(14))                                           /*!< KBI014 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBI015         (BIT(15))                                           /*!< KBI015 module clock enable */
#define SYSTEM_CORE0_KBI0_CKE_KBIC0          (BIT(31))                                           /*!< KBIC0 APB module clock enable */

/* SYSTEM_CORE0_KBI1_CKE */
#define SYSTEM_CORE0_KBI1_CKE_KBI10          (BIT(0))                                            /*!< KBI00 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI11          (BIT(1))                                            /*!< KBI01 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI12          (BIT(2))                                            /*!< KBI02 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI13          (BIT(3))                                            /*!< KBI03 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI14          (BIT(4))                                            /*!< KBI04 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI15          (BIT(5))                                            /*!< KBI05 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI16          (BIT(6))                                            /*!< KBI06 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI17          (BIT(7))                                            /*!< KBI07 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI18          (BIT(8))                                            /*!< KBI08 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI19          (BIT(9))                                            /*!< KBI09 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI110         (BIT(10))                                           /*!< KBI110 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI111         (BIT(11))                                           /*!< KBI111 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI112         (BIT(12))                                           /*!< KBI112 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI113         (BIT(13))                                           /*!< KBI113 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI114         (BIT(14))                                           /*!< KBI114 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBI115         (BIT(15))                                           /*!< KBI115 module clock enable */
#define SYSTEM_CORE0_KBI1_CKE_KBIC1          (BIT(31))                                           /*!< KBIC1 APB module clock enable */

/* SYSTEM_CORE0_MOD0_CKE */
#define SYSTEM_CORE0_MOD0_CKE_UART0          (BIT(0))                                            /*!< UART0 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_UART1          (BIT(1))                                            /*!< UART1 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_UART2          (BIT(2))                                            /*!< UART2 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_UART3          (BIT(3))                                            /*!< UART3 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_UART4          (BIT(4))                                            /*!< UART4 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_UART5          (BIT(5))                                            /*!< UART5 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_UART6          (BIT(6))                                            /*!< UART6 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_UART7          (BIT(7))                                            /*!< UART7 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_LPUART         (BIT(8))                                            /*!< LPUART module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_USART0         (BIT(9))                                            /*!< USART0 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_USART1         (BIT(10))                                           /*!< USART1 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_ISO78160       (BIT(11))                                           /*!< ISO7816_0 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_ISO78161       (BIT(12))                                           /*!< ISO7816_1 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_SPI0           (BIT(13))                                           /*!< SPI0 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_SPI1           (BIT(14))                                           /*!< SPI1 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_SPI2           (BIT(15))                                           /*!< SPI2 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_SPI3           (BIT(16))                                           /*!< SPI3 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_I2C0           (BIT(17))                                           /*!< I2C0 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_I2C1           (BIT(18))                                           /*!< I2C1 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_IOCNT          (BIT(20))                                           /*!< IOCNT module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_LCD            (BIT(21))                                           /*!< LCD module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_DSP            (BIT(22))                                           /*!< DSP module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_M2M            (BIT(24))                                           /*!< M2M module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_D2F            (BIT(25))                                           /*!< D2F module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_ADC0           (BIT(26))                                           /*!< ADC0 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_DAC            (BIT(27))                                           /*!< DAC module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_TC0            (BIT(28))                                           /*!< TC0 module clock enable */
#define SYSTEM_CORE0_MOD0_CKE_TC1            (BIT(29))                                           /*!< TC1 module clock enable */

/* SYSTEM_CORE0_MOD1_CKE */
#define SYSTEM_CORE0_MOD1_CKE_SIMPTC0        (BIT(0))                                            /*!< SIMP_TC0 Module Clock Enable */
#define SYSTEM_CORE0_MOD1_CKE_SIMPTC1        (BIT(1))                                            /*!< SIMP_TC1 Module Clock Enable */
#define SYSTEM_CORE0_MOD1_CKE_SIMPTC2        (BIT(2))                                            /*!< SIMP_TC2 Module Clock Enable */
#define SYSTEM_CORE0_MOD1_CKE_SIMPTC3        (BIT(3))                                            /*!< SIMP_TC3 Module Clock Enable */
#define SYSTEM_CORE0_MOD1_CKE_RS_VDD         (BIT(4))                                            /*!< RS_VDD Module Clock Enable */
#define SYSTEM_CORE0_MOD1_CKE_LSRX           (BIT(6))                                            /*!< LSRX Module Clock Enable */
#define SYSTEM_CORE0_MOD1_CKE_LVDCMP         (BIT(9))                                            /*!< LVDCMP Module Clock Enable */
#define SYSTEM_CORE0_MOD1_CKE_CRC0           (BIT(10))                                           /*!< CRC0 Module Clock Enable */

/* SYSTEM_CORE0_MOD2_CKE */
#define SYSTEM_CORE0_MOD2_CKE_QSPI0          (BIT(0))                                            /*!< QSPI0 Module Clock Enable */
#define SYSTEM_CORE0_MOD2_CKE_QSPI1          (BIT(1))                                            /*!< QSPI1 Module Clock Enable */
#define SYSTEM_CORE0_MOD2_CKE_SEA_BUS        (BIT(2))                                            /*!< SEA_BUS Module Clock Enable */
#define SYSTEM_CORE0_MOD2_CKE_SEA_PKE        (BIT(3))                                            /*!< SEA_PKE Module Clock Enable */
#define SYSTEM_CORE0_MOD2_CKE_SEA_AES        (BIT(4))                                            /*!< SEA_AES Module Clock Enable */
#define SYSTEM_CORE0_MOD2_CKE_SEA_TRNG       (BIT(5))                                            /*!< SEA_TRNG Module Clock Enable */
#define SYSTEM_CORE0_MOD2_CKE_SEA_SHA        (BIT(6))                                            /*!< SEA_SHA Module Clock Enable */
#define SYSTEM_CORE0_MOD2_CKE_KEY_STORE      (BIT(7))                                            /*!< KEY_STORE Module Clock Enable */
#define SYSTEM_CORE0_MOD2_CKE_AES            (BIT(8))                                            /*!< AES Module Clock Enable */

/* SYSTEM_CORE0_SYSFREQ_STA */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS   (BITS(0,2))                                         /*!< CORE0 System Main Clock Frequency Indication */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS   (BITS(4,6))                                         /*!< CORE1 System Main Clock Frequency Indication */
#define SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK   (BITS(8,10))                                        /*!< Flash System Frequency Indication */

/* SYSTEM_CORE0_SYSCLK_STA */
#define SYSTEM_CORE0_SYSCLK_STA_SYS_ON_LOSC_RCL  (BIT(0))                                            /*!< Set to 1 when the system runs on external low frequency (LOSC) or internal RCL32K clock */
#define SYSTEM_CORE0_SYSCLK_STA_SYS_ON_RCH   (BIT(1))                                            /*!< Set to 1 when the system runs on internal high frequency clock (RCH) */
#define SYSTEM_CORE0_SYSCLK_STA_SYS_ON_PLL_HOSC  (BIT(2))                                            /*!< Set to 1 when the system runs on external high frequency (HOSC) or internal PLL clock */
#define SYSTEM_CORE0_SYSCLK_STA_LOSC_ON_FLAG  (BIT(4))                                            /*!< LOSC Clock Enable Flag */
#define SYSTEM_CORE0_SYSCLK_STA_PLL_ON_FLAG  (BIT(5))                                            /*!< PLL Clock Enable Flag */
#define SYSTEM_CORE0_SYSCLK_STA_RCH_ON_FLAG  (BIT(6))                                            /*!< RCH Clock Enable Flag */
#define SYSTEM_CORE0_SYSCLK_STA_RCL32K_ON_FLAG  (BIT(7))                                            /*!< RCL32K Clock Enable Flag */
#define SYSTEM_CORE0_SYSCLK_STA_HOSC_ON_FLAG  (BIT(8))                                            /*!< HOSC Clock Enable Flag */
#define SYSTEM_CORE0_SYSCLK_STA_RCL307K_ON_FLAG  (BIT(9))                                            /*!< RCL307K Clock Enable Flag */

/* SYSTEM_CORE0_CLK_CAL_CTRL */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_START  (BIT(0))                                            /*!< Clock Calibration Start Bit */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_STOP   (BIT(1))                                            /*!< Clock Calibration Stop Bit */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_SEL    (BIT(2))                                            /*!< Clock Calibration Source Selection */
#define SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL  (BITS(4,7))                                         /*!< Reference Clock Source Selection */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL  (BITS(8,11))                                        /*!< Clock Under Test Source Selection */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_DONE_IE  (BIT(12))                                           /*!< Clock Calibration Complete Flag Interrupt Enable */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_OV_IE  (BIT(13))                                           /*!< Calibrated Clock Counter Overflow Flag Interrupt Enable */

/* SYSTEM_CORE0_CLK_REF_CNT */
#define SYSTEM_CORE0_CLK_REF_CNT_REF_CLK_CNT  (BITS(0,23))                                        /*!< Reference Clock Count Value */

/* SYSTEM_CORE0_CLK_CAL_CNT */
#define SYSTEM_CORE0_CLK_CAL_CNT_CAL_CLK_CNT  (BITS(0,23))                                        /*!< Count value returned by the clock under calibration */

/* SYSTEM_CORE0_CLK_CAL_STA */
#define SYSTEM_CORE0_CLK_CAL_STA_CAL_DONE    (BIT(0))                                            /*!< Clock Calibration Complete Flag */
#define SYSTEM_CORE0_CLK_CAL_STA_CAL_OV      (BIT(1))                                            /*!< Calibrated Clock Counter Overflow Flag */
#define SYSTEM_CORE0_CLK_CAL_STA_AOS_CAL_BUSY  (BIT(2))                                            /*!< AOS triggers the calibration busy flag */

/* SYSTEM_CORE0_RST_CTRL */
#define SYSTEM_CORE0_RST_CTRL_CORE0T1_RST_EN  (BITS(0,15))                                        /*!< CORE0 Reset CORE1 Enable */
#define SYSTEM_CORE0_RST_CTRL_CORE1_RST_SEL  (BITS(16,31))                                       /*!< CORE1 WDT and Soft Reset Target Selection */

/* SYSTEM_CORE0_RST_STOP_CTRL */
#define SYSTEM_CORE0_RST_STOP_CTRL_CORE1_STOP  (BITS(0,15))                                        /*!< CORE1 STOP ON RESET Selection */

/* SYSTEM_CORE0_POW_RST_FLAG */
#define SYSTEM_CORE0_POW_RST_FLAG_VDET5      (BIT(0))                                            /*!< VDET5 Reset Flag */
#define SYSTEM_CORE0_POW_RST_FLAG_POR5       (BIT(4))                                            /*!< POR5 Reset Flag */
#define SYSTEM_CORE0_POW_RST_FLAG_BOR5       (BIT(5))                                            /*!< BOR5 Reset Flag */
#define SYSTEM_CORE0_POW_RST_FLAG_PROB5      (BIT(6))                                            /*!< PROB5 Reset Flag */
#define SYSTEM_CORE0_POW_RST_FLAG_POR15      (BIT(8))                                            /*!< POR15 Reset Flag */
#define SYSTEM_CORE0_POW_RST_FLAG_BOR15      (BIT(9))                                            /*!< BOR15 Reset Flag */
#define SYSTEM_CORE0_POW_RST_FLAG_PROB15     (BIT(10))                                           /*!< PROB15 Reset Flag */

/* SYSTEM_CORE0_SYS_RST_FLAG */
#define SYSTEM_CORE0_SYS_RST_FLAG_POWEROK    (BIT(0))                                            /*!< Power Reset Flag */
#define SYSTEM_CORE0_SYS_RST_FLAG_PIN        (BIT(1))                                            /*!< External Pin Reset Flag */
#define SYSTEM_CORE0_SYS_RST_FLAG_CORE0_WDT  (BIT(2))                                            /*!< CORE0 WDT Reset Flag */
#define SYSTEM_CORE0_SYS_RST_FLAG_CORE1T0_WDT  (BIT(3))                                            /*!< CORE1T0 WDT Reset Flag */
#define SYSTEM_CORE0_SYS_RST_FLAG_CORE0_SOFT  (BIT(4))                                            /*!< CORE0 Software Reset Flag */
#define SYSTEM_CORE0_SYS_RST_FLAG_CORE1T0_SOFT  (BIT(5))                                            /*!< CORE1T 0 Software Reset Flag */
#define SYSTEM_CORE0_SYS_RST_FLAG_BOI_VBAT   (BIT(6))                                            /*!< VBAT BOI Power-Down Reset Occurred */

/* SYSTEM_CORE0_BOI_RST_CTRL */
#define SYSTEM_CORE0_BOI_RST_CTRL_BOI_VBAT_RST_IE  (BIT(6))                                            /*!< BOI_VBAT_RST Interrupt Enable Bit */

/* SYSTEM_CORE0_UPDATE_RST_CTRL */
#define SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_EN  (BITS(0,15))                                        /*!< CORE0 Update Reset Enable */
#define SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_FLAG  (BIT(16))                                           /*!< CORE0 Update Reset Flag */

/* SYSTEM_CORE0_PMOD0_RST */
#define SYSTEM_CORE0_PMOD0_RST_MAILBOX       (BIT(0))                                            /*!< Software reset MAILBOX module */
#define SYSTEM_CORE0_PMOD0_RST_AOS           (BIT(1))                                            /*!< Software reset AOS module */
#define SYSTEM_CORE0_PMOD0_RST_GDMA00        (BIT(2))                                            /*!< Software reset GDMA00 module */
#define SYSTEM_CORE0_PMOD0_RST_GDMA01        (BIT(3))                                            /*!< Software reset GDMA01 module */
#define SYSTEM_CORE0_PMOD0_RST_GPIO          (BIT(4))                                            /*!< Software reset GPIO module */
#define SYSTEM_CORE0_PMOD0_RST_CRC1          (BIT(6))                                            /*!< Software reset CRC1 module */

/* SYSTEM_CORE0_MOD0_RST */
#define SYSTEM_CORE0_MOD0_RST_UART0          (BIT(0))                                            /*!< Software reset UART0 module */
#define SYSTEM_CORE0_MOD0_RST_UART1          (BIT(1))                                            /*!< Software reset UART1 module */
#define SYSTEM_CORE0_MOD0_RST_UART2          (BIT(2))                                            /*!< Software reset UART2 module */
#define SYSTEM_CORE0_MOD0_RST_UART3          (BIT(3))                                            /*!< Software reset UART3 module */
#define SYSTEM_CORE0_MOD0_RST_UART4          (BIT(4))                                            /*!< Software reset UART4 module */
#define SYSTEM_CORE0_MOD0_RST_UART5          (BIT(5))                                            /*!< Software reset UART5 module */
#define SYSTEM_CORE0_MOD0_RST_UART6          (BIT(6))                                            /*!< Software reset UART6 module */
#define SYSTEM_CORE0_MOD0_RST_UART7          (BIT(7))                                            /*!< Software reset UART7 module */
#define SYSTEM_CORE0_MOD0_RST_LPUART         (BIT(8))                                            /*!< Software reset LPUART module */
#define SYSTEM_CORE0_MOD0_RST_USART0         (BIT(9))                                            /*!< Software reset USART0 module */
#define SYSTEM_CORE0_MOD0_RST_USART1         (BIT(10))                                           /*!< Software reset USART1 module */
#define SYSTEM_CORE0_MOD0_RST_ISO78160       (BIT(11))                                           /*!< Software reset ISO78160 module */
#define SYSTEM_CORE0_MOD0_RST_ISO78161       (BIT(12))                                           /*!< Software reset ISO78161 module */
#define SYSTEM_CORE0_MOD0_RST_SPI0           (BIT(13))                                           /*!< Software reset SPI0 module */
#define SYSTEM_CORE0_MOD0_RST_SPI1           (BIT(14))                                           /*!< Software reset SPI1 module */
#define SYSTEM_CORE0_MOD0_RST_SPI2           (BIT(15))                                           /*!< Software reset SPI2 module */
#define SYSTEM_CORE0_MOD0_RST_SPI3           (BIT(16))                                           /*!< Software reset SPI3 module */
#define SYSTEM_CORE0_MOD0_RST_I2C0           (BIT(17))                                           /*!< Software reset I2C0 module */
#define SYSTEM_CORE0_MOD0_RST_I2C1           (BIT(18))                                           /*!< Software reset I2C1 module */
#define SYSTEM_CORE0_MOD0_RST_CAN            (BIT(19))                                           /*!< Software reset CAN module */
#define SYSTEM_CORE0_MOD0_RST_IOCNT          (BIT(20))                                           /*!< Software reset IOCNT module */
#define SYSTEM_CORE0_MOD0_RST_LCD            (BIT(21))                                           /*!< Software reset LCD module */
#define SYSTEM_CORE0_MOD0_RST_DSP            (BIT(22))                                           /*!< Software reset DSP module */
#define SYSTEM_CORE0_MOD0_RST_M2M            (BIT(24))                                           /*!< Software reset M2M module */
#define SYSTEM_CORE0_MOD0_RST_D2F            (BIT(25))                                           /*!< Software reset D2F module */
#define SYSTEM_CORE0_MOD0_RST_ADC0           (BIT(26))                                           /*!< Software reset ADC0 module */
#define SYSTEM_CORE0_MOD0_RST_DAC            (BIT(27))                                           /*!< Software reset DAC module */
#define SYSTEM_CORE0_MOD0_RST_TC0            (BIT(28))                                           /*!< Software reset TC0 module */
#define SYSTEM_CORE0_MOD0_RST_TC1            (BIT(29))                                           /*!< Software reset TC1 module */

/* SYSTEM_CORE0_MOD1_RST */
#define SYSTEM_CORE0_MOD1_RST_SIMPTC0        (BIT(0))                                            /*!< Software reset SIMP_TC0 module */
#define SYSTEM_CORE0_MOD1_RST_SIMPTC1        (BIT(1))                                            /*!< Software reset SIMP_TC1 module */
#define SYSTEM_CORE0_MOD1_RST_SIMPTC2        (BIT(2))                                            /*!< Software reset SIMP_TC2 module */
#define SYSTEM_CORE0_MOD1_RST_SIMPTC3        (BIT(3))                                            /*!< Software reset SIMP_TC3 module */
#define SYSTEM_CORE0_MOD1_RST_RS_VDD         (BIT(4))                                            /*!< Software reset RS_VDD module */
#define SYSTEM_CORE0_MOD1_RST_LSRX           (BIT(6))                                            /*!< Software reset LSRX module */
#define SYSTEM_CORE0_MOD1_RST_KBI0           (BIT(7))                                            /*!< Software reset KBI0 module */
#define SYSTEM_CORE0_MOD1_RST_KBI1           (BIT(8))                                            /*!< Software reset KBI1 module */
#define SYSTEM_CORE0_MOD1_RST_LVDCMP         (BIT(9))                                            /*!< Software reset LVDCMP module */
#define SYSTEM_CORE0_MOD1_RST_CRC0           (BIT(10))                                           /*!< Software reset CRC0 module */

/* SYSTEM_CORE0_MOD2_RST */
#define SYSTEM_CORE0_MOD2_RST_QSPI0          (BIT(0))                                            /*!< Software reset QSPI0 module */
#define SYSTEM_CORE0_MOD2_RST_QSPI1          (BIT(1))                                            /*!< Software reset QSPI1 module */
#define SYSTEM_CORE0_MOD2_RST_SEA            (BIT(2))                                            /*!< Software reset SEA module */
#define SYSTEM_CORE0_MOD2_RST_AES            (BIT(8))                                            /*!< Software reset AES module */

/* SYSTEM_CORE0_BOR5_CTRL */
#define SYSTEM_CORE0_BOR5_CTRL_BOR5_SEL      (BITS(0,15))                                        /*!< BOR5 Threshold Selection Signal, Power-up threshold / Power-down threshold */

/* SYSTEM_CORE0_PLVD5_CTRL */
#define SYSTEM_CORE0_PLVD5_CTRL_PLVD5_IE     (BIT(0))                                            /*!< PLVD5 Interrupt Enable */
#define SYSTEM_CORE0_PLVD5_CTRL_PLVD5_FLAG   (BIT(1))                                            /*!< PLVD5 Low Voltage Event Occurred */

/* SYSTEM_CORE0_ADDR_MAP_CTRL */
#define SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0     (BITS(0,2))                                         /*!< Core0 Address ReMapping */
#define SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1     (BITS(4,6))                                         /*!< Core1 Address ReMapping */

/* SYSTEM_CORE0_FLASH_MAP_CTRL */
#define SYSTEM_CORE0_FLASH_MAP_CTRL_CORE0_FLASH  (BIT(0))                                            /*!< Core0 Flash Address Mapping */
#define SYSTEM_CORE0_FLASH_MAP_CTRL_CORE1_FLASH  (BIT(1))                                            /*!< Core1 Flash Address Mapping */

/* SYSTEM_CORE0_DMA_PRI0_CTRL */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH0   (BITS(0,1))                                         /*!< Channel 0: SPI0 channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH1   (BITS(2,3))                                         /*!< Channel 1: SPI1 channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH2   (BITS(4,5))                                         /*!< Channel 2: SPI2 channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH3   (BITS(6,7))                                         /*!< Channel 3: SPI3 channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH4   (BITS(8,9))                                         /*!< Channel 4: SARADC channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH5   (BITS(10,11))                                       /*!< Channel 5: DAC channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH6   (BITS(12,13))                                       /*!< Channel 6: UART0 channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH7   (BITS(14,15))                                       /*!< Channel 7: UART1 channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH8   (BITS(16,17))                                       /*!< Channel 8: UART2 channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH9   (BITS(18,19))                                       /*!< Channel 9: UART3 channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH10  (BITS(20,21))                                       /*!< Channel 10: UART4 channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH11  (BITS(22,23))                                       /*!< Channel 11: UART5 channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH12  (BITS(24,25))                                       /*!< Channel 12: UART6 channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH13  (BITS(26,27))                                       /*!< Channel 13: UART7 channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH14  (BITS(28,29))                                       /*!< Channel 14: CAN channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH15  (BITS(30,31))                                       /*!< Channel 15: TC0_PWM channel access SRAM priority configuration. */

/* SYSTEM_CORE0_DMA_PRI1_CTRL */
#define SYSTEM_CORE0_DMA_PRI1_CTRL_DMA_CH16  (BITS(0,1))                                         /*!< Channel 16: TC1_PWM channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI1_CTRL_DMA_CH17  (BITS(2,3))                                         /*!< Channel 17: AES channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI1_CTRL_DMA_CH18  (BITS(4,5))                                         /*!< Channel 18: M2M channel access SRAM priority configuration. */

/* SYSTEM_CORE0_DMA_PRI2_CTRL */
#define SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH0   (BITS(0,1))                                         /*!< Channel 0: First level arbitration result channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH1   (BITS(2,3))                                         /*!< Channel 1: General DMA read channel (MDMA0x_RD/MDMA1x_RD) access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH2   (BITS(4,5))                                         /*!< Channel 2: General DMA write channel (MDMA0x_WR/MDMA1x_WR) access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH3   (BITS(6,7))                                         /*!< Channel 3: DSP channel access SRAM priority configuration. */
#define SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH4   (BITS(8,9))                                         /*!< Channel 4: CORE0/CORE1 channel access SRAM priority configuration. */

/* SYSTEM_CORE0_IRFR_CTRL */
#define SYSTEM_CORE0_IRFR_CTRL_INFRARED_CYCLE  (BITS(0,7))                                         /*!< Infrared clock division factor in HCMM and RCMM modes, based on 7.3728MHZ */

/* SYSTEM_CORE0_DIG_REV_CTRL */
#define SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_IE  (BITS(0,1))                                         /*!< CORE02CORE1 Bridge Illegal Access Interrupt Enable */
#define SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_PENDING  (BITS(2,3))                                         /*!< CORE02CORE1 Bridge Illegal Access Flag */

/* SYSTEM_CORE0_CHIP_ID */
#define SYSTEM_CORE0_CHIP_ID_CHIP_ID         (BITS(0,15))                                        /*!< CHIP_ID: C248 */

/* SYSTEM_CORE0_DIE_VER */
#define SYSTEM_CORE0_DIE_VER_VERSION         (BITS(0,3))                                         /*!< Version Number */
#define SYSTEM_CORE0_DIE_VER_CHIP_MODEL      (BITS(4,31))                                        /*!< Chip Model: A9701C, Encoded in ASCII */

/* SYSTEM_CORE0_FAB_UID0 */
#define SYSTEM_CORE0_FAB_UID0_LOT_NUM        (BITS(0,31))                                        /*!< This register is configured by the bootloader */

/* SYSTEM_CORE0_FAB_UID1 */
#define SYSTEM_CORE0_FAB_UID1_WAFER_ID       (BITS(0,7))                                         /*!< the wafer WAFER ID information */
#define SYSTEM_CORE0_FAB_UID1_X_LABEL        (BITS(8,19))                                        /*!< DIE X-coordinate */
#define SYSTEM_CORE0_FAB_UID1_Y_LABEL        (BITS(20,31))                                       /*!< DIE Y-coordinate */

/* constants definitions */
/* SYSTEM_CORE0_SYS_PSW1_SYS_PSW1 */
#define SYSTEM_CORE0_SYS_PSW1_SYS_PSW1_POS   (0U)                                                /*!< SYSTEM_CORE0_SYS_PSW1_SYS_PSW1 bit offset */

/* SYSTEM_CORE0_RCH_CTRL_PD */
#define SYSTEM_CORE0_RCH_CTRL_PD_POS         (0U)                                                /*!< SYSTEM_CORE0_RCH_CTRL_PD bit offset */

/* SYSTEM_CORE0_RCH_CTRL_DIV */
#define SYSTEM_CORE0_RCH_CTRL_DIV_POS        (1U)                                                /*!< SYSTEM_CORE0_RCH_CTRL_DIV bit offset */
#define SYSTEM_CORE0_RCH_CTRL_DIV_CFG(regval)  (SYSTEM_CORE0_RCH_CTRL_DIV & ((uint32_t)(regval) << SYSTEM_CORE0_RCH_CTRL_DIV_POS))
#define SYSTEM_CORE0_RCH_CTRL_DIV_1          (SYSTEM_CORE0_RCH_CTRL_DIV_CFG(0x0))                /*!< Divide by 1 */
#define SYSTEM_CORE0_RCH_CTRL_DIV_2          (SYSTEM_CORE0_RCH_CTRL_DIV_CFG(0x1))                /*!< Divide by 2 */
#define SYSTEM_CORE0_RCH_CTRL_DIV_4          (SYSTEM_CORE0_RCH_CTRL_DIV_CFG(0x2))                /*!< Divide by 4 */
#define SYSTEM_CORE0_RCH_CTRL_DIV_8          (SYSTEM_CORE0_RCH_CTRL_DIV_CFG(0x3))                /*!< Divide by 8 */
#define SYSTEM_CORE0_RCH_CTRL_DIV_16         (SYSTEM_CORE0_RCH_CTRL_DIV_CFG(0x4))                /*!< Divide by 16 */
#define SYSTEM_CORE0_RCH_CTRL_DIV_32         (SYSTEM_CORE0_RCH_CTRL_DIV_CFG(0x5))                /*!< Divide by 32 (default) */

/* SYSTEM_CORE0_PLL_CTRL_PD */
#define SYSTEM_CORE0_PLL_CTRL_PD_POS         (0U)                                                /*!< SYSTEM_CORE0_PLL_CTRL_PD bit offset */

/* SYSTEM_CORE0_PLL_CTRL_FREQ */
#define SYSTEM_CORE0_PLL_CTRL_FREQ_POS       (1U)                                                /*!< SYSTEM_CORE0_PLL_CTRL_FREQ bit offset */
#define SYSTEM_CORE0_PLL_CTRL_FREQ_CFG(regval)  (SYSTEM_CORE0_PLL_CTRL_FREQ & ((uint32_t)(regval) << SYSTEM_CORE0_PLL_CTRL_FREQ_POS))
#define SYSTEM_CORE0_PLL_CTRL_FREQ_7M3728    (SYSTEM_CORE0_PLL_CTRL_FREQ_CFG(0x0))               /*!< PLL output frequency selection is 7.3728MHz */
#define SYSTEM_CORE0_PLL_CTRL_FREQ_14M7456   (SYSTEM_CORE0_PLL_CTRL_FREQ_CFG(0x1))               /*!< PLL output frequency selection is 14.7456MHz */
#define SYSTEM_CORE0_PLL_CTRL_FREQ_29M4912   (SYSTEM_CORE0_PLL_CTRL_FREQ_CFG(0x2))               /*!< PLL output frequency selection is 29.4912MHz */
#define SYSTEM_CORE0_PLL_CTRL_FREQ_58M9824   (SYSTEM_CORE0_PLL_CTRL_FREQ_CFG(0x3))               /*!< PLL output frequency selection is 58.9824MHz */

/* SYSTEM_CORE0_PLL_CTRL_DIV */
#define SYSTEM_CORE0_PLL_CTRL_DIV_POS        (4U)                                                /*!< SYSTEM_CORE0_PLL_CTRL_DIV bit offset */
#define SYSTEM_CORE0_PLL_CTRL_DIV_CFG(regval)  (SYSTEM_CORE0_PLL_CTRL_DIV & ((uint32_t)(regval) << SYSTEM_CORE0_PLL_CTRL_DIV_POS))
#define SYSTEM_CORE0_PLL_CTRL_DIV_1          (SYSTEM_CORE0_PLL_CTRL_DIV_CFG(0x0))                /*!< Divide by 1 */
#define SYSTEM_CORE0_PLL_CTRL_DIV_2          (SYSTEM_CORE0_PLL_CTRL_DIV_CFG(0x1))                /*!< Divide by 2 */
#define SYSTEM_CORE0_PLL_CTRL_DIV_4          (SYSTEM_CORE0_PLL_CTRL_DIV_CFG(0x2))                /*!< Divide by 4 */
#define SYSTEM_CORE0_PLL_CTRL_DIV_8          (SYSTEM_CORE0_PLL_CTRL_DIV_CFG(0x3))                /*!< Divide by 8 */
#define SYSTEM_CORE0_PLL_CTRL_DIV_16         (SYSTEM_CORE0_PLL_CTRL_DIV_CFG(0x4))                /*!< Divide by 16 */

/* SYSTEM_CORE0_PLL_CTRL_LOCK */
#define SYSTEM_CORE0_PLL_CTRL_LOCK_POS       (8U)                                                /*!< SYSTEM_CORE0_PLL_CTRL_LOCK bit offset */

/* SYSTEM_CORE0_HOSC_CTRL_PD */
#define SYSTEM_CORE0_HOSC_CTRL_PD_POS        (0U)                                                /*!< SYSTEM_CORE0_HOSC_CTRL_PD bit offset */

/* SYSTEM_CORE0_HOSC_CTRL_FREQ */
#define SYSTEM_CORE0_HOSC_CTRL_FREQ_POS      (1U)                                                /*!< SYSTEM_CORE0_HOSC_CTRL_FREQ bit offset */
#define SYSTEM_CORE0_HOSC_CTRL_FREQ_CFG(regval)  (SYSTEM_CORE0_HOSC_CTRL_FREQ & ((uint32_t)(regval) << SYSTEM_CORE0_HOSC_CTRL_FREQ_POS))
#define SYSTEM_CORE0_HOSC_CTRL_FREQ_7M3728   (SYSTEM_CORE0_HOSC_CTRL_FREQ_CFG(0x0))              /*!< External high-frequency crystal is 7.3728MHz */
#define SYSTEM_CORE0_HOSC_CTRL_FREQ_14M7456  (SYSTEM_CORE0_HOSC_CTRL_FREQ_CFG(0x1))              /*!< External high-frequency crystal is 14.7456MHz */
#define SYSTEM_CORE0_HOSC_CTRL_FREQ_29M4912  (SYSTEM_CORE0_HOSC_CTRL_FREQ_CFG(0x2))              /*!< External high-frequency crystal is 29.4912MHz */

/* SYSTEM_CORE0_HOSC_CTRL_DIV */
#define SYSTEM_CORE0_HOSC_CTRL_DIV_POS       (4U)                                                /*!< SYSTEM_CORE0_HOSC_CTRL_DIV bit offset */
#define SYSTEM_CORE0_HOSC_CTRL_DIV_CFG(regval)  (SYSTEM_CORE0_HOSC_CTRL_DIV & ((uint32_t)(regval) << SYSTEM_CORE0_HOSC_CTRL_DIV_POS))
#define SYSTEM_CORE0_HOSC_CTRL_DIV_1         (SYSTEM_CORE0_HOSC_CTRL_DIV_CFG(0x0))               /*!< Divide by 1 */
#define SYSTEM_CORE0_HOSC_CTRL_DIV_2         (SYSTEM_CORE0_HOSC_CTRL_DIV_CFG(0x1))               /*!< Divide by 2 */
#define SYSTEM_CORE0_HOSC_CTRL_DIV_4         (SYSTEM_CORE0_HOSC_CTRL_DIV_CFG(0x2))               /*!< Divide by 4 */
#define SYSTEM_CORE0_HOSC_CTRL_DIV_8         (SYSTEM_CORE0_HOSC_CTRL_DIV_CFG(0x3))               /*!< Divide by 8 */
#define SYSTEM_CORE0_HOSC_CTRL_DIV_16        (SYSTEM_CORE0_HOSC_CTRL_DIV_CFG(0x4))               /*!< Divide by 16 */

/* SYSTEM_CORE0_HOSC_CTRL_PGA */
#define SYSTEM_CORE0_HOSC_CTRL_PGA_POS       (8U)                                                /*!< SYSTEM_CORE0_HOSC_CTRL_PGA bit offset */
#define SYSTEM_CORE0_HOSC_CTRL_PGA_CFG(regval)  (SYSTEM_CORE0_HOSC_CTRL_PGA & ((uint32_t)(regval) << SYSTEM_CORE0_HOSC_CTRL_PGA_POS))
#define SYSTEM_CORE0_HOSC_CTRL_PGA_1X_0_8MS  (SYSTEM_CORE0_HOSC_CTRL_PGA_CFG(0x0))               /*!< 1x transconductance gain, 928uA current, 0.8mS */
#define SYSTEM_CORE0_HOSC_CTRL_PGA_2X_2_4MS  (SYSTEM_CORE0_HOSC_CTRL_PGA_CFG(0x1))               /*!< 2x transconductance gain, 1856uA current, 2.4mS */
#define SYSTEM_CORE0_HOSC_CTRL_PGA_4X_3_2MS  (SYSTEM_CORE0_HOSC_CTRL_PGA_CFG(0x2))               /*!< 4x transconductance gain, 3712uA current, 3.2mS */
#define SYSTEM_CORE0_HOSC_CTRL_PGA_8X_5_6MS  (SYSTEM_CORE0_HOSC_CTRL_PGA_CFG(0x3))               /*!< 8x transconductance gain, 7424uA current, 5.6mS */
#define SYSTEM_CORE0_HOSC_CTRL_PGA_1X_1_2MS  (SYSTEM_CORE0_HOSC_CTRL_PGA_CFG(0x4))               /*!< 1x transconductance gain, enhanced gain, 1.2mS */
#define SYSTEM_CORE0_HOSC_CTRL_PGA_2X_1_6MS  (SYSTEM_CORE0_HOSC_CTRL_PGA_CFG(0x5))               /*!< 2x transconductance gain, enhanced gain, 1.6mS */
#define SYSTEM_CORE0_HOSC_CTRL_PGA_4X_4_8MS  (SYSTEM_CORE0_HOSC_CTRL_PGA_CFG(0x6))               /*!< 4x transconductance gain, enhanced gain, 4.8mS */
#define SYSTEM_CORE0_HOSC_CTRL_PGA_8X_6_4MS  (SYSTEM_CORE0_HOSC_CTRL_PGA_CFG(0x7))               /*!< 8x transconductance gain, enhanced gain, 6.4mS */

/* SYSTEM_CORE0_HOSC_CTRL_DSEL */
#define SYSTEM_CORE0_HOSC_CTRL_DSEL_POS      (12U)                                               /*!< SYSTEM_CORE0_HOSC_CTRL_DSEL bit offset */
#define SYSTEM_CORE0_HOSC_CTRL_DSEL_CFG(regval)  (SYSTEM_CORE0_HOSC_CTRL_DSEL & ((uint32_t)(regval) << SYSTEM_CORE0_HOSC_CTRL_DSEL_POS))
#define SYSTEM_CORE0_HOSC_CTRL_DSEL_25_5NS   (SYSTEM_CORE0_HOSC_CTRL_DSEL_CFG(0x0))              /*!< External high-frequency crystal is 7.3728MHz */
#define SYSTEM_CORE0_HOSC_CTRL_DSEL_10_5NS   (SYSTEM_CORE0_HOSC_CTRL_DSEL_CFG(0x1))              /*!< External high-frequency crystal is 14.7456MHz */
#define SYSTEM_CORE0_HOSC_CTRL_DSEL_5NS      (SYSTEM_CORE0_HOSC_CTRL_DSEL_CFG(0x2))              /*!< External high-frequency crystal is 29.4912MHz */
#define SYSTEM_CORE0_HOSC_CTRL_DSEL_NONE     (SYSTEM_CORE0_HOSC_CTRL_DSEL_CFG(0x4))              /*!< HOSC clock does not use analog filtering */

/* SYSTEM_CORE0_RCL307K_CTRL_PD */
#define SYSTEM_CORE0_RCL307K_CTRL_PD_POS     (0U)                                                /*!< SYSTEM_CORE0_RCL307K_CTRL_PD bit offset */

/* SYSTEM_CORE0_CLK_SEL_SYS_RCL_LOSC */
#define SYSTEM_CORE0_CLK_SEL_SYS_RCL_LOSC_POS  (0U)                                                /*!< SYSTEM_CORE0_CLK_SEL_SYS_RCL_LOSC bit offset */

/* SYSTEM_CORE0_CLK_SEL_SYS_PLL_HOSC */
#define SYSTEM_CORE0_CLK_SEL_SYS_PLL_HOSC_POS  (1U)                                                /*!< SYSTEM_CORE0_CLK_SEL_SYS_PLL_HOSC bit offset */

/* SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC */
#define SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC_POS  (2U)                                                /*!< SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC bit offset */
#define SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC_CFG(regval)  (SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC & ((uint32_t)(regval) << SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC_POS))
#define SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC_LOSC  (SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC_CFG(0x0))        /*!< LCD selects LOSC external low-frequency crystal as clock source */
#define SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC_RCL32K  (SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC_CFG(0x1))        /*!< LCD selects RCL32k internal low-frequency crystal as clock source */
#define SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC_RCL307K  (SYSTEM_CORE0_CLK_SEL_LCD_RCL_LOSC_CFG(0x2))        /*!< LCD selects RCL307k internal low-frequency crystal as clock source (Not available) */

/* SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC */
#define SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC_POS  (4U)                                                /*!< SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC bit offset */
#define SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC_CFG(regval)  (SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC & ((uint32_t)(regval) << SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC_POS))
#define SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC_LOSC  (SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC_CFG(0x0))       /*!< IRFR selects LOSC external low-frequency crystal as clock source */
#define SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC_RCL32K  (SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC_CFG(0x1))       /*!< IRFR selects RCL32k internal low-frequency crystal as clock source */
#define SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC_RCL307K  (SYSTEM_CORE0_CLK_SEL_IRFR_RCL_LOSC_CFG(0x2))       /*!< IRFR selects RCL307k internal low-frequency crystal as clock source */

/* SYSTEM_CORE0_CLK_SEL_KBI_RCL_LOSC */
#define SYSTEM_CORE0_CLK_SEL_KBI_RCL_LOSC_POS  (6U)                                                /*!< SYSTEM_CORE0_CLK_SEL_KBI_RCL_LOSC bit offset */

/* SYSTEM_CORE0_CLK_SEL_LPUART_RCL_LOSC */
#define SYSTEM_CORE0_CLK_SEL_LPUART_RCL_LOSC_POS  (7U)                                                /*!< SYSTEM_CORE0_CLK_SEL_LPUART_RCL_LOSC bit offset */

/* SYSTEM_CORE0_CLK_SEL_VBAT_RCL */
#define SYSTEM_CORE0_CLK_SEL_VBAT_RCL_POS    (8U)                                                /*!< SYSTEM_CORE0_CLK_SEL_VBAT_RCL bit offset */

/* SYSTEM_CORE0_CLK_SEL_D2F_RCL_LOSC */
#define SYSTEM_CORE0_CLK_SEL_D2F_RCL_LOSC_POS  (9U)                                                /*!< SYSTEM_CORE0_CLK_SEL_D2F_RCL_LOSC bit offset */

/* SYSTEM_CORE0_CLK_SEL_CORE0_RCL_LOSC */
#define SYSTEM_CORE0_CLK_SEL_CORE0_RCL_LOSC_POS  (10U)                                               /*!< SYSTEM_CORE0_CLK_SEL_CORE0_RCL_LOSC bit offset */

/* SYSTEM_CORE0_CLK_SEL_CORE1_RCL_LOSC */
#define SYSTEM_CORE0_CLK_SEL_CORE1_RCL_LOSC_POS  (11U)                                               /*!< SYSTEM_CORE0_CLK_SEL_CORE1_RCL_LOSC bit offset */

/* SYSTEM_CORE0_SYS_MODE_SYS_MODE */
#define SYSTEM_CORE0_SYS_MODE_SYS_MODE_POS   (0U)                                                /*!< SYSTEM_CORE0_SYS_MODE_SYS_MODE bit offset */
#define SYSTEM_CORE0_SYS_MODE_SYS_MODE_CFG(regval)  (SYSTEM_CORE0_SYS_MODE_SYS_MODE & ((uint32_t)(regval) << SYSTEM_CORE0_SYS_MODE_SYS_MODE_POS))
#define SYSTEM_CORE0_SYS_MODE_SYS_MODE_HCM   (SYSTEM_CORE0_SYS_MODE_SYS_MODE_CFG(0x4))           /*!< Write D, set to enter high-frequency mode HCM, read value 4'b0100 */
#define SYSTEM_CORE0_SYS_MODE_SYS_MODE_RCM   (SYSTEM_CORE0_SYS_MODE_SYS_MODE_CFG(0x2))           /*!< Write E, set to enter RC mode RCM, read value 4'b0010 */
#define SYSTEM_CORE0_SYS_MODE_SYS_MODE_LCM   (SYSTEM_CORE0_SYS_MODE_SYS_MODE_CFG(0x1))           /*!< Write F, set to enter 32.768KHz mode LCM, read value 4'b0001 */

/* SYSTEM_CORE0_SYS_MODE_FLASH_BUSY */
#define SYSTEM_CORE0_SYS_MODE_FLASH_BUSY_POS  (4U)                                                /*!< SYSTEM_CORE0_SYS_MODE_FLASH_BUSY bit offset */

/* SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV */
#define SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_POS  (0U)                                                /*!< SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV bit offset */
#define SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CFG(regval)  (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV & ((uint32_t)(regval) << SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_POS))
#define SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_NONE_CORE0_NONE  (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CFG(0x0))    /*!< CORE1 no division, CORE0 no division */
#define SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_NONE_CORE0_2  (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CFG(0x1))    /*!< CORE1 no division, CORE0 divide by 2 */
#define SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_NONE_CORE0_4  (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CFG(0x2))    /*!< CORE1 no division, CORE0 divide by 4 */
#define SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_NONE_CORE0_8  (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CFG(0x3))    /*!< CORE1 no division, CORE0 divide by 8 */
#define SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_2_CORE0_2  (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CFG(0x5))    /*!< CORE1 divide by 2, CORE0 divide by 2 */
#define SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_2_CORE0_4  (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CFG(0x6))    /*!< CORE1 divide by 2, CORE0 divide by 4 */
#define SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_2_CORE0_8  (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CFG(0x7))    /*!< CORE1 divide by 2, CORE0 divide by 8 */
#define SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_4_CORE0_4  (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CFG(0xA))    /*!< CORE1 divide by 4, CORE0 divide by 4 */
#define SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_4_CORE0_8  (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CFG(0xB))    /*!< CORE1 divide by 4, CORE0 divide by 8 */
#define SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CORE1_8_CORE0_8  (SYSTEM_CORE0_CORE_DIV_CORE1_CORE0_DIV_CFG(0xF))    /*!< CORE1 divide by 8, CORE0 divide by 8 */

/* SYSTEM_CORE0_PMOD0_CKE_WDT */
#define SYSTEM_CORE0_PMOD0_CKE_WDT_POS       (0U)                                                /*!< SYSTEM_CORE0_PMOD0_CKE_WDT bit offset */

/* SYSTEM_CORE0_PMOD0_CKE_MAILBOX */
#define SYSTEM_CORE0_PMOD0_CKE_MAILBOX_POS   (1U)                                                /*!< SYSTEM_CORE0_PMOD0_CKE_MAILBOX bit offset */

/* SYSTEM_CORE0_PMOD0_CKE_AOS */
#define SYSTEM_CORE0_PMOD0_CKE_AOS_POS       (2U)                                                /*!< SYSTEM_CORE0_PMOD0_CKE_AOS bit offset */

/* SYSTEM_CORE0_PMOD0_CKE_SCU */
#define SYSTEM_CORE0_PMOD0_CKE_SCU_POS       (3U)                                                /*!< SYSTEM_CORE0_PMOD0_CKE_SCU bit offset */

/* SYSTEM_CORE0_PMOD0_CKE_GIC */
#define SYSTEM_CORE0_PMOD0_CKE_GIC_POS       (4U)                                                /*!< SYSTEM_CORE0_PMOD0_CKE_GIC bit offset */

/* SYSTEM_CORE0_PMOD0_CKE_GDMA00 */
#define SYSTEM_CORE0_PMOD0_CKE_GDMA00_POS    (8U)                                                /*!< SYSTEM_CORE0_PMOD0_CKE_GDMA00 bit offset */

/* SYSTEM_CORE0_PMOD0_CKE_GDMA01 */
#define SYSTEM_CORE0_PMOD0_CKE_GDMA01_POS    (9U)                                                /*!< SYSTEM_CORE0_PMOD0_CKE_GDMA01 bit offset */

/* SYSTEM_CORE0_PMOD0_CKE_UART38K */
#define SYSTEM_CORE0_PMOD0_CKE_UART38K_POS   (10U)                                               /*!< SYSTEM_CORE0_PMOD0_CKE_UART38K bit offset */

/* SYSTEM_CORE0_PMOD0_CKE_GPIO */
#define SYSTEM_CORE0_PMOD0_CKE_GPIO_POS      (11U)                                               /*!< SYSTEM_CORE0_PMOD0_CKE_GPIO bit offset */

/* SYSTEM_CORE0_PMOD0_CKE_LCSS */
#define SYSTEM_CORE0_PMOD0_CKE_LCSS_POS      (12U)                                               /*!< SYSTEM_CORE0_PMOD0_CKE_LCSS bit offset */

/* SYSTEM_CORE0_PMOD0_CKE_CRC1 */
#define SYSTEM_CORE0_PMOD0_CKE_CRC1_POS      (14U)                                               /*!< SYSTEM_CORE0_PMOD0_CKE_CRC1 bit offset */

/* SYSTEM_CORE0_CORE1_CKE_CORE1_CKE */
#define SYSTEM_CORE0_CORE1_CKE_CORE1_CKE_POS  (0U)                                                /*!< SYSTEM_CORE0_CORE1_CKE_CORE1_CKE bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI00 */
#define SYSTEM_CORE0_KBI0_CKE_KBI00_POS      (0U)                                                /*!< SYSTEM_CORE0_KBI0_CKE_KBI00 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI01 */
#define SYSTEM_CORE0_KBI0_CKE_KBI01_POS      (1U)                                                /*!< SYSTEM_CORE0_KBI0_CKE_KBI01 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI02 */
#define SYSTEM_CORE0_KBI0_CKE_KBI02_POS      (2U)                                                /*!< SYSTEM_CORE0_KBI0_CKE_KBI02 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI03 */
#define SYSTEM_CORE0_KBI0_CKE_KBI03_POS      (3U)                                                /*!< SYSTEM_CORE0_KBI0_CKE_KBI03 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI04 */
#define SYSTEM_CORE0_KBI0_CKE_KBI04_POS      (4U)                                                /*!< SYSTEM_CORE0_KBI0_CKE_KBI04 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI05 */
#define SYSTEM_CORE0_KBI0_CKE_KBI05_POS      (5U)                                                /*!< SYSTEM_CORE0_KBI0_CKE_KBI05 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI06 */
#define SYSTEM_CORE0_KBI0_CKE_KBI06_POS      (6U)                                                /*!< SYSTEM_CORE0_KBI0_CKE_KBI06 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI07 */
#define SYSTEM_CORE0_KBI0_CKE_KBI07_POS      (7U)                                                /*!< SYSTEM_CORE0_KBI0_CKE_KBI07 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI08 */
#define SYSTEM_CORE0_KBI0_CKE_KBI08_POS      (8U)                                                /*!< SYSTEM_CORE0_KBI0_CKE_KBI08 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI09 */
#define SYSTEM_CORE0_KBI0_CKE_KBI09_POS      (9U)                                                /*!< SYSTEM_CORE0_KBI0_CKE_KBI09 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI010 */
#define SYSTEM_CORE0_KBI0_CKE_KBI010_POS     (10U)                                               /*!< SYSTEM_CORE0_KBI0_CKE_KBI010 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI011 */
#define SYSTEM_CORE0_KBI0_CKE_KBI011_POS     (11U)                                               /*!< SYSTEM_CORE0_KBI0_CKE_KBI011 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI012 */
#define SYSTEM_CORE0_KBI0_CKE_KBI012_POS     (12U)                                               /*!< SYSTEM_CORE0_KBI0_CKE_KBI012 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI013 */
#define SYSTEM_CORE0_KBI0_CKE_KBI013_POS     (13U)                                               /*!< SYSTEM_CORE0_KBI0_CKE_KBI013 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI014 */
#define SYSTEM_CORE0_KBI0_CKE_KBI014_POS     (14U)                                               /*!< SYSTEM_CORE0_KBI0_CKE_KBI014 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBI015 */
#define SYSTEM_CORE0_KBI0_CKE_KBI015_POS     (15U)                                               /*!< SYSTEM_CORE0_KBI0_CKE_KBI015 bit offset */

/* SYSTEM_CORE0_KBI0_CKE_KBIC0 */
#define SYSTEM_CORE0_KBI0_CKE_KBIC0_POS      (31U)                                               /*!< SYSTEM_CORE0_KBI0_CKE_KBIC0 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI10 */
#define SYSTEM_CORE0_KBI1_CKE_KBI10_POS      (0U)                                                /*!< SYSTEM_CORE0_KBI1_CKE_KBI10 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI11 */
#define SYSTEM_CORE0_KBI1_CKE_KBI11_POS      (1U)                                                /*!< SYSTEM_CORE0_KBI1_CKE_KBI11 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI12 */
#define SYSTEM_CORE0_KBI1_CKE_KBI12_POS      (2U)                                                /*!< SYSTEM_CORE0_KBI1_CKE_KBI12 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI13 */
#define SYSTEM_CORE0_KBI1_CKE_KBI13_POS      (3U)                                                /*!< SYSTEM_CORE0_KBI1_CKE_KBI13 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI14 */
#define SYSTEM_CORE0_KBI1_CKE_KBI14_POS      (4U)                                                /*!< SYSTEM_CORE0_KBI1_CKE_KBI14 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI15 */
#define SYSTEM_CORE0_KBI1_CKE_KBI15_POS      (5U)                                                /*!< SYSTEM_CORE0_KBI1_CKE_KBI15 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI16 */
#define SYSTEM_CORE0_KBI1_CKE_KBI16_POS      (6U)                                                /*!< SYSTEM_CORE0_KBI1_CKE_KBI16 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI17 */
#define SYSTEM_CORE0_KBI1_CKE_KBI17_POS      (7U)                                                /*!< SYSTEM_CORE0_KBI1_CKE_KBI17 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI18 */
#define SYSTEM_CORE0_KBI1_CKE_KBI18_POS      (8U)                                                /*!< SYSTEM_CORE0_KBI1_CKE_KBI18 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI19 */
#define SYSTEM_CORE0_KBI1_CKE_KBI19_POS      (9U)                                                /*!< SYSTEM_CORE0_KBI1_CKE_KBI19 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI110 */
#define SYSTEM_CORE0_KBI1_CKE_KBI110_POS     (10U)                                               /*!< SYSTEM_CORE0_KBI1_CKE_KBI110 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI111 */
#define SYSTEM_CORE0_KBI1_CKE_KBI111_POS     (11U)                                               /*!< SYSTEM_CORE0_KBI1_CKE_KBI111 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI112 */
#define SYSTEM_CORE0_KBI1_CKE_KBI112_POS     (12U)                                               /*!< SYSTEM_CORE0_KBI1_CKE_KBI112 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI113 */
#define SYSTEM_CORE0_KBI1_CKE_KBI113_POS     (13U)                                               /*!< SYSTEM_CORE0_KBI1_CKE_KBI113 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI114 */
#define SYSTEM_CORE0_KBI1_CKE_KBI114_POS     (14U)                                               /*!< SYSTEM_CORE0_KBI1_CKE_KBI114 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBI115 */
#define SYSTEM_CORE0_KBI1_CKE_KBI115_POS     (15U)                                               /*!< SYSTEM_CORE0_KBI1_CKE_KBI115 bit offset */

/* SYSTEM_CORE0_KBI1_CKE_KBIC1 */
#define SYSTEM_CORE0_KBI1_CKE_KBIC1_POS      (31U)                                               /*!< SYSTEM_CORE0_KBI1_CKE_KBIC1 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_UART0 */
#define SYSTEM_CORE0_MOD0_CKE_UART0_POS      (0U)                                                /*!< SYSTEM_CORE0_MOD0_CKE_UART0 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_UART1 */
#define SYSTEM_CORE0_MOD0_CKE_UART1_POS      (1U)                                                /*!< SYSTEM_CORE0_MOD0_CKE_UART1 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_UART2 */
#define SYSTEM_CORE0_MOD0_CKE_UART2_POS      (2U)                                                /*!< SYSTEM_CORE0_MOD0_CKE_UART2 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_UART3 */
#define SYSTEM_CORE0_MOD0_CKE_UART3_POS      (3U)                                                /*!< SYSTEM_CORE0_MOD0_CKE_UART3 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_UART4 */
#define SYSTEM_CORE0_MOD0_CKE_UART4_POS      (4U)                                                /*!< SYSTEM_CORE0_MOD0_CKE_UART4 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_UART5 */
#define SYSTEM_CORE0_MOD0_CKE_UART5_POS      (5U)                                                /*!< SYSTEM_CORE0_MOD0_CKE_UART5 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_UART6 */
#define SYSTEM_CORE0_MOD0_CKE_UART6_POS      (6U)                                                /*!< SYSTEM_CORE0_MOD0_CKE_UART6 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_UART7 */
#define SYSTEM_CORE0_MOD0_CKE_UART7_POS      (7U)                                                /*!< SYSTEM_CORE0_MOD0_CKE_UART7 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_LPUART */
#define SYSTEM_CORE0_MOD0_CKE_LPUART_POS     (8U)                                                /*!< SYSTEM_CORE0_MOD0_CKE_LPUART bit offset */

/* SYSTEM_CORE0_MOD0_CKE_USART0 */
#define SYSTEM_CORE0_MOD0_CKE_USART0_POS     (9U)                                                /*!< SYSTEM_CORE0_MOD0_CKE_USART0 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_USART1 */
#define SYSTEM_CORE0_MOD0_CKE_USART1_POS     (10U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_USART1 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_ISO78160 */
#define SYSTEM_CORE0_MOD0_CKE_ISO78160_POS   (11U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_ISO78160 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_ISO78161 */
#define SYSTEM_CORE0_MOD0_CKE_ISO78161_POS   (12U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_ISO78161 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_SPI0 */
#define SYSTEM_CORE0_MOD0_CKE_SPI0_POS       (13U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_SPI0 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_SPI1 */
#define SYSTEM_CORE0_MOD0_CKE_SPI1_POS       (14U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_SPI1 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_SPI2 */
#define SYSTEM_CORE0_MOD0_CKE_SPI2_POS       (15U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_SPI2 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_SPI3 */
#define SYSTEM_CORE0_MOD0_CKE_SPI3_POS       (16U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_SPI3 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_I2C0 */
#define SYSTEM_CORE0_MOD0_CKE_I2C0_POS       (17U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_I2C0 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_I2C1 */
#define SYSTEM_CORE0_MOD0_CKE_I2C1_POS       (18U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_I2C1 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_IOCNT */
#define SYSTEM_CORE0_MOD0_CKE_IOCNT_POS      (20U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_IOCNT bit offset */

/* SYSTEM_CORE0_MOD0_CKE_LCD */
#define SYSTEM_CORE0_MOD0_CKE_LCD_POS        (21U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_LCD bit offset */

/* SYSTEM_CORE0_MOD0_CKE_DSP */
#define SYSTEM_CORE0_MOD0_CKE_DSP_POS        (22U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_DSP bit offset */

/* SYSTEM_CORE0_MOD0_CKE_M2M */
#define SYSTEM_CORE0_MOD0_CKE_M2M_POS        (24U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_M2M bit offset */

/* SYSTEM_CORE0_MOD0_CKE_D2F */
#define SYSTEM_CORE0_MOD0_CKE_D2F_POS        (25U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_D2F bit offset */

/* SYSTEM_CORE0_MOD0_CKE_ADC0 */
#define SYSTEM_CORE0_MOD0_CKE_ADC0_POS       (26U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_ADC0 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_DAC */
#define SYSTEM_CORE0_MOD0_CKE_DAC_POS        (27U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_DAC bit offset */

/* SYSTEM_CORE0_MOD0_CKE_TC0 */
#define SYSTEM_CORE0_MOD0_CKE_TC0_POS        (28U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_TC0 bit offset */

/* SYSTEM_CORE0_MOD0_CKE_TC1 */
#define SYSTEM_CORE0_MOD0_CKE_TC1_POS        (29U)                                               /*!< SYSTEM_CORE0_MOD0_CKE_TC1 bit offset */

/* SYSTEM_CORE0_MOD1_CKE_SIMPTC0 */
#define SYSTEM_CORE0_MOD1_CKE_SIMPTC0_POS    (0U)                                                /*!< SYSTEM_CORE0_MOD1_CKE_SIMPTC0 bit offset */

/* SYSTEM_CORE0_MOD1_CKE_SIMPTC1 */
#define SYSTEM_CORE0_MOD1_CKE_SIMPTC1_POS    (1U)                                                /*!< SYSTEM_CORE0_MOD1_CKE_SIMPTC1 bit offset */

/* SYSTEM_CORE0_MOD1_CKE_SIMPTC2 */
#define SYSTEM_CORE0_MOD1_CKE_SIMPTC2_POS    (2U)                                                /*!< SYSTEM_CORE0_MOD1_CKE_SIMPTC2 bit offset */

/* SYSTEM_CORE0_MOD1_CKE_SIMPTC3 */
#define SYSTEM_CORE0_MOD1_CKE_SIMPTC3_POS    (3U)                                                /*!< SYSTEM_CORE0_MOD1_CKE_SIMPTC3 bit offset */

/* SYSTEM_CORE0_MOD1_CKE_RS_VDD */
#define SYSTEM_CORE0_MOD1_CKE_RS_VDD_POS     (4U)                                                /*!< SYSTEM_CORE0_MOD1_CKE_RS_VDD bit offset */

/* SYSTEM_CORE0_MOD1_CKE_LSRX */
#define SYSTEM_CORE0_MOD1_CKE_LSRX_POS       (6U)                                                /*!< SYSTEM_CORE0_MOD1_CKE_LSRX bit offset */

/* SYSTEM_CORE0_MOD1_CKE_LVDCMP */
#define SYSTEM_CORE0_MOD1_CKE_LVDCMP_POS     (9U)                                                /*!< SYSTEM_CORE0_MOD1_CKE_LVDCMP bit offset */

/* SYSTEM_CORE0_MOD1_CKE_CRC0 */
#define SYSTEM_CORE0_MOD1_CKE_CRC0_POS       (10U)                                               /*!< SYSTEM_CORE0_MOD1_CKE_CRC0 bit offset */

/* SYSTEM_CORE0_MOD2_CKE_QSPI0 */
#define SYSTEM_CORE0_MOD2_CKE_QSPI0_POS      (0U)                                                /*!< SYSTEM_CORE0_MOD2_CKE_QSPI0 bit offset */

/* SYSTEM_CORE0_MOD2_CKE_QSPI1 */
#define SYSTEM_CORE0_MOD2_CKE_QSPI1_POS      (1U)                                                /*!< SYSTEM_CORE0_MOD2_CKE_QSPI1 bit offset */

/* SYSTEM_CORE0_MOD2_CKE_SEA_BUS */
#define SYSTEM_CORE0_MOD2_CKE_SEA_BUS_POS    (2U)                                                /*!< SYSTEM_CORE0_MOD2_CKE_SEA_BUS bit offset */

/* SYSTEM_CORE0_MOD2_CKE_SEA_PKE */
#define SYSTEM_CORE0_MOD2_CKE_SEA_PKE_POS    (3U)                                                /*!< SYSTEM_CORE0_MOD2_CKE_SEA_PKE bit offset */

/* SYSTEM_CORE0_MOD2_CKE_SEA_AES */
#define SYSTEM_CORE0_MOD2_CKE_SEA_AES_POS    (4U)                                                /*!< SYSTEM_CORE0_MOD2_CKE_SEA_AES bit offset */

/* SYSTEM_CORE0_MOD2_CKE_SEA_TRNG */
#define SYSTEM_CORE0_MOD2_CKE_SEA_TRNG_POS   (5U)                                                /*!< SYSTEM_CORE0_MOD2_CKE_SEA_TRNG bit offset */

/* SYSTEM_CORE0_MOD2_CKE_SEA_SHA */
#define SYSTEM_CORE0_MOD2_CKE_SEA_SHA_POS    (6U)                                                /*!< SYSTEM_CORE0_MOD2_CKE_SEA_SHA bit offset */

/* SYSTEM_CORE0_MOD2_CKE_KEY_STORE */
#define SYSTEM_CORE0_MOD2_CKE_KEY_STORE_POS  (7U)                                                /*!< SYSTEM_CORE0_MOD2_CKE_KEY_STORE bit offset */

/* SYSTEM_CORE0_MOD2_CKE_AES */
#define SYSTEM_CORE0_MOD2_CKE_AES_POS        (8U)                                                /*!< SYSTEM_CORE0_MOD2_CKE_AES bit offset */

/* SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_POS  (0U)                                                /*!< SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS bit offset */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_CFG(regval)  (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS & ((uint32_t)(regval) << SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_POS))
#define SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_32K768  (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_CFG(0x0))       /*!< Current system main clock is 32.768KHz */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_1M8432  (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_CFG(0x1))       /*!< Current system main clock is 1.8432MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_3M6864  (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_CFG(0x2))       /*!< Current system main clock is 3.6864MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_7M3728  (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_CFG(0x3))       /*!< Current system main clock is 7.3728MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_14M7456  (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_CFG(0x4))       /*!< Current system main clock is 14.7456MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_29M4912  (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_CFG(0x5))       /*!< Current system main clock is 29.4912MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_OVERRANGE  (SYSTEM_CORE0_SYSFREQ_STA_CORE0_SYS_CFG(0x7))       /*!< Current system main clock configuration exceeds allowed operating frequency range */

/* SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_POS  (4U)                                                /*!< SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS bit offset */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_CFG(regval)  (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS & ((uint32_t)(regval) << SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_POS))
#define SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_32K768  (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_CFG(0x0))       /*!< Current system main clock is 32.768KHz */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_1M8432  (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_CFG(0x1))       /*!< Current system main clock is 1.8432MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_3M6864  (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_CFG(0x2))       /*!< Current system main clock is 3.6864MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_7M3728  (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_CFG(0x3))       /*!< Current system main clock is 7.3728MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_14M7456  (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_CFG(0x4))       /*!< Current system main clock is 14.7456MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_29M4912  (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_CFG(0x5))       /*!< Current system main clock is 29.4912MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_58M9824  (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_CFG(0x6))       /*!< Current system main clock is 58.9824MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_OVERRANGE  (SYSTEM_CORE0_SYSFREQ_STA_CORE1_SYS_CFG(0x7))       /*!< Current system main clock configuration exceeds allowed operating frequency range */

/* SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK */
#define SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_POS  (8U)                                                /*!< SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK bit offset */
#define SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_CFG(regval)  (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK & ((uint32_t)(regval) << SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_POS))
#define SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_32K768  (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_CFG(0x0))       /*!< Current Flash clock is 32.768KHz */
#define SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_1M8432  (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_CFG(0x1))       /*!< Current Flash clock is 1.8432MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_3M6864  (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_CFG(0x2))       /*!< Current Flash clock is 3.6864MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_7M3728  (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_CFG(0x3))       /*!< Current Flash clock is 7.3728MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_14M7456  (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_CFG(0x4))       /*!< Current Flash clock is 14.7456MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_29M4912  (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_CFG(0x5))       /*!< Current Flash clock is 29.4912MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_58M9824  (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_CFG(0x6))       /*!< Current Flash clock is 58.9824MHz */
#define SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_OVERRANGE  (SYSTEM_CORE0_SYSFREQ_STA_FLASH_CLK_CFG(0x7))       /*!< Current Flash configuration exceeds allowed operating frequency range */

/* SYSTEM_CORE0_SYSCLK_STA_SYS_ON_LOSC_RCL */
#define SYSTEM_CORE0_SYSCLK_STA_SYS_ON_LOSC_RCL_POS  (0U)                                                /*!< SYSTEM_CORE0_SYSCLK_STA_SYS_ON_LOSC_RCL bit offset */

/* SYSTEM_CORE0_SYSCLK_STA_SYS_ON_RCH */
#define SYSTEM_CORE0_SYSCLK_STA_SYS_ON_RCH_POS  (1U)                                                /*!< SYSTEM_CORE0_SYSCLK_STA_SYS_ON_RCH bit offset */

/* SYSTEM_CORE0_SYSCLK_STA_SYS_ON_PLL_HOSC */
#define SYSTEM_CORE0_SYSCLK_STA_SYS_ON_PLL_HOSC_POS  (2U)                                                /*!< SYSTEM_CORE0_SYSCLK_STA_SYS_ON_PLL_HOSC bit offset */

/* SYSTEM_CORE0_SYSCLK_STA_LOSC_ON_FLAG */
#define SYSTEM_CORE0_SYSCLK_STA_LOSC_ON_FLAG_POS  (4U)                                                /*!< SYSTEM_CORE0_SYSCLK_STA_LOSC_ON_FLAG bit offset */

/* SYSTEM_CORE0_SYSCLK_STA_PLL_ON_FLAG */
#define SYSTEM_CORE0_SYSCLK_STA_PLL_ON_FLAG_POS  (5U)                                                /*!< SYSTEM_CORE0_SYSCLK_STA_PLL_ON_FLAG bit offset */

/* SYSTEM_CORE0_SYSCLK_STA_RCH_ON_FLAG */
#define SYSTEM_CORE0_SYSCLK_STA_RCH_ON_FLAG_POS  (6U)                                                /*!< SYSTEM_CORE0_SYSCLK_STA_RCH_ON_FLAG bit offset */

/* SYSTEM_CORE0_SYSCLK_STA_RCL32K_ON_FLAG */
#define SYSTEM_CORE0_SYSCLK_STA_RCL32K_ON_FLAG_POS  (7U)                                                /*!< SYSTEM_CORE0_SYSCLK_STA_RCL32K_ON_FLAG bit offset */

/* SYSTEM_CORE0_SYSCLK_STA_HOSC_ON_FLAG */
#define SYSTEM_CORE0_SYSCLK_STA_HOSC_ON_FLAG_POS  (8U)                                                /*!< SYSTEM_CORE0_SYSCLK_STA_HOSC_ON_FLAG bit offset */

/* SYSTEM_CORE0_SYSCLK_STA_RCL307K_ON_FLAG */
#define SYSTEM_CORE0_SYSCLK_STA_RCL307K_ON_FLAG_POS  (9U)                                                /*!< SYSTEM_CORE0_SYSCLK_STA_RCL307K_ON_FLAG bit offset */

/* SYSTEM_CORE0_CLK_CAL_CTRL_CAL_START */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_START_POS  (0U)                                                /*!< SYSTEM_CORE0_CLK_CAL_CTRL_CAL_START bit offset */

/* SYSTEM_CORE0_CLK_CAL_CTRL_CAL_STOP */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_STOP_POS  (1U)                                                /*!< SYSTEM_CORE0_CLK_CAL_CTRL_CAL_STOP bit offset */

/* SYSTEM_CORE0_CLK_CAL_CTRL_CAL_SEL */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_SEL_POS  (2U)                                                /*!< SYSTEM_CORE0_CLK_CAL_CTRL_CAL_SEL bit offset */

/* SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL */
#define SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_POS  (4U)                                                /*!< SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL bit offset */
#define SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_CFG(regval)  (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL & ((uint32_t)(regval) << SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_POS))
#define SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_LOSC  (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_CFG(0x0))    /*!< Select LOSC as reference clock */
#define SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_RCL32K  (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_CFG(0x1))    /*!< Select RCL_32K as reference clock */
#define SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_HOSC  (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_CFG(0x2))    /*!< Select HOSC as reference clock */
#define SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_RCHVCC  (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_CFG(0x3))    /*!< Select RCH_VCC as reference clock */
#define SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_PLL  (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_CFG(0x4))    /*!< Select PLL as reference clock */
#define SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_RCHVBAT  (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_CFG(0x5))    /*!< Select RCH_VBAT as reference clock */
#define SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_RCL307K  (SYSTEM_CORE0_CLK_CAL_CTRL_REF_CLK_SEL_CFG(0x6))    /*!< Select RCL_307K as reference clock */

/* SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_POS  (8U)                                                /*!< SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL bit offset */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_CFG(regval)  (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL & ((uint32_t)(regval) << SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_POS))
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_LOSC  (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_CFG(0x0))    /*!< Select LOSC as clock under test */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_RCL32K  (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_CFG(0x1))    /*!< Select RCL_32K as clock under test */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_HOSC  (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_CFG(0x2))    /*!< Select HOSC as clock under test */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_RCHVCC  (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_CFG(0x3))    /*!< Select RCH_VCC as clock under test */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_PLL  (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_CFG(0x4))    /*!< Select PLL as clock under test */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_RCHVBAT  (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_CFG(0x5))    /*!< Select RCH_VBAT as clock under test */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_RCL307K  (SYSTEM_CORE0_CLK_CAL_CTRL_CAL_CLK_SEL_CFG(0x6))    /*!< Select RCL_307K as clock under test */

/* SYSTEM_CORE0_CLK_CAL_CTRL_CAL_DONE_IE */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_DONE_IE_POS  (12U)                                               /*!< SYSTEM_CORE0_CLK_CAL_CTRL_CAL_DONE_IE bit offset */

/* SYSTEM_CORE0_CLK_CAL_CTRL_CAL_OV_IE */
#define SYSTEM_CORE0_CLK_CAL_CTRL_CAL_OV_IE_POS  (13U)                                               /*!< SYSTEM_CORE0_CLK_CAL_CTRL_CAL_OV_IE bit offset */

/* SYSTEM_CORE0_CLK_REF_CNT_REF_CLK_CNT */
#define SYSTEM_CORE0_CLK_REF_CNT_REF_CLK_CNT_POS  (0U)                                                /*!< SYSTEM_CORE0_CLK_REF_CNT_REF_CLK_CNT bit offset */

/* SYSTEM_CORE0_CLK_CAL_CNT_CAL_CLK_CNT */
#define SYSTEM_CORE0_CLK_CAL_CNT_CAL_CLK_CNT_POS  (0U)                                                /*!< SYSTEM_CORE0_CLK_CAL_CNT_CAL_CLK_CNT bit offset */

/* SYSTEM_CORE0_CLK_CAL_STA_CAL_DONE */
#define SYSTEM_CORE0_CLK_CAL_STA_CAL_DONE_POS  (0U)                                                /*!< SYSTEM_CORE0_CLK_CAL_STA_CAL_DONE bit offset */

/* SYSTEM_CORE0_CLK_CAL_STA_CAL_OV */
#define SYSTEM_CORE0_CLK_CAL_STA_CAL_OV_POS  (1U)                                                /*!< SYSTEM_CORE0_CLK_CAL_STA_CAL_OV bit offset */

/* SYSTEM_CORE0_CLK_CAL_STA_AOS_CAL_BUSY */
#define SYSTEM_CORE0_CLK_CAL_STA_AOS_CAL_BUSY_POS  (2U)                                                /*!< SYSTEM_CORE0_CLK_CAL_STA_AOS_CAL_BUSY bit offset */

/* SYSTEM_CORE0_RST_CTRL_CORE0T1_RST_EN */
#define SYSTEM_CORE0_RST_CTRL_CORE0T1_RST_EN_POS  (0U)                                                /*!< SYSTEM_CORE0_RST_CTRL_CORE0T1_RST_EN bit offset */

/* SYSTEM_CORE0_RST_CTRL_CORE1_RST_SEL */
#define SYSTEM_CORE0_RST_CTRL_CORE1_RST_SEL_POS  (16U)                                               /*!< SYSTEM_CORE0_RST_CTRL_CORE1_RST_SEL bit offset */

/* SYSTEM_CORE0_RST_STOP_CTRL_CORE1_STOP */
#define SYSTEM_CORE0_RST_STOP_CTRL_CORE1_STOP_POS  (0U)                                                /*!< SYSTEM_CORE0_RST_STOP_CTRL_CORE1_STOP bit offset */

/* SYSTEM_CORE0_POW_RST_FLAG_VDET5 */
#define SYSTEM_CORE0_POW_RST_FLAG_VDET5_POS  (0U)                                                /*!< SYSTEM_CORE0_POW_RST_FLAG_VDET5 bit offset */

/* SYSTEM_CORE0_POW_RST_FLAG_POR5 */
#define SYSTEM_CORE0_POW_RST_FLAG_POR5_POS   (4U)                                                /*!< SYSTEM_CORE0_POW_RST_FLAG_POR5 bit offset */

/* SYSTEM_CORE0_POW_RST_FLAG_BOR5 */
#define SYSTEM_CORE0_POW_RST_FLAG_BOR5_POS   (5U)                                                /*!< SYSTEM_CORE0_POW_RST_FLAG_BOR5 bit offset */

/* SYSTEM_CORE0_POW_RST_FLAG_PROB5 */
#define SYSTEM_CORE0_POW_RST_FLAG_PROB5_POS  (6U)                                                /*!< SYSTEM_CORE0_POW_RST_FLAG_PROB5 bit offset */

/* SYSTEM_CORE0_POW_RST_FLAG_POR15 */
#define SYSTEM_CORE0_POW_RST_FLAG_POR15_POS  (8U)                                                /*!< SYSTEM_CORE0_POW_RST_FLAG_POR15 bit offset */

/* SYSTEM_CORE0_POW_RST_FLAG_BOR15 */
#define SYSTEM_CORE0_POW_RST_FLAG_BOR15_POS  (9U)                                                /*!< SYSTEM_CORE0_POW_RST_FLAG_BOR15 bit offset */

/* SYSTEM_CORE0_POW_RST_FLAG_PROB15 */
#define SYSTEM_CORE0_POW_RST_FLAG_PROB15_POS  (10U)                                               /*!< SYSTEM_CORE0_POW_RST_FLAG_PROB15 bit offset */

/* SYSTEM_CORE0_SYS_RST_FLAG_POWEROK */
#define SYSTEM_CORE0_SYS_RST_FLAG_POWEROK_POS  (0U)                                                /*!< SYSTEM_CORE0_SYS_RST_FLAG_POWEROK bit offset */

/* SYSTEM_CORE0_SYS_RST_FLAG_PIN */
#define SYSTEM_CORE0_SYS_RST_FLAG_PIN_POS    (1U)                                                /*!< SYSTEM_CORE0_SYS_RST_FLAG_PIN bit offset */

/* SYSTEM_CORE0_SYS_RST_FLAG_CORE0_WDT */
#define SYSTEM_CORE0_SYS_RST_FLAG_CORE0_WDT_POS  (2U)                                                /*!< SYSTEM_CORE0_SYS_RST_FLAG_CORE0_WDT bit offset */

/* SYSTEM_CORE0_SYS_RST_FLAG_CORE1T0_WDT */
#define SYSTEM_CORE0_SYS_RST_FLAG_CORE1T0_WDT_POS  (3U)                                                /*!< SYSTEM_CORE0_SYS_RST_FLAG_CORE1T0_WDT bit offset */

/* SYSTEM_CORE0_SYS_RST_FLAG_CORE0_SOFT */
#define SYSTEM_CORE0_SYS_RST_FLAG_CORE0_SOFT_POS  (4U)                                                /*!< SYSTEM_CORE0_SYS_RST_FLAG_CORE0_SOFT bit offset */

/* SYSTEM_CORE0_SYS_RST_FLAG_CORE1T0_SOFT */
#define SYSTEM_CORE0_SYS_RST_FLAG_CORE1T0_SOFT_POS  (5U)                                                /*!< SYSTEM_CORE0_SYS_RST_FLAG_CORE1T0_SOFT bit offset */

/* SYSTEM_CORE0_SYS_RST_FLAG_BOI_VBAT */
#define SYSTEM_CORE0_SYS_RST_FLAG_BOI_VBAT_POS  (6U)                                                /*!< SYSTEM_CORE0_SYS_RST_FLAG_BOI_VBAT bit offset */

/* SYSTEM_CORE0_BOI_RST_CTRL_BOI_VBAT_RST_IE */
#define SYSTEM_CORE0_BOI_RST_CTRL_BOI_VBAT_RST_IE_POS  (6U)                                                /*!< SYSTEM_CORE0_BOI_RST_CTRL_BOI_VBAT_RST_IE bit offset */

/* SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_EN */
#define SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_EN_POS  (0U)                                                /*!< SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_EN bit offset */

/* SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_FLAG */
#define SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_FLAG_POS  (16U)                                               /*!< SYSTEM_CORE0_UPDATE_RST_CTRL_CORE0_RST_FLAG bit offset */

/* SYSTEM_CORE0_PMOD0_RST_MAILBOX */
#define SYSTEM_CORE0_PMOD0_RST_MAILBOX_POS   (0U)                                                /*!< SYSTEM_CORE0_PMOD0_RST_MAILBOX bit offset */

/* SYSTEM_CORE0_PMOD0_RST_AOS */
#define SYSTEM_CORE0_PMOD0_RST_AOS_POS       (1U)                                                /*!< SYSTEM_CORE0_PMOD0_RST_AOS bit offset */

/* SYSTEM_CORE0_PMOD0_RST_GDMA00 */
#define SYSTEM_CORE0_PMOD0_RST_GDMA00_POS    (2U)                                                /*!< SYSTEM_CORE0_PMOD0_RST_GDMA00 bit offset */

/* SYSTEM_CORE0_PMOD0_RST_GDMA01 */
#define SYSTEM_CORE0_PMOD0_RST_GDMA01_POS    (3U)                                                /*!< SYSTEM_CORE0_PMOD0_RST_GDMA01 bit offset */

/* SYSTEM_CORE0_PMOD0_RST_GPIO */
#define SYSTEM_CORE0_PMOD0_RST_GPIO_POS      (4U)                                                /*!< SYSTEM_CORE0_PMOD0_RST_GPIO bit offset */

/* SYSTEM_CORE0_PMOD0_RST_CRC1 */
#define SYSTEM_CORE0_PMOD0_RST_CRC1_POS      (6U)                                                /*!< SYSTEM_CORE0_PMOD0_RST_CRC1 bit offset */

/* SYSTEM_CORE0_MOD0_RST_UART0 */
#define SYSTEM_CORE0_MOD0_RST_UART0_POS      (0U)                                                /*!< SYSTEM_CORE0_MOD0_RST_UART0 bit offset */

/* SYSTEM_CORE0_MOD0_RST_UART1 */
#define SYSTEM_CORE0_MOD0_RST_UART1_POS      (1U)                                                /*!< SYSTEM_CORE0_MOD0_RST_UART1 bit offset */

/* SYSTEM_CORE0_MOD0_RST_UART2 */
#define SYSTEM_CORE0_MOD0_RST_UART2_POS      (2U)                                                /*!< SYSTEM_CORE0_MOD0_RST_UART2 bit offset */

/* SYSTEM_CORE0_MOD0_RST_UART3 */
#define SYSTEM_CORE0_MOD0_RST_UART3_POS      (3U)                                                /*!< SYSTEM_CORE0_MOD0_RST_UART3 bit offset */

/* SYSTEM_CORE0_MOD0_RST_UART4 */
#define SYSTEM_CORE0_MOD0_RST_UART4_POS      (4U)                                                /*!< SYSTEM_CORE0_MOD0_RST_UART4 bit offset */

/* SYSTEM_CORE0_MOD0_RST_UART5 */
#define SYSTEM_CORE0_MOD0_RST_UART5_POS      (5U)                                                /*!< SYSTEM_CORE0_MOD0_RST_UART5 bit offset */

/* SYSTEM_CORE0_MOD0_RST_UART6 */
#define SYSTEM_CORE0_MOD0_RST_UART6_POS      (6U)                                                /*!< SYSTEM_CORE0_MOD0_RST_UART6 bit offset */

/* SYSTEM_CORE0_MOD0_RST_UART7 */
#define SYSTEM_CORE0_MOD0_RST_UART7_POS      (7U)                                                /*!< SYSTEM_CORE0_MOD0_RST_UART7 bit offset */

/* SYSTEM_CORE0_MOD0_RST_LPUART */
#define SYSTEM_CORE0_MOD0_RST_LPUART_POS     (8U)                                                /*!< SYSTEM_CORE0_MOD0_RST_LPUART bit offset */

/* SYSTEM_CORE0_MOD0_RST_USART0 */
#define SYSTEM_CORE0_MOD0_RST_USART0_POS     (9U)                                                /*!< SYSTEM_CORE0_MOD0_RST_USART0 bit offset */

/* SYSTEM_CORE0_MOD0_RST_USART1 */
#define SYSTEM_CORE0_MOD0_RST_USART1_POS     (10U)                                               /*!< SYSTEM_CORE0_MOD0_RST_USART1 bit offset */

/* SYSTEM_CORE0_MOD0_RST_ISO78160 */
#define SYSTEM_CORE0_MOD0_RST_ISO78160_POS   (11U)                                               /*!< SYSTEM_CORE0_MOD0_RST_ISO78160 bit offset */

/* SYSTEM_CORE0_MOD0_RST_ISO78161 */
#define SYSTEM_CORE0_MOD0_RST_ISO78161_POS   (12U)                                               /*!< SYSTEM_CORE0_MOD0_RST_ISO78161 bit offset */

/* SYSTEM_CORE0_MOD0_RST_SPI0 */
#define SYSTEM_CORE0_MOD0_RST_SPI0_POS       (13U)                                               /*!< SYSTEM_CORE0_MOD0_RST_SPI0 bit offset */

/* SYSTEM_CORE0_MOD0_RST_SPI1 */
#define SYSTEM_CORE0_MOD0_RST_SPI1_POS       (14U)                                               /*!< SYSTEM_CORE0_MOD0_RST_SPI1 bit offset */

/* SYSTEM_CORE0_MOD0_RST_SPI2 */
#define SYSTEM_CORE0_MOD0_RST_SPI2_POS       (15U)                                               /*!< SYSTEM_CORE0_MOD0_RST_SPI2 bit offset */

/* SYSTEM_CORE0_MOD0_RST_SPI3 */
#define SYSTEM_CORE0_MOD0_RST_SPI3_POS       (16U)                                               /*!< SYSTEM_CORE0_MOD0_RST_SPI3 bit offset */

/* SYSTEM_CORE0_MOD0_RST_I2C0 */
#define SYSTEM_CORE0_MOD0_RST_I2C0_POS       (17U)                                               /*!< SYSTEM_CORE0_MOD0_RST_I2C0 bit offset */

/* SYSTEM_CORE0_MOD0_RST_I2C1 */
#define SYSTEM_CORE0_MOD0_RST_I2C1_POS       (18U)                                               /*!< SYSTEM_CORE0_MOD0_RST_I2C1 bit offset */

/* SYSTEM_CORE0_MOD0_RST_CAN */
#define SYSTEM_CORE0_MOD0_RST_CAN_POS        (19U)                                               /*!< SYSTEM_CORE0_MOD0_RST_CAN bit offset */

/* SYSTEM_CORE0_MOD0_RST_IOCNT */
#define SYSTEM_CORE0_MOD0_RST_IOCNT_POS      (20U)                                               /*!< SYSTEM_CORE0_MOD0_RST_IOCNT bit offset */

/* SYSTEM_CORE0_MOD0_RST_LCD */
#define SYSTEM_CORE0_MOD0_RST_LCD_POS        (21U)                                               /*!< SYSTEM_CORE0_MOD0_RST_LCD bit offset */

/* SYSTEM_CORE0_MOD0_RST_DSP */
#define SYSTEM_CORE0_MOD0_RST_DSP_POS        (22U)                                               /*!< SYSTEM_CORE0_MOD0_RST_DSP bit offset */

/* SYSTEM_CORE0_MOD0_RST_M2M */
#define SYSTEM_CORE0_MOD0_RST_M2M_POS        (24U)                                               /*!< SYSTEM_CORE0_MOD0_RST_M2M bit offset */

/* SYSTEM_CORE0_MOD0_RST_D2F */
#define SYSTEM_CORE0_MOD0_RST_D2F_POS        (25U)                                               /*!< SYSTEM_CORE0_MOD0_RST_D2F bit offset */

/* SYSTEM_CORE0_MOD0_RST_ADC0 */
#define SYSTEM_CORE0_MOD0_RST_ADC0_POS       (26U)                                               /*!< SYSTEM_CORE0_MOD0_RST_ADC0 bit offset */

/* SYSTEM_CORE0_MOD0_RST_DAC */
#define SYSTEM_CORE0_MOD0_RST_DAC_POS        (27U)                                               /*!< SYSTEM_CORE0_MOD0_RST_DAC bit offset */

/* SYSTEM_CORE0_MOD0_RST_TC0 */
#define SYSTEM_CORE0_MOD0_RST_TC0_POS        (28U)                                               /*!< SYSTEM_CORE0_MOD0_RST_TC0 bit offset */

/* SYSTEM_CORE0_MOD0_RST_TC1 */
#define SYSTEM_CORE0_MOD0_RST_TC1_POS        (29U)                                               /*!< SYSTEM_CORE0_MOD0_RST_TC1 bit offset */

/* SYSTEM_CORE0_MOD1_RST_SIMPTC0 */
#define SYSTEM_CORE0_MOD1_RST_SIMPTC0_POS    (0U)                                                /*!< SYSTEM_CORE0_MOD1_RST_SIMPTC0 bit offset */

/* SYSTEM_CORE0_MOD1_RST_SIMPTC1 */
#define SYSTEM_CORE0_MOD1_RST_SIMPTC1_POS    (1U)                                                /*!< SYSTEM_CORE0_MOD1_RST_SIMPTC1 bit offset */

/* SYSTEM_CORE0_MOD1_RST_SIMPTC2 */
#define SYSTEM_CORE0_MOD1_RST_SIMPTC2_POS    (2U)                                                /*!< SYSTEM_CORE0_MOD1_RST_SIMPTC2 bit offset */

/* SYSTEM_CORE0_MOD1_RST_SIMPTC3 */
#define SYSTEM_CORE0_MOD1_RST_SIMPTC3_POS    (3U)                                                /*!< SYSTEM_CORE0_MOD1_RST_SIMPTC3 bit offset */

/* SYSTEM_CORE0_MOD1_RST_RS_VDD */
#define SYSTEM_CORE0_MOD1_RST_RS_VDD_POS     (4U)                                                /*!< SYSTEM_CORE0_MOD1_RST_RS_VDD bit offset */

/* SYSTEM_CORE0_MOD1_RST_LSRX */
#define SYSTEM_CORE0_MOD1_RST_LSRX_POS       (6U)                                                /*!< SYSTEM_CORE0_MOD1_RST_LSRX bit offset */

/* SYSTEM_CORE0_MOD1_RST_KBI0 */
#define SYSTEM_CORE0_MOD1_RST_KBI0_POS       (7U)                                                /*!< SYSTEM_CORE0_MOD1_RST_KBI0 bit offset */

/* SYSTEM_CORE0_MOD1_RST_KBI1 */
#define SYSTEM_CORE0_MOD1_RST_KBI1_POS       (8U)                                                /*!< SYSTEM_CORE0_MOD1_RST_KBI1 bit offset */

/* SYSTEM_CORE0_MOD1_RST_LVDCMP */
#define SYSTEM_CORE0_MOD1_RST_LVDCMP_POS     (9U)                                                /*!< SYSTEM_CORE0_MOD1_RST_LVDCMP bit offset */

/* SYSTEM_CORE0_MOD1_RST_CRC0 */
#define SYSTEM_CORE0_MOD1_RST_CRC0_POS       (10U)                                               /*!< SYSTEM_CORE0_MOD1_RST_CRC0 bit offset */

/* SYSTEM_CORE0_MOD2_RST_QSPI0 */
#define SYSTEM_CORE0_MOD2_RST_QSPI0_POS      (0U)                                                /*!< SYSTEM_CORE0_MOD2_RST_QSPI0 bit offset */

/* SYSTEM_CORE0_MOD2_RST_QSPI1 */
#define SYSTEM_CORE0_MOD2_RST_QSPI1_POS      (1U)                                                /*!< SYSTEM_CORE0_MOD2_RST_QSPI1 bit offset */

/* SYSTEM_CORE0_MOD2_RST_SEA */
#define SYSTEM_CORE0_MOD2_RST_SEA_POS        (2U)                                                /*!< SYSTEM_CORE0_MOD2_RST_SEA bit offset */

/* SYSTEM_CORE0_MOD2_RST_AES */
#define SYSTEM_CORE0_MOD2_RST_AES_POS        (8U)                                                /*!< SYSTEM_CORE0_MOD2_RST_AES bit offset */

/* SYSTEM_CORE0_BOR5_CTRL_BOR5_SEL */
#define SYSTEM_CORE0_BOR5_CTRL_BOR5_SEL_POS  (0U)                                                /*!< SYSTEM_CORE0_BOR5_CTRL_BOR5_SEL bit offset */

/* SYSTEM_CORE0_PLVD5_CTRL_PLVD5_IE */
#define SYSTEM_CORE0_PLVD5_CTRL_PLVD5_IE_POS  (0U)                                                /*!< SYSTEM_CORE0_PLVD5_CTRL_PLVD5_IE bit offset */

/* SYSTEM_CORE0_PLVD5_CTRL_PLVD5_FLAG */
#define SYSTEM_CORE0_PLVD5_CTRL_PLVD5_FLAG_POS  (1U)                                                /*!< SYSTEM_CORE0_PLVD5_CTRL_PLVD5_FLAG bit offset */

/* SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0 */
#define SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0_POS  (0U)                                                /*!< SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0 bit offset */
#define SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0_CFG(regval)  (SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0 & ((uint32_t)(regval) << SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0_POS))
#define SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0_0   (SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0_CFG(0x0))         /*!< CORE0 FLASH mapped to address 0x0020_0000 */
#define SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0_1   (SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0_CFG(0x1))         /*!< CORE0 FLASH mapped to address 0x0000_0000 */
#define SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0_2   (SYSTEM_CORE0_ADDR_MAP_CTRL_CORE0_CFG(0x2))         /*!< CORE0 FLASH mapped to address 0x1020_0000 */

/* SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1 */
#define SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1_POS  (4U)                                                /*!< SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1 bit offset */
#define SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1_CFG(regval)  (SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1 & ((uint32_t)(regval) << SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1_POS))
#define SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1_0   (SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1_CFG(0x0))         /*!< CORE1 FLASH mapped to address 0x8000_0000 */
#define SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1_1   (SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1_CFG(0x1))         /*!< CORE1 FLASH mapped to address 0x9000_0000 */
#define SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1_2   (SYSTEM_CORE0_ADDR_MAP_CTRL_CORE1_CFG(0x2))         /*!< CORE1 FLASH mapped to address 0x8100_0000 */

/* SYSTEM_CORE0_FLASH_MAP_CTRL_CORE0_FLASH */
#define SYSTEM_CORE0_FLASH_MAP_CTRL_CORE0_FLASH_POS  (0U)                                                /*!< SYSTEM_CORE0_FLASH_MAP_CTRL_CORE0_FLASH bit offset */

/* SYSTEM_CORE0_FLASH_MAP_CTRL_CORE1_FLASH */
#define SYSTEM_CORE0_FLASH_MAP_CTRL_CORE1_FLASH_POS  (1U)                                                /*!< SYSTEM_CORE0_FLASH_MAP_CTRL_CORE1_FLASH bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH0 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH0_POS  (0U)                                                /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH0 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH1 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH1_POS  (2U)                                                /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH1 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH2 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH2_POS  (4U)                                                /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH2 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH3 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH3_POS  (6U)                                                /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH3 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH4 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH4_POS  (8U)                                                /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH4 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH5 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH5_POS  (10U)                                               /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH5 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH6 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH6_POS  (12U)                                               /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH6 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH7 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH7_POS  (14U)                                               /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH7 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH8 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH8_POS  (16U)                                               /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH8 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH9 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH9_POS  (18U)                                               /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH9 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH10 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH10_POS  (20U)                                               /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH10 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH11 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH11_POS  (22U)                                               /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH11 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH12 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH12_POS  (24U)                                               /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH12 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH13 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH13_POS  (26U)                                               /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH13 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH14 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH14_POS  (28U)                                               /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH14 bit offset */

/* SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH15 */
#define SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH15_POS  (30U)                                               /*!< SYSTEM_CORE0_DMA_PRI0_CTRL_DMA_CH15 bit offset */

/* SYSTEM_CORE0_DMA_PRI1_CTRL_DMA_CH16 */
#define SYSTEM_CORE0_DMA_PRI1_CTRL_DMA_CH16_POS  (0U)                                                /*!< SYSTEM_CORE0_DMA_PRI1_CTRL_DMA_CH16 bit offset */

/* SYSTEM_CORE0_DMA_PRI1_CTRL_DMA_CH17 */
#define SYSTEM_CORE0_DMA_PRI1_CTRL_DMA_CH17_POS  (2U)                                                /*!< SYSTEM_CORE0_DMA_PRI1_CTRL_DMA_CH17 bit offset */

/* SYSTEM_CORE0_DMA_PRI1_CTRL_DMA_CH18 */
#define SYSTEM_CORE0_DMA_PRI1_CTRL_DMA_CH18_POS  (4U)                                                /*!< SYSTEM_CORE0_DMA_PRI1_CTRL_DMA_CH18 bit offset */

/* SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH0 */
#define SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH0_POS  (0U)                                                /*!< SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH0 bit offset */

/* SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH1 */
#define SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH1_POS  (2U)                                                /*!< SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH1 bit offset */

/* SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH2 */
#define SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH2_POS  (4U)                                                /*!< SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH2 bit offset */

/* SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH3 */
#define SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH3_POS  (6U)                                                /*!< SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH3 bit offset */

/* SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH4 */
#define SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH4_POS  (8U)                                                /*!< SYSTEM_CORE0_DMA_PRI2_CTRL_DMA_CH4 bit offset */

/* SYSTEM_CORE0_IRFR_CTRL_INFRARED_CYCLE */
#define SYSTEM_CORE0_IRFR_CTRL_INFRARED_CYCLE_POS  (0U)                                                /*!< SYSTEM_CORE0_IRFR_CTRL_INFRARED_CYCLE bit offset */

/* SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_IE */
#define SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_IE_POS  (0U)                                                /*!< SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_IE bit offset */

/* SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_PENDING */
#define SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_PENDING_POS  (2U)                                                /*!< SYSTEM_CORE0_DIG_REV_CTRL_CORE02CORE1_BRG_PENDING bit offset */

/* SYSTEM_CORE0_CHIP_ID_CHIP_ID */
#define SYSTEM_CORE0_CHIP_ID_CHIP_ID_POS     (0U)                                                /*!< SYSTEM_CORE0_CHIP_ID_CHIP_ID bit offset */

/* SYSTEM_CORE0_DIE_VER_VERSION */
#define SYSTEM_CORE0_DIE_VER_VERSION_POS     (0U)                                                /*!< SYSTEM_CORE0_DIE_VER_VERSION bit offset */

/* SYSTEM_CORE0_DIE_VER_CHIP_MODEL */
#define SYSTEM_CORE0_DIE_VER_CHIP_MODEL_POS  (4U)                                                /*!< SYSTEM_CORE0_DIE_VER_CHIP_MODEL bit offset */

/* SYSTEM_CORE0_FAB_UID0_LOT_NUM */
#define SYSTEM_CORE0_FAB_UID0_LOT_NUM_POS    (0U)                                                /*!< SYSTEM_CORE0_FAB_UID0_LOT_NUM bit offset */

/* SYSTEM_CORE0_FAB_UID1_WAFER_ID */
#define SYSTEM_CORE0_FAB_UID1_WAFER_ID_POS   (0U)                                                /*!< SYSTEM_CORE0_FAB_UID1_WAFER_ID bit offset */

/* SYSTEM_CORE0_FAB_UID1_X_LABEL */
#define SYSTEM_CORE0_FAB_UID1_X_LABEL_POS    (8U)                                                /*!< SYSTEM_CORE0_FAB_UID1_X_LABEL bit offset */

/* SYSTEM_CORE0_FAB_UID1_Y_LABEL */
#define SYSTEM_CORE0_FAB_UID1_Y_LABEL_POS    (20U)                                               /*!< SYSTEM_CORE0_FAB_UID1_Y_LABEL bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_SYSTEM_CORE0_H */
