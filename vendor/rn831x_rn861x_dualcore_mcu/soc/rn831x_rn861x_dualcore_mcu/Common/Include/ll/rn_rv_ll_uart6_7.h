/**
 *****************************************************************************
 * @file   rn_rv_ll_uart6_7.h
 * @author Renergy Software Team
 * @brief  Header file of UART6_7 LL module.
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
#ifndef __RN_LL_UART6_7_H
#define __RN_LL_UART6_7_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_uart6_7.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (UART6) || defined (UART7)

/** @defgroup UART6_7_LL UART6_7
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup UART6_7_LL_Private_Constants UART6_7 Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup UART6_7_LL_Private_Macros UART6_7 Private Macros
 * @{
 */


/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup UART6_7_LL_ES_INIT UART6_7 Exported Init structure
 * @{
 */

/**
 * @brief LL UART6_7 Init Structure definition
 */
typedef struct
{
    uint32_t BaudRate;                  /*!< This field defines expected Uart communication baud rate.

                                           This feature can be modified afterwards using unitary function @ref LL_UART6_7_SetBaudRate().*/

    uint32_t DataWidth;                 /*!< Specifies the number of data bits transmitted or received in a frame.
                                             This parameter can be a value of @ref UART6_7_LL_EC_DATAWIDTH.

                                             This feature can be modified afterwards using unitary function @ref LL_UART6_7_SetDataWidth().*/

    uint32_t StopBits;                  /*!< Specifies the number of stop bits transmitted.
                                             This parameter can be a value of @ref UART6_7_LL_EC_STOPBITS.

                                             This feature can be modified afterwards using unitary function @ref LL_UART6_7_SetStopBitsLength().*/

    uint32_t Parity;                    /*!< Specifies the parity mode.
                                             This parameter can be a value of @ref UART6_7_LL_EC_PARITY.

                                             This feature can be modified afterwards using unitary function @ref LL_UART6_7_SetParity().*/

    uint32_t TransferBitOrder;          /*!< Specifies the data transmitted/received with the LSB first or MSB first.
                                             This parameter can be a value of @ref UART6_7_LL_EC_BIT_ORDER.

                                             This feature can be modified afterwards using unitary function @ref LL_UART6_7_SetTransferBitOrder().*/

} LL_UART6_7_InitTypeDef;

/**
 * @}
 */
#endif /* USE_FULL_LL_DRIVER */

/* Exported constants -------------------------------------------------------*/
/** @defgroup UART6_7_LL_Exported_Constants UART6_7 Exported Constants
 * @{
 */

/** @defgroup UART6_7_LL_EC_PARITY Parity Control
 * @{
 */
#define LL_UART6_7_PARITY_NONE                    UART6_7_CTRL_PARS_NONE                  /*!< No parity           */
#define LL_UART6_7_PARITY_ODD                     UART6_7_CTRL_PARS_ODD                   /*!< Odd parity          */
#define LL_UART6_7_PARITY_EVEN                    UART6_7_CTRL_PARS_EVEN                  /*!< Even parity         */
#define LL_UART6_7_PARITY_ZERO                    UART6_7_CTRL_PARS_ZERO                  /*!< Fixed to 0 parity   */
#define LL_UART6_7_PARITY_ONE                     UART6_7_CTRL_PARS_ONE                   /*!< Fixed to 1 parity   */
#define LL_UART6_7_PARITY_USER_DEFINED            UART6_7_CTRL_PARS_USER_DEFINED          /*!< User-defined parity */
/**
 * @}
 */

/** @defgroup UART6_7_LL_EC_DATAWIDTH Datawidth
 * @{
 */
#define LL_UART6_7_DATAWIDTH_5B                   UART6_7_CTRL_DATLEN_5B                  /*!< 5 bits data length */
#define LL_UART6_7_DATAWIDTH_6B                   UART6_7_CTRL_DATLEN_6B                  /*!< 6 bits data length */
#define LL_UART6_7_DATAWIDTH_7B                   UART6_7_CTRL_DATLEN_7B                  /*!< 7 bits data length */
#define LL_UART6_7_DATAWIDTH_8B                   UART6_7_CTRL_DATLEN_8B                  /*!< 8 bits data length */
/**
 * @}
 */

/** @defgroup UART6_7_LL_EC_BIT_ORDER Transmission Bit Order
 * @{
 */
#define LL_UART6_7_LSB_FIRST                    UART6_7_CTRL_LMSB_LSB_FIRST              /*!< Data is transmitted/received with the LSB first */
#define LL_UART6_7_MSB_FIRST                    UART6_7_CTRL_LMSB_MSB_FIRST              /*!< Data is transmitted/received with the MSB first */
/**
 * @}
 */

/** @defgroup UART6_7_LL_EC_DATA_POLARITY Data Polarity
 * @{
 */
#define LL_UART6_7_DATA_POLARITY_HIGH                 UART6_7_CTRL_NEG_POSITIVE              /*!< Positive polarity (Idle level = High) */
#define LL_UART6_7_DATA_POLARITY_LOW                  UART6_7_CTRL_NEG_NEGATIVE              /*!< Negative polarity (Idle level = Low)  */
/**
 * @}
 */

/** @defgroup UART6_7_LL_EC_IRDA_POLARITY IRDA Polarity
 * @{
 */
#define LL_UART6_7_IRDA_POLARITY_HIGH                 UART6_7_CTRL_IRSEL_POSITIVE            /*!< Positive polarity (Idle level = High, Modulation = Low) */
#define LL_UART6_7_IRDA_POLARITY_LOW                  UART6_7_CTRL_IRSEL_NEGATIVE            /*!< Negative polarity (Idle level = Low, Modulation = High) */
/**
 * @}
 */

/** @defgroup UART6_7_LL_EC_STOPBITS Stop Bits
 * @{
 */
#define LL_UART6_7_STOPBITS_1                         UART6_7_CTRL_STOPS_1                       /*!< 1 stop bit */
#define LL_UART6_7_STOPBITS_2                         UART6_7_CTRL_STOPS_2                       /*!< 2 stop bit */
/**
 * @}
 */

/** @defgroup UART6_7_LL_EC_PARMEM Parity Error Store Memory
 * @{
 */
#define LL_UART6_7_PARMEM_STORE_MEMORY                UART6_7_CTRL_PAR_MEM_STORE_MEMORY                   /*!< Data is stored in memory regardless of parity error (for uart6_7).                 */
#define LL_UART6_7_PARMEM_NOT_STORE_MEMORY            UART6_7_CTRL_PAR_MEM_NOT_STORE_MEMORY               /*!< When parity error occurs, the current frame is not stored in memory (for uart6_7). */
/**
 * @}
 */

/** @defgroup UART6_7_LL_EC_DMA_PARITY DMA_PARITY
 * @{
 */
#define LL_UART6_7_DMA_PARITY_ZERO                     UART6_7_DMA_CTRL_DMA_PASS_ZERO                      /*!< DMA Transmit Fixed to 0 parity */
#define LL_UART6_7_DMA_PARITY_ONE                      UART6_7_DMA_CTRL_DMA_PASS_ONE                       /*!< DMA Transmit Fixed to 1 parity */
/**
 * @}
 */

/** @defgroup UART6_7_LL_EC_DELAY_INSERT Delay_Insert
 * @{
 */
#define LL_UART6_7_DRLAY_TIMEOUT_CNT_1B                UART6_7_TX_DRLAY_TIMEOUT_CNT_1B                     /*!< 1 UART6_7 data bit time  */
#define LL_UART6_7_DRLAY_TIMEOUT_CNT_2B                UART6_7_TX_DRLAY_TIMEOUT_CNT_2B                     /*!< 2 UART6_7 data bit times */
#define LL_UART6_7_DRLAY_TIMEOUT_CNT_3B                UART6_7_TX_DRLAY_TIMEOUT_CNT_3B                     /*!< 3 UART6_7 data bit times */
#define LL_UART6_7_DRLAY_TIMEOUT_CNT_4B                UART6_7_TX_DRLAY_TIMEOUT_CNT_4B                     /*!< 4 UART6_7 data bit times */
/**
 * @}
 */

/** @defgroup UART6_7_LL_EC_CF_TIMEOUT UART6_7_CF_TIMEOUT
 * @{
 */
#define LL_UART6_7_CF_TIMEOUT_NONE                 UART6_7_CF_OV_CF_OV_TIME_TIMEOUT_NONE                     /*!< Timeout mechanism disabled */
#define LL_UART6_7_CF_TIMEOUT_22B                  UART6_7_CF_OV_CF_OV_TIME_TIMEOUT_22B                      /*!< Timeout after 22 bits      */
#define LL_UART6_7_CF_TIMEOUT_44B                  UART6_7_CF_OV_CF_OV_TIME_TIMEOUT_44B                      /*!< Timeout after 44 bits      */
#define LL_UART6_7_CF_TIMEOUT_88B                  UART6_7_CF_OV_CF_OV_TIME_TIMEOUT_88B                      /*!< Timeout after 88 bits      */
/**
 * @}
 */

/** @defgroup UART6_7_LL_EC_CAL_MODE UART6_7_CAL_MODE
 * @{
 */
#define LL_UART6_7_CAL_MODE_CONTINUOUS                 UART6_7_CAL_CTRL_CAL_MODE_CONTINUOUS                     /*!< Continuous measurement */
#define LL_UART6_7_CAL_MODE_FIXED_DATA                 UART6_7_CAL_CTRL_CAL_MODE_FIXED_DATA                     /*!< Fixed data measurement */
/**
 * @}
 */

/** @defgroup UART6_7_LL_EC_CAL_POSITION_START Cal_Position_Start
 * @{
 */
#define LL_UART6_7_CAL_POSITION_START_RXFALLINGEDGE                 UART6_7_CAL_CTRL_POS_START_RX_FALLING_EDGE                 /*!< RX falling edge */
#define LL_UART6_7_CAL_POSITION_START_RXRISINGEDGE                  UART6_7_CAL_CTRL_POS_START_RX_RISING_EDGE                   /*!< RX rising edge */
/**
 * @}
 */

/** @defgroup UART6_7_LL_EC_CAL_POSITION_END Cal_Position_End
 * @{
 */
#define LL_UART6_7_CAL_POSITION_END_RXFALLINGEDGE                  UART6_7_CAL_CTRL_POS_END_RX_FALLING_EDGE                    /*!< RX falling edge */
#define LL_UART6_7_CAL_POSITION_END_RXRISINGEDGE                   UART6_7_CAL_CTRL_POS_END_RX_RISING_EDGE                      /*!< RX rising edge */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup UART6_7_LL_Exported_Macros UART6_7 Exported Macros
 * @{
 */

/** @defgroup UART6_7_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in UART6_7 register
 * @param  __INSTANCE__ UART6_7 Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_UART6_7_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in UART6_7 register
 * @param  __INSTANCE__ UART6_7 Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_UART6_7_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/** @defgroup UART6_7_LL_EM_Exported_Macros_Helper Exported Macros Helper
 * @{
 */

/**
 * @brief  Compute UART6_7DIV value according to Peripheral Clock and expected Baud Rate.
 * @param  __PERIPHCLK__ Peripheral Clock frequency used for UART6_7 instance
 * @param  __BAUDRATE__ Baud rate value to achieve
 * @retval UART6_7DIV value to be used for BAUD register
 */
#define __LL_UART6_7_CALC_BAUDRATE_DIVMANT(__PERIPHCLK__, __BAUDRATE__)   (((__PERIPHCLK__) / ((__BAUDRATE__) << 4)) - 1)

#define __LL_UART6_7_CALC_BAUDRATE_DIVFRAC(__PERIPHCLK__, __BAUDRATE__)   (( ((uint64_t)( (__PERIPHCLK__) % ((__BAUDRATE__) << 4) ) << 14) + (((__BAUDRATE__) << 4) >> 1) ) \
                                                                            / ((__BAUDRATE__) << 4) )

/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup UART6_7_LL_Exported_Functions UART6_7 Exported Functions
 * @{
 */

/** @defgroup UART6_7_LL_EF_Configuration Configuration functions
 * @{
 */

/**
 * @brief  UART6_7 Enable
 * @rmtoll CTRL          EN            LL_UART6_7_Enable
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_Enable(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CTRL, UART6_7_CTRL_EN);
}

/**
 * @brief  UART6_7 Disable (all UART6_7 prescalers and outputs are disabled)
 * @rmtoll CTRL          EN            LL_UART6_7_Disable
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_Disable(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CTRL, UART6_7_CTRL_EN);
}

/**
 * @brief  Indicate if UART6_7 is enabled
 * @rmtoll CTRL          EN            LL_UART6_7_IsEnabled
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabled(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_EN) == (UART6_7_CTRL_EN));
}

/**
 * @brief  Set the length of the stop bits
 * @rmtoll CTRL          STOPS          LL_UART6_7_SetStopBitsLength
 * @param  UART6_7x UART6_7 Instance
 * @param  StopBits This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_STOPBITS_1
 *         @arg @ref LL_UART6_7_STOPBITS_2
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_SetStopBitsLength(UART6_7_TypeDef *UART6_7x, uint32_t StopBits)
{
    MODIFY_REG(UART6_7x->CTRL, UART6_7_CTRL_STOPS, StopBits);
}

/**
 * @brief  Retrieve the length of the stop bits
 * @rmtoll CTRL          STOPS          LL_UART6_7_GetStopBitsLength
 * @param  UART6_7x UART6_7 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART6_7_STOPBITS_1
 *         @arg @ref LL_UART6_7_STOPBITS_2
 */
__STATIC_INLINE uint32_t LL_UART6_7_GetStopBitsLength(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_STOPS));
}

