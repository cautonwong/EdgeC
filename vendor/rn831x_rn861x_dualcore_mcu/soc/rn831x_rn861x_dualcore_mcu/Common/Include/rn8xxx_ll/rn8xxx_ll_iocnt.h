/********************************************************************************
 * @file    rn8xxx_ll_iocnt.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-12-27
 * @brief   Header file containing functions prototypes of Uart HAL library.
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
 * V1.1.1   2023-12-27  Add       Add macro definition for geting register and register bit
 * V1.1.0               Initial
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _Rn8xxx_IOCNT_H
#define _Rn8xxx_IOCNT_H

/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll_def.h"

#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) ||\
	defined(RN831x_RN861x_MCU_V4)
/* Exported define ------------------------------------------------------------*/

/*********************************************************************************/
#if defined(RN831x_RN861x_MCU_V2)
#define GET_REG_IOCNT_CFG0(iocnt , databits) ((iocnt)->IOCNT0_CFG & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_CFG1(iocnt , databits) ((iocnt)->IOCNT1_CFG & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_CFG2(iocnt , databits) ((iocnt)->IOCNT2_CFG & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_CTL(iocnt , databits) ((iocnt)->CTRL0 & GET_DATA_MASK(databits))

#define SET_REG_IOCNT_CFG0(iocnt , data) ((iocnt)->IOCNT0_CFG = (data))
#define SET_REG_IOCNT_CFG1(iocnt , data) ((iocnt)->IOCNT1_CFG = (data))
#define SET_REG_IOCNT_CFG2(iocnt , data) ((iocnt)->IOCNT2_CFG = (data))
#define SET_REG_IOCNT_CTL(iocnt , data) ((iocnt)->CTRL0 = (data))

#define GET_BIT_IOCNT_CTL(iocnt, bit) (((iocnt)->CTRL0 & (bit)) ? (1u) : (0u))
#define SET_BIT_IOCNT_CTL(iocnt, bit) SET_REG32_BIT((iocnt)->CTRL0, (bit))

#define IOCNT0_CTL_EN  SET_REG32_BIT(GPIO->CTRL0, (BIT0))
#define IOCNT1_CTL_EN  SET_REG32_BIT(GPIO->CTRL0, (BIT1))
#define IOCNT2_CTL_EN  SET_REG32_BIT(GPIO->CTRL0, (BIT2))

#define IOCNT0_CTL_DIS  CLR_REG32_BIT(GPIO->CTRL0, (BIT0))
#define IOCNT1_CTL_DIS  CLR_REG32_BIT(GPIO->CTRL0, (BIT1))
#define IOCNT2_CTL_DIS  CLR_REG32_BIT(GPIO->CTRL0, (BIT2))

typedef enum
{
    IOCNT_ID0 = 0,
    IOCNT_ID1 = 1,
    IOCNT_ID2 = 2,

} eIOCNT_ID_TypeDef;

typedef enum
{
    IOCNTx_CNT_DIS = 0,      /* IOCNT2~IOCNT0 使能配置:不使能 */
    IOCNTx_CNT_EN = 1,       /* IOCNT2~IOCNT0 使能配置:使能 */

    IOCNTx_MODE_RISING = 0,  /* IOCNT2~IOCNT0 脉冲计数边沿选择:上升沿计数 */
    IOCNTx_MODE_FALLING = 1, /* IOCNT2~IOCNT0 脉冲计数边沿选择:下升沿计数 */

    CNT_CLR = 0,            /*"所有脉冲计数器类型选择:读后清零型"*/
    CNT_ADD = 1,            /*"所有脉冲计数器类型选择:累加型"*/

} eIOCNT_CTL_TypeDef;

typedef union
{
    struct
    {
        uint32_t CNT : 20;
        uint32_t CFG : 11;
        uint32_t INT :  1;
    }bitIOCNTCFG;
    uint32_t IOCNTCFG;
} uIOCNT_CFG_TypeDef;

typedef union
{
    struct
    {
        uint32_t IOCNT0_EN :  1;
        uint32_t IOCNT1_EN :  1;
        uint32_t IOCNT2_EN :  1;
        uint32_t Reserved0 :  5;
        uint32_t IOCNT0_MODE  :  1;
        uint32_t IOCNT1_MODE  :  1;
        uint32_t IOCNT2_MODE  :  1;
        uint32_t Reserved1    :  5;
        uint32_t CNT_CLR      :  1;

    }bitCTL;
    uint32_t CTL;
} uIOCNT_CTL_TypeDef;

typedef struct
{
    uIOCNT_CFG_TypeDef CFGx;
    uint8_t Ctrl_EN;
    uint8_t Ctrl_Mode;
    uint8_t Ctrl_CntClr;
} sLL_IOCNT_InitTypeDef;

