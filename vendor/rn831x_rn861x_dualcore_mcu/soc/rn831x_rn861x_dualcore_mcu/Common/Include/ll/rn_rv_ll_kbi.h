/**
 *****************************************************************************
 * @file   rn_rv_ll_kbi.h
 * @author Renergy Software Team
 * @brief  Header file of KBI LL module.
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
#ifndef __RN_LL_KBI_H
#define __RN_LL_KBI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_kbi.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (KBI0) || defined (KBI1)

/** @defgroup KBI_LL KBI
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup KBI_LL_Private_Constants KBI Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup KBI_LL_Private_Macros KBI Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup KBI_LL_ES_INIT KBI Exported Init structure
 * @{
 */

/**
 * @brief LL KBI Init Structure definition
 */
typedef struct
{
    uint32_t Channel_0_15;          /*!< Specifies the KBI Channels to be enabled or disabled for Channels in range 0 to 15
                                        This parameter can be any combination of @ref KBI_LL_EC_CHANNEL */

    FunctionalState ITCommand;      /*!< Specifies the new state of the selected KBI Channels interrupt.
                                        This parameter can be set either to ENABLE or DISABLE */

    uint32_t TriggerMode;           /*!< Specifies the Trigger Mode.
                                        This parameter can be a value of @ref KBI_LL_EC_Trigger_Mode.

                                        This feature can be modified afterwards using unitary function @ref LL_KBI_SetTriggerModeChannel_0_15() */

    uint32_t ClockSelect;           /*!< Specifies the Filter Clock Select.
                                        This parameter can be a value of @ref KBI_LL_EC_FILTER_CLOCK_SELECT.

                                        This feature can be modified afterwards using unitary function @ref LL_KBI_SetAddress(). */

    uint32_t ClockDiv1;             /*!< Specifies the Filter Clock Division 1.
                                        This parameter can be a value of @ref KBI_LL_EC_FILTER_CLOCK_DIV1.

                                        This feature can be modified afterwards using unitary function @ref LL_KBI_SetFilterClockDiv1_0_15(). */

    uint32_t ClockDiv2;             /*!< Specifies the Filter Clock Division 2.
                                        This parameter must be set to a value between Min_Data = 0x00 and Max_Data = 0x1F

                                        This feature can be modified afterwards using unitary function @ref LL_KBI_SetFilterClockDiv2_0_15(). */
} LL_KBI_InitTypeDef;

/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/* Exported constants -------------------------------------------------------*/
/** @defgroup KBI_LL_Exported_Constants KBI Exported Constants
 * @{
 */

/** @defgroup KBI_LL_EC_CHANNEL Channel
 * @{
 */
#define LL_KBI_CHANNEL_0                    (BIT(0))                        /*!< Select Channel 0   */
#define LL_KBI_CHANNEL_1                    (BIT(1))                        /*!< Select Channel 1   */
#define LL_KBI_CHANNEL_2                    (BIT(2))                        /*!< Select Channel 2   */
#define LL_KBI_CHANNEL_3                    (BIT(3))                        /*!< Select Channel 3   */
#define LL_KBI_CHANNEL_4                    (BIT(4))                        /*!< Select Channel 4   */
#define LL_KBI_CHANNEL_5                    (BIT(5))                        /*!< Select Channel 5   */
#define LL_KBI_CHANNEL_6                    (BIT(6))                        /*!< Select Channel 6   */
#define LL_KBI_CHANNEL_7                    (BIT(7))                        /*!< Select Channel 7   */
#define LL_KBI_CHANNEL_8                    (BIT(8))                        /*!< Select Channel 8   */
#define LL_KBI_CHANNEL_9                    (BIT(9))                        /*!< Select Channel 9   */
#define LL_KBI_CHANNEL_10                   (BIT(10))                       /*!< Select Channel 10  */
#define LL_KBI_CHANNEL_11                   (BIT(11))                       /*!< Select Channel 11  */
#define LL_KBI_CHANNEL_12                   (BIT(12))                       /*!< Select Channel 12  */
#define LL_KBI_CHANNEL_13                   (BIT(13))                       /*!< Select Channel 13  */
#define LL_KBI_CHANNEL_14                   (BIT(14))                       /*!< Select Channel 14  */
#define LL_KBI_CHANNEL_15                   (BIT(15))                       /*!< Select Channel 15  */
#define LL_KBI_CHANNEL_ALL                  (BITS(0,15))                    /*!< Select all Channel */
/**
 * @}
 */

