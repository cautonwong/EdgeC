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
#include <stdio.h>

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

UART_HandleTypeDef huart0;

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

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if(uartHandle->Instance.Instance0_5 == UART0) {
        /* USER CODE BEGIN UART0_MspInit 0 */

        /* USER CODE END UART0_MspInit 0 */
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

        /**UART0 GPIO Configuration
        PE02     ------> UART0_TX
        PE01     ------> UART0_RX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF;
        GPIO_InitStruct.Alternate = GPIO_AF6_UART0_2_4_6;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

        /* UART0 interrupt Init */
        /* GIC set UART0_IRQn to core0 */
        HAL_GIC_SetIrqTargetCore(UART0_IRQn, HAL_GIC_IRQ_TARGET_CORE0);
        /* Initialize ECLIC for interrupt */
        ECLIC_Register_IRQ(UART0_IRQn, ECLIC_VECTOR_INTERRUPT, ECLIC_LEVEL_TRIGGER, 3, 1, NULL);
        /* enable global interrupt */
        __enable_irq();
        /* USER CODE BEGIN UART0_MspInit 1 */

        /* USER CODE END UART0_MspInit 1 */
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{
    if(uartHandle->Instance.Instance0_5 == UART0) {
        /* USER CODE BEGIN UART0_MspDeInit 0 */

        /* USER CODE END UART0_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_SYSTEM_PSW_UNLOCK();
        __HAL_SYSTEM_UART0_CLK_DISABLE();
        __HAL_SYSTEM_PSW_LOCK();

        /**UART0 GPIO Configuration
        PE02     ------> UART0_TX
        PE01     ------> UART0_RX
        */
        HAL_GPIO_DeInit(GPIOE, GPIO_PIN_1 | GPIO_PIN_2);

        /* UART0 interrupt Deinit */
        HAL_GIC_SetIrqTargetCore(UART0_IRQn, HAL_GIC_IRQ_TARGET_NONE);
        ECLIC_DisableIRQ(UART0_IRQn);
        /* USER CODE BEGIN UART0_MspDeInit 1 */

        /* USER CODE END UART0_MspDeInit 1 */
    }
}


/* USER CODE BEGIN 1 */
/**
 * @brief  This function is weak function, redirect printf output to UART.
 * @retval Size
 */
ssize_t _write(int fd, const void* ptr, size_t len)
{
    UNUSED(fd);
    const uint8_t* writebuf = (const uint8_t*)ptr;
    HAL_UART_Transmit(&huart0 , (uint8_t *)writebuf, len, 0xFF);
    return len;
}
/* USER CODE END 1 */