#else
#define GET_REG_IOCNT_CFG0(iocnt , databits) ((iocnt)->CFG0 & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_CFG1(iocnt , databits) ((iocnt)->CFG1 & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_CFG2(iocnt , databits) ((iocnt)->CFG2 & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_CFG3(iocnt , databits) ((iocnt)->CFG3 & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_CFG4(iocnt , databits) ((iocnt)->CFG4 & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_OUT0(iocnt , databits) ((iocnt)->OUT0 & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_OUT1(iocnt , databits) ((iocnt)->OUT1 & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_OUT2(iocnt , databits) ((iocnt)->OUT2 & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_OUT3(iocnt , databits) ((iocnt)->OUT3 & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_OUT4(iocnt , databits) ((iocnt)->OUT4 & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_CHNL(iocnt , databits) ((iocnt)->CHNL & GET_DATA_MASK(databits))
#if defined(RN831x_RN861x_MCU_V4)
#define GET_REG_IOCNT_CTL(iocnt , databits) ((iocnt)->CTRL & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_COPL(iocnt , databits) ((iocnt)->COPL & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_REMD0(iocnt , databits) ((iocnt)->REMD0 & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_REMD1(iocnt , databits) ((iocnt)->REMD1 & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_REMD2(iocnt , databits) ((iocnt)->REMD2 & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_REMD3(iocnt , databits) ((iocnt)->REMD3 & GET_DATA_MASK(databits))
#define GET_REG_IOCNT_REMD4(iocnt , databits) ((iocnt)->REMD4 & GET_DATA_MASK(databits))
#else
#define GET_REG_IOCNT_CTL(iocnt , databits) ((iocnt)->CTL & GET_DATA_MASK(databits))
#endif
#define SET_REG_IOCNT_CFG0(iocnt , data) ((iocnt)->CFG0 = (data))
#define SET_REG_IOCNT_CFG1(iocnt , data) ((iocnt)->CFG1 = (data))
#define SET_REG_IOCNT_CFG2(iocnt , data) ((iocnt)->CFG2 = (data))
#define SET_REG_IOCNT_CFG3(iocnt , data) ((iocnt)->CFG3 = (data))
#define SET_REG_IOCNT_CFG4(iocnt , data) ((iocnt)->CFG4 = (data))
#define SET_REG_IOCNT_OUT0(iocnt , data) ((iocnt)->OUT0 = (data))
#define SET_REG_IOCNT_OUT1(iocnt , data) ((iocnt)->OUT1 = (data))
#define SET_REG_IOCNT_OUT2(iocnt , data) ((iocnt)->OUT2 = (data))
#define SET_REG_IOCNT_OUT3(iocnt , data) ((iocnt)->OUT3 = (data))
#define SET_REG_IOCNT_OUT4(iocnt , data) ((iocnt)->OUT4 = (data))
#define SET_REG_IOCNT_CHNL(iocnt , data) ((iocnt)->CHNL = (data))
#if defined(RN831x_RN861x_MCU_V4)
#define SET_REG_IOCNT_CMD(iocnt , data) ((iocnt)->CMD = (data))

#define SET_REG_IOCNT_COPL(iocnt , data) ((iocnt)->COPL = (data))

#define SET_REG_IOCNT_REMD0(iocnt , data) ((iocnt)->REMD0 = (data))
#define SET_REG_IOCNT_REMD1(iocnt , data) ((iocnt)->REMD1 = (data))
#define SET_REG_IOCNT_REMD2(iocnt , data) ((iocnt)->REMD2 = (data))
#define SET_REG_IOCNT_REMD3(iocnt , data) ((iocnt)->REMD3 = (data))
#define SET_REG_IOCNT_REMD4(iocnt , data) ((iocnt)->REMD4 = (data))

#define SET_REG_IOCNT_CTL(iocnt , data) ((iocnt)->CTRL = (data))
#define GET_BIT_IOCNT_CTL(iocnt, bit) (((iocnt)->CTRL & (bit)) ? (1u) : (0u))
#define SET_BIT_IOCNT_CTL(iocnt, bit) SET_REG32_BIT((iocnt)->CTRL, (bit))
#define CLR_BIT_IOCNT_CTL(iocnt, bit) CLR_REG32_BIT((iocnt)->CTRL, (bit))
#else
#define SET_REG_IOCNT_CTL(iocnt , data) ((iocnt)->CTL = (data))
#define GET_BIT_IOCNT_CTL(iocnt, bit) (((iocnt)->CTL & (bit)) ? (1u) : (0u))
#define SET_BIT_IOCNT_CTL(iocnt, bit) SET_REG32_BIT((iocnt)->CTL, (bit))
#endif
typedef enum
{
    IOCNT_ID0 = 0,
    IOCNT_ID1 = 1,
    IOCNT_ID2 = 2,
    IOCNT_ID3 = 3,
    IOCNT_ID4 = 4,

} eIOCNT_ID_TypeDef;


