/********************************************************************************
  * @file    LL_LvdCmpSar.c
  * @author  Renergy Application Team
  * @version V1.1.1
  * @date    2025-01-14
  * @brief   Lvd\sar-adc\cmp驱动
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
                     ##### LvdCmpSar peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]
    V1.1.1 25.1.14
    1、修改LL_ANA_SarADCGet函数，i定义为u8，但与0x3fff比较的bug。

    @endverbatim
  */

/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll.h"
#ifdef LL_MADC_MODULE_ENABLED
/* Private define ------------------------------------------------------------*/
#if defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3) || \
    defined(RN202x_RN7326_SOC_V2)
#define SAR_REG_ANA_PASSWD (*(uint32_t *)(0x4002C044)) /*"ANA_PASSWD "*/
#define SAR_REG_SAR_TEST (*(uint32_t *)(0x4002C04c))   /*"SAR TEST"*/
#define SAR_REG_ANA_CTL (*(uint32_t *)(0x4002C058))    /*"ANA_ CTL"*/
#endif

#if defined(RN831x_RN861x_MCU_V4)
#define SAR_REG_ANA_PASSWD (*(uint32_t *)(0x200B8044)) /*"ANA_PASSWD "*/
#define SAR_REG_SAR_TEST (*(uint32_t *)(0x200B804c))   /*"SAR TEST"*/
#define SAR_REG_ANA_CTL (*(uint32_t *)(0x200B8058))    /*"ANA_ CTL"*/
#endif

/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
#if defined(RN831x_RN861x_MCU_V4)
#define RN_LVDCTRL  (LVDCMP->LVDCTRL)
#define RN_LVD_STAT (LVDCMP->IF)
#else
#define RN_LVDCTRL  (MADC->LVD_CTRL)
#define RN_LVD_STAT (MADC->LVD_STAT)
#endif
/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/

/*********************************************************************************************************
** Function name:
** Descriptions:       	 LVD模块初始化
** input parameters:     lvds:lvd配置寄存器 SwitchIE:中断使能寄存器
** output parameters:   无
** Returned value: 	 无
*********************************************************************************************************/

void LL_ANA_LVDInit(eLVD_LVDS_Typedef lvds, eLVDCMPSAR_IE_TypeDef SwitchIE)
{
    RN_LVDCTRL &= 0xFFFFFFD0;
    RN_LVDCTRL |= (((uint32_t)lvds) | ((uint32_t)SwitchIE << 5)); /*" LVD 阈值电压及LVD中断使能设置"*/

    RN_LVD_STAT = 0x10;/*清LVD状态*/
}

/*********************************************************************************************************
** Function name:     	 LL_ANA_LVDDisable
** Descriptions:       	 关闭LVD模块
** input parameters:     void
** output parameters:   无
** Returned value: 	 无
*********************************************************************************************************/
void LL_ANA_LVDDisable(void)
{
    SYSCTL_ENABLE_WRITE;
    LVD_PD_POWDWN;
#ifndef RN831x_RN861x_MCU_V4
    RN_LVDCTRL &= 0xFFFFFFD0U;
#else
    LVDCMP->LVDCTRL &= 0xFFFFFFD0;
#endif
    SYSCTL_DISABLE_WRITE;
}

/*********************************************************************************************************
** Function name:     	LL_ANA_CMPInit
** Descriptions:       	 CMP初始化
** input parameters:     ch: 通道 SwitchR600：内部下拉600K电阻配置
**                       SwitchHysen：内部迟滞
** output parameters:   无
** Returned value: 	 无
*********************************************************************************************************/
void LL_ANA_CMPInit(eCMP_CH_TypeDef ch, eCMP_R600K_TypeDef SwitchR600, eCMP_Hysen_TypeDef SwitchHysen, eLVDCMPSAR_IE_TypeDef SwitchIE)
{
   	SYSCTL_ENABLE_WRITE;
    if (ch == CMP1_CHANNEL)
    {
        CMP1_PD_POWON;
        if (SwitchR600 == CMP_R600K_DISABLE)
        {
            PWD_CMP1R_DIS;
        }
        else
        {
            PWD_CMP1R_EN;
        }

#if defined(RN831x_RN861x_MCU_V4)
        BITBAND_ADDR((uint32_t)(&LVDCMP->CMPCTRL), 2U) = (uint32_t)SwitchHysen;
#else
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 10) = (uint32_t)SwitchHysen;
#endif

        if (SwitchIE == LVDCMPSAR_IE_ENABLE)
        {
            RN_LVDCTRL |= 0x00000040; /*"BIT6 CMP1IE"*/
        }
        else
        {
            RN_LVDCTRL &= (~0x00000040); /*"BIT6 CMP1IE"*/
        }
    }
    else
    {
        CMP2_PD_POWON;
        if (SwitchR600 == CMP_R600K_DISABLE)
        {
            PWD_CMP2R_DIS;
        }
        else
        {
            PWD_CMP2R_EN;
        }

#if defined(RN831x_RN861x_MCU_V4)
        BITBAND_ADDR((uint32_t)(&LVDCMP->CMPCTRL), 3U) = (uint32_t)SwitchHysen;
#else
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 11) = (uint32_t)SwitchHysen;
#endif

        if (SwitchIE == LVDCMPSAR_IE_ENABLE)
        {
            RN_LVDCTRL |= 0x00000080; /*"BIT7 CMP2IE"*/
        }
        else
        {
            RN_LVDCTRL &= (~0x00000080); /*"BIT7 CMP2IE"*/
        }
    }
    SYSCTL_DISABLE_WRITE;
}