/**
 * @brief  Set Word length (i.e. nb of data bits, excluding start and stop bits)
 * @rmtoll CTRL          DATLEN             LL_UART6_7_SetDataWidth
 * @param  UART6_7x UART6_7 Instance
 * @param  DataWidth This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_DATAWIDTH_5B
 *         @arg @ref LL_UART6_7_DATAWIDTH_6B
 *         @arg @ref LL_UART6_7_DATAWIDTH_7B
 *         @arg @ref LL_UART6_7_DATAWIDTH_8B
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_SetDataWidth(UART6_7_TypeDef *UART6_7x, uint32_t DataWidth)
{
    MODIFY_REG(UART6_7x->CTRL, UART6_7_CTRL_DATLEN, DataWidth);
}

/**
 * @brief  Return Word length (i.e. nb of data bits, excluding start and stop bits)
 * @rmtoll CTRL          DATLEN             LL_UART6_7_GetDataWidth
 * @param  UART6_7x UART6_7 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART6_7_DATAWIDTH_5B
 *         @arg @ref LL_UART6_7_DATAWIDTH_6B
 *         @arg @ref LL_UART6_7_DATAWIDTH_7B
 *         @arg @ref LL_UART6_7_DATAWIDTH_8B
 */
__STATIC_INLINE uint32_t LL_UART6_7_GetDataWidth(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_DATLEN));
}

/**
 * @brief  Configure Parity.
 * @rmtoll CTRL          PARS            LL_UART6_7_SetParity
 * @param  UART6_7x UART6_7 Instance
 * @param  Parity This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_PARITY_NONE
 *         @arg @ref LL_UART6_7_PARITY_ODD
 *         @arg @ref LL_UART6_7_PARITY_EVEN
 *         @arg @ref LL_UART6_7_PARITY_ZERO
 *         @arg @ref LL_UART6_7_PARITY_ONE
 *         @arg @ref LL_UART6_7_PARITY_USER_DEFINED
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_SetParity(UART6_7_TypeDef *UART6_7x, uint32_t Parity)
{
    MODIFY_REG(UART6_7x->CTRL, UART6_7_CTRL_PARS, Parity);
}

/**
 * @brief  Return Parity configuration
 * @rmtoll CTRL          PARS            LL_UART6_7_GetParity
 * @param  UART6_7x UART6_7 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART6_7_PARITY_NONE
 *         @arg @ref LL_UART6_7_PARITY_ODD
 *         @arg @ref LL_UART6_7_PARITY_EVEN
 *         @arg @ref LL_UART6_7_PARITY_ZERO
 *         @arg @ref LL_UART6_7_PARITY_ONE
 *         @arg @ref LL_UART6_7_PARITY_USER_DEFINED
 */
__STATIC_INLINE uint32_t LL_UART6_7_GetParity(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_PARS));
}

/**
 * @brief  Configure Character frame format (Datawidth, Parity control, Stop Bits)
 * @note   Call of this function is equivalent to following function call sequence :
 *         - Data Width configuration using @ref LL_UART6_7_SetDataWidth() function
 *         - Parity Control and mode configuration using @ref LL_UART6_7_SetParity() function
 *         - Stop bits configuration using @ref LL_UART6_7_SetStopBitsLength() function
 * @rmtoll CTRL          DATLEN            LL_UART6_7_ConfigCharacter\n
 *         CTRL          PARS              LL_UART6_7_ConfigCharacter\n
 *         CTRL          STOPS             LL_UART6_7_ConfigCharacter
 * @param  UART6_7x UART6_7 Instance
 * @param  DataWidth This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_DATAWIDTH_5B
 *         @arg @ref LL_UART6_7_DATAWIDTH_6B
 *         @arg @ref LL_UART6_7_DATAWIDTH_7B
 *         @arg @ref LL_UART6_7_DATAWIDTH_8B
 * @param  Parity This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_PARITY_NONE
 *         @arg @ref LL_UART6_7_PARITY_ODD
 *         @arg @ref LL_UART6_7_PARITY_EVEN
 *         @arg @ref LL_UART6_7_PARITY_ZERO
 *         @arg @ref LL_UART6_7_PARITY_ONE
 *         @arg @ref LL_UART6_7_PARITY_USER_DEFINED
 * @param  StopBits This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_STOPBITS_1
 *         @arg @ref LL_UART6_7_STOPBITS_2
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ConfigCharacter(UART6_7_TypeDef *UART6_7x, uint32_t DataWidth, uint32_t Parity,
                                              uint32_t StopBits)
{
    MODIFY_REG(UART6_7x->CTRL, UART6_7_CTRL_DATLEN | UART6_7_CTRL_PARS | UART6_7_CTRL_STOPS, Parity | DataWidth | StopBits);
}

/**
 * @brief  Internal Loopback Enable
 * @rmtoll CTRL          ILBE            LL_UART6_7_EnableInternalLoop
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableInternalLoop(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CTRL, UART6_7_CTRL_ILBE);
}

/**
 * @brief  Internal Loopback Disable
 * @rmtoll CTRL          ILBE            LL_UART6_7_DisableInternalLoop
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableInternalLoop(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CTRL, UART6_7_CTRL_ILBE);
}

/**
 * @brief  Indicate if Internal Loopback is enabled
 * @rmtoll CTRL          ILBE            LL_UART6_7_IsEnabledInternalLoop
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledInternalLoop(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_ILBE) == (UART6_7_CTRL_ILBE));
}

/**
 * @brief  Set transfer bit order
 * @note   This bit should not be changed when communication is ongoing.
 * @rmtoll CTRL          LMSB      LL_UART6_7_SetTransferBitOrder
 * @param  UART6_7x UART6_7 Instance
 * @param  BitOrder This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_LSB_FIRST
 *         @arg @ref LL_UART6_7_MSB_FIRST
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_SetTransferBitOrder(UART6_7_TypeDef *UART6_7x, uint32_t BitOrder)
{
    MODIFY_REG(UART6_7x->CTRL, UART6_7_CTRL_LMSB, BitOrder);
}

/**
 * @brief  Get transfer bit order
 * @rmtoll CTRL          LMSB      LL_UART6_7_GetTransferBitOrder
 * @param  UART6_7x UART6_7 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART6_7_LSB_FIRST
 *         @arg @ref LL_UART6_7_MSB_FIRST
 */
__STATIC_INLINE uint32_t LL_UART6_7_GetTransferBitOrder(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_LMSB));
}

/**
 * @brief  Select the polarity of the trasmit and receive data
 * @rmtoll CTRL          NEG          LL_UART6_7_SetDataPolarity
 * @param  UART6_7x UART6_7 Instance
 * @param  DataPolarity This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_DATA_POLARITY_HIGH
 *         @arg @ref LL_UART6_7_DATA_POLARITY_LOW
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_SetDataPolarity(UART6_7_TypeDef *UART6_7x, uint32_t DataPolarity)
{
    MODIFY_REG(UART6_7x->CTRL, UART6_7_CTRL_NEG, DataPolarity);
}

/**
 * @brief  Return the polarity of the trasmit and receive data
 * @rmtoll CTRL          NEG          LL_UART6_7_GetDataPolarity
 * @param  UART6_7x UART6_7 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART6_7_DATA_POLARITY_HIGH
 *         @arg @ref LL_UART6_7_DATA_POLARITY_LOW
 */
__STATIC_INLINE uint32_t LL_UART6_7_GetDataPolarity(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_NEG));
}

/**
 * @brief  Memory storage configuration on parity error (just use for uart6_7)
 * @note   When parity error occurs, the current frame store or not store in memory
 * @rmtoll CTRL          PAR_MEM          LL_UART6_7_SetParityErrorHandling
 * @param  UART6_7x UART6_7 Instance (just use uart6_7 Instance)
 * @param  ErrorHandling This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_PARMEM_STORE_MEMORY
 *         @arg @ref LL_UART6_7_PARMEM_NOT_STORE_MEMORY
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_SetParityErrorHandling(UART6_7_TypeDef *UART6_7x, uint32_t ErrorHandling)
{
    MODIFY_REG(UART6_7x->CTRL, UART6_7_CTRL_PAR_MEM, ErrorHandling);
}

/**
 * @brief  Return the Memory storage configuration on parity error (just use for uart6_7)
 * @note   When parity error occurs, the current frame store or not store in memory
 * @rmtoll CTRL          PAR_MEM          LL_UART6_7_GetParityErrorHandling
 * @param  UART6_7x UART6_7 Instance (just use uart6_7 Instance)
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART6_7_PARMEM_STORE_MEMORY
 *         @arg @ref LL_UART6_7_PARMEM_NOT_STORE_MEMORY
 */
__STATIC_INLINE uint32_t LL_UART6_7_GetParityErrorHandling(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_PAR_MEM));
}

/**
 * @brief  Configure UART6_7 BAUD and FDIV register for achieving expected Baud Rate value.
 * @note   Compute and set UART6_7DIV value in BAUD and FDIV register
 *         according to used Peripheral Clock, and expected Baud Rate values
 * @note   Peripheral clock and Baud rate values provided as function parameters should be valid
 * @rmtoll BAUD          CLKDIV           LL_UART6_7_SetBaudRate\n
 *         FDIV          FDIV             LL_UART6_7_SetBaudRate
 * @param  UART6_7x UART6_7 Instance
 * @param  PeriphClk Peripheral Clock
 * @param  BaudRate Baud Rate
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_SetBaudRate(UART6_7_TypeDef *UART6_7x, uint32_t PeriphClk, uint32_t BaudRate)
{
    UART6_7x->BAUD = (uint16_t)(__LL_UART6_7_CALC_BAUDRATE_DIVMANT(PeriphClk, BaudRate));
    UART6_7x->FDIV = (uint16_t)(__LL_UART6_7_CALC_BAUDRATE_DIVFRAC(PeriphClk, BaudRate));
}

/**
 * @brief  Return current Baud Rate value, according to UART6_7DIV present in BAUD and FDIV register,
 *         and to used Peripheral Clock values
 * @rmtoll BAUD          CLKDIV           LL_UART6_7_GetBaudRate\n
 *         FDIV          FDIV             LL_UART6_7_GetBaudRate
 * @param  UART6_7x UART6_7 Instance
 * @param  PeriphClk Peripheral Clock
 * @retval Baud Rate
 */
