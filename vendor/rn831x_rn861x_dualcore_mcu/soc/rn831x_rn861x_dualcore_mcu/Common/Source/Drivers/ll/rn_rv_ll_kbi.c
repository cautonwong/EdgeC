/**
 *****************************************************************************
 * @file   rn_rv_ll_kbi.c
 * @author Renergy Software Team
 * @brief  KBI LL module driver.
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
#include "rn_rv_ll_kbi.h"
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

#if defined (KBI0) || defined (KBI1)

/** @addtogroup KBI_LL KBI
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup KBI_LL_Private_Macros
 * @{
 */

#define IS_LL_KBI_CHANNEL_0_15(__VALUE__)        (((__VALUE__) & ~LL_KBI_CHANNEL_ALL) == 0x00000000U)

#define IS_LL_KBI_TRIGGER_MODE(__VALUE__)        (((__VALUE__) == LL_KBI_TRIGGER_RISING) ||\
                                                  ((__VALUE__) == LL_KBI_TRIGGER_FALLING) ||\
                                                  ((__VALUE__) == LL_KBI_TRIGGER_RISING_FALLING))

#define IS_LL_KBI_CLOCK_SELECT(__VALUE__)        (((__VALUE__) == LL_KBI_FILTER_CLOCK_32_768) ||\
                                                  ((__VALUE__) == LL_KBI_FILTER_CLOCK_SYSCLK))

#define IS_LL_KBI_CLOCK_DIV1(__VALUE__)          (((__VALUE__) == LL_KBI_FILTER_CLOCK_DIV_NONE) || \
                                                  ((__VALUE__) == LL_KBI_FILTER_CLOCK_DIV_32))

#define IS_LL_KBI_CLOCK_DIV2(__VALUE__)          ((__VALUE__) <= 0x0000001FU)
/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup KBI_LL_Exported_Functions
 * @{
 */

/** @addtogroup KBI_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize KBI registers (Registers restored to their default values).
 * @param  KBIx KBI Instance
 * @retval An LL_ErrStatus enumeration value:
 *          - LL_SUCCESS: KBI registers are de-initialized
 *          - LL_ERROR:   KBI registers are not de-initialized
 */
ErrorStatus LL_KBI_DeInit(KBI_TypeDef *KBIx)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_KBI_ALL_INSTANCE(KBIx));

    if (KBIx == KBI0_CORE0) {
        /* Force reset of KBI clock */
        LL_SYSTEM_CORE0_MOD1_ForceReset(LL_SYSTEM_CORE0_MOD1_PERIPH_KBI0);

        /* Release reset of KBI clock */
        LL_SYSTEM_CORE0_MOD1_ReleaseReset(LL_SYSTEM_CORE0_MOD1_PERIPH_KBI0);
    } else if (KBIx == KBI1_CORE0) {
        /* Force reset of KBI clock */
        LL_SYSTEM_CORE0_MOD1_ForceReset(LL_SYSTEM_CORE0_MOD1_PERIPH_KBI1);

        /* Release reset of KBI clock */
        LL_SYSTEM_CORE0_MOD1_ReleaseReset(LL_SYSTEM_CORE0_MOD1_PERIPH_KBI1);
    } else if (KBIx == KBI0_CORE1) {
        /* Force reset of KBI clock */
        LL_SYSTEM_CORE1_MOD1_ForceReset(LL_SYSTEM_CORE1_MOD1_PERIPH_KBI0);

        /* Release reset of KBI clock */
        LL_SYSTEM_CORE1_MOD1_ReleaseReset(LL_SYSTEM_CORE1_MOD1_PERIPH_KBI0);
    } else if (KBIx == KBI1_CORE1) {
        /* Force reset of KBI clock */
        LL_SYSTEM_CORE1_MOD1_ForceReset(LL_SYSTEM_CORE1_MOD1_PERIPH_KBI1);

        /* Release reset of KBI clock */
        LL_SYSTEM_CORE1_MOD1_ReleaseReset(LL_SYSTEM_CORE1_MOD1_PERIPH_KBI1);
    } else {
        status = ERROR;
    }

    return status;
}

/**
 * @brief  Initialize the KBI registers according to the specified parameters in KBI_InitStruct.
 * @param  KBIx KBIx Instance
 * @param  KBI_InitStruct pointer to a @ref LL_KBI_InitTypeDef structure.
 * @retval An ErrorStatus enumeration value:
 *          - LL_SUCCESS: KBI registers are initialized
 *          - LL_ERROR:   Not applicable
 */
