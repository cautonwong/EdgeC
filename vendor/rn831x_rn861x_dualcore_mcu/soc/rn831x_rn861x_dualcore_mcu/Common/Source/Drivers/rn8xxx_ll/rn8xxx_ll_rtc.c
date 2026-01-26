/********************************************************************************
  * @file    Rn8xxx_RTC.c
  * @author  Renergy Application Team
  * @version V1.1.1
  * @date    2025-01-14
  * @brief   RTC模块驱动
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
                     ##### RTC peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [修订记录]
    V1.1.1 25.1.14
    1、将从spl中获取RTC 二次曲线补偿值，由固定地址，修改为了通过库函数获取。
    2、针对RN861x v3版本，使用固化到flash的库函数，做了兼容性处理。

    @endverbatim
  */
/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll.h"
#include "rn8xxx_ll_rtc_lib.h"

#ifdef LL_RTC_MODULE_ENABLED
// extern ErrorStatus Std_Memcmp(const void* cmp1, const void* cmp2, uint16_t len);


/* Private define ------------------------------------------------------------*/
/*"寄存器地址"*/
//#define RTC_REG_TEMP2 (*(uint32_t *)(0x4003C0F8U))     /*"BIT0-BIT11"*/

/*"SPL地址"*/
#define RTC_RTCPS_EN (0x8eU) //RTC_PS 写密码

#if defined(RN831x_RN861x_MCU_V4)
#define RTC_SAR_CLK_EN                                      \
    {                                                       \
        BITBAND_ADDR((uint32_t)(&SYSTEM->MOD1_CKE), 4U) = 1U; \
    }
#define GPIO_CLK_EN                                      \
    {                                                       \
        BITBAND_ADDR((uint32_t)(&SYSTEM->PMOD0_CKE), 11U) = 1U; \
    }
#endif
/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/

/*********************************************************************************************************
** Function name:     	rtc_Delay
** Descriptions:       	延时函数（用于RTC驱动）
** input parameters:    None
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
static void rtc_Delay(void)
{
    uint16_t i;
    for (i = 400U; i > 0U; i--)
    {
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
    }
}

/*********************************************************************************************************
** Function name:     	LL_RTC_CtlCheck
** Descriptions:       	RTC控制寄存器校验
** input parameters:    rtc_tse  温补模式：
                            00,禁止自动温补；
                            01 启动自动温补，按照TCP的设置进行周期性温补；
                            02 温补模式0（温度寄存器可更改， 有用户填入温度值，用户每写一次温度寄存器启动一次温度补偿）
                            03 温补模式1，温度寄存器值不可更改，每次写温度寄存器就启动一次温度补偿，温度寄存器的值由soc测量得到
                        rtc_tcp: 温补周期
                            0 2s; 1 10s默认；2 20s ；
                            3 30s;  4 1分钟；   5 2分钟；
                            6 5分钟；   7 10分钟
                        Fout 输出 （该寄存器仅在上下电起作用）
                            0 禁止输出； 1 1hz输出（建议使用）
                            2 1/30hz输出；  3 32768hz输出
                            4 16hz输出；    5 8hz输出
                            6 4hz输出；     7 1hz输出
** output parameters:   None
** Returned value:     SUCCESS成功，ERROR失败
*********************************************************************************************************/
ErrorStatus LL_RTC_CtlCheck(eRTC_TSE_Typedef rtc_tse, eRTC_TCP_Typedef rtc_tcp, eRTC_FOUT_Typedef rtc_fout)
{
    uint32_t CheckCtrl;

    CheckCtrl = (((uint32_t)rtc_fout << 0U) | /*"RTC时钟源输出频率"*/
                 ((uint32_t)rtc_tcp << 3U) |  /*"温度补偿周期"*/
                 ((uint32_t)rtc_tse << 6U));  /*"温度传感器温补模式"*/

    if (((RTC->CTRL) & 0xffU) == CheckCtrl)
    {
        return (ERN_SUCCESS);
    }
    else
    {
        return (ERN_ERROR);
    }
}

