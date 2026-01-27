/********************************************************************************
  * @file    rn8xxx_ll_lcd.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-08-29
  * @brief   LCD driver
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
                     ##### LCD peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]


    @endverbatim
  */

/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll.h"
#ifdef LL_LCD_MODULE_ENABLED
/* Private define ------------------------------------------------------------*/

/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/*********************************************************************************************************
** Function name:       lcd_BIASLVL_Get
** Descriptions:        Charge Pmup 方式提供 LCD 偏置电压
** input parameters:    LCD_Voltage  液晶屏电压等级N.NNV(单位V ,量纲-2)
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
static uint8_t lcd_BIASLVL_Get(uint16_t Vref, uint8_t bisa, uint16_t LCD_Voltage)
{
    uint8_t BIASLVL;
    uint32_t VaVoltage;
    uint32_t u32_LCD_Voltage;
    u32_LCD_Voltage = LCD_Voltage * 1000;
    if (bisa == (uint8_t)LCD_1_3BIAS)
    {
        VaVoltage = u32_LCD_Voltage / 3;
        VaVoltage = VaVoltage / Vref;
    }
    else
    {
        VaVoltage = u32_LCD_Voltage / 4;
        VaVoltage = VaVoltage / Vref;
    }

    if (VaVoltage >= 1000)
    {
        BIASLVL =  0x20U + ((VaVoltage-1000)*63)/1000;
    }
    else
    {
        BIASLVL = (VaVoltage*63)/1000 - 32u;
    }
    BIASLVL = (BIASLVL & 0x3Fu);
    return BIASLVL;
}


/*********************************************************************************************************
** Function name:       lcd_LDO_V_Get
** Descriptions:        电阻串分压方式
** input parameters:    LCD_Voltage  液晶屏电压等级N.NNV(单位V ,量纲-2)
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
static uint16_t lcd_LDO_V_Get(uint16_t LCD_Voltage)
{
    uint16_t lcd_ldo_V;
    if (LCD_Voltage <= 270u)
    {
        lcd_ldo_V = LCD_LDO2_70V;
    }
    else if (LCD_Voltage >= 360u)
    {
        lcd_ldo_V = LCD_LDO3_60V;
    }
    else
    {
        lcd_ldo_V = (LCD_Voltage - 270u) / 6u;
    }
    return lcd_ldo_V;
}

#if defined(RN831x_RN861x_MCU_V4)
/*********************************************************************************************************
** Function name:       LL_LCD_Af_Com_Seg_Cfg
** Descriptions:        SEG和COM复用配置
** input parameters:    LCD_AfPort  复用端口，可选参数: LCD_AF0/LCD_AF1/LCD_AF2/LCD_AF3
                        SegComCfg   复用配置，可选参数: LCD_AF_SEG/LCD_AF_COM
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_LCD_Af_Com_Seg_Cfg(eLcdCtrlAf_TypeDef LCD_AfPort, uint8_t SegComCfg)
{
    uint32_t Ctrl_cfg, Ctrl_bak;
    Ctrl_bak = GET_REG_LCD_CTRL(LCD, 31);
    Ctrl_cfg = Ctrl_bak | (SegComCfg << LCD_AfPort);
    SET_REG_LCD_CTRL(LCD, Ctrl_cfg);
}
#endif

/* Exported functions ----------------------------------------------------------*/

/*********************************************************************************************************
** Function name:       LL_LCD_Init
** Descriptions:        液晶驱动初始化
** input parameters:    mode  驱动模式
**                      type
**                      bisa  由液晶bisa参数决定
**                      duty  由液晶COM口决定
**                      LCD_Voltage  液晶屏电压等级N.NNV(单位V ,量纲-2)
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_LCD_Init(uLcdCtrl_TypeDef LCD_CtlPara, uint16_t LCD_Voltage)
{

    uint16_t lcd_ldo_V;
    uint8_t BIASLVL;
    /*"开启LCD的APB时钟"*/

    LCD->CTRL = 0x0u;

    /*"等待 LCD_STATUS 状态寄存器第 6 位是否为 0，空闲状态，"*/
    /*"如空闲则进入下一步，否则等待，"*/
    /*"等待时   间大概为 16MS 左右。"*/
