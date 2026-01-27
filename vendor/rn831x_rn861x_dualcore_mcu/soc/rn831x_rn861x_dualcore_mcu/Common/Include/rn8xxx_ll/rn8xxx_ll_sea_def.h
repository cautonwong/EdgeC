/******************************************************************************
 * @file        rn8xxx_ll_sea_def.h
 * @brief       paramters for the SEA Engine
 * @author      Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 *****************************************************************************/
#ifndef SEA_CYPHER_PARAMETERS_H_
#define SEA_CYPHER_PARAMETERS_H_

#include "renergy_chip_config.h"

#include <stdint.h>
#include <string.h>

/*
 * Arm Compiler 4/5
 */
#if   defined ( __CC_ARM )
      #include "cmsis_armcc.h"
/*
 * IAR Compiler
 */
#elif defined ( __ICCARM__ )
      #if 0
      #include "cmsis_iar.h"
      #else
      #include "cmsis_iccarm.h"
      #endif
/*
 * GNU Compiler
 */
#elif defined ( __GNUC__ )

#if defined(RN831x_RN861x_MCU_V4)
#include "nmsis_gcc.h"
#else
#include "cmsis_gcc.h"
#endif

#else

#if defined(RN831x_RN861x_MCU_V4)
#include "nmsis_compiler.h"
#else
#include "cmsis_compiler.h" /* CMSIS compiler specific defines */
#endif

#endif

#if defined(RN831x_RN861x_MCU_V4)
#include "core_compatiable.h"

typedef enum __attribute__((__packed__))
{
    RN202x_RN7326_SOC_V2 = 0,
    RN821x_RN721x_SOC_V3 = 1,
    RN831x_RN861x_MCU_V4_CORE0 = 2,
    RN831x_RN861x_MCU_V4__CORE1 = 3,
    OTHERS_CHIPID,
} rn_chipID_t;
#endif

/******************************************************************************
 * support byte aligned
 *****************************************************************************/
#define AES_CRYPT_ITEMS_IN_BYTES
#define HASH_CRYPT_ITEMS_IN_BYTES
#define ECC_CRYPT_ITEMS_IN_BYTES

/******************************************************************************
 * support hash PRCESS Latch & Recover
 *****************************************************************************/
#define HASH_PROCESS_LATCH_RECOVER_SUPPORT

/******************************************************************************
 * support AES PRCESS Latch & Recover
 *****************************************************************************/
#define AES_PROCESS_LATCH_RECOVER_SUPPORT

/******************************************************************************
 * support AES GCM GMAC ONLY
 *****************************************************************************/
#define AES_DECRYPT_GCM_SPECIAL_MOD_TYPE 0

#define NB    4U
/* Number of 32-bit words comprising the cypher key */
#define NK_AES128    4U
#define NK_AES192    6U
#define NK_AES256    8U

/* Number of bytes comprising the cypher key */
#define AES128_KEYSIZE    ((unsigned char)(4U*NK_AES128))
#define AES192_KEYSIZE    ((unsigned char)(4U*NK_AES192))
#define AES256_KEYSIZE    ((unsigned char)(4U*NK_AES256))

/* input and output buffer blocks processed every turn */
#define AES_PLAIN_BSIZE            (4U*NB)
#define AES_CRYPT_BSIZE            (4U*NB)
#define AES_BLOCK_BSIZE            (4U*NB)
#define AES_BLOCK_BSHIFT           (4U)

/******************************************************************************
 * State columns
 * fixed for all allowed configurations
 * HASH
 *****************************************************************************/
/* SHA  MOD NUMBER */
#define NMOD_SHA          (5U)

/* SHA  OUTPUT */
#define NOUT_SHA_1        (5U)
#define NOUT_SHA_224      (7U)
#define NOUT_SHA_256      (8U)
#define NOUT_SHA_384      (12U)
#define NOUT_SHA_512      (16U)

#define NOUT_SHA_MAX      (NOUT_SHA_512)

/* SHA  IV */
#define NIV_SHA_160        (5U)
#define NIV_SHA_224_256    (8U)
#define NIV_SHA_384_256    (16U)

#define HASH_SHA_160_256_IV_BSIZE    (16U*NB)
#define HASH_SHA_384_512_IV_BSIZE    (32U*NB)

/* SHA Number of 32-bit words comprising the block data */
#define NBLOCK_DATA_SHA_160_256    (16U)
#define NBLOCK_DATA_SHA_384_512    (32U)

