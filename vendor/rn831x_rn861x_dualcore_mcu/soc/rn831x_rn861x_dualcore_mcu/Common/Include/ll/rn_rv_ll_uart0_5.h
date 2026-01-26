/**
 *****************************************************************************
 * @file   rn_rv_ll_uart0_5.h
 * @author Renergy Software Team
 * @brief  Header file of UART0_5 LL module.
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
#ifndef __RN_LL_UART0_5_H
#define __RN_LL_UART0_5_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_uart0_5.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (UART0) || defined (UART1) || defined (UART2) || defined (UART3) || defined (UART4) || defined (UART5)

/** @defgroup UART0_5_LL UART0_5
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup UART0_5_LL_Private_Constants UART0_5 Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup UART0_5_LL_Private_Macros UART0_5 Private Macros
 * @{
 */


/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup UART0_5_LL_ES_INIT UART0_5 Exported Init structure
 * @{
 */

/**
 * @brief LL UART0_5 Init Structure definition
 */
typedef struct
{
    uint32_t BaudRate;                  /*!< This field defines expected Uart communication baud rate.

                                           This feature can be modified afterwards using unitary function @ref LL_UART0_5_SetBaudRate().*/

    uint32_t DataWidth;                 /*!< Specifies the number of data bits transmitted or received in a frame.
                                             This parameter can be a value of @ref UART0_5_LL_EC_DATAWIDTH.

                                             This feature can be modified afterwards using unitary function @ref LL_UART0_5_SetDataWidth().*/

    uint32_t StopBits;                  /*!< Specifies the number of stop bits transmitted.
                                             This parameter can be a value of @ref UART0_5_LL_EC_STOPBITS.

                                             This feature can be modified afterwards using unitary function @ref LL_UART0_5_SetStopBitsLength().*/

    uint32_t Parity;                    /*!< Specifies the parity mode.
                                             This parameter can be a value of @ref UART0_5_LL_EC_PARITY.

                                             This feature can be modified afterwards using unitary function @ref LL_UART0_5_SetParity().*/

    uint32_t TransferBitOrder;          /*!< Specifies the data transmitted/received with the LSB first or MSB first.
                                             This parameter can be a value of @ref UART0_5_LL_EC_BIT_ORDER.

                                             This feature can be modified afterwards using unitary function @ref LL_UART0_5_SetTransferBitOrder().*/

} LL_UART0_5_InitTypeDef;

/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/* Exported constants -------------------------------------------------------*/
/** @defgroup UART0_5_LL_Exported_Constants UART0_5 Exported Constants
 * @{
 */

/** @defgroup UART0_5_LL_EC_PARITY Parity Control
 * @{
 */
#define LL_UART0_5_PARITY_NONE                    UART0_5_CTRL_PARS_NONE                  /*!< No parity           */
#define LL_UART0_5_PARITY_ODD                     UART0_5_CTRL_PARS_ODD                   /*!< Odd parity          */
#define LL_UART0_5_PARITY_EVEN                    UART0_5_CTRL_PARS_EVEN                  /*!< Even parity         */
#define LL_UART0_5_PARITY_ZERO                    UART0_5_CTRL_PARS_ZERO                  /*!< Fixed to 0 parity   */
#define LL_UART0_5_PARITY_ONE                     UART0_5_CTRL_PARS_ONE                   /*!< Fixed to 1 parity   */
#define LL_UART0_5_PARITY_USER_DEFINED            UART0_5_CTRL_PARS_USER_DEFINED          /*!< User-defined parity */
/**
 * @}
 */

/** @defgroup UART0_5_LL_EC_DATAWIDTH Datawidth
 * @{
 */
#define LL_UART0_5_DATAWIDTH_5B                   UART0_5_CTRL_DATLEN_5B          /*!< 5 bits data length */
#define LL_UART0_5_DATAWIDTH_6B                   UART0_5_CTRL_DATLEN_6B          /*!< 6 bits data length */
#define LL_UART0_5_DATAWIDTH_7B                   UART0_5_CTRL_DATLEN_7B          /*!< 7 bits data length */
#define LL_UART0_5_DATAWIDTH_8B                   UART0_5_CTRL_DATLEN_8B          /*!< 8 bits data length */
/**
 * @}
 */

/** @defgroup UART0_5_LL_EC_BIT_ORDER Transmission Bit Order
 * @{
 */
#define LL_UART0_5_LSB_FIRST                    UART0_5_CTRL_LMSB_LSB_FIRST             /*!< Data is transmitted/received with the LSB first */
#define LL_UART0_5_MSB_FIRST                    UART0_5_CTRL_LMSB_MSB_FIRST             /*!< Data is transmitted/received with the MSB first */
/**
 * @}
 */

/** @defgroup UART0_5_LL_EC_DATA_POLARITY Data Polarity
 * @{
 */
#define LL_UART0_5_DATA_POLARITY_HIGH                 UART0_5_CTRL_NEG_POSITIVE         /*!< Positive polarity (Idle level = High) */
#define LL_UART0_5_DATA_POLARITY_LOW                  UART0_5_CTRL_NEG_NEGATIVE         /*!< Negative polarity (Idle level = Low)  */
/**
 * @}
 */

/** @defgroup UART0_5_LL_EC_IRDA_POLARITY IRDA Polarity
 * @{
 */
#define LL_UART0_5_IRDA_POLARITY_HIGH                   UART0_5_CTRL_IRSEL_POSITIVE          /*!< Positive polarity (Idle level = High, Modulation = Low) */
#define LL_UART0_5_IRDA_POLARITY_LOW                    UART0_5_CTRL_IRSEL_NEGATIVE          /*!< Negative polarity (Idle level = Low, Modulation = High) */
/**
 * @}
 */

/** @defgroup UART0_5_LL_EC_STOPBITS Stop Bits
 * @{
 */
#define LL_UART0_5_STOPBITS_1                     UART0_5_CTRL_STOPS_1                       /*!< 1 stop bit */
#define LL_UART0_5_STOPBITS_2                     UART0_5_CTRL_STOPS_2                       /*!< 2 stop bit */
/**
 * @}
 */

/** @defgroup UART0_5_LL_EC_PARMEM Parity Error Store Memory
 * @{
 */
#define LL_UART0_5_PARMEM_STORE_MEMORY                UART0_5_CTRL_PAR_MEM_STORE_MEMORY                   /*!< Data is stored in memory regardless of parity error (for uart0_5).                 */
#define LL_UART0_5_PARMEM_NOT_STORE_MEMORY            UART0_5_CTRL_PAR_MEM_NOT_STORE_MEMORY               /*!< When parity error occurs, the current frame is not stored in memory (for uart0_5). */
/**
 * @}
 */

/** @defgroup UART0_5_LL_EC_DMA_PARITY DMA_PARITY
 * @{
 */
#define LL_UART0_5_DMA_PARITY_ZERO                     UART0_5_DMA_CTRL_DMA_PASS_ZERO                      /*!< DMA Transmit Fixed to 0 parity */
#define LL_UART0_5_DMA_PARITY_ONE                      UART0_5_DMA_CTRL_DMA_PASS_ONE                       /*!< DMA Transmit Fixed to 1 parity */
/**
 * @}
 */

/** @defgroup UART0_5_LL_EC_DELAY_INSERT Delay_Insert
 * @{
 */
#define LL_UART0_5_DRLAY_TIMEOUT_CNT_1B                UART0_5_TX_DRLAY_TIMEOUT_CNT_1B                     /*!< 1 UART0_5 data bit time  */
#define LL_UART0_5_DRLAY_TIMEOUT_CNT_2B                UART0_5_TX_DRLAY_TIMEOUT_CNT_2B                     /*!< 2 UART0_5 data bit times */
#define LL_UART0_5_DRLAY_TIMEOUT_CNT_3B                UART0_5_TX_DRLAY_TIMEOUT_CNT_3B                     /*!< 3 UART0_5 data bit times */
#define LL_UART0_5_DRLAY_TIMEOUT_CNT_4B                UART0_5_TX_DRLAY_TIMEOUT_CNT_4B                     /*!< 4 UART0_5 data bit times */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup UART0_5_LL_Exported_Macros UART0_5 Exported Macros
 * @{
 */

/** @defgroup UART0_5_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in UART0_5 register
 * @param  __INSTANCE__ UART0_5 Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_UART0_5_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in UART0_5 register
 * @param  __INSTANCE__ UART0_5 Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_UART0_5_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/** @defgroup UART0_5_LL_EM_Exported_Macros_Helper Exported Macros Helper
 * @{
 */

/**
 * @brief  Compute UART0_5 DIV value according to Peripheral Clock and expected Baud Rate.
 * @param  __PERIPHCLK__ Peripheral Clock frequency used for UART0_5 instance
 * @param  __BAUDRATE__ Baud rate value to achieve
 * @retval UART0_5 DIV value to be used for BAUD register
 */
#define __LL_UART0_5_CALC_BAUDRATE_DIVMANT(__PERIPHCLK__, __BAUDRATE__)   (((__PERIPHCLK__) / ((__BAUDRATE__) << 4)) - 1)

#define __LL_UART0_5_CALC_BAUDRATE_DIVFRAC(__PERIPHCLK__, __BAUDRATE__)   (( ((uint64_t)( (__PERIPHCLK__) % ((__BAUDRATE__) << 4) ) << 14) + (((__BAUDRATE__) << 4) >> 1) ) \
                                                                            / ((__BAUDRATE__) << 4) )

/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup UART0_5_LL_Exported_Functions UART0_5 Exported Functions
 * @{
 */

/** @defgroup UART0_5_LL_EF_Configuration Configuration functions
 * @{
 */

/**
 * @brief  UART0_5 Enable
 * @rmtoll CTRL          EN            LL_UART0_5_Enable
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_Enable(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->CTRL, UART0_5_CTRL_EN);
}

/**
 * @brief  UART0_5 Disable (all UART0_5 prescalers and outputs are disabled)
 * @rmtoll CTRL          EN            LL_UART0_5_Disable
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_Disable(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->CTRL, UART0_5_CTRL_EN);
}

/**
 * @brief  Indicate if UART0_5 is enabled
 * @rmtoll CTRL          EN            LL_UART0_5_IsEnabled
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsEnabled(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->CTRL, UART0_5_CTRL_EN) == (UART0_5_CTRL_EN));
}

/**
 * @brief  Set the length of the stop bits
 * @rmtoll CTRL          STOPS          LL_UART0_5_SetStopBitsLength
 * @param  UART0_5x UART0_5 Instance
 * @param  StopBits This parameter can be one of the following values:
 *         @arg @ref LL_UART0_5_STOPBITS_1
 *         @arg @ref LL_UART0_5_STOPBITS_2
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_SetStopBitsLength(UART0_5_TypeDef *UART0_5x, uint32_t StopBits)
{
    MODIFY_REG(UART0_5x->CTRL, UART0_5_CTRL_STOPS, StopBits);
}

/**
 * @brief  Retrieve the length of the stop bits
 * @rmtoll CTRL          STOPS          LL_UART0_5_GetStopBitsLength
 * @param  UART0_5x UART0_5 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART0_5_STOPBITS_1
 *         @arg @ref LL_UART0_5_STOPBITS_2
 */
__STATIC_INLINE uint32_t LL_UART0_5_GetStopBitsLength(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->CTRL, UART0_5_CTRL_STOPS));
}

