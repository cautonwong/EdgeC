/**
 ******************************************************************************
 * @file   rn_rv_hal_system.c
 * @author Renergy Software Team
 * @brief  SYSTEM HAL module driver.
 *          This file provides firmware functions to manage the following
 *          functionalities of the internal SYSTEM memory:
 *           + Program operations functions
 *           + Memory Control functions
 *           + Peripheral State functions
 *
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
#include "rn_rv_hal.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @defgroup SYSTEM SYSTEM
 * @brief     SYSTEM driver modules
 * @{
 */
#if defined(HAL_SYSTEM_MODULE_ENABLED) && defined(HAL_RPC_MODULE_ENABLED)

/* Private typedef ----------------------------------------------------------*/
/** @defgroup SYSTEM_Private_Typedef SYSTEM Private Typedef
 * @{
 */

/**
 * @brief  SYSTEM IAP interface Structure definition
 */
typedef struct
{
    uint32_t Reserved[4];
    uint32_t (*SYSTEM_ClockModeConfig)     (HAL_SYSTEM_ClockModeTypeDef ClockMode, HAL_SYSTEM_SourceDivTypeDef SourceDivison, HAL_SYSTEM_CoreDivTypeDef CoreDivison);
    void     (*SYSTEM_FastClockModeConfig) (HAL_SYSTEM_ClockModeTypeDef ClockMode);
    uint32_t (*SYSTEM_SetCoreDivision)     (HAL_SYSTEM_CoreDivTypeDef CoreDivison);
    uint32_t (*SYSTEM_PLLOutputFreqConfig) (HAL_SYSTEM_ClockModeTypeDef PLLFrequency, HAL_SYSTEM_PLLSwitchTypeDef PLLState);
    uint32_t (*SYSTEM_OptionConfigCheck)   (void);
    uint32_t (*SYSTEM_GetRTCDota)          (void);
    uint32_t (*SYSTEM_SetRTCDota)          (uint16_t RTCDota);
} SYSTEM_ApiTypeDef;

/**
 * @brief  HAL SYSTEM IAP function pointer definition
 */
typedef uint32_t (*pSYSTEM_ClockModeConfig)     (HAL_SYSTEM_ClockModeTypeDef ClockMode, HAL_SYSTEM_SourceDivTypeDef SourceDivison, HAL_SYSTEM_CoreDivTypeDef CoreDivison);
typedef void     (*pSYSTEM_FastClockModeConfig) (HAL_SYSTEM_ClockModeTypeDef ClockMode);
typedef uint32_t (*pSYSTEM_SetCoreDivision)     (HAL_SYSTEM_CoreDivTypeDef CoreDivison);
typedef uint32_t (*pSYSTEM_PLLOutputFreqConfig) (HAL_SYSTEM_ClockModeTypeDef PLLFrequency, HAL_SYSTEM_PLLSwitchTypeDef PLLState);
typedef uint32_t (*pSYSTEM_OptionConfigCheck)   (void);

/* RTCDota set and get functions */
typedef uint32_t (*pSYSTEM_GetRTCDota)          (void);
typedef uint32_t (*pSYSTEM_SetRTCDota)          (uint16_t RTCDota);

/**
 * @}
 */

/* Private define -----------------------------------------------------------*/
/** @defgroup SYSTEM_Private_Constants SYSTEM Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macro ------------------------------------------------------------*/
/** @defgroup SYSTEM_Private_Macros SYSTEM Private Macros
 * @{
 */

/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/** @defgroup SYSTEM_Private_Functions SYSTEM Private Functions
 * @{
 */
