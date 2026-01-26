/**
 *****************************************************************************
 * @file   rn_rv_ll_aos.h
 * @author Renergy Software Team
 * @brief  Header file of AOS LL module.
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
#ifndef __RN_LL_AOS_H
#define __RN_LL_AOS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_aos.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (AOS)

/** @defgroup AOS_LL AOS
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup AOS_LL_Private_Constants AOS Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup AOS_LL_Private_Macros AOS Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup AOS_LL_ES_INIT AOS Exported Init structure
 * @{
 */

/**
 * @brief LL AOS Init Structure definition
 */
typedef struct
{
    uint32_t TargetPurpose;       /*!< Specifies the target purpose.
                                     This parameter can be a value of @ref AOS_LL_EC_TARGET_PURPOSE.

                                     This feature can be modified afterwards using unitary function @ref LL_AOS_SetTriggerSignal_TC0() and etc.. */

    uint32_t TriggerSignal;       /*!< Specifies the Trigger Signal source.
                                     This parameter can be a value of @ref AOS_LL_EC_TARGET_0_3_TC0_TRGSEL etc..

                                     This feature can be modified afterwards using unitary function @ref LL_AOS_SetTriggerSignal_TC0() and etc.. */
} LL_AOS_InitTypeDef;

/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/* Exported constants -------------------------------------------------------*/
/** @defgroup AOS_LL_Exported_Constants AOS Exported Constants
 * @{
 */

/** @defgroup AOS_LL_EC_TARGET_PURPOSE Target purpose
 * @{
 */
#define LL_AOS_TARGET_0_TC0                   (0)             /*!< Target 0 for TC0 External input clock source                      */
#define LL_AOS_TARGET_1_TC0                   (1)             /*!< Target 1 for TC0 External input events in slave mode              */
#define LL_AOS_TARGET_2_TC0                   (2)             /*!< Target 2 for TC0 Channel 0 captures external input events         */
#define LL_AOS_TARGET_3_TC0                   (3)             /*!< Target 3 for TC0 Channel 1 captures external input events         */
#define LL_AOS_TARGET_4_TC1                   (4)             /*!< Target 4 for TC1 External input clock source                      */
#define LL_AOS_TARGET_5_TC1                   (5)             /*!< Target 5 for TC1 External input events in slave mode              */
#define LL_AOS_TARGET_6_TC1                   (6)             /*!< Target 6 for TC1 Channel 0 captures external input events         */
#define LL_AOS_TARGET_7_TC1                   (7)             /*!< Target 7 for TC1 Channel 1 captures external input events         */
#define LL_AOS_TARGET_8_SIMPTC0               (8)             /*!< Target 8 for SIMPTC0 Hardware trigger channel 0 starts counting   */
#define LL_AOS_TARGET_9_SIMPTC0               (9)             /*!< Target 9 for SIMPTC0 Hardware trigger channel 1 starts counting   */
#define LL_AOS_TARGET_10_SIMPTC0              (10)            /*!< Target 10 for SIMPTC0 Hardware trigger channel 2 starts counting  */
#define LL_AOS_TARGET_11_SIMPTC0              (11)            /*!< Target 11 for SIMPTC0 Hardware trigger channel 3 starts counting  */
#define LL_AOS_TARGET_12_SIMPTC1              (12)            /*!< Target 12 for SIMPTC1 Hardware trigger channel 0 starts counting  */
#define LL_AOS_TARGET_13_SIMPTC1              (13)            /*!< Target 13 for SIMPTC1 Hardware trigger channel 1 starts counting  */
#define LL_AOS_TARGET_14_SIMPTC1              (14)            /*!< Target 14 for SIMPTC1 Hardware trigger channel 2 starts counting  */
#define LL_AOS_TARGET_15_SIMPTC1              (15)            /*!< Target 15 for SIMPTC1 Hardware trigger channel 3 starts counting  */
#define LL_AOS_TARGET_16_SIMPTC2              (16)            /*!< Target 16 for SIMPTC2 Hardware trigger channel 0 starts counting  */
#define LL_AOS_TARGET_17_SIMPTC2              (17)            /*!< Target 17 for SIMPTC2 Hardware trigger channel 1 starts counting  */
#define LL_AOS_TARGET_18_SIMPTC2              (18)            /*!< Target 18 for SIMPTC2 Hardware trigger channel 2 starts counting  */
#define LL_AOS_TARGET_19_SIMPTC2              (19)            /*!< Target 19 for SIMPTC2 Hardware trigger channel 3 starts counting  */
#define LL_AOS_TARGET_20_SIMPTC3              (20)            /*!< Target 20 for SIMPTC3 Hardware trigger channel 0 starts counting  */
#define LL_AOS_TARGET_21_SIMPTC3              (21)            /*!< Target 21 for SIMPTC3 Hardware trigger channel 1 starts counting  */
#define LL_AOS_TARGET_22_SIMPTC3              (22)            /*!< Target 22 for SIMPTC3 Hardware trigger channel 2 starts counting  */
#define LL_AOS_TARGET_23_SIMPTC3              (23)            /*!< Target 23 for SIMPTC3 Hardware trigger channel 3 starts counting  */
#define LL_AOS_TARGET_24_D2F                  (24)            /*!< Target 24 for D2F Hardware trigger hardware trigger D2F           */
#define LL_AOS_TARGET_25_IOCNT                (25)            /*!< Target 25 for IOCNT Channel 0 count source                        */
#define LL_AOS_TARGET_26_IOCNT                (26)            /*!< Target 26 for IOCNT Channel 1 count source                        */
#define LL_AOS_TARGET_27_IOCNT                (27)            /*!< Target 27 for IOCNT Channel 2 count source                        */
#define LL_AOS_TARGET_28_IOCNT                (28)            /*!< Target 28 for IOCNT Channel 3 count source                        */
#define LL_AOS_TARGET_29_IOCNT                (29)            /*!< Target 29 for IOCNT Channel 4 count source                        */
#define LL_AOS_TARGET_30_RC_CAL               (30)            /*!< Target 30 for RC_CAL Hardware trigger correction                  */
#define LL_AOS_TARGET_31_CAN                  (31)            /*!< Target 31 for CAN Timestamp input source                          */
#define LL_AOS_TARGET_32_ADC0                 (32)            /*!< Target 32 for ADC0 Hardware trigger sequence A starts conversion */
#define LL_AOS_TARGET_33_ADC0                 (33)            /*!< Target 33 for ADC0 Hardware trigger sequence B starts conversion */
#define LL_AOS_TARGET_34_DAC                  (34)            /*!< Target 34 for DAC Hardware trigger DAC starts conversion          */
/**
 * @}
 */

/** @defgroup AOS_LL_EC_TARGET_0_3_TC0_TRGSEL Target_0 to Target_3 TC0 Trigger source
 * @{
 */
#define LL_AOS_TARGET_0_3_TRIGGER_NONE                      AOS_PE_TRGSEL0_TRGSEL_NONE                       /*!< No trigger signal                                                                  */
#define LL_AOS_TARGET_0_3_TRIGGER_UART0_RX_INPUT            AOS_PE_TRGSEL0_TRGSEL_UART0_RX_INPUT             /*!< UART0 RX input signal as trigger signal for target0 to target3                     */
#define LL_AOS_TARGET_0_3_TRIGGER_UART1_RX_INPUT            AOS_PE_TRGSEL0_TRGSEL_UART1_RX_INPUT             /*!< UART1 RX input signal as trigger signal for target0 to target3                     */
#define LL_AOS_TARGET_0_3_TRIGGER_UART2_RX_INPUT            AOS_PE_TRGSEL0_TRGSEL_UART2_RX_INPUT             /*!< UART2 RX input signal as trigger signal for target0 to target3                     */
#define LL_AOS_TARGET_0_3_TRIGGER_UART3_RX_INPUT            AOS_PE_TRGSEL0_TRGSEL_UART3_RX_INPUT             /*!< UART3 RX input signal as trigger signal for target0 to target3                     */
#define LL_AOS_TARGET_0_3_TRIGGER_UART4_RX_INPUT            AOS_PE_TRGSEL0_TRGSEL_UART4_RX_INPUT             /*!< UART4 RX input signal as trigger signal for target0 to target3                     */
#define LL_AOS_TARGET_0_3_TRIGGER_UART5_RX_INPUT            AOS_PE_TRGSEL0_TRGSEL_UART5_RX_INPUT             /*!< UART5 RX input signal as trigger signal for target0 to target3                     */
#define LL_AOS_TARGET_0_3_TRIGGER_UART6_RX_INPUT            AOS_PE_TRGSEL0_TRGSEL_UART6_RX_INPUT             /*!< UART6 RX input signal as trigger signal for target0 to target3                     */
#define LL_AOS_TARGET_0_3_TRIGGER_UART7_RX_INPUT            AOS_PE_TRGSEL0_TRGSEL_UART7_RX_INPUT             /*!< UART7 RX input signal as trigger signal for target0 to target3                     */
#define LL_AOS_TARGET_0_3_TRIGGER_LPUART_RX_INPUT           AOS_PE_TRGSEL0_TRGSEL_LPUART_RX_INPUT            /*!< LPUART RX input signal as trigger signal for target0 to target3                    */
#define LL_AOS_TARGET_0_3_TRIGGER_USART0_RX_INPUT           AOS_PE_TRGSEL0_TRGSEL_USART0_RX_INPUT            /*!< USART0 RX input signal as trigger signal for target0 to target3                    */
#define LL_AOS_TARGET_0_3_TRIGGER_USART0_TX_INPUT           AOS_PE_TRGSEL0_TRGSEL_USART0_TX_INPUT            /*!< USART0 TX input signal as trigger signal for target0 to target3                    */
#define LL_AOS_TARGET_0_3_TRIGGER_USART1_RX_INPUT           AOS_PE_TRGSEL0_TRGSEL_USART1_RX_INPUT            /*!< USART1 RX input signal as trigger signal for target0 to target3                    */
#define LL_AOS_TARGET_0_3_TRIGGER_USART1_TX_INPUT           AOS_PE_TRGSEL0_TRGSEL_USART1_TX_INPUT            /*!< USART1 TX input signal as trigger signal for target0 to target3                    */
#define LL_AOS_TARGET_0_3_TRIGGER_ISO78160_IO_INPUT         AOS_PE_TRGSEL0_TRGSEL_ISO78160_IO_INPUT          /*!< ISO78160 IO input signal as trigger signal for target0 to target3                  */
#define LL_AOS_TARGET_0_3_TRIGGER_ISO78160_IN_INPUT         AOS_PE_TRGSEL0_TRGSEL_ISO78160_IN_INPUT          /*!< ISO78160 IN input signal as trigger signal for target0 to target3                  */
#define LL_AOS_TARGET_0_3_TRIGGER_ISO78161_IO_INPUT         AOS_PE_TRGSEL0_TRGSEL_ISO78161_IO_INPUT          /*!< ISO78161 IO input signal as trigger signal for target0 to target3                  */
#define LL_AOS_TARGET_0_3_TRIGGER_ISO78161_IN_INPUT         AOS_PE_TRGSEL0_TRGSEL_ISO78161_IN_INPUT          /*!< ISO78161 IN input signal as trigger signal for target0 to target3                  */
#define LL_AOS_TARGET_0_3_TRIGGER_CAN_RX_INPUT              AOS_PE_TRGSEL0_TRGSEL_CAN_RX_INPUT               /*!< CAN RX input signal as trigger signal for target0 to target3                       */
#define LL_AOS_TARGET_0_3_TRIGGER_D2F_CF_OUT0               AOS_PE_TRGSEL0_TRGSEL_D2F_CF_OUT0                /*!< D2F CF_OUT0 output signal as trigger signal for target0 to target3                 */
#define LL_AOS_TARGET_0_3_TRIGGER_D2F_CF_OUT1               AOS_PE_TRGSEL0_TRGSEL_D2F_CF_OUT1                /*!< D2F CF_OUT1 output signal as trigger signal for target0 to target3                 */
#define LL_AOS_TARGET_0_3_TRIGGER_TC1_CH0_POS               AOS_PE_TRGSEL0_TRGSEL_TC1_CH0_POS                /*!< TC1 channel 0 positive port output signal as trigger signal for target0 to target3 */
#define LL_AOS_TARGET_0_3_TRIGGER_TC1_CH0_NEG               AOS_PE_TRGSEL0_TRGSEL_TC1_CH0_NEG                /*!< TC1 channel 0 negative port output signal as trigger signal for target0 to target3 */
#define LL_AOS_TARGET_0_3_TRIGGER_TC1_CH1_POS               AOS_PE_TRGSEL0_TRGSEL_TC1_CH1_POS                /*!< TC1 channel 1 positive port output signal as trigger signal for target0 to target3 */
#define LL_AOS_TARGET_0_3_TRIGGER_TC1_CH1_NEG               AOS_PE_TRGSEL0_TRGSEL_TC1_CH1_NEG                /*!< TC1 channel 1 negative port output signal as trigger signal for target0 to target3 */
#define LL_AOS_TARGET_0_3_TRIGGER_VBAT_RTC_OUT              AOS_PE_TRGSEL0_TRGSEL_VBAT_RTC_OUT               /*!< VBAT RTC cycle output signal as trigger signal for target0 to target3              */
#define LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN0              AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN0               /*!< GPIO TC PIN0 signal as trigger signal for target0 to target3                       */
#define LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN1              AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN1               /*!< GPIO TC PIN1 signal as trigger signal for target0 to target3                       */
#define LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN2              AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN2               /*!< GPIO TC PIN2 signal as trigger signal for target0 to target3                       */
#define LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN3              AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN3               /*!< GPIO TC PIN3 signal as trigger signal for target0 to target3                       */
#define LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN4              AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN4               /*!< GPIO TC PIN4 signal as trigger signal for target0 to target3                       */
#define LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN5              AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN5               /*!< GPIO TC PIN5 signal as trigger signal for target0 to target3                       */
#define LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN6              AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN6               /*!< GPIO TC PIN6 signal as trigger signal for target0 to target3                       */
#define LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN7              AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN7               /*!< GPIO TC PIN7 signal as trigger signal for target0 to target3                       */
#define LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN8              AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN8               /*!< GPIO TC PIN8 signal as trigger signal for target0 to target3                       */
#define LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN9              AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN9               /*!< GPIO TC PIN9 signal as trigger signal for target0 to target3                       */
#define LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN10             AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN10              /*!< GPIO TC PIN10 signal as trigger signal for target0 to target3                      */
#define LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN11             AOS_PE_TRGSEL0_TRGSEL_GPIO_TC_PIN11              /*!< GPIO TC PIN11 signal as trigger signal for target0 to target3                      */
/**
 * @}
 */

