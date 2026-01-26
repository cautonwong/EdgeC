/**
 *****************************************************************************
 * @file   rn_rv_ll_can.h
 * @author Renergy Software Team
 * @brief  Header file of CAN LL module.
 *
 *****************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 *****************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RN_LL_CAN_H
#define __RN_LL_CAN_H

#ifdef __cplusplus
"C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_can.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (CAN)

/** @defgroup CAN_LL CAN
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/
/** @defgroup CAN_LL_Exported_constants CAN Exported constants
 * @{
 */

/** @defgroup CAN_LL_EC_TIMER_SRC Time Source
 * @{
 */
#define LL_CAN_TIMERSRC_BAUDRATE_CLK        0x0U /*!< Timer Source choice : BaudRate Clock. */
#define LL_CAN_TIMERSRC_SIMPTC3_OR_GPIO     0x1U /*!< Timer Source choice : SimpleTC3 overflow signal or GPIO signal. */
/**
 * @}
 */

/** @defgroup CAN_LL_EC_MBIDX Mailbox Index
 * @{
 */
#define LL_CAN_MB0_MASK                     CAN_IFLAG1_BUF0I
#define LL_CAN_MB1_MASK                     CAN_IFLAG1_BUF1I
#define LL_CAN_MB2_MASK                     CAN_IFLAG1_BUF2I
#define LL_CAN_MB3_MASK                     CAN_IFLAG1_BUF3I
#define LL_CAN_MB4_MASK                     CAN_IFLAG1_BUF4I
#define LL_CAN_MB5_MASK                     CAN_IFLAG1_BUF5I
#define LL_CAN_MB6_MASK                     CAN_IFLAG1_BUF6I
#define LL_CAN_MB7_MASK                     CAN_IFLAG1_BUF7I
#define LL_CAN_MB15TO8_MASK                 CAN_IFLAG1_BUF15TO8I
/**
 * @}
 */

/** @defgroup CAN_LL_EC_RXFIFO_FILTER_TYPE Rx Fifo Fliter Type
 * @{
 */
#define LL_CAN_RXFIFO_FILTER_TYPE_A         0x0U /*!< One full ID (standard and extended) per ID Filter element. */
#define LL_CAN_RXFIFO_FILTER_TYPE_B         0x1U /*!< Two full standard IDs or two partial 14-bit ID slices per ID Filter Table element. */
#define LL_CAN_RXFIFO_FILTER_TYPE_C         0x2U /*!< Four partial 8-bit Standard or extended ID slices per ID Filter Table element. */
#define LL_CAN_RXFIFO_FILTER_TYPE_D         0x3U /*!< All frames rejected. */
/**
 * @}
 */

/** @defgroup CAN_LL_EC_ORDING_MECHANISM Ordering Mechanism
 * @{
 */
#define LL_CAN_PRIO_SMALLEST_MBIDX_FIRST    0x0U /*!< The mailbox with minimum mailbox number sending first. */
#define LL_CAN_PRIO_HIGHEST_FIRST           0x1U /*!< The mailbox with highest priority(PRIO+ID) sending first. */
/**
 * @}
 */

/** @defgroup CAN_LL_EC_SAMPLING_MODE Sampling Mode
 * @{
 */
#define LL_CAN_SAMPLING_ONE_POINT           0x0U /*!< Sample one point. */
#define LL_CAN_SAMPLING_THREE_POINT         0x1U /*!< Sample three points. */
/**
 * @}
 */

/** @defgroup CAN_LL_EC_RXFIFO_PRIORITY Rx Fifo Priority
 * @{
 */
#define LL_CAN_RXFIFO_PRIORITY_LOW          0x0U /*!< Matching process start from Rx Message Buffer first */
#define LL_CAN_RXFIFO_PRIORITY_HIGH         0x1U /*!< Matching process start from Rx FIFO first */
/**
 * @}
 */

/** @defgroup CAN_LL_EC_MB_CODE Mailbox CODE Field
 * @{
 */
#define LL_CAN_RXMB_INACTIVE                0x0U /*!< MB is not active. */
#define LL_CAN_RXMB_FULL                    0x2U /*!< MB is full. */
#define LL_CAN_RXMB_EMPTY                   0x4U /*!< MB is active and empty. */
#define LL_CAN_RXMB_OVERRUN                 0x6U /*!< MB is overwritten into a full buffer. */
#define LL_CAN_RXMB_BUSY                    0x8U /*!< CAN is updating the contents of the MB. The CPU must not access the MB. */
#define LL_CAN_RXMB_RANSWER                 0xAU /*!< A frame was configured to recognize a Remote Request Frame and transmit a Response Frame in return. */
#define LL_CAN_RXMB_NOTUSED                 0xFU /*!< Not used. */

#define LL_CAN_TXMB_INACTIVE                0x8U /*!< MB is not active. */
#define LL_CAN_TXMB_ABORT                   0x9U /*!< MB is aborted. */
#define LL_CAN_TXMB_DATAORREMOT             0xCU /*!< MB is a TX Data Frame(when MB RTR = 0) or MB is a TX Remote Request Frame (when MB RTR = 1). */
#define LL_CAN_TXMB_TANSWER                 0xEU /*!< MB is a TX Response Request Frame from an incoming Remote Request Frame. */
#define LL_CAN_TXMB_NOTUSED                 0xFU /*!< Not used. */
/**
 * @}
 */

/** @defgroup CAN_LL_EC_FLAG Flag Defination
 * @{
 */
#define LL_CAN_FLAG_SYNCH                   CAN_ESR1_SYNCH     /*!< CAN Synchronization Status. */
#define LL_CAN_FLAG_TXWARNINGINT            CAN_ESR1_TWRNINT   /*!< Tx Warning Interrupt Flag. */
#define LL_CAN_FLAG_RXWARNINGINT            CAN_ESR1_RWRNINT   /*!< Rx Warning Interrupt Flag. */
#define LL_CAN_FLAG_TXERRORWARNING          CAN_ESR1_TXWRN     /*!< Tx Error Warning Status. */
#define LL_CAN_FLAG_RXERRORWARNING          CAN_ESR1_RXWRN     /*!< Rx Error Warning Status. */
#define LL_CAN_FLAG_IDLE                    CAN_ESR1_IDLE      /*!< CAN IDLE Status Flag. */
#define LL_CAN_FLAG_FAULTCONFINEMENT        CAN_ESR1_FLTCONF   /*!< Fault Confinement State Flag. */
#define LL_CAN_FLAG_TRANSMITTING            CAN_ESR1_TX        /*!< CAN In Transmission Status. */
#define LL_CAN_FLAG_RECEIVING               CAN_ESR1_RX        /*!< CAN In Reception Status. */
#define LL_CAN_FLAG_BUSOFFINT               CAN_ESR1_BOFFINT   /*!< Bus Off Interrupt Flag. */
#define LL_CAN_FLAG_ERRORINT                CAN_ESR1_ERRINT    /*!< Error Interrupt Flag. */
#define LL_CAN_FLAG_WAKEUPINT               CAN_ESR1_WAKINT    /*!< Wake-Up Interrupt Flag. */

#define LL_CAN_ERROR_STUFFING               CAN_ESR1_STFERR    /*!< Stuffing Error. */
#define LL_CAN_ERROR_FORM                   CAN_ESR1_FRMERR    /*!< Form Error. */
#define LL_CAN_ERROR_CAN                    CAN_ESR1_CRCERR    /*!< Cyclic Redundancy Check Error. */
#define LL_CAN_ERROR_ACK                    CAN_ESR1_ACKERR    /*!< Received no ACK on transmission. */
#define LL_CAN_ERROR_BIT0                   CAN_ESR1_BIT0ERR   /*!< Unable to send dominant bit. */
#define LL_CAN_ERROR_BIT1                   CAN_ESR1_BIT1ERR   /*!< Unable to send recessive bit. */
/**
 * @}
 */

/** @defgroup CAN_LL_EC_FAULT_STATE Fault State
 * @{
 */
