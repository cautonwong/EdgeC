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
#include "rn_rv_ll_wdt.h"
#include "unity.h"
#include <stdio.h>

static void test_pass();

#define PMPADDR(x)      _AC(CSR_PMPADDR, x)
#define PMPCFG(x)       _AC(CSR_PMPCFG, x)

static uint32_t memory_pmp1[4];

void pmp_demo(void)
{
    printf("PMPCFG0 : 0x%lx\n", __get_PMPCFGx(0));
    printf("PMP0CFG - PMP3CFG: 0x%x 0x%x 0x%x 0x%x\n", \
            (uint32_t)__get_PMPxCFG(0), (uint32_t)__get_PMPxCFG(1),
            (uint32_t)__get_PMPxCFG(2), (uint32_t)__get_PMPxCFG(3));

    for (int i = 0; i < __PMP_ENTRY_NUM; i ++) {
        printf("%d PMPADDRx  PMPxCFG   0x%x 0x%x\n", i, (uint32_t)__get_PMPADDRx(i), (uint32_t)__get_PMPxCFG(i));
    }
    TEST_ASSERT_EQUAL_UINT(__get_PMPADDRx(0), __RV_CSR_READ(PMPADDR(0)));
    TEST_ASSERT_EQUAL_UINT(__get_PMPADDRx(4), __RV_CSR_READ(PMPADDR(4)));
    for (int i = 0; i < __PMP_ENTRY_NUM / 4; i ++) {
        printf("%d PMPCFGx  0x%lx\n", i, __get_PMPCFGx(i));
    }
    __set_PMPADDRx(0, (rv_csr_t)(&memory_pmp1[0]) >> 2);
    __set_PMPxCFG(0, PMP_A_NA4 | PMP_R | PMP_W);
    printf("PMPADDR0  PMP0CFG   0x%x 0x%x\n", (uint32_t)__get_PMPADDRx(0), (uint32_t)__get_PMPxCFG(0));
    __set_PMPADDRx(1, (rv_csr_t)(&memory_pmp1[4]) >> 2);
    __set_PMPxCFG(1, PMP_A_NA4 | PMP_R | PMP_W);
    printf("PMPADDR1  PMP1CFG   0x%x 0x%x\n", (uint32_t)__get_PMPADDRx(1), (uint32_t)__get_PMPxCFG(1));
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

    RUN_TEST(pmp_demo, 0);

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
