/**
 *****************************************************************************
 * @file   rn_rv_ll_lcss_vcc.c
 * @author Renergy Software Team
 * @brief  LCSS_VCC LL module driver.
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
#include "rn_rv_ll_lcss_vcc.h"

#ifdef  USE_FULL_ASSERT
#include "rn_rv_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (LCSS_VCC)

/** @addtogroup LCSS_VCC_LL LCSS_VCC
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup LCSS_VCC_LL_Private_Macros
 * @{
 */

#define IS_LL_LCSS_VCC_HF_THRESHOLD(__VALUE__)          ((__VALUE__)  <= 0x000000FFU)

#define IS_LL_LCSS_VCC_LF_THRESHOLD(__VALUE__)          ((__VALUE__)  <= 0x000000FFU)

#define IS_LL_LCSS_VCC_CLOCK_DIV(__VALUE__)             (((__VALUE__) == LL_LCSS_VCC_CLOCK_DIV_NONE) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_CLOCK_DIV_2) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_CLOCK_DIV_4) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_CLOCK_DIV_8))

#define IS_LL_LCSS_VCC_HF_CLOCK_SELECT(__VALUE__)       (((__VALUE__) == LL_LCSS_VCC_HF_SYSCLK_PLL_RCH) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_HF_PLLSOURCE_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_HF_KBI_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_HF_LPUART_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_HF_LCD_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_HF_INFRARED_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_HF_SYSCLK_LOSC_RCL))

#define IS_LL_LCSS_VCC_LF_CLOCK_SELECT(__VALUE__)       (((__VALUE__) == LL_LCSS_VCC_LF_SYSCLK_PLL_RCH) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_LF_PLLSOURCE_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_LF_KBI_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_LF_LPUART_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_LF_LCD_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_LF_INFRARED_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_LF_SYSCLK_LOSC_RCL))

#define IS_LL_LCSS_VCC_ARCV_CLOCK_SELECT(__VALUE__)     (((__VALUE__) == LL_LCSS_VCC_ARCV_SYSCLK_PLL_RCH) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_ARCV_PLLSOURCE_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_ARCV_KBI_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_ARCV_LPUART_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_ARCV_LCD_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_ARCV_INFRARED_LOSC_RCL) ||\
                                                            ((__VALUE__) == LL_LCSS_VCC_ARCV_SYSCLK_LOSC_RCL))

#define IS_LL_LCSS_VCC_ANOMALY_CYCLES(__VALUE__)        ((__VALUE__)  <= 0x0000000FU)

#define IS_LL_LCSS_VCC_RECOVERY_CYCLES(__VALUE__)       ((__VALUE__)  <= 0x0000FFFFU)
/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup LCSS_VCC_LL_Exported_Functions
 * @{
 */

/** @addtogroup LCSS_VCC_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize LCSS_VCC registers (Registers restored to their default values).
 * @param  LCSS_VCCx LCSS_VCC Instance
 * @retval An LL_ErrStatus enumeration value:
 *          - LL_SUCCESS: LCSS_VCC registers are de-initialized
 *          - LL_ERROR:   LCSS_VCC registers are not de-initialized
 */
ErrorStatus LL_LCSS_VCC_DeInit(LCSS_VCC_TypeDef *LCSS_VCCx)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_LCSS_VCC_ALL_INSTANCE(LCSS_VCCx));

    /* Unlock LCSS_VCC PSW register */
    LL_LCSS_VCC_UnlockPSW0();
    LL_LCSS_VCC_UnlockPSW1();

    /* Disable the LCSS_VCC */
    LL_LCSS_VCC_WriteReg(LCSS_VCCx, EN, 0x00000000U);

    /* Reset LCSS_VCC Auto switch configuration register */
    LL_LCSS_VCC_WriteReg(LCSS_VCCx, ASWH, 0x00000000U);

    /* Reset LCSS_VCC Control register */
    LL_LCSS_VCC_WriteReg(LCSS_VCCx, CTRL, 0x00000000U);

    /* Reset LCSS_VCC RCL32K frequency offset register */
    MODIFY_REG(LCSS_VCCx->CTRL, LCSS_VCC_CTRL_DETD_DIV | LCSS_VCC_CTRL_HF_CYC
                | LCSS_VCC_CTRL_RCV_CYC, 0x00000002U);

    /* Reset LCSS_VCC RCL32K frequency offset register */
    LL_LCSS_VCC_WriteReg(LCSS_VCCx, RCL_OS, 0x00000000U);

    /* Reset LCSS_VCC High frequency abnormal offset threshold configuration register */
    LL_LCSS_VCC_WriteReg(LCSS_VCCx, HFDLA_TH, 0x00000003U);

    /* Reset LCSS_VCC Low frequency abnormal offset threshold configuration register */
    LL_LCSS_VCC_WriteReg(LCSS_VCCx, LFDLA_TH, 0x00000031U);

    /* Reset LCSS_VCC Clock configuration register for software switch */
    LL_LCSS_VCC_WriteReg(LCSS_VCCx, CLKSWH, 0x00000000U);

    /* Reset LCSS_VCC Clock selected register */
    LL_LCSS_VCC_WriteReg(LCSS_VCCx, CLKSEL, 0x00000000U);

    /* Reset LCSS_VCC Interrupt enable register */
    LL_LCSS_VCC_WriteReg(LCSS_VCCx, IE, 0x00000000U);

    /* Reset LCSS_VCC Interrupt Status register */
    LL_LCSS_VCC_WriteReg(LCSS_VCCx, IF, 0x00000000U);

    /* Reset LCSS_VCC High frequency abnormal frequency register */
    LL_LCSS_VCC_WriteReg(LCSS_VCCx, HFREQ, 0x00000000U);

    /* Reset LCSS_VCC Low frequency abnormal frequency register */
    LL_LCSS_VCC_WriteReg(LCSS_VCCx, LFREQ, 0x00000000U);

    /* Lock LCSS_VCC PSW register */
    LL_LCSS_VCC_LockPSW0();
    LL_LCSS_VCC_LockPSW1();

    return status;
}