__STATIC_INLINE uint32_t LL_UART6_7_GetBaudRate(const UART6_7_TypeDef *UART6_7x, uint32_t PeriphClk)
{
    uint32_t usartdiv = 0x0U;
    uint32_t baudresult = 0x0U;

    usartdiv = ((UART6_7x->BAUD + 1U) << 14) + UART6_7x->FDIV;
    baudresult = (((uint64_t)PeriphClk << 14) + ((usartdiv << 3) >> 4)) / (usartdiv << 4);
    return (baudresult);
}


/**
 * @brief  Receive Timeout Configuration, if set 0 is invalid
 * @rmtoll RX_TO          TIMEOUT_CNT          LL_UART6_7_DMA_SetTimeoutCount
 * @param  UART6_7x UART6_7 Instance (just use uart6_7 Instance)
 * @param  TimeoutCount between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_SetTimeoutCount(UART6_7_TypeDef *UART6_7x, uint32_t TimeoutCount)
{
    MODIFY_REG(UART6_7x->RX_TO, UART6_7_RX_TO_TIMEOUT_CNT, TimeoutCount << UART6_7_RX_TO_TIMEOUT_CNT_POS);
}

/**
 * @brief  Return the Receive Timeout Configuration
 * @rmtoll RX_TO          TIMEOUT_CNT          LL_UART6_7_DMA_GetTimeoutCount
 * @param  UART6_7x UART6_7 Instance (just use uart6_7 Instance)
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF
 */
__STATIC_INLINE uint32_t LL_UART6_7_GetTimeoutCount(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->RX_TO, UART6_7_RX_TO_TIMEOUT_CNT) >> UART6_7_RX_TO_TIMEOUT_CNT_POS);
}

/**
 * @brief  Delay Insertion configuration (just use for uart6_7)
 * @note   The delay is inserted between data frames. When TX_DELAY_EN = 1 is valid.
 * @rmtoll TX_DRLAY          TIMEOUT_CNT          LL_UART6_7_SetTxDelayTime
 * @param  UART6_7x UART6_7 Instance (just use uart6_7 Instance)
 * @param  TxDelayTime This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_DRLAY_TIMEOUT_CNT_1B
 *         @arg @ref LL_UART6_7_DRLAY_TIMEOUT_CNT_2B
 *         @arg @ref LL_UART6_7_DRLAY_TIMEOUT_CNT_3B
 *         @arg @ref LL_UART6_7_DRLAY_TIMEOUT_CNT_4B
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_SetTxDelayTime(UART6_7_TypeDef *UART6_7x, uint32_t TxDelayTime)
{
    MODIFY_REG(UART6_7x->TX_DRLAY, UART6_7_TX_DRLAY_TIMEOUT_CNT, TxDelayTime);
}

/**
 * @brief  Return the Delay Insertion configuration (just use for uart6_7)
 * @note   The delay is inserted between data frames. When TX_DELAY_EN = 1 is valid.
 * @rmtoll TX_DRLAY          TIMEOUT_CNT          LL_UART6_7_GetTxDelayTime
 * @param  UART6_7x UART6_7 Instance (just use uart6_7 Instance)
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART6_7_DRLAY_TIMEOUT_CNT_1B
 *         @arg @ref LL_UART6_7_DRLAY_TIMEOUT_CNT_2B
 *         @arg @ref LL_UART6_7_DRLAY_TIMEOUT_CNT_3B
 *         @arg @ref LL_UART6_7_DRLAY_TIMEOUT_CNT_4B
 */
__STATIC_INLINE uint32_t LL_UART6_7_GetTxDelayTime(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->TX_DRLAY, UART6_7_TX_DRLAY_TIMEOUT_CNT));
}

/**
 * @brief  Enable Transmit Delay (just use for uart6_7)
 * @rmtoll TX_DRLAY          TX_DELAY_EN          LL_UART6_7_EnableTxDelay
 * @param  UART6_7x UART6_7 Instance (just use uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableTxDelay(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->TX_DRLAY, UART6_7_TX_DRLAY_TX_DELAY_EN);
}

/**
 * @brief  Disable Transmit Delay (just use for uart6_7)
 * @rmtoll TX_DRLAY          TX_DELAY_EN          LL_UART6_7_DisableTxDelay
 * @param  UART6_7x UART6_7 Instance (just use uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableTxDelay(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->TX_DRLAY, UART6_7_TX_DRLAY_TX_DELAY_EN);
}

/**
 * @brief  Indicate if Transmit Delay is enabled (just use for uart6_7)
 * @rmtoll TX_DRLAY          TX_DELAY_EN          LL_UART6_7_IsEnabledTxDelay
 * @param  UART6_7x UART6_7 Instance (just use uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledTxDelay(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->TX_DRLAY, UART6_7_TX_DRLAY_TX_DELAY_EN) == (UART6_7_TX_DRLAY_TX_DELAY_EN));
}

/**
 * @}
 */

/** @defgroup UART6_7_LL_EF_Metering_Configuration Metering Configuration functions only for uart6_7
 * @{
 */

/**
 * @brief  UART6_7_CF Mode Enable
 * @rmtoll CF_CTRL          UART6_7_CF_EN            LL_UART6_7_CF_Enable
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CF_Enable(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CF_CTRL, UART6_7_CF_CTRL_UART_CF_EN);
}

/**
 * @brief  UART6_7_CF Mode Disable
 * @rmtoll CF_CTRL          UART6_7_CF_EN            LL_UART6_7_CF_Disable
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CF_Disable(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CF_CTRL, UART6_7_CF_CTRL_UART_CF_EN);
}

/**
 * @brief  Indicate if UART6_7_CF Mode is enabled
 * @rmtoll CF_CTRL          UART6_7_CF_EN            LL_UART6_7_CF_IsEnabled
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_CF_IsEnabled(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CF_CTRL, UART6_7_CF_CTRL_UART_CF_EN) == (UART6_7_CF_CTRL_UART_CF_EN));
}

/**
 * @brief  Interrupt Data Write to SRAM Enable in UART6_7_CF mode (configurable when UART6_7_CF_EN=0)
 * @note   Write data to SRAM via DMA when metering interrupt data is received.
 * @rmtoll CF_CTRL          IRQ_WR_EN            LL_UART6_7_CF_EnableIRQDataWriteToSRAM
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CF_EnableIRQDataWriteToSRAM(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CF_CTRL, UART6_7_CF_CTRL_IRQ_WR_EN);
}

/**
 * @brief  Interrupt Data Write to SRAM Disable in UART6_7_CF mode (configurable when UART6_7_CF_EN=0)
 * @note   Write data to SRAM via DMA when metering interrupt data is received.
 * @rmtoll CF_CTRL          IRQ_WR_EN            LL_UART6_7_CF_DisableIRQDataWriteToSRAM
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CF_DisableIRQDataWriteToSRAM(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CF_CTRL, UART6_7_CF_CTRL_IRQ_WR_EN);
}

/**
 * @brief  Indicate if Interrupt Data Write to SRAM is enabled
 * @note   Write data to SRAM via DMA when metering interrupt data is received.
 * @rmtoll CF_CTRL          IRQ_WR_EN            LL_UART6_7_CF_IsEnabledIRQDataWriteToSRAM
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_CF_IsEnabledIRQDataWriteToSRAM(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CF_CTRL, UART6_7_CF_CTRL_IRQ_WR_EN) == (UART6_7_CF_CTRL_IRQ_WR_EN));
}

/**
 * @brief  Pulse Output Enable in UART6_7_CF mode
 * @note   Pulse Output Enable in UART6_7_CF mode (configurable when UART6_7_CF_EN=0).
 *         Output pulse to IOCNT when metering pulse data interrupt is set
 * @rmtoll CF_CTRL          PULSE_OE            LL_UART6_7_CF_EnablePulseOutput
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CF_EnablePulseOutput(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CF_CTRL, UART6_7_CF_CTRL_PULSE_OE);
}

/**
 * @brief  Pulse Output Disable
 * @note   Pulse Output Enable in UART6_7_CF mode (configurable when UART6_7_CF_EN=0).
 *         Output pulse to IOCNT when metering pulse data interrupt is set
 * @rmtoll CF_CTRL          PULSE_OE            LL_UART6_7_CF_DisablePulseOutput
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CF_DisablePulseOutput(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CF_CTRL, UART6_7_CF_CTRL_PULSE_OE);
}

/**
 * @brief  Indicate if Pulse Output is enabled
 * @note   Pulse Output Enable in UART6_7_CF mode (configurable when UART6_7_CF_EN=0).
 *         Output pulse to IOCNT when metering pulse data interrupt is set
 * @rmtoll CF_CTRL          PULSE_OE            LL_UART6_7_CF_IsEnabledPulseOutput
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_CF_IsEnabledPulseOutput(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CF_CTRL, UART6_7_CF_CTRL_PULSE_OE) == (UART6_7_CF_CTRL_PULSE_OE));
}

/**
 * @brief  Set the PF pulse sequence value for UART6_7 to recognize.
 * @note   Configurable when UART6_7_CF_EN=0. In UART6_7_CF mode,
 *         receiving this data (with each byte having a parity error) is recognized as a PF pulse
 * @rmtoll PFDAT          PFDAT          LL_UART6_7_CF_SetPFData
 * @param  UART6_7x UART6_7 Instance
 * @param  PFValue between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CF_SetPFData(UART6_7_TypeDef *UART6_7x, uint32_t PFValue)
{
    MODIFY_REG(UART6_7x->PFDAT, UART6_7_PFDAT_PFDAT, PFValue);
}

/**
 * @brief  Retrieve the PF pulse sequence value for UART6_7 to recognize.
 * @note   Configurable when UART6_7_CF_EN=0. In UART6_7_CF mode,
 *         receiving this data (with each byte having a parity error) is recognized as a PF pulse
 * @rmtoll PFDAT          PFDAT          LL_UART6_7_CF_GetPFData
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_UART6_7_CF_GetPFData(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->PFDAT, UART6_7_PFDAT_PFDAT));
}

/**
 * @brief  Set the QF pulse sequence value for UART6_7 to recognize.
 * @note   Configurable when UART6_7_CF_EN=0. In UART6_7_CF mode,
 *         receiving this data (with each byte having a parity error) is recognized as a QF pulse
 * @rmtoll QFDAT          QFDAT          LL_UART6_7_CF_SetQFData
 * @param  UART6_7x UART6_7 Instance
 * @param  QFValue between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CF_SetQFData(UART6_7_TypeDef *UART6_7x, uint32_t QFValue)
{
    MODIFY_REG(UART6_7x->QFDAT, UART6_7_QFDAT_QFDAT, QFValue);
}

/**
 * @brief  Retrieve the QF pulse sequence value for UART6_7 to recognize.
 * @note   Configurable when UART6_7_CF_EN=0. In UART6_7_CF mode,
 *         receiving this data (with each byte having a parity error) is recognized as a QF pulse
 * @rmtoll QFDAT          QFDAT          LL_UART6_7_CF_GetQFData
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_UART6_7_CF_GetQFData(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->QFDAT, UART6_7_QFDAT_QFDAT));
}

/**
 * @brief  Set the FPF pulse sequence value for UART6_7 to recognize.
 * @note   Configurable when UART6_7_CF_EN=0. In UART6_7_CF mode,
 *         receiving this data (with each byte having a parity error) is recognized as a FPF pulse
 * @rmtoll FPFDAT          FPFDAT          LL_UART6_7_CF_SetFPFData
 * @param  UART6_7x UART6_7 Instance
 * @param  FPFValue between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CF_SetFPFData(UART6_7_TypeDef *UART6_7x, uint32_t FPFValue)
{
    MODIFY_REG(UART6_7x->FPFDAT, UART6_7_FPFDAT_FPFDAT, FPFValue);
}

/**
 * @brief  Retrieve the FPF pulse sequence value for UART6_7 to recognize.
 * @note   Configurable when UART6_7_CF_EN=0. In UART6_7_CF mode,
 *         receiving this data (with each byte having a parity error) is recognized as a FPF pulse
 * @rmtoll FPFDAT          FPFDAT          LL_UART6_7_CF_GetFPFData
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_UART6_7_CF_GetFPFData(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->FPFDAT, UART6_7_FPFDAT_FPFDAT));
}

/**
 * @brief  Set the Interrupt data value for UART6_7 to recognize.
 * @note   Configurable when UART6_7_CF_EN=0. In UART6_7_CF mode,
 *         receiving this data (with each byte having a parity error) is recognized as an interrupt event (for single-phase chip)
 * @rmtoll IRQDAT          IRQDAT          LL_UART6_7_CF_SetIRQData
 * @param  UART6_7x UART6_7 Instance
 * @param  IRQValue between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CF_SetIRQData(UART6_7_TypeDef *UART6_7x, uint32_t IRQValue)
{
    MODIFY_REG(UART6_7x->IRQDAT, UART6_7_IRQDAT_IRQDAT, IRQValue);
}

/**
 * @brief  Retrieve the Interrupt data value for UART6_7 to recognize.
 * @note   Configurable when UART6_7_CF_EN=0. In UART6_7_CF mode,
 *         receiving this data (with each byte having a parity error) is recognized as an interrupt event (for single-phase chip)
 * @rmtoll IRQDAT          IRQDAT          LL_UART6_7_CF_GetIRQData
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_UART6_7_CF_GetIRQData(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->IRQDAT, UART6_7_IRQDAT_IRQDAT));
}

/**
 * @brief  Set the Interrupt data value for UART6_7 to recognize.
 * @note   Configurable when UART6_7_CF_EN=0. In UART6_7_CF mode,
 *         receiving this data (with each byte having a parity error) is recognized as an interrupt event (for single-phase chip)
 * @rmtoll CF_OV          CF_OV_TIME          LL_UART6_7_CF_SetTimeoutConfig
 * @param  UART6_7x UART6_7 Instance
 * @param  TimeoutConfig This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_CF_TIMEOUT_NONE
 *         @arg @ref LL_UART6_7_CF_TIMEOUT_22B
 *         @arg @ref LL_UART6_7_CF_TIMEOUT_44B
 *         @arg @ref LL_UART6_7_CF_TIMEOUT_88B
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CF_SetTimeoutConfig(UART6_7_TypeDef *UART6_7x, uint32_t TimeoutConfig)
{
    MODIFY_REG(UART6_7x->CF_OV, UART6_7_CF_OV_CF_OV_TIME, TimeoutConfig);
}

/**
 * @brief  Retrieve the Interrupt data value for UART6_7 to recognize.
 * @note   Configurable when UART6_7_CF_EN=0. In UART6_7_CF mode,
 *         receiving this data (with each byte having a parity error) is recognized as an interrupt event (for single-phase chip)
 * @rmtoll CF_OV          CF_OV_TIME          LL_UART6_7_CF_GetTimeoutConfig
 * @param  UART6_7x UART6_7 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART6_7_CF_TIMEOUT_NONE
 *         @arg @ref LL_UART6_7_CF_TIMEOUT_22B
 *         @arg @ref LL_UART6_7_CF_TIMEOUT_44B
 *         @arg @ref LL_UART6_7_CF_TIMEOUT_88B
 */
