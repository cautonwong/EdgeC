/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_keystore.h
 * @author  Renergy Software Team
 * @brief   KEYSTORE module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_KEYSTORE_H
#define _RN831x_RN861x_MCU_KEYSTORE_H

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

/* KEYSTORE */
typedef struct
{
    __IO uint32_t CTRL;           /*!< offset: 0x000       info: KeyStore Control Register. */
    __IO uint32_t KP;             /*!< offset: 0x004       info: KeyStore Key Parameter Register. */
    __IO uint32_t ENC;            /*!< offset: 0x008       info: KeyStore Encrypt Parameter Register. */
    __IO uint32_t IE;             /*!< offset: 0x00C       info: KeyStore Interrupt Enable Register. */
    __IO uint32_t STA;            /*!< offset: 0x010       info: KeyStore Status Register. */
    __IO uint32_t START;          /*!< offset: 0x014       info: KeyStore Start Register. */
    __IO uint32_t KBP;            /*!< offset: 0x018       info: KeyStore Key Block Register. */
    __IO uint32_t RESERVED0[5];   /*!< offset: 0x01C ~ 02C info: Reserved. */
    __IO uint32_t KEY[18];        /*!< offset: 0x030 ~ 074 info: KeyStore Key Storage Registers. */
} KEYSTORE_TypeDef;

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
#define KEYSTORE_CORE0_BASE (0x3000A000U)
#define KEYSTORE_CORE1_BASE (0xB000A000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define KEYSTORE_CORE0 ((KEYSTORE_TypeDef *) KEYSTORE_CORE0_BASE)
#define KEYSTORE_CORE1 ((KEYSTORE_TypeDef *) KEYSTORE_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define KEYSTORE  KEYSTORE_CORE0
#endif
#if defined(CORE1)
#define KEYSTORE  KEYSTORE_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** KEYSTORE Instances *******************************/
#define IS_KEYSTORE_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == KEYSTORE_CORE0) || \
                                               ((INSTANCE) == KEYSTORE_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* KEYSTORE_CTRL */
#define KEYSTORE_CTRL_OPMODE                 (BITS(0,2))                                         /*!< Set KeyStore different operation mode */

/* KEYSTORE_KP */
#define KEYSTORE_KP_ENC                      (BIT(0))                                            /*!< Enable encryption in creating key. */
#define KEYSTORE_KP_READABLE                 (BIT(1))                                            /*!< Set whether the key is readable. */
#define KEYSTORE_KP_SIZE                     (BITS(2,5))                                         /*!< Set the key length. */
#define KEYSTORE_KP_IDX                      (BITS(6,12))                                        /*!< Set the creating key index in KeyStore. */

/* KEYSTORE_ENC */
#define KEYSTORE_ENC_IDX                     (BITS(0,6))                                         /*!< The index of key due to encrypt */

/* KEYSTORE_IE */
#define KEYSTORE_IE_DONE                     (BIT(0))                                            /*!< Enable KeyStore operation complete interrupt */
#define KEYSTORE_IE_OPERR                    (BIT(1))                                            /*!< Enable KeyStore operation error interrupt */
#define KEYSTORE_IE_CSERR                    (BIT(2))                                            /*!< Enable KeyStore operation verify error interrupt */
#define KEYSTORE_IE_RDERR                    (BIT(3))                                            /*!< Enable KeyStore operation read error interrupt */

/* KEYSTORE_STA */
#define KEYSTORE_STA_DONE                    (BIT(0))                                            /*!< KeyStore operation complete interrupt flag. */
#define KEYSTORE_STA_OPERR                   (BIT(1))                                            /*!< KeyStore operation error interrupt flag. */
#define KEYSTORE_STA_CSERR                   (BIT(2))                                            /*!< KeyStore verify error interrupt flag. */
#define KEYSTORE_STA_RDERR                   (BIT(3))                                            /*!< KeyStore read error interrupt flag. */
#define KEYSTORE_STA_KSBUSY                  (BIT(4))                                            /*!< KeyStore busy flag. */
#define KEYSTORE_STA_SEABUSY                 (BIT(5))                                            /*!< SEA busy flag. */

/* KEYSTORE_START */
#define KEYSTORE_START_EN                    (BIT(0))                                            /*!< Enable KeyStore running */

/* KEYSTORE_KBP */
#define KEYSTORE_KBP_PROG                    (BIT(0))                                            /*!< Whether the key slot is programmed. */
#define KEYSTORE_KBP_RVK                     (BIT(1))                                            /*!< Whether the key slot is revoked. */
#define KEYSTORE_KBP_DEC                     (BIT(2))                                            /*!< Whether the key slot is encrypted. */
#define KEYSTORE_KBP_READABLE                (BIT(3))                                            /*!< Whether the key slot is readable to CPU. */
#define KEYSTORE_KBP_SIZE                    (BITS(4,7))                                         /*!< The key length in bits. */

/* KEYSTORE_KEY */
#define KEYSTORE_KEY_BUF                     (BITS(0,31))                                        /*!< Key Buffer 0-17 */

/* constants definitions */
/* KEYSTORE_CTRL_OPMODE */
#define KEYSTORE_CTRL_OPMODE_POS             (0U)                                                /*!< KEYSTORE_CTRL_OPMODE bit offset */

/* KEYSTORE_KP_ENC */
#define KEYSTORE_KP_ENC_POS                  (0U)                                                /*!< KEYSTORE_KP_ENC bit offset */

/* KEYSTORE_KP_READABLE */
#define KEYSTORE_KP_READABLE_POS             (1U)                                                /*!< KEYSTORE_KP_READABLE bit offset */

/* KEYSTORE_KP_SIZE */
#define KEYSTORE_KP_SIZE_POS                 (2U)                                                /*!< KEYSTORE_KP_SIZE bit offset */
#define KEYSTORE_KP_SIZE_CFG(regval)         (KEYSTORE_KP_SIZE & ((uint32_t)(regval) << KEYSTORE_KP_SIZE_POS))
#define KEYSTORE_KP_SIZE_KLEN128             (KEYSTORE_KP_SIZE_CFG(0x0))                         /*!< 128-bit key. */
#define KEYSTORE_KP_SIZE_KLEN160             (KEYSTORE_KP_SIZE_CFG(0x1))                         /*!< 160-bit key. */
#define KEYSTORE_KP_SIZE_KLEN192             (KEYSTORE_KP_SIZE_CFG(0x2))                         /*!< 192-bit key. */
#define KEYSTORE_KP_SIZE_KLEN224             (KEYSTORE_KP_SIZE_CFG(0x3))                         /*!< 224-bit key. */
#define KEYSTORE_KP_SIZE_KLEN256             (KEYSTORE_KP_SIZE_CFG(0x4))                         /*!< 256-bit key. */
#define KEYSTORE_KP_SIZE_KLEN384             (KEYSTORE_KP_SIZE_CFG(0x5))                         /*!< 384-bit key. */
#define KEYSTORE_KP_SIZE_KLEN512             (KEYSTORE_KP_SIZE_CFG(0x6))                         /*!< 512-bit key. */
#define KEYSTORE_KP_SIZE_KLEN521             (KEYSTORE_KP_SIZE_CFG(0x7))                         /*!< 521-bit key. */
#define KEYSTORE_KP_SIZE_KLEN576             (KEYSTORE_KP_SIZE_CFG(0x8))                         /*!< 576-bit key. */

/* KEYSTORE_KP_IDX */
#define KEYSTORE_KP_IDX_POS                  (6U)                                                /*!< KEYSTORE_KP_IDX bit offset */

/* KEYSTORE_ENC_IDX */
#define KEYSTORE_ENC_IDX_POS                 (0U)                                                /*!< KEYSTORE_ENC_IDX bit offset */

/* KEYSTORE_IE_DONE */
#define KEYSTORE_IE_DONE_POS                 (0U)                                                /*!< KEYSTORE_IE_DONE bit offset */

/* KEYSTORE_IE_OPERR */
#define KEYSTORE_IE_OPERR_POS                (1U)                                                /*!< KEYSTORE_IE_OPERR bit offset */

/* KEYSTORE_IE_CSERR */
#define KEYSTORE_IE_CSERR_POS                (2U)                                                /*!< KEYSTORE_IE_CSERR bit offset */

/* KEYSTORE_IE_RDERR */
#define KEYSTORE_IE_RDERR_POS                (3U)                                                /*!< KEYSTORE_IE_RDERR bit offset */

/* KEYSTORE_STA_DONE */
#define KEYSTORE_STA_DONE_POS                (0U)                                                /*!< KEYSTORE_STA_DONE bit offset */

/* KEYSTORE_STA_OPERR */
#define KEYSTORE_STA_OPERR_POS               (1U)                                                /*!< KEYSTORE_STA_OPERR bit offset */

/* KEYSTORE_STA_CSERR */
#define KEYSTORE_STA_CSERR_POS               (2U)                                                /*!< KEYSTORE_STA_CSERR bit offset */

/* KEYSTORE_STA_RDERR */
#define KEYSTORE_STA_RDERR_POS               (3U)                                                /*!< KEYSTORE_STA_RDERR bit offset */

/* KEYSTORE_STA_KSBUSY */
#define KEYSTORE_STA_KSBUSY_POS              (4U)                                                /*!< KEYSTORE_STA_KSBUSY bit offset */

/* KEYSTORE_STA_SEABUSY */
#define KEYSTORE_STA_SEABUSY_POS             (5U)                                                /*!< KEYSTORE_STA_SEABUSY bit offset */

/* KEYSTORE_START_EN */
#define KEYSTORE_START_EN_POS                (0U)                                                /*!< KEYSTORE_START_EN bit offset */

/* KEYSTORE_KBP_PROG */
#define KEYSTORE_KBP_PROG_POS                (0U)                                                /*!< KEYSTORE_KBP_PROG bit offset */

/* KEYSTORE_KBP_RVK */
#define KEYSTORE_KBP_RVK_POS                 (1U)                                                /*!< KEYSTORE_KBP_RVK bit offset */

/* KEYSTORE_KBP_DEC */
#define KEYSTORE_KBP_DEC_POS                 (2U)                                                /*!< KEYSTORE_KBP_DEC bit offset */

/* KEYSTORE_KBP_READABLE */
#define KEYSTORE_KBP_READABLE_POS            (3U)                                                /*!< KEYSTORE_KBP_READABLE bit offset */

/* KEYSTORE_KBP_SIZE */
#define KEYSTORE_KBP_SIZE_POS                (4U)                                                /*!< KEYSTORE_KBP_SIZE bit offset */
#define KEYSTORE_KBP_SIZE_CFG(regval)        (KEYSTORE_KBP_SIZE & ((uint32_t)(regval) << KEYSTORE_KBP_SIZE_POS))
#define KEYSTORE_KBP_SIZE_KLEN128            (KEYSTORE_KBP_SIZE_CFG(0x0))                        /*!< 128-bit key. */
#define KEYSTORE_KBP_SIZE_KLEN160            (KEYSTORE_KBP_SIZE_CFG(0x1))                        /*!< 160-bit key. */
#define KEYSTORE_KBP_SIZE_KLEN192            (KEYSTORE_KBP_SIZE_CFG(0x2))                        /*!< 192-bit key. */
#define KEYSTORE_KBP_SIZE_KLEN224            (KEYSTORE_KBP_SIZE_CFG(0x3))                        /*!< 224-bit key. */
#define KEYSTORE_KBP_SIZE_KLEN256            (KEYSTORE_KBP_SIZE_CFG(0x4))                        /*!< 256-bit key. */
#define KEYSTORE_KBP_SIZE_KLEN384            (KEYSTORE_KBP_SIZE_CFG(0x5))                        /*!< 384-bit key. */
#define KEYSTORE_KBP_SIZE_KLEN512            (KEYSTORE_KBP_SIZE_CFG(0x6))                        /*!< 512-bit key. */
#define KEYSTORE_KBP_SIZE_KLEN521            (KEYSTORE_KBP_SIZE_CFG(0x7))                        /*!< 521-bit key. */
#define KEYSTORE_KBP_SIZE_KLEN576            (KEYSTORE_KBP_SIZE_CFG(0x8))                        /*!< 576-bit key. */

/* KEYSTORE_KEY_BUF */
#define KEYSTORE_KEY_BUF_POS                 (0U)                                                /*!< KEYSTORE_KEY_BUF bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_KEYSTORE_H */
