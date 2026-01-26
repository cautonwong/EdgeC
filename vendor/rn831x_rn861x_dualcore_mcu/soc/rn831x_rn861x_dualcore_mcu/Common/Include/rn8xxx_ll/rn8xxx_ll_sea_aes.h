/******************************************************************************
 * @file        rn8xxx_ll_sea_aes.h
 * @brief       public interface for AES
 * @author      Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 *****************************************************************************/
#ifndef AES_CYPHER_H_
#define AES_CYPHER_H_

#include "rn8xxx_ll_sea_common.h"

extern ErrStatus LL_AES_gf128_mult(const uint8_t *pDataX, const uint8_t *pDataY,  uint8_t *pDataZ);
extern ErrStatus LL_AES_crypt_init(const aesCypherConfigStr*  configuration);
extern ErrStatus LL_AES_crypt_deinit(void);
extern ErrStatus LL_AES_crypt_sign(const plainData_t* plainData, uint32_t size, CYPHER_DATA_STA is_last, cryptData_t* cryptedData);
extern ErrStatus LL_AES_decrypt_verify(const cryptData_t* cryptedData, uint32_t size, CYPHER_DATA_STA is_last,  plainData_t* plainData);

extern ErrStatus LL_AES_gmac(const ghashData_t* ghashData, uint32_t size, CYPHER_DATA_STA is_last);

extern ErrStatus LL_AES_process_status_latch(aesProcessParameters* latch_buf);
extern ErrStatus LL_AES_process_status_recover(const aesProcessParameters* recover_buf);

extern ErrStatus LL_AES_key_wrap(const plainData_t *plainData, uint32_t in_len, cryptData_t *cryptedData, uint32_t *out_len);
extern ErrStatus LL_AES_key_unwrap(const cryptData_t *cryptedData, uint32_t in_len, plainData_t *plainData, uint32_t *out_len);
extern ErrStatus LL_AES_update_aad(const add_t *aadData, uint32_t size);
#endif
/* r2725 */