#define NBLOCK_DATA_SHA_160_256_BSIZE (4U * NBLOCK_DATA_SHA_160_256)
#define NBLOCK_DATA_SHA_384_512_BSIZE (4U * NBLOCK_DATA_SHA_384_512)

/* SHA BLOCK SHIFT */
/* SHA Number of byte comprising the block data shift */
#define BSHIFT_BLOCK_DATA_SHA_160_256    (6U)
#define BSHIFT_BLOCK_DATA_SHA_384_512    (7U)

#define NBLOCK_DATA_SHA_MAX     (uint8_t)(NBLOCK_DATA_SHA_384_512)

/******************************************************************************
 * size of the cypher items in bytes/32bit words
 *****************************************************************************/
#ifdef AES_CRYPT_ITEMS_IN_BYTES
/* input/output items are in bytes */
typedef uint8_t  plainData_t;
typedef uint8_t  cryptData_t;
typedef uint8_t  add_t;
typedef uint8_t  ghashData_t;
typedef uint8_t  iv_t;
#define AES_REGS_DIV_DATA_ITEM    16U
#else
/* input/output items are in words */
typedef uint32_t plainData_t;
typedef uint32_t cryptData_t;
typedef uint32_t add_t;
typedef uint32_t ghashData_t;
typedef uint32_t iv_t;
#define AES_REGS_DIV_DATA_ITEM    4U
#endif

#ifdef HASH_CRYPT_ITEMS_IN_BYTES
/* input/output items are in bytes */
typedef uint8_t  hashData_t;
#else
typedef uint32_t hashData_t ;
#endif

typedef uint32_t regData_t;

/* key is specified in bytes */
typedef uint8_t  aesKey_t;

/* tag is specified in bytes */
typedef uint8_t  tag_t;

#ifdef ECC_CRYPT_ITEMS_IN_BYTES
typedef uint8_t eccData_t;
#else
typedef uint32_t eccData_t;
#endif

typedef enum __attribute__((__packed__)) {
    SEA_MSG_BIG_ENDIAN    = 0x0,
    SEA_MSG_LITTLE_ENDIAN = 0x1,
} SEA_ENDIAN_MOD;

typedef enum __attribute__((__packed__)) {
    CypherDatasNotTheLast     = 0x0,
    CypherDatasIsTheLast      = 0x1,
} CYPHER_DATA_STA;

typedef enum __attribute__((__packed__)) {
    AES_PLAIN_ONLY           = 0x0,
    AES_IV_ONLY              = 0x1,
    AES_IV_PLAIN3            = 0x2,
    AES_PLAIN_IV3            = 0x3,
} SEA_AES_CRYPT_STRC;

typedef struct {
    regData_t  h_y0[4]            ;
    regData_t  h_iv[4]            ;
    regData_t  h_add[4]           ;
    regData_t  h_predata[4]       ;
    regData_t  h_default[4]       ;
    regData_t  iv[4]              ;
    regData_t  cypher_predata[4]  ;
    regData_t  key[NK_AES256]     ;
    uint32_t   C_SIZE             ;
} SEA_AES_G_PARAS;

typedef struct {
    uint32_t   datas[NBLOCK_DATA_SHA_MAX];
    uint8_t    size_last   ;
    uint8_t    reserved[3] ;
} SEA_HASH_DATAIN_MANAGE;

#if !defined(RN831x_RN861x_MCU_V4)
typedef struct {
    regData_t               DATA[NOUT_SHA_MAX];
    SEA_HASH_DATAIN_MANAGE  datain;
}SEA_HASH_G_PARAS;
#endif

typedef struct {
    uint8_t   data8[0x20000]    ;
}SEA_REGS_BYTEMSK;

typedef struct {
    uint16_t  data16[0x10000]   ;
}SEA_REGS_hWordMSK;

typedef struct {
    uint32_t  data32[0x8000]   ;
}SEA_REGS_WordMSK;

/******************************************************************************
 * State columns
 * fixed for all allowed configurations
 * AES
 *****************************************************************************/
typedef enum __attribute__((__packed__)) {
    AES_SPC_NONE               = 0x0 ,
    AES_SPC_GCM_VERIFY_ONLY    = 0x1 , /*AES GCM decrypt_verify func, then  only VERIFY*/
    AES_SPC_GMAC_CryptData     = 0x2 , /*AES GCM GMAC,input data is crypt datas。*/
} AES_SPECIAL_MOD;

typedef enum __attribute__((__packed__)) {
    SEA_AES_128,
    SEA_AES_192,
    SEA_AES_256
} SEA_AES_LEN_MODE;