/**
 * @brief  Set Word length (i.e. nb of data bits, excluding start and stop bits)
 * @rmtoll CTRL          DATLEN             LL_UART0_5_SetDataWidth
 * @param  UART0_5x UART0_5 Instance
 * @param  DataWidth This parameter can be one of the following values:
 *         @arg @ref LL_UART0_5_DATAWIDTH_5B
 *         @arg @ref LL_UART0_5_DATAWIDTH_6B
 *         @arg @ref LL_UART0_5_DATAWIDTH_7B
 *         @arg @ref LL_UART0_5_DATAWIDTH_8B
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_SetDataWidth(UART0_5_TypeDef *UART0_5x, uint32_t DataWidth)
{
    MODIFY_REG(UART0_5x->CTRL, UART0_5_CTRL_DATLEN, DataWidth);
}

/**
 * @brief  Return Word length (i.e. nb of data bits, excluding start and stop bits)
 * @rmtoll CTRL          DATLEN             LL_UART0_5_GetDataWidth
 * @param  UART0_5x UART0_5 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART0_5_DATAWIDTH_5B
 *         @arg @ref LL_UART0_5_DATAWIDTH_6B
 *         @arg @ref LL_UART0_5_DATAWIDTH_7B
 *         @arg @ref LL_UART0_5_DATAWIDTH_8B
 */
__STATIC_INLINE uint32_t LL_UART0_5_GetDataWidth(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->CTRL, UART0_5_CTRL_DATLEN));
}

/**
 * @brief  Configure Parity.
 * @rmtoll CTRL          PARS            LL_UART0_5_SetParity
 * @param  UART0_5x UART0_5 Instance
 * @param  Parity This parameter can be one of the following values:
 *         @arg @ref LL_UART0_5_PARITY_NONE
 *         @arg @ref LL_UART0_5_PARITY_ODD
 *         @arg @ref LL_UART0_5_PARITY_EVEN
 *         @arg @ref LL_UART0_5_PARITY_ZERO
 *         @arg @ref LL_UART0_5_PARITY_ONE
 *         @arg @ref LL_UART0_5_PARITY_USER_DEFINED
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_SetParity(UART0_5_TypeDef *UART0_5x, uint32_t Parity)
{
    MODIFY_REG(UART0_5x->CTRL, UART0_5_CTRL_PARS, Parity);
}

/**
 * @brief  Return Parity configuration
 * @rmtoll CTRL          PARS            LL_UART0_5_GetParity
 * @param  UART0_5x UART0_5 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART0_5_PARITY_NONE
 *         @arg @ref LL_UART0_5_PARITY_ODD
 *         @arg @ref LL_UART0_5_PARITY_EVEN
 *         @arg @ref LL_UART0_5_PARITY_ZERO
 *         @arg @ref LL_UART0_5_PARITY_ONE
 *         @arg @ref LL_UART0_5_PARITY_USER_DEFINED
 */
__STATIC_INLINE uint32_t LL_UART0_5_GetParity(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->CTRL, UART0_5_CTRL_PARS));
}