/** @defgroup AOS_LL_EC_TARGET_4_7_TC1_TRGSEL Target_4 to Target_7 TC0 Trigger source
 * @{
 */
#define LL_AOS_TARGET_4_7_TRIGGER_NONE                      AOS_PE_TRGSEL4_TRGSEL_NONE                       /*!< No trigger signal                                                                  */
#define LL_AOS_TARGET_4_7_TRIGGER_UART0_RX_INPUT            AOS_PE_TRGSEL4_TRGSEL_UART0_RX_INPUT             /*!< UART0 RX input signal as trigger signal for target4 to target7                     */
#define LL_AOS_TARGET_4_7_TRIGGER_UART1_RX_INPUT            AOS_PE_TRGSEL4_TRGSEL_UART1_RX_INPUT             /*!< UART1 RX input signal as trigger signal for target4 to target7                     */
#define LL_AOS_TARGET_4_7_TRIGGER_UART2_RX_INPUT            AOS_PE_TRGSEL4_TRGSEL_UART2_RX_INPUT             /*!< UART2 RX input signal as trigger signal for target4 to target7                     */
#define LL_AOS_TARGET_4_7_TRIGGER_UART3_RX_INPUT            AOS_PE_TRGSEL4_TRGSEL_UART3_RX_INPUT             /*!< UART3 RX input signal as trigger signal for target4 to target7                     */
#define LL_AOS_TARGET_4_7_TRIGGER_UART4_RX_INPUT            AOS_PE_TRGSEL4_TRGSEL_UART4_RX_INPUT             /*!< UART4 RX input signal as trigger signal for target4 to target7                     */
#define LL_AOS_TARGET_4_7_TRIGGER_UART5_RX_INPUT            AOS_PE_TRGSEL4_TRGSEL_UART5_RX_INPUT             /*!< UART5 RX input signal as trigger signal for target4 to target7                     */
#define LL_AOS_TARGET_4_7_TRIGGER_UART6_RX_INPUT            AOS_PE_TRGSEL4_TRGSEL_UART6_RX_INPUT             /*!< UART6 RX input signal as trigger signal for target4 to target7                     */
#define LL_AOS_TARGET_4_7_TRIGGER_UART7_RX_INPUT            AOS_PE_TRGSEL4_TRGSEL_UART7_RX_INPUT             /*!< UART7 RX input signal as trigger signal for target4 to target7                     */
#define LL_AOS_TARGET_4_7_TRIGGER_LPUART_RX_INPUT           AOS_PE_TRGSEL4_TRGSEL_LPUART_RX_INPUT            /*!< LPUART RX input signal as trigger signal for target4 to target7                    */
#define LL_AOS_TARGET_4_7_TRIGGER_USART0_RX_INPUT           AOS_PE_TRGSEL4_TRGSEL_USART0_RX_INPUT            /*!< USART0 RX input signal as trigger signal for target4 to target7                    */
#define LL_AOS_TARGET_4_7_TRIGGER_USART0_TX_INPUT           AOS_PE_TRGSEL4_TRGSEL_USART0_TX_INPUT            /*!< USART0 TX input signal as trigger signal for target4 to target7                    */
#define LL_AOS_TARGET_4_7_TRIGGER_USART1_RX_INPUT           AOS_PE_TRGSEL4_TRGSEL_USART1_RX_INPUT            /*!< USART1 RX input signal as trigger signal for target4 to target7                    */
#define LL_AOS_TARGET_4_7_TRIGGER_USART1_TX_INPUT           AOS_PE_TRGSEL4_TRGSEL_USART1_TX_INPUT            /*!< USART1 TX input signal as trigger signal for target4 to target7                    */
#define LL_AOS_TARGET_4_7_TRIGGER_ISO78160_IO_INPUT         AOS_PE_TRGSEL4_TRGSEL_ISO78160_IO_INPUT          /*!< ISO78160 IO input signal as trigger signal for target4 to target7                  */
#define LL_AOS_TARGET_4_7_TRIGGER_ISO78160_IN_INPUT         AOS_PE_TRGSEL4_TRGSEL_ISO78160_IN_INPUT          /*!< ISO78160 IN input signal as trigger signal for target4 to target7                  */
#define LL_AOS_TARGET_4_7_TRIGGER_ISO78161_IO_INPUT         AOS_PE_TRGSEL4_TRGSEL_ISO78161_IO_INPUT          /*!< ISO78161 IO input signal as trigger signal for target4 to target7                  */
#define LL_AOS_TARGET_4_7_TRIGGER_ISO78161_IN_INPUT         AOS_PE_TRGSEL4_TRGSEL_ISO78161_IN_INPUT          /*!< ISO78161 IN input signal as trigger signal for target4 to target7                  */
#define LL_AOS_TARGET_4_7_TRIGGER_CAN_RX_INPUT              AOS_PE_TRGSEL4_TRGSEL_CAN_RX_INPUT               /*!< CAN RX input signal as trigger signal for target4 to target7                       */
#define LL_AOS_TARGET_4_7_TRIGGER_D2F_CF_OUT1               AOS_PE_TRGSEL4_TRGSEL_D2F_CF_OUT1                /*!< D2F CF_OUT0 output signal as trigger signal for target4 to target7                 */
#define LL_AOS_TARGET_4_7_TRIGGER_D2F_CF_OUT2               AOS_PE_TRGSEL4_TRGSEL_D2F_CF_OUT2                /*!< D2F CF_OUT1 output signal as trigger signal for target4 to target7                 */
#define LL_AOS_TARGET_4_7_TRIGGER_TC0_CH0_POS               AOS_PE_TRGSEL4_TRGSEL_TC0_CH0_POS                /*!< TC0 channel 0 positive port output signal as trigger signal for target4 to target7 */
#define LL_AOS_TARGET_4_7_TRIGGER_TC0_CH0_NEG               AOS_PE_TRGSEL4_TRGSEL_TC0_CH0_NEG                /*!< TC0 channel 0 negative port output signal as trigger signal for target4 to target7 */
#define LL_AOS_TARGET_4_7_TRIGGER_TC0_CH1_POS               AOS_PE_TRGSEL4_TRGSEL_TC0_CH1_POS                /*!< TC0 channel 1 positive port output signal as trigger signal for target4 to target7 */
#define LL_AOS_TARGET_4_7_TRIGGER_TC0_CH1_NEG               AOS_PE_TRGSEL4_TRGSEL_TC0_CH1_NEG                /*!< TC0 channel 1 negative port output signal as trigger signal for target4 to target7 */
#define LL_AOS_TARGET_4_7_TRIGGER_VBAT_RTC_OUT              AOS_PE_TRGSEL4_TRGSEL_VBAT_RTC_OUT               /*!< VBAT RTC cycle output signal as trigger signal for target4 to target7              */
#define LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN0              AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN0               /*!< GPIO TC PIN0 signal as trigger signal for target4 to target7                       */
#define LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN1              AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN1               /*!< GPIO TC PIN1 signal as trigger signal for target4 to target7                       */
#define LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN2              AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN2               /*!< GPIO TC PIN2 signal as trigger signal for target4 to target7                       */
#define LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN3              AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN3               /*!< GPIO TC PIN3 signal as trigger signal for target4 to target7                       */
#define LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN4              AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN4               /*!< GPIO TC PIN4 signal as trigger signal for target4 to target7                       */
#define LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN5              AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN5               /*!< GPIO TC PIN5 signal as trigger signal for target4 to target7                       */
#define LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN6              AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN6               /*!< GPIO TC PIN6 signal as trigger signal for target4 to target7                       */
#define LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN7              AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN7               /*!< GPIO TC PIN7 signal as trigger signal for target4 to target7                       */
#define LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN8              AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN8               /*!< GPIO TC PIN8 signal as trigger signal for target4 to target7                       */
#define LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN9              AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN9               /*!< GPIO TC PIN9 signal as trigger signal for target4 to target7                       */
#define LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN10             AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN10              /*!< GPIO TC PIN10 signal as trigger signal for target4 to target7                      */
#define LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN11             AOS_PE_TRGSEL4_TRGSEL_GPIO_TC_PIN11              /*!< GPIO TC PIN11 signal as trigger signal for target4 to target7                      */
/**
 * @}
 */

/** @defgroup AOS_LL_EC_TARGET_8_11_SIMPTC0_TRGSEL Target_8 to Target_11 SIMPTC0 Trigger source
 * @{
 */
#define LL_AOS_TARGET_8_11_TRIGGER_NONE                    AOS_PE_TRGSEL8_TRGSEL_NONE                        /*!< No trigger signal                                           */
#define LL_AOS_TARGET_8_TRIGGER_KBI0_CH0_OUTPUT            AOS_PE_TRGSEL8_TRGSEL_KBI0_CH0_OUTPUT             /*!< KBI0 channel 0 output signal as trigger signal for target8  */
#define LL_AOS_TARGET_8_TRIGGER_KBI0_CH4_OUTPUT            AOS_PE_TRGSEL8_TRGSEL_KBI0_CH4_OUTPUT             /*!< KBI0 channel 4 output signal as trigger signal for target8  */
#define LL_AOS_TARGET_8_TRIGGER_KBI1_CH0_OUTPUT            AOS_PE_TRGSEL8_TRGSEL_KBI1_CH0_OUTPUT             /*!< KBI1 channel 0 output signal as trigger signal for target8  */
#define LL_AOS_TARGET_8_TRIGGER_KBI1_CH4_OUTPUT            AOS_PE_TRGSEL8_TRGSEL_KBI1_CH4_OUTPUT             /*!< KBI1 channel 4 output signal as trigger signal for target8  */

#define LL_AOS_TARGET_9_TRIGGER_KBI0_CH1_OUTPUT            AOS_PE_TRGSEL9_TRGSEL_KBI0_CH1_OUTPUT             /*!< KBI0 channel 1 output signal as trigger signal for target9  */
#define LL_AOS_TARGET_9_TRIGGER_KBI0_CH5_OUTPUT            AOS_PE_TRGSEL9_TRGSEL_KBI0_CH5_OUTPUT             /*!< KBI0 channel 5 output signal as trigger signal for target9  */
#define LL_AOS_TARGET_9_TRIGGER_KBI1_CH1_OUTPUT            AOS_PE_TRGSEL9_TRGSEL_KBI1_CH1_OUTPUT             /*!< KBI1 channel 1 output signal as trigger signal for target9  */
#define LL_AOS_TARGET_9_TRIGGER_KBI1_CH5_OUTPUT            AOS_PE_TRGSEL9_TRGSEL_KBI1_CH5_OUTPUT             /*!< KBI1 channel 5 output signal as trigger signal for target9  */

#define LL_AOS_TARGET_10_TRIGGER_KBI0_CH2_OUTPUT           AOS_PE_TRGSEL10_TRGSEL_KBI0_CH2_OUTPUT            /*!< KBI0 channel 2 output signal as trigger signal for target10 */
#define LL_AOS_TARGET_10_TRIGGER_KBI0_CH6_OUTPUT           AOS_PE_TRGSEL10_TRGSEL_KBI0_CH6_OUTPUT            /*!< KBI0 channel 6 output signal as trigger signal for target10 */
#define LL_AOS_TARGET_10_TRIGGER_KBI1_CH2_OUTPUT           AOS_PE_TRGSEL10_TRGSEL_KBI1_CH2_OUTPUT            /*!< KBI1 channel 2 output signal as trigger signal for target10 */
#define LL_AOS_TARGET_10_TRIGGER_KBI1_CH6_OUTPUT           AOS_PE_TRGSEL10_TRGSEL_KBI1_CH6_OUTPUT            /*!< KBI1 channel 6 output signal as trigger signal for target10 */

#define LL_AOS_TARGET_11_TRIGGER_KBI0_CH3_OUTPUT           AOS_PE_TRGSEL11_TRGSEL_KBI0_CH3_OUTPUT            /*!< KBI0 channel 3 output signal as trigger signal for target11 */
#define LL_AOS_TARGET_11_TRIGGER_KBI0_CH7_OUTPUT           AOS_PE_TRGSEL11_TRGSEL_KBI0_CH7_OUTPUT            /*!< KBI0 channel 7 output signal as trigger signal for target11 */
#define LL_AOS_TARGET_11_TRIGGER_KBI1_CH3_OUTPUT           AOS_PE_TRGSEL11_TRGSEL_KBI1_CH3_OUTPUT            /*!< KBI1 channel 3 output signal as trigger signal for target11 */
#define LL_AOS_TARGET_11_TRIGGER_KBI1_CH7_OUTPUT           AOS_PE_TRGSEL11_TRGSEL_KBI1_CH7_OUTPUT            /*!< KBI1 channel 7 output signal as trigger signal for target11 */
/**
 * @}
 */

/** @defgroup AOS_LL_EC_TARGET_12_15_SIMPTC1_TRGSEL Target_12 to Target_15 SIMPTC1 Trigger source
 * @{
 */
