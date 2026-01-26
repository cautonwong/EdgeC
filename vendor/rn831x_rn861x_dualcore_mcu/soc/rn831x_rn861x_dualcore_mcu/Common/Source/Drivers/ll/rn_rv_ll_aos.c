/**
 *****************************************************************************
 * @file   rn_rv_ll_aos.c
 * @author Renergy Software Team
 * @brief  AOS LL module driver.
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
#include "rn_rv_ll_aos.h"
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
#if defined (AOS)

/** @addtogroup AOS_LL AOS
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup AOS_LL_Private_Macros
 * @{
 */

#define IS_LL_AOS_TARGET_PURPOSE(__VALUE__)    (((__VALUE__) == LL_AOS_TARGET_0_TC0)       || \
                                                 ((__VALUE__) == LL_AOS_TARGET_1_TC0)      || \
                                                 ((__VALUE__) == LL_AOS_TARGET_2_TC0)      || \
                                                 ((__VALUE__) == LL_AOS_TARGET_3_TC0)      || \
                                                 ((__VALUE__) == LL_AOS_TARGET_4_TC1)      || \
                                                 ((__VALUE__) == LL_AOS_TARGET_5_TC1)      || \
                                                 ((__VALUE__) == LL_AOS_TARGET_6_TC1)      || \
                                                 ((__VALUE__) == LL_AOS_TARGET_7_TC1)      || \
                                                 ((__VALUE__) == LL_AOS_TARGET_8_SIMPTC0)  || \
                                                 ((__VALUE__) == LL_AOS_TARGET_9_SIMPTC0)  || \
                                                 ((__VALUE__) == LL_AOS_TARGET_10_SIMPTC0) || \
                                                 ((__VALUE__) == LL_AOS_TARGET_11_SIMPTC0) || \
                                                 ((__VALUE__) == LL_AOS_TARGET_12_SIMPTC1) || \
                                                 ((__VALUE__) == LL_AOS_TARGET_13_SIMPTC1) || \
                                                 ((__VALUE__) == LL_AOS_TARGET_14_SIMPTC1) || \
                                                 ((__VALUE__) == LL_AOS_TARGET_15_SIMPTC1) || \
                                                 ((__VALUE__) == LL_AOS_TARGET_16_SIMPTC2) || \
                                                 ((__VALUE__) == LL_AOS_TARGET_17_SIMPTC2) || \
                                                 ((__VALUE__) == LL_AOS_TARGET_18_SIMPTC2) || \
                                                 ((__VALUE__) == LL_AOS_TARGET_19_SIMPTC2) || \
                                                 ((__VALUE__) == LL_AOS_TARGET_20_SIMPTC3) || \
                                                 ((__VALUE__) == LL_AOS_TARGET_21_SIMPTC3) || \
                                                 ((__VALUE__) == LL_AOS_TARGET_22_SIMPTC3) || \
                                                 ((__VALUE__) == LL_AOS_TARGET_23_SIMPTC3) || \
                                                 ((__VALUE__) == LL_AOS_TARGET_24_D2F)     || \
                                                 ((__VALUE__) == LL_AOS_TARGET_25_IOCNT)   || \
                                                 ((__VALUE__) == LL_AOS_TARGET_26_IOCNT)   || \
                                                 ((__VALUE__) == LL_AOS_TARGET_27_IOCNT)   || \
                                                 ((__VALUE__) == LL_AOS_TARGET_28_IOCNT)   || \
                                                 ((__VALUE__) == LL_AOS_TARGET_29_IOCNT)   || \
                                                 ((__VALUE__) == LL_AOS_TARGET_30_RC_CAL)  || \
                                                 ((__VALUE__) == LL_AOS_TARGET_31_CAN)     || \
                                                 ((__VALUE__) == LL_AOS_TARGET_32_ADC0)   || \
                                                 ((__VALUE__) == LL_AOS_TARGET_33_ADC0)   || \
                                                 ((__VALUE__) == LL_AOS_TARGET_34_DAC))

#define IS_LL_AOS_TRIGGER_SIGNAL(__VALUE__)    ((__VALUE__) <= 37U)
/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup AOS_LL_Exported_Functions
 * @{
 */

/** @addtogroup AOS_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize AOS registers (Registers restored to their default values).
 * @param  AOSx AOS Instance
 * @retval An LL_ErrStatus enumeration value:
 *          - LL_SUCCESS: AOS registers are de-initialized
 *          - LL_ERROR:   AOS registers are not de-initialized
 */
ErrorStatus LL_AOS_DeInit(AOS_TypeDef *AOSx)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_AOS_ALL_INSTANCE(AOSx));

    if (AOSx == AOS_CORE0) {
        /* Force reset of AOS clock */
        LL_SYSTEM_CORE0_PMOD0_ForceReset(LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_AOS);

        /* Release reset of AOS clock */
        LL_SYSTEM_CORE0_PMOD0_ReleaseReset(LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_AOS);
    } else if (AOSx == AOS_CORE1) {
        /* Force reset of AOS clock */
        LL_SYSTEM_CORE1_PMOD0_ForceReset(LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_AOS);

        /* Release reset of AOS clock */
        LL_SYSTEM_CORE1_PMOD0_ReleaseReset(LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_AOS);
    }
    else {
        status = ERROR;
    }

    return status;
}

