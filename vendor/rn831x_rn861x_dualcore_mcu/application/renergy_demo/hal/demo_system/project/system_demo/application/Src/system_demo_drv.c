/**
  *****************************************************************************
  * @file   system_demo_drv.c
  * @author Renergy Software Developers
  * @brief  Test source code
  *
  *****************************************************************************
  * @attention
  *
  * Copyright (c) 2025, Renergy Technology Inc.
  * All rights reserved.
  *
  *****************************************************************************
  */
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wunused-variable"

#include "system_demo_drv.h"
#include "uart.h"

/* Note: CORE0 clock <= 29M5*/
/* Note: CORE0 clock <= CORE1 clock*/
uint8_t hal_system_control_func(void)
{
    uint8_t funStat = 0;
    /*1. change clock mode*/
    /*Change to LOSC*/
    funStat = HAL_SYSTEM_ClockModeConfig(HAL_SYSTEM_CLKMODE_LOSC, HAL_SYSTEM_SRCDIV_1, HAL_SYSTEM_COREDIV_CORE1_1_CORE0_1);
    ASSERT_ERRSET(funStat, 0x1);
    /*Change to RC59M(CORE1), RC29M5(CORE0)*/
    funStat = HAL_SYSTEM_ClockModeConfig(HAL_SYSTEM_CLKMODE_RC_59M, HAL_SYSTEM_SRCDIV_1, HAL_SYSTEM_COREDIV_CORE1_1_CORE0_2);
    ASSERT_ERRSET(funStat, 0x1);
    /*Change to RC29M5(CORE1), RC29M5(CORE0)*/
    funStat = HAL_SYSTEM_ClockModeConfig(HAL_SYSTEM_CLKMODE_RC_59M, HAL_SYSTEM_SRCDIV_2, HAL_SYSTEM_COREDIV_CORE1_1_CORE0_1);
    ASSERT_ERRSET(funStat, 0x1);

    /*2. fast mode change*/
    funStat = HAL_SYSTEM_FastClockModeConfig(HAL_SYSTEM_CLKMODE_RC_1M8);
    ASSERT_ERRSET(funStat, 1);

    /*3. set core division*/
    /*Change to RC59M(CORE1), RC29M5(CORE0)*/
    funStat = HAL_SYSTEM_ClockModeConfig(HAL_SYSTEM_CLKMODE_RC_59M, HAL_SYSTEM_SRCDIV_1, HAL_SYSTEM_COREDIV_CORE1_1_CORE0_2);
    ASSERT_ERRSET(funStat, 1);
    /*set CORE1 division, change to RC29M5(CORE1), RC29M5(CORE0)*/
    funStat = HAL_SYSTEM_SetCoreDivision(HAL_SYSTEM_COREDIV_CORE1_2_CORE0_2);
    ASSERT_ERRSET(funStat, 1);

    /*4. configure pll clock at other clock mode*/
    funStat = HAL_SYSTEM_ClockModeConfig(HAL_SYSTEM_CLKMODE_RC_59M, HAL_SYSTEM_SRCDIV_1, HAL_SYSTEM_COREDIV_CORE1_1_CORE0_2);
    ASSERT_ERRSET(funStat, 1);
    /*turn on PLL29M5*/
    funStat = HAL_SYSTEM_PLLConfig(HAL_SYSTEM_CLKMODE_PLL_29M5, HAL_SYSTEM_PLL_ON);
    ASSERT_ERRSET(funStat, 1);
    /*turn off PLL*/
    funStat = HAL_SYSTEM_PLLConfig(HAL_SYSTEM_CLKMODE_PLL_29M5, HAL_SYSTEM_PLL_OFF);
    ASSERT_ERRSET(funStat, 1);

    /*recover clock mode*/
    funStat = HAL_SYSTEM_ClockModeConfig(HAL_SYSTEM_CLKMODE_RC_1M8, HAL_SYSTEM_SRCDIV_1, HAL_SYSTEM_COREDIV_CORE1_1_CORE0_1);
    ASSERT_ERRSET(funStat, 0x1);

    return funStat;
}

#pragma GCC diagnostic pop

