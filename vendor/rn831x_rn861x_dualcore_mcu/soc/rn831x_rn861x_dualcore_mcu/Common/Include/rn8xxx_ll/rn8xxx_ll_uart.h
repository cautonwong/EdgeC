/********************************************************************************
 * @file    rn8xxx_uart.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-09-07
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
 * V1.1.1   2023-09-07  Add       Add macro definition for geting register and register bit
 * V1.1.0               Initial
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef Rn8xxx_UART_H_
#define Rn8xxx_UART_H_

/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll_def.h"


/* Exported define ------------------------------------------------------------*/

#define UART_NUM 6

#define UARTCTL_DEFAULT 0x00     /*" UART_CTL 寄存器复位默认值 "*/

#ifdef RN831x_RN861x_MCU_V4
#define UART_TypeDef    UART0_5_TypeDef
#define UART_CTRL_IRSEL UART0_5_CTRL_IRSEL
#endif

/*********************************************************************************/
#define GET_REG_UART_CTRL(uartx, databits) (((uartx)->CTRL) & GET_DATA_MASK(databits))
#define GET_REG_UART_BAUD(uartx, databits) (((uartx)->BAUD) & GET_DATA_MASK(databits))
#define GET_REG_UART_STA(uartx, databits) (((uartx)->STA) & GET_DATA_MASK(databits))
#define GET_REG_UART_TXD(uartx, databits) (((uartx)->TXD) & GET_DATA_MASK(databits))
#define GET_REG_UART_RXD(uartx, databits) (((uartx)->RXD) & GET_DATA_MASK(databits))
#define GET_REG_UART_FDIV(uartx, databits) (((uartx)->FDIV) & GET_DATA_MASK(databits))

#define SET_REG_UART_CTRL(uartx, data) (((uartx)->CTRL) = (data))
#define SET_REG_UART_BAUD(uartx, data) (((uartx)->BAUD) = (data))
#define SET_REG_UART_STA(uartx, data) (((uartx)->STA) = (data))
#define SET_REG_UART_TXD(uartx, data) (((uartx)->TXD) = (data))
#define SET_REG_UART_RXD(uartx, data) (((uartx)->RXD) = (data))
#define SET_REG_UART_FDIV(uartx, data) (((uartx)->FDIV) = (data))

/*********************************************************************************/

#define GET_BIT_UART_CTRL(uartx, bit) ((((uartx)->CTRL) & (bit)) ? (1u) : (0u))
#define GET_BIT_UART_BAUD(uartx, bit) ((((uartx)->BAUD) & (bit)) ? (1u) : (0u))
#define GET_BIT_UART_STA(uartx, bit) ((((uartx)->STA) & (bit)) ? (1u) : (0u))
#define GET_BIT_UART_TXD(uartx, bit) ((((uartx)->TXD) & (bit)) ? (1u) : (0u))
#define GET_BIT_UART_RXD(uartx, bit) ((((uartx)->RXD) & (bit)) ? (1u) : (0u))
#define GET_BIT_UART_FDIV(uartx, bit) ((((uartx)->FDIV) & (bit)) ? (1u) : (0u))

/*#define SET_BIT_UART_CTRL(uartx, bit) SET_BIT(uartx->CTRL, bit)*/
#define SET_BIT_UART_CTRL(uartx, bit) (SET_REG32_BIT(((uartx)->CTRL), (bit)))
#define SET_BIT_UART_BAUD(uartx, bit) (SET_REG32_BIT(((uartx)->BAUD), (bit)))
#define SET_BIT_UART_STA(uartx, bit) (((uartx)->STA) = (bit))
#define SET_BIT_UART_TXD(uartx, bit) (SET_REG32_BIT(((uartx)->TXD), (bit)))
#define SET_BIT_UART_RXD(uartx, bit) (SET_REG32_BIT(((uartx)->RXD), (bit)))
#define SET_BIT_UART_FDIV(uartx, bit) (SET_REG32_BIT(((uartx)->FDIV), (bit)))

