/******************************************************************************
 * @file     rn8xxx_v2_rtc_lib.h
 * @brief    rtc driver header
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#ifndef RN8XXX_LL_RTC_LIB_H_
#define RN8XXX_LL_RTC_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rn8xxx_ll_devices.h"

/**********************************************************
  api return
**********************************************************/
typedef enum
{
    RTC_PASS = 0,
    RTC_FAIL = 1,
} eRtcRet_TypeDef;

/**
 * @brief LL_RTC_ReadDota
 *
 * @param
 *    none
 * @return
 *    unsigned short dota: dota value of RTC
 *
 */
unsigned short  LL_RTC_ReadDota(void);
#define RtcReadDota LL_RTC_ReadDota

/**
 * @brief LL_RTC_WriteDota
 *
 * @param
 *    in unsigned short dota: dota value of RTC
 * @return
 *    NVM_PASS: rtc-dota write pass
 *    NVM_FAIL: rtc-dota write fail
 *
 */
eRtcRet_TypeDef LL_RTC_WriteDota(unsigned short dota);
#define RtcWriteDota LL_RTC_WriteDota

#if defined(RN_CM0_PLATFORM)      || \
    defined(RN202x_RN7326_SOC_V2) || \
    defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3) || \
    defined(RN821x_RN721x_SOC_V2) || \
    defined(RN821x_RN721x_SOC_V3) || \
    defined(RN831x_RN861x_MCU_V4)

typedef enum {
    RTC_CLK_IS_LOSC = 0,
    RTC_CLK_IS_RCL = 1
} rtc_clk_e;

/**
 * @brief LL_Rtc_ClkSel
 *   losc -> rcl:
 *     1. change vbat clk sel to rcl
 *     2. open vcc rcl path
 *   rcl -> losc:
 *     1. close vcc rcl path
 *     2. change vbat clk sel to losc
 * @param clk_sel RTC_CLK_IS_LOSC select losc as vbat clock
 *                RTC_CLK_IS_RCL select rcl as vbat clock
 */
void LL_RTC_ClkSel(rtc_clk_e clk_sel);
#endif

#ifdef __cplusplus
}
#endif

#endif
/* r2989 */