volatile bool core1_reset_interrupt = false;
/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup SYSTEM_Exported_Functions SYSTEM Exported Functions
 * @{
 */

/** @defgroup SYSTEM_Exported_Functions_Group1 Clock Mode and Frequency Config functions
 *  @brief     management system Clock Mode, Frequency and Division functions.
 *
@verbatim
================================================================================
                ##### Clock Mode and Frequency Config functions #####
================================================================================
    [..]
      This subsection provides a set of functions allowing to control the SYSTEM
      clock and frequency operations.

@endverbatim
 * @{
 */

/**
 * @brief  Configure the system main clock and frequency.
 * @note   Core0's clock frequency should not be exceed 29.4912Mhz.
 *         If ClockMode is HAL_SYSTEM_CLKMODE_PLL_59M or HAL_SYSTEM_CLKMODE_RC_59M,
 *         CoreDivison can not be HAL_SYSTEM_COREDIV_CORE1_1_CORE0_1, when SourceDivison is HAL_SYSTEM_SRCDIV_1.
 * @param  ClockMode: Indicate the source clock mode of system.
 *                    This parameter can be a value of @ref HAL_SYSTEM_ClockModeTypeDef.
 * @param  SourceDivison: Specify the source clock division(only for HOSC/PLL/RC mode) to be configed.
 *                        This parameter can be a value of @ref HAL_SYSTEM_SourceDivTypeDef.
 * @param  CoreDivison:   Specify the core0 and core1 clock divison(only for HOSC/PLL/RC mode) to be configed.
 *                        This parameter can be a value of @ref HAL_SYSTEM_CoreDivTypeDef.
 * @retval HAL_StatusTypeDef HAL Status
 */
HAL_StatusTypeDef HAL_SYSTEM_ClockModeConfig(HAL_SYSTEM_ClockModeTypeDef ClockMode, HAL_SYSTEM_SourceDivTypeDef SourceDivison, HAL_SYSTEM_CoreDivTypeDef CoreDivison)
{
#ifdef CORE0
    uint32_t iap_addr;
    uint32_t result = 0xFFU;

    /* config HOSC */
    __HAL_SYSTEM_PSW_UNLOCK();
    if (ClockMode == HAL_SYSTEM_CLKMODE_HOSC_7M4) {
        LL_SYSTEM_CORE0_SetHOSCAnalogFilterTime(LL_SYSTEM_CORE0_HOSCTIME_25_5NS);
        LL_SYSTEM_CORE0_SetHOSCClockGain(LL_SYSTEM_CORE0_HOSCGAIN_1X_0_8MS);
    } else if (ClockMode == HAL_SYSTEM_CLKMODE_HOSC_14M7) {
        LL_SYSTEM_CORE0_SetHOSCAnalogFilterTime(LL_SYSTEM_CORE0_HOSCTIME_10_5NS);
        LL_SYSTEM_CORE0_SetHOSCClockGain(LL_SYSTEM_CORE0_HOSCGAIN_2X_2_4MS);
    } else if (ClockMode == HAL_SYSTEM_CLKMODE_HOSC_29M5) {
        LL_SYSTEM_CORE0_SetHOSCAnalogFilterTime(LL_SYSTEM_CORE0_HOSCTIME_5NS);
        LL_SYSTEM_CORE0_SetHOSCClockGain(LL_SYSTEM_CORE0_HOSCGAIN_4X_3_2MS);
    }
    __HAL_SYSTEM_PSW_LOCK();
    /* If ClockMode is not 32K or auto_standby not 0, close auto_standby */
    if (((ClockMode != HAL_SYSTEM_CLKMODE_LOSC) || (ClockMode != HAL_SYSTEM_CLKMODE_RCL_32K)) && ((FLASH->CTRL & 0x1U) == 0x1U)) {
        FLASH->CTRL &= ~(1 << 0);
    }

    /* Call IAP */
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_ERROR;
    }
    SYSTEM_ApiTypeDef *Iap_Func = (SYSTEM_ApiTypeDef *)iap_addr;
    pSYSTEM_ClockModeConfig func_ptr = (pSYSTEM_ClockModeConfig)((uint32_t)Iap_Func->SYSTEM_ClockModeConfig + ROM_BASE_ADDR);
    result = func_ptr(ClockMode, SourceDivison, CoreDivison);

    /* If ClockMode is 32K and return pass, open auto_standby */
    if (((ClockMode == HAL_SYSTEM_CLKMODE_LOSC) || (ClockMode == HAL_SYSTEM_CLKMODE_RCL_32K)) && (result == HAL_OK)) {
        FLASH->CTRL |= 1 << 0;
    }
    return result;
#endif /* CORE0 */
#ifdef CORE1
    uint32_t api_id = SYSTEM_CLOCKMODECONFIG_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)ClockMode;
    api_params[2] = (uint32_t)SourceDivison;
    api_params[3] = (uint32_t)CoreDivison;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 3, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_StatusTypeDef)result;
    }
    return (HAL_StatusTypeDef)api_params[1];
