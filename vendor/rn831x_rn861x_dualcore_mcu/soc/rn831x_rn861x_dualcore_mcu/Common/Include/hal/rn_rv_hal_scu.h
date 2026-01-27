/**
 ******************************************************************************
 * @file   rn_rv_hal_scu.h
 * @author Renergy Software Team
 * @brief  Header file of SCU HAL module.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RN_RV_HAL_SCU_H
#define __RN_RV_HAL_SCU_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal_def.h"
#include "rn_rv_ll_scu.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup SCU
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup SCU_Exported_Types SCU Exported Types
 * @{
 */
/**
 * @brief  SRAM Secure Length Type enumeration
 */
typedef enum __attribute__((__packed__))
{
    HAL_SCU_SRAM_SIZE_16KB       = 0x1U,
    HAL_SCU_SRAM_SIZE_32KB       = 0x2U,
    HAL_SCU_SRAM_SIZE_48KB       = 0x3U,
    HAL_SCU_SRAM_SIZE_64KB       = 0x4U,
    HAL_SCU_SRAM_SIZE_80KB       = 0x5U,
    HAL_SCU_SRAM_SIZE_96KB       = 0x6U,
    HAL_SCU_SRAM_SIZE_112KB      = 0x7U,
    HAL_SCU_SRAM_SIZE_128KB      = 0x8U,
    HAL_SCU_SRAM_SIZE_144KB      = 0x9U,
    HAL_SCU_SRAM_SIZE_160KB      = 0xAU,
} HAL_SCU_SRAMSecLenTypeDef;
/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup SCU_Exported_Constants SCU Exported Constants
 * @{
 */
/** @defgroup SCU_EC_GPIO_PIN  GPIO PIN
 * @{
 */
#define HAL_SCU_GPIO_PIN_00             (0x00000001U)       /*!< Select pin 00  */
#define HAL_SCU_GPIO_PIN_01             (0x00000002U)       /*!< Select pin 01  */
#define HAL_SCU_GPIO_PIN_02             (0x00000004U)       /*!< Select pin 02  */
#define HAL_SCU_GPIO_PIN_03             (0x00000008U)       /*!< Select pin 03  */
#define HAL_SCU_GPIO_PIN_04             (0x00000010U)       /*!< Select pin 04  */
#define HAL_SCU_GPIO_PIN_05             (0x00000020U)       /*!< Select pin 05  */
#define HAL_SCU_GPIO_PIN_06             (0x00000040U)       /*!< Select pin 06  */
#define HAL_SCU_GPIO_PIN_07             (0x00000080U)       /*!< Select pin 07  */
#define HAL_SCU_GPIO_PIN_08             (0x00000100U)       /*!< Select pin 08  */
#define HAL_SCU_GPIO_PIN_09             (0x00000200U)       /*!< Select pin 09  */
#define HAL_SCU_GPIO_PIN_10             (0x00000400U)       /*!< Select pin 10  */
#define HAL_SCU_GPIO_PIN_11             (0x00000800U)       /*!< Select pin 11  */
#define HAL_SCU_GPIO_PIN_12             (0x00001000U)       /*!< Select pin 12  */
#define HAL_SCU_GPIO_PIN_13             (0x00002000U)       /*!< Select pin 13  */
#define HAL_SCU_GPIO_PIN_14             (0x00004000U)       /*!< Select pin 14  */
#define HAL_SCU_GPIO_PIN_15             (0x00008000U)       /*!< Select pin 15  */
#define HAL_SCU_GPIO_PIN_All            (0x0000FFFFU)       /*!< Select pin ALL */
/**
 * @}
 */

/** @defgroup SCU_Interrupt Interrupts and Flags
 * @{
 */
/* SCU_PERIPH_SEC_IE0 */
#define SCU_PERIPH0_IT_UART0                    (SCU_PERIPH_SEC_IE0_UART0IE)              /*!< UART0 SCU Illegal Access Interrupt    */
#define SCU_PERIPH0_IT_UART1                    (SCU_PERIPH_SEC_IE0_UART1IE)              /*!< UART1 SCU Illegal Access Interrupt    */
#define SCU_PERIPH0_IT_UART2                    (SCU_PERIPH_SEC_IE0_UART2IE)              /*!< UART2 SCU Illegal Access Interrupt    */
#define SCU_PERIPH0_IT_UART3                    (SCU_PERIPH_SEC_IE0_UART3IE)              /*!< UART3 SCU Illegal Access Interrupt    */
#define SCU_PERIPH0_IT_UART4                    (SCU_PERIPH_SEC_IE0_UART4IE)              /*!< UART4 SCU Illegal Access Interrupt    */
#define SCU_PERIPH0_IT_UART5                    (SCU_PERIPH_SEC_IE0_UART5IE)              /*!< UART5 SCU Illegal Access Interrupt    */
#define SCU_PERIPH0_IT_UART6                    (SCU_PERIPH_SEC_IE0_UART6IE)              /*!< UART6 SCU Illegal Access Interrupt    */
#define SCU_PERIPH0_IT_UART7                    (SCU_PERIPH_SEC_IE0_UART7IE)              /*!< UART7 SCU Illegal Access Interrupt    */
#define SCU_PERIPH0_IT_LPUART                   (SCU_PERIPH_SEC_IE0_LPUARTIE)             /*!< LPUART SCU Illegal Access Interrupt   */
#define SCU_PERIPH0_IT_USART0                   (SCU_PERIPH_SEC_IE0_USART0IE)             /*!< USART0 SCU Illegal Access Interrupt   */
#define SCU_PERIPH0_IT_USART1                   (SCU_PERIPH_SEC_IE0_USART1IE)             /*!< USART1 SCU Illegal Access Interrupt   */
#define SCU_PERIPH0_IT_ISO78160                 (SCU_PERIPH_SEC_IE0_ISO78160IE)           /*!< ISO78160 SCU Illegal Access Interrupt */
#define SCU_PERIPH0_IT_ISO78161                 (SCU_PERIPH_SEC_IE0_ISO78161IE)           /*!< ISO78161 SCU Illegal Access Interrupt */
#define SCU_PERIPH0_IT_SPI0                     (SCU_PERIPH_SEC_IE0_SPI0IE)               /*!< SPI0 SCU Illegal Access Interrupt     */
#define SCU_PERIPH0_IT_SPI1                     (SCU_PERIPH_SEC_IE0_SPI1IE)               /*!< SPI1 SCU Illegal Access Interrupt     */
#define SCU_PERIPH0_IT_SPI2                     (SCU_PERIPH_SEC_IE0_SPI2IE)               /*!< SPI2 SCU Illegal Access Interrupt     */
#define SCU_PERIPH0_IT_SPI3                     (SCU_PERIPH_SEC_IE0_SPI3IE)               /*!< SPI3 SCU Illegal Access Interrupt     */
#define SCU_PERIPH0_IT_I2C0                     (SCU_PERIPH_SEC_IE0_I2C0IE)               /*!< I2C0 SCU Illegal Access Interrupt     */
#define SCU_PERIPH0_IT_I2C1                     (SCU_PERIPH_SEC_IE0_I2C1IE)               /*!< I2C1 SCU Illegal Access Interrupt     */
#define SCU_PERIPH0_IT_IOCNT                    (SCU_PERIPH_SEC_IE0_IOCNTIE)              /*!< IOCNT SCU Illegal Access Interrupt    */
#define SCU_PERIPH0_IT_LCD                      (SCU_PERIPH_SEC_IE0_LCDIE)                /*!< LCD SCU Illegal Access Interrupt      */
#define SCU_PERIPH0_IT_DSP                      (SCU_PERIPH_SEC_IE0_DSPIE)                /*!< DSP SCU Illegal Access Interrupt      */
#define SCU_PERIPH0_IT_M2M                      (SCU_PERIPH_SEC_IE0_M2MIE)                /*!< M2M SCU Illegal Access Interrupt      */
#define SCU_PERIPH0_IT_D2F                      (SCU_PERIPH_SEC_IE0_D2FIE)                /*!< D2F SCU Illegal Access Interrupt      */
#define SCU_PERIPH0_IT_ADC0                     (SCU_PERIPH_SEC_IE0_ADC0IE)               /*!< ADC0 SCU Illegal Access Interrupt     */
#define SCU_PERIPH0_IT_DAC                      (SCU_PERIPH_SEC_IE0_DACIE)                /*!< DAC SCU Illegal Access Interrupt      */
#define SCU_PERIPH0_IT_TC0                      (SCU_PERIPH_SEC_IE0_TC0IE)                /*!< TC0 SCU Illegal Access Interrupt      */
#define SCU_PERIPH0_IT_TC1                      (SCU_PERIPH_SEC_IE0_TC1IE)                /*!< TC1 SCU Illegal Access Interrupt      */
/* SCU_PERIPH_SEC_IE1 */
#define SCU_PERIPH1_IT_SIMPTC0                  (SCU_PERIPH_SEC_IE1_SIMPTC0IE)            /*!< SIMPTC0 SCU Illegal Access Interrupt  */
#define SCU_PERIPH1_IT_SIMPTC1                  (SCU_PERIPH_SEC_IE1_SIMPTC1IE)            /*!< SIMPTC1 SCU Illegal Access Interrupt  */
#define SCU_PERIPH1_IT_SIMPTC2                  (SCU_PERIPH_SEC_IE1_SIMPTC2IE)            /*!< SIMPTC2 SCU Illegal Access Interrupt  */
#define SCU_PERIPH1_IT_SIMPTC3                  (SCU_PERIPH_SEC_IE1_SIMPTC3IE)            /*!< SIMPTC3 SCU Illegal Access Interrupt  */
#define SCU_PERIPH1_IT_ANA                      (SCU_PERIPH_SEC_IE1_RS_VDDIE)             /*!< ANA SCU Illegal Access Interrupt      */
#define SCU_PERIPH1_IT_LSRX                     (SCU_PERIPH_SEC_IE1_LSRXIE)               /*!< LSRX SCU Illegal Access Interrupt     */
#define SCU_PERIPH1_IT_KBI0                     (SCU_PERIPH_SEC_IE1_KBI0IE)               /*!< KBI0 SCU Illegal Access Interrupt     */
#define SCU_PERIPH1_IT_KBI1                     (SCU_PERIPH_SEC_IE1_KBI1IE)               /*!< KBI1 SCU Illegal Access Interrupt     */
#define SCU_PERIPH1_IT_LVDCMP                   (SCU_PERIPH_SEC_IE1_LVD_CMPIE)            /*!< LVDCMP SCU Illegal Access Interrupt   */
#define SCU_PERIPH1_IT_CRC0                     (SCU_PERIPH_SEC_IE1_CRC0IE)               /*!< CRC0 SCU Illegal Access Interrupt     */
/* SCU_PERIPH_SEC_IE2 */
#define SCU_PERIPH2_IT_QSPI0                    (SCU_PERIPH_SEC_IE2_QSPI0IE)              /*!< QSPI0 SCU Illegal Access Interrupt    */
#define SCU_PERIPH2_IT_QSPI1                    (SCU_PERIPH_SEC_IE2_QSPI1IE)              /*!< QSPI1 SCU Illegal Access Interrupt    */
#define SCU_PERIPH2_IT_SEA                      (SCU_PERIPH_SEC_IE2_SEAIE)                /*!< SEA SCU Illegal Access Interrupt      */
#define SCU_PERIPH2_IT_AES                      (SCU_PERIPH_SEC_IE2_AESIE)                /*!< AES SCU Illegal Access Interrupt      */

/* SCU_PERIPH_SEC_IF0 */
#define SCU_PERIPH0_IF_UART0                    (SCU_PERIPH_SEC_IF0_UART0IF)              /*!< UART0 SCU Illegal Access Interrupt flag    */
#define SCU_PERIPH0_IF_UART1                    (SCU_PERIPH_SEC_IF0_UART1IF)              /*!< UART1 SCU Illegal Access Interrupt flag    */
#define SCU_PERIPH0_IF_UART2                    (SCU_PERIPH_SEC_IF0_UART2IF)              /*!< UART2 SCU Illegal Access Interrupt flag    */
#define SCU_PERIPH0_IF_UART3                    (SCU_PERIPH_SEC_IF0_UART3IF)              /*!< UART3 SCU Illegal Access Interrupt flag    */
#define SCU_PERIPH0_IF_UART4                    (SCU_PERIPH_SEC_IF0_UART4IF)              /*!< UART4 SCU Illegal Access Interrupt flag    */
#define SCU_PERIPH0_IF_UART5                    (SCU_PERIPH_SEC_IF0_UART5IF)              /*!< UART5 SCU Illegal Access Interrupt flag    */
#define SCU_PERIPH0_IF_UART6                    (SCU_PERIPH_SEC_IF0_UART6IF)              /*!< UART6 SCU Illegal Access Interrupt flag    */
#define SCU_PERIPH0_IF_UART7                    (SCU_PERIPH_SEC_IF0_UART7IF)              /*!< UART7 SCU Illegal Access Interrupt flag    */
#define SCU_PERIPH0_IF_LPUART                   (SCU_PERIPH_SEC_IF0_LPUARTIF)             /*!< LPUART SCU Illegal Access Interrupt flag   */
#define SCU_PERIPH0_IF_USART0                   (SCU_PERIPH_SEC_IF0_USART0IF)             /*!< USART0 SCU Illegal Access Interrupt flag   */
#define SCU_PERIPH0_IF_USART1                   (SCU_PERIPH_SEC_IF0_USART1IF)             /*!< USART1 SCU Illegal Access Interrupt flag   */
#define SCU_PERIPH0_IF_ISO78160                 (SCU_PERIPH_SEC_IF0_ISO78160IF)           /*!< ISO78160 SCU Illegal Access Interrupt flag */
#define SCU_PERIPH0_IF_ISO78161                 (SCU_PERIPH_SEC_IF0_ISO78161IF)           /*!< ISO78161 SCU Illegal Access Interrupt flag */
#define SCU_PERIPH0_IF_SPI0                     (SCU_PERIPH_SEC_IF0_SPI0IF)               /*!< SPI0 SCU Illegal Access Interrupt flag     */
#define SCU_PERIPH0_IF_SPI1                     (SCU_PERIPH_SEC_IF0_SPI1IF)               /*!< SPI1 SCU Illegal Access Interrupt flag     */
#define SCU_PERIPH0_IF_SPI2                     (SCU_PERIPH_SEC_IF0_SPI2IF)               /*!< SPI2 SCU Illegal Access Interrupt flag     */
#define SCU_PERIPH0_IF_SPI3                     (SCU_PERIPH_SEC_IF0_SPI3IF)               /*!< SPI3 SCU Illegal Access Interrupt flag     */
#define SCU_PERIPH0_IF_I2C0                     (SCU_PERIPH_SEC_IF0_I2C0IF)               /*!< I2C0 SCU Illegal Access Interrupt flag     */
#define SCU_PERIPH0_IF_I2C1                     (SCU_PERIPH_SEC_IF0_I2C1IF)               /*!< I2C1 SCU Illegal Access Interrupt flag     */
#define SCU_PERIPH0_IF_IOCNT                    (SCU_PERIPH_SEC_IF0_IOCNTIF)              /*!< IOCNT SCU Illegal Access Interrupt flag    */
#define SCU_PERIPH0_IF_LCD                      (SCU_PERIPH_SEC_IF0_LCDIF)                /*!< LCD SCU Illegal Access Interrupt flag      */
#define SCU_PERIPH0_IF_DSP                      (SCU_PERIPH_SEC_IF0_DSPIF)                /*!< DSP SCU Illegal Access Interrupt flag      */
#define SCU_PERIPH0_IF_M2M                      (SCU_PERIPH_SEC_IF0_M2MIF)                /*!< M2M SCU Illegal Access Interrupt flag      */
#define SCU_PERIPH0_IF_D2F                      (SCU_PERIPH_SEC_IF0_D2FIF)                /*!< D2F SCU Illegal Access Interrupt flag      */
#define SCU_PERIPH0_IF_ADC0                     (SCU_PERIPH_SEC_IF0_ADC0IF)               /*!< ADC0 SCU Illegal Access Interrupt flag     */
#define SCU_PERIPH0_IF_DAC                      (SCU_PERIPH_SEC_IF0_DACIF)                /*!< DAC SCU Illegal Access Interrupt flag      */
#define SCU_PERIPH0_IF_TC0                      (SCU_PERIPH_SEC_IF0_TC0IF)                /*!< TC0 SCU Illegal Access Interrupt flag      */
#define SCU_PERIPH0_IF_TC1                      (SCU_PERIPH_SEC_IF0_TC1IF)                /*!< TC1 SCU Illegal Access Interrupt flag      */
/* SCU_PERIPH_SEC_IF1 */
#define SCU_PERIPH1_IF_SIMPTC0                  (SCU_PERIPH_SEC_IF1_SIMPTC0IF)            /*!< SIMPTC0 SCU Illegal Access Interrupt flag  */
#define SCU_PERIPH1_IF_SIMPTC1                  (SCU_PERIPH_SEC_IF1_SIMPTC1IF)            /*!< SIMPTC1 SCU Illegal Access Interrupt flag  */
#define SCU_PERIPH1_IF_SIMPTC2                  (SCU_PERIPH_SEC_IF1_SIMPTC2IF)            /*!< SIMPTC2 SCU Illegal Access Interrupt flag  */
#define SCU_PERIPH1_IF_SIMPTC3                  (SCU_PERIPH_SEC_IF1_SIMPTC3IF)            /*!< SIMPTC3 SCU Illegal Access Interrupt flag  */
#define SCU_PERIPH1_IF_ANA                      (SCU_PERIPH_SEC_IF1_RS_VDDIF)             /*!< ANA SCU Illegal Access Interrupt  flag     */
#define SCU_PERIPH1_IF_LSRX                     (SCU_PERIPH_SEC_IF1_LSRXIF)               /*!< LSRX SCU Illegal Access Interrupt flag     */
#define SCU_PERIPH1_IF_KBI0                     (SCU_PERIPH_SEC_IF1_KBI0IF)               /*!< KBI0 SCU Illegal Access Interrupt flag     */
#define SCU_PERIPH1_IF_KBI1                     (SCU_PERIPH_SEC_IF1_KBI1IF)               /*!< KBI1 SCU Illegal Access Interrupt flag     */
#define SCU_PERIPH1_IF_LVDCMP                   (SCU_PERIPH_SEC_IF1_LVD_CMPIF)            /*!< LVDCMP SCU Illegal Access Interrupt flag   */
#define SCU_PERIPH1_IF_CRC0                     (SCU_PERIPH_SEC_IF1_CRC0IF)               /*!< CRC0 SCU Illegal Access Interrupt flag     */
/* SCU_PERIPH_SEC_IF2 */
#define SCU_PERIPH2_IF_QSPI0                    (SCU_PERIPH_SEC_IF2_QSPI0IF)              /*!< QSPI0 SCU Illegal Access Interrupt flag    */
#define SCU_PERIPH2_IF_QSPI1                    (SCU_PERIPH_SEC_IF2_QSPI1IF)              /*!< QSPI1 SCU Illegal Access Interrupt flag    */
#define SCU_PERIPH2_IF_SEA                      (SCU_PERIPH_SEC_IF2_SEAIF)                /*!< SEA SCU Illegal Access Interrupt flag      */
#define SCU_PERIPH2_IF_AES                      (SCU_PERIPH_SEC_IF2_AESIF)                /*!< AES SCU Illegal Access Interrupt flag      */
/**
 * @}
 */

/**
 * @}
 */
/* Exported macros ----------------------------------------------------------*/
/** @defgroup SCU_Exported_Macros SCU Exported Macros
 * @{
 */

/** @defgroup SCU_WKEY Unlock and Lock
 *  @brief    Unlock or Lock the SCU_WKEY, and get status.
 *  @note     After reset, the SCU registers (used for registers write access)
 *            are Locked and the application software has to Unlock this WKEY register before
 *            write them.
 * @{
 */
#define __HAL_SCU_WKEY_UNLOCK()           (SCU->WKEY = 0xA5387492U)

#define __HAL_SCU_WKEY_LOCK()             (SCU->WKEY = 0x00000000U)

#define __HAL_SCU_WKEY_IS_UNLOCKED()      (SCU->WKEY == 0xA5387492U)
#define __HAL_SCU_WKEY_IS_LOCKD()         (SCU->WKEY != 0xA5387492U)
/**
 * @}
 */

/** @defgroup SCU_PERIPH_Secure_Nonsecure  Peripheral Security Attribute Configuration
 *  @brief  Set the Peripheral to Secure or Nonsecure.
 *  @note   Only can Configure in core0
 * @{
 */
/* SCU_PERIPH_SEC_CFG0 */
#define __HAL_SCU_PERIPH0_SET_SECURE_ALL()   (SCU->PERIPH_SEC_CFG0 = 0x3F77FFFFU)
#define __HAL_SCU_UART0_SET_SECURE()         (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_UART0SEC)
#define __HAL_SCU_UART1_SET_SECURE()         (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_UART1SEC)
#define __HAL_SCU_UART2_SET_SECURE()         (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_UART2SEC)
#define __HAL_SCU_UART3_SET_SECURE()         (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_UART3SEC)
#define __HAL_SCU_UART4_SET_SECURE()         (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_UART4SEC)
#define __HAL_SCU_UART5_SET_SECURE()         (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_UART5SEC)
#define __HAL_SCU_UART6_SET_SECURE()         (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_UART6SEC)
#define __HAL_SCU_UART7_SET_SECURE()         (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_UART7SEC)
#define __HAL_SCU_LPUART_SET_SECURE()        (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_LPUARTSEC)
#define __HAL_SCU_USART0_SET_SECURE()        (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_USART0SEC)
#define __HAL_SCU_USART1_SET_SECURE()        (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_USART1SEC)
#define __HAL_SCU_ISO78160_SET_SECURE()      (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_ISO78160SEC)
#define __HAL_SCU_ISO78161_SET_SECURE()      (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_ISO78161SEC)
#define __HAL_SCU_SPI0_SET_SECURE()          (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_SPI0SEC)
#define __HAL_SCU_SPI1_SET_SECURE()          (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_SPI1SEC)
#define __HAL_SCU_SPI2_SET_SECURE()          (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_SPI2SEC)
#define __HAL_SCU_SPI3_SET_SECURE()          (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_SPI3SEC)
#define __HAL_SCU_I2C0_SET_SECURE()          (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_I2C0SEC)
#define __HAL_SCU_I2C1_SET_SECURE()          (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_I2C1SEC)
#define __HAL_SCU_IOCNT_SET_SECURE()         (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_IOCNTSEC)
#define __HAL_SCU_LCD_SET_SECURE()           (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_LCDSEC)
#define __HAL_SCU_DSP_SET_SECURE()           (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_DSPSEC)
#define __HAL_SCU_M2M_SET_SECURE()           (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_M2MSEC)
#define __HAL_SCU_D2F_SET_SECURE()           (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_D2FSEC)
#define __HAL_SCU_ADC0_SET_SECURE()          (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_ADC0SEC)
#define __HAL_SCU_DAC_SET_SECURE()           (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_DACSEC)
#define __HAL_SCU_TC0_SET_SECURE()           (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_TC0SEC)
#define __HAL_SCU_TC1_SET_SECURE()           (SCU->PERIPH_SEC_CFG0 |= SCU_PERIPH_SEC_CFG0_TC1SEC)
/* SCU_PERIPH_SEC_CFG1 */
#define __HAL_SCU_PERIPH1_SET_SECURE_ALL()   (SCU->PERIPH_SEC_CFG1 = 0x7DFU)
#define __HAL_SCU_SIMPTC0_SET_SECURE()       (SCU->PERIPH_SEC_CFG1 |= SCU_PERIPH_SEC_CFG1_SIMPTC0SEC)
#define __HAL_SCU_SIMPTC1_SET_SECURE()       (SCU->PERIPH_SEC_CFG1 |= SCU_PERIPH_SEC_CFG1_SIMPTC1SEC)
#define __HAL_SCU_SIMPTC2_SET_SECURE()       (SCU->PERIPH_SEC_CFG1 |= SCU_PERIPH_SEC_CFG1_SIMPTC2SEC)
#define __HAL_SCU_SIMPTC3_SET_SECURE()       (SCU->PERIPH_SEC_CFG1 |= SCU_PERIPH_SEC_CFG1_SIMPTC3SEC)
#define __HAL_SCU_ANA_SET_SECURE()           (SCU->PERIPH_SEC_CFG1 |= SCU_PERIPH_SEC_CFG1_RS_VDDSEC)
#define __HAL_SCU_LSRX_SET_SECURE()          (SCU->PERIPH_SEC_CFG1 |= SCU_PERIPH_SEC_CFG1_LSRXSEC)
#define __HAL_SCU_KBI0_SET_SECURE()          (SCU->PERIPH_SEC_CFG1 |= SCU_PERIPH_SEC_CFG1_KBI0SEC)
#define __HAL_SCU_KBI1_SET_SECURE()          (SCU->PERIPH_SEC_CFG1 |= SCU_PERIPH_SEC_CFG1_KBI1SEC)
#define __HAL_SCU_LVDCMP_SET_SECURE()        (SCU->PERIPH_SEC_CFG1 |= SCU_PERIPH_SEC_CFG1_LVD_CMPSEC)
#define __HAL_SCU_CRC0_SET_SECURE()          (SCU->PERIPH_SEC_CFG1 |= SCU_PERIPH_SEC_CFG1_CRC0SEC)
/* SCU_PERIPH_SEC_CFG2 */
#define __HAL_SCU_PERIPH2_SET_SECURE_ALL()   (SCU->PERIPH_SEC_CFG2 = 0x107U)
#define __HAL_SCU_QSPI0_SET_SECURE()         (SCU->PERIPH_SEC_CFG2 |= SCU_PERIPH_SEC_CFG2_QSPI0SEC)
#define __HAL_SCU_QSPI1_SET_SECURE()         (SCU->PERIPH_SEC_CFG2 |= SCU_PERIPH_SEC_CFG2_QSPI1SEC)
#define __HAL_SCU_SEA_SET_SECURE()           (SCU->PERIPH_SEC_CFG2 |= SCU_PERIPH_SEC_CFG2_SEASEC)
#define __HAL_SCU_AES_SET_SECURE()           (SCU->PERIPH_SEC_CFG2 |= SCU_PERIPH_SEC_CFG2_AESSEC)


/* SCU_PERIPH_SEC_CFG0 */
#define __HAL_SCU_PERIPH0_SET_NONSECURE_ALL()   (SCU->PERIPH_SEC_CFG0 = 0x00000000U)
#define __HAL_SCU_UART0_SET_NONSECURE()         (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_UART0SEC))
#define __HAL_SCU_UART1_SET_NONSECURE()         (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_UART1SEC))
#define __HAL_SCU_UART2_SET_NONSECURE()         (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_UART2SEC))
#define __HAL_SCU_UART3_SET_NONSECURE()         (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_UART3SEC))
#define __HAL_SCU_UART4_SET_NONSECURE()         (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_UART4SEC))
#define __HAL_SCU_UART5_SET_NONSECURE()         (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_UART5SEC))
#define __HAL_SCU_UART6_SET_NONSECURE()         (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_UART6SEC))
#define __HAL_SCU_UART7_SET_NONSECURE()         (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_UART7SEC))
#define __HAL_SCU_LPUART_SET_NONSECURE()        (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_LPUARTSEC))
#define __HAL_SCU_USART0_SET_NONSECURE()        (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_USART0SEC))
#define __HAL_SCU_USART1_SET_NONSECURE()        (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_USART1SEC))
#define __HAL_SCU_ISO78160_SET_NONSECURE()      (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_ISO78160SEC))
#define __HAL_SCU_ISO78161_SET_NONSECURE()      (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_ISO78161SEC))
#define __HAL_SCU_SPI0_SET_NONSECURE()          (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_SPI0SEC))
#define __HAL_SCU_SPI1_SET_NONSECURE()          (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_SPI1SEC))
#define __HAL_SCU_SPI2_SET_NONSECURE()          (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_SPI2SEC))
#define __HAL_SCU_SPI3_SET_NONSECURE()          (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_SPI3SEC))
#define __HAL_SCU_I2C0_SET_NONSECURE()          (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_I2C0SEC))
#define __HAL_SCU_I2C1_SET_NONSECURE()          (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_I2C1SEC))
#define __HAL_SCU_IOCNT_SET_NONSECURE()         (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_IOCNTSEC))
#define __HAL_SCU_LCD_SET_NONSECURE()           (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_LCDSEC))
#define __HAL_SCU_DSP_SET_NONSECURE()           (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_DSPSEC))
#define __HAL_SCU_M2M_SET_NONSECURE()           (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_M2MSEC))
#define __HAL_SCU_D2F_SET_NONSECURE()           (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_D2FSEC))
#define __HAL_SCU_ADC0_SET_NONSECURE()          (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_ADC0SEC))
#define __HAL_SCU_DAC_SET_NONSECURE()           (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_DACSEC))
#define __HAL_SCU_TC0_SET_NONSECURE()           (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_TC0SEC))
#define __HAL_SCU_TC1_SET_NONSECURE()           (SCU->PERIPH_SEC_CFG0 &= ~(SCU_PERIPH_SEC_CFG0_TC1SEC))
/* SCU_PERIPH_SEC_CFG1 */
#define __HAL_SCU_PERIPH1_SET_NONSECURE_ALL()   (SCU->PERIPH_SEC_CFG1 = 0x000U)
#define __HAL_SCU_SIMPTC0_SET_NONSECURE()       (SCU->PERIPH_SEC_CFG1 &= ~(SCU_PERIPH_SEC_CFG1_SIMPTC0SEC))
#define __HAL_SCU_SIMPTC1_SET_NONSECURE()       (SCU->PERIPH_SEC_CFG1 &= ~(SCU_PERIPH_SEC_CFG1_SIMPTC1SEC))
#define __HAL_SCU_SIMPTC2_SET_NONSECURE()       (SCU->PERIPH_SEC_CFG1 &= ~(SCU_PERIPH_SEC_CFG1_SIMPTC2SEC))
#define __HAL_SCU_SIMPTC3_SET_NONSECURE()       (SCU->PERIPH_SEC_CFG1 &= ~(SCU_PERIPH_SEC_CFG1_SIMPTC3SEC))
#define __HAL_SCU_ANA_SET_NONSECURE()           (SCU->PERIPH_SEC_CFG1 &= ~(SCU_PERIPH_SEC_CFG1_RS_VDDSEC))
#define __HAL_SCU_LSRX_SET_NONSECURE()          (SCU->PERIPH_SEC_CFG1 &= ~(SCU_PERIPH_SEC_CFG1_LSRXSEC))
#define __HAL_SCU_KBI0_SET_NONSECURE()          (SCU->PERIPH_SEC_CFG1 &= ~(SCU_PERIPH_SEC_CFG1_KBI0SEC))
#define __HAL_SCU_KBI1_SET_NONSECURE()          (SCU->PERIPH_SEC_CFG1 &= ~(SCU_PERIPH_SEC_CFG1_KBI1SEC))
#define __HAL_SCU_LVDCMP_SET_NONSECURE()        (SCU->PERIPH_SEC_CFG1 &= ~(SCU_PERIPH_SEC_CFG1_LVD_CMPSEC))
#define __HAL_SCU_CRC0_SET_NONSECURE()          (SCU->PERIPH_SEC_CFG1 &= ~(SCU_PERIPH_SEC_CFG1_CRC0SEC))
/* SCU_PERIPH_SEC_CFG2 */
#define __HAL_SCU_PERIPH2_SET_NONSECURE_ALL()   (SCU->PERIPH_SEC_CFG2 = 0x0000U)
#define __HAL_SCU_QSPI0_SET_NONSECURE()         (SCU->PERIPH_SEC_CFG2 &= ~(SCU_PERIPH_SEC_CFG2_QSPI0SEC))
#define __HAL_SCU_QSPI1_SET_NONSECURE()         (SCU->PERIPH_SEC_CFG2 &= ~(SCU_PERIPH_SEC_CFG2_QSPI1SEC))
#define __HAL_SCU_SEA_SET_NONSECURE()           (SCU->PERIPH_SEC_CFG2 &= ~(SCU_PERIPH_SEC_CFG2_SEASEC))
#define __HAL_SCU_AES_SET_NONSECURE()           (SCU->PERIPH_SEC_CFG2 &= ~(SCU_PERIPH_SEC_CFG2_AESSEC))
/**
 * @}
 */

/** @defgroup SCU_PERIPH_Secure_Nonsecure_Status Peripheral Security Attribute Configuration Status
 *  @brief  Get the Secure or Nonsecure status of the SCU peripheral.
 *  @note   can read by core0 and core1
 * @{
 */
/* SCU_PERIPH_SEC_CFG0 */
#define __HAL_SCU_UART0_IS_SECURED()          ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART0SEC)) != RESET)
#define __HAL_SCU_UART0_IS_NONSECURED()       ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART0SEC)) == RESET)
#define __HAL_SCU_UART1_IS_SECURED()          ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART1SEC)) != RESET)
#define __HAL_SCU_UART1_IS_NONSECURED()       ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART1SEC)) == RESET)
#define __HAL_SCU_UART2_IS_SECURED()          ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART2SEC)) != RESET)
#define __HAL_SCU_UART2_IS_NONSECURED()       ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART2SEC)) == RESET)
#define __HAL_SCU_UART3_IS_SECURED()          ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART3SEC)) != RESET)
#define __HAL_SCU_UART3_IS_NONSECURED()       ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART3SEC)) == RESET)
#define __HAL_SCU_UART4_IS_SECURED()          ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART4SEC)) != RESET)
#define __HAL_SCU_UART4_IS_NONSECURED()       ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART4SEC)) == RESET)
#define __HAL_SCU_UART5_IS_SECURED()          ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART5SEC)) != RESET)
#define __HAL_SCU_UART5_IS_NONSECURED()       ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART5SEC)) == RESET)
#define __HAL_SCU_UART6_IS_SECURED()          ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART6SEC)) != RESET)
#define __HAL_SCU_UART6_IS_NONSECURED()       ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART6SEC)) == RESET)
#define __HAL_SCU_UART7_IS_SECURED()          ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART7SEC)) != RESET)
#define __HAL_SCU_UART7_IS_NONSECURED()       ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_UART7SEC)) == RESET)
#define __HAL_SCU_LPUART_IS_SECURED()         ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_LPUARTSEC)) != RESET)
#define __HAL_SCU_LPUART_IS_NONSECURED()      ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_LPUARTSEC)) == RESET)
#define __HAL_SCU_USART0_IS_SECURED()         ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_USART0SEC)) != RESET)
#define __HAL_SCU_USART0_IS_NONSECURED()      ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_USART0SEC)) == RESET)
#define __HAL_SCU_USART1_IS_SECURED()         ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_USART1SEC)) != RESET)
#define __HAL_SCU_USART1_IS_NONSECURED()      ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_USART1SEC)) == RESET)
#define __HAL_SCU_ISO78160_IS_SECURED()       ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_ISO78160SEC)) != RESET)
#define __HAL_SCU_ISO78160_IS_NONSECURED()    ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_ISO78160SEC)) == RESET)
#define __HAL_SCU_ISO78161_IS_SECURED()       ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_ISO78161SEC)) != RESET)
#define __HAL_SCU_ISO78161_IS_NONSECURED()    ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_ISO78161SEC)) == RESET)
#define __HAL_SCU_SPI0_IS_SECURED()           ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_SPI0SEC)) != RESET)
#define __HAL_SCU_SPI0_IS_NONSECURED()        ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_SPI0SEC)) == RESET)
#define __HAL_SCU_SPI1_IS_SECURED()           ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_SPI1SEC)) != RESET)
#define __HAL_SCU_SPI1_IS_NONSECURED()        ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_SPI1SEC)) == RESET)
#define __HAL_SCU_SPI2_IS_SECURED()           ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_SPI2SEC)) != RESET)
#define __HAL_SCU_SPI2_IS_NONSECURED()        ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_SPI2SEC)) == RESET)
#define __HAL_SCU_SPI3_IS_SECURED()           ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_SPI3SEC)) != RESET)
#define __HAL_SCU_SPI3_IS_NONSECURED()        ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_SPI3SEC)) == RESET)
#define __HAL_SCU_I2C0_IS_SECURED()           ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_I2C0SEC)) != RESET)
#define __HAL_SCU_I2C0_IS_NONSECURED()        ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_I2C0SEC)) == RESET)
#define __HAL_SCU_I2C1_IS_SECURED()           ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_I2C1SEC)) != RESET)
#define __HAL_SCU_I2C1_IS_NONSECURED()        ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_I2C1SEC)) == RESET)
#define __HAL_SCU_IOCNT_IS_SECURED()          ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_IOCNTSEC)) != RESET)
#define __HAL_SCU_IOCNT_IS_NONSECURED()       ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_IOCNTSEC)) == RESET)
#define __HAL_SCU_LCD_IS_SECURED()            ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_LCDSEC)) != RESET)
#define __HAL_SCU_LCD_IS_NONSECURED()         ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_LCDSEC)) == RESET)
#define __HAL_SCU_DSP_IS_SECURED()            ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_DSPSEC)) != RESET)
#define __HAL_SCU_DSP_IS_NONSECURED()         ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_DSPSEC)) == RESET)
#define __HAL_SCU_M2M_IS_SECURED()            ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_M2MSEC)) != RESET)
#define __HAL_SCU_M2M_IS_NONSECURED()         ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_M2MSEC)) == RESET)
#define __HAL_SCU_ADC0_IS_SECURED()           ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_ADC0SEC)) != RESET)
#define __HAL_SCU_ADC0_IS_NONSECURED()        ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_ADC0SEC)) == RESET)
#define __HAL_SCU_DAC_IS_SECURED()            ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_DACSEC)) != RESET)
#define __HAL_SCU_DAC_IS_NONSECURED()         ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_DACSEC)) == RESET)
#define __HAL_SCU_TC0_IS_SECURED()            ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_TC0SEC)) != RESET)
#define __HAL_SCU_TC0_IS_NONSECURED()         ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_TC0SEC)) == RESET)
#define __HAL_SCU_TC1_IS_SECURED()            ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_TC1SEC)) != RESET)
#define __HAL_SCU_TC1_IS_NONSECURED()         ((SCU->PERIPH_SEC_CFG0 & (SCU_PERIPH_SEC_CFG0_TC1SEC)) == RESET)
/* SCU_PERIPH_SEC_CFG1 */
#define __HAL_SCU_SIMPTC0_IS_SECURED()        ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_SIMPTC0SEC)) != RESET)
#define __HAL_SCU_SIMPTC0_IS_NONSECURED()     ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_SIMPTC0SEC)) == RESET)
#define __HAL_SCU_SIMPTC1_IS_SECURED()        ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_SIMPTC1SEC)) != RESET)
#define __HAL_SCU_SIMPTC1_IS_NONSECURED()     ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_SIMPTC1SEC)) == RESET)
#define __HAL_SCU_SIMPTC2_IS_SECURED()        ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_SIMPTC2SEC)) != RESET)
#define __HAL_SCU_SIMPTC2_IS_NONSECURED()     ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_SIMPTC2SEC)) == RESET)
#define __HAL_SCU_SIMPTC3_IS_SECURED()        ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_SIMPTC3SEC)) != RESET)
#define __HAL_SCU_SIMPTC3_IS_NONSECURED()     ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_SIMPTC3SEC)) == RESET)
#define __HAL_SCU_ANA_IS_SECURED()            ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_RS_VDDSEC)) != RESET)
#define __HAL_SCU_ANA_IS_NONSECURED()         ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_RS_VDDSEC)) == RESET)
#define __HAL_SCU_LSRX_IS_SECURED()           ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_LSRXSEC)) != RESET)
#define __HAL_SCU_LSRX_IS_NONSECURED()        ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_LSRXSEC)) == RESET)
#define __HAL_SCU_KBI0_IS_SECURED()           ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_KBI0SEC)) != RESET)
#define __HAL_SCU_KBI0_IS_NONSECURED()        ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_KBI0SEC)) == RESET)
#define __HAL_SCU_KBI1_IS_SECURED()           ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_KBI1SEC)) != RESET)
#define __HAL_SCU_KBI1_IS_NONSECURED()        ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_KBI1SEC)) == RESET)
#define __HAL_SCU_LVDCMP_IS_SECURED()         ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_LVD_CMPSEC)) != RESET)
#define __HAL_SCU_LVDCMP_IS_NONSECURED()      ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_LVD_CMPSEC)) == RESET)
#define __HAL_SCU_CRC0_IS_SECURED()           ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_CRC0SEC)) != RESET)
#define __HAL_SCU_CRC0_IS_NONSECURED()        ((SCU->PERIPH_SEC_CFG1 & (SCU_PERIPH_SEC_CFG1_CRC0SEC)) == RESET)
/* SCU_PERIPH_SEC_CFG2 */
#define __HAL_SCU_QSPI0_IS_SECURED()          ((SCU->PERIPH_SEC_CFG2 & (SCU_PERIPH_SEC_CFG2_QSPI0SEC)) != RESET)
#define __HAL_SCU_QSPI0_IS_NONSECURED()       ((SCU->PERIPH_SEC_CFG2 & (SCU_PERIPH_SEC_CFG2_QSPI0SEC)) == RESET)
#define __HAL_SCU_QSPI1_IS_SECURED()          ((SCU->PERIPH_SEC_CFG2 & (SCU_PERIPH_SEC_CFG2_QSPI1SEC)) != RESET)
#define __HAL_SCU_QSPI1_IS_NONSECURED()       ((SCU->PERIPH_SEC_CFG2 & (SCU_PERIPH_SEC_CFG2_QSPI1SEC)) == RESET)
#define __HAL_SCU_SEA_IS_SECURED()            ((SCU->PERIPH_SEC_CFG2 & (SCU_PERIPH_SEC_CFG2_SEASEC)) != RESET)
#define __HAL_SCU_SEA_IS_NONSECURED()         ((SCU->PERIPH_SEC_CFG2 & (SCU_PERIPH_SEC_CFG2_SEASEC)) == RESET)
#define __HAL_SCU_AES_IS_SECURED()            ((SCU->PERIPH_SEC_CFG2 & (SCU_PERIPH_SEC_CFG2_AESSEC)) != RESET)
#define __HAL_SCU_AES_IS_NONSECURED()         ((SCU->PERIPH_SEC_CFG2 & (SCU_PERIPH_SEC_CFG2_AESSEC)) == RESET)
/**
 * @}
 */

/** @defgroup SCU_GPIO_SECURITY_CFG_NONSECURITY_CFG  GPIO Security Attribute Configuration
 *  @brief  Set the GPIO to Secure or Nonsecure.
 *  @note   Only can Configure in core0
 * @{
 */
/** @brief GPIO Security Attribute Configuration.
 *  @param  __GPIO_PIN__ This parameter can be a combination of the following values:
 *         @arg @ref HAL_SCU_GPIO_PIN_00 (*)
 *         @arg @ref HAL_SCU_GPIO_PIN_01 (*)
 *         @arg @ref HAL_SCU_GPIO_PIN_02
 *         @arg @ref HAL_SCU_GPIO_PIN_03
 *         @arg @ref HAL_SCU_GPIO_PIN_04
 *         @arg @ref HAL_SCU_GPIO_PIN_05
 *         @arg @ref HAL_SCU_GPIO_PIN_06
 *         @arg @ref HAL_SCU_GPIO_PIN_07
 *         @arg @ref HAL_SCU_GPIO_PIN_08
 *         @arg @ref HAL_SCU_GPIO_PIN_09
 *         @arg @ref HAL_SCU_GPIO_PIN_10
 *         @arg @ref HAL_SCU_GPIO_PIN_11
 *         @arg @ref HAL_SCU_GPIO_PIN_12
 *         @arg @ref HAL_SCU_GPIO_PIN_13
 *         @arg @ref HAL_SCU_GPIO_PIN_14
 *         @arg @ref HAL_SCU_GPIO_PIN_15
 *         @arg @ref HAL_SCU_GPIO_PIN_All
 *
 *         (*) GPIO_H only have PIN_00 and PIN_01.
 */
/* SCU_GPIOA_SEC_CFG */
#define __HAL_SCU_GPIOA_SET_SECURE_ALL()                 (SCU->GPIOA_SEC_CFG = 0xFFFFU)
#define __HAL_SCU_GPIOA_SET_SECURE(__GPIO_PIN__)         (SCU->GPIOA_SEC_CFG |= (__GPIO_PIN__))
/* SCU_GPIOB_SEC_CFG */
#define __HAL_SCU_GPIOB_SET_SECURE_ALL()                 (SCU->GPIOB_SEC_CFG = 0xFFFFU)
#define __HAL_SCU_GPIOB_SET_SECURE(__GPIO_PIN__)         (SCU->GPIOB_SEC_CFG |= (__GPIO_PIN__))
/* SCU_GPIOC_SEC_CFG */
#define __HAL_SCU_GPIOC_SET_SECURE_ALL()                 (SCU->GPIOC_SEC_CFG = 0xFFFFU)
#define __HAL_SCU_GPIOC_SET_SECURE(__GPIO_PIN__)         (SCU->GPIOC_SEC_CFG |= (__GPIO_PIN__))
/* SCU_GPIOD_SEC_CFG */
#define __HAL_SCU_GPIOD_SET_SECURE_ALL()                 (SCU->GPIOD_SEC_CFG = 0xFFFFU)
#define __HAL_SCU_GPIOD_SET_SECURE(__GPIO_PIN__)         (SCU->GPIOD_SEC_CFG |= (__GPIO_PIN__))
/* SCU_GPIOE_SEC_CFG */
#define __HAL_SCU_GPIOE_SET_SECURE_ALL()                 (SCU->GPIOE_SEC_CFG = 0xFFFFU)
#define __HAL_SCU_GPIOE_SET_SECURE(__GPIO_PIN__)         (SCU->GPIOE_SEC_CFG |= (__GPIO_PIN__))
/* SCU_GPIOF_SEC_CFG */
#define __HAL_SCU_GPIOF_SET_SECURE_ALL()                 (SCU->GPIOF_SEC_CFG = 0xFFFFU)
#define __HAL_SCU_GPIOF_SET_SECURE(__GPIO_PIN__)         (SCU->GPIOF_SEC_CFG |= (__GPIO_PIN__))
/* SCU_GPIOG_SEC_CFG */
#define __HAL_SCU_GPIOG_SET_SECURE_ALL()                 (SCU->GPIOG_SEC_CFG = 0xFFFFU)
#define __HAL_SCU_GPIOG_SET_SECURE(__GPIO_PIN__)         (SCU->GPIOG_SEC_CFG |= (__GPIO_PIN__))
/* SCU_GPIOH_SEC_CFG */
#define __HAL_SCU_GPIOH_SET_SECURE_ALL()                 (SCU->GPIOH_SEC_CFG = 0x3U)
#define __HAL_SCU_GPIOH_SET_SECURE(__GPIO_PIN__)         (SCU->GPIOH_SEC_CFG |= (__GPIO_PIN__))

/* SCU_GPIOA_SEC_CFG */
#define __HAL_SCU_GPIOA_SET_NONSECURE_ALL()              (SCU->GPIOA_SEC_CFG = 0x0000U)
#define __HAL_SCU_GPIOA_SET_NONSECURE(__GPIO_PIN__)      (SCU->GPIOA_SEC_CFG &= ~(__GPIO_PIN__))
/* SCU_GPIOB_SEC_CFG */
#define __HAL_SCU_GPIOB_SET_NONSECURE_ALL()              (SCU->GPIOB_SEC_CFG = 0x0000U)
#define __HAL_SCU_GPIOB_SET_NONSECURE(__GPIO_PIN__)      (SCU->GPIOB_SEC_CFG &= ~(__GPIO_PIN__))
/* SCU_GPIOC_SEC_CFG */
#define __HAL_SCU_GPIOC_SET_NONSECURE_ALL()              (SCU->GPIOC_SEC_CFG = 0x0000U)
#define __HAL_SCU_GPIOC_SET_NONSECURE(__GPIO_PIN__)      (SCU->GPIOC_SEC_CFG &= ~(__GPIO_PIN__))
/* SCU_GPIOD_SEC_CFG */
#define __HAL_SCU_GPIOD_SET_NONSECURE_ALL()              (SCU->GPIOD_SEC_CFG = 0x0000U)
#define __HAL_SCU_GPIOD_SET_NONSECURE(__GPIO_PIN__)      (SCU->GPIOD_SEC_CFG &= ~(__GPIO_PIN__))
/* SCU_GPIOE_SEC_CFG */
#define __HAL_SCU_GPIOE_SET_NONSECURE_ALL()              (SCU->GPIOE_SEC_CFG = 0x0000U)
#define __HAL_SCU_GPIOE_SET_NONSECURE(__GPIO_PIN__)      (SCU->GPIOE_SEC_CFG &= ~(__GPIO_PIN__))
/* SCU_GPIOF_SEC_CFG */
#define __HAL_SCU_GPIOF_SET_NONSECURE_ALL()              (SCU->GPIOF_SEC_CFG = 0x0000U)
#define __HAL_SCU_GPIOF_SET_NONSECURE(__GPIO_PIN__)      (SCU->GPIOF_SEC_CFG &= ~(__GPIO_PIN__))
/* SCU_GPIOG_SEC_CFG */
#define __HAL_SCU_GPIOG_SET_NONSECURE_ALL()              (SCU->GPIOG_SEC_CFG = 0x0000U)
#define __HAL_SCU_GPIOG_SET_NONSECURE(__GPIO_PIN__)      (SCU->GPIOG_SEC_CFG &= ~(__GPIO_PIN__))
/* SCU_GPIOH_SEC_CFG */
#define __HAL_SCU_GPIOH_SET_NONSECURE_ALL()              (SCU->GPIOH_SEC_CFG = 0x0U)
#define __HAL_SCU_GPIOH_SET_NONSECURE(__GPIO_PIN__)      (SCU->GPIOH_SEC_CFG &= ~(__GPIO_PIN__))
/**
 * @}
 */

/** @defgroup SCU_GPIO_SECURITY_CFG_NONSECURITY_CFG_Status  GPIO Security Attribute Configuration Status
 *  @brief  Get the GPIO to Secure or Nonsecure Status.
 *  @note   can read by core0 and core1
 * @{
 */
/** @brief GPIO Security Attribute Configuration Status.
 *  @param  __GPIO_PIN__ This parameter can be one of the following values:
 *         @arg @ref HAL_SCU_GPIO_PIN_00 (*)
 *         @arg @ref HAL_SCU_GPIO_PIN_01 (*)
 *         @arg @ref HAL_SCU_GPIO_PIN_02
 *         @arg @ref HAL_SCU_GPIO_PIN_03
 *         @arg @ref HAL_SCU_GPIO_PIN_04
 *         @arg @ref HAL_SCU_GPIO_PIN_05
 *         @arg @ref HAL_SCU_GPIO_PIN_06
 *         @arg @ref HAL_SCU_GPIO_PIN_07
 *         @arg @ref HAL_SCU_GPIO_PIN_08
 *         @arg @ref HAL_SCU_GPIO_PIN_09
 *         @arg @ref HAL_SCU_GPIO_PIN_10
 *         @arg @ref HAL_SCU_GPIO_PIN_11
 *         @arg @ref HAL_SCU_GPIO_PIN_12
 *         @arg @ref HAL_SCU_GPIO_PIN_13
 *         @arg @ref HAL_SCU_GPIO_PIN_14
 *         @arg @ref HAL_SCU_GPIO_PIN_15
 *
 *         (*) GPIO_H only have PIN_00 and PIN_01.
 */
#define __HAL_SCU_GPIOA_IS_SECURED(__GPIO_PIN__)          ((SCU->GPIOA_SEC_CFG & (__GPIO_PIN__)) != RESET)
#define __HAL_SCU_GPIOA_IS_NONSECURED(__GPIO_PIN__)       ((SCU->GPIOA_SEC_CFG & (__GPIO_PIN__)) == RESET)

#define __HAL_SCU_GPIOB_IS_SECURED(__GPIO_PIN__)          ((SCU->GPIOB_SEC_CFG & (__GPIO_PIN__)) != RESET)
#define __HAL_SCU_GPIOB_IS_NONSECURED(__GPIO_PIN__)       ((SCU->GPIOB_SEC_CFG & (__GPIO_PIN__)) == RESET)

#define __HAL_SCU_GPIOC_IS_SECURED(__GPIO_PIN__)          ((SCU->GPIOC_SEC_CFG & (__GPIO_PIN__)) != RESET)
#define __HAL_SCU_GPIOC_IS_NONSECURED(__GPIO_PIN__)       ((SCU->GPIOC_SEC_CFG & (__GPIO_PIN__)) == RESET)

#define __HAL_SCU_GPIOD_IS_SECURED(__GPIO_PIN__)          ((SCU->GPIOD_SEC_CFG & (__GPIO_PIN__)) != RESET)
#define __HAL_SCU_GPIOD_IS_NONSECURED(__GPIO_PIN__)       ((SCU->GPIOD_SEC_CFG & (__GPIO_PIN__)) == RESET)

#define __HAL_SCU_GPIOE_IS_SECURED(__GPIO_PIN__)          ((SCU->GPIOE_SEC_CFG & (__GPIO_PIN__)) != RESET)
#define __HAL_SCU_GPIOE_IS_NONSECURED(__GPIO_PIN__)       ((SCU->GPIOE_SEC_CFG & (__GPIO_PIN__)) == RESET)

#define __HAL_SCU_GPIOF_IS_SECURED(__GPIO_PIN__)          ((SCU->GPIOF_SEC_CFG & (__GPIO_PIN__)) != RESET)
#define __HAL_SCU_GPIOF_IS_NONSECURED(__GPIO_PIN__)       ((SCU->GPIOF_SEC_CFG & (__GPIO_PIN__)) == RESET)

#define __HAL_SCU_GPIOG_IS_SECURED(__GPIO_PIN__)          ((SCU->GPIOG_SEC_CFG & (__GPIO_PIN__)) != RESET)
#define __HAL_SCU_GPIOG_IS_NONSECURED(__GPIO_PIN__)       ((SCU->GPIOG_SEC_CFG & (__GPIO_PIN__)) == RESET)

#define __HAL_SCU_GPIOH_IS_SECURED(__GPIO_PIN__)          ((SCU->GPIOH_SEC_CFG & (__GPIO_PIN__)) != RESET)
#define __HAL_SCU_GPIOH_IS_NONSECURED(__GPIO_PIN__)       ((SCU->GPIOH_SEC_CFG & (__GPIO_PIN__)) == RESET)
/**
 * @}
 */

/** @defgroup SCU_Flags_Interrupts_Management Flags Interrupts Management
 *  @brief macros to manage the specified SCU Flags and interrupts.
 * @{
 */
/** @brief Enable SCU Peripheral0 Illegal Access interrupt.
 *  @param  __INTERRUPT__ specifies the SCU Peripheral0 Illegal Access interrupt sources to be enabled.
 *          This parameter can be any combination of the following values:
 *            @arg @ref SCU_PERIPH0_IT_UART0          UART0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_UART1          UART1 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_UART2          UART2 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_UART3          UART3 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_UART4          UART4 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_UART5          UART5 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_UART6          UART6 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_UART7          UART7 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_LPUART         LPUART SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_USART0         USART0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_USART1         USART1 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_ISO78160       ISO78160 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_ISO78161       ISO78161 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_SPI0           SPI0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_SPI1           SPI1 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_SPI2           SPI2 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_SPI3           SPI3 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_I2C0           I2C0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_I2C1           I2C1 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_IOCNT          IOCNT SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_LCD            LCD SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_DSP            DSP SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_M2M            M2M SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_D2F            D2F SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_ADC0           ADC0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_DAC            DAC SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_TC0            TC0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_TC1            TC1 SCU Illegal Access Interrupt
 */
#define __HAL_SCU_PERIPH0_ENABLE_IT(__INTERRUPT__) (*(__IO uint32_t *) SCU_PERIPH_SEC_IE0_ADDRESS |= (__INTERRUPT__))

/** @brief Enable SCU Peripheral1 Illegal Access interrupt.
 *  @param  __INTERRUPT__ specifies the SCU Peripheral1 Illegal Access interrupt sources to be enabled.
 *          This parameter can be any combination of the following values:
 *            @arg @ref SCU_PERIPH1_IT_SIMPTC0          SIMPTC0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_SIMPTC1          SIMPTC1 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_SIMPTC2          SIMPTC2 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_SIMPTC3          SIMPTC3 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_ANA              ANA SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_LSRX             LSRX SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_KBI0             KBI0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_KBI1             KBI1 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_LVDCMP           LVDCMP SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_CRC0             CRC0 SCU Illegal Access Interrupt
 */
#define __HAL_SCU_PERIPH1_ENABLE_IT(__INTERRUPT__) (*(__IO uint32_t *) SCU_PERIPH_SEC_IE1_ADDRESS |= (__INTERRUPT__))

/** @brief Enable SCU Peripheral2 Illegal Access interrupt.
 *  @param  __INTERRUPT__ specifies the SCU Peripheral2 Illegal Access interrupt sources to be enabled.
 *          This parameter can be any combination of the following values:
 *            @arg @ref SCU_PERIPH2_IT_QSPI0          QSPI0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH2_IT_QSPI1          QSPI1 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH2_IT_SEA            SEA SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH2_IT_AES            AES SCU Illegal Access Interrupt
 */
#define __HAL_SCU_PERIPH2_ENABLE_IT(__INTERRUPT__) (*(__IO uint32_t *) SCU_PERIPH_SEC_IE2_ADDRESS |= (__INTERRUPT__))

/** @brief Disable SCU Peripheral0 Illegal Access interrupt.
 *  @param  __INTERRUPT__ specifies the SCU Peripheral0 Illegal Access interrupt sources to be disabled.
 *          This parameter can be any combination of the following values:
 *            @arg @ref SCU_PERIPH0_IT_UART0          UART0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_UART1          UART1 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_UART2          UART2 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_UART3          UART3 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_UART4          UART4 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_UART5          UART5 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_UART6          UART6 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_UART7          UART7 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_LPUART         LPUART SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_USART0         USART0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_USART1         USART1 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_ISO78160       ISO78160 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_ISO78161       ISO78161 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_SPI0           SPI0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_SPI1           SPI1 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_SPI2           SPI2 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_SPI3           SPI3 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_I2C0           I2C0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_I2C1           I2C1 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_IOCNT          IOCNT SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_LCD            LCD SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_DSP            DSP SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_M2M            M2M SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_D2F            D2F SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_ADC0           ADC0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_DAC            DAC SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_TC0            TC0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH0_IT_TC1            TC1 SCU Illegal Access Interrupt
 */
#define __HAL_SCU_PERIPH0_DISABLE_IT(__INTERRUPT__) (*(__IO uint32_t *) SCU_PERIPH_SEC_IE0_ADDRESS &= (uint32_t)(~(__INTERRUPT__)))

/** @brief Disable SCU Peripheral1 Illegal Access interrupt.
 *  @param  __INTERRUPT__ specifies the SCU Peripheral1 Illegal Access interrupt sources to be disabled.
 *          This parameter can be any combination of the following values:
 *            @arg @ref SCU_PERIPH1_IT_SIMPTC0          SIMPTC0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_SIMPTC1          SIMPTC1 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_SIMPTC2          SIMPTC2 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_SIMPTC3          SIMPTC3 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_ANA              ANA SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_LSRX             LSRX SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_KBI0             KBI0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_KBI1             KBI1 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_LVDCMP           LVDCMP SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH1_IT_CRC0             CRC0 SCU Illegal Access Interrupt
 */
#define __HAL_SCU_PERIPH1_DISABLE_IT(__INTERRUPT__) (*(__IO uint32_t *) SCU_PERIPH_SEC_IE1_ADDRESS &= (uint32_t)(~(__INTERRUPT__)))

/** @brief Disable SCU Peripheral2 Illegal Access interrupt.
 *  @param  __INTERRUPT__ specifies the SCU Peripheral2 Illegal Access interrupt sources to be disabled.
 *          This parameter can be any combination of the following values:
 *            @arg @ref SCU_PERIPH2_IT_QSPI0          QSPI0 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH2_IT_QSPI1          QSPI1 SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH2_IT_SEA            SEA SCU Illegal Access Interrupt
 *            @arg @ref SCU_PERIPH2_IT_AES            AES SCU Illegal Access Interrupt
 */
#define __HAL_SCU_PERIPH2_DISABLE_IT(__INTERRUPT__) (*(__IO uint32_t *) SCU_PERIPH_SEC_IE2_ADDRESS &= (uint32_t)(~(__INTERRUPT__)))

/** @brief Clear the SCU Peripheral Illegal Access interrupt pending bits.
 */
#define __HAL_SCU_PERIPH0_CLEAR_IT() (*(__IO uint32_t *) SCU_PERIPH_SEC_IF0_ADDRESS = 0x3F77FFFFU)
#define __HAL_SCU_PERIPH1_CLEAR_IT() (*(__IO uint32_t *) SCU_PERIPH_SEC_IF1_ADDRESS = 0x7DFU)
#define __HAL_SCU_PERIPH2_CLEAR_IT() (*(__IO uint32_t *) SCU_PERIPH_SEC_IF2_ADDRESS = 0x107U)

/** @brief Check the SCU's Peripheral Illegal Access interrupt has occurred or not.
 *  @retval The new state of __INTERRUPT__ (TRUE or FALSE).
 */
#define __HAL_SCU_PERIPH0_GET_IT() ((*(__IO uint32_t *) SCU_PERIPH_SEC_IF0_ADDRESS) != RESET)
#define __HAL_SCU_PERIPH1_GET_IT() ((*(__IO uint32_t *) SCU_PERIPH_SEC_IF1_ADDRESS) != RESET)
#define __HAL_SCU_PERIPH2_GET_IT() ((*(__IO uint32_t *) SCU_PERIPH_SEC_IF2_ADDRESS) != RESET)

/** @brief Clear the flags of Peripheral0 Illegal Access interrupt.
 *  @param  __FLAG__ specifies the SCU Peripheral0 Illegal Access interrupt sources to be enabled.
 *          This parameter can be any combination of the following values:
 *            @arg @ref SCU_PERIPH0_IF_UART0          UART0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_UART1          UART1 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_UART2          UART2 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_UART3          UART3 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_UART4          UART4 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_UART5          UART5 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_UART6          UART6 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_UART7          UART7 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_LPUART         LPUART SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_USART0         USART0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_USART1         USART1 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_ISO78160       ISO78160 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_ISO78161       ISO78161 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_SPI0           SPI0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_SPI1           SPI1 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_SPI2           SPI2 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_SPI3           SPI3 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_I2C0           I2C0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_I2C1           I2C1 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_IOCNT          IOCNT SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_LCD            LCD SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_DSP            DSP SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_M2M            M2M SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_D2F            D2F SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_ADC0           ADC0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_DAC            DAC SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_TC0            TC0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_TC1            TC1 SCU Illegal Access Interrupt Flag
 */
#define __HAL_SCU_PERIPH0_CLEAR_FLAG(__FLAG__) (*(__IO uint32_t *) SCU_PERIPH_SEC_IF0_ADDRESS |= (__FLAG__))

/** @brief Clear the flags of Peripheral1 Illegal Access interrupt.
 *  @param  __FLAG__ specifies the SCU Peripheral1 Illegal Access interrupt sources to be enabled.
 *          This parameter can be any combination of the following values:
 *            @arg @ref SCU_PERIPH1_IF_SIMPTC0          SIMPTC0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_SIMPTC1          SIMPTC1 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_SIMPTC2          SIMPTC2 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_SIMPTC3          SIMPTC3 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_ANA              ANA SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_LSRX             LSRX SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_KBI0             KBI0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_KBI1             KBI1 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_LVDCMP           LVDCMP SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_CRC0             CRC0 SCU Illegal Access Interrupt Flag
 */
#define __HAL_SCU_PERIPH1_CLEAR_FLAG(__FLAG__) (*(__IO uint32_t *) SCU_PERIPH_SEC_IF1_ADDRESS |= (__FLAG__))

/** @brief Clear the flags of SCU Peripheral2 Illegal Access interrupt.
 *  @param  __FLAG__ specifies the SCU Peripheral2 Illegal Access interrupt sources to be enabled.
 *          This parameter can be any combination of the following values:
 *            @arg @ref SCU_PERIPH2_IF_QSPI0          QSPI0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH2_IF_QSPI1          QSPI1 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH2_IF_SEA            SEA SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH2_IF_AES            AES SCU Illegal Access Interrupt Flag
 */
#define __HAL_SCU_PERIPH2_CLEAR_FLAG(__FLAG__) (*(__IO uint32_t *) SCU_PERIPH_SEC_IF2_ADDRESS |= (__FLAG__))

/** @brief Check SCU flag is set or not..
 *  @param  __FLAG__ specifies the SCU Peripheral0 Illegal Access interrupt sources to be disabled.
 *          This parameter can be any combination of the following values:
 *            @arg @ref SCU_PERIPH0_IF_UART0          UART0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_UART1          UART1 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_UART2          UART2 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_UART3          UART3 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_UART4          UART4 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_UART5          UART5 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_UART6          UART6 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_UART7          UART7 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_LPUART         LPUART SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_USART0         USART0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_USART1         USART1 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_ISO78160       ISO78160 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_ISO78161       ISO78161 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_SPI0           SPI0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_SPI1           SPI1 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_SPI2           SPI2 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_SPI3           SPI3 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_I2C0           I2C0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_I2C1           I2C1 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_IOCNT          IOCNT SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_LCD            LCD SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_DSP            DSP SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_M2M            M2M SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_D2F            D2F SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_ADC0           ADC0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_DAC            DAC SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_TC0            TC0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH0_IF_TC1            TC1 SCU Illegal Access Interrupt Flag
 */
#define __HAL_SCU_PERIPH0_GET_FLAG(__FLAG__) ((*(__IO uint32_t *) SCU_PERIPH_SEC_IF0_ADDRESS & (__FLAG__)) == (__FLAG__))

/** @brief Disable SCU Peripheral1 Illegal Access interrupt.
 *  @param  __FLAG__ specifies the SCU Peripheral1 Illegal Access interrupt sources to be disabled.
 *          This parameter can be any combination of the following values:
 *            @arg @ref SCU_PERIPH1_IF_SIMPTC0          SIMPTC0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_SIMPTC1          SIMPTC1 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_SIMPTC2          SIMPTC2 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_SIMPTC3          SIMPTC3 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_ANA              ANA SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_LSRX             LSRX SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_KBI0             KBI0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_KBI1             KBI1 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_LVDCMP           LVDCMP SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH1_IF_CRC0             CRC0 SCU Illegal Access Interrupt Flag
 */
#define __HAL_SCU_PERIPH1_GET_FLAG(__FLAG__) ((*(__IO uint32_t *) SCU_PERIPH_SEC_IF1_ADDRESS & (__FLAG__)) == (__FLAG__))

/** @brief Disable SCU Peripheral2 Illegal Access interrupt.
 *  @param  __FLAG__ specifies the SCU Peripheral2 Illegal Access interrupt sources to be disabled.
 *          This parameter can be any combination of the following values:
 *            @arg @ref SCU_PERIPH2_IF_QSPI0          QSPI0 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH2_IF_QSPI1          QSPI1 SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH2_IF_SEA            SEA SCU Illegal Access Interrupt Flag
 *            @arg @ref SCU_PERIPH2_IF_AES            AES SCU Illegal Access Interrupt Flag
 */
#define __HAL_SCU_PERIPH2_GET_FLAG(__FLAG__) ((*(__IO uint32_t *) SCU_PERIPH_SEC_IF2_ADDRESS & (__FLAG__)) == (__FLAG__))

/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup SCU_Exported_Functions SCU Exported Functions
 * @{
 */

/** @addtogroup SCU_Exported_Functions_Group1
 * @{
 */


/**
 * @}
 */

/** @addtogroup SCU_Exported_Functions_Group2
 * @{
 */
/* SCU Control functions ***************************************************/
HAL_StatusTypeDef HAL_SCU_SetSecureSRAMSize(HAL_SCU_SRAMSecLenTypeDef SRAMSize);
HAL_StatusTypeDef HAL_SCU_SetNonSecureSRAMSize(HAL_SCU_SRAMSecLenTypeDef SRAMSize);
uint32_t          HAL_SCU_GetSecureSRAMSize(void);
uint32_t          HAL_SCU_GetNonSecureSRAMSize(void);
/* SCU IRQHandler and Callbacks **************************************/
void              HAL_SCU_IRQHandler(void);
void              HAL_SCU_Periph0IllegalAccessCallback(void);
void              HAL_SCU_Periph1IllegalAccessCallback(void);
void              HAL_SCU_Periph2IllegalAccessCallback(void);

/**
 * @}
 */

/** @addtogroup SCU_Exported_Functions_Group3
 * @{
 */

/**
 * @}
 */

/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/** @defgroup SCU_Private_Types SCU Private Types
 * @{
 */

/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/** @defgroup SCU_Private_Variables SCU Private Variables
 * @{
 */

/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup SCU_Private_Constants SCU Private Constants
 * @{
 */
/** @defgroup SCU_Register_Offset Register offsets
 * @{
 */
#ifdef CORE0
#define SCU_BASE                     (SCU_CORE0_BASE)
#endif
#ifdef CORE1
#define SCU_BASE                     (SCU_CORE1_BASE)
#endif

#define SCU_PERIPH_SEC_IE0_OFFSET   0x0060U
#define SCU_PERIPH_SEC_IE1_OFFSET   0x0064U
#define SCU_PERIPH_SEC_IE2_OFFSET   0x0068U

#define SCU_PERIPH_SEC_IF0_OFFSET   0x006CU
#define SCU_PERIPH_SEC_IF1_OFFSET   0x0070U
#define SCU_PERIPH_SEC_IF2_OFFSET   0x0074U
/**
 * @}
 */

/* PERIPH_SEC_IE register base address */
#define SCU_PERIPH_SEC_IE0_ADDRESS           ((uint32_t)(SCU_BASE + SCU_PERIPH_SEC_IE0_OFFSET))
#define SCU_PERIPH_SEC_IE1_ADDRESS           ((uint32_t)(SCU_BASE + SCU_PERIPH_SEC_IE1_OFFSET))
#define SCU_PERIPH_SEC_IE2_ADDRESS           ((uint32_t)(SCU_BASE + SCU_PERIPH_SEC_IE2_OFFSET))

/* PERIPH_SEC_IF register base address */
#define SCU_PERIPH_SEC_IF0_ADDRESS           ((uint32_t)(SCU_BASE + SCU_PERIPH_SEC_IF0_OFFSET))
#define SCU_PERIPH_SEC_IF1_ADDRESS           ((uint32_t)(SCU_BASE + SCU_PERIPH_SEC_IF1_OFFSET))
#define SCU_PERIPH_SEC_IF2_ADDRESS           ((uint32_t)(SCU_BASE + SCU_PERIPH_SEC_IF2_OFFSET))

/**
 * @}
 */

/* Private Macros -----------------------------------------------------------*/
/** @defgroup SCU_Private_Macros SCU Private Macros
 * @{
 */

/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup SCU_Private_Functions SCU Private Functions
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

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_HAL_SCU_H */