/**
 * @brief  Configure Character frame format (Datawidth, Parity control, Stop Bits)
 * @note   Call of this function is equivalent to following function call sequence :
 *         - Data Width configuration using @ref LL_UART0_5_SetDataWidth() function
 *         - Parity Control and mode configuration using @ref LL_UART0_5_SetParity() function
 *         - Stop bits configuration using @ref LL_UART0_5_SetStopBitsLength() function
 * @rmtoll CTRL          DATLEN            LL_UART0_5_ConfigCharacter\n
 *         CTRL          PARS              LL_UART0_5_ConfigCharacter\n
 *         CTRL          STOPS             LL_UART0_5_ConfigCharacter
 * @param  UART0_5x UART0_5 Instance
 * @param  DataWidth This parameter can be one of the following values:
 *         @arg @ref LL_UART0_5_DATAWIDTH_5B
 *         @arg @ref LL_UART0_5_DATAWIDTH_6B
 *         @arg @ref LL_UART0_5_DATAWIDTH_7B
 *         @arg @ref LL_UART0_5_DATAWIDTH_8B
 * @param  Parity This parameter can be one of the following values:
 *         @arg @ref LL_UART0_5_PARITY_NONE
 *         @arg @ref LL_UART0_5_PARITY_ODD
 *         @arg @ref LL_UART0_5_PARITY_EVEN
 *         @arg @ref LL_UART0_5_PARITY_ZERO
 *         @arg @ref LL_UART0_5_PARITY_ONE
 *         @arg @ref LL_UART0_5_PARITY_USER_DEFINED
 * @param  StopBits This parameter can be one of the following values:
 *         @arg @ref LL_UART0_5_STOPBITS_1
 *         @arg @ref LL_UART0_5_STOPBITS_2
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_ConfigCharacter(UART0_5_TypeDef *UART0_5x, uint32_t DataWidth, uint32_t Parity,
                                              uint32_t StopBits)
{
    MODIFY_REG(UART0_5x->CTRL, UART0_5_CTRL_DATLEN | UART0_5_CTRL_PARS | UART0_5_CTRL_STOPS, Parity | DataWidth | StopBits);
}

/**
 * @brief  Internal Loopback Enable
 * @rmtoll CTRL          ILBE            LL_UART0_5_EnableInternalLoop
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_EnableInternalLoop(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->CTRL, UART0_5_CTRL_ILBE);
}

/**
 * @brief  Internal Loopback Disable
 * @rmtoll CTRL          ILBE            LL_UART0_5_DisableInternalLoop
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DisableInternalLoop(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->CTRL, UART0_5_CTRL_ILBE);
}

/**
 * @brief  Indicate if Internal Loopback is enabled
 * @rmtoll CTRL          ILBE            LL_UART0_5_IsEnabledInternalLoop
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsEnabledInternalLoop(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->CTRL, UART0_5_CTRL_ILBE) == (UART0_5_CTRL_ILBE));
}

/**
 * @brief  Set transfer bit order
 * @note   This bit should not be changed when communication is ongoing.
 * @rmtoll CTRL          LMSB      LL_UART0_5_SetTransferBitOrder
 * @param  UART0_5x UART0_5 Instance
 * @param  BitOrder This parameter can be one of the following values:
 *         @arg @ref LL_UART0_5_LSB_FIRST
 *         @arg @ref LL_UART0_5_MSB_FIRST
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_SetTransferBitOrder(UART0_5_TypeDef *UART0_5x, uint32_t BitOrder)
{
    MODIFY_REG(UART0_5x->CTRL, UART0_5_CTRL_LMSB, BitOrder);
}

/**
 * @brief  Get transfer bit order
 * @rmtoll CTRL          LMSB      LL_UART0_5_GetTransferBitOrder
 * @param  UART0_5x UART0_5 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART0_5_LSB_FIRST
 *         @arg @ref LL_UART0_5_MSB_FIRST
 */
__STATIC_INLINE uint32_t LL_UART0_5_GetTransferBitOrder(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->CTRL, UART0_5_CTRL_LMSB));
}

/**
 * @brief  Select the polarity of the trasmit and receive data
 * @rmtoll CTRL          NEG          LL_UART0_5_SetDataPolarity
 * @param  UART0_5x UART0_5 Instance
 * @param  DataPolarity This parameter can be one of the following values:
 *         @arg @ref LL_UART0_5_DATA_POLARITY_HIGH
 *         @arg @ref LL_UART0_5_DATA_POLARITY_LOW
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_SetDataPolarity(UART0_5_TypeDef *UART0_5x, uint32_t DataPolarity)
{
    MODIFY_REG(UART0_5x->CTRL, UART0_5_CTRL_NEG, DataPolarity);
}

/**
 * @brief  Return the polarity of the trasmit and receive data
 * @rmtoll CTRL          NEG          LL_UART0_5_GetDataPolarity
 * @param  UART0_5x UART0_5 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART0_5_DATA_POLARITY_HIGH
 *         @arg @ref LL_UART0_5_DATA_POLARITY_LOW
 */
__STATIC_INLINE uint32_t LL_UART0_5_GetDataPolarity(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->CTRL, UART0_5_CTRL_NEG));
}

/**
 * @brief  Memory storage configuration on parity error (just use for uart0_5)
 * @note   When parity error occurs, the current frame store or not store in memory
 * @rmtoll CTRL          PAR_MEM          LL_UART0_5_SetParityErrorHandling
 * @param  UART0_5x UART0_5 Instance (just use uart0_5 Instance)
 * @param  ErrorHandling This parameter can be one of the following values:
 *         @arg @ref LL_UART0_5_PARMEM_STORE_MEMORY
 *         @arg @ref LL_UART0_5_PARMEM_NOT_STORE_MEMORY
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_SetParityErrorHandling(UART0_5_TypeDef *UART0_5x, uint32_t ErrorHandling)
{
    MODIFY_REG(UART0_5x->CTRL, UART0_5_CTRL_PAR_MEM, ErrorHandling);
}

/**
 * @brief  Return the Memory storage configuration on parity error (just use for uart0_5)
 * @note   When parity error occurs, the current frame store or not store in memory
 * @rmtoll CTRL          PAR_MEM          LL_UART0_5_GetParityErrorHandling
 * @param  UART0_5x UART0_5 Instance (just use uart0_5 Instance)
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART0_5_PARMEM_STORE_MEMORY
 *         @arg @ref LL_UART0_5_PARMEM_NOT_STORE_MEMORY
 */
__STATIC_INLINE uint32_t LL_UART0_5_GetParityErrorHandling(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->CTRL, UART0_5_CTRL_PAR_MEM));
}

/**
 * @brief  Configure UART0_5 BAUD and FDIV register for achieving expected Baud Rate value.
 * @note   Compute and set UART0_5DIV value in BAUD and FDIV register
 *         according to used Peripheral Clock, and expected Baud Rate values
 * @note   Peripheral clock and Baud rate values provided as function parameters should be valid
 * @rmtoll BAUD          CLKDIV           LL_UART0_5_SetBaudRate\n
 *         FDIV          FDIV             LL_UART0_5_SetBaudRate
 * @param  UART0_5x UART0_5 Instance
 * @param  PeriphClk Peripheral Clock
 * @param  BaudRate Baud Rate
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_SetBaudRate(UART0_5_TypeDef *UART0_5x, uint32_t PeriphClk, uint32_t BaudRate)
{
    UART0_5x->BAUD = (uint16_t)(__LL_UART0_5_CALC_BAUDRATE_DIVMANT(PeriphClk, BaudRate));
    UART0_5x->FDIV = (uint16_t)(__LL_UART0_5_CALC_BAUDRATE_DIVFRAC(PeriphClk, BaudRate));
}

/**
 * @brief  Return current Baud Rate value, according to UART0_5 DIV present in BAUD and FDIV register,
 *         and to used Peripheral Clock values
 * @rmtoll BAUD          CLKDIV           LL_UART0_5_GetBaudRate\n
 *         FDIV          FDIV             LL_UART0_5_GetBaudRate
 * @param  UART0_5x UART0_5 Instance
 * @param  PeriphClk Peripheral Clock
 * @retval Baud Rate
 */
__STATIC_INLINE uint32_t LL_UART0_5_GetBaudRate(const UART0_5_TypeDef *UART0_5x, uint32_t PeriphClk)
{
    uint32_t usartdiv = 0x0U;
    uint32_t baudresult = 0x0U;

    usartdiv = ((UART0_5x->BAUD + 1U) << 14) + UART0_5x->FDIV;
    baudresult = (((uint64_t)PeriphClk << 14) + ((usartdiv << 3) >> 4)) / (usartdiv << 4);
    return (baudresult);
}

/**
 * @brief  Receive Timeout Configuration, if set 0 is invalid
 * @rmtoll RX_TO          TIMEOUT_CNT          LL_UART0_5_DMA_SetTimeoutCount
 * @param  UART0_5x UART0_5 Instance
 * @param  TimeoutCount between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_SetTimeoutCount(UART0_5_TypeDef *UART0_5x, uint32_t TimeoutCount)
{
    MODIFY_REG(UART0_5x->RX_TO, UART0_5_RX_TO_TIMEOUT_CNT, TimeoutCount << UART0_5_RX_TO_TIMEOUT_CNT_POS);
}

/**
 * @brief  Return the Receive Timeout Configuration
 * @rmtoll RX_TO          TIMEOUT_CNT          LL_UART0_5_DMA_GetTimeoutCount
 * @param  UART0_5x UART0_5 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF
 */
