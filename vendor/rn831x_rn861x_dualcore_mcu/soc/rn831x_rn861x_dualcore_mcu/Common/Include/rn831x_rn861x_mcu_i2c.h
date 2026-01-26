/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_i2c.h
 * @author  Renergy Software Team
 * @brief   I2C module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_I2C_H
#define _RN831x_RN861x_MCU_I2C_H

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

/* I2C */
typedef struct
{
    __IO uint32_t CTRL;                      /*!< offset: 0x000 info: Control Register */
    __IO uint32_t CLK;                       /*!< offset: 0x004 info: Clock Configuration Register */
    __IO uint32_t STA;                       /*!< offset: 0x008 info: Status Indicator Register */
    __IO uint32_t ADDR;                      /*!< offset: 0x00C info: Device Address Register */
    __IO uint32_t DATA;                      /*!< offset: 0x010 info: Transceiver Register */
} I2C_TypeDef;

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
#define I2C0_CORE0_BASE (0x20050000U)
#define I2C1_CORE0_BASE (0x20054000U)
#define I2C0_CORE1_BASE (0xA0050000U)
#define I2C1_CORE1_BASE (0xA0054000U)
#define I2C0_CORE0_BB_BASE (0x22A00000U)
#define I2C1_CORE0_BB_BASE (0x22A80000U)
#define I2C0_CORE1_BB_BASE (0xA2A00000U)
#define I2C1_CORE1_BB_BASE (0xA2A80000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define I2C0_CORE0 ((I2C_TypeDef *)      I2C0_CORE0_BASE)
#define I2C1_CORE0 ((I2C_TypeDef *)      I2C1_CORE0_BASE)
#define I2C0_CORE1 ((I2C_TypeDef *)      I2C0_CORE1_BASE)
#define I2C1_CORE1 ((I2C_TypeDef *)      I2C1_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define I2C0  I2C0_CORE0
#define I2C1  I2C1_CORE0
#endif
#if defined(CORE1)
#define I2C0  I2C0_CORE1
#define I2C1  I2C1_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** I2C Instances *******************************/
#define IS_I2C_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == I2C0_CORE0) || \
                                          ((INSTANCE) == I2C1_CORE0) || \
                                          ((INSTANCE) == I2C0_CORE1) || \
                                          ((INSTANCE) == I2C1_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* I2C_CTRL */
#define I2C_CTRL_EN                          (BIT(0))                                            /*!< Mode selection bit */
#define I2C_CTRL_BUSCON                      (BITS(1,2))                                         /*!< ACK Signal reception and transmission */
#define I2C_CTRL_IRQE                        (BIT(3))                                            /*!< Enable/disable interrupt */
#define I2C_CTRL_ACK                         (BIT(4))                                            /*!< Bus control generate, start/stop condition generate */
#define I2C_CTRL_MODE                        (BIT(5))                                            /*!< Enable module */

/* I2C_CLK */
#define I2C_CLK_CLKDIV                       (BITS(0,2))                                         /*!< Clock division parameter selection, set speed mode */

/* I2C_STA */
#define I2C_STA_STPD                         (BIT(0))                                            /*!< Stop signal detection interrupt flag */
#define I2C_STA_RX_NACK                      (BIT(1))                                            /*!< Received NACK interrupt flag */
#define I2C_STA_TRANC                        (BIT(2))                                            /*!< Transmission completion interrupt flag */
#define I2C_STA_TXEMPT                       (BIT(3))                                            /*!< Transmit data register empty error flag */
#define I2C_STA_OVF                          (BIT(4))                                            /*!< Receive overflow interrupt identifier bit  (slave mode) */
#define I2C_STA_COL                          (BIT(5))                                            /*!< Transmi/receive conflict interrupt identifier bit (slave mode) */
#define I2C_STA_BUSY                         (BIT(6))                                            /*!< Communication status flag */
#define I2C_STA_MATCH                        (BIT(7))                                            /*!< Address matching bit, will clear after detecting start/stop singal */
#define I2C_STA_DIR                          (BIT(8))                                            /*!< Read/Write direction */

/* I2C_ADDR */
#define I2C_ADDR_RW                          (BIT(0))                                            /*!< Master read/write direction control bit */
#define I2C_ADDR_SADR                        (BITS(1,7))                                         /*!< Device address, can not write during address transmission. Master mode, represents the slave device. Slave mode, use this address to compare with the address sent by master. */

/* I2C_DATA */
#define I2C_DATA_TRDATA                      (BITS(0,7))                                         /*!< Receive/send data. When host sends data, the cache should be empty before the data can be written */

/* constants definitions */
/* I2C_CTRL_EN */
#define I2C_CTRL_EN_POS                      (0U)                                                /*!< I2C_CTRL_EN bit offset */

/* I2C_CTRL_BUSCON */
#define I2C_CTRL_BUSCON_POS                  (1U)                                                /*!< I2C_CTRL_BUSCON bit offset */
#define I2C_CTRL_BUSCON_CFG(regval)  (I2C_CTRL_BUSCON & ((uint32_t)(regval) << I2C_CTRL_BUSCON_POS))
#define I2C_CTRL_BUSCON_NONE                 (I2C_CTRL_BUSCON_CFG(0x0))                          /*!< No Action */
#define I2C_CTRL_BUSCON_START                (I2C_CTRL_BUSCON_CFG(0x1))                          /*!< Start Generation */
#define I2C_CTRL_BUSCON_STOP                 (I2C_CTRL_BUSCON_CFG(0x2))                          /*!< Stop Generation */

/* I2C_CTRL_IRQE */
#define I2C_CTRL_IRQE_POS                    (3U)                                                /*!< I2C_CTRL_IRQE bit offset */

/* I2C_CTRL_ACK */
#define I2C_CTRL_ACK_POS                     (4U)                                                /*!< I2C_CTRL_ACK bit offset */

/* I2C_CTRL_MODE */
#define I2C_CTRL_MODE_POS                    (5U)                                                /*!< I2C_CTRL_MODE bit offset */

/* I2C_CLK_CLKDIV */
#define I2C_CLK_CLKDIV_POS                   (0U)                                                /*!< I2C_CLK_CLKDIV bit offset */
#define I2C_CLK_CLKDIV_CFG(regval)  (I2C_CLK_CLKDIV & ((uint32_t)(regval) << I2C_CLK_CLKDIV_POS))
#define I2C_CLK_CLKDIV_1M8_STANDARD_MODE     (I2C_CLK_CLKDIV_CFG(0x2))                           /*!< Standard mode in system clock 1.8432MHz, fast mode is not supported in this clock */
#define I2C_CLK_CLKDIV_3M6_STANDARD_MODE     (I2C_CLK_CLKDIV_CFG(0x3))                           /*!< Standard mode in system clock 3.6864MHz, fast mode is not supported in this clock */
#define I2C_CLK_CLKDIV_7M3_STANDARD_MODE     (I2C_CLK_CLKDIV_CFG(0x4))                           /*!< Standard mode in system clock 7.3728MHz */
#define I2C_CLK_CLKDIV_7M3_FAST_MODE         (I2C_CLK_CLKDIV_CFG(0x2))                           /*!< Fast mode in system clock 7.3728MHz */
#define I2C_CLK_CLKDIV_14M7_STANDARD_MODE    (I2C_CLK_CLKDIV_CFG(0x5))                           /*!< Standard mode in system clock 14.7456MHz */
#define I2C_CLK_CLKDIV_14M7_FAST_MODE        (I2C_CLK_CLKDIV_CFG(0x3))                           /*!< Fast mode in system clock 14.7456MHz */
#define I2C_CLK_CLKDIV_29M4_STANDARD_MODE    (I2C_CLK_CLKDIV_CFG(0x6))                           /*!< Standard mode in system clock 29.4912MHz */
#define I2C_CLK_CLKDIV_29M4_FAST_MODE        (I2C_CLK_CLKDIV_CFG(0x4))                           /*!< Fast mode in system clock 29.4912MHz */
#define I2C_CLK_CLKDIV_58M9_STANDARD_MODE    (I2C_CLK_CLKDIV_CFG(0x7))                           /*!< Standard mode in system clock 58.9824MHz */
#define I2C_CLK_CLKDIV_58M9_FAST_MODE        (I2C_CLK_CLKDIV_CFG(0x5))                           /*!< Fast mode in system clock 58.9824MHz */

/* I2C_STA_STPD */
#define I2C_STA_STPD_POS                     (0U)                                                /*!< I2C_STA_STPD bit offset */

/* I2C_STA_RX_NACK */
#define I2C_STA_RX_NACK_POS                  (1U)                                                /*!< I2C_STA_RX_NACK bit offset */

/* I2C_STA_TRANC */
#define I2C_STA_TRANC_POS                    (2U)                                                /*!< I2C_STA_TRANC bit offset */

/* I2C_STA_TXEMPT */
#define I2C_STA_TXEMPT_POS                   (3U)                                                /*!< I2C_STA_TXEMPT bit offset */

/* I2C_STA_OVF */
#define I2C_STA_OVF_POS                      (4U)                                                /*!< I2C_STA_OVF bit offset */

/* I2C_STA_COL */
#define I2C_STA_COL_POS                      (5U)                                                /*!< I2C_STA_COL bit offset */

/* I2C_STA_BUSY */
#define I2C_STA_BUSY_POS                     (6U)                                                /*!< I2C_STA_BUSY bit offset */

/* I2C_STA_MATCH */
#define I2C_STA_MATCH_POS                    (7U)                                                /*!< I2C_STA_MATCH bit offset */

/* I2C_STA_DIR */
#define I2C_STA_DIR_POS                      (8U)                                                /*!< I2C_STA_DIR bit offset */

/* I2C_ADDR_RW */
#define I2C_ADDR_RW_POS                      (0U)                                                /*!< I2C_ADDR_RW bit offset */

/* I2C_ADDR_SADR */
#define I2C_ADDR_SADR_POS                    (1U)                                                /*!< I2C_ADDR_SADR bit offset */

/* I2C_DATA_TRDATA */
#define I2C_DATA_TRDATA_POS                  (0U)                                                /*!< I2C_DATA_TRDATA bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_I2C_H */