#endif /* CORE1 */
}

/**
 * @brief  Configure fast mode change of system clock.
 * @note   Only support the following clock mode: HAL_SYSTEM_CLKMODE_LOSC and HAL_SYSTEM_CLKMODE_RC_1M8.
 *
 * @param  ClockMode: This parameter can be one of the following values from @ref HAL_SYSTEM_ClockModeTypeDef:
 *         @arg @ref HAL_SYSTEM_CLKMODE_LOSC
 *         @arg @ref HAL_SYSTEM_CLKMODE_RC_1M8
 * @retval HAL_StatusTypeDef HAL Status
 */
HAL_StatusTypeDef HAL_SYSTEM_FastClockModeConfig(HAL_SYSTEM_ClockModeTypeDef ClockMode)
{
#ifdef CORE0
    uint32_t iap_addr;

    /* If ClockMode is not 32K or auto_standby not 0, close auto_standby */
    if ((ClockMode != HAL_SYSTEM_CLKMODE_LOSC) && ((FLASH->CTRL & 0x1U) == 0x1U)) {
        FLASH->CTRL &= ~(1 << 0);
    }

    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_ERROR;
    }
    SYSTEM_ApiTypeDef *Iap_Func = (SYSTEM_ApiTypeDef *)iap_addr;
    pSYSTEM_FastClockModeConfig func_ptr = (pSYSTEM_FastClockModeConfig)((uint32_t)Iap_Func->SYSTEM_FastClockModeConfig + ROM_BASE_ADDR);
    func_ptr(ClockMode);

    /* If ClockMode is 32K and return pass, open auto_standby */
    if (ClockMode == HAL_SYSTEM_CLKMODE_LOSC) {
        FLASH->CTRL |= 1 << 0;
    }
    return HAL_OK;
#endif /* CORE0 */
#ifdef CORE1
    uint32_t api_id = SYSTEM_FASTCLOCKMODECONFIG_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)ClockMode;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 1, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_StatusTypeDef)result;
    }

    return (HAL_StatusTypeDef)api_params[1];
#endif /* CORE1 */
}

/**
 * @brief  Configure core divison of system main clock.
 *
 * @param  CoreDivison:   Specify the core divison.
 *                        This parameter can be a value of @ref HAL_SYSTEM_CoreDivTypeDef
 * @retval HAL_StatusTypeDef HAL Status
 */
HAL_StatusTypeDef HAL_SYSTEM_SetCoreDivision(HAL_SYSTEM_CoreDivTypeDef CoreDivison)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_ERROR;
    }
    SYSTEM_ApiTypeDef *Iap_Func = (SYSTEM_ApiTypeDef *)iap_addr;
    pSYSTEM_SetCoreDivision func_ptr = (pSYSTEM_SetCoreDivision)((uint32_t)Iap_Func->SYSTEM_SetCoreDivision + ROM_BASE_ADDR);
    return func_ptr(CoreDivison);
#endif /* CORE0 */
#ifdef CORE1
    uint32_t api_id = SYSTEM_SETCOREDIVISION_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)CoreDivison;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 1, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_StatusTypeDef)result;
    }

    return (HAL_StatusTypeDef)api_params[1];
#endif /* CORE1 */
}

