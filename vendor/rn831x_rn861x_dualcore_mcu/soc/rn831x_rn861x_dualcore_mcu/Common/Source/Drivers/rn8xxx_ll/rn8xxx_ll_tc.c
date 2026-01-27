/*******************************************************************************
  * @file    Rn8xxx_TC.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-08-29
  * @brief   Timer drive
  *
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
  ******************************************************************************
  *@verbatim
  ==============================================================================
                     ##### timer peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]


    @endverbatim
  */

/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll.h"
#ifdef LL_TC_MODULE_ENABLED
/* Private define ------------------------------------------------------------*/

/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/
static uint32_t gTime[2]; // 时间测量用
/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/

/*********************************************************************************************************
** Function name:       LL_TC_SquareWave_Init
** Descriptions:        方波输出初始化
** input parameters:    Timer：定时器模块,只支持TC0/TC1
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_SquareWave_Init(TC_TypeDef *Timer)
{
    Timer->CCFG = 0;
    Timer->PS = 0;
}
/*********************************************************************************************************
** Function name:       LL_TC_SquareWave_SetConfig
** Descriptions:        方波输出初始化
** input parameters:    Timer：定时器模块,只支持TC0/TC1
                        SquareWaveClock:方波频率HZ
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_SquareWave_SetConfig(TC_TypeDef *Timer, uint8_t ch, uint32_t SquareWaveClock, FunctionalState IrqEn)
{
    Timer->DN = (SystemCoreClock / (SquareWaveClock * 2 * (Timer->PS + 1))) - 1;    //	/* " 根据设置频率设置分频数"*/
    if (ch == 0)
    {   // DFTLVL  EFELVL  OM  CCM  ENABLE
#if !defined(RN831x_RN861x_MCU_V4)
        Timer->CM0 = (0 << 29) | (1 << 28) | (3 << 25) | (1 << 1) | (1 << 0);
#else
        Timer->CM0 = (0 << TC_CM0_DFTLVL_POS) | (1 << TC_CM0_EFELVL_POS) | (3 << TC_CM0_OM_POS) | (1 << TC_CM0_CCM_POS) | (1 << TC_CM0_ENABLE_POS);
#endif

    }
    else
    {
#if !defined(RN831x_RN861x_MCU_V4)
        Timer->CM1 = (0 << 29) | (1 << 28) | (3 << 25) | (1 << 1) | (1 << 0);
#else
        Timer->CM1 = (0 << TC_CM1_DFTLVL_POS) | (1 << TC_CM1_EFELVL_POS) | (3 << TC_CM1_OM_POS) | (1 << TC_CM1_CCM_POS) | (1 << TC_CM0_ENABLE_POS);
#endif
    }
    if (IrqEn == ERN_ENABLE)
    {
        Timer->IE |= 0x01;
    }
}
/*********************************************************************************************************
** Function name:       LL_TC_SquareWave_Start
** Descriptions:        方波输出启动
** input parameters:    Timer：定时器模块,只支持TC0/TC1
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_SquareWave_Start(TC_TypeDef *Timer)
{
    Timer->CTRL |= BIT0;
}
/*********************************************************************************************************
** Function name:       LL_TC_SquareWave_Stop
** Descriptions:        方波输出停止
** input parameters:    Timer：定时器模块,只支持TC0/TC1
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_SquareWave_Stop(TC_TypeDef *Timer)
{
    Timer->CTRL &= ~(BIT0);
}

/*********************************************************************************************************
** Function name:       LL_TC_Time_Init
** Descriptions:        ms定时器初始化
** input parameters:    Timer：定时器模块,只支持TC0/TC1
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_Time_Init(TC_TypeDef *Timer)
{
    Timer->CCFG = 0;
    Timer->PS = 4;
    Timer->CTRL = 0x00;
}
/*********************************************************************************************************
** Function name:       LL_TC_Time_SetConfig
** Descriptions:        ms定时器配置
** input parameters:    Timer：定时器模块,只支持TC0/TC1
                        tUnit_ms:定时器单位ms
                        sigleMode:单次模式使能
                        IrqEn:定時器溢出中断使能
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_Time_SetConfig(TC_TypeDef *Timer, uint16_t tUnit_ms, FunctionalState sigleMode, FunctionalState IrqEn)
{
    Timer->DN = (uint32_t)((((uint64_t)SystemCoreClock * (uint64_t)tUnit_ms) / (((uint64_t)Timer->PS + 1) * 1000)) - 1); // 根据设置间隔时间设置分频数
    if (sigleMode == ERN_ENABLE)
    {
        Timer->CTRL |= 0x10;
    }
    if (IrqEn == ERN_ENABLE)
    {
        Timer->IE |= 0x01;
    }
}
/*********************************************************************************************************
** Function name:       LL_TC_Time_Start
** Descriptions:        ms定时器启动
** input parameters:    Timer：定时器模块,只支持TC0/TC1
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_Time_Start(TC_TypeDef *Timer)
{
    Timer->CTRL |= BIT0;
}
/*********************************************************************************************************
** Function name:       LL_TC_Time_Stop
** Descriptions:        ms定时器初始化
** input parameters:    Timer：定时器模块,只支持TC0/TC1
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_Time_Stop(TC_TypeDef *Timer)
{
    Timer->CTRL &= ~(BIT0);
}
/*********************************************************************************************************
** Function name:       LL_TC_TimeUs_Init
** Descriptions:        us定时器初始化
** input parameters:    Timer：定时器模块,只支持TC0/TC1
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_TimeUs_Init(TC_TypeDef *Timer)
{
    Timer->CCFG = 0;
    Timer->PS = 0;
    Timer->CTRL = 0x00;
}
/*********************************************************************************************************
** Function name:       fnLL_TC_TimeInit
** Descriptions:        us定时器初始化
** input parameters:    Timer：定时器模块,只支持TC0/TC1
                        tUnit_us:定时器单位us
                        IrqEn:定時器溢出中断使能
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_TimeUs_SetConfig(TC_TypeDef *Timer, uint16_t tUnit_us, FunctionalState sigleMode, FunctionalState IrqEn)
{
    Timer->DN = (uint32_t)((((uint64_t)SystemCoreClock * tUnit_us) / (((uint64_t)Timer->PS + 1) * 1000000)) - 1); // 根据设置间隔时间设置分频数
    if (sigleMode == ERN_ENABLE)
    {
        Timer->CTRL |= 0x10;
    }
    if (IrqEn == ERN_ENABLE)
    {
        Timer->IE |= 0x01;
    }
}
/*********************************************************************************************************
** Function name:       LL_TC_TimeUs_Start
** Descriptions:        us定时器启动
** input parameters:    Timer: 定时器模块,只支持TC0/TC1
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_TimeUs_Start(TC_TypeDef *Timer)
{
    Timer->CTRL |= BIT0;
}
/*********************************************************************************************************
** Function name:       LL_TC_TimeUs_Stop
** Descriptions:        us定时器停止
** input parameters:    Timer: 定时器模块,只支持TC0/TC1
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_TimeUs_Stop(TC_TypeDef *Timer)
{
    Timer->CTRL &= ~(BIT0);
}

/*********************************************************************************************************
** Function name:       LL_TC_PwmOut_Init
** Descriptions:        PWM输出初始化
** input parameters:    Timer: 定时器模块,只支持TC0/TC1
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_PwmOut_Init(TC_TypeDef *Timer)
{
    Timer->CCFG = 0;
    Timer->PS = 0;
}

/*********************************************************************************************************
** Function name:       LL_TC_PwmOut_SetConfig
** Descriptions:        PWM输出配置
** input parameters:    Timer:定时器模块,只支持TC0/TC1
                        pwm_freq: 频率
                        duty :浮点数，占空比,单位：%
                        timer_cfg：通道配置
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_PwmOut_SetConfig(TC_TypeDef *Timer, uint32_t pwm_freq, uint32_t duty, uTimerCfg_TypeDef timer_cfg)
{
    //    IRQn_Type IRQn;
    //timer_cfg.CM_DFTLVL = TC_Low_Level; /*比较输出缺省电平：低电平*/
    //timer_cfg.CM_EFELVL = TC_Low_Level; /*比较输出有效电平：低电平*/
    //timer_cfg.CM_OM = TC_OM_MODE_Pwm1;  /*pwm模式 1*/
    timer_cfg.bitTimerCfg.CM_CCM = (uint8_t)TC_CCM_Compare;  /*输出比较模式*/
    //timer_cfg.CM_Enalbe = TC_Disable;   /*通道使能*/
    //timer_cfg.CTRL_Start = TC_Enable;   /*定时器启动*/
    //SYSCTL_ENABLE_WRITE;

    Timer->DN = ((SystemCoreClock / pwm_freq) / (Timer->PS + 1U)) - 1U; //	/* " 根据设置频率设置分频数"*/

    Timer->CCFG &= ~(0x1); /*计数时钟源选择内部系统时钟*/
    if (timer_cfg.bitTimerCfg.channele == 1)
    {
        Timer->CCD1 = (uint32_t)((float)(Timer->DN) * (float)duty / 100.0);

#if !defined(RN831x_RN861x_MCU_V4)
        Timer->CM1 = (timer_cfg.bitTimerCfg.CM_DFTLVL << 29) | (timer_cfg.bitTimerCfg.CM_EFELVL << 28) | (timer_cfg.bitTimerCfg.CM_OM << 25) | (timer_cfg.bitTimerCfg.CM_DL << 3) | (timer_cfg.bitTimerCfg.CM_DIEN << 2) | (timer_cfg.bitTimerCfg.CM_CCM << 1) | (timer_cfg.bitTimerCfg.CM_Enalbe << 0);
#else
        Timer->CM1 = (timer_cfg.bitTimerCfg.CM_DFTLVL << TC_CM1_DFTLVL_POS) \
                    | (timer_cfg.bitTimerCfg.CM_EFELVL << TC_CM1_EFELVL_POS) \
                    | (timer_cfg.bitTimerCfg.CM_OM << TC_CM1_OM_POS) \
                    | (timer_cfg.bitTimerCfg.CM_DL << TC_CM1_DL_POS) \
                    | (timer_cfg.bitTimerCfg.CM_DIEN << TC_CM1_DIEN_POS) \
                    | (timer_cfg.bitTimerCfg.CM_CCM << TC_CM1_CCM_POS) \
                    | (timer_cfg.bitTimerCfg.CM_Enalbe << TC_CM1_ENABLE_POS);
#endif
    }
    else
    {
        Timer->CCD0 = (uint32_t)((float)(Timer->DN) * (float)duty / 100.0);
#if !defined(RN831x_RN861x_MCU_V4)
        Timer->CM0 = (timer_cfg.bitTimerCfg.CM_DFTLVL << 29) | (timer_cfg.bitTimerCfg.CM_EFELVL << 28) | (timer_cfg.bitTimerCfg.CM_OM << 25) | (timer_cfg.bitTimerCfg.CM_DL << 3) | (timer_cfg.bitTimerCfg.CM_DIEN << 2) | (timer_cfg.bitTimerCfg.CM_CCM << 1) | (timer_cfg.bitTimerCfg.CM_Enalbe << 0);
#else
        Timer->CM0 = (timer_cfg.bitTimerCfg.CM_DFTLVL << TC_CM0_DFTLVL_POS) \
                    | (timer_cfg.bitTimerCfg.CM_EFELVL << TC_CM0_EFELVL_POS) \
                    | (timer_cfg.bitTimerCfg.CM_OM << TC_CM0_OM_POS) \
                    | (timer_cfg.bitTimerCfg.CM_DL << TC_CM0_DL_POS) \
                    | (timer_cfg.bitTimerCfg.CM_DIEN << TC_CM0_DIEN_POS) \
                    | (timer_cfg.bitTimerCfg.CM_CCM << TC_CM0_CCM_POS) \
                    | (timer_cfg.bitTimerCfg.CM_Enalbe << TC_CM0_ENABLE_POS);
#endif
    }
    Timer->IE = (timer_cfg.bitTimerCfg.IE_SLVIE << 3) | (timer_cfg.bitTimerCfg.IE_CC1IE << 2) | (timer_cfg.bitTimerCfg.IE_CC0IE << 1) | (timer_cfg.bitTimerCfg.IE_OVIE << 0);
}
/*********************************************************************************************************
** Function name:       LL_TC_PwmOut_Start
** Descriptions:        PWM输出
** input parameters:    Timer:定时器模块,只支持TC0/TC1
                        timer_cfg：通道配置
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_PwmOut_Start(TC_TypeDef *Timer, uTimerCfg_TypeDef timer_cfg)
{
    if (timer_cfg.bitTimerCfg.channele == 1)
    {
        Timer->CM1 |= (uint32_t)TC_Enable;
    }
    else
    {
        Timer->CM0 |= (uint32_t)TC_Enable;
    }
    Timer->CTRL |= (uint32_t)TC_Start;
}

/*********************************************************************************************************
** Function name:       LL_TC_PwmOut_Stop
** Descriptions:        PWM功能停止
** input parameters:    Timer: 定时器模块,只支持TC0/TC1
                        timer_cfg：通道配置
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_PwmOut_Stop(TC_TypeDef *Timer, uTimerCfg_TypeDef timer_cfg)
{
    if (timer_cfg.bitTimerCfg.channele == 1)
    {
        Timer->CM1 &= ~((uint32_t)TC_Enable);
    }
    else
    {
        Timer->CM0 &= ~((uint32_t)TC_Enable);
    }
    Timer->CTRL &= ~(uint32_t)TC_Start;
}

/*********************************************************************************************************
** Function name:       LL_TC_PusleMes_Init
** Descriptions:        脉宽测量（捕获输入）
** input parameters:
                        Timer: 定时器模块,只支持TC0/TC1
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_PusleMes_Init(TC_TypeDef *Timer)
{
    Timer->CCFG = 0;
    Timer->PS = 0;
}

/*********************************************************************************************************
** Function name:       LL_TC_PusleMes_SetConfig
** Descriptions:        脉宽测量（捕获输入）
** input parameters:    Timer: 定时器模块,只支持TC0/TC1
                        timeLenth: 测量周期（最大测量长度，单位：ms）
                        timer_cfg :相关配置
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
#if !defined(RN831x_RN861x_MCU_V4)
void LL_TC_PusleMes_SetConfig(TC_TypeDef *Timer, uint32_t timeLenth, uTimerCfg_TypeDef timer_cfg)
{
    //timer_cfg.CM_DFTLVL = TC_Low_Level; /*比较输出缺省电平：低电平*/
    //timer_cfg.CM_EFELVL = TC_Low_Level; /*比较输出有效电平：低电平*/
    //timer_cfg.CM_OM = TC_OM_MODE_Pwm1;  /*pwm模式 1*/
    timer_cfg.bitTimerCfg.CM_CCM = (uint8_t)TC_CCM_Capture;  /*输入捕获模式*/
    // timer_cfg.CM_Enalbe = TC_Enable;/*通道使能*/

    Timer->DN = (uint32_t)(((float)SystemCoreClock * ((float)timeLenth / 1000.0)) / ((float)Timer->PS + 1.0)) - 1; //	/* " 根据设置频率设置分频数"*/

    Timer->CCFG &= ~(0x1); /*计数时钟源选择内部系统时钟*/
    if (timer_cfg.bitTimerCfg.channele == 1)
    {
        Timer->CM1 = (timer_cfg.bitTimerCfg.CM_CPOL << 9) | (timer_cfg.bitTimerCfg.CM_CS << 20) | (timer_cfg.bitTimerCfg.CM_CCM << 1);
    }
    else
    {
        Timer->CM0 = (timer_cfg.bitTimerCfg.CM_CPOL << 9) | (timer_cfg.bitTimerCfg.CM_CS << 20) | (timer_cfg.bitTimerCfg.CM_CCM << 1);
    }
    Timer->IE = (timer_cfg.bitTimerCfg.IE_SLVIE << 3) | (timer_cfg.bitTimerCfg.IE_CC1IE << 2) | (timer_cfg.bitTimerCfg.IE_CC0IE << 1) | (timer_cfg.bitTimerCfg.IE_OVIE << 0);
    Timer->CTRL = (timer_cfg.bitTimerCfg.CTRL_SLVEN << 2);
}
#endif

