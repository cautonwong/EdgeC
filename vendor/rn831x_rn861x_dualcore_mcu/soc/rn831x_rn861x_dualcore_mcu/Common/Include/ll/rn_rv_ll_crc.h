/**
 *****************************************************************************
 * @file   rn_rv_ll_crc.h
 * @author Renergy Software Team
 * @brief  Header file of CRC LL module.
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
#ifndef __RN_LL_CRC_H
#define __RN_LL_CRC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_crc.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (CRC0) || defined (CRC1)

/** @defgroup CRC_LL CRC
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/
/** @defgroup CRC_LL_Exported_Constants CRC Exported Constants
 * @{
 */

/** @defgroup CRC_LL_EC_POLYSIZE Polysize Defines
 * @brief     Polysize defines which can be used with LL_CRC_SetPolynomialSize and LL_CRC_GetPolynomialSize functions.
 * @{
 */
#define LL_CRC_POLYSIZE_BIT32              0x00000000U
#define LL_CRC_POLYSIZE_BIT16              CRC_CTRL_POLYSIZE_CRC16
#define LL_CRC_POLYSIZE_BIT8               CRC_CTRL_POLYSIZE_CRC8
#define LL_CRC_POLYSIZE_BIT7               CRC_CTRL_POLYSIZE_CRC7
/**
 * @}
 */

/** @defgroup CRC_LL_EC_INREVERSE Inversion mode Defines
 * @brief     Inversion mode defines which can be used with LL_CRC_SetInputRevMode and LL_CRC_GetInputRevMode functions.
 * @{
 */
#define LL_CRC_INPUT_NOREVERSE             0x00000000U
#define LL_CRC_INPUT_REVERSE_IN_BYTE       CRC_CTRL_REVIN_BYTE
#define LL_CRC_INPUT_REVERSE_IN_HALFWORD   CRC_CTRL_REVIN_HWORD
#define LL_CRC_INPUT_REVERSE_IN_WORD       CRC_CTRL_REVIN_WORD
/**
 * @}
 */

/** @defgroup CRC_LL_EC_OPDW Operation Data Width Defines
 * @brief     Operation Data Width defines which can be used with LL_CRC_SetDataWidth and LL_CRC_GetDataWidth functions.
 * @{
 */
#define LL_CRC_OPT_DATA_WIDTH_IN_WORD      0x00000000U
#define LL_CRC_OPT_DATA_WIDTH_IN_HALFWORD  CRC_CTRL_OPDW_HWORD
#define LL_CRC_OPT_DATA_WIDTH_IN_BYTE      CRC_CTRL_OPDW_BYTE
/**
 * @}
 */

/** @defgroup CRC_LL_EC_DMA_MODE DMA Mode Defines
 * @brief     DMA Mode defines which can be used with LL_CRC_SetDMAMode and LL_CRC_GetDMAMode functions.
 * @{
 */
#define LL_CRC_DMA_MODE_CRC                0x00000000U
#define LL_CRC_DMA_MODE_CHKSUM             CRC_DMA_CTRL_MODE
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/

/** @defgroup CRC_LL_Exported_Macros CRC Exported Macros
 * @{
 */

/** @defgroup CRC_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */
/**
 * @brief  Write a value in CRC register
 * @param  __INSTANCE__ CRC Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_CRC_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, __VALUE__)

/**
 * @brief  Read a value in CRC register
 * @param  __INSTANCE__ CRC Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_CRC_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup CRC_LL_Exported_Functions CRC Exported Functions
 * @{
 */

/** @defgroup CRC_LL_EF_Data_Management Data_Management
 * @{
 */

/**
 * @brief  Write given data to the CRC calculator.
 * @rmtoll DATA       DR          LL_CRC_FeedData32
 * @param  CRCx CRC instance
 * @param  InData value to be provided to CRC calculator between between Min_Data=0 and Max_Data=0xFFFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_CRC_FeedData32(CRC_TypeDef *CRCx, uint32_t InData)
{
    MODIFY_REG(CRCx->DATA, CRC_DATA_DATA, InData);
}

/**
 * @brief  Return the current CRC calculation result.
 * @rmtoll DATA       DR          LL_CRC_ReadData32
 * @param  CRCx CRC instance
 * @retval Current CRC calculation result as stored in CRC Data register.
 */
