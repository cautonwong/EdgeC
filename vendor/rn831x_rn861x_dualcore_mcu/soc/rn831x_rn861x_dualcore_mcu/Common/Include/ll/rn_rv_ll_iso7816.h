/**
 *****************************************************************************
 * @file   rn_rv_ll_iso7816.h
 * @author Renergy Software Team
 * @brief  Header file of ISO7816 LL module.
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
#ifndef __RN_LL_ISO7816_H
#define __RN_LL_ISO7816_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_iso7816.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (ISO78160) || defined (ISO78161)

/** @defgroup ISO7816_LL ISO7816
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup ISO7816_LL_Private_Constants ISO7816 Private Constants
 * @{
 */

/* Defines used to perform compute and check in the macros */
#define LL_ISO7816_XXX           000000U

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup ISO7816_LL_Private_Macros ISO7816 Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup ISO7816_LL_ES_INIT ISO7816 Exported Init structure
 * @{
 */

/**
 * @brief LL ISO7816 Init Structure definition
 */
typedef struct
{
    uint32_t ClockDivision;       /*!< Specifies the clock output division coefficient.
                                    This parameter can be a value of @ref ISO7816_LL_EC_CLOCKDIV.

                                    This feature can be modified afterwards using unitary function @ref LL_ISO7816_SetClockDivision(). */

    uint32_t ErrorWidth;          /*!< Specifies the length of ETU transmitted in a error signal.
                                    This parameter can be a value of @ref ISO7816_LL_EC_ERRORWIDTH.

                                    This feature can be modified afterwards using unitary function @ref LL_ISO7816_SetErrorWidth() */

    uint32_t Parity;              /*!< Specifies the parity mode.
                                    This parameter can be a value of @ref ISO7816_LL_EC_PARITY.

                                   This feature can be modified afterwards using unitary function @ref LL_ISO7816_SetParity(). */

    uint32_t TransferDirection;   /*!< Specifies whether the Receive and/or Transmit mode is enabled or disabled.
                                    This parameter can be a value of @ref ISO7816_LL_EC_DIRECTION

                                    This feature can be modified afterwards using unitary function @ref LL_ISO7816_SetTransferDirection(). */

    uint32_t AutoRetryCount;      /*!< Specifies the Automatic Retransmission Count when data parity check error.
                                    This parameter can be a value of @ref ISO7816_LL_EC_ISO7816_AUTO_RETRANSMISSION_COUNT

                                    This feature can be modified afterwards using unitary function @ref LL_ISO7816_SetAutoRetransmissionCount(). */

    uint32_t TXGuardTime;         /*!< Specifies the length of Transmit data guard time.
                                    This parameter can be a value of @ref ISO7816_LL_EC_TXGUARDTIME

                                    This feature can be modified afterwards using unitary function @ref LL_ISO7816_SetGuardTime_TX(). */

    uint32_t RXGuardTime;         /*!< Specifies the length of Receive data guard time.
                                    This parameter can be a value of @ref ISO7816_LL_EC_RXGUARDTIME

                                    This feature can be modified afterwards using unitary function @ref LL_ISO7816_GetGuardTime_TX(). */
} LL_ISO7816_InitTypeDef;

/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/* Exported constants -------------------------------------------------------*/
/** @defgroup ISO7816_LL_Exported_Constants ISO7816 Exported Constants
 * @{
 */

/** @defgroup ISO7816_LL_EC_GET_FLAG Get Flags Defines
 * @brief    Flags defines which can be used with LL_ISO7816_ReadReg function
 * @{
 */
#define LL_ISO7816_STA_TXIF                 ISO7816_STA_TXIF                      /*!< Data transmit interrupt flag                                           */
#define LL_ISO7816_STA_RXIF                 ISO7816_STA_RXIF                      /*!< Data received interrupt flag                                           */
#define LL_ISO7816_STA_OVIF                 ISO7816_STA_OVL_IF                    /*!< Receive data overflow interrupt flag                                   */
#define LL_ISO7816_STA_COLIF                ISO7816_STA_COL_IF                    /*!< Transmit data conflict error interrupt flag                            */
#define LL_ISO7816_STA_RXPARERRIF           ISO7816_STA_RXPAR_ERRIF               /*!< Receive data parity error flag                                         */
#define LL_ISO7816_STA_TXPARERRIF           ISO7816_STA_TXPAR_ERRIF               /*!< Transmit data parity error flag                                        */
#define LL_ISO7816_STA_TXBUSY               ISO7816_STA_TXBUSY                    /*!< Data sending busy flag, read only                                      */
#define LL_ISO7816_STA_RXBUSY               ISO7816_STA_RXBUSY                    /*!< Data receiving busy flag, read only                                    */
#define LL_ISO7816_STA_RXF                  ISO7816_STA_RX_FLAG                   /*!< Data receiving complete flag, read only                                */
#define LL_ISO7816_STA_TXF                  ISO7816_STA_TX_FLAG                   /*!< Send buffer empty flag, read only                                      */
#define LL_ISO7816_STA_BDDIV                ISO7816_STA_BDDIV_R                   /*!< Baud rate match indication bit, read only                              */
#define LL_ISO7816_STA_FRAMEERR             ISO7816_STA_FRAME_ERR                 /*!< Receive data frame format error interrupt flag                         */
#define LL_ISO7816_STA_OLD                  ISO7816_STA_OLD_FLAG                  /*!< Receive OLD signal interrupt flag, it is valid after OLD_EN is enabled */
#define LL_ISO7816_STA_CARDOUT              ISO7816_STA_CARD_OUT_FLAG             /*!< Card pull-out detect flag, it is valid after CARD_CHECK_EN is enabled  */
/**
 * @}
 */

/** @defgroup ISO7816_LL_EC_ENCODING Data Encoding Method
 * @{
 */
#define LL_ISO7816_LSB_FIRST                0x00000000U                                /*!< Data encoding method selection (LSB)*/
#define LL_ISO7816_MSB_FIRST                ISO7816_CTRL_DIRSEL                        /*!< Data encoding method selection (MSB)*/
/**
 * @}
 */

/** @defgroup ISO7816_LL_EC_DIRECTION Communication Direction
 * @{
 */
#define LL_ISO7816_DIRECTION_NONE           0x00000000U                               /*!< Transmit data and Receive data are disabled           */
#define LL_ISO7816_DIRECTION_TX             ISO7816_CTRL_TX_EN                        /*!< Transmit data is enabled and Receive data is disabled */
#define LL_ISO7816_DIRECTION_RX             ISO7816_CTRL_RX_EN                        /*!< Transmit data is disabled and Receive data is enabled */
#define LL_ISO7816_DIRECTION_TX_RX          (ISO7816_CTRL_TX_EN | ISO7816_CTRL_RX_EN) /*!< Transmit data and Receive data are enabled            */
/**
 * @}
 */

/** @defgroup ISO7816_LL_EC_PARITY Parity Control
 * @{
 */
#define LL_ISO7816_PARITY_NONE              ISO7816_CTRL_PARSEL_PARITY_NONE           /*!< No Parity                       */
#define LL_ISO7816_PARITY_ODD               ISO7816_CTRL_PARSEL_PARITY_ODD            /*!< Odd Parity is selected          */
#define LL_ISO7816_PARITY_EVEN              ISO7816_CTRL_PARSEL_PARITY_EVEN           /*!< Even Parity is selected         */
#define LL_ISO7816_PARITY_ZERO              ISO7816_CTRL_PARSEL_PARITY_ZERO           /*!< Zero Parity is selected         */
#define LL_ISO7816_PARITY_ONE               ISO7816_CTRL_PARSEL_PARITY_ONE            /*!< One Parity is selected          */
#define LL_ISO7816_PARITY_USER_DEFINED      ISO7816_CTRL_PARSEL_PARITY_USER_DEFINED   /*!< User defined Parity is selected */
/**
 * @}
 */

/** @defgroup ISO7816_LL_EC_ERRORWIDTH Error Signal Width
 * @{
 */
#define LL_ISO7816_ERRWIDTH_2ETU            ISO7816_CTRL_ERRWTH_2           /*!< Error signal width 2 ETU selected   */
#define LL_ISO7816_ERRWIDTH_1ETU            ISO7816_CTRL_ERRWTH_1           /*!< Error signal width 1 ETU selected   */
#define LL_ISO7816_ERRWIDTH_1_5ETU          ISO7816_CTRL_ERRWTH_1_5         /*!< Error signal width 1.5 ETU selected */
/**
 * @}
 */

/** @defgroup ISO7816_LL_EC_ISO7816_AUTO_RETRANSMISSION_COUNT Automatic Retransmission Count
 * @{
 */
#define LL_ISO7816_RETRYCOUNT_NONE          ISO7816_CTRL_REP_CNT_NONE           /*!< No automatic retransmission */
#define LL_ISO7816_RETRYCOUNT_1             ISO7816_CTRL_REP_CNT_1              /*!< Automatic retransmission count is 1 */
#define LL_ISO7816_RETRYCOUNT_2             ISO7816_CTRL_REP_CNT_2              /*!< Automatic retransmission count is 2 */
#define LL_ISO7816_RETRYCOUNT_3             ISO7816_CTRL_REP_CNT_3              /*!< Automatic retransmission count is 3 */
#define LL_ISO7816_RETRYCOUNT_4             ISO7816_CTRL_REP_CNT_4              /*!< Automatic retransmission count is 4 */
#define LL_ISO7816_RETRYCOUNT_5             ISO7816_CTRL_REP_CNT_5              /*!< Automatic retransmission count is 5 */
#define LL_ISO7816_RETRYCOUNT_6             ISO7816_CTRL_REP_CNT_6              /*!< Automatic retransmission count is 6 */
#define LL_ISO7816_RETRYCOUNT_7             ISO7816_CTRL_REP_CNT_7              /*!< Automatic retransmission count is 7 */
/**
 * @}
 */

/** @defgroup ISO7816_LL_EC_TXGUARDTIME TX Guard Time
 * @{
 */
#define LL_ISO7816_GUARDTIME_TX_2ETU            0x00000000U                   /*!< Transmit data GT width selected 2 ETU */
#define LL_ISO7816_GUARDTIME_TX_1ETU            ISO7816_CTRL_TX_GT            /*!< Transmit data GT width selected 1 ETU  */
/**
 * @}
 */

/** @defgroup ISO7816_LL_EC_RXGUARDTIME RX Guard Time
 * @{
 */
#define LL_ISO7816_GUARDTIME_RX_2ETU            0x00000000U                   /*!< Receive data GT width selected 2 ETU */
#define LL_ISO7816_GUARDTIME_RX_1ETU            ISO7816_CTRL_RX_GT            /*!< Receive data GT width selected 1 ETU */
/**
 * @}
 */

/** @defgroup ISO7816_LL_EC_CLOCKDIV Clock Division
 * @{
 */
#define LL_ISO7816_CLOCKDIV_NONE               ISO7816_CLK_CLKDIV_0           /*!< Frequency division coefficient is 0  */
#define LL_ISO7816_CLOCKDIV_2                  ISO7816_CLK_CLKDIV_2           /*!< Frequency division coefficient is 2  */
#define LL_ISO7816_CLOCKDIV_4                  ISO7816_CLK_CLKDIV_4           /*!< Frequency division coefficient is 4  */
#define LL_ISO7816_CLOCKDIV_8                  ISO7816_CLK_CLKDIV_8           /*!< Frequency division coefficient is 8  */
#define LL_ISO7816_CLOCKDIV_16                 ISO7816_CLK_CLKDIV_16          /*!< Frequency division coefficient is 16 */
#define LL_ISO7816_CLOCKDIV_32                 ISO7816_CLK_CLKDIV_32          /*!< Frequency division coefficient is 32 */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup ISO7816_LL_Exported_Macros ISO7816 Exported Macros
 * @{
 */
/** @defgroup ISO7816_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in ISO7816 register
 * @param  __INSTANCE__ ISO7816 Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_ISO7816_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in ISO7816 register
 * @param  __INSTANCE__ ISO7816 Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_ISO7816_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup ISO7816_LL_Exported_Functions ISO7816 Exported Functions
 * @{
 */

/** @defgroup ISO7816_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief  Enable ISO7816 peripheral (EN = 1).
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_Enable(ISO7816_TypeDef *ISO7816x)
{
    SET_BIT(ISO7816x->CTRL, ISO7816_CTRL_EN);
}

/**
 * @brief  Disable ISO7816 peripheral (EN = 0).
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_Disable(ISO7816_TypeDef *ISO7816x)
{
    CLEAR_BIT(ISO7816x->CTRL, ISO7816_CTRL_EN);
}

/**
 * @brief  Check if the ISO7816 peripheral is enabled or disabled.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsEnabled(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_EN) == (ISO7816_CTRL_EN));
}

/**
 * @brief  Configure the ISO7816 Data Encoding Method (LSB or MSB).
 * @note   Set the data Encoding method.
 * @param  ISO7816x ISO7816 Instance.
 * @param  BitOrder This parameter can be one of the following values:
 *         @arg @ref LL_ISO7816_LSB_FIRST
 *         @arg @ref LL_ISO7816_MSB_FIRST
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_SetTransferBitOrder(ISO7816_TypeDef *ISO7816x, uint32_t BitOrder)
{
    MODIFY_REG(ISO7816x->CTRL, ISO7816_CTRL_DIRSEL, BitOrder);
}

/**
 * @brief  Get the the ISO7816 Data Encoding Method (LSB or MSB).
 * @param  ISO7816x ISO7816 Instance.
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ISO7816_LSB_FIRST
 *         @arg @ref LL_ISO7816_MSB_FIRST
 */
__STATIC_INLINE uint32_t LL_ISO7816_GetTransferBitOrder(ISO7816_TypeDef *ISO7816x)
{
    return (uint32_t)(READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_DIRSEL));
}

/**
 * @brief  Receive data Enable.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_EnableDirectionRx(ISO7816_TypeDef *ISO7816x)
{
    SET_BIT(ISO7816x->CTRL, ISO7816_CTRL_RX_EN);
}

/**
 * @brief  Receive data Disable.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_DisableDirectionRx(ISO7816_TypeDef *ISO7816x)
{
    CLEAR_BIT(ISO7816x->CTRL, ISO7816_CTRL_RX_EN);
}

/**
 * @brief  Transmit data Enable
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_EnableDirectionTx(ISO7816_TypeDef *ISO7816x)
{
    SET_BIT(ISO7816x->CTRL, ISO7816_CTRL_TX_EN);
}

/**
 * @brief  Transmit data Disable
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_DisableDirectionTx(ISO7816_TypeDef *ISO7816x)
{
    CLEAR_BIT(ISO7816x->CTRL, ISO7816_CTRL_TX_EN);
}

/**
 * @brief  Configure simultaneously enabled/disabled states
 *         of Transmit data and Receive data.
 * @param  ISO7816x ISO7816 Instance
 * @param  TransferDirection This parameter can be one of the following values:
 *         @arg @ref LL_ISO7816_DIRECTION_NONE
 *         @arg @ref LL_ISO7816_DIRECTION_TX
 *         @arg @ref LL_ISO7816_DIRECTION_RX
 *         @arg @ref LL_ISO7816_DIRECTION_TX_RX
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_SetTransferDirection(ISO7816_TypeDef *ISO7816x, uint32_t TransferDirection)
{
    MODIFY_REG(ISO7816x->CTRL, ISO7816_CTRL_TX_EN | ISO7816_CTRL_RX_EN, TransferDirection);
}

/**
 * @brief  Return enabled/disabled states of Transmit data and Receive data.
 * @param  ISO7816x ISO7816 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ISO7816_DIRECTION_NONE
 *         @arg @ref LL_ISO7816_DIRECTION_TX
 *         @arg @ref LL_ISO7816_DIRECTION_RX
 *         @arg @ref LL_ISO7816_DIRECTION_TX_RX
 */
__STATIC_INLINE uint32_t LL_ISO7816_GetTransferDirection(ISO7816_TypeDef *ISO7816x)
{
    return (uint32_t)(READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_TX_EN | ISO7816_CTRL_RX_EN));
}