__STATIC_INLINE uint32_t LL_UART6_7_CF_GetTimeoutConfig(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->CF_OV, UART6_7_CF_OV_CF_OV_TIME));
}

/**
 * @brief  Timeout Buffer Clear Enable
 * @note   If a UART6_7_CF timeout occurs and this bit is 1 (enable),
 *         the partially received pulse data is written to SRAM.
 * @rmtoll CF_OV          OV_BUF_CLR            LL_UART6_7_CF_EnableTimeoutBufferClean
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CF_EnableTimeoutBufferClean(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CF_OV, UART6_7_CF_OV_OV_BUF_CLR);
}

/**
 * @brief  Timeout Buffer Clear Disable
 * @note   If a UART6_7_CF timeout occurs and this bit is 1 (enable),
 *         the partially received pulse data is written to SRAM.
 * @rmtoll CF_OV          OV_BUF_CLR            LL_UART6_7_CF_DisableTimeoutBufferClean
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CF_DisableTimeoutBufferClean(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CF_OV, UART6_7_CF_OV_OV_BUF_CLR);
}

/**
 * @brief  Indicate if Timeout Buffer Clear is enabled
 * @note   If a UART6_7_CF timeout occurs and this bit is 1 (enable),
 *         the partially received pulse data is written to SRAM.
 * @rmtoll CF_OV          OV_BUF_CLR            LL_UART6_7_CF_IsEnabledTimeoutBufferClean
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_CF_IsEnabledTimeoutBufferClean(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CF_OV, UART6_7_CF_OV_OV_BUF_CLR) == (UART6_7_CF_OV_OV_BUF_CLR));
}

/**
 * @brief  UART6_7_TW Mode Enable
 * @note   Switches UART6_7 TX to send low-level waveform
 * @rmtoll TW_CTRL          UART6_7_TW_MODE            LL_UART6_7_TW_Enable
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_TW_Enable(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->TW_CTRL, UART6_7_TW_CTRL_UART_TW_MODE);
}

/**
 * @brief  UART6_7_TW Mode Disable
 * @note   Switches UART6_7 TX to send low-level waveform
 * @rmtoll TW_CTRL          UART6_7_TW_MODE            LL_UART6_7_TW_Disable
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_TW_Disable(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->TW_CTRL, UART6_7_TW_CTRL_UART_TW_MODE);
}

/**
 * @brief  Indicate if UART6_7_TW Mode is enabled
 * @note   Switches UART6_7 TX to send low-level waveform
 * @rmtoll TW_CTRL          UART6_7_TW_MODE            LL_UART6_7_TW_IsEnabled
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_TW_IsEnabled(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->TW_CTRL, UART6_7_TW_CTRL_UART_TW_MODE) == (UART6_7_TW_CTRL_UART_TW_MODE));
}

/**
 * @brief  Set the Number of low-level pulses to send
 * @rmtoll TW_CTRL          TW_NUM          LL_UART6_7_TW_SetLowLevelCount
 * @param  UART6_7x UART6_7 Instance
 * @param  LowLevelCount between Min_Data=0x0 and Max_Data=0xF
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_TW_SetLowLevelCount(UART6_7_TypeDef *UART6_7x, uint32_t LowLevelCount)
{
    MODIFY_REG(UART6_7x->TW_CTRL, UART6_7_TW_CTRL_TW_NUM, LowLevelCount);
}

/**
 * @brief  Retrieve the Number of low-level pulses to send
 * @rmtoll TW_CTRL          TW_NUM          LL_UART6_7_TW_GetLowLevelCount
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x0 and Max_Data=0xF
 */
__STATIC_INLINE uint32_t LL_UART6_7_TW_GetLowLevelCount(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->TW_CTRL, UART6_7_TW_CTRL_TW_NUM));
}

/**
 * @brief  Set the TW mode baud rate configuration for each bit length
 * @note   Default value (0xBFF) is for 14M system clock and 4800 baud.
 *         Cannot be modified during transmission. (0x8 is configured as 8*time period)
 * @rmtoll TW_CTRL          TW_BAUD          LL_UART6_7_TW_SetBaudRate
 * @param  UART6_7x UART6_7 Instance
 * @param  BaudRate between Min_Data=0x0000 and Max_Data=0x3FFF
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_TW_SetBaudRate(UART6_7_TypeDef *UART6_7x, uint32_t BaudRate)
{
    MODIFY_REG(UART6_7x->TW_BAUD, UART6_7_TW_BAUD_TW_BAUD, BaudRate);
}

/**
 * @brief  Retrieve the TW mode baud rate configuration for each bit length
 * @note   Default value (0xBFF) is for 14M system clock and 4800 baud.
 *         Cannot be modified during transmission. (0x8 is configured as 8*time period)
 * @rmtoll TW_CTRL          TW_BAUD          LL_UART6_7_TW_GetBaudRate
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x0000 and Max_Data=0x3FFF
 */
__STATIC_INLINE uint32_t LL_UART6_7_TW_GetBaudRate(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->TW_BAUD, UART6_7_TW_BAUD_TW_BAUD));
}

/**
 * @brief  Set the TW Transmission Kick Start
 * @note   Write 1 to start transmission. Writing during transmission is ignored.
 *         Read as 1 during transmission. Cleared automatically when transmission completes.
 * @rmtoll TW_EN          TW_EN          LL_UART6_7_TW_GenerateStartCondition
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_TW_GenerateStartCondition(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->TW_EN, UART6_7_TW_EN_TW_EN);
}

/**
 * @brief  UART6_7_CAL Calibration Enable
 * @rmtoll CAL_CTRL          UART6_7_CAL_EN            LL_UART6_7_CAL_Enable
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CAL_Enable(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CAL_CTRL, UART6_7_CAL_CTRL_UART_CAL_EN);
}

/**
 * @brief  UART6_7_CAL Calibration Disable
 * @rmtoll CAL_CTRL          UART6_7_CAL_EN            LL_UART6_7_CAL_Disable
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CAL_Disable(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CAL_CTRL, UART6_7_CAL_CTRL_UART_CAL_EN);
}

/**
 * @brief  Indicate if UART6_7_CAL Calibration is enabled
 * @rmtoll CAL_CTRL          UART6_7_CAL_EN            LL_UART6_7_CAL_IsEnabled
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_CAL_IsEnabled(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CAL_CTRL, UART6_7_CAL_CTRL_UART_CAL_EN) == (UART6_7_CAL_CTRL_UART_CAL_EN));
}

/**
 * @brief  Set the TW mode baud rate configuration for each bit length
 * @rmtoll CAL_CTRL          CAL_MODE          LL_UART6_7_CAL_SetMode
 * @param  UART6_7x UART6_7 Instance
 * @param  CALMode This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_CAL_MODE_CONTINUOUS
 *         @arg @ref LL_UART6_7_CAL_MODE_FIXED_DATA
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CAL_SetMode(UART6_7_TypeDef *UART6_7x, uint32_t CALMode)
{
    MODIFY_REG(UART6_7x->CAL_CTRL, UART6_7_CAL_CTRL_CAL_MODE, CALMode);
}

/**
 * @brief  Retrieve the TW mode baud rate configuration for each bit length
 * @rmtoll CAL_CTRL          CAL_MODE          LL_UART6_7_CAL_GetMode
 * @param  UART6_7x UART6_7 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART6_7_CAL_MODE_CONTINUOUS
 *         @arg @ref LL_UART6_7_CAL_MODE_FIXED_DATA
 */
__STATIC_INLINE uint32_t LL_UART6_7_CAL_GetMode(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->CAL_CTRL, UART6_7_CAL_CTRL_CAL_MODE));
}

/**
 * @brief  Set the Measurement Start Point
 * @rmtoll CAL_CTRL          POS_START          LL_UART6_7_CAL_SetPositionStart
 * @param  UART6_7x UART6_7 Instance
 * @param  PositionStart This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_CAL_POSITION_START_RXFALLINGEDGE
 *         @arg @ref LL_UART6_7_CAL_POSITION_START_RXRISINGEDGE
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CAL_SetPositionStart(UART6_7_TypeDef *UART6_7x, uint32_t PositionStart)
{
    MODIFY_REG(UART6_7x->CAL_CTRL, UART6_7_CAL_CTRL_POS_START, PositionStart);
}

/**
 * @brief  Retrieve the Measurement Start Point
 * @rmtoll CAL_CTRL          POS_START          LL_UART6_7_CAL_GetPositionStart
 * @param  UART6_7x UART6_7 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART6_7_CAL_POSITION_START_RXFALLINGEDGE
 *         @arg @ref LL_UART6_7_CAL_POSITION_START_RXRISINGEDGE
 */