ErrorStatus LL_KBI_Init(KBI_TypeDef *KBIx, LL_KBI_InitTypeDef *KBI_InitStruct)
{
    ErrorStatus status = SUCCESS;

    /* Check the KBI Instance KBIx */
    assert_param(IS_KBI_ALL_INSTANCE(KBIx));

    /* Check the KBI parameters from KBI_InitStruct */
    assert_param(IS_LL_KBI_CHANNEL_0_15(KBI_InitStruct->Channel_0_15));
    assert_param(IS_LL_KBI_TRIGGER_MODE(KBI_InitStruct->TriggerMode));
    assert_param(IS_LL_KBI_CLOCK_SELECT(KBI_InitStruct->ClockSelect));
    assert_param(IS_LL_KBI_CLOCK_DIV1(KBI_InitStruct->ClockDiv1));
    assert_param(IS_LL_KBI_CLOCK_DIV2(KBI_InitStruct->ClockDiv2));

    /* Enable the selected KBIx clock */

    /* Disable the selected KBIx Channelx clock */

    /* Disable the selected KBIx Channelx */
    LL_KBI_DisableChannel_0_15(KBIx, KBI_InitStruct->Channel_0_15);

    /* ENABLE ITCommand */
    if (KBI_InitStruct->ITCommand != DISABLE) {
        /* Configure KBI interrupt Channels in range from 0 to 15 */
        LL_KBI_EnableIT_0_15(KBIx, KBI_InitStruct->Channel_0_15);
    } else {  /* DISABLE LineCommand */
        /* De-configure KBI interrupt Channels in range from 0 to 15 */
        LL_KBI_DisableIT_0_15(KBIx, KBI_InitStruct->Channel_0_15);
    }

    /*---------------------------- KBIx Trigger Mode Configuration -----------------------
    * Configure the Trigger Mode :
    * - TriggerMode: KBI_MODE_MODEx bits
    */
    LL_KBI_SetTriggerModeChannel_0_15(KBIx, KBI_InitStruct->Channel_0_15, KBI_InitStruct->TriggerMode);

    /*---------------------------- KBIx Filter Clock Select Configuration ------------------------
    * Configure the Filter Clock Select :
    * - ClockSelect: KBI_CHx_FLT_TIME_CLK_SEL bit
    */
    LL_KBI_SetFilterClock_0_15(KBIx, KBI_InitStruct->Channel_0_15, KBI_InitStruct->ClockSelect);

    /*---------------------------- KBIx Filter Clock Division 1 Configuration ------------
    * Configure the Filter Clock Division 1 :
    * - ClockDiv1: KBI_CHx_FLT_TIME_CLK_DIV1 bit
    */
    LL_KBI_SetFilterClockDiv1_0_15(KBIx, KBI_InitStruct->Channel_0_15, KBI_InitStruct->ClockDiv1);

    /*---------------------------- KBIx Filter Clock Division 2 Configuration ------------
    * Configure the Filter Clock Division 2 :
    * - ClockDiv2: KBI_CHx_FLT_TIME_CLK_DIV2 bits
    */
    LL_KBI_SetFilterClockDiv2_0_15(KBIx, KBI_InitStruct->Channel_0_15, KBI_InitStruct->ClockDiv2);

    /* Enable the selected KBIx Channelx clock */

    /* Enable the selected KBIx channels in range from 0 to 15 */
    LL_KBI_EnableChannel_0_15(KBIx, KBI_InitStruct->Channel_0_15);

    return status;
}

/**
 * @brief  Set each @ref LL_KBI_InitTypeDef field to default value.
 * @param  KBI_InitStruct Pointer to a @ref LL_KBI_InitTypeDef structure.
 * @retval None
 */
void LL_KBI_StructInit(LL_KBI_InitTypeDef *KBI_InitStruct)
{
    /* Set KBI_InitStruct fields to default values */
    KBI_InitStruct->Channel_0_15 = LL_KBI_CHANNEL_1;
    KBI_InitStruct->ITCommand    = ENABLE;
    KBI_InitStruct->TriggerMode  = LL_KBI_TRIGGER_RISING;
    /* For keyboard mode (filter time set) */
    KBI_InitStruct->ClockSelect  = LL_KBI_FILTER_CLOCK_32_768;
    KBI_InitStruct->ClockDiv1    = LL_KBI_FILTER_CLOCK_DIV_32;
    KBI_InitStruct->ClockDiv2    = 0x13U;
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
#endif /* KBI0 || KBI1 */

/**
 * @}
 */
#endif /* USE_FULL_LL_DRIVER */
