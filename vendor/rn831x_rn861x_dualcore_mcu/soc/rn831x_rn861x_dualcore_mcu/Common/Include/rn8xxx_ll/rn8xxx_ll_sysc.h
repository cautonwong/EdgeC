/********************************************************************************
 * @file    Rn8xxxSysc.h
 * @author  Renergy Application Team
 * @version V1.1.0
 * @date    2023-09-06
 * @brief   Header file containing functions prototypes of Sysc HAL library.
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
 *
 ******************************************************************************
 * @ Revised records
 * V1.1.0  Initial
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef Rn8xxx_SYSC_H_
#define Rn8xxx_SYSC_H_

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll_def.h"
/* Exported define ------------------------------------------------------------*/

/*********************************************************************************/

/*********************************************************************************/
#define GET_REG_SYSC_OSC_CTRL1(sysc, databits) ((sysc)->OSC_CTRL1 & GET_DATA_MASK(databits))
#define GET_REG_SYSC_SYS_PD(sysc, databits) ((sysc)->SYS_PD & GET_DATA_MASK(databits))
#define GET_REG_SYSC_ADC_CTRL(sysc, databits) ((sysc)->ADC_CTRL & GET_DATA_MASK(databits))
#define GET_REG_SYSC_OSC_CTRL2(sysc, databits) ((sysc)->OSC_CTRL2 & GET_DATA_MASK(databits))
#define GET_REG_SYSC_SYS_RST(sysc, databits) ((sysc)->SYS_RST & GET_DATA_MASK(databits))
#define GET_REG_SYSC_MOD0_EN(sysc, databits) ((sysc)->MOD0_EN & GET_DATA_MASK(databits))
#define GET_REG_SYSC_MOD1_EN(sysc, databits) ((sysc)->MOD1_EN & GET_DATA_MASK(databits))
#define GET_REG_SYSC_INTC_EN(sysc, databits) ((sysc)->INTC_EN & GET_DATA_MASK(databits))
#define GET_REG_SYSC_KBI_EN(sysc, databits) ((sysc)->KBI_EN & GET_DATA_MASK(databits))
#define GET_REG_SYSC_SYS_PS(sysc, databits) ((sysc)->SYS_PS & GET_DATA_MASK(databits))

#define SET_REG_SYSC_OSC_CTRL1(sysc, data) ((sysc)->OSC_CTRL1 = (data))
#define SET_REG_SYSC_SYS_PD(sysc, data) ((sysc)->SYS_PD = (data))
#define SET_REG_SYSC_ADC_CTRL(sysc, data) ((sysc)->ADC_CTRL = (data))
#define SET_REG_SYSC_OSC_CTRL2(sysc, data) ((sysc)->OSC_CTRL2 = (data))
#define SET_REG_SYSC_SYS_RST(sysc, data) ((sysc)->SYS_RST = (data))
#define SET_REG_SYSC_MOD0_EN(sysc, data) ((sysc)->MOD0_EN = (data))
#define SET_REG_SYSC_MOD1_EN(sysc, data) ((sysc)->MOD1_EN = (data))
#define SET_REG_SYSC_INTC_EN(sysc, data) ((sysc)->INTC_EN = (data))
#define SET_REG_SYSC_KBI_EN(sysc, data) ((sysc)->KBI_EN = (data))
#define SET_REG_SYSC_SYS_PS(sysc, data) ((sysc)->SYS_PS = (data))

/*********************************************************************************/

#define GET_BIT_SYSC_OSC_CTRL1(sysc, bit) (((sysc)->OSC_CTRL1 & (bit)) ? (1) : (0))
#define GET_BIT_SYSC_SYS_PD(sysc, bit) (((sysc)->SYS_PD & (bit)) ? (1) : (0))
#define GET_BIT_SYSC_ADC_CTRL(sysc, bit) (((sysc)->ADC_CTRL & (bit)) ? (1) : (0))
#define GET_BIT_SYSC_OSC_CTRL2(sysc, bit) (((sysc)->OSC_CTRL2 & (bit)) ? (1) : (0))
#define GET_BIT_SYSC_SYS_RST(sysc, bit) (((sysc)->SYS_RST & (bit)) ? (1) : (0))
#define GET_BIT_SYSC_MOD0_EN(sysc, bit) (((sysc)->MOD0_EN & (bit)) ? (1) : (0))
#define GET_BIT_SYSC_MOD1_EN(sysc, bit) (((sysc)->MOD1_EN & (bit)) ? (1) : (0))
#define GET_BIT_SYSC_INTC_EN(sysc, bit) (((sysc)->INTC_EN & (bit)) ? (1) : (0))
#define GET_BIT_SYSC_KBI_EN(sysc, bit) (((sysc)->KBI_EN & (bit)) ? (1) : (0))
#define GET_BIT_SYSC_SYS_PS(sysc, bit) (((sysc)->SYS_PS & (bit)) ? (1) : (0))

#define SET_BIT_SYSC_OSC_CTRL1(sysc, bit) ((sysc)->OSC_CTRL1 |= (bit))
#define SET_BIT_SYSC_SYS_PD(sysc, bit) ((sysc)->SYS_PD |= (bit))
#define SET_BIT_SYSC_ADC_CTRL(sysc, bit) ((sysc)->ADC_CTRL |= (bit))
#define SET_BIT_SYSC_OSC_CTRL2(sysc, bit) ((sysc)->OSC_CTRL2 |= (bit))
#define SET_BIT_SYSC_SYS_RST(sysc, bit) ((sysc)->SYS_RST = (bit))
#define SET_BIT_SYSC_MOD0_EN(sysc, bit) ((sysc)->MOD0_EN |= (bit))
#define SET_BIT_SYSC_MOD1_EN(sysc, bit) ((sysc)->MOD1_EN |= (bit))
#define SET_BIT_SYSC_INTC_EN(sysc, bit) ((sysc)->INTC_EN |= (bit))
#define SET_BIT_SYSC_KBI_EN(sysc, bit) ((sysc)->KBI_EN |= (bit))
#define SET_BIT_SYSC_SYS_PS(sysc, bit) ((sysc)->SYS_PS |= (bit))

/*--------------------------系统控制寄存器使能-----------------------------------*/
#if defined(RN831x_RN861x_MCU_V4)
#define SYSCTL_ENABLE_REGISTE (SYSTEM->SYS_PSW1)
#define SYSCTL_ENABLE_WRITE    \
    {                          \
        SYSTEM->SYS_PSW1 = 0x82U; \
    }
#define SYSCTL_DISABLE_WRITE   \
    {                          \
        SYSTEM->SYS_PSW1 = 0x00U; \
    }
#else
#define SYSCTL_ENABLE_REGISTE (SYSCTL->SYS_PS)
#define SYSCTL_ENABLE_WRITE    \
    {                          \
        SYSCTL->SYS_PS = 0x82U; \
    }
#define SYSCTL_DISABLE_WRITE   \
    {                          \
        SYSCTL->SYS_PS = 0x00U; \
    }
#endif
/*--------------------------------------------------------------*/
/*--------------------------各模块APB时钟对应位置-----------------------------------*/
#if defined (RN831x_RN861x_MCU_V4)

typedef enum{
    LL_SYSC_RST_VALID,
    LL_SYSC_RST_INVALID
}eLL_SYSC_RST_Status_TypeDef;