#define LL_AOS_TARGET_12_15_TRIGGER_NONE                   AOS_PE_TRGSEL12_TRGSEL_NONE                        /*!< No trigger signal                                           */
#define LL_AOS_TARGET_12_TRIGGER_KBI0_CH0_OUTPUT           AOS_PE_TRGSEL12_TRGSEL_KBI0_CH0_OUTPUT             /*!< KBI0 channel 0 output signal as trigger signal for target12 */
#define LL_AOS_TARGET_12_TRIGGER_KBI0_CH4_OUTPUT           AOS_PE_TRGSEL12_TRGSEL_KBI0_CH4_OUTPUT             /*!< KBI0 channel 4 output signal as trigger signal for target12 */
#define LL_AOS_TARGET_12_TRIGGER_KBI1_CH0_OUTPUT           AOS_PE_TRGSEL12_TRGSEL_KBI1_CH0_OUTPUT             /*!< KBI1 channel 0 output signal as trigger signal for target12 */
#define LL_AOS_TARGET_12_TRIGGER_KBI1_CH4_OUTPUT           AOS_PE_TRGSEL12_TRGSEL_KBI1_CH4_OUTPUT             /*!< KBI1 channel 4 output signal as trigger signal for target12 */

#define LL_AOS_TARGET_13_TRIGGER_KBI0_CH1_OUTPUT           AOS_PE_TRGSEL13_TRGSEL_KBI0_CH1_OUTPUT             /*!< KBI0 channel 1 output signal as trigger signal for target13 */
#define LL_AOS_TARGET_13_TRIGGER_KBI0_CH5_OUTPUT           AOS_PE_TRGSEL13_TRGSEL_KBI0_CH5_OUTPUT             /*!< KBI0 channel 5 output signal as trigger signal for target13 */
#define LL_AOS_TARGET_13_TRIGGER_KBI1_CH1_OUTPUT           AOS_PE_TRGSEL13_TRGSEL_KBI1_CH1_OUTPUT             /*!< KBI1 channel 1 output signal as trigger signal for target13 */
#define LL_AOS_TARGET_13_TRIGGER_KBI1_CH5_OUTPUT           AOS_PE_TRGSEL13_TRGSEL_KBI1_CH5_OUTPUT             /*!< KBI1 channel 5 output signal as trigger signal for target13 */

#define LL_AOS_TARGET_14_TRIGGER_KBI0_CH2_OUTPUT           AOS_PE_TRGSEL14_TRGSEL_KBI0_CH2_OUTPUT             /*!< KBI0 channel 2 output signal as trigger signal for target14 */
#define LL_AOS_TARGET_14_TRIGGER_KBI0_CH6_OUTPUT           AOS_PE_TRGSEL14_TRGSEL_KBI0_CH6_OUTPUT             /*!< KBI0 channel 6 output signal as trigger signal for target14 */
#define LL_AOS_TARGET_14_TRIGGER_KBI1_CH2_OUTPUT           AOS_PE_TRGSEL14_TRGSEL_KBI1_CH2_OUTPUT             /*!< KBI1 channel 2 output signal as trigger signal for target14 */
#define LL_AOS_TARGET_14_TRIGGER_KBI1_CH6_OUTPUT           AOS_PE_TRGSEL14_TRGSEL_KBI1_CH6_OUTPUT             /*!< KBI1 channel 6 output signal as trigger signal for target14 */

#define LL_AOS_TARGET_15_TRIGGER_KBI0_CH3_OUTPUT           AOS_PE_TRGSEL15_TRGSEL_KBI0_CH3_OUTPUT             /*!< KBI0 channel 3 output signal as trigger signal for target15 */
#define LL_AOS_TARGET_15_TRIGGER_KBI0_CH7_OUTPUT           AOS_PE_TRGSEL15_TRGSEL_KBI0_CH7_OUTPUT             /*!< KBI0 channel 7 output signal as trigger signal for target15 */
#define LL_AOS_TARGET_15_TRIGGER_KBI1_CH3_OUTPUT           AOS_PE_TRGSEL15_TRGSEL_KBI1_CH3_OUTPUT             /*!< KBI1 channel 3 output signal as trigger signal for target15 */
#define LL_AOS_TARGET_15_TRIGGER_KBI1_CH7_OUTPUT           AOS_PE_TRGSEL15_TRGSEL_KBI1_CH7_OUTPUT             /*!< KBI1 channel 7 output signal as trigger signal for target15 */
/**
 * @}
 */

/** @defgroup AOS_LL_EC_TARGET_16_19_SIMPTC2_TRGSEL Target_16 to Target_19 SIMPTC2 Trigger source
 * @{
 */
#define LL_AOS_TARGET_16_19_TRIGGER_NONE                   AOS_PE_TRGSEL16_TRGSEL_NONE                        /*!< No trigger signal                                            */
#define LL_AOS_TARGET_16_TRIGGER_KBI0_CH8_OUTPUT           AOS_PE_TRGSEL16_TRGSEL_KBI0_CH8_OUTPUT             /*!< KBI0 channel 8 output signal as trigger signal for target16  */
#define LL_AOS_TARGET_16_TRIGGER_KBI0_CH12_OUTPUT          AOS_PE_TRGSEL16_TRGSEL_KBI0_CH12_OUTPUT            /*!< KBI0 channel 12 output signal as trigger signal for target16 */
#define LL_AOS_TARGET_16_TRIGGER_KBI1_CH8_OUTPUT           AOS_PE_TRGSEL16_TRGSEL_KBI1_CH8_OUTPUT             /*!< KBI1 channel 8 output signal as trigger signal for target16  */
#define LL_AOS_TARGET_16_TRIGGER_KBI1_CH12_OUTPUT          AOS_PE_TRGSEL16_TRGSEL_KBI1_CH12_OUTPUT            /*!< KBI1 channel 12 output signal as trigger signal for target16 */

#define LL_AOS_TARGET_17_TRIGGER_KBI0_CH9_OUTPUT           AOS_PE_TRGSEL17_TRGSEL_KBI0_CH9_OUTPUT             /*!< KBI0 channel 9 output signal as trigger signal for target17  */
#define LL_AOS_TARGET_17_TRIGGER_KBI0_CH13_OUTPUT          AOS_PE_TRGSEL17_TRGSEL_KBI0_CH13_OUTPUT            /*!< KBI0 channel 13 output signal as trigger signal for target17 */
#define LL_AOS_TARGET_17_TRIGGER_KBI1_CH9_OUTPUT           AOS_PE_TRGSEL17_TRGSEL_KBI1_CH9_OUTPUT             /*!< KBI1 channel 9 output signal as trigger signal for target17  */
#define LL_AOS_TARGET_17_TRIGGER_KBI1_CH13_OUTPUT          AOS_PE_TRGSEL17_TRGSEL_KBI1_CH13_OUTPUT            /*!< KBI1 channel 13 output signal as trigger signal for target17 */

#define LL_AOS_TARGET_18_TRIGGER_KBI0_CH10_OUTPUT          AOS_PE_TRGSEL18_TRGSEL_KBI0_CH10_OUTPUT            /*!< KBI0 channel 10 output signal as trigger signal for target18 */
#define LL_AOS_TARGET_18_TRIGGER_KBI0_CH14_OUTPUT          AOS_PE_TRGSEL18_TRGSEL_KBI0_CH14_OUTPUT            /*!< KBI0 channel 14 output signal as trigger signal for target18 */
#define LL_AOS_TARGET_18_TRIGGER_KBI1_CH10_OUTPUT          AOS_PE_TRGSEL18_TRGSEL_KBI1_CH10_OUTPUT            /*!< KBI1 channel 10 output signal as trigger signal for target18 */
#define LL_AOS_TARGET_18_TRIGGER_KBI1_CH14_OUTPUT          AOS_PE_TRGSEL18_TRGSEL_KBI1_CH14_OUTPUT            /*!< KBI1 channel 14 output signal as trigger signal for target18 */

#define LL_AOS_TARGET_19_TRIGGER_KBI0_CH11_OUTPUT          AOS_PE_TRGSEL19_TRGSEL_KBI0_CH11_OUTPUT            /*!< KBI0 channel 11 output signal as trigger signal for target19 */
#define LL_AOS_TARGET_19_TRIGGER_KBI0_CH15_OUTPUT          AOS_PE_TRGSEL19_TRGSEL_KBI0_CH15_OUTPUT            /*!< KBI0 channel 15 output signal as trigger signal for target19 */
#define LL_AOS_TARGET_19_TRIGGER_KBI1_CH11_OUTPUT          AOS_PE_TRGSEL19_TRGSEL_KBI1_CH11_OUTPUT            /*!< KBI1 channel 11 output signal as trigger signal for target19 */
#define LL_AOS_TARGET_19_TRIGGER_KBI1_CH15_OUTPUT          AOS_PE_TRGSEL19_TRGSEL_KBI1_CH15_OUTPUT            /*!< KBI1 channel 15 output signal as trigger signal for target19 */
/**
 * @}
 */

/** @defgroup AOS_LL_EC_TARGET_20_23_SIMPTC2_TRGSEL Target_20 to Target_23 SIMPTC3 Trigger source
 * @{
 */
#define LL_AOS_TARGET_20_23_TRIGGER_NONE                   AOS_PE_TRGSEL23_TRGSEL_NONE                        /*!< No trigger signal                                            */
#define LL_AOS_TARGET_20_TRIGGER_KBI0_CH8_OUTPUT           AOS_PE_TRGSEL20_TRGSEL_KBI0_CH8_OUTPUT             /*!< KBI0 channel 8 output signal as trigger signal for target16  */
#define LL_AOS_TARGET_20_TRIGGER_KBI0_CH12_OUTPUT          AOS_PE_TRGSEL20_TRGSEL_KBI0_CH12_OUTPUT            /*!< KBI0 channel 12 output signal as trigger signal for target16 */
#define LL_AOS_TARGET_20_TRIGGER_KBI1_CH8_OUTPUT           AOS_PE_TRGSEL20_TRGSEL_KBI1_CH8_OUTPUT             /*!< KBI1 channel 8 output signal as trigger signal for target16  */
#define LL_AOS_TARGET_20_TRIGGER_KBI1_CH12_OUTPUT          AOS_PE_TRGSEL20_TRGSEL_KBI1_CH12_OUTPUT            /*!< KBI1 channel 12 output signal as trigger signal for target16 */

#define LL_AOS_TARGET_21_TRIGGER_KBI0_CH9_OUTPUT           AOS_PE_TRGSEL21_TRGSEL_KBI0_CH9_OUTPUT             /*!< KBI0 channel 9 output signal as trigger signal for target17  */
#define LL_AOS_TARGET_21_TRIGGER_KBI0_CH13_OUTPUT          AOS_PE_TRGSEL21_TRGSEL_KBI0_CH13_OUTPUT            /*!< KBI0 channel 13 output signal as trigger signal for target17 */
#define LL_AOS_TARGET_21_TRIGGER_KBI1_CH9_OUTPUT           AOS_PE_TRGSEL21_TRGSEL_KBI1_CH9_OUTPUT             /*!< KBI1 channel 9 output signal as trigger signal for target17  */
#define LL_AOS_TARGET_21_TRIGGER_KBI1_CH13_OUTPUT          AOS_PE_TRGSEL21_TRGSEL_KBI1_CH13_OUTPUT            /*!< KBI1 channel 13 output signal as trigger signal for target17 */

#define LL_AOS_TARGET_22_TRIGGER_KBI0_CH10_OUTPUT          AOS_PE_TRGSEL22_TRGSEL_KBI0_CH10_OUTPUT            /*!< KBI0 channel 10 output signal as trigger signal for target18 */
#define LL_AOS_TARGET_22_TRIGGER_KBI0_CH14_OUTPUT          AOS_PE_TRGSEL22_TRGSEL_KBI0_CH14_OUTPUT            /*!< KBI0 channel 14 output signal as trigger signal for target18 */
#define LL_AOS_TARGET_22_TRIGGER_KBI1_CH10_OUTPUT          AOS_PE_TRGSEL22_TRGSEL_KBI1_CH10_OUTPUT            /*!< KBI1 channel 10 output signal as trigger signal for target18 */
#define LL_AOS_TARGET_22_TRIGGER_KBI1_CH14_OUTPUT          AOS_PE_TRGSEL22_TRGSEL_KBI1_CH14_OUTPUT            /*!< KBI1 channel 14 output signal as trigger signal for target18 */

#define LL_AOS_TARGET_23_TRIGGER_KBI0_CH11_OUTPUT          AOS_PE_TRGSEL23_TRGSEL_KBI0_CH11_OUTPUT            /*!< KBI0 channel 11 output signal as trigger signal for target19 */
#define LL_AOS_TARGET_23_TRIGGER_KBI0_CH15_OUTPUT          AOS_PE_TRGSEL23_TRGSEL_KBI0_CH15_OUTPUT            /*!< KBI0 channel 15 output signal as trigger signal for target19 */
#define LL_AOS_TARGET_23_TRIGGER_KBI1_CH11_OUTPUT          AOS_PE_TRGSEL23_TRGSEL_KBI1_CH11_OUTPUT            /*!< KBI1 channel 11 output signal as trigger signal for target19 */
#define LL_AOS_TARGET_23_TRIGGER_KBI1_CH15_OUTPUT          AOS_PE_TRGSEL23_TRGSEL_KBI1_CH15_OUTPUT            /*!< KBI1 channel 15 output signal as trigger signal for target19 */
/**
 * @}
 */

/** @defgroup AOS_LL_EC_TARGET_24_D2F_TRGSEL Target_24 D2F Trigger source
 * @{
 */
#define LL_AOS_TARGET_24_TRIGGER_NONE                      AOS_PE_TRGSEL24_TRGSEL_NONE                        /*!< No trigger signal                                                */
#define LL_AOS_TARGET_24_TRIGGER_SIMPTC0_CH0_OVL           AOS_PE_TRGSEL24_TRGSEL_SIMPTC0_CH0_OVL             /*!< SIMPTC0 channel 0 overflow signal as trigger signal for target24 */
#define LL_AOS_TARGET_24_TRIGGER_SIMPTC1_CH0_OVL           AOS_PE_TRGSEL24_TRGSEL_SIMPTC1_CH0_OVL             /*!< SIMPTC1 channel 0 overflow signal as trigger signal for target24 */
#define LL_AOS_TARGET_24_TRIGGER_SIMPTC2_CH0_OVL           AOS_PE_TRGSEL24_TRGSEL_SIMPTC2_CH0_OVL             /*!< SIMPTC2 channel 0 overflow signal as trigger signal for target24 */
#define LL_AOS_TARGET_24_TRIGGER_SIMPTC3_CH0_OVL           AOS_PE_TRGSEL24_TRGSEL_SIMPTC3_CH0_OVL             /*!< SIMPTC3 channel 0 overflow signal as trigger signal for target24 */
/**
 * @}
 */