#if defined(RN831x_RN861x_MCU_V4)

typedef enum
{
    SEL_INT_FOLLOW = 0x0U,       /*" 输出脉冲电平方向与输入脉冲保持一致 "*/
    SEL_INT_INVERT = 0x1U,       /*" 输出脉冲反向 "*/
} eIOCNT_INT_TypeDef; /* 脉冲转发输入通道选择 */  /*使能 内部特殊命令 进行脉冲转发时，必须 bypass 滤波功能 */

typedef enum
{
    SEL_IOCNTx_INTERNAL = 0x0U,       /*" IOCNTx 选择内部输入特殊命令 "*/
    SEL_IOCNTx_EXTERNAL = 0x1U,       /*" IOCNTx 选择外部输入 "*/
} eIOCNT_CHNLSEL_TypeDef; /* 脉冲转发输入通道选择 */  /*使能 内部特殊命令 进行脉冲转发时，必须 bypass 滤波功能 */

typedef enum
{
    SEL_IOCNTx_COPL_0 = 0x0U, /*" IOCNTx在第1个输入脉冲时输出第1个低频脉冲，在第（n*分频系数）+1个输入脉冲时输出第n+1个低频脉冲 "*/
    SEL_IOCNTx_COPL_1 = 0x1U, /*" IOCNTx在第（n*分频系数）个输入脉冲时输出第n个低频脉冲 "*/
} eIOCNT_COPL_TypeDef; /* 脉冲转发输出脉冲配置选择 */

typedef union
{
    struct
    {
        uint32_t COPL0: 1;
        uint32_t COPL1: 1;
        uint32_t COPL2: 1;
        uint32_t COPL3: 1;
        uint32_t COPL4: 1;
        uint32_t Reserved0 : 27;
    }bitIOCNTCOPL;
    uint32_t IOCNTCOPL;
} uIOCNT_COPL_TypeDef;

#else
typedef enum
{
    SEL_CF_OUT0 = 0x00,       /*" 内部输入 CF_OUT0 "*/
    SEL_CF_OUT1 = 0x01,       /*" 内部输入 CF_OUT1 "*/
    SEL_CF_OUT2 = 0x02,       /*" 内部输入 CF_OUT2 "*/
    SEL_CF_OUT3 = 0x03,       /*" 内部输入 CF_OUT3 "*/
    SEL_CF_OUT4 = 0x04,       /*" 内部输入 CF_OUT4 "*/

    SEL_D2F_OUT0 = 0x05,       /*" 内部输入 D2F_OUT0 "*/
    SEL_D2F_OUT1 = 0x06,       /*" 内部输入 D2F_OUT1 "*/
    SEL_D2F_OUT2 = 0x07,       /*" 内部输入 D2F_OUT2 "*/
    SEL_D2F_OUT3 = 0x08,       /*" 内部输入 D2F_OUT3 "*/
    SEL_D2F_OUT4 = 0x09,       /*" 内部输入 D2F_OUT4 "*/

    SEL_INT1 = 0x0A,       /*" 外部中断输入 INT1 "*/
    SEL_INT2 = 0x0B,       /*" 外部中断输入 INT2 "*/
    SEL_INT3 = 0x0C,       /*" 外部中断输入 INT3 "*/
    SEL_INT4 = 0x0D,       /*" 外部中断输入 INT4 "*/
    SEL_INT5 = 0x0E,       /*" 外部中断输入 INT5 "*/
    SEL_INT6 = 0x0F,       /*" 外部中断输入 INT6 "*/
    SEL_INT7 = 0x10,       /*" 外部中断输入 INT7 "*/

    SEL_UART0_PF = 0x11,       /*" 内部输入 UART0_PF "*/ /* 使能 UART 进行脉冲转发时，分频系数必须配置大于等 于 2 分频，必须 bypass 滤波功能 */
    SEL_UART1_PF = 0x12,       /*" 内部输入 UART1_PF "*/
    SEL_UART2_PF = 0x13,       /*" 内部输入 UART2_PF "*/
    SEL_UART3_PF = 0x14,       /*" 内部输入 UART3_PF "*/
    SEL_UART4_PF = 0x15,       /*" 内部输入 UART4_PF "*/
    SEL_UART5_PF = 0x16,       /*" 内部输入 UART5_PF "*/

    SEL_UART0_QF = 0x17,       /*" 内部输入 UART0_QF "*/
    SEL_UART1_QF = 0x18,       /*" 内部输入 UART1_QF "*/
    SEL_UART2_QF = 0x19,       /*" 内部输入 UART2_QF "*/
    SEL_UART3_QF = 0x1A,       /*" 内部输入 UART3_QF "*/
    SEL_UART4_QF = 0x1B,       /*" 内部输入 UART4_QF "*/
    SEL_UART5_QF = 0x1C,       /*" 内部输入 UART5_QF "*/

    SEL_UART0_FPF = 0x1D,       /*" 内部输入 UART0_FPF "*/
    SEL_UART1_FPF = 0x1E,       /*" 内部输入 UART1_FPF "*/
    SEL_UART2_FPF = 0x1F,       /*" 内部输入 UART2_FPF "*/
    SEL_UART3_FPF = 0x20,       /*" 内部输入 UART3_FPF "*/
    SEL_UART4_FPF = 0x21,       /*" 内部输入 UART4_FPF "*/
    SEL_UART5_FPF = 0x22,       /*" 内部输入 UART5_FPF "*/

} eIOCNT_CHNLSEL_TypeDef; /* 脉冲转发输入通道选择 */  /*使能 UART 进行脉冲转发时，分频系数必须配置大于等 于 2 分频，必须 bypass 滤波功能 */
#endif