/**
 * @brief  Control the PLL clock on/off and configure the frequency of PLL clock.
 * @note   This function should not be used when the current system clock is PLL.
 *
 * @param  PLLFrequency:  Specify the PLL clock Frequency to be configed.
 *                        This parameter can be a value of @ref HAL_SYSTEM_ClockModeTypeDef
 * @param  PLLState: specify the PLL clock state @ref HAL_SYSTEM_PLLSwitchTypeDef, this parameter can be one of the following values:
 *         @arg @ref HAL_SYSTEM_PLL_OFF
 *         @arg @ref HAL_SYSTEM_PLL_ON
 * @retval HAL_StatusTypeDef HAL Status
 */
HAL_StatusTypeDef HAL_SYSTEM_PLLConfig(HAL_SYSTEM_ClockModeTypeDef PLLFrequency, HAL_SYSTEM_PLLSwitchTypeDef PLLState)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_ERROR;
    }
    SYSTEM_ApiTypeDef *Iap_Func = (SYSTEM_ApiTypeDef *)iap_addr;
    pSYSTEM_PLLOutputFreqConfig func_ptr = (pSYSTEM_PLLOutputFreqConfig)((uint32_t)Iap_Func->SYSTEM_PLLOutputFreqConfig + ROM_BASE_ADDR);
    return func_ptr(PLLFrequency, PLLState);
#endif /* CORE0 */
#ifdef CORE1
    uint32_t api_id = SYSTEM_PLLCONFIG_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)PLLFrequency;
    api_params[2] = (uint32_t)PLLState;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 2, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_StatusTypeDef)result;
    }

    return (HAL_StatusTypeDef)api_params[1];
#endif /* CORE1 */
}

/**
 * @brief  Returns the SYSCLK frequency, if 0 get will be incorrect.
 * @note   The system frequency computed by this function in the chip.
 *         It is calculated based on the SYSTEM.SYSFREQ_STA.
 *
 * @note   This function can be used by the user application to compute the
 *         baud-rate for the communication peripherals or configure other parameters.
 *
 * @note   Each time SYSCLK changes, this function must be called to update the
 *         right SYSCLK value. Otherwise, any configuration based on this function will be incorrect.
 *
 * @param  SysClock Specify the which system clock select @ref HAL_SYSTEM_SysClockTypeDef, this parameter can be one of the following values:
 *         @arg @ref HAL_SYSTEM_CLK_CORE0
 *         @arg @ref HAL_SYSTEM_CLK_CORE1
 *         @arg @ref HAL_SYSTEM_CLK_FLASH
 * @retval SYSCLK frequency in Hz.
 */
uint32_t HAL_SYSTEM_GetSysClockFreq(HAL_SYSTEM_SysClockTypeDef SysClock)
{
    uint32_t sysclockfreq;
#ifdef CORE0
    /* Get SYSCLK -------------------------------------------------------*/
    switch (SysClock)
    {
        case HAL_SYSTEM_CLK_CORE0:  /* get used as core0's clock frequency */
        {
            sysclockfreq = __LL_SYSTEM_CORE0_CALC_CORE0_CLK_FREQ(LL_SYSTEM_CORE0_GetCore0SystemClockFreq());
            break;
        }
        case HAL_SYSTEM_CLK_CORE1:  /* get used as core1's clock frequency */
        {
            sysclockfreq = __LL_SYSTEM_CORE0_CALC_CORE1_CLK_FREQ(LL_SYSTEM_CORE0_GetCore1SystemClockFreq());
            break;
        }
        case HAL_SYSTEM_CLK_FLASH:  /* get used as flash's clock frequency */
        {
            sysclockfreq = __LL_SYSTEM_CORE0_CALC_FLASH_CLK_FREQ(LL_SYSTEM_CORE1_GetFlashSystemClockFreq());
            break;
        }
        default:  /* return 0 default */
        {
            sysclockfreq = 0U;
            break;
        }
    }
#endif /* CORE0 */
#ifdef CORE1
    /* Get SYSCLK -------------------------------------------------------*/
    switch (SysClock)
    {
        case HAL_SYSTEM_CLK_CORE0:  /* get used as core0's clock frequency */
        {
            sysclockfreq = __LL_SYSTEM_CORE1_CALC_CORE0_CLK_FREQ(LL_SYSTEM_CORE1_GetCore0SystemClockFreq());
            break;
        }
        case HAL_SYSTEM_CLK_CORE1:  /* get used as core1's clock frequency */
        {
            sysclockfreq = __LL_SYSTEM_CORE1_CALC_CORE1_CLK_FREQ(LL_SYSTEM_CORE1_GetCore1SystemClockFreq());
            break;
        }
        case HAL_SYSTEM_CLK_FLASH:  /* get used as flash's clock frequency */
        {
            sysclockfreq = __LL_SYSTEM_CORE1_CALC_FLASH_CLK_FREQ(LL_SYSTEM_CORE1_GetFlashSystemClockFreq());
            break;
        }
        default:  /* return 0 default */
        {
            sysclockfreq = 0U;
            break;
        }
    }
#endif /* CORE1 */

    return sysclockfreq;
}

