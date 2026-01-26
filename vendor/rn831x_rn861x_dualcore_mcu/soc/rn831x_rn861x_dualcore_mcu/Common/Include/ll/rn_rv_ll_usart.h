/**
 *****************************************************************************
 * @file   rn_rv_ll_usart.h
 * @author Renergy Software Team
 * @brief  Header file of USART LL module.
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
#ifndef __RN_LL_USART_H
#define __RN_LL_USART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_usart.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (USART0) || defined (USART1)

/** @defgroup USART_LL USART
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup USART_LL_Private_Constants USART Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup USART_LL_Private_Macros USART Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup USART_LL_ES_INIT USART Exported Init structure
 * @{
 */

/**
 * @brief LL USART Init Structure definition
 */
typedef struct
{
    uint32_t BaudRate;                  /*!< This field defines expected Usart communication baud rate.

                                             This feature can be modified afterwards using unitary function @ref LL_USART_SetBaudRate().*/

    uint32_t DataWidth;                 /*!< Specifies the number of data bits transmitted or received in a frame.
                                             This parameter can be a value of @ref USART_LL_EC_DATAWIDTH.

                                             This feature can be modified afterwards using unitary function @ref LL_USART_SetDataWidth().*/

    uint32_t StopBits;                  /*!< Specifies the number of stop bits transmitted.
                                             This parameter can be a value of @ref USART_LL_EC_STOPBITS.

                                             This feature can be modified afterwards using unitary function @ref LL_USART_SetStopBitsLength().*/

    uint32_t Parity;                    /*!< Specifies the parity mode.
                                             This parameter can be a value of @ref USART_LL_EC_PARITY.

                                             This feature can be modified afterwards using unitary function @ref LL_USART_SetParity().*/

    uint32_t TransferDirection;         /*!< Specifies whether the Receive and/or Transmit mode is enabled or disabled.
                                             This parameter can be a value of @ref USART_LL_EC_DIRECTION.

                                             This feature can be modified afterwards using unitary function @ref LL_USART_SetTransferDirection().*/

    uint32_t HardwareFlowControl;       /*!< Specifies whether the hardware flow control mode is enabled or disabled.
                                           This parameter can be a value of @ref USART_LL_EC_HWCONTROL.

                                           This feature can be modified afterwards using unitary function @ref LL_USART_SetHWFlowCtrl().*/

} LL_USART_InitTypeDef;

/**
 * @brief LL USART Clock Init Structure definition
 */
typedef struct
{
    uint32_t ClockOutput;               /*!< Specifies whether the USART clock is enabled or disabled.
                                             This parameter can be a value of @ref USART_LL_EC_CLOCK.

                                             USART HW configuration can be modified afterwards using unitary functions
                                             @ref LL_USART_EnableSCLKOutput() or @ref LL_USART_DisableSCLKOutput().
                                             For more details, refer to description of this function. */

    uint32_t ClockPolarity;             /*!< Specifies the steady state of the serial clock.
                                             This parameter can be a value of @ref USART_LL_EC_POLARITY.

                                             USART HW configuration can be modified afterwards using unitary functions @ref LL_USART_SetClockPolarity().
                                             For more details, refer to description of this function. */

    uint32_t ClockPhase;                /*!< Specifies the clock transition on which the bit capture is made.
                                             This parameter can be a value of @ref USART_LL_EC_PHASE.

                                             USART HW configuration can be modified afterwards using unitary functions @ref LL_USART_SetClockPhase().
                                             For more details, refer to description of this function. */

    uint32_t LastBitClockPulse;         /*!< Specifies whether the clock pulse corresponding to the last transmitted
                                             data bit (MSB) has to be output on the SCLK pin in synchronous mode.
                                             This parameter can be a value of @ref USART_LL_EC_LASTCLKPULSE.

                                             USART HW configuration can be modified afterwards using unitary functions @ref LL_USART_SetLastClkPulseOutput().
                                             For more details, refer to description of this function. */

} LL_USART_ClockInitTypeDef;

/**
 * @}
 */
#endif /* USE_FULL_LL_DRIVER */

/* Exported constants -------------------------------------------------------*/
/** @defgroup USART_LL_Exported_Constants USART Exported Constants
 * @{
 */

/** @defgroup USART_LL_EC_DMA_DIRECTION DMA Communication Direction
 * @{
 */
#define LL_USART_DMA_DIRECTION_NONE                 0x00000000U                                                         /*!< DMA Transmitter and Receiver are disabled           */
#define LL_USART_DMA_DIRECTION_RX                   (USART_SETUP_SEPARATE_EN | USART_SETUP_RX_DMA_EN)                   /*!< DMA Transmitter is disabled and Receiver is enabled */
#define LL_USART_DMA_DIRECTION_TX                   (USART_SETUP_SEPARATE_EN | USART_SETUP_DMA_EN)                      /*!< DMA Transmitter is enabled and Receiver is disabled */
#define LL_USART_DMA_DIRECTION_TX_RX                USART_SETUP_DMA_EN                                                  /*!< DMA Transmitter and Receiver are enabled            */
/**
 * @}
 */

/** @defgroup USART_LL_EC_SAMPLE_POSITION Sample Position
 * @{
 */
#define LL_USART_SAMPLE_POSITION_SPREAD                    USART_SETUP_SAMPLE_POSITION_SPREAD                     /*!< Sampling point is spread */
#define LL_USART_SAMPLE_POSITION_DOWN                      USART_SETUP_SAMPLE_POSITION_DOWN                       /*!< Sampling point is down   */
#define LL_USART_SAMPLE_POSITION_UP                        USART_SETUP_SAMPLE_POSITION_UP                         /*!< Sampling point is up     */
/**
 * @}
 */

/** @defgroup USART_LL_EC_DIRECTION Communication Direction
 * @{
 */
#define LL_USART_DIRECTION_NONE                 0x00000000U                        /*!< Transmitter and Receiver are disabled */
#define LL_USART_DIRECTION_RX                   0x00000001U                        /*!< Transmitter is disabled and Receiver is enabled */
#define LL_USART_DIRECTION_TX                   0x00000002U                        /*!< Transmitter is enabled and Receiver is disabled */
#define LL_USART_DIRECTION_TX_RX                0x00000003U                        /*!< Transmitter and Receiver are enabled */
/**
 * @}
 */

/** @defgroup USART_LL_EC_PARITY Parity Control
 * @{
 */
#define LL_USART_PARITY_NONE                    0x00000000U                                             /*!< Parity control disabled                            */
#define LL_USART_PARITY_EVEN                    USART_SETUP_PARITY_EN                                   /*!< Parity control enabled and Even Parity is selected */
#define LL_USART_PARITY_ODD                     (USART_SETUP_PARITY_EN | USART_SETUP_PARITY_SEL)        /*!< Parity control enabled and Odd Parity is selected  */
/**
 * @}
 */

/** @defgroup USART_LL_EC_DATAWIDTH Datawidth
 * @{
 */
#define LL_USART_DATAWIDTH_5B                   USART_SETUP_BIT_LENGTH_5B             /*!< 5 bits word length : Start bit, 5 data bits, n stop bits */
#define LL_USART_DATAWIDTH_6B                   USART_SETUP_BIT_LENGTH_6B             /*!< 6 bits word length : Start bit, 6 data bits, n stop bits */
#define LL_USART_DATAWIDTH_7B                   USART_SETUP_BIT_LENGTH_7B             /*!< 7 bits word length : Start bit, 7 data bits, n stop bits */
#define LL_USART_DATAWIDTH_8B                   USART_SETUP_BIT_LENGTH_8B             /*!< 8 bits word length : Start bit, 8 data bits, n stop bits */
#define LL_USART_DATAWIDTH_9B                   USART_SETUP_BIT_LENGTH_9B             /*!< 9 bits word length : Start bit, 9 data bits, n stop bits */
/**
 * @}
 */

/** @defgroup USART_LL_EC_CLOCK Clock Signal
 * @{
 */

#define LL_USART_CLOCK_DISABLE                  0x00000000U            /*!< Clock signal not provided */
#define LL_USART_CLOCK_ENABLE                   USART_SETUP_CLK_EN     /*!< Clock signal provided */
/**
 * @}
 */

/** @defgroup USART_LL_EC_LASTCLKPULSE Last Clock Pulse
 * @{
 */
#define LL_USART_LASTCLKPULSE_NO_OUTPUT         USART_SETUP_LBCL_NO_OUTPUT           /*!< The clock pulse of the last data bit is not output to the SCLK pin */
#define LL_USART_LASTCLKPULSE_OUTPUT            USART_SETUP_LBCL_OUTPUT              /*!< The clock pulse of the last data bit is output to the SCLK pin     */
/**
 * @}
 */

/** @defgroup USART_LL_EC_PHASE Clock Phase
 * @{
 */
#define LL_USART_PHASE_1EDGE                    USART_SETUP_CPHA_1EDGE           /*!< The first clock transition is the first data capture edge  */
#define LL_USART_PHASE_2EDGE                    USART_SETUP_CPHA_2EDGE           /*!< The second clock transition is the first data capture edge */
/**
 * @}
 */

/** @defgroup USART_LL_EC_POLARITY Clock Polarity
 * @{
 */
