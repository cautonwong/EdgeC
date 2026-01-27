/**
 *****************************************************************************
 * @file   rn_rv_ll_lcss_vbat.c
 * @author Renergy Software Team
 * @brief  LCSS_VBAT LL module driver.
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
#include "rn_rv_ll_lcss_vbat.h"

#ifdef  USE_FULL_ASSERT
#include "rn_rv_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (LCSS_VBAT)

/** @addtogroup LCSS_VBAT_LL LCSS_VBAT
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup LCSS_VBAT_LL_Private_Macros
 * @{
 */

#define IS_LL_LCSS_VBAT_STOP_THRESHOLD(__VALUE__)         ((__VALUE__)  <= 0x0000001FU)

#define IS_LL_LCSS_VBAT_RECOVERY_CYCLES(__VALUE__)        ((__VALUE__)  <= 0x000000FEU)

#define IS_LL_LCSS_VBAT_TIMER_CONFIG(__VALUE__)           (((__VALUE__) == LL_LCSS_VBAT_TIMING_NONE) ||\
                                                            ((__VALUE__) == LL_LCSS_VBAT_TIMING_10MS) ||\
                                                            ((__VALUE__) == LL_LCSS_VBAT_TIMING_20MS) ||\
                                                            ((__VALUE__) == LL_LCSS_VBAT_TIMING_40MS))
/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup LCSS_VBAT_LL_Exported_Functions
 * @{
 */

/** @addtogroup LCSS_VBAT_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize LCSS_VBAT registers (Registers restored to their default values).
 * @param  LCSS_VBATx LCSS_VBAT Instance
 * @retval An LL_ErrStatus enumeration value:
 *          - LL_SUCCESS: LCSS_VBAT registers are de-initialized
 *          - LL_ERROR:   LCSS_VBAT registers are not de-initialized
 */
ErrorStatus LL_LCSS_VBAT_DeInit(LCSS_VBAT_TypeDef *LCSS_VBATx)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_LCSS_VBAT_ALL_INSTANCE(LCSS_VBATx));

    /* Unlock LCSS_VCC PSW register */
    LL_LCSS_VBAT_UnlockPSW0(LCSS_VBATx);
    LL_LCSS_VBAT_UnlockPSW1(LCSS_VBATx);

    /* Disable the LCSS_VBAT */
    LL_LCSS_VBAT_Disable(LCSS_VBATx);

    /* Reset LCSS_VBAT control register */
    LL_LCSS_VBAT_SetStopThreshold(LCSS_VBATx, 0x1FU);
    MODIFY_REG(LCSS_VBATx->CTRL, LCSS_VBAT_CTRL_RCV_CYC | LCSS_VBAT_CTRL_STOP_ASWH
                | LCSS_VBAT_CTRL_RCV_ASWH | LCSS_VBAT_CTRL_TC_CFG, 0x00000000U);

    /* Reset LCSS_VBAT Clock configuration register */
    LL_LCSS_VBAT_SetRTCClockSource(LCSS_VBATx, LL_LCSS_VBAT_RTC_LOSC);

    /* Reset LCSS_VBAT interrupt enable register */
    LL_LCSS_VBAT_WriteReg(LCSS_VBATx, IE, 0x0U);

    /* Reset LCSS_VBAT interrupt pending register */
    LL_LCSS_VBAT_WriteReg(LCSS_VBATx, IF, 0xFU);

    /* Lock LCSS_VBAT PSW register */
    LL_LCSS_VBAT_LockPSW0(LCSS_VBATx);
    LL_LCSS_VBAT_LockPSW1(LCSS_VBATx);

    return status;
}

/**
 * @brief  Initialize the LCSS_VBAT registers according to the specified parameters in LCSS_VBAT_InitStruct.
 * @param  LCSS_VBATx LCSS_VBATx Instance
 * @param  LCSS_VBAT_InitStruct pointer to a @ref LL_LCSS_VBAT_InitTypeDef structure.
 * @retval An ErrorStatus enumeration value:
 *          - LL_SUCCESS: LCSS_VBAT registers are initialized
 *          - LL_ERROR:   Not applicable
 */