/*********************************************************************************************************
** Function name:     	LL_RTC_CtlInit
** Descriptions:       	RTC控制寄存器配置函数
** input parameters:    rtc_tse:温度传感器温补模式
**                      rtc_tcp:温度补偿周期
**                      rtc_fout:RTC时钟源输出频率
** output parameters:   None
** Returned value:     SUCCESS成功，ERROR失败
*********************************************************************************************************/
ErrorStatus LL_RTC_CtlInit(eRTC_TSE_Typedef rtc_tse,
                           eRTC_TCP_Typedef rtc_tcp,
                           eRTC_FOUT_Typedef rtc_fout)
{
    SYSCTL_ENABLE_WRITE;
    RTC_SAR_CLK_EN; /*" 打开RTC/SAR时钟，建议此时钟一直打开"*/

    if (rtc_fout != RTC_FOUT_Disable)
    {
        GPIO_CLK_EN; /*" 打开GPIO APB时钟，建议此时钟一直打开"*/
    }
    RtcCtl_EnableWrite;

    if (fnRegState_Detect((uint32_t)(&RTC->CTRL), 9, 0, RTC_TIMEOUT) == ERN_ERROR) /*"Wr_busy等待"*/
    {
        return (ERN_ERROR);
    }

    RTC->CTRL = (((uint32_t)rtc_fout << 0U) | /*"RTC时钟源输出频率"*/
                 ((uint32_t)rtc_tcp << 3U) |  /*"温度补偿周期"*/
                 ((uint32_t)rtc_tse << 6U) |  /*"温度传感器温补模式"*/
                 (1U << 8U));        /*"禁止 RTC 寄存器写操作 "*/

    rtc_Delay();
    SYSCTL_DISABLE_WRITE;
    return (LL_RTC_CtlCheck(rtc_tse, rtc_tcp, rtc_fout));

}
/*********************************************************************************************************
** Function name:     	LL_RTC_AlarmInit
** Descriptions:       	闹钟（小时/分钟/秒）
** input parameters:    None
** output parameters:   None
** Returned value:     SUCCESS成功，ERROR失败
*********************************************************************************************************/
ErrorStatus LL_RTC_AlarmInit(uint8_t SCA, uint8_t MNA, uint8_t HRA)
{
    SYSCTL_ENABLE_WRITE;

    RTC_SAR_CLK_EN; /*" 打开RTC/SAR时钟，建议此时钟一直打开"*/

    RtcCtl_EnableWrite;
    RTC->SCA = SCA;
    RTC->MNA = MNA;
    RTC->HRA = HRA;

    RTC->IF = (0x08U);
    RTC->IE |= (0x08U | 0x200U);/*bit9:秒中断来源,0,选择RTC的秒脉冲；1，选择系统时钟模式*/
                                /*bit3：秒中断使能。0，不使能；1，使能。*/

    rtc_Delay();
    SYSCTL_DISABLE_WRITE;
    return (ERN_SUCCESS);
}

