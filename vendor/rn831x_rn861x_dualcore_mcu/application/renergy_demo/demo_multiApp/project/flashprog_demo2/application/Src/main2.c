/**
 ******************************************************************************
 * @file           : main.c
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
#include "main.h"
#include "uart.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "rn_rv_ll_wdt.h"
/* USER CODE END Includes */

int main(void)
{
    void (*Demo3_Entry)(unsigned int);
    unsigned int demo3_addr;
    unsigned int i;

    /* Enable WDT clock */
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_WDT_CLK_ENABLE();
    __HAL_SYSTEM_PSW_LOCK();

	/* Initialize uart */
    setvbuf(stdout, NULL, _IONBF, 0);
	MX_UART0_Init();

    printf("==== Program Demo2 ====\n");

    for(i = 0; i < 0x1000U; i++)
    {
        __NOP();
    	/*feed WDT*/
    	LL_WDT_ReloadCounter(WDT);
    }
    //Jump to flash_demo3
    demo3_addr = 0x8000U;
    Demo3_Entry = (void (*)(unsigned int))(demo3_addr);
    (*Demo3_Entry)(demo3_addr);
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
