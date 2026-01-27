/**
 ******************************************************************************
 * @file   rn_rv_ll_dac.h
 * @author Renergy Software Team
 * @brief  Header file of DAC LL module driver.
 *         Brief description
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
#ifndef __RN_RV_LL_DAC_H
#define __RN_RV_LL_DAC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu.h"
#include "rn831x_rn861x_mcu_dac.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (DAC_CORE0) || defined (DAC_CORE1)

/** @defgroup DAC_LL DAC
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/** @defgroup DAC_LL_ES_INIT DAC Exported Init structure
 * @{
 */

/**
 * @brief DAC initial structure definition.
 */
typedef struct
{
    uint32_t DataAlignment;        /*!< Specifies the data alignment value .
                                       This parameter can be a value of @ref DAC_LL_EC_DATA_ALIGNMENT.
                                       This feature can be modified afterwards using unitary function @ref LL_DAC_SetDataAlignment().*/

    uint32_t ClockDivision;        /*!< Specifies the clock division value.
                                       This parameter must be a number between Min_Data=0x0 and Max_Data=0xFFFF.

                                       This feature can be modified afterwards using unitary function @ref LL_DAC_SetClockDivision()}.*/
    uint32_t OutputChannel;        /*!< Specifies the outputchannel value .
                                       This parameter can be a value of @ref DAC_LL_EC_OUTPUT_CHANNEL.
                                       This feature can be modified afterwards using unitary function @ref LL_DAC_SetOutputChannel().*/
    uint32_t TriggerMode;        /*!< Specifies the trigger mode value.
                                       This parameter can be a value of @ref DAC_LL_EC_TRIGGER_MODE.
                                       This feature can be modified afterwards using unitary function @ref LL_DAC_SetTriggerMode().*/

} LL_DAC_InitTypeDef;

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/* Exported macro -----------------------------------------------------------*/
/** @defgroup DAC_LL_Exported_Macros DAC Exported Macros
 * @{
 */

/** @defgroup DAC_LL_EC_OUTPUT_BUFFER Output Buffer Selection
 * @{
 */
#define LL_DAC_OUTPUT_BUFFER_DISABLE         (DAC_CFG_SEL_BUFF_NO_BUFF)              /*!< Output not through buffer */
#define LL_DAC_OUTPUT_BUFFER_ENABLE          (DAC_CFG_SEL_BUFF_BUFF)                 /*!< Output through buffer */
/**
 * @}
 */ /* end of group DAC_LL_EC_OUTPUT_BUFFER */

/** @defgroup DAC_LL_EC_INITIAL_OUTPUT_STATE Initial Output State
 * @{
 */
#define LL_DAC_INITIAL_OUTPUT_HIGHZ          (DAC_CFG_INIT_MODE_HIGHZ)               /*!< Analog output in high-Z state when channel disabled */
#define LL_DAC_INITIAL_OUTPUT_GND            (DAC_CFG_INIT_MODE_GND)                 /*!< Analog output connected to ground when channel disabled */
/**
 * @}
 */ /* end of group DAC_LL_EC_INITIAL_OUTPUT_STATE */

/** @defgroup DAC_LL_EC_TRIGGER_MODE Trigger Mode
 * @{
 */
#define LL_DAC_TRIGGER_CONTINUOUS            (DAC_CFG_TRIG_CTRL_COUTINUE)            /*!< Continuous output mode */
#define LL_DAC_TRIGGER_SOFTWARE              (DAC_CFG_TRIG_CTRL_SWT)                 /*!< Software trigger mode */
#define LL_DAC_TRIGGER_EXTERNAL              (DAC_CFG_TRIG_CTRL_EXT)                 /*!< External trigger mode */
/**
 * @}
 */ /* end of group DAC_LL_EC_TRIGGER_MODE */

/** @defgroup DAC_LL_EC_DATA_ALIGNMENT Data Alignment
 * @{
 */
#define LL_DAC_DATA_ALIGN_RIGHT              (DAC_CFG_DPSEL_RA)                      /*!< Right-aligned data */
#define LL_DAC_DATA_ALIGN_LEFT               (DAC_CFG_DPSEL_LA)                      /*!< Left-aligned data */
/**
 * @}
 */ /* end of group DAC_LL_EC_DATA_ALIGNMENT */

/** @defgroup DAC_LL_EC_ADP_TIMEOUT_HANDLING ADP Timeout Handling Method
 * @{
 */