__STATIC_INLINE uint32_t LL_UART0_5_GetTimeoutCount(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->RX_TO, UART0_5_RX_TO_TIMEOUT_CNT) >> UART0_5_RX_TO_TIMEOUT_CNT_POS);
}


/**
 * @brief  Delay Insertion configuration (just use for uart0_5)
 * @note   The delay is inserted between data frames. When TX_DELAY_EN = 1 is valid.
 * @rmtoll TX_DRLAY          TIMEOUT_CNT          LL_UART0_5_SetTxDelayTime
 * @param  UART0_5x UART0_5 Instance (just use uart0_5 Instance)
 * @param  TxDelayTime This parameter can be one of the following values:
 *         @arg @ref LL_UART0_5_DRLAY_TIMEOUT_CNT_1B
 *         @arg @ref LL_UART0_5_DRLAY_TIMEOUT_CNT_2B
 *         @arg @ref LL_UART0_5_DRLAY_TIMEOUT_CNT_3B
 *         @arg @ref LL_UART0_5_DRLAY_TIMEOUT_CNT_4B
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_SetTxDelayTime(UART0_5_TypeDef *UART0_5x, uint32_t TxDelayTime)
{
    MODIFY_REG(UART0_5x->TX_DRLAY, UART0_5_TX_DRLAY_TIMEOUT_CNT, TxDelayTime);
}

/**
 * @brief  Return the Delay Insertion configuration (just use for uart0_5)
 * @note   The delay is inserted between data frames. When TX_DELAY_EN = 1 is valid.
 * @rmtoll TX_DRLAY          TIMEOUT_CNT          LL_UART0_5_GetTxDelayTime
 * @param  UART0_5x UART0_5 Instance (just use uart0_5 Instance)
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART0_5_DRLAY_TIMEOUT_CNT_1B
 *         @arg @ref LL_UART0_5_DRLAY_TIMEOUT_CNT_2B
 *         @arg @ref LL_UART0_5_DRLAY_TIMEOUT_CNT_3B
 *         @arg @ref LL_UART0_5_DRLAY_TIMEOUT_CNT_4B
 */
__STATIC_INLINE uint32_t LL_UART0_5_GetTxDelayTime(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->TX_DRLAY, UART0_5_TX_DRLAY_TIMEOUT_CNT));
}

/**
 * @brief  Enable Transmit Delay (just use for uart0_5)
 * @rmtoll TX_DRLAY          TX_DELAY_EN          LL_UART0_5_EnableTxDelay
 * @param  UART0_5x UART0_5 Instance (just use uart0_5)
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_EnableTxDelay(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->TX_DRLAY, UART0_5_TX_DRLAY_TX_DELAY_EN);
}

/**
 * @brief  Disable Transmit Delay (just use for uart0_5)
 * @rmtoll TX_DRLAY          TX_DELAY_EN          LL_UART0_5_DisableTxDelay
 * @param  UART0_5x UART0_5 Instance (just use uart0_5)
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DisableTxDelay(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->TX_DRLAY, UART0_5_TX_DRLAY_TX_DELAY_EN);
}

/**
 * @brief  Indicate if Transmit Delay is enabled (just use for uart0_5)
 * @rmtoll TX_DRLAY          TX_DELAY_EN          LL_UART0_5_IsEnabledTxDelay
 * @param  UART0_5x UART0_5 Instance (just use uart0_5)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsEnabledTxDelay(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->TX_DRLAY, UART0_5_TX_DRLAY_TX_DELAY_EN) == (UART0_5_TX_DRLAY_TX_DELAY_EN));
}

/**
 * @}
 */

/** @defgroup UART0_5_LL_EF_Configuration_IRDA Configuration functions related to Irda feature
 * @{
 */

/**
 * @brief  Enable IrDA mode
 * @rmtoll CTRL          IRE          LL_UART0_5_EnableIrda
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_EnableIrda(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->CTRL, UART0_5_CTRL_IRE);
}

/**
 * @brief  Disable IrDA mode
 * @rmtoll CTRL          IRE          LL_UART0_5_DisableIrda
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DisableIrda(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->CTRL, UART0_5_CTRL_IRE);
}

/**
 * @brief  Indicate if IrDA mode is enabled
 * @rmtoll CTRL          IRE          LL_UART0_5_IsEnabledIrda
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsEnabledIrda(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->CTRL, UART0_5_CTRL_IRE) == (UART0_5_CTRL_IRE));
}

/**
 * @brief  Configure IrDA polarity (Positive or Negative)
 * @rmtoll CTRL          IRSEL          LL_UART0_5_SetIrdaPolarity
 * @param  UART0_5x UART0_5 Instance
 * @param  IrdaPolarity This parameter can be one of the following values:
 *         @arg @ref LL_UART0_5_IRDA_POLARITY_HIGH
 *         @arg @ref LL_UART0_5_IRDA_POLARITY_LOW
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_SetIrdaPolarity(UART0_5_TypeDef *UART0_5x, uint32_t IrdaPolarity)
{
    MODIFY_REG(UART0_5x->CTRL, UART0_5_CTRL_IRSEL, IrdaPolarity);
}

/**
 * @brief  Retrieve IrDA polarity (Positive or Negative)
 * @rmtoll CTRL          IRSEL          LL_UART0_5_GetIrdaPolarity
 * @param  UART0_5x UART0_5 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART0_5_IRDA_POLARITY_HIGH
 *         @arg @ref LL_UART0_5_IRDA_POLARITY_LOW
 */
__STATIC_INLINE uint32_t LL_UART0_5_GetIrdaPolarity(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->CTRL, UART0_5_CTRL_IRSEL));
}

/**
 * @}
 */

/** @defgroup UART0_5_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable Transmit Data Interrupt
 * @rmtoll CTRL          TXIE        LL_UART0_5_EnableIT_TX
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_EnableIT_TX(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->CTRL, UART0_5_CTRL_TXIE);
}

/**
 * @brief  Disable Transmit Data Interrupt
 * @rmtoll CTRL          TXIE        LL_UART0_5_DisableIT_TX
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DisableIT_TX(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->CTRL, UART0_5_CTRL_TXIE);
}

/**
 * @brief  Check if the UART0_5 Transmit Data Interrupt is enabled or disabled.
 * @rmtoll CTRL          TXIE        LL_UART0_5_IsEnabledIT_TX
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsEnabledIT_TX(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->CTRL, UART0_5_CTRL_TXIE) == (UART0_5_CTRL_TXIE));
}

/**
 * @brief  Enable  Receive Data Interrupt
 * @rmtoll CTRL          RXIE        LL_UART0_5_EnableIT_RX
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_EnableIT_RX(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->CTRL, UART0_5_CTRL_RXIE);
}

/**
 * @brief  Disable  Receive Data Interrupt
 * @rmtoll CTRL          RXIE        LL_UART0_5_DisableIT_RX
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DisableIT_RX(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->CTRL, UART0_5_CTRL_RXIE);
}

/**
 * @brief  Check if the UART0_5  Receive Data Interrupt is enabled or disabled.
 * @rmtoll CTRL          RXIE        LL_UART0_5_IsEnabledIT_RX
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsEnabledIT_RX(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->CTRL, UART0_5_CTRL_RXIE) == (UART0_5_CTRL_RXIE));
}

/**
 * @brief  Enable Error Interrupt
 * @rmtoll CTRL          ERRIE        LL_UART0_5_EnableIT_ERR
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_EnableIT_ERR(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->CTRL, UART0_5_CTRL_ERRIE);
}

/**
 * @brief  Disable Error Interrupt
 * @rmtoll CTRL          ERRIE        LL_UART0_5_DisableIT_ERR
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DisableIT_ERR(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->CTRL, UART0_5_CTRL_ERRIE);
}

/**
 * @brief  Check if the UART0_5 Error Interrupt is enabled or disabled.
 * @rmtoll CTRL          ERRIE        LL_UART0_5_IsEnabledIT_ERR
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsEnabledIT_ERR(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->CTRL, UART0_5_CTRL_ERRIE) == (UART0_5_CTRL_ERRIE));
}

/**
 * @brief  Enable Receive Timeout Interrupt (just use for uart0_5)
 * @rmtoll CTRL          RX_TO_IE        LL_UART0_5_EnableIT_RXTO
 * @param  UART0_5x UART0_5 Instance (just use uart0_5)
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_EnableIT_RXTO(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->CTRL, UART0_5_CTRL_RX_TO_IE);
}

/**
 * @brief  Disable Receive Timeout Interrupt (just use for uart0_5)
 * @rmtoll CTRL          RX_TO_IE        LL_UART0_5_DisableIT_RXTO
 * @param  UART0_5x UART0_5 Instance (just use uart0_5)
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DisableIT_RXTO(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->CTRL, UART0_5_CTRL_RX_TO_IE);
}

/**
 * @brief  Check if the UART0_5 Receive Timeout Interrupt is enabled or disabled (just use for uart0_5)
 * @rmtoll CTRL          RX_TO_IE        LL_UART0_5_IsEnabledIT_RXTO
 * @param  UART0_5x UART0_5 Instance (just use uart0_5)
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsEnabledIT_RXTO(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->CTRL, UART0_5_CTRL_RX_TO_IE) == (UART0_5_CTRL_RX_TO_IE));
}

/**
 * @}
 */