#define LL_USART_POLARITY_LOW                   USART_SETUP_CPOL_LOW         /*!< Steady low value on SCLK pin outside transmission window  */
#define LL_USART_POLARITY_HIGH                  USART_SETUP_CPOL_HIGH        /*!< Steady high value on SCLK pin outside transmission window */
/**
 * @}
 */

/** @defgroup USART_LL_EC_STOPBITS Stop Bits
 * @{
 */
#define LL_USART_STOPBITS_0_5                   USART_TXCTRL_CFG_STOP_BIT_0_5                       /*!< 0.5 stop bit  */
#define LL_USART_STOPBITS_1                     USART_TXCTRL_CFG_STOP_BIT_1                         /*!< 1 stop bit    */
#define LL_USART_STOPBITS_1_5                   USART_TXCTRL_CFG_STOP_BIT_1_5                       /*!< 1.5 stop bits */
#define LL_USART_STOPBITS_2                     USART_TXCTRL_CFG_STOP_BIT_2                         /*!< 2 stop bits   */
/**
 * @}
 */

/** @defgroup USART_LL_EC_HWCONTROL Hardware Control
 * @{
 */
#define LL_USART_HWCONTROL_NONE                 0x00000000U                             /*!< CTS and RTS hardware flow control disabled                                             */
#define LL_USART_HWCONTROL_RTS                  USART_SETUP_RTSE                        /*!< RTS output enabled, data is only requested when there is space in the receive buffer   */
#define LL_USART_HWCONTROL_CTS                  USART_SETUP_CTSE                        /*!< CTS mode enabled, data is only transmitted when the nCTS input is asserted (tied to 0) */
#define LL_USART_HWCONTROL_RTS_CTS              (USART_SETUP_RTSE | USART_SETUP_CTSE)   /*!< CTS and RTS hardware flow control enabled                                              */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup USART_LL_Exported_Macros USART Exported Macros
 * @{
 */
/** @defgroup USART_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in USART register
 * @param  __INSTANCE__ USART Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_USART_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in USART register
 * @param  __INSTANCE__ USART Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_USART_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/** @defgroup USART_LL_EM_Exported_Macros_Helper Exported Macros Helper
 * @{
 */

/**
 * @brief  Compute USARTDIV value according to Peripheral Clock and expected Baud Rate.
 * @param  __PERIPHCLK__ Peripheral Clock frequency used for USART instance
 * @param  __BAUDRATE__ Baud rate value to achieve
 * @retval USARTDIV value to be used for BAUD_DIV register
 */
#define __LL_USART_CALC_BAUDRATE_DIVMANT(__PERIPHCLK__, __BAUDRATE__)   (((__PERIPHCLK__)) / ((__BAUDRATE__)) - 1)

#define __LL_USART_CALC_BAUDRATE_DIVFRAC(__PERIPHCLK__, __BAUDRATE__)   ((uint32_t)(((((__PERIPHCLK__) << 3 ) + \
                                                                            ((__BAUDRATE__) >> 1)) / (__BAUDRATE__)) - 8U))
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup USART_LL_Exported_Functions USART Exported Functions
 * @{
 */

/** @defgroup USART_LL_EF_Configuration Configuration functions
 * @{
 */

/**
 * @brief  Transmitter Enable
 * @rmtoll TXCTRL          TXEN            LL_USART_EnableDirectionTx
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableDirectionTx(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->TXCTRL, USART_TXCTRL_TXEN);
}

/**
 * @brief  Transmitter Disable
 * @rmtoll TXCTRL          TXEN            LL_USART_DisableDirectionTx
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableDirectionTx(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->TXCTRL, USART_TXCTRL_TXEN);
}

/**
 * @brief  Indicate if Transmitter is enabled
 * @rmtoll TXCTRL          TXEN         LL_USART_IsEnabledDirectionTx
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledDirectionTx(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->TXCTRL, USART_TXCTRL_TXEN) == (USART_TXCTRL_TXEN));
}

/**
 * @brief  Set the length of the stop bits
 * @rmtoll TXCTRL          CFG_STOP_BIT          LL_USART_SetStopBitsLength
 * @param  USARTx USART Instance
 * @param  StopBits This parameter can be one of the following values:
 *         @arg @ref LL_USART_STOPBITS_0_5
 *         @arg @ref LL_USART_STOPBITS_1
 *         @arg @ref LL_USART_STOPBITS_1_5
 *         @arg @ref LL_USART_STOPBITS_2
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetStopBitsLength(USART_TypeDef *USARTx, uint32_t StopBits)
{
    MODIFY_REG(USARTx->TXCTRL, USART_TXCTRL_CFG_STOP_BIT, StopBits);
}

/**
 * @brief  Retrieve the length of the stop bits
 * @rmtoll TXCTRL          CFG_STOP_BIT          LL_USART_GetStopBitsLength
 * @param  USARTx USART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_USART_STOPBITS_0_5
 *         @arg @ref LL_USART_STOPBITS_1
 *         @arg @ref LL_USART_STOPBITS_1_5
 *         @arg @ref LL_USART_STOPBITS_2
 */
__STATIC_INLINE uint32_t LL_USART_GetStopBitsLength(const USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->TXCTRL, USART_TXCTRL_CFG_STOP_BIT));
}

/**
 * @brief  TX GDMA reload mode Enable
 * @rmtoll TXCTRL          CONTINUOUS            LL_USART_EnableTxDMAReload
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableTxDMAReload(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->TXCTRL, USART_TXCTRL_CONTINUOUS);
}

/**
 * @brief  TX GDMA reload mode Disable
 * @rmtoll TXCTRL          CONTINUOUS            LL_USART_DisableTxDMAReload
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableTxDMAReload(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->TXCTRL, USART_TXCTRL_CONTINUOUS);
}

/**
 * @brief  Indicate if TX GDMA reload mode is enabled
 * @rmtoll TXCTRL          CONTINUOUS            LL_USART_DisableTxDMAReload
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledTxDMAReload(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->TXCTRL, USART_TXCTRL_CONTINUOUS) == (USART_TXCTRL_CONTINUOUS));
}

/**
 * @brief  Clear TX channel Enable
 * @note   Write only. Stop & clear transmission (= 1).
 * @rmtoll TXCTRL          CLR            LL_USART_EnableTxClean
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableTxClean(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->TXCTRL, USART_TXCTRL_CLR);
}

/**
 * @brief  Clear TX channel Disable
 * @note   Write only.
 * @rmtoll TXCTRL          CLR            LL_USART_DisableTxClean
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableTxClean(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->TXCTRL, USART_TXCTRL_CLR);
}

/**
 * @brief  Set the Transmit watermark level
 * @rmtoll TXCTRL          TXCNT          LL_USART_SetTxWatermarkLevel
 * @param  USARTx USART Instance
 * @param  WatermarkLevel Value between Min_Data=0x00 and Max_Data=0x1F
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetTxWatermarkLevel(USART_TypeDef *USARTx, uint32_t WatermarkLevel)
{
  MODIFY_REG(USARTx->TXCTRL, USART_TXCTRL_TXCNT, WatermarkLevel << USART_TXCTRL_TXCNT_POS);
}

/**
 * @brief  Retrieve the Transmit watermark level
 * @rmtoll TXCTRL          TXCNT          LL_USART_GetTxWatermarkLevel
 * @param  USARTx USART Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x1F
 */
__STATIC_INLINE uint32_t LL_USART_GetTxWatermarkLevel(const USART_TypeDef *USARTx)
{
  return (uint32_t)(READ_BIT(USARTx->TXCTRL, USART_TXCTRL_TXCNT) >> USART_TXCTRL_TXCNT_POS);
}

/**
 * @brief  Receiver Enable
 * @rmtoll RXCTRL          RXEN            LL_USART_EnableDirectionRx
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableDirectionRx(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->RXCTRL, USART_RXCTRL_RXEN);
}

/**
 * @brief  Receiver Disable
 * @rmtoll RXCTRL          RXEN            LL_USART_DisableDirectionRx
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableDirectionRx(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->RXCTRL, USART_RXCTRL_RXEN);
}

/**
 * @brief  Indicate if Receiver is enabled
 * @rmtoll RXCTRL          RXEN         LL_USART_IsEnabledDirectionRx
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledDirectionRx(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->RXCTRL, USART_RXCTRL_RXEN) == (USART_RXCTRL_RXEN));
}

/**
 * @brief  RX GDMA reload mode Enable
 * @rmtoll RXCTRL          CONTINUOUS            LL_USART_EnableRxDMAReload
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableRxDMAReload(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->RXCTRL, USART_RXCTRL_CONTINUOUS);
}

/**
 * @brief  RX GDMA reload mode Disable
 * @rmtoll RXCTRL          CONTINUOUS            LL_USART_DisableRxDMAReload
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableRxDMAReload(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->RXCTRL, USART_RXCTRL_CONTINUOUS);
}

/**
 * @brief  Indicate if RX GDMA reload mode is enabled
 * @rmtoll RXCTRL          CONTINUOUS            LL_USART_IsEnabledRxDMAReload
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledRxDMAReload(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->RXCTRL, USART_RXCTRL_CONTINUOUS) == (USART_RXCTRL_CONTINUOUS));
}

/**
 * @brief  Clear RX channel Enable
 * @note   Write only. Stop & clear reception (= 1).
 * @rmtoll RXCTRL          CLR            LL_USART_EnableRxClean
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableRxClean(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->RXCTRL, USART_RXCTRL_CLR);
}

/**
 * @brief  Clear RX channel Disable
 * @note   Write only.
 * @rmtoll RXCTRL          CLR            LL_USART_DisableRxClean
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableRxClean(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->RXCTRL, USART_RXCTRL_CLR);
}

/**
 * @brief  Set the Receive watermark level
 * @rmtoll RXCTRL          RXCNT          LL_USART_SetRxWatermarkLevel
 * @param  USARTx USART Instance
 * @param  WatermarkLevel Value between Min_Data=0x00 and Max_Data=0x1F
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetRxWatermarkLevel(USART_TypeDef *USARTx, uint32_t WatermarkLevel)
{
    MODIFY_REG(USARTx->RXCTRL, USART_RXCTRL_RXCNT, WatermarkLevel << USART_RXCTRL_RXCNT_POS);
}

/**
 * @brief  Retrieve the Receive watermark level
 * @rmtoll RXCTRL          RXCNT          LL_USART_GetRxWatermarkLevel
 * @param  USARTx USART Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x1F
 */
