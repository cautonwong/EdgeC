/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    gic.c
 * @brief   This file provides code for the configuration
 *          of all used GIC pins.
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
#include "gic.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GIC                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
void MX_GIC_Init(void)
{
    /* GIC Ports Clock Enable */
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_GIC_CLK_ENABLE();
    __HAL_SYSTEM_PSW_LOCK();
}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
