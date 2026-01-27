/**
 *****************************************************************************
 * @file   rn_rv_ll_lpuart.h
 * @author Renergy Software Team
 * @brief  Header file of LPUART LL module.
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
#ifndef __RN_LL_LPUART_H
#define __RN_LL_LPUART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_lpuart.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (LPUART)

/** @defgroup LPUART_LL LPUART
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup LPUART_LL_Private_Constants LPUART Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup LPUART_LL_Private_Macros LPUART Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup LPUART_LL_ES_INIT LPUART Exported Init structure
 * @{
 */

/**
 * @brief LL LPUART Init Structure definition
 */
typedef struct
{
    uint32_t BaudRate;                  /*!< This field defines expected Usart communication baud rate.
                                             This parameter can be a value of @ref LPUART_LL_EC_BAUDRATE.

                                             This feature can be modified afterwards using unitary function @ref LL_LPUART_SetBaudRate().*/

    uint32_t DataWidth;                 /*!< Specifies the number of data bits transmitted or received in a frame.
                                             This parameter can be a value of @ref LPUART_LL_EC_DATAWIDTH.

                                             This feature can be modified afterwards using unitary function @ref LL_LPUART_SetDataWidth().*/

    uint32_t StopBits;                  /*!< Specifies the number of stop bits transmitted.
                                             This parameter can be a value of @ref LPUART_LL_EC_STOPBITS.

                                             This feature can be modified afterwards using unitary function @ref LL_LPUART_SetStopBitsLength().*/

    uint32_t Parity;                    /*!< Specifies the parity mode.
                                             This parameter can be a value of @ref LPUART_LL_EC_PARITY.

                                             This feature can be modified afterwards using unitary function @ref LL_LPUART_SetParity().*/

    uint32_t TransferDirection;         /*!< Specifies whether the Receive and/or Transmit mode is enabled or disabled.
                                             This parameter can be a value of @ref LPUART_LL_EC_DIRECTION.

                                             This feature can be modified afterwards using unitary function @ref LL_LPUART_SetTransferDirection().*/

    uint32_t TransferBitOrder;          /*!< Specifies the data transmitted/received with the LSB first or MSB first.
                                             This parameter can be a value of @ref LPUART_LL_EC_BIT_ORDER.

                                             This feature can be modified afterwards using unitary function @ref LL_LPUART_SetTransferBitOrder().*/

    uint32_t WakeUpMethod;              /*!< Specifies the Wake Up method.
                                             This parameter can be a value of @ref LPUART_LL_EC_WAKEUP.

                                             This feature can be modified afterwards using unitary function @ref LL_LPUART_SetWakeUpMethod().*/
} LL_LPUART_InitTypeDef;

/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/* Exported constants -------------------------------------------------------*/
/** @defgroup LPUART_LL_Exported_Constants LPUART Exported Constants
 * @{
 */

/** @defgroup LPUART_LL_EC_BAUDRATE BaudRate
 * @{
 */
#define LL_LPUART_BAUDRATE_9600                   (LPUART_BAUD_BAUD_9600 | LPUART_BAUD_MCTL_9600)             /*!< Set Baud Rate is 9600bps */
#define LL_LPUART_BAUDRATE_4800                   (LPUART_BAUD_BAUD_4800 | LPUART_BAUD_MCTL_4800)             /*!< Set Baud Rate is 4800bps */
#define LL_LPUART_BAUDRATE_2400                   (LPUART_BAUD_BAUD_2400 | LPUART_BAUD_MCTL_2400)             /*!< Set Baud Rate is 2400bps */
#define LL_LPUART_BAUDRATE_1200                   (LPUART_BAUD_BAUD_1200 | LPUART_BAUD_MCTL_1200)             /*!< Set Baud Rate is 1200bps */
#define LL_LPUART_BAUDRATE_600                    (LPUART_BAUD_BAUD_600  | LPUART_BAUD_MCTL_600 )             /*!< Set Baud Rate is 600bps  */
#define LL_LPUART_BAUDRATE_300                    (LPUART_BAUD_BAUD_300  | LPUART_BAUD_MCTL_300 )             /*!< Set Baud Rate is 300bps  */
/**
 * @}
 */

/** @defgroup LPUART_LL_EC_DIRECTION Communication Direction
 * @{
 */
#define LL_LPUART_DIRECTION_NONE                 0x00000000U                            /*!< Transmitter and Receiver are disabled */
#define LL_LPUART_DIRECTION_RX                   LPUART_MODE_RE                         /*!< Transmitter is disabled and Receiver is enabled */
#define LL_LPUART_DIRECTION_TX                   LPUART_MODE_TE                         /*!< Transmitter is enabled and Receiver is disabled */
#define LL_LPUART_DIRECTION_TX_RX                (LPUART_MODE_RE | LPUART_MODE_TE)      /*!< Transmitter and Receiver are enabled */
/**
 * @}
 */

/** @defgroup LPUART_LL_EC_PARITY Parity Control
 * @{
 */
#define LL_LPUART_PARITY_NONE                    LPUART_MODE_PARS_NONE                  /*!< No parity           */
#define LL_LPUART_PARITY_ODD                     LPUART_MODE_PARS_ODD                   /*!< Odd parity          */
#define LL_LPUART_PARITY_EVEN                    LPUART_MODE_PARS_EVEN                  /*!< Even parity         */
#define LL_LPUART_PARITY_ZERO                    LPUART_MODE_PARS_ZERO                  /*!< Fixed to 0 parity   */
#define LL_LPUART_PARITY_ONE                     LPUART_MODE_PARS_ONE                   /*!< Fixed to 1 parity   */
#define LL_LPUART_PARITY_USER_DEFINED            LPUART_MODE_PARS_USER_DEFINED          /*!< User-defined parity */
/**
 * @}
 */

/** @defgroup LPUART_LL_EC_DATAWIDTH Datawidth
 * @{
 */
#define LL_LPUART_DATAWIDTH_5B                   LPUART_MODE_DATLEN_5B             /*!< 5 bits data length */
#define LL_LPUART_DATAWIDTH_6B                   LPUART_MODE_DATLEN_6B             /*!< 6 bits data length */
#define LL_LPUART_DATAWIDTH_7B                   LPUART_MODE_DATLEN_7B             /*!< 7 bits data length */
#define LL_LPUART_DATAWIDTH_8B                   LPUART_MODE_DATLEN_8B             /*!< 8 bits data length */
/**
 * @}
 */

