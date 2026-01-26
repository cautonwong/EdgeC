/**
 ******************************************************************************
 * @file   rn_rv_hal_gpio.h
 * @author Renergy Software Team
 * @brief  This file contains all the functions prototypes for the HAL
 *         module driver.
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
#ifndef __RN_RV_HAL_GPIO_H
#define __RN_RV_HAL_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal_def.h"
#include "rn_rv_ll_gpio.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup GPIO
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup GPIO_Exported_Types GPIO Exported Types
 * @{
 */

/**
 * @brief GPIO Init structure definition
 */
typedef struct
{
    uint32_t Pin;           /*!< Specifies the GPIO pins to be configured.
                                This parameter can be any value of @ref GPIO_pins_define */

    uint32_t Mode;          /*!< Specifies the operating mode for the selected pins.
                                This parameter can be a value of @ref GPIO_mode_define */

    uint32_t Pull;          /*!< Specifies the Pull-up or Pull-Down activation for the selected pins.
                            This parameter can be a value of @ref GPIO_pull_define */

    uint32_t OutputType;    /*!< Specifies the output type for the selected pins.
                                This parameter can be any value of @ref GPIO_output_define*/

    uint32_t InputType;     /*!< Specifies the input type for the selected pins.
                                This parameter can be any value of @ref GPIO_input_define*/

    uint32_t Speed;         /*!< Specifies the speed for the selected pins.
                                This parameter can be a value of @ref GPIO_speed_define */

    uint32_t Alternate;     /*!< Peripheral to be connected to the selected pins.
                                   This parameter can be a value of @ref GPIO_Alternate_function_selection */
}GPIO_InitTypeDef;

/**
 * @brief  GPIO Bit SET and Bit RESET enumeration
 */
typedef enum __attribute__((__packed__))
{
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET
}GPIO_PinState;
/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup GPIO_Exported_Constants GPIO Exported Constants
 * @{
 */

/** @defgroup GPIO_pins_define GPIO pins define
 * @{
 */
#define GPIO_PIN_0                 ((uint16_t)0x0001)  /* Pin 0 selected    */
#define GPIO_PIN_1                 ((uint16_t)0x0002)  /* Pin 1 selected    */
#define GPIO_PIN_2                 ((uint16_t)0x0004)  /* Pin 2 selected    */
#define GPIO_PIN_3                 ((uint16_t)0x0008)  /* Pin 3 selected    */
#define GPIO_PIN_4                 ((uint16_t)0x0010)  /* Pin 4 selected    */
#define GPIO_PIN_5                 ((uint16_t)0x0020)  /* Pin 5 selected    */
#define GPIO_PIN_6                 ((uint16_t)0x0040)  /* Pin 6 selected    */
#define GPIO_PIN_7                 ((uint16_t)0x0080)  /* Pin 7 selected    */
#define GPIO_PIN_8                 ((uint16_t)0x0100)  /* Pin 8 selected    */
#define GPIO_PIN_9                 ((uint16_t)0x0200)  /* Pin 9 selected    */
#define GPIO_PIN_10                ((uint16_t)0x0400)  /* Pin 10 selected   */
#define GPIO_PIN_11                ((uint16_t)0x0800)  /* Pin 11 selected   */
#define GPIO_PIN_12                ((uint16_t)0x1000)  /* Pin 12 selected   */
#define GPIO_PIN_13                ((uint16_t)0x2000)  /* Pin 13 selected   */
#define GPIO_PIN_14                ((uint16_t)0x4000)  /* Pin 14 selected   */
#define GPIO_PIN_15                ((uint16_t)0x8000)  /* Pin 15 selected   */
#define GPIO_PIN_All               ((uint16_t)0xFFFF)  /* All pins selected */

#define GPIO_PIN_MASK              0x0000FFFFU         /* PIN mask */
/**
 * @}
 */

/** @defgroup GPIO_mode_define GPIO mode define
 * @brief GPIO Configuration Mode
 * @{
 */
#define  GPIO_MODE_INPUT       LL_GPIO_MODE_INPUT           /*!< Input Mode              */
#define  GPIO_MODE_OUTPUT      LL_GPIO_MODE_OUTPUT          /*!< Output Mode             */
#define  GPIO_MODE_AF          LL_GPIO_MODE_ALTERNATE       /*!< Alternate Function Mode */
#define  GPIO_MODE_ANALOG      LL_GPIO_MODE_ANALOG          /*!< Analog Mode             */
/**
 * @}
 */

/** @defgroup GPIO_output_define  GPIO output define
 * @brief GPIO Output Type
 * @{
 */
#define GPIO_OUTPUT_PP         LL_GPIO_OUTPUT_PUSHPULL   /*!< Output push-pull type */
#define GPIO_OUTPUT_OD         LL_GPIO_OUTPUT_OPENDRAIN  /*!< Output open-drain type */
/**
 * @}
 */

/** @defgroup GPIO_input_define  GPIO input define
 * @brief GPIO Input Type
 * @{
 */
#define GPIO_INPUT_CMOS        LL_GPIO_INPUT_CMOS  /*!< CMOS buffer type, please refer to the product datasheet */
#define GPIO_INPUT_TTL         LL_GPIO_INPUT_TTL   /*!< TTL buffer type, please refer to the product datasheet */
/**
 * @}
 */

/** @defgroup GPIO_speed_define  GPIO speed define
 * @brief GPIO Output Maximum frequency
 * @{
 */
#define GPIO_SPEED_FREQ_NORMAL    LL_GPIO_SPEED_FREQ_NORMAL  /*!< Normal output speed, please refer to the product datasheet */
#define GPIO_SPEED_FREQ_HIGH      LL_GPIO_SPEED_FREQ_HIGH    /*!< Fast output speed, please refer to the product datasheet  */
/**
 * @}
 */

/** @defgroup GPIO_pull_define GPIO pull define
 * @brief GPIO Pull-Up or Pull-Down Activation
 * @{
 */
#define GPIO_NOPULL               LL_GPIO_PULL_NO     /*!< No Pull-up or Pull-down activation  */
#define GPIO_PULLUP               LL_GPIO_PULL_UP     /*!< Pull-up activation                  */
#define GPIO_PULLDOWN             LL_GPIO_PULL_DOWN   /*!< Pull-down activation                */
/**
 * @}
 */

/** @defgroup GPIO_Alternate_function_selection GPIO alternate function define
 * @brief GPIO alternate function
 * @{
 */
#define GPIO_AF0_SYS                  LL_GPIO_AF_0
#define GPIO_AF1_QSPI0                LL_GPIO_AF_1
#define GPIO_AF2_SPI0_QSPI1           LL_GPIO_AF_2
#define GPIO_AF3_SPI1                 LL_GPIO_AF_3
#define GPIO_AF4_SPI2_UART0_UART2     LL_GPIO_AF_4
#define GPIO_AF5_SPI3_UART1_UART3     LL_GPIO_AF_5
#define GPIO_AF6_UART0_2_4_6          LL_GPIO_AF_6
#define GPIO_AF7_UART1_3_5_7          LL_GPIO_AF_7
#define GPIO_AF8_LPUART_CAN_RTC1S     LL_GPIO_AF_8
#define GPIO_AF9_I2C_ISO7816          LL_GPIO_AF_9
#define GPIO_AF10_KEY                 LL_GPIO_AF_10
#define GPIO_AF11_USART_TCO           LL_GPIO_AF_11
#define GPIO_AF12_TCI_TCO             LL_GPIO_AF_12
#define GPIO_AF13_CF_IOCNT            LL_GPIO_AF_13
#define GPIO_AF14_RESERVE             LL_GPIO_AF_14
#define GPIO_AF15_LCD                 LL_GPIO_AF_15
/**
 * @}
 */

/** @defgroup GPIO_JTAG_pin_define GPIO JTAG pin define
 * @{
 */
#define GPIO_JTCK                 (0x00000001U)
#define GPIO_JTMS                 (0x00000002U)
#define GPIO_JTDI                 (0x00000004U)
#define GPIO_JTDO                 (0x00000008U)

#define GPIO_JTAG_MASK            (0x0000000FU)
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @defgroup GPIO_Private_Macros GPIO Private Macros
 * @{
 */
#define IS_GPIO_PIN_ACTION(ACTION)      (((ACTION) == GPIO_PIN_RESET) || ((ACTION) == GPIO_PIN_SET))
#define IS_GPIO_PIN(PIN)                (((((uint32_t)PIN) & GPIO_PIN_MASK ) != 0x00U) && \
                                         ((((uint32_t)PIN) & ~GPIO_PIN_MASK) == 0x00U))
#define IS_GPIO_MODE(MODE)              (((MODE) == GPIO_MODE_INPUT)         ||\
                                         ((MODE) == GPIO_MODE_OUTPUT)        ||\
                                         ((MODE) == GPIO_MODE_AF)            ||\
                                         ((MODE) == GPIO_MODE_ANALOG))
