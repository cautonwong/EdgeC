/**
 ******************************************************************************
 * @file   main.c
 * @author Renergy Software Team
 * @brief  Hello world demo.
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
#include "rn_rv_ll_wdt.h"
#include "test_dma.h"
#include "unity.h"
#include <stdio.h>

//#define ENABLE_GCOV_TEST

static void test_pass(void);

#ifdef QSPI_TEST_ENABLE
static void start_core1(void)
{
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_GPIO_CLK_ENABLE();
    __HAL_SCU_WKEY_UNLOCK();
    HAL_SCU_SetNonSecureSRAMSize(HAL_SCU_SRAM_SIZE_48KB);
    __HAL_SYSTEM_CORE1_FORCE_RESET();
    __HAL_SYSTEM_CORE1_RELEASE_RESET();
    __HAL_SCU_WKEY_LOCK();
    __HAL_SYSTEM_PSW_LOCK();
}
#endif

int main(void)
{
    /* Reset of all peripherals, Initializes the Systick. */
    HAL_Init();

    /* Enable WDT clock */
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_WDT_CLK_ENABLE();
    __HAL_SYSTEM_SCU_CLK_ENABLE();
    __HAL_SYSTEM_GIC_CLK_ENABLE();
    __HAL_SYSTEM_GDMA00_CLK_ENABLE();
    __HAL_SYSTEM_GDMA01_CLK_ENABLE();
    __HAL_SYSTEM_PSW_LOCK();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_UART0_Init();

    setvbuf(stdout, NULL, _IONBF, 0);

    /* Initialize Test Framework Unity. */
    UNITY_BEGIN();
    UnityPrint("==== Initialize Test Framework Unity ====");
    UNITY_PRINT_EOL();
    UNITY_PRINT_EOL();

    __HAL_SYSTEM_PSW_UNLOCK();
#ifdef QSPI_TEST_ENABLE
    start_core1();
    __HAL_SYSTEM_QSPI0_CLK_ENABLE();
    __HAL_SYSTEM_QSPI1_CLK_ENABLE();
    RUN_TEST(test_qspi,  MAKE_UNITY_NUM(k_unity_dma, 1));
#endif
#ifdef USART_TEST_ENABLE
    __HAL_SYSTEM_USART0_CLK_ENABLE();
    RUN_TEST(test_usart, MAKE_UNITY_NUM(k_unity_dma, 2));
#endif
#ifdef SEA_TEST_ENABLE
    RUN_TEST(test_sea,   MAKE_UNITY_NUM(k_unity_dma, 3)); // PASS
#endif
#ifdef CRC_TEST_ENABLE
    __HAL_SYSTEM_CRC0_CLK_ENABLE();
    RUN_TEST(test_crc,   MAKE_UNITY_NUM(k_unity_dma, 4)); // PASS
#endif
#ifdef M2M_TEST_ENABLE
    __HAL_SYSTEM_M2M_CLK_ENABLE();
    RUN_TEST(test_m2m,   MAKE_UNITY_NUM(k_unity_dma, 5)); // PASS
#endif
    __HAL_SYSTEM_PSW_LOCK();

#ifdef ENABLE_GCOV_TEST
    extern long gcov_collect(unsigned long interface);
    gcov_collect(2);
#endif

    /* Test End. */
    UNITY_END();
    if (Unity.TestFailures == 0U) {
        test_pass();
    }
    printf("==== Test End ====\n");

    while(1) {
        WDT_FEED;
    }
}

static void test_pass()
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

