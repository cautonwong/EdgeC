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

/* UART3 init function */
void MX_UART3_Init(void)
{

    /* USER CODE BEGIN UART3_Init 0 */

    /* USER CODE END UART3_Init 0 */

    /* USER CODE BEGIN UART3_Init 1 */

    /* USER CODE END UART3_Init 1 */
    huart3.Instance.Instance0_5 = UART3;
    huart3.Init.BaudRate        = 115200;
    huart3.Init.DataWidth       = UART_DATAWIDTH_8B;
    huart3.Init.StopBits        = UART_STOPBITS_1;
    huart3.Init.Parity          = UART_PARITY_NONE;
    huart3.Init.RxTimeout       = 0xFFU;
    /* DMA Config */
    huart3.hdmatx.Mode          = UART_DMA_NORMAL;
    huart3.hdmarx.Mode          = UART_DMA_NORMAL;
    if (HAL_UART_Init(&huart3) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN UART3_Init 2 */

    /* USER CODE END UART3_Init 2 */

}

/* UART6 init function */
void MX_UART6_Init(void)
{

    /* USER CODE BEGIN UART6_Init 0 */

    /* USER CODE END UART6_Init 0 */

    /* USER CODE BEGIN UART6_Init 1 */

    /* USER CODE END UART6_Init 1 */
    huart6.Instance.Instance6_7 = UART6;
    huart6.Init.BaudRate = 115200;
    huart6.Init.DataWidth = UART_DATAWIDTH_8B;
    huart6.Init.StopBits = UART_STOPBITS_1;
    huart6.Init.Parity = UART_PARITY_NONE;
    huart6.Init.RxTimeout = 0xFFU;
    /* DMA Config */
    huart6.hdmatx.Mode          = UART_DMA_NORMAL;
    huart6.hdmarx.Mode          = UART_DMA_NORMAL;
    if (HAL_UART_Init(&huart6) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN UART1_Init 2 */

    /* USER CODE END UART1_Init 2 */

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

        /* USER CODE BEGIN UART0_MspInit 1 */

        /* USER CODE END UART0_MspInit 1 */
    } else if(uartHandle->Instance.Instance0_5 == UART1) {
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

        /* SCU Configuration set to SECURE(core0) */
        __HAL_SCU_WKEY_UNLOCK();
        __HAL_SCU_UART1_SET_SECURE();
        __HAL_SCU_GPIOE_SET_SECURE(HAL_SCU_GPIO_PIN_04 | HAL_SCU_GPIO_PIN_05);
        __HAL_SCU_WKEY_LOCK();
        /**UART1 GPIO Configuration
        PE05     ------> UART1_TX
        PE04     ------> UART1_RX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF;
        GPIO_InitStruct.Alternate = GPIO_AF7_UART1_3_5_7;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

        /* UART1 interrupt Init */
        /* GIC set UART1_IRQn to core0 */
        HAL_GIC_SetIrqTargetCore(UART1_IRQn, HAL_GIC_IRQ_TARGET_CORE0);
        /* Initialize ECLIC for interrupt */
        ECLIC_Register_IRQ(UART1_IRQn, ECLIC_VECTOR_INTERRUPT, ECLIC_LEVEL_TRIGGER, 3, 1, NULL);
        /* enable global interrupt */
        __enable_irq();

        /* USER CODE BEGIN UART1_MspInit 1 */

        /* USER CODE END UART1_MspInit 1 */
    } else if (uartHandle->Instance.Instance0_5 == UART3) {
        /* USER CODE BEGIN UART3_MspInit 0 */

        /* USER CODE END UART3_MspInit 0 */
        /* UART3 SCU clock enable */
        __HAL_SYSTEM_PSW_UNLOCK();
        __HAL_SYSTEM_UART3_CLK_ENABLE();
        __HAL_SYSTEM_SCU_CLK_ENABLE();
        /* GPIO clock enable */
        __HAL_SYSTEM_GPIO_CLK_ENABLE();
        __HAL_SYSTEM_PSW_LOCK();

        /* SCU Configuration set to SECURE(core0) */
        __HAL_SCU_WKEY_UNLOCK();
        __HAL_SCU_UART3_SET_SECURE();
        __HAL_SCU_GPIOA_SET_SECURE(HAL_SCU_GPIO_PIN_00 | HAL_SCU_GPIO_PIN_01);
        __HAL_SCU_WKEY_LOCK();
        /**UART3 GPIO Configuration
        PA01     ------> UART3_TX
        PA00     ------> UART3_RX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_AF;
        GPIO_InitStruct.Alternate = GPIO_AF7_UART1_3_5_7;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* USER CODE BEGIN UART3_MspInit 1 */

        /* USER CODE END UART3_MspInit 1 */
    } else if (uartHandle->Instance.Instance6_7 == UART6) {
        /* USER CODE BEGIN UART6_MspInit 0 */

        /* USER CODE END UART6_MspInit 0 */
        /* UART6 SCU GIC clock enable */
        __HAL_SYSTEM_PSW_UNLOCK();
        __HAL_SYSTEM_UART6_CLK_ENABLE();
        __HAL_SYSTEM_SCU_CLK_ENABLE();
        __HAL_SYSTEM_GIC_CLK_ENABLE();
        /* GPIO clock enable */
        __HAL_SYSTEM_GPIO_CLK_ENABLE();
        __HAL_SYSTEM_PSW_LOCK();

        /* SCU Configuration set to SECURE(core0) */
        __HAL_SCU_WKEY_UNLOCK();
        __HAL_SCU_UART6_SET_SECURE();
        __HAL_SCU_GPIOA_SET_SECURE(HAL_SCU_GPIO_PIN_04 | HAL_SCU_GPIO_PIN_03);
        __HAL_SCU_WKEY_LOCK();
        /**UART6 GPIO Configuration
        PA04     ------> UART6_TX
        PA03     ------> UART6_RX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF;
        GPIO_InitStruct.Alternate = GPIO_AF6_UART0_2_4_6;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* UART1 interrupt Init */
        /* GIC set UART1_IRQn to core0 */
        HAL_GIC_SetIrqTargetCore(UART6_IRQn, HAL_GIC_IRQ_TARGET_CORE0);
        /* Initialize ECLIC for interrupt */
        ECLIC_Register_IRQ(UART6_IRQn, ECLIC_VECTOR_INTERRUPT, ECLIC_LEVEL_TRIGGER, 3, 1, NULL);
        /* enable global interrupt */
        __enable_irq();

        /* USER CODE BEGIN UART6_MspInit 1 */

        /* USER CODE END UART6_MspInit 1 */
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

        /* USER CODE BEGIN UART0_MspDeInit 1 */

        /* USER CODE END UART0_MspDeInit 1 */
    } else if(uartHandle->Instance.Instance0_5 == UART1) {
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
    } else if(uartHandle->Instance.Instance0_5 == UART3) {
        /* USER CODE BEGIN UART3_MspDeInit 0 */

        /* USER CODE END UART3_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_SYSTEM_PSW_UNLOCK();
        __HAL_SYSTEM_UART3_CLK_DISABLE();
        __HAL_SYSTEM_PSW_LOCK();

        /**UART3 GPIO Configuration
        PA01     ------> UART3_TX
        PA00     ------> UART3_RX
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0 | GPIO_PIN_1);

        /* USER CODE BEGIN UART3_MspDeInit 1 */

        /* USER CODE END UART3_MspDeInit 1 */
    } else if(uartHandle->Instance.Instance6_7 == UART6) {
        /* USER CODE BEGIN UART1_MspDeInit 0 */

        /* USER CODE END UART1_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_SYSTEM_PSW_UNLOCK();
        __HAL_SYSTEM_UART6_CLK_DISABLE();
        __HAL_SYSTEM_PSW_LOCK();

        /**UART1 GPIO Configuration
        PA04     ------> UART6_TX
        PA03     ------> UART6_RX
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4 | GPIO_PIN_3);

        /* UART1 interrupt Deinit */
        HAL_GIC_SetIrqTargetCore(UART6_IRQn, HAL_GIC_IRQ_TARGET_NONE);
        ECLIC_DisableIRQ(UART6_IRQn);
        /* USER CODE BEGIN UART1_MspDeInit 1 */

        /* USER CODE END UART1_MspDeInit 1 */
    }
}

/* USER CODE BEGIN 1 */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    uint8_t msg[] = "TX_OK!\r\n";
    HAL_UART_Transmit(huart, msg, strlen((const char *)msg), 0xFF);
}

