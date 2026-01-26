// See LICENSE for license details.
/**
 ******************************************************************************
 * @file           : main_core0.c
 * @brief          : Main program body
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
#include "main_core0.h"
#include "uart.h"
#include "gpio.h"
#include "scu.h"
#include "gic.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "rn_rv_ll_wdt.h"
#include "rpc_api.h"
#include "unity.h"
/* USER CODE END Includes */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
void start_core1(void);
void core1_reset_interrupt_enable(void);
void prepare_for_sleep(void);
void post_wakeup_init(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern volatile bool core1_reset_interrupt;
extern volatile bool sleep_requested;
/* USER CODE END 0 */

int main(void)
{
    /* USER CODE BEGIN 1 */
    uint32_t counter = 0;
    uint32_t icc_status = HAL_OK;
    uint32_t sysclockfreq, result;
    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* enable WDT clock */
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_WDT_CLK_ENABLE();
    __HAL_SYSTEM_PSW_LOCK();

    /* USER CODE END Init */

    /* Configure the system clock */
    result = HAL_SYSTEM_ClockModeConfig(HAL_SYSTEM_CLKMODE_HOSC_29M5, HAL_SYSTEM_SRCDIV_1, HAL_SYSTEM_COREDIV_CORE1_1_CORE0_1);
    /* Get the sysclk frequency */
    if (result == HAL_OK) {
        sysclockfreq = HAL_SYSTEM_GetSysClockFreq(HAL_SYSTEM_CLK_CORE0);
    }

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    /* assign uart1/gpio to core1 */
    MX_SCU_Init();
    MX_GIC_Init();
    /* uart0 for printf() */
    MX_UART0_Init();

    printf("\r\n==== Test Start Core0 ====\r\n");

    /* start core1 and allocate sram to core1 */
    start_core1();
    /* init core1 reset interrupt */
    core1_reset_interrupt_enable();

    printf("\r\nhello world core0\r\n");

    /* connect icc for inter-core communication (for rpc service) */
    while (HAL_ICC_Init(HAL_RPC_TIMEOUT_FLAG) != HAL_OK)
    {
    };

    while (1) {
        /* core1 reset task */
    	MX_UART0_Init();
        if (true == core1_reset_interrupt)
        {
            core1_reset_interrupt = false;
            HAL_ICC_DeInit();
            /* restart up core1 */
            start_core1();
            icc_status = HAL_ICC_Init(HAL_RPC_TIMEOUT_FLAG);
            printf("\r\ncore1 has received a reset interrupt!\r\n");
            LL_WDT_ReloadCounter(WDT);
        }
        /* icc connect timeout reconnect task */
        else if (icc_status != HAL_OK)
        {
            /* restart up core1 */
            start_core1();
            icc_status = HAL_ICC_Init(HAL_RPC_TIMEOUT_FLAG);
            printf("\r\ncore1 icc connect timeout, waiting and retrying!\r\n");
            LL_WDT_ReloadCounter(WDT);
        }
        /* sleep request task */
        else if (true == sleep_requested)
        {
            sleep_requested = false;
            prepare_for_sleep();
            printf("waiting for a interrupt to wake up core0!!\r\n");

            __WFI();

            post_wakeup_init();
            printf("\r\ncore0 wake up!!\r\n");
        }
        else
        {
            /* do nothing */
        }
        counter++;
        if (counter == 10000) {
            counter = 0;
            printf("...\n");
        }
        /*feed WDT*/
        LL_WDT_ReloadCounter(WDT);
    }

}

/* USER CODE END 4 */
void HAL_SYSTEM_Core1ResetCallback(void)
{
    extern volatile bool core1_reset_interrupt;
    core1_reset_interrupt = true;
}

uint32_t test_rpc_function(uint32_t Param1, uint32_t Param2, uint32_t Param3)
{
    printf("\n");
    printf("core1 call rpc function: test_rpc_function()\n");
    printf("Param1=%d, Param2=%d, Param3=%d\n", Param1, Param2, Param3);
    printf("\n");
    return (Param1 + Param2 + Param3);
}

void test_rpc_helloworld_function(void)
{
    printf("\n");
    printf("hello world! in core0, call by core1\n");
    printf("\n");
}


void start_core1(void)
{
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_CORE1_RELEASE_RESET();
    __HAL_SYSTEM_CORE1_FORCE_RESET();
    __HAL_SYSTEM_PSW_LOCK();
}


void core1_reset_interrupt_enable(void)
{
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_GIC_CLK_ENABLE();
    __HAL_SYSTEM_PSW_LOCK();

    HAL_GIC_SetIrqTargetCore(CORE1_RST_IRQn, HAL_GIC_IRQ_TARGET_CORE0);
    ECLIC_Register_IRQ(CORE1_RST_IRQn, ECLIC_VECTOR_INTERRUPT, ECLIC_POSTIVE_EDGE_TRIGGER, 9, 0, NULL);
    __enable_irq();

}


void prepare_for_sleep(void)
{
    /* add your operation */
}

void post_wakeup_init(void)
{
    /* add your operation */
}


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

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
