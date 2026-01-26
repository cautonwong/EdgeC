/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_dma.h
 * @author  Renergy Software Team
 * @brief   DMA module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_DMA_H
#define _RN831x_RN861x_MCU_DMA_H

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

/* DMA */
typedef struct
{
    struct
    {
        __IO uint32_t CTRL;         /*!< offset: 0x000       info: DMA Control Register. */
        __IO uint32_t NDTR;         /*!< offset: 0x004       info: DMA Data Length Register. */
        __IO uint32_t MADR;         /*!< offset: 0x008       info: DMA Memory Address Register. */
        __IO uint32_t PADR;         /*!< offset: 0x00C       info: DMA Peripheral-side Address Register. */
        __IO uint32_t RESERVED0[4]; /*!< offset: 0x010 ~ 01C info: Reserved. */
    } CH0[5]; /*<! The real address of the normal channel: [Address offset] + 20 * x(x=0,1,2,3,4). */
    __IO uint32_t RESERVED1[24];    /*!< offset: 0x0A0 ~ 0xFC info: Reserved. */
    struct
    {
        __IO uint32_t MCTRL;        /*!< offset: 0x100       info: DMA Control Register. */
        __IO uint32_t MNDTR;        /*!< offset: 0x104       info: DMA Data Length Register. */
        __IO uint32_t MDADR;        /*!< offset: 0x108       info: DMA M2M Channel Destination Register. */
        __IO uint32_t MSADR;        /*!< offset: 0x10C       info: DMA M2M Channel Source Address Register. */
        __IO uint32_t RESERVED2[4]; /*!< offset: 0x110 ~ 11C info: Reserved. */
    } CH1[2]; /*<! The real address of the M2M channel: [Address offset] + 20 * i(i=0,1). */
    __IO uint32_t RESERVED3[48];    /*!< offset: 0x140 ~ 1FC info: Reserved. */
    __IO uint32_t IE;               /*!< offset: 0x200       info: DMA Normal Channel Interrupt Enable Register. */
    __IO uint32_t IF;               /*!< offset: 0x204       info: DMA Normal Channel Interrupt Flags Register. */
    __IO uint32_t MIE;              /*!< offset: 0x208       info: DMA M2M Channel Interrupt Enable Register. */
    __IO uint32_t MIF;              /*!< offset: 0x20C       info: DMA M2M Channel Interrupt Flags Register. */
} DMA_TypeDef;

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
#define DMA00_BASE      (0x20104000U)
#define DMA01_BASE      (0x20108000U)
#define DMA10_BASE      (0xA0104000U)
#define DMA11_BASE      (0xA0108000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define DMA00      ((DMA_TypeDef *)      DMA00_BASE)
#define DMA01      ((DMA_TypeDef *)      DMA01_BASE)
#define DMA10      ((DMA_TypeDef *)      DMA10_BASE)
#define DMA11      ((DMA_TypeDef *)      DMA11_BASE)


/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** DMA Instances *******************************/
#define IS_DMA_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == DMA00) || \
                                          ((INSTANCE) == DMA01) || \
                                          ((INSTANCE) == DMA10) || \
                                          ((INSTANCE) == DMA11))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* DMA_CTRL */
#define DMA_CTRL_EN                          (BIT(0))                                            /*!< Enable DMA to start data migration. */
#define DMA_CTRL_CIRC                        (BIT(1))                                            /*!< Enable DMA circle mode. */
#define DMA_CTRL_PL                          (BITS(2,3))                                         /*!< Configure the DMA channel priority. */
#define DMA_CTRL_MSIZE                       (BITS(4,5))                                         /*!< Configure the data size of  DMA for a single transaction. */
#define DMA_CTRL_DIR                         (BIT(6))                                            /*!< Configure the DMA transmission direction. */

/* DMA_NDTR */
#define DMA_NDTR_NDTLOW                      (BITS(0,17))                                        /*!< when current channel could not access Flash. */
#define DMA_NDTR_NDTHIGH                     (BITS(18,19))                                       /*!< when current channel could access Flash. */

/* DMA_MADR */
#define DMA_MADR_ADDRLOW                     (BITS(0,17))                                        /*!< when current channel could not access Flash. */
#define DMA_MADR_ADDRHIGH                    (BITS(18,21))                                       /*!< when current channel could access Flash. */
#define DMA_MADR_ADDRVERYHIGH                (BITS(28,31))                                       /*!< default valid bit field. */