#define LL_CAN_STA_ERROR_ACTIVE             0x0U /*!< Active Error state. */
#define LL_CAN_STA_ERROR_PASSIVE            0x1U /*!< Passive Error state. */
#define LL_CAN_STA_BUS_OFF_0                0x2U /*!< Bus Off Error state. */
#define LL_CAN_STA_BUS_OFF_1                0x3U /*!< Bus Off Error state. */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @defgroup CAN_LL_Exported_Functions CAN Exported Functions
 * @{
 */

/** @defgroup CAN_LL_EF_Configuration Configuration
 * @{
 */
/**
 * @brief  Set the maxinum number of mailboxes to enable.
 * @rmtoll MCR         MAXMB            LL_CAN_SetMaxMB
 * @note   The number of the last MB that will take part in the matching and
 *         arbitration processes. The reset value (0xF) is equivalent to a 16
 *         message buffer configuration. MAXMB can be written in Freeze mode
 *         only because it is blocked by hardware in other modes.
 *                      NumberOfMaxMB = Number + 1
 *         MAXMB must be programmed with a value smaller than or equal to the
 *         number of available message buffers. Additionally, the definition of
 *         MAXMB value must take into account the region of MBs occupied by
 *         Rx FIFO and its ID filtering table space defined by CTRL2.RFFN.
 * @param  CANx CAN instance
 * @param  Number Between 0 to 0xF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetMaxMB(CAN_TypeDef *CANx, uint32_t Number)
{
    MODIFY_REG(CANx->MCR, CAN_MCR_MAXMB, Number);
}

/**
 * @brief  Get the maxinum number of mailboxes to enable.
 * @rmtoll MCR         MAXMB            LL_CAN_GetMaxMB
 * @param  CANx CAN instance
 * @retval Between 0 to 0xF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetMaxMB(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->MCR, CAN_MCR_MAXMB) >> CAN_MCR_MAXMB_POS);
}

/**
 * @brief  Set the format of Rx FIFO ID Filter Table elements.
 * @rmtoll MCR         IDAM            LL_CAN_SetRxFifoIDFilterFormat
 * @note   All elements of the table are configured at the same time by IDAM
 *         (they are all the same format). IDAM can be written in Freeze mode
 *         only because it is blocked by hardware in other modes.
 * @param  CANx CAN instance
 * @param  Format This parameter can be one of the following values:
 *         @arg @ref LL_CAN_RXFIFO_FILTER_TYPE_A
 *         @arg @ref LL_CAN_RXFIFO_FILTER_TYPE_B
 *         @arg @ref LL_CAN_RXFIFO_FILTER_TYPE_C
 *         @arg @ref LL_CAN_RXFIFO_FILTER_TYPE_D
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetRxFifoIDFilterFormat(CAN_TypeDef *CANx, uint32_t Format)
{
    MODIFY_REG(CANx->MCR, CAN_MCR_IDAM, Format);
}

/**
 * @brief  Get the format of Rx FIFO ID Filter Table elements.
 * @rmtoll MCR         IDAM            LL_CAN_GetRxFifoIDFilterFormat
 * @param  CANx CAN instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_CAN_RXFIFO_FILTER_TYPE_A
 *         @arg @ref LL_CAN_RXFIFO_FILTER_TYPE_B
 *         @arg @ref LL_CAN_RXFIFO_FILTER_TYPE_C
 *         @arg @ref LL_CAN_RXFIFO_FILTER_TYPE_D
 */
__STATIC_INLINE uint32_t LL_CAN_GetRxFifoIDFilterFormat(CAN_TypeDef *CANx)
{
    return (uint32_t)READ_BIT(CANx->MCR, CAN_MCR_IDAM);
}

/**
 * @brief  Enable Tx abort.
 * @rmtoll MCR         ABTEN         LL_CAN_EnableTxAbort
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableTxAbort(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_AEN);
}

/**
 * @brief  Disable Tx abort.
 * @rmtoll MCR         ABTEN         LL_CAN_DisableTxAbort
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableTxAbort(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->MCR, CAN_MCR_AEN);
}

/**
 * @brief  Check if Tx abort is enabled or not.
 * @rmtoll MCR         ABTEN         LL_CAN_IsEnabledTxAbort
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledTxAbort(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_AEN) == CAN_MCR_AEN) ? 1UL : 0UL);
}

/**
 * @brief  Enable local priority.
 * @rmtoll MCR         PRIOEN         LL_CAN_EnableLocalPriority
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableLocalPriority(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_LPRIOEN);
}

/**
 * @brief  Disable local priority.
 * @rmtoll MCR         PRIOEN         LL_CAN_DisableLocalPriority
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableLocalPriority(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->MCR, CAN_MCR_LPRIOEN);
}

/**
 * @brief  Check if local priority is enabled or not.
 * @rmtoll MCR         PRIOEN         LL_CAN_IsEnabledLocalPriority
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledLocalPriority(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_LPRIOEN) == CAN_MCR_LPRIOEN) ? 1UL : 0UL);
}

/**
 * @brief  Enable Pretended Networking.
 * @rmtoll MCR         PNETEN         LL_CAN_EnablePretendedNetwork
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnablePretendedNetwork(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_PNETEN);
}

/**
 * @brief  Disable Pretended Networking.
 * @rmtoll MCR         PNETEN         LL_CAN_DisablePretendedNetwork
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisablePretendedNetwork(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->MCR, CAN_MCR_PNETEN);
}

/**
 * @brief  Check if Pretended Networking is enabled or not.
 * @rmtoll MCR         PNETEN         LL_CAN_IsEnabledPretendedNetwork
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledPretendedNetwork(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_PNETEN) == CAN_MCR_PNETEN) ? 1UL : 0UL);
}

/**
 * @brief  Enable Rx Fifo DMA.
 * @rmtoll MCR         DMAEN         LL_CAN_EnableRxFifoDMA
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableRxFifoDMA(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_DMA);
}

/**
 * @brief  Disable Rx Fifo DMA.
 * @rmtoll MCR         DMAEN         LL_CAN_DisableRxFifoDMA
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableRxFifoDMA(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->MCR, CAN_MCR_DMA);
}

/**
 * @brief  Check if Rx Fifo DMA is enabled or not.
 * @rmtoll MCR         DMAEN         LL_CAN_IsEnabledRxFifoDMA
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledRxFifoDMA(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_DMA) == CAN_MCR_DMA) ? 1UL : 0UL);
}

/**
 * @brief  Enable Individual Rx Masking and Queue feature.
 * @rmtoll MCR         IRMQEN         LL_CAN_EnableRxMaskingandQueue
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableRxMaskingandQueue(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_IRMQ);
}

/**
 * @brief  Disable Individual Rx Masking and Queue feature.
 * @rmtoll MCR         IRMQEN         LL_CAN_DisableRxMaskingandQueue
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableRxMaskingandQueue(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->MCR, CAN_MCR_IRMQ);
}

/**
 * @brief  Check if Individual Rx Masking and Queue feature is enabled or not.
 * @rmtoll MCR         IRMQEN         LL_CAN_IsEnabledRxMaskingandQueue
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledRxMaskingandQueue(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_IRMQ) == CAN_MCR_IRMQ) ? 1UL : 0UL);
}

/**
 * @brief  Enable Self Reception.
 * @rmtoll MCR         SRXEN         LL_CAN_EnableSelfReception
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableSelfReception(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_SRXDIS);
}

/**
 * @brief  Disable Self Reception.
 * @rmtoll MCR         SRXEN         LL_CAN_DisableSelfReception
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableSelfReception(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->MCR, CAN_MCR_SRXDIS);
}

/**
 * @brief  Check if Self Reception is enabled or not.
 * @rmtoll MCR         SRXEN         LL_CAN_IsEnabledSelfReception
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledSelfReception(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_SRXDIS) == CAN_MCR_SRXDIS) ? 1UL : 0UL);
}

/**
 * @brief  Enable Sleep Mode.
 * @rmtoll MCR         DOZE         LL_CAN_EnableSleepMode
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableSleepMode(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_DOZE);
}

/**
 * @brief  Disable Sleep Mode.
 * @rmtoll MCR         DOZE         LL_CAN_DisableSleepMode
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableSleepMode(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->MCR, CAN_MCR_DOZE);
}

/**
 * @brief  Check if Sleep Mode is enabled or not.
 * @rmtoll MCR         DOZE         LL_CAN_IsEnabledSleepMode
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledSleepMode(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_DOZE) == CAN_MCR_DOZE) ? 1UL : 0UL);
}

/**
 * @brief  Check if Low-Power Mode is enabled or not.
 * @rmtoll MCR         LPM_STA         LL_CAN_IsEnteredLowPowerMode
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnteredLowPowerMode(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_LPMACK) == CAN_MCR_LPMACK) ? 1UL : 0UL);
}

/**
 * @brief  Enable Self-awakening.
 * @rmtoll MCR         SWUEN         LL_CAN_EnableSelfAwakening
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableSelfAwakening(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_SLFWAK);
}

/**
 * @brief  Disable Self-awakening.
 * @rmtoll MCR         SWUEN         LL_CAN_DisableSelfAwakening
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableSelfAwakening(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->MCR, CAN_MCR_SLFWAK);
}

/**
 * @brief  Check if Self-awakening is enabled or not.
 * @rmtoll MCR         SWUEN         LL_CAN_IsEnabledSelfAwakening
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledSelfAwakening(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_SLFWAK) == CAN_MCR_SLFWAK) ? 1UL : 0UL);
}

/**
 * @brief  Check if Freeze Mode Acknowledge is running or not.
 * @rmtoll MCR         FRZ_STA         LL_CAN_GetFreezeModeACK
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsFreezeModeRunning(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_FRZACK) == CAN_MCR_FRZACK) ? 1UL : 0UL);
}

/**
 * @brief  Starting Soft Reset.
 * @rmtoll MCR         SOFTRST         LL_CAN_SoftReset
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SoftReset(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_SOFTRST);
}

/**
 * @brief  Check if Soft Reset is done or not.
 * @rmtoll MCR         SOFTRST         LL_CAN_IsSoftResetDone
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsSoftResetDone(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_SOFTRST) != CAN_MCR_SOFTRST) ? 1UL : 0UL);
}

/**
 * @brief  Get the CAN Status.
 * @rmtoll MCR         NOTRDY         LL_CAN_GetStatus
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 *         If return 1, it means that FlexCAN is either in Stop, Sleep, Disable or Freeze Mode.
 *         If return 0, it means that FlexCAN is either in Normal, Listen-Only, or Loop-Back Mode.
 */
__STATIC_INLINE uint32_t LL_CAN_GetStatus(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->MCR, CAN_MCR_NOTRDY) >> CAN_MCR_NOTRDY_POS);
}

/**
 * @brief  Enable HALT.
 * @rmtoll MCR         HALT         LL_CAN_EnableHalt
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableHalt(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_HALT);
}

/**
 * @brief  Disable HALT.
 * @rmtoll MCR         HALT         LL_CAN_DisableHalt
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableHalt(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->MCR, CAN_MCR_HALT);
}

/**
 * @brief  Check if HALT is enabled or not.
 * @rmtoll MCR         HALT         LL_CAN_IsEnabledHalt
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledHalt(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_HALT) == CAN_MCR_HALT) ? 1UL : 0UL);
}

/**
 * @brief  Enable Rx Fifo.
 * @rmtoll MCR         RFEN         LL_CAN_EnableRxFifo
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableRxFifo(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_RFEN);
}

/**
 * @brief  Disable Rx Fifo.
 * @rmtoll MCR         RFEN         LL_CAN_DisableRxFifo
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableRxFifo(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->MCR, CAN_MCR_RFEN);
}

/**
 * @brief  Check if Rx Fifo is enabled or not.
 * @rmtoll MCR         RFEN         LL_CAN_IsEnabledRxFifo
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledRxFifo(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_RFEN) == CAN_MCR_RFEN) ? 1UL : 0UL);
}

/**
 * @brief  Enable Freeze Mode.
 * @rmtoll MCR         FRZEN         LL_CAN_EnableFreezeMode
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableFreezeMode(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_FRZ);
}

/**
 * @brief  Disable Freeze Mode.
 * @rmtoll MCR         FRZEN         LL_CAN_DisableFreezeMode
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableFreezeMode(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->MCR, CAN_MCR_FRZ);
}

/**
 * @brief  Check if Freeze Mode is enabled or not.
 * @rmtoll MCR         FRZEN         LL_CAN_IsEnabledFreezeMode
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledFreezeMode(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_FRZ) == CAN_MCR_FRZ) ? 1UL : 0UL);
}

/**
 * @brief  Enable the FlexCAN Module.
 * @rmtoll MCR         MDIS         LL_CAN_Enable
 * @note   When MCR.MDIS=1, the FlexCAN module will be disabled as well as the clocks
 *         to the PE and CHI submodules.
 *         When MCR.MDIS=0, the FlexCAN module will be enabled.
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_Enable(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->MCR, CAN_MCR_MDIS);
}

/**
 * @brief  Disable the FlexCAN Module.
 * @rmtoll MCR         MDIS         LL_CAN_Disable
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_Disable(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_MDIS);
}

/**
 * @brief  Check if the FlexCAN Module is enabled or not.
 * @rmtoll MCR         MDIS         LL_CAN_IsEnabled
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabled(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_MDIS) != CAN_MCR_MDIS) ? 1UL : 0UL);
}

/**
 * @brief  Set Propagetion Segment.
 * @rmtoll CTRL1         PROPSEDG            LL_CAN_SetPropSegment
 * @note   Defines the length of the Propagation Segment in the bit time:
 *                  Propagation time = (PROPSEG + 1) × Tq
 *         The valid values for PROPSEG are 0~7. PROPSEG can be written only in
 *         Freeze mode because it is blocked by hardware in other modes.
 * @param  CANx CAN instance
 * @param  Number Between 0 to 7.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetPropSegment(CAN_TypeDef *CANx, uint32_t Number)
{
    MODIFY_REG(CANx->CTRL1, CAN_CTRL1_PROPSEG, Number);
}

/**
 * @brief  Get Propagetion Segment.
 * @rmtoll CTRL1         PROPSEDG            LL_CAN_GetPropSegment
 * @param  CANx CAN instance
 * @retval Between 0 to 7.
 */
__STATIC_INLINE uint32_t LL_CAN_GetPropSegment(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->CTRL1, CAN_CTRL1_PROPSEG) >> CAN_CTRL1_PROPSEG_POS);
}

/**
 * @brief  Enable Listen-Only Mode.
 * @rmtoll CTRL1         LOM         LL_CAN_EnableListenOnlyMode
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableListenOnlyMode(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->CTRL1, CAN_CTRL1_LOM);
}

/**
 * @brief  Disable Listen-Only Mode.
 * @rmtoll CTRL1         LOM         LL_CAN_DisableListenOnlyMode
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableListenOnlyMode(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->CTRL1, CAN_CTRL1_LOM);
}

/**
 * @brief  Check if Listen-Only Mode is enabled or not.
 * @rmtoll CTRL1         LOM         LL_CAN_IsEnabledListenOnlyMode
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledListenOnlyMode(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->CTRL1, CAN_CTRL1_LOM) == CAN_CTRL1_LOM) ? 1UL : 0UL);
}

/**
 * @brief  Set the ording mechanism for message buffer transmission.
 * @rmtoll CTRL1         LBUF            LL_CAN_SetMBTransmitPriority
 * @param  CANx CAN instance
 * @param  Priority This parameter can be one of the following values:
 *         @arg @ref LL_CAN_PRIO_SMALLEST_MBIDX_FIRST
 *         @arg @ref LL_CAN_PRIO_HIGHEST_FIRST
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetMBTransmitPriority(CAN_TypeDef *CANx, uint32_t Priority)
{
    MODIFY_REG(CANx->CTRL1, CAN_CTRL1_LBUF, Priority);
}

/**
 * @brief  Get the ording mechanism for message buffer transmission.
 * @rmtoll CTRL1         LBUF            LL_CAN_GetMBTransmitPriority
 * @param  CANx CAN instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_CAN_PRIO_SMALLEST_MBIDX_FIRST
 *         @arg @ref LL_CAN_PRIO_HIGHEST_FIRST
 */
