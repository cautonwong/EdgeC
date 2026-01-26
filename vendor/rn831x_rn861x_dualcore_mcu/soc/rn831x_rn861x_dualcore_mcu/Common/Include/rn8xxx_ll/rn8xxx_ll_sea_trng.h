/******************************************************************************
 * @file        rn8xxx_ll_sea_trng.h
 * @brief       public interface for TRNG
 * @author      Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 *****************************************************************************/
#ifndef TRNG_DRV_H_
#define TRNG_DRV_H_

#include "rn8xxx_ll_sea_common.h"

extern void LL_TRNG_gen(uint32_t random[], uint8_t data_len);

#if defined(RN831x_RN861x_MCU_V4)
#include "rn8xxx_ll_keystore.h"

ErrStatus LL_TRNG_keygen(KeyStore_InitTypeDef *kp, uint8_t k_index);
#endif
#endif
/* r2725 */
