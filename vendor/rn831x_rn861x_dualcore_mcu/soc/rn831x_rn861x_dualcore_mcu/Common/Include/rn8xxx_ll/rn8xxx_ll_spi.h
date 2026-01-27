/******************************************************************************
 * @file    Rn8xxxSpi.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-09-07
 * @brief   Header file containing functions prototypes of Spi HAL library.
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
#ifndef Rn8xxx_SPI_H_
#define Rn8xxx_SPI_H_

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll_def.h"
/* Exported define ------------------------------------------------------------*/
#define SPI_TIMEOUT 0x3ff

#define SPI_DMAAdrLen_MASK 0x1ffff
#define SPIS_MAX_DEPTH 32
#define SPICTL_DEFAULT 0x02  /*"spi_CTL 寄存器复位默认值"*/

/*********************************************************************************/
#ifdef RN831x_RN861x_MCU_V4
#define GET_REG_SPI_CTRL(spi, databits) ((spi)->CTRL & GET_DATA_MASK(databits))
#define GET_REG_SPI_STA(spi, databits) ((spi)->STA & GET_DATA_MASK(databits))
#define GET_REG_SPI_TX(spi, databits) ((spi)->TXDATA & GET_DATA_MASK(databits))
#define GET_REG_SPI_RX(spi, databits) ((spi)->RXDATA & GET_DATA_MASK(databits))
#define GET_REG_SPI_TXDFLT(spi, databits) ((spi)->TXDFLT & GET_DATA_MASK(databits))

#define SET_REG_SPI_CTRL(spi, data) ((spi)->CTRL = (data))
#define SET_REG_SPI_STA(spi, data) ((spi)->STA = (data))
#define SET_REG_SPI_TX(spi, data) ((spi)->TXDATA = (data))
#define SET_REG_SPI_RX(spi, data) ((spi)->RXDATA = (data))
#define SET_REG_SPI_TXDFLT(spi, data) ((spi)->TXDFLT = (data))
/*********************************************************************************/

#define GET_BIT_SPI_CTRL(spi, bit) (((spi)->CTRL & (bit)) ? (1) : (0))
#define GET_BIT_SPI_STA(spi, bit) (((spi)->STA & (bit)) ? (1) : (0))
#define GET_BIT_SPI_TX(spi, bit) (((spi)->TXDATA & (bit)) ? (1) : (0))
#define GET_BIT_SPI_RX(spi, bit) (((spi)->RXDATA & (bit)) ? (1) : (0))
#define GET_BIT_SPI_TXDFLT(spi, bit) (((spi)->TXDFLT & (bit)) ? (1) : (0))

#define SET_BIT_SPI_CTRL(spi, bit) ((spi)->CTRL |= (bit))
#define SET_BIT_SPI_STA(spi, bit) ((spi)->STA = (bit))
#define SET_BIT_SPI_TX(spi, bit) ((spi)->TXDATA |= (bit))
#define SET_BIT_SPI_RX(spi, bit) ((spi)->RXDATA |= (bit))
#define SET_BIT_SPI_TXDFLT(spi, bit) ((spi)->TXDFLT |= (bit))

#define CLR_BIT_SPI_CTRL(spi, bit) ((spi)->CTRL &= (bit))
#define CLR_BIT_SPI_STA(spi, bit) (SET_BIT_SPI_STA((spi), (bit)))
#else
#define GET_REG_SPI_CTRL(spi, databits) ((spi)->CTRL & GET_DATA_MASK(databits))
#define GET_REG_SPI_STA(spi, databits) ((spi)->STA & GET_DATA_MASK(databits))
#define GET_REG_SPI_TX(spi, databits) ((spi)->TX & GET_DATA_MASK(databits))
#define GET_REG_SPI_RX(spi, databits) ((spi)->RX & GET_DATA_MASK(databits))
#define GET_REG_SPI_TXDFLT(spi, databits) ((spi)->TXDFLT & GET_DATA_MASK(databits))

#define SET_REG_SPI_CTRL(spi, data) ((spi)->CTRL = (data))
#define SET_REG_SPI_STA(spi, data) ((spi)->STA = (data))
#define SET_REG_SPI_TX(spi, data) ((spi)->TX = (data))
#define SET_REG_SPI_RX(spi, data) ((spi)->RX = (data))
#define SET_REG_SPI_TXDFLT(spi, data) ((spi)->TXDFLT = (data))
/*********************************************************************************/

