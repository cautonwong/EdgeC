/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_system_core1.h
 * @author  Renergy Software Team
 * @brief   SYSTEM_CORE1 module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_SYSTEM_CORE1_H
#define _RN831x_RN861x_MCU_SYSTEM_CORE1_H

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

/* SYSTEM_CORE1 */
typedef struct
{
    __IO uint32_t SYS_PSW1;                  /*!< offset: 0x000 info: Public Password Register */
    __IO uint32_t RESERVED0[72];             /*!< offset: 0x004 ~ 120 */
    __IO uint32_t SYS_MODE;                  /*!< offset: 0x124 info: System Mode Register */
    __IO uint32_t RESERVED1;                 /*!< offset: 0x128 */
    __IO uint32_t PMOD0_CKE;                 /*!< offset: 0x12C info: Private Module Clock Enable Register */
    __IO uint32_t RESERVED2;                 /*!< offset: 0x130 */
    __IO uint32_t KBI0_CKE;                  /*!< offset: 0x134 info: KBI0 Clock Enable Register */
    __IO uint32_t KBI1_CKE;                  /*!< offset: 0x138 info: KBI1 Clock Enable Register */
    __IO uint32_t MOD0_CKE;                  /*!< offset: 0x13C info: Module Clock Enable Register 0 */
    __IO uint32_t MOD1_CKE;                  /*!< offset: 0x140 info: Module Clock Enable Register 1 */
    __IO uint32_t MOD2_CKE;                  /*!< offset: 0x144 info: Module Clock Enable Register 2 */
    __IO uint32_t RESERVED3[10];             /*!< offset: 0x148 ~ 16C */
    __IO uint32_t SYSFREQ_STA;               /*!< offset: 0x170 info: System Clock Frequency Indication Register */
    __IO uint32_t RESERVED4[104];            /*!< offset: 0x174 ~ 310 */
    __IO uint32_t SYS_RST_FLAG;              /*!< offset: 0x314 info: System Reset Flag Register */
    __IO uint32_t BOI_RST_CTRL;              /*!< offset: 0x318 info: BOI_VBAT Reset Control Register */
    __IO uint32_t UPDATE_RST_CTRL;           /*!< offset: 0x31C info: Update Reset Control Register */
    __IO uint32_t RESERVED5[4];              /*!< offset: 0x320 ~ 32C */
    __IO uint32_t PMOD0_RST;                 /*!< offset: 0x330 info: CORE0 Private Module Reset Register */
    __IO uint32_t MOD0_RST;                  /*!< offset: 0x334 info: Assignable Module 0 Reset Register */
    __IO uint32_t MOD1_RST;                  /*!< offset: 0x338 info: Assignable Module 1 Reset Register */
    __IO uint32_t MOD2_RST;                  /*!< offset: 0x33C info: Assignable Module 2 Reset Register */
    __IO uint32_t RESERVED6[112];            /*!< offset: 0x340 ~ 4FC */
    __IO uint32_t ADDR_MAP_CTRL;             /*!< offset: 0x500 info: Address Mapping Control Register */
    __IO uint32_t FLASH_MAP_CTRL;            /*!< offset: 0x504 info: FLASH Mapping Control Register */
    __IO uint32_t DMA_PRI0_CTRL;             /*!< offset: 0x508 info: DMA Access SRAM Priority Configuration Register 0 */
    __IO uint32_t DMA_PRI1_CTRL;             /*!< offset: 0x50C info: DMA Access SRAM Priority Configuration Register 1 */
    __IO uint32_t DMA_PRI2_CTRL;             /*!< offset: 0x510 info: DMA Access SRAM Priority Configuration Register 2 */
    __IO uint32_t RESERVED7[2];              /*!< offset: 0x514 ~ 518 */
    __IO uint32_t CORE1_STA;                 /*!< offset: 0x51C info: Core1 STOP status registers */
    __IO uint32_t RESERVED8[184];            /*!< offset: 0x520 ~ 7FC */
    __IO uint32_t CHIP_ID;                   /*!< offset: 0x800 info: CHIP ID Register */
    __IO uint32_t DIE_VER;                   /*!< offset: 0x804 info: Chip Version Register */
    __IO uint32_t FAB_UID0;                  /*!< offset: 0x808 info: Chip Unique ID 0 Register */
    __IO uint32_t FAB_UID1;                  /*!< offset: 0x80C info: Chip Unique ID 1 Register */
} SYSTEM_CORE1_TypeDef;

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
#define SYSTEM_CORE1_BASE (0xA0080000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define SYSTEM_CORE1 ((SYSTEM_CORE1_TypeDef *)SYSTEM_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE1)
#define SYSTEM  SYSTEM_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** SYSTEM_CORE1 Instances *******************************/
#define IS_SYSTEM_CORE1_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == SYSTEM_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* SYSTEM_CORE1_SYS_PSW1 */
#define SYSTEM_CORE1_SYS_PSW1_SYS_PSW1       (BITS(0,7))                                         /*!< When SYS_PSW1 = 8'h82, public registers are writable. */

/* SYSTEM_CORE1_SYS_MODE */
#define SYSTEM_CORE1_SYS_MODE_SYS_MODE       (BITS(0,3))                                         /*!< HCM/RC/LCM mode */
#define SYSTEM_CORE1_SYS_MODE_FLASH_BUSY     (BIT(4))                                            /*!< Flash busy status */

/* SYSTEM_CORE1_PMOD0_CKE */
#define SYSTEM_CORE1_PMOD0_CKE_WDT           (BIT(0))                                            /*!< CORE0_WDT APB clock enable */
#define SYSTEM_CORE1_PMOD0_CKE_MAILBOX       (BIT(1))                                            /*!< CORE0_MAILBOX module clock enable */
#define SYSTEM_CORE1_PMOD0_CKE_AOS           (BIT(2))                                            /*!< CORE0_AOS module clock enable */
#define SYSTEM_CORE1_PMOD0_CKE_SCU           (BIT(3))                                            /*!< CORE0_SCU module clock enable */
#define SYSTEM_CORE1_PMOD0_CKE_GIC           (BIT(4))                                            /*!< CORE0_GIC module clock enable */
#define SYSTEM_CORE1_PMOD0_CKE_GDMA10        (BIT(8))                                            /*!< GDMA10 module clock enable */
#define SYSTEM_CORE1_PMOD0_CKE_GDMA11        (BIT(9))                                            /*!< GDMA11 module clock enable */
#define SYSTEM_CORE1_PMOD0_CKE_UART38K       (BIT(10))                                           /*!< UART38K module clock enable */
#define SYSTEM_CORE1_PMOD0_CKE_GPIO          (BIT(11))                                           /*!< GPIO module enable */
#define SYSTEM_CORE1_PMOD0_CKE_HSEM          (BIT(12))                                           /*!< HSEM module enable */

/* SYSTEM_CORE1_KBI0_CKE */
#define SYSTEM_CORE1_KBI0_CKE_KBI00          (BIT(0))                                            /*!< KBI00 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI01          (BIT(1))                                            /*!< KBI01 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI02          (BIT(2))                                            /*!< KBI02 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI03          (BIT(3))                                            /*!< KBI03 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI04          (BIT(4))                                            /*!< KBI04 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI05          (BIT(5))                                            /*!< KBI05 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI06          (BIT(6))                                            /*!< KBI06 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI07          (BIT(7))                                            /*!< KBI07 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI08          (BIT(8))                                            /*!< KBI08 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI09          (BIT(9))                                            /*!< KBI09 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI010         (BIT(10))                                           /*!< KBI010 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI011         (BIT(11))                                           /*!< KBI011 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI012         (BIT(12))                                           /*!< KBI012 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI013         (BIT(13))                                           /*!< KBI013 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI014         (BIT(14))                                           /*!< KBI014 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBI015         (BIT(15))                                           /*!< KBI015 module clock enable */
#define SYSTEM_CORE1_KBI0_CKE_KBIC0          (BIT(31))                                           /*!< KBIC0 APB module clock enable */

/* SYSTEM_CORE1_KBI1_CKE */
#define SYSTEM_CORE1_KBI1_CKE_KBI10          (BIT(0))                                            /*!< KBI00 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI11          (BIT(1))                                            /*!< KBI01 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI12          (BIT(2))                                            /*!< KBI02 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI13          (BIT(3))                                            /*!< KBI03 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI14          (BIT(4))                                            /*!< KBI04 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI15          (BIT(5))                                            /*!< KBI05 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI16          (BIT(6))                                            /*!< KBI06 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI17          (BIT(7))                                            /*!< KBI07 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI18          (BIT(8))                                            /*!< KBI08 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI19          (BIT(9))                                            /*!< KBI09 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI110         (BIT(10))                                           /*!< KBI110 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI111         (BIT(11))                                           /*!< KBI111 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI112         (BIT(12))                                           /*!< KBI112 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI113         (BIT(13))                                           /*!< KBI113 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI114         (BIT(14))                                           /*!< KBI114 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBI115         (BIT(15))                                           /*!< KBI115 module clock enable */
#define SYSTEM_CORE1_KBI1_CKE_KBIC1          (BIT(31))                                           /*!< KBIC1 APB module clock enable */

