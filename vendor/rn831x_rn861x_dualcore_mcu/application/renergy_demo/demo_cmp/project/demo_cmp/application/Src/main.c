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
#include "rn_rv_ll_lvdcmp.h"
#include "rn_rv_ll_wdt.h"
#include <stdio.h>

#define CMP_HIGH  1
#define CMP_LOW   0

void cmp_delay(void)
{
    unsigned int i;
    volatile uint8_t res;
    // 9cycles per loop at OZ(Os/Ofast/O2), 9/1.8432=4.8us...9/29.4912=0.3us
    // delay = 3us
    for (i = 0; i < 10; i++) {
        res = LL_LVDCMP_IsActiveFlag_CMP1(LVDCMP);
    }
    return;
}

uint8_t get_cmp1_sta(void)
{
    unsigned int i;
    volatile uint8_t res;

    LL_LVDCMP_DisableCMP1(LVDCMP);
    cmp_delay(); // must delay
    LL_LVDCMP_ClearFlag_CMP1(LVDCMP);

    LL_LVDCMP_EnableCMP1(LVDCMP);
    cmp_delay(); // must delay
    res = LL_LVDCMP_IsActiveFlag_CMP1(LVDCMP);
    LL_LVDCMP_ClearFlag_CMP1(LVDCMP);

    if (res == 1) {
        return CMP_LOW;
    } else {
        return CMP_HIGH;
    }
}

int main(void)
{
    /* Change to 29Mhz. */
    HAL_SYSTEM_ClockModeConfig(HAL_SYSTEM_CLKMODE_RC_59M, HAL_SYSTEM_SRCDIV_2, HAL_SYSTEM_COREDIV_CORE1_1_CORE0_1);

    /* Reset of all peripherals, Initializes the Systick. */
    HAL_Init();

    /* Enable WDT clock */
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_WDT_CLK_ENABLE();
    __HAL_SYSTEM_GPIO_CLK_ENABLE();
    __HAL_SYSTEM_LVDCMP_CLK_ENABLE();
    __HAL_SYSTEM_PSW_LOCK();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_UART0_Init();

    uint8_t res, cmp;

    LL_LVDCMP_EnableCMP1(LVDCMP);
    // LL_LVDCMP_EnableCMP2(LVDCMP);
    LL_LVDCMP_EnableIT_CMP1(LVDCMP);
    // LL_LVDCMP_EnableIT_CMP2(LVDCMP);

    cmp = get_cmp1_sta();
    while (1) {
        LL_WDT_ReloadCounter(WDT);
        res = get_cmp1_sta();
        if (cmp != res) {
            if (res == CMP_HIGH) {
                printf("+++POWER UP!\n");
            } else {
                printf("---POWER DOWN!\n");
            }
            cmp = res;
        }
        HAL_Delay(20);
    }

    while(1) {
        LL_WDT_ReloadCounter(WDT);
    }
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

