/**
 ******************************************************************************
 * @file   main.c
 * @author Renergy Software Team
 * @brief
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
#include "uart.h"
#include "gpio.h"

/* Private includes ---------------------------------------------------------*/
#include "unity.h"
#include "rn_rv_ll_wdt.h"
#include <stdio.h>

static void test_pass();

#define TIMER_TICKS 800

void eclic_mtip_handler(void)
{
    static uint32_t int_t_cnt = 0;    /* timer interrupt counter */
    printf("[IN TIMER INTERRUPT]timer interrupt hit %d times", int_t_cnt++);
    // Reload Timer Interrupt
    SysTick_Reload(TIMER_TICKS);
}

void eclic_demo(void)
{
    // Globally disable all interrupt servicing
    __disable_irq();

    ECLIC_SetShvIRQ(SysTimer_IRQn, ECLIC_NON_VECTOR_INTERRUPT);
    TEST_ASSERT_EQUAL_UINT(ECLIC_GetShvIRQ(SysTimer_IRQn), ECLIC_NON_VECTOR_INTERRUPT);

    ECLIC_SetTrigIRQ(SysTimer_IRQn, ECLIC_POSTIVE_EDGE_TRIGGER);
    TEST_ASSERT_EQUAL_UINT(ECLIC_GetTrigIRQ(SysTimer_IRQn), ECLIC_POSTIVE_EDGE_TRIGGER);

    ECLIC_SetLevelIRQ(SysTimer_IRQn, 1);
    TEST_ASSERT_EQUAL_UINT(ECLIC_GetLevelIRQ(SysTimer_IRQn), 1);

    ECLIC_SetLevelIRQ(SysTimer_IRQn, 0);
    TEST_ASSERT_EQUAL_UINT(ECLIC_GetLevelIRQ(SysTimer_IRQn), 0);

    ECLIC_SetPriorityIRQ(SysTimer_IRQn, 0);
    TEST_ASSERT_EQUAL_UINT(ECLIC_GetPriorityIRQ(SysTimer_IRQn), 0);

    ECLIC_SetCfgNlbits(__ECLIC_INTCTLBITS - 1);
    TEST_ASSERT_EQUAL_UINT(ECLIC_GetCfgNlbits(), (__ECLIC_INTCTLBITS - 1));

    ECLIC_SetPriorityIRQ(SysTimer_IRQn, 1);
    TEST_ASSERT_EQUAL_UINT(ECLIC_GetPriorityIRQ(SysTimer_IRQn), 1);

    // Clear its pending state
    ECLIC_ClearPendingIRQ(SysTimer_IRQn);
    TEST_ASSERT_EQUAL_UINT(ECLIC_GetPendingIRQ(SysTimer_IRQn), 0);

    // Enable the interrupt
    ECLIC_EnableIRQ(SysTimer_IRQn);
    TEST_ASSERT_EQUAL_UINT(ECLIC_GetEnableIRQ(SysTimer_IRQn), 1);

    // Set the interrupt pending state
    ECLIC_SetPendingIRQ(SysTimer_IRQn);
    for (uint32_t i = 10; i > 0; --i) {}

    // Interrupt is not taken
    TEST_ASSERT_EQUAL_UINT(ECLIC_GetPendingIRQ(SysTimer_IRQn), 1);

    // Globally enable interrupt servicing
    __enable_irq();

    for (uint32_t i = 1000; i > 0; --i) {}

    // Interrupt it not pending anymore.
    TEST_ASSERT_EQUAL_UINT(ECLIC_GetPendingIRQ(SysTimer_IRQn), 0);

    // Disable interrupt
    ECLIC_DisableIRQ(SysTimer_IRQn);
    TEST_ASSERT_EQUAL_UINT(ECLIC_GetEnableIRQ(SysTimer_IRQn), 0);

    // Set interrupt pending
    ECLIC_SetPendingIRQ(SysTimer_IRQn);
    for (uint32_t i = 10; i > 0; --i) {}

    // Interrupt is not taken again
    TEST_ASSERT_EQUAL_UINT(ECLIC_GetPendingIRQ(SysTimer_IRQn), 1);

    // Clear interrupt pending
    ECLIC_ClearPendingIRQ(SysTimer_IRQn);
    for (uint32_t i = 10; i > 0; --i) {}

    // Interrupt it not pending anymore.
    TEST_ASSERT_EQUAL_UINT(ECLIC_GetPendingIRQ(SysTimer_IRQn), 0);

    // Globally disable interrupt servicing
    __disable_irq();
}

int main(void)
{
    /* Reset of all peripherals, Initializes the Systick. */
    HAL_Init();

    /* Enable WDT clock */
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_WDT_CLK_ENABLE();
    __HAL_SYSTEM_PSW_LOCK();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_UART0_Init();

    /* Initialize Test Framework Unity. */
    UNITY_BEGIN();
    UnityPrint("==== Initialize Test Framework Unity ====");
    UNITY_PRINT_EOL();
    UNITY_PRINT_EOL();

    RUN_TEST(eclic_demo, 0);

    /* Test End. */
    UNITY_END();
    if (Unity.TestFailures == 0U) {
        test_pass();
    }
    printf("==== Test End ====\n");

    while(1) {
        LL_WDT_ReloadCounter(WDT);
    }
}

void test_pass()
{
    printf("\n");
    printf("\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~ #####     ##     ####    ####  ~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~ #    #   #  #   #       #      ~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~ #    #  #    #   ####    ####  ~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~ #####   ######       #       # ~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~ #       #    #  #    #  #    # ~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~ #       #    #   ####    ####  ~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("\n");
}

/**
 * @brief  This function is weak function, redirect printf output to UART.
 * @retval Size
 */
ssize_t _write(int fd, const void* ptr, size_t len)
{
    const uint8_t* writebuf = (const uint8_t*)ptr;
    HAL_UART_Transmit(&huart0, (uint8_t *)writebuf, len, 0xFF);
    return len;
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {

    };
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
    /* User can add his own implementation to report the file name and line number,
        ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
