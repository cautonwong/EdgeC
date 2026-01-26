/********************************************************************************
  * @file    rn8xxx_ll_iic.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-08-29
  * @brief   IIC driver
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
  ##### IIC peripheral features #####
  IIC通讯的时钟触发模式（读数据）
                        
  发送：
  byte1时钟由buscon.start命令触发
  byte2~byten时钟	由写trdata触发
  Byte(n+1)：由buscon.start命令触发 

  接收：
  Byte1：由清i2c_stat.tranc位触发
  Byte2~byten：由读trdata触发
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]


    @endverbatim
  */

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll.h"
#ifdef LL_IIC_MODULE_ENABLED
/* Private define ------------------------------------------------------------*/

/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/

static void LL_IIC_DelayUs(uint32_t i)
{
#ifdef RN831x_RN861x_MCU_V4
    if (((SYSTEM->SYSFREQ_STA >> 0) & 0x7u) != 0x0u) /* 高频模式*/
#else
    if (((SYSCTL->OSC_CTRL1 >> 8) & 0x3u) != 0x3u) /* 高频模式*/
#endif
    {
        for (; i > 0u; i--)
        {
            __NOP();
            __NOP();
            __NOP();
            __NOP();
            __NOP();
            __NOP();
            __NOP();
            __NOP();
        }
    }
    else
    {
        for (; i > 0u; i--)
        {
            __NOP();
            __NOP();
        }
    }
}
/*********************************************************************************************************
** Function name:     LL_IIC_ClkDiv_Get
** Descriptions:  IIC clk div get
** input parameters:  Clk_Khz: i2c speed
** output parameters:	null
** Returned value:   div
*********************************************************************************************************/
static uint8_t LL_IIC_ClkDiv_Get(eIICCLK_TypeDef Clk_Khz)
{
    uint8_t Clk_Offset,Div;

     /*800K,400K,200K,100K*/
    const uint8_t Div_Tab[5][4] =
    {
      {0x03U, 0x04U, 0x05U, 0x06U}, /*29.4Mhz  776Khz、388Khz、191Khz、96Khz  */
      {0x02U, 0x03U, 0x04U, 0x05U}, /*14.7Mhz  737Khz、388Khz、194Khz、95Khz  */
      {0x01U, 0x02U, 0x03U, 0x04U}, /*7.3Mhz   737Khz、388Khz、194Khz、95Khz  */
      {0x03U, 0x03U, 0x03U, 0x03U}, /*3.6Mhz   97Khz、97Khz、97Khz、97Khz  */
      {0x02U, 0x02U, 0x02U, 0x02U}, /*1.8Mhz,  92Khz、92Khz、92Khz、92Khz */
    };

    if( (Clk_Khz != I2C_100K) && (Clk_Khz != I2C_400K) )
    {
      Div = 0xFFu;
    }
    else
    {
      Clk_Offset = (uint8_t)Clk_Khz;

      switch (SystemCoreClock)
      {
      case 29491200U:
        Div = Div_Tab[0][Clk_Offset];
        break;
      case 14745600U:
        Div = Div_Tab[1][Clk_Offset];
        break;
      case 7372800U:
        Div = Div_Tab[2][Clk_Offset];
        break;
      case 3686400U:
        Div = Div_Tab[3][Clk_Offset];
        break;
      case 1843200U:
        Div = Div_Tab[4][Clk_Offset];
        break;
      default:
        Div = 0xFFu;
        break;
      }
    }
    return Div;
}

/*********************************************************************************************************
** Function name:     LL_IIC_HardMode_Init
** Descriptions:  IIC initial
** input parameters: clkdiv:division for IIC apb clock
** output parameters:	null
** Returned value:   null
*********************************************************************************************************/
ErrorStatus LL_IIC_HardMode_Init(I2C_TypeDef *I2Cx, eIICCLK_TypeDef ClkKhz)
{
    uint8_t Clk_Div;
    /* 开启I2C的APB时钟*/

    /* I2C分频参数 */
    Clk_Div = LL_IIC_ClkDiv_Get(ClkKhz);
    if (Clk_Div > 7u)
    {
       return ERN_ERROR;
    }
    /* I2C时钟配置*/
    I2Cx->CLK = Clk_Div;
    return ERN_SUCCESS;
}
/*********************************************************************************************************
** Function name:     LL_IIC_HardMode_Start
** Descriptions:  to triggle start signal matching IIC  standard
** input parameters: iic_id: communication id of slave iic
** output parameters:	null
** Returned value:  SUCCESS:  ;  ERN_ERROR:  ;
*********************************************************************************************************/
ErrorStatus LL_IIC_HardMode_Start(I2C_TypeDef *I2Cx, uint8_t iic_id)
{
    I2Cx->STA = 0x3fu;                                                              /* 清I2C状态*/
    I2Cx->ADDR = iic_id;                                                           /* 从I2C模块地址*/
    I2Cx->CTRL = 0x33u;                                                             /* I2C主模式下，启动I2C START时序及从I2C模块地址发送*/
    if (fnRegState_Detect((uint32_t)(&I2Cx->STA), 2u, 1u, IIC_TIMEOUT) == ERN_ERROR) /* 等待START时序及I2C模块地址传输完成*/
    {                                                                              /* 超时未完成START时序及I2C模块地址传输，则清标志寄存器并返回错误状态*/
        I2Cx->STA = 0x3fu;
        return ERN_ERROR;
    }
    if (0x02u == (I2Cx->STA & 0x02u))
    {
        I2Cx->STA = 0x3fu;
        return ERN_ERROR;
    }
    I2Cx->STA = 0x3fu;
    return ERN_SUCCESS;
}