/** @defgroup UART0_5_LL_EF_FLAG_management FLAG_management
 * @{
 */

/**
 * @brief  Check if the UART0_5 Receive Complete Flag is set or not
 * @rmtoll STA           RX            LL_UART0_5_IsActiveFlag_RX
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsActiveFlag_RX(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->STA, UART0_5_STA_RX) == (UART0_5_STA_RX));
}

/**
 * @brief  Clear Receive Complete Flag
 * @rmtoll STA           RX            LL_UART0_5_ClearFlag_RX
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_ClearFlag_RX(UART0_5_TypeDef *UART0_5x)
{
    WRITE_REG(UART0_5x->STA, UART0_5_STA_RX);
}

/**
 * @brief  Check if the UART0_5 Transmit Complete Flag is set or not
 * @rmtoll STA           TX            LL_UART0_5_IsActiveFlag_TX
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsActiveFlag_TX(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->STA, UART0_5_STA_TX) == (UART0_5_STA_TX));
}

/**
 * @brief  Clear Transmit Complete Flag
 * @rmtoll STA           TX            LL_UART0_5_ClearFlag_TX
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_ClearFlag_TX(UART0_5_TypeDef *UART0_5x)
{
    WRITE_REG(UART0_5x->STA, UART0_5_STA_TX);
}

/**
 * @brief  Check if the UART0_5 Parity Error Flag is set or not
 * @rmtoll STA           PE            LL_UART0_5_IsActiveFlag_PE
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsActiveFlag_PE(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->STA, UART0_5_STA_PE) == (UART0_5_STA_PE));
}

/**
 * @brief  Clear Parity Error Flag
 * @rmtoll STA           PE            LL_UART0_5_ClearFlag_PE
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_ClearFlag_PE(UART0_5_TypeDef *UART0_5x)
{
    WRITE_REG(UART0_5x->STA, UART0_5_STA_PE);
}

/**
 * @brief  Check if the UART0_5 Overrun Error Flag is set or not
 * @rmtoll STA           OE            LL_UART0_5_IsActiveFlag_OE
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsActiveFlag_OE(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->STA, UART0_5_STA_OE) == (UART0_5_STA_OE));
}

/**
 * @brief  Clear Overrun Error Flag
 * @rmtoll STA           OE            LL_UART0_5_ClearFlag_OE
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_ClearFlag_OE(UART0_5_TypeDef *UART0_5x)
{
    WRITE_REG(UART0_5x->STA, UART0_5_STA_OE);
}

/**
 * @brief  Check if the UART0_5 Framing Error Flag is set or not
 * @rmtoll STA           FE            LL_UART0_5_IsActiveFlag_FE
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsActiveFlag_FE(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->STA, UART0_5_STA_FE) == (UART0_5_STA_FE));
}

/**
 * @brief  Clear Framing Error Flag
 * @rmtoll STA           FE            LL_UART0_5_ClearFlag_FE
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_ClearFlag_FE(UART0_5_TypeDef *UART0_5x)
{
    WRITE_REG(UART0_5x->STA, UART0_5_STA_FE);
}

/**
 * @brief  Check if the UART0_5 Data Error Flag is set or not
 * @rmtoll STA           DE            LL_UART0_5_IsActiveFlag_DE
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsActiveFlag_DE(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->STA, UART0_5_STA_DE) == (UART0_5_STA_DE));
}

/**
 * @brief  Clear Data Error Flag
 * @rmtoll STA           DE            LL_UART0_5_ClearFlag_DE
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_ClearFlag_DE(UART0_5_TypeDef *UART0_5x)
{
    WRITE_REG(UART0_5x->STA, UART0_5_STA_DE);
}

/**
 * @brief  Check if the UART0_5 Receiver Busy Flag is set or not (read only)
 * @rmtoll STA           RB            LL_UART0_5_IsActiveFlag_RB
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsActiveFlag_RB(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->STA, UART0_5_STA_RB) == (UART0_5_STA_RB));
}

/**
 * @brief  Check if the UART0_5 Transmiter Busy Flag is set or not (read only)
 * @rmtoll STA           TB            LL_UART0_5_IsActiveFlag_TB
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsActiveFlag_TB(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->STA, UART0_5_STA_TB) == (UART0_5_STA_TB));
}

/**
 * @brief  Check if the UART0_5 Transmit FIFO Empty Flag is set or not (read only)
 * @rmtoll STA           TX_FIFO_EMPTY            LL_UART0_5_IsActiveFlag_TXFIFOEMPTY
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsActiveFlag_TXFIFOEMPTY(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->STA, UART0_5_STA_TX_FIFO_EMPTY) == (UART0_5_STA_TX_FIFO_EMPTY));
}

/**
 * @brief  Check if the UART0_5 Transmit FIFO Full Flag is set or not (read only)
 * @rmtoll STA           TX_FIFO_FULL            LL_UART0_5_IsActiveFlag_TXFIFOFULL
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsActiveFlag_TXFIFOFULL(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->STA, UART0_5_STA_TX_FIFO_FULL) == (UART0_5_STA_TX_FIFO_FULL));
}

/**
 * @brief  Check if the UART0_5 Receive Timeout Flag is set or not
 * @rmtoll STA           RX_TO            LL_UART0_5_IsActiveFlag_RXTO
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsActiveFlag_RXTO(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->STA, UART0_5_STA_RX_TO) == (UART0_5_STA_RX_TO));
}

/**
 * @brief  Clear Receive Timeout Flag
 * @rmtoll STA           RXTO            LL_UART0_5_ClearFlag_RXTO
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_ClearFlag_RXTO(UART0_5_TypeDef *UART0_5x)
{
    WRITE_REG(UART0_5x->STA, UART0_5_STA_RX_TO);
}

/**
 * @}
 */

/** @defgroup UART0_5_LL_EF_DMA_Management DMA_Management
 * @{
 */

