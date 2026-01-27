/**
 ******************************************************************************
 * @file   rn_rv_hal_can.h
 * @author Renergy Software Team
 * @brief  Header file of CAN HAL module.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RN_RV_HAL_CAN_H
#define __RN_RV_HAL_CAN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal_def.h"
#include "rn_rv_ll_can.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup CAN
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup CAN_Exported_Types CAN Exported Types
 * @{
 */

/**
 * @brief CAN frame format.
 */
typedef enum __attribute__((__packed__))
{
    CAN_FRAME_FORMAT_STANDARD       = 0x0U, /*!< Standard frame format attribute. */
    CAN_FRAME_FORMAT_EXTEND         = 0x1U  /*!< Extend frame format attribute. */
} CAN_FrameFormatTypeDef;

/**
 * @brief CAN frame type.
 */
typedef enum __attribute__((__packed__))
{
    CAN_FRAME_TYPE_DATA             = 0x0U, /*!< Data frame type attribute. */
    CAN_FRAME_TYPE_REMOTE           = 0x1U  /*!< Remote frame type attribute. */
} CAN_FrameAttributeTypeDef;

/**
 * @brief CAN clock source.
 */
typedef enum __attribute__((__packed__))
{
    CAN_CLOCK_SOURCE_PLL_OSC_ASYNC  = 0x0U, /*!< CAN Protocol Engine clock from pll or Oscillator. */
    CAN_CLOCK_SOURCE_RCH_ASYNC      = 0x1U, /*!< CAN Protocol Engine clock from rch. */
    CAN_CLOCK_SOURCE_PLL_OSC_SYNC   = 0x2U, /*!< CAN Protocol Engine clock from pll or Oscillator. */
    CAN_CLOCK_SOURCE_RCH_SYNC       = 0x3U  /*!< CAN Protocol Engine clock from rch. */
} CAN_ClockSourceTypeDef;

/**
 * @brief CAN wake up source.
 */
typedef enum __attribute__((__packed__))
{
    CAN_WAKEUP_SOURCE_UNFILTERED    = 0x0U, /*!< CAN uses unfiltered Rx input to detect edge. */
    CAN_WAKEUP_SOURCE_FILTERED      = 0x1U  /*!< CAN uses filtered Rx input to detect edge. */
} CAN_WakeupSourceTypeDef;

/**
 * @brief CAN Rx Fifo Filter type.
 */
typedef enum __attribute__((__packed__))
{
    CAN_RXFIFO_FILTER_TYPEA         = 0x0U, /*!< One full ID (standard and extended) per ID Filter element. */
    CAN_RXFIFO_FILTER_TYPEB         = 0x1U, /*!< Two full standard IDs or two partial 14-bit ID slices per ID Filter Table element. */
    CAN_RXFIFO_FILTER_TYPEC         = 0x2U, /*!< Four partial 8-bit Standard or extended ID slices per ID Filter Table element. */
    CAN_RXFIFO_FILTER_TYPED         = 0x3U  /*!< All frames rejected. */
} CAN_RxFifoFilterTypeDef;

/**
 * @brief  CAN Rx FIFO priority.
 * The matching process starts from the Rx MB(or Rx FIFO) with higher priority.
 * If no MB(or Rx FIFO filter) is satisfied, the matching process goes on with
 * the Rx FIFO(or Rx MB) with lower priority.
 */

typedef enum __attribute__((__packed__))
{
    CAN_RXFIFO_PRIORITY_LOW         = 0x0U, /*!< Matching process start from Rx Message Buffer first */
    CAN_RXFIFO_PRIORITY_HIGH        = 0x1U  /*!< Matching process start from Rx FIFO first */
} CAN_RxFifoPriorityTypeDef;

/**
 * @brief CAN transmit mode.
 */
typedef enum __attribute__((__packed__))
{
    CAN_TRANSMIT_MODE_NORMAL        = 0x0U,  /*!< Normal format attribute. */
    CAN_TRANSMIT_MODE_LISTENONLY    = 0x1U,  /*!< Rx Active with ack from ohter device. */
    CAN_TRANSMIT_MODE_LOOPBACK      = 0x2U,  /*!< TX Active with datas LoopBack to RX. */
    CAN_TRANSMIT_MODE_LOOPBACKONLY  = 0x3U   /*!< LoopBack Inside Only. */
} CAN_TransmitModeTypeDef;