/**
 * @brief  Enable ISO7816 BGT function (BGT_EN = 1).
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_EnableBGT(ISO7816_TypeDef *ISO7816x)
{
    SET_BIT(ISO7816x->CTRL, ISO7816_CTRL_BGT_EN);
}

/**
 * @brief  Disable ISO7816 BGT function (BGT_EN = 0).
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_DisableBGT(ISO7816_TypeDef *ISO7816x)
{
    CLEAR_BIT(ISO7816x->CTRL, ISO7816_CTRL_BGT_EN);
}

/**
 * @brief  Check if the ISO7816 BGT function is enabled or disabled.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsEnabledBGT(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_BGT_EN) == (ISO7816_CTRL_BGT_EN));
}

/**
 * @brief  Configure Parity.
 * @param  ISO7816x ISO7816 Instance
 * @param  Parity This parameter can be one of the following values:
 *         @arg @ref LL_ISO7816_PARITY_NONE
 *         @arg @ref LL_ISO7816_PARITY_ODD
 *         @arg @ref LL_ISO7816_PARITY_EVEN
 *         @arg @ref LL_ISO7816_PARITY_ZERO
 *         @arg @ref LL_ISO7816_PARITY_ONE
 *         @arg @ref LL_ISO7816_PARITY_USER_DEFINED
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_SetParity(ISO7816_TypeDef *ISO7816x, uint32_t Parity)
{
    MODIFY_REG(ISO7816x->CTRL, ISO7816_CTRL_PARSEL, Parity);
}

/**
 * @brief  Return Parity configuration.
 * @param  ISO7816x ISO7816 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ISO7816_PARITY_NONE
 *         @arg @ref LL_ISO7816_PARITY_ODD
 *         @arg @ref LL_ISO7816_PARITY_EVEN
 *         @arg @ref LL_ISO7816_PARITY_ZERO
 *         @arg @ref LL_ISO7816_PARITY_ONE
 *         @arg @ref LL_ISO7816_PARITY_USER_DEFINED
 */