#define GET_BIT_SPI_CTRL(spi, bit) (((spi)->CTRL & (bit)) ? (1) : (0))
#define GET_BIT_SPI_STA(spi, bit) (((spi)->STA & (bit)) ? (1) : (0))
#define GET_BIT_SPI_TX(spi, bit) (((spi)->TX & (bit)) ? (1) : (0))
#define GET_BIT_SPI_RX(spi, bit) (((spi)->RX & (bit)) ? (1) : (0))
#define GET_BIT_SPI_TXDFLT(spi, bit) (((spi)->TXDFLT & (bit)) ? (1) : (0))

#define SET_BIT_SPI_CTRL(spi, bit) ((spi)->CTRL |= (bit))
#define SET_BIT_SPI_STA(spi, bit) ((spi)->STA = (bit))
#define SET_BIT_SPI_TX(spi, bit) ((spi)->TX |= (bit))
#define SET_BIT_SPI_RX(spi, bit) ((spi)->RX |= (bit))
#define SET_BIT_SPI_TXDFLT(spi, bit) ((spi)->TXDFLT |= (bit))

#define CLR_BIT_SPI_CTRL(spi, bit) ((spi)->CTRL &= (bit))
#define CLR_BIT_SPI_STA(spi, bit) (SET_BIT_SPI_STA((spi), (bit)))
#endif
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) || \
	defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3) || \
	defined(RN202x_RN7326_SOC_B)  || defined(RN202x_RN7326_SOC_V2) || \
	defined(RN831x_RN861x_MCU_V4)
/*********************************************************************************/
#define GET_REG_SPI_DMA_CTRL(spi, databits) ((spi)->DMA_CTRL & GET_DATA_MASK(databits))
#define GET_REG_SPI_DMA_TBADR(spi, databits) ((spi)->DMA_TBADR & GET_DATA_MASK(databits))
#define GET_REG_SPI_DMA_RBADR(spi, databits) ((spi)->DMA_RBADR & GET_DATA_MASK(databits))
#define GET_REG_SPI_DMA_TLEN(spi, databits) ((spi)->DMA_TLEN & GET_DATA_MASK(databits))
#define GET_REG_SPI_DMA_RLEN(spi, databits) ((spi)->DMA_RLEN & GET_DATA_MASK(databits))
#define GET_REG_SPI_DMA_TADR(spi, databits) ((spi)->DMA_TADR & GET_DATA_MASK(databits))
#define GET_REG_SPI_DMA_RADR(spi, databits) ((spi)->DMA_RADR & GET_DATA_MASK(databits))
#define GET_REG_SPI_DMA_IE(spi, databits) ((spi)->DMA_IE & GET_DATA_MASK(databits))
#define GET_REG_SPI_DMA_FLG(spi, databits) ((spi)->DMA_FLG & GET_DATA_MASK(databits))

#define SET_REG_SPI_DMA_CTRL(spi, data) ((spi)->DMA_CTRL = (data))
#define SET_REG_SPI_DMA_TBADR(spi, data) ((spi)->DMA_TBADR = (data))
#define SET_REG_SPI_DMA_RBADR(spi, data) ((spi)->DMA_RBADR = (data))
#define SET_REG_SPI_DMA_TLEN(spi, data) ((spi)->DMA_TLEN = (data))
#define SET_REG_SPI_DMA_RLEN(spi, data) ((spi)->DMA_RLEN = (data))
#define SET_REG_SPI_DMA_TADR(spi, data) ((spi)->DMA_TADR = (data))
#define SET_REG_SPI_DMA_RADR(spi, data) ((spi)->DMA_RADR = (data))
#define SET_REG_SPI_DMA_IE(spi, data) ((spi)->DMA_IE = (data))
#define SET_REG_SPI_DMA_FLG(spi, data) ((spi)->DMA_FLG = (data))
/*********************************************************************************/

#define GET_BIT_SPI_DMA_CTRL(spi, bit) (((spi)->DMA_CTRL & (bit)) ? (1u) : (0u))
#define GET_BIT_SPI_DMA_IE(spi, bit) (((spi)->DMA_IE & (bit)) ? (1u) : (0u))
#define GET_BIT_SPI_DMA_FLG(spi, bit) (((spi)->DMA_FLG & (bit)) ? (1u) : (0u))

