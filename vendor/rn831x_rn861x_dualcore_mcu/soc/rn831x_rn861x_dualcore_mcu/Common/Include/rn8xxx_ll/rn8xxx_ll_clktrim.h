/******************************************************************************
 * @file     rn8xxx_ll_sysclk.h
 * @brief    System clock driver header
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#ifndef RN8XXX_LL_CLKTRIM_H_
#define RN8XXX_LL_CLKTRIM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rn8xxx_ll_devices.h"
/**********************************************************
  api return
**********************************************************/
typedef enum {
    CLK_TRIM_PASS = 0U,
    CLK_TRIM_TIMEOUT = 1U,
    CLK_TRIM_OUT_OF_RANGE = 2U,
    CLK_TRIM_UNSTABLE = 3U,
    CLK_TRIM_NO_CLK = 4U,
    CLK_TRIM_REG_ERROR = 5U,
    CLK_TRIM_INPROCESS = 6U
} eClkTrimRet_TypeDef;

typedef enum {
    CLK_TRIM_IRQ_DIS = 0U,
    CLK_TRIM_IRQ_EN = 1U
} eclkTrimIRQ_TypeDef;

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN831x_RN861x_MCU_V4)
/**
 * @brief trim rch(29 or 32Mhz) by losc
 *
 * @param err_thousandth : rch clk err, one hundred thousandth unit
 *                         if pass NULL, no clk err return
 * @return eClkTrimRet_TypeDef
 */
eClkTrimRet_TypeDef LL_CLKTRIM_RchTrimByLosc(int *err_thousandth);
#define RchTrimByLosc LL_CLKTRIM_RchTrimByLosc

/**
 * @brief trim rcl(32Khz) by rch
 *
 *  Calculating time is about 52ms
 *
 * @return eClkTrimRet_TypeDef CLK_TRIM_PASS or fail
 */
eClkTrimRet_TypeDef LL_CLKTRIM_RclTrimByRch(void);

/**
 * @brief monitor sysclk(cmp rch and pll) asynchrously
 *  asynchrous monitor will use interrupt, NVIC_EnableIRQ(0U) needs to be added after
 *  LL_CLKTRIM_SysClkMonitorAsync(). Add corresponding code in ISRRef. to application notes document).
 * @return eClkTrimRet_TypeDef CLK_TRIM_PASS or fail
 */
eClkTrimRet_TypeDef LL_CLKTRIM_SysClkMonitorAsync(void);

/**
 * @brief get sysclk monitor status
 *  Use the interface in interrupt handler to check if sysclk is stable (Ref. to application notes document)
 * @param clk_jitter_ppm clk jitter in monitor time
 * @return eClkTrimRet_TypeDef CLK_TRIM_PASS or fail
 */
eClkTrimRet_TypeDef LL_CLKTRIM_SysClkMonitorAsyncStatus(int *clk_jitter_ppm);

/**
 * @brief monitor sysclk(cmp rch and pll) synchrously
 *
 * @param clk_jitter_ppm clk jitter in monitor time
 * @return eClkTrimRet_TypeDef CLK_TRIM_PASS or fail
 */
eClkTrimRet_TypeDef LL_CLKTRIM_SysClkMonitorSync(int *clk_jitter_ppm);

/**
 * @brief check if losc is stop
 *
 * @return eClkTrimRet_TypeDef CLK_TRIM_PASS or fail
 */
eClkTrimRet_TypeDef LL_CLKTRIM_LoscStopCheck(void);

#endif

#ifdef __cplusplus
}
#endif

#endif
/* r2989 */