typedef enum
{
    LL_SYSC_RSTFLAG_POWEROK = (0U),
    LL_SYSC_RSTFLAG_PIN = (1U),
    LL_SYSC_RSTFLAG_CORE0WDT = (2U),
    LL_SYSC_RSTFLAG_CORE0SOFT =(4U),
    LL_SYSC_RSTFLAG_BOIVBAT =(6U),

}eLL_SYSC_RSTFlag_TypeDef;
#endif
#if defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_C) ||defined(RN821x_RN721x_SOC_D)
typedef enum
{
    LL_SYSC_TC_ID =  (0U),
    LL_SYSC_CPC_ID = (3U),
    LL_SYSC_TC0_ID = (4U),
    LL_SYSC_TC1_ID = (5U),
    LL_SYSC_UART4_ID = (6U),
    LL_SYSC_UART5_ID = (7U),
    LL_SYSC_UART0_ID = (8U),
    LL_SYSC_UART1_ID = (9U),
    LL_SYSC_UART2_ID =   (10U),
    LL_SYSC_UART3_ID =   (11U),
    LL_SYSC_UART38K_ID = (12U),
    LL_SYSC_ISO7816_ID = (13U),
    LL_SYSC_I2C_ID =  (14U),
    LL_SYSC_SPI0_ID = (15U),
    LL_SYSC_SIMPTC_ID = (20U),
    LL_SYSC_DMA0_ID = (32U),    /* mode1 0 + 32 */
    LL_SYSC_DMA1_ID = (33U),    /* mode1 1 + 32 */
    LL_SYSC_SPI1_ID = (35U),    /* mode1 3 + 32 */
    LL_SYSC_DMA_ID = (36U),     /* mode1 4 + 32 */
    LL_SYSC_GPIO_ID = (37U),    /* mode1 5 + 32 */
    LL_SYSC_LCD_ID = (38U),     /* mode1 6 + 32 */
    LL_SYSC_EMU_ID = (39U),     /* mode1 7 + 32 */
    LL_SYSC_NVM_ID = (40U),     /* mode1 8 + 32 */
    LL_SYSC_WDT_ID = (41U),     /* mode1 9 + 32 */
    LL_SYSC_RTC_ID = (42U),     /* mode1 10 + 32 */
    LL_SYSC_SAR_ID = (43U),     /* mode1 11 + 32 */
    LL_SYSC_NVM_REG_ID = (44U), /* mode1 12 + 32 */

    LL_SYSC_INTC0_ID = (64U), /* INT_EN 0 + 64*/
    LL_SYSC_INTC1_ID = (65U), /* INT_EN 1 + 64*/
    LL_SYSC_INTC2_ID = (66U), /* INT_EN 2 + 64*/
    LL_SYSC_INTC3_ID = (67U), /* INT_EN 3 + 64*/
    LL_SYSC_INTC4_ID = (68U), /* INT_EN 4 + 64*/
    LL_SYSC_INTC5_ID = (69U), /* INT_EN 5 + 64*/
    LL_SYSC_INTC6_ID = (70U), /* INT_EN 6 + 64*/
    LL_SYSC_INTC7_ID = (71U), /* INT_EN 7 + 64*/
    LL_SYSC_INTC_ID = (72U), /* INT_EN 8 + 64*/

    LL_SYSC_KBI0_ID = (96U),  /* KBI_EN 0 + 96*/
    LL_SYSC_KBI1_ID = (97U),  /* KBI_EN 1 + 96*/
    LL_SYSC_KBI2_ID = (98U),  /* KBI_EN 2 + 96*/
    LL_SYSC_KBI3_ID = (99U),  /* KBI_EN 3 + 96*/
    LL_SYSC_KBI4_ID = (100U), /* KBI_EN 4 + 96*/
    LL_SYSC_KBI5_ID = (101U), /* KBI_EN 5 + 96*/
    LL_SYSC_KBI6_ID = (102U), /* KBI_EN 6 + 96*/
    LL_SYSC_KBI7_ID = (103U), /* KBI_EN 7 + 96*/
    LL_SYSC_KBI_ID = (104U), /* KBI_EN 8 + 96*/
} eModuleID_TypeDef;

typedef enum
{
    LL_SYSC_PD_ADCI1 = (0U),
    LL_SYSC_PD_ADCI2 = (1U),
    LL_SYSC_PD_ADCU = (2U),
    LL_SYSC_PD_LVD = (3U),
    LL_SYSC_PD_CMP1 =(4U),
    LL_SYSC_PD_CMP2 = (5U),
    LL_SYSC_PD_BGR = (6U),

}eLL_SYSC_PDModuleID_TypeDef;


typedef enum
{
    LL_SYSC_ADCCHN_U =  (0U),
    LL_SYSC_ADCCHN_I1 = (1U),
    LL_SYSC_ADCCHN_I2 = (2U)
}eLL_SYSC_ADCChannel_TypeDef;

typedef enum
{
    LL_SYSC_ADCPGA_1 = (0U),
    LL_SYSC_ADCPGA_2 = (1U),
    LL_SYSC_ADCPGA_4 = (2U),
    LL_SYSC_ADCPGA_8 = (2U),
    LL_SYSC_ADCPGA_16 = (3U),
    LL_SYSC_ADCPGA_MAX = (7U),
}eLL_SYSC_ADCPGA_TypeDef;

typedef enum
{
    LL_SYSC_RSTRQST_EMU = (0U),
    LL_SYSC_RSTRQST_NVM = (1U),

}eLL_SYSC_RSTRqst_TypeDef;

typedef enum{
    LL_SYSC_RST_VALID,
    LL_SYSC_RST_INVALID
}eLL_SYSC_RST_Status_TypeDef;

typedef enum
{
    LL_SYSC_RSTFLAG_POWEROK = (5U),
    LL_SYSC_RSTFLAG_PIN = (6U),
    LL_SYSC_RSTFLAG_WDT = (7U),
    LL_SYSC_RSTFLAG_MCU =(8U),

}eLL_SYSC_RSTFlag_TypeDef;

typedef enum
{
    LL_SYSC_RSTCONFG_LOCKUP = (2U),

}eLL_SYSC_RSTConfg_TypeDef;
#elif defined(RN821x_RN721x_SOC_V2)
typedef enum
{
    LL_SYSC_TC0_ID = (4U),
    LL_SYSC_TC1_ID = (5U),
    LL_SYSC_UART4_ID = (6U),
    LL_SYSC_UART5_ID = (7U),
    LL_SYSC_UART0_ID = (8U),
    LL_SYSC_UART1_ID = (9U),
    LL_SYSC_UART2_ID = (10U),
    LL_SYSC_UART3_ID = (11U),
    LL_SYSC_UART38K_ID = (12U),
    LL_SYSC_ISO7816_ID = (13U),
    LL_SYSC_I2C_ID = (14U),
    LL_SYSC_SPI0_ID = (15U),

    LL_SYSC_SPI2_ID = (17U),
    LL_SYSC_SPI3_ID = (18U),

    LL_SYSC_SIMPTC_ID = (20U),

    LL_SYSC_LPUART_ID = (25U),
    LL_SYSC_CRC_ID = (26U),


    LL_SYSC_SPI1_ID = (35U),    /* mode1 3 + 32 = 35 */

    LL_SYSC_GPIO_ID = (37U),    /* mode1 5 + 32 */
    LL_SYSC_LCD_ID = (38U),     /* mode1 6 + 32 */
    LL_SYSC_EMU_ID = (39U),     /* mode1 7 + 32 */
    LL_SYSC_NVM_ID = (40U),     /* mode1 8 + 32 */
    LL_SYSC_WDT_ID = (41U),    /*  mode1 9 + 32 */
    LL_SYSC_RTC_ID = (42U),     /* mode1 10 + 32  */
    LL_SYSC_SAR_ID = (43U),     /* mode1 11 + 32  */
    LL_SYSC_NVMREG_ID = (44U), /* mode1 12 + 32 */
    LL_SYSC_D2F_ID = (45U), /* mode1 13 + 32*/
    LL_SYSC_DSP_ID = (46U), /* mode1 14 + 32*/
    LL_SYSC_M2M_ID = (47U), /* mode1 15 + 32*/
    LL_SYSC_FLK_ID = (48U), /* mode1 16 + 32*/
    LL_SYSC_IOCNT_ID = (49U), /* mode1 17 + 32*/
    LL_SYSC_ECT_ID = (50U), /* mode1 18 + 32*/

    LL_SYSC_INTC0_ID = (64U), /* INT_EN 0 + 64*/
    LL_SYSC_INTC1_ID = (65U), /* INT_EN 1 + 64*/
    LL_SYSC_INTC2_ID = (66U), /* INT_EN 2 + 64*/
    LL_SYSC_INTC3_ID = (67U), /* INT_EN 3 + 64*/
    LL_SYSC_INTC4_ID = (68U), /* INT_EN 4 + 64*/
    LL_SYSC_INTC5_ID = (69U), /* INT_EN 5 + 64*/
    LL_SYSC_INTC6_ID = (70U), /* INT_EN 6 + 64*/
    LL_SYSC_INTC7_ID = (71U), /* INT_EN 7 + 64*/
    LL_SYSC_INTC_ID = (72U), /* INT_EN 8 + 64*/

    LL_SYSC_KBI0_ID = (96U),  /* KBI_EN 0 + 96*/
    LL_SYSC_KBI1_ID = (97U),  /* KBI_EN 1 + 96*/
    LL_SYSC_KBI2_ID = (98U),  /* KBI_EN 2 + 96*/
    LL_SYSC_KBI3_ID = (99U),  /* KBI_EN 3 + 96*/
    LL_SYSC_KBI4_ID = (100U), /* KBI_EN 4 + 96*/
    LL_SYSC_KBI5_ID = (101U), /* KBI_EN 5 + 96*/
    LL_SYSC_KBI6_ID = (102U), /* KBI_EN 6 + 96*/
    LL_SYSC_KBI7_ID = (103U), /* KBI_EN 7 + 96*/
    LL_SYSC_KBI_ID = (104U), /* KBI_EN 8 + 96*/
} eModuleID_TypeDef;