__STATIC_INLINE uint32_t LL_CRC_ReadData32(CRC_TypeDef *CRCx)
{
    return (uint32_t)READ_BIT(CRCx->DATA, CRC_DATA_DATA);
}

/**
 * @}
 */

/** @defgroup CRC_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief  Check if the CRC calculation is done or not.
 * @rmtoll STA       DONE          LL_CRC_IsCalculationDone
 * @param  CRCx CRC instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CRC_IsCalculationDone(CRC_TypeDef *CRCx)
{
    return ((READ_BIT(CRCx->STA, CRC_STA_DONE) == CRC_STA_DONE) ? 1UL : 0UL);
}

/**
 * @brief  Set the polynomial size.
 * @rmtoll CTRL       POLYSIZE          LL_CRC_SetPolynomialSize
 * @param  CRCx CRC instance
 * @param  PolynimialSize This parameter can be one of the following values:
 *         @arg @ref LL_CRC_POLYSIZE_BIT32
 *         @arg @ref LL_CRC_POLYSIZE_BIT16
 *         @arg @ref LL_CRC_POLYSIZE_BIT8
 *         @arg @ref LL_CRC_POLYSIZE_BIT7
 * @retval None
 */
__STATIC_INLINE void LL_CRC_SetPolynomialSize(CRC_TypeDef *CRCx, uint32_t PolynomialSize)
{
    MODIFY_REG(CRCx->CTRL, CRC_CTRL_POLYSIZE, PolynomialSize);
}

/**
 * @brief  Get the polynomial size.
 * @rmtoll CTRL       POLYSIZE          LL_CRC_GetPolynomialSize
 * @param  CRCx CRC instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_CRC_POLYSIZE_BIT32
 *         @arg @ref LL_CRC_POLYSIZE_BIT16
 *         @arg @ref LL_CRC_POLYSIZE_BIT8
 *         @arg @ref LL_CRC_POLYSIZE_BIT7
 */
__STATIC_INLINE uint32_t LL_CRC_GetPolynomialSize(CRC_TypeDef *CRCx)
{
    return (uint32_t)(READ_BIT(CRCx->CTRL, CRC_CTRL_POLYSIZE));
}

/**
 * @brief  Enable bitwise xor for output data.
 * @rmtoll CTRL         XOREN         LL_CRC_EnableXor
 * @param  CRCx CRC instance
 * @retval None
 */
__STATIC_INLINE void LL_CRC_EnableXor(CRC_TypeDef *CRCx)
{
    SET_BIT(CRCx->CTRL, CRC_CTRL_XOR);
}

/**
 * @brief  Disable bitwise xor for output data.
 * @rmtoll CTRL         XOREN         LL_CRC_DisableXor
 * @param  CRCx CRC instance
 * @retval None
 */
__STATIC_INLINE void LL_CRC_DisableXor(CRC_TypeDef *CRCx)
{
    CLEAR_BIT(CRCx->CTRL, CRC_CTRL_XOR);
}

/**
 * @brief  Check if bitwise xor for output data is enabled or not.
 * @rmtoll CTRL         XOREN         LL_CRC_IsEnabledXor
 * @param  CRCx CRC instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CRC_IsEnabledXor(CRC_TypeDef *CRCx)
{
    return ((READ_BIT(CRCx->CTRL, CRC_CTRL_XOR) == CRC_CTRL_XOR) ? 1UL : 0UL);
}

/**
 * @brief  Set the reverse mode for input data.
 * @rmtoll CTRL         REVIN         LL_CRC_SetInputRevMode
 * @param  CRCx CRC instance
 * @param  Mode This parameter can be one of the following values:
 *         @arg @ref LL_CRC_INPUT_NOTREVERSE
 *         @arg @ref LL_CRC_INPUT_REVERSE_IN_BYTE
 *         @arg @ref LL_CRC_INPUT_REVERSE_IN_HALFWORD
 *         @arg @ref LL_CRC_INPUT_REVERSE_IN_WORD
 * @retval None
 */
__STATIC_INLINE void LL_CRC_SetInputRevMode(CRC_TypeDef *CRCx, uint32_t Mode)
{
    MODIFY_REG(CRCx->CTRL, CRC_CTRL_REVIN, Mode);
}