/*********************************************************************************************************
** Function name:       LL_RTC_Cnt1Init
** Descriptions:        RTC定时器1初始化
** input parameters:    Unit_S:定时器时间，单位S
** output parameters:   None
** Returned value:      SUCCESS 成功  ERROR 数据错误失败
*********************************************************************************************************/
ErrorStatus LL_RTC_Cnt1Init(uint8_t Unit_S)
{
    if (Unit_S == 0U)
    {
        return ERN_ERROR;
    }
    RtcCtl_EnableWrite; /* RTC Write enable*/
    RTC->CNT1 = Unit_S - 1;
    RTC->IF = (0x02U);
    RTC->IE |= (0x02U);
    RtcCtl_DisableWrite;
    return ERN_SUCCESS;
}
/*********************************************************************************************************
** Function name:       LL_RTC_Cnt2Init
** Descriptions:        RTC定时器2初始化
** input parameters:    Unit_mS:定时器时间，单位ms,刻度3.9ms
** output parameters:   None
** Returned value:      SUCCESS 成功  ERN_ERROR 数据错误失败
*********************************************************************************************************/
ErrorStatus LL_RTC_Cnt2Init(uint8_t Unit_mS)
{
    if (Unit_mS == 0U)
    {
        return ERN_ERROR;
    }
    RtcCtl_EnableWrite; /* RTC Write enable*/
    RTC->CNT2 = Unit_mS - 1;
    RTC->IF = 0x04U;
    RTC->IE |= 0x04U;
    RtcCtl_DisableWrite;
    return ERN_SUCCESS;
}
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V4)|| defined(RN831x_RN861x_MCU_V2)||defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3) || defined(RN202x_RN7326_SOC_V2)
/*********************************************************************************************************
** Function name:		LL_RTC_CurveModeCheck
** Descriptions:		曲线参数校验
** input parameters:
** output parameters:
** Returned value:
*********************************************************************************************************/
ErrorStatus LL_RTC_CurveModeCheck(eRTC_CurveMode RtcCurveMode,
                                  uRTCCurvePara RTCCurvePara,
                                  eRTC_IsCurveParaDef IsCurveParaDef)
{
    ErrorStatus temp_err_status = ERN_SUCCESS;
    if (RTC_CurveMode_4 == RtcCurveMode) /*"四次曲线"*/
    {
        if(0xea65 != RTC->MODE1)
        {
          temp_err_status = ERN_ERROR;
        }
        if((RTCCurvePara.Curve4Para.DATA0&0x1FFF) != RTC->DOTA0)
        {
            temp_err_status = ERN_ERROR;
        }
        if((RTCCurvePara.Curve4Para.XT1&0x1FFF) != RTC->XT1)
        {
            temp_err_status = ERN_ERROR;
        }
        if(RTCCurvePara.Curve4Para.ALPHA != RTC->ALPHA)
        {
            temp_err_status = ERN_ERROR;
        }
        if((RTCCurvePara.Curve4Para.BETA&0x7FFF) != RTC->BETA)
        {
            temp_err_status = ERN_ERROR;
        }
        if(RTCCurvePara.Curve4Para.GAMMA != RTC->GAMMA)
        {
            temp_err_status = ERN_ERROR;
        }
        if(RTCCurvePara.Curve4Para.ZETA != RTC->ZETA)
        {
            temp_err_status = ERN_ERROR;
        }
    }
    else /*"二次曲线"*/
    {
        if (CurveParaDef == IsCurveParaDef)
        {
            uint16_t xt0,alphl,alphh;

#if defined(RN831x_RN861x_MCU_V4)
            if((SYSTEM->ADDR_MAP_CTRL&0xFU) == 0){
            	xt0       	= (*(uint16_t *)(0x20041E));
            	alphl     	= (*(uint16_t *)(0x200420));
            	alphh     	= (*(uint16_t *)(0x200422));
            }else{
                xt0         = (*(uint16_t *)(0x10041E));
                alphl       = (*(uint16_t *)(0x100420));
                alphh       = (*(uint16_t *)(0x100422));
            }
#else
            if(SYSOPTION_PASS != rn_lib->LL_SYSOPTION_AlphalCfgGet((uint16_t*)&xt0)) return ERN_ERROR;
            if(SYSOPTION_PASS != rn_lib->LL_SYSOPTION_AlphalCfgGet((uint16_t*)&alphl)) return ERN_ERROR;
            if(SYSOPTION_PASS != rn_lib->LL_SYSOPTION_AlphalCfgGet((uint16_t*)&alphh)) return ERN_ERROR;
#endif
            if(0x00 != RTC->MODE1)
            {
                temp_err_status = ERN_ERROR;
            }
            if(xt0 != RTC->XT0)
            {
                temp_err_status = ERN_ERROR;
            }
            if(alphl != RTC->ALPHAL)
            {
                temp_err_status = ERN_ERROR;
            }
            if(alphh != RTC->ALPHAH)
            {
                temp_err_status = ERN_ERROR;
            }
        }
        else
        {
              if(0x00 != RTC->MODE1)
              {
                temp_err_status = ERN_ERROR;
              }
              if((RTCCurvePara.Curve2Para.DATA0&0x1FFF) != RTC->DOTA0)
              {
                temp_err_status = ERN_ERROR;
              }
              if((RTCCurvePara.Curve2Para.XT0&0x007F) != RTC->XT0)
              {
                temp_err_status = ERN_ERROR;
              }
              if((RTCCurvePara.Curve2Para.ALPHAL&0x07FF) != RTC->ALPHAL)
              {
                temp_err_status = ERN_ERROR;
              }
              if((RTCCurvePara.Curve2Para.ALPHAH&0x07FF) != RTC->ALPHAH)
              {
                  temp_err_status = ERN_ERROR;
              }
        }
    }
    return temp_err_status;
}