__STATIC_INLINE uint32_t LL_USART_GetRxWatermarkLevel(const USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->RXCTRL, USART_RXCTRL_RXCNT) >> USART_RXCTRL_RXCNT_POS);
}

/**
 * @brief  Configure USART DIV register for achieving expected Baud Rate value.
 * @note   Compute and set USARTDIV value in DIV Register (full DIV content)
 *         according to used Peripheral Clock, and expected Baud Rate values
 * @note   Peripheral clock and Baud rate values provided as function parameters should be valid
 *         (Baud rate value != 0, BAUD_DIV[20:0] should not be 0 or 1)
 * @rmtoll DIV          BAUD_DIV           LL_USART_SetBaudRate
 * @param  USARTx USART Instance
 * @param  PeriphClk Peripheral Clock (Hz)
 * @param  BaudRate Baud Rate (bps)
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetBaudRate(USART_TypeDef *USARTx, uint32_t PeriphClk, uint32_t BaudRate)
{
    if (((USARTx->SETUP >> USART_SETUP_FRACTION_BAUD_POS) & 0x1) == 0) {
        USARTx->DIV = (uint32_t)(__LL_USART_CALC_BAUDRATE_DIVMANT(PeriphClk, BaudRate));
    } else {
        USARTx->DIV = (uint32_t)(__LL_USART_CALC_BAUDRATE_DIVFRAC(PeriphClk, BaudRate));
    }
}

/**
 * @brief  Return current Baud Rate value, according to USARTDIV present in DIV register
 *         (full DIV content), and to used Peripheral Clock and Oversampling mode values
 * @note   In case of non-initialized or invalid value stored in BRR register, value 0 will be returned.
 * @rmtoll DIV          BAUD_DIV           LL_USART_GetBaudRate
 * @param  USARTx USART Instance
 * @param  PeriphClk Peripheral Clock
 * @retval Baud Rate
 */
__STATIC_INLINE uint32_t LL_USART_GetBaudRate(const USART_TypeDef *USARTx, uint32_t PeriphClk)
{
    uint32_t usartdiv = 0x0U;
    float fractionaldiv = 0.0f;
    uint32_t baudresult = 0x0U;

    usartdiv = USARTx->DIV;

    if (((USARTx->SETUP >> USART_SETUP_FRACTION_BAUD_POS) & 0x1) == 0) {
        baudresult = PeriphClk / (usartdiv + 1);
    } else {
        fractionaldiv = (usartdiv >> 3) + (usartdiv & 0x7) * 0.125f;
        baudresult = PeriphClk / (fractionaldiv + 1);
    }
    return (baudresult);
}

/**
 * @brief  Configure Parity (enabled/disabled and parity mode if enabled).
 * @note   This function selects if hardware parity control (generation and detection) is enabled or disabled.
 *         When the parity control is enabled (Odd or Even), computed parity bit is inserted at the MSB position
 *         (9th or 8th bit depending on data width) and parity is checked on the received data.
 * @rmtoll SETUP          PARITY_EN            LL_USART_SetParity\n
 *         SETUP          PARITY_SEL           LL_USART_SetParity
 * @param  USARTx USART Instance
 * @param  Parity This parameter can be one of the following values:
 *         @arg @ref LL_USART_PARITY_NONE
 *         @arg @ref LL_USART_PARITY_EVEN
 *         @arg @ref LL_USART_PARITY_ODD
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetParity(USART_TypeDef *USARTx, uint32_t Parity)
{
    MODIFY_REG(USARTx->SETUP, USART_SETUP_PARITY_EN | USART_SETUP_PARITY_SEL, Parity);
}

/**
 * @brief  Return Parity configuration (enabled/disabled and parity mode if enabled)
 * @rmtoll SETUP          PARITY_EN            LL_USART_GetParity\n
 *         SETUP          PARITY_SEL           LL_USART_GetParity
 * @param  USARTx USART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_USART_PARITY_NONE
 *         @arg @ref LL_USART_PARITY_EVEN
 *         @arg @ref LL_USART_PARITY_ODD
 */
__STATIC_INLINE uint32_t LL_USART_GetParity(const USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->SETUP, USART_SETUP_PARITY_EN | USART_SETUP_PARITY_SEL));
}

/**
 * @brief  Enable Clear RX FIFO on all mode
 * @rmtoll SETUP          CLEAN_FIFO         LL_USART_EnableFIFOClean
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableFIFOClean(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->SETUP, USART_SETUP_CLEAN_FIFO);
}

/**
 * @brief  Disable Clear RX FIFO on all mode
 * @rmtoll SETUP          CLEAN_FIFO         LL_USART_DisableFIFOClean
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableFIFOClean(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->SETUP, USART_SETUP_CLEAN_FIFO);
}

/**
 * @brief  Indicate if Clear RX FIFO on all mode is enabled
 * @rmtoll SETUP          CLEAN_FIFO         LL_USART_IsEnabledFIFOClean
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledFIFOClean(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->SETUP, USART_SETUP_CLEAN_FIFO) == (USART_SETUP_CLEAN_FIFO));
}

/**
 * @brief  Set Word length (i.e. nb of data bits, excluding start and stop bits)
 * @rmtoll SETUP          BIT_LENGTH             LL_USART_SetDataWidth
 * @param  USARTx USART Instance
 * @param  DataWidth This parameter can be one of the following values:
 *         @arg @ref LL_USART_DATAWIDTH_5B
 *         @arg @ref LL_USART_DATAWIDTH_6B
 *         @arg @ref LL_USART_DATAWIDTH_7B
 *         @arg @ref LL_USART_DATAWIDTH_8B
 *         @arg @ref LL_USART_DATAWIDTH_9B
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetDataWidth(USART_TypeDef *USARTx, uint32_t DataWidth)
{
    MODIFY_REG(USARTx->SETUP, USART_SETUP_BIT_LENGTH, DataWidth);
}

/**
 * @brief  Return Word length (i.e. nb of data bits, excluding start and stop bits)
 * @rmtoll SETUP          BIT_LENGTH             LL_USART_GetDataWidth
 * @param  USARTx USART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_USART_DATAWIDTH_5B
 *         @arg @ref LL_USART_DATAWIDTH_6B
 *         @arg @ref LL_USART_DATAWIDTH_7B
 *         @arg @ref LL_USART_DATAWIDTH_8B
 *         @arg @ref LL_USART_DATAWIDTH_9B
 */
__STATIC_INLINE uint32_t LL_USART_GetDataWidth(const USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->SETUP, USART_SETUP_BIT_LENGTH));
}

/**
 * @brief  Configure Character frame format (Datawidth, Parity control, Stop Bits)
 * @note   Call of this function is equivalent to following function call sequence :
 *         - Data Width configuration using @ref LL_USART_SetDataWidth() function
 *         - Parity Control and mode configuration using @ref LL_USART_SetParity() function
 *         - Stop bits configuration using @ref LL_USART_SetStopBitsLength() function
 * @rmtoll SETUP          BIT_LENGTH            LL_USART_ConfigCharacter\n
 *         SETUP          PARITY_EN             LL_USART_ConfigCharacter\n
 *         SETUP          PARITY_SEL            LL_USART_ConfigCharacter\n
 *         TXCTRL         CFG_STOP_BIT          LL_USART_ConfigCharacter
 * @param  USARTx USART Instance
 * @param  DataWidth This parameter can be one of the following values:
 *         @arg @ref LL_USART_DATAWIDTH_5B
 *         @arg @ref LL_USART_DATAWIDTH_6B
 *         @arg @ref LL_USART_DATAWIDTH_7B
 *         @arg @ref LL_USART_DATAWIDTH_8B
 *         @arg @ref LL_USART_DATAWIDTH_9B
 * @param  Parity This parameter can be one of the following values:
 *         @arg @ref LL_USART_PARITY_NONE
 *         @arg @ref LL_USART_PARITY_EVEN
 *         @arg @ref LL_USART_PARITY_ODD
 * @param  StopBits This parameter can be one of the following values:
 *         @arg @ref LL_USART_STOPBITS_0_5
 *         @arg @ref LL_USART_STOPBITS_1
 *         @arg @ref LL_USART_STOPBITS_1_5
 *         @arg @ref LL_USART_STOPBITS_2
 * @retval None
 */