typedef enum __attribute__((__packed__)) {
   SEA_AES_ECB             = 0 ,
   SEA_AES_CBC             = 1 ,
   SEA_AES_CTR             = 2 ,
   SEA_AES_CFB128          = 3 ,
   SEA_AES_OFB             = 4 ,
} SEA_AES_CRYPT_MODE;

/******************************************************************************
 * internal parameters, donot care it
 *****************************************************************************/
#if !defined(RN831x_RN861x_MCU_V4)
typedef struct {
    SEA_AES_CRYPT_MODE        cryptMode              ;
    uint8_t                   reserved0[3]           ;
    uint32_t                  cryptSize              ;
    uint32_t                  aadSize                ;
    uint8_t                   crypt_ir               ;
    uint8_t                   is_start_ghash         ;
    uint8_t                   is_start_cryptKernel   ;
    uint8_t                   is_start_crypt         ;
    SEA_AES_CRYPT_STRC        crypt_kernelMode       ;
    uint8_t                   reserved1[3]           ;
    uint32_t                  mem_limit;
} aesEngineParameters;
#else
typedef struct {
    SEA_AES_CRYPT_MODE        cryptMode;
    SEA_AES_CRYPT_STRC        crypt_kernelMode;
    rn_chipID_t               chip_version;
    uint8_t                   is_last;
    uint32_t                  cryptSize;
    uint32_t                  aadSize;
    uint8_t                   crypt_ir;
    uint8_t                   is_start_ghash;
    uint8_t                   is_start_cryptKernel;
    uint8_t                   is_start_crypt;
    uint32_t                  mem_limit;
} aesEngineParameters;
#endif

 /******************************************************************************
 * Configuration of the algorithm type
 * Allowed values:
 *     - AES_128 for a 128 bit key
 *     - AES_192 for a 192 bit key
 *     - AES_256 for a 256 bit key
 *****************************************************************************/
typedef enum __attribute__((__packed__)) {
    AES_enCrypt,   /*crypt*/
    AES_deCrypt,   /*decrypt*/
} AES_ENCRYPT_SEL;

typedef enum __attribute__((__packed__)) {
    AES_128,
    AES_192,
    AES_256
} AES_BITWIDE_MODE;

typedef enum __attribute__((__packed__)) {
    AES_ECB             = 0 ,
    AES_CBC             = 1 ,
    AES_CTR             = 2 ,
    AES_CFB128          = 3 ,
    AES_OFB             = 4 ,
    AES_GCM             = 5 ,
} AES_CRYPT_MODE;

typedef enum __attribute__((__packed__)) {
    AES_VERIFY_SUCCESS    = 0x0 , /*tag verify success*/
    AES_VERIFY_FAIL       = 0x1 , /*tag verigy error*/
} AES_VERIFY_STA;

#if defined(RN831x_RN861x_MCU_V4)
typedef enum __attribute__((__packed__)) {
    AES_GENERAL_MODE_EN,
    AES_SECURE_MODE_EN,
} AES_RUNNING_MODE;

typedef enum __attribute__((__packed__)) {
    HASH_GENERAL_MODE_EN,
    HASH_DMA_MODE_EN,
} HASH_RUNNING_MODE;

typedef enum __attribute__((__packed__)) {
    ECC_GENERAL_MODE_EN,
    ECC_SECURE_MODE_EN,
} ECC_RUNNING_MODE;
#endif

typedef enum __attribute__((__packed__)) {
    AES_KW_MODE_KW,   /* key wrap using RFC-3394 */
    AES_KW_MODE_KWP,  /* key wrap with padding using RFC-5649 */
} AES_KW_MODE;

