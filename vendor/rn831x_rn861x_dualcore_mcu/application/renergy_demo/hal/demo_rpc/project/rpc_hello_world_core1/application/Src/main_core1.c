// See LICENSE for license details.
/**
 ******************************************************************************
 * @file           : main_core1.c
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
 #include "main_core1.h"
 #include "uart.h"
 #include "gpio.h"
 #include "gic.h"
 /* Private includes ----------------------------------------------------------*/
 /* USER CODE BEGIN Includes */
 #include <stdio.h>
 #include "rn_rv_ll_wdt.h"
 #include "rpc_api.h"
 /* USER CODE END Includes */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
void core1_reset_init(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/* USER CODE END 0 */
int main(void)
{
    /* USER CODE BEGIN 1 */
    uint32_t counter = 0;
    uint32_t result, result1, result2;
    uint64_t tickstart;
    uint32_t sysclockfreq;
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

    /* Get the sysclk frequency */
    sysclockfreq = HAL_SYSTEM_GetSysClockFreq(HAL_SYSTEM_CLK_CORE1);

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_GIC_Init();
    /* uart0 for printf() */
    MX_UART1_Init();


    printf("\r\n==== Test Start Core1 ====\n");

    printf("\r\nhello world core1\n");

    /* enable core1 reset interrupt */
//    core1_reset_init();

    /****************************************************************************
     * @NOTE: 	Core1 should must connect icc firstly, waiting for core0 link up!
     *
     * ***************************************************************************/
    while (HAL_ICC_Init(HAL_RPC_TIMEOUT_FLAG) != HAL_OK)
    {
    };

    /* rpc call hello world in core0 */

    test_rpc_helloworld_function();

    result = test_rpc_function(20, 30, 40);
    printf("rpc call return result = %d\n", result);

    result = test_rpc_function(30, 10, 50);
    printf("rpc call return result = %d\n", result);

    /* change clock mode by rpc call */
    result1 = HAL_SYSTEM_ClockModeConfig(HAL_SYSTEM_CLKMODE_PLL_7M4, HAL_SYSTEM_SRCDIV_1, HAL_SYSTEM_COREDIV_CORE1_1_CORE0_1);

    result2 = HAL_SYSTEM_ClockModeConfig(HAL_SYSTEM_CLKMODE_RC_59M, HAL_SYSTEM_SRCDIV_2, HAL_SYSTEM_COREDIV_CORE1_1_CORE0_2);
    MX_UART1_Init();
    printf("change clock mode result1 = %d, result2 = %d\n", result1, result2);
    printf("current clock frequency is %dHz\n", __LL_SYSTEM_CORE1_CALC_CORE1_CLK_FREQ(LL_SYSTEM_CORE1_GetCore1SystemClockFreq()));

    result = test_rpc_function(10, 20, 30);
    printf("rpc call return result = %d\n", result);

    result = test_rpc_function(10, 40, 50);
    printf("rpc call return result = %d\n", result);

    while (1) {
        counter++;
        if (counter == 10000) {
            counter = 0;
            printf("...\n");
        }
        /*feed WDT*/
        LL_WDT_ReloadCounter(WDT);
    }
}

/* USER CODE BEGIN 4 */
void core1_reset_init(void)
{
    LL_SYSTEM_CORE1_UnlockSystemPSW();
    /* enable gic */
    LL_SYSTEM_CORE0_PMOD0_EnableClock(LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GIC);
    /* enable CORE1_RST Interrupt */
    LL_SYSTEM_CORE1_ClearFlag_CORE1RST();
    LL_SYSTEM_CORE1_EnableIT_CORE1RST();

    LL_SYSTEM_CORE0_LockSystemPSW();
//    core1_reset_interrupt_enable(1U);
}


/**
 * @brief  This function is weak function, redirect printf output to UART.
 * @retval Size
 */
ssize_t _write(int fd, const void* ptr, size_t len)
{
    UNUSED(fd);
    const uint8_t* writebuf = (const uint8_t*)ptr;
    HAL_UART_Transmit(&huart1 , (uint8_t *)writebuf, len, 0xFF);
    return len;
}

/* USER CODE END 4 */
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
