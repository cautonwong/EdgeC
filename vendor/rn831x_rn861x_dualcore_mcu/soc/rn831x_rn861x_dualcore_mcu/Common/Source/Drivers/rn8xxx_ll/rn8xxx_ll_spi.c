/*******************************************************************************
  * @file    LL_SPI.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-08-29
  * @brief   Spi driver
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
                     ##### Spi peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]


    @endverbatim
  */
/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll.h"
#ifdef LL_SPI_MODULE_ENABLED
/* Private define ------------------------------------------------------------*/

/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/
/*********************************************************************************************************
** Function name:     	*LL_Spi_GetRegAddr
** Descriptions:       	get the SPI address
** input parameters:    spi_id:the serial no of spi; 
** output parameters:   None
** Returned value:     related SPI address
*********************************************************************************************************/
SPI_TypeDef *LL_SPI_GetRegAddr(eSPI_ID_TypeDef spi_id)
{

#if defined(RN831x_RN861x_MCU_V1)||defined(RN831x_RN861x_MCU_V2)||defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V4) \
    || defined(RN821x_RN721x_SOC_B)||defined(RN821x_RN721x_SOC_C)||defined(RN821x_RN721x_SOC_D)||defined(RN821x_RN721x_SOC_V2) 
    if(spi_id == SPI0_ID)  
    {
      return SPI0;
    }
#endif

#if defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V4) \
      || defined(RN821x_RN721x_SOC_D) || defined(RN821x_RN721x_SOC_V2)
    else if(spi_id == SPI1_ID)
    {
      return SPI1;
    }
#endif
    
#if defined(RN821x_RN721x_SOC_V3) \
    ||defined(RN202x_RN7326_SOC_B) ||defined(RN202x_RN7326_SOC_V2)
    if(spi_id == SPI1_ID)
    {
      return SPI1;
    }
#endif
    
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V4)
    else if(spi_id == SPI2_ID)	
    {
      return SPI2;
    }
#endif

#if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3) \
    || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V4) \
    ||defined(RN202x_RN7326_SOC_B) ||defined(RN202x_RN7326_SOC_V2)
    else if(spi_id == SPI3_ID)
    {
      return SPI3;
    }
#endif

#if defined(RN202x_RN7326_SOC_B) ||defined(RN202x_RN7326_SOC_V2)
  else if(spi_id == SPI4_ID)
  {
    return SPI4;
  }
  // else
  // {
  //  return((SPI_TypeDef *)LL_ZERO);
  // }  
#endif	
  else
  {
    return((SPI_TypeDef *)LL_ZERO);
  } 

}

/*********************************************************************************************************
** Function name:     fnSpi_ReadByte
** Descriptions:     to get data spi received
** input parameters: spi_id no
** output parameters:  null
** Returned value:   spi received data
*********************************************************************************************************/
uint8_t LL_SPI_ReadByte(SPI_TypeDef *SPIx)
{
    SPIx->STA = 0xff;                                                              /*清状态*/
#ifdef RN831x_RN861x_MCU_V4
    SPIx->TXDATA = 0xff;                                                               /*往发送寄存器写数据，启动SPI时钟发送*/
#else
    SPIx->TX = 0xff;                                                               /*往发送寄存器写数据，启动SPI时钟发送*/
#endif
    if (fnRegState_Detect((uint32_t)(&SPIx->STA), 0, 1, SPI_TIMEOUT) == ERN_ERROR) /*等待SPI传输完成*/
    {
        ;
    }
#ifdef RN831x_RN861x_MCU_V4
    return (uint8_t)(SPIx->RXDATA);
#else
    return (uint8_t)(SPIx->RX);
#endif
}
/*********************************************************************************************************
** Function name:     fnSpi_WriteByte
** Descriptions:    to send data through SPI
** input parameters:  spi_id:  dat: data to be send
** output parameters:  null
** Returned value: null
*********************************************************************************************************/
void LL_SPI_WriteByte(SPI_TypeDef *SPIx, uint8_t Dat)
{
    SPIx->STA = 0xff; /*清状态*/
#ifdef RN831x_RN861x_MCU_V4
    SPIx->TXDATA = Dat;   /*写待发送的数据*/
#else
    SPIx->TX = Dat;   /*写待发送的数据*/
#endif

    if (fnRegState_Detect((uint32_t)(&SPIx->STA), 0, 1, SPI_TIMEOUT) == ERN_ERROR) /*等待发送完成*/
    {
        ;
    }
}