/** @defgroup AOS_LL_EC_TARGET_25_29_IOCNT_TRGSEL Target_25 to Target_29 IOCNT Trigger source
 * @{
 */
#define LL_AOS_TARGET_25_29_TRIGGER_NONE                   AOS_PE_TRGSEL25_TRGSEL_NONE                       /*!< No trigger signal                         */
#define LL_AOS_TARGET_25_29_TRIGGER_UART6_PF_OUTPUT        AOS_PE_TRGSEL25_TRGSEL_UART6_PF_OUTPUT            /*!< UART6 PF output signal as trigger signal for target25 totarget29    */
#define LL_AOS_TARGET_25_29_TRIGGER_UART6_QF_OUTPUT        AOS_PE_TRGSEL25_TRGSEL_UART6_QF_OUTPUT            /*!< UART6 QF output signal as trigger signal or target25 totarget29     */
#define LL_AOS_TARGET_25_29_TRIGGER_UART6_FPF_OUTPUT       AOS_PE_TRGSEL25_TRGSEL_UART6_FPF_OUTPUT           /*!< UART6 FPF output signal as trigger signal for target25 totarget29   */
#define LL_AOS_TARGET_25_29_TRIGGER_UART7_PF_OUTPUT        AOS_PE_TRGSEL25_TRGSEL_UART7_PF_OUTPUT            /*!< UART7 PF output signal as trigger signal for target25 totarget29    */
#define LL_AOS_TARGET_25_29_TRIGGER_UART7_QF_OUTPUT        AOS_PE_TRGSEL25_TRGSEL_UART7_QF_OUTPUT            /*!< UART7 QF output signal as trigger signal for target25 totarget29    */
#define LL_AOS_TARGET_25_29_TRIGGER_UART7_FPF_OUTPUT       AOS_PE_TRGSEL25_TRGSEL_UART7_FPF_OUTPUT           /*!< UART7 FPF output signal as trigger signal for target25 totarget29   */
#define LL_AOS_TARGET_25_29_TRIGGER_D2F_CF_OUT0            AOS_PE_TRGSEL25_TRGSEL_D2F_CF_OUT0                /*!< D2F CF OUT0 output signal as trigger signal for target25 totarget29 */
#define LL_AOS_TARGET_25_29_TRIGGER_D2F_CF_OUT1            AOS_PE_TRGSEL25_TRGSEL_D2F_CF_OUT1                /*!< D2F CF OUT1 output signal as trigger signal for target25 totarget29 */
#define LL_AOS_TARGET_25_29_TRIGGER_D2F_CF_OUT2            AOS_PE_TRGSEL25_TRGSEL_D2F_CF_OUT2                /*!< D2F CF OUT2 output signal as trigger signal for target25 totarget29 */

#define LL_AOS_TARGET_25_TRIGGER_KBI0_CH0_INPUT            AOS_PE_TRGSEL25_TRGSEL_KBI0_CH0_INPUT             /*!< KBI0 channel 0 input signal as trigger signal for target25          */
#define LL_AOS_TARGET_25_TRIGGER_KBI0_CH1_INPUT            AOS_PE_TRGSEL25_TRGSEL_KBI0_CH1_INPUT             /*!< KBI0 channel 1 input signal as trigger signal for target25          */
#define LL_AOS_TARGET_25_TRIGGER_KBI0_CH2_INPUT            AOS_PE_TRGSEL25_TRGSEL_KBI0_CH2_INPUT             /*!< KBI0 channel 2 input signal as trigger signal for target25          */
#define LL_AOS_TARGET_25_TRIGGER_KBI0_CH3_INPUT            AOS_PE_TRGSEL25_TRGSEL_KBI0_CH3_INPUT             /*!< KBI0 channel 3 input signal as trigger signal for target25          */
#define LL_AOS_TARGET_25_TRIGGER_KBI1_CH0_INPUT            AOS_PE_TRGSEL25_TRGSEL_KBI1_CH0_INPUT             /*!< KBI1 channel 0 input signal as trigger signal for target25          */
#define LL_AOS_TARGET_25_TRIGGER_KBI1_CH1_INPUT            AOS_PE_TRGSEL25_TRGSEL_KBI1_CH1_INPUT             /*!< KBI1 channel 1 input signal as trigger signal for target25          */
#define LL_AOS_TARGET_25_TRIGGER_KBI1_CH2_INPUT            AOS_PE_TRGSEL25_TRGSEL_KBI1_CH2_INPUT             /*!< KBI1 channel 2 input signal as trigger signal for target25          */
#define LL_AOS_TARGET_25_TRIGGER_KBI1_CH3_INPUT            AOS_PE_TRGSEL25_TRGSEL_KBI1_CH3_INPUT             /*!< KBI1 channel 3 input signal as trigger signal for target25          */

#define LL_AOS_TARGET_26_TRIGGER_KBI0_CH4_INPUT            AOS_PE_TRGSEL26_TRGSEL_KBI0_CH4_INPUT             /*!< KBI0 channel 4 input signal as trigger signal for target26          */
#define LL_AOS_TARGET_26_TRIGGER_KBI0_CH5_INPUT            AOS_PE_TRGSEL26_TRGSEL_KBI0_CH5_INPUT             /*!< KBI0 channel 5 input signal as trigger signal for target26          */
#define LL_AOS_TARGET_26_TRIGGER_KBI0_CH6_INPUT            AOS_PE_TRGSEL26_TRGSEL_KBI0_CH6_INPUT             /*!< KBI0 channel 6 input signal as trigger signal for target26          */
#define LL_AOS_TARGET_26_TRIGGER_KBI0_CH7_INPUT            AOS_PE_TRGSEL26_TRGSEL_KBI0_CH7_INPUT             /*!< KBI0 channel 7 input signal as trigger signal for target26          */
#define LL_AOS_TARGET_26_TRIGGER_KBI1_CH4_INPUT            AOS_PE_TRGSEL26_TRGSEL_KBI1_CH4_INPUT             /*!< KBI1 channel 4 input signal as trigger signal for target26          */
#define LL_AOS_TARGET_26_TRIGGER_KBI1_CH5_INPUT            AOS_PE_TRGSEL26_TRGSEL_KBI1_CH5_INPUT             /*!< KBI1 channel 5 input signal as trigger signal for target26          */
#define LL_AOS_TARGET_26_TRIGGER_KBI1_CH6_INPUT            AOS_PE_TRGSEL26_TRGSEL_KBI1_CH6_INPUT             /*!< KBI1 channel 6 input signal as trigger signal for target26          */
#define LL_AOS_TARGET_26_TRIGGER_KBI1_CH7_INPUT            AOS_PE_TRGSEL26_TRGSEL_KBI1_CH7_INPUT             /*!< KBI1 channel 7 input signal as trigger signal for target26          */

#define LL_AOS_TARGET_27_TRIGGER_KBI0_CH8_INPUT            AOS_PE_TRGSEL26_TRGSEL_KBI0_CH8_INPUT             /*!< KBI0 channel 8 input signal as trigger signal for target27          */
#define LL_AOS_TARGET_27_TRIGGER_KBI0_CH9_INPUT            AOS_PE_TRGSEL26_TRGSEL_KBI0_CH9_INPUT             /*!< KBI0 channel 9 input signal as trigger signal for target27          */
#define LL_AOS_TARGET_27_TRIGGER_KBI0_CH10_INPUT           AOS_PE_TRGSEL26_TRGSEL_KBI0_CH10_INPUT            /*!< KBI0 channel 10 input signal as trigger signal for target27         */
#define LL_AOS_TARGET_27_TRIGGER_KBI0_CH11_INPUT           AOS_PE_TRGSEL26_TRGSEL_KBI0_CH11_INPUT            /*!< KBI0 channel 11 input signal as trigger signal for target27         */
#define LL_AOS_TARGET_27_TRIGGER_KBI1_CH8_INPUT            AOS_PE_TRGSEL26_TRGSEL_KBI1_CH8_INPUT             /*!< KBI1 channel 8 input signal as trigger signal for target27          */
#define LL_AOS_TARGET_27_TRIGGER_KBI1_CH9_INPUT            AOS_PE_TRGSEL26_TRGSEL_KBI1_CH9_INPUT             /*!< KBI1 channel 9 input signal as trigger signal for target27          */
#define LL_AOS_TARGET_27_TRIGGER_KBI1_CH10_INPUT           AOS_PE_TRGSEL26_TRGSEL_KBI1_CH10_INPUT            /*!< KBI1 channel 10 input signal as trigger signal for target27         */
#define LL_AOS_TARGET_27_TRIGGER_KBI1_CH11_INPUT           AOS_PE_TRGSEL26_TRGSEL_KBI1_CH11_INPUT            /*!< KBI1 channel 11 input signal as trigger signal for target27         */

#define LL_AOS_TARGET_28_TRIGGER_KBI0_CH12_INPUT           AOS_PE_TRGSEL28_TRGSEL_KBI0_CH12_INPUT            /*!< KBI0 channel 12 input signal as trigger signal for target28         */
#define LL_AOS_TARGET_28_TRIGGER_KBI0_CH13_INPUT           AOS_PE_TRGSEL28_TRGSEL_KBI0_CH13_INPUT            /*!< KBI0 channel 13 input signal as trigger signal for target28         */
#define LL_AOS_TARGET_28_TRIGGER_KBI0_CH14_INPUT           AOS_PE_TRGSEL28_TRGSEL_KBI0_CH14_INPUT            /*!< KBI0 channel 14 input signal as trigger signal for target28         */
#define LL_AOS_TARGET_28_TRIGGER_KBI0_CH15_INPUT           AOS_PE_TRGSEL28_TRGSEL_KBI0_CH15_INPUT            /*!< KBI0 channel 15 input signal as trigger signal for target28         */
#define LL_AOS_TARGET_28_TRIGGER_KBI1_CH12_INPUT           AOS_PE_TRGSEL28_TRGSEL_KBI1_CH12_INPUT            /*!< KBI1 channel 12 input signal as trigger signal for target28         */
#define LL_AOS_TARGET_28_TRIGGER_KBI1_CH13_INPUT           AOS_PE_TRGSEL28_TRGSEL_KBI1_CH13_INPUT            /*!< KBI1 channel 13 input signal as trigger signal for target28         */
#define LL_AOS_TARGET_28_TRIGGER_KBI1_CH14_INPUT           AOS_PE_TRGSEL28_TRGSEL_KBI1_CH14_INPUT            /*!< KBI1 channel 14 input signal as trigger signal for target28         */
#define LL_AOS_TARGET_28_TRIGGER_KBI1_CH15_INPUT           AOS_PE_TRGSEL28_TRGSEL_KBI1_CH15_INPUT            /*!< KBI1 channel 15 input signal as trigger signal for target28         */

#define LL_AOS_TARGET_29_TRIGGER_KBI0_CH0_INPUT            AOS_PE_TRGSEL29_TRGSEL_KBI0_CH0_INPUT             /*!< KBI0 channel 0 input signal as trigger signal for target29          */
#define LL_AOS_TARGET_29_TRIGGER_KBI0_CH1_INPUT            AOS_PE_TRGSEL29_TRGSEL_KBI0_CH1_INPUT             /*!< KBI0 channel 1 input signal as trigger signal for target29          */
#define LL_AOS_TARGET_29_TRIGGER_KBI0_CH2_INPUT            AOS_PE_TRGSEL29_TRGSEL_KBI0_CH2_INPUT             /*!< KBI0 channel 2 input signal as trigger signal for target29          */
#define LL_AOS_TARGET_29_TRIGGER_KBI0_CH3_INPUT            AOS_PE_TRGSEL29_TRGSEL_KBI0_CH3_INPUT             /*!< KBI0 channel 3 input signal as trigger signal for target29          */
#define LL_AOS_TARGET_29_TRIGGER_KBI1_CH0_INPUT            AOS_PE_TRGSEL29_TRGSEL_KBI1_CH0_INPUT             /*!< KBI1 channel 0 input signal as trigger signal for target29          */
#define LL_AOS_TARGET_29_TRIGGER_KBI1_CH1_INPUT            AOS_PE_TRGSEL29_TRGSEL_KBI1_CH1_INPUT             /*!< KBI1 channel 1 input signal as trigger signal for target29          */
#define LL_AOS_TARGET_29_TRIGGER_KBI1_CH2_INPUT            AOS_PE_TRGSEL29_TRGSEL_KBI1_CH2_INPUT             /*!< KBI1 channel 2 input signal as trigger signal for target29          */
#define LL_AOS_TARGET_29_TRIGGER_KBI1_CH3_INPUT            AOS_PE_TRGSEL29_TRGSEL_KBI1_CH3_INPUT             /*!< KBI1 channel 3 input signal as trigger signal for target29          */
/**
 * @}
 */

/** @defgroup AOS_LL_EC_TARGET_30_RC_CAL_TRGSEL Target_30 RC_CAL Trigger source
 * @{
 */