/* SYSTEM_CORE1_MOD0_CKE */
#define SYSTEM_CORE1_MOD0_CKE_UART0          (BIT(0))                                            /*!< UART0 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_UART1          (BIT(1))                                            /*!< UART1 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_UART2          (BIT(2))                                            /*!< UART2 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_UART3          (BIT(3))                                            /*!< UART3 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_UART4          (BIT(4))                                            /*!< UART4 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_UART5          (BIT(5))                                            /*!< UART5 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_UART6          (BIT(6))                                            /*!< UART6 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_UART7          (BIT(7))                                            /*!< UART7 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_LPUART         (BIT(8))                                            /*!< LPUART module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_USART0         (BIT(9))                                            /*!< USART0 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_USART1         (BIT(10))                                           /*!< USART1 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_ISO78160       (BIT(11))                                           /*!< ISO7816_0 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_ISO78161       (BIT(12))                                           /*!< ISO7816_1 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_SPI0           (BIT(13))                                           /*!< SPI0 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_SPI1           (BIT(14))                                           /*!< SPI1 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_SPI2           (BIT(15))                                           /*!< SPI2 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_SPI3           (BIT(16))                                           /*!< SPI3 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_I2C0           (BIT(17))                                           /*!< I2C0 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_I2C1           (BIT(18))                                           /*!< I2C1 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_IOCNT          (BIT(20))                                           /*!< IOCNT module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_LCD            (BIT(21))                                           /*!< LCD module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_DSP            (BIT(22))                                           /*!< DSP module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_M2M            (BIT(24))                                           /*!< M2M module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_D2F            (BIT(25))                                           /*!< D2F module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_ADC0           (BIT(26))                                           /*!< ADC0 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_DAC            (BIT(27))                                           /*!< DAC module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_TC0            (BIT(28))                                           /*!< TC0 module clock enable */
#define SYSTEM_CORE1_MOD0_CKE_TC1            (BIT(29))                                           /*!< TC1 module clock enable */

/* SYSTEM_CORE1_MOD1_CKE */
#define SYSTEM_CORE1_MOD1_CKE_SIMPTC0        (BIT(0))                                            /*!< SIMP_TC0 Module Clock Enable */
#define SYSTEM_CORE1_MOD1_CKE_SIMPTC1        (BIT(1))                                            /*!< SIMP_TC1 Module Clock Enable */
#define SYSTEM_CORE1_MOD1_CKE_SIMPTC2        (BIT(2))                                            /*!< SIMP_TC2 Module Clock Enable */
#define SYSTEM_CORE1_MOD1_CKE_SIMPTC3        (BIT(3))                                            /*!< SIMP_TC3 Module Clock Enable */
#define SYSTEM_CORE1_MOD1_CKE_RS_VDD         (BIT(4))                                            /*!< RS_VDD Module Clock Enable */
#define SYSTEM_CORE1_MOD1_CKE_LSRX           (BIT(6))                                            /*!< LSRX Module Clock Enable */
#define SYSTEM_CORE1_MOD1_CKE_LVDCMP         (BIT(9))                                            /*!< LVDCMP Module Clock Enable */
#define SYSTEM_CORE1_MOD1_CKE_CRC0           (BIT(10))                                           /*!< CRC0 Module Clock Enable */

/* SYSTEM_CORE1_MOD2_CKE */
#define SYSTEM_CORE1_MOD2_CKE_QSPI0          (BIT(0))                                            /*!< QSPI0 Module Clock Enable */
#define SYSTEM_CORE1_MOD2_CKE_QSPI1          (BIT(1))                                            /*!< QSPI1 Module Clock Enable */
#define SYSTEM_CORE1_MOD2_CKE_SEA_BUS        (BIT(2))                                            /*!< SEA_BUS Module Clock Enable */
#define SYSTEM_CORE1_MOD2_CKE_SEA_PKE        (BIT(3))                                            /*!< SEA_PKE Module Clock Enable */
#define SYSTEM_CORE1_MOD2_CKE_SEA_AES        (BIT(4))                                            /*!< SEA_AES Module Clock Enable */
#define SYSTEM_CORE1_MOD2_CKE_SEA_TRNG       (BIT(5))                                            /*!< SEA_TRNG Module Clock Enable */
#define SYSTEM_CORE1_MOD2_CKE_SEA_SHA        (BIT(6))                                            /*!< SEA_SHA Module Clock Enable */
#define SYSTEM_CORE1_MOD2_CKE_KEY_STORE      (BIT(7))                                            /*!< KEY_STORE Module Clock Enable */
#define SYSTEM_CORE1_MOD2_CKE_AES            (BIT(8))                                            /*!< AES Module Clock Enable */

/* SYSTEM_CORE1_SYSFREQ_STA */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS   (BITS(0,2))                                         /*!< CORE0 System Main Clock Frequency Indication */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS   (BITS(4,6))                                         /*!< CORE1 System Main Clock Frequency Indication */
#define SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK   (BITS(8,10))                                        /*!< Flash System Frequency Indication */

/* SYSTEM_CORE1_SYS_RST_FLAG */
#define SYSTEM_CORE1_SYS_RST_FLAG_POWEROK    (BIT(0))                                            /*!< Power Reset Flag */
#define SYSTEM_CORE1_SYS_RST_FLAG_PIN        (BIT(1))                                            /*!< External Pin Reset Flag */
#define SYSTEM_CORE1_SYS_RST_FLAG_CORE0_WDT  (BIT(2))                                            /*!< CORE0 WDT Reset Flag */
#define SYSTEM_CORE1_SYS_RST_FLAG_CORE1T0_WDT  (BIT(3))                                            /*!< CORE1T0 WDT Reset Flag */
#define SYSTEM_CORE1_SYS_RST_FLAG_CORE0_SOFT  (BIT(4))                                            /*!< CORE0 Software Reset Flag */
#define SYSTEM_CORE1_SYS_RST_FLAG_CORE1T0_SOFT  (BIT(5))                                            /*!< CORE1T0 Software Reset Flag */
#define SYSTEM_CORE1_SYS_RST_FLAG_BOI_VBAT   (BIT(6))                                            /*!< VBAT BOI Power-Down Reset Occurred */
#define SYSTEM_CORE1_SYS_RST_FLAG_CORE1_WDT  (BIT(7))                                            /*!< CORE1 WDT Reset Flag */
#define SYSTEM_CORE1_SYS_RST_FLAG_CORE1_SOFT  (BIT(8))                                            /*!< CORE1 Software Reset Flag */
#define SYSTEM_CORE1_SYS_RST_FLAG_CORE0T1_SOFT  (BIT(9))                                            /*!< CORE0T1 Software Reset Flag */

/* SYSTEM_CORE1_BOI_RST_CTRL */
#define SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_PENDING  (BIT(0))                                            /*!< Core1 Reset Flag */
#define SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_IE  (BIT(1))                                            /*!< Core1 Reset Enable */
#define SYSTEM_CORE1_BOI_RST_CTRL_BOI_VBAT_RST_IE  (BIT(4))                                            /*!< BOI_VBAT_RST Interrupt Enable */

/* SYSTEM_CORE1_UPDATE_RST_CTRL */
#define SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_EN  (BITS(0,15))                                        /*!< CORE0 Update Reset Enable */
#define SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_FLAG  (BIT(16))                                           /*!< CORE0 Update Reset Flag */

/* SYSTEM_CORE1_PMOD0_RST */
#define SYSTEM_CORE1_PMOD0_RST_MAILBOX       (BIT(0))                                            /*!< Software reset MAILBOX module */
#define SYSTEM_CORE1_PMOD0_RST_AOS           (BIT(1))                                            /*!< Software reset AOS module */
#define SYSTEM_CORE1_PMOD0_RST_GDMA10        (BIT(2))                                            /*!< Software reset GDMA10 module */
#define SYSTEM_CORE1_PMOD0_RST_GDMA11        (BIT(3))                                            /*!< Software reset GDMA11 module */
#define SYSTEM_CORE1_PMOD0_RST_HSEM          (BIT(4))                                            /*!< Software reset HSEM module */

