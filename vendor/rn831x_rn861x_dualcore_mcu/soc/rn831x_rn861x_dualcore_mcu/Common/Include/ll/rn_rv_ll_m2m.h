/**
 *****************************************************************************
 * @file   rn_rv_ll_m2m.h
 * @author Renergy Software Team
 * @brief  Header file of M2M LL module.
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
#ifndef __RN_LL_M2M_H
#define __RN_LL_M2M_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_m2m.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (M2M)

/** @defgroup M2M_LL M2M
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup M2M_LL_ES_INIT M2M Exported Init structure
 * @{
 */

/**
 * @brief  M2M Init Structure definition
 */
typedef struct
{
    uint32_t SrcAddress;           /*!< Specifies the Source base address.
                                        This parameter must be a value between Min_Data = 0 and Max_Data = 0xFFFFFFFF. */

    uint32_t DstAddress;           /*!< Specifies the Destination base address.
                                        This parameter must be a value between Min_Data = 0 and Max_Data = 0xFFFFFFFF. */

    uint32_t SrcDataValidBit;      /*!< Specifies the Source base data valid bit.
                                        This parameter can be a value of @ref M2M_Data_Mode */

    uint32_t DstDataValidBit;      /*!< Specifies the Destination base data valid bit.
                                        This parameter can be a value of @ref M2M_Data_Mode */

    uint32_t DstDataReverseMode;   /*!< Specifies the data reverse mode.
                                        This parameter can be a value of @ref M2M_Reverse_Mode */

    uint32_t DummyEnable;          /*!< Specifies the dummy enable bit.
                                        This parameter must be 0 or 1. */

    uint32_t Dummy;                /*!< Specifies the dummy.
                                        This parameter must be a value between Min_Data = 0 and Max_Data = 0xFFFFFFFF. */

    uint32_t NbData;               /*!< Specifies the number of data to transfer, in data unit.
                                        This parameter must be a value between Min_Data = 0 and Max_Data = 0xFFFFF */

} LL_M2M_InitTypeDef;

/**
 * @}
 */
#endif

/* Exported constants -------------------------------------------------------*/
/** @defgroup M2M_LL_Exported_Constants M2M Exported Constants
 * @{
 */

/** @defgroup M2M_LL_EC_Data_Mode Data Mode
 * @brief     DataMode defines which can be used with LL_M2M_SetInputMode and LL_M2M_GetInputMode functions.
 * @{
 */
#define LL_M2M_DATA_VALID_BYTE0                0x00000001U
#define LL_M2M_DATA_VALID_BYTE1                0x00000002U
#define LL_M2M_DATA_VALID_BYTE2                0x00000004U
#define LL_M2M_DATA_VALID_BYTE3                0x00000008U
#define LL_M2M_DATA_VALID_ALL                  0x0000000FU
/**
 * @}
 */

/** @defgroup M2M_LL_EC_Reverse_Mode Reverse Mode
 * @{
 */
