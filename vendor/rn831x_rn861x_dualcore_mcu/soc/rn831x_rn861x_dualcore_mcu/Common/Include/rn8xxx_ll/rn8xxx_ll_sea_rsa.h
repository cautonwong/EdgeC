/******************************************************************************
 * @file        rn8xxx_ll_sea_rsa.h
 * @brief       public interface for RSA
 * @author      Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 *****************************************************************************/
#ifndef RSA_CYPHER_H_
#define RSA_CYPHER_H_

#include "rn8xxx_ll_sea_common.h"

extern ErrStatus LL_RSA_keygen(uRSA_t *rsa);
extern ErrStatus LL_RSA_sign_gen(uRSA_t *rsa, const uint32_t hashData[], uint32_t hashtype, uint32_t sign[]);
extern ErrStatus LL_RSA_sign_ver(uRSA_t *rsa, const uint32_t hashData[], uint32_t hashtype, uint32_t sign[]);
extern ErrStatus LL_RSA_pub_encrypt(uRSA_t *rsa, const uint32_t plain[], uint32_t plainlen, uint32_t cipher[]);
extern ErrStatus LL_RSA_pub_decrypt(uRSA_t *rsa, const uint32_t cipher[], uint32_t cipherlen, uint32_t plain[]);
extern ErrStatus LL_RSA_pvt_encrypt(uRSA_t *rsa, const uint32_t plain[], uint32_t plainlen, uint32_t cipher[]);
extern ErrStatus LL_RSA_pvt_decrypt(uRSA_t *rsa, const uint32_t cipher[], uint32_t cipherlen, uint32_t plain[]);
#endif
/* r2725 */
