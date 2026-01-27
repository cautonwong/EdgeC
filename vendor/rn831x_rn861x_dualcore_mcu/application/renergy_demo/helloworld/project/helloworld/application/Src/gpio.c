/**
 ******************************************************************************
 * @file   gpio.c
 * @author Renergy Software Team
 * @brief  This file provides code for the configuration of all used GPIO pins.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include "gpio.h"

/* Private includes ---------------------------------------------------------*/
/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Private user code --------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/
/* Exported macro -----------------------------------------------------------*/
/* Exported functions prototypes --------------------------------------------*/
void MX_GPIO_Init(void)
{
    /* GPIO Ports Clock Enable */
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_GPIO_CLK_ENABLE();
    __HAL_SYSTEM_PSW_LOCK();
}
