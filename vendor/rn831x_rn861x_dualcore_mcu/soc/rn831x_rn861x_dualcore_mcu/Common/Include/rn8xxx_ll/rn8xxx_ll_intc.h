/********************************************************************************
 * @file    rn8xxx_intc.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-09-07
 * @brief   Header file containing functions prototypes of INTC HAL library.
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
#ifndef Rn8xxx_INTC_H
#define Rn8xxx_INTC_H

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll_def.h"
/* Exported define ------------------------------------------------------------*/

/*********************************************************************************/
#define GET_REG_INTC_CTL(intc, databits) ((intc)->CTRL & GET_DATA_MASK(databits))
#define GET_REG_INTC_MODE(intc, databits) ((intc)->MODE & GET_DATA_MASK(databits))
#define GET_REG_INTC_MASK(intc, databits) ((intc)->MASK & GET_DATA_MASK(databits))
#define GET_REG_INTC_STA(intc, databits) ((intc)->STA & GET_DATA_MASK(databits))

#define SET_REG_INTC_CTL(intc, data) ((intc)->CTRL = (data))
#define SET_REG_INTC_MODE(intc, data) ((intc)->MODE = (data))
#define SET_REG_INTC_MASK(intc, data) ((intc)->MASK = (data))
#define SET_REG_INTC_STA(intc, data) ((intc)->STA = (data))
/*********************************************************************************/

#define GET_BIT_INTC_CTL(intc, bit) ((((intc)->CTRL & (bit)) == (bit))? (1U) : (0U))
#define GET_BIT_INTC_MODE(intc, bit) ((((intc)->MODE & (bit)) == (bit))? (1U) : (0U))
#define GET_BIT_INTC_MASK(intc, bit) ((((intc)->MASK & (bit)) == (bit))? (1U) : (0U))
#define GET_BIT_INTC_STA(intc, bit) ((((intc)->STA & (bit)) == (bit)) ? (1U) : (0U))

#define SET_BIT_INTC_CTL(intc, bit) ((intc)->CTRL |= (bit))
#define SET_BIT_INTC_MODE(intc, bit) ((intc)->MODE |= (bit))
#define SET_BIT_INTC_MASK(intc, bit) ((intc)->MASK |= (bit))
#define SET_BIT_INTC_STA(intc, bit) (BITBAND_ADDR((uint32_t)(&((intc)->STA)), (bit)) = 1U)

#define CLR_BIT_INTC_CTL(intc, bit) ((intc)->CTRL &= (~(bit)))
#define CLR_BIT_INTC_MODE(intc, bit) ((intc)->MODE &= (~(bit)))
#define CLR_BIT_INTC_MASK(intc, bit) ((intc)->MASK &= (~(bit)))
#define CLR_BIT_INTC_STAT(intc, bit) (SET_BIT_INTC_STA((intc), (bit)))
/* Exported enum ------------------------------------------------------------*/
#if defined(RN831x_RN861x_MCU_V4)
typedef enum
{
    INTC_ID0  = 0,
    INTC_ID1  = 1,
    INTC_ID2  = 2,
    INTC_ID3  = 3,
    INTC_ID4  = 4,
    INTC_ID5  = 5,
    INTC_ID6  = 6,
    INTC_ID7  = 7,
    INTC_ID8  = 8,
    INTC_ID9  = 9,
    INTC_ID10 = 10,
    INTC_ID11 = 11,
    INTC_ID12 = 12,
    INTC_ID13 = 13,
    INTC_ID14 = 14,
    INTC_ID15 = 15,
    INTC_IDMAX
} eINTC_ID_TypeDef; /*"INTC       ID号"*/
#else
typedef enum
{
    INTC_ID0 = 0,
    INTC_ID1 = 1,
    INTC_ID2 = 2,
    INTC_ID3 = 3,
    INTC_ID4 = 4,
    INTC_ID5 = 5,
    INTC_ID6 = 6,
    INTC_ID7 = 7,
    INTC_IDMAX = 8,
} eINTC_ID_TypeDef; /*"INTC       ID号"*/

#endif

typedef enum
{
    INTC_RISINGEDGE = 0,     /*"00：上升沿"*/
    INTC_FALLINGEDGE = 1,    /*"01：下降沿"*/
    INTC_DOUBLEEDGE = 2,     /*"10：双边沿"*/
    INTC_NOEDGE = 3,          /*"11：保留"*/
} eINTC_TriggerEdge_TypeDef; /*"INTC     触发边沿"*/

typedef enum
{
    INTC_IRQ_ENABLE = 1,  /*"INTC 中断使能"*/
    INTC_IRQ_DISABLE = 0, /*"INTC 中断禁止"*/
} eINTC_IRQ_TypeDef;      /*"INTC 中断使能选择"*/

typedef enum
{
    INTC_INDEPEND_EXTX = 0,     /*"独立外部中断EXTx"*/
    INTC_SHARE_EXT0 = 1,        /*"共用外部中断EXT0"*/
} eINTC_Choice_IRQ_Typedef;

#if defined(RN831x_RN861x_MCU_V4)
typedef enum
{
    INTC_KBI_ID0 = 0,      /*" USE kbi0"*/
	INTC_KBI_ID1 = 1,      /*" USE kbi1"*/
} eINTC_Choice_KBI_Typedef;
#endif

/* Exported types ------------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
#if defined(RN831x_RN861x_MCU_V4)
extern void LL_INTC_Init(eINTC_Choice_KBI_Typedef KBI_Id, eINTC_ID_TypeDef INTC_Id, eINTC_TriggerEdge_TypeDef TriggerEdge, eINTC_IRQ_TypeDef IrqFlag);
#else
extern void LL_INTC_Init(eINTC_Choice_IRQ_Typedef IrqType, eINTC_ID_TypeDef INTC_Id, eINTC_TriggerEdge_TypeDef TriggerEdge, eINTC_IRQ_TypeDef IrqFlag);
#endif

extern void LL_INTC_Disable(eINTC_ID_TypeDef INTC_Id);
#endif /*END _INTC_H*/

/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
