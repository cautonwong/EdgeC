/******************************************************************************
 * @file    rn8xxx_wdt.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-09-04
 * @brief   Header file containing functions prototypes of WDT HAL library.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2016 Renergy.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef Rn8xxx_WDT_H
#define Rn8xxx_WDT_H

/* Includes ------------------------------------------------------------------*/
/*#include <stdint.h>*/
#include "rn8xxx_ll_def.h"
/* Exported define ------------------------------------------------------------*/
#define WDT_TIMEOUT 0x3fff

/* Exported enum ------------------------------------------------------------*/
typedef enum
{
    WDT_WDCS_16ms = 0,  /* \看门狗定时器的溢出时间为16ms */
    WDT_WDCS_32ms = 1,  /* \看门狗定时器的溢出时间为32ms */
    WDT_WDCS_128ms = 2, /* \看门狗定时器的溢出时间为128ms */
    WDT_WDCS_512ms = 3, /* \看门狗定时器的溢出时间为512ms */
    WDT_WDCS_1s = 4,    /* \看门狗定时器的溢出时间为1s(def) */
    WDT_WDCS_2s = 5,    /* \看门狗定时器的溢出时间为2s */
    WDT_WDCS_4s = 6,    /* \看门狗定时器的溢出时间为4s */
    WDT_WDCS_8s = 7,    /* \看门狗定时器的溢出时间为8s */
} eWDT_WDCS_TypeDef;    /* \看门狗定时器的溢出时配置 */

typedef enum
{
    WDT_Window_25 = 0,  /* \25% */
    WDT_Window_50 = 1,  /* \50% */
    WDT_Window_75 = 2,  /* \75% */
    WDT_Window_100 = 3, /* \100% (def)*/
} eWDT_Window_TypeDef;  /* \窗口打开周期 */

typedef enum
{
    WDT_Int_Disable = 0, /* \不使用间隔中断*/
    WDT_Int_Enable = 1,  /* \达到溢出事件75%时产生间隔中段*/
} eWDT_Int_TypeDef;      /* \看门狗定时器间隔中断 */

#if defined(RN831x_RN861x_MCU_V4)
typedef enum
{
    WDT_CLKDIV_No  = 0,  /* \不分频(def) */
    WDT_CLKDIV_2   = 1,  /* \2分频 */
    WDT_CLKDIV_4   = 2,  /* \4分频 */
    WDT_CLKDIV_8   = 3,  /* \8分频*/
    WDT_CLKDIV_16  = 4,  /* \16分频 */
    WDT_CLKDIV_32  = 5,  /* \32分频 */
    WDT_CLKDIV_64  = 6,  /* \64分频*/
    WDT_CLKDIV_128 = 7,  /* \128分频*/
} eWDTCLKDIV_TypeDef;  /* \看门狗时钟分频 */

typedef enum
{
    WDT_RADWAKE_Disable = 0,  /* \不使用任意唤醒功能 */
    WDT_RADWAKE_Enable  = 1,  /* \根据WAKE_CNT配置值进行唤醒 */
} eWDT_RADWAKE_TypeDef;      /* \看门狗任意唤醒 */

typedef enum
{
    WDT_HALT_Disable = 0xA5A5A581U, /* \如果 HALT_EN=0xA5A5A581, 那么调试时暂停 WDT 计数*/
    WDT_HALT_Enable  = 0x5A5A5A7EU, /* \如果 HATL_EN=其他值, 那么调试时不暂停WDT 计数*/
} eWDT_HALT_TypeDef;                /* \WDT HALT */

typedef enum
{
    WDT_STBY_Disable = 0xA5A5A5C7U, /* \如果 STBY_EN=32’h A5A5A5C7, 那么当CPU睡眠时关闭 WDT*/
    WDT_STBY_Enable  = 0x5A5A5A38U, /* \如果 STBY_EN=其他值, 那么当 CPU 睡眠时不关闭 WDT*/
} eWDT_STBY_TypeDef;                /* \WDT STBY */
#else
typedef enum
{
    WDT_HALT_Disable = 0x7e5a, /* \如果 HALT_EN=0x7e5a， 那么暂停 WDT 计数*/
    WDT_HALT_Enable = 0,       /* \如果 HATL_EN=其他值， 那么不暂停WDT 计数*/
} eWDT_HALT_TypeDef;           /* \CPU 处于 HALT 时（调试时暂停） */

typedef enum
{
    WDT_STBY_Disable = 0xa538, /* \如果 STBY_EN=16’h a538， 那么 CPU 睡眠时关 WDT (def)*/
    WDT_STBY_Enable = 0,       /* \如果 STBY_EN=其他值， 那么 CPU 睡眠时不关闭 WD*/
} eWDT_STBY_TypeDef;           /* \如果 CPU 处于 sleep 或 deepSleep 时（睡眠时使用） */
#endif

/* Exported types ------------------------------------------------------------*/
typedef struct
{
#if defined(RN831x_RN861x_MCU_V4)
    uint32_t WakeCnt;              /* \看门狗任意唤醒计数值 */
    eWDT_RADWAKE_TypeDef RadWake;  /* \看门狗任意唤醒 */
    eWDTCLKDIV_TypeDef Clkdiv;     /* \看门狗时钟分频 */
#endif
    eWDT_WDCS_TypeDef wdcs;     /* \看门狗的溢出时间 */
    eWDT_Window_TypeDef Window; /* \窗口打开周期 */
    eWDT_Int_TypeDef Int;       /* \看门狗间隔中断 */
    eWDT_HALT_TypeDef Halt;     /* \CPU 处于 HALT 时（调试时使用），WDT的状态控制 */
    eWDT_STBY_TypeDef Stby;     /* \ CPU 处于 sleep 或 deepSleep 时（睡眠时使用），WDT的状态控制 */
} sWDT_Cfg_TypeDef;
/* Exported data ----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
extern void LL_WDT_CfgDef(sWDT_Cfg_TypeDef *Cfg);
extern void LL_WDT_Cfg(const sWDT_Cfg_TypeDef *Cfg);
extern void LL_WDT_RloadDefCfg(void);
extern void LL_WDT_ReloadCounter(void);
#endif /*Rn8xxx_WDT_H*/

/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