/* SYSTEM_CORE1_MOD0_RST */
#define SYSTEM_CORE1_MOD0_RST_UART0          (BIT(0))                                            /*!< Software reset UART0 module */
#define SYSTEM_CORE1_MOD0_RST_UART1          (BIT(1))                                            /*!< Software reset UART1 module */
#define SYSTEM_CORE1_MOD0_RST_UART2          (BIT(2))                                            /*!< Software reset UART2 module */
#define SYSTEM_CORE1_MOD0_RST_UART3          (BIT(3))                                            /*!< Software reset UART3 module */
#define SYSTEM_CORE1_MOD0_RST_UART4          (BIT(4))                                            /*!< Software reset UART4 module */
#define SYSTEM_CORE1_MOD0_RST_UART5          (BIT(5))                                            /*!< Software reset UART5 module */
#define SYSTEM_CORE1_MOD0_RST_UART6          (BIT(6))                                            /*!< Software reset UART6 module */
#define SYSTEM_CORE1_MOD0_RST_UART7          (BIT(7))                                            /*!< Software reset UART7 module */
#define SYSTEM_CORE1_MOD0_RST_LPUART         (BIT(8))                                            /*!< Software reset LPUART module */
#define SYSTEM_CORE1_MOD0_RST_USART0         (BIT(9))                                            /*!< Software reset USART0 module */
#define SYSTEM_CORE1_MOD0_RST_USART1         (BIT(10))                                           /*!< Software reset USART1 module */
#define SYSTEM_CORE1_MOD0_RST_ISO78160       (BIT(11))                                           /*!< Software reset ISO78160 module */
#define SYSTEM_CORE1_MOD0_RST_ISO78161       (BIT(12))                                           /*!< Software reset ISO78161 module */
#define SYSTEM_CORE1_MOD0_RST_SPI0           (BIT(13))                                           /*!< Software reset SPI0 module */
#define SYSTEM_CORE1_MOD0_RST_SPI1           (BIT(14))                                           /*!< Software reset SPI1 module */
#define SYSTEM_CORE1_MOD0_RST_SPI2           (BIT(15))                                           /*!< Software reset SPI2 module */
#define SYSTEM_CORE1_MOD0_RST_SPI3           (BIT(16))                                           /*!< Software reset SPI3 module */
#define SYSTEM_CORE1_MOD0_RST_I2C0           (BIT(17))                                           /*!< Software reset I2C0 module */
#define SYSTEM_CORE1_MOD0_RST_I2C1           (BIT(18))                                           /*!< Software reset I2C1 module */
#define SYSTEM_CORE1_MOD0_RST_CAN            (BIT(19))                                           /*!< Software reset CAN module */
#define SYSTEM_CORE1_MOD0_RST_IOCNT          (BIT(20))                                           /*!< Software reset IOCNT module */
#define SYSTEM_CORE1_MOD0_RST_LCD            (BIT(21))                                           /*!< Software reset LCD module */
#define SYSTEM_CORE1_MOD0_RST_DSP            (BIT(22))                                           /*!< Software reset DSP module */
#define SYSTEM_CORE1_MOD0_RST_M2M            (BIT(24))                                           /*!< Software reset M2M module */
#define SYSTEM_CORE1_MOD0_RST_D2F            (BIT(25))                                           /*!< Software reset D2F module */
#define SYSTEM_CORE1_MOD0_RST_ADC0           (BIT(26))                                           /*!< Software reset ADC0 module */
#define SYSTEM_CORE1_MOD0_RST_DAC            (BIT(27))                                           /*!< Software reset DAC module */
#define SYSTEM_CORE1_MOD0_RST_TC0            (BIT(28))                                           /*!< Software reset TC0 module */
#define SYSTEM_CORE1_MOD0_RST_TC1            (BIT(29))                                           /*!< Software reset TC1 module */

/* SYSTEM_CORE1_MOD1_RST */
#define SYSTEM_CORE1_MOD1_RST_SIMPTC0        (BIT(0))                                            /*!< Software reset SIMP_TC0 module */
#define SYSTEM_CORE1_MOD1_RST_SIMPTC1        (BIT(1))                                            /*!< Software reset SIMP_TC1 module */
#define SYSTEM_CORE1_MOD1_RST_SIMPTC2        (BIT(2))                                            /*!< Software reset SIMP_TC2 module */
#define SYSTEM_CORE1_MOD1_RST_SIMPTC3        (BIT(3))                                            /*!< Software reset SIMP_TC3 module */
#define SYSTEM_CORE1_MOD1_RST_RS_VDD         (BIT(4))                                            /*!< Software reset RS_VDD module */
#define SYSTEM_CORE1_MOD1_RST_LSRX           (BIT(6))                                            /*!< Software reset LSRX module */
#define SYSTEM_CORE1_MOD1_RST_KBI0           (BIT(7))                                            /*!< Software reset KBI0 module */
#define SYSTEM_CORE1_MOD1_RST_KBI1           (BIT(8))                                            /*!< Software reset KBI1 module */
#define SYSTEM_CORE1_MOD1_RST_LVDCMP         (BIT(9))                                            /*!< Software reset LVDCMP module */
#define SYSTEM_CORE1_MOD1_RST_CRC0           (BIT(10))                                           /*!< Software reset CRC0 module */

/* SYSTEM_CORE1_MOD2_RST */
#define SYSTEM_CORE1_MOD2_RST_QSPI0          (BIT(0))                                            /*!< Software reset QSPI0 module */
#define SYSTEM_CORE1_MOD2_RST_QSPI1          (BIT(1))                                            /*!< Software reset QSPI1 module */
#define SYSTEM_CORE1_MOD2_RST_SEA            (BIT(2))                                            /*!< Software reset SEA module */
#define SYSTEM_CORE1_MOD2_RST_AES            (BIT(8))                                            /*!< Software reset AES module */

/* SYSTEM_CORE1_ADDR_MAP_CTRL */
#define SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0     (BITS(0,2))                                         /*!< Core0 Address ReMapping */
#define SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1     (BITS(4,6))                                         /*!< Core1 Address ReMapping */

/* SYSTEM_CORE1_FLASH_MAP_CTRL */
#define SYSTEM_CORE1_FLASH_MAP_CTRL_CORE0_FLASH  (BIT(0))                                            /*!< Core0 Flash Address Mapping */
#define SYSTEM_CORE1_FLASH_MAP_CTRL_CORE1_FLASH  (BIT(1))                                            /*!< Core1 Flash Address Mapping */

/* SYSTEM_CORE1_DMA_PRI0_CTRL */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH0   (BITS(0,1))                                         /*!< Channel 0: SPI0 channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH1   (BITS(2,3))                                         /*!< Channel 1: SPI1 channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH2   (BITS(4,5))                                         /*!< Channel 2: SPI2 channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH3   (BITS(6,7))                                         /*!< Channel 3: SPI3 channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH4   (BITS(8,9))                                         /*!< Channel 4: SARADC channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH5   (BITS(10,11))                                       /*!< Channel 5: DAC channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH6   (BITS(12,13))                                       /*!< Channel 6: UART0 channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH7   (BITS(14,15))                                       /*!< Channel 7: UART1 channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH8   (BITS(16,17))                                       /*!< Channel 8: UART2 channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH9   (BITS(18,19))                                       /*!< Channel 9: UART3 channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH10  (BITS(20,21))                                       /*!< Channel 10: UART4 channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH11  (BITS(22,23))                                       /*!< Channel 11: UART5 channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH12  (BITS(24,25))                                       /*!< Channel 12: UART6 channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH13  (BITS(26,27))                                       /*!< Channel 13: UART7 channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH14  (BITS(28,29))                                       /*!< Channel 14: CAN channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH15  (BITS(30,31))                                       /*!< Channel 15: TCO_PWM channel access SRAM priority configuration. */

/* SYSTEM_CORE1_DMA_PRI1_CTRL */
#define SYSTEM_CORE1_DMA_PRI1_CTRL_DMA_CH16  (BITS(0,1))                                         /*!< Channel 16: TC1_PWM channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI1_CTRL_DMA_CH17  (BITS(2,3))                                         /*!< Channel 17: AES channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI1_CTRL_DMA_CH18  (BITS(4,5))                                         /*!< Channel 18: M2M channel access SRAM priority configuration. */

/* SYSTEM_CORE1_DMA_PRI2_CTRL */
#define SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH0   (BITS(0,1))                                         /*!< Channel 0: First level arbitration result channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH1   (BITS(2,3))                                         /*!< Channel 1: General DMA read channel (MDMA0x_RD/MDMA1x_RD) access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH2   (BITS(4,5))                                         /*!< Channel 2: General DMA write channel (MDMA0x_WR/MDMA1x_WR) access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH3   (BITS(6,7))                                         /*!< Channel 3: DSP channel access SRAM priority configuration. */
#define SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH4   (BITS(8,9))                                         /*!< Channel 4: CORE0/CORE1 channel access SRAM priority configuration. */

/* SYSTEM_CORE1_CORE1_STA */
#define SYSTEM_CORE1_CORE1_STA_CORE1_STOP_FLAG  (BIT(0))                                            /*!< Core1 STOP State Indication */

/* SYSTEM_CORE1_CHIP_ID */
#define SYSTEM_CORE1_CHIP_ID_CHIP_ID         (BITS(0,15))                                        /*!< CHIP_ID: C248 */

/* SYSTEM_CORE1_DIE_VER */
#define SYSTEM_CORE1_DIE_VER_VERSION         (BITS(0,3))                                         /*!< Version Number */
#define SYSTEM_CORE1_DIE_VER_CHIP_MODEL      (BITS(4,31))                                        /*!< Chip Model: A9701C, Encoded in ASCII */

/* SYSTEM_CORE1_FAB_UID0 */
#define SYSTEM_CORE1_FAB_UID0_LOT_NUM        (BITS(0,31))                                        /*!< This register is configured by the bootloader */

/* SYSTEM_CORE1_FAB_UID1 */
#define SYSTEM_CORE1_FAB_UID1_WAFER_ID       (BITS(0,7))                                         /*!< the wafer WAFER ID information */
#define SYSTEM_CORE1_FAB_UID1_X_LABEL        (BITS(8,19))                                        /*!< DIE X-coordinate */
#define SYSTEM_CORE1_FAB_UID1_Y_LABEL        (BITS(20,31))                                       /*!< DIE Y-coordinate */

