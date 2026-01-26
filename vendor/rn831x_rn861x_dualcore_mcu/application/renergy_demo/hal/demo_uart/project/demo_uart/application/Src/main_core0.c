/* USER CODE BEGIN Header */
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
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "rn_rv_ll_wdt.h"
#include <stdio.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
#define ACK_OK "ACK_OK\r\n"
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
ssize_t _write(int fd, const void* ptr, size_t len);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t tx_buf[] = "\r\nsend ------> hello, world!\r\n";
uint8_t rx_data[1];
uint8_t rx_index = 0;
uint8_t rx_buf[256] = {0};
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    uint32_t sysclockfreq = 0, result = 0xFF, counter = 0;
    /* USER CODE BEGIN 1 */
    uint8_t txmsg0[] = "hello, world!\r\n";
    uint8_t txmsg1[] = "uart1, hello!\r\n";
    uint8_t activemsg[] = ">>\r\n";
    uint8_t abortmsg[] = "\r\nTransmit abort ------> send\r\n";
    uint8_t abortendmsg[] = "\r\nsend ------> Abort end\r\n";
    uint32_t i;
    uint16_t RxLen;
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

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    /* uart0 for printf() */
    MX_UART0_Init();
    /* uart1 for uart test demo */
    MX_UART1_Init();
    /* uart3 for uart test demo */
    MX_UART3_Init();
    /* uart6 for uart test demo */
    MX_UART6_Init();

    /* USER CODE BEGIN 2 */

    printf("\r\n==== UART Test Start Core0 ====\r\n");

    printf("\r\nhello world\r\n");

#if 1
    /* USER CODE END 2 */
    printf("\r\nUART Transmit TEST Start, clk is %ldHz\r\n", sysclockfreq);

    /****** transmit data in non blocking mode (interrupt mode) ******/
    /* 1. hello, world!\r\n */
    HAL_UART_Transmit_IT(&huart1, txmsg0, strlen((const char *)txmsg0));

    while(huart1.gState != HAL_UART_STATE_READY)
    {
    };
    /* 2. uart1, hello!\r\n */
    HAL_UART_Transmit_IT(&huart1, txmsg1, strlen((const char *)txmsg1));

    /* 3. hello, world!\r\n */
    for (i = 0; i < strlen((const char *)txmsg0); i++) {
        while (HAL_UART_Transmit_IT(&huart1, (const uint8_t *)&txmsg0[i], 1) != HAL_OK)
        {
        };
    }

    while(huart1.gState != HAL_UART_STATE_READY)
    {
    };
    /* 4. uart1, hello!\r\n  but abort*/
    HAL_UART_Transmit_IT(&huart1, txmsg1, strlen((const char *)txmsg1));
    HAL_Delay(1);
    HAL_UART_AbortTransmit(&huart1);
    HAL_Delay(1);
    HAL_UART_Transmit(&huart1, abortmsg, strlen((const char *)abortmsg), 0xFF);
    /* 5. uart1, hello!\r\n *//* 4. uart1, hello!\r\n */
    HAL_UART_Transmit_IT(&huart1, txmsg1, strlen((const char *)txmsg1));

    printf("\r\nUART DMA Transmit TEST Start, clk is %ldHz\r\n", sysclockfreq);
    HAL_UART_Transmit_DMA(&huart1, tx_buf, strlen((const char *)tx_buf));
    while(huart1.gState != HAL_UART_STATE_READY)
    {
    };
    HAL_UART_Transmit_DMA(&huart1, tx_buf, strlen((const char *)tx_buf));
    HAL_Delay(1);
    /* abort use HAL_UART_Abort */
    HAL_UART_Abort(&huart1);
    HAL_Delay(1);
    result = HAL_UART_Transmit_DMA(&huart1, abortmsg, strlen((const char *)abortmsg));
    HAL_Delay(1);
    /* abort use HAL_UART_AbortTransmit */
    HAL_UART_AbortTransmit(&huart1);
    HAL_Delay(1);
    HAL_UART_Transmit_DMA(&huart1, tx_buf, strlen((const char *)tx_buf));
    HAL_Delay(1);
    /* abort use HAL_UART_Abort_IT will call HAL_UART_AbortCpltCallback */
    HAL_UART_Abort_IT(&huart1);
    HAL_Delay(1);
    HAL_UART_Transmit_DMA(&huart1, abortmsg, strlen((const char *)abortmsg));
    HAL_Delay(1);
    /* abort use HAL_UART_AbortTransmit_IT will call HAL_UART_AbortTransmitCpltCallback */
    HAL_UART_AbortTransmit_IT(&huart1);
    HAL_Delay(1);
    HAL_UART_Transmit_DMA(&huart1, tx_buf, strlen((const char *)tx_buf));
    HAL_Delay(1);
    /* abort use HAL_UART_DMAStop */
    HAL_UART_DMAStop(&huart1);
    HAL_Delay(1);
    HAL_UART_Transmit_DMA(&huart1, abortendmsg, strlen((const char *)abortendmsg));
