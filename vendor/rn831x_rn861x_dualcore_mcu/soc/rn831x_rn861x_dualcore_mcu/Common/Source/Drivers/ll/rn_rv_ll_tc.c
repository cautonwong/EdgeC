/**
 ******************************************************************************
 * @file    rn_rv_ll_tc.c
 * @author  Renergy Software Team
 * @brief   TC LL module driver.
 *          This file provides firmware functions to manage the following
 *          functionalities of the Timer Counter (TC) peripheral:
 *           + Initialization and de-initialization functions
 *           + Peripheral Control functions
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_ll_tc.h"
#include "rn_rv_ll_system_core0.h"
#include "rn_rv_ll_system_core1.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined(TC0_CORE0) || defined(TC1_CORE0) || defined(TC0_CORE1) || defined(TC1_CORE1)

/** @addtogroup TC_LL
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup TC_LL_Private_Macros
 * @{
 */

/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/

/* Exported functions -------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @addtogroup TC_LL_Exported_Functions
 * @{
 */

/** @addtogroup TC_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize TC registers (Registers restored to their default values).
 * @param  TCx TC Instance
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: TC registers are de-initialized
 *          - ERROR:   TC Instance is not valid
 */
ErrorStatus LL_TC_DeInit(TC_TypeDef *TCx)
{
   ErrorStatus result = SUCCESS;

   /* Check the parameters */
   assert_param(IS_LL_TC_ALL_INSTANCE(TCx));

   if (TCx == TC0_CORE0)
   {
       LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_TC0);
       LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_TC0);
   }
   else if (TCx == TC1_CORE0)
   {
       LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_TC1);
       LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_TC1);
   }
   else if (TCx == TC0_CORE1)
   {
       LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_TC0);
       LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_TC0);
   }
   else if (TCx == TC1_CORE1)
   {
       LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_TC1);
       LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_TC1);
   }
   else
   {
       result = ERROR;
   }

   return result;
}

/**
 * @brief  Set each @ref LL_TC_InitTypeDef field to default value.
 * @param  TC_InitStruct pointer to a @ref LL_TC_InitTypeDef structure
 *         whose fields will be set to default values.
 * @retval None
 */
void LL_TC_StructInit(LL_TC_InitTypeDef *TC_InitStruct)
{
    /* Set TC_InitStruct fields to default values */
    TC_InitStruct->Prescaler        = (uint16_t)0x0000;
    TC_InitStruct->Autoreload       = 0x0U;
    TC_InitStruct->CH0OCIdleState       = LL_TC_OCIDLESTATE_LOW;
    TC_InitStruct->CH1OCIdleState       = LL_TC_OCIDLESTATE_LOW;
}

/**
 * @brief  Configure the TC time base unit.
 * @param  TCx TC Instance
 * @param  TC_InitStruct pointer to a @ref LL_TC_InitTypeDef structure (TC time base unit configuration data structure)
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: TC registers are initialized
 *          - ERROR:   Not applicable
 */
ErrorStatus LL_TC_Init(TC_TypeDef *TCx, LL_TC_InitTypeDef *TC_InitStruct)
{
    ErrorStatus result = SUCCESS;

    /* Check the TC Instance TCx */
    assert_param(IS_LL_TC_ALL_INSTANCE(TCx));

    /* Check the TC parameters from TC_InitStruct */
    assert_param(IS_LL_TC_PRESCALER(TC_InitStruct->Prescaler));
    assert_param(IS_LL_TC_AUTORELOAD(TC_InitStruct->Autoreload));
    assert_param(IS_LL_TC_OCIDLESTATE(TC_InitStruct->CH0OCIdleState));
    assert_param(IS_LL_TC_OCIDLESTATE(TC_InitStruct->CH0OCIdleState));

    /*---------------------------- TCx Configuration ------------------------
    * Configure TCx with parameters:
    * - Prescaler: TC_PS register
    * - Autoreload: TC_CNT register
    * - DebugMode: TC_CTRL_DBGSTBDIS field
    * - CH0OCIdleState: TC_CM0_DFTLVL field
    * - CH1OCIdleState: TC_CM1_DFTLVL field
    */

    /* Set the prescaler value */
    LL_TC_SetPrescaler(TCx, TC_InitStruct->Prescaler);

    /* Set the auto-reload value */
    LL_TC_SetAutoReload(TCx, TC_InitStruct->Autoreload);

    /* Set the counter mode at debug state */
    LL_TC_SetDebugMode(TCx, TC_InitStruct->DebugMode);

    /* Set (output level when channel 1 compare/capture function disable */
    LL_TC_OC_SetIdleState(TCx, LL_TC_CHANNEL_CH1, TC_InitStruct->CH1OCIdleState);

    return result;
}

