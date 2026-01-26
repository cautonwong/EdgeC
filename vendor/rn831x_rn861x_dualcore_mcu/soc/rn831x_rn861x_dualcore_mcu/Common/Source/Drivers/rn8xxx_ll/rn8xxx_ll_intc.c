/********************************************************************************
  * @file    LL_INTC.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-08-29
  * @brief   外部中断模块驱动
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
                     ##### INTC peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]


    @endverbatim
  */

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll.h"
#ifdef LL_INTC_MODULE_ENABLED
/* Private define ------------------------------------------------------------*/

/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/*********************************************************************************************************
** Function name:       LL_Intc_IRQ
** Descriptions:        外部中断模块
** input parameters:    INTC_Id      IrqFlag
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2)||defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3)
static void LL_Intc_IRQ(eINTC_ID_TypeDef INTC_Id, eINTC_IRQ_TypeDef IrqFlag)
{
    IRQn_Type IRQn;

    /*"共用外部中断EXT0"*/
    if(GET_BIT_INTC_CTL(INTC, BIT8) != 0U)
    // if( ( (INTC->CTRL) & 0x100U) == 0x100U )
    {
        IRQn = EXT0_IRQn;

        if (INTC_IRQ_ENABLE == IrqFlag)
        {
            NVIC_EnableIRQ(IRQn);
        }
        else
        {
            if( ( (INTC->MASK) & 0xFFU) == 0U)
            {
                NVIC_DisableIRQ(IRQn);
            }
        }
    }
    else
    {
        switch (INTC_Id)
        {
            case INTC_ID0:
                IRQn = EXT0_IRQn;
                break;
            case INTC_ID1:
                IRQn = EXT1_IRQn;
                break;
            case INTC_ID2:
                IRQn = EXT2_IRQn;
                break;
            case INTC_ID3:
                IRQn = EXT3_IRQn;
                break;
            case INTC_ID4:
                IRQn = EXT4_IRQn;
                break;
            case INTC_ID5:
                IRQn = EXT5_IRQn;
                break;
            case INTC_ID6:
                IRQn = EXT6_IRQn;
                break;
            case INTC_ID7:
                IRQn = EXT7_IRQn;
                break;
            default:
                IRQn = NonMaskableInt_IRQn;
                break;
        }

        if (IRQn != NonMaskableInt_IRQn) {
            if (INTC_IRQ_ENABLE == IrqFlag)
            {
                NVIC_EnableIRQ(IRQn);
            }
            else
            {
                NVIC_DisableIRQ(IRQn);
            }
        }
    }
}
#elif defined(RN202x_RN7326_SOC_V2)
static void LL_Intc_IRQ(eINTC_ID_TypeDef INTC_Id, eINTC_IRQ_TypeDef IrqFlag)
{
    IRQn_Type IRQn;

    /*"共用外部中断EXT0"*/
	if(INTC_Id < INTC_IDMAX)
    {
        IRQn = EXT_IRQn;

        if (INTC_IRQ_ENABLE == IrqFlag)
        {
            NVIC_EnableIRQ(IRQn);
        }
        else
        {
            if( ( (INTC->MASK) & 0xFFU) == 0U)
            {
                NVIC_DisableIRQ(IRQn);
            }
        }
    }
}
#elif defined(RN831x_RN861x_MCU_V4)
static void LL_Intc_IRQ(eINTC_ID_TypeDef INTC_Id, eINTC_IRQ_TypeDef IrqFlag)
{
    IRQn_Type IRQn;

    switch (INTC_Id)
    {
        case INTC_ID0:
            IRQn = KBI0_IRQ0;
            break;
        case INTC_ID1:
            IRQn = KBI0_IRQ1;
            break;
        case INTC_ID2:
            IRQn = KBI0_IRQ2;
            break;
        case INTC_ID3:
            IRQn = KBI0_IRQ3;
            break;
        case INTC_ID4:
            IRQn = KBI0_IRQ4;
            break;
        case INTC_ID5:
            IRQn = KBI0_IRQ5;
            break;
        case INTC_ID6:
            IRQn = KBI0_IRQ6;
            break;
        case INTC_ID7:
            IRQn = KBI0_IRQ7;
            break;
        case INTC_ID8:
            IRQn = KBI0_IRQ8;
            break;
        case INTC_ID9:
            IRQn = KBI0_IRQ9;
            break;
        case INTC_ID10:
            IRQn = KBI0_IRQ10;
            break;
        case INTC_ID11:
            IRQn = KBI0_IRQ11;
            break;
        case INTC_ID12:
            IRQn = KBI0_IRQ12;
            break;
        case INTC_ID13:
            IRQn = KBI0_IRQ13;
            break;
        case INTC_ID14:
            IRQn = KBI0_IRQ14;
            break;
        case INTC_ID15:
            IRQn = KBI0_IRQ15;
            break;
        default:
            return;
    }

    if (INTC_IRQ_ENABLE == IrqFlag)
    {
        ECLIC_EnableIRQ(IRQn);
    }
    else
    {
        ECLIC_DisableIRQ(IRQn);
    }
}
#else
static void LL_Intc_IRQ(eINTC_ID_TypeDef INTC_Id, eINTC_IRQ_TypeDef IrqFlag)
{
    IRQn_Type IRQn;

    switch (INTC_Id)
    {
        case INTC_ID0:
            IRQn = EXT0_IRQn;
            break;
        case INTC_ID1:
            IRQn = EXT1_IRQn;
            break;
        case INTC_ID2:
            IRQn = EXT2_IRQn;
            break;
        case INTC_ID3:
            IRQn = EXT3_IRQn;
            break;
        case INTC_ID4:
            IRQn = EXT4_IRQn;
            break;
        case INTC_ID5:
            IRQn = EXT5_IRQn;
            break;
        case INTC_ID6:
            IRQn = EXT6_IRQn;
            break;
        case INTC_ID7:
            IRQn = EXT7_IRQn;
            break;
        default:
            return;
    }

    if (INTC_IRQ_ENABLE == IrqFlag)
    {
        NVIC_EnableIRQ(IRQn);
    }
    else
    {
        NVIC_DisableIRQ(IRQn);
    }
}
#endif
/* Exported functions ----------------------------------------------------------*/

