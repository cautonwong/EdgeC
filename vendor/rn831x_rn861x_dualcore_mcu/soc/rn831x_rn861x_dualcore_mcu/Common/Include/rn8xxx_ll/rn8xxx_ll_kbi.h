/********************************************************************************
 * @file    Rn8xxx_KBI.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-09-07
 * @brief   Header file containing functions prototypes of KBI HAL library.
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
#ifndef Rn8xxx_KBI_H
#define Rn8xxx_KBI_H

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll_def.h"
/* Exported define ------------------------------------------------------------*/

/*********************************************************************************/
#define GET_REG_KBI_CTRL(kbi, databits) ((kbi)->CTRL & GET_DATA_MASK(databits))
#define GET_REG_KBI_SEL(kbi, databits) ((kbi)->SEL & GET_DATA_MASK(databits))
#define GET_REG_KBI_DATA(kbi, databits) ((kbi)->DATA & GET_DATA_MASK(databits))
#define GET_REG_KBI_MASK(kbi, databits) ((kbi)->MASK & GET_DATA_MASK(databits))

#define SET_REG_KBI_CTRL(kbi, data) ((kbi)->CTRL = (data))
#define SET_REG_KBI_SEL(kbi, data) ((kbi)->SEL = (data))
#define SET_REG_KBI_DATA(kbi, data) ((kbi)->DATA = (data))
#define SET_REG_KBI_MASK(kbi, data) ((kbi)->MASK = (data))
/*********************************************************************************/

#define GET_BIT_KBI_CTRL(kbi, bit) (((kbi)->CTRL & (bit)) ? (1U) : (0U))
#define GET_BIT_KBI_SEL(kbi, bit) (((kbi)->SEL & (bit)) ? (1U) : (0U))
#define GET_BIT_KBI_DATA(kbi, bit) (((kbi)->DATA & (bit)) ? (1U) : (0U))
#define GET_BIT_KBI_MASK(kbi, bit) (((kbi)->MASK & (bit)) ? (1U) : (0U))

#define SET_BIT_KBI_CTRL(kbi, bit) ((kbi)->CTRL |= (bit))
#define SET_BIT_KBI_SEL(kbi, bit) ((kbi)->SEL |= (bit))
#define SET_BIT_KBI_DATA(kbi, bit) (BITBAND_ADDR((uint32_t)(&((kbi)->DATA)), (bit)) = 1U)
#define SET_BIT_KBI_MASK(kbi, bit) ((kbi)->MASK |= (bit))

#define CLR_BIT_KBI_CTRL(kbi, bit) ((kbi)->CTRL &= (~(bit)))
#define CLR_BIT_KBI_SEL(kbi, bit) ((kbi)->SEL &= (~(bit)))
#define CLR_BIT_KBI_DATA(kbi, bit) (SET_BIT_KBI_DATA((kbi), (bit)))
#define CLR_BIT_KBI_MASK(kbi, bit) ((kbi)->MASK &= (~(bit)))
/* Exported enum ------------------------------------------------------------*/
typedef enum
{
    KBI_ID0 = 0,
    KBI_ID1 = 1,
    KBI_ID2 = 2,
    KBI_ID3 = 3,
    KBI_ID4 = 4,
    KBI_ID5 = 5,
    KBI_ID6 = 6,
    KBI_ID7 = 7,
#ifdef RN831x_RN861x_MCU_V4
    KBI_ID8  = 8,
    KBI_ID9  = 9,
    KBI_ID10 = 10,
    KBI_ID11 = 11,
    KBI_ID12 = 12,
    KBI_ID13 = 13,
    KBI_ID14 = 14,
    KBI_ID15 = 15,
#endif
    KBI_IDMAX = 8,
} eKBI_ID_TypeDef; /*"KBI    ID号"*/

typedef enum
{
    KBI_RISIONGEDGE = 0,    /*"0：上升沿有效"*/
    KBI_FALLINGEDGE = 1,    /*"1：下降沿有效"*/
#ifdef RN831x_RN861x_MCU_V4
	KBI_RISING_FALLING = 2
#endif
} eKBI_TriggerEdge_TypeDef; /*"KBI     触发边沿"*/

typedef enum
{
    KBI_IRQ_ENABLE = 1,  /*"KBI 中断使能"*/
    KBI_IRQ_DISABLE = 0, /*"KBI 中断禁止"*/
} eKBI_IRQ_TypeDef;       /*"KBI 中断使能选择"*/

/* Exported types ------------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
extern void LL_KBI_Init(eKBI_ID_TypeDef KBI_Id, eKBI_TriggerEdge_TypeDef TriggerEdge, eKBI_IRQ_TypeDef IrqFlag);

extern void LL_KBI_Disable(eKBI_ID_TypeDef KBI_Id);
#endif /*KBI _INTC_H*/

/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
