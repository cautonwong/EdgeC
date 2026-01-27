/**
 *****************************************************************************
 * @file rn_rv_ll_simptc.h
 * @author Renergy Software Team
 * @brief  Header file of SIMPTC LL module.
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
#ifndef __RN_RV_LL_SIMPTC_H
#define __RN_RV_LL_SIMPTC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_simptc.h"

/** @addtogroup RN_LL_Driver
 * @{
 */
#if defined (SIMPTC0_0_CORE0) || defined (SIMPTC0_1_CORE0) || defined (SIMPTC0_2_CORE0) || defined (SIMPTC0_3_CORE0) || \
    defined (SIMPTC1_0_CORE0) || defined (SIMPTC1_1_CORE0) || defined (SIMPTC1_2_CORE0) || defined (SIMPTC1_3_CORE0) || \
    defined (SIMPTC2_0_CORE0) || defined (SIMPTC2_1_CORE0) || defined (SIMPTC2_2_CORE0) || defined (SIMPTC2_3_CORE0) || \
    defined (SIMPTC3_0_CORE0) || defined (SIMPTC3_1_CORE0) || defined (SIMPTC3_2_CORE0) || defined (SIMPTC3_3_CORE0) || \
    defined (SIMPTC0_0_CORE1) || defined (SIMPTC0_1_CORE1) || defined (SIMPTC0_2_CORE1) || defined (SIMPTC0_3_CORE1) || \
    defined (SIMPTC1_0_CORE1) || defined (SIMPTC1_1_CORE1) || defined (SIMPTC1_2_CORE1) || defined (SIMPTC1_3_CORE1) || \
    defined (SIMPTC2_0_CORE1) || defined (SIMPTC2_1_CORE1) || defined (SIMPTC2_2_CORE1) || defined (SIMPTC2_3_CORE1) || \
    defined (SIMPTC3_0_CORE1) || defined (SIMPTC3_1_CORE1) || defined (SIMPTC3_2_CORE1) || defined (SIMPTC3_3_CORE1)

/** @defgroup SIMPTC_LL SIMPTC
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/** @defgroup SIMPTC_LL_ES_INIT SIMPTC Exported Init structure
 * @{
 */

/**
 * @brief  SIMPTC Time Base configuration structure definition.
 */