#define LL_DAC_ADP_TIMEOUT_WAIT_AD_DONE      (DAC_CFG_ADP_CTRL_WAITAD_DONE)          /*!< Wait until A/D conversion completes after timeout */
#define LL_DAC_ADP_TIMEOUT_ONLY_TIMEOUT     (DAC_CFG_ADP_CTRL_ONLYTIMEOUT)          /*!< Do not wait for A/D conversion completion after timeout */
/**
 * @}
 */ /* end of group DAC_LL_EC_ADP_TIMEOUT_HANDLING */

/** @defgroup DAC_LL_EC_BUFFER_STABILIZATION Buffer Stabilization Time
 * @{
 */
#define LL_DAC_BUFFER_STAB_1TOCLK            (DAC_CFG_TBUFF_1TOCLK)                  /*!< 1Toclk */
#define LL_DAC_BUFFER_STAB_2TOCLK            (DAC_CFG_TBUFF_2TOCLK)                  /*!< 2Toclk (default) */
#define LL_DAC_BUFFER_STAB_3TOCLK            (DAC_CFG_TBUFF_3TOCLK)                  /*!< 3Toclk */
#define LL_DAC_BUFFER_STAB_4TOCLK            (DAC_CFG_TBUFF_4TOCLK)                  /*!< 4Toclk */
/**
 * @}
 */ /* end of group DAC_LL_EC_BUFFER_STABILIZATION */

/** @defgroup DAC_LL_EC_OUTPUT_CHANNEL Output Channel Selection
 * @{
 */
#define LL_DAC_OUTPUT_CHANNEL_ALL_CLOSED     (DAC_CFG_OCH_EN_EN)                     /*!< All output channels are closed */
#define LL_DAC_OUTPUT_CHANNEL_PAD0           (DAC_CFG_OCH_SEL_PAD0)                  /*!< Output to PAD0 (P104) */
#define LL_DAC_OUTPUT_CHANNEL_PAD1           (DAC_CFG_OCH_SEL_PAD1)                  /*!< Output to PAD1 (P140) */
#define LL_DAC_OUTPUT_CHANNEL_ADC0           (DAC_CFG_OCH_SEL_ADC0)                  /*!< Output to ADC0 */
/**
 * @}
 */ /* end of group DAC_LL_EC_OUTPUT_CHANNEL */

/** @defgroup DAC_LL_EC_BufferPowerMode Buffer Power Mode
 * @{
 */
#define LL_DAC_BUFFER_POWER_MODE_ALWAYS_ON         DAC_CFG_BUFF_EN           /*!< Buffer analog circuit always on */
#define LL_DAC_BUFFER_POWER_MODE_DEMAND_ON         ((uint32_t)0x00000000U)   /*!< Buffer analog circuit only turns on when working */
/**
 * @}
 */

/**
 * @}
 */ /* end of group DAC_LL_Exported_Macros */

/* Exported functions -------------------------------------------------------*/
/** @defgroup DAC_LL_Exported_Functions DAC Exported Functions
 * @{
 */

/** @defgroup DAC_LL_EF_Configuration Configuration functions
 * @{
 */

/**
 * @brief  Enable DAC channel conversion
 * @rmtoll CFG         DAC_EN            LL_DAC_Enable
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_Enable(DAC_TypeDef *DACx)
{
    SET_BIT(DACx->CFG, DAC_CFG_DAC_EN);
}

/**
 * @brief  Disable DAC channel conversion
 * @rmtoll CFG         DAC_EN            LL_DAC_Disable
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_Disable(DAC_TypeDef *DACx)
{
    CLEAR_BIT(DACx->CFG, DAC_CFG_DAC_EN);
}

/**
 * @brief  Indicates whether the DAC channel conversion is enabled
 * @rmtoll CFG         DAC_EN            LL_DAC_IsEnabled
 * @param  DACx DAC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_DAC_IsEnabled(DAC_TypeDef *DACx)
{
    return (uint32_t)((READ_BIT(DACx->CFG, DAC_CFG_DAC_EN) == (DAC_CFG_DAC_EN)) ? 1UL : 0UL);
}

/**
 * @brief  Enable output buffer
 * @rmtoll CFG         BUFF_EN            LL_DAC_EnableOutputBuffer
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_EnableOutputBuffer(DAC_TypeDef *DACx)
{
    SET_BIT(DACx->CFG, DAC_CFG_BUFF_EN);
}

/**
 * @brief  Disable output buffer
 * @rmtoll CFG         BUFF_EN            LL_DAC_DisableOutputBuffer
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_DisableOutputBuffer(DAC_TypeDef *DACx)
{
    CLEAR_BIT(DACx->CFG, DAC_CFG_BUFF_EN);
}

/**
 * @brief  Indicates whether the output buffer is enabled
 * @note   This function only checks the buffer enable bit, not the selection bit
 * @rmtoll CFG         BUFF_EN            LL_DAC_IsEnabledOutputBuffer
 * @param  DACx DAC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_DAC_IsEnabledOutputBuffer(DAC_TypeDef *DACx)
{
    return (uint32_t)((READ_BIT(DACx->CFG, DAC_CFG_BUFF_EN) == (DAC_CFG_BUFF_EN)) ? 1UL : 0UL);
}

/**
 * @brief  Set the buffer power mode
 * @rmtoll CFG         BUFF_EN            LL_DAC_SetBufferPowerMode
 * @param  DACx DAC Instance
 * @param  PowerMode This parameter can be one of the following values:
 *         @arg @ref LL_DAC_BUFFER_POWER_MODE_ALWAYS_ON
 *         @arg @ref LL_DAC_BUFFER_POWER_MODE_DEMAND_ON
 * @retval None
 */
