/******************************************************************************
 * @file     rn8xxx_ll_eeprom.h
 * @brief    non-valatile eeprom memory driver header
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#ifndef RN8XXX_LL_EEPROM_H_
#define RN8XXX_LL_EEPROM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "renergy_chip_config.h"
#include "rn8xxx_ll_devices.h"

/**********************************************************
  api return
**********************************************************/
typedef enum
{
    EEPROM_PASS = 0,
    EEPROM_COUNT_ERROR = 6,
    EEPROM_INVALID_PAGE = 8,
    EEPROM_SRC_ADDR_NOT_ALIGN = 10,
    EEPROM_SRC_ADDR_NOT_MAPPED = 11,
    EEPROM_DST_ADDR_NOT_ALIGN = 12,
    EEPROM_DST_ADDR_NOT_MAPPED = 13,
    EEPROM_VOL_INVALID = 20,
    EEPROM_CLK_ERROR = 21
} eEepromRet_TypeDef;

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN831x_RN861x_MCU_V1)
/**
 * @brief eeprom program
 *
 * @param dst_addr eeprom program address
 * @param src_addr program source address
 * @param len program byte length
 * @return eEepromRet_TypeDef interface result
 */
eEepromRet_TypeDef LL_EEPROM_Program(unsigned int dst_addr, unsigned int src_addr, unsigned int len);
#define eepromProgram LL_EEPROM_Program

/**
 * @brief eeprom enter standby
 *
 */
void    LL_EEPROM_Standby(void);
#define eepromStandby LL_EEPROM_Standby

/**
 * @brief eeprom wake up and exit standby
 *
 */
void    LL_EEPROM_Wakeup(void);
#define eepromWakeup LL_EEPROM_Wakeup
#endif

#ifdef __cplusplus
}
#endif

#endif
/* r2989 */
