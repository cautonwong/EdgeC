/******************************************************************************
 * @file        rn8xxx_ll_sea_rsa.c
 * @brief       RSA functions
 * @author      Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 *****************************************************************************/

#include "rn8xxx_ll_sea_rsa.h"
#include "rn8xxx_ll_sea_trng.h"

#ifndef  RN821x_RN721x_SOC_V3

#if !defined(RN831x_RN861x_MCU_V4)

#define RSA_KSIZE 32/*only support 1024bits*/
#define SIGN_MODE 0U/*sign type*/
#define DEF_MODE 1U/*normal type*/

static void SEA_RSA_IR_FUNC_RUN(uint8_t IR_FUNC, uint32_t TIMEOUT_CFG, ErrStatus *ERR_RET);
static ErrStatus rsa_calc_J0(const uint32_t J0[]);
static ErrStatus rsa_calc_H(uint16_t num, const uint32_t N[]);
static ErrStatus rsa_mod_inv(const uint32_t N[], uint32_t nbits, const uint32_t X[], uint32_t result[], PassFlag *flag);
static ErrStatus rsa_mod_exp(const uint32_t N[], uint32_t nbits, const uint32_t X[], uint32_t xbits, const uint32_t E[], uint32_t ebits, uint32_t result[]);
static ErrStatus rsa_encrypt(uRSA_t *rsa, const uint32_t plain[], uint32_t plainlen, uint32_t cipher[], uint8_t mode);
static ErrStatus rsa_decrypt(uRSA_t *rsa, const uint32_t cipher[], uint32_t cipherlen, uint32_t plain[], uint8_t mode);
static ErrStatus rsa_key_gen(uRSA_t *rsa);
static ErrStatus rsa_key_ver(uRSA_t *rsa);
extern ErrStatus ecc_cal_H(uint16_t num, const uint32_t P[]);
extern ErrStatus ecc_modinv(const uint32_t P[], const uint32_t U[], uint8_t wnum, uint16_t bnum, uint32_t result[], PassFlag *flag);
static ErrStatus rsa_pkcs1_v1_5_padding(const uint32_t src[], uint32_t srclen, uint32_t tgt[], uint32_t tgtlen);
static ErrStatus BN_isPrime(uint32_t p[], uint8_t words);
static ErrStatus BN_isZero(const uint32_t N[]);
static void BN_Mult(uint32_t X[], uint32_t Y[], uint32_t res[]);
static uint32_t BN_getBitLen(const uint32_t N[]);
static uint32_t BN_getWordLen(const uint32_t N[]);

static void SEA_RSA_IR_FUNC_RUN(uint8_t IR_FUNC, uint32_t TIMEOUT_CFG, ErrStatus *ERR_RET)
{
    uint32_t cnt = 0U;
    SEA_CYPHER->CTRL = (IR_FUNC);
    SEA_CYPHER->Status = 1U;
    SEA_CYPHER->START = 1U;

    while((SEA_CYPHER->Status & 1U) == 0U){
        if(cnt++ > (TIMEOUT_CFG)){
            *(ERR_RET) = ERROR;
            setErrorCode(RSA_TIMEOUT_FAILURE);
            break;
        }
    }
}

/**
 * @brief rsa_calc_J0
 * @param J0: modular value (only low 64 bits)
 */
static ErrStatus rsa_calc_J0(const uint32_t J0[])
{
    ErrStatus tmp_err = SUCCESS;

    SEA_CYPHER->N0Low = J0[0];
    SEA_CYPHER->N0High = J0[1];

    SEA_RSA_IR_FUNC_RUN(SEA_CTRL_IR_PKA_J0_CAL, SEA_TIMEOUT_IR_PKA_J0_CAL, &tmp_err);

    return tmp_err;
}

/**
 * @brief rsa_calc_H
 * @param num: modular length in bits
 * @param N:   modular value
 */