/*********************************************************************************************************
** Function name:     	 LL_ANA_CMPDisable
** Descriptions:       	 关闭CMP模块
** input parameters:     无
** output parameters:   无
** Returned value: 	 无
*********************************************************************************************************/
void LL_ANA_CMPDisable(eCMP_CH_TypeDef ch)
{
    SYSCTL_ENABLE_WRITE;
    if (ch == CMP1_CHANNEL)
    {
        CMP1_PD_POWDWN;
        PWD_CMP1R_DIS;
        HYSEN_CMP1_DIS;
        RN_LVDCTRL &= (~0x00000040); /*"BIT6 CMP1IE"*/
    }
    else if(ch == CMP2_CHANNEL)
    {
        CMP2_PD_POWDWN;
        PWD_CMP2R_DIS;
        HYSEN_CMP2_DIS;
        RN_LVDCTRL &= (~0x00000080); /*"BIT7 CMP2IE"*/
    }
    else
    {
      /* lint -e(9063) */
    }
    SYSCTL_DISABLE_WRITE;
}

/*********************************************************************************************************
** Function name:     	 LL_ANA_SarADCInit
** Descriptions:       	 sarADC初始化
** input parameters:     ch：配置通道 pga：增益
** output parameters:   无
** Returned value: 	 无
*********************************************************************************************************/
void LL_ANA_SarADCInit(eSarCtrl_CH_TypeDef ch, eSarCtrl_PGA_TypeDef pga)
{

    if (fnRegState_Detect((uint32_t)(&MADC->AD_STAT), 1, 0, LVDCMPSAR_TIMEOUT) == ERN_ERROR)
    {/* 检测自动测温是否忙，如果忙，退出SAR ADC启动；防止SAR和测温冲突 */
        return;
    }

    MADC->AD_STAT = 0x01; /*"清除ADC 转换结果已完成标志"*/


    MADC->AD_CTRL = (((uint32_t)(ch&7U) << 0) |     /*" SAR_CH"*/
                         ((uint32_t)pga << 3) |    /*" SAR-ADC 增益控制"*/
                         (0U << 5) |     /*" SAR-ADC 中断控制"*/
                         ((uint32_t)((ch>>3U)&1U)<< 6) |     /*" SAR_CH 定义"*/
                         (0x0eU << 7) |  /*" SAR ADC 在开启后到开始采样转换需要等待的时间选择457.5 微秒"*/
                         (0x10U << 12)); /*" 从开启 REF 到开启 ADC 需要等待的时间"*/

}
/*********************************************************************************************************
** Function name:     	 fnSarADC_Get
** Descriptions:       	 获取测量的sarADC值
** input parameters:     void
** output parameters:
** Returned value: 	 ADC测量值
*********************************************************************************************************/
uint32_t LL_ANA_SarADCGet(void)
{
    uint32_t SAR_DAT=0;
    uint32_t i=0;
    if (fnRegState_Detect((uint32_t)(&MADC->AD_STAT), 1, 0, LVDCMPSAR_TIMEOUT) == ERN_ERROR)
    {/*status 寄存器bit1=0，自动温度测量没有进行，才可启动sar 测温 ，自动温补时间*/
        return 0;
    }
    MADC->AD_START = 0x01; /*"star sar-adc"*/

    if (fnRegState_Detect((uint32_t)(&MADC->AD_STAT), 0, 1, LVDCMPSAR_TIMEOUT) == ERN_SUCCESS)
    {/*status 寄存器bit1=1，sar测量完成；sar测量完成后清状态*/
        MADC->AD_STAT = 0x01; /*"clear sar-adc stat"*/
        for(i=0;i < LVDCMPSAR_TIMEOUT;i++)
        {
          if (fnRegState_Detect((uint32_t)(&MADC->AD_STAT), 0, 0, 1) == ERN_ERROR)
          {/* MCU和三相SOC要求该位置1后，至少延时30us在清零，才能可靠清零；该处检测为保证可靠清零，如果没有清零成功会再次启动清零；查询函数有超时处理，故不做循环检测处理 */
            MADC->AD_STAT = 0x01; /*"clear sar-adc stat"*/
          }
          else
          { /* 清零成功 */
            break;
          }
        }
        SAR_DAT = MADC->AD_DATA;
        return (SAR_DAT);
    }
    else
    {
        return (0);
    }
}
/*********************************************************************************************************
** Function name:     	 LL_ANA_LvdCmpStatCheck
** Descriptions:       	 LvdCmp 状态标志位判断
** input parameters:      ch:通道LVD/CMP1/CMP2 ;
**					checkTime :检测时间，单位us
** output parameters:
** Returned value: 	 状态标志位高于阈值或低于阈值:
**                           ERN_SUCCESS:高于阈值
**                           ERN_ERROR:低于阈值
*********************************************************************************************************/
ErrorStatus LL_ANA_LvdCmpStatCheck(eLVDCMP_CH_TypeDef ch, uint32_t checkTime)
{
    if (fnRegState_Detect((uint32_t)(&RN_LVD_STAT), (uint32_t)ch, 1U, checkTime) == ERN_SUCCESS)
        return ERN_SUCCESS;
    else
        return ERN_ERROR;
}
/*********************************************************************************************************
** Function name:     	 LL_ANA_LvdCmpStatCheck
** Descriptions:       	 LvdCmp 状态标志位判断
** input parameters:      ch:通道LVD/CMP1/CMP2 ;
**					checkTime :检测时间，单位us
** output parameters:
** Returned value: 	 状态标志位高于阈值或低于阈值:
**                           ERN_SUCCESS:低于阈值
**                           ERN_ERROR:高于阈值
*********************************************************************************************************/
ErrorStatus LL_ANA_LvdCmp_STATPowerDown(eLVDCMP_CH_TypeDef ch, uint32_t checkTime)
{
    if (fnRegState_Detect((uint32_t)(&RN_LVD_STAT), (uint32_t)ch, 1, checkTime) == ERN_SUCCESS)
        return ERN_ERROR;  //只要有一次高于阈值就保持上电模式，防止抖动
    else
        return ERN_SUCCESS;  //持续低于阈值时判断为掉电模式
}

