/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    uart.c
 * @brief   This file provides code for the configuration
 *          of the UART instances.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "uart.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

UART_HandleTypeDef huart0;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;

/* UART0 init function */
void MX_UART0_Init(void)
{

    /* USER CODE BEGIN UART0_Init 0 */

    /* USER CODE END UART0_Init 0 */

    /* USER CODE BEGIN UART0_Init 1 */

    /* USER CODE END UART0_Init 1 */
    huart0.Instance.Instance0_5 = UART0;
    huart0.Init.BaudRate        = 115200;
    huart0.Init.DataWidth       = UART_DATAWIDTH_8B;
    huart0.Init.StopBits        = UART_STOPBITS_1;
    huart0.Init.Parity          = UART_PARITY_NONE;
    huart0.Init.RxTimeout       = 0xFFU;
    if (HAL_UART_Init(&huart0) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN UART0_Init 2 */

    /* USER CODE END UART0_Init 2 */

}

/* UART1 init function */
void MX_UART1_Init(void)
{

    /* USER CODE BEGIN UART1_Init 0 */

    /* USER CODE END UART1_Init 0 */

    /* USER CODE BEGIN UART1_Init 1 */

    /* USER CODE END UART1_Init 1 */
    huart1.Instance.Instance0_5 = UART1;
    huart1.Init.BaudRate        = 115200;
    huart1.Init.DataWidth       = UART_DATAWIDTH_8B;
    huart1.Init.StopBits        = UART_STOPBITS_1;
    huart1.Init.Parity          = UART_PARITY_NONE;
    huart1.Init.RxTimeout       = 0xFFU;
    /* DMA Config */
    huart1.hdmatx.Mode          = UART_DMA_NORMAL;
    huart1.hdmarx.Mode          = UART_DMA_NORMAL;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN UART1_Init 2 */

    /* USER CODE END UART1_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if(uartHandle->Instance.Instance0_5 == UART1) {
        /* USER CODE BEGIN UART1_MspInit 0 */

        /* USER CODE END UART1_MspInit 0 */
        /* UART1 SCU GIC clock enable */
        __HAL_SYSTEM_PSW_UNLOCK();
        __HAL_SYSTEM_UART1_CLK_ENABLE();
        __HAL_SYSTEM_SCU_CLK_ENABLE();
        __HAL_SYSTEM_GIC_CLK_ENABLE();
        /* GPIO clock enable */
        __HAL_SYSTEM_GPIO_CLK_ENABLE();
        __HAL_SYSTEM_PSW_LOCK();

        /* (core1 can not set!!) SCU Configuration set to NONSECURE(core1) */
//        __HAL_SCU_WKEY_UNLOCK();
//        __HAL_SCU_UART1_SET_SECURE();
//        __HAL_SCU_GPIOE_SET_SECURE(HAL_SCU_GPIO_PIN_04 | HAL_SCU_GPIO_PIN_05);
//        __HAL_SCU_WKEY_LOCK();
        /**UART1 GPIO Configuration
        PE05     ------> UART1_TX
        PE04     ------> UART1_RX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF;
        GPIO_InitStruct.Alternate = GPIO_AF7_UART1_3_5_7;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

        /*core1 can not set!!) UART1 interrupt Init */
//        /* GIC set UART1_IRQn to core1 in core0 */
//        HAL_GIC_SetIrqTargetCore(UART1_IRQn, HAL_GIC_IRQ_TARGET_CORE1);
        /* Initialize ECLIC for interrupt */
        ECLIC_Register_IRQ(UART1_IRQn, ECLIC_VECTOR_INTERRUPT, ECLIC_LEVEL_TRIGGER, 3, 1, NULL);
        /* enable global interrupt */
        __enable_irq();

        /* USER CODE BEGIN UART1_MspInit 1 */

        /* USER CODE END UART1_MspInit 1 */
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{
    if(uartHandle->Instance.Instance0_5 == UART1) {
        /* USER CODE BEGIN UART1_MspDeInit 0 */

        /* USER CODE END UART1_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_SYSTEM_PSW_UNLOCK();
        __HAL_SYSTEM_UART1_CLK_DISABLE();
        __HAL_SYSTEM_PSW_LOCK();

        /**UART1 GPIO Configuration
        PE05     ------> UART1_TX
        PE04     ------> UART1_RX
        */
        HAL_GPIO_DeInit(GPIOE, GPIO_PIN_4 | GPIO_PIN_5);

        /* UART1 interrupt Deinit */
        HAL_GIC_SetIrqTargetCore(UART1_IRQn, HAL_GIC_IRQ_TARGET_NONE);
        ECLIC_DisableIRQ(UART1_IRQn);
        /* USER CODE BEGIN UART1_MspDeInit 1 */

        /* USER CODE END UART1_MspDeInit 1 */
    }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