__STATIC_INLINE uint32_t LL_UART6_7_CAL_GetPositionStart(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->CAL_CTRL, UART6_7_CAL_CTRL_POS_START));
}

/**
 * @brief  Set the Measurement End Point
 * @rmtoll CAL_CTRL          POS_END          LL_UART6_7_CAL_SetPositionEnd
 * @param  UART6_7x UART6_7 Instance
 * @param  PositionEnd This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_CAL_POSITION_END_RXFALLINGEDGE
 *         @arg @ref LL_UART6_7_CAL_POSITION_END_RXRISINGEDGE
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CAL_SetPositionEnd(UART6_7_TypeDef *UART6_7x, uint32_t PositionEnd)
{
    MODIFY_REG(UART6_7x->CAL_CTRL, UART6_7_CAL_CTRL_POS_END, PositionEnd);
}

/**
 * @brief  Retrieve the Measurement End Point
 * @rmtoll CAL_CTRL          POS_END          LL_UART6_7_CAL_GetPositionEnd
 * @param  UART6_7x UART6_7 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART6_7_CAL_POSITION_END_RXFALLINGEDGE
 *         @arg @ref LL_UART6_7_CAL_POSITION_END_RXRISINGEDGE
 */
__STATIC_INLINE uint32_t LL_UART6_7_CAL_GetPositionEnd(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->CAL_CTRL, UART6_7_CAL_CTRL_POS_END));
}

/**
 * @brief  Set the Measurement End Point
 * @rmtoll CAL_CTRL          CAL_DATA          LL_UART6_7_CAL_SetFixedData
 * @param  UART6_7x UART6_7 Instance
 * @param  FixedData between Min_Data=0x00 and Max_Data=0x1F
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_CAL_SetFixedData(UART6_7_TypeDef *UART6_7x, uint32_t FixedData)
{
    MODIFY_REG(UART6_7x->CAL_CTRL, UART6_7_CAL_CTRL_CAL_DATA, FixedData);
}

/**
 * @brief  Retrieve the Measurement result (read only)
 * @note   Continuous mode: Updated after each low-level measurement.
 *         Fixed data mode: Updated after receiving and measuring the configured low-level length.
 * @rmtoll CAL_CNT          CAL_CNT          LL_UART6_7_CAL_GetMeasurementResult
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_UART6_7_CAL_GetMeasurementResult(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->CAL_CTRL, UART6_7_CAL_CNT_CAL_CNT));
}

/**
 * @}
 */

/** @defgroup UART6_7_LL_EF_Configuration_IRDA Configuration functions related to Irda feature
 * @{
 */

/**
 * @brief  Enable IrDA mode
 * @rmtoll CTRL          IRE          LL_UART6_7_EnableIrda
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableIrda(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CTRL, UART6_7_CTRL_IRE);
}

/**
 * @brief  Disable IrDA mode
 * @rmtoll CTRL          IRE          LL_UART6_7_DisableIrda
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableIrda(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CTRL, UART6_7_CTRL_IRE);
}

/**
 * @brief  Indicate if IrDA mode is enabled
 * @rmtoll CTRL          IRE          LL_UART6_7_IsEnabledIrda
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledIrda(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_IRE) == (UART6_7_CTRL_IRE));
}

/**
 * @brief  Configure IrDA polarity (Positive or Negative)
 * @rmtoll CTRL          IRSEL          LL_UART6_7_SetIrdaPolarity
 * @param  UART6_7x UART6_7 Instance
 * @param  IrdaPolarity This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_IRDA_POLARITY_HIGH
 *         @arg @ref LL_UART6_7_IRDA_POLARITY_LOW
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_SetIrdaPolarity(UART6_7_TypeDef *UART6_7x, uint32_t IrdaPolarity)
{
    MODIFY_REG(UART6_7x->CTRL, UART6_7_CTRL_IRSEL, IrdaPolarity);
}

/**
 * @brief  Retrieve IrDA polarity (Positive or Negative)
 * @rmtoll CTRL          IRSEL          LL_UART6_7_GetIrdaPolarity
 * @param  UART6_7x UART6_7 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART6_7_IRDA_POLARITY_HIGH
 *         @arg @ref LL_UART6_7_IRDA_POLARITY_LOW
 */
__STATIC_INLINE uint32_t LL_UART6_7_GetIrdaPolarity(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_IRSEL));
}

/**
 * @}
 */

/** @defgroup UART6_7_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable Transmit Data Interrupt
 * @rmtoll CTRL          TXIE        LL_UART6_7_EnableIT_TX
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableIT_TX(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CTRL, UART6_7_CTRL_TXIE);
}

/**
 * @brief  Disable Transmit Data Interrupt
 * @rmtoll CTRL          TXIE        LL_UART6_7_DisableIT_TX
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableIT_TX(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CTRL, UART6_7_CTRL_TXIE);
}

/**
 * @brief  Check if the UART6_7 Transmit Data Interrupt is enabled or disabled.
 * @rmtoll CTRL          TXIE        LL_UART6_7_IsEnabledIT_TX
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledIT_TX(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_TXIE) == (UART6_7_CTRL_TXIE));
}

/**
 * @brief  Enable  Receive Data Interrupt
 * @rmtoll CTRL          RXIE        LL_UART6_7_EnableIT_RX
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableIT_RX(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CTRL, UART6_7_CTRL_RXIE);
}

/**
 * @brief  Disable  Receive Data Interrupt
 * @rmtoll CTRL          RXIE        LL_UART6_7_DisableIT_RX
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableIT_RX(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CTRL, UART6_7_CTRL_RXIE);
}

/**
 * @brief  Check if the UART6_7  Receive Data Interrupt is enabled or disabled.
 * @rmtoll CTRL          RXIE        LL_UART6_7_IsEnabledIT_RX
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledIT_RX(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_RXIE) == (UART6_7_CTRL_RXIE));
}

/**
 * @brief  Enable Error Interrupt
 * @rmtoll CTRL          ERRIE        LL_UART6_7_EnableIT_ERR
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableIT_ERR(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CTRL, UART6_7_CTRL_ERRIE);
}

/**
 * @brief  Disable Error Interrupt
 * @rmtoll CTRL          ERRIE        LL_UART6_7_DisableIT_ERR
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableIT_ERR(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CTRL, UART6_7_CTRL_ERRIE);
}

/**
 * @brief  Check if the UART6_7 Error Interrupt is enabled or disabled.
 * @rmtoll CTRL          ERRIE        LL_UART6_7_IsEnabledIT_ERR
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledIT_ERR(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_ERRIE) == (UART6_7_CTRL_ERRIE));
}

/**
 * @brief  Enable Receive Timeout Interrupt (just use for uart6_7)
 * @rmtoll CTRL          RX_TO_IE        LL_UART6_7_EnableIT_RXTO
 * @param  UART6_7x UART6_7 Instance (just use uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableIT_RXTO(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CTRL, UART6_7_CTRL_RX_TO_IE);
}

/**
 * @brief  Disable Receive Timeout Interrupt (just use for uart6_7)
 * @rmtoll CTRL          RX_TO_IE        LL_UART6_7_DisableIT_RXTO
 * @param  UART6_7x UART6_7 Instance (just use uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableIT_RXTO(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CTRL, UART6_7_CTRL_RX_TO_IE);
}

/**
 * @brief  Check if the UART6_7 Receive Timeout Interrupt is enabled or disabled (just use for uart6_7)
 * @rmtoll CTRL          RX_TO_IE        LL_UART6_7_IsEnabledIT_RXTO
 * @param  UART6_7x UART6_7 Instance (just use uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledIT_RXTO(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_RX_TO_IE) == (UART6_7_CTRL_RX_TO_IE));
}

/**
 * @}
 */

/** @defgroup UART6_7_LL_EF_Metering_Mode_IT_Management Metering Mode IT_Management for uart6_7
 * @{
 */

/**
 * @brief  Enable receiving PF pulse interrupt (only for uart6_7)
 * @rmtoll CTRL          PF_IE        LL_UART6_7_EnableIT_PF
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableIT_PF(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CTRL, UART6_7_CTRL_PF_IE);
}

/**
 * @brief  Disable receiving PF pulse interrupt (only for uart6_7)
 * @rmtoll CTRL          PF_IE        LL_UART6_7_DisableIT_PF
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableIT_PF(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CTRL, UART6_7_CTRL_PF_IE);
}

/**
 * @brief  Check if the UART6_7 receiving PF interrupt pulse is enabled or disabled (only for uart6_7)
 * @rmtoll CTRL          PF_IE        LL_UART6_7_IsEnabledIT_PF
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledIT_PF(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_PF_IE) == (UART6_7_CTRL_PF_IE));
}

/**
 * @brief  Enable receiving QF pulse interrupt (only for uart6_7)
 * @rmtoll CTRL          QF_IE        LL_UART6_7_EnableIT_QF
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableIT_QF(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CTRL, UART6_7_CTRL_QF_IE);
}

/**
 * @brief  Disable receiving QF pulse interrupt (only for uart6_7)
 * @rmtoll CTRL          QF_IE        LL_UART6_7_DisableIT_QF
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableIT_QF(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CTRL, UART6_7_CTRL_QF_IE);
}

/**
 * @brief  Check if the UART6_7 receiving QF pulse interrupt is enabled or disabled (only for uart6_7)
 * @rmtoll CTRL          QF_IE        LL_UART6_7_IsEnabledIT_QF
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledIT_QF(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_QF_IE) == (UART6_7_CTRL_QF_IE));
}

/**
 * @brief  Enable receiving FPF pulse interrupt (only for uart6_7)
 * @rmtoll CTRL          FPF_IE        LL_UART6_7_EnableIT_FPF
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableIT_FPF(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CTRL, UART6_7_CTRL_FPF_IE);
}

/**
 * @brief  Disable receiving FPF pulse interrupt (only for uart6_7)
 * @rmtoll CTRL          FPF_IE        LL_UART6_7_DisableIT_FPF
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableIT_FPF(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CTRL, UART6_7_CTRL_FPF_IE);
}

/**
 * @brief  Check if the UART6_7 receiving FPF pulse interrupt is enabled or disabled (only for uart6_7)
 * @rmtoll CTRL          FPF_IE        LL_UART6_7_IsEnabledIT_FPF
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledIT_FPF(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_FPF_IE) == (UART6_7_CTRL_FPF_IE));
}

/**
 * @brief  Enable UART6_7_CF mode timeout error interrupt (only for uart6_7)
 * @rmtoll CTRL          CF_ERR_IE        LL_UART6_7_EnableIT_CFERR
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableIT_CFERR(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CTRL, UART6_7_CTRL_CF_ERR_IE);
}

/**
 * @brief  Disable UART6_7_CF mode timeout error interrupt (only for uart6_7)
 * @rmtoll CTRL          CF_ERR_IE        LL_UART6_7_DisableIT_CFERR
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableIT_CFERR(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CTRL, UART6_7_CTRL_CF_ERR_IE);
}

/**
 * @brief  Check if the UART6_7_CF mode timeout error interrupt is enabled or disabled (only for uart6_7)
 * @rmtoll CTRL          CF_ERR_IE        LL_UART6_7_IsEnabledIT_CFERR
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledIT_CFERR(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_CF_ERR_IE) == (UART6_7_CTRL_CF_ERR_IE));
}

/**
 * @brief  Enable low-level signal transmission completion interrupt (only for uart6_7)
 * @rmtoll CTRL          TW_IE        LL_UART6_7_EnableIT_TW
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableIT_TW(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CTRL, UART6_7_CTRL_TW_IE);
}

/**
 * @brief  Disable low-level signal transmission completion interrupt (only for uart6_7)
 * @rmtoll CTRL          TW_IE        LL_UART6_7_DisableIT_TW
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableIT_TW(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CTRL, UART6_7_CTRL_TW_IE);
}

/**
 * @brief  Check if the low-level signal transmission completion interrupt is enabled or disabled (only for uart6_7)
 * @rmtoll CTRL          TW_IE        LL_UART6_7_IsEnabledIT_TW
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledIT_TW(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_TW_IE) == (UART6_7_CTRL_TW_IE));
}

/**
 * @brief  Enable signal measurement completion interrupt (only for uart6_7)
 * @rmtoll CTRL          CAL_IE        LL_UART6_7_EnableIT_CAL
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableIT_CAL(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CTRL, UART6_7_CTRL_CAL_IE);
}

/**
 * @brief  Disable signal measurement completion interrupt (only for uart6_7)
 * @rmtoll CTRL          CAL_IE        LL_UART6_7_DisableIT_CAL
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableIT_CAL(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CTRL, UART6_7_CTRL_CAL_IE);
}

/**
 * @brief  Check if the signal measurement completion interrupt is enabled or disabled (only for uart6_7)
 * @rmtoll CTRL          CAL_IE        LL_UART6_7_IsEnabledIT_CAL
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledIT_CAL(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_CAL_IE) == (UART6_7_CTRL_CAL_IE));
}

/**
 * @brief  Enable receiving interrupt data interrupt (only for uart6_7)
 * @rmtoll CTRL          RX_IRQ_IE        LL_UART6_7_EnableIT_RXIRQ
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableIT_RXIRQ(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->CTRL, UART6_7_CTRL_RX_IRQ_IE);
}

/**
 * @brief  Disable receiving interrupt data interrupt (only for uart6_7)
 * @rmtoll CTRL          RX_IRQ_IE        LL_UART6_7_DisableIT_RXIRQ
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableIT_RXIRQ(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->CTRL, UART6_7_CTRL_RX_IRQ_IE);
}

/**
 * @brief  Check if the receiving interrupt data interrupt is enabled or disabled (only for uart6_7)
 * @rmtoll CTRL          RX_IRQ_IE        LL_UART6_7_IsEnabledIT_RXIRQ
 * @param  UART6_7x UART6_7 Instance (only for uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledIT_RXIRQ(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->CTRL, UART6_7_CTRL_RX_IRQ_IE) == (UART6_7_CTRL_RX_IRQ_IE));
}

/**
 * @}
 */