__STATIC_INLINE void LL_DAC_SetBufferPowerMode(DAC_TypeDef *DACx, uint32_t PowerMode)
{
    MODIFY_REG(DACx->CFG, DAC_CFG_BUFF_EN, PowerMode);
}

/**
 * @brief  Get the buffer power mode
 * @rmtoll CFG         BUFF_EN            LL_DAC_GetBufferPowerMode
 * @param  DACx DAC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DAC_BUFFER_POWER_MODE_ALWAYS_ON
 *         @arg @ref LL_DAC_BUFFER_POWER_MODE_DEMAND_ON
 */
__STATIC_INLINE uint32_t LL_DAC_GetBufferPowerMode(DAC_TypeDef *DACx)
{
    return (uint32_t)(READ_BIT(DACx->CFG, DAC_CFG_BUFF_EN));
}
/**
 * @brief  Set the initial output state
 * @rmtoll CFG         INIT_MODE            LL_DAC_SetInitialOutputState
 * @param  DACx DAC Instance
 * @param  OutputState This parameter can be one of the following values:
 *         @arg @ref LL_DAC_INITIAL_OUTPUT_HIGHZ
 *         @arg @ref LL_DAC_INITIAL_OUTPUT_GND
 * @retval None
 */
__STATIC_INLINE void LL_DAC_SetInitialOutputState(DAC_TypeDef *DACx, uint32_t OutputState)
{
    MODIFY_REG(DACx->CFG, DAC_CFG_INIT_MODE, OutputState);
}

/**
 * @brief  Get the initial output state
 * @rmtoll CFG         INIT_MODE            LL_DAC_GetInitialOutputState
 * @param  DACx DAC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DAC_INITIAL_OUTPUT_HIGHZ
 *         @arg @ref LL_DAC_INITIAL_OUTPUT_GND
 */
__STATIC_INLINE uint32_t LL_DAC_GetInitialOutputState(DAC_TypeDef *DACx)
{
    return (uint32_t)(READ_BIT(DACx->CFG, DAC_CFG_INIT_MODE));
}

/**
 * @brief  Enable DMA request
 * @note   When DMA enable, must config TRIG_CTRL as External trigger
 * @rmtoll CFG         DMA_EN            LL_DAC_EnableDMAReq
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_EnableDMAReq(DAC_TypeDef *DACx)
{
    SET_BIT(DACx->CFG, DAC_CFG_DMA_EN);
}

/**
 * @brief  Disable DMA request
 * @rmtoll CFG         DMA_EN            LL_DAC_DisableDMAReq
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_DisableDMAReq(DAC_TypeDef *DACx)
{
    CLEAR_BIT(DACx->CFG, DAC_CFG_DMA_EN);
}

/**
 * @brief  Indicates whether the DMA request is enabled
 * @rmtoll CFG         DMA_EN            LL_DAC_IsEnabledDMAReq
 * @param  DACx DAC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_DAC_IsEnabledDMAReq(DAC_TypeDef *DACx)
{
    return (uint32_t)((READ_BIT(DACx->CFG, DAC_CFG_DMA_EN) == (DAC_CFG_DMA_EN)) ? 1UL : 0UL);
}

/**
 * @brief  Set the trigger mode
 * @rmtoll CFG         TRIG_CTRL            LL_DAC_SetTriggerMode
 * @param  DACx DAC Instance
 * @param  TriggerMode This parameter can be one of the following values:
 *         @arg @ref LL_DAC_TRIGGER_CONTINUOUS
 *         @arg @ref LL_DAC_TRIGGER_SOFTWARE
 *         @arg @ref LL_DAC_TRIGGER_EXTERNAL
 * @retval None
 */
__STATIC_INLINE void LL_DAC_SetTriggerMode(DAC_TypeDef *DACx, uint32_t TriggerMode)
{
    MODIFY_REG(DACx->CFG, DAC_CFG_TRIG_CTRL, TriggerMode);
}

