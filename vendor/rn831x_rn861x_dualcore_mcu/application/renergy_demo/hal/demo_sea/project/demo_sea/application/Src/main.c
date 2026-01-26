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
#include "sea_test.h"
#include <stdio.h>

static void test_pass(void);
static void start_core1(void);

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

    printf("corehartid: %lu\n", HAL_GetCoreId());

    start_core1();

    /* NOTE: Need Stack space more than 15K. */
#ifndef SEA_HAL_FULL_TEST
    /* Testing independent AES module. */
    RUN_TEST(aes_ecb_test,       MAKE_UNITY_NUM(k_unity_aes, 1));
    WDT_FEED;
    RUN_TEST(aes_gcm_test,       MAKE_UNITY_NUM(k_unity_aes, 2));
    WDT_FEED;
    RUN_TEST(aes_gmac_test,      MAKE_UNITY_NUM(k_unity_aes, 3));
    WDT_FEED;
    RUN_TEST(aes_others_test,    MAKE_UNITY_NUM(k_unity_aes, 4));
    WDT_FEED;
    RUN_TEST(aes_keywrap_test,   MAKE_UNITY_NUM(k_unity_aes, 5));
    WDT_FEED;
    RUN_TEST(aes_updateAAD_test, MAKE_UNITY_NUM(k_unity_aes, 6));
#else
    /* Testing SEA Module. */
    RUN_TEST(aes_ecb_test,       MAKE_UNITY_NUM(k_unity_sea, 1));
    WDT_FEED;
    RUN_TEST(aes_gcm_test,       MAKE_UNITY_NUM(k_unity_sea, 2));
    WDT_FEED;
    RUN_TEST(aes_gmac_test,      MAKE_UNITY_NUM(k_unity_sea, 3));
    WDT_FEED;
    RUN_TEST(aes_others_test,    MAKE_UNITY_NUM(k_unity_sea, 4));
    WDT_FEED;
    RUN_TEST(aes_keywrap_test,   MAKE_UNITY_NUM(k_unity_sea, 5));
    WDT_FEED;
    RUN_TEST(aes_updateAAD_test, MAKE_UNITY_NUM(k_unity_sea, 6));
    WDT_FEED;

    RUN_TEST(hash_test,          MAKE_UNITY_NUM(k_unity_sea, 7));
    WDT_FEED;

    /* For IAP-SEA interface, NIST P-521 & Brainpool P-512 get timeout error. */
    RUN_TEST(ECC_sign_test,      MAKE_UNITY_NUM(k_unity_sea, 8));
    WDT_FEED;
    RUN_TEST(ECC_keygen_test,    MAKE_UNITY_NUM(k_unity_sea, 9));
    WDT_FEED;
    RUN_TEST(ECDH_NIST256_test,  MAKE_UNITY_NUM(k_unity_sea, 10));
    WDT_FEED;
    RUN_TEST(ECDH_NIST384_test,  MAKE_UNITY_NUM(k_unity_sea, 11));
    WDT_FEED;
    RUN_TEST(ECDSA_Ver_NIST256,  MAKE_UNITY_NUM(k_unity_sea, 12));
    WDT_FEED;
    RUN_TEST(ECDSA_Ver_NIST384,  MAKE_UNITY_NUM(k_unity_sea, 13));
    WDT_FEED;
    RUN_TEST(ECDSA_Ver_NIST521,  MAKE_UNITY_NUM(k_unity_sea, 14));
    WDT_FEED;

    RUN_TEST(rsa_test,           MAKE_UNITY_NUM(k_unity_sea, 15));
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