/**
 * @brief This function handles the core0 SYSTEM all interrupt request.
 * @note  This API should be called under the CORE0_SYSREG_HANDLER().
 * @retval None
 */
void HAL_SYSTEM_CORE0_IRQHandler(void)
{
    /* Check SYSTEM_CAL_IF_OV flag  */
    if (__HAL_SYSTEM_CAL_GET_IT(SYSTEM_CAL_IF_OV)) {
        /* SYSTEM_CAL_IF_OV interrupt user callback */
        HAL_SYSTEM_CalibrationOVCallback();

        /* Clear SYSTEM_CAL_IF_OV pending bit */
        __HAL_SYSTEM_CAL_CLEAR_IT(SYSTEM_CAL_IF_OV);
    }
    /* Check SYSTEM_CAL_IF_DONE flag  */
    if (__HAL_SYSTEM_CAL_GET_IT(SYSTEM_CAL_IF_DONE)) {
        /* SYSTEM_CAL_IF_DONE interrupt user callback */
        HAL_SYSTEM_CalibrationDONECallback();

        /* Clear SYSTEM_CAL_IF_DONE pending bit */
        __HAL_SYSTEM_CAL_CLEAR_IT(SYSTEM_CAL_IF_DONE);
    }
}

/**
 * @brief This function handles the core1 SYSTEM all interrupt request.
 * @note  This API should be called under the CORE1_SYSREG_HANDLER().
 * @retval None
 */
void HAL_SYSTEM_CORE1_IRQHandler(void)
{
//   /* Check RCC CSSF flag  */
//   if (__HAL_RCC_GET_IT(RCC_IT_CSS))
//   {
//     /* RCC Clock Security System interrupt user callback */
//     HAL_RCC_CSSCallback();

//     /* Clear RCC CSS pending bit */
//     __HAL_RCC_CLEAR_IT(RCC_IT_CSS);
//   }
}

/**
 * @brief  This function handles core1 reset event interrupt request.
 * @note   This API should be called under the CORE1_RST_HANDLER().
 * @retval None
 */
void HAL_SYSTEM_CORE1_RESET_IRQHandler(void)
{
    /* Check CORE1_RST_PENDING flag */
    if (LL_SYSTEM_CORE1_IsActiveFlag_CORE1RST()) {
        /* CORE1 reset interrupt user callback */
        HAL_SYSTEM_Core1ResetCallback();

        /* Clear CORE1_RST_PENDING pending bit */
        LL_SYSTEM_CORE1_UnlockSystemPSW();
        LL_SYSTEM_CORE1_ClearFlag_CORE1RST();
        LL_SYSTEM_CORE1_LockSystemPSW();
    }
}

/**
 * @brief  SYSTEM_CAL_IF_OV interrupt callback
 * @retval none
 */
__WEAK void HAL_SYSTEM_CalibrationOVCallback(void)
{
    /* NOTE : This function Should not be modified, when the callback is needed,
        the HAL_SYSTEM_CalibrationOVCallback could be implemented in the user file
    */
}

/**
 * @brief  SYSTEM_CAL_IF_DONE interrupt callback
 * @retval none
 */
