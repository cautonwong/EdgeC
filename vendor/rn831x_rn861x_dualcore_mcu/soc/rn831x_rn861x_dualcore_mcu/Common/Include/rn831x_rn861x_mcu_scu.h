/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_scu.h
 * @author  Renergy Software Team
 * @brief   SCU module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_SCU_H
#define _RN831x_RN861x_MCU_SCU_H

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

/* SCU */
typedef struct
{
    __IO uint32_t WKEY;                      /*!< offset: 0x000 info: Write Password Protection Register */
    __IO uint32_t LCK;                       /*!< offset: 0x004 info: LCK Control Register */
    __IO uint32_t PERIPH_SEC_CFG0;           /*!< offset: 0x008 info: Peripheral Security Configuration Register 0 */
    __IO uint32_t PERIPH_SEC_CFG1;           /*!< offset: 0x00C info: Peripheral Security Configuration Register 1 */
    __IO uint32_t PERIPH_SEC_CFG2;           /*!< offset: 0x010 info: Peripheral Security Configuration Register 2 */
    __IO uint32_t RESERVED0[2];              /*!< offset: 0x014 ~ 018 */
    __IO uint32_t GPIOA_SEC_CFG;             /*!< offset: 0x01C info: GPIOA Security Configuration Register */
    __IO uint32_t GPIOB_SEC_CFG;             /*!< offset: 0x020 info: GPIOB Security Configuration Register */
    __IO uint32_t GPIOC_SEC_CFG;             /*!< offset: 0x024 info: GPIOC Security Configuration Register */
    __IO uint32_t GPIOD_SEC_CFG;             /*!< offset: 0x028 info: GPIOD Security Configuration Register */
    __IO uint32_t GPIOE_SEC_CFG;             /*!< offset: 0x02C info: GPIOE Security Configuration Register */
    __IO uint32_t GPIOF_SEC_CFG;             /*!< offset: 0x030 info: GPIOF Security Configuration Register */
    __IO uint32_t GPIOG_SEC_CFG;             /*!< offset: 0x034 info: GPIOG Security Configuration Register */
    __IO uint32_t GPIOH_SEC_CFG;             /*!< offset: 0x038 info: GPIOH Security Configuration Register */
    __IO uint32_t RESERVED1[8];              /*!< offset: 0x03C ~ 058 */
    __IO uint32_t SRAM_SEC_WM_CFG;           /*!< offset: 0x05C info: SRAM Security Watermark Configuration Register */
    __IO uint32_t PERIPH_SEC_IE0;            /*!< offset: 0x060 info: Peripheral Illegal Access Interrupt Enable Register 0 */
    __IO uint32_t PERIPH_SEC_IE1;            /*!< offset: 0x064 info: Peripheral Illegal Access Interrupt Enable Register 1 */
    __IO uint32_t PERIPH_SEC_IE2;            /*!< offset: 0x068 info: Peripheral Illegal Access Interrupt Enable Register 2 */
    __IO uint32_t PERIPH_SEC_IF0;            /*!< offset: 0x06C info: Peripheral Illegal Access Interrupt Flag Register 0 */
    __IO uint32_t PERIPH_SEC_IF1;            /*!< offset: 0x070 info: Peripheral Illegal Access Interrupt Flag Register 1 */
    __IO uint32_t PERIPH_SEC_IF2;            /*!< offset: 0x074 info: Peripheral Illegal Access Interrupt Flag Register 2 */
} SCU_TypeDef;

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
#define SCU_CORE0_BASE  (0x30018000U)
#define SCU_CORE1_BASE  (0xB0018000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define SCU_CORE0  ((SCU_TypeDef *)      SCU_CORE0_BASE)
#define SCU_CORE1  ((SCU_TypeDef *)      SCU_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define SCU  SCU_CORE0
#endif
#if defined(CORE1)
#define SCU  SCU_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** SCU Instances *******************************/
#define IS_SCU_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == SCU_CORE0) || \
                                          ((INSTANCE) == SCU_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* SCU_WKEY */
#define SCU_WKEY_WKEY                        (BITS(0,31))                                        /*!< Password protection bits. */

/* SCU_LCK */
#define SCU_LCK_LCK                          (BITS(0,31))                                        /*!< SCU security attribute (08H~5CH) configuration lock register. */

/* SCU_PERIPH_SEC_CFG0 */
#define SCU_PERIPH_SEC_CFG0_UART0SEC         (BIT(0))                                            /*!< UART0 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_UART1SEC         (BIT(1))                                            /*!< UART1 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_UART2SEC         (BIT(2))                                            /*!< UART2 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_UART3SEC         (BIT(3))                                            /*!< UART3 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_UART4SEC         (BIT(4))                                            /*!< UART4 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_UART5SEC         (BIT(5))                                            /*!< UART5 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_UART6SEC         (BIT(6))                                            /*!< UART6 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_UART7SEC         (BIT(7))                                            /*!< UART7 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_LPUARTSEC        (BIT(8))                                            /*!< LPUART security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_USART0SEC        (BIT(9))                                            /*!< USART0 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_USART1SEC        (BIT(10))                                           /*!< USART1 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_ISO78160SEC      (BIT(11))                                           /*!< ISO7816-0 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_ISO78161SEC      (BIT(12))                                           /*!< ISO7816-1 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_SPI0SEC          (BIT(13))                                           /*!< SPI0 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_SPI1SEC          (BIT(14))                                           /*!< SPI1 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_SPI2SEC          (BIT(15))                                           /*!< SPI2 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_SPI3SEC          (BIT(16))                                           /*!< SPI3 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_I2C0SEC          (BIT(17))                                           /*!< I2C0 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_I2C1SEC          (BIT(18))                                           /*!< I2C1 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_CANSEC           (BIT(19))                                           /*!< CAN security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_IOCNTSEC         (BIT(20))                                           /*!< IOCNT security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_LCDSEC           (BIT(21))                                           /*!< LCD security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_DSPSEC           (BIT(22))                                           /*!< DSP security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_M2MSEC           (BIT(24))                                           /*!< M2M security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_D2FSEC           (BIT(25))                                           /*!< D2F security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_ADC0SEC          (BIT(26))                                           /*!< ADC0 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_DACSEC           (BIT(27))                                           /*!< DAC security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_TC0SEC           (BIT(28))                                           /*!< TC0 security attribute configuration */
#define SCU_PERIPH_SEC_CFG0_TC1SEC           (BIT(29))                                           /*!< TC1 security attribute configuration */

/* SCU_PERIPH_SEC_CFG1 */
#define SCU_PERIPH_SEC_CFG1_SIMPTC0SEC       (BIT(0))                                            /*!< SIMP_TC0 security attribute configuration */
#define SCU_PERIPH_SEC_CFG1_SIMPTC1SEC       (BIT(1))                                            /*!< SIMP_TC1 security attribute configuration */
#define SCU_PERIPH_SEC_CFG1_SIMPTC2SEC       (BIT(2))                                            /*!< SIMP_TC2 security attribute configuration */
#define SCU_PERIPH_SEC_CFG1_SIMPTC3SEC       (BIT(3))                                            /*!< SIMP_TC3 security attribute configuration */
#define SCU_PERIPH_SEC_CFG1_RS_VDDSEC        (BIT(4))                                            /*!< RS_VDD security attribute configuration */
#define SCU_PERIPH_SEC_CFG1_LSRXSEC          (BIT(6))                                            /*!< LSRX security attribute configuration */
#define SCU_PERIPH_SEC_CFG1_KBI0SEC          (BIT(7))                                            /*!< KBI0 security attribute configuration */
#define SCU_PERIPH_SEC_CFG1_KBI1SEC          (BIT(8))                                            /*!< KBI1 security attribute configuration */
#define SCU_PERIPH_SEC_CFG1_LVD_CMPSEC       (BIT(9))                                            /*!< LVD_CMP security attribute configuration */
#define SCU_PERIPH_SEC_CFG1_CRC0SEC          (BIT(10))                                           /*!< CRC0 security attribute configuration */

/* SCU_PERIPH_SEC_CFG2 */
#define SCU_PERIPH_SEC_CFG2_QSPI0SEC         (BIT(0))                                            /*!< QSPI0 security attribute configuration */
#define SCU_PERIPH_SEC_CFG2_QSPI1SEC         (BIT(1))                                            /*!< QSPI1 security attribute configuration */
#define SCU_PERIPH_SEC_CFG2_SEASEC           (BIT(2))                                            /*!< SEA security attribute configuration */
#define SCU_PERIPH_SEC_CFG2_AESSEC           (BIT(8))                                            /*!< AES security attribute configuration */

/* SCU_GPIOA_SEC_CFG */
#define SCU_GPIOA_SEC_CFG_PA_SEC             (BITS(0,15))                                        /*!< Security attribute configuration for PA00~PA15 */

/* SCU_GPIOB_SEC_CFG */
#define SCU_GPIOB_SEC_CFG_PB_SEC             (BITS(0,15))                                        /*!< Security attribute configuration for PB00~PB15 */

/* SCU_GPIOC_SEC_CFG */
#define SCU_GPIOC_SEC_CFG_PC_SEC             (BITS(0,15))                                        /*!< Security attribute configuration for PC00~PC15 */

/* SCU_GPIOD_SEC_CFG */
#define SCU_GPIOD_SEC_CFG_PD_SEC             (BITS(0,15))                                        /*!< Security attribute configuration for PD00~PD15 */

/* SCU_GPIOE_SEC_CFG */
#define SCU_GPIOE_SEC_CFG_PE_SEC             (BITS(0,15))                                        /*!< Security attribute configuration for PE00~PE15 */

/* SCU_GPIOF_SEC_CFG */
#define SCU_GPIOF_SEC_CFG_PF_SEC             (BITS(0,15))                                        /*!< Security attribute configuration for PF00~PF15 */

/* SCU_GPIOG_SEC_CFG */
#define SCU_GPIOG_SEC_CFG_PG_SEC             (BITS(0,15))                                        /*!< Security attribute configuration for PG00~PG15 */

/* SCU_GPIOH_SEC_CFG */
#define SCU_GPIOH_SEC_CFG_PH_SEC             (BITS(0,1))                                         /*!< Security attribute configuration for PH00~PH01 */

/* SCU_SRAM_SEC_WM_CFG */
#define SCU_SRAM_SEC_WM_CFG_SRAM_SEC_LEN     (BITS(0,3))                                         /*!< Secure SRAM region length configuration (1-4'ha) */

/* SCU_PERIPH_SEC_IE0 */
#define SCU_PERIPH_SEC_IE0_UART0IE           (BIT(0))                                            /*!< UART0 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_UART1IE           (BIT(1))                                            /*!< UART1 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_UART2IE           (BIT(2))                                            /*!< UART2 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_UART3IE           (BIT(3))                                            /*!< UART3 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_UART4IE           (BIT(4))                                            /*!< UART4 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_UART5IE           (BIT(5))                                            /*!< UART5 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_UART6IE           (BIT(6))                                            /*!< UART6 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_UART7IE           (BIT(7))                                            /*!< UART7 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_LPUARTIE          (BIT(8))                                            /*!< LPUART illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_USART0IE          (BIT(9))                                            /*!< USART0 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_USART1IE          (BIT(10))                                           /*!< USART1 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_ISO78160IE        (BIT(11))                                           /*!< ISO7816-0 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_ISO78161IE        (BIT(12))                                           /*!< ISO7816-1 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_SPI0IE            (BIT(13))                                           /*!< SPI0 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_SPI1IE            (BIT(14))                                           /*!< SPI1 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_SPI2IE            (BIT(15))                                           /*!< SPI2 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_SPI3IE            (BIT(16))                                           /*!< SPI3 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_I2C0IE            (BIT(17))                                           /*!< I2C0 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_I2C1IE            (BIT(18))                                           /*!< I2C1 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_CANIE             (BIT(19))                                           /*!< CAN illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_IOCNTIE           (BIT(20))                                           /*!< IOCNT illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_LCDIE             (BIT(21))                                           /*!< LCD illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_DSPIE             (BIT(22))                                           /*!< DSP illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_M2MIE             (BIT(24))                                           /*!< M2M illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_D2FIE             (BIT(25))                                           /*!< D2F illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_ADC0IE            (BIT(26))                                           /*!< ADC0 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_DACIE             (BIT(27))                                           /*!< DAC illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_TC0IE             (BIT(28))                                           /*!< TC0 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE0_TC1IE             (BIT(29))                                           /*!< TC1 illegal access interrupt enable configuration */

/* SCU_PERIPH_SEC_IE1 */
#define SCU_PERIPH_SEC_IE1_SIMPTC0IE         (BIT(0))                                            /*!< SIMPTC0 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE1_SIMPTC1IE         (BIT(1))                                            /*!< SIMPTC1 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE1_SIMPTC2IE         (BIT(2))                                            /*!< SIMPTC2 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE1_SIMPTC3IE         (BIT(3))                                            /*!< SIMPTC3 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE1_RS_VDDIE          (BIT(4))                                            /*!< RS_VDD illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE1_LSRXIE            (BIT(6))                                            /*!< LSRX illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE1_KBI0IE            (BIT(7))                                            /*!< KBI0 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE1_KBI1IE            (BIT(8))                                            /*!< KBI1 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE1_LVD_CMPIE         (BIT(9))                                            /*!< LVD_CMP illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE1_CRC0IE            (BIT(10))                                           /*!< CRC0 illegal access interrupt enable configuration */

/* SCU_PERIPH_SEC_IE2 */
#define SCU_PERIPH_SEC_IE2_QSPI0IE           (BIT(0))                                            /*!< QSPI0 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE2_QSPI1IE           (BIT(1))                                            /*!< QSPI1 illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE2_SEAIE             (BIT(2))                                            /*!< SEA illegal access interrupt enable configuration */
#define SCU_PERIPH_SEC_IE2_AESIE             (BIT(8))                                            /*!< AES illegal access interrupt enable configuration */

/* SCU_PERIPH_SEC_IF0 */
#define SCU_PERIPH_SEC_IF0_UART0IF           (BIT(0))                                            /*!< UART0 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_UART1IF           (BIT(1))                                            /*!< UART1 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_UART2IF           (BIT(2))                                            /*!< UART2 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_UART3IF           (BIT(3))                                            /*!< UART3 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_UART4IF           (BIT(4))                                            /*!< UART4 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_UART5IF           (BIT(5))                                            /*!< UART5 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_UART6IF           (BIT(6))                                            /*!< UART6 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_UART7IF           (BIT(7))                                            /*!< UART7 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_LPUARTIF          (BIT(8))                                            /*!< LPUART illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_USART0IF          (BIT(9))                                            /*!< USART0 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_USART1IF          (BIT(10))                                           /*!< USART1 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_ISO78160IF        (BIT(11))                                           /*!< ISO7816-0 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_ISO78161IF        (BIT(12))                                           /*!< ISO7816-1 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_SPI0IF            (BIT(13))                                           /*!< SPI0 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_SPI1IF            (BIT(14))                                           /*!< SPI1 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_SPI2IF            (BIT(15))                                           /*!< SPI2 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_SPI3IF            (BIT(16))                                           /*!< SPI3 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_I2C0IF            (BIT(17))                                           /*!< I2C0 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_I2C1IF            (BIT(18))                                           /*!< I2C1 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_CANIF             (BIT(19))                                           /*!< CAN illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_IOCNTIF           (BIT(20))                                           /*!< IOCNT illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_LCDIF             (BIT(21))                                           /*!< LCD illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_DSPIF             (BIT(22))                                           /*!< DSP illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_M2MIF             (BIT(24))                                           /*!< M2M illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_D2FIF             (BIT(25))                                           /*!< D2F illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_ADC0IF            (BIT(26))                                           /*!< ADC0 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_DACIF             (BIT(27))                                           /*!< DAC illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_TC0IF             (BIT(28))                                           /*!< TC0 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF0_TC1IF             (BIT(29))                                           /*!< TC1 illegal access interrupt flag bit */

/* SCU_PERIPH_SEC_IF1 */
#define SCU_PERIPH_SEC_IF1_SIMPTC0IF         (BIT(0))                                            /*!< SIMPTC0 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF1_SIMPTC1IF         (BIT(1))                                            /*!< SIMPTC1 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF1_SIMPTC2IF         (BIT(2))                                            /*!< SIMPTC2 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF1_SIMPTC3IF         (BIT(3))                                            /*!< SIMPTC3 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF1_RS_VDDIF          (BIT(4))                                            /*!< RS_VDD illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF1_LSRXIF            (BIT(6))                                            /*!< LSRX illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF1_KBI0IF            (BIT(7))                                            /*!< KBI0 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF1_KBI1IF            (BIT(8))                                            /*!< KBI1 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF1_LVD_CMPIF         (BIT(9))                                            /*!< LVD_CMP illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF1_CRC0IF            (BIT(10))                                           /*!< CRC0 illegal access interrupt flag bit */

/* SCU_PERIPH_SEC_IF2 */
#define SCU_PERIPH_SEC_IF2_QSPI0IF           (BIT(0))                                            /*!< QSPI0 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF2_QSPI1IF           (BIT(1))                                            /*!< QSPI1 illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF2_SEAIF             (BIT(2))                                            /*!< SEA illegal access interrupt flag bit */
#define SCU_PERIPH_SEC_IF2_AESIF             (BIT(8))                                            /*!< AES illegal access interrupt flag bit */

/* constants definitions */
/* SCU_WKEY_WKEY */
#define SCU_WKEY_WKEY_POS                    (0U)                                                /*!< SCU_WKEY_WKEY bit offset */

/* SCU_LCK_LCK */
#define SCU_LCK_LCK_POS                      (0U)                                                /*!< SCU_LCK_LCK bit offset */

/* SCU_PERIPH_SEC_CFG0_UART0SEC */
#define SCU_PERIPH_SEC_CFG0_UART0SEC_POS     (0U)                                                /*!< SCU_PERIPH_SEC_CFG0_UART0SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_UART1SEC */
#define SCU_PERIPH_SEC_CFG0_UART1SEC_POS     (1U)                                                /*!< SCU_PERIPH_SEC_CFG0_UART1SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_UART2SEC */
#define SCU_PERIPH_SEC_CFG0_UART2SEC_POS     (2U)                                                /*!< SCU_PERIPH_SEC_CFG0_UART2SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_UART3SEC */
#define SCU_PERIPH_SEC_CFG0_UART3SEC_POS     (3U)                                                /*!< SCU_PERIPH_SEC_CFG0_UART3SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_UART4SEC */
#define SCU_PERIPH_SEC_CFG0_UART4SEC_POS     (4U)                                                /*!< SCU_PERIPH_SEC_CFG0_UART4SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_UART5SEC */
#define SCU_PERIPH_SEC_CFG0_UART5SEC_POS     (5U)                                                /*!< SCU_PERIPH_SEC_CFG0_UART5SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_UART6SEC */
#define SCU_PERIPH_SEC_CFG0_UART6SEC_POS     (6U)                                                /*!< SCU_PERIPH_SEC_CFG0_UART6SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_UART7SEC */
#define SCU_PERIPH_SEC_CFG0_UART7SEC_POS     (7U)                                                /*!< SCU_PERIPH_SEC_CFG0_UART7SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_LPUARTSEC */
#define SCU_PERIPH_SEC_CFG0_LPUARTSEC_POS    (8U)                                                /*!< SCU_PERIPH_SEC_CFG0_LPUARTSEC bit offset */

/* SCU_PERIPH_SEC_CFG0_USART0SEC */
#define SCU_PERIPH_SEC_CFG0_USART0SEC_POS    (9U)                                                /*!< SCU_PERIPH_SEC_CFG0_USART0SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_USART1SEC */
#define SCU_PERIPH_SEC_CFG0_USART1SEC_POS    (10U)                                               /*!< SCU_PERIPH_SEC_CFG0_USART1SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_ISO78160SEC */
#define SCU_PERIPH_SEC_CFG0_ISO78160SEC_POS  (11U)                                               /*!< SCU_PERIPH_SEC_CFG0_ISO78160SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_ISO78161SEC */
#define SCU_PERIPH_SEC_CFG0_ISO78161SEC_POS  (12U)                                               /*!< SCU_PERIPH_SEC_CFG0_ISO78161SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_SPI0SEC */
#define SCU_PERIPH_SEC_CFG0_SPI0SEC_POS      (13U)                                               /*!< SCU_PERIPH_SEC_CFG0_SPI0SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_SPI1SEC */
#define SCU_PERIPH_SEC_CFG0_SPI1SEC_POS      (14U)                                               /*!< SCU_PERIPH_SEC_CFG0_SPI1SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_SPI2SEC */
#define SCU_PERIPH_SEC_CFG0_SPI2SEC_POS      (15U)                                               /*!< SCU_PERIPH_SEC_CFG0_SPI2SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_SPI3SEC */
#define SCU_PERIPH_SEC_CFG0_SPI3SEC_POS      (16U)                                               /*!< SCU_PERIPH_SEC_CFG0_SPI3SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_I2C0SEC */
#define SCU_PERIPH_SEC_CFG0_I2C0SEC_POS      (17U)                                               /*!< SCU_PERIPH_SEC_CFG0_I2C0SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_I2C1SEC */
#define SCU_PERIPH_SEC_CFG0_I2C1SEC_POS      (18U)                                               /*!< SCU_PERIPH_SEC_CFG0_I2C1SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_CANSEC */
#define SCU_PERIPH_SEC_CFG0_CANSEC_POS       (19U)                                               /*!< SCU_PERIPH_SEC_CFG0_CANSEC bit offset */

/* SCU_PERIPH_SEC_CFG0_IOCNTSEC */
#define SCU_PERIPH_SEC_CFG0_IOCNTSEC_POS     (20U)                                               /*!< SCU_PERIPH_SEC_CFG0_IOCNTSEC bit offset */

/* SCU_PERIPH_SEC_CFG0_LCDSEC */
#define SCU_PERIPH_SEC_CFG0_LCDSEC_POS       (21U)                                               /*!< SCU_PERIPH_SEC_CFG0_LCDSEC bit offset */

/* SCU_PERIPH_SEC_CFG0_DSPSEC */
#define SCU_PERIPH_SEC_CFG0_DSPSEC_POS       (22U)                                               /*!< SCU_PERIPH_SEC_CFG0_DSPSEC bit offset */

/* SCU_PERIPH_SEC_CFG0_M2MSEC */
#define SCU_PERIPH_SEC_CFG0_M2MSEC_POS       (24U)                                               /*!< SCU_PERIPH_SEC_CFG0_M2MSEC bit offset */

/* SCU_PERIPH_SEC_CFG0_D2FSEC */
#define SCU_PERIPH_SEC_CFG0_D2FSEC_POS       (25U)                                               /*!< SCU_PERIPH_SEC_CFG0_D2FSEC bit offset */

/* SCU_PERIPH_SEC_CFG0_ADC0SEC */
#define SCU_PERIPH_SEC_CFG0_ADC0SEC_POS      (26U)                                               /*!< SCU_PERIPH_SEC_CFG0_ADC0SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_DACSEC */
#define SCU_PERIPH_SEC_CFG0_DACSEC_POS       (27U)                                               /*!< SCU_PERIPH_SEC_CFG0_DACSEC bit offset */

/* SCU_PERIPH_SEC_CFG0_TC0SEC */
#define SCU_PERIPH_SEC_CFG0_TC0SEC_POS       (28U)                                               /*!< SCU_PERIPH_SEC_CFG0_TC0SEC bit offset */

/* SCU_PERIPH_SEC_CFG0_TC1SEC */
#define SCU_PERIPH_SEC_CFG0_TC1SEC_POS       (29U)                                               /*!< SCU_PERIPH_SEC_CFG0_TC1SEC bit offset */

/* SCU_PERIPH_SEC_CFG1_SIMPTC0SEC */
#define SCU_PERIPH_SEC_CFG1_SIMPTC0SEC_POS   (0U)                                                /*!< SCU_PERIPH_SEC_CFG1_SIMPTC0SEC bit offset */

/* SCU_PERIPH_SEC_CFG1_SIMPTC1SEC */
#define SCU_PERIPH_SEC_CFG1_SIMPTC1SEC_POS   (1U)                                                /*!< SCU_PERIPH_SEC_CFG1_SIMPTC1SEC bit offset */

/* SCU_PERIPH_SEC_CFG1_SIMPTC2SEC */
#define SCU_PERIPH_SEC_CFG1_SIMPTC2SEC_POS   (2U)                                                /*!< SCU_PERIPH_SEC_CFG1_SIMPTC2SEC bit offset */

/* SCU_PERIPH_SEC_CFG1_SIMPTC3SEC */
#define SCU_PERIPH_SEC_CFG1_SIMPTC3SEC_POS   (3U)                                                /*!< SCU_PERIPH_SEC_CFG1_SIMPTC3SEC bit offset */

/* SCU_PERIPH_SEC_CFG1_RS_VDDSEC */
#define SCU_PERIPH_SEC_CFG1_RS_VDDSEC_POS    (4U)                                                /*!< SCU_PERIPH_SEC_CFG1_RS_VDDSEC bit offset */

/* SCU_PERIPH_SEC_CFG1_LSRXSEC */
#define SCU_PERIPH_SEC_CFG1_LSRXSEC_POS      (6U)                                                /*!< SCU_PERIPH_SEC_CFG1_LSRXSEC bit offset */

/* SCU_PERIPH_SEC_CFG1_KBI0SEC */
#define SCU_PERIPH_SEC_CFG1_KBI0SEC_POS      (7U)                                                /*!< SCU_PERIPH_SEC_CFG1_KBI0SEC bit offset */

/* SCU_PERIPH_SEC_CFG1_KBI1SEC */
#define SCU_PERIPH_SEC_CFG1_KBI1SEC_POS      (8U)                                                /*!< SCU_PERIPH_SEC_CFG1_KBI1SEC bit offset */

/* SCU_PERIPH_SEC_CFG1_LVD_CMPSEC */
#define SCU_PERIPH_SEC_CFG1_LVD_CMPSEC_POS   (9U)                                                /*!< SCU_PERIPH_SEC_CFG1_LVD_CMPSEC bit offset */

/* SCU_PERIPH_SEC_CFG1_CRC0SEC */
#define SCU_PERIPH_SEC_CFG1_CRC0SEC_POS      (10U)                                               /*!< SCU_PERIPH_SEC_CFG1_CRC0SEC bit offset */

/* SCU_PERIPH_SEC_CFG2_QSPI0SEC */
#define SCU_PERIPH_SEC_CFG2_QSPI0SEC_POS     (0U)                                                /*!< SCU_PERIPH_SEC_CFG2_QSPI0SEC bit offset */

/* SCU_PERIPH_SEC_CFG2_QSPI1SEC */
#define SCU_PERIPH_SEC_CFG2_QSPI1SEC_POS     (1U)                                                /*!< SCU_PERIPH_SEC_CFG2_QSPI1SEC bit offset */

/* SCU_PERIPH_SEC_CFG2_SEASEC */
#define SCU_PERIPH_SEC_CFG2_SEASEC_POS       (2U)                                                /*!< SCU_PERIPH_SEC_CFG2_SEASEC bit offset */

/* SCU_PERIPH_SEC_CFG2_AESSEC */
#define SCU_PERIPH_SEC_CFG2_AESSEC_POS       (8U)                                                /*!< SCU_PERIPH_SEC_CFG2_AESSEC bit offset */

/* SCU_GPIOA_SEC_CFG_PA_SEC */
#define SCU_GPIOA_SEC_CFG_PA_SEC_POS         (0U)                                                /*!< SCU_GPIOA_SEC_CFG_PA_SEC bit offset */

/* SCU_GPIOB_SEC_CFG_PB_SEC */
#define SCU_GPIOB_SEC_CFG_PB_SEC_POS         (0U)                                                /*!< SCU_GPIOB_SEC_CFG_PB_SEC bit offset */

/* SCU_GPIOC_SEC_CFG_PC_SEC */
#define SCU_GPIOC_SEC_CFG_PC_SEC_POS         (0U)                                                /*!< SCU_GPIOC_SEC_CFG_PC_SEC bit offset */

/* SCU_GPIOD_SEC_CFG_PD_SEC */
#define SCU_GPIOD_SEC_CFG_PD_SEC_POS         (0U)                                                /*!< SCU_GPIOD_SEC_CFG_PD_SEC bit offset */

/* SCU_GPIOE_SEC_CFG_PE_SEC */
#define SCU_GPIOE_SEC_CFG_PE_SEC_POS         (0U)                                                /*!< SCU_GPIOE_SEC_CFG_PE_SEC bit offset */

/* SCU_GPIOF_SEC_CFG_PF_SEC */
#define SCU_GPIOF_SEC_CFG_PF_SEC_POS         (0U)                                                /*!< SCU_GPIOF_SEC_CFG_PF_SEC bit offset */

/* SCU_GPIOG_SEC_CFG_PG_SEC */
#define SCU_GPIOG_SEC_CFG_PG_SEC_POS         (0U)                                                /*!< SCU_GPIOG_SEC_CFG_PG_SEC bit offset */

/* SCU_GPIOH_SEC_CFG_PH_SEC */
#define SCU_GPIOH_SEC_CFG_PH_SEC_POS         (0U)                                                /*!< SCU_GPIOH_SEC_CFG_PH_SEC bit offset */

/* SCU_SRAM_SEC_WM_CFG_SRAM_SEC_LEN */
#define SCU_SRAM_SEC_WM_CFG_SRAM_SEC_LEN_POS  (0U)                                                /*!< SCU_SRAM_SEC_WM_CFG_SRAM_SEC_LEN bit offset */

/* SCU_PERIPH_SEC_IE0_UART0IE */
#define SCU_PERIPH_SEC_IE0_UART0IE_POS       (0U)                                                /*!< SCU_PERIPH_SEC_IE0_UART0IE bit offset */

/* SCU_PERIPH_SEC_IE0_UART1IE */
#define SCU_PERIPH_SEC_IE0_UART1IE_POS       (1U)                                                /*!< SCU_PERIPH_SEC_IE0_UART1IE bit offset */

/* SCU_PERIPH_SEC_IE0_UART2IE */
#define SCU_PERIPH_SEC_IE0_UART2IE_POS       (2U)                                                /*!< SCU_PERIPH_SEC_IE0_UART2IE bit offset */

/* SCU_PERIPH_SEC_IE0_UART3IE */
#define SCU_PERIPH_SEC_IE0_UART3IE_POS       (3U)                                                /*!< SCU_PERIPH_SEC_IE0_UART3IE bit offset */

/* SCU_PERIPH_SEC_IE0_UART4IE */
#define SCU_PERIPH_SEC_IE0_UART4IE_POS       (4U)                                                /*!< SCU_PERIPH_SEC_IE0_UART4IE bit offset */

/* SCU_PERIPH_SEC_IE0_UART5IE */
#define SCU_PERIPH_SEC_IE0_UART5IE_POS       (5U)                                                /*!< SCU_PERIPH_SEC_IE0_UART5IE bit offset */

/* SCU_PERIPH_SEC_IE0_UART6IE */
#define SCU_PERIPH_SEC_IE0_UART6IE_POS       (6U)                                                /*!< SCU_PERIPH_SEC_IE0_UART6IE bit offset */

/* SCU_PERIPH_SEC_IE0_UART7IE */
#define SCU_PERIPH_SEC_IE0_UART7IE_POS       (7U)                                                /*!< SCU_PERIPH_SEC_IE0_UART7IE bit offset */

/* SCU_PERIPH_SEC_IE0_LPUARTIE */
#define SCU_PERIPH_SEC_IE0_LPUARTIE_POS      (8U)                                                /*!< SCU_PERIPH_SEC_IE0_LPUARTIE bit offset */

/* SCU_PERIPH_SEC_IE0_USART0IE */
#define SCU_PERIPH_SEC_IE0_USART0IE_POS      (9U)                                                /*!< SCU_PERIPH_SEC_IE0_USART0IE bit offset */

/* SCU_PERIPH_SEC_IE0_USART1IE */
#define SCU_PERIPH_SEC_IE0_USART1IE_POS      (10U)                                               /*!< SCU_PERIPH_SEC_IE0_USART1IE bit offset */

/* SCU_PERIPH_SEC_IE0_ISO78160IE */
#define SCU_PERIPH_SEC_IE0_ISO78160IE_POS    (11U)                                               /*!< SCU_PERIPH_SEC_IE0_ISO78160IE bit offset */

/* SCU_PERIPH_SEC_IE0_ISO78161IE */
#define SCU_PERIPH_SEC_IE0_ISO78161IE_POS    (12U)                                               /*!< SCU_PERIPH_SEC_IE0_ISO78161IE bit offset */

/* SCU_PERIPH_SEC_IE0_SPI0IE */
#define SCU_PERIPH_SEC_IE0_SPI0IE_POS        (13U)                                               /*!< SCU_PERIPH_SEC_IE0_SPI0IE bit offset */

/* SCU_PERIPH_SEC_IE0_SPI1IE */
#define SCU_PERIPH_SEC_IE0_SPI1IE_POS        (14U)                                               /*!< SCU_PERIPH_SEC_IE0_SPI1IE bit offset */

/* SCU_PERIPH_SEC_IE0_SPI2IE */
#define SCU_PERIPH_SEC_IE0_SPI2IE_POS        (15U)                                               /*!< SCU_PERIPH_SEC_IE0_SPI2IE bit offset */

/* SCU_PERIPH_SEC_IE0_SPI3IE */
#define SCU_PERIPH_SEC_IE0_SPI3IE_POS        (16U)                                               /*!< SCU_PERIPH_SEC_IE0_SPI3IE bit offset */

/* SCU_PERIPH_SEC_IE0_I2C0IE */
#define SCU_PERIPH_SEC_IE0_I2C0IE_POS        (17U)                                               /*!< SCU_PERIPH_SEC_IE0_I2C0IE bit offset */

/* SCU_PERIPH_SEC_IE0_I2C1IE */
#define SCU_PERIPH_SEC_IE0_I2C1IE_POS        (18U)                                               /*!< SCU_PERIPH_SEC_IE0_I2C1IE bit offset */

/* SCU_PERIPH_SEC_IE0_CANIE */
#define SCU_PERIPH_SEC_IE0_CANIE_POS         (19U)                                               /*!< SCU_PERIPH_SEC_IE0_CANIE bit offset */

/* SCU_PERIPH_SEC_IE0_IOCNTIE */
#define SCU_PERIPH_SEC_IE0_IOCNTIE_POS       (20U)                                               /*!< SCU_PERIPH_SEC_IE0_IOCNTIE bit offset */

/* SCU_PERIPH_SEC_IE0_LCDIE */
#define SCU_PERIPH_SEC_IE0_LCDIE_POS         (21U)                                               /*!< SCU_PERIPH_SEC_IE0_LCDIE bit offset */

/* SCU_PERIPH_SEC_IE0_DSPIE */
#define SCU_PERIPH_SEC_IE0_DSPIE_POS         (22U)                                               /*!< SCU_PERIPH_SEC_IE0_DSPIE bit offset */

/* SCU_PERIPH_SEC_IE0_M2MIE */
#define SCU_PERIPH_SEC_IE0_M2MIE_POS         (24U)                                               /*!< SCU_PERIPH_SEC_IE0_M2MIE bit offset */

/* SCU_PERIPH_SEC_IE0_D2FIE */
#define SCU_PERIPH_SEC_IE0_D2FIE_POS         (25U)                                               /*!< SCU_PERIPH_SEC_IE0_D2FIE bit offset */

/* SCU_PERIPH_SEC_IE0_ADC0IE */
#define SCU_PERIPH_SEC_IE0_ADC0IE_POS        (26U)                                               /*!< SCU_PERIPH_SEC_IE0_ADC0IE bit offset */

/* SCU_PERIPH_SEC_IE0_DACIE */
#define SCU_PERIPH_SEC_IE0_DACIE_POS         (27U)                                               /*!< SCU_PERIPH_SEC_IE0_DACIE bit offset */

/* SCU_PERIPH_SEC_IE0_TC0IE */
#define SCU_PERIPH_SEC_IE0_TC0IE_POS         (28U)                                               /*!< SCU_PERIPH_SEC_IE0_TC0IE bit offset */

/* SCU_PERIPH_SEC_IE0_TC1IE */
#define SCU_PERIPH_SEC_IE0_TC1IE_POS         (29U)                                               /*!< SCU_PERIPH_SEC_IE0_TC1IE bit offset */

/* SCU_PERIPH_SEC_IE1_SIMPTC0IE */
#define SCU_PERIPH_SEC_IE1_SIMPTC0IE_POS     (0U)                                                /*!< SCU_PERIPH_SEC_IE1_SIMPTC0IE bit offset */

/* SCU_PERIPH_SEC_IE1_SIMPTC1IE */
#define SCU_PERIPH_SEC_IE1_SIMPTC1IE_POS     (1U)                                                /*!< SCU_PERIPH_SEC_IE1_SIMPTC1IE bit offset */

/* SCU_PERIPH_SEC_IE1_SIMPTC2IE */
#define SCU_PERIPH_SEC_IE1_SIMPTC2IE_POS     (2U)                                                /*!< SCU_PERIPH_SEC_IE1_SIMPTC2IE bit offset */

/* SCU_PERIPH_SEC_IE1_SIMPTC3IE */
#define SCU_PERIPH_SEC_IE1_SIMPTC3IE_POS     (3U)                                                /*!< SCU_PERIPH_SEC_IE1_SIMPTC3IE bit offset */

/* SCU_PERIPH_SEC_IE1_RS_VDDIE */
#define SCU_PERIPH_SEC_IE1_RS_VDDIE_POS      (4U)                                                /*!< SCU_PERIPH_SEC_IE1_RS_VDDIE bit offset */

/* SCU_PERIPH_SEC_IE1_LSRXIE */
#define SCU_PERIPH_SEC_IE1_LSRXIE_POS        (6U)                                                /*!< SCU_PERIPH_SEC_IE1_LSRXIE bit offset */

/* SCU_PERIPH_SEC_IE1_KBI0IE */
#define SCU_PERIPH_SEC_IE1_KBI0IE_POS        (7U)                                                /*!< SCU_PERIPH_SEC_IE1_KBI0IE bit offset */

/* SCU_PERIPH_SEC_IE1_KBI1IE */
#define SCU_PERIPH_SEC_IE1_KBI1IE_POS        (8U)                                                /*!< SCU_PERIPH_SEC_IE1_KBI1IE bit offset */

/* SCU_PERIPH_SEC_IE1_LVD_CMPIE */
#define SCU_PERIPH_SEC_IE1_LVD_CMPIE_POS     (9U)                                                /*!< SCU_PERIPH_SEC_IE1_LVD_CMPIE bit offset */

/* SCU_PERIPH_SEC_IE1_CRC0IE */
#define SCU_PERIPH_SEC_IE1_CRC0IE_POS        (10U)                                               /*!< SCU_PERIPH_SEC_IE1_CRC0IE bit offset */

/* SCU_PERIPH_SEC_IE2_QSPI0IE */
#define SCU_PERIPH_SEC_IE2_QSPI0IE_POS       (0U)                                                /*!< SCU_PERIPH_SEC_IE2_QSPI0IE bit offset */

/* SCU_PERIPH_SEC_IE2_QSPI1IE */
#define SCU_PERIPH_SEC_IE2_QSPI1IE_POS       (1U)                                                /*!< SCU_PERIPH_SEC_IE2_QSPI1IE bit offset */

/* SCU_PERIPH_SEC_IE2_SEAIE */
#define SCU_PERIPH_SEC_IE2_SEAIE_POS         (2U)                                                /*!< SCU_PERIPH_SEC_IE2_SEAIE bit offset */

/* SCU_PERIPH_SEC_IE2_AESIE */
#define SCU_PERIPH_SEC_IE2_AESIE_POS         (8U)                                                /*!< SCU_PERIPH_SEC_IE2_AESIE bit offset */

/* SCU_PERIPH_SEC_IF0_UART0IF */
#define SCU_PERIPH_SEC_IF0_UART0IF_POS       (0U)                                                /*!< SCU_PERIPH_SEC_IF0_UART0IF bit offset */

/* SCU_PERIPH_SEC_IF0_UART1IF */
#define SCU_PERIPH_SEC_IF0_UART1IF_POS       (1U)                                                /*!< SCU_PERIPH_SEC_IF0_UART1IF bit offset */

/* SCU_PERIPH_SEC_IF0_UART2IF */
#define SCU_PERIPH_SEC_IF0_UART2IF_POS       (2U)                                                /*!< SCU_PERIPH_SEC_IF0_UART2IF bit offset */

/* SCU_PERIPH_SEC_IF0_UART3IF */
#define SCU_PERIPH_SEC_IF0_UART3IF_POS       (3U)                                                /*!< SCU_PERIPH_SEC_IF0_UART3IF bit offset */

/* SCU_PERIPH_SEC_IF0_UART4IF */
#define SCU_PERIPH_SEC_IF0_UART4IF_POS       (4U)                                                /*!< SCU_PERIPH_SEC_IF0_UART4IF bit offset */

/* SCU_PERIPH_SEC_IF0_UART5IF */
#define SCU_PERIPH_SEC_IF0_UART5IF_POS       (5U)                                                /*!< SCU_PERIPH_SEC_IF0_UART5IF bit offset */

/* SCU_PERIPH_SEC_IF0_UART6IF */
#define SCU_PERIPH_SEC_IF0_UART6IF_POS       (6U)                                                /*!< SCU_PERIPH_SEC_IF0_UART6IF bit offset */

/* SCU_PERIPH_SEC_IF0_UART7IF */
#define SCU_PERIPH_SEC_IF0_UART7IF_POS       (7U)                                                /*!< SCU_PERIPH_SEC_IF0_UART7IF bit offset */

/* SCU_PERIPH_SEC_IF0_LPUARTIF */
#define SCU_PERIPH_SEC_IF0_LPUARTIF_POS      (8U)                                                /*!< SCU_PERIPH_SEC_IF0_LPUARTIF bit offset */

/* SCU_PERIPH_SEC_IF0_USART0IF */
#define SCU_PERIPH_SEC_IF0_USART0IF_POS      (9U)                                                /*!< SCU_PERIPH_SEC_IF0_USART0IF bit offset */

/* SCU_PERIPH_SEC_IF0_USART1IF */
#define SCU_PERIPH_SEC_IF0_USART1IF_POS      (10U)                                               /*!< SCU_PERIPH_SEC_IF0_USART1IF bit offset */

/* SCU_PERIPH_SEC_IF0_ISO78160IF */
#define SCU_PERIPH_SEC_IF0_ISO78160IF_POS    (11U)                                               /*!< SCU_PERIPH_SEC_IF0_ISO78160IF bit offset */

/* SCU_PERIPH_SEC_IF0_ISO78161IF */
#define SCU_PERIPH_SEC_IF0_ISO78161IF_POS    (12U)                                               /*!< SCU_PERIPH_SEC_IF0_ISO78161IF bit offset */

/* SCU_PERIPH_SEC_IF0_SPI0IF */
#define SCU_PERIPH_SEC_IF0_SPI0IF_POS        (13U)                                               /*!< SCU_PERIPH_SEC_IF0_SPI0IF bit offset */

/* SCU_PERIPH_SEC_IF0_SPI1IF */
#define SCU_PERIPH_SEC_IF0_SPI1IF_POS        (14U)                                               /*!< SCU_PERIPH_SEC_IF0_SPI1IF bit offset */

/* SCU_PERIPH_SEC_IF0_SPI2IF */
#define SCU_PERIPH_SEC_IF0_SPI2IF_POS        (15U)                                               /*!< SCU_PERIPH_SEC_IF0_SPI2IF bit offset */

/* SCU_PERIPH_SEC_IF0_SPI3IF */
#define SCU_PERIPH_SEC_IF0_SPI3IF_POS        (16U)                                               /*!< SCU_PERIPH_SEC_IF0_SPI3IF bit offset */

/* SCU_PERIPH_SEC_IF0_I2C0IF */
#define SCU_PERIPH_SEC_IF0_I2C0IF_POS        (17U)                                               /*!< SCU_PERIPH_SEC_IF0_I2C0IF bit offset */

/* SCU_PERIPH_SEC_IF0_I2C1IF */
#define SCU_PERIPH_SEC_IF0_I2C1IF_POS        (18U)                                               /*!< SCU_PERIPH_SEC_IF0_I2C1IF bit offset */

/* SCU_PERIPH_SEC_IF0_CANIF */
#define SCU_PERIPH_SEC_IF0_CANIF_POS         (19U)                                               /*!< SCU_PERIPH_SEC_IF0_CANIF bit offset */

/* SCU_PERIPH_SEC_IF0_IOCNTIF */
#define SCU_PERIPH_SEC_IF0_IOCNTIF_POS       (20U)                                               /*!< SCU_PERIPH_SEC_IF0_IOCNTIF bit offset */

/* SCU_PERIPH_SEC_IF0_LCDIF */
#define SCU_PERIPH_SEC_IF0_LCDIF_POS         (21U)                                               /*!< SCU_PERIPH_SEC_IF0_LCDIF bit offset */

/* SCU_PERIPH_SEC_IF0_DSPIF */
#define SCU_PERIPH_SEC_IF0_DSPIF_POS         (22U)                                               /*!< SCU_PERIPH_SEC_IF0_DSPIF bit offset */

/* SCU_PERIPH_SEC_IF0_M2MIF */
#define SCU_PERIPH_SEC_IF0_M2MIF_POS         (24U)                                               /*!< SCU_PERIPH_SEC_IF0_M2MIF bit offset */

/* SCU_PERIPH_SEC_IF0_D2FIF */
#define SCU_PERIPH_SEC_IF0_D2FIF_POS         (25U)                                               /*!< SCU_PERIPH_SEC_IF0_D2FIF bit offset */

/* SCU_PERIPH_SEC_IF0_ADC0IF */
#define SCU_PERIPH_SEC_IF0_ADC0IF_POS        (26U)                                               /*!< SCU_PERIPH_SEC_IF0_ADC0IF bit offset */

/* SCU_PERIPH_SEC_IF0_DACIF */
#define SCU_PERIPH_SEC_IF0_DACIF_POS         (27U)                                               /*!< SCU_PERIPH_SEC_IF0_DACIF bit offset */

/* SCU_PERIPH_SEC_IF0_TC0IF */
#define SCU_PERIPH_SEC_IF0_TC0IF_POS         (28U)                                               /*!< SCU_PERIPH_SEC_IF0_TC0IF bit offset */

/* SCU_PERIPH_SEC_IF0_TC1IF */
#define SCU_PERIPH_SEC_IF0_TC1IF_POS         (29U)                                               /*!< SCU_PERIPH_SEC_IF0_TC1IF bit offset */

/* SCU_PERIPH_SEC_IF1_SIMPTC0IF */
#define SCU_PERIPH_SEC_IF1_SIMPTC0IF_POS     (0U)                                                /*!< SCU_PERIPH_SEC_IF1_SIMPTC0IF bit offset */

/* SCU_PERIPH_SEC_IF1_SIMPTC1IF */
#define SCU_PERIPH_SEC_IF1_SIMPTC1IF_POS     (1U)                                                /*!< SCU_PERIPH_SEC_IF1_SIMPTC1IF bit offset */

/* SCU_PERIPH_SEC_IF1_SIMPTC2IF */
#define SCU_PERIPH_SEC_IF1_SIMPTC2IF_POS     (2U)                                                /*!< SCU_PERIPH_SEC_IF1_SIMPTC2IF bit offset */

/* SCU_PERIPH_SEC_IF1_SIMPTC3IF */
#define SCU_PERIPH_SEC_IF1_SIMPTC3IF_POS     (3U)                                                /*!< SCU_PERIPH_SEC_IF1_SIMPTC3IF bit offset */

/* SCU_PERIPH_SEC_IF1_RS_VDDIF */
#define SCU_PERIPH_SEC_IF1_RS_VDDIF_POS      (4U)                                                /*!< SCU_PERIPH_SEC_IF1_RS_VDDIF bit offset */

/* SCU_PERIPH_SEC_IF1_LSRXIF */
#define SCU_PERIPH_SEC_IF1_LSRXIF_POS        (6U)                                                /*!< SCU_PERIPH_SEC_IF1_LSRXIF bit offset */

/* SCU_PERIPH_SEC_IF1_KBI0IF */
#define SCU_PERIPH_SEC_IF1_KBI0IF_POS        (7U)                                                /*!< SCU_PERIPH_SEC_IF1_KBI0IF bit offset */

/* SCU_PERIPH_SEC_IF1_KBI1IF */
#define SCU_PERIPH_SEC_IF1_KBI1IF_POS        (8U)                                                /*!< SCU_PERIPH_SEC_IF1_KBI1IF bit offset */

/* SCU_PERIPH_SEC_IF1_LVD_CMPIF */
#define SCU_PERIPH_SEC_IF1_LVD_CMPIF_POS     (9U)                                                /*!< SCU_PERIPH_SEC_IF1_LVD_CMPIF bit offset */

/* SCU_PERIPH_SEC_IF1_CRC0IF */
#define SCU_PERIPH_SEC_IF1_CRC0IF_POS        (10U)                                               /*!< SCU_PERIPH_SEC_IF1_CRC0IF bit offset */

/* SCU_PERIPH_SEC_IF2_QSPI0IF */
#define SCU_PERIPH_SEC_IF2_QSPI0IF_POS       (0U)                                                /*!< SCU_PERIPH_SEC_IF2_QSPI0IF bit offset */

/* SCU_PERIPH_SEC_IF2_QSPI1IF */
#define SCU_PERIPH_SEC_IF2_QSPI1IF_POS       (1U)                                                /*!< SCU_PERIPH_SEC_IF2_QSPI1IF bit offset */

/* SCU_PERIPH_SEC_IF2_SEAIF */
#define SCU_PERIPH_SEC_IF2_SEAIF_POS         (2U)                                                /*!< SCU_PERIPH_SEC_IF2_SEAIF bit offset */

/* SCU_PERIPH_SEC_IF2_AESIF */
#define SCU_PERIPH_SEC_IF2_AESIF_POS         (8U)                                                /*!< SCU_PERIPH_SEC_IF2_AESIF bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_SCU_H */
