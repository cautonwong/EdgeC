/********************************************************************************
  * @file    Rn8xxx_ISO7816.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-08-29
  * @brief   ISO7861 driver
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
                     ##### ISO7861 peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]


    @endverbatim
  */
/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll.h"
#ifdef LL_ISO7816_MODULE_ENABLED
#if !defined(RN821x_RN721x_SOC_V3)
/* Private define ------------------------------------------------------------*/
#define ISO7816_TIMEOUT 0xfff
/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/
/*********************************************************************************************************
** Function name:     	LL_ISO7816_Init
** Descriptions:       	初始化7816
** input parameters:    None
** output parameters:       None
** Returned value:     None
*********************************************************************************************************/
void LL_ISO7816_Init(eISO7816_ID_TypeDef id)
{
#ifdef RN831x_RN861x_MCU_V4
  if(id == ISO7816_0ID)
  {
    ISO78160->CTRL = 0x1800ea01U;
    // ISO7816->BDDIV0 |= BIT21;
    ISO78160->STA = 0x3fffU; /* clear status */
  }
  else
  {
    //ISO7816->CTRL1 = 0x70000201U;
    //ISO7816->STA1 = 0x3ffU; /* clear status */
    ISO78161->CTRL = 0x1800ea01U;
    ISO78161->STA = 0x3fffU; /* clear status */
  }   
#else
  if(id == ISO7816_0ID)
  {
    ISO7816->CTRL0 = 0x0800ea01U;
    // ISO7816->BDDIV0 |= BIT21;
    ISO7816->STA0 = 0xffU; /* clear status */
  }
  else
  {
    //ISO7816->CTRL1 = 0x70000201U;
    //ISO7816->STA1 = 0x3ffU; /* clear status */
    ISO7816->CTRL1 = 0x1200ea01U;
    ISO7816->STA1 = 0x3ffU; /* clear status */
  }
#endif
}


/*********************************************************************************************************
** Function name:     	LL_ISO7816_CLK_Disable
** Descriptions:       	7816时钟脚输出关闭
** input parameters:    None
** output parameters:       None
** Returned value:     None
*********************************************************************************************************/
void LL_ISO7816_CLK_Disable(void)
{
#ifdef RN831x_RN861x_MCU_V4
  ISO78160->CLK &= (~BIT3);
#else
  ISO7816->CLK &= (~BIT3);
#endif

}