#define SET_BIT_TO_CLR_UART_STA(uartx, bit) (((uartx)->STA) = (bit))

#define UART_WRITE_DATA(uartx, data) (SET_REG_UART_TXD((uartx), (data)))
#define UART_READ_DATA(uartx) (GET_REG_UART_RXD((uartx), (UART_RXD_RXDATA)))

#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) \
   || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V4) \
   || defined(RN202x_RN7326_SOC_B) || defined(RN202x_RN7326_SOC_V2)
#define UART_DMA_RX_ERR_IE (BIT5) /*" 接收数据覆盖中断使能 "*/
#define UART_DMA_TX_ERR_IE (BIT4) /*" 发送数据错误中断使能 "*/
#define UART_DMA_RX_FIE (BIT3)    /*" DMA接收全满中断使能 "*/
#define UART_DMA_RX_HIE (BIT2)    /*" DMA接收半满中断使能 "*/
#define UART_DMA_TX_FIE (BIT1)    /*" DMA发送全满中断使能 "*/
#define UART_DMA_TX_HIE (BIT0)    /*" DMA发送半满中断使能 "*/

/*********************************************************************************/
#define GET_REG_UART_DMA_CTRL(uartx, databits) ((uartx)->DMA_CTRL & GET_DATA_MASK(databits))
#define GET_REG_UART_DMA_TBADR(uartx, databits) ((uartx)->DMA_TBADR & GET_DATA_MASK(databits))
#define GET_REG_UART_DMA_RBADR(uartx, databits) ((uartx)->DMA_RBADR & GET_DATA_MASK(databits))
#define GET_REG_UART_DMA_TLEN(uartx, databits) ((uartx)->DMA_TLEN & GET_DATA_MASK(databits))
#define GET_REG_UART_DMA_RLEN(uartx, databits) ((uartx)->DMA_RLEN & GET_DATA_MASK(databits))
#define GET_REG_UART_DMA_TADR(uartx, databits) ((uartx)->DMA_TADR & GET_DATA_MASK(databits))
#define GET_REG_UART_DMA_RADR(uartx, databits) ((uartx)->DMA_RADR & GET_DATA_MASK(databits))
#define GET_REG_UART_DMA_IE(uartx, databits) ((uartx)->DMA_IE & GET_DATA_MASK(databits))
#define GET_REG_UART_DMA_IF(uartx, databits) ((uartx)->DMA_IF & GET_DATA_MASK(databits))
#define GET_REG_UART_DMA_TO(uartx, databits) ((uartx)->DMA_TO & GET_DATA_MASK(databits))

#define SET_REG_UART_DMA_CTRL(uartx, data) ((uartx)->DMA_CTRL = (data))
#define SET_REG_UART_DMA_TBADR(uartx, data) ((uartx)->DMA_TBADR = (data))
#define SET_REG_UART_DMA_RBADR(uartx, data) ((uartx)->DMA_RBADR = (data))
#define SET_REG_UART_DMA_TLEN(uartx, data) ((uartx)->DMA_TLEN = (data))
#define SET_REG_UART_DMA_RLEN(uartx, data) ((uartx)->DMA_RLEN = (data))
#define SET_REG_UART_DMA_TADR(uartx, data) ((uartx)->DMA_TADR = (data))
#define SET_REG_UART_DMA_RADR(uartx, data) ((uartx)->DMA_RADR = (data))
#define SET_REG_UART_DMA_IE(uartx, data) ((uartx)->DMA_IE = (data))
#define SET_REG_UART_DMA_IF(uartx, data) ((uartx)->DMA_IF = (data))
#define SET_REG_UART_DMA_TO(uartx, data) ((uartx)->DMA_TO = (data))
/*********************************************************************************/

#define GET_BIT_UART_DMA_CTRL(uartx, bit) (((uartx)->DMA_CTRL & (bit)) ? (1u) : (0u))
#define GET_BIT_UART_DMA_IE(uartx, bit) (((uartx)->DMA_IE & (bit)) ? (1u) : (0u))
#define GET_BIT_UART_DMA_IF(uartx, bit) (((uartx)->DMA_IF & (bit)) ? (1u) : (0u))


