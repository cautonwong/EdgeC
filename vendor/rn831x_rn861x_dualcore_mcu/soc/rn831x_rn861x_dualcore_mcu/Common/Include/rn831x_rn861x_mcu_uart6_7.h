/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_uart6_7.h
 * @author  Renergy Software Team
 * @brief   UART6_7 module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_UART6_7_H
#define _RN831x_RN861x_MCU_UART6_7_H

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

/* UART6_7 */
typedef struct
{
    __IO uint32_t CTRL;                      /*!< offset: 0x000 info: Control Register */
    __IO uint32_t BAUD;                      /*!< offset: 0x004 info: Baud Rate Register */
    __IO uint32_t STA;                       /*!< offset: 0x008 info: Status Register */
    __IO uint32_t TXDATA;                    /*!< offset: 0x00C info: Transmit Data Register */
    __IO uint32_t RXDATA;                    /*!< offset: 0x010 info: Receive Data Register */
    __IO uint32_t FDIV;                      /*!< offset: 0x014 info: Fractional Divider Register */
    __IO uint32_t DMA_CTRL;                  /*!< offset: 0x018 info: DMA Control Register */
    __IO uint32_t DMA_TBADR;                 /*!< offset: 0x01C info: DMA Transmit Base Address Register */
    __IO uint32_t DMA_RBADR;                 /*!< offset: 0x020 info: Receive Base Address Register */
    __IO uint32_t DMA_TLEN;                  /*!< offset: 0x024 info: DMA Transmit Length Register */
    __IO uint32_t DMA_RLEN;                  /*!< offset: 0x028 info: DMA Receive Length Register */
    __IO uint32_t DMA_TADR;                  /*!< offset: 0x02C info: DMA Current Transmit Address Register */
    __IO uint32_t DMA_RADR;                  /*!< offset: 0x030 info: DMA Current Receive Address Register */
    __IO uint32_t DMA_IE;                    /*!< offset: 0x034 info: DMA Interrupt Enable Register */
    __IO uint32_t DMA_IF;                    /*!< offset: 0x038 info: DMA Interrupt Flag Register */
    __IO uint32_t RX_TO;                     /*!< offset: 0x03C info: Receive Timeout Configuration Register */
    __IO uint32_t CF_CTRL;                   /*!< offset: 0x040 info: UART_CF CF Control Register */
    __IO uint32_t PFDAT;                     /*!< offset: 0x044 info: UART_CF PF Pulse Data Register */
    __IO uint32_t QFDAT;                     /*!< offset: 0x048 info: UART_CF QF Pulse Data Register */
    __IO uint32_t FPFDAT;                    /*!< offset: 0x04C info: UART_CF FPF Pulse Data Register */
    __IO uint32_t IRQDAT;                    /*!< offset: 0x050 info: UART_CF Interrupt Data Register */
    __IO uint32_t CF_OV;                     /*!< offset: 0x054 info: UART_CF Timeout Control Register, Configurable only when UART_CF_EN=0. */
    __IO uint32_t TW_CTRL;                   /*!< offset: 0x058 info: UART_TW Control Register */
    __IO uint32_t TW_BAUD;                   /*!< offset: 0x05C info: UART_TW Baud Rate Register */
    __IO uint32_t TW_EN;                     /*!< offset: 0x060 info: UART_TW Enable Register */
    __IO uint32_t CAL_CTRL;                  /*!< offset: 0x064 info: UART Calibration Control Register (Low-level Length Counting) */
    __IO uint32_t CAL_CNT;                   /*!< offset: 0x068 info: ART Calibration Count Register */
    __IO uint32_t TX_DRLAY;                  /*!< offset: 0x06C info: Transmit Delay Configuration Register (UART6-7) */
} UART6_7_TypeDef;

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
#define UART6_CORE0_BASE (0x20018000U)
#define UART7_CORE0_BASE (0x2001C000U)
#define UART6_CORE1_BASE (0xA0018000U)
#define UART7_CORE1_BASE (0xA001C000U)
#define UART6_CORE0_BB_BASE (0x22300000U)
#define UART7_CORE0_BB_BASE (0x22380000U)
#define UART6_CORE1_BB_BASE (0xA2300000U)
#define UART7_CORE1_BB_BASE (0xA2380000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define UART6_CORE0 ((UART6_7_TypeDef *)  UART6_CORE0_BASE)
#define UART7_CORE0 ((UART6_7_TypeDef *)  UART7_CORE0_BASE)
#define UART6_CORE1 ((UART6_7_TypeDef *)  UART6_CORE1_BASE)
#define UART7_CORE1 ((UART6_7_TypeDef *)  UART7_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define UART6  UART6_CORE0
#define UART7  UART7_CORE0
#endif
#if defined(CORE1)
#define UART6  UART6_CORE1
#define UART7  UART7_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** UART6_7 Instances *******************************/
#define IS_UART6_7_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == UART6_CORE0) || \
                                              ((INSTANCE) == UART7_CORE0) || \
                                              ((INSTANCE) == UART6_CORE1) || \
                                              ((INSTANCE) == UART7_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* UART6_7_CTRL */
#define UART6_7_CTRL_EN                      (BIT(0))                                            /*!< Module Enable */
#define UART6_7_CTRL_TXIE                    (BIT(1))                                            /*!< Transmit Data Interrupt Enable */
#define UART6_7_CTRL_RXIE                    (BIT(2))                                            /*!< Receive Data Interrupt Enable */
#define UART6_7_CTRL_ERRIE                   (BIT(3))                                            /*!< Error Interrupt Enable */
#define UART6_7_CTRL_STOPS                   (BIT(4))                                            /*!< Stop Bit Length */
#define UART6_7_CTRL_DATLEN                  (BITS(5,6))                                         /*!< Data Length */
#define UART6_7_CTRL_PARS                    (BITS(7,9))                                         /*!< Parity Selection */
#define UART6_7_CTRL_IRE                     (BIT(10))                                           /*!< Infrared Modulation Enable */
#define UART6_7_CTRL_ILBE                    (BIT(11))                                           /*!< Internal Loopback Enable */
#define UART6_7_CTRL_IRSEL                   (BIT(12))                                           /*!< Infrared modulation polarity selection */
#define UART6_7_CTRL_LMSB                    (BIT(13))                                           /*!< LSB/MSB transmission order */
#define UART6_7_CTRL_NEG                     (BIT(14))                                           /*!< Data polarity selection */
#define UART6_7_CTRL_PF_IE                   (BIT(15))                                           /*!< Interrupt Enable for receiving PF pulse */
#define UART6_7_CTRL_QF_IE                   (BIT(16))                                           /*!< Interrupt Enable for receiving QF pulse */
#define UART6_7_CTRL_FPF_IE                  (BIT(17))                                           /*!< Interrupt Enable for receiving FPF pulse */
#define UART6_7_CTRL_CF_ERR_IE               (BIT(18))                                           /*!< Interrupt Enable for UART_CF mode timeout error */
#define UART6_7_CTRL_TW_IE                   (BIT(19))                                           /*!< Interrupt Enable for low-level signal transmission completion */
#define UART6_7_CTRL_CAL_IE                  (BIT(20))                                           /*!< Interrupt Enable for signal measurement completion */
#define UART6_7_CTRL_RX_IRQ_IE               (BIT(21))                                           /*!< Interrupt Enable for receiving interrupt data (IRQ) */
#define UART6_7_CTRL_RX_TO_IE                (BIT(22))                                           /*!< Receive Timeout Interrupt Enable */
#define UART6_7_CTRL_PAR_MEM                 (BIT(23))                                           /*!< Memory storage configuration on parity error */

/* UART6_7_BAUD */
#define UART6_7_BAUD_CLKDIV                  (BITS(0,13))                                        /*!< Clock divider value for UARTx. */

/* UART6_7_STA */
#define UART6_7_STA_RX                       (BIT(0))                                            /*!< Reception Complete Flag */
#define UART6_7_STA_TX                       (BIT(1))                                            /*!< Transmission Complete Flag */
#define UART6_7_STA_PE                       (BIT(2))                                            /*!< Parity Error Flag */
#define UART6_7_STA_OE                       (BIT(3))                                            /*!< Overrun Error Flag */
#define UART6_7_STA_FE                       (BIT(4))                                            /*!< Frame Error Flag */
#define UART6_7_STA_DE                       (BIT(5))                                            /*!< Data Error Flag */
#define UART6_7_STA_RB                       (BIT(6))                                            /*!< Reception Busy Flag */
#define UART6_7_STA_TB                       (BIT(7))                                            /*!< Transmission Busy Flag */
#define UART6_7_STA_TX_FIFO_EMPTY            (BIT(8))                                            /*!< Transmit FIFO Empty */
#define UART6_7_STA_TX_FIFO_FULL             (BIT(9))                                            /*!< Transmit FIFO Full */
#define UART6_7_STA_RX_TO                    (BIT(10))                                           /*!< Receive Timeout Flag */
#define UART6_7_STA_PF_IF                    (BIT(15))                                           /*!< Interrupt Flag for receiving PF pulse */
#define UART6_7_STA_QF_IF                    (BIT(16))                                           /*!< Interrupt Flag for receiving QF pulse */
#define UART6_7_STA_FPF_IF                   (BIT(17))                                           /*!< Interrupt Flag for receiving FPF pulse */
#define UART6_7_STA_CF_ERR_IF                (BIT(18))                                           /*!< Interrupt Flag for UART_CF mode timeout error */
#define UART6_7_STA_TW_IF                    (BIT(19))                                           /*!< Interrupt Flag for low-level signal transmission completion (TW) */
#define UART6_7_STA_CAL_IF                   (BIT(20))                                           /*!< Interrupt Flag for signal measurement completion */
#define UART6_7_STA_RX_IRQ_IF                (BIT(21))                                           /*!< Interrupt Flag for receiving interrupt data (IRQ) */

/* UART6_7_TXDATA */
#define UART6_7_TXDATA_TXDATA                (BITS(0,7))                                         /*!< Transmit Data */
#define UART6_7_TXDATA_UP                    (BIT(8))                                            /*!< User-defined Parity Bit (used when parity is set to user-defined) */

/* UART6_7_RXDATA */
#define UART6_7_RXDATA_RXDATA                (BITS(0,7))                                         /*!< Receive Data */
#define UART6_7_RXDATA_UP                    (BIT(8))                                            /*!< Parity Bit (received).User-defined Parity Bit (used when parity is set to user-defined) */

/* UART6_7_FDIV */
#define UART6_7_FDIV_FDIV                    (BITS(0,13))                                        /*!< Fractional divider coefficient. */

/* UART6_7_DMA_CTRL */
#define UART6_7_DMA_CTRL_TX_DMA_EN           (BIT(0))                                            /*!< Transmit DMA Enable */
#define UART6_7_DMA_CTRL_RX_DMA_EN           (BIT(1))                                            /*!< Receive DMA Enable */
#define UART6_7_DMA_CTRL_TX_CYC_MODE         (BIT(2))                                            /*!< Transmit Cycle Mode Enable */
#define UART6_7_DMA_CTRL_RX_CYC_MODE         (BIT(3))                                            /*!< Receive Cycle Mode Enable */
#define UART6_7_DMA_CTRL_DMA_PASS            (BIT(4))                                            /*!< DMA Parity Check Bit */

/* UART6_7_DMA_TBADR */
#define UART6_7_DMA_TBADR_DMA_TBADR          (BITS(0,17))                                        /*!< DMA Transmit Base Address (byte address). The lower two bits must be configured according to the data width. */

/* UART6_7_DMA_RBADR */
#define UART6_7_DMA_RBADR_DMA_RBADR          (BITS(0,17))                                        /*!< DMA Receive Base Address (byte address). The lower two bits must be configured according to the data width. */

/* UART6_7_DMA_TLEN */
#define UART6_7_DMA_TLEN_DMA_TLEN            (BITS(0,17))                                        /*!< DMA Transmit Length (in bytes). The lower two bits must be configured according to the data width. */

/* UART6_7_DMA_RLEN */
#define UART6_7_DMA_RLEN_DMA_RLEN            (BITS(0,17))                                        /*!< DMA Receive Length (in bytes). The lower two bits must be configured according to the data width. */

/* UART6_7_DMA_TADR */
#define UART6_7_DMA_TADR_DMA_TADR            (BITS(0,17))                                        /*!< Current DMA Transmit Address (byte address). */

/* UART6_7_DMA_RADR */
#define UART6_7_DMA_RADR_DMA_RADR            (BITS(0,17))                                        /*!< Current DMA Receive Address (byte address). */

/* UART6_7_DMA_IE */
#define UART6_7_DMA_IE_TX_HIE                (BIT(0))                                            /*!< DMA Transmit Half Empty Interrupt Enable */
#define UART6_7_DMA_IE_TX_FIE                (BIT(1))                                            /*!< DMA Transmit Empty (Full) Interrupt Enable */
#define UART6_7_DMA_IE_RX_HIE                (BIT(2))                                            /*!< DMA Receive Half Full Interrupt Enable */
#define UART6_7_DMA_IE_RX_FIE                (BIT(3))                                            /*!< DMA Receive Full Interrupt Enable */
#define UART6_7_DMA_IE_TX_ERR_IE             (BIT(4))                                            /*!< Transmit Data Error Interrupt Enable */
#define UART6_7_DMA_IE_RX_ERR_IE             (BIT(5))                                            /*!< Receive Data Error Interrupt Enable */
#define UART6_7_DMA_IE_RX_DONE_IE            (BIT(6))                                            /*!< DMA Receive Timeout Interrupt Enable */

/* UART6_7_DMA_IF */
#define UART6_7_DMA_IF_TX_HDONE              (BIT(0))                                            /*!< DMA Transmit Half Empty Interrupt Flag */
#define UART6_7_DMA_IF_TX_FDONE              (BIT(1))                                            /*!< DMA Transmit Full Empty Interrupt Flag */
#define UART6_7_DMA_IF_RX_HDONE              (BIT(2))                                            /*!< DMA Receive Half Full Interrupt Flag */
#define UART6_7_DMA_IF_RX_FDONE              (BIT(3))                                            /*!< DMA Receive Full Interrupt Flag */
#define UART6_7_DMA_IF_RX_ERR                (BIT(5))                                            /*!< Receive Data Error Flag */
#define UART6_7_DMA_IF_RX_DONE               (BIT(6))                                            /*!< DMA Receive Complete Flag */

/* UART6_7_RX_TO */
#define UART6_7_RX_TO_TIMEOUT_CNT            (BITS(0,7))                                         /*!< Timeout value in terms of UART data bit widths. After the start bit is received, if no data is received for this duration, a timeout occurs. */

/* UART6_7_CF_CTRL */
#define UART6_7_CF_CTRL_UART_CF_EN           (BIT(0))                                            /*!< UART_CF Mode Enable */
#define UART6_7_CF_CTRL_IRQ_WR_EN            (BIT(1))                                            /*!< Interrupt Data Write to SRAM Enable in UART_CF mode (configurable when UART_CF_EN=0) */
#define UART6_7_CF_CTRL_PULSE_OE             (BIT(2))                                            /*!< Pulse Output Enable in UART_CF mode (configurable when UART_CF_EN=0) */

/* UART6_7_PFDAT */
#define UART6_7_PFDAT_PFDAT                  (BITS(0,31))                                        /*!< PF pulse sequence value for UART to recognize. */

/* UART6_7_QFDAT */
#define UART6_7_QFDAT_QFDAT                  (BITS(0,31))                                        /*!< QF pulse sequence value for UART to recognize. */

/* UART6_7_FPFDAT */
#define UART6_7_FPFDAT_FPFDAT                (BITS(0,31))                                        /*!< FPF pulse sequence value for UART to recognize. */

/* UART6_7_IRQDAT */
#define UART6_7_IRQDAT_IRQDAT                (BITS(0,31))                                        /*!< Interrupt data value for UART to recognize. */

/* UART6_7_CF_OV */
#define UART6_7_CF_OV_CF_OV_TIME             (BITS(0,1))                                         /*!< UART_CF Timeout Configuration (calculated at 894k baud) */
#define UART6_7_CF_OV_OV_BUF_CLR             (BIT(2))                                            /*!< Timeout Buffer Clear Enable */

/* UART6_7_TW_CTRL */
#define UART6_7_TW_CTRL_UART_TW_MODE         (BIT(0))                                            /*!< UART_TW Mode Enable */
#define UART6_7_TW_CTRL_TW_NUM               (BITS(1,4))                                         /*!< Number of low-level pulses to send = (TW_NUM + 1) */

/* UART6_7_TW_BAUD */
#define UART6_7_TW_BAUD_TW_BAUD              (BITS(0,13))                                        /*!< TW mode baud rate configuration for each bit length. Default value is for 14M system clock and 4800 baud. */

/* UART6_7_TW_EN */
#define UART6_7_TW_EN_TW_EN                  (BITS(0,1))                                         /*!< TW_KICK */

/* UART6_7_CAL_CTRL */
#define UART6_7_CAL_CTRL_UART_CAL_EN         (BIT(0))                                            /*!< Calibration Enable */
#define UART6_7_CAL_CTRL_CAL_MODE            (BIT(1))                                            /*!< Measurement Mode */
#define UART6_7_CAL_CTRL_POS_START           (BIT(2))                                            /*!< Measurement Start Point */
#define UART6_7_CAL_CTRL_POS_END             (BIT(3))                                            /*!< Measurement End Point */
#define UART6_7_CAL_CTRL_CAL_DATA            (BITS(4,12))                                        /*!< Data value for fixed data measurement mode. The measurement register is updated only when this data is received */

/* UART6_7_CAL_CNT */
#define UART6_7_CAL_CNT_CAL_CNT              (BITS(0,31))                                        /*!< Measurement result: Continuous mode: Updated after each low-level measurement. Fixed data mode: Updated after receiving and measuring the configured low-level length. */

/* UART6_7_TX_DRLAY */
#define UART6_7_TX_DRLAY_TIMEOUT_CNT         (BITS(0,1))                                         /*!< Delay Insertion. Effective when TX_DELAY_EN=1 */
#define UART6_7_TX_DRLAY_TX_DELAY_EN         (BIT(2))                                            /*!< Transmit Delay Enable */

/* constants definitions */
/* UART6_7_CTRL_EN */
#define UART6_7_CTRL_EN_POS                  (0U)                                                /*!< UART6_7_CTRL_EN bit offset */

/* UART6_7_CTRL_TXIE */
#define UART6_7_CTRL_TXIE_POS                (1U)                                                /*!< UART6_7_CTRL_TXIE bit offset */

/* UART6_7_CTRL_RXIE */
#define UART6_7_CTRL_RXIE_POS                (2U)                                                /*!< UART6_7_CTRL_RXIE bit offset */

/* UART6_7_CTRL_ERRIE */
#define UART6_7_CTRL_ERRIE_POS               (3U)                                                /*!< UART6_7_CTRL_ERRIE bit offset */

/* UART6_7_CTRL_STOPS */
#define UART6_7_CTRL_STOPS_POS               (4U)                                                /*!< UART6_7_CTRL_STOPS bit offset */
#define UART6_7_CTRL_STOPS_1                 ((uint32_t)0x00000000U)                             /*!< 1 stop bit. */
#define UART6_7_CTRL_STOPS_2                 (UART6_7_CTRL_STOPS)                                /*!< 2 stop bits. */

/* UART6_7_CTRL_DATLEN */
#define UART6_7_CTRL_DATLEN_POS              (5U)                                                /*!< UART6_7_CTRL_DATLEN bit offset */
#define UART6_7_CTRL_DATLEN_CFG(regval)  (UART6_7_CTRL_DATLEN & ((uint32_t)(regval) << UART6_7_CTRL_DATLEN_POS))
#define UART6_7_CTRL_DATLEN_5B               (UART6_7_CTRL_DATLEN_CFG(0x0))                      /*!< 5 data bits */
#define UART6_7_CTRL_DATLEN_6B               (UART6_7_CTRL_DATLEN_CFG(0x1))                      /*!< 6 data bits */
#define UART6_7_CTRL_DATLEN_7B               (UART6_7_CTRL_DATLEN_CFG(0x2))                      /*!< 7 data bits */
#define UART6_7_CTRL_DATLEN_8B               (UART6_7_CTRL_DATLEN_CFG(0x3))                      /*!< 8 data bits */

/* UART6_7_CTRL_PARS */
#define UART6_7_CTRL_PARS_POS                (7U)                                                /*!< UART6_7_CTRL_PARS bit offset */
#define UART6_7_CTRL_PARS_CFG(regval)  (UART6_7_CTRL_PARS & ((uint32_t)(regval) << UART6_7_CTRL_PARS_POS))
#define UART6_7_CTRL_PARS_NONE               (UART6_7_CTRL_PARS_CFG(0x0))                        /*!< No parity */
#define UART6_7_CTRL_PARS_ODD                (UART6_7_CTRL_PARS_CFG(0x1))                        /*!< Odd parity */
#define UART6_7_CTRL_PARS_EVEN               (UART6_7_CTRL_PARS_CFG(0x2))                        /*!< Even parity */
#define UART6_7_CTRL_PARS_ZERO               (UART6_7_CTRL_PARS_CFG(0x3))                        /*!< Fixed to 0 parity */
#define UART6_7_CTRL_PARS_ONE                (UART6_7_CTRL_PARS_CFG(0x4))                        /*!< Fixed to 1 parity */
#define UART6_7_CTRL_PARS_USER_DEFINED       (UART6_7_CTRL_PARS_CFG(0x5))                        /*!< User-defined parity */

/* UART6_7_CTRL_IRE */
#define UART6_7_CTRL_IRE_POS                 (10U)                                               /*!< UART6_7_CTRL_IRE bit offset */

/* UART6_7_CTRL_ILBE */
#define UART6_7_CTRL_ILBE_POS                (11U)                                               /*!< UART6_7_CTRL_ILBE bit offset */

/* UART6_7_CTRL_IRSEL */
#define UART6_7_CTRL_IRSEL_POS               (12U)                                               /*!< UART6_7_CTRL_IRSEL bit offset */
#define UART6_7_CTRL_IRSEL_POSITIVE          ((uint32_t)0x00000000U)                             /*!< Positive polarity (default driven level is high, low level is modulated). */
#define UART6_7_CTRL_IRSEL_NEGATIVE          (UART6_7_CTRL_IRSEL)                                /*!< Negative polarity (default driven level is low, high level is modulated). */

/* UART6_7_CTRL_LMSB */
#define UART6_7_CTRL_LMSB_POS                (13U)                                               /*!< UART6_7_CTRL_LMSB bit offset */
#define UART6_7_CTRL_LMSB_LSB_FIRST          ((uint32_t)0x00000000U)                             /*!< LSB first */
#define UART6_7_CTRL_LMSB_MSB_FIRST          (UART6_7_CTRL_LMSB)                                 /*!< MSB first */

/* UART6_7_CTRL_NEG */
#define UART6_7_CTRL_NEG_POS                 (14U)                                               /*!< UART6_7_CTRL_NEG bit offset */
#define UART6_7_CTRL_NEG_POSITIVE            ((uint32_t)0x00000000U)                             /*!< Positive polarity (default driven level is high, data is not inverted). */
#define UART6_7_CTRL_NEG_NEGATIVE            (UART6_7_CTRL_NEG)                                  /*!< Negative polarity (default driven level is low, data is inverted). */

/* UART6_7_CTRL_PF_IE */
#define UART6_7_CTRL_PF_IE_POS               (15U)                                               /*!< UART6_7_CTRL_PF_IE bit offset */

/* UART6_7_CTRL_QF_IE */
#define UART6_7_CTRL_QF_IE_POS               (16U)                                               /*!< UART6_7_CTRL_QF_IE bit offset */

/* UART6_7_CTRL_FPF_IE */
#define UART6_7_CTRL_FPF_IE_POS              (17U)                                               /*!< UART6_7_CTRL_FPF_IE bit offset */

/* UART6_7_CTRL_CF_ERR_IE */
#define UART6_7_CTRL_CF_ERR_IE_POS           (18U)                                               /*!< UART6_7_CTRL_CF_ERR_IE bit offset */

/* UART6_7_CTRL_TW_IE */
#define UART6_7_CTRL_TW_IE_POS               (19U)                                               /*!< UART6_7_CTRL_TW_IE bit offset */

/* UART6_7_CTRL_CAL_IE */
#define UART6_7_CTRL_CAL_IE_POS              (20U)                                               /*!< UART6_7_CTRL_CAL_IE bit offset */

/* UART6_7_CTRL_RX_IRQ_IE */
#define UART6_7_CTRL_RX_IRQ_IE_POS           (21U)                                               /*!< UART6_7_CTRL_RX_IRQ_IE bit offset */

/* UART6_7_CTRL_RX_TO_IE */
#define UART6_7_CTRL_RX_TO_IE_POS            (22U)                                               /*!< UART6_7_CTRL_RX_TO_IE bit offset */

/* UART6_7_CTRL_PAR_MEM */
#define UART6_7_CTRL_PAR_MEM_POS             (23U)                                               /*!< UART6_7_CTRL_PAR_MEM bit offset */
#define UART6_7_CTRL_PAR_MEM_STORE_MEMORY    ((uint32_t)0x00000000U)                             /*!< Data is stored in memory regardless of parity error. */
#define UART6_7_CTRL_PAR_MEM_NOT_STORE_MEMORY  (UART6_7_CTRL_PAR_MEM)                              /*!< When parity error occurs, the current frame is not stored in memory. */

/* UART6_7_BAUD_CLKDIV */
#define UART6_7_BAUD_CLKDIV_POS              (0U)                                                /*!< UART6_7_BAUD_CLKDIV bit offset */

/* UART6_7_STA_RX */
#define UART6_7_STA_RX_POS                   (0U)                                                /*!< UART6_7_STA_RX bit offset */

/* UART6_7_STA_TX */
#define UART6_7_STA_TX_POS                   (1U)                                                /*!< UART6_7_STA_TX bit offset */

/* UART6_7_STA_PE */
#define UART6_7_STA_PE_POS                   (2U)                                                /*!< UART6_7_STA_PE bit offset */

/* UART6_7_STA_OE */
#define UART6_7_STA_OE_POS                   (3U)                                                /*!< UART6_7_STA_OE bit offset */

/* UART6_7_STA_FE */
#define UART6_7_STA_FE_POS                   (4U)                                                /*!< UART6_7_STA_FE bit offset */

/* UART6_7_STA_DE */
#define UART6_7_STA_DE_POS                   (5U)                                                /*!< UART6_7_STA_DE bit offset */

/* UART6_7_STA_RB */
#define UART6_7_STA_RB_POS                   (6U)                                                /*!< UART6_7_STA_RB bit offset */

/* UART6_7_STA_TB */
#define UART6_7_STA_TB_POS                   (7U)                                                /*!< UART6_7_STA_TB bit offset */

/* UART6_7_STA_TX_FIFO_EMPTY */
#define UART6_7_STA_TX_FIFO_EMPTY_POS        (8U)                                                /*!< UART6_7_STA_TX_FIFO_EMPTY bit offset */

/* UART6_7_STA_TX_FIFO_FULL */
#define UART6_7_STA_TX_FIFO_FULL_POS         (9U)                                                /*!< UART6_7_STA_TX_FIFO_FULL bit offset */

/* UART6_7_STA_RX_TO */
#define UART6_7_STA_RX_TO_POS                (10U)                                               /*!< UART6_7_STA_RX_TO bit offset */

/* UART6_7_STA_PF_IF */
#define UART6_7_STA_PF_IF_POS                (15U)                                               /*!< UART6_7_STA_PF_IF bit offset */

/* UART6_7_STA_QF_IF */
#define UART6_7_STA_QF_IF_POS                (16U)                                               /*!< UART6_7_STA_QF_IF bit offset */

/* UART6_7_STA_FPF_IF */
#define UART6_7_STA_FPF_IF_POS               (17U)                                               /*!< UART6_7_STA_FPF_IF bit offset */

/* UART6_7_STA_CF_ERR_IF */
#define UART6_7_STA_CF_ERR_IF_POS            (18U)                                               /*!< UART6_7_STA_CF_ERR_IF bit offset */

/* UART6_7_STA_TW_IF */
#define UART6_7_STA_TW_IF_POS                (19U)                                               /*!< UART6_7_STA_TW_IF bit offset */

/* UART6_7_STA_CAL_IF */
#define UART6_7_STA_CAL_IF_POS               (20U)                                               /*!< UART6_7_STA_CAL_IF bit offset */

/* UART6_7_STA_RX_IRQ_IF */
#define UART6_7_STA_RX_IRQ_IF_POS            (21U)                                               /*!< UART6_7_STA_RX_IRQ_IF bit offset */

/* UART6_7_TXDATA_TXDATA */
#define UART6_7_TXDATA_TXDATA_POS            (0U)                                                /*!< UART6_7_TXDATA_TXDATA bit offset */

/* UART6_7_TXDATA_UP */
#define UART6_7_TXDATA_UP_POS                (8U)                                                /*!< UART6_7_TXDATA_UP bit offset */

/* UART6_7_RXDATA_RXDATA */
#define UART6_7_RXDATA_RXDATA_POS            (0U)                                                /*!< UART6_7_RXDATA_RXDATA bit offset */

/* UART6_7_RXDATA_UP */
#define UART6_7_RXDATA_UP_POS                (8U)                                                /*!< UART6_7_RXDATA_UP bit offset */

/* UART6_7_FDIV_FDIV */
#define UART6_7_FDIV_FDIV_POS                (0U)                                                /*!< UART6_7_FDIV_FDIV bit offset */

/* UART6_7_DMA_CTRL_TX_DMA_EN */
#define UART6_7_DMA_CTRL_TX_DMA_EN_POS       (0U)                                                /*!< UART6_7_DMA_CTRL_TX_DMA_EN bit offset */

/* UART6_7_DMA_CTRL_RX_DMA_EN */
#define UART6_7_DMA_CTRL_RX_DMA_EN_POS       (1U)                                                /*!< UART6_7_DMA_CTRL_RX_DMA_EN bit offset */

/* UART6_7_DMA_CTRL_TX_CYC_MODE */
#define UART6_7_DMA_CTRL_TX_CYC_MODE_POS     (2U)                                                /*!< UART6_7_DMA_CTRL_TX_CYC_MODE bit offset */

/* UART6_7_DMA_CTRL_RX_CYC_MODE */
#define UART6_7_DMA_CTRL_RX_CYC_MODE_POS     (3U)                                                /*!< UART6_7_DMA_CTRL_RX_CYC_MODE bit offset */

/* UART6_7_DMA_CTRL_DMA_PASS */
#define UART6_7_DMA_CTRL_DMA_PASS_POS        (4U)                                                /*!< UART6_7_DMA_CTRL_DMA_PASS bit offset */
#define UART6_7_DMA_CTRL_DMA_PASS_ZERO       ((uint32_t)0x00000000U)                             /*!< DMA Transmit Fixed to 0 parity */
#define UART6_7_DMA_CTRL_DMA_PASS_ONE        (UART6_7_DMA_CTRL_DMA_PASS)                         /*!< DMA Transmit Fixed to 1 parity */

/* UART6_7_DMA_TBADR_DMA_TBADR */
#define UART6_7_DMA_TBADR_DMA_TBADR_POS      (0U)                                                /*!< UART6_7_DMA_TBADR_DMA_TBADR bit offset */

/* UART6_7_DMA_RBADR_DMA_RBADR */
#define UART6_7_DMA_RBADR_DMA_RBADR_POS      (0U)                                                /*!< UART6_7_DMA_RBADR_DMA_RBADR bit offset */

/* UART6_7_DMA_TLEN_DMA_TLEN */
#define UART6_7_DMA_TLEN_DMA_TLEN_POS        (0U)                                                /*!< UART6_7_DMA_TLEN_DMA_TLEN bit offset */

/* UART6_7_DMA_RLEN_DMA_RLEN */
#define UART6_7_DMA_RLEN_DMA_RLEN_POS        (0U)                                                /*!< UART6_7_DMA_RLEN_DMA_RLEN bit offset */

/* UART6_7_DMA_TADR_DMA_TADR */
#define UART6_7_DMA_TADR_DMA_TADR_POS        (0U)                                                /*!< UART6_7_DMA_TADR_DMA_TADR bit offset */

/* UART6_7_DMA_RADR_DMA_RADR */
#define UART6_7_DMA_RADR_DMA_RADR_POS        (0U)                                                /*!< UART6_7_DMA_RADR_DMA_RADR bit offset */

/* UART6_7_DMA_IE_TX_HIE */
#define UART6_7_DMA_IE_TX_HIE_POS            (0U)                                                /*!< UART6_7_DMA_IE_TX_HIE bit offset */

/* UART6_7_DMA_IE_TX_FIE */
#define UART6_7_DMA_IE_TX_FIE_POS            (1U)                                                /*!< UART6_7_DMA_IE_TX_FIE bit offset */

/* UART6_7_DMA_IE_RX_HIE */
#define UART6_7_DMA_IE_RX_HIE_POS            (2U)                                                /*!< UART6_7_DMA_IE_RX_HIE bit offset */

/* UART6_7_DMA_IE_RX_FIE */
#define UART6_7_DMA_IE_RX_FIE_POS            (3U)                                                /*!< UART6_7_DMA_IE_RX_FIE bit offset */

/* UART6_7_DMA_IE_TX_ERR_IE */
#define UART6_7_DMA_IE_TX_ERR_IE_POS         (4U)                                                /*!< UART6_7_DMA_IE_TX_ERR_IE bit offset */

/* UART6_7_DMA_IE_RX_ERR_IE */
#define UART6_7_DMA_IE_RX_ERR_IE_POS         (5U)                                                /*!< UART6_7_DMA_IE_RX_ERR_IE bit offset */

/* UART6_7_DMA_IE_RX_DONE_IE */
#define UART6_7_DMA_IE_RX_DONE_IE_POS        (6U)                                                /*!< UART6_7_DMA_IE_RX_DONE_IE bit offset */

/* UART6_7_DMA_IF_TX_HDONE */
#define UART6_7_DMA_IF_TX_HDONE_POS          (0U)                                                /*!< UART6_7_DMA_IF_TX_HDONE bit offset */

/* UART6_7_DMA_IF_TX_FDONE */
#define UART6_7_DMA_IF_TX_FDONE_POS          (1U)                                                /*!< UART6_7_DMA_IF_TX_FDONE bit offset */

/* UART6_7_DMA_IF_RX_HDONE */
#define UART6_7_DMA_IF_RX_HDONE_POS          (2U)                                                /*!< UART6_7_DMA_IF_RX_HDONE bit offset */

/* UART6_7_DMA_IF_RX_FDONE */
#define UART6_7_DMA_IF_RX_FDONE_POS          (3U)                                                /*!< UART6_7_DMA_IF_RX_FDONE bit offset */

/* UART6_7_DMA_IF_RX_ERR */
#define UART6_7_DMA_IF_RX_ERR_POS            (5U)                                                /*!< UART6_7_DMA_IF_RX_ERR bit offset */

/* UART6_7_DMA_IF_RX_DONE */
#define UART6_7_DMA_IF_RX_DONE_POS           (6U)                                                /*!< UART6_7_DMA_IF_RX_DONE bit offset */

/* UART6_7_RX_TO_TIMEOUT_CNT */
#define UART6_7_RX_TO_TIMEOUT_CNT_POS        (0U)                                                /*!< UART6_7_RX_TO_TIMEOUT_CNT bit offset */

/* UART6_7_CF_CTRL_UART_CF_EN */
#define UART6_7_CF_CTRL_UART_CF_EN_POS       (0U)                                                /*!< UART6_7_CF_CTRL_UART_CF_EN bit offset */

/* UART6_7_CF_CTRL_IRQ_WR_EN */
#define UART6_7_CF_CTRL_IRQ_WR_EN_POS        (1U)                                                /*!< UART6_7_CF_CTRL_IRQ_WR_EN bit offset */

/* UART6_7_CF_CTRL_PULSE_OE */
#define UART6_7_CF_CTRL_PULSE_OE_POS         (2U)                                                /*!< UART6_7_CF_CTRL_PULSE_OE bit offset */

/* UART6_7_PFDAT_PFDAT */
#define UART6_7_PFDAT_PFDAT_POS              (0U)                                                /*!< UART6_7_PFDAT_PFDAT bit offset */

/* UART6_7_QFDAT_QFDAT */
#define UART6_7_QFDAT_QFDAT_POS              (0U)                                                /*!< UART6_7_QFDAT_QFDAT bit offset */

/* UART6_7_FPFDAT_FPFDAT */
#define UART6_7_FPFDAT_FPFDAT_POS            (0U)                                                /*!< UART6_7_FPFDAT_FPFDAT bit offset */

/* UART6_7_IRQDAT_IRQDAT */
#define UART6_7_IRQDAT_IRQDAT_POS            (0U)                                                /*!< UART6_7_IRQDAT_IRQDAT bit offset */

/* UART6_7_CF_OV_CF_OV_TIME */
#define UART6_7_CF_OV_CF_OV_TIME_POS         (0U)                                                /*!< UART6_7_CF_OV_CF_OV_TIME bit offset */
#define UART6_7_CF_OV_CF_OV_TIME_CFG(regval)  (UART6_7_CF_OV_CF_OV_TIME & ((uint32_t)(regval) << UART6_7_CF_OV_CF_OV_TIME_POS))
#define UART6_7_CF_OV_CF_OV_TIME_TIMEOUT_NONE  (UART6_7_CF_OV_CF_OV_TIME_CFG(0x0))                 /*!< Timeout mechanism disabled */
#define UART6_7_CF_OV_CF_OV_TIME_TIMEOUT_22B  (UART6_7_CF_OV_CF_OV_TIME_CFG(0x1))                 /*!< Timeout after 22 bits */
#define UART6_7_CF_OV_CF_OV_TIME_TIMEOUT_44B  (UART6_7_CF_OV_CF_OV_TIME_CFG(0x2))                 /*!< Timeout after 44 bits */
#define UART6_7_CF_OV_CF_OV_TIME_TIMEOUT_88B  (UART6_7_CF_OV_CF_OV_TIME_CFG(0x3))                 /*!< Timeout after 88 bits */

/* UART6_7_CF_OV_OV_BUF_CLR */
#define UART6_7_CF_OV_OV_BUF_CLR_POS         (2U)                                                /*!< UART6_7_CF_OV_OV_BUF_CLR bit offset */

/* UART6_7_TW_CTRL_UART_TW_MODE */
#define UART6_7_TW_CTRL_UART_TW_MODE_POS     (0U)                                                /*!< UART6_7_TW_CTRL_UART_TW_MODE bit offset */

/* UART6_7_TW_CTRL_TW_NUM */
#define UART6_7_TW_CTRL_TW_NUM_POS           (1U)                                                /*!< UART6_7_TW_CTRL_TW_NUM bit offset */

/* UART6_7_TW_BAUD_TW_BAUD */
#define UART6_7_TW_BAUD_TW_BAUD_POS          (0U)                                                /*!< UART6_7_TW_BAUD_TW_BAUD bit offset */

/* UART6_7_TW_EN_TW_EN */
#define UART6_7_TW_EN_TW_EN_POS              (0U)                                                /*!< UART6_7_TW_EN_TW_EN bit offset */

/* UART6_7_CAL_CTRL_UART_CAL_EN */
#define UART6_7_CAL_CTRL_UART_CAL_EN_POS     (0U)                                                /*!< UART6_7_CAL_CTRL_UART_CAL_EN bit offset */

/* UART6_7_CAL_CTRL_CAL_MODE */
#define UART6_7_CAL_CTRL_CAL_MODE_POS        (1U)                                                /*!< UART6_7_CAL_CTRL_CAL_MODE bit offset */
#define UART6_7_CAL_CTRL_CAL_MODE_CONTINUOUS  ((uint32_t)0x00000000U)                             /*!< Continuous measurement */
#define UART6_7_CAL_CTRL_CAL_MODE_FIXED_DATA  (UART6_7_CAL_CTRL_CAL_MODE)                         /*!< Fixed data measurement */

/* UART6_7_CAL_CTRL_POS_START */
#define UART6_7_CAL_CTRL_POS_START_POS       (2U)                                                /*!< UART6_7_CAL_CTRL_POS_START bit offset */
#define UART6_7_CAL_CTRL_POS_START_RX_FALLING_EDGE  ((uint32_t)0x00000000U)                             /*!< RX falling edge */
#define UART6_7_CAL_CTRL_POS_START_RX_RISING_EDGE  (UART6_7_CAL_CTRL_POS_START)                        /*!< RX rising edge */

/* UART6_7_CAL_CTRL_POS_END */
#define UART6_7_CAL_CTRL_POS_END_POS         (3U)                                                /*!< UART6_7_CAL_CTRL_POS_END bit offset */
#define UART6_7_CAL_CTRL_POS_END_RX_FALLING_EDGE  ((uint32_t)0x00000000U)                             /*!< RX falling edge */
#define UART6_7_CAL_CTRL_POS_END_RX_RISING_EDGE  (UART6_7_CAL_CTRL_POS_END)                          /*!< RX rising edge */

/* UART6_7_CAL_CTRL_CAL_DATA */
#define UART6_7_CAL_CTRL_CAL_DATA_POS        (4U)                                                /*!< UART6_7_CAL_CTRL_CAL_DATA bit offset */

/* UART6_7_CAL_CNT_CAL_CNT */
#define UART6_7_CAL_CNT_CAL_CNT_POS          (0U)                                                /*!< UART6_7_CAL_CNT_CAL_CNT bit offset */

/* UART6_7_TX_DRLAY_TIMEOUT_CNT */
#define UART6_7_TX_DRLAY_TIMEOUT_CNT_POS     (0U)                                                /*!< UART6_7_TX_DRLAY_TIMEOUT_CNT bit offset */
#define UART6_7_TX_DRLAY_TIMEOUT_CNT_CFG(regval)  (UART6_7_TX_DRLAY_TIMEOUT_CNT & ((uint32_t)(regval) << UART6_7_TX_DRLAY_TIMEOUT_CNT_POS))
#define UART6_7_TX_DRLAY_TIMEOUT_CNT_1B      (UART6_7_TX_DRLAY_TIMEOUT_CNT_CFG(0x0))             /*!< 1 UART data bit time */
#define UART6_7_TX_DRLAY_TIMEOUT_CNT_2B      (UART6_7_TX_DRLAY_TIMEOUT_CNT_CFG(0x1))             /*!< 2 UART data bit times */
#define UART6_7_TX_DRLAY_TIMEOUT_CNT_3B      (UART6_7_TX_DRLAY_TIMEOUT_CNT_CFG(0x2))             /*!< 3 UART data bit times */
#define UART6_7_TX_DRLAY_TIMEOUT_CNT_4B      (UART6_7_TX_DRLAY_TIMEOUT_CNT_CFG(0x3))             /*!< 4 UART data bit times */

/* UART6_7_TX_DRLAY_TX_DELAY_EN */
#define UART6_7_TX_DRLAY_TX_DELAY_EN_POS     (2U)                                                /*!< UART6_7_TX_DRLAY_TX_DELAY_EN bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_UART6_7_H */
