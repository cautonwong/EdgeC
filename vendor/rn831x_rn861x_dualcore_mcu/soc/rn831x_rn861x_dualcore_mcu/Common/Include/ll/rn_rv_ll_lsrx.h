/**
 ******************************************************************************
 * @file   rn_rv_ll_lsrx.h
 * @author Renergy Software Team
 * @brief
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
#ifndef __RN_RV_LL_LSRX_H
#define __RN_RV_LL_LSRX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_lsrx.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined(LSRX)

/** @defgroup LSRX_LL LSRX
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup LSRX_LL_Exported_Types LSRX Exported Types
 * @{
 */

/**
 * @brief LL LSRX Init Structure definition
 */
typedef struct
{
    uint32_t Threshold;     /*!< Specifies the LSRX receive timeout threshold to be configured. */

    uint32_t ParityData;    /*!< Specifies the LSRX parity data to be configured. */

    uint32_t WaveCycle;     /*!< Specifies the calibration wave output cycle to be configured. */

    uint32_t ChnlSel0;     /*!< Specifies the LSRX channel0 selection.
                                This parameter can be any value of @ref LSRX_LL_EC_CHSEL */

    uint32_t ChnlSel1;     /*!< Specifies the LSRX channel1 selection.
                                This parameter can be any value of @ref LSRX_LL_EC_CHSEL */

    uint32_t ChnlSel2;     /*!< Specifies the LSRX channel2 selection.
                                This parameter can be any value of @ref LSRX_LL_EC_CHSEL */

    uint32_t ChnlSel3;     /*!< Specifies the LSRX channel3 selection.
                                This parameter can be any value of @ref LSRX_LL_EC_CHSEL */

    uint32_t ChnlRxEn0;    /*!< Specifies channel0 receive enable.
                                This parameter can be any value of @ref LSRX_LL_EC_RXEN */

    uint32_t ChnlRxEn1;    /*!< Specifies channel1 receive enable.
                                This parameter can be any value of @ref LSRX_LL_EC_RXEN */

    uint32_t ChnlRxEn2;    /*!< Specifies channel2 receive enable.
                                This parameter can be any value of @ref LSRX_LL_EC_RXEN */

    uint32_t ChnlRxEn3;    /*!< Specifies channel3 receive enable.
                                This parameter can be any value of @ref LSRX_LL_EC_RXEN */
} LL_LSRX_InitTypeDef;
/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/* Exported constants -------------------------------------------------------*/
/** @defgroup LSRX_LL_Exported_Constants LSRX Exported Constants
 * @{
 */

/** @defgroup LSRX_LL_EC_CHSEL LSRX Channel select
 * @{
 */
#define LL_LSRX_CHSEL_UART0                    (0x0000000U)  /*!< Select UART0 as channel */
#define LL_LSRX_CHSEL_UART1                    (0x0000001U)  /*!< Select UART1 as channel */
#define LL_LSRX_CHSEL_UART2                    (0x0000002U)  /*!< Select UART2 as channel */
#define LL_LSRX_CHSEL_UART3                    (0x0000003U)  /*!< Select UART3 as channel */
#define LL_LSRX_CHSEL_UART4                    (0x0000004U)  /*!< Select UART4 as channel */
#define LL_LSRX_CHSEL_UART5                    (0x0000005U)  /*!< Select UART5 as channel */
#define LL_LSRX_CHSEL_UART6                    (0x0000006U)  /*!< Select UART6 as channel */
#define LL_LSRX_CHSEL_UART7                    (0x0000007U)  /*!< Select UART7 as channel */
/**
 * @}
 */

/** @defgroup LSRX_LL_EC_RXEN    LSRX channel receive
 * @{
 */
#define LL_LSRX_RXEN_DISBALE     (0x00000000U)  //Disable LSRX receive
#define LL_LSRX_RXEN_ENABLE      (0x00000001U)  //Enable LSRX receive
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @defgroup LSRX_LL_Exported_Functions LSRX Exported Functions
 * @{
 */

/** @defgroup LSRX_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief Configure identify frame header data.
 * @note  Warning: If enable LSRX moudule, identify frame header data can not write.
 * @rmtoll FTOP        FTOP         LL_LSRX_SetFrameHeader
 * @param LSRXx       LSRX instance
 * @param FrameHeader Frame header data, 32bit
 * @return None
 */
__STATIC_INLINE void LL_LSRX_SetFrameHeader(LSRX_TypeDef *LSRXx, uint32_t FrameHeader)
{
    WRITE_REG(LSRXx->FTOP, FrameHeader);
}

/**
 * @brief  Return identify frame header data.
 * @rmtoll FTOP        FTOP         LL_LSRX_GetFrameHeader
 * @param  LSRXx LSRX instance
 * @retval Frame header data
 */
__STATIC_INLINE uint32_t LL_LSRX_GetFrameHeader(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)READ_REG(LSRXx->FTOP);
}

