/*******************************************************************************
  * @file    LL_UART.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-08-29
  * @brief   Uart drive
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
           ##### Uart peripheral features #####
  ==============================================================================
  [..]

           ##### How to use this driver #####
  ==============================================================================
  [..]


  @endverbatim
  */

/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll.h"
#ifdef LL_UART_MODULE_ENABLED
/* Private define ------------------------------------------------------------*/

/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/
/*********************************************************************************************************
** Function name:     	*Rn8xxx_Uart_GetRegAddr
** Descriptions:       	get the UART address
** input parameters:    uart_id:the serial no of uart; 
** output parameters:   None
** Returned value:     related UART address
*********************************************************************************************************/
UART_TypeDef *LL_UART_GetRegAddr(const eUART_ID_TypeDef uart_id)
{
  if (uart_id == UART0_ID)
  {
    return UART0;
  }
  else if (uart_id == UART1_ID)
  {
    return UART1;
  }
  else if (uart_id == UART2_ID)
  {
    return UART2;
  }
  else if (uart_id == UART3_ID)
  {
    return UART3;
  }
  else if (uart_id == UART4_ID)
  {
    return UART4;
  }
#if !defined(RN831x_RN861x_MCU_V3)
  else
  {
    return UART5;
  }
#else
  else if (uart_id == UART5_ID)
  {
    return UART5;
  }
  else if (uart_id == UART6_ID)
  {
    return UART6;
  }
  else
  {
    return UART7;
  }
#endif
}