__STATIC_INLINE uint32_t LL_ISO7816_GetParity(ISO7816_TypeDef *ISO7816x)
{
    return (uint32_t)(READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_PARSEL));
}

/**
 * @brief  Set Error Signal Width.
 * @param  ISO7816x ISO7816 Instance
 * @param  ErrorWidth This parameter can be one of the following values:
 *         @arg @ref LL_ISO7816_ERRWIDTH_2ETU
 *         @arg @ref LL_ISO7816_ERRWIDTH_1ETU
 *         @arg @ref LL_ISO7816_ERRWIDTH_1_5ETU
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_SetErrorWidth(ISO7816_TypeDef *ISO7816x, uint32_t ErrorWidth)
{
    MODIFY_REG(ISO7816x->CTRL, ISO7816_CTRL_ERRWTH, ErrorWidth);
}

/**
 * @brief  Return Error Signal Width.
 * @param  ISO7816x ISO7816 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ISO7816_ERRWIDTH_2ETU
 *         @arg @ref LL_ISO7816_ERRWIDTH_1ETU
 *         @arg @ref LL_ISO7816_ERRWIDTH_1_5ETU
 */
__STATIC_INLINE uint32_t LL_ISO7816_GetErrorWidth(ISO7816_TypeDef *ISO7816x)
{
    return (uint32_t)(READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_ERRWTH));
}

