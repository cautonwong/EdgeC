/******************************************************************************
 * @file        rn8xxx_ll_sea_common.c
 * @brief       SEA engine functions
 * @author      Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 *****************************************************************************/
#include "rn8xxx_ll_sea_common.h"

#if defined(RN831x_RN861x_MCU_V4)
#include "rn831x_rn861x_mcu_system_core0.h"
#include "rn831x_rn861x_mcu_system_core1.h"

#else
static uint32_t   cryptLibError = CYPHER_OK;
SEA_G_PARAS      g_cypher_paras;

void sea_memset_word(uint32_t dst[], uint32_t data, uint32_t len )
{
    unsigned  int i;
    for(i=0U; i< len; i++){
      dst[i] = data;
    }
}

void sea_memset(uint8_t dst[], const uint8_t data, uint32_t len )
{
    unsigned  int i;
    for(i=0U; i< len; i++){
      dst[i] = data;
    }
}

/* memory for hword operate*/
void sea_memcpy_word(uint32_t dst[], const uint32_t  src[], uint32_t len )
{
    unsigned int i;
    for(i=0U; i< len; i++){
      dst[i] = src[i];
    }
}

void sea_memcpy(uint8_t dst[], const uint8_t  src[], uint32_t len )
{
    unsigned int i;
    for(i=0U; i< len; i++){
     dst[i] = src[i];
    }
}
#endif

void sea_init_all(void)
{
    unsigned int i, addr;
    unsigned int sysps_bak;

#if !defined(RN831x_RN861x_MCU_V4)
    sysps_bak = SYSCTRL_PS;
    SYSCTRL_PS = 0x82U;
    SYSCTRL_MOD0_EN |= ((uint32_t)1U<<3U) | ((uint32_t)1U<<21U) | ((uint32_t)1U<<22U) | ((uint32_t)1U<<23U) | ((uint32_t)1U<<24U);
    for(i = 0U; i < 1024U; i++)
    {
          addr = SEA_EMB_BASEADDR + (4U*i) ;
        *((unsigned int *)(addr)) = 0x0U;
    }
    SYSCTRL_PS = sysps_bak;
#else
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD0_CKE |= ((uint32_t)1U<<2U) | ((uint32_t)1U<<3U) | ((uint32_t)1U<<4U) | ((uint32_t)1U<<5U) | ((uint32_t)1U<<6U);
    for(i = 0U; i < 1024U; i++)
    {
#if defined(CORE0)
        addr = (0x30009000U + (4U * i));
#elif defined(CORE1)
        addr = (0xB0009000U + (4U * i));
#endif
        *((unsigned int *)(addr)) = 0x0U;
    }
    SYSTEM->SYS_PSW1 = sysps_bak;
#endif
}
void sea_trng_init(void)
{
    unsigned int sysps_bak;

#if !defined(RN831x_RN861x_MCU_V4)
    sysps_bak = SYSCTRL_PS;
    SYSCTRL_PS = 0x82U;
#ifdef  RN821x_RN721x_SOC_V3
    SYSCTRL_MOD0_EN |=  ((uint32_t)1U<<23U);
#else
    SYSCTRL_MOD0_EN |= ((uint32_t)1U<<3U) | ((uint32_t)1U<<23U);
#endif

    SYSCTRL_PS = sysps_bak;
#else
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD0_CKE |= ((uint32_t)1U<<2U) | ((uint32_t)1U<<5U);
    SYSTEM->SYS_PSW1 = sysps_bak;
#endif
}
void sea_ecc_init(void)
{
    unsigned int sysps_bak;

#if !defined(RN831x_RN861x_MCU_V4)
    sysps_bak = SYSCTRL_PS;
    SYSCTRL_PS = 0x82U;
    SYSCTRL_MOD0_EN |= ((uint32_t)1U<<3U) | ((uint32_t)1U<<21U);
    SYSCTRL_PS = sysps_bak;
#else
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD0_CKE |= ((uint32_t)1U<<2U) | ((uint32_t)1U<<3U);
    SYSTEM->SYS_PSW1 = sysps_bak;
#endif
}
void sea_rsa_init(void)
{
    unsigned int sysps_bak;

#if !defined(RN831x_RN861x_MCU_V4)
    sysps_bak = SYSCTRL_PS;
    SYSCTRL_PS = 0x82U;
    SYSCTRL_MOD0_EN |= ((uint32_t)1U<<3U) | ((uint32_t)1U<<21U);
    SYSCTRL_PS = sysps_bak;
#else
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD0_CKE |= ((uint32_t)1U<<2U) | ((uint32_t)1U<<3U);
    SYSTEM->SYS_PSW1 = sysps_bak;
#endif
}
void sea_hash_init(void)
{
    unsigned int sysps_bak;

#if !defined(RN831x_RN861x_MCU_V4)
    sysps_bak = SYSCTRL_PS;
    SYSCTRL_PS = 0x82U;
    SYSCTRL_MOD0_EN |= ((uint32_t)1U<<3U) |((uint32_t)1U<<24U);
    SYSCTRL_PS = sysps_bak;
#else
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD0_CKE |= ((uint32_t)1U<<2U) | ((uint32_t)1U<<6U);
    SYSTEM->SYS_PSW1 = sysps_bak;
#endif
}
void sea_hash_deinit(void)
{
    unsigned int sysps_bak;

#if !defined(RN831x_RN861x_MCU_V4)
    sysps_bak = SYSCTRL_PS;
    SYSCTRL_PS = 0x82U;
    SYSCTRL_MOD0_EN &= (~(((uint32_t)1U<<3U) |((uint32_t)1U<<24U)));
    SYSCTRL_PS = sysps_bak;
#else
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD0_CKE &= (~(((uint32_t)1U<<2U) | ((uint32_t)1U<<6U)));
    SYSTEM->SYS_PSW1 = sysps_bak;
#endif
}