#define SET_BIT_UART_DMA_CTRL(uartx, bit) (SET_REG32_BIT((uartx)->DMA_CTRL, (bit)))
#define SET_BIT_UART_DMA_IE(uartx, bit) (SET_REG32_BIT((uartx)->DMA_IE, (bit)))
#define SET_BIT_UART_DMA_IF(uartx, bit) ((uartx)->DMA_IF = (bit))

#define CLR_BIT_UART_DMA_CTRL(uartx, bit) (CLR_REG32_BIT((uartx)->DMA_CTRL, (bit)))
#define CLR_BIT_UART_DMA_IE(uartx, bit) (CLR_REG32_BIT((uartx)->DMA_IE, (bit)))

#define SET_BIT_TO_CLR_UART_DMA_IF(uartx, bit) ((uartx)->DMA_IF = (bit))
#endif
/* Exported enum ------------------------------------------------------------*/
typedef enum
{
    UART0_ID = 0,
    UART1_ID = 1,
    UART2_ID = 2,
    UART3_ID = 3,
    UART4_ID = 4,
    UART5_ID = 5,
    #if !defined(RN831x_RN861x_MCU_V3) && !defined(RN831x_RN861x_MCU_V4)
    UARTx_ID = 6,
    #else
    UART6_ID = 6,
    UART7_ID = 7,
    UARTx_ID = 8,
    #endif
} eUART_ID_TypeDef;

typedef enum
{
    UART_BPS_300 = 300, /*"波特率"*/
    UART_BPS_600 = 600,
    UART_BPS_1200 = 1200,
    UART_BPS_2400 = 2400,
    UART_BPS_4800 = 4800,
    UART_BPS_7200 = 7200,
    UART_BPS_9600 = 9600,
    UART_BPS_14400 = 14400,
    UART_BPS_19200 = 19200,
    UART_BPS_38400 = 38400,
    UART_BPS_57600 = 56700,
    UART_BPS_115200 = 115200,
    UART_BPS_894886 = 892857,
    UART_BPS_8785 = 8785,
    UART_BPS_1638400 = 1638400,
    UART_BPS_7412     = 7412,
    UART_BPS_1382400  = 1382400,
} eUART_BPS_TypeDef;

typedef enum
{
    UARTCTL_NEG_H = 0x00, /*"默认驱动电平为高，发送/接收的数据极性不变"*/
    UARTCTL_NEG_L = 0x01, /*"默认驱动电平为低，发送/接收的数据极性取反"*/

    UARTCTL_LSB = 0x00, /*"LSB先传输"*/
    UARTCTL_MSB = 0x01, /*"MSB先传输"*/

    UARTCTL_IRSEL_H = 0x00, /*"正极性，即低电平调制输出，高电平（默认状态）保持"*/
    UARTCTL_IRSEL_L = 0x01, /*"负极性，即数据取反，高电平调制输出，低电平保持"*/
                            /*"-IRSEL 只决定空闲输出（无效电平）时的电平高低，不影响有效数据期间的电平"*/

    UARTCTL_ILBE_DIS = 0x00,
    UARTCTL_ILBE_EN = 0x01,/*内部回环使能，若使能该选项，芯片内部UART接收管脚短接到发送管脚，外部输入信号会被屏蔽，正常应用不要使能该位*/

    UARTCTL_IRE_DIS = 0x00, /*"关闭红外调制输出"*/
    UARTCTL_IRE_EN = 0x01,  /*"打开红外调制输出，用 38k 载波调制输出数据的低电平"*/

    UARTCTL_NO_PARITY = 0x00,   /*"无校验"*/
    UARTCTL_ODD_PARITY = 0x01,  /*"奇校验"*/
    UARTCTL_EVEN_PARITY = 0x02, /*"偶校验"*/
    UARTCTL_0_PARITY = 0x03,     /*"固定为零校验"*/
    UARTCTL_1_PARITY = 0x04,    /*"固定为 1 校验"*/

    UARTCTL_5BIT_DAT = 0x00, /*"数据位"*/
    UARTCTL_6BIT_DAT = 0x01,
    UARTCTL_7BIT_DAT = 0x02,
    UARTCTL_8BIT_DAT = 0x03,

    UARTCTL_1STOP_SBIT = 0x00, /*"停止位"*/
    UARTCTL_2STOP_SBIT = 0x01,

    UARTCTL_ERRIE_DIS = 0x00,
    UARTCTL_ERRIE_EN = 0x01,

    UARTCTL_RXIE_DIS = 0x00,
    UARTCTL_RXIE_EN = 0x01,

    UARTCTL_TXIE_DIS = 0x00,
    UARTCTL_TXIE_EN = 0x01,

    UARTCTL_DIS = 0x00,
    UARTCTL_EN = 0x01,

} eUART_CtlPara_TypeDef;