__STATIC_INLINE uint32_t LL_CAN_GetMBTransmitPriority(CAN_TypeDef *CANx)
{
    return (uint32_t)READ_BIT(CANx->CTRL1, CAN_CTRL1_LBUF);
}

/**
 * @brief  Enable Timer Sync.
 * @rmtoll CTRL1         TSYNEN         LL_CAN_EnableTimerSync
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableTimerSync(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->CTRL1, CAN_CTRL1_TSYN);
}

/**
 * @brief  Disable Timer Sync.
 * @rmtoll CTRL1         TSYNEN         LL_CAN_DisableTimerSync
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableTimerSync(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->CTRL1, CAN_CTRL1_TSYN);
}

/**
 * @brief  Check if Timer Sync is enabled or not.
 * @rmtoll CTRL1         TSYNEN         LL_CAN_IsEnabledTimerSync
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledTimerSync(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->CTRL1, CAN_CTRL1_TSYN) == CAN_CTRL1_TSYN) ? 1UL : 0UL);
}

/**
 * @brief  Enable the Bus Off state with automatic recovering.
 * @rmtoll CTRL1         BOFFREN         LL_CAN_EnableBusOffState
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableBusOffState(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->CTRL1, CAN_CTRL1_BOFFRREC);
}

/**
 * @brief  Disable the Bus Off state with automatic recovering.
 * @rmtoll CTRL1         BOFFREN         LL_CAN_DisableBusOffState
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableBusOffState(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->CTRL1, CAN_CTRL1_BOFFRREC);
}

/**
 * @brief  Check if the Bus Off state with automatic recovering is enabled or not.
 * @rmtoll CTRL1         BOFFREN         LL_CAN_IsEnabledBusOffState
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledBusOffState(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->CTRL1, CAN_CTRL1_BOFFRREC) == CAN_CTRL1_BOFFRREC) ? 1UL : 0UL);
}

/**
 * @brief  Set Bit Sampling Point.
 * @rmtoll CTRL1         SMP            LL_CAN_SetBitSamplingPoint
 * @param  CANx CAN instance
 * @param  SMPNum This parameter can be one of the following values:
 *         @arg @ref LL_CAN_SAMPLING_ONE_POINT
 *         @arg @ref LL_CAN_SAMPLING_THREE_POINT
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetBitSamplingPoint(CAN_TypeDef *CANx, uint32_t SMPNum)
{
    MODIFY_REG(CANx->CTRL1, CAN_CTRL1_SMP, SMPNum);
}

/**
 * @brief  Get Bit Sampling Point.
 * @rmtoll CTRL1         SMP            LL_CAN_GetBitSamplingPoint
 * @param  CANx CAN instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_CAN_SAMPLING_ONE_POINT
 *         @arg @ref LL_CAN_SAMPLING_THREE_POINT
 */
__STATIC_INLINE uint32_t LL_CAN_GetBitSamplingPoint(CAN_TypeDef *CANx)
{
    return (uint32_t)READ_BIT(CANx->CTRL1, CAN_CTRL1_SMP);
}

/**
 * @brief  Enable Loop-back Mode.
 * @rmtoll CTRL1         LPB         LL_CAN_EnableLoopBackMode
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableLoopBackMode(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->CTRL1, CAN_CTRL1_LPB);
}

/**
 * @brief  Disable Loop-back Mode.
 * @rmtoll CTRL1         LPB         LL_CAN_DisableLoopBackMode
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableLoopBackMode(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->CTRL1, CAN_CTRL1_LPB);
}

/**
 * @brief  Check if Loop-back Mode is enabled or not.
 * @rmtoll CTRL1         LPB         LL_CAN_IsEnabledLoopBackMode
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledLoopBackMode(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->CTRL1, CAN_CTRL1_LPB) == CAN_CTRL1_LPB) ? 1UL : 0UL);
}

/**
 * @brief  Set Phase Segment 2.
 * @rmtoll CTRL1         PSEG2            LL_CAN_SetPhaseSegment2
 * @note   Defines the length of Phase Segment 2 in the bit time:
 *                    Phase Segment 2 = (PSEG2 + 1) × Tq
 * @param  CANx CAN instance
 * @param  PSeg2Len Between 0 to 7.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetPhaseSegment2(CAN_TypeDef *CANx, uint32_t PSeg2Len)
{
    MODIFY_REG(CANx->CTRL1, CAN_CTRL1_PSEG2, PSeg2Len);
}

/**
 * @brief  Get Phase Segment 2.
 * @rmtoll CTRL1         PSEG2            LL_CAN_GetPhaseSegment2
 * @param  CANx CAN instance
 * @retval Between 0 to 7.
 */
__STATIC_INLINE uint32_t LL_CAN_GetPhaseSegment2(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->CTRL1, CAN_CTRL1_PSEG2) >> CAN_CTRL1_PSEG2_POS);
}

/**
 * @brief  Set Phase Segment 1.
 * @rmtoll CTRL1         PSEG1            LL_CAN_SetPhaseSegment1
 * @note   Defines the length of Phase Segment 1 in the bit time:
 *                Phase Segment 1 = (PSEG1 + 1) × Tq
 * @param  CANx CAN instance
 * @param  PSeg1Len Between 0 to 7.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetPhaseSegment1(CAN_TypeDef *CANx, uint32_t PSeg1Len)
{
    MODIFY_REG(CANx->CTRL1, CAN_CTRL1_PSEG1, PSeg1Len);
}

/**
 * @brief  Get Phase Segment 1.
 * @rmtoll CTRL1         PSEG1            LL_CAN_GetPhaseSegment1
 * @param  CANx CAN instance
 * @retval Between 0 to 7.
 */
__STATIC_INLINE uint32_t LL_CAN_GetPhaseSegment1(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->CTRL1, CAN_CTRL1_PSEG1) >> CAN_CTRL1_PSEG1_POS);
}

/**
 * @brief  Set Resync Jump Width.
 * @rmtoll CTRL1         RJW            LL_CAN_SetResyncJumpWidth
 * @note   Defines the maximum number of Tq that a bit time can be changed by
 *         one re-synchronization:
 *                          Resync Jump Width = RJW + 1
 * @param  CANx CAN instance
 * @param  Width Between 0 to 3.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetResyncJumpWidth(CAN_TypeDef *CANx, uint32_t Width)
{
    MODIFY_REG(CANx->CTRL1, CAN_CTRL1_RJW, Width);
}

/**
 * @brief  Get Resync Jump Width.
 * @rmtoll CTRL1         RJW            LL_CAN_GetResyncJumpWidth
 * @param  CANx CAN instance
 * @retval Between 0 to 3.
 */
__STATIC_INLINE uint32_t LL_CAN_GetResyncJumpWidth(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->CTRL1, CAN_CTRL1_RJW) >> CAN_CTRL1_RJW_POS);
}

/**
 * @brief  Set Prescaler Division Factor.
 * @rmtoll CTRL1         PRESDIV            LL_CAN_SetPrescalerDiv
 * @note    Defines the ratio between the PE clock frequency and the Serial
 *          Clock (Sclock) frequency:
 *              Sclock frequency = PE clock frequency / (PRESDIV + 1)
 * @param  CANx CAN instance
 * @param  Ratio Between 0 to 0xFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetPrescalerDiv(CAN_TypeDef *CANx, uint32_t Ratio)
{
    MODIFY_REG(CANx->CTRL1, CAN_CTRL1_PRESDIV, Ratio);
}

/**
 * @brief  Get Prescaler Division Factor.
 * @rmtoll CTRL1         PRESDIV            LL_CAN_GetPrescalerDiv
 * @param  CANx CAN instance
 * @retval Between 0 to 0xFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetPrescalerDiv(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->CTRL1, CAN_CTRL1_PRESDIV) >> CAN_CTRL1_PRESDIV_POS);
}

/**
 * @brief  Set Timer Register.
 * @rmtoll TIMER         CNT            LL_CAN_SetTimerCnt
 * @param  CANx CAN instance
 * @param  TimerCnt Between 0 to 0xFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetTimerCnt(CAN_TypeDef *CANx, uint32_t TimerCnt)
{
    MODIFY_REG(CANx->TIMER, CAN_TIMER_TIMER, TimerCnt);
}

/**
 * @brief  Get Timer Register.
 * @rmtoll TIMER         CNT            LL_CAN_GetTimerCnt
 * @param  CANx CAN instance
 * @retval Between 0 to 0xFFFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetTimerCnt(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->TIMER, CAN_TIMER_TIMER) >> CAN_TIMER_TIMER_POS);
}

/**
 * @brief  Set Rx Mailbox Global Mask.
 * @rmtoll RXMGMASK         MG            LL_CAN_SetRxMBGlobalMsk
 * @param  CANx CAN instance
 * @param  Mask Between 0 to 0xFFFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetRxMBGlobalMsk(CAN_TypeDef *CANx, uint32_t Mask)
{
    MODIFY_REG(CANx->RXMGMASK, CAN_RXMGMASK_MG, Mask);
}

/**
 * @brief  Get Rx Mailbox Global Mask.
 * @rmtoll RXMGMASK         MG            LL_CAN_GetRxMBGlobalMsk
 * @param  CANx CAN instance
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetRxMBGlobalMsk(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->RXMGMASK, CAN_RXMGMASK_MG) >> CAN_RXMGMASK_MG_POS);
}

/**
 * @brief  Set Rx MB14 Mask.
 * @rmtoll RX14MASK         MG            LL_CAN_SetRxMB14ask
 * @param  CANx CAN instance
 * @param  Mask Between 0 to 0xFFFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetRxMB14Msk(CAN_TypeDef *CANx, uint32_t Mask)
{
    MODIFY_REG(CANx->RX14MASK, CAN_RX14MASK_RX14M, Mask);
}

/**
 * @brief  Get Rx MB14 Mask.
 * @rmtoll RX14MASK         MG            LL_CAN_GetRxMB14Msk
 * @param  CANx CAN instance
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetRxMB14Msk(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->RX14MASK, CAN_RX14MASK_RX14M) >> CAN_RX14MASK_RX14M_POS);
}

/**
 * @brief  Set Rx MB15 Mask.
 * @rmtoll RX15MASK         MG            LL_CAN_SetRxMB15Msk
 * @param  CANx CAN instance
 * @param  Mask Between 0 to 0xFFFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetRxMB15Msk(CAN_TypeDef *CANx, uint32_t Mask)
{
    MODIFY_REG(CANx->RX15MASK, CAN_RX15MASK_RX15M, Mask);
}

/**
 * @brief  Get Rx MB15 Mask.
 * @rmtoll RX15MASK         MG            LL_CAN_GetRxMB15Msk
 * @param  CANx CAN instance
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetRxMB15Msk(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->RX15MASK, CAN_RX15MASK_RX15M) >> CAN_RX15MASK_RX15M_POS);
}

/**
 * @brief  Set Tx Error Counter Cnt.
 * @rmtoll ECR         TXERRCNT            LL_CAN_SetTxErrorCnt
 * @param  CANx CAN instance
 * @param  ErrorCnt Between 0 to 0xFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetTxErrorCnt(CAN_TypeDef *CANx, uint32_t ErrorCnt)
{
    MODIFY_REG(CANx->ECR, CAN_ECR_TXERRCNT, ErrorCnt);
}

/**
 * @brief  Get Tx Error Counter Cnt.
 * @rmtoll ECR         TXERRCNT            LL_CAN_GetTxErrorCnt
 * @param  CANx CAN instance
 * @retval Between 0 to 0xFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetTxErrorCnt(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->ECR, CAN_ECR_TXERRCNT) >> CAN_ECR_TXERRCNT_POS);
}

/**
 * @brief  Set Rx Error Counter Cnt.
 * @rmtoll ECR         RXERRCNT            LL_CAN_SetRxErrorCnt
 * @param  CANx CAN instance
 * @param  ErrorCnt Between 0 to 0xFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetRxErrorCnt(CAN_TypeDef *CANx, uint32_t ErrorCnt)
{
    MODIFY_REG(CANx->ECR, CAN_ECR_RXERRCNT, ErrorCnt);
}

/**
 * @brief  Get Rx Error Counter Cnt.
 * @rmtoll ECR         RXERRCNT            LL_CAN_GetRxErrorCnt
 * @param  CANx CAN instance
 * @retval Between 0 to 0xFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetRxErrorCnt(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->ECR, CAN_ECR_RXERRCNT) >> CAN_ECR_RXERRCNT_POS);
}

/**
 * @brief  Enable Ehanced Bit Timing.
 * @rmtoll CTRL2         BTEEN         LL_CAN_EnableEnhancedBitTiming
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableEnhancedBitTiming(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->CTRL2, CAN_CTRL2_BTEEN);
}

/**
 * @brief  Disable Ehanced Bit Timing.
 * @rmtoll CTRL2         BTEEN         LL_CAN_DisableEnhancedBitTiming
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableEnhancedBitTiming(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->CTRL2, CAN_CTRL2_BTEEN);
}

/**
 * @brief  Check if Ehanced Bit Timing is enabled or not.
 * @rmtoll CTRL2         BTEEN         LL_CAN_IsEnableeEnhancedBitTiming
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledEnhancedBitTiming(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->CTRL2, CAN_CTRL2_BTEEN) == CAN_CTRL2_BTEEN) ? 1UL : 0UL);
}

/**
 * @brief  Set Input Source of the internel Timer counter.
 * @rmtoll CTRL2         TIMER_SRC            LL_CAN_SetTimerSource
 * @param  CANx CAN instance
 * @param  InputSrc This parameter can be one of the following values:
 *         @arg @ref LL_CAN_TIMERSRC_BAUDRATE_CLK
 *         @arg @ref LL_CAN_TIMERSRC_SIMPTC3_OR_GPIO
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetTimerSource(CAN_TypeDef *CANx, uint32_t InputSrc)
{
    MODIFY_REG(CANx->CTRL2, CAN_CTRL2_TIMERSRC, InputSrc);
}

/**
 * @brief  Get Input Source of the internel Timer counter.
 * @rmtoll CTRL2         TIMER_SRC            LL_CAN_GetTimerSource
 * @param  CANx CAN instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_CAN_TIMERSRC_BAUDRATE_CLK
 *         @arg @ref LL_CAN_TIMERSRC_SIMPTC3_OR_GPIO
 */
