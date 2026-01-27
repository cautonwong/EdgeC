/******************************************************************************
 * @file        rn8xxx_ll_sea_common.h
 * @brief       configuration for SEA Engine
 * @author      Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 *****************************************************************************/
#ifndef SEA_CYPHER_H_
#define SEA_CYPHER_H_

#include "rn8xxx_ll_sea_def.h"

#if !defined(RN831x_RN861x_MCU_V4)
#ifdef RN202x_RN7326_SOC_V2
#define SEA_CYPHER                          ((SEA_CYPHER_TypeDef              *) 0x50008000     )
#define SEA_AES_IV                          ((SEA_AES_IV_TypeDef              *) 0x50008150     )
#define SEA_EMB_BASEADDR                    (0x50009000)

#define SYSCTRL_PS                          (*((volatile uint32_t             *) 0x4003403cU   ))
#define SYSCTRL_MOD0_EN                     (*((volatile uint32_t             *) 0x40034028U   ))

#else
#define SEA_CYPHER                          ((SEA_CYPHER_TypeDef              *) 0x5000C000     )

#ifdef  RN821x_RN721x_SOC_V3
#define SEA_AES_IV                          ((SEA_AES_IV_TypeDef              *) 0x5000C008     )
#else
#define SEA_AES_IV                          ((SEA_AES_IV_TypeDef              *) 0x5000C150     )
#endif
#define SEA_EMB_BASEADDR                    (0x5000D000)

#define SYSCTRL_PS                          (*((volatile uint32_t             *) 0x40034030U     ))
#define SYSCTRL_MOD0_EN                     (*((volatile uint32_t             *) 0x4003401cU     ))
#endif

/*--------------------- SEA --------------------------------------------------*/
#ifndef  RN821x_RN721x_SOC_V3
typedef struct {
    volatile  uint32_t  Reserved[64];              /*0x0~0xFC*/
    volatile  uint32_t  START;                     /*0x100*/
    volatile  uint32_t  CTRL;                      /*0x104*/
    volatile  uint32_t  N0Low;                     /*0x108*/
    volatile  uint32_t  N0High;                    /*0x10C*/
    volatile  uint32_t  NLen;                      /*0x110*/
    volatile  uint32_t  ELen;                      /*0x114*/
    volatile  uint32_t  IE;                        /*0x118*/
    volatile  uint32_t  Status;                    /*0x11C*/
    volatile  uint32_t  GHASH_X[4];                /*0x120~0x12C*/
    volatile  uint32_t  GHASH_Y[4];                /*0x130~0x13C*/
    volatile  uint32_t  GHASH_Z[4];                /*0x140~0x14C*/
    volatile  uint32_t  IVector[4];                /*0x150~0x15C*/
    volatile  uint32_t  Reserved1[153];            /*0x160~0x3c0*/
    volatile  uint32_t  TRNG_CTL0;                 /*0x3C4*/
    volatile  uint32_t  TRNG_CTL1;                 /*0x3C8*/
    volatile  uint32_t  TRNG_CTL2;                 /*0x3CC*/
    volatile  uint32_t  TRNG_STATE;                /*0x3D0*/
    volatile  uint32_t  TRNG_BUF[4];               /*0x3D4~0x3E0*/
} SEA_CYPHER_TypeDef;
#else
typedef struct {
    volatile  uint32_t  START;                     /*0x00*/
    volatile  uint32_t  CTRL;                      /*0x04*/
    volatile  uint32_t  IVector[4];                /*0x08~0x14*/
    volatile  uint32_t  IE;                        /*0x18*/
    volatile  uint32_t  Status;                    /*0x1c*/
    volatile  uint32_t  AES_DMA_ADDR;              /*0x20*/
    volatile  uint32_t  GHASH_X[4];                /*0x24~0x30*/
    volatile  uint32_t  GHASH_Y[4];                /*0x34~0x40*/
    volatile  uint32_t  GHASH_Z[4];                /*0x44~0x50*/
    volatile  uint32_t  Reserved_54;               /*0x54*/
    volatile  uint32_t  TRNG_CTL0;                 /*0x58*/
    volatile  uint32_t  TRNG_CTL1;                 /*0x5c*/
    volatile  uint32_t  TRNG_CTL2;                 /*0x60*/
    volatile  uint32_t  TRNG_STATE;                /*0x64*/
    volatile  uint32_t  TRNG_BUF[4];               /*0x68~0x74*/
} SEA_CYPHER_TypeDef;
#endif

