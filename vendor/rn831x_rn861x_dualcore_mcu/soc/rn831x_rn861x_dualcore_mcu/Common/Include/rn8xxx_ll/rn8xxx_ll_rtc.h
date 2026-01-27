/******************************************************************************
 * @file    rn8xxx_rtc.h
 * @author  Renergy Application Team
 * @version V1.1.2
 * @date    2023-09-08
 * @brief   Header file containing functions prototypes of RTC HAL library.
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
 * V1.1.1   2023-09-08  Add       Add function declare
 * V1.1.1   2023-09-07  Add       Add macro definition for geting register and register bit
 * V1.1.0               Initial
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef Rn8xxx_RTC_H_
#define Rn8xxx_RTC_H_

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll_def.h"
/* Exported define ------------------------------------------------------------*/
#define RTC_TIMEOUT 0x3FFFU

#define RTC_ALARM_MASK 0x80U
#define RTC_ALARM_ENABLE 0x80U
#define RTC_ALARM_DISABLE 0x00U

#define RtcCtl_EnableWrite       \
    {                            \
        RTC->CTRL |= 0x00000100U; \
    }
#define RtcCtl_DisableWrite      \
    {                            \
        RTC->CTRL &= 0xfffffeffU; \
    }

/*********************************************************************************/
#define GET_REG_RTC_CTRL(rtc, databits) ((rtc)->CTRL & GET_DATA_MASK(databits))
#define GET_REG_RTC_CNT1(rtc, databits) ((rtc)->CNT1 & GET_DATA_MASK(databits))
#define GET_REG_RTC_CNT2(rtc, databits) ((rtc)->CNT2 & GET_DATA_MASK(databits))
#define GET_REG_RTC_IE(rtc, databits) ((rtc)->IE & GET_DATA_MASK(databits))
#define GET_REG_RTC_IF(rtc, databits) ((rtc)->IF & GET_DATA_MASK(databits))
#define GET_REG_RTC_TEMP(rtc, databits) ((rtc)->TEMP & GET_DATA_MASK(databits))
#define GET_REG_RTC_LOSC_CFG1(rtc, databits) ((rtc)->LOSC_CFG1 & GET_DATA_MASK(databits))
#define GET_REG_RTC_IOEN(rtc, databits) ((rtc)->IOEN & GET_DATA_MASK(databits))
#define GET_REG_RTC_IOMODE(rtc, databits) ((rtc)->IOMODE & GET_DATA_MASK(databits))

#define SET_REG_RTC_CTRL(rtc, data)  ((rtc)->CTRL = (data))
#define SET_REG_RTC_CNT1(rtc, data)  ((rtc)->CNT1 = (data))
#define SET_REG_RTC_CNT2(rtc, data)  ((rtc)->CNT2 = (data))
#define SET_REG_RTC_IE(rtc, data) ((rtc)->IE = (data))
#define SET_REG_RTC_IF(rtc, data) ((rtc)->IF = (data))
#define SET_REG_RTC_TEMP(rtc, data) ((rtc)->TEMP = (data))
#define SET_REG_RTC_LOSC_CFG1(rtc, data) ((rtc)->LOSC_CFG1 = (data))
#define SET_REG_RTC_IOEN(rtc, data) ((rtc)->IOEN = (data))
#define SET_REG_RTC_IOMODE(rtc, data) ((rtc)->IOMODE = (data))

/*********************************************************************************/

#define GET_BIT_RTC_CTRL(rtc, bit) (((rtc)->CTRL & (bit)) ? (1U) : (0U))
#define GET_BIT_RTC_CNT1(rtc, bit) (((rtc)->CNT1 & (bit)) ? (1U) : (0U))
#define GET_BIT_RTC_CNT2(rtc, bit) (((rtc)->CNT2 & (bit)) ? (1U) : (0U))
#define GET_BIT_RTC_IE(rtc, bit) (((rtc)->IE & (bit)) ? (1U) : (0U))
#define GET_BIT_RTC_IF(rtc, bit) (((rtc)->IF & (bit)) ? (1U) : (0U))
#define GET_BIT_RTC_TEMP(rtc, bit) (((rtc)->TEMP & (bit)) ? (1U) : (0U))
#define GET_BIT_RTC_LOSC_CFG1(rtc, bit) (((rtc)->LOSC_CFG1 & (bit)) ? (1U) : (0U))
#define GET_BIT_RTC_IOEN(rtc, bit) (((rtc)->IOEN & (bit)) ? (1U) : (0U))
#define GET_BIT_RTC_IOMODE(rtc, bit) (((rtc)->IOMODE & (bit)) ? (1U) : (0U))