__STATIC_INLINE uint32_t LL_CAN_GetTimerSource(CAN_TypeDef *CANx)
{
    return (uint32_t)READ_BIT(CANx->CTRL2, CAN_CTRL2_TIMERSRC);
}

/**
 * @brief  Enable Entire Frame Arbitration Field Comparison Enable for Rx Mailboxes.
 * @rmtoll CTRL2         EACEN         LL_CAN_EnableRxMBFrameComparison
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableRxMBFrameComparison(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->CTRL2, CAN_CTRL2_EACEN);
}

/**
 * @brief  Disable Entire Frame Arbitration Field Comparison Enable for Rx Mailboxes.
 * @rmtoll CTRL2         EACEN         LL_CAN_DisableRxMBFrameComparison
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableRxMBFrameComparison(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->CTRL2, CAN_CTRL2_EACEN);
}

/**
 * @brief  Check if Entire Frame Arbitration Field Comparison Enable for Rx Mailboxes is enabled or not.
 * @rmtoll CTRL2         EACEN         LL_CAN_IsEnabledRxMBFrameComparison
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledRxMBFrameComparison(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->CTRL2, CAN_CTRL2_EACEN) == CAN_CTRL2_EACEN) ? 1UL : 0UL);
}

/**
 * @brief  Enable Remote Request Storing.
 * @rmtoll CTRL2         RRS         LL_CAN_EnableRemoteRequestStoring
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableRemoteRequestStoring(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->CTRL2, CAN_CTRL2_RRS);
}

/**
 * @brief  Disable Remote Request Storing.
 * @rmtoll CTRL2         RRS         LL_CAN_DisableRemoteRequestStoring
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableRemoteRequestStoring(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->CTRL2, CAN_CTRL2_RRS);
}

/**
 * @brief  Check if Remote Request Storing is enabled or not.
 * @rmtoll CTRL2         RRS         LL_CAN_IsEnabledRemoteRequestStoring
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledRemoteRequestStoring(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->CTRL2, CAN_CTRL2_RRS) == CAN_CTRL2_RRS) ? 1UL : 0UL);
}

/**
 * @brief  Set Rx Mailbox Reception Priority.
 * @rmtoll CTRL2         MRP            LL_CAN_SetRxMBReceptionPriority
 * @param  CANx CAN instance
 * @param  Priority This parameter can be one of the following values:
 *         @arg @ref LL_CAN_RXFIFO_PRIORITY_LOW
 *         @arg @ref LL_CAN_RXFIFO_PRIORITY_HIGH
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetRxMBReceptionPriority(CAN_TypeDef *CANx, uint32_t Priority)
{
    MODIFY_REG(CANx->CTRL2, CAN_CTRL2_MRP, Priority);
}

/**
 * @brief  Get Rx Mailbox Reception Priority.
 * @rmtoll CTRL2         MRP            LL_CAN_GetRxMBReceptionPriority
 * @param  CANx CAN instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_CAN_RXFIFO_PRIORITY_LOW
 *         @arg @ref LL_CAN_RXFIFO_PRIORITY_HIGH
 */
__STATIC_INLINE uint32_t LL_CAN_GetRxMBReceptionPriority(CAN_TypeDef *CANx)
{
    return (uint32_t)READ_BIT(CANx->CTRL2, CAN_CTRL2_MRP);
}

/**
 * @brief  Set Tx Arbitration Start Delay.
 * @rmtoll CTRL2         TASD            LL_CAN_SetTxStartDelay
 * @ntoe   Indicates how many CAN bits the Tx arbitration process start point
 *         can be delayed from the first bit of CRC field on CAN bus.
 * @param  CANx CAN instance
 * @param  Delay Between 0 to 0x1F.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetTxStartDelay(CAN_TypeDef *CANx, uint32_t Delay)
{
    MODIFY_REG(CANx->CTRL2, CAN_CTRL2_TASD, Delay);
}

/**
 * @brief  Get Tx Arbitration Start Delay.
 * @rmtoll CTRL2         TASD            LL_CAN_GetTxStartDelay
 * @param  CANx CAN instance
 * @retval Between 0 to 0x1F.
 */
__STATIC_INLINE uint32_t LL_CAN_GetTxStartDelay(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->CTRL2, CAN_CTRL2_TASD) >> CAN_CTRL2_TASD_POS);
}

/**
 * @brief  Set the number of Rx FIFO Filters.
 * @rmtoll CTRL2         RFFN            LL_CAN_SetRxFifoFilterNum
 * @note   The number of the last remaining available mailbox is defined by the
 *         least value between (Maximum number of mailboxes (16) - 1) and the
 *         MCR.MAXMB field.
 * @param  CANx CAN instance
 * @param  FilterNum Between 0 to 0xF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetRxFifoFilterNum(CAN_TypeDef *CANx, uint32_t FilterNum)
{
    MODIFY_REG(CANx->CTRL2, CAN_CTRL2_RFFN, FilterNum);
}

/**
 * @brief  Get the number of Rx FIFO Filters.
 * @rmtoll CTRL2         RFFN            LL_CAN_GetRxFifoFilterNum
 * @param  CANx CAN instance
 * @retval Between 0 to 0xF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetRxFifoFilterNum(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->CTRL2, CAN_CTRL2_RFFN) >> CAN_CTRL2_RFFN_POS);
}

/**
 * @brief  Get Lowest Priority Tx Mailbox.
 * @rmtoll ESR2         LPTM            LL_CAN_GetLowestPriorityTxMB
 * @note   If ESR2.VPS is asserted, LPTM indicates the lowest number inactive mailbox.
 *         If there is no inactive mailbox, then the mailbox indicated depends
 *         on CTRL1.LBUF bit value:
 *              a. If CTRL1.LBUF bit is negated, the mailbox indicated is the one that
 *              has the greatest arbitration value;
 *              b. If CTRL1.LBUF bit is asserted, the mailbox indicated is the highest
 *              number active Tx mailbox.
 *              c. If a Tx mailbox is being transmitted, it is not considered in LPTM
 *              calculation.
 *         If ESR2.IMB is not asserted and a frame is transmitted successfully,
 *         LPTM is updated with its mailbox number.
 * @param  CANx CAN instance
 * @retval Between 0 to 0xFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetLowestPriorityTxMB(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->ESR2, CAN_ESR2_LPTM) >> CAN_ESR2_LPTM_POS);
}

/**
 * @brief  Set Rx FIFO Global Mask.
 * @rmtoll RXFGMASK         FGM            LL_CAN_SetRxFifoGlobalMsk
 * @param  CANx CAN instance
 * @param  Mask Between 0 to 0xFFFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetRxFifoGlobalMsk(CAN_TypeDef *CANx, uint32_t Mask)
{
    MODIFY_REG(CANx->RXFGMASK, CAN_RXFGMASK_FGM, Mask);
}

/**
 * @brief  Get Rx FIFO Global Mask.
 * @rmtoll RXFGMASK         FGM            LL_CAN_GetRxFifoGlobalMsk
 * @param  CANx CAN instance
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetRxFifoGlobalMsk(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->RXFGMASK, CAN_RXFGMASK_FGM) >> CAN_RXFGMASK_FGM_POS);
}

/**
 * @brief  Get Rx FIFO Information.
 * @rmtoll RXFIR         IDHIT            LL_CAN_GetRxFifoInfo
 * @param  CANx CAN instance
 * @retval Between 0 to 0x1FF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetRxFifoInfo(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->RXFIR, CAN_RXFIR_IDHIT) >> CAN_RXFIR_IDHIT_POS);
}

/**
 * @brief  Set Control and Status word.
 * @rmtoll CS         TIME_STAMP            LL_CAN_SetCSConfig
 *         CS         DLC                   LL_CAN_SetCSConfig
 *         CS         RTR                   LL_CAN_SetCSConfig
 *         CS         IDE                   LL_CAN_SetCSConfig
 *         CS         SRR                   LL_CAN_SetCSConfig
 *         CS         CODE                  LL_CAN_SetCSConfig
 *         CS         ESI                   LL_CAN_SetCSConfig
 *         CS         BRS                   LL_CAN_SetCSConfig
 *         CS         EDL                   LL_CAN_SetCSConfig
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @param  Configuration Between 0 to 0xFFFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetCSConfig(CAN_TypeDef *CANx, uint32_t MBIdx, uint32_t Configuration)
{
    MODIFY_REG(CANx->MB[MBIdx].CS, (CAN_CS_TIME_STAMP | CAN_CS_DLC | CAN_CS_RTR | \
            CAN_CS_IDE | CAN_CS_SRR | CAN_CS_CODE | CAN_CS_ESI | CAN_CS_BRS | \
            CAN_CS_EDL), Configuration);
}

/**
 * @brief  Get Control and Status word.
 * @rmtoll CS         TIME_STAMP            LL_CAN_GetCSConfig
 *         CS         DLC                   LL_CAN_GetCSConfig
 *         CS         RTR                   LL_CAN_GetCSConfig
 *         CS         IDE                   LL_CAN_GetCSConfig
 *         CS         SRR                   LL_CAN_GetCSConfig
 *         CS         CODE                  LL_CAN_GetCSConfig
 *         CS         ESI                   LL_CAN_GetCSConfig
 *         CS         BRS                   LL_CAN_GetCSConfig
 *         CS         EDL                   LL_CAN_GetCSConfig
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetCSConfig(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    return (uint32_t)READ_BIT(CANx->MB[MBIdx].CS, (CAN_CS_TIME_STAMP | CAN_CS_DLC | \
                            CAN_CS_RTR | CAN_CS_IDE | CAN_CS_SRR | CAN_CS_CODE | \
                            CAN_CS_ESI | CAN_CS_BRS | CAN_CS_EDL));
}

/**
 * @brief  Set Length of Data in bytes.
 * @rmtoll CS         DLC            LL_CAN_SetDLC
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @param  DataLength Between 0 to 0xF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetDLC(CAN_TypeDef *CANx, uint32_t MBIdx, uint32_t DataLength)
{
    MODIFY_REG(CANx->MB[MBIdx].CS, CAN_CS_DLC, DataLength);
}

/**
 * @brief  Get Length of Data in bytes.
 * @rmtoll CS         DLC            LL_CAN_GetDLC
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval Between 0 to 0xF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetDLC(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    return (READ_BIT(CANx->MB[MBIdx].CS, CAN_CS_DLC) >> CAN_CS_DLC_POS);
}

/**
 * @brief  Enable ID Extend Frame.
 * @rmtoll CS         IDE         LL_CAN_EnableIDE
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableIDE(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    SET_BIT(CANx->MB[MBIdx].CS, CAN_CS_IDE);
}

/**
 * @brief  Disable ID Extend Frame.
 * @rmtoll CS         IDE         LL_CAN_DisableIDE
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableIDE(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    CLEAR_BIT(CANx->MB[MBIdx].CS, CAN_CS_IDE);
}

/**
 * @brief  Check if ID Extend Frame is enabled or not.
 * @rmtoll CS         IDE         LL_CAN_IsEnabledMBIDE
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledIDE(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    return ((READ_BIT(CANx->MB[MBIdx].CS, CAN_CS_IDE) == CAN_CS_IDE) ? 1UL : 0UL);
}

/**
 * @brief  Set Message Buffer Code.
 * @rmtoll CS         CODE            LL_CAN_SetCODE
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @param  Code This parameter can be one of the following values:
 *         @arg @ref LL_CAN_RXMB_INACTIVE
 *         @arg @ref LL_CAN_RXMB_FULL
 *         @arg @ref LL_CAN_RXMB_EMPTY
 *         @arg @ref LL_CAN_RXMB_OVERRUN
 *         @arg @ref LL_CAN_RXMB_BUSY
 *         @arg @ref LL_CAN_RXMB_RANSWER
 *         @arg @ref LL_CAN_RXMB_NOTUSED
 *         @arg @ref LL_CAN_TXMB_INACTIVE
 *         @arg @ref LL_CAN_TXMB_ABORT
 *         @arg @ref LL_CAN_TXMB_DATAORREMOTE
 *         @arg @ref LL_CAN_TXMB_TANSWER
 *         @arg @ref LL_CAN_TXMB_NOTUSED
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetCODE(CAN_TypeDef *CANx, uint32_t MBIdx, uint32_t Code)
{
    MODIFY_REG(CANx->MB[MBIdx].CS, CAN_CS_CODE, Code);
}

/**
 * @brief  Get Message Buffer Code.
 * @rmtoll CS         CODE            LL_CAN_GetCODE
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_CAN_RXMB_INACTIVE
 *         @arg @ref LL_CAN_RXMB_FULL
 *         @arg @ref LL_CAN_RXMB_EMPTY
 *         @arg @ref LL_CAN_RXMB_OVERRUN
 *         @arg @ref LL_CAN_RXMB_BUSY
 *         @arg @ref LL_CAN_RXMB_RANSWER
 *         @arg @ref LL_CAN_RXMB_NOTUSED
 *         @arg @ref LL_CAN_TXMB_INACTIVE
 *         @arg @ref LL_CAN_TXMB_ABORT
 *         @arg @ref LL_CAN_TXMB_DATAORREMOTE
 *         @arg @ref LL_CAN_TXMB_TANSWER
 *         @arg @ref LL_CAN_TXMB_NOTUSED
 */
