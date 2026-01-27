/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_can.h
 * @author  Renergy Software Team
 * @brief   CAN module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_CAN_H
#define _RN831x_RN861x_MCU_CAN_H

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

/* CAN */
typedef struct
{
    __IO uint32_t MCR;              /*!< offset: 0x0000        info: CAN Module Configuration Register. */
    __IO uint32_t CTRL1;            /*!< offset: 0x0004        info: CAN Control Register 1. */
    __IO uint32_t TIMER;            /*!< offset: 0x0008        info: CAN Free Running Timer Register. */
    __IO uint32_t RESERVED0;        /*!< offset: 0x000C        info: Reserved. */
    __IO uint32_t RXMGMASK;         /*!< offset: 0x0010        info: CAN Rx Mailboxes Global Mask Register. */
    __IO uint32_t RX14MASK;         /*!< offset: 0x0014        info: CAN Rx14 Mask Register. */
    __IO uint32_t RX15MASK;         /*!< offset: 0x0018        info: CAN Rx15 Mask Register. */
    __IO uint32_t ECR;              /*!< offset: 0x001C        info: CAN Error Counter Register. */
    __IO uint32_t ESR1;             /*!< offset: 0x0020        info: CAN Error and Status Register 1. */
    __IO uint32_t RESERVED1;        /*!< offset: 0x0024        info: Reserved. */
    __IO uint32_t IMASK1;           /*!< offset: 0x0028        info: CAN Interrupt Mask Register 1. */
    __IO uint32_t RESERVED2;        /*!< offset: 0x002C        info: Reserved. */
    __IO uint32_t IFLAG1;           /*!< offset: 0x0030        info: CAN Interrupt Flag Register 1. */
    __IO uint32_t CTRL2;            /*!< offset: 0x0034        info: CAN Control Register 2. */
    __IO uint32_t ESR2;             /*!< offset: 0x0038        info: CAN Error and Status Register 2. */
    __IO uint32_t RESERVED3[2];     /*!< offset: 0x003C ~ 0040 info: Reserved.  */
    __IO uint32_t CRCR;             /*!< offset: 0x0044        info: CAN CRC Register. */
    __IO uint32_t RXFGMASK;         /*!< offset: 0x0048        info: CAN Rx FIFO Global Mask Register. */
    __IO uint32_t RXFIR;            /*!< offset: 0x004C        info: CAN Rx FIFO Infomation Register. */
    __IO uint32_t CBT;              /*!< offset: 0x0050        info: CAN Bit Timing Register. */
    __IO uint32_t RESERVED4[11];    /*!< offset: 0x0054 ~ 007C info: Reserved. */
    struct
    {
        __IO uint32_t CS;           /*!< offset: 0x0080        info: CAN CRC Register. */
        __IO uint32_t ID;           /*!< offset: 0x0084        info: CAN Rx FIFO Global Mask Register. */
        __IO uint32_t WORD0;        /*!< offset: 0x0088        info: CAN Rx FIFO Infomation Register. */
        __IO uint32_t WORD1;        /*!< offset: 0x008C        info: CAN Bit Timing Register. */
    } MB[16];
    __IO uint32_t RESERVED5[448];   /*!< offset: 0x0180 ~ 087C info: Reserved. */
    __IO uint32_t RXIMR[16];        /*!< offset: 0x0880 ~ 08BC info: CAN Rx Individual Mask Registers. */
    __IO uint32_t RESERVED6[204];   /*!< offset: 0x08C0 ~ 0BEC info: Reserved. */
    __IO uint32_t EPRS;             /*!< offset: 0x0BF0        info: CAN Enhanced Pre-allocated Bit Timing Setting Register. */
    __IO uint32_t ECBT;             /*!< offset: 0x0BF4        info: CAN Enhanced Pre-allocated Bit Timing Paramaters Configuration Register. */
    __IO uint32_t RESERVED7[2434];  /*!< offset: 0x0BF8 ~ 31FC info: Reserved. */
    __IO uint32_t DMA_RBADR;        /*!< offset: 0x3200        info: CAN DMA Rx Beginning Address Register. */
    __IO uint32_t DMA_RLEN;         /*!< offset: 0x3204        info: CAN DMA Rx Data Length Register. */
    __IO uint32_t DMA_RADR;         /*!< offset: 0x3208        info: CAN DMA Current Address Register. */
    __IO uint32_t DMA_IE;           /*!< offset: 0x320C        info: CAN DMA Interrupt Enable Register. */
    __IO uint32_t DMA_IF;           /*!< offset: 0x3210        info: CAN DMA Interrupt Flag Register. */
    __IO uint32_t TMR_SEL;          /*!< offset: 0x3214        info: CAN Timer Filter Select Register. */
    __IO uint32_t STOP_MD;          /*!< offset: 0x3218        info: CAN Stop Mode Register. */
} CAN_TypeDef;

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
#define CAN_CORE0_BASE  (0x20058000U)
#define CAN_CORE1_BASE  (0xA0058000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define CAN_CORE0  ((CAN_TypeDef *)      CAN_CORE0_BASE)
#define CAN_CORE1  ((CAN_TypeDef *)      CAN_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define CAN  CAN_CORE0
#endif
#if defined(CORE1)
#define CAN  CAN_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** CAN Instances *******************************/
#define IS_CAN_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == CAN_CORE0) || \
                                          ((INSTANCE) == CAN_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* CAN_MCR */
#define CAN_MCR_MAXMB                        (BITS(0,6))                                         /*!< The number of the last MB that will take part in the matching and arbitration processes. */
#define CAN_MCR_IDAM                         (BITS(8,9))                                         /*!< The format of the Rx FIFO ID Filter Table elements. */
#define CAN_MCR_AEN                          (BIT(12))                                           /*!< Enable abort. */
#define CAN_MCR_LPRIOEN                      (BIT(13))                                           /*!< Enable Local Priority. */
#define CAN_MCR_PNETEN                       (BIT(14))                                           /*!< Enable Pretended Network. */
#define CAN_MCR_DMA                          (BIT(15))                                           /*!< Enable DMA feature for Rx FIFO. */
#define CAN_MCR_IRMQ                         (BIT(16))                                           /*!< Enable individual Rx masking and queue feature. */
#define CAN_MCR_SRXDIS                       (BIT(17))                                           /*!< Disable self reception. */
#define CAN_MCR_DOZE                         (BIT(18))                                           /*!< Enable sleep mode. */
#define CAN_MCR_WAKSRC                       (BIT(19))
#define CAN_MCR_LPMACK                       (BIT(20))                                           /*!< Low-Power Mode Acknowledge. */
#define CAN_MCR_WRNEN                        (BIT(21))                                           /*!< Warning Interrupt Enable. */
#define CAN_MCR_SLFWAK                       (BIT(22))                                           /*!< Wake-up byself Enable. */
#define CAN_MCR_FRZACK                       (BIT(24))                                           /*!< Freeze Mode Acknowledge. */
#define CAN_MCR_SOFTRST                      (BIT(25))                                           /*!< Soft reset. */
#define CAN_MCR_WAKMSK                       (BIT(26))                                           /*!< Enable Wake-up Interrupt. */
#define CAN_MCR_NOTRDY                       (BIT(27))                                           /*!< CAN is either in Disable or Freeze mode. */
#define CAN_MCR_HALT                         (BIT(28))                                           /*!< Enter Freeze mode if the FRZ bit is asserted. */
#define CAN_MCR_RFEN                         (BIT(29))                                           /*!< Enable Rx FIFO. */
#define CAN_MCR_FRZ                          (BIT(30))                                           /*!< Enabled to enter Freeze mode. */
#define CAN_MCR_MDIS                         (BIT(31))                                           /*!< Disable the CAN module as well as the clocks to the PE and CHI submodules. */

/* CAN_CTRL1 */
#define CAN_CTRL1_PROPSEG                    (BITS(0,2))                                         /*!< Propagation Segment - Defines the length of the Propagation Segment in the bit time. */
#define CAN_CTRL1_LOM                        (BIT(3))                                            /*!< Configures the CAN to operate in Listen-Only mode. */
#define CAN_CTRL1_LBUF                       (BIT(4))                                            /*!< The ordering mechanism for message buffer transmission. */
#define CAN_CTRL1_TSYN                       (BIT(5))                                            /*!< Timer Sync - a mechanism that resets the free-running timer each time a message is received in MB0. */
#define CAN_CTRL1_BOFFRREC                   (BIT(6))                                            /*!< Defines how the CAN recovers from Bus Off state. */
#define CAN_CTRL1_SMP                        (BIT(7))                                            /*!< The sampling mode of CAN bits at the Rx input. */
#define CAN_CTRL1_RWRNMSK                    (BIT(10))                                           /*!< Rx Warning Interrupt (CAN_ESR1.RWRNINT) Mask. */
#define CAN_CTRL1_TWRNMSK                    (BIT(11))                                           /*!< Tx Warning Interrupt (CAN_ESR1.TWRNINT) Mask. */
#define CAN_CTRL1_LPB                        (BIT(12))                                           /*!< Loop-Back enabled. */
#define CAN_CTRL1_ERRMSK                     (BIT(14))                                           /*!< Error Interrupt (CAN_ESR1.ERRINT) Mask. */
#define CAN_CTRL1_BOFFMSK                    (BIT(15))                                           /*!< Bus Off Interrupt (CAN_ESR1.BOFFINT) Mask. */
#define CAN_CTRL1_PSEG2                      (BITS(16,18))                                       /*!< Phase Segment 2 - Defines the length of Phase Segment 2 in the bit time. */
#define CAN_CTRL1_PSEG1                      (BITS(19,21))                                       /*!< Phase Segment 1 - Defines the length of Phase Segment 1 in the bit time. */
#define CAN_CTRL1_RJW                        (BITS(22,23))                                       /*!< Resync Jump Width - Defines the maximum number of Tq that a bit time can be changed by one re-synchronization. */
#define CAN_CTRL1_PRESDIV                    (BITS(24,31))                                       /*!< Prescaler Division Factor - Defines the ratio between the PE clock frequency and the Serial Clock (Sclock) frequency. */

/* CAN_TIMER */
#define CAN_TIMER_TIMER                      (BITS(0,15))                                        /*!< Timer Value: Contains the free-running counter value. */

/* CAN_RXMGMASK */
#define CAN_RXMGMASK_MG                      (BITS(0,31))                                        /*!< Rx Mailboxes Global Mask Bit. */

/* CAN_RX14MASK */
#define CAN_RX14MASK_RX14M                   (BITS(0,31))                                        /*!< Rx MB14 Mask Bits. */

/* CAN_RX15MASK */
#define CAN_RX15MASK_RX15M                   (BITS(0,31))                                        /*!< Rx MB15 Mask Bits. */

/* CAN_ECR */
#define CAN_ECR_TXERRCNT                     (BITS(0,7))                                         /*!< Transmit Error Counter - Calculate all errors detected in transmitted messages. */
#define CAN_ECR_RXERRCNT                     (BITS(8,15))                                        /*!< Receive Error Counter - Calculate all errors detected in received messages. */

/* CAN_ESR1 */
#define CAN_ESR1_WAKINT                      (BIT(0))                                            /*!< Wake-up Interrupt. */
#define CAN_ESR1_ERRINT                      (BIT(1))                                            /*!< Error Interrupt. */
#define CAN_ESR1_BOFFINT                     (BIT(2))                                            /*!< Bus Off Interrupt. */
#define CAN_ESR1_RX                          (BIT(3))                                            /*!< CAN is receiving a message. */
#define CAN_ESR1_FLTCONF                     (BITS(4,5))                                         /*!< Fault State of the CAN module. */
#define CAN_ESR1_TX                          (BIT(6))                                            /*!< CAN is transmitting a message. */
#define CAN_ESR1_IDLE                        (BIT(7))                                            /*!< CAN bus is in IDLE state. */
#define CAN_ESR1_RXWRN                       (BIT(8))                                            /*!< Rx Error Warning - Indicates when repetitive errors are occurring during message reception. */
#define CAN_ESR1_TXWRN                       (BIT(9))                                            /*!< TX Error Warning - Indicates when repetitive errors are occurring during message transmission. */
#define CAN_ESR1_STFERR                      (BIT(10))                                           /*!< Stuffing Error - Indicates that a Stuffing Error in a non-FD message or in the arbitration or data phase of a CAN FD message or in the arbitration or data phase of a CAN FD message was detected by the receiver node. */
#define CAN_ESR1_FRMERR                      (BIT(11))                                           /*!< Form Error - Indicates that a Form Error was detected by the receiver node (that is, a fixed-form bit field contains at least one illegal bit). */
#define CAN_ESR1_CRCERR                      (BIT(12))                                           /*!< Cyclic Redundancy Check Error - Indicates that a CRC Error was detected by the receiver node (that is, the calculated CRC is different from the received CRC). */
#define CAN_ESR1_ACKERR                      (BIT(13))                                           /*!< Acknowledge Error - Indicates that an Acknowledge Error was detected by the transmitter node (that is, a dominant bit wasn't detected during the ACK SLOT). */
#define CAN_ESR1_BIT0ERR                     (BIT(14))                                           /*!< Bit0 Error - Indicates when an inconsistency occurs between the transmitted and the received bit in a message. */
#define CAN_ESR1_BIT1ERR                     (BIT(15))                                           /*!< Bit1 Error - Indicates when an inconsistency occurs between the transmitted and the received bit in a message. */
#define CAN_ESR1_RWRNINT                     (BIT(16))                                           /*!< Rx Warning Interrupt Flag. */
#define CAN_ESR1_TWRNINT                     (BIT(17))                                           /*!< Tx Warning Interrupt Flag. */
#define CAN_ESR1_SYNCH                       (BIT(18))                                           /*!< CAN Synchronization Status. */
#define CAN_ESR1_BOFFDONEINT                 (BIT(19))                                           /*!< Bus Off Done Interrupt. */
#define CAN_ESR1_ERROVR                      (BIT(21))                                           /*!< Error Overrun - Indicates that an error condition occurred when any error flag is already set. */

/* CAN_IMASK1 */
#define CAN_IMASK1_BUF15TO0M                 (BITS(0,15))                                        /*!< MBi Mask Bits - Each bit enables or disables the corresponding interrupt for MB31 to MB0. */

/* CAN_IFLAG1 */
#define CAN_IFLAG1_BUF0I                     (BIT(0))                                            /*!< MB0 Interrupt or Clear FIFO bit. */
#define CAN_IFLAG1_BUF1I                     (BIT(1))                                            /*!< MB1 Interrupt or Reserved. */
#define CAN_IFLAG1_BUF2I                     (BIT(2))                                            /*!< MB2 Interrupt or Reserved. */
#define CAN_IFLAG1_BUF3I                     (BIT(3))                                            /*!< MB3 Interrupt or Reserved. */
#define CAN_IFLAG1_BUF4I                     (BIT(4))                                            /*!< MB4 Interrupt or Reserved. */
#define CAN_IFLAG1_BUF5I                     (BIT(5))                                            /*!< MB5 Interrupt or Frames Available in Rx FIFO. */
#define CAN_IFLAG1_BUF6I                     (BIT(6))                                            /*!< MB6 Interrupt or Rx FIFO Warning. */
#define CAN_IFLAG1_BUF7I                     (BIT(7))                                            /*!< MB7 Interrupt or Rx FIFO Overflow. */
#define CAN_IFLAG1_BUF15TO8I                 (BITS(8,15))                                        /*!< MBi Interrupt Flag Bits - Each bit flags the corresponding Message Buffer Interrupt for MB15 to MB8. */

/* CAN_CTRL2 */
#define CAN_CTRL2_BTEEN                      (BIT(13))                                           /*!< Baudrate Config Type. */
#define CAN_CTRL2_TIMERSRC                   (BIT(15))                                           /*!< Timer Source choice. */
#define CAN_CTRL2_EACEN                      (BIT(16))                                           /*!< Entire Frame Arbitration Field Comparison Enable For Rx Mailboxes. */
#define CAN_CTRL2_RRS                        (BIT(17))                                           /*!< Remote Request Storing. */
#define CAN_CTRL2_MRP                        (BIT(18))                                           /*!< Mailboxes Reception Priority. */
#define CAN_CTRL2_TASD                       (BITS(19,23))                                       /*!< Tx Arbitration Start Delay - Indicates how many CAN bits the Tx arbitration process start point can be delayed from the first bit of CRC field on CAN bus. See "23.3.10.6 Tx Arbitration start delay". */
#define CAN_CTRL2_RFFN                       (BITS(24,27))                                       /*!< The number of Rx FIFO filters, as shown in Table 23-12 RFFN must not be programmed with values that make the number of MBs occupied by Rx FIFO and ID Filter exceed the number of mailboxes defined by CAN_MCR.MAXMB. */
#define CAN_CTRL2_BOFFDONEMSK                (BIT(30))                                           /*!< Bus Off Done Interrupt (CAN_ESR1.BOFFDONEINT) Mask. */

/* CAN_ESR2 */
#define CAN_ESR2_IMB                         (BIT(13))                                           /*!< Inactive Mailbox. */
#define CAN_ESR2_VPS                         (BIT(14))                                           /*!< Indicates whether CAN_ESR2.IMB and CAN_ESR2.LPTM contents are currently valid or not. */
#define CAN_ESR2_TWAMSK                      (BIT(15))                                           /*!< Time Overflow Interrupt Enable. */
#define CAN_ESR2_LPTM                        (BITS(16,22))                                       /*!< Lowest Priority Tx Mailbox. */
#define CAN_ESR2_TWAINT                      (BIT(31))                                           /*!< Time Overflow Interrupt Flag. */

/* CAN_CRCR */
#define CAN_CRCR_TXCRC                       (BITS(0,14))                                        /*!< Transmitted CRC Value - Indicates the CRC value of the last transmitted message. */
#define CAN_CRCR_MBCRC                       (BITS(16,22))                                       /*!< CRC Mailbox - Indicates the number of the mailbox corresponding to the value in the CAN_CRCR.TXCRC field. */

/* CAN_RXFGMASK */
#define CAN_RXFGMASK_FGM                     (BITS(0,31))                                        /*!< Rx FIFO Global Mask Bits - Mask the ID Filter Table elements bits. */

/* CAN_RXFIR */
#define CAN_RXFIR_IDHIT                      (BITS(0,8))                                         /*!< ID Acceptance Filter Hit Indicator - Indicates which ID Acceptance Filter (IDAF) was hit by the received message in the Rx FIFO. */

/* CAN_CBT */
#define CAN_CBT_EPSEG2                       (BITS(0,4))                                         /*!< Extended Phase Segment 2. */
#define CAN_CBT_EPSEG1                       (BITS(5,9))                                         /*!< Extended Phase Segment 1. */
#define CAN_CBT_EPROPSEG                     (BITS(10,15))                                       /*!< Extended Propagation Segment. */
#define CAN_CBT_ERJW                         (BITS(16,20))                                       /*!< Extended Resync Jump Width. */
#define CAN_CBT_ERPRSDIV                     (BITS(21,30))                                       /*!< Extended Prescaler Division Factor. */
#define CAN_CBT_BTF                          (BIT(31))                                           /*!< Bit Timing Format - Enables the use of extended CAN bit timing fields EPRESDIV, EPROPSEG, EPSEG1, EPSEG2 and ERJW, replacing the CAN bit timing variables defined in CAN_CTRL1. */

/* CAN_CS */
#define CAN_CS_TIME_STAMP                    (BITS(0,15))                                        /*!< Time Stamp Field. */
#define CAN_CS_DLC                           (BITS(16,19))                                       /*!< Data Length Code. */
#define CAN_CS_RTR                           (BIT(20))                                           /*!< Remote Transmission Request. */
#define CAN_CS_IDE                           (BIT(21))                                           /*!< ID type. */
#define CAN_CS_SRR                           (BIT(22))                                           /*!< Substitute Remote Request. */
#define CAN_CS_CODE                          (BITS(24,27))                                       /*!< Mailbox Code Field. */
#define CAN_CS_ESI                           (BIT(29))                                           /*!< Error Status in Tx. */
#define CAN_CS_BRS                           (BIT(30))
#define CAN_CS_EDL                           (BIT(31))

/* CAN_ID */
#define CAN_ID_EXT                           (BITS(0,17))
#define CAN_ID_STD                           (BITS(18,28))
#define CAN_ID_PRIO                          (BITS(29,31))

/* CAN_WORD0 */
#define CAN_WORD0_DATA                       (BITS(0,31))

/* CAN_WORD1 */
#define CAN_WORD1_DATA                       (BITS(0,31))

/* CAN_RXIMR */
#define CAN_RXIMR_MI                         (BITS(0,31))                                        /*!< Individual Mask Bits. */

/* CAN_EPRS */
#define CAN_EPRS_ENPRESDIV                   (BITS(0,9))

/* CAN_ECBT */
#define CAN_ECBT_NTSEG1                      (BITS(0,7))                                         /*!< Phase Buffer Segment 1 */
#define CAN_ECBT_NTSEG2                      (BITS(12,18))                                       /*!< Phase Buffer Segment 2 */
#define CAN_ECBT_NRJW                        (BITS(22,28))                                       /*!< Resynchronization jump width */

/* CAN_DMA_RBADR */
#define CAN_DMA_RBADR_ADDR                   (BITS(2,17))

/* CAN_DMA_RLEN */
#define CAN_DMA_RLEN_LEN                     (BITS(2,17))

/* CAN_DMA_RADR */
#define CAN_DMA_RADR_ADDR                    (BITS(2,17))

/* CAN_DMA_IE */
#define CAN_DMA_IE_RXHFULL                   (BIT(0))                                            /*!< Rx Half-Full Mask Bits. */
#define CAN_DMA_IE_RXFULL                    (BIT(1))                                            /*!< Rx Full Mask Bits. */

/* CAN_DMA_IF */
#define CAN_DMA_IF_RXHFULL                   (BIT(0))                                            /*!< Rx Half-Full Mask Bits. */
#define CAN_DMA_IF_RXFULL                    (BIT(1))                                            /*!< Rx Full Mask Bits. */

/* CAN_TMR_SEL */
#define CAN_TMR_SEL_FILTER                   (BIT(0))                                            /*!< Configure external input sources for filtering processing. */

/* CAN_STOP_MD */
#define CAN_STOP_MD_PWD                      (BITS(0,31))                                        /*!< The key of entering stop mode. */

/* constants definitions */
/* CAN_MCR_MAXMB */
#define CAN_MCR_MAXMB_POS                    (0U)                                                /*!< CAN_MCR_MAXMB bit offset */
#define CAN_MCR_MAXMB_CFG(regval)            (CAN_MCR_MAXMB & ((uint32_t)(regval) << CAN_MCR_MAXMB_POS))
#define CAN_MCR_MAXMB_0                      (CAN_MCR_MAXMB_CFG(0x01))
#define CAN_MCR_MAXMB_1                      (CAN_MCR_MAXMB_CFG(0x02))
#define CAN_MCR_MAXMB_2                      (CAN_MCR_MAXMB_CFG(0x04))
#define CAN_MCR_MAXMB_3                      (CAN_MCR_MAXMB_CFG(0x08))
#define CAN_MCR_MAXMB_4                      (CAN_MCR_MAXMB_CFG(0x10))
#define CAN_MCR_MAXMB_5                      (CAN_MCR_MAXMB_CFG(0x20))
#define CAN_MCR_MAXMB_6                      (CAN_MCR_MAXMB_CFG(0x40))

/* CAN_MCR_IDAM */
#define CAN_MCR_IDAM_POS                     (8U)                                                /*!< CAN_MCR_IDAM bit offset */
#define CAN_MCR_IDAM_CFG(regval)             (CAN_MCR_IDAM & ((uint32_t)(regval) << CAN_MCR_IDAM_POS))
#define CAN_MCR_IDAM_0                       (CAN_MCR_IDAM_CFG(0x0))                             /*!< One full ID (standard and extended) per ID Filter Table element. */
#define CAN_MCR_IDAM_1                       (CAN_MCR_IDAM_CFG(0x1))                             /*!< Two full standard IDs or two partial 14-bit extended IDs per ID Filter Table element. */
#define CAN_MCR_IDAM_2                       (CAN_MCR_IDAM_CFG(0x2))                             /*!< Four partial 8-bit standard IDs per ID Filter Table element. */
#define CAN_MCR_IDAM_3                       (CAN_MCR_IDAM_CFG(0x3))                             /*!< All frames rejected. */

/* CAN_MCR_AEN */
#define CAN_MCR_AEN_POS                      (12U)                                               /*!< CAN_MCR_AEN bit offset */

/* CAN_MCR_LPRIOEN */
#define CAN_MCR_LPRIOEN_POS                  (13U)                                               /*!< CAN_MCR_LPRIOEN bit offset */

/* CAN_MCR_PNETEN */
#define CAN_MCR_PNETEN_POS                   (14U)                                               /*!< CAN_MCR_PNETEN bit offset */

/* CAN_MCR_DMA */
#define CAN_MCR_DMA_POS                      (15U)                                               /*!< CAN_MCR_DMA bit offset */

/* CAN_MCR_IRMQ */
#define CAN_MCR_IRMQ_POS                     (16U)                                               /*!< CAN_MCR_IRMQ bit offset */

/* CAN_MCR_SRXDIS */
#define CAN_MCR_SRXDIS_POS                   (17U)                                               /*!< CAN_MCR_SRXDIS bit offset */

/* CAN_MCR_DOZE */
#define CAN_MCR_DOZE_POS                     (18U)                                               /*!< CAN_MCR_DOZE bit offset */

/* CAN_MCR_WAKSRC */
#define CAN_MCR_WAKSRC_POS                   (19U)                                               /*!< CAN_MCR_WAKSRC bit offset */

/* CAN_MCR_LPMACK */
#define CAN_MCR_LPMACK_POS                   (20U)                                               /*!< CAN_MCR_LPMACK bit offset */

/* CAN_MCR_WRNEN */
#define CAN_MCR_WRNEN_POS                    (21U)                                               /*!< CAN_MCR_WRNEN bit offset */

/* CAN_MCR_SLFWAK */
#define CAN_MCR_SLFWAK_POS                   (22U)                                               /*!< CAN_MCR_SLFWAK bit offset */

/* CAN_MCR_FRZACK */
#define CAN_MCR_FRZACK_POS                   (24U)                                               /*!< CAN_MCR_FRZACK bit offset */

/* CAN_MCR_SOFTRST */
#define CAN_MCR_SOFTRST_POS                  (25U)                                               /*!< CAN_MCR_SOFTRST bit offset */

/* CAN_MCR_WAKMSK */
#define CAN_MCR_WAKMSK_POS                   (26U)                                               /*!< CAN_MCR_WAKMSK bit offset */

/* CAN_MCR_NOTRDY */
#define CAN_MCR_NOTRDY_POS                   (27U)                                               /*!< CAN_MCR_NOTRDY bit offset */

/* CAN_MCR_HALT */
#define CAN_MCR_HALT_POS                     (28U)                                               /*!< CAN_MCR_HALT bit offset */

/* CAN_MCR_RFEN */
#define CAN_MCR_RFEN_POS                     (29U)                                               /*!< CAN_MCR_RFEN bit offset */

/* CAN_MCR_FRZ */
#define CAN_MCR_FRZ_POS                      (30U)                                               /*!< CAN_MCR_FRZ bit offset */

/* CAN_MCR_MDIS */
#define CAN_MCR_MDIS_POS                     (31U)                                               /*!< CAN_MCR_MDIS bit offset */

/* CAN_CTRL1_PROPSEG */
#define CAN_CTRL1_PROPSEG_POS                (0U)                                                /*!< CAN_CTRL1_PROPSEG bit offset */
#define CAN_CTRL1_PROPSEG_0                  (0x1U << CAN_CTRL1_PROPSEG_POS)
#define CAN_CTRL1_PROPSEG_1                  (0x2U << CAN_CTRL1_PROPSEG_POS)
#define CAN_CTRL1_PROPSEG_2                  (0x4U << CAN_CTRL1_PROPSEG_POS)

/* CAN_CTRL1_LOM */
#define CAN_CTRL1_LOM_POS                    (3U)                                                /*!< CAN_CTRL1_LOM bit offset */

/* CAN_CTRL1_LBUF */
#define CAN_CTRL1_LBUF_POS                   (4U)                                                /*!< CAN_CTRL1_LBUF bit offset */

/* CAN_CTRL1_TSYN */
#define CAN_CTRL1_TSYN_POS                   (5U)                                                /*!< CAN_CTRL1_TSYN bit offset */

/* CAN_CTRL1_BOFFRREC */
#define CAN_CTRL1_BOFFRREC_POS               (6U)                                                /*!< CAN_CTRL1_BOFFRREC bit offset */

/* CAN_CTRL1_SMP */
#define CAN_CTRL1_SMP_POS                    (7U)                                                /*!< CAN_CTRL1_SMP bit offset */

/* CAN_CTRL1_RWRNMSK */
#define CAN_CTRL1_RWRNMSK_POS                (10U)                                               /*!< CAN_CTRL1_RWRNMSK bit offset */

/* CAN_CTRL1_TWRNMSK */
#define CAN_CTRL1_TWRNMSK_POS                (11U)                                               /*!< CAN_CTRL1_TWRNMSK bit offset */

/* CAN_CTRL1_LPB */
#define CAN_CTRL1_LPB_POS                    (12U)                                               /*!< CAN_CTRL1_LPB bit offset */

/* CAN_CTRL1_ERRMSK */
#define CAN_CTRL1_ERRMSK_POS                 (14U)                                               /*!< CAN_CTRL1_ERRMSK bit offset */

/* CAN_CTRL1_BOFFMSK */
#define CAN_CTRL1_BOFFMSK_POS                (15U)                                               /*!< CAN_CTRL1_BOFFMSK bit offset */

/* CAN_CTRL1_PSEG2 */
#define CAN_CTRL1_PSEG2_POS                  (16U)                                               /*!< CAN_CTRL1_PSEG2 bit offset */
#define CAN_CTRL1_PSEG2_0                    (0x1U << CAN_CTRL1_PSEG2_POS)
#define CAN_CTRL1_PSEG2_1                    (0x2U << CAN_CTRL1_PSEG2_POS)
#define CAN_CTRL1_PSEG2_2                    (0x4U << CAN_CTRL1_PSEG2_POS)

/* CAN_CTRL1_PSEG1 */
#define CAN_CTRL1_PSEG1_POS                  (19U)                                               /*!< CAN_CTRL1_PSEG1 bit offset */
#define CAN_CTRL1_PSEG1_0                    (0x1U << CAN_CTRL1_PSEG1_POS)
#define CAN_CTRL1_PSEG1_1                    (0x2U << CAN_CTRL1_PSEG1_POS)
#define CAN_CTRL1_PSEG1_2                    (0x4U << CAN_CTRL1_PSEG1_POS)

/* CAN_CTRL1_RJW */
#define CAN_CTRL1_RJW_POS                    (22U)                                               /*!< CAN_CTRL1_RJW bit offset */
#define CAN_CTRL1_RJW_0                      (0x1U << CAN_CTRL1_RJW_POS)
#define CAN_CTRL1_RJW_1                      (0x2U << CAN_CTRL1_RJW_POS)

/* CAN_CTRL1_PRESDIV */
#define CAN_CTRL1_PRESDIV_POS                (24U)                                               /*!< CAN_CTRL1_PRESDIV bit offset */
#define CAN_CTRL1_PRESDIV_0                  (0x01U << CAN_CTRL1_PRESDIV_POS)
#define CAN_CTRL1_PRESDIV_1                  (0x02U << CAN_CTRL1_PRESDIV_POS)
#define CAN_CTRL1_PRESDIV_2                  (0x04U << CAN_CTRL1_PRESDIV_POS)
#define CAN_CTRL1_PRESDIV_3                  (0x08U << CAN_CTRL1_PRESDIV_POS)
#define CAN_CTRL1_PRESDIV_4                  (0x10U << CAN_CTRL1_PRESDIV_POS)
#define CAN_CTRL1_PRESDIV_5                  (0x20U << CAN_CTRL1_PRESDIV_POS)
#define CAN_CTRL1_PRESDIV_6                  (0x40U << CAN_CTRL1_PRESDIV_POS)
#define CAN_CTRL1_PRESDIV_7                  (0x80U << CAN_CTRL1_PRESDIV_POS)

/* CAN_TIMER_TIMER */
#define CAN_TIMER_TIMER_POS                  (0U)                                                /*!< CAN_TIMER_TIMER bit offset */
#define CAN_TIMER_TIMER_0                    (0x0001U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_1                    (0x0002U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_2                    (0x0004U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_3                    (0x0008U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_4                    (0x0010U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_5                    (0x0020U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_6                    (0x0040U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_7                    (0x0080U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_8                    (0x0100U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_9                    (0x0200U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_10                   (0x0400U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_11                   (0x0800U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_12                   (0x1000U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_13                   (0x2000U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_14                   (0x4000U << CAN_TIMER_TIMER_POS)
#define CAN_TIMER_TIMER_15                   (0x8000U << CAN_TIMER_TIMER_POS)

/* CAN_RXMGMASK_MG */
#define CAN_RXMGMASK_MG_POS                  (0U)                                                /*!< CAN_RXMGMASK_MG bit offset */
#define CAN_RXMGMASK_MG_0                    (0x00000001U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_1                    (0x00000002U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_2                    (0x00000004U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_3                    (0x00000008U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_4                    (0x00000010U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_5                    (0x00000020U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_6                    (0x00000040U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_7                    (0x00000080U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_8                    (0x00000100U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_9                    (0x00000200U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_10                   (0x00000400U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_11                   (0x00000800U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_12                   (0x00001000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_13                   (0x00002000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_14                   (0x00004000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_15                   (0x00008000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_16                   (0x00010000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_17                   (0x00020000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_18                   (0x00040000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_19                   (0x00080000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_20                   (0x00100000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_21                   (0x00200000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_22                   (0x00400000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_23                   (0x00800000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_24                   (0x01000000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_25                   (0x02000000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_26                   (0x04000000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_27                   (0x08000000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_28                   (0x10000000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_29                   (0x20000000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_30                   (0x40000000U << CAN_RXMGMASK_MG_POS)
#define CAN_RXMGMASK_MG_31                   (0x80000000U << CAN_RXMGMASK_MG_POS)

/* CAN_RX14MASK_RX14M */
#define CAN_RX14MASK_RX14M_POS               (0U)                                                /*!< CAN_RX14MASK_RX14M bit offset */
#define CAN_RX14MASK_RX14M_0                 (0x00000001U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_1                 (0x00000002U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_2                 (0x00000004U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_3                 (0x00000008U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_4                 (0x00000010U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_5                 (0x00000020U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_6                 (0x00000040U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_7                 (0x00000080U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_8                 (0x00000100U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_9                 (0x00000200U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_10                (0x00000400U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_11                (0x00000800U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_12                (0x00001000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_13                (0x00002000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_14                (0x00004000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_15                (0x00008000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_16                (0x00010000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_17                (0x00020000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_18                (0x00040000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_19                (0x00080000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_20                (0x00100000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_21                (0x00200000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_22                (0x00400000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_23                (0x00800000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_24                (0x01000000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_25                (0x02000000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_26                (0x04000000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_27                (0x08000000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_28                (0x10000000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_29                (0x20000000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_30                (0x40000000U << CAN_RX14MASK_RX14M_POS)
#define CAN_RX14MASK_RX14M_31                (0x80000000U << CAN_RX14MASK_RX14M_POS)

/* CAN_RX15MASK_RX15M */
#define CAN_RX15MASK_RX15M_POS               (0U)                                                /*!< CAN_RX15MASK_RX15M bit offset */
#define CAN_RX15MASK_RX15M_0                 (0x00000001U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_1                 (0x00000002U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_2                 (0x00000004U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_3                 (0x00000008U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_4                 (0x00000010U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_5                 (0x00000020U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_6                 (0x00000040U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_7                 (0x00000080U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_8                 (0x00000100U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_9                 (0x00000200U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_10                (0x00000400U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_11                (0x00000800U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_12                (0x00001000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_13                (0x00002000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_14                (0x00004000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_15                (0x00008000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_16                (0x00010000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_17                (0x00020000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_18                (0x00040000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_19                (0x00080000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_20                (0x00100000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_21                (0x00200000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_22                (0x00400000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_23                (0x00800000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_24                (0x01000000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_25                (0x02000000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_26                (0x04000000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_27                (0x08000000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_28                (0x10000000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_29                (0x20000000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_30                (0x40000000U << CAN_RX15MASK_RX15M_POS)
#define CAN_RX15MASK_RX15M_31                (0x80000000U << CAN_RX15MASK_RX15M_POS)

/* CAN_ECR_TXERRCNT */
#define CAN_ECR_TXERRCNT_POS                 (0U)                                                /*!< CAN_ECR_TXERRCNT bit offset */
#define CAN_ECR_TXERRCNT_0                   (0x01U << CAN_ECR_TXERRCNT_POS)
#define CAN_ECR_TXERRCNT_1                   (0x02U << CAN_ECR_TXERRCNT_POS)
#define CAN_ECR_TXERRCNT_2                   (0x04U << CAN_ECR_TXERRCNT_POS)
#define CAN_ECR_TXERRCNT_3                   (0x08U << CAN_ECR_TXERRCNT_POS)
#define CAN_ECR_TXERRCNT_4                   (0x10U << CAN_ECR_TXERRCNT_POS)
#define CAN_ECR_TXERRCNT_5                   (0x20U << CAN_ECR_TXERRCNT_POS)
#define CAN_ECR_TXERRCNT_6                   (0x40U << CAN_ECR_TXERRCNT_POS)
#define CAN_ECR_TXERRCNT_7                   (0x80U << CAN_ECR_TXERRCNT_POS)

/* CAN_ECR_RXERRCNT */
#define CAN_ECR_RXERRCNT_POS                 (8U)                                                /*!< CAN_ECR_RXERRCNT bit offset */
#define CAN_ECR_RXERRCNT_0                   (0x01U << CAN_ECR_RXERRCNT_POS)
#define CAN_ECR_RXERRCNT_1                   (0x02U << CAN_ECR_RXERRCNT_POS)
#define CAN_ECR_RXERRCNT_2                   (0x04U << CAN_ECR_RXERRCNT_POS)
#define CAN_ECR_RXERRCNT_3                   (0x08U << CAN_ECR_RXERRCNT_POS)
#define CAN_ECR_RXERRCNT_4                   (0x10U << CAN_ECR_RXERRCNT_POS)
#define CAN_ECR_RXERRCNT_5                   (0x20U << CAN_ECR_RXERRCNT_POS)
#define CAN_ECR_RXERRCNT_6                   (0x40U << CAN_ECR_RXERRCNT_POS)
#define CAN_ECR_RXERRCNT_7                   (0x80U << CAN_ECR_RXERRCNT_POS)

/* CAN_ESR1_WAKINT */
#define CAN_ESR1_WAKINT_POS                  (0U)                                                /*!< CAN_ESR1_WAKINT bit offset */

/* CAN_ESR1_ERRINT */
#define CAN_ESR1_ERRINT_POS                  (1U)                                                /*!< CAN_ESR1_ERRINT bit offset */

/* CAN_ESR1_BOFFINT */
#define CAN_ESR1_BOFFINT_POS                 (2U)                                                /*!< CAN_ESR1_BOFFINT bit offset */

/* CAN_ESR1_RX */
#define CAN_ESR1_RX_POS                      (3U)                                                /*!< CAN_ESR1_RX bit offset */

/* CAN_ESR1_FLTCONF */
#define CAN_ESR1_FLTCONF_POS                 (4U)                                                /*!< CAN_ESR1_FLTCONF bit offset */
#define CAN_ESR1_FLTCONF_CFG(regval)         (CAN_ESR1_FLTCONF & ((uint32_t)(regval) << CAN_ESR1_FLTCONF_POS))
#define CAN_ESR1_FLTCONF_ERR_ACTIVE          (CAN_ESR1_FLTCONF_CFG(0x0))                         /*!< Error Active. */
#define CAN_ESR1_FLTCONF_ERR_PASSIVE         (CAN_ESR1_FLTCONF_CFG(0x1))                         /*!< Error passive. */
#define CAN_ESR1_FLTCONF_BUF_OFF             (CAN_ESR1_FLTCONF_CFG(0x2))                         /*!< Buf off. */

/* CAN_ESR1_TX */
#define CAN_ESR1_TX_POS                      (6U)                                                /*!< CAN_ESR1_TX bit offset */

/* CAN_ESR1_IDLE */
#define CAN_ESR1_IDLE_POS                    (7U)                                                /*!< CAN_ESR1_IDLE bit offset */

/* CAN_ESR1_RXWRN */
#define CAN_ESR1_RXWRN_POS                   (8U)                                                /*!< CAN_ESR1_RXWRN bit offset */

/* CAN_ESR1_TXWRN */
#define CAN_ESR1_TXWRN_POS                   (9U)                                                /*!< CAN_ESR1_TXWRN bit offset */

/* CAN_ESR1_STFERR */
#define CAN_ESR1_STFERR_POS                  (10U)                                               /*!< CAN_ESR1_STFERR bit offset */

/* CAN_ESR1_FRMERR */
#define CAN_ESR1_FRMERR_POS                  (11U)                                               /*!< CAN_ESR1_FRMERR bit offset */

/* CAN_ESR1_CRCERR */
#define CAN_ESR1_CRCERR_POS                  (12U)                                               /*!< CAN_ESR1_CRCERR bit offset */

/* CAN_ESR1_ACKERR */
#define CAN_ESR1_ACKERR_POS                  (13U)                                               /*!< CAN_ESR1_ACKERR bit offset */

/* CAN_ESR1_BIT0ERR */
#define CAN_ESR1_BIT0ERR_POS                 (14U)                                               /*!< CAN_ESR1_BIT0ERR bit offset */

/* CAN_ESR1_BIT1ERR */
#define CAN_ESR1_BIT1ERR_POS                 (15U)                                               /*!< CAN_ESR1_BIT1ERR bit offset */

/* CAN_ESR1_RWRNINT */
#define CAN_ESR1_RWRNINT_POS                 (16U)                                               /*!< CAN_ESR1_RWRNINT bit offset */

/* CAN_ESR1_TWRNINT */
#define CAN_ESR1_TWRNINT_POS                 (17U)                                               /*!< CAN_ESR1_TWRNINT bit offset */

/* CAN_ESR1_SYNCH */
#define CAN_ESR1_SYNCH_POS                   (18U)                                               /*!< CAN_ESR1_SYNCH bit offset */

/* CAN_ESR1_BOFFDONEINT */
#define CAN_ESR1_BOFFDONEINT_POS             (19U)                                               /*!< CAN_ESR1_BOFFDONEINT bit offset */

/* CAN_ESR1_ERROVR */
#define CAN_ESR1_ERROVR_POS                  (21U)                                               /*!< CAN_ESR1_ERROVR bit offset */

/* CAN_IMASK1_BUF15TO0M */
#define CAN_IMASK1_BUF15TO0M_POS             (0U)                                                /*!< CAN_IMASK1_BUF15TO0M bit offset */
#define CAN_IMASK1_BUF15TO0M_0               (0x0001U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_1               (0x0002U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_2               (0x0004U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_3               (0x0008U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_4               (0x0010U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_5               (0x0020U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_6               (0x0040U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_7               (0x0080U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_8               (0x0100U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_9               (0x0200U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_10              (0x0400U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_11              (0x0800U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_12              (0x1000U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_13              (0x2000U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_14              (0x4000U << CAN_IMASK1_BUF15TO0M_POS)
#define CAN_IMASK1_BUF15TO0M_15              (0x8000U << CAN_IMASK1_BUF15TO0M_POS)

/* CAN_IFLAG1_BUF0I */
#define CAN_IFLAG1_BUF0I_POS                 (0U)                                                /*!< CAN_IFLAG1_BUF0I bit offset */

/* CAN_IFLAG1_BUF1I */
#define CAN_IFLAG1_BUF1I_POS                 (1U)                                                /*!< CAN_IFLAG1_BUF1I bit offset */

/* CAN_IFLAG1_BUF2I */
#define CAN_IFLAG1_BUF2I_POS                 (2U)                                                /*!< CAN_IFLAG1_BUF2I bit offset */

/* CAN_IFLAG1_BUF3I */
#define CAN_IFLAG1_BUF3I_POS                 (3U)                                                /*!< CAN_IFLAG1_BUF3I bit offset */

/* CAN_IFLAG1_BUF4I */
#define CAN_IFLAG1_BUF4I_POS                 (4U)                                                /*!< CAN_IFLAG1_BUF4I bit offset */

/* CAN_IFLAG1_BUF5I */
#define CAN_IFLAG1_BUF5I_POS                 (5U)                                                /*!< CAN_IFLAG1_BUF5I bit offset */

/* CAN_IFLAG1_BUF6I */
#define CAN_IFLAG1_BUF6I_POS                 (6U)                                                /*!< CAN_IFLAG1_BUF6I bit offset */

/* CAN_IFLAG1_BUF7I */
#define CAN_IFLAG1_BUF7I_POS                 (7U)                                                /*!< CAN_IFLAG1_BUF7I bit offset */

/* CAN_IFLAG1_BUF15TO8I */
#define CAN_IFLAG1_BUF15TO8I_POS             (8U)                                                /*!< CAN_IFLAG1_BUF15TO8I bit offset */
#define CAN_IFLAG1_BUF15TO8I_0               (0x01U << CAN_IFLAG1_BUF15TO8I_POS)
#define CAN_IFLAG1_BUF15TO8I_1               (0x02U << CAN_IFLAG1_BUF15TO8I_POS)
#define CAN_IFLAG1_BUF15TO8I_2               (0x04U << CAN_IFLAG1_BUF15TO8I_POS)
#define CAN_IFLAG1_BUF15TO8I_3               (0x08U << CAN_IFLAG1_BUF15TO8I_POS)
#define CAN_IFLAG1_BUF15TO8I_4               (0x10U << CAN_IFLAG1_BUF15TO8I_POS)
#define CAN_IFLAG1_BUF15TO8I_5               (0x20U << CAN_IFLAG1_BUF15TO8I_POS)
#define CAN_IFLAG1_BUF15TO8I_6               (0x40U << CAN_IFLAG1_BUF15TO8I_POS)
#define CAN_IFLAG1_BUF15TO8I_7               (0x80U << CAN_IFLAG1_BUF15TO8I_POS)

/* CAN_CTRL2_BTEEN */
#define CAN_CTRL2_BTEEN_POS                  (13U)                                               /*!< CAN_CTRL2_BTEEN bit offset */

/* CAN_CTRL2_TIMERSRC */
#define CAN_CTRL2_TIMERSRC_POS               (15U)                                               /*!< CAN_CTRL2_TIMERSRC bit offset */

/* CAN_CTRL2_EACEN */
#define CAN_CTRL2_EACEN_POS                  (16U)                                               /*!< CAN_CTRL2_EACEN bit offset */

/* CAN_CTRL2_RRS */
#define CAN_CTRL2_RRS_POS                    (17U)                                               /*!< CAN_CTRL2_RRS bit offset */

/* CAN_CTRL2_MRP */
#define CAN_CTRL2_MRP_POS                    (18U)                                               /*!< CAN_CTRL2_MRP bit offset */

/* CAN_CTRL2_TASD */
#define CAN_CTRL2_TASD_POS                   (19U)                                               /*!< CAN_CTRL2_TASD bit offset */
#define CAN_CTRL2_TASD_0                     ((uint32_t)0x01U << CAN_CTRL2_TASD_POS)
#define CAN_CTRL2_TASD_1                     ((uint32_t)0x02U << CAN_CTRL2_TASD_POS)
#define CAN_CTRL2_TASD_2                     ((uint32_t)0x04U << CAN_CTRL2_TASD_POS)
#define CAN_CTRL2_TASD_3                     ((uint32_t)0x08U << CAN_CTRL2_TASD_POS)
#define CAN_CTRL2_TASD_4                     ((uint32_t)0x10U << CAN_CTRL2_TASD_POS)

/* CAN_CTRL2_RFFN */
#define CAN_CTRL2_RFFN_POS                   (24U)                                               /*!< CAN_CTRL2_RFFN bit offset */
#define CAN_CTRL2_RFFN_0                     ((uint32_t)0x1U << CAN_CTRL2_RFFN_POS)
#define CAN_CTRL2_RFFN_1                     ((uint32_t)0x2U << CAN_CTRL2_RFFN_POS)
#define CAN_CTRL2_RFFN_2                     ((uint32_t)0x4U << CAN_CTRL2_RFFN_POS)
#define CAN_CTRL2_RFFN_3                     ((uint32_t)0x8U << CAN_CTRL2_RFFN_POS)

/* CAN_CTRL2_BOFFDONEMSK */
#define CAN_CTRL2_BOFFDONEMSK_POS            (30U)                                               /*!< CAN_CTRL2_BOFFDONEMSK bit offset */

/* CAN_ESR2_IMB */
#define CAN_ESR2_IMB_POS                     (13U)                                               /*!< CAN_ESR2_IMB bit offset */

/* CAN_ESR2_VPS */
#define CAN_ESR2_VPS_POS                     (14U)                                               /*!< CAN_ESR2_VPS bit offset */

/* CAN_ESR2_TWAMSK */
#define CAN_ESR2_TWAMSK_POS                  (15U)                                               /*!< CAN_ESR2_TWAMSK bit offset */

/* CAN_ESR2_LPTM */
#define CAN_ESR2_LPTM_POS                    (16U)                                               /*!< CAN_ESR2_LPTM bit offset */
#define CAN_ESR2_LPTM_0                      ((uint32_t)0x01U << CAN_ESR2_LPTM_POS)
#define CAN_ESR2_LPTM_1                      ((uint32_t)0x02U << CAN_ESR2_LPTM_POS)
#define CAN_ESR2_LPTM_2                      ((uint32_t)0x04U << CAN_ESR2_LPTM_POS)
#define CAN_ESR2_LPTM_3                      ((uint32_t)0x08U << CAN_ESR2_LPTM_POS)
#define CAN_ESR2_LPTM_4                      ((uint32_t)0x10U << CAN_ESR2_LPTM_POS)
#define CAN_ESR2_LPTM_5                      ((uint32_t)0x20U << CAN_ESR2_LPTM_POS)
#define CAN_ESR2_LPTM_6                      ((uint32_t)0x40U << CAN_ESR2_LPTM_POS)

/* CAN_ESR2_TWAINT */
#define CAN_ESR2_TWAINT_POS                  (31U)                                               /*!< CAN_ESR2_TWAINT bit offset */

/* CAN_CRCR_TXCRC */
#define CAN_CRCR_TXCRC_POS                   (0U)                                                /*!< CAN_CRCR_TXCRC bit offset */
#define CAN_CRCR_TXCRC_0                     (0x0001U << CAN_CRCR_TXCRC_POS)
#define CAN_CRCR_TXCRC_1                     (0x0002U << CAN_CRCR_TXCRC_POS)
#define CAN_CRCR_TXCRC_2                     (0x0004U << CAN_CRCR_TXCRC_POS)
#define CAN_CRCR_TXCRC_3                     (0x0008U << CAN_CRCR_TXCRC_POS)
#define CAN_CRCR_TXCRC_4                     (0x0010U << CAN_CRCR_TXCRC_POS)
#define CAN_CRCR_TXCRC_5                     (0x0020U << CAN_CRCR_TXCRC_POS)
#define CAN_CRCR_TXCRC_6                     (0x0040U << CAN_CRCR_TXCRC_POS)
#define CAN_CRCR_TXCRC_7                     (0x0080U << CAN_CRCR_TXCRC_POS)
#define CAN_CRCR_TXCRC_8                     (0x0100U << CAN_CRCR_TXCRC_POS)
#define CAN_CRCR_TXCRC_9                     (0x0200U << CAN_CRCR_TXCRC_POS)
#define CAN_CRCR_TXCRC_10                    (0x0400U << CAN_CRCR_TXCRC_POS)
#define CAN_CRCR_TXCRC_11                    (0x0800U << CAN_CRCR_TXCRC_POS)
#define CAN_CRCR_TXCRC_12                    (0x1000U << CAN_CRCR_TXCRC_POS)
#define CAN_CRCR_TXCRC_13                    (0x2000U << CAN_CRCR_TXCRC_POS)
#define CAN_CRCR_TXCRC_14                    (0x4000U << CAN_CRCR_TXCRC_POS)

/* CAN_CRCR_MBCRC */
#define CAN_CRCR_MBCRC_POS                   (16U)                                               /*!< CAN_CRCR_MBCRC bit offset */
#define CAN_CRCR_MBCRC_0                     (0x01U << CAN_CRCR_MBCRC_POS)
#define CAN_CRCR_MBCRC_1                     (0x02U << CAN_CRCR_MBCRC_POS)
#define CAN_CRCR_MBCRC_2                     (0x04U << CAN_CRCR_MBCRC_POS)
#define CAN_CRCR_MBCRC_3                     (0x08U << CAN_CRCR_MBCRC_POS)
#define CAN_CRCR_MBCRC_4                     (0x10U << CAN_CRCR_MBCRC_POS)
#define CAN_CRCR_MBCRC_5                     (0x20U << CAN_CRCR_MBCRC_POS)
#define CAN_CRCR_MBCRC_6                     (0x40U << CAN_CRCR_MBCRC_POS)

/* CAN_RXFGMASK_FGM */
#define CAN_RXFGMASK_FGM_POS                 (0U)                                                /*!< CAN_RXFGMASK_FGM bit offset */
#define CAN_RXFGMASK_FGM_0                   (0x00000001U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_1                   (0x00000002U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_2                   (0x00000004U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_3                   (0x00000008U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_4                   (0x00000010U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_5                   (0x00000020U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_6                   (0x00000040U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_7                   (0x00000080U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_8                   (0x00000100U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_9                   (0x00000200U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_10                  (0x00000400U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_11                  (0x00000800U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_12                  (0x00001000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_13                  (0x00002000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_14                  (0x00004000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_15                  (0x00008000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_16                  (0x00010000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_17                  (0x00020000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_18                  (0x00040000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_19                  (0x00080000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_20                  (0x00100000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_21                  (0x00200000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_22                  (0x00400000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_23                  (0x00800000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_24                  (0x01000000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_25                  (0x02000000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_26                  (0x04000000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_27                  (0x08000000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_28                  (0x10000000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_29                  (0x20000000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_30                  (0x40000000U << CAN_RXFGMASK_FGM_POS)
#define CAN_RXFGMASK_FGM_31                  (0x80000000U << CAN_RXFGMASK_FGM_POS)

/* CAN_RXFIR_IDHIT */
#define CAN_RXFIR_IDHIT_POS                  (0U)                                                /*!< CAN_RXFIR_IDHIT bit offset */
#define CAN_RXFIR_IDHIT_0                    (0x001U << CAN_RXFIR_IDHIT_POS)
#define CAN_RXFIR_IDHIT_1                    (0x002U << CAN_RXFIR_IDHIT_POS)
#define CAN_RXFIR_IDHIT_2                    (0x004U << CAN_RXFIR_IDHIT_POS)
#define CAN_RXFIR_IDHIT_3                    (0x008U << CAN_RXFIR_IDHIT_POS)
#define CAN_RXFIR_IDHIT_4                    (0x010U << CAN_RXFIR_IDHIT_POS)
#define CAN_RXFIR_IDHIT_5                    (0x020U << CAN_RXFIR_IDHIT_POS)
#define CAN_RXFIR_IDHIT_6                    (0x040U << CAN_RXFIR_IDHIT_POS)
#define CAN_RXFIR_IDHIT_7                    (0x080U << CAN_RXFIR_IDHIT_POS)
#define CAN_RXFIR_IDHIT_8                    (0x100U << CAN_RXFIR_IDHIT_POS)

/* CAN_CBT_EPSEG2 */
#define CAN_CBT_EPSEG2_POS                   (0U)                                                /*!< CAN_CBT_EPSEG2 bit offset */
#define CAN_CBT_EPSEG2_0                     (0x01U << CAN_CBT_EPSEG2_POS)
#define CAN_CBT_EPSEG2_1                     (0x02U << CAN_CBT_EPSEG2_POS)
#define CAN_CBT_EPSEG2_2                     (0x04U << CAN_CBT_EPSEG2_POS)
#define CAN_CBT_EPSEG2_3                     (0x08U << CAN_CBT_EPSEG2_POS)
#define CAN_CBT_EPSEG2_4                     (0x10U << CAN_CBT_EPSEG2_POS)

/* CAN_CBT_EPSEG1 */
#define CAN_CBT_EPSEG1_POS                   (5U)                                                /*!< CAN_CBT_EPSEG1 bit offset */
#define CAN_CBT_EPSEG1_0                     (0x01U << CAN_CBT_EPSEG1_POS)
#define CAN_CBT_EPSEG1_1                     (0x02U << CAN_CBT_EPSEG1_POS)
#define CAN_CBT_EPSEG1_2                     (0x04U << CAN_CBT_EPSEG1_POS)
#define CAN_CBT_EPSEG1_3                     (0x08U << CAN_CBT_EPSEG1_POS)
#define CAN_CBT_EPSEG1_4                     (0x10U << CAN_CBT_EPSEG1_POS)

/* CAN_CBT_EPROPSEG */
#define CAN_CBT_EPROPSEG_POS                 (10U)                                               /*!< CAN_CBT_EPROPSEG bit offset */
#define CAN_CBT_EPROPSEG_0                   (0x01U << CAN_CBT_EPROPSEG_POS)
#define CAN_CBT_EPROPSEG_1                   (0x02U << CAN_CBT_EPROPSEG_POS)
#define CAN_CBT_EPROPSEG_2                   (0x04U << CAN_CBT_EPROPSEG_POS)
#define CAN_CBT_EPROPSEG_3                   (0x08U << CAN_CBT_EPROPSEG_POS)
#define CAN_CBT_EPROPSEG_4                   (0x10U << CAN_CBT_EPROPSEG_POS)
#define CAN_CBT_EPROPSEG_5                   (0x20U << CAN_CBT_EPROPSEG_POS)

/* CAN_CBT_ERJW */
#define CAN_CBT_ERJW_POS                     (16U)                                               /*!< CAN_CBT_ERJW bit offset */
#define CAN_CBT_ERJW_0                       (0x01U << CAN_CBT_ERJW_POS)
#define CAN_CBT_ERJW_1                       (0x02U << CAN_CBT_ERJW_POS)
#define CAN_CBT_ERJW_2                       (0x04U << CAN_CBT_ERJW_POS)
#define CAN_CBT_ERJW_3                       (0x08U << CAN_CBT_ERJW_POS)
#define CAN_CBT_ERJW_4                       (0x10U << CAN_CBT_ERJW_POS)

/* CAN_CBT_ERPRSDIV */
#define CAN_CBT_ERPRSDIV_POS                 (21U)                                               /*!< CAN_CBT_ERPRSDIV bit offset */
#define CAN_CBT_EPRESDIV_0                   (0x001U << CAN_CBT_EPRESDIV_POS)
#define CAN_CBT_EPRESDIV_1                   (0x002U << CAN_CBT_EPRESDIV_POS)
#define CAN_CBT_EPRESDIV_2                   (0x004U << CAN_CBT_EPRESDIV_POS)
#define CAN_CBT_EPRESDIV_3                   (0x008U << CAN_CBT_EPRESDIV_POS)
#define CAN_CBT_EPRESDIV_4                   (0x010U << CAN_CBT_EPRESDIV_POS)
#define CAN_CBT_EPRESDIV_5                   (0x020U << CAN_CBT_EPRESDIV_POS)
#define CAN_CBT_EPRESDIV_6                   (0x040U << CAN_CBT_EPRESDIV_POS)
#define CAN_CBT_EPRESDIV_7                   (0x080U << CAN_CBT_EPRESDIV_POS)
#define CAN_CBT_EPRESDIV_8                   (0x100U << CAN_CBT_EPRESDIV_POS)
#define CAN_CBT_EPRESDIV_9                   (0x200U << CAN_CBT_EPRESDIV_POS)

/* CAN_CBT_BTF */
#define CAN_CBT_BTF_POS                      (31U)                                               /*!< CAN_CBT_BTF bit offset */

/* CAN_CS_TIME_STAMP */
#define CAN_CS_TIME_STAMP_POS                (0U)                                                /*!< CAN_CS_TIME_STAMP bit offset */
#define CAN_CS_TIME_STAMP_CFG(regval)        (CAN_CS_TIME_STAMP & ((uint32_t)(regval) << CAN_CS_TIME_STAMP_POS))
#define CAN_CS_TIME_STAMP_0                  (CAN_CS_TIME_STAMP_CFG(0x0001))
#define CAN_CS_TIME_STAMP_1                  (CAN_CS_TIME_STAMP_CFG(0x0002))
#define CAN_CS_TIME_STAMP_2                  (CAN_CS_TIME_STAMP_CFG(0x0004))
#define CAN_CS_TIME_STAMP_3                  (CAN_CS_TIME_STAMP_CFG(0x0008))
#define CAN_CS_TIME_STAMP_4                  (CAN_CS_TIME_STAMP_CFG(0x0010))
#define CAN_CS_TIME_STAMP_5                  (CAN_CS_TIME_STAMP_CFG(0x0020))
#define CAN_CS_TIME_STAMP_6                  (CAN_CS_TIME_STAMP_CFG(0x0040))
#define CAN_CS_TIME_STAMP_7                  (CAN_CS_TIME_STAMP_CFG(0x0080))
#define CAN_CS_TIME_STAMP_8                  (CAN_CS_TIME_STAMP_CFG(0x0100))
#define CAN_CS_TIME_STAMP_9                  (CAN_CS_TIME_STAMP_CFG(0x0200))
#define CAN_CS_TIME_STAMP_10                 (CAN_CS_TIME_STAMP_CFG(0x0400))
#define CAN_CS_TIME_STAMP_11                 (CAN_CS_TIME_STAMP_CFG(0x0800))
#define CAN_CS_TIME_STAMP_12                 (CAN_CS_TIME_STAMP_CFG(0x1000))
#define CAN_CS_TIME_STAMP_13                 (CAN_CS_TIME_STAMP_CFG(0x2000))
#define CAN_CS_TIME_STAMP_14                 (CAN_CS_TIME_STAMP_CFG(0x4000))
#define CAN_CS_TIME_STAMP_15                 (CAN_CS_TIME_STAMP_CFG(0x8000))

/* CAN_CS_DLC */
#define CAN_CS_DLC_POS                       (16U)                                               /*!< CAN_CS_DLC bit offset */
#define CAN_CS_DLC_CFG(regval)               (CAN_CS_DLC & ((uint32_t)(regval) << CAN_CS_DLC_POS))
#define CAN_CS_DLC_0                         (CAN_CS_DLC_CFG(0x1))
#define CAN_CS_DLC_1                         (CAN_CS_DLC_CFG(0x2))
#define CAN_CS_DLC_2                         (CAN_CS_DLC_CFG(0x4))
#define CAN_CS_DLC_3                         (CAN_CS_DLC_CFG(0x8))

/* CAN_CS_RTR */
#define CAN_CS_RTR_POS                       (20U)                                               /*!< CAN_CS_RTR bit offset */

/* CAN_CS_IDE */
#define CAN_CS_IDE_POS                       (21U)                                               /*!< CAN_CS_IDE bit offset */

/* CAN_CS_SRR */
#define CAN_CS_SRR_POS                       (22U)                                               /*!< CAN_CS_SRR bit offset */

/* CAN_CS_CODE */
#define CAN_CS_CODE_POS                      (24U)                                               /*!< CAN_CS_CODE bit offset */
#define CAN_CS_CODE_CFG(regval)              (CAN_CS_CODE & ((uint32_t)(regval) << CAN_CS_CODE_POS))
#define CAN_CS_CODE_0                        (CAN_CS_CODE_CFG(0x1))
#define CAN_CS_CODE_1                        (CAN_CS_CODE_CFG(0x2))
#define CAN_CS_CODE_2                        (CAN_CS_CODE_CFG(0x4))
#define CAN_CS_CODE_3                        (CAN_CS_CODE_CFG(0x8))

/* CAN_CS_ESI */
#define CAN_CS_ESI_POS                       (29U)                                               /*!< CAN_CS_ESI bit offset */

/* CAN_CS_BRS */
#define CAN_CS_BRS_POS                       (30U)                                               /*!< CAN_CS_BRS bit offset */

/* CAN_CS_EDL */
#define CAN_CS_EDL_POS                       (31U)                                               /*!< CAN_CS_EDL bit offset */

/* CAN_ID_EXT */
#define CAN_ID_EXT_POS                       (0U)                                                /*!< CAN_ID_EXT bit offset */
#define CAN_ID_EXT_CFG(regval)               (CAN_ID_EXT & ((uint32_t)(regval) << CAN_ID_EXT_POS))
#define CAN_ID_EXT_0                         (CAN_ID_EXT_CFG(0x00001))
#define CAN_ID_EXT_1                         (CAN_ID_EXT_CFG(0x00002))
#define CAN_ID_EXT_2                         (CAN_ID_EXT_CFG(0x00004))
#define CAN_ID_EXT_3                         (CAN_ID_EXT_CFG(0x00008))
#define CAN_ID_EXT_4                         (CAN_ID_EXT_CFG(0x00010))
#define CAN_ID_EXT_5                         (CAN_ID_EXT_CFG(0x00020))
#define CAN_ID_EXT_6                         (CAN_ID_EXT_CFG(0x00040))
#define CAN_ID_EXT_7                         (CAN_ID_EXT_CFG(0x00080))
#define CAN_ID_EXT_8                         (CAN_ID_EXT_CFG(0x00100))
#define CAN_ID_EXT_9                         (CAN_ID_EXT_CFG(0x00200))
#define CAN_ID_EXT_10                        (CAN_ID_EXT_CFG(0x00400))
#define CAN_ID_EXT_11                        (CAN_ID_EXT_CFG(0x00800))
#define CAN_ID_EXT_12                        (CAN_ID_EXT_CFG(0x01000))
#define CAN_ID_EXT_13                        (CAN_ID_EXT_CFG(0x02000))
#define CAN_ID_EXT_14                        (CAN_ID_EXT_CFG(0x04000))
#define CAN_ID_EXT_15                        (CAN_ID_EXT_CFG(0x08000))
#define CAN_ID_EXT_16                        (CAN_ID_EXT_CFG(0x10000))
#define CAN_ID_EXT_17                        (CAN_ID_EXT_CFG(0x20000))

/* CAN_ID_STD */
#define CAN_ID_STD_POS                       (18U)                                               /*!< CAN_ID_STD bit offset */
#define CAN_ID_STD_CFG(regval)               (CAN_ID_STD & ((uint32_t)(regval) << CAN_ID_STD_POS))
#define CAN_ID_STD_0                         (CAN_ID_STD_CFG(0x001))
#define CAN_ID_STD_1                         (CAN_ID_STD_CFG(0x002))
#define CAN_ID_STD_2                         (CAN_ID_STD_CFG(0x004))
#define CAN_ID_STD_3                         (CAN_ID_STD_CFG(0x008))
#define CAN_ID_STD_4                         (CAN_ID_STD_CFG(0x010))
#define CAN_ID_STD_5                         (CAN_ID_STD_CFG(0x020))
#define CAN_ID_STD_6                         (CAN_ID_STD_CFG(0x040))
#define CAN_ID_STD_7                         (CAN_ID_STD_CFG(0x080))
#define CAN_ID_STD_8                         (CAN_ID_STD_CFG(0x100))
#define CAN_ID_STD_9                         (CAN_ID_STD_CFG(0x200))
#define CAN_ID_STD_10                        (CAN_ID_STD_CFG(0x400))

/* CAN_ID_PRIO */
#define CAN_ID_PRIO_POS                      (29U)                                               /*!< CAN_ID_PRIO bit offset */
#define CAN_ID_PRIO_0                        ((uint32_t)0x1U << CAN_ID_PRIO_POS)
#define CAN_ID_PRIO_1                        ((uint32_t)0x2U << CAN_ID_PRIO_POS)
#define CAN_ID_PRIO_2                        ((uint32_t)0x4U << CAN_ID_PRIO_POS)

/* CAN_WORD0_DATA */
#define CAN_WORD0_DATA_POS                   (0U)                                                /*!< CAN_WORD0_DATA bit offset */

/* CAN_WORD1_DATA */
#define CAN_WORD1_DATA_POS                   (0U)                                                /*!< CAN_WORD1_DATA bit offset */

/* CAN_RXIMR_MI */
#define CAN_RXIMR_MI_POS                     (0U)                                                /*!< CAN_RXIMR_MI bit offset */
#define CAN_RXIMR_MI_0                       (0x00000001U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_1                       (0x00000002U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_2                       (0x00000004U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_3                       (0x00000008U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_4                       (0x00000010U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_5                       (0x00000020U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_6                       (0x00000040U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_7                       (0x00000080U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_8                       (0x00000100U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_9                       (0x00000200U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_10                      (0x00000400U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_11                      (0x00000800U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_12                      (0x00001000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_13                      (0x00002000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_14                      (0x00004000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_15                      (0x00008000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_16                      (0x00010000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_17                      (0x00020000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_18                      (0x00040000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_19                      (0x00080000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_20                      (0x00100000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_21                      (0x00200000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_22                      (0x00400000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_23                      (0x00800000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_24                      (0x01000000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_25                      (0x02000000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_26                      (0x04000000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_27                      (0x08000000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_28                      (0x10000000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_29                      (0x20000000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_30                      (0x40000000U << CAN_RXIMR_MI_POS)
#define CAN_RXIMR_MI_31                      (0x80000000U << CAN_RXIMR_MI_POS)

/* CAN_EPRS_ENPRESDIV */
#define CAN_EPRS_ENPRESDIV_POS               (0U)                                                /*!< CAN_EPRS_ENPRESDIV bit offset */

/* CAN_ECBT_NTSEG1 */
#define CAN_ECBT_NTSEG1_POS                  (0U)                                                /*!< CAN_ECBT_NTSEG1 bit offset */

/* CAN_ECBT_NTSEG2 */
#define CAN_ECBT_NTSEG2_POS                  (12U)                                               /*!< CAN_ECBT_NTSEG2 bit offset */

/* CAN_ECBT_NRJW */
#define CAN_ECBT_NRJW_POS                    (22U)                                               /*!< CAN_ECBT_NRJW bit offset */

/* CAN_DMA_RBADR_ADDR */
#define CAN_DMA_RBADR_ADDR_POS               (0U)                                                /*!< CAN_DMA_RBADR_ADDR bit offset */

/* CAN_DMA_RLEN_LEN */
#define CAN_DMA_RLEN_LEN_POS                 (0U)                                                /*!< CAN_DMA_RLEN_LEN bit offset */

/* CAN_DMA_RADR_ADDR */
#define CAN_DMA_RADR_ADDR_POS                (0U)                                                /*!< CAN_DMA_RADR_ADDR bit offset */

/* CAN_DMA_IE_RXHFULL */
#define CAN_DMA_IE_RXHFULL_POS               (0U)                                                /*!< CAN_DMA_IE_RXHFULL bit offset */

/* CAN_DMA_IE_RXFULL */
#define CAN_DMA_IE_RXFULL_POS                (1U)                                                /*!< CAN_DMA_IE_RXFULL bit offset */

/* CAN_DMA_IF_RXHFULL */
#define CAN_DMA_IF_RXHFULL_POS               (0U)                                                /*!< CAN_DMA_IF_RXHFULL bit offset */

/* CAN_DMA_IF_RXFULL */
#define CAN_DMA_IF_RXFULL_POS                (1U)                                                /*!< CAN_DMA_IF_RXFULL bit offset */

/* CAN_TMR_SEL_FILTER */
#define CAN_TMR_SEL_FILTER_POS               (0U)                                                /*!< CAN_TMR_SEL_FILTER bit offset */

/* CAN_STOP_MD_PWD */
#define CAN_STOP_MD_PWD_POS                  (0U)                                                /*!< CAN_STOP_MD_PWD bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_CAN_H */
