/**
 *****************************************************************************
 * @file rn_rv_ll_lsrx.c
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
#include "rn_rv_ll_lsrx.h"
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

#if defined(LSRX)

/** @defgroup LSRX_LL LSRX
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup LSRX_LL_Private_Macros
 * @{
 */
#define IS_LL_LSRX_CHSEL(__VALUE__)             (((__VALUE__) == LL_LSRX_CHSEL_UART0)   ||\
                                                 ((__VALUE__) == LL_LSRX_CHSEL_UART1)   ||\
                                                 ((__VALUE__) == LL_LSRX_CHSEL_UART2)   ||\
                                                 ((__VALUE__) == LL_LSRX_CHSEL_UART3)   ||\
                                                 ((__VALUE__) == LL_LSRX_CHSEL_UART4)   ||\
                                                 ((__VALUE__) == LL_LSRX_CHSEL_UART5)   ||\
                                                 ((__VALUE__) == LL_LSRX_CHSEL_UART6)   ||\
                                                 ((__VALUE__) == LL_LSRX_CHSEL_UART7))

#define IS_LL_LSRX_RXEN(__VALUE__)             (((__VALUE__) == LL_LSRX_RXEN_DISBALE)   ||\
                                                 ((__VALUE__) == LL_LSRX_RXEN_ENABLE ))
/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup LSRX_LL_Exported_Functions
 * @{
 */

/** @addtogroup LSRX_LL_EF_Init
 * @{
 */

/**
 * @brief De-initialize LSRX registers
 * @param LSRXx LSRX Instance
 * @return ErrorStatus value:
 *         SUCCESS: LSRX registers are de-initialized
 *         ERROR:   Wrong LSRX Port
 */
ErrorStatus LL_LSRX_DeInit(LSRX_TypeDef *LSRXx)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_LSRX_ALL_INSTANCE(LSRXx));

    if (LSRXx == LSRX_CORE0) {
        /* Force and Release reset on clock of LSRXx Port */
        LL_SYSTEM_CORE0_MOD1_ForceReset(LL_SYSTEM_CORE0_MOD1_PERIPH_LSRX);
        LL_SYSTEM_CORE0_MOD1_ReleaseReset(LL_SYSTEM_CORE0_MOD1_PERIPH_LSRX);
    }
    else if(LSRXx == LSRX_CORE1)
    {
        /* Force and Release reset on clock of LSRXx Port */
        LL_SYSTEM_CORE1_MOD1_ForceReset(LL_SYSTEM_CORE0_MOD1_PERIPH_LSRX);
        LL_SYSTEM_CORE1_MOD1_ReleaseReset(LL_SYSTEM_CORE0_MOD1_PERIPH_LSRX);
    }
    else {
        status = ERROR;
    }

    return status;
}

/**
 * @brief Initialize LSRX registers according to the specified parameters in LSRX_InitStruct.
 * @param LSRXx LSRX Instance
 * @param LSRX_InitStruct pointer to LL_LSRX_InitTypeDef structure
 * @return ErrorStatus:
 *         SUCCESS: LSRX registers are de-initialized
 *         ERROR:   Not applicable
 */
