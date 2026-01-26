/******************************************************************************
 * @file    rn8xxx_tc.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-09-07
 * @brief   Header file containing functions prototypes of Timer HAL library.
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
 * @ Revised records
 * V1.1.1   2023-09-07  Add       Add macro definition for geting register and register bit
 * V1.1.0               Initial
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TC_H_
#define TC_H_

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll_def.h"
/* Private define ------------------------------------------------------------*/

/*********************************************************************************/
#define GET_REG_TC_CNT(tc, databits) ((tc)->CNT & GET_DATA_MASK(databits))
#define GET_REG_TC_PS(tc, databits) ((tc)->PS & GET_DATA_MASK(databits))
#define GET_REG_TC_DN(tc, databits) ((tc)->DN & GET_DATA_MASK(databits))
#define GET_REG_TC_CCD0(tc, databits) ((tc)->CCD0 & GET_DATA_MASK(databits))
#define GET_REG_TC_CCD1(tc, databits) ((tc)->CCD1 & GET_DATA_MASK(databits))
#define GET_REG_TC_CCFG(tc, databits) ((tc)->CCFG & GET_DATA_MASK(databits))
#define GET_REG_TC_CTRL(tc, databits) ((tc)->CTRL & GET_DATA_MASK(databits))
#define GET_REG_TC_CM0(tc, databits) ((tc)->CM0 & GET_DATA_MASK(databits))
#define GET_REG_TC_CM1(tc, databits) ((tc)->CM1 & GET_DATA_MASK(databits))
#define GET_REG_TC_IE(tc, databits) ((tc)->IE & GET_DATA_MASK(databits))
#define GET_REG_TC_STA(tc, databits) ((tc)->STA & GET_DATA_MASK(databits))

#define SET_REG_TC_CNT(tc, data) ((tc)->CNT = (data))
#define SET_REG_TC_PS(tc, data) ((tc)->PS = (data))
#define SET_REG_TC_DN(tc, data) ((tc)->DN = (data))
#define SET_REG_TC_CCD0(tc, data) ((tc)->CCD0 = (data))
#define SET_REG_TC_CCD1(tc, data) ((tc)->CCD1 = (data))
#define SET_REG_TC_CCFG(tc, data) ((tc)->CCFG = (data))
#define SET_REG_TC_CTRL(tc, data) ((tc)->CTRL = (data))
#define SET_REG_TC_CM0(tc, data) ((tc)->CM0 = (data))
#define SET_REG_TC_CM1(tc, data) ((tc)->CM1 = (data))
#define SET_REG_TC_IE(tc, data) ((tc)->IE = (data))
#define SET_REG_TC_STA(tc, data) ((tc)->STA = (data))

/*********************************************************************************/

#define GET_BIT_TC_CNT(tc, bit) (((tc)->CNT & (bit)) ? (1) : (0))
#define GET_BIT_TC_PS(tc, bit) (((tc)->PS & (bit)) ? (1) : (0))
#define GET_BIT_TC_DN(tc, bit) (((tc)->DN & (bit)) ? (1) : (0))
#define GET_BIT_TC_CCD0(tc, bit) (((tc)->CCD0 & (bit)) ? (1) : (0))
#define GET_BIT_TC_CCD1(tc, bit) (((tc)->CCD1 & (bit)) ? (1) : (0))
#define GET_BIT_TC_CCFG(tc, bit) (((tc)->CCFG & (bit)) ? (1) : (0))
#define GET_BIT_TC_CTRL(tc, bit) (((tc)->CTRL & (bit)) ? (1) : (0))
#define GET_BIT_TC_CM0(tc, bit) (((tc)->CM0 & (bit)) ? (1) : (0))
#define GET_BIT_TC_CM1(tc, bit) (((tc)->CM1 & (bit)) ? (1) : (0))
#define GET_BIT_TC_IE(tc, bit) (((tc)->IE & (bit)) ? (1) : (0))
#define GET_BIT_TC_STA(tc, bit) (((tc)->STA & (bit)) ? (1) : (0))