/*********************************************************************************************************
** Function name:     	*Rn8xxx_Uart_GetRegAddr
** Descriptions:       	get the UART6_7 address
** input parameters:    uart_id:the serial no of uart6_7;
** output parameters:   None
** Returned value:     related UART6_7 address
*********************************************************************************************************/
UART6_7_TypeDef *LL_UART6_7_GetRegAddr(const eUART_ID_TypeDef uart_id)
{
  if (uart_id == UART6_ID)
  {
    return UART6;
  }
  else if (uart_id == UART7_ID)
  {
    return UART7;
  }
}
/*********************************************************************************************************
** Function name:     LL_UART_Init
** Descriptions:    	init uart configuration of uart parameters(check mode, baudrate, stop bis, data lenth )
** input parameters:
** output parameters: null
** Returned value: 		null
** Note: before call this function, system clk must be set and SystemCoreClock be updated
*********************************************************************************************************/
// void LL_UART_Init(UART_TypeDef *UARTx, const sLL_UART_InitTypeDef *sUART_Init)
void LL_UART_Init(const eUART_ID_TypeDef uart_id, const sLL_UART_InitTypeDef *sUART_Init)
{
  uint8_t UARTType;
  union
  {
    UART0_5_TypeDef *UART0_5;
    UART6_7_TypeDef *UART6_7;
  }UARTx;

  /* get uart addresss */
  if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
  {
    UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
    UARTType = UART_TYPE_6_7;
  }
  else
  {
    UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
    UARTType = UART_TYPE_0_5;
  }
    uint32_t ClkDiv;
    float fClkDiv;
    if (UARTType == UART_TYPE_0_5)
    {
      UARTx.UART0_5->CTRL = 0U;
    }
    else {
      UARTx.UART6_7->CTRL = 0U;
    }

    /*"配置UART通讯波特率"*/
    ClkDiv = SystemCoreClock; /*"获取当前系统时钟	"*/
    fClkDiv = ((float)ClkDiv) / ((float)(sUART_Init->BaudRate * (float)16));

    ClkDiv = (uint32_t)fClkDiv;
    if (UARTType == UART_TYPE_0_5)
    {
      UARTx.UART0_5->BAUD = ClkDiv - 1;
      UARTx.UART0_5->FDIV = (uint32_t)(((fClkDiv - ClkDiv) * 16384) + 0.5);

      /*"配置UART模式控制字"*/
      UARTx.UART0_5->CTRL = sUART_Init->Ctrl.UartCtrl;
    }
    else
    {
      UARTx.UART6_7->BAUD = ClkDiv - 1;
      UARTx.UART6_7->FDIV = (uint32_t)(((fClkDiv - ClkDiv) * 16384) + 0.5);

      /*"配置UART模式控制字"*/
      UARTx.UART6_7->CTRL = sUART_Init->Ctrl.UartCtrl;
    }

    if ((sUART_Init->Ctrl.UartCtrl & UART_CTRL_IRSEL) != 0U)
    {
        UART38K_CLK_EN;
    }
}
/*********************************************************************************************************
** Function name:     LL_UART_DeInit
** Descriptions:    	init uart configuration including IO and uart parameters(check mode, baudrate, stop bis, data lenth )
** input parameters:
** output parameters: null
** Returned value: 		null
*********************************************************************************************************/
void LL_UART_DeInit(const eUART_ID_TypeDef uart_id)
{
  uint8_t UARTType;
  union
  {
    UART0_5_TypeDef *UART0_5;
    UART6_7_TypeDef *UART6_7;
  }UARTx;

  if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
  {
    UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
    UARTType = UART_TYPE_6_7;
  }
  else
  {
    UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
    UARTType = UART_TYPE_0_5;
  }

  if (UARTType == UART_TYPE_0_5)
  {
    UARTx.UART0_5->STA = 0x3f;
    UARTx.UART0_5->CTRL = 0;
    UARTx.UART0_5->BAUD = 0;
    UARTx.UART0_5->FDIV = 0;
  }
  else {
    UARTx.UART6_7->STA = 0x3f;
    UARTx.UART6_7->CTRL = 0;
    UARTx.UART6_7->BAUD = 0;
    UARTx.UART6_7->FDIV = 0;
  }
}
/*********************************************************************************************************
** Function name:     LL_UART_SttConfig
** Descriptions:   		modify uart state
** input parameters:  ComPort:   USART_IT: transmition direction as tx or rx;
                      NewState: enable of disable corresponding transmition
** output parameters: null
** Returned value:  	null
*********************************************************************************************************/
void LL_UART_SttConfig(const eUART_ID_TypeDef uart_id, FunctionalMODE USART_IT, FunctionalState NewState)
{
  uint8_t UARTType;
  union
  {
    UART0_5_TypeDef *UART0_5;
    UART6_7_TypeDef *UART6_7;
  }UARTx;

  if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
  {
    UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
    UARTType = UART_TYPE_6_7;
  }
  else
  {
    UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
    UARTType = UART_TYPE_0_5;
  }

    uint8_t Compose;
    Compose = (uint8_t)(((uint32_t)USART_IT << 1) | ((uint32_t)NewState));
    switch (Compose)
    {
        //-------------------------
    case (uint8_t)(((uint32_t)USART_IT_RX << 1) | (uint32_t)ERN_DISABLE): /*" 接收|关闭"*/
        if (UARTType == UART_TYPE_0_5)
        {
          UARTx.UART0_5->CTRL &= 0xfff3;             /*" 屏蔽接收中断,屏蔽接收错误中断"*/
          UARTx.UART0_5->STA = 0x3d;                 /*" 清接收中断标志及接收错误标志"*/
        }
        else
        {
          UARTx.UART6_7->CTRL &= 0xfff3;             /*" 屏蔽接收中断,屏蔽接收错误中断"*/
          UARTx.UART6_7->STA = 0x3d;                 /*" 清接收中断标志及接收错误标志"*/
        }
        break;
    case (uint8_t)(((uint32_t)USART_IT_RX << 1) | (uint32_t)ERN_ENABLE): /*" 接收|打开"*/
        if (UARTType == UART_TYPE_0_5)
        {
          UARTx.UART0_5->CTRL |= 0x0c;              /*" 允许接收中断,允许接收错误中断"*/
          UARTx.UART0_5->STA = 0x3d;                /*" 清接收中断标志及接收错误标志"*/
        }
        else
        {
          UARTx.UART6_7->CTRL |= 0x0c;              /*" 允许接收中断,允许接收错误中断"*/
          UARTx.UART6_7->STA = 0x3d;                /*" 清接收中断标志及接收错误标志"*/
        }
        break;
    case (uint8_t)(((uint32_t)USART_IT_TX << 1) | (uint32_t)ERN_DISABLE): /*" 发送|关闭"*/
        if (UARTType == UART_TYPE_0_5)
        {
          UARTx.UART0_5->CTRL &= 0xfffd;             /*" 屏蔽发送中断"*/
          UARTx.UART0_5->STA = 0x02;                 /*" 清发送中断标志"*/
        }
        else
        {
          UARTx.UART6_7->CTRL &= 0xfffd;             /*" 屏蔽发送中断"*/
          UARTx.UART6_7->STA = 0x02;                 /*" 清发送中断标志"*/
        }
        break;
    case (uint8_t)(((uint32_t)USART_IT_TX << 1) | (uint32_t)ERN_ENABLE): /*" 发送|打开"*/
        if (UARTType == UART_TYPE_0_5)
        {
          UARTx.UART0_5->CTRL |= 0x02;              /*" 允许发送中断"*/
          UARTx.UART0_5->STA = 0x02;                /*" 清发送中断标志"*/
        }
        else
        {
          UARTx.UART6_7->CTRL |= 0x02;              /*" 允许发送中断"*/
          UARTx.UART6_7->STA = 0x02;                /*" 清发送中断标志"*/
        }
        break;
    default:
        break;
    }
}