typedef enum
{
    LL_SYSC_PD_ADCI1 = (0U),
    LL_SYSC_PD_ADCI2 = (1U),
    LL_SYSC_PD_ADCU = (2U),
    LL_SYSC_PD_LVD = (3U),
    LL_SYSC_PD_CMP1 =(4U),
    LL_SYSC_PD_CMP2 = (5U),
    LL_SYSC_PD_BGR = (6U),
    LL_SYSC_PD_BGRLP = (7U),

    LL_SYSC_PD_LDO3_ID = (17U),
}eLL_SYSC_PDModuleID_TypeDef;


typedef enum
{
    LL_SYSC_ADCCHN_I1 = (0U),
    LL_SYSC_ADCCHN_I2 = (1U),
    LL_SYSC_ADCCHN_U =(2U)
}eLL_SYSC_ADCChannel_TypeDef;

typedef enum
{
    LL_SYSC_ADCPGA_1 = (0U),
    LL_SYSC_ADCPGA_2 = (1U),
    LL_SYSC_ADCPGA_4 = (2U),
    LL_SYSC_ADCPGA_8 = (2U),
    LL_SYSC_ADCPGA_16 = (3U),
    LL_SYSC_ADCPGA_MAX = (7U),
}eLL_SYSC_ADCPGA_TypeDef;

typedef enum
{
    LL_SYSC_RSTRQST_EMU = (0U),
    LL_SYSC_RSTRQST_NVM = (1U),
    LL_SYSC_RSTRQST_EMUREG = (3U),
    LL_SYSC_RSTRQST_CM0 = (12U),
    LL_SYSC_RSTRQST_IIC = (20U),
}eLL_SYSC_RSTRqst_TypeDef;

typedef enum{

    LL_SYSC_RST_INVALID = (0U),
    LL_SYSC_RST_VALID = (1U)
}eLL_SYSC_RST_Status_TypeDef;

typedef enum
{
    LL_SYSC_RSTFLAG_POWEROK = (5U),
    LL_SYSC_RSTFLAG_PIN = (6U),
    LL_SYSC_RSTFLAG_WDT = (7U),
    LL_SYSC_RSTFLAG_MCU =(8U),
    LL_SYSC_RSTFLAG_BORV2P8 = (11U),
    LL_SYSC_RSTFLAG_CM0 = (13U),
}eLL_SYSC_RSTFlag_TypeDef;

typedef enum
{
    LL_SYSC_RSTCONFG_LOCKUP = (2U),
    LL_SYSC_RSTCONFG_BORV2P8 = (9U),
    LL_SYSC_RSTCONFG_UART0 = (14U),
    LL_SYSC_RSTCONFG_UART1 = (15U),
    LL_SYSC_RSTCONFG_UART2 = (16U),
    LL_SYSC_RSTCONFG_UART3 = (17U),
    LL_SYSC_RSTCONFG_UART4 = (18U),
    LL_SYSC_RSTCONFG_UART5 = (19U),

}eLL_SYSC_RSTConfg_TypeDef;

typedef enum{
    LL_SYSC_DMA_CHN_CACHE = (0U),
    LL_SYSC_DMA_CHN_EMU = (1U),
    LL_SYSC_DMA_CHN_SPI0 = (2U),
    LL_SYSC_DMA_CHN_SPI1 = (3U),
    LL_SYSC_DMA_CHN_SPI2 = (4U),
    LL_SYSC_DMA_CHN_SPI3 = (5U),
    LL_SYSC_DMA_CHN_UART0 = (6U),
    LL_SYSC_DMA_CHN_UART1 = (7U),
    LL_SYSC_DMA_CHN_UART2 = (8U),
    LL_SYSC_DMA_CHN_UART3 = (9U),
    LL_SYSC_DMA_CHN_UART4 = (10U),
    LL_SYSC_DMA_CHN_UART5 = (11U),
    LL_SYSC_DMA_CHN_CAN = (12U),
    LL_SYSC_DMA_CHN_TC1 = (13U),
    LL_SYSC_DMA_CHN_DSP = (14U + 2U),
    LL_SYSC_DMA_CHN_CRC = (15U + 2U),
    LL_SYSC_DMA_CHN_M2M = (16U + 2U),
    LL_SYSC_DMA_CHN_CPU = (17U + 2U)
}eLL_SYSC_DMA_CHN_TypeDef;




typedef enum{

LL_SYSC_DMA_PRI0  = (0U), /* 优先级0最低 */
LL_SYSC_DMA_PRI1  = (1U), /* 优先级1 */
LL_SYSC_DMA_PRI2  = (2U), /* 优先级2 */
LL_SYSC_DMA_PRI3  = (3U), /* 优先级3最高 */
LL_SYSC_DMA_PRIMAX = (3U)
}eLL_SYSC_DMA_PRI_TypeDef;

#define LL_SYSC_DMA_PRI_PASSWORD 0XE
#define LL_SYSC_DMA_PRI_PASSWORD_ZONE 0XF
#elif defined(RN821x_RN721x_SOC_V3)
typedef enum
{

    LL_SYSC_TC1_ID = (5U),
    LL_SYSC_UART4_ID = (6U),
    LL_SYSC_UART5_ID = (7U),
    LL_SYSC_UART0_ID = (8U),
    LL_SYSC_UART1_ID = (9U),
    LL_SYSC_UART2_ID = (10U),
    LL_SYSC_UART3_ID = (11U),
    LL_SYSC_UART38K_ID = (12U),

    LL_SYSC_I2C_ID = (14U),



    LL_SYSC_SPI3_ID = (18U),

    LL_SYSC_SIMPTC_ID = (20U),

    LL_SYSC_SPI1_ID = (35U),    /* mode1 3 + 32 = 35 */

    LL_SYSC_GPIO_ID = (37U),    /* mode1 5 + 32 */
    LL_SYSC_LCD_ID = (38U),     /* mode1 6 + 32 */
    LL_SYSC_EMU_ID = (39U),     /* mode1 7 + 32 */
    LL_SYSC_NVM_ID = (40U),     /* mode1 8 + 32 */
    LL_SYSC_WDT_ID = (41U),    /*  mode1 9 + 32 */
    LL_SYSC_RTC_ID = (42U),     /* mode1 10 + 32  */
    LL_SYSC_SAR_ID = (43U),     /* mode1 11 + 32  */
    LL_SYSC_NVMREG_ID = (44U), /* mode1 12 + 32 */

    LL_SYSC_IOCNT_ID = (49U), /* mode1 17 + 32*/


    LL_SYSC_INTC0_ID = (64U), /* INT_EN 0 + 64*/
    LL_SYSC_INTC1_ID = (65U), /* INT_EN 1 + 64*/
    LL_SYSC_INTC2_ID = (66U), /* INT_EN 2 + 64*/
    LL_SYSC_INTC3_ID = (67U), /* INT_EN 3 + 64*/
    LL_SYSC_INTC4_ID = (68U), /* INT_EN 4 + 64*/
    LL_SYSC_INTC5_ID = (69U), /* INT_EN 5 + 64*/
    LL_SYSC_INTC6_ID = (70U), /* INT_EN 6 + 64*/
    LL_SYSC_INTC7_ID = (71U), /* INT_EN 7 + 64*/
    LL_SYSC_INTC_ID = (72U), /* INT_EN 8 + 64*/

    LL_SYSC_KBI0_ID = (96U),  /* KBI_EN 0 + 96*/
    LL_SYSC_KBI1_ID = (97U),  /* KBI_EN 1 + 96*/
    LL_SYSC_KBI2_ID = (98U),  /* KBI_EN 2 + 96*/
    LL_SYSC_KBI3_ID = (99U),  /* KBI_EN 3 + 96*/
    LL_SYSC_KBI4_ID = (100U), /* KBI_EN 4 + 96*/
    LL_SYSC_KBI5_ID = (101U), /* KBI_EN 5 + 96*/
    LL_SYSC_KBI6_ID = (102U), /* KBI_EN 6 + 96*/
    LL_SYSC_KBI7_ID = (103U), /* KBI_EN 7 + 96*/
    LL_SYSC_KBI_ID = (104U), /* KBI_EN 8 + 96*/
} eModuleID_TypeDef;