__STATIC_INLINE void LL_USART_ConfigCharacter(USART_TypeDef *USARTx, uint32_t DataWidth, uint32_t Parity,
                                              uint32_t StopBits)
{
    MODIFY_REG(USARTx->SETUP, USART_SETUP_PARITY_EN | USART_SETUP_PARITY_SEL | USART_SETUP_BIT_LENGTH, Parity | DataWidth);
    MODIFY_REG(USARTx->TXCTRL, USART_TXCTRL_CFG_STOP_BIT, StopBits);
}

/**
 * @brief  Enable Clock output on SCLK pin
 * @note   Macro IS_USART_INSTANCE(USARTx) can be used to check whether or not
 *         Synchronous mode is supported by the USARTx instance.
 * @rmtoll SETUP          CLK_EN         LL_USART_EnableSCLKOutput
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableSCLKOutput(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->SETUP, USART_SETUP_CLK_EN);
}

/**
 * @brief  Disable Clock output on SCLK pin
 * @note   Macro IS_USART_INSTANCE(USARTx) can be used to check whether or not
 *         Synchronous mode is supported by the USARTx instance.
 * @rmtoll SETUP          CLK_EN         LL_USART_DisableSCLKOutput
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableSCLKOutput(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->SETUP, USART_SETUP_CLK_EN);
}

/**
 * @brief  Indicate if Clock output on SCLK pin is enabled
 * @note   Macro IS_USART_INSTANCE(USARTx) can be used to check whether or not
 *         Synchronous mode is supported by the USARTx instance.
 * @rmtoll SETUP          CLK_EN         LL_USART_IsEnabledSCLKOutput
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledSCLKOutput(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->SETUP, USART_SETUP_CLK_EN) == (USART_SETUP_CLK_EN));
}

/**
 * @brief  Select the polarity of the clock output on the SCLK pin in synchronous mode
 * @note   Macro IS_USART_INSTANCE(USARTx) can be used to check whether or not
 *         Synchronous mode is supported by the USARTx instance.
 * @rmtoll SETUP          CPOL          LL_USART_SetClockPolarity
 * @param  USARTx USART Instance
 * @param  ClockPolarity This parameter can be one of the following values:
 *         @arg @ref LL_USART_POLARITY_LOW
 *         @arg @ref LL_USART_POLARITY_HIGH
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetClockPolarity(USART_TypeDef *USARTx, uint32_t ClockPolarity)
{
    MODIFY_REG(USARTx->SETUP, USART_SETUP_CPOL, ClockPolarity);
}

/**
 * @brief  Return polarity of the clock output on the SCLK pin in synchronous mode
 * @note   Macro IS_USART_INSTANCE(USARTx) can be used to check whether or not
 *         Synchronous mode is supported by the USARTx instance.
 * @rmtoll SETUP          CPOL          LL_USART_GetClockPolarity
 * @param  USARTx USART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_USART_POLARITY_LOW
 *         @arg @ref LL_USART_POLARITY_HIGH
 */
__STATIC_INLINE uint32_t LL_USART_GetClockPolarity(const USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->SETUP, USART_SETUP_CPOL));
}

/**
 * @brief  Select the phase of the clock output on the SCLK pin in synchronous mode
 * @note   Macro IS_USART_INSTANCE(USARTx) can be used to check whether or not
 *         Synchronous mode is supported by the USARTx instance.
 * @rmtoll SETUP          CPHA          LL_USART_SetClockPhase
 * @param  USARTx USART Instance
 * @param  ClockPhase This parameter can be one of the following values:
 *         @arg @ref LL_USART_PHASE_1EDGE
 *         @arg @ref LL_USART_PHASE_2EDGE
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetClockPhase(USART_TypeDef *USARTx, uint32_t ClockPhase)
{
    MODIFY_REG(USARTx->SETUP, USART_SETUP_CPHA, ClockPhase);
}

/**
 * @brief  Return phase of the clock output on the SCLK pin in synchronous mode
 * @note   Macro IS_USART_INSTANCE(USARTx) can be used to check whether or not
 *         Synchronous mode is supported by the USARTx instance.
 * @rmtoll SETUP          CPHA          LL_USART_GetClockPhase
 * @param  USARTx USART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_USART_PHASE_1EDGE
 *         @arg @ref LL_USART_PHASE_2EDGE
 */
__STATIC_INLINE uint32_t LL_USART_GetClockPhase(const USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->SETUP, USART_SETUP_CPHA));
}

/**
 * @brief  Configure if Clock pulse of the last data bit is output to the SCLK pin or not
 * @note   Macro IS_USART_INSTANCE(USARTx) can be used to check whether or not
 *         Synchronous mode is supported by the USARTx instance.
 * @rmtoll SETUP          LBCL          LL_USART_SetLastClkPulseOutput
 * @param  USARTx USART Instance
 * @param  LastBitClockPulse This parameter can be one of the following values:
 *         @arg @ref LL_USART_LASTCLKPULSE_NO_OUTPUT
 *         @arg @ref LL_USART_LASTCLKPULSE_OUTPUT
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetLastClkPulseOutput(USART_TypeDef *USARTx, uint32_t LastBitClockPulse)
{
    MODIFY_REG(USARTx->SETUP, USART_SETUP_LBCL, LastBitClockPulse);
}

/**
 * @brief  Retrieve Clock pulse of the last data bit output configuration
 *         (Last bit Clock pulse output to the SCLK pin or not)
 * @note   Macro IS_USART_INSTANCE(USARTx) can be used to check whether or not
 *         Synchronous mode is supported by the USARTx instance.
 * @rmtoll SETUP          LBCL          LL_USART_GetLastClkPulseOutput
 * @param  USARTx USART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_USART_LASTCLKPULSE_NO_OUTPUT
 *         @arg @ref LL_USART_LASTCLKPULSE_OUTPUT
 */
__STATIC_INLINE uint32_t LL_USART_GetLastClkPulseOutput(const USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->SETUP, USART_SETUP_LBCL));
}

/**
 * @brief  Configure Clock signal format (Phase Polarity and choice about output of last bit clock pulse)
 * @note   Macro IS_USART_INSTANCE(USARTx) can be used to check whether or not
 *         Synchronous mode is supported by the USARTx instance.
 * @note   Call of this function is equivalent to following function call sequence :
 *         - Clock Phase configuration using @ref LL_USART_SetClockPhase() function
 *         - Clock Polarity configuration using @ref LL_USART_SetClockPolarity() function
 *         - Output of Last bit Clock pulse configuration using @ref LL_USART_SetLastClkPulseOutput() function
 * @rmtoll SETUP          CPHA          LL_USART_ConfigClock\n
 *         SETUP          CPOL          LL_USART_ConfigClock\n
 *         SETUP          LBCL          LL_USART_ConfigClock
 * @param  USARTx USART Instance
 * @param  Phase This parameter can be one of the following values:
 *         @arg @ref LL_USART_PHASE_1EDGE
 *         @arg @ref LL_USART_PHASE_2EDGE
 * @param  Polarity This parameter can be one of the following values:
 *         @arg @ref LL_USART_POLARITY_LOW
 *         @arg @ref LL_USART_POLARITY_HIGH
 * @param  LBCPOutput This parameter can be one of the following values:
 *         @arg @ref LL_USART_LASTCLKPULSE_NO_OUTPUT
 *         @arg @ref LL_USART_LASTCLKPULSE_OUTPUT
 * @retval None
 */
__STATIC_INLINE void LL_USART_ConfigClock(USART_TypeDef *USARTx, uint32_t Phase, uint32_t Polarity, uint32_t LBCPOutput)
{
    MODIFY_REG(USARTx->SETUP, USART_SETUP_CPHA | USART_SETUP_CPOL | USART_SETUP_LBCL, Phase | Polarity | LBCPOutput);
}