#if !defined(RN831x_RN861x_MCU_V4)
typedef struct {
    AES_ENCRYPT_SEL       encrypt_sel;  /*AES_enCrypt_sel or AES_deCrypt_sel*/
    AES_BITWIDE_MODE      bitWideMode;  /*AES_128;AES_192;AES_256*/
    AES_CRYPT_MODE        cryptMode  ;  /*AES_ECB;AES_GCM*/
    AES_KW_MODE           keyWarpMode;  /* AES_KW_MODE_KW/AES_KW_MODE_KWP */
    aesKey_t*                keyLocation;  /*pointer to  aes key location. specified in words align*/
    uint32_t              keySize;      /*size of the key in bytes; GCM only*/
    iv_t*                 ivLocation ;  /*pointer to Initialization vector location. specified in words align. valid on (CBC,CTR,CFB128,AES_OFB,GCM), except ECB*/
    uint32_t              ivSize ;      /*size of the Initialization vector in bytes; GCM only*/
    /*----the under paras is for GCM ONLY-----*/
    add_t*                addLocation  ; /*pointer to add message location. specified in words align.*/
    uint32_t              addSize      ; /*size of the add message in bytes; GCM only*/
   /*---tagSignBuf---*/
   /*when crypt and Sign, pointer to the buffer for tag out*/
   /*when decrypt and verify, pointer to the tag location, for tag verify*/
    tag_t*                tagSignBuf   ;
    uint32_t              tagSize      ; /*size of the key in bytes; valid by Sign & Verify*/
    tag_t*                tagVerifyBuf ; /*when decrypt and verify, pointer to the buffer for verify tag out*/
    /*---tagVerifySta---*/
   /*when decrypt and verify, pointer to the verify result flag*/
   /*return : = 0verify success;=1:verify error*/
    AES_VERIFY_STA*       tagVerifySta ;
    AES_SPECIAL_MOD       specialMode  ;
#ifdef  RN821x_RN721x_SOC_V3
    uint32_t       aesBlockRam ;
#endif
    uint8_t reserved[3];
    uint32_t              mem_limit;
} aesCypherConfigStr;
#else
typedef struct {
    AES_ENCRYPT_SEL encrypt_sel;  /* AES_enCrypt_sel/AES_deCrypt_sel */
    AES_BITWIDE_MODE bitWideMode; /* AES_128/AES_192/AES_256 */
    AES_CRYPT_MODE cryptMode;     /* AES_ECB/AES_CBC/AES_CTR/AES_CFB128/AES_OFB/AES_GCM */
    AES_RUNNING_MODE runningMode; /* AES_GENERAL_MODE_EN/AES_SECURE_MODE_EN */
    aesKey_t *keyLocation;        /* pointer to AES Key Location */
    iv_t *ivLocation;             /* pointer to IV(Initialization Vector) Location, except AES_ECB */
    uint32_t ivSize;              /* size of the IV in bytes, only effective in AES_GCM */
    add_t *addLocation;           /* pointer to AAD(Additional Authenticationed Data) location, only effective in AES_GCM */
    uint32_t addSize;             /* size of the AAD in bytes, only effective in AES_GCM */
    tag_t *tagSignBuf;            /* when encrypt and sign, pointer to the buffer for tag output. when decrypt and verify, pointer to the tag location, only effective in AES_GCM */
    uint32_t tagSize;             /* size of the tag in bytes, only effective in AES_GCM */
    tag_t *tagVerifyBuf;          /* when decrypt and verify, pointer to the buffer for verify tag out, only effective in AES_GCM */
    AES_VERIFY_STA *tagVerifySta; /* when decrypt and verify, pointer to the verify result flag, return =0:verify success; =1:verify error */
    AES_SPECIAL_MOD specialMode;  /* use AES_SPC_NONE, expect AES GMAC */
    AES_KW_MODE keyWarpMode;      /* AES_KW_MODE_KW/AES_KW_MODE_KWP */
    uint8_t keyIndex;             /* specify the keyIndex in KeyStore when runningMode is AES_SECURE_MODE_EN */
    uint8_t reserved;
    union
    {
        uint32_t keySize;
        uint32_t mem_limit;
    };
} aesCypherConfigStr;
#endif

/******************************************************************************
 *aes process parameters
 *****************************************************************************/
typedef struct  {
    aesCypherConfigStr      config      ;
    aesEngineParameters     engine      ;
    SEA_AES_G_PARAS         proc        ;
} aesProcessParameters;

typedef enum __attribute__((__packed__)) {
    SEA_SHA_1    = 0  ,
    SEA_SHA_224  = 1  ,
    SEA_SHA_256  = 2  ,
    SEA_SHA_NC3  = 3  ,
    SEA_SHA_384  = 4  ,
    SEA_SHA_512  = 5  ,
} SEA_SHA_MODE;

/******************************************************************************
 * size of the messages  in bytes
 *****************************************************************************/
typedef struct {
    uint32_t          size_64[2]   ;  /*NC, not used*/
    /*uint32_t          size_128[2]  ;*/  /*is valid in SHA384 and SAH512*/
} SHA_MSG_SIZE_MODE;

typedef enum __attribute__((__packed__)) {
    SHA_MSG_ONLY      ,
    SHA_MSG_FILL_ADD  ,
    SHA_MSG_FILL      ,
    SHA_ZERO_ADD      ,
}SHA_BLOCK_DATA_TPYE;