/**
 * @brief  Set Clock Output Division Coefficient.
 * @param  ISO7816x ISO7816 Instance
 * @param  ClockDivision This parameter can be one of the following values:
 *         @arg @ref LL_ISO7816_CLOCKDIV_NONE
 *         @arg @ref LL_ISO7816_CLOCKDIV_2
 *         @arg @ref LL_ISO7816_CLOCKDIV_4
 *         @arg @ref LL_ISO7816_CLOCKDIV_8
 *         @arg @ref LL_ISO7816_CLOCKDIV_16
 *         @arg @ref LL_ISO7816_CLOCKDIV_32
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_SetClockDivision(ISO7816_TypeDef *ISO7816x, uint32_t ClockDivision)
{
    MODIFY_REG(ISO7816x->CLK, ISO7816_CLK_CLKDIV, ClockDivision);
}

/**
 * @brief  Return Clock Output Division Coefficient.
 * @param  ISO7816x ISO7816 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ISO7816_CLOCKDIV_NONE
 *         @arg @ref LL_ISO7816_CLOCKDIV_2
 *         @arg @ref LL_ISO7816_CLOCKDIV_4
 *         @arg @ref LL_ISO7816_CLOCKDIV_8
 *         @arg @ref LL_ISO7816_CLOCKDIV_16
 *         @arg @ref LL_ISO7816_CLOCKDIV_32
 */
__STATIC_INLINE uint32_t LL_ISO7816_GetClockDivision(ISO7816_TypeDef *ISO7816x)
{
    return (uint32_t)(READ_BIT(ISO7816x->CLK, ISO7816_CLK_CLKDIV));
}

/**
 * @brief  Enable NACK (i.e. error signal) transmission when receiveed data parity check failed (T = 0 protocol).
 * @param  ISO7816x ISO7816 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_EnableNACK(ISO7816_TypeDef *ISO7816x)
{
    SET_BIT(ISO7816x->CTRL, ISO7816_CTRL_RXPAR_ESEL);
}

/**
 * @brief  Disable NACK (i.e. error signal) transmission when receiveed data parity check failed (T = 0 protocol).
 * @param  ISO7816x ISO7816 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_DisableNACK(ISO7816_TypeDef *ISO7816x)
{
    CLEAR_BIT(ISO7816x->CTRL, ISO7816_CTRL_RXPAR_ESEL);
}

/**
 * @brief  Indicate if NACK (i.e. error signal) transmission is enabled or disabled (T = 0 protocol).
 * @param  ISO7816x ISO7816 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsEnabledNACK(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_RXPAR_ESEL) == (ISO7816_CTRL_RXPAR_ESEL));
}

/**
 * @brief  Set Automatic Retransmission Count when data parity check error occurs.
 * @param  ISO7816x ISO7816 Instance
 * @param  AutoRetryCount This parameter can be one of the following values:
 *         @arg @ref LL_ISO7816_RETRYCOUNT_NONE
 *         @arg @ref LL_ISO7816_RETRYCOUNT_1
 *         @arg @ref LL_ISO7816_RETRYCOUNT_2
 *         @arg @ref LL_ISO7816_RETRYCOUNT_3
 *         @arg @ref LL_ISO7816_RETRYCOUNT_4
 *         @arg @ref LL_ISO7816_RETRYCOUNT_5
 *         @arg @ref LL_ISO7816_RETRYCOUNT_6
 *         @arg @ref LL_ISO7816_RETRYCOUNT_7
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_SetAutoRetransmissionCount(ISO7816_TypeDef *ISO7816x, uint32_t AutoRetryCount)
{
    MODIFY_REG(ISO7816x->CTRL, ISO7816_CTRL_REP_CNT, AutoRetryCount);
}

/**
 * @brief  Return Automatic Retransmission Count.
 * @param  ISO7816x ISO7816 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ISO7816_RETRYCOUNT_NONE
 *         @arg @ref LL_ISO7816_RETRYCOUNT_1
 *         @arg @ref LL_ISO7816_RETRYCOUNT_2
 *         @arg @ref LL_ISO7816_RETRYCOUNT_3
 *         @arg @ref LL_ISO7816_RETRYCOUNT_4
 *         @arg @ref LL_ISO7816_RETRYCOUNT_5
 *         @arg @ref LL_ISO7816_RETRYCOUNT_6
 *         @arg @ref LL_ISO7816_RETRYCOUNT_7
 */
