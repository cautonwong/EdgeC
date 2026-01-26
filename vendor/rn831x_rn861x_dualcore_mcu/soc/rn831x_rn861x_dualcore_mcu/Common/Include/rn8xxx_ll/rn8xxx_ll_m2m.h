/******************************************************************************
 * @file    rn8xxx_m2m.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-09-07
 * @brief   Header file containing functions prototypes of M2M HAL library.
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

#ifndef Rn8xxx_M2M_H_
#define Rn8xxx_M2M_H_

/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll_def.h"
/* Exported define ------------------------------------------------------------*/
#define M2M_TIMEOUT 0x3ff

/*********************************************************************************/
#define GET_REG_M2M_MODE(m2m, databits) ((m2m)->MODE & GET_DATA_MASK(databits))
#define GET_REG_M2M_CTL(m2m, databits) ((m2m)->CTL & GET_DATA_MASK(databits))
#define GET_REG_M2M_DUMMY(m2m, databits) ((m2m)->DUMMY & GET_DATA_MASK(databits))
#define GET_REG_M2M_SADDR(m2m, databits) ((m2m)->SADDR & GET_DATA_MASK(databits))
#define GET_REG_M2M_DADDR(m2m, databits) ((m2m)->DADDR & GET_DATA_MASK(databits))
#define GET_REG_M2M_ILEN(m2m, databits) ((m2m)->ILEN & GET_DATA_MASK(databits))
#define GET_REG_M2M_IE(m2m, databits) ((m2m)->IE & GET_DATA_MASK(databits))
#define GET_REG_M2M_IF(m2m, databits) ((m2m)->IF & GET_DATA_MASK(databits))

#define SET_REG_M2M_MODE(m2m, data) ((m2m)->MODE = (data))
#define SET_REG_M2M_CTL(m2m, data) ((m2m)->CTL = (data))
#define SET_REG_M2M_DUMMY(m2m, data) ((m2m)->DUMMY = (data))
#define SET_REG_M2M_SADDR(m2m, data) ((m2m)->SADDR = (data))
#define SET_REG_M2M_DADDR(m2m, data) ((m2m)->DADDR = (data))
#define SET_REG_M2M_ILEN(m2m, data) ((m2m)->ILEN = (data))
#define SET_REG_M2M_IE(m2m, data) ((m2m)->IE = (data))
#define SET_REG_M2M_IF(m2m, data) ((m2m)->IF = (data))

/*********************************************************************************/

#define GET_BIT_M2M_MODE(m2m, bit) (((m2m)->MODE & (bit)) ? (1) : (0))
#define GET_BIT_M2M_CTL(m2m, bit) (((m2m)->CTL & (bit)) ? (1) : (0))
#define GET_BIT_M2M_DUMMY(m2m, bit) (((m2m)->DUMMY & (bit)) ? (1) : (0))
#define GET_BIT_M2M_SADDR(m2m, bit) (((m2m)->SADDR & (bit)) ? (1) : (0))
#define GET_BIT_M2M_DADDR(m2m, bit) (((m2m)->DADDR & (bit)) ? (1) : (0))
#define GET_BIT_M2M_ILEN(m2m, bit) (((m2m)->ILEN & (bit)) ? (1) : (0))
#define GET_BIT_M2M_IE(m2m, bit) (((m2m)->IE & (bit)) ? (1) : (0))
#define GET_BIT_M2M_IF(m2m, bit) (((m2m)->IF & (bit)) ? (1) : (0))

#define SET_BIT_M2M_MODE(m2m, bit) ((m2m)->MODE |= (bit))
#define SET_BIT_M2M_CTL(m2m, bit) ((m2m)->CTL |= (bit))
#define SET_BIT_M2M_DUMMY(m2m, bit) ((m2m)->DUMMY |= (bit))
#define SET_BIT_M2M_SADDR(m2m, bit) ((m2m)->SADDR |= (bit))
#define SET_BIT_M2M_DADDR(m2m, bit) ((m2m)->DADDR |= (bit))
#define SET_BIT_M2M_ILEN(m2m, bit) ((m2m)->ILEN |= (bit))
#define SET_BIT_M2M_IE(m2m, bit) ((m2m)->IE |= (bit))
#define SET_BIT_M2M_IF(m2m, bit) ((m2m)->IF = (bit))

#define CLR_BIT_M2M_MODE(m2m, bit) ((m2m)->MODE &= (~(bit)))
#define CLR_BIT_M2M_CTL(m2m, bit) ((m2m)->CTL &= (~(bit)))
#define CLR_BIT_M2M_DUMMY(m2m, bit) ((m2m)->DUMMY &= (~(bit)))
#define CLR_BIT_M2M_ILEN(m2m, bit) ((m2m)->ILEN &= (~(bit)))
#define CLR_BIT_M2M_IE(m2m, bit) ((m2m)->IE &= (~(bit)))
#define CLR_BIT_M2M_IF(m2m, bit) (SET_BIT_M2M_IF((m2m), (bit)))

typedef enum
{
    IVLD_1BYTE = 0x01, /*"输入1字节有效"*/
    IVLD_2BYTE = 0x03, /*"输入2字节有效"*/
    IVLD_3BYTE = 0x07, /*"输入3字节有效"*/
    IVLD_4BYTE = 0x0F, /*"输入4字节有效"*/

    OVLD_1BYTE = 0x01, /*"输出1字节有效"*/
    OVLD_2BYTE = 0x03, /*"输出2字节有效"*/
    OVLD_3BYTE = 0x07, /*"输出3字节有效"*/
    OVLD_4BYTE = 0x0F, /*"输出4字节有效"*/

    ORV_EN = 1,  /*"反序搬运"*/
    ORV_DIS = 0, /*"正序搬运"*/

    DUMMY_EN = 1,  /*"目标地址中无效byte位置，写入dummy值"*/
    DUMMY_DIS = 0, /*"目标地址中无效byte位置，不进行写操作"*/

    ADDR_RV_EN = 1,  /*"数据按地址反序排列"*/
    ADDR_RV_DIS = 0, /*"数据按地址正序排列"*/

} eM2MMode_TypeDef;

typedef union
{
    struct
    {
        uint32_t IVLD : 4;
        uint32_t OVLD : 4;
        uint32_t ORV_EN : 1;
        uint32_t DUMMY_EN : 1;
        uint32_t ADDR_RVEN : 1;
        uint32_t RESERVEBIT : 21;
    }bitM2MMode;
    uint32_t M2MMode;
} uM2MMode_TypeDef;

typedef struct
{
  uint32_t SAddr;        /*"源地址, 注意：MCU需word对齐"*/
  uint32_t DAddr;        /*"目标地址，注意：MCU需word对齐*/
  uint32_t TxLen;        /*"传送长度*/
  uint32_t DUMMY;        /*dummy值, DUMMY_EN使能才生效，默认为0*/
  uM2MMode_TypeDef Ctrl;
} sll_M2M_InitTypeDef;

#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V4)
extern ErrorStatus LL_M2M_Move(const sll_M2M_InitTypeDef *sMem_Init);
#endif
#endif
/****************************  (C) COPYRIGHT Renergy  ****************************/
/****************************          END OF FILE          ****************************/