/*********************************************************************************************************
** Function name:     	 LL_ANA_LvdCmp_STATPowerUp
** Descriptions:       	 LvdCmp 状态标志位判断  ,
**               只要有一次低于阈值就判断为掉电，防止上电抖动
** input parameters:      ch:通道LVD/CMP1/CMP2 ;
**					checkTime :检测时间，单位us
** output parameters:
** Returned value: 	 状态标志位高于阈值或低于阈值:
**                           ERN_SUCCESS:高于阈值
**                           ERN_ERROR:低于阈值
*********************************************************************************************************/
ErrorStatus LL_ANA_LvdCmp_STATPowerUp(eLVDCMP_CH_TypeDef ch, uint32_t checkTime)
{
    if (fnRegState_Detect((uint32_t)(&RN_LVD_STAT), (uint32_t)ch, 0, checkTime) == ERN_SUCCESS)
        return ERN_ERROR;   //只要有一次低于阈值就判断为掉电，防止上电抖动
    else
      return ERN_SUCCESS; //上电模式
}
/*********************************************************************************************************
** Function name:     	 LL_ANA_LvdCmpSarModeConfig
** Descriptions:       	 SAR测温模式配置
** input parameters:
**
** output parameters:
** Returned value:
*********************************************************************************************************/
#if defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3) || \
    defined(RN831x_RN861x_MCU_V4) || \
    defined(RN202x_RN7326_SOC_V2)
void LL_ANA_LvdCmpSarModeConfig(eSAR_PerfMODE PerfMODE)
{
    if (SAR_HIGHMODE == PerfMODE)
    {
        SAR_REG_ANA_PASSWD = 0x8E;
        SAR_REG_SAR_TEST = (0x00000002 << 15);   /*"SARPS"*/
        SAR_REG_SAR_TEST = ((0x00000002 << 15) | /*"SARPS"*/
                            (0x00000002 << 13) | /*"F_SAR"*/
                            (0x00000001 << 12) | /*"SAR_MODE"*/
                            (0x00000001 << 10) | /*"dem_mode"*/
                            (0x0000000F << 6) |  /*"ibsel_sar"*/
                            (0x00000001 << 11) | /*"IBIAS_MODE_H"*/
                            (0x00000003 << 4) |  /*"IBIAS_MODE"*/
                            (0x00000001 << 3));  /*"DEM_EN"*/
        SAR_REG_ANA_CTL = 0x10000;               /*"寄存器写使能"*/
        SAR_REG_ANA_CTL = 0x1a8a8;              /*"使能RCH和BGR"*/
    }
    else
    {
        SAR_REG_ANA_PASSWD = 0x8E;
        SAR_REG_SAR_TEST = (0x00000002 << 15);   /*"SARPS"*/
        SAR_REG_SAR_TEST = ((0x00000002 << 15) | /*"SARPS"*/
                            (0x00000002 << 13) | /*"F_SAR"*/
                            (0x00000001 << 12) | /*"SAR_MODE"*/
                            (0x00000000 << 10) | /*"dem_mode"*/
                            (0x0000000F << 6) |  /*"ibsel_sar"*/
                            (0x00000001 << 11) | /*"IBIAS_MODE_H"*/
                            (0x00000001 << 4) |  /*"IBIAS_MODE"*/
                            (0x00000001 << 3));  /*"DEM_EN"*/
        SAR_REG_ANA_CTL = 0x10000;               /*"寄存器写使能"*/
        SAR_REG_ANA_CTL = 0x10000;
    }
}
#endif
#endif /*LL_ANA_MODULE_ENABLED*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
