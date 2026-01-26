/*******************************************************************************
  * @file    Rn8xxx_WDT.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-08-29
  * @brief   看门狗模块驱动
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 Renergy.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  *@verbatim
  ==============================================================================
                     ##### WDT peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]


    @endverbatim
  */

/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll.h"
#ifdef LL_WDT_MODULE_ENABLED
/* Private define ------------------------------------------------------------*/
#if defined(RN831x_RN861x_MCU_V4)
#define WDT_REG_CTL (WDT->CTL)  /*"WDT 配置寄存器"*/
#define WDT_REG_PASS (WDT->PASS) /*"WDT 密码寄存器"*/
#define WDT_REG_HALT (WDT->HALT) /*"控制 CPU HALT 时 WDT 是否停止计数"*/
#define WDT_REG_STBY (WDT->STBY) /*"控制 CPU SLEEP 时 WDT 是否停止计数"*/
#else
#define WDT_REG_CTL (*(uint32_t *)(0x40030004))  /*"WDT 配置寄存器"*/
#define WDT_REG_PASS (*(uint32_t *)(0x40030008)) /*"WDT 密码寄存器"*/
#define WDT_REG_HALT (*(uint32_t *)(0x40030014)) /*"控制 CPU HALT 时 WDT 是否停止计数(spl1)"*/
#define WDT_REG_STBY (*(uint32_t *)(0x40030018)) /*"控制 CPU SLEEP 时 WDT 是否停止计数(spl1)"*/
#endif
/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/
/*********************************************************************************************************
** Function name:
** Descriptions:     提供看门狗配置默认参数，仅改RAM值，不动芯片寄存器
** input parameters:
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_WDT_CfgDef(sWDT_Cfg_TypeDef *Cfg)
{
#if defined(RN831x_RN861x_MCU_V4)
    Cfg->RadWake = WDT_RADWAKE_Disable;
    Cfg->Clkdiv = WDT_CLKDIV_No;
    Cfg->WakeCnt = 0;
#endif
    Cfg->wdcs = WDT_WDCS_1s;
    Cfg->Window = WDT_Window_100;
    Cfg->Int = WDT_Int_Disable;
    Cfg->Halt = WDT_HALT_Disable;
    Cfg->Stby = WDT_STBY_Disable;
}

/*********************************************************************************************************
** Function name:
** Descriptions:     看门狗配置函数
** input parameters:
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_WDT_Cfg(const sWDT_Cfg_TypeDef *Cfg)
{
    WDT_REG_PASS = 0x70; /*"开启写保护"*/
    WDT_REG_PASS = 0x7e;
    if (fnRegState_Detect((uint32_t)(&WDT->EN), 8, 0, WDT_TIMEOUT) == ERN_SUCCESS)
    {
#if defined(RN831x_RN861x_MCU_V4)
        WDT_REG_CTL = (((uint32_t)Cfg->wdcs << 0U)  |
                       ((uint32_t)Cfg->Window << 5) |
                       ((uint32_t)Cfg->Int << 7)    |
                       ((uint32_t)Cfg->Clkdiv << 8) |
                       ((uint32_t)Cfg->RadWake << 11) |
                       ((uint32_t)(Cfg->WakeCnt & 0x7FFFFU) << 12)) ;
#else
        WDT_REG_CTL = (((uint32_t)Cfg->wdcs << 1) |
                       ((uint32_t)Cfg->Window << 5) |
                       ((uint32_t)Cfg->Int << 7));
#endif
    }
    if (fnRegState_Detect((uint32_t)(&WDT->EN), 8, 0, WDT_TIMEOUT) == ERN_SUCCESS)
    {
        WDT_REG_HALT = (uint32_t)(Cfg->Halt);
    }
    if (fnRegState_Detect((uint32_t)(&WDT->EN), 8, 0, WDT_TIMEOUT) == ERN_SUCCESS)
    {
        WDT_REG_STBY = (uint32_t)Cfg->Stby; /*当 CPU 处于 sleep 或者 deepSleep 时（睡眠时使用）*/
    }

    WDT_REG_PASS = 0x70;
    WDT_REG_PASS = 0x00;
    return;
}

/*********************************************************************************************************
** Function name:
** Descriptions:     看门狗配置默认芯片寄存器
** input parameters:
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_WDT_RloadDefCfg(void)
{
    sWDT_Cfg_TypeDef Cfg;
    LL_WDT_CfgDef(&Cfg);
    LL_WDT_Cfg((const sWDT_Cfg_TypeDef *)&Cfg);
}

/*********************************************************************************************************
** Function name:
** Descriptions:     喂狗函数
** input parameters:
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_WDT_ReloadCounter(void)
{
    WDT->EN = 0xbb; /* restart watch dog timer */
}
#endif /*LL_WDT_MODULE_ENABLED*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