/******************************************************************************
 * configure  SHA128/SHA192/SHA256/SHA384/SHA512 initialization
 *****************************************************************************/
#if !defined(RN831x_RN861x_MCU_V4)
typedef struct {
    SEA_SHA_MODE        bitWideMode ; /* SHA-128/224/256/384/512 select*/
    uint8_t reserved[3];
    SHA_MSG_SIZE_MODE   msgSize     ; /* size of the messages  in bytes*/
} hashConfigStr;

/******************************************************************************
 * internal parameters, donot care it
 *****************************************************************************/
typedef struct  {
    uint32_t                *pIv;
    uint8_t                  ivLen;
    uint8_t                  blockSize      ;
    uint8_t                  blockShift     ;
    uint8_t                  MsgAddLenSize  ;
    uint8_t                  dataOutLen ;
    uint8_t                  ctr_ir     ;
    uint8_t                  is_start   ;
    uint8_t                  reserved   ;
    uint32_t                 messageSize;
} hashEngineParameters;

/******************************************************************************
 *hash process parameters
 *****************************************************************************/
typedef struct  {
    hashConfigStr            config      ;
    hashEngineParameters     engine      ;
    SEA_HASH_G_PARAS         proc        ;
} hashProcessParameters;
#else
typedef struct {
    SEA_SHA_MODE bitWideMode;      /* SHA-128/SHA-224/SHA-256/SHA-384/SHA-512 */
    HASH_RUNNING_MODE runningMode; /* HASH_GENERAL_MODE_EN or HASH_DMA_MODE_EN */
    uint8_t reserved[2];
} hashProcConfigStr;

typedef struct {
    union
    {
        struct
        {
            SEA_SHA_MODE bitWideMode;
            uint8_t reserved[3];
        };
        hashProcConfigStr proc_cfg;
    };
    SHA_MSG_SIZE_MODE msgSize;     /* size of the messages in bytes*/
} hashConfigStr;

/******************************************************************************
 * internal parameters, no need care
 *****************************************************************************/
typedef struct {
    uint32_t *pIv;
    uint8_t ivLen;
    uint8_t blockSize;
    uint8_t blockShift;
    uint8_t MsgAddLenSize;
    uint8_t dataOutLen;
    uint8_t ctr_ir;
    uint8_t is_start;
    uint8_t is_last;
    uint32_t messageSize;
    rn_chipID_t chip_version;
    uint8_t dma_enable;
    uint8_t reserved0[2];
} hashEngineParameters;

typedef struct {
    regData_t DATA[NOUT_SHA_MAX];
    SEA_HASH_DATAIN_MANAGE datain;
    SHA_BLOCK_DATA_TPYE block_type;
    uint8_t last_block;
} SEA_HASH_G_PARAS;

/******************************************************************************
 * HASH process parameters
 *****************************************************************************/
typedef struct {
	hashProcConfigStr config;
    hashEngineParameters engine;
    SEA_HASH_G_PARAS proc;
} hashProcessParameters;
#endif

typedef struct {
    SEA_AES_G_PARAS   aes_buf  ;
    SEA_HASH_G_PARAS  hash_buf ;
} SEA_G_PARAS;

typedef enum __attribute__((__packed__)) {
#if !defined(RN831x_RN861x_MCU_V4)
    SUCCESS = 0,
    ERROR = !SUCCESS
#else
    SEA_SUCCESS = 0,
    SEA_ERROR = !(SEA_SUCCESS),
    AES_CRYPTKEY_ADDR_ALIGN_FAILURE = 2,
    AES_CRYPT_FAILURE = 3,
    AES_DECRYPT_FAILURE = 4,
    AES_GMAC_FAILURE = 5,
    AES_NULL_POINTER_FAILURE = 6,
    AES_KW_MODE_FAILURE = 7,
    AES_KW_INLEN_FAILURE = 8,
    AES_KW_CHECK_ICV_FAILURE = 9,
    AES_KW_CHECK_PADDING_FAILURE = 10,
    SHA_DATA_ALIGN_FAILURE = 11,
    SHA_DATASIGN_FAILURE = 12,
    SHA_DMA_TIMEOUT_FAILURE = 13,
    SHA_DMA_DATA_TRANS_FAILURE = 14,
    ECC_TIMEOUT_FAILURE = 15,
    ECC_PARASLEN_FAILURE = 16,
    ECC_LOOPTIMETOUT_FAILURE = 17,
    ECC_PKEY_FAILURE = 18,
    RSA_TIMEOUT_FAILURE = 19,
    RSA_PARASLEN_FAILURE = 20,
    RSA_KEY_FAILURE = 21,
    RSA_PADDING_FAILURE = 22,
    SEA_KS_WRKEY_FAILURE = 23,
    SEA_KS_RDKEY_FAILURE = 24,
    SEA_TRNG_KEYGEN_FAILURE = 25,
    SEA_TRNG_KLEN_FAILURE = 26,
    SEA_TRNG_TIMEOUT_FAILURE = 27,
    SEA_TRNG_RANDOM_FAILURE = 28,
    SEA_FLASH_CLK_FREQ_FAILURE = 29,
    SEA_IAP_FAILURE = 30,
#endif
} ErrStatus;

