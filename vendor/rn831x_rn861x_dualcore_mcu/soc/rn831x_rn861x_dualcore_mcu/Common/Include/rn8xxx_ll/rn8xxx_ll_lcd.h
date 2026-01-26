/********************************************************************************
  * @file    Rn8xxx_Lcd.h
  * @author  Renergy Application Team
  * @version V1.1.1
  * @date    2023-09-07
  * @brief   Header file containing functions prototypes of Lcd HAL library.
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
#ifndef LcdDrive_H
#define LcdDrive_H

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll_def.h"
/* Exported define ------------------------------------------------------------*/
#define LCD_TIMEOUT 0x3fffu
#define LCDCTL_DEFAULT 0x00u   /*"LCD_CTL 寄存器复位默认值"*/

/*********************************************************************************/
#define GET_REG_LCD_CTRL(lcd, databits) ((lcd)->CTRL & GET_DATA_MASK(databits))
#if defined(RN831x_RN861x_MCU_V4)
#define GET_REG_LCD_STA(lcd, databits) ((lcd)->STATUS & GET_DATA_MASK(databits))
#else
#define GET_REG_LCD_STA(lcd, databits) ((lcd)->STA & GET_DATA_MASK(databits))
#endif
#define GET_REG_LCD_CLKDIV(lcd, databits) ((lcd)->CLKDIV & GET_DATA_MASK(databits))
#define GET_REG_LCD_BLINK(lcd, databits) ((lcd)->BLINK & GET_DATA_MASK(databits))
#define GET_REG_LCD_PS(lcd, databits) ((lcd)->PS & GET_DATA_MASK(databits))
#define GET_REG_LCD_RESCTL(lcd, databits) ((lcd)->RESCTL & GET_DATA_MASK(databits))

#define SET_REG_LCD_CTRL(lcd, data) ((lcd)->CTRL = (data))
#if defined(RN831x_RN861x_MCU_V4)
#define SET_REG_LCD_STA(lcd, data) ((lcd)->STATUS = (data))
#else
#define SET_REG_LCD_STA(lcd, data) ((lcd)->STA = (data))
#endif
#define SET_REG_LCD_CLKDIV(lcd, data) ((lcd)->CLKDIV = (data))
#define SET_REG_LCD_BLINK(lcd, data) ((lcd)->BLINK = (data))
#define SET_REG_LCD_PS(lcd, data) ((lcd)->PS = (data))
#define SET_REG_LCD_RESCTL(lcd, data) ((lcd)->RESCTL = (data))

/*********************************************************************************/

#define GET_BIT_LCD_CTRL(lcd, bit) (((lcd)->CTRL & (bit)) ? (1U) : (0U))
#if defined(RN831x_RN861x_MCU_V4)
#define GET_BIT_LCD_STA(lcd, bit) (((lcd)->STATUS & (bit)) ? (1U) : (0U))
#else
#define GET_BIT_LCD_STA(lcd, bit) (((lcd)->STA & (bit)) ? (1U) : (0U))
#endif
#define GET_BIT_LCD_CLKDIV(lcd, bit) (((lcd)->CLKDIV & (bit)) ? (1U) : (0U))
#define GET_BIT_LCD_BLINK(lcd, bit) (((lcd)->BLINK & (bit)) ? (1U) : (0U))
#define GET_BIT_LCD_PS(lcd, bit) (((lcd)->PS & (bit)) ? (1U) : (0U))
#define GET_BIT_LCD_RESCTL(lcd, bit) (((lcd)->RESCTL & (bit)) ? (1U) : (0U))

#define SET_BIT_LCD_CTRL(lcd, bit) ((lcd)->CTRL |= (bit))
#if defined(RN831x_RN861x_MCU_V4)
#define SET_BIT_LCD_STA(lcd, bit) ((lcd)->STATUS |= (bit))
#else
#define SET_BIT_LCD_STA(lcd, bit) ((lcd)->STA |= (bit))
#endif
#define SET_BIT_LCD_CLKDIV(lcd, bit) ((lcd)->CLKDIV |= (bit))
#define SET_BIT_LCD_BLINK(lcd, bit) ((lcd)->BLINK |= (bit))
#define SET_BIT_LCD_PS(lcd, bit) ((lcd)->PS |= (bit))
#define SET_BIT_LCD_RESCTL(lcd, bit) ((lcd)->RESCTL |= (bit))

/*"****************************  LCD_BUF  COM口定义    ****************************"*/
#define LCD_BUF_BIT0 (0x01U)
#define LCD_BUF_BIT1 (0x02U)
#define LCD_BUF_BIT2 (0x04U)
#define LCD_BUF_BIT3 (0x08U)
#define LCD_BUF_BIT4 (0x10U)
#define LCD_BUF_BIT5 (0x20U)
#define LCD_BUF_BIT6 (0x40U)
#define LCD_BUF_BIT7 (0x80U)