/*********************************************************************************************************
** Function name:       LL_INTC_Init
** Descriptions:        外部中断模块
** input parameters:    IrqType  INTC_Id   TriggerEdge   IrqFlag
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2)||defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3)
void LL_INTC_Init(eINTC_Choice_IRQ_Typedef IrqType, eINTC_ID_TypeDef INTC_Id, eINTC_TriggerEdge_TypeDef TriggerEdge, eINTC_IRQ_TypeDef IrqFlag)
{
    uint32_t eId;

    eId = (uint32_t)INTC_Id;

    if (INTC_Id < INTC_IDMAX)
    {
        if(INTC_INDEPEND_EXTX == IrqType)
        {
            BITBAND_ADDR((uint32_t)(&INTC->CTRL), 8U) = 0U;               /*"独立外部中断EXTx"*/
        }
        else
        {
            BITBAND_ADDR((uint32_t)(&INTC->CTRL), 8U) = 1U;                /*"选择共用外部中断EXT0"*/
        }
        
        BITBAND_ADDR((uint32_t)(&INTC->CTRL), eId) = 1U;                    /*"外部中断模块使能"*/
        /*"上升沿&下降沿选择"*/
        {
            uint16_t Edge;
            Edge = (uint16_t)TriggerEdge;
            BITBAND_ADDR((uint32_t)(&INTC->MODE), (eId * 2U)) = (Edge & 0x01U); 
            BITBAND_ADDR((uint32_t)(&INTC->MODE), ((eId * 2U) + 1U)) = ((Edge >> 1U) & 0x01U);
        }

        BITBAND_ADDR((uint32_t)(&INTC->STA), eId) = 1U;        /*"中断事件清零"*/

        /*"配置中断使能"*/
        if(INTC_IRQ_ENABLE == IrqFlag)
        {
            BITBAND_ADDR((uint32_t)(&INTC->MASK), eId) = 1U; 
        }
        else
        {
            BITBAND_ADDR((uint32_t)(&INTC->MASK), eId) = 0U; 
        }

        LL_Intc_IRQ(INTC_Id, IrqFlag);
    }
}
#elif defined(RN831x_RN861x_MCU_V4)
void LL_INTC_Init(eINTC_Choice_KBI_Typedef KBI_Id, eINTC_ID_TypeDef INTC_Id, eINTC_TriggerEdge_TypeDef TriggerEdge, eINTC_IRQ_TypeDef IrqFlag)
{
    uint32_t eId=0;
    KBI_TypeDef *INTC = 0;

    if (KBI_Id == 0) {
        INTC = KBI0;
    }
    else if (KBI_Id == 1) {
        INTC = KBI1;
    }
    eId = (uint32_t)INTC_Id;

    if (INTC_Id < INTC_IDMAX)
    {
        BITBAND_ADDR((uint32_t)(&INTC->CTRL), eId) = 1U;             /*"外部中断模块使能"*/
        /*"上升沿&下降沿选择"*/
        {
            uint16_t Edge=0;
            Edge = (uint16_t)TriggerEdge;
            BITBAND_ADDR((uint32_t)(&INTC->MODE), (eId * 2U)) = (Edge & 0x01U);
            BITBAND_ADDR((uint32_t)(&INTC->MODE), (eId * 2U + 1U)) = ((Edge >> 1U) & 0x01U);
        }

        BITBAND_ADDR((uint32_t)(&INTC->STA), eId) = 1U;        /*"中断事件清零"*/

        /*"配置中断使能"*/
        if(INTC_IRQ_ENABLE == IrqFlag)
        {
            BITBAND_ADDR((uint32_t)(&INTC->MASK), eId) = 1U;
        }
        else
        {
            BITBAND_ADDR((uint32_t)(&INTC->MASK), eId) = 0U;
        }
        /* 配置外部中断模式 */
        __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&INTC->CH_FLT_TIME[INTC_Id]) + (4 * INTC_Id)));
        if ((((SYSTEM->SYSFREQ_STA >> 0) & 0x7u) != 0x0u)) {
            MODIFY_REG(*pReg, KBI_CH_FLT_TIME_CLK_SEL | KBI_CH_FLT_TIME_CLK_DIV1 | KBI_CH_FLT_TIME_CLK_DIV2, 0x47);
        } else {
            MODIFY_REG(*pReg, KBI_CH_FLT_TIME_CLK_SEL | KBI_CH_FLT_TIME_CLK_DIV1 | KBI_CH_FLT_TIME_CLK_DIV2, 0x44);
        }

        LL_Intc_IRQ(INTC_Id, IrqFlag);
    }
}