/**
 * @brief  Get the trigger mode
 * @rmtoll CFG         TRIG_CTRL            LL_DAC_GetTriggerMode
 * @param  DACx DAC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DAC_TRIGGER_CONTINUOUS
 *         @arg @ref LL_DAC_TRIGGER_SOFTWARE
 *         @arg @ref LL_DAC_TRIGGER_EXTERNAL
 */
__STATIC_INLINE uint32_t LL_DAC_GetTriggerMode(DAC_TypeDef *DACx)
{
    return (uint32_t)(READ_BIT(DACx->CFG, DAC_CFG_TRIG_CTRL));
}

/**
 * @brief  Enable sample and hold mode
 * @rmtoll CFG         SH_MODE            LL_DAC_EnableSampleHoldMode
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_EnableSampleHoldMode(DAC_TypeDef *DACx)
{
    SET_BIT(DACx->CFG, DAC_CFG_SH_MODE);
}

/**
 * @brief  Disable sample and hold mode
 * @rmtoll CFG         SH_MODE            LL_DAC_DisableSampleHoldMode
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_DisableSampleHoldMode(DAC_TypeDef *DACx)
{
    CLEAR_BIT(DACx->CFG, DAC_CFG_SH_MODE);
}

/**
 * @brief  Indicates whether the sample and hold mode is enabled
 * @rmtoll CFG         SH_MODE            LL_DAC_IsEnabledSampleHoldMode
 * @param  DACx DAC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_DAC_IsEnabledSampleHoldMode(DAC_TypeDef *DACx)
{
    return (uint32_t)((READ_BIT(DACx->CFG, DAC_CFG_SH_MODE) == (DAC_CFG_SH_MODE)) ? 1UL : 0UL);
}

/**
 * @brief  Set the data alignment format
 * @rmtoll CFG         DPSEL            LL_DAC_SetDataAlignment
 * @param  DACx DAC Instance
 * @param  DataAlignment This parameter can be one of the following values:
 *         @arg @ref LL_DAC_DATA_ALIGN_RIGHT
 *         @arg @ref LL_DAC_DATA_ALIGN_LEFT
 * @retval None
 */
__STATIC_INLINE void LL_DAC_SetDataAlignment(DAC_TypeDef *DACx, uint32_t DataAlignment)
{
    MODIFY_REG(DACx->CFG, DAC_CFG_DPSEL, DataAlignment);
}

/**
 * @brief  Get the data alignment format
 * @rmtoll CFG         DPSEL            LL_DAC_GetDataAlignment
 * @param  DACx DAC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DAC_DATA_ALIGN_RIGHT
 *         @arg @ref LL_DAC_DATA_ALIGN_LEFT
 */
__STATIC_INLINE uint32_t LL_DAC_GetDataAlignment(DAC_TypeDef *DACx)
{
    return (uint32_t)(READ_BIT(DACx->CFG, DAC_CFG_DPSEL));
}

/**
 * @brief  Enable A/D conversion priority function
 * @rmtoll CFG         ADPEN            LL_DAC_EnableADConvPriority
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_EnableADConvPriority(DAC_TypeDef *DACx)
{
    SET_BIT(DACx->CFG, DAC_CFG_ADPEN);
}

/**
 * @brief  Disable A/D conversion priority function
 * @rmtoll CFG         ADPEN            LL_DAC_DisableADConvPriority
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_DisableADConvPriority(DAC_TypeDef *DACx)
{
    CLEAR_BIT(DACx->CFG, DAC_CFG_ADPEN);
}

/**
 * @brief  Indicates whether the A/D conversion priority function is enabled
 * @rmtoll CFG         ADPEN            LL_DAC_IsEnabledADConvPriority
 * @param  DACx DAC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_DAC_IsEnabledADConvPriority(DAC_TypeDef *DACx)
{
    return (uint32_t)((READ_BIT(DACx->CFG, DAC_CFG_ADPEN) == (DAC_CFG_ADPEN)) ? 1UL : 0UL);
}

/**
 * @brief  Set the ADP timeout handling method
 * @note   Valid when A/D conversion priority function is enabled
 * @rmtoll CFG         ADP_CTRL            LL_DAC_SetADPTimeoutHandlingMethod
 * @param  DACx DAC Instance
 * @param  HandlingMethod This parameter can be one of the following values:
 *         @arg @ref LL_DAC_ADP_TIMEOUT_WAIT_AD_DONE
 *         @arg @ref LL_DAC_ADP_TIMEOUT_ONLY_TIMEOUT
 * @retval None
 */