/* DMA_PADR */
#define DMA_PADR_ADDRLOW                     (BITS(0,21))                                        /*!< default valid bit field. */
#define DMA_PADR_ADDRHIGH                    (BITS(28,31))                                       /*!< default valid bit field. */

/* DMA_MCTRL */
#define DMA_MCTRL_EN                         (BIT(0))                                            /*!< Enable DMA to start data migration */
#define DMA_MCTRL_CIRC                       (BIT(1))                                            /*!< Enable DMA circle mode */
#define DMA_MCTRL_PL                         (BITS(2,3))                                         /*!< Configure the DMA channel priority */
#define DMA_MCTRL_DSIZE                      (BITS(4,5))                                         /*!< Configure the dest port data size of  DMA for a single transaction */
#define DMA_MCTRL_SSIZE                      (BITS(6,7))                                         /*!< Configure the source port data size of  DMA for a single transaction */

/* DMA_MNDTR */
#define DMA_MNDTR_NDTLOW                     (BITS(0,17))                                        /*!< when current channel could not access Flash. */
#define DMA_MNDTR_NDTHIGH                    (BITS(18,19))                                       /*!< when current channel could access Flash. */

/* DMA_MDADR */
#define DMA_MDADR_ADDRLOW                    (BITS(0,21))                                        /*!< default valid bit field. */
#define DMA_MDADR_ADDRHIGH                   (BITS(28,31))                                       /*!< default valid bit field. */

/* DMA_MSADR */
#define DMA_MSADR_ADDRLOW                    (BITS(0,21))                                        /*!< default valid bit field. */
#define DMA_MSADR_ADDRHIGH                   (BITS(28,31))                                       /*!< default valid bit field. */

/* DMA_IE */
#define DMA_IE_HTIE0                         (BIT(0))                                            /*!< Enable channel 0 transfer half interrupt. */
#define DMA_IE_TCIE0                         (BIT(1))                                            /*!< Enable channel 0 transfer complete interrupt. */
#define DMA_IE_TEIE0                         (BIT(2))                                            /*!< Enable channel 0 transfer error interrupt. */
#define DMA_IE_HTIE1                         (BIT(4))                                            /*!< Enable channel 1 transfer half interrupt. */
#define DMA_IE_TCIE1                         (BIT(5))                                            /*!< Enable channel 1 transfer complete interrupt. */
#define DMA_IE_TEIE1                         (BIT(6))                                            /*!< Enable channel 1 transfer error interrupt. */
#define DMA_IE_HTIE2                         (BIT(8))                                            /*!< Enable channel 2 transfer half interrupt. */
#define DMA_IE_TCIE2                         (BIT(9))                                            /*!< Enable channel 2 transfer complete interrupt. */
#define DMA_IE_TEIE2                         (BIT(10))                                           /*!< Enable channel 2 transfer error interrupt. */
#define DMA_IE_HTIE3                         (BIT(12))                                           /*!< Enable channel 3 transfer half interrupt. */
#define DMA_IE_TCIE3                         (BIT(13))                                           /*!< Enable channel 3 transfer complete interrupt. */
#define DMA_IE_TEIE3                         (BIT(14))                                           /*!< Enable channel 3 transfer error interrupt. */
#define DMA_IE_HTIE4                         (BIT(16))                                           /*!< Enable channel 4 transfer half interrupt, only valid in DMAx0. */
#define DMA_IE_TCIE4                         (BIT(17))                                           /*!< Enable channel 4 transfer complete interrupt, only valid in DMAx0. */
#define DMA_IE_TEIE4                         (BIT(18))                                           /*!< Enable channel 4 transfer error interrupt, only valid in DMAx0. */

