/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_lpuart.h
 * @author  Renergy Software Team
 * @brief   LPUART module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_LPUART_H
#define _RN831x_RN861x_MCU_LPUART_H

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

/* LPUART */
typedef struct
{
    __IO uint32_t MODE;                      /*!< offset: 0x000 info: Mode Configuration Register */
    __IO uint32_t IE;                        /*!< offset: 0x004 info: Interrupt Enable Register */
    __IO uint32_t STA;                       /*!< offset: 0x008 info: Status Register */
    __IO uint32_t BAUD;                      /*!< offset: 0x00C info: Baud Rate Register */
    __IO uint32_t TXDATA;                    /*!< offset: 0x010 info: Transmit Data Buffer Register */
    __IO uint32_t RXDATA;                    /*!< offset: 0x014 info: Receive Data Buffer Register */
    __IO uint32_t DMR;                       /*!< offset: 0x018 info: Data Match Register */
} LPUART_TypeDef;

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
#define LPUART_CORE0_BASE (0x20020000U)
#define LPUART_CORE1_BASE (0xA0020000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define LPUART_CORE0 ((LPUART_TypeDef *)   LPUART_CORE0_BASE)
#define LPUART_CORE1 ((LPUART_TypeDef *)   LPUART_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define LPUART  LPUART_CORE0
#endif
#if defined(CORE1)
#define LPUART  LPUART_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** LPUART Instances *******************************/
#define IS_LPUART_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == LPUART_CORE0) || \
                                             ((INSTANCE) == LPUART_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* LPUART_MODE */
#define LPUART_MODE_TE                       (BIT(0))                                            /*!< Transmitter Enable */
#define LPUART_MODE_RE                       (BIT(1))                                            /*!< Receiver Enable */
#define LPUART_MODE_IRE                      (BIT(2))                                            /*!< Infrared Enable */
#define LPUART_MODE_NEG                      (BIT(3))                                            /*!< Clear TX channel and stop & clear transmission */
#define LPUART_MODE_IRPOL                    (BIT(4))                                            /*!< Transmit watermark level */
#define LPUART_MODE_DATLEN                   (BITS(5,6))                                         /*!< Data Length - excludes parity bit */
#define LPUART_MODE_PARS                     (BITS(7,9))                                         /*!< Parity Selection */
#define LPUART_MODE_STOPS                    (BIT(10))                                           /*!< Stop Bit Width Select */
#define LPUART_MODE_LMSB                     (BIT(11))                                           /*!< LSB/MSB Transmission Order Select */
#define LPUART_MODE_ILBE                     (BIT(12))                                           /*!< Internal Loopback Enable */
#define LPUART_MODE_WUSEL                    (BITS(13,14))                                       /*!< Wakeup Event Configuration */

/* LPUART_IE */
#define LPUART_IE_TXIE                       (BIT(0))                                            /*!< Transmit Complete Interrupt Enable */
#define LPUART_IE_RXIE                       (BIT(1))                                            /*!< Receive Complete Interrupt Enable */
#define LPUART_IE_WUFIE                      (BIT(2))                                            /*!< Wakeup from Sleep Interrupt Enable */
#define LPUART_IE_PEIE                       (BIT(3))                                            /*!< Parity Error Interrupt Enable */
#define LPUART_IE_OEIE                       (BIT(4))                                            /*!< Overrun Error Interrupt Enable */
#define LPUART_IE_FEIE                       (BIT(5))                                            /*!< Framing Error Interrupt Enable */
#define LPUART_IE_DEIE                       (BIT(6))                                            /*!< Data Error Interrupt Enable */
#define LPUART_IE_NEIE                       (BIT(7))                                            /*!< Start Bit Noise Detected Interrupt Enable */
#define LPUART_IE_IDLEIE                     (BIT(8))                                            /*!< Idle Frame Detected Interrupt Enable */
#define LPUART_IE_TXFEIE                     (BIT(9))                                            /*!< TX FIFO Empty Interrupt Enable */
#define LPUART_IE_TXFHEIE                    (BIT(10))                                           /*!< TX FIFO Half Empty Interrupt Enable */
#define LPUART_IE_RXFNEIE                    (BIT(11))                                           /*!< RX FIFO Not Empty Interrupt Enable */
#define LPUART_IE_RXFHFIE                    (BIT(12))                                           /*!< RX FIFO Half Full Interrupt Enable */
#define LPUART_IE_RXFFIE                     (BIT(13))                                           /*!< RX FIFO Full Interrupt Enable */

/* LPUART_STA */
#define LPUART_STA_TXIF                      (BIT(0))                                            /*!< Transmit Complete Flag */
#define LPUART_STA_RXIF                      (BIT(1))                                            /*!< Receive Complete Flag */
#define LPUART_STA_WUF                       (BIT(2))                                            /*!< Wakeup Event Flag */
#define LPUART_STA_PEIF                      (BIT(3))                                            /*!< Parity Error Flag */
#define LPUART_STA_OEIF                      (BIT(4))                                            /*!< Overrun Error Flag */
#define LPUART_STA_FEIF                      (BIT(5))                                            /*!< Framing Error Flag */
#define LPUART_STA_DEIF                      (BIT(6))                                            /*!< Data Error Flag */
#define LPUART_STA_NEIF                      (BIT(7))                                            /*!< Start Bit Noise Detected Flag */
#define LPUART_STA_IDLEIF                    (BIT(8))                                            /*!< Idle Frame Detected Flag */
#define LPUART_STA_TBUSY                     (BIT(9))                                            /*!< Transmitter Busy Flag */
#define LPUART_STA_RBUSY                     (BIT(10))                                           /*!< Receiver Busy Flag */
#define LPUART_STA_TXFEIF                    (BIT(11))                                           /*!< TX FIFO Empty Flag */
#define LPUART_STA_TXFHEIF                   (BIT(12))                                           /*!< TX FIFO Half Empty Flag */
#define LPUART_STA_TXFF                      (BIT(13))                                           /*!< TX FIFO Full Flag */
#define LPUART_STA_RXFNEIF                   (BIT(14))                                           /*!< RX FIFO Not Empty Flag */
#define LPUART_STA_RXFHFIF                   (BIT(15))                                           /*!< RX FIFO Half Full Flag */
#define LPUART_STA_RXFFIF                    (BIT(16))                                           /*!< RX FIFO Full Flag */
#define LPUART_STA_TXFCNT                    (BITS(17,19))                                       /*!< TX FIFO Data Count */
#define LPUART_STA_RXFCNT                    (BITS(20,22))                                       /*!< RX FIFO Data Count */

/* LPUART_BAUD */
#define LPUART_BAUD_BAUD                     (BITS(0,2))                                         /*!< Bit Width Modulation Control */
#define LPUART_BAUD_MCTL                     (BITS(16,27))                                       /*!< Baud Rate Select */

/* LPUART_TXDATA */
#define LPUART_TXDATA_TXDATA                 (BITS(0,7))                                         /*!< Transmit Data */
#define LPUART_TXDATA_UP                     (BIT(8))                                            /*!< User Parity Bit */

/* LPUART_RXDATA */
#define LPUART_RXDATA_RXDATA                 (BITS(0,7))                                         /*!< Receive Data */
#define LPUART_RXDATA_UP                     (BIT(8))                                            /*!< User Parity Bit */

/* LPUART_DMR */
#define LPUART_DMR_MDATA                     (BITS(0,7))                                         /*!< Match Data. Compares with the first received frame of data. If a match is detected, it triggers the wakeup interrupt (used for wakeup from sleep mode). */

/* constants definitions */
/* LPUART_MODE_TE */
#define LPUART_MODE_TE_POS                   (0U)                                                /*!< LPUART_MODE_TE bit offset */

/* LPUART_MODE_RE */
#define LPUART_MODE_RE_POS                   (1U)                                                /*!< LPUART_MODE_RE bit offset */

/* LPUART_MODE_IRE */
#define LPUART_MODE_IRE_POS                  (2U)                                                /*!< LPUART_MODE_IRE bit offset */

/* LPUART_MODE_NEG */
#define LPUART_MODE_NEG_POS                  (3U)                                                /*!< LPUART_MODE_NEG bit offset */
#define LPUART_MODE_NEG_POSITIVE             ((uint32_t)0x00000000U)                             /*!< Positive polarity (Idle level = High) */
#define LPUART_MODE_NEG_NEGATIVE             (LPUART_MODE_NEG)                                   /*!< Negative polarity (Idle level = Low) */

/* LPUART_MODE_IRPOL */
#define LPUART_MODE_IRPOL_POS                (4U)                                                /*!< LPUART_MODE_IRPOL bit offset */
#define LPUART_MODE_IRPOL_POSITIVE           ((uint32_t)0x00000000U)                             /*!< Positive polarity (Idle level = High, Modulation = Low) */
#define LPUART_MODE_IRPOL_NEGATIVE           (LPUART_MODE_IRPOL)                                 /*!< Negative polarity (Idle level = Low, Modulation = High) */

/* LPUART_MODE_DATLEN */
#define LPUART_MODE_DATLEN_POS               (5U)                                                /*!< LPUART_MODE_DATLEN bit offset */
#define LPUART_MODE_DATLEN_CFG(regval)  (LPUART_MODE_DATLEN & ((uint32_t)(regval) << LPUART_MODE_DATLEN_POS))
#define LPUART_MODE_DATLEN_5B                (LPUART_MODE_DATLEN_CFG(0x0))                       /*!< 5 bits data length */
#define LPUART_MODE_DATLEN_6B                (LPUART_MODE_DATLEN_CFG(0x1))                       /*!< 6 bits data length */
#define LPUART_MODE_DATLEN_7B                (LPUART_MODE_DATLEN_CFG(0x2))                       /*!< 7 bits data length */
#define LPUART_MODE_DATLEN_8B                (LPUART_MODE_DATLEN_CFG(0x3))                       /*!< 8 bits data length */

/* LPUART_MODE_PARS */
#define LPUART_MODE_PARS_POS                 (7U)                                                /*!< LPUART_MODE_PARS bit offset */
#define LPUART_MODE_PARS_CFG(regval)  (LPUART_MODE_PARS & ((uint32_t)(regval) << LPUART_MODE_PARS_POS))
#define LPUART_MODE_PARS_NONE                (LPUART_MODE_PARS_CFG(0x0))                         /*!< No parity */
#define LPUART_MODE_PARS_ODD                 (LPUART_MODE_PARS_CFG(0x1))                         /*!< Odd parity */
#define LPUART_MODE_PARS_EVEN                (LPUART_MODE_PARS_CFG(0x2))                         /*!< Even parity */
#define LPUART_MODE_PARS_ZERO                (LPUART_MODE_PARS_CFG(0x3))                         /*!< Fixed to 0 parity */
#define LPUART_MODE_PARS_ONE                 (LPUART_MODE_PARS_CFG(0x4))                         /*!< Fixed to 1 parity */
#define LPUART_MODE_PARS_USER_DEFINED        (LPUART_MODE_PARS_CFG(0x5))                         /*!< User-defined parity */

/* LPUART_MODE_STOPS */
#define LPUART_MODE_STOPS_POS                (10U)                                               /*!< LPUART_MODE_STOPS bit offset */
#define LPUART_MODE_STOPS_1                  ((uint32_t)0x00000000U)                             /*!< 1 stop bit. */
#define LPUART_MODE_STOPS_2                  (LPUART_MODE_STOPS)                                 /*!< 2 stop bits. */

/* LPUART_MODE_LMSB */
#define LPUART_MODE_LMSB_POS                 (11U)                                               /*!< LPUART_MODE_LMSB bit offset */

/* LPUART_MODE_ILBE */
#define LPUART_MODE_ILBE_POS                 (12U)                                               /*!< LPUART_MODE_ILBE bit offset */

/* LPUART_MODE_WUSEL */
#define LPUART_MODE_WUSEL_POS                (13U)                                               /*!< LPUART_MODE_WUSEL bit offset */
#define LPUART_MODE_WUSEL_CFG(regval)  (LPUART_MODE_WUSEL & ((uint32_t)(regval) << LPUART_MODE_WUSEL_POS))
#define LPUART_MODE_WUSEL_STARTBIT           (LPUART_MODE_WUSEL_CFG(0x0))                        /*!< Start bit detection */
#define LPUART_MODE_WUSEL_RXFALLINGEDGE      (LPUART_MODE_WUSEL_CFG(0x1))                        /*!< RX falling edge detection */
#define LPUART_MODE_WUSEL_RXCOMPLETE         (LPUART_MODE_WUSEL_CFG(0x2))                        /*!< Data reception complete */
#define LPUART_MODE_WUSEL_FRAMEMATCH         (LPUART_MODE_WUSEL_CFG(0x3))                        /*!< Frame data match successful */

/* LPUART_IE_TXIE */
#define LPUART_IE_TXIE_POS                   (0U)                                                /*!< LPUART_IE_TXIE bit offset */

/* LPUART_IE_RXIE */
#define LPUART_IE_RXIE_POS                   (1U)                                                /*!< LPUART_IE_RXIE bit offset */

/* LPUART_IE_WUFIE */
#define LPUART_IE_WUFIE_POS                  (2U)                                                /*!< LPUART_IE_WUFIE bit offset */

/* LPUART_IE_PEIE */
#define LPUART_IE_PEIE_POS                   (3U)                                                /*!< LPUART_IE_PEIE bit offset */

/* LPUART_IE_OEIE */
#define LPUART_IE_OEIE_POS                   (4U)                                                /*!< LPUART_IE_OEIE bit offset */

/* LPUART_IE_FEIE */
#define LPUART_IE_FEIE_POS                   (5U)                                                /*!< LPUART_IE_FEIE bit offset */

/* LPUART_IE_DEIE */
#define LPUART_IE_DEIE_POS                   (6U)                                                /*!< LPUART_IE_DEIE bit offset */

/* LPUART_IE_NEIE */
#define LPUART_IE_NEIE_POS                   (7U)                                                /*!< LPUART_IE_NEIE bit offset */

/* LPUART_IE_IDLEIE */
#define LPUART_IE_IDLEIE_POS                 (8U)                                                /*!< LPUART_IE_IDLEIE bit offset */

/* LPUART_IE_TXFEIE */
#define LPUART_IE_TXFEIE_POS                 (9U)                                                /*!< LPUART_IE_TXFEIE bit offset */

/* LPUART_IE_TXFHEIE */
#define LPUART_IE_TXFHEIE_POS                (10U)                                               /*!< LPUART_IE_TXFHEIE bit offset */

/* LPUART_IE_RXFNEIE */
#define LPUART_IE_RXFNEIE_POS                (11U)                                               /*!< LPUART_IE_RXFNEIE bit offset */

/* LPUART_IE_RXFHFIE */
#define LPUART_IE_RXFHFIE_POS                (12U)                                               /*!< LPUART_IE_RXFHFIE bit offset */

/* LPUART_IE_RXFFIE */
#define LPUART_IE_RXFFIE_POS                 (13U)                                               /*!< LPUART_IE_RXFFIE bit offset */

/* LPUART_STA_TXIF */
#define LPUART_STA_TXIF_POS                  (0U)                                                /*!< LPUART_STA_TXIF bit offset */

/* LPUART_STA_RXIF */
#define LPUART_STA_RXIF_POS                  (1U)                                                /*!< LPUART_STA_RXIF bit offset */

/* LPUART_STA_WUF */
#define LPUART_STA_WUF_POS                   (2U)                                                /*!< LPUART_STA_WUF bit offset */

/* LPUART_STA_PEIF */
#define LPUART_STA_PEIF_POS                  (3U)                                                /*!< LPUART_STA_PEIF bit offset */

/* LPUART_STA_OEIF */
#define LPUART_STA_OEIF_POS                  (4U)                                                /*!< LPUART_STA_OEIF bit offset */

/* LPUART_STA_FEIF */
#define LPUART_STA_FEIF_POS                  (5U)                                                /*!< LPUART_STA_FEIF bit offset */

/* LPUART_STA_DEIF */
#define LPUART_STA_DEIF_POS                  (6U)                                                /*!< LPUART_STA_DEIF bit offset */

/* LPUART_STA_NEIF */
#define LPUART_STA_NEIF_POS                  (7U)                                                /*!< LPUART_STA_NEIF bit offset */

/* LPUART_STA_IDLEIF */
#define LPUART_STA_IDLEIF_POS                (8U)                                                /*!< LPUART_STA_IDLEIF bit offset */

/* LPUART_STA_TBUSY */
#define LPUART_STA_TBUSY_POS                 (9U)                                                /*!< LPUART_STA_TBUSY bit offset */

/* LPUART_STA_RBUSY */
#define LPUART_STA_RBUSY_POS                 (10U)                                               /*!< LPUART_STA_RBUSY bit offset */

/* LPUART_STA_TXFEIF */
#define LPUART_STA_TXFEIF_POS                (11U)                                               /*!< LPUART_STA_TXFEIF bit offset */

/* LPUART_STA_TXFHEIF */
#define LPUART_STA_TXFHEIF_POS               (12U)                                               /*!< LPUART_STA_TXFHEIF bit offset */

/* LPUART_STA_TXFF */
#define LPUART_STA_TXFF_POS                  (13U)                                               /*!< LPUART_STA_TXFF bit offset */

/* LPUART_STA_RXFNEIF */
#define LPUART_STA_RXFNEIF_POS               (14U)                                               /*!< LPUART_STA_RXFNEIF bit offset */

/* LPUART_STA_RXFHFIF */
#define LPUART_STA_RXFHFIF_POS               (15U)                                               /*!< LPUART_STA_RXFHFIF bit offset */

/* LPUART_STA_RXFFIF */
#define LPUART_STA_RXFFIF_POS                (16U)                                               /*!< LPUART_STA_RXFFIF bit offset */

/* LPUART_STA_TXFCNT */
#define LPUART_STA_TXFCNT_POS                (17U)                                               /*!< LPUART_STA_TXFCNT bit offset */

/* LPUART_STA_RXFCNT */
#define LPUART_STA_RXFCNT_POS                (20U)                                               /*!< LPUART_STA_RXFCNT bit offset */

/* LPUART_BAUD_BAUD */
#define LPUART_BAUD_BAUD_POS                 (0U)                                                /*!< LPUART_BAUD_BAUD bit offset */
#define LPUART_BAUD_BAUD_CFG(regval)  (LPUART_BAUD_BAUD & ((uint32_t)(regval) << LPUART_BAUD_BAUD_POS))
#define LPUART_BAUD_BAUD_9600                (LPUART_BAUD_BAUD_CFG(0x0))                         /*!< Baud Rate is 9600bps */
#define LPUART_BAUD_BAUD_4800                (LPUART_BAUD_BAUD_CFG(0x1))                         /*!< Baud Rate is 4800bps */
#define LPUART_BAUD_BAUD_2400                (LPUART_BAUD_BAUD_CFG(0x2))                         /*!< Baud Rate is 2400bps */
#define LPUART_BAUD_BAUD_1200                (LPUART_BAUD_BAUD_CFG(0x3))                         /*!< Baud Rate is 1200bps */
#define LPUART_BAUD_BAUD_600                 (LPUART_BAUD_BAUD_CFG(0x4))                         /*!< Baud Rate is 600bps */
#define LPUART_BAUD_BAUD_300                 (LPUART_BAUD_BAUD_CFG(0x5))                         /*!< Baud Rate is 300bps */

/* LPUART_BAUD_MCTL */
#define LPUART_BAUD_MCTL_POS                 (16U)                                               /*!< LPUART_BAUD_MCTL bit offset */
#define LPUART_BAUD_MCTL_CFG(regval)  (LPUART_BAUD_MCTL & ((uint32_t)(regval) << LPUART_BAUD_MCTL_POS))
#define LPUART_BAUD_MCTL_9600                (LPUART_BAUD_MCTL_CFG(0x4A5))                       /*!< Baud Rate is 9600bps, set MCTL */
#define LPUART_BAUD_MCTL_4800                (LPUART_BAUD_MCTL_CFG(0xF7D))                       /*!< Baud Rate is 4800bps, set MCTL */
#define LPUART_BAUD_MCTL_2400                (LPUART_BAUD_MCTL_CFG(0x6DB))                       /*!< Baud Rate is 2400bps, set MCTL */
#define LPUART_BAUD_MCTL_1200                (LPUART_BAUD_MCTL_CFG(0x407))                       /*!< Baud Rate is 1200bps, set MCTL */
#define LPUART_BAUD_MCTL_600                 (LPUART_BAUD_MCTL_CFG(0x7F))                        /*!< Baud Rate is 600bps, set MCTL */
#define LPUART_BAUD_MCTL_300                 (LPUART_BAUD_MCTL_CFG(0x803))                       /*!< Baud Rate is 300bps, set MCTL */

/* LPUART_TXDATA_TXDATA */
#define LPUART_TXDATA_TXDATA_POS             (0U)                                                /*!< LPUART_TXDATA_TXDATA bit offset */

/* LPUART_TXDATA_UP */
#define LPUART_TXDATA_UP_POS                 (8U)                                                /*!< LPUART_TXDATA_UP bit offset */

/* LPUART_RXDATA_RXDATA */
#define LPUART_RXDATA_RXDATA_POS             (0U)                                                /*!< LPUART_RXDATA_RXDATA bit offset */

/* LPUART_RXDATA_UP */
#define LPUART_RXDATA_UP_POS                 (8U)                                                /*!< LPUART_RXDATA_UP bit offset */

/* LPUART_DMR_MDATA */
#define LPUART_DMR_MDATA_POS                 (0U)                                                /*!< LPUART_DMR_MDATA bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_LPUART_H */