#define LL_AOS_TARGET_30_TRIGGER_NONE                      AOS_PE_TRGSEL24_TRGSEL_NONE                       /*!< No trigger signal                                                   */
#define LL_AOS_TARGET_30_TRIGGER_KBI0_CH0_OUTPUT           AOS_PE_TRGSEL30_TRGSEL_KBI0_CH0_OUTPUT            /*!< KBI0 channel 0 output signal as trigger signal for target30         */
#define LL_AOS_TARGET_30_TRIGGER_KBI0_CH1_OUTPUT           AOS_PE_TRGSEL30_TRGSEL_KBI0_CH1_OUTPUT            /*!< KBI0 channel 1 output signal as trigger signal for target30         */
#define LL_AOS_TARGET_30_TRIGGER_KBI0_CH2_OUTPUT           AOS_PE_TRGSEL30_TRGSEL_KBI0_CH2_OUTPUT            /*!< KBI0 channel 2 output signal as trigger signal for target30         */
#define LL_AOS_TARGET_30_TRIGGER_KBI0_CH3_OUTPUT           AOS_PE_TRGSEL30_TRGSEL_KBI0_CH3_OUTPUT            /*!< KBI0 channel 3 output signal as trigger signal for target30         */
#define LL_AOS_TARGET_30_TRIGGER_KBI1_CH0_OUTPUT           AOS_PE_TRGSEL30_TRGSEL_KBI1_CH0_OUTPUT            /*!< KBI1 channel 0 output signal as trigger signal for target30         */
#define LL_AOS_TARGET_30_TRIGGER_KBI1_CH1_OUTPUT           AOS_PE_TRGSEL30_TRGSEL_KBI1_CH1_OUTPUT            /*!< KBI1 channel 1 output signal as trigger signal for target30         */
#define LL_AOS_TARGET_30_TRIGGER_KBI1_CH2_OUTPUT           AOS_PE_TRGSEL30_TRGSEL_KBI1_CH2_OUTPUT            /*!< KBI1 channel 2 output signal as trigger signal for target30         */
#define LL_AOS_TARGET_30_TRIGGER_KBI1_CH3_OUTPUT           AOS_PE_TRGSEL30_TRGSEL_KBI1_CH3_OUTPUT            /*!< KBI1 channel 3 output signal as trigger signal for target30         */
#define LL_AOS_TARGET_30_TRIGGER_SIMPTC0_CH0_OVL           AOS_PE_TRGSEL30_TRGSEL_SIMPTC0_CH0_OVL            /*!< SIMPTC0 channel 0 overflow signal as trigger signal for target30    */
#define LL_AOS_TARGET_30_TRIGGER_SIMPTC1_CH0_OVL           AOS_PE_TRGSEL30_TRGSEL_SIMPTC1_CH0_OVL            /*!< SIMPTC1 channel 0 overflow signal as trigger signal for target30    */
#define LL_AOS_TARGET_30_TRIGGER_SIMPTC2_CH0_OVL           AOS_PE_TRGSEL30_TRGSEL_SIMPTC2_CH0_OVL            /*!< SIMPTC2 channel 0 overflow signal as trigger signal for target30    */
#define LL_AOS_TARGET_30_TRIGGER_SIMPTC3_CH0_OVL           AOS_PE_TRGSEL30_TRGSEL_SIMPTC3_CH0_OVL            /*!< SIMPTC3 channel 0 overflow signal as trigger signal for target30    */
/**
 * @}
 */

/** @defgroup AOS_LL_EC_TARGET_31_CAN_TRGSEL Target_31 CAN Trigger source
 * @{
 */
#define LL_AOS_TARGET_31_TRIGGER_NONE                      AOS_PE_TRGSEL31_TRGSEL_NONE                       /*!< No trigger signal                                                      */
#define LL_AOS_TARGET_31_TRIGGER_SIMPTC0_CH2_OVL           AOS_PE_TRGSEL31_TRGSEL_SIMPTC0_CH2_OVL            /*!< SIMPTC0 channel 2 count overflow signal as trigger signal for target31 */
#define LL_AOS_TARGET_31_TRIGGER_SIMPTC0_CH3_OVL           AOS_PE_TRGSEL31_TRGSEL_SIMPTC0_CH3_OVL            /*!< SIMPTC0 channel 3 count overflow signal as trigger signal for target31 */
#define LL_AOS_TARGET_31_TRIGGER_SIMPTC1_CH2_OVL           AOS_PE_TRGSEL31_TRGSEL_SIMPTC1_CH2_OVL            /*!< SIMPTC1 channel 2 count overflow signal as trigger signal for target31 */
#define LL_AOS_TARGET_31_TRIGGER_SIMPTC1_CH3_OVL           AOS_PE_TRGSEL31_TRGSEL_SIMPTC1_CH3_OVL            /*!< SIMPTC1 channel 3 count overflow signal as trigger signal for target31 */
#define LL_AOS_TARGET_31_TRIGGER_SIMPTC2_CH2_OVL           AOS_PE_TRGSEL31_TRGSEL_SIMPTC2_CH2_OVL            /*!< SIMPTC2 channel 2 count overflow signal as trigger signal for target31 */
#define LL_AOS_TARGET_31_TRIGGER_SIMPTC2_CH3_OVL           AOS_PE_TRGSEL31_TRGSEL_SIMPTC2_CH3_OVL            /*!< SIMPTC2 channel 3 count overflow signal as trigger signal for target31 */
#define LL_AOS_TARGET_31_TRIGGER_SIMPTC3_CH2_OVL           AOS_PE_TRGSEL31_TRGSEL_SIMPTC3_CH2_OVL            /*!< SIMPTC3 channel 2 count overflow signal as trigger signal for target31 */
#define LL_AOS_TARGET_31_TRIGGER_SIMPTC3_CH3_OVL           AOS_PE_TRGSEL31_TRGSEL_SIMPTC3_CH3_OVL            /*!< SIMPTC3 channel 3 count overflow signal as trigger signal for target31 */
#define LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN0              AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN0               /*!< GPIO TC PIN0 signal as trigger signal for target31                     */
#define LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN1              AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN1               /*!< GPIO TC PIN1 signal as trigger signal for target31                     */
#define LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN2              AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN2               /*!< GPIO TC PIN2 signal as trigger signal for target31                     */
#define LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN3              AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN3               /*!< GPIO TC PIN3 signal as trigger signal for target31                     */
#define LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN4              AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN4               /*!< GPIO TC PIN4 signal as trigger signal for target31                     */
#define LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN5              AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN5               /*!< GPIO TC PIN5 signal as trigger signal for target31                     */
#define LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN6              AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN6               /*!< GPIO TC PIN6 signal as trigger signal for target31                     */
#define LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN7              AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN7               /*!< GPIO TC PIN7 signal as trigger signal for target31                     */
#define LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN8              AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN8               /*!< GPIO TC PIN8 signal as trigger signal for target31                     */
#define LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN9              AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN9               /*!< GPIO TC PIN9 signal as trigger signal for target31                     */
#define LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN10             AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN10              /*!< GPIO TC PIN10 signal as trigger signal for target31                    */
#define LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN11             AOS_PE_TRGSEL31_TRGSEL_GPIO_TC_PIN11              /*!< GPIO TC PIN11 signal as trigger signal for target31                    */
/**
 * @}
 */

/** @defgroup AOS_LL_EC_TARGET_32_33_ADC0_TRGSEL Target_32 and Target_33 ADC0 Trigger source
 * @{
 */
#define LL_AOS_TARGET_32_33_TRIGGER_NONE                   AOS_PE_TRGSEL32_TRGSEL_NONE                       /*!< No trigger signal                                                      */
#define LL_AOS_TARGET_32_TRIGGER_KBI0_CH0_OUTPUT           AOS_PE_TRGSEL32_TRGSEL_KBI0_CH0_OUTPUT            /*!< KBI0 channel 0 output signal as trigger signal for target32            */
#define LL_AOS_TARGET_32_TRIGGER_KBI0_CH1_OUTPUT           AOS_PE_TRGSEL32_TRGSEL_KBI0_CH1_OUTPUT            /*!< KBI0 channel 1 output signal as trigger signal for target32            */
#define LL_AOS_TARGET_32_TRIGGER_KBI0_CH2_OUTPUT           AOS_PE_TRGSEL32_TRGSEL_KBI0_CH2_OUTPUT            /*!< KBI0 channel 2 output signal as trigger signal for target32            */
#define LL_AOS_TARGET_32_TRIGGER_KBI0_CH3_OUTPUT           AOS_PE_TRGSEL32_TRGSEL_KBI0_CH3_OUTPUT            /*!< KBI0 channel 3 output signal as trigger signal for target32            */
#define LL_AOS_TARGET_32_TRIGGER_KBI1_CH0_OUTPUT           AOS_PE_TRGSEL32_TRGSEL_KBI1_CH0_OUTPUT            /*!< KBI1 channel 0 output signal as trigger signal for target32            */
#define LL_AOS_TARGET_32_TRIGGER_KBI1_CH1_OUTPUT           AOS_PE_TRGSEL32_TRGSEL_KBI1_CH1_OUTPUT            /*!< KBI1 channel 1 output signal as trigger signal for target32            */
#define LL_AOS_TARGET_32_TRIGGER_KBI1_CH2_OUTPUT           AOS_PE_TRGSEL32_TRGSEL_KBI1_CH2_OUTPUT            /*!< KBI1 channel 2 output signal as trigger signal for target32            */
#define LL_AOS_TARGET_32_TRIGGER_KBI1_CH3_OUTPUT           AOS_PE_TRGSEL32_TRGSEL_KBI1_CH3_OUTPUT            /*!< KBI1 channel 3 output signal as trigger signal for target32            */
#define LL_AOS_TARGET_32_TRIGGER_SIMPTC0_CH0_OVL           AOS_PE_TRGSEL32_TRGSEL_SIMPTC0_CH0_OVL            /*!< SIMPTC0 channel 0 count overflow signal as trigger signal for target32 */
#define LL_AOS_TARGET_32_TRIGGER_SIMPTC1_CH0_OVL           AOS_PE_TRGSEL32_TRGSEL_SIMPTC1_CH0_OVL            /*!< SIMPTC1 channel 0 count overflow signal as trigger signal for target32 */
#define LL_AOS_TARGET_32_TRIGGER_SIMPTC2_CH0_OVL           AOS_PE_TRGSEL32_TRGSEL_SIMPTC2_CH0_OVL            /*!< SIMPTC2 channel 0 count overflow signal as trigger signal for target32 */
#define LL_AOS_TARGET_32_TRIGGER_SIMPTC3_CH0_OVL           AOS_PE_TRGSEL32_TRGSEL_SIMPTC3_CH0_OVL            /*!< SIMPTC3 channel 0 count overflow signal as trigger signal for target32 */

#define LL_AOS_TARGET_33_TRIGGER_KBI0_CH4_OUTPUT           AOS_PE_TRGSEL33_TRGSEL_KBI0_CH4_OUTPUT            /*!< KBI0 channel 4 output signal as trigger signal for target33            */
#define LL_AOS_TARGET_33_TRIGGER_KBI0_CH5_OUTPUT           AOS_PE_TRGSEL33_TRGSEL_KBI0_CH5_OUTPUT            /*!< KBI0 channel 5 output signal as trigger signal for target33            */
#define LL_AOS_TARGET_33_TRIGGER_KBI0_CH6_OUTPUT           AOS_PE_TRGSEL33_TRGSEL_KBI0_CH6_OUTPUT            /*!< KBI0 channel 6 output signal as trigger signal for target33            */
#define LL_AOS_TARGET_33_TRIGGER_KBI0_CH7_OUTPUT           AOS_PE_TRGSEL33_TRGSEL_KBI0_CH7_OUTPUT            /*!< KBI0 channel 7 output signal as trigger signal for target33            */
#define LL_AOS_TARGET_33_TRIGGER_KBI1_CH4_OUTPUT           AOS_PE_TRGSEL33_TRGSEL_KBI1_CH4_OUTPUT            /*!< KBI1 channel 4 output signal as trigger signal for target33            */
#define LL_AOS_TARGET_33_TRIGGER_KBI1_CH5_OUTPUT           AOS_PE_TRGSEL33_TRGSEL_KBI1_CH5_OUTPUT            /*!< KBI1 channel 5 output signal as trigger signal for target33            */
#define LL_AOS_TARGET_33_TRIGGER_KBI1_CH6_OUTPUT           AOS_PE_TRGSEL33_TRGSEL_KBI1_CH6_OUTPUT            /*!< KBI1 channel 6 output signal as trigger signal for target33            */
#define LL_AOS_TARGET_33_TRIGGER_KBI1_CH7_OUTPUT           AOS_PE_TRGSEL33_TRGSEL_KBI1_CH7_OUTPUT            /*!< KBI1 channel 7 output signal as trigger signal for target33            */
#define LL_AOS_TARGET_33_TRIGGER_SIMPTC0_CH1_OVL           AOS_PE_TRGSEL33_TRGSEL_SIMPTC0_CH1_OVL            /*!< SIMPTC0 channel 1 overflow signal as trigger signal for target33       */
#define LL_AOS_TARGET_33_TRIGGER_SIMPTC1_CH1_OVL           AOS_PE_TRGSEL33_TRGSEL_SIMPTC1_CH1_OVL            /*!< SIMPTC1 channel 1 overflow signal as trigger signal for target33       */
#define LL_AOS_TARGET_33_TRIGGER_SIMPTC2_CH1_OVL           AOS_PE_TRGSEL33_TRGSEL_SIMPTC2_CH1_OVL            /*!< SIMPTC2 channel 1 overflow signal as trigger signal for target33       */
#define LL_AOS_TARGET_33_TRIGGER_SIMPTC3_CH1_OVL           AOS_PE_TRGSEL33_TRGSEL_SIMPTC3_CH1_OVL            /*!< SIMPTC3 channel 1 overflow signal as trigger signal for target33       */
/**
 * @}
 */

/** @defgroup AOS_LL_EC_TARGET_34_DAC_TRGSEL Target_34 DAC Trigger source
 * @{
 */