__STATIC_INLINE uint32_t LL_ISO7816_GetAutoRetransmissionCount(ISO7816_TypeDef *ISO7816x)
{
    return (uint32_t)(READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_REP_CNT));
}

/**
 * @brief  Set Extra Guard time value N (CTRL[24:17] bits : Extra Guard time value)
 * @param  ISO7816x ISO7816 Instance
 * @param  ExtraGuardTime Value between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_SetExtraGuardTime(ISO7816_TypeDef *ISO7816x, uint32_t ExtraGuardTime)
{
    MODIFY_REG(ISO7816x->CTRL, ISO7816_CTRL_EGT, ExtraGuardTime << ISO7816_CTRL_EGT_POS);
}

/**
 * @brief  Return Extra Guard time value N (CTRL[24:17] bits : Extra Guard time value)
 * @param  ISO7816x ISO7816 Instance
 * @retval Extra Guard time value (Value between Min_Data=0x00 and Max_Data=0xFF)
 */
__STATIC_INLINE uint32_t LL_ISO7816_GetExtraGuardTime(ISO7816_TypeDef *ISO7816x)
{
    return (uint32_t)(READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_EGT) >> ISO7816_CTRL_EGT_POS);
}

/**
 * @brief  Enable IO Data bidirectional.
 * @note   Data ouput/input use only one port.
 * @param  ISO7816x ISO7816 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_EnableIOBidirectional(ISO7816_TypeDef *ISO7816x)
{
    SET_BIT(ISO7816x->CTRL, ISO7816_CTRL_IO_EN);
}

/**
 * @brief  Disable IO Data bidirectional.
 * @note   Only ouput, data input from 7816_I port.
 * @param  ISO7816x ISO7816 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_DisableIOBidirectional(ISO7816_TypeDef *ISO7816x)
{
    CLEAR_BIT(ISO7816x->CTRL, ISO7816_CTRL_IO_EN);
}

/**
 * @brief  Indicate if IO Data bidirectional is enabled or disabled.
 * @param  ISO7816x ISO7816 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsEnabledIOBidirectional(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_IO_EN) == (ISO7816_CTRL_IO_EN));
}

/**
 * @brief  Set Transmit Data Guard time.
 * @param  ISO7816x ISO7816 Instance
 * @param  TXGuardTime This parameter can be one of the following values:
 *         @arg @ref LL_ISO7816_GUARDTIME_TX_2ETU
 *         @arg @ref LL_ISO7816_GUARDTIME_TX_1ETU
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_SetGuardTime_TX(ISO7816_TypeDef *ISO7816x, uint32_t TXGuardTime)
{
    MODIFY_REG(ISO7816x->CTRL, ISO7816_CTRL_TX_GT, TXGuardTime);
}

/**
 * @brief  Return Transmit Data Guard time.
 * @param  ISO7816x ISO7816 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ISO7816_GUARDTIME_TX_2ETU
 *         @arg @ref LL_ISO7816_GUARDTIME_TX_1ETU
 */
__STATIC_INLINE uint32_t LL_ISO7816_GetGuardTime_TX(ISO7816_TypeDef *ISO7816x)
{
    return (uint32_t)(READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_TX_GT));
}

/**
 * @brief  Set Receive Data Guard time.
 * @param  ISO7816x ISO7816 Instance
 * @param  RXGuardTime This parameter can be one of the following values:
 *         @arg @ref LL_ISO7816_GUARDTIME_RX_2ETU
 *         @arg @ref LL_ISO7816_GUARDTIME_RX_1ETU
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_SetGuardTime_RX(ISO7816_TypeDef *ISO7816x, uint32_t RXGuardTime)
{
    MODIFY_REG(ISO7816x->CTRL, ISO7816_CTRL_RX_GT, RXGuardTime);
}

/**
 * @brief  Return Receive Data Guard time.
 * @param  ISO7816x ISO7816 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ISO7816_GUARDTIME_RX_2ETU
 *         @arg @ref LL_ISO7816_GUARDTIME_RX_1ETU
 */
__STATIC_INLINE uint32_t LL_ISO7816_GetGuardTime_RX(ISO7816_TypeDef *ISO7816x)
{
    return (uint32_t)(READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_RX_GT));
}

