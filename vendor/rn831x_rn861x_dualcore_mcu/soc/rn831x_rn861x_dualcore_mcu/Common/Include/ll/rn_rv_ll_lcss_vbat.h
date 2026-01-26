/**
 *****************************************************************************
 * @file   rn_rv_ll_lcss_vbat.h
 * @author Renergy Software Team
 * @brief  Header file of LCSS_VBAT LL module.
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
#ifndef __RN_LL_LCSS_VBAT_H
#define __RN_LL_LCSS_VBAT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_lcss_vbat.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (LCSS_VBAT)

/** @defgroup LCSS_VBAT_LL LCSS_VBAT
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup LCSS_VBAT_LL_Private_Constants LCSS_VBAT Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup LCSS_VBAT_LL_Private_Macros LCSS_VBAT Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup LCSS_VBAT_LL_ES_INIT LCSS_VBAT Exported Init structure
 * @{
 */

/**
 * @brief LL LCSS_VBAT Init Structure definition
 */
typedef struct
{
    uint32_t StopThreshold;                 /*!< Specifies the Clock Stop Threshold.
                                                This parameter must be set to a value between Min_Data=0x00 and Max_Data=0x1F */

    uint32_t RecoveryCycles;                /*!< Specifies the LOSC Recovery Cycles.
                                                This parameter must be set to a value between Min_Data=0x00 and Max_Data=0xFE */

    FunctionalState StopAutoSwitchCommand;  /*!< Specifies the Auto switch when clock stopped.
                                                This parameter can be set either to ENABLE or DISABLE */

    FunctionalState AutoRecoveryCommand;    /*!< Specifies the Auto switch when clock recovery.
                                                This parameter can be set either to ENABLE or DISABLE */

    uint32_t TimerConfig;                   /*!< Specifies Timing detection configuration.
                                                This parameter can be a value of @ref LCSS_VBAT_LL_EC_Timer_Config.

                                                This feature can be modified afterwards using unitary function @ref LL_LCSS_VBAT_SetTimerConfig(). */
} LL_LCSS_VBAT_InitTypeDef;

/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/* Exported constants -------------------------------------------------------*/
/** @defgroup LCSS_VBAT_LL_Exported_Constants LCSS_VBAT Exported Constants
 * @{
 */

/** @defgroup LCSS_VBAT_LL_EC_Timer_Config Timer_Config
 * @{
 */
#define LL_LCSS_VBAT_TIMING_NONE                  LCSS_VBAT_CTRL_TC_CFG_TIMING_NONE                      /*!< No timing detection                               */
#define LL_LCSS_VBAT_TIMING_10MS                  LCSS_VBAT_CTRL_TC_CFG_TIMING_10MS                      /*!< 10ms timing detection, start detection every 10ms */
#define LL_LCSS_VBAT_TIMING_20MS                  LCSS_VBAT_CTRL_TC_CFG_TIMING_20MS                      /*!< 20ms timing detection, start detection every 20ms */
#define LL_LCSS_VBAT_TIMING_40MS                  LCSS_VBAT_CTRL_TC_CFG_TIMING_40MS                      /*!< 40ms timing detection, start detection every 40ms */
/**
 * @}
 */

/** @defgroup LCSS_VBAT_LL_EC_RTC_Clock_Source RTC_Clock_Source
 * @{
 */