static ErrStatus rsa_calc_H(uint16_t num, const uint32_t N[])
{
    uint32_t temp, i;
    ErrStatus tmp_err = SUCCESS;

    for(i = 0U; i < (num >> 5); i++){
        SEA_RSA_MOD_N->REG[i] = N[i];
        SEA_RSA_H->REG[i] = 0U;
    }
    temp = num & 0x1fU;
    SEA_RSA_H->REG[i] = (uint32_t)1U << (uint32_t)temp;

    if(0U != temp){
        SEA_RSA_MOD_N->REG[i] = N[i];
    }
    SEA_CYPHER->NLen = num;

    SEA_RSA_IR_FUNC_RUN(SEA_CTRL_IR_RSA_H_CAL, SEA_TIMEOUT_IR_RSA_H_CAL, &tmp_err);

    return tmp_err;
}

/**
 * @brief  rsa_mod_inv
 * @param  N:      modular value
 * @param  nbits:  bits length of modular value
 * @param  X:      value
 * @param  xbits:  bits length of value
 * @return result: x^-1 mod N
 */
static ErrStatus rsa_mod_inv(const uint32_t N[], uint32_t nbits, const uint32_t X[], uint32_t result[], PassFlag *flag)
{
    uint32_t i, nwords;
    ErrStatus tmp_err = SUCCESS;
    nwords = nbits >> 5;

    (void)rsa_calc_J0(N);
    (void)ecc_cal_H((uint16_t)nbits, N);

    for(i = 0U; i < nwords; i++){
        SEA_ECC_MAP_CONST->REG[i] = 0U;
        SEA_ECC_MOD_P->REG[i] = 0U;
        SEA_ECC_MOD_PT->REG[i] = 0U;
        SEA_ECC_MAP_U->REG[i] = 0U;
        SEA_ECC_MAP_UT->REG[i] = 0U;
    }

    for(i = 0U; i < nwords; i++){
        SEA_ECC_MOD_P->REG[i] = N[i];
        SEA_ECC_MOD_PT->REG[i] = N[i];
        SEA_ECC_MAP_U->REG[i] = X[i];
        SEA_ECC_MAP_UT->REG[i] = X[i];
    }
    SEA_ECC_MAP_CONST->REG[0] = 1U;

    SEA_RSA_IR_FUNC_RUN(SEA_CTRL_IR_ECC_ModInv, SEA_TIMEOUT_IR_ECC_ModInv, &tmp_err);

    if(0U == (SEA_CYPHER->Status & 2U)){
        *flag = FAIL;
    }
    else{
        *flag = PASS;
    }

    for(i = 0U; i < nwords; i++){
        result[i] = SEA_ECC_MAP_UZ->REG[i];
    }

    return tmp_err;
}

/**
 * @brief  rsa_mod_exp
 * @param  N:      modular value
 * @param  nbits:  bits length of modular value
 * @param  E:      value
 * @param  ebits:  bits length of value
 * @param  X:      value
 * @param  xbits:  bits length of value
 * @return result: X^E mod N
 */
static ErrStatus rsa_mod_exp(const uint32_t N[], uint32_t nbits, const uint32_t X[], uint32_t xbits, const uint32_t E[], uint32_t ebits, uint32_t result[])
{
    uint32_t i, nwords, xwords, ewords;
    ErrStatus tmp_err = SUCCESS;

    (void)rsa_calc_J0(N);
    (void)rsa_calc_H((uint16_t)nbits, N);

    nwords = nbits >> 5;
    xwords = (0U != (xbits & 0x1fU))?((xbits >> 5) + 1U):(xbits >> 5);
    ewords = (0U != (ebits & 0x1fU))?((ebits >> 5) + 1U):(ebits >> 5);

    for(i = 0U; i < nwords; i++){
        SEA_RSA_MAP_E->REG[i] = 0U;
        SEA_RSA_MAP_R0->REG[i] = 0U;
        SEA_RSA_MAP_R0T->REG[i] = 0U;
        SEA_RSA_MAP_R1->REG[i] = 0U;
        SEA_RSA_MAP_R1T->REG[i] = 0U;
    }

    for(i = 0U; i < ewords; i++){
        SEA_RSA_MAP_E->REG[i] = E[i];
    }
    SEA_CYPHER->ELen = ebits;

    /*write X*/
    for(i = 0U; i < xwords; i++){
        SEA_RSA_MAP_R1->REG[i] = X[i];
        SEA_RSA_MAP_R1T->REG[i] = X[i];
    }

    /*clear R0/R0T*/
    for(i = 0U; i < nwords; i++){
        SEA_RSA_MAP_R0->REG[i] = 0U;
        SEA_RSA_MAP_R0T->REG[i] = 0U;
    }
    SEA_RSA_MAP_R0->REG[0] = 1U;
    SEA_RSA_MAP_R0T->REG[0] = 1U;

    SEA_RSA_IR_FUNC_RUN(SEA_CTRL_IR_RSA_ModExp, SEA_TIMEOUT_IR_RSA_ModExp, &tmp_err);

    for(i = 0U; i < nwords; i++){
        result[i] = SEA_RSA_MAP_R0->REG[i];
    }

    return tmp_err;
}

