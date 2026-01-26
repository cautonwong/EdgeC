/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_aes.h
 * @author  Renergy Software Team
 * @brief   AES module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_AES_H
#define _RN831x_RN861x_MCU_AES_H

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

/* AES */
typedef struct
{
    __IO uint32_t START;            /*!< offset: 0x000       info: AES Start Register. */
    __IO uint32_t CTRL;             /*!< offset: 0x004       info: AES Control Register. */
    __IO uint32_t IV[4];            /*!< offset: 0x008 ~ 014 info: AES IVector Register. */
    __IO uint32_t IE;               /*!< offset: 0x018       info: AES Interrupt Enable Register. */
    __IO uint32_t STA;              /*!< offset: 0x01C       info: AES Status Register. */
    __IO uint32_t ADDR;             /*!< offset: 0x020       info: AES Offset Address Configuration Register. */
    __IO uint32_t GHASH_X[4];       /*!< offset: 0x024 ~ 030 info: 128-bit GHASH Multiplicand X Register. */
    __IO uint32_t GHASH_Y[4];       /*!< offset: 0x034 ~ 040 info: 128-bit GHASH Multiplier Y Register. */
    __IO uint32_t GHASH_Z[4];       /*!< offset: 0x044 ~ 050 info: 128-bit GHASH Result Z Register. */
} AES_TypeDef;

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
#define AES_CORE0_BASE  (0x30014000U)
#define AES_CORE1_BASE  (0xB0014000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define AES_CORE0  ((AES_TypeDef *)      AES_CORE0_BASE)
#define AES_CORE1  ((AES_TypeDef *)      AES_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define AES  AES_CORE0
#endif
#if defined(CORE1)
#define AES  AES_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** AES Instances *******************************/
#define IS_AES_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == AES_CORE0) || \
                                          ((INSTANCE) == AES_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* AES_START */
#define AES_START_EN                         (BIT(0))

/* AES_CTRL */
#define AES_CTRL_IR                          (BITS(0,7))                                         /*!< Set AES calculation methods. */
#define AES_CTRL_SIZE                        (BITS(16,17))                                       /*!< Set AES key length. */
#define AES_CTRL_MODE                        (BITS(18,20))                                       /*!< Set AES data stream mode. */

/* AES_IV */
#define AES_IV_IV                            (BITS(0,31))

/* AES_IE */
#define AES_IE_AES                           (BIT(3))                                            /*!< AES calculation complete interrupt. */
#define AES_IE_GHASH                         (BIT(4))                                            /*!< GHASH calculation complete interrupt. */

/* AES_STA */
#define AES_STA_DONE                         (BIT(0))                                            /*!< SEA calculations completed flag. */
#define AES_STA_AESDONE                      (BIT(3))                                            /*!< AES calculations completed flag. */
#define AES_STA_GHASHDONE                    (BIT(4))                                            /*!< GHASH calculations completed flag. */

/* AES_ADDR */
#define AES_ADDR_ADDR                        (BITS(2,17))

/* AES_GHASH_X */
#define AES_GHASH_X_X                        (BITS(0,31))

/* AES_GHASH_Y */
#define AES_GHASH_Y_Y                        (BITS(0,31))

/* AES_GHASH_Z */
#define AES_GHASH_Z_Z                        (BITS(0,31))

/* constants definitions */
/* AES_START_EN */
#define AES_START_EN_POS                     (0U)                                                /*!< AES_START_EN bit offset */

/* AES_CTRL_IR */
#define AES_CTRL_IR_POS                      (0U)                                                /*!< AES_CTRL_IR bit offset */
#define AES_CTRL_IR_CFG(regval)              (AES_CTRL_IR & ((uint32_t)(regval) << AES_CTRL_IR_POS))
#define AES_CTRL_IR_KEYEXP                   (AES_CTRL_IR_CFG(0x40))                             /*!< AES expend key. */
#define AES_CTRL_IR_ENC                      (AES_CTRL_IR_CFG(0x42))                             /*!< AES encryption. */
#define AES_CTRL_IR_DEC                      (AES_CTRL_IR_CFG(0x43))                             /*!< AES decryption. */
#define AES_CTRL_IR_GF128                    (AES_CTRL_IR_CFG(0x80))                             /*!< GHASH algorithm. */

/* AES_CTRL_SIZE */
#define AES_CTRL_SIZE_POS                    (16U)                                               /*!< AES_CTRL_SIZE bit offset */
#define AES_CTRL_SIZE_CFG(regval)            (AES_CTRL_SIZE & ((uint32_t)(regval) << AES_CTRL_SIZE_POS))
#define AES_CTRL_SIZE_KLEN128                (AES_CTRL_SIZE_CFG(0x0))                            /*!< 128 bits AES key. */
#define AES_CTRL_SIZE_KLEN192                (AES_CTRL_SIZE_CFG(0x1))                            /*!< 192 bits AES key. */
#define AES_CTRL_SIZE_KLEN256                (AES_CTRL_SIZE_CFG(0x2))                            /*!< 256 bits AES key. */

/* AES_CTRL_MODE */
#define AES_CTRL_MODE_POS                    (18U)                                               /*!< AES_CTRL_MODE bit offset */
#define AES_CTRL_MODE_CFG(regval)            (AES_CTRL_MODE & ((uint32_t)(regval) << AES_CTRL_MODE_POS))
#define AES_CTRL_MODE_ECB                    (AES_CTRL_MODE_CFG(0x0))                            /*!< AES ECB mode. */
#define AES_CTRL_MODE_CBC                    (AES_CTRL_MODE_CFG(0x1))                            /*!< AES CBC mode. */
#define AES_CTRL_MODE_CTR                    (AES_CTRL_MODE_CFG(0x2))                            /*!< AES CTR mode. */
#define AES_CTRL_MODE_CFB128                 (AES_CTRL_MODE_CFG(0x3))                            /*!< AES CFB128 mode. */
#define AES_CTRL_MODE_OFB                    (AES_CTRL_MODE_CFG(0x4))                            /*!< AES OFB mode. */

/* AES_IV_IV */
#define AES_IV_IV_POS                        (0U)                                                /*!< AES_IV_IV bit offset */

/* AES_IE_AES */
#define AES_IE_AES_POS                       (3U)                                                /*!< AES_IE_AES bit offset */

/* AES_IE_GHASH */
#define AES_IE_GHASH_POS                     (4U)                                                /*!< AES_IE_GHASH bit offset */

/* AES_STA_DONE */
#define AES_STA_DONE_POS                     (0U)                                                /*!< AES_STA_DONE bit offset */

/* AES_STA_AESDONE */
#define AES_STA_AESDONE_POS                  (3U)                                                /*!< AES_STA_AESDONE bit offset */

/* AES_STA_GHASHDONE */
#define AES_STA_GHASHDONE_POS                (4U)                                                /*!< AES_STA_GHASHDONE bit offset */

/* AES_ADDR_ADDR */
#define AES_ADDR_ADDR_POS                    (0U)                                                /*!< AES_ADDR_ADDR bit offset */

/* AES_GHASH_X_X */
#define AES_GHASH_X_X_POS                    (0U)                                                /*!< AES_GHASH_X_X bit offset */

/* AES_GHASH_Y_Y */
#define AES_GHASH_Y_Y_POS                    (0U)                                                /*!< AES_GHASH_Y_Y bit offset */

/* AES_GHASH_Z_Z */
#define AES_GHASH_Z_Z_POS                    (0U)                                                /*!< AES_GHASH_Z_Z bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_AES_H */
