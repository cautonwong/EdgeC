/******************************************************************************
 * @file    Rn8xxx_TC.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-09-07
 * @brief   Header file containing functions prototypes of Timer HAL library.
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
#ifndef SIMPTC_H_
#define SIMPTC_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "rn8xxx_ll_def.h"

#if defined(RN821x_RN721x_SOC_V2) || \
    defined(RN821x_RN721x_SOC_V3) || \
    defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3) || \
    defined(RN831x_RN861x_MCU_V4) || \
    defined(RN202x_RN7326_SOC_V2)
/* Private define ------------------------------------------------------------*/

/*********************************************************************************/

#if defined(RN831x_RN861x_MCU_V4)
#define GET_REG_SIMPTC_LOAD(smtc, databits) ((smtc)->DN & GET_DATA_MASK(databits))
#define SET_REG_SIMPTC_LOAD(smtc, data) ((smtc)->DN = (data))
#else
#define GET_REG_SIMPTC_LOAD(smtc, databits) ((smtc)->LOAD & GET_DATA_MASK(databits))
#define SET_REG_SIMPTC_LOAD(smtc, data) ((smtc)->LOAD = (data))
#endif

#define GET_REG_SIMPTC_CTRL(smtc, databits) ((smtc)->CTRL & GET_DATA_MASK(databits))
#define GET_REG_SIMPTC_VAL(smtc, databits) ((smtc)->VAL & GET_DATA_MASK(databits))

#define SET_REG_SIMPTC_CTRL(smtc, data) ((smtc)->CTRL = (data))
/*#define SET_REG_SIMPTC_VAL(smtc, data) ((smtc)->VAL = (data))*/

#define GET_BIT_SIMPTC_CTRL(smtc, bit) (((smtc)->CTRL & (bit)) ? (1) : (0))
#define SET_BIT_SIMPTC_CTRL(smtc, bit) ((smtc)->CTRL |= (bit))
#define CLR_BIT_SIMPTC_CTRL(smtc, bit) ((smtc)->CTRL &= ~(bit))

#define LL_SIMPTC_START(smtc) (SET_BIT_SIMPTC_CTRL((smtc), SIMP_TC_CTRL_EN))
#define LL_SIMPTC_STOP(smtc) (CLR_BIT_SIMPTC_CTRL((smtc), SIMP_TC_CTRL_EN))

#define LL_SIMPTC_IRQ_EN(smtc) (SET_BIT_SIMPTC_CTRL((smtc), SIMP_TC_CTRL_IRQEN))
#define LL_SIMPTC_IRQ_DIS(smtc) (CLR_BIT_SIMPTC_CTRL((smtc), SIMP_TC_CTRL_IRQEN))

typedef enum
{
    SIMPTC_CTRL_DIS = 0,
    SIMPTC_CTRL_EN = 1,

    SIMPTC_CTRL_OV_DIS = 0,
    SIMPTC_CTRL_OV_EN = 1,

    SIMPTC_CTRL_MODE_DIS = 0,
    SIMPTC_CTRL_MODE_EN = 1,

    SIMPTC_CTRL_IRQ_DIS = 0,
    SIMPTC_CTRL_IRQ_EN = 1,
} eSimpTcCtrl_TypeDef;

typedef union
{
    struct
    {
        uint32_t EN : 1;
        uint32_t OV : 1;
        uint32_t MODE : 1;
        uint32_t IRQEN : 1;
    }bitSimpTcCtrl;
    uint32_t SimpTcCtrl;
} uSimpTcCtrl_TypeDef;

typedef struct
{
    uSimpTcCtrl_TypeDef Ctrl;
    uint16_t mS;
    uint16_t uS;
} sLL_SIMPTC_InitTypeDef;

#if defined(RN831x_RN861x_MCU_V4)
extern ErrorStatus LL_SIMPTC_Init(SIMPTC_TypeDef *SIMP_TCx, const sLL_SIMPTC_InitTypeDef *sSIMPTC_Init);
extern void LL_SIMPTC_Disable(SIMPTC_TypeDef *SIMP_TCx);
#else
extern ErrorStatus LL_SIMPTC_Init(SIMP_TC_TypeDef *SIMP_TCx, const sLL_SIMPTC_InitTypeDef *sSIMPTC_Init);
extern void LL_SIMPTC_Disable(SIMP_TC_TypeDef *SIMP_TCx);
#endif
#endif
#endif

/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