typedef enum
{
    LL_SYSC_PD_ADCI1 = (0U),
    LL_SYSC_PD_ADCI2 = (1U),
    LL_SYSC_PD_ADCU = (2U),
    LL_SYSC_PD_LVD = (3U),
    LL_SYSC_PD_CMP1 =(4U),
    LL_SYSC_PD_CMP2 = (5U),
    LL_SYSC_PD_BGR = (6U),


    LL_SYSC_PD_LDO3_ID = (17U),
}eLL_SYSC_PDModuleID_TypeDef;


typedef enum
{
    LL_SYSC_ADCCHN_I1 = (0U),
    LL_SYSC_ADCCHN_I2 = (1U),
    LL_SYSC_ADCCHN_U =(2U)
}eLL_SYSC_ADCChannel_TypeDef;

typedef enum
{
    LL_SYSC_ADCPGA_1 = (0U),
    LL_SYSC_ADCPGA_2 = (1U),
    LL_SYSC_ADCPGA_4 = (2U),
    LL_SYSC_ADCPGA_8 = (2U),
    LL_SYSC_ADCPGA_16 = (3U),
    LL_SYSC_ADCPGA_MAX = (7U),
}eLL_SYSC_ADCPGA_TypeDef;

typedef enum
{
    LL_SYSC_RSTRQST_EMU = (0U),
    LL_SYSC_RSTRQST_NVM = (1U),
    LL_SYSC_RSTRQST_EMUREG = (3U),
    LL_SYSC_RSTRQST_CM0 = (12U),
    LL_SYSC_RSTRQST_IIC = (20U),
}eLL_SYSC_RSTRqst_TypeDef;

typedef enum{

    LL_SYSC_RST_INVALID = (0U),
    LL_SYSC_RST_VALID = (1U)
}eLL_SYSC_RST_Status_TypeDef;

typedef enum
{
    LL_SYSC_RSTFLAG_POWEROK = (5U),
    LL_SYSC_RSTFLAG_PIN = (6U),
    LL_SYSC_RSTFLAG_WDT = (7U),
    LL_SYSC_RSTFLAG_MCU =(8U),
    LL_SYSC_RSTFLAG_BORV2P8 = (11U),
    LL_SYSC_RSTFLAG_CM0 = (13U),
}eLL_SYSC_RSTFlag_TypeDef;

typedef enum
{
    LL_SYSC_RSTCONFG_LOCKUP = (2U),
    LL_SYSC_RSTCONFG_BORV2P8 = (9U),
    LL_SYSC_RSTCONFG_UART0 = (14U),
    LL_SYSC_RSTCONFG_UART1 = (15U),
    LL_SYSC_RSTCONFG_UART2 = (16U),
    LL_SYSC_RSTCONFG_UART3 = (17U),
    LL_SYSC_RSTCONFG_UART4 = (18U),
    LL_SYSC_RSTCONFG_UART5 = (19U),

}eLL_SYSC_RSTConfg_TypeDef;

typedef enum{
    LL_SYSC_DMA_CHN_CACHE = (0U),
    LL_SYSC_DMA_CHN_EMU = (1U),

    LL_SYSC_DMA_CHN_SPI1 = (3U),

    LL_SYSC_DMA_CHN_SPI3 = (5U),
    LL_SYSC_DMA_CHN_UART0 = (6U),
    LL_SYSC_DMA_CHN_UART1 = (7U),
    LL_SYSC_DMA_CHN_UART2 = (8U),
    LL_SYSC_DMA_CHN_UART3 = (9U),
    LL_SYSC_DMA_CHN_UART4 = (10U),
    LL_SYSC_DMA_CHN_UART5 = (11U),

    LL_SYSC_DMA_CHN_TC1 = (13U),
    LL_SYSC_DMA_CHN_DSP = (14U + 2U),

    LL_SYSC_DMA_CHN_CPU = (17U + 2U)
}eLL_SYSC_DMA_CHN_TypeDef;



typedef enum{

LL_SYSC_DMA_PRI0  = (0U), /* 优先级0最低 */
LL_SYSC_DMA_PRI1  = (1U), /* 优先级1 */
LL_SYSC_DMA_PRI2  = (2U), /* 优先级2 */
LL_SYSC_DMA_PRI3  = (3U), /* 优先级3最高 */
LL_SYSC_DMA_PRIMAX = (3U)
}eLL_SYSC_DMA_PRI_TypeDef;

#define LL_SYSC_DMA_PRI_PASSWORD 0XE
#define LL_SYSC_DMA_PRI_PASSWORD_ZONE 0XF