/**
 * @brief  Enable the fourth byte parity function of frame header.
 * @note   Byte parity refers to pariting in bytes as a unit.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B3PRTEN           LL_LSRX_EnableParityByte3
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableParityByte3(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->FHD, LSRX_FHD_B3PRTEN);
}

/**
 * @brief  Disable the fourth byte parity function of frame header.
 * @note   Byte parity refers to pariting in bytes as a unit.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B3PRTEN           LL_LSRX_DisableParityByte3
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableParityByte3(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->FHD, LSRX_FHD_B3PRTEN);
}

/**
 * @brief  Return if the fourth byte parity function of frame header is enable or disable.
 * @note   Byte parity refers to pariting in bytes as a unit.
 * @rmtoll FHD         LSRX_B3PRTEN           LL_LSRX_IsEnabledParityByte3
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledParityByte3(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->FHD, LSRX_FHD_B3PRTEN) == LSRX_FHD_B3PRTEN);
}

/**
 * @brief  Enable the fourth byte check function of frame header.
 * @note   Byte check refers to checking whether the corresponding bytes are equal to
 *         the corresponding bytes of LSRX_FTOP.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B3CHKEN           LL_LSRX_EnableCheckByte3
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableCheckByte3(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->FHD, LSRX_FHD_B3CHKEN);
}

/**
 * @brief  Disable the fourth byte check function of frame header.
 * @note   Byte check refers to checking whether the corresponding bytes are equal to
 *         the corresponding bytes of LSRX_FTOP.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B3CHKEN           LL_LSRX_DisableCheckByte3
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableCheckByte3(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->FHD, LSRX_FHD_B3CHKEN);
}

/**
 * @brief  Return if the fourth byte check function of frame header is enable or disable.
 * @note   Byte check refers to checking whether the corresponding bytes are equal to
 *         the corresponding bytes of LSRX_FTOP.
 * @rmtoll FHD         LSRX_B3CHKEN           LL_LSRX_IsEnabledCheckByte3
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledCheckByte3(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->FHD, LSRX_FHD_B3CHKEN) == LSRX_FHD_B3CHKEN);
}

/**
 * @brief  Enable the third byte parity function of frame header.
 * @note   Byte parity refers to pariting in bytes as a unit.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B2PRTEN           LL_LSRX_EnableParityByte2
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableParityByte2(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->FHD, LSRX_FHD_B2PRTEN);
}

/**
 * @brief  Disable the third byte parity function of frame header.
 * @note   Byte parity refers to pariting in bytes as a unit.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B2PRTEN           LL_LSRX_DisableParityByte2
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableParityByte2(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->FHD, LSRX_FHD_B2PRTEN);
}

/**
 * @brief  Return if the third byte parity function of frame header is enable or disable.
 * @note   Byte parity refers to pariting in bytes as a unit.
 * @rmtoll FHD         LSRX_B2PRTEN           LL_LSRX_IsEnabledParityByte2
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledParityByte2(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->FHD, LSRX_FHD_B2PRTEN) == LSRX_FHD_B2PRTEN);
}

/**
 * @brief  Enable the third byte check function of frame header.
 * @note   Byte check refers to checking whether the corresponding bytes are equal to
 *         the corresponding bytes of LSRX_FTOP.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B2CHKEN           LL_LSRX_EnableCheckByte2
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableCheckByte2(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->FHD, LSRX_FHD_B2CHKEN);
}

/**
 * @brief  Disable the third byte check function of frame header.
 * @note   Byte check refers to checking whether the corresponding bytes are equal to
 *         the corresponding bytes of LSRX_FTOP.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B2CHKEN           LL_LSRX_DisableCheckByte2
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableCheckByte2(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->FHD, LSRX_FHD_B2CHKEN);
}

/**
 * @brief  Return if the third byte check function of frame header is enable or disable.
 * @note   Byte check refers to checking whether the corresponding bytes are equal to
 *         the corresponding bytes of LSRX_FTOP.
 * @rmtoll FHD         LSRX_B2CHKEN           LL_LSRX_IsEnabledCheckByte2
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledCheckByte2(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->FHD, LSRX_FHD_B2CHKEN) == LSRX_FHD_B2CHKEN);
}

/**
 * @brief  Enable the second byte parity function of frame header.
 * @note   Byte parity refers to pariting in bytes as a unit.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B1PRTEN           LL_LSRX_EnableParityByte1
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableParityByte1(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->FHD, LSRX_FHD_B1PRTEN);
}

/**
 * @brief  Disable the second byte parity function of frame header.
 * @note   Byte parity refers to pariting in bytes as a unit.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B1PRTEN           LL_LSRX_DisableParityByte1
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableParityByte1(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->FHD, LSRX_FHD_B1PRTEN);
}

/**
 * @brief  Return if the second byte parity function of frame header is enable or disable.
 * @note   Byte parity refers to pariting in bytes as a unit.
 * @rmtoll FHD         LSRX_B1PRTEN           LL_LSRX_IsEnabledParityByte1
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledParityByte1(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->FHD, LSRX_FHD_B1PRTEN) == LSRX_FHD_B1PRTEN);
}

/**
 * @brief  Enable the second byte check function of frame header.
 * @note   Byte check refers to checking whether the corresponding bytes are equal to
 *         the corresponding bytes of LSRX_FTOP.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B1CHKEN           LL_LSRX_EnableCheckByte1
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableCheckByte1(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->FHD, LSRX_FHD_B1CHKEN);
}

/**
 * @brief  Disable the second byte check function of frame header.
 * @note   Byte check refers to checking whether the corresponding bytes are equal to
 *         the corresponding bytes of LSRX_FTOP.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B1CHKEN           LL_LSRX_DisableCheckByte1
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableCheckByte1(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->FHD, LSRX_FHD_B1CHKEN);
}

/**
 * @brief  Return if the second byte check function of frame header is enable or disable.
 * @note   Byte check refers to checking whether the corresponding bytes are equal to
 *         the corresponding bytes of LSRX_FTOP.
 * @rmtoll FHD         LSRX_B1CHKEN           LL_LSRX_IsEnabledCheckByte1
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledCheckByte1(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->FHD, LSRX_FHD_B1CHKEN) == LSRX_FHD_B1CHKEN);
}

/**
 * @brief  Enable the first byte parity function of frame header.
 * @note   Byte parity refers to pariting in bytes as a unit.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B0PRTEN           LL_LSRX_EnableParityByte0
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableParityByte0(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->FHD, LSRX_FHD_B0PRTEN);
}

/**
 * @brief  Disable the first byte parity function of frame header.
 * @note   Byte parity refers to pariting in bytes as a unit.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B0PRTEN           LL_LSRX_DisableParityByte0
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableParityByte0(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->FHD, LSRX_FHD_B0PRTEN);
}

/**
 * @brief  Return if the first byte parity function of frame header is enable or disable.
 * @note   Byte parity refers to pariting in bytes as a unit.
 * @rmtoll FHD         LSRX_B0PRTEN           LL_LSRX_IsEnabledParityByte0
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledParityByte0(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->FHD, LSRX_FHD_B0PRTEN) == LSRX_FHD_B0PRTEN);
}

/**
 * @brief  Enable the first byte check function of frame header.
 * @note   Byte check refers to checking whether the corresponding bytes are equal to
 *         the corresponding bytes of LSRX_FTOP.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B0CHKEN           LL_LSRX_EnableCheckByte0
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableCheckByte0(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->FHD, LSRX_FHD_B0CHKEN);
}

/**
 * @brief  Disable the first byte check function of frame header.
 * @note   Byte check refers to checking whether the corresponding bytes are equal to
 *         the corresponding bytes of LSRX_FTOP.
 * @note   1: enable, 0: disable
 * @rmtoll FHD         LSRX_B0CHKEN           LL_LSRX_DisableCheckByte0
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableCheckByte0(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->FHD, LSRX_FHD_B0CHKEN);
}

/**
 * @brief  Return if the first byte check function of frame header is enable or disable.
 * @note   Byte check refers to checking whether the corresponding bytes are equal to
 *         the corresponding bytes of LSRX_FTOP.
 * @rmtoll FHD         LSRX_B0CHKEN           LL_LSRX_IsEnabledCheckByte0
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledCheckByte0(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->FHD, LSRX_FHD_B0CHKEN) == LSRX_FHD_B0CHKEN);
}

/**
 * @brief Configure timeout threshold.
 * @note  Threshold time is used to dextermine whether a data frame has timed out and not been received.
 * @note  When LSRX_CHLx does not receive data, the timing starts from the LSRX module enable.
 * @note  When LSRX_CHLx receive frame header, the timing starts from the arrival time of the frame header.
 * @note  If the frame header reception is not checked after the threshold time,
 *        CHLx reception timeout interrput occurs.
 * @note  Threshold time = (Time + 1)*8.888888ms
 * @rmtoll CTL        TIME_TH         LL_LSRX_SetThresholdTime
 * @param LSRXx LSRX instance
 * @param Time  timeout threshold, 5bit, 0<=Time<=31
 * @return None
 */
