/**
 ******************************************************************************
 * @file   uart.c
 * @author Renergy Software Team
 * @brief  This file provides code for the configuration of all used UARTs.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "uart.h"

/* Private includes ---------------------------------------------------------*/
/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Private user code --------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/
UART_HandleTypeDef huart0;

/* Exported macro -----------------------------------------------------------*/
/* Exported functions prototypes --------------------------------------------*/
/* UART0 init function */
void MX_UART0_Init(void)
{
    huart0.Instance.Instance0_5 = UART0;
    huart0.Init.BaudRate        = 115200;
    huart0.Init.DataWidth       = UART_DATAWIDTH_8B;
    huart0.Init.StopBits        = UART_STOPBITS_1;
    huart0.Init.Parity          = UART_PARITY_NONE;
    huart0.Init.RxTimeout       = 0xFFU;
    if (HAL_UART_Init(&huart0) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{
    /* Default uart port is UART0 */
    if (uartHandle->Instance.Instance0_5 == UART0) {
        /* UART0 SCU GIC clock enable */
        __HAL_SYSTEM_PSW_UNLOCK();
        __HAL_SYSTEM_UART0_CLK_ENABLE();
        __HAL_SYSTEM_SCU_CLK_ENABLE();

        /* GPIO clock enable */
        __HAL_SYSTEM_GPIO_CLK_ENABLE();
        __HAL_SYSTEM_PSW_LOCK();

        /* SCU Configuration set to SECURE(core0) */
        __HAL_SCU_WKEY_UNLOCK();
        __HAL_SCU_UART0_SET_SECURE();
        __HAL_SCU_GPIOE_SET_SECURE(HAL_SCU_GPIO_PIN_02 | HAL_SCU_GPIO_PIN_01);
        __HAL_SCU_WKEY_LOCK();

        /**
         * UART0 GPIO Configuration:
         *  PE02 -> UART0_TX
         *  PE01 -> UART0_RX
         */
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin       = (GPIO_PIN_1 | GPIO_PIN_2);
        GPIO_InitStruct.Mode      = GPIO_MODE_AF;
        GPIO_InitStruct.Alternate = GPIO_AF6_UART0_2_4_6;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{
    if(uartHandle->Instance.Instance0_5 == UART0) {
        /* Peripheral clock disable */
        __HAL_SYSTEM_PSW_UNLOCK();
        __HAL_SYSTEM_UART0_CLK_DISABLE();
        __HAL_SYSTEM_PSW_LOCK();

        /**
         * UART0 GPIO Configuration:
         *  PE02 -> UART0_TX
         *  PE01 -> UART0_RX
         */
        HAL_GPIO_DeInit(GPIOE, (GPIO_PIN_1 | GPIO_PIN_2));
    }
}