__WEAK void HAL_SYSTEM_CalibrationDONECallback(void)
{
    /* NOTE : This function Should not be modified, when the callback is needed,
        the HAL_SYSTEM_CalibrationDONECallback could be implemented in the user file
    */
}

__WEAK void HAL_SYSTEM_Core1ResetCallback(void)
{
    /* NOTE : This function Should not be modified, when the callback is needed,
        the HAL_SYSTEM_Core1ResetCallback could be implemented in the user file
    */
}
/**
 * @}
 */

/** @defgroup SYSTEM_Exported_Functions_Group2 System Option Config Check functions
 *  @brief     System Option Config Check functions
 *
@verbatim
@endverbatim
 * @{
 */

/**
 * @brief  Check system configuration parameters.
 *
 * @param  None
 * @retval HAL_StatusTypeDef HAL Status
 */
HAL_StatusTypeDef HAL_SYSTEM_OptionConfigCheck(void)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_ERROR;
    }
    SYSTEM_ApiTypeDef *Iap_Func = (SYSTEM_ApiTypeDef *)iap_addr;
    pSYSTEM_OptionConfigCheck func_ptr = (pSYSTEM_OptionConfigCheck)((uint32_t)Iap_Func->SYSTEM_OptionConfigCheck + ROM_BASE_ADDR);
    return func_ptr();
#endif /* CORE0 */
#ifdef CORE1
    uint32_t api_id = SYSTEM_OPTIONCONFIGCHECK_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 0, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_StatusTypeDef)result;
    }

    return (HAL_StatusTypeDef)api_params[1];
#endif /* CORE1 */
}

/**
 * @}
 */

/** @defgroup SYSTEM_Exported_Functions_Group3 RTC Dota management functions
 *  @brief     RTC Dota operation functions
 *
@verbatim
================================================================================
                    ##### RTC Dota management functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) HAL_SYSTEM_GetRTCDota                  : RTC Dota Set
        (+) HAL_SYSTEM_SetRTCDota                  : RTC Dota Get
@endverbatim
 * @{
 */

/**
 * @brief  Set RTC Dota value.
 *
 * @param  RTCDota: specify the RTC Dota Value.
 * @retval HAL_StatusTypeDef HAL Status.
 */
HAL_StatusTypeDef HAL_SYSTEM_SetRTCDota(uint16_t RTCDota)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_ERROR;
    }
    SYSTEM_ApiTypeDef *Iap_Func = (SYSTEM_ApiTypeDef *)iap_addr;
    pSYSTEM_SetRTCDota func_ptr = (pSYSTEM_SetRTCDota)((uint32_t)Iap_Func->SYSTEM_SetRTCDota + ROM_BASE_ADDR);
    return func_ptr(RTCDota);
#endif /* CORE0 */
#ifdef CORE1
    uint32_t api_id = SYSTEM_SETRTCDOTA_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)RTCDota;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 1, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_StatusTypeDef)result;
    }

    return (HAL_StatusTypeDef)api_params[1];
#endif /* CORE1 */
}

/**
 * @brief  Get the RTC Dota value.
 *
 * @param  None
 * @retval RTC Dota value.
 */
uint16_t HAL_SYSTEM_GetRTCDota(void)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_ERROR;
    }
    SYSTEM_ApiTypeDef *Iap_Func = (SYSTEM_ApiTypeDef *)iap_addr;
    pSYSTEM_GetRTCDota func_ptr = (pSYSTEM_GetRTCDota)((uint32_t)Iap_Func->SYSTEM_GetRTCDota + ROM_BASE_ADDR);
    return func_ptr();
#endif /* CORE0 */
#ifdef CORE1
    uint32_t api_id = SYSTEM_GETRTCDOTA_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 0, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (uint16_t)result;
    }

    return (uint16_t)api_params[1];
#endif /* CORE1 */
}

/**
 * @}
 */

/**
 * @}
 */

#endif /* HAL_SYSTEM_MODULE_ENABLED && HAL_RPC_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