/** @defgroup UART6_7_LL_EF_FLAG_management FLAG_management
 * @{
 */

/**
 * @brief  Check if the UART6_7 Receive Complete Flag is set or not
 * @rmtoll STA           RX            LL_UART6_7_IsActiveFlag_RX
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_RX(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_RX) == (UART6_7_STA_RX));
}

/**
 * @brief  Clear Receive Complete Flag
 * @rmtoll STA           RX            LL_UART6_7_ClearFlag_RX
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ClearFlag_RX(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->STA, UART6_7_STA_RX);
}

/**
 * @brief  Check if the UART6_7 Transmit Complete Flag is set or not
 * @rmtoll STA           TX            LL_UART6_7_IsActiveFlag_TX
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_TX(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_TX) == (UART6_7_STA_TX));
}

/**
 * @brief  Clear Transmit Complete Flag
 * @rmtoll STA           TX            LL_UART6_7_ClearFlag_TX
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ClearFlag_TX(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->STA, UART6_7_STA_TX);
}

/**
 * @brief  Check if the UART6_7 Parity Error Flag is set or not
 * @rmtoll STA           PE            LL_UART6_7_IsActiveFlag_PE
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_PE(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_PE) == (UART6_7_STA_PE));
}

/**
 * @brief  Clear Parity Error Flag
 * @rmtoll STA           PE            LL_UART6_7_ClearFlag_PE
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ClearFlag_PE(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->STA, UART6_7_STA_PE);
}

/**
 * @brief  Check if the UART6_7 Overrun Error Flag is set or not
 * @rmtoll STA           OE            LL_UART6_7_IsActiveFlag_OE
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_OE(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_OE) == (UART6_7_STA_OE));
}

/**
 * @brief  Clear Overrun Error Flag
 * @rmtoll STA           OE            LL_UART6_7_ClearFlag_OE
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ClearFlag_OE(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->STA, UART6_7_STA_OE);
}

/**
 * @brief  Check if the UART6_7 Framing Error Flag is set or not
 * @rmtoll STA           FE            LL_UART6_7_IsActiveFlag_FE
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_FE(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_FE) == (UART6_7_STA_FE));
}

/**
 * @brief  Clear Framing Error Flag
 * @rmtoll STA           FE            LL_UART6_7_ClearFlag_FE
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ClearFlag_FE(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->STA, UART6_7_STA_FE);
}

/**
 * @brief  Check if the UART6_7 Data Error Flag is set or not
 * @rmtoll STA           DE            LL_UART6_7_IsActiveFlag_DE
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_DE(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_DE) == (UART6_7_STA_DE));
}

/**
 * @brief  Clear Data Error Flag
 * @rmtoll STA           DE            LL_UART6_7_ClearFlag_DE
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ClearFlag_DE(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->STA, UART6_7_STA_DE);
}

/**
 * @brief  Check if the UART6_7 Receiver Busy Flag is set or not (read only)
 * @rmtoll STA           RB            LL_UART6_7_IsActiveFlag_RB
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_RB(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_RB) == (UART6_7_STA_RB));
}

/**
 * @brief  Check if the UART6_7 Transmiter Busy Flag is set or not (read only)
 * @rmtoll STA           TB            LL_UART6_7_IsActiveFlag_TB
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_TB(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_TB) == (UART6_7_STA_TB));
}

/**
 * @brief  Check if the UART6_7 Transmit FIFO Empty Flag is set or not (read only)
 * @rmtoll STA           TX_FIFO_EMPTY            LL_UART6_7_IsActiveFlag_TXFIFOEMPTY
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_TXFIFOEMPTY(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_TX_FIFO_EMPTY) == (UART6_7_STA_TX_FIFO_EMPTY));
}

/**
 * @brief  Check if the UART6_7 Transmit FIFO Full Flag is set or not (read only)
 * @rmtoll STA           TX_FIFO_FULL            LL_UART6_7_IsActiveFlag_TXFIFOFULL
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_TXFIFOFULL(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_TX_FIFO_FULL) == (UART6_7_STA_TX_FIFO_FULL));
}

/**
 * @brief  Check if the UART6_7 Receive Timeout Flag is set or not
 * @rmtoll STA           RX_TO            LL_UART6_7_IsActiveFlag_RXTO
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_RXTO(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_RX_TO) == (UART6_7_STA_RX_TO));
}

/**
 * @brief  Clear Receive Timeout Flag
 * @rmtoll STA           RXTO            LL_UART6_7_ClearFlag_RXTO
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ClearFlag_RXTO(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->STA, UART6_7_STA_RX_TO);
}

/**
 * @}
 */

/** @defgroup UART6_7_LL_EF_Metering_Mode_FLAG_management Metering Mode FLAG_management for uart6_7
 * @{
 */

/**
 * @brief  Check if the Interrupt Flag for receiving PF pulse is set or not (just for uart6_7)
 * @rmtoll STA           PF_IF            LL_UART6_7_IsActiveFlag_PF
 * @param  UART6_7x UART6_7 Instance (just for uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_PF(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_PF_IF) == (UART6_7_STA_PF_IF));
}

/**
 * @brief  Clear the Interrupt Flag for receiving PF pulse (just for uart6_7)
 * @rmtoll STA           PF_IF            LL_UART6_7_ClearFlag_PF
 * @param  UART6_7x UART6_7 Instance (just for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ClearFlag_PF(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->STA, UART6_7_STA_PF_IF);
}

/**
 * @brief  Check if the Interrupt Flag for receiving QF pulse is set or not (just for uart6_7)
 * @rmtoll STA           QF_IF            LL_UART6_7_IsActiveFlag_QF
 * @param  UART6_7x UART6_7 Instance (just for uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_QF(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_QF_IF) == (UART6_7_STA_QF_IF));
}

/**
 * @brief  Clear the Interrupt Flag for receiving QF pulse (just for uart6_7)
 * @rmtoll STA           QF_IF            LL_UART6_7_ClearFlag_QF
 * @param  UART6_7x UART6_7 Instance (just for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ClearFlag_QF(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->STA, UART6_7_STA_QF_IF);
}

/**
 * @brief  Check if the Interrupt Flag for receiving FPF pulse is set or not (just for uart6_7)
 * @rmtoll STA           FPF_IF            LL_UART6_7_IsActiveFlag_FPF
 * @param  UART6_7x UART6_7 Instance (just for uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_FPF(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_FPF_IF) == (UART6_7_STA_FPF_IF));
}

/**
 * @brief  Clear the Interrupt Flag for receiving FPF pulse (just for uart6_7)
 * @rmtoll STA           FPF_IF            LL_UART6_7_ClearFlag_FPF
 * @param  UART6_7x UART6_7 Instance (just for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ClearFlag_FPF(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->STA, UART6_7_STA_FPF_IF);
}

/**
 * @brief  Check if the Interrupt Flag for UART6_7_CF mode timeout error is set or not (just for uart6_7)
 * @rmtoll STA           CF_ERR_IF            LL_UART6_7_IsActiveFlag_CFERR
 * @param  UART6_7x UART6_7 Instance (just for uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_CFERR(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_CF_ERR_IF) == (UART6_7_STA_CF_ERR_IF));
}

/**
 * @brief  Clear the Interrupt Flag for UART6_7_CF mode timeout error (just for uart6_7)
 * @rmtoll STA           CF_ERR_IF            LL_UART6_7_ClearFlag_CFERR
 * @param  UART6_7x UART6_7 Instance (just for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ClearFlag_CFERR(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->STA, UART6_7_STA_CF_ERR_IF);
}

/**
 * @brief  Check if the Interrupt Flag for low-level signal transmission completion (TW) is set or not (just for uart6_7)
 * @rmtoll STA           TW_IF            LL_UART6_7_IsActiveFlag_TW
 * @param  UART6_7x UART6_7 Instance (just for uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_TW(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_TW_IF) == (UART6_7_STA_TW_IF));
}

/**
 * @brief  Clear the Interrupt Flag for low-level signal transmission completion (TW) (just for uart6_7)
 * @rmtoll STA           TW_IF            LL_UART6_7_ClearFlag_TW
 * @param  UART6_7x UART6_7 Instance (just for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ClearFlag_TW(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->STA, UART6_7_STA_TW_IF);
}

/**
 * @brief  Check if the Interrupt Flag for signal measurement completion is set or not (just for uart6_7)
 * @rmtoll STA           CAL_IF            LL_UART6_7_IsActiveFlag_CAL
 * @param  UART6_7x UART6_7 Instance (just for uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_CAL(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_CAL_IF) == (UART6_7_STA_CAL_IF));
}

/**
 * @brief  Clear the Interrupt Flag for signal measurement completion (just for uart6_7)
 * @rmtoll STA           CAL_IF            LL_UART6_7_ClearFlag_CAL
 * @param  UART6_7x UART6_7 Instance (just for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ClearFlag_CAL(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->STA, UART6_7_STA_CAL_IF);
}

/**
 * @brief  Check if the the Interrupt Flag for receiving interrupt data (IRQ) is set or not (just for uart6_7)
 * @rmtoll STA           RX_IRQ_IF            LL_UART6_7_IsActiveFlag_RXIRQ
 * @param  UART6_7x UART6_7 Instance (just for uart6_7)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsActiveFlag_RXIRQ(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->STA, UART6_7_STA_RX_IRQ_IF) == (UART6_7_STA_RX_IRQ_IF));
}

/**
 * @brief  Clear the Interrupt Flag for receiving interrupt data (IRQ) (just for uart6_7)
 * @rmtoll STA           RX_IRQ_IF            LL_UART6_7_ClearFlag_RXIRQ
 * @param  UART6_7x UART6_7 Instance (just for uart6_7)
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_ClearFlag_RXIRQ(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->STA, UART6_7_STA_RX_IRQ_IF);
}

/**
 * @}
 */