#define SET_BIT_SPI_DMA_CTRL(spi, bit) (SET_REG32_BIT((spi)->DMA_CTRL, (bit)))
#define SET_BIT_SPI_DMA_IE(spi, bit) (SET_REG32_BIT((spi)->DMA_IE, (bit)))
#define SET_BIT_SPI_DMA_FLG(spi, bit) ((spi)->DMA_FLG = (bit))

#define CLR_BIT_SPI_DMA_CTRL(spi, bit) (CLR_REG32_BIT((spi)->DMA_CTRL, (bit)))
#define CLR_BIT_SPI_DMA_IE(spi, bit) (CLR_REG32_BIT((spi)->DMA_IE, (bit)))

#define SET_BIT_TO_CLR_SPI_DMA_FLG(spi, bit) ((spi)->DMA_FLG = (bit))
#endif
/* Exported enum ------------------------------------------------------------*/
#if defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_C)   
typedef enum
{
    SPI0_ID = 0,   
} eSPI_ID_TypeDef;

#elif defined(RN821x_RN721x_SOC_D)  \
    || defined(RN831x_RN861x_MCU_V3)    
typedef enum
{
    SPI0_ID = 0,
    SPI1_ID = 1,     
} eSPI_ID_TypeDef;
#elif defined(RN821x_RN721x_SOC_V2)  \
    || defined(RN831x_RN861x_MCU_V2) \
	|| defined(RN831x_RN861x_MCU_V4)
typedef enum
{
    SPI0_ID = 0,
    SPI1_ID = 1,
    SPI2_ID = 2,
    SPI3_ID = 3,
} eSPI_ID_TypeDef;
#elif defined(RN821x_RN721x_SOC_V3)      
typedef enum
{   
    SPI1_ID = 1,   
    SPI3_ID = 3,    
} eSPI_ID_TypeDef;
#elif defined(RN202x_RN7326_SOC_B) ||defined(RN202x_RN7326_SOC_V2)
typedef enum
{    
    SPI1_ID = 1,   
    SPI3_ID = 3,
    SPI4_ID = 4,   
} eSPI_ID_TypeDef;
#endif

typedef enum
{
    SPICTL_SLV_TX_ADV_DIS = 0,  /* SPI 从机低速模式，fcpu/8，向下兼容 */
    SPICTL_SLV_TX_ADV_EN = 1,   /* SPI 从机高速模式，从机速率可达到 fcpu/4 */

    SPICTL_SCSN_POS_IRQDIS = 0, /* SCSN_POS 中断使能关闭  */
    SPICTL_SCSN_POS_IRQEN = 1,  /* SCSN_POS 中断使能打开 */    

    SPICTL_SCSN_NEG_IRQDIS = 0, /* SCSN_NEG 中断使能关闭 */
    SPICTL_SCSN_NEG_IRQEN = 1,  /* SCSN_NEG 中断使能打开 */

    SPICTL_TXEMPY_IRQDIS = 0,   /* TXEMPT 中断使能关闭 */
    SPICTL_TXEMPY_IRQEN = 1,    /* TXEMPT 中断使能打开 */

    SPICTL_TX_DFLT_DIS = 0,     /* 发送 BUF 空时,发送上次传输 SPI_TXDATA 的值 */
    SPICTL_TX_DFLT_EN = 1,      /* 发送 BUF 空时,发送 SPI_TXDFLT 寄存器值 */

    SPICTL_WIDTH_8BIT = 0,      /* 数据宽度选择8bit */
    SPICTL_WIDTH_16BIT = 1,     /* 数据宽度选择16bit */
    SPICTL_WIDTH_32BIT = 2,     /* 数据宽度选择32bit */
    SPICTL_WIDTH_RSVBIT = 3,    /* 数据宽度选择 预留 8bit */

    SPICTL_SCSN_DIS = 0,        /* 不使能主模式 SCSN 模式错误检测，SCSN 为 通用 IO */
    SPICTL_SCSN_EN = 1,         /* 使能主模式 SCSN 模式错误检测，SCSN 作为 SPI 的输入 */

    SPICTL_CPH = 0,             /* 时钟相位选择:前边沿采样数据 */
    SPICTL_CPA = 1,             /* 时钟相位选择:前边沿建立数据 */

    SPICTL_CPOL = 0,            /* 时钟极性选择:“SCK”在空闲状态时被设置为低电平 */
    SPICTL_CPOH = 1,            /* 时钟极性选择:“SCK”在空闲状态时被设置为高电平 */

    SPICTL_MSB = 0,             /* MSB 先传输 */
    SPICTL_LSB = 1,             /* LSB 先传输 */

    SPICTL_TXCOL_IRQ_DIS = 0,   /* 数据写冲突中断使能关闭 */
    SPICTL_TXCOL_IRQ_EN = 1,    /* 数据写冲突中断使能使能 */

    SPICTL_RXCOL_IRQ_DIS = 0,   /* 数据读冲突中断使能关闭 */
    SPICTL_RXCOL_IRQ_EN = 1,    /* 数据读冲突中断使能使能 */

    SPICTL_ERR_IRQ_DIS = 0,     /* SCSN 模式错误中断使能打开 */
    SPICTL_ERR_IRQ_EN = 1,      /* SCSN 模式错误中断使能关闭*/

    SPICTL_TR_IRQ_DIS = 0,      /* 关闭发送数据中断 */
    SPICTL_TR_IRQ_EN = 1,       /* 打开发送数据中断*/

    SPICTL_SLAV = 0,            /* 从机 */
    SPICTL_MAST = 1,            /* 主机 */

    SPICTL_DIS = 0,             /* 关闭 SPI 接口 */
    SPICTL_EN = 1,              /* 打开 SPI 接口 */
} eSPICtl_TypeDef;              /* SPI 控制寄存器 SPI_CTL选择 */