/*********************************************************************************************************
** Function name:     	LL_ISO7816_SquareWave_Init
** Descriptions:       	初始化7816时钟脚输出方波信号
** input parameters:    SquareWaveClock 方波频率(HZ)
** output parameters:       None
** Returned value:     None
*********************************************************************************************************/
ErrorStatus LL_ISO7816_SquareWave_Init(eISO7816_Clk_TypeDef SquareWaveClock)
{
	uint32_t ClkDiv;
  ErrorStatus res = ERN_SUCCESS;
#ifdef RN831x_RN861x_MCU_V4
  ISO78160->CTRL |= 0x01U;
#else
  ISO7816->CTRL1 |= 0x01U;
#endif
  ClkDiv = (uint32_t)SquareWaveClock;   
  if(ClkDiv != 0U)  
    ClkDiv = SystemCoreClock / ClkDiv;

#ifdef RN831x_RN861x_MCU_V4
  switch (ClkDiv)
  {
    case 1:
        ISO78160->CLK = (BIT3);               /* 不分频 */
        break;
      case 2:
        ISO78160->CLK = (BIT3 | BIT0);      /* 2分频 */
        break;
      case 4:
        ISO78160->CLK = (BIT3 | BIT1);      /* 4分频 */
        break;
      case 8:
        ISO78160->CLK = (BIT3 |BIT1| BIT0); /* 8分频 */
        break;
      case 16:
        ISO78160->CLK = (BIT3 |BIT2);       /* 16分频 */
        break;
      default:
        res = ERN_ERROR;
        break;      
  }
#else
  switch (ClkDiv)
  {
    case 1:
        ISO7816->CLK = (BIT3);               /* 不分频 */
        break;
      case 2:
        ISO7816->CLK = (BIT3 | BIT0);      /* 2分频 */
        break;
      case 4:
        ISO7816->CLK = (BIT3 | BIT1);      /* 4分频 */
        break;
      case 8:
        ISO7816->CLK = (BIT3 |BIT1| BIT0); /* 8分频 */
        break;
      case 16:
        ISO7816->CLK = (BIT3 |BIT2);       /* 16分频 */
        break;
      default:
        res = ERN_ERROR;
        break;
  }


#endif
  return res; 
  
}
/*********************************************************************************************************
** Function name:     	fnCardGetByte
** Descriptions:       	从卡或ESAM接收一个8位字节
** input parameters:    Data 为要接收的8位数据  ,CH 通道选择 Rpt 重新接收次?
** output parameters:       None
** Returned value:      接收成功后等待1个ETU返回，从检测到启始位到成功返回时长大于11个ETU
*********************************************************************************************************/
ErrorStatus LL_ISO7816_ReadByte(eISO7816_ID_TypeDef id, uint8_t *Data)
{
#ifdef RN831x_RN861x_MCU_V4
    if (id == ISO7816_0ID)
    {
        ISO78160->CTRL &= 0xfffffffbU;
        ISO78160->CTRL |= 0x08U;
        ISO78160->STA = 0x3fU;

        if (fnRegState_Detect((uint32_t)(&ISO78160->STA), 8, 1, ISO7816_TIMEOUT * 2) == ERN_ERROR)
        {
            ISO78160->STA = 0x3fU;
            return ERN_ERROR;
        }

        if ((ISO78160->STA & 0x10U) != 0x0U)
        {
            ISO78160->STA = 0x3fU; /* clear err flag */
            return ERN_ERROR;
        }
        *Data = (uint8_t)ISO78160->DATA;
        ISO78160->STA = 0x3fU;
    }
    else
    {
        ISO78160->CTRL &= 0xfffffffbU;
        ISO78160->CTRL |= 0x08U;
        ISO78160->STA = 0x3fU;

        if (fnRegState_Detect((uint32_t)(&ISO78160->STA), 8, 1, ISO7816_TIMEOUT * 2) == ERN_ERROR)
        {
            ISO78160->STA = 0x3fU;
            return ERN_ERROR;
        }

        if ((ISO78160->STA & 0x10U) != 0x0U)
        {
            ISO78160->STA = 0x3fU; /* clear err flag */
            return ERN_ERROR;
        }
        *Data = (uint8_t)ISO78160->DATA;
        ISO78160->STA = 0x3fU;
    }
#else
    if (id == ISO7816_0ID)
    {
        ISO7816->CTRL0 &= 0xfffffffbU;
        ISO7816->CTRL0 |= 0x08U;
        ISO7816->STA0 = 0x3fU;

        if (fnRegState_Detect((uint32_t)(&ISO7816->STA0), 8, 1, ISO7816_TIMEOUT * 2) == ERN_ERROR)
        {
            ISO7816->STA0 = 0x3fU;
            return ERN_ERROR;
        }

        if ((ISO7816->STA0 & 0x10U) != 0x0U)
        {
            ISO7816->STA0 = 0x3fU; /* clear err flag */
            return ERN_ERROR;
        }
        *Data = (uint8_t)ISO7816->DATA0;
        ISO7816->STA0 = 0x3fU;
    }
    else
    {
        ISO7816->CTRL1 &= 0xfffffffbU;
        ISO7816->CTRL1 |= 0x08U;
        ISO7816->STA1 = 0x3fU;

        if (fnRegState_Detect((uint32_t)(&ISO7816->STA1), 8, 1, ISO7816_TIMEOUT * 2) == ERN_ERROR)
        {
            ISO7816->STA1 = 0x3fU;
            return ERN_ERROR;
        }

        if ((ISO7816->STA1 & 0x10U) != 0x0U)
        {
            ISO7816->STA1 = 0x3fU; /* clear err flag */
            return ERN_ERROR;
        }
        *Data = (uint8_t)ISO7816->DATA1;
        ISO7816->STA1 = 0x3fU;
    }
#endif
    return ERN_SUCCESS;
}

