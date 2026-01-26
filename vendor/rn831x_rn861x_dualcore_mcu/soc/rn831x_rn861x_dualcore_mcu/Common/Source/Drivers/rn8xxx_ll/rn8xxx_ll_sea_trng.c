/******************************************************************************
 * @file        rn8xxx_ll_sea_trng.c
 * @brief       TRNG functions
 * @author      Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 *****************************************************************************/
#include "rn8xxx_ll_sea_trng.h"

#if !defined(RN831x_RN861x_MCU_V4)
/**
 * @brief trng_gen
 * @param random the expected random numbers' address
 * @param data_len the expected random numbers' length
 *
 */
void LL_TRNG_gen(uint32_t random[], uint8_t data_len)
{
    uint8_t i;
    uint32_t cnt;
    sea_trng_init();
    *(uint32_t*)(0x5000C3C0) = 0xE4u;
    SEA_CYPHER->TRNG_CTL1 = 0x6543210U;
    SEA_CYPHER->TRNG_CTL2 = 0x884u;
    SEA_CYPHER->Status = 0x40U;
    SEA_CYPHER->TRNG_CTL0 = 0x001Fu;

    for (i = 0U; i < data_len; i++) {
      if (0x0U == (i & 0x3U)) {
        cnt = 0U;
        while((SEA_CYPHER->Status & 0x40U)==0U){
          if(cnt++ > 8000U){
            break;
          }
        }
      }
      random[i] = SEA_CYPHER->TRNG_BUF[i & 0x3U];
    }

    SEA_CYPHER->TRNG_CTL0 = 0x0U;
}

#else
void LL_TRNG_gen(uint32_t random[], uint8_t data_len)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_TRNG_gen func_ptr = (pLL_TRNG_gen)((uint32_t)Iap_Func->LL_TRNG_gen + ROM_BASE_ADDR);
    func_ptr(random, data_len);
}

ErrStatus LL_TRNG_keygen(KeyStore_InitTypeDef *kp, uint8_t k_index)
{
    uint32_t key[18] = {0}, keyword = 0U;
    keyword = ((0U != (kp->KeyLen & 0x1FU)) \
              ? ((kp->KeyLen >> 5U) + 1U) \
              : (kp->KeyLen >> 5U));
    LL_TRNG_gen(key, keyword);
    if (KEYSTORE_STA_SUCCESS != LL_KEYSTORE_WriteKey(kp, key, k_index)) {
        return SEA_ERROR;
    } else {
        return SEA_SUCCESS;
    }
}

#endif

/* r2725 */