typedef enum
{                       /*"BIT7:BIT8"*/
  SPICTL_MODE_OO = 0,   /*" 上升沿，数据采样;下降沿，数据建立"*/
  SPICTL_MODE_01 = 1,   /*" 上升沿，数据建立;下降沿，数据采样  "*/
  SPICTL_MODE_10 = 2,   /*" 下降沿，数据采样;上升沿，数据建立  "*/
  SPICTL_MODE_11 = 3,   /*" 下降沿，数据建立;上升沿，数据采样  "*/
} eSPICtl_Mode_TypeDef; /*"CPOL时钟极性/CPHA时钟相位选择"*/

typedef enum
{
    SPIDMA_TX_EN = 0x01,       /*"发送 DMA 使能"*/
    SPIDMA_RX_EN = 0x02,       /*"接收 DMA 使能"*/
    SPIDMA_TX_CYC_MODE = 0x04, /*"发送循环模式使能"*/
    SPIDMA_RX_CYC_MODE = 0x08, /*"接收循环模式使能"*/
} eSPIDMA_CtlPara_TypeDef;

typedef enum
{
    SPIDMA_TX_HIE = 0x01,    /*"BIT0:DMA发送半满中断使能"*/
    SPIDMA_TX_FIE = 0x02,    /*"BIT1:DMA发送全满中断使能"*/
    SPIDMA_RX_HIE = 0x04,    /*"BIT2:DMA接收半满中断使能"*/
    SPIDMA_RX_FIE = 0x08,    /*"BIT3:DMA接收全满中断使能"*/
    SPIDMA_TX_ERR_IE = 0x10, /*"BIT4:发送数据错误中断使能"*/
    SPIDMA_RX_ERR_IE = 0x20, /*"BIT5:接收数据覆盖中断使能"*/
} eSPIDMA_IE_TypeDef;

typedef enum
{
    SPIDMA_FLAG_TX_HDONE = (1u << 0), /*"DMA发送半满中断"*/
    SPIDMA_FLAG_TX_FDONE = (1u << 1), /*"DMA发送全满中断"*/
    SPIDMA_FLAG_RX_HDONE = (1u << 2), /*"DMA接收半满中断"*/
    SPIDMA_FLAG_RX_FDONE = (1u << 3), /*"DMA接收全满中断"*/
    SPIDMA_FLAG_Reverse = (1u << 4),  /*"保留"*/
    SPIDMA_FLAG_RX_ERR = (1u << 5),   /*"接收数据覆盖"*/
} eSPIDMA_FLAG_Typedef;

