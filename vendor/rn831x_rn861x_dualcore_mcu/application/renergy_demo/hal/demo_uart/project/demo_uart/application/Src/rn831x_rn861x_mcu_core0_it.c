/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_it_core0.c
 * @brief   Interrupt Service Routines.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main_core0.h"
#include "rn831x_rn861x_mcu_core0_it.h"
#include "uart.h"
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */


/******************************************************************************/
/*           RISCV Processor Interruption and Exception Handlers          */
/******************************************************************************/

/**
 * @brief This function handles UART0 global interrupt.
 */
__INTERRUPT void UART0_HANDLER(void)
{
    /* USER CODE BEGIN UART0_IRQn 0 */

    /* USER CODE END UART0_IRQn 0 */
    HAL_UART_IRQHandler(&huart0);

    /* USER CODE BEGIN UART0_IRQn 1 */

    /* USER CODE END UART0_IRQn 1 */
}

/**
 * @brief This function handles UART1 global interrupt.
 */
__INTERRUPT void UART1_HANDLER(void)
{
    /* USER CODE BEGIN UART1_IRQn 0 */

    /* USER CODE END UART1_IRQn 0 */
    HAL_UART_IRQHandler(&huart1);

    /* USER CODE BEGIN UART1_IRQn 1 */

    /* USER CODE END UART1_IRQn 1 */
}
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