/** @defgroup LPUART_LL_EC_BIT_ORDER Transmission Bit Order
 * @{
 */
#define LL_LPUART_LSB_FIRST                    0x00000000U                  /*!< Data is transmitted/received with the LSB first */
#define LL_LPUART_MSB_FIRST                    LPUART_MODE_LMSB             /*!< Data is transmitted/received with the MSB first */
/**
 * @}
 */

/** @defgroup LPUART_LL_EC_DATA_POLARITY Data Polarity
 * @{
 */
#define LL_LPUART_DATA_POLARITY_HIGH                  LPUART_MODE_NEG_POSITIVE         /*!< Positive polarity (Idle level = High) */
#define LL_LPUART_DATA_POLARITY_LOW                   LPUART_MODE_NEG_NEGATIVE         /*!< Negative polarity (Idle level = Low)  */
/**
 * @}
 */

/** @defgroup LPUART_LL_EC_IRDA_POLARITY IRDA Polarity
 * @{
 */
#define LL_LPUART_IRDA_POLARITY_HIGH                  LPUART_MODE_IRPOL_POSITIVE        /*!< Positive polarity (Idle level = High, Modulation = Low) */
#define LL_LPUART_IRDA_POLARITY_LOW                   LPUART_MODE_IRPOL_NEGATIVE        /*!< Negative polarity (Idle level = Low, Modulation = High) */
/**
 * @}
 */

/** @defgroup LPUART_LL_EC_STOPBITS Stop Bits
 * @{
 */
#define LL_LPUART_STOPBITS_1                     LPUART_MODE_STOPS_1                       /*!< 1 stop bit  */
#define LL_LPUART_STOPBITS_2                     LPUART_MODE_STOPS_2                       /*!< 2 stop bit  */
/**
 * @}
 */

/** @defgroup LPUART_LL_EC_WAKEUP Wakeup
 * @{
 */