#elif defined (RN831x_RN861x_MCU_V2) || defined (RN831x_RN861x_MCU_V3)
typedef enum
{
    LL_SYSC_TC0_ID = (4U),
#if !defined(RN831x_RN861x_MCU_V3)
    LL_SYSC_TC1_ID = (5U),
#endif
    LL_SYSC_UART4_ID = (6U),
    LL_SYSC_UART5_ID = (7U),
    LL_SYSC_UART0_ID = (8U),
    LL_SYSC_UART1_ID = (9U),
    LL_SYSC_UART2_ID = (10U),
    LL_SYSC_UART3_ID = (11U),
    LL_SYSC_UART38K_ID = (12U),
    LL_SYSC_ISO7816_ID = (13U),
    LL_SYSC_I2C_ID = (14U),
    LL_SYSC_SPI0_ID = (15U),
    LL_SYSC_SPI1_ID = (16U),
#if !defined(RN831x_RN861x_MCU_V3)
    LL_SYSC_SPI2_ID = (17U),
    LL_SYSC_SPI3_ID = (18U),
#endif
    LL_SYSC_SIMPTC_ID = (20U),
#if defined(RN831x_RN861x_MCU_V3)
    LL_SYSC_UART6_ID = (25U),
    LL_SYSC_UART7_ID = (26U),
    LL_SYSC_LSRX_ID = (27U),
#endif

    LL_SYSC_GPIO_ID = (37U),    /* mode1 5 + 32 */
    LL_SYSC_LCD_ID = (38U),     /* mode1 6 + 32 */

    LL_SYSC_WDT_ID = (41U),     /* mode1 9 + 32 */
    LL_SYSC_RTCSAR_ID = (42U),    /* mode1 10 + 32 */
    LL_SYSC_CMPLVD_ID = (43U),    /* mode1 11 + 32 */

    LL_SYSC_D2F_ID = (45U), /* mode1 13 + 32 */
#if !defined(RN831x_RN861x_MCU_V3)
    LL_SYSC_DSP_ID = (46U), /* mode1 14 + 32 */
    LL_SYSC_M2M_ID = (47U), /* mode1 15 + 32 */
#endif

    LL_SYSC_INTC0_ID = (64U), /* INT_EN 0 + 64*/
    LL_SYSC_INTC1_ID = (65U), /* INT_EN 1 + 64*/
    LL_SYSC_INTC2_ID = (66U), /* INT_EN 2 + 64*/
    LL_SYSC_INTC3_ID = (67U), /* INT_EN 3 + 64*/
    LL_SYSC_INTC4_ID = (68U), /* INT_EN 4 + 64*/
    LL_SYSC_INTC5_ID = (69U), /* INT_EN 5 + 64*/
    LL_SYSC_INTC6_ID = (70U), /* INT_EN 6 + 64*/
    LL_SYSC_INTC7_ID = (71U), /* INT_EN 7 + 64*/
    LL_SYSC_INTC_ID = (72U), /* INT_EN 8 + 64*/

    LL_SYSC_KBI0_ID = (96U),  /* KBI_EN 0 + 96*/
    LL_SYSC_KBI1_ID = (97U),  /* KBI_EN 1 + 96*/
    LL_SYSC_KBI2_ID = (98U),  /* KBI_EN 2 + 96*/
    LL_SYSC_KBI3_ID = (99U),  /* KBI_EN 3 + 96*/
    LL_SYSC_KBI4_ID = (100U), /* KBI_EN 4 + 96*/
    LL_SYSC_KBI5_ID = (101U), /* KBI_EN 5 + 96*/
    LL_SYSC_KBI6_ID = (102U), /* KBI_EN 6 + 96*/
    LL_SYSC_KBI7_ID = (103U), /* KBI_EN 7 + 96*/
    LL_SYSC_KBI_ID = (104U), /* KBI_EN 8 + 96*/

} eModuleID_TypeDef;

typedef enum
{

    LL_SYSC_PD_LVD = (3U),
    LL_SYSC_PD_CMP1 =(4U),
    LL_SYSC_PD_CMP2 = (5U),

}eLL_SYSC_PDModuleID_TypeDef;

typedef enum
{
    LL_SYSC_RSTRQST_CM0 = (12U),
    LL_SYSC_RSTRQST_IIC = (20U),
}eLL_SYSC_RSTRqst_TypeDef;

typedef enum{
    LL_SYSC_RST_VALID,/* 复位有效，发生过该复位 */
    LL_SYSC_RST_INVALID /* 复位无效，没有发生过该复位 */
}eLL_SYSC_RST_Status_TypeDef;

typedef enum
{
    LL_SYSC_RSTFLAG_POWEROK = (5U),
    LL_SYSC_RSTFLAG_PIN = (6U),
    LL_SYSC_RSTFLAG_WDT = (7U),
    LL_SYSC_RSTFLAG_MCU =(8U),
}eLL_SYSC_RSTFlag_TypeDef;

typedef enum
{
    LL_SYSC_RSTCONFG_LOCKUP = (2U),
}eLL_SYSC_RSTConfg_TypeDef;

typedef enum{

    LL_SYSC_DMA_CHN_SPI0 = (0U),
    LL_SYSC_DMA_CHN_SPI1 = (1U),
#if !defined(RN831x_RN861x_MCU_V3)
    LL_SYSC_DMA_CHN_SPI2 = (2U),
    LL_SYSC_DMA_CHN_SPI3 = (3U),
#endif
    LL_SYSC_DMA_CHN_UART0 = (4U),
    LL_SYSC_DMA_CHN_UART1 = (5U),
    LL_SYSC_DMA_CHN_UART2 = (6U),
    LL_SYSC_DMA_CHN_UART3 = (7U),
    LL_SYSC_DMA_CHN_UART4 = (8U),
    LL_SYSC_DMA_CHN_UART5 = (9U),
#if !defined(RN831x_RN861x_MCU_V3)
    LL_SYSC_DMA_CHN_DSP = (10U),
    LL_SYSC_DMA_CHN_M2M = (11U),
#endif
    LL_SYSC_DMA_CHN_CPU = (12U),
#if defined(RN831x_RN861x_MCU_V3)
    LL_SYSC_DMA_CHN_UART6 = (13U),
    LL_SYSC_DMA_CHN_UART7 = (14U),
#endif

}eLL_SYSC_DMA_CHN_TypeDef;

typedef enum{
LL_SYSC_DMA_PRI0  = (0U), /* 优先级0最低 */
LL_SYSC_DMA_PRI1  = (1U), /* 优先级1 */
LL_SYSC_DMA_PRI2  = (2U), /* 优先级2 */
LL_SYSC_DMA_PRI3  = (3U), /* 优先级3最高 */
LL_SYSC_DMA_PRIMAX = (3U)
}eLL_SYSC_DMA_PRI_TypeDef;

#define LL_SYSC_DMA_PRI_PASSWORD 0XE
#define LL_SYSC_DMA_PRI_PASSWORD_ZONE 0XF

#elif defined (RN831x_RN861x_MCU_V4)
typedef enum
{
    CRC1_CKE          = (14U),
    SYSTC_CKE         = (13U),
    LCSS_CKE          = (12U),
    GPIO_CKE          = (11U), // core0和core1是或的关系，有一个开了就开
    UART38K_CKE       = (10U), //
    GDMA01_CKE        = (9U),
    GDMA00_CKE        = (8U),
    GIC_CKE           = (4U),
    SCU_CKE           = (3U),
    AOS_CKE           = (2U),
    MAILBOX_CKE       = (1U),
    WDT_CKE           = (0U),

    KBI0_CKE          = (32U + 31U),
    KBI0_15_CKE       = (32U + 15U),
    KBI0_14_CKE       = (32U + 14U),
    KBI0_13_CKE       = (32U + 13U),
    KBI0_12_CKE       = (32U + 12U),
    KBI0_11_CKE       = (32U + 11U),
    KBI0_10_CKE       = (32U + 10U),
    KBI0_9_CKE        = (32U + 9U),
    KBI0_8_CKE        = (32U + 8U),
    KBI0_7_CKE        = (32U + 7U),
    KBI0_6_CKE        = (32U + 6U),
    KBI0_5_CKE        = (32U + 5U),
    KBI0_4_CKE        = (32U + 4U),
    KBI0_3_CKE        = (32U + 3U),
    KBI0_2_CKE        = (32U + 2U),
    KBI0_1_CKE        = (32U + 1U),
    KBI0_0_CKE        = (32U + 0U),

    KBI1_CKE          = (64U + 31U),
    KBI1_15_CKE       = (64U + 15U),
    KBI1_14_CKE       = (64U + 14U),
    KBI1_13_CKE       = (64U + 13U),
    KBI1_12_CKE       = (64U + 12U),
    KBI1_11_CKE       = (64U + 11U),
    KBI1_10_CKE       = (64U + 10U),
    KBI1_9_CKE        = (64U + 9U),
    KBI1_8_CKE        = (64U + 8U),
    KBI1_7_CKE        = (64U + 7U),
    KBI1_6_CKE        = (64U + 6U),
    KBI1_5_CKE        = (64U + 5U),
    KBI1_4_CKE        = (64U + 4U),
    KBI1_3_CKE        = (64U + 3U),
    KBI1_2_CKE        = (64U + 2U),
    KBI1_1_CKE        = (64U + 1U),
    KBI1_0_CKE        = (64U + 0U),

    TC1_CKE            = (96 + 29U),
    TC0_CKE            = (96 + 28U),
    DAC_CKE            = (96 + 27U),
    ADC0_CKE           = (96 + 26U),
    D2F_CKE            = (96 + 25U),
    M2M_CKE            = (96 + 24U),
    DSP_CKE            = (96 + 22U),
    LCD_CKE            = (96 + 21U),
    IOCNT_CKE          = (96 + 20U),
    CAN_CKE            = (96 + 19U),
    I2C1_CKE           = (96 + 18U),
    I2C0_CKE           = (96 + 17U),
    SPI3_CKE           = (96 + 16U),
    SPI2_CKE           = (96 + 15U),
    SPI1_CKE           = (96 + 14U),
    SPI0_CKE           = (96 + 13U),
    ISO7816A_CKE       = (96 + 12U),
    ISO7816B_CKE       = (96 + 11U),
    USART1_CKE         = (96 + 10U),
    USART0_CKE         = (96 + 9U),
    LPUART_CKE         = (96 + 8U),
    UART7_CKE          = (96 + 7U),
    UART6_CKE          = (96 + 6U),
    UART5_CKE          = (96 + 5U),
    UART4_CKE          = (96 + 4U),
    UART3_CKE          = (96 + 3U),
    UART2_CKE          = (96 + 2U),
    UART1_CKE          = (96 + 1U),
    UART0_CKE          = (96 + 0U),

    CRC0_CKE           = (128U + 10U),
    LVDCMP_CKE         = (128U + 9U),
    LSRX_CKE           = (128U + 6U),
    ANA_CKE            = (128U + 4U),
    SIMP_TC3_CKE       = (128U + 3U),
    SIMP_TC2_CKE       = (128U + 2U),
    SIMP_TC1_CKE       = (128U + 1U),
    SIMP_TC0_CKE       = (128U + 0U),

    AES_CKE            = (160U + 8U),
    KEYSTORE_CKE       = (160U + 7U),
    SEA_SHA_CKE        = (160U + 6U),
    SEA_TRNG_CKE       = (160U + 5U),
    SEA_AES_CKE        = (160U + 4U),
    SEA_PKE_CKE        = (160U + 3U),
    SEA_BUS_CKE        = (160U + 2U),
    QSPI1_CKE          = (160U + 1U),
    QSPI0_CKE          = (160U + 0U)

} eModuleID_TypeDef;

