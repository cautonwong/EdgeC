/******************************************************************************
 * @file        rn8xxx_ll_sea_hash.h
 * @brief       public interface for HASH
 * @author      Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 *****************************************************************************/
#ifndef HASH_CYPHER_H_
#define HASH_CYPHER_H_

#include "rn8xxx_ll_sea_common.h"

extern ErrStatus LL_HASH_init(const hashConfigStr* configuration);
extern ErrStatus LL_HASH_deinit(void);
extern ErrStatus LL_HASH(const hashData_t* verifyMessage, uint32_t size, CYPHER_DATA_STA is_last, hashData_t *verifyOut);

extern ErrStatus LL_HASH_process_status_latch(hashProcessParameters* latch_buf);
extern ErrStatus LL_HASH_process_status_recover(const hashProcessParameters* recover_buf);
#endif
/* r2731 */