/**
 * @brief  Enable ISO7816 OLD Detect (OLD_EN = 1).
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_EnableOLDDetect(ISO7816_TypeDef *ISO7816x)
{
    SET_BIT(ISO7816x->CTRL, ISO7816_CTRL_OLD_EN);
}

/**
 * @brief  Disable ISO7816 OLD Detect (OLD_EN = 0).
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_DisableOLDDetect(ISO7816_TypeDef *ISO7816x)
{
    CLEAR_BIT(ISO7816x->CTRL, ISO7816_CTRL_OLD_EN);
}

/**
 * @brief  Check if the ISO7816 OLD Detect is enabled or disabled.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsEnabledOLDDetect(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_OLD_EN) == (ISO7816_CTRL_OLD_EN));
}

/**
 * @brief  Enable Card Clock Output (CLKO_EN = 1).
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_EnableClockOutput(ISO7816_TypeDef *ISO7816x)
{
    SET_BIT(ISO7816x->CLK, ISO7816_CLK_CLKO_EN);
}

/**
 * @brief  Disable Card Clock Output (CLKO_EN = 0).
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_DisableClockOutput(ISO7816_TypeDef *ISO7816x)
{
    CLEAR_BIT(ISO7816x->CLK, ISO7816_CLK_CLKO_EN);
}

/**
 * @brief  Check if the Card Clock Output is enabled or disabled.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsEnabledClockOutput(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->CLK, ISO7816_CLK_CLKO_EN) == (ISO7816_CLK_CLKO_EN));
}

/**
 * @brief  Enable Software Configuration of F/D (FDS_EN = 1).
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_EnableSoftwareFD(ISO7816_TypeDef *ISO7816x)
{
    SET_BIT(ISO7816x->BDDIV, ISO7816_BDDIV_FDS_EN);
}

/**
 * @brief  Disable Software Configuration of F/D (FDS_EN = 0).
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_DisableSoftwareFD(ISO7816_TypeDef *ISO7816x)
{
    CLEAR_BIT(ISO7816x->BDDIV, ISO7816_BDDIV_FDS_EN);
}

/**
 * @brief  Check if the Software Configuration of F/D is enabled or disabled.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsEnabledSoftwareFD(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->BDDIV, ISO7816_BDDIV_FDS_EN) == (ISO7816_BDDIV_FDS_EN));
}

/**
 * @brief  Set Software Configuration of F/D value.
 * @note   Only can be written when FDS_EN = 1, if not the default value is 14’d372.
 * @param  ISO7816x ISO7816 Instance
 * @param  SoftwareFD Value between Min_Data=0x00 and Max_Data=0x3FFF
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_SetSoftwareFD(ISO7816_TypeDef *ISO7816x, uint32_t SoftwareFD)
{
    MODIFY_REG(ISO7816x->BDDIV, ISO7816_BDDIV_FDS, SoftwareFD << ISO7816_BDDIV_FDS_POS);
}

/**
 * @brief  Return Software Configuration of F/D value.
 * @param  ISO7816x ISO7816 Instance
 * @retval Software Configuration of F/D value (Value between Min_Data=0x00 and Max_Data=0x3FFF)
 */
__STATIC_INLINE uint32_t LL_ISO7816_GetSoftwareFD(ISO7816_TypeDef *ISO7816x)
{
    return (uint32_t)(READ_BIT(ISO7816x->BDDIV, ISO7816_BDDIV_FDS) >> ISO7816_BDDIV_FDS_POS);
}

/**
 * @brief  Set Answer To Reset FD value.
 * @note   FD[7:4] bits: F (clock rate conversion factor),
 *         FD[3:0] bits: D (baud rate correction factor).
 * @param  ISO7816x ISO7816 Instance
 * @param  AnswerToResetFD Value between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_SetAnswerToResetFD(ISO7816_TypeDef *ISO7816x, uint32_t AnswerToResetFD)
{
    MODIFY_REG(ISO7816x->BDDIV, ISO7816_BDDIV_FD, AnswerToResetFD << ISO7816_BDDIV_FD_POS);
}

/**
 * @brief  Return Answer To Reset FD value.
 * @param  ISO7816x ISO7816 Instance
 * @retval Answer To Reset FD value (Value between Min_Data=0x00 and Max_Data=0xFF)
 */
__STATIC_INLINE uint32_t LL_ISO7816_GetAnswerToResetFD(ISO7816_TypeDef *ISO7816x)
{
    return (uint32_t)(READ_BIT(ISO7816x->BDDIV, ISO7816_BDDIV_FD) >> ISO7816_BDDIV_FD_POS);
}

/**
 * @}
 */

/** @defgroup ISO7816_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable ISO7816 data Transmit interrupt.
 * @rmtoll CTRL       TX_IRQ_EN         LL_ISO7816_EnableIT_TX
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_EnableIT_TX(ISO7816_TypeDef *ISO7816x)
{
    SET_BIT(ISO7816x->CTRL, ISO7816_CTRL_TX_IRQ_EN);
}

/**
 * @brief  Disable ISO7816 data Transmit interrupt.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_DisableIT_TX(ISO7816_TypeDef *ISO7816x)
{
    CLEAR_BIT(ISO7816x->CTRL, ISO7816_CTRL_TX_IRQ_EN);
}

/**
 * @brief  Check if the ISO7816 data Transmit interrupt is enabled or disabled.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsEnabledIT_TX(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_TX_IRQ_EN) == (ISO7816_CTRL_TX_IRQ_EN));
}

/**
 * @brief  Enable ISO7816 data Receive interrupt.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_EnableIT_RX(ISO7816_TypeDef *ISO7816x)
{
    SET_BIT(ISO7816x->CTRL, ISO7816_CTRL_RX_IRQ_EN);
}

/**
 * @brief  Disable ISO7816 data Receive interrupt.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_DisableIT_RX(ISO7816_TypeDef *ISO7816x)
{
    CLEAR_BIT(ISO7816x->CTRL, ISO7816_CTRL_RX_IRQ_EN);
}

/**
 * @brief  Check if the ISO7816 data Receive interrupt is enabled or disabled.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsEnabledIT_RX(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_RX_IRQ_EN) == (ISO7816_CTRL_RX_IRQ_EN));
}

/**
 * @brief  Enable ISO7816 data Transmission Error interrupt.
 * @note   Conflict when transmit data, overflow when receive data, and frame
 *         format error when received data will be Transmission error interrupt
 *         occured.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_EnableIT_ERR(ISO7816_TypeDef *ISO7816x)
{
    SET_BIT(ISO7816x->CTRL, ISO7816_CTRL_ERR_IRQ_EN);
}

/**
 * @brief  Disable ISO7816 data Transmission Error interrupt.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_DisableIT_ERR(ISO7816_TypeDef *ISO7816x)
{
    CLEAR_BIT(ISO7816x->CTRL, ISO7816_CTRL_ERR_IRQ_EN);
}

/**
 * @brief  Check if the ISO7816 data Transmission Error interrupt is enabled or disabled.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsEnabledIT_ERR(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_ERR_IRQ_EN) == (ISO7816_CTRL_ERR_IRQ_EN));
}

/**
 * @brief  Enable ISO7816 OLD Detect interrupt.
 * @note   Only valid when OLD detection function is enabled
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_EnableIT_OLD(ISO7816_TypeDef *ISO7816x)
{
    SET_BIT(ISO7816x->CTRL, ISO7816_CTRL_OLD_IRQ_EN);
}

/**
 * @brief  Disable ISO7816 OLD Detect interrupt.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_DisableIT_OLD(ISO7816_TypeDef *ISO7816x)
{
    CLEAR_BIT(ISO7816x->CTRL, ISO7816_CTRL_OLD_IRQ_EN);
}

/**
 * @brief  Check if the ISO7816 OLD Detect interrupt is enabled or disabled.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsEnabledIT_OLD(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_OLD_IRQ_EN) == (ISO7816_CTRL_OLD_IRQ_EN));
}

/**
 * @brief  Enable ISO7816 Card Pull-out Detect interrupt.
 * @note   Only valid when OLD detection function is enabled
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_EnableIT_CARD(ISO7816_TypeDef *ISO7816x)
{
    SET_BIT(ISO7816x->CTRL, ISO7816_CTRL_CARD_CHECK_EN);
}

/**
 * @brief  Disable ISO7816 Card Pull-out Detect interrupt.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_DisableIT_CARD(ISO7816_TypeDef *ISO7816x)
{
    CLEAR_BIT(ISO7816x->CTRL, ISO7816_CTRL_CARD_CHECK_EN);
}

/**
 * @brief  Check if the ISO7816 Card Pull-out Detect interrupt is enabled or disabled.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsEnabledIT_CARD(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->CTRL, ISO7816_CTRL_CARD_CHECK_EN) == (ISO7816_CTRL_CARD_CHECK_EN));
}

/**
 * @}
 */