#define SET_BIT_TC_CNT(tc, bit) ((tc)->CNT |= (bit))
#define SET_BIT_TC_PS(tc, bit) ((tc)->PS |= (bit))
#define SET_BIT_TC_DN(tc, bit) ((tc)->DN |= (bit))
#define SET_BIT_TC_CCD0(tc, bit) ((tc)->CCD0 |= (bit))
#define SET_BIT_TC_CCD1(tc, bit) ((tc)->CCD0 |= (bit))
#define SET_BIT_TC_CCFG(tc, bit) ((tc)->CCFG |= (bit))
#define SET_BIT_TC_CTRL(tc, bit) ((tc)->CTRL |= (bit))
#define SET_BIT_TC_CM0(tc, bit) ((tc)->CM0 |= (bit))
#define SET_BIT_TC_CM1(tc, bit) ((tc)->CM1 |= (bit))
#define SET_BIT_TC_IE(tc, bit) ((tc)->IE |= (bit))
#define SET_BIT_TC_STA(tc, bit) ((tc)->STA = (bit))

typedef enum
{
    TC_GPIO_P10 = 0,
    TC_GPIO_P11 = 1,
    TC_GPIO_P12 = 2,
    TC_GPIO_P13 = 3,
    TC_GPIO_P14 = 4,
    TC_GPIO_P15 = 5,
    TC_GPIO_P16 = 6,
    TC_GPIO_P17 = 7,

    TC_CHANNLE_0 = 0, /*通道0*/
    TC_CHANNLE_1 = 1, /*ͨ通道1*/

    TC_Low_Level = 0,  /*低电平*/
    TC_High_Level = 1, /*高电平*/

    TC_OM_MODE_Pwm1 = 6, /*pwm模式 1*/
    TC_OM_MODE_Pwm2 = 7, /*pwm模式2*/

    TC_CM_CPOL_RAISE =  0,  /*捕获上升沿*/
    TC_CM_CPOL_FALL =  1,   /*捕获下降沿*/
    TC_CM_CPOL_DULE =  2,   /*捕获双边沿*/

    TC_CCM_Capture = 0, /*捕获模式*/
    TC_CCM_Compare = 1, /*比较模式*/

    TC_DIEN_Disable = 0, /*死区插入不使能*/
    TC_DIEN_Enable = 1, /*死区插入不使能*/

    TC_Disable = 0, /*关闭定时器*/
    TC_Enable = 1,  /*打开定时器*/

    TC_Stop = 0,  /*停止定时器*/
    TC_Start = 1, /*启动定时器*/

#if !defined(RN831x_RN861x_MCU_V4)
    TC_CS_UART0_RXD = 0,   /*uart0 rxd*/
    TC_CS_UART1_RXD = 1,   /*uart1 rxd*/
    TC_CS_UART2_RXD = 2,   /*uart2 rxd*/
    TC_CS_UART3_RXD = 3,   /*uart3 rxd*/
    TC_CS_TC_OUTN0 = 4,    /*TC1/TC0 outN[0]*/
    TC_CS_TC_OUTP0 = 5,    /*TC1/TC0 outP[0]*/
    TC_CS_TC_OUTN1 = 6,    /*TC1/TC0 outN[1]*/
    TC_CS_TC_OUTP1 = 7,    /*TC1/TC0 outP[1]*/
    TC_CS_UART4_RXD = 8,   /*uart4 rxd*/
    TC_CS_UART5_RXD = 9,   /*uart5 rxd*/
    TC_CS_7816_0_P07 = 10, /*7816_0 P07*/
    TC_CS_7816_1_P10 = 11, /*7816_1 P10*/
    TC_CS_7816_1_P11 = 12, /*7816_1 P10*/
    TC_CS_reserve = 13,    /*保留*/
    TC_CS_CF4_OUT = 14,    /*CF4_OUT*/
    TC_CS_CF3_OUT = 15,    /*CF3_OUT*/
    TC_CS_CF2_OUT = 16,    /*CF2_OUT*/
    TC_CS_CF1_OUT = 17,    /*CF1_OUT*/
    TC_CS_CF0_OUT = 18,    /*CF0_OUT*/
    TC_CS_RTC_OUT = 19,    /*RTC_OUT*/
    TC_CS_P0_1 = 20,       /* TCI*/
    TC_CS_P0_3 = 21,
    TC_CS_P1_2 = 22,
    TC_CS_P1_3 = 23,
    TC_CS_P1_4 = 24,
    TC_CS_P1_5 = 25,
    TC_CS_P2_4 = 26,
    TC_CS_P2_5 = 27,
    TC_CS_P2_6 = 28,
    TC_CS_P2_7 = 29,
    TC_CS_P3_0 = 30,
    TC_CS_P3_6 = 31,
#endif
} eTC_TypeDef;