__STATIC_INLINE uint32_t LL_CAN_GetCODE(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    return (READ_BIT(CANx->MB[MBIdx].CS, CAN_CS_CODE) >> CAN_CS_CODE_POS);
}

/**
 * @brief  Set Frame Identifier.
 * @rmtoll CS         EXT            LL_CAN_SetID
 *         CS         STD            LL_CAN_SetID
 *         CS         PRIO           LL_CAN_SetID
 * @note   For mailbox structure, If SMB.IDE is asserted, the ID is 29 bits
 *         (ID Standard + ID Extended). If SMB.IDE is negated, the ID is only
 *         11 bits (ID Standard). For Rx FIFO structure, ID depends on MCR.IDAM.
 * @param  CANx The CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @param  IDNum Between 0 to 0x3FFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetID(CAN_TypeDef *CANx, uint32_t MBIdx, uint32_t IDNum)
{
    MODIFY_REG(CANx->MB[MBIdx].ID, (CAN_ID_EXT | CAN_ID_STD | CAN_ID_PRIO), IDNum);
}

/**
 * @brief  Get Frame Identifier.
 * @rmtoll CS         EXT            LL_CAN_SetID
 *         CS         STD            LL_CAN_SetID
 *         CS         PRIO           LL_CAN_SetID
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval Between 0 to 0x3FFFFFFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetID(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    return (uint32_t)READ_BIT(CANx->MB[MBIdx].ID, (CAN_ID_EXT | CAN_ID_STD | CAN_ID_PRIO));
}

/**
 * @brief  Set the WORD0 field of the mailbox.
 * @rmtoll WORD0         WORD0            LL_CAN_SetWord0
 * @param  CANx  The CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @param  Value Between 0 to 0xFFFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetWord0(CAN_TypeDef *CANx, uint32_t MBIdx, uint32_t Value)
{
    WRITE_REG(CANx->MB[MBIdx].WORD0, Value);
}

/**
 * @brief  Get the WORD0 field of the mailbox.
 * @rmtoll WORD0         WORD0            LL_CAN_GetWord0
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetWord0(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    return READ_REG(CANx->MB[MBIdx].WORD0);
}

/**
 * @brief  Set the WORD1 field of the mailbox.
 * @rmtoll WORD1         WORD1            LL_CAN_SetWord1
 * @param  CANx  The CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @param  Value Between 0 to 0xFFFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetWord1(CAN_TypeDef *CANx, uint32_t MBIdx, uint32_t Value)
{
    WRITE_REG(CANx->MB[MBIdx].WORD1, Value);
}

/**
 * @brief  Get the WORD1 field of the mailbox.
 * @rmtoll WORD1         WORD1            LL_CAN_GetWord1
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetWord1(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    return READ_REG(CANx->MB[MBIdx].WORD1);
}

/**
 * @brief  Set the Rx Individual Mask.
 * @rmtoll RXIMR         MI            LL_CAN_SetRxIndividualMsk
 * @param  CANx CAN instance
 * @param  MaskIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @param  Mask Between 0 to 0xFFFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetRxIndividualMsk(CAN_TypeDef *CANx, uint32_t MaskIdx, uint32_t Mask)
{
    WRITE_REG(CANx->RXIMR[MaskIdx], Mask);
}

/**
 * @brief  Get the Rx Individual Mask.
 * @rmtoll RXIMR         MI            LL_CAN_GetRxIndividualMsk
 * @param  CANx CAN instance
 * @param  MaskIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetRxIndividualMsk(CAN_TypeDef *CANx, uint32_t MaskIdx)
{
    return READ_REG(CANx->RXIMR[MaskIdx]);
}

/**
 * @brief  Set the dividing frequency coefficient of the Ehanced Pre-allocated Bit Timing.
 * @rmtoll EPRS         ENPRESDIV            LL_CAN_SetEBitTimingFreqDiv
 * @param  CANx CAN instance
 * @param  FreqDiv Between 0 to 0x3FF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetEBitTimingFreqDiv(CAN_TypeDef *CANx, uint32_t FreqDiv)
{
    MODIFY_REG(CANx->EPRS, CAN_EPRS_ENPRESDIV, FreqDiv);
}

/**
 * @brief  Get the dividing frequency coefficient of the Ehanced Pre-allocated Bit Timing.
 * @rmtoll EPRS         ENPRESDIV            LL_CAN_GetEBitTimingFreqDiv
 * @param  CANx CAN instance
 * @retval Between 0 to 0x3FF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetEBitTimingFreqDiv(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->EPRS, CAN_EPRS_ENPRESDIV) >> CAN_EPRS_ENPRESDIV_POS);
}

/**
 * @brief  Set the number of the Ehanced Phase Buffer Segment 1.
 * @rmtoll ECBT         NTSEG1            LL_CAN_SetEPhaseSegment1
 * @note   Including the Propagation Time Segment(PTS) and Phase Buffer Segment 1:
 *                  NumberOfPTSandPSeg1 = NTSEG1 + 1
 * @param  CANx CAN instance
 * @param  Seg1Num Between 0 to 0xFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetEPhaseSegment1(CAN_TypeDef *CANx, uint32_t Seg1Num)
{
    MODIFY_REG(CANx->ECBT, CAN_ECBT_NTSEG1, Seg1Num);
}

/**
 * @brief  Get the number of the Ehanced Phase Buffer Segment 1.
 * @rmtoll ECBT         NTSEG1            LL_CAN_GetEPhaseSegment1
 * @param  CANx CAN instance
 * @retval Between 0 to 0xFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetEPhaseSegment1(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->ECBT, CAN_ECBT_NTSEG1) >> CAN_ECBT_NTSEG1_POS);
}

/**
 * @brief  Set the number of the Ehanced Phase Buffer Segment 2.
 * @rmtoll ECBT         NTSEG2            LL_CAN_SetEPhaseSegment2
 * @param  CANx CAN instance
 * @param  Seg2Num Between 0 to 0x7F.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetEPhaseSegment2(CAN_TypeDef *CANx, uint32_t Seg2Num)
{
    MODIFY_REG(CANx->ECBT, CAN_ECBT_NTSEG2, Seg2Num);
}

/**
 * @brief  Get the number of the Ehanced Phase Buffer Segment 2.
 * @rmtoll ECBT         NTSEG2            LL_CAN_GetEPhaseSegment2
 * @param  CANx CAN instance
 * @retval Between 0 to 0x7F.
 */
__STATIC_INLINE uint32_t LL_CAN_GetEPhaseSegment2(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->ECBT, CAN_ECBT_NTSEG2) >> CAN_ECBT_NTSEG2_POS);
}

/**
 * @brief  Set the number of the Ehanced Resync Jump Width.
 * @rmtoll ECBT         NRJW            LL_CAN_SetEResyncJumpWidth
 * @note   Defines the maximum number of Tq that a bit time can be changed by
 *         one re-synchronization:
 *                        NumberOfResyncJumpWidth = SJW + 1
 * @param  CANx CAN instance
 * @param  SJWNum Between 0 to 0x7F.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetEResyncJumpWidth(CAN_TypeDef *CANx, uint32_t SJWNum)
{
    MODIFY_REG(CANx->ECBT, CAN_ECBT_NRJW, SJWNum);
}

/**
 * @brief  Get the number of the Ehanced Resync Jump Width.
 * @rmtoll ECBT         NRJW            LL_CAN_GetEResyncJumpWidth
 * @param  CANx CAN instance
 * @retval Between 0 to 0x7F.
 */
__STATIC_INLINE uint32_t LL_CAN_GetEResyncJumpWidth(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->ECBT, CAN_ECBT_NRJW) >> CAN_ECBT_NRJW_POS);
}

/**
 * @brief  Set Rx DMA Beginning Address.
 * @rmtoll DMA_RBADR         ADDR            LL_CAN_SetRxDMABeginningAddr
 * @param  CANx CAN instance
 * @param  BeginningAddr Between 0 to 0x3FFFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetRxDMABeginningAddr(CAN_TypeDef *CANx, uint32_t BeginningAddr)
{
    MODIFY_REG(CANx->DMA_RBADR, CAN_DMA_RBADR_ADDR, BeginningAddr);
}

/**
 * @brief  Get Rx DMA Beginning Address.
 * @rmtoll DMA_RBADR         ADDR            LL_CAN_GetRxDMABeginningAddr
 * @param  CANx CAN instance
 * @retval Between 0 to 0x3FFFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetRxDMABeginningAddr(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->DMA_RBADR, CAN_DMA_RBADR_ADDR) >> CAN_DMA_RBADR_ADDR_POS);
}

/**
 * @brief  Set Rx DMA Data Length.
 * @rmtoll DMA_RLEN         BYTE            LL_CAN_SetRxDMADataLength
 * @param  CANx CAN instance
 * @param  DataLength Between 0 to 0x3FFFF.
 * @retval None
 */
