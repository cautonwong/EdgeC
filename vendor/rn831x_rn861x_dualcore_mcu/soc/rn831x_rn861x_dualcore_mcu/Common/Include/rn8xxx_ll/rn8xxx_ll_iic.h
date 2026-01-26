/********************************************************************************
 * @file    RN8xxx_iic.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-09-07
 * @brief   Header file containing functions prototypes of IIC HAL library.
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
#ifndef Rn8xxx_IIC_H
#define Rn8xxx_IIC_H

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll_def.h"

/* Exported define ------------------------------------------------------------*/
#define IIC_TIMEOUT 0xfffU

/*********************************************************************************/
#define GET_REG_I2C_CTL(i2c, databits) ((i2c)->CTRL & GET_DATA_MASK(databits))
#define GET_REG_I2C_CLK(i2c, databits) ((i2c)->CLK & GET_DATA_MASK(databits))
#define GET_REG_I2C_STAT(i2c, databits) ((i2c)->STA & GET_DATA_MASK(databits))
#define GET_REG_I2C_ADDR(i2c, databits) ((i2c)->ADDR & GET_DATA_MASK(databits))
#define GET_REG_I2C_DATA(i2c, databits) ((i2c)->DATA & GET_DATA_MASK(databits))

#define SET_REG_I2C_CTL(i2c, data)  ((i2c)->CTRL = (data))
#define SET_REG_I2C_CLK(i2c, data)  ((i2c)->CLK = (data))
#define SET_REG_I2C_STAT(i2c, data) ((i2c)->STA = (data))
#define SET_REG_I2C_ADDR(i2c, data) ((i2c)->ADDR = (data))
#define SET_REG_I2C_DATA(i2c, data) ((i2c)->DATA = (data))

/*********************************************************************************/

#define GET_BIT_I2C_CTL(i2c, bit) (((i2c)->CTRL & (bit)) ? (1) : (0))
#define GET_BIT_I2C_CLK(i2c, bit) (((i2c)->CLK & (bit)) ? (1) : (0))
#define GET_BIT_I2C_STAT(i2c, bit) (((i2c)->STA & (bit)) ? (1) : (0))
#define GET_BIT_I2C_ADDR(i2c, bit) (((i2c)->ADDR & (bit)) ? (1) : (0))
#define GET_BIT_I2C_DATA(i2c, bit) (((i2c)->DATA & (bit)) ? (1) : (0))

#define SET_BIT_I2C_CTL(i2c, bit) ((i2c)->CTRL |= (bit))
#define SET_BIT_I2C_CLK(i2c, bit) ((i2c)->CLK |= (bit))
#define SET_BIT_I2C_STAT(i2c, bit) ((i2c)->STA = (bit))
#define SET_BIT_I2C_ADDR(i2c, bit) ((i2c)->ADDR |= (bit))
#define SET_BIT_I2C_DATA(i2c, bit) ((i2c)->DATA |= (bit))

#define CLR_BIT_I2C_CTL(i2c, bit) ((i2c)->CTRL &= (~(bit)))
#define CLR_BIT_I2C_CLK(i2c, bit) ((i2c)->CLK &= (~(bit)))
#define CLR_BIT_I2C_STAT(i2c, bit) (SET_BIT_I2C_STAT((i2c), (bit)))
#define CLR_BIT_I2C_ADDR(i2c, bit) ((i2c)->ADDR &= (~(bit)))
/* Exported enum ------------------------------------------------------------*/

/*只支持以下速率可选，有误差*/
typedef enum
{
    I2C_100K  = 3,
    I2C_400K  = 1,
    
} eIICCLK_TypeDef;

typedef enum
{
    ACK_Disable = 0,
    ACK_Enable = 1,
} eACK_Enable_TypeDef;
/* Exported data ----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

extern ErrorStatus LL_IIC_HardMode_Init(I2C_TypeDef *I2Cx,eIICCLK_TypeDef clkdiv);
extern ErrorStatus LL_IIC_HardMode_Start(I2C_TypeDef *I2Cx, uint8_t iic_id);
extern void LL_IIC_HardMode_Stop(I2C_TypeDef *I2Cx);
extern void IIC_HardMode_Reset(I2C_TypeDef *I2Cx);
extern ErrorStatus LL_IIC_HardMode_SendByte(I2C_TypeDef *I2Cx,uint8_t data);
extern uint8_t LL_IIC_HardMode_ReceiveByte(I2C_TypeDef *I2Cx,eACK_Enable_TypeDef ACK);
extern void LL_IIC_HardMode_Disable(I2C_TypeDef *I2Cx);
#endif 

/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