/** @defgroup UART6_7_LL_EF_DMA_Management DMA_Management
 * @{
 */

/**
 * @brief  Enable DMA Mode for transmission
 * @rmtoll DMA_CTRL          TX_DMA_EN          LL_UART6_7_EnableDMAReq_TX
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableDMAReq_TX(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->DMA_CTRL, UART6_7_DMA_CTRL_TX_DMA_EN);
}

/**
 * @brief  Disable DMA Mode for transmission
 * @rmtoll DMA_CTRL          TX_DMA_EN          LL_UART6_7_DisableDMAReq_TX
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableDMAReq_TX(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->DMA_CTRL, UART6_7_DMA_CTRL_TX_DMA_EN);
}

/**
 * @brief  Check if DMA Mode is enabled for transmission
 * @rmtoll DMA_CTRL          TX_DMA_EN          LL_UART6_7_IsEnabledDMAReq_TX
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledDMAReq_TX(const UART6_7_TypeDef *UART6_7x)
{
  return (READ_BIT(UART6_7x->DMA_CTRL, UART6_7_DMA_CTRL_TX_DMA_EN) == (UART6_7_DMA_CTRL_TX_DMA_EN));
}

/**
 * @brief  Enable DMA Mode for reception
 * @rmtoll DMA_CTRL          RX_DMA_EN          LL_UART6_7_EnableDMAReq_RX
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_EnableDMAReq_RX(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->DMA_CTRL, UART6_7_DMA_CTRL_RX_DMA_EN);
}

/**
 * @brief  Disable DMA Mode for reception
 * @rmtoll DMA_CTRL          RX_DMA_EN          LL_UART6_7_DisableDMAReq_RX
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DisableDMAReq_RX(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->DMA_CTRL, UART6_7_DMA_CTRL_RX_DMA_EN);
}

/**
 * @brief  Check if DMA Mode is enabled for reception
 * @rmtoll DMA_CTRL          RX_DMA_EN          LL_UART6_7_IsEnabledDMAReq_RX
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_IsEnabledDMAReq_RX(const UART6_7_TypeDef *UART6_7x)
{
  return (READ_BIT(UART6_7x->DMA_CTRL, UART6_7_DMA_CTRL_RX_DMA_EN) == (UART6_7_DMA_CTRL_RX_DMA_EN));
}

/**
 * @brief  Enable DMA Transmit Cycle Mode
 * @rmtoll DMA_CTRL          TX_CYC_MODE        LL_UART6_7_DMA_EnableTxCircularMode
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_EnableTxCircularMode(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->DMA_CTRL, UART6_7_DMA_CTRL_TX_CYC_MODE);
}

/**
 * @brief  Disable DMA Transmit Cycle Mode
 * @rmtoll DMA_CTRL          TX_CYC_MODE        LL_UART6_7_DMA_DisableTxCircularMode
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_DisableTxCircularMode(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->DMA_CTRL, UART6_7_DMA_CTRL_TX_CYC_MODE);
}

/**
 * @brief  Check if the UART6_7 DMA Transmit Cycle Mode is enabled or disabled.
 * @rmtoll DMA_CTRL          TX_CYC_MODE        LL_UART6_7_DMA_IsEnabledTxCircularMode
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsEnabledTxCircularMode(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_CTRL, UART6_7_DMA_CTRL_TX_CYC_MODE) == (UART6_7_DMA_CTRL_TX_CYC_MODE));
}

/**
 * @brief  Enable DMA Receive Cycle Mode
 * @rmtoll DMA_CTRL          RX_CYC_MODE        LL_UART6_7_DMA_EnableRxCircularMode
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_EnableRxCircularMode(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->DMA_CTRL, UART6_7_DMA_CTRL_RX_CYC_MODE);
}

/**
 * @brief  Disable DMA Receive Cycle Mode
 * @rmtoll DMA_CTRL          RX_CYC_MODE        LL_UART6_7_DMA_DisableRxCircularMode
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_DisableRxCircularMode(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->DMA_CTRL, UART6_7_DMA_CTRL_RX_CYC_MODE);
}

/**
 * @brief  Check if the UART6_7 DMA Receive Cycle Mode is enabled or disabled.
 * @rmtoll DMA_CTRL          RX_CYC_MODE        LL_UART6_7_DMA_IsEnabledRxCircularMode
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsEnabledRxCircularMode(const UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_CTRL, UART6_7_DMA_CTRL_RX_CYC_MODE) == (UART6_7_DMA_CTRL_RX_CYC_MODE));
}

/**
 * @brief  Configure DMA Parity.
 * @rmtoll DMA_CTRL          DMA_PASS            LL_UART6_7_DMA_SetParity
 * @param  UART6_7x UART6_7 Instance
 * @param  Parity This parameter can be one of the following values:
 *         @arg @ref LL_UART6_7_DMA_PARITY_ZERO
 *         @arg @ref LL_UART6_7_DMA_PARITY_ONE
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_SetParity(UART6_7_TypeDef *UART6_7x, uint32_t Parity)
{
    MODIFY_REG(UART6_7x->DMA_CTRL, UART6_7_DMA_CTRL_DMA_PASS, Parity);
}

/**
 * @brief  Return DMA Parity configuration
 * @rmtoll DMA_CTRL          DMA_PASS            LL_UART6_7_DMA_GetParity
 * @param  UART6_7x UART6_7 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART6_7_DMA_PARITY_ZERO
 *         @arg @ref LL_UART6_7_DMA_PARITY_ONE
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_GetParity(UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->DMA_CTRL, UART6_7_DMA_CTRL_DMA_PASS));
}

/**
 * @brief  Configure DMA Transmit Base Address (byte address)
 * @rmtoll DMA_TBADR          DMA_TBADR            LL_UART6_7_DMA_SetTxStartAddress
 * @param  UART6_7x UART6_7 Instance
 * @param  TxStartAddress between Min_Data=0x00 and Max_Data=0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_SetTxStartAddress(UART6_7_TypeDef *UART6_7x, uint32_t TxStartAddress)
{
    MODIFY_REG(UART6_7x->DMA_TBADR, UART6_7_DMA_TBADR_DMA_TBADR, TxStartAddress << UART6_7_DMA_TBADR_DMA_TBADR_POS);
}

/**
 * @brief  Return DMA Transmit Base Address (byte address) configuration
 * @rmtoll DMA_TBADR          DMA_TBADR            LL_UART6_7_DMA_GetTxStartAddress
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_GetTxStartAddress(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->DMA_TBADR, UART6_7_DMA_TBADR_DMA_TBADR) >> UART6_7_DMA_TBADR_DMA_TBADR_POS);
}

/**
 * @brief  Configure DMA Receive Base Address (byte address)
 * @rmtoll DMA_RBADR          DMA_RBADR            LL_UART6_7_DMA_SetRxStartAddress
 * @param  UART6_7x UART6_7 Instance
 * @param  RxStartAddress between Min_Data=0x00 and Max_Data=0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_SetRxStartAddress(UART6_7_TypeDef *UART6_7x, uint32_t RxStartAddress)
{
    MODIFY_REG(UART6_7x->DMA_RBADR, UART6_7_DMA_RBADR_DMA_RBADR, RxStartAddress << UART6_7_DMA_RBADR_DMA_RBADR_POS);
}

/**
 * @brief  Return DMA Receive Base Address (byte address) configuration
 * @rmtoll DMA_RBADR          DMA_RBADR            LL_UART6_7_DMA_GetRxStartAddress
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_GetRxStartAddress(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->DMA_RBADR, UART6_7_DMA_RBADR_DMA_RBADR) >> UART6_7_DMA_RBADR_DMA_RBADR_POS);
}

/**
 * @brief  Configure DMA Transmit Length (in bytes)
 * @rmtoll DMA_TLEN          DMA_TLEN            LL_UART6_7_DMA_SetTxLength
 * @param  UART6_7x UART6_7 Instance
 * @param  TxLength between Min_Data=0x00 and Max_Data=0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_SetTxLength(UART6_7_TypeDef *UART6_7x, uint32_t TxLength)
{
    MODIFY_REG(UART6_7x->DMA_TLEN, UART6_7_DMA_TLEN_DMA_TLEN, TxLength << UART6_7_DMA_TLEN_DMA_TLEN_POS);
}

/**
 * @brief  Return DMA Transmit Length (in bytes) configuration
 * @rmtoll DMA_TLEN          DMA_TLEN            LL_UART6_7_DMA_GetTxLength
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_GetTxLength(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->DMA_TLEN, UART6_7_DMA_TLEN_DMA_TLEN) >> UART6_7_DMA_TLEN_DMA_TLEN_POS);
}

/**
 * @brief  Configure DMA Receive Length (in bytes)
 * @rmtoll DMA_RLEN          DMA_RLEN            LL_UART6_7_DMA_SetRxLength
 * @param  UART6_7x UART6_7 Instance
 * @param  RxLength between Min_Data=0x00 and Max_Data=0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_SetRxLength(UART6_7_TypeDef * UART6_7x, uint32_t RxLength)
{
    MODIFY_REG(UART6_7x->DMA_RLEN, UART6_7_DMA_RLEN_DMA_RLEN, RxLength << UART6_7_DMA_RLEN_DMA_RLEN_POS);
}

/**
 * @brief  Return DMA Receive Length (in bytes) configuration
 * @rmtoll DMA_RLEN          DMA_RLEN            LL_UART6_7_DMA_GetRxLength
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_GetRxLength(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->DMA_RLEN, UART6_7_DMA_RLEN_DMA_RLEN) >> UART6_7_DMA_RLEN_DMA_RLEN_POS);
}

/**
 * @brief  Return Current DMA Transmit Address (byte address) configuration
 * @rmtoll DMA_TADR          DMA_TADR            LL_UART6_7_DMA_GetTxAddress
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_GetTxAddress(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->DMA_TADR, UART6_7_DMA_TADR_DMA_TADR) >> UART6_7_DMA_TADR_DMA_TADR_POS);
}

/**
 * @brief  Return Current DMA Receive Address (byte address) configuration
 * @rmtoll DMA_RADR          DMA_RADR            LL_UART6_7_DMA_GetRxAddress
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_GetRxAddress(const UART6_7_TypeDef *UART6_7x)
{
    return (uint32_t)(READ_BIT(UART6_7x->DMA_RADR, UART6_7_DMA_TADR_DMA_TADR) >> UART6_7_DMA_RADR_DMA_RADR_POS);
}

/**
 * @}
 */

/** @defgroup UART6_7_LL_EF_DMA_IT_Management DMA_IT_Management
 * @{
 */

