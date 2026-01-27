/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_qspi.h
 * @author  Renergy Software Team
 * @brief   QSPI module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_QSPI_H
#define _RN831x_RN861x_MCU_QSPI_H

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

/* QSPI */
typedef struct
{
    __IO uint32_t SCKDIV;                    /*!< offset: 0x000 info: Clock Divisor Register */
    __IO uint32_t SCKMODE;                   /*!< offset: 0x004 info: Clock Mode Register (Master/Slave) */
    __IO uint32_t SCKSAMPLE;                 /*!< offset: 0x008 info: Sample Point Configuration Register (DDR Mode) */
    __IO uint32_t FORCE;                     /*!< offset: 0x00C info: Pin Output Force Register */
    __IO uint32_t CSID;                      /*!< offset: 0x010 info: Chip Select ID Register */
    __IO uint32_t CSDEF;                     /*!< offset: 0x014 info: Chip Select Default Value Register */
    __IO uint32_t CSMODE;                    /*!< offset: 0x018 info: Chip Select Mode Register */
    __IO uint32_t VERSION;                   /*!< offset: 0x01C info: Version Register */
    __IO uint32_t ADDR_WRAP;                 /*!< offset: 0x020 info: SPI Flash Start Address Offset Register */
    __IO uint32_t BOUNDARY_CFG;              /*!< offset: 0x024 info: SPI Flash Address Boundary Configuration Register */
    __IO uint32_t DELAY0;                    /*!< offset: 0x028 info: Delay Control Register 0 */
    __IO uint32_t DELAY1;                    /*!< offset: 0x02C info: Delay Control Register 1 */
    __IO uint32_t FIFO_NUM;                  /*!< offset: 0x030 info: FIFO Valid Data Count Register */
    __IO uint32_t TSIZE;                     /*!< offset: 0x034 info: DMA Transfer Data Length Register. */
    __IO uint32_t RSIZE;                     /*!< offset: 0x038 info: DMA Receive Data Length Register. */
    __IO uint32_t RESERVED0;                 /*!< offset: 0x03C */
    __IO uint32_t FMT;                       /*!< offset: 0x040 info: Frame Format Register (Master/Slave, Non-XIP Mode) */
    __IO uint32_t RESERVED1;                 /*!< offset: 0x044 */
    __IO uint32_t TXDATA;                    /*!< offset: 0x048 info: Transmit Data Register (Master/Slave) */
    __IO uint32_t RXDATA;                    /*!< offset: 0x04C info: Receive Data Register (Master/Slave) */
    __IO uint32_t TX_MARK;                   /*!< offset: 0x050 info: Transmit FIFO Threshold Register (Master/Slave) */
    __IO uint32_t RX_MARK;                   /*!< offset: 0x054 info: Receive FIFO Threshold Register (Master/Slave) */
    __IO uint32_t RESERVED2[2];              /*!< offset: 0x058 ~ 05C */
    __IO uint32_t FCTRL;                     /*!< offset: 0x060 info: SPI Flash Interface Control Register (XIP) */
    __IO uint32_t FFMT;                      /*!< offset: 0x064 info: SPI Flash Command Format Register (XIP) */
    __IO uint32_t RESERVED3[2];              /*!< offset: 0x068 ~ 06C */
    __IO uint32_t IE;                        /*!< offset: 0x070 info: Interrupt Enable Register (Master/Slave) */
    __IO uint32_t IP;                        /*!< offset: 0x074 info: Interrupt Pending Register (Master/Slave) */
    __IO uint32_t FFMT1;                     /*!< offset: 0x078 info: SPI Flash Command Format Register 1 (XIP) */
    __IO uint32_t STA;                       /*!< offset: 0x07C info: Status Register */
    __IO uint32_t SDR_SCKSAMPLE;             /*!< offset: 0x080 info: Sample Point Configuration Register (SDR Mode) */
    __IO uint32_t CR;                        /*!< offset: 0x084 info: Control Register */
} QSPI_TypeDef;

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
#define QSPI0_BASE      (0xB0088000U)
#define QSPI1_BASE      (0xB008C000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define QSPI0      ((QSPI_TypeDef *)     QSPI0_BASE)
#define QSPI1      ((QSPI_TypeDef *)     QSPI1_BASE)