void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    uint8_t msg[] = "HALF_TX_OK!\r\n";
    /* can not send msg, because huart->gState not HAL_UART_STATE_READY */
    HAL_UART_Transmit(huart, msg, strlen((const char *)msg), 0xFF);
}

void HAL_UART_AbortCpltCallback(UART_HandleTypeDef *huart)
{
    uint8_t msg[] = "  ||*** abort use HAL_UART_Abort_IT OK!\r\n";
    HAL_UART_Transmit(huart, msg, strlen((const char *)msg), 0xFF);
}

void HAL_UART_AbortTransmitCpltCallback(UART_HandleTypeDef *huart)
{
    uint8_t msg[] = "  ||### abort use HAL_UART_AbortTransmit_IT OK!\r\n";
    HAL_UART_Transmit(huart, msg, strlen((const char *)msg), 0xFF);
}

void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef *huart)
{
    uint8_t msg[] = "  ||### abort use HAL_UART_AbortReceive_IT OK!\r\n";
    HAL_UART_Transmit(huart, msg, strlen((const char *)msg), 0xFF);
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    uint8_t msg[] = " DMA receive Half OK!\r\n";
    HAL_UART_Transmit(huart, msg, strlen((const char *)msg), 0xFF);
}

/* USER CODE END 1 */
