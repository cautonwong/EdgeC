/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_aos.h
 * @author  Renergy Software Team
 * @brief   AOS module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_AOS_H
#define _RN831x_RN861x_MCU_AOS_H

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

/* AOS */
typedef struct
{
    __IO uint32_t SEC_EN;                    /*!< offset: 0x000 info: Security peripheral linkage enable register, can only be accessed through address 0x20088000 */
    __IO uint32_t NSEC_EN;                   /*!< offset: 0x004 info: Non-security peripheral linkage enable registe, can only be accessed through address 0xA0088000 */
    __IO uint32_t RESERVED0[2];              /*!< offset: 0x008 ~ 00C */
    __IO uint32_t PE_TRGSEL0;                /*!< offset: 0x010 info: Target peripheral trigger source selection register0, for TC0 External input clock source */
    __IO uint32_t PE_TRGSEL1;                /*!< offset: 0x014 info: Target peripheral trigger source selection register1, for TC0 External input events in slave mode */
    __IO uint32_t PE_TRGSEL2;                /*!< offset: 0x018 info: Target peripheral trigger source selection register2, for TC0 Channel 0 captures external input events */
    __IO uint32_t PE_TRGSEL3;                /*!< offset: 0x01C info: Target peripheral trigger source selection register3, for TC0 Channel 1 captures external input events */
    __IO uint32_t PE_TRGSEL4;                /*!< offset: 0x020 info: Target peripheral trigger source selection register4, for TC1 External input clock source */
    __IO uint32_t PE_TRGSEL5;                /*!< offset: 0x024 info: Target peripheral trigger source selection register5, for TC1 External input events in slave mode */
    __IO uint32_t PE_TRGSEL6;                /*!< offset: 0x028 info: Target peripheral trigger source selection register6, for TC1 Channel 0 captures external input events */
    __IO uint32_t PE_TRGSEL7;                /*!< offset: 0x02C info: Target peripheral trigger source selection register7, for TC1 Channel 1 captures external input events */
    __IO uint32_t PE_TRGSEL8;                /*!< offset: 0x030 info: Target peripheral trigger source selection register8, for SIMPTC0 Hardware trigger channel 0 starts counting */
    __IO uint32_t PE_TRGSEL9;                /*!< offset: 0x034 info: Target peripheral trigger source selection register9, for SIMPTC0 Hardware trigger channel 1 starts counting */
    __IO uint32_t PE_TRGSEL10;               /*!< offset: 0x038 info: Target peripheral trigger source selection register10, for SIMPTC0 Hardware trigger channel 2 starts counting */
    __IO uint32_t PE_TRGSEL11;               /*!< offset: 0x03C info: Target peripheral trigger source selection register11, for SIMPTC0 Hardware trigger channel 3 starts counting */
    __IO uint32_t PE_TRGSEL12;               /*!< offset: 0x040 info: Target peripheral trigger source selection register12, for SIMPTC1 Hardware trigger channel 0 starts counting */
    __IO uint32_t PE_TRGSEL13;               /*!< offset: 0x044 info: Target peripheral trigger source selection register13, for SIMPTC1 Hardware trigger channel 1 starts counting */
    __IO uint32_t PE_TRGSEL14;               /*!< offset: 0x048 info: Target peripheral trigger source selection register14, for SIMPTC1 Hardware trigger channel 2 starts counting */
    __IO uint32_t PE_TRGSEL15;               /*!< offset: 0x04C info: Target peripheral trigger source selection register15, for SIMPTC1 Hardware trigger channel 3 starts counting */
    __IO uint32_t PE_TRGSEL16;               /*!< offset: 0x050 info: Target peripheral trigger source selection register16, for SIMPTC2 Hardware trigger channel 0 starts counting */
    __IO uint32_t PE_TRGSEL17;               /*!< offset: 0x054 info: Target peripheral trigger source selection register17, for SIMPTC2 Hardware trigger channel 1 starts counting */
    __IO uint32_t PE_TRGSEL18;               /*!< offset: 0x058 info: Target peripheral trigger source selection register18, for SIMPTC2 Hardware trigger channel 2 starts counting */
    __IO uint32_t PE_TRGSEL19;               /*!< offset: 0x05C info: Target peripheral trigger source selection register19, for SIMPTC2 Hardware trigger channel 3 starts counting */
    __IO uint32_t PE_TRGSEL20;               /*!< offset: 0x060 info: Target peripheral trigger source selection register20, for SIMPTC3 Hardware trigger channel 0 starts counting */
    __IO uint32_t PE_TRGSEL21;               /*!< offset: 0x064 info: Target peripheral trigger source selection register21, for SIMPTC3 Hardware trigger channel 1 starts counting */
    __IO uint32_t PE_TRGSEL22;               /*!< offset: 0x068 info: Target peripheral trigger source selection register22, for SIMPTC3 Hardware trigger channel 2 starts counting */
    __IO uint32_t PE_TRGSEL23;               /*!< offset: 0x06C info: Target peripheral trigger source selection register23, for SIMPTC3 Hardware trigger channel 3 starts counting */
    __IO uint32_t PE_TRGSEL24;               /*!< offset: 0x070 info: Target peripheral trigger source selection register24, for D2F Hardware trigger hardware trigger D2F */
    __IO uint32_t PE_TRGSEL25;               /*!< offset: 0x074 info: Target peripheral trigger source selection register25, for IOCNT Channel 0 count source */
    __IO uint32_t PE_TRGSEL26;               /*!< offset: 0x078 info: Target peripheral trigger source selection register26, for IOCNT Channel 1 count source */
    __IO uint32_t PE_TRGSEL27;               /*!< offset: 0x07C info: Target peripheral trigger source selection register27, for IOCNT Channel 2 count source */
    __IO uint32_t PE_TRGSEL28;               /*!< offset: 0x080 info: Target peripheral trigger source selection register28, for IOCNT Channel 3 count source */
    __IO uint32_t PE_TRGSEL29;               /*!< offset: 0x084 info: Target peripheral trigger source selection register29, for IOCNT Channel 4 count source */
    __IO uint32_t PE_TRGSEL30;               /*!< offset: 0x088 info: Target peripheral trigger source selection register30, for RC_CAL Hardware trigger correction */
    __IO uint32_t PE_TRGSEL31;               /*!< offset: 0x08C info: Target peripheral trigger source selection register31, for CAN Timestamp input source */
    __IO uint32_t PE_TRGSEL32;               /*!< offset: 0x090 info: Target peripheral trigger source selection register32, for ADC0 Hardware trigger sequence A starts conversion */
    __IO uint32_t PE_TRGSEL33;               /*!< offset: 0x094 info: Target peripheral trigger source selection register33, for ADC0 Hardware trigger sequence B starts conversion */
    __IO uint32_t PE_TRGSEL34;               /*!< offset: 0x098 info: Target peripheral trigger source selection register34, for DAC Hardware trigger DAC starts conversion */
} AOS_TypeDef;

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
#define AOS_CORE0_BASE  (0x20088000U)
#define AOS_CORE1_BASE  (0xA0088000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define AOS_CORE0  ((AOS_TypeDef *)      AOS_CORE0_BASE)
#define AOS_CORE1  ((AOS_TypeDef *)      AOS_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define AOS  AOS_CORE0
#endif
#if defined(CORE1)
#define AOS  AOS_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** AOS Instances *******************************/
#define IS_AOS_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == AOS_CORE0) || \
                                          ((INSTANCE) == AOS_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* AOS_SEC_EN */
#define AOS_SEC_EN_SEC_EN                    (BITS(0,7))                                         /*!< Security peripheral linkage enable */

/* AOS_NSEC_EN */
#define AOS_NSEC_EN_NSEC_EN                  (BITS(0,7))                                         /*!< Non-security peripheral linkage enable */

/* AOS_PE_TRGSEL0 */
#define AOS_PE_TRGSEL0_TRGSEL                (BITS(0,5))                                         /*!< TC0 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL1 */
#define AOS_PE_TRGSEL1_TRGSEL                (BITS(0,5))                                         /*!< TC0 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL2 */
#define AOS_PE_TRGSEL2_TRGSEL                (BITS(0,5))                                         /*!< TC0 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL3 */
#define AOS_PE_TRGSEL3_TRGSEL                (BITS(0,5))                                         /*!< TC0 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL4 */
#define AOS_PE_TRGSEL4_TRGSEL                (BITS(0,5))                                         /*!< TC1 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL5 */
#define AOS_PE_TRGSEL5_TRGSEL                (BITS(0,5))                                         /*!< TC1 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL6 */
#define AOS_PE_TRGSEL6_TRGSEL                (BITS(0,5))                                         /*!< TC1 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL7 */
#define AOS_PE_TRGSEL7_TRGSEL                (BITS(0,5))                                         /*!< TC1 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL8 */
#define AOS_PE_TRGSEL8_TRGSEL                (BITS(0,5))                                         /*!< SIMPTC0 channel 0 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL9 */
#define AOS_PE_TRGSEL9_TRGSEL                (BITS(0,5))                                         /*!< SIMPTC0 channel 1 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL10 */
#define AOS_PE_TRGSEL10_TRGSEL               (BITS(0,5))                                         /*!< SIMPTC0 channel 2 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL11 */
#define AOS_PE_TRGSEL11_TRGSEL               (BITS(0,5))                                         /*!< SIMPTC0 channel 3 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL12 */
#define AOS_PE_TRGSEL12_TRGSEL               (BITS(0,5))                                         /*!< SIMPTC1 channel 0 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL13 */
#define AOS_PE_TRGSEL13_TRGSEL               (BITS(0,5))                                         /*!< SIMPTC1 channel 1 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL14 */
#define AOS_PE_TRGSEL14_TRGSEL               (BITS(0,5))                                         /*!< SIMPTC1 channel 2 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL15 */
#define AOS_PE_TRGSEL15_TRGSEL               (BITS(0,5))                                         /*!< SIMPTC1 channel 3 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL16 */
#define AOS_PE_TRGSEL16_TRGSEL               (BITS(0,5))                                         /*!< SIMPTC2 channel 0 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL17 */
#define AOS_PE_TRGSEL17_TRGSEL               (BITS(0,5))                                         /*!< SIMPTC2 channel 1 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL18 */
#define AOS_PE_TRGSEL18_TRGSEL               (BITS(0,5))                                         /*!< SIMPTC2 channel 2 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL19 */
#define AOS_PE_TRGSEL19_TRGSEL               (BITS(0,5))                                         /*!< SIMPTC2 channel 3 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL20 */
#define AOS_PE_TRGSEL20_TRGSEL               (BITS(0,5))                                         /*!< SIMPTC3 channel 0 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL21 */
#define AOS_PE_TRGSEL21_TRGSEL               (BITS(0,5))                                         /*!< SIMPTC3 channel 1 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL22 */
#define AOS_PE_TRGSEL22_TRGSEL               (BITS(0,5))                                         /*!< SIMPTC3 channel 2 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL23 */
#define AOS_PE_TRGSEL23_TRGSEL               (BITS(0,5))                                         /*!< SIMPTC3 channel 3 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL24 */
#define AOS_PE_TRGSEL24_TRGSEL               (BITS(0,5))                                         /*!< D2F Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL25 */
#define AOS_PE_TRGSEL25_TRGSEL               (BITS(0,5))                                         /*!< IOCNT channel 0 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL26 */
#define AOS_PE_TRGSEL26_TRGSEL               (BITS(0,5))                                         /*!< IOCNT channel 1 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL27 */
#define AOS_PE_TRGSEL27_TRGSEL               (BITS(0,5))                                         /*!< IOCNT channel 2 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL28 */
#define AOS_PE_TRGSEL28_TRGSEL               (BITS(0,5))                                         /*!< IOCNT channel 3 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL29 */
#define AOS_PE_TRGSEL29_TRGSEL               (BITS(0,5))                                         /*!< IOCNT channel 4 Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL30 */
#define AOS_PE_TRGSEL30_TRGSEL               (BITS(0,5))                                         /*!< RC_CAL Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL31 */
#define AOS_PE_TRGSEL31_TRGSEL               (BITS(0,5))                                         /*!< CAN Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL32 */
#define AOS_PE_TRGSEL32_TRGSEL               (BITS(0,5))                                         /*!< ADC0 Hardware trigger sequence A Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL33 */
#define AOS_PE_TRGSEL33_TRGSEL               (BITS(0,5))                                         /*!< ADC0 Hardware trigger sequence B Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* AOS_PE_TRGSEL34 */
#define AOS_PE_TRGSEL34_TRGSEL               (BITS(0,5))                                         /*!< DAC Trigger source event number. If 0 is written, it means no trigger source event is selected. */

/* constants definitions */
/* AOS_SEC_EN_SEC_EN */
#define AOS_SEC_EN_SEC_EN_POS                (0U)                                                /*!< AOS_SEC_EN_SEC_EN bit offset */

/* AOS_NSEC_EN_NSEC_EN */
#define AOS_NSEC_EN_NSEC_EN_POS              (0U)                                                /*!< AOS_NSEC_EN_NSEC_EN bit offset */

/* AOS_PE_TRGSEL0_TRGSEL */
#define AOS_PE_TRGSEL0_TRGSEL_POS            (0U)                                                /*!< AOS_PE_TRGSEL0_TRGSEL bit offset */
#define AOS_PE_TRGSEL0_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL0_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL0_TRGSEL_POS))
#define AOS_PE_TRGSEL0_TRGSEL_NONE           (AOS_PE_TRGSEL0_TRGSEL_CFG(0))                      /*!< No trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_UART0_RX_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(1))                      /*!< UART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_UART1_RX_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(2))                      /*!< UART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_UART2_RX_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(3))                      /*!< UART2 RX input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_UART3_RX_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(4))                      /*!< UART3 RX input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_UART4_RX_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(5))                      /*!< UART4 RX input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_UART5_RX_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(6))                      /*!< UART5 RX input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_UART6_RX_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(7))                      /*!< UART6 RX input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_UART7_RX_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(8))                      /*!< UART7 RX input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_LPUART_RX_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(9))                      /*!< LPUART RX input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_USART0_RX_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(10))                     /*!< USART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_USART0_TX_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(11))                     /*!< USART0 TX input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_USART1_RX_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(12))                     /*!< USART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_USART1_TX_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(13))                     /*!< USART1 TX input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_ISO78160_IO_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(14))                     /*!< ISO78160 IO input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_ISO78160_IN_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(15))                     /*!< ISO78160 IN input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_ISO78161_IO_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(16))                     /*!< ISO78161 IO input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_ISO78161_IN_INPUT  (AOS_PE_TRGSEL0_TRGSEL_CFG(17))                     /*!< ISO78161 IN input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_CAN_RX_INPUT   (AOS_PE_TRGSEL0_TRGSEL_CFG(18))                     /*!< CAN RX input signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_D2F_CF_OUT0    (AOS_PE_TRGSEL0_TRGSEL_CFG(19))                     /*!< D2F CF_OUT0 output signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_D2F_CF_OUT1    (AOS_PE_TRGSEL0_TRGSEL_CFG(20))                     /*!< D2F CF_OUT1 output signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_TC1_CH0_POS    (AOS_PE_TRGSEL0_TRGSEL_CFG(21))                     /*!< TC1 channel 0 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_TC1_CH0_NEG    (AOS_PE_TRGSEL0_TRGSEL_CFG(22))                     /*!< TC1 channel 0 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_TC1_CH1_POS    (AOS_PE_TRGSEL0_TRGSEL_CFG(23))                     /*!< TC1 channel 1 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_TC1_CH1_NEG    (AOS_PE_TRGSEL0_TRGSEL_CFG(24))                     /*!< TC1 channel 1 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_VBAT_RTC_OUT   (AOS_PE_TRGSEL0_TRGSEL_CFG(25))                     /*!< VBAT RTC cycle output signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN0   (AOS_PE_TRGSEL0_TRGSEL_CFG(26))                     /*!< GPIO TC PIN0 signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN1   (AOS_PE_TRGSEL0_TRGSEL_CFG(27))                     /*!< GPIO TC PIN1 signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN2   (AOS_PE_TRGSEL0_TRGSEL_CFG(28))                     /*!< GPIO TC PIN2 signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN3   (AOS_PE_TRGSEL0_TRGSEL_CFG(29))                     /*!< GPIO TC PIN3 signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN4   (AOS_PE_TRGSEL0_TRGSEL_CFG(30))                     /*!< GPIO TC PIN4 signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN5   (AOS_PE_TRGSEL0_TRGSEL_CFG(31))                     /*!< GPIO TC PIN5 signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN6   (AOS_PE_TRGSEL0_TRGSEL_CFG(32))                     /*!< GPIO TC PIN6 signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN7   (AOS_PE_TRGSEL0_TRGSEL_CFG(33))                     /*!< GPIO TC PIN7 signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN8   (AOS_PE_TRGSEL0_TRGSEL_CFG(34))                     /*!< GPIO TC PIN8 signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN9   (AOS_PE_TRGSEL0_TRGSEL_CFG(35))                     /*!< GPIO TC PIN9 signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN10  (AOS_PE_TRGSEL0_TRGSEL_CFG(36))                     /*!< GPIO TC PIN10 signal as trigger signal */
#define AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN11  (AOS_PE_TRGSEL0_TRGSEL_CFG(37))                     /*!< GPIO TC PIN11 signal as trigger signal */

/* AOS_PE_TRGSEL1_TRGSEL */
#define AOS_PE_TRGSEL1_TRGSEL_POS            (0U)                                                /*!< AOS_PE_TRGSEL1_TRGSEL bit offset */
#define AOS_PE_TRGSEL1_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL1_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL1_TRGSEL_POS))
#define AOS_PE_TRGSEL1_TRGSEL_NONE           (AOS_PE_TRGSEL1_TRGSEL_CFG(0))                      /*!< No trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_UART0_RX_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(1))                      /*!< UART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_UART1_RX_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(2))                      /*!< UART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_UART2_RX_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(3))                      /*!< UART2 RX input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_UART3_RX_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(4))                      /*!< UART3 RX input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_UART4_RX_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(5))                      /*!< UART4 RX input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_UART5_RX_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(6))                      /*!< UART5 RX input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_UART6_RX_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(7))                      /*!< UART6 RX input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_UART7_RX_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(8))                      /*!< UART7 RX input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_LPUART_RX_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(9))                      /*!< LPUART RX input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_USART0_RX_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(10))                     /*!< USART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_USART0_TX_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(11))                     /*!< USART0 TX input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_USART1_RX_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(12))                     /*!< USART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_USART1_TX_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(13))                     /*!< USART1 TX input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_ISO78160_IO_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(14))                     /*!< ISO78160 IO input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_ISO78160_IN_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(15))                     /*!< ISO78160 IN input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_ISO78161_IO_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(16))                     /*!< ISO78161 IO input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_ISO78161_IN_INPUT  (AOS_PE_TRGSEL1_TRGSEL_CFG(17))                     /*!< ISO78161 IN input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_CAN_RX_INPUT   (AOS_PE_TRGSEL1_TRGSEL_CFG(18))                     /*!< CAN RX input signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_D2F_CF_OUT0    (AOS_PE_TRGSEL1_TRGSEL_CFG(19))                     /*!< D2F CF_OUT0 output signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_D2F_CF_OUT1    (AOS_PE_TRGSEL1_TRGSEL_CFG(20))                     /*!< D2F CF_OUT1 output signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_TC1_CH0_POS    (AOS_PE_TRGSEL1_TRGSEL_CFG(21))                     /*!< TC1 channel 0 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_TC1_CH0_NEG    (AOS_PE_TRGSEL1_TRGSEL_CFG(22))                     /*!< TC1 channel 0 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_TC1_CH1_POS    (AOS_PE_TRGSEL1_TRGSEL_CFG(23))                     /*!< TC1 channel 1 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_TC1_CH1_NEG    (AOS_PE_TRGSEL1_TRGSEL_CFG(24))                     /*!< TC1 channel 1 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_VBAT_RTC_OUT   (AOS_PE_TRGSEL1_TRGSEL_CFG(25))                     /*!< VBAT RTC cycle output signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_GPIO_TC_PIN0   (AOS_PE_TRGSEL1_TRGSEL_CFG(26))                     /*!< GPIO TC PIN0 signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_GPIO_TC_PIN1   (AOS_PE_TRGSEL1_TRGSEL_CFG(27))                     /*!< GPIO TC PIN1 signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_GPIO_TC_PIN2   (AOS_PE_TRGSEL1_TRGSEL_CFG(28))                     /*!< GPIO TC PIN2 signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_GPIO_TC_PIN3   (AOS_PE_TRGSEL1_TRGSEL_CFG(29))                     /*!< GPIO TC PIN3 signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_GPIO_TC_PIN4   (AOS_PE_TRGSEL1_TRGSEL_CFG(30))                     /*!< GPIO TC PIN4 signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_GPIO_TC_PIN5   (AOS_PE_TRGSEL1_TRGSEL_CFG(31))                     /*!< GPIO TC PIN5 signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_GPIO_TC_PIN6   (AOS_PE_TRGSEL1_TRGSEL_CFG(32))                     /*!< GPIO TC PIN6 signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_GPIO_TC_PIN7   (AOS_PE_TRGSEL1_TRGSEL_CFG(33))                     /*!< GPIO TC PIN7 signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_GPIO_TC_PIN8   (AOS_PE_TRGSEL1_TRGSEL_CFG(34))                     /*!< GPIO TC PIN8 signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_GPIO_TC_PIN9   (AOS_PE_TRGSEL1_TRGSEL_CFG(35))                     /*!< GPIO TC PIN9 signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_GPIO_TC_PIN10  (AOS_PE_TRGSEL1_TRGSEL_CFG(36))                     /*!< GPIO TC PIN10 signal as trigger signal */
#define AOS_PE_TRGSEL1_TRGSEL_GPIO_TC_PIN11  (AOS_PE_TRGSEL1_TRGSEL_CFG(37))                     /*!< GPIO TC PIN11 signal as trigger signal */

/* AOS_PE_TRGSEL2_TRGSEL */
#define AOS_PE_TRGSEL2_TRGSEL_POS            (0U)                                                /*!< AOS_PE_TRGSEL2_TRGSEL bit offset */
#define AOS_PE_TRGSEL2_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL2_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL2_TRGSEL_POS))
#define AOS_PE_TRGSEL2_TRGSEL_NONE           (AOS_PE_TRGSEL2_TRGSEL_CFG(0))                      /*!< No trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_UART0_RX_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(1))                      /*!< UART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_UART1_RX_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(2))                      /*!< UART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_UART2_RX_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(3))                      /*!< UART2 RX input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_UART3_RX_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(4))                      /*!< UART3 RX input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_UART4_RX_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(5))                      /*!< UART4 RX input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_UART5_RX_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(6))                      /*!< UART5 RX input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_UART6_RX_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(7))                      /*!< UART6 RX input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_UART7_RX_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(8))                      /*!< UART7 RX input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_LPUART_RX_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(9))                      /*!< LPUART RX input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_USART0_RX_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(10))                     /*!< USART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_USART0_TX_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(11))                     /*!< USART0 TX input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_USART1_RX_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(12))                     /*!< USART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_USART1_TX_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(13))                     /*!< USART1 TX input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_ISO78160_IO_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(14))                     /*!< ISO78160 IO input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_ISO78160_IN_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(15))                     /*!< ISO78160 IN input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_ISO78161_IO_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(16))                     /*!< ISO78161 IO input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_ISO78161_IN_INPUT  (AOS_PE_TRGSEL2_TRGSEL_CFG(17))                     /*!< ISO78161 IN input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_CAN_RX_INPUT   (AOS_PE_TRGSEL2_TRGSEL_CFG(18))                     /*!< CAN RX input signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_D2F_CF_OUT0    (AOS_PE_TRGSEL2_TRGSEL_CFG(19))                     /*!< D2F CF_OUT0 output signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_D2F_CF_OUT1    (AOS_PE_TRGSEL2_TRGSEL_CFG(20))                     /*!< D2F CF_OUT1 output signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_TC1_CH0_POS    (AOS_PE_TRGSEL2_TRGSEL_CFG(21))                     /*!< TC1 channel 0 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_TC1_CH0_NEG    (AOS_PE_TRGSEL2_TRGSEL_CFG(22))                     /*!< TC1 channel 0 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_TC1_CH1_POS    (AOS_PE_TRGSEL2_TRGSEL_CFG(23))                     /*!< TC1 channel 1 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_TC1_CH1_NEG    (AOS_PE_TRGSEL2_TRGSEL_CFG(24))                     /*!< TC1 channel 1 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_VBAT_RTC_OUT   (AOS_PE_TRGSEL2_TRGSEL_CFG(25))                     /*!< VBAT RTC cycle output signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_GPIO_TC_PIN0   (AOS_PE_TRGSEL2_TRGSEL_CFG(26))                     /*!< GPIO TC PIN0 signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_GPIO_TC_PIN1   (AOS_PE_TRGSEL2_TRGSEL_CFG(27))                     /*!< GPIO TC PIN1 signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_GPIO_TC_PIN2   (AOS_PE_TRGSEL2_TRGSEL_CFG(28))                     /*!< GPIO TC PIN2 signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_GPIO_TC_PIN3   (AOS_PE_TRGSEL2_TRGSEL_CFG(29))                     /*!< GPIO TC PIN3 signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_GPIO_TC_PIN4   (AOS_PE_TRGSEL2_TRGSEL_CFG(30))                     /*!< GPIO TC PIN4 signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_GPIO_TC_PIN5   (AOS_PE_TRGSEL2_TRGSEL_CFG(31))                     /*!< GPIO TC PIN5 signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_GPIO_TC_PIN6   (AOS_PE_TRGSEL2_TRGSEL_CFG(32))                     /*!< GPIO TC PIN6 signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_GPIO_TC_PIN7   (AOS_PE_TRGSEL2_TRGSEL_CFG(33))                     /*!< GPIO TC PIN7 signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_GPIO_TC_PIN8   (AOS_PE_TRGSEL2_TRGSEL_CFG(34))                     /*!< GPIO TC PIN8 signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_GPIO_TC_PIN9   (AOS_PE_TRGSEL2_TRGSEL_CFG(35))                     /*!< GPIO TC PIN9 signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_GPIO_TC_PIN10  (AOS_PE_TRGSEL2_TRGSEL_CFG(36))                     /*!< GPIO TC PIN10 signal as trigger signal */
#define AOS_PE_TRGSEL2_TRGSEL_GPIO_TC_PIN11  (AOS_PE_TRGSEL2_TRGSEL_CFG(37))                     /*!< GPIO TC PIN11 signal as trigger signal */

/* AOS_PE_TRGSEL3_TRGSEL */
#define AOS_PE_TRGSEL3_TRGSEL_POS            (0U)                                                /*!< AOS_PE_TRGSEL3_TRGSEL bit offset */
#define AOS_PE_TRGSEL3_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL3_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL3_TRGSEL_POS))
#define AOS_PE_TRGSEL3_TRGSEL_NONE           (AOS_PE_TRGSEL3_TRGSEL_CFG(0))                      /*!< No trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_UART0_RX_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(1))                      /*!< UART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_UART1_RX_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(2))                      /*!< UART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_UART2_RX_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(3))                      /*!< UART2 RX input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_UART3_RX_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(4))                      /*!< UART3 RX input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_UART4_RX_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(5))                      /*!< UART4 RX input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_UART5_RX_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(6))                      /*!< UART5 RX input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_UART6_RX_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(7))                      /*!< UART6 RX input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_UART7_RX_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(8))                      /*!< UART7 RX input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_LPUART_RX_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(9))                      /*!< LPUART RX input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_USART0_RX_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(10))                     /*!< USART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_USART0_TX_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(11))                     /*!< USART0 TX input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_USART1_RX_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(12))                     /*!< USART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_USART1_TX_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(13))                     /*!< USART1 TX input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_ISO78160_IO_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(14))                     /*!< ISO78160 IO input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_ISO78160_IN_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(15))                     /*!< ISO78160 IN input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_ISO78161_IO_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(16))                     /*!< ISO78161 IO input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_ISO78161_IN_INPUT  (AOS_PE_TRGSEL3_TRGSEL_CFG(17))                     /*!< ISO78161 IN input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_CAN_RX_INPUT   (AOS_PE_TRGSEL3_TRGSEL_CFG(18))                     /*!< CAN RX input signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_D2F_CF_OUT0    (AOS_PE_TRGSEL3_TRGSEL_CFG(19))                     /*!< D2F CF_OUT0 output signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_D2F_CF_OUT1    (AOS_PE_TRGSEL3_TRGSEL_CFG(20))                     /*!< D2F CF_OUT1 output signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_TC1_CH0_POS    (AOS_PE_TRGSEL3_TRGSEL_CFG(21))                     /*!< TC1 channel 0 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_TC1_CH0_NEG    (AOS_PE_TRGSEL3_TRGSEL_CFG(22))                     /*!< TC1 channel 0 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_TC1_CH1_POS    (AOS_PE_TRGSEL3_TRGSEL_CFG(23))                     /*!< TC1 channel 1 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_TC1_CH1_NEG    (AOS_PE_TRGSEL3_TRGSEL_CFG(24))                     /*!< TC1 channel 1 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_VBAT_RTC_OUT   (AOS_PE_TRGSEL3_TRGSEL_CFG(25))                     /*!< VBAT RTC cycle output signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_GPIO_TC_PIN0   (AOS_PE_TRGSEL3_TRGSEL_CFG(26))                     /*!< GPIO TC PIN0 signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_GPIO_TC_PIN1   (AOS_PE_TRGSEL3_TRGSEL_CFG(27))                     /*!< GPIO TC PIN1 signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_GPIO_TC_PIN2   (AOS_PE_TRGSEL3_TRGSEL_CFG(28))                     /*!< GPIO TC PIN2 signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_GPIO_TC_PIN3   (AOS_PE_TRGSEL3_TRGSEL_CFG(29))                     /*!< GPIO TC PIN3 signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_GPIO_TC_PIN4   (AOS_PE_TRGSEL3_TRGSEL_CFG(30))                     /*!< GPIO TC PIN4 signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_GPIO_TC_PIN5   (AOS_PE_TRGSEL3_TRGSEL_CFG(31))                     /*!< GPIO TC PIN5 signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_GPIO_TC_PIN6   (AOS_PE_TRGSEL3_TRGSEL_CFG(32))                     /*!< GPIO TC PIN6 signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_GPIO_TC_PIN7   (AOS_PE_TRGSEL3_TRGSEL_CFG(33))                     /*!< GPIO TC PIN7 signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_GPIO_TC_PIN8   (AOS_PE_TRGSEL3_TRGSEL_CFG(34))                     /*!< GPIO TC PIN8 signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_GPIO_TC_PIN9   (AOS_PE_TRGSEL3_TRGSEL_CFG(35))                     /*!< GPIO TC PIN9 signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_GPIO_TC_PIN10  (AOS_PE_TRGSEL3_TRGSEL_CFG(36))                     /*!< GPIO TC PIN10 signal as trigger signal */
#define AOS_PE_TRGSEL3_TRGSEL_GPIO_TC_PIN11  (AOS_PE_TRGSEL3_TRGSEL_CFG(37))                     /*!< GPIO TC PIN11 signal as trigger signal */

/* AOS_PE_TRGSEL4_TRGSEL */
#define AOS_PE_TRGSEL4_TRGSEL_POS            (0U)                                                /*!< AOS_PE_TRGSEL4_TRGSEL bit offset */
#define AOS_PE_TRGSEL4_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL4_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL4_TRGSEL_POS))
#define AOS_PE_TRGSEL4_TRGSEL_NONE           (AOS_PE_TRGSEL4_TRGSEL_CFG(0))                      /*!< No trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_UART0_RX_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(1))                      /*!< UART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_UART1_RX_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(2))                      /*!< UART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_UART2_RX_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(3))                      /*!< UART2 RX input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_UART3_RX_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(4))                      /*!< UART3 RX input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_UART4_RX_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(5))                      /*!< UART4 RX input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_UART5_RX_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(6))                      /*!< UART5 RX input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_UART6_RX_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(7))                      /*!< UART6 RX input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_UART7_RX_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(8))                      /*!< UART7 RX input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_LPUART_RX_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(9))                      /*!< LPUART RX input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_USART0_RX_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(10))                     /*!< USART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_USART0_TX_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(11))                     /*!< USART0 TX input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_USART1_RX_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(12))                     /*!< USART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_USART1_TX_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(13))                     /*!< USART1 TX input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_ISO78160_IO_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(14))                     /*!< ISO78160 IO input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_ISO78160_IN_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(15))                     /*!< ISO78160 IN input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_ISO78161_IO_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(16))                     /*!< ISO78161 IO input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_ISO78161_IN_INPUT  (AOS_PE_TRGSEL4_TRGSEL_CFG(17))                     /*!< ISO78161 IN input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_CAN_RX_INPUT   (AOS_PE_TRGSEL4_TRGSEL_CFG(18))                     /*!< CAN RX input signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_D2F_CF_OUT1    (AOS_PE_TRGSEL4_TRGSEL_CFG(19))                     /*!< D2F CF_OUT1 output signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_D2F_CF_OUT2    (AOS_PE_TRGSEL4_TRGSEL_CFG(20))                     /*!< D2F CF_OUT2 output signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_TC0_CH0_POS    (AOS_PE_TRGSEL4_TRGSEL_CFG(21))                     /*!< TC0 channel 0 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_TC0_CH0_NEG    (AOS_PE_TRGSEL4_TRGSEL_CFG(22))                     /*!< TC0 channel 0 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_TC0_CH1_POS    (AOS_PE_TRGSEL4_TRGSEL_CFG(23))                     /*!< TC0 channel 1 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_TC0_CH1_NEG    (AOS_PE_TRGSEL4_TRGSEL_CFG(24))                     /*!< TC0 channel 1 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_VBAT_RTC_OUT   (AOS_PE_TRGSEL4_TRGSEL_CFG(25))                     /*!< VBAT RTC cycle output signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN0   (AOS_PE_TRGSEL4_TRGSEL_CFG(26))                     /*!< GPIO TC PIN0 signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN1   (AOS_PE_TRGSEL4_TRGSEL_CFG(27))                     /*!< GPIO TC PIN1 signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN2   (AOS_PE_TRGSEL4_TRGSEL_CFG(28))                     /*!< GPIO TC PIN2 signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN3   (AOS_PE_TRGSEL4_TRGSEL_CFG(29))                     /*!< GPIO TC PIN3 signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN4   (AOS_PE_TRGSEL4_TRGSEL_CFG(30))                     /*!< GPIO TC PIN4 signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN5   (AOS_PE_TRGSEL4_TRGSEL_CFG(31))                     /*!< GPIO TC PIN5 signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN6   (AOS_PE_TRGSEL4_TRGSEL_CFG(32))                     /*!< GPIO TC PIN6 signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN7   (AOS_PE_TRGSEL4_TRGSEL_CFG(33))                     /*!< GPIO TC PIN7 signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN8   (AOS_PE_TRGSEL4_TRGSEL_CFG(34))                     /*!< GPIO TC PIN8 signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN9   (AOS_PE_TRGSEL4_TRGSEL_CFG(35))                     /*!< GPIO TC PIN9 signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN10  (AOS_PE_TRGSEL4_TRGSEL_CFG(36))                     /*!< GPIO TC PIN10 signal as trigger signal */
#define AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN11  (AOS_PE_TRGSEL4_TRGSEL_CFG(37))                     /*!< GPIO TC PIN11 signal as trigger signal */

/* AOS_PE_TRGSEL5_TRGSEL */
#define AOS_PE_TRGSEL5_TRGSEL_POS            (0U)                                                /*!< AOS_PE_TRGSEL5_TRGSEL bit offset */
#define AOS_PE_TRGSEL5_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL5_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL5_TRGSEL_POS))
#define AOS_PE_TRGSEL5_TRGSEL_NONE           (AOS_PE_TRGSEL5_TRGSEL_CFG(0))                      /*!< No trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_UART0_RX_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(1))                      /*!< UART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_UART1_RX_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(2))                      /*!< UART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_UART2_RX_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(3))                      /*!< UART2 RX input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_UART3_RX_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(4))                      /*!< UART3 RX input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_UART4_RX_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(5))                      /*!< UART4 RX input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_UART5_RX_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(6))                      /*!< UART5 RX input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_UART6_RX_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(7))                      /*!< UART6 RX input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_UART7_RX_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(8))                      /*!< UART7 RX input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_LPUART_RX_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(9))                      /*!< LPUART RX input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_USART0_RX_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(10))                     /*!< USART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_USART0_TX_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(11))                     /*!< USART0 TX input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_USART1_RX_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(12))                     /*!< USART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_USART1_TX_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(13))                     /*!< USART1 TX input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_ISO78160_IO_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(14))                     /*!< ISO78160 IO input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_ISO78160_IN_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(15))                     /*!< ISO78160 IN input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_ISO78161_IO_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(16))                     /*!< ISO78161 IO input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_ISO78161_IN_INPUT  (AOS_PE_TRGSEL5_TRGSEL_CFG(17))                     /*!< ISO78161 IN input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_CAN_RX_INPUT   (AOS_PE_TRGSEL5_TRGSEL_CFG(18))                     /*!< CAN RX input signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_D2F_CF_OUT1    (AOS_PE_TRGSEL5_TRGSEL_CFG(19))                     /*!< D2F CF_OUT1 output signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_D2F_CF_OUT2    (AOS_PE_TRGSEL5_TRGSEL_CFG(20))                     /*!< D2F CF_OUT2 output signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_TC0_CH0_POS    (AOS_PE_TRGSEL5_TRGSEL_CFG(21))                     /*!< TC0 channel 0 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_TC0_CH0_NEG    (AOS_PE_TRGSEL5_TRGSEL_CFG(22))                     /*!< TC0 channel 0 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_TC0_CH1_POS    (AOS_PE_TRGSEL5_TRGSEL_CFG(23))                     /*!< TC0 channel 1 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_TC0_CH1_NEG    (AOS_PE_TRGSEL5_TRGSEL_CFG(24))                     /*!< TC0 channel 1 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_VBAT_RTC_OUT   (AOS_PE_TRGSEL5_TRGSEL_CFG(25))                     /*!< VBAT RTC cycle output signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_GPIO_TC_PIN0   (AOS_PE_TRGSEL5_TRGSEL_CFG(26))                     /*!< GPIO TC PIN0 signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_GPIO_TC_PIN1   (AOS_PE_TRGSEL5_TRGSEL_CFG(27))                     /*!< GPIO TC PIN1 signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_GPIO_TC_PIN2   (AOS_PE_TRGSEL5_TRGSEL_CFG(28))                     /*!< GPIO TC PIN2 signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_GPIO_TC_PIN3   (AOS_PE_TRGSEL5_TRGSEL_CFG(29))                     /*!< GPIO TC PIN3 signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_GPIO_TC_PIN4   (AOS_PE_TRGSEL5_TRGSEL_CFG(30))                     /*!< GPIO TC PIN4 signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_GPIO_TC_PIN5   (AOS_PE_TRGSEL5_TRGSEL_CFG(31))                     /*!< GPIO TC PIN5 signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_GPIO_TC_PIN6   (AOS_PE_TRGSEL5_TRGSEL_CFG(32))                     /*!< GPIO TC PIN6 signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_GPIO_TC_PIN7   (AOS_PE_TRGSEL5_TRGSEL_CFG(33))                     /*!< GPIO TC PIN7 signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_GPIO_TC_PIN8   (AOS_PE_TRGSEL5_TRGSEL_CFG(34))                     /*!< GPIO TC PIN8 signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_GPIO_TC_PIN9   (AOS_PE_TRGSEL5_TRGSEL_CFG(35))                     /*!< GPIO TC PIN9 signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_GPIO_TC_PIN10  (AOS_PE_TRGSEL5_TRGSEL_CFG(36))                     /*!< GPIO TC PIN10 signal as trigger signal */
#define AOS_PE_TRGSEL5_TRGSEL_GPIO_TC_PIN11  (AOS_PE_TRGSEL5_TRGSEL_CFG(37))                     /*!< GPIO TC PIN11 signal as trigger signal */

/* AOS_PE_TRGSEL6_TRGSEL */
#define AOS_PE_TRGSEL6_TRGSEL_POS            (0U)                                                /*!< AOS_PE_TRGSEL6_TRGSEL bit offset */
#define AOS_PE_TRGSEL6_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL6_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL6_TRGSEL_POS))
#define AOS_PE_TRGSEL6_TRGSEL_NONE           (AOS_PE_TRGSEL6_TRGSEL_CFG(0))                      /*!< No trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_UART0_RX_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(1))                      /*!< UART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_UART1_RX_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(2))                      /*!< UART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_UART2_RX_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(3))                      /*!< UART2 RX input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_UART3_RX_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(4))                      /*!< UART3 RX input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_UART4_RX_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(5))                      /*!< UART4 RX input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_UART5_RX_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(6))                      /*!< UART5 RX input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_UART6_RX_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(7))                      /*!< UART6 RX input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_UART7_RX_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(8))                      /*!< UART7 RX input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_LPUART_RX_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(9))                      /*!< LPUART RX input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_USART0_RX_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(10))                     /*!< USART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_USART0_TX_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(11))                     /*!< USART0 TX input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_USART1_RX_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(12))                     /*!< USART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_USART1_TX_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(13))                     /*!< USART1 TX input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_ISO78160_IO_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(14))                     /*!< ISO78160 IO input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_ISO78160_IN_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(15))                     /*!< ISO78160 IN input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_ISO78161_IO_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(16))                     /*!< ISO78161 IO input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_ISO78161_IN_INPUT  (AOS_PE_TRGSEL6_TRGSEL_CFG(17))                     /*!< ISO78161 IN input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_CAN_RX_INPUT   (AOS_PE_TRGSEL6_TRGSEL_CFG(18))                     /*!< CAN RX input signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_D2F_CF_OUT1    (AOS_PE_TRGSEL6_TRGSEL_CFG(19))                     /*!< D2F CF_OUT1 output signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_D2F_CF_OUT2    (AOS_PE_TRGSEL6_TRGSEL_CFG(20))                     /*!< D2F CF_OUT2 output signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_TC0_CH0_POS    (AOS_PE_TRGSEL6_TRGSEL_CFG(21))                     /*!< TC0 channel 0 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_TC0_CH0_NEG    (AOS_PE_TRGSEL6_TRGSEL_CFG(22))                     /*!< TC0 channel 0 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_TC0_CH1_POS    (AOS_PE_TRGSEL6_TRGSEL_CFG(23))                     /*!< TC0 channel 1 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_TC0_CH1_NEG    (AOS_PE_TRGSEL6_TRGSEL_CFG(24))                     /*!< TC0 channel 1 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_VBAT_RTC_OUT   (AOS_PE_TRGSEL6_TRGSEL_CFG(25))                     /*!< VBAT RTC cycle output signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_GPIO_TC_PIN0   (AOS_PE_TRGSEL6_TRGSEL_CFG(26))                     /*!< GPIO TC PIN0 signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_GPIO_TC_PIN1   (AOS_PE_TRGSEL6_TRGSEL_CFG(27))                     /*!< GPIO TC PIN1 signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_GPIO_TC_PIN2   (AOS_PE_TRGSEL6_TRGSEL_CFG(28))                     /*!< GPIO TC PIN2 signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_GPIO_TC_PIN3   (AOS_PE_TRGSEL6_TRGSEL_CFG(29))                     /*!< GPIO TC PIN3 signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_GPIO_TC_PIN4   (AOS_PE_TRGSEL6_TRGSEL_CFG(30))                     /*!< GPIO TC PIN4 signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_GPIO_TC_PIN5   (AOS_PE_TRGSEL6_TRGSEL_CFG(31))                     /*!< GPIO TC PIN5 signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_GPIO_TC_PIN6   (AOS_PE_TRGSEL6_TRGSEL_CFG(32))                     /*!< GPIO TC PIN6 signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_GPIO_TC_PIN7   (AOS_PE_TRGSEL6_TRGSEL_CFG(33))                     /*!< GPIO TC PIN7 signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_GPIO_TC_PIN8   (AOS_PE_TRGSEL6_TRGSEL_CFG(34))                     /*!< GPIO TC PIN8 signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_GPIO_TC_PIN9   (AOS_PE_TRGSEL6_TRGSEL_CFG(35))                     /*!< GPIO TC PIN9 signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_GPIO_TC_PIN10  (AOS_PE_TRGSEL6_TRGSEL_CFG(36))                     /*!< GPIO TC PIN10 signal as trigger signal */
#define AOS_PE_TRGSEL6_TRGSEL_GPIO_TC_PIN11  (AOS_PE_TRGSEL6_TRGSEL_CFG(37))                     /*!< GPIO TC PIN11 signal as trigger signal */

/* AOS_PE_TRGSEL7_TRGSEL */
#define AOS_PE_TRGSEL7_TRGSEL_POS            (0U)                                                /*!< AOS_PE_TRGSEL7_TRGSEL bit offset */
#define AOS_PE_TRGSEL7_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL7_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL7_TRGSEL_POS))
#define AOS_PE_TRGSEL7_TRGSEL_NONE           (AOS_PE_TRGSEL7_TRGSEL_CFG(0))                      /*!< No trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_UART0_RX_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(1))                      /*!< UART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_UART1_RX_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(2))                      /*!< UART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_UART2_RX_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(3))                      /*!< UART2 RX input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_UART3_RX_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(4))                      /*!< UART3 RX input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_UART4_RX_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(5))                      /*!< UART4 RX input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_UART5_RX_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(6))                      /*!< UART5 RX input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_UART6_RX_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(7))                      /*!< UART6 RX input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_UART7_RX_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(8))                      /*!< UART7 RX input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_LPUART_RX_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(9))                      /*!< LPUART RX input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_USART0_RX_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(10))                     /*!< USART0 RX input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_USART0_TX_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(11))                     /*!< USART0 TX input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_USART1_RX_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(12))                     /*!< USART1 RX input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_USART1_TX_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(13))                     /*!< USART1 TX input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_ISO78160_IO_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(14))                     /*!< ISO78160 IO input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_ISO78160_IN_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(15))                     /*!< ISO78160 IN input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_ISO78161_IO_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(16))                     /*!< ISO78161 IO input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_ISO78161_IN_INPUT  (AOS_PE_TRGSEL7_TRGSEL_CFG(17))                     /*!< ISO78161 IN input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_CAN_RX_INPUT   (AOS_PE_TRGSEL7_TRGSEL_CFG(18))                     /*!< CAN RX input signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_D2F_CF_OUT1    (AOS_PE_TRGSEL7_TRGSEL_CFG(19))                     /*!< D2F CF_OUT1 output signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_D2F_CF_OUT2    (AOS_PE_TRGSEL7_TRGSEL_CFG(20))                     /*!< D2F CF_OUT2 output signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_TC0_CH0_POS    (AOS_PE_TRGSEL7_TRGSEL_CFG(21))                     /*!< TC0 channel 0 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_TC0_CH0_NEG    (AOS_PE_TRGSEL7_TRGSEL_CFG(22))                     /*!< TC0 channel 0 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_TC0_CH1_POS    (AOS_PE_TRGSEL7_TRGSEL_CFG(23))                     /*!< TC0 channel 1 positive port output signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_TC0_CH1_NEG    (AOS_PE_TRGSEL7_TRGSEL_CFG(24))                     /*!< TC0 channel 1 negative port output signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_VBAT_RTC_OUT   (AOS_PE_TRGSEL7_TRGSEL_CFG(25))                     /*!< VBAT RTC cycle output signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_GPIO_TC_PIN0   (AOS_PE_TRGSEL7_TRGSEL_CFG(26))                     /*!< GPIO TC PIN0 signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_GPIO_TC_PIN1   (AOS_PE_TRGSEL7_TRGSEL_CFG(27))                     /*!< GPIO TC PIN1 signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_GPIO_TC_PIN2   (AOS_PE_TRGSEL7_TRGSEL_CFG(28))                     /*!< GPIO TC PIN2 signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_GPIO_TC_PIN3   (AOS_PE_TRGSEL7_TRGSEL_CFG(29))                     /*!< GPIO TC PIN3 signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_GPIO_TC_PIN4   (AOS_PE_TRGSEL7_TRGSEL_CFG(30))                     /*!< GPIO TC PIN4 signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_GPIO_TC_PIN5   (AOS_PE_TRGSEL7_TRGSEL_CFG(31))                     /*!< GPIO TC PIN5 signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_GPIO_TC_PIN6   (AOS_PE_TRGSEL7_TRGSEL_CFG(32))                     /*!< GPIO TC PIN6 signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_GPIO_TC_PIN7   (AOS_PE_TRGSEL7_TRGSEL_CFG(33))                     /*!< GPIO TC PIN7 signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_GPIO_TC_PIN8   (AOS_PE_TRGSEL7_TRGSEL_CFG(34))                     /*!< GPIO TC PIN8 signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_GPIO_TC_PIN9   (AOS_PE_TRGSEL7_TRGSEL_CFG(35))                     /*!< GPIO TC PIN9 signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_GPIO_TC_PIN10  (AOS_PE_TRGSEL7_TRGSEL_CFG(36))                     /*!< GPIO TC PIN10 signal as trigger signal */
#define AOS_PE_TRGSEL7_TRGSEL_GPIO_TC_PIN11  (AOS_PE_TRGSEL7_TRGSEL_CFG(37))                     /*!< GPIO TC PIN11 signal as trigger signal */

/* AOS_PE_TRGSEL8_TRGSEL */
#define AOS_PE_TRGSEL8_TRGSEL_POS            (0U)                                                /*!< AOS_PE_TRGSEL8_TRGSEL bit offset */
#define AOS_PE_TRGSEL8_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL8_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL8_TRGSEL_POS))
#define AOS_PE_TRGSEL8_TRGSEL_NONE           (AOS_PE_TRGSEL8_TRGSEL_CFG(0))                      /*!< No trigger signal */
#define AOS_PE_TRGSEL8_TRGSEL_KBI0_CH0_OUTPUT  (AOS_PE_TRGSEL8_TRGSEL_CFG(1))                      /*!< KBI0 channel 0 output signal as trigger signal */
#define AOS_PE_TRGSEL8_TRGSEL_KBI0_CH4_OUTPUT  (AOS_PE_TRGSEL8_TRGSEL_CFG(2))                      /*!< KBI0 channel 4 output signal as trigger signal */
#define AOS_PE_TRGSEL8_TRGSEL_KBI1_CH0_OUTPUT  (AOS_PE_TRGSEL8_TRGSEL_CFG(3))                      /*!< KBI1 channel 0 output signal as trigger signal */
#define AOS_PE_TRGSEL8_TRGSEL_KBI1_CH4_OUTPUT  (AOS_PE_TRGSEL8_TRGSEL_CFG(4))                      /*!< KBI1 channel 4 output signal as trigger signal */

/* AOS_PE_TRGSEL9_TRGSEL */
#define AOS_PE_TRGSEL9_TRGSEL_POS            (0U)                                                /*!< AOS_PE_TRGSEL9_TRGSEL bit offset */
#define AOS_PE_TRGSEL9_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL9_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL9_TRGSEL_POS))
#define AOS_PE_TRGSEL9_TRGSEL_NONE           (AOS_PE_TRGSEL9_TRGSEL_CFG(0))                      /*!< No trigger signal */
#define AOS_PE_TRGSEL9_TRGSEL_KBI0_CH1_OUTPUT  (AOS_PE_TRGSEL9_TRGSEL_CFG(1))                      /*!< KBI0 channel 1 output signal as trigger signal */
#define AOS_PE_TRGSEL9_TRGSEL_KBI0_CH5_OUTPUT  (AOS_PE_TRGSEL9_TRGSEL_CFG(2))                      /*!< KBI0 channel 5 output signal as trigger signal */
#define AOS_PE_TRGSEL9_TRGSEL_KBI1_CH1_OUTPUT  (AOS_PE_TRGSEL9_TRGSEL_CFG(3))                      /*!< KBI1 channel 1 output signal as trigger signal */
#define AOS_PE_TRGSEL9_TRGSEL_KBI1_CH5_OUTPUT  (AOS_PE_TRGSEL9_TRGSEL_CFG(4))                      /*!< KBI1 channel 5 output signal as trigger signal */

/* AOS_PE_TRGSEL10_TRGSEL */
#define AOS_PE_TRGSEL10_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL10_TRGSEL bit offset */
#define AOS_PE_TRGSEL10_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL10_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL10_TRGSEL_POS))
#define AOS_PE_TRGSEL10_TRGSEL_NONE          (AOS_PE_TRGSEL10_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL10_TRGSEL_KBI0_CH2_OUTPUT  (AOS_PE_TRGSEL10_TRGSEL_CFG(1))                     /*!< KBI0 channel 2 output signal as trigger signal */
#define AOS_PE_TRGSEL10_TRGSEL_KBI0_CH6_OUTPUT  (AOS_PE_TRGSEL10_TRGSEL_CFG(2))                     /*!< KBI0 channel 6 output signal as trigger signal */
#define AOS_PE_TRGSEL10_TRGSEL_KBI1_CH2_OUTPUT  (AOS_PE_TRGSEL10_TRGSEL_CFG(3))                     /*!< KBI1 channel 2 output signal as trigger signal */
#define AOS_PE_TRGSEL10_TRGSEL_KBI1_CH6_OUTPUT  (AOS_PE_TRGSEL10_TRGSEL_CFG(4))                     /*!< KBI1 channel 6 output signal as trigger signal */

/* AOS_PE_TRGSEL11_TRGSEL */
#define AOS_PE_TRGSEL11_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL11_TRGSEL bit offset */
#define AOS_PE_TRGSEL11_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL11_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL11_TRGSEL_POS))
#define AOS_PE_TRGSEL11_TRGSEL_NONE          (AOS_PE_TRGSEL11_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL11_TRGSEL_KBI0_CH3_OUTPUT  (AOS_PE_TRGSEL11_TRGSEL_CFG(1))                     /*!< KBI0 channel 3 output signal as trigger signal */
#define AOS_PE_TRGSEL11_TRGSEL_KBI0_CH7_OUTPUT  (AOS_PE_TRGSEL11_TRGSEL_CFG(2))                     /*!< KBI0 channel 7 output signal as trigger signal */
#define AOS_PE_TRGSEL11_TRGSEL_KBI1_CH3_OUTPUT  (AOS_PE_TRGSEL11_TRGSEL_CFG(3))                     /*!< KBI1 channel 3 output signal as trigger signal */
#define AOS_PE_TRGSEL11_TRGSEL_KBI1_CH7_OUTPUT  (AOS_PE_TRGSEL11_TRGSEL_CFG(4))                     /*!< KBI1 channel 7 output signal as trigger signal */

/* AOS_PE_TRGSEL12_TRGSEL */
#define AOS_PE_TRGSEL12_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL12_TRGSEL bit offset */
#define AOS_PE_TRGSEL12_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL12_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL12_TRGSEL_POS))
#define AOS_PE_TRGSEL12_TRGSEL_NONE          (AOS_PE_TRGSEL12_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL12_TRGSEL_KBI0_CH0_OUTPUT  (AOS_PE_TRGSEL12_TRGSEL_CFG(1))                     /*!< KBI0 channel 0 output signal as trigger signal */
#define AOS_PE_TRGSEL12_TRGSEL_KBI0_CH4_OUTPUT  (AOS_PE_TRGSEL12_TRGSEL_CFG(2))                     /*!< KBI0 channel 4 output signal as trigger signal */
#define AOS_PE_TRGSEL12_TRGSEL_KBI1_CH0_OUTPUT  (AOS_PE_TRGSEL12_TRGSEL_CFG(3))                     /*!< KBI1 channel 0 output signal as trigger signal */
#define AOS_PE_TRGSEL12_TRGSEL_KBI1_CH4_OUTPUT  (AOS_PE_TRGSEL12_TRGSEL_CFG(4))                     /*!< KBI1 channel 4 output signal as trigger signal */

/* AOS_PE_TRGSEL13_TRGSEL */
#define AOS_PE_TRGSEL13_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL13_TRGSEL bit offset */
#define AOS_PE_TRGSEL13_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL13_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL13_TRGSEL_POS))
#define AOS_PE_TRGSEL13_TRGSEL_NONE          (AOS_PE_TRGSEL13_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL13_TRGSEL_KBI0_CH1_OUTPUT  (AOS_PE_TRGSEL13_TRGSEL_CFG(1))                     /*!< KBI0 channel 1 output signal as trigger signal */
#define AOS_PE_TRGSEL13_TRGSEL_KBI0_CH5_OUTPUT  (AOS_PE_TRGSEL13_TRGSEL_CFG(2))                     /*!< KBI0 channel 5 output signal as trigger signal */
#define AOS_PE_TRGSEL13_TRGSEL_KBI1_CH1_OUTPUT  (AOS_PE_TRGSEL13_TRGSEL_CFG(3))                     /*!< KBI1 channel 1 output signal as trigger signal */
#define AOS_PE_TRGSEL13_TRGSEL_KBI1_CH5_OUTPUT  (AOS_PE_TRGSEL13_TRGSEL_CFG(4))                     /*!< KBI1 channel 5 output signal as trigger signal */

/* AOS_PE_TRGSEL14_TRGSEL */
#define AOS_PE_TRGSEL14_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL14_TRGSEL bit offset */
#define AOS_PE_TRGSEL14_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL14_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL14_TRGSEL_POS))
#define AOS_PE_TRGSEL14_TRGSEL_NONE          (AOS_PE_TRGSEL14_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL14_TRGSEL_KBI0_CH2_OUTPUT  (AOS_PE_TRGSEL14_TRGSEL_CFG(1))                     /*!< KBI0 channel 2 output signal as trigger signal */
#define AOS_PE_TRGSEL14_TRGSEL_KBI0_CH6_OUTPUT  (AOS_PE_TRGSEL14_TRGSEL_CFG(2))                     /*!< KBI0 channel 6 output signal as trigger signal */
#define AOS_PE_TRGSEL14_TRGSEL_KBI1_CH2_OUTPUT  (AOS_PE_TRGSEL14_TRGSEL_CFG(3))                     /*!< KBI1 channel 2 output signal as trigger signal */
#define AOS_PE_TRGSEL14_TRGSEL_KBI1_CH6_OUTPUT  (AOS_PE_TRGSEL14_TRGSEL_CFG(4))                     /*!< KBI1 channel 6 output signal as trigger signal */

/* AOS_PE_TRGSEL15_TRGSEL */
#define AOS_PE_TRGSEL15_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL15_TRGSEL bit offset */
#define AOS_PE_TRGSEL15_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL15_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL15_TRGSEL_POS))
#define AOS_PE_TRGSEL15_TRGSEL_NONE          (AOS_PE_TRGSEL15_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL15_TRGSEL_KBI0_CH3_OUTPUT  (AOS_PE_TRGSEL15_TRGSEL_CFG(1))                     /*!< KBI0 channel 3 output signal as trigger signal */
#define AOS_PE_TRGSEL15_TRGSEL_KBI0_CH7_OUTPUT  (AOS_PE_TRGSEL15_TRGSEL_CFG(2))                     /*!< KBI0 channel 7 output signal as trigger signal */
#define AOS_PE_TRGSEL15_TRGSEL_KBI1_CH3_OUTPUT  (AOS_PE_TRGSEL15_TRGSEL_CFG(3))                     /*!< KBI1 channel 3 output signal as trigger signal */
#define AOS_PE_TRGSEL15_TRGSEL_KBI1_CH7_OUTPUT  (AOS_PE_TRGSEL15_TRGSEL_CFG(4))                     /*!< KBI1 channel 7 output signal as trigger signal */

/* AOS_PE_TRGSEL16_TRGSEL */
#define AOS_PE_TRGSEL16_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL16_TRGSEL bit offset */
#define AOS_PE_TRGSEL16_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL16_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL16_TRGSEL_POS))
#define AOS_PE_TRGSEL16_TRGSEL_NONE          (AOS_PE_TRGSEL16_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL16_TRGSEL_KBI0_CH8_OUTPUT  (AOS_PE_TRGSEL16_TRGSEL_CFG(1))                     /*!< KBI0 channel 8 output signal as trigger signal */
#define AOS_PE_TRGSEL16_TRGSEL_KBI0_CH12_OUTPUT  (AOS_PE_TRGSEL16_TRGSEL_CFG(2))                     /*!< KBI0 channel 12 output signal as trigger signal */
#define AOS_PE_TRGSEL16_TRGSEL_KBI1_CH8_OUTPUT  (AOS_PE_TRGSEL16_TRGSEL_CFG(3))                     /*!< KBI1 channel 8 output signal as trigger signal */
#define AOS_PE_TRGSEL16_TRGSEL_KBI1_CH12_OUTPUT  (AOS_PE_TRGSEL16_TRGSEL_CFG(4))                     /*!< KBI1 channel 12 output signal as trigger signal */

/* AOS_PE_TRGSEL17_TRGSEL */
#define AOS_PE_TRGSEL17_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL17_TRGSEL bit offset */
#define AOS_PE_TRGSEL17_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL17_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL17_TRGSEL_POS))
#define AOS_PE_TRGSEL17_TRGSEL_NONE          (AOS_PE_TRGSEL17_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL17_TRGSEL_KBI0_CH9_OUTPUT  (AOS_PE_TRGSEL17_TRGSEL_CFG(1))                     /*!< KBI0 channel 9 output signal as trigger signal */
#define AOS_PE_TRGSEL17_TRGSEL_KBI0_CH13_OUTPUT  (AOS_PE_TRGSEL17_TRGSEL_CFG(2))                     /*!< KBI0 channel 13 output signal as trigger signal */
#define AOS_PE_TRGSEL17_TRGSEL_KBI1_CH9_OUTPUT  (AOS_PE_TRGSEL17_TRGSEL_CFG(3))                     /*!< KBI1 channel 9 output signal as trigger signal */
#define AOS_PE_TRGSEL17_TRGSEL_KBI1_CH13_OUTPUT  (AOS_PE_TRGSEL17_TRGSEL_CFG(4))                     /*!< KBI1 channel 13 output signal as trigger signal */

/* AOS_PE_TRGSEL18_TRGSEL */
#define AOS_PE_TRGSEL18_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL18_TRGSEL bit offset */
#define AOS_PE_TRGSEL18_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL18_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL18_TRGSEL_POS))
#define AOS_PE_TRGSEL18_TRGSEL_NONE          (AOS_PE_TRGSEL18_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL18_TRGSEL_KBI0_CH10_OUTPUT  (AOS_PE_TRGSEL18_TRGSEL_CFG(1))                     /*!< KBI0 channel 10 output signal as trigger signal */
#define AOS_PE_TRGSEL18_TRGSEL_KBI0_CH14_OUTPUT  (AOS_PE_TRGSEL18_TRGSEL_CFG(2))                     /*!< KBI0 channel 14 output signal as trigger signal */
#define AOS_PE_TRGSEL18_TRGSEL_KBI1_CH10_OUTPUT  (AOS_PE_TRGSEL18_TRGSEL_CFG(3))                     /*!< KBI1 channel 10 output signal as trigger signal */
#define AOS_PE_TRGSEL18_TRGSEL_KBI1_CH14_OUTPUT  (AOS_PE_TRGSEL18_TRGSEL_CFG(4))                     /*!< KBI1 channel 14 output signal as trigger signal */

/* AOS_PE_TRGSEL19_TRGSEL */
#define AOS_PE_TRGSEL19_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL19_TRGSEL bit offset */
#define AOS_PE_TRGSEL19_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL19_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL19_TRGSEL_POS))
#define AOS_PE_TRGSEL19_TRGSEL_NONE          (AOS_PE_TRGSEL19_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL19_TRGSEL_KBI0_CH11_OUTPUT  (AOS_PE_TRGSEL19_TRGSEL_CFG(1))                     /*!< KBI0 channel 11 output signal as trigger signal */
#define AOS_PE_TRGSEL19_TRGSEL_KBI0_CH15_OUTPUT  (AOS_PE_TRGSEL19_TRGSEL_CFG(2))                     /*!< KBI0 channel 15 output signal as trigger signal */
#define AOS_PE_TRGSEL19_TRGSEL_KBI1_CH11_OUTPUT  (AOS_PE_TRGSEL19_TRGSEL_CFG(3))                     /*!< KBI1 channel 11 output signal as trigger signal */
#define AOS_PE_TRGSEL19_TRGSEL_KBI1_CH15_OUTPUT  (AOS_PE_TRGSEL19_TRGSEL_CFG(4))                     /*!< KBI1 channel 15 output signal as trigger signal */

/* AOS_PE_TRGSEL20_TRGSEL */
#define AOS_PE_TRGSEL20_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL20_TRGSEL bit offset */
#define AOS_PE_TRGSEL20_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL20_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL20_TRGSEL_POS))
#define AOS_PE_TRGSEL20_TRGSEL_NONE          (AOS_PE_TRGSEL20_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL20_TRGSEL_KBI0_CH8_OUTPUT  (AOS_PE_TRGSEL20_TRGSEL_CFG(1))                     /*!< KBI0 channel 8 output signal as trigger signal */
#define AOS_PE_TRGSEL20_TRGSEL_KBI0_CH12_OUTPUT  (AOS_PE_TRGSEL20_TRGSEL_CFG(2))                     /*!< KBI0 channel 12 output signal as trigger signal */
#define AOS_PE_TRGSEL20_TRGSEL_KBI1_CH8_OUTPUT  (AOS_PE_TRGSEL20_TRGSEL_CFG(3))                     /*!< KBI1 channel 8 output signal as trigger signal */
#define AOS_PE_TRGSEL20_TRGSEL_KBI1_CH12_OUTPUT  (AOS_PE_TRGSEL20_TRGSEL_CFG(4))                     /*!< KBI1 channel 12 output signal as trigger signal */

/* AOS_PE_TRGSEL21_TRGSEL */
#define AOS_PE_TRGSEL21_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL21_TRGSEL bit offset */
#define AOS_PE_TRGSEL21_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL21_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL21_TRGSEL_POS))
#define AOS_PE_TRGSEL21_TRGSEL_NONE          (AOS_PE_TRGSEL21_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL21_TRGSEL_KBI0_CH9_OUTPUT  (AOS_PE_TRGSEL21_TRGSEL_CFG(1))                     /*!< KBI0 channel 9 output signal as trigger signal */
#define AOS_PE_TRGSEL21_TRGSEL_KBI0_CH13_OUTPUT  (AOS_PE_TRGSEL21_TRGSEL_CFG(2))                     /*!< KBI0 channel 13 output signal as trigger signal */
#define AOS_PE_TRGSEL21_TRGSEL_KBI1_CH9_OUTPUT  (AOS_PE_TRGSEL21_TRGSEL_CFG(3))                     /*!< KBI1 channel 9 output signal as trigger signal */
#define AOS_PE_TRGSEL21_TRGSEL_KBI1_CH13_OUTPUT  (AOS_PE_TRGSEL21_TRGSEL_CFG(4))                     /*!< KBI1 channel 13 output signal as trigger signal */

/* AOS_PE_TRGSEL22_TRGSEL */
#define AOS_PE_TRGSEL22_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL22_TRGSEL bit offset */
#define AOS_PE_TRGSEL22_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL22_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL22_TRGSEL_POS))
#define AOS_PE_TRGSEL22_TRGSEL_NONE          (AOS_PE_TRGSEL22_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL22_TRGSEL_KBI0_CH10_OUTPUT  (AOS_PE_TRGSEL22_TRGSEL_CFG(1))                     /*!< KBI0 channel 10 output signal as trigger signal */
#define AOS_PE_TRGSEL22_TRGSEL_KBI0_CH14_OUTPUT  (AOS_PE_TRGSEL22_TRGSEL_CFG(2))                     /*!< KBI0 channel 14 output signal as trigger signal */
#define AOS_PE_TRGSEL22_TRGSEL_KBI1_CH10_OUTPUT  (AOS_PE_TRGSEL22_TRGSEL_CFG(3))                     /*!< KBI1 channel 10 output signal as trigger signal */
#define AOS_PE_TRGSEL22_TRGSEL_KBI1_CH14_OUTPUT  (AOS_PE_TRGSEL22_TRGSEL_CFG(4))                     /*!< KBI1 channel 14 output signal as trigger signal */

/* AOS_PE_TRGSEL23_TRGSEL */
#define AOS_PE_TRGSEL23_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL23_TRGSEL bit offset */
#define AOS_PE_TRGSEL23_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL23_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL23_TRGSEL_POS))
#define AOS_PE_TRGSEL23_TRGSEL_NONE          (AOS_PE_TRGSEL23_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL23_TRGSEL_KBI0_CH11_OUTPUT  (AOS_PE_TRGSEL23_TRGSEL_CFG(1))                     /*!< KBI0 channel 11 output signal as trigger signal */
#define AOS_PE_TRGSEL23_TRGSEL_KBI0_CH15_OUTPUT  (AOS_PE_TRGSEL23_TRGSEL_CFG(2))                     /*!< KBI0 channel 15 output signal as trigger signal */
#define AOS_PE_TRGSEL23_TRGSEL_KBI1_CH11_OUTPUT  (AOS_PE_TRGSEL23_TRGSEL_CFG(3))                     /*!< KBI1 channel 11 output signal as trigger signal */
#define AOS_PE_TRGSEL23_TRGSEL_KBI1_CH15_OUTPUT  (AOS_PE_TRGSEL23_TRGSEL_CFG(4))                     /*!< KBI1 channel 15 output signal as trigger signal */

/* AOS_PE_TRGSEL24_TRGSEL */
#define AOS_PE_TRGSEL24_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL24_TRGSEL bit offset */
#define AOS_PE_TRGSEL24_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL24_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL24_TRGSEL_POS))
#define AOS_PE_TRGSEL24_TRGSEL_NONE          (AOS_PE_TRGSEL24_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL24_TRGSEL_SIMPTC0_CH0_OVL  (AOS_PE_TRGSEL24_TRGSEL_CFG(1))                     /*!< SIMPTC0 channel 0 overflow signal as trigger signal */
#define AOS_PE_TRGSEL24_TRGSEL_SIMPTC1_CH0_OVL  (AOS_PE_TRGSEL24_TRGSEL_CFG(2))                     /*!< SIMPTC1 channel 0 overflow signal as trigger signal */
#define AOS_PE_TRGSEL24_TRGSEL_SIMPTC2_CH0_OVL  (AOS_PE_TRGSEL24_TRGSEL_CFG(3))                     /*!< SIMPTC2 channel 0 overflow signal as trigger signal */
#define AOS_PE_TRGSEL24_TRGSEL_SIMPTC3_CH0_OVL  (AOS_PE_TRGSEL24_TRGSEL_CFG(4))                     /*!< SIMPTC3 channel 0 overflow signal as trigger signal */

/* AOS_PE_TRGSEL25_TRGSEL */
#define AOS_PE_TRGSEL25_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL25_TRGSEL bit offset */
#define AOS_PE_TRGSEL25_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL25_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL25_TRGSEL_POS))
#define AOS_PE_TRGSEL25_TRGSEL_NONE          (AOS_PE_TRGSEL25_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_UART6_PF_OUTPUT  (AOS_PE_TRGSEL25_TRGSEL_CFG(1))                     /*!< UART6 PF output signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_UART6_QF_OUTPUT  (AOS_PE_TRGSEL25_TRGSEL_CFG(2))                     /*!< UART6 QF output signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_UART6_FPF_OUTPUT  (AOS_PE_TRGSEL25_TRGSEL_CFG(3))                     /*!< UART6 FPF output signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_UART7_PF_OUTPUT  (AOS_PE_TRGSEL25_TRGSEL_CFG(4))                     /*!< UART7 PF output signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_UART7_QF_OUTPUT  (AOS_PE_TRGSEL25_TRGSEL_CFG(5))                     /*!< UART7 QF output signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_UART7_FPF_OUTPUT  (AOS_PE_TRGSEL25_TRGSEL_CFG(6))                     /*!< UART7 FPF output signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_D2F_CF_OUT0   (AOS_PE_TRGSEL25_TRGSEL_CFG(7))                     /*!< D2F CF OUT0 output signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_D2F_CF_OUT1   (AOS_PE_TRGSEL25_TRGSEL_CFG(8))                     /*!< D2F CF OUT1 output signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_D2F_CF_OUT2   (AOS_PE_TRGSEL25_TRGSEL_CFG(9))                     /*!< D2F CF OUT2 output signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_KBI0_CH0_INPUT  (AOS_PE_TRGSEL25_TRGSEL_CFG(10))                    /*!< KBI0 channel 0 input signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_KBI0_CH1_INPUT  (AOS_PE_TRGSEL25_TRGSEL_CFG(11))                    /*!< KBI0 channel 1 input signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_KBI0_CH2_INPUT  (AOS_PE_TRGSEL25_TRGSEL_CFG(12))                    /*!< KBI0 channel 2 input signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_KBI0_CH3_INPUT  (AOS_PE_TRGSEL25_TRGSEL_CFG(13))                    /*!< KBI0 channel 3 input signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_KBI1_CH0_INPUT  (AOS_PE_TRGSEL25_TRGSEL_CFG(14))                    /*!< KBI1 channel 0 input signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_KBI1_CH1_INPUT  (AOS_PE_TRGSEL25_TRGSEL_CFG(15))                    /*!< KBI1 channel 1 input signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_KBI1_CH2_INPUT  (AOS_PE_TRGSEL25_TRGSEL_CFG(16))                    /*!< KBI1 channel 2 input signal as trigger signal */
#define AOS_PE_TRGSEL25_TRGSEL_KBI1_CH3_INPUT  (AOS_PE_TRGSEL25_TRGSEL_CFG(17))                    /*!< KBI1 channel 3 input signal as trigger signal */

/* AOS_PE_TRGSEL26_TRGSEL */
#define AOS_PE_TRGSEL26_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL26_TRGSEL bit offset */
#define AOS_PE_TRGSEL26_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL26_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL26_TRGSEL_POS))
#define AOS_PE_TRGSEL26_TRGSEL_NONE          (AOS_PE_TRGSEL26_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_UART6_PF_OUTPUT  (AOS_PE_TRGSEL26_TRGSEL_CFG(1))                     /*!< UART6 PF output signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_UART6_QF_OUTPUT  (AOS_PE_TRGSEL26_TRGSEL_CFG(2))                     /*!< UART6 QF output signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_UART6_FPF_OUTPUT  (AOS_PE_TRGSEL26_TRGSEL_CFG(3))                     /*!< UART6 FPF output signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_UART7_PF_OUTPUT  (AOS_PE_TRGSEL26_TRGSEL_CFG(4))                     /*!< UART7 PF output signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_UART7_QF_OUTPUT  (AOS_PE_TRGSEL26_TRGSEL_CFG(5))                     /*!< UART7 QF output signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_UART7_FPF_OUTPUT  (AOS_PE_TRGSEL26_TRGSEL_CFG(6))                     /*!< UART7 FPF output signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_D2F_CF_OUT0   (AOS_PE_TRGSEL26_TRGSEL_CFG(7))                     /*!< D2F CF OUT0 output signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_D2F_CF_OUT1   (AOS_PE_TRGSEL26_TRGSEL_CFG(8))                     /*!< D2F CF OUT1 output signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_D2F_CF_OUT2   (AOS_PE_TRGSEL26_TRGSEL_CFG(9))                     /*!< D2F CF OUT2 output signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_KBI0_CH4_INPUT  (AOS_PE_TRGSEL26_TRGSEL_CFG(10))                    /*!< KBI0 channel 4 input signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_KBI0_CH5_INPUT  (AOS_PE_TRGSEL26_TRGSEL_CFG(11))                    /*!< KBI0 channel 5 input signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_KBI0_CH6_INPUT  (AOS_PE_TRGSEL26_TRGSEL_CFG(12))                    /*!< KBI0 channel 6 input signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_KBI0_CH7_INPUT  (AOS_PE_TRGSEL26_TRGSEL_CFG(13))                    /*!< KBI0 channel 7 input signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_KBI1_CH4_INPUT  (AOS_PE_TRGSEL26_TRGSEL_CFG(14))                    /*!< KBI1 channel 4 input signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_KBI1_CH5_INPUT  (AOS_PE_TRGSEL26_TRGSEL_CFG(15))                    /*!< KBI1 channel 5 input signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_KBI1_CH6_INPUT  (AOS_PE_TRGSEL26_TRGSEL_CFG(16))                    /*!< KBI1 channel 6 input signal as trigger signal */
#define AOS_PE_TRGSEL26_TRGSEL_KBI1_CH7_INPUT  (AOS_PE_TRGSEL26_TRGSEL_CFG(17))                    /*!< KBI1 channel 7 input signal as trigger signal */

/* AOS_PE_TRGSEL27_TRGSEL */
#define AOS_PE_TRGSEL27_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL27_TRGSEL bit offset */
#define AOS_PE_TRGSEL27_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL27_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL27_TRGSEL_POS))
#define AOS_PE_TRGSEL27_TRGSEL_NONE          (AOS_PE_TRGSEL27_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_UART6_PF_OUTPUT  (AOS_PE_TRGSEL27_TRGSEL_CFG(1))                     /*!< UART6 PF output signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_UART6_QF_OUTPUT  (AOS_PE_TRGSEL27_TRGSEL_CFG(2))                     /*!< UART6 QF output signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_UART6_FPF_OUTPUT  (AOS_PE_TRGSEL27_TRGSEL_CFG(3))                     /*!< UART6 FPF output signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_UART7_PF_OUTPUT  (AOS_PE_TRGSEL27_TRGSEL_CFG(4))                     /*!< UART7 PF output signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_UART7_QF_OUTPUT  (AOS_PE_TRGSEL27_TRGSEL_CFG(5))                     /*!< UART7 QF output signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_UART7_FPF_OUTPUT  (AOS_PE_TRGSEL27_TRGSEL_CFG(6))                     /*!< UART7 FPF output signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_D2F_CF_OUT0   (AOS_PE_TRGSEL27_TRGSEL_CFG(7))                     /*!< D2F CF OUT0 output signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_D2F_CF_OUT1   (AOS_PE_TRGSEL27_TRGSEL_CFG(8))                     /*!< D2F CF OUT1 output signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_D2F_CF_OUT2   (AOS_PE_TRGSEL27_TRGSEL_CFG(9))                     /*!< D2F CF OUT2 output signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_KBI0_CH8_INPUT  (AOS_PE_TRGSEL27_TRGSEL_CFG(10))                    /*!< KBI0 channel 8 input signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_KBI0_CH9_INPUT  (AOS_PE_TRGSEL27_TRGSEL_CFG(11))                    /*!< KBI0 channel 9 input signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_KBI0_CH10_INPUT  (AOS_PE_TRGSEL27_TRGSEL_CFG(12))                    /*!< KBI0 channel 10 input signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_KBI0_CH11_INPUT  (AOS_PE_TRGSEL27_TRGSEL_CFG(13))                    /*!< KBI0 channel 11 input signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_KBI1_CH8_INPUT  (AOS_PE_TRGSEL27_TRGSEL_CFG(14))                    /*!< KBI1 channel 8 input signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_KBI1_CH9_INPUT  (AOS_PE_TRGSEL27_TRGSEL_CFG(15))                    /*!< KBI1 channel 9 input signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_KBI1_CH10_INPUT  (AOS_PE_TRGSEL27_TRGSEL_CFG(16))                    /*!< KBI1 channel 10 input signal as trigger signal */
#define AOS_PE_TRGSEL27_TRGSEL_KBI1_CH11_INPUT  (AOS_PE_TRGSEL27_TRGSEL_CFG(17))                    /*!< KBI1 channel 11 input signal as trigger signal */

/* AOS_PE_TRGSEL28_TRGSEL */
#define AOS_PE_TRGSEL28_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL28_TRGSEL bit offset */
#define AOS_PE_TRGSEL28_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL28_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL28_TRGSEL_POS))
#define AOS_PE_TRGSEL28_TRGSEL_NONE          (AOS_PE_TRGSEL28_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_UART6_PF_OUTPUT  (AOS_PE_TRGSEL28_TRGSEL_CFG(1))                     /*!< UART6 PF output signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_UART6_QF_OUTPUT  (AOS_PE_TRGSEL28_TRGSEL_CFG(2))                     /*!< UART6 QF output signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_UART6_FPF_OUTPUT  (AOS_PE_TRGSEL28_TRGSEL_CFG(3))                     /*!< UART6 FPF output signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_UART7_PF_OUTPUT  (AOS_PE_TRGSEL28_TRGSEL_CFG(4))                     /*!< UART7 PF output signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_UART7_QF_OUTPUT  (AOS_PE_TRGSEL28_TRGSEL_CFG(5))                     /*!< UART7 QF output signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_UART7_FPF_OUTPUT  (AOS_PE_TRGSEL28_TRGSEL_CFG(6))                     /*!< UART7 FPF output signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_D2F_CF_OUT0   (AOS_PE_TRGSEL28_TRGSEL_CFG(7))                     /*!< D2F CF OUT0 output signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_D2F_CF_OUT1   (AOS_PE_TRGSEL28_TRGSEL_CFG(8))                     /*!< D2F CF OUT1 output signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_D2F_CF_OUT2   (AOS_PE_TRGSEL28_TRGSEL_CFG(9))                     /*!< D2F CF OUT2 output signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_KBI0_CH12_INPUT  (AOS_PE_TRGSEL28_TRGSEL_CFG(10))                    /*!< KBI0 channel 12 input signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_KBI0_CH13_INPUT  (AOS_PE_TRGSEL28_TRGSEL_CFG(11))                    /*!< KBI0 channel 13 input signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_KBI0_CH14_INPUT  (AOS_PE_TRGSEL28_TRGSEL_CFG(12))                    /*!< KBI0 channel 14 input signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_KBI0_CH15_INPUT  (AOS_PE_TRGSEL28_TRGSEL_CFG(13))                    /*!< KBI0 channel 15 input signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_KBI1_CH12_INPUT  (AOS_PE_TRGSEL28_TRGSEL_CFG(14))                    /*!< KBI1 channel 12 input signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_KBI1_CH13_INPUT  (AOS_PE_TRGSEL28_TRGSEL_CFG(15))                    /*!< KBI1 channel 13 input signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_KBI1_CH14_INPUT  (AOS_PE_TRGSEL28_TRGSEL_CFG(16))                    /*!< KBI1 channel 14 input signal as trigger signal */
#define AOS_PE_TRGSEL28_TRGSEL_KBI1_CH15_INPUT  (AOS_PE_TRGSEL28_TRGSEL_CFG(17))                    /*!< KBI1 channel 15 input signal as trigger signal */

/* AOS_PE_TRGSEL29_TRGSEL */
#define AOS_PE_TRGSEL29_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL29_TRGSEL bit offset */
#define AOS_PE_TRGSEL29_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL29_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL29_TRGSEL_POS))
#define AOS_PE_TRGSEL29_TRGSEL_NONE          (AOS_PE_TRGSEL29_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_UART6_PF_OUTPUT  (AOS_PE_TRGSEL29_TRGSEL_CFG(1))                     /*!< UART6 PF output signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_UART6_QF_OUTPUT  (AOS_PE_TRGSEL29_TRGSEL_CFG(2))                     /*!< UART6 QF output signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_UART6_FPF_OUTPUT  (AOS_PE_TRGSEL29_TRGSEL_CFG(3))                     /*!< UART6 FPF output signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_UART7_PF_OUTPUT  (AOS_PE_TRGSEL29_TRGSEL_CFG(4))                     /*!< UART7 PF output signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_UART7_QF_OUTPUT  (AOS_PE_TRGSEL29_TRGSEL_CFG(5))                     /*!< UART7 QF output signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_UART7_FPF_OUTPUT  (AOS_PE_TRGSEL29_TRGSEL_CFG(6))                     /*!< UART7 FPF output signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_D2F_CF_OUT0   (AOS_PE_TRGSEL29_TRGSEL_CFG(7))                     /*!< D2F CF OUT0 output signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_D2F_CF_OUT1   (AOS_PE_TRGSEL29_TRGSEL_CFG(8))                     /*!< D2F CF OUT1 output signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_D2F_CF_OUT2   (AOS_PE_TRGSEL29_TRGSEL_CFG(9))                     /*!< D2F CF OUT2 output signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_KBI0_CH0_INPUT  (AOS_PE_TRGSEL29_TRGSEL_CFG(10))                    /*!< KBI0 channel 0 input signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_KBI0_CH1_INPUT  (AOS_PE_TRGSEL29_TRGSEL_CFG(11))                    /*!< KBI0 channel 1 input signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_KBI0_CH2_INPUT  (AOS_PE_TRGSEL29_TRGSEL_CFG(12))                    /*!< KBI0 channel 2 input signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_KBI0_CH3_INPUT  (AOS_PE_TRGSEL29_TRGSEL_CFG(13))                    /*!< KBI0 channel 3 input signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_KBI1_CH0_INPUT  (AOS_PE_TRGSEL29_TRGSEL_CFG(14))                    /*!< KBI1 channel 0 input signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_KBI1_CH1_INPUT  (AOS_PE_TRGSEL29_TRGSEL_CFG(15))                    /*!< KBI1 channel 1 input signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_KBI1_CH2_INPUT  (AOS_PE_TRGSEL29_TRGSEL_CFG(16))                    /*!< KBI1 channel 2 input signal as trigger signal */
#define AOS_PE_TRGSEL29_TRGSEL_KBI1_CH3_INPUT  (AOS_PE_TRGSEL29_TRGSEL_CFG(17))                    /*!< KBI1 channel 3 input signal as trigger signal */

/* AOS_PE_TRGSEL30_TRGSEL */
#define AOS_PE_TRGSEL30_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL30_TRGSEL bit offset */
#define AOS_PE_TRGSEL30_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL30_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL30_TRGSEL_POS))
#define AOS_PE_TRGSEL30_TRGSEL_NONE          (AOS_PE_TRGSEL30_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL30_TRGSEL_KBI0_CH0_OUTPUT  (AOS_PE_TRGSEL30_TRGSEL_CFG(1))                     /*!< KBI0 channel 0 output signal as trigger signal */
#define AOS_PE_TRGSEL30_TRGSEL_KBI0_CH1_OUTPUT  (AOS_PE_TRGSEL30_TRGSEL_CFG(2))                     /*!< KBI0 channel 1 output signal as trigger signal */
#define AOS_PE_TRGSEL30_TRGSEL_KBI0_CH2_OUTPUT  (AOS_PE_TRGSEL30_TRGSEL_CFG(3))                     /*!< KBI0 channel 2 output signal as trigger signal */
#define AOS_PE_TRGSEL30_TRGSEL_KBI0_CH3_OUTPUT  (AOS_PE_TRGSEL30_TRGSEL_CFG(4))                     /*!< KBI0 channel 3 output signal as trigger signal */
#define AOS_PE_TRGSEL30_TRGSEL_KBI1_CH0_OUTPUT  (AOS_PE_TRGSEL30_TRGSEL_CFG(5))                     /*!< KBI1 channel 0 output signal as trigger signal */
#define AOS_PE_TRGSEL30_TRGSEL_KBI1_CH1_OUTPUT  (AOS_PE_TRGSEL30_TRGSEL_CFG(6))                     /*!< KBI1 channel 1 output signal as trigger signal */
#define AOS_PE_TRGSEL30_TRGSEL_KBI1_CH2_OUTPUT  (AOS_PE_TRGSEL30_TRGSEL_CFG(7))                     /*!< KBI1 channel 2 output signal as trigger signal */
#define AOS_PE_TRGSEL30_TRGSEL_KBI1_CH3_OUTPUT  (AOS_PE_TRGSEL30_TRGSEL_CFG(8))                     /*!< KBI1 channel 3 output signal as trigger signal */
#define AOS_PE_TRGSEL30_TRGSEL_SIMPTC0_CH0_OVL  (AOS_PE_TRGSEL30_TRGSEL_CFG(9))                     /*!< SIMPTC0 channel 0 overflow signal as trigger signal */
#define AOS_PE_TRGSEL30_TRGSEL_SIMPTC1_CH0_OVL  (AOS_PE_TRGSEL30_TRGSEL_CFG(10))                    /*!< SIMPTC1 channel 0 overflow signal as trigger signal */
#define AOS_PE_TRGSEL30_TRGSEL_SIMPTC2_CH0_OVL  (AOS_PE_TRGSEL30_TRGSEL_CFG(11))                    /*!< SIMPTC2 channel 0 overflow signal as trigger signal */
#define AOS_PE_TRGSEL30_TRGSEL_SIMPTC3_CH0_OVL  (AOS_PE_TRGSEL30_TRGSEL_CFG(12))                    /*!< SIMPTC3 channel 0 overflow signal as trigger signal */

/* AOS_PE_TRGSEL31_TRGSEL */
#define AOS_PE_TRGSEL31_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL31_TRGSEL bit offset */
#define AOS_PE_TRGSEL31_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL31_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL31_TRGSEL_POS))
#define AOS_PE_TRGSEL31_TRGSEL_NONE          (AOS_PE_TRGSEL31_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_SIMPTC0_CH2_OVL  (AOS_PE_TRGSEL31_TRGSEL_CFG(1))                     /*!< SIMPTC0 channel 2 count overflow signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_SIMPTC0_CH3_OVL  (AOS_PE_TRGSEL31_TRGSEL_CFG(2))                     /*!< SIMPTC0 channel 3 count overflow signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_SIMPTC1_CH2_OVL  (AOS_PE_TRGSEL31_TRGSEL_CFG(3))                     /*!< SIMPTC1 channel 2 count overflow signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_SIMPTC1_CH3_OVL  (AOS_PE_TRGSEL31_TRGSEL_CFG(4))                     /*!< SIMPTC1 channel 3 count overflow signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_SIMPTC2_CH2_OVL  (AOS_PE_TRGSEL31_TRGSEL_CFG(5))                     /*!< SIMPTC2 channel 2 count overflow signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_SIMPTC2_CH3_OVL  (AOS_PE_TRGSEL31_TRGSEL_CFG(6))                     /*!< SIMPTC2 channel 3 count overflow signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_SIMPTC3_CH2_OVL  (AOS_PE_TRGSEL31_TRGSEL_CFG(7))                     /*!< SIMPTC3 channel 2 count overflow signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_SIMPTC3_CH3_OVL  (AOS_PE_TRGSEL31_TRGSEL_CFG(8))                     /*!< SIMPTC3 channel 3 count overflow signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN0  (AOS_PE_TRGSEL31_TRGSEL_CFG(9))                     /*!< GPIO TC PIN0 signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN1  (AOS_PE_TRGSEL31_TRGSEL_CFG(10))                    /*!< GPIO TC PIN1 signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN2  (AOS_PE_TRGSEL31_TRGSEL_CFG(11))                    /*!< GPIO TC PIN2 signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN3  (AOS_PE_TRGSEL31_TRGSEL_CFG(12))                    /*!< GPIO TC PIN3 signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN4  (AOS_PE_TRGSEL31_TRGSEL_CFG(13))                    /*!< GPIO TC PIN4 signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN5  (AOS_PE_TRGSEL31_TRGSEL_CFG(14))                    /*!< GPIO TC PIN5 signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN6  (AOS_PE_TRGSEL31_TRGSEL_CFG(15))                    /*!< GPIO TC PIN6 signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN7  (AOS_PE_TRGSEL31_TRGSEL_CFG(16))                    /*!< GPIO TC PIN7 signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN8  (AOS_PE_TRGSEL31_TRGSEL_CFG(17))                    /*!< GPIO TC PIN8 signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN9  (AOS_PE_TRGSEL31_TRGSEL_CFG(18))                    /*!< GPIO TC PIN9 signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN10  (AOS_PE_TRGSEL31_TRGSEL_CFG(19))                    /*!< GPIO TC PIN10 signal as trigger signal */
#define AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN11  (AOS_PE_TRGSEL31_TRGSEL_CFG(20))                    /*!< GPIO TC PIN11 signal as trigger signal */

/* AOS_PE_TRGSEL32_TRGSEL */
#define AOS_PE_TRGSEL32_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL32_TRGSEL bit offset */
#define AOS_PE_TRGSEL32_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL32_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL32_TRGSEL_POS))
#define AOS_PE_TRGSEL32_TRGSEL_NONE          (AOS_PE_TRGSEL32_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL32_TRGSEL_KBI0_CH0_OUTPUT  (AOS_PE_TRGSEL32_TRGSEL_CFG(1))                     /*!< KBI0 channel 0 output signal as trigger signal */
#define AOS_PE_TRGSEL32_TRGSEL_KBI0_CH1_OUTPUT  (AOS_PE_TRGSEL32_TRGSEL_CFG(2))                     /*!< KBI0 channel 1 output signal as trigger signal */
#define AOS_PE_TRGSEL32_TRGSEL_KBI0_CH2_OUTPUT  (AOS_PE_TRGSEL32_TRGSEL_CFG(3))                     /*!< KBI0 channel 2 output signal as trigger signal */
#define AOS_PE_TRGSEL32_TRGSEL_KBI0_CH3_OUTPUT  (AOS_PE_TRGSEL32_TRGSEL_CFG(4))                     /*!< KBI0 channel 3 output signal as trigger signal */
#define AOS_PE_TRGSEL32_TRGSEL_KBI1_CH0_OUTPUT  (AOS_PE_TRGSEL32_TRGSEL_CFG(5))                     /*!< KBI1 channel 0 output signal as trigger signal */
#define AOS_PE_TRGSEL32_TRGSEL_KBI1_CH1_OUTPUT  (AOS_PE_TRGSEL32_TRGSEL_CFG(6))                     /*!< KBI1 channel 1 output signal as trigger signal */
#define AOS_PE_TRGSEL32_TRGSEL_KBI1_CH2_OUTPUT  (AOS_PE_TRGSEL32_TRGSEL_CFG(7))                     /*!< KBI1 channel 2 output signal as trigger signal */
#define AOS_PE_TRGSEL32_TRGSEL_KBI1_CH3_OUTPUT  (AOS_PE_TRGSEL32_TRGSEL_CFG(8))                     /*!< KBI1 channel 3 output signal as trigger signal */
#define AOS_PE_TRGSEL32_TRGSEL_SIMPTC0_CH0_OVL  (AOS_PE_TRGSEL32_TRGSEL_CFG(9))                     /*!< SIMPTC0 channel 0 count overflow signal as trigger signal */
#define AOS_PE_TRGSEL32_TRGSEL_SIMPTC1_CH0_OVL  (AOS_PE_TRGSEL32_TRGSEL_CFG(10))                    /*!< SIMPTC1 channel 0 count overflow signal as trigger signal */
#define AOS_PE_TRGSEL32_TRGSEL_SIMPTC2_CH0_OVL  (AOS_PE_TRGSEL32_TRGSEL_CFG(11))                    /*!< SIMPTC2 channel 0 count overflow signal as trigger signal */
#define AOS_PE_TRGSEL32_TRGSEL_SIMPTC3_CH0_OVL  (AOS_PE_TRGSEL32_TRGSEL_CFG(12))                    /*!< SIMPTC3 channel 0 count overflow signal as trigger signal */

/* AOS_PE_TRGSEL33_TRGSEL */
#define AOS_PE_TRGSEL33_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL33_TRGSEL bit offset */
#define AOS_PE_TRGSEL33_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL33_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL33_TRGSEL_POS))
#define AOS_PE_TRGSEL33_TRGSEL_NONE          (AOS_PE_TRGSEL33_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL33_TRGSEL_KBI0_CH4_OUTPUT  (AOS_PE_TRGSEL33_TRGSEL_CFG(1))                     /*!< KBI0 channel 4 output signal as trigger signal */
#define AOS_PE_TRGSEL33_TRGSEL_KBI0_CH5_OUTPUT  (AOS_PE_TRGSEL33_TRGSEL_CFG(2))                     /*!< KBI0 channel 5 output signal as trigger signal */
#define AOS_PE_TRGSEL33_TRGSEL_KBI0_CH6_OUTPUT  (AOS_PE_TRGSEL33_TRGSEL_CFG(3))                     /*!< KBI0 channel 6 output signal as trigger signal */
#define AOS_PE_TRGSEL33_TRGSEL_KBI0_CH7_OUTPUT  (AOS_PE_TRGSEL33_TRGSEL_CFG(4))                     /*!< KBI0 channel 7 output signal as trigger signal */
#define AOS_PE_TRGSEL33_TRGSEL_KBI1_CH4_OUTPUT  (AOS_PE_TRGSEL33_TRGSEL_CFG(5))                     /*!< KBI1 channel 4 output signal as trigger signal */
#define AOS_PE_TRGSEL33_TRGSEL_KBI1_CH5_OUTPUT  (AOS_PE_TRGSEL33_TRGSEL_CFG(6))                     /*!< KBI1 channel 5 output signal as trigger signal */
#define AOS_PE_TRGSEL33_TRGSEL_KBI1_CH6_OUTPUT  (AOS_PE_TRGSEL33_TRGSEL_CFG(7))                     /*!< KBI1 channel 6 output signal as trigger signal */
#define AOS_PE_TRGSEL33_TRGSEL_KBI1_CH7_OUTPUT  (AOS_PE_TRGSEL33_TRGSEL_CFG(8))                     /*!< KBI1 channel 7 output signal as trigger signal */
#define AOS_PE_TRGSEL33_TRGSEL_SIMPTC0_CH1_OVL  (AOS_PE_TRGSEL33_TRGSEL_CFG(9))                     /*!< SIMPTC0 channel 1 overflow signal as trigger signal */
#define AOS_PE_TRGSEL33_TRGSEL_SIMPTC1_CH1_OVL  (AOS_PE_TRGSEL33_TRGSEL_CFG(10))                    /*!< SIMPTC1 channel 1 overflow signal as trigger signal */
#define AOS_PE_TRGSEL33_TRGSEL_SIMPTC2_CH1_OVL  (AOS_PE_TRGSEL33_TRGSEL_CFG(11))                    /*!< SIMPTC2 channel 1 overflow signal as trigger signal */
#define AOS_PE_TRGSEL33_TRGSEL_SIMPTC3_CH1_OVL  (AOS_PE_TRGSEL33_TRGSEL_CFG(12))                    /*!< SIMPTC3 channel 1 overflow signal as trigger signal */

/* AOS_PE_TRGSEL34_TRGSEL */
#define AOS_PE_TRGSEL34_TRGSEL_POS           (0U)                                                /*!< AOS_PE_TRGSEL34_TRGSEL bit offset */
#define AOS_PE_TRGSEL34_TRGSEL_CFG(regval)  (AOS_PE_TRGSEL34_TRGSEL & ((uint32_t)(regval) << AOS_PE_TRGSEL34_TRGSEL_POS))
#define AOS_PE_TRGSEL34_TRGSEL_NONE          (AOS_PE_TRGSEL34_TRGSEL_CFG(0))                     /*!< No trigger signal */
#define AOS_PE_TRGSEL34_TRGSEL_KBI0_CH8_OUTPUT  (AOS_PE_TRGSEL34_TRGSEL_CFG(1))                     /*!< KBI0 channel 8 output signal as trigger signal */
#define AOS_PE_TRGSEL34_TRGSEL_KBI0_CH9_OUTPUT  (AOS_PE_TRGSEL34_TRGSEL_CFG(2))                     /*!< KBI0 channel 9 output signal as trigger signal */
#define AOS_PE_TRGSEL34_TRGSEL_KBI0_CH10_OUTPUT  (AOS_PE_TRGSEL34_TRGSEL_CFG(3))                     /*!< KBI0 channel 10 output signal as trigger signal */
#define AOS_PE_TRGSEL34_TRGSEL_KBI0_CH11_OUTPUT  (AOS_PE_TRGSEL34_TRGSEL_CFG(4))                     /*!< KBI0 channel 11 output signal as trigger signal */
#define AOS_PE_TRGSEL34_TRGSEL_KBI1_CH8_OUTPUT  (AOS_PE_TRGSEL34_TRGSEL_CFG(5))                     /*!< KBI1 channel 8 output signal as trigger signal */
#define AOS_PE_TRGSEL34_TRGSEL_KBI1_CH9_OUTPUT  (AOS_PE_TRGSEL34_TRGSEL_CFG(6))                     /*!< KBI1 channel 9 output signal as trigger signal */
#define AOS_PE_TRGSEL34_TRGSEL_KBI1_CH10_OUTPUT  (AOS_PE_TRGSEL34_TRGSEL_CFG(7))                     /*!< KBI1 channel 10 output signal as trigger signal */
#define AOS_PE_TRGSEL34_TRGSEL_KBI1_CH11_OUTPUT  (AOS_PE_TRGSEL34_TRGSEL_CFG(8))                     /*!< KBI1 channel 11 output signal as trigger signal */
#define AOS_PE_TRGSEL34_TRGSEL_SIMPTC0_CH2_OVL  (AOS_PE_TRGSEL34_TRGSEL_CFG(9))                     /*!< SIMPTC0 channel 2 count overflow signal as trigger signal */
#define AOS_PE_TRGSEL34_TRGSEL_SIMPTC1_CH2_OVL  (AOS_PE_TRGSEL34_TRGSEL_CFG(10))                    /*!< SIMPTC1 channel 2 count overflow signal as trigger signal */
#define AOS_PE_TRGSEL34_TRGSEL_SIMPTC2_CH2_OVL  (AOS_PE_TRGSEL34_TRGSEL_CFG(11))                    /*!< SIMPTC2 channel 2 count overflow signal as trigger signal */
#define AOS_PE_TRGSEL34_TRGSEL_SIMPTC3_CH2_OVL  (AOS_PE_TRGSEL34_TRGSEL_CFG(12))                    /*!< SIMPTC3 channel 2 count overflow signal as trigger signal */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_AOS_H */