typedef enum
{
    USART_IT_RX = 0,
    USART_IT_TX = !USART_IT_RX
} FunctionalMODE;

typedef union
{
    struct
    {
        uint32_t EN         : 1;
        uint32_t TXIE       : 1;
        uint32_t RXIE       : 1;
        uint32_t ERRIE      : 1;
        uint32_t STOPS      : 1;
        uint32_t DATLEN     : 2;
        uint32_t PARS       : 3;
        uint32_t IRE        : 1;
        uint32_t ILBE       : 1;
        uint32_t IRSEL      : 1;
        uint32_t LMSB       : 1;
        uint32_t NEG        : 1;
#ifdef RN831x_RN861x_MCU_V4
        uint32_t PAR_MEM    : 1;
        uint32_t RX_TO_IE   : 1;
#endif
    }bitUartCtrl;
    uint32_t UartCtrl;
} uUartCtrl_TypeDef;

typedef enum
{
    UARTDMA_TX_DIS = 0,
    UARTDMA_TX_EN = 1,

    UARTDMA_RX_DIS = 0,
    UARTDMA_RX_EN = 1,

    UARTDMA_TX_CYC_DIS = 0,
    UARTDMA_TX_CYC_EN = 1,

    UARTDMA_RX_CYC_DIS = 0,
    UARTDMA_RX_CYC_EN = 1,

} eUART_DMACtlPara_TypeDef;

typedef enum
{
    UARTDMA_TX_DMA_EN = 0x01,       /*"发送 DMA 使能"*/
    UARTDMA_RX_DMA_EN = 0x02,       /*"接收 DMA 使能"*/
    UARTDMA_TX_CYC_MODE = 0x04, /*"发送循环模式使能"*/
    UARTDMA_RX_CYC_MODE = 0x08, /*"接收循环模式使能"*/
} eUARTDMA_CtlType_TypeDef;

typedef enum
{
  UARTDMA_TX_HIE = 0x01,  /*"BIT0:DMA发送半满中断使能"*/

  UARTDMA_TX_FIE = 0x02,  /*"BIT1:DMA发送全满中断使能"*/

  UARTDMA_RX_HIE = 0x04,  /*"BIT2:DMA接收半满中断使能"*/

  UARTDMA_RX_FIE = 0x08,  /*"BIT3:DMA接收全满中断使能"*/

  UARTDMA_TX_ERR_IE = 0x10, /*"BIT4:发送数据错误中断使能"*/

  UARTDMA_RX_ERR_IE = 0x20, /*"BIT5:接收数据覆盖中断使能"*/

}eUARTDMA_IEPara_TypeDef;

typedef union
{
  struct
  {
    uint8_t TX_HIE:1;/* DMA发送半空中断*/
    uint8_t TX_FIE:1;/* DMA发送全空中断 */
    uint8_t RX_HIE:1;/* DMA接收半空中断 */
    uint8_t RX_FIE:1;/* DMA接收全空中断 */
  }bitUartDmaIE;
  uint8_t UartDmaIE;
}uUartDMAIE_TypeDef;