/**
 * @brief  Set each @ref LL_TC_OC_InitTypeDef field to default value.
 * @param  TC_OC_InitStruct pointer to a @ref LL_TC_OC_InitTypeDef structure
 *         whose fields will be set to default values.
 * @retval None
 */
void LL_TC_OC_StructInit(LL_TC_OC_InitTypeDef *TC_OC_InitStruct)
{
    /* Set TC_OC_InitStruct fields to default values */
    TC_OC_InitStruct->OCMode        = LL_TC_OCMODE_TRISTATE;
    TC_OC_InitStruct->OCState       = LL_TC_OCSTATE_DISABLE;
    TC_OC_InitStruct->CompareValue  = 0x00000000U;
    TC_OC_InitStruct->OCPolarity    = LL_TC_OCPOLARITY_HIGH;
    TC_OC_InitStruct->DeadTime      = 0x00U;
}

/**
 * @brief  Configure the TC output channel.
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @param  TC_OC_InitStruct pointer to a @ref LL_TC_OC_InitTypeDef structure (TC output channel configuration data structure)
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: TC output channel is initialized
 *          - ERROR:   Incorrect input parameters
 */
ErrorStatus LL_TC_OC_Init(TC_TypeDef *TCx, uint32_t Channel, LL_TC_OC_InitTypeDef *TC_OC_InitStruct)
{
    ErrorStatus result = SUCCESS;

    /* Check the TC Instance TCx and Channel */
    assert_param(IS_LL_TC_ALL_INSTANCE(TCx));
    assert_param(IS_LL_TC_CHANNEL(Channel));

    /* Check the TC OC parameters from TC_OC_InitStruct */
    assert_param(IS_LL_TC_OCMODE(TC_OC_InitStruct->OCMode));
    assert_param(IS_LL_TC_OCSTATE(TC_OC_InitStruct->OCState));
    assert_param(IS_LL_TC_COMPAREVALUE(TC_OC_InitStruct->CompareValue));
    assert_param(IS_LL_TC_POLARITY(TC_OC_InitStruct->OCPolarity));
    assert_param(IS_LL_TC_DEADTIME(TC_OC_InitStruct->DeadTime));

    /*---------------------------- TCx CCMRx Configuration ------------------------
    * Configure TCxCH output channel with parameters:
    * - DurationMode: TC_CMx_CCM and TC_CMx_OM bits
    * - OutputPolarity: TC_CMx_EFELVL bit
    * - OutputIdleState: TC_CMx_DFTLVL bit
    * - DeadTime: TC_CMx_DL bits
    */

    if (IS_LL_TC_CHANNEL(Channel) && IS_LL_TC_ALL_INSTANCE(TCx))
    {
        /* Configure the output channel */
        LL_TC_OC_ConfigOutput(TCx, Channel,
                        TC_OC_InitStruct->OCMode |
                        TC_OC_InitStruct->OCPolarity);
        /* Set compare value */
        if (Channel == LL_TC_CHANNEL_CH0)
        {
            LL_TC_OC_SetCompareCH0(TCx, TC_OC_InitStruct->CompareValue);
        }
        else if (Channel == LL_TC_CHANNEL_CH1)
        {
            LL_TC_OC_SetCompareCH1(TCx, TC_OC_InitStruct->CompareValue);
        }

        /* Configure dead time */
        LL_TC_OC_SetDeadTime(TCx, Channel, TC_OC_InitStruct->DeadTime);

        /* Enable or disable the channel */
        if (TC_OC_InitStruct->OCState == LL_TC_OCSTATE_ENABLE)
        {
            LL_TC_CC_EnableChannel(TCx, Channel);
        }
        else
        {
            LL_TC_CC_DisableChannel(TCx, Channel);
        }

    } else {
        result = ERROR;
    }

    return result;
}