/**
 * @brief BN_isPrime
 * @param p:       value
 * @param words:   word length of p
 * @return result: is prime: SUCCESS, not prime: ERROR
 */
static ErrStatus BN_isPrime(uint32_t p[], uint8_t words)
{
    uint8_t i;
    uint32_t p_len, a_len, a[RSA_KSIZE], d[RSA_KSIZE], result[RSA_KSIZE];
    ErrStatus tmp_err = SUCCESS;

    for(i = 0U; i < RSA_KSIZE; i++){
        a[i] = 0U;
        result[i] = 0U;
        if(i<words){
            d[i] = (i > 0U) ? p[i] : (p[i] - 1U); /*d=p-1*/
            continue;
        }
        d[i] = 0U;
    }
    LL_TRNG_gen(a, 7);
    a_len = BN_getBitLen(a);
    p_len = BN_getBitLen(p);

    if(SUCCESS != rsa_mod_exp(p, p_len, a, a_len, d, p_len, result)){
        tmp_err = ERROR;/*a^(p-1)(mod p)*/
    }

    if(1U != result[0]){
        tmp_err = ERROR;
    }

    return tmp_err;
}

/**
 * @brief rsa_pkcs1_v1_5_padding
 * @param src:     plaintext before padding
 * @param srclen:  bit length of source
 * @param tgt:     plaintext after padding
 * @param tgtlen:  bit length of target
 */
static ErrStatus rsa_pkcs1_v1_5_padding(const uint32_t src[], uint32_t srclen, uint32_t tgt[], uint32_t tgtlen)
{
    ErrStatus tmp_err = SUCCESS;
    uint32_t i, j = 0U, swords, twords, lshift, rand[RSA_KSIZE];

    if((srclen + 88U) > tgtlen){
        tmp_err = ERROR;/*padding need more than 11 bytes*/
        setErrorCode(RSA_PARASLEN_FAILURE);
    }

    lshift = srclen & 0x1fU;
    swords = (0U != lshift)?((srclen >> 5) + 1U):(srclen >> 5);
    twords = tgtlen >> 5;

    if((SUCCESS == BN_isZero(src)) || (srclen != BN_getBitLen(src))){
        tmp_err = ERROR;
        setErrorCode(RSA_PADDING_FAILURE);
    }

    if(CYPHER_OK == checkErrors()){
        LL_TRNG_gen(rand, RSA_KSIZE);
        if(0U == lshift){
            for(i = 0U; i < twords; i++){
                if(i < swords){
                    tgt[i] = src[i];
                }
                else{
                    tgt[i] = (i != swords)?rand[j++]:(rand[j] & 0xFFFFFF00U);/*padding end flag is 0x00*/
                }
            }
        }
        else{
            for(i = 0U; i < twords; i++){
                if(i < (swords - 1U)){
                    tgt[i] = src[i];
                }
                else if(i > (swords - 1U)){
                    tgt[i] = rand[j++];
                }
                else{
                    tgt[i] = rand[j] & (0xFFFFFFFFU << (lshift + 8U));
                    tgt[i] |= src[i];
                }
            }
        }
        tgt[i - 1U] = 0x20000U | (tgt[i - 1U] >> 16U);/*padding start flag is 0x0002*/
    }

    return tmp_err;
}

/**
 * @brief rsa_encrypt
 * @param plain:   value
 * @param plainlen:  bit length of plain
 * @param cipher:  value
 */