/* constants definitions */
/* SYSTEM_CORE1_SYS_PSW1_SYS_PSW1 */
#define SYSTEM_CORE1_SYS_PSW1_SYS_PSW1_POS   (0U)                                                /*!< SYSTEM_CORE1_SYS_PSW1_SYS_PSW1 bit offset */

/* SYSTEM_CORE1_SYS_MODE_SYS_MODE */
#define SYSTEM_CORE1_SYS_MODE_SYS_MODE_POS   (0U)                                                /*!< SYSTEM_CORE1_SYS_MODE_SYS_MODE bit offset */
#define SYSTEM_CORE1_SYS_MODE_SYS_MODE_CFG(regval)  (SYSTEM_CORE1_SYS_MODE_SYS_MODE & ((uint32_t)(regval) << SYSTEM_CORE1_SYS_MODE_SYS_MODE_POS))
#define SYSTEM_CORE1_SYS_MODE_SYS_MODE_HCM   (SYSTEM_CORE1_SYS_MODE_SYS_MODE_CFG(0x4))           /*!< High-frequency mode HCM, read value 4'b0100 */
#define SYSTEM_CORE1_SYS_MODE_SYS_MODE_RCM   (SYSTEM_CORE1_SYS_MODE_SYS_MODE_CFG(0x2))           /*!< RC mode RCM, read value 4'b0010 */
#define SYSTEM_CORE1_SYS_MODE_SYS_MODE_LCM   (SYSTEM_CORE1_SYS_MODE_SYS_MODE_CFG(0x1))           /*!< 32.768KHz mode LCM, read value 4'b0001 */

/* SYSTEM_CORE1_SYS_MODE_FLASH_BUSY */
#define SYSTEM_CORE1_SYS_MODE_FLASH_BUSY_POS  (4U)                                                /*!< SYSTEM_CORE1_SYS_MODE_FLASH_BUSY bit offset */

/* SYSTEM_CORE1_PMOD0_CKE_WDT */
#define SYSTEM_CORE1_PMOD0_CKE_WDT_POS       (0U)                                                /*!< SYSTEM_CORE1_PMOD0_CKE_WDT bit offset */

/* SYSTEM_CORE1_PMOD0_CKE_MAILBOX */
#define SYSTEM_CORE1_PMOD0_CKE_MAILBOX_POS   (1U)                                                /*!< SYSTEM_CORE1_PMOD0_CKE_MAILBOX bit offset */

/* SYSTEM_CORE1_PMOD0_CKE_AOS */
#define SYSTEM_CORE1_PMOD0_CKE_AOS_POS       (2U)                                                /*!< SYSTEM_CORE1_PMOD0_CKE_AOS bit offset */

/* SYSTEM_CORE1_PMOD0_CKE_SCU */
#define SYSTEM_CORE1_PMOD0_CKE_SCU_POS       (3U)                                                /*!< SYSTEM_CORE1_PMOD0_CKE_SCU bit offset */

/* SYSTEM_CORE1_PMOD0_CKE_GIC */
#define SYSTEM_CORE1_PMOD0_CKE_GIC_POS       (4U)                                                /*!< SYSTEM_CORE1_PMOD0_CKE_GIC bit offset */

/* SYSTEM_CORE1_PMOD0_CKE_GDMA10 */
#define SYSTEM_CORE1_PMOD0_CKE_GDMA10_POS    (8U)                                                /*!< SYSTEM_CORE1_PMOD0_CKE_GDMA10 bit offset */

/* SYSTEM_CORE1_PMOD0_CKE_GDMA11 */
#define SYSTEM_CORE1_PMOD0_CKE_GDMA11_POS    (9U)                                                /*!< SYSTEM_CORE1_PMOD0_CKE_GDMA11 bit offset */

/* SYSTEM_CORE1_PMOD0_CKE_UART38K */
#define SYSTEM_CORE1_PMOD0_CKE_UART38K_POS   (10U)                                               /*!< SYSTEM_CORE1_PMOD0_CKE_UART38K bit offset */

/* SYSTEM_CORE1_PMOD0_CKE_GPIO */
#define SYSTEM_CORE1_PMOD0_CKE_GPIO_POS      (11U)                                               /*!< SYSTEM_CORE1_PMOD0_CKE_GPIO bit offset */

/* SYSTEM_CORE1_PMOD0_CKE_HSEM */
#define SYSTEM_CORE1_PMOD0_CKE_HSEM_POS      (12U)                                               /*!< SYSTEM_CORE1_PMOD0_CKE_HSEM bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI00 */
#define SYSTEM_CORE1_KBI0_CKE_KBI00_POS      (0U)                                                /*!< SYSTEM_CORE1_KBI0_CKE_KBI00 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI01 */
#define SYSTEM_CORE1_KBI0_CKE_KBI01_POS      (1U)                                                /*!< SYSTEM_CORE1_KBI0_CKE_KBI01 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI02 */
#define SYSTEM_CORE1_KBI0_CKE_KBI02_POS      (2U)                                                /*!< SYSTEM_CORE1_KBI0_CKE_KBI02 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI03 */
#define SYSTEM_CORE1_KBI0_CKE_KBI03_POS      (3U)                                                /*!< SYSTEM_CORE1_KBI0_CKE_KBI03 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI04 */
#define SYSTEM_CORE1_KBI0_CKE_KBI04_POS      (4U)                                                /*!< SYSTEM_CORE1_KBI0_CKE_KBI04 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI05 */
#define SYSTEM_CORE1_KBI0_CKE_KBI05_POS      (5U)                                                /*!< SYSTEM_CORE1_KBI0_CKE_KBI05 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI06 */
#define SYSTEM_CORE1_KBI0_CKE_KBI06_POS      (6U)                                                /*!< SYSTEM_CORE1_KBI0_CKE_KBI06 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI07 */
#define SYSTEM_CORE1_KBI0_CKE_KBI07_POS      (7U)                                                /*!< SYSTEM_CORE1_KBI0_CKE_KBI07 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI08 */
#define SYSTEM_CORE1_KBI0_CKE_KBI08_POS      (8U)                                                /*!< SYSTEM_CORE1_KBI0_CKE_KBI08 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI09 */
#define SYSTEM_CORE1_KBI0_CKE_KBI09_POS      (9U)                                                /*!< SYSTEM_CORE1_KBI0_CKE_KBI09 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI010 */
#define SYSTEM_CORE1_KBI0_CKE_KBI010_POS     (10U)                                               /*!< SYSTEM_CORE1_KBI0_CKE_KBI010 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI011 */
#define SYSTEM_CORE1_KBI0_CKE_KBI011_POS     (11U)                                               /*!< SYSTEM_CORE1_KBI0_CKE_KBI011 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI012 */
#define SYSTEM_CORE1_KBI0_CKE_KBI012_POS     (12U)                                               /*!< SYSTEM_CORE1_KBI0_CKE_KBI012 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI013 */
#define SYSTEM_CORE1_KBI0_CKE_KBI013_POS     (13U)                                               /*!< SYSTEM_CORE1_KBI0_CKE_KBI013 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI014 */
#define SYSTEM_CORE1_KBI0_CKE_KBI014_POS     (14U)                                               /*!< SYSTEM_CORE1_KBI0_CKE_KBI014 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBI015 */
#define SYSTEM_CORE1_KBI0_CKE_KBI015_POS     (15U)                                               /*!< SYSTEM_CORE1_KBI0_CKE_KBI015 bit offset */