__STATIC_INLINE void LL_LSRX_SetThresholdTime(LSRX_TypeDef *LSRXx, uint32_t Time)
{
    MODIFY_REG(LSRXx->CTL, LSRX_CTL_TIME_TH, (Time << LSRX_CTL_TIME_TH_POS));
}

/**
 * @brief Return timeout threshold.
 * @note  Threshold time is used to dextermine whether a data frame has timed out and not been received.
 * @note  When LSRX_CHLx does not receive data, the timing starts from the LSRX module enable.
 * @note  When LSRX_CHLx receive frame header, the timing starts from the arrival time of the frame header.
 * @note  If the frame header reception is not checked after the threshold time,
 *        CHLx reception timeout interrput occurs.
 * @rmtoll CTL        TIME_TH         LL_LSRX_GetThresholdTime
 * @param  LSRXx LSRX instance
 * @retval timeout threshold
 */
__STATIC_INLINE uint32_t LL_LSRX_GetThresholdTime(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->CTL, LSRX_CTL_TIME_TH) >> LSRX_CTL_TIME_TH_POS);
}

/**
 * @brief  Enable LSRX channel3 receive function.
 * @note   0: disable, 1: enable
 * @rmtoll CTL   CHL3EN    LL_LSRX_EnableChannel3
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableChannel3(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->CTL, LSRX_CTL_CHL3EN);
}

/**
 * @brief  Disable LSRX channel3 receive function.
 * @note   0: disable, 1: enable
 * @rmtoll CTL   CHL3EN    LL_LSRX_DisableChannel3
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableChannel3(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->CTL, LSRX_CTL_CHL3EN);
}

/**
 * @brief  Return if LSRX channel3 receive function is enable or disable.
 * @rmtoll CTL   CHL3EN    LL_LSRX_IsEnabledChannel3
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledChannel3(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->CTL, LSRX_CTL_CHL3EN) == LSRX_CTL_CHL3EN);
}

/**
 * @brief  Enable LSRX channel2 receive function.
 * @note   0: disable, 1: enable
 * @rmtoll CTL   CHL2EN    LL_LSRX_EnableChannel2
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableChannel2(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->CTL, LSRX_CTL_CHL2EN);
}

/**
 * @brief  Disable LSRX channel2 receive function.
 * @note   0: disable, 1: enable
 * @rmtoll CTL   CHL2EN    LL_LSRX_DisableChannel2
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableChannel2(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->CTL, LSRX_CTL_CHL2EN);
}

/**
 * @brief  Return if LSRX channel2 receive function is enable or disable.
 * @rmtoll CTL   CHL2EN    LL_LSRX_IsEnabledChannel2
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledChannel2(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->CTL, LSRX_CTL_CHL2EN) == LSRX_CTL_CHL2EN);
}

/**
 * @brief  Enable LSRX channel1 receive function.
 * @note   0: disable, 1: enable
 * @rmtoll CTL   CHL1EN    LL_LSRX_EnableChannel1
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableChannel1(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->CTL, LSRX_CTL_CHL1EN);
}

/**
 * @brief  Disable LSRX channel1 receive function.
 * @note   0: disable, 1: enable
 * @rmtoll CTL   CHL1EN    LL_LSRX_DisableChannel1
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableChannel1(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->CTL, LSRX_CTL_CHL1EN);
}

/**
 * @brief  Return if LSRX channel1 receive function is enable or disable.
 * @rmtoll CTL   CHL1EN    LL_LSRX_IsEnabledChannel1
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledChannel1(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->CTL, LSRX_CTL_CHL1EN) == LSRX_CTL_CHL1EN);
}

/**
 * @brief  Enable LSRX channel0 receive function.
 * @note   0: disable, 1: enable
 * @rmtoll CTL   CHL0EN    LL_LSRX_EnableChannel0
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableChannel0(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->CTL, LSRX_CTL_CHL0EN);
}

/**
 * @brief  Disable LSRX channel0 receive function.
 * @note   0: disable, 1: enable
 * @rmtoll CTL   CHL0EN    LL_LSRX_DisableChannel0
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableChannel0(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->CTL, LSRX_CTL_CHL0EN);
}

/**
 * @brief  Return if LSRX channel0 receive function is enable or disable.
 * @rmtoll CTL   CHL0EN    LL_LSRX_IsEnabledChannel0
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledChannel0(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->CTL, LSRX_CTL_CHL0EN) == LSRX_CTL_CHL0EN);
}

/**
 * @brief  Enable LSRX idle detect.
 * @note   0: disable, 1: enable
 * @rmtoll CTL   IDLE_DET    LL_LSRX_EnableIdleDetect
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableIdleDetect(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->CTL, LSRX_CTL_IDLE_DET);
}

/**
 * @brief  Disable LSRX idle detect.
 * @note   0: disable, 1: enable
 * @rmtoll CTL   IDLE_DET    LL_LSRX_DisableIdleDetect
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableIdleDetect(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->CTL, LSRX_CTL_IDLE_DET);
}

/**
 * @brief  Return if LSRX idle detect is enable or disable.
 * @rmtoll CTL   IDLE_DET    LL_LSRX_IsEnabledIdleDetect
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledIdleDetect(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->CTL, LSRX_CTL_IDLE_DET) == LSRX_CTL_IDLE_DET);
}

/**
 * @brief Configure channel3.
 * @rmtoll CTL    CHL3_SEL    LL_LSRX_SetChannel3
 * @param LSRXx LSRX instance
 * @param Uartx This parameter can be one of the following values:
 *        @arg @ref LL_LSRX_CHSEL_UART0
 *        @arg @ref LL_LSRX_CHSEL_UART1
 *        @arg @ref LL_LSRX_CHSEL_UART2
 *        @arg @ref LL_LSRX_CHSEL_UART3
 *        @arg @ref LL_LSRX_CHSEL_UART4
 *        @arg @ref LL_LSRX_CHSEL_UART5
 *        @arg @ref LL_LSRX_CHSEL_UART6
 *        @arg @ref LL_LSRX_CHSEL_UART7
 * @return None
 */