__STATIC_INLINE void LL_CAN_SetRxDMADataLength(CAN_TypeDef *CANx, uint32_t DataLength)
{
    MODIFY_REG(CANx->DMA_RLEN, CAN_DMA_RLEN_LEN, DataLength);
}

/**
 * @brief  Get Rx DMA Data Length.
 * @rmtoll DMA_RLEN         BYTE            LL_CAN_GetRxDMADataLength
 * @param  CANx CAN instance
 * @retval Between 0 to 0x3FFFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetRxDMADataLength(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->DMA_RLEN, CAN_DMA_RLEN_LEN) >> CAN_DMA_RLEN_LEN_POS);
}

/**
 * @brief  Get Rx DMA Current Address.
 * @rmtoll DMA_RADR         ADDR            LL_CAN_GetRxDMACurrentAddr
 * @param  CANx CAN instance
 * @retval Between 0 to 0x3FFFF.
 */
__STATIC_INLINE uint32_t LL_CAN_GetRxDMACurrentAddr(CAN_TypeDef *CANx)
{
    return (READ_BIT(CANx->DMA_RADR, CAN_DMA_RADR_ADDR) >> CAN_DMA_RADR_ADDR_POS);
}

/**
 * @}
 */

/** @defgroup CAN_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable Error Warning interrupt.
 * @rmtoll MCR         WRNEN         LL_CAN_EnableIT_WRN
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableIT_WRN(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_WRNEN);
}

/**
 * @brief  Disable Error Warning interrupt.
 * @rmtoll MCR         WRNEN         LL_CAN_DisableIT_WRN
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableIT_WRN(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->MCR, CAN_MCR_WRNEN);
}

/**
 * @brief  Check if Error Warning interrupt is enabled or not.
 * @rmtoll MCR         WRNEN         LL_CAN_IsEnabledIT_WRN
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledIT_WRN(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_WRNEN) == CAN_MCR_WRNEN) ? 1UL : 0UL);
}

/**
 * @brief  Enable Self-awakening interrupt.
 * @rmtoll MCR         WAK_IE         LL_CAN_EnableIT_WAK
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableIT_WAK(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->MCR, CAN_MCR_WAKMSK);
}

/**
 * @brief  Disable Self-awakening interrupt.
 * @rmtoll MCR         WAK_IE         LL_CAN_DisableIT_WAK
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableIT_WAK(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->MCR, CAN_MCR_WAKMSK);
}

/**
 * @brief  Check if Self-awakening interrupt is enabled or not.
 * @rmtoll MCR         WAK_IE         LL_CAN_IsEnabledIT_WAK
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledIT_WAK(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->MCR, CAN_MCR_WAKMSK) == CAN_MCR_WAKMSK) ? 1UL : 0UL);
}

/**
 * @brief  Enable Rx Warning interrupt.
 * @rmtoll CTRL1         RWRN_IE         LL_CAN_EnableIT_RWRN
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableIT_RWRN(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->CTRL1, CAN_CTRL1_RWRNMSK);
}

/**
 * @brief  Disable Rx Warning interrupt.
 * @rmtoll CTRL1         RWRN_IE         LL_CAN_DisableIT_RWRN
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableIT_RWRN(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->CTRL1, CAN_CTRL1_RWRNMSK);
}

/**
 * @brief  Check if Rx Warning interrupt is enabled or not.
 * @rmtoll CTRL1         RWRN_IE         LL_CAN_IsEnabledIT_RWRN
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledIT_RWRN(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->CTRL1, CAN_CTRL1_RWRNMSK) == CAN_CTRL1_RWRNMSK) ? 1UL : 0UL);
}

/**
 * @brief  Enable Tx Warning interrupt.
 * @rmtoll CTRL1         TWRN_IE         LL_CAN_EnableIT_TWRN
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableIT_TWRN(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->CTRL1, CAN_CTRL1_TWRNMSK);
}

/**
 * @brief  Disable Tx Warning interrupt.
 * @rmtoll CTRL1         TWRN_IE         LL_CAN_DisableIT_TWRN
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableIT_TWRN(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->CTRL1, CAN_CTRL1_TWRNMSK);
}

/**
 * @brief  Check if Tx Warning interrupt is enabled or not.
 * @rmtoll CTRL1         TWRN_IE         LL_CAN_IsEnabledIT_TWRN
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledIT_TWRN(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->CTRL1, CAN_CTRL1_TWRNMSK) == CAN_CTRL1_TWRNMSK) ? 1UL : 0UL);
}

/**
 * @brief  Enable Error interrupt.
 * @rmtoll CTRL1         ERR_IE         LL_CAN_EnableIT_ERR
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableIT_ERR(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->CTRL1, CAN_CTRL1_ERRMSK);
}

/**
 * @brief  Disable Error interrupt.
 * @rmtoll CTRL1         ERR_IE         LL_CAN_DisableIT_ERR
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableIT_ERR(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->CTRL1, CAN_CTRL1_ERRMSK);
}

/**
 * @brief  Check if Error interrupt is enabled or not.
 * @rmtoll CTRL1         ERR_IE         LL_CAN_IsEnabledIT_ERR
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledIT_ERR(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->CTRL1, CAN_CTRL1_ERRMSK) == CAN_CTRL1_ERRMSK) ? 1UL : 0UL);
}

/**
 * @brief  Enable Bus Off interrupt.
 * @rmtoll CTRL1         BOFFREN         LL_CAN_EnableIT_BOFF
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableIT_BOFF(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->CTRL1, CAN_CTRL1_BOFFMSK);
}

/**
 * @brief  Disable Bus Off interrupt.
 * @rmtoll CTRL1         BOFFREN         LL_CAN_DisableIT_BOFF
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableIT_BOFF(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->CTRL1, CAN_CTRL1_BOFFMSK);
}

/**
 * @brief  Check if Bus Off interrupt is enabled or not.
 * @rmtoll CTRL1         BOFFREN         LL_CAN_IsEnabledIT_BOFF
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledIT_BOFF(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->CTRL1, CAN_CTRL1_BOFFMSK) == CAN_CTRL1_BOFFMSK) ? 1UL : 0UL);
}

/**
 * @brief  Enable the corresponding Mailboxes interrupt.
 * @rmtoll IMASK1         BUF15TO0M         LL_CAN_EnableIT_MB
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableIT_MB(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    SET_BIT(CANx->IMASK1, MBIdx);
}

/**
 * @brief  Disable the corresponding Mailboxes interrupt.
 * @rmtoll IMASK1         BUF15TO0M         LL_CAN_DisableIT_MB
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableIT_MB(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    CLEAR_BIT(CANx->IMASK1, MBIdx);
}

/**
 * @brief  Check if the corresponding Mailboxes interrupt is enabled or not.
 * @rmtoll IMASK1         BUF15TO0M         LL_CAN_IsEnabledIT_MB
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledIT_MB(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    return ((READ_BIT(CANx->IMASK1, MBIdx) == MBIdx) ? 1UL : 0UL);
}

/**
 * @brief  Enable Bus Off Done interrupt.
 * @rmtoll CTRL2         BOFFD_IE         LL_CAN_EnableIT_BOFFD
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableIT_BOFFD(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->CTRL2, CAN_CTRL2_BOFFDONEMSK);
}

/**
 * @brief  Disable Bus Off Done interrupt.
 * @rmtoll CTRL2         BOFFD_IE         LL_CAN_DisableIT_BOFFD
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableIT_BOFFD(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->CTRL2, CAN_CTRL2_BOFFDONEMSK);
}

/**
 * @brief  Check if Bus Off Done interrupt is enabled or not.
 * @rmtoll CTRL2         BOFFD_IE         LL_CAN_IsEnabledIT_BOFFD
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledIT_BOFFD(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->CTRL2, CAN_CTRL2_BOFFDONEMSK) == CAN_CTRL2_BOFFDONEMSK) ? 1UL : 0UL);
}

/**
 * @brief  Enable Timer Overrun interrupt.
 * @rmtoll ESR2         TWA_IE         LL_CAN_EnableIT_TWA
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableIT_TWA(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->ESR2, CAN_ESR2_TWAMSK);
}

/**
 * @brief  Disable Timer Overrun interrupt.
 * @rmtoll ESR2         TWA_IE         LL_CAN_DisableIT_TWA
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableIT_TWA(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->ESR2, CAN_ESR2_TWAMSK);
}

/**
 * @brief  Check if Timer Overrun interrupt is enabled or not.
 * @rmtoll ESR2         TWA_IE         LL_CAN_IsEnabledIT_TWA
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledIT_TWA(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR2, CAN_ESR2_TWAMSK) == CAN_ESR2_TWAMSK) ? 1UL : 0UL);
}

/**
 * @brief  Enable DMA half tansfer interrupt.
 * @rmtoll DMA_IE         RX_HIE         LL_CAN_EnableIT_RXHT
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableIT_RXHT(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->DMA_IE, CAN_DMA_IE_RXHFULL);
}

/**
 * @brief  Disable DMA half tansfer interrupt.
 * @rmtoll DMA_IE         RX_HIE         LL_CAN_DisableIT_RXHT
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableIT_RXHT(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->DMA_IE, CAN_DMA_IE_RXHFULL);
}

/**
 * @brief  Check if DMA half tansfer interrupt is enabled or not.
 * @rmtoll DMA_IE         RX_HIE         LL_CAN_IsEnabledIT_RXHT
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledIT_RXHT(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->DMA_IE, CAN_DMA_IE_RXHFULL) == CAN_DMA_IE_RXHFULL) ? 1UL : 0UL);
}

/**
 * @brief  Enable DMA tansfer complete interrupt.
 * @rmtoll DMA_IE         RX_FIE         LL_CAN_EnableIT_RXTC
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_EnableIT_RXTC(CAN_TypeDef *CANx)
{
    SET_BIT(CANx->DMA_IE, CAN_DMA_IE_RXFULL);
}

/**
 * @brief  Disable DMA tansfer complete interrupt.
 * @rmtoll DMA_IE         RX_FIE         LL_CAN_DisableIT_RXTC
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_DisableIT_RXTC(CAN_TypeDef *CANx)
{
    CLEAR_BIT(CANx->DMA_IE, CAN_DMA_IE_RXFULL);
}

/**
 * @brief  Check if DMA tansfer complete interrupt is enabled or not.
 * @rmtoll DMA_IE         RX_FIE         LL_CAN_IsEnabledIT_RXTC
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsEnabledIT_RXTC(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->DMA_IE, CAN_DMA_IE_RXFULL) == CAN_DMA_IE_RXFULL) ? 1UL : 0UL);
}

/**
 * @}
 */

/** @defgroup CAN_LL_EF_FLAG_Management FLAG_Management
 * @{
 */