/**
 * @brief  Get the reverse mode for input data.
 * @rmtoll CTRL         REVIN         LL_CRC_GetInputRevMode
 * @param  CRCx CRC instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_CRC_INPUT_NOTREVERSE
 *         @arg @ref LL_CRC_INPUT_REVERSE_IN_BYTE
 *         @arg @ref LL_CRC_INPUT_REVERSE_IN_HALFWORD
 *         @arg @ref LL_CRC_INPUT_REVERSE_IN_WORD
 */
__STATIC_INLINE uint32_t LL_CRC_GetInputRevMode(CRC_TypeDef *CRCx)
{
    return (READ_BIT(CRCx->CTRL, CRC_CTRL_REVIN));
}

/**
 * @brief  Enable reverse output data.
 * @rmtoll CTRL         REVOUT         LL_CRC_EnableOutputReverse
 * @param  CRCx CRC instance
 * @retval None
 */
__STATIC_INLINE void LL_CRC_EnableOutputReverse(CRC_TypeDef *CRCx)
{
    SET_BIT(CRCx->CTRL, CRC_CTRL_REVOUT);
}

/**
 * @brief  Disable reverse output data.
 * @rmtoll CTRL         REVOUT         LL_CRC_DisableOutputReverse
 * @param  CRCx CRC instance
 * @retval None
 */
__STATIC_INLINE void LL_CRC_DisableOutputReverse(CRC_TypeDef *CRCx)
{
    CLEAR_BIT(CRCx->CTRL, CRC_CTRL_REVOUT);
}

/**
 * @brief  Check if reverse output data is enabled or not.
 * @rmtoll CTRL         REVOUT         LL_CRC_IsEnabledOutputReverse
 * @param  CRCx CRC instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CRC_IsEnabledOutputReverse(CRC_TypeDef *CRCx)
{
    return ((READ_BIT(CRCx->CTRL, CRC_CTRL_REVOUT) == CRC_CTRL_REVOUT) ? 1UL : 0UL);
}

/**
 * @brief  Set the data width of CRC calculation.
 * @rmtoll CTRL         OPDW         LL_CRC_SetDataWidth
 * @param  CRCx CRC instance
 * @param  DataWidth This parameter can be one of the following values:
 *         @arg @ref LL_CRC_OPT_DATA_WIDTH_IN_WORD
 *         @arg @ref LL_CRC_OPT_DATA_WIDTH_IN_HALFWORD
 *         @arg @ref LL_CRC_OPT_DATA_WIDTH_IN_BYTE
 * @retval None
 */
__STATIC_INLINE void LL_CRC_SetDataWidth(CRC_TypeDef *CRCx, uint32_t DataWidth)
{
    MODIFY_REG(CRCx->CTRL, CRC_CTRL_OPDW, DataWidth);
}

/**
 * @brief  Get the data width of CRC calculation.
 * @rmtoll CTRL         OPDW         LL_CRC_GetDataWidth
 * @param  CRCx CRC instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_CRC_OPT_DATA_WIDTH_IN_WORD
 *         @arg @ref LL_CRC_OPT_DATA_WIDTH_IN_HALFWORD
 *         @arg @ref LL_CRC_OPT_DATA_WIDTH_IN_BYTE
 */
__STATIC_INLINE uint32_t LL_CRC_GetDataWidth(CRC_TypeDef *CRCx)
{
    return (READ_BIT(CRCx->CTRL, CRC_CTRL_OPDW));
}

/**
 * @brief  Set init value.
 * @rmtoll INIT         VAL         LL_CRC_SetInitValue
 * @param  CRCx CRC instance
 * @param  InitValue CRC init value.
 * @retval None
 */
__STATIC_INLINE void LL_CRC_SetInitValue(CRC_TypeDef *CRCx, uint32_t InitValue)
{
    MODIFY_REG(CRCx->INIT, CRC_INIT_VAL, InitValue);
}

/**
 * @brief  Get init value.
 * @rmtoll INIT         VAL         LL_CRC_GetInitValue
 * @param  CRCx CRC instance
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_CRC_GetInitValue(CRC_TypeDef *CRCx)
{
    return (READ_BIT(CRCx->INIT, CRC_INIT_VAL));
}

/**
 * @brief  Set CRC polynomial.
 * @rmtoll POL         VAL         LL_CRC_SetPolynomial
 * @param  CRCx CRC instance
 * @param  Polynomial CRC polynomial.
 * @retval None
 */
