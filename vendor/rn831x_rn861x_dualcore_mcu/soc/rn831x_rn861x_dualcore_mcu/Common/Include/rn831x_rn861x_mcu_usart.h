/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_usart.h
 * @author  Renergy Software Team
 * @brief   USART module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_USART_H
#define _RN831x_RN861x_MCU_USART_H

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

/* USART */
typedef struct
{
    __IO uint32_t TXDATA;                    /*!< offset: 0x000 info: Transmit Data Register */
    __IO uint32_t RXDATA;                    /*!< offset: 0x004 info: Receive Data Register */
    __IO uint32_t TXCTRL;                    /*!< offset: 0x008 info: Transmit Control Register */
    __IO uint32_t RXCTRL;                    /*!< offset: 0x00C info: Receive Control Register */
    __IO uint32_t IE;                        /*!< offset: 0x010 info: Interrupt Enable Register */
    __IO uint32_t STA;                       /*!< offset: 0x014 info: Status Register */
    __IO uint32_t DIV;                       /*!< offset: 0x018 info: Baud Rate Divider Register */
    __IO uint32_t SETUP;                     /*!< offset: 0x01C info: Configuration Register */
    __IO uint32_t RESERVED0;                 /*!< offset: 0x020 */
    __IO uint32_t RX_SIZE;                   /*!< offset: 0x024 info: RX Transfer Size Register */
    __IO uint32_t RESERVED1[2];              /*!< offset: 0x028 ~ 02C */
    __IO uint32_t TX_SIZE;                   /*!< offset: 0x030 info: TX Transfer Size Register */
    __IO uint32_t RESERVED2[4];              /*!< offset: 0x034 ~ 040 */
    __IO uint32_t RX_IDLE;                   /*!< offset: 0x044 info: RX Idle Timeout Register */
    __IO uint32_t RX_WM;                     /*!< offset: 0x048 info: RX Watermark Timeout Register */
    __IO uint32_t RX_FIFO_LEFT_ENTRY;        /*!< offset: 0x04C info: RX FIFO Free Space */
    __IO uint32_t TX_FIFO_LEFT_ENTRY;        /*!< offset: 0x050 info: TX FIFO Free Space */
    __IO uint32_t RESERVED3[2];              /*!< offset: 0x054 ~ 058 */
    __IO uint32_t RX_LOW_LEVEL_CNT;          /*!< offset: 0x05C info: RX Low-Level Duration */
} USART_TypeDef;

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
#define USART0_CORE0_BASE (0x20024000U)
#define USART1_CORE0_BASE (0x20028000U)
#define USART0_CORE1_BASE (0xA0024000U)
#define USART1_CORE1_BASE (0xA0028000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define USART0_CORE0 ((USART_TypeDef *)    USART0_CORE0_BASE)
#define USART1_CORE0 ((USART_TypeDef *)    USART1_CORE0_BASE)
#define USART0_CORE1 ((USART_TypeDef *)    USART0_CORE1_BASE)
#define USART1_CORE1 ((USART_TypeDef *)    USART1_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define USART0  USART0_CORE0
#define USART1  USART1_CORE0
#endif
#if defined(CORE1)
#define USART0  USART0_CORE1
#define USART1  USART1_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** USART Instances *******************************/
#define IS_USART_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == USART0_CORE0) || \
                                            ((INSTANCE) == USART1_CORE0) || \
                                            ((INSTANCE) == USART0_CORE1) || \
                                            ((INSTANCE) == USART1_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* USART_TXDATA */
#define USART_TXDATA_TXDATA                  (BITS(0,8))                                         /*!< Data to transmit */

/* USART_RXDATA */
#define USART_RXDATA_RXDATA                  (BITS(0,8))                                         /*!< Received data */

/* USART_TXCTRL */
#define USART_TXCTRL_TXEN                    (BIT(0))                                            /*!< Transmit enable */
#define USART_TXCTRL_CFG_STOP_BIT            (BITS(1,2))                                         /*!< Stop bit length */
#define USART_TXCTRL_CONTINUOUS              (BIT(3))                                            /*!< GDMA reload mode */
#define USART_TXCTRL_CLR                     (BIT(4))                                            /*!< Clear TX channel and stop & clear transmission */
#define USART_TXCTRL_TXCNT                   (BITS(16,20))                                       /*!< Transmit watermark level */

/* USART_RXCTRL */
#define USART_RXCTRL_RXEN                    (BIT(0))                                            /*!< Receive enable */
#define USART_RXCTRL_CONTINUOUS              (BIT(1))                                            /*!< GDMA reload mode */
#define USART_RXCTRL_CLR                     (BIT(2))                                            /*!< Clear RX channel */
#define USART_RXCTRL_RXCNT                   (BITS(16,20))                                       /*!< Receive watermark level */

/* USART_IE */
#define USART_IE_TXIE                        (BIT(0))                                            /*!< TX watermark interrupt enable */
#define USART_IE_RXIE                        (BIT(1))                                            /*!< RX watermark interrupt enable */
#define USART_IE_OWIE                        (BIT(2))                                            /*!< RX overflow interrupt enable */
#define USART_IE_PARITY_ERR_IE               (BIT(3))                                            /*!< RX parity error interrupt enable */
#define USART_IE_CTSIE_RISE_IE               (BIT(4))                                            /*!< CTS rising edge interrupt enable */
#define USART_IE_CTSIE_FALL_IE               (BIT(5))                                            /*!< CTS falling edge interrupt enable */
#define USART_IE_RX_FRAME_ERR_IE             (BIT(6))                                            /*!< RX frame error interrupt enable */
#define USART_IE_STOP_BIT_ERR_IE             (BIT(7))                                            /*!< Stop bit error interrupt */
#define USART_IE_RX_IDLE_TOUT_IE             (BIT(10))                                           /*!< RX idle timeout interrupt enable */
#define USART_IE_RX_WM_TOUT_IE               (BIT(11))                                           /*!< RX watermark timeout interrupt enable */
#define USART_IE_TX_EOT_IE                   (BIT(12))                                           /*!< TX transfer complete interrupt enable */
#define USART_IE_RX_EOT_IE                   (BIT(13))                                           /*!< RX transfer complete interrupt enable */

/* USART_STA */
#define USART_STA_TXIP                       (BIT(0))                                            /*!< TX interrupt pending (FIFO level < TXCNT) */
#define USART_STA_RXIP                       (BIT(1))                                            /*!< RX interrupt pending (FIFO level > RXCNT) */
#define USART_STA_TX_BUSY                    (BIT(2))                                            /*!< TX busy */
#define USART_STA_RX_BUSY                    (BIT(3))                                            /*!< RX busy */
#define USART_STA_RX_ERROR_FLOW              (BIT(4))                                            /*!< RX overflow error */
#define USART_STA_RX_ERROR_PARITY            (BIT(5))                                            /*!< RX parity error */
#define USART_STA_CTS_RISE_FLAG              (BIT(6))                                            /*!< CTS rising edge detected */
#define USART_STA_CTS_FALL_FLAG              (BIT(7))                                            /*!< CTS falling edge detected */
#define USART_STA_RX_FRAME_ERR_FLAG          (BIT(8))                                            /*!< Stop bit low-level error, RX Frame Error Flag */
#define USART_STA_STOP_BIT_ERR_FLAG          (BIT(9))                                            /*!< Stop bit length mismatch error */
#define USART_STA_RX_IDLE_TOUT_STA           (BIT(12))                                           /*!< RX idle timeout status */
#define USART_STA_RX_WM_TOUT_STA             (BIT(13))                                           /*!< RX watermark timeout status */
#define USART_STA_FULL                       (BIT(14))                                           /*!< TX FIFO full */
#define USART_STA_EMPTY                      (BIT(15))                                           /*!< RX FIFO empty */
#define USART_STA_TX_EOT                     (BIT(16))                                           /*!< TX transfer complete */
#define USART_STA_RX_EOT                     (BIT(17))                                           /*!< RX transfer complete */

/* USART_DIV */
#define USART_DIV_BAUD_DIV                   (BITS(0,20))                                        /*!< Fractional mode uses bits [20:3] for integer, [2:0] for fraction (0.000-0.875). Must be equal or greater than 2. */

/* USART_SETUP */
#define USART_SETUP_PARITY_EN                (BIT(0))                                            /*!< Parity enable */
#define USART_SETUP_PARITY_SEL               (BIT(1))                                            /*!< Parity type */
#define USART_SETUP_SEPARATE_EN              (BIT(2))                                            /*!< Separate TX/RX DMA control */
#define USART_SETUP_CLEAN_FIFO               (BIT(3))                                            /*!< Clear RX FIFO */
#define USART_SETUP_BIT_LENGTH               (BITS(4,6))                                         /*!< Data bits */
#define USART_SETUP_CLK_EN                   (BIT(7))                                            /*!< CK pin enable */
#define USART_SETUP_CPOL                     (BIT(8))                                            /*!< Clock polarity */
#define USART_SETUP_CPHA                     (BIT(9))                                            /*!< Clock phase */
#define USART_SETUP_LBCL                     (BIT(10))                                           /*!< Last bit clock pulse */
#define USART_SETUP_HDSEL                    (BIT(11))                                           /*!< Half-duplex mode */
#define USART_SETUP_RTSE                     (BIT(13))                                           /*!< RTS flow control */
#define USART_SETUP_CTSE                     (BIT(14))                                           /*!< CTS flow control */
#define USART_SETUP_DMA_EN                   (BIT(15))                                           /*!< TX DMA mode (or global if SEPARATE_EN=0) */
#define USART_SETUP_VOTE_MAJORITY_EN         (BIT(16))                                           /*!< 2/3 majority vote */
#define USART_SETUP_SAMPLE_POSITION          (BITS(17,18))                                       /*!< Sampling point */
#define USART_SETUP_FRACTION_BAUD            (BIT(19))                                           /*!< Fractional baud enable */
#define USART_SETUP_RX_DMA_EN                (BIT(20))                                           /*!< RX DMA mode (when SEPARATE_EN=1) */

/* USART_RX_SIZE */
#define USART_RX_SIZE_RX_SIZE                (BITS(0,19))                                        /*!< RX Transfer Size Register */

/* USART_TX_SIZE */
#define USART_TX_SIZE_TX_SIZE                (BITS(0,19))                                        /*!< TX Transfer Size Register */

/* USART_RX_IDLE */
#define USART_RX_IDLE_IDLE_TOUT_NUM          (BITS(0,31))                                        /*!< Timeout value for idle detection */

/* USART_RX_WM */
#define USART_RX_WM_WM_TOUT_NUM              (BITS(0,31))                                        /*!< Timeout value for RX watermark */

/* USART_RX_FIFO_LEFT_ENTRY */
#define USART_RX_FIFO_LEFT_ENTRY_LEFT_ENTRY  (BITS(0,4))                                         /*!< Free entries in RX FIFO */

/* USART_TX_FIFO_LEFT_ENTRY */
#define USART_TX_FIFO_LEFT_ENTRY_LEFT_ENTRY  (BITS(0,4))                                         /*!< Free entries in TX FIFO */

/* USART_RX_LOW_LEVEL_CNT */
#define USART_RX_LOW_LEVEL_CNT_RX_LOW_LEVEL_LENGTH  (BITS(0,19))                                        /*!< Low-level duration in clock cycles */
#define USART_RX_LOW_LEVEL_CNT_RX_HIGH_LEVEL_DETECT  (BIT(31))                                           /*!< Low-Level rising edge detected */

/* constants definitions */
/* USART_TXDATA_TXDATA */
#define USART_TXDATA_TXDATA_POS              (0U)                                                /*!< USART_TXDATA_TXDATA bit offset */

/* USART_RXDATA_RXDATA */
#define USART_RXDATA_RXDATA_POS              (0U)                                                /*!< USART_RXDATA_RXDATA bit offset */

/* USART_TXCTRL_TXEN */
#define USART_TXCTRL_TXEN_POS                (0U)                                                /*!< USART_TXCTRL_TXEN bit offset */

/* USART_TXCTRL_CFG_STOP_BIT */
#define USART_TXCTRL_CFG_STOP_BIT_POS        (1U)                                                /*!< USART_TXCTRL_CFG_STOP_BIT bit offset */
#define USART_TXCTRL_CFG_STOP_BIT_CFG(regval)  (USART_TXCTRL_CFG_STOP_BIT & ((uint32_t)(regval) << USART_TXCTRL_CFG_STOP_BIT_POS))
#define USART_TXCTRL_CFG_STOP_BIT_0_5        (USART_TXCTRL_CFG_STOP_BIT_CFG(0x0))                /*!< 0.5 stop bit */
#define USART_TXCTRL_CFG_STOP_BIT_1          (USART_TXCTRL_CFG_STOP_BIT_CFG(0x1))                /*!< 1 stop bit */
#define USART_TXCTRL_CFG_STOP_BIT_1_5        (USART_TXCTRL_CFG_STOP_BIT_CFG(0x2))                /*!< 1.5 stop bits */
#define USART_TXCTRL_CFG_STOP_BIT_2          (USART_TXCTRL_CFG_STOP_BIT_CFG(0x3))                /*!< 2 stop bits */

/* USART_TXCTRL_CONTINUOUS */
#define USART_TXCTRL_CONTINUOUS_POS          (3U)                                                /*!< USART_TXCTRL_CONTINUOUS bit offset */

/* USART_TXCTRL_CLR */
#define USART_TXCTRL_CLR_POS                 (4U)                                                /*!< USART_TXCTRL_CLR bit offset */

/* USART_TXCTRL_TXCNT */
#define USART_TXCTRL_TXCNT_POS               (16U)                                               /*!< USART_TXCTRL_TXCNT bit offset */

/* USART_RXCTRL_RXEN */
#define USART_RXCTRL_RXEN_POS                (0U)                                                /*!< USART_RXCTRL_RXEN bit offset */

/* USART_RXCTRL_CONTINUOUS */
#define USART_RXCTRL_CONTINUOUS_POS          (1U)                                                /*!< USART_RXCTRL_CONTINUOUS bit offset */

/* USART_RXCTRL_CLR */
#define USART_RXCTRL_CLR_POS                 (2U)                                                /*!< USART_RXCTRL_CLR bit offset */

/* USART_RXCTRL_RXCNT */
#define USART_RXCTRL_RXCNT_POS               (16U)                                               /*!< USART_RXCTRL_RXCNT bit offset */

/* USART_IE_TXIE */
#define USART_IE_TXIE_POS                    (0U)                                                /*!< USART_IE_TXIE bit offset */

/* USART_IE_RXIE */
#define USART_IE_RXIE_POS                    (1U)                                                /*!< USART_IE_RXIE bit offset */

/* USART_IE_OWIE */
#define USART_IE_OWIE_POS                    (2U)                                                /*!< USART_IE_OWIE bit offset */

/* USART_IE_PARITY_ERR_IE */
#define USART_IE_PARITY_ERR_IE_POS           (3U)                                                /*!< USART_IE_PARITY_ERR_IE bit offset */

/* USART_IE_CTSIE_RISE_IE */
#define USART_IE_CTSIE_RISE_IE_POS           (4U)                                                /*!< USART_IE_CTSIE_RISE_IE bit offset */

/* USART_IE_CTSIE_FALL_IE */
#define USART_IE_CTSIE_FALL_IE_POS           (5U)                                                /*!< USART_IE_CTSIE_FALL_IE bit offset */

/* USART_IE_RX_FRAME_ERR_IE */
#define USART_IE_RX_FRAME_ERR_IE_POS         (6U)                                                /*!< USART_IE_RX_FRAME_ERR_IE bit offset */

/* USART_IE_STOP_BIT_ERR_IE */
#define USART_IE_STOP_BIT_ERR_IE_POS         (7U)                                                /*!< USART_IE_STOP_BIT_ERR_IE bit offset */

/* USART_IE_RX_IDLE_TOUT_IE */
#define USART_IE_RX_IDLE_TOUT_IE_POS         (10U)                                               /*!< USART_IE_RX_IDLE_TOUT_IE bit offset */

/* USART_IE_RX_WM_TOUT_IE */
#define USART_IE_RX_WM_TOUT_IE_POS           (11U)                                               /*!< USART_IE_RX_WM_TOUT_IE bit offset */

/* USART_IE_TX_EOT_IE */
#define USART_IE_TX_EOT_IE_POS               (12U)                                               /*!< USART_IE_TX_EOT_IE bit offset */

/* USART_IE_RX_EOT_IE */
#define USART_IE_RX_EOT_IE_POS               (13U)                                               /*!< USART_IE_RX_EOT_IE bit offset */

/* USART_STA_TXIP */
#define USART_STA_TXIP_POS                   (0U)                                                /*!< USART_STA_TXIP bit offset */

/* USART_STA_RXIP */
#define USART_STA_RXIP_POS                   (1U)                                                /*!< USART_STA_RXIP bit offset */

/* USART_STA_TX_BUSY */
#define USART_STA_TX_BUSY_POS                (2U)                                                /*!< USART_STA_TX_BUSY bit offset */

/* USART_STA_RX_BUSY */
#define USART_STA_RX_BUSY_POS                (3U)                                                /*!< USART_STA_RX_BUSY bit offset */

/* USART_STA_RX_ERROR_FLOW */
#define USART_STA_RX_ERROR_FLOW_POS          (4U)                                                /*!< USART_STA_RX_ERROR_FLOW bit offset */

/* USART_STA_RX_ERROR_PARITY */
#define USART_STA_RX_ERROR_PARITY_POS        (5U)                                                /*!< USART_STA_RX_ERROR_PARITY bit offset */

/* USART_STA_CTS_RISE_FLAG */
#define USART_STA_CTS_RISE_FLAG_POS          (6U)                                                /*!< USART_STA_CTS_RISE_FLAG bit offset */

/* USART_STA_CTS_FALL_FLAG */
#define USART_STA_CTS_FALL_FLAG_POS          (7U)                                                /*!< USART_STA_CTS_FALL_FLAG bit offset */

/* USART_STA_RX_FRAME_ERR_FLAG */
#define USART_STA_RX_FRAME_ERR_FLAG_POS      (8U)                                                /*!< USART_STA_RX_FRAME_ERR_FLAG bit offset */

/* USART_STA_STOP_BIT_ERR_FLAG */
#define USART_STA_STOP_BIT_ERR_FLAG_POS      (9U)                                                /*!< USART_STA_STOP_BIT_ERR_FLAG bit offset */

/* USART_STA_RX_IDLE_TOUT_STA */
#define USART_STA_RX_IDLE_TOUT_STA_POS       (12U)                                               /*!< USART_STA_RX_IDLE_TOUT_STA bit offset */

/* USART_STA_RX_WM_TOUT_STA */
#define USART_STA_RX_WM_TOUT_STA_POS         (13U)                                               /*!< USART_STA_RX_WM_TOUT_STA bit offset */

/* USART_STA_FULL */
#define USART_STA_FULL_POS                   (14U)                                               /*!< USART_STA_FULL bit offset */

/* USART_STA_EMPTY */
#define USART_STA_EMPTY_POS                  (15U)                                               /*!< USART_STA_EMPTY bit offset */

/* USART_STA_TX_EOT */
#define USART_STA_TX_EOT_POS                 (16U)                                               /*!< USART_STA_TX_EOT bit offset */

/* USART_STA_RX_EOT */
#define USART_STA_RX_EOT_POS                 (17U)                                               /*!< USART_STA_RX_EOT bit offset */

/* USART_DIV_BAUD_DIV */
#define USART_DIV_BAUD_DIV_POS               (0U)                                                /*!< USART_DIV_BAUD_DIV bit offset */

/* USART_SETUP_PARITY_EN */
#define USART_SETUP_PARITY_EN_POS            (0U)                                                /*!< USART_SETUP_PARITY_EN bit offset */

/* USART_SETUP_PARITY_SEL */
#define USART_SETUP_PARITY_SEL_POS           (1U)                                                /*!< USART_SETUP_PARITY_SEL bit offset */
#define USART_SETUP_PARITY_SEL_EVEN          ((uint32_t)0x00000000U)                             /*!< Even parity */
#define USART_SETUP_PARITY_SEL_ODD           (USART_SETUP_PARITY_SEL)                            /*!< Odd parity */

/* USART_SETUP_SEPARATE_EN */
#define USART_SETUP_SEPARATE_EN_POS          (2U)                                                /*!< USART_SETUP_SEPARATE_EN bit offset */

/* USART_SETUP_CLEAN_FIFO */
#define USART_SETUP_CLEAN_FIFO_POS           (3U)                                                /*!< USART_SETUP_CLEAN_FIFO bit offset */

/* USART_SETUP_BIT_LENGTH */
#define USART_SETUP_BIT_LENGTH_POS           (4U)                                                /*!< USART_SETUP_BIT_LENGTH bit offset */
#define USART_SETUP_BIT_LENGTH_CFG(regval)  (USART_SETUP_BIT_LENGTH & ((uint32_t)(regval) << USART_SETUP_BIT_LENGTH_POS))
#define USART_SETUP_BIT_LENGTH_5B            (USART_SETUP_BIT_LENGTH_CFG(0x0))                   /*!< 5 bits data length */
#define USART_SETUP_BIT_LENGTH_6B            (USART_SETUP_BIT_LENGTH_CFG(0x1))                   /*!< 6 bits data length */
#define USART_SETUP_BIT_LENGTH_7B            (USART_SETUP_BIT_LENGTH_CFG(0x2))                   /*!< 7 bits data length */
#define USART_SETUP_BIT_LENGTH_8B            (USART_SETUP_BIT_LENGTH_CFG(0x3))                   /*!< 8 bits data length */
#define USART_SETUP_BIT_LENGTH_9B            (USART_SETUP_BIT_LENGTH_CFG(0x4))                   /*!< 9 bits data length */

/* USART_SETUP_CLK_EN */
#define USART_SETUP_CLK_EN_POS               (7U)                                                /*!< USART_SETUP_CLK_EN bit offset */

/* USART_SETUP_CPOL */
#define USART_SETUP_CPOL_POS                 (8U)                                                /*!< USART_SETUP_CPOL bit offset */
#define USART_SETUP_CPOL_LOW                 ((uint32_t)0x00000000U)                             /*!< Idle low */
#define USART_SETUP_CPOL_HIGH                (USART_SETUP_CPOL)                                  /*!< Idle high */

/* USART_SETUP_CPHA */
#define USART_SETUP_CPHA_POS                 (9U)                                                /*!< USART_SETUP_CPHA bit offset */
#define USART_SETUP_CPHA_1EDGE               ((uint32_t)0x00000000U)                             /*!< First edge */
#define USART_SETUP_CPHA_2EDGE               (USART_SETUP_CPHA)                                  /*!< Second edge */

/* USART_SETUP_LBCL */
#define USART_SETUP_LBCL_POS                 (10U)                                               /*!< USART_SETUP_LBCL bit offset */
#define USART_SETUP_LBCL_NO_OUTPUT           ((uint32_t)0x00000000U)                             /*!< No Output on CK pin */
#define USART_SETUP_LBCL_OUTPUT              (USART_SETUP_LBCL)                                  /*!< Output on CK pin */

/* USART_SETUP_HDSEL */
#define USART_SETUP_HDSEL_POS                (11U)                                               /*!< USART_SETUP_HDSEL bit offset */

/* USART_SETUP_RTSE */
#define USART_SETUP_RTSE_POS                 (13U)                                               /*!< USART_SETUP_RTSE bit offset */

/* USART_SETUP_CTSE */
#define USART_SETUP_CTSE_POS                 (14U)                                               /*!< USART_SETUP_CTSE bit offset */

/* USART_SETUP_DMA_EN */
#define USART_SETUP_DMA_EN_POS               (15U)                                               /*!< USART_SETUP_DMA_EN bit offset */

/* USART_SETUP_VOTE_MAJORITY_EN */
#define USART_SETUP_VOTE_MAJORITY_EN_POS     (16U)                                               /*!< USART_SETUP_VOTE_MAJORITY_EN bit offset */

/* USART_SETUP_SAMPLE_POSITION */
#define USART_SETUP_SAMPLE_POSITION_POS      (17U)                                               /*!< USART_SETUP_SAMPLE_POSITION bit offset */
#define USART_SETUP_SAMPLE_POSITION_CFG(regval)  (USART_SETUP_SAMPLE_POSITION & ((uint32_t)(regval) << USART_SETUP_SAMPLE_POSITION_POS))
#define USART_SETUP_SAMPLE_POSITION_SPREAD   (USART_SETUP_SAMPLE_POSITION_CFG(0x0))              /*!< Sampling point is spread */
#define USART_SETUP_SAMPLE_POSITION_DOWN     (USART_SETUP_SAMPLE_POSITION_CFG(0x1))              /*!< Sampling point is down */
#define USART_SETUP_SAMPLE_POSITION_UP       (USART_SETUP_SAMPLE_POSITION_CFG(0x2))              /*!< Sampling point is up */

/* USART_SETUP_FRACTION_BAUD */
#define USART_SETUP_FRACTION_BAUD_POS        (19U)                                               /*!< USART_SETUP_FRACTION_BAUD bit offset */

/* USART_SETUP_RX_DMA_EN */
#define USART_SETUP_RX_DMA_EN_POS            (20U)                                               /*!< USART_SETUP_RX_DMA_EN bit offset */

/* USART_RX_SIZE_RX_SIZE */
#define USART_RX_SIZE_RX_SIZE_POS            (0U)                                                /*!< USART_RX_SIZE_RX_SIZE bit offset */

/* USART_TX_SIZE_TX_SIZE */
#define USART_TX_SIZE_TX_SIZE_POS            (0U)                                                /*!< USART_TX_SIZE_TX_SIZE bit offset */

/* USART_RX_IDLE_IDLE_TOUT_NUM */
#define USART_RX_IDLE_IDLE_TOUT_NUM_POS      (0U)                                                /*!< USART_RX_IDLE_IDLE_TOUT_NUM bit offset */

/* USART_RX_WM_WM_TOUT_NUM */
#define USART_RX_WM_WM_TOUT_NUM_POS          (0U)                                                /*!< USART_RX_WM_WM_TOUT_NUM bit offset */

/* USART_RX_FIFO_LEFT_ENTRY_LEFT_ENTRY */
#define USART_RX_FIFO_LEFT_ENTRY_LEFT_ENTRY_POS  (0U)                                                /*!< USART_RX_FIFO_LEFT_ENTRY_LEFT_ENTRY bit offset */

/* USART_TX_FIFO_LEFT_ENTRY_LEFT_ENTRY */
#define USART_TX_FIFO_LEFT_ENTRY_LEFT_ENTRY_POS  (0U)                                                /*!< USART_TX_FIFO_LEFT_ENTRY_LEFT_ENTRY bit offset */

/* USART_RX_LOW_LEVEL_CNT_RX_LOW_LEVEL_LENGTH */
#define USART_RX_LOW_LEVEL_CNT_RX_LOW_LEVEL_LENGTH_POS  (0U)                                                /*!< USART_RX_LOW_LEVEL_CNT_RX_LOW_LEVEL_LENGTH bit offset */

/* USART_RX_LOW_LEVEL_CNT_RX_HIGH_LEVEL_DETECT */
#define USART_RX_LOW_LEVEL_CNT_RX_HIGH_LEVEL_DETECT_POS  (31U)                                               /*!< USART_RX_LOW_LEVEL_CNT_RX_HIGH_LEVEL_DETECT bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_USART_H */