/**
 * @brief  Check if Error and Status flag is set or not.
 * @rmtoll ESR1         WAKINT         LL_CAN_IsActiveFlag
 *         ESR1         ERRINT         LL_CAN_IsActiveFlag
 *         ESR1         BOFFINT        LL_CAN_IsActiveFlag
 *         ESR1         FLTCONF        LL_CAN_IsActiveFlag
 *         ESR1         IDLE           LL_CAN_IsActiveFlag
 *         ESR1         RXWRN          LL_CAN_IsActiveFlag
 *         ESR1         TXWRN          LL_CAN_IsActiveFlag
 *         ESR1         BIT0ERR        LL_CAN_IsActiveFlag
 *         ESR1         BIT1ERR        LL_CAN_IsActiveFlag
 *         ESR1         RWRNINT        LL_CAN_IsActiveFlag
 *         ESR1         TWRNINT        LL_CAN_IsActiveFlag
 *         ESR1         SYNCH          LL_CAN_IsActiveFlag
 *         ESR1         BOFFDONEINT    LL_CAN_IsActiveFlag
 *         ESR1         ERROVR         LL_CAN_IsActiveFlag
 * @param  CANx CAN instance
 * @param  Flag This parameter can be one of the following values:
 *         @arg @ref LL_CAN_FLAG_SYNCH
 *         @arg @ref LL_CAN_FLAG_TXWARNINGINT
 *         @arg @ref LL_CAN_FLAG_RXWARNINGINT
 *         @arg @ref LL_CAN_FLAG_TXERRORWARNING
 *         @arg @ref LL_CAN_FLAG_RXERRORWARNING
 *         @arg @ref LL_CAN_FLAG_IDLE
 *         @arg @ref LL_CAN_FLAG_FAULTCONFINEMENT
 *         @arg @ref LL_CAN_FLAG_TRANSMITTING
 *         @arg @ref LL_CAN_FLAG_RECEIVING
 *         @arg @ref LL_CAN_FLAG_BUSOFFINT
 *         @arg @ref LL_CAN_FLAG_ERRORINT
 *         @arg @ref LL_CAN_FLAG_WAKEUPINT
 *
 *         @arg @ref LL_CAN_ERROR_STUFFING
 *         @arg @ref LL_CAN_ERROR_FORM
 *         @arg @ref LL_CAN_ERROR_CAN
 *         @arg @ref LL_CAN_ERROR_ACK
 *         @arg @ref LL_CAN_ERROR_BIT0
 *         @arg @ref LL_CAN_ERROR_BIT1
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag(CAN_TypeDef *CANx, uint32_t Flag)
{
    return ((READ_BIT(CANx->ESR1, Flag) == Flag) ? 1UL : 0UL);
}

/**
 * @brief  Clear Error and Status flag.
 * @rmtoll ESR1         WAKINT         LL_CAN_IsActiveFlag
 *         ESR1         ERRINT         LL_CAN_IsActiveFlag
 *         ESR1         BOFFINT        LL_CAN_IsActiveFlag
 *         ESR1         FLTCONF        LL_CAN_IsActiveFlag
 *         ESR1         IDLE           LL_CAN_IsActiveFlag
 *         ESR1         RXWRN          LL_CAN_IsActiveFlag
 *         ESR1         TXWRN          LL_CAN_IsActiveFlag
 *         ESR1         BIT0ERR        LL_CAN_IsActiveFlag
 *         ESR1         BIT1ERR        LL_CAN_IsActiveFlag
 *         ESR1         RWRNINT        LL_CAN_IsActiveFlag
 *         ESR1         TWRNINT        LL_CAN_IsActiveFlag
 *         ESR1         SYNCH          LL_CAN_IsActiveFlag
 *         ESR1         BOFFDONEINT    LL_CAN_IsActiveFlag
 *         ESR1         ERROVR         LL_CAN_IsActiveFlag
 * @param  CANx CAN instance
 * @param  Flag This parameter can be one of the following values:
 *         @arg @ref LL_CAN_FLAG_SYNCH
 *         @arg @ref LL_CAN_FLAG_TX_WARNINGINT
 *         @arg @ref LL_CAN_FLAG_RX_WARNINGINT
 *         @arg @ref LL_CAN_FLAG_TXERRORWARNING
 *         @arg @ref LL_CAN_FLAG_RXERRORWARNING
 *         @arg @ref LL_CAN_FLAG_IDLE
 *         @arg @ref LL_CAN_FLAG_FAULTCONFINEMENT
 *         @arg @ref LL_CAN_FLAG_TRANSMITTING
 *         @arg @ref LL_CAN_FLAG_RECEIVING
 *         @arg @ref LL_CAN_FLAG_BUSOFFINT
 *         @arg @ref LL_CAN_FLAG_ERRORINT
 *         @arg @ref LL_CAN_FLAG_WAKEUPINT
 *
 *         @arg @ref LL_CAN_ERROR_STUFFING
 *         @arg @ref LL_CAN_ERROR_FORM
 *         @arg @ref LL_CAN_ERROR_CAN
 *         @arg @ref LL_CAN_ERROR_ACK
 *         @arg @ref LL_CAN_ERROR_BIT0
 *         @arg @ref LL_CAN_ERROR_BIT1
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag(CAN_TypeDef *CANx, uint32_t Flag)
{
    WRITE_REG(CANx->ESR1, Flag);
}

/**
 * @brief  Check if Wake-up interrupt flag is set or not.
 * @rmtoll ESR1         WAKINT         LL_CAN_IsActiveFlag_WAKINT
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_WAKINT(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_WAKINT) == CAN_ESR1_WAKINT) ? 1UL : 0UL);
}

/**
 * @brief  Clear Wake-up interrupt flag.
 * @rmtoll ESR1         WAKINT         LL_CAN_ClearFlag_WAKINT
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_WAKINT(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->ESR1, CAN_ESR1_WAKINT);
}

/**
 * @brief  Check if Error interrupt flag is set or not.
 * @rmtoll ESR1         ERRINT         LL_CAN_IsActiveFlag_ERRINT
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_ERRINT(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_ERRINT) == CAN_ESR1_ERRINT) ? 1UL : 0UL);
}

/**
 * @brief  Clear Error interrupt flag.
 * @rmtoll ESR1         ERRINT         LL_CAN_ClearFlag_ERRINT
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_ERRINT(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->ESR1, CAN_ESR1_ERRINT);
}

/**
 * @brief  Check if Bus Off interrupt flag is set or not.
 * @rmtoll ESR1         BOFFINT         LL_CAN_IsActiveFlag_BOFFINT
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_BOFFINT(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_BOFFINT) == CAN_ESR1_BOFFINT) ? 1UL : 0UL);
}

/**
 * @brief  Clear Bus Off interrupt flag.
 * @rmtoll ESR1         BOFFINT         LL_CAN_ClearFlag_BOFFINT
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_BOFFINT(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->ESR1, CAN_ESR1_BOFFINT);
}

/**
 * @brief  Check if FlexCAN Rx State flag is set or not.
 * @rmtoll ESR1         RX         LL_CAN_IsActiveFlag_RX
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_RX(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_RX) == CAN_ESR1_RX) ? 1UL : 0UL);
}

/**
 * @brief  Check if Fault State flag is set or not.
 * @rmtoll ESR1         FLTCONF         LL_CAN_GetFaultStateFlag
 * @param  CANx CAN instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_CAN_STA_ERROR_ACTIVE
 *         @arg @ref LL_CAN_STA_ERROR_PASSIVE
 *         @arg @ref LL_CAN_STA_BUS_OFF_0
 *         @arg @ref LL_CAN_STA_BUS_OFF_1
 */
__STATIC_INLINE uint32_t LL_CAN_GetFaultStateFlag(CAN_TypeDef *CANx)
{
    return (uint32_t)READ_BIT(CANx->ESR1, CAN_ESR1_FLTCONF);
}