typedef enum __attribute__((__packed__)) {
    PASS = 0,
    FAIL = !(PASS),
} PassFlag;

/*!< ECC curve definition */
typedef struct {
    uint8_t  num_words; /*ECC curve bit width is rounded up in words          */
    uint8_t  num_bytes; /*ECC curve bit width is rounded up in bytes          */
    uint16_t num_bits;  /*ECC curve bit width                                 */
    uint32_t p[17];     /*ECC curve modulus                                   */
    uint32_t n[17];     /*ECC curve rank                                      */
    uint32_t a[17];     /*ECC curve Weierstrass Equation coefficient a        */
    uint32_t b[17];     /*ECC curve Weierstrass Equation coefficient b        */
    uint32_t G[34];     /*ECC curve base point                                */
}uECC_Curve_t;

/**
 * @brief RSA structure definitions
 *        RSA Public Key : (N, E)
 *        RSA Private Key: (N, D)
*/
typedef struct {
    uint32_t rsa_keylen; /* key length of RSA                */
    uint32_t n_len;      /* bit length of modulus N          */
    uint32_t e_len;      /* bit length of public exponent E  */
    uint32_t d_len;      /* bit length of private exponent D */
    uint32_t n[32];      /* the RSA modulus                  */
    uint32_t e[32];      /* the RSA public exponent          */
    uint32_t d[32];      /* the RSA private exponent         */
} uRSA_t;

#if defined(RN831x_RN861x_MCU_V4)
#include "rn8xxx_ll_utils.h"
#include "rn8xxx_ll_def.h"