__STATIC_INLINE void LL_LSRX_SetChannel3(LSRX_TypeDef *LSRXx, uint32_t Uartx)
{
    MODIFY_REG(LSRXx->CTL, LSRX_CTL_CHL3_SEL, (Uartx << LSRX_CTL_CHL3_SEL_POS));
}

/**
 * @brief Return channel3 configuration.
 * @rmtoll CTL    CHL3_SEL    LL_LSRX_GetChannel3
 * @param  LSRXx LSRX instance
 * @retval Returned value can be one of the following values:
 *        @arg @ref LL_LSRX_CHSEL_UART0
 *        @arg @ref LL_LSRX_CHSEL_UART1
 *        @arg @ref LL_LSRX_CHSEL_UART2
 *        @arg @ref LL_LSRX_CHSEL_UART3
 *        @arg @ref LL_LSRX_CHSEL_UART4
 *        @arg @ref LL_LSRX_CHSEL_UART5
 *        @arg @ref LL_LSRX_CHSEL_UART6
 *        @arg @ref LL_LSRX_CHSEL_UART7
 */
__STATIC_INLINE uint32_t LL_LSRX_GetChannel3(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->CTL, LSRX_CTL_CHL3_SEL) >> LSRX_CTL_CHL3_SEL_POS);
}