#define IS_GPIO_OUTPUT(OUTPUT)          ((OUTPUT == GPIO_OUTPUT_PP) || (OUTPUT == GPIO_OUTPUT_OD))
#define IS_GPIO_INPUT(INPUT)            ((INPUT == GPIO_INPUT_CMOS) || (INPUT == GPIO_INPUT_TTL))
#define IS_GPIO_SPEED(SPEED)            (((SPEED) == GPIO_SPEED_FREQ_NORMAL) || ((SPEED) == GPIO_SPEED_FREQ_HIGH))
#define IS_GPIO_PULL(PULL)              (((PULL) == GPIO_NOPULL) || ((PULL) == GPIO_PULLUP) || \
                                        ((PULL) == GPIO_PULLDOWN))
#define IS_GPIO_AF(AF)                  (((AF) == GPIO_AF0_SYS )   ||\
                                         ((AF) == GPIO_AF1_QSPI0 )   ||\
                                         ((AF) == GPIO_AF2_SPI0_QSPI1 )   ||\
                                         ((AF) == GPIO_AF3_SPI1 )   ||\
                                         ((AF) == GPIO_AF4_SPI2_UART0_UART2 )   ||\
                                         ((AF) == GPIO_AF5_SPI3_UART1_UART3 )   ||\
                                         ((AF) == GPIO_AF6_UART0_2_4_6 )   ||\
                                         ((AF) == GPIO_AF7_UART1_3_5_7 )   ||\
                                         ((AF) == GPIO_AF8_LPUART_CAN_RTC1S )   ||\
                                         ((AF) == GPIO_AF9_I2C_ISO7816 )   ||\
                                         ((AF) == GPIO_AF10_KEY)   ||\
                                         ((AF) == GPIO_AF11_USART_TCO)   ||\
                                         ((AF) == GPIO_AF12_TCI_TCO)   ||\
                                         ((AF) == GPIO_AF13_CF_IOCNT)   ||\
                                         ((AF) == GPIO_AF14_RESERVE)   ||\
                                         ((AF) == GPIO_AF15_LCD))
#define IS_GPIO_JTAG_PIN(JTAG)          ((((JTAG) & GPIO_JTAG_MASK) != 0x0U) && \
                                         (((JTAG) & ~GPIO_JTAG_MASK) == 0x0U))
/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup GPIO_Exported_Functions
 * @{
 */

/** @addtogroup GPIO_Exported_Functions_Group1
 * @{
 */
/* Initialization and de-initialization functions ****************************/
void HAL_GPIO_Init(GPIO_TypeDef  *GPIOx, GPIO_InitTypeDef *GPIO_Init);
void HAL_GPIO_DeInit(GPIO_TypeDef  *GPIOx, uint32_t GPIO_Pin);
/**
 * @}
 */

/** @addtogroup GPIO_Exported_Functions_Group2
 * @{
 */
/* IO operation functions ****************************************************/
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
void HAL_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
/**
 * @}
 */

/** @addtogroup GPIO_Exported_Functions_Group3
 * @{
 */
/* Enable and disable Analog or force input functions ************************/
void HAL_GPIO_EnableAnalog(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_DisableAnalog(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_EnableForceInput(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_DisableForceInput(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
/* Enable JTAG Pin functions *************************************************/
void HAL_GPIO_EnableJTAG(GPIO_DBG_TypeDef *GPIO_DBGx, uint32_t JTAG_PIN);
void HAL_GPIO_DisableJTAG(GPIO_DBG_TypeDef *GPIO_DBGx, uint32_t JTAG_PIN);
/**
 * @}
 */

/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup GPIO_Private_Functions GPIO Private Functions
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

#endif /* __RN_RV_HAL_GPIO_H */