typedef struct {
    volatile  uint32_t  KEY_EXP[4*15];            /*0x0~0xFC*/
} SEA_AES_EXT_KEY_TypeDef;

typedef struct {
    volatile  uint32_t  DATA_IN[4];
} SEA_AES_CYPHER_IN_TypeDef;

typedef struct {
    volatile  uint32_t  DATA_OUT[4];
} SEA_AES_CYPHER_OUT_TypeDef;

typedef struct {
    volatile  uint32_t  IV[4];
} SEA_AES_IV_TypeDef;

#ifdef  RN821x_RN721x_SOC_V3
typedef struct {
    SEA_AES_CYPHER_IN_TypeDef   AES_DMA_IN  ;
    SEA_AES_CYPHER_OUT_TypeDef  AES_DMA_OUT ;
    SEA_AES_EXT_KEY_TypeDef     AES_DMA_KEY;
}SEA_AES_DMA_RAM_Typedef;

typedef struct {
uint32_t h[8];
uint32_t w[80];
}SW_HASH256_Engine_t;

typedef struct {
uint64_t h[8] ;
uint64_t w[80];
}SW_HASH512_Engine_t;

typedef struct {
    volatile  uint32_t  HASH_DOUR[16] ;             /**/
} SEA_HASH_TypeDef;

#else

typedef struct {
    volatile  uint32_t  HASH_IVR[16]  ;             /**/
    volatile  uint32_t  HASH_DINR[32] ;             /**/
    volatile  uint32_t  Reserved4[464];             /**/
    volatile  uint32_t  HASH_DOUR[16] ;             /**/
} SEA_HASH_TypeDef;
#endif

typedef struct {
    volatile  uint32_t  REG[18]  ;
} SEA_ECC_REG18_t;

typedef struct {
    volatile  uint32_t  REG[20]  ;
} SEA_ECC_REG20_t;

typedef struct {
  volatile uint32_t REG[32];
} SEA_RSA_REG32_t;

#define SEA_RSA_MOD_T0                      ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0x000 ))
#define SEA_RSA_MOD_N                       ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0x200 ))
#define SEA_RSA_MOD_T1                      ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0x800 ))
#define SEA_RSA_H                           ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0xA00 ))
#define SEA_RSA_MAP_R0                      ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0x000 ))
#define SEA_RSA_MAP_R0T                     ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0x800 ))
#define SEA_RSA_MAP_R1                      ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0x100 ))
#define SEA_RSA_MAP_R1T                     ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0x900 ))
#define SEA_RSA_MAP_N                       ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0x200 ))
#define SEA_RSA_MAP_E                       ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0x300 ))
#define SEA_RSA_MAP_H                       ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0xA00 ))
#define SEA_RSA_MAP_M                       ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0xB00 ))
#define SEA_RSA_MAP_X                       ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0x000 ))
#define SEA_RSA_MAP_N                       ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0x200 ))
#define SEA_RSA_MAP_Y                       ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0x800 ))
#define SEA_RSA_MAP_H                       ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0xA00 ))
#define SEA_RSA_MAP_M                       ((SEA_RSA_REG32_t *)(SEA_EMB_BASEADDR + 0xB00 ))