/*********************************************************************************************************
** Function name:     LL_SPI_TransByte
** Descriptions:    to send/receive data through SPI
** input parameters:  spi_id:  dat: data to be send
** output parameters:  null
** Returned value: spi received data
*********************************************************************************************************/
uint8_t LL_SPI_TransByte(SPI_TypeDef *SPIx, uint8_t Dat)
{
    SPIx->STA = 0xff; /*清状态*/
#ifdef RN831x_RN861x_MCU_V4
    SPIx->TXDATA = Dat;   /*写待发送的数据*/
#else
    SPIx->TX = Dat;   /*写待发送的数据*/
#endif
    if (fnRegState_Detect((uint32_t)(&SPIx->STA), 0, 1, SPI_TIMEOUT) == ERN_ERROR) /*等待发送完成*/
    {
        ;
    }
#ifdef RN831x_RN861x_MCU_V4
    return (uint8_t)(SPIx->RXDATA);
#else
    return (uint8_t)(SPIx->RX);
#endif
}
/*********************************************************************************************************
** Function name:     	LL_SPI_Init
** Descriptions:        初始化SPI
** input parameters:    spi_id:第几个SPI;
                        MastSlav:主从选择
                        LMSB:传输BIT顺序
                        Mode:CPOL时钟极性/CPHA时钟相位选择
                        Width:数据宽度选择
                        SpiIE:数据中断选择通过  eSPICtl_IE_TypeDef的枚举. 例如(SCSN_NEG_IRQ_EN|SCSN_POS_IRQ_EN)
                        speed:单位HZ     .例如:500000表示500KHZ
** output parameters:   null
** Returned value:      null
*********************************************************************************************************/
void LL_SPI_Init(SPI_TypeDef *SPIx, sLL_SPI_InitTypeDef *sSPI_Init)
{
    uint32_t u32ClkDiv;
    uint8_t u8ClkDiv;

    u32ClkDiv = SystemCoreClock; /*"获取当前系统时钟	"*/
    if ((2U * sSPI_Init->Speed) > u32ClkDiv) /*如果spi速率大于主频的一半，默认spi速率为主频的一半；主要针对客户主频为RCH1.6M场景*/
    {
      sSPI_Init->Ctrl.bitSpiCtrl.CLKDIV = 0;
      SPIx->CTRL = sSPI_Init->Ctrl.SpiCtrl;
    }
    else
    {
      u8ClkDiv = (uint8_t)(((u32ClkDiv) / (2U * (sSPI_Init->Speed))) - 1U);
      sSPI_Init->Ctrl.bitSpiCtrl.CLKDIV = u8ClkDiv;
      SPIx->CTRL = sSPI_Init->Ctrl.SpiCtrl;
    }
}
/*********************************************************************************************************
** Function name:       LL_SPI_Disable
** Descriptions:        关闭SPI模块
** input parameters:    SPIx：SPI模块
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_SPI_Disable(SPI_TypeDef *SPIx)
{
    SET_REG_SPI_CTRL(SPIx,0);
}

#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2)\
   || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3) \
    || defined(RN202x_RN7326_SOC_B) || defined(RN202x_RN7326_SOC_V2) || defined(RN831x_RN861x_MCU_V4)
/*********************************************************************************************************
** Function name:     LL_SPI_DMAInit
** Descriptions:    	普通SPI DMA    支持	SPI0	SPI1
** input parameters:    spi_id:第几个SPI;
                        SpiDmaCtlPara: DMA 控制寄存器.  通过  eSPI_DMACtlPara_TypeDef的枚举
                        TbAdr:发送起始地址
                        RbAdr:接收起始地址
                        TLen:DMA 发送长度
                        RLen:DMA 接收长度
                        SPIDmaIE:DMA中断配置.通过  eSPIDMA_IE_TypeDef的枚举. 例如(SPI_DMA_TX_HIE|SPI_DMA_TX_FIE)

** output parameters: null
** Returned value: 		null
*********************************************************************************************************/
void LL_SPI_DMAInit(SPI_TypeDef *SPIi, const sLL_SPIDMA_InitTypeDef *sSPIDMA_Init)
{
    SPIi->DMA_CTRL = 0;
    SPIi->DMA_TBADR = sSPIDMA_Init->TbAdr; /*offset = 0x1C*/
    SPIi->DMA_RBADR = sSPIDMA_Init->RbAdr; /*offset = 0x20*/
    SPIi->DMA_TLEN = sSPIDMA_Init->TLen;   /*offset = 0x24*/
    SPIi->DMA_RLEN = sSPIDMA_Init->RLen;   /*offset = 0x28*/
    SPIi->DMA_CTRL = sSPIDMA_Init->DmaCtrl.SpiDmaCtrl;
    SPIi->DMA_FLG = 0x3f;
    SPIi->DMA_IE = sSPIDMA_Init->DmaIE;
}

