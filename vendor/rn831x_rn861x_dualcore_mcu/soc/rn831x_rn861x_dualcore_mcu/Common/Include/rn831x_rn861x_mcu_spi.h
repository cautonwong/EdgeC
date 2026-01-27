/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_spi.h
 * @author  Renergy Software Team
 * @brief   SPI module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_SPI_H
#define _RN831x_RN861x_MCU_SPI_H

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

/* SPI */
typedef struct
{
    __IO uint32_t CTRL;                      /*!< offset: 0x000 info: Control Register */
    __IO uint32_t STA;                       /*!< offset: 0x004 info: Status Register */
    __IO uint32_t TXDATA;                    /*!< offset: 0x008 info: Transmit Data Register */
    __IO uint32_t RXDATA;                    /*!< offset: 0x00C info: Receive Data Register */
    __IO uint32_t TXDFLT;                    /*!< offset: 0x010 info: Default Transmit Data Register */
    __IO uint32_t DMA_CTRL;                  /*!< offset: 0x014 info: DMA Control Register */
    __IO uint32_t DMA_TBADR;                 /*!< offset: 0x018 info: DMA Transmit Base Address Register */
    __IO uint32_t DMA_RBADR;                 /*!< offset: 0x01C info: DMA Receive Base Address Register */
    __IO uint32_t DMA_TLEN;                  /*!< offset: 0x020 info: DMA Transmit Length Register */
    __IO uint32_t DMA_RLEN;                  /*!< offset: 0x024 info: DMA Receive Length Register */
    __IO uint32_t DMA_TADR;                  /*!< offset: 0x028 info: Current DMA Transmit Address Register */
    __IO uint32_t DMA_RADR;                  /*!< offset: 0x02C info: Current DMA Receive Address Register */
    __IO uint32_t DMA_IE;                    /*!< offset: 0x030 info: DMA Interrupt Enable Register */
    __IO uint32_t DMA_FLG;                   /*!< offset: 0x034 info: DMA Interrupt Flag Register */
    __IO uint32_t DLYSAMP;                   /*!< offset: 0x038 info: Sample Delay Configuration Register */
} SPI_TypeDef;

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
#define SPI0_CORE0_BASE (0x20038000U)
#define SPI1_CORE0_BASE (0x2003C000U)
#define SPI2_CORE0_BASE (0x20040000U)
#define SPI3_CORE0_BASE (0x20044000U)
#define SPI0_CORE1_BASE (0xA0038000U)
#define SPI1_CORE1_BASE (0xA003C000U)
#define SPI2_CORE1_BASE (0xA0040000U)
#define SPI3_CORE1_BASE (0xA0044000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define SPI0_CORE0 ((SPI_TypeDef *)      SPI0_CORE0_BASE)
#define SPI1_CORE0 ((SPI_TypeDef *)      SPI1_CORE0_BASE)
#define SPI2_CORE0 ((SPI_TypeDef *)      SPI2_CORE0_BASE)
#define SPI3_CORE0 ((SPI_TypeDef *)      SPI3_CORE0_BASE)
#define SPI0_CORE1 ((SPI_TypeDef *)      SPI0_CORE1_BASE)
#define SPI1_CORE1 ((SPI_TypeDef *)      SPI1_CORE1_BASE)
#define SPI2_CORE1 ((SPI_TypeDef *)      SPI2_CORE1_BASE)
#define SPI3_CORE1 ((SPI_TypeDef *)      SPI3_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define SPI0  SPI0_CORE0
#define SPI1  SPI1_CORE0
#define SPI2  SPI2_CORE0
#define SPI3  SPI3_CORE0
#endif
#if defined(CORE1)
#define SPI0  SPI0_CORE1
#define SPI1  SPI1_CORE1
#define SPI2  SPI2_CORE1
#define SPI3  SPI3_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** SPI Instances *******************************/
#define IS_SPI_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == SPI0_CORE0) || \
                                          ((INSTANCE) == SPI1_CORE0) || \
                                          ((INSTANCE) == SPI2_CORE0) || \
                                          ((INSTANCE) == SPI3_CORE0) || \
                                          ((INSTANCE) == SPI0_CORE1) || \
                                          ((INSTANCE) == SPI1_CORE1) || \
                                          ((INSTANCE) == SPI2_CORE1) || \
                                          ((INSTANCE) == SPI3_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* SPI_CTRL */
#define SPI_CTRL_EN                          (BIT(0))                                            /*!< SPI Interface Enable */
#define SPI_CTRL_MAST_SLAV                   (BIT(1))                                            /*!< Master/Slave Mode Selection */
#define SPI_CTRL_TR_IRQ_EN                   (BIT(2))                                            /*!< Data Transfer Interrupt Enable */
#define SPI_CTRL_ERR_IRQ_EN                  (BIT(3))                                            /*!< SCSN Mode Error Interrupt Enable */
#define SPI_CTRL_RXCOL_IRQ_EN                (BIT(4))                                            /*!< RX data conflict interrupt enable */
#define SPI_CTRL_TXCOL_IRQ_EN                (BIT(5))                                            /*!< TX data conflict interrupt enable */
#define SPI_CTRL_LMSB                        (BIT(6))                                            /*!< MSB/LSB first selection */
#define SPI_CTRL_CPOL                        (BIT(7))                                            /*!< Clock polarity */
#define SPI_CTRL_CPHA                        (BIT(8))                                            /*!< Clock phase selection */
#define SPI_CTRL_SCSN_EN                     (BIT(9))                                            /*!< SCSN mode error detection enable (master mode only) */
#define SPI_CTRL_WIDTH                       (BITS(10,11))                                       /*!< Data width selection */
#define SPI_CTRL_CLKDIV                      (BITS(12,19))                                       /*!< SCK clock divider factor */
#define SPI_CTRL_TX_DFLT_EN                  (BIT(22))                                           /*!< Behavior when TX buffer is empty */
#define SPI_CTRL_TXEMPT_IRQEN                (BIT(23))                                           /*!< TX empty interrupt enable */
#define SPI_CTRL_SCSN_NEG_IRQEN              (BIT(24))                                           /*!< SCSN falling edge interrupt enable */
#define SPI_CTRL_SCSN_POS_IRQEN              (BIT(25))                                           /*!< SCSN rising edge interrupt enable */
#define SPI_CTRL_SLV_TX_ADV                  (BIT(26))                                           /*!< SPI slave high-speed mode enable */
#define SPI_CTRL_DLY_EN                      (BIT(27))                                           /*!< Delay enable (DMA master mode) */
#define SPI_CTRL_DLY_CYC_SEL                 (BITS(28,29))                                       /*!< Data delay selection (valid only in DMA master mode when delay is enabled). */

/* SPI_STA */
#define SPI_STA_TRIF                         (BIT(0))                                            /*!< Transfer complete flag */
#define SPI_STA_ERRIF                        (BIT(1))                                            /*!< SCSN mode error flag */
#define SPI_STA_RXCOLIF                      (BIT(2))                                            /*!< RX overflow flag */
#define SPI_STA_TXCOLIF                      (BIT(3))                                            /*!< TX conflict flag */
#define SPI_STA_TXBUF_VLD                    (BIT(4))                                            /*!< TX buffer valid flag */
#define SPI_STA_TXEMPT                       (BIT(5))                                            /*!< TX empty conflict flag */
#define SPI_STA_SCSN_NEG                     (BIT(6))                                            /*!< Slave mode: CSN falling event flag */
#define SPI_STA_SCSN_POS                     (BIT(7))                                            /*!< Slave mode: CSN rising event flag */

/* SPI_TXDATA */
#define SPI_TXDATA_TXDATA                    (BITS(0,31))                                        /*!< Transmit Data Register. Write valid when enabled. */

/* SPI_RXDATA */
#define SPI_RXDATA_RXDATA                    (BITS(0,31))                                        /*!< Receive Data Register */

/* SPI_TXDFLT */
#define SPI_TXDFLT_TXDFLT                    (BITS(0,31))                                        /*!< Default Transmit Data Register */

/* SPI_DMA_CTRL */
#define SPI_DMA_CTRL_TX_DMA_EN               (BIT(0))                                            /*!< Transmit DMA Enable */
#define SPI_DMA_CTRL_RX_DMA_EN               (BIT(1))                                            /*!< Receive DMA Enable */
#define SPI_DMA_CTRL_TX_CYC_MODE             (BIT(2))                                            /*!< Transmit Cycle Mode Enable */
#define SPI_DMA_CTRL_RX_CYC_MODE             (BIT(3))                                            /*!< Receive Cycle Mode Enable */
#define SPI_DMA_CTRL_CS_STOP_RXDMA           (BIT(4))                                            /*!< Disable RX_DMA_EN upon detection of SCSN high level */

/* SPI_DMA_TBADR */
#define SPI_DMA_TBADR_DMA_TBADR              (BITS(0,17))                                        /*!< DMA Transmit Start Address (Byte address). */

/* SPI_DMA_RBADR */
#define SPI_DMA_RBADR_DMA_RBADR              (BITS(0,17))                                        /*!< DMA Receive Base Address (Byte address). */

/* SPI_DMA_TLEN */
#define SPI_DMA_TLEN_DMA_TLEN                (BITS(0,17))                                        /*!< DMA Transmit Length (in Bytes). */

/* SPI_DMA_RLEN */
#define SPI_DMA_RLEN_DMA_RLEN                (BITS(0,17))                                        /*!< DMA Receive Length (in Bytes). */

/* SPI_DMA_TADR */
#define SPI_DMA_TADR_DMA_TADR                (BITS(0,17))                                        /*!< Current DMA Transmit Address (Byte address). */

/* SPI_DMA_RADR */
#define SPI_DMA_RADR_DMA_RADR                (BITS(0,17))                                        /*!< Current DMA Receive Address (Byte address). */

/* SPI_DMA_IE */
#define SPI_DMA_IE_TX_HIE                    (BIT(0))                                            /*!< DMA Transmit Half Interrupt Enable */
#define SPI_DMA_IE_TX_FIE                    (BIT(1))                                            /*!< DMA Transmit Full (Complete) Interrupt Enable */
#define SPI_DMA_IE_RX_HIE                    (BIT(2))                                            /*!< DMA Receive Half Interrupt Enable */
#define SPI_DMA_IE_RX_FIE                    (BIT(3))                                            /*!< DMA Receive Full (Complete) Interrupt Enable */
#define SPI_DMA_IE_TX_ERR_IE                 (BIT(4))                                            /*!< Transmit Data Error Interrupt Enable */
#define SPI_DMA_IE_RX_ERR_IE                 (BIT(5))                                            /*!< Receive Data Overwrite Interrupt Enable */

/* SPI_DMA_FLG */
#define SPI_DMA_FLG_TX_HDONE                 (BIT(0))                                            /*!< DMA Transmit Half Interrupt Flag */
#define SPI_DMA_FLG_TX_FDONE                 (BIT(1))                                            /*!< DMA Transmit Full (Complete) Interrupt Flag */
#define SPI_DMA_FLG_RX_HDONE                 (BIT(2))                                            /*!< DMA Receive Half Interrupt Flag */
#define SPI_DMA_FLG_RX_FDONE                 (BIT(3))                                            /*!< DMA Receive Full (Complete) Interrupt Flag */
#define SPI_DMA_FLG_RX_ERR                   (BIT(5))                                            /*!< Receive Data Overwrite Flag */

/* SPI_DLYSAMP */
#define SPI_DLYSAMP_DLYSMP_EN                (BIT(0))                                            /*!< SPI Receive Data Sample Delay Enable */
#define SPI_DLYSAMP_DLYSMP_SEL               (BITS(1,2))                                         /*!< Master Mode SPI Receive Data Sample Delay Selection */

/* constants definitions */
/* SPI_CTRL_EN */
#define SPI_CTRL_EN_POS                      (0U)                                                /*!< SPI_CTRL_EN bit offset */

/* SPI_CTRL_MAST_SLAV */
#define SPI_CTRL_MAST_SLAV_POS               (1U)                                                /*!< SPI_CTRL_MAST_SLAV bit offset */

/* SPI_CTRL_TR_IRQ_EN */
#define SPI_CTRL_TR_IRQ_EN_POS               (2U)                                                /*!< SPI_CTRL_TR_IRQ_EN bit offset */

/* SPI_CTRL_ERR_IRQ_EN */
#define SPI_CTRL_ERR_IRQ_EN_POS              (3U)                                                /*!< SPI_CTRL_ERR_IRQ_EN bit offset */

/* SPI_CTRL_RXCOL_IRQ_EN */
#define SPI_CTRL_RXCOL_IRQ_EN_POS            (4U)                                                /*!< SPI_CTRL_RXCOL_IRQ_EN bit offset */

/* SPI_CTRL_TXCOL_IRQ_EN */
#define SPI_CTRL_TXCOL_IRQ_EN_POS            (5U)                                                /*!< SPI_CTRL_TXCOL_IRQ_EN bit offset */

/* SPI_CTRL_LMSB */
#define SPI_CTRL_LMSB_POS                    (6U)                                                /*!< SPI_CTRL_LMSB bit offset */

/* SPI_CTRL_CPOL */
#define SPI_CTRL_CPOL_POS                    (7U)                                                /*!< SPI_CTRL_CPOL bit offset */

/* SPI_CTRL_CPHA */
#define SPI_CTRL_CPHA_POS                    (8U)                                                /*!< SPI_CTRL_CPHA bit offset */

/* SPI_CTRL_SCSN_EN */
#define SPI_CTRL_SCSN_EN_POS                 (9U)                                                /*!< SPI_CTRL_SCSN_EN bit offset */

/* SPI_CTRL_WIDTH */
#define SPI_CTRL_WIDTH_POS                   (10U)                                               /*!< SPI_CTRL_WIDTH bit offset */
#define SPI_CTRL_WIDTH_CFG(regval)  (SPI_CTRL_WIDTH & ((uint32_t)(regval) << SPI_CTRL_WIDTH_POS))
#define SPI_CTRL_WIDTH_8BIT                  (SPI_CTRL_WIDTH_CFG(0x0))                           /*!< Data length for SPI transfer: 8 bits */
#define SPI_CTRL_WIDTH_16BIT                 (SPI_CTRL_WIDTH_CFG(0x1))                           /*!< Data length for SPI transfer: 16 bits */
#define SPI_CTRL_WIDTH_32BIT                 (SPI_CTRL_WIDTH_CFG(0x2))                           /*!< Data length for SPI transfer: 32 bits */

/* SPI_CTRL_CLKDIV */
#define SPI_CTRL_CLKDIV_POS                  (12U)                                               /*!< SPI_CTRL_CLKDIV bit offset */

/* SPI_CTRL_TX_DFLT_EN */
#define SPI_CTRL_TX_DFLT_EN_POS              (22U)                                               /*!< SPI_CTRL_TX_DFLT_EN bit offset */

/* SPI_CTRL_TXEMPT_IRQEN */
#define SPI_CTRL_TXEMPT_IRQEN_POS            (23U)                                               /*!< SPI_CTRL_TXEMPT_IRQEN bit offset */

/* SPI_CTRL_SCSN_NEG_IRQEN */
#define SPI_CTRL_SCSN_NEG_IRQEN_POS          (24U)                                               /*!< SPI_CTRL_SCSN_NEG_IRQEN bit offset */

/* SPI_CTRL_SCSN_POS_IRQEN */
#define SPI_CTRL_SCSN_POS_IRQEN_POS          (25U)                                               /*!< SPI_CTRL_SCSN_POS_IRQEN bit offset */

/* SPI_CTRL_SLV_TX_ADV */
#define SPI_CTRL_SLV_TX_ADV_POS              (26U)                                               /*!< SPI_CTRL_SLV_TX_ADV bit offset */

/* SPI_CTRL_DLY_EN */
#define SPI_CTRL_DLY_EN_POS                  (27U)                                               /*!< SPI_CTRL_DLY_EN bit offset */

/* SPI_CTRL_DLY_CYC_SEL */
#define SPI_CTRL_DLY_CYC_SEL_POS             (28U)                                               /*!< SPI_CTRL_DLY_CYC_SEL bit offset */
#define SPI_CTRL_DLY_CYC_SEL_CFG(regval)  (SPI_CTRL_DLY_CYC_SEL & ((uint32_t)(regval) << SPI_CTRL_DLY_CYC_SEL_POS))
#define SPI_CTRL_DLY_CYC_SEL_1CYCLE          (SPI_CTRL_DLY_CYC_SEL_CFG(0x0))                     /*!< Insert 1 SPI_CLK delay per 8-bit data */
#define SPI_CTRL_DLY_CYC_SEL_2CYCLE          (SPI_CTRL_DLY_CYC_SEL_CFG(0x1))                     /*!< Insert 2 SPI_CLK delays per 8-bit data */
#define SPI_CTRL_DLY_CYC_SEL_4CYCLE          (SPI_CTRL_DLY_CYC_SEL_CFG(0x2))                     /*!< Insert 4 SPI_CLK delays per 8-bit data */
#define SPI_CTRL_DLY_CYC_SEL_8CYCLE          (SPI_CTRL_DLY_CYC_SEL_CFG(0x3))                     /*!< Insert 8 SPI_CLK delays per 8-bit data */

/* SPI_STA_TRIF */
#define SPI_STA_TRIF_POS                     (0U)                                                /*!< SPI_STA_TRIF bit offset */

/* SPI_STA_ERRIF */
#define SPI_STA_ERRIF_POS                    (1U)                                                /*!< SPI_STA_ERRIF bit offset */

/* SPI_STA_RXCOLIF */
#define SPI_STA_RXCOLIF_POS                  (2U)                                                /*!< SPI_STA_RXCOLIF bit offset */

/* SPI_STA_TXCOLIF */
#define SPI_STA_TXCOLIF_POS                  (3U)                                                /*!< SPI_STA_TXCOLIF bit offset */

/* SPI_STA_TXBUF_VLD */
#define SPI_STA_TXBUF_VLD_POS                (4U)                                                /*!< SPI_STA_TXBUF_VLD bit offset */

/* SPI_STA_TXEMPT */
#define SPI_STA_TXEMPT_POS                   (5U)                                                /*!< SPI_STA_TXEMPT bit offset */

/* SPI_STA_SCSN_NEG */
#define SPI_STA_SCSN_NEG_POS                 (6U)                                                /*!< SPI_STA_SCSN_NEG bit offset */

/* SPI_STA_SCSN_POS */
#define SPI_STA_SCSN_POS_POS                 (7U)                                                /*!< SPI_STA_SCSN_POS bit offset */

/* SPI_TXDATA_TXDATA */
#define SPI_TXDATA_TXDATA_POS                (0U)                                                /*!< SPI_TXDATA_TXDATA bit offset */

/* SPI_RXDATA_RXDATA */
#define SPI_RXDATA_RXDATA_POS                (0U)                                                /*!< SPI_RXDATA_RXDATA bit offset */

/* SPI_TXDFLT_TXDFLT */
#define SPI_TXDFLT_TXDFLT_POS                (0U)                                                /*!< SPI_TXDFLT_TXDFLT bit offset */

/* SPI_DMA_CTRL_TX_DMA_EN */
#define SPI_DMA_CTRL_TX_DMA_EN_POS           (0U)                                                /*!< SPI_DMA_CTRL_TX_DMA_EN bit offset */

/* SPI_DMA_CTRL_RX_DMA_EN */
#define SPI_DMA_CTRL_RX_DMA_EN_POS           (1U)                                                /*!< SPI_DMA_CTRL_RX_DMA_EN bit offset */

/* SPI_DMA_CTRL_TX_CYC_MODE */
#define SPI_DMA_CTRL_TX_CYC_MODE_POS         (2U)                                                /*!< SPI_DMA_CTRL_TX_CYC_MODE bit offset */

/* SPI_DMA_CTRL_RX_CYC_MODE */
#define SPI_DMA_CTRL_RX_CYC_MODE_POS         (3U)                                                /*!< SPI_DMA_CTRL_RX_CYC_MODE bit offset */

/* SPI_DMA_CTRL_CS_STOP_RXDMA */
#define SPI_DMA_CTRL_CS_STOP_RXDMA_POS       (4U)                                                /*!< SPI_DMA_CTRL_CS_STOP_RXDMA bit offset */

/* SPI_DMA_TBADR_DMA_TBADR */
#define SPI_DMA_TBADR_DMA_TBADR_POS          (0U)                                                /*!< SPI_DMA_TBADR_DMA_TBADR bit offset */

/* SPI_DMA_RBADR_DMA_RBADR */
#define SPI_DMA_RBADR_DMA_RBADR_POS          (0U)                                                /*!< SPI_DMA_RBADR_DMA_RBADR bit offset */

/* SPI_DMA_TLEN_DMA_TLEN */
#define SPI_DMA_TLEN_DMA_TLEN_POS            (0U)                                                /*!< SPI_DMA_TLEN_DMA_TLEN bit offset */

/* SPI_DMA_RLEN_DMA_RLEN */
#define SPI_DMA_RLEN_DMA_RLEN_POS            (0U)                                                /*!< SPI_DMA_RLEN_DMA_RLEN bit offset */

/* SPI_DMA_TADR_DMA_TADR */
#define SPI_DMA_TADR_DMA_TADR_POS            (0U)                                                /*!< SPI_DMA_TADR_DMA_TADR bit offset */

/* SPI_DMA_RADR_DMA_RADR */
#define SPI_DMA_RADR_DMA_RADR_POS            (0U)                                                /*!< SPI_DMA_RADR_DMA_RADR bit offset */

/* SPI_DMA_IE_TX_HIE */
#define SPI_DMA_IE_TX_HIE_POS                (0U)                                                /*!< SPI_DMA_IE_TX_HIE bit offset */

/* SPI_DMA_IE_TX_FIE */
#define SPI_DMA_IE_TX_FIE_POS                (1U)                                                /*!< SPI_DMA_IE_TX_FIE bit offset */

/* SPI_DMA_IE_RX_HIE */
#define SPI_DMA_IE_RX_HIE_POS                (2U)                                                /*!< SPI_DMA_IE_RX_HIE bit offset */

/* SPI_DMA_IE_RX_FIE */
#define SPI_DMA_IE_RX_FIE_POS                (3U)                                                /*!< SPI_DMA_IE_RX_FIE bit offset */

/* SPI_DMA_IE_TX_ERR_IE */
#define SPI_DMA_IE_TX_ERR_IE_POS             (4U)                                                /*!< SPI_DMA_IE_TX_ERR_IE bit offset */

/* SPI_DMA_IE_RX_ERR_IE */
#define SPI_DMA_IE_RX_ERR_IE_POS             (5U)                                                /*!< SPI_DMA_IE_RX_ERR_IE bit offset */

/* SPI_DMA_FLG_TX_HDONE */
#define SPI_DMA_FLG_TX_HDONE_POS             (0U)                                                /*!< SPI_DMA_FLG_TX_HDONE bit offset */

/* SPI_DMA_FLG_TX_FDONE */
#define SPI_DMA_FLG_TX_FDONE_POS             (1U)                                                /*!< SPI_DMA_FLG_TX_FDONE bit offset */

/* SPI_DMA_FLG_RX_HDONE */
#define SPI_DMA_FLG_RX_HDONE_POS             (2U)                                                /*!< SPI_DMA_FLG_RX_HDONE bit offset */

/* SPI_DMA_FLG_RX_FDONE */
#define SPI_DMA_FLG_RX_FDONE_POS             (3U)                                                /*!< SPI_DMA_FLG_RX_FDONE bit offset */

/* SPI_DMA_FLG_RX_ERR */
#define SPI_DMA_FLG_RX_ERR_POS               (5U)                                                /*!< SPI_DMA_FLG_RX_ERR bit offset */

/* SPI_DLYSAMP_DLYSMP_EN */
#define SPI_DLYSAMP_DLYSMP_EN_POS            (0U)                                                /*!< SPI_DLYSAMP_DLYSMP_EN bit offset */

/* SPI_DLYSAMP_DLYSMP_SEL */
#define SPI_DLYSAMP_DLYSMP_SEL_POS           (1U)                                                /*!< SPI_DLYSAMP_DLYSMP_SEL bit offset */
#define SPI_DLYSAMP_DLYSMP_SEL_CFG(regval)  (SPI_DLYSAMP_DLYSMP_SEL & ((uint32_t)(regval) << SPI_DLYSAMP_DLYSMP_SEL_POS))
#define SPI_DLYSAMP_DLYSMP_SEL_1CYCLE        (SPI_DLYSAMP_DLYSMP_SEL_CFG(0x0))                   /*!< Delay sampling by 1 sys_clk cycle */
#define SPI_DLYSAMP_DLYSMP_SEL_2CYCLE        (SPI_DLYSAMP_DLYSMP_SEL_CFG(0x1))                   /*!< Delay sampling by 2 sys_clk cycles */
#define SPI_DLYSAMP_DLYSMP_SEL_3CYCLE        (SPI_DLYSAMP_DLYSMP_SEL_CFG(0x2))                   /*!< Delay sampling by 3 sys_clk cycles */
#define SPI_DLYSAMP_DLYSMP_SEL_4CYCLE        (SPI_DLYSAMP_DLYSMP_SEL_CFG(0x3))                   /*!< Delay sampling by 4 sys_clk cycles */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_SPI_H */