/* DMA_IF */
#define DMA_IF_HTIF0                         (BIT(0))                                            /*!< Check channel 0 transfer half interrupt flag is actived. */
#define DMA_IF_TCIF0                         (BIT(1))                                            /*!< Check channel 0 transfer complete interrupt flag is actived. */
#define DMA_IF_TEIF0                         (BIT(2))                                            /*!< Check channel 0 transfer error interrupt flag is actived. */
#define DMA_IF_HTIF1                         (BIT(4))                                            /*!< Check channel 1 transfer half interrupt flag is actived. */
#define DMA_IF_TCIF1                         (BIT(5))                                            /*!< Check channel 1 transfer complete interrupt flag is actived. */
#define DMA_IF_TEIF1                         (BIT(6))                                            /*!< Check channel 1 transfer error interrupt flag is actived. */
#define DMA_IF_HTIF2                         (BIT(8))                                            /*!< Check channel 2 transfer half interrupt flag is actived. */
#define DMA_IF_TCIF2                         (BIT(9))                                            /*!< Check channel 2 transfer complete interrupt flag is actived. */
#define DMA_IF_TEIF2                         (BIT(10))                                           /*!< Check channel 2 transfer error interrupt flag is actived. */
#define DMA_IF_HTIF3                         (BIT(12))                                           /*!< Check channel 3 transfer half interrupt flag is actived. */
#define DMA_IF_TCIF3                         (BIT(13))                                           /*!< Check channel 3 transfer complete interrupt flag is actived. */
#define DMA_IF_TEIF3                         (BIT(14))                                           /*!< Check channel 3 transfer error interrupt flag is actived. */
#define DMA_IF_HTIF4                         (BIT(16))                                           /*!< Check channel 4 transfer half interrupt flag is actived, only valid in DMAx0. */
#define DMA_IF_TCIF4                         (BIT(17))                                           /*!< Check channel 4 transfer complete interrupt flag is actived, only valid in DMAx0. */
#define DMA_IF_TEIF4                         (BIT(18))                                           /*!< Check channel 4 transfer error interrupt flag is actived, only valid in DMAx0. */

/* DMA_MIE */
#define DMA_MIE_HTIE0                        (BIT(0))                                            /*!< Enable M2M channel 0 transfer half interrupt. */
#define DMA_MIE_TCIE0                        (BIT(1))                                            /*!< Enable M2M channel 0 transfer complete interrupt. */
#define DMA_MIE_TEIE0                        (BIT(2))                                            /*!< Enable M2M channel 0 transfer error interrupt. */
#define DMA_MIE_HTIE1                        (BIT(4))                                            /*!< Enable M2M channel 1 transfer half interrupt. */
#define DMA_MIE_TCIE1                        (BIT(5))                                            /*!< Enable M2M channel 1 transfer complete interrupt. */
#define DMA_MIE_TEIE1                        (BIT(6))                                            /*!< Enable M2M channel 1 transfer error interrupt. */

/* DMA_MIF */
#define DMA_MIF_HTIF0                        (BIT(0))                                            /*!< Check M2M channel 0 tansfer half interrupt flag is actived. */
#define DMA_MIF_TCIF0                        (BIT(1))                                            /*!< Check M2M channel 0 tansfer complete interrupt flag is actived. */
#define DMA_MIF_TEIF0                        (BIT(2))                                            /*!< Check M2M channel 0 tansfer error interrupt flag is actived. */
#define DMA_MIF_HTIF1                        (BIT(4))                                            /*!< Check M2M channel 1 tansfer half interrupt flag is actived. */
#define DMA_MIF_TCIF1                        (BIT(5))                                            /*!< Check M2M channel 1 tansfer complete interrupt flag is actived. */
#define DMA_MIF_TEIF1                        (BIT(6))                                            /*!< Check M2M channel 1 tansfer error interrupt flag is actived. */

/* constants definitions */
/* DMA_CTRL_EN */
#define DMA_CTRL_EN_POS                      (0U)                                                /*!< DMA_CTRL_EN bit offset */

/* DMA_CTRL_CIRC */
#define DMA_CTRL_CIRC_POS                    (1U)                                                /*!< DMA_CTRL_CIRC bit offset */

/* DMA_CTRL_PL */
#define DMA_CTRL_PL_POS                      (2U)                                                /*!< DMA_CTRL_PL bit offset */
#define DMA_CTRL_PL_CFG(regval)              (DMA_CTRL_PL & ((uint32_t)(regval) << DMA_CTRL_PL_POS))
#define DMA_CTRL_PL_LOW                      (DMA_CTRL_PL_CFG(0x0))                              /*!< Set the channel priority to the lowest. */
#define DMA_CTRL_PL_MEDIUM                   (DMA_CTRL_PL_CFG(0x1))                              /*!< Set the channel priority to medium. */
#define DMA_CTRL_PL_HIGH                     (DMA_CTRL_PL_CFG(0x2))                              /*!< Set the channel priority to high. */
#define DMA_CTRL_PL_VERYHIGH                 (DMA_CTRL_PL_CFG(0x3))                              /*!< Set the channel priority to the highest. */