/*********************************************************************************************************
** Function name:		LL_RTC_CurveModeInit
** Descriptions:		RTC温度补偿曲线初始化函数
** input parameters:	RtcCurveMode:曲线模式
**                      RTCCurvePara:曲线系数
**                      IsCurveParaDef:曲线系数是否采用默认值
** output parameters:   None
** Returned value:      SUCCESS 成功  ERROR 数据错误失败
*********************************************************************************************************/
ErrorStatus LL_RTC_CurveModeInit(eRTC_CurveMode RtcCurveMode,
                                 uRTCCurvePara RTCCurvePara,
                                 eRTC_IsCurveParaDef IsCurveParaDef)
{
    ErrorStatus temp_err_status = ERN_SUCCESS;
    SYSCTL_ENABLE_WRITE;
    RTC_SAR_CLK_EN;     /*" 打开RTC/SAR/GPIO APB时钟，建议此时钟一直打开"*/
    RtcCtl_EnableWrite; /*" RTC Write enable"*/
    RTC->PS = RTC_RTCPS_EN; /*"RTC_PS  当8'h8E时，补偿寄存器44H~60H才可写"*/

    if (RTC_CurveMode_4 == RtcCurveMode) /*"四次曲线"*/
    {
        RTC->MODE1 = 0xea65; /*"RTC_MODE1 当16'hEA65：启用四次曲线补偿"*/ //
        RTC->DOTA0 = RTCCurvePara.Curve4Para.DATA0;                       /*"晶振温度曲线顶点温度"*/
        RTC->XT1 = RTCCurvePara.Curve4Para.XT1;                           /*"晶振温度曲线顶点温度"*/
        RTC->ALPHA = RTCCurvePara.Curve4Para.ALPHA;                       /*"晶振一次温度系数"*/
        RTC->BETA = RTCCurvePara.Curve4Para.BETA;                         /*"晶振二次温度系数"*/
        RTC->GAMMA = RTCCurvePara.Curve4Para.GAMMA;                       /*"晶振三次温度系数"*/
        RTC->ZETA = RTCCurvePara.Curve4Para.ZETA;                         /*"晶振四次温度系数"*/
    }
    else /*"二次曲线"*/
    {
        RTC->MODE1 = 0; /*"RTC_MODE1 当0：启用二次曲线补偿"*/
                         /* RTC->DOTA0 = RTCCurvePara.Curve2Para.DATA0;*/
        if (CurveParaDef == IsCurveParaDef)
        { /*"从SPL中恢复,RA9701A"*/
            uint16_t xt0,alphl,alphh;

#if defined(RN831x_RN861x_MCU_V4)
            if((SYSTEM->ADDR_MAP_CTRL&0xFU) == 0){
            	xt0  		= (*(uint16_t *)(0x20041E));
            	alphl  		= (*(uint16_t *)(0x200420));
            	alphh    	= (*(uint16_t *)(0x200422));
            }else{
                xt0         = (*(uint16_t *)(0x10041E));
                alphl       = (*(uint16_t *)(0x100420));
                alphh       = (*(uint16_t *)(0x100422));
            }
#else
            if(SYSOPTION_PASS != rn_lib->LL_SYSOPTION_AlphalCfgGet((uint16_t*)&xt0)) return ERN_ERROR;
            if(SYSOPTION_PASS != rn_lib->LL_SYSOPTION_AlphalCfgGet((uint16_t*)&alphl)) return ERN_ERROR;
            if(SYSOPTION_PASS != rn_lib->LL_SYSOPTION_AlphalCfgGet((uint16_t*)&alphh)) return ERN_ERROR;
#endif
            RTC->XT0 = xt0 & 0xffff;      /*"RTC_XT0"*/
            RTC->ALPHAL = alphl & 0xffff; /*"RTC_ALPHAL"*/
            RTC->ALPHAH = alphh & 0xffff; /*"RTC_ALPHAH"*/
        }
        else
        {
            RTC->XT0 = RTCCurvePara.Curve2Para.XT0;       /*"RTC_XT0"*/                                                            /*"设置二次曲线参数"*/
            RTC->ALPHAL = RTCCurvePara.Curve2Para.ALPHAL; /*"RTC_ALPHAL"*/
            RTC->ALPHAH = RTCCurvePara.Curve2Para.ALPHAH; /*"RTC_ALPHAH"*/
        }
        if (RTC_FAIL == LL_RTC_WriteDota(RTCCurvePara.Curve2Para.DATA0))
        {
          temp_err_status = ERN_ERROR;
        }
    }
    RtcCtl_DisableWrite;
    SYSCTL_DISABLE_WRITE;
    temp_err_status = LL_RTC_CurveModeCheck(RtcCurveMode, RTCCurvePara, IsCurveParaDef);
    return (temp_err_status);
}