#endif

    printf("\r\nUART Receive TEST Start, clk is %ldHz\r\n", sysclockfreq);

    /****** receive data in blocking mode ******/
//    HAL_UART_Transmit_DMA(&huart3, tx_buf, strlen((const char *)tx_buf));

//    HAL_UART_Receive(&huart1, rx_buf, strlen((const char *)tx_buf), 0xFF);

//    if (strcmp((const char *)rx_buf, (const char *)tx_buf) == 0) {
//        HAL_UART_Transmit(&huart1, (const uint8_t *)ACK_OK, strlen((const char *)ACK_OK), 0xFF);
//        HAL_UART_Transmit(&huart1, rx_buf, strlen((const char *)rx_buf), 0xFF);
//    }

    memset((void *)rx_buf, 0, sizeof(rx_buf));
    /* Fixed-length reception */
    /* IT */
//    HAL_UART_Receive_IT(&huart1, rx_buf, 5);
    /* DMA */
//    HAL_UART_Receive_DMA(&huart1, rx_buf, 5);

    /* Variable-length reception */
//    HAL_UART_Receive_IT(&huart1, rx_data, 1);
//    HAL_UART_Receive_DMA(&huart1, rx_data, 1);

    /* Variable-length reception */
    /* Receive to IDLE (Timeout), Timeout length can set by huart1.Init.RxTimeout */
//    HAL_UARTEx_ReceiveToIdle(&huart1, rx_buf, sizeof(rx_buf), &RxLen, 0xFF);
//    printf("\r\nfinally received %d data, reception ends on IDLE(Timeout) event\r\n", RxLen);
//    HAL_UART_Transmit(&huart1, rx_buf, strlen((const char *)rx_buf), 0xFF);

//    printf("\r\nreceive HAL_UARTEx_ReceiveToIdle_IT Start\r\n");
//    HAL_UARTEx_ReceiveToIdle_IT(&huart1, rx_buf, sizeof(rx_buf));

    printf("\r\nreceive HAL_UARTEx_ReceiveToIdle_DMA Start\r\n");
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buf, sizeof(rx_buf));
    HAL_UARTEx_ReceiveToIdle_DMA(&huart6, rx_buf, sizeof(rx_buf));

    printf("\r\nUART1 TEST END\r\n");
//    HAL_UART_DeInit(&huart1);
//    HAL_UART_DeInit(&huart3);
//    HAL_UART_DeInit(&huart6);

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */

    /* use gcov for Coverage of test */
//    extern long gprof_collect(unsigned long interface);
//    extern long gcov_collect(unsigned long interface);
//    gprof_collect(2);
//    gcov_collect(2);
    while (1) {
//        HAL_Delay(100);
//        HAL_UART_Receive(&huart1, rx_buf, strlen((const char *)tx_buf), 0xFF);
//        if (strcmp((const char *)rx_buf, (const char *)tx_buf) == 0) {
//            HAL_UART_Transmit(&huart1, (const uint8_t *)ACK_OK, strlen((const char *)ACK_OK), 0xFF);
//            HAL_UART_Transmit(&huart1, rx_buf, strlen((const char *)rx_buf), 0xFF);
//            memset(rx_buf, 0, 256);
//        }
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        /* feed WDT */
        counter++;
        if (counter == 15000) {
            HAL_UART_Transmit(&huart6, activemsg, strlen((const char *)activemsg), 0xFF);
            counter = 0;
        }
        LL_WDT_ReloadCounter(WDT);
    }
    /* USER CODE END 3 */

}

/* USER CODE BEGIN 4 */

