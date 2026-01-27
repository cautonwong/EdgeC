/**
 ******************************************************************************
 * @file   rn_rv_ll_dac.c
 * @author Renergy Software Team
 * @brief  DAC LL module driver.
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
#include "rn_rv_ll_dac.h"
#include "rn_rv_ll_system_core0.h"
#include "rn_rv_ll_system_core1.h"

#ifdef USE_FULL_ASSERT
#include "rn_rv_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup RN_LL_Driver
 * @{
 */

/** @addtogroup DAC_LL
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @addtogroup DAC_LL_Exported_Functions
 * @{
 */

/** @addtogroup DAC_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize registers of DAC instances to default reset values.
 * @param  DACx DAC instance
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: DAC registers are de-initialized
 *          - ERROR: not applicable
 */
ErrorStatus LL_DAC_DeInit(DAC_TypeDef *DACx)
{
   /* Check the parameters */
   assert_param(IS_DAC_ALL_INSTANCE(DACx));

   if (DACx == DAC_CORE0) {
      /* Force reset of DAC clock (core clock) */
      LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_DAC);
      /* Release reset of DAC clock (core clock) */
      LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_DAC);
   } else if (DACx == DAC_CORE1) {
      LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_DAC);
      /* Release reset of DAC clock (core clock) */
      LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_DAC);
   } else {
      return ERROR;
   }

   return SUCCESS;
}
/**
 * @brief  Initialize DAC.
 * @param  DACx DAC instance
 * @param  DAC_InitStruct Pointer to LL_DAC_InitTypeDef
 * @retval SUCCESS or ERROR
 */
ErrorStatus LL_DAC_Init(DAC_TypeDef *DACx, LL_DAC_InitTypeDef *DAC_InitStruct)
{
    LL_DAC_SetDataAlignment(DACx, DAC_InitStruct->DataAlignment);
    LL_DAC_SetClockDivision(DACx, DAC_InitStruct->ClockDivision);
    LL_DAC_SetOutputChannel(DACx, DAC_InitStruct->OutputChannel);
    LL_DAC_SetTriggerMode  (DACx, DAC_InitStruct->TriggerMode);
    return SUCCESS;
}
/**
 * @brief  Set each @ref LL_DAC_StructInit field to default value.
 * @param  DAC_InitStruct Pointer to a @ref LL_DAC_StructInit structure
 *                            whose fields will be set to default values.
 * @retval None
 */
void LL_DAC_StructInit(LL_DAC_InitTypeDef *DAC_InitStruct)
{
    DAC_InitStruct->DataAlignment     = LL_DAC_DATA_ALIGN_RIGHT;
    DAC_InitStruct->ClockDivision     = 0x0U;
    DAC_InitStruct->OutputChannel     = LL_DAC_OUTPUT_CHANNEL_ALL_CLOSED;
    DAC_InitStruct->TriggerMode       = LL_DAC_TRIGGER_CONTINUOUS;
}
/**
 * @}
 */ /* end of group DAC_LL_EF_Init */

/**
 * @}
 */ /* end of group DAC_LL_Exported_Functions */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */ /* end of group DAC_LL */
/**
 * @}
 */ /* end of group RN_LL_Driver */