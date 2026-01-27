/**
 ******************************************************************************
 * @file   rn_rv_ll_rtc.c
 * @author Renergy Software Team
 * @brief  RTC LL module driver.
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
#include "rn_rv_ll_rtc.h"
#include "rn_rv_ll_system_core0.h"
#include "rn_rv_ll_system_core1.h"

#ifdef USE_FULL_ASSERT
#include "rn_rv_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif
#if defined(USE_FULL_LL_DRIVER)

/** @addtogroup RN_LL_Driver
 * @{
 */

/** @addtogroup RTC_LL
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup RTC_LL_Exported_Functions
 * @{
 */

/** @addtogroup RTC_LL_EF_Init
 * @{
 */
ErrorStatus LL_RTC_DeInit(RTC_TypeDef *RTCx)
{
    /* Check the parameters */
    assert_param(IS_RTC_ALL_INSTANCE(RTCx));

    if (RTCx == RTC_CORE0) {
       /* Force reset of RS_VDD clock (core clock) */
       LL_SYSTEM_CORE0_MOD1_ForceReset(LL_SYSTEM_CORE0_MOD1_PERIPH_RS_VDD);
       /* Release reset of RS_VDD clock (core clock) */
       LL_SYSTEM_CORE0_MOD1_ForceReset(LL_SYSTEM_CORE0_MOD1_PERIPH_RS_VDD);
    } else if (RTCx == RTC_CORE1) {
       LL_SYSTEM_CORE1_MOD1_ForceReset(LL_SYSTEM_CORE1_MOD1_PERIPH_RS_VDD);
       /* Release reset of RS_VDD clock (core clock) */
       LL_SYSTEM_CORE1_MOD1_ReleaseReset(LL_SYSTEM_CORE1_MOD1_PERIPH_RS_VDD);
    } else {
       return ERROR;
    }

    return SUCCESS;
}

/**
 * @brief  Set the RTC current time.
 * @param  RTCx RTC Instance
 * @param  RTC_Format This parameter can be one of the following values:
 *         @arg @ref LL_RTC_FORMAT_BIN
 *         @arg @ref LL_RTC_FORMAT_BCD
 * @param  RTC_TimeStruct pointer to a RTC_TimeTypeDef structure that contains
 *                        the time configuration information for the RTC.
 * @param  RTC_DateStruct pointer to a LL_RTC_DateTypeDef structure that contains
 *                        the time configuration information for the RTC.
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: RTC Time register is configured
 *          - ERROR: RTC Time register is not configured
 */
ErrorStatus LL_RTC_TIME_Init(RTC_TypeDef *RTCx,  LL_RTC_TimeTypeDef *RTC_TimeStruct, LL_RTC_DateTypeDef *RTC_DateStruct)
{
    LL_RTC_DisableConfigWriteProtection(RTCx);
    LL_RTC_DATE_Config(
        RTCx,
        RTC_DateStruct->WeekDay,
        RTC_DateStruct->Month,
        RTC_DateStruct->Day,
        RTC_DateStruct->Year
    );
    LL_RTC_TIME_Config(
        RTCx,
        RTC_TimeStruct->Hours,
        RTC_TimeStruct->Minutes,
        RTC_TimeStruct->Seconds
    );
    LL_RTC_EnableConfigWriteProtection(RTCx);
    return SUCCESS;
}

/**
 * @brief  Set each @ref LL_RTC_DateTypeDef field to  (date = Monday, January 01 xx00)
 * @param  RTC_DateStruct pointer to a @ref LL_RTC_DateTypeDef structure which will be initialized.
 * @retval None
 */
void LL_RTC_DATE_StructInit(LL_RTC_DateTypeDef *RTC_DateStruct)
{
    /* Monday, January 01 xx00 */
    RTC_DateStruct->WeekDay = LL_RTC_WEEKDAY_MONDAY;
    RTC_DateStruct->Day     = 1U;
    RTC_DateStruct->Month   = LL_RTC_MONTH_JANUARY;
    RTC_DateStruct->Year    = 0U;
}

/**
 * @brief  Set each @ref LL_RTC_TimeTypeDef field to  (Time = 00h:00min:00sec).
 * @param  RTC_TimeStruct pointer to a @ref LL_RTC_TimeTypeDef structure which will be initialized.
 * @retval None
 */
void LL_RTC_TIME_StructInit(LL_RTC_TimeTypeDef *RTC_TimeStruct)
{
    RTC_TimeStruct->Hours      = 0U;
    RTC_TimeStruct->Minutes    = 0U;
    RTC_TimeStruct->Seconds    = 0U;
}

/**
 * @}
 */ /* end of group RTC_LL_EF_Init */

/**
 * @}
 */ /* end of group RTC_LL_Exported_Functions */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */ /* end of group RTC_LL */
/**
 * @}
 */ /* end of group RN_LL_Driver */