#define LL_AOS_TARGET_34_TRIGGER_NONE                      AOS_PE_TRGSEL34_TRGSEL_NONE                       /*!< No trigger signal                                                      */
#define LL_AOS_TARGET_34_TRIGGER_KBI0_CH8_OUTPUT           AOS_PE_TRGSEL34_TRGSEL_KBI0_CH8_OUTPUT            /*!< KBI0 channel 8 output signal as trigger signal for target34            */
#define LL_AOS_TARGET_34_TRIGGER_KBI0_CH9_OUTPUT           AOS_PE_TRGSEL34_TRGSEL_KBI0_CH9_OUTPUT            /*!< KBI0 channel 9 output signal as trigger signal for target34            */
#define LL_AOS_TARGET_34_TRIGGER_KBI0_CH10_OUTPUT          AOS_PE_TRGSEL34_TRGSEL_KBI0_CH10_OUTPUT           /*!< KBI0 channel 10 output signal as trigger signal for target34           */
#define LL_AOS_TARGET_34_TRIGGER_KBI0_CH11_OUTPUT          AOS_PE_TRGSEL34_TRGSEL_KBI0_CH11_OUTPUT           /*!< KBI0 channel 11 output signal as trigger signal for target34           */
#define LL_AOS_TARGET_34_TRIGGER_KBI1_CH8_OUTPUT           AOS_PE_TRGSEL34_TRGSEL_KBI1_CH8_OUTPUT            /*!< KBI1 channel 8 output signal as trigger signal for target34            */
#define LL_AOS_TARGET_34_TRIGGER_KBI1_CH9_OUTPUT           AOS_PE_TRGSEL34_TRGSEL_KBI1_CH9_OUTPUT            /*!< KBI1 channel 9 output signal as trigger signal for target34            */
#define LL_AOS_TARGET_34_TRIGGER_KBI1_CH10_OUTPUT          AOS_PE_TRGSEL34_TRGSEL_KBI1_CH10_OUTPUT           /*!< KBI1 channel 10 output signal as trigger signal for target34           */
#define LL_AOS_TARGET_34_TRIGGER_KBI1_CH11_OUTPUT          AOS_PE_TRGSEL34_TRGSEL_KBI1_CH11_OUTPUT           /*!< KBI1 channel 11 output signal as trigger signal for target34           */
#define LL_AOS_TARGET_34_TRIGGER_SIMPTC0_CH2_OVL           AOS_PE_TRGSEL34_TRGSEL_SIMPTC0_CH2_OVL            /*!< SIMPTC0 channel 2 count overflow signal as trigger signal for target34 */
#define LL_AOS_TARGET_34_TRIGGER_SIMPTC1_CH2_OVL           AOS_PE_TRGSEL34_TRGSEL_SIMPTC1_CH2_OVL            /*!< SIMPTC1 channel 2 count overflow signal as trigger signal for target34 */
#define LL_AOS_TARGET_34_TRIGGER_SIMPTC2_CH2_OVL           AOS_PE_TRGSEL34_TRGSEL_SIMPTC2_CH2_OVL            /*!< SIMPTC2 channel 2 count overflow signal as trigger signal for target34 */
#define LL_AOS_TARGET_34_TRIGGER_SIMPTC3_CH2_OVL           AOS_PE_TRGSEL34_TRGSEL_SIMPTC3_CH2_OVL            /*!< SIMPTC3 channel 2 count overflow signal as trigger signal for target34 */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup AOS_LL_Exported_Macros AOS Exported Macros
 * @{
 */
/** @defgroup AOS_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in AOS register
 * @param  __INSTANCE__ AOS Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_AOS_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in AOS register
 * @param  __INSTANCE__ AOS Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_AOS_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup AOS_LL_Exported_Functions AOS Exported Functions
 * @{
 */

/** @defgroup AOS_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief  Enable AOS Security peripheral (EN = 0xA5).
 * @rmtoll SEC_EN           SEC_EN         LL_AOS_EnableSecurityPeriph
 * @retval None
 */
__STATIC_INLINE void LL_AOS_EnableSecurityPeriph(void)
{
    MODIFY_REG(AOS_CORE0->SEC_EN, AOS_SEC_EN_SEC_EN, 0xA5U);
}

/**
 * @brief  Disable AOS Security peripheral (EN != 0xA5).
 * @rmtoll SEC_EN           SEC_EN         LL_AOS_DisableSecurityPeriph
 * @retval None
 */
__STATIC_INLINE void LL_AOS_DisableSecurityPeriph(void)
{
    MODIFY_REG(AOS_CORE0->SEC_EN, AOS_SEC_EN_SEC_EN, 0x00U);
}

/**
 * @brief  Check if the AOS peripheral is enabled or disabled.
 * @rmtoll SEC_EN           SEC_EN         LL_AOS_IsEnabledSecurityPeriph
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_AOS_IsEnabledSecurityPeriph(void)
{
    return (READ_REG(AOS_CORE0->SEC_EN) == (0xA5U));
}

/**
 * @brief  Enable AOS Non-Security peripheral (EN = 0x8E).
 * @rmtoll NSEC_EN           NSEC_EN         LL_AOS_EnableNonSecurityPeriph
 * @retval None
 */
__STATIC_INLINE void LL_AOS_EnableNonSecurityPeriph(void)
{
    MODIFY_REG(AOS_CORE1->NSEC_EN, AOS_NSEC_EN_NSEC_EN, 0x8EU);
}

/**
 * @brief  Disable AOS Non-Security peripheral (EN != 0x8E).
 * @rmtoll NSEC_EN           NSEC_EN         LL_AOS_DisableNonSecurityPeriph
 * @retval None
 */
__STATIC_INLINE void LL_AOS_DisableNonSecurityPeriph(void)
{
    MODIFY_REG(AOS_CORE1->NSEC_EN, AOS_NSEC_EN_NSEC_EN, 0x00U);
}

/**
 * @brief  Check if the AOS Non-Security is enabled or disabled.
 * @rmtoll NSEC_EN           NSEC_EN         LL_AOS_IsEnabledNonSecurityPeriph
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_AOS_IsEnabledNonSecurityPeriph(void)
{
    return (READ_REG(AOS_CORE1->NSEC_EN) == (0x8EU));
}
/**
 * @}
 */

/** @defgroup AOS_LL_EF_Trigger_Management Trigger_Management
 * @{
 */

/**
 * @brief  Set Trigger source for target0-target3 peripheral TC0.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_SetTriggerSignal_TC0
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_0_TC0
 *         @arg @ref LL_AOS_TARGET_1_TC0
 *         @arg @ref LL_AOS_TARGET_2_TC0
 *         @arg @ref LL_AOS_TARGET_3_TC0
 * @param TriggerSignal This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART0_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART1_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART2_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART3_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART4_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART5_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART6_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART7_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_LPUART_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_USART0_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_USART0_TX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_USART1_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_USART1_TX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_ISO78160_IO_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_ISO78160_IN_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_ISO78161_IO_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_ISO78161_IN_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_CAN_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_D2F_CF_OUT0
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_D2F_CF_OUT1
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_TC1_CH0_POS
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_TC1_CH0_NEG
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_TC1_CH1_POS
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_TC1_CH1_NEG
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_VBAT_RTC_OUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN0
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN1
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN2
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN3
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN4
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN5
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN6
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN7
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN8
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN9
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN10
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN11
 * @retval None
 */
__STATIC_INLINE void LL_AOS_SetTriggerSignal_TC0(AOS_TypeDef *AOSx, uint32_t TargetPurpose, uint32_t TriggerSignal)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    MODIFY_REG(*pReg, AOS_PE_TRGSEL0_TRGSEL, TriggerSignal);
}

/**
 * @brief  Get Trigger source for target0-target3 peripheral TC0.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_GetTriggerSignal_TC0
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_0_TC0
 *         @arg @ref LL_AOS_TARGET_1_TC0
 *         @arg @ref LL_AOS_TARGET_2_TC0
 *         @arg @ref LL_AOS_TARGET_3_TC0
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART0_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART1_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART2_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART3_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART4_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART5_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART6_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_UART7_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_LPUART_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_USART0_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_USART0_TX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_USART1_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_USART1_TX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_ISO78160_IO_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_ISO78160_IN_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_ISO78161_IO_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_ISO78161_IN_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_CAN_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_D2F_CF_OUT0
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_D2F_CF_OUT1
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_TC1_CH0_POS
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_TC1_CH0_NEG
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_TC1_CH1_POS
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_TC1_CH1_NEG
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_VBAT_RTC_OUT
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN0
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN1
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN2
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN3
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN4
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN5
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN6
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN7
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN8
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN9
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN10
 *         @arg @ref LL_AOS_TARGET_0_3_TRIGGER_GPIO_TC_PIN11
 */
__STATIC_INLINE uint32_t LL_AOS_GetTriggerSignal_TC0(AOS_TypeDef *AOSx, uint32_t TargetPurpose)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    return (READ_BIT(*pReg, AOS_PE_TRGSEL0_TRGSEL));
}

/**
 * @brief  Set Trigger source for target4 to target7 peripheral TC1.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_SetTriggerSignal_TC1
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_4_TC1
 *         @arg @ref LL_AOS_TARGET_5_TC1
 *         @arg @ref LL_AOS_TARGET_6_TC1
 *         @arg @ref LL_AOS_TARGET_7_TC1
 * @param TriggerSignal This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART0_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART1_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART2_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART3_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART4_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART5_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART6_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART7_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_LPUART_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_USART0_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_USART0_TX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_USART1_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_USART1_TX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_ISO78160_IO_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_ISO78160_IN_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_ISO78161_IO_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_ISO78161_IN_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_CAN_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_D2F_CF_OUT1
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_D2F_CF_OUT2
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_TC0_CH0_POS
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_TC0_CH0_NEG
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_TC0_CH1_POS
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_TC0_CH1_NEG
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_VBAT_RTC_OUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN0
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN1
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN2
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN3
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN4
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN5
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN6
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN7
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN8
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN9
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN10
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN11
 * @retval None
 */
__STATIC_INLINE void LL_AOS_SetTriggerSignal_TC1(AOS_TypeDef *AOSx, uint32_t TargetPurpose, uint32_t TriggerSignal)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    MODIFY_REG(*pReg, AOS_PE_TRGSEL4_TRGSEL, TriggerSignal);
}

/**
 * @brief  Get Trigger source for target4 to target7 peripheral TC1.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_GetTriggerSignal_TC1
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_4_TC1
 *         @arg @ref LL_AOS_TARGET_5_TC1
 *         @arg @ref LL_AOS_TARGET_6_TC1
 *         @arg @ref LL_AOS_TARGET_7_TC1
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART0_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART1_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART2_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART3_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART4_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART5_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART6_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_UART7_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_LPUART_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_USART0_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_USART0_TX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_USART1_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_USART1_TX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_ISO78160_IO_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_ISO78160_IN_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_ISO78161_IO_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_ISO78161_IN_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_CAN_RX_INPUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_D2F_CF_OUT1
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_D2F_CF_OUT2
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_TC0_CH0_POS
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_TC0_CH0_NEG
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_TC0_CH1_POS
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_TC0_CH1_NEG
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_VBAT_RTC_OUT
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN0
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN1
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN2
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN3
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN4
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN5
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN6
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN7
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN8
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN9
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN10
 *         @arg @ref LL_AOS_TARGET_4_7_TRIGGER_GPIO_TC_PIN11
 */
__STATIC_INLINE uint32_t LL_AOS_GetTriggerSignal_TC1(AOS_TypeDef *AOSx, uint32_t TargetPurpose)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    return (READ_BIT(*pReg, AOS_PE_TRGSEL4_TRGSEL));
}

/**
 * @brief  Set Trigger source for target8 to target11 peripheral SIMPTC0.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_SetTriggerSignal_SIMPTC0
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_8_SIMPTC0
 *         @arg @ref LL_AOS_TARGET_9_SIMPTC0
 *         @arg @ref LL_AOS_TARGET_10_SIMPTC0
 *         @arg @ref LL_AOS_TARGET_11_SIMPTC0
 * @param TriggerSignal This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_8_11_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_8_TRIGGER_KBI0_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_8_TRIGGER_KBI0_CH4_OUTPUT
 *         @arg @ref LL_AOS_TARGET_8_TRIGGER_KBI1_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_8_TRIGGER_KBI1_CH4_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_9_TRIGGER_KBI0_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_9_TRIGGER_KBI0_CH5_OUTPUT
 *         @arg @ref LL_AOS_TARGET_9_TRIGGER_KBI1_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_9_TRIGGER_KBI1_CH5_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_10_TRIGGER_KBI0_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_10_TRIGGER_KBI0_CH6_OUTPUT
 *         @arg @ref LL_AOS_TARGET_10_TRIGGER_KBI1_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_10_TRIGGER_KBI1_CH6_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_11_TRIGGER_KBI0_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_11_TRIGGER_KBI0_CH7_OUTPUT
 *         @arg @ref LL_AOS_TARGET_11_TRIGGER_KBI1_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_11_TRIGGER_KBI1_CH7_OUTPUT
 * @retval None
 */
__STATIC_INLINE void LL_AOS_SetTriggerSignal_SIMPTC0(AOS_TypeDef *AOSx, uint32_t TargetPurpose, uint32_t TriggerSignal)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    MODIFY_REG(*pReg, AOS_PE_TRGSEL8_TRGSEL, TriggerSignal);
}

/**
 * @brief  Get Trigger source for target8 to target11 peripheral SIMPTC0.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_GetTriggerSignal_SIMPTC0
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_8_SIMPTC0
 *         @arg @ref LL_AOS_TARGET_9_SIMPTC0
 *         @arg @ref LL_AOS_TARGET_10_SIMPTC0
 *         @arg @ref LL_AOS_TARGET_11_SIMPTC0
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_8_11_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_8_TRIGGER_KBI0_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_8_TRIGGER_KBI0_CH4_OUTPUT
 *         @arg @ref LL_AOS_TARGET_8_TRIGGER_KBI1_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_8_TRIGGER_KBI1_CH4_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_9_TRIGGER_KBI0_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_9_TRIGGER_KBI0_CH5_OUTPUT
 *         @arg @ref LL_AOS_TARGET_9_TRIGGER_KBI1_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_9_TRIGGER_KBI1_CH5_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_10_TRIGGER_KBI0_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_10_TRIGGER_KBI0_CH6_OUTPUT
 *         @arg @ref LL_AOS_TARGET_10_TRIGGER_KBI1_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_10_TRIGGER_KBI1_CH6_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_11_TRIGGER_KBI0_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_11_TRIGGER_KBI0_CH7_OUTPUT
 *         @arg @ref LL_AOS_TARGET_11_TRIGGER_KBI1_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_11_TRIGGER_KBI1_CH7_OUTPUT
 */
