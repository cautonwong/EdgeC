/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_crc.h
 * @author  Renergy Software Team
 * @brief   CRC module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_CRC_H
#define _RN831x_RN861x_MCU_CRC_H

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

/* CRC */
typedef struct
{
    __IO uint32_t DATA;                      /*!< offset: 0x000 info: CRC Data Register. */
    __IO uint32_t STA;                       /*!< offset: 0x004 info: CRC Status Register. */
    __IO uint32_t CTRL;                      /*!< offset: 0x008 info: CRC Control Register. */
    __IO uint32_t INIT;                      /*!< offset: 0x00C info: CRC Init Register. */
    __IO uint32_t POL;                       /*!< offset: 0x010 info: CRC Polynomial Register. */
    __IO uint32_t XOR;                       /*!< offset: 0x014 info: CRC XOR Register. */
    __IO uint32_t DMA_CTRL;                  /*!< offset: 0x018 info: CRC DMA Control Register. */
} CRC_TypeDef;

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
#define CRC0_CORE0_BASE (0x20070000U)
#define CRC0_CORE1_BASE (0xA0070000U)
#define CRC1_BASE       (0x20114000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define CRC0_CORE0 ((CRC_TypeDef *)      CRC0_CORE0_BASE)
#define CRC0_CORE1 ((CRC_TypeDef *)      CRC0_CORE1_BASE)
#define CRC1       ((CRC_TypeDef *)      CRC1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define CRC0  CRC0_CORE0
#endif
#if defined(CORE1)
#define CRC0  CRC0_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** CRC Instances *******************************/
#define IS_CRC_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == CRC0_CORE0) || \
                                          ((INSTANCE) == CRC0_CORE1) || \
                                          ((INSTANCE) == CRC1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* CRC_DATA */
#define CRC_DATA_DATA                        (BITS(0,31))                                        /*!< CRC data input */

/* CRC_STA */
#define CRC_STA_DONE                         (BIT(0))                                            /*!< CRC status */

/* CRC_CTRL */
#define CRC_CTRL_POLYSIZE                    (BITS(0,1))                                         /*!< Size of polynomial value. */
#define CRC_CTRL_XOR                         (BIT(2))                                            /*!< Enable CRC XOR. */
#define CRC_CTRL_REVOUT                      (BIT(3))                                            /*!< Reverse output data. */
#define CRC_CTRL_REVIN                       (BITS(4,5))                                         /*!< Reverse input data. */
#define CRC_CTRL_OPDW                        (BITS(6,7))                                         /*!< Configure the data width of CRC operation. */

/* CRC_INIT */
#define CRC_INIT_VAL                         (BITS(0,31))                                        /*!< CRC init value */

/* CRC_POL */
#define CRC_POL_VAL                          (BITS(0,31))                                        /*!< CRC polynomial value */

/* CRC_XOR */
#define CRC_XOR_VAL                          (BITS(0,31))                                        /*!< CRC XOR value */

/* CRC_DMA_CTRL */
#define CRC_DMA_CTRL_EN                      (BIT(0))                                            /*!< Enable CRC DMA */
#define CRC_DMA_CTRL_MODE                    (BIT(1))                                            /*!< Set CRC DMA calculation mode */

/* constants definitions */
/* CRC_DATA_DATA */
#define CRC_DATA_DATA_POS                    (0U)                                                /*!< CRC_DATA_DATA bit offset */

/* CRC_STA_DONE */
#define CRC_STA_DONE_POS                     (0U)                                                /*!< CRC_STA_DONE bit offset */

/* CRC_CTRL_POLYSIZE */
#define CRC_CTRL_POLYSIZE_POS                (0U)                                                /*!< CRC_CTRL_POLYSIZE bit offset */
#define CRC_CTRL_POLYSIZE_CFG(regval)  (CRC_CTRL_POLYSIZE & ((uint32_t)(regval) << CRC_CTRL_POLYSIZE_POS))
#define CRC_CTRL_POLYSIZE_CRC32              (CRC_CTRL_POLYSIZE_CFG(0x0))                        /*!< 32-bit polynomial. */
#define CRC_CTRL_POLYSIZE_CRC16              (CRC_CTRL_POLYSIZE_CFG(0x1))                        /*!< 16-bit polynomial. */
#define CRC_CTRL_POLYSIZE_CRC8               (CRC_CTRL_POLYSIZE_CFG(0x2))                        /*!< 8-bit polynomial. */
#define CRC_CTRL_POLYSIZE_CRC7               (CRC_CTRL_POLYSIZE_CFG(0x3))                        /*!< 7-bit polynomial. */

/* CRC_CTRL_XOR */
#define CRC_CTRL_XOR_POS                     (2U)                                                /*!< CRC_CTRL_XOR bit offset */

/* CRC_CTRL_REVOUT */
#define CRC_CTRL_REVOUT_POS                  (3U)                                                /*!< CRC_CTRL_REVOUT bit offset */

/* CRC_CTRL_REVIN */
#define CRC_CTRL_REVIN_POS                   (4U)                                                /*!< CRC_CTRL_REVIN bit offset */
#define CRC_CTRL_REVIN_CFG(regval)  (CRC_CTRL_REVIN & ((uint32_t)(regval) << CRC_CTRL_REVIN_POS))
#define CRC_CTRL_REVIN_DEFAULT               (CRC_CTRL_REVIN_CFG(0x0))                           /*!< No reverse. */
#define CRC_CTRL_REVIN_BYTE                  (CRC_CTRL_REVIN_CFG(0x1))                           /*!< Reverse data in byte. */
#define CRC_CTRL_REVIN_HWORD                 (CRC_CTRL_REVIN_CFG(0x2))                           /*!< Reverse data in half-word. */
#define CRC_CTRL_REVIN_WORD                  (CRC_CTRL_REVIN_CFG(0x3))                           /*!< Reverse data in word. */

/* CRC_CTRL_OPDW */
#define CRC_CTRL_OPDW_POS                    (6U)                                                /*!< CRC_CTRL_OPDW bit offset */
#define CRC_CTRL_OPDW_CFG(regval)  (CRC_CTRL_OPDW & ((uint32_t)(regval) << CRC_CTRL_OPDW_POS))
#define CRC_CTRL_OPDW_WORD                   (CRC_CTRL_OPDW_CFG(0x0))                            /*!< Set the data width to word. */
#define CRC_CTRL_OPDW_HWORD                  (CRC_CTRL_OPDW_CFG(0x1))                            /*!< Set the data width to half-word. */
#define CRC_CTRL_OPDW_BYTE                   (CRC_CTRL_OPDW_CFG(0x2))                            /*!< Set the data width to byte. */

/* CRC_INIT_VAL */
#define CRC_INIT_VAL_POS                     (0U)                                                /*!< CRC_INIT_VAL bit offset */

/* CRC_POL_VAL */
#define CRC_POL_VAL_POS                      (0U)                                                /*!< CRC_POL_VAL bit offset */

/* CRC_XOR_VAL */
#define CRC_XOR_VAL_POS                      (0U)                                                /*!< CRC_XOR_VAL bit offset */

/* CRC_DMA_CTRL_EN */
#define CRC_DMA_CTRL_EN_POS                  (0U)                                                /*!< CRC_DMA_CTRL_EN bit offset */

/* CRC_DMA_CTRL_MODE */
#define CRC_DMA_CTRL_MODE_POS                (1U)                                                /*!< CRC_DMA_CTRL_MODE bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_CRC_H */