uint8_t LL_UART_Read(const eUART_ID_TypeDef uart_id)
{
#ifdef RN831x_RN861x_MCU_V4
  uint8_t UARTType;
  union
  {
    UART0_5_TypeDef *UART0_5;
    UART6_7_TypeDef *UART6_7;
  }UARTx;

  if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
  {
    UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
    UARTType = UART_TYPE_6_7;
  }
  else
  {
    UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
    UARTType = UART_TYPE_0_5;
  }
  if (UARTType == UART_TYPE_0_5)
  {
    return ((uint8_t)UARTx.UART0_5->RXDATA);
  }
  else
  {
    return ((uint8_t)UARTx.UART6_7->RXDATA);
  }
#else
    return ((uint8_t)UARTx->RXD);
#endif
}
void LL_UART_Write(const eUART_ID_TypeDef uart_id, uint8_t data)
{
#ifdef RN831x_RN861x_MCU_V4
  uint8_t UARTType;
  union
  {
    UART0_5_TypeDef *UART0_5;
    UART6_7_TypeDef *UART6_7;
  }UARTx;

  if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
  {
    UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
    UARTType = UART_TYPE_6_7;
  }
  else
  {
    UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
    UARTType = UART_TYPE_0_5;
  }
  /* send data */
  if (UARTType == UART_TYPE_0_5)
  {
    UARTx.UART0_5->TXDATA = data;
  }
  else
  {
    UARTx.UART6_7->TXDATA = data;
  }
#else
    UARTx->TXD = data;
#endif
}

/*********************************************************************************************************
** Function name:       LL_UART_Disable
** Descriptions:        关闭UART模块
** input parameters:    UARTx：UART模块
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_UART_Disable(const eUART_ID_TypeDef uart_id)
{
  uint8_t UARTType;
  union
  {
    UART0_5_TypeDef *UART0_5;
    UART6_7_TypeDef *UART6_7;
  }UARTx;

  if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
  {
    UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
    UARTType = UART_TYPE_6_7;
  }
  else
  {
    UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
    UARTType = UART_TYPE_0_5;
  }

  if (UARTType == UART_TYPE_0_5)
  {
    SET_REG_UART_CTRL(UARTx.UART0_5,0);
  }
  else
  {
    SET_REG_UART_CTRL(UARTx.UART6_7,0);
  }
}

#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) \
   || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V4) \
   || defined(RN202x_RN7326_SOC_B) || defined(RN202x_RN7326_SOC_V2)
/*********************************************************************************************************
** Function name:     LL_UART_DMAInit
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		null
*********************************************************************************************************/
void LL_UART_DMAInit(const eUART_ID_TypeDef uart_id, sLL_UARTDMA_InitTypeDef sUART_DMAInit)
{
  uint8_t UARTType;
  union
  {
    UART0_5_TypeDef *UART0_5;
    UART6_7_TypeDef *UART6_7;
  }UARTx;

  if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
  {
    UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
    UARTType = UART_TYPE_6_7;
  }
  else
  {
    UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
    UARTType = UART_TYPE_0_5;
  }

  if (UARTType == UART_TYPE_0_5)
  {
    UARTx.UART0_5->DMA_CTRL = 0;
    UARTx.UART0_5->DMA_TBADR = sUART_DMAInit.TBADR; /*offset = 0x1C*/
    UARTx.UART0_5->DMA_RBADR = sUART_DMAInit.RBADR; /*offset = 0x20*/
    UARTx.UART0_5->DMA_TLEN = sUART_DMAInit.TLEN;   /*offset = 0x24*/
    UARTx.UART0_5->DMA_RLEN = sUART_DMAInit.RLEN;   /*offset = 0x28*/
    UARTx.UART0_5->DMA_IF = 0x3f;
    UARTx.UART0_5->DMA_IE = sUART_DMAInit.IE;
    UARTx.UART0_5->DMA_CTRL = sUART_DMAInit.CTL.UartDmaCtrl;
  }
  else
  {
    UARTx.UART6_7->DMA_CTRL = 0;
    UARTx.UART6_7->DMA_TBADR = sUART_DMAInit.TBADR; /*offset = 0x1C*/
    UARTx.UART6_7->DMA_RBADR = sUART_DMAInit.RBADR; /*offset = 0x20*/
    UARTx.UART6_7->DMA_TLEN = sUART_DMAInit.TLEN;   /*offset = 0x24*/
    UARTx.UART6_7->DMA_RLEN = sUART_DMAInit.RLEN;   /*offset = 0x28*/
    UARTx.UART6_7->DMA_IF = 0x3f;
    UARTx.UART6_7->DMA_IE = sUART_DMAInit.IE;
    UARTx.UART6_7->DMA_CTRL = sUART_DMAInit.CTL.UartDmaCtrl;
  }
}
/*********************************************************************************************************
** Function name:     LL_UART_DMA_SetRecAddr
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		ErrorStatus
*********************************************************************************************************/
ErrorStatus LL_UART_DMA_SetRecAddr(const eUART_ID_TypeDef uart_id, uint32_t u32Addr)
{
  uint8_t UARTType;
  union
  {
    UART0_5_TypeDef *UART0_5;
    UART6_7_TypeDef *UART6_7;
  }UARTx;

  if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
  {
    UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
    UARTType = UART_TYPE_6_7;
  }
  else
  {
    UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
    UARTType = UART_TYPE_0_5;
  }

  if (UARTType == UART_TYPE_0_5)
  {
    UARTx.UART0_5->DMA_RBADR = u32Addr;
  }
  else
  {
    UARTx.UART6_7->DMA_RBADR = u32Addr;
  }
  return ERN_SUCCESS;
}
/*********************************************************************************************************
** Function name:     LL_UART_DMA_SetTransAddr
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		ErrorStatus
*********************************************************************************************************/
ErrorStatus LL_UART_DMA_SetTransAddr(const eUART_ID_TypeDef uart_id, uint32_t u32Addr)
{
    uint8_t UARTType;
  union
  {
    UART0_5_TypeDef *UART0_5;
    UART6_7_TypeDef *UART6_7;
  }UARTx;

  if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
  {
    UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
    UARTType = UART_TYPE_6_7;
  }
  else
  {
    UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
    UARTType = UART_TYPE_0_5;
  }

  if (UARTType == UART_TYPE_0_5)
  {
    UARTx.UART0_5->DMA_TBADR = u32Addr;
  }
  else
  {
    UARTx.UART6_7->DMA_TBADR = u32Addr;
  }
    return ERN_SUCCESS;
}