typedef enum
{
    CRC1_RST          = (6U),
    SYSTC_RST         = (5U),
    GPIO_RST          = (4U), // core0和core1是或的关系，有一个开了就开
    GDMA01_RST        = (3U),
    GDMA00_RST        = (2U),
    AOS_RST           = (1U),
    MAILBOX_RST       = (0U),

    TC1_RST            = (32 + 29U),
    TC0_RST            = (32 + 28U),
    DAC_RST            = (32 + 27U),
    ADC0_RST           = (32 + 26U),
    D2F_RST            = (32 + 25U),
    M2M_RST            = (32 + 24U),
    DSP_RST            = (32 + 22U),
    LCD_RST            = (32 + 21U),
    IOCNT_RST          = (32 + 20U),
    I2C1_RST           = (32 + 18U),
    I2C0_RST           = (32 + 17U),
    SPI3_RST           = (32 + 16U),
    SPI2_RST           = (32 + 15U),
    SPI1_RST           = (32 + 14U),
    SPI0_RST           = (32 + 13U),
    ISO78161_RST       = (32 + 12U),
    ISO78160_RST       = (32 + 11U),
    USART1_RST         = (32 + 10U),
    USART0_RST         = (32 + 9U),
    LPUART_RST         = (32 + 8U),
    UART7_RST          = (32 + 7U),
    UART6_RST          = (32 + 6U),
    UART5_RST          = (32 + 5U),
    UART4_RST          = (32 + 4U),
    UART3_RST          = (32 + 3U),
    UART2_RST          = (32 + 2U),
    UART1_RST          = (32 + 1U),
    UART0_RST          = (32 + 0U),

    CRC0_RST           = (64U + 10U),
    LVDCMP_RST         = (64U + 9U),
    KBI1_RST           = (64U + 8U),
    KBI0_RST           = (64U + 7U),
    LSRX_RST           = (64U + 6U),
    ANA_RST            = (64U + 4U),
    SIMP_TC3_RST       = (64U + 3U),
    SIMP_TC2_RST       = (64U + 2U),
    SIMP_TC1_RST       = (64U + 1U),
    SIMP_TC0_RST       = (64U + 0U),

    AES_RST            = (96U + 8U),
    SEA_RST            = (96U + 2U),
    QSPI1_RST          = (96U + 1U),
    QSPI0_RST          = (96U + 0U)

} eLL_SYSC_RSTRqst_TypeDef;

#elif defined(RN202x_RN7326_SOC_V2)
typedef enum
{
    LL_SYSC_TC0_ID = (4U),
    LL_SYSC_TC1_ID = (5U),
    LL_SYSC_UART4_ID = (6U),
    LL_SYSC_UART5_ID = (7U),
    LL_SYSC_UART0_ID = (8U),
    LL_SYSC_UART1_ID = (9U),
    LL_SYSC_UART2_ID = (10U),
    LL_SYSC_UART3_ID = (11U),
    LL_SYSC_UART38K_ID = (12U),
    LL_SYSC_ISO7816_ID = (13U),
    LL_SYSC_I2C_ID = (14U),
    LL_SYSC_SPI0_ID = (15U),
    LL_SYSC_SPI1_ID = (16U),
    LL_SYSC_SPI2_ID = (17U),
    LL_SYSC_SPI3_ID = (18U),
    LL_SYSC_SPI4_ID = (19U),
    LL_SYSC_SIMPTC_ID = (20U),

    LL_SYSC_LPUART_ID = (25U),
    LL_SYSC_CRC_ID = (26U),
    LL_SYSC_CAN_ID = (27U),
    LL_SYSC_I2C1_ID = (28U),

    LL_SYSC_M2M_ID = (36U),    /* mode1 3 + 32 = 35 */
    LL_SYSC_GPIO_ID = (37U),    /* mode1 5 + 32 */
    LL_SYSC_DSP_ID = (38U), /* mode1 6 + 32*/
    LL_SYSC_EMU_ID = (39U),     /* mode1 7 + 32 */
    LL_SYSC_NVM_ID = (40U),     /* mode1 8 + 32 */
    LL_SYSC_WDT_ID = (41U),    /*  mode1 9 + 32 */
    LL_SYSC_VBAT_ID = (42U),     /* mode1 10 + 32  */
    LL_SYSC_CMPLVD_ID = (43U),     /* mode1 11 + 32  */

    LL_SYSC_ECT_ID = (45U), /* mode1 13 + 32*/
    LL_SYSC_IOCNT_ID = (46U), /* mode1 14 + 32*/
    LL_SYSC_GPADC_ID = (47U), /* mode1 15 + 32*/
    LL_SYSC_FLK_ID = (48U), /* mode1 16 + 32*/

    LL_SYSC_WAVE_ID = (50U), /* mode1 19 + 32*/

    LL_SYSC_INTC0_ID = (64U), /* INT_EN 0 + 64*/
    LL_SYSC_INTC1_ID = (65U), /* INT_EN 1 + 64*/
    LL_SYSC_INTC2_ID = (66U), /* INT_EN 2 + 64*/
    LL_SYSC_INTC3_ID = (67U), /* INT_EN 3 + 64*/
    LL_SYSC_INTC4_ID = (68U), /* INT_EN 4 + 64*/
    LL_SYSC_INTC5_ID = (69U), /* INT_EN 5 + 64*/
    LL_SYSC_INTC6_ID = (70U), /* INT_EN 6 + 64*/
    LL_SYSC_INTC7_ID = (71U), /* INT_EN 7 + 64*/
    LL_SYSC_INTC_ID = (72U), /* INT_EN 8 + 64*/

    LL_SYSC_KBI0_ID = (96U),  /* KBI_EN 0 + 96*/
    LL_SYSC_KBI1_ID = (97U),  /* KBI_EN 1 + 96*/
    LL_SYSC_KBI2_ID = (98U),  /* KBI_EN 2 + 96*/
    LL_SYSC_KBI3_ID = (99U),  /* KBI_EN 3 + 96*/
    LL_SYSC_KBI4_ID = (100U), /* KBI_EN 4 + 96*/
    LL_SYSC_KBI5_ID = (101U), /* KBI_EN 5 + 96*/
    LL_SYSC_KBI6_ID = (102U), /* KBI_EN 6 + 96*/
    LL_SYSC_KBI7_ID = (103U), /* KBI_EN 7 + 96*/
    LL_SYSC_KBI_ID = (104U), /* KBI_EN 8 + 96*/
} eModuleID_TypeDef;