/**
 * @brief Configure channel2.
 * @rmtoll CTL    CHL2_SEL    LL_LSRX_SetChannel2
 * @param LSRXx LSRX instance
 * @param Uartx This parameter can be one of the following values:
 *        @arg @ref LL_LSRX_CHSEL_UART0
 *        @arg @ref LL_LSRX_CHSEL_UART1
 *        @arg @ref LL_LSRX_CHSEL_UART2
 *        @arg @ref LL_LSRX_CHSEL_UART3
 *        @arg @ref LL_LSRX_CHSEL_UART4
 *        @arg @ref LL_LSRX_CHSEL_UART5
 *        @arg @ref LL_LSRX_CHSEL_UART6
 *        @arg @ref LL_LSRX_CHSEL_UART7
 * @return None
 */
__STATIC_INLINE void LL_LSRX_SetChannel2(LSRX_TypeDef *LSRXx, uint32_t Uartx)
{
    MODIFY_REG(LSRXx->CTL, LSRX_CTL_CHL2_SEL, (Uartx << LSRX_CTL_CHL2_SEL_POS));
}

/**
 * @brief Return channel2 configuration.
 * @rmtoll CTL    CHL2_SEL    LL_LSRX_GetChannel2
 * @param  LSRXx LSRX instance
 * @retval Returned value can be one of the following values:
 *        @arg @ref LL_LSRX_CHSEL_UART0
 *        @arg @ref LL_LSRX_CHSEL_UART1
 *        @arg @ref LL_LSRX_CHSEL_UART2
 *        @arg @ref LL_LSRX_CHSEL_UART3
 *        @arg @ref LL_LSRX_CHSEL_UART4
 *        @arg @ref LL_LSRX_CHSEL_UART5
 *        @arg @ref LL_LSRX_CHSEL_UART6
 *        @arg @ref LL_LSRX_CHSEL_UART7
 */
__STATIC_INLINE uint32_t LL_LSRX_GetChannel2(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->CTL, LSRX_CTL_CHL2_SEL) >> LSRX_CTL_CHL2_SEL_POS);
}

/**
 * @brief Configure channel1.
 * @rmtoll CTL    CHL1_SEL    LL_LSRX_SetChannel1
 * @param LSRXx LSRX instance
 * @param Uartx This parameter can be one of the following values:
 *        @arg @ref LL_LSRX_CHSEL_UART0
 *        @arg @ref LL_LSRX_CHSEL_UART1
 *        @arg @ref LL_LSRX_CHSEL_UART2
 *        @arg @ref LL_LSRX_CHSEL_UART3
 *        @arg @ref LL_LSRX_CHSEL_UART4
 *        @arg @ref LL_LSRX_CHSEL_UART5
 *        @arg @ref LL_LSRX_CHSEL_UART6
 *        @arg @ref LL_LSRX_CHSEL_UART7
 * @return None
 */
__STATIC_INLINE void LL_LSRX_SetChannel1(LSRX_TypeDef *LSRXx, uint32_t Uartx)
{
    MODIFY_REG(LSRXx->CTL, LSRX_CTL_CHL1_SEL, (Uartx << LSRX_CTL_CHL1_SEL_POS));
}

/**
 * @brief Return channel1 configuration.
 * @rmtoll CTL    CHL1_SEL    LL_LSRX_GetChannel1
 * @param  LSRXx LSRX instance
 * @retval Returned value can be one of the following values:
 *        @arg @ref LL_LSRX_CHSEL_UART0
 *        @arg @ref LL_LSRX_CHSEL_UART1
 *        @arg @ref LL_LSRX_CHSEL_UART2
 *        @arg @ref LL_LSRX_CHSEL_UART3
 *        @arg @ref LL_LSRX_CHSEL_UART4
 *        @arg @ref LL_LSRX_CHSEL_UART5
 *        @arg @ref LL_LSRX_CHSEL_UART6
 *        @arg @ref LL_LSRX_CHSEL_UART7
 */
__STATIC_INLINE uint32_t LL_LSRX_GetChannel1(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->CTL, LSRX_CTL_CHL1_SEL) >> LSRX_CTL_CHL1_SEL_POS);
}

/**
 * @brief Configure channel0.
 * @rmtoll CTL    CHL0_SEL    LL_LSRX_SetChannel0
 * @param LSRXx LSRX instance
 * @param Uartx This parameter can be one of the following values:
 *        @arg @ref LL_LSRX_CHSEL_UART0
 *        @arg @ref LL_LSRX_CHSEL_UART1
 *        @arg @ref LL_LSRX_CHSEL_UART2
 *        @arg @ref LL_LSRX_CHSEL_UART3
 *        @arg @ref LL_LSRX_CHSEL_UART4
 *        @arg @ref LL_LSRX_CHSEL_UART5
 *        @arg @ref LL_LSRX_CHSEL_UART6
 *        @arg @ref LL_LSRX_CHSEL_UART7
 * @return None
 */
__STATIC_INLINE void LL_LSRX_SetChannel0(LSRX_TypeDef *LSRXx, uint32_t Uartx)
{
    MODIFY_REG(LSRXx->CTL, LSRX_CTL_CHL0_SEL, (Uartx << LSRX_CTL_CHL0_SEL_POS));
}

/**
 * @brief Return channel0 configuration.
 * @rmtoll CTL    CHL0_SEL    LL_LSRX_GetChannel0
 * @param  LSRXx LSRX instance
 * @retval Returned value can be one of the following values:
 *        @arg @ref LL_LSRX_CHSEL_UART0
 *        @arg @ref LL_LSRX_CHSEL_UART1
 *        @arg @ref LL_LSRX_CHSEL_UART2
 *        @arg @ref LL_LSRX_CHSEL_UART3
 *        @arg @ref LL_LSRX_CHSEL_UART4
 *        @arg @ref LL_LSRX_CHSEL_UART5
 *        @arg @ref LL_LSRX_CHSEL_UART6
 *        @arg @ref LL_LSRX_CHSEL_UART7
 */
__STATIC_INLINE uint32_t LL_LSRX_GetChannel0(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->CTL, LSRX_CTL_CHL0_SEL) >> LSRX_CTL_CHL0_SEL_POS);
}

/**
 * @brief  Configure output parity data.
 * @note   LSRX transmit parity data at baud rate.
 * @note   Every time this register is written, LSRX output a calibration wave.
 * @rmtoll CAL_CFG    PAR_DATA    LL_LSRX_SetParityData
 * @param  LSRXx LSRX instance
 * @param  Data  Output parity data, 8bit, 0<=Data<=0xFF
 * @return None
 */
__STATIC_INLINE void LL_LSRX_SetParityData(LSRX_TypeDef *LSRXx, uint32_t Data)
{
    MODIFY_REG(LSRXx->CAL_CFG, LSRX_CAL_CFG_PAR_DATA, (Data << LSRX_CAL_CFG_PAR_DATA_POS));
}

/**
 * @brief  Return output parity data.
 * @note   LSRX transmit parity data at baud rate.
 * @rmtoll CAL_CFG    PAR_DATA    LL_LSRX_GetParityData
 * @param  LSRXx LSRX instance
 * @retval Output parity data
 */
__STATIC_INLINE uint32_t LL_LSRX_GetParityData(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->CAL_CFG, LSRX_CAL_CFG_PAR_DATA) >> LSRX_CAL_CFG_PAR_DATA_POS);
}

/**
 * @brief  Configure calibration output cycle.
 * @note   Every time this register is written, LSRX output a calibration wave.
 * @note   Calibration cycle = (Count + 1)
 * @rmtoll CAL_CFG    CAL_CNT    LL_LSRX_SetCalibrationCycle
 * @param  LSRXx LSRX instance
 * @param  Count calibration output cycle count, 23bit, 0<=Count<=0x7FFFFF
 * @return None
 */
__STATIC_INLINE void LL_LSRX_SetCalibrationCycle(LSRX_TypeDef *LSRXx, uint32_t Count)
{
    MODIFY_REG(LSRXx->CAL_CFG, LSRX_CAL_CFG_CAL_CNT, Count);
}

/**
 * @brief  Return calibration output cycle.
 * @note   Every time this register is written, LSRX output a calibration wave.
 * @rmtoll CAL_CFG    CAL_CNT    LL_LSRX_GetCalibrationCycle
 * @param  LSRXx LSRX instance
 * @retval Cycle count
 */
__STATIC_INLINE uint32_t LL_LSRX_GetCalibrationCycle(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->CAL_CFG, LSRX_CAL_CFG_CAL_CNT));
}

/**
 * @brief  Enable LSRX.
 * @note   0: disable, 1: enable
 * @rmtoll EN   EN    LL_LSRX_Enable
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_Enable(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->EN, LSRX_EN_EN);
}

/**
 * @brief  Disable LSRX.
 * @note   0: disable, 1: enable
 * @rmtoll EN   EN    LL_LSRX_Disable
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_Disable(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->EN, LSRX_EN_EN);
}

/**
 * @brief  Return if LSRX is enable or disable.
 * @rmtoll EN   EN    LL_LSRX_IsEnabled
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabled(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->EN, LSRX_EN_EN) == LSRX_EN_EN);
}

/**
 * @brief  Enable data update interrupt.
 * @note   0: disable, 1: enable
 * @rmtoll IE   CAL_UPDATA_IE    LL_LSRX_EnableIT_UPDATE
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableIT_UPDATE(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->IE, LSRX_IE_CAL_UPDATE_IE);
}

/**
 * @brief  Disable data update interrupt.
 * @note   0: disable, 1: enable
 * @rmtoll IE   CAL_UPDATA_IE    LL_LSRX_DisableIT_UPDATE
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableIT_UPDATE(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->IE, LSRX_IE_CAL_UPDATE_IE);
}

/**
 * @brief  Return if data update interrupt is enable or disable.
 * @rmtoll IE   CAL_UPDATA_IE    LL_LSRX_IsEnabledIT_UPDATE
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledIT_UPDATE(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->IE, LSRX_IE_CAL_UPDATE_IE) == LSRX_IE_CAL_UPDATE_IE);
}

/**
 * @brief  Enable parity wave transmit complete interrupt.
 * @note   0: disable, 1: enable
 * @rmtoll IE   CAL_FINISH_IE    LL_LSRX_EnableIT_TXCMPL
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_EnableIT_TXCMPL(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->IE, LSRX_IE_CAL_FINISH_IE);
}

/**
 * @brief  Disable parity wave transmit complete interrupt.
 * @note   0: disable, 1: enable
 * @rmtoll IE   CAL_FINISH_IE    LL_LSRX_DisableIT_TXCMPL
 * @param  LSRXx LSRX instance
 * @return None
 */