/**
 * @brief  Enable RTS HW Flow Control
 * @rmtoll SETUP          RTSE          LL_USART_EnableRTSHWFlowCtrl
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableRTSHWFlowCtrl(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->SETUP, USART_SETUP_RTSE);
}

/**
 * @brief  Disable RTS HW Flow Control
 * @rmtoll SETUP          RTSE          LL_USART_DisableRTSHWFlowCtrl
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableRTSHWFlowCtrl(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->SETUP, USART_SETUP_RTSE);
}

/**
 * @brief  Enable CTS HW Flow Control
 * @rmtoll SETUP          CTSE          LL_USART_EnableCTSHWFlowCtrl
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableCTSHWFlowCtrl(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->SETUP, USART_SETUP_CTSE);
}

/**
 * @brief  Disable CTS HW Flow Control
 * @rmtoll SETUP          CTSE          LL_USART_DisableCTSHWFlowCtrl
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableCTSHWFlowCtrl(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->SETUP, USART_SETUP_CTSE);
}

/**
 * @brief  Configure HW Flow Control mode (both CTS and RTS)
 * @note   Macro IS_UART_HWFLOW_INSTANCE(USARTx) can be used to check whether or not
 *         Hardware Flow control feature is supported by the USARTx instance.
 * @rmtoll SETUP          RTSE          LL_USART_SetHWFlowCtrl\n
 *         SETUP          CTSE          LL_USART_SetHWFlowCtrl
 * @param  USARTx USART Instance
 * @param  HardwareFlowControl This parameter can be one of the following values:
 *         @arg @ref LL_USART_HWCONTROL_NONE
 *         @arg @ref LL_USART_HWCONTROL_RTS
 *         @arg @ref LL_USART_HWCONTROL_CTS
 *         @arg @ref LL_USART_HWCONTROL_RTS_CTS
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetHWFlowCtrl(USART_TypeDef *USARTx, uint32_t HardwareFlowControl)
{
    MODIFY_REG(USARTx->SETUP, USART_SETUP_RTSE | USART_SETUP_CTSE, HardwareFlowControl);
}

/**
 * @brief  Return HW Flow Control configuration (both CTS and RTS)
 * @note   Macro IS_UART_HWFLOW_INSTANCE(USARTx) can be used to check whether or not
 *         Hardware Flow control feature is supported by the USARTx instance.
 * @rmtoll SETUP          RTSE          LL_USART_GetHWFlowCtrl\n
 *         SETUP          CTSE          LL_USART_GetHWFlowCtrl
 * @param  USARTx USART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_USART_HWCONTROL_NONE
 *         @arg @ref LL_USART_HWCONTROL_RTS
 *         @arg @ref LL_USART_HWCONTROL_CTS
 *         @arg @ref LL_USART_HWCONTROL_RTS_CTS
 */
__STATIC_INLINE uint32_t LL_USART_GetHWFlowCtrl(const USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->SETUP, USART_SETUP_RTSE | USART_SETUP_CTSE));
}

/**
 * @brief  Enable One bit sampling method (disable 2/3 majority vote)
 * @rmtoll SETUP          VOTE_MAJORITY_EN        LL_USART_EnableOneBitSamp
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableOneBitSamp(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->SETUP, USART_SETUP_VOTE_MAJORITY_EN);
}

/**
 * @brief  Disable One bit sampling method (enable 2/3 majority vote)
 * @rmtoll SETUP          VOTE_MAJORITY_EN        LL_USART_DisableOneBitSamp
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableOneBitSamp(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->SETUP, USART_SETUP_VOTE_MAJORITY_EN);
}

/**
 * @brief  Indicate if One bit sampling method is enabled
 * @rmtoll SETUP          VOTE_MAJORITY_EN        LL_USART_IsEnabledOneBitSamp
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledOneBitSamp(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->SETUP, USART_SETUP_VOTE_MAJORITY_EN) == (USART_SETUP_VOTE_MAJORITY_EN));
}

/**
 * @brief  Set Sampling point
 * @rmtoll SETUP          SAMPLE_POSITION          LL_USART_SetSamplingPosition
 * @param  USARTx USART Instance
 * @param  SamplingPosition This parameter can be one of the following values:
 *         @arg @ref LL_USART_SAMPLE_POSITION_SPREAD
 *         @arg @ref LL_USART_SAMPLE_POSITION_DOWN
 *         @arg @ref LL_USART_SAMPLE_POSITION_UP
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetSamplingPosition(USART_TypeDef *USARTx, uint32_t SamplingPosition)
{
  MODIFY_REG(USARTx->SETUP, USART_SETUP_SAMPLE_POSITION, SamplingPosition);
}

/**
 * @brief  Get Sampling point
 * @rmtoll SETUP          SAMPLE_POSITION          LL_USART_GetSamplingPosition
 * @param  USARTx USART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_USART_SAMPLE_POSITION_SPREAD
 *         @arg @ref LL_USART_SAMPLE_POSITION_DOWN
 *         @arg @ref LL_USART_SAMPLE_POSITION_UP
 */
__STATIC_INLINE uint32_t LL_USART_GetSamplingPosition(const USART_TypeDef *USARTx)
{
  return (uint32_t)(READ_BIT(USARTx->SETUP, USART_SETUP_SAMPLE_POSITION));
}

/**
 * @brief  Enable Fractional baud enable
 * @rmtoll SETUP          FRACTION_BAUD        LL_USART_EnableFraction
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableFraction(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->SETUP, USART_SETUP_FRACTION_BAUD);
}

/**
 * @brief  Disable Fractional baud enable
 * @rmtoll SETUP          FRACTION_BAUD        LL_USART_DisableFraction
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableFraction(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->SETUP, USART_SETUP_FRACTION_BAUD);
}

/**
 * @brief  Indicate if Fractional baud enable is enabled
 * @rmtoll SETUP          FRACTION_BAUD        LL_USART_IsEnabledFraction
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledFraction(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->SETUP, USART_SETUP_FRACTION_BAUD) == (USART_SETUP_FRACTION_BAUD));
}

/**
 * @brief  Set total RX data size (Remaining RX data size)
 * @rmtoll RX_SIZE          RX_SIZE          LL_USART_SetRxSize
 * @param  USARTx USART Instance
 * @param  RxSizeValue Value between Min_Data=0x00 and Max_Data=0xFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetRxSize(USART_TypeDef *USARTx, uint32_t RxSizeValue)
{
    MODIFY_REG(USARTx->RX_SIZE, USART_RX_SIZE_RX_SIZE, RxSizeValue);
}

/**
 * @brief  Get total RX data size (Remaining RX data size)
 * @rmtoll RX_SIZE          RX_SIZE          LL_USART_GetRxSize
 * @param  USARTx USART Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFFF
 */
__STATIC_INLINE uint32_t LL_USART_GetRxSize(const USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->RX_SIZE, USART_RX_SIZE_RX_SIZE));
}

/**
 * @brief  Set total TX data size (Remaining TX data size)
 * @rmtoll TX_SIZE          TX_SIZE          LL_USART_SetTxSize
 * @param  USARTx USART Instance
 * @param  TxSizeValue Value between Min_Data=0x00 and Max_Data=0xFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetTxSize(USART_TypeDef *USARTx, uint32_t TxSizeValue)
{
    MODIFY_REG(USARTx->TX_SIZE, USART_TX_SIZE_TX_SIZE, TxSizeValue);
}

/**
 * @brief  Get total TX data size (Remaining TX data size)
 * @rmtoll TX_SIZE          TX_SIZE          LL_USART_GetTxSize
 * @param  USARTx USART Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFFF
 */
__STATIC_INLINE uint32_t LL_USART_GetTxSize(const USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->TX_SIZE, USART_TX_SIZE_TX_SIZE));
}

/**
 * @brief  Set Timeout value for idle detection
 * @rmtoll RX_IDLE          IDLE_TOUT_NUM          LL_USART_SetIDLETimeout
 * @param  USARTx USART Instance
 * @param  IDLETimeout Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetIDLETimeout(USART_TypeDef *USARTx, uint32_t IDLETimeout)
{
    MODIFY_REG(USARTx->RX_IDLE, USART_RX_IDLE_IDLE_TOUT_NUM, IDLETimeout);
}

/**
 * @brief  Get Timeout value for idle detection
 * @rmtoll RX_IDLE          IDLE_TOUT_NUM          LL_USART_GetIDLETimeout
 * @param  USARTx USART Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_USART_GetIDLETimeout(const USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->RX_IDLE, USART_RX_IDLE_IDLE_TOUT_NUM));
}

/**
 * @brief  Set Timeout value for Watermark detection
 * @rmtoll RX_WM          WM_TOUT_NUM          LL_USART_SetWMTimeout
 * @param  USARTx USART Instance
 * @param  WMTimeout Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetWMTimeout(USART_TypeDef *USARTx, uint32_t WMTimeout)
{
    MODIFY_REG(USARTx->RX_WM, USART_RX_WM_WM_TOUT_NUM, WMTimeout);
}

/**
 * @brief  Get Timeout value for Watermark detection
 * @rmtoll RX_IDLE          WM_TOUT_NUM          LL_USART_GetWMTimeout
 * @param  USARTx USART Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_USART_GetWMTimeout(const USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->RX_WM, USART_RX_WM_WM_TOUT_NUM));
}

/**
 * @brief  Get Free entries in RX FIFO
 * @rmtoll RX_FIFO_LEFT_ENTRY          LEFT_ENTRY          LL_USART_GetRXFIFOFreeLevel
 * @param  USARTx USART Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x1F
 */
__STATIC_INLINE uint32_t LL_USART_GetRxFIFOFreeLevel(USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->RX_FIFO_LEFT_ENTRY, USART_RX_FIFO_LEFT_ENTRY_LEFT_ENTRY));
}

/**
 * @brief  Get Free entries in TX FIFO
 * @rmtoll TX_FIFO_LEFT_ENTRY          LEFT_ENTRY          LL_USART_GetTxFIFOFreeLevel
 * @param  USARTx USART Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x1F
 */
__STATIC_INLINE uint32_t LL_USART_GetTxFIFOFreeLevel(USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->TX_FIFO_LEFT_ENTRY, USART_TX_FIFO_LEFT_ENTRY_LEFT_ENTRY));
}

/**
 * @brief  Get Low-level duration in clock cycles
 * @rmtoll RX_LOW_LEVEL_CNT          RX_LOW_LEVEL_LENGTH          LL_USART_GetRxLowLevelLength
 * @param  USARTx USART Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFFF
 */