/*********************************************************************************************************
** Function name:       LL_TC_PusleMes_Start
** Descriptions:        脉宽测量启动
** input parameters:    Timer：定时器模块,只支持TC0/TC1
                        timer_cfg: 对应的通道
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_PusleMes_Start(TC_TypeDef *Timer, uTimerCfg_TypeDef timer_cfg)
{
    timer_cfg.bitTimerCfg.CTRL_Start = (uint8_t)TC_Start; /*定时器启动*/
    timer_cfg.bitTimerCfg.CM_Enalbe = (uint8_t)TC_Enable;
    if (timer_cfg.bitTimerCfg.channele == 1)
    {
        Timer->CM1 |= (timer_cfg.bitTimerCfg.CM_Enalbe << 0);
    }
    else
    {
        Timer->CM0 |= (timer_cfg.bitTimerCfg.CM_Enalbe << 0);
    }
    Timer->CTRL |= timer_cfg.bitTimerCfg.CTRL_Start;
}
/*********************************************************************************************************
** Function name:       LL_TC_PusleMes_Stop
** Descriptions:        脉宽测量停止
** input parameters:    Timer：定时器模块,只支持TC0/TC1
                        timer_cfg: 对应的通道
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_PusleMes_Stop(TC_TypeDef *Timer, uTimerCfg_TypeDef timer_cfg)
{
    timer_cfg.bitTimerCfg.CTRL_Start = (uint8_t)TC_Start; /*定时器启动*/
    if (timer_cfg.bitTimerCfg.channele == 1)
    {
        Timer->CM1 &= ~(timer_cfg.bitTimerCfg.CM_Enalbe << 0);
    }
    else
    {
        Timer->CM0 &= ~(timer_cfg.bitTimerCfg.CM_Enalbe << 0);
    }
}
/*********************************************************************************************************
** Function name:       fnTimeMeasue
** Descriptions:        脉宽测量函数时间读取
** input parameters:    Timer:定时器模块,只支持TC0/TC1
** output parameters:   返回值，脉宽时间（ms)
** Returned value:      None
*********************************************************************************************************/
uint32_t fnTimeMeasue(const TC_TypeDef *Timer)
{
    // gTime[1] /TC0->DN * timeLen ,即为时间宽度
    if (Timer->CCD0 >= gTime[0])
        gTime[1] = Timer->CCD0 - gTime[0];
    else
    {
        //gTime[1] = Timer->CCD0 + Timer->DN - gTime[0];
        gTime[1] = Timer->CCD0;
        gTime[1] += Timer->DN;
        gTime[1] -= gTime[0];
        return (gTime[1]);
    }
    gTime[0] = Timer->CCD0;
    return (0);
}

/*********************************************************************************************************
** Function name:       LL_TC_Disable
** Descriptions:        关闭定时器模块
** input parameters:    Timer：定时器模块
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_TC_Disable(TC_TypeDef *Timer)
{
    SET_REG_TC_CTRL(Timer,0);
}
#endif /*LL_TC_MODULE_ENABLED*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