/**
 * @brief  Set each @ref LL_TC_IC_InitTypeDef field to default value.
 * @param  TC_IC_InitStruct pointer to a @ref LL_TC_IC_InitTypeDef structure
 *         whose fields will be set to default values.
 * @retval None
 */
void LL_TC_IC_StructInit(LL_TC_IC_InitTypeDef *TC_IC_InitStruct)
{
    /* Set TC_IC_InitStruct fields to default values */
    TC_IC_InitStruct->ICPolarity  = LL_TC_IC_POLARITY_RISING;
    TC_IC_InitStruct->ICFilter    = 0x0U;
}

/**
 * @brief  Configure the TC input channel.
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @param  TC_IC_InitStruct pointer to a @ref LL_TC_IC_InitTypeDef structure (TC input channel configuration data structure)
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: TC input channel is initialized
 *          - ERROR:   Incorrect input parameters
 */
ErrorStatus LL_TC_IC_Init(TC_TypeDef *TCx, uint32_t Channel, LL_TC_IC_InitTypeDef *TC_IC_InitStruct)
{
    ErrorStatus result = SUCCESS;

    /* Check the TC Instance TCx and Channel */
    assert_param(IS_LL_TC_ALL_INSTANCE(TCx));
    assert_param(IS_LL_TC_CHANNEL(Channel));

    /* Check the TC IC parameters from TC_IC_InitStruct */
    assert_param(IS_LL_TC_IC_POLARITY(TC_IC_InitStruct->ICPolarity));
    assert_param(IS_LL_TC_IC_FILTER(TC_IC_InitStruct->ICFilter));

    /*---------------------------- TCx CCMRx Configuration ------------------------
    * Configure TCxCH input channel with parameters:
    * - InputPolarity: TC_CMx_CPOL bit
    * - InputFilter: TC_CMx_FLTEN bit
    */

    /* Configure the input channel */
    LL_TC_IC_Config(TCx, Channel,
                    TC_IC_InitStruct->ICPolarity ,
                    TC_IC_InitStruct->ICFilter);

    return result;
}

/**
 * @brief  Set each @ref LL_TC_PWMMOD_InitTypeDef field to default value.
 * @param  TC_PWMMOD_InitStruct pointer to a @ref LL_TC_PWMMOD_InitTypeDef structure
 *         whose fields will be set to default values.
 * @retval None
 */
void LL_TC_PWMMOD_StructInit(LL_TC_PWMMOD_InitTypeDef *TC_PWMMOD_InitStruct)
{
    /* Set TC_PWMMOD_InitStruct fields to default values */
    TC_PWMMOD_InitStruct->PWMMODMode        = LL_TC_PWMMOD_CONTROLMODE_SOFTWARE;
    TC_PWMMOD_InitStruct->StopMode          = LL_TC_PWMMOD_STOPMODE_COMPLETE;
    TC_PWMMOD_InitStruct->DurationMode      = LL_TC_PWMMOD_DURATIONMODE_CONTINUOUS;
    TC_PWMMOD_InitStruct->WaveLength        = 0x00000000U;
    TC_PWMMOD_InitStruct->PulseWidth       = 0x00000000U;
    TC_PWMMOD_InitStruct->PulsePeriod      = 0x0000FFFFU;
}