typedef struct
{
    uint32_t OnePulseMode;          /*!< Specifies the counter mode.
                                         This parameter can be a value of @ref SIMPTC_LL_EC_ONEPULSEMODE.

                                         This feature can be modified afterwards using unitary function
                                         @ref LL_SIMPTC_SetOnePulseMode().*/

    uint32_t AutoReload;            /*!< Specifies the auto reload value to be loaded into the active
                                         Auto-Reload Register at the next update event.
                                         This parameter must be a number between Min_Data=0x0000 and Max_Data=0xFFFFFFFF.

                                         This feature can be modified afterwards using unitary function
                                         @ref LL_SIMPTC_SetAutoReload().*/

    uint32_t EnableSource;          /*!< Specifies the counter enable source.
                                         This parameter can be a value of @ref SIMPTC_LL_EC_ENABLESOURCE.
                                         Note: AOS enable counting only supports single count mode.

                                         This feature can be modified afterwards using unitary function
                                         @ref LL_SIMPTC_SetEnableSource().*/

    uint32_t DebugMode;             /*!< Specifies the debug mode.
                                         This parameter can be a value of @ref SIMPTC_LL_EC_DEBUGMODE.

                                         This feature can be modified afterwards using unitary function
                                         @ref LL_SIMPTC_SetDebugMode().*/
} LL_SIMPTC_InitTypeDef;
/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup SIMPTC_LL_Exported_Constants SIMPTC Exported Constants
 * @{
 */

/** @defgroup SIMPTC_LL_EC_ONEPULSEMODE One Pulse Mode
 * @{
 */
#define LL_SIMPTC_ONEPULSEMODE_SINGLE             0x00000000U             /*!< Counter stops counting at the target value */
#define LL_SIMPTC_ONEPULSEMODE_CONTINUOUS         SIMPTC_CTRL_MODE        /*!< Counter restarts from 0 after reaching target value */
/**
 * @}
 */

/** @defgroup SIMPTC_LL_EC_ENABLESOURCE Enable Source
 * @{
 */
#define LL_SIMPTC_ENABLESOURCE_SOFTWARE          0x00000000U             /*!< Software enable counter counting */
#define LL_SIMPTC_ENABLESOURCE_AOS               SIMPTC_CTRL_CNT_SEL     /*!< AOS enable counter counting */
/**
 * @}
 */

/** @defgroup SIMPTC_LL_EC_DEBUGMODE Debug Mode
 * @{
 */
#define LL_SIMPTC_DEBUGMODE_PAUSE                0x00000000U             /*!< Pause counting during debug */
#define LL_SIMPTC_DEBUGMODE_CONTINUE             SIMPTC_CTRL_DBGSTBDIS   /*!< Continue counting during debug */
/**
 * @}
 */

/** @defgroup SIMPTC_LL_EC_IT IT Defines
 * @brief    IT defines which can be used with LL_SIMPTC_ReadReg and  LL_SIMPTC_WriteReg functions.
 * @{
 */
#define LL_SIMPTC_IF_OV                          SIMPTC_IF_OV            /*!< Overflow interrupt flag */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup SIMPTC_LL_Exported_Macros SIMPTC Exported Macros
 * @{
 */

/** @defgroup SIMPTC_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in SIMPTC register
 * @param  __INSTANCE__ SIMPTC Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_SIMPTC_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, __VALUE__)

/**
 * @brief  Read a value in SIMPTC register
 * @param  __INSTANCE__ SIMPTC Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_SIMPTC_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup SIMPTC_LL_Exported_Functions SIMPTC Exported Functions
 * @{
 */

/** @defgroup SIMPTC_LL_EF_Time_Base Time Base configuration
 * @{
 */

/**
 * @brief  Enable SIMPTC counter.
 * @rmtoll CTRL         CNT_SFT_EN           LL_SIMPTC_EnableCounter
 * @param  SIMPTCx_x SIMPTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_SIMPTC_EnableCounter(SIMPTC_TypeDef *SIMPTCx_x)
{
    SET_BIT(SIMPTCx_x->CTRL, SIMPTC_CTRL_CNT_SFT_EN);
}

/**
 * @brief  Disable SIMPTC counter.
 * @rmtoll CTRL         CNT_SFT_EN           LL_SIMPTC_DisableCounter
 * @param  SIMPTCx_x SIMPTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_SIMPTC_DisableCounter(SIMPTC_TypeDef *SIMPTCx_x)
{
    CLEAR_BIT(SIMPTCx_x->CTRL, SIMPTC_CTRL_CNT_SFT_EN);
}

/**
 * @brief  Indicates whether the SIMPTC counter is enabled.
 * @rmtoll CTRL         CNT_SFT_EN           LL_SIMPTC_IsEnabledCounter
 * @param  SIMPTCx_x SIMPTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SIMPTC_IsEnabledCounter(const SIMPTC_TypeDef *SIMPTCx_x)
{
    return ((READ_BIT(SIMPTCx_x->CTRL, SIMPTC_CTRL_CNT_SFT_EN) == (SIMPTC_CTRL_CNT_SFT_EN)) ? 1UL : 0UL);
}

/**
 * @brief  Set the counter mode (single or continuous).
 * @note   Macro @ref IS_SIMPTC_INSTANCE(SIMPTCx_x) can be used to check whether or not
 *         a SIMPTC instance supports single mode.
 * @note   AOS enable counting only supports single count mode.
 * @rmtoll CTRL         MODE         LL_SIMPTC_SetOnePulseMode
 * @param  SIMPTCx_x SIMPTC Instance
 * @param  OnePulseMode This parameter can be one of the following values:
 *         @arg @ref LL_SIMPTC_ONEPULSEMODE_SINGLE
 *         @arg @ref LL_SIMPTC_ONEPULSEMODE_CONTINUOUS
 * @retval None
 */
__STATIC_INLINE void LL_SIMPTC_SetOnePulseMode(SIMPTC_TypeDef *SIMPTCx_x, uint32_t OnePulseMode)
{
    MODIFY_REG(SIMPTCx_x->CTRL, SIMPTC_CTRL_MODE, OnePulseMode);
}

/**
 * @brief  Get the counter mode (single or continuous).
 * @rmtoll CTRL         MODE         LL_SIMPTC_GetOnePulseMode
 * @param  SIMPTCx_x SIMPTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SIMPTC_ONEPULSEMODE_SINGLE
 *         @arg @ref LL_SIMPTC_ONEPULSEMODE_CONTINUOUS
 */
__STATIC_INLINE uint32_t LL_SIMPTC_GetOnePulseMode(const SIMPTC_TypeDef *SIMPTCx_x)
{
    return (uint32_t)(READ_BIT(SIMPTCx_x->CTRL, SIMPTC_CTRL_MODE));
}

/**
 * @brief  Set the counter mode at debug state.
 * @rmtoll CTRL         DBGSTBDIS    LL_SIMPTC_SetDebugMode
 * @param  SIMPTCx_x SIMPTC Instance
 * @param  DebugMode This parameter can be one of the following values:
 *         @arg @ref LL_SIMPTC_DEBUGMODE_PAUSE
 *         @arg @ref LL_SIMPTC_DEBUGMODE_CONTINUE
 * @retval None
 */
__STATIC_INLINE void LL_SIMPTC_SetDebugMode(SIMPTC_TypeDef *SIMPTCx_x, uint32_t DebugMode)
{
    MODIFY_REG(SIMPTCx_x->CTRL, SIMPTC_CTRL_DBGSTBDIS, DebugMode);
}

/**
 * @brief  Get the counter mode configuration at debug state.
 * @rmtoll CTRL         DBGSTBDIS    LL_SIMPTC_GetDebugMode
 * @param  SIMPTCx_x SIMPTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SIMPTC_DEBUGMODE_PAUSE
 *         @arg @ref LL_SIMPTC_DEBUGMODE_CONTINUE
 */
__STATIC_INLINE uint32_t LL_SIMPTC_GetDebugMode(const SIMPTC_TypeDef *SIMPTCx_x)
{
    return (uint32_t)(READ_BIT(SIMPTCx_x->CTRL, SIMPTC_CTRL_DBGSTBDIS));
}

/**
 * @brief  Set the counter enable source.
 * @rmtoll CTRL         CNT_SEL      LL_SIMPTC_SetEnableSource
 * @param  SIMPTCx_x SIMPTC Instance
 * @param  EnableSource This parameter can be one of the following values:
 *         @arg @ref LL_SIMPTC_ENABLESOURCE_SOFTWARE
 *         @arg @ref LL_SIMPTC_ENABLESOURCE_AOS
 * @retval None
 */
__STATIC_INLINE void LL_SIMPTC_SetEnableSource(SIMPTC_TypeDef *SIMPTCx_x, uint32_t EnableSource)
{
    MODIFY_REG(SIMPTCx_x->CTRL, SIMPTC_CTRL_CNT_SEL, EnableSource);
}

/**
 * @brief  Get the counter enable source.
 * @rmtoll CTRL         CNT_SEL      LL_SIMPTC_GetEnableSource
 * @param  SIMPTCx_x SIMPTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SIMPTC_ENABLESOURCE_SOFTWARE
 *         @arg @ref LL_SIMPTC_ENABLESOURCE_AOS
 */
__STATIC_INLINE uint32_t LL_SIMPTC_GetEnableSource(const SIMPTC_TypeDef *SIMPTCx_x)
{
    return (uint32_t)(READ_BIT(SIMPTCx_x->CTRL, SIMPTC_CTRL_CNT_SEL));
}

/**
 * @brief  Set the target count value.
 * @rmtoll DN         DN         LL_SIMPTC_SetAutoReload
 * @param  SIMPTCx_x SIMPTC Instance
 * @param  AutoReload between Min_Data=0 and Max_Data=0xFFFFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_SIMPTC_SetAutoReload(SIMPTC_TypeDef *SIMPTCx_x, uint32_t AutoReload)
{
    WRITE_REG(SIMPTCx_x->DN, AutoReload);
}

/**
 * @brief  Get the target count value.
 * @rmtoll DN         DN         LL_SIMPTC_GetAutoReload
 * @param  SIMPTCx_x SIMPTC Instance
 * @retval AutoReload value between Min_Data=0 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_SIMPTC_GetAutoReload(const SIMPTC_TypeDef *SIMPTCx_x)
{
    return (uint32_t)(READ_REG(SIMPTCx_x->DN));
}

/**
 * @brief  Get the current counter value.
 * @rmtoll VAL          VAL          LL_SIMPTC_GetCounter
 * @param  SIMPTCx_x SIMPTC Instance
 * @retval Counter value between Min_Data=0 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_SIMPTC_GetCounter(const SIMPTC_TypeDef *SIMPTCx_x)
{
    return (uint32_t)(READ_REG(SIMPTCx_x->VAL));
}

/**
 * @}
 */

/** @defgroup SIMPTC_LL_EF_FLAG_Management FLAG-Management
 * @{
 */

/**
 * @brief  Clear the overflow flag.
 * @rmtoll IF           OV           LL_SIMPTC_ClearFlag_OV
 * @param  SIMPTCx_x SIMPTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_SIMPTC_ClearFlag_OV(SIMPTC_TypeDef *SIMPTCx_x)
{
    WRITE_REG(SIMPTCx_x->IF, SIMPTC_IF_OV);
}

/**
 * @brief  Indicate whether overflow flag is set.
 * @rmtoll IF           OV           LL_SIMPTC_IsActiveFlag_OV
 * @param  SIMPTCx_x SIMPTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SIMPTC_IsActiveFlag_OV(const SIMPTC_TypeDef *SIMPTCx_x)
{
    return ((READ_BIT(SIMPTCx_x->IF, SIMPTC_IF_OV) == (SIMPTC_IF_OV)) ? 1UL : 0UL);
}

/**
 * @}
 */

/** @defgroup SIMPTC_LL_EF_IT_Management IT-Management
 * @{
 */

/**
 * @brief  Enable overflow interrupt.
 * @rmtoll CTRL         IRQEN        LL_SIMPTC_EnableIT_OV
 * @param  SIMPTCx_x SIMPTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_SIMPTC_EnableIT_OV(SIMPTC_TypeDef *SIMPTCx_x)
{
    SET_BIT(SIMPTCx_x->CTRL, SIMPTC_CTRL_IRQEN);
}

/**
 * @brief  Disable overflow interrupt.
 * @rmtoll CTRL         IRQEN        LL_SIMPTC_DisableIT_OV
 * @param  SIMPTCx_x SIMPTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_SIMPTC_DisableIT_OV(SIMPTC_TypeDef *SIMPTCx_x)
{
    CLEAR_BIT(SIMPTCx_x->CTRL, SIMPTC_CTRL_IRQEN);
}

/**
 * @brief  Indicates whether the overflow interrupt is enabled.
 * @rmtoll CTRL         IRQEN        LL_SIMPTC_IsEnabledIT_OV
 * @param  SIMPTCx_x SIMPTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SIMPTC_IsEnabledIT_OV(const SIMPTC_TypeDef *SIMPTCx_x)
{
    return ((READ_BIT(SIMPTCx_x->CTRL, SIMPTC_CTRL_IRQEN) == (SIMPTC_CTRL_IRQEN)) ? 1UL : 0UL);
}
/**
 * @}
 */
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup SIMPTC_LL_EF_Init Initialisation and deinitialisation functions
 * @{
 */
ErrorStatus LL_SIMPTC_DeInit(SIMPTC_TypeDef *SIMPTCx_x);
void LL_SIMPTC_StructInit(LL_SIMPTC_InitTypeDef *SIMPTC_InitStruct);
ErrorStatus LL_SIMPTC_Init(SIMPTC_TypeDef *SIMPTCx_x, LL_SIMPTC_InitTypeDef *SIMPTC_InitStruct);
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

#endif /* SIMPTC0_0 || SIMPTC0_1 || ... */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_LL_SIMPTC_H */