__STATIC_INLINE void LL_DAC_SetADPTimeoutHandlingMethod(DAC_TypeDef *DACx, uint32_t HandlingMethod)
{
    MODIFY_REG(DACx->CFG, DAC_CFG_ADP_CTRL, HandlingMethod);
}

/**
 * @brief  Get the ADP timeout handling method
 * @rmtoll CFG         ADP_CTRL            LL_DAC_GetADPTimeoutHandlingMethod
 * @param  DACx DAC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DAC_ADP_TIMEOUT_WAIT_AD_DONE
 *         @arg @ref LL_DAC_ADP_TIMEOUT_ONLY_TIMEOUT
 */
__STATIC_INLINE uint32_t LL_DAC_GetADPTimeoutHandlingMethod(DAC_TypeDef *DACx)
{
    return (uint32_t)(READ_BIT(DACx->CFG, DAC_CFG_ADP_CTRL));
}

/**
 * @brief  Set the buffer stabilization time
 * @rmtoll CFG         TBUFF            LL_DAC_SetBufferStabilizationTime
 * @param  DACx DAC Instance
 * @param  StabilizationTime This parameter can be one of the following values:
 *         @arg @ref LL_DAC_BUFFER_STAB_1TOCLK
 *         @arg @ref LL_DAC_BUFFER_STAB_2TOCLK
 *         @arg @ref LL_DAC_BUFFER_STAB_3TOCLK
 *         @arg @ref LL_DAC_BUFFER_STAB_4TOCLK
 * @retval None
 */
__STATIC_INLINE void LL_DAC_SetBufferStabilizationTime(DAC_TypeDef *DACx, uint32_t StabilizationTime)
{
    MODIFY_REG(DACx->CFG, DAC_CFG_TBUFF, StabilizationTime);
}

/**
 * @brief  Get the buffer stabilization time
 * @rmtoll CFG         TBUFF            LL_DAC_GetBufferStabilizationTime
 * @param  DACx DAC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DAC_BUFFER_STAB_1TOCLK
 *         @arg @ref LL_DAC_BUFFER_STAB_2TOCLK
 *         @arg @ref LL_DAC_BUFFER_STAB_3TOCLK
 *         @arg @ref LL_DAC_BUFFER_STAB_4TOCLK
 */
__STATIC_INLINE uint32_t LL_DAC_GetBufferStabilizationTime(DAC_TypeDef *DACx)
{
    return (uint32_t)(READ_BIT(DACx->CFG, DAC_CFG_TBUFF));
}

/**
 * @brief  Set the output channel
 * @rmtoll CFG         OCH_EN,OCH_SEL            LL_DAC_SetOutputChannel
 * @param  DACx DAC Instance
 * @param  OutputChannel This parameter can be one of the following values:
 *         @arg @ref LL_DAC_OUTPUT_CHANNEL_ALL_CLOSED
 *         @arg @ref LL_DAC_OUTPUT_CHANNEL_PAD0
 *         @arg @ref LL_DAC_OUTPUT_CHANNEL_PAD1
 *         @arg @ref LL_DAC_OUTPUT_CHANNEL_ADC0
 * @retval None
 */
__STATIC_INLINE void LL_DAC_SetOutputChannel(DAC_TypeDef *DACx, uint32_t OutputChannel)
{
    if (OutputChannel == LL_DAC_OUTPUT_CHANNEL_ALL_CLOSED)
    {
        CLEAR_BIT(DACx->CFG, DAC_CFG_OCH_EN);
    }
    else
    {
        SET_BIT(DACx->CFG, DAC_CFG_OCH_EN);
        MODIFY_REG(DACx->CFG, DAC_CFG_OCH_SEL, OutputChannel);
    }
}

/**
 * @brief  Get the output channel
 * @rmtoll CFG         OCH_EN,OCH_SEL            LL_DAC_GetOutputChannel
 * @param  DACx DAC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DAC_OUTPUT_CHANNEL_ALL_CLOSED
 *         @arg @ref LL_DAC_OUTPUT_CHANNEL_PAD0
 *         @arg @ref LL_DAC_OUTPUT_CHANNEL_PAD1
 *         @arg @ref LL_DAC_OUTPUT_CHANNEL_ADC0
 */
__STATIC_INLINE uint32_t LL_DAC_GetOutputChannel(DAC_TypeDef *DACx)
{
    if (READ_BIT(DACx->CFG, DAC_CFG_OCH_EN) == 0)
    {
        return LL_DAC_OUTPUT_CHANNEL_ALL_CLOSED;
    }
    else
    {
        return (uint32_t)(READ_BIT(DACx->CFG, DAC_CFG_OCH_SEL));
    }
}