/**
 * @brief  Initialize the AOS registers according to the specified parameters in AOS_InitStruct.
 * @param  AOSx AOSx Instance
 * @param  AOS_InitStruct pointer to a @ref LL_AOS_InitTypeDef structure.
 * @retval An ErrorStatus enumeration value:
 *          - LL_SUCCESS: AOS registers are initialized
 *          - LL_ERROR:   Not applicable
 */
ErrorStatus LL_AOS_Init(AOS_TypeDef *AOSx, LL_AOS_InitTypeDef *AOS_InitStruct)
{
    ErrorStatus status = SUCCESS;

    /* Check the AOS Instance AOSx */
    assert_param(IS_AOS_ALL_INSTANCE(AOSx));

    /* Check the AOS parameters from AOS_InitStruct */
    assert_param(IS_LL_AOS_TARGET_PURPOSE(AOS_InitStruct->TargetPurpose));
    assert_param(IS_LL_AOS_TRIGGER_SIGNAL(AOS_InitStruct->TriggerSignal));

    /* Disable the selected AOSx Peripheral */
    if (AOSx == AOS_CORE0) {
        LL_AOS_DisableSecurityPeriph();
    } else if (AOSx == AOS_CORE1) {
        LL_AOS_DisableNonSecurityPeriph();
    }

    /*---------------------------- AOSx Target Purpose Configuration -----------------------
    * Configure AOSx Target Purpose and Trigger Signal with parameter:
    * - TargetPurpose: AOS_PE_TRGSELx registers
    * - TriggerSignal: AOS_PE_TRGSEL bits
    */
    if ((AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_0_TC0)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_1_TC0)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_2_TC0)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_3_TC0)) {
        LL_AOS_SetTriggerSignal_TC0(AOSx, AOS_InitStruct->TargetPurpose, AOS_InitStruct->TriggerSignal);
    } else if ((AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_4_TC1)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_5_TC1)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_6_TC1)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_7_TC1)) {
        LL_AOS_SetTriggerSignal_TC1(AOSx, AOS_InitStruct->TargetPurpose, AOS_InitStruct->TriggerSignal);
    } else if ((AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_8_SIMPTC0)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_9_SIMPTC0)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_10_SIMPTC0)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_11_SIMPTC0)) {
        LL_AOS_SetTriggerSignal_SIMPTC0(AOSx, AOS_InitStruct->TargetPurpose, AOS_InitStruct->TriggerSignal);
    } else if ((AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_12_SIMPTC1)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_13_SIMPTC1)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_14_SIMPTC1)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_15_SIMPTC1)) {
        LL_AOS_SetTriggerSignal_SIMPTC1(AOSx, AOS_InitStruct->TargetPurpose, AOS_InitStruct->TriggerSignal);
    } else if ((AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_16_SIMPTC2)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_17_SIMPTC2)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_18_SIMPTC2)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_19_SIMPTC2)) {
        LL_AOS_SetTriggerSignal_SIMPTC2(AOSx, AOS_InitStruct->TargetPurpose, AOS_InitStruct->TriggerSignal);
    } else if ((AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_20_SIMPTC3)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_21_SIMPTC3)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_22_SIMPTC3)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_23_SIMPTC3)) {
        LL_AOS_SetTriggerSignal_SIMPTC3(AOSx, AOS_InitStruct->TargetPurpose, AOS_InitStruct->TriggerSignal);
    } else if ((AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_24_D2F)) {
        LL_AOS_SetTriggerSignal_D2F(AOSx, AOS_InitStruct->TriggerSignal);
    } else if ((AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_25_IOCNT)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_26_IOCNT)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_27_IOCNT)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_28_IOCNT)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_29_IOCNT)) {
        LL_AOS_SetTriggerSignal_IOCNT(AOSx, AOS_InitStruct->TargetPurpose, AOS_InitStruct->TriggerSignal);
    } else if ((AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_30_RC_CAL)) {
        LL_AOS_SetTriggerSignal_RC_CAL(AOSx, AOS_InitStruct->TriggerSignal);
    } else if ((AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_31_CAN)) {
        LL_AOS_SetTriggerSignal_CAN(AOSx, AOS_InitStruct->TriggerSignal);
    } else if ((AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_32_ADC0)\
        || (AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_33_ADC0)) {
        LL_AOS_SetTriggerSignal_ADC0(AOSx, AOS_InitStruct->TargetPurpose, AOS_InitStruct->TriggerSignal);
    } else if ((AOS_InitStruct->TargetPurpose == LL_AOS_TARGET_34_DAC)) {
        LL_AOS_SetTriggerSignal_RC_CAL(AOSx, AOS_InitStruct->TriggerSignal);
    }

    /* Enable the selected AOSx Peripheral */
    if (AOSx == AOS_CORE0) {
        LL_AOS_EnableSecurityPeriph();
    } else if (AOSx == AOS_CORE1) {
        LL_AOS_EnableNonSecurityPeriph();
    }

    return status;
}

/**
 * @brief  Set each @ref LL_AOS_InitTypeDef field to default value.
 * @param  AOS_InitStruct Pointer to a @ref LL_AOS_InitTypeDef structure.
 * @retval None
 */
void LL_AOS_StructInit(LL_AOS_InitTypeDef *AOS_InitStruct)
{
  /* Set AOS_InitStruct fields to default values */
    AOS_InitStruct->TargetPurpose = LL_AOS_TARGET_0_TC0;
    AOS_InitStruct->TriggerSignal = LL_AOS_TARGET_0_3_TRIGGER_NONE;
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
#endif /* AOS */

/**
 * @}
 */
#endif /* USE_FULL_LL_DRIVER */
