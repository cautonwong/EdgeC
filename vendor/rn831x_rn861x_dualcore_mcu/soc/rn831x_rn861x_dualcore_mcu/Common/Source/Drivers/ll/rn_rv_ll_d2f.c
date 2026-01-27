/**
 *****************************************************************************
 * @file rn_rv_ll_d2f.c
 * @author Renergy Software Team
 * @brief
 *
 *****************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 *****************************************************************************
 */
#if defined(USE_FULL_LL_DRIVER)

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_ll_d2f.h"
#include "rn_rv_ll_system_core0.h"
#include "rn_rv_ll_system_core1.h"

#ifdef  USE_FULL_ASSERT
#include "rn_rv_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined(D2F)

/** @defgroup D2F_LL D2F
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup D2F_LL_Private_Macros
 * @{
 */
#define IS_LL_D2F_CLOCK_MODE(__VALUE__)        (((__VALUE__) == LL_D2F_CLKMODE_DEFAULT)   ||\
                                                ((__VALUE__) == LL_D2F_CLKMODE_CONSTANT))

#define IS_LL_D2F_LOAD_TYPE(__VALUE__)         (((__VALUE__) == LL_D2F_LOADTYPE_NORMAL)   ||\
                                                ((__VALUE__) == LL_D2F_LOADTYPE_SIMPTC))

#define IS_LL_D2F_ENERGY_TYPE(__VALUE__)       (((__VALUE__) == LL_D2F_ENGTYPE_R2CLR)     ||\
                                                ((__VALUE__) == LL_D2F_ENGTYPE_COUNTER))

#define IS_LL_D2F_CF0_CHANNEL(__VALUE__)       (((__VALUE__) == LL_D2F_CF0_D2F00)     ||\
                                                ((__VALUE__) == LL_D2F_CF0_D2F01)     ||\
                                                ((__VALUE__) == LL_D2F_CF0_D2F02)     ||\
                                                ((__VALUE__) == LL_D2F_CF0_D2F03))

#define IS_LL_D2F_CF1_CHANNEL(__VALUE__)       (((__VALUE__) == LL_D2F_CF1_D2F04)     ||\
                                                ((__VALUE__) == LL_D2F_CF1_D2F05)     ||\
                                                ((__VALUE__) == LL_D2F_CF1_D2F06)     ||\
                                                ((__VALUE__) == LL_D2F_CF1_D2F07))

#define IS_LL_D2F_CF2_CHANNEL(__VALUE__)       (((__VALUE__) == LL_D2F_CF2_D2F08)     ||\
                                                ((__VALUE__) == LL_D2F_CF2_D2F09)     ||\
                                                ((__VALUE__) == LL_D2F_CF2_D2F10)     ||\
                                                ((__VALUE__) == LL_D2F_CF2_D2F11))
/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup D2F_LL_Exported_Functions
 * @{
 */

/** @addtogroup D2F_LL_EF_Init
 * @{
 */

/**
 * @brief De-initialize D2F registers
 * @param D2Fx D2F Instance
 * @return ErrorStatus value:
 *          - SUCCESS: D2F registers are de-initialized
 *          - ERROR:   Wrong D2F Port
 */
ErrorStatus LL_D2F_DeInit(D2F_TypeDef *D2Fx)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_D2F_ALL_INSTANCE(D2Fx));

    if (D2Fx == D2F_CORE0)
    {
        /* Force and Release reset on clock of D2Fx Port */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_D2F);
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_D2F);
    }
    else if(D2Fx == D2F_CORE1)
    {
        /* Force and Release reset on clock of D2Fx Port */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_D2F);
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_D2F);
    }
    else
    {
        status = ERROR;
    }

    return status;
}

/**
 * @brief Initialize D2F registers according to the specified parameters in D2F_InitStruct
 * @note  The D2F module must be disabled before hardware configuration modeifcation.
 * @note  At most three CFs(CF0/CF1/CF2) can be support, and each CF is shared by four D2F channels.
 * @note  Each CF can only be used by one D2F channel at the same time.
 * @note  If the CF output function is required, the corresponding I/O must be configured as CF alternate function.
 * @param D2Fx D2F Instance
 * @param D2F_InitStruct pointer to LL_D2F_InitTypeDef structure
 * @return ErrorStatus:
 *          - SUCCESS: D2F registers are de-initialized
 *          - ERROR:   Not applicable
 */
ErrorStatus LL_D2F_Init(D2F_TypeDef *D2Fx, LL_D2F_InitTypeDef *D2F_InitStruct)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_D2F_ALL_INSTANCE(D2Fx));
    assert_param(IS_LL_D2F_CLOCK_MODE(D2F_InitStruct->ClockMode));
    assert_param(IS_LL_D2F_LOAD_TYPE(D2F_InitStruct->PowerLoadType));
    assert_param(IS_LL_D2F_ENERGY_TYPE(D2F_InitStruct->EnergyType));
    assert_param(IS_LL_D2F_CF0_CHANNEL(D2F_InitStruct->CF0Channel));
    assert_param(IS_LL_D2F_CF1_CHANNEL(D2F_InitStruct->CF1Channel));
    assert_param(IS_LL_D2F_CF2_CHANNEL(D2F_InitStruct->CF2Channel));

    /* Note: Hardware constraint (refer to description of this function)  */
    /*       D2F instance must be disabled.                               */
    if(LL_D2F_IsEnabled(D2Fx) == 0UL)
    {
        /*  - ClockMode configuration      */
        LL_D2F_SetIntegratorClkMode(D2Fx, D2F_InitStruct->ClockMode);
        /*  - PowerLoadType configuration  */
        LL_D2F_SetPowerLoadType(D2Fx, D2F_InitStruct->PowerLoadType);
        /*  - EnergyType configuration     */
        LL_D2F_SetEnergyType(D2Fx, D2F_InitStruct->EnergyType);
        /*  - CF0Channel configuration     */
        LL_D2F_SetCF0Channel(D2Fx, D2F_InitStruct->CF0Channel);
        /*  - CF1Channel configuration     */
        LL_D2F_SetCF1Channel(D2Fx, D2F_InitStruct->CF1Channel);
        /*  - CF2Channel configuration     */
        LL_D2F_SetCF2Channel(D2Fx, D2F_InitStruct->CF2Channel);
        /*  - Enable D2F Peripheral */
        LL_D2F_Enable(D2Fx);
    } else {
        /* Initialization error: D2F instance is not disabled.  */
        status = ERROR;
    }

    return status;
}

/**
 * @brief Set each @ref LL_D2F_InitTypeDef field to default value.
 * @param D2F_InitStruct pointer to LL_D2F_InitTypeDef structure
 * @retval None
 */
void LL_D2F_StructInit(LL_D2F_InitTypeDef *D2F_InitStruct)
{
    /* Reset D2F init structure parameters values */
    D2F_InitStruct->ClockMode     = LL_D2F_CLKMODE_DEFAULT;
    D2F_InitStruct->PowerLoadType = LL_D2F_LOADTYPE_NORMAL;
    D2F_InitStruct->EnergyType    = LL_D2F_ENGTYPE_R2CLR;
    D2F_InitStruct->CF0Channel    = LL_D2F_CF0_D2F00;
    D2F_InitStruct->CF1Channel    = LL_D2F_CF1_D2F04;
    D2F_InitStruct->CF2Channel    = LL_D2F_CF2_D2F08;
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

#endif /* D2F */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