/**
 * @brief  Configure the TC PWM modulation.
 * @param  TCx TC Instance
 * @param  TC_PWMMOD_InitStruct pointer to a @ref LL_TC_PWMMOD_InitTypeDef structure (TC PWM modulation configuration data structure)
 * @note   Channel 1 must be configured to PWM1 or PWM2 mode before enabling PWM modulation.
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: TC PWM modulation is initialized
 *          - ERROR:   Incorrect input parameters
 */
ErrorStatus LL_TC_PWMMOD_Init(TC_TypeDef *TCx, LL_TC_PWMMOD_InitTypeDef *TC_PWMMOD_InitStruct)
{
    ErrorStatus result = SUCCESS;

    /* Check the TC Instance TCx */
    assert_param(IS_LL_TC_ALL_INSTANCE(TCx));

    /* Check the TC PWM modulation parameters from TC_PWMMOD_InitStruct */
    assert_param(IS_LL_TC_PWMMOD_CONTROLMODE(TC_PWMMOD_InitStruct->PWMMODMode));
    assert_param(IS_LL_TC_PWMMOD_DURATIONMODE(TC_PWMMOD_InitStruct->DurationMode));
    assert_param(IS_LL_TC_PWMMOD_STOPMODE(TC_PWMMOD_InitStruct->StopMode));
    assert_param(IS_LL_TC_PWMMOD_WAVELENGTH(TC_PWMMOD_InitStruct->WaveLength));
    assert_param(IS_LL_TC_PWMMOD_PULSEWIDTH(TC_PWMMOD_InitStruct->PulseWidth));
    assert_param(IS_LL_TC_PWMMOD_PULSEPERIOD(TC_PWMMOD_InitStruct->PulsePeriod));

    /*---------------------------- TCx PWM Configuration ------------------------
    * Configure TCx PWM modulation with parameters:
    * - ControlMode: TC_PWM_CFG_CMS bit
    * - DurationMode: TC_PWM_CFG_ORV bit
    * - StopMode: TC_PWM_CFG_CSS bit
    * - WaveLength: TC_PWM_CFG_WL bits
    * - PulsePeriod: TC_PWM_CNT_N_PWM bits
    * - PulseWidth: TC_CCD1 register
    */

    /* Set control mode */
    LL_TC_PWMMOD_SetControlMode(TCx, TC_PWMMOD_InitStruct->PWMMODMode);

    /* Set output duration mode*/
    LL_TC_PWMMOD_SetOutputDurationMode(TCx, TC_PWMMOD_InitStruct->DurationMode);

    /* Set StopMode */
    LL_TC_PWMMOD_SetStopMode(TCx, TC_PWMMOD_InitStruct->StopMode);

    /* Set wave length (only valid in length mode) */
    if (TC_PWMMOD_InitStruct->DurationMode == LL_TC_PWMMOD_DURATIONMODE_LENGTH)
    {
        LL_TC_PWMMOD_SetWaveLength(TCx, TC_PWMMOD_InitStruct->WaveLength);
    }

    /* Set pulse period length */
    LL_TC_PWMMOD_SetPeriod(TCx, TC_PWMMOD_InitStruct->PulsePeriod);

    /* Set pulse length (using Channel 1 compare register) */
    LL_TC_OC_SetCompareCH1(TCx, TC_PWMMOD_InitStruct->PulseWidth);

    return result;
}



/**
 * @}
 * TC_LL_EF_Init
 */

/**
 * @}
 * TC_LL_Exported_Functions
 */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 * TC_LL
 */

#endif /* TC0_CORE0 || TC1_CORE0 || TC0_CORE1 || TC1_CORE1 */

/**
 * @}
 * RN_LL_Driver
 */