/**
 * @brief CAN message frame structure.
 */
typedef struct _can_frame
{
    struct
    {
        uint32_t timestamp : 16;       /*!< CAN internal Free-Running Counter Time Stamp. */
        uint32_t length    : 4;        /*!< CAN frame payload length in bytes(Range: 0~8). */
        uint32_t type      : 1;        /*!< CAN Frame Type(DATA or REMOTE). */
        uint32_t format    : 1;        /*!< CAN Frame Identifier(STD or EXT format). */
        uint32_t           : 1;        /*!< Reserved. */
        uint32_t idhit     : 9;        /*!< CAN Rx FIFO filter hit id(This value is only used in Rx FIFO receive mode). */
    };
    struct
    {
        uint32_t id        : 29;       /*!< CAN Frame Identifier, should be set using CAN_ID_EXT_FORMAT() or CAN_ID_STD_FORMAT() macro. */
        uint32_t pri       : 3;        /*!< Reserved. */
    };
    struct
    {
        uint32_t dataWord0;            /*!< CAN Frame payload word0. bit-endian, bit[31:0] = {byte0, byte1, byte2, byte3} */
        uint32_t dataWord1;            /*!< CAN Frame payload word1. bit-endian, bit[31:0] = {byte4, byte5, byte6, byte7}*/
    };
} CAN_FrameTypeDef;

/**
 * @brief CAN protocol timing characteristic configuration structure.
 */
typedef struct _can_timing_config
{
    uint16_t preDivider;               /*!< Clock Pre-scaler Division Factor. */
    uint8_t  propSeg;                  /*!< Propagation Segment. */
    uint8_t  phaseSeg1;                /*!< Phase Segment 1. */
    uint8_t  phaseSeg2;                /*!< Phase Segment 2. */
    uint8_t  reSyncJumpwidth;          /*!< Re-sync Jump Width. */
} CAN_TimingConfigTypeDef;

/**
 * @brief CAN module configuration structure.
 */
typedef struct _can_config
{
    CAN_ClockSourceTypeDef  clkSrc;               /*!< Clock source for CAN Protocol Engine. */
    CAN_WakeupSourceTypeDef wakeupSrc;            /*!< Wake up source selection. */
    uint8_t                 maxMbNum;             /*!< The maximum number of Message Buffers used by user. */
    uint8_t                 enableLoopBack;       /*!< Enable or Disable Loop Back Self Test Mode. */
    uint8_t                 enableTimerSync;      /*!< Enable or Disable Timer Synchronization. */
    uint8_t                 enableSelfWakeup;     /*!< Enable or Disable Self Wakeup Mode. */
    uint8_t                 enableIndividMask;    /*!< Enable or Disable Rx Individual Mask. */
    uint8_t                 disableSelfReception; /*!< Enable or Disable Self Reflection. */
    uint8_t                 enableListenOnlyMode; /*!< Enable or Disable Listen Only Mode. */
    CAN_TimingConfigTypeDef timingConfig;         /*!< Protocol timing . */
} CAN_ConfigTypeDef;

/**
 * @brief  CAN Receive Message Buffer configuration structure
 *
 * This structure is used as the parameter of  HAL_CAN_ConfigRxMb() function.
 * The  HAL_CAN_ConfigRxMb() function is used to configure CAN Receive
 * Message Buffer. The function abort previous receiving process, clean the
 * Message Buffer and activate the Rx Message Buffer using given Message Buffer
 * setting.
 */
typedef struct _can_rx_mb_config
{
    uint32_t                  id;     /*!< CAN Message Buffer Frame Identifier, should be set using CAN_ID_EXT_FORMAT() or CAN_ID_STD_FORMAT() macro. */
    CAN_FrameFormatTypeDef    format; /*!< CAN Frame Identifier format(Standard of Extend). */
    CAN_FrameAttributeTypeDef type;   /*!< CAN Frame Type(Data or Remote). */
} CAN_RxMbConfigTypeDef;

/**
 * @brief CAN Rx FIFO configuration structure.
 */