/** @defgroup ISO7816_LL_EF_FLAG_management FLAG_management
 * @{
 */

/**
 * @brief  Indicate the status of Data Transmit Interrupt flag.
 * @note   RESET: Write one to clear.
 *         SET: When data transfer to shift register from send buffer register.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsActiveFlag_TXIF(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->STA, ISO7816_STA_TXIF) == (ISO7816_STA_TXIF));
}

/**
 * @brief  Clear Data Transmit Interrupt flag.
 * @note   Write one to clear.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_ClearFlag_TXIF(ISO7816_TypeDef *ISO7816x)
{
    WRITE_REG(ISO7816x->STA, ISO7816_STA_TXIF);
}

/**
 * @brief  Indicate the status of Data Receive Interrupt flag.
 * @note   RESET: Write one to clear.
 *         SET: When data transfer receive buffer register from shift register.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsActiveFlag_RXIF(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->STA, ISO7816_STA_RXIF) == (ISO7816_STA_RXIF));
}

/**
 * @brief  Clear Data Receive Interrupt flag.
 * @note   Write one to clear.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_ClearFlag_RXIF(ISO7816_TypeDef *ISO7816x)
{
    WRITE_REG(ISO7816x->STA, ISO7816_STA_RXIF);
}

/**
 * @brief  Indicate the status of Receive Data Overflow Interrupt flag.
 * @note   RESET: Write one to clear.
 *         SET: When receive buffer register not been read out, and new data is received.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsActiveFlag_OVF(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->STA, ISO7816_STA_OVL_IF) == (ISO7816_STA_OVL_IF));
}

/**
 * @brief  Clear Receive Data Overflow Interrupt flag.
 * @note   Write one to clear.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_ClearFlag_OVF(ISO7816_TypeDef *ISO7816x)
{
    WRITE_REG(ISO7816x->STA, ISO7816_STA_OVL_IF);
}

/**
 * @brief  Indicate the status of Transmit Data Conflict interrupt flag.
 * @note   RESET: Write one to clear.
 *         SET: When transmit data Conflict error occurs.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsActiveFlag_COL(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->STA, ISO7816_STA_COL_IF) == (ISO7816_STA_COL_IF));
}

/**
 * @brief  Clear Transmit Data Conflict interrupt flag.
 * @note   write one to Clear.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_ClearFlag_COL(ISO7816_TypeDef *ISO7816x)
{
    WRITE_REG(ISO7816x->STA, ISO7816_STA_COL_IF);
}

/**
 * @brief  Indicate the status of Receive Data Parity Error flag.
 * @note   RESET: Write one to clear.
 *         SET: When receive data parity error occurs.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsActiveFlag_RXPARERR(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->STA, ISO7816_STA_RXPAR_ERRIF) == (ISO7816_STA_RXPAR_ERRIF));
}

/**
 * @brief  Clear Receive Data Parity Error flag.
 * @note   write one to Clear.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_ClearFlag_RXPARERR(ISO7816_TypeDef *ISO7816x)
{
    WRITE_REG(ISO7816x->STA, ISO7816_STA_RXPAR_ERRIF);
}

/**
 * @brief  Indicate the status of Transmit Data Parity Error flag.
 * @note   RESET: Write one to clear.
 *         SET: When transmit data parity error occurs.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsActiveFlag_TXPARERR(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->STA, ISO7816_STA_TXPAR_ERRIF) == (ISO7816_STA_TXPAR_ERRIF));
}

/**
 * @brief  Clear Transmit Data Parity Error flag.
 * @note   write one to Clear.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_ClearFlag_TXPARERR(ISO7816_TypeDef *ISO7816x)
{
    WRITE_REG(ISO7816x->STA, ISO7816_STA_TXPAR_ERRIF);
}

/**
 * @brief  Indicate the status of Data Transmit Busy flag (read only).
 *         RESET: Read only.
 *         SET: When data transmit busy.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsActiveFlag_TXBUSY(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->STA, ISO7816_STA_TXBUSY) == (ISO7816_STA_TXBUSY));
}

/**
 * @brief  Indicate the status of Data Receive Busy flag (read only).
 *         RESET: Read only.
 *         SET: When data receive busy.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsActiveFlag_RXBUSY(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->STA, ISO7816_STA_RXBUSY) == (ISO7816_STA_RXBUSY));
}

/**
 * @brief  Indicate the status of Receiving Data Finsihed flag.
 * @note   RESET: When read receive buffer register.
 *         SET: When 1 byte data has be received.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsActiveFlag_RXDONE(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->STA, ISO7816_STA_RX_FLAG) == (ISO7816_STA_RX_FLAG));
}

/**
 * @brief  Indicate the status of Transmit buffer empty flag.
 * @note   RESET: When data transfer to shift register from transmit buffer register.
 *         SET: When data is written to transmit buffer register.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsActiveFlag_TXBUFE(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->STA, ISO7816_STA_TX_FLAG) == (ISO7816_STA_TX_FLAG));
}

/**
 * @brief  Indicate the status of Baud Rate matched flag.
 * @note   RESET: When baud rate is matched.
 *         SET: When baud rate is not matched.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsActiveFlag_BDMATCH(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->STA, ISO7816_STA_BDDIV_R) == (ISO7816_STA_BDDIV_R));
}

/**
 * @brief  Indicate the status of Receive Data Frame Format Error Interrupt flag.
 * @note   RESET: Write one to clear.
 *         SET: When receive data frame format error.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsActiveFlag_FRAMEERR(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->STA, ISO7816_STA_FRAME_ERR) == (ISO7816_STA_FRAME_ERR));
}

/**
 * @brief  Clear Receive Data Frame Format Error Interrupt flag.
 * @note   write one to Clear.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_ClearFlag_FRAMEERR(ISO7816_TypeDef *ISO7816x)
{
    WRITE_REG(ISO7816x->STA, ISO7816_STA_FRAME_ERR);
}

/**
 * @brief  Indicate the status of OLD Detect Interrupt flag.
 * @note   RESET: Write one to clear.
 *         SET: When receive OLD signal.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsActiveFlag_OLD(ISO7816_TypeDef * ISO7816x)
{
    return (READ_BIT(ISO7816x->STA, ISO7816_STA_OLD_FLAG) == (ISO7816_STA_OLD_FLAG));
}

/**
 * @brief  Clear OLD Detect Interrupt flag.
 * @note   write one to Clear.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_ClearFlag_OLD(ISO7816_TypeDef *ISO7816x)
{
    WRITE_REG(ISO7816x->STA, ISO7816_STA_OLD_FLAG);
}

/**
 * @brief  Indicate the status of Card Out Detection flag.
 * @note   RESET:  Write one to clear.
 *         SET: When card is out.
 * @param  ISO7816x ISO7816 Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ISO7816_IsActiveFlag_CARDOUT(ISO7816_TypeDef *ISO7816x)
{
    return (READ_BIT(ISO7816x->STA, ISO7816_STA_CARD_OUT_FLAG) == (ISO7816_STA_CARD_OUT_FLAG));
}

/**
 * @brief  Clear Card Out Detection flag.
 * @note   write one to Clear.
 * @param  ISO7816x ISO7816 Instance.
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_ClearFlag_CARDOUT(ISO7816_TypeDef * ISO7816x)
{
    WRITE_REG(ISO7816x->STA, ISO7816_STA_CARD_OUT_FLAG);
}

/**
 * @}
 */