/**
 * @brief  Initialize the LCSS_VCC registers according to the specified parameters in LCSS_VCC_InitStruct.
 * @param  LCSS_VCCx LCSS_VCCx Instance
 * @param  LCSS_VCC_InitStruct pointer to a @ref LL_LCSS_VCC_InitTypeDef structure.
 * @retval An ErrorStatus enumeration value:
 *          - LL_SUCCESS: LCSS_VCC registers are initialized
 *          - LL_ERROR:   Not applicable
 */
ErrorStatus LL_LCSS_VCC_Init(LL_LCSS_VCC_InitTypeDef *LCSS_VCC_InitStruct)
{
    ErrorStatus status = SUCCESS;

    /* Check the LCSS_VCC Instance LCSS_VCCx */
    assert_param(IS_LCSS_VCC_ALL_INSTANCE(LCSS_VCC));

    /* Check the LCSS_VCC parameters from LCSS_VCC_InitStruct */
    assert_param(IS_LL_LCSS_VCC_HF_THRESHOLD(LCSS_VCC_InitStruct->HFThreshold));
    assert_param(IS_LL_LCSS_VCC_LF_THRESHOLD(LCSS_VCC_InitStruct->LFThreshold));
    assert_param(IS_LL_LCSS_VCC_CLOCK_DIV(LCSS_VCC_InitStruct->ClockDivision));
    assert_param(IS_LL_LCSS_VCC_HF_CLOCK_SELECT(LCSS_VCC_InitStruct->HFClockSelect));
    assert_param(IS_LL_LCSS_VCC_LF_CLOCK_SELECT(LCSS_VCC_InitStruct->LFClockSelect));
    assert_param(IS_LL_LCSS_VCC_ARCV_CLOCK_SELECT(LCSS_VCC_InitStruct->ARCVClockSelect));
    assert_param(IS_LL_LCSS_VCC_ANOMALY_CYCLES(LCSS_VCC_InitStruct->AnomalyCycles));
    assert_param(IS_LL_LCSS_VCC_RECOVERY_CYCLES(LCSS_VCC_InitStruct->RecoveryCycles));

    /* UnlocK the registers: LCSS_EN/LCSS_ASWH/LCSS_CTRL/LCSS_RCL_OS/LCSS_HFDLA_TH/LCSS_LFDLA_TH/LCSS_IE */
    LL_LCSS_VCC_UnlockPSW0();

    /*---------------------------- LCSS_VCCx Enable automatic switch to backup clock in high frequency abnormality for selected clock Configuration -----------------------
    * Enable the Clock Selected automatic switch to backup:
    * - ClockDivision: LCSS_VCC_ASWH_HF_ASWHx bit
    */
    LL_LCSS_VCC_EnableHFAutoSwitch(LCSS_VCC_InitStruct->HFClockSelect);

    /*---------------------------- LCSS_VCCx Enable automatic switch to backup clock in low frequency abnormality for selected clock Configuration -----------------------
    * Enable the Clock Selected automatic switch to backup :
    * - ClockDivision: LCSS_VCC_ASWH_LF_ASWHx bit
    */
    LL_LCSS_VCC_EnableLFAutoSwitch(LCSS_VCC_InitStruct->LFClockSelect);

    /*---------------------------- LCSS_VCCx Enable automatic switch to source clock in normal frequency for selected clock Configuration -----------------------
    * Enable the Clock Selected automatic switch to source :
    * - ClockDivision: LCSS_VCC_ASWH_CK_ARCVx bit
    */
    LL_LCSS_VCC_EnableAutoRecovery(LCSS_VCC_InitStruct->ARCVClockSelect);

    /*---------------------------- LCSS_VCCx Clock Division Configuration -----------------------
    * Configure the Clock Division :
    * - ClockDivision: LCSS_VCC_CTRL_DETD_DIV bits
    */
    LL_LCSS_VCC_SetClockDivision(LCSS_VCC_InitStruct->ClockDivision);

    /*---------------------------- LCSS_VCCx High/low Frequency Abnormality Threshold Configuration -----------------------
    * Configure the abnormality threshold :
    * - AnomalyCycles: LCSS_VCC_CTRL_HF_CYC bits
    */
    LL_LCSS_VCC_SetAnomalyCycles(LCSS_VCC_InitStruct->AnomalyCycles);

    /*---------------------------- LCSS_VCCx LOSC Recovery Time Configuration ------------------------
    * Configure the Recovery Time :
    * - RecoveryCycles: LCSS_VCC_CTRL_RCV_CYC bits
    */
    LL_LCSS_VCC_SetRecoveryCycles(LCSS_VCC_InitStruct->RecoveryCycles);

    /*---------------------------- LCSS_VCCx High Frequency Abnormal offset threshold Configuration ------------
    * Configure the High Frequency Abnormal Threshold :
    * - ClockDiv1: LCSS_VCC_HFDLA_TH_HFDLA_TH bits
    */
    LL_LCSS_VCC_SetHFThreshold(LCSS_VCC_InitStruct->HFThreshold);

    /*---------------------------- LCSS_VCCx low frequency abnormal offset threshold Configuration ------------
    * Configure the Low Frequency Abnormal Threshold :
    * - ClockDiv2: LCSS_VCC_LFDLA_TH_LFDLA_TH bits
    */
    LL_LCSS_VCC_SetLFThreshold(LCSS_VCC_InitStruct->LFThreshold);


    /* ENABLE HF Detection */
    if (LCSS_VCC_InitStruct->HFDetectionCommand != DISABLE) {
        /* Enable high frequence detection */
        LL_LCSS_VCC_EnableHFDetection();
    } else { /* DISABLE high frequence detection */
        LL_LCSS_VCC_DisableHFDetection();
    }

    /* ENABLE LF Detection */
    if (LCSS_VCC_InitStruct->LFDetectionCommand != DISABLE) {
        /* Enable low frequence detection */
        LL_LCSS_VCC_EnableLFDetection();
    } else { /* DISABLE low frequence detection */
        LL_LCSS_VCC_DisableLFDetection();
    }

    /* LocK the registers: LCSS_EN/LCSS_ASWH/LCSS_CTRL/LCSS_RCL_OS/LCSS_HFDLA_TH/LCSS_LFDLA_TH/LCSS_IE */
    LL_LCSS_VCC_LockPSW0();

    return status;
}

