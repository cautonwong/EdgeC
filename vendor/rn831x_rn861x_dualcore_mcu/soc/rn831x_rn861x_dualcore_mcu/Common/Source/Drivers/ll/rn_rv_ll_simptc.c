/**
 *****************************************************************************
 * @file rn_rv_ll_simptc.c
 * @author Renergy Software Team
 * @brief SIMPTC LL module driver.
 *
 *****************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 *****************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_ll_simptc.h"
#include "rn_rv_ll_system_core0.h"
#include "rn_rv_ll_system_core1.h"

#ifdef  USE_FULL_ASSERT
#include "rn_rv_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif/* USE_FULL_ASSERT */

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

/** @addtogroup SIMPTC_LL
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup SIMPTC_LL_Private_Macros
 * @{
 */

#define IS_LL_SIMPTC_ONEPULSEMODE(__VALUE__)    (((__VALUE__) == LL_SIMPTC_ONEPULSEMODE_SINGLE) \
                                                || ((__VALUE__) == LL_SIMPTC_ONEPULSEMODE_CONTINUOUS))

#define IS_LL_SIMPTC_ENABLESOURCE(__VALUE__)    (((__VALUE__) == LL_SIMPTC_ENABLESOURCE_SOFTWARE) \
                                                || ((__VALUE__) == LL_SIMPTC_ENABLESOURCE_AOS))

#define IS_LL_SIMPTC_DEBUGMODE(__VALUE__)       (((__VALUE__) == LL_SIMPTC_DEBUGMODE_PAUSE) \
                                                || ((__VALUE__) == LL_SIMPTC_DEBUGMODE_CONTINUE))