#define LL_LCSS_VBAT_RTC_LOSC                     LCSS_VBAT_CLKSWH_RTC_CLK_SWH_LOSC                      /*!< RTC Selected LOSC clock   */
#define LL_LCSS_VBAT_RTC_RCL                      LCSS_VBAT_CLKSWH_RTC_CLK_SWH_RCL                       /*!< RTC Selected RCL32k clock */
#define LL_LCSS_VBAT_RTC_RCO                      LCSS_VBAT_CLKSWH_RTC_CLK_SWH_RCO                       /*!< RTC Selected RCO clock    */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup LCSS_VBAT_LL_Exported_Macros LCSS_VBAT Exported Macros
 * @{
 */
/** @defgroup LCSS_VBAT_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in LCSS_VBAT register
 * @param  __INSTANCE__ LCSS_VBAT Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_LCSS_VBAT_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in LCSS_VBAT register
 * @param  __INSTANCE__ LCSS_VBAT Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_LCSS_VBAT_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup LCSS_VBAT_LL_Exported_Functions LCSS_VBAT Exported Functions
 * @{
 */

/** @defgroup LCSS_VBAT_LL_EF_Unlock UnlocK
 * @{
 */

/**
 * @brief  UnlocK registers include LCSS_VBAT_EN/LCSS_CTRL/LCSS_IE
 * @note   Read back as 1(0x853CU) or 0(others)
 * @rmtoll LCSS_PSW0         PSW0           LL_LCSS_VBAT_Unlock_PSW0
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_UnlockPSW0(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    MODIFY_REG(LCSS_VBATx->PSW0, LCSS_VBAT_PSW0_PSW0, 0x853CU);
}

/**
 * @brief  Lock registers include LCSS_VBAT_EN/LCSS_CTRL/LCSS_IE
 * @note   Read back as 1(0x853CU) or 0(others)
 * @rmtoll LCSS_PSW0         PSW0           LL_LCSS_VBAT_Lock_PSW0
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_LockPSW0(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    MODIFY_REG(LCSS_VBATx->PSW0, LCSS_VBAT_PSW0_PSW0, 0x0000U);
}

/**
 * @brief  Indicate if the PSW registers is Unlocked include LCSS_VBAT_EN/LCSS_CTRL/LCSS_IE
 * @note   Read back as 1(0x853CU) or 0(others)
 * @rmtoll LCSS_PSW0         PSW0           LL_LCSS_VBAT_IsUnlocked_PSW0
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_IsUnlockedPSW0(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->PSW0, LCSS_VBAT_PSW0_PSW0) == (0x0001U));
}

/**
 * @brief  UnlocK registers include LCSS_CLKSWH
 * @note   Read back as 1(0x8529U) or 0(others)
 * @rmtoll PSW1         PSW1           LL_LCSS_VBAT_Unlock_PSW1
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_UnlockPSW1(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    MODIFY_REG(LCSS_VBATx->PSW1, LCSS_VBAT_PSW1_PSW1, 0x8529U);
}

/**
 * @brief  Lock registers include LCSS_CLKSWH
 * @note   Read back as 1(0x8529U) or 0(others)
 * @rmtoll PSW1         PSW1           LL_LCSS_VBAT_Lock_PSW1
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_LockPSW1(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    MODIFY_REG(LCSS_VBATx->PSW1, LCSS_VBAT_PSW1_PSW1, 0x0000U);
}

/**
 * @brief  Indicate if the PSW registers is Unlocked include LCSS_CLKSWH
 * @note   Read back as 1(0x8529U) or 0(others)
 * @rmtoll PSW1         PSW1           LL_LCSS_VBAT_IsUnlocked_PSW1
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_IsUnlockedPSW1(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->PSW1, LCSS_VBAT_PSW1_PSW1) == (0x0001U));
}

/**
 * @}
 */

/** @defgroup I2C_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief  Enable LCSS_VBAT function
 * @rmtoll EN         LCSS_VBAT_EN           LL_LCSS_VBAT_Enable
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_Enable(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    SET_BIT(LCSS_VBATx->EN, LCSS_VBAT_EN_EN);
}

/**
 * @brief  Disable LCSS_VBAT function
 * @rmtoll EN         LCSS_VBAT_EN           LL_LCSS_VBAT_Disable
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_Disable(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    CLEAR_BIT(LCSS_VBATx->EN, LCSS_VBAT_EN_EN);
}

/**
 * @brief  Indicate if the LCSS_VBAT function is enabled
 * @rmtoll EN         LCSS_VBAT_EN           LL_LCSS_VBAT_IsEnabled
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_IsEnabled(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->EN, LCSS_VBAT_EN_EN) == (LCSS_VBAT_EN_EN));
}

/**
 * @brief  Set Clock Stop Threshold
 * @rmtoll CTRL          STOP_TH           LL_LCSS_VBAT_SetStopThreshold
 * @param  StopThreshold Value between Min_Data=0x00 and Max_Data=0x1F
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_SetStopThreshold(LCSS_VBAT_TypeDef *LCSS_VBATx, uint32_t StopThreshold)
{
    MODIFY_REG(LCSS_VBATx->CTRL, LCSS_VBAT_CTRL_STOP_TH, StopThreshold << LCSS_VBAT_CTRL_STOP_TH_POS);
}

/**
 * @brief  Get Clock Stop Threshold
 * @rmtoll CTRL          STOP_TH           LL_LCSS_VBAT_GetStopThreshold
 * @retval Stop Threshold (Value between Min_Data=0x00 and Max_Data=0x1F)
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_GetStopThreshold(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->CTRL, LCSS_VBAT_CTRL_STOP_TH) >> LCSS_VBAT_CTRL_STOP_TH_POS);
}

/**
 * @brief  Set LOSC Recovery Cycles
 * @rmtoll CTRL          RCV_CYC           LL_LCSS_VBAT_SetRecoveryCycles
 * @param  RecoveryCycles Value between Min_Data=0x00 and Max_Data=0xFE
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_SetRecoveryCycles(LCSS_VBAT_TypeDef *LCSS_VBATx, uint32_t RecoveryCycles)
{
    MODIFY_REG(LCSS_VBATx->CTRL, LCSS_VBAT_CTRL_RCV_CYC, RecoveryCycles << LCSS_VBAT_CTRL_RCV_CYC_POS);
}

/**
 * @brief  Get LOSC Recovery Cycles
 * @rmtoll CTRL          RCV_CYC           LL_LCSS_VBAT_GetRecoveryCycles
 * @retval Recovery Cycles (Value between Min_Data=0x00 and Max_Data=0xFE)
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_GetRecoveryCycles(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->CTRL, LCSS_VBAT_CTRL_RCV_CYC) >> LCSS_VBAT_CTRL_RCV_CYC_POS);
}

/**
 * @brief  Enable Auto switch when clock stopped function
 * @rmtoll CTRL         STOP_ASWH           LL_LCSS_VBAT_EnableStopAutoSwitch
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_EnableStopAutoSwitch(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    SET_BIT(LCSS_VBATx->CTRL, LCSS_VBAT_CTRL_STOP_ASWH);
}

/**
 * @brief  Disable Auto switch when clock stopped function
 * @rmtoll CTRL         STOP_ASWH           LL_LCSS_VBAT_DisableStopAutoSwitch
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_DisableStopAutoSwitch(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    CLEAR_BIT(LCSS_VBATx->CTRL, LCSS_VBAT_CTRL_STOP_ASWH);
}

/**
 * @brief  Indicate if the Auto switch when clock stopped function is enabled
 * @rmtoll CTRL         STOP_ASWH           LL_LCSS_VBAT_IsEnabledStopAutoSwitch
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_IsEnabledStopAutoSwitch(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->CTRL, LCSS_VBAT_CTRL_STOP_ASWH) == (LCSS_VBAT_CTRL_STOP_ASWH));
}

/**
 * @brief  Enable Auto switch when clock recovery function
 * @rmtoll CTRL         RCV_ASWH           LL_LCSS_VBAT_EnableAutoRecovery
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_EnableAutoRecovery(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    SET_BIT(LCSS_VBATx->CTRL, LCSS_VBAT_CTRL_RCV_ASWH);
}

/**
 * @brief  Disable Auto switch when clock recovery function
 * @rmtoll CTRL         RCV_ASWH           LL_LCSS_VBAT_DisableAutoRecovery
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_DisableAutoRecovery(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    CLEAR_BIT(LCSS_VBATx->CTRL, LCSS_VBAT_CTRL_RCV_ASWH);
}

/**
 * @brief  Indicate if the Auto switch when clock recovery function is enabled
 * @rmtoll CTRL         RCV_ASWH           LL_LCSS_VBAT_IsEnabledAutoRecovery
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_IsEnabledAutoRecovery(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->CTRL, LCSS_VBAT_CTRL_RCV_ASWH) == (LCSS_VBAT_CTRL_RCV_ASWH));
}

/**
 * @brief  Set Timing detection configuration
 * @rmtoll CTRL          TC_CFG           LL_LCSS_VBAT_SetTimerConfig
 * @param  TimerConfig This parameter can be one of the following values:
 *         @arg @ref LL_LCSS_VBAT_TIMING_NONE
 *         @arg @ref LL_LCSS_VBAT_TIMING_10MS
 *         @arg @ref LL_LCSS_VBAT_TIMING_20MS
 *         @arg @ref LL_LCSS_VBAT_TIMING_40MS
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_SetTimerConfig(LCSS_VBAT_TypeDef *LCSS_VBATx, uint32_t TimerConfig)
{
    MODIFY_REG(LCSS_VBATx->CTRL, LCSS_VBAT_CTRL_TC_CFG, TimerConfig);
}

/**
 * @brief  Get Timing detection configuration
 * @rmtoll CTRL          TC_CFG           LL_LCSS_VBAT_GetTimerConfig
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LCSS_VBAT_TIMING_NONE
 *         @arg @ref LL_LCSS_VBAT_TIMING_10MS
 *         @arg @ref LL_LCSS_VBAT_TIMING_20MS
 *         @arg @ref LL_LCSS_VBAT_TIMING_40MS
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_GetTimerConfig(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->CTRL, LCSS_VBAT_CTRL_TC_CFG));
}

/**
 * @brief  Set RTC Clock source selection (default is LL_LCSS_VBAT_RTC_LOSC)
 * @note   this register write only.
 * @rmtoll CLKSWH          RTC_CLK_SWH           LL_LCSS_VBAT_SetRTCClockSource
 * @param  RTCClockSource This parameter can be one of the following values:
 *         @arg @ref LL_LCSS_VBAT_RTC_LOSC
 *         @arg @ref LL_LCSS_VBAT_RTC_RCL
 *         @arg @ref LL_LCSS_VBAT_RTC_RCO
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_SetRTCClockSource(LCSS_VBAT_TypeDef *LCSS_VBATx, uint32_t RTCClockSource)
{
    MODIFY_REG(LCSS_VBATx->CLKSWH, LCSS_VBAT_CLKSWH_RTC_CLK_SWH, RTCClockSource);
}

/**
 * @brief  Get RTC Clock source selection (default is LL_LCSS_VBAT_RTC_LOSC)
 * @note   this register must be written to 0 before reading to read the latest value.
 * @rmtoll CLKSEL          RTC_CLK_SEL           LL_LCSS_VBAT_GetRTCClockSource
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LCSS_VBAT_RTC_LOSC
 *         @arg @ref LL_LCSS_VBAT_RTC_RCL
 *         @arg @ref LL_LCSS_VBAT_RTC_RCO
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_GetRTCClockSource(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    MODIFY_REG(LCSS_VBATx->CLKSEL, LCSS_VBAT_CLKSEL_RTC_CLK_SEL, 0x0U);
    return (READ_BIT(LCSS_VBATx->CLKSEL, LCSS_VBAT_CLKSEL_RTC_CLK_SEL));
}

/**
 * @}
 */

/** @defgroup LCSS_VBAT_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable LOSC stopped interrupt
 * @rmtoll IE         LOSC_STOP_IE           LL_LCSS_VBAT_EnableIT_LOSCSTOP
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_EnableIT_LOSCSTOP(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    SET_BIT(LCSS_VBATx->IE, LCSS_VBAT_IE_LOSC_STOP_IE);
}

/**
 * @brief  Disable LOSC stopped interrupt
 * @rmtoll IE         LOSC_STOP_IE           LL_LCSS_VBAT_DisableIT_LOSCSTOP
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_DisableIT_LOSCSTOP(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    CLEAR_BIT(LCSS_VBATx->IE, LCSS_VBAT_IE_LOSC_STOP_IE);
}

/**
 * @brief  Indicate if the LOSC stopped interrupt is enabled
 * @rmtoll IE         LOSC_STOP_IE           LL_LCSS_VBAT_IsEnabledIT_LOSCSTOP
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_IsEnabledIT_LOSCSTOP(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->IE, LCSS_VBAT_IE_LOSC_STOP_IE) == (LCSS_VBAT_IE_LOSC_STOP_IE));
}

/**
 * @brief  Enable LOSC is ok interrupt
 * @rmtoll IE         LOSC_OK_IE           LL_LCSS_VBAT_EnableIT_LOSCOK
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_EnableIT_LOSCOK(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    SET_BIT(LCSS_VBATx->IE, LCSS_VBAT_IE_LOSC_OK_IE);
}

/**
 * @brief  Disable LOSC is ok interrupt
 * @rmtoll IE         LOSC_OK_IE           LL_LCSS_VBAT_DisableIT_LOSCOK
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_DisableIT_LOSCOK(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    CLEAR_BIT(LCSS_VBATx->IE, LCSS_VBAT_IE_LOSC_OK_IE);
}

/**
 * @brief  Indicate if the LOSC is ok interrupt is enabled
 * @rmtoll IE         LOSC_OK_IE           LL_LCSS_VBAT_IsEnabledIT_LOSCOK
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_IsEnabledIT_LOSCOK(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->IE, LCSS_VBAT_IE_LOSC_OK_IE) == (LCSS_VBAT_IE_LOSC_OK_IE));
}

/**
 * @brief  Enable RCL stopped interrupt
 * @rmtoll IE         RCL_STOP_IE           LL_LCSS_VBAT_EnableIT_RCLSTOP
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_EnableIT_RCLSTOP(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    SET_BIT(LCSS_VBATx->IE, LCSS_VBAT_IE_RCL_STOP_IE);
}

/**
 * @brief  Disable RCL stopped interrupt
 * @rmtoll IE         RCL_STOP_IE           LL_LCSS_VBAT_DisableIT_RCLSTOP
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_DisableIT_RCLSTOP(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    CLEAR_BIT(LCSS_VBATx->IE, LCSS_VBAT_IE_RCL_STOP_IE);
}

/**
 * @brief  Indicate if the RCL stopped interrupt is enabled
 * @rmtoll IE         RCL_STOP_IE           LL_LCSS_VBAT_IsEnabledIT_RCLSTOP
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_IsEnabledIT_RCLSTOP(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->IE, LCSS_VBAT_IE_RCL_STOP_IE) == (LCSS_VBAT_IE_RCL_STOP_IE));
}

/**
 * @brief  Enable RCL is ok interrupt
 * @rmtoll IE         RCL_OK_IE           LL_LCSS_VBAT_EnableIT_RCLOK
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_EnableIT_RCLOK(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    SET_BIT(LCSS_VBATx->IE, LCSS_VBAT_IE_RCL_OK_IE);
}

/**
 * @brief  Disable RCL is ok interrupt
 * @rmtoll IE         RCL_OK_IE           LL_LCSS_VBAT_DisableIT_RCLOK
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_DisableIT_RCLOK(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    CLEAR_BIT(LCSS_VBATx->IE, LCSS_VBAT_IE_RCL_OK_IE);
}

/**
 * @brief  Indicate if the RCL is ok interrupt is enabled
 * @rmtoll IE         RCL_OK_IE           LL_LCSS_VBAT_IsEnabledIT_RCLOK
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_IsEnabledIT_RCLOK(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->IE, LCSS_VBAT_IE_RCL_OK_IE) == (LCSS_VBAT_IE_RCL_OK_IE));
}

/**
 * @}
 */

/** @defgroup LCSS_VBAT_LL_EF_FLAG_management FLAG_management
 * @{
 */

/**
 * @brief  Check if the LOSC_STOP Flag is set or not
 * @note   when LCSS_VBAT_EN=0, the state will not be automatically updated
 *         (this bit must be written 1 twice to clear)
 * @rmtoll IF          LOSC_STOP_IF           LL_LCSS_VBAT_IsActiveFlag_LOSCSTOP
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_IsActiveFlag_LOSCSTOP(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->IF, LCSS_VBAT_IF_LOSC_STOP_IF) == (LCSS_VBAT_IF_LOSC_STOP_IF));
}

/**
 * @brief  Read the LOSC_STOP Flag
 * @note   when LCSS_VBAT_EN=0, the state will not be automatically updated
 *         (this bit must be written 1 twice to clear)
 * @rmtoll IF          LOSC_STOP_IF           LL_LCSS_VBAT_ClearFlag_LOSCSTOP
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_ClearFlag_LOSCSTOP(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    WRITE_REG(LCSS_VBATx->IF, LCSS_VBAT_IF_LOSC_STOP_IF);
    WRITE_REG(LCSS_VBATx->IF, LCSS_VBAT_IF_LOSC_STOP_IF);
}

/**
 * @brief  Check if the LOSC_OK Flag is set or not
 * @note   when LCSS_VBAT_EN=0, the state will not be automatically updated
 *         (this bit must be written 1 twice to clear)
 * @rmtoll IF          LOSC_OK           LL_LCSS_VBAT_IsActiveFlag_LOSCOK
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_IsActiveFlag_LOSCOK(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->IF, LCSS_VBAT_IF_LOSC_OK) == (LCSS_VBAT_IF_LOSC_OK));
}

/**
 * @brief  Read the LOSC_OK Flag
 * @note   when LCSS_VBAT_EN=0, the state will not be automatically updated
 *         (this bit must be written 1 twice to clear)
 * @rmtoll IF          LOSC_OK           LL_LCSS_VBAT_ClearFlag_LOSCOK
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_ClearFlag_LOSCOK(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    WRITE_REG(LCSS_VBATx->IF, LCSS_VBAT_IF_LOSC_OK);
    WRITE_REG(LCSS_VBATx->IF, LCSS_VBAT_IF_LOSC_OK);
}

/**
 * @brief  Check if the RCL_STOP Flag is set or not
 * @note   when LCSS_VBAT_EN=0, the state will not be automatically updated
 *         (this bit must be written 1 twice to clear)
 * @rmtoll IF          RCL_STOP_IF           LL_LCSS_VBAT_IsActiveFlag_RCLSTOP
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_IsActiveFlag_RCLSTOP(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->IF, LCSS_VBAT_IF_RCL_STOP_IF) == (LCSS_VBAT_IF_RCL_STOP_IF));
}

/**
 * @brief  Read the RCL_STOP Flag
 * @note   when LCSS_VBAT_EN=0, the state will not be automatically updated
 *         (this bit must be written 1 twice to clear)
 * @rmtoll IF          RCL_STOP_IF           LL_LCSS_VBAT_ClearFlag_RCLSTOP
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_ClearFlag_RCLSTOP(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    WRITE_REG(LCSS_VBATx->IF, LCSS_VBAT_IF_RCL_STOP_IF);
    WRITE_REG(LCSS_VBATx->IF, LCSS_VBAT_IF_RCL_STOP_IF);
}

/**
 * @brief  Check if the RCL_OK Flag is set or not
 * @note   when LCSS_VBAT_EN=0, the state will not be automatically updated
 *         (this bit must be written 1 twice to clear)
 * @rmtoll IF          RCL_OK           LL_LCSS_VBAT_IsActiveFlag_RCLOK
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VBAT_IsActiveFlag_RCLOK(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    return (READ_BIT(LCSS_VBATx->IF, LCSS_VBAT_IF_RCL_OK) == (LCSS_VBAT_IF_RCL_OK));
}

/**
 * @brief  Read the RCL_OK Flag
 * @note   when LCSS_VBAT_EN=0, the state will not be automatically updated
 *         (this bit must be written 1 twice to clear)
 * @rmtoll IF          RCL_OK           LL_LCSS_VBAT_ClearFlag_RCLOK
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VBAT_ClearFlag_RCLOK(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    WRITE_REG(LCSS_VBATx->IF, LCSS_VBAT_IF_RCL_OK);
    WRITE_REG(LCSS_VBATx->IF, LCSS_VBAT_IF_RCL_OK);
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup LCSS_VBAT_LL_EF_Init Initialization and de-initialization functions
 * @{
 */

// The initialization function can have more than one argument.
ErrorStatus LL_LCSS_VBAT_Init(LCSS_VBAT_TypeDef *LCSS_VBATx, LL_LCSS_VBAT_InitTypeDef *LCSS_VBAT_InitStruct);
ErrorStatus LL_LCSS_VBAT_DeInit(LCSS_VBAT_TypeDef *LCSS_VBATx);
void LL_LCSS_VBAT_StructInit(LL_LCSS_VBAT_InitTypeDef *LCSS_VBAT_InitStruct); // If the module is not suitable for the default configuration, you could not defines this function.

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

#endif /* LCSS_VBAT */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_LCSS_VBAT_H */