/*********************************************************************************************************
** Function name:     	fnCardSendByte
** Descriptions:       	   向卡或ESAM发送一个8位字节
** input parameters:       c 为要发送的8位数据
** output parameters:       None
** Returned value:         0为正确，1为不正确
*********************************************************************************************************/
ErrorStatus LL_ISO7816_WriteByte(eISO7816_ID_TypeDef id, uint8_t Data)
{

#ifdef RN831x_RN861x_MCU_V4
    if (id == ISO7816_0ID)
    {

        //ISO7816->CTRL0 &= 0xfffffff7U;
        ISO78160->CTRL |= 0x04U;
        ISO78160->STA = 0x3fU;
        ISO78160->DATA = Data;
        if (fnRegState_Detect((uint32_t)(&ISO78160->STA), 6, 0, ISO7816_TIMEOUT * 2) == ERN_ERROR)
        {
            ISO78160->STA = 0x3fU;
            return ERN_ERROR;
        }

        if ((ISO78160->STA & 0x28U) != 0U) /* 判断发送错误标志 */
        {
            ISO78160->STA = 0x3fU; /* clear err flag */
            return ERN_ERROR;
        }
        ISO78160->STA = 0x3fU;
    }
    else
    {
        //ISO7816->CTRL1 &= 0xfffffff7U;
        ISO78160->CTRL |= 0x04U;
        ISO78160->STA = 0x3fU;
        ISO78160->DATA = Data;

        if (fnRegState_Detect((uint32_t)(&ISO78160->STA), 6, 0, ISO7816_TIMEOUT * 2) == ERN_ERROR)
        {
            ISO78160->STA = 0x3fU;
            return ERN_ERROR;
        }

        if ((ISO78160->STA & 0x28U) != 0x0U) /* 判断发送错误标志 */
        {
            ISO78160->STA = 0x3fU; /* clear err flag */
            return ERN_ERROR;
        }
        ISO78160->STA = 0x3fU;
    }
#else
    if (id == ISO7816_0ID)
    {

        //ISO7816->CTRL0 &= 0xfffffff7U;
        ISO7816->CTRL0 |= 0x04U;
        ISO7816->STA0 = 0x3fU;
        ISO7816->DATA0 = Data;
        if (fnRegState_Detect((uint32_t)(&ISO7816->STA0), 6, 0, ISO7816_TIMEOUT * 2) == ERN_ERROR)
        {
            ISO7816->STA0 = 0x3fU;
            return ERN_ERROR;
        }

        if ((ISO7816->STA0 & 0x28U) != 0U) /* 判断发送错误标志 */
        {
            ISO7816->STA0 = 0x3fU; /* clear err flag */
            return ERN_ERROR;
        }
        ISO7816->STA0 = 0x3fU;
    }
    else
    {
        //ISO7816->CTRL1 &= 0xfffffff7U;
        ISO7816->CTRL1 |= 0x04U;
        ISO7816->STA1 = 0x3fU;
        ISO7816->DATA1 = Data;

        if (fnRegState_Detect((uint32_t)(&ISO7816->STA1), 6, 0, ISO7816_TIMEOUT * 2) == ERN_ERROR)
        {
            ISO7816->STA1 = 0x3fU;
            return ERN_ERROR;
        }

        if ((ISO7816->STA1 & 0x28U) != 0x0U) /* 判断发送错误标志 */
        {
            ISO7816->STA1 = 0x3fU; /* clear err flag */
            return ERN_ERROR;
        }
        ISO7816->STA1 = 0x3fU;
    }
#endif
    return ERN_SUCCESS;
}
/*********************************************************************************************************
** Function name:     	LL_ISO7816_Disable
** Descriptions:       	
** input parameters:    None
** output parameters:       None
** Returned value:     None
*********************************************************************************************************/
void LL_ISO7816_Disable(eISO7816_ID_TypeDef id)
{ 
#ifdef RN831x_RN861x_MCU_V4
    if (id == ISO7816_0ID)
    {
      ISO78160->CTRL &= (~BIT0);
    }
    else
    {
      ISO78160->CTRL &= (~BIT0);
    } 
#else
    if (id == ISO7816_0ID)
    {
      ISO7816->CTRL0 &= (~BIT0);
    }
    else
    {
      ISO7816->CTRL1 &= (~BIT0);
    }
#endif
}
#endif
#endif
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