/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @addtogroup SIMPTC_LL_Exported_Functions
 * @{
 */

/** @addtogroup SIMPTC_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize SIMPTC registers (Registers restored to their default values).
 * @param  SIMPTCx_x SIMPTC Instance
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: SIMPTC registers are de-initialized
 *          - ERROR: invalid SIMPTC instance
 */
ErrorStatus LL_SIMPTC_DeInit(SIMPTC_TypeDef *SIMPTCx_x)
{
    ErrorStatus result = SUCCESS;

    /* Check the parameters */
    assert_param(IS_SIMPTC_ALL_INSTANCE(SIMPTCx_x));


    if (SIMPTCx_x == SIMPTC0_0_CORE0 || SIMPTCx_x == SIMPTC0_1_CORE0 || SIMPTCx_x == SIMPTC0_2_CORE0 || SIMPTCx_x == SIMPTC0_3_CORE0 )
    {
        LL_SYSTEM_CORE0_MOD1_ForceReset(  LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC0);
        LL_SYSTEM_CORE0_MOD1_ReleaseReset(LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC0);
    }
    else if (SIMPTCx_x == SIMPTC1_0_CORE0 || SIMPTCx_x == SIMPTC1_1_CORE0 || SIMPTCx_x == SIMPTC1_2_CORE0 || SIMPTCx_x == SIMPTC1_3_CORE0 )
    {
        LL_SYSTEM_CORE0_MOD1_ForceReset(  LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC1);
        LL_SYSTEM_CORE0_MOD1_ReleaseReset(LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC1);
    }
    else if (SIMPTCx_x == SIMPTC2_0_CORE0 || SIMPTCx_x == SIMPTC2_1_CORE0 || SIMPTCx_x == SIMPTC2_2_CORE0 || SIMPTCx_x == SIMPTC2_3_CORE0 )
    {
        LL_SYSTEM_CORE0_MOD1_ForceReset(  LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC2);
        LL_SYSTEM_CORE0_MOD1_ReleaseReset(LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC2);
    }
    else if (SIMPTCx_x == SIMPTC3_0_CORE0 || SIMPTCx_x == SIMPTC3_1_CORE0 || SIMPTCx_x == SIMPTC3_2_CORE0 || SIMPTCx_x == SIMPTC3_3_CORE0 )
    {
        LL_SYSTEM_CORE0_MOD1_ForceReset(  LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC3);
        LL_SYSTEM_CORE0_MOD1_ReleaseReset(LL_SYSTEM_CORE0_MOD1_PERIPH_SIMPTC3);
    }
    else if (SIMPTCx_x == SIMPTC0_0_CORE1 || SIMPTCx_x == SIMPTC0_1_CORE1 || SIMPTCx_x == SIMPTC0_2_CORE1 || SIMPTCx_x == SIMPTC0_3_CORE1 )
    {
        LL_SYSTEM_CORE1_MOD1_ForceReset(  LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC0);
        LL_SYSTEM_CORE1_MOD1_ReleaseReset(LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC0);
    }
    else if (SIMPTCx_x == SIMPTC1_0_CORE1 || SIMPTCx_x == SIMPTC1_1_CORE1 || SIMPTCx_x == SIMPTC1_2_CORE1 || SIMPTCx_x == SIMPTC1_3_CORE1 )
    {
        LL_SYSTEM_CORE1_MOD1_ForceReset(  LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC1);
        LL_SYSTEM_CORE1_MOD1_ReleaseReset(LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC1);
    }
    else if (SIMPTCx_x == SIMPTC2_0_CORE1 || SIMPTCx_x == SIMPTC2_1_CORE1 || SIMPTCx_x == SIMPTC2_2_CORE1 || SIMPTCx_x == SIMPTC2_3_CORE1 )
    {
        LL_SYSTEM_CORE1_MOD1_ForceReset(  LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC2);
        LL_SYSTEM_CORE1_MOD1_ReleaseReset(LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC2);
    }
    else if (SIMPTCx_x == SIMPTC3_0_CORE1 || SIMPTCx_x == SIMPTC3_1_CORE1 || SIMPTCx_x == SIMPTC3_2_CORE1 || SIMPTCx_x == SIMPTC3_3_CORE1 )
    {
        LL_SYSTEM_CORE1_MOD1_ForceReset(  LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC3);
        LL_SYSTEM_CORE1_MOD1_ReleaseReset(LL_SYSTEM_CORE1_MOD1_PERIPH_SIMPTC3);
    }
    else {
        result = ERROR;
    }

    return result;
}

/**
 * @brief  Set each @ref LL_SIMPTC_InitTypeDef field to default value.
 * @param  SIMPTC_InitStruct pointer to a @ref LL_SIMPTC_InitTypeDef structure
 *         whose fields will be set to default values.
 * @retval None
 */
void LL_SIMPTC_StructInit(LL_SIMPTC_InitTypeDef *SIMPTC_InitStruct)
{
    /* Set default configuration */
    SIMPTC_InitStruct->OnePulseMode    = LL_SIMPTC_ONEPULSEMODE_SINGLE;
    SIMPTC_InitStruct->AutoReload      = 0x00000000U;
    SIMPTC_InitStruct->EnableSource    = LL_SIMPTC_ENABLESOURCE_SOFTWARE;
    SIMPTC_InitStruct->DebugMode       = LL_SIMPTC_DEBUGMODE_PAUSE;
}

/**
 * @brief  Configure the SIMPTC time base unit.
 * @param  SIMPTCx_x SIMPTC Instance
 * @param  SIMPTC_InitStruct pointer to a @ref LL_SIMPTC_InitTypeDef structure
 *         (SIMPTC time base unit configuration data structure)
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: SIMPTC registers are initialized
 *          - ERROR: not applicable
 */
ErrorStatus LL_SIMPTC_Init(SIMPTC_TypeDef *SIMPTCx_x, LL_SIMPTC_InitTypeDef *SIMPTC_InitStruct)
{
    ErrorStatus result = SUCCESS;

    /* Check the parameters */
    assert_param(IS_SIMPTC_ALL_INSTANCE(SIMPTCx_x));
    assert_param(IS_LL_SIMPTC_ONEPULSEMODE(SIMPTC_InitStruct->OnePulseMode));
    assert_param(IS_LL_SIMPTC_ENABLESOURCE(SIMPTC_InitStruct->EnableSource));
    assert_param(IS_LL_SIMPTC_DEBUGMODE(SIMPTC_InitStruct->DebugMode));

    if (IS_SIMPTC_ALL_INSTANCE(SIMPTCx_x))
    {
        /* Set the Target count value */
        LL_SIMPTC_SetAutoReload(SIMPTCx_x, SIMPTC_InitStruct->AutoReload);

        /* Set the counter mode */
        LL_SIMPTC_SetOnePulseMode(SIMPTCx_x, SIMPTC_InitStruct->OnePulseMode);

        /* Set the counter enable source */
        LL_SIMPTC_SetEnableSource(SIMPTCx_x, SIMPTC_InitStruct->EnableSource);

        /* Set the debug mode */
        LL_SIMPTC_SetDebugMode(SIMPTCx_x, SIMPTC_InitStruct->DebugMode);

        /* Clear overflow flag */
        LL_SIMPTC_ClearFlag_OV(SIMPTCx_x);
    }
    else
    {
        result = ERROR;
    }

    return result;
}
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#endif /* SIMPTC0_0_CORE0 || SIMPTC0_1_CORE0 || ... */

/**
 * @}
 */
