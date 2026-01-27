/********************************************************************************
  * @file    LL_KBI.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-08-29
  * @brief   KBI按键模块驱动
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
                     ##### KBI peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]


    @endverbatim
  */

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll.h"
#ifdef LL_KBI_MODULE_ENABLED
/* Private define ------------------------------------------------------------*/

/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    KBI_Id   TriggerEdge   IrqFlag
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_KBI_Init(eKBI_ID_TypeDef KBI_Id, eKBI_TriggerEdge_TypeDef TriggerEdge, eKBI_IRQ_TypeDef IrqFlag)
{
    uint32_t eId;
#ifdef RN831x_RN861x_MCU_V4
    eId = (uint32_t)KBI_Id;
    KBI_TypeDef *INTC = KBI0;

    if (KBI_Id < KBI_IDMAX)
    {
        BITBAND_ADDR((uint32_t)(&KBI0->CTRL), eId) = 1U;          /*"KBI模块使能"*/

        /*"上升沿&下降沿选择"*/
        if(KBI_FALLINGEDGE == TriggerEdge)
        {
            BITBAND_ADDR((uint32_t)(&KBI0->MODE), eId) = 1U;
        }
        else
        {
            BITBAND_ADDR((uint32_t)(&KBI0->MODE), eId) = 0U;
        }

        BITBAND_ADDR((uint32_t)(&KBI0->STA), eId) = 1U;       /*"KBI事件清零"*/

        /*"配置中断使能"*/
        if(KBI_IRQ_ENABLE == IrqFlag)
        {
            BITBAND_ADDR((uint32_t)(&KBI0->MASK), eId) = 1U;
        }
        else
        {
            BITBAND_ADDR((uint32_t)(&KBI0->MASK), eId) = 0U;
        }

        __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&INTC->CH_FLT_TIME[KBI_Id]) + (4 * KBI_Id)));
        MODIFY_REG(*pReg, KBI_CH_FLT_TIME_CLK_SEL | KBI_CH_FLT_TIME_CLK_DIV1 | KBI_CH_FLT_TIME_CLK_DIV2, 0x33);

        if(((KBI0->MASK) & 0xFFU) != 0U)
        {
            ECLIC_EnableIRQ(KBI0_IRQn);
        }
    }
#else
    eId = (uint32_t)KBI_Id;

    if (KBI_Id < KBI_IDMAX)
    {
        BITBAND_ADDR((uint32_t)(&KBI->CTRL), eId) = 1U;          /*"KBI模块使能"*/

        /*"上升沿&下降沿选择"*/
        if(KBI_FALLINGEDGE == TriggerEdge)
        {
            BITBAND_ADDR((uint32_t)(&KBI->SEL), eId) = 1U;
        }
        else
        {
            BITBAND_ADDR((uint32_t)(&KBI->SEL), eId) = 0U;
        }

        BITBAND_ADDR((uint32_t)(&KBI->DATA), eId) = 1U;       /*"KBI事件清零"*/

        /*"配置中断使能"*/
        if(KBI_IRQ_ENABLE == IrqFlag)
        {
            BITBAND_ADDR((uint32_t)(&KBI->MASK), eId) = 1U;
        }
        else
        {
            BITBAND_ADDR((uint32_t)(&KBI->MASK), eId) = 0U;
        }

        if(((KBI->MASK) & 0xFFU) != 0U)
        {
            NVIC_EnableIRQ(KBI_IRQn);
        }
    }
#endif
}
/*********************************************************************************************************
** Function name:       LL_KBI_Disable
** Descriptions:        KBI 按键模块
** input parameters:    KBI_Id   
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_KBI_Disable(eKBI_ID_TypeDef KBI_Id)
{
    uint32_t eId;

    eId = (uint32_t)KBI_Id;
#ifdef RN831x_RN861x_MCU_V4
    BITBAND_ADDR((uint32_t)(&KBI0->CTRL), eId) = 0U;          /*"关闭外部中断模块"*/
    BITBAND_ADDR((uint32_t)(&KBI0->MODE), eId) = 0U;
    BITBAND_ADDR((uint32_t)(&KBI0->STA), eId) = 1U;           /*"中断事件清零"*/
    BITBAND_ADDR((uint32_t)(&KBI0->MASK), eId) = 0U;
#else
    BITBAND_ADDR((uint32_t)(&KBI->CTRL), eId) = 0U;          /*"关闭外部中断模块"*/
    BITBAND_ADDR((uint32_t)(&KBI->SEL), eId) = 0U;
    BITBAND_ADDR((uint32_t)(&KBI->DATA), eId) = 1U;           /*"中断事件清零"*/
    BITBAND_ADDR((uint32_t)(&KBI->MASK), eId) = 0U;

#endif
}
#endif /*LL_KBI_MODULE_ENABLED*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