typedef enum
{
    IOCNTx_CNT_DIS = 0,      /* IOCNT4~IOCNT0 使能配置:不使能 */
    IOCNTx_CNT_EN = 1,       /* IOCNT4~IOCNT0 使能配置:使能 */

    IOCNTx_MODE_RISING = 0,  /* IOCNT4~IOCNT0 脉冲计数边沿选择:上升沿计数 */
    IOCNTx_MODE_FALLING = 1, /* IOCNT4~IOCNT0 脉冲计数边沿选择:下升沿计数 */

    CNT_CLR = 0,            /*"所有脉冲计数器类型选择:读后清零型"*/
    CNT_ADD = 1,            /*"所有脉冲计数器类型选择:累加型"*/

    IOCNTx_FLT_EN = 0,      /*"IOCNT4~IOCNT0 滤波功能:使能"*/
    IOCNTx_FLT_DIS = 1,     /*"IOCNT4~IOCNT0 滤波功能:不使能"*/

} eIOCNT_CTL_TypeDef;

typedef union
{
    struct
    {
        uint32_t CNT : 20;
        uint32_t CFG : 11;
        uint32_t INT :  1;
    }bitIOCNTCFG;
    uint32_t IOCNTCFG;
} uIOCNT_CFG_TypeDef;

typedef union
{
    struct
    {
        uint32_t IOCNT0_SEL : 6;
        uint32_t IOCNT1_SEL : 6;
        uint32_t IOCNT2_SEL : 6;
        uint32_t IOCNT3_SEL : 6;
        uint32_t IOCNT4_SEL : 6;
        uint32_t Reserved0 :  2;
    }bitCHNL;
    uint32_t CHNL;
} uIOCNT_CHNL_TypeDef;

typedef union
{
    struct
    {
        uint32_t IOCNT0_EN :  1;
        uint32_t IOCNT1_EN :  1;
        uint32_t IOCNT2_EN :  1;
        uint32_t IOCNT3_EN :  1;
        uint32_t IOCNT4_EN :  1;
        uint32_t Reserved0 :  3;

        uint32_t IOCNT0_MODE  :  1;
        uint32_t IOCNT1_MODE  :  1;
        uint32_t IOCNT2_MODE  :  1;
        uint32_t IOCNT3_MODE  :  1;
        uint32_t IOCNT4_MODE  :  1;
        uint32_t Reserved1    :  3;

        uint32_t CNT_CLR  :  1;
        uint32_t FLT_BYPASS0  :  1;
        uint32_t FLT_BYPASS1  :  1;
        uint32_t FLT_BYPASS2  :  1;
        uint32_t FLT_BYPASS3  :  1;
        uint32_t FLT_BYPASS4  :  1;
        uint32_t Reserved2    :  3;

        uint32_t FLT_CFG  :  7;

    }bitCTL;
    uint32_t CTL;
} uIOCNT_CTL_TypeDef;

typedef struct
{
    uIOCNT_CFG_TypeDef CFGx;
    uIOCNT_CHNL_TypeDef CHNLx;
    uint8_t Ctrl_EN;
    uint8_t Ctrl_Mode;
    uint8_t Ctrl_CntClr;
    uint8_t Ctrl_FLTEN;
    uint8_t Ctrl_FLTCfg;
#if defined(RN831x_RN861x_MCU_V4)
    uint32_t COPL;
    uint32_t REMD;
#endif

} sLL_IOCNT_InitTypeDef;

#endif

/* Exported data ----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
ErrorStatus LL_IOCNT_IOCNTxInit(sLL_IOCNT_InitTypeDef *sIOCNT_Init , eIOCNT_ID_TypeDef IOCNT_id);
void LL_IOCNT_Disable(eIOCNT_ID_TypeDef IOCNT_id);
#endif
#endif //_UART_H
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