/* SYSTEM_CORE1_KBI0_CKE_KBIC0 */
#define SYSTEM_CORE1_KBI0_CKE_KBIC0_POS      (31U)                                               /*!< SYSTEM_CORE1_KBI0_CKE_KBIC0 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI10 */
#define SYSTEM_CORE1_KBI1_CKE_KBI10_POS      (0U)                                                /*!< SYSTEM_CORE1_KBI1_CKE_KBI10 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI11 */
#define SYSTEM_CORE1_KBI1_CKE_KBI11_POS      (1U)                                                /*!< SYSTEM_CORE1_KBI1_CKE_KBI11 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI12 */
#define SYSTEM_CORE1_KBI1_CKE_KBI12_POS      (2U)                                                /*!< SYSTEM_CORE1_KBI1_CKE_KBI12 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI13 */
#define SYSTEM_CORE1_KBI1_CKE_KBI13_POS      (3U)                                                /*!< SYSTEM_CORE1_KBI1_CKE_KBI13 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI14 */
#define SYSTEM_CORE1_KBI1_CKE_KBI14_POS      (4U)                                                /*!< SYSTEM_CORE1_KBI1_CKE_KBI14 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI15 */
#define SYSTEM_CORE1_KBI1_CKE_KBI15_POS      (5U)                                                /*!< SYSTEM_CORE1_KBI1_CKE_KBI15 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI16 */
#define SYSTEM_CORE1_KBI1_CKE_KBI16_POS      (6U)                                                /*!< SYSTEM_CORE1_KBI1_CKE_KBI16 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI17 */
#define SYSTEM_CORE1_KBI1_CKE_KBI17_POS      (7U)                                                /*!< SYSTEM_CORE1_KBI1_CKE_KBI17 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI18 */
#define SYSTEM_CORE1_KBI1_CKE_KBI18_POS      (8U)                                                /*!< SYSTEM_CORE1_KBI1_CKE_KBI18 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI19 */
#define SYSTEM_CORE1_KBI1_CKE_KBI19_POS      (9U)                                                /*!< SYSTEM_CORE1_KBI1_CKE_KBI19 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI110 */
#define SYSTEM_CORE1_KBI1_CKE_KBI110_POS     (10U)                                               /*!< SYSTEM_CORE1_KBI1_CKE_KBI110 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI111 */
#define SYSTEM_CORE1_KBI1_CKE_KBI111_POS     (11U)                                               /*!< SYSTEM_CORE1_KBI1_CKE_KBI111 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI112 */
#define SYSTEM_CORE1_KBI1_CKE_KBI112_POS     (12U)                                               /*!< SYSTEM_CORE1_KBI1_CKE_KBI112 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI113 */
#define SYSTEM_CORE1_KBI1_CKE_KBI113_POS     (13U)                                               /*!< SYSTEM_CORE1_KBI1_CKE_KBI113 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI114 */
#define SYSTEM_CORE1_KBI1_CKE_KBI114_POS     (14U)                                               /*!< SYSTEM_CORE1_KBI1_CKE_KBI114 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBI115 */
#define SYSTEM_CORE1_KBI1_CKE_KBI115_POS     (15U)                                               /*!< SYSTEM_CORE1_KBI1_CKE_KBI115 bit offset */

/* SYSTEM_CORE1_KBI1_CKE_KBIC1 */
#define SYSTEM_CORE1_KBI1_CKE_KBIC1_POS      (31U)                                               /*!< SYSTEM_CORE1_KBI1_CKE_KBIC1 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_UART0 */
#define SYSTEM_CORE1_MOD0_CKE_UART0_POS      (0U)                                                /*!< SYSTEM_CORE1_MOD0_CKE_UART0 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_UART1 */
#define SYSTEM_CORE1_MOD0_CKE_UART1_POS      (1U)                                                /*!< SYSTEM_CORE1_MOD0_CKE_UART1 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_UART2 */
#define SYSTEM_CORE1_MOD0_CKE_UART2_POS      (2U)                                                /*!< SYSTEM_CORE1_MOD0_CKE_UART2 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_UART3 */
#define SYSTEM_CORE1_MOD0_CKE_UART3_POS      (3U)                                                /*!< SYSTEM_CORE1_MOD0_CKE_UART3 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_UART4 */
#define SYSTEM_CORE1_MOD0_CKE_UART4_POS      (4U)                                                /*!< SYSTEM_CORE1_MOD0_CKE_UART4 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_UART5 */
#define SYSTEM_CORE1_MOD0_CKE_UART5_POS      (5U)                                                /*!< SYSTEM_CORE1_MOD0_CKE_UART5 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_UART6 */
#define SYSTEM_CORE1_MOD0_CKE_UART6_POS      (6U)                                                /*!< SYSTEM_CORE1_MOD0_CKE_UART6 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_UART7 */
#define SYSTEM_CORE1_MOD0_CKE_UART7_POS      (7U)                                                /*!< SYSTEM_CORE1_MOD0_CKE_UART7 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_LPUART */
#define SYSTEM_CORE1_MOD0_CKE_LPUART_POS     (8U)                                                /*!< SYSTEM_CORE1_MOD0_CKE_LPUART bit offset */

/* SYSTEM_CORE1_MOD0_CKE_USART0 */
#define SYSTEM_CORE1_MOD0_CKE_USART0_POS     (9U)                                                /*!< SYSTEM_CORE1_MOD0_CKE_USART0 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_USART1 */
#define SYSTEM_CORE1_MOD0_CKE_USART1_POS     (10U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_USART1 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_ISO78160 */
#define SYSTEM_CORE1_MOD0_CKE_ISO78160_POS   (11U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_ISO78160 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_ISO78161 */
#define SYSTEM_CORE1_MOD0_CKE_ISO78161_POS   (12U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_ISO78161 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_SPI0 */
#define SYSTEM_CORE1_MOD0_CKE_SPI0_POS       (13U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_SPI0 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_SPI1 */
#define SYSTEM_CORE1_MOD0_CKE_SPI1_POS       (14U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_SPI1 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_SPI2 */
#define SYSTEM_CORE1_MOD0_CKE_SPI2_POS       (15U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_SPI2 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_SPI3 */
#define SYSTEM_CORE1_MOD0_CKE_SPI3_POS       (16U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_SPI3 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_I2C0 */
#define SYSTEM_CORE1_MOD0_CKE_I2C0_POS       (17U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_I2C0 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_I2C1 */
#define SYSTEM_CORE1_MOD0_CKE_I2C1_POS       (18U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_I2C1 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_IOCNT */
#define SYSTEM_CORE1_MOD0_CKE_IOCNT_POS      (20U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_IOCNT bit offset */

/* SYSTEM_CORE1_MOD0_CKE_LCD */
#define SYSTEM_CORE1_MOD0_CKE_LCD_POS        (21U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_LCD bit offset */

/* SYSTEM_CORE1_MOD0_CKE_DSP */
#define SYSTEM_CORE1_MOD0_CKE_DSP_POS        (22U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_DSP bit offset */

/* SYSTEM_CORE1_MOD0_CKE_M2M */
#define SYSTEM_CORE1_MOD0_CKE_M2M_POS        (24U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_M2M bit offset */

/* SYSTEM_CORE1_MOD0_CKE_D2F */
#define SYSTEM_CORE1_MOD0_CKE_D2F_POS        (25U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_D2F bit offset */

/* SYSTEM_CORE1_MOD0_CKE_ADC0 */
#define SYSTEM_CORE1_MOD0_CKE_ADC0_POS       (26U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_ADC0 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_DAC */
#define SYSTEM_CORE1_MOD0_CKE_DAC_POS        (27U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_DAC bit offset */

/* SYSTEM_CORE1_MOD0_CKE_TC0 */
#define SYSTEM_CORE1_MOD0_CKE_TC0_POS        (28U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_TC0 bit offset */

/* SYSTEM_CORE1_MOD0_CKE_TC1 */
#define SYSTEM_CORE1_MOD0_CKE_TC1_POS        (29U)                                               /*!< SYSTEM_CORE1_MOD0_CKE_TC1 bit offset */

/* SYSTEM_CORE1_MOD1_CKE_SIMPTC0 */
#define SYSTEM_CORE1_MOD1_CKE_SIMPTC0_POS    (0U)                                                /*!< SYSTEM_CORE1_MOD1_CKE_SIMPTC0 bit offset */

/* SYSTEM_CORE1_MOD1_CKE_SIMPTC1 */
#define SYSTEM_CORE1_MOD1_CKE_SIMPTC1_POS    (1U)                                                /*!< SYSTEM_CORE1_MOD1_CKE_SIMPTC1 bit offset */

/* SYSTEM_CORE1_MOD1_CKE_SIMPTC2 */
#define SYSTEM_CORE1_MOD1_CKE_SIMPTC2_POS    (2U)                                                /*!< SYSTEM_CORE1_MOD1_CKE_SIMPTC2 bit offset */

/* SYSTEM_CORE1_MOD1_CKE_SIMPTC3 */
#define SYSTEM_CORE1_MOD1_CKE_SIMPTC3_POS    (3U)                                                /*!< SYSTEM_CORE1_MOD1_CKE_SIMPTC3 bit offset */

/* SYSTEM_CORE1_MOD1_CKE_RS_VDD */
#define SYSTEM_CORE1_MOD1_CKE_RS_VDD_POS     (4U)                                                /*!< SYSTEM_CORE1_MOD1_CKE_RS_VDD bit offset */

/* SYSTEM_CORE1_MOD1_CKE_LSRX */
#define SYSTEM_CORE1_MOD1_CKE_LSRX_POS       (6U)                                                /*!< SYSTEM_CORE1_MOD1_CKE_LSRX bit offset */

/* SYSTEM_CORE1_MOD1_CKE_LVDCMP */
#define SYSTEM_CORE1_MOD1_CKE_LVDCMP_POS     (9U)                                                /*!< SYSTEM_CORE1_MOD1_CKE_LVDCMP bit offset */

/* SYSTEM_CORE1_MOD1_CKE_CRC0 */
#define SYSTEM_CORE1_MOD1_CKE_CRC0_POS       (10U)                                               /*!< SYSTEM_CORE1_MOD1_CKE_CRC0 bit offset */

/* SYSTEM_CORE1_MOD2_CKE_QSPI0 */
#define SYSTEM_CORE1_MOD2_CKE_QSPI0_POS      (0U)                                                /*!< SYSTEM_CORE1_MOD2_CKE_QSPI0 bit offset */

/* SYSTEM_CORE1_MOD2_CKE_QSPI1 */
#define SYSTEM_CORE1_MOD2_CKE_QSPI1_POS      (1U)                                                /*!< SYSTEM_CORE1_MOD2_CKE_QSPI1 bit offset */