__STATIC_INLINE uint32_t LL_USART_GetRxLowLevelLength(USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->RX_LOW_LEVEL_CNT, USART_RX_LOW_LEVEL_CNT_RX_LOW_LEVEL_LENGTH));
}

/**
 * @}
 */

/** @defgroup USART_LL_EF_Configuration_HalfDuplex Configuration functions related to Half Duplex feature
 * @{
 */

/**
 * @brief  Enable Single Wire Half-Duplex mode
 * @rmtoll SETUP          HDSEL         LL_USART_EnableHalfDuplex
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableHalfDuplex(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->SETUP, USART_SETUP_HDSEL);
}

/**
 * @brief  Disable Single Wire Half-Duplex mode
 * @rmtoll SETUP          HDSEL         LL_USART_DisableHalfDuplex
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableHalfDuplex(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->SETUP, USART_SETUP_HDSEL);
}

/**
 * @brief  Indicate if Single Wire Half-Duplex mode is enabled
 * @rmtoll SETUP          HDSEL         LL_USART_IsEnabledHalfDuplex
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledHalfDuplex(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->SETUP, USART_SETUP_HDSEL) == (USART_SETUP_HDSEL));
}

/**
 * @}
 */

/** @defgroup USART_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable TX watermark interrupt
 * @rmtoll IE          TXIE        LL_USART_EnableIT_TX
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableIT_TX(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->IE, USART_IE_TXIE);
}

/**
 * @brief  Disable TX watermark interrupt
 * @rmtoll IE          TXIE        LL_USART_DisableIT_TX
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableIT_TX(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->IE, USART_IE_TXIE);
}

/**
 * @brief  Check if the USART TX watermark interrupt is enabled or disabled.
 * @rmtoll IE          TXIE        LL_USART_IsEnabledIT_TX
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledIT_TX(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->IE, USART_IE_TXIE) == (USART_IE_TXIE));
}

/**
 * @brief  Enable RX watermark interrupt
 * @rmtoll IE          RXIE        LL_USART_EnableIT_RX
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableIT_RX(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->IE, USART_IE_RXIE);
}

/**
 * @brief  Disable RX watermark interrupt
 * @rmtoll IE          RXIE        LL_USART_DisableIT_RX
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableIT_RX(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->IE, USART_IE_RXIE);
}

/**
 * @brief  Check if the USART RX watermark interrupt is enabled or disabled.
 * @rmtoll IE          RXIE        LL_USART_IsEnabledIT_RX
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledIT_RX(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->IE, USART_IE_RXIE) == (USART_IE_RXIE));
}

/**
 * @brief  Enable RX overflow interrupt
 * @rmtoll IE          OWIE        LL_USART_EnableIT_OVF
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableIT_OVF(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->IE, USART_IE_OWIE);
}

/**
 * @brief  Disable RX overflow interrupt
 * @rmtoll IE          OWIE        LL_USART_DisableIT_OVF
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableIT_OVF(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->IE, USART_IE_OWIE);
}

/**
 * @brief  Check if the USART RX overflow interrupt is enabled or disabled.
 * @rmtoll IE          OW_IE        LL_USART_IsEnabledIT_OVF
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledIT_OVF(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->IE, USART_IE_OWIE) == (USART_IE_OWIE));
}

/**
 * @brief  Enable RX parity error interrupt
 * @rmtoll IE          PARITY_ERR_IE        LL_USART_EnableIT_RXPARERR
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableIT_RXPARERR(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->IE, USART_IE_PARITY_ERR_IE);
}

/**
 * @brief  Disable RX parity error interrupt
 * @rmtoll IE          PARITY_ERR_IE        LL_USART_DisableIT_RXPARERR
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableIT_RXPARERR(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->IE, USART_IE_PARITY_ERR_IE);
}

/**
 * @brief  Check if the USART RX parity error interrupt is enabled or disabled.
 * @rmtoll IE          PARITY_ERR_IE        LL_USART_IsEnabledIT_RXPARERR
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledIT_RXPARERR(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->IE, USART_IE_PARITY_ERR_IE) == (USART_IE_PARITY_ERR_IE));
}

/**
 * @brief  Enable CTS rising edge interrupt
 * @rmtoll IE          CTSIE_RISE_IE        LL_USART_EnableIT_CTSTAISE
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableIT_CTSTAISE(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->IE, USART_IE_CTSIE_RISE_IE);
}

/**
 * @brief  Disable CTS rising edge interrupt
 * @rmtoll IE          CTSIE_RISE_EN        LL_USART_DisableIT_CTSTAISE
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableIT_CTSTAISE(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->IE, USART_IE_CTSIE_RISE_IE);
}

/**
 * @brief  Check if the USART CTS rising edge interrupt is enabled or disabled.
 * @rmtoll IE          CTSIE_RISE_EN        LL_USART_IsEnabledIT_CTSTAISE
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledIT_CTSTAISE(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->IE, USART_IE_CTSIE_RISE_IE) == (USART_IE_CTSIE_RISE_IE));
}

/**
 * @brief  Enable CTS falling edge interrupt
 * @rmtoll IE          CTSIE_FALL_IE        LL_USART_EnableIT_CTSFALL
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableIT_CTSFALL(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->IE, USART_IE_CTSIE_FALL_IE);
}

/**
 * @brief  Disable CTS falling edge interrupt
 * @rmtoll IE          CTSIE_FALL_IE        LL_USART_DisableIT_CTSFALL
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableIT_CTSFALL(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->IE, USART_IE_CTSIE_FALL_IE);
}

/**
 * @brief  Check if the USART CTS falling edge interrupt is enabled or disabled.
 * @rmtoll IE          CTSIE_FALL_IE        LL_USART_IsEnabledIT_CTSFALL
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledIT_CTSFALL(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->IE, USART_IE_CTSIE_FALL_IE) == (USART_IE_CTSIE_FALL_IE));
}

/**
 * @brief  Enable RX frame error interrupt
 * @rmtoll IE          RX_FRAME_ERR_EN        LL_USART_EnableIT_RXFRAMEERR
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableIT_RXFRAMEERR(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->IE, USART_IE_RX_FRAME_ERR_IE);
}

/**
 * @brief  Disable RX frame error interrupt
 * @rmtoll IE          RX_FRAME_ERR_IE        LL_USART_DisableIT_RXFRAMEERR
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableIT_RXFRAMEERR(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->IE, USART_IE_RX_FRAME_ERR_IE);
}

/**
 * @brief  Check if the USART RX frame error interrupt is enabled or disabled.
 * @rmtoll IE          RX_FRAME_ERR_EN        LL_USART_IsEnabledIT_RXFRAMEERR
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledIT_RXFRAMEERR(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->IE, USART_IE_RX_FRAME_ERR_IE) == (USART_IE_RX_FRAME_ERR_IE));
}

/**
 * @brief  Enable Stop bit error interrupt
 * @rmtoll IE          STOP_BIT_ERR_IE        LL_USART_EnableIT_STOPBITERR
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableIT_STOPBITERR(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->IE, USART_IE_STOP_BIT_ERR_IE);
}

/**
 * @brief  Disable Stop bit error interrupt
 * @rmtoll IE          STOP_BIT_ERR_IE        LL_USART_DisableIT_STOPBITERR
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableIT_STOPBITERR(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->IE, USART_IE_STOP_BIT_ERR_IE);
}

/**
 * @brief  Check if the USART Stop bit error interrupt is enabled or disabled.
 * @rmtoll IE          STOP_BIT_ERR_IE        LL_USART_IsEnabledIT_STOPBITERR
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledIT_STOPBITERR(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->IE, USART_IE_STOP_BIT_ERR_IE) == (USART_IE_STOP_BIT_ERR_IE));
}

/**
 * @brief  Enable RX idle timeout interrupt
 * @rmtoll IE          RX_IDLE_TOUT_IE        LL_USART_EnableIT_RXIDLETO
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableIT_RXIDLETO(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->IE, USART_IE_RX_IDLE_TOUT_IE);
}

/**
 * @brief  Disable RX idle timeout interrupt
 * @rmtoll IE          RX_IDLE_TOUT_IE        LL_USART_DisableIT_RXIDLETO
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableIT_RXIDLETO(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->IE, USART_IE_RX_IDLE_TOUT_IE);
}

/**
 * @brief  Check if the USART RX idle timeout interrupt is enabled or disabled.
 * @rmtoll IE          RX_IDLE_TOUT_IE        LL_USART_IsEnabledIT_RXIDLETO
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledIT_RXIDLETO(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->IE, USART_IE_RX_IDLE_TOUT_IE) == (USART_IE_RX_IDLE_TOUT_IE));
}

/**
 * @brief  Enable RX watermark timeout interrupt
 * @rmtoll IE          RX_WM_TOUT_IE        LL_USART_EnableIT_RXWMTO
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableIT_RXWMTO(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->IE, USART_IE_RX_WM_TOUT_IE);
}

/**
 * @brief  Disable RX watermark timeout interrupt
 * @rmtoll IE          RX_WM_TOUT_IE        LL_USART_DisableIT_RXWMTO
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableIT_RXWMTO(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->IE, USART_IE_RX_WM_TOUT_IE);
}

/**
 * @brief  Check if the USART RX watermark timeout interrupt is enabled or disabled.
 * @rmtoll IE          RX_WM_TOUT_IE        LL_USART_IsEnabledIT_RXWMTO
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledIT_RXWMTO(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->IE, USART_IE_RX_WM_TOUT_IE) == (USART_IE_RX_WM_TOUT_IE));
}

/**
 * @brief  Enable TX transfer complete interrupt
 * @rmtoll IE          TX_EOT_IE        LL_USART_EnableIT_TXTC
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableIT_TXTC(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->IE, USART_IE_TX_EOT_IE);
}

/**
 * @brief  Disable TX transfer complete interrupt
 * @rmtoll IE          TX_EOT_IE        LL_USART_DisableIT_TXTC
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableIT_TXTC(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->IE, USART_IE_TX_EOT_IE);
}

/**
 * @brief  Check if the USART TX transfer complete interrupt is enabled or disabled.
 * @rmtoll IE          TX_EOT_IE        LL_USART_IsEnabledIT_TXTC
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledIT_TXTC(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->IE, USART_IE_TX_EOT_IE) == (USART_IE_TX_EOT_IE));
}

/**
 * @brief  Enable RX transfer complete interrupt
 * @rmtoll IE          RX_EOT_IE        LL_USART_EnableIT_RXTC
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableIT_RXTC(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->IE, USART_IE_RX_EOT_IE);
}

/**
 * @brief  Disable RX transfer complete interrupt
 * @rmtoll IE          RX_EOT_IE        LL_USART_DisableIT_RXTC
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableIT_RXTC(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->IE, USART_IE_RX_EOT_IE);
}

/**
 * @brief  Check if the USART RX transfer complete interrupt is enabled or disabled.
 * @rmtoll IE          RX_EOT_IE        LL_USART_IsEnabledIT_RXTC
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledIT_RXTC(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->IE, USART_IE_RX_EOT_IE) == (USART_IE_RX_EOT_IE));
}

/**
 * @}
 */