typedef enum
{
    LL_SYSC_PD_VSELBOR5 = (0U),

    LL_SYSC_PD_LVD = (3U),
    LL_SYSC_PD_CMP0 =(4U),
    LL_SYSC_PD_CMP1 = (5U),

    LL_SYSC_PD_ADCIA = (16U),
    LL_SYSC_PD_ADCIB = (17U),
    LL_SYSC_PD_ADCIC = (18U),
    LL_SYSC_PD_ADCUA = (19U),
    LL_SYSC_PD_ADCUB = (20U),
    LL_SYSC_PD_ADCUC = (21U),
    LL_SYSC_PD_ADCIN = (22U),
    LL_SYSC_PD_VREFA = (23U),

    LL_SYSC_PD_LDO3_ID = (25U),
}eLL_SYSC_PDModuleID_TypeDef;


typedef enum
{
    LL_SYSC_ADCCHN_IA = (0U),
    LL_SYSC_ADCCHN_IB = (1U),
    LL_SYSC_ADCCHN_IC = (2U),
    LL_SYSC_ADCCHN_UA = (3U),
    LL_SYSC_ADCCHN_UB =(4U),
    LL_SYSC_ADCCHN_UC =(5U)
}eLL_SYSC_ADCChannel_TypeDef;

typedef enum
{
    LL_SYSC_ADCPGA_1 = (0U),
    LL_SYSC_ADCPGA_2 = (1U),
    LL_SYSC_ADCPGA_4 = (2U),
    LL_SYSC_ADCPGA_8 = (2U),
    LL_SYSC_ADCPGA_16 = (3U),
    LL_SYSC_ADCPGA_MAX = (7U),
}eLL_SYSC_ADCPGA_TypeDef;

typedef enum
{
    LL_SYSC_RSTRQST_EMU = (0U),
    LL_SYSC_RSTRQST_EMUREG = (2U),
    LL_SYSC_RSTRQST_FLK = (4U),

    LL_SYSC_RSTRQST_CM0 = (32U),
    LL_SYSC_RSTRQST_DSP = (33U),
    LL_SYSC_RSTRQST_ECT = (34U),
    LL_SYSC_RSTRQST_I2C0 = (35U),

    LL_SYSC_RSTRQST_I2C1 = (45U),
    LL_SYSC_RSTRQST_RISO = (46U),
    LL_SYSC_RSTRQST_CAN = (47U),

}eLL_SYSC_RSTRqst_TypeDef;

typedef enum{

    LL_SYSC_RST_INVALID = (0U),
    LL_SYSC_RST_VALID = (1U)
}eLL_SYSC_RST_Status_TypeDef;

typedef enum
{
    LL_SYSC_RSTFLAG_POWEROK = (5U),
    LL_SYSC_RSTFLAG_PIN = (6U),
    LL_SYSC_RSTFLAG_WDT = (7U),
    LL_SYSC_RSTFLAG_MCU =(8U),
    LL_SYSC_RSTFLAG_BORV2P8 = (11U),
    LL_SYSC_RSTFLAG_CM0 = (13U),

    LL_SYSC_RSTFLAG_BOR5 = (44U),
}eLL_SYSC_RSTFlag_TypeDef;

typedef enum
{
    LL_SYSC_RSTCONFG_LOCKUP = (2U),
    LL_SYSC_RSTCONFG_BORV2P8 = (9U),

    LL_SYSC_RSTCONFG_UART0 = (36U),
    LL_SYSC_RSTCONFG_UART1 = (37U),
    LL_SYSC_RSTCONFG_UART2 = (38U),
    LL_SYSC_RSTCONFG_UART3 = (39U),
    LL_SYSC_RSTCONFG_UART4 = (40U),
    LL_SYSC_RSTCONFG_UART5 = (41U),

    LL_SYSC_RSTCONFG_BOR5IE = (42U),
    LL_SYSC_RSTCONFG_BOR5RST = (43U),

}eLL_SYSC_RSTConfg_TypeDef;

typedef enum{
    LL_SYSC_DMA_CHN_CACHE = (0U),
    LL_SYSC_DMA_CHN_EMU1 = (1U),
    LL_SYSC_DMA_CHN_SPI0 = (2U),
    LL_SYSC_DMA_CHN_SPI1 = (3U),
    LL_SYSC_DMA_CHN_UART2 = (4U),
    LL_SYSC_DMA_CHN_SPI2 = (5U),
    LL_SYSC_DMA_CHN_SPI3 = (6U),
    LL_SYSC_DMA_CHN_SPI4 = (7U),
    LL_SYSC_DMA_CHN_DSP = (8U),
    LL_SYSC_DMA_CHN_SRAM = (9U),
    LL_SYSC_DMA_CHN_EMU2 = (10U),

    LL_SYSC_DMA_CHN_UART0 = (11U),
    LL_SYSC_DMA_CHN_UART1 = (12U),

    LL_SYSC_DMA_CHN_UART3 = (13U),
    LL_SYSC_DMA_CHN_UART4 = (14U),
    LL_SYSC_DMA_CHN_UART5 = (15U),
    LL_SYSC_DMA_CHN_CAN = (16U),
    LL_SYSC_DMA_CHN_TC1 = (17U),

    LL_SYSC_DMA_CHN_CRC = (18U),
    LL_SYSC_DMA_CHN_M2M = (19U),
    LL_SYSC_DMA_CHN_EMU3 = (20U),

}eLL_SYSC_DMA_CHN_TypeDef;




typedef enum{

LL_SYSC_DMA_PRI0  = (0U), /* 优先级0最低 */
LL_SYSC_DMA_PRI1  = (1U), /* 优先级1 */
LL_SYSC_DMA_PRI2  = (2U), /* 优先级2 */
LL_SYSC_DMA_PRI3  = (3U), /* 优先级3最高 */
LL_SYSC_DMA_PRIMAX = (3U)
}eLL_SYSC_DMA_PRI_TypeDef;

#define LL_SYSC_DMA_PRI_PASSWORD 0XE
#define LL_SYSC_DMA_PRI_PASSWORD_ZONE 0XF
#endif

#if defined(RN831x_RN861x_MCU_V4)
/*--------------------------使能各模块apb时钟-----------------------------------*/
#define UART38K_CLK_EN                                      \
    {                                                       \
        BITBAND_ADDR((uint32_t)(&SYSTEM->PMOD0_CKE), 10U) = 1U; \
    }
#define UART38K_CLK_DIS                                     \
    {                                                       \
        BITBAND_ADDR((uint32_t)(&SYSTEM->PMOD0_CKE), 10U) = 0U; \
    }
#define GPIO_CLK_EN                                        \
    {                                                      \
        BITBAND_ADDR((uint32_t)(&SYSTEM->PMOD0_CKE), 11U) = 1U; \
    }
#define GPIO_CLK_DIS                                       \
    {                                                      \
        BITBAND_ADDR((uint32_t)(&SYSTEM->PMOD0_CKE), 11U) = 0U; \
    }
#define RTC_SAR_CLK_EN                                      \
    {                                                       \
        BITBAND_ADDR((uint32_t)(&SYSTEM->MOD1_CKE), 4U) = 1U; \
    }
#define RTC_SAR_CLK_DIS                                     \
    {                                                       \
        BITBAND_ADDR((uint32_t)(&SYSTEM->MOD1_CKE), 4U) = 0U; \
    }

/* 模块电源开启SYSCTL->SYS_PD --------------------------------*/
#define LVD_PD_POWON                                      \
    {                                                     \
        (LVDCMP->LVDPWD) = 0U;                            \
    }
#define LVD_PD_POWDWN                                     \
    {                                                     \
        (LVDCMP->LVDPWD) = 0x55AAAA55U;                   \
    }
