/******************************************************************************
 * @file        rn8xxx_ll_sea_ecc.h
 * @brief       public interface for ECC
 * @author      Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 *****************************************************************************/
#ifndef ECC_CYPHER_H_
#define ECC_CYPHER_H_

#include "rn8xxx_ll_sea_common.h"

extern const uECC_Curve_t Curve_NIST_P192     ;
extern const uECC_Curve_t Curve_NIST_P224     ;
extern const uECC_Curve_t Curve_NIST_P256     ;
extern const uECC_Curve_t Curve_NIST_P384     ;
extern const uECC_Curve_t Curve_NIST_P521     ;
extern const uECC_Curve_t Curve_Brainpool_256 ;
extern const uECC_Curve_t Curve_Brainpool_384 ;
extern const uECC_Curve_t Curve_Brainpool_512 ;

extern ErrStatus LL_ECC_keygen(const uECC_Curve_t *curve, eccData_t *_Q, eccData_t *_pkey);
extern ErrStatus LL_ECC_public_keygen(const uECC_Curve_t *curve, eccData_t *_Q, const eccData_t *_pkey);
extern ErrStatus LL_ECC_public_keygen_uncompressed(const uECC_Curve_t *curve, eccData_t *_Q, const eccData_t *_pkey);
extern ErrStatus LL_ECC_keyver(const uECC_Curve_t *curve, const eccData_t *_Q, PassFlag *ver_flag);
extern ErrStatus LL_ECC_DHkeygen(const uECC_Curve_t *curve, const eccData_t *_Q, const eccData_t *_pkey, eccData_t *_DHkey);
extern ErrStatus LL_ECC_DHkeygen_x(const uECC_Curve_t *curve, const eccData_t *_Q, const eccData_t *_pkey, eccData_t *_DHkey);

extern ErrStatus LL_ECC_sign_gen(const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, eccData_t *_Q, eccData_t *_pkey, eccData_t *_r, eccData_t *_s);
extern ErrStatus LL_ECC_specify_pkey_sign_gen(const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, eccData_t *_Q, eccData_t *_pkey, eccData_t *_r, eccData_t *_s);
extern ErrStatus LL_ECC_sign_ver(const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, const eccData_t *_Q, const eccData_t *_r, const eccData_t *_s, PassFlag *ver_flag);

#if defined(RN831x_RN861x_MCU_V4)
extern ErrStatus LL_ECC_keygen_sec(const uECC_Curve_t *curve, uint8_t pkeyIndex, uint8_t QxIndex, uint8_t QyIndex);
extern ErrStatus LL_ECC_sign_gen_sec(const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, const uint8_t pkeyIndex, eccData_t *_r, eccData_t *_s);
extern ErrStatus LL_ECC_sign_ver_sec(const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, const uint8_t QxIndex, const uint8_t QyIndex, const eccData_t *_r, const eccData_t *_s, PassFlag *ver_flag);
#endif
#endif
/* r2894 */