void sea_aes_init(void)
{
    unsigned int sysps_bak;

#if !defined(RN831x_RN861x_MCU_V4)
    sysps_bak = SYSCTRL_PS;
    SYSCTRL_PS = 0x82U;

#ifdef  RN821x_RN721x_SOC_V3
    SYSCTRL_MOD0_EN |= ((uint32_t)1U<<22U);
#else
    SYSCTRL_MOD0_EN |= ((uint32_t)1U<<3U) | ((uint32_t)1U<<22U);
#endif
    SYSCTRL_PS = sysps_bak;
#else
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD0_CKE |= ((uint32_t)1U<<2U) | ((uint32_t)1U<<4U);
    SYSTEM->SYS_PSW1 = sysps_bak;
#endif
}

void sea_aes_deinit(void)
{
    unsigned int sysps_bak;

#if !defined(RN831x_RN861x_MCU_V4)
    sysps_bak = SYSCTRL_PS;
    SYSCTRL_PS = 0x82U;

#ifdef  RN821x_RN721x_SOC_V3
    SYSCTRL_MOD0_EN &= (~((uint32_t)1U<<3U) |((uint32_t)1U<<22U));
#else
    SYSCTRL_MOD0_EN &= (~(((uint32_t)1U<<3U) |((uint32_t)1U<<22U)));
#endif
    SYSCTRL_PS = sysps_bak;
#else
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD0_CKE &= (~(((uint32_t)1U<<2U) | ((uint32_t)1U<<3U)));
    SYSTEM->SYS_PSW1 = sysps_bak;
#endif
}

#if !defined(RN831x_RN861x_MCU_V4)
void sea_enable(void)
{
    unsigned int sysps_bak;

    sysps_bak = SYSCTRL_PS;
    SYSCTRL_PS = 0x82U;

#ifdef  RN821x_RN721x_SOC_V3
    SYSCTRL_MOD0_EN |= ((uint32_t)1U<<22U) | ((uint32_t)1U<<23U);
#else
    SYSCTRL_MOD0_EN |= ((uint32_t)1U<<3U) | ((uint32_t)1U<<21U) | ((uint32_t)1U<<22U) | ((uint32_t)1U<<23U) | ((uint32_t)1U<<24U);
#endif
    SYSCTRL_PS = sysps_bak;
}