/** @defgroup USART_LL_EF_FLAG_management FLAG_management
 * @{
 */

/**
 * @brief  Check if the USART TX interrupt pending Flag is set or not
 * @note   Read only. FIFO level < TXCNT.
 * @rmtoll STA           TXIP            LL_USART_IsActiveFlag_TXWM
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_TXWM(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_TXIP) == (USART_STA_TXIP));
}

/**
 * @brief  Check if the USART RX interrupt pending Flag is set or not
 * @note   Read only. FIFO level > RXCNT.
 * @rmtoll STA           RXIP            LL_USART_IsActiveFlag_RXWM
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_RXWM(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_RXIP) == (USART_STA_RXIP));
}

/**
 * @brief  Check if the USART TX busy Flag is set or not
 * @note   Read only.
 * @rmtoll STA           TX_BUSY            LL_USART_IsActiveFlag_TXBUSY
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_TXBUSY(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_TX_BUSY) == (USART_STA_TX_BUSY));
}

/**
 * @brief  Check if the USART RX busy Flag is set or not
 * @note   Read only.
 * @rmtoll STA           RX_BUSY            LL_USART_IsActiveFlag_RXBUSY
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_RXBUSY(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_RX_BUSY) == (USART_STA_RX_BUSY));
}

/**
 * @brief  Check if the USART RX overflow error Flag is set or not
 * @rmtoll STA           RX_ERROR_FLOW            LL_USART_IsActiveFlag_RXOVF
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_RXOVF(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_RX_ERROR_FLOW) == (USART_STA_RX_ERROR_FLOW));
}

/**
 * @brief  Clear RX overflow error Flag
 * @rmtoll STA           RX_ERROR_FLOW            LL_USART_ClearFlag_RXOVF
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_ClearFlag_RXOVF(USART_TypeDef *USARTx)
{
    WRITE_REG(USARTx->STA, USART_STA_RX_ERROR_FLOW);
}

/**
 * @brief  Check if the USART RX parity error Flag is set or not
 * @rmtoll STA           RX_ERROR_PARITY            LL_USART_IsActiveFlag_RXPARERR
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_RXPARERR(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_RX_ERROR_PARITY) == (USART_STA_RX_ERROR_PARITY));
}

/**
 * @brief  Clear RX parity error Flag
 * @rmtoll STA           RX_ERROR_PARITY            LL_USART_ClearFlag_RXPARERR
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_ClearFlag_RXPARERR(USART_TypeDef *USARTx)
{
    WRITE_REG(USARTx->STA, USART_STA_RX_ERROR_PARITY);
}

/**
 * @brief  Check if the USART CTS rising edge detected Flag is set or not
 * @rmtoll STA           CTS_RISE_FLAG            LL_USART_IsActiveFlag_CTSTAISE
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_CTSTAISE(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_CTS_RISE_FLAG) == (USART_STA_CTS_RISE_FLAG));
}

/**
 * @brief  Clear CTS rising edge detected Flag
 * @rmtoll STA           CTS_RISE_FLAG            LL_USART_ClearFlag_CTSTAISE
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_ClearFlag_CTSTAISE(USART_TypeDef *USARTx)
{
    WRITE_REG(USARTx->STA, USART_STA_CTS_RISE_FLAG);
}

/**
 * @brief  Check if the USART CTS falling edge detected Flag is set or not
 * @rmtoll STA           CTS_FALL_FLAG            LL_USART_IsActiveFlag_CTSFALL
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_CTSFALL(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_CTS_FALL_FLAG) == (USART_STA_CTS_FALL_FLAG));
}

/**
 * @brief  Clear CTS falling edge detected Flag
 * @rmtoll STA           CTS_FALL_FLAG            LL_USART_ClearFlag_CTSFALL
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_ClearFlag_CTSFALL(USART_TypeDef *USARTx)
{
    WRITE_REG(USARTx->STA, USART_STA_CTS_FALL_FLAG);
}

/**
 * @brief  Check if the USART RX Frame Error Flag is set or not
 * @rmtoll STA           RX_FRAME_ERR_FLAG            LL_USART_IsActiveFlag_RXFRAMEERR
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_RXFRAMEERR(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_RX_FRAME_ERR_FLAG) == (USART_STA_RX_FRAME_ERR_FLAG));
}

/**
 * @brief  Clear RX Frame Error Flag
 * @rmtoll STA           RX_FRAME_ERR_FLAG            LL_USART_ClearFlag_RXFRAMEERR
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_ClearFlag_RXFRAMEERR(USART_TypeDef *USARTx)
{
    WRITE_REG(USARTx->STA, USART_STA_RX_FRAME_ERR_FLAG);
}

/**
 * @brief  Check if the USART Stop bit length mismatch error Flag is set or not
 * @rmtoll STA           STOP_BIT_ERR_FLAG            LL_USART_IsActiveFlag_STOPBITERR
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_STOPBITERR(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_STOP_BIT_ERR_FLAG) == (USART_STA_STOP_BIT_ERR_FLAG));
}

/**
 * @brief  Clear Stop bit length mismatch error Flag
 * @rmtoll STA           STOP_BIT_ERR_FLAG            LL_USART_ClearFlag_STOPBITERR
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_ClearFlag_STOPBITERR(USART_TypeDef *USARTx)
{
    WRITE_REG(USARTx->STA, USART_STA_STOP_BIT_ERR_FLAG);
}

/**
 * @brief  Check if the USART RX IDLE timeout Flag is set or not
 * @rmtoll STA           RX_IDLE_TOUT_STA            LL_USART_IsActiveFlag_RXIDLETO
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_RXIDLETO(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_RX_IDLE_TOUT_STA) == (USART_STA_RX_IDLE_TOUT_STA));
}

/**
 * @brief  Clear RX IDLE timeout Flag
 * @rmtoll STA           RX_IDLE_TOUT_STA            LL_USART_ClearFlag_RXIDLETO
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_ClearFlag_RXIDLETO(USART_TypeDef *USARTx)
{
    WRITE_REG(USARTx->STA, USART_STA_RX_IDLE_TOUT_STA);
}

/**
 * @brief  Check if the USART RX Watermark timeout Flag is set or not
 * @rmtoll STA           RX_WM_TOUT_STA            LL_USART_IsActiveFlag_RXWMTO
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_RXWMTO(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_RX_WM_TOUT_STA) == (USART_STA_RX_WM_TOUT_STA));
}

/**
 * @brief  Clear RX Watermark timeout Flag
 * @rmtoll STA           RX_WM_TOUT_STA            LL_USART_ClearFlag_RXWMTO
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_ClearFlag_RXWMTO(USART_TypeDef *USARTx)
{
    WRITE_REG(USARTx->STA, USART_STA_RX_WM_TOUT_STA);
}

/**
 * @brief  Check if the USART TX FIFO Full Flag is set or not
 * @note   Read only.
 * @rmtoll STA           FULL            LL_USART_IsActiveFlag_FIFOFULL
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_FIFOFULL(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_FULL) == (USART_STA_FULL));
}

/**
 * @brief  Check if the USART RX FIFO Empty Flag is set or not
 * @note   Read only.
 * @rmtoll STA           EMPTY            LL_USART_IsActiveFlag_FIFOEMPTY
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_FIFOEMPTY(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_EMPTY) == (USART_STA_EMPTY));
}

/**
 * @brief  Check if the USART TX Transfer Complete Flag is set or not
 * @rmtoll STA           TX_EOT            LL_USART_IsActiveFlag_TXTC
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_TXTC(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_TX_EOT) == (USART_STA_TX_EOT));
}

/**
 * @brief  Clear TX Transfer Complete Flag
 * @rmtoll STA           TX_EOT            LL_USART_ClearFlag_TXTC
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_ClearFlag_TXTC(USART_TypeDef *USARTx)
{
    WRITE_REG(USARTx->STA, USART_STA_TX_EOT);
}

/**
 * @brief  Check if the USART RX Transfer Complete Flag is set or not
 * @rmtoll STA           RX_EOT            LL_USART_IsActiveFlag_RXTC
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_RXTC(const USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->STA, USART_STA_RX_EOT) == (USART_STA_RX_EOT));
}

/**
 * @brief  Clear RX Transfer Complete Flag
 * @rmtoll STA           RX_EOT            LL_USART_ClearFlag_RXTC
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_ClearFlag_RXTC(USART_TypeDef *USARTx)
{
    WRITE_REG(USARTx->STA, USART_STA_RX_EOT);
}

/**
 * @brief  Check if the Rising edge detected Flag is set or not
 * @rmtoll RX_LOW_LEVEL_CNT           RX_HIGH_LEVEL_DETECT            LL_USART_IsActiveFlag_RxRisingEdge
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsActiveFlag_RxRisingEdge(USART_TypeDef *USARTx)
{
    return (READ_BIT(USARTx->RX_LOW_LEVEL_CNT, USART_RX_LOW_LEVEL_CNT_RX_HIGH_LEVEL_DETECT) == (USART_RX_LOW_LEVEL_CNT_RX_HIGH_LEVEL_DETECT));
}

/**
 * @brief  Clear Rising edge detected Flag (Write 0 to Clear)
 * @rmtoll RX_LOW_LEVEL_CNT           RX_HIGH_LEVEL_DETECT            LL_USART_ClearFlag_RxRisingEdge
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_ClearFlag_RxRisingEdge(USART_TypeDef *USARTx)
{
    CLEAR_BIT(USARTx->RX_LOW_LEVEL_CNT, USART_RX_LOW_LEVEL_CNT_RX_HIGH_LEVEL_DETECT);
}

/**
 * @}
 */

