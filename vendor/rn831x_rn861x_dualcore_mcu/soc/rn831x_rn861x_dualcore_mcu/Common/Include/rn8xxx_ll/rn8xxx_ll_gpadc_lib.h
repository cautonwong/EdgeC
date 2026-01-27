/******************************************************************************
 * @file     rn8xxx_ll_gpadc_lib.h
 * @brief    GPADC configuration header
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#ifndef RN8XXX_LL_GPADC_LIB_H_
#define RN8XXX_LL_GPADC_LIB_H_

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
    GPADC_LIB_PASS = 0,
    GPADC_LIB_FAIL = 1
} eGpadcLibRet_TypeDef;

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B) ||\
    defined(RN202x_RN7326_SOC_V2)

/**
 * @brief get GPADC Tadj from flash special line
 *
 * @param tadj tadj value from flash special line
 * @return eGpadcLibRet_TypeDef pass of fail
 */
eGpadcLibRet_TypeDef LL_GPADC_TadjGet(unsigned int *tadj);

#endif

#ifdef __cplusplus
}
#endif

#endif
/* r2989 */