/* Exported types ------------------------------------------------------------*/
typedef union
{
    struct
    {
        uint32_t EN : 1;
        uint32_t MAST_SLAV : 1;
        uint32_t TR_IRQ_EN : 1;
        uint32_t ERR_IRQ_EN : 1;
        uint32_t RXCOL_IRQ_EN : 1;
        uint32_t TXCOL_IRQ_EN : 1;
        uint32_t LMSB : 1;
        uint32_t CPOL : 1;
        uint32_t CPHA : 1;
        uint32_t SCSN_EN : 1;
        uint32_t WIDTH : 2;
        uint32_t CLKDIV : 8;
        uint32_t Reverse : 2; /*保留位，默认为 0，不要写 1*/
        uint32_t TX_DFLT_EN : 1;
        uint32_t TX_EMPT_IRQ_EN : 1;
        uint32_t SCSN_NEG_IRQ_EN : 1;
        uint32_t SCSN_POS_IRQ_EN : 1;
        uint32_t SLV_TX_ADV : 1;
    }bitSpiCtrl;
    uint32_t SpiCtrl;
} uSPiCtrl_TypeDef;


typedef union
{
    struct
    {
        uint32_t TX_DMA_EN : 1;   /*发送DMA使能*/
        uint32_t RX_DMA_EN : 1;   /*接收DMA使能*/
        uint32_t TX_CYC_MODE : 1; /*发送循环模式使能*/
        uint32_t RX_CYC_MODE : 1; /*接收循环模式使能*/
    }bitSpiDmaCtrl;
    uint32_t SpiDmaCtrl;
} uSpiDmaCtrl_TypeDef;


typedef struct
{
    uSPiCtrl_TypeDef Ctrl;
    uint32_t TxDflt;
    uint32_t Speed;
} sLL_SPI_InitTypeDef;

typedef struct
{
    uSpiDmaCtrl_TypeDef DmaCtrl;
    uint32_t TbAdr;
    uint32_t RbAdr;
    uint32_t TLen;
    uint32_t RLen;
    uint32_t DmaIE;
} sLL_SPIDMA_InitTypeDef;


/* Exported data ----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
extern SPI_TypeDef *LL_SPI_GetRegAddr(eSPI_ID_TypeDef spi_id);   
extern uint8_t LL_SPI_ReadByte(SPI_TypeDef *SPIx);
extern void LL_SPI_WriteByte(SPI_TypeDef *SPIx, uint8_t Dat);
extern uint8_t LL_SPI_TransByte(SPI_TypeDef *SPIx, uint8_t Dat);
extern void LL_SPI_Init(SPI_TypeDef *SPIx, sLL_SPI_InitTypeDef *sSPI_Init);
extern void LL_SPI_Disable(SPI_TypeDef *SPIx);

#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) \
    || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3) \
    || defined(RN202x_RN7326_SOC_B) || defined(RN202x_RN7326_SOC_V2)
extern void LL_SPI_DMAInit(SPI_TypeDef *SPIi, const sLL_SPIDMA_InitTypeDef *sSPIDMA_Init);
extern ErrorStatus LL_SPI_DMA_SetRecAddr(SPI_TypeDef *SPIi, uint32_t u32Addr);
extern ErrorStatus LL_SPI_DMA_SetTransAddr(SPI_TypeDef *SPIi, uint32_t u32Addr);
extern ErrorStatus LL_SPI_DMA_SetRlen(SPI_TypeDef *SPIi, uint32_t u32Len);
extern ErrorStatus LL_SPI_DMA_SetTlen(SPI_TypeDef *SPIi, uint32_t u32Len);
extern void LL_SPI_DMA_IEConfig(SPI_TypeDef *SPIi, eSPIDMA_IE_TypeDef int_type, FunctionalState enNewState);
extern ErrorStatus LL_SPI_DMA_CtlCmd(SPI_TypeDef *SPIi, eSPIDMA_CtlPara_TypeDef ctl_type, FunctionalState enNewState);
extern uint32_t LL_SPI_DMARxInfo(const SPI_TypeDef *SPIx);
extern uint32_t LL_SPI_DMATxInfo(const SPI_TypeDef *SPIx);
#endif

#endif /*_SPI_H*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