#define SEA_ECC_MOD_P                       ((SEA_ECC_REG20_t *)(SEA_EMB_BASEADDR + 0x120 ))
#define SEA_ECC_MOD_PT                      ((SEA_ECC_REG20_t *)(SEA_EMB_BASEADDR + 0x920 ))
#define SEA_ECC_MODADD_ADDX                 ((SEA_ECC_REG20_t *)(SEA_EMB_BASEADDR + 0x000 ))
#define SEA_ECC_MODADD_ADDY                 ((SEA_ECC_REG20_t *)(SEA_EMB_BASEADDR + 0x800 ))
#define SEA_ECC_H                           ((SEA_ECC_REG18_t *)(SEA_EMB_BASEADDR + 0x8D8 ))
#define SEA_ECC_MAP_Q0_X                    ((SEA_ECC_REG18_t *)(SEA_EMB_BASEADDR + 0x000 ))
#define SEA_ECC_MAP_Q0_Y                    ((SEA_ECC_REG18_t *)(SEA_EMB_BASEADDR + 0x800 ))
#define SEA_ECC_MAP_Q0_Z                    ((SEA_ECC_REG18_t *)(SEA_EMB_BASEADDR + 0x090 ))
#define SEA_ECC_MAP_Q1_X                    ((SEA_ECC_REG18_t *)(SEA_EMB_BASEADDR + 0x048 ))
#define SEA_ECC_MAP_Q1_Y                    ((SEA_ECC_REG18_t *)(SEA_EMB_BASEADDR + 0x848 ))
#define SEA_ECC_MAP_Q1_Z                    ((SEA_ECC_REG18_t *)(SEA_EMB_BASEADDR + 0x890 ))
#define SEA_ECC_MAP_A                       ((SEA_ECC_REG18_t *)(SEA_EMB_BASEADDR + 0x0D8 ))
#define SEA_ECC_MAP_U                       ((SEA_ECC_REG18_t *)(SEA_EMB_BASEADDR + 0x630 ))
#define SEA_ECC_MAP_UT                      ((SEA_ECC_REG18_t *)(SEA_EMB_BASEADDR + 0xE30 ))
#define SEA_ECC_MAP_UZ                      ((SEA_ECC_REG18_t *)(SEA_EMB_BASEADDR + 0xDA0 ))
#define SEA_ECC_MAP_CONST                   ((SEA_ECC_REG18_t *)(SEA_EMB_BASEADDR + 0x9B0 ))

#ifndef  RN821x_RN721x_SOC_V3
#define SEA_AES_EXP_KEY                     ((SEA_AES_EXT_KEY_TypeDef         *)(SEA_EMB_BASEADDR +  0x760 ))
#define SEA_AES_CYPHER_IN                   ((SEA_AES_CYPHER_IN_TypeDef       *)(SEA_EMB_BASEADDR +  0x740 ))
#define SEA_AES_CYPHER_OUT                  ((SEA_AES_CYPHER_OUT_TypeDef      *)(SEA_EMB_BASEADDR +  0xF90 ))
#endif

#ifndef  RN821x_RN721x_SOC_V3
#define SEA_HASH                            ((SEA_HASH_TypeDef                *)(SEA_EMB_BASEADDR +  0x000 ))
#endif

/*SEA CTL*/
#define SEA_CTRL_SHA_MOD_Pos                (24UL) /*!< Position of SHA MOD field. */
#define SEA_CTRL_SHA_MOD_Msk                (0x7UL << SEA_CTRL_SHA_MOD_Pos)

#define SEA_CTRL_AES_MOD_Pos                (18UL) /*!< Position of ASE MOD field. */
#define SEA_CTRL_AES_MOD_Msk                (0x7UL << SEA_CTRL_AES_MOD_Pos)

#define SEA_CTRL_AES_LEN_Pos                (16UL) /*!< Position of ASE LEN field. */
#define SEA_CTRL_AES_LEN_Msk                (0x3UL << SEA_CTRL_AES_LEN_Pos)