/** @defgroup KBI_LL_EC_Trigger_Mode Trigger_Mode (Edge Trigger)
 * @{
 */
#define LL_KBI_TRIGGER_RISING               KBI_MODE_MODE0_RISING           /*!< Trigger Rising Mode           */
#define LL_KBI_TRIGGER_FALLING              KBI_MODE_MODE0_FALLING          /*!< Trigger Falling Mode          */
#define LL_KBI_TRIGGER_RISING_FALLING       KBI_MODE_MODE0_RISING_FALLING   /*!< Trigger Rising & Falling Mode */
/**
 * @}
 */

/** @defgroup KBI_LL_EC_FILTER_CLOCK_SELECT Filter Clock Select
 * @{
 */
#define LL_KBI_FILTER_CLOCK_32_768          0x00000000U                     /*!< Filter clock is 32.768kHz */
#define LL_KBI_FILTER_CLOCK_SYSCLK          KBI_CH_FLT_TIME_CLK_SEL        /*!< Filter clock is sys_clk   */
/**
 * @}
 */

/** @defgroup KBI_LL_EC_FILTER_CLOCK_DIV1 Filter Clock Division 1
 * @{
 */
#define LL_KBI_FILTER_CLOCK_DIV_NONE        0x00000000U                     /*!< No frequency division for division level 1 */
#define LL_KBI_FILTER_CLOCK_DIV_32          KBI_CH_FLT_TIME_CLK_DIV1       /*!< 32 frequency division for division level 1 */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup KBI_LL_Exported_Macros KBI Exported Macros
 * @{
 */
/** @defgroup KBI_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in KBI register
 * @param  __INSTANCE__ KBI Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_KBI_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in KBI register
 * @param  __INSTANCE__ KBI Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_KBI_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup KBI_LL_Exported_Functions KBI Exported Functions
 * @{
 */

/** @defgroup KBI_LL_EF_Channel_Management Channel_Management
 * @{
 */

/**
 * @brief  Enable KBI Event request for Channels in range 0 to 15
 * @rmtoll CTRL         Enable           LL_KBI_EnableChannel_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 *         @arg @ref LL_KBI_CHANNEL_ALL
 * @retval None
 */
__STATIC_INLINE void LL_KBI_EnableChannel_0_15 (KBI_TypeDef *KBIx, uint32_t Channel)
{
    SET_BIT(KBIx->CTRL, Channel);
}

/**
 * @brief  Disable KBI Event request for Channels in range 0 to 15
 * @rmtoll CTRL         Enable           LL_KBI_DisableChannel_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 *         @arg @ref LL_KBI_CHANNEL_ALL
 * @retval None
 */
__STATIC_INLINE void LL_KBI_DisableChannel_0_15(KBI_TypeDef *KBIx, uint32_t Channel)
{
    CLEAR_BIT(KBIx->CTRL, Channel);
}

/**
 * @brief  Indicate if KBI Event request is enabled for Channels in range 0 to 15
 * @rmtoll CTRL         Enable           LL_KBI_IsEnabledChannel_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 *         @arg @ref LL_KBI_CHANNEL_ALL
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_KBI_IsEnabledChannel_0_15(KBI_TypeDef *KBIx, uint32_t Channel)
{
    return (READ_BIT(KBIx->CTRL, Channel) == (Channel));
}

/**
 * @}
 */

/** @defgroup KBI_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable KBI Interrupt request for Channels in range 0 to 15
 * @rmtoll MASK         MASK           LL_KBI_EnableIT_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 *         @arg @ref LL_KBI_CHANNEL_ALL
 * @retval None
 */
__STATIC_INLINE void LL_KBI_EnableIT_0_15(KBI_TypeDef *KBIx, uint32_t Channel)
{
    SET_BIT(KBIx->MASK, Channel);
}

/**
 * @brief  Disable KBI Interrupt request for Channels in range 0 to 15
 * @rmtoll MASK         MASK           LL_KBI_DisableIT_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 *         @arg @ref LL_KBI_CHANNEL_ALL
 * @retval None
 */
__STATIC_INLINE void LL_KBI_DisableIT_0_15(KBI_TypeDef *KBIx, uint32_t Channel)
{
    CLEAR_BIT(KBIx->MASK, Channel);
}

/**
 * @brief  Indicate if the KBI Interrupt request for Channels in range 0 to 15
 * @rmtoll MASK         MASK           LL_KBI_IsEnabledIT_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 *         @arg @ref LL_KBI_CHANNEL_ALL
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_KBI_IsEnabledIT_0_15(KBI_TypeDef *KBIx, uint32_t Channel)
{
    return (READ_BIT(KBIx->MASK, Channel) == (Channel));
}

/**
 * @}
 */

/** @defgroup KBI_LL_EF_Trigger_Mode_Management Trigger_Mode_Management
 * @{
 */

/**
 * @brief  Set KBI Trigger mode for Channels in range 0 to 15
 * @rmtoll MODE        MODEx           LL_KBI_SetTriggerModeChannel_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 *         @arg @ref LL_KBI_CHANNEL_ALL
 * @param  Mode This parameter can be one of the following values:
 *         @arg @ref LL_KBI_TRIGGER_RISING
 *         @arg @ref LL_KBI_TRIGGER_FALLING
 *         @arg @ref LL_KBI_TRIGGER_RISING_FALLING
 * @retval None
 */
__STATIC_INLINE void LL_KBI_SetTriggerModeChannel_0_15(KBI_TypeDef *KBIx, uint32_t Channel, uint32_t TriggerMode)
{
    MODIFY_REG(KBIx->MODE, (KBI_MODE_MODE0 << (POSITION_VAL(Channel) * 2U)), (TriggerMode << (POSITION_VAL(Channel) * 2U)));
}

/**
 * @brief  Get KBI Trigger mode for Channels in range 0 to 15
 * @rmtoll MODE        MODEx           LL_KBI_GetTriggerModeChannel_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 *         @arg @ref LL_KBI_CHANNEL_ALL
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_KBI_TRIGGER_RISING
 *         @arg @ref LL_KBI_TRIGGER_FALLING
 *         @arg @ref LL_KBI_TRIGGER_RISING_FALLING
 */
__STATIC_INLINE uint32_t LL_KBI_GetTriggerModeChannel_0_15(KBI_TypeDef *KBIx, uint32_t Channel)
{
  return (uint32_t)(READ_BIT(KBIx->MODE, (KBI_MODE_MODE0 << (POSITION_VAL(Channel) * 2U))) >> (POSITION_VAL(Channel) * 2U));
}

/**
 * @}
 */

/** @defgroup KBI_LL_EF_FLAG_management FLAG_management
 * @{
 */

/**
 * @brief  Check if the KBI Flag is set or not for Channels in range 0 to 15
 * @note   This bit is set when the selected edge event arrives on the interrupt
 *         line. This bit is cleared by writing a 1 to the bit.
 * @rmtoll STA          STA           LL_KBI_IsActiveFlag_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be a combination of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 *         @arg @ref LL_KBI_CHANNEL_ALL
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_KBI_IsActiveFlag_0_15(KBI_TypeDef *KBIx, uint32_t Channel)
{
    return (READ_BIT(KBIx->STA, Channel) == (Channel));
}

/**
 * @brief  Read KBI Combination Interrupt Flag for Channels in range 0 to 15
 * @note   This bit is set when the selected edge event arrives on the interrupt
 *         line. This bit is cleared by writing a 1 to the bit.
 * @rmtoll STA          STA           LL_KBI_ReadFlag_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be a combination of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 *         @arg @ref LL_KBI_CHANNEL_ALL
 * @retval @note This bit is set when the selected edge event arrives on the interrupt
 */
__STATIC_INLINE uint32_t LL_KBI_ReadFlag_0_15(KBI_TypeDef *KBIx, uint32_t Channel)
{
    return (uint32_t)(READ_BIT(KBIx->STA, Channel));
}

/**
 * @brief  Clear KBI Flags for Channels in range 0 to 15
 * @note   This bit is set when the selected edge event arrives on the interrupt
 *         line. This bit is cleared by writing a 1 to the bit.
 * @rmtoll STA          STA           LL_KBI_ClearFlag_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be a combination of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 *         @arg @ref LL_KBI_CHANNEL_ALL
 * @retval None
 */
__STATIC_INLINE void LL_KBI_ClearFlag_0_15(KBI_TypeDef *KBIx, uint32_t Channel)
{
    WRITE_REG(KBIx->STA, Channel);
}

/**
 * @}
 */

/** @defgroup KBI_LL_EF_Filter_Time_Management Filter_Time_Management
 * @{
 */

/**
 * @brief  Set Filter Clock for Channels in range 0 to 15
 * @rmtoll CHx_FLT_TIME          CLK_SEL           LL_KBI_SetFilterClock_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 * @param  ClockSelect This parameter can be one of the following values:
 *         @arg @ref LL_KBI_FILTER_CLOCK_32_768
 *         @arg @ref LL_KBI_FILTER_CLOCK_SYSCLK
 * @retval None
 */
__STATIC_INLINE void LL_KBI_SetFilterClock_0_15(KBI_TypeDef *KBIx, uint32_t Channel, uint32_t ClockSelect)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&KBIx->CH_FLT_TIME[POSITION_VAL(Channel)]) + (4 * POSITION_VAL(Channel))));
    MODIFY_REG(*pReg, KBI_CH_FLT_TIME_CLK_SEL, ClockSelect);
}