#define READTEMP_NUM 10
/*********************************************************************************************************
** Function name:
** Descriptions: 该函数在用户温补模式0下的应用
** input parameters:
** output parameters:
** Returned value:
*********************************************************************************************************/
void LL_RTC_Mode0TempUpdate(void)
{
  uint8_t i = 0U, j = 0U;
  int32_t temp32 = 0U; /*用于判符号位*/
  uint32_t u32Tmp = 0U;

  RTC->PS = RTC_RTCPS_EN; /*"RTC_PS "*/
  j = 0U;
  for (i = 0U; i < READTEMP_NUM; i++)
  {
    if (fnRegState_Detect((uint32_t)(&RTC->CTRL), 10U, 0U, RTC_TIMEOUT) == ERN_SUCCESS) /*检查 cal_busy ，确保下降沿不允许启动温补*/
    {
      RTC->TPS_START = 1;                                                                    /*启动SAR进行测温*/
      SystemDelayUs(488);                                                                    /*等待adc采样完成， 理论需要延时4个clk，即4*(1/32768 s）= 4*122us=488us，SystemDelayUs函数不准，按3倍余量*/
      if (fnRegState_Detect((uint32_t)(&MADC->AD_STAT), 1U, 0U, RTC_TIMEOUT) == ERN_SUCCESS) /*检查tps_busy*/
      {
        SystemDelayUs(610);                                                                 /* 延时3个clk,等待计算完成，即3*（1/32768) = 3*122us= 366us，实际按5*122 = 610us延时*/
        if (fnRegState_Detect((uint32_t)(&RTC->CTRL), 10U, 0U, RTC_TIMEOUT) == ERN_SUCCESS) /*检查cal_busy确保计算完成*/
        {
          /*读取温度值*/
          u32Tmp = RTC->TEMP_CAL;
          // SEGGER_RTT_printf(0, "temp16 = %d \r\n" , u32Tmp);
          if ((u32Tmp & 0x800U) == 0x800U)
          {
            temp32 += (int32_t)(u32Tmp | 0xfffff800U);
          }
          else
          {
            temp32 += (int32_t)(u32Tmp & 0x7ffU);
          }

          j++;
        }
      }
    }
  }

  if (j != 0)
  {
    temp32 = (int32_t)((float)temp32 * 16U / j);
    // SEGGER_RTT_printf(0, "temp32 = %d , %x ,%d\r\n" , temp32 ,temp32 ,(int32_t)(temp32/256));
    SystemDelayUs(130U); /*--等待至少61us（2*Tlosc_clk），再配置RTC_TEMP才能生效*/
    if (fnRegState_Detect((uint32_t)(&RTC->CTRL), 10U, 0U, RTC_TIMEOUT) == ERN_SUCCESS)
    {
      if (RTC->MODE1 == 0xea65)
        temp32 &= 0xffff;
      else
        temp32 = (temp32 >> 6) & 0x3ff;/*lint !e702*/
      RTC->TEMP = temp32; /*启动自动温补*//*lint !e732*/
    }
  }
}
#endif
/*********************************************************************************************************
** Function name:     	 fnRTCDrive_TempGet
** Descriptions:       	获取MCU内部温度
** input parameters:     void
** output parameters:
** Returned value: 	 Temperature_value  温度值单位  ℃量纲-3
*********************************************************************************************************/
int32_t LL_RTC_TempGet(void)
{
    int32_t Temperature_value;
    #if !defined(RN831x_RN861x_MCU_V1) &&  !defined(RN821x_RN721x_SOC_B) &&  !defined(RN821x_RN721x_SOC_C) && !defined(RN821x_RN721x_SOC_D) && !defined(RN202x_RN7326_SOC_B)
        if ((RTC->TEMP2 & BIT11) == BIT11)
        {
            Temperature_value = (int32_t)(RTC->TEMP2 & 0xFFFU) - 0x1000U;
        }
        else
        {
            Temperature_value = (int32_t)(RTC->TEMP2 & 0xFFFU);
        }
        Temperature_value = (int32_t)((float)(Temperature_value)*62.5); /*"62.5=1000/16"*/
    #else
        if (RTC->TEMP & BIT9)
        {
            Temperature_value = (int32_t)(RTC->TEMP & 0x3FFU) - 0x400U;
        }
        else
        {
            Temperature_value = (int32_t)(RTC->TEMP & 0x3FFU);
        }
        Temperature_value = (int32_t)((float)(Temperature_value)*250); /*"250=1000/4"*/
    #endif

    return (Temperature_value);
}

