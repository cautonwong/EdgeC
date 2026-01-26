/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_it.c
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
/* Includes ------------------------------------------------------------------*/
#include "main_core1.h"
#include "rn831x_rn861x_mcu_core1_it.h"
#include "uart.h"

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
  * @brief This function handles inter core interrupt.
  */
__INTERRUPT void INTER_CORE_HANDLER(void)
{
    /* USER CODE BEGIN INTERCORE_IRQn 0 */

    /* USER CODE END INTERCORE_IRQn 0 */

    HAL_GIC_ICI_IRQHandler();
    /* USER CODE BEGIN INTERCORE_IRQn 1 */

    /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles mailbox core1 interrupt.
  */
__INTERRUPT void MAILBOX_HOST_HANDLER(void)
{
    /* USER CODE BEGIN MAILBOXHOST0_IRQn 0 */

    /* USER CODE END MAILBOXHOST0_IRQn 0 */
    HAL_MAILBOX_CORE1_IRQHandler();
    /* USER CODE BEGIN MAILBOXHOST0_IRQn 1 */

    /* USER CODE END MAILBOXHOST0_IRQn 1 */

}

/**
  * @brief This function handles scu interrupt.
  */
__INTERRUPT void SCU_HANDLER(void)
{
    /* USER CODE BEGIN SCU_IRQn 0 */

    /* USER CODE END SCU_IRQn 0 */
    HAL_SCU_IRQHandler();

    /* USER CODE BEGIN SCU_IRQn 1 */

    /* USER CODE END SCU_IRQn 1 */
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