typedef union
{
    struct
    {
        uint32_t TX_DMA_EN : 1;   /*"发送DMA使能"*/
        uint32_t RX_DMA_EN : 1;   /*"接收DMA使能"*/
        uint32_t TX_CYC_MODE : 1; /*"发送循环模式使能"*/
        uint32_t RX_CYC_MODE : 1; /*"接收循环模式使能"*/
    }bitUartDmaCtrl;
    uint32_t UartDmaCtrl;
} uUartDmaCtrl_TypeDef;

typedef struct
{
    uUartDmaCtrl_TypeDef CTL;
    uint32_t TBADR;
    uint32_t RBADR;
    uint32_t TLEN;
    uint32_t RLEN;
    uint8_t IE;
} uUART_DMAPara_TypeDef;

typedef struct
{
    uUartDmaCtrl_TypeDef CTL;
    uint32_t TBADR;
    uint32_t RBADR;
    uint32_t TLEN;
    uint32_t RLEN;
    uint8_t IE;
} sLL_UARTDMA_InitTypeDef;


typedef struct
{
    uint32_t BaudRate;
    uUartCtrl_TypeDef Ctrl;
} sLL_UART_InitTypeDef;

/* Exported data ----------------------------------------------------------*/
#define UART_TYPE_0_5               ((uint8_t)(0x01U))             /*!< Instance use uart0_5 */
#define UART_TYPE_6_7               ((uint8_t)(0x10U))             /*!< Instance use uart6_7 */
/* Exported functions ------------------------------------------------------- */
extern UART_TypeDef *LL_UART_GetRegAddr(const eUART_ID_TypeDef uart_id);
extern UART6_7_TypeDef *LL_UART6_7_GetRegAddr(const eUART_ID_TypeDef uart_id);
extern void LL_UART_Init(const eUART_ID_TypeDef uart_id, const sLL_UART_InitTypeDef *sUART_Init);
extern void LL_UART_DeInit(const eUART_ID_TypeDef uart_id);
extern void LL_UART_SttConfig(const eUART_ID_TypeDef uart_id, FunctionalMODE USART_IT, FunctionalState NewState);
extern uint8_t LL_UART_Read(const eUART_ID_TypeDef uart_id);
extern void LL_UART_Write(const eUART_ID_TypeDef uart_id, uint8_t data);
extern void LL_UART_Disable(const eUART_ID_TypeDef uart_id);

#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) \
   || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V4) \
   || defined(RN202x_RN7326_SOC_B) || defined(RN202x_RN7326_SOC_V2)
extern void LL_UART_DMAInit(const eUART_ID_TypeDef uart_id, sLL_UARTDMA_InitTypeDef sUART_DMAInit);
extern ErrorStatus LL_UART_DMA_SetRecAddr(const eUART_ID_TypeDef uart_id, uint32_t u32Addr);
extern ErrorStatus LL_UART_DMA_SetTransAddr(const eUART_ID_TypeDef uart_id, uint32_t u32Addr);
extern ErrorStatus LL_UART_DMA_SetRecLen(const eUART_ID_TypeDef uart_id, uint32_t u32Len);
extern ErrorStatus LL_UART_DMA_SetTransLen(const eUART_ID_TypeDef uart_id, uint32_t u32Len);
extern void LL_UART_DMA_IEConfig(const eUART_ID_TypeDef uart_id, eUARTDMA_IEPara_TypeDef int_type, FunctionalState enNewState);
extern ErrorStatus LL_UART_DMA_CtlCmd(const eUART_ID_TypeDef uart_id,eUARTDMA_CtlType_TypeDef ctl_type, FunctionalState enNewState);

extern uint16_t LL_UART_DMARxInfo(const eUART_ID_TypeDef uart_id);
extern uint16_t LL_UART_DMATxInfo(const eUART_ID_TypeDef uart_id);
#endif

#endif /*_UART_H*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