ErrorStatus LL_LCSS_VBAT_Init(LCSS_VBAT_TypeDef *LCSS_VBATx, LL_LCSS_VBAT_InitTypeDef *LCSS_VBAT_InitStruct)
{
    ErrorStatus status = SUCCESS;

    /* Check the LCSS_VBAT Instance LCSS_VBATx */
    assert_param(IS_LCSS_VBAT_ALL_INSTANCE(LCSS_VBATx));

    /* Check the LCSS_VBAT parameters from LCSS_VBAT_InitStruct */
    assert_param(IS_LL_LCSS_VBAT_STOP_THRESHOLD(LCSS_VBAT_InitStruct->StopThreshold));
    assert_param(IS_LL_LCSS_VBAT_RECOVERY_CYCLES(LCSS_VBAT_InitStruct->RecoveryCycles));
    assert_param(IS_LL_LCSS_VBAT_TIMER_CONFIG(LCSS_VBAT_InitStruct->TimerConfig));

    /* UnlocK the registers: LCSS_VBAT_EN/LCSS_CTRL/LCSS_IE */
    LL_LCSS_VBAT_UnlockPSW0(LCSS_VBATx);

    /* Disable the selected LCSS_VBATx */
    LL_LCSS_VBAT_Disable(LCSS_VBATx);

    /*---------------------------- LCSS_VBATx Clock Stop Threshold Configuration ------------------------
    * Configure the Clock Stop Threshold :
    * - StopThreshold: LCSS_VBAT_CTRL_STOP_TH bits
    */
    LL_LCSS_VBAT_SetStopThreshold(LCSS_VBATx, LCSS_VBAT_InitStruct->StopThreshold);

    /*---------------------------- LCSS_VBATx LOSC Recovery Cycles Configuration ------------
    * Configure the LOSC Recovery Cycles :
    * - RecoveryCycles: LCSS_VBAT_CTRL_RCV_CYC bits
    */
    LL_LCSS_VBAT_SetRecoveryCycles(LCSS_VBATx, LCSS_VBAT_InitStruct->RecoveryCycles);

    /*---------------------------- LCSS_VBATx Timing Detection Configuration ------------
    * Configure the Timing Detection :
    * - ClockDiv2: LCSS_VBAT_CTRL_TC_CFG bits
    */
    LL_LCSS_VBAT_SetTimerConfig(LCSS_VBATx, LCSS_VBAT_InitStruct->TimerConfig);

    /* ENABLE Auto switch when clock stopped function */
    if (LCSS_VBAT_InitStruct->StopAutoSwitchCommand != DISABLE) {
        /* Enable Auto switch when clock stopped function */
        LL_LCSS_VBAT_EnableStopAutoSwitch(LCSS_VBATx);
    } else { /* DISABLE Auto switch when clock stopped function */
        LL_LCSS_VBAT_DisableStopAutoSwitch(LCSS_VBATx);
    }

    /* ENABLE Auto switch when clock recovery function */
    if (LCSS_VBAT_InitStruct->AutoRecoveryCommand != DISABLE) {
        /* Enable Auto switch when clock recovery function */
        LL_LCSS_VBAT_EnableAutoRecovery(LCSS_VBATx);
    } else { /* DISABLE Auto switch when clock recovery function */
        LL_LCSS_VBAT_DisableAutoRecovery(LCSS_VBATx);
    }

    /* Enable the selected LCSS_VBATx */
    LL_LCSS_VBAT_Enable(LCSS_VBATx);

    /* LocK the registers: LCSS_VBAT_EN/LCSS_CTRL/LCSS_IE */
    LL_LCSS_VBAT_LockPSW0(LCSS_VBATx);

    return status;
}

/**
 * @brief  Set each @ref LL_LCSS_VBAT_InitTypeDef field to default value.
 * @param  LCSS_VBAT_InitStruct Pointer to a @ref LL_LCSS_VBAT_InitTypeDef structure.
 * @retval None
 */
void LL_LCSS_VBAT_StructInit(LL_LCSS_VBAT_InitTypeDef *LCSS_VBAT_InitStruct)
{
    /* Set LCSS_VBAT_InitStruct fields to default values */
    LCSS_VBAT_InitStruct->StopThreshold         = 0x1F;
    LCSS_VBAT_InitStruct->RecoveryCycles        = 0x0;
    LCSS_VBAT_InitStruct->StopAutoSwitchCommand = ENABLE;
    LCSS_VBAT_InitStruct->AutoRecoveryCommand   = ENABLE;
    LCSS_VBAT_InitStruct->TimerConfig           = LL_LCSS_VBAT_TIMING_NONE;
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

#endif /* LCSS_VBAT */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