/* DMA_CTRL_MSIZE */
#define DMA_CTRL_MSIZE_POS                   (4U)                                                /*!< DMA_CTRL_MSIZE bit offset */
#define DMA_CTRL_MSIZE_CFG(regval)           (DMA_CTRL_MSIZE & ((uint32_t)(regval) << DMA_CTRL_MSIZE_POS))
#define DMA_CTRL_MSIZE_BYTE                  (DMA_CTRL_MSIZE_CFG(0x0))                           /*!< Data is transferred in byte. */
#define DMA_CTRL_MSIZE_HWORD                 (DMA_CTRL_MSIZE_CFG(0x1))                           /*!< Data is transferred in half-word. */
#define DMA_CTRL_MSIZE_WORD                  (DMA_CTRL_MSIZE_CFG(0x2))                           /*!< Data is transferred in word. */

/* DMA_CTRL_DIR */
#define DMA_CTRL_DIR_POS                     (6U)                                                /*!< DMA_CTRL_DIR bit offset */

/* DMA_NDTR_NDTLOW */
#define DMA_NDTR_NDTLOW_POS                  (0U)                                                /*!< DMA_NDTR_NDTLOW bit offset */

/* DMA_NDTR_NDTHIGH */
#define DMA_NDTR_NDTHIGH_POS                 (18U)                                               /*!< DMA_NDTR_NDTHIGH bit offset */

/* DMA_MADR_ADDRLOW */
#define DMA_MADR_ADDRLOW_POS                 (0U)                                                /*!< DMA_MADR_ADDRLOW bit offset */

/* DMA_MADR_ADDRHIGH */
#define DMA_MADR_ADDRHIGH_POS                (18U)                                               /*!< DMA_MADR_ADDRHIGH bit offset */

/* DMA_MADR_ADDRVERYHIGH */
#define DMA_MADR_ADDRVERYHIGH_POS            (28U)                                               /*!< DMA_MADR_ADDRVERYHIGH bit offset */

/* DMA_PADR_ADDRLOW */
#define DMA_PADR_ADDRLOW_POS                 (0U)                                                /*!< DMA_PADR_ADDRLOW bit offset */

/* DMA_PADR_ADDRHIGH */
#define DMA_PADR_ADDRHIGH_POS                (28U)                                               /*!< DMA_PADR_ADDRHIGH bit offset */

/* DMA_MCTRL_EN */
#define DMA_MCTRL_EN_POS                     (0U)                                                /*!< DMA_MCTRL_EN bit offset */

/* DMA_MCTRL_CIRC */
#define DMA_MCTRL_CIRC_POS                   (1U)                                                /*!< DMA_MCTRL_CIRC bit offset */

/* DMA_MCTRL_PL */
#define DMA_MCTRL_PL_POS                     (2U)                                                /*!< DMA_MCTRL_PL bit offset */
#define DMA_MCTRL_PL_CFG(regval)             (DMA_MCTRL_PL & ((uint32_t)(regval) << DMA_MCTRL_PL_POS))
#define DMA_MCTRL_PL_LOW                     (DMA_MCTRL_PL_CFG(0x0))                             /*!< Set the channel priority to the lowest */
#define DMA_MCTRL_PL_MEDIUM                  (DMA_MCTRL_PL_CFG(0x1))                             /*!< Set the channel priority to medium */
#define DMA_MCTRL_PL_HIGH                    (DMA_MCTRL_PL_CFG(0x2))                             /*!< Set the channel priority to high */
#define DMA_MCTRL_PL_VERYHIGH                (DMA_MCTRL_PL_CFG(0x3))                             /*!< Set the channel priority to the highest */

/* DMA_MCTRL_DSIZE */
#define DMA_MCTRL_DSIZE_POS                  (4U)                                                /*!< DMA_MCTRL_DSIZE bit offset */
#define DMA_MCTRL_DSIZE_CFG(regval)          (DMA_MCTRL_DSIZE & ((uint32_t)(regval) << DMA_MCTRL_DSIZE_POS))
#define DMA_MCTRL_DSIZE_BYTE                 (DMA_MCTRL_DSIZE_CFG(0x0))                          /*!< Data is transferred in byte */
#define DMA_MCTRL_DSIZE_HWORD                (DMA_MCTRL_DSIZE_CFG(0x1))                          /*!< Data is transferred in half-word */
#define DMA_MCTRL_DSIZE_WORD                 (DMA_MCTRL_DSIZE_CFG(0x2))                          /*!< Data is transferred in word */

