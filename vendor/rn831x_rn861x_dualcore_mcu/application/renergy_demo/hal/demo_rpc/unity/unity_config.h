#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "uart.h"

#define UNITY_OUTPUT_CHAR(c) 	do { \
									uint8_t unity_char = (c); \
									HAL_UART_Transmit(&huart0 , &unity_char, 1, 0xFF); \
								} while(0);
#define UNITY_OUTPUT_START() 	MX_UART0_Init()

#define UNITY_SHOW_TEST_FILE_PATH          /* open file path when test failed */
#define UNITY_SHOW_TEST_FAILED_LINE        /* open test line when use test template test failed  */
#define UNITY_DIFFERENTIATE_FINAL_FAIL     /* open "ED", from "FAIL" to "FAILED"*/
#define UNITY_INCLUDE_PRINT_FORMATTED

enum _unity_module
{
    k_unity_aes     = 1,
    k_unity_tc      = 2,
    k_unity_aos     = 3,
    k_unity_cmu     = 4,
    k_unity_crc     = 5,
    k_unity_gdma    = 6,
    k_unity_simptc  = 7,
    k_unity_flash   = 8,
    k_unity_can     = 9,
    k_unity_dma     = 10,
    k_unity_gpio    = 11,
    k_unity_analog  = 12,
    k_unity_i2c     = 13,
    k_unity_d2f     = 14,
    k_unity_iso7816 = 15,
    k_unity_lvdcmp  = 16,
    k_unity_kbi     = 17,
    k_unity_ks      = 18,
    k_unity_lcd     = 19,
    k_unity_lcss    = 20,
    k_unity_lpuart  = 21,
    k_unity_dsp     = 22,
    k_unity_rs_vdd  = 23,
    k_unity_m2m     = 24,
    k_unity_mailbox = 25,
    k_unity_qspi    = 26,
    k_unity_rmu     = 27,
    k_unity_vbat    = 28,
    k_unity_sea     = 29,
    k_unity_dac     = 30,
    k_unity_spi     = 31,
    k_unity_hsadc   = 32,
    k_unity_uart    = 33,
    k_unity_usart   = 34,
    k_unity_gic     = 35,
    k_unity_scu     = 36,
    k_unity_sys_reg = 37,
    k_unity_iocnt   = 38,
    k_unity_wdt     = 39,
    k_unity_lsrx    = 40,
    k_unity_rom     = 41,
    k_unity_sys_tc  = 42,
};

#define MAKE_UNITY_NUM(unity_module, caseID) (((uint32_t)(unity_module)*10000) + (uint32_t)(caseID))

/* end of UNITY_CONFIG_H */
#ifdef __cplusplus
}
#endif

#endif