/* SYSTEM_CORE1_MOD2_CKE_SEA_BUS */
#define SYSTEM_CORE1_MOD2_CKE_SEA_BUS_POS    (2U)                                                /*!< SYSTEM_CORE1_MOD2_CKE_SEA_BUS bit offset */

/* SYSTEM_CORE1_MOD2_CKE_SEA_PKE */
#define SYSTEM_CORE1_MOD2_CKE_SEA_PKE_POS    (3U)                                                /*!< SYSTEM_CORE1_MOD2_CKE_SEA_PKE bit offset */

/* SYSTEM_CORE1_MOD2_CKE_SEA_AES */
#define SYSTEM_CORE1_MOD2_CKE_SEA_AES_POS    (4U)                                                /*!< SYSTEM_CORE1_MOD2_CKE_SEA_AES bit offset */

/* SYSTEM_CORE1_MOD2_CKE_SEA_TRNG */
#define SYSTEM_CORE1_MOD2_CKE_SEA_TRNG_POS   (5U)                                                /*!< SYSTEM_CORE1_MOD2_CKE_SEA_TRNG bit offset */

/* SYSTEM_CORE1_MOD2_CKE_SEA_SHA */
#define SYSTEM_CORE1_MOD2_CKE_SEA_SHA_POS    (6U)                                                /*!< SYSTEM_CORE1_MOD2_CKE_SEA_SHA bit offset */

/* SYSTEM_CORE1_MOD2_CKE_KEY_STORE */
#define SYSTEM_CORE1_MOD2_CKE_KEY_STORE_POS  (7U)                                                /*!< SYSTEM_CORE1_MOD2_CKE_KEY_STORE bit offset */

/* SYSTEM_CORE1_MOD2_CKE_AES */
#define SYSTEM_CORE1_MOD2_CKE_AES_POS        (8U)                                                /*!< SYSTEM_CORE1_MOD2_CKE_AES bit offset */

/* SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_POS  (0U)                                                /*!< SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS bit offset */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_CFG(regval)  (SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS & ((uint32_t)(regval) << SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_POS))
#define SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_32K768  (SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_CFG(0x0))       /*!< Current system main clock is 32.768KHz */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_1M8432  (SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_CFG(0x1))       /*!< Current system main clock is 1.8432MHz */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_3M6864  (SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_CFG(0x2))       /*!< Current system main clock is 3.6864MHz */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_7M3728  (SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_CFG(0x3))       /*!< Current system main clock is 7.3728MHz */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_14M7456  (SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_CFG(0x4))       /*!< Current system main clock is 14.7456MHz */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_29M4912  (SYSTEM_CORE1_SYSFREQ_STA_CORE0_SYS_CFG(0x5))       /*!< Current system main clock is 29.4912MHz */

/* SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_POS  (4U)                                                /*!< SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS bit offset */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_CFG(regval)  (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS & ((uint32_t)(regval) << SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_POS))
#define SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_32K768  (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_CFG(0x0))       /*!< Current system main clock is 32.768KHz */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_1M8432  (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_CFG(0x1))       /*!< Current system main clock is 1.8432MHz */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_3M6864  (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_CFG(0x2))       /*!< Current system main clock is 3.6864MHz */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_7M3728  (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_CFG(0x3))       /*!< Current system main clock is 7.3728MHz */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_14M7456  (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_CFG(0x4))       /*!< Current system main clock is 14.7456MHz */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_29M4912  (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_CFG(0x5))       /*!< Current system main clock is 29.4912MHz */
#define SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_58M9824  (SYSTEM_CORE1_SYSFREQ_STA_CORE1_SYS_CFG(0x6))       /*!< Current system main clock is 58.9824MHz */

/* SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK */
#define SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_POS  (8U)                                                /*!< SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK bit offset */
#define SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_CFG(regval)  (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK & ((uint32_t)(regval) << SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_POS))
#define SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_32K768  (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_CFG(0x0))       /*!< Current system main clock is 32.768KHz */
#define SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_1M8432  (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_CFG(0x1))       /*!< Current system main clock is 1.8432MHz */
#define SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_3M6864  (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_CFG(0x2))       /*!< Current system main clock is 3.6864MHz */
#define SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_7M3728  (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_CFG(0x3))       /*!< Current system main clock is 7.3728MHz */
#define SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_14M7456  (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_CFG(0x4))       /*!< Current system main clock is 14.7456MHz */
#define SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_29M4912  (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_CFG(0x5))       /*!< Current system main clock is 29.4912MHz */
#define SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_58M9824  (SYSTEM_CORE1_SYSFREQ_STA_FLASH_CLK_CFG(0x6))       /*!< Current system main clock is 58.9824MHz */

/* SYSTEM_CORE1_SYS_RST_FLAG_POWEROK */
#define SYSTEM_CORE1_SYS_RST_FLAG_POWEROK_POS  (0U)                                                /*!< SYSTEM_CORE1_SYS_RST_FLAG_POWEROK bit offset */

/* SYSTEM_CORE1_SYS_RST_FLAG_PIN */
#define SYSTEM_CORE1_SYS_RST_FLAG_PIN_POS    (1U)                                                /*!< SYSTEM_CORE1_SYS_RST_FLAG_PIN bit offset */

/* SYSTEM_CORE1_SYS_RST_FLAG_CORE0_WDT */
#define SYSTEM_CORE1_SYS_RST_FLAG_CORE0_WDT_POS  (2U)                                                /*!< SYSTEM_CORE1_SYS_RST_FLAG_CORE0_WDT bit offset */

/* SYSTEM_CORE1_SYS_RST_FLAG_CORE1T0_WDT */
#define SYSTEM_CORE1_SYS_RST_FLAG_CORE1T0_WDT_POS  (3U)                                                /*!< SYSTEM_CORE1_SYS_RST_FLAG_CORE1T0_WDT bit offset */

/* SYSTEM_CORE1_SYS_RST_FLAG_CORE0_SOFT */
#define SYSTEM_CORE1_SYS_RST_FLAG_CORE0_SOFT_POS  (4U)                                                /*!< SYSTEM_CORE1_SYS_RST_FLAG_CORE0_SOFT bit offset */

/* SYSTEM_CORE1_SYS_RST_FLAG_CORE1T0_SOFT */
#define SYSTEM_CORE1_SYS_RST_FLAG_CORE1T0_SOFT_POS  (5U)                                                /*!< SYSTEM_CORE1_SYS_RST_FLAG_CORE1T0_SOFT bit offset */

/* SYSTEM_CORE1_SYS_RST_FLAG_BOI_VBAT */
#define SYSTEM_CORE1_SYS_RST_FLAG_BOI_VBAT_POS  (6U)                                                /*!< SYSTEM_CORE1_SYS_RST_FLAG_BOI_VBAT bit offset */

/* SYSTEM_CORE1_SYS_RST_FLAG_CORE1_WDT */
#define SYSTEM_CORE1_SYS_RST_FLAG_CORE1_WDT_POS  (7U)                                                /*!< SYSTEM_CORE1_SYS_RST_FLAG_CORE1_WDT bit offset */

/* SYSTEM_CORE1_SYS_RST_FLAG_CORE1_SOFT */
#define SYSTEM_CORE1_SYS_RST_FLAG_CORE1_SOFT_POS  (8U)                                                /*!< SYSTEM_CORE1_SYS_RST_FLAG_CORE1_SOFT bit offset */

/* SYSTEM_CORE1_SYS_RST_FLAG_CORE0T1_SOFT */
#define SYSTEM_CORE1_SYS_RST_FLAG_CORE0T1_SOFT_POS  (9U)                                                /*!< SYSTEM_CORE1_SYS_RST_FLAG_CORE0T1_SOFT bit offset */

/* SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_PENDING */
#define SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_PENDING_POS  (0U)                                                /*!< SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_PENDING bit offset */

/* SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_IE */
#define SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_IE_POS  (1U)                                                /*!< SYSTEM_CORE1_BOI_RST_CTRL_CORE1_RST_IE bit offset */

/* SYSTEM_CORE1_BOI_RST_CTRL_BOI_VBAT_RST_IE */
#define SYSTEM_CORE1_BOI_RST_CTRL_BOI_VBAT_RST_IE_POS  (4U)                                                /*!< SYSTEM_CORE1_BOI_RST_CTRL_BOI_VBAT_RST_IE bit offset */

/* SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_EN */
#define SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_EN_POS  (0U)                                                /*!< SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_EN bit offset */

/* SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_FLAG */
#define SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_FLAG_POS  (16U)                                               /*!< SYSTEM_CORE1_UPDATE_RST_CTRL_CORE1_RST_FLAG bit offset */

/* SYSTEM_CORE1_PMOD0_RST_MAILBOX */
#define SYSTEM_CORE1_PMOD0_RST_MAILBOX_POS   (0U)                                                /*!< SYSTEM_CORE1_PMOD0_RST_MAILBOX bit offset */