/* DMA_MCTRL_SSIZE */
#define DMA_MCTRL_SSIZE_POS                  (6U)                                                /*!< DMA_MCTRL_SSIZE bit offset */
#define DMA_MCTRL_SSIZE_CFG(regval)          (DMA_MCTRL_SSIZE & ((uint32_t)(regval) << DMA_MCTRL_SSIZE_POS))
#define DMA_MCTRL_SSIZE_BYTE                 (DMA_MCTRL_SSIZE_CFG(0x0))                          /*!< Data is transferred in byte */
#define DMA_MCTRL_SSIZE_HWORD                (DMA_MCTRL_SSIZE_CFG(0x1))                          /*!< Data is transferred in half-word */
#define DMA_MCTRL_SSIZE_WORD                 (DMA_MCTRL_SSIZE_CFG(0x2))                          /*!< Data is transferred in word */

/* DMA_MNDTR_NDTLOW */
#define DMA_MNDTR_NDTLOW_POS                 (0U)                                                /*!< DMA_MNDTR_NDTLOW bit offset */

/* DMA_MNDTR_NDTHIGH */
#define DMA_MNDTR_NDTHIGH_POS                (18U)                                               /*!< DMA_MNDTR_NDTHIGH bit offset */

/* DMA_MDADR_ADDRLOW */
#define DMA_MDADR_ADDRLOW_POS                (0U)                                                /*!< DMA_MDADR_ADDRLOW bit offset */

/* DMA_MDADR_ADDRHIGH */
#define DMA_MDADR_ADDRHIGH_POS               (28U)                                               /*!< DMA_MDADR_ADDRHIGH bit offset */

/* DMA_MSADR_ADDRLOW */
#define DMA_MSADR_ADDRLOW_POS                (0U)                                                /*!< DMA_MSADR_ADDRLOW bit offset */

/* DMA_MSADR_ADDRHIGH */
#define DMA_MSADR_ADDRHIGH_POS               (28U)                                               /*!< DMA_MSADR_ADDRHIGH bit offset */

/* DMA_IE_HTIE0 */
#define DMA_IE_HTIE0_POS                     (0U)                                                /*!< DMA_IE_HTIE0 bit offset */

/* DMA_IE_TCIE0 */
#define DMA_IE_TCIE0_POS                     (1U)                                                /*!< DMA_IE_TCIE0 bit offset */

/* DMA_IE_TEIE0 */
#define DMA_IE_TEIE0_POS                     (2U)                                                /*!< DMA_IE_TEIE0 bit offset */

/* DMA_IE_HTIE1 */
#define DMA_IE_HTIE1_POS                     (4U)                                                /*!< DMA_IE_HTIE1 bit offset */

/* DMA_IE_TCIE1 */
#define DMA_IE_TCIE1_POS                     (5U)                                                /*!< DMA_IE_TCIE1 bit offset */

/* DMA_IE_TEIE1 */
#define DMA_IE_TEIE1_POS                     (6U)                                                /*!< DMA_IE_TEIE1 bit offset */

/* DMA_IE_HTIE2 */
#define DMA_IE_HTIE2_POS                     (8U)                                                /*!< DMA_IE_HTIE2 bit offset */

/* DMA_IE_TCIE2 */
#define DMA_IE_TCIE2_POS                     (9U)                                                /*!< DMA_IE_TCIE2 bit offset */

/* DMA_IE_TEIE2 */
#define DMA_IE_TEIE2_POS                     (10U)                                               /*!< DMA_IE_TEIE2 bit offset */

/* DMA_IE_HTIE3 */
#define DMA_IE_HTIE3_POS                     (12U)                                               /*!< DMA_IE_HTIE3 bit offset */

/* DMA_IE_TCIE3 */
#define DMA_IE_TCIE3_POS                     (13U)                                               /*!< DMA_IE_TCIE3 bit offset */

/* DMA_IE_TEIE3 */
#define DMA_IE_TEIE3_POS                     (14U)                                               /*!< DMA_IE_TEIE3 bit offset */

/* DMA_IE_HTIE4 */
#define DMA_IE_HTIE4_POS                     (16U)                                               /*!< DMA_IE_HTIE4 bit offset */

/* DMA_IE_TCIE4 */
#define DMA_IE_TCIE4_POS                     (17U)                                               /*!< DMA_IE_TCIE4 bit offset */

/* DMA_IE_TEIE4 */
#define DMA_IE_TEIE4_POS                     (18U)                                               /*!< DMA_IE_TEIE4 bit offset */

/* DMA_IF_HTIF0 */
#define DMA_IF_HTIF0_POS                     (0U)                                                /*!< DMA_IF_HTIF0 bit offset */