void sea_disable(void)
{
    unsigned int sysps_bak;

    sysps_bak = SYSCTRL_PS;
    SYSCTRL_PS = 0x82U;
#ifdef  RN821x_RN721x_SOC_V3
    SYSCTRL_MOD0_EN &= (~((uint32_t)0x3U<<22U));
#else
    SYSCTRL_MOD0_EN &= (~(((uint32_t)1U<<3U) |((uint32_t)0xfU<<20U)));
#endif
    SYSCTRL_PS = sysps_bak;
}

void endian_rbytes(uint32_t *p_disc, const uint32_t *p_src, uint32_t len_w)
{
    uint32_t i;
    const SEA_REGS_WordMSK  * const tmp_word_src       = (const SEA_REGS_WordMSK  *)(uint32_t)p_src;
    SEA_REGS_WordMSK        * const tmp_word_disc      = (SEA_REGS_WordMSK  *)(uint32_t)p_disc;

    for(i=0U; i<len_w; i++)
    {
        tmp_word_disc->data32[i] =  (uint32_t)(__REV(tmp_word_src->data32[i])) ;
    }
}

void endian_rbytes_size(uint32_t *p_disc, const uint32_t *p_src, uint32_t size)
{
    uint32_t i,j, temp_data_u32;
    SEA_REGS_BYTEMSK        *tmp_datamsk;
    SEA_REGS_hWordMSK       *tmp_hword_datamsk;
    const SEA_REGS_WordMSK  * const tmp_word_src       = (const SEA_REGS_WordMSK  *)(uint32_t)p_src;
    SEA_REGS_WordMSK        * const tmp_word_disc      = (SEA_REGS_WordMSK  *)(uint32_t)p_disc;

    for(i=0U; i<(size>>2U); i++)
    {
        tmp_word_disc->data32[i] =  (uint32_t)(__REV(tmp_word_src->data32[i])) ;
    }
    /*when size is not align by 32bit word*/
    j = size & 3u;
    if(0U != j){
        temp_data_u32 =  (uint32_t)(__REV(tmp_word_src->data32[i])) ;
        tmp_datamsk       =  (SEA_REGS_BYTEMSK  *)(uint32_t)&tmp_word_disc->data32[i];
        tmp_hword_datamsk =  (SEA_REGS_hWordMSK *)(uint32_t)&tmp_word_disc->data32[i];
        if(1U == j){
          tmp_datamsk->data8[0] =  (uint8_t)temp_data_u32  ;
        }else if(2U == j){
          tmp_hword_datamsk->data16[0] = (uint16_t)temp_data_u32  ;
        }else{
          tmp_hword_datamsk->data16[0] = (uint16_t)temp_data_u32  ;
          tmp_datamsk->data8[2]        = (uint8_t)(temp_data_u32 >> 16) ;
        }
    }
}

void setErrorCode(uint32_t code)
{
    uint32_t tmp32;
    tmp32 = (uint32_t)cryptLibError | (uint32_t)code;
    cryptLibError = (uint32_t)tmp32;
}

void clrErrorCode(uint32_t code)
{
    uint32_t tmp32;
    tmp32 = (uint32_t)cryptLibError & (~((uint32_t)code));
    cryptLibError = (uint32_t)(tmp32);
}

uint32_t checkErrors(void)
{
    return((uint32_t)(cryptLibError));
}

#ifndef  SOFT_VER
#define  SOFT_VER 0x00010004U
#endif
unsigned int LL_SEA_version(void)
{
    return SOFT_VER;
}

#ifndef  CODE_VER
#define  CODE_VER 2782U
#endif
unsigned int GetSeaCodeVersion(void)
{
    return CODE_VER;
}

#else
aesProcessParameters  aes_params;
hashProcessParameters hash_params;
#endif
/* r2893 */