/* IDLE(Timeout) RX Event */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == &huart1)
    {
        HAL_UART_Transmit(huart, rx_buf, Size, 0xFF);
        /* IT */
//        HAL_UARTEx_ReceiveToIdle_IT(&huart1, rx_buf, sizeof(rx_buf));
        /* DMA */
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buf, sizeof(rx_buf));
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint8_t msg[] = "receive -----> OK\r\n";
    uint8_t tx_msg0[] = "receive OK-----> 0\r\n";
    uint8_t tx_msg1[] = "receive OK-----> 1\r\n";
    uint8_t abort_msg0[] = "abort receive OK -----> HAL_UART_Abort\r\n";
    uint8_t abort_msg1[] = "abort receive OK -----> HAL_UART_AbortReceive\r\n";
    uint8_t abort_msg2[] = "abort receive OK -----> HAL_UART_Abort_IT\r\n";
    uint8_t abort_msg3[] = "abort receive OK -----> HAL_UART_AbortReceive_IT\r\n";
    uint8_t abort_msg4[] = "abort receive OK -----> DMAAbortEnd\r\n";
    if (huart == &huart1)
    {
        /* Fixed-length reception */
//         HAL_UART_Transmit(huart, msg, strlen((const char *)msg), 0xFF);

//         printf("read -----> %s\n", rx_buf);
//         HAL_UART_Receive_IT(&huart1, rx_buf, 5);
//         if (strcmp((const char *)rx_buf, "Abort") == 0 ) {
//            HAL_UART_Abort(huart);
//            HAL_UART_Transmit(huart, abort_msg0, strlen((const char *)abort_msg0), 0xFF);
//            memset((void *)rx_buf, 0, sizeof(rx_buf));
//            HAL_UART_Receive_DMA(&huart1, rx_buf, 12);
//         }

//         if (strcmp((const char *)rx_buf, "AbortReceive") == 0 ) {
//            HAL_UART_AbortReceive(huart);
//            HAL_UART_Transmit(huart, abort_msg1, strlen((const char *)abort_msg1), 0xFF);
//            memset((void *)rx_buf, 0, sizeof(rx_buf));
//            HAL_UART_Receive_DMA(&huart1, rx_buf, 8);
//         }
//         if (strcmp((const char *)rx_buf, "Abort_IT") == 0 ) {
//            HAL_UART_Abort_IT(huart);
//            HAL_UART_Transmit(huart, abort_msg2, strlen((const char *)abort_msg2), 0xFF);
//            memset((void *)rx_buf, 0, sizeof(rx_buf));
//            HAL_UART_Receive_DMA(&huart1, rx_buf, 15);
//         }
//         if (strcmp((const char *)rx_buf, "AbortReceive_IT") == 0 ) {
//            HAL_UART_AbortReceive_IT(huart);
//            HAL_UART_Transmit(huart, abort_msg3, strlen((const char *)abort_msg3), 0xFF);
//            memset((void *)rx_buf, 0, sizeof(rx_buf));
//            HAL_UART_Receive_DMA(&huart1, rx_buf, 11);
//         }
//         if (strcmp((const char *)rx_buf, "DMAAbortEnd") == 0 ) {
//            HAL_UART_DMAStop(huart);
//            HAL_UART_Transmit(huart, abort_msg4, strlen((const char *)abort_msg4), 0xFF);
//         }
//         HAL_UART_Receive_IT(&huart1, rx_buf, strlen((const char *)tx_buf));

        /* Variable-length reception */
//        rx_buf[rx_index++] = rx_data[0];
//        if (rx_data[0] == '\n' || rx_index >= sizeof(rx_buf) - 1) {
//            printf("read end -----> %s\n", rx_buf);
//            rx_index = 0;
//            memset((void *)rx_buf, 0, sizeof(rx_buf));
//        }
//        if (rx_data[0] == '0') {
//            HAL_UART_Transmit(huart, tx_msg0, strlen((const char *)tx_msg0), 0xFF);
//        }
//        if (rx_data[0] == '1') {
//            HAL_UART_Transmit(huart, tx_msg1, strlen((const char *)tx_msg1), 0xFF);
//        }
//        HAL_UART_Transmit(huart, rx_data, 1, 0xFF);
////        HAL_UART_Receive_IT(&huart1, rx_data, 1);
//        HAL_UART_Receive_DMA(&huart1, rx_data, 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    uint8_t overflow_msg[] = "overflow -----> receive re-continue\r\n";
    if (huart == &huart1)
    {
        HAL_UART_Transmit(huart, overflow_msg, strlen((const char *)overflow_msg), 0xFF);
        HAL_UART_Receive_IT(&huart1, rx_buf, 5);
    }
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