/** @defgroup ISO7816_LL_EF_Data_Management Data_Management
 * @{
 */

/**
 * @brief  Read Receiver Data register (Receive Data value, 8 bits).
 * @param  ISO7816x ISO7816 Instance.
 * @retval Value between Min_Data=0x0 and Max_Data=0xFF
 */
__STATIC_INLINE uint8_t LL_ISO7816_ReceiveData8(ISO7816_TypeDef *ISO7816x)
{
    return (uint8_t)(READ_BIT(ISO7816x->DATA, ISO7816_DATA_DAT) >> ISO7816_DATA_DAT_POS);
}

/**
 * @brief  Read Receiver Data register (Receive Data value, 9 bits, bit8 is a parity by user defined)
 * @param  ISO7816x ISO7816 Instance.
 * @retval Value between Min_Data=0x0 and Max_Data=0x1FF
 */
__STATIC_INLINE uint16_t LL_ISO7816_ReceiveData9(ISO7816_TypeDef *ISO7816x)
{
    return (uint16_t)(READ_REG(ISO7816x->DATA) >> ISO7816_DATA_DAT_POS);
}

/**
 * @brief  Write in Transmitter Data Register (Transmit Data value, 8 bits).
 * @param  ISO7816x ISO7816 Instance.
 * @param  Data Value between Min_Data=0x0 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_TransmitData8(ISO7816_TypeDef *ISO7816x, uint8_t Value)
{
    MODIFY_REG(ISO7816x->DATA, ISO7816_DATA_DAT, Value << ISO7816_DATA_DAT_POS);
}

/**
 * @brief  Write in Transmitter Data Register (Transmit Data value, 9 bits, bit8 is a parity by user defined).
 * @param  ISO7816x ISO7816 Instance.
 * @param  Data Value between Min_Data=0x0 and Max_Data=0x1FF
 * @retval None
 */
__STATIC_INLINE void LL_ISO7816_TransmitData9(ISO7816_TypeDef *ISO7816x, uint16_t Value)
{
    WRITE_REG(ISO7816x->DATA, Value << ISO7816_DATA_DAT_POS);
}

/**
 * @}
 */

/** @defgroup ISO7816_LL_EF_Init Initialization and de-initialization functions
 * @{
 */

// The initialization function can have more than one argument.
ErrorStatus LL_ISO7816_Init(ISO7816_TypeDef *ISO7816x, LL_ISO7816_InitTypeDef *ISO7816_InitStruct);
ErrorStatus LL_ISO7816_DeInit(ISO7816_TypeDef *ISO7816x);
void LL_ISO7816_StructInit(LL_ISO7816_InitTypeDef *ISO7816_InitStruct); // If the module is not suitable for the default configuration, you could not defines this function.

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#endif /* ISO78160 || ISO78161 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_ISO7816_H */