__STATIC_INLINE void LL_LSRX_DisableIT_TXCMPL(LSRX_TypeDef *LSRXx)
{
    CLEAR_BIT(LSRXx->IE, LSRX_IE_CAL_FINISH_IE);
}

/**
 * @brief  Return if parity wave transmit complete interrupt is enable or disable.
 * @rmtoll IE   CAL_FINISH_IE    LL_LSRX_IsEnabledIT_TXCMPL
 * @param  LSRXx LSRX instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LSRX_IsEnabledIT_TXCMPL(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->IE, LSRX_IE_CAL_FINISH_IE) == LSRX_IE_CAL_FINISH_IE);
}

/**
 * @brief Indicates whether channel3 receive timeout interrupt flag is set.
 * @rmtoll IF   CHL3_Valid_IF   LL_LSRX_IsActiveFlag_RCVTO3
 * @param LSRXx    LSRX Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_LSRX_IsActiveFlag_RCVTO3(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->IF, LSRX_IF_CHL3_VALID_IF));
}

/**
 * @brief Clear channel3 receive timeout interrupt and flag.
 * @rmtoll IF   CHL3_Valid_IF   LL_LSRX_ClearFlag_RCVTO3
 * @param LSRXx    LSRX Instance
 */
__STATIC_INLINE void LL_LSRX_ClearFlag_RCVTO3(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->IF, LSRX_IF_CHL3_VALID_IF);
}

/**
 * @brief Indicates whether channel2 receive timeout interrupt flag is set.
 * @rmtoll IF   CHL2_Valid_IF   LL_LSRX_IsActiveFlag_RCVTO2
 * @param LSRXx    LSRX Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_LSRX_IsActiveFlag_RCVTO2(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->IF, LSRX_IF_CHL2_VALID_IF));
}

/**
 * @brief Clear channel2 receive timeout interrupt and flag.
 * @rmtoll IF   CHL2_Valid_IF   LL_LSRX_ClearFlag_RCVTO2
 * @param LSRXx    LSRX Instance
 */
__STATIC_INLINE void LL_LSRX_ClearFlag_RCVTO2(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->IF, LSRX_IF_CHL2_VALID_IF);
}

/**
 * @brief Indicates whether channel1 receive timeout interrupt flag is set.
 * @rmtoll IF   CHL1_Valid_IF   LL_LSRX_IsActiveFlag_RCVTO1
 * @param LSRXx    LSRX Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_LSRX_IsActiveFlag_RCVTO1(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->IF, LSRX_IF_CHL1_VALID_IF));
}

/**
 * @brief Clear channel1 receive timeout interrupt and flag.
 * @rmtoll IF   CHL1_Valid_IF   LL_LSRX_ClearFlag_RCVTO1
 * @param LSRXx    LSRX Instance
 */
__STATIC_INLINE void LL_LSRX_ClearFlag_RCVTO1(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->IF, LSRX_IF_CHL1_VALID_IF);
}

/**
 * @brief Indicates whether channel0 receive timeout interrupt flag is set.
 * @rmtoll IF   CHL0_Valid_IF   LL_LSRX_IsActiveFlag_RCVTO0
 * @param LSRXx    LSRX Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_LSRX_IsActiveFlag_RCVTO0(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->IF, LSRX_IF_CHL0_VALID_IF));
}

/**
 * @brief Clear channel0 receive timeout interrupt and flag.
 * @rmtoll IF   CHL0_Valid_IF   LL_LSRX_ClearFlag_RCVTO0
 * @param LSRXx    LSRX Instance
 */
__STATIC_INLINE void LL_LSRX_ClearFlag_RCVTO0(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->IF, LSRX_IF_CHL0_VALID_IF);
}

/**
 * @brief Indicates whether channel3 calibration updata interrupt flag is set.
 * @rmtoll IF   CHL3_UPDATA_IF   LL_LSRX_IsActiveFlag_UPDATE3
 * @param LSRXx    LSRX Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_LSRX_IsActiveFlag_UPDATE3(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->IF, LSRX_IF_CHL3_UPDATE_IF));
}

/**
 * @brief Clear channel3 calibration updata interrupt and flag.
 * @rmtoll IF   CHL3_UPDATA_IF   LL_LSRX_ClearFlag_UPDATE3
 * @param LSRXx    LSRX Instance
 */
__STATIC_INLINE void LL_LSRX_ClearFlag_UPDATE3(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->IF, LSRX_IF_CHL3_UPDATE_IF);
}

/**
 * @brief Indicates whether channel2 calibration updata interrupt flag is set.
 * @rmtoll IF   CHL2_UPDATA_IF   LL_LSRX_IsActiveFlag_UPDATE2
 * @param LSRXx    LSRX Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_LSRX_IsActiveFlag_UPDATE2(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->IF, LSRX_IF_CHL2_UPDATE_IF));
}

/**
 * @brief Clear channel2 calibration updata interrupt and flag.
 * @rmtoll IF   CHL2_UPDATA_IF   LL_LSRX_ClearFlag_UPDATE2
 * @param LSRXx    LSRX Instance
 */