__STATIC_INLINE uint32_t LL_AOS_GetTriggerSignal_SIMPTC0(AOS_TypeDef *AOSx, uint32_t TargetPurpose)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    return (READ_BIT(*pReg, AOS_PE_TRGSEL8_TRGSEL));
}

/**
 * @brief  Set Trigger source for target12 to target15 peripheral SIMPTC1.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_SetTriggerSignal_SIMPTC1
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_12_SIMPTC1
 *         @arg @ref LL_AOS_TARGET_13_SIMPTC1
 *         @arg @ref LL_AOS_TARGET_14_SIMPTC1
 *         @arg @ref LL_AOS_TARGET_15_SIMPTC1
 * @param TriggerSignal This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_12_15_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_12_TRIGGER_KBI0_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_12_TRIGGER_KBI0_CH4_OUTPUT
 *         @arg @ref LL_AOS_TARGET_12_TRIGGER_KBI1_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_12_TRIGGER_KBI1_CH4_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_13_TRIGGER_KBI0_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_13_TRIGGER_KBI0_CH5_OUTPUT
 *         @arg @ref LL_AOS_TARGET_13_TRIGGER_KBI1_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_13_TRIGGER_KBI1_CH5_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_14_TRIGGER_KBI0_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_14_TRIGGER_KBI0_CH6_OUTPUT
 *         @arg @ref LL_AOS_TARGET_14_TRIGGER_KBI1_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_14_TRIGGER_KBI1_CH6_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_15_TRIGGER_KBI0_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_15_TRIGGER_KBI0_CH7_OUTPUT
 *         @arg @ref LL_AOS_TARGET_15_TRIGGER_KBI1_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_15_TRIGGER_KBI1_CH7_OUTPUT
 * @retval None
 */
__STATIC_INLINE void LL_AOS_SetTriggerSignal_SIMPTC1(AOS_TypeDef *AOSx, uint32_t TargetPurpose, uint32_t TriggerSignal)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    MODIFY_REG(*pReg, AOS_PE_TRGSEL12_TRGSEL, TriggerSignal);
}

/**
 * @brief  Get Trigger source for target12 to target15 peripheral SIMPTC1.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_GetTriggerSignal_SIMPTC1
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_12_SIMPTC1
 *         @arg @ref LL_AOS_TARGET_13_SIMPTC1
 *         @arg @ref LL_AOS_TARGET_14_SIMPTC1
 *         @arg @ref LL_AOS_TARGET_15_SIMPTC1
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_12_15_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_12_TRIGGER_KBI0_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_12_TRIGGER_KBI0_CH4_OUTPUT
 *         @arg @ref LL_AOS_TARGET_12_TRIGGER_KBI1_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_12_TRIGGER_KBI1_CH4_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_13_TRIGGER_KBI0_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_13_TRIGGER_KBI0_CH5_OUTPUT
 *         @arg @ref LL_AOS_TARGET_13_TRIGGER_KBI1_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_13_TRIGGER_KBI1_CH5_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_14_TRIGGER_KBI0_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_14_TRIGGER_KBI0_CH6_OUTPUT
 *         @arg @ref LL_AOS_TARGET_14_TRIGGER_KBI1_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_14_TRIGGER_KBI1_CH6_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_15_TRIGGER_KBI0_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_15_TRIGGER_KBI0_CH7_OUTPUT
 *         @arg @ref LL_AOS_TARGET_15_TRIGGER_KBI1_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_15_TRIGGER_KBI1_CH7_OUTPUT
 */
__STATIC_INLINE uint32_t LL_AOS_GetTriggerSignal_SIMPTC1(AOS_TypeDef *AOSx, uint32_t TargetPurpose)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    return (READ_BIT(*pReg, AOS_PE_TRGSEL12_TRGSEL));
}

/**
 * @brief  Set Trigger source for target16 to target19 peripheral SIMPTC2.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_SetTriggerSignal_SIMPTC2
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_16_SIMPTC2
 *         @arg @ref LL_AOS_TARGET_17_SIMPTC2
 *         @arg @ref LL_AOS_TARGET_18_SIMPTC2
 *         @arg @ref LL_AOS_TARGET_19_SIMPTC2
 * @param TriggerSignal This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_16_19_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_16_TRIGGER_KBI0_CH8_OUTPUT
 *         @arg @ref LL_AOS_TARGET_16_TRIGGER_KBI0_CH12_OUTPUT
 *         @arg @ref LL_AOS_TARGET_16_TRIGGER_KBI1_CH8_OUTPUT
 *         @arg @ref LL_AOS_TARGET_16_TRIGGER_KBI1_CH12_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_17_TRIGGER_KBI0_CH9_OUTPUT
 *         @arg @ref LL_AOS_TARGET_17_TRIGGER_KBI0_CH13_OUTPUT
 *         @arg @ref LL_AOS_TARGET_17_TRIGGER_KBI1_CH9_OUTPUT
 *         @arg @ref LL_AOS_TARGET_17_TRIGGER_KBI1_CH13_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_18_TRIGGER_KBI0_CH10_OUTPUT
 *         @arg @ref LL_AOS_TARGET_18_TRIGGER_KBI0_CH14_OUTPUT
 *         @arg @ref LL_AOS_TARGET_18_TRIGGER_KBI1_CH10_OUTPUT
 *         @arg @ref LL_AOS_TARGET_18_TRIGGER_KBI1_CH14_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_19_TRIGGER_KBI0_CH11_OUTPUT
 *         @arg @ref LL_AOS_TARGET_19_TRIGGER_KBI0_CH15_OUTPUT
 *         @arg @ref LL_AOS_TARGET_19_TRIGGER_KBI1_CH11_OUTPUT
 *         @arg @ref LL_AOS_TARGET_19_TRIGGER_KBI1_CH15_OUTPUT
 * @retval None
 */
__STATIC_INLINE void LL_AOS_SetTriggerSignal_SIMPTC2(AOS_TypeDef *AOSx, uint32_t TargetPurpose, uint32_t TriggerSignal)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    MODIFY_REG(*pReg, AOS_PE_TRGSEL16_TRGSEL, TriggerSignal);
}

/**
 * @brief  Get Trigger source for target16 to target19 peripheral SIMPTC2.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_GetTriggerSignal_SIMPTC2
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_16_SIMPTC2
 *         @arg @ref LL_AOS_TARGET_17_SIMPTC2
 *         @arg @ref LL_AOS_TARGET_18_SIMPTC2
 *         @arg @ref LL_AOS_TARGET_19_SIMPTC2
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_16_19_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_16_TRIGGER_KBI0_CH8_OUTPUT
 *         @arg @ref LL_AOS_TARGET_16_TRIGGER_KBI0_CH12_OUTPUT
 *         @arg @ref LL_AOS_TARGET_16_TRIGGER_KBI1_CH8_OUTPUT
 *         @arg @ref LL_AOS_TARGET_16_TRIGGER_KBI1_CH12_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_17_TRIGGER_KBI0_CH9_OUTPUT
 *         @arg @ref LL_AOS_TARGET_17_TRIGGER_KBI0_CH13_OUTPUT
 *         @arg @ref LL_AOS_TARGET_17_TRIGGER_KBI1_CH9_OUTPUT
 *         @arg @ref LL_AOS_TARGET_17_TRIGGER_KBI1_CH13_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_18_TRIGGER_KBI0_CH10_OUTPUT
 *         @arg @ref LL_AOS_TARGET_18_TRIGGER_KBI0_CH14_OUTPUT
 *         @arg @ref LL_AOS_TARGET_18_TRIGGER_KBI1_CH10_OUTPUT
 *         @arg @ref LL_AOS_TARGET_18_TRIGGER_KBI1_CH14_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_19_TRIGGER_KBI0_CH11_OUTPUT
 *         @arg @ref LL_AOS_TARGET_19_TRIGGER_KBI0_CH15_OUTPUT
 *         @arg @ref LL_AOS_TARGET_19_TRIGGER_KBI1_CH11_OUTPUT
 *         @arg @ref LL_AOS_TARGET_19_TRIGGER_KBI1_CH15_OUTPUT
 */
__STATIC_INLINE uint32_t LL_AOS_GetTriggerSignal_SIMPTC2(AOS_TypeDef *AOSx, uint32_t TargetPurpose)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    return (READ_BIT(*pReg, AOS_PE_TRGSEL16_TRGSEL));
}

/**
 * @brief  Set Trigger source for target20 to target23 peripheral SIMPTC3.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_SetTriggerSignal_SIMPTC3
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_20_SIMPTC3
 *         @arg @ref LL_AOS_TARGET_21_SIMPTC3
 *         @arg @ref LL_AOS_TARGET_22_SIMPTC3
 *         @arg @ref LL_AOS_TARGET_23_SIMPTC3
 * @param TriggerSignal This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_20_23_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_20_TRIGGER_KBI0_CH8_OUTPUT
 *         @arg @ref LL_AOS_TARGET_20_TRIGGER_KBI0_CH12_OUTPUT
 *         @arg @ref LL_AOS_TARGET_20_TRIGGER_KBI1_CH8_OUTPUT
 *         @arg @ref LL_AOS_TARGET_20_TRIGGER_KBI1_CH12_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_21_TRIGGER_KBI0_CH9_OUTPUT
 *         @arg @ref LL_AOS_TARGET_21_TRIGGER_KBI0_CH13_OUTPUT
 *         @arg @ref LL_AOS_TARGET_21_TRIGGER_KBI1_CH9_OUTPUT
 *         @arg @ref LL_AOS_TARGET_21_TRIGGER_KBI1_CH13_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_22_TRIGGER_KBI0_CH10_OUTPUT
 *         @arg @ref LL_AOS_TARGET_22_TRIGGER_KBI0_CH14_OUTPUT
 *         @arg @ref LL_AOS_TARGET_22_TRIGGER_KBI1_CH10_OUTPUT
 *         @arg @ref LL_AOS_TARGET_22_TRIGGER_KBI1_CH14_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_23_TRIGGER_KBI0_CH11_OUTPUT
 *         @arg @ref LL_AOS_TARGET_23_TRIGGER_KBI0_CH15_OUTPUT
 *         @arg @ref LL_AOS_TARGET_23_TRIGGER_KBI1_CH11_OUTPUT
 *         @arg @ref LL_AOS_TARGET_23_TRIGGER_KBI1_CH15_OUTPUT
 * @retval None
 */
__STATIC_INLINE void LL_AOS_SetTriggerSignal_SIMPTC3(AOS_TypeDef *AOSx, uint32_t TargetPurpose, uint32_t TriggerSignal)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    MODIFY_REG(*pReg, AOS_PE_TRGSEL20_TRGSEL, TriggerSignal);
}

/**
 * @brief  Get Trigger source for target20 to target23 peripheral SIMPTC3.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_GetTriggerSignal_SIMPTC3
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_20_SIMPTC3
 *         @arg @ref LL_AOS_TARGET_21_SIMPTC3
 *         @arg @ref LL_AOS_TARGET_22_SIMPTC3
 *         @arg @ref LL_AOS_TARGET_23_SIMPTC3
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_20_23_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_20_TRIGGER_KBI0_CH8_OUTPUT
 *         @arg @ref LL_AOS_TARGET_20_TRIGGER_KBI0_CH12_OUTPUT
 *         @arg @ref LL_AOS_TARGET_20_TRIGGER_KBI1_CH8_OUTPUT
 *         @arg @ref LL_AOS_TARGET_20_TRIGGER_KBI1_CH12_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_21_TRIGGER_KBI0_CH9_OUTPUT
 *         @arg @ref LL_AOS_TARGET_21_TRIGGER_KBI0_CH13_OUTPUT
 *         @arg @ref LL_AOS_TARGET_21_TRIGGER_KBI1_CH9_OUTPUT
 *         @arg @ref LL_AOS_TARGET_21_TRIGGER_KBI1_CH13_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_22_TRIGGER_KBI0_CH10_OUTPUT
 *         @arg @ref LL_AOS_TARGET_22_TRIGGER_KBI0_CH14_OUTPUT
 *         @arg @ref LL_AOS_TARGET_22_TRIGGER_KBI1_CH10_OUTPUT
 *         @arg @ref LL_AOS_TARGET_22_TRIGGER_KBI1_CH14_OUTPUT
 *
 *         @arg @ref LL_AOS_TARGET_23_TRIGGER_KBI0_CH11_OUTPUT
 *         @arg @ref LL_AOS_TARGET_23_TRIGGER_KBI0_CH15_OUTPUT
 *         @arg @ref LL_AOS_TARGET_23_TRIGGER_KBI1_CH11_OUTPUT
 *         @arg @ref LL_AOS_TARGET_23_TRIGGER_KBI1_CH15_OUTPUT
 */
__STATIC_INLINE uint32_t LL_AOS_GetTriggerSignal_SIMPTC3(AOS_TypeDef *AOSx, uint32_t TargetPurpose)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    return (READ_BIT(*pReg, AOS_PE_TRGSEL20_TRGSEL));
}

/**
 * @brief  Set Trigger source for target24 peripheral D2F.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_SetTriggerSignal_D2F
 * @param  AOSx AOS Instance.
 * @param TriggerSignal This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_24_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_24_TRIGGER_SIMPTC0_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_24_TRIGGER_SIMPTC1_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_24_TRIGGER_SIMPTC2_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_24_TRIGGER_SIMPTC3_CH0_OVL
 * @retval None
 */
__STATIC_INLINE void LL_AOS_SetTriggerSignal_D2F(AOS_TypeDef *AOSx, uint32_t TriggerSignal)
{
    MODIFY_REG(AOSx->PE_TRGSEL24, AOS_PE_TRGSEL24_TRGSEL, TriggerSignal);
}

