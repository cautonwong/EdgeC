/******************************************************************************
 * @file     rn8xxx_v2_lib.h
 * @brief    rn8xxx v2 library code
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#ifndef RN8XXX_V2_LIB_H_
#define RN8XXX_V2_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rn8xxx_ll_sysclk.h"
#include "rn8xxx_ll_flash.h"
#include "rn8xxx_ll_eeprom.h"
#include "rn8xxx_ll_rtc_lib.h"
#include "rn8xxx_ll_sysoption.h"
#include "rn8xxx_ll_sipeeprom.h"
#include "rn8xxx_ll_clktrim.h"
#include "rn8xxx_ll_emu_lib.h"
#include "rn8xxx_ll_gpadc_lib.h"

typedef enum {
    IS_LIB_VALID = 0U,
    IS_LIB_INVALID = 1U
} NvrLibValid_TypeDef;

NvrLibValid_TypeDef LL_LIB_Check(void);

#ifdef __cplusplus
}
#endif

#endif
/* r2989 */