#define SET_BIT_RTC_CTRL(rtc, bit) ((rtc)->CTRL |= (bit))
#define SET_BIT_RTC_CNT1(rtc, bit) ((rtc)->CNT1 |= (bit))
#define SET_BIT_RTC_CNT2(rtc, bit) ((rtc)->CNT2 |= (bit))
#define SET_BIT_RTC_IE(rtc, bit) ((rtc)->IE |= (bit))
#define SET_BIT_RTC_IF(rtc, bit) ((rtc)->IF = (bit))
#define SET_BIT_RTC_LOSC_CFG1(rtc, bit) ((rtc)->LOSC_CFG1 |= (bit))
#define SET_BIT_RTC_IOEN(rtc, bit) ((rtc)->IOEN |= (bit))
#define SET_BIT_RTC_IOMODE(rtc, bit) ((rtc)->IOMODE |= (bit))

#define CLR_BIT_RTC_CTRL(rtc, bit) ((rtc)->CTRL &= (~(bit)))
#define CLR_BIT_RTC_CNT1(rtc, bit) ((rtc)->CNT1 &= (~(bit)))
#define CLR_BIT_RTC_CNT2(rtc, bit) ((rtc)->CNT2 &= (~(bit)))
#define CLR_BIT_RTC_IE(rtc, bit) ((rtc)->IE &= (~(bit)))
#define CLR_BIT_RTC_IF(rtc, bit) (SET_BIT_RTC_IF((rtc), (bit)))
#define CLR_BIT_RTC_LOSC_CFG1(rtc, bit) ((rtc)->LOSC_CFG1 &= (~(bit)))
#define CLR_BIT_RTC_IOEN(rtc, bit) ((rtc)->IOEN &= (~(bit)))
#define CLR_BIT_RTC_IOMODE(rtc, bit) ((rtc)->IOMODE &= (~(bit)))
/* Exported enum ------------------------------------------------------------*/
/**********************************************************
  api return
**********************************************************/
typedef enum
{
    RTC_CurveMode_2 = 2U, /*"二次曲线"*/
    RTC_CurveMode_4 = 4U, /*"四次曲线"*/
} eRTC_CurveMode;        /*"曲线模式"*/

typedef enum
{                      /*"注意
                        1\DOTA0均需要设置
                        2\ 四次曲线参数必须设置"*/
  CurveParaDef = 0U,    /*"按芯片SPL默认值恢复曲线参数"*/
  CurveParaSet = 1U,    /*"按传入参数设置曲线参数"*/
} eRTC_IsCurveParaDef; /*"曲线系数是否采用默认值"*/

typedef enum
{
    RTC_IE_EN = 0U, /*"禁止输出"*/

} eRTC_IE_Typedef; /*"RTC时钟源输出频率"*/

typedef enum
{
    RTC_FOUT_Disable = 0U,  /*"禁止输出"*/
    RTC_FOUT_1HZ = 1U,      /*"1Hz 输出"*/
    RTC_FOUT_1Div30HZ = 2U, /*"1/30Hz 输出"*/
    RTC_FOUT_32768HZ = 3U,  /*"32768Hz 输出"*/
    RTC_FOUT_16HZ = 4U,     /*"16Hz 输出"*/
    RTC_FOUT_8HZ = 5U,      /*"8Hz 输出"*/
    RTC_FOUT_4HZ = 6U,      /*"4Hz 输出"*/
} eRTC_FOUT_Typedef;      /*"RTC时钟源输出频率"*/

typedef enum
{
    RTC_TCP_2Sec = 0U,  /*"2S"*/
    RTC_TCP_10Sec = 1U, /*"10S"*/
    RTC_TCP_20Sec = 2U, /*"20S"*/
    RTC_TCP_30Sec = 3U, /*"30S"*/
    RTC_TCP_1Min = 4U,  /*"1 min"*/
    RTC_TCP_2Min = 5U,  /*"2 min"*/
    RTC_TCP_5Min = 6U,  /*"5 min"*/
    RTC_TCP_10Min = 7U, /*"10 min"*/
} eRTC_TCP_Typedef;    /*"温度补偿周期"*/