#define SEA_CTRL_IR_Pos                     (0UL) /*!< Position of SEA_CTRL_IR_Pos field. */
#define SEA_CTRL_IR_Msk                     (0xFFUL << SEA_CTRL_IR_Pos)
#define SEA_CTRL_IR_ECC_H_CAL               0x01U
#define SEA_CTRL_IR_ECC_ModMul              0x02U
#define SEA_CTRL_IR_ECC_ADD_I2M             0x03U
#define SEA_CTRL_IR_ECC_DBL_I2M             0x04U
#define SEA_CTRL_IR_ECC_Point_ADD           0x05U
#define SEA_CTRL_IR_ECC_Point_DBL           0x06U
#define SEA_CTRL_IR_ECC_M2I                 0x07U
#define SEA_CTRL_IR_ECC_ModInv              0x08U
#define SEA_CTRL_IR_ECC_ModAdd              0x09U
#define SEA_CTRL_IR_ECC_ModSub              0x0aU
#define SEA_CTRL_IR_RSA_H_CAL               0x11U
#define SEA_CTRL_IR_RSA_ModMult             0x12U
#define SEA_CTRL_IR_RSA_ModExp              0x13U
#define SEA_CTRL_IR_PKA_J0_CAL              0x20U
#define SEA_CTRL_IR_AES_KeyExp              0x40U
#define SEA_CTRL_IR_AES_Enc                 0x42U
#define SEA_CTRL_IR_AES_Dec                 0x43U
#define SEA_CTRL_IR_AES_GF128               0x80U
#define SEA_CTRL_IR_SHA0                    0x90U
#define SEA_CTRL_IR_SHA2                    0x91U

/*TIME OUT CLKCNT*/
#define SEA_TIMEOUT_IR_ECC_H_CAL        (80000U)
#define SEA_TIMEOUT_IR_ECC_ModMul       (80000U)
#define SEA_TIMEOUT_IR_ECC_ADD_I2M      (8000U)
#define SEA_TIMEOUT_IR_ECC_DBL_I2M      (400000U)
#define SEA_TIMEOUT_IR_ECC_Point_ADD    (400000U)
#define SEA_TIMEOUT_IR_ECC_Point_DBL    (400000U)
#define SEA_TIMEOUT_IR_ECC_M2I          (40000U)
#define SEA_TIMEOUT_IR_ECC_ModInv       (400000U)
#define SEA_TIMEOUT_IR_ECC_ModAdd       (400U)
#define SEA_TIMEOUT_IR_ECC_ModSub       (400U)
#define SEA_TIMEOUT_IR_RSA_H_CAL        (80000U)
#define SEA_TIMEOUT_IR_RSA_ModMult      (80000U)
#define SEA_TIMEOUT_IR_RSA_ModExp       (400000U)
#define SEA_TIMEOUT_IR_PKA_J0_CAL       (400U)
#define SEA_TIMEOUT_IR_AES_KeyExp       (400U)
#define SEA_TIMEOUT_IR_AES_Enc          (400U)
#define SEA_TIMEOUT_IR_AES_Dec          (400U)
#define SEA_TIMEOUT_IR_AES_GF128        (400U)
#define SEA_TIMEOUT_IR_SHA0             (400U)
#define SEA_TIMEOUT_IR_SHA2             (400U)

/*SEA STATUS*/
#define SEA_STA_TRNG_ERROR_Pos      (7UL)
#define SEA_STA_TRNG_ERROR_Msk      (0x1UL << SEA_STA_TRNG_ERROR_Pos)
#define SEA_STA_TRNG_ERROR_DONE     (0x01)

#define SEA_STA_TRNG_Pos            (6UL)
#define SEA_STA_TRNG_Msk            (0x1UL << SEA_STA_TRNG_Pos)
#define SEA_STA_TRNG_DONE           (0x01)

#define SEA_STA_HASH_Pos            (5UL)
#define SEA_STA_HASH_Msk            (0x1UL << SEA_STA_HASH_Pos)
#define SEA_STA_HASH_DONE           (0x01)