/**
 * @brief  Set the clock division
 * @note   Toclk = Tsys_clk/(CONV_DIV+1) (refer to user manual for recommend value)
 * @rmtoll CFG         CONV_DIV            LL_DAC_SetClockDivision
 * @param  DACx DAC Instance
 * @param  ClockDivision Between 0 to 0xFFFF
 * @retval None
 */
__STATIC_INLINE void LL_DAC_SetClockDivision(DAC_TypeDef *DACx, uint32_t ClockDivision)
{
    MODIFY_REG(DACx->CFG, DAC_CFG_CONV_DIV, (ClockDivision << DAC_CFG_CONV_DIV_POS));
}

/**
 * @brief  Get the clock division
 * @rmtoll CFG         CONV_DIV            LL_DAC_GetClockDivision
 * @param  DACx DAC Instance
 * @retval Clock division value
 */
__STATIC_INLINE uint32_t LL_DAC_GetClockDivision(DAC_TypeDef *DACx)
{
    return (uint32_t)(READ_BIT(DACx->CFG, DAC_CFG_CONV_DIV) >> DAC_CFG_CONV_DIV_POS);
}

/**
 * @}
 */ /* end of group DAC_LL_EF_Configuration */

/** @defgroup DAC_LL_EF_Configuration_ADPOT  Configuration  ADPOT functions
 * @{
 */

/**
 * @brief  Set the A/D conversion priority timeout
 * @note   Timeout configuration in conversion clock cycles Toclk
 * @rmtoll ADPOT         ADP_OVT            LL_DAC_SetADConvPriorityTimeout
 * @param  DACx DAC Instance
 * @param  ADConvPriorityTimeout Between 0 to 0xFFFF
 * @retval None
 */
__STATIC_INLINE void LL_DAC_SetADConvPriorityTimeout(DAC_TypeDef *DACx, uint32_t ADConvPriorityTimeout)
{
    MODIFY_REG(DACx->ADPOT, DAC_ADPOT_ADP_OVT, (ADConvPriorityTimeout << DAC_ADPOT_ADP_OVT_POS));
}

/**
 * @brief  Get the A/D conversion priority timeout
 * @rmtoll ADPOT         ADP_OVT            LL_DAC_GetADConvPriorityTimeout
 * @param  DACx DAC Instance
 * @retval A/D conversion priority timeout value
 */
__STATIC_INLINE uint32_t LL_DAC_GetADConvPriorityTimeout(DAC_TypeDef *DACx)
{
    return (uint32_t)(READ_BIT(DACx->ADPOT, DAC_ADPOT_ADP_OVT) >> DAC_ADPOT_ADP_OVT_POS);
}

/**
 * @brief  Set the busy flag assertion time
 * @note   Busy flag assertion time in system clock cycles Tsys_clk
 * @rmtoll ADPOT         DAC_BUSYT            LL_DAC_SetBusyFlagAssertionTime
 * @param  DACx DAC Instance
 * @param  BusyFlagAssertionTime Between 0 to 0xF
 * @retval None
 */
__STATIC_INLINE void LL_DAC_SetBusyFlagAssertionTime(DAC_TypeDef *DACx, uint32_t BusyFlagAssertionTime)
{
    MODIFY_REG(DACx->ADPOT, DAC_ADPOT_DAC_BUSYT, (BusyFlagAssertionTime << DAC_ADPOT_DAC_BUSYT_POS));
}

/**
 * @brief  Get the busy flag assertion time
 * @rmtoll ADPOT         DAC_BUSYT            LL_DAC_GetBusyFlagAssertionTime
 * @param  DACx DAC Instance
 * @retval Busy flag assertion time value
 */
__STATIC_INLINE uint32_t LL_DAC_GetBusyFlagAssertionTime(DAC_TypeDef *DACx)
{
    return (uint32_t)(READ_BIT(DACx->ADPOT, DAC_ADPOT_DAC_BUSYT) >> DAC_ADPOT_DAC_BUSYT_POS);
}

/**
 * @}
 */ /* end of group DAC_LL_EF_Configuration_ADPOT */

/** @defgroup DAC_LL_EF_CAL_Configuration Calibration Configuration functions
 * @{
 */

/**
 * @brief  Set the output trim value
 * @note   Two's complement signed value when output buffer is enabled
 * @rmtoll OTRIM         OTRIM            LL_DAC_SetOutputTrimValue
 * @param  DACx DAC Instance
 * @param  TrimValue Between 0 to 0xFFFF (two's complement -128 to 127)
 * @retval None
 */
__STATIC_INLINE void LL_DAC_SetOutputTrimValue(DAC_TypeDef *DACx, uint32_t TrimValue)
{
    MODIFY_REG(DACx->OTRIM, DAC_OTRIM_OTRIM, (TrimValue << DAC_OTRIM_OTRIM_POS));
}