/*********************************************************************************************************
** Function name:     LL_SPI_DMA_SetRecAddr
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		ErrorStatus
*********************************************************************************************************/
ErrorStatus LL_SPI_DMA_SetRecAddr(SPI_TypeDef *SPIi, uint32_t u32Addr)
{   
  SPIi->DMA_RBADR = u32Addr;
  return ERN_SUCCESS;
}
/*********************************************************************************************************
** Function name:     LL_SPI_DMA_SetTransAddr
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		ErrorStatus
*********************************************************************************************************/
ErrorStatus LL_SPI_DMA_SetTransAddr(SPI_TypeDef *SPIi, uint32_t u32Addr)
{
    SPIi->DMA_TBADR = u32Addr;
    return ERN_SUCCESS;
}
/*********************************************************************************************************
** Function name:     LL_SPI_DMA_SetRlen
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		ErrorStatus
*********************************************************************************************************/
ErrorStatus LL_SPI_DMA_SetRlen(SPI_TypeDef *SPIi, uint32_t u32Len)
{
    SPIi->DMA_RLEN = u32Len;
    return ERN_SUCCESS;
}

/*********************************************************************************************************
** Function name:     LL_SPI_DMA_SetTlen
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		ErrorStatus
*********************************************************************************************************/
ErrorStatus LL_SPI_DMA_SetTlen(SPI_TypeDef *SPIi, uint32_t u32Len)
{
   SPIi->DMA_TLEN = u32Len;
   return ERN_SUCCESS;
}

/*********************************************************************************************************
** Function name:     LL_SPI_DMA_ConfigInt
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		ErrorStatus
*********************************************************************************************************/
void LL_SPI_DMA_IEConfig(SPI_TypeDef *SPIi, eSPIDMA_IE_TypeDef int_type, FunctionalState enNewState)
{
  if(ERN_ENABLE == enNewState)
  {
    SET_BIT_SPI_DMA_IE(SPIi,int_type);
  }
  else{
    CLR_BIT_SPI_DMA_IE(SPIi,int_type);
  }
}

/*********************************************************************************************************
** Function name:     LL_SPI_DMACtl
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		ErrorStatus
*********************************************************************************************************/
ErrorStatus LL_SPI_DMA_CtlCmd(SPI_TypeDef *SPIi, eSPIDMA_CtlPara_TypeDef ctl_type, FunctionalState enNewState)
{   
  if(ERN_ENABLE == enNewState)
  {
    SET_BIT_SPI_DMA_CTRL(SPIi,ctl_type);
  }
  else{
    CLR_BIT_SPI_DMA_CTRL(SPIi,ctl_type);
  } 
  return ERN_SUCCESS;
}
/*********************************************************************************************************
** Function name:     	LL_SPI_DMA_RxInfo
** Descriptions:
** input parameters:    null
** output parameters:   SPI DMA接收到的数据长度
** Returned value:      null
*********************************************************************************************************/
uint32_t LL_SPI_DMARxInfo(const SPI_TypeDef *SPIx)
{
    uint32_t rx_count;
    rx_count = SPIx->DMA_RADR;/* SPI DMA接收缓存当前地址 */
    rx_count -= SPIx->DMA_RBADR;/* SPI DMA接收缓存起始地址 */
    return (rx_count & SPI_DMAAdrLen_MASK);
}
/*********************************************************************************************************
** Function name:     	LL_SPI_DMA_TxInfo
** Descriptions:
** input parameters:    null
** output parameters:   SPI DMA发送的数据长度
** Returned value:      null
*********************************************************************************************************/
uint32_t LL_SPI_DMATxInfo(const SPI_TypeDef *SPIx)
{
    uint32_t tx_count;
    tx_count = SPIx->DMA_TADR;/* SPI DMA 发送缓存当前地址 */
    tx_count -= SPIx->DMA_TBADR;/* SPI DMA 发送缓存起始地址 */
    return (tx_count & SPI_DMAAdrLen_MASK);
}
#endif
#endif /*LL_SPI_MODULE_ENABLED*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