/* SYSTEM_CORE1_PMOD0_RST_AOS */
#define SYSTEM_CORE1_PMOD0_RST_AOS_POS       (1U)                                                /*!< SYSTEM_CORE1_PMOD0_RST_AOS bit offset */

/* SYSTEM_CORE1_PMOD0_RST_GDMA10 */
#define SYSTEM_CORE1_PMOD0_RST_GDMA10_POS    (2U)                                                /*!< SYSTEM_CORE1_PMOD0_RST_GDMA10 bit offset */

/* SYSTEM_CORE1_PMOD0_RST_GDMA11 */
#define SYSTEM_CORE1_PMOD0_RST_GDMA11_POS    (3U)                                                /*!< SYSTEM_CORE1_PMOD0_RST_GDMA11 bit offset */

/* SYSTEM_CORE1_PMOD0_RST_HSEM */
#define SYSTEM_CORE1_PMOD0_RST_HSEM_POS      (4U)                                                /*!< SYSTEM_CORE1_PMOD0_RST_HSEM bit offset */

/* SYSTEM_CORE1_MOD0_RST_UART0 */
#define SYSTEM_CORE1_MOD0_RST_UART0_POS      (0U)                                                /*!< SYSTEM_CORE1_MOD0_RST_UART0 bit offset */

/* SYSTEM_CORE1_MOD0_RST_UART1 */
#define SYSTEM_CORE1_MOD0_RST_UART1_POS      (1U)                                                /*!< SYSTEM_CORE1_MOD0_RST_UART1 bit offset */

/* SYSTEM_CORE1_MOD0_RST_UART2 */
#define SYSTEM_CORE1_MOD0_RST_UART2_POS      (2U)                                                /*!< SYSTEM_CORE1_MOD0_RST_UART2 bit offset */

/* SYSTEM_CORE1_MOD0_RST_UART3 */
#define SYSTEM_CORE1_MOD0_RST_UART3_POS      (3U)                                                /*!< SYSTEM_CORE1_MOD0_RST_UART3 bit offset */

/* SYSTEM_CORE1_MOD0_RST_UART4 */
#define SYSTEM_CORE1_MOD0_RST_UART4_POS      (4U)                                                /*!< SYSTEM_CORE1_MOD0_RST_UART4 bit offset */

/* SYSTEM_CORE1_MOD0_RST_UART5 */
#define SYSTEM_CORE1_MOD0_RST_UART5_POS      (5U)                                                /*!< SYSTEM_CORE1_MOD0_RST_UART5 bit offset */

/* SYSTEM_CORE1_MOD0_RST_UART6 */
#define SYSTEM_CORE1_MOD0_RST_UART6_POS      (6U)                                                /*!< SYSTEM_CORE1_MOD0_RST_UART6 bit offset */

/* SYSTEM_CORE1_MOD0_RST_UART7 */
#define SYSTEM_CORE1_MOD0_RST_UART7_POS      (7U)                                                /*!< SYSTEM_CORE1_MOD0_RST_UART7 bit offset */

/* SYSTEM_CORE1_MOD0_RST_LPUART */
#define SYSTEM_CORE1_MOD0_RST_LPUART_POS     (8U)                                                /*!< SYSTEM_CORE1_MOD0_RST_LPUART bit offset */

/* SYSTEM_CORE1_MOD0_RST_USART0 */
#define SYSTEM_CORE1_MOD0_RST_USART0_POS     (9U)                                                /*!< SYSTEM_CORE1_MOD0_RST_USART0 bit offset */

/* SYSTEM_CORE1_MOD0_RST_USART1 */
#define SYSTEM_CORE1_MOD0_RST_USART1_POS     (10U)                                               /*!< SYSTEM_CORE1_MOD0_RST_USART1 bit offset */

/* SYSTEM_CORE1_MOD0_RST_ISO78160 */
#define SYSTEM_CORE1_MOD0_RST_ISO78160_POS   (11U)                                               /*!< SYSTEM_CORE1_MOD0_RST_ISO78160 bit offset */

/* SYSTEM_CORE1_MOD0_RST_ISO78161 */
#define SYSTEM_CORE1_MOD0_RST_ISO78161_POS   (12U)                                               /*!< SYSTEM_CORE1_MOD0_RST_ISO78161 bit offset */

/* SYSTEM_CORE1_MOD0_RST_SPI0 */
#define SYSTEM_CORE1_MOD0_RST_SPI0_POS       (13U)                                               /*!< SYSTEM_CORE1_MOD0_RST_SPI0 bit offset */

/* SYSTEM_CORE1_MOD0_RST_SPI1 */
#define SYSTEM_CORE1_MOD0_RST_SPI1_POS       (14U)                                               /*!< SYSTEM_CORE1_MOD0_RST_SPI1 bit offset */

/* SYSTEM_CORE1_MOD0_RST_SPI2 */
#define SYSTEM_CORE1_MOD0_RST_SPI2_POS       (15U)                                               /*!< SYSTEM_CORE1_MOD0_RST_SPI2 bit offset */

/* SYSTEM_CORE1_MOD0_RST_SPI3 */
#define SYSTEM_CORE1_MOD0_RST_SPI3_POS       (16U)                                               /*!< SYSTEM_CORE1_MOD0_RST_SPI3 bit offset */

/* SYSTEM_CORE1_MOD0_RST_I2C0 */
#define SYSTEM_CORE1_MOD0_RST_I2C0_POS       (17U)                                               /*!< SYSTEM_CORE1_MOD0_RST_I2C0 bit offset */

/* SYSTEM_CORE1_MOD0_RST_I2C1 */
#define SYSTEM_CORE1_MOD0_RST_I2C1_POS       (18U)                                               /*!< SYSTEM_CORE1_MOD0_RST_I2C1 bit offset */

/* SYSTEM_CORE1_MOD0_RST_CAN */
#define SYSTEM_CORE1_MOD0_RST_CAN_POS        (19U)                                               /*!< SYSTEM_CORE1_MOD0_RST_CAN bit offset */

/* SYSTEM_CORE1_MOD0_RST_IOCNT */
#define SYSTEM_CORE1_MOD0_RST_IOCNT_POS      (20U)                                               /*!< SYSTEM_CORE1_MOD0_RST_IOCNT bit offset */

/* SYSTEM_CORE1_MOD0_RST_LCD */
#define SYSTEM_CORE1_MOD0_RST_LCD_POS        (21U)                                               /*!< SYSTEM_CORE1_MOD0_RST_LCD bit offset */

/* SYSTEM_CORE1_MOD0_RST_DSP */
#define SYSTEM_CORE1_MOD0_RST_DSP_POS        (22U)                                               /*!< SYSTEM_CORE1_MOD0_RST_DSP bit offset */

/* SYSTEM_CORE1_MOD0_RST_M2M */
#define SYSTEM_CORE1_MOD0_RST_M2M_POS        (24U)                                               /*!< SYSTEM_CORE1_MOD0_RST_M2M bit offset */

/* SYSTEM_CORE1_MOD0_RST_D2F */
#define SYSTEM_CORE1_MOD0_RST_D2F_POS        (25U)                                               /*!< SYSTEM_CORE1_MOD0_RST_D2F bit offset */

/* SYSTEM_CORE1_MOD0_RST_ADC0 */
#define SYSTEM_CORE1_MOD0_RST_ADC0_POS       (26U)                                               /*!< SYSTEM_CORE1_MOD0_RST_ADC0 bit offset */

/* SYSTEM_CORE1_MOD0_RST_DAC */
#define SYSTEM_CORE1_MOD0_RST_DAC_POS        (27U)                                               /*!< SYSTEM_CORE1_MOD0_RST_DAC bit offset */

/* SYSTEM_CORE1_MOD0_RST_TC0 */
#define SYSTEM_CORE1_MOD0_RST_TC0_POS        (28U)                                               /*!< SYSTEM_CORE1_MOD0_RST_TC0 bit offset */

/* SYSTEM_CORE1_MOD0_RST_TC1 */
#define SYSTEM_CORE1_MOD0_RST_TC1_POS        (29U)                                               /*!< SYSTEM_CORE1_MOD0_RST_TC1 bit offset */

/* SYSTEM_CORE1_MOD1_RST_SIMPTC0 */
#define SYSTEM_CORE1_MOD1_RST_SIMPTC0_POS    (0U)                                                /*!< SYSTEM_CORE1_MOD1_RST_SIMPTC0 bit offset */

/* SYSTEM_CORE1_MOD1_RST_SIMPTC1 */
#define SYSTEM_CORE1_MOD1_RST_SIMPTC1_POS    (1U)                                                /*!< SYSTEM_CORE1_MOD1_RST_SIMPTC1 bit offset */

/* SYSTEM_CORE1_MOD1_RST_SIMPTC2 */
#define SYSTEM_CORE1_MOD1_RST_SIMPTC2_POS    (2U)                                                /*!< SYSTEM_CORE1_MOD1_RST_SIMPTC2 bit offset */

/* SYSTEM_CORE1_MOD1_RST_SIMPTC3 */
#define SYSTEM_CORE1_MOD1_RST_SIMPTC3_POS    (3U)                                                /*!< SYSTEM_CORE1_MOD1_RST_SIMPTC3 bit offset */