typedef struct _can_rx_fifo_config
{
    uint32_t                 *idFilterTable; /*!< Pointer to the CAN Rx FIFO identifier filter table. */
    uint8_t                   idFilterNum;   /*!< The quantity of filter elements. */
    CAN_RxFifoFilterTypeDef   idFilterType;  /*!< The CAN Rx FIFO Filter type. */
    CAN_RxFifoPriorityTypeDef priority;      /*!< The CAN Rx FIFO receive priority. */
} CAN_RxFifoConfigTypeDef;

/**
 * @brief CAN Message Buffer transfer.
 */
typedef struct _can_mb_transfer
{
    CAN_FrameTypeDef *frame; /*!< The buffer of CAN Message to be transfer. */
    uint8_t           MbIdx; /*!< The index of Message buffer used to transfer Message. */
} CAN_MbTransferTypeDef;

/**
 * @brief CAN Rx FIFO transfer.
 */
typedef struct _can_fifo_transfer
{
    CAN_FrameTypeDef *frame; /*!< The buffer of CAN Message to be received from Rx FIFO. */
} CAN_RxFifoTransferTypeDef;

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup CAN_Exported_Constants CAN Exported Constants
 * @{
 */

/**
 * @brief Message buffer size
 */
#define CAN_HAS_MB_MAX_NUM              (16U)

/* The count of CAN_WORD1 */
#define CAN_WORD1_COUNT                 (16U)
/* The count of CAN_WORD0 */
#define CAN_WORD0_COUNT                 (16U)
/* The count of CAN_CS */
#define CAN_CS_COUNT                    (16U)

/**
 * @brief CAN Frame ID helper macro.
 */
#define CAN_ID_STD_FORMAT(id) \
        (((uint32_t)(((uint32_t)(id)) << CAN_ID_STD_POS)) & CAN_ID_STD) /*!< Standard Frame ID helper macro. */
#define CAN_ID_EXT_FORMAT(id) \
        (((uint32_t)(((uint32_t)(id)) << CAN_ID_EXT_POS)) & (CAN_ID_EXT | CAN_ID_STD)) /*!< Extend Frame ID helper macro. */

/**
 * @brief CAN Rx Message Buffer Mask helper macro.
 */
#define CAN_RX_MB_STD_MASK(id, rtr, ide) \
        (((uint32_t)((uint32_t)(rtr) << 31) | (uint32_t)((uint32_t)(ide) << 30)) | \
         CAN_ID_STD_FORMAT(id))           /*!< Standard Rx Message Buffer Mask helper macro. */
#define CAN_RX_MB_EXT_MASK(id, rtr, ide) \
        (((uint32_t)((uint32_t)(rtr) << 31) | (uint32_t)((uint32_t)(ide) << 30)) | \
         CAN_ID_EXT_FORMAT(id))           /*!< Extend Rx Message Buffer Mask helper macro. */

/**
 * @brief CAN Rx FIFO Mask helper macro.
 */
#define CAN_RX_FIFO_STD_MASK_TYPE_A(id, rtr, ide) \
        (((uint32_t)((uint32_t)(rtr) << 31) | (uint32_t)((uint32_t)(ide) << 30)) | \
        (CAN_ID_STD_FORMAT(id) << 1))                 /*!< Standard Rx FIFO Mask helper macro Type A helper macro. */
#define CAN_RX_FIFO_STD_MASK_TYPE_B_HIGH(id, rtr, ide) \
        (((uint32_t)((uint32_t)(rtr) << 31) | (uint32_t)((uint32_t)(ide) << 30)) | \
        (((uint32_t)(id) & 0x7FF) << 19))             /*!< Standard Rx FIFO Mask helper macro Type B upper part helper macro. */
#define CAN_RX_FIFO_STD_MASK_TYPE_B_LOW(id, rtr, ide) \
        (((uint32_t)((uint32_t)(rtr) << 15) | (uint32_t)((uint32_t)(ide) << 14)) | \
        (((uint32_t)(id) & 0x7FF) << 3))              /*!< Standard Rx FIFO Mask helper macro Type B lower part helper macro. */
#define CAN_RX_FIFO_STD_MASK_TYPE_C_HIGH(id) \
        (((uint32_t)(id) & 0x7F8) << 21)              /*!< Standard Rx FIFO Mask helper macro Type C upper part helper macro. */
#define CAN_RX_FIFO_STD_MASK_TYPE_C_MID_HIGH(id) \
        (((uint32_t)(id) & 0x7F8) << 13)              /*!< Standard Rx FIFO Mask helper macro Type C mid-upper part helper macro. */
#define CAN_RX_FIFO_STD_MASK_TYPE_C_MID_LOW(id) \
        (((uint32_t)(id) & 0x7F8) << 5)               /*!< Standard Rx FIFO Mask helper macro Type C mid-lower part helper macro. */
#define CAN_RX_FIFO_STD_MASK_TYPE_C_LOW(id) \
        (((uint32_t)(id) & 0x7F8) >> 3)               /*!< Standard Rx FIFO Mask helper macro Type C lower part helper macro. */
#define CAN_RX_FIFO_EXT_MASK_TYPE_A(id, rtr, ide) \
        (((uint32_t)((uint32_t)(rtr) << 31) | (uint32_t)((uint32_t)(ide) << 30)) | \
        (CAN_ID_EXT_FORMAT(id) << 1))                 /*!< Extend Rx FIFO Mask helper macro Type A helper macro. */
#define CAN_RX_FIFO_EXT_MASK_TYPE_B_HIGH(id, rtr, ide) \
        (((uint32_t)((uint32_t)(rtr) << 31) | (uint32_t)((uint32_t)(ide) << 30)) | \
        ((CAN_ID_EXT_FORMAT(id) & 0x1FFF8000) << 1))  /*!< Extend Rx FIFO Mask helper macro Type B upper part helper macro. */
#define CAN_RX_FIFO_EXT_MASK_TYPE_B_LOW(id, rtr, ide) \
        (((uint32_t)((uint32_t)(rtr) << 15) | (uint32_t)((uint32_t)(ide) << 14)) | \
        ((CAN_ID_EXT_FORMAT(id) & 0x1FFF8000) >> 15)) /*!< Extend Rx FIFO Mask helper macro Type B lower part helper macro. */
#define CAN_RX_FIFO_EXT_MASK_TYPE_C_HIGH(id) \
        ((CAN_ID_EXT_FORMAT(id) & 0x1FE00000) << 3)   /*!< Extend Rx FIFO Mask helper macro Type C upper part helper macro. */
#define CAN_RX_FIFO_EXT_MASK_TYPE_C_MID_HIGH(id) \
        ((CAN_ID_EXT_FORMAT(id) & 0x1FE00000) >> 5)   /*!< Extend Rx FIFO Mask helper macro Type C mid-upper part helper macro. */
#define CAN_RX_FIFO_EXT_MASK_TYPE_C_MID_LOW(id) \
        ((CAN_ID_EXT_FORMAT(id) & 0x1FE00000) >> 13)  /*!< Extend Rx FIFO Mask helper macro Type C mid-lower part helper macro. */
#define CAN_RX_FIFO_EXT_MASK_TYPE_C_LOW(id) \
        ((CAN_ID_EXT_FORMAT(id) & 0x1FE00000) >> 21)  /*!< Extend Rx FIFO Mask helper macro Type C lower part helper macro. */

/**
 * @brief CAN Rx FIFO Filter helper macro.
 */
#define CAN_RX_FIFO_STD_FILTER_TYPE_A(id, rtr, ide) \
        CAN_RX_FIFO_STD_MASK_TYPE_A(id, rtr, ide)           /*!< Standard Rx FIFO Filter helper macro Type A helper macro. */
#define CAN_RX_FIFO_STD_FILTER_TYPE_B_HIGH(id, rtr, ide) \
        CAN_RX_FIFO_STD_MASK_TYPE_B_HIGH(id, rtr, ide)      /*!< Standard Rx FIFO Filter helper macro Type B upper part helper macro. */
#define CAN_RX_FIFO_STD_FILTER_TYPE_B_LOW(id, rtr, ide) \
        CAN_RX_FIFO_STD_MASK_TYPE_B_LOW(id, rtr, ide)       /*!< Standard Rx FIFO Filter helper macro Type B lower part helper macro. */
#define CAN_RX_FIFO_STD_FILTER_TYPE_C_HIGH(id) \
        CAN_RX_FIFO_STD_MASK_TYPE_C_HIGH(id)                /*!< Standard Rx FIFO Filter helper macro Type C upper part helper macro. */
#define CAN_RX_FIFO_STD_FILTER_TYPE_C_MID_HIGH(id) \
        CAN_RX_FIFO_STD_MASK_TYPE_C_MID_HIGH(id)            /*!< Standard Rx FIFO Filter helper macro Type C mid-upper part helper macro. */
#define CAN_RX_FIFO_STD_FILTER_TYPE_C_MID_LOW(id) \
        CAN_RX_FIFO_STD_MASK_TYPE_C_MID_LOW(id)             /*!< Standard Rx FIFO Filter helper macro Type C mid-lower part helper macro. */
#define CAN_RX_FIFO_STD_FILTER_TYPE_C_LOW(id) \
        CAN_RX_FIFO_STD_MASK_TYPE_C_LOW(id)                 /*!< Standard Rx FIFO Filter helper macro Type C lower part helper macro. */
#define CAN_RX_FIFO_EXT_FILTER_TYPE_A(id, rtr, ide) \
        CAN_RX_FIFO_EXT_MASK_TYPE_A(id, rtr, ide)           /*!< Extend Rx FIFO Filter helper macro Type A helper macro. */
#define CAN_RX_FIFO_EXT_FILTER_TYPE_B_HIGH(id, rtr, ide) \
        CAN_RX_FIFO_EXT_MASK_TYPE_B_HIGH(id, rtr, ide)      /*!< Extend Rx FIFO Filter helper macro Type B upper part helper macro. */
#define CAN_RX_FIFO_EXT_FILTER_TYPE_B_LOW(id, rtr, ide) \
        CAN_RX_FIFO_EXT_MASK_TYPE_B_LOW(id, rtr, ide)       /*!< Extend Rx FIFO Filter helper macro Type B lower part helper macro. */
#define CAN_RX_FIFO_EXT_FILTER_TYPE_C_HIGH(id) \
        CAN_RX_FIFO_EXT_MASK_TYPE_C_HIGH(id)                /*!< Extend Rx FIFO Filter helper macro Type C upper part helper macro. */
#define CAN_RX_FIFO_EXT_FILTER_TYPE_C_MID_HIGH(id) \
        CAN_RX_FIFO_EXT_MASK_TYPE_C_MID_HIGH(id)            /*!< Extend Rx FIFO Filter helper macro Type C mid-upper part helper macro. */
#define CAN_RX_FIFO_EXT_FILTER_TYPE_C_MID_LOW(id) \
        CAN_RX_FIFO_EXT_MASK_TYPE_C_MID_LOW(id)             /*!< Extend Rx FIFO Filter helper macro Type C mid-lower part helper macro. */
#define CAN_RX_FIFO_EXT_FILTER_TYPE_C_LOW(id) \
        CAN_RX_FIFO_EXT_MASK_TYPE_C_LOW(id)                 /*!< Extend Rx FIFO Filter helper macro Type C lower part helper macro. */

/**
 * @brief CAN transfer status.
 */
#define CAN_STATUS_TXBUSY               0U   /*!< Tx Message Buffer is Busy. */
#define CAN_STATUS_TXIDLE               1U   /*!< Tx Message Buffer is Idle. */
#define CAN_STATUS_TXSWITCHTORX         2U   /*!< Remote Message is send out and Message buffer changed to Receive one. */
#define CAN_STATUS_RXBUSY               3U   /*!< Rx Message Buffer is Busy. */
#define CAN_STATUS_RXIDLE               4U   /*!< Rx Message Buffer is Idle. */
#define CAN_STATUS_RXOVERFLOW           5U   /*!< Rx Message Buffer is Overflowed. */
#define CAN_STATUS_RXFIFOBUSY           6U   /*!< Rx Message FIFO is Busy. */
#define CAN_STATUS_RXFIFOIDLE           7U   /*!< Rx Message FIFO is Idle. */
#define CAN_STATUS_RXFIFOOVERFLOW       8U   /*!< Rx Message FIFO is overflowed. */
#define CAN_STATUS_RXFIFOWARNING        9U   /*!< Rx Message FIFO is almost overflowed. */
#define CAN_STATUS_ERRORSTATUS          10U  /*!< CAN Module Error and Status. */
#define CAN_STATUS_WAKEUP               11U  /*!< CAN is waken up from STOP mode. */
#define CAN_STATUS_UNHANDLED            12U  /*!< UnHadled Interrupt asserted. */
#define CAN_STATUS_RXREMOTE             13U  /*!< Rx Remote Message Received in Mail box. */

#define CAN_STATUS_SUCCESS              0U   /*!< Generic status for Success. */
#define CAN_STATUS_FAIL                 1U   /*!< Generic status for Fail. */
#define CAN_STATUS_READONLY             2U   /*!< Generic status for read only failure. */
#define CAN_STATUS_OUTOFRANGE           3U   /*!< Generic status for out of range access. */
#define CAN_STATUS_INVALIDARGUMENT      4U   /*!< Generic status for invalid argument check. */
#define CAN_STATUS_TIMEOUT              5U   /*!< Generic status for timeout. */
#define CAN_STATUS_NOTRANSFERINPROGRESS 6U   /*!< Generic status for no transfer in progress. */

#define CAN_STATE_IDLE                  0x0U /*!< MB/RxFIFO idle. */
#define CAN_STATE_RXDATA                0x1U /*!< MB receiving. */
#define CAN_STATE_RXREMOTE              0x2U /*!< MB receiving remote reply. */
#define CAN_STATE_TXDATA                0x3U /*!< MB transmitting. */
#define CAN_STATE_TXREMOTE              0x4U /*!< MB transmitting remote request. */
#define CAN_STATE_RXFIFO                0x5U /*!< RxFIFO receiving. */

/**
 * @brief CAN message buffer CODE for Rx buffers.
 */
#define CAN_RXMBINACTIVE                0x0U /*!< MB is not active. */
#define CAN_RXMBFULL                    0x2U /*!< MB is full. */
#define CAN_RXMBEMPTY                   0x4U /*!< MB is active and empty. */
#define CAN_RXMBOVERRUN                 0x6U /*!< MB is overwritten into a full buffer. */
#define CAN_RXMBBUSY                    0x8U /*!< CAN is updating the contents of the MB. The CPU must not access the MB. */
#define CAN_RXMBRANSWER                 0xAU /*!< A frame was configured to recognize a Remote Request Frame and transmit a Response Frame in return. */
#define CAN_RXMBNOTUSED                 0xFU /*!< Not used. */

/**
 * @brief CAN message buffer CODE FOR Tx buffers.
 */
#define CAN_TXMBINACTIVE                0x8U /*!< MB is not active. */
#define CAN_TXMBABORT                   0x9U /*!< MB is aborted. */
#define CAN_TXMBDATAORREMOTE            0xCU /*!< MB is a TX Data Frame(when MB RTR = 0) or MB is a TX Remote Request Frame (when MB RTR = 1). */
#define CAN_TXMBTANSWER                 0xEU /*!< MB is a TX Response Request Frame from an incoming Remote Request Frame. */
#define CAN_TXMBNOTUSED                 0xFU /*!< Not used. */

/**
 * @brief CAN interrupt configuration structure, default settings all disabled.
 * This structure contains the settings for all of the CAN Module interrupt configurations.
 * Note: CAN Message Buffers and Rx FIFO have their own interrupts.
 */
#define CAN_IT_BUSOFF                   CAN_CTRL1_BOFFMSK  /*!< Bus Off interrupt. */
#define CAN_IT_ERROR                    CAN_CTRL1_ERRMSK   /*!< Error interrupt. */
#define CAN_IT_RXWARNING                CAN_CTRL1_RWRNMSK  /*!< Rx Warning interrupt. */
#define CAN_IT_TXWARNING                CAN_CTRL1_TWRNMSK  /*!< Tx Warning interrupt. */
#define CAN_IT_WAKEUP                   CAN_MCR_WAKMSK     /*!< Wake Up interrupt. */

/**
 * @brief CAN Flags Definition.
 * This provides constants for the CAN status flags for use in the CAN functions.
 */
#define CAN_FLAG_SYNCH                  CAN_ESR1_SYNCH     /*!< CAN Synchronization Status. */
#define CAN_FLAG_TXWARNINGINT           CAN_ESR1_TWRNINT   /*!< Tx Warning Interrupt Flag. */
#define CAN_FLAG_RXWARNINGINT           CAN_ESR1_RWRNINT   /*!< Rx Warning Interrupt Flag. */
#define CAN_FLAG_TXERRORWARNING         CAN_ESR1_TXWRN     /*!< Tx Error Warning Status. */
#define CAN_FLAG_RXERRORWARNING         CAN_ESR1_RXWRN     /*!< Rx Error Warning Status. */
#define CAN_FLAG_IDLE                   CAN_ESR1_IDLE      /*!< CAN IDLE Status Flag. */
#define CAN_FLAG_FAULTCONFINEMENT       CAN_ESR1_FLTCONF   /*!< Fault Confinement State Flag. */
#define CAN_FLAG_TRANSMITTING           CAN_ESR1_TX        /*!< CAN In Transmission Status. */
#define CAN_FLAG_RECEIVING              CAN_ESR1_RX        /*!< CAN In Reception Status. */
#define CAN_FLAG_BUSOFFINT              CAN_ESR1_BOFFINT   /*!< Bus Off Interrupt Flag. */
#define CAN_FLAG_ERRORINT               CAN_ESR1_ERRINT    /*!< Error Interrupt Flag. */
#define CAN_FLAG_WAKEUPINT              CAN_ESR1_WAKINT    /*!< Wake-Up Interrupt Flag. */

#define CAN_ERROR_STUFFING              CAN_ESR1_STFERR    /*!< Stuffing Error. */
#define CAN_ERROR_FORM                  CAN_ESR1_FRMERR    /*!< Form Error. */
#define CAN_ERROR_CRC                   CAN_ESR1_CRCERR    /*!< Cyclic Redundancy Check Error. */
#define CAN_ERROR_ACK                   CAN_ESR1_ACKERR    /*!< Received no ACK on transmission. */
#define CAN_ERROR_BIT0                  CAN_ESR1_BIT0ERR   /*!< Unable to send dominant bit. */
#define CAN_ERROR_BIT1                  CAN_ESR1_BIT1ERR   /*!< Unable to send recessive bit. */

/**
 * @brief  CAN Rx FIFO status flags.
 *
 * The CAN Rx FIFO Status enumerations are used to determine the status of the
 * Rx FIFO. Because Rx FIFO occupy the MB0 ~ MB7 (Rx Fifo filter also occupies
 * more Message Buffer space), Rx FIFO status flags are mapped to the corresponding
 * Message Buffer status flags.
 */
#define CAN_RXFIFOOVERFLOWFLAG          CAN_IFLAG1_BUF7I   /*!< Rx FIFO overflow flag. */
#define CAN_RXFIFOWARNINGFLAG           CAN_IFLAG1_BUF6I   /*!< Rx FIFO almost full flag. */
#define CAN_RXFIFOFRAMEAVLFLAG          CAN_IFLAG1_BUF5I   /*!< Frames available in Rx FIFO flag. */

/**
 * @}
 */
/* Exported macros ----------------------------------------------------------*/
/** @defgroup CAN_Exported_Macros CAN Exported Macros
 * @{
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup CAN_Exported_Functions CAN Exported Functions
 * @{
 */

/** @addtogroup CAN_Exported_Functions_Group1
 * @{
 */
/* Initialization and Configuration functions ********************************/
void        HAL_CAN_Init(CAN_TypeDef *can, const CAN_ConfigTypeDef *pConfig);
void        HAL_CAN_DeInit(CAN_TypeDef *can);
/**
 * @}
 */

/** @addtogroup CAN_Exported_Functions_Group2
 * @{
 */
/* IO Operation functions ****************************************************/
void        HAL_CAN_Start(CAN_TypeDef *can);
void        HAL_CAN_Stop(CAN_TypeDef *can);
void        HAL_CAN_EnterFreezeMode(CAN_TypeDef *can);
void        HAL_CAN_ExitFreezeMode(CAN_TypeDef *can);
uint32_t    HAL_CAN_AddTxMessage(CAN_TypeDef *can, uint8_t MbIdx, const CAN_FrameTypeDef *pTxFrame);
uint32_t    HAL_CAN_GetRxMessage(CAN_TypeDef *can, uint8_t MbIdx, CAN_FrameTypeDef *pRxFrame);
uint32_t    HAL_CAN_GetRxFifoMessage(CAN_TypeDef *can, CAN_FrameTypeDef *pRxFrame);
uint32_t    HAL_CAN_TransferSendBlocking(CAN_TypeDef *can, uint8_t MbIdx, CAN_FrameTypeDef *pTxFrame);
uint32_t    HAL_CAN_TransferReceiveBlocking(CAN_TypeDef *can, uint8_t MbIdx, CAN_FrameTypeDef *pRxFrame);
uint32_t    HAL_CAN_TransferReceiveFifoBlocking(CAN_TypeDef *can, CAN_FrameTypeDef *pRxFrame);
void        HAL_CAN_EnableRxFifoDMA(CAN_TypeDef *can);
void        HAL_CAN_DisableRxFifoDMA(CAN_TypeDef *can);
/**
 * @}
 */

/** @addtogroup CAN_Exported_Functions_Group3
 * @{
 */
/* Interrupt functions *******************************************************/
FlagStatus  HAL_CAN_GetState(CAN_TypeDef *can, uint32_t Flag);
FlagStatus  HAL_CAN_GetDMAState(CAN_TypeDef *can, uint32_t DMAFlag);
FlagStatus  HAL_CAN_GetMbState(CAN_TypeDef *can, uint32_t MbFlag);
void        HAL_CAN_ClearFlag(CAN_TypeDef *can, uint32_t Flag);
void        HAL_CAN_ClearDMAFlag(CAN_TypeDef *can, uint32_t DMAFlag);
void        HAL_CAN_ClearMbFlag(CAN_TypeDef *can, uint32_t MbFlag);
void        HAL_CAN_ConfigInterrupt(CAN_TypeDef *can, uint32_t ITs, FunctionalState State);
void        HAL_CAN_ConfigDMAInterrupt(CAN_TypeDef *can, uint32_t DMAITs, FunctionalState State);
void        HAL_CAN_ConfigMbInterrupt(CAN_TypeDef *can, uint32_t MbITs, FunctionalState State);
/**
 * @}
 */

/** @addtogroup CAN_Exported_Functions_Group4
 * @{
 */
/* Configuration functions ***************************************************/
ErrorStatus HAL_CAN_GetDefaultConfig(CAN_ConfigTypeDef *pConfig, CAN_TransmitModeTypeDef TrMode);
uint32_t    HAL_CAN_GetRxFifoHeadAddress(CAN_TypeDef *can);
void        HAL_CAN_GetBusErrorCount(CAN_TypeDef *can, uint8_t *pTxErrBuf, uint8_t *pRxErrBuf);
void        HAL_CAN_SetTimingConfig(CAN_TypeDef *can, const CAN_TimingConfigTypeDef *pConfig);
void        HAL_CAN_SetRxMbGlobalMask(CAN_TypeDef *can, uint32_t Mask);
void        HAL_CAN_SetRxFifoGlobalMask(CAN_TypeDef *can, uint32_t Mask);
void        HAL_CAN_SetRxIndividualMask(CAN_TypeDef *can, uint8_t MaskIdx, uint32_t Mask);
void        HAL_CAN_ConfigTxMb(CAN_TypeDef *can, uint8_t MbIdx, FunctionalState State);
void        HAL_CAN_ConfigRxMb(CAN_TypeDef *can, uint8_t MbIdx, const CAN_RxMbConfigTypeDef *pRxMbConfig, FunctionalState State);
void        HAL_CAN_ConfigRxFifo(CAN_TypeDef *can, const CAN_RxFifoConfigTypeDef *pRxFifoConfig, FunctionalState State);
void        HAL_CAN_ConfigRxDMA(CAN_TypeDef *can, CAN_FrameTypeDef *Frame, uint32_t FrameNum);
/**
 * @}
 */

/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/** @defgroup CAN_Private_Types CAN Private Types
 * @{
 */

/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/** @defgroup CAN_Private_Variables CAN Private Variables
 * @{
 */

/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup CAN_Private_Constants CAN Private Constants
 * @{
 */

/**
 * @}
 */

/* Private Macros -----------------------------------------------------------*/
/** @defgroup CAN_Private_Macros CAN Private Macros
 * @{
 */

/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup CAN_Private_Functions CAN Private Functions
 * @{
 */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_HAL_CAN_H */