/*********************************************************************************************************
** Function name:     	LL_RTC_Read
** Descriptions:       	RTC读取数据
** input parameters:    Dst读取数据缓冲区目标指针
** output parameters:   None
** Returned value:     SUCCESS成功，ERROR失败
*********************************************************************************************************/
ErrorStatus LL_RTC_Read(uint8_t *Dst)
{
    uint8_t Buffer[7], i;
    for (i = 0; i < 3; i++)
    {
        /*读取RTC万年历时间*/
        /*当两次读取相同时才认为读取成功，防止跳S时时间读错*/
        Dst[0] = RTC->SC;
        Dst[1] = RTC->MN;
        Dst[2] = RTC->HR;
        Dst[4] = RTC->DT;
        Dst[5] = RTC->MO;
        Dst[6] = RTC->YR;
        Dst[3] = RTC->DW;

        Buffer[0] = RTC->SC;
        Buffer[1] = RTC->MN;
        Buffer[2] = RTC->HR;
        Buffer[4] = RTC->DT;
        Buffer[5] = RTC->MO;
        Buffer[6] = RTC->YR;
        Buffer[3] = RTC->DW;
        if (memcmp(&Buffer[0], &Dst[0], 6U) != 0)
        {
            continue;
        }
        else
        {
            return (ERN_SUCCESS);
        }
    }
    return (ERN_ERROR);
}

/*********************************************************************************************************
** Function name:     	LL_RTC_Write
** Descriptions:       	RTC写入数据
** input parameters:    Dst写数缓冲区源指针，Repeat失败重复次数
** output parameters:   None
** Returned value:     SUCCESS成功，ERROR失败
*********************************************************************************************************/
ErrorStatus LL_RTC_Write(const uint8_t *Dst, uint8_t Repeat)
{
    uint8_t Buffer[7], i;
    RtcCtl_EnableWrite; /*RTC Write enable*/

    for (; Repeat > 0U; Repeat--)
    {
        for (i = 0U; i < 3U; i++)
        {
            if (!(RTC->CTRL & 0x200))
                break; /*判断RTC是否处于忙标志*/
            rtc_Delay();
        }
        if ((RTC->CTRL & 0x200U) == 0x200U)
            continue;
        /*设置RTC万年历时间*/
        /*当两次读取相同时才认为读取成功，防止跳S时时间读错*/
        RTC->SC = Dst[0];
        RTC->MN = Dst[1];
        RTC->HR = Dst[2];
        RTC->YR = Dst[6];
        RTC->MO = Dst[5];
        RTC->DT = Dst[4];
        RTC->DW = Dst[3];
        rtc_Delay(); /*写入后，需延时350us后在读取寄存器，以确定是否写入成功*/
        if (LL_RTC_Read(Buffer) == ERN_ERROR)
        {
            continue;
        }
        if (memcmp(&Buffer[0], &Dst[0], 6U) != 0)
        {
            continue;
        }
        else
        {
            RtcCtl_DisableWrite;
            return (ERN_SUCCESS);
        }
    }
    RtcCtl_DisableWrite; // RTC Write Disable
    return (ERN_ERROR);
}

