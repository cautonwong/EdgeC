/******************************************************************************
 * @file     rn8xxx_ll_sysclk.h
 * @brief    System clock driver header
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#ifndef RN8XXX_LL_SIPEEPROM_H_
#define RN8XXX_LL_SIPEEPROM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rn8xxx_ll_devices.h"

/**********************************************************
  api return
**********************************************************/
typedef enum {
    I2C_RET_PASS = 0U,
    I2C_RET_FAIL = 1U
} SIPEEPROM_RET_TYPEDEF;

#if defined(RN_CM0_PLATFORM)      || \
    defined(RN202x_RN7326_SOC_V2) || \
    defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3) || \
    defined(RN821x_RN721x_SOC_V2) || \
    defined(RN821x_RN721x_SOC_V3) || \
    defined(RN831x_RN861x_MCU_V4)
void LL_SIPEEPROM_Init(void);
void LL_SIPEEPROM_Rst(void);
SIPEEPROM_RET_TYPEDEF LL_SIPEEPROM_Read(unsigned int addr, unsigned char *data, unsigned len);
SIPEEPROM_RET_TYPEDEF LL_SIPEEPROM_Write(unsigned int addr, const unsigned char *data, unsigned int len);
#endif

#ifdef __cplusplus
}
#endif

#endif
/* r2989 */
