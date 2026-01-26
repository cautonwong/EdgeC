/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_uart0_5.h
 * @author  Renergy Software Team
 * @brief   UART0_5 module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_UART0_5_H
#define _RN831x_RN861x_MCU_UART0_5_H

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

/* UART0_5 */
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
    __IO uint32_t TX_DRLAY;                  /*!< offset: 0x040 info: Transmit Delay Configuration Register (UART0-5) */
} UART0_5_TypeDef;

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
#define UART0_CORE0_BASE (0x20000000U)
#define UART1_CORE0_BASE (0x20004000U)
#define UART2_CORE0_BASE (0x20008000U)
#define UART3_CORE0_BASE (0x2000C000U)
#define UART4_CORE0_BASE (0x20010000U)
#define UART5_CORE0_BASE (0x20014000U)
#define UART0_CORE1_BASE (0xA0000000U)
#define UART1_CORE1_BASE (0xA0004000U)
#define UART2_CORE1_BASE (0xA0008000U)
#define UART3_CORE1_BASE (0xA000C000U)
#define UART4_CORE1_BASE (0xA0010000U)
#define UART5_CORE1_BASE (0xA0014000U)
#define UART0_CORE0_BB_BASE (0x22000000U)
#define UART1_CORE0_BB_BASE (0x22080000U)
#define UART2_CORE0_BB_BASE (0x22100000U)
#define UART3_CORE0_BB_BASE (0x22180000U)
#define UART4_CORE0_BB_BASE (0x22200000U)
#define UART5_CORE0_BB_BASE (0x22280000U)
#define UART0_CORE1_BB_BASE (0xA2000000U)
#define UART1_CORE1_BB_BASE (0xA2080000U)
#define UART2_CORE1_BB_BASE (0xA2100000U)
#define UART3_CORE1_BB_BASE (0xA2180000U)
#define UART4_CORE1_BB_BASE (0xA2200000U)
#define UART5_CORE1_BB_BASE (0xA2280000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define UART0_CORE0 ((UART0_5_TypeDef *)  UART0_CORE0_BASE)
#define UART1_CORE0 ((UART0_5_TypeDef *)  UART1_CORE0_BASE)
#define UART2_CORE0 ((UART0_5_TypeDef *)  UART2_CORE0_BASE)
#define UART3_CORE0 ((UART0_5_TypeDef *)  UART3_CORE0_BASE)
#define UART4_CORE0 ((UART0_5_TypeDef *)  UART4_CORE0_BASE)
#define UART5_CORE0 ((UART0_5_TypeDef *)  UART5_CORE0_BASE)
#define UART0_CORE1 ((UART0_5_TypeDef *)  UART0_CORE1_BASE)
#define UART1_CORE1 ((UART0_5_TypeDef *)  UART1_CORE1_BASE)
#define UART2_CORE1 ((UART0_5_TypeDef *)  UART2_CORE1_BASE)
#define UART3_CORE1 ((UART0_5_TypeDef *)  UART3_CORE1_BASE)
#define UART4_CORE1 ((UART0_5_TypeDef *)  UART4_CORE1_BASE)
#define UART5_CORE1 ((UART0_5_TypeDef *)  UART5_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define UART0  UART0_CORE0
#define UART1  UART1_CORE0
#define UART2  UART2_CORE0
#define UART3  UART3_CORE0
#define UART4  UART4_CORE0
#define UART5  UART5_CORE0
#endif
#if defined(CORE1)
#define UART0  UART0_CORE1
#define UART1  UART1_CORE1
#define UART2  UART2_CORE1
#define UART3  UART3_CORE1
#define UART4  UART4_CORE1
#define UART5  UART5_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** UART0_5 Instances *******************************/
#define IS_UART0_5_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == UART0_CORE0) || \
                                              ((INSTANCE) == UART1_CORE0) || \
                                              ((INSTANCE) == UART2_CORE0) || \
                                              ((INSTANCE) == UART3_CORE0) || \
                                              ((INSTANCE) == UART4_CORE0) || \
                                              ((INSTANCE) == UART5_CORE0) || \
                                              ((INSTANCE) == UART0_CORE1) || \
                                              ((INSTANCE) == UART1_CORE1) || \
                                              ((INSTANCE) == UART2_CORE1) || \
                                              ((INSTANCE) == UART3_CORE1) || \
                                              ((INSTANCE) == UART4_CORE1) || \
                                              ((INSTANCE) == UART5_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* UART0_5_CTRL */
#define UART0_5_CTRL_EN                      (BIT(0))                                            /*!< Module Enable */
#define UART0_5_CTRL_TXIE                    (BIT(1))                                            /*!< Transmit Data Interrupt Enable */
#define UART0_5_CTRL_RXIE                    (BIT(2))                                            /*!< Receive Data Interrupt Enable */
#define UART0_5_CTRL_ERRIE                   (BIT(3))                                            /*!< Error Interrupt Enable */
#define UART0_5_CTRL_STOPS                   (BIT(4))                                            /*!< Stop Bit Length */
#define UART0_5_CTRL_DATLEN                  (BITS(5,6))                                         /*!< Data Length */
#define UART0_5_CTRL_PARS                    (BITS(7,9))                                         /*!< Parity Selection */
#define UART0_5_CTRL_IRE                     (BIT(10))                                           /*!< Infrared Modulation Enable */
#define UART0_5_CTRL_ILBE                    (BIT(11))                                           /*!< Internal Loopback Enable */
#define UART0_5_CTRL_IRSEL                   (BIT(12))                                           /*!< Infrared modulation polarity selection */
#define UART0_5_CTRL_LMSB                    (BIT(13))                                           /*!< LSB/MSB transmission order */
#define UART0_5_CTRL_NEG                     (BIT(14))                                           /*!< Data polarity selection */
#define UART0_5_CTRL_PAR_MEM                 (BIT(15))                                           /*!< Memory storage configuration on parity error */
#define UART0_5_CTRL_RX_TO_IE                (BIT(16))                                           /*!< Receive Timeout Interrupt Enable */

/* UART0_5_BAUD */
#define UART0_5_BAUD_CLKDIV                  (BITS(0,13))                                        /*!< Clock divider value for UARTx. */

/* UART0_5_STA */
#define UART0_5_STA_RX                       (BIT(0))                                            /*!< Reception Complete Flag */
#define UART0_5_STA_TX                       (BIT(1))                                            /*!< Transmission Complete Flag */
#define UART0_5_STA_PE                       (BIT(2))                                            /*!< Parity Error Flag */
#define UART0_5_STA_OE                       (BIT(3))                                            /*!< Overrun Error Flag */
#define UART0_5_STA_FE                       (BIT(4))                                            /*!< Frame Error Flag */
#define UART0_5_STA_DE                       (BIT(5))                                            /*!< Data Error Flag */
#define UART0_5_STA_RB                       (BIT(6))                                            /*!< Reception Busy Flag */
#define UART0_5_STA_TB                       (BIT(7))                                            /*!< Transmission Busy Flag */
#define UART0_5_STA_TX_FIFO_EMPTY            (BIT(8))                                            /*!< Transmit FIFO Empty */
#define UART0_5_STA_TX_FIFO_FULL             (BIT(9))                                            /*!< Transmit FIFO Full */
#define UART0_5_STA_RX_TO                    (BIT(10))                                           /*!< Receive Timeout Flag */

/* UART0_5_TXDATA */
#define UART0_5_TXDATA_TXDATA                (BITS(0,7))                                         /*!< Transmit Data */
#define UART0_5_TXDATA_UP                    (BIT(8))                                            /*!< User-defined Parity Bit (used when parity is set to user-defined) */

/* UART0_5_RXDATA */
#define UART0_5_RXDATA_RXDATA                (BITS(0,7))                                         /*!< Receive Data */
#define UART0_5_RXDATA_UP                    (BIT(8))                                            /*!< Parity Bit (received).User-defined Parity Bit (used when parity is set to user-defined) */

/* UART0_5_FDIV */
#define UART0_5_FDIV_FDIV                    (BITS(0,13))                                        /*!< Fractional divider coefficient. */

/* UART0_5_DMA_CTRL */
#define UART0_5_DMA_CTRL_TX_DMA_EN           (BIT(0))                                            /*!< Transmit DMA Enable */
#define UART0_5_DMA_CTRL_RX_DMA_EN           (BIT(1))                                            /*!< Receive DMA Enable */
#define UART0_5_DMA_CTRL_TX_CYC_MODE         (BIT(2))                                            /*!< Transmit Cycle Mode Enable */
#define UART0_5_DMA_CTRL_RX_CYC_MODE         (BIT(3))                                            /*!< Receive Cycle Mode Enable */
#define UART0_5_DMA_CTRL_DMA_PASS            (BIT(4))                                            /*!< DMA Parity Check Bit */

/* UART0_5_DMA_TBADR */
#define UART0_5_DMA_TBADR_DMA_TBADR          (BITS(0,17))                                        /*!< DMA Transmit Base Address (byte address). The lower two bits must be configured according to the data width. */

/* UART0_5_DMA_RBADR */
#define UART0_5_DMA_RBADR_DMA_RBADR          (BITS(0,17))                                        /*!< DMA Receive Base Address (byte address). The lower two bits must be configured according to the data width. */

/* UART0_5_DMA_TLEN */
#define UART0_5_DMA_TLEN_DMA_TLEN            (BITS(0,17))                                        /*!< DMA Transmit Length (in bytes). The lower two bits must be configured according to the data width. */

/* UART0_5_DMA_RLEN */
#define UART0_5_DMA_RLEN_DMA_RLEN            (BITS(0,17))                                        /*!< DMA Receive Length (in bytes). The lower two bits must be configured according to the data width. */

/* UART0_5_DMA_TADR */
#define UART0_5_DMA_TADR_DMA_TADR            (BITS(0,17))                                        /*!< Current DMA Transmit Address (byte address). */

/* UART0_5_DMA_RADR */
#define UART0_5_DMA_RADR_DMA_RADR            (BITS(0,17))                                        /*!< Current DMA Receive Address (byte address). */

/* UART0_5_DMA_IE */
#define UART0_5_DMA_IE_TX_HIE                (BIT(0))                                            /*!< DMA Transmit Half Empty Interrupt Enable */
#define UART0_5_DMA_IE_TX_FIE                (BIT(1))                                            /*!< DMA Transmit Empty (Full) Interrupt Enable */
#define UART0_5_DMA_IE_RX_HIE                (BIT(2))                                            /*!< DMA Receive Half Full Interrupt Enable */
#define UART0_5_DMA_IE_RX_FIE                (BIT(3))                                            /*!< DMA Receive Full Interrupt Enable */
#define UART0_5_DMA_IE_TX_ERR_IE             (BIT(4))                                            /*!< Transmit Data Error Interrupt Enable */
#define UART0_5_DMA_IE_RX_ERR_IE             (BIT(5))                                            /*!< Receive Data Error Interrupt Enable */
#define UART0_5_DMA_IE_RX_DONE_IE            (BIT(6))                                            /*!< DMA Receive Timeout Interrupt Enable */

/* UART0_5_DMA_IF */
#define UART0_5_DMA_IF_TX_HDONE              (BIT(0))                                            /*!< DMA Transmit Half Empty Interrupt Flag */
#define UART0_5_DMA_IF_TX_FDONE              (BIT(1))                                            /*!< DMA Transmit Full Empty Interrupt Flag */
#define UART0_5_DMA_IF_RX_HDONE              (BIT(2))                                            /*!< DMA Receive Half Full Interrupt Flag */
#define UART0_5_DMA_IF_RX_FDONE              (BIT(3))                                            /*!< DMA Receive Full Interrupt Flag */
#define UART0_5_DMA_IF_RX_ERR                (BIT(5))                                            /*!< Receive Data Error Flag */
#define UART0_5_DMA_IF_RX_DONE               (BIT(6))                                            /*!< DMA Receive Complete Flag */

/* UART0_5_RX_TO */
#define UART0_5_RX_TO_TIMEOUT_CNT            (BITS(0,7))                                         /*!< Timeout value in terms of UART data bit widths. After the start bit is received, if no data is received for this duration, a timeout occurs. */

/* UART0_5_TX_DRLAY */
#define UART0_5_TX_DRLAY_TIMEOUT_CNT         (BITS(0,1))                                         /*!< Delay Insertion. Effective when TX_DELAY_EN=1 */
#define UART0_5_TX_DRLAY_TX_DELAY_EN         (BIT(2))                                            /*!< Transmit Delay Enable */

/* constants definitions */
/* UART0_5_CTRL_EN */
#define UART0_5_CTRL_EN_POS                  (0U)                                                /*!< UART0_5_CTRL_EN bit offset */

/* UART0_5_CTRL_TXIE */
#define UART0_5_CTRL_TXIE_POS                (1U)                                                /*!< UART0_5_CTRL_TXIE bit offset */

/* UART0_5_CTRL_RXIE */
#define UART0_5_CTRL_RXIE_POS                (2U)                                                /*!< UART0_5_CTRL_RXIE bit offset */

/* UART0_5_CTRL_ERRIE */
#define UART0_5_CTRL_ERRIE_POS               (3U)                                                /*!< UART0_5_CTRL_ERRIE bit offset */

/* UART0_5_CTRL_STOPS */
#define UART0_5_CTRL_STOPS_POS               (4U)                                                /*!< UART0_5_CTRL_STOPS bit offset */
#define UART0_5_CTRL_STOPS_1                 ((uint32_t)0x00000000U)                             /*!< 1 stop bit. */
#define UART0_5_CTRL_STOPS_2                 (UART0_5_CTRL_STOPS)                                /*!< 2 stop bits. */

/* UART0_5_CTRL_DATLEN */
#define UART0_5_CTRL_DATLEN_POS              (5U)                                                /*!< UART0_5_CTRL_DATLEN bit offset */
#define UART0_5_CTRL_DATLEN_CFG(regval)  (UART0_5_CTRL_DATLEN & ((uint32_t)(regval) << UART0_5_CTRL_DATLEN_POS))
#define UART0_5_CTRL_DATLEN_5B               (UART0_5_CTRL_DATLEN_CFG(0x0))                      /*!< 5 data bits */
#define UART0_5_CTRL_DATLEN_6B               (UART0_5_CTRL_DATLEN_CFG(0x1))                      /*!< 6 data bits */
#define UART0_5_CTRL_DATLEN_7B               (UART0_5_CTRL_DATLEN_CFG(0x2))                      /*!< 7 data bits */
#define UART0_5_CTRL_DATLEN_8B               (UART0_5_CTRL_DATLEN_CFG(0x3))                      /*!< 8 data bits */

/* UART0_5_CTRL_PARS */
#define UART0_5_CTRL_PARS_POS                (7U)                                                /*!< UART0_5_CTRL_PARS bit offset */
#define UART0_5_CTRL_PARS_CFG(regval)  (UART0_5_CTRL_PARS & ((uint32_t)(regval) << UART0_5_CTRL_PARS_POS))
#define UART0_5_CTRL_PARS_NONE               (UART0_5_CTRL_PARS_CFG(0x0))                        /*!< No parity */
#define UART0_5_CTRL_PARS_ODD                (UART0_5_CTRL_PARS_CFG(0x1))                        /*!< Odd parity */
#define UART0_5_CTRL_PARS_EVEN               (UART0_5_CTRL_PARS_CFG(0x2))                        /*!< Even parity */
#define UART0_5_CTRL_PARS_ZERO               (UART0_5_CTRL_PARS_CFG(0x3))                        /*!< Fixed to 0 parity */
#define UART0_5_CTRL_PARS_ONE                (UART0_5_CTRL_PARS_CFG(0x4))                        /*!< Fixed to 1 parity */
#define UART0_5_CTRL_PARS_USER_DEFINED       (UART0_5_CTRL_PARS_CFG(0x5))                        /*!< User-defined parity */

/* UART0_5_CTRL_IRE */
#define UART0_5_CTRL_IRE_POS                 (10U)                                               /*!< UART0_5_CTRL_IRE bit offset */

/* UART0_5_CTRL_ILBE */
#define UART0_5_CTRL_ILBE_POS                (11U)                                               /*!< UART0_5_CTRL_ILBE bit offset */

/* UART0_5_CTRL_IRSEL */
#define UART0_5_CTRL_IRSEL_POS               (12U)                                               /*!< UART0_5_CTRL_IRSEL bit offset */
#define UART0_5_CTRL_IRSEL_POSITIVE          ((uint32_t)0x00000000U)                             /*!< Positive polarity (default driven level is high, low level is modulated). */
#define UART0_5_CTRL_IRSEL_NEGATIVE          (UART0_5_CTRL_IRSEL)                                /*!< Negative polarity (default driven level is low, high level is modulated). */

/* UART0_5_CTRL_LMSB */
#define UART0_5_CTRL_LMSB_POS                (13U)                                               /*!< UART0_5_CTRL_LMSB bit offset */
#define UART0_5_CTRL_LMSB_LSB_FIRST          ((uint32_t)0x00000000U)                             /*!< LSB first */
#define UART0_5_CTRL_LMSB_MSB_FIRST          (UART0_5_CTRL_LMSB)                                 /*!< MSB first */

/* UART0_5_CTRL_NEG */
#define UART0_5_CTRL_NEG_POS                 (14U)                                               /*!< UART0_5_CTRL_NEG bit offset */
#define UART0_5_CTRL_NEG_POSITIVE            ((uint32_t)0x00000000U)                             /*!< Positive polarity (default driven level is high, data is not inverted). */
#define UART0_5_CTRL_NEG_NEGATIVE            (UART0_5_CTRL_NEG)                                  /*!< Negative polarity (default driven level is low, data is inverted). */

/* UART0_5_CTRL_PAR_MEM */
#define UART0_5_CTRL_PAR_MEM_POS             (15U)                                               /*!< UART0_5_CTRL_PAR_MEM bit offset */
#define UART0_5_CTRL_PAR_MEM_STORE_MEMORY    ((uint32_t)0x00000000U)                             /*!< Data is stored in memory regardless of parity error. */
#define UART0_5_CTRL_PAR_MEM_NOT_STORE_MEMORY  (UART0_5_CTRL_PAR_MEM)                              /*!< When parity error occurs, the current frame is not stored in memory. */

/* UART0_5_CTRL_RX_TO_IE */
#define UART0_5_CTRL_RX_TO_IE_POS            (16U)                                               /*!< UART0_5_CTRL_RX_TO_IE bit offset */

/* UART0_5_BAUD_CLKDIV */
#define UART0_5_BAUD_CLKDIV_POS              (0U)                                                /*!< UART0_5_BAUD_CLKDIV bit offset */

/* UART0_5_STA_RX */
#define UART0_5_STA_RX_POS                   (0U)                                                /*!< UART0_5_STA_RX bit offset */

/* UART0_5_STA_TX */
#define UART0_5_STA_TX_POS                   (1U)                                                /*!< UART0_5_STA_TX bit offset */

/* UART0_5_STA_PE */
#define UART0_5_STA_PE_POS                   (2U)                                                /*!< UART0_5_STA_PE bit offset */

/* UART0_5_STA_OE */
#define UART0_5_STA_OE_POS                   (3U)                                                /*!< UART0_5_STA_OE bit offset */

/* UART0_5_STA_FE */
#define UART0_5_STA_FE_POS                   (4U)                                                /*!< UART0_5_STA_FE bit offset */

/* UART0_5_STA_DE */
#define UART0_5_STA_DE_POS                   (5U)                                                /*!< UART0_5_STA_DE bit offset */

/* UART0_5_STA_RB */
#define UART0_5_STA_RB_POS                   (6U)                                                /*!< UART0_5_STA_RB bit offset */

/* UART0_5_STA_TB */
#define UART0_5_STA_TB_POS                   (7U)                                                /*!< UART0_5_STA_TB bit offset */

/* UART0_5_STA_TX_FIFO_EMPTY */
#define UART0_5_STA_TX_FIFO_EMPTY_POS        (8U)                                                /*!< UART0_5_STA_TX_FIFO_EMPTY bit offset */

/* UART0_5_STA_TX_FIFO_FULL */
#define UART0_5_STA_TX_FIFO_FULL_POS         (9U)                                                /*!< UART0_5_STA_TX_FIFO_FULL bit offset */

/* UART0_5_STA_RX_TO */
#define UART0_5_STA_RX_TO_POS                (10U)                                               /*!< UART0_5_STA_RX_TO bit offset */

/* UART0_5_TXDATA_TXDATA */
#define UART0_5_TXDATA_TXDATA_POS            (0U)                                                /*!< UART0_5_TXDATA_TXDATA bit offset */

/* UART0_5_TXDATA_UP */
#define UART0_5_TXDATA_UP_POS                (8U)                                                /*!< UART0_5_TXDATA_UP bit offset */

/* UART0_5_RXDATA_RXDATA */
#define UART0_5_RXDATA_RXDATA_POS            (0U)                                                /*!< UART0_5_RXDATA_RXDATA bit offset */

/* UART0_5_RXDATA_UP */
#define UART0_5_RXDATA_UP_POS                (8U)                                                /*!< UART0_5_RXDATA_UP bit offset */

/* UART0_5_FDIV_FDIV */
#define UART0_5_FDIV_FDIV_POS                (0U)                                                /*!< UART0_5_FDIV_FDIV bit offset */

/* UART0_5_DMA_CTRL_TX_DMA_EN */
#define UART0_5_DMA_CTRL_TX_DMA_EN_POS       (0U)                                                /*!< UART0_5_DMA_CTRL_TX_DMA_EN bit offset */

/* UART0_5_DMA_CTRL_RX_DMA_EN */
#define UART0_5_DMA_CTRL_RX_DMA_EN_POS       (1U)                                                /*!< UART0_5_DMA_CTRL_RX_DMA_EN bit offset */

/* UART0_5_DMA_CTRL_TX_CYC_MODE */
#define UART0_5_DMA_CTRL_TX_CYC_MODE_POS     (2U)                                                /*!< UART0_5_DMA_CTRL_TX_CYC_MODE bit offset */

/* UART0_5_DMA_CTRL_RX_CYC_MODE */
#define UART0_5_DMA_CTRL_RX_CYC_MODE_POS     (3U)                                                /*!< UART0_5_DMA_CTRL_RX_CYC_MODE bit offset */

/* UART0_5_DMA_CTRL_DMA_PASS */
#define UART0_5_DMA_CTRL_DMA_PASS_POS        (4U)                                                /*!< UART0_5_DMA_CTRL_DMA_PASS bit offset */
#define UART0_5_DMA_CTRL_DMA_PASS_ZERO       ((uint32_t)0x00000000U)                             /*!< DMA Transmit Fixed to 0 parity */
#define UART0_5_DMA_CTRL_DMA_PASS_ONE        (UART0_5_DMA_CTRL_DMA_PASS)                         /*!< DMA Transmit Fixed to 1 parity */

/* UART0_5_DMA_TBADR_DMA_TBADR */
#define UART0_5_DMA_TBADR_DMA_TBADR_POS      (0U)                                                /*!< UART0_5_DMA_TBADR_DMA_TBADR bit offset */

/* UART0_5_DMA_RBADR_DMA_RBADR */
#define UART0_5_DMA_RBADR_DMA_RBADR_POS      (0U)                                                /*!< UART0_5_DMA_RBADR_DMA_RBADR bit offset */

/* UART0_5_DMA_TLEN_DMA_TLEN */
#define UART0_5_DMA_TLEN_DMA_TLEN_POS        (0U)                                                /*!< UART0_5_DMA_TLEN_DMA_TLEN bit offset */

/* UART0_5_DMA_RLEN_DMA_RLEN */
#define UART0_5_DMA_RLEN_DMA_RLEN_POS        (0U)                                                /*!< UART0_5_DMA_RLEN_DMA_RLEN bit offset */

/* UART0_5_DMA_TADR_DMA_TADR */
#define UART0_5_DMA_TADR_DMA_TADR_POS        (0U)                                                /*!< UART0_5_DMA_TADR_DMA_TADR bit offset */

/* UART0_5_DMA_RADR_DMA_RADR */
#define UART0_5_DMA_RADR_DMA_RADR_POS        (0U)                                                /*!< UART0_5_DMA_RADR_DMA_RADR bit offset */

/* UART0_5_DMA_IE_TX_HIE */
#define UART0_5_DMA_IE_TX_HIE_POS            (0U)                                                /*!< UART0_5_DMA_IE_TX_HIE bit offset */

/* UART0_5_DMA_IE_TX_FIE */
#define UART0_5_DMA_IE_TX_FIE_POS            (1U)                                                /*!< UART0_5_DMA_IE_TX_FIE bit offset */

/* UART0_5_DMA_IE_RX_HIE */
#define UART0_5_DMA_IE_RX_HIE_POS            (2U)                                                /*!< UART0_5_DMA_IE_RX_HIE bit offset */

/* UART0_5_DMA_IE_RX_FIE */
#define UART0_5_DMA_IE_RX_FIE_POS            (3U)                                                /*!< UART0_5_DMA_IE_RX_FIE bit offset */

/* UART0_5_DMA_IE_TX_ERR_IE */
#define UART0_5_DMA_IE_TX_ERR_IE_POS         (4U)                                                /*!< UART0_5_DMA_IE_TX_ERR_IE bit offset */

/* UART0_5_DMA_IE_RX_ERR_IE */
#define UART0_5_DMA_IE_RX_ERR_IE_POS         (5U)                                                /*!< UART0_5_DMA_IE_RX_ERR_IE bit offset */

/* UART0_5_DMA_IE_RX_DONE_IE */
#define UART0_5_DMA_IE_RX_DONE_IE_POS        (6U)                                                /*!< UART0_5_DMA_IE_RX_DONE_IE bit offset */

/* UART0_5_DMA_IF_TX_HDONE */
#define UART0_5_DMA_IF_TX_HDONE_POS          (0U)                                                /*!< UART0_5_DMA_IF_TX_HDONE bit offset */

/* UART0_5_DMA_IF_TX_FDONE */
#define UART0_5_DMA_IF_TX_FDONE_POS          (1U)                                                /*!< UART0_5_DMA_IF_TX_FDONE bit offset */

/* UART0_5_DMA_IF_RX_HDONE */
#define UART0_5_DMA_IF_RX_HDONE_POS          (2U)                                                /*!< UART0_5_DMA_IF_RX_HDONE bit offset */

/* UART0_5_DMA_IF_RX_FDONE */
#define UART0_5_DMA_IF_RX_FDONE_POS          (3U)                                                /*!< UART0_5_DMA_IF_RX_FDONE bit offset */

/* UART0_5_DMA_IF_RX_ERR */
#define UART0_5_DMA_IF_RX_ERR_POS            (5U)                                                /*!< UART0_5_DMA_IF_RX_ERR bit offset */

/* UART0_5_DMA_IF_RX_DONE */
#define UART0_5_DMA_IF_RX_DONE_POS           (6U)                                                /*!< UART0_5_DMA_IF_RX_DONE bit offset */

/* UART0_5_RX_TO_TIMEOUT_CNT */
#define UART0_5_RX_TO_TIMEOUT_CNT_POS        (0U)                                                /*!< UART0_5_RX_TO_TIMEOUT_CNT bit offset */

/* UART0_5_TX_DRLAY_TIMEOUT_CNT */
#define UART0_5_TX_DRLAY_TIMEOUT_CNT_POS     (0U)                                                /*!< UART0_5_TX_DRLAY_TIMEOUT_CNT bit offset */
#define UART0_5_TX_DRLAY_TIMEOUT_CNT_CFG(regval)  (UART0_5_TX_DRLAY_TIMEOUT_CNT & ((uint32_t)(regval) << UART0_5_TX_DRLAY_TIMEOUT_CNT_POS))
#define UART0_5_TX_DRLAY_TIMEOUT_CNT_1B      (UART0_5_TX_DRLAY_TIMEOUT_CNT_CFG(0x0))             /*!< 1 UART data bit time */
#define UART0_5_TX_DRLAY_TIMEOUT_CNT_2B      (UART0_5_TX_DRLAY_TIMEOUT_CNT_CFG(0x1))             /*!< 2 UART data bit times */
#define UART0_5_TX_DRLAY_TIMEOUT_CNT_3B      (UART0_5_TX_DRLAY_TIMEOUT_CNT_CFG(0x2))             /*!< 3 UART data bit times */
#define UART0_5_TX_DRLAY_TIMEOUT_CNT_4B      (UART0_5_TX_DRLAY_TIMEOUT_CNT_CFG(0x3))             /*!< 4 UART data bit times */

/* UART0_5_TX_DRLAY_TX_DELAY_EN */
#define UART0_5_TX_DRLAY_TX_DELAY_EN_POS     (2U)                                                /*!< UART0_5_TX_DRLAY_TX_DELAY_EN bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_UART0_5_H */