#define LL_M2M_DATA_REV_NONE                   (0x00000000U)                     /*!< Reverse Mode : no reverse */
#define LL_M2M_DATA_REV_BYTEONLY               (M2M_MODE_ORV)                    /*!< Reverse Mode : reverse in byte only */
#define LL_M2M_DATA_REV_ADDR                   (M2M_MODE_ORV | M2M_MODE_ADDRREV) /*!< Reverse Mode : reverse in address, including byte reverse */
#define LL_M2M_DATA_REV_ADDRONLY               (M2M_MODE_ADDRREV)                /*!< Reverse Mode : reverse in address only */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup M2M_LL_Exported_Macros M2M Exported Macros
 * @{
 */

/** @defgroup M2M_LL_EM_WRITE_READ Common Write and read registers macros
 * @{
 */
/**
 * @brief  Write a value in M2M register
 * @param  __INSTANCE__ M2M Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_M2M_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in M2M register
 * @param  __INSTANCE__ M2M Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_M2M_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup M2M_LL_Exported_Functions M2M Exported Functions
 * @{
 */

/** @defgroup M2M_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief  Enable the M2M module.
 * @rmtoll CTRL         EN         LL_M2M_Enable
 * @param  M2Mx M2M instance
 * @retval None
 */
__STATIC_INLINE void LL_M2M_Enable(M2M_TypeDef *M2Mx)
{
    SET_BIT(M2Mx->CTRL, M2M_CTRL_EN);
}

/**
 * @brief  Disable the M2M module.
 * @rmtoll CTRL         EN         LL_M2M_Disable
 * @param  M2Mx M2M instance
 * @retval None
 */
__STATIC_INLINE void LL_M2M_Disable(M2M_TypeDef *M2Mx)
{
    CLEAR_BIT(M2Mx->CTRL, M2M_CTRL_EN);
}

/**
 * @brief  Check if the M2M module is enabled or not.
 * @rmtoll CTRL         EN         LL_M2M_IsEnabled
 * @param  M2Mx M2M instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_M2M_IsEnabled(M2M_TypeDef *M2Mx)
{
    return ((READ_BIT(M2Mx->CTRL, M2M_CTRL_EN) == M2M_CTRL_EN) ? 1UL : 0UL);
}

/**
 * @brief  Set input data mode.
 * @rmtoll MODE         IVLD            LL_M2M_SetInputMode
 * @param  M2Mx M2M instance
 * @param  Mode This parameter can be one of the following values:
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte0
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte1
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte2
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte3
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_ALL
 * @retval None
 */
__STATIC_INLINE void LL_M2M_SetInputMode(M2M_TypeDef *M2Mx, uint32_t Mode)
{
    MODIFY_REG(M2Mx->MODE, M2M_MODE_IVLD, Mode);
}

/**
 * @brief  Get input data mode.
 * @rmtoll MODE         IVLD            LL_M2M_GetInputMode
 * @param  M2Mx M2M instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte0
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte1
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte2
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte3
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_ALL
 */
__STATIC_INLINE uint32_t LL_M2M_GetInputMode(M2M_TypeDef *M2Mx)
{
    return (uint32_t)READ_BIT(M2Mx->MODE, M2M_MODE_IVLD);
}

/**
 * @brief  Set output data mode.
 * @rmtoll MODE         OVLD            LL_M2M_SetOutputMode
 * @param  M2Mx M2M instance
 * @param  Mode This parameter can be one of the following values:
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte0
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte1
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte2
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte3
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_ALL
 * @retval None
 */
__STATIC_INLINE void LL_M2M_SetOutputMode(M2M_TypeDef *M2Mx, uint32_t Mode)
{
    MODIFY_REG(M2Mx->MODE, M2M_MODE_OVLD, Mode);
}

/**
 * @brief  Get output data mode.
 * @rmtoll MODE         OVLD            LL_M2M_GetOutputMode
 * @param  M2Mx M2M instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte0
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte1
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte2
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_Byte3
 *         @arg @ref LL_M2M_DATA_BYTE_VALID_ALL
 */
__STATIC_INLINE uint32_t LL_M2M_GetOutputMode(M2M_TypeDef *M2Mx)
{
    return (uint32_t)READ_BIT(M2Mx->MODE, M2M_MODE_OVLD);
}

/**
 * @brief  Enable output data to be reversed by byte.
 * @rmtoll MODE         ORV_EN         LL_M2M_EnableOutputRevInByte
 * @param  M2Mx M2M instance
 * @retval None
 */
__STATIC_INLINE void LL_M2M_EnableOutputRevByByte(M2M_TypeDef *M2Mx)
{
    SET_BIT(M2Mx->MODE, M2M_MODE_ORV);
}

/**
 * @brief  Disable output data to be reversed by byte.
 * @rmtoll MODE         ORV_EN         LL_M2M_DisableOutputRevByByte
 * @param  M2Mx M2M instance
 * @retval None
 */
__STATIC_INLINE void LL_M2M_DisableOutputRevByByte(M2M_TypeDef *M2Mx)
{
    CLEAR_BIT(M2Mx->MODE, M2M_MODE_ORV);
}

/**
 * @brief  Check if output data to be reversed by byte is enabled or not.
 * @rmtoll MODE         ORV_EN         LL_M2M_IsEnabledOutputRevByByte
 * @param  M2Mx M2M instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_M2M_IsEnabledOutputRevByByte(M2M_TypeDef *M2Mx)
{
    return ((READ_BIT(M2Mx->MODE, M2M_MODE_ORV) == M2M_MODE_ORV) ? 1UL : 0UL);
}

/**
 * @brief  Enable dummy value to work.
 * @rmtoll MODE         DUMMY_EN         LL_M2M_EnableDummy
 * @param  M2Mx M2M instance
 * @retval None
 */
__STATIC_INLINE void LL_M2M_EnableDummy(M2M_TypeDef *M2Mx)
{
    SET_BIT(M2Mx->MODE, M2M_MODE_DUMMY);
}

/**
 * @brief  Disable dummy value to work.
 * @rmtoll MODE         DUMMY_EN         LL_M2M_DisableDummy
 * @param  M2Mx M2M instance
 * @retval None
 */
__STATIC_INLINE void LL_M2M_DisableDummy(M2M_TypeDef *M2Mx)
{
    CLEAR_BIT(M2Mx->MODE, M2M_MODE_DUMMY);
}

/**
 * @brief  Check if dummy value to work is enabled or not.
 * @rmtoll MODE         DUMMY_EN         LL_M2M_IsEnabledDummy
 * @param  M2Mx M2M instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_M2M_IsEnabledDummy(M2M_TypeDef *M2Mx)
{
    return ((READ_BIT(M2Mx->MODE, M2M_MODE_DUMMY) == M2M_MODE_DUMMY) ? 1UL : 0UL);
}

/**
 * @brief  Enable output data to be reversed by address.
 * @rmtoll MODE         ADDR_RV_EN         LL_M2M_EnableOutputRevInAddress
 * @param  M2Mx M2M instance
 * @retval None
 */
__STATIC_INLINE void LL_M2M_EnableOutputRevByAddress(M2M_TypeDef *M2Mx)
{
    SET_BIT(M2Mx->MODE, M2M_MODE_ADDRREV);
}

/**
 * @brief  Disable output data to be reversed by address.
 * @rmtoll MODE         ADDR_RV_EN         LL_M2M_DisableOutputRevByAddress
 * @param  M2Mx M2M instance
 * @retval None
 */
__STATIC_INLINE void LL_M2M_DisableOutputRevByAddress(M2M_TypeDef *M2Mx)
{
    CLEAR_BIT(M2Mx->MODE, M2M_MODE_ADDRREV);
}

/**
 * @brief  Check if output data to be reversed by address is enabled or not.
 * @rmtoll MODE         ADDR_RV_EN         LL_M2M_IsEnabledOutputRevByAddress
 * @param  M2Mx M2M instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_M2M_IsEnabledOutputRevByAddress(M2M_TypeDef *M2Mx)
{
    return ((READ_BIT(M2Mx->MODE, M2M_MODE_ADDRREV) == M2M_MODE_ADDRREV) ? 1UL : 0UL);
}

/**
 * @brief  Set dummy value.
 * @rmtoll DUMMY         VAL            LL_M2M_SetDummyValue
 * @note   The 32bits value corresponds one-to-one to the MODE.OVLD configuration by byte.
 * @param  M2Mx M2M instance
 * @param  Number Between 0 to 0xFFFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_M2M_SetDummyValue(M2M_TypeDef *M2Mx, uint32_t Number)
{
    MODIFY_REG(M2Mx->DUMMY, M2M_DUMMY_VAL, Number);
}

/**
 * @brief  Get dummy value.
 * @rmtoll DUMMY         VAL            LL_M2M_GetDummyValue
 * @param  M2Mx M2M instance
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_M2M_GetDummyValue(M2M_TypeDef *M2Mx)
{
    return (uint32_t)READ_BIT(M2Mx->DUMMY, M2M_DUMMY_VAL);
}

/**
 * @brief  Set source address.
 * @rmtoll SADDR         ADDR            LL_M2M_SetSrcAddress
 * @note   The source data address and the length of source data cannot exceed
 *         the sram address range.
 * @param  M2Mx M2M instance
 * @param  SrcAddress Between 0 to 0x1FFFF.
 * @retval None
 */
__STATIC_INLINE void LL_M2M_SetSrcAddress(M2M_TypeDef *M2Mx, uint32_t SrcAddress)
{
    MODIFY_REG(M2Mx->SADDR, M2M_SADDR_ADDR, SrcAddress);
}

/**
 * @brief  Get source address.
 * @rmtoll SADDR         ADDR            LL_M2M_GetSrcAddress
 * @param  M2Mx M2M instance
 * @retval Between 0 to 0x1FFFF.
 */
__STATIC_INLINE uint32_t LL_M2M_GetSrcAddress(M2M_TypeDef *M2Mx)
{
    return (READ_BIT(M2Mx->SADDR, M2M_SADDR_ADDR) >> M2M_SADDR_ADDR_POS);
}

/**
 * @brief  Set destination address.
 * @rmtoll DADDR         ADDR            LL_M2M_SetDstAddress
 * @note   The destination data address and the length of destination data cannot exceed
 *         the sram address range.
 * @param  M2Mx M2M instance
 * @param  DstAddress Between 0 to 0x1FFFF.
 * @retval None
 */
__STATIC_INLINE void LL_M2M_SetDstAddress(M2M_TypeDef *M2Mx, uint32_t DstAddress)
{
    MODIFY_REG(M2Mx->DADDR, M2M_DADDR_ADDR, DstAddress);
}

/**
 * @brief  Get destination address.
 * @rmtoll DADDR         ADDR            LL_M2M_GetDstAddress
 * @param  M2Mx M2M instance
 * @retval Between 0 to 0x1FFFF.
 */
__STATIC_INLINE uint32_t LL_M2M_GetDstAddress(M2M_TypeDef *M2Mx)
{
    return (READ_BIT(M2Mx->DADDR, M2M_DADDR_ADDR) >> M2M_DADDR_ADDR_POS);
}

/**
 * @brief  Set data transfer length.
 * @rmtoll ILEN         LEN            LL_M2M_SetDataLength
 * @note   The length of source data cannot exceed the size of sram. ILEN=0 and
 *         CTRL.EN=1 cannot be configured.
 * @param  M2Mx M2M instance
 * @param  NbData Between 0 to 0x1FFFF.
 * @retval None
 */
__STATIC_INLINE void LL_M2M_SetDataLength(M2M_TypeDef *M2Mx, uint32_t NbData)
{
    MODIFY_REG(M2Mx->ILEN, M2M_ILEN_LEN, NbData);
}

/**
 * @brief  Get data transfer length.
 * @rmtoll ILEN         LEN            LL_M2M_GetDataLength
 * @param  M2Mx M2M instance
 * @retval Between 0 to 0x1FFFF.
 */
__STATIC_INLINE uint32_t LL_M2M_GetDataLength(M2M_TypeDef *M2Mx)
{
    return (READ_BIT(M2Mx->ILEN, M2M_ILEN_LEN) >> M2M_ILEN_LEN_POS);
}

/**
 * @}
 */

/** @defgroup M2M_LL_EF_FLAG_Management FLAG_Management
 * @{
 */

/**
 * @brief  Check if transfer complete flag is set or not.
 * @rmtoll IF         DONE         LL_M2M_IsActiveFlag
 * @param  M2Mx M2M instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_M2M_IsActiveFlag(M2M_TypeDef *M2Mx)
{
    return ((READ_BIT(M2Mx->IF, M2M_IF_DONE) == M2M_IF_DONE) ? 1UL : 0UL);
}

/**
 * @brief  Clear transfer complete flag.
 * @rmtoll IF         DONE         LL_M2M_ClearFlag
 * @param  M2Mx M2M instance
 * @retval None
 */
__STATIC_INLINE void LL_M2M_ClearFlag(M2M_TypeDef *M2Mx)
{
    WRITE_REG(M2Mx->IF, M2M_IF_DONE);
}

/**
 * @}
 */

/** @defgroup M2M_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable transfer complete interrupt.
 * @rmtoll IE         DONE         LL_M2M_EnableIT
 * @param  M2Mx M2M instance
 * @retval None
 */
__STATIC_INLINE void LL_M2M_EnableIT(M2M_TypeDef *M2Mx)
{
    SET_BIT(M2Mx->IE, M2M_IE_DONE);
}

/**
 * @brief  Disable transfer complete interrupt.
 * @rmtoll IE         DONE         LL_M2M_DisableIT
 * @param  M2Mx M2M instance
 * @retval None
 */
__STATIC_INLINE void LL_M2M_DisableIT(M2M_TypeDef *M2Mx)
{
    CLEAR_BIT(M2Mx->IE, M2M_IE_DONE);
}

/**
 * @brief  Check if transfer complete interrupt is enabled or not.
 * @rmtoll IE         DONE         LL_M2M_IsEnabledIT
 * @param  M2Mx M2M instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_M2M_IsEnabledIT(M2M_TypeDef *M2Mx)
{
    return ((READ_BIT(M2Mx->IE, M2M_IE_DONE) == M2M_IE_DONE) ? 1UL : 0UL);
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup M2M_LL_EF_Init Initialization and de-initialization functions
 * @{
 */
ErrorStatus LL_M2M_Init(M2M_TypeDef *M2Mx, LL_M2M_InitTypeDef *M2M_InitStruct);
ErrorStatus LL_M2M_DeInit(M2M_TypeDef *M2Mx);
void LL_M2M_StructInit(LL_M2M_InitTypeDef *M2M_InitStruct);
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

#endif /* M2M */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_M2M_H */
