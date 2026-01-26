/*******************************************************************************
 * @file    rn8xxx_ISO7816.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-09-07
 * @brief   Header file containing functions prototypes of ISO7816 HAL library.
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
 *
 ******************************************************************************
 * @ Revised records
 * V1.1.1   2023-09-07  Add       Add macro definition for geting register and register bit
 * V1.1.0               Initial
 */
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef Rn8xxx_ISO7816_H
#define Rn8xxx_ISO7816_H

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll_iso7816.h"
/* Exported define ------------------------------------------------------------*/

#define GET_REG_ISP7816_CTRL0(isp7816x, databits) ((isp7816x)->CTRL0 & GET_DATA_MASK(databits))
#define GET_REG_ISP7816_CTRL1(isp7816x, databits) ((isp7816x)->CTRL1 & GET_DATA_MASK(databits))
#define GET_REG_ISP7816_CLK(isp7816x, databits) ((isp7816x)->CLK & GET_DATA_MASK(databits))
#define GET_REG_ISP7816_BDDIV0(isp7816x, databits) ((isp7816x)->BDDIV0 & GET_DATA_MASK(databits))
#define GET_REG_ISP7816_BDDIV1(isp7816x, databits) ((isp7816x)->BDDIV1 & GET_DATA_MASK(databits))
#define GET_REG_ISP7816_STA0(isp7816x, databits) ((isp7816x)->STA0 & GET_DATA_MASK(databits))
#define GET_REG_ISP7816_STA1(isp7816x, databits) ((isp7816x)->STA1 & GET_DATA_MASK(databits))
#define GET_REG_ISP7816_DATA0(isp7816x, databits) ((isp7816x)->DATA0 & GET_DATA_MASK(databits))
#define GET_REG_ISP7816_DATA1(isp7816x, databits) ((isp7816x)->DATA1 & GET_DATA_MASK(databits))

#define SET_REG_ISO7816_CTRL0(isp7816x, data) ((isp7816x)->CTRL0 = (data))
#define SET_REG_ISO7816_CTRL1(isp7816x, data) ((isp7816x)->CTRL1 = (data))
#define SET_REG_ISO7816_CLK(isp7816x, data) ((isp7816x)->CLK = (data))
#define SET_REG_ISO7816_BDDIV0(isp7816x, data) ((isp7816x)->BDDIV0 = (data))
#define SET_REG_ISO7816_BDDIV1(isp7816x, data) ((isp7816x)->BDDIV1 = (data))
#define SET_REG_ISO7816_STA0(isp7816x, data) ((isp7816x)->STA0 = (data))
#define SET_REG_ISO7816_STA1(isp7816x, data) ((isp7816x)->STA1 = (data))
#define SET_REG_ISO7816_DATA0(isp7816x, data) ((isp7816x)->DATA0 = (data))
#define SET_REG_ISO7816_DATA1(isp7816x, data) ((isp7816x)->DATA1 = (data))
/*********************************************************************************/

#define GET_BIT_ISO7816_CTRL0(isp7816x, bit) (((isp7816x)->CTRL0 & (bit)) ? (1) : (0))
#define GET_BIT_ISO7816_CTRL1(isp7816x, bit) (((isp7816x)->CTRL1 & (bit)) ? (1) : (0))
#define GET_BIT_ISO7816_CLK(isp7816x, bit) (((isp7816x)->CLK & (bit)) ? (1) : (0))
#define GET_BIT_ISO7816_BDDIV0(isp7816x, bit) (((isp7816x)->BDDIV0 & (bit)) ? (1) : (0))
#define GET_BIT_ISO7816_BDDIV1(isp7816x, bit) (((isp7816x)->BDDIV1 & (bit)) ? (1) : (0))
#define GET_BIT_ISO7816_STA0(isp7816x, bit) (((isp7816x)->STA0 & (bit)) ? (1) : (0))
#define GET_BIT_ISO7816_STA1(isp7816x, bit) (((isp7816x)->STA1 & (bit)) ? (1) : (0))
#define GET_BIT_ISO7816_DATA0(isp7816x, bit) (((isp7816x)->DATA0 & (bit)) ? (1) : (0))
#define GET_BIT_ISO7816_DATA1(isp7816x, bit) (((isp7816x)->DATA1 & (bit)) ? (1) : (0))

#define SET_BIT_ISO7816_CTRL0(isp7816x, bit) ((isp7816x)->CTRL0 |= (bit))
#define SET_BIT_ISO7816_CTRL1(isp7816x, bit) ((isp7816x)->CTRL1 |= (bit))
#define SET_BIT_ISO7816_CLK(isp7816x, bit) ((isp7816x)->CLK |= (bit))
#define SET_BIT_ISO7816_BDDIV0(isp7816x, bit) ((isp7816x)->BDDIV0 |= (bit))
#define SET_BIT_ISO7816_BDDIV1(isp7816x, bit) ((isp7816x)->BDDIV1 |= (bit))
#define SET_BIT_ISO7816_STA0(isp7816x, bit) ((isp7816x)->STA0 = (bit))
#define SET_BIT_ISO7816_STA1(isp7816x, bit) ((isp7816x)->STA1 = (bit))
#define SET_BIT_ISO7816_DATA0(isp7816x, bit) ((isp7816x)->DATA0 |= (bit))
#define SET_BIT_ISO7816_DATA1(isp7816x, bit) ((isp7816x)->DATA1 |= (bit))

/* Exported enum ------------------------------------------------------------*/
typedef enum
{
    ISO7816_0ID = 0x00U,
    ISO7816_1ID = 0x01U,
} eISO7816_ID_TypeDef;

typedef enum
{
    ISO7816_Clk_7372800 = 7372800U,
    ISO7816_Clk_3686400 = 3686400U,
    ISO7816_Clk_1843200 = 1843200U,
    ISO7816_Clk_921600 = 921600U,
    ISO7816_Clk_460800 = 460800U,

} eISO7816_Clk_TypeDef;
/* Exported data ----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

extern void LL_ISO7816_Init(eISO7816_ID_TypeDef id);
extern void LL_ISO7816_Disable(eISO7816_ID_TypeDef id);
extern void LL_ISO7816_CLK_Disable(void);
extern ErrorStatus LL_ISO7816_SquareWave_Init(eISO7816_Clk_TypeDef SquareWaveClock);
extern ErrorStatus LL_ISO7816_ReadByte(eISO7816_ID_TypeDef id, uint8_t *Data);
extern ErrorStatus LL_ISO7816_WriteByte(eISO7816_ID_TypeDef id, uint8_t Data);

#endif

/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