/**
 * @brief  Enable DMA Mode for transmission
 * @rmtoll DMA_CTRL          TX_DMA_EN          LL_UART0_5_EnableDMAReq_TX
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_EnableDMAReq_TX(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->DMA_CTRL, UART0_5_DMA_CTRL_TX_DMA_EN);
}

/**
 * @brief  Disable DMA Mode for transmission
 * @rmtoll DMA_CTRL          TX_DMA_EN          LL_UART0_5_DisableDMAReq_TX
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DisableDMAReq_TX(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->DMA_CTRL, UART0_5_DMA_CTRL_TX_DMA_EN);
}

/**
 * @brief  Check if DMA Mode is enabled for transmission
 * @rmtoll DMA_CTRL          TX_DMA_EN          LL_UART0_5_IsEnabledDMAReq_TX
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsEnabledDMAReq_TX(const UART0_5_TypeDef *UART0_5x)
{
  return (READ_BIT(UART0_5x->DMA_CTRL, UART0_5_DMA_CTRL_TX_DMA_EN) == (UART0_5_DMA_CTRL_TX_DMA_EN));
}

/**
 * @brief  Enable DMA Mode for reception
 * @rmtoll DMA_CTRL          RX_DMA_EN          LL_UART0_5_EnableDMAReq_RX
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_EnableDMAReq_RX(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->DMA_CTRL, UART0_5_DMA_CTRL_RX_DMA_EN);
}

/**
 * @brief  Disable DMA Mode for reception
 * @rmtoll DMA_CTRL          RX_DMA_EN          LL_UART0_5_DisableDMAReq_RX
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DisableDMAReq_RX(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->DMA_CTRL, UART0_5_DMA_CTRL_RX_DMA_EN);
}

/**
 * @brief  Check if DMA Mode is enabled for reception
 * @rmtoll DMA_CTRL          RX_DMA_EN          LL_UART0_5_IsEnabledDMAReq_RX
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_IsEnabledDMAReq_RX(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_CTRL, UART0_5_DMA_CTRL_RX_DMA_EN) == (UART0_5_DMA_CTRL_RX_DMA_EN));
}

/**
 * @brief  Enable DMA Transmit Cycle Mode
 * @rmtoll DMA_CTRL          TX_CYC_MODE        LL_UART0_5_DMA_EnableTxCircularMode
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_EnableTxCircularMode(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->DMA_CTRL, UART0_5_DMA_CTRL_TX_CYC_MODE);
}

/**
 * @brief  Disable DMA Transmit Cycle Mode
 * @rmtoll DMA_CTRL          TX_CYC_MODE        LL_UART0_5_DMA_DisableTxCircularMode
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_DisableTxCircularMode(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->DMA_CTRL, UART0_5_DMA_CTRL_TX_CYC_MODE);
}

/**
 * @brief  Check if the UART0_5 DMA Transmit Cycle Mode is enabled or disabled.
 * @rmtoll DMA_CTRL          TX_CYC_MODE        LL_UART0_5_DMA_IsEnabledTxCircularMode
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsEnabledTxCircularMode(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_CTRL, UART0_5_DMA_CTRL_TX_CYC_MODE) == (UART0_5_DMA_CTRL_TX_CYC_MODE));
}

/**
 * @brief  Enable DMA Receive Cycle Mode
 * @rmtoll DMA_CTRL          RX_CYC_MODE        LL_UART0_5_DMA_EnableRxCircularMode
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_EnableRxCircularMode(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->DMA_CTRL, UART0_5_DMA_CTRL_RX_CYC_MODE);
}

/**
 * @brief  Disable DMA Receive Cycle Mode
 * @rmtoll DMA_CTRL          RX_CYC_MODE        LL_UART0_5_DMA_DisableRxCircularMode
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_DisableRxCircularMode(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->DMA_CTRL, UART0_5_DMA_CTRL_RX_CYC_MODE);
}

/**
 * @brief  Check if the UART0_5 DMA Receive Cycle Mode is enabled or disabled.
 * @rmtoll DMA_CTRL          RX_CYC_MODE        LL_UART0_5_DMA_IsEnabledRxCircularMode
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsEnabledRxCircularMode(const UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_CTRL, UART0_5_DMA_CTRL_RX_CYC_MODE) == (UART0_5_DMA_CTRL_RX_CYC_MODE));
}

/**
 * @brief  Configure DMA Parity.
 * @rmtoll DMA_CTRL          DMA_PASS            LL_UART0_5_DMA_SetParity
 * @param  UART0_5x UART0_5 Instance
 * @param  Parity This parameter can be one of the following values:
 *         @arg @ref LL_UART0_5_DMA_PARITY_ZERO
 *         @arg @ref LL_UART0_5_DMA_PARITY_ONE
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_SetParity(UART0_5_TypeDef *UART0_5x, uint32_t Parity)
{
    MODIFY_REG(UART0_5x->DMA_CTRL, UART0_5_DMA_CTRL_DMA_PASS, Parity);
}

/**
 * @brief  Return DMA Parity configuration
 * @rmtoll DMA_CTRL          DMA_PASS            LL_UART0_5_DMA_GetParity
 * @param  UART0_5x UART0_5 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_UART0_5_DMA_PARITY_ZERO
 *         @arg @ref LL_UART0_5_DMA_PARITY_ONE
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_GetParity(UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->DMA_CTRL, UART0_5_DMA_CTRL_DMA_PASS));
}

/**
 * @brief  Configure DMA Transmit Base Address (byte address)
 * @rmtoll DMA_TBADR          DMA_TBADR            LL_UART0_5_DMA_SetTxStartAddress
 * @param  UART0_5x UART0_5 Instance
 * @param  TxStartAddress between Min_Data=0x00 and Max_Data=0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_SetTxStartAddress(UART0_5_TypeDef *UART0_5x, uint32_t TxStartAddress)
{
    MODIFY_REG(UART0_5x->DMA_TBADR, UART0_5_DMA_TBADR_DMA_TBADR, TxStartAddress << UART0_5_DMA_TBADR_DMA_TBADR_POS);
}

/**
 * @brief  Return DMA Transmit Base Address (byte address) configuration
 * @rmtoll DMA_TBADR          DMA_TBADR            LL_UART0_5_DMA_GetTxStartAddress
 * @param  UART0_5x UART0_5 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_GetTxStartAddress(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->DMA_TBADR, UART0_5_DMA_TBADR_DMA_TBADR) >> UART0_5_DMA_TBADR_DMA_TBADR_POS);
}

/**
 * @brief  Configure DMA Receive Base Address (byte address)
 * @rmtoll DMA_RBADR          DMA_RBADR            LL_UART0_5_DMA_SetRxStartAddress
 * @param  UART0_5x UART0_5 Instance
 * @param  RxStartAddress between Min_Data=0x00 and Max_Data=0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_SetRxStartAddress(UART0_5_TypeDef *UART0_5x, uint32_t RxStartAddress)
{
    MODIFY_REG(UART0_5x->DMA_RBADR, UART0_5_DMA_RBADR_DMA_RBADR, RxStartAddress << UART0_5_DMA_RBADR_DMA_RBADR_POS);
}

/**
 * @brief  Return DMA Receive Base Address (byte address) configuration
 * @rmtoll DMA_RBADR          DMA_RBADR            LL_UART0_5_DMA_GetRxStartAddress
 * @param  UART0_5x UART0_5 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_GetRxStartAddress(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->DMA_RBADR, UART0_5_DMA_RBADR_DMA_RBADR) >> UART0_5_DMA_RBADR_DMA_RBADR_POS);
}

/**
 * @brief  Configure DMA Transmit Length (in bytes)
 * @rmtoll DMA_TLEN          DMA_TLEN            LL_UART0_5_DMA_SetTxLength
 * @param  UART0_5x UART0_5 Instance
 * @param  TxLength between Min_Data=0x00 and Max_Data=0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_SetTxLength(UART0_5_TypeDef *UART0_5x, uint32_t TxLength)
{
    MODIFY_REG(UART0_5x->DMA_TLEN, UART0_5_DMA_TLEN_DMA_TLEN, TxLength << UART0_5_DMA_TLEN_DMA_TLEN_POS);
}

/**
 * @brief  Return DMA Transmit Length (in bytes) configuration
 * @rmtoll DMA_TLEN          DMA_TLEN            LL_UART0_5_DMA_GetTxLength
 * @param  UART0_5x UART0_5 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_GetTxLength(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->DMA_TLEN, UART0_5_DMA_TLEN_DMA_TLEN) >> UART0_5_DMA_TLEN_DMA_TLEN_POS);
}

/**
 * @brief  Configure DMA Receive Length (in bytes)
 * @rmtoll DMA_RLEN          DMA_RLEN            LL_UART0_5_DMA_SetRxLength
 * @param  UART0_5x UART0_5 Instance
 * @param  RxLength between Min_Data=0x00 and Max_Data=0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_SetRxLength(UART0_5_TypeDef * UART0_5x, uint32_t RxLength)
{
    MODIFY_REG(UART0_5x->DMA_RLEN, UART0_5_DMA_RLEN_DMA_RLEN, RxLength << UART0_5_DMA_RLEN_DMA_RLEN_POS);
}

/**
 * @brief  Return DMA Receive Length (in bytes) configuration
 * @rmtoll DMA_RLEN          DMA_RLEN            LL_UART0_5_DMA_GetRxLength
 * @param  UART0_5x UART0_5 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_GetRxLength(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->DMA_RLEN, UART0_5_DMA_RLEN_DMA_RLEN) >> UART0_5_DMA_RLEN_DMA_RLEN_POS);
}

/**
 * @brief  Return Current DMA Transmit Address (byte address) configuration (read only)
 * @rmtoll DMA_TADR          DMA_TADR            LL_UART0_5_DMA_GetTxAddress
 * @param  UART0_5x UART0_5 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_GetTxAddress(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->DMA_TADR, UART0_5_DMA_TADR_DMA_TADR) >> UART0_5_DMA_TADR_DMA_TADR_POS);
}

/**
 * @brief  Return Current DMA Receive Address (byte address) configuration (read only)
 * @rmtoll DMA_RADR          DMA_RADR            LL_UART0_5_DMA_GetRxAddress
 * @param  UART0_5x UART0_5 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_GetRxAddress(const UART0_5_TypeDef *UART0_5x)
{
    return (uint32_t)(READ_BIT(UART0_5x->DMA_RADR, UART0_5_DMA_TADR_DMA_TADR) >> UART0_5_DMA_RADR_DMA_RADR_POS);
}

/**
 * @}
 */