/* DMA_IF_TCIF0 */
#define DMA_IF_TCIF0_POS                     (1U)                                                /*!< DMA_IF_TCIF0 bit offset */

/* DMA_IF_TEIF0 */
#define DMA_IF_TEIF0_POS                     (2U)                                                /*!< DMA_IF_TEIF0 bit offset */

/* DMA_IF_HTIF1 */
#define DMA_IF_HTIF1_POS                     (4U)                                                /*!< DMA_IF_HTIF1 bit offset */

/* DMA_IF_TCIF1 */
#define DMA_IF_TCIF1_POS                     (5U)                                                /*!< DMA_IF_TCIF1 bit offset */

/* DMA_IF_TEIF1 */
#define DMA_IF_TEIF1_POS                     (6U)                                                /*!< DMA_IF_TEIF1 bit offset */

/* DMA_IF_HTIF2 */
#define DMA_IF_HTIF2_POS                     (8U)                                                /*!< DMA_IF_HTIF2 bit offset */

/* DMA_IF_TCIF2 */
#define DMA_IF_TCIF2_POS                     (9U)                                                /*!< DMA_IF_TCIF2 bit offset */

/* DMA_IF_TEIF2 */
#define DMA_IF_TEIF2_POS                     (10U)                                               /*!< DMA_IF_TEIF2 bit offset */

/* DMA_IF_HTIF3 */
#define DMA_IF_HTIF3_POS                     (12U)                                               /*!< DMA_IF_HTIF3 bit offset */

/* DMA_IF_TCIF3 */
#define DMA_IF_TCIF3_POS                     (13U)                                               /*!< DMA_IF_TCIF3 bit offset */

/* DMA_IF_TEIF3 */
#define DMA_IF_TEIF3_POS                     (14U)                                               /*!< DMA_IF_TEIF3 bit offset */

/* DMA_IF_HTIF4 */
#define DMA_IF_HTIF4_POS                     (16U)                                               /*!< DMA_IF_HTIF4 bit offset */

/* DMA_IF_TCIF4 */
#define DMA_IF_TCIF4_POS                     (17U)                                               /*!< DMA_IF_TCIF4 bit offset */

/* DMA_IF_TEIF4 */
#define DMA_IF_TEIF4_POS                     (18U)                                               /*!< DMA_IF_TEIF4 bit offset */

/* DMA_MIE_HTIE0 */
#define DMA_MIE_HTIE0_POS                    (0U)                                                /*!< DMA_MIE_HTIE0 bit offset */

/* DMA_MIE_TCIE0 */
#define DMA_MIE_TCIE0_POS                    (1U)                                                /*!< DMA_MIE_TCIE0 bit offset */

/* DMA_MIE_TEIE0 */
#define DMA_MIE_TEIE0_POS                    (2U)                                                /*!< DMA_MIE_TEIE0 bit offset */

/* DMA_MIE_HTIE1 */
#define DMA_MIE_HTIE1_POS                    (4U)                                                /*!< DMA_MIE_HTIE1 bit offset */

/* DMA_MIE_TCIE1 */
#define DMA_MIE_TCIE1_POS                    (5U)                                                /*!< DMA_MIE_TCIE1 bit offset */

/* DMA_MIE_TEIE1 */
#define DMA_MIE_TEIE1_POS                    (6U)                                                /*!< DMA_MIE_TEIE1 bit offset */

/* DMA_MIF_HTIF0 */
#define DMA_MIF_HTIF0_POS                    (0U)                                                /*!< DMA_MIF_HTIF0 bit offset */

/* DMA_MIF_TCIF0 */
#define DMA_MIF_TCIF0_POS                    (1U)                                                /*!< DMA_MIF_TCIF0 bit offset */

/* DMA_MIF_TEIF0 */
#define DMA_MIF_TEIF0_POS                    (2U)                                                /*!< DMA_MIF_TEIF0 bit offset */

/* DMA_MIF_HTIF1 */
#define DMA_MIF_HTIF1_POS                    (4U)                                                /*!< DMA_MIF_HTIF1 bit offset */

/* DMA_MIF_TCIF1 */
#define DMA_MIF_TCIF1_POS                    (5U)                                                /*!< DMA_MIF_TCIF1 bit offset */

/* DMA_MIF_TEIF1 */
#define DMA_MIF_TEIF1_POS                    (6U)                                                /*!< DMA_MIF_TEIF1 bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_DMA_H */