/* SYSTEM_CORE1_MOD1_RST_RS_VDD */
#define SYSTEM_CORE1_MOD1_RST_RS_VDD_POS     (4U)                                                /*!< SYSTEM_CORE1_MOD1_RST_RS_VDD bit offset */

/* SYSTEM_CORE1_MOD1_RST_LSRX */
#define SYSTEM_CORE1_MOD1_RST_LSRX_POS       (6U)                                                /*!< SYSTEM_CORE1_MOD1_RST_LSRX bit offset */

/* SYSTEM_CORE1_MOD1_RST_KBI0 */
#define SYSTEM_CORE1_MOD1_RST_KBI0_POS       (7U)                                                /*!< SYSTEM_CORE1_MOD1_RST_KBI0 bit offset */

/* SYSTEM_CORE1_MOD1_RST_KBI1 */
#define SYSTEM_CORE1_MOD1_RST_KBI1_POS       (8U)                                                /*!< SYSTEM_CORE1_MOD1_RST_KBI1 bit offset */

/* SYSTEM_CORE1_MOD1_RST_LVDCMP */
#define SYSTEM_CORE1_MOD1_RST_LVDCMP_POS     (9U)                                                /*!< SYSTEM_CORE1_MOD1_RST_LVDCMP bit offset */

/* SYSTEM_CORE1_MOD1_RST_CRC0 */
#define SYSTEM_CORE1_MOD1_RST_CRC0_POS       (10U)                                               /*!< SYSTEM_CORE1_MOD1_RST_CRC0 bit offset */

/* SYSTEM_CORE1_MOD2_RST_QSPI0 */
#define SYSTEM_CORE1_MOD2_RST_QSPI0_POS      (0U)                                                /*!< SYSTEM_CORE1_MOD2_RST_QSPI0 bit offset */

/* SYSTEM_CORE1_MOD2_RST_QSPI1 */
#define SYSTEM_CORE1_MOD2_RST_QSPI1_POS      (1U)                                                /*!< SYSTEM_CORE1_MOD2_RST_QSPI1 bit offset */

/* SYSTEM_CORE1_MOD2_RST_SEA */
#define SYSTEM_CORE1_MOD2_RST_SEA_POS        (2U)                                                /*!< SYSTEM_CORE1_MOD2_RST_SEA bit offset */

/* SYSTEM_CORE1_MOD2_RST_AES */
#define SYSTEM_CORE1_MOD2_RST_AES_POS        (8U)                                                /*!< SYSTEM_CORE1_MOD2_RST_AES bit offset */

/* SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0 */
#define SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0_POS  (0U)                                                /*!< SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0 bit offset */
#define SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0_CFG(regval)  (SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0 & ((uint32_t)(regval) << SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0_POS))
#define SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0_0   (SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0_CFG(0x0))         /*!< CORE0 FLASH mapped to address 0x0020_0000 */
#define SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0_1   (SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0_CFG(0x1))         /*!< CORE0 FLASH mapped to address 0x0000_0000 */
#define SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0_2   (SYSTEM_CORE1_ADDR_MAP_CTRL_CORE0_CFG(0x2))         /*!< CORE0 FLASH mapped to address 0x1020_0000 */

/* SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1 */
#define SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1_POS  (4U)                                                /*!< SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1 bit offset */
#define SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1_CFG(regval)  (SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1 & ((uint32_t)(regval) << SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1_POS))
#define SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1_0   (SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1_CFG(0x0))         /*!< CORE1 FLASH mapped to address 0x8000_0000 */
#define SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1_1   (SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1_CFG(0x1))         /*!< CORE1 FLASH mapped to address 0x9000_0000 */
#define SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1_2   (SYSTEM_CORE1_ADDR_MAP_CTRL_CORE1_CFG(0x2))         /*!< CORE1 FLASH mapped to address 0x8100_0000 */

/* SYSTEM_CORE1_FLASH_MAP_CTRL_CORE0_FLASH */
#define SYSTEM_CORE1_FLASH_MAP_CTRL_CORE0_FLASH_POS  (0U)                                                /*!< SYSTEM_CORE1_FLASH_MAP_CTRL_CORE0_FLASH bit offset */

/* SYSTEM_CORE1_FLASH_MAP_CTRL_CORE1_FLASH */
#define SYSTEM_CORE1_FLASH_MAP_CTRL_CORE1_FLASH_POS  (1U)                                                /*!< SYSTEM_CORE1_FLASH_MAP_CTRL_CORE1_FLASH bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH0 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH0_POS  (0U)                                                /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH0 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH1 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH1_POS  (2U)                                                /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH1 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH2 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH2_POS  (4U)                                                /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH2 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH3 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH3_POS  (6U)                                                /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH3 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH4 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH4_POS  (8U)                                                /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH4 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH5 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH5_POS  (10U)                                               /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH5 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH6 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH6_POS  (12U)                                               /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH6 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH7 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH7_POS  (14U)                                               /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH7 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH8 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH8_POS  (16U)                                               /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH8 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH9 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH9_POS  (18U)                                               /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH9 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH10 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH10_POS  (20U)                                               /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH10 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH11 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH11_POS  (22U)                                               /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH11 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH12 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH12_POS  (24U)                                               /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH12 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH13 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH13_POS  (26U)                                               /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH13 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH14 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH14_POS  (28U)                                               /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH14 bit offset */

/* SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH15 */
#define SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH15_POS  (30U)                                               /*!< SYSTEM_CORE1_DMA_PRI0_CTRL_DMA_CH15 bit offset */

/* SYSTEM_CORE1_DMA_PRI1_CTRL_DMA_CH16 */
#define SYSTEM_CORE1_DMA_PRI1_CTRL_DMA_CH16_POS  (0U)                                                /*!< SYSTEM_CORE1_DMA_PRI1_CTRL_DMA_CH16 bit offset */

/* SYSTEM_CORE1_DMA_PRI1_CTRL_DMA_CH17 */
#define SYSTEM_CORE1_DMA_PRI1_CTRL_DMA_CH17_POS  (2U)                                                /*!< SYSTEM_CORE1_DMA_PRI1_CTRL_DMA_CH17 bit offset */

/* SYSTEM_CORE1_DMA_PRI1_CTRL_DMA_CH18 */
#define SYSTEM_CORE1_DMA_PRI1_CTRL_DMA_CH18_POS  (4U)                                                /*!< SYSTEM_CORE1_DMA_PRI1_CTRL_DMA_CH18 bit offset */

/* SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH0 */
#define SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH0_POS  (0U)                                                /*!< SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH0 bit offset */

/* SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH1 */
#define SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH1_POS  (2U)                                                /*!< SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH1 bit offset */

/* SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH2 */
#define SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH2_POS  (4U)                                                /*!< SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH2 bit offset */

/* SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH3 */
#define SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH3_POS  (6U)                                                /*!< SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH3 bit offset */

/* SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH4 */
#define SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH4_POS  (8U)                                                /*!< SYSTEM_CORE1_DMA_PRI2_CTRL_DMA_CH4 bit offset */

/* SYSTEM_CORE1_CORE1_STA_CORE1_STOP_FLAG */
#define SYSTEM_CORE1_CORE1_STA_CORE1_STOP_FLAG_POS  (0U)                                                /*!< SYSTEM_CORE1_CORE1_STA_CORE1_STOP_FLAG bit offset */

/* SYSTEM_CORE1_CHIP_ID_CHIP_ID */
#define SYSTEM_CORE1_CHIP_ID_CHIP_ID_POS     (0U)                                                /*!< SYSTEM_CORE1_CHIP_ID_CHIP_ID bit offset */

/* SYSTEM_CORE1_DIE_VER_VERSION */
#define SYSTEM_CORE1_DIE_VER_VERSION_POS     (0U)                                                /*!< SYSTEM_CORE1_DIE_VER_VERSION bit offset */

/* SYSTEM_CORE1_DIE_VER_CHIP_MODEL */
#define SYSTEM_CORE1_DIE_VER_CHIP_MODEL_POS  (4U)                                                /*!< SYSTEM_CORE1_DIE_VER_CHIP_MODEL bit offset */

/* SYSTEM_CORE1_FAB_UID0_LOT_NUM */
#define SYSTEM_CORE1_FAB_UID0_LOT_NUM_POS    (0U)                                                /*!< SYSTEM_CORE1_FAB_UID0_LOT_NUM bit offset */

/* SYSTEM_CORE1_FAB_UID1_WAFER_ID */
#define SYSTEM_CORE1_FAB_UID1_WAFER_ID_POS   (0U)                                                /*!< SYSTEM_CORE1_FAB_UID1_WAFER_ID bit offset */

/* SYSTEM_CORE1_FAB_UID1_X_LABEL */
#define SYSTEM_CORE1_FAB_UID1_X_LABEL_POS    (8U)                                                /*!< SYSTEM_CORE1_FAB_UID1_X_LABEL bit offset */

/* SYSTEM_CORE1_FAB_UID1_Y_LABEL */
#define SYSTEM_CORE1_FAB_UID1_Y_LABEL_POS    (20U)                                               /*!< SYSTEM_CORE1_FAB_UID1_Y_LABEL bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_SYSTEM_CORE1_H */