/**
 * @brief  Get Filter Clock for Channels in range 0 to 15
 * @rmtoll CHx_FLT_TIME          CLK_SEL           LL_KBI_GetFilterClock_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_KBI_FILTER_CLOCK_32_768
 *         @arg @ref LL_KBI_FILTER_CLOCK_SYSCLK
 */
__STATIC_INLINE uint32_t LL_KBI_GetFilterClock_0_15(KBI_TypeDef *KBIx, uint32_t Channel)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&KBIx->CH_FLT_TIME[POSITION_VAL(Channel)]) + (4 * POSITION_VAL(Channel))));
    return (READ_BIT(*pReg, KBI_CH_FLT_TIME_CLK_SEL));
}

/**
 * @brief  Set Filter Clock Division 1 for Channels in range 0 to 15
 * @rmtoll CHx_FLT_TIME          CLK_DIV1           LL_KBI_SetFilterClockDiv1_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 * @param  ClockSelect This parameter can be one of the following values:
 *         @arg @ref LL_KBI_FILTER_CLOCK_DIV_NONE
 *         @arg @ref LL_KBI_FILTER_CLOCK_DIV_32
 * @retval None
 */
__STATIC_INLINE void LL_KBI_SetFilterClockDiv1_0_15(KBI_TypeDef *KBIx, uint32_t Channel, uint32_t ClockDiv1)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&KBIx->CH_FLT_TIME[POSITION_VAL(Channel)]) + (4 * POSITION_VAL(Channel))));
    MODIFY_REG(*pReg, KBI_CH_FLT_TIME_CLK_DIV1, ClockDiv1);
}