/**
 * @brief  Get the output trim value
 * @rmtoll OTRIM         OTRIM            LL_DAC_GetOutputTrimValue
 * @param  DACx DAC Instance
 * @retval Output trim value (two's complement)
 */
__STATIC_INLINE uint32_t LL_DAC_GetOutputTrimValue(DAC_TypeDef *DACx)
{
    return (uint32_t)(READ_BIT(DACx->OTRIM, DAC_OTRIM_OTRIM) >> DAC_OTRIM_OTRIM_POS);
}

/**
 * @}
 */ /* end of group DAC_LL_EF_CAL_Configuration */

/** @defgroup DAC_LL_EF_Trigger_Configuration Trigger Configuration functions
 * @{
 */

/**
 * @brief  Trigger software conversion
 * @note   D/A conversion software trigger
 * @rmtoll SWT         SWT            LL_DAC_TrigSWConversion
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_TrigSWConversion(DAC_TypeDef *DACx)
{
    SET_BIT(DACx->SWT, DAC_SWT_SWT);
}

/**
 * @}
 */ /* end of group DAC_LL_EF_Trigger_Configuration */

/** @defgroup DAC_LL_EF_IT_Management Interrupt Management functions
 * @{
 */

/**
 * @brief  Enable data output register update interrupt
 * @rmtoll IE         DOUT_IE            LL_DAC_EnableIT_DOUT
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_EnableIT_DOUT(DAC_TypeDef *DACx)
{
    SET_BIT(DACx->IE, DAC_IE_DOUT_IE);
}

/**
 * @brief  Disable data output register update interrupt
 * @rmtoll IE         DOUT_IE            LL_DAC_DisableIT_DOUT
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_DisableIT_DOUT(DAC_TypeDef *DACx)
{
    CLEAR_BIT(DACx->IE, DAC_IE_DOUT_IE);
}

/**
 * @brief  Indicates whether the data output register update interrupt is enabled
 * @rmtoll IE         DOUT_IE            LL_DAC_IsEnabledIT_DOUT
 * @param  DACx DAC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_DAC_IsEnabledIT_DOUT(DAC_TypeDef *DACx)
{
    return (uint32_t)((READ_BIT(DACx->IE, DAC_IE_DOUT_IE) == (DAC_IE_DOUT_IE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable DMA error interrupt
 * @rmtoll IE         DERR_IE            LL_DAC_EnableIT_DMAERR
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_EnableIT_DMAERR(DAC_TypeDef *DACx)
{
    SET_BIT(DACx->IE, DAC_IE_DERR_IE);
}

/**
 * @brief  Disable DMA error interrupt
 * @rmtoll IE         DERR_IE            LL_DAC_DisableIT_DMAERR
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_DisableIT_DMAERR(DAC_TypeDef *DACx)
{
    CLEAR_BIT(DACx->IE, DAC_IE_DERR_IE);
}

/**
 * @brief  Indicates whether the DMA error interrupt is enabled
 * @rmtoll IE         DERR_IE            LL_DAC_IsEnabledIT_DMAERR
 * @param  DACx DAC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_DAC_IsEnabledIT_DMAERR(DAC_TypeDef *DACx)
{
    return (uint32_t)((READ_BIT(DACx->IE, DAC_IE_DERR_IE) == (DAC_IE_DERR_IE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable A/D conversion wait timeout interrupt
 * @rmtoll IE         ADPOV_IE            LL_DAC_EnableIT_ADPOV
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_EnableIT_ADPOV(DAC_TypeDef *DACx)
{
    SET_BIT(DACx->IE, DAC_IE_ADPOV_IE);
}

/**
 * @brief  Disable A/D conversion wait timeout interrupt
 * @rmtoll IE         ADPOV_IE            LL_DAC_DisableIT_ADPOV
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_DisableIT_ADPOV(DAC_TypeDef *DACx)
{
    CLEAR_BIT(DACx->IE, DAC_IE_ADPOV_IE);
}

/**
 * @brief  Indicates whether the A/D conversion wait timeout interrupt is enabled
 * @rmtoll IE         ADPOV_IE            LL_DAC_IsEnabledIT_ADPOV
 * @param  DACx DAC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_DAC_IsEnabledIT_ADPOV(DAC_TypeDef *DACx)
{
    return (uint32_t)((READ_BIT(DACx->IE, DAC_IE_ADPOV_IE) == (DAC_IE_ADPOV_IE)) ? 1UL : 0UL);
}

/**
 * @}
 */ /* end of group DAC_LL_EF_IT_Management */

