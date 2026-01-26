/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    scu.c
 * @brief   This file provides code for the configuration
 *          of all used SCU pins.
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
#include "scu.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure SCU                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

void MX_SCU_Init(void)
{

    /* SCU Ports Clock Enable */
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_SCU_CLK_ENABLE();
    __HAL_SYSTEM_PSW_LOCK();

    /* allocate SRAM to core1 firstly */
    HAL_SCU_SetSecureSRAMSize(HAL_SCU_SRAM_SIZE_80KB);

    /* assign uart1/gpio to core1 */
    /* SCU Configuration set to SECURE(core0) */
    __HAL_SCU_WKEY_UNLOCK();
    __HAL_SCU_UART1_SET_NONSECURE();
    __HAL_SCU_GPIOE_SET_NONSECURE(HAL_SCU_GPIO_PIN_04 | HAL_SCU_GPIO_PIN_05);
    __HAL_SCU_WKEY_LOCK();
    /**UART1 GPIO Configuration
    PE05     ------> UART1_TX
    PE04     ------> UART1_RX
    */

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