/** @defgroup UART0_5_LL_EF_DMA_IT_Management DMA_IT_Management
 * @{
 */

/**
 * @brief  Enable DMA Transmit Half Empty Interrupt
 * @rmtoll DMA_IE          TX_HIE        LL_UART0_5_DMA_EnableIT_TXHALF
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_EnableIT_TXHALF(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_TX_HIE);
}

/**
 * @brief  Disable DMA Transmit Half Empty Interrupt
 * @rmtoll DMA_IE          TX_HIE        LL_UART0_5_DMA_DisableIT_TXHALF
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_DisableIT_TXHALF(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_TX_HIE);
}

/**
 * @brief  Check if the UART0_5 DMA Transmit Half Empty Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          TX_HIE        LL_UART0_5_DMA_IsEnabledIT_TXHALF
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsEnabledIT_TXHALF(UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_TX_HIE) == (UART0_5_DMA_IE_TX_HIE));
}

/**
 * @brief  Enable DMA Transmit Empty (Full) Interrupt
 * @rmtoll DMA_IE          TX_FIE        LL_UART0_5_DMA_EnableIT_TXFULL
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_EnableIT_TXFULL(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_TX_FIE);
}

/**
 * @brief  Disable DMA Transmit Empty (Full) Interrupt
 * @rmtoll DMA_IE          TX_FIE        LL_UART0_5_DMA_DisableIT_TXFULL
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_DisableIT_TXFULL(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_TX_FIE);
}

/**
 * @brief  Check if the UART0_5 DMA Transmit Empty (Full) Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          TX_FIE        LL_UART0_5_DMA_IsEnabledIT_TXFULL
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsEnabledIT_TXFULL(UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_TX_FIE) == (UART0_5_DMA_IE_TX_FIE));
}

/**
 * @brief  Enable DMA Receive Half Full Interrupt
 * @rmtoll DMA_IE          RX_HIE        LL_UART0_5_DMA_EnableIT_RXHALF
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_EnableIT_RXHALF(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_RX_HIE);
}

/**
 * @brief  Disable DMA Receive Half Full Interrupt
 * @rmtoll DMA_IE          RX_HIE        LL_UART0_5_DMA_DisableIT_RXHALF
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_DisableIT_RXHALF(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_RX_HIE);
}

/**
 * @brief  Check if the UART0_5 DMA Receive Half Full Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          RX_HIE        LL_UART0_5_DMA_IsEnabledIT_RXHALF
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsEnabledIT_RXHALF(UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_RX_HIE) == (UART0_5_DMA_IE_RX_HIE));
}

/**
 * @brief  Enable DMA Receive Full Interrupt
 * @rmtoll DMA_IE          RX_FIE        LL_UART0_5_DMA_EnableIT_RXFULL
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_EnableIT_RXFULL(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_RX_FIE);
}

/**
 * @brief  Disable DMA Receive Full Interrupt
 * @rmtoll DMA_IE          RX_FIE        LL_UART0_5_DMA_DisableIT_RXFULL
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_DisableIT_RXFULL(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_RX_FIE);
}

/**
 * @brief  Check if the UART0_5 DMA Receive Full Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          RX_FIE        LL_UART0_5_DMA_IsEnabledIT_RXFULL
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsEnabledIT_RXFULL(UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_RX_FIE) == (UART0_5_DMA_IE_RX_FIE));
}

/**
 * @brief  Enable DMA Transmit Data Error Interrupt
 * @rmtoll DMA_IE          TX_ERR_IE        LL_UART0_5_DMA_EnableIT_TXERR
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_EnableIT_TXERR(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_TX_ERR_IE);
}

/**
 * @brief  Disable DMA Transmit Data Error Interrupt
 * @rmtoll DMA_IE          TX_ERR_IE        LL_UART0_5_DMA_DisableIT_TXERR
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_DisableIT_TXERR(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_TX_ERR_IE);
}

/**
 * @brief  Check if the UART0_5 DMA Transmit Data Error Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          TX_ERR_IE        LL_UART0_5_DMA_IsEnabledIT_TXERR
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsEnabledIT_TXERR(UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_TX_ERR_IE) == (UART0_5_DMA_IE_TX_ERR_IE));
}

/**
 * @brief  Enable DMA Receive Data Error Interrupt
 * @rmtoll DMA_IE          RX_ERR_IE        LL_UART0_5_DMA_EnableIT_RXERR
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_EnableIT_RXERR(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_RX_ERR_IE);
}

/**
 * @brief  Disable DMA Receive Data Error Interrupt
 * @rmtoll DMA_IE          RX_ERR_IE        LL_UART0_5_DMA_DisableIT_RXERR
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_DisableIT_RXERR(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_RX_ERR_IE);
}

/**
 * @brief  Check if the UART0_5 DMA Receive Data Error Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          RX_ERR_IE        LL_UART0_5_DMA_IsEnabledIT_RXERR
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsEnabledIT_RXERR(UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_RX_ERR_IE) == (UART0_5_DMA_IE_RX_ERR_IE));
}

/**
 * @brief  Enable DMA Receive Complete Interrupt
 * @rmtoll DMA_IE          RX_DONE_IE        LL_UART0_5_DMA_EnableIT_RXTC
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_EnableIT_RXTC(UART0_5_TypeDef *UART0_5x)
{
    SET_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_RX_DONE_IE);
}

/**
 * @brief  Disable DMA Receive Complete Interrupt
 * @rmtoll DMA_IE          RX_DONE_IE        LL_UART0_5_DMA_DisableIT_RXTC
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_DisableIT_RXTC(UART0_5_TypeDef *UART0_5x)
{
    CLEAR_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_RX_DONE_IE);
}

/**
 * @brief  Check if the UART0_5 DMA Receive Complete Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          RX_DONE_IE        LL_UART0_5_DMA_IsEnabledIT_RXTC
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsEnabledIT_RXTC(UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_IE, UART0_5_DMA_IE_RX_DONE_IE) == (UART0_5_DMA_IE_RX_DONE_IE));
}

/**
 * @}
 */

/** @defgroup UART0_5_LL_EF_DMA_FLAG_management DMA_FLAG_management
 * @{
 */