/** @defgroup DAC_LL_EF_FLAG_Management Flag Management functions
 * @{
 */

/**
 * @brief  Indicate whether data output register update interrupt flag is active
 * @rmtoll IF          DOUT_IE           LL_DAC_IsActiveFlag_DOUT
 * @param  DACx DAC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_DAC_IsActiveFlag_DOUT(DAC_TypeDef *DACx)
{
    return (uint32_t)((READ_BIT(DACx->IF, DAC_IF_DOUT_IF) == (DAC_IF_DOUT_IF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the data output register update interrupt flag
 * @rmtoll IF          DOUT_IE           LL_DAC_ClearFlag_DOUT
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_ClearFlag_DOUT(DAC_TypeDef *DACx)
{
    SET_BIT(DACx->IF, DAC_IF_DOUT_IF);
}

/**
 * @brief  Indicate whether DMA error interrupt flag is active
 * @rmtoll IF          DERR_IE           LL_DAC_IsActiveFlag_DMAERR
 * @param  DACx DAC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_DAC_IsActiveFlag_DMAERR(DAC_TypeDef *DACx)
{
    return (uint32_t)((READ_BIT(DACx->IF, DAC_IF_DERR_IF) == (DAC_IF_DERR_IF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the DMA error interrupt flag
 * @rmtoll IF          DERR_IE           LL_DAC_ClearFlag_DMAERR
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_ClearFlag_DMAERR(DAC_TypeDef *DACx)
{
    SET_BIT(DACx->IF, DAC_IF_DERR_IF);
}

/**
 * @brief  Indicate whether A/D conversion wait timeout interrupt flag is active
 * @rmtoll IF          ADPOV_IE           LL_DAC_IsActiveFlag_ADPOV
 * @param  DACx DAC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_DAC_IsActiveFlag_ADPOV(DAC_TypeDef *DACx)
{
    return (uint32_t)((READ_BIT(DACx->IF, DAC_IF_ADPOV_IF) == (DAC_IF_ADPOV_IF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the A/D conversion wait timeout interrupt flag
 * @rmtoll IF          ADPOV_IE           LL_DAC_ClearFlag_ADPOV
 * @param  DACx DAC Instance
 * @retval None
 */
__STATIC_INLINE void LL_DAC_ClearFlag_ADPOV(DAC_TypeDef *DACx)
{
    SET_BIT(DACx->IF, DAC_IF_ADPOV_IF);
}

/**
 * @}
 */ /* end of group DAC_LL_EF_FLAG_Management */

/** @defgroup DAC_LL_EF_Data_Management Data Management functions
 * @{
 */

/**
 * @brief  Set the data to be loaded in the data holding register
 * @note   When DAC_CFG.DPSEL=0 (data right-aligned), write to DHOLD[11:0]
 *         When DAC_CFG.DPSEL=1 (data left-aligned), write to DHOLD[15:4]
 * @rmtoll DHOLD         DHOLD            LL_DAC_ConvertDualData
 * @param  DACx DAC Instance
 * @param  ConvertDualData Between 0 to 0xFFFF
 * @retval None
 */
__STATIC_INLINE void LL_DAC_ConvertDualData(DAC_TypeDef *DACx, uint32_t ConvertDualData)
{
    MODIFY_REG(DACx->DHOLD, DAC_DHOLD_DHOLD, (ConvertDualData << DAC_DHOLD_DHOLD_POS));
}

/**
 * @brief  Retrieve output data currently generated for the selected DAC channel
 * @rmtoll DOUT         DOUT            LL_DAC_RetrieveOutputData
 * @param  DACx DAC Instance
 * @retval DAC output data value (12-bit)
 */
__STATIC_INLINE uint32_t LL_DAC_RetrieveOutputData(DAC_TypeDef *DACx)
{
    return (uint32_t)(READ_BIT(DACx->DOUT, DAC_DOUT_DOUT) >> DAC_DOUT_DOUT_POS);
}

/**
 * @}
 */ /* end of group DAC_LL_EF_Data_Management */
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup DAC_LL_EF_Init Initialization and de-initialization functions
 * @{
 */
ErrorStatus LL_DAC_DeInit(DAC_TypeDef *DACx);
ErrorStatus LL_DAC_Init(DAC_TypeDef *DACx, LL_DAC_InitTypeDef *DAC_InitStruct);
void        LL_DAC_StructInit(LL_DAC_InitTypeDef *DAC_InitStruct);
/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */ /* end of group DAC_LL_Exported_Functions */

/**
 * @}
 */ /* end of group DAC_LL */

#endif /* DAC_CORE0 || DAC_CORE1 */

/**
 * @}
 */ /* end of group RN_LL_Driver */

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_LL_DAC_H */