/*********************************************************************************************************
** Function name:     LL_UART_DMA_SetRecAddr
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		ErrorStatus
*********************************************************************************************************/
ErrorStatus LL_UART_DMA_SetRecLen(const eUART_ID_TypeDef uart_id, uint32_t u32Len)
{
  uint8_t UARTType;
  union
  {
    UART0_5_TypeDef *UART0_5;
    UART6_7_TypeDef *UART6_7;
  }UARTx;

  if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
  {
    UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
    UARTType = UART_TYPE_6_7;
  }
  else
  {
    UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
    UARTType = UART_TYPE_0_5;
  }

  if (UARTType == UART_TYPE_0_5)
  {
    UARTx.UART0_5->DMA_RLEN = u32Len;
  }
  else
  {
    UARTx.UART6_7->DMA_RLEN = u32Len;
  }
  return ERN_SUCCESS;
}
/*********************************************************************************************************
** Function name:     LL_UART_DMA_SetTransAddr
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		ErrorStatus
*********************************************************************************************************/
ErrorStatus LL_UART_DMA_SetTransLen(const eUART_ID_TypeDef uart_id, uint32_t u32Len)
{
  uint8_t UARTType;
  union
  {
    UART0_5_TypeDef *UART0_5;
    UART6_7_TypeDef *UART6_7;
  }UARTx;

  if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
  {
    UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
    UARTType = UART_TYPE_6_7;
  }
  else
  {
    UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
    UARTType = UART_TYPE_0_5;
  }

  if (UARTType == UART_TYPE_0_5)
  {
    UARTx.UART0_5->DMA_TLEN = u32Len;
  }
  else
  {
    UARTx.UART6_7->DMA_TLEN = u32Len;
  }
    return ERN_SUCCESS;
}

