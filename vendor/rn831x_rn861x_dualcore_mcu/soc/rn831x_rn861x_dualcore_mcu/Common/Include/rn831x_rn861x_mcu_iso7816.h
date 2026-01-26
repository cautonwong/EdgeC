/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_iso7816.h
 * @author  Renergy Software Team
 * @brief   ISO7816 module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_ISO7816_H
#define _RN831x_RN861x_MCU_ISO7816_H

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

/* ISO7816 */
typedef struct
{
    __IO uint32_t CTRL;                      /*!< offset: 0x000 info: Control Register */
    __IO uint32_t CLK;                       /*!< offset: 0x004 info: Clock Configuration Register */
    __IO uint32_t BDDIV;                     /*!< offset: 0x008 info: Baudrate coefficients Register */
    __IO uint32_t STA;                       /*!< offset: 0x00C info: Status Indicator Register */
    __IO uint32_t DATA;                      /*!< offset: 0x010 info: Transceiver Register */
} ISO7816_TypeDef;

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
#define ISO78160_CORE0_BASE (0x20030000U)
#define ISO78161_CORE0_BASE (0x20034000U)
#define ISO78160_CORE1_BASE (0xA0030000U)
#define ISO78161_CORE1_BASE (0xA0034000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define ISO78160_CORE0 ((ISO7816_TypeDef *)  ISO78160_CORE0_BASE)
#define ISO78161_CORE0 ((ISO7816_TypeDef *)  ISO78161_CORE0_BASE)
#define ISO78160_CORE1 ((ISO7816_TypeDef *)  ISO78160_CORE1_BASE)
#define ISO78161_CORE1 ((ISO7816_TypeDef *)  ISO78161_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define ISO78160  ISO78160_CORE0
#define ISO78161  ISO78161_CORE0
#endif
#if defined(CORE1)
#define ISO78160  ISO78160_CORE1
#define ISO78161  ISO78161_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** ISO7816 Instances *******************************/
#define IS_ISO7816_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == ISO78160_CORE0) || \
                                              ((INSTANCE) == ISO78161_CORE0) || \
                                              ((INSTANCE) == ISO78160_CORE1) || \
                                              ((INSTANCE) == ISO78161_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* ISO7816_CTRL */
#define ISO7816_CTRL_EN                      (BIT(0))                                            /*!< Controller enable */
#define ISO7816_CTRL_DIRSEL                  (BIT(1))                                            /*!< Data encoding method selection */
#define ISO7816_CTRL_TX_EN                   (BIT(2))                                            /*!< Transmit data enable */
#define ISO7816_CTRL_RX_EN                   (BIT(3))                                            /*!< Receive data enable */
#define ISO7816_CTRL_TX_IRQ_EN               (BIT(4))                                            /*!< Transmit interrupt enable */
#define ISO7816_CTRL_RX_IRQ_EN               (BIT(5))                                            /*!< Receive interrupt enable */
#define ISO7816_CTRL_ERR_IRQ_EN              (BIT(6))                                            /*!< Transfer error interrupt enable */
#define ISO7816_CTRL_BGT_EN                  (BIT(7))                                            /*!< Data received and sent BGT control */
#define ISO7816_CTRL_PARSEL                  (BITS(8,10))                                        /*!< Parity selection, configure Parity */
#define ISO7816_CTRL_ERRWTH                  (BITS(11,12))                                       /*!< Error signal width selection */
#define ISO7816_CTRL_RXPAR_ESEL              (BIT(13))                                           /*!< Receive data parity check error handling methods selection, enable error signal (NACK) transmission when data check error */
#define ISO7816_CTRL_REP_CNT                 (BITS(14,16))                                       /*!< Automatic retransmission times when data parity check error occurs */
#define ISO7816_CTRL_EGT                     (BITS(17,24))                                       /*!< EGT width selection value (0~255), additional protection time N */
#define ISO7816_CTRL_IO_EN                   (BIT(25))                                           /*!< Data bidirectional enable */
#define ISO7816_CTRL_TX_GT                   (BIT(27))                                           /*!< Send data GT width selection */
#define ISO7816_CTRL_RX_GT                   (BIT(28))                                           /*!< Receive data GT width selection */
#define ISO7816_CTRL_OLD_EN                  (BIT(29))                                           /*!< OLD detection function enable */
#define ISO7816_CTRL_OLD_IRQ_EN              (BIT(30))                                           /*!< OLD Detection interrupt enable */
#define ISO7816_CTRL_CARD_CHECK_EN           (BIT(31))                                           /*!< Card pull-out detection enable, Only valid when OLD detection function is enabled */

/* ISO7816_CLK */
#define ISO7816_CLK_CLKDIV                   (BITS(0,2))                                         /*!< Clock output division coefficient selection */
#define ISO7816_CLK_CLKO_EN                  (BIT(3))                                            /*!< Card clock output enable */

/* ISO7816_BDDIV */
#define ISO7816_BDDIV_FD                     (BITS(0,7))                                         /*!< This 8 bits FI(b4-b7) and DI(b0-b3) transmitted by Answer To Resest (ATR), the configuration information is shown in table */
#define ISO7816_BDDIV_FDS                    (BITS(8,21))                                        /*!< Configured by software, can only be written when FDS_EN = 1, in other cases, it is 14d'372 */
#define ISO7816_BDDIV_FDS_EN                 (BIT(22))                                           /*!< Software configuration of F/D */

/* ISO7816_STA */
#define ISO7816_STA_TXIF                     (BIT(0))                                            /*!< Data transmit interrupt flag */
#define ISO7816_STA_RXIF                     (BIT(1))                                            /*!< Data received interrupt flag */
#define ISO7816_STA_OVL_IF                   (BIT(2))                                            /*!< Receive data overflow interrupt flag */
#define ISO7816_STA_COL_IF                   (BIT(3))                                            /*!< Transmit data conflict error interrupt flag */
#define ISO7816_STA_RXPAR_ERRIF              (BIT(4))                                            /*!< Receive data parity error flag */
#define ISO7816_STA_TXPAR_ERRIF              (BIT(5))                                            /*!< Transmit data parity error flag */
#define ISO7816_STA_TXBUSY                   (BIT(6))                                            /*!< Data sending busy flag, read only */
#define ISO7816_STA_RXBUSY                   (BIT(7))                                            /*!< Data receiving busy flag, read only */
#define ISO7816_STA_RX_FLAG                  (BIT(8))                                            /*!< Data receiving complete flag, read only */
#define ISO7816_STA_TX_FLAG                  (BIT(9))                                            /*!< Send buffer empty flag, read only */
#define ISO7816_STA_BDDIV_R                  (BIT(10))                                           /*!< Baud rate match indication bit, read only */
#define ISO7816_STA_FRAME_ERR                (BIT(11))                                           /*!< Receive data frame format error interrupt flag */
#define ISO7816_STA_OLD_FLAG                 (BIT(12))                                           /*!< Receive OLD signal interrupt flag, it is valid after OLD_EN is enabled */
#define ISO7816_STA_CARD_OUT_FLAG            (BIT(13))                                           /*!< Card pull-out detect flag, it is valid after CARD_CHECK_EN is enabled */

/* ISO7816_DATA */
#define ISO7816_DATA_DAT                     (BITS(0,7))                                         /*!< Data register */
#define ISO7816_DATA_DAT_BIT8                (BIT(8))                                            /*!< When parsel is in user defined mode, this data bit is parity */

/* constants definitions */
/* ISO7816_CTRL_EN */
#define ISO7816_CTRL_EN_POS                  (0U)                                                /*!< ISO7816_CTRL_EN bit offset */

/* ISO7816_CTRL_DIRSEL */
#define ISO7816_CTRL_DIRSEL_POS              (1U)                                                /*!< ISO7816_CTRL_DIRSEL bit offset */

/* ISO7816_CTRL_TX_EN */
#define ISO7816_CTRL_TX_EN_POS               (2U)                                                /*!< ISO7816_CTRL_TX_EN bit offset */

/* ISO7816_CTRL_RX_EN */
#define ISO7816_CTRL_RX_EN_POS               (3U)                                                /*!< ISO7816_CTRL_RX_EN bit offset */

/* ISO7816_CTRL_TX_IRQ_EN */
#define ISO7816_CTRL_TX_IRQ_EN_POS           (4U)                                                /*!< ISO7816_CTRL_TX_IRQ_EN bit offset */

/* ISO7816_CTRL_RX_IRQ_EN */
#define ISO7816_CTRL_RX_IRQ_EN_POS           (5U)                                                /*!< ISO7816_CTRL_RX_IRQ_EN bit offset */

/* ISO7816_CTRL_ERR_IRQ_EN */
#define ISO7816_CTRL_ERR_IRQ_EN_POS          (6U)                                                /*!< ISO7816_CTRL_ERR_IRQ_EN bit offset */

/* ISO7816_CTRL_BGT_EN */
#define ISO7816_CTRL_BGT_EN_POS              (7U)                                                /*!< ISO7816_CTRL_BGT_EN bit offset */

/* ISO7816_CTRL_PARSEL */
#define ISO7816_CTRL_PARSEL_POS              (8U)                                                /*!< ISO7816_CTRL_PARSEL bit offset */
#define ISO7816_CTRL_PARSEL_CFG(regval)  (ISO7816_CTRL_PARSEL & ((uint32_t)(regval) << ISO7816_CTRL_PARSEL_POS))
#define ISO7816_CTRL_PARSEL_PARITY_NONE      (ISO7816_CTRL_PARSEL_CFG(0x0))                      /*!< No parity */
#define ISO7816_CTRL_PARSEL_PARITY_ODD       (ISO7816_CTRL_PARSEL_CFG(0x1))                      /*!< Odd parity is selected */
#define ISO7816_CTRL_PARSEL_PARITY_EVEN      (ISO7816_CTRL_PARSEL_CFG(0x2))                      /*!< Even parity is selected */
#define ISO7816_CTRL_PARSEL_PARITY_ZERO      (ISO7816_CTRL_PARSEL_CFG(0x3))                      /*!< Fixed zero parity is selected */
#define ISO7816_CTRL_PARSEL_PARITY_ONE       (ISO7816_CTRL_PARSEL_CFG(0x4))                      /*!< Fixed one parity is selected */
#define ISO7816_CTRL_PARSEL_PARITY_USER_DEFINED  (ISO7816_CTRL_PARSEL_CFG(0x5))                      /*!< User defined parity is selected, */

/* ISO7816_CTRL_ERRWTH */
#define ISO7816_CTRL_ERRWTH_POS              (11U)                                               /*!< ISO7816_CTRL_ERRWTH bit offset */
#define ISO7816_CTRL_ERRWTH_CFG(regval)  (ISO7816_CTRL_ERRWTH & ((uint32_t)(regval) << ISO7816_CTRL_ERRWTH_POS))
#define ISO7816_CTRL_ERRWTH_2                (ISO7816_CTRL_ERRWTH_CFG(0x0))                      /*!< Error signal width 2 ETU selected */
#define ISO7816_CTRL_ERRWTH_1                (ISO7816_CTRL_ERRWTH_CFG(0x1))                      /*!< Error signal width 1 ETU selected */
#define ISO7816_CTRL_ERRWTH_1_5              (ISO7816_CTRL_ERRWTH_CFG(0x2))                      /*!< Error signal width 1.5 ETU selected */

/* ISO7816_CTRL_RXPAR_ESEL */
#define ISO7816_CTRL_RXPAR_ESEL_POS          (13U)                                               /*!< ISO7816_CTRL_RXPAR_ESEL bit offset */

/* ISO7816_CTRL_REP_CNT */
#define ISO7816_CTRL_REP_CNT_POS             (14U)                                               /*!< ISO7816_CTRL_REP_CNT bit offset */
#define ISO7816_CTRL_REP_CNT_CFG(regval)  (ISO7816_CTRL_REP_CNT & ((uint32_t)(regval) << ISO7816_CTRL_REP_CNT_POS))
#define ISO7816_CTRL_REP_CNT_NONE            (ISO7816_CTRL_REP_CNT_CFG(0x0))                     /*!< No automatic retransmission */
#define ISO7816_CTRL_REP_CNT_1               (ISO7816_CTRL_REP_CNT_CFG(0x1))                     /*!< Automatic retransmission count is 1 */
#define ISO7816_CTRL_REP_CNT_2               (ISO7816_CTRL_REP_CNT_CFG(0x2))                     /*!< Automatic retransmission count is 2 */
#define ISO7816_CTRL_REP_CNT_3               (ISO7816_CTRL_REP_CNT_CFG(0x3))                     /*!< Automatic retransmission count is 3 */
#define ISO7816_CTRL_REP_CNT_4               (ISO7816_CTRL_REP_CNT_CFG(0x4))                     /*!< Automatic retransmission count is 4 */
#define ISO7816_CTRL_REP_CNT_5               (ISO7816_CTRL_REP_CNT_CFG(0x5))                     /*!< Automatic retransmission count is 5 */
#define ISO7816_CTRL_REP_CNT_6               (ISO7816_CTRL_REP_CNT_CFG(0x6))                     /*!< Automatic retransmission count is 6 */
#define ISO7816_CTRL_REP_CNT_7               (ISO7816_CTRL_REP_CNT_CFG(0x7))                     /*!< Automatic retransmission count is 7 */

/* ISO7816_CTRL_EGT */
#define ISO7816_CTRL_EGT_POS                 (17U)                                               /*!< ISO7816_CTRL_EGT bit offset */

/* ISO7816_CTRL_IO_EN */
#define ISO7816_CTRL_IO_EN_POS               (25U)                                               /*!< ISO7816_CTRL_IO_EN bit offset */

/* ISO7816_CTRL_TX_GT */
#define ISO7816_CTRL_TX_GT_POS               (27U)                                               /*!< ISO7816_CTRL_TX_GT bit offset */

/* ISO7816_CTRL_RX_GT */
#define ISO7816_CTRL_RX_GT_POS               (28U)                                               /*!< ISO7816_CTRL_RX_GT bit offset */

/* ISO7816_CTRL_OLD_EN */
#define ISO7816_CTRL_OLD_EN_POS              (29U)                                               /*!< ISO7816_CTRL_OLD_EN bit offset */

/* ISO7816_CTRL_OLD_IRQ_EN */
#define ISO7816_CTRL_OLD_IRQ_EN_POS          (30U)                                               /*!< ISO7816_CTRL_OLD_IRQ_EN bit offset */

/* ISO7816_CTRL_CARD_CHECK_EN */
#define ISO7816_CTRL_CARD_CHECK_EN_POS       (31U)                                               /*!< ISO7816_CTRL_CARD_CHECK_EN bit offset */

/* ISO7816_CLK_CLKDIV */
#define ISO7816_CLK_CLKDIV_POS               (0U)                                                /*!< ISO7816_CLK_CLKDIV bit offset */
#define ISO7816_CLK_CLKDIV_CFG(regval)  (ISO7816_CLK_CLKDIV & ((uint32_t)(regval) << ISO7816_CLK_CLKDIV_POS))
#define ISO7816_CLK_CLKDIV_0                 (ISO7816_CLK_CLKDIV_CFG(0x0))                       /*!< Frequency division coefficient is 0 */
#define ISO7816_CLK_CLKDIV_2                 (ISO7816_CLK_CLKDIV_CFG(0x1))                       /*!< Frequency division coefficient is 2 */
#define ISO7816_CLK_CLKDIV_4                 (ISO7816_CLK_CLKDIV_CFG(0x2))                       /*!< Frequency division coefficient is 4 */
#define ISO7816_CLK_CLKDIV_8                 (ISO7816_CLK_CLKDIV_CFG(0x3))                       /*!< Frequency division coefficient is 8 */
#define ISO7816_CLK_CLKDIV_16                (ISO7816_CLK_CLKDIV_CFG(0x4))                       /*!< Frequency division coefficient is 16 */
#define ISO7816_CLK_CLKDIV_32                (ISO7816_CLK_CLKDIV_CFG(0x5))                       /*!< Frequency division coefficient is 32 */

/* ISO7816_CLK_CLKO_EN */
#define ISO7816_CLK_CLKO_EN_POS              (3U)                                                /*!< ISO7816_CLK_CLKO_EN bit offset */

/* ISO7816_BDDIV_FD */
#define ISO7816_BDDIV_FD_POS                 (0U)                                                /*!< ISO7816_BDDIV_FD bit offset */

/* ISO7816_BDDIV_FDS */
#define ISO7816_BDDIV_FDS_POS                (8U)                                                /*!< ISO7816_BDDIV_FDS bit offset */

/* ISO7816_BDDIV_FDS_EN */
#define ISO7816_BDDIV_FDS_EN_POS             (22U)                                               /*!< ISO7816_BDDIV_FDS_EN bit offset */

/* ISO7816_STA_TXIF */
#define ISO7816_STA_TXIF_POS                 (0U)                                                /*!< ISO7816_STA_TXIF bit offset */

/* ISO7816_STA_RXIF */
#define ISO7816_STA_RXIF_POS                 (1U)                                                /*!< ISO7816_STA_RXIF bit offset */

/* ISO7816_STA_OVL_IF */
#define ISO7816_STA_OVL_IF_POS               (2U)                                                /*!< ISO7816_STA_OVL_IF bit offset */

/* ISO7816_STA_COL_IF */
#define ISO7816_STA_COL_IF_POS               (3U)                                                /*!< ISO7816_STA_COL_IF bit offset */

/* ISO7816_STA_RXPAR_ERRIF */
#define ISO7816_STA_RXPAR_ERRIF_POS          (4U)                                                /*!< ISO7816_STA_RXPAR_ERRIF bit offset */

/* ISO7816_STA_TXPAR_ERRIF */
#define ISO7816_STA_TXPAR_ERRIF_POS          (5U)                                                /*!< ISO7816_STA_TXPAR_ERRIF bit offset */

/* ISO7816_STA_TXBUSY */
#define ISO7816_STA_TXBUSY_POS               (6U)                                                /*!< ISO7816_STA_TXBUSY bit offset */

/* ISO7816_STA_RXBUSY */
#define ISO7816_STA_RXBUSY_POS               (7U)                                                /*!< ISO7816_STA_RXBUSY bit offset */

/* ISO7816_STA_RX_FLAG */
#define ISO7816_STA_RX_FLAG_POS              (8U)                                                /*!< ISO7816_STA_RX_FLAG bit offset */

/* ISO7816_STA_TX_FLAG */
#define ISO7816_STA_TX_FLAG_POS              (9U)                                                /*!< ISO7816_STA_TX_FLAG bit offset */

/* ISO7816_STA_BDDIV_R */
#define ISO7816_STA_BDDIV_R_POS              (10U)                                               /*!< ISO7816_STA_BDDIV_R bit offset */

/* ISO7816_STA_FRAME_ERR */
#define ISO7816_STA_FRAME_ERR_POS            (11U)                                               /*!< ISO7816_STA_FRAME_ERR bit offset */

/* ISO7816_STA_OLD_FLAG */
#define ISO7816_STA_OLD_FLAG_POS             (12U)                                               /*!< ISO7816_STA_OLD_FLAG bit offset */

/* ISO7816_STA_CARD_OUT_FLAG */
#define ISO7816_STA_CARD_OUT_FLAG_POS        (13U)                                               /*!< ISO7816_STA_CARD_OUT_FLAG bit offset */

/* ISO7816_DATA_DAT */
#define ISO7816_DATA_DAT_POS                 (0U)                                                /*!< ISO7816_DATA_DAT bit offset */

/* ISO7816_DATA_DAT_BIT8 */
#define ISO7816_DATA_DAT_BIT8_POS            (8U)                                                /*!< ISO7816_DATA_DAT_BIT8 bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_ISO7816_H */