/**
 * @brief  Enable DMA Transmit Half Empty Interrupt
 * @rmtoll DMA_IE          TX_HIE        LL_UART6_7_DMA_EnableIT_TXHALF
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_EnableIT_TXHALF(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_TX_HIE);
}

/**
 * @brief  Disable DMA Transmit Half Empty Interrupt
 * @rmtoll DMA_IE          TX_HIE        LL_UART6_7_DMA_DisableIT_TXHALF
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_DisableIT_TXHALF(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_TX_HIE);
}

/**
 * @brief  Check if the UART6_7 DMA Transmit Half Empty Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          TX_HIE        LL_UART6_7_DMA_IsEnabledIT_TXHALF
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsEnabledIT_TXHALF(UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_TX_HIE) == (UART6_7_DMA_IE_TX_HIE));
}

/**
 * @brief  Enable DMA Transmit Empty (Full) Interrupt
 * @rmtoll DMA_IE          TX_FIE        LL_UART6_7_DMA_EnableIT_TXFULL
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_EnableIT_TXFULL(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_TX_FIE);
}

/**
 * @brief  Disable DMA Transmit Empty (Full) Interrupt
 * @rmtoll DMA_IE          TX_FIE        LL_UART6_7_DMA_DisableIT_TXFULL
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_DisableIT_TXFULL(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_TX_FIE);
}

/**
 * @brief  Check if the UART6_7 DMA Transmit Empty (Full) Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          TX_FIE        LL_UART6_7_DMA_IsEnabledIT_TXFULL
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsEnabledIT_TXFULL(UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_TX_FIE) == (UART6_7_DMA_IE_TX_FIE));
}

/**
 * @brief  Enable DMA Receive Half Full Interrupt
 * @rmtoll DMA_IE          RX_HIE        LL_UART6_7_DMA_EnableIT_RXHALF
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_EnableIT_RXHALF(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_RX_HIE);
}

/**
 * @brief  Disable DMA Receive Half Full Interrupt
 * @rmtoll DMA_IE          RX_HIE        LL_UART6_7_DMA_DisableIT_RXHALF
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_DisableIT_RXHALF(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_RX_HIE);
}

/**
 * @brief  Check if the UART6_7 DMA Receive Half Full Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          RX_HIE        LL_UART6_7_DMA_IsEnabledIT_RXHALF
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsEnabledIT_RXHALF(UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_RX_HIE) == (UART6_7_DMA_IE_RX_HIE));
}

/**
 * @brief  Enable DMA Receive Full Interrupt
 * @rmtoll DMA_IE          RX_FIE        LL_UART6_7_DMA_EnableIT_RXFULL
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_EnableIT_RXFULL(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_RX_FIE);
}

/**
 * @brief  Disable DMA Receive Full Interrupt
 * @rmtoll DMA_IE          RX_FIE        LL_UART6_7_DMA_DisableIT_RXFULL
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_DisableIT_RXFULL(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_RX_FIE);
}

/**
 * @brief  Check if the UART6_7 DMA Receive Full Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          RX_FIE        LL_UART6_7_DMA_IsEnabledIT_RXFULL
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsEnabledIT_RXFULL(UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_RX_FIE) == (UART6_7_DMA_IE_RX_FIE));
}

/**
 * @brief  Enable DMA Transmit Data Error Interrupt
 * @rmtoll DMA_IE          TX_ERR_IE        LL_UART6_7_DMA_EnableIT_TXERR
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_EnableIT_TXERR(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_TX_ERR_IE);
}

/**
 * @brief  Disable DMA Transmit Data Error Interrupt
 * @rmtoll DMA_IE          TX_ERR_IE        LL_UART6_7_DMA_DisableIT_TXERR
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_DisableIT_TXERR(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_TX_ERR_IE);
}

/**
 * @brief  Check if the UART6_7 DMA Transmit Data Error Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          TX_ERR_IE        LL_UART6_7_DMA_IsEnabledIT_TXERR
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsEnabledIT_TXERR(UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_TX_ERR_IE) == (UART6_7_DMA_IE_TX_ERR_IE));
}

/**
 * @brief  Enable DMA Receive Data Error Interrupt
 * @rmtoll DMA_IE          RX_ERR_IE        LL_UART6_7_DMA_EnableIT_RXERR
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_EnableIT_RXERR(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_RX_ERR_IE);
}

/**
 * @brief  Disable DMA Receive Data Error Interrupt
 * @rmtoll DMA_IE          RX_ERR_IE        LL_UART6_7_DMA_DisableIT_RXERR
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_DisableIT_RXERR(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_RX_ERR_IE);
}

/**
 * @brief  Check if the UART6_7 DMA Receive Data Error Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          RX_ERR_IE        LL_UART6_7_DMA_IsEnabledIT_RXERR
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsEnabledIT_RXERR(UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_RX_ERR_IE) == (UART6_7_DMA_IE_RX_ERR_IE));
}

/**
 * @brief  Enable DMA Receive Complete Interrupt
 * @rmtoll DMA_IE          RX_DONE_IE        LL_UART6_7_DMA_EnableIT_RXTC
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_EnableIT_RXTC(UART6_7_TypeDef *UART6_7x)
{
    SET_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_RX_DONE_IE);
}

/**
 * @brief  Disable DMA Receive Complete Interrupt
 * @rmtoll DMA_IE          RX_DONE_IE        LL_UART6_7_DMA_DisableIT_RXTC
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_DisableIT_RXTC(UART6_7_TypeDef *UART6_7x)
{
    CLEAR_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_RX_DONE_IE);
}

/**
 * @brief  Check if the UART6_7 DMA Receive Complete Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          RX_DONE_IE        LL_UART6_7_DMA_IsEnabledIT_RXTC
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsEnabledIT_RXTC(UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_IE, UART6_7_DMA_IE_RX_DONE_IE) == (UART6_7_DMA_IE_RX_DONE_IE));
}

/**
 * @}
 */

/** @defgroup UART6_7_LL_EF_DMA_FLAG_management DMA_FLAG_management
 * @{
 */

/**
 * @brief  Check if the UART6_7 DMA Transmit Half Empty Interrupt Flag is set or not
 * @rmtoll DMA_IF           TX_HDONE            LL_UART6_7_DMA_IsActiveFlag_TXHALF
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsActiveFlag_TXHALF(UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_IF, UART6_7_DMA_IF_TX_HDONE) == (UART6_7_DMA_IF_TX_HDONE));
}

/**
 * @brief  Clear DMA Transmit Half Empty Interrupt Flag
 * @rmtoll DMA_IF           TX_HDONE            LL_UART6_7_DMA_ClearFlag_TXHALF
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_ClearFlag_TXHALF(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->DMA_IF, UART6_7_DMA_IF_TX_HDONE);
}

/**
 * @brief  Check if the UART6_7 DMA Transmit Empty (Full) Interrupt Flag is set or not
 * @rmtoll DMA_IF           TX_FDONE            LL_UART6_7_DMA_IsActiveFlag_TXFULL
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsActiveFlag_TXFULL(UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_IF, UART6_7_DMA_IF_TX_FDONE) == (UART6_7_DMA_IF_TX_FDONE));
}

/**
 * @brief  Clear DMA Transmit Empty (Full) Interrupt Flag
 * @rmtoll DMA_IF           TX_FDONE            LL_UART6_7_DMA_ClearFlag_TXFULL
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_ClearFlag_TXFULL(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->DMA_IF, UART6_7_DMA_IF_TX_FDONE);
}

/**
 * @brief  Check if the UART6_7 DMA Receive Half Full Interrupt Flag is set or not
 * @rmtoll DMA_IF           RX_HDONE            LL_UART6_7_DMA_IsActiveFlag_RXHALF
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsActiveFlag_RXHALF(UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_IF, UART6_7_DMA_IF_RX_HDONE) == (UART6_7_DMA_IF_RX_HDONE));
}

/**
 * @brief  Clear DMA Receive Half Full Interrupt Flag
 * @rmtoll DMA_IF           RX_HDONE            LL_UART6_7_DMA_ClearFlag_RXHALF
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_ClearFlag_RXHALF(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->DMA_IF, UART6_7_DMA_IF_RX_HDONE);
}

/**
 * @brief  Check if the UART6_7 DMA Receive Half Full Interrupt Flag is set or not
 * @rmtoll DMA_IF           RX_FDONE            LL_UART6_7_DMA_IsActiveFlag_RXFULL
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsActiveFlag_RXFULL(UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_IF, UART6_7_DMA_IF_RX_FDONE) == (UART6_7_DMA_IF_RX_FDONE));
}

/**
 * @brief  Clear DMA Receive Half Full Interrupt Flag
 * @rmtoll DMA_IF           RX_FDONE            LL_UART6_7_DMA_ClearFlag_RXFULL
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_ClearFlag_RXFULL(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->DMA_IF, UART6_7_DMA_IF_RX_FDONE);
}

/**
 * @brief  Check if the UART6_7 DMA Receive Data Error Flag is set or not
 * @rmtoll DMA_IF           RX_ERR            LL_UART6_7_DMA_IsActiveFlag_RXERR
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsActiveFlag_RXERR(UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_IF, UART6_7_DMA_IF_RX_ERR) == (UART6_7_DMA_IF_RX_ERR));
}

/**
 * @brief  Clear DMA Receive Data Error Flag
 * @rmtoll DMA_IF           RX_ERR            LL_UART6_7_DMA_ClearFlag_RXERR
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_ClearFlag_RXERR(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->DMA_IF, UART6_7_DMA_IF_RX_ERR);
}

/**
 * @brief  Check if the UART6_7 DMA Receive Complete Flag is set or not
 * @rmtoll DMA_IF           RX_DONE            LL_UART6_7_DMA_IsActiveFlag_RXTC
 * @param  UART6_7x UART6_7 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART6_7_DMA_IsActiveFlag_RXTC(UART6_7_TypeDef *UART6_7x)
{
    return (READ_BIT(UART6_7x->DMA_IF, UART6_7_DMA_IF_RX_DONE) == (UART6_7_DMA_IF_RX_DONE));
}

/**
 * @brief  Clear DMA Receive Complete Flag
 * @rmtoll DMA_IF           RX_DONE            LL_UART6_7_DMA_ClearFlag_RXTC
 * @param  UART6_7x UART6_7 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_DMA_ClearFlag_RXTC(UART6_7_TypeDef *UART6_7x)
{
    WRITE_REG(UART6_7x->DMA_IF, UART6_7_DMA_IF_RX_DONE);
}

/**
 * @}
 */

/** @defgroup UART6_7_LL_EF_Data_Management Data_Management
 * @{
 */

/**
 * @brief  Read Receiver Data register (Receive Data value, 8 bits)
 * @rmtoll RXDATA           RXDATA            LL_UART6_7_ReceiveData8
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF
 */
__STATIC_INLINE uint8_t LL_UART6_7_ReceiveData8(const UART6_7_TypeDef *UART6_7x)
{
    return (uint8_t)(READ_BIT(UART6_7x->RXDATA, UART6_7_RXDATA_RXDATA));
}

/**
 * @brief  Read Receiver Data register (Receive Data value, 9 bits)
 * @rmtoll RXDATA           RXDATA            LL_UART6_7_ReceiveData9\n
 *         RXDATA           UP                LL_UART6_7_ReceiveData9
 * @param  UART6_7x UART6_7 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x1FF
 */
__STATIC_INLINE uint16_t LL_UART6_7_ReceiveData9(const UART6_7_TypeDef *UART6_7x)
{
    return (uint16_t)(READ_BIT(UART6_7x->RXDATA, UART6_7_RXDATA_RXDATA | UART6_7_RXDATA_UP));
}

/**
 * @brief  Write in Transmitter Data Register (Transmit Data value, 8 bits)
 * @rmtoll TXDATA           TXDATA            LL_UART6_7_TransmitData8
 * @param  UART6_7x UART6_7 Instance
 * @param  Value between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_TransmitData8(UART6_7_TypeDef *UART6_7x, uint8_t Value)
{
    UART6_7x->TXDATA = Value;
}

/**
 * @brief  Write in Transmitter Data Register (Transmit Data value, 9 bits)
 * @rmtoll TXDATA           TXDATA            LL_UART6_7_TransmitData9\n
 *         TXDATA           UP                LL_UART6_7_TransmitData9
 * @param  UART6_7x UART6_7 Instance
 * @param  Value between Min_Data=0x00 and Max_Data=0x1FF
 * @retval None
 */
__STATIC_INLINE void LL_UART6_7_TransmitData9(UART6_7_TypeDef *UART6_7x, uint16_t Value)
{
    UART6_7x->TXDATA = Value & 0x1FFU;
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup UART6_7_LL_EF_Init Initialization and de-initialization functions
 * @{
 */

// The initialization function can have more than one argument.
ErrorStatus LL_UART6_7_Init(UART6_7_TypeDef *UART6_7x, const LL_UART6_7_InitTypeDef *UART6_7_InitStruct);
ErrorStatus LL_UART6_7_DeInit(UART6_7_TypeDef *UART6_7x);
void        LL_UART6_7_StructInit(LL_UART6_7_InitTypeDef *UART6_7_InitStruct); // If the module is not suitable for the default configuration, you could not defines this function.

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

#endif /* UART6 || UART7 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_UART6_7_H */