/**
 * @brief  Get Filter Clock Division 1 for Channels in range 0 to 15
 * @rmtoll CHx_FLT_TIME          CLK_DIV1           LL_KBI_GetFilterClockDiv1_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_KBI_FILTER_CLOCK_DIV_NONE
 *         @arg @ref LL_KBI_FILTER_CLOCK_DIV_32
 */
__STATIC_INLINE uint32_t LL_KBI_GetFilterClockDiv1_0_15(KBI_TypeDef *KBIx, uint32_t Channel)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&KBIx->CH_FLT_TIME[POSITION_VAL(Channel)]) + (4 * POSITION_VAL(Channel))));
    return (READ_BIT(*pReg, KBI_CH_FLT_TIME_CLK_DIV1));
}

/**
 * @brief  Set Filter Clock Division 2 for Channels in range 0 to 15
 * @rmtoll CHx_FLT_TIME          CLK_DIV2           LL_KBI_SetFilterClockDiv2_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 * @param  ClockDiv2 Value between Min_Data=0x00 and Max_Data=0x1F
 * @retval None
 */
__STATIC_INLINE void LL_KBI_SetFilterClockDiv2_0_15(KBI_TypeDef *KBIx, uint32_t Channel, uint32_t ClockDiv2)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&KBIx->CH_FLT_TIME[POSITION_VAL(Channel)]) + (4 * POSITION_VAL(Channel))));
    MODIFY_REG(*pReg, KBI_CH_FLT_TIME_CLK_DIV2, (ClockDiv2 << KBI_CH_FLT_TIME_CLK_DIV2_POS));
}