/*********************************************************************************************************
** Function name:     LL_IIC_HardMode_Stop
** Descriptions:  to send stop signal matching IIC  standard
** input parameters: null
** output parameters:	null
** Returned value:   null
*********************************************************************************************************/
void LL_IIC_HardMode_Stop(I2C_TypeDef *I2Cx)
{
    if (0x100u == (I2Cx->STA & 0x100u))
    {                                                                                      /* I2C处于读操作状态下，则需待上一次传输完成后，才启动STOP时序*/
        if (fnRegState_Detect((uint32_t)(&I2Cx->STA), 2U, 1U, IIC_TIMEOUT * 2u) == ERN_ERROR) /* 等待传输是否完成*/
        {                                                                                  /* 超时传输未完成，则清状态字，清控制字，并返回*/
            I2Cx->STA = 0x3fu;
            I2Cx->CTRL = 0u;
            return;
        }
    }
    I2Cx->CTRL = 0x35u; /* 启动STOP时序发送*/

    if (fnRegState_Detect((uint32_t)(&I2Cx->STA), 0U, 1U, IIC_TIMEOUT) == ERN_ERROR) /* 等待STOP时序发送完成*/
    {
    }

    I2Cx->STA = 0x3fu;
    I2Cx->CTRL = 0u;
}
/*********************************************************************************************************
** Function name:reset IIC module
** Descriptions:
** input parameters:IIC pointer
** output parameters:
** Returned value:
*********************************************************************************************************/
void IIC_HardMode_Reset(I2C_TypeDef *I2Cx)
{

    LL_IIC_DelayUs(1u);
    LL_IIC_HardMode_Stop(I2Cx);
    LL_IIC_DelayUs(2u);
    LL_IIC_HardMode_Stop(I2Cx);
}
/*********************************************************************************************************
** Function name:     LL_IIC_HardMode_SendByte
** Descriptions:  to send data through IIC
** input parameters:  data: data to be sent
** output parameters:	null
** Returned value:   SUCCESS:  ;   ERROR:  ;
*********************************************************************************************************/
ErrorStatus LL_IIC_HardMode_SendByte(I2C_TypeDef *I2Cx,uint8_t data)
{
    I2Cx->STA = 0x3fu;
    I2Cx->DATA = data;
    if (fnRegState_Detect((uint32_t)(&I2Cx->STA), 2U, 1U, IIC_TIMEOUT) == ERN_ERROR) /* 等待I2C数据发送完成*/
    {                                                                              /* 超时数据未发送完成，清状态，并返回错误*/
        I2Cx->STA = 0x3fu;
        return ERN_ERROR;
    }
    if (0x20u == (I2Cx->STA & 0x20u))
    { /* 发送冲突，则清状态，并返回错误*/
        I2Cx->STA = 0x3fu;
        return ERN_ERROR;
    }
    I2Cx->STA = 0x3fu;
    return ERN_SUCCESS;
}
/*********************************************************************************************************
** Function name:
** Descriptions:  		IIC initial
** input parameters:  ACK: 1=don't change IIC->CTRL to keep the need for ack  as the last time;
                                                     0= change IIC->CTRL to don't triggle ack on the 9th clk                             
** output parameters:	null
** Returned value:   	null
*********************************************************************************************************/
uint8_t LL_IIC_HardMode_ReceiveByte(I2C_TypeDef *I2Cx,eACK_Enable_TypeDef ACK)
{
    uint8_t data = 0;		
	
    if(fnRegState_Detect((uint32_t)(&I2Cx->STA) , 2u , 1u , IIC_TIMEOUT*2) == ERN_ERROR)//等待数据接收完成
    {/*超时数据未接收完成，清状态，返回数据0*/
      I2Cx->STA = 0x3fu;		
      return (uint8_t)I2Cx->DATA;
    }
    if((I2Cx->STA & 0x20u) == 0x20u)
    {/*发生发送冲突，则清状态，返回数据0*/
      I2Cx->STA = 0x3f;		
      return (uint8_t)I2Cx->DATA;
    }		
    if((uint8_t)ACK != 1u)
    {	/*第9个CLK，不产生ACK响应*/
      I2Cx->CTRL &= 0xffffffefU;
    }
    SystemDelayUs(200);	/*用于等待修改Ctrl寄存器触发的第1个字节发送完*/
    I2Cx->STA = 0x3fu;
    SystemDelayUs(400);/*用于等待清STAT寄存器触发的第2个字节发送完*/
    data = (uint8_t)I2Cx->DATA;/*获取接收寄存器内的数据	*/
  
    return data;
}

/*********************************************************************************************************
** Function name:IIC_HardMode_Disable
** Descriptions:Disable I2C
** input parameters:I2Cx
** output parameters:null
** Returned value:null
*********************************************************************************************************/
void LL_IIC_HardMode_Disable(I2C_TypeDef *I2Cx)
{
    I2Cx->STA = 0x3fu;
    I2Cx->CTRL = 0u;
}
#endif /*LL_IIC_MODULE_ENABLED*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