ErrorStatus LL_LSRX_Init(LSRX_TypeDef *LSRXx, LL_LSRX_InitTypeDef *LSRX_InitStruct)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_LSRX_ALL_INSTANCE(LSRXx));
    assert_param(IS_LL_LSRX_CHSEL(LSRX_InitStruct->ChnlSel0));
    assert_param(IS_LL_LSRX_CHSEL(LSRX_InitStruct->ChnlSel1));
    assert_param(IS_LL_LSRX_CHSEL(LSRX_InitStruct->ChnlSel2));
    assert_param(IS_LL_LSRX_CHSEL(LSRX_InitStruct->ChnlSel3));
    assert_param(IS_LL_LSRX_RXEN(LSRX_InitStruct->ChnlRxEn0));
    assert_param(IS_LL_LSRX_RXEN(LSRX_InitStruct->ChnlRxEn1));
    assert_param(IS_LL_LSRX_RXEN(LSRX_InitStruct->ChnlRxEn2));
    assert_param(IS_LL_LSRX_RXEN(LSRX_InitStruct->ChnlRxEn3));

    if((LSRX_InitStruct->Threshold > 0x1FU)  ||
       (LSRX_InitStruct->ParityData > 0xFFU) ||
       (LSRX_InitStruct->WaveCycle > 0x7FFFFFU))
    {
        return ERROR;
    }

    /* Note: Hardware constraint (refer to description of this function)  */
    /*       LSRX instance must be disabled.                               */
    if(LL_LSRX_IsEnabled(LSRXx) == 0UL)
    {
        /* Threshold time configuration      */
        LL_LSRX_SetThresholdTime(LSRXx, LSRX_InitStruct->Threshold);
        /* Parity data configuration  */
        LL_LSRX_SetParityData(LSRXx, LSRX_InitStruct->ParityData);
        /* Calibration wave cycle configuration     */
        LL_LSRX_SetCalibrationCycle(LSRXx, LSRX_InitStruct->WaveCycle);

        /* Channel0 select configuration      */
        LL_LSRX_SetChannel0(LSRXx, LSRX_InitStruct->ChnlSel0);
        /* Channel0 receive function configuration      */
        if(LSRX_InitStruct->ChnlRxEn0 == LL_LSRX_RXEN_ENABLE)
        {
            LL_LSRX_EnableChannel0(LSRXx);
        } else {
            LL_LSRX_DisableChannel0(LSRXx);
        }

        /* Channel1 select configuration      */
        LL_LSRX_SetChannel1(LSRXx, LSRX_InitStruct->ChnlSel1);
        /* Channel1 receive function configuration      */
        if(LSRX_InitStruct->ChnlRxEn1 == LL_LSRX_RXEN_ENABLE)
        {
            LL_LSRX_EnableChannel1(LSRXx);
        } else {
            LL_LSRX_DisableChannel1(LSRXx);
        }

        /* Channel2 select configuration      */
        LL_LSRX_SetChannel2(LSRXx, LSRX_InitStruct->ChnlSel2);
        /* Channel2 receive function configuration      */
        if(LSRX_InitStruct->ChnlRxEn2 == LL_LSRX_RXEN_ENABLE)
        {
            LL_LSRX_EnableChannel2(LSRXx);
        } else {
            LL_LSRX_DisableChannel2(LSRXx);
        }

        /* Channel3 select configuration      */
        LL_LSRX_SetChannel3(LSRXx, LSRX_InitStruct->ChnlSel3);
        /* Channel3 receive function configuration      */
        if(LSRX_InitStruct->ChnlRxEn3 == LL_LSRX_RXEN_ENABLE)
        {
            LL_LSRX_EnableChannel3(LSRXx);
        } else {
            LL_LSRX_DisableChannel3(LSRXx);
        }

        /* Enable LSRX Peripheral */
        LL_LSRX_Enable(LSRXx);
    } else {
        /* Initialization error: LSRX instance is not disabled.  */
        status = ERROR;
    }

    return status;
}

/**
 * @brief Set each @ref LL_LSRX_InitTypeDef field to default value.
 * @param LSRX_InitStruct pointer to LL_LSRX_InitTypeDef structure
 * @retval None
 */
void LL_LSRX_StructInit(LL_LSRX_InitTypeDef *LSRX_InitStruct)
{
    /* Reset LSRX init structure parameters values */
    LSRX_InitStruct->Threshold  = 0x9U;
    LSRX_InitStruct->ParityData = 0x2FU;
    LSRX_InitStruct->WaveCycle  = 0x1FFFFU;
	LSRX_InitStruct->ChnlSel0   = LL_LSRX_CHSEL_UART0;
	LSRX_InitStruct->ChnlSel1   = LL_LSRX_CHSEL_UART1;
	LSRX_InitStruct->ChnlSel2   = LL_LSRX_CHSEL_UART2;
	LSRX_InitStruct->ChnlSel3   = LL_LSRX_CHSEL_UART3;
	LSRX_InitStruct->ChnlRxEn0  = LL_LSRX_RXEN_DISBALE;
	LSRX_InitStruct->ChnlRxEn1  = LL_LSRX_RXEN_DISBALE;
	LSRX_InitStruct->ChnlRxEn2  = LL_LSRX_RXEN_DISBALE;
	LSRX_InitStruct->ChnlRxEn3  = LL_LSRX_RXEN_DISBALE;
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

#endif /* LSRX */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