/*********************************************************************************************************
** Function name:     	LL_RTC_CurveParaToReg
** Descriptions:
** input parameters:    None
** output parameters:   None
** Returned value:     None
*********************************************************************************************************/
void LL_RTC_CurveParaToReg(float RTC_DATA0,
                           float RTC_XT1,
                           float RTC_ALPHA,
                           float RTC_BETA,
                           float RTC_GAMMA,
                           float RTC_ZETA,
                           sRTCCurve4Para *RTCCurve4Para)
{
    // int16_t i16_temp;
    RTCCurve4Para->DATA0 = (uint16_t)((int16_t)(RTC_DATA0 * 32U)) & 0x1FFFU;
    RTCCurve4Para->XT1 = (uint16_t)((int16_t)(RTC_XT1 * 256U)) & 0x1FFFU;
    RTCCurve4Para->ALPHA = (uint16_t)((int16_t)(RTC_ALPHA * 262144U)) & 0xFFFFU;            /*""*/
    RTCCurve4Para->BETA = (uint16_t)((int16_t)(RTC_BETA * 524288U)) & 0x7FFFU;              /*""*/
    RTCCurve4Para->GAMMA = (uint16_t)((int16_t)(65536U + RTC_GAMMA * 268435456U)) & 0xFFFFU; /*""*/
    if (RTC_ZETA >= 0)
    {
        RTCCurve4Para->ZETA = (uint16_t)(RTC_ZETA * 17179869184U) & 0xFFFFU; /*""*/
    }
    else
    {
        RTCCurve4Para->ZETA = (uint16_t)(65536 + RTC_ZETA * 17179869184U) & 0xFFFFU; /*""*/
    }
}

#if !defined(RN821x_RN721x_SOC_D) && !defined(RN821x_RN721x_SOC_C) && !defined(RN821x_RN721x_SOC_B)
/*********************************************************************************************************
** Function name:
** Descriptions: TPS测量温度
** input parameters:
** output parameters: 返回值为：温度,有符号数，单位：1/16 (℃）
** Returned value:
*********************************************************************************************************/
ErrorStatus LL_RTC_TPS_TemprMea(int32_t *sTmper)
{
  uint8_t i = 0U, j = 0U;
  int32_t temp32 = 0U; /*用于判符号位*/
  uint32_t u32Tmp = 0U;
  int32_t uReadTimes = 1U; /*默认不平均*/

  RTC->PS = RTC_RTCPS_EN; /*"RTC_PS "*/
  j = 0U;
  for (i = 0U; i < uReadTimes; i++)
  {
    if (fnRegState_Detect((uint32_t)(&RTC->CTRL), 10U, 0U, RTC_TIMEOUT) == ERN_SUCCESS) /*检查 cal_busy ，确保下降沿不允许启动温补*/
    {
      RTC->TPS_START = 1;                                                                    /*启动SAR进行测温*/
      SystemDelayUs(488);                                                                    /*等待adc采样完成， 理论需要延时4个clk，即4*(1/32768 s）= 4*122us=488us，SystemDelayUs函数不准，按3倍余量*/
      if (fnRegState_Detect((uint32_t)(&MADC->AD_STAT), 1U, 0U, RTC_TIMEOUT) == ERN_SUCCESS) /*检查tps_busy*/
      {
        SystemDelayUs(610);                                                                 /* 延时3个clk,等待计算完成，即3*（1/32768) = 3*122us= 366us，实际按5*122 = 610us延时*/
        if (fnRegState_Detect((uint32_t)(&RTC->CTRL), 10U, 0U, RTC_TIMEOUT) == ERN_SUCCESS) /*检查cal_busy确保计算完成*/
        {
          /*读取温度值*/
          u32Tmp = RTC->TEMP_CAL;
          // SEGGER_RTT_printf(0, "temp16 = %d \r\n" , u32Tmp);
          if ((u32Tmp & 0x800U) == 0x800U)
            temp32 += (int32_t)(u32Tmp | 0xfffff800U);
          else
            temp32 += (int32_t)(u32Tmp & 0x7ffU);
          j++;
        }
      }
    }
  }

  if (j != 0)
  {
    temp32 = (int32_t)((float)temp32 / j);
    *sTmper = temp32;
  }
  return (ERN_ERROR);
}
#endif

#endif /*LL_RTC_MODULE_ENABLED*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