/**
 * @brief  Get Trigger source for target24 peripheral D2F.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_GetTriggerSignal_D2F
 * @param  AOSx AOS Instance.
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_24_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_24_TRIGGER_SIMPTC0_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_24_TRIGGER_SIMPTC1_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_24_TRIGGER_SIMPTC2_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_24_TRIGGER_SIMPTC3_CH0_OVL
 */
__STATIC_INLINE uint32_t LL_AOS_GetTriggerSignal_D2F(AOS_TypeDef *AOSx)
{
    return (READ_BIT(AOSx->PE_TRGSEL24, AOS_PE_TRGSEL24_TRGSEL));
}

/**
 * @brief  Set Trigger source for target25 to target29 peripheral IOCNT.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_SetTriggerSignal_IOCNT
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_25_IOCNT
 *         @arg @ref LL_AOS_TARGET_26_IOCNT
 *         @arg @ref LL_AOS_TARGET_27_IOCNT
 *         @arg @ref LL_AOS_TARGET_28_IOCNT
 *         @arg @ref LL_AOS_TARGET_29_IOCNT
 * @param TriggerSignal This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_UART6_PF_OUTPUT
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_UART6_QF_OUTPUT
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_UART6_FPF_OUTPUT
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_UART7_PF_OUTPUT
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_UART7_QF_OUTPUT
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_UART7_FPF_OUTPUT
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_D2F_CF_OUT0
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_D2F_CF_OUT1
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_D2F_CF_OUT2
 *
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI0_CH0_INPUT
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI0_CH1_INPUT
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI0_CH2_INPUT
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI0_CH3_INPUT
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI1_CH0_INPUT
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI1_CH1_INPUT
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI1_CH2_INPUT
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI1_CH3_INPUT
 *
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI0_CH4_INPUT
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI0_CH5_INPUT
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI0_CH6_INPUT
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI0_CH7_INPUT
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI1_CH4_INPUT
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI1_CH5_INPUT
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI1_CH6_INPUT
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI1_CH7_INPUT
 *
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI0_CH8_INPUT
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI0_CH9_INPUT
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI0_CH10_INPUT
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI0_CH11_INPUT
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI1_CH8_INPUT
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI1_CH9_INPUT
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI1_CH10_INPUT
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI1_CH11_INPUT

 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI0_CH12_INPUT
 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI0_CH13_INPUT
 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI0_CH14_INPUT
 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI0_CH15_INPUT
 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI1_CH12_INPUT
 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI1_CH13_INPUT
 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI1_CH14_INPUT
 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI1_CH15_INPUT

 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI0_CH0_INPUT
 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI0_CH1_INPUT
 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI0_CH2_INPUT
 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI0_CH3_INPUT
 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI1_CH0_INPUT
 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI1_CH1_INPUT
 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI1_CH2_INPUT
 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI1_CH3_INPUT
 * @retval None
 */
__STATIC_INLINE void LL_AOS_SetTriggerSignal_IOCNT(AOS_TypeDef *AOSx, uint32_t TargetPurpose, uint32_t TriggerSignal)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    MODIFY_REG(*pReg, AOS_PE_TRGSEL25_TRGSEL, TriggerSignal);
}

/**
 * @brief  Get Trigger source for target25 to target29 peripheral IOCNT.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_GetTriggerSignal_IOCNT
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_25_IOCNT
 *         @arg @ref LL_AOS_TARGET_26_IOCNT
 *         @arg @ref LL_AOS_TARGET_27_IOCNT
 *         @arg @ref LL_AOS_TARGET_28_IOCNT
 *         @arg @ref LL_AOS_TARGET_29_IOCNT
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_UART6_PF_OUTPUT
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_UART6_QF_OUTPUT
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_UART6_FPF_OUTPUT
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_UART7_PF_OUTPUT
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_UART7_QF_OUTPUT
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_UART7_FPF_OUTPUT
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_D2F_CF_OUT0
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_D2F_CF_OUT1
 *         @arg @ref LL_AOS_TARGET_25_29_TRIGGER_D2F_CF_OUT2
 *
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI0_CH0_INPUT
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI0_CH1_INPUT
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI0_CH2_INPUT
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI0_CH3_INPUT
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI1_CH0_INPUT
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI1_CH1_INPUT
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI1_CH2_INPUT
 *         @arg @ref LL_AOS_TARGET_25_TRIGGER_KBI1_CH3_INPUT
 *
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI0_CH4_INPUT
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI0_CH5_INPUT
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI0_CH6_INPUT
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI0_CH7_INPUT
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI1_CH4_INPUT
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI1_CH5_INPUT
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI1_CH6_INPUT
 *         @arg @ref LL_AOS_TARGET_26_TRIGGER_KBI1_CH7_INPUT
 *
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI0_CH8_INPUT
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI0_CH9_INPUT
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI0_CH10_INPUT
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI0_CH11_INPUT
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI1_CH8_INPUT
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI1_CH9_INPUT
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI1_CH10_INPUT
 *         @arg @ref LL_AOS_TARGET_27_TRIGGER_KBI1_CH11_INPUT

 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI0_CH12_INPUT
 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI0_CH13_INPUT
 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI0_CH14_INPUT
 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI0_CH15_INPUT
 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI1_CH12_INPUT
 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI1_CH13_INPUT
 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI1_CH14_INPUT
 *         @arg @ref LL_AOS_TARGET_28_TRIGGER_KBI1_CH15_INPUT

 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI0_CH0_INPUT
 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI0_CH1_INPUT
 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI0_CH2_INPUT
 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI0_CH3_INPUT
 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI1_CH0_INPUT
 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI1_CH1_INPUT
 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI1_CH2_INPUT
 *         @arg @ref LL_AOS_TARGET_29_TRIGGER_KBI1_CH3_INPUT
 */
__STATIC_INLINE uint32_t LL_AOS_GetTriggerSignal_IOCNT(AOS_TypeDef *AOSx, uint32_t TargetPurpose)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    return (READ_BIT(*pReg, AOS_PE_TRGSEL25_TRGSEL));
}

/**
 * @brief  Set Trigger source for target30 peripheral RC_CAL (LL_AOS_TARGET_30_RC_CAL).
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_SetTriggerSignal_RC_CAL
 * @param  AOSx AOS Instance.
 * @param TriggerSignal This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI0_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI0_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI0_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI0_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI1_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI1_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI1_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI1_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_SIMPTC0_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_SIMPTC1_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_SIMPTC2_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_SIMPTC3_CH0_OVL
 * @retval None
 */
__STATIC_INLINE void LL_AOS_SetTriggerSignal_RC_CAL(AOS_TypeDef *AOSx, uint32_t TriggerSignal)
{
    MODIFY_REG(AOSx->PE_TRGSEL30, AOS_PE_TRGSEL30_TRGSEL, TriggerSignal);
}

/**
 * @brief  Get Trigger source for target30 peripheral RC_CAL (LL_AOS_TARGET_30_RC_CAL).
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_GetTriggerSignal_RC_CAL
 * @param  AOSx AOS Instance.
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI0_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI0_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI0_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI0_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI1_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI1_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI1_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_KBI1_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_SIMPTC0_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_SIMPTC1_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_SIMPTC2_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_30_TRIGGER_SIMPTC3_CH0_OVL
 */
__STATIC_INLINE uint32_t LL_AOS_GetTriggerSignal_RC_CAL(AOS_TypeDef *AOSx)
{
    return (READ_BIT(AOSx->PE_TRGSEL30, AOS_PE_TRGSEL30_TRGSEL));
}

/**
 * @brief  Set Trigger source for target31 peripheral CAN (LL_AOS_TARGET_31_CAN).
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_SetTriggerSignal_CAN
 * @param  AOSx AOS Instance.
 * @param TriggerSignal This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC0_CH2_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC0_CH3_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC1_CH2_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC1_CH3_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC2_CH2_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC2_CH3_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC3_CH2_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC3_CH3_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN0
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN1
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN2
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN3
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN4
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN5
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN6
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN7
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN8
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN9
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN10
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN11
 * @retval None
 */
__STATIC_INLINE void LL_AOS_SetTriggerSignal_CAN(AOS_TypeDef *AOSx, uint32_t TriggerSignal)
{
    MODIFY_REG(AOSx->PE_TRGSEL31, AOS_PE_TRGSEL31_TRGSEL, TriggerSignal);
}

/**
 * @brief  Get Trigger source for target31 peripheral CAN (LL_AOS_TARGET_31_CAN).
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_GetTriggerSignal_CAN
 * @param  AOSx AOS Instance.
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC0_CH2_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC0_CH3_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC1_CH2_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC1_CH3_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC2_CH2_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC2_CH3_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC3_CH2_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_SIMPTC3_CH3_OVL
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN0
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN1
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN2
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN3
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN4
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN5
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN6
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN7
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN8
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN9
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN10
 *         @arg @ref LL_AOS_TARGET_31_TRIGGER_GPIO_TC_PIN11
 */
__STATIC_INLINE uint32_t LL_AOS_GetTriggerSignal_CAN(AOS_TypeDef *AOSx)
{
    return (READ_BIT(AOSx->PE_TRGSEL31, AOS_PE_TRGSEL31_TRGSEL));
}

/**
 * @brief  Set Trigger source for target32 to target33 peripheral ADC0.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_SetTriggerSignal_ADC0
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_32_ADC0
 *         @arg @ref LL_AOS_TARGET_33_ADC0
 * @param TriggerSignal This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_32_33_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI0_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI0_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI0_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI0_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI1_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI1_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI1_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI1_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_SIMPTC0_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_SIMPTC1_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_SIMPTC2_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_SIMPTC3_CH0_OVL
 *
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI0_CH4_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI0_CH5_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI0_CH6_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI0_CH7_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI1_CH4_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI1_CH5_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI1_CH6_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI1_CH7_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_SIMPTC0_CH1_OVL
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_SIMPTC1_CH1_OVL
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_SIMPTC2_CH1_OVL
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_SIMPTC3_CH1_OVL
 * @retval None
 */
__STATIC_INLINE void LL_AOS_SetTriggerSignal_ADC0(AOS_TypeDef *AOSx, uint32_t TargetPurpose, uint32_t TriggerSignal)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    MODIFY_REG(*pReg, AOS_PE_TRGSEL32_TRGSEL, TriggerSignal);
}

/**
 * @brief  Get Trigger source for target32 to target33 peripheral ADC0.
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_GetTriggerSignal_ADC0
 * @param  AOSx AOS Instance.
 * @param  TargetPurpose This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_32_ADC0
 *         @arg @ref LL_AOS_TARGET_33_ADC0
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_32_33_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI0_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI0_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI0_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI0_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI1_CH0_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI1_CH1_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI1_CH2_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_KBI1_CH3_OUTPUT
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_SIMPTC0_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_SIMPTC1_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_SIMPTC2_CH0_OVL
 *         @arg @ref LL_AOS_TARGET_32_TRIGGER_SIMPTC3_CH0_OVL
 *
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI0_CH4_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI0_CH5_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI0_CH6_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI0_CH7_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI1_CH4_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI1_CH5_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI1_CH6_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_KBI1_CH7_OUTPUT
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_SIMPTC0_CH1_OVL
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_SIMPTC1_CH1_OVL
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_SIMPTC2_CH1_OVL
 *         @arg @ref LL_AOS_TARGET_33_TRIGGER_SIMPTC3_CH1_OVL
 */
__STATIC_INLINE uint32_t LL_AOS_GetTriggerSignal_ADC0(AOS_TypeDef *AOSx, uint32_t TargetPurpose)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&AOSx->PE_TRGSEL0) + (4 * TargetPurpose)));
    return (READ_BIT(*pReg, AOS_PE_TRGSEL32_TRGSEL));
}

/**
 * @brief  Set Trigger source for target34 peripheral DAC (LL_AOS_TARGET_34_DAC).
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_SetTriggerSignal_DAC
 * @param  AOSx AOS Instance.
 * @param TriggerSignal This parameter can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI0_CH8_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI0_CH9_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI0_CH10_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI0_CH11_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI1_CH8_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI1_CH9_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI1_CH10_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI1_CH11_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_SIMPTC0_CH2_OVL
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_SIMPTC1_CH2_OVL
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_SIMPTC2_CH2_OVL
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_SIMPTC3_CH2_OVL
 * @retval None
 */
__STATIC_INLINE void LL_AOS_SetTriggerSignal_DAC(AOS_TypeDef *AOSx, uint32_t TriggerSignal)
{
    MODIFY_REG(AOSx->PE_TRGSEL34, AOS_PE_TRGSEL34_TRGSEL, TriggerSignal);
}

/**
 * @brief  Get Trigger source for target34 peripheral DAC (LL_AOS_TARGET_34_DAC).
 * @rmtoll PE_TRGSEL           TRGSEL         LL_AOS_GetTriggerSignal_DAC
 * @param  AOSx AOS Instance.
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_NONE
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI0_CH8_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI0_CH9_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI0_CH10_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI0_CH11_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI1_CH8_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI1_CH9_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI1_CH10_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_KBI1_CH11_OUTPUT
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_SIMPTC0_CH2_OVL
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_SIMPTC1_CH2_OVL
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_SIMPTC2_CH2_OVL
 *         @arg @ref LL_AOS_TARGET_34_TRIGGER_SIMPTC3_CH2_OVL
 */
__STATIC_INLINE uint32_t LL_AOS_GetTriggerSignal_DAC(AOS_TypeDef *AOSx)
{
    return (READ_BIT(AOSx->PE_TRGSEL34, AOS_PE_TRGSEL34_TRGSEL));
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup AOS_LL_EF_Init Initialization and de-initialization functions
 * @{
 */

// The initialization function can have more than one argument.
ErrorStatus LL_AOS_Init(AOS_TypeDef *AOSx, LL_AOS_InitTypeDef *AOS_InitStruct);
ErrorStatus LL_AOS_DeInit(AOS_TypeDef *AOSx);
void LL_AOS_StructInit(LL_AOS_InitTypeDef *AOS_InitStruct); // If the module is not suitable for the default configuration, you could not defines this function.

/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */

/**
 * @}
 */
#endif /* AOS */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_AOS_H */
