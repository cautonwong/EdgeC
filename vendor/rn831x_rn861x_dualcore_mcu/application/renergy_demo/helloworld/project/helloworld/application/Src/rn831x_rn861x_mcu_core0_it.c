/**
 ******************************************************************************
 * @file   rn831x_rn861x_mcu_core0_it.c
 * @author Renergy Software Team
 * @brief  Interrupt Service Routines.
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
#include "main.h"
#include "rn831x_rn861x_mcu_core0_it.h"
#include "uart.h"
#include <stdio.h>

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
/**
 * @brief This function handles UART0 global interrupt.
 */
__INTERRUPT void UART0_HANDLER(void)
{
    HAL_UART_IRQHandler(&huart0);
}