typedef struct
{
    uint32_t  Reserved[11];
    ErrStatus (*LL_AES_gf128_mult)              (aesProcessParameters *pParas, const unsigned char *pDataX, const unsigned char *pDataY, unsigned char *pDataZ);
    ErrStatus (*LL_AES_crypt_init)              (aesProcessParameters *pParas, const aesCypherConfigStr *configuration);
    ErrStatus (*LL_AES_crypt_deinit)            (void);
    ErrStatus (*LL_AES_crypt_sign)              (aesProcessParameters *pParas, const plainData_t *plainData, uint32_t size, CYPHER_DATA_STA is_last, cryptData_t *cryptedData);
    ErrStatus (*LL_AES_decrypt_verify)          (aesProcessParameters *pParas, const cryptData_t *cryptedData, uint32_t size, CYPHER_DATA_STA is_last, plainData_t *plainData);
    ErrStatus (*LL_AES_gmac)                    (aesProcessParameters *pParas, const ghashData_t *ghashData, uint32_t size, CYPHER_DATA_STA is_last);
    ErrStatus (*LL_AES_process_status_latch)    (aesProcessParameters *pParas, aesProcessParameters *latch_buf);
    ErrStatus (*LL_AES_process_status_recover)  (const aesProcessParameters *recover_buf, aesProcessParameters *pParas);
    ErrStatus (*LL_AES_key_wrap)                (aesProcessParameters *pParas, const plainData_t *plainData, uint32_t in_len, cryptData_t *cryptedData, uint32_t *out_len);
    ErrStatus (*LL_AES_key_unwrap)              (aesProcessParameters *pParas, const cryptData_t *cryptedData, uint32_t in_len, plainData_t *plainData, uint32_t *out_len);
    ErrStatus (*LL_AES_update_aad)              (aesProcessParameters *pParas, const add_t *aadData, uint32_t size);
    ErrStatus (*LL_AES_runOneTime)              (void);
    ErrStatus (*LL_HASH_init)                   (hashProcessParameters *pParas, const hashConfigStr *configuration);
    ErrStatus (*LL_HASH_deinit)                 (void);
    ErrStatus (*LL_HASH)                        (hashProcessParameters *pParas, const hashData_t *verifyMessage, uint32_t size, CYPHER_DATA_STA is_last, hashData_t *verifyOut);
    ErrStatus (*LL_HASH_process_status_latch)   (const hashProcessParameters *pParas, hashProcessParameters *latch_buf);
    ErrStatus (*LL_HASH_process_status_recover) (const hashProcessParameters *recover_buf, hashProcessParameters *pParas);
    ErrStatus (*LL_ECC_sign_gen)                (const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, eccData_t *_Q, eccData_t *_pkey, eccData_t *_r, eccData_t *_s);
    ErrStatus (*LL_ECC_specify_pkey_sign_gen)   (const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, eccData_t *_Q, const eccData_t *_pkey, eccData_t *_r, eccData_t *_s);
    ErrStatus (*LL_ECC_sign_ver)                (const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, const eccData_t *_Q, const eccData_t *_r, const eccData_t *_s, PassFlag *ver_flag);
    ErrStatus (*LL_ECC_DHkeygen)                (const uECC_Curve_t *curve, const eccData_t *_Q, const eccData_t *_pkey, eccData_t *_DHkey);
    ErrStatus (*LL_ECC_keygen)                  (const uECC_Curve_t *curve, eccData_t *_Q, eccData_t *_pkey);
    ErrStatus (*LL_ECC_public_keygen)           (const uECC_Curve_t *curve, eccData_t *_Q, const eccData_t *_pkey);
    ErrStatus (*LL_ECC_keyver)                  (const uECC_Curve_t *curve, const eccData_t *_Q, PassFlag *ver_flag);
    ErrStatus (*LL_ECC_keygen_sec)              (const uECC_Curve_t *curve, uint8_t pkeyIndex, uint8_t QxIndex, uint8_t QyIndex);
    ErrStatus (*LL_ECC_sign_gen_sec)            (const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, const uint8_t pkeyIndex, eccData_t *_r, eccData_t *_s);
    ErrStatus (*LL_ECC_sign_ver_sec)            (const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, const uint8_t QxIndex, const uint8_t QyIndex, const eccData_t *_r, const eccData_t *_s, PassFlag *ver_flag);
    ErrStatus (*LL_RSA_keygen)                  (uRSA_t *rsa);
    ErrStatus (*LL_RSA_sign_gen)                (uRSA_t *rsa, const uint32_t hashData[], uint32_t hashtype, uint32_t sign[]);
    ErrStatus (*LL_RSA_sign_ver)                (uRSA_t *rsa, const uint32_t hashData[], uint32_t hashtype, uint32_t sign[]);
    ErrStatus (*LL_RSA_pub_encrypt)             (uRSA_t *rsa, const uint32_t plain[], uint32_t plainlen, uint32_t cipher[]);
    ErrStatus (*LL_RSA_pub_decrypt)             (uRSA_t *rsa, const uint32_t cipher[], uint32_t cipherlen, uint32_t plain[]);
    ErrStatus (*LL_RSA_pvt_encrypt)             (uRSA_t *rsa, const uint32_t plain[], uint32_t plainlen, uint32_t cipher[]);
    ErrStatus (*LL_RSA_pvt_decrypt)             (uRSA_t *rsa, const uint32_t cipher[], uint32_t cipherlen, uint32_t plain[]);
    void      (*LL_TRNG_gen)                    (uint32_t random[], uint8_t data_len);
} SEA_ApiTypeDef;