/*********************************************************************************************************
** Function name:     LL_UART_DMA_ConfigInt
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		ErrorStatus
*********************************************************************************************************/
void LL_UART_DMA_IEConfig(const eUART_ID_TypeDef uart_id, eUARTDMA_IEPara_TypeDef int_type, FunctionalState enNewState)
{
  uint8_t UARTType;
  union
  {
    UART0_5_TypeDef *UART0_5;
    UART6_7_TypeDef *UART6_7;
  }UARTx;

  if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
  {
    UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
    UARTType = UART_TYPE_6_7;
  }
  else
  {
    UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
    UARTType = UART_TYPE_0_5;
  }

  if(ERN_ENABLE == enNewState)
  {
    if (UARTType == UART_TYPE_0_5)
    {
      SET_BIT_UART_DMA_IE(UARTx.UART0_5, int_type);
    }
    else
    {
      SET_BIT_UART_DMA_IE(UARTx.UART6_7, int_type);
    }
  }
  else{
    if (UARTType == UART_TYPE_0_5)
    {
      CLR_BIT_UART_DMA_IE(UARTx.UART0_5, int_type);
    }
    else
    {
      CLR_BIT_UART_DMA_IE(UARTx.UART6_7, int_type);
    }
  }
}

/*********************************************************************************************************
** Function name:     LL_UART_DMACtl
** Descriptions:
** output parameters: null
** input parameters:
** Returned value: 		ErrorStatus
*********************************************************************************************************/
ErrorStatus LL_UART_DMA_CtlCmd(const eUART_ID_TypeDef uart_id, eUARTDMA_CtlType_TypeDef ctl_type, FunctionalState enNewState)
{
  uint8_t UARTType;
  union
  {
    UART0_5_TypeDef *UART0_5;
    UART6_7_TypeDef *UART6_7;
  }UARTx;

  if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
  {
    UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
    UARTType = UART_TYPE_6_7;
  }
  else
  {
    UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
    UARTType = UART_TYPE_0_5;
  }

  if(ERN_ENABLE == enNewState)
  {
    if (UARTType == UART_TYPE_0_5)
    {
      SET_BIT_UART_DMA_CTRL(UARTx.UART0_5,ctl_type);
    }
    else
    {
      SET_BIT_UART_DMA_CTRL(UARTx.UART6_7,ctl_type);
    }
  }
  else
  {
    if (UARTType == UART_TYPE_0_5)
    {
      CLR_BIT_UART_DMA_CTRL(UARTx.UART0_5,ctl_type);
    }
    else
    {
      CLR_BIT_UART_DMA_CTRL(UARTx.UART6_7,ctl_type);
    }
  }
  return ERN_SUCCESS;
}
/*********************************************************************************************************
** Function name:     	LL_UART_DMARxInfo
** Descriptions:
** input parameters:    null
** output parameters:   Uart DMA获取当前DMA接收地址
** Returned value:      null
*********************************************************************************************************/
uint16_t LL_UART_DMARxInfo(const eUART_ID_TypeDef uart_id)
{
    uint32_t rx_count;
    uint8_t UARTType;
    union
    {
      UART0_5_TypeDef *UART0_5;
      UART6_7_TypeDef *UART6_7;
    }UARTx;

    if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
    {
      UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
      UARTType = UART_TYPE_6_7;
    }
    else
    {
      UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
      UARTType = UART_TYPE_0_5;
    }
    if (UARTType == UART_TYPE_0_5)
    {
      rx_count = UARTx.UART0_5->DMA_RADR;
      rx_count -= UARTx.UART0_5->DMA_RBADR;
    }
    else
    {
      rx_count = UARTx.UART6_7->DMA_RADR;
      rx_count -= UARTx.UART6_7->DMA_RBADR;
    }

    return ((uint16_t)rx_count);
}
/*********************************************************************************************************
** Function name:     	LL_UART_DMATxInfo
** Descriptions:
** input parameters:    	null
** output parameters:   获取当前DMA发送地址
** Returned value:      null
*********************************************************************************************************/
uint16_t LL_UART_DMATxInfo(const eUART_ID_TypeDef uart_id)
{
    uint32_t tx_count;
    uint8_t UARTType;
    union
    {
      UART0_5_TypeDef *UART0_5;
      UART6_7_TypeDef *UART6_7;
    }UARTx;

    if ((uart_id == UART6_ID) || (uart_id == UART7_ID))
    {
      UARTx.UART6_7 = LL_UART6_7_GetRegAddr(uart_id);
      UARTType = UART_TYPE_6_7;
    }
    else
    {
      UARTx.UART0_5 = LL_UART_GetRegAddr(uart_id);
      UARTType = UART_TYPE_0_5;
    }
    if (UARTType == UART_TYPE_0_5)
    {
      tx_count = UARTx.UART0_5->DMA_TADR;
      tx_count -= UARTx.UART0_5->DMA_TBADR;
    }
    else
    {
      tx_count = UARTx.UART6_7->DMA_TADR;
      tx_count -= UARTx.UART6_7->DMA_TBADR;
    }
    return ((uint16_t)tx_count);
}
#endif
#endif /*LL_UART_MODULE_ENABLED*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
