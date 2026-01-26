/********************************************************************************
  * @file    Rn8xxx_Gpio.c
  * @author  Renergy Application Team
  * @version V1.1.2
  * @date    2023-10-08
  * @brief   GPIO驱动
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
                     ##### GPIO peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]


    @endverbatim
  */

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll.h"
#ifdef LL_GPIO_MODULE_ENABLED

#if !defined(RN831x_RN861x_MCU_V4)
/* Private define ------------------------------------------------------------*/
#define GPIO_ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

#ifdef RN202x_RN7326_SOC_V2
#define GPIO_PSW0_EnableWrite   {GPIO->PSW0 = 0x82;}
#define GPIO_PSW0_DisableWrite  {GPIO->PSW0 = 0x00 ;}
#define GPIO_PSW1_EnableWrite   {GPIO_APB->PSW1 = 0x82;}
#define GPIO_PSW1_DisableWrite  {GPIO_APB->PSW1 = 0x00;}
#define GPIO_PB0MASK_Enable     {GPIO_APB->PB0MASK = 0x8a5e;}
#endif
/* Private enum -------------------------------------------------------------*/
typedef enum
{
    _bit0 = 0x00,
    _bit1 = 0x01,
    _bit2 = 0x02,
    _bit3 = 0x03,
    _bit4 = 0x04,
    _bit5 = 0x05,
    _bit6 = 0x06,
    _bit7 = 0x07,

    _bit8 = 0x08,
    _bit9 = 0x09,
    _bit10 = 0x0a,
    _bit11 = 0x0b,
    _bit12 = 0x0c,
    _bit13 = 0x0d,
    _bit14 = 0x0e,
    _bit15 = 0x0f,

    _bit16 = 0x10,
    _bit17 = 0x11,
    _bit18 = 0x12,
    _bit19 = 0x13,
    _bit20 = 0x14,
    _bit21 = 0x15,
    _bit22 = 0x16,
    _bit23 = 0x17,

    _bit24 = 0x18,
    _bit25 = 0x19,
    _bit26 = 0x1a,
    _bit27 = 0x1b,
    _bit28 = 0x1c,
    _bit29 = 0x1d,
    _bit30 = 0x1e,
    _bit31 = 0x1f
} eGPIOBitNumber_TypeDef;

typedef enum
{
    GPIO_PCA0,
    GPIO_PCA1,
    #if defined(RN831x_RN861x_MCU_V3) ||\
        defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3) || \
        defined(RN202x_RN7326_SOC_B) || defined(RN202x_RN7326_SOC_V2)
    GPIO_PCA2,
    #endif
    #if defined(RN831x_RN861x_MCU_V3) ||\
        defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    GPIO_PCA3,
    #endif
    #if defined(RN202x_RN7326_SOC_B) || defined(RN202x_RN7326_SOC_V2)
    GPIO_PCB1,
    #else
    GPIO_PCB,
    #endif
    #if defined(RN831x_RN861x_MCU_V1) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) ||\
        defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_D) || defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    GPIO_PCB2,
    #endif
    #if defined(RN831x_RN861x_MCU_V3) ||\
        defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3) ||\
        defined(RN202x_RN7326_SOC_V2)
    GPIO_PCB3,
    #endif
    #if defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
    GPIO_PCB5,
    #endif
    #if defined(RN831x_RN861x_MCU_V1) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) ||\
        defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_D)|| defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3) ||\
        defined(RN202x_RN7326_SOC_V2)
    GPIO_PCC,
    #endif
    #if defined(RN831x_RN861x_MCU_V3) ||\
        defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    GPIO_PCC2,
    GPIO_PCC3,
    GPIO_PCC4,
    #endif
    #if defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
    GPIO_PCC5,
    #endif
    #if defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) ||\
        defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    GPIO_PCD,
    #endif
    #if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    GPIO_PCD2,
    #endif
    #if defined(RN831x_RN861x_MCU_V1) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_D) ||\
    defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    GPIO_PCE,
    #endif
    #ifdef RN202x_RN7326_SOC_V2
    GPIO_IOCFG_UART_I2C,
    GPIO_IOCFG_IOCNT,
    #endif
}eGPIOFunRegID_TypeDef;/*lint !e726*/

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
    //volatile uint32_t *RegAd;
    eGPIOFunRegID_TypeDef RegID;
    eGPIOBitNumber_TypeDef starNum;
} sGPIOFunReg_TypeDef; /*"GPIO功能配置寄存器"*/

typedef struct
{
    eGPIOFunction_TypeDef GpioFun;
    uint8_t PutData;
} sGPIOFunCll_TypeDef; /*"GPIO功能配置值"*/

typedef struct
{
    const sGPIOFunCll_TypeDef *FunCell;
    const sGPIOFunReg_TypeDef *FunRegCell;
    uint8_t FunCellNum;
    uint8_t FunRegCellNum;
} sGPIOFunTable_TypeDef;

/* Private data ---------------------------------------------------------*/
/*"***************************** IO口复用功能配置寄存器字典 *****************************"*/
static volatile uint32_t * const GPIO_FunReg_Dict[] = {
    (volatile uint32_t *)&GPIO->PCA0,
    (volatile uint32_t *)&GPIO->PCA1,
    #if defined(RN831x_RN861x_MCU_V3) ||\
        defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3) || \
        defined(RN202x_RN7326_SOC_B) || defined(RN202x_RN7326_SOC_V2)
    (volatile uint32_t *)&GPIO->PCA2,
    #endif
    #if defined(RN831x_RN861x_MCU_V3) ||\
        defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    (volatile uint32_t *)&GPIO->PCA3,
    #endif
    #if defined(RN202x_RN7326_SOC_B) || defined(RN202x_RN7326_SOC_V2)
    (volatile uint32_t *)&GPIO->PCB1,
    #else
    (volatile uint32_t *)&GPIO->PCB,
    #endif
    #if defined(RN831x_RN861x_MCU_V1) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) ||\
        defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_D) || defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    (volatile uint32_t *)&GPIO->PCB2,
    #endif
    #if defined(RN831x_RN861x_MCU_V3) ||\
        defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3) ||\
        defined(RN202x_RN7326_SOC_V2)
    (volatile uint32_t *)&GPIO->PCB3,
    #endif
    #if defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
    (volatile uint32_t *)&GPIO->PCB5,
    #endif
    #if defined(RN831x_RN861x_MCU_V1) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) ||\
        defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_D)|| defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3) ||\
        defined(RN202x_RN7326_SOC_V2)
    (volatile uint32_t *)&GPIO->PCC,
    #endif
    #if defined(RN831x_RN861x_MCU_V3) ||\
        defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    (volatile uint32_t *)&GPIO->PCC2,
    (volatile uint32_t *)&GPIO->PCC3,
    (volatile uint32_t *)&GPIO->PCC4,
    #endif
    #if defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
    (volatile uint32_t *)&GPIO->PCC5,
    #endif
    #if defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) ||\
        defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    (volatile uint32_t *)&GPIO->PCD,
    #endif
    #if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    (volatile uint32_t *)&GPIO->PCD2,
    #endif
    #if defined(RN831x_RN861x_MCU_V1) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_D) ||\
    defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    (volatile uint32_t *)&GPIO->PCE,
    #endif
    #ifdef RN202x_RN7326_SOC_V2
    (volatile uint32_t *)&GPIO->IOCFG_UART_I2C,
    (volatile uint32_t *)&GPIO->IOCFG_IOCNT,
    #endif
};
/*"***************************** P0_0 *****************************"*/
static const sGPIOFunReg_TypeDef P0_0_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_IOCNT[0], PCA0[1], PCA0[0]"*/
    {GPIO_PCA0, _bit0},        /*"Bit0"*/
    {GPIO_PCA0, _bit1},        /*"Bit1"*/
    {GPIO_IOCFG_IOCNT, _bit0}, /*"Bit2"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /*"由{PCA2[1:0],PCA0[1:0]} 定义端口 P00 复用配置："*/
    {GPIO_PCA0, _bit0}, /*"Bit0"*/
    {GPIO_PCA0, _bit1}, /*"Bit1"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA2[1:0],PCA0[1:0]} 定义端口 P00 复用配置："*/
    {GPIO_PCA0, _bit0}, /*"Bit0"*/
    {GPIO_PCA0, _bit1}, /*"Bit1"*/
    {GPIO_PCA2, _bit0}, /*"Bit2"*/
    {GPIO_PCA2, _bit1}, /*"Bit3"*/
#else
    /*"由{PCA2[1:0],PCA0[1:0]} 定义端口 P00 复用配置："*/
    {GPIO_PCA0, _bit0}, /*"Bit0"*/
    {GPIO_PCA0, _bit1}, /*"Bit1"*/
#endif
};
static const sGPIOFunCll_TypeDef P0_0_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /*"    =0000:选择为 IO 口"*/
    {_AIN, 0x01},        /*"    =0001:选择为模拟输入口CMP0"*/
    {_INT, 0x02},        /*"    =0010:选择为 INT0"*/
    {_CF_OUT2, 0x03},    /*"    =0011:选择为CF2"*/
    {_CF2, 0x03},        /*"    =0011:选择为CF2"*/
    {_IOCNT_OUT2, 0x07}, /*"	=0111:选择为IOCNT2"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_AIN, 0x01},      /*"=0001： 选择为模拟输入口 AIN0"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY2"*/
    {_UART2, 0x03},    /*"=0011： 选择为 RX2,PCA3[28]， PCA0[31:30]}=001 特殊处理"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_AIN, 0x01},      /*"=0001： 选择为模拟输入口 AIN0"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY2"*/
    {_UART2, 0x03},    /*"=0011： 选择为 RX2"*/
    {_IA_IN, 0x04},   /*"=0100： 选择为 IA_IN"*/
    {_IB_IN, 0x05},   /*"=0101： 选择为 IB_IN"*/
    {_TC1_P_1, 0x06}, /*"=0110： 选择为 TC1_P[1]"*/
                      /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_AIN, 0x01},      /*"=0001： 选择为模拟输入口 AIN0"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY2"*/
    {_UART2, 0x03},    /*"=0011： 选择为 RX2"*/

/*"注： 当 P12 选择为 KEY2 时， P12 优先级高于 P00， P00 输入无效"*/
#endif
};

/*"***************************** P0_1 *****************************"*/
static const sGPIOFunReg_TypeDef P0_1_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[23], PCB1[30], PCA1[31], PCA0[3], PCA0[2]"*/
    {GPIO_PCA0, _bit2},            /*"Bit0"*/
    {GPIO_PCA0, _bit3},            /*"Bit1"*/
    {GPIO_PCA1, _bit31},           /*"Bit2"*/
    {GPIO_PCB1, _bit30},           /*"Bit3"*/
    {GPIO_IOCFG_UART_I2C, _bit23}, /*"Bit4"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /*"由{PCA2[3:2],PCA0[3:2]}定义端口 P01 复用配置："*/
    {GPIO_PCA0, _bit2}, /*"Bit0"*/
    {GPIO_PCA0, _bit3}, /*"Bit1"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA2[3:2],PCA0[3:2]}定义端口 P01 复用配置："*/
    {GPIO_PCA0, _bit2}, /*"Bit0"*/
    {GPIO_PCA0, _bit3}, /*"Bit1"*/
    {GPIO_PCA2, _bit2}, /*"Bit2"*/
    {GPIO_PCA2, _bit3}, /*"Bit3"*/
#else
    /*"由{PCA2[3:2],PCA0[3:2]}定义端口 P01 复用配置："*/
    {GPIO_PCA0, _bit2}, /*"Bit0"*/
    {GPIO_PCA0, _bit3}, /*"Bit1"*/
#endif
};
static const sGPIOFunCll_TypeDef P0_1_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /*"=00000:选择为IO口；"*/
    {_AIN, 0x01},      /*"=00001: 选择为模拟输入口SDAIN4；"*/
    {_TC_IN, 0x02},    /*"=00010:TCI；"*/
    {_KEY, 0x03},      /*"=00011: 选择为KEY0；"*/
    {_IIC0, 0x04},     /*"=00100: SCL,I2C0"*/
    {_IIC1, 0x14},     /*"=10100: SCL,I2C1"*/
    {_LVD, 0x08},      /*"=01000: P01为LVD输入"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_AIN, 0x01},      /*"=0001： 选择为模拟输入口 AIN1"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY3"*/
    {_UART2, 0x03},    /*"=0011： 选择为 TX2;PCA0[31:30]}=001 特殊处理"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_AIN, 0x01},      /*"=0001： 选择为模拟输入口 AIN1"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY3"*/
    {_UART2, 0x03},    /*"=0011： 选择为 TX2"*/
    {_IA_IN, 0x04},   /*"=0100： 选择为 IA_IN"*/
    {_U_IN, 0x05},    /*"=0101： 选择为 U_IN"*/
    {_TC1_N_1, 0x06}, /*"=0110： 选择为 TC1_N[1]"*/
                      /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_AIN, 0x01},      /*"=0001： 选择为模拟输入口 AIN1"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY3"*/
    {_UART2, 0x03},    /*"=0011： 选择为 TX2"*/
/*"注： 当 P13 选择为 KEY3 时， P13 优先级高于 P01， P01 输入无效"*/
#endif
};

/*"***************************** P0_2 *****************************"*/
static const sGPIOFunReg_TypeDef P0_2_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[23], IOCFG_IOCNT[1], PCA1[31], PCA0[5], PCA0[4]"*/
    {GPIO_PCA0, _bit4},            /*"Bit0"*/
    {GPIO_PCA0, _bit5},            /*"Bit1"*/
    {GPIO_PCA1, _bit31},           /*"Bit2"*/
    {GPIO_IOCFG_IOCNT, _bit1},     /*"Bit3"*/
    {GPIO_IOCFG_UART_I2C, _bit23}, /*"Bit4"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA2[6:4],PCA0[4]}定义端口 P02 的复用配置："*/
    {GPIO_PCA0, _bit4}, /*"Bit0"*/
    {GPIO_PCA2, _bit4}, /*"Bit1"*/
    {GPIO_PCA2, _bit5}, /*"Bit2"*/
    {GPIO_PCA2, _bit6}, /*"Bit3"*/
#else
    /*"由{PCA2[6:4],PCA0[4]}定义端口 P02 的复用配置："*/
    {GPIO_PCA0, _bit4}, /*"Bit0"*/
#endif
};
static const sGPIOFunCll_TypeDef P0_2_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /*"=00000: 选择为IO口；"*/
    {_CF_OUT0, 0x01},    /*"=00001: 选择为CF0"*/
    {_CF0, 0x01},        /*"=00001: 选择为CF0"*/
    {_IOCNT_OUT0, 0x09}, /*"=01001: 选择为IOCNT0"*/
    {_RTC_OUT, 0x20},    /*"=00010: 选择为RTCOUT0；（其他口若配置为RTCOUT时，可同时输出；）"*/
    {_AIN, 0x03},        /*"=00011: 选择为模拟输入SDAIN5"*/
    {_IIC0, 0x04},       /*"=00100: SDA,I2C0"*/
    {_IIC1, 0x14},       /*"=10100: SDA,I2C1"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_AIN, 0x01},      /*"=0001： 选择为模拟输入口 AIN2/CMP1"*/
    {_IA_IN, 0x02},    /*"=0010： 选择为 IA_IN"*/
    {_IB_IN, 0x03},    /*"=0011： 选择为 IB_IN"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_AIN, 0x01},      /*"=0001： 选择为模拟输入口 AIN2/CMP1"*/
#endif
};
/*"***************************** P0_3 *****************************"*/
static const sGPIOFunReg_TypeDef P0_3_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_IOCNT[2], PCA0[29], PCA0[7], PCA0[6]"*/
    {GPIO_PCA0, _bit6},        /*"Bit0"*/
    {GPIO_PCA0, _bit7},        /*"Bit1"*/
    {GPIO_PCA0, _bit29},       /*"Bit2"*/
    {GPIO_IOCFG_IOCNT, _bit2}, /*"Bit3"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA2[9:7],PCA0[5]}定义端口 P03 的复用配置："*/
    {GPIO_PCA0, _bit5}, /*"Bit0"*/
    {GPIO_PCA2, _bit7}, /*"Bit1"*/
    {GPIO_PCA2, _bit8}, /*"Bit2"*/
    {GPIO_PCA2, _bit9}, /*"Bit3"*/
#else
    /*"由{PCA2[9:7],PCA0[5]}定义端口 P03 的复用配置："*/
    {GPIO_PCA0, _bit5}, /*"Bit0"*/
#endif
};
static const sGPIOFunCll_TypeDef P0_3_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /*"=0000: 选择为IO口；"*/
    {_CF_OUT1, 0x01},    /*"=0001: 选择为CF1"*/
    {_CF1, 0x01},        /*"=0001: 选择为CF1"*/
    {_IOCNT_OUT1, 0x09}, /*"=1001: 选择为IOCNT1"*/
    {_TC0_P_0, 0x02},    /*"=0010: 选择为TC输出TC0_P[0]"*/
    {_TC_IN, 0x03},      /*"=0011: 选择为TC输入TCI"*/
    {_RTC_OUT, 0x04},    /*"=0100: 选择为RTCOUT"*/
    {_RTC_1S, 0x05},     /*"=0101: 选择为RTC_1S"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口 "*/
    {_AIN, 0x01},      /*"=0001： 选择为模拟输入口 AIN3/CMP2"*/
    {_IA_IN, 0x02},    /*"=0010： 选择为 IA_IN"*/
    {_U_IN, 0x03},     /*"=0011： 选择为 U_IN"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口 "*/
    {_AIN, 0x01},      /*"=0001： 选择为模拟输入口 AIN3/CMP2"*/
#endif
};
/*"***************************** P0_4 *****************************"*/
static const sGPIOFunReg_TypeDef P0_4_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[1], IOCFG_UART_I2C[0], PCA0[31], PCA0[30], PCA0[8]"*/
    {GPIO_PCA0, _bit8},           /*"Bit0"*/
    {GPIO_PCA0, _bit30},          /*"Bit1"*/
    {GPIO_PCA0, _bit31},          /*"Bit2"*/
    {GPIO_IOCFG_UART_I2C, _bit0}, /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit1}, /*"Bit5"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA2[11:10],PCA0[8],PCA0[6]} 定义端口 P04 复用配置："*/
    {GPIO_PCA0, _bit6},  /*"Bit0"*/
    {GPIO_PCA0, _bit8},  /*"Bit1"*/
    {GPIO_PCA2, _bit10}, /*"Bit2"*/
    {GPIO_PCA2, _bit11}, /*"Bit3"*/
#else
    /*"由{PCA2[11:10],PCA0[8],PCA0[6]} 定义端口 P04 复用配置："*/
    {GPIO_PCA0, _bit6}, /*"Bit0"*/
    {GPIO_PCA0, _bit8}, /*"Bit1"*/
#endif
};
static const sGPIOFunCll_TypeDef P0_4_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /*"=00000: 都选择为IO口；"*/
    {_UART0, 0x01},    /*"=00001: 选择为UART0，P04为RX0，P05为TX0	""*/
    {_LPUART, 0x09},   /*"=01001: 选择为LPUART，P04为RX0，P05为TX0	""*/
    {_CAN, 0x11},      /*"=10001: 选择为CAN，P04为RX0，P05为TX0	""*/
    {_UART2, 0x02},    /*"=00010: 选择为UART2，P04为RX2，P05为TX2"*/
    {_IIC0, 0x06},     /*"=00110: P04和P05为I2C0接口，P04为SCL0，P05为SDA0"*/
    {_IIC1, 0x07},     /*"=00111: P04和P05为I2C1接口，P04为SCL1，P05为SDA1"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_AIN, 0x01},      /*"=0001： 选择为模拟输入口 AIN4/LVDIN"*/
    {_KEY, 0x02},      /*"=001x： P04 选择为 KEY4"*/
    {_IA_IN, 0x04},    /*"=0100： 选择为 IA_IN"*/
    {_IB_IN, 0x05},    /*"=0101： 选择为 IB_IN"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_AIN, 0x01},      /*"=0001： 选择为模拟输入口 AIN4/LVDIN"*/
    {_KEY, 0x02},      /*"=001x： P04 选择为 KEY4"*/
/*"注： 当 P14 和 P04 都选择为 KEY4 时， P14 优先级高于 P04， P04输入无效"*/
#endif
};
/*"***************************** P0_5 *****************************"*/
static const sGPIOFunReg_TypeDef P0_5_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[1], IOCFG_UART_I2C[0], PCA0[31], PCA0[30], PCA0[8]"*/
    {GPIO_PCA0, _bit8},           /*"Bit0"*/
    {GPIO_PCA0, _bit30},          /*"Bit1"*/
    {GPIO_PCA0, _bit31},          /*"Bit2"*/
    {GPIO_IOCFG_UART_I2C, _bit0}, /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit1}, /*"Bit5"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA2[14:12],PCA0[7]}定义端口 P05 的复用配置："*/
    {GPIO_PCA0, _bit7},  /*"Bit0"*/
    {GPIO_PCA2, _bit12}, /*"Bit1"*/
    {GPIO_PCA2, _bit13}, /*"Bit2"*/
    {GPIO_PCA2, _bit14}, /*"Bit3"*/
#else
    /*"由{PCA2[14:12],PCA0[7]}定义端口 P05 的复用配置："*/
    {GPIO_PCA0, _bit7}, /*"Bit0"*/
#endif
};
static const sGPIOFunCll_TypeDef P0_5_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /*"=00000: 都选择为IO口；"*/
    {_UART0, 0x01},    /*"=00001: 选择为UART0，P04为RX0，P05为TX0	""*/
    {_LPUART, 0x09},   /*"=01001: 选择为LPUART，P04为RX0，P05为TX0	""*/
    {_CAN, 0x11},      /*"=10001: 选择为CAN，P04为RX0，P05为TX0	""*/
    {_UART2, 0x02},    /*"=00010: 选择为UART2，P04为RX2，P05为TX2"*/
    {_IIC0, 0x06},     /*"=00110: P04和P05为I2C0接口，P04为SCL0，P05为SDA0"*/
    {_IIC1, 0x07},     /*"=00111: P04和P05为I2C1接口，P04为SCL1，P05为SDA1"*/
#else
    {_NORMALIO, 0x00}, /*"保留"*/
    {_AIN, 0x01},      /*"=1： 选择为模拟输入口 "*/
#endif
};
/*"***************************** P0_6 *****************************"*/
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P0_6_FunReg[] = {
    /*"PCA0[10], PCA0[9]"*/
    {GPIO_PCA0, _bit9},  /*"Bit0"*/
    {GPIO_PCA0, _bit10}, /*"Bit1"*/
};
static const sGPIOFunCll_TypeDef P0_6_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0000: 选择为IO口；"*/
    {_ISO7816, 0x01},  /*"=0001: 选择为7816CLK"*/
    {_INT, 0x02},      /*"=0010:  选择为INT1"*/
    {_SPI, 0x03},      /*"=0011: 选择为SPI0/1/2的SCSN；"*/
};
#endif
/*"***************************** P0_7 *****************************"*/
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P0_7_FunReg[] = {
    /*"PCA0[12], PCA0[11]"*/
    {GPIO_PCA0, _bit11}, /*"Bit0"*/
    {GPIO_PCA0, _bit12}, /*"Bit1"*/
};
static const sGPIOFunCll_TypeDef P0_7_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0000: 选择为IO口；"*/
    {_ISO7816, 0x01},  /*"=0001: 选择为7816CLK"*/
    {_KEY, 0x02},      /*"=0010:  选择为INT1"*/
    {_SPI, 0x03},      /*"=0011: 选择为SPI0/1/2的SCSN；"*/
};
#endif
/*"***************************** P1_0 *****************************"*/
static const sGPIOFunReg_TypeDef P1_0_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCA0[14], PCA0[13]"*/
    {GPIO_PCA0, _bit13}, /*"Bit0"*/
    {GPIO_PCA0, _bit14}, /*"Bit1"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /*"由{PCA2[16:15],PCA0[10:9]}定义端口 P10 复用配置："*/
    {GPIO_PCA0, _bit9},  /*"Bit0"*/
    {GPIO_PCA0, _bit10}, /*"Bit1"*/
    {GPIO_PCA2, _bit15}, /*"Bit2"*/
    {GPIO_PCA2, _bit16}, /*"Bit3"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA2[16:15],PCA0[10:9]}定义端口 P10 复用配置："*/
    {GPIO_PCA0, _bit9},  /*"Bit0"*/
    {GPIO_PCA0, _bit10}, /*"Bit1"*/
    {GPIO_PCA2, _bit15}, /*"Bit2"*/
    {GPIO_PCA2, _bit16}, /*"Bit3"*/

#else
    /*"由{PCA2[16:15],PCA0[10:9]}定义端口 P10 复用配置："*/
    {GPIO_PCA0, _bit9},  /*"Bit0"*/
    {GPIO_PCA0, _bit10}, /*"Bit1"*/
#endif
};
static const sGPIOFunCll_TypeDef P1_0_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /*"=0000: 选择为IO口；"*/
    {_ISO7816, 0x01},  /*"=0001: 选择为78161_IO"*/
    {_INT, 0x02},      /*"=0010:  选择为INT2"*/
    {_SPI, 0x03},      /*"=0011: 选择为SPI0/1/2的MOSI0/1或SDI2；"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},        /*"=0001： 选择为 KEY0"*/
    {_TC0_N_0, 0x02},    /*"=0010： 选择为 TC0_N[0]"*/
    {_TC_IN, 0x03},      /*"=0011： 选择为 TC 输入"*/
    {_D2F_OUT0, 0x04},   /*"=0100：  选择为 D2F_OUT0"*/
    {_IOCNT_OUT1, 0x05}, /*"=0101： 选择为 IOCNT_OUT1"*/
    {_INT, 0x06},        /*"=0110： 选择为 INT0 （优先级 P30＞P10＞P74）"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},        /*"=0001： 选择为 KEY0"*/
    {_TC0_N_0, 0x02},    /*"=0010： 选择为 TC0_N[0]"*/
    {_TC_IN, 0x03},      /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
    {_IA_IN, 0x04},      /*"=0100： 选择为 IA_IN"*/
    {_IB_IN, 0x05},      /*"=0101： 选择为 IB_IN"*/
    {_CF_OUT2, 0x06},    /*"=0110： 选择为 CF_OUT2"*/
    {_D2F_OUT2, 0x07},   /*"=0111： 选择为 D2F_OUT2"*/
    {_IOCNT_OUT2, 0x08}, /*"=1000： 选择为 IOCNT_OUT2"*/
    {_TC1_N_1, 0x09},    /*"=1001： 选择为 TC1_N[1](智能微断需求)"*/
                         /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /*"=0001： 选择为 KEY0"*/
    {_TC0_N_0, 0x02},  /*"=0010： 选择为 TC0_N[0]"*/
    {_TC_IN, 0x03},    /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
#endif
};
/*"***************************** P1_1 *****************************"*/
static const sGPIOFunReg_TypeDef P1_1_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCA0[16], PCA0[15]"*/
    {GPIO_PCA0, _bit15}, /*"Bit0"*/
    {GPIO_PCA0, _bit16}, /*"Bit1"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /*"由{PCA2[18:17],PCA0[12:11]}定义端口 P11 复用配置："*/
    {GPIO_PCA0, _bit11}, /*"Bit0"*/
    {GPIO_PCA0, _bit12}, /*"Bit1"*/
    {GPIO_PCA2, _bit17}, /*"Bit2"*/
    {GPIO_PCA2, _bit18}, /*"Bit3"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA2[18:17],PCA0[12:11]}定义端口 P11 复用配置："*/
    {GPIO_PCA0, _bit11}, /*"Bit0"*/
    {GPIO_PCA0, _bit12}, /*"Bit1"*/
    {GPIO_PCA2, _bit17}, /*"Bit2"*/
    {GPIO_PCA2, _bit18}, /*"Bit3"*/
#else
    /*"由{PCA2[18:17],PCA0[12:11]}定义端口 P11 复用配置："*/
    {GPIO_PCA0, _bit11}, /*"Bit0"*/
    {GPIO_PCA0, _bit12}, /*"Bit1"*/
#endif
};
static const sGPIOFunCll_TypeDef P1_1_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /*"=0000:  选择为IO口；"*/
    {_ISO7816, 0x01},  /*"=0001:  选择为78161_I"*/
    {_KEY, 0x02},      /*"=0010:  选择为KEY2"*/
    {_SPI, 0x03},      /*"=0011:  选择为SPI0/1/2的MISO0/1或SDO2；"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},        /*"=0001： 选择为 KEY1(优先级 P11＞P85）"*/
    {_TC0_P_0, 0x02},    /*"=0010： 选择为 TC0_P[0]"*/
    {_TC_IN, 0x03},      /*"=0011： 选择为 TC 输入"*/
    {_D2F_OUT1, 0x04},   /* =0100： 选择为 D2F_OUT1 */
    {_IOCNT_OUT2, 0x05}, /* =0101： 选择为 IOCNT_OUT2 */
    {_INT, 0x06},        /* =0110： 选择为 INT1 （优先级 P40＞P11＞P75） */
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},        /*"=0001： 选择为 KEY1"*/
    {_TC0_P_0, 0x02},    /*"=0010： 选择为 TC0_P[0]"*/
    {_TC_IN, 0x03},      /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
    {_IA_IN, 0x04},      /*"=0100： 选择为 IA_IN"*/
    {_U_IN, 0x05},       /*"=0101： 选择为 U_IN"*/
    {_CF_OUT3, 0x06},    /*"=0110： 选择为 CF_OUT3"*/
    {_D2F_OUT3, 0x07},   /*"=0111： 选择为 D2F_OUT3"*/
    {_IOCNT_OUT3, 0x08}, /*"=1000： 选择为 IOCNT_OUT3"*/
    {_TC1_P_1, 0x09},    /*"=1001： 选择为 TC1_P[1](智能微断需求)"*/
                         /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /*"=0001： 选择为 KEY1"*/
    {_TC0_P_0, 0x02},  /*"=0010： 选择为 TC0_P[0]"*/
    {_TC_IN, 0x03},    /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
#endif
};
/*"***************************** P1_2 *****************************"*/
static const sGPIOFunReg_TypeDef P1_2_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCA0[18], PCA0[17]"*/
    {GPIO_PCA0, _bit17}, /*"Bit0"*/
    {GPIO_PCA0, _bit18}, /*"Bit1"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA2[20:19],PCA0[14:13]}定义端口 P12 复用配置："*/
    {GPIO_PCA0, _bit13}, /*"Bit0"*/
    {GPIO_PCA0, _bit14}, /*"Bit1"*/
    {GPIO_PCA2, _bit19}, /*"Bit2"*/
    {GPIO_PCA2, _bit20}, /*"Bit3"*/
#else
    /*"由{PCA2[20:19],PCA0[14:13]}定义端口 P12 复用配置："*/
    {GPIO_PCA0, _bit13}, /*"Bit0"*/
    {GPIO_PCA0, _bit14}, /*"Bit1"*/
#endif
};
static const sGPIOFunCll_TypeDef P1_2_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /*"=0000: 选择为IO口；"*/
    {_SPI, 0x01},      /*"=00001:  选择为SPI0/1/2的SCSN；仅在SPI从模式有效。"*/
    {_TC0_P_0, 0x02},  /*"=0010:   选择为TC输出TC0_P[0]，"*/
    {_TC_IN, 0x03},    /*"=0011:  选择为TC输入；"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},        /*"=0001： 选择为 KEY2"*/
    {_TC0_N_1, 0x02},    /*"=0010： 选择为 TC0_N[1]]"*/
    {_TC_IN, 0x03},      /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
    {_IB_IN, 0x04},      /*"=0100： 选择为 IB_IN"*/
    {_U_IN, 0x05},       /*"=0101： 选择为 U_IN"*/
    {_CF_OUT4, 0x06},    /*"=0110： 选择为 CF_OUT4"*/
    {_D2F_OUT4, 0x07},   /*"=0111： 选择为 D2F_OUT4"*/
    {_IOCNT_OUT4, 0x08}, /*"=1000： 选择为 IOCNT_OUT4"*/
    {_TRIG_OUT, 0x09},   /*"=1001： 选择为 TRIG_OUT(智能微断需求)"*/
                         /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /*"=0001： 选择为 KEY2"*/
    {_TC0_N_1, 0x02},  /*"=0010： 选择为 TC0_N[1]]"*/
    {_TC_IN, 0x03},    /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
#endif
};
/*"***************************** P1_3 *****************************"*/
static const sGPIOFunReg_TypeDef P1_3_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCA0[20], PCA0[19]"*/
    {GPIO_PCA0, _bit19}, /*"Bit0"*/
    {GPIO_PCA0, _bit20}, /*"Bit1"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA2[22:21],PCA0[16:15]}定义端口 P13 复用配置："*/
    {GPIO_PCA0, _bit15}, /*"Bit0"*/
    {GPIO_PCA0, _bit16}, /*"Bit1"*/
    {GPIO_PCA2, _bit21}, /*"Bit2"*/
    {GPIO_PCA2, _bit22}, /*"Bit3"*/
#else
    /*"由{PCA2[22:21],PCA0[16:15]}定义端口 P13 复用配置："*/
    {GPIO_PCA0, _bit15}, /*"Bit0"*/
    {GPIO_PCA0, _bit16}, /*"Bit1"*/
#endif
};
static const sGPIOFunCll_TypeDef P1_3_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /*"=0000:选择为IO口；"*/
    {_SPI, 0x01},      /*"=0001:选择为SPI0的SCLK0或SPI2的SCLK2。"*/
    {_TC0_N_0, 0x02},  /*"=0010: 选择为TC输出TC0_N[0]"*/
    {_TC_IN, 0x03},    /*"=0011:选择为TC输入；"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},        /*"=0001： 选择为 KEY3"*/
    {_TC0_P_1, 0x02},    /*"=0010： 选择为 TC0_P[1]"*/
    {_TC_IN, 0x03},      /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
    {_IOCNT_OUT2, 0x04}, /*"=0100： 选择为 IOCNT_OUT2"*/
    {_ADC_CLKO, 0x05},   /*"=0101： 选择为 ADC_CLKO"*/
    {_IA_IN, 0x06},      /*"=0110： 选择为 IA_IN"*/
    {_U_IN, 0x07},       /*"=0111： 选择为 U_IN"*/
                         /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /*"=0001： 选择为 KEY3"*/
    {_TC0_P_1, 0x02},  /*"=0010： 选择为 TC0_P[1]"*/
    {_TC_IN, 0x03},    /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
#endif
};
/*"***************************** P1_4 *****************************"*/
static const sGPIOFunReg_TypeDef P1_4_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCA0[22], PCA0[21]"*/
    {GPIO_PCA0, _bit21}, /*"Bit0"*/
    {GPIO_PCA0, _bit22}, /*"Bit1"*/
#else
    /*"由{PCA2[24:23],PCA0[18:17]}定义端口 P14 复用配置："*/
    {GPIO_PCA0, _bit17}, /*"Bit0"*/
    {GPIO_PCA0, _bit18}, /*"Bit1"*/
#ifdef RN821x_RN721x_SOC_V2
    {GPIO_PCA2, _bit23}, /*"Bit2"*/
    {GPIO_PCA2, _bit24}, /*"Bit3"*/
#endif
#endif
};
static const sGPIOFunCll_TypeDef P1_4_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /*"=0000:选择为IO口；"*/
    {_SPI, 0x01},      /*"=0001:选择为SPI0的MOSI0或SPI2的SDI2；"*/
    {_TC0_P_1, 0x02},  /*"=0010: 选择为TC输出TC0_P[1]"*/
    {_TC_IN, 0x03},    /*"=0011:选择为TC输入；"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /*"=0001： 选择为 KEY4"*/
    {_TC1_N_0, 0x02},  /*"=0010： 选择为 TC1_N[0]"*/
    {_TC_IN, 0x03},    /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
    {_SPI3, 0x04},     /*"=0100： 选择为 SPI3_SCSN"*/
    {_INT, 0x05},      /*"=0101： 选择为 INT6"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /*"=0001： 选择为 KEY4"*/
    {_TC1_N_0, 0x02},  /*"=0010： 选择为 TC1_N[0]"*/
    {_TC_IN, 0x03},    /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
/*"注： 当 P36 和 P14 都选择为 INT7 时， P36 优先级高于 P14， P14输入无效"*/
#endif
};
/*"***************************** P1_5 *****************************"*/
static const sGPIOFunReg_TypeDef P1_5_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCA0[24], PCA0[23]"*/
    {GPIO_PCA0, _bit23}, /*"Bit0"*/
    {GPIO_PCA0, _bit24}, /*"Bit1"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA2[26:25],PCA0[20:19]}定义端口 P15 复用配置："*/
    {GPIO_PCA0, _bit19}, /*"Bit0"*/
    {GPIO_PCA0, _bit20}, /*"Bit1"*/
    {GPIO_PCA2, _bit25}, /*"Bit2"*/
    {GPIO_PCA2, _bit26}, /*"Bit3"*/
#else
    /*"由{PCA2[26:25],PCA0[20:19]}定义端口 P15 复用配置："*/
    {GPIO_PCA0, _bit19}, /*"Bit0"*/
    {GPIO_PCA0, _bit20}, /*"Bit1"*/
#endif
};
static const sGPIOFunCll_TypeDef P1_5_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /*"=0000:选择为IO口；"*/
    {_SPI, 0x01},      /*"=0001:选择为MISO0/1或SPI2的SDO2；"*/
    {_TC0_N_1, 0x02},  /*"=0010: 选择为TC输出TC0_N[1]"*/
    {_TC_IN, 0x03},    /*"=0011:选择为TC输入；"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /*"=0001： 选择为 KEY5"*/
    {_TC1_P_0, 0x02},  /*"=0010： 选择为 TC1_P[0]"*/
    {_TC_IN, 0x03},    /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
    {_SPI3, 0x04},     /*"=0100： 选择为 SPI3_SCLK"*/
    {_INT, 0x05},      /*"=0101： 选择为 INT7"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /*"=0001： 选择为 KEY5"*/
    {_TC1_P_0, 0x02},  /*"=0010： 选择为 TC1_P[0]"*/
    {_TC_IN, 0x03},    /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
/*"注： 当 P37 和 P15 都选择为 INT7 时， P37 优先级高于 P15， P15输入无效"*/
#endif
};
/*"***************************** P1_6 *****************************"*/
static const sGPIOFunReg_TypeDef P1_6_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCA0[26], PCA0[25]"*/
    {GPIO_PCA0, _bit25}, /*"Bit0"*/
    {GPIO_PCA0, _bit26}, /*"Bit1"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA2[28:27],PCA0[22:21]}定义端口 P16 复用配置："*/
    {GPIO_PCA0, _bit21}, /*"Bit0"*/
    {GPIO_PCA0, _bit22}, /*"Bit1"*/
    {GPIO_PCA2, _bit27}, /*"Bit2"*/
    {GPIO_PCA2, _bit28}, /*"Bit3"*/
#else
    /*"由{PCA2[28:27],PCA0[22:21]}定义端口 P16 复用配置："*/
    {GPIO_PCA0, _bit21}, /*"Bit0"*/
    {GPIO_PCA0, _bit22}, /*"Bit1"*/
#endif
};
static const sGPIOFunCll_TypeDef P1_6_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /*"=0000:选择为IO口；"*/
    {_INT, 0x01},      /*"=0001:选择为INT3；"*/
    {_HOSCO, 0x02},    /*"=0010:选择为晶体HOSCO管脚；"*/
    {_SPI3, 0x03},     /*"=0011:SCSN3"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /*"=0001： 选择为 KEY6"*/
    {_TC1_N_1, 0x02},  /*"=0010： 选择为 TC1_N[1]"*/
    {_TC_IN, 0x03},    /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
    {_SPI3, 0x04},     /*"=0100： 选择为 SPI3_MISO"*/
    {_TRIG_OUT, 0x05}, /*"=0101： 选择为 TRIG_OUT(智能微断需求)"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /*"=0001： 选择为 KEY6"*/
    {_TC1_N_1, 0x02},  /*"=0010： 选择为 TC1_N[1]"*/
    {_TC_IN, 0x03},    /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
#endif
};
/*"***************************** P1_7 *****************************"*/
static const sGPIOFunReg_TypeDef P1_7_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCA0[28], PCA0[27]"*/
    {GPIO_PCA0, _bit27}, /*"Bit0"*/
    {GPIO_PCA0, _bit28}, /*"Bit1"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA2[30:29],PCA0[24:23]}定义端口 P17 复用配置："*/
    {GPIO_PCA0, _bit23}, /*"Bit0"*/
    {GPIO_PCA0, _bit24}, /*"Bit1"*/
    {GPIO_PCA2, _bit29}, /*"Bit2"*/
    {GPIO_PCA2, _bit30}, /*"Bit3"*/
#else
    /*"由{PCA2[30:29],PCA0[24:23]}定义端口 P17 复用配置："*/
    {GPIO_PCA0, _bit23}, /*"Bit0"*/
    {GPIO_PCA0, _bit24}, /*"Bit1"*/
#endif
};
static const sGPIOFunCll_TypeDef P1_7_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /*"=0000:选择为IO口；"*/
    {_INT, 0x01},      /*"=0001:选择为INT4；"*/
    {_HOSCI, 0x02},    /*"=0010:选择为晶体HOSCI管脚；"*/
    {_SPI3, 0x03},     /*"=0011:选择为SCLK3"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /*"=0001： 选择为 KEY7"*/
    {_TC1_P_1, 0x02},  /*"=0010： 选择为 TC1_P[1]"*/
    {_TC_IN, 0x03},    /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/
    {_SPI3, 0x04},     /*"=0100： 选择为 SPI3_MOSI"*/
    {_TRIG_OUT, 0x05}, /*"=0101： 选择为 TRIG_OUT(智能微断需求)"*/
    {_AIN, 0x06},      /*"=0110： 选择为模拟输入口 AIN5"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /*"=0001： 选择为 KEY7"*/
    {_TC1_P_1, 0x02},  /*"=0010： 选择为 TC1_P[1]"*/
    {_TC_IN, 0x03},    /*"=0011： 选择为 TC 输入， 对 TC0/TC1 可同时有效"*/

#endif
};
/*"***************************** P2_0 *****************************"*/
static const sGPIOFunReg_TypeDef P2_0_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[2], IOCFG_IOCNT[3], PCA2[1], PCA2[0], PCA1[1], PCA1[0]]"*/
    {GPIO_PCA1, _bit0},           /*"Bit0"*/
    {GPIO_PCA1, _bit1},           /*"Bit1"*/
    {GPIO_PCA2, _bit0},           /*"Bit2"*/
    {GPIO_PCA2, _bit1},           /*"Bit3"*/
    {GPIO_IOCFG_IOCNT, _bit3},    /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit2}, /*"Bit5"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* {PCA3[28]_PCA0[31:30]_PCA3[6]_PCA3[2:0]_PCA0[25]}定义 P20 口的复用配置 */
    {GPIO_PCA3, _bit0}, /*"Bit0"*/
    {GPIO_PCA3, _bit1}, /*"Bit1"*/
    {GPIO_PCA3, _bit2}, /*"Bit2"*/
    {GPIO_PCA3, _bit6}, /*"Bit3"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA3[6],PCA3[2:0],PCA0[25]}定义端口 P20 复用配置："*/
    {GPIO_PCA0, _bit25}, /*"Bit0"*/
    {GPIO_PCA3, _bit0}, /*"Bit1"*/
    {GPIO_PCA3, _bit1}, /*"Bit2"*/
    {GPIO_PCA3, _bit2}, /*"Bit3"*/
    {GPIO_PCA3, _bit6}, /*"Bit4"*/
#else
    /*"由{PCA3[6],PCA3[2:0],PCA0[25]}定义端口 P20 复用配置："*/
    {GPIO_PCA0, _bit25}, /*"Bit0"*/
#endif
};
static const sGPIOFunCll_TypeDef P2_0_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /*"=000000:选择为IO口；"*/
    {_UART1, 0x01},      /*"=000001:选择为UART1接口；"*/
    {_LPUART, 0x21},     /*"=100001:选择为LPUART接口；"*/
    {_SWD, 0x02},        /*"=000010:选择为SWD，默认配置；"*/
    {_UART2, 0x03},      /*"=000011: 选择为UART2接口；"*/
    {_CF_OUT3, 0x04},    /*"=000100:选择为CF3"*/
    {_CF3, 0x04},        /*"=000100:选择为CF3"*/
    {_IOCNT_OUT3, 0x14}, /*"=010100:选择为IOCNT3"*/
    {_RTC_1S, 0x08},     /*"=001000:：选择为RTC_1S"*/
    {_TC0_P_1, 0x0C},    /*"=001100:选择为TC0_P[1]"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x08}, /*"=x_xx_1_000_x： 选择为 IO 口"*/
    {_UART0, 0x09},    /* x_xx_1_001_x： 选择为 RX0 */
    {_UART2, 0x0A},    /* x_xx_1_010_x： 选择为 RX2 */
    {_TC0_N_0, 0x0B},  /* x_xx_1_011_x： 选择为 TC0_N[0] */
    {_SPI0, 0x0B},     /* x_xx_1_110_x： 选择为 SPI0_SCSN */
    {_SPI1, 0x0B},     /* x_xx_1_111_x： 选择为 SPI1_SCSN */
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART0, 0x12},    /*"=10010： 选择为 RX0"*/
    {_UART2, 0x14},    /*"=10100： 选择为 RX2"*/
    {_SPI1, 0x16},     /*"=10110： 选择为 SPI1_SCSN"*/
    {_SPI3, 0x18},     /*"=11000： 选择为 SPI3_SCSN"*/
    {_CAN_RX, 0x1A},   /*"=11010： 选择为 CAN_RX"*/
    {_TC1_N_1, 0x1C},  /*"=11100： 选择为 TC1_N[1](智能微断需求)"*/
    {_TRIG_OUT, 0x1E}, /*"=11110： 选择为 TRIG_OUT(智能微断需求)"*/
#else
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART0, 0x01},    /*"=00001： 选择为 UART0 接口， P20 为 RX0， P21 为 TX0"*/
/*" 注1：PCA0[25],PCA3[6]都会同时影响P20 P21
    注2：当 PCA3 寄存器的 bit[6]=0 时， 该配置有效；
        当 PCA3 寄存器的 bit[6]=1 时， 由 PCA3 寄存器的 bit[5:0]决定 P20/P21的复用。"*/
#endif
};
/*"***************************** P2_1 *****************************"*/
static const sGPIOFunReg_TypeDef P2_1_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[2], IOCFG_IOCNT[4], PCA2[3], PCA2[2], PCA1[1], PCA1[0]"*/
    {GPIO_PCA1, _bit0},           /*"Bit0"*/
    {GPIO_PCA1, _bit1},           /*"Bit1"*/
    {GPIO_PCA2, _bit2},           /*"Bit2"*/
    {GPIO_PCA2, _bit3},           /*"Bit3"*/
    {GPIO_IOCFG_IOCNT, _bit4},    /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit2}, /*"Bit5"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由 {PCA3[28]_PCA0[31:30]_PCA3[6]_PCA3[5:3]_PCA0[25]} 定义 P21 口的复用配置： */
    {GPIO_PCA3, _bit3}, /*"Bit0"*/
    {GPIO_PCA3, _bit4}, /*"Bit1"*/
    {GPIO_PCA3, _bit5}, /*"Bit2"*/
    {GPIO_PCA3, _bit6}, /*"Bit3"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA3[6],PCA3[5:3],PCA0[25]}定义端口 P21 复用配置："*/
    {GPIO_PCA0, _bit25}, /*"Bit0"*/
    {GPIO_PCA3, _bit3}, /*"Bit1"*/
    {GPIO_PCA3, _bit4}, /*"Bit2"*/
    {GPIO_PCA3, _bit5}, /*"Bit3"*/
    {GPIO_PCA3, _bit6}, /*"Bit4"*/
#else
    /*"由{PCA3[6],PCA3[5:3],PCA0[25]}定义端口 P21 复用配置："*/
    {GPIO_PCA0, _bit25}, /*"Bit0"*/
#endif
};
static const sGPIOFunCll_TypeDef P2_1_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /*"=000000:选择为IO口；"*/
    {_UART1, 0x01},      /*"=000001:选择为UART1"*/
    {_LPUART, 0x21},     /*"=100001:选择为LPUART接口；"*/
    {_SWD, 0x02},        /*"=000010:选择为SWD，默认配置；"*/
    {_UART2, 0x03},      /*"=000011: 选择为UART2"*/
    {_CF_OUT2, 0x04},    /*"=000100:选择为CF2"*/
    {_CF2, 0x04},        /*"=000100:选择为CF2"*/
    {_IOCNT_OUT2, 0x14}, /*"=010100:选择为IOCNT2"*/
    {_RTC_OUT, 0x08},    /*"=001000:选择为RTC_OUT"*/
    {_TC0_P_0, 0x0C},    /*"=001100:选择为TC0_P[0]"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x08}, /*"=x_xx_1_000_x： 选择为 IO 口"*/
    {_UART0, 0x09},    /* x_xx_1_001_x： 选择为 TX0 */
    {_UART2, 0x0A},    /* x_xx_1_010_x： 选择为 TX2 */
    {_TC0_P_0, 0x0B},  /* x_xx_1_011_x： 选择为 TC0_P[0] */
    {_TC_IN, 0x0C},    /* x_xx_1_100_x： 选择为 TCIN */
    {_INT, 0x0D},      /* x_xx_1_101_x： 选择为 INT6（优先级 P36＞P21＞P82） */
    {_SPI0, 0x0E},     /* x_xx_1_110_x： 选择为 SPI0_SCLK */
    {_SPI1, 0x0F},     /* x_xx_1_111_x： 选择为 SPI1_SCLK */
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART0, 0x12},    /*"=10010： 选择为 TX0"*/
    {_UART2, 0x14},    /*"=10100： 选择为 TX2"*/
    {_SPI1, 0x16},     /*"=10110： 选择为 SPI1_SCLK"*/
    {_SPI3, 0x18},     /*"=11000： 选择为 SPI3_SCLK"*/
    {_CAN_TX, 0x1A},   /*"=11010： 选择为 CAN_TX"*/
    {_TC1_P_1, 0x1C},  /*"=11100： 选择为 TC1_P[1](智能微断需求)"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART0, 0x01},    /*"=00001： 选择为 UART0 接口， P20 为 RX0， P21 为 TX0"*/
/*" 注1：PCA0[25],PCA3[6]都会同时影响P20 P21
    注2：当 PCA3 寄存器的 bit[6]=0 时， 该配置有效；
        当 PCA3 寄存器的 bit[6]=1 时， 由 PCA3 寄存器的 bit[5:0]决定 P20/P21的复用。"*/
#endif
};
/*"***************************** P2_2 *****************************"*/
static const sGPIOFunReg_TypeDef P2_2_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[3], IOCFG_IOCNT[6], PCA2[5], PCA2[4], PCA1[4], PCA1[2]"*/
    {GPIO_PCA1, _bit2},           /*"Bit0"*/
    {GPIO_PCA1, _bit4},           /*"Bit1"*/
    {GPIO_PCA2, _bit4},           /*"Bit2"*/
    {GPIO_PCA2, _bit5},           /*"Bit3"*/
    {GPIO_IOCFG_IOCNT, _bit6},    /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit3}, /*"Bit5"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由 {PCA3[28]_PCA0[31:30]_PCA3[13]_PCA3[9:7]_PCA1[16]_PCA0[26]}定义 P22 口的复用配置： */
    {GPIO_PCA3, _bit7},  /*"Bit0"*/
    {GPIO_PCA3, _bit8},  /*"Bit1"*/
    {GPIO_PCA3, _bit9},  /*"Bit2"*/
    {GPIO_PCA3, _bit13}, /*"Bit3"*/
#elif defined(RN831x_RN861x_MCU_V2)
    /*"由{PCA3[13],PCA3[9:7],PCA0[26]}定义端口 P22 复用配置："*/
    {GPIO_PCA0, _bit26}, /*"Bit0"*/
    {GPIO_PCA1, _bit16}, /*"Bit1"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA3[13],PCA3[9:7],PCA0[26]}定义端口 P22 复用配置："*/
    {GPIO_PCA0, _bit26}, /*"Bit0"*/
    {GPIO_PCA3, _bit7},  /*"Bit1"*/
    {GPIO_PCA3, _bit8},  /*"Bit2"*/
    {GPIO_PCA3, _bit9},  /*"Bit3"*/
    {GPIO_PCA3, _bit13}, /*"Bit4"*/
#else
    /*"由{PCA3[13],PCA3[9:7],PCA0[26]}定义端口 P22 复用配置："*/
    {GPIO_PCA0, _bit26}, /*"Bit0"*/
#endif
};
static const sGPIOFunCll_TypeDef P2_2_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /*"=000000:  选择为IO口；"*/
    {_KEY, 0x01},        /*"=000001: 选择为KEY4"*/
    {_UART2, 0x02},      /*"=000010: P22/P23配置为UART2,P22为RX2，P23为TX2;"*/
    {_LPUART, 0x22},     /*"=100010: P22/P23配置为LPUART,P22为RX2，P23为TX2;"*/
    {_SPI3, 0x04},       /*"=000100: ：选择为MOSI3备注：P16、P17、P22、P23为一组，组成SPI3。"*/
    {_TC0_N_0, 0x08},    /*"=001000: 选择为TC1_N[0]"*/
    {_CF_OUT4, 0x0C},    /*"=001100:选择为CF4"*/
    {_CF4, 0x0C},        /*"=001100:选择为CF4"*/
    {_IOCNT_OUT4, 0x1C}, /*"=011100:选择为IOCNT4"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x08}, /*"=x_xx_1_000_x_x： 选择为 IO 口"*/
    {_UART1, 0x09},    /* x_xx_1_001_x_x： 选择为 RX1 */
    {_UART2, 0x0A},    /* x_xx_1_010_x_x： 选择为 RX2 */
    {_TC0_N_1, 0x0B},  /* x_xx_1_011_x_x： 选择为 TC0_N[1] */
    {_SPI0, 0x0E},     /* x_xx_1_110_x_x： 选择为 SPI0_MISO */
    {_SPI1, 0x0F},     /* x_xx_1_111_x_x： 选择为 SPI1_MISO */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART1, 0x01},    /*"=00001： 选择为 UART1 接口， P22 为 RX1， P23 为 TX1"*/
    {_UART2, 0x02},    /*"=00010： 选择为 UART2 接口， P22 为 RX2， P23 为 TX2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART1, 0X12},    /*"=10010： 选择为 RX1"*/
    {_UART2, 0x14},    /*"=10100： 选择为 RX2"*/
    {_SPI1, 0x16},     /*"=10110： 选择为 SPI1_MISO"*/
    {_SPI3, 0x18},     /*"=11000： 选择为 SPI3_MISO"*/
    {_CAN_RX, 0x1A},   /*"=11010： 选择为 CAN_RX"*/
    {_TRIG_OUT, 0x1C}, /*"=11100： 选择为 TRIG_OUT(智能微断需求)"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART1, 0x01},    /*"=00001： 选择为 UART1 接口， P22 为 RX1， P23 为 TX1"*/
/*"	注1：PCA0[26],PCA3[13]都会同时影响P22 P23
    注2：当 PCA3 寄存器的 bit[13]=0 时， 该位配置有效；
        当 PCA3寄存器的 bit[13]=1 时， 由 PCA3 寄存器的 bit[12:7]决定P22/P22 的复用。"*/
#endif
};
/*"***************************** P2_3 *****************************"*/
static const sGPIOFunReg_TypeDef P2_3_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[3], PCA2[7], PCA2[6], PCA1[4], PCA1[3]"*/
    {GPIO_PCA1, _bit3},           /*"Bit0"*/
    {GPIO_PCA1, _bit4},           /*"Bit1"*/
    {GPIO_PCA2, _bit6},           /*"Bit2"*/
    {GPIO_PCA2, _bit7},           /*"Bit3"*/
    {GPIO_IOCFG_UART_I2C, _bit3}, /*"Bit4"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* {PCA3[28]_PCA0[31:30]_PCA3[13]_PCA3[12:10]_PCA1[16]_PCA0[26]} 定义 P23 口的复用配置： */
    {GPIO_PCA3, _bit10}, /*"Bit0"*/
    {GPIO_PCA3, _bit11}, /*"Bit1"*/
    {GPIO_PCA3, _bit12}, /*"Bit2"*/
    {GPIO_PCA3, _bit13}, /*"Bit3"*/
#elif defined(RN831x_RN861x_MCU_V2)
    /*"由{PCA3[13],PCA3[12:10],PCA0[26]}定义端口 P23 复用配置："*/
    {GPIO_PCA0, _bit26}, /*"Bit0"*/
    {GPIO_PCA1, _bit16}, /*"Bit1"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA3[13],PCA3[12:10],PCA0[26]}定义端口 P23 复用配置："*/
    {GPIO_PCA0, _bit26}, /*"Bit0"*/
    {GPIO_PCA3, _bit10}, /*"Bit1"*/
    {GPIO_PCA3, _bit11}, /*"Bit2"*/
    {GPIO_PCA3, _bit12}, /*"Bit3"*/
    {GPIO_PCA3, _bit13}, /*"Bit4"*/
#else
    /*"由{PCA3[13],PCA3[12:10],PCA0[26]}定义端口 P23 复用配置："*/
    {GPIO_PCA0, _bit26}, /*"Bit0"*/
#endif
};
static const sGPIOFunCll_TypeDef P2_3_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /*"=00000: 选择为IO口；"*/
    {_KEY, 0x01},      /*"=00001: 选择为KEY5"*/
    {_UART2, 0x02},    /*"=00010: P22/P23配置为UART2,P22为RX2，P23为TX2;"*/
    {_LPUART, 0x12},   /*"=10010: P22/P23配置为LPUART,P22为RX2，P23为TX2;"*/
    {_SPI3, 0x04},     /*"=00100: 选择为MISO3"*/
    {_TC1_P_0, 0x08},  /*"=01000: 选择为TC1_P[0]"*/
    {_RTC_OUT, 0x0C},  /*"=01100: RTC_OUT"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x08}, /*"=x_xx_1_000_x_x： 选择为 IO 口"*/
    {_UART1, 0x09},    /* x_xx_1_001_x_x： 选择为 TX1 */
    {_UART2, 0x0A},    /* x_xx_1_010_x_x： 选择为 TX2 */
    {_TC0_P_1, 0x0B},  /* x_xx_1_011_x_x： 选择为 TC0_P[1] */
    {_TC_IN, 0x0C},    /* x_xx_1_100_x_x： 选择为 TCIN */
    {_INT, 0x0D},      /* x_xx_1_101_x_x： 选择为 INT7（P37 选择为 INT7 优先级高） */
    {_SPI0, 0x0E},     /* x_xx_1_110_x_x： 选择为 SPI0_MOSI */
    {_SPI1, 0x0F},     /* x_xx_1_111_x_x： 选择为 SPI1_MOSI */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART1, 0X01},    /*"=00001： 选择为 UART1 接口， P22 为 RX1， P23 为 TX1"*/
    {_UART2, 0x02},    /*"=00010： 选择为 UART2 接口， P22 为 RX2， P23 为 TX2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART1, 0x12},    /*"=10010： 选择为 TX1"*/
    {_UART2, 0x14},    /*"=10100： 选择为 TX2"*/
    {_SPI1, 0x16},     /*"=10110： 选择为 SPI1_MOSI"*/
    {_SPI3, 0x18},     /*"=11000： 选择为 SPI3_MOSI"*/
    {_CAN_TX, 0x1A},   /*"=11010： 选择为 CAN_TX"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART1, 0X01},    /*"=00001： 选择为 UART1 接口， P22 为 RX1， P23 为 TX1"*/
/*"	注1：PCA0[26],PCA3[13]都会同时影响P22 P23
    注2：当 PCA3 寄存器的 bit[13]=0 时， 该位配置有效；
        当 PCA3寄存器的 bit[13]=1 时， 由 PCA3 寄存器的 bit[12:7]决定P22/P22 的复用。"*/
#endif
};
/*"***************************** P2_4 *****************************"*/
static const sGPIOFunReg_TypeDef P2_4_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[24], IOCFG_IOCNT[7], PCA2[9], PCA2[8], PCA1[6], PCA1[5]"*/
    {GPIO_PCA1, _bit5},            /*"Bit0"*/
    {GPIO_PCA1, _bit6},            /*"Bit1"*/
    {GPIO_PCA2, _bit8},            /*"Bit2"*/
    {GPIO_PCA2, _bit9},            /*"Bit3"*/
    {GPIO_IOCFG_IOCNT, _bit7},     /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit24}, /*"Bit5"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由 {PCA3[28]_PCA0[31:30]_PCA3[20]_PCA3[16:14]_PCA0[29]_PCA0[27]}定义 P24 口的复用配置： */
    {GPIO_PCA0, _bit27}, /*"BIT0"*/
    {GPIO_PCA0, _bit29}, /*"BIT1"*/
    {GPIO_PCA3, _bit14}, /*"Bit2"*/
    {GPIO_PCA3, _bit15}, /*"Bit3"*/
    {GPIO_PCA3, _bit16}, /*"Bit4"*/
    {GPIO_PCA3, _bit20}, /*"Bit5"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA3[20],PCA3[16:14],PCA0[29],PCA0[27]}定义端口 P24 复用配置："*/
    {GPIO_PCA0, _bit27}, /*"BIT0"*/
    {GPIO_PCA0, _bit29}, /*"BIT1"*/
    {GPIO_PCA3, _bit14}, /*"BIT2"*/
    {GPIO_PCA3, _bit15}, /*"BIT3"*/
    {GPIO_PCA3, _bit16}, /*"BIT4"*/
    {GPIO_PCA3, _bit20}, /*"BIT5"*/
#else
    {GPIO_PCA0, _bit27}, /*"BIT0"*/
    {GPIO_PCA0, _bit29}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P2_4_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /*=000000: 选择为IO口；*/
    {_IIC0, 0x01},       /*=000001: 选择为I2C0接口，P24为SCL；*/
    {_IIC1, 0x21},       /*=100001: 选择为I2C1接口，P24为SCL；*/
    {_TC1_P_0, 0x02},    /*=000010:  选择为TC输出；P24为TC1_P[0]*/
    {_TC_IN, 0x03},      /*=000011: 选择为TC输入；*/
    {_SPI4, 0x04},       /*=000100: 选择为SCSN4备注：P24、P25、P26、P27为一组，组成SPI4。*/
    {_ISO7816, 0x08},    /*=001000: 选择为7816CLK*/
    {_CF_OUT4, 0x0C},    /*=001100: ：CF4*/
    {_CF4, 0x0C},        /*=001100: ：CF4*/
    {_IOCNT_OUT4, 0x1C}, /*=011100: ：IOCNT4*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_SWD, 0x02},      /* x_xx_0_xxx_1_x： 选择为 SWDCLK */
    {_NORMALIO, 0x20}, /* x_xx_1_000_x_x： 选择为 IO 口 */
    {_UART2, 0x24},    /* x_xx_1_001_x_x： 选择为 RX2 */
    {_TC0_N_1, 0x28},  /* x_xx_1_010_x_x： 选择为 TC0_N[1] */
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=000000： 选择为 IO 口"*/
    {_SWD, 0x02},      /*"=00001x： 选择为 SWD 口， P24 为 SWDCLK， P25 为 SWDIO"*/
    {_UART2, 0x24},    /*"=100100： 选择为 RX2"*/
    {_CAN_RX, 0x28},   /*"=101000： 选择为 CAN_RX"*/
    {_TC1_P_1, 0x2C},  /*"=101100： 选择为 TC1_P[1]"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=000000： 选择为 IO 口"*/
    {_SWD, 0x02},      /*"=00001x： 选择为 SWD 口， P24 为 SWDCLK， P25 为 SWDIO"*/
    {_UART2, 0x01},    /*"=000001： 选择为 UART2 接口， P24 为 RX2， P25 为 TX2"*/
/*"	注1：PCA0[29],PCA0[27],PCA3[29]都会同时影响P24 P25
    注2：当 PCA3 寄存器的 bit[29]=0 时， 该配置有效；
        当 PCA3寄存器的 bit[20]=1 时， 由 PCA3 寄存器的 bit[19:14]决定P24/P25 的复用。"*/
#endif
};
/*"***************************** P2_5 *****************************"*/
static const sGPIOFunReg_TypeDef P2_5_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[24], PCA2[11], PCA2[10], PCA1[8], PCA1[7]"*/
    {GPIO_PCA1, _bit7},            /*"Bit0"*/
    {GPIO_PCA1, _bit8},            /*"Bit1"*/
    {GPIO_PCA2, _bit10},           /*"Bit2"*/
    {GPIO_PCA2, _bit11},           /*"Bit3"*/
    {GPIO_IOCFG_UART_I2C, _bit24}, /*"Bit4"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由 {PCA3[28]_PCA0[31:30]_PCA3[20]_PCA3[19:17]_PCA0[29]_PCA0[27]}定义 P25 口的复用配置： */
    {GPIO_PCA0, _bit27}, /*"BIT0"*/
    {GPIO_PCA0, _bit29}, /*"BIT1"*/
    {GPIO_PCA3, _bit17}, /*"Bit2"*/
    {GPIO_PCA3, _bit18}, /*"Bit3"*/
    {GPIO_PCA3, _bit19}, /*"Bit4"*/
    {GPIO_PCA3, _bit20}, /*"Bit5"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA3[20],PCA3[19:17],PCA0[29],PCA0[27]}定义端口 P24 复用配置："*/
    {GPIO_PCA0, _bit27}, /*"BIT0"*/
    {GPIO_PCA0, _bit29}, /*"BIT1"*/
    {GPIO_PCA3, _bit17}, /*"BIT2"*/
    {GPIO_PCA3, _bit18}, /*"BIT3"*/
    {GPIO_PCA3, _bit19}, /*"BIT4"*/
    {GPIO_PCA3, _bit20}, /*"BIT5"*/
#else
    /*"由{PCA3[20],PCA3[19:17],PCA0[29],PCA0[27]}定义端口 P24 复用配置："*/
    {GPIO_PCA0, _bit27}, /*"BIT0"*/
    {GPIO_PCA0, _bit29}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P2_5_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /*"=00000: 选择为IO口；"*/
    {_IIC0, 0x01},     /*"=00001:选择为I2C0接口，P25为SDA；"*/
    {_IIC1, 0x11},     /*"=10001:选择为I2C1接口，P25为SDA；"*/
    {_TC1_P_1, 0x02},  /*"=00010:  选择为TC输出；P25为TC1_P[1]"*/
    {_TC_IN, 0x03},    /*"=00011: 选择为TC输入；"*/
    {_SPI4, 0x04},     /*"=00100: 选择为SCLK4"*/
    {_ISO7816, 0x08},  /*"=01000: 选择为78160IO"*/
    {_RTC_OUT, 0x0C},  /*"=01100: RTC_OUT"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_SWD, 0x02},      /* x_xx_0_xxx_1_x： 选择为 SWDCLK */
    {_NORMALIO, 0x20}, /* x_xx_1_000_x_x： 选择为 IO 口 */
    {_UART2, 0x24},    /* 0_00_1_001_x_x： 选择为 TX2 */
    {_TC0_P_1, 0x28},  /* x_xx_1_010_x_x： 选择为 TC0_P[1] */
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=000000： 选择为 IO 口"*/
    {_SWD, 0x02},      /*"=00001x： 选择为 SWD 口， P24 为 SWDCLK， P25 为 SWDIO"*/
    {_UART2, 0x24},    /*"=100100： 选择为 TX2"*/
    {_CAN_TX, 0x28},   /*"=101000： 选择为 CAN_TX"*/
    {_TC1_N_1, 0x2C},  /*"=101100： 选择为 TC1_N[1]"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=000000： 选择为 IO 口"*/
    {_SWD, 0x02},      /*"=00001x： 选择为 SWD 口， P24 为 SWDCLK， P25 为 SWDIO"*/
    {_UART2, 0x01},    /*"=000001： 选择为 UART2 接口， P24 为 RX2， P25 为 TX2"*/
/*"	注1：PCA0[29],PCA0[27],PCA3[20]都会同时影响P24 P25
    注2：当 PCA3 寄存器的 bit[20]=0 时， 该配置有效；
        当 PCA3寄存器的 bit[20]=1 时， 由 PCA3 寄存器的 bit[19:14]决定P24/P25 的复用。"*/
#endif
};
/*"***************************** P2_6 *****************************"*/
static const sGPIOFunReg_TypeDef P2_6_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[4],  IOCFG_IOCNT[8], PCA2[13], PCA2[12], PCA1[10], PCA1[9]"*/
    {GPIO_PCA1, _bit9},           /*"Bit0"*/
    {GPIO_PCA1, _bit10},          /*"Bit1"*/
    {GPIO_PCA2, _bit12},          /*"Bit2"*/
    {GPIO_PCA2, _bit13},          /*"Bit3"*/
    {GPIO_IOCFG_IOCNT, _bit8},    /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit4}, /*"Bit5"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由 {PCA3[28]_PCA0[31:30]_PCA3[27]_PCA3[23:21]_PCA1[17]_PCA0[28]}定义 P26 口的复用配置： */
    {GPIO_PCA3, _bit21}, /*"Bit0"*/
    {GPIO_PCA3, _bit22}, /*"Bit1"*/
    {GPIO_PCA3, _bit23}, /*"Bit2"*/
    {GPIO_PCA3, _bit27}, /*"Bit3"*/
#elif defined(RN831x_RN861x_MCU_V2)
    /*"由{PCA3[27],PCA3[23:21],PCA0[28]}定义端口 P24 复用配置："*/
    {GPIO_PCA0, _bit28}, /*"BIT0"*/
    {GPIO_PCA1, _bit17}, /*"Bit1"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA3[27],PCA3[23:21],PCA0[28]}定义端口 P24 复用配置："*/
    {GPIO_PCA0, _bit28}, /*"BIT0"*/
    {GPIO_PCA3, _bit21}, /*"BIT1"*/
    {GPIO_PCA3, _bit22}, /*"BIT2"*/
    {GPIO_PCA3, _bit23}, /*"BIT3"*/
    {GPIO_PCA3, _bit27}, /*"BIT4"*/
#else
    /*"由{PCA3[27],PCA3[23:21],PCA0[28]}定义端口 P24 复用配置："*/
    {GPIO_PCA0, _bit28}, /*"BIT0"*/
#endif
};
static const sGPIOFunCll_TypeDef P2_6_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /*"=000000: 选择为IO口；"*/
    {_UART3, 0x01},      /*"=000001: 选择为UART3接口，P26为RX3；"*/
    {_LPUART, 0x21},     /*"=100001: 选择为LPUART接口，P26为RX3；"*/
    {_TC1_N_0, 0x02},    /*"=000010:  选择为TC输出；P26为TC1_N[0]。"*/
    {_TC_IN, 0x03},      /*"=000011: 选择为TC输入；"*/
    {_CF_OUT2, 0x04},    /*"=000100: 选择为CF2"*/
    {_CF2, 0x04},        /*"=000100: 选择为CF2"*/
    {_IOCNT_OUT2, 0x14}, /*"=010100: 选择为IOCNT2"*/
    {_RTC_OUT, 0x08},    /*"=001000: 选择为RTC_OUT"*/
    {_SPI4, 0x0C},       /*"=001100: 选择为MOSI4"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x08}, /*"=x_xx_1_000_x_x： 选择为 IO 口"*/
    {_UART3, 0x09},    /* x_xx_1_001_x_x： 选择为 RX3*/
    {_UART2, 0x0A},    /* x_xx_1_010_x_x： 选择为 RX2 */
    {_TC0_N_1, 0x0B},  /* x_xx_1_011_x_x： 选择为 TC0_N[1] */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART3, 0x01},    /*"=00001： 选择为 UART3 接口， P26 为 RX3， P27 为 TX3"*/
    {_UART2, 0x02},    /*"=00010： 选择为 UART2 接口， P26 为 RX2， P27 为 TX2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART3, 0x12},    /*"=10010： 选择为 RX3"*/
    {_UART2, 0x14},    /*"=10100： 选择为 RX2"*/
    {_TRIG_OUT, 0x16}, /*"=10110： 选择为 TRIG_OUT(智能微断需求)"*/
    {_CAN_RX, 0x18},   /*"=11000： 选择为 CAN_RX"*/
    {_TC1_P_1, 0x1A},  /*"=11010： 选择为 TC1_P[1]"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART3, 0x01},    /*"=00001： 选择为 UART3 接口， P26 为 RX3， P27 为 TX3"*/
/*"	注1： PCA0[28],PCA3[27]都会同时影响P26 P27
    注2：当 PCA3 寄存器的 bit[27]=0 时， 该配置有效；
        当 PCA3寄存器的 bit[27]=1 时， 由 PCA3 寄存器的 bit[26:21]决定P26P27 的复用。"*/
#endif
};
/*"***************************** P2_7 *****************************"*/
static const sGPIOFunReg_TypeDef P2_7_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[4],  IOCFG_IOCNT[9], PCA2[15], PCA2[14], PCA1[12], PCA1[11]"*/
    {GPIO_PCA1, _bit11},          /*"Bit0"*/
    {GPIO_PCA1, _bit12},          /*"Bit1"*/
    {GPIO_PCA2, _bit14},          /*"Bit2"*/
    {GPIO_PCA2, _bit15},          /*"Bit3"*/
    {GPIO_IOCFG_IOCNT, _bit9},    /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit4}, /*"Bit5"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /*  {PCA3[28]_PCA0[31:30]_PCA3[27]_PCA3[26:24]_PCA1[17]_PCA0[28]}定义 P27 口的复用配置： */
    {GPIO_PCA3, _bit24}, /*"Bit0"*/
    {GPIO_PCA3, _bit25}, /*"Bit1"*/
    {GPIO_PCA3, _bit26}, /*"Bit2"*/
    {GPIO_PCA3, _bit27}, /*"Bit3"*/
#elif defined(RN831x_RN861x_MCU_V2)
    {GPIO_PCA0, _bit28}, /*"BIT0"*/
    {GPIO_PCA1, _bit17}, /*"Bit1"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA3[27],PCA3[26:24],PCA0[28]}定义端口 P24 复用配置："*/
    {GPIO_PCA0, _bit28}, /*"BIT0"*/
    {GPIO_PCA3, _bit24}, /*"BIT1"*/
    {GPIO_PCA3, _bit25}, /*"BIT2"*/
    {GPIO_PCA3, _bit26}, /*"BIT3"*/
    {GPIO_PCA3, _bit27}, /*"BIT4"*/
#else
    {GPIO_PCA0, _bit28}, /*"BIT0"*/
#endif
};
static const sGPIOFunCll_TypeDef P2_7_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /*"=000000: 选择为IO口；"*/
    {_UART3, 0x01},      /*"=000001: 选择为UART3接口，P27为TX3；"*/
    {_LPUART, 0x21},     /*"=100001: 选择为LPUART接口，P27为TX3；"*/
    {_TC1_N_1, 0x02},    /*"=000010:  选择为TC输出；P27为TC1_N[1]。"*/
    {_TC_IN, 0x03},      /*"=000011: 选择为TC输入； "*/
    {_CF_OUT3, 0x04},    /*"=000100: 选择为CF3"*/
    {_CF3, 0x04},        /*"=000100: 选择为CF3"*/
    {_IOCNT_OUT3, 0x14}, /*"=010100: 选择为IOCNT3"*/
    {_RTC_1S, 0x08},     /*"=001000: 选择为RTC_1S"*/
    {_SPI4, 0x0C},       /*"=001100: 选择为MISO4"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x08}, /* x_xx_1_000_x_x： 选择为 IO 口 */
    {_UART3, 0x09},    /* x_xx_1_001_x_x： 选择为 TX3 */
    {_UART2, 0x0A},    /* x_xx_1_010_x_x： 选择为 TX2 */
    {_TC0_P_1, 0x0B},  /* x_xx_1_011_x_x： 选择为 TC0_P[1] */
    {_INT, 0x0C},      /* x_xx_1_100_x_x： 选择为 INT2（ 优先级 P32＞P27＞P76） */
    {_KEY, 0x0D},      /* x_xx_1_101_x_x： 选择为 KEY2（优先级 P00＞P27＞P86） */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART3, 0x01},    /*"=00001： 选择为 UART3 接口， P26 为 RX3， P27 为 TX3"*/
    {_UART2, 0x02},    /*"=00010： 选择为 UART2 接口， P26 为 RX2， P27 为 TX2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART3, 0x12},    /*"=10010： 选择为 TX3"*/
    {_UART2, 0x14},    /*"=10100： 选择为 TX2"*/
    {_TRIG_OUT, 0x16}, /*"=10110： 选择为 TRIG_OUT(智能微断需求)"*/
    {_CAN_TX, 0x18},   /*"=11000： 选择为 CAN_TX"*/
    {_TC1_N_1, 0x1A},  /*"=11010： 选择为 TC1_N[1]"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=00000： 选择为 IO 口"*/
    {_UART3, 0x01},    /*"=00001： 选择为 UART3 接口， P26 为 RX3， P27 为 TX3"*/
/*"	注1： PCA0[28],PCA3[27]都会同时影响P26 P27
    注2：当 PCA3 寄存器的 bit[27]=0 时， 该配置有效；
        当 PCA3寄存器的 bit[27]=1 时， 由 PCA3 寄存器的 bit[26:21]决定P26P27 的复用。"*/
#endif
};
/*"***************************** P3_0 *****************************"*/
static const sGPIOFunReg_TypeDef P3_0_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_IOCNT[5], PCA2[17], PCA2[16], PCA1[14], PCA1[13]"*/
    {GPIO_PCA1, _bit13},       /*"Bit0"*/
    {GPIO_PCA1, _bit14},       /*"Bit1"*/
    {GPIO_PCA2, _bit16},       /*"Bit2"*/
    {GPIO_PCA2, _bit17},       /*"Bit3"*/
    {GPIO_IOCFG_IOCNT, _bit5}, /*"Bit4"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCA1[19:18],PCA1[1:0]}定义端口 P30 复用配置： */
    {GPIO_PCA1, _bit0},  /*"BIT0"*/
    {GPIO_PCA1, _bit1},  /*"BIT1"*/
    {GPIO_PCA1, _bit18}, /*"BIT2"*/
    {GPIO_PCA1, _bit19}, /*"BIT3"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA1[17:16],PCA1[1:0]}定义端口 P30 复用配置："*/
    {GPIO_PCA1, _bit0},  /*"BIT0"*/
    {GPIO_PCA1, _bit1},  /*"BIT1"*/
    {GPIO_PCA1, _bit16}, /*"BIT2"*/
    {GPIO_PCA1, _bit17}, /*"BIT3"*/
#else
    /*"由{PCA1[17:16],PCA1[1:0]}定义端口 P30 复用配置："*/
    {GPIO_PCA1, _bit0}, /*"BIT0"*/
    {GPIO_PCA1, _bit1}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P3_0_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /*"=00000: 选择为IO口；"*/
    {_TC_IN, 0x01},      /*"=00001: 选择为TCI；"*/
    {_KEY, 0x02},        /*"=00010: 选择为KEY3；"*/
    {_CF_OUT4, 0x04},    /*"=00100: 选择为CF4"*/
    {_CF4, 0x04},        /*"=00100: 选择为CF4"*/
    {_IOCNT_OUT4, 0x14}, /*"=10100: 选择为IOCNT4"*/
    {_TC1_P_1, 0x08},    /*"=01000: 选择为TC1_P[1]"*/
    {_RTC_OUT, 0x0C},    /*"=01100: 选择为RTC_OUT"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},        /*"=0001： 选择为外部中断输入口 INT0"*/
    {_TC_IN, 0x02},      /*"=0010： 选择为 TC 输入"*/
    {_UART4, 0x03},      /*"=0011： 选择为 TX4"*/
    {_D2F_OUT1, 0x04},   /* =0100： 选择为 D2F_OUT1 */
    {_D2F_OUT2, 0x05},   /* =0101： 选择为 D2F_OUT2 */
    {_IOCNT_OUT0, 0x06}, /* =0110： 选择为 IOCNT_OUT0 */
    {_RTC_1S, 0x07},     /* =0111： 选择为万年历秒脉冲输出 RTC1S */
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT0"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TC 输入"*/
    {_UART4, 0x03},    /*"=0011： 选择为 TX4"*/
    {_RTC_OUT, 0x04},  /*"=0100： 选择为 RTC_OUT"*/
    {_ADC_CLKO, 0x05}, /*"=0101： 选择为 ADC_CLKO"*/
    {_TRIG_OUT, 0x06}, /*"=0110： 选择为 TRIG_OUT(智能微断需求)"*/
    {_CAN_TX, 0x07},   /*"=0111： 选择为 CAN_TX"*/
    {_RTC_1S, 0x08},   /*"=1000： 选择为万年历秒输出 RTC1S"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT0"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TC 输入"*/
    {_UART4, 0x03},    /*"=0011： 选择为 TX4"*/
#endif
};
/*"***************************** P3_1 *****************************"*/
static const sGPIOFunReg_TypeDef P3_1_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[25],  IOCFG_IOCNT[10], PCA2[19], PCA2[18], PCA1[15]"*/
    {GPIO_PCA1, _bit15},           /*"Bit0"*/
    {GPIO_PCA2, _bit18},           /*"Bit1"*/
    {GPIO_PCA2, _bit19},           /*"Bit2"*/
    {GPIO_IOCFG_IOCNT, _bit10},    /*"Bit3"*/
    {GPIO_IOCFG_UART_I2C, _bit25}, /*"Bit4"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"{PCA1[19:18],PCA1[3:2]}定义端口 P31 复用配置："*/
    {GPIO_PCA1, _bit2},  /*"BIT0"*/
    {GPIO_PCA1, _bit3},  /*"BIT1"*/
    {GPIO_PCA1, _bit18}, /*"BIT2"*/
    {GPIO_PCA1, _bit19}, /*"BIT3"*/
#else
    /*"{PCA1[19:18],PCA1[3:2]}定义端口 P31 复用配置："*/
    {GPIO_PCA1, _bit2}, /*"BIT0"*/
    {GPIO_PCA1, _bit3}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P3_1_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /* =00000:选择为IO口； */
    {_SPI, 0x01},        /* =00001:选择为SPI接口， */
    {_IIC0, 0x02},       /* =00010:选择为SCL，选择为I2C0 */
    {_IIC1, 0x12},       /* =10010:选择为SCL，I2C1 */
    {_CF_OUT4, 0x04},    /* =00100:选择为CF4 */
    {_CF4, 0x04},        /* =00100:选择为CF4 */
    {_IOCNT_OUT4, 0x0C}, /* =01100:选择为IOCNT4 */
    {_RTC_OUT, 0x06},    /* =00110:选择为RTC_OUT */
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT1"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TC 输入"*/
    {_UART4, 0x03},    /*"=0011： 选择为 RX4"*/
    {_RTC_OUT, 0x04},  /*"=0100： 选择为 RTC_OUT"*/
    {_ADC_CLKO, 0x05}, /*"=0101： 选择为 ADC_CLKO"*/
    {_TRIG_OUT, 0x06}, /*"=0110： 选择为 TRIG_OUT(智能微断需求)"*/
    {_CAN_RX, 0x07},   /*"=0111： 选择为 CAN_RX"*/
    {_CF_OUT0, 0x08},  /*"=1000： 选择为 CF_OUT0"*/
    {_D2F_OUT0, 0x09}, /*"=1001： 选择为 D2F_OUT0"*/
    {_TC1_P_1, 0x0A},  /*"=1010： 选择为 TC1_P[1](智能微断需求)"*/
    {_RTC_1S, 0x0B},   /*"=1011： 选择为万年历秒输出 RTC1S"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT1"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TC 输入"*/
    {_UART4, 0x03},    /*"=0011： 选择为 RX4"*/
#endif
};
/*"***************************** P3_2 *****************************"*/
static const sGPIOFunReg_TypeDef P3_2_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[25], IOCFG_IOCNT[11], PCA2[21], PCA2[20], PCA1[15]"*/
    {GPIO_PCA1, _bit15},           /*"Bit0"*/
    {GPIO_PCA2, _bit20},           /*"Bit1"*/
    {GPIO_PCA2, _bit21},           /*"Bit2"*/
    {GPIO_IOCFG_IOCNT, _bit11},    /*"Bit3"*/
    {GPIO_IOCFG_UART_I2C, _bit25}, /*"Bit4"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCA1[21:20],PCA1[5:4]}定义端口 P32 复用配置： */
    {GPIO_PCA1, _bit4},  /*"BIT0"*/
    {GPIO_PCA1, _bit5},  /*"BIT1"*/
    {GPIO_PCA1, _bit20}, /*"BIT2"*/
    {GPIO_PCA1, _bit21}, /*"BIT3"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"{PCA1[21:20],PCA1[5:4]}定义端口 P32 复用配置："*/
    {GPIO_PCA1, _bit4},  /*"BIT0"*/
    {GPIO_PCA1, _bit5},  /*"BIT1"*/
    {GPIO_PCA1, _bit20}, /*"BIT2"*/
    {GPIO_PCA1, _bit21}, /*"BIT3"*/
#else
    /*"{PCA1[21:20],PCA1[5:4]}定义端口 P32 复用配置："*/
    {GPIO_PCA1, _bit4}, /*"BIT0"*/
    {GPIO_PCA1, _bit5}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P3_2_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /* =00000:选择为IO口； */
    {_SPI, 0x01},        /* =00001:选择为SPI接口， */
    {_IIC0, 0x02},       /* =00010:选择为SDA，可选择为I2C0 */
    {_IIC1, 0x12},       /* =10010:选择为SDA，可选择为I2C1 */
    {_CF_OUT3, 0x04},    /* =00100:选择为CF3 */
    {_CF3, 0x04},        /* =00100:选择为CF3 */
    {_IOCNT_OUT3, 0x0C}, /* =01100:选择为IOCNT3 */
    {_RTC_1S, 0x06},     /* =00110:选择为RTC_1S */
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},        /*"=0001： 选择为外部中断输入口 INT2"*/
    {_RTC_OUT, 0x02},    /*"=0010： 选择为 RTC 输出 RTC_OUT（ 默认选择为 RTC 输出）"*/
    {_KEY, 0x03},        /*"=0011： 选择为 KEY5"*/
    {_D2F_OUT0, 0x04},   /* =0100： 选择为 D2F_OUT0 */
    {_D2F_OUT2, 0x05},   /* =0101： 选择为 D2F_OUT2 */
    {_IOCNT_OUT0, 0x06}, /* =0110： 选择为 IOCNT_OUT0 */
    {_RTC_1S, 0x07},     /* =0111： 选择为万年历秒脉冲输出 RTC1S */
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT2"*/
    {_RTC_OUT, 0x02},  /*"=0010： 选择为 RTC 输出 RTC_OUT（ 默认选择为 RTC 输出）"*/
    {_KEY, 0x03},      /*"=0011： 选择为 KEY5"*/
    {_ADC_CLKO, 0x04}, /*"=0100： 选择为 ADC_CLKO"*/
    {_TRIG_OUT, 0x05}, /*"=0101： 选择为 TRIG_OUT(智能微断需求)"*/
    {_CF_OUT1, 0x06},  /*"=0110： 选择为 CF_OUT1"*/
    {_D2F_OUT1, 0x07}, /*"=0111： 选择为 D2F_OUT1"*/
    {_TC1_N_1, 0x08},  /*"=1000： 选择为 TC1_N[1](智能微断需求)"*/
    {_RTC_1S, 0x09},   /*"=1001： 选择为万年历秒输出 RTC1S"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT2"*/
    {_RTC_OUT, 0x02},  /*"=0010： 选择为 RTC 输出 RTC_OUT（ 默认选择为 RTC 输出）"*/
    {_KEY, 0x03},      /*"=0011： 选择为 KEY5"*/
#endif
};
/*"***************************** P3_3 *****************************"*/
static const sGPIOFunReg_TypeDef P3_3_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[5], IOCFG_IOCNT[12], PCA2[23], PCA2[22], PCA1[15]"*/
    {GPIO_PCA1, _bit15},          /*"Bit0"*/
    {GPIO_PCA2, _bit22},          /*"Bit1"*/
    {GPIO_PCA2, _bit23},          /*"Bit2"*/
    {GPIO_IOCFG_IOCNT, _bit12},   /*"Bit3"*/
    {GPIO_IOCFG_UART_I2C, _bit5}, /*"Bit4"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"{PCA1[23:22],PCA1[7:6]}定义端口 P33 复用配置："*/
    {GPIO_PCA1, _bit6},  /*"BIT0"*/
    {GPIO_PCA1, _bit7},  /*"BIT1"*/
    {GPIO_PCA1, _bit22}, /*"BIT2"*/
    {GPIO_PCA1, _bit23}, /*"BIT3"*/
#else
    /*"{PCA1[23:22],PCA1[7:6]}定义端口 P33 复用配置："*/
    {GPIO_PCA1, _bit6}, /*"BIT0"*/
    {GPIO_PCA1, _bit7}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P3_3_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /* =00000:选择为IO口； */
    {_SPI, 0x01},        /* =00001:选择为SPI接口， */
    {_UART2, 0x02},      /* =00010:选择为UART2 RX2 */
    {_LPUART, 0x12},     /* =10010:选择为LPUART RX */
    {_CF_OUT2, 0x04},    /* =00100:选择为CF2 */
    {_CF2, 0x04},        /* =00100:选择为CF2 */
    {_IOCNT_OUT2, 0x0C}, /* =01100:选择为IOCNT2 */
    {_RTC_OUT, 0x06},    /* =00110:选择为RTC_OUT */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT3"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TC 输入"*/
    {_CF0, 0x03},      /*"=0011： 选择为电能积分模块 D2F 的输出 CF0"*/
    {_D2F_OUT0, 0x03}, /*"=0011： 选择为电能积分模块 D2F 的输出 D2F_OUT0"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},        /*"=0001： 选择为外部中断输入口 INT3"*/
    {_TC_IN, 0x02},      /*"=0010： 选择为 TC 输入"*/
    {_ADC_CLKO, 0x03},   /*"=0011： 选择为 ADC_CLKO"*/
    {_IOCNT_OUT3, 0x04}, /*"=0100： 选择为 IOCNT_OUT3"*/
    {_IA_IN, 0x05},      /*"=0101： 选择为 IA_IN"*/
    {_IB_IN, 0x06},      /*"=0110： 选择为 IB_IN"*/
                         /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT3"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TC 输入"*/
#endif
};
/*"***************************** P3_4 *****************************"*/
static const sGPIOFunReg_TypeDef P3_4_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[5], IOCFG_IOCNT[13], PCA2[25], PCA2[24], PCA1[15]"*/
    {GPIO_PCA1, _bit15},          /*"Bit0"*/
    {GPIO_PCA2, _bit24},          /*"Bit1"*/
    {GPIO_PCA2, _bit25},          /*"Bit2"*/
    {GPIO_IOCFG_IOCNT, _bit13},   /*"Bit3"*/
    {GPIO_IOCFG_UART_I2C, _bit5}, /*"Bit4"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"{PCA1[25:24],PCA1[9:8]}定义端口 P34 复用配置："*/
    {GPIO_PCA1, _bit8},  /*"BIT0"*/
    {GPIO_PCA1, _bit9},  /*"BIT1"*/
    {GPIO_PCA1, _bit24}, /*"BIT2"*/
    {GPIO_PCA1, _bit25}, /*"BIT3"*/
#else
    /*"{PCA1[25:24],PCA1[9:8]}定义端口 P34 复用配置："*/
    {GPIO_PCA1, _bit8}, /*"BIT0"*/
    {GPIO_PCA1, _bit9}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P3_4_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /* =00000:选择为IO口； */
    {_SPI, 0x01},        /* =0001:选择为SPI接口， */
    {_UART2, 0x02},      /* =00010:选择为UART2 TX2 */
    {_LPUART, 0x12},     /* =10010:选择为LPUART TX */
    {_CF_OUT1, 0x04},    /* =00100:选择为CF1 */
    {_CF1, 0x04},        /* =00100:选择为CF1 */
    {_IOCNT_OUT1, 0x0C}, /* =01100:选择为IOCNT1 */
    {_RTC_1S, 0x06},     /* =00110:选择为RTC_1S */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT4"*/
    {_SF_OUT, 0x02},   /*"=0010： 选择为视在电能脉冲输出 SF_OUT"*/
    {_CF1, 0x03},      /*"=0011： 选择为电能积分模块 D2F 的输出 CF1"*/
    {_D2F_OUT1, 0x03}, /*"=0011： 选择为电能积分模块 D2F 的输出 D2F_OUT1"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},        /*"=0001： 选择为外部中断输入口 INT4"*/
    {_SF_OUT, 0x02},     /*"=0010： 选择为视在电能脉冲输出 SF_OUT"*/
    {_D2F_OUT3, 0x03},   /*"=0011： 选择为 D2F_OUT3"*/
    {_CF_OUT3, 0x04},    /*"=0100： 选择为 CF_OUT3"*/
    {_IOCNT_OUT3, 0x05}, /*"=0101： 选择为 IOCNT_OUT3"*/
    {_ADC_CLKO, 0x06},   /*"=0110： 选择为 ADC_CLKO"*/
    {_IA_IN, 0x07},      /*"=0111： 选择为 IA_IN"*/
    {_TRIG_OUT, 0x08},   /*"=1000： 选择为 TRIG_OUT(智能微断需求)"*/
                         /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT4"*/
    {_SF_OUT, 0x02},   /*"=0010： 选择为视在电能脉冲输出 SF_OUT"*/
#endif
};
/*"***************************** P3_5 *****************************"*/
static const sGPIOFunReg_TypeDef P3_5_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[26], IOCFG_IOCNT[14], PCA2[27], PCA2[26], PCA1[17], PCA1[16]"*/
    {GPIO_PCA1, _bit16},           /*"Bit0"*/
    {GPIO_PCA1, _bit17},           /*"Bit1"*/
    {GPIO_PCA2, _bit26},           /*"Bit2"*/
    {GPIO_PCA2, _bit27},           /*"Bit3"*/
    {GPIO_IOCFG_IOCNT, _bit14},    /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit26}, /*"Bit5"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"{PCA1[27:26],PCA1[11:10]}定义端口 P35 复用配置："*/
    {GPIO_PCA1, _bit10}, /*"BIT0"*/
    {GPIO_PCA1, _bit11}, /*"BIT1"*/
    {GPIO_PCA1, _bit26}, /*"BIT2"*/
    {GPIO_PCA1, _bit27}, /*"BIT3"*/
#else
    /*"{PCA1[27:26],PCA1[11:10]}定义端口 P35 复用配置："*/
    {GPIO_PCA1, _bit10}, /*"BIT0"*/
    {GPIO_PCA1, _bit11}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P3_5_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /* =000000:选择为IO口； */
    {_CF_OUT2, 0x01},    /* =000001:选择为CF2 */
    {_CF2, 0x01},        /* =000001:选择为CF2 */
    {_IOCNT_OUT2, 0x11}, /* =010001:选择为IOCNT2 */
    {_INT, 0x02},        /* =000010: 选择为INT5输入口； */
    {_IIC0, 0x04},       /* =000100: 选择为SCL，可配置为I2C0 */
    {_IIC1, 0x24},       /* =100100: 选择为SCL，可配置为I2C1 */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT5"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TC 输入"*/
    {_CF2, 0x03},      /*"=0011： 选择为电能积分模块 D2F 的输出 CF2"*/
    {_D2F_OUT2, 0x03}, /*"=0011： 选择为电能积分模块 D2F 的输出 D2F_OUT2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},        /*"=0001： 选择为外部中断输入口 INT5"*/
    {_TC_IN, 0x02},      /*"=0010： 选择为 TC 输入"*/
    {_D2F_OUT4, 0x03},   /*"=0011： 选择为 D2F_OUT4"*/
    {_IOCNT_OUT4, 0x04}, /*"=0100： 选择为 IOCNT_OUT4"*/
    {_ADC_CLKO, 0x05},   /*"=0101： 选择为 ADC_CLKO"*/
    {_IA_IN, 0x06},      /*"=0110： 选择为 IA_IN"*/
    {_IB_IN, 0x07},      /*"=0111： 选择为 IB_IN"*/
    {_TRIG_OUT, 0x08},   /*"=1000： 选择为 TRIG_OUT(智能微断需求)"*/
    {_CF_OUT4, 0x09},    /*"=1001： 选择为 CF_OUT4"*/
                         /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT5"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TC 输入"*/
#endif
};
/*"***************************** P3_6 *****************************"*/
static const sGPIOFunReg_TypeDef P3_6_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[26], IOCFG_IOCNT[15], PCA2[29], PCA2[28], PCA1[19], PCA1[18]"*/
    {GPIO_PCA1, _bit18},           /*"Bit0"*/
    {GPIO_PCA1, _bit19},           /*"Bit1"*/
    {GPIO_PCA2, _bit28},           /*"Bit2"*/
    {GPIO_PCA2, _bit29},           /*"Bit3"*/
    {GPIO_IOCFG_IOCNT, _bit15},    /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit26}, /*"Bit5"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"PCA1[29:28],PCA1[13:12]}定义端口 P36 复用配置："*/
    {GPIO_PCA1, _bit12}, /*"BIT0"*/
    {GPIO_PCA1, _bit13}, /*"BIT1"*/
    {GPIO_PCA1, _bit28}, /*"BIT2"*/
    {GPIO_PCA1, _bit29}, /*"BIT3"*/
#else
    /*"PCA1[29:28],PCA1[13:12]}定义端口 P36 复用配置："*/
    {GPIO_PCA1, _bit12}, /*"BIT0"*/
    {GPIO_PCA1, _bit13}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P3_6_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /* =000000:选择为IO口； */
    {_CF_OUT3, 0x01},    /* =000001:选择为CF3 */
    {_CF3, 0x01},        /* =000001:选择为CF3 */
    {_IOCNT_OUT2, 0x11}, /* =010001:选择为IOCNT3 */
    {_TC0_P_1, 0x02},    /* =000010: 选择为TC输出TC0_P[1] */
    {_TC_IN, 0x03},      /* =000011:选择为TC输入TCI */
    {_IIC0, 0x04},       /* =000100:选择为SDA，可配置为I2C0 */
    {_IIC1, 0x24},       /* =100100:选择为SDA，可配置为I2C1 */
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT6"*/
    {_HOSCO, 0x02},    /*"=001x： 选择为高频晶体管脚 HOSCO"*/
    {_CAN_RX, 0x04},   /*"=0100： 选择为 CAN_RX"*/
                       /*"=其他： 保留"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT6"*/
    {_SEG, 0x05},      /*"=0101： 选择为 SEG34"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT6"*/
    {_HOSCO, 0x02},    /*"=001x： 选择为高频晶体管脚 HOSCO"*/
/*"备注： 只要 PC36[1]和 PC37[1]中任何一位为高， 那么就选择为 HOSC"*/
#endif
};
/*"***************************** P3_7 *****************************"*/
static const sGPIOFunReg_TypeDef P3_7_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_IOCNT[16], PCA2[31], PCA2[30], PCA1[21], PCA1[20]"*/
    {GPIO_PCA1, _bit20},        /*"Bit0"*/
    {GPIO_PCA1, _bit21},        /*"Bit1"*/
    {GPIO_PCA2, _bit30},        /*"Bit2"*/
    {GPIO_PCA2, _bit31},        /*"Bit3"*/
    {GPIO_IOCFG_IOCNT, _bit16}, /*"Bit4"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCA1[31:30],PCA1[15:14]}定义端口 P37 复用配置："*/
    {GPIO_PCA1, _bit14}, /*"BIT0"*/
    {GPIO_PCA1, _bit15}, /*"BIT1"*/
    {GPIO_PCA1, _bit30}, /*"BIT2"*/
    {GPIO_PCA1, _bit31}, /*"BIT3"*/
#else
    /*"由{PCA1[31:30],PCA1[15:14]}定义端口 P37 复用配置："*/
    {GPIO_PCA1, _bit14}, /*"BIT0"*/
    {GPIO_PCA1, _bit15}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P3_7_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /* =00000:选择为IO口； */
    {_CF_OUT4, 0x01},    /* =00001:选择为CF4 */
    {_CF4, 0x01},        /* =00001:选择为CF4 */
    {_IOCNT_OUT4, 0x11}, /* =10001:选择为IOCNT4 */
    {_RTC_OUT, 0x02},    /* =00010: 选择为RTCOUT；（P02，P42若配置为RTCOUT时，可同时输出；） */
    {_CF_OUT0, 0x04},    /* =00100：选择为CF0 */
    {_CF0, 0x04},        /* =00100：选择为CF0 */
    {_IOCNT_OUT0, 0x14}, /* =10100：选择为IOCNT0 */
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT7"*/
    {_HOSCI, 0x02},    /*"=001x： 选择为高频晶体管脚 HOSCI"*/
    {_CAN_TX, 0x04},   /*"=0100： 选择为 CAN_TX"*/
                       /*"=其他： 保留"*/
/*"备注： 只要 PC36[1]和 PC37[1]中任何一位为高， 那么就选择为 HOSC"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT7"*/
    {_SEG, 0x05},      /*"=0101： 选择为  SEG35"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},      /*"=0001： 选择为外部中断输入口 INT7"*/
    {_HOSCI, 0x02},    /*"=001x： 选择为高频晶体管脚 HOSCI"*/
/*"备注： 只要 PC36[1]和 PC37[1]中任何一位为高， 那么就选择为 HOSC"*/
#endif
};
/*"***************************** P4_0 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2 /*"在VBAT域，不能使用bitband"*/
static const sGPIOFunReg_TypeDef P4_0_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /* 由{PCB2[1:0],PCB[0]}定义 P40 端口复用配置： */
    {GPIO_PCB, _bit0},  /*"BIT0"*/
    {GPIO_PCB2, _bit0}, /*"BIT1"*/
    {GPIO_PCB2, _bit1}, /*"BIT2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCB3[0],PCB2[1:0],PCB[0]}定义端口 P40 复用配置："*/
    {GPIO_PCB, _bit0},  /*"BIT0"*/
    {GPIO_PCB2, _bit0}, /*"BIT1"*/
    {GPIO_PCB2, _bit1}, /*"BIT2"*/
    {GPIO_PCB3, _bit0}, /*"BIT3"*/
#else
    /*"由{PCB3[0],PCB2[1:0],PCB[0]}定义端口 P40 复用配置："*/
    {GPIO_PCB, _bit0},  /*"BIT0"*/
    {GPIO_PCB2, _bit0}, /*"BIT1"*/
    {GPIO_PCB2, _bit1}, /*"BIT2"*/
#endif
};
static const sGPIOFunCll_TypeDef P4_0_Fun[] = {
#ifdef RN831x_RN861x_MCU_V3
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 选择为 7816_CLK"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT1"*/
    {_UART6, 0x03},    /* =011： 选择为 RX6 */
    {_SPI1, 0x04},     /* =100： 选择为 SPI1_SCSN */
    {_SPI0, 0x05},     /* =101： 选择为 SPI0_SCSN */
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 选择为 7816_CLK"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT1"*/
    {_SPI2, 0x03}, /*"=0011： 选择为 SPI2_SCSN"*/
    {_SPI1, 0x04}, /*"=0100： 选择为 SPI1_SCSN"*/
    {_ADC_CLKO, 0x05}, /*"=0101： 选择为 _ADC_CLKO"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT1"*/
    {_SPI1, 0x04}, /*"=0100： 选择为 SPI1_SCSN"*/
    {_ADC_CLKO, 0x05}, /*"=0101： 选择为 _ADC_CLKO"*/
    {_UART3, 0x06}, /*"=0110; 选择为 TX3"*/
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 选择为 7816_CLK"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT1"*/
    {_SPI2, 0x06}, /*"=0110： 选择为 SPI2_SCLK"*/
    {_SPI1, 0x04}, /*"=0100： 选择为 SPI1_SCSN"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 选择为 7816_CLK"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT1"*/
    {_SPI1, 0x04}, /*"=0100： 选择为 SPI1_SCSN"*/
#endif
};
/*"注 1： PC40 仅在 SPI 从模式下有效， 在 SPI 主模式下不要配置为 1。
注 2： 当 P31 和 P40 都选择为 INT1 时， P31 优先级高于 P40，P40 输入无效"*/
#endif
/*"***************************** P4_1 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2 /*"在VBAT域，不能使用bitband"*/
static const sGPIOFunReg_TypeDef P4_1_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /* 由{PCB2[3:2],PCB[1]}定义 P41 端口复用配置： */
    {GPIO_PCB, _bit1},  /*"BIT0"*/
    {GPIO_PCB2, _bit2}, /*"BIT1"*/
    {GPIO_PCB2, _bit3}, /*"BIT2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCB3[1],PCB2[3:2],PCB[1]}定义端口 P41 复用配置："*/
    {GPIO_PCB, _bit1},  /*"BIT0"*/
    {GPIO_PCB2, _bit2}, /*"BIT1"*/
    {GPIO_PCB2, _bit3}, /*"BIT2"*/
    {GPIO_PCB3, _bit1}, /*"BIT3"*/
#else
    /*"由{PCB3[1],PCB2[3:2],PCB[1]}定义端口 P41 复用配置："*/
    {GPIO_PCB, _bit1},  /*"BIT0"*/
    {GPIO_PCB2, _bit2}, /*"BIT1"*/
    {GPIO_PCB2, _bit3}, /*"BIT2"*/
#endif
};
static const sGPIOFunCll_TypeDef P4_1_Fun[] = {
#ifdef RN831x_RN861x_MCU_V3
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 选择为 78160_IO"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT3"*/
    {_UART6, 0x03},    /* =011： 选择为 TX6 */
    {_SPI1, 0x04},     /* =100： 选择为 SPI1_SCLK */
    {_SPI0, 0x05},     /* =101： 选择为 SPI0_SCLK */
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 选择为 78160_IO"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT3"*/
    {_SPI2, 0x03},     /*"=0011： 选择为 SPI2_SCLK"*/
    {_SPI1, 0x04},     /*"=0100： 选择为 SPI1_SCLK"*/
    {_IA_IN, 0x05},    /*"=0101： 选择为 IA_IN"*/
    {_IB_IN, 0x06},    /*"=0110： 选择为 IB_IN"*/
                       /*"=其他： 保留 "*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT3"*/
    {_SPI1, 0x04},     /*"=0100： 选择为 SPI1_SCLK"*/
    {_IA_IN, 0x05},    /*"=0101： 选择为 IA_IN"*/
    {_IB_IN, 0x06},    /*"=0110： 选择为 IB_IN"*/
    {_UART3, 0x07},    /*"=0111： 选择为 RX3"*/
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 选择为 78160_IO"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT3"*/
    {_SPI1, 0x04},     /*"=0100： 选择为 SPI1_SCLK"*/
    {_SPI2, 0x06},     /*"=0110： 选择为 SPI2_SCLK"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 选择为 78160_IO"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT3"*/
    {_SPI1, 0x04},     /*"=0100： 选择为 SPI1_SCLK"*/
#endif
};
/*"注： 当 P33 和 P41 都选择为 INT3 时， P33 优先级高于 P41， P41输入无效"*/
#endif
/*"***************************** P4_2 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2 /*"在VBAT域，不能使用bitband"*/
static const sGPIOFunReg_TypeDef P4_2_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /* 由{PCB2[5:4],PCB[2]}定义 P42 端口复用配置： */
    {GPIO_PCB, _bit2},  /*"BIT0"*/
    {GPIO_PCB2, _bit4}, /*"BIT1"*/
    {GPIO_PCB2, _bit5}, /*"BIT2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCB3[2],PCB2[5:4],PCB[2]}定义端口 P42 复用配置： "*/
    {GPIO_PCB, _bit2},  /*"BIT0"*/
    {GPIO_PCB2, _bit4}, /*"BIT1"*/
    {GPIO_PCB2, _bit5}, /*"BIT2"*/
    {GPIO_PCB3, _bit2}, /*"BIT3"*/
#else
    /*"由{PCB3[2],PCB2[5:4],PCB[2]}定义端口 P42 复用配置： "*/
    {GPIO_PCB, _bit2},  /*"BIT0"*/
    {GPIO_PCB2, _bit4}, /*"BIT1"*/
    {GPIO_PCB2, _bit5}, /*"BIT2"*/
#endif
};
static const sGPIOFunCll_TypeDef P4_2_Fun[] = {
#ifdef RN831x_RN861x_MCU_V3
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 78161_IO"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT4*/
    {_UART7, 0x03},    /* =011： 选择为 RX7 */
    {_SPI1, 0x04},     /* =100： 选择为 SPI1_MISO */
    {_SPI0, 0x05},     /* =101： 选择为 SPI0_MISO */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 78161_IO"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT4*/
    {_SPI1, 0x04},     /*"=0100： 选择为 SPI1_MISO"*/
    {_SPI2, 0x06},     /*"=0110： 选择为 SPI2_MISO"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 78161_IO"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT4*/
    {_SPI2, 0x03},     /*"=0011： 选择为 SPI2_MISO"*/
    {_SPI1, 0x04},     /*"=0100： 选择为 SPI1_MISO"*/
    {_IA_IN, 0x05},    /*"=0101： 选择为 IA_IN"*/
    {_U_IN, 0x06},     /*"=0110： 选择为 U_IN"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT4*/
    {_SPI1, 0x04},     /*"=0100： 选择为 SPI1_MISO"*/
    {_IA_IN, 0x05},    /*"=0101： 选择为 IA_IN"*/
    {_UART4, 0x07},    /*"=0111： 选择为 TX4"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 78161_IO"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT4*/
    {_SPI1, 0x04},     /*"=0100： 选择为 SPI1_MISO"*/
#endif
};
/*"注： 当 P34 和 P42 都选择为 INT4 时， P34 优先级高于 P42， P42输入无效 "*/
#endif
/*"***************************** P4_3 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2 /*"在VBAT域，不能使用bitband"*/
static const sGPIOFunReg_TypeDef P4_3_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /*"由{PCB3[3],PCB2[7:6],PCB[3]}定义端口 P43 复用配置："*/
    {GPIO_PCB, _bit3},  /*"BIT0"*/
    {GPIO_PCB2, _bit6}, /*"BIT1"*/
    {GPIO_PCB2, _bit7}, /*"BIT2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCB3[3],PCB2[7:6],PCB[3]}定义端口 P43 复用配置："*/
    {GPIO_PCB, _bit3},  /*"BIT0"*/
    {GPIO_PCB2, _bit6}, /*"BIT1"*/
    {GPIO_PCB2, _bit7}, /*"BIT2"*/
    {GPIO_PCB3, _bit3}, /*"BIT3"*/
#else
    /*"由{PCB3[3],PCB2[7:6],PCB[3]}定义端口 P43 复用配置："*/
    {GPIO_PCB, _bit3},  /*"BIT0"*/
    {GPIO_PCB2, _bit6}, /*"BIT1"*/
    {GPIO_PCB2, _bit7}, /*"BIT2"*/
#endif
};
static const sGPIOFunCll_TypeDef P4_3_Fun[] = {
#ifdef RN831x_RN861x_MCU_V3
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 选择为 78161_I"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT5"*/
    {_UART7, 0x03},    /* =011： 选择为 TX7 */
    {_SPI1, 0x04},     /* =100： 选择为 SPI1_MOSI */
    {_SPI0, 0x05},     /* =101： 选择为 SPI0_MOSI */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 选择为 78161_I"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT5"*/
    {_SPI1, 0x04},     /*"=0100： 选择为 SPI1_MOSI"*/
    {_SPI2, 0x06},     /*"=0110： 选择为 SPI2_MOSI"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 选择为 78161_I"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT5"*/
    {_SPI2, 0x03},     /*"=0011： 选择为 SPI2_MOSI"*/
    {_SPI1, 0x04},     /*"=0100： 选择为 SPI1_MOSI"*/
    {_IA_IN, 0x05},    /*"=0101： 选择为 IA_IN"*/
    {_IB_IN, 0x06},    /*"=0110： 选择为 IB_IN"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT5"*/
    {_SPI1, 0x04},     /*"=0100： 选择为 SPI1_MOSI"*/
    {_IA_IN, 0x05},    /*"=0101： 选择为 IA_IN"*/
    {_IB_IN, 0x06},    /*"=0110： 选择为 IB_IN"*/
    {_UART4, 0x07},    /*"=0111： 选择为 RX4"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ISO7816, 0x01},  /*"=0001： 选择为 78161_I"*/
    {_INT, 0x02},      /*"=0010： 选择为 INT5"*/
    {_SPI1, 0x04},     /*"=0100： 选择为 SPI1_MOSI"*/
#endif
};
/*"注： 当 P35 和 P43 都选择为 INT5 时， P35 优先级高于 P43， P43输入无效 "*/
#endif
/*"***************************** P4_4 *****************************"*/
static const sGPIOFunReg_TypeDef P4_4_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCB1[10],PCB1[9],IOCFG_UART_I2C[20], IOCFG_UART_I2C[19], PCB1[1], PCB1[0]"*/
    {GPIO_PCB1, _bit0},            /*"Bit0"*/
    {GPIO_PCB1, _bit1},            /*"Bit1"*/
    {GPIO_IOCFG_UART_I2C, _bit19}, /*"Bit2"*/
    {GPIO_IOCFG_UART_I2C, _bit20}, /*"Bit3"*/
    {GPIO_PCB1, _bit9},            /*"Bit4"*/
    {GPIO_PCB1, _bit10},           /*"Bit5"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /*"由{PCB3[4],PCB2[9:8],PCB[4]}定义端口 P44 复用配置："*/
    {GPIO_PCB, _bit4},  /*"BIT0"*/
    {GPIO_PCB2, _bit8}, /*"BIT1"*/
    {GPIO_PCB2, _bit9}, /*"BIT2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCB3[4],PCB2[9:8],PCB[4]}定义端口 P44 复用配置："*/
    {GPIO_PCB, _bit4},  /*"BIT0"*/
    {GPIO_PCB2, _bit8}, /*"BIT1"*/
    {GPIO_PCB2, _bit9}, /*"BIT2"*/
    {GPIO_PCB3, _bit4}, /*"BIT3"*/
#else
    /*"由{PCB3[4],PCB2[9:8],PCB[4]}定义端口 P44 复用配置："*/
    {GPIO_PCB, _bit4},  /*"BIT0"*/
    {GPIO_PCB2, _bit8}, /*"BIT1"*/
    {GPIO_PCB2, _bit9}, /*"BIT2"*/
#endif
};
static const sGPIOFunCll_TypeDef P4_4_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /* =000000: 选择为IO口； */
    {_AIN, 0x01},      /* =000001: 选择为sigma-delta ADC输入SDAIN0 */
    {_UART4, 0x02},    /* =000010:选择为TX4；配置为UART4 */
    {_CAN, 0x06},      /* =000110:选择为TX4；配置为CAN */
    {_LPUART, 0x0A},   /* =001010:选择为TX4；配置为LPUART */
    {_INT, 0x03},      /* =000011:选择为INT6； */
    {_SPI3, 0x10},     /* =010000:P44 为从模式下的 SCSN3， 主模式仍为 GPIO。注意关联：P45 为 SCLK3;P46 为 MOSI3；P47 为 MISO3。 */
    {_SPI4, 0x20},     /* =100000:P44 为从模式下的 SCSN4， 主模式仍为 GPIO。注意关联：P45 为 SCLK4;P46 为 MOSI4；P47 为 MISO4。 */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_SCSN"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY6"*/
    {_AIN, 0x00},      /*"=0000： P44 可复用为 AIN5， 由模拟外设章节 ANA_PAD 的配置决定， 且优先级最高。"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY6"*/
    {_KEY_OUT, 0x03},  /*"=0011： 选择为按键检测指示输出 KEYOUT"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_SCSN"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY6"*/
    {_ADC_CLKO, 0x03}, /*"=0011： 选择为 _ADC_CLKO"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY6"*/
    {_ADC_CLKO, 0x03}, /*"=0011： 选择为 _ADC_CLKO"*/
    {_SPI3, 0x04},     /*"=0100： 选择为 SPI3_SCSN"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_SCSN"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY6"*/
/*"注 1： PC44 仅在 SPI 从模式下有效， 在 SPI 主模式下不要配置为 1。
注 2： 当 P16 和 P44 都选择为 KEY6 时， P16 优先级高于 P44，P44 输入无效
p44-47 靠近模拟， 原则上不建议用为 spi 功能， 减少对计量影响， 建议使用模拟对边的 gpio 做为 spi。 "*/
#endif
};
/*"***************************** P4_5 *****************************"*/
static const sGPIOFunReg_TypeDef P4_5_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCB1[10],PCB1[9],IOCFG_UART_I2C[20], IOCFG_UART_I2C[19], PCB1[3], PCB1[2]"*/
    {GPIO_PCB1, _bit2},            /*"Bit0"*/
    {GPIO_PCB1, _bit3},            /*"Bit1"*/
    {GPIO_IOCFG_UART_I2C, _bit19}, /*"Bit2"*/
    {GPIO_IOCFG_UART_I2C, _bit20}, /*"Bit3"*/
    {GPIO_PCB1, _bit9},            /*"Bit4"*/
    {GPIO_PCB1, _bit10},           /*"Bit5"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /*"由{PCB3[5],PCB2[11:10],PCB[5]}定义端口 P45 复用配置："*/
    {GPIO_PCB, _bit5},   /*"BIT0"*/
    {GPIO_PCB2, _bit10}, /*"BIT1"*/
    {GPIO_PCB2, _bit11}, /*"BIT2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCB3[5],PCB2[11:10],PCB[5]}定义端口 P45 复用配置："*/
    {GPIO_PCB, _bit5},   /*"BIT0"*/
    {GPIO_PCB2, _bit10}, /*"BIT1"*/
    {GPIO_PCB2, _bit11}, /*"BIT2"*/
    {GPIO_PCB3, _bit5},  /*"BIT3"*/
#else
    /*"由{PCB3[5],PCB2[11:10],PCB[5]}定义端口 P45 复用配置："*/
    {GPIO_PCB, _bit5},   /*"BIT0"*/
    {GPIO_PCB2, _bit10}, /*"BIT1"*/
    {GPIO_PCB2, _bit11}, /*"BIT2"*/
#endif
};
static const sGPIOFunCll_TypeDef P4_5_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /* =000000: 选择为IO口； */
    {_AIN, 0x01},      /* =000001: 选择为sigma-delta ADC输入SDAIN1； */
    {_UART4, 0x02},    /* =000010:   选择为RX4；可配置为UART4 */
    {_CAN, 0x06},      /* =000110:   选择为RX4；可配置CAN */
    {_LPUART, 0x0A},   /* =001010:   选择为RX4；可配置为LPUART */
    {_INT, 0x03},      /* =000011: 选择为INT7； */
    {_SPI3, 0x10},     /* =010000: P45 为 SCLK3;注意关联：P44为从模式下的 SCSN3， 主模式仍为 GPIO。P46 为 MOSI3；P47 为 MISO3。 */
    {_SPI4, 0x20},     /* =100000:P45 为 SCLK4;注意关联：P44 为从模式下的 SCSN4， 主模式仍为 GPIO。P46 为 MOSI4；P47 为 MISO4。 */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_SCLK"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY7"*/
    {_AIN, 0x00},      /*"=0000： P45 可复用为 AIN6， 由模拟外设章节 ANA_PAD 的配置决定， 且优先级最高。"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY7"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_SCLK"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY7"*/
    {_ADC_CLKO, 0x03}, /*"=0011： 选择为 _ADC_CLKO"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_SCLK"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY7"*/
    {_ADC_CLKO, 0x03}, /*"=0011： 选择为 _ADC_CLKO"*/
    {_SPI3, 0x04},     /*"=0100： 选择为 SPI3_SCLK"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_SCLK"*/
    {_KEY, 0x02},      /*"=0010： 选择为 KEY7"*/
/*"注： 当 P17 和 P45 都选择为 KEY7 时， P17 优先级高于 P45， P45输入无效 "*/
#endif
};
/*"***************************** P4_6 *****************************"*/
static const sGPIOFunReg_TypeDef P4_6_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCB1[10],PCB1[9],IOCFG_UART_I2C[20], IOCFG_UART_I2C[19], PCB1[5], PCB1[4]"*/
    {GPIO_PCB1, _bit4},            /*"Bit0"*/
    {GPIO_PCB1, _bit5},            /*"Bit1"*/
    {GPIO_IOCFG_UART_I2C, _bit19}, /*"Bit2"*/
    {GPIO_IOCFG_UART_I2C, _bit20}, /*"Bit3"*/
    {GPIO_PCB1, _bit9},            /*"Bit4"*/
    {GPIO_PCB1, _bit10},           /*"Bit5"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCB2[13:12],PCB[6]}定义 P46 端口复用配置： */
    {GPIO_PCB, _bit6},   /*"BIT0"*/
    {GPIO_PCB2, _bit12}, /*"BIT1"*/
    {GPIO_PCB2, _bit13}, /*"BIT2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCB3[6],PCB2[13:12],PCB[6]}定义端口 P46 复用配置："*/
    {GPIO_PCB, _bit6},   /*"BIT0"*/
    {GPIO_PCB2, _bit12}, /*"BIT1"*/
    {GPIO_PCB2, _bit13}, /*"BIT2"*/
    {GPIO_PCB3, _bit6},  /*"BIT3"*/
#else
    /*"由{PCB3[6],PCB2[13:12],PCB[6]}定义端口 P46 复用配置："*/
    {GPIO_PCB, _bit6},   /*"BIT0"*/
    {GPIO_PCB2, _bit12}, /*"BIT1"*/
    {GPIO_PCB2, _bit13}, /*"BIT2"*/
#endif
};
static const sGPIOFunCll_TypeDef P4_6_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /* =000000: 选择为IO口； */
    {_AIN, 0x01},      /* =000001: 选择为sigma-delta ADC输入SDAIN2； */
    {_UART5, 0x02},    /* =000010: 选择为TX5；可配置为UART5 */
    {_CAN, 0x06},      /* =000110: 选择为TX5；可配置为CAN */
    {_LPUART, 0x0A},   /* =001010: 选择为TX5；可配置为LPUART */
    {_KEY, 0x03},      /* =000011: 选择为KEY6；(PC40选择KEY6较PC46优先级高) */
    {_SPI3, 0x10},     /* =010000: P46 为 MOSI3；注意关联：P44为从模式下的 SCSN3，P45 为 SCLK3; 主模式仍为 GPIO。P47 为 MISO3。 */
    {_SPI4, 0x20},     /* =100000:P46 为 MOSI4；注意关联：P44 为从模式下的 SCSN4， 主模式仍为 GPIO。P45 为 SCLK4;P47 为 MISO4。 */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_MISO"*/
    {_UART4, 0x02},    /*"=0010： 选择为 RX4"*/
    {_CF1, 0x04},      /*"=0100： 选择为电能积分模块 D2F 的输出 CF1"*/
    {_D2F_OUT1, 0x04}, /*"=0100： 选择为电能积分模块 D2F 的输出 D2F_OUT1"*/
    {_CF2, 0x06},      /*"=0110： 选择为电能积分模块 D2F 的输出 CF2"*/
    {_D2F_OUT2, 0x06}, /*"=0110： 选择为电能积分模块 D2F 的输出 D2F_OUT2"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},       /*"=0001： 选择为 SPI0_MISO"*/
    {_UART4, 0x02},      /*"=0010： 选择为 RX4"*/
    {_IOCNT_OUT1, 0x03}, /* =011： 选择为 IOCNT_OUT1 */
    {_D2F_OUT1, 0x04},   /* =100： 选择为 D2F_OUT1 */
    {_IOCNT_OUT0, 0x05}, /* =101： 选择为 IOCNT_OUT0 */
    {_D2F_OUT2, 0x06},   /* =110： 选择为 D2F_OUT2 */
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_MISO"*/
    {_UART4, 0x02},    /*"=0010： 选择为 RX4"*/
    {_CAN_RX, 0x03},   /*"=0011： 选择为 CAN_RX"*/
    {_SPI3, 0x04},     /*"=0100： 选择为 SPI3_MISO"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_UART4, 0x02},    /*"=0010： 选择为 RX4"*/
    {_SPI3, 0x04},     /*"=0100： 选择为 SPI3_MISO"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_MISO"*/
    {_UART4, 0x02},    /*"=0010： 选择为 RX4"*/
#endif
};
/*"***************************** P4_7 *****************************"*/
static const sGPIOFunReg_TypeDef P4_7_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCB1[10],PCB1[9],IOCFG_UART_I2C[20], IOCFG_UART_I2C[19], IOCFG_IOCNT[17], PCB1[8], PCB1[7], PCB1[6]"*/
    {GPIO_PCB1, _bit6},            /*"Bit0"*/
    {GPIO_PCB1, _bit7},            /*"Bit1"*/
    {GPIO_PCB1, _bit8},            /*"Bit2"*/
    {GPIO_IOCFG_IOCNT, _bit17},    /*"Bit3"*/
    {GPIO_IOCFG_UART_I2C, _bit19}, /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit20}, /*"Bit5"*/
    {GPIO_PCB1, _bit9},            /*"Bit6"*/
    {GPIO_PCB1, _bit10},           /*"Bit7"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCB2[15:14],PCB[7]}定义 P47 端口复用配置： */
    {GPIO_PCB, _bit7},   /*"BIT0"*/
    {GPIO_PCB2, _bit14}, /*"BIT1"*/
    {GPIO_PCB2, _bit15}, /*"BIT2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCB3[7],PCB2[15:14],PCB[7]}定义端口 P47 复用配置："*/
    {GPIO_PCB, _bit7},   /*"BIT0"*/
    {GPIO_PCB2, _bit14}, /*"BIT1"*/
    {GPIO_PCB2, _bit15}, /*"BIT2"*/
    {GPIO_PCB3, _bit7},  /*"BIT3"*/
#else
    /*"由{PCB3[7],PCB2[15:14],PCB[7]}定义端口 P47 复用配置："*/
    {GPIO_PCB, _bit7},   /*"BIT0"*/
    {GPIO_PCB2, _bit14}, /*"BIT1"*/
    {GPIO_PCB2, _bit15}, /*"BIT2"*/
#endif
};
static const sGPIOFunCll_TypeDef P4_7_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00},   /* =00000000: 选择为IO口； */
    {_AIN, 0x01},        /* =00000001: 选择为sigma-delta ADC输入SDAIN3； */
    {_UART5, 0x02},      /* =00000010: 选择为RX5；可配置为UART5 */
    {_CAN, 0x12},        /* =00010010: 选择为RX5；可配置为CAN */
    {_LPUART, 0x22},     /* =00100010: 选择为RX5；可配置为LPUART */
    {_KEY, 0x03},        /* =00000011: 选择为KEY7；(PC41选择KEY7较PC47优先级高) */
    {_CF_OUT4, 0x04},    /* =00000100: 选择为CF4 */
    {_CF4, 0x04},        /* =00000100: 选择为CF4 */
    {_IOCNT_OUT4, 0x0C}, /* =00001100: 选择为IOCNT4 */
    {_CF_OUT0, 0x05},    /* =00000101: 选择为CF0 */
    {_CF0, 0x05},        /* =00000101: 选择为CF0 */
    {_IOCNT_OUT0, 0x0D}, /* =00001101: 选择为IOCNT0 */
    {_RTC_OUT, 0x06},    /* =00000110: 选择为RTC_OUT */
    {_SPI3, 0x40},       /* =01000000: P47 为 MISO3。注意关联：P44为从模式下的 SCSN3， 主模式仍为 GPIO。P45 为 SCLK3;P46 为 MOSI3； */
    {_SPI4, 0x80},       /* =10000000:P47 为 MISO4。注意关联：P44 为从模式下的 SCSN4， 主模式仍为 GPIO。P45 为 SCLK4;P46 为 MOSI4； */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_MOSI"*/
    {_UART4, 0x02},    /*"=0010： 选择为 TX4"*/
    {_CF0, 0x04},      /*"=0100： 选择为电能积分模块 D2F 的输出 CF0"*/
    {_D2F_OUT0, 0x04}, /*"=0100： 选择为电能积分模块 D2F 的输出 D2F_OUT0"*/
    {_CF1, 0x06},      /*"=0110： 选择为电能积分模块 D2F 的输出 CF1"*/
    {_D2F_OUT1, 0x06}, /*"=0110： 选择为电能积分模块 D2F 的输出 D2F_OUT1"*/
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},       /*"=0001： 选择为 SPI0_MOSI"*/
    {_UART4, 0x02},      /*"=0010： 选择为 TX4"*/
    {_IOCNT_OUT2, 0x03}, /* =011： 选择为 IOCNT_OUT2 */
    {_D2F_OUT0, 0x04},   /* =100： 选择为 D2F_OUT0 */
    {_IOCNT_OUT0, 0x05}, /* =101： 选择为 IOCNT_OUT0 */
    {_D2F_OUT1, 0x06},   /* =110： 选择为 D2F_OUT1 */
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_MOSI"*/
    {_UART4, 0x02},    /*"=0010： 选择为 TX4"*/
    {_CAN_TX, 0x03},   /*"=0011： 选择为 CAN_TX"*/
    {_SPI3, 0x04},     /*"=0100： 选择为 SPI3_MOSI"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_UART4, 0x02},    /*"=0010： 选择为 TX4"*/
    {_SPI3, 0x04},     /*"=0100： 选择为 SPI3_MOSI"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_MOSI"*/
    {_UART4, 0x02},    /*"=0010： 选择为 TX4"*/
#endif
};
/*"***************************** P5_0 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P5_0_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /* 由{PCB2[17:16],PCB[8]}定义 P50 端口复用配置： */
    {GPIO_PCB, _bit8},   /*"BIT0"*/
    {GPIO_PCB2, _bit16}, /*"BIT1"*/
    {GPIO_PCB2, _bit17}, /*"BIT2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCB3[8],PCB2[17:16],PCB[8]}定义端口 P50 复用配置："*/
    {GPIO_PCB, _bit8},   /*"BIT0"*/
    {GPIO_PCB2, _bit16}, /*"BIT1"*/
    {GPIO_PCB2, _bit17}, /*"BIT2"*/
    {GPIO_PCB3, _bit8},  /*"BIT3"*/
#else
    /*"由{PCB3[8],PCB2[17:16],PCB[8]}定义端口 P50 复用配置："*/
    {GPIO_PCB, _bit8},   /*"BIT0"*/
    {GPIO_PCB2, _bit16}, /*"BIT1"*/
    {GPIO_PCB2, _bit17}, /*"BIT2"*/
#endif
};
static const sGPIOFunCll_TypeDef P5_0_Fun[] = {
#ifdef RN831x_RN861x_MCU_V3
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_PF, 0x01},         /*"=0001： 选择为有功脉冲输出 PF"*/
    {_RTC_OUT, 0x02},    /*"=0010： 选择为 RTC_OUT"*/
    {_D2F_OUT0, 0x03},   /* =011： 选择为 D2F_OUT0 */
    {_SPI0, 0x04},       /* =100： 选择为 SPI0_SCSN */
    {_IOCNT_OUT1, 0x05}, /* =101： 选择为 IOCNT_OUT1 */
    {_SF1, 0x06},        /* =110： 选择为 SF1 脉冲输出 */
    {_RTC_1S, 0x07},     /* =111： 选择为万年历秒脉冲输出 RTC1S */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_PF, 0x01},       /*"=0001： 选择为有功脉冲输出 PF"*/
    {_RTC_OUT, 0x02},  /*"=0010： 选择为 RTC_OUT"*/
    {_SPI0, 0x04},     /*"=0100： P50 选择为 SPI0_SCSN；"*/
    {_SF1, 0x06},      /*"=0110： SF1（INT6 引脚输出）"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_PF, 0x01},         /*"=0001： 选择为有功脉冲输出 PF"*/
    {_RTC_OUT, 0x02},    /*"=0010： 选择为 RTC_OUT"*/
    {_CF_OUT0, 0x03},    /*"=0011： 选择为 CF_OUT0"*/
    {_SF, 0x04},         /*"=0100： 选择为视在脉冲输出 SF"*/
    {_IOCNT_OUT0, 0x05}, /*"=0101： 选择为 IOCNT_OUT0"*/
    {_QF, 0x06},         /*"=0110： 选择为无功脉冲输出 QF"*/
    {_D2F_OUT0, 0x07},   /*"=0111： 选择为 D2F_OUT0"*/
    {_RTC_1S, 0x08},     /*"=1000： 选择为万年历秒输出 RTC1S"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_PF, 0x01},         /*"=0001： 选择为有功脉冲输出 PF"*/
    {_RTC_OUT, 0x02},    /*"=0010： 选择为 RTC_OUT"*/
    {_SF, 0x04},         /*"=0100： 选择为视在脉冲输出 SF"*/
    {_IOCNT_OUT0, 0x05}, /*"=0101： 选择为 IOCNT_OUT0"*/
    {_QF, 0x06},         /*"=0110： 选择为无功脉冲输出 QF"*/
    {_RTC_1S, 0x08},     /*"=1000： 选择为万年历秒输出 RTC1S"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_PF, 0x01},       /*"=0001： 选择为有功脉冲输出 PF"*/
    {_RTC_OUT, 0x02},  /*"=0010： 选择为 RTC_OUT"*/
#if defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_D)
    {_SF, 0x04}, /*"=0100： 选择为视在脉冲输出 SF"*/
    {_QF, 0x06}, /*"=0110： 选择为无功脉冲输出 QF"*/
#endif
#endif
};
#endif
/*"***************************** P5_1 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P5_1_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /* 由{PCB2[19:18],PCB[9]}定义 P51 端口复用配置： */
    {GPIO_PCB, _bit9},   /*"BIT0"*/
    {GPIO_PCB2, _bit18}, /*"BIT1"*/
    {GPIO_PCB2, _bit19}, /*"BIT2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCB3[9],PCB2[19:18],PCB[9]}定义端口 P51 复用配置："*/
    {GPIO_PCB, _bit9},   /*"BIT0"*/
    {GPIO_PCB2, _bit18}, /*"BIT1"*/
    {GPIO_PCB2, _bit19}, /*"BIT2"*/
    {GPIO_PCB3, _bit9},  /*"BIT3"*/
#else
    /*"由{PCB3[9],PCB2[19:18],PCB[9]}定义端口 P51 复用配置："*/
    {GPIO_PCB, _bit9},   /*"BIT0"*/
    {GPIO_PCB2, _bit18}, /*"BIT1"*/
    {GPIO_PCB2, _bit19}, /*"BIT2"*/
#ifdef RN821x_RN721x_SOC_V2
    {GPIO_PCB3, _bit9}, /*"BIT3"*/
#endif
#endif
};
static const sGPIOFunCll_TypeDef P5_1_Fun[] = {
#ifdef RN831x_RN861x_MCU_V3
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_QF, 0x01},         /*"=0001： 选择为无功脉冲输出 QF"*/
    {_RTC_OUT, 0x02},    /*"=0010： 选择为 RTC_OUT"*/
    {_D2F_OUT1, 0x03},   /* =011： 选择为 D2F_OUT1 */
    {_SPI0, 0x04},       /* =100： 选择为 SPI0_SCLK */
    {_IOCNT_OUT2, 0x05}, /* =101： 选择为 IOCNT_OUT2 */
    {_SF2, 0x06},        /* =110： 选择为 SF2 脉冲输出 */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_QF, 0x01},       /*"=0001： 选择为无功脉冲输出 QF"*/
    {_RTC_OUT, 0x02},  /*"=0010： 选择为 RTC_OUT"*/
    {_SPI0, 0x04},     /*"=0100： P51 选择为 SPI0_SCLK；"*/
    {_SF2, 0x06},      /*"=0110： SF2； (INT7 引脚输出)"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_QF, 0x01},         /*"=0001： 选择为无功脉冲输出 QF"*/
    {_RTC_OUT, 0x02},    /*"=0010： 选择为 RTC_OUT"*/
    {_CF_OUT1, 0x03},    /*"=0011： 选择为 CF_OUT1"*/
    {_PF, 0x04},         /*"=0100： 选择为有功脉冲输出 PF"*/
    {_IOCNT_OUT1, 0x05}, /*"=0101： 选择为 IOCNT_OUT1"*/
    {_SF, 0x06},         /*"=0110： 选择为视在脉冲输出 SF"*/
    {_D2F_OUT1, 0x07},   /*"=0111： 选择为 D2F_OUT1"*/
    {_RTC_1S, 0x08},     /*"=1000： 选择为万年历秒输出 RTC1S "*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_QF, 0x01},         /*"=0001： 选择为无功脉冲输出 QF"*/
    {_RTC_OUT, 0x02},    /*"=0010： 选择为 RTC_OUT"*/
    {_PF, 0x04},         /*"=0100： 选择为有功脉冲输出 PF"*/
    {_IOCNT_OUT1, 0x05}, /*"=0101： 选择为 IOCNT_OUT1"*/
    {_SF, 0x06},         /*"=0110： 选择为视在脉冲输出 SF"*/
    {_RTC_1S, 0x08},     /*"=1000： 选择为万年历秒输出 RTC1S "*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_QF, 0x01},       /*"=0001： 选择为无功脉冲输出 QF"*/
    {_RTC_OUT, 0x02},  /*"=0010： 选择为 RTC_OUT"*/
#if defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_D)
    {_PF, 0x04}, /*"=0100： 选择为有功脉冲输出 PF"*/
    {_SF, 0x06}, /*"=0110： 选择为视在脉冲输出 SF"*/
#endif
#endif
};
#endif
/*"***************************** P5_2 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P5_2_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /* 由{PCB2[21:20],PCB[10]}定义 P52 端口复用配置： */
    {GPIO_PCB, _bit10},  /*"BIT0"*/
    {GPIO_PCB2, _bit20}, /*"BIT1"*/
    {GPIO_PCB2, _bit21}, /*"BIT2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCB3[10],PCB2[21:20],PCB[10]}定义端口 P52 复用配置："*/
    {GPIO_PCB, _bit10},  /*"BIT0"*/
    {GPIO_PCB2, _bit20}, /*"BIT1"*/
    {GPIO_PCB2, _bit21}, /*"BIT2"*/
    {GPIO_PCB3, _bit10}, /*"BIT3"*/
#else
    /*"由{PCB3[10],PCB2[21:20],PCB[10]}定义端口 P52 复用配置："*/
    {GPIO_PCB, _bit10},  /*"BIT0"*/
    {GPIO_PCB2, _bit20}, /*"BIT1"*/
    {GPIO_PCB2, _bit21}, /*"BIT2"*/
#endif
};
static const sGPIOFunCll_TypeDef P5_2_Fun[] = {
#ifdef RN831x_RN861x_MCU_V3
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_IIC, 0x01},      /*"=0001： 选择为 I2C_SCL"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_TC0_N_1, 0x04},  /* =100： 选择为 TC0_N[1] */
    {_SPI1, 0x06},     /* =110： 选择为 SPI1_SCSN */
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_IIC, 0x01},      /*"=0001： 选择为 I2C_SCL"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_ADC_CLKO, 0x03}, /*"=0011： 选择为 ADC_CLKO"*/
    {_TC0_N_1, 0x04},  /*"=0100： 选择为 TC0_N[1]"*/
                       /*"=0101： 保留"*/
    {_SPI1, 0x06},     /*"=0110： 选择为 SPI1_SCSN"*/
                       /*"=0111： 保留"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_IIC, 0x01},      /*"=0001： 选择为 I2C_SCL"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_ADC_CLKO, 0x03}, /*"=0011： 选择为 ADC_CLKO"*/
    {_SPI1, 0x06},     /*"=0110： 选择为 SPI1_SCSN"*/
    {_TC1_N_0, 0x07},  /*"=0111： 选择为 TC1_N[0]"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_IIC, 0x01},      /*"=0001： 选择为 I2C_SCL"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_TC0_N_1, 0x04},  /*"=0100： 选择为 TC0_N[1]"*/
                       /*"=0101： 保留"*/
    {_SPI1, 0x06},     /*"=0110： 选择为 SPI1_SCSN"*/
                       /*"=0111： 保留"*/
#endif
};
/*"注： PC52 仅在 SPI 从模式下有效， 在 SPI 主模式下不要配置为1。"*/
#endif
/*"***************************** P5_3 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P5_3_FunReg[] = {
    /*"由{PCB3[11],PCB2[23:22],PCB[11]}定义端口 P53 复用配置："*/
    {GPIO_PCB, _bit11},  /*"BIT0"*/
    {GPIO_PCB2, _bit22}, /*"BIT1"*/
    {GPIO_PCB2, _bit23}, /*"BIT2"*/
#if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {GPIO_PCB3, _bit11}, /*"BIT3"*/
#endif
};
static const sGPIOFunCll_TypeDef P5_3_Fun[] = {
#if defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_IIC, 0x01},      /*"=0001： 选择为 I2C_SDA"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_IA_IN, 0x03},    /*"=0011： 选择为 IA_IN"*/
    {_IB_IN, 0x05},    /*"=0101： 选择为 IB_IN"*/
    {_SPI1, 0x06},     /*"=0110： 选择为 SPI1_SCLK"*/
    {_TC1_P_0, 0x07},  /*"=0111： 选择为 TC1_P[0]"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_IIC, 0x01},      /*"=0001： 选择为 I2C_SDA"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_IA_IN, 0x03},    /*"=0011： 选择为 IA_IN"*/
    {_TC0_P_1, 0x04},  /*"=0100： 选择为 TC0_P[1]"*/
    {_IB_IN, 0x05},    /*"=0101： 选择为 IB_IN"*/
    {_SPI1, 0x06},     /*"=0110： 选择为 SPI1_SCLK"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_IIC, 0x01},      /*"=0001： 选择为 I2C_SDA"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_TC0_P_1, 0x04},  /*"=0100： 选择为 TC0_P[1]"*/
    {_SPI1, 0x06},     /*"=0110： 选择为 SPI1_SCLK"*/
#endif
};
#endif
/*"***************************** P5_4 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
/*"由{PCB3[12],PCB2[25:24],PCB[12]}定义端口 P54 复用配置："*/
static const sGPIOFunReg_TypeDef P5_4_FunReg[] = {
    {GPIO_PCB, _bit12},  /*"BIT0"*/
    {GPIO_PCB2, _bit24}, /*"BIT1"*/
    {GPIO_PCB2, _bit25}, /*"BIT2"*/
#if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {GPIO_PCB3, _bit12}, /*"BIT3"*/
#endif
};
static const sGPIOFunCll_TypeDef P5_4_Fun[] = {
#if defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_UART5, 0x01},    /*"=0001： 选择为 RX5"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_IA_IN, 0x03},    /*"=0011： 选择为 IA_IN"*/
    {_TC1_N_0, 0x04},  /*"=0100： 选择为 TC1_N[0]"*/
    {_U_IN, 0x05},     /*"=0101： 选择为 U_IN"*/
    {_SPI1, 0x06},     /*"=0110： 选择为 SPI1_MISO"*/
    {_CAN_RX, 0x07},   /*"=0111： 选择为 CAN_RX"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_UART5, 0x01},    /*"=0001： 选择为 RX5"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_IA_IN, 0x03},    /*"=0011： 选择为 IA_IN"*/
    {_TC1_N_0, 0x04},  /*"=0100： 选择为 TC1_N[0]"*/
    {_SPI1, 0x06},     /*"=0110： 选择为 SPI1_MISO"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_UART5, 0x01},    /*"=0001： 选择为 RX5"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_TC1_N_0, 0x04},  /*"=0100： 选择为 TC1_N[0]"*/
    {_SPI1, 0x06},     /*"=0110： 选择为 SPI1_MISO"*/
#endif
};
#endif
/*"***************************** P5_5 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P5_5_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /*"由{PCB3[13],PCB2[27:26],PCB[13]}定义端口 P55 复用配置"*/
    {GPIO_PCB, _bit13},  /*"BIT0"*/
    {GPIO_PCB2, _bit26}, /*"BIT1"*/
    {GPIO_PCB2, _bit27}, /*"BIT2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCB3[13],PCB2[27:26],PCB[13]}定义端口 P55 复用配置"*/
    {GPIO_PCB, _bit13},  /*"BIT0"*/
    {GPIO_PCB2, _bit26}, /*"BIT1"*/
    {GPIO_PCB2, _bit27}, /*"BIT2"*/
    {GPIO_PCB3, _bit13}, /*"BIT3"*/
#else
    /*"由{PCB3[13],PCB2[27:26],PCB[13]}定义端口 P55 复用配置"*/
    {GPIO_PCB, _bit13},  /*"BIT0"*/
    {GPIO_PCB2, _bit26}, /*"BIT1"*/
    {GPIO_PCB2, _bit27}, /*"BIT2"*/
#endif
};
static const sGPIOFunCll_TypeDef P5_5_Fun[] = {
#ifdef RN831x_RN861x_MCU_V3
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_UART5, 0x01},    /*"=0001： 选择为 TX5"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_TC0_P_0, 0x03},  /*"=011： 选择为 TC0_P[0]"*/
    {_SPI1, 0x06},     /* =110： 选择为 SPI1_MOSI */
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_UART5, 0x01},    /*"=0001： 选择为 TX5"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_IA_IN, 0x03},    /*"=0011： 选择为 IA_IN"*/
    {_TC1_P_0, 0x04},  /*"=0100： 选择为 TC1_P[0]"*/
    {_IB_IN, 0x05},    /*"=0101： 选择为 IB_IN"*/
    {_SPI1, 0x06},     /*"=0110： 选择为 SPI1_MOSI"*/
    {_CAN_TX, 0x07},   /*"=0111： 选择为 CAN_TX"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_UART5, 0x01},    /*"=0001： 选择为 TX5"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_IA_IN, 0x03},    /*"=0011： 选择为 IA_IN"*/
    {_TC1_P_0, 0x04},  /*"=0100： 选择为 TC1_P[0]"*/
    {_IB_IN, 0x05},    /*"=0101： 选择为 IB_IN"*/
    {_SPI1, 0x06},     /*"=0110： 选择为 SPI1_MOSI"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_UART5, 0x01},    /*"=0001： 选择为 TX5"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_TC1_P_0, 0x04},  /*"=0100： 选择为 TC1_P[0]"*/
    {_SPI1, 0x06},     /*"=0110： 选择为 SPI1_MOSI"*/
#endif
};
#endif
/*"***************************** P5_6 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P5_6_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /*"由{PCB3[14],PCB2[29:28],PCB[14]}定义端口 P56 复用配置："*/
    {GPIO_PCB, _bit14},  /*"BIT0"*/
    {GPIO_PCB2, _bit28}, /*"BIT1"*/
    {GPIO_PCB2, _bit29}, /*"BIT2"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCB3[14],PCB2[29:28],PCB[14]}定义端口 P56 复用配置："*/
    {GPIO_PCB, _bit14},  /*"BIT0"*/
    {GPIO_PCB2, _bit28}, /*"BIT1"*/
    {GPIO_PCB2, _bit29}, /*"BIT2"*/
    {GPIO_PCB3, _bit14}, /*"BIT3"*/
#else
    /*"由{PCB3[14],PCB2[29:28],PCB[14]}定义端口 P56 复用配置："*/
    {GPIO_PCB, _bit14},  /*"BIT0"*/
    {GPIO_PCB2, _bit28}, /*"BIT1"*/
    {GPIO_PCB2, _bit29}, /*"BIT2"*/
#endif
};
static const sGPIOFunCll_TypeDef P5_6_Fun[] = {
#ifdef RN831x_RN861x_MCU_V3
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_TC0_N_1, 0x03},  /* =011： 选择为 TC0_N[1] */
    {_RTC_1S, 0x05},   /* =101： 选择为万年历秒脉冲输出 RTC1S */
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_ZXOUT, 0x01},      /*"=0001： 选择为过零输出 ZX_OUT"*/
    {_TC_IN, 0x02},      /*"=0010： 选择为 TCIN"*/
    {_INT, 0x03},        /*"=0011： 选择为 INT2"*/
    {_TC1_N_1, 0x04},    /*"=0100： 选择为 TC1_N[1]"*/
    {_RTC_OUT, 0x05},    /*"=0101： 选择为 RTC_OUT"*/
    {_IOCNT_OUT0, 0x06}, /*"=0110： 选择为 IOCNT_OUT0"*/
    {_ADC_CLKO, 0x07},   /*"=0111： 选择为 ADC_CLKO"*/
    {_RTC_1S, 0x08},     /*"=1000： 选择为万年历秒输出 RTC1S"*/
/*"注： 当 P32 和 P56 都选择为 INT2 时， P32 优先级高于 P56， P56输入无效"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ZXOUT, 0x01},    /*"=0001： 选择为过零输出 ZX_OUT"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_TC1_N_1, 0x04},  /*"=0100： 选择为 TC1_N[1]"*/
#endif
};
#endif
/*"***************************** P5_7 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
/*"由{PCB3[15],PCB2[31:30],PCB[15]}定义端口 P57 复用配置："*/
static const sGPIOFunReg_TypeDef P5_7_FunReg[] = {
    {GPIO_PCB, _bit15},  /*"BIT0"*/
    {GPIO_PCB2, _bit30}, /*"BIT1"*/
    {GPIO_PCB2, _bit31}, /*"BIT2"*/
#if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {GPIO_PCB3, _bit15}, /*"BIT3"*/
#endif
};
static const sGPIOFunCll_TypeDef P5_7_Fun[] = {
#if defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_SF, 0x01},         /*"=0001： 选择为视在脉冲输出 SF"*/
    {_TC_IN, 0x02},      /*"=0010： 选择为 TCIN"*/
    {_CF_OUT2, 0x03},    /*"=0011： 选择为 CF_OUT2"*/
    {_TC1_P_1, 0x04},    /*"=0100： 选择为 TC1_P[1]"*/
    {_IOCNT_OUT2, 0x05}, /*"=0101： 选择为 IOCNT_OUT2"*/
    {_D2F_OUT2, 0x06},   /*"=0110： 选择为 D2F_OUT2"*/
    {_ADC_CLKO, 0x07},   /*"=0111： 选择为 ADC_CLKO"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_SF, 0x01},         /*"=0001： 选择为视在脉冲输出 SF"*/
    {_TC_IN, 0x02},      /*"=0010： 选择为 TCIN"*/
    {_TC1_P_1, 0x04},    /*"=0100： 选择为 TC1_P[1]"*/
    {_IOCNT_OUT2, 0x05}, /*"=0101： 选择为 IOCNT_OUT2"*/
    {_ADC_CLKO, 0x07},   /*"=0111： 选择为 ADC_CLKO"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SF, 0x01},       /*"=0001： 选择为视在脉冲输出 SF"*/
    {_TC_IN, 0x02},    /*"=0010： 选择为 TCIN"*/
    {_TC1_P_1, 0x04},  /*"=0100： 选择为 TC1_P[1]"*/
#endif
};
#endif
/*"***************************** P6_0 *****************************"*/
static const sGPIOFunReg_TypeDef P6_0_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[10], IOCFG_UART_I2C[9], IOCFG_UART_I2C[8], IOCFG_IOCNT[18], PCB1[29], PCB1[28], PCB1[13], PCB1[12]"*/
    {GPIO_PCB1, _bit12},           /*"Bit0"*/
    {GPIO_PCB1, _bit13},           /*"Bit1"*/
    {GPIO_PCB1, _bit28},           /*"Bit2"*/
    {GPIO_PCB1, _bit29},           /*"Bit3"*/
    {GPIO_IOCFG_IOCNT, _bit18},    /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit8},  /*"Bit5"*/
    {GPIO_IOCFG_UART_I2C, _bit9},  /*"Bit6"*/
    {GPIO_IOCFG_UART_I2C, _bit10}, /*"Bit7"*/
#else
    /*"PCB[16]"*/
    {GPIO_PCB, _bit16}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P6_0_Fun[] = {
    {_NORMALIO, 0x00},   /* =00000000: 选择为IO口； */
    {_AIN, 0x01},        /* =00000001:选择为sigma-delta ADC输入SDAIN8； */
    {_CF_OUT0, 0x02},    /* =00000010: CF0 */
    {_CF0, 0x02},        /* =00000010: CF0 */
    {_IOCNT_OUT0, 0x12}, /* =00010010: IOCNT0 */
    {_RTC_OUT, 0x03},    /* =00000011:RTC_OUT； */
    {_UART0, 0x04},      /* =00000100: P61/P60 为 TX/RX ，配 置 为 UART0 */
    {_UART1, 0x24},      /* =00100100: P61/P60 为 TX/RX ,配 置 为UART1 */
    {_UART2, 0x44},      /* =01000100: P61/P60 为 TX/RX ,配 置 为UART2 */
    {_UART3, 0x64},      /* =01100100: P61/P60 为 TX/RX ,配 置 为UART3 */
    {_UART4, 0x84},      /* =10000100: P61/P60 为 TX/RX ,配 置 为UART4 */
    {_UART5, 0xA4},      /* =10100100: P61/P60 为 TX/RX ,配 置 为UART5 */
    {_LPUART, 0xC4},     /* =11000100: P61/P60 为 TX/RX ,配 置 为 LPUART */
    {_CAN, 0xE4},        /* =11100100: P61/P60 为 TX/RX ,配 置 为 CAN */
    {_SPI3, 0x08},       /* =00001000:P61 为 SPI3 从模式下的 SCSN3， SPI3 主模式仍为 GPIO；P60 为 SCLK3。 */
    {_SPI4, 0x0C},       /* =00001100:P61 为 SPI4 从模式下的 SCSN4， SPI4 主模式下仍为 GPIO；P60 为 SCLK4。 */
};
#endif
/*"***************************** P6_1 *****************************"*/
static const sGPIOFunReg_TypeDef P6_1_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[10], IOCFG_UART_I2C[9], IOCFG_UART_I2C[8], IOCFG_IOCNT[19], PCB1[29], PCB1[28], PCB1[15], PCB1[14]"*/
    {GPIO_PCB1, _bit14},           /*"Bit0"*/
    {GPIO_PCB1, _bit15},           /*"Bit1"*/
    {GPIO_PCB1, _bit28},           /*"Bit2"*/
    {GPIO_PCB1, _bit29},           /*"Bit3"*/
    {GPIO_IOCFG_IOCNT, _bit19},    /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit8},  /*"Bit5"*/
    {GPIO_IOCFG_UART_I2C, _bit9},  /*"Bit6"*/
    {GPIO_IOCFG_UART_I2C, _bit10}, /*"Bit7"*/
#else
    /*"PCB[17]"*/
    {GPIO_PCB, _bit17}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P6_1_Fun[] = {
    {_NORMALIO, 0x00},   /* =000000: 选择为IO口； */
    {_CF_OUT3, 0x01},    /* =000001: CF3 */
    {_CF3, 0x01},        /* =000001: CF3 */
    {_IOCNT_OUT3, 0x11}, /* =010001: IOCNT3 */
    {_RTC_OUT, 0x02},    /* =000010: RTC_OUT； */
    {_UART0, 0x04},      /* =00000100: P61/P60 为 TX/RX ，配 置 为 UART0 */
    {_UART1, 0x24},      /* =00100100: P61/P60 为 TX/RX ,配 置 为UART1 */
    {_UART2, 0x44},      /* =01000100: P61/P60 为 TX/RX ,配 置 为UART2 */
    {_UART3, 0x64},      /* =01100100: P61/P60 为 TX/RX ,配 置 为UART3 */
    {_UART4, 0x84},      /* =10000100: P61/P60 为 TX/RX ,配 置 为UART4 */
    {_UART5, 0xA4},      /* =10100100: P61/P60 为 TX/RX ,配 置 为UART5 */
    {_LPUART, 0xC4},     /* =11000100: P61/P60 为 TX/RX ,配 置 为 LPUART */
    {_CAN, 0xE4},        /* =11100100: P61/P60 为 TX/RX ,配 置 为 CAN */
    {_SPI3, 0x08},       /* =001000:P61 为 SPI3 从模式下的 SCSN3， SPI3 主模式仍为 GPIO；P60 为 SCLK3。 */
    {_SPI4, 0x0C},       /* =001100:P61 为 SPI4 从模式下的 SCSN4， SPI4 主模式下仍为 GPIO；P60 为 SCLK4。 */
};
#endif
/*"***************************** P6_2 *****************************"*/
static const sGPIOFunReg_TypeDef P6_2_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_IOCNT[20], PCB1[17], PCB1[16]"*/
    {GPIO_PCB1, _bit16},        /*"Bit0"*/
    {GPIO_PCB1, _bit17},        /*"Bit1"*/
    {GPIO_IOCFG_IOCNT, _bit20}, /*"Bit3"*/
#else
    /*"PCB[18]"*/
    {GPIO_PCB, _bit18}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P6_2_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000:选择为IO口； */
    {_CF_OUT4, 0x01},    /* =0001: CF4 */
    {_CF4, 0x01},        /* =0001: CF4 */
    {_IOCNT_OUT4, 0x05}, /* =0101: IOCNT4 */
    {_RTC_1S, 0x02},     /* =0010:RTC_1S； */
    {_AIN, 0x03},        /* =0011:选择为CMP1 */
};
#endif
/*"***************************** P6_3 *****************************"*/
static const sGPIOFunReg_TypeDef P6_3_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_IOCNT[21], PCB1[19], PCB1[18]"*/
    {GPIO_PCB1, _bit18},        /*"Bit0"*/
    {GPIO_PCB1, _bit19},        /*"Bit1"*/
    {GPIO_IOCFG_IOCNT, _bit21}, /*"Bit3"*/
#else
    /*"PCB[19]"*/
    {GPIO_PCB, _bit19}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P6_3_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000:选择为IO口； */
    {_CF_OUT4, 0x01},    /* =0001:CF4 */
    {_CF4, 0x01},        /* =0001:CF4 */
    {_IOCNT_OUT4, 0x05}, /* =0101:IOCNT4 */
    {_RTC_OUT, 0x02},    /* =0010:RTC_OUT； */
};
#endif
/*"***************************** P6_4 *****************************"*/
static const sGPIOFunReg_TypeDef P6_4_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_IOCNT[22], PCB1[21], PCB1[20]"*/
    {GPIO_PCB1, _bit20},        /*"Bit0"*/
    {GPIO_PCB1, _bit21},        /*"Bit1"*/
    {GPIO_IOCFG_IOCNT, _bit22}, /*"Bit3"*/
#else
    /*"PCB[20]"*/
    {GPIO_PCB, _bit20}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P6_4_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000: 选择为IO口； */
    {_CF_OUT1, 0x01},    /* =0001: CF1 */
    {_CF1, 0x01},        /* =0001: CF1 */
    {_IOCNT_OUT1, 0x05}, /* =0101: IOCNT1 */
    {_RTC_OUT, 0x02},    /* =0010: RTC_OUT； */
};
#endif
/*"***************************** P6_5 *****************************"*/
static const sGPIOFunReg_TypeDef P6_5_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_IOCNT[23], PCB1[23], PCB1[22]"*/
    {GPIO_PCB1, _bit22},        /*"Bit0"*/
    {GPIO_PCB1, _bit23},        /*"Bit1"*/
    {GPIO_IOCFG_IOCNT, _bit23}, /*"Bit3"*/
#else
    /*"PCB[21]"*/
    {GPIO_PCB, _bit21}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P6_5_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000: 选择为IO口； */
    {_CF_OUT2, 0x01},    /* =0001:  CF2 */
    {_CF2, 0x01},        /* =0001:  CF2 */
    {_IOCNT_OUT2, 0x05}, /* =0101: IOCNT2 */
    {_RTC_1S, 0x02},     /* =0010: RTC_1S； */
};
#endif
/*"***************************** P6_6 *****************************"*/
static const sGPIOFunReg_TypeDef P6_6_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_IOCNT[24], PCB1[25], PCB1[24]"*/
    {GPIO_PCB1, _bit24},        /*"Bit0"*/
    {GPIO_PCB1, _bit25},        /*"Bit1"*/
    {GPIO_IOCFG_IOCNT, _bit24}, /*"Bit3"*/
#else
    /*"PCB[22]"*/
    {GPIO_PCB, _bit22}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P6_6_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000: 选择为IO口； */
    {_CF_OUT3, 0x01},    /* =0001: CF3 */
    {_CF3, 0x01},        /* =0001: CF3 */
    {_IOCNT_OUT3, 0x05}, /* =0101: IOCNT3 */
    {_RTC_OUT, 0x02},    /* =0010: RTC_OUT； */
};
#endif
/*"***************************** P6_7 *****************************"*/
static const sGPIOFunReg_TypeDef P6_7_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_IOCNT[25], PCB1[27], PCB1[26]"*/
    {GPIO_PCB1, _bit26},        /*"Bit0"*/
    {GPIO_PCB1, _bit27},        /*"Bit1"*/
    {GPIO_IOCFG_IOCNT, _bit25}, /*"Bit3"*/
#else
    /*"PCB[23]"*/
    {GPIO_PCB, _bit23}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P6_7_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000:选择为IO口； */
    {_CF_OUT4, 0x01},    /* =0001:CF4 */
    {_CF4, 0x01},        /* =0001:CF4 */
    {_IOCNT_OUT4, 0x05}, /* =0101:IOCNT4 */
    {_RTC_OUT, 0x02},    /* =0010:RTC_OUT； */
};
#endif
/*"***************************** P7_0 *****************************"*/
static const sGPIOFunReg_TypeDef P7_0_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCB3[2], PCB3[1], PCB3[0]"*/
    {GPIO_PCB3, _bit0}, /*"Bit0"*/
    {GPIO_PCB3, _bit1}, /*"Bit1"*/
    {GPIO_PCB3, _bit2}, /*"Bit3"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCB5[2:0],PCB[24]}定义端口 P70 复用配置： */
    {GPIO_PCB, _bit24}, /*"BIT0"*/
    {GPIO_PCB5, _bit0}, /*"BIT1"*/
    {GPIO_PCB5, _bit1}, /*"BIT2"*/
    {GPIO_PCB5, _bit2}, /*"BIT3"*/
#elif defined(RN821x_RN721x_SOC_V3)
    /* 由{PCB5[2:0],PCB[24]}定义端口 P70 复用配置： */
    {GPIO_PCB, _bit24}, /*"BIT0"*/
    {GPIO_PCB5, _bit0}, /*"BIT1"*/
    {GPIO_PCB5, _bit1}, /*"BIT2"*/
    {GPIO_PCB5, _bit2}, /*"BIT3"*/
#else
    /*"PCB[24]"*/
    {GPIO_PCB, _bit24}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P7_0_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_KEY, 0x01},      /* =0001: 选择为KEY，可配置为KEY0-KEY7，由IOCFG_KEY寄存器确定 */
    {_INT, 0x02},      /* =0010: 选择为INT，可配置为INT0-INT7，由IOCFG_INT寄存器确定 */
    {_TC_IN, 0x03},    /* =0011: 选择为TCI； */
    {_ADC_CLKO, 0x04}, /* =0100: 选择为ADCCKO； */
    {_TC1_N_1, 0x05},  /* =0101: 选择为TC1_N[1]； */
};
#elif defined(RN831x_RN861x_MCU_V3)
static const sGPIOFunCll_TypeDef P7_0_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000: 选择为IO口； */
    {_SEG, 0x01},        /* =0001： 选择 LCD */
    {_IIC, 0x02},        /* =0010： 选择 SCL */
    {_IOCNT_OUT0, 0x03}, /* =0100： 选择 IOCNT_OUT0 */
};
#elif defined(RN821x_RN721x_SOC_V3)
static const sGPIOFunCll_TypeDef P7_0_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000: 选择为IO口； */
    {_SEG, 0x01},        /* =0001： 选择 LCD */
};
#endif
/*"***************************** P7_1 *****************************"*/
static const sGPIOFunReg_TypeDef P7_1_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCB3[5], PCB3[4], PCB3[3]"*/
    {GPIO_PCB3, _bit3}, /*"Bit0"*/
    {GPIO_PCB3, _bit4}, /*"Bit1"*/
    {GPIO_PCB3, _bit5}, /*"Bit3"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCB5[5:3],PCB[25]}定义端口 P71 复用配置： */
    {GPIO_PCB, _bit25}, /*"BIT0"*/
    {GPIO_PCB5, _bit3}, /*"BIT1"*/
    {GPIO_PCB5, _bit4}, /*"BIT2"*/
    {GPIO_PCB5, _bit5}, /*"BIT3"*/
#elif defined(RN821x_RN721x_SOC_V3)
/* 由{PCB5[5:3],PCB[25]}定义端口 P71 复用配置 */
    {GPIO_PCB, _bit25}, /*"BIT0"*/
    {GPIO_PCB5, _bit3}, /*"BIT1"*/
    {GPIO_PCB5, _bit4}, /*"BIT2"*/
    {GPIO_PCB5, _bit5}, /*"BIT3"*/
#else
    /*"PCB[25]"*/
    {GPIO_PCB, _bit25}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P7_1_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_KEY, 0x01},      /* =0001: 选择为KEY，可配置为KEY0-KEY7，由IOCFG_KEY寄存器确定； */
    {_INT, 0x02},      /* =0010: 选择为INT，可配置为INT0-INT7，由IOCFG_INT寄存器确定； */
    {_TC_IN, 0x03},    /* =0011: 选择为TCI； */
    {_IC_IN, 0x04},    /* =0100: 选择为ICDIN; */
};
#elif defined(RN831x_RN861x_MCU_V3)
static const sGPIOFunCll_TypeDef P7_1_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000: 选择为IO口； */
    {_SEG, 0x01},        /* =0001： 选择 LCD */
    {_IIC, 0x02},        /* =0010： 选择 SDA */
    {_IOCNT_OUT1, 0x04}, /* =0100： 选择 IOCNT_OUT1 */
};
#elif defined(RN821x_RN721x_SOC_V3)
static const sGPIOFunCll_TypeDef P7_1_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000: 选择为IO口； */
    {_SEG, 0x01},        /* =0001： 选择 LCD */
};
#endif
/*"***************************** P7_2 *****************************"*/
static const sGPIOFunReg_TypeDef P7_2_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCB3[8], PCB3[7], PCB3[6]"*/
    {GPIO_PCB3, _bit6}, /*"Bit0"*/
    {GPIO_PCB3, _bit7}, /*"Bit1"*/
    {GPIO_PCB3, _bit8}, /*"Bit3"*/
#elif defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
    /* 由{PCB5[8:6],PCB[26]}定义端口 P72 复用配置： */
    {GPIO_PCB, _bit26}, /*"BIT0"*/
    {GPIO_PCB5, _bit6}, /*"BIT1"*/
    {GPIO_PCB5, _bit7}, /*"BIT2"*/
    {GPIO_PCB5, _bit8}, /*"BIT3"*/
#else
    /*"PCB[26]"*/
    {GPIO_PCB, _bit26}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P7_2_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_KEY, 0x01},      /* =0001: 选择为KEY，可配置为KEY0-KEY7，由IOCFG_KEY寄存器确定； */
    {_INT, 0x02},      /* =0010: 选择为INT，可配置为INT0-INT7，由IOCFG_INT寄存器确定； */
    {_TC_IN, 0x03},    /* =0011: 选择为TCI； */
    {_IB_IN, 0x04},    /* =0100: 选择为IBDIN; */
};
#elif defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
static const sGPIOFunCll_TypeDef P7_2_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_SEG, 0x01},      /* =0001： 选择 LCD */
    {_KEY, 0x02},      /* =0010： 选择 KEY6（优先级 P44>P72） */
};
#endif
/*"***************************** P7_3 *****************************"*/
static const sGPIOFunReg_TypeDef P7_3_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCB3[11], PCB3[10], PCB3[9]"*/
    {GPIO_PCB3, _bit9},  /*"Bit0"*/
    {GPIO_PCB3, _bit10}, /*"Bit1"*/
    {GPIO_PCB3, _bit11}, /*"Bit3"*/
#elif defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
    /* 由{PCB5[11:9],PCB[27]}定义端口 P73 复用配置： */
    {GPIO_PCB, _bit27},  /*"BIT0"*/
    {GPIO_PCB5, _bit9},  /*"BIT1"*/
    {GPIO_PCB5, _bit10}, /*"BIT2"*/
    {GPIO_PCB5, _bit11}, /*"BIT3"*/
#else
    /*"PCB[27]"*/
    {GPIO_PCB, _bit27}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P7_3_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000：选择为IO口； */
    {_KEY, 0x01},      /* =0001:：选择为KEY，可配置为KEY0-KEY7，由IOCFG_KEY寄存器确定； */
    {_INT, 0x02},      /* =0010:选择为INT，可配置为INT0-INT7，由IOCFG_INT寄存器确定； */
    {_TC_IN, 0x03},    /* =0011:选择为TCI； */
    {_IA_IN, 0x04},    /* =0100:选择为IADIN; */
};
#elif defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
static const sGPIOFunCll_TypeDef P7_3_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000：选择为IO口； */
    {_SEG, 0x01},        /* =0001： 选择 LCD */
    {_IOCNT_OUT1, 0x02}, /* =0010： 选择 IOCNT_OUT1 */
};
#endif
/*"***************************** P7_4 *****************************"*/
static const sGPIOFunReg_TypeDef P7_4_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCB3[14], PCB3[13], PCB3[12]"*/
    {GPIO_PCB3, _bit12}, /*"Bit0"*/
    {GPIO_PCB3, _bit13}, /*"Bit1"*/
    {GPIO_PCB3, _bit14}, /*"Bit3"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCB5[14:12],PCB[28]}定义端口 P74 复用配置： */
    {GPIO_PCB, _bit28},  /*"BIT0"*/
    {GPIO_PCB5, _bit12}, /*"BIT1"*/
    {GPIO_PCB5, _bit13}, /*"BIT2"*/
    {GPIO_PCB5, _bit14}, /*"BIT3"*/
#else
    /*"PCB[28]"*/
    {GPIO_PCB, _bit28}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P7_4_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000:选择为IO口； */
    {_KEY, 0x01},      /* =0001:选择为KEY，可配置为KEY0-KEY7，由IOCFG_KEY寄存器确定； */
    {_INT, 0x02},      /* =0010:选择为INT，可配置为INT0-INT7，由IOCFG_INT寄存器确定； */
    {_TC_IN, 0x03},    /* =0011:选择为TCI； */
    {_IIC0, 0x04},     /* =0100:选择为SDA0，I2C0数据口； */
    {_IIC1, 0x05},     /* =0101:选择为SDA1，I2C1数据口； */
};
#elif defined(RN831x_RN861x_MCU_V3)
static const sGPIOFunCll_TypeDef P7_4_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000:选择为IO口； */
    {_SEG, 0x01},      /* =0001： 选择 LCD */
    {_INT, 0x02},      /* =0010： 选择 INT0（优先级 P30>P10>P74） */
};
#endif
/*"***************************** P7_5 *****************************"*/
static const sGPIOFunReg_TypeDef P7_5_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCB3[17], PCB3[16], PCB3[15]"*/
    {GPIO_PCB3, _bit15}, /*"Bit0"*/
    {GPIO_PCB3, _bit16}, /*"Bit1"*/
    {GPIO_PCB3, _bit17}, /*"Bit3"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCB5[17:15],PCB[29]}定义端口 P75 复用配置： */
    {GPIO_PCB, _bit29},  /*"BIT0"*/
    {GPIO_PCB5, _bit15}, /*"BIT1"*/
    {GPIO_PCB5, _bit16}, /*"BIT2"*/
    {GPIO_PCB5, _bit17}, /*"BIT3"*/
#else
    /*"PCB[29]"*/
    {GPIO_PCB, _bit29}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P7_5_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_KEY, 0x01},      /* =0001: 选择为KEY，可配置为KEY0-KEY7，由IOCFG_KEY寄存器确定； */
    {_INT, 0x02},      /* =0010: 选择为INT，可配置为INT0-INT7，由IOCFG_INT寄存器确定； */
    {_TC_IN, 0x03},    /* =0011: 选择为TCI； */
    {_IIC0, 0x04},     /* =0100: 选择为SCL0，I2C0时钟； */
    {_IIC1, 0x05},     /* =0101: 选择为SCL1，I2C1时钟； */
};
#elif defined(RN831x_RN861x_MCU_V3)
static const sGPIOFunCll_TypeDef P7_5_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_SEG, 0x01},      /* =0001： 选择 LCD */
    {_INT, 0x02},      /* =0010： 选择 INT1（优先级 P40>P11>P75） */
};
#endif
/*"***************************** P7_6 *****************************"*/
static const sGPIOFunReg_TypeDef P7_6_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCB3[20], PCB3[19], PCB3[18]"*/
    {GPIO_PCB3, _bit18}, /*"Bit0"*/
    {GPIO_PCB3, _bit19}, /*"Bit1"*/
    {GPIO_PCB3, _bit20}, /*"Bit3"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCB5[20:18],PCB[30]}定义端口 P76 复用配置： */
    {GPIO_PCB, _bit30},  /*"BIT0"*/
    {GPIO_PCB5, _bit18}, /*"BIT1"*/
    {GPIO_PCB5, _bit19}, /*"BIT2"*/
    {GPIO_PCB5, _bit20}, /*"BIT3"*/
#else
    /*"PCB[30]"*/
    {GPIO_PCB, _bit30}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P7_6_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_KEY, 0x01},      /* =0001: 选择为KEY，可配置为KEY0-KEY7，由IOCFG_KEY寄存器确定； */
    {_INT, 0x02},      /* =0010: 选择为INT，可配置为INT0-INT7，由IOCFG_INT寄存器确定； */
    {_TC_IN, 0x03},    /* =0011: 选择为TCI； */
    {_TC1_P_1, 0x05},  /* =0101: 选择为TC1_P[1]； */
};
#elif defined(RN831x_RN861x_MCU_V3)
static const sGPIOFunCll_TypeDef P7_6_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_SEG, 0x01},      /* =0001： 选择 LCD */
    {_INT, 0x02},      /* =0010： 选择 INT2（优先级 P32>P27>P76） */
};
#endif
/*"***************************** P7_7 *****************************"*/
static const sGPIOFunReg_TypeDef P7_7_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCB3[23], PCB3[22], PCB3[21]"*/
    {GPIO_PCB3, _bit21}, /*"Bit0"*/
    {GPIO_PCB3, _bit22}, /*"Bit1"*/
    {GPIO_PCB3, _bit23}, /*"Bit3"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCB5[23:21],PCB[31]}定义端口 P77 复用配置： */
    {GPIO_PCB, _bit31},  /*"BIT0"*/
    {GPIO_PCB5, _bit21}, /*"BIT1"*/
    {GPIO_PCB5, _bit22}, /*"BIT2"*/
    {GPIO_PCB5, _bit23}, /*"BIT3"*/
#else
    /*"PCB[31]"*/
    {GPIO_PCB, _bit31}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P7_7_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000:选择为IO口； */
    {_KEY, 0x01},      /* =0001:选择为KEY，可配置为KEY0-KEY7，由IOCFG_KEY寄存器确定； */
    {_INT, 0x02},      /* =0010:选择为INT，可配置为INT0-INT7，由IOCFG_INT寄存器确定； */
    {_TC_IN, 0x03},    /* =0011选择为TCI； */
    {_TC1_N_1, 0x04},  /* =0100:选择为TC1_N[1]； */
};
#elif defined(RN831x_RN861x_MCU_V3)
static const sGPIOFunCll_TypeDef P7_7_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000:选择为IO口； */
    {_SEG, 0x01},      /* =0001： 选择 LCD */
    {_INT, 0x02},      /* =0010： 选择 INT3（优先级 P41>P142>P77） */
};
#endif
/*"***************************** P8_0 *****************************"*/
static const sGPIOFunReg_TypeDef P8_0_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[13], IOCFG_UART_I2C[12], IOCFG_UART_I2C[11], PCC[1], PCC[0]"*/
    {GPIO_PCC, _bit0},             /*"Bit0"*/
    {GPIO_PCC, _bit1},             /*"Bit1"*/
    {GPIO_IOCFG_UART_I2C, _bit11}, /*"Bit2"*/
    {GPIO_IOCFG_UART_I2C, _bit12}, /*"Bit3"*/
    {GPIO_IOCFG_UART_I2C, _bit13}, /*"Bit4"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCC4[2:0],PCC[0]}定义端口 P80 复用配置： */
    {GPIO_PCC, _bit0},  /*"BIT0"*/
    {GPIO_PCC4, _bit0}, /*"BIT1"*/
    {GPIO_PCC4, _bit1}, /*"BIT2"*/
    {GPIO_PCC4, _bit2}, /*"BIT3"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCC4[2:0],PCC[0]}定义端口 P80 复用配置"*/
    {GPIO_PCC, _bit0}, /*"BIT0"*/
    {GPIO_PCC4, _bit0}, /*"BIT1"*/
    {GPIO_PCC4, _bit1}, /*"BIT2"*/
    {GPIO_PCC4, _bit2}, /*"BIT3"*/
#else
    /*"由{PCC4[2:0],PCC[0]}定义端口 P80 复用配置"*/
    {GPIO_PCC, _bit0}, /*"BIT0"*/
#endif
};
static const sGPIOFunCll_TypeDef P8_0_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /* =00000:选择为IO口； */
    {_SPI3, 0x01},     /* =00001:选择为SPI3从模式SCSN3，SPI3主模式仍为GPIO； */
    {_SPI4, 0x02},     /* =00010:选择为SPI4从模式SCSN4，SPI4主模式仍为GPIO； */
    {_UART0, 0x03},    /* =00011:选择为TX，配置为UART0 */
    {_UART1, 0x07},    /* =00111:选择为TX，配置为UART1 */
    {_UART2, 0x0B},    /* =01011:选择为TX，配置为UART2 */
    {_UART3, 0x0F},    /* =01111:选择为TX，配置为UART3 */
    {_UART4, 0x13},    /* =10011:选择为TX，配置为UART4 */
    {_UART5, 0x17},    /* =10111:选择为TX，配置为UART5 */
    {_LPUART, 0x1B},   /* =11011:选择为TX，配置为LPUAR */
    {_CAN, 0x1F},      /* =11111:选择为TX，可配置为CAN */
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SEG, 0x01},      /* =0001： 选择 LCD */
    {_INT, 0x02},      /* =0010： 选择 INT4（优先级 P42>P145>P80） */
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
    {_IIC, 0x02}, /*"=0010： 选择为 SCL"*/
                  /*"=其他： 保留"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
    {_IIC, 0x02}, /*"=0010： 选择为 SCL"*/
    {_INT, 0x03}, /*"=0011： 选择为 INT4（优先级 P42>P80）"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
#endif
};
/*"***************************** P8_1 *****************************"*/
/*"由{PCC4[5:3],PCC[1]}定义端口 P81 复用配置"*/
static const sGPIOFunReg_TypeDef P8_1_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[13], IOCFG_UART_I2C[12], IOCFG_UART_I2C[11],  PCC[3], PCC[2]"*/
    {GPIO_PCC, _bit2},             /*"Bit0"*/
    {GPIO_PCC, _bit3},             /*"Bit1"*/
    {GPIO_IOCFG_UART_I2C, _bit11}, /*"Bit2"*/
    {GPIO_IOCFG_UART_I2C, _bit12}, /*"Bit3"*/
    {GPIO_IOCFG_UART_I2C, _bit13}, /*"Bit4"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCC4[5:3],PCC[1]}定义端口 P81 复用配置： */
    {GPIO_PCC, _bit1},  /*"BIT0"*/
    {GPIO_PCC4, _bit3}, /*"BIT1"*/
    {GPIO_PCC4, _bit4}, /*"BIT2"*/
    {GPIO_PCC4, _bit5}, /*"BIT3"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {GPIO_PCC, _bit1},  /*"BIT0"*/
    {GPIO_PCC4, _bit3}, /*"BIT1"*/
    {GPIO_PCC4, _bit4}, /*"BIT2"*/
    {GPIO_PCC4, _bit5}, /*"BIT3"*/
#else
    {GPIO_PCC, _bit1}, /*"BIT0"*/
#endif
};
static const sGPIOFunCll_TypeDef P8_1_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /* =0000:选择为IO口； */
    {_SPI3, 0x01},     /* =0001:选择为SCLK3； */
    {_SPI4, 0x02},     /* =0010:选择为SCLK4； */
    {_UART0, 0x03},    /* =00011:选择为RX，配置为UART0 */
    {_UART1, 0x07},    /* =00111:选择为RX，配置为UART1 */
    {_UART2, 0x0B},    /* =01011:选择为RX，配置为UART2 */
    {_UART3, 0x0F},    /* =01111:选择为RX，配置为UART3 */
    {_UART4, 0x13},    /* =10011:选择为RX，配置为UART4 */
    {_UART5, 0x17},    /* =10111:选择为RX，配置为UART5 */
    {_LPUART, 0x1B},   /* =11011:选择为RX，配置为LPUAR */
    {_CAN, 0x1F},      /* =11111:选择为RX，可配置为CAN */
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
    {_INT, 0x02},      /* =0010： 选择 INT5（优先级 P43>P146>P81） */
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
    {_IIC, 0x02},      /*"=0010： 选择为 SDA"*/
                       /*"=其他： 保留"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
    {_IIC, 0x02},      /*"=0010： 选择为 SDA"*/
    {_INT, 0x03},      /*"=0011： 选择为 INT5（优先级 P43>P81）"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
#endif
};
/*"***************************** P8_2 *****************************"*/
static const sGPIOFunReg_TypeDef P8_2_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[18], IOCFG_UART_I2C[17], IOCFG_UART_I2C[16],  PCC[6], PCC[5], PCC[4]"*/
    {GPIO_PCC, _bit4},             /*"Bit0"*/
    {GPIO_PCC, _bit5},             /*"Bit1"*/
    {GPIO_PCC, _bit6},             /*"Bit2"*/
    {GPIO_IOCFG_UART_I2C, _bit16}, /*"Bit3"*/
    {GPIO_IOCFG_UART_I2C, _bit17}, /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit18}, /*"Bit5"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCC4[8:6],PCC[2]}定义端口 P82 复用配置： */
    {GPIO_PCC, _bit2},  /*"BIT0"*/
    {GPIO_PCC4, _bit6}, /*"BIT1"*/
    {GPIO_PCC4, _bit7}, /*"BIT2"*/
    {GPIO_PCC4, _bit8}, /*"BIT3"*/
#elif defined(RN821x_RN721x_SOC_V3)
    /* 由{PCC4[8:6],PCC[2]}定义端口 P82 复用配置： */
    {GPIO_PCC, _bit2},  /*"BIT0"*/
    {GPIO_PCC4, _bit6}, /*"BIT1"*/
    {GPIO_PCC4, _bit7}, /*"BIT2"*/
    {GPIO_PCC4, _bit8}, /*"BIT3"*/
#else
    /*"PC82 定义端口 P82 复用配置："*/
    {GPIO_PCC, _bit2}, /*"BIT0"*/
#endif
};
static const sGPIOFunCll_TypeDef P8_2_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /* =000000:选择为IO口； */
    {_SPI3, 0x01},     /* =000001: 选择为MOSI3； */
    {_SPI4, 0x02},     /* =000010:选择为MOSI4； */
    {_UART0, 0x03},    /* =000011: 选择为TX，配置为UART0 */
    {_UART1, 0x0B},    /* =001011: 选择为TX，配置为UART1 */
    {_UART2, 0x13},    /* =010011: 选择为TX，配置为UART2 */
    {_UART3, 0x1B},    /* =011011: 选择为TX，配置为UART3 */
    {_UART4, 0x23},    /* =100011: 选择为TX，配置为UART4 */
    {_UART5, 0x2B},    /* =101011: 选择为TX，配置为UART5 */
    {_LPUART, 0x33},   /* =110011: 选择为TX，配置为LPUART */
    {_CAN, 0x3B},      /* =111011: 选择为TX，配置为CAN */
    {_IIC0, 0x04},     /* =000100:选择为SCL0，I2C0时钟 */
    {_IIC1, 0x05},     /* =000101:选择为SCL1，I2C1时钟 */
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0： 选择为 IO 口；"*/
    {_SEG, 0x01},      /*"=1： 选择 LCD。"*/
    {_INT, 0x02},      /* =0010： 选择 INT6（优先级 P36>P21>P82） */
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0： 选择为 IO 口；"*/
    {_SEG, 0x01},      /*"=1： 选择 LCD。"*/
    {_INT, 0x02},      /*"=0010： 选择 INT6（优先级 P36>P82）"*/
#else
    {_NORMALIO, 0x00}, /*"=0： 选择为 IO 口；"*/
    {_SEG, 0x01},      /*"=1： 选择 LCD。"*/
#endif
};
/*"***************************** P8_3 *****************************"*/
static const sGPIOFunReg_TypeDef P8_3_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"IOCFG_UART_I2C[18], IOCFG_UART_I2C[17], IOCFG_UART_I2C[16],  PCC[9], PCC[8], PCC[7]"*/
    {GPIO_PCC, _bit7},             /*"Bit0"*/
    {GPIO_PCC, _bit8},             /*"Bit1"*/
    {GPIO_PCC, _bit9},             /*"Bit2"*/
    {GPIO_IOCFG_UART_I2C, _bit16}, /*"Bit3"*/
    {GPIO_IOCFG_UART_I2C, _bit17}, /*"Bit4"*/
    {GPIO_IOCFG_UART_I2C, _bit18}, /*"Bit5"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCC4[11:9],PCC[3]}定义端口 P83 复用配置： */
    {GPIO_PCC, _bit3},   /*"BIT0"*/
    {GPIO_PCC4, _bit9},  /*"BIT1"*/
    {GPIO_PCC4, _bit10}, /*"BIT2"*/
    {GPIO_PCC4, _bit11}, /*"BIT3"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCC4[11:9],PCC[3]}定义端口 P83 复用配置"*/
    {GPIO_PCC, _bit3},   /*"BIT0"*/
    {GPIO_PCC4, _bit9},  /*"BIT1"*/
    {GPIO_PCC4, _bit10}, /*"BIT2"*/
    {GPIO_PCC4, _bit11}, /*"BIT3"*/
#else
    /*"由{PCC4[11:9],PCC[3]}定义端口 P83 复用配置"*/
    {GPIO_PCC, _bit3}, /*"BIT0"*/
#endif
};
static const sGPIOFunCll_TypeDef P8_3_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /* =0000:选择为IO口； */
    {_SPI3, 0x01},     /* =0001:选择为MISO3； */
    {_SPI4, 0x02},     /* =0010:选择为MISO4； */
    {_UART0, 0x03},    /* =000011: 选择为RX，配置为UART0 */
    {_UART1, 0x0B},    /* =001011: 选择为RX，配置为UART1 */
    {_UART2, 0x13},    /* =010011: 选择为RX，配置为UART2 */
    {_UART3, 0x1B},    /* =011011: 选择为RX，配置为UART3 */
    {_UART4, 0x23},    /* =100011: 选择为RX，配置为UART4 */
    {_UART5, 0x2B},    /* =101011: 选择为RX，配置为UART5 */
    {_LPUART, 0x33},   /* =110011: 选择为RX，配置为LPUART */
    {_CAN, 0x3B},      /* =111011: 选择为RX，配置为CAN */
    {_IIC0, 0x04},     /* =0100:选择为SDA0，I2C0数据 */
    {_IIC1, 0x05},     /* =0101:选择为SDA1，I2C1数据 */
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
    {_INT, 0x02},      /* =0010： 选择 INT7（优先级 P37>P23>P83）  */
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
    {_SPI3, 0x02},     /*"=0010： 选择为 SPI3_SCSN（OD 引脚， 需求作为从机输入）"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
#endif
};
/*"***************************** P8_4 *****************************"*/
static const sGPIOFunReg_TypeDef P8_4_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCC[12], PCC[11], PCC[10]"*/
    {GPIO_PCC, _bit10}, /*"Bit0"*/
    {GPIO_PCC, _bit11}, /*"Bit1"*/
    {GPIO_PCC, _bit12}, /*"Bit2"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCC4[14:12],PCC[4]}定义端口 P84 复用配置： */
    {GPIO_PCC, _bit4},   /*"BIT0"*/
    {GPIO_PCC4, _bit12}, /*"BIT1"*/
    {GPIO_PCC4, _bit13}, /*"BIT2"*/
    {GPIO_PCC4, _bit14}, /*"BIT3"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCC4[14:12],PCC[4]}定义端口 P84 复用配置"*/
    {GPIO_PCC, _bit4},   /*"BIT0"*/
    {GPIO_PCC4, _bit12}, /*"BIT1"*/
    {GPIO_PCC4, _bit13}, /*"BIT2"*/
    {GPIO_PCC4, _bit14}, /*"BIT3"*/
#else
    /*"由{PCC4[14:12],PCC[4]}定义端口 P84 复用配置"*/
    {GPIO_PCC, _bit4}, /*"BIT0"*/
#endif
};
static const sGPIOFunCll_TypeDef P8_4_Fun[] = {
#ifdef RN202x_RN7326_SOC_V2
    {_NORMALIO, 0x00}, /* =0000:选择为IO口； */
    {_KEY, 0x01},      /* =0001:选择为KEY，可配置为KEY0-KEY7，由IOCFG_KEY寄存器确定； */
    {_INT, 0x02},      /* =0010:选择为INT，可配置为INT0-INT7，由IOCFG_INT寄存器确定； */
    {_TC_IN, 0x03},    /* =0011:选择为TCI； */
    {_TC1_P_0, 0x04},  /* =0100:选择为TC1_P[0]； */
#elif defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
    {_KEY, 0x02},      /*  =0010： 选择 KEY0（优先级 P10>P84） */
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
    {_SPI3, 0x02},     /*"=0010： 选择为 SPI3_SCLK（OD 引脚， 需求作为从机输入）"*/
                       /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
#endif
};
/*"***************************** P8_5 *****************************"*/
static const sGPIOFunReg_TypeDef P8_5_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCC[15], PCC[14], PCC[13]"*/
    {GPIO_PCC, _bit13}, /*"Bit0"*/
    {GPIO_PCC, _bit14}, /*"Bit1"*/
    {GPIO_PCC, _bit15}, /*"Bit2"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCC4[17:15],PCC[5]}定义端口 P85 复用配置： */
    {GPIO_PCC, _bit5},   /*"BIT0"*/
    {GPIO_PCC4, _bit15}, /*"BIT1"*/
    {GPIO_PCC4, _bit16}, /*"BIT2"*/
    {GPIO_PCC4, _bit17}, /*"BIT3"*/
#else
    /*"由{PCC[5]}定义端口 P85 复用配置"*/
    {GPIO_PCC, _bit5}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P8_5_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000:选择为IO口； */
    {_KEY, 0x01},      /* =0001:选择为KEY，可配置为KEY0-KEY7，由IOCFG_KEY寄存器确定； */
    {_INT, 0x02},      /* =0010:选择为INT，可配置为INT0-INT7，由IOCFG_INT寄存器确定； */
    {_TC_IN, 0x03},    /* =0011:选择为TCI； */
    {_TC1_N_0, 0x04},  /* =0100:选择为TC1_N[0]； */
};
#elif defined(RN831x_RN861x_MCU_V3)
static const sGPIOFunCll_TypeDef P8_5_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000:选择为IO口； */
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
    {_KEY, 0x02},      /*  =0010： 选择 KEY0（优先级 P10>P84） */
};
#endif
/*"***************************** P8_6 *****************************"*/
/*"由{PCC[6]}定义端口 P86 复用配置"*/
static const sGPIOFunReg_TypeDef P8_6_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCC[18], PCC[17], PCC[16]"*/
    {GPIO_PCC, _bit16}, /*"Bit0"*/
    {GPIO_PCC, _bit17}, /*"Bit1"*/
    {GPIO_PCC, _bit18}, /*"Bit2"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCC4[20:18],PCC[6]}定义端口 P86 复用配置： */
    {GPIO_PCC, _bit6},   /*"BIT0"*/
    {GPIO_PCC4, _bit18}, /*"BIT1"*/
    {GPIO_PCC4, _bit19}, /*"BIT2"*/
    {GPIO_PCC4, _bit20}, /*"BIT3"*/
#else
    {GPIO_PCC, _bit6}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P8_6_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_KEY, 0x01},      /* =0001: 选择为KEY，可配置为KEY0-KEY7，由IOCFG_KEY寄存器确定； */
    {_INT, 0x02},      /* =0010: 选择为INT，可配置为INT0-INT7，由IOCFG_INT寄存器确定； */
    {_TC_IN, 0x03},    /* =0011: 选择为TCI； */
    {_TC1_P_1, 0x04},  /* =0100: 选择为TC1_P[1]； */
};
#elif defined(RN831x_RN861x_MCU_V3)
static const sGPIOFunCll_TypeDef P8_6_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_SEG, 0x01},      /*"=0001： 选择为 LCD"*/
    {_KEY, 0x02},      /* 选择 KEY2（优先级 P00>P27>P86）*/
};
#endif
/*"***************************** P8_7 *****************************"*/
static const sGPIOFunReg_TypeDef P8_7_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCC[21], PCC[20], PCC[19]"*/
    {GPIO_PCC, _bit19}, /*"Bit0"*/
    {GPIO_PCC, _bit20}, /*"Bit1"*/
    {GPIO_PCC, _bit21}, /*"Bit2"*/
#elif defined(RN831x_RN861x_MCU_V3)
    /* 由{PCC4[23:21],PCC[7]}定义端口 P87 复用配置： */
    {GPIO_PCC, _bit7},   /*"BIT0"*/
    {GPIO_PCC4, _bit21}, /*"BIT1"*/
    {GPIO_PCC4, _bit22}, /*"BIT2"*/
    {GPIO_PCC4, _bit23}, /*"BIT3"*/
#else
    /*"由{PCC[7]}定义端口 P87 复用配置"*/
    {GPIO_PCC, _bit7}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P8_7_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_KEY, 0x01},      /* =0001: 选择为KEY, 可配置为KEY0-KEY7, 由IOCFG_KEY寄存器确定； */
    {_INT, 0x02},      /* =0010: 选择为INT，可配置为INT0-INT7，由IOCFG_INT寄存器确定； */
    {_TC_IN, 0x03},    /* =0011: 选择为TCI； */
    {_TC1_N_1, 0x04},  /* =0100: 选择为TC1_N[1]； */
};
#elif defined(RN831x_RN861x_MCU_V3)
static const sGPIOFunCll_TypeDef P8_7_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_SEG, 0x01},      /* =0001： 选择 LCD */
    {_KEY, 0x02},      /* =0010： 选择 KEY3（优先级 P01>P143>P87） */
};
#endif
/*"***************************** P9_0 *****************************"*/
static const sGPIOFunReg_TypeDef P9_0_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCC[23], PCC[22]"*/
    {GPIO_PCC, _bit22}, /*"Bit0"*/
    {GPIO_PCC, _bit23}, /*"Bit1"*/
#elif defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
    /* 由{PCC5[2:0],PCC[8]}定义端口 P90 复用配置： */
    {GPIO_PCC, _bit8},  /*"BIT0"*/
    {GPIO_PCC5, _bit0}, /*"BIT1"*/
    {GPIO_PCC5, _bit1}, /*"BIT2"*/
    {GPIO_PCC5, _bit2}, /*"BIT3"*/
#else
    /*"由{PCC[8]}定义端口 P90 复用配置"*/
    {GPIO_PCC, _bit8}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P9_0_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_KEY, 0x01},      /* =0001: 选择为KEY，可配置为KEY0-KEY7，由IOCFG_KEY寄存器确定； */
    {_INT, 0x02},      /* =0010: 选择为INT，可配置为INT0-INT7，由IOCFG_INT寄存器确定； */
    {_TC_IN, 0x03},    /* =0011: 选择为TCI； */
};
#elif defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
static const sGPIOFunCll_TypeDef P9_0_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_SEG, 0x01},      /* =0001： 选择 LCD */
    {_KEY, 0x02},      /*=0010： 选择 KEY4（优先级 P04>P140>P90）*/
};
#endif
/*"***************************** P9_1 *****************************"*/
static const sGPIOFunReg_TypeDef P9_1_FunReg[] = {
#ifdef RN202x_RN7326_SOC_V2
    /*"PCC[25], PCC[24]"*/
    {GPIO_PCC, _bit24}, /*"Bit0"*/
    {GPIO_PCC, _bit25}, /*"Bit1"*/
#elif defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
    /* 由{PCC5[5:3],PCC[9]}定义端口 P91 复用配置： */
    {GPIO_PCC, _bit9},  /*"BIT0"*/
    {GPIO_PCC5, _bit3}, /*"BIT1"*/
    {GPIO_PCC5, _bit4}, /*"BIT2"*/
    {GPIO_PCC5, _bit5}, /*"BIT3"*/
#else
    /*"由{PCC[9]}定义端口 P91 复用配置"*/
    {GPIO_PCC, _bit9}, /*"BIT0"*/
#endif
};
#ifdef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P9_1_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_KEY, 0x01},      /* =0001: 选择为KEY，可配置为KEY0-KEY7，由IOCFG_KEY寄存器确定； */
    {_INT, 0x02},      /* =0010: 选择为INT，可配置为INT0-INT7，由IOCFG_INT寄存器确定； */
    {_TC_IN, 0x03},    /* =0011: 选择为TCI； */
};
#elif defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
static const sGPIOFunCll_TypeDef P9_1_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_SEG, 0x01},      /* =0001： 选择 LCD */
    {_KEY, 0x02},      /*=0010： 选择 KEY5（优先级 P32>P141>P91）*/
};
#endif
/*"***************************** P9_2 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P9_2_FunReg[] = {
#if defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
    /* 由{PCC5[8:6],PCC[10]}定义端口 P92 复用配置： */
    {GPIO_PCC, _bit10}, /*"BIT0"*/
    {GPIO_PCC5, _bit6}, /*"BIT1"*/
    {GPIO_PCC5, _bit7}, /*"BIT2"*/
    {GPIO_PCC5, _bit8}, /*"BIT3"*/
#else
    /*"由{PCC[10]}定义端口 P92 复用配置"*/
    {GPIO_PCC, _bit10}, /*"BIT0"*/
#endif
};
#if defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
static const sGPIOFunCll_TypeDef P9_2_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000: 选择为IO口； */
    {_SEG, 0x01},        /* =0001： 选择 LCD */
    {_IOCNT_OUT0, 0x02}, /*=0010： 选择 IOCNT_OUT0*/
};
#endif
#endif
/*"***************************** P9_3 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P9_3_FunReg[] = {
#if defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
    /* 由{PCC4[11:9],PCC[11]}定义端口 P93 复用配置：*/
    {GPIO_PCC, _bit11},  /*"BIT0"*/
    {GPIO_PCC5, _bit9},  /*"BIT1"*/
    {GPIO_PCC5, _bit10}, /*"BIT2"*/
    {GPIO_PCC5, _bit11}, /*"BIT3"*/
#else
    /*"由{PCC[11]}定义端口 P93 复用配置"*/
    {GPIO_PCC, _bit11}, /*"BIT0"*/
#endif
};
#if defined(RN831x_RN861x_MCU_V3)
static const sGPIOFunCll_TypeDef P9_3_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000: 选择为IO口； */
    {_SEG, 0x01},        /* =0001： 选择 LCD */
    {_IOCNT_OUT1, 0x02}, /*=0010： 选择 IOCNT_OUT1*/
};
#elif defined(RN821x_RN721x_SOC_V3)
static const sGPIOFunCll_TypeDef P9_3_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000: 选择为IO口； */
    {_SEG, 0x01},        /* =0001： 选择 LCD */
    {_IOCNT_OUT4, 0x02}, /*=0010： 选择 IOCNT_OUT4*/
};
#endif
#endif
/*"***************************** P9_4 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P9_4_FunReg[] = {
#if defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
    /* 由{PCC5[14:12],PCC[12]}定义端口 P94 复用配置：*/
    {GPIO_PCC, _bit12},  /*"BIT0"*/
    {GPIO_PCC5, _bit12}, /*"BIT1"*/
    {GPIO_PCC5, _bit13}, /*"BIT2"*/
    {GPIO_PCC5, _bit14}, /*"BIT3"*/
#else
    /*"由{PCC[12]}定义端口 P94 复用配置"*/
    {GPIO_PCC, _bit12}, /*"BIT0"*/
#endif
};
#if defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
static const sGPIOFunCll_TypeDef P9_4_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000: 选择为IO口； */
    {_SEG, 0x01},        /* =0001： 选择 LCD */
    {_IOCNT_OUT2, 0x02}, /*=0010： 选择 IOCNT_OUT2*/
};
#endif
#endif
/*"***************************** P9_5 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P9_5_FunReg[] = {
#if defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V3)
    /* 由{PCC5[17:15],PCC[13]}定义端口 P95 复用配置：*/
    {GPIO_PCC, _bit13},  /*"BIT0"*/
    {GPIO_PCC5, _bit15}, /*"BIT1"*/
    {GPIO_PCC5, _bit16}, /*"BIT2"*/
    {GPIO_PCC5, _bit17}, /*"BIT3"*/
#else
    /*"由{PCC[13]}定义端口 P95 复用配置"*/
    {GPIO_PCC, _bit13}, /*"BIT0"*/
#endif
};
#ifdef RN831x_RN861x_MCU_V3
static const sGPIOFunCll_TypeDef P9_5_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000: 选择为IO口； */
    {_SEG, 0x01},        /* =0001： 选择 LCD */
    {_IOCNT_OUT2, 0x02}, /*=0010： 选择 IOCNT_OUT2*/
};
#elif defined(RN821x_RN721x_SOC_V3)
static const sGPIOFunCll_TypeDef P9_5_Fun[] = {
    {_NORMALIO, 0x00},   /* =0000: 选择为IO口； */
    {_SEG, 0x01},        /* =0001： 选择 LCD */
    {_IOCNT_OUT3, 0x02}, /*=0010： 选择 IOCNT_OUT3*/
};
#endif
#endif
/*"***************************** P9_6 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P9_6_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /* 由{PCC5[20:18],PCC[14]}定义端口 P96 复用配置*/
    {GPIO_PCC, _bit14},  /*"BIT0"*/
    {GPIO_PCC5, _bit18}, /*"BIT1"*/
    {GPIO_PCC5, _bit19}, /*"BIT2"*/
    {GPIO_PCC5, _bit20}, /*"BIT3"*/
#else
    /*"由{PCC[14]}定义端口 P96 复用配置"*/
    {GPIO_PCC, _bit14}, /*"BIT0"*/
#endif
};
#ifdef RN831x_RN861x_MCU_V3
static const sGPIOFunCll_TypeDef P9_6_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_SEG, 0x01},      /* =0001： 选择 LCD */
};
#endif
#endif
/*"***************************** P9_7 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P9_7_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /* 由{PCC5[23:21],PCC[15]}定义端口 P97 复用配置：*/
    {GPIO_PCC, _bit15},  /*"BIT0"*/
    {GPIO_PCC5, _bit21}, /*"BIT1"*/
    {GPIO_PCC5, _bit22}, /*"BIT2"*/
    {GPIO_PCC5, _bit23}, /*"BIT3"*/
#else
    /*"由{PCC[15]}定义端口 P97 复用配置"*/
    {GPIO_PCC, _bit15}, /*"BIT0"*/
#endif
};
#ifdef RN831x_RN861x_MCU_V3
static const sGPIOFunCll_TypeDef P9_7_Fun[] = {
    {_NORMALIO, 0x00}, /* =0000: 选择为IO口； */
    {_SEG, 0x01},      /* =0001： 选择 LCD */
};
#endif
#endif
/*"***************************** P10_0 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P10_0_FunReg[] = {
    /*"由{PCC[16]}定义端口 P100 复用配置"*/
    {GPIO_PCC, _bit16}, /*"BIT0"*/
};
#endif
/*"***************************** P10_1 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P10_1_FunReg[] = {
    /*"由{PCC[17]}定义端口 P101 复用配置"*/
    {GPIO_PCC, _bit17}, /*"BIT0"*/
};
#endif
/*"***************************** P10_2 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P10_2_FunReg[] = {
    /*"由{PCC[18]}定义端口 P102 复用配置"*/
    {GPIO_PCC, _bit18}, /*"BIT0"*/
};
#endif
/*"***************************** P10_3 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P10_3_FunReg[] = {
    /*"由{PCC[19]}定义端口 P103 复用配置"*/
    {GPIO_PCC, _bit19}, /*"BIT0"*/
};
#endif
/*"***************************** P10_4 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P10_4_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /* 由{PCC2[14:12],PCC[20]}定义端口 P104 复用配置 */
    {GPIO_PCC, _bit20},  /*"BIT0"*/
    {GPIO_PCC2, _bit12}, /*"BIT1"*/
    {GPIO_PCC2, _bit13}, /*"BIT2"*/
    {GPIO_PCC2, _bit14}, /*"BIT3"*/
#elif defined(RN821x_RN721x_SOC_V2)
    /*"由{PCC2[14:12],PCC[20]}定义端口 P104 复用配置"*/
    {GPIO_PCC, _bit20},  /*"BIT0"*/
    {GPIO_PCC2, _bit12}, /*"BIT1"*/
    {GPIO_PCC2, _bit13}, /*"BIT2"*/
    {GPIO_PCC2, _bit14}, /*"BIT3"*/
#else
    {GPIO_PCC, _bit20}, /*"BIT0"*/
#endif
};
static const sGPIOFunCll_TypeDef P10_4_Fun[] = {
#ifdef RN831x_RN861x_MCU_V3
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI3, 0x01},     /* =0001： 保留（SPI3_SCSN) */
    {_SPI0, 0x02},     /* =0010： 选择为 SPI0_SCSN */
    {_SPI1, 0x03},     /* =0011： 选择为 SPI1_SCSN */
    {_UART4, 0x04},    /* =0100： 选择为 TX4 */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI3, 0x01}, /*"=0001： P104 复用为 SPI3_SCSN"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_U_IN, 0x01}, /*"=0001： U_IN"*/
                   /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
#endif
};
/*"	注 1： 作为 ADC 模拟输入一直有效， 不需要配置复用； 作为IO 口时， 仅支持输入功能。
    注 2： 使能外灌功能时， UP 配置为外灌通道， UN 悬空不可接地且配置 ie_n = 1， 即配置为 IO 输入但输入不使能， 防止外灌漏电。"*/
#endif
/*"***************************** P10_5 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P10_5_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /* 由{PCC2[17:15],PCC[21]}定义端口 P105 复用配置 */
    {GPIO_PCC, _bit21},  /*"BIT0"*/
    {GPIO_PCC2, _bit15}, /*"BIT1"*/
    {GPIO_PCC2, _bit16}, /*"BIT2"*/
    {GPIO_PCC2, _bit17}, /*"BIT3"*/
#elif defined(RN821x_RN721x_SOC_V2)
    /*"由{PCC2[17:15],PCC[21]}定义端口 P105 复用配置"*/
    {GPIO_PCC, _bit21},  /*"BIT0"*/
    {GPIO_PCC2, _bit15}, /*"BIT1"*/
    {GPIO_PCC2, _bit16}, /*"BIT2"*/
    {GPIO_PCC2, _bit17}, /*"BIT3"*/
#else
    {GPIO_PCC, _bit21}, /*"BIT0"*/
#endif
};
static const sGPIOFunCll_TypeDef P10_5_Fun[] = {
#ifdef RN831x_RN861x_MCU_V3
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI3, 0x01},     /* =0001： 保留（SPI3_SCLK） */
    {_SPI0, 0x02},     /* =0010： 选择为 SPI0_SCLK */
    {_SPI1, 0x03},     /* =0011： 选择为 SPI1_SCLK */
    {_UART4, 0x04},    /* =0100： 选择为 RX4 */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI3, 0x01}, /*"=0001： P105 复用为 SPI3_SCLK"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_U_IN, 0x01}, /*"=0001： U_IN"*/
                   /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
#endif
};
/*"注： 作为 ADC 模拟输入一直有效， 不需要配置复用； 作为 IO口时， 仅支持输入功能。"*/
#endif
/*"***************************** P10_6 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P10_6_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /* 由{PCC2[20:18],PCC[22]}定义端口 P106 复用配置 */
    {GPIO_PCC, _bit22},  /*"BIT0"*/
    {GPIO_PCC2, _bit18}, /*"BIT1"*/
    {GPIO_PCC2, _bit19}, /*"BIT2"*/
    {GPIO_PCC2, _bit20}, /*"BIT3"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    /*"由{PCC2[20:18],PCC[22]}定义端口 P106 复用配置"*/
    {GPIO_PCC, _bit22},  /*"BIT0"*/
    {GPIO_PCC2, _bit18}, /*"BIT1"*/
    {GPIO_PCC2, _bit19}, /*"BIT2"*/
    {GPIO_PCC2, _bit20}, /*"BIT3"*/
#else
    {GPIO_PCC, _bit22}, /*"BIT0"*/
#endif
};
static const sGPIOFunCll_TypeDef P10_6_Fun[] = {
#ifdef RN831x_RN861x_MCU_V3
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI3, 0x01},     /* =0001： 保留（SPI3_MISO） */
    {_SPI0, 0x02},     /* =0010： 选择为 SPI0_MISO */
    {_SPI1, 0x03},     /* =0011： 选择为 SPI1_MISO */
    {_UART5, 0x04},    /* =0100： 选择为 RX5 */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI3, 0x01},     /*"=0001： P106 复用为 SPI3_MISO"*/
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_IB_IN, 0x01},    /*"=0001： IB_IN"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
#endif
};
/*"	注 1： 作为 ADC 模拟输入一直有效， 不需要配置复用； 作为IO 口时， 仅支持输入功能。
    注 2： 使能外灌功能时， IBP 配置为外灌通道， IBN 悬空不可接地且配置 ie_n = 1， 即配置为 IO 输入但输入不使能， 防止外灌漏电。"*/
#endif
/*"***************************** P10_7 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P10_7_FunReg[] = {
#ifdef RN831x_RN861x_MCU_V3
    /* 由{PCC2[23:21],PCC[23]}定义端口 P107 复用配置 */
    {GPIO_PCC, _bit23},  /*"BIT0"*/
    {GPIO_PCC2, _bit21}, /*"BIT1"*/
    {GPIO_PCC2, _bit22}, /*"BIT2"*/
    {GPIO_PCC2, _bit23}, /*"BIT3"*/
#elif defined(RN821x_RN721x_SOC_V2)
    /*"由{PCC2[23:21],PCC[23]}定义端口 P107 复用配置"*/
    {GPIO_PCC, _bit23},  /*"BIT0"*/
    {GPIO_PCC2, _bit21}, /*"BIT1"*/
    {GPIO_PCC2, _bit22}, /*"BIT2"*/
    {GPIO_PCC2, _bit23}, /*"BIT3"*/
#else
    {GPIO_PCC, _bit23}, /*"BIT0"*/
#endif
};
static const sGPIOFunCll_TypeDef P10_7_Fun[] = {
#ifdef RN831x_RN861x_MCU_V3
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI3, 0x01},     /* =0001： 保留（SPI3_MOSI） */
    {_SPI0, 0x02},     /* =0010： 选择为 SPI0_MOSI */
    {_SPI1, 0x03},     /* =0011： 选择为 SPI1_MOSI */
    {_UART5, 0x04},    /* =0100： 选择为 TX5 */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI3, 0x01}, /*"=0001： P107 复用为 SPI3_MOSI"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_IB_IN, 0x01}, /*"=0001： IB_IN"*/
                    /*"=其他： 保留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
#endif
};
/*"注： 作为 ADC 模拟输入一直有效， 不需要配置复用； 作为 IO口时， 仅支持输入功能。"*/
#endif
/*"***************************** P11_0 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
/*"由{PCC3[2:0],PCC[24]}定义 P110 的配置"*/
static const sGPIOFunReg_TypeDef P11_0_FunReg[] = {
    {GPIO_PCC, _bit24}, /*"BIT0"*/
#if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {GPIO_PCC3, _bit0}, /*"BIT1"*/
    {GPIO_PCC3, _bit1}, /*"BIT2"*/
    {GPIO_PCC3, _bit2}, /*"BIT3"*/
#endif
};
static const sGPIOFunCll_TypeDef P11_0_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_SCSN"*/
#ifdef RN821x_RN721x_SOC_V2
    {_SPI2, 0x02}, /*"=0010： 选择为 SPI2_SCSN"*/
    {_SPI3, 0x03}, /*"=0011： 选择为 SPI3_SCSN"*/
                   /*"=其他： 保留"*/
#endif
};
/*"	注 1： PC110 仅在 SPI 从模式下有效， 在 SPI 主模式下不要配置为 1.
    注 2： P40~P43 口和 P110~P113 口不能同时复用为 SPI2
    注 3： P14~P17， P20~P23 口和 P110~P113 口不能同时复用为SPI3"*/
#endif
/*"***************************** P11_1 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
/*"{PCC3[5:3],PCC[25]}定义 P111 的配置"*/
static const sGPIOFunReg_TypeDef P11_1_FunReg[] = {
    {GPIO_PCC, _bit25}, /*"BIT0"*/
#if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {GPIO_PCC3, _bit3}, /*"BIT1"*/
    {GPIO_PCC3, _bit4}, /*"BIT2"*/
    {GPIO_PCC3, _bit5}, /*"BIT3"*/
#endif
};
static const sGPIOFunCll_TypeDef P11_1_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_SCLK"*/
#ifdef RN821x_RN721x_SOC_V2
    {_SPI2, 0x02}, /*"=0010： 选择为 SPI2_SCLK"*/
    {_SPI3, 0x03}, /*"=0011： 选择为 SPI3_SCLK"*/
                   /*"=其他： 保留"*/
#endif
};
#endif
/*"***************************** P11_2 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
/*"{PCC3[8:6],PCC[26]}定义 P112 的配置"*/
static const sGPIOFunReg_TypeDef P11_2_FunReg[] = {
    {GPIO_PCC, _bit26}, /*"BIT0"*/
#if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {GPIO_PCC3, _bit6}, /*"BIT1"*/
    {GPIO_PCC3, _bit7}, /*"BIT2"*/
    {GPIO_PCC3, _bit8}, /*"BIT3"*/
#endif
};
static const sGPIOFunCll_TypeDef P11_2_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_MISO"*/
#ifdef RN821x_RN721x_SOC_V2
    {_SPI2, 0x02}, /*"=0010： 选择为 SPI2_MISO"*/
    {_SPI3, 0x03}, /*"=0011： 选择为 SPI3_MISO"*/
                   /*"=其他： 保留"*/
#endif
};
#endif
/*"***************************** P11_3 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
/*"{PCC3[11:9],PCC[27]}定义 P113 的配置"*/
static const sGPIOFunReg_TypeDef P11_3_FunReg[] = {
    {GPIO_PCC, _bit27}, /*"BIT0"*/
#if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    {GPIO_PCC3, _bit9},  /*"BIT1"*/
    {GPIO_PCC3, _bit10}, /*"BIT2"*/
    {GPIO_PCC3, _bit11}, /*"BIT3"*/
#endif
};
static const sGPIOFunCll_TypeDef P11_3_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_SPI0, 0x01},     /*"=0001： 选择为 SPI0_MOSI"*/
#ifdef RN821x_RN721x_SOC_V2
    {_SPI2, 0x02}, /*"=0010： 选择为 SPI2_MOSI"*/
    {_SPI3, 0x03}, /*"=0011： 选择为 SPI3_MOSI"*/
                   /*"=其他： 保留"*/
#endif
};
#endif
/*"***************************** P11_4 *****************************"*/
#ifdef RN821x_RN721x_SOC_V2
/*"PC114 定义端口 P114 复用配置"*/
static const sGPIOFunReg_TypeDef P11_4_FunReg[] = {
    {GPIO_PCC3, _bit12}, /*"BIT0"*/
    {GPIO_PCC3, _bit13}, /*"BIT1"*/
    {GPIO_PCC3, _bit14}, /*"BIT2"*/
    {GPIO_PCC3, _bit15}, /*"BIT3"*/
};
static const sGPIOFunCll_TypeDef P11_4_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_IA_IN, 0x01},    /*"=0001： IA_IN"*/
                       /*"=其他： 保留"*/
};

/*"	注 1： 作为 ADC 模拟输入一直有效， 不需要配置复用； 作为IO 口时， 仅支持输入功能。
    注 2： 使能外灌功能时， IAP 配置为外灌通道， IAN 悬空不可接地且配置 ie_n = 1， 即配置为 IO 输入但输入不使能， 防止外灌漏电"*/
#endif
/*"***************************** P11_5 *****************************"*/
#ifdef RN821x_RN721x_SOC_V2
/*"PC115 定义端口 P115 复用配置"*/
static const sGPIOFunReg_TypeDef P11_5_FunReg[] = {
    {GPIO_PCC3, _bit16}, /*"BIT0"*/
    {GPIO_PCC3, _bit17}, /*"BIT1"*/
    {GPIO_PCC3, _bit18}, /*"BIT2"*/
    {GPIO_PCC3, _bit19}, /*"BIT3"*/
};
static const sGPIOFunCll_TypeDef P11_5_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_IA_IN, 0x01},    /*"=0001： IA_IN"*/
                       /*"=其他： 保留"*/
};
/*"注： 作为 ADC 模拟输入一直有效， 不需要配置复用； 作为 IO口时， 仅支持输入功能。"*/
#endif
/*"***************************** P11_6 *****************************"*/
#ifdef RN821x_RN721x_SOC_V2
/*"PC116 定义端口 P116 复用配置"*/
static const sGPIOFunReg_TypeDef P11_6_FunReg[] = {
    {GPIO_PCC3, _bit20}, /*"BIT0"*/
    {GPIO_PCC3, _bit21}, /*"BIT1"*/
    {GPIO_PCC3, _bit22}, /*"BIT2"*/
    {GPIO_PCC3, _bit23}, /*"BIT3"*/
};
static const sGPIOFunCll_TypeDef P11_6_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_AIN, 0x01},      /*"=0001： 选择为模拟输入口 AIN6"*/
                       /*"=其他： 保留"*/
};
#endif
/*"***************************** P11_7 *****************************"*/
#ifdef RN821x_RN721x_SOC_V2
/*"PC117 定义端口 P117 复用配置"*/
static const sGPIOFunReg_TypeDef P11_7_FunReg[] = {
    {GPIO_PCC3, _bit24}, /*"BIT0"*/
    {GPIO_PCC3, _bit25}, /*"BIT1"*/
    {GPIO_PCC3, _bit26}, /*"BIT2"*/
    {GPIO_PCC3, _bit27}, /*"BIT3"*/
};
static const sGPIOFunCll_TypeDef P11_7_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_RTC_1S, 0x01},   /*"=0001： 选择为万年历秒输出 RTC1S"*/
                       /*"=其他： 保留"*/
};
#endif
/*"PC127~PC120 定义端口复用配置：P127~P120 对应 COM7~COM0"*/
/*"***************************** P12_0 *****************************"*/
#if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
static const sGPIOFunReg_TypeDef P12_0_FunReg[] = {
    {GPIO_PCD, _bit0}, /*"BIT0"*/
};
#endif
/*"***************************** P12_1 *****************************"*/
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
static const sGPIOFunReg_TypeDef P12_1_FunReg[] = {
    {GPIO_PCD, _bit1}, /*"BIT0"*/
};
#endif
/*"***************************** P12_2 *****************************"*/
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
static const sGPIOFunReg_TypeDef P12_2_FunReg[] = {
    {GPIO_PCD, _bit2}, /*"BIT0"*/
};
#endif
/*"***************************** P12_3 *****************************"*/
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
static const sGPIOFunReg_TypeDef P12_3_FunReg[] = {
    {GPIO_PCD, _bit3}, /*"BIT0"*/
};
#endif
/*"***************************** P12_4 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P12_4_FunReg[] = {
    {GPIO_PCE, _bit0}, /*"BIT0"*/
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
    {GPIO_PCD, _bit4}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P12_4_Fun[] = {
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0： 选择为 IO 口；"*/
    {_SEG, 0x02},      /*"=1： 选择 LCD  SEG0"*/
    {_COM, 0x03},      /*"=1： 选择 LCD  COM4"*/
#else
    {_SEG, 0x00}, /*"=1： 选择 LCD  SEG0"*/
    {_COM, 0x01}, /*"=1： 选择 LCD  COM4"*/
#endif
};
#endif
/*"***************************** P12_5 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P12_5_FunReg[] = {
    {GPIO_PCE, _bit1}, /*"BIT0"*/
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
    {GPIO_PCD, _bit5}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P12_5_Fun[] = {
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0： 选择为 IO 口；"*/
    {_SEG, 0x02},      /*"=1： 选择 LCD  SEG1"*/
    {_COM, 0x03},      /*"=1： 选择 LCD  COM5"*/
#else
    {_SEG, 0x00}, /*"=1： 选择 LCD  SEG1"*/
    {_COM, 0x01}, /*"=1： 选择 LCD  COM5"*/
#endif
};
#endif
/*"***************************** P12_6 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P12_6_FunReg[] = {
    {GPIO_PCE, _bit2}, /*"BIT0"*/
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
    {GPIO_PCD, _bit6}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P12_6_Fun[] = {
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0： 选择为 IO 口；"*/
    {_SEG, 0x02},      /*"=1： 选择 LCD  SEG2"*/
    {_COM, 0x03},      /*"=1： 选择 LCD  COM6"*/
#else
    {_SEG, 0x00}, /*"=1： 选择 LCD  SEG2"*/
    {_COM, 0x01}, /*"=1： 选择 LCD  COM6"*/
#endif
};
#endif
/*"***************************** P12_7 *****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunReg_TypeDef P12_7_FunReg[] = {
    {GPIO_PCE, _bit3}, /*"BIT0"*/
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
    {GPIO_PCD, _bit7}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P12_7_Fun[] = {
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0： 选择为 IO 口；"*/
    {_SEG, 0x02},      /*"=1： 选择 LCD  SEG3"*/
    {_COM, 0x03},      /*"=1： 选择 LCD  COM7"*/
#else
    {_SEG, 0x00}, /*"=1： 选择 LCD  SEG3"*/
    {_COM, 0x01}, /*"=1： 选择 LCD  COM7"*/
#endif
};
#endif
/*"***************************** P13_0 *****************************"*/
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
/*"P130 对应 LCDVA"*/
static const sGPIOFunReg_TypeDef P13_0_FunReg[] = {
    {GPIO_PCD, _bit8}, /*"BIT0"*/
};
static const sGPIOFunCll_TypeDef P13_0_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0： 选择为 IO 口；"*/
    {_LCDV, 0x01},     /*"=1： 选择 LCD 相关引脚或保留"*/
};
#endif
/*"***************************** P13_1 *****************************"*/
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
/*"P131 对应 LCDVB"*/
static const sGPIOFunReg_TypeDef P13_1_FunReg[] = {
    {GPIO_PCD, _bit9}, /*"BIT0"*/
};
static const sGPIOFunCll_TypeDef P13_1_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0： 选择为 IO 口；"*/
    {_LCDV, 0x01},     /*"=1： 选择 LCD 相关引脚或保留"*/
};
#endif
/*"***************************** P13_2 *****************************"*/
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
/*"P132 对应 LCDVC"*/
static const sGPIOFunReg_TypeDef P13_2_FunReg[] = {
    {GPIO_PCD, _bit10}, /*"BIT0"*/
};
static const sGPIOFunCll_TypeDef P13_2_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_LCDV, 0x01},     /*"=1： 选择 LCD 相关引脚或保留"*/
};
#endif
/*"***************************** P13_3 *****************************"*/
#if 0
/*"P133 对应 LCDVD， 预留， 暂不支持复用"*/
static const sGPIOFunReg_TypeDef P13_3_FunReg[]={
	{GPIO_PCD,  _bit11},/*"BIT0"*/
};
static const sGPIOFunCll_TypeDef P13_3_Fun[]={
	{_NORMALIO,		0x00},			/*"=0000： 选择为 IO 口"*/
	{_LCDV,			0x01},			/*"=1： 选择 LCD 相关引脚或保留"*/
};
#endif
/*"***************************** P13_4 *****************************"*/
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
/*"P134 对应 LCDVP1"*/
static const sGPIOFunReg_TypeDef P13_4_FunReg[] = {
    {GPIO_PCD, _bit12}, /*"BIT0"*/
};
static const sGPIOFunCll_TypeDef P13_4_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_LCDV, 0x01},     /*"=1： 选择 LCD 相关引脚或保留"*/
};
#endif
/*"***************************** P13_5 *****************************"*/
#if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
/*"P135 对应 LCDVP2"*/
static const sGPIOFunReg_TypeDef P13_5_FunReg[] = {
    {GPIO_PCD, _bit13}, /*"BIT0"*/
};
static const sGPIOFunCll_TypeDef P13_5_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_LCDV, 0x01},     /*"=1： 选择 LCD 相关引脚或保留"*/
};
#endif
/*"***************************** P14_0 *****************************"*/
#if defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V2)
static const sGPIOFunReg_TypeDef P14_0_FunReg[] = {
#if defined(RN831x_RN861x_MCU_V3)
    {GPIO_PCD, _bit16}, /*"BIT0"*/
    {GPIO_PCD, _bit17}, /*"BIT1"*/
#elif defined(RN831x_RN861x_MCU_V2)
    {GPIO_PCD, _bit16}, /*"BIT0"*/
    {GPIO_PCD, _bit17}, /*"BIT1"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {GPIO_PCD2, _bit0}, /*"BIT0"*/
    {GPIO_PCD2, _bit1}, /*"BIT1"*/
    {GPIO_PCD2, _bit2}, /*"BIT2"*/
    {GPIO_PCD2, _bit3}, /*"BIT3"*/
#endif
};
static const sGPIOFunCll_TypeDef P14_0_Fun[] = {
#if defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /* =01： 选择为 KEY4（优先级： P04>P140>P90） */
    {_TC_IN, 0x02},    /* =10： 选择为 TCIN */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ADC_CLKO, 0x01}, /*"=0001： 选择为 ADC_CLKO"*/
    {_TRIG_OUT, 0x02}, /*"=0010： 选择为 TRIG_OUT(智能微断需求)"*/
    {_RTC_1S, 0x03},   /*"=0011： 选择为万年历秒输出 RTC1S"*/
                       /*"=其他： 预留"*/
#endif
};
#endif
/*"***************************** P14_1 *****************************"*/
#if defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V2)
static const sGPIOFunReg_TypeDef P14_1_FunReg[] = {
#if defined(RN831x_RN861x_MCU_V3)
    {GPIO_PCD, _bit18}, /*"BIT0"*/
    {GPIO_PCD, _bit19}, /*"BIT1"*/
#elif defined(RN831x_RN861x_MCU_V2)
    {GPIO_PCD, _bit18}, /*"BIT0"*/
    {GPIO_PCD, _bit19}, /*"BIT1"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {GPIO_PCD2, _bit4}, /*"BIT0"*/
    {GPIO_PCD2, _bit5}, /*"BIT1"*/
    {GPIO_PCD2, _bit6}, /*"BIT2"*/
    {GPIO_PCD2, _bit7}, /*"BIT3"*/
#endif
};
static const sGPIOFunCll_TypeDef P14_1_Fun[] = {
#if defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /* =01： 选择为 KEY5（优先级： P32>P141>P91） */
    {_TC_IN, 0x02},    /* =10： 选择为 TCIN */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ADC_CLKO, 0x01}, /*"=0001： 选择为 ADC_CLKO"*/
    {_TRIG_OUT, 0x02}, /*"=0010： 选择为 TRIG_OUT(智能微断需求)"*/
    {_RTC_1S, 0x03},   /*"=0011： 选择为万年历秒输出 RTC1S"*/
                       /*"=其他： 预留"*/
#endif
};
#endif
/*"***************************** P14_2 *****************************"*/
#if defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V2)
static const sGPIOFunReg_TypeDef P14_2_FunReg[] = {
#if defined(RN831x_RN861x_MCU_V3)
    {GPIO_PCD, _bit20}, /*"BIT0"*/
    {GPIO_PCD, _bit21}, /*"BIT1"*/
#elif defined(RN831x_RN861x_MCU_V2)
    {GPIO_PCD, _bit20}, /*"BIT0"*/
    {GPIO_PCD, _bit21}, /*"BIT1"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {GPIO_PCD2, _bit8},  /*"BIT0"*/
    {GPIO_PCD2, _bit9},  /*"BIT1"*/
    {GPIO_PCD2, _bit10}, /*"BIT2"*/
    {GPIO_PCD2, _bit11}, /*"BIT3"*/
#endif
};
static const sGPIOFunCll_TypeDef P14_2_Fun[] = {
#if defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},        /* =01： 选择为 INT3（优先级： P41>P142>P77） */
    {_IOCNT_OUT0, 0x02}, /* =10： 选择为 IOCNT_OUT0 */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ADC_CLKO, 0x01}, /*"=0001： 选择为 ADC_CLKO"*/
    {_TRIG_OUT, 0x02}, /*"=0010： 选择为 TRIG_OUT(智能微断需求)"*/
    {_RTC_1S, 0x03},   /*"=0011： 选择为万年历秒输出 RTC1S"*/
                       /*"=其他： 预留"*/
#endif
};
#endif
/*"***************************** P14_3 *****************************"*/
#if defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V2)
static const sGPIOFunReg_TypeDef P14_3_FunReg[] = {
#if defined(RN831x_RN861x_MCU_V3)
    {GPIO_PCD, _bit22}, /*"BIT0"*/
    {GPIO_PCD, _bit23}, /*"BIT1"*/
#elif defined(RN831x_RN861x_MCU_V2)
    {GPIO_PCD, _bit22}, /*"BIT0"*/
    {GPIO_PCD, _bit23}, /*"BIT1"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {GPIO_PCD2, _bit12}, /*"BIT0"*/
    {GPIO_PCD2, _bit13}, /*"BIT1"*/
    {GPIO_PCD2, _bit14}, /*"BIT2"*/
    {GPIO_PCD2, _bit15}, /*"BIT3"*/
#endif
};
static const sGPIOFunCll_TypeDef P14_3_Fun[] = {
#if defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_KEY, 0x01},      /* =01： 选择为 KEY3（优先级： P01>P143>P87） */
    {_TC_IN, 0x02},    /* =10： 选择为 TCIN */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ADC_CLKO, 0x01}, /*"=0001： 选择为 ADC_CLKO"*/
    {_TRIG_OUT, 0x02}, /*"=0010： 选择为 TRIG_OUT(智能微断需求)"*/
    {_SPI2, 0x03},     /*"=0011： 选择为 SPI2_SCSN"*/
                       /*"=其他： 预留"*/
                       /*"注 1： PC143 仅在 SPI 从模式下有效， 在 SPI 主模式下不要配置为 1"*/
#endif
};
#endif
/*"***************************** P14_4 *****************************"*/
#if defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V2)
static const sGPIOFunReg_TypeDef P14_4_FunReg[] = {
#if defined(RN831x_RN861x_MCU_V3)
    {GPIO_PCD, _bit24}, /*"BIT0"*/
    {GPIO_PCD, _bit25}, /*"BIT1"*/
#elif defined(RN831x_RN861x_MCU_V2)
    {GPIO_PCD, _bit24}, /*"BIT0"*/
    {GPIO_PCD, _bit25}, /*"BIT1"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {GPIO_PCD2, _bit16}, /*"BIT0"*/
    {GPIO_PCD2, _bit17}, /*"BIT1"*/
    {GPIO_PCD2, _bit18}, /*"BIT2"*/
    {GPIO_PCD2, _bit19}, /*"BIT3"*/
#endif
};
static const sGPIOFunCll_TypeDef P14_4_Fun[] = {
#if defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ADC_CLKO, 0x01}, /*"=0001： 选择为 ADC_CLKO"*/
    {_TRIG_OUT, 0x02}, /*"=0010： 选择为 TRIG_OUT(智能微断需求)"*/
    {_SPI2, 0x03},     /*"=0011： 选择为 SPI2_SCLK"*/
                       /*"=其他： 预留"*/
#endif
};
#endif
/*"***************************** P14_5 *****************************"*/
#if defined(RN831x_RN861x_MCU_V3) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V2)
static const sGPIOFunReg_TypeDef P14_5_FunReg[] = {
#if defined(RN831x_RN861x_MCU_V3)
    {GPIO_PCD, _bit26}, /*"BIT0"*/
    {GPIO_PCD, _bit27}, /*"BIT1"*/
#elif defined(RN831x_RN861x_MCU_V2)
    {GPIO_PCD, _bit26}, /*"BIT0"*/
    {GPIO_PCD, _bit27}, /*"BIT1"*/
#elif defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3)
    {GPIO_PCD2, _bit20}, /*"BIT0"*/
    {GPIO_PCD2, _bit21}, /*"BIT1"*/
    {GPIO_PCD2, _bit22}, /*"BIT2"*/
    {GPIO_PCD2, _bit23}, /*"BIT3"*/
#endif
};
static const sGPIOFunCll_TypeDef P14_5_Fun[] = {
#if defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},        /*  =01： 选择为 INT4（优先级： P42>P145>P80） */
    {_IOCNT_OUT1, 0x02}, /*  =10： 选择为 IOCNT_OUT1 */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ADC_CLKO, 0x01}, /*"=0001： 选择为 ADC_CLKO"*/
    {_TRIG_OUT, 0x02}, /*"=0010： 选择为 TRIG_OUT(智能微断需求)"*/
    {_SPI2, 0x03},     /*"=0011： 选择为 SPI2_MISO"*/
                       /*"=其他： 预留"*/
#elif defined(RN821x_RN721x_SOC_V3)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ADC_CLKO, 0x01}, /*"=0001： 选择为 ADC_CLKO"*/
    {_TRIG_OUT, 0x02}, /*"=0010： 选择为 TRIG_OUT(智能微断需求)"*/
    {_SEG, 0x04}, /*"=0100： SEG36"*/
#endif
};
#endif
/*"***************************** P14_6 *****************************"*/
#if defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V2)
static const sGPIOFunReg_TypeDef P14_6_FunReg[] = {
#if defined(RN831x_RN861x_MCU_V3)
    {GPIO_PCD, _bit28}, /*"BIT0"*/
    {GPIO_PCD, _bit29}, /*"BIT1"*/
#elif defined(RN831x_RN861x_MCU_V2)
    {GPIO_PCD, _bit28}, /*"BIT0"*/
    {GPIO_PCD, _bit29}, /*"BIT1"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {GPIO_PCD2, _bit24}, /*"BIT0"*/
    {GPIO_PCD2, _bit25}, /*"BIT1"*/
    {GPIO_PCD2, _bit26}, /*"BIT2"*/
    {GPIO_PCD2, _bit27}, /*"BIT3"*/
#endif
};
static const sGPIOFunCll_TypeDef P14_6_Fun[] = {
#if defined(RN831x_RN861x_MCU_V3)
    {_NORMALIO, 0x00},   /*"=0000： 选择为 IO 口"*/
    {_INT, 0x01},        /* =01： 选择为 INT5（优先级： P43>P146>P81） */
    {_IOCNT_OUT2, 0x02}, /* =10： 选择为 IOCNT_OUT2 */
#elif defined(RN831x_RN861x_MCU_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
#elif defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_ADC_CLKO, 0x01}, /*"=0001： 选择为 ADC_CLKO"*/
    {_TRIG_OUT, 0x02}, /*"=0010： 选择为 TRIG_OUT(智能微断需求)"*/
    {_SPI2, 0x03},     /*"=0011： 选择为 SPI2_MOSI"*/
                       /*"=其他： 预留"*/
#endif
};
#endif
/*"***************************** P14_7 *****************************"*/
#if defined(RN831x_RN861x_MCU_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN821x_RN721x_SOC_V2)
static const sGPIOFunReg_TypeDef P14_7_FunReg[] = {
#if defined(RN821x_RN721x_SOC_V3) || defined(RN821x_RN721x_SOC_V2)
    {GPIO_PCD2, _bit28}, /*"BIT0"*/
    {GPIO_PCD2, _bit29}, /*"BIT1"*/
    {GPIO_PCD2, _bit30}, /*"BIT2"*/
    {GPIO_PCD2, _bit31}, /*"BIT3"*/
#elif defined(RN831x_RN861x_MCU_V2)
    {GPIO_PCD, _bit30}, /*"BIT0"*/
    {GPIO_PCD, _bit31}, /*"BIT1"*/
#endif
};
static const sGPIOFunCll_TypeDef P14_7_Fun[] = {
#if defined(RN821x_RN721x_SOC_V3) || defined(RN821x_RN721x_SOC_V2)
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
    {_RTC_1S, 0x01}, /*"=0001： 选择为万年历秒输出 RTC1S"*/
                     /*"=其他： 预留"*/
#else
    {_NORMALIO, 0x00}, /*"=0000： 选择为 IO 口"*/
#endif
};
#endif
/*"***************************** LCD功能模式*****************************"*/
#ifndef RN202x_RN7326_SOC_V2
static const sGPIOFunCll_TypeDef P_LCD_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0： 选择为 IO 口"*/
    {_SEG, 0x01},      /*"=1： 选择为 LCD"*/
};
static const sGPIOFunCll_TypeDef P_COM_Fun[] = {
    {_NORMALIO, 0x00}, /*"=0： 选择为 IO 口"*/
    {_COM, 0x01},      /*"=1： 选择为 COM"*/
};
#endif
/*"***************************** gpio映射表 *****************************"*/
/*"***************************** 注意与IO口一一映射，不能空缺 *****************************"*/
static const sGPIOFunTable_TypeDef GPIOFunTable[] = {
    /*"****************************************************Group0*********************************************************************"*/
    #ifdef RN202x_RN7326_SOC_V2
    /*"P0_0"*/ {P0_0_Fun,P0_0_FunReg,GPIO_ARRAY_LENGTH(P0_0_Fun),GPIO_ARRAY_LENGTH(P0_0_FunReg),},
    /*"P0_1"*/ {P0_1_Fun,P0_1_FunReg,GPIO_ARRAY_LENGTH(P0_1_Fun),GPIO_ARRAY_LENGTH(P0_1_FunReg),},
    /*"P0_2"*/ {P0_2_Fun,P0_2_FunReg,GPIO_ARRAY_LENGTH(P0_2_Fun),GPIO_ARRAY_LENGTH(P0_2_FunReg),},
    /*"P0_3"*/ {P0_3_Fun,P0_3_FunReg,GPIO_ARRAY_LENGTH(P0_3_Fun),GPIO_ARRAY_LENGTH(P0_3_FunReg),},
    /*"P0_4"*/ {P0_4_Fun,P0_4_FunReg,GPIO_ARRAY_LENGTH(P0_4_Fun),GPIO_ARRAY_LENGTH(P0_4_FunReg),},
    /*"P0_5"*/ {P0_5_Fun,P0_5_FunReg,GPIO_ARRAY_LENGTH(P0_5_Fun),GPIO_ARRAY_LENGTH(P0_5_FunReg),},
    /*"P0_6"*/ {P0_6_Fun,P0_6_FunReg,GPIO_ARRAY_LENGTH(P0_6_Fun),GPIO_ARRAY_LENGTH(P0_6_FunReg),},
    /*"P0_7"*/ {P0_7_Fun,P0_7_FunReg,GPIO_ARRAY_LENGTH(P0_7_Fun),GPIO_ARRAY_LENGTH(P0_7_FunReg),},
    #else
    /*"P0_0"*/ {P0_0_Fun,P0_0_FunReg,GPIO_ARRAY_LENGTH(P0_0_Fun),GPIO_ARRAY_LENGTH(P0_0_FunReg),},
    /*"P0_1"*/ {P0_1_Fun,P0_1_FunReg,GPIO_ARRAY_LENGTH(P0_1_Fun),GPIO_ARRAY_LENGTH(P0_1_FunReg),},
    /*"P0_2"*/ {P0_2_Fun,P0_2_FunReg,GPIO_ARRAY_LENGTH(P0_2_Fun),GPIO_ARRAY_LENGTH(P0_2_FunReg),},
    /*"P0_3"*/ {P0_3_Fun,P0_3_FunReg,GPIO_ARRAY_LENGTH(P0_3_Fun),GPIO_ARRAY_LENGTH(P0_3_FunReg),},
    /*"P0_4"*/ {P0_4_Fun,P0_4_FunReg,GPIO_ARRAY_LENGTH(P0_4_Fun),GPIO_ARRAY_LENGTH(P0_4_FunReg),},
    /*"P0_5"*/ {P0_5_Fun,P0_5_FunReg,GPIO_ARRAY_LENGTH(P0_5_Fun),GPIO_ARRAY_LENGTH(P0_5_FunReg),},
    /*"P0_6"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P0_7"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    #endif
    /*"****************************************************Group1*********************************************************************"*/
    /*"P1_0"*/ {P1_0_Fun,P1_0_FunReg,GPIO_ARRAY_LENGTH(P1_0_Fun),GPIO_ARRAY_LENGTH(P1_0_FunReg),},
    /*"P1_1"*/ {P1_1_Fun,P1_1_FunReg,GPIO_ARRAY_LENGTH(P1_1_Fun),GPIO_ARRAY_LENGTH(P1_1_FunReg),},
    /*"P1_2"*/ {P1_2_Fun,P1_2_FunReg,GPIO_ARRAY_LENGTH(P1_2_Fun),GPIO_ARRAY_LENGTH(P1_2_FunReg),},
    /*"P1_3"*/ {P1_3_Fun,P1_3_FunReg,GPIO_ARRAY_LENGTH(P1_3_Fun),GPIO_ARRAY_LENGTH(P1_3_FunReg),},
    /*"P1_4"*/ {P1_4_Fun,P1_4_FunReg,GPIO_ARRAY_LENGTH(P1_4_Fun),GPIO_ARRAY_LENGTH(P1_4_FunReg),},
    /*"P1_5"*/ {P1_5_Fun,P1_5_FunReg,GPIO_ARRAY_LENGTH(P1_5_Fun),GPIO_ARRAY_LENGTH(P1_5_FunReg),},
    /*"P1_6"*/ {P1_6_Fun,P1_6_FunReg,GPIO_ARRAY_LENGTH(P1_6_Fun),GPIO_ARRAY_LENGTH(P1_6_FunReg),},
    /*"P1_7"*/ {P1_7_Fun,P1_7_FunReg,GPIO_ARRAY_LENGTH(P1_7_Fun),GPIO_ARRAY_LENGTH(P1_7_FunReg),},
    /*"****************************************************Group2*********************************************************************"*/
    /*"P2_0"*/ {P2_0_Fun,P2_0_FunReg,GPIO_ARRAY_LENGTH(P2_0_Fun),GPIO_ARRAY_LENGTH(P2_0_FunReg),},
    /*"P2_1"*/ {P2_1_Fun,P2_1_FunReg,GPIO_ARRAY_LENGTH(P2_1_Fun),GPIO_ARRAY_LENGTH(P2_1_FunReg),},
    /*"P2_2"*/ {P2_2_Fun,P2_2_FunReg,GPIO_ARRAY_LENGTH(P2_2_Fun),GPIO_ARRAY_LENGTH(P2_2_FunReg),},
    /*"P2_3"*/ {P2_3_Fun,P2_3_FunReg,GPIO_ARRAY_LENGTH(P2_3_Fun),GPIO_ARRAY_LENGTH(P2_3_FunReg),},
    /*"P2_4"*/ {P2_4_Fun,P2_4_FunReg,GPIO_ARRAY_LENGTH(P2_4_Fun),GPIO_ARRAY_LENGTH(P2_4_FunReg),},
    /*"P2_5"*/ {P2_5_Fun,P2_5_FunReg,GPIO_ARRAY_LENGTH(P2_5_Fun),GPIO_ARRAY_LENGTH(P2_5_FunReg),},
    /*"P2_6"*/ {P2_6_Fun,P2_6_FunReg,GPIO_ARRAY_LENGTH(P2_6_Fun),GPIO_ARRAY_LENGTH(P2_6_FunReg),},
    /*"P2_7"*/ {P2_7_Fun,P2_7_FunReg,GPIO_ARRAY_LENGTH(P2_7_Fun),GPIO_ARRAY_LENGTH(P2_7_FunReg),},
    /*"****************************************************Group3*********************************************************************"*/
    /*"P3_0"*/ {P3_0_Fun,P3_0_FunReg,GPIO_ARRAY_LENGTH(P3_0_Fun),GPIO_ARRAY_LENGTH(P3_0_FunReg),},
    /*"P3_1"*/ {P3_1_Fun,P3_1_FunReg,GPIO_ARRAY_LENGTH(P3_1_Fun),GPIO_ARRAY_LENGTH(P3_1_FunReg),},
    /*"P3_2"*/ {P3_2_Fun,P3_2_FunReg,GPIO_ARRAY_LENGTH(P3_2_Fun),GPIO_ARRAY_LENGTH(P3_2_FunReg),},
    /*"P3_3"*/ {P3_3_Fun,P3_3_FunReg,GPIO_ARRAY_LENGTH(P3_3_Fun),GPIO_ARRAY_LENGTH(P3_3_FunReg),},
    /*"P3_4"*/ {P3_4_Fun,P3_4_FunReg,GPIO_ARRAY_LENGTH(P3_4_Fun),GPIO_ARRAY_LENGTH(P3_4_FunReg),},
    /*"P3_5"*/ {P3_5_Fun,P3_5_FunReg,GPIO_ARRAY_LENGTH(P3_5_Fun),GPIO_ARRAY_LENGTH(P3_5_FunReg),},
    /*"P3_6"*/ {P3_6_Fun,P3_6_FunReg,GPIO_ARRAY_LENGTH(P3_6_Fun),GPIO_ARRAY_LENGTH(P3_6_FunReg),},
    /*"P3_7"*/ {P3_7_Fun,P3_7_FunReg,GPIO_ARRAY_LENGTH(P3_7_Fun),GPIO_ARRAY_LENGTH(P3_7_FunReg),},
    /*"****************************************************Group4*********************************************************************"*/
    #ifdef RN202x_RN7326_SOC_V2/*VBAT域管脚*/
    /*"P4_0"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P4_1"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P4_2"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P4_3"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P4_4"*/ {P4_4_Fun,P4_4_FunReg,GPIO_ARRAY_LENGTH(P4_4_Fun),GPIO_ARRAY_LENGTH(P4_4_FunReg),},
    /*"P4_5"*/ {P4_5_Fun,P4_5_FunReg,GPIO_ARRAY_LENGTH(P4_5_Fun),GPIO_ARRAY_LENGTH(P4_5_FunReg),},
    /*"P4_6"*/ {P4_6_Fun,P4_6_FunReg,GPIO_ARRAY_LENGTH(P4_6_Fun),GPIO_ARRAY_LENGTH(P4_6_FunReg),},
    /*"P4_7"*/ {P4_7_Fun,P4_7_FunReg,GPIO_ARRAY_LENGTH(P4_7_Fun),GPIO_ARRAY_LENGTH(P4_7_FunReg),},
    #else
    /*"P4_0"*/ {P4_0_Fun,P4_0_FunReg,GPIO_ARRAY_LENGTH(P4_0_Fun),GPIO_ARRAY_LENGTH(P4_0_FunReg),},
    /*"P4_1"*/ {P4_1_Fun,P4_1_FunReg,GPIO_ARRAY_LENGTH(P4_1_Fun),GPIO_ARRAY_LENGTH(P4_1_FunReg),},
    /*"P4_2"*/ {P4_2_Fun,P4_2_FunReg,GPIO_ARRAY_LENGTH(P4_2_Fun),GPIO_ARRAY_LENGTH(P4_2_FunReg),},
    /*"P4_3"*/ {P4_3_Fun,P4_3_FunReg,GPIO_ARRAY_LENGTH(P4_3_Fun),GPIO_ARRAY_LENGTH(P4_3_FunReg),},
    /*"P4_4"*/ {P4_4_Fun,P4_4_FunReg,GPIO_ARRAY_LENGTH(P4_4_Fun),GPIO_ARRAY_LENGTH(P4_4_FunReg),},
    /*"P4_5"*/ {P4_5_Fun,P4_5_FunReg,GPIO_ARRAY_LENGTH(P4_5_Fun),GPIO_ARRAY_LENGTH(P4_5_FunReg),},
    /*"P4_6"*/ {P4_6_Fun,P4_6_FunReg,GPIO_ARRAY_LENGTH(P4_6_Fun),GPIO_ARRAY_LENGTH(P4_6_FunReg),},
    /*"P4_7"*/ {P4_7_Fun,P4_7_FunReg,GPIO_ARRAY_LENGTH(P4_7_Fun),GPIO_ARRAY_LENGTH(P4_7_FunReg),},
    #endif

    /*"****************************************************Group5*********************************************************************"*/
    #ifdef RN202x_RN7326_SOC_V2/*VBAT域管脚*/
    /*"P5_0"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P5_1"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P5_2"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P5_3"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P5_4"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P5_5"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P5_6"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P5_7"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    #else
    /*"P5_0"*/ {P5_0_Fun,P5_0_FunReg,GPIO_ARRAY_LENGTH(P5_0_Fun),GPIO_ARRAY_LENGTH(P5_0_FunReg),},
    /*"P5_1"*/ {P5_1_Fun,P5_1_FunReg,GPIO_ARRAY_LENGTH(P5_1_Fun),GPIO_ARRAY_LENGTH(P5_1_FunReg),},
    /*"P5_2"*/ {P5_2_Fun,P5_2_FunReg,GPIO_ARRAY_LENGTH(P5_2_Fun),GPIO_ARRAY_LENGTH(P5_2_FunReg),},
    /*"P5_3"*/ {P5_3_Fun,P5_3_FunReg,GPIO_ARRAY_LENGTH(P5_3_Fun),GPIO_ARRAY_LENGTH(P5_3_FunReg),},
    /*"P5_4"*/ {P5_4_Fun,P5_4_FunReg,GPIO_ARRAY_LENGTH(P5_4_Fun),GPIO_ARRAY_LENGTH(P5_4_FunReg),},
    /*"P5_5"*/ {P5_5_Fun,P5_5_FunReg,GPIO_ARRAY_LENGTH(P5_5_Fun),GPIO_ARRAY_LENGTH(P5_5_FunReg),},
    /*"P5_6"*/ {P5_6_Fun,P5_6_FunReg,GPIO_ARRAY_LENGTH(P5_6_Fun),GPIO_ARRAY_LENGTH(P5_6_FunReg),},
    /*"P5_7"*/ {P5_7_Fun,P5_7_FunReg,GPIO_ARRAY_LENGTH(P5_7_Fun),GPIO_ARRAY_LENGTH(P5_7_FunReg),},
    #endif
    /*"****************************************************Group6*********************************************************************"*/
    #ifdef RN202x_RN7326_SOC_V2
    /*"P6_0"*/ {P6_0_Fun,P6_0_FunReg,GPIO_ARRAY_LENGTH(P6_0_Fun),GPIO_ARRAY_LENGTH(P6_0_FunReg),},
    /*"P6_1"*/ {P6_1_Fun,P6_1_FunReg,GPIO_ARRAY_LENGTH(P6_1_Fun),GPIO_ARRAY_LENGTH(P6_1_FunReg),},
    /*"P6_2"*/ {P6_2_Fun,P6_2_FunReg,GPIO_ARRAY_LENGTH(P6_2_Fun),GPIO_ARRAY_LENGTH(P6_2_FunReg),},
    /*"P6_3"*/ {P6_3_Fun,P6_3_FunReg,GPIO_ARRAY_LENGTH(P6_3_Fun),GPIO_ARRAY_LENGTH(P6_3_FunReg),},
    /*"P6_4"*/ {P6_4_Fun,P6_4_FunReg,GPIO_ARRAY_LENGTH(P6_4_Fun),GPIO_ARRAY_LENGTH(P6_4_FunReg),},
    /*"P6_5"*/ {P6_5_Fun,P6_5_FunReg,GPIO_ARRAY_LENGTH(P6_5_Fun),GPIO_ARRAY_LENGTH(P6_5_FunReg),},
    /*"P6_6"*/ {P6_6_Fun,P6_6_FunReg,GPIO_ARRAY_LENGTH(P6_6_Fun),GPIO_ARRAY_LENGTH(P6_6_FunReg),},
    /*"P6_7"*/ {P6_7_Fun,P6_7_FunReg,GPIO_ARRAY_LENGTH(P6_7_Fun),GPIO_ARRAY_LENGTH(P6_7_FunReg),},
    #else
    /*"P6_0"*/ {P_LCD_Fun,P6_0_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P6_0_FunReg),},
    /*"P6_1"*/ {P_LCD_Fun,P6_1_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P6_1_FunReg),},
    /*"P6_2"*/ {P_LCD_Fun,P6_2_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P6_2_FunReg),},
    /*"P6_3"*/ {P_LCD_Fun,P6_3_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P6_3_FunReg),},
    /*"P6_4"*/ {P_LCD_Fun,P6_4_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P6_4_FunReg),},
    /*"P6_5"*/ {P_LCD_Fun,P6_5_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P6_5_FunReg),},
    /*"P6_6"*/ {P_LCD_Fun,P6_6_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P6_6_FunReg),},
    /*"P6_7"*/ {P_LCD_Fun,P6_7_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P6_7_FunReg),},
    #endif
    /*"****************************************************Group7*********************************************************************"*/
    #if defined(RN202x_RN7326_SOC_V2) || defined(RN831x_RN861x_MCU_V3)
    /*"P7_0"*/ {P7_0_Fun,P7_0_FunReg,GPIO_ARRAY_LENGTH(P7_0_Fun),GPIO_ARRAY_LENGTH(P7_0_FunReg),},
    /*"P7_1"*/ {P7_1_Fun,P7_1_FunReg,GPIO_ARRAY_LENGTH(P7_1_Fun),GPIO_ARRAY_LENGTH(P7_1_FunReg),},
    /*"P7_2"*/ {P7_2_Fun,P7_2_FunReg,GPIO_ARRAY_LENGTH(P7_2_Fun),GPIO_ARRAY_LENGTH(P7_2_FunReg),},
    /*"P7_3"*/ {P7_3_Fun,P7_3_FunReg,GPIO_ARRAY_LENGTH(P7_3_Fun),GPIO_ARRAY_LENGTH(P7_3_FunReg),},
    /*"P7_4"*/ {P7_4_Fun,P7_4_FunReg,GPIO_ARRAY_LENGTH(P7_4_Fun),GPIO_ARRAY_LENGTH(P7_4_FunReg),},
    /*"P7_5"*/ {P7_5_Fun,P7_5_FunReg,GPIO_ARRAY_LENGTH(P7_5_Fun),GPIO_ARRAY_LENGTH(P7_5_FunReg),},
    /*"P7_6"*/ {P7_6_Fun,P7_6_FunReg,GPIO_ARRAY_LENGTH(P7_6_Fun),GPIO_ARRAY_LENGTH(P7_6_FunReg),},
    /*"P7_7"*/ {P7_7_Fun,P7_7_FunReg,GPIO_ARRAY_LENGTH(P7_7_Fun),GPIO_ARRAY_LENGTH(P7_7_FunReg),},
    #elif defined(RN821x_RN721x_SOC_V3)
    /*"P7_0"*/ {P7_0_Fun,P7_0_FunReg,GPIO_ARRAY_LENGTH(P7_0_Fun),GPIO_ARRAY_LENGTH(P7_0_FunReg),},
    /*"P7_1"*/ {P7_1_Fun,P7_1_FunReg,GPIO_ARRAY_LENGTH(P7_1_Fun),GPIO_ARRAY_LENGTH(P7_1_FunReg),},
    /*"P7_2"*/ {P7_2_Fun,P7_2_FunReg,GPIO_ARRAY_LENGTH(P7_2_Fun),GPIO_ARRAY_LENGTH(P7_2_FunReg),},
    /*"P7_3"*/ {P7_3_Fun,P7_3_FunReg,GPIO_ARRAY_LENGTH(P7_3_Fun),GPIO_ARRAY_LENGTH(P7_3_FunReg),},
    /*"P7_4"*/ {P_LCD_Fun,P7_4_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P7_4_FunReg),},
    /*"P7_5"*/ {P_LCD_Fun,P7_5_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P7_5_FunReg),},
    /*"P7_6"*/ {P_LCD_Fun,P7_6_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P7_6_FunReg),},
    /*"P7_7"*/ {P_LCD_Fun,P7_7_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P7_7_FunReg),},
    #else
    /*"P7_0"*/ {P_LCD_Fun,P7_0_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P7_0_FunReg),},
    /*"P7_1"*/ {P_LCD_Fun,P7_1_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P7_1_FunReg),},
    /*"P7_2"*/ {P_LCD_Fun,P7_2_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P7_2_FunReg),},
    /*"P7_3"*/ {P_LCD_Fun,P7_3_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P7_3_FunReg),},
    /*"P7_4"*/ {P_LCD_Fun,P7_4_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P7_4_FunReg),},
    /*"P7_5"*/ {P_LCD_Fun,P7_5_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P7_5_FunReg),},
    /*"P7_6"*/ {P_LCD_Fun,P7_6_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P7_6_FunReg),},
    /*"P7_7"*/ {P_LCD_Fun,P7_7_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P7_7_FunReg),},
    #endif
    /*"****************************************************Group8*********************************************************************"*/
    #if defined(RN202x_RN7326_SOC_V2) || defined(RN831x_RN861x_MCU_V3)
    /*"P8_0"*/ {P8_0_Fun,P8_0_FunReg,GPIO_ARRAY_LENGTH(P8_0_Fun),GPIO_ARRAY_LENGTH(P8_0_FunReg),},
    /*"P8_1"*/ {P8_1_Fun,P8_1_FunReg,GPIO_ARRAY_LENGTH(P8_1_Fun),GPIO_ARRAY_LENGTH(P8_1_FunReg),},
    /*"P8_2"*/ {P8_2_Fun,P8_2_FunReg,GPIO_ARRAY_LENGTH(P8_2_Fun),GPIO_ARRAY_LENGTH(P8_2_FunReg),},
    /*"P8_3"*/ {P8_3_Fun,P8_3_FunReg,GPIO_ARRAY_LENGTH(P8_3_Fun),GPIO_ARRAY_LENGTH(P8_3_FunReg),},
    /*"P8_4"*/ {P8_4_Fun,P8_4_FunReg,GPIO_ARRAY_LENGTH(P8_4_Fun),GPIO_ARRAY_LENGTH(P8_4_FunReg),},
    /*"P8_5"*/ {P8_5_Fun,P8_5_FunReg,GPIO_ARRAY_LENGTH(P8_5_Fun),GPIO_ARRAY_LENGTH(P8_5_FunReg),},
    /*"P8_6"*/ {P8_6_Fun,P8_6_FunReg,GPIO_ARRAY_LENGTH(P8_6_Fun),GPIO_ARRAY_LENGTH(P8_6_FunReg),},
    /*"P8_7"*/ {P8_7_Fun,P8_7_FunReg,GPIO_ARRAY_LENGTH(P8_7_Fun),GPIO_ARRAY_LENGTH(P8_7_FunReg),},
    #else
    /*"P8_0"*/ {P8_0_Fun,P8_0_FunReg,GPIO_ARRAY_LENGTH(P8_0_Fun),GPIO_ARRAY_LENGTH(P8_0_FunReg),},
    /*"P8_1"*/ {P8_1_Fun,P8_1_FunReg,GPIO_ARRAY_LENGTH(P8_1_Fun),GPIO_ARRAY_LENGTH(P8_1_FunReg),},
    /*"P8_2"*/ {P8_2_Fun,P8_2_FunReg,GPIO_ARRAY_LENGTH(P8_2_Fun),GPIO_ARRAY_LENGTH(P8_2_FunReg),},
    /*"P8_3"*/ {P8_3_Fun,P8_3_FunReg,GPIO_ARRAY_LENGTH(P8_3_Fun),GPIO_ARRAY_LENGTH(P8_3_FunReg),},
    /*"P8_4"*/ {P8_4_Fun,P8_4_FunReg,GPIO_ARRAY_LENGTH(P8_4_Fun),GPIO_ARRAY_LENGTH(P8_4_FunReg),},
    /*"P8_5"*/ {P_LCD_Fun,P8_5_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P8_5_FunReg),},
    /*"P8_6"*/ {P_LCD_Fun,P8_6_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P8_6_FunReg),},
    /*"P8_7"*/ {P_LCD_Fun,P8_7_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P8_7_FunReg),},
    #endif
    /*"****************************************************Group9*********************************************************************"*/
    #ifdef RN202x_RN7326_SOC_V2
    /*"P9_0"*/ {P9_0_Fun,P9_0_FunReg,GPIO_ARRAY_LENGTH(P9_0_Fun),GPIO_ARRAY_LENGTH(P9_0_FunReg),},
    /*"P9_1"*/ {P9_1_Fun,P9_1_FunReg,GPIO_ARRAY_LENGTH(P9_1_Fun),GPIO_ARRAY_LENGTH(P9_1_FunReg),},
    /*"P9_2"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P9_3"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P9_4"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P9_5"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P9_6"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P9_7"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    #elif defined(RN831x_RN861x_MCU_V3)
    /*"P9_0"*/ {P9_0_Fun,P9_0_FunReg,GPIO_ARRAY_LENGTH(P9_0_Fun),GPIO_ARRAY_LENGTH(P9_0_FunReg),},
    /*"P9_1"*/ {P9_1_Fun,P9_1_FunReg,GPIO_ARRAY_LENGTH(P9_1_Fun),GPIO_ARRAY_LENGTH(P9_1_FunReg),},
    /*"P9_2"*/ {P9_2_Fun,P9_2_FunReg,GPIO_ARRAY_LENGTH(P9_2_Fun),GPIO_ARRAY_LENGTH(P9_2_FunReg),},
    /*"P9_3"*/ {P9_3_Fun,P9_3_FunReg,GPIO_ARRAY_LENGTH(P9_3_Fun),GPIO_ARRAY_LENGTH(P9_3_FunReg),},
    /*"P9_4"*/ {P9_4_Fun,P9_4_FunReg,GPIO_ARRAY_LENGTH(P9_4_Fun),GPIO_ARRAY_LENGTH(P9_4_FunReg),},
    /*"P9_5"*/ {P9_5_Fun,P9_5_FunReg,GPIO_ARRAY_LENGTH(P9_5_Fun),GPIO_ARRAY_LENGTH(P9_5_FunReg),},
    /*"P9_6"*/ {P9_6_Fun,P9_6_FunReg,GPIO_ARRAY_LENGTH(P9_6_Fun),GPIO_ARRAY_LENGTH(P9_6_FunReg),},
    /*"P9_7"*/ {P9_7_Fun,P9_7_FunReg,GPIO_ARRAY_LENGTH(P9_7_Fun),GPIO_ARRAY_LENGTH(P9_7_FunReg),},
    #elif defined(RN821x_RN721x_SOC_V3)
    /*"P9_0"*/ {P9_0_Fun,P9_0_FunReg,GPIO_ARRAY_LENGTH(P9_0_Fun),GPIO_ARRAY_LENGTH(P9_0_FunReg),},
    /*"P9_1"*/ {P9_1_Fun,P9_1_FunReg,GPIO_ARRAY_LENGTH(P9_1_Fun),GPIO_ARRAY_LENGTH(P9_1_FunReg),},
    /*"P9_2"*/ {P9_2_Fun,P9_2_FunReg,GPIO_ARRAY_LENGTH(P9_2_Fun),GPIO_ARRAY_LENGTH(P9_2_FunReg),},
    /*"P9_3"*/ {P9_3_Fun,P9_3_FunReg,GPIO_ARRAY_LENGTH(P9_3_Fun),GPIO_ARRAY_LENGTH(P9_3_FunReg),},
    /*"P9_4"*/ {P9_4_Fun,P9_4_FunReg,GPIO_ARRAY_LENGTH(P9_4_Fun),GPIO_ARRAY_LENGTH(P9_4_FunReg),},
    /*"P9_5"*/ {P9_5_Fun,P9_5_FunReg,GPIO_ARRAY_LENGTH(P9_5_Fun),GPIO_ARRAY_LENGTH(P9_5_FunReg),},
    /*"P9_6"*/ {P_LCD_Fun,P9_6_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P9_6_FunReg),},
    /*"P9_7"*/ {P_LCD_Fun,P9_7_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P9_7_FunReg),},
    #else
    /*"P9_0"*/ {P_LCD_Fun,P9_0_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P9_0_FunReg),},
    /*"P9_1"*/ {P_LCD_Fun,P9_1_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P9_1_FunReg),},
    /*"P9_2"*/ {P_LCD_Fun,P9_2_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P9_2_FunReg),},
    /*"P9_3"*/ {P_LCD_Fun,P9_3_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P9_3_FunReg),},
    /*"P9_4"*/ {P_LCD_Fun,P9_4_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P9_4_FunReg),},
    /*"P9_5"*/ {P_LCD_Fun,P9_5_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P9_5_FunReg),},
    /*"P9_6"*/ {P_LCD_Fun,P9_6_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P9_6_FunReg),},
    /*"P9_7"*/ {P_LCD_Fun,P9_7_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P9_7_FunReg),},
    #endif
    /*"****************************************************Group10*********************************************************************"*/
    #ifdef RN202x_RN7326_SOC_V2
    /*"P10_0"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P10_1"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P10_2"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P10_3"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P10_4"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P10_5"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P10_6"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P10_7"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    #else
    /*"P10_0"*/ {P_LCD_Fun,P10_0_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P10_0_FunReg),},
    /*"P10_1"*/ {P_LCD_Fun,P10_1_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P10_1_FunReg),},
    /*"P10_2"*/ {P_LCD_Fun,P10_2_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P10_2_FunReg),},
    /*"P10_3"*/ {P_LCD_Fun,P10_3_FunReg,GPIO_ARRAY_LENGTH(P_LCD_Fun),GPIO_ARRAY_LENGTH(P10_3_FunReg),},
    /*"P10_4"*/ {P10_4_Fun,P10_4_FunReg,GPIO_ARRAY_LENGTH(P10_4_Fun),GPIO_ARRAY_LENGTH(P10_4_FunReg),},
    /*"P10_5"*/ {P10_5_Fun,P10_5_FunReg,GPIO_ARRAY_LENGTH(P10_5_Fun),GPIO_ARRAY_LENGTH(P10_5_FunReg),},
    /*"P10_6"*/ {P10_6_Fun,P10_6_FunReg,GPIO_ARRAY_LENGTH(P10_6_Fun),GPIO_ARRAY_LENGTH(P10_6_FunReg),},
    /*"P10_7"*/ {P10_7_Fun,P10_7_FunReg,GPIO_ARRAY_LENGTH(P10_7_Fun),GPIO_ARRAY_LENGTH(P10_7_FunReg),},
    #endif
    /*"****************************************************Group11*********************************************************************"*/
    #ifdef RN202x_RN7326_SOC_V2
    /*"P11_0"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P11_1"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P11_2"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P11_3"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P11_4"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P11_5"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P11_6"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P11_7"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    #elif defined(RN821x_RN721x_SOC_V2)
    /*"P11_0"*/ {P11_0_Fun,P11_0_FunReg,GPIO_ARRAY_LENGTH(P11_0_Fun),GPIO_ARRAY_LENGTH(P11_0_FunReg),},
    /*"P11_1"*/ {P11_1_Fun,P11_1_FunReg,GPIO_ARRAY_LENGTH(P11_1_Fun),GPIO_ARRAY_LENGTH(P11_1_FunReg),},
    /*"P11_2"*/ {P11_2_Fun,P11_2_FunReg,GPIO_ARRAY_LENGTH(P11_2_Fun),GPIO_ARRAY_LENGTH(P11_2_FunReg),},
    /*"P11_3"*/ {P11_3_Fun,P11_3_FunReg,GPIO_ARRAY_LENGTH(P11_3_Fun),GPIO_ARRAY_LENGTH(P11_3_FunReg),},
    /*"P11_4"*/ {P11_4_Fun,P11_4_FunReg,GPIO_ARRAY_LENGTH(P11_4_Fun),GPIO_ARRAY_LENGTH(P11_4_FunReg),},
    /*"P11_5"*/ {P11_5_Fun,P11_5_FunReg,GPIO_ARRAY_LENGTH(P11_5_Fun),GPIO_ARRAY_LENGTH(P11_5_FunReg),},
    /*"P11_6"*/ {P11_6_Fun,P11_6_FunReg,GPIO_ARRAY_LENGTH(P11_6_Fun),GPIO_ARRAY_LENGTH(P11_6_FunReg),},
    /*"P11_7"*/ {P11_7_Fun,P11_7_FunReg,GPIO_ARRAY_LENGTH(P11_7_Fun),GPIO_ARRAY_LENGTH(P11_7_FunReg),},
    #else
    /*"P11_0"*/ {P11_0_Fun,P11_0_FunReg,GPIO_ARRAY_LENGTH(P11_0_Fun),GPIO_ARRAY_LENGTH(P11_0_FunReg),},
    /*"P11_1"*/ {P11_1_Fun,P11_1_FunReg,GPIO_ARRAY_LENGTH(P11_1_Fun),GPIO_ARRAY_LENGTH(P11_1_FunReg),},
    /*"P11_2"*/ {P11_2_Fun,P11_2_FunReg,GPIO_ARRAY_LENGTH(P11_2_Fun),GPIO_ARRAY_LENGTH(P11_2_FunReg),},
    /*"P11_3"*/ {P11_3_Fun,P11_3_FunReg,GPIO_ARRAY_LENGTH(P11_3_Fun),GPIO_ARRAY_LENGTH(P11_3_FunReg),},
    /*"P11_4"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P11_5"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P11_6"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P11_7"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    #endif
/*"****************************************************Group12*********************************************************************"*/
    #if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
    /*"P12_0"*/ {P_COM_Fun,P12_0_FunReg,GPIO_ARRAY_LENGTH(P_COM_Fun),GPIO_ARRAY_LENGTH(P12_0_FunReg),},
    /*"P12_1"*/ {P_COM_Fun,P12_1_FunReg,GPIO_ARRAY_LENGTH(P_COM_Fun),GPIO_ARRAY_LENGTH(P12_1_FunReg),},
    /*"P12_2"*/ {P_COM_Fun,P12_2_FunReg,GPIO_ARRAY_LENGTH(P_COM_Fun),GPIO_ARRAY_LENGTH(P12_2_FunReg),},
    /*"P12_3"*/ {P_COM_Fun,P12_3_FunReg,GPIO_ARRAY_LENGTH(P_COM_Fun),GPIO_ARRAY_LENGTH(P12_3_FunReg),},
    /*"P12_4"*/ {P12_4_Fun,P12_4_FunReg,GPIO_ARRAY_LENGTH(P12_4_Fun),GPIO_ARRAY_LENGTH(P12_4_FunReg),},
    /*"P12_5"*/ {P12_5_Fun,P12_5_FunReg,GPIO_ARRAY_LENGTH(P12_5_Fun),GPIO_ARRAY_LENGTH(P12_5_FunReg),},
    /*"P12_6"*/ {P12_6_Fun,P12_6_FunReg,GPIO_ARRAY_LENGTH(P12_6_Fun),GPIO_ARRAY_LENGTH(P12_6_FunReg),},
    /*"P12_7"*/ {P12_7_Fun,P12_7_FunReg,GPIO_ARRAY_LENGTH(P12_7_Fun),GPIO_ARRAY_LENGTH(P12_7_FunReg),},
    #elif defined(RN202x_RN7326_SOC_V2)
    /*"P12_0"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P12_1"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P12_2"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P12_3"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P12_4"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P12_5"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P12_6"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P12_7"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    #else
    /*"P12_0"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P12_1"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P12_2"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P12_3"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P12_4"*/ {P12_4_Fun,P12_4_FunReg,GPIO_ARRAY_LENGTH(P12_4_Fun),GPIO_ARRAY_LENGTH(P12_4_FunReg),},
    /*"P12_5"*/ {P12_5_Fun,P12_5_FunReg,GPIO_ARRAY_LENGTH(P12_5_Fun),GPIO_ARRAY_LENGTH(P12_5_FunReg),},
    /*"P12_6"*/ {P12_6_Fun,P12_6_FunReg,GPIO_ARRAY_LENGTH(P12_6_Fun),GPIO_ARRAY_LENGTH(P12_6_FunReg),},
    /*"P12_7"*/ {P12_7_Fun,P12_7_FunReg,GPIO_ARRAY_LENGTH(P12_7_Fun),GPIO_ARRAY_LENGTH(P12_7_FunReg),},
    #endif
/*"****************************************************Group13*********************************************************************"*/
    #if defined(RN821x_RN721x_SOC_V2) ||  defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
    /*"P13_0"*/ {P13_0_Fun,P13_0_FunReg,GPIO_ARRAY_LENGTH(P13_0_Fun),GPIO_ARRAY_LENGTH(P13_0_FunReg),},
    /*"P13_1"*/ {P13_1_Fun,P13_1_FunReg,GPIO_ARRAY_LENGTH(P13_1_Fun),GPIO_ARRAY_LENGTH(P13_1_FunReg),},
    /*"P13_2"*/ {P13_2_Fun,P13_2_FunReg,GPIO_ARRAY_LENGTH(P13_2_Fun),GPIO_ARRAY_LENGTH(P13_2_FunReg),},
    /*"P13_3"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P13_4"*/ {P13_4_Fun,P13_4_FunReg,GPIO_ARRAY_LENGTH(P13_4_Fun),GPIO_ARRAY_LENGTH(P13_4_FunReg),},
    /*"P13_5"*/ {P13_5_Fun,P13_5_FunReg,GPIO_ARRAY_LENGTH(P13_5_Fun),GPIO_ARRAY_LENGTH(P13_5_FunReg),},
    /*"P13_6"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P13_7"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    #else
    /*"P13_0"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P13_1"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P13_2"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P13_3"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P13_4"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P13_5"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P13_6"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P13_7"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    #endif
/*"****************************************************Group14*********************************************************************"*/
    #ifdef RN831x_RN861x_MCU_V3
    /*"P14_0"*/ {P14_0_Fun,P14_0_FunReg,GPIO_ARRAY_LENGTH(P14_0_Fun),GPIO_ARRAY_LENGTH(P14_0_FunReg),},
    /*"P14_1"*/ {P14_1_Fun,P14_1_FunReg,GPIO_ARRAY_LENGTH(P14_1_Fun),GPIO_ARRAY_LENGTH(P14_1_FunReg),},
    /*"P14_2"*/ {P14_2_Fun,P14_2_FunReg,GPIO_ARRAY_LENGTH(P14_2_Fun),GPIO_ARRAY_LENGTH(P14_2_FunReg),},
    /*"P14_3"*/ {P14_3_Fun,P14_3_FunReg,GPIO_ARRAY_LENGTH(P14_3_Fun),GPIO_ARRAY_LENGTH(P14_3_FunReg),},
    /*"P14_4"*/ {P14_4_Fun,P14_4_FunReg,GPIO_ARRAY_LENGTH(P14_4_Fun),GPIO_ARRAY_LENGTH(P14_4_FunReg),},
    /*"P14_5"*/ {P14_5_Fun,P14_5_FunReg,GPIO_ARRAY_LENGTH(P14_5_Fun),GPIO_ARRAY_LENGTH(P14_5_FunReg),},
    /*"P14_6"*/ {P14_6_Fun,P14_6_FunReg,GPIO_ARRAY_LENGTH(P14_6_Fun),GPIO_ARRAY_LENGTH(P14_6_FunReg),},
    /*"P14_7"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    #elif defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2)
    /*"P14_0"*/ {P14_0_Fun,P14_0_FunReg,GPIO_ARRAY_LENGTH(P14_0_Fun),GPIO_ARRAY_LENGTH(P14_0_FunReg),},
    /*"P14_1"*/ {P14_1_Fun,P14_1_FunReg,GPIO_ARRAY_LENGTH(P14_1_Fun),GPIO_ARRAY_LENGTH(P14_1_FunReg),},
    /*"P14_2"*/ {P14_2_Fun,P14_2_FunReg,GPIO_ARRAY_LENGTH(P14_2_Fun),GPIO_ARRAY_LENGTH(P14_2_FunReg),},
    /*"P14_3"*/ {P14_3_Fun,P14_3_FunReg,GPIO_ARRAY_LENGTH(P14_3_Fun),GPIO_ARRAY_LENGTH(P14_3_FunReg),},
    /*"P14_4"*/ {P14_4_Fun,P14_4_FunReg,GPIO_ARRAY_LENGTH(P14_4_Fun),GPIO_ARRAY_LENGTH(P14_4_FunReg),},
    /*"P14_5"*/ {P14_5_Fun,P14_5_FunReg,GPIO_ARRAY_LENGTH(P14_5_Fun),GPIO_ARRAY_LENGTH(P14_5_FunReg),},
    /*"P14_6"*/ {P14_6_Fun,P14_6_FunReg,GPIO_ARRAY_LENGTH(P14_6_Fun),GPIO_ARRAY_LENGTH(P14_6_FunReg),},
    /*"P14_7"*/ {P14_7_Fun,P14_7_FunReg,GPIO_ARRAY_LENGTH(P14_7_Fun),GPIO_ARRAY_LENGTH(P14_7_FunReg),},
    #elif defined(RN821x_RN721x_SOC_V3)
    /*"P14_0"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P14_1"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P14_2"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P14_3"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P14_4"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P14_5"*/ {P14_5_Fun,P14_5_FunReg,GPIO_ARRAY_LENGTH(P14_5_Fun),GPIO_ARRAY_LENGTH(P14_5_FunReg),},
    /*"P14_6"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P14_7"*/ {P14_7_Fun,P14_7_FunReg,GPIO_ARRAY_LENGTH(P14_7_Fun),GPIO_ARRAY_LENGTH(P14_7_FunReg),},
    #else
    /*"P14_0"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P14_1"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P14_2"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P14_3"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P14_4"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P14_5"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P14_6"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    /*"P14_7"*/ {NULL,NULL,LL_ZERO,LL_ZERO,},
    #endif
};
/* Exported data  ----------------------------------------------------------*/

/* Private functions ------------------------------------------------------------*/
#ifdef RN202x_RN7326_SOC_V2
/*****************************************************************************
** Function name:   ll_GPIO_AHB_CfgFun
** Description:     设置VBAT域IO口功能，如此口无对应功能，退出
**				注意:有部分IO有关联性，配置后会改变其他IO属性
** Parameters:  pin_id:端口号
**              Fun:IO口对应功能，所有功能使用eGPIOFunction_TypeDef枚举
** Returned value:NONE
******************************************************************************/
static ErrorStatus ll_gpio_AHB_CfgFun(uint8_t pin_id, eGPIOFunction_TypeDef Fun)
{
    ErrorStatus status = ERN_ERROR;
    uint32_t RegMask,RegValue;
	switch(pin_id)
	{
		case PIN_4_0:
			RegMask = ~0x03U;    /* PCB0[1], PCB0[0] */
            status = ERN_SUCCESS;
            if(_NORMALIO == Fun)/* =0000: 选择为IO口； */
			{
				RegValue = 0x00U;
			}
			else if(_KEY == Fun)/* =0001: 选择为KEY6；（1.具备开盖事件记录功能；2.PC40选择KEY6较P46优先级高）； */
			{
				RegValue = 0x01U;
			}
			else if(_AIN == Fun)/* =0010: 选择为模拟输入口AIN3（外部编号），AIN6(内部编号） */
			{
				RegValue = 0x02U;
			}
			else
			{
                status = ERN_ERROR;
			}
			break;
		case PIN_4_1:/* PCB0[3], PCB0[2] */
			RegMask = ~0x0CU;
            status = ERN_SUCCESS;
            if(_NORMALIO == Fun)/* =0000: 选择为IO口； */
			{
				RegValue = 0x00U;
			}
			else if(_KEY == Fun)/* =0001: 选择为KEY7；（1.具备开盖事件记录功能；2、PC41选择KEY7较P47优先级高）；  */
			{
				RegValue = 0x04U;
			}
			else if(Fun == _AIN)/* =0010: 选择为模拟输入口AIN2（外部编号）；AIN5(内部编号）； */
			{
				RegValue = 0x08U;
			}
			else
			{
                status = ERN_ERROR;
			}
			break;
		case PIN_4_2:/* PCB0[8], PCB0[5], PCB0[4] */
			RegMask = ~0x130U;
            status = ERN_SUCCESS;
            if(_NORMALIO == Fun)/* =0000: 选择为IO口； */
			{
				RegValue = 0x00U;
			}
			else if(_RTC_OUT == Fun) /* =0001:选择为RTCOUT；（P02，P37若配置为RTCOUT时，可同时输出；） */
			{
				RegValue = 0x10U;
			}
			else if(_AIN == Fun)/* =0010:选择为模拟输入口AIN1； */
			{
				RegValue = 0x20U;
			}
			else if((_IIC == Fun)||(_IIC0 == Fun))/* =0100:选择为SCL,I2C0时钟 */
			{
				RegValue = 0x100U;
			}
			else if(_KEY == Fun)/* =0101:选择为KEY6，同时选择时P40口优先级高 */
			{
				RegValue = 0x110U;
			}
			else if(_RTC_1S == Fun)/* =0110:选择为RTC_1S(直接是万年历秒脉冲，不是PLL_1Hz) */
			{
				RegValue = 0x120U;
			}
			else
			{
                status = ERN_ERROR;
			}
			break;
		case PIN_4_3:/* PCB0[7], PCB0[6] */
            RegMask = ~0xC0;
            status = ERN_SUCCESS;
            if(_NORMALIO == Fun)/* =0000: 选择为IO口； */
			{
				RegValue = 0x00U;
			}
			else if(_AIN == Fun)/* =01：选择为模拟输入口AIN0； */
			{
				RegValue = 0x40U;
			}
			else if((_IIC == Fun)||(_IIC0 == Fun))/* =10：选择为SDA,I2C0数据 */
			{
				RegValue = 0x80U;
			}
			else if(_KEY_OUT == Fun)/* =11：选择为按键触发指示信号输出KEYOUT */
			{
				RegValue = 0xC0U;
			}
			else
			{
                status = ERN_ERROR;
			}
			break;
		case PIN_5_0:/* PCB0[13], PCB0[12] */
			RegMask = ~0x3000U;
            status = ERN_SUCCESS;
            if(_NORMALIO == Fun)/* =0000: 选择为IO口； */
			{
				RegValue = 0x00U;
			}
			else if(_AIN == Fun)/* =01：选择为模拟输入口AIN4（外部编号）；AIN3(内部编号） */
			{
				RegValue = 0x1000U;
			}
			else if(_KEY == Fun)/* =10：选择为KEY6，P40和P42优先级高 */
			{
				RegValue = 0x2000U;
			}
			else if(_RTC_OUT == Fun)/* =11：选择为RTC_OUT */
			{
				RegValue = 0x3000U;
			}
			else
			{
                status = ERN_ERROR;
			}
			break;
		case PIN_5_1:/* PCB0[15], PCB0[14] */
            RegMask = ~0xC000U;
            status = ERN_SUCCESS;
            if(_NORMALIO == Fun)/* =0000: 选择为IO口； */
			{
				RegValue = 0x00U;
			}
			else if(Fun == _AIN)/* =01：选择为模拟输入口AIN5（外部编号）；AIN2(内部编号） */
			{
				RegValue = 0x4000U;
			}
			else if(_KEY_OUT == Fun)/* =10：选择为KEYOUT */
			{
				RegValue = 0x8000U;
			}
			else if(_RTC_1S == Fun)/* =11：选择为RTC_1S */
			{
				RegValue = 0xC000U;
			}
			else
			{
                status = ERN_ERROR;
			}
			break;
        case PIN_5_2:/* PCB2[1], PCB2[0] */
            RegMask = ~0x0003U;
            status = ERN_SUCCESS;
            if(_NORMALIO == Fun)/* =0000: 选择为IO口； */
			{
				RegValue = 0x00U;
			}
			else if(Fun == _AIN)/* =11：选择为AIN4(内部编号） */
			{
				RegValue = 0x0003U;
			}
			else//GPIO
			{
                status = ERN_ERROR;
			}
			break;
        case PIN_5_3:/* PCB2[3], PCB2[2] */
            RegMask = ~0x000CU;
            status = ERN_SUCCESS;
            if(_NORMALIO == Fun)/* =0000: 选择为IO口； */
			{
				RegValue = 0x00U;
			}
			else if(Fun == _AIN)/* =11：选择为AIN7(内部编号） */
			{
				RegValue = 0x000CU;
			}
			else//GPIO
			{
                status = ERN_ERROR;
			}
			break;
		default:
            status = ERN_ERROR;
	        break;
	}
    if(ERN_SUCCESS == status)
    {
        GPIO_PSW1_EnableWrite;
        if((PIN_5_2 == pin_id)||(PIN_5_3 == pin_id))
        {
            GPIO_APB->PCB2 &= RegMask;
            GPIO_APB->PCB2 |= RegValue;
        }
        else
        {
            GPIO_APB->PCB0 &= RegMask;
            GPIO_APB->PCB0 |= RegValue;
        }
        GPIO_PB0MASK_Enable;
        GPIO_PSW1_DisableWrite;
    }
    return status;
}
#endif

#if defined(RN821x_RN721x_SOC_B) || \
    defined(RN821x_RN721x_SOC_C) || \
    defined(RN821x_RN721x_SOC_D) || \
    defined(RN821x_RN721x_SOC_V2) || \
    defined(RN821x_RN721x_SOC_V3) || \
    defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3)
/*****************************************************************************
** Function name:   ll_gpio_SpclUart_CfgFun
** Description:     设置IO口Uart相关的特殊功能，
** Parameters:  pin_id:端口号
**              Fun:IO口对应功能，所有功能使用eGPIOFunction_TypeDef枚举
** Returned value:NONE
******************************************************************************/
static void ll_gpio_SpclUart_CfgFun(uint8_t pin_id, eGPIOFunction_TypeDef Fun)
{
#ifdef RN831x_RN861x_MCU_V3
    if ((_UART2 == Fun)&&((PIN_0_0 == pin_id)||(PIN_0_1 == pin_id)))
    {
        BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 30U) = 1U;
        BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 31U) = 0U;
        BITBAND_ADDR((uint32_t)(&GPIO->PCA3), 28U) = 0U;
    }
#elif defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
    if(_UART2 == Fun)
    {
        if((PIN_0_0 == pin_id)||(PIN_0_1 == pin_id))
        {
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 30U) = 1U;
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 31U) = 0U;
            BITBAND_ADDR((uint32_t)(&GPIO->PCA3), 28U) = 0U;
        }
        else if((PIN_2_0 == pin_id)||(PIN_2_1 == pin_id))
        {
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 30U) = 0U;
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 31U) = 1U;
            BITBAND_ADDR((uint32_t)(&GPIO->PCA3), 28U) = 0U;
        }
        else if((PIN_2_2 == pin_id)||(PIN_2_3 == pin_id))
        {
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 30U) = 1U;
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 31U) = 1U;
            BITBAND_ADDR((uint32_t)(&GPIO->PCA3), 28U) = 0U;
        }
        else if((PIN_2_4 == pin_id)||(PIN_2_5 == pin_id))
        {
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 30U) = 0U;
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 31U) = 0U;
            BITBAND_ADDR((uint32_t)(&GPIO->PCA3), 28U) = 0U;
        }
        else if((PIN_2_6 == pin_id)||(PIN_2_7 == pin_id))
        {
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 30U) = 0U;
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 31U) = 0U;
            BITBAND_ADDR((uint32_t)(&GPIO->PCA3), 28U) = 1U;
        }
    }
#else
    if (_UART2 == Fun) /*"UART2_SEL需要提前配置"*/
    {
        switch (pin_id)
        {
        case PIN_2_4: /*"P24/P25选择为UART2"*/
        case PIN_2_5:
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 30U) = 0U;
#if defined(RN831x_RN861x_MCU_V2)
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 31U) = 0U;
#endif
            break;
        case PIN_0_0: /*"P00/P01选择为UART2"*/
        case PIN_0_1:
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 30U) = 1U;
#if defined(RN831x_RN861x_MCU_V2)
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 31U) = 0U;
#endif
            break;
#ifdef RN831x_RN861x_MCU_V2
        case PIN_2_2: /*"P22/P23选择为UART2"*/
        case PIN_2_3:
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 30U) = 0U;
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 31U) = 1U;
            break;
        case PIN_2_6: /*"P26/P27选择为UART2"*/
        case PIN_2_7:
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 30U) = 1U;
            BITBAND_ADDR((uint32_t)(&GPIO->PCA0), 31U) = 1U;
            break;
#endif
        default:
            break;
        }
    }
#endif
}
#endif
#if defined(RN821x_RN721x_SOC_B) || \
    defined(RN821x_RN721x_SOC_C) || \
    defined(RN821x_RN721x_SOC_D) || \
    defined(RN821x_RN721x_SOC_V2) || \
    defined(RN821x_RN721x_SOC_V3) || \
    defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3)
/*****************************************************************************
** Function name:   ll_gpio_SpclSpi0_CfgFun
** Description:     设置IO口spi相关的特殊功能，
** Parameters:  pin_id:端口号
**              Fun:IO口对应功能，所有功能使用eGPIOFunction_TypeDef枚举
** Returned value:NONE
******************************************************************************/
static void ll_gpio_SpclSpi0_CfgFun(uint8_t pin_id)
{
    switch (pin_id)
    {
    case PIN_5_0: /*"选择P50/P51/P46/P47作为SPI0接口"*/
    case PIN_5_1:
    case PIN_4_6:
    case PIN_4_7:
        BITBAND_ADDR((uint32_t)(&GPIO->PCC), 28U) = 0U;
        break;
    case PIN_11_0: /*"选择P110~P113作为SPI0接口"*/
    case PIN_11_1:
    case PIN_11_2:
    case PIN_11_3:
        BITBAND_ADDR((uint32_t)(&GPIO->PCC), 28U) = 1U;
        break;
    default:
        break;
    }
}
#endif
#if defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
/*****************************************************************************
** Function name:   ll_gpio_SpclMadc_CfgFun
** Description:     设置IO口SAR相关的特殊功能，
** Parameters:  pin_id:端口号
**              Fun:IO口对应功能，所有功能使用eGPIOFunction_TypeDef枚举
** Returned value:NONE
******************************************************************************/
static void ll_gpio_SpclMadc_CfgFun(uint8_t pin_id, eGPIOFunction_TypeDef Fun)
{
    if (_AIN == Fun)
    {
        MADC->ANA_PAD |= (0x00000001U << ((uint32_t)pin_id - (uint32_t)(PIN_4_4)));
    }
    else
    {
        MADC->ANA_PAD &= (~(0x00000001U << ((uint32_t)pin_id - (uint32_t)(PIN_4_4))));
    }
}
#endif

#if defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3)
/*****************************************************************************
** Function name:   ll_gpio_SpclRtc_CfgFun
** Description:     设置IO口RTC相关的特殊功能，
** Parameters:  pin_id:端口号
**              Fun:IO口对应功能，所有功能使用eGPIOFunction_TypeDef枚举
** Returned value:NONE
******************************************************************************/
static void ll_gpio_SpclRtc_CfgFun(uint8_t pin_id, eGPIOFunction_TypeDef Fun)
{
    if (pin_id == PIN_5_6)
    {
        RTC->IOMODE &= 0xFFFFFF0FU;
        RTC->IOMODE |= 0xb0U;

        /*"P56 口的 RTCOUT 复用关系由 RTC 章节寄存器决定，与 GPIO 章节复用配置寄存器无关。
          当 VCC 无电， VBAT 上电时， P56 默认输出 1Hz。一旦 VCC上电， 1Hz 输出会被关闭。外围电路设计需要注意该问题。"*/
        if (Fun == _RTC_OUT)
        {
            RTC->IOEN |= 0x00a0U;
        }
        else
        {
            RTC->IOEN &= 0xFFFFFF0FU;
        }

        RTC->IOMODE &= 0xFFFFFF0FU;
    }
}
#endif
/* Exported functions ----------------------------------------------------------*/

/*****************************************************************************
** Function name:LL_GPIO_SetPin
** Description:　GPIO 输出电平
** Parameters: pin_id:端口号  value:输出电平
** Returned value:NONE
******************************************************************************/
void LL_GPIO_SetPin(uint8_t pin_id, uint8_t value)
{
    uint32_t pin_addr;
    uint8_t group_no;

    if (pin_id < PIN_nMAX)
    {
        group_no = (pin_id >> 3); /*"获取IO口组别号"*/
#ifdef RN202x_RN7326_SOC_V2
        if((PIN_4_0 == pin_id)||(PIN_4_1 == pin_id)||(PIN_4_2 == pin_id)||(PIN_4_3 == pin_id)
                ||(PIN_5_0 == pin_id)||(PIN_5_1 == pin_id)||(PIN_5_2 == pin_id)||(PIN_5_3 == pin_id))
        {
            GPIO_PSW1_EnableWrite;
            if(value)
            {
                GPIO_APB->PB0SET = (0x00000001U << (pin_id-PIN_4_0));/*"输出高电平"*/
            }
            else
            {
                GPIO_APB->PB0CLR = (0x00000001U << (pin_id-PIN_4_0));/*"输出低电平"*/
            }
            GPIO_PB0MASK_Enable;
            GPIO_PSW1_DisableWrite;
        }
        else
        {
            if(group_no <= 3U) /*" Group0 Group1 Group2 Group3"*/
            {
                pin_addr = (uint32_t )(&GPIO->PA);
            }
            else if(group_no == 4U)
            {
                pin_addr = (uint32_t)(&GPIO->PB1);
            }
            else if((group_no == 6U) ||(group_no == 7U) )
            {
                pin_addr = (uint32_t)(&GPIO->PB1);
                group_no--;
            }
            else if(group_no <= 9U)
            {
                pin_addr = (uint32_t)(&GPIO->PC);
            }
            else
            {
                return;
            }
            GPIO_PSW0_EnableWrite;		/*  打开IO操作使能 */
            BITBAND_ADDR(pin_addr,(pin_id & 0x07U)+(group_no & 0x03U)*8U) = value;
            GPIO_PSW0_DisableWrite;		/*  关闭IO操作使能 */
        }
#else
        /*"*********************查询相应寄存器地址************************"*/
        if (group_no <= 3U) /*" Group0 Group1 Group2 Group3"*/
        {
            pin_addr = (uint32_t)(&GPIO->PA);
        }
        else if (group_no <= 7U) /*" Group4 Group5 Group6 Group7"*/
        {
            pin_addr = (uint32_t)(&GPIO->PB);
        }
        else if (group_no <= 11U) /*" Group8 Group9 Group10 Group11"*/
        {
            pin_addr = (uint32_t)(&GPIO->PC);
        }
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3)
        else if (group_no <= 14U) /*" Group12 Group13 Group14 "*/
        {
            pin_addr = (uint32_t)(&GPIO->PD);
        }
#endif
        else
        {
            return;
        }
        /*"设置IO口输出value电平"*/
        BITBAND_ADDR(pin_addr, ((pin_id & 0x07U) + (group_no & 0x03U) * 8U)) = value;
#endif
    }

}

/*****************************************************************************
** Function name:   LL_GPIO_ReadPin
** Description:　读取GPIO 电平
** Parameters:  pin_id:端口号
** Returned value: GPIO输入电平
******************************************************************************/
uint8_t LL_GPIO_ReadPin(uint8_t pin_id)
{
    uint32_t pin_addr;
    uint8_t group_no, value;

    if (pin_id < PIN_nMAX)
    {
        group_no = (pin_id >> 3); /*"获取IO口组别号"*/
#ifdef RN202x_RN7326_SOC_V2
        if((PIN_4_0 == pin_id)||(PIN_4_1 == pin_id)||(PIN_4_2 == pin_id)||(PIN_4_3 == pin_id)
            ||(PIN_5_0 == pin_id)||(PIN_5_1 == pin_id)||(PIN_5_2 == pin_id)||(PIN_5_3 == pin_id))
        {
            GPIO_PB0MASK_Enable;		/* 打开PB0信号屏蔽寄存器 */
            return(((GPIO_APB->PB0 &(0x00000001 << (pin_id-PIN_4_0))) ? (0x01) : (0x00)));
        }
        else
        {
            if(group_no <= 3U) /*" Group0 Group1 Group2 Group3"*/
	        {
		        pin_addr = (uint32_t )(&GPIO->PA);
	        }
            else if(group_no == 4U) /*" Group4 "*/
            {
                pin_addr = (uint32_t)(&GPIO->PB1);
            }
            else if((group_no == 6U) ||(group_no == 7U))/*" Group6 Group7"*/
            {
                pin_addr = (uint32_t)(&GPIO->PB1);
                group_no--;
            }
            else if(group_no <= 9U)/*" Group8 Group9"*/
            {
                pin_addr = (uint32_t)(&GPIO->PC);
            }
	        else
	        {
		        return(0U);
	        }
            /*" 返回IO口状态"*/
	        value = BITBAND_ADDR(pin_addr,(pin_id &0x07)+(group_no%4)*8) ;
	        return(value);
        }
#else
        /*"*********************查询相应寄存器地址************************"*/
        if (group_no <= 3U) /*" Group0 Group1 Group2 Group3"*/
        {
            pin_addr = (uint32_t)(&GPIO->PA);
        }
        else if (group_no <= 7U) /*" Group4 Group5 Group6 Group7"*/
        {
            pin_addr = (uint32_t)(&GPIO->PB);
        }
        else if (group_no <= 11U) /*" Group8 Group9 Group10 Group11"*/
        {
            pin_addr = (uint32_t)(&GPIO->PC);
        }
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3)
        else if (group_no <= 14U) /*" Group12 Group13 Group14 "*/
        {
            pin_addr = (uint32_t)(&GPIO->PD);
        }
#endif
        else
        {
            return (0U);
        }

        /*" 返回IO口状态"*/
        value = (uint8_t)BITBAND_ADDR(pin_addr, ((pin_id & 0x07U) + (group_no & 0x3U) * 8U));
        return (value);
#endif
    }

    return (0U);
}
/*****************************************************************************
** Function name:LL_GPIO_OverturnPin
** Description:　设置IO口翻转
** Parameters: pin_id:端口号
** Returned value:
******************************************************************************/
void LL_GPIO_OverturnPin(uint8_t pin_id)
{
    uint8_t Val_Over;
    Val_Over = LL_GPIO_ReadPin(pin_id); /*"得到相应引脚的电平"*/
    Val_Over ^= 1U;
    LL_GPIO_SetPin(pin_id, Val_Over); /*"输出相反电平"*/
    return;
}

/*****************************************************************************
** Function name:LL_GPIO_CfgPullMode
** Description:　设置IO口上拉的打开及关闭
** Parameters: pin_id:端口号 mode:Pull_ON 打开  Pull_OFF 关闭
** Returned value:NONE
******************************************************************************/
void LL_GPIO_CfgPullMode(uint8_t pin_id, eGPIOPull_TypeDef mode)
{
    uint32_t pin_addr;
    uint8_t group_no;

    if (pin_id < PIN_nMAX)
    {
        group_no = (pin_id >> 3); /*"获取IO口组别号"*/
#ifdef RN202x_RN7326_SOC_V2
        if((PIN_4_0 == pin_id)||(PIN_4_1 == pin_id)||(PIN_4_2 == pin_id)||(PIN_4_3 == pin_id)
            ||(PIN_5_0 == pin_id)||(PIN_5_1 == pin_id)||(PIN_5_2 == pin_id)||(PIN_5_3 == pin_id))
        {
            GPIO_PSW1_EnableWrite;
            if(Pull_ON == mode)
            {
                GPIO_APB->PUB0 |= (0x00000001 << (pin_id - PIN_4_0));  /* 设置IO口的上拉 */
            }
            else
            {
                GPIO_APB->PUB0 &= ~(0x00000001 << (pin_id - PIN_4_0));  /* 关闭IO口的上拉 */
            }
            GPIO_PB0MASK_Enable;
            GPIO_PSW1_DisableWrite;
        }
        else
        {
            if( group_no <= 3U) /*" Group0 Group1 Group2 Group3"*/
            {
                pin_addr = (uint32_t )(&GPIO->PUA);
            }
            else if(group_no == 4U) /*" Group4 "*/
            {
                pin_addr = (uint32_t)(&GPIO->PUB1);
            }
            else if((group_no == 6U) ||(group_no == 7U))/*" Group6 Group7"*/
            {
                pin_addr = (uint32_t)(&GPIO->PUB1);
                group_no--;
            }
            else if(group_no <= 9U)/*" Group8 Group9"*/
            {
                pin_addr = (uint32_t)(&GPIO->PUC);
            }
            else
            {
                return;
            }
            GPIO_PSW0_EnableWrite;
            // 设置IO口上拉寄存器
            BITBAND_ADDR(pin_addr,(pin_id &0x07)+(group_no%4)*8) = ((mode == Pull_ON)?1:0);
            GPIO_PSW0_DisableWrite;
        }
#else
        /*"*********************查询相应寄存器地址************************"*/
        if (group_no <= 3U) /*" Group0 Group1 Group2 Group3"*/
        {
            pin_addr = (uint32_t)(&GPIO->PUA);
        }
        else if (group_no <= 7U) /*" Group4 Group5 Group6 Group7"*/
        {
            pin_addr = (uint32_t)(&GPIO->PUB);
        }
        else if (group_no <= 11U) /*" Group8 Group9 Group10 Group11"*/
        {
            pin_addr = (uint32_t)(&GPIO->PUC);
        }
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3)
        else if (group_no <= 14U) /*" Group12 Group13 Group14"*/
        {
            pin_addr = (uint32_t)(&GPIO->PUD);
        }
#endif
        else
        {
            return;
        }
        #if defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3) || defined(RN202x_RN7326_SOC_V2)
        #if defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V3)
        if(pin_id == PIN_14_1)/*"MCU_V2,SOC_V3的P141上拉配置是反的"*/
        #elif defined(RN831x_RN861x_MCU_V3) || defined(RN821x_RN721x_SOC_V2)
        if(pin_id == PIN_14_0)/*"MCU_V3,SOC_V2的P140上拉配置是反的"*/
        #elif defined(RN202x_RN7326_SOC_V2)
        if(pin_id == PIN_7_4)/*"P74上拉配置是反的"*/
        #endif
        {
            if(Pull_ON == mode)
            {
                mode = Pull_OFF;
            }
            else
            {
                mode = Pull_ON;
            }
        }
        #endif

        /*" 设置IO口上拉寄存器"*/
        BITBAND_ADDR(pin_addr, ((pin_id & 0x07U) + (group_no % 4U) * 8U)) = (uint32_t)mode;

#endif
    }
    return;
}
/*****************************************************************************
** Function name:LL_GPIO_CfgInputMode
** Description:　设置IO口输入模式
** Parameters: pin_id:端口号
**   mode:输入输出口状态，可选择为TTL_MODE,COMS_MODE
** Returned value:NONE
******************************************************************************/
void LL_GPIO_CfgInputMode(uint8_t pin_id, eGPIOInputMode_TypeDef mode)
{
    uint32_t pin_addr3;
    uint8_t group_no;
    uint8_t pil_cnt = 0xffU;
    if (pin_id < PIN_nMAX)
    {
        group_no = (pin_id >> 3); /* "获取IO口组别号" */
#ifdef RN202x_RN7326_SOC_V2
        if ((PIN_4_0 == pin_id) || (PIN_4_1 == pin_id) || (PIN_4_2 == pin_id) || (PIN_4_3 == pin_id) || (PIN_5_0 == pin_id) || (PIN_5_1 == pin_id) || (PIN_5_2 == pin_id) || (PIN_5_3 == pin_id))
        {
            GPIO_PSW1_EnableWrite;
            if (mode == TTL_MODE)
            {
                GPIO_APB->PIMB0 |= (0x00000010 << (pin_id - PIN_4_0));
            }
            else
            {
                GPIO_APB->PIMB0 &= ~(0x00000010 << (pin_id - PIN_4_0));
            }
            GPIO_PB0MASK_Enable;
            GPIO_PSW1_DisableWrite;
        }
        else
        {
            if (group_no <= 3U) /*" Group0 Group1 Group2 Group3"*/
            {
                pin_addr3 = (uint32_t)(&GPIO->PIMA1);
                pil_cnt = (pin_id & 0x07) + group_no * 8;
            }
            else if (group_no == 4) /*" Group4"*/
            {
                pin_addr3 = (uint32_t)(&GPIO->PIMB1);
                pil_cnt = (pin_id & 0x07) + 16;
            }
            else if (group_no == 6) /*" Group6"*/
            {
                pin_addr3 = (uint32_t)(&GPIO->PIMB1);
                pil_cnt = (pin_id & 0x07) + 24;
            }
            else if (group_no == 7) /*" Group7"*/
            {
                pin_addr3 = (uint32_t)(&GPIO->PIMB2);
                pil_cnt = (pin_id & 0x07) + 16;
            }
            if (group_no <= 9U) /*" Group8 Group9 "*/
            {
                pin_addr3 = (uint32_t)(&GPIO->PIMC1);
                pil_cnt = pin_id - PIN_8_0;
            }
            else
            {
                return;
            }
            GPIO_PSW0_EnableWrite;
            if (mode == TTL_MODE)
            {
                BITBAND_ADDR(pin_addr3, pil_cnt) = 1;
            }
            else
            {
                BITBAND_ADDR(pin_addr3, pil_cnt) = 0;
            }
            GPIO_PSW0_DisableWrite;
        }
#else
        /*"*********************查询相应寄存器地址************************"*/
        if (group_no <= 3U) /*" Group0 Group1 Group2 Group3"*/
        {
            pin_addr3 = (uint32_t)(&GPIO->PIMA);
            if (group_no == 1U)
            {
                pil_cnt = (pin_id & 0x07U) + 16U;
            }
            else if (group_no == 2U)
            {
                pil_cnt = (pin_id & 0x07U) + 24U;
            }
            else
            {
                pil_cnt = 0xffU;
            }
        }
        else if (group_no <= 7U) /*" Group4 Group5 Group6 Group7"*/
        {
            pin_addr3 = (uint32_t)(&GPIO->PIMB);
            if (group_no == 4U)
            {
                pil_cnt = (pin_id & 0x07U) + 16U;
            }
            else if (group_no == 5U)
            {
                pil_cnt = (pin_id & 0x07U) + 24U;
            }
            else
            {
                pil_cnt = 0xffU;
            }
        }
        else if (group_no <= 11U) /*" Group8 Group9 Group10 Group11"*/
        {
            pin_addr3 = (uint32_t)(&GPIO->PIMC);
            if ((pin_id == PIN_11_0) || (pin_id == PIN_11_1) || (pin_id == PIN_11_2) || (pin_id == PIN_11_3))
            {
                pil_cnt = (pin_id & 0x07U) + 4U;
            }
            else if ((pin_id == PIN_11_4) || (pin_id == PIN_11_5) || (pin_id == PIN_11_6) || (pin_id == PIN_11_7))
            {
                pil_cnt = (pin_id & 0x07U) + 8U;
            }
            else
            {
                pil_cnt = 0xffU;
            }
        }
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3)
        else if (group_no <= 14U) /*" Group12 Group13 Group14"*/
        {
            pin_addr3 = (uint32_t)(&GPIO->PIMD);
            if ((pin_id == PIN_14_0) || (pin_id == PIN_14_1) || (pin_id == PIN_14_2) || (pin_id == PIN_14_3))
            {
                pil_cnt = (pin_id & 0x07U) + 4U;
            }
            else if ((pin_id == PIN_14_4) || (pin_id == PIN_14_5) || (pin_id == PIN_14_6) || (pin_id == PIN_14_7))
            {
                pil_cnt = (pin_id & 0x07U) + 8U;
            }
            else
            {
                pil_cnt = 0xffU;
            }
        }
#endif
        else
        {
            return;
        }
        /*"*********************配置相应寄存器************************"*/

        if ((mode == TTL_MODE) && (pil_cnt != 0xffU))
        {
            BITBAND_ADDR(pin_addr3, pil_cnt) = 1U;
        }
        else if ((mode == COMS_MODE) && (pil_cnt != 0xffU))
        {
            BITBAND_ADDR(pin_addr3, pil_cnt) = 0U;
        }
        else
        {
            /*lint !e438*/
        }
#endif
    }

    return;
}
/*****************************************************************************
** Function name:GPIO_Config_IOMode
** Description:　设置IO口模式
** Parameters: pin_id:端口号
**   dir:GPIO_MODE_IN 输入模式  GPIO_MODE_OUT 输出模式 GPIO_MODE_NORMAL 常态模式
**   mode:输入输出口状态，可选择为TTL_IN,NORMAL_IN输入，
**        输出可配置为NORMAL_OUT、N_OPENDRAIN_OUT
** Returned value:NONE
******************************************************************************/
void LL_GPIO_CfgOutputMode(uint8_t pin_id, eGPIOOutputMode_TypeDef mode)
{
    uint32_t pin_addr3;
    uint8_t group_no;
    uint8_t pid_cnt = 0xffU;
    if (pin_id < PIN_nMAX)
    {
        group_no = (pin_id >> 3); /*"获取IO口组别号"*/
#ifdef RN202x_RN7326_SOC_V2
        if ((PIN_4_0 == pin_id) || (PIN_4_1 == pin_id) || (PIN_4_2 == pin_id) || (PIN_4_3 == pin_id) || (PIN_5_0 == pin_id) || (PIN_5_1 == pin_id) || (PIN_5_2 == pin_id) || (PIN_5_3 == pin_id))
        {
            GPIO_PSW1_EnableWrite;
            /* 设置普通输出或开漏输出模式 */
            if (mode == N_ch_MODE)
            {
                GPIO_APB->PIMB0 |= (0x00000001 << (pin_id - PIN_4_0));
            }
            else
            {
                GPIO_APB->PIMB0 &= ~(0x00000001 << (pin_id - PIN_4_0));
            }
            GPIO_PB0MASK_Enable;
            GPIO_PSW1_DisableWrite;
        }
        else
        {
            if (group_no <= 3U) /*" Group0 Group1 Group2 Group3"*/
            {
                pin_addr3 = (uint32_t)(&GPIO->PIMA0);
                pid_cnt = pin_id - PIN_0_0;
            }
            else if (group_no == 4U) /*" Group4"*/
            {
                pin_addr3 = (uint32_t)(&GPIO->PIMB1);
                pid_cnt = pin_id - PIN_4_0;
            }
            else if (group_no == 6U) /*" Group6"*/
            {
                pin_addr3 = (uint32_t)(&GPIO->PIMB1);
                pid_cnt = (pin_id & 0x07) + 8;
            }
            else if (group_no == 7U) /*" Group7"*/
            {
                pin_addr3 = (uint32_t)(&GPIO->PIMB2);
                pid_cnt = pin_id - PIN_7_0;
            }
            else if (group_no <= 9U) /*" Group8 Group9 "*/
            {
                pin_addr3 = (uint32_t)(&GPIO->PIMC0);
                pid_cnt = pin_id - PIN_8_0;
            }
            else
            {
                return;
            }
            GPIO_PSW0_EnableWrite;
            if (mode == N_ch_MODE)
            {
                BITBAND_ADDR(pin_addr3, pid_cnt) = 1;
            }
            else
            {
                BITBAND_ADDR(pin_addr3, pid_cnt) = 0;
            }
            GPIO_PSW0_DisableWrite;
        }
#else
        /*"*********************查询相应寄存器地址************************"*/
        if (group_no <= 3U) /*" Group0 Group1 Group2 Group3"*/
        {
            pin_addr3 = (uint32_t)(&GPIO->PIMA);
            if (group_no == 1U)
            {
                pid_cnt = (pin_id & 0x07U);
            }
            else if (group_no == 2U)
            {
                pid_cnt = (pin_id & 0x07U) + 8U;
            }
            else
            {
                pid_cnt = 0xffU;
            }
        }
        else if (group_no <= 7U) /*" Group4 Group5 Group6 Group7"*/
        {
            pin_addr3 = (uint32_t)(&GPIO->PIMB);
            if (group_no == 4U)
            {
                pid_cnt = (pin_id & 0x07U);
            }
            else if (group_no == 5U)
            {
                pid_cnt = (pin_id & 0x07U) + 8U;
            }
            else
            {
                pid_cnt = 0xffU;
            }
        }
        else if (group_no <= 11U) /*" Group8 Group9 Group10 Group11"*/
        {
            pin_addr3 = (uint32_t)(&GPIO->PIMC);
            if ((pin_id == PIN_11_0) || (pin_id == PIN_11_1) || (pin_id == PIN_11_2) || (pin_id == PIN_11_3))
            {
                pid_cnt = (pin_id & 0x07U);
            }
            else if ((pin_id == PIN_11_4) || (pin_id == PIN_11_5) || (pin_id == PIN_11_6) || (pin_id == PIN_11_7))
            {
                pid_cnt = (pin_id & 0x07U) + 4U;
            }
            else
            {
                pid_cnt = 0xffU;
            }
        }
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3)
        else if (group_no <= 14U) /*" Group12 Group13 Group14"*/
        {
            pin_addr3 = (uint32_t)(&GPIO->PIMD);
            if ((pin_id == PIN_14_0) || (pin_id == PIN_14_1) || (pin_id == PIN_14_2) || (pin_id == PIN_14_3))
            {
                pid_cnt = (pin_id & 0x07U);
            }
            else if ((pin_id == PIN_14_4) || (pin_id == PIN_14_5) || (pin_id == PIN_14_6) || (pin_id == PIN_14_7))
            {
                pid_cnt = (pin_id & 0x07U) + 4U;
            }
            else
            {
                pid_cnt = 0xffU;
            }
        }
#endif
        else
        {
            return;
        }
        /*"*********************配置相应寄存器************************"*/
        if ((mode == N_ch_MODE) && (pid_cnt != 0xffU))
        {
            BITBAND_ADDR(pin_addr3, pid_cnt) = 1U;
        }
        else if ((mode == PushPll_MODE) && (pid_cnt != 0xffU))
        {
            BITBAND_ADDR(pin_addr3, pid_cnt) = 0U;
        }
        else
        {
            return;
        }
#endif
    }

    return;
}

/*****************************************************************************
** Function name:GPIO_Config_IOMode
** Description:　设置IO口模式
** Parameters: pin_id:端口号
**   dir:GPIO_MODE_IN 输入模式  GPIO_MODE_OUT 输出模式 GPIO_MODE_NORMAL 常态模式
** Returned value:NONE
******************************************************************************/
void LL_GPIO_CfgDirMode(uint8_t pin_id, eGPIODir_TypeDef dir)
{
    uint32_t pin_addr1, pin_addr2;
    uint8_t group_no;
#ifdef RN202x_RN7326_SOC_V2
    uint8_t pid_cnt = 0xffU;
#endif
    if (pin_id < PIN_nMAX)
    {
        group_no = (pin_id >> 3); /*"获取IO口组别号"*/
#ifdef RN202x_RN7326_SOC_V2
        if ((PIN_4_0 == pin_id) || (PIN_4_1 == pin_id) || (PIN_4_2 == pin_id) || (PIN_4_3 == pin_id) || (PIN_5_0 == pin_id) || (PIN_5_1 == pin_id) || (PIN_5_2 == pin_id) || (PIN_5_3 == pin_id))
        {
            GPIO_PSW1_EnableWrite;
            /* 设置普通输出或开漏输出模式 */
            if (GPIO_MODE_OUT == dir)
            {
                GPIO_APB->PMB0 &= ~(0x00000001 << (pin_id - PIN_4_0));
                GPIO_APB->PIEB0 |= (0x00000001 << (pin_id - PIN_4_0));
            }
            else if(GPIO_MODE_IN == dir)
            {
                GPIO_APB->PMB0 |= (0x00000001 << (pin_id - PIN_4_0));
                GPIO_APB->PIEB0 &= ~(0x00000001 << (pin_id - PIN_4_0));
            }
            else
            {
                GPIO_APB->PMB0 |= (0x00000001 << (pin_id - PIN_4_0));
                GPIO_APB->PIEB0 |= (0x00000001 << (pin_id - PIN_4_0));
            }
            GPIO_PB0MASK_Enable;
            GPIO_PSW1_DisableWrite;
        }
        else
        {
            /*"*********************查询相应寄存器地址************************"*/
            if (group_no <= 3U) /*" Group0 Group1 Group2 Group3"*/
            {
                pin_addr1 = (uint32_t)(&GPIO->PMA);
                pin_addr2 = (uint32_t)(&GPIO->PIEA);
                pid_cnt = pin_id - PIN_0_0;
            }
            else if (group_no == 4U) /*" Group4"*/
            {
                pin_addr1 = (uint32_t)(&GPIO->PMB1);
                pin_addr2 = (uint32_t)(&GPIO->PIEB1);
                pid_cnt = pin_id - PIN_4_0;
            }
            else if ((group_no == 6U)||(group_no == 7U)) /*" Group6,Group7"*/
            {
                pin_addr1 = (uint32_t)(&GPIO->PMB1);
                pin_addr2 = (uint32_t)(&GPIO->PIEB1);
                pid_cnt = pin_id - PIN_6_0 + 8;
            }
            else if (group_no <= 9U) /*" Group8 Group9 "*/
            {
                pin_addr1 = (uint32_t)(&GPIO->PMC);
                pin_addr2 = (uint32_t)(&GPIO->PIEC);
                pid_cnt = pin_id - PIN_8_0;
            }
            else
            {
                return;
            }
            GPIO_PSW0_EnableWrite;
            if (GPIO_MODE_OUT == dir)   /*" 输出模式"*/
            {
                BITBAND_ADDR(pin_addr1, pid_cnt) = 0U; /*"GPIO->PM模式设置为输出"*/
            }
            else if(GPIO_MODE_IN == dir)    /*" 输入模式"*/
            {
                BITBAND_ADDR(pin_addr1, pid_cnt) = 1U; /*" GPIO->PM模式设置为输入"*/
                BITBAND_ADDR(pin_addr2, pid_cnt) = 0U; /*" 打开输入使能"*/
            }
            else /*" 正常模式，掉电不用的IO口配置为此模式"*/
            {
                BITBAND_ADDR(pin_addr1, pid_cnt) = 1U; /*" GPIO->PM模式设置为输入"*/
                BITBAND_ADDR(pin_addr2, pid_cnt) = 1U;
            }
            GPIO_PSW0_DisableWrite;
        }
#else
        /*"*********************查询相应寄存器地址************************"*/
        if (group_no <= 3U) /*" Group0 Group1 Group2 Group3"*/
        {
            pin_addr1 = (uint32_t)(&GPIO->PMA);
            pin_addr2 = (uint32_t)(&GPIO->PIEA);
        }
        else if (group_no <= 7U) /*" Group4 Group5 Group6 Group7"*/
        {
            pin_addr1 = (uint32_t)(&GPIO->PMB);
            pin_addr2 = (uint32_t)(&GPIO->PIEB);
        }
        else if (group_no <= 11U) /*" Group8 Group9 Group10 Group11"*/
        {
            pin_addr1 = (uint32_t)(&GPIO->PMC);
            pin_addr2 = (uint32_t)(&GPIO->PIEC);
        }
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3)
        else if (group_no <= 14U) /*" Group12 Group13 Group14"*/
        {
            pin_addr1 = (uint32_t)(&GPIO->PMD);
            pin_addr2 = (uint32_t)(&GPIO->PIED);
        }
#endif
        else
        {
            return;
        }
        /*"*********************配置相应寄存器************************"*/
        switch (dir)
        {
        case GPIO_MODE_IN:     /*" 输入模式"*/
        case GPIO_MODE_NORMAL: /*" 正常模式，掉电不用的IO口配置为此模式"*/
            {
                BITBAND_ADDR(pin_addr1, ((pin_id & 0x07U) + (group_no % 4U) * 8U)) = 1U; /*" GPIO->PM模式设置为输入"*/
                if (dir == GPIO_MODE_IN)
                {
                    BITBAND_ADDR(pin_addr2, ((pin_id & 0x07U) + (group_no % 4U) * 8U)) = 0U; /*" 打开输入使能"*/
                }
                else
                {
                    BITBAND_ADDR(pin_addr2, ((pin_id & 0x07U) + (group_no % 4U) * 8U)) = 1U;
                }
            }
            break;
        case GPIO_MODE_OUT: /*" 输出模式"*/
            {
                BITBAND_ADDR(pin_addr1, ((pin_id & 0x07U) + (group_no % 4U) * 8U)) = 0U; /*"GPIO->PM模式设置为输出"*/
            }
            break;
        default:
            break;
        }

#endif
    }
    return;
}
/*****************************************************************************
** Function name:GPIO_Config_IOMode
** Description:　设置IO口模式
** Parameters: pin_id:端口号
**   dir:GPIO_MODE_IN 输入模式  GPIO_MODE_OUT 输出模式 GPIO_MODE_NORMAL 常态模式
**   mode:输入输出口状态，可选择为TTL_IN,NORMAL_IN输入，
**        输出可配置为NORMAL_OUT、N_OPENDRAIN_OUT
** Returned value:NONE
******************************************************************************/
void LL_GPIO_CfgIOMode(uint8_t pin_id, eGPIODir_TypeDef dir, eGPIOMode_TypeDef mode)
{
    uint32_t pin_addr1, pin_addr2, pin_addr3;
    uint8_t group_no;
    uint8_t pil_cnt = 0xffU, pid_cnt = 0xffU;
    #ifdef RN202x_RN7326_SOC_V2
    uint32_t pin_addr4;
    #endif
    if (pin_id >= PIN_nMAX)
    {
        return;
    }
    group_no = (pin_id >> 3); /*"获取IO口组别号"*/
#ifdef RN202x_RN7326_SOC_V2
        if ((PIN_4_0 == pin_id) || (PIN_4_1 == pin_id) || (PIN_4_2 == pin_id) || (PIN_4_3 == pin_id) || (PIN_5_0 == pin_id) || (PIN_5_1 == pin_id) || (PIN_5_2 == pin_id) || (PIN_5_3 == pin_id))
        {
            GPIO_PSW1_EnableWrite;
            /* 设置普通输出或开漏输出模式 */
            if (GPIO_MODE_OUT == dir)
            {
                GPIO_APB->PMB0 &= ~(0x00000001 << (pin_id - PIN_4_0));
                GPIO_APB->PIEB0 |= (0x00000001 << (pin_id - PIN_4_0));
                if(N_OPENDRAIN_OUT == mode) /*"N通道漏极开路模式输出"*/
                {
                    GPIO_APB->PIMB0 |= (0x00000001 << (pin_id - PIN_4_0));/*""*/
                }
                else/*"普通模式输出,推挽输出"*/
                {
                    GPIO_APB->PIMB0  &= ~(0x00000001 << (pin_id - PIN_4_0));
                }
            }
            else
            {
                GPIO_APB->PMB0 |= (0x00000001 << (pin_id - PIN_4_0));
                if(GPIO_MODE_IN == dir)
                {
                    GPIO_APB->PIEB0 &= ~(0x00000001 << (pin_id - PIN_4_0));
                }
                else/*"输入不使能;不使用的IO口配置为此模式"*/
                {
                    GPIO_APB->PIEB0 |= (0x00000001 << (pin_id - PIN_4_0));
                }
                if(TTL_IN == mode) /*""TTL输入缓冲器类型"*/
                {
                    GPIO_APB->PIMB0 |= (0x00000001 << (pin_id - PIN_4_0 + 4));/*""*/
                }
                else/*""普通模式输入，即CMOS输入缓冲器类型"*/
                {
                    GPIO_APB->PIMB0  &= ~(0x00000001 << (pin_id - PIN_4_0 + 4));
                }
            }
            GPIO_PB0MASK_Enable;
            GPIO_PSW1_DisableWrite;
        }
        else
        {
            /*"*********************查询相应寄存器地址************************"*/
            if (group_no <= 3U) /*" Group0 Group1 Group2 Group3"*/
            {
                pin_addr1 = (uint32_t)(&GPIO->PMA);
                pin_addr2 = (uint32_t)(&GPIO->PIEA);
                pin_addr3 = (uint32_t)(&GPIO->PIMA0);
                pin_addr4 = (uint32_t)(&GPIO->PIMA1);
                pid_cnt = pin_id - PIN_0_0;
                pil_cnt = pin_id - PIN_0_0;
            }
            else if (group_no == 4U) /*" Group4"*/
            {
                pin_addr1 = (uint32_t)(&GPIO->PMB1);
                pin_addr2 = (uint32_t)(&GPIO->PIEB1);
                pin_addr3 = (uint32_t)(&GPIO->PIMB1);
                pin_addr4 = (uint32_t)(&GPIO->PIMB1);
                pid_cnt = pin_id - PIN_4_0;
                if(GPIO_MODE_OUT == dir)
                {
                    pil_cnt = pin_id - PIN_4_0;
                }
                else
                {
                    pil_cnt = pin_id - PIN_4_0 + 16;
                }
            }
            else if (group_no == 6U) /*" Group6"*/
            {
                pin_addr1 = (uint32_t)(&GPIO->PMB1);
                pin_addr2 = (uint32_t)(&GPIO->PIEB1);
                pin_addr3 = (uint32_t)(&GPIO->PIMB1);
                pin_addr4 = (uint32_t)(&GPIO->PIMB1);
                pid_cnt = pin_id - PIN_6_0 + 8;
                if(GPIO_MODE_OUT == dir)
                {
                    pil_cnt = pin_id - PIN_6_0 + 8;
                }
                else
                {
                    pil_cnt = pin_id - PIN_6_0 + 24;
                }

            }
            else if (group_no == 7U) /*" Group7"*/
            {
                pin_addr1 = (uint32_t)(&GPIO->PMB1);
                pin_addr2 = (uint32_t)(&GPIO->PIEB1);
                pin_addr3 = (uint32_t)(&GPIO->PIMB2);
                pin_addr4 = (uint32_t)(&GPIO->PIMB2);
                pid_cnt = pin_id - PIN_6_0 + 8;
                if(GPIO_MODE_OUT == dir)
                {
                    pil_cnt = pin_id - PIN_7_0;
                }
                else
                {
                    pil_cnt = pin_id - PIN_7_0 + 16;
                }
            }
            else if (group_no <= 9U) /*" Group8 Group9 "*/
            {
                pin_addr1 = (uint32_t)(&GPIO->PMC);
                pin_addr2 = (uint32_t)(&GPIO->PIEC);
                pin_addr3 = (uint32_t)(&GPIO->PIMC0);
                pin_addr4 = (uint32_t)(&GPIO->PIMC1);
                pid_cnt = pin_id - PIN_8_0;
                pil_cnt = pin_id - PIN_8_0;
            }
            else
            {
                return;
            }

            GPIO_PSW0_EnableWrite;
            if (GPIO_MODE_OUT == dir)   /*" 输出模式"*/
            {
                BITBAND_ADDR(pin_addr1, pid_cnt) = 0U; /*"GPIO->PM模式设置为输出"*/
                if(N_OPENDRAIN_OUT == mode) /*"N通道漏极开路模式输出"*/
                {
                    BITBAND_ADDR(pin_addr3, pil_cnt) = 1U;
                }
                else
                {
                    BITBAND_ADDR(pin_addr3, pil_cnt) = 0U;
                }
            }
            else   /*" 输入模式"*/
            {
                BITBAND_ADDR(pin_addr1, pid_cnt) = 1U; /*" GPIO->PM模式设置为输入"*/
                if(GPIO_MODE_IN == dir)
                {
                    BITBAND_ADDR(pin_addr2, pid_cnt) = 0U; /*" 打开输入使能"*/
                }
                else/*" 正常模式，掉电不用的IO口配置为此模式"*/
                {
                    BITBAND_ADDR(pin_addr2, pid_cnt) = 1U;
                }

                if(TTL_IN == mode) /*""TTL输入缓冲器类型"*/
                {
                    BITBAND_ADDR(pin_addr4, pil_cnt) = 1U;
                }
                else
                {
                    BITBAND_ADDR(pin_addr4, pil_cnt) = 0U;
                }
            }

            GPIO_PSW0_DisableWrite;
        }
#else
    /*"*********************查询相应寄存器地址************************"*/
    if (group_no <= 3U) /*" Group0 Group1 Group2 Group3"*/
    {
        pin_addr1 = (uint32_t)(&GPIO->PMA);
        pin_addr2 = (uint32_t)(&GPIO->PIEA);
        pin_addr3 = (uint32_t)(&GPIO->PIMA);
        if (group_no == 1U)
        {
            pil_cnt = (pin_id & 0x07U) + 16U;
            pid_cnt = (pin_id & 0x07U);
        }
        else if (group_no == 2U)
        {
            pil_cnt = (pin_id & 0x07U) + 24U;
            pid_cnt = (pin_id & 0x07U) + 8U;
        }
        else
        {
            pil_cnt = 0xffU;
            pid_cnt = 0xffU;
        }
    }
    else if (group_no <= 7U) /*" Group4 Group5 Group6 Group7"*/
    {
        pin_addr1 = (uint32_t)(&GPIO->PMB);
        pin_addr2 = (uint32_t)(&GPIO->PIEB);
        pin_addr3 = (uint32_t)(&GPIO->PIMB);
        if (group_no == 4U)
        {
            pil_cnt = (pin_id & 0x07U) + 16U;
            pid_cnt = (pin_id & 0x07U);
        }
        else if (group_no == 5U)
        {
            pil_cnt = (pin_id & 0x07U) + 24U;
            pid_cnt = (pin_id & 0x07U) + 8U;
        }
        else
        {
            pil_cnt = 0xffU;
            pid_cnt = 0xffU;
        }
    }
    else if (group_no <= 11U) /*" Group8 Group9 Group10 Group11"*/
    {
        pin_addr1 = (uint32_t)(&GPIO->PMC);
        pin_addr2 = (uint32_t)(&GPIO->PIEC);
        pin_addr3 = (uint32_t)(&GPIO->PIMC);
        if ((pin_id == PIN_11_0) || (pin_id == PIN_11_1) || (pin_id == PIN_11_2) || (pin_id == PIN_11_3))
        {
            pil_cnt = (pin_id & 0x07U) + 4U;
            pid_cnt = (pin_id & 0x07U);
        }
        else if ((pin_id == PIN_11_4) || (pin_id == PIN_11_5) || (pin_id == PIN_11_6) || (pin_id == PIN_11_7))
        {
            pil_cnt = (pin_id & 0x07U) + 8U;
            pid_cnt = (pin_id & 0x07U) + 4U;
        }
        else
        {
            pil_cnt = 0xffU;
            pid_cnt = 0xffU;
        }
    }
    #if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3)
    else if (group_no <= 14U) /*" Group12 Group13 Group14"*/
    {
        pin_addr1 = (uint32_t)(&GPIO->PMD);
        pin_addr2 = (uint32_t)(&GPIO->PIED);
        pin_addr3 = (uint32_t)(&GPIO->PIMD);
        if ((pin_id == PIN_14_0) || (pin_id == PIN_14_1) || (pin_id == PIN_14_2) || (pin_id == PIN_14_3))
        {
            pil_cnt = (pin_id & 0x07U) + 4U;
            pid_cnt = (pin_id & 0x07U);
        }
        else if ((pin_id == PIN_14_4) || (pin_id == PIN_14_5) || (pin_id == PIN_14_6) || (pin_id == PIN_14_7))
        {
            pil_cnt = (pin_id & 0x07U) + 8U;
            pid_cnt = (pin_id & 0x07U) + 4U;
        }
        else
        {
            pil_cnt = 0xffU;
            pid_cnt = 0xffU;
        }
    }
    #endif
    else
    {
        return;
    }
    /*"*********************配置相应寄存器************************"*/
    switch (dir)
    {
    case GPIO_MODE_IN:     /*" 输入模式"*/
    case GPIO_MODE_NORMAL: /*" 正常模式，掉电不用的IO口配置为此模式"*/
        {
            BITBAND_ADDR(pin_addr1, ((pin_id & 0x07U) + (group_no % 4) * 8U)) = 1U; /*" GPIO->PM模式设置为输入"*/
            if (dir == GPIO_MODE_IN)
            {
                BITBAND_ADDR(pin_addr2, ((pin_id & 0x07U) + (group_no % 4U) * 8U)) = 0U; /*" 打开输入使能"*/
            }
            else
            {
                BITBAND_ADDR(pin_addr2, ((pin_id & 0x07U) + (group_no % 4U) * 8U)) = 1U;
            }

            if ((mode == TTL_IN) && (pil_cnt != 0xffU))
            {
                BITBAND_ADDR(pin_addr3, pil_cnt) = 1U;
            }
            else if ((mode == NORMAL_IN) && (pil_cnt != 0xffU))
            {
                BITBAND_ADDR(pin_addr3, pil_cnt) = 0U;
            }
            else
            {
                /*lint !e9063*/
            }
        }
        break;
    case GPIO_MODE_OUT: /*" 输出模式"*/
        {
            BITBAND_ADDR(pin_addr1, ((pin_id & 0x07U) + (group_no % 4U) * 8U)) = 0U; /*"GPIO->PM模式设置为输出"*/

            if ((mode == N_OPENDRAIN_OUT) && (pid_cnt != 0xffU))
            {
                BITBAND_ADDR(pin_addr3, pid_cnt) = 1U;
            }
            else if ((mode == NORMAL_OUT) && (pid_cnt != 0xffU))
            {
                BITBAND_ADDR(pin_addr3, pid_cnt) = 0U;
            }
            else
            {
                /*lint !e9063*/
            }
        }
        break;
    default:
        break;
    }
    return;
#endif
}

/*****************************************************************************
** Function name:   LL_GPIO_CfgFun
** Description:     设置IO口功能，如此口无对应功能，退出
**				注意:有部分IO有关联性，配置后会改变其他IO属性
** Parameters:  pin_id:端口号
**              Fun:IO口对应功能，所有功能使用eGPIOFunction_TypeDef枚举
** Returned value:NONE
******************************************************************************/
ErrorStatus LL_GPIO_CfgFun(uint8_t pin_id, eGPIOFunction_TypeDef Fun)
{
    uint16_t i, pbit;
    ErrorStatus status;
    uint32_t RegAd, FunLen, RegLen;
    eGPIOFunRegID_TypeDef RegID;
    eGPIOBitNumber_TypeDef starNum;
    uint32_t PutData;

    status = ERN_ERROR;
    #ifdef RN202x_RN7326_SOC_V2
	GPIO_PSW0_EnableWrite;
    #endif
    if (pin_id < PIN_SegCom_Group) /*"普通GPIO口复用功能配置"*/
    {
        /*"查询相应功能"*/
        if ((GPIOFunTable[pin_id].FunCell != NULL) && (GPIOFunTable[pin_id].FunRegCell != NULL))
        {
            FunLen = GPIOFunTable[pin_id].FunCellNum;
            for (i = 0U; i < FunLen; i++)
            {
                if (Fun == GPIOFunTable[pin_id].FunCell[i].GpioFun)
                {
                    RegLen = GPIOFunTable[pin_id].FunRegCellNum;
                    PutData = GPIOFunTable[pin_id].FunCell[i].PutData;
                    for (pbit = 0U; pbit < RegLen; pbit++) /*"寄存器遍历"*/
                    {
                        RegID = GPIOFunTable[pin_id].FunRegCell[pbit].RegID;
                        RegAd = (uint32_t)(GPIO_FunReg_Dict[RegID]);
                        starNum = GPIOFunTable[pin_id].FunRegCell[pbit].starNum;
                        BITBAND_ADDR(RegAd, (uint32_t)starNum) = (((PutData) >> pbit) & 0x01U);
                    }
                    status = ERN_SUCCESS;
                    break;
                }
                else
                {
                    /*lint !e9063*/
                }
            }
        }
        #ifdef RN202x_RN7326_SOC_V2
        else if((PIN_4_0 == pin_id)||(PIN_4_1 == pin_id)||(PIN_4_2 == pin_id)||(PIN_4_3 == pin_id)
                ||(PIN_5_0 == pin_id)||(PIN_5_1 == pin_id)||(PIN_5_2 == pin_id)||(PIN_5_3 == pin_id))
        {/*"VBAT域管脚配置"*/
            status = ll_gpio_AHB_CfgFun(pin_id,Fun);
        }
        #endif
        else
        {
            status = ERN_ERROR;
        }
        /*"*********************特殊配置****************************************"*/
#if defined(RN821x_RN721x_SOC_B) || \
    defined(RN821x_RN721x_SOC_C) || \
    defined(RN821x_RN721x_SOC_D) || \
    defined(RN821x_RN721x_SOC_V2) || \
    defined(RN821x_RN721x_SOC_V3) || \
    defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3)
        if (_UART2 == Fun) /*"UART2_SEL需要提前配置"*/
        {
            ll_gpio_SpclUart_CfgFun(pin_id,Fun);
        }
#endif
#if defined(RN821x_RN721x_SOC_B) || \
    defined(RN821x_RN721x_SOC_C) || \
    defined(RN821x_RN721x_SOC_D) || \
    defined(RN821x_RN721x_SOC_V2) || \
    defined(RN821x_RN721x_SOC_V3) || \
    defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3)
        if (_SPI0 == Fun)
        {
            ll_gpio_SpclSpi0_CfgFun(pin_id);
        }
#endif
#if defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3)
        if ((pin_id == PIN_4_4) || (pin_id == PIN_4_5))
        {
            ll_gpio_SpclMadc_CfgFun(pin_id, Fun);
        }
#endif
#if defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3)
        if(pin_id == PIN_5_6)
        {
            ll_gpio_SpclRtc_CfgFun(pin_id, Fun);
        }
#endif
    }
    else
    {
        status = ERN_ERROR;
    }
    #ifdef RN202x_RN7326_SOC_V2
	GPIO_PSW0_DisableWrite;
    #endif

    return status;
}

/*****************************************************************************
** Function name:   LL_GPIO_Init
** Description:     初始化功能口
**				注意:有部分IO有关联性，配置后会改变其他IO属性
** Parameters:  GPIO_Init:初始化参数
**
** Returned value:NONE
******************************************************************************/
void LL_GPIO_Init(const GPIO_InitTypeDef *GPIO_Init)
{
    (void)LL_GPIO_CfgFun(GPIO_Init->Pin, GPIO_Init->Mode);
    LL_GPIO_SetPin(GPIO_Init->Pin, (uint8_t)(GPIO_Init->OutputLevel));
    LL_GPIO_CfgDirMode(GPIO_Init->Pin, GPIO_Init->Dir);
    LL_GPIO_CfgPullMode(GPIO_Init->Pin, GPIO_Init->Pull);
    LL_GPIO_CfgInputMode(GPIO_Init->Pin, GPIO_Init->InputMode);
    LL_GPIO_CfgOutputMode(GPIO_Init->Pin, GPIO_Init->OutputMode);
}

/*****************************************************************************
** Function name:   LL_GPIO_CfgInit
** Description:     初始化功能口(功能同LL_GPIO_Init，输入参数形式不一样)
**				注意:有部分IO有关联性，配置后会改变其他IO属性
** Parameters:
**
** Returned value:NONE
******************************************************************************/
void LL_GPIO_CfgInit(uint8_t Pin,                         /*!< IO口ID */
                     eGPIOFunction_TypeDef Mode,           /*!< 复用模式 */
                     eGPIOOutputLevel_TypeDef OutputLevel, /*"输出电平选择"*/
                     eGPIOPull_TypeDef Pull,               /*!< 上拉选择 */
                     eGPIODir_TypeDef Dir,                 /*!< 输入输出选择 */
                     eGPIOInputMode_TypeDef InputMode,     /*!< 输入模式*/
                     eGPIOOutputMode_TypeDef OutputMode)   /*!< 输出模式*/
{
    (void)LL_GPIO_CfgFun(Pin, Mode);
    LL_GPIO_SetPin(Pin, (uint8_t)OutputLevel);
    LL_GPIO_CfgDirMode(Pin, Dir);
    LL_GPIO_CfgPullMode(Pin, Pull);
    LL_GPIO_CfgInputMode(Pin, InputMode);
    LL_GPIO_CfgOutputMode(Pin, OutputMode);
}
#ifdef RN202x_RN7326_SOC_V2
/*****************************************************************************
** Function name:   LL_GPIO_SPISelNoInit
** Description:     配置SPI的组合功能
** Parameters:
**
** Returned value:NONE
******************************************************************************/
void LL_GPIO_SPISelNoInit(eGPIO_SPISelNo_TypeDef gpio_spi_sel)
{
	GPIO_PSW0_EnableWrite;
    BITBAND_ADDR((uint32_t )(&GPIO->PCA1) , (22U)) = (((uint32_t)gpio_spi_sel)&(0x01U));
    BITBAND_ADDR((uint32_t )(&GPIO->PCA1) , (23U)) = ((((uint32_t)gpio_spi_sel)&(0x02U))>>1);
    BITBAND_ADDR((uint32_t )(&GPIO->PCA1) , (30U)) = ((((uint32_t)gpio_spi_sel)&(0x04U))>>2);
	GPIO_PSW0_DisableWrite;
}
#endif

#else
/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
#define IS_LL_GPIO_PIN(__VALUE__)          (((0x00000000U) < (__VALUE__)) && ((__VALUE__) <= (LL_GPIO_PIN_ALL)))

#define IS_LL_GPIO_MODE(__VALUE__)         (((__VALUE__) == LL_GPIO_MODE_INPUT)     ||\
                                            ((__VALUE__) == LL_GPIO_MODE_OUTPUT)    ||\
                                            ((__VALUE__) == LL_GPIO_MODE_ALTERNATE) ||\
                                            ((__VALUE__) == LL_GPIO_MODE_ANALOG))

#define IS_LL_GPIO_OUTPUT_TYPE(__VALUE__)  (((__VALUE__) == LL_GPIO_OUTPUT_PUSHPULL)  ||\
                                            ((__VALUE__) == LL_GPIO_OUTPUT_OPENDRAIN))

#define IS_LL_GPIO_INPUT_TYPE(__VALUE__)  (((__VALUE__) == LL_GPIO_INPUT_CMOS)  ||\
                                            ((__VALUE__) == LL_GPIO_INPUT_TTL))

#define IS_LL_GPIO_SPEED(__VALUE__)        (((__VALUE__) == LL_GPIO_SPEED_FREQ_NORMAL) ||\
                                            ((__VALUE__) == LL_GPIO_SPEED_FREQ_HIGH))


#define IS_LL_GPIO_PULL(__VALUE__)         (((__VALUE__) == LL_GPIO_PULL_NO)   ||\
                                            ((__VALUE__) == LL_GPIO_PULL_UP)   ||\
                                            ((__VALUE__) == LL_GPIO_PULL_DOWN))

#define IS_LL_GPIO_ALTERNATE(__VALUE__)    (((__VALUE__) == LL_GPIO_AF_0  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_1  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_2  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_3  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_4  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_5  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_6  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_7  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_8  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_9  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_10 )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_11 )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_12 )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_13 )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_14 )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_15 ))

#define GPIO_NUMBER           16U

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/**
 * @brief  Initializes the GPIOx peripheral according to the specified parameters in the GPIO_Init.
 * @note   PL00~PL03 are VBAT IO, configuration priority: VBAT > GPIO, if config PL00~PL03 as VBAT funciton, the GPIO
 *         configuration will be disabled.
 * @note   PL01 (VBAT power domain) IO function priority: MADC IO Function > RTC IO function > Gernal GPIO
 * @note   GPIOH only support GPIO_PIN_0 ~ GPIO_PIN_1
 *         GPIOL only support GPIO_PIN_0 ~ GPIO_PIN_3
 * @param  GPIOx where x can be (A..H, L) to select the GPIO peripheral
 * @param  GPIO_Init pointer to a GPIO_InitTypeDef structure that contains
 *         the configuration information for the specified GPIO peripheral.
 * @retval None
 */
void LL_GPIO_Init(GPIO_TypeDef  *GPIOx, GPIO_InitTypeDef *GPIO_Init)
{
    uint32_t position;
    uint32_t ioposition = 0x00U;
    uint32_t iocurrent = 0x00U;

    /* Check the parameters */
    assert_param(IS_GPIO_ALL_INSTANCE(GPIOx));
    assert_param(IS_GPIO_PIN(GPIO_Init->Pin));
    assert_param(IS_GPIO_MODE(GPIO_Init->Mode));

    /* Configure the port pins */
    for(position = 0U; position < GPIO_NUMBER; position++)
    {
        /* Get the IO position */
        ioposition = 0x01U << position;
        /* Get the current IO position */
        iocurrent = (uint32_t)(GPIO_Init->Pin) & ioposition;

        if(iocurrent == ioposition)
        {
            /*--------------------- GPIO Mode Configuration ------------------------*/
            /* In case of Output or Alternate function mode selection */
            if(((GPIO_Init->Mode) == GPIO_MODE_OUTPUT) || ((GPIO_Init->Mode) == GPIO_MODE_AF))
            {
                /* Check the Speed parameter */
                assert_param(IS_GPIO_SPEED(GPIO_Init->Speed));
                /* Configure the IO Speed */
                LL_GPIO_SetPinSpeed(GPIOx, iocurrent, GPIO_Init->Speed);
                /* Check the Speed parameter */
                assert_param(IS_GPIO_SPEED(GPIO_Init->OutputType));
                /* Configure the IO Output Type */
                LL_GPIO_SetPinOutputType(GPIOx, iocurrent, GPIO_Init->OutputType);
            }

            if((GPIO_Init->Mode) != GPIO_MODE_ANALOG)
            {
                /* Check the Speed parameter */
                assert_param(IS_GPIO_SPEED(GPIO_Init->InputType));
                /* Configure the IO Input Type */
                LL_GPIO_SetPinInputType(GPIOx, iocurrent, GPIO_Init->InputType);
            }

            if(((GPIO_Init->Mode) == GPIO_MODE_INPUT) || ((GPIO_Init->Mode) == GPIO_MODE_AF))
            {
                /* Check the parameters */
                assert_param(IS_GPIO_PULL(GPIO_Init->Pull));
                /* Activate the Pull-up or Pull down resistor for the current IO */
                LL_GPIO_SetPinPull(GPIOx, iocurrent, GPIO_Init->Pull);
            }

            /* In case of Alternate function mode selection */
            if((GPIO_Init->Mode) == GPIO_MODE_AF)
            {
                /* Check the Alternate function parameter */
                assert_param(IS_GPIO_AF(GPIO_Init->Alternate));
                if (position < 8U)
                {
                    /* Configure Alternate function mapped with the current IO */
                    LL_GPIO_SetAFPin_0_7(GPIOx, iocurrent, GPIO_Init->Alternate);
                }
                else
                {
                    LL_GPIO_SetAFPin_8_15(GPIOx, iocurrent, GPIO_Init->Alternate);
                }
            }

            /* Configure IO Direction mode */
            LL_GPIO_SetPinMode(GPIOx, iocurrent, GPIO_Init->Mode);
        }
    }
}

/**
 * @brief  De-initializes the GPIOx peripheral registers to their default reset values.
 * @param  GPIOx where x can be (A..H, L) to select the GPIO peripheral.
 * @param  GPIO_Pin specifies the port bit to be written.
 *          This parameter can be one of GPIO_PIN_x where x can be (0..15).
 * @retval None
 */
void LL_GPIO_DeInit(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin)
{
    uint32_t position;
    uint32_t ioposition = 0x0U;
    uint32_t iocurrent = 0x0U;

    /* Check the parameters */
    assert_param(IS_GPIO_ALL_INSTANCE(GPIOx));

    /* Configure the port pins */
    for(position = 0U; position < GPIO_NUMBER; position++)
    {
        /* Get the IO position */
        ioposition = 0x01U << position;
        /* Get the current IO position */
        iocurrent = (GPIO_Pin) & ioposition;

        if(iocurrent == ioposition)
        {
            /*------------------------- GPIO Mode Configuration --------------------*/
            /* Configure IO Direction in Input Floating Mode */
            LL_GPIO_SetPinMode(GPIOx, iocurrent, GPIO_MODE_ANALOG);

            /* Deactivate the Pull-up and Pull-down resistor for the current IO */
            LL_GPIO_SetPinPull(GPIOx, iocurrent, GPIO_NOPULL);

            /* Configure the default value IO Input Type */
            LL_GPIO_SetPinInputType(GPIOx, iocurrent, GPIO_INPUT_CMOS);

            /* Configure the default value IO Output Type */
            LL_GPIO_SetPinOutputType(GPIOx, iocurrent, GPIO_OUTPUT_PP);

            /* Configure the default value for IO Speed */
            LL_GPIO_SetPinSpeed(GPIOx, iocurrent, GPIO_SPEED_FREQ_NORMAL);

            /* Configure the default Alternate Function in current IO */
            if(position < 8U)
            {
                LL_GPIO_SetAFPin_0_7(GPIOx, iocurrent, GPIO_AF0_SYS);
            }
            else
            {
                LL_GPIO_SetAFPin_8_15(GPIOx, iocurrent, GPIO_AF0_SYS);
            }
        }
    }
}

/**
 * @brief  Reads the specified input port pin.
 * @param  GPIOx where x can be (A..H, L) to select the GPIO peripheral.
 * @param  GPIO_Pin specifies the port bit to read.
 *         This parameter can be GPIO_PIN_x where x can be (0..15).
 * @retval The input port pin value.
 */
GPIO_PinState LL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    GPIO_PinState bitstatus;

    /* Check the parameters */
    assert_param(IS_GPIO_PIN(GPIO_Pin));

    if((LL_GPIO_ReadInputPort(GPIOx) & GPIO_Pin) != (uint32_t)GPIO_PIN_RESET)
    {
        bitstatus = GPIO_PIN_SET;
    }
    else
    {
        bitstatus = GPIO_PIN_RESET;
    }
    return bitstatus;
}


/**
 * @brief  Sets or clears the selected data port bit.
 * @note   This function uses GPIOx_BSRR register to allow atomic read/modify
 *         accesses. In this way, there is no risk of an IRQ occurring between
 *         the read and the modify access.
 *
 * @param  GPIOx where x can be (A..H, L) to select the GPIO peripheral.
 * @param  GPIO_Pin specifies the port bit to be written.
 *          This parameter can be one of GPIO_PIN_x where x can be (0..15).
 * @param  PinState specifies the value to be written to the selected bit.
 *          This parameter can be one of the GPIO_PinState enum values:
 *            @arg GPIO_PIN_RESET: to clear the port pin
 *            @arg GPIO_PIN_SET: to set the port pin
 * @retval None
 */
void LL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
    /* Check the parameters */
    assert_param(IS_GPIO_PIN(GPIO_Pin));
    assert_param(IS_GPIO_PIN_ACTION(PinState));

    if(PinState != GPIO_PIN_RESET)
    {
        LL_GPIO_SetOutputPin(GPIOx, GPIO_Pin);
    }
    else
    {
        LL_GPIO_ResetOutputPin(GPIOx, GPIO_Pin);
    }
}

/**
 * @brief  Enable JTAG pin.
 * @param  GPIO_DBGx GPIO_DBG instance.
 * @param  JTAG_PIN Select GPIO as JTAG pin.
 *         This parameter can be a combination of the GPIO_JTAG_pin_define values:
 *            @arg GPIO_JTCK: select PG14 for JTCK
 *            @arg GPIO_JTMS: select PG15 for JTMS
 *            @arg GPIO_JTDI: select PG11 for JTDI
 *            @arg GPIO_JTDO: select PG10 for JTDO
 * @retval None
 */
void LL_GPIO_EnableJTAG(GPIO_DBG_TypeDef *GPIO_DBGx, uint32_t JTAG_PIN)
{
    /* Check the parameters */
	assert_param(IS_GPIO_JTAG_PIN(JTAG_PIN));
    /* Enable JTAG pin */
    if((JTAG_PIN & GPIO_DBG_JTCK_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_EnablePinJTCK(GPIO_DBGx);
    }
    if((JTAG_PIN & GPIO_DBG_JTMS_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_EnablePinJTMS(GPIO_DBGx);
    }
    if((JTAG_PIN & GPIO_DBG_JTDI_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_EnablePinJTDI(GPIO_DBGx);
    }
    if((JTAG_PIN & GPIO_DBG_JTDO_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_EnablePinJTDO(GPIO_DBGx);
    }
}

/**
 * @brief  Disable JTAG pin.
 * @param  GPIO_DBGx GPIO_DBG instance.
 * @param  JTAG_PIN Select GPIO as normal IO.
 *         This parameter can be a combination of the GPIO_JTAG_pin_define values:
 *            @arg GPIO_JTCK: select PG14 for JTCK
 *            @arg GPIO_JTMS: select PG15 for JTMS
 *            @arg GPIO_JTDI: select PG11 for JTDI
 *            @arg GPIO_JTDO: select PG10 for JTDO
 * @retval None
 */
void LL_GPIO_DisableJTAG(GPIO_DBG_TypeDef *GPIO_DBGx, uint32_t JTAG_PIN)
{
    /* Check the parameters */
	assert_param(IS_GPIO_JTAG_PIN(JTAG_PIN));
    /* Disable JTAG pin */
    if((JTAG_PIN & GPIO_DBG_JTCK_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_DisablePinJTCK(GPIO_DBGx);
    }
    if((JTAG_PIN & GPIO_DBG_JTMS_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_DisablePinJTMS(GPIO_DBGx);
    }
    if((JTAG_PIN & GPIO_DBG_JTDI_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_DisablePinJTDI(GPIO_DBGx);
    }
    if((JTAG_PIN & GPIO_DBG_JTDO_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_DisablePinJTDO(GPIO_DBGx);
    }
}

#endif
#endif /*LL_GPIO_MODULE_ENABLED*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