__STATIC_INLINE void LL_CRC_SetPolynomial(CRC_TypeDef *CRCx, uint32_t Polynomial)
{
    MODIFY_REG(CRCx->POL, CRC_POL_VAL, Polynomial);
}

/**
 * @brief  Get CRC polynomial.
 * @rmtoll POL         VAL         LL_CRC_GetPolynomial
 * @param  CRCx CRC instance
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_CRC_GetPolynomial(CRC_TypeDef *CRCx)
{
    return (READ_BIT(CRCx->POL, CRC_POL_VAL));
}

/**
 * @brief  Set ExclusiveOR Value.
 * @rmtoll XOR         VAL         LL_CRC_SetExclusiveORValue
 * @param  CRCx CRC instance
 * @param  XorValue Specifies ExclusiveOR Value.
 * @retval None
 */
__STATIC_INLINE void LL_CRC_SetExclusiveORValue(CRC_TypeDef *CRCx, uint32_t XorValue)
{
    MODIFY_REG(CRCx->XOR, CRC_XOR_VAL, XorValue);
}

/**
 * @brief  Get ExclusiveOR Value.
 * @rmtoll XOR         VAL         LL_CRC_GetExclusiveORValue
 * @param  CRCx CRC instance
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_CRC_GetExclusiveORValue(CRC_TypeDef *CRCx)
{
    return (READ_BIT(CRCx->XOR, CRC_XOR_VAL));
}

/**
 * @brief  Set CRC DMA running mode, only valid in CRC0.
 * @rmtoll DMA_CTRL         MODE         LL_CRC_SetDMAMode
 * @param  CRCx CRC instance
 * @param  Mode This parameter can be one of the following values:
 *         @arg @ref LL_CRC_DMA_MODE_CRC
 *         @arg @ref LL_CRC_DMA_MODE_CHKSUM
 * @retval None
 */
__STATIC_INLINE void LL_CRC_SetDMAMode(CRC_TypeDef *CRCx, uint32_t Mode)
{
    MODIFY_REG(CRCx->DMA_CTRL, CRC_DMA_CTRL_MODE, Mode);
}

/**
 * @brief  Get CRC DMA running mode, only valid in CRC0.
 * @rmtoll DMA_CTRL         MODE         LL_CRC_GetDMAMode
 * @param  CRCx CRC instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_CRC_DMA_MODE_CRC
 *         @arg @ref LL_CRC_DMA_MODE_CHKSUM
 */
__STATIC_INLINE uint32_t LL_CRC_GetDMAMode(CRC_TypeDef *CRCx)
{
    return (uint32_t)(READ_BIT(CRCx->DMA_CTRL, CRC_DMA_CTRL_MODE));
}

/**
 * @brief  Enable CRC DMA, only valid in CRC0.
 * @rmtoll DMA_CTRL         EN         LL_CRC_EnableDMA
 * @param  CRCx CRC instance
 * @retval None
 */
__STATIC_INLINE void LL_CRC_EnableDMA(CRC_TypeDef *CRCx)
{
    SET_BIT(CRCx->DMA_CTRL, CRC_DMA_CTRL_EN);
}

/**
 * @brief  Disable CRC DMA, only valid in CRC0.
 * @rmtoll DMA_CTRL         EN         LL_CRC_DisableDMA
 * @param  CRCx CRC instance
 * @retval None
 */
__STATIC_INLINE void LL_CRC_DisableDMA(CRC_TypeDef *CRCx)
{
    CLEAR_BIT(CRCx->DMA_CTRL, CRC_DMA_CTRL_EN);
}

/**
 * @brief  Check if CRC DMA is enabled or not, only valid in CRC0.
 * @rmtoll DMA_CTRL         EN         LL_CRC_IsEnabledDMA
 * @param  CRCx CRC instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_CRC_IsEnabledDMA(CRC_TypeDef *CRCx)
{
    return ((READ_BIT(CRCx->DMA_CTRL, CRC_DMA_CTRL_EN) == CRC_DMA_CTRL_EN) ? 1UL : 0UL);
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup CRC_LL_EF_Init Initialization and de-initialization functions
 * @{
 */
ErrorStatus LL_CRC_DeInit(CRC_TypeDef *CRCx);
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

#endif /* CRC0 || CRC1 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_CRC_H */