#define LL_LPUART_WAKEUP_STARTBIT                LPUART_MODE_WUSEL_STARTBIT                    /*!<  LPUART wake up from Mute mode on Start bit detection         */
#define LL_LPUART_WAKEUP_RXFALLINGEDGE           LPUART_MODE_WUSEL_RXFALLINGEDGE               /*!<  LPUART wake up from Mute mode on RX falling edge detection   */
#define LL_LPUART_WAKEUP_RXCOMPLETE              LPUART_MODE_WUSEL_RXCOMPLETE                  /*!<  LPUART wake up from Mute mode on Data reception complete     */
#define LL_LPUART_WAKEUP_FRAMEMATCH              LPUART_MODE_WUSEL_FRAMEMATCH                  /*!<  LPUART wake up from Mute mode on Frame data match successful */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup LPUART_LL_Exported_Macros LPUART Exported Macros
 * @{
 */
/** @defgroup LPUART_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in LPUART register
 * @param  __INSTANCE__ LPUART Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_LPUART_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in LPUART register
 * @param  __INSTANCE__ LPUART Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_LPUART_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup LPUART_LL_Exported_Functions LPUART Exported Functions
 * @{
 */

/** @defgroup LPUART_LL_EF_Configuration Configuration functions
 * @{
 */

/**
 * @brief  Transmitter Enable
 * @rmtoll MODE          TE            LL_LPUART_EnableDirectionTx
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableDirectionTx(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->MODE, LPUART_MODE_TE);
}

/**
 * @brief  Transmitter Disable
 * @rmtoll MODE          TE            LL_LPUART_DisableDirectionTx
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableDirectionTx(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->MODE, LPUART_MODE_TE);
}

/**
 * @brief  Indicate if Transmitter is enabled
 * @rmtoll MODE          TE         LL_LPUART_IsEnabledDirectionTx
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledDirectionTx(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->MODE, LPUART_MODE_TE) == (LPUART_MODE_TE));
}

/**
 * @brief  Receiver Enable
 * @rmtoll MODE          RE            LL_LPUART_EnableDirectionRx
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableDirectionRx(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->MODE, LPUART_MODE_RE);
}

/**
 * @brief  Receiver Disable
 * @rmtoll MODE          RE            LL_LPUART_DisableDirectionRx
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableDirectionRx(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->MODE, LPUART_MODE_RE);
}

/**
 * @brief  Indicate if Receiver is enabled
 * @rmtoll MODE          RE         LL_LPUART_IsEnabledDirectionRx
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledDirectionRx(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->MODE, LPUART_MODE_RE) == (LPUART_MODE_RE));
}

/**
 * @brief  Configure simultaneously enabled/disabled states
 *         of Transmitter and Receiver
 * @rmtoll MODE          RE            LL_LPUART_SetTransferDirection\n
 *         MODE          TE            LL_LPUART_SetTransferDirection
 * @param  LPUARTx LPUART Instance
 * @param  TransferDirection This parameter can be one of the following values:
 *         @arg @ref LL_LPUART_DIRECTION_NONE
 *         @arg @ref LL_LPUART_DIRECTION_RX
 *         @arg @ref LL_LPUART_DIRECTION_TX
 *         @arg @ref LL_LPUART_DIRECTION_TX_RX
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_SetTransferDirection(LPUART_TypeDef *LPUARTx, uint32_t TransferDirection)
{
    MODIFY_REG(LPUARTx->MODE, LPUART_MODE_RE | LPUART_MODE_TE, TransferDirection);
}

/**
 * @brief  Return enabled/disabled states of Transmitter and Receiver
 * @rmtoll MODE          RE            LL_LPUART_GetTransferDirection\n
 *         MODE          TE            LL_LPUART_GetTransferDirection
 * @param  LPUARTx LPUART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LPUART_DIRECTION_NONE
 *         @arg @ref LL_LPUART_DIRECTION_RX
 *         @arg @ref LL_LPUART_DIRECTION_TX
 *         @arg @ref LL_LPUART_DIRECTION_TX_RX
 */
__STATIC_INLINE uint32_t LL_LPUART_GetTransferDirection(const LPUART_TypeDef *LPUARTx)
{
    return (uint32_t)(READ_BIT(LPUARTx->MODE, LPUART_MODE_RE | LPUART_MODE_TE));
}

/**
 * @brief  Select the polarity of the trasmit and receive data
 * @rmtoll MODE          NEG          LL_LPUART_SetDataPolarity
 * @param  LPUARTx LPUART Instance
 * @param  DataPolarity This parameter can be one of the following values:
 *         @arg @ref LL_LPUART_DATA_POLARITY_HIGH
 *         @arg @ref LL_LPUART_DATA_POLARITY_LOW
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_SetDataPolarity(LPUART_TypeDef *LPUARTx, uint32_t DataPolarity)
{
    MODIFY_REG(LPUARTx->MODE, LPUART_MODE_NEG, DataPolarity);
}

/**
 * @brief  Return the polarity of the trasmit and receive data
 * @rmtoll MODE          NEG          LL_LPUART_GetDataPolarity
 * @param  LPUARTx LPUART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LPUART_DATA_POLARITY_HIGH
 *         @arg @ref LL_LPUART_DATA_POLARITY_LOW
 */
__STATIC_INLINE uint32_t LL_LPUART_GetDataPolarity(const LPUART_TypeDef *LPUARTx)
{
    return (uint32_t)(READ_BIT(LPUARTx->MODE, LPUART_MODE_NEG));
}

/**
 * @brief  Set Word length (i.e. nb of data bits, excluding start and stop bits)
 * @rmtoll MODE          DATLEN             LL_LPUART_SetDataWidth
 * @param  LPUARTx LPUART Instance
 * @param  DataWidth This parameter can be one of the following values:
 *         @arg @ref LL_LPUART_DATAWIDTH_5B
 *         @arg @ref LL_LPUART_DATAWIDTH_6B
 *         @arg @ref LL_LPUART_DATAWIDTH_7B
 *         @arg @ref LL_LPUART_DATAWIDTH_8B
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_SetDataWidth(LPUART_TypeDef *LPUARTx, uint32_t DataWidth)
{
    MODIFY_REG(LPUARTx->MODE, LPUART_MODE_DATLEN, DataWidth);
}

/**
 * @brief  Return Word length (i.e. nb of data bits, excluding start and stop bits)
 * @rmtoll MODE          DATLEN             LL_LPUART_GetDataWidth
 * @param  LPUARTx LPUART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LPUART_DATAWIDTH_5B
 *         @arg @ref LL_LPUART_DATAWIDTH_6B
 *         @arg @ref LL_LPUART_DATAWIDTH_7B
 *         @arg @ref LL_LPUART_DATAWIDTH_8B
 */
__STATIC_INLINE uint32_t LL_LPUART_GetDataWidth(const LPUART_TypeDef *LPUARTx)
{
    return (uint32_t)(READ_BIT(LPUARTx->MODE, LPUART_MODE_DATLEN));
}

/**
 * @brief  Configure Parity.
 * @rmtoll MODE          PARS            LL_LPUART_SetParity
 * @param  LPUARTx LPUART Instance
 * @param  Parity This parameter can be one of the following values:
 *         @arg @ref LL_LPUART_PARITY_NONE
 *         @arg @ref LL_LPUART_PARITY_ODD
 *         @arg @ref LL_LPUART_PARITY_EVEN
 *         @arg @ref LL_LPUART_PARITY_ZERO
 *         @arg @ref LL_LPUART_PARITY_ONE
 *         @arg @ref LL_LPUART_PARITY_USER_DEFINED
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_SetParity(LPUART_TypeDef *LPUARTx, uint32_t Parity)
{
    MODIFY_REG(LPUARTx->MODE, LPUART_MODE_PARS, Parity);
}

/**
 * @brief  Return Parity configuration
 * @rmtoll MODE          PARS            LL_LPUART_GetParity
 * @param  LPUARTx LPUART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LPUART_PARITY_NONE
 *         @arg @ref LL_LPUART_PARITY_ODD
 *         @arg @ref LL_LPUART_PARITY_EVEN
 *         @arg @ref LL_LPUART_PARITY_ZERO
 *         @arg @ref LL_LPUART_PARITY_ONE
 *         @arg @ref LL_LPUART_PARITY_USER_DEFINED
 */
__STATIC_INLINE uint32_t LL_LPUART_GetParity(const LPUART_TypeDef *LPUARTx)
{
    return (uint32_t)(READ_BIT(LPUARTx->MODE, LPUART_MODE_PARS));
}

/**
 * @brief  Set the length of the stop bits
 * @rmtoll MODE          STOPS          LL_LPUART_SetStopBitsLength
 * @param  LPUARTx LPUART Instance
 * @param  StopBits This parameter can be one of the following values:
 *         @arg @ref LL_LPUART_STOPBITS_1
 *         @arg @ref LL_LPUART_STOPBITS_2
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_SetStopBitsLength(LPUART_TypeDef *LPUARTx, uint32_t StopBits)
{
    MODIFY_REG(LPUARTx->MODE, LPUART_MODE_STOPS, StopBits);
}

/**
 * @brief  Retrieve the length of the stop bits
 * @rmtoll MODE          STOPS          LL_LPUART_GetStopBitsLength
 * @param  LPUARTx LPUART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LPUART_STOPBITS_1
 *         @arg @ref LL_LPUART_STOPBITS_2
 */
__STATIC_INLINE uint32_t LL_LPUART_GetStopBitsLength(const LPUART_TypeDef *LPUARTx)
{
    return (uint32_t)(READ_BIT(LPUARTx->MODE, LPUART_MODE_STOPS));
}

/**
 * @brief  Configure Character frame format (Datawidth, Parity control, Stop Bits)
 * @note   Call of this function is equivalent to following function call sequence :
 *         - Data Width configuration using @ref LL_LPUART_SetDataWidth() function
 *         - Parity Control and mode configuration using @ref LL_LPUART_SetParity() function
 *         - Stop bits configuration using @ref LL_LPUART_SetStopBitsLength() function
 * @rmtoll MODE          DATLEN            LL_LPUART_ConfigCharacter\n
 *         MODE          PARS              LL_LPUART_ConfigCharacter\n
 *         MODE          STOPS             LL_LPUART_ConfigCharacter
 * @param  LPUARTx LPUART Instance
 * @param  DataWidth This parameter can be one of the following values:
 *         @arg @ref LL_LPUART_DATAWIDTH_5B
 *         @arg @ref LL_LPUART_DATAWIDTH_6B
 *         @arg @ref LL_LPUART_DATAWIDTH_7B
 *         @arg @ref LL_LPUART_DATAWIDTH_8B
 * @param  Parity This parameter can be one of the following values:
 *         @arg @ref LL_LPUART_PARITY_NONE
 *         @arg @ref LL_LPUART_PARITY_ODD
 *         @arg @ref LL_LPUART_PARITY_EVEN
 *         @arg @ref LL_LPUART_PARITY_ZERO
 *         @arg @ref LL_LPUART_PARITY_ONE
 *         @arg @ref LL_LPUART_PARITY_USER_DEFINED
 * @param  StopBits This parameter can be one of the following values:
 *         @arg @ref LL_LPUART_STOPBITS_1
 *         @arg @ref LL_LPUART_STOPBITS_2
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ConfigCharacter(LPUART_TypeDef *LPUARTx, uint32_t DataWidth, uint32_t Parity,
                                              uint32_t StopBits)
{
    MODIFY_REG(LPUARTx->MODE, LPUART_MODE_DATLEN | LPUART_MODE_PARS | LPUART_MODE_STOPS, Parity | DataWidth | StopBits);
}

/**
 * @brief  Set transfer bit order
 * @note   This bit should not be changed when communication is ongoing.
 * @rmtoll MODE          LMSB      LL_LPUART_SetTransferBitOrder
 * @param  LPUARTx LPUART Instance
 * @param  BitOrder This parameter can be one of the following values:
 *         @arg @ref LL_LPUART_LSB_FIRST
 *         @arg @ref LL_LPUART_MSB_FIRST
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_SetTransferBitOrder(LPUART_TypeDef *LPUARTx, uint32_t BitOrder)
{
    MODIFY_REG(LPUARTx->MODE, LPUART_MODE_LMSB, BitOrder);
}

/**
 * @brief  Get transfer bit order
 * @rmtoll MODE          LMSB      LL_LPUART_GetTransferBitOrder
 * @param  LPUARTx LPUART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LPUART_LSB_FIRST
 *         @arg @ref LL_LPUART_MSB_FIRST
 */
__STATIC_INLINE uint32_t LL_LPUART_GetTransferBitOrder(const LPUART_TypeDef *LPUARTx)
{
    return (uint32_t)(READ_BIT(LPUARTx->MODE, LPUART_MODE_LMSB));
}

/**
 * @brief  Internal Loopback Enable
 * @rmtoll MODE          ILBE            LL_LPUART_EnableInternalLoop
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableInternalLoop(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->MODE, LPUART_MODE_ILBE);
}

/**
 * @brief  Internal Loopback Disable
 * @rmtoll MODE          ILBE            LL_LPUART_DisableInternalLoop
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableInternalLoop(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->MODE, LPUART_MODE_ILBE);
}

/**
 * @brief  Indicate if Internal Loopback is enabled
 * @rmtoll MODE          ILBE            LL_LPUART_IsEnabledInternalLoop
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledInternalLoop(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->MODE, LPUART_MODE_ILBE) == (LPUART_MODE_ILBE));
}

/**
 * @brief  Set Receiver Wake Up method from Mute mode.
 * @rmtoll MODE          WUSEL          LL_LPUART_SetWakeUpMethod
 * @param  LPUARTx LPUART Instance
 * @param  Method This parameter can be one of the following values:
 *         @arg @ref LL_LPUART_WAKEUP_STARTBIT
 *         @arg @ref LL_LPUART_WAKEUP_RXFALLINGEDGE
 *         @arg @ref LL_LPUART_WAKEUP_RXCOMPLETE
 *         @arg @ref LL_LPUART_WAKEUP_FRAMEMATCH
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_SetWakeUpMethod(LPUART_TypeDef *LPUARTx, uint32_t Method)
{
  MODIFY_REG(LPUARTx->MODE, LPUART_MODE_WUSEL, Method);
}

/**
 * @brief  Return Receiver Wake Up method from Mute mode
 * @rmtoll MODE          WUSEL          LL_LPUART_GetWakeUpMethod
 * @param  LPUARTx LPUART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LPUART_WAKEUP_STARTBIT
 *         @arg @ref LL_LPUART_WAKEUP_RXFALLINGEDGE
 *         @arg @ref LL_LPUART_WAKEUP_RXCOMPLETE
 *         @arg @ref LL_LPUART_WAKEUP_FRAMEMATCH
 */
__STATIC_INLINE uint32_t LL_LPUART_GetWakeUpMethod(const LPUART_TypeDef *LPUARTx)
{
  return (uint32_t)(READ_BIT(LPUARTx->MODE, LPUART_MODE_WUSEL));
}

/**
 * @brief  Return TX FIFO Data Count
 * @rmtoll STA          TXFCNT          LL_LPUART_GetTxFIFODataCount
 * @param  LPUARTx LPUART Instance
 * @retval Value between Min_Data=0x0 and Max_Data=0x3
 */
__STATIC_INLINE uint32_t LL_LPUART_GetTxFIFODataCount(const LPUART_TypeDef *LPUARTx)
{
  return (uint32_t)(READ_BIT(LPUARTx->STA, LPUART_STA_TXFCNT));
}

/**
 * @brief  Return RX FIFO Data Count
 * @rmtoll MODE          RXFCNT          LL_LPUART_GetRxFIFODataCount
 * @param  LPUARTx LPUART Instance
 * @retval Value between Min_Data=0x0 and Max_Data=0x3
 */
__STATIC_INLINE uint32_t LL_LPUART_GetRxFIFODataCount(const LPUART_TypeDef *LPUARTx)
{
  return (uint32_t)(READ_BIT(LPUARTx->STA, LPUART_STA_RXFCNT));
}

/**
 * @brief  Configure LPUART BAUD register for achieving expected Baud Rate value.
 * @note   set LPUARTDIV value in BAUD Register (BAUD and MCTL content)
 *         according to used MCTL, and expected Baud Rate values
 * @rmtoll BAUD          BAUD           LL_LPUART_SetBaudRate\n
 *         BAUD          MCTL           LL_LPUART_SetBaudRate
 * @param  LPUARTx LPUART Instance
 * @param  BaudRate This parameter can be one of the following values:
 *         @arg @ref LL_LPUART_BAUDRATE_9600
 *         @arg @ref LL_LPUART_BAUDRATE_4800
 *         @arg @ref LL_LPUART_BAUDRATE_2400
 *         @arg @ref LL_LPUART_BAUDRATE_1200
 *         @arg @ref LL_LPUART_BAUDRATE_600
 *         @arg @ref LL_LPUART_BAUDRATE_300
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_SetBaudRate(LPUART_TypeDef *LPUARTx, uint32_t BaudRate)
{
    MODIFY_REG(LPUARTx->BAUD, LPUART_BAUD_BAUD | LPUART_BAUD_MCTL, BaudRate);
}

/**
 * @brief  Return current Baud Rate value, according to LPUARTDIV present in BAUD register
 *         (BAUD and MCTL content), and to used MCTL and BAUD values
 * @rmtoll BAUD          BAUD           LL_LPUART_GetBaudRate\n
 *         BAUD          MCTL           LL_LPUART_GetBaudRate
 * @param  LPUARTx LPUART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LPUART_BAUDRATE_9600
 *         @arg @ref LL_LPUART_BAUDRATE_4800
 *         @arg @ref LL_LPUART_BAUDRATE_2400
 *         @arg @ref LL_LPUART_BAUDRATE_1200
 *         @arg @ref LL_LPUART_BAUDRATE_600
 *         @arg @ref LL_LPUART_BAUDRATE_300
 */
__STATIC_INLINE uint32_t LL_LPUART_GetBaudRate(const LPUART_TypeDef *LPUARTx)
{
    return (uint32_t)(READ_BIT(LPUARTx->BAUD, LPUART_BAUD_BAUD | LPUART_BAUD_MCTL));
}

/**
 * @}
 */

/** @defgroup LPUART_LL_EF_Configuration_IRDA Configuration functions related to Irda feature
 * @{
 */

/**
 * @brief  Enable IrDA mode
 * @rmtoll MODE          IRE          LL_LPUART_EnableIrda
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIrda(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->MODE, LPUART_MODE_IRE);
}

/**
 * @brief  Disable IrDA mode
 * @rmtoll MODE          IRE          LL_LPUART_DisableIrda
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIrda(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->MODE, LPUART_MODE_IRE);
}

/**
 * @brief  Indicate if IrDA mode is enabled
 * @rmtoll MODE          IRE          LL_LPUART_IsEnabledIrda
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIrda(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->MODE, LPUART_MODE_IRE) == (LPUART_MODE_IRE));
}

/**
 * @brief  Configure IrDA polarity (Positive or Negative)
 * @rmtoll MODE          IRPOL          LL_LPUART_SetIrdaPolarity
 * @param  LPUARTx LPUART Instance
 * @param  IrdaPolarity This parameter can be one of the following values:
 *         @arg @ref LL_LPUART_IRDA_POLARITY_HIGH
 *         @arg @ref LL_LPUART_IRDA_POLARITY_LOW
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_SetIrdaPolarity(LPUART_TypeDef *LPUARTx, uint32_t IrdaPolarity)
{
    MODIFY_REG(LPUARTx->MODE, LPUART_MODE_IRPOL, IrdaPolarity);
}

/**
 * @brief  Retrieve IrDA polarity (Positive or Negative)
 * @rmtoll MODE          IRPOL          LL_LPUART_GetIrdaPolarity
 * @param  LPUARTx LPUART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LPUART_IRDA_POLARITY_HIGH
 *         @arg @ref LL_LPUART_IRDA_POLARITY_LOW
 */
__STATIC_INLINE uint32_t LL_LPUART_GetIrdaPolarity(const LPUART_TypeDef *LPUARTx)
{
    return (uint32_t)(READ_BIT(LPUARTx->MODE, LPUART_MODE_IRPOL));
}

/**
 * @}
 */

/** @defgroup LPUART_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable Transmit Complete Interrupt
 * @rmtoll IE          TXIE        LL_LPUART_EnableIT_TX
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIT_TX(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->IE, LPUART_IE_TXIE);
}

/**
 * @brief  Disable Transmit Complete Interrupt
 * @rmtoll IE          TXIE        LL_LPUART_DisableIT_TX
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIT_TX(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->IE, LPUART_IE_TXIE);
}

/**
 * @brief  Check if the LPUART Transmit Complete Interrupt is enabled or disabled.
 * @rmtoll IE          TXIE        LL_LPUART_IsEnabledIT_TX
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIT_TX(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->IE, LPUART_IE_TXIE) == (LPUART_IE_TXIE));
}

/**
 * @brief  Enable Receive Complete Interrupt
 * @rmtoll IE          RXIE        LL_LPUART_EnableIT_RX
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIT_RX(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->IE, LPUART_IE_RXIE);
}

/**
 * @brief  Disable Receive Complete Interrupt
 * @rmtoll IE          RXIE        LL_LPUART_DisableIT_RX
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIT_RX(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->IE, LPUART_IE_RXIE);
}

/**
 * @brief  Check if the LPUART Receive Complete Interrupt is enabled or disabled.
 * @rmtoll IE          RXIE        LL_LPUART_IsEnabledIT_RX
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIT_RX(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->IE, LPUART_IE_RXIE) == (LPUART_IE_RXIE));
}

/**
 * @brief  Enable Wakeup from Sleep Interrupt
 * @rmtoll IE          WUFIE        LL_LPUART_EnableIT_WUF
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIT_WUF(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->IE, LPUART_IE_WUFIE);
}

/**
 * @brief  Disable Wakeup from Sleep Interrupt
 * @rmtoll IE          WUFIE        LL_LPUART_DisableIT_WUF
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIT_WUF(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->IE, LPUART_IE_WUFIE);
}

/**
 * @brief  Check if the LPUART Wakeup from Sleep Interrupt is enabled or disabled.
 * @rmtoll IE          WUFIE        LL_LPUART_IsEnabledIT_WUF
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIT_WUF(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->IE, LPUART_IE_WUFIE) == (LPUART_IE_WUFIE));
}

/**
 * @brief  Enable Parity Error Interrupt
 * @rmtoll IE          PEIE        LL_LPUART_EnableIT_PE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIT_PE(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->IE, LPUART_IE_PEIE);
}

/**
 * @brief  Disable Parity Error Interrupt
 * @rmtoll IE          PEIE        LL_LPUART_DisableIT_PE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIT_PE(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->IE, LPUART_IE_PEIE);
}

/**
 * @brief  Check if the LPUART Parity Error Interrupt is enabled or disabled.
 * @rmtoll IE          PEIE        LL_LPUART_IsEnabledIT_PE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIT_PE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->IE, LPUART_IE_PEIE) == (LPUART_IE_PEIE));
}

/**
 * @brief  Enable Overrun Error Interrupt
 * @rmtoll IE          OEIE        LL_LPUART_EnableIT_OE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIT_OE(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->IE, LPUART_IE_OEIE);
}

/**
 * @brief  Disable Overrun Error Interrupt
 * @rmtoll IE          OEIE        LL_LPUART_DisableIT_OE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIT_OE(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->IE, LPUART_IE_OEIE);
}

/**
 * @brief  Check if the LPUART Overrun Error Interrupt is enabled or disabled.
 * @rmtoll IE          OEIE        LL_LPUART_IsEnabledIT_OE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIT_OE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->IE, LPUART_IE_OEIE) == (LPUART_IE_OEIE));
}

/**
 * @brief  Enable Framing Error Interrupt
 * @rmtoll IE          FEIE        LL_LPUART_EnableIT_FE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIT_FE(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->IE, LPUART_IE_FEIE);
}

/**
 * @brief  Disable Framing Error Interrupt
 * @rmtoll IE          FEIE        LL_LPUART_DisableIT_FE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIT_FE(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->IE, LPUART_IE_FEIE);
}

/**
 * @brief  Check if the LPUART Framing Error Interrupt is enabled or disabled.
 * @rmtoll IE          FEIE        LL_LPUART_IsEnabledIT_FE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIT_FE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->IE, LPUART_IE_FEIE) == (LPUART_IE_FEIE));
}

/**
 * @brief  Enable Data Error Interrupt
 * @rmtoll IE          DEIE        LL_LPUART_EnableIT_DE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIT_DE(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->IE, LPUART_IE_DEIE);
}

/**
 * @brief  Disable Data Error Interrupt
 * @rmtoll IE          DEIE        LL_LPUART_DisableIT_DE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIT_DE(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->IE, LPUART_IE_DEIE);
}

/**
 * @brief  Check if the LPUART Data Error Interrupt is enabled or disabled.
 * @rmtoll IE          DEIE        LL_LPUART_IsEnabledIT_DE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIT_DE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->IE, LPUART_IE_DEIE) == (LPUART_IE_DEIE));
}

/**
 * @brief  Enable Start Bit Noise Detected Interrupt
 * @rmtoll IE          NEIE        LL_LPUART_EnableIT_NE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIT_NE(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->IE, LPUART_IE_NEIE);
}

/**
 * @brief  Disable Start Bit Noise Detected Interrupt
 * @rmtoll IE          NEIE        LL_LPUART_DisableIT_NE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIT_NE(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->IE, LPUART_IE_NEIE);
}

/**
 * @brief  Check if the LPUART Start Bit Noise Detected Interrupt is enabled or disabled.
 * @rmtoll IE          NEIE        LL_LPUART_IsEnabledIT_NE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIT_NE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->IE, LPUART_IE_NEIE) == (LPUART_IE_NEIE));
}

/**
 * @brief  Enable IDLE Frame Detected Interrupt
 * @rmtoll IE          IDLEIE        LL_LPUART_EnableIT_IDLE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIT_IDLE(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->IE, LPUART_IE_IDLEIE);
}

/**
 * @brief  Disable IDLE Frame Detected Interrupt
 * @rmtoll IE          IDLEIE        LL_LPUART_DisableIT_IDLE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIT_IDLE(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->IE, LPUART_IE_IDLEIE);
}

/**
 * @brief  Check if the LPUART IDLE Frame Detected Interrupt is enabled or disabled.
 * @rmtoll IE          IDLEIE        LL_LPUART_IsEnabledIT_IDLE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIT_IDLE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->IE, LPUART_IE_IDLEIE) == (LPUART_IE_IDLEIE));
}

/**
 * @brief  Enable TX FIFO Empty Interrupt
 * @rmtoll IE          TXFEIE        LL_LPUART_EnableIT_TXFE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIT_TXFE(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->IE, LPUART_IE_TXFEIE);
}

/**
 * @brief  Disable TX FIFO Empty Interrupt
 * @rmtoll IE          TXFEIE        LL_LPUART_DisableIT_TXFE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIT_TXFE(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->IE, LPUART_IE_TXFEIE);
}

/**
 * @brief  Check if the LPUART TX FIFO Empty Interrupt is enabled or disabled.
 * @rmtoll IE          TXFEIE        LL_LPUART_IsEnabledIT_TXFE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIT_TXFE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->IE, LPUART_IE_TXFEIE) == (LPUART_IE_TXFEIE));
}

/**
 * @brief  Enable TX FIFO Half Empty Interrupt
 * @rmtoll IE          TXFHEIE        LL_LPUART_EnableIT_TXFHE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIT_TXFHE(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->IE, LPUART_IE_TXFHEIE);
}

/**
 * @brief  Disable TX FIFO Half Empty Interrupt
 * @rmtoll IE          TXFHEIE        LL_LPUART_DisableIT_TXFHE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIT_TXFHE(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->IE, LPUART_IE_TXFHEIE);
}

/**
 * @brief  Check if the LPUART TX FIFO Half Empty Interrupt is enabled or disabled.
 * @rmtoll IE          TXFHEIE        LL_LPUART_IsEnabledIT_TXFHE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIT_TXFHE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->IE, LPUART_IE_TXFHEIE) == (LPUART_IE_TXFHEIE));
}

/**
 * @brief  Enable RX FIFO Not Empty Interrupt
 * @rmtoll IE          RXFNEIE        LL_LPUART_EnableIT_RXFNE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIT_RXFNE(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->IE, LPUART_IE_RXFNEIE);
}

/**
 * @brief  Disable RX FIFO Not Empty Interrupt
 * @rmtoll IE          RXFNEIE        LL_LPUART_DisableIT_RXFNE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIT_RXFNE(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->IE, LPUART_IE_RXFNEIE);
}

/**
 * @brief  Check if the LPUART RX FIFO Not Empty Interrupt is enabled or disabled.
 * @rmtoll IE          RXFNEIE        LL_LPUART_IsEnabledIT_RXFNE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIT_RXFNE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->IE, LPUART_IE_RXFNEIE) == (LPUART_IE_RXFNEIE));
}

/**
 * @brief  Enable RX FIFO Half Full Interrupt
 * @rmtoll IE          RXFHFIE        LL_LPUART_EnableIT_RXFHF
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIT_RXFHF(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->IE, LPUART_IE_RXFHFIE);
}

/**
 * @brief  Disable RX FIFO Half Full Interrupt
 * @rmtoll IE          RXFHFIE        LL_LPUART_DisableIT_RXFHF
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIT_RXFHF(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->IE, LPUART_IE_RXFHFIE);
}

/**
 * @brief  Check if the LPUART RX FIFO Half Full Interrupt is enabled or disabled.
 * @rmtoll IE          RXFHFIE        LL_LPUART_IsEnabledIT_RXFHF
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIT_RXFHF(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->IE, LPUART_IE_RXFHFIE) == (LPUART_IE_RXFHFIE));
}

/**
 * @brief  Enable RX FIFO Full Interrupt
 * @rmtoll IE          RXFFIE        LL_LPUART_EnableIT_RXFF
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_EnableIT_RXFF(LPUART_TypeDef *LPUARTx)
{
    SET_BIT(LPUARTx->IE, LPUART_IE_RXFFIE);
}

/**
 * @brief  Disable RX FIFO Full Interrupt
 * @rmtoll IE          RXFFIE        LL_LPUART_DisableIT_RXFF
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_DisableIT_RXFF(LPUART_TypeDef *LPUARTx)
{
    CLEAR_BIT(LPUARTx->IE, LPUART_IE_RXFFIE);
}

/**
 * @brief  Check if the LPUART RX FIFO Full Interrupt is enabled or disabled.
 * @rmtoll IE          RXFFIE        LL_LPUART_IsEnabledIT_RXFF
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsEnabledIT_RXFF(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->IE, LPUART_IE_RXFFIE) == (LPUART_IE_RXFFIE));
}

/**
 * @}
 */

/** @defgroup LPUART_LL_EF_FLAG_management FLAG_management
 * @{
 */

/**
 * @brief  Check if the LPUART Transmit Complete Flag is set or not
 * @rmtoll STA           TXIF            LL_LPUART_IsActiveFlag_TX
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_TX(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_TXIF) == (LPUART_STA_TXIF));
}

/**
 * @brief  Clear Transmit Complete Flag
 * @rmtoll STA           TXIF            LL_LPUART_ClearFlag_TX
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_TX(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_TXIF);
}

/**
 * @brief  Check if the LPUART Receive Complete Flag is set or not
 * @rmtoll STA           RXIF            LL_LPUART_IsActiveFlag_RX
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_RX(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_RXIF) == (LPUART_STA_RXIF));
}

/**
 * @brief  Clear Receive Complete Flag
 * @rmtoll STA           RXIF            LL_LPUART_ClearFlag_RX
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_RX(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_RXIF);
}

/**
 * @brief  Check if the LPUART Wakeup Event Flag is set or not
 * @rmtoll STA           WUF            LL_LPUART_IsActiveFlag_WU
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_WU(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_WUF) == (LPUART_STA_WUF));
}

/**
 * @brief  Clear Wakeup Event Flag
 * @rmtoll STA           WUF            LL_LPUART_ClearFlag_WU
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_WU(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_WUF);
}

/**
 * @brief  Check if the LPUART Parity Error Flag is set or not
 * @rmtoll STA           PEIF            LL_LPUART_IsActiveFlag_PE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_PE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_PEIF) == (LPUART_STA_PEIF));
}

/**
 * @brief  Clear Parity Error Flag
 * @rmtoll STA           PEIF            LL_LPUART_ClearFlag_PE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_PE(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_PEIF);
}

/**
 * @brief  Check if the LPUART Overrun Error Flag is set or not
 * @rmtoll STA           OEIF            LL_LPUART_IsActiveFlag_OE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_OE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_OEIF) == (LPUART_STA_OEIF));
}

/**
 * @brief  Clear Overrun Error Flag
 * @rmtoll STA           OEIF            LL_LPUART_ClearFlag_OE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_OE(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_OEIF);
}

/**
 * @brief  Check if the LPUART Framing Error Flag is set or not
 * @rmtoll STA           FEIF            LL_LPUART_IsActiveFlag_FE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_FE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_FEIF) == (LPUART_STA_FEIF));
}

/**
 * @brief  Clear Framing Error Flag
 * @rmtoll STA           FEIF            LL_LPUART_ClearFlag_FE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_FE(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_FEIF);
}

/**
 * @brief  Check if the LPUART Data Error Flag is set or not
 * @rmtoll STA           DEIF            LL_LPUART_IsActiveFlag_DE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_DE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_DEIF) == (LPUART_STA_DEIF));
}

/**
 * @brief  Clear Data Error Flag
 * @rmtoll STA           DEIF            LL_LPUART_ClearFlag_DE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_DE(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_DEIF);
}

/**
 * @brief  Check if the LPUART Start Bit Noise Detected Flag is set or not
 * @rmtoll STA           NEIF            LL_LPUART_IsActiveFlag_NE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_NE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_NEIF) == (LPUART_STA_NEIF));
}

/**
 * @brief  Clear Start Bit Noise Detected Flag
 * @rmtoll STA           NEIF            LL_LPUART_ClearFlag_NE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_NE(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_NEIF);
}

/**
 * @brief  Check if the LPUART Idle Frame Detected Flag is set or not
 * @rmtoll STA           IDLEIF            LL_LPUART_IsActiveFlag_IDLE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_IDLE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_IDLEIF) == (LPUART_STA_IDLEIF));
}

/**
 * @brief  Clear Idle Frame Detected Flag
 * @rmtoll STA           IDLEIF            LL_LPUART_ClearFlag_IDLE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_IDLE(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_IDLEIF);
}

/**
 * @brief  Check if the LPUART Transmitter Busy Flag is set or not (read only)
 * @rmtoll STA           TBUSY            LL_LPUART_IsActiveFlag_TBUSY
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_TBUSY(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_TBUSY) == (LPUART_STA_TBUSY));
}

/**
 * @brief  Check if the LPUART Receiver Busy Flag is set or not (read only)
 * @rmtoll STA           RBUSY            LL_LPUART_IsActiveFlag_RBUSY
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_RBUSY(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_RBUSY) == (LPUART_STA_RBUSY));
}

/**
 * @brief  Check if the LPUART TX FIFO Empty Flag is set or not
 * @rmtoll STA           TXFEIF            LL_LPUART_IsActiveFlag_TXFE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_TXFE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_TXFEIF) == (LPUART_STA_TXFEIF));
}

/**
 * @brief  Clear TX FIFO Empty Flag
 * @rmtoll STA           TXFEIF            LL_LPUART_ClearFlag_TXFE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_TXFE(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_TXFEIF);
}

/**
 * @brief  Check if the LPUART TX FIFO Half Empty Flag is set or not
 * @rmtoll STA           TXFHEIF            LL_LPUART_IsActiveFlag_TXFHE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_TXFHE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_TXFHEIF) == (LPUART_STA_TXFHEIF));
}

/**
 * @brief  Clear TX FIFO Half Empty Flag
 * @rmtoll STA           TXFHEIF            LL_LPUART_ClearFlag_TXFHE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_TXFHE(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_TXFHEIF);
}

/**
 * @brief  Check if the LPUART TX FIFO Full Flag is set or not
 * @rmtoll STA           TXFF            LL_LPUART_IsActiveFlag_TXFF
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_TXFF(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_TXFF) == (LPUART_STA_TXFF));
}

/**
 * @brief  Clear TX FIFO Full Flag
 * @rmtoll STA           TXFF            LL_LPUART_ClearFlag_TXFF
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_TXFF(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_TXFF);
}

/**
 * @brief  Check if the LPUART RX FIFO Not Empty Flag is set or not
 * @rmtoll STA           RXFNEIF            LL_LPUART_IsActiveFlag_RXFNE
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_RXFNE(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_RXFNEIF) == (LPUART_STA_RXFNEIF));
}

/**
 * @brief  Clear RX FIFO Not Empty Flag
 * @rmtoll STA           RXFNEIF            LL_LPUART_ClearFlag_RXFNE
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_RXFNE(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_RXFNEIF);
}

/**
 * @brief  Check if the LPUART RX FIFO Half Full Flag is set or not
 * @rmtoll STA           RXFHFIF            LL_LPUART_IsActiveFlag_RXFHF
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_RXFHF(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_RXFHFIF) == (LPUART_STA_RXFHFIF));
}

/**
 * @brief  Clear RX FIFO Half Full Flag
 * @rmtoll STA           RXFHFIF            LL_LPUART_ClearFlag_RXFHF
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_RXFHF(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_RXFHFIF);
}

/**
 * @brief  Check if the LPUART RX FIFO Full Flag is set or not
 * @rmtoll STA           RXFFIF            LL_LPUART_IsActiveFlag_RXFF
 * @param  LPUARTx LPUART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LPUART_IsActiveFlag_RXFF(const LPUART_TypeDef *LPUARTx)
{
    return (READ_BIT(LPUARTx->STA, LPUART_STA_RXFFIF) == (LPUART_STA_RXFFIF));
}

/**
 * @brief  Clear RX FIFO Full Flag
 * @rmtoll STA           RXFFIF            LL_LPUART_ClearFlag_RXFF
 * @param  LPUARTx LPUART Instance
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_ClearFlag_RXFF(LPUART_TypeDef *LPUARTx)
{
    WRITE_REG(LPUARTx->STA, LPUART_STA_RXFFIF);
}

/**
 * @}
 */

/** @defgroup LPUART_LL_EF_Data_Management Data_Management
 * @{
 */

/**
 * @brief  Read Receiver Data register (Receive Data value, 8 bits)
 * @rmtoll RXDATA           RXDATA            LL_LPUART_ReceiveData8
 * @param  LPUARTx LPUART Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF
 */
__STATIC_INLINE uint8_t LL_LPUART_ReceiveData8(const LPUART_TypeDef *LPUARTx)
{
    return (uint8_t)(READ_BIT(LPUARTx->RXDATA, LPUART_RXDATA_RXDATA));
}

/**
 * @brief  Read Receiver Data register (Receive Data value, 9 bits)
 * @rmtoll RXDATA           RXDATA            LL_LPUART_ReceiveData9\n
 *         RXDATA           UP                LL_LPUART_ReceiveData9
 * @param  LPUARTx LPUART Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x1FF
 */
__STATIC_INLINE uint16_t LL_LPUART_ReceiveData9(const LPUART_TypeDef *LPUARTx)
{
    return (uint16_t)(READ_BIT(LPUARTx->RXDATA, LPUART_RXDATA_RXDATA | LPUART_RXDATA_UP));
}

/**
 * @brief  Write in Transmitter Data Register (Transmit Data value, 8 bits)
 * @rmtoll TXDATA           TXDATA            LL_LPUART_TransmitData8
 * @param  LPUARTx LPUART Instance
 * @param  Value between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_TransmitData8(LPUART_TypeDef *LPUARTx, uint8_t Value)
{
    LPUARTx->TXDATA = Value;
}

/**
 * @brief  Write in Transmitter Data Register (Transmit Data value, 9 bits)
 * @rmtoll TXDATA           TXDATA            LL_LPUART_TransmitData9\n
 *         TXDATA           UP                LL_LPUART_TransmitData9
 * @param  LPUARTx LPUART Instance
 * @param  Value between Min_Data=0x00 and Max_Data=0x1FF
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_TransmitData9(LPUART_TypeDef *LPUARTx, uint16_t Value)
{
    LPUARTx->TXDATA = Value & 0x1FFU;
}

/**
 * @brief  Write in Data Match Register
 * @note   Compares with the first received frame of data. If a match is detected,
 *         it triggers the wakeup interrupt (used for wakeup from sleep mode).
 * @rmtoll DMR           MDATA            LL_LPUART_SetMatchData
 * @param  LPUARTx LPUART Instance
 * @param  Value between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_LPUART_SetMatchData(LPUART_TypeDef *LPUARTx, uint8_t Value)
{
    LPUARTx->DMR = Value;
}

/**
 * @brief  Read Data Match Register
 * @note   Compares with the first received frame of data. If a match is detected,
 *         it triggers the wakeup interrupt (used for wakeup from sleep mode).
 * @rmtoll DMR           MDATA            LL_LPUART_GetMatchData
 * @param  LPUARTx LPUART Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF
 */
__STATIC_INLINE uint8_t LL_LPUART_GetMatchData(const LPUART_TypeDef *LPUARTx)
{
    return (uint8_t)(READ_BIT(LPUARTx->DMR, LPUART_DMR_MDATA));
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup LPUART_LL_EF_Init Initialization and de-initialization functions
 * @{
 */

// The initialization function can have more than one argument.
ErrorStatus LL_LPUART_Init(LPUART_TypeDef *LPUARTx, const LL_LPUART_InitTypeDef *LPUART_InitStruct);
ErrorStatus LL_LPUART_DeInit(LPUART_TypeDef *LPUARTx);
void        LL_LPUART_StructInit(LL_LPUART_InitTypeDef *LPUART_InitStruct); // If the module is not suitable for the default configuration, you could not defines this function.

/**
 * @}
 */
#endif /* USE_FULL_LL_DRIVER */

/**
 * @}
 */

/**
 * @}
 */
#endif /* LPUART */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_LPUART_H */