/*"****************************   LCD_BUF指针(SEG口)定义   ****************************"*/
#define LCD_BUF_0 (0U)   /*"偏移地址 0x	20	 "*/
#define LCD_BUF_1 (1U)   /*"偏移地址 0x	21	 "*/
#define LCD_BUF_2 (2U)
#define LCD_BUF_3 (3U)
#define LCD_BUF_4 (4U)
#define LCD_BUF_5 (5U)
#define LCD_BUF_6 (6U)
#define LCD_BUF_7 (7U)
#define LCD_BUF_8 (8U)
#define LCD_BUF_9 (9U)
#define LCD_BUF_10 (10U)
#define LCD_BUF_11 (11U)
#define LCD_BUF_12 (12U)
#define LCD_BUF_13 (13U)
#define LCD_BUF_14 (14U)
#define LCD_BUF_15 (15U)
#define LCD_BUF_16 (16U)
#define LCD_BUF_17 (17U)
#define LCD_BUF_18 (18U)
#define LCD_BUF_19 (19U)
#define LCD_BUF_20 (20U)
#define LCD_BUF_21 (21U)
#define LCD_BUF_22 (22U)
#define LCD_BUF_23 (23U)
#define LCD_BUF_24 (24U)
#define LCD_BUF_25 (25U)
#define LCD_BUF_26 (26U)
#define LCD_BUF_27 (27U)
#define LCD_BUF_28 (28U)
#define LCD_BUF_29 (29U)
#define LCD_BUF_30 (30U)
#define LCD_BUF_31 (31U)
#define LCD_BUF_32 (32U)
#define LCD_BUF_33 (33U)
#define LCD_BUF_34 (34U)
#define LCD_BUF_35 (35U)
#define LCD_BUF_36 (36U)
#define LCD_BUF_37 (37U)
#define LCD_LDO2_70V  (0x00U)
#define LCD_LDO3_60V  (0x0fU)
/* Exported enum ------------------------------------------------------------*/

typedef enum
{
#if defined(RN831x_RN861x_MCU_V4)
    LCD_AF_SEG = 0x00, /*" 端口复用为SEG "*/
    LCD_AF_COM = 0x01, /*" 端口复用为COM "*/
#endif

    LCD_CHARGEPUMP = 0x00,   /*"Charge Pmup 方式提供 LCD 偏置电压"*/
    LCD_RESISTORDIVI = 0x01, /*"内部电阻串分压方式提供 LCD 偏置电压 "*/

    LCD_TYPE_A = 0x00, /*"Type A 驱动为行反转方式，即在 1 个 frame 的时间内完成一次正负驱动的交替"*/
    LCD_TYPE_B = 0x01, /*"Type B 驱动为帧反转方式，即需要在 2 个 frame 的时间内才能完成一次正负驱动的交替 "*/

    LCD_1_3BIAS = 0x00, /*"1/3bias "*/
    LCD_1_4BIAS = 0x01, /*"1/4bias "*/

    LCD_STATIC_DUTY = 0x00, /*"000：静态输出（COM0）"*/
    LCD_1_2DUTY = 0x01,     /*"001： 1/2 占空比（COM0~1）"*/
    LCD_1_3DUTY = 0x02,     /*"010： 1/3 占空比（COM0~2）"*/
    LCD_1_4DUTY = 0x03,     /*"011： 1/4 占空比（COM0~3）"*/
    LCD_1_6DUTY = 0x04,     /*"100： 1/6 占空比（COM0~5）"*/
    LCD_1_8DUTY = 0x05,     /*"101： 1/8 占空比（COM0~7）"*/

    LCD_CTL_EN = 0x01,
    LCD_CTL_DIS = 0x00,

} eLcdCtrl_TypeDef;

typedef union
{
    struct
    {
        uint32_t EN : 1;
        uint32_t DUTY : 3;
        uint32_t BIAS : 1;
        uint32_t BIASLVL : 6;
        uint32_t TYPE : 1;
        uint32_t PWD_PUMP : 1;
#if defined(RN831x_RN861x_MCU_V4)
        uint32_t AF3 : 1;
        uint32_t AF2 : 1;
        uint32_t AF1 : 1;
        uint32_t AF0 : 1;
#endif
    }bitLcdCtrl;
    uint32_t LcdCtrl;
} uLcdCtrl_TypeDef;

#if defined(RN831x_RN861x_MCU_V4)
typedef enum
{
    LCD_AF0 = 13U,
    LCD_AF1 = 14U,
    LCD_AF2 = 15U,
    LCD_AF3 = 16U,
} eLcdCtrlAf_TypeDef;
#endif

/* Exported types ------------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
extern void LL_LCD_Init(uLcdCtrl_TypeDef LCD_CtlPara, uint16_t LCD_Voltage);
extern void LL_LCD_ScrSet(uint8_t value, uint8_t len);
extern void LL_LCD_ScrCpy(const uint8_t *buf, uint8_t len);
extern void LL_LCD_Disable(void);
#if defined(RN831x_RN861x_MCU_V4)
extern void LL_LCD_Af_Com_Seg_Cfg(eLcdCtrlAf_TypeDef LCD_AfPort, uint8_t SegComCfg);
#endif
#endif
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