typedef enum
{
    RTC_TSE_Disable = 0U,   /*"禁止自动温补"*/
    RTC_TSE_AutoMode = 1U,  /*"启动自动温补。按照 TCP 的设置进行周期性温补"*/
    RTC_TSE_UserMode0 = 2U, /*"启动用户温补模式 0，
                                温度寄存器可更改，
                                由用户填入温度值，
                                用户每写一次温度寄存器启动一次温度补偿；"*/
    RTC_TSE_UserMode1 = 3U, /*"启动用户温补模式 1，
	                            温度寄存器不可更改，
	                            每次写温度寄存器就启动一次温度补偿操作，
	                            温度寄存器的值由MCU 测量得到"*/
} eRTC_TSE_Typedef;        /*"温度传感器温补模式"*/

typedef enum
{
    /*"最晚两次记录时
        没有区分缓存中存的时间先后，
        应由软件做判断。"*/
    RTC_KeyFaillingEdge_1 = 0U, /*"第1次下降沿"*/
    RTC_KeyRisingEdge_1 = 1U,       /*"第1次上升沿"*/
    RTC_KeyFaillingEdge_2 = 2U,     /*"第2次下降沿"*/
    RTC_KeyRisingEdge_2 = 3U,       /*"第2次上升沿"*/
} eRTCKeyTimeStampEdge_TypeDef;

typedef enum
{
    RTC_KeyTimeEarliest = 0U, /*"记录IOEN使能后最早两次按键事件"*/
    RTC_KeyTimeLatest = 1U,  /*"记录IOEN使能后最晚两次按键事件"*/
} eRTCKeyTime_TypeDef;

/* Exported types ------------------------------------------------------------*/

typedef struct
{
    uint16_t DATA0; /*"初始频率偏差"*/
    uint16_t XT1;   /*"晶振温度曲线顶点温度"*/
    uint16_t ALPHA; /*"晶振一次温度系数"*/
    uint16_t BETA;  /*"晶振二次温度系数"*/
    uint16_t GAMMA; /*"晶振三次温度系数"*/
    uint16_t ZETA;  /*"晶振四次温度系数"*/
} sRTCCurve4Para;
typedef struct
{
    uint16_t DATA0;  /*"初始频率偏差"*/
    uint16_t XT0;    /*"晶振温度曲线顶点温度"*/
    uint16_t ALPHAL; /*"晶振低温段二次温度系数"*/
    uint16_t ALPHAH; /*"晶振高温段二次温度系数"*/
} sRTCCurve2Para;

typedef union
{
    sRTCCurve2Para Curve2Para;
    sRTCCurve4Para Curve4Para;
} uRTCCurvePara;

/* Exported data ----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */\
extern ErrorStatus LL_RTC_Read(uint8_t *Dst);
extern ErrorStatus LL_RTC_Write(const uint8_t *Dst, uint8_t Repeat);
extern ErrorStatus LL_RTC_CtlCheck(eRTC_TSE_Typedef rtc_tse, eRTC_TCP_Typedef rtc_tcp, eRTC_FOUT_Typedef rtc_fout);
extern ErrorStatus LL_RTC_CtlInit(eRTC_TSE_Typedef rtc_tse, eRTC_TCP_Typedef rtc_tcp, eRTC_FOUT_Typedef rtc_fout);
extern ErrorStatus LL_RTC_AlarmInit(uint8_t SCA, uint8_t MNA, uint8_t HRA);
extern ErrorStatus LL_RTC_Cnt1Init(uint8_t Unit_S);
extern ErrorStatus LL_RTC_Cnt2Init(uint8_t Unit_mS);
#if defined(RN821x_RN721x_SOC_V2)|| defined(RN831x_RN861x_MCU_V4) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3) ||defined(RN202x_RN7326_SOC_V2)
extern ErrorStatus LL_RTC_CurveModeCheck(eRTC_CurveMode RtcCurveMode, uRTCCurvePara RTCCurvePara, eRTC_IsCurveParaDef IsCurveParaDef);
extern ErrorStatus LL_RTC_CurveModeInit(eRTC_CurveMode RtcCurveMode, uRTCCurvePara RTCCurvePara, eRTC_IsCurveParaDef IsCurveParaDef);
extern void LL_RTC_CurveParaToReg(float RTC_DATA0,
                                  float RTC_XT1,
                                  float RTC_ALPHA,
                                  float RTC_BETA,
                                  float RTC_GAMMA,
                                  float RTC_ZETA,
                                  sRTCCurve4Para *RTCCurve4Para);
#endif
extern int32_t LL_RTC_TempGet(void);
extern void LL_RTC_Mode0TempUpdate(void);
extern ErrorStatus LL_RTC_TPS_TemprMea(int32_t * sTmper);


#endif /*_Rn8xxx_RTC_H*/

/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
