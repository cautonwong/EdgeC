/**
 *****************************************************************************
 * @file rn_rv_ll_dsp.c
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
#include "rn_rv_ll_dsp.h"
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

#if defined(DSP)

/** @defgroup DSP_LL DSP
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup DSP_LL_Private_Macros
 * @{
 */
#define IS_LL_DSP_MODE(__VALUE__)         (((__VALUE__) == LL_DSP_MODE_DISABLE   ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_I2F_ONCE  ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_I2F_DMA   ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_F2I_ONCE  ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_F2I_DMA   ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_FMUL      ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_FADD      ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_FSUB      ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_FMULADD   ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_BTFY_ONCE ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_BTFY_DMA  ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_FFT       ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_BITREVERSE) ||\
                                           ((__VALUE__) == LL_DSP_MODE_IIR_ONCE  ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_IIR_DMA   ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_FIR_DMA   ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_LINEAR    ) ||\
                                           ((__VALUE__) == LL_DSP_MODE_LAGRANGE  ))

#define IS_LL_DSP_VALID_BYTE(__VALUE__)    (((__VALUE__) == LL_DSP_VALID_4BYTE  ) ||\
                                            ((__VALUE__) == LL_DSP_VALID_3BYTE  ) ||\
                                            ((__VALUE__) == LL_DSP_VALID_2BYTE  ) ||\
                                            ((__VALUE__) == LL_DSP_VALID_1BYTE  ))

#define IS_LL_DSP_ROUND(__VALUE__)         (((__VALUE__) == LL_DSP_ROUNDMODE_NEAREST  ) ||\
                                            ((__VALUE__) == LL_DSP_ROUNDMODE_ZERO     ) ||\
                                            ((__VALUE__) == LL_DSP_ROUNDMODE_POS_INF  ) ||\
                                            ((__VALUE__) == LL_DSP_ROUNDMODE_NEG_INF  ) ||\
                                            ((__VALUE__) == LL_DSP_ROUNDMODE_UP       ) ||\
                                            ((__VALUE__) == LL_DSP_ROUNDMODE_AWAY     ))
/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup DSP_LL_Exported_Functions
 * @{
 */

/** @addtogroup DSP_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize DSP registers
 * @param  DSPx DSP port
 * @return ErrorStatus value:
 *          - SUCCESS: DSP registers are de-initialized
 *          - ERROR:   Wrong DSP Port
 */
ErrorStatus LL_DSP_DeInit(DSP_TypeDef *DSPx)
{
    ErrorStatus status = SUCCESS;
    /* Check the parameters */
    assert_param(IS_DSP_ALL_INSTANCE(DSPx));

    if(DSPx == DSP_CORE0)
    {
        /* Force and Release reset on clock of DSPx Port */
        /* TODO use api function */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_DSP);
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_DSP);
    }
    else if(DSPx == DSP_CORE1)
    {
        /* Force and Release reset on clock of DSPx Port */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_DSP);
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_DSP);
    }
    else
    {
        status = ERROR;
    }
    return (status);
}

/**
 * @brief Initialize DSP registers according to the specified parameters in DSP_InitStruct
 * @param DSPx DSP Port
 * @param DSP_InitStruct pointer to LL_DSP_InitTypeDef structure
 * @return ErrorStatus value:
 *          - SUCCESS: DSP registers are initialized
 *          - ERROR:   Not applicable
 */
ErrorStatus LL_DSP_Init(DSP_TypeDef *DSPx, LL_DSP_InitTypeDef *DSP_InitStruct)
{
    /* Check the parameters */
    assert_param(IS_DSP_ALL_INSTANCE(DSPx));
    assert_param(IS_LL_DSP_MODE(DSP_InitStruct->DSPMode));
    assert_param(IS_LL_DSP_VALID_BYTE(DSP_InitStruct->I2FValidByte));
    assert_param(IS_LL_DSP_ROUND(DSP_InitStruct->RoundMode));

    /* DSP mode configuration */
    LL_DSP_SetDSPMode(DSPx, DSP_InitStruct->DSPMode);

    /* I2F valid byte configuration */
    LL_DSP_SetI2FPreprocess(DSPx, DSP_InitStruct->I2FValidByte);

    /* Round mode configuration*/
    LL_DSP_SetRoundMode(DSPx, DSP_InitStruct->RoundMode);

    return (SUCCESS);
}

/**
 * @brief Set each @ref LL_DSP_InitTypeDef field to default value.
 * @param DSP_InitStruct pointer to LL_DSP_InitTypeDef structure
 * @retval None
 */
void LL_DSP_StructInit(LL_DSP_InitTypeDef *DSP_InitStruct)
{
    /* Reset DSP init structure parameters values */
    DSP_InitStruct->DSPMode        = LL_DSP_MODE_DISABLE;
    DSP_InitStruct->I2FValidByte   = LL_DSP_VALID_4BYTE;
    DSP_InitStruct->RoundMode      = LL_DSP_ROUNDMODE_NEAREST;
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

#endif /* DSP */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
