/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_sea.h
 * @author  Renergy Software Team
 * @brief   SEA module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_SEA_H
#define _RN831x_RN861x_MCU_SEA_H

#include "rn831x_rn861x_mcu.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/*                Device Specific Peripheral registers structures             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Peripherals rn831x_rn861x_mcu Peripherals
 * @brief rn831x_rn861x_mcu Device Specific Peripheral registers structures
 * @{
 */

/* SEA */
typedef struct
{
    __IO uint32_t RESERVED0[64];     /*!< offset: 0x000 ~ 0FC info: Reserved. */
    __IO uint32_t START;             /*!< offset: 0x100       info: SEA Start Register. */
    __IO uint32_t CTRL;              /*!< offset: 0x104       info: SEA Control Register. */
    __IO uint32_t N0LOW;             /*!< offset: 0x108       info: SEA FIPS Parameter Register 0. */
    __IO uint32_t N0HIGH;            /*!< offset: 0x10C       info: SEA FIPS Parameter Register 1. */
    __IO uint32_t NLEN;              /*!< offset: 0x110       info: SEA Module Length Register. */
    __IO uint32_t ELEN;              /*!< offset: 0x114       info: SEA Power Length Register. */
    __IO uint32_t IE;                /*!< offset: 0x118       info: SEA Interrupt Enable Register. */
    __IO uint32_t STA;               /*!< offset: 0x11C       info: SEA Status Register. */
    __IO uint32_t GHASH_X[4];        /*!< offset: 0x120 ~ 12C info: 128-bit GHASH Multiplicand X Registers. */
    __IO uint32_t GHASH_Y[4];        /*!< offset: 0x130 ~ 13C info: 128-bit GHASH Multiplier Y Registers. */
    __IO uint32_t GHASH_Z[4];        /*!< offset: 0x140 ~ 14C info: 128-bit GHASH Result Z Registers. */
    __IO uint32_t IV[4];             /*!< offset: 0x150 ~ 15C info: SEA IVector Registers. */
    __IO uint32_t AES_KEY_CTRL;      /*!< offset: 0x160       info: AES Key Control Register. */
    __IO uint32_t ECC_KEY_CTRL;      /*!< offset: 0x164       info: ECC Key Control Register. */
    __IO uint32_t HASH_DMA_IDATA;    /*!< offset: 0x168       info: HASH DMA Input Data Register. */
    __IO uint32_t HASH_DMA_STA;      /*!< offset: 0x16C       info: HASH DMA Status Register. */
    __IO uint32_t TRNG_KEY_CTRL;     /*!< offset: 0x170       info: TRNG Key Control Register. */
    __IO uint32_t RESERVED1[148];    /*!< offset: 0x174 ~ 3C0 info: Reserved. */
    __IO uint32_t TRNG_CTRL0;        /*!< offset: 0x3C4       info: TRNG Control Register 0. */
    __IO uint32_t TRNG_CTRL1;        /*!< offset: 0x3C8       info: TRNG Control Register 1. */
    __IO uint32_t TRNG_CTRL2;        /*!< offset: 0x3CC       info: TRNG Control Register 2. */
    __IO uint32_t TRNG_STA;          /*!< offset: 0x3D0       info: TRNG Status Register. */
    __IO uint32_t TRNG_BUF[4];       /*!< offset: 0x3D4 ~ 3E0 info: TRNG Buffer Registers. */
} SEA_TypeDef;

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_Peripherals */

/******************************************************************************/
/*                         Peripheral memory map                              */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_MemoryMap rn831x_rn861x_mcu Memory Mapping
 * @{
 */

/* base address */
#define SEA_CORE0_BASE  (0x30008000U)
#define SEA_CORE1_BASE  (0xB0008000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define SEA_CORE0  ((SEA_TypeDef *)      SEA_CORE0_BASE)
#define SEA_CORE1  ((SEA_TypeDef *)      SEA_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define SEA  SEA_CORE0
#endif
#if defined(CORE1)
#define SEA  SEA_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** SEA Instances *******************************/
#define IS_SEA_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == SEA_CORE0) || \
                                          ((INSTANCE) == SEA_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* SEA_START */
#define SEA_START_EN                         (BIT(0))                                            /*!< Start SEA running */

/* SEA_CTRL */
#define SEA_CTRL_IR                          (BITS(0,7))                                         /*!< Set different calculation methods. */
#define SEA_CTRL_AESLEN                      (BITS(16,17))                                       /*!< Set AES key length. */
#define SEA_CTRL_AESMOD                      (BITS(18,20))                                       /*!< Set AES data stream mode. */
#define SEA_CTRL_SHAMOD                      (BITS(24,26))                                       /*!< Set HASH calculation mode. */
#define SEA_CTRL_SHADMA                      (BIT(27))                                           /*!< Enable HASH DMA. */
#define SEA_CTRL_SHADMAREV                   (BIT(28))                                           /*!< Enable HASH DMA reverse input in byte. */

/* SEA_N0LOW */
#define SEA_N0LOW_VAL                        (BITS(0,31))                                        /*!< The low 32bits of Modular N */

/* SEA_N0HIGH */
#define SEA_N0HIGH_VAL                       (BITS(0,31))                                        /*!< The high 32bits of Modular N */

/* SEA_NLEN */
#define SEA_NLEN_LEN                         (BITS(0,11))                                        /*!< The bit length of Modular N */

/* SEA_ELEN */
#define SEA_ELEN_LEN                         (BITS(0,11))                                        /*!< The bit length of  Power E */

/* SEA_IE */
#define SEA_IE_PKE                           (BIT(2))                                            /*!< Enable ECC and RSA finish interrupt. */
#define SEA_IE_AES                           (BIT(3))                                            /*!< Enable AES finish interrupt. */
#define SEA_IE_GHASH                         (BIT(4))                                            /*!< Enable GHASH finish interrupt. */
#define SEA_IE_HASH                          (BIT(5))                                            /*!< Enable HASH finish interrupt. */
#define SEA_IE_TRNGRDY                       (BIT(6))                                            /*!< Enable TRNG data ready interrupt. */
#define SEA_IE_TRNGERR                       (BIT(7))                                            /*!< Enable TRNG random number detecion error interrupt. */
#define SEA_IE_KSGENERR                      (BIT(8))                                            /*!< Enabel store key generated by TRNG to Keystore error interrupt. */
#define SEA_IE_KSRDERR                       (BIT(9))                                            /*!< Enabel fail to read key from Keystore interrupt. */

/* SEA_STA */
#define SEA_STA_DONE                         (BIT(0))                                            /*!< SEA calculations completed flag. */
#define SEA_STA_MODINVRDY                    (BIT(1))                                            /*!< Modular Inversion operation can be possible. */
#define SEA_STA_PKEDONE                      (BIT(2))                                            /*!< ECC and RSA calculations completed flag. */
#define SEA_STA_AESDONE                      (BIT(3))                                            /*!< AES calculations completed flag. */
#define SEA_STA_GHASHDONE                    (BIT(4))                                            /*!< GHASH calculations completed flag. */
#define SEA_STA_HASHDONE                     (BIT(5))                                            /*!< HASH calculations completed flag. */
#define SEA_STA_TRNGRDY                      (BIT(6))                                            /*!< TRNG data ready pending interrupt flag. */
#define SEA_STA_TRNGERR                      (BIT(7))                                            /*!< TRNG random error interrupt flag. */
#define SEA_STA_KSGENERR                     (BIT(8))                                            /*!< Keystore write key interrupt flag. */
#define SEA_STA_KSRDERR                      (BIT(9))                                            /*!< Keystore read key interrupt flag. */
#define SEA_STA_SEABUSY                      (BIT(10))                                           /*!< SEA Module busy. */
#define SEA_STA_KSBUSY                       (BIT(11))                                           /*!< Keystore Module busy. */

/* SEA_GHASH_X */
#define SEA_GHASH_X_X                        (BITS(0,31))                                        /*!< The Multplicand X of GHASH */

/* SEA_GHASH_Y */
#define SEA_GHASH_Y_Y                        (BITS(0,31))                                        /*!< The Multiplier Y of GHASH */

/* SEA_GHASH_Z */
#define SEA_GHASH_Z_Z                        (BITS(0,31))                                        /*!< The Result Z of GHASH */

/* SEA_IV */
#define SEA_IV_IV                            (BITS(0,31))                                        /*!< AES IV buffer */

/* SEA_AES_KEY_CTRL */
#define SEA_AES_KEY_CTRL_SECMODE             (BIT(0))                                            /*!< Enable AES secure mode. */
#define SEA_AES_KEY_CTRL_LAST                (BIT(1))                                            /*!< AES last calculation flag. */
#define SEA_AES_KEY_CTRL_IDX                 (BITS(2,8))                                         /*!< AES key index in KeyStore. */

/* SEA_ECC_KEY_CTRL */
#define SEA_ECC_KEY_CTRL_SECMODE             (BIT(0))                                            /*!< Enable ECC secure mode. */
#define SEA_ECC_KEY_CTRL_IDX                 (BITS(1,7))                                         /*!< ECC key<x> index in KeyStore. */

/* SEA_HASH_DMA_IDATA */
#define SEA_HASH_DMA_IDATA_DATA              (BITS(0,31))                                        /*!< DMA Input Data */

/* SEA_HASH_DMA_STA */
#define SEA_HASH_DMA_STA_CNT                 (BITS(0,4))                                         /*!< DMA Status */

/* SEA_TRNG_KEY_CTRL */
#define SEA_TRNG_KEY_CTRL_SECMODE            (BIT(0))                                            /*!< Enable TRNG secure mode. */
#define SEA_TRNG_KEY_CTRL_ERRCNT             (BIT(1))                                            /*!< Still generate key with random error. */
#define SEA_TRNG_KEY_CTRL_READABLE           (BIT(2))                                            /*!< Set key is readable to CPU. */
#define SEA_TRNG_KEY_CTRL_LEN                (BITS(3,6))                                         /*!< Set key length. */
#define SEA_TRNG_KEY_CTRL_IDX                (BITS(7,13))                                        /*!< Set key index. */
#define SEA_TRNG_KEY_CTRL_ENC                (BITS(14,15))                                       /*!< Enable key encrypted store in KeyStore. */
#define SEA_TRNG_KEY_CTRL_ENCIDX             (BITS(16,22))                                       /*!< Set key index to encrypt current key. */

/* SEA_TRNG_CTRL0 */
#define SEA_TRNG_CTRL0_EN                    (BIT(0))                                            /*!< Enable TRNG calculation. */
#define SEA_TRNG_CTRL0_BYPASSCRC             (BIT(1))                                            /*!< Enable CRC or BypassCRC. */
#define SEA_TRNG_CTRL0_DAOSEL                (BIT(2))                                            /*!< Selet the interleaving algorithm. */
#define SEA_TRNG_CTRL0_BYPASSCRT             (BIT(3))                                            /*!< Enable Error correction algorithm. */
#define SEA_TRNG_CTRL0_LOOP0                 (BIT(4))                                            /*!< Enable Ring vibration loop 0. */
#define SEA_TRNG_CTRL0_LOOP1                 (BIT(5))                                            /*!< Enable Ring vibration loop 1. */
#define SEA_TRNG_CTRL0_LOOP2                 (BIT(6))                                            /*!< Enable Ring vibration loop 2. */
#define SEA_TRNG_CTRL0_LOOP3                 (BIT(7))                                            /*!< Enable Ring vibration loop 3. */
#define SEA_TRNG_CTRL0_LOOP4                 (BIT(8))                                            /*!< Enable Ring vibration loop 4. */
#define SEA_TRNG_CTRL0_LOOP5                 (BIT(9))                                            /*!< Enable Ring vibration loop 5. */
#define SEA_TRNG_CTRL0_LOOP6                 (BIT(10))                                           /*!< Enable Ring vibration loop 6. */
#define SEA_TRNG_CTRL0_LOOP7                 (BIT(11))                                           /*!< Enable Ring vibration loop 7. */
#define SEA_TRNG_CTRL0_CAPTURENUM            (BITS(16,31))                                       /*!< Defines random number sampling. */

/* SEA_TRNG_CTRL1 */
#define SEA_TRNG_CTRL1_ADDR0SEL              (BITS(0,2))                                         /*!< Select Bit0 to interweave new address. */
#define SEA_TRNG_CTRL1_ADDR1SEL              (BITS(4,6))                                         /*!< Select Bit1 to interweave new address. */
#define SEA_TRNG_CTRL1_ADDR2SEL              (BITS(8,10))                                        /*!< Select Bit2 to interweave new address. */
#define SEA_TRNG_CTRL1_ADDR3SEL              (BITS(12,14))                                       /*!< Select Bit3 to interweave new address. */
#define SEA_TRNG_CTRL1_ADDR4SEL              (BITS(16,18))                                       /*!< Select Bit4 to interweave new address. */
#define SEA_TRNG_CTRL1_ADDR5SEL              (BITS(20,22))                                       /*!< Select Bit5 to interweave new address. */
#define SEA_TRNG_CTRL1_ADDR6SEL              (BITS(24,26))                                       /*!< Select Bit6 to interweave new address. */

/* SEA_TRNG_CTRL2 */
#define SEA_TRNG_CTRL2_RPTERRTHD             (BITS(0,4))                                         /*!< Error correction starts when the number of detection cycle errors >=  the number of automatic error corrections. */
#define SEA_TRNG_CTRL2_FIXTHD                (BITS(5,9))                                         /*!< Error correction starts when the number of detection cycle errors >=  the number of configured threshold. */
#define SEA_TRNG_CTRL2_AUTOFIXTHD            (BITS(10,14))                                       /*!< Error correction starts when the number of detection cycle errors >=  the number of error corrections. */

/* SEA_TRNG_STA */
#define SEA_TRNG_STA_ERRNUM                  (BITS(0,4))                                         /*!< TRNG Status */

/* SEA_TRNG_BUF */
#define SEA_TRNG_BUF_BUF                     (BITS(0,31))                                        /*!< TRNG Buffer */

/* constants definitions */
/* SEA_START_EN */
#define SEA_START_EN_POS                     (0U)                                                /*!< SEA_START_EN bit offset */

/* SEA_CTRL_IR */
#define SEA_CTRL_IR_POS                      (0U)                                                /*!< SEA_CTRL_IR bit offset */
#define SEA_CTRL_IR_CFG(regval)              (SEA_CTRL_IR & ((uint32_t)(regval) << SEA_CTRL_IR_POS))
#define SEA_CTRL_IR_ECC_H                    (SEA_CTRL_IR_CFG(0x01))                             /*!< Start calculating H in ECC */
#define SEA_CTRL_IR_ECC_MODMULT              (SEA_CTRL_IR_CFG(0x02))                             /*!< Start Modular Exponentiation in ECC */
#define SEA_CTRL_IR_ECC_ADD_I2M              (SEA_CTRL_IR_CFG(0x03))                             /*!< ADD I2M */
#define SEA_CTRL_IR_ECC_DBL_I2M              (SEA_CTRL_IR_CFG(0x04))                             /*!< DBL I2M */
#define SEA_CTRL_IR_ECC_POINT_ADD            (SEA_CTRL_IR_CFG(0x05))                             /*!< Point Addition */
#define SEA_CTRL_IR_ECC_POINT_DBL            (SEA_CTRL_IR_CFG(0x06))                             /*!< Point Double */
#define SEA_CTRL_IR_ECC_M2I                  (SEA_CTRL_IR_CFG(0x07))                             /*!< M2I */
#define SEA_CTRL_IR_ECC_MODINV               (SEA_CTRL_IR_CFG(0x08))                             /*!< Modular Inversion */
#define SEA_CTRL_IR_ECC_MODADD               (SEA_CTRL_IR_CFG(0x09))                             /*!< Modular Addition */
#define SEA_CTRL_IR_ECC_MODSUB               (SEA_CTRL_IR_CFG(0x0A))                             /*!< Moduar Subtraction */
#define SEA_CTRL_IR_RSA_H                    (SEA_CTRL_IR_CFG(0x11))                             /*!< Get H */
#define SEA_CTRL_IR_RSA_MODMULT              (SEA_CTRL_IR_CFG(0x12))                             /*!< Modular Multiplication */
#define SEA_CTRL_IR_RSA_MODEXP               (SEA_CTRL_IR_CFG(0x13))                             /*!< Modular Exponentiation */
#define SEA_CTRL_IR_J0                       (SEA_CTRL_IR_CFG(0x20))                             /*!< Get J0 */
#define SEA_CTRL_IR_AES_KEYEXP               (SEA_CTRL_IR_CFG(0x40))                             /*!< AES expend key */
#define SEA_CTRL_IR_AES_ENC                  (SEA_CTRL_IR_CFG(0x42))                             /*!< AES encryption */
#define SEA_CTRL_IR_AES_DEC                  (SEA_CTRL_IR_CFG(0x43))                             /*!< AES decryption */
#define SEA_CTRL_IR_AES_GF128                (SEA_CTRL_IR_CFG(0x80))                             /*!< AES GF128 operation */
#define SEA_CTRL_IR_SHA0                     (SEA_CTRL_IR_CFG(0x90))                             /*!< SHA1/SHA224/256 */
#define SEA_CTRL_IR_SHA1                     (SEA_CTRL_IR_CFG(0x91))                             /*!< SHA384/SHA512 */

/* SEA_CTRL_AESLEN */
#define SEA_CTRL_AESLEN_POS                  (16U)                                               /*!< SEA_CTRL_AESLEN bit offset */
#define SEA_CTRL_AESLEN_CFG(regval)          (SEA_CTRL_AESLEN & ((uint32_t)(regval) << SEA_CTRL_AESLEN_POS))
#define SEA_CTRL_AESLEN_KELN128              (SEA_CTRL_AESLEN_CFG(0x0))                          /*!< 128-bit key. */
#define SEA_CTRL_AESLEN_KLEN192              (SEA_CTRL_AESLEN_CFG(0x1))                          /*!< 192-bit key. */
#define SEA_CTRL_AESLEN_KLEN256              (SEA_CTRL_AESLEN_CFG(0x2))                          /*!< 256-bit key. */

/* SEA_CTRL_AESMOD */
#define SEA_CTRL_AESMOD_POS                  (18U)                                               /*!< SEA_CTRL_AESMOD bit offset */
#define SEA_CTRL_AESMOD_CFG(regval)          (SEA_CTRL_AESMOD & ((uint32_t)(regval) << SEA_CTRL_AESMOD_POS))
#define SEA_CTRL_AESMOD_ECB                  (SEA_CTRL_AESMOD_CFG(0x0))                          /*!< AES ECB mode. */
#define SEA_CTRL_AESMOD_CBC                  (SEA_CTRL_AESMOD_CFG(0x1))                          /*!< AES CBC mode. */
#define SEA_CTRL_AESMOD_CTR                  (SEA_CTRL_AESMOD_CFG(0x2))                          /*!< AES CTR mode. */
#define SEA_CTRL_AESMOD_CFB128               (SEA_CTRL_AESMOD_CFG(0x3))                          /*!< AES CFB128 mode. */
#define SEA_CTRL_AESMOD_OFB                  (SEA_CTRL_AESMOD_CFG(0x4))                          /*!< AES OFB mode. */

/* SEA_CTRL_SHAMOD */
#define SEA_CTRL_SHAMOD_POS                  (24U)                                               /*!< SEA_CTRL_SHAMOD bit offset */
#define SEA_CTRL_SHAMOD_CFG(regval)          (SEA_CTRL_SHAMOD & ((uint32_t)(regval) << SEA_CTRL_SHAMOD_POS))
#define SEA_CTRL_SHAMOD_SHA1                 (SEA_CTRL_SHAMOD_CFG(0x0))                          /*!< SHA1 mode. */
#define SEA_CTRL_SHAMOD_SHA224               (SEA_CTRL_SHAMOD_CFG(0x1))                          /*!< SHA224 mode. */
#define SEA_CTRL_SHAMOD_SHA256               (SEA_CTRL_SHAMOD_CFG(0x2))                          /*!< SHA256 mode. */
#define SEA_CTRL_SHAMOD_SHA384               (SEA_CTRL_SHAMOD_CFG(0x4))                          /*!< SHA384 mode. */
#define SEA_CTRL_SHAMOD_SHA512               (SEA_CTRL_SHAMOD_CFG(0x5))                          /*!< SHA512 mode. */

/* SEA_CTRL_SHADMA */
#define SEA_CTRL_SHADMA_POS                  (27U)                                               /*!< SEA_CTRL_SHADMA bit offset */

/* SEA_CTRL_SHADMAREV */
#define SEA_CTRL_SHADMAREV_POS               (28U)                                               /*!< SEA_CTRL_SHADMAREV bit offset */

/* SEA_N0LOW_VAL */
#define SEA_N0LOW_VAL_POS                    (0U)                                                /*!< SEA_N0LOW_VAL bit offset */

/* SEA_N0HIGH_VAL */
#define SEA_N0HIGH_VAL_POS                   (0U)                                                /*!< SEA_N0HIGH_VAL bit offset */

/* SEA_NLEN_LEN */
#define SEA_NLEN_LEN_POS                     (0U)                                                /*!< SEA_NLEN_LEN bit offset */

/* SEA_ELEN_LEN */
#define SEA_ELEN_LEN_POS                     (0U)                                                /*!< SEA_ELEN_LEN bit offset */

/* SEA_IE_PKE */
#define SEA_IE_PKE_POS                       (2U)                                                /*!< SEA_IE_PKE bit offset */

/* SEA_IE_AES */
#define SEA_IE_AES_POS                       (3U)                                                /*!< SEA_IE_AES bit offset */

/* SEA_IE_GHASH */
#define SEA_IE_GHASH_POS                     (4U)                                                /*!< SEA_IE_GHASH bit offset */

/* SEA_IE_HASH */
#define SEA_IE_HASH_POS                      (5U)                                                /*!< SEA_IE_HASH bit offset */

/* SEA_IE_TRNGRDY */
#define SEA_IE_TRNGRDY_POS                   (6U)                                                /*!< SEA_IE_TRNGRDY bit offset */

/* SEA_IE_TRNGERR */
#define SEA_IE_TRNGERR_POS                   (7U)                                                /*!< SEA_IE_TRNGERR bit offset */

/* SEA_IE_KSGENERR */
#define SEA_IE_KSGENERR_POS                  (8U)                                                /*!< SEA_IE_KSGENERR bit offset */

/* SEA_IE_KSRDERR */
#define SEA_IE_KSRDERR_POS                   (9U)                                                /*!< SEA_IE_KSRDERR bit offset */

/* SEA_STA_DONE */
#define SEA_STA_DONE_POS                     (0U)                                                /*!< SEA_STA_DONE bit offset */

/* SEA_STA_MODINVRDY */
#define SEA_STA_MODINVRDY_POS                (1U)                                                /*!< SEA_STA_MODINVRDY bit offset */

/* SEA_STA_PKEDONE */
#define SEA_STA_PKEDONE_POS                  (2U)                                                /*!< SEA_STA_PKEDONE bit offset */

/* SEA_STA_AESDONE */
#define SEA_STA_AESDONE_POS                  (3U)                                                /*!< SEA_STA_AESDONE bit offset */

/* SEA_STA_GHASHDONE */
#define SEA_STA_GHASHDONE_POS                (4U)                                                /*!< SEA_STA_GHASHDONE bit offset */

/* SEA_STA_HASHDONE */
#define SEA_STA_HASHDONE_POS                 (5U)                                                /*!< SEA_STA_HASHDONE bit offset */

/* SEA_STA_TRNGRDY */
#define SEA_STA_TRNGRDY_POS                  (6U)                                                /*!< SEA_STA_TRNGRDY bit offset */

/* SEA_STA_TRNGERR */
#define SEA_STA_TRNGERR_POS                  (7U)                                                /*!< SEA_STA_TRNGERR bit offset */

/* SEA_STA_KSGENERR */
#define SEA_STA_KSGENERR_POS                 (8U)                                                /*!< SEA_STA_KSGENERR bit offset */

/* SEA_STA_KSRDERR */
#define SEA_STA_KSRDERR_POS                  (9U)                                                /*!< SEA_STA_KSRDERR bit offset */

/* SEA_STA_SEABUSY */
#define SEA_STA_SEABUSY_POS                  (10U)                                               /*!< SEA_STA_SEABUSY bit offset */

/* SEA_STA_KSBUSY */
#define SEA_STA_KSBUSY_POS                   (11U)                                               /*!< SEA_STA_KSBUSY bit offset */

/* SEA_GHASH_X_X */
#define SEA_GHASH_X_X_POS                    (0U)                                                /*!< SEA_GHASH_X_X bit offset */

/* SEA_GHASH_Y_Y */
#define SEA_GHASH_Y_Y_POS                    (0U)                                                /*!< SEA_GHASH_Y_Y bit offset */

/* SEA_GHASH_Z_Z */
#define SEA_GHASH_Z_Z_POS                    (0U)                                                /*!< SEA_GHASH_Z_Z bit offset */

/* SEA_IV_IV */
#define SEA_IV_IV_POS                        (0U)                                                /*!< SEA_IV_IV bit offset */

/* SEA_AES_KEY_CTRL_SECMODE */
#define SEA_AES_KEY_CTRL_SECMODE_POS         (0U)                                                /*!< SEA_AES_KEY_CTRL_SECMODE bit offset */

/* SEA_AES_KEY_CTRL_LAST */
#define SEA_AES_KEY_CTRL_LAST_POS            (1U)                                                /*!< SEA_AES_KEY_CTRL_LAST bit offset */

/* SEA_AES_KEY_CTRL_IDX */
#define SEA_AES_KEY_CTRL_IDX_POS             (2U)                                                /*!< SEA_AES_KEY_CTRL_IDX bit offset */

/* SEA_ECC_KEY_CTRL_SECMODE */
#define SEA_ECC_KEY_CTRL_SECMODE_POS         (0U)                                                /*!< SEA_ECC_KEY_CTRL_SECMODE bit offset */

/* SEA_ECC_KEY_CTRL_IDX */
#define SEA_ECC_KEY_CTRL_IDX_POS             (1U)                                                /*!< SEA_ECC_KEY_CTRL_IDX bit offset */

/* SEA_HASH_DMA_IDATA_DATA */
#define SEA_HASH_DMA_IDATA_DATA_POS          (0U)                                                /*!< SEA_HASH_DMA_IDATA_DATA bit offset */

/* SEA_HASH_DMA_STA_CNT */
#define SEA_HASH_DMA_STA_CNT_POS             (0U)                                                /*!< SEA_HASH_DMA_STA_CNT bit offset */

/* SEA_TRNG_KEY_CTRL_SECMODE */
#define SEA_TRNG_KEY_CTRL_SECMODE_POS        (0U)                                                /*!< SEA_TRNG_KEY_CTRL_SECMODE bit offset */

/* SEA_TRNG_KEY_CTRL_ERRCNT */
#define SEA_TRNG_KEY_CTRL_ERRCNT_POS         (1U)                                                /*!< SEA_TRNG_KEY_CTRL_ERRCNT bit offset */

/* SEA_TRNG_KEY_CTRL_READABLE */
#define SEA_TRNG_KEY_CTRL_READABLE_POS       (2U)                                                /*!< SEA_TRNG_KEY_CTRL_READABLE bit offset */

/* SEA_TRNG_KEY_CTRL_LEN */
#define SEA_TRNG_KEY_CTRL_LEN_POS            (3U)                                                /*!< SEA_TRNG_KEY_CTRL_LEN bit offset */

/* SEA_TRNG_KEY_CTRL_IDX */
#define SEA_TRNG_KEY_CTRL_IDX_POS            (7U)                                                /*!< SEA_TRNG_KEY_CTRL_IDX bit offset */

/* SEA_TRNG_KEY_CTRL_ENC */
#define SEA_TRNG_KEY_CTRL_ENC_POS            (14U)                                               /*!< SEA_TRNG_KEY_CTRL_ENC bit offset */

/* SEA_TRNG_KEY_CTRL_ENCIDX */
#define SEA_TRNG_KEY_CTRL_ENCIDX_POS         (16U)                                               /*!< SEA_TRNG_KEY_CTRL_ENCIDX bit offset */

/* SEA_TRNG_CTRL0_EN */
#define SEA_TRNG_CTRL0_EN_POS                (0U)                                                /*!< SEA_TRNG_CTRL0_EN bit offset */

/* SEA_TRNG_CTRL0_BYPASSCRC */
#define SEA_TRNG_CTRL0_BYPASSCRC_POS         (1U)                                                /*!< SEA_TRNG_CTRL0_BYPASSCRC bit offset */

/* SEA_TRNG_CTRL0_DAOSEL */
#define SEA_TRNG_CTRL0_DAOSEL_POS            (2U)                                                /*!< SEA_TRNG_CTRL0_DAOSEL bit offset */

/* SEA_TRNG_CTRL0_BYPASSCRT */
#define SEA_TRNG_CTRL0_BYPASSCRT_POS         (3U)                                                /*!< SEA_TRNG_CTRL0_BYPASSCRT bit offset */

/* SEA_TRNG_CTRL0_LOOP0 */
#define SEA_TRNG_CTRL0_LOOP0_POS             (4U)                                                /*!< SEA_TRNG_CTRL0_LOOP0 bit offset */

/* SEA_TRNG_CTRL0_LOOP1 */
#define SEA_TRNG_CTRL0_LOOP1_POS             (5U)                                                /*!< SEA_TRNG_CTRL0_LOOP1 bit offset */

/* SEA_TRNG_CTRL0_LOOP2 */
#define SEA_TRNG_CTRL0_LOOP2_POS             (6U)                                                /*!< SEA_TRNG_CTRL0_LOOP2 bit offset */

/* SEA_TRNG_CTRL0_LOOP3 */
#define SEA_TRNG_CTRL0_LOOP3_POS             (7U)                                                /*!< SEA_TRNG_CTRL0_LOOP3 bit offset */

/* SEA_TRNG_CTRL0_LOOP4 */
#define SEA_TRNG_CTRL0_LOOP4_POS             (8U)                                                /*!< SEA_TRNG_CTRL0_LOOP4 bit offset */

/* SEA_TRNG_CTRL0_LOOP5 */
#define SEA_TRNG_CTRL0_LOOP5_POS             (9U)                                                /*!< SEA_TRNG_CTRL0_LOOP5 bit offset */

/* SEA_TRNG_CTRL0_LOOP6 */
#define SEA_TRNG_CTRL0_LOOP6_POS             (10U)                                               /*!< SEA_TRNG_CTRL0_LOOP6 bit offset */

/* SEA_TRNG_CTRL0_LOOP7 */
#define SEA_TRNG_CTRL0_LOOP7_POS             (11U)                                               /*!< SEA_TRNG_CTRL0_LOOP7 bit offset */

/* SEA_TRNG_CTRL0_CAPTURENUM */
#define SEA_TRNG_CTRL0_CAPTURENUM_POS        (16U)                                               /*!< SEA_TRNG_CTRL0_CAPTURENUM bit offset */

/* SEA_TRNG_CTRL1_ADDR0SEL */
#define SEA_TRNG_CTRL1_ADDR0SEL_POS          (0U)                                                /*!< SEA_TRNG_CTRL1_ADDR0SEL bit offset */

/* SEA_TRNG_CTRL1_ADDR1SEL */
#define SEA_TRNG_CTRL1_ADDR1SEL_POS          (4U)                                                /*!< SEA_TRNG_CTRL1_ADDR1SEL bit offset */

/* SEA_TRNG_CTRL1_ADDR2SEL */
#define SEA_TRNG_CTRL1_ADDR2SEL_POS          (8U)                                                /*!< SEA_TRNG_CTRL1_ADDR2SEL bit offset */

/* SEA_TRNG_CTRL1_ADDR3SEL */
#define SEA_TRNG_CTRL1_ADDR3SEL_POS          (12U)                                               /*!< SEA_TRNG_CTRL1_ADDR3SEL bit offset */

/* SEA_TRNG_CTRL1_ADDR4SEL */
#define SEA_TRNG_CTRL1_ADDR4SEL_POS          (16U)                                               /*!< SEA_TRNG_CTRL1_ADDR4SEL bit offset */

/* SEA_TRNG_CTRL1_ADDR5SEL */
#define SEA_TRNG_CTRL1_ADDR5SEL_POS          (20U)                                               /*!< SEA_TRNG_CTRL1_ADDR5SEL bit offset */

/* SEA_TRNG_CTRL1_ADDR6SEL */
#define SEA_TRNG_CTRL1_ADDR6SEL_POS          (24U)                                               /*!< SEA_TRNG_CTRL1_ADDR6SEL bit offset */

/* SEA_TRNG_CTRL2_RPTERRTHD */
#define SEA_TRNG_CTRL2_RPTERRTHD_POS         (0U)                                                /*!< SEA_TRNG_CTRL2_RPTERRTHD bit offset */

/* SEA_TRNG_CTRL2_FIXTHD */
#define SEA_TRNG_CTRL2_FIXTHD_POS            (5U)                                                /*!< SEA_TRNG_CTRL2_FIXTHD bit offset */

/* SEA_TRNG_CTRL2_AUTOFIXTHD */
#define SEA_TRNG_CTRL2_AUTOFIXTHD_POS        (10U)                                               /*!< SEA_TRNG_CTRL2_AUTOFIXTHD bit offset */

/* SEA_TRNG_STA_ERRNUM */
#define SEA_TRNG_STA_ERRNUM_POS              (0U)                                                /*!< SEA_TRNG_STA_ERRNUM bit offset */

/* SEA_TRNG_BUF_BUF */
#define SEA_TRNG_BUF_BUF_POS                 (0U)                                                /*!< SEA_TRNG_BUF_BUF bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_SEA_H */