/**
 * @brief  Check if FlexCAN Tx State flag is set or not.
 * @rmtoll ESR1         TX         LL_CAN_IsActiveFlag_TX
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_TX(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_TX) == CAN_ESR1_TX) ? 1UL : 0UL);
}

/**
 * @brief  Check if Bus State flag is set or not.
 * @rmtoll ESR1         IDLE         LL_CAN_IsActiveFlag_IDLE
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_IDLE(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_IDLE) == CAN_ESR1_IDLE) ? 1UL : 0UL);
}

/**
 * @brief  Check if Rx Error Warning flag is set or not.
 * @rmtoll ESR1         RXWRN         LL_CAN_IsActiveFlag_RXWRN
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_RXWRN(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_RXWRN) == CAN_ESR1_RXWRN) ? 1UL : 0UL);
}

/**
 * @brief  Check if Tx Error Warning flag is set or not.
 * @rmtoll ESR1         TXWRN         LL_CAN_IsActiveFlag_TXWRN
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_TXWRN(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_TXWRN) == CAN_ESR1_TXWRN) ? 1UL : 0UL);
}

/**
 * @brief  Check if Stuffing Error flag is set or not.
 * @rmtoll ESR1         STFERR         LL_CAN_IsActiveFlag_STFERR
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_STFERR(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_STFERR) == CAN_ESR1_STFERR) ? 1UL : 0UL);
}

/**
 * @brief  Check if Form Error flag is set or not.
 * @rmtoll ESR1         FRMERR         LL_CAN_IsActiveFlag_FRMERR
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_FRMERR(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_FRMERR) == CAN_ESR1_FRMERR) ? 1UL : 0UL);
}

/**
 * @brief  Check if Cyclic Redundancy Check Error flag is set or not.
 * @rmtoll ESR1         CRCERR         LL_CAN_IsActiveFlag_CRCERR
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_CRCERR(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_CRCERR) == CAN_ESR1_CRCERR) ? 1UL : 0UL);
}

/**
 * @brief  Check if Acknowledge Error flag is set or not.
 * @rmtoll ESR1         ACKERR         LL_CAN_IsActiveFlag_ACKERR
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_ACKERR(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_ACKERR) == CAN_ESR1_ACKERR) ? 1UL : 0UL);
}

/**
 * @brief  Check if Bit0 Error flag is set or not.
 * @rmtoll ESR1         BIT0ERR         LL_CAN_IsActiveFlag_BIT0ERR
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_BIT0ERR(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_BIT0ERR) == CAN_ESR1_BIT0ERR) ? 1UL : 0UL);
}

/**
 * @brief  Check if Bit1 Error flag is set or not.
 * @rmtoll ESR1         BIT1ERR         LL_CAN_IsActiveFlag_BIT1ERR
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_BIT1ERR(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_BIT1ERR) == CAN_ESR1_BIT1ERR) ? 1UL : 0UL);
}

/**
 * @brief  Check if Rx Warning interrupt flag is set or not.
 * @rmtoll ESR1         RWRNINT         LL_CAN_IsActiveFlag_RWRNINT
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_RWRNINT(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_RWRNINT) == CAN_ESR1_RWRNINT) ? 1UL : 0UL);
}

/**
 * @brief  Clear Rx Warning interrupt flag.
 * @rmtoll ESR1         RWRNINT         LL_CAN_ClearFlag_RWRNINT
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_RWRNINT(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->ESR1, CAN_ESR1_RWRNINT);
}

/**
 * @brief  Check if Tx Warning interrupt flag is set or not.
 * @rmtoll ESR1         TWRNINT         LL_CAN_IsActiveFlag_TWRNINT
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_TWRNINT(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_TWRNINT) == CAN_ESR1_TWRNINT) ? 1UL : 0UL);
}

/**
 * @brief  Clear Tx Warning interrupt flag.
 * @rmtoll ESR1         TWRNINT         LL_CAN_ClearFlag_TWRNINT
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_TWRNINT(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->ESR1, CAN_ESR1_TWRNINT);
}

/**
 * @brief  Check if FlexCAN Synchronization Status flag is set or not.
 * @rmtoll ESR1         SYNCH         LL_CAN_IsActiveFlag_SYNCH
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_SYNCH(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_SYNCH) == CAN_ESR1_SYNCH) ? 1UL : 0UL);
}

/**
 * @brief  Check if Bus Off Done interrupt flag is set or not.
 * @rmtoll ESR1         BOFFDONEINT         LL_CAN_IsActiveFlag_BOFFDONEINT
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_BOFFDONEINT(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_BOFFDONEINT) == CAN_ESR1_BOFFDONEINT) ? 1UL : 0UL);
}

/**
 * @brief  Clear Bus Off Done interrupt flag.
 * @rmtoll ESR1         BOFFDONEINT         LL_CAN_ClearFlag_BOFFDONEINT
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_BOFFDONEINT(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->ESR1, CAN_ESR1_BOFFDONEINT);
}

/**
 * @brief  Check if Error Overrun flag is set or not.
 * @rmtoll ESR1         ERROVR         LL_CAN_IsActiveFlag_ERROVR
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_ERROVR(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR1, CAN_ESR1_ERROVR) == CAN_ESR1_ERROVR) ? 1UL : 0UL);
}

/**
 * @brief  Clear Error Overrun flag.
 * @rmtoll ESR1         ERROVR         LL_CAN_ClearFlag_ERROVR
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_ERROVR(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->ESR1, CAN_ESR1_ERROVR);
}

/**
 * @brief  Check if Mailbox interrupt flag is set or not.
 * @rmtoll IFLAG1         BUF15TO8I     LL_CAN_IsActiveFlag_MB
 *         IFLAG1         BUF7I         LL_CAN_IsActiveFlag_MB
 *         IFLAG1         BUF6I         LL_CAN_IsActiveFlag_MB
 *         IFLAG1         BUF5I         LL_CAN_IsActiveFlag_MB
 *         IFLAG1         BUF4I         LL_CAN_IsActiveFlag_MB
 *         IFLAG1         BUF3I         LL_CAN_IsActiveFlag_MB
 *         IFLAG1         BUF2I         LL_CAN_IsActiveFlag_MB
 *         IFLAG1         BUF1I         LL_CAN_IsActiveFlag_MB
 *         IFLAG1         BUF0I         LL_CAN_IsActiveFlag_MB
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_MB(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    return ((READ_BIT(CANx->IFLAG1, MBIdx) == MBIdx) ? 1UL : 0UL);
}

/**
 * @brief  Clear Mailbox interrupt flag.
 * @rmtoll IFLAG1         BUF15TO8I     LL_CAN_ClearFlag_MB
 *         IFLAG1         BUF7I         LL_CAN_ClearFlag_MB
 *         IFLAG1         BUF6I         LL_CAN_ClearFlag_MB
 *         IFLAG1         BUF5I         LL_CAN_ClearFlag_MB
 *         IFLAG1         BUF4I         LL_CAN_ClearFlag_MB
 *         IFLAG1         BUF3I         LL_CAN_ClearFlag_MB
 *         IFLAG1         BUF2I         LL_CAN_ClearFlag_MB
 *         IFLAG1         BUF1I         LL_CAN_ClearFlag_MB
 *         IFLAG1         BUF0I         LL_CAN_ClearFlag_MB
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_MB(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    WRITE_REG(CANx->IFLAG1, MBIdx);
}

/**
 * @brief  Get Mailbox interrupt flag.
 * @rmtoll IFLAG1         BUF15TO8I     LL_CAN_ReadFlag_MB
 *         IFLAG1         BUF7I         LL_CAN_ReadFlag_MB
 *         IFLAG1         BUF6I         LL_CAN_ReadFlag_MB
 *         IFLAG1         BUF5I         LL_CAN_ReadFlag_MB
 *         IFLAG1         BUF4I         LL_CAN_ReadFlag_MB
 *         IFLAG1         BUF3I         LL_CAN_ReadFlag_MB
 *         IFLAG1         BUF2I         LL_CAN_ReadFlag_MB
 *         IFLAG1         BUF1I         LL_CAN_ReadFlag_MB
 *         IFLAG1         BUF0I         LL_CAN_ReadFlag_MB
 * @param  CANx CAN instance
 * @param  MBIdx This parameter can be one of the following values:
 *         @arg @ref LL_CAN_MB0_MASK
 *         @arg @ref LL_CAN_MB1_MASK
 *         @arg @ref LL_CAN_MB2_MASK
 *         @arg @ref LL_CAN_MB3_MASK
 *         @arg @ref LL_CAN_MB4_MASK
 *         @arg @ref LL_CAN_MB5_MASK
 *         @arg @ref LL_CAN_MB6_MASK
 *         @arg @ref LL_CAN_MB7_MASK
 *         @arg @ref LL_CAN_MB15TO8_MASK
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_ReadFlag_MB(CAN_TypeDef *CANx, uint32_t MBIdx)
{
    return (uint32_t)(READ_BIT(CANx->IFLAG1, MBIdx));
}

/**
 * @brief  Check if MB0 interrupt or Clear FIFO bit flag is set or not.
 * @rmtoll IFLAG1         BUF0I         LL_CAN_IsActiveFlag_BUF0I
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_BUF0I(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->IFLAG1, CAN_IFLAG1_BUF0I) == CAN_IFLAG1_BUF0I) ? 1UL : 0UL);
}

/**
 * @brief  Clear MB0 interrupt or Clear FIFO bit flag.
 * @rmtoll IFLAG1         BUF0I         LL_CAN_ClearFlag_BUF0I
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_BUF0I(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->IFLAG1, CAN_IFLAG1_BUF0I);
}

/**
 * @brief  Check if MB1 interrupt or Rx FIFO Reserved bit flag is set or not.
 * @rmtoll IFLAG1         BUF1I         LL_CAN_IsActiveFlag_BUF1I
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_BUF1I(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->IFLAG1, CAN_IFLAG1_BUF1I) == CAN_IFLAG1_BUF1I) ? 1UL : 0UL);
}

/**
 * @brief  Clear MB1 interrupt or Rx FIFO Reserved bit flag.
 * @rmtoll IFLAG1         BUF1I         LL_CAN_ClearFlag_BUF1I
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_BUF1I(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->IFLAG1, CAN_IFLAG1_BUF1I);
}

/**
 * @brief  Check if MB2 interrupt or Rx FIFO Reserved bit flag is set or not.
 * @rmtoll IFLAG1         BUF2I         LL_CAN_IsActiveFlag_BUF2I
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_BUF2I(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->IFLAG1, CAN_IFLAG1_BUF2I) == CAN_IFLAG1_BUF2I) ? 1UL : 0UL);
}

/**
 * @brief  Clear MB2 interrupt or Rx FIFO Reserved bit flag.
 * @rmtoll IFLAG1         BUF2I         LL_CAN_ClearFlag_BUF2I
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_BUF2I(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->IFLAG1, CAN_IFLAG1_BUF2I);
}

/**
 * @brief  Check if MB3 interrupt or Rx FIFO Reserved bit flag is set or not.
 * @rmtoll IFLAG1         BUF3I         LL_CAN_IsActiveFlag_BUF3I
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_BUF3I(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->IFLAG1, CAN_IFLAG1_BUF3I) == CAN_IFLAG1_BUF3I) ? 1UL : 0UL);
}

/**
 * @brief  Clear MB3 interrupt or Rx FIFO Reserved bit flag.
 * @rmtoll IFLAG1         BUF3I         LL_CAN_ClearFlag_BUF3I
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_BUF3I(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->IFLAG1, CAN_IFLAG1_BUF3I);
}

/**
 * @brief  Check if MB4 interrupt or Rx FIFO Reserved bit flag is set or not.
 * @rmtoll IFLAG1         BUF4I         LL_CAN_IsActiveFlag_BUF4I
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_BUF4I(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->IFLAG1, CAN_IFLAG1_BUF4I) == CAN_IFLAG1_BUF4I) ? 1UL : 0UL);
}

/**
 * @brief  Clear MB4 interrupt or Rx FIFO Reserved bit flag.
 * @rmtoll IFLAG1         BUF4I         LL_CAN_ClearFlag_BUF4I
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_BUF4I(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->IFLAG1, CAN_IFLAG1_BUF4I);
}

/**
 * @brief  Check if MB5 interrupt or Frames Avaliable in Rx FIFO interrupt flag is set or not.
 * @rmtoll IFLAG1         BUF5I         LL_CAN_IsActiveFlag_BUF5I
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_BUF5I(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->IFLAG1, CAN_IFLAG1_BUF5I) == CAN_IFLAG1_BUF5I) ? 1UL : 0UL);
}

/**
 * @brief  Clear MB5 interrupt or Frames Avaliable in Rx FIFO interrupt flag.
 * @rmtoll IFLAG1         BUF5I         LL_CAN_ClearFlag_BUF5I
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_BUF5I(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->IFLAG1, CAN_IFLAG1_BUF5I);
}

/**
 * @brief  Check if MB6 interrupt or Rx FIFO Warning flag is set or not.
 * @rmtoll IFLAG1         BUF6I         LL_CAN_IsActiveFlag_BUF6I
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_BUF6I(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->IFLAG1, CAN_IFLAG1_BUF6I) == CAN_IFLAG1_BUF6I) ? 1UL : 0UL);
}

/**
 * @brief  Clear MB6 interrupt or Rx FIFO Warning flag.
 * @rmtoll IFLAG1         BUF6I         LL_CAN_ClearFlag_BUF6I
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_BUF6I(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->IFLAG1, CAN_IFLAG1_BUF6I);
}

/**
 * @brief  Check if MB7 interrupt or Rx FIFO Overflow flag is set or not.
 * @rmtoll IFLAG1         BUF7I         LL_CAN_IsActiveFlag_BUF7I
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_BUF7I(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->IFLAG1, CAN_IFLAG1_BUF7I) == CAN_IFLAG1_BUF7I) ? 1UL : 0UL);
}

/**
 * @brief  Clear MB7 interrupt or Rx FIFO Overflow flag.
 * @rmtoll IFLAG1         BUF7I         LL_CAN_ClearFlag_BUF7I
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_BUF7I(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->IFLAG1, CAN_IFLAG1_BUF7I);
}

/**
 * @brief  Check if MB8~15 interrupt flag is set or not.
 * @rmtoll IFLAG1         BUF15TO8I         LL_CAN_IsActiveFlag_BUF15TO8I
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_BUF15TO8I(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->IFLAG1, CAN_IFLAG1_BUF15TO8I) == CAN_IFLAG1_BUF15TO8I) ? 1UL : 0UL);
}

/**
 * @brief  Clear MB8~15 interrupt flag.
 * @rmtoll IFLAG1         BUF15TO8I         LL_CAN_ClearFlag_BUF15TO8I
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_BUF15TO8I(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->IFLAG1, CAN_IFLAG1_BUF15TO8I);
}

/**
 * @brief  Check if Inactive Mailbox flag is set or not.
 * @rmtoll ESR2         IMB         LL_CAN_IsActiveFlag_IMB
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0):
 *         return 1: If ESR2.VPS is asserted, there is at least one inactive mailbox.
 *         return 0: If ESR2.VPS is asserted, ESR2.LPTM is not an inactive mailbox.
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_IMB(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR2, CAN_ESR2_IMB) == CAN_ESR2_IMB) ? 1UL : 0UL);
}

/**
 * @brief  Indicates whether ESR2.IMB and ESR2.LPTM contents are currently valid or not.
 * @rmtoll ESR2         VPS         LL_CAN_IsActiveFlag_VPS
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0):
 *         return 1: Contents of IMB and LPTM are valid.
 *         return 0: Contents of IMB and LPTM are invalid.
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_VPS(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR2, CAN_ESR2_VPS) == CAN_ESR2_VPS) ? 1UL : 0UL);
}

/**
 * @brief  Check if Timer Overrun interrupt flag is set or not.
 * @rmtoll ESR2         TWAINT         LL_CAN_IsActiveFlag_TWAINT
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_TWAINT(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->ESR2, CAN_ESR2_TWAINT) == CAN_ESR2_TWAINT) ? 1UL : 0UL);
}

/**
 * @brief  Clear Timer Overrun interrupt flag.
 * @rmtoll ESR2         TWAINT         LL_CAN_ClearFlag_TWAINT
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_TWAINT(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->ESR2, CAN_ESR2_TWAINT);
}

/**
 * @brief  Check if DMA half transfer flag is set or not.
 * @rmtoll DMA_IF         RXHT         LL_CAN_IsActiveFlag_RXHT
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_RXHT(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->DMA_IF, CAN_DMA_IF_RXHFULL) == CAN_DMA_IF_RXHFULL) ? 1UL : 0UL);
}

/**
 * @brief  Clear DMA half transfer flag.
 * @rmtoll DMA_IF         RXHT         LL_CAN_ClearFlag_RXHT
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_RXHT(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->DMA_IF, CAN_DMA_IF_RXHFULL);
}

/**
 * @brief  Check if DMA transfer complete flag is set or not.
 * @rmtoll DMA_IF         RX_FDONE         LL_CAN_IsActiveFlag_RX_FDONE
 * @param  CANx CAN instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CAN_IsActiveFlag_RXTC(CAN_TypeDef *CANx)
{
    return ((READ_BIT(CANx->DMA_IF, CAN_DMA_IF_RXFULL) == CAN_DMA_IF_RXFULL) ? 1UL : 0UL);
}

/**
 * @brief  Clear DMA transfer complete flag.
 * @rmtoll DMA_IF         RX_FDONE         LL_CAN_ClearFlag_RX_FDONE
 * @param  CANx CAN instance
 * @retval None
 */
__STATIC_INLINE void LL_CAN_ClearFlag_RXTC(CAN_TypeDef *CANx)
{
    WRITE_REG(CANx->DMA_IF, CAN_DMA_IF_RXFULL);
}

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#endif /* CAN */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_CAN_H */