#define SEA_STA_GHASH_Pos           (4UL)
#define SEA_STA_GHASH_Msk           (0x1UL << SEA_STA_GHASH_Pos)
#define SEA_STA_GHASH_DONE          (0x01)

#define SEA_STA_AES_Pos             (3UL)
#define SEA_STA_AES_Msk             (0x1UL << SEA_STA_AES_Pos)
#define SEA_STA_AES_DONE            (0x01)

#define SEA_STA_ECC_Pos             (2UL)
#define SEA_STA_ECC_Msk             (0x1UL << SEA_STA_ECC_Pos)
#define SEA_STA_ECC_DONE            (0x01)

#define SEA_STA_MODINV_Pos          (1UL)
#define SEA_STA_MODINV_Msk          (0x1UL << SEA_STA_MODINV_Pos)
#define SEA_STA_MODINV_DONE         (0x01)

#define SEA_STA_MUX_Pos             (0UL) /*!< Position of SEA_STATUS DONE MUX field. */
#define SEA_STA_MUX_Msk             (0x1UL << SEA_STA_MUX_Pos)
#define SEA_STA_MUX_DONE            (0x01)

void endian_rbytes(uint32_t *p_disc, const uint32_t *p_src, uint32_t len_w);
void endian_rbytes_size(uint32_t *p_disc, const uint32_t *p_src, uint32_t size);

/******************************************************************************
 * Utility functions
 *****************************************************************************/
#define    CYPHER_OK                          0x0
#define    AES_CRYPTKEY_ADDR_ALIGN_FAILURE    0x1
#define    AES_INIT_ADDR_ALIGN_FAILURE        0x2
#define    AES_CRYPT_FAILURE                  0x3
#define    AES_DECRYPT_FAILURE                0x4
#define    AES_GMAC_FAILURE                   0x5
#define    SHA_DATA_ALIGN_FAILURE             0x11
#define    SHA_DATALEN_FAILURE                0x12
#define    SHA_DATASIGN_FAILURE               0x14
#define    ECC_TIMEOUT_FAILURE                0x21
#define    ECC_PARASLEN_FAILURE               0x22
#define    ECC_LOOPTIMETOUT_FAILURE           0x24
#define    ECC_PKEY_FAILURE                   0x28
#define    RSA_TIMEOUT_FAILURE                0x31
#define    RSA_PARASLEN_FAILURE               0x32
#define    RSA_KEY_FAILURE                    0x33
#define    RSA_PADDING_FAILURE                0x34

extern void setErrorCode(uint32_t code)   ;
extern void clrErrorCode(uint32_t code)   ;
extern uint32_t checkErrors(void)         ;

extern SEA_G_PARAS  g_cypher_paras;

void sea_enable(void);
void sea_disable(void);
#endif

void sea_init_all(void);
void sea_trng_init(void);

void sea_hash_init(void);
void sea_hash_deinit(void);

void sea_aes_init(void);
void sea_aes_deinit(void);
void sea_ecc_init(void);
void sea_rsa_init(void);

#if !defined(RN831x_RN861x_MCU_V4)
void sea_memset_word(uint32_t dst[], uint32_t data, uint32_t len);
void sea_memset(uint8_t dst[], const uint8_t data, uint32_t len);
void sea_memcpy_word(uint32_t dst[], const uint32_t src[], uint32_t len);
void sea_memcpy(uint8_t dst[], const uint8_t src[], uint32_t len);

/* void endian_swap_byte(uint8_t *p_in, uint32_t *p_out, uint32_t len_w);*/

unsigned int LL_SEA_version(void)   ;
unsigned int GetSeaCodeVersion(void);

#define SEA_MEMCPY(dst, src, len)  (void)memcpy((dst), (src), (len))

#else
extern aesProcessParameters  aes_params;
extern hashProcessParameters hash_params;
#endif
#endif
/* r2726 */