/*TC_CM0 TC_CM1*/
typedef union
{
    struct
    {
        uint32_t CCFG_CM : 1;    /**/
        uint32_t CTRL_Start : 1; /**/
        uint32_t CTRL_SLVEN : 1; /**/
        uint32_t CM_DFTLVL : 1;  /*比较输出缺省电平*/
        uint32_t CM_EFELVL : 1;  /*比较输出有效电平*/
        uint32_t CM_OM : 3;      /*比较输出模式：*/
        uint32_t CM_CCM : 1;     /*捕获比较模式选择：0,捕获；1,比较*/
        uint32_t CM_Enalbe : 1;  /**/
        uint32_t CM_DIEN : 1;      /*插入死区使能*/
        uint32_t CM_DL : 6;      /*比较输出死区长度*/
#if !defined(RN831x_RN861x_MCU_V4)
        uint32_t CM_CS : 5;      /*捕获外部输入事件选择*/
#endif
        uint32_t CM_CPOL: 2;     /*输入捕获外部事件选择*/
        uint32_t IE_SLVIE : 1;   /*从模式事件标志*/
        uint32_t IE_CC1IE : 1;   /**/
        uint32_t IE_CC0IE : 1;   /**/
        uint32_t IE_OVIE : 1;    /**/
        uint32_t channele : 1;   /*通道*/
        uint32_t Reserve : 5;    /**/
    }bitTimerCfg;
    uint32_t TimerCfg;
}uTimerCfg_TypeDef;

/* Exported define ------------------------------------------------------------*/
#define TIMER0 0
#define TIMER1 1

/* Exported enum ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
/*方波输出函数*/
extern void LL_TC_SquareWave_Init(TC_TypeDef *Timer);
extern void LL_TC_SquareWave_SetConfig(TC_TypeDef *Timer, uint8_t ch, uint32_t SquareWaveClock, FunctionalState IrqEn);
extern void LL_TC_SquareWave_Start(TC_TypeDef *Timer);
extern void LL_TC_SquareWave_Stop(TC_TypeDef *Timer);
/*定时器输出*/
extern void LL_TC_Time_Init(TC_TypeDef *Timer);
extern void LL_TC_Time_SetConfig(TC_TypeDef *Timer, uint16_t tUnit_ms, FunctionalState sigleMode, FunctionalState IrqEn);
extern void LL_TC_Time_Start(TC_TypeDef *Timer);
extern void LL_TC_Time_Stop(TC_TypeDef *Timer);
/*US延时*/
extern void LL_TC_TimeUs_Init(TC_TypeDef *Timer);
extern void LL_TC_TimeUs_SetConfig(TC_TypeDef *Timer, uint16_t tUnit_us, FunctionalState sigleMode, FunctionalState IrqEn);
extern void LL_TC_TimeUs_Start(TC_TypeDef *Timer);
extern void LL_TC_TimeUs_Stop(TC_TypeDef *Timer);
/*PWM输出*/
extern void LL_TC_PwmOut_Init(TC_TypeDef *Timer);
extern void LL_TC_PwmOut_SetConfig(TC_TypeDef *Timer, uint32_t pwm_freq, uint32_t duty, uTimerCfg_TypeDef timer_cfg);
extern void LL_TC_PwmOut_Start(TC_TypeDef *Timer, uTimerCfg_TypeDef timer_cfg);
extern void LL_TC_PwmOut_Stop(TC_TypeDef *Timer, uTimerCfg_TypeDef timer_cfg);
/*脉宽测量模式*/
extern void LL_TC_PusleMes_Init(TC_TypeDef *Timer);
#if !defined(RN831x_RN861x_MCU_V4)
extern void LL_TC_PusleMes_SetConfig(TC_TypeDef *Timer, uint32_t timeLenth, uTimerCfg_TypeDef timer_cfg);
#endif
extern void LL_TC_PusleMes_Start(TC_TypeDef *Timer, uTimerCfg_TypeDef timer_cfg);
extern void LL_TC_PusleMes_Stop(TC_TypeDef *Timer, uTimerCfg_TypeDef timer_cfg);
extern uint32_t fnTimeMeasue(const TC_TypeDef *Timer);

extern void LL_TC_Disable(TC_TypeDef *Timer);

#endif

/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