/**
 * @brief  Check if the UART0_5 DMA Transmit Half Empty Interrupt Flag is set or not
 * @rmtoll DMA_IF           TX_HDONE            LL_UART0_5_DMA_IsActiveFlag_TXHALF
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsActiveFlag_TXHALF(UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_IF, UART0_5_DMA_IF_TX_HDONE) == (UART0_5_DMA_IF_TX_HDONE));
}

/**
 * @brief  Clear DMA Transmit Half Empty Interrupt Flag
 * @rmtoll DMA_IF           TX_HDONE            LL_UART0_5_DMA_ClearFlag_TXHALF
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_ClearFlag_TXHALF(UART0_5_TypeDef *UART0_5x)
{
    WRITE_REG(UART0_5x->DMA_IF, UART0_5_DMA_IF_TX_HDONE);
}

/**
 * @brief  Check if the UART0_5 DMA Transmit Empty (Full) Interrupt Flag is set or not
 * @rmtoll DMA_IF           TX_FDONE            LL_UART0_5_DMA_IsActiveFlag_TXFULL
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsActiveFlag_TXFULL(UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_IF, UART0_5_DMA_IF_TX_FDONE) == (UART0_5_DMA_IF_TX_FDONE));
}

/**
 * @brief  Clear DMA Transmit Empty (Full) Interrupt Flag
 * @rmtoll DMA_IF           TX_FDONE            LL_UART0_5_DMA_ClearFlag_TXFULL
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_ClearFlag_TXFULL(UART0_5_TypeDef *UART0_5x)
{
    WRITE_REG(UART0_5x->DMA_IF, UART0_5_DMA_IF_TX_FDONE);
}

/**
 * @brief  Check if the UART0_5 DMA Receive Half Full Interrupt Flag is set or not
 * @rmtoll DMA_IF           RX_HDONE            LL_UART0_5_DMA_IsActiveFlag_RXHALF
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsActiveFlag_RXHALF(UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_IF, UART0_5_DMA_IF_RX_HDONE) == (UART0_5_DMA_IF_RX_HDONE));
}

/**
 * @brief  Clear DMA Receive Half Full Interrupt Flag
 * @rmtoll DMA_IF           RX_HDONE            LL_UART0_5_DMA_ClearFlag_RXHALF
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_ClearFlag_RXHALF(UART0_5_TypeDef *UART0_5x)
{
    WRITE_REG(UART0_5x->DMA_IF, UART0_5_DMA_IF_RX_HDONE);
}

/**
 * @brief  Check if the UART0_5 DMA Receive Half Full Interrupt Flag is set or not
 * @rmtoll DMA_IF           RX_FDONE            LL_UART0_5_DMA_IsActiveFlag_RXFULL
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsActiveFlag_RXFULL(UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_IF, UART0_5_DMA_IF_RX_FDONE) == (UART0_5_DMA_IF_RX_FDONE));
}

/**
 * @brief  Clear DMA Receive Half Full Interrupt Flag
 * @rmtoll DMA_IF           RX_FDONE            LL_UART0_5_DMA_ClearFlag_RXFULL
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_ClearFlag_RXFULL(UART0_5_TypeDef *UART0_5x)
{
    WRITE_REG(UART0_5x->DMA_IF, UART0_5_DMA_IF_RX_FDONE);
}

/**
 * @brief  Check if the UART0_5 DMA Receive Data Error Flag is set or not
 * @rmtoll DMA_IF           RX_ERR            LL_UART0_5_DMA_IsActiveFlag_RXERR
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsActiveFlag_RXERR(UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_IF, UART0_5_DMA_IF_RX_ERR) == (UART0_5_DMA_IF_RX_ERR));
}

/**
 * @brief  Clear DMA Receive Data Error Flag
 * @rmtoll DMA_IF           RX_ERR            LL_UART0_5_DMA_ClearFlag_RXERR
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_ClearFlag_RXERR(UART0_5_TypeDef *UART0_5x)
{
    WRITE_REG(UART0_5x->DMA_IF, UART0_5_DMA_IF_RX_ERR);
}

/**
 * @brief  Check if the UART0_5 DMA Receive Complete Flag is set or not
 * @rmtoll DMA_IF           RX_DONE            LL_UART0_5_DMA_IsActiveFlag_RXTC
 * @param  UART0_5x UART0_5 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_UART0_5_DMA_IsActiveFlag_RXTC(UART0_5_TypeDef *UART0_5x)
{
    return (READ_BIT(UART0_5x->DMA_IF, UART0_5_DMA_IF_RX_DONE) == (UART0_5_DMA_IF_RX_DONE));
}

/**
 * @brief  Clear DMA Receive Complete Flag
 * @rmtoll DMA_IF           RX_DONE            LL_UART0_5_DMA_ClearFlag_RXTC
 * @param  UART0_5x UART0_5 Instance
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_DMA_ClearFlag_RXTC(UART0_5_TypeDef *UART0_5x)
{
    WRITE_REG(UART0_5x->DMA_IF, UART0_5_DMA_IF_RX_DONE);
}

/**
 * @}
 */

/** @defgroup UART0_5_LL_EF_Data_Management Data_Management
 * @{
 */

/**
 * @brief  Read Receiver Data register (Receive Data value, 8 bits)
 * @rmtoll RXDATA           RXDATA            LL_UART0_5_ReceiveData8
 * @param  UART0_5x UART0_5 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF
 */
__STATIC_INLINE uint8_t LL_UART0_5_ReceiveData8(const UART0_5_TypeDef *UART0_5x)
{
    return (uint8_t)(READ_BIT(UART0_5x->RXDATA, UART0_5_RXDATA_RXDATA));
}

/**
 * @brief  Read Receiver Data register (Receive Data value, 9 bits)
 * @rmtoll RXDATA           RXDATA            LL_UART0_5_ReceiveData9\n
 *         RXDATA           UP                LL_UART0_5_ReceiveData9
 * @param  UART0_5x UART0_5 Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x1FF
 */
__STATIC_INLINE uint16_t LL_UART0_5_ReceiveData9(const UART0_5_TypeDef *UART0_5x)
{
    return (uint16_t)(READ_BIT(UART0_5x->RXDATA, UART0_5_RXDATA_RXDATA | UART0_5_RXDATA_UP));
}

/**
 * @brief  Write in Transmitter Data Register (Transmit Data value, 8 bits)
 * @rmtoll TXDATA           TXDATA            LL_UART0_5_TransmitData8
 * @param  UART0_5x UART0_5 Instance
 * @param  Value between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_TransmitData8(UART0_5_TypeDef *UART0_5x, uint8_t Value)
{
    UART0_5x->TXDATA = Value;
}

/**
 * @brief  Write in Transmitter Data Register (Transmit Data value, 9 bits)
 * @rmtoll TXDATA           TXDATA            LL_UART0_5_TransmitData9\n
 *         TXDATA           UP                LL_UART0_5_TransmitData9
 * @param  UART0_5x UART0_5 Instance
 * @param  Value between Min_Data=0x00 and Max_Data=0x1FF
 * @retval None
 */
__STATIC_INLINE void LL_UART0_5_TransmitData9(UART0_5_TypeDef *UART0_5x, uint16_t Value)
{
    UART0_5x->TXDATA = Value & 0x1FFU;
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup UART0_5_LL_EF_Init Initialization and de-initialization functions
 * @{
 */

// The initialization function can have more than one argument.
ErrorStatus LL_UART0_5_Init(UART0_5_TypeDef *UART0_5x, const LL_UART0_5_InitTypeDef *UART0_5_InitStruct);
ErrorStatus LL_UART0_5_DeInit(UART0_5_TypeDef *UART0_5x);
void        LL_UART0_5_StructInit(LL_UART0_5_InitTypeDef *UART0_5_InitStruct); // If the module is not suitable for the default configuration, you could not defines this function.

/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */

/**
 * @}
 */

#endif /* UART0 || UART1 || UART2 || UART3 || UART4 || UART5 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_UART0_5_H */
