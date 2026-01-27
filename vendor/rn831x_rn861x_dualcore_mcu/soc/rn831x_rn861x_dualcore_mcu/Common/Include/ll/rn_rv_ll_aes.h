/**
 ******************************************************************************
 * @file   rn_rv_ll_aes.h
 * @author Renergy Software Team
 * @brief  Header file of AES LL module.
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
#ifndef __RN_LL_AES_H
#define __RN_LL_AES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_aes.h"
#include "rn831x_rn861x_mcu_sea.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (AES)

/** @defgroup AES_LL AES
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/
/** @defgroup AES_LL_Exported_Constants AES Exported Constants
 * @{
 */

/** @defgroup AES_LL_IR_TYPE IR Type Defines
 * @brief     IR Type defines which can be used with LL_AES_SetIRType and LL_AES_GetIRType functions.
 * @{
 */
#define LL_AES_IR_KEYEXP               AES_CTRL_IR_KEYEXP
#define LL_AES_IR_ENCRYPT              AES_CTRL_IR_ENC
#define LL_AES_IR_DECRYPT              AES_CTRL_IR_DEC
#define LL_AES_IR_GF128                AES_CTRL_IR_GF128
/**
 * @}
 */

/** @defgroup AES_LL_KEY_LEN_TYPE KeyLen Type Defines
 * @brief     KeyLen Type defines which can be used with LL_AES_SetKeyLength and LL_AES_GetKeyLength functions.
 * @{
 */
#define LL_AES_KEY_LEN_128             AES_CTRL_SIZE_KLEN128
#define LL_AES_KEY_LEN_192             AES_CTRL_SIZE_KLEN192
#define LL_AES_KEY_LEN_256             AES_CTRL_SIZE_KLEN256
/**
 * @}
 */

/** @defgroup AES_LL_DATA_STREAM_MODE Data Stream Mode Defines
 * @brief     Data Stream Mode defines which can be used with LL_AES_SetDataStreamMode and LL_AES_GetDataStreamMode functions.
 * @{
 */