/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** QSPI Instances *******************************/
#define IS_QSPI_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == QSPI0) || \
                                           ((INSTANCE) == QSPI1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* QSPI_SCKDIV */
#define QSPI_SCKDIV_SCKDIV                   (BITS(0,11))                                        /*!< Clock divisor factor selection bits */

/* QSPI_SCKMODE */
#define QSPI_SCKMODE_CPHA                    (BIT(0))                                            /*!< Clock (SCK) Phase selection bit. In DDR mode, only CPHA=0 is supported */
#define QSPI_SCKMODE_CPOL                    (BIT(1))                                            /*!< Clock (SCK) Polarity selection bit. */

/* QSPI_SCKSAMPLE */
#define QSPI_SCKSAMPLE_SCKSMP                (BITS(0,11))                                        /*!< DDR Mode SCK sample point delay selection bit */

/* QSPI_FORCE */
#define QSPI_FORCE_FORCE_EN                  (BIT(0))                                            /*!< Force output enable for non-data pins in different interface modes. */
#define QSPI_FORCE_FORCE_WP                  (BIT(1))                                            /*!< WP (IO2) pin output state selection bit (when not used as data line). Invalid in Quad-line mode. */

/* QSPI_CSID */
#define QSPI_CSID_CSID                       (BITS(0,2))                                         /*!< Chip Select device ID selection bit */

/* QSPI_CSDEF */
#define QSPI_CSDEF_CSDEF                     (BIT(0))                                            /*!< CS0 default (idle) state selection bit (Master mode) */

/* QSPI_CSMODE */
#define QSPI_CSMODE_CSMODE                   (BITS(0,1))                                         /*!< Chip Select mode selection bits. Use Hold mode for continuous data transfer, switch to Disable mode after transfer completes. */

/* QSPI_VERSION */
#define QSPI_VERSION_VERSION                 (BITS(0,31))                                        /*!< QSPI Version 2.1.0 */

/* QSPI_ADDR_WRAP */
#define QSPI_ADDR_WRAP_OFFSET                (BITS(0,31))                                        /*!< SPI Flash start address offset value */

/* QSPI_BOUNDARY_CFG */
#define QSPI_BOUNDARY_CFG_BOUNDARY           (BITS(0,31))                                        /*!< XIP boundary address configuration register */

/* QSPI_DELAY0 */
#define QSPI_DELAY0_CSSCK                    (BITS(0,7))                                         /*!< Delay cycles from CS assertion (falling edge) to first valid SCK rising edge. */
#define QSPI_DELAY0_SCKCS                    (BITS(16,23))                                       /*!< Delay cycles from last valid SCK edge to CS deassertion (rising edge). */

/* QSPI_DELAY1 */
#define QSPI_DELAY1_INTERCS                  (BITS(0,7))                                         /*!< Minimum delay cycles from CS deassertion (rising edge) to next CS assertion (falling edge). */
#define QSPI_DELAY1_INTERXFR                 (BITS(16,23))                                       /*!< Delay cycles between two data frames. */

/* QSPI_FIFO_NUM */
#define QSPI_FIFO_NUM_TX_NUM                 (BITS(0,2))                                         /*!< TX FIFO valid data count. */
#define QSPI_FIFO_NUM_RX_NUM                 (BITS(16,18))                                       /*!< RX FIFO valid data count. */

/* QSPI_TSIZE */
#define QSPI_TSIZE_TSIZE                     (BITS(0,31))                                        /*!< transfer data length (in beats) */

/* QSPI_RSIZE */
#define QSPI_RSIZE_RSIZE                     (BITS(0,31))                                        /*!< receive data length (in beats). */

/* QSPI_FMT */
#define QSPI_FMT_PROTO                       (BITS(0,1))                                         /*!< Interface mode (number of data lines) selection bits. */
#define QSPI_FMT_ENDIAN                      (BIT(2))                                            /*!< Bit endianness (transmission order within byte). */
#define QSPI_FMT_DIR                         (BIT(3))                                            /*!< Pin data transfer direction selection bit (for configurable I/O pins). */
#define QSPI_FMT_PROTO_HI                    (BIT(4))                                            /*!< High bit extension for QSPI_FMT.PROTO[1:0] (See PROTO below). */
#define QSPI_FMT_ENDIAN_BYTE                 (BIT(5))                                            /*!< Byte endianness (transmission order within 32-bit word). */
#define QSPI_FMT_LEN                         (BITS(16,21))                                       /*!< Frame data bit width selection bits. Master mode supports 4-32 bits, Slave mode supports 8/16/32 bits. */

/* QSPI_TXDATA */
#define QSPI_TXDATA_TXDATA                   (BITS(0,31))                                        /*!< Transmit data buffer. Valid data width depends on QSPI_FMT.LEN register configuration. */

/* QSPI_RXDATA */
#define QSPI_RXDATA_RXDATA                   (BITS(0,31))                                        /*!< Receive data buffer. Valid data width depends on QSPI_FMT.LEN register configuration. */

/* QSPI_TX_MARK */
#define QSPI_TX_MARK_TX_MARK                 (BITS(0,2))                                         /*!< ransmit FIFO threshold selection bit. Interrupt flag tx_irq is raised when TX FIFO valid data count is less than this threshold. */

/* QSPI_RX_MARK */
#define QSPI_RX_MARK_RX_MARK                 (BITS(0,2))                                         /*!< Receive FIFO threshold selection bit. Interrupt flag rx_irq is raised when RX FIFO valid data count exceeds this threshold. */

/* QSPI_FCTRL */
#define QSPI_FCTRL_FLASH_EN                  (BIT(0))                                            /*!< XIP Mode Enable bit. */
#define QSPI_FCTRL_FLASH_WMASK_EN            (BIT(1))                                            /*!< Measurement Mode */
#define QSPI_FCTRL_FLASH_WEN                 (BIT(2))                                            /*!< Measurement Start Point */
#define QSPI_FCTRL_FLASH_BURST_EN            (BIT(3))                                            /*!< Measurement End Point */
#define QSPI_FCTRL_FLASH_WRAP_EN             (BIT(4))                                            /*!< Data value for fixed data measurement mode. The measurement register is updated only when this data is received */
#define QSPI_FCTRL_BOUNDRY_EN                (BIT(5))                                            /*!< XIP Mode Boundary Address Enable bit */

/* QSPI_FFMT */
#define QSPI_FFMT_CMD_EN                     (BIT(0))                                            /*!< Instruction Send Enable bit. */
#define QSPI_FFMT_ADDR_LEN                   (BITS(1,3))                                         /*!< Address Length selection bits. */
#define QSPI_FFMT_RPAD_CNT                   (BITS(4,7))                                         /*!< Read instruction dummy cycle count selection bits (Lower 4 bits). The 5th bit (MSB) of RPAD_CNT is configured in FFMT1.RPAD_CNT_H. */
#define QSPI_FFMT_CMD_PROTO                  (BITS(8,9))                                         /*!< Command Phase Interface Mode selection bits. */
#define QSPI_FFMT_ADDR_PROTO                 (BITS(10,11))                                       /*!< Address Phase Interface Mode selection bits. */
#define QSPI_FFMT_DATA_PROTO                 (BITS(12,13))                                       /*!< Data Phase Interface Mode selection bits. */
#define QSPI_FFMT_ENDIAN_F                   (BIT(14))                                           /*!< XIP Mode bit endianness (transmission order within byte). This bit does not change byte order, only the bit order within a single byte. */
#define QSPI_FFMT_DATA_PROTO_HI              (BIT(15))                                           /*!< High bit extension for QSPI_FFMT.DATA_PROTO[1:0] (See DATA_PROTO). */
#define QSPI_FFMT_RCMD_CODE                  (BITS(16,23))                                       /*!< SPI Flash Read Instruction Opcode. */
#define QSPI_FFMT_PAD_CODE                   (BITS(24,31))                                       /*!< First 8 bits of data transmitted during dummy cycles. */

/* QSPI_IE */
#define QSPI_IE_TX_IRQ_EN                    (BIT(0))                                            /*!< Transmit FIFO Threshold Interrupt Enable bit */
#define QSPI_IE_RX_IRQ_EN                    (BIT(1))                                            /*!< Receive FIFO Threshold Interrupt Enable bit. */
#define QSPI_IE_TX_UDR_EN                    (BIT(2))                                            /*!< Transmit FIFO Underflow Interrupt Enable bit. */
#define QSPI_IE_RX_OVR_EN                    (BIT(3))                                            /*!< Receive FIFO Overflow Interrupt Enable bit. */
#define QSPI_IE_RX_UDR_EN                    (BIT(4))                                            /*!< Receive FIFO Underflow Interrupt Enable bit. */
#define QSPI_IE_TX_OVR_EN                    (BIT(5))                                            /*!< Transmit FIFO Overflow Interrupt Enable bit. */
#define QSPI_IE_DONE_EN                      (BIT(7))                                            /*!< Transfer Done Interrupt Enable bit. */
#define QSPI_IE_TX_DONE_EN                   (BIT(11))                                           /*!< (DMA) Transmit Done Interrupt Enable bit. */
#define QSPI_IE_RX_DONE_EN                   (BIT(12))                                           /*!< (DMA) Receive Done Interrupt Enable bit. */
#define QSPI_IE_CFG_ERR_EN                   (BIT(14))                                           /*!< Configuration Frame Error Interrupt Enable bit. */

/* QSPI_IP */
#define QSPI_IP_TX_IRQ                       (BIT(0))                                            /*!< Transmit FIFO Threshold Interrupt Flag. Set when TX FIFO valid data count is less than TX_MARK threshold. */
#define QSPI_IP_RX_IRQ                       (BIT(1))                                            /*!< Receive FIFO Threshold Interrupt Flag. Set when RX FIFO valid data count exceeds RX_MARK threshold. */

/* QSPI_FFMT1 */
#define QSPI_FFMT1_WCMD_CODE                 (BITS(0,7))                                         /*!< SPI Flash Write Instruction Opcode. */
#define QSPI_FFMT1_WPAD_CNT                  (BITS(8,12))                                        /*!< Write instruction dummy cycle count selection bits. */
#define QSPI_FFMT1_RPAD_CNT_H                (BIT(13))                                           /*!< High bit (Bit 4) for read instruction dummy cycle count selection. Lower 4 bits are in FFMT.RPAD_CNT. */
#define QSPI_FFMT1_DDR_EN                    (BITS(14,17))                                       /*!< DDR Enable bits for different phases: Instruction[14], Address[15], Mode[16], Data[17]. */
#define QSPI_FFMT1_MODE_PROTO                (BITS(18,19))                                       /*!< Mode Byte Phase Interface Mode selection bits. */
#define QSPI_FFMT1_MODE_CODE                 (BITS(20,27))                                       /*!< SPI Flash Mode Byte. */
#define QSPI_FFMT1_MODE_CNT                  (BITS(28,31))                                       /*!< Mode Byte Length selection bits. */

/* QSPI_STA */
#define QSPI_STA_BUSY                        (BIT(0))                                            /*!< Transfer Busy Flag (Master/Slave) */
#define QSPI_STA_RX_OVR_ERR                  (BIT(2))                                            /*!< Receive FIFO Overflow Error Flag. */
#define QSPI_STA_TX_UDR_ERR                  (BIT(3))                                            /*!< Transmit FIFO Underflow Error Flag. */
#define QSPI_STA_TX_FULL                     (BIT(4))                                            /*!< Transmit FIFO Full Flag. */
#define QSPI_STA_RX_EMPTY                    (BIT(5))                                            /*!< Receive FIFO Empty Flag. */
#define QSPI_STA_RX_UDR_ERR                  (BIT(9))                                            /*!< Receive FIFO Underflow Error Flag. Set if QSPI_RXDATA is read while RX FIFO is empty. */
#define QSPI_STA_TX_OVR_ERR                  (BIT(10))                                           /*!< Transmit FIFO Overflow Error Flag. Set if QSPI_TXDATA is written while TX FIFO is full. */
#define QSPI_STA_TX_EMPTY                    (BIT(11))                                           /*!< Transmit FIFO Empty Flag. */
#define QSPI_STA_RX_FULL                     (BIT(12))                                           /*!< Receive FIFO Full Flag. */
#define QSPI_STA_DONE                        (BIT(13))                                           /*!< Transfer Done Flag. Set when a data transfer is complete (send/receive finished) and TX FIFO is empty. */
#define QSPI_STA_TX_DONE                     (BIT(17))                                           /*!< DMA Transmit Done Flag. Set when transmitted data count reaches TSIZE. */
#define QSPI_STA_RX_DONE                     (BIT(18))                                           /*!< DMA Receive Done Flag. Set when received data count reaches RSIZE. */
#define QSPI_STA_CFG_ERR                     (BIT(20))                                           /*!< Frame Configuration Error Flag. Set when a frame config error occurs in normal QSPI or XIP mode. */

/* QSPI_SDR_SCKSAMPLE */
#define QSPI_SDR_SCKSAMPLE_SDR_SCKSAMPLE     (BITS(0,11))                                        /*!< SDR Mode SCK sample point delay selection bit. */

/* QSPI_CR */
#define QSPI_CR_MSTER                        (BIT(0))                                            /*!< Master/Slave Mode Selection bit. */
#define QSPI_CR_DMA_EN                       (BIT(1))                                            /*!< DMA Mode Enable bit. */
#define QSPI_CR_DDR_EN                       (BIT(2))                                            /*!< DDR Mode Enable bit (Master mode only). */
#define QSPI_CR_CSI                          (BIT(3))                                            /*!< Software-controlled Chip Select state (Slave mode, valid when SSM=1). */
#define QSPI_CR_CSOE                         (BIT(4))                                            /*!< Master Mode Chip Select Output Enable bit. */
#define QSPI_CR_SSM                          (BIT(5))                                            /*!< Software Slave Management (CS) Enable bit. */
#define QSPI_CR_DMA_TX_EN                    (BIT(8))                                            /*!< Transmit DMA Mode Enable bit. */
#define QSPI_CR_DMA_RX_EN                    (BIT(9))                                            /*!< Receive DMA Mode Enable bit. */
#define QSPI_CR_DMA_RXCT_MODE                (BIT(10))                                           /*!< DMA Continuous Receive Mode Enable bit. */
#define QSPI_CR_DMA_TXCT_MODE                (BIT(11))                                           /*!< DMA Continuous Transmit Mode Enable bit. */
#define QSPI_CR_SPI_RXFIFO_EN                (BIT(13))                                           /*!< Receive FIFO Enable bit. */

/* constants definitions */
/* QSPI_SCKDIV_SCKDIV */
#define QSPI_SCKDIV_SCKDIV_POS               (0U)                                                /*!< QSPI_SCKDIV_SCKDIV bit offset */

/* QSPI_SCKMODE_CPHA */
#define QSPI_SCKMODE_CPHA_POS                (0U)                                                /*!< QSPI_SCKMODE_CPHA bit offset */

/* QSPI_SCKMODE_CPOL */
#define QSPI_SCKMODE_CPOL_POS                (1U)                                                /*!< QSPI_SCKMODE_CPOL bit offset */

/* QSPI_SCKSAMPLE_SCKSMP */
#define QSPI_SCKSAMPLE_SCKSMP_POS            (0U)                                                /*!< QSPI_SCKSAMPLE_SCKSMP bit offset */

/* QSPI_FORCE_FORCE_EN */
#define QSPI_FORCE_FORCE_EN_POS              (0U)                                                /*!< QSPI_FORCE_FORCE_EN bit offset */

/* QSPI_FORCE_FORCE_WP */
#define QSPI_FORCE_FORCE_WP_POS              (1U)                                                /*!< QSPI_FORCE_FORCE_WP bit offset */

/* QSPI_CSID_CSID */
#define QSPI_CSID_CSID_POS                   (0U)                                                /*!< QSPI_CSID_CSID bit offset */

/* QSPI_CSDEF_CSDEF */
#define QSPI_CSDEF_CSDEF_POS                 (0U)                                                /*!< QSPI_CSDEF_CSDEF bit offset */

/* QSPI_CSMODE_CSMODE */
#define QSPI_CSMODE_CSMODE_POS               (0U)                                                /*!< QSPI_CSMODE_CSMODE bit offset */
#define QSPI_CSMODE_CSMODE_CFG(regval)  (QSPI_CSMODE_CSMODE & ((uint32_t)(regval) << QSPI_CSMODE_CSMODE_POS))
#define QSPI_CSMODE_CSMODE_AUTO              (QSPI_CSMODE_CSMODE_CFG(0x0))                       /*!< Auto mode */
#define QSPI_CSMODE_CSMODE_HOLD              (QSPI_CSMODE_CSMODE_CFG(0x2))                       /*!< Hold mode */
#define QSPI_CSMODE_CSMODE_DISABLE           (QSPI_CSMODE_CSMODE_CFG(0x3))                       /*!< Disable mode */

/* QSPI_VERSION_VERSION */
#define QSPI_VERSION_VERSION_POS             (0U)                                                /*!< QSPI_VERSION_VERSION bit offset */

/* QSPI_ADDR_WRAP_OFFSET */
#define QSPI_ADDR_WRAP_OFFSET_POS            (0U)                                                /*!< QSPI_ADDR_WRAP_OFFSET bit offset */

/* QSPI_BOUNDARY_CFG_BOUNDARY */
#define QSPI_BOUNDARY_CFG_BOUNDARY_POS       (0U)                                                /*!< QSPI_BOUNDARY_CFG_BOUNDARY bit offset */

/* QSPI_DELAY0_CSSCK */
#define QSPI_DELAY0_CSSCK_POS                (0U)                                                /*!< QSPI_DELAY0_CSSCK bit offset */

/* QSPI_DELAY0_SCKCS */
#define QSPI_DELAY0_SCKCS_POS                (16U)                                               /*!< QSPI_DELAY0_SCKCS bit offset */

/* QSPI_DELAY1_INTERCS */
#define QSPI_DELAY1_INTERCS_POS              (0U)                                                /*!< QSPI_DELAY1_INTERCS bit offset */

/* QSPI_DELAY1_INTERXFR */
#define QSPI_DELAY1_INTERXFR_POS             (16U)                                               /*!< QSPI_DELAY1_INTERXFR bit offset */

/* QSPI_FIFO_NUM_TX_NUM */
#define QSPI_FIFO_NUM_TX_NUM_POS             (0U)                                                /*!< QSPI_FIFO_NUM_TX_NUM bit offset */

/* QSPI_FIFO_NUM_RX_NUM */
#define QSPI_FIFO_NUM_RX_NUM_POS             (16U)                                               /*!< QSPI_FIFO_NUM_RX_NUM bit offset */

/* QSPI_TSIZE_TSIZE */
#define QSPI_TSIZE_TSIZE_POS                 (0U)                                                /*!< QSPI_TSIZE_TSIZE bit offset */

/* QSPI_RSIZE_RSIZE */
#define QSPI_RSIZE_RSIZE_POS                 (0U)                                                /*!< QSPI_RSIZE_RSIZE bit offset */

/* QSPI_FMT_PROTO */
#define QSPI_FMT_PROTO_POS                   (0U)                                                /*!< QSPI_FMT_PROTO bit offset */
#define QSPI_FMT_PROTO_CFG(regval)  (QSPI_FMT_PROTO & ((uint32_t)(regval) << QSPI_FMT_PROTO_POS))
#define QSPI_FMT_PROTO_SINGLE                (QSPI_FMT_PROTO_CFG(0x0))                           /*!< Single-line mode */
#define QSPI_FMT_PROTO_DUAL                  (QSPI_FMT_PROTO_CFG(0x1))                           /*!< Dual-line mode */
#define QSPI_FMT_PROTO_QUAD                  (QSPI_FMT_PROTO_CFG(0x2))                           /*!< Quad-line mode */
#define QSPI_FMT_PROTO_HALF_DUPLEX           (QSPI_FMT_PROTO_CFG(0x3))                           /*!< Single-line Half-duplex mode */

/* QSPI_FMT_ENDIAN */
#define QSPI_FMT_ENDIAN_POS                  (2U)                                                /*!< QSPI_FMT_ENDIAN bit offset */

/* QSPI_FMT_DIR */
#define QSPI_FMT_DIR_POS                     (3U)                                                /*!< QSPI_FMT_DIR bit offset */

/* QSPI_FMT_PROTO_HI */
#define QSPI_FMT_PROTO_HI_POS                (4U)                                                /*!< QSPI_FMT_PROTO_HI bit offset */

/* QSPI_FMT_ENDIAN_BYTE */
#define QSPI_FMT_ENDIAN_BYTE_POS             (5U)                                                /*!< QSPI_FMT_ENDIAN_BYTE bit offset */

/* QSPI_FMT_LEN */
#define QSPI_FMT_LEN_POS                     (16U)                                               /*!< QSPI_FMT_LEN bit offset */
#define QSPI_FMT_LEN_CFG(regval)  (QSPI_FMT_LEN & ((uint32_t)(regval) << QSPI_FMT_LEN_POS))
#define QSPI_FMT_LEN_8BIT                    (QSPI_FMT_LEN_CFG(8))                               /*!< Data length for SPI transfer: 8 bits */
#define QSPI_FMT_LEN_16BIT                   (QSPI_FMT_LEN_CFG(16))                              /*!< Data length for SPI transfer: 16 bits */
#define QSPI_FMT_LEN_32BIT                   (QSPI_FMT_LEN_CFG(32))                              /*!< Data length for SPI transfer: 32 bits */

/* QSPI_TXDATA_TXDATA */
#define QSPI_TXDATA_TXDATA_POS               (0U)                                                /*!< QSPI_TXDATA_TXDATA bit offset */

/* QSPI_RXDATA_RXDATA */
#define QSPI_RXDATA_RXDATA_POS               (0U)                                                /*!< QSPI_RXDATA_RXDATA bit offset */

/* QSPI_TX_MARK_TX_MARK */
#define QSPI_TX_MARK_TX_MARK_POS             (0U)                                                /*!< QSPI_TX_MARK_TX_MARK bit offset */
#define QSPI_TX_MARK_TX_MARK_CFG(regval)  (QSPI_TX_MARK_TX_MARK & ((uint32_t)(regval) << QSPI_TX_MARK_TX_MARK_POS))
#define QSPI_TX_MARK_TX_MARK_1               (QSPI_TX_MARK_TX_MARK_CFG(0x1))                     /*!< 1 valid data item */
#define QSPI_TX_MARK_TX_MARK_2               (QSPI_TX_MARK_TX_MARK_CFG(0x2))                     /*!< 2 valid data items */
#define QSPI_TX_MARK_TX_MARK_3               (QSPI_TX_MARK_TX_MARK_CFG(0x3))                     /*!< 3 valid data items */
#define QSPI_TX_MARK_TX_MARK_4               (QSPI_TX_MARK_TX_MARK_CFG(0x4))                     /*!< 4 valid data items */

/* QSPI_RX_MARK_RX_MARK */
#define QSPI_RX_MARK_RX_MARK_POS             (0U)                                                /*!< QSPI_RX_MARK_RX_MARK bit offset */
#define QSPI_RX_MARK_RX_MARK_CFG(regval)  (QSPI_RX_MARK_RX_MARK & ((uint32_t)(regval) << QSPI_RX_MARK_RX_MARK_POS))
#define QSPI_RX_MARK_RX_MARK_0               (QSPI_RX_MARK_RX_MARK_CFG(0x0))                     /*!< 0 valid data item */
#define QSPI_RX_MARK_RX_MARK_1               (QSPI_RX_MARK_RX_MARK_CFG(0x1))                     /*!< 1 valid data item */
#define QSPI_RX_MARK_RX_MARK_2               (QSPI_RX_MARK_RX_MARK_CFG(0x2))                     /*!< 2 valid data items */
#define QSPI_RX_MARK_RX_MARK_3               (QSPI_RX_MARK_RX_MARK_CFG(0x3))                     /*!< 3 valid data items */

/* QSPI_FCTRL_FLASH_EN */
#define QSPI_FCTRL_FLASH_EN_POS              (0U)                                                /*!< QSPI_FCTRL_FLASH_EN bit offset */

/* QSPI_FCTRL_FLASH_WMASK_EN */
#define QSPI_FCTRL_FLASH_WMASK_EN_POS        (1U)                                                /*!< QSPI_FCTRL_FLASH_WMASK_EN bit offset */

/* QSPI_FCTRL_FLASH_WEN */
#define QSPI_FCTRL_FLASH_WEN_POS             (2U)                                                /*!< QSPI_FCTRL_FLASH_WEN bit offset */

/* QSPI_FCTRL_FLASH_BURST_EN */
#define QSPI_FCTRL_FLASH_BURST_EN_POS        (3U)                                                /*!< QSPI_FCTRL_FLASH_BURST_EN bit offset */

/* QSPI_FCTRL_FLASH_WRAP_EN */
#define QSPI_FCTRL_FLASH_WRAP_EN_POS         (4U)                                                /*!< QSPI_FCTRL_FLASH_WRAP_EN bit offset */

/* QSPI_FCTRL_BOUNDRY_EN */
#define QSPI_FCTRL_BOUNDRY_EN_POS            (5U)                                                /*!< QSPI_FCTRL_BOUNDRY_EN bit offset */

/* QSPI_FFMT_CMD_EN */
#define QSPI_FFMT_CMD_EN_POS                 (0U)                                                /*!< QSPI_FFMT_CMD_EN bit offset */

/* QSPI_FFMT_ADDR_LEN */
#define QSPI_FFMT_ADDR_LEN_POS               (1U)                                                /*!< QSPI_FFMT_ADDR_LEN bit offset */
#define QSPI_FFMT_ADDR_LEN_CFG(regval)  (QSPI_FFMT_ADDR_LEN & ((uint32_t)(regval) << QSPI_FFMT_ADDR_LEN_POS))
#define QSPI_FFMT_ADDR_LEN_NONE              (QSPI_FFMT_ADDR_LEN_CFG(0x0))                       /*!< Skip Address phase */
#define QSPI_FFMT_ADDR_LEN_8BIT              (QSPI_FFMT_ADDR_LEN_CFG(0x1))                       /*!< Address Length 8 bits */
#define QSPI_FFMT_ADDR_LEN_16BIT             (QSPI_FFMT_ADDR_LEN_CFG(0x2))                       /*!< Address Length 16 bits */
#define QSPI_FFMT_ADDR_LEN_24BIT             (QSPI_FFMT_ADDR_LEN_CFG(0x3))                       /*!< Address Length 24 bits (default) */
#define QSPI_FFMT_ADDR_LEN_32BIT             (QSPI_FFMT_ADDR_LEN_CFG(0x4))                       /*!< Address Length 32 bits */

/* QSPI_FFMT_RPAD_CNT */
#define QSPI_FFMT_RPAD_CNT_POS               (4U)                                                /*!< QSPI_FFMT_RPAD_CNT bit offset */

/* QSPI_FFMT_CMD_PROTO */
#define QSPI_FFMT_CMD_PROTO_POS              (8U)                                                /*!< QSPI_FFMT_CMD_PROTO bit offset */
#define QSPI_FFMT_CMD_PROTO_CFG(regval)  (QSPI_FFMT_CMD_PROTO & ((uint32_t)(regval) << QSPI_FFMT_CMD_PROTO_POS))
#define QSPI_FFMT_CMD_PROTO_SINGLE           (QSPI_FFMT_CMD_PROTO_CFG(0x0))                      /*!< XIP Command Single-line mode */
#define QSPI_FFMT_CMD_PROTO_DUAL             (QSPI_FFMT_CMD_PROTO_CFG(0x1))                      /*!< XIP Command Dual-line mode */
#define QSPI_FFMT_CMD_PROTO_QUAD             (QSPI_FFMT_CMD_PROTO_CFG(0x2))                      /*!< XIP Command Quad-line mode */

/* QSPI_FFMT_ADDR_PROTO */
#define QSPI_FFMT_ADDR_PROTO_POS             (10U)                                               /*!< QSPI_FFMT_ADDR_PROTO bit offset */
#define QSPI_FFMT_ADDR_PROTO_CFG(regval)  (QSPI_FFMT_ADDR_PROTO & ((uint32_t)(regval) << QSPI_FFMT_ADDR_PROTO_POS))
#define QSPI_FFMT_ADDR_PROTO_SINGLE          (QSPI_FFMT_ADDR_PROTO_CFG(0x0))                     /*!< XIP Address Single-line mode */
#define QSPI_FFMT_ADDR_PROTO_DUAL            (QSPI_FFMT_ADDR_PROTO_CFG(0x1))                     /*!< XIP Address Dual-line mode */
#define QSPI_FFMT_ADDR_PROTO_QUAD            (QSPI_FFMT_ADDR_PROTO_CFG(0x2))                     /*!< XIP Address Quad-line mode */

/* QSPI_FFMT_DATA_PROTO */
#define QSPI_FFMT_DATA_PROTO_POS             (12U)                                               /*!< QSPI_FFMT_DATA_PROTO bit offset */
#define QSPI_FFMT_DATA_PROTO_CFG(regval)  (QSPI_FFMT_DATA_PROTO & ((uint32_t)(regval) << QSPI_FFMT_DATA_PROTO_POS))
#define QSPI_FFMT_DATA_PROTO_SINGLE          (QSPI_FFMT_DATA_PROTO_CFG(0x0))                     /*!< XIP Data Single-line mode */
#define QSPI_FFMT_DATA_PROTO_DUAL            (QSPI_FFMT_DATA_PROTO_CFG(0x1))                     /*!< XIP Data Dual-line mode */
#define QSPI_FFMT_DATA_PROTO_QUAD            (QSPI_FFMT_DATA_PROTO_CFG(0x2))                     /*!< XIP Data Quad-line mode */
#define QSPI_FFMT_DATA_PROTO_HALF_DUPLEX     (QSPI_FFMT_DATA_PROTO_CFG(0x3))                     /*!< XIP Data Single-line Half-duplex mode */

/* QSPI_FFMT_ENDIAN_F */
#define QSPI_FFMT_ENDIAN_F_POS               (14U)                                               /*!< QSPI_FFMT_ENDIAN_F bit offset */

/* QSPI_FFMT_DATA_PROTO_HI */
#define QSPI_FFMT_DATA_PROTO_HI_POS          (15U)                                               /*!< QSPI_FFMT_DATA_PROTO_HI bit offset */

/* QSPI_FFMT_RCMD_CODE */
#define QSPI_FFMT_RCMD_CODE_POS              (16U)                                               /*!< QSPI_FFMT_RCMD_CODE bit offset */

/* QSPI_FFMT_PAD_CODE */
#define QSPI_FFMT_PAD_CODE_POS               (24U)                                               /*!< QSPI_FFMT_PAD_CODE bit offset */

/* QSPI_IE_TX_IRQ_EN */
#define QSPI_IE_TX_IRQ_EN_POS                (0U)                                                /*!< QSPI_IE_TX_IRQ_EN bit offset */

/* QSPI_IE_RX_IRQ_EN */
#define QSPI_IE_RX_IRQ_EN_POS                (1U)                                                /*!< QSPI_IE_RX_IRQ_EN bit offset */

/* QSPI_IE_TX_UDR_EN */
#define QSPI_IE_TX_UDR_EN_POS                (2U)                                                /*!< QSPI_IE_TX_UDR_EN bit offset */

/* QSPI_IE_RX_OVR_EN */
#define QSPI_IE_RX_OVR_EN_POS                (3U)                                                /*!< QSPI_IE_RX_OVR_EN bit offset */

/* QSPI_IE_RX_UDR_EN */
#define QSPI_IE_RX_UDR_EN_POS                (4U)                                                /*!< QSPI_IE_RX_UDR_EN bit offset */

/* QSPI_IE_TX_OVR_EN */
#define QSPI_IE_TX_OVR_EN_POS                (5U)                                                /*!< QSPI_IE_TX_OVR_EN bit offset */

/* QSPI_IE_DONE_EN */
#define QSPI_IE_DONE_EN_POS                  (7U)                                                /*!< QSPI_IE_DONE_EN bit offset */

/* QSPI_IE_TX_DONE_EN */
#define QSPI_IE_TX_DONE_EN_POS               (11U)                                               /*!< QSPI_IE_TX_DONE_EN bit offset */

/* QSPI_IE_RX_DONE_EN */
#define QSPI_IE_RX_DONE_EN_POS               (12U)                                               /*!< QSPI_IE_RX_DONE_EN bit offset */

/* QSPI_IE_CFG_ERR_EN */
#define QSPI_IE_CFG_ERR_EN_POS               (14U)                                               /*!< QSPI_IE_CFG_ERR_EN bit offset */

/* QSPI_IP_TX_IRQ */
#define QSPI_IP_TX_IRQ_POS                   (0U)                                                /*!< QSPI_IP_TX_IRQ bit offset */

/* QSPI_IP_RX_IRQ */
#define QSPI_IP_RX_IRQ_POS                   (1U)                                                /*!< QSPI_IP_RX_IRQ bit offset */

/* QSPI_FFMT1_WCMD_CODE */
#define QSPI_FFMT1_WCMD_CODE_POS             (0U)                                                /*!< QSPI_FFMT1_WCMD_CODE bit offset */

/* QSPI_FFMT1_WPAD_CNT */
#define QSPI_FFMT1_WPAD_CNT_POS              (8U)                                                /*!< QSPI_FFMT1_WPAD_CNT bit offset */

/* QSPI_FFMT1_RPAD_CNT_H */
#define QSPI_FFMT1_RPAD_CNT_H_POS            (13U)                                               /*!< QSPI_FFMT1_RPAD_CNT_H bit offset */

/* QSPI_FFMT1_DDR_EN */
#define QSPI_FFMT1_DDR_EN_POS                (14U)                                               /*!< QSPI_FFMT1_DDR_EN bit offset */

/* QSPI_FFMT1_MODE_PROTO */
#define QSPI_FFMT1_MODE_PROTO_POS            (18U)                                               /*!< QSPI_FFMT1_MODE_PROTO bit offset */
#define QSPI_FFMT1_MODE_PROTO_CFG(regval)  (QSPI_FFMT1_MODE_PROTO & ((uint32_t)(regval) << QSPI_FFMT1_MODE_PROTO_POS))
#define QSPI_FFMT1_MODE_PROTO_SINGLE         (QSPI_FFMT1_MODE_PROTO_CFG(0x0))                    /*!< XIP Mode Single-line mode */
#define QSPI_FFMT1_MODE_PROTO_DUAL           (QSPI_FFMT1_MODE_PROTO_CFG(0x1))                    /*!< XIP Mode Dual-line mode */
#define QSPI_FFMT1_MODE_PROTO_QUAD           (QSPI_FFMT1_MODE_PROTO_CFG(0x2))                    /*!< XIP Mode Quad-line mode */

/* QSPI_FFMT1_MODE_CODE */
#define QSPI_FFMT1_MODE_CODE_POS             (20U)                                               /*!< QSPI_FFMT1_MODE_CODE bit offset */

/* QSPI_FFMT1_MODE_CNT */
#define QSPI_FFMT1_MODE_CNT_POS              (28U)                                               /*!< QSPI_FFMT1_MODE_CNT bit offset */

/* QSPI_STA_BUSY */
#define QSPI_STA_BUSY_POS                    (0U)                                                /*!< QSPI_STA_BUSY bit offset */

/* QSPI_STA_RX_OVR_ERR */
#define QSPI_STA_RX_OVR_ERR_POS              (2U)                                                /*!< QSPI_STA_RX_OVR_ERR bit offset */

/* QSPI_STA_TX_UDR_ERR */
#define QSPI_STA_TX_UDR_ERR_POS              (3U)                                                /*!< QSPI_STA_TX_UDR_ERR bit offset */

/* QSPI_STA_TX_FULL */
#define QSPI_STA_TX_FULL_POS                 (4U)                                                /*!< QSPI_STA_TX_FULL bit offset */

/* QSPI_STA_RX_EMPTY */
#define QSPI_STA_RX_EMPTY_POS                (5U)                                                /*!< QSPI_STA_RX_EMPTY bit offset */

/* QSPI_STA_RX_UDR_ERR */
#define QSPI_STA_RX_UDR_ERR_POS              (9U)                                                /*!< QSPI_STA_RX_UDR_ERR bit offset */

/* QSPI_STA_TX_OVR_ERR */
#define QSPI_STA_TX_OVR_ERR_POS              (10U)                                               /*!< QSPI_STA_TX_OVR_ERR bit offset */

/* QSPI_STA_TX_EMPTY */
#define QSPI_STA_TX_EMPTY_POS                (11U)                                               /*!< QSPI_STA_TX_EMPTY bit offset */

/* QSPI_STA_RX_FULL */
#define QSPI_STA_RX_FULL_POS                 (12U)                                               /*!< QSPI_STA_RX_FULL bit offset */

/* QSPI_STA_DONE */
#define QSPI_STA_DONE_POS                    (13U)                                               /*!< QSPI_STA_DONE bit offset */

/* QSPI_STA_TX_DONE */
#define QSPI_STA_TX_DONE_POS                 (17U)                                               /*!< QSPI_STA_TX_DONE bit offset */

/* QSPI_STA_RX_DONE */
#define QSPI_STA_RX_DONE_POS                 (18U)                                               /*!< QSPI_STA_RX_DONE bit offset */

/* QSPI_STA_CFG_ERR */
#define QSPI_STA_CFG_ERR_POS                 (20U)                                               /*!< QSPI_STA_CFG_ERR bit offset */

/* QSPI_SDR_SCKSAMPLE_SDR_SCKSAMPLE */
#define QSPI_SDR_SCKSAMPLE_SDR_SCKSAMPLE_POS  (0U)                                                /*!< QSPI_SDR_SCKSAMPLE_SDR_SCKSAMPLE bit offset */

/* QSPI_CR_MSTER */
#define QSPI_CR_MSTER_POS                    (0U)                                                /*!< QSPI_CR_MSTER bit offset */

/* QSPI_CR_DMA_EN */
#define QSPI_CR_DMA_EN_POS                   (1U)                                                /*!< QSPI_CR_DMA_EN bit offset */

/* QSPI_CR_DDR_EN */
#define QSPI_CR_DDR_EN_POS                   (2U)                                                /*!< QSPI_CR_DDR_EN bit offset */

/* QSPI_CR_CSI */
#define QSPI_CR_CSI_POS                      (3U)                                                /*!< QSPI_CR_CSI bit offset */

/* QSPI_CR_CSOE */
#define QSPI_CR_CSOE_POS                     (4U)                                                /*!< QSPI_CR_CSOE bit offset */

/* QSPI_CR_SSM */
#define QSPI_CR_SSM_POS                      (5U)                                                /*!< QSPI_CR_SSM bit offset */

/* QSPI_CR_DMA_TX_EN */
#define QSPI_CR_DMA_TX_EN_POS                (8U)                                                /*!< QSPI_CR_DMA_TX_EN bit offset */

/* QSPI_CR_DMA_RX_EN */
#define QSPI_CR_DMA_RX_EN_POS                (9U)                                                /*!< QSPI_CR_DMA_RX_EN bit offset */

/* QSPI_CR_DMA_RXCT_MODE */
#define QSPI_CR_DMA_RXCT_MODE_POS            (10U)                                               /*!< QSPI_CR_DMA_RXCT_MODE bit offset */

/* QSPI_CR_DMA_TXCT_MODE */
#define QSPI_CR_DMA_TXCT_MODE_POS            (11U)                                               /*!< QSPI_CR_DMA_TXCT_MODE bit offset */

/* QSPI_CR_SPI_RXFIFO_EN */
#define QSPI_CR_SPI_RXFIFO_EN_POS            (13U)                                               /*!< QSPI_CR_SPI_RXFIFO_EN bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_QSPI_H */