static ErrStatus rsa_encrypt(uRSA_t *rsa, const uint32_t plain[], uint32_t plainlen, uint32_t cipher[], uint8_t mode)
{
    uint32_t i, tmp[RSA_KSIZE];
    ErrStatus tmp_err = SUCCESS;

    for(i = 0U; i < RSA_KSIZE; i++){
        tmp[i] = 0U;
    }
    clrErrorCode(0xff);

    if(plainlen != BN_getBitLen(plain)){
        tmp_err = ERROR;
        setErrorCode(RSA_PARASLEN_FAILURE);
    }

    if(CYPHER_OK == checkErrors()){
        if(SUCCESS == rsa_pkcs1_v1_5_padding(plain, plainlen, tmp, rsa->rsa_keylen)){
            if(0U == mode){
                (void)rsa_mod_exp(rsa->n, rsa->n_len, tmp, BN_getBitLen(tmp), rsa->d, rsa->d_len, cipher);/*M^d(mod N)*/
            }
            else{
                (void)rsa_mod_exp(rsa->n, rsa->n_len, tmp, BN_getBitLen(tmp), rsa->e, rsa->e_len, cipher);/*M^e(mod N)*/
            }
        }
        else{
            tmp_err = ERROR;
        }
    }
    else{
        tmp_err = ERROR;
    }

    return tmp_err;
}

/**
 * @brief rsa_decrypt
 * @param cipher: value
 * @param cipherlen: bit length of cipher
 * @param plain:  value
 */
static ErrStatus rsa_decrypt(uRSA_t *rsa, const uint32_t cipher[], uint32_t cipherlen, uint32_t plain[], uint8_t mode)
{
    ErrStatus tmp_err = SUCCESS;

    clrErrorCode(0xff);

    if(cipherlen != BN_getBitLen(cipher)){
        tmp_err = ERROR;
        setErrorCode(RSA_PARASLEN_FAILURE);
    }

    if(CYPHER_OK == checkErrors()){
        if(0U == mode){
            (void)rsa_mod_exp(rsa->n, rsa->n_len, cipher, cipherlen, rsa->e, rsa->e_len, plain); /*C^e(mod N)*/
        }
        else{
            (void)rsa_mod_exp(rsa->n, rsa->n_len, cipher, cipherlen, rsa->d, rsa->d_len, plain); /*C^d(mod N)*/
        }
    }
    else{
        tmp_err = ERROR;
    }

    return tmp_err;
}

/**
 * @brief BN_Mult
 * @param X:       value
 * @param X_len:   word length of X
 * @param Y:       value
 * @param Y_len:   word length of Y
 * @param res:     value
 * @return result: res = X*Y
 */
static void BN_Mult(uint32_t X[], uint32_t Y[], uint32_t res[])
{
    uint64_t mul, carry;
    uint32_t xwords, ywords, i, j;

    xwords = BN_getWordLen(X);
    ywords = BN_getWordLen(Y);

    if((0U != xwords) && (0U != ywords)){
        for(i = 0U; i < xwords; i++){
            carry = 0U;
            for(j = 0U; j < ywords; j++){
                mul = ((uint64_t)X[i] * Y[j]) + res[i + j] + carry;
                res[i + j] = (uint32_t)mul;
                carry = ((uint32_t *)(uint32_t)&mul)[1];
            }
            res[i + ywords] = (uint32_t)carry;
        }
    }
}

/**
 * @brief BN_getBitLen
 * @param N:       value
 * @return result: bit length of N
 */
static uint32_t BN_getBitLen(const uint32_t N[])
{
    uint32_t pos = 0U, nwords = 0U, tmp = 0U;
    int i=0;

    nwords = BN_getWordLen(N);
    if(0U != nwords){
        for(i = (int)(nwords - 1U); i >= 0; i--){
            if(0U != N[i]){
                tmp = N[i];
                break;
            }
        }

        while(0U != tmp){
            tmp >>= 1;
            pos++;
        }
    }

    return (uint32_t)(i * 32) + pos;
}

/**
 * @brief BN_getWordLen
 * @param N:       value
 * @return result: word length of N
 */