#define LL_AES_STREAM_MODE_ECB         AES_CTRL_MODE_ECB
#define LL_AES_STREAM_MODE_CBC         AES_CTRL_MODE_CBC
#define LL_AES_STREAM_MODE_CTR         AES_CTRL_MODE_CTR
#define LL_AES_STREAM_MODE_CFB128      AES_CTRL_MODE_CFB128
#define LL_AES_STREAM_MODE_OFB         AES_CTRL_MODE_OFB
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @defgroup AES_LL_Exported_Functions AES Exported Functions
 * @{
 */

/** @defgroup AES_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief  Enable the AES module.
 * @rmtoll START         EN         LL_AES_Enable
 * @param  AESx AES instance
 * @retval None
 */
__STATIC_INLINE void LL_AES_Enable(AES_TypeDef *AESx)
{
    SET_BIT(AESx->START, AES_START_EN);
}

/**
 * @brief  Disable the AES module.
 * @rmtoll START         EN         LL_AES_Disable
 * @param  AESx AES instance
 * @retval None
 */
__STATIC_INLINE void LL_AES_Disable(AES_TypeDef *AESx)
{
    CLEAR_BIT(AESx->START, AES_START_EN);
}

/**
 * @brief  Check if the AES module is enabled or not.
 * @rmtoll START         EN         LL_AES_IsEnabled
 * @param  AESx AES instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_AES_IsEnabled(AES_TypeDef *AESx)
{
    return ((READ_BIT(AESx->START, AES_START_EN) == AES_START_EN) ? 1UL : 0UL);
}

/**
 * @brief  Set interrupt type to enbale.
 * @rmtoll CTRL         IR            LL_AES_SetIRType
 * @param  AESx AES instance
 * @param  IRType This parameter can be one of the following values:
 *         @arg @ref LL_AES_IR_KEYEXP
 *         @arg @ref LL_AES_IR_ENCRYPT
 *         @arg @ref LL_AES_IR_DECRYPT
 *         @arg @ref LL_AES_IR_GF128
 * @retval None
 */
__STATIC_INLINE void LL_AES_SetIRType(AES_TypeDef *AESx, uint32_t IRType)
{
    MODIFY_REG(AESx->CTRL, AES_CTRL_IR, IRType);
}

/**
 * @brief  Get interrupt type to enbale.
 * @rmtoll CTRL         IR            LL_AES_GetIRType
 * @param  AESx AES instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AES_IR_KEYEXP
 *         @arg @ref LL_AES_IR_ENCRYPT
 *         @arg @ref LL_AES_IR_DECRYPT
 *         @arg @ref LL_AES_IR_GF128
 */
__STATIC_INLINE uint32_t LL_AES_GetIRType(AES_TypeDef *AESx)
{
    return (uint32_t)READ_BIT(AESx->CTRL, AES_CTRL_IR);
}

/**
 * @brief  Set aes key length.
 * @rmtoll CTRL         LEN            LL_AES_SetKeyLength
 * @param  AESx AES instance
 * @param  KeyLen This parameter can be one of the following values:
 *         @arg @ref LL_AES_KEY_LEN_128
 *         @arg @ref LL_AES_KEY_LEN_192
 *         @arg @ref LL_AES_KEY_LEN_256
 * @retval None
 */
__STATIC_INLINE void LL_AES_SetKeyLength(AES_TypeDef *AESx, uint32_t KeyLen)
{
    MODIFY_REG(AESx->CTRL, AES_CTRL_SIZE, KeyLen);
}

/**
 * @brief  Get aes key length.
 * @rmtoll CTRL         LEN            LL_AES_GetKeyLength
 * @param  AESx AES instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AES_KEY_LEN_128
 *         @arg @ref LL_AES_KEY_LEN_192
 *         @arg @ref LL_AES_KEY_LEN_256
 */
__STATIC_INLINE uint32_t LL_AES_GetKeyLength(AES_TypeDef *AESx)
{
    return (uint32_t)READ_BIT(AESx->CTRL, AES_CTRL_SIZE);
}

/**
 * @brief  Set the data stream processing mode.
 * @rmtoll CTRL         MODE            LL_AES_SetDataStreamMode
 * @param  AESx AES instance
 * @param  StreamMode This parameter can be one of the following values:
 *         @arg @ref LL_AES_STREAM_MODE_ECB
 *         @arg @ref LL_AES_STREAM_MODE_CBC
 *         @arg @ref LL_AES_STREAM_MODE_CTR
 *         @arg @ref LL_AES_STREAM_MODE_CFB128
 *         @arg @ref LL_AES_STREAM_MODE_OFB
 * @retval None
 */
__STATIC_INLINE void LL_AES_SetDataStreamMode(AES_TypeDef *AESx, uint32_t StreamMode)
{
    MODIFY_REG(AESx->CTRL, AES_CTRL_MODE, StreamMode);
}

/**
 * @brief  Get the data stream processing mode.
 * @rmtoll CTRL         MODE            LL_AES_GetDataStreamMode
 * @param  AESx AES instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_AES_STREAM_MODE_ECB
 *         @arg @ref LL_AES_STREAM_MODE_CBC
 *         @arg @ref LL_AES_STREAM_MODE_CTR
 *         @arg @ref LL_AES_STREAM_MODE_CFB128
 *         @arg @ref LL_AES_STREAM_MODE_OFB
 */
__STATIC_INLINE uint32_t LL_AES_GetDataStreamMode(AES_TypeDef *AESx)
{
    return (uint32_t)READ_BIT(AESx->CTRL, AES_CTRL_MODE);
}

/**
 * @brief  Set the init aes vector array.
 * @rmtoll IV         IV             LL_AES_SetIVArray
 * @param  AESx AES instance
 * @param  IV input vector buffer.
 * @retval None
 */
__STATIC_INLINE void LL_AES_SetIVArray(AES_TypeDef *AESx, uint32_t IV[])
{
    uint32_t i;
    for (i = 0U; i < 4U; i++) {
        WRITE_REG(AESx->IV[i], IV[i]);
    }
}

/**
 * @brief  Get the init vector array.
 * @rmtoll IV         IV             LL_AES_GetIVArray
 * @param  AESx AES instance
 * @param  IV output buffer.
 * @retval None
 */
__STATIC_INLINE void LL_AES_GetIVArray(AES_TypeDef *AESx, uint32_t *IV)
{
    uint32_t i;
    for (i = 0U; i < 4U; i++) {
        IV[i] = READ_REG(AESx->IV[i]);
    }
}

/**
 * @brief  Set the init vector value.
 * @rmtoll IV         IV             LL_AES_SetIV
 * @param  AESx AES instance
 * @param  Idx index of IV buffer.
 * @param  Number Between 0 to 0xFFFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_AES_SetIV(AES_TypeDef *AESx, uint32_t Idx, uint32_t Number)
{
    WRITE_REG(AESx->IV[Idx], Number);
}

/**
 * @brief  Get the init vector value.
 * @rmtoll IV         IV             LL_AES_SetIV
 * @param  AESx AES instance
 * @param  Idx index of IV buffer.
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_AES_GetIV(AES_TypeDef *AESx, uint32_t Idx)
{
    return READ_REG(AESx->IV[Idx]);
}

/**
 * @brief  Set the module base address.
 * @rmtoll ADDR         OFFSET            LL_AES_SetAddress
 * @param  AESx AES instance
 * @param  Address Between 0 to 0x3FFFF.
 * @retval None
 */
__STATIC_INLINE void LL_AES_SetAddress(AES_TypeDef *AESx, uint32_t Address)
{
    MODIFY_REG(AESx->ADDR, AES_ADDR_ADDR, Address);
}

/**
 * @brief  Get the module base address.
 * @rmtoll ADDR         OFFSET            LL_AES_GetAddress
 * @param  AESx AES instance
 * @retval Between 0 to 0x3FFFF.
 */
__STATIC_INLINE uint32_t LL_AES_GetAddress(AES_TypeDef *AESx)
{
    return (READ_BIT(AESx->ADDR, AES_ADDR_ADDR) >> AES_ADDR_ADDR_POS);
}

/**
 * @brief  Set the 128-bit GHASH Multiplicand X array.
 * @rmtoll GHASH_X         X             LL_AES_SetGHASHXArray
 * @param  AESx AES instance
 * @param  X input buffer.
 * @retval None
 */
__STATIC_INLINE void LL_AES_SetGHASHXArray(AES_TypeDef *AESx, const uint32_t X[])
{
    uint32_t i;
    for (i = 0U; i < 4U; i++) {
        WRITE_REG(AESx->GHASH_X[i], X[i]);
    }
}

/**
 * @brief  Get the 128-bit GHASH Multiplicand X array.
 * @rmtoll GHASH_X         X             LL_AES_GetGHASHXArray
 * @param  AESx AES instance
 * @param  X output buffer.
 * @retval None
 */
__STATIC_INLINE void LL_AES_GetGHASHXArray(AES_TypeDef *AESx, uint32_t X[])
{
    uint32_t i;
    for (i = 0U; i < 4U; i++) {
        X[i] = READ_REG(AESx->GHASH_X[i]);
    }
}

/**
 * @brief  Set the 128-bit GHASH Multiplier Y array.
 * @rmtoll GHASH_Y         Y             LL_AES_SetGHASHYArray
 * @param  AESx AES instance
 * @param  Y input buffer.
 * @retval None
 */
__STATIC_INLINE void LL_AES_SetGHASHYArray(AES_TypeDef *AESx, uint32_t Y[])
{
    uint32_t i;
    for (i = 0U; i < 4U; i++) {
        WRITE_REG(AESx->GHASH_Y[i], Y[i]);
    }
}

/**
 * @brief  Get the 128-bit GHASH Multiplier Y array.
 * @rmtoll GHASH_Y         Y             LL_AES_GetGHASHYArray
 * @param  AESx AES instance
 * @param  Y output buffer.
 * @retval None
 */
__STATIC_INLINE void LL_AES_GetGHASHYArray(AES_TypeDef *AESx, uint32_t Y[])
{
    uint32_t i;
    for (i = 0U; i < 4U; i++) {
        Y[i] = READ_REG(AESx->GHASH_Y[i]);
    }
}

/**
 * @brief  Get the 128-bit GHASH Result Z array.
 * @rmtoll GHASH_Z         Z             LL_AES_GetGHASHZArray
 * @param  AESx AES instance
 * @param  Z output buffer.
 * @retval None
 */
__STATIC_INLINE void LL_AES_GetGHASHZArray(AES_TypeDef *AESx, uint32_t Z[])
{
    uint32_t i;
    for (i = 0U; i < 4U; i++) {
        Z[i] = READ_REG(AESx->GHASH_Z[i]);
    }
}

/**
 * @brief  Set the 128-bit GHASH Multiplicand X.
 * @rmtoll GHASH_X         X             LL_AES_SetGHASHX
 * @param  AESx AES instance
 * @param  Idx index of IV buffer.
 * @param  Number Between 0 to 0xFFFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_AES_SetGHASHX(AES_TypeDef *AESx, uint32_t Idx, uint32_t Number)
{
    WRITE_REG(AESx->GHASH_X[Idx], Number);
}

/**
 * @brief  Get the 128-bit GHASH Multiplicand X.
 * @rmtoll GHASH_X         X             LL_AES_GetGHASHX
 * @param  AESx AES instance
 * @param  Idx index of IV buffer.
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_AES_GetGHASHX(AES_TypeDef *AESx, uint32_t Idx)
{
    return READ_REG(AESx->GHASH_X[Idx]);
}

/**
 * @brief  Set the 128-bit GHASH Multiplier Y.
 * @rmtoll GHASH_Y         Y             LL_AES_SetGHASHY
 * @param  AESx AES instance
 * @param  Idx index of IV buffer.
 * @param  Number Between 0 to 0xFFFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_AES_SetGHASHY(AES_TypeDef *AESx, uint32_t Idx, uint32_t Number)
{
    WRITE_REG(AESx->GHASH_Y[Idx], Number);
}

/**
 * @brief  Get the 128-bit GHASH Multiplier Y.
 * @rmtoll GHASH_Y         Y             LL_AES_GetGHASHY
 * @param  AESx AES instance
 * @param  Idx index of IV buffer.
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_AES_GetGHASHY(AES_TypeDef *AESx, uint32_t Idx)
{
    return READ_REG(AESx->GHASH_Y[Idx]);
}

/**
 * @brief  Get the 128-bit GHASH Result Z.
 * @rmtoll GHASH_Z         Z             LL_AES_GetGHASHZ
 * @param  AESx AES instance
 * @param  Idx index of IV buffer.
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_AES_GetGHASHZ(AES_TypeDef *AESx, uint32_t Idx)
{
    return READ_REG(AESx->GHASH_Z[Idx]);
}

/**
 * @}
 */

/** @defgroup AES_LL_EF_FLAG_Management FLAG_Management
 * @{
 */
/**
 * @brief  Enable aes calculation finish interrupt.
 * @rmtoll IE         FINISH         LL_AES_EnableIT_AESDONE
 * @param  AESx AES instance
 * @retval None
 */
__STATIC_INLINE void LL_AES_EnableIT_AESDONE(AES_TypeDef *AESx)
{
    SET_BIT(AESx->IE, AES_IE_AES);
}

/**
 * @brief  Disable aes calculation finish interrupt.
 * @rmtoll IE         FINISH         LL_AES_DisableIT_AESDONE
 * @param  AESx AES instance
 * @retval None
 */
__STATIC_INLINE void LL_AES_DisableIT_AESDONE(AES_TypeDef *AESx)
{
    CLEAR_BIT(AESx->IE, AES_IE_AES);
}

/**
 * @brief  Check if aes calculation finish interrupt is enabled or not.
 * @rmtoll IE         FINISH         LL_AES_IsEnabledIT_AESDONE
 * @param  AESx AES instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_AES_IsEnabledIT_AESDONE(AES_TypeDef *AESx)
{
    return ((READ_BIT(AESx->IE, AES_IE_AES) == AES_IE_AES) ? 1UL : 0UL);
}

/**
 * @brief  Enable GHash calculation finish interrupt.
 * @rmtoll IE         GHASH         LL_AES_EnableIT_GHASHDONE
 * @param  AESx AES instance
 * @retval None
 */
__STATIC_INLINE void LL_AES_EnableIT_GHASHDONE(AES_TypeDef *AESx)
{
    SET_BIT(AESx->IE, AES_IE_GHASH);
}

/**
 * @brief  Disable GHash calculation finish interrupt.
 * @rmtoll IE         GHASH         LL_AES_DisableIT_GHASHDONE
 * @param  AESx AES instance
 * @retval None
 */
__STATIC_INLINE void LL_AES_DisableIT_GHASHDONE(AES_TypeDef *AESx)
{
    CLEAR_BIT(AESx->IE, AES_IE_GHASH);
}

/**
 * @brief  Check if GHash calculation finish interrupt is enabled or not.
 * @rmtoll IE         GHASH         LL_AES_IsEnabledIT_GHASHDONE
 * @param  AESx AES instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_AES_IsEnabledIT_GHASHDONE(AES_TypeDef *AESx)
{
    return ((READ_BIT(AESx->IE, AES_IE_GHASH) == AES_IE_GHASH) ? 1UL : 0UL);
}

/**
 * @brief  Check if SEA Crypt calculation finish flag is set or not.
 * @rmtoll STA         CRYPT_DONE         LL_AES_IsActiveFlag_CRYPTDONE
 * @param  AESx AES instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_AES_IsActiveFlag_CRYPTDONE(AES_TypeDef *AESx)
{
    return ((READ_BIT(AESx->STA, AES_STA_DONE) == AES_STA_DONE) ? 1UL : 0UL);
}

/**
 * @brief  Clear SEA Crypt calculation finish flag.
 * @rmtoll STA         CRYPT_DONE         LL_AES_ClearFlag_CRYPTDONE
 * @param  AESx AES instance
 * @retval None
 */
__STATIC_INLINE void LL_AES_ClearFlag_CRYPTDONE(AES_TypeDef *AESx)
{
    WRITE_REG(AESx->STA, AES_STA_DONE);
}

/**
 * @brief  Check if aes calculation finish flag is set or not.
 * @rmtoll STA         AES_DONE         LL_AES_IsActiveFlag_AESDONE
 * @param  AESx AES instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_AES_IsActiveFlag_AESDONE(AES_TypeDef *AESx)
{
    return ((READ_BIT(AESx->STA, AES_STA_AESDONE) == AES_STA_AESDONE) ? 1UL : 0UL);
}

/**
 * @brief  Clear aes calculation finish flag.
 * @rmtoll STA         AES_DONE         LL_AES_ClearFlag_AESDONE
 * @param  AESx AES instance
 * @retval None
 */
__STATIC_INLINE void LL_AES_ClearFlag_AESDONE(AES_TypeDef *AESx)
{
    WRITE_REG(AESx->STA, AES_STA_AESDONE);
}

/**
 * @brief  Check if GHash calculation finish flag is set or not.
 * @rmtoll STA         GHASH_DONE         LL_AES_IsActiveFlag_GHASHDONE
 * @param  AESx AES instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_AES_IsActiveFlag_GHASHDONE(AES_TypeDef *AESx)
{
    return ((READ_BIT(AESx->STA, AES_STA_GHASHDONE) == AES_STA_GHASHDONE) ? 1UL : 0UL);
}

/**
 * @brief  Clear GHash calculation finish flag.
 * @rmtoll STA         GHASH_DONE         LL_AES_ClearFlag_GHASHDONE
 * @param  AESx AES instance
 * @retval None
 */
__STATIC_INLINE void LL_AES_ClearFlag_GHASHDONE(AES_TypeDef *AESx)
{
    WRITE_REG(AESx->STA, AES_STA_GHASHDONE);
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

#endif /* AES */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_AES_H */