#else
void LL_INTC_Init(eINTC_Choice_IRQ_Typedef IrqType,eINTC_ID_TypeDef INTC_Id, eINTC_TriggerEdge_TypeDef TriggerEdge, eINTC_IRQ_TypeDef IrqFlag)
{
    uint32_t eId=0;

    eId = (uint32_t)INTC_Id;

    if (INTC_Id < INTC_IDMAX)
    {
               
        BITBAND_ADDR((uint32_t)(&INTC->CTRL), eId) = 1U;             /*"外部中断模块使能"*/
        /*"上升沿&下降沿选择"*/
        {
            uint16_t Edge=0;
            Edge = (uint16_t)TriggerEdge;
            BITBAND_ADDR((uint32_t)(&INTC->MODE), (eId * 2U)) = (Edge & 0x01U); 
            BITBAND_ADDR((uint32_t)(&INTC->MODE), (eId * 2U + 1U)) = ((Edge >> 1U) & 0x01U);
        }

        BITBAND_ADDR((uint32_t)(&INTC->STA), eId) = 1U;        /*"中断事件清零"*/

        /*"配置中断使能"*/
        if(INTC_IRQ_ENABLE == IrqFlag)
        {
            BITBAND_ADDR((uint32_t)(&INTC->MASK), eId) = 1U; 
        }
        else
        {
            BITBAND_ADDR((uint32_t)(&INTC->MASK), eId) = 0U; 
        }

        LL_Intc_IRQ(INTC_Id, IrqFlag);
    }
}

#endif

/*********************************************************************************************************
** Function name:       LL_INTC_Disable
** Descriptions:        外部中断模块
** input parameters:    INTC_Id
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
#if defined(RN831x_RN861x_MCU_V4)
void LL_INTC_Disable(eINTC_ID_TypeDef INTC_Id)
{
    uint32_t eId;
    KBI_TypeDef *INTC;

    INTC = KBI0;
    eId = (uint32_t)INTC_Id;

    BITBAND_ADDR((uint32_t)(&INTC->CTRL), eId) = 0U; 
    BITBAND_ADDR((uint32_t)(&INTC->STA), eId) = 1U;        /*"中断事件清零"*/
    BITBAND_ADDR((uint32_t)(&INTC->MASK), eId) = 0U; 
    
    LL_Intc_IRQ(INTC_Id, INTC_IRQ_DISABLE);
}
#else
void LL_INTC_Disable(eINTC_ID_TypeDef INTC_Id)
{
    uint32_t eId;

    eId = (uint32_t)INTC_Id;

    BITBAND_ADDR((uint32_t)(&INTC->CTRL), eId) = 0U; 
    BITBAND_ADDR((uint32_t)(&INTC->STA), eId) = 1U;        /*"中断事件清零"*/
    BITBAND_ADDR((uint32_t)(&INTC->MASK), eId) = 0U; 
    
    LL_Intc_IRQ(INTC_Id, INTC_IRQ_DISABLE);
}
#endif
#endif /*LL_INTC_MODULE_ENABLED*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