static uint32_t BN_getWordLen(const uint32_t N[])
{
    uint32_t i = 0;

    while(0U!=N[i]){
        if(RSA_KSIZE==i){
            break;
        }
        i++;
    }

    return i;
}

/**
 * @brief BN_isZero
 * @param N:       value
 * @param words:   words length of N
 * @return result: N==0:True, n!=0:false
 */
static ErrStatus BN_isZero(const uint32_t N[])
{
    uint32_t i, nwords;
    ErrStatus tmp_err = SUCCESS;

    nwords = BN_getWordLen(N);
    for(i = 0U; i < nwords; i++){
        if(N[i] != 0){
            tmp_err = ERROR;
            break;
        }
    }

    return tmp_err;
}

/**
 * @brief rsa_key_gen
 */
static ErrStatus rsa_key_gen(uRSA_t *rsa)
{
    int times=10, flag=0;
    uint8_t i, keysize;
    uint32_t random[4], P[RSA_KSIZE], Q[RSA_KSIZE], euler_n[RSA_KSIZE], euler_len;
    PassFlag tmp_flag = PASS;
    ErrStatus tmp_err = SUCCESS;

    clrErrorCode(0xff);

    if((0U != (rsa->rsa_keylen & 0x1fU)) && (512U != rsa->rsa_keylen)){
        tmp_err = ERROR;
        setErrorCode(RSA_PARASLEN_FAILURE);
    }

    for(i=0U;i<RSA_KSIZE;i++){
        P[i]=0U;
        Q[i]=0U;
    }

    keysize = (uint8_t)(rsa->rsa_keylen >> 6);
    LL_TRNG_gen(random, 4U);
    LL_TRNG_gen(P, keysize);
    P[keysize - 1] |= (0xC0000000U | random[0]);
    P[0] |= 0x1U;

    LL_TRNG_gen(Q, keysize);
    Q[keysize - 1] |= (0xC0000000U | random[1]);
    Q[0] |= 0x1U;

    while(0 < times){
        if(SUCCESS != BN_isPrime(P, keysize)){
            P[0] += 2U;
        }
        else{
            flag++;
        }

        if(SUCCESS != BN_isPrime(Q, keysize)){
            Q[0] += 2U;
        }
        else{
            flag++;
        }
        times--;
    }

    if(2 > flag){
        tmp_err = ERROR;
        setErrorCode(RSA_KEY_FAILURE);
    }

    if(CYPHER_OK == checkErrors()){
        BN_Mult(P, Q, rsa->n); /*N=P*Q*/
        rsa->e[0] = 0x10001U; /*E=65537*/

        P[0] -= 1U;
        Q[0] -= 1U;
        for(i = 0U; i < RSA_KSIZE; i++){
            euler_n[i] = 0U;
        }
        BN_Mult(P, Q, euler_n); /*euler(N)=(P-1)(Q-1)*/
        euler_len = BN_getBitLen(euler_n);
        if(SUCCESS != rsa_mod_inv(euler_n, euler_len, rsa->e, rsa->d, &tmp_flag)){
            tmp_err = ERROR; /*d=e^-1(mod euler(N))*/
        }
        if(PASS != tmp_flag){
            tmp_err = ERROR;
        }
        rsa->n_len = BN_getBitLen(rsa->n);
        rsa->e_len = BN_getBitLen(rsa->e);
        rsa->d_len = BN_getBitLen(rsa->d);
    }
    else{
        tmp_err = ERROR;
    }

    return tmp_err;
}

ErrStatus LL_RSA_keygen(uRSA_t *rsa)
{
    uint8_t i;
    ErrStatus tmp_err = SUCCESS;
    sea_init_all();

    for (i = 0U; i < RSA_KSIZE; i++) {
        rsa->n[i] = 0U;
        rsa->e[i] = 0U;
        rsa->d[i] = 0U;
    }

    if(512U == rsa->rsa_keylen){
        if(SUCCESS != rsa_key_gen(rsa)){
            tmp_err = ERROR;
        }

        if(SUCCESS != rsa_key_ver(rsa)){
            tmp_err = ERROR;
        }
    }
    else{
        tmp_err = ERROR;
    }

    return tmp_err;
}