#if defined(RN831x_RN861x_MCU_V4)
    if (fnRegState_Detect((uint32_t)(&LCD->STATUS), 6u, 0u, LCD_TIMEOUT) == ERN_ERROR)
    {
        return;
    }
#else
    if (fnRegState_Detect((uint32_t)(&LCD->STA), 6u, 0u, LCD_TIMEOUT) == ERN_ERROR)
    {
        return;
    }
#endif
    if ((uint8_t)LCD_CHARGEPUMP == LCD_CtlPara.bitLcdCtrl.PWD_PUMP)
    {
        LCD_CtlPara.bitLcdCtrl.EN = (uint8_t)LCD_CTL_DIS;

        BIASLVL = lcd_BIASLVL_Get(125u, (uint8_t)LCD_CtlPara.bitLcdCtrl.BIAS, LCD_Voltage);
        LCD_CtlPara.bitLcdCtrl.BIASLVL = BIASLVL;

        LCD->CTRL = LCD_CtlPara.LcdCtrl;
        #if !defined(RN821x_RN721x_SOC_B)
        LCD->RESCTL = 0xB81u; /*"默认值"*/
        #endif
    }
    else
    {
        LCD_CtlPara.bitLcdCtrl.EN = (uint8_t)LCD_CTL_DIS;

        LCD->CTRL = LCD_CtlPara.LcdCtrl;

        lcd_ldo_V = lcd_LDO_V_Get(LCD_Voltage);
        #if !defined(RN821x_RN721x_SOC_B)
        LCD->RESCTL = ((1u << 2u) |         /*" RES_FT驱动 2 次"*/
                       (1u << 4u) |         /*"RES_DT"*/
                       (1u << 8u) |         /*"打开快速充电功能 "*/
                       ((uint32_t)lcd_ldo_V << 9u)); /*"LDO 输出电平选择信号 "*/
        #endif
    }
    switch (LCD_CtlPara.bitLcdCtrl.DUTY)
    {
        case (uint8_t)LCD_STATIC_DUTY:
            LCD->CLKDIV = 0xffU;
            break;
        case (uint8_t)LCD_1_2DUTY:
            LCD->CLKDIV = 0x7fU;
            break;
        case (uint8_t)LCD_1_3DUTY:
            LCD->CLKDIV = 0x54U;
            break;
        case (uint8_t)LCD_1_4DUTY:
            LCD->CLKDIV = 0x3fU;
            break;
        case (uint8_t)LCD_1_6DUTY:
            LCD->CLKDIV = 0x2aU;
            break;
        case (uint8_t)LCD_1_8DUTY:
            LCD->CLKDIV = 0x1fU;
            break;
        default: break;
    }

    LCD->BLINK = 0x000001U; /*"关闭闪烁功能"*/

    LCD->CTRL |= (uint32_t)LCD_CTL_EN;

}

/*********************************************************************************************************
** Function name:       LL_LCD_ScrSet
** Descriptions:        液晶内容设置
** input parameters:    value：设置值，len:长度
** output parameters:       None
** Returned value:      None
*********************************************************************************************************/
void LL_LCD_ScrSet(uint8_t value, uint8_t len)
{
    uint8_t i;
    if (len > 38u)

    {
        return;
    }
    for (i = 0; i < len; i++)
    {
        LCD->BUF[i] = value;
    }
    return;
}

/*********************************************************************************************************
** Function name:       LL_LCD_ScrCpy
** Descriptions:        液晶内容设置
** input parameters:    buf:输入参数 len：长度
** output parameters:       None
** Returned value:      None
*********************************************************************************************************/
void LL_LCD_ScrCpy(const uint8_t *buf, uint8_t len)
{
    uint8_t i;
    if (len > 38u)
    {
        return;
    }
    for (i = 0; i < len; i++)
    {
        LCD->BUF[i] = buf[i];
    }
    return;
}

/*********************************************************************************************************
** Function name:LL_LCD_Disable
** Descriptions:
** input parameters:
** output parameters:
** Returned value:
*********************************************************************************************************/
void LL_LCD_Disable(void)
{
    LCD->CTRL &= (~BIT0);
}
#endif /*LL_LCD_MODULE_ENABLED*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