/** @defgroup USART_LL_EF_DMA_Management DMA_Management
 * @{
 */

/**
 * @brief  Enable DMA Mode for reception
 * @rmtoll SETUP          SEPARATE_EN          LL_USART_EnableDMAReq_RX\n
 *         SETUP          RX_DMA_EN            LL_USART_EnableDMAReq_RX
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableDMAReq_RX(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->SETUP, USART_SETUP_SEPARATE_EN | USART_SETUP_RX_DMA_EN);
}

/**
 * @brief  Disable DMA Mode for reception
 * @rmtoll SETUP          SEPARATE_EN          LL_USART_DisableDMAReq_RX\n
 *         SETUP          RX_DMA_EN            LL_USART_DisableDMAReq_RX
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableDMAReq_RX(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->SETUP, USART_SETUP_SEPARATE_EN);
    CLEAR_BIT(USARTx->SETUP, USART_SETUP_RX_DMA_EN);
}

/**
 * @brief  Check if DMA Mode is enabled for reception
 * @rmtoll SETUP          SEPARATE_EN          LL_USART_IsEnabledDMAReq_RX\n
 *         SETUP          RX_DMA_EN            LL_USART_IsEnabledDMAReq_RX
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledDMAReq_RX(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->SETUP, USART_SETUP_SEPARATE_EN);
    return (READ_BIT(USARTx->SETUP, USART_SETUP_RX_DMA_EN) == (USART_SETUP_RX_DMA_EN));
}

/**
 * @brief  Enable DMA Mode for transmission
 * @rmtoll SETUP          SEPARATE_EN          LL_USART_EnableDMAReq_TX\n
 *         SETUP          DMA_EN               LL_USART_EnableDMAReq_TX
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_EnableDMAReq_TX(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->SETUP, USART_SETUP_SEPARATE_EN | USART_SETUP_DMA_EN);
}

/**
 * @brief  Disable DMA Mode for transmission
 * @rmtoll SETUP          SEPARATE_EN          LL_USART_DisableDMAReq_TX\n
 *         SETUP          DMA_EN               LL_USART_DisableDMAReq_TX
 * @param  USARTx USART Instance
 * @retval None
 */
__STATIC_INLINE void LL_USART_DisableDMAReq_TX(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->SETUP, USART_SETUP_SEPARATE_EN);
    CLEAR_BIT(USARTx->SETUP, USART_SETUP_DMA_EN);
}

/**
 * @brief  Check if DMA Mode is enabled for transmission
 * @rmtoll SETUP          SEPARATE_EN          LL_USART_IsEnabledDMAReq_TX\n
 *         SETUP          DMA_EN               LL_USART_IsEnabledDMAReq_TX
 * @param  USARTx USART Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_USART_IsEnabledDMAReq_TX(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->SETUP, USART_SETUP_SEPARATE_EN);
    return (READ_BIT(USARTx->SETUP, USART_SETUP_DMA_EN) == (USART_SETUP_DMA_EN));
}

/**
 * @brief  Set the DMA Mode for transmission or reception
 * @rmtoll SETUP           SEPARATE_EN            LL_USART_SetDMAReqDirection\n
 *         SETUP           DMA_EN                 LL_USART_SetDMAReqDirection\n
 *         SETUP           RX_DMA_EN              LL_USART_SetDMAReqDirection
 * @param  USARTx USART Instance
 * @param  DMADirection This parameter can be one of the following values:
 *         @arg @ref LL_USART_DMA_DIRECTION_NONE
 *         @arg @ref LL_USART_DMA_DIRECTION_RX
 *         @arg @ref LL_USART_DMA_DIRECTION_TX
 *         @arg @ref LL_USART_DMA_DIRECTION_TX_RX
 * @retval None
 */
__STATIC_INLINE void LL_USART_SetDMAReqDirection(USART_TypeDef *USARTx, uint32_t DMADirection)
{
    MODIFY_REG(USARTx->SETUP, USART_SETUP_SEPARATE_EN | USART_SETUP_DMA_EN | USART_SETUP_RX_DMA_EN, DMADirection);
}

/**
 * @brief  Get the DMA Mode for transmission or reception
 * @rmtoll SETUP           SEPARATE_EN            LL_USART_GetDMAReqDirection\n
 *         SETUP           DMA_EN                 LL_USART_GetDMAReqDirection\n
 *         SETUP           RX_DMA_EN              LL_USART_GetDMAReqDirection
 * @note   Address of Data Register is valid for both Transmit and Receive transfers.
 * @param  USARTx USART Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_USART_DMA_DIRECTION_NONE
 *         @arg @ref LL_USART_DMA_DIRECTION_RX
 *         @arg @ref LL_USART_DMA_DIRECTION_TX
 *         @arg @ref LL_USART_DMA_DIRECTION_TX_RX
 */
__STATIC_INLINE uint32_t LL_USART_GetDMAReqDirection(USART_TypeDef *USARTx)
{
    return (uint32_t)(READ_BIT(USARTx->SETUP, USART_SETUP_SEPARATE_EN | USART_SETUP_DMA_EN | USART_SETUP_RX_DMA_EN));
}

/**
 * @}
 */

/** @defgroup USART_LL_EF_Data_Management Data_Management
 * @{
 */

/**
 * @brief  Read Receiver Data register (Receive Data value, 9 bits)
 * @rmtoll RXDATA           RXDATA            LL_USART_ReceiveData
 * @param  USARTx USART Instance.
 * @retval Value between Min_Data=0x00 and Max_Data=0x1FF
 */
__STATIC_INLINE uint16_t LL_USART_ReceiveData(const USART_TypeDef *USARTx)
{
    return (uint8_t)(READ_BIT(USARTx->RXDATA, USART_RXDATA_RXDATA));
}

/**
 * @brief  Write in Transmitter Data Register (Transmit Data value, 9 bits)
 * @rmtoll TXDATA           TXDATA            LL_USART_TransmitData
 * @param  USARTx USART Instance.
 * @param  Value Value between Min_Data=0x00 and Max_Data=0x1FF
 * @retval None
 */
__STATIC_INLINE void LL_USART_TransmitData(USART_TypeDef *USARTx, uint16_t Value)
{
    MODIFY_REG(USARTx->TXDATA, USART_TXDATA_TXDATA, Value);
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup USART_LL_EF_Init Initialization and de-initialization functions
 * @{
 */

ErrorStatus LL_USART_Init(USART_TypeDef *USARTx, const LL_USART_InitTypeDef *USART_InitStruct);
ErrorStatus LL_USART_DeInit(USART_TypeDef *USARTx);
void        LL_USART_StructInit(LL_USART_InitTypeDef *USART_InitStruct); // If the module is not suitable for the default configuration, you could not defines this function.
ErrorStatus LL_USART_ClockInit(USART_TypeDef *USARTx, const LL_USART_ClockInitTypeDef *USART_ClockInitStruct);
void        LL_USART_ClockStructInit(LL_USART_ClockInitTypeDef *USART_ClockInitStruct);

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

#endif /* USART0 || USART1 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_USART_H */