static ErrStatus rsa_key_ver(uRSA_t *rsa)
{
    ErrStatus tmp_err = SUCCESS;

    rsa->n_len = BN_getBitLen(rsa->n);
    rsa->e_len = BN_getBitLen(rsa->e);
    rsa->d_len = BN_getBitLen(rsa->d);

    if(0U != (rsa->n_len & 0x1fU)){
        tmp_err = ERROR;
    }

    if((512 != rsa->n_len) && (1024 != rsa->n_len)){
        tmp_err = ERROR;
    }

    if(rsa->n_len < rsa->d_len){
        tmp_err = ERROR;
    }

    return tmp_err;
}

ErrStatus LL_RSA_pub_encrypt(uRSA_t *rsa, const uint32_t plain[], uint32_t plainlen, uint32_t cipher[])
{
    ErrStatus tmp_err = SUCCESS;
    sea_init_all();

    if(SUCCESS == rsa_key_ver(rsa)){
        if((1U <= BN_getBitLen(plain))&&((rsa->n_len-88U) >= BN_getBitLen(plain))){
            if(SUCCESS != rsa_encrypt(rsa, plain, plainlen, cipher, DEF_MODE)){
                tmp_err = ERROR;
            }
        }
        else{
            tmp_err = ERROR;
        }
    }
    else{
        tmp_err = ERROR;
    }

    return tmp_err;
}

ErrStatus LL_RSA_pub_decrypt(uRSA_t *rsa, const uint32_t cipher[], uint32_t cipherlen, uint32_t plain[])
{
    ErrStatus tmp_err = SUCCESS;
    sea_init_all();

    if(SUCCESS == rsa_key_ver(rsa)){
        if((1U <= BN_getBitLen(cipher))&&(rsa->n_len >= BN_getBitLen(cipher))){
            if(SUCCESS != rsa_decrypt(rsa, cipher, cipherlen, plain, SIGN_MODE)){
                tmp_err = ERROR;
            }
        }
        else{
            tmp_err = ERROR;
        }
    }
    else{
        tmp_err = ERROR;
    }

    return tmp_err;
}

ErrStatus LL_RSA_pvt_encrypt(uRSA_t *rsa, const uint32_t plain[], uint32_t plainlen, uint32_t cipher[])
{
    ErrStatus tmp_err = SUCCESS;
    sea_init_all();

    if(SUCCESS == rsa_key_ver(rsa)){
        if((0U != BN_getBitLen(plain)) && ((rsa->n_len-88U) >= BN_getBitLen(plain))){
            if(SUCCESS != rsa_encrypt(rsa, plain, plainlen, cipher, SIGN_MODE)){
                tmp_err = ERROR;
            }
        }
        else{
            tmp_err = ERROR;
        }
    }
    else{
        tmp_err = ERROR;
    }

    return tmp_err;
}

ErrStatus LL_RSA_pvt_decrypt(uRSA_t *rsa, const uint32_t cipher[], uint32_t cipherlen, uint32_t plain[])
{
    ErrStatus tmp_err = SUCCESS;
    sea_init_all();

    if(SUCCESS == rsa_key_ver(rsa)){
        if((1U <= BN_getBitLen(cipher))&&(rsa->n_len >= BN_getBitLen(cipher))){
            if(SUCCESS != rsa_decrypt(rsa, cipher, cipherlen, plain, DEF_MODE)){
                tmp_err = ERROR;
            }
        }
        else{
            tmp_err = ERROR;
        }
    }
    else{
        tmp_err = ERROR;
    }

    return tmp_err;
}

ErrStatus LL_RSA_sign_gen(uRSA_t *rsa, const uint32_t hashData[], uint32_t hashtype, uint32_t sign[])
{
    ErrStatus tmp_err = SUCCESS;
    sea_init_all();

    if((SUCCESS == rsa_key_ver(rsa)) && (hashtype == BN_getBitLen(hashData))){
        if(SUCCESS != rsa_encrypt(rsa, hashData, hashtype, sign, SIGN_MODE)){
            tmp_err = ERROR;
        }
        if(0U == BN_getBitLen(sign)){
            tmp_err = ERROR;
        }
    }
    else{
        tmp_err = ERROR;
    }

    return tmp_err;
}