/**
 * @brief  Get Filter Clock Division 2 for Channels in range 0 to 15
 * @rmtoll CHx_FLT_TIME          CLK_DIV2           LL_KBI_GetFilterClockDiv2_0_15
 * @param  KBIx KBI Instance.
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_KBI_CHANNEL_0
 *         @arg @ref LL_KBI_CHANNEL_1
 *         @arg @ref LL_KBI_CHANNEL_2
 *         @arg @ref LL_KBI_CHANNEL_3
 *         @arg @ref LL_KBI_CHANNEL_4
 *         @arg @ref LL_KBI_CHANNEL_5
 *         @arg @ref LL_KBI_CHANNEL_6
 *         @arg @ref LL_KBI_CHANNEL_7
 *         @arg @ref LL_KBI_CHANNEL_8
 *         @arg @ref LL_KBI_CHANNEL_9
 *         @arg @ref LL_KBI_CHANNEL_10
 *         @arg @ref LL_KBI_CHANNEL_11
 *         @arg @ref LL_KBI_CHANNEL_12
 *         @arg @ref LL_KBI_CHANNEL_13
 *         @arg @ref LL_KBI_CHANNEL_14
 *         @arg @ref LL_KBI_CHANNEL_15
 *         @arg @ref LL_KBI_CHANNEL_ALL
 * @retval Filter Clock Division 2 (Value between Min_Data=0x00 and Max_Data=0x1F)
 */
__STATIC_INLINE uint32_t LL_KBI_GetFilterClockDiv2_0_15(KBI_TypeDef *KBIx, uint32_t Channel)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&KBIx->CH_FLT_TIME[POSITION_VAL(Channel)]) + (4 * POSITION_VAL(Channel))));
    return (READ_BIT(*pReg, KBI_CH_FLT_TIME_CLK_DIV2) >> KBI_CH_FLT_TIME_CLK_DIV2_POS);
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup KBI_LL_EF_Init Initialization and de-initialization functions
 * @{
 */

// The initialization function can have more than one argument.
ErrorStatus LL_KBI_Init(KBI_TypeDef *KBIx, LL_KBI_InitTypeDef *KBI_InitStruct);
ErrorStatus LL_KBI_DeInit(KBI_TypeDef *KBIx);
void LL_KBI_StructInit(LL_KBI_InitTypeDef *KBI_InitStruct); // If the module is not suitable for the default configuration, you could not defines this function.

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

#endif /* KBI0 || KBI1 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_KBI_H */