#define CMP1_PD_POWON                                     \
    {                                                     \
        (LVDCMP->CMP1PWD) = 0U;                           \
    }
#define CMP1_PD_POWDWN                                    \
    {                                                     \
        (LVDCMP->CMP1PWD) = 0xA5A55A5AU;                   \
    }
#define CMP2_PD_POWON                                     \
    {                                                     \
        (LVDCMP->CMP2PWD) = 0U;                         \
    }
#define CMP2_PD_POWDWN                                    \
    {                                                     \
        (LVDCMP->CMP2PWD) = 0xAA5555AAU;                   \
    }
#define PWD_CMP1R_EN                                      \
    {                                                     \
        BITBAND_ADDR((uint32_t)(&SYSTEM->MOD1_CKE), 9U) = 1U; \
    }
#define PWD_CMP1R_DIS                                     \
    {                                                     \
		BITBAND_ADDR((uint32_t)(&SYSTEM->MOD1_CKE), 9U) = 0U; \
    }
#define PWD_CMP2R_EN                                      \
    {                                                     \
        BITBAND_ADDR((uint32_t)(&SYSTEM->MOD1_CKE), 9U) = 1U; \
    }
#define PWD_CMP2R_DIS                                     \
    {                                                     \
        BITBAND_ADDR((uint32_t)(&SYSTEM->MOD1_CKE), 9U) = 0U; \
    }
#define HYSEN_CMP1_EN                                      \
    {                                                      \
        BITBAND_ADDR((uint32_t)(&LVDCMP->CMPCTRL), 2U) = 1U; \
    }
#define HYSEN_CMP1_DIS                                     \
    {                                                      \
        BITBAND_ADDR((uint32_t)(&LVDCMP->CMPCTRL), 2U) = 0U; \
    }
#define HYSEN_CMP2_EN                                      \
    {                                                      \
        BITBAND_ADDR((uint32_t)(&LVDCMP->CMPCTRL), 3U) = 1U; \
    }
#define HYSEN_CMP2_DIS                                     \
    {                                                      \
        BITBAND_ADDR((uint32_t)(&LVDCMP->CMPCTRL), 3U) = 0U; \
    }

/* ------------------------------------------------------------ */
#define DRIVECLOCK_SETHOSCFREQ(x)                                \
    {                                                            \
        SYSTEM->HOSC_CTRL &= 0xfffffff9U, SYSTEM->HOSC_CTRL |= (x); \
    }
#else
/*--------------------------使能各模块apb时钟-----------------------------------*/
#define UART38K_CLK_EN                                      \
    {                                                       \
        BITBAND_ADDR((uint32_t)(&SYSCTL->MOD0_EN), 12U) = 1U; \
    }
#define UART38K_CLK_DIS                                     \
    {                                                       \
        BITBAND_ADDR((uint32_t)(&SYSCTL->MOD0_EN), 12U) = 0U; \
    }

/*"SYSCTL->MOD1_EN"*/
#define GPIO_CLK_EN                                        \
    {                                                      \
        BITBAND_ADDR((uint32_t)(&SYSCTL->MOD1_EN), 5U) = 1U; \
    }
#define GPIO_CLK_DIS                                       \
    {                                                      \
        BITBAND_ADDR((uint32_t)(&SYSCTL->MOD1_EN), 5U) = 0U; \
    }

#define RTC_SAR_CLK_EN                                      \
    {                                                       \
        BITBAND_ADDR((uint32_t)(&SYSCTL->MOD1_EN), 10U) = 1U; \
    }
#define RTC_SAR_CLK_DIS                                     \
    {                                                       \
        BITBAND_ADDR((uint32_t)(&SYSCTL->MOD1_EN), 10U) = 0U; \
    }




/* 模块电源开启SYSCTL->SYS_PD --------------------------------*/
#define LVD_PD_POWON                                      \
    {                                                     \
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 3U) = 0U; \
    }
#define LVD_PD_POWDWN                                     \
    {                                                     \
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 3U) = 1U; \
    }
#define CMP1_PD_POWON                                     \
    {                                                     \
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 4U) = 0U; \
    }
#define CMP1_PD_POWDWN                                    \
    {                                                     \
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 4U) = 1U; \
    }
#define CMP2_PD_POWON                                     \
    {                                                     \
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 5U) = 0U; \
    }
#define CMP2_PD_POWDWN                                    \
    {                                                     \
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 5U) = 1U; \
    }
#define PWD_CMP1R_EN                                      \
    {                                                     \
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 8U) = 0U; \
    }
#define PWD_CMP1R_DIS                                     \
    {                                                     \
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 8U) = 1U; \
    }
#define PWD_CMP2R_EN                                      \
    {                                                     \
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 9U) = 0U; \
    }
#define PWD_CMP2R_DIS                                     \
    {                                                     \
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 9U) = 1U; \
    }
#define HYSEN_CMP1_EN                                      \
    {                                                      \
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 10U) = 1U; \
    }
#define HYSEN_CMP1_DIS                                     \
    {                                                      \
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 10U) = 0U; \
    }
#define HYSEN_CMP2_EN                                      \
    {                                                      \
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 11U) = 1U; \
    }
#define HYSEN_CMP2_DIS                                     \
    {                                                      \
        BITBAND_ADDR((uint32_t)(&SYSCTL->SYS_PD), 11U) = 0U; \
    }

/* ------------------------------------------------------------ */
#define DRIVECLOCK_SETHOSCFREQ(x)                                \
    {                                                            \
        SYSCTL->OSC_CTRL2 &= 0xfffffff8U, SYSCTL->OSC_CTRL2 |= (x); \
    }
#endif
/* Exported enum ------------------------------------------------------------*/
typedef enum
{
    HOSE_SET7372M = 0x0U,
    HOSE_SET1475M = 0x01U,
    HOSE_SET2949M = 0x03U,
    HOSE_SETNULL = 0x0U,
} eClockHoscFreq_TypeDef;

/* Exported enum ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
extern void LL_SYSC_ApbClkCtrl(eModuleID_TypeDef moduel_id, FunctionalState cmd);
#if !defined (RN831x_RN861x_MCU_V4)
extern void LL_SYSC_AnaPowerCtrl(eLL_SYSC_PDModuleID_TypeDef pdmoduel_id, FunctionalState cmd);
#endif
#if defined(RN202x_RN7326_SOC_V2) || defined(RN821x_RN721x_SOC_V3) || defined(RN821x_RN721x_SOC_V2) \
|| defined(RN821x_RN721x_SOC_B)|| defined(RN821x_RN721x_SOC_C)|| defined(RN821x_RN721x_SOC_D)
extern void LL_SYSC_ADCPGACtrl(eLL_SYSC_ADCChannel_TypeDef adc_channel, eLL_SYSC_ADCPGA_TypeDef adc_pag);
#endif
#if !defined (RN831x_RN861x_MCU_V4)
extern void LL_SYSC_SysRstFlagClr(eLL_SYSC_RSTFlag_TypeDef sysc_rst_id);
extern eLL_SYSC_RST_Status_TypeDef LL_SYSC_SysRstFlagGet(eLL_SYSC_RSTFlag_TypeDef sysc_rst_id);
extern void LL_SYSC_SysRstCfg(eLL_SYSC_RSTConfg_TypeDef sysc_rst_id);
extern void LL_SYSC_SysRstRqst(eLL_SYSC_RSTRqst_TypeDef sysc_rst_id, FunctionalState cmd);
#endif
#if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_B)|| defined(RN821x_RN721x_SOC_C)|| defined(RN821x_RN721x_SOC_D)
extern void LL_SYSC_SysRstRqst(eLL_SYSC_RSTRqst_TypeDef anamoduel_id, FunctionalState cmd);
#endif
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) \
    || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3) \
    || defined(RN202x_RN7326_SOC_B) || defined(RN202x_RN7326_SOC_V2)
extern void LL_SYSC_DMAPriSet(eLL_SYSC_DMA_CHN_TypeDef dam_chn,eLL_SYSC_DMA_PRI_TypeDef dma_pri);
#endif


#endif /*_SYSC_H*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