ErrStatus LL_RSA_sign_ver(uRSA_t *rsa, const uint32_t hashData[], uint32_t hashtype, uint32_t sign[])
{
    uint32_t i, tmp;
    uint32_t result[RSA_KSIZE];
    ErrStatus tmp_err = SUCCESS;

    for(i = 0U; i < RSA_KSIZE; i++){
        result[i] = 0U;
    }
    sea_init_all();

    if((SUCCESS == rsa_key_ver(rsa)) && (hashtype == BN_getBitLen(hashData))){
        if(SUCCESS != rsa_decrypt(rsa, sign, BN_getBitLen(sign), result, SIGN_MODE)){
            tmp_err = ERROR;
        }
        else{
            tmp = hashtype >> 5;
            for(i = 0U; i < RSA_KSIZE; i++){
                if(i == tmp){
                    break;
                }
                if(hashData[i] == result[i]){
                    continue;
                }
                else{
                    tmp_err = ERROR;
                    break;
                }
            }
        }
    }
    else{
        tmp_err = ERROR;
    }

    return tmp_err;
}

#else
ErrStatus LL_RSA_keygen(uRSA_t *rsa)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_RSA_keygen func_ptr = (pLL_RSA_keygen)((uint32_t)Iap_Func->LL_RSA_keygen + ROM_BASE_ADDR);
    return func_ptr(rsa);
}

ErrStatus LL_RSA_sign_gen(uRSA_t *rsa, const uint32_t hashData[], uint32_t hashtype, uint32_t sign[])
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_RSA_sign_gen func_ptr = (pLL_RSA_sign_gen)((uint32_t)Iap_Func->LL_RSA_sign_gen + ROM_BASE_ADDR);
    return func_ptr(rsa, hashData, hashtype, sign);
}

ErrStatus LL_RSA_sign_ver(uRSA_t *rsa, const uint32_t hashData[], uint32_t hashtype, uint32_t sign[])
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_RSA_sign_ver func_ptr = (pLL_RSA_sign_ver)((uint32_t)Iap_Func->LL_RSA_sign_ver + ROM_BASE_ADDR);
    return func_ptr(rsa, hashData, hashtype, sign);
}

ErrStatus LL_RSA_pub_encrypt(uRSA_t *rsa, const uint32_t plain[], uint32_t plainlen, uint32_t cipher[])
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_RSA_pub_encrypt func_ptr = (pLL_RSA_pub_encrypt)((uint32_t)Iap_Func->LL_RSA_pub_encrypt + ROM_BASE_ADDR);
    return func_ptr(rsa, plain, plainlen, cipher);
}

ErrStatus LL_RSA_pub_decrypt(uRSA_t *rsa, const uint32_t cipher[], uint32_t cipherlen, uint32_t plain[])
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_RSA_pub_decrypt func_ptr = (pLL_RSA_pub_decrypt)((uint32_t)Iap_Func->LL_RSA_pub_decrypt + ROM_BASE_ADDR);
    return func_ptr(rsa, cipher, cipherlen, plain);
}

ErrStatus LL_RSA_pvt_encrypt(uRSA_t *rsa, const uint32_t plain[], uint32_t plainlen, uint32_t cipher[])
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_RSA_pvt_encrypt func_ptr = (pLL_RSA_pvt_encrypt)((uint32_t)Iap_Func->LL_RSA_pvt_encrypt + ROM_BASE_ADDR);
    return func_ptr(rsa, plain, plainlen, cipher);
}

ErrStatus LL_RSA_pvt_decrypt(uRSA_t *rsa, const uint32_t cipher[], uint32_t cipherlen, uint32_t plain[])
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_RSA_pvt_decrypt func_ptr = (pLL_RSA_pvt_decrypt)((uint32_t)Iap_Func->LL_RSA_pvt_decrypt + ROM_BASE_ADDR);
    return func_ptr(rsa, cipher, cipherlen, plain);
}
#endif
#endif
/* r2725 */