typedef ErrStatus (*pLL_AES_gf128_mult)              (aesProcessParameters *pParas, const unsigned char *pDataX, const unsigned char *pDataY, unsigned char *pDataZ);
typedef ErrStatus (*pLL_AES_crypt_init)              (aesProcessParameters *pParas, const aesCypherConfigStr *configuration);
typedef ErrStatus (*pLL_AES_crypt_deinit)            (void);
typedef ErrStatus (*pLL_AES_crypt_sign)              (aesProcessParameters *pParas, const plainData_t *plainData, uint32_t size, CYPHER_DATA_STA is_last, cryptData_t *cryptedData);
typedef ErrStatus (*pLL_AES_decrypt_verify)          (aesProcessParameters *pParas, const cryptData_t *cryptedData, uint32_t size, CYPHER_DATA_STA is_last, plainData_t *plainData);
typedef ErrStatus (*pLL_AES_gmac)                    (aesProcessParameters *pParas, const ghashData_t *ghashData, uint32_t size, CYPHER_DATA_STA is_last);
typedef ErrStatus (*pLL_AES_process_status_latch)    (aesProcessParameters *pParas, aesProcessParameters *latch_buf);
typedef ErrStatus (*pLL_AES_process_status_recover)  (const aesProcessParameters *recover_buf, aesProcessParameters *pParas);
typedef ErrStatus (*pLL_AES_key_wrap)                (aesProcessParameters *pParas, const plainData_t *plainData, uint32_t in_len, cryptData_t *cryptedData, uint32_t *out_len);
typedef ErrStatus (*pLL_AES_key_unwrap)              (aesProcessParameters *pParas, const cryptData_t *cryptedData, uint32_t in_len, plainData_t *plainData, uint32_t *out_len);
typedef ErrStatus (*pLL_AES_update_aad)              (aesProcessParameters *pParas, const add_t *aadData, uint32_t size);
typedef ErrStatus (*pLL_AES_runOneTime)              (void);
typedef ErrStatus (*pLL_HASH_init)                   (hashProcessParameters *pParas, const hashConfigStr *configuration);
typedef ErrStatus (*pLL_HASH_deinit)                 (void);
typedef ErrStatus (*pLL_HASH)                        (hashProcessParameters *pParas, const hashData_t *verifyMessage, uint32_t size, CYPHER_DATA_STA is_last, hashData_t *verifyOut);
typedef ErrStatus (*pLL_HASH_process_status_latch)   (const hashProcessParameters *pParas, hashProcessParameters *latch_buf);
typedef ErrStatus (*pLL_HASH_process_status_recover) (const hashProcessParameters *recover_buf, hashProcessParameters *pParas);
typedef ErrStatus (*pLL_ECC_sign_gen)                (const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, eccData_t *_Q, eccData_t *_pkey, eccData_t *_r, eccData_t *_s);
typedef ErrStatus (*pLL_ECC_specify_pkey_sign_gen)   (const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, eccData_t *_Q, const eccData_t *_pkey, eccData_t *_r, eccData_t *_s);
typedef ErrStatus (*pLL_ECC_sign_ver)                (const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, const eccData_t *_Q, const eccData_t *_r, const eccData_t *_s, PassFlag *ver_flag);
typedef ErrStatus (*pLL_ECC_DHkeygen)                (const uECC_Curve_t *curve, const eccData_t *_Q, const eccData_t *_pkey, eccData_t *_DHkey);
typedef ErrStatus (*pLL_ECC_keygen)                  (const uECC_Curve_t *curve, eccData_t *_Q, eccData_t *_pkey);
typedef ErrStatus (*pLL_ECC_public_keygen)           (const uECC_Curve_t *curve, eccData_t *_Q, const eccData_t *_pkey);
typedef ErrStatus (*pLL_ECC_keyver)                  (const uECC_Curve_t *curve, const eccData_t *_Q, PassFlag *ver_flag);
typedef ErrStatus (*pLL_ECC_keygen_sec)              (const uECC_Curve_t *curve, uint8_t pkeyIndex, uint8_t QxIndex, uint8_t QyIndex);
typedef ErrStatus (*pLL_ECC_sign_gen_sec)            (const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, const uint8_t pkeyIndex, eccData_t *_r, eccData_t *_s);
typedef ErrStatus (*pLL_ECC_sign_ver_sec)            (const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, const uint8_t QxIndex, const uint8_t QyIndex, const eccData_t *_r, const eccData_t *_s, PassFlag *ver_flag);
typedef ErrStatus (*pLL_RSA_keygen)                  (uRSA_t *rsa);
typedef ErrStatus (*pLL_RSA_sign_gen)                (uRSA_t *rsa, const uint32_t hashData[], uint32_t hashtype, uint32_t sign[]);
typedef ErrStatus (*pLL_RSA_sign_ver)                (uRSA_t *rsa, const uint32_t hashData[], uint32_t hashtype, uint32_t sign[]);
typedef ErrStatus (*pLL_RSA_pub_encrypt)             (uRSA_t *rsa, const uint32_t plain[], uint32_t plainlen, uint32_t cipher[]);
typedef ErrStatus (*pLL_RSA_pub_decrypt)             (uRSA_t *rsa, const uint32_t cipher[], uint32_t cipherlen, uint32_t plain[]);
typedef ErrStatus (*pLL_RSA_pvt_encrypt)             (uRSA_t *rsa, const uint32_t plain[], uint32_t plainlen, uint32_t cipher[]);
typedef ErrStatus (*pLL_RSA_pvt_decrypt)             (uRSA_t *rsa, const uint32_t cipher[], uint32_t cipherlen, uint32_t plain[]);
typedef void      (*pLL_TRNG_gen)                    (uint32_t random[], uint8_t data_len);
#endif
#endif
/* r2896 */