/**
 * @brief  Set each @ref LL_LCSS_VCC_InitTypeDef field to default value.
 * @param  LCSS_VCC_InitStruct Pointer to a @ref LL_LCSS_VCC_InitTypeDef structure.
 * @retval None
 */
void LL_LCSS_VCC_StructInit(LL_LCSS_VCC_InitTypeDef *LCSS_VCC_InitStruct)
{
    /* Set LCSS_VCC_InitStruct fields to default values */
    LCSS_VCC_InitStruct->HFThreshold        = 0x3;
    LCSS_VCC_InitStruct->LFThreshold        = 0x31;
    LCSS_VCC_InitStruct->HFDetectionCommand = ENABLE;
    LCSS_VCC_InitStruct->LFDetectionCommand = ENABLE;
    LCSS_VCC_InitStruct->ClockDivision      = LL_LCSS_VCC_CLOCK_DIV_4;
    LCSS_VCC_InitStruct->HFClockSelect      = LL_LCSS_VCC_HF_SYSCLK_PLL_RCH;
    LCSS_VCC_InitStruct->LFClockSelect      = LL_LCSS_VCC_LF_SYSCLK_PLL_RCH;
    LCSS_VCC_InitStruct->ARCVClockSelect    = LL_LCSS_VCC_ARCV_SYSCLK_PLL_RCH;
    LCSS_VCC_InitStruct->AnomalyCycles      = 0x0;
    LCSS_VCC_InitStruct->RecoveryCycles     = 0x0;
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

#endif /* LCSS_VCC */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