__STATIC_INLINE void LL_LSRX_ClearFlag_UPDATE2(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->IF, LSRX_IF_CHL2_UPDATE_IF);
}

/**
 * @brief Indicates whether channel1 calibration updata interrupt flag is set.
 * @rmtoll IF   CHL1_UPDATA_IF   LL_LSRX_IsActiveFlag_UPDATE1
 * @param LSRXx    LSRX Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_LSRX_IsActiveFlag_UPDATE1(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->IF, LSRX_IF_CHL1_UPDATE_IF));
}

/**
 * @brief Clear channel1 calibration updata interrupt and flag.
 * @rmtoll IF   CHL1_UPDATA_IF   LL_LSRX_ClearFlag_UPDATE1
 * @param LSRXx    LSRX Instance
 */
__STATIC_INLINE void LL_LSRX_ClearFlag_UPDATE1(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->IF, LSRX_IF_CHL1_UPDATE_IF);
}

/**
 * @brief Indicates whether channel0 calibration updata interrupt flag is set.
 * @rmtoll IF   CHL0_UPDATA_IF   LL_LSRX_IsActiveFlag_UPDATE0
 * @param LSRXx    LSRX Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_LSRX_IsActiveFlag_UPDATE0(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->IF, LSRX_IF_CHL0_UPDATE_IF));
}

/**
 * @brief Clear channel0 calibration updata interrupt and flag.
 * @rmtoll IF   CHL0_UPDATA_IF   LL_LSRX_ClearFlag_UPDATE0
 * @param LSRXx    LSRX Instance
 */
__STATIC_INLINE void LL_LSRX_ClearFlag_UPDATE0(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->IF, LSRX_IF_CHL0_UPDATE_IF);
}

/**
 * @brief Indicates whether calibration wave transmit complete interrupt flag is set.
 * @rmtoll IF   CAL_FINISH_IF   LL_LSRX_IsActiveFlag_TXCMPL
 * @param LSRXx    LSRX Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_LSRX_IsActiveFlag_TXCMPL(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_BIT(LSRXx->IF, LSRX_IF_CAL_FINISH_IF));
}

/**
 * @brief Clear calibration wave transmit complete interrupt and flag.
 * @rmtoll IF   CAL_FINISH_IF   LL_LSRX_ClearFlag_TXCMPL
 * @param LSRXx    LSRX Instance
 */
__STATIC_INLINE void LL_LSRX_ClearFlag_TXCMPL(LSRX_TypeDef *LSRXx)
{
    SET_BIT(LSRXx->IF, LSRX_IF_CAL_FINISH_IF);
}

/**
 * @brief  Return channel0 frame header time.
 * @note   When detect phase failure, channel0~3 frame header time will be reset to 0.
 * @rmtoll CH0FH_TIME    CH0FH_TIME    LL_LSRX_GetFrameHeaderTime0
 * @param  LSRXx LSRX instance
 * @retval Cycle count
 */
__STATIC_INLINE uint32_t LL_LSRX_GetFrameHeaderTime0(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_REG(LSRXx->CH0FH_TIME));
}

/**
 * @brief  Return channel1 frame header time.
 * @note   When detect phase failure, channel0~3 frame header time will be reset to 0.
 * @rmtoll CH1FH_TIME    CH1FH_TIME    LL_LSRX_GetFrameHeaderTime1
 * @param  LSRXx LSRX instance
 * @retval Cycle count
 */
__STATIC_INLINE uint32_t LL_LSRX_GetFrameHeaderTime1(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_REG(LSRXx->CH1FH_TIME));
}

/**
 * @brief  Return channel2 frame header time.
 * @note   When detect phase failure, channel0~3 frame header time will be reset to 0.
 * @rmtoll CH2FH_TIME    CH2FH_TIME    LL_LSRX_GetFrameHeaderTime2
 * @param  LSRXx LSRX instance
 * @retval Cycle count
 */
__STATIC_INLINE uint32_t LL_LSRX_GetFrameHeaderTime2(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_REG(LSRXx->CH2FH_TIME));
}

/**
 * @brief  Return channel3 frame header time.
 * @note   When detect phase failure, channel0~3 frame header time will be reset to 0.
 * @rmtoll CH3FH_TIME    CH3FH_TIME    LL_LSRX_GetFrameHeaderTime3
 * @param  LSRXx LSRX instance
 * @retval Cycle count
 */
__STATIC_INLINE uint32_t LL_LSRX_GetFrameHeaderTime3(LSRX_TypeDef *LSRXx)
{
    return (uint32_t)(READ_REG(LSRXx->CH3FH_TIME));
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup LSRX_LL_EF_Init Initialization and de-initialization functions
 * @{
 */
ErrorStatus LL_LSRX_DeInit(LSRX_TypeDef *LSRXx);
ErrorStatus LL_LSRX_Init(LSRX_TypeDef *LSRXx, LL_LSRX_InitTypeDef *LSRX_InitStruct);
void        LL_LSRX_StructInit(LL_LSRX_InitTypeDef *LSRX_InitStruct);
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

#endif/* LSRX */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_LL_LSRX_H */
