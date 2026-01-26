/**
 ******************************************************************************
 * @file   rn_rv_ll_lvdcmp.c
 * @author Renergy Software Team
 * @brief  LVDCMP LL module driver.
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
#include "rn_rv_ll_lvdcmp.h"
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

/** @addtogroup LVDCMP_LL
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @addtogroup LVDCMP_LL_Exported_Functions
 * @{
 */

/** @addtogroup LVDCMP_LL_EF_Init
 * @{
 */
/**
 * @brief  De-initialize registers of LVDCMP instances to default reset values.
 * @param  LVDCMPx LVDCMP instance
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: LVDCMP registers are de-initialized
 *          - ERROR: not applicable
 */
ErrorStatus LL_LVDCMP_DeInit(LVDCMP_TypeDef *LVDCMPx)
{
   /* Check the parameters */
   assert_param(IS_LVDCMP_ALL_INSTANCE(LVDCMPx));

   if (LVDCMPx == LVDCMP_CORE0) {
      /* Force reset of LVDCMP clock (core clock) */
      LL_SYSTEM_CORE0_MOD1_ForceReset(LL_SYSTEM_CORE0_MOD1_PERIPH_LVDCMP);
      /* Release reset of LVDCMP clock (core clock) */
      LL_SYSTEM_CORE0_MOD1_ForceReset(LL_SYSTEM_CORE0_MOD1_PERIPH_LVDCMP);
   } else if (LVDCMPx == LVDCMP_CORE1) {
      LL_SYSTEM_CORE1_MOD1_ForceReset(LL_SYSTEM_CORE1_MOD1_PERIPH_LVDCMP);
      /* Release reset of LVDCMP clock (core clock) */
      LL_SYSTEM_CORE1_MOD1_ReleaseReset(LL_SYSTEM_CORE1_MOD1_PERIPH_LVDCMP);
   } else {
      return ERROR;
   }

   return SUCCESS;
}
/**
 * @brief  Initialize the LVD configuration.
 * @note   Must be called when both sequences are stopped.
 * @param  LVDCMPx LVDCMP instance
 * @param  LVD_InitStruct Pointer to LL_LVDInit_TypeDef
 * @retval SUCCESS or ERROR
 */
ErrorStatus LL_LVDCMP_LVDInit(LVDCMP_TypeDef *LVDCMPx,  LL_LVDInit_TypeDef  *LVD_InitStruct)
{
   LL_LVDCMP_SetLVDVoltageThreshold(LVDCMPx, LVD_InitStruct->LVDVoltageThreshold);
   LL_LVDCMP_SetLVDFilter(LVDCMPx, LVD_InitStruct->LVDFilter);
   return SUCCESS;
}

/**
 * @brief  Initialize the CMP1 configuration.
 * @note   Must be called when both sequences are stopped.
 * @param  LVDCMPx LVDCMP instance
 * @param  CMP1_InitStruct Pointer to LL_CMP1Init_TypeDef
 * @retval SUCCESS or ERROR
 */
ErrorStatus LL_LVDCMP_CMP1Init(LVDCMP_TypeDef *LVDCMPx,  LL_CMP1Init_TypeDef  *CMP1_InitStruct)
{
   LL_LVDCMP_SetCMP1ResistorMode(LVDCMPx, CMP1_InitStruct->CMP1ResistorMode);
   LL_LVDCMP_SetCMP1Filter(LVDCMPx, CMP1_InitStruct->CMP1Filter);
   if (CMP1_InitStruct->CMP1Hysteresis){
      LL_LVDCMP_EnableCMP1Hysteresis(LVDCMPx);
   } else{
      LL_LVDCMP_DisableCMP1Hysteresis(LVDCMPx);
   }
   return SUCCESS;
}
/**
 * @brief  Initialize the CMP2 configuration.
 * @note   Must be called when both sequences are stopped.
 * @param  LVDCMPx LVDCMP instance
 * @param  CMP2_InitStruct Pointer to LL_CMP2Init_TypeDef
 * @retval SUCCESS or ERROR
 */
ErrorStatus LL_LVDCMP_CMP2Init(LVDCMP_TypeDef *LVDCMPx,  LL_CMP2Init_TypeDef  *CMP2_InitStruct)
{
   LL_LVDCMP_SetCMP2ResistorMode(LVDCMPx, CMP2_InitStruct->CMP2ResistorMode);
   LL_LVDCMP_SetCMP2Filter(LVDCMPx, CMP2_InitStruct->CMP2Filter);
   if (CMP2_InitStruct->CMP2Hysteresis){
      LL_LVDCMP_EnableCMP2Hysteresis(LVDCMPx);
   } else{
      LL_LVDCMP_DisableCMP2Hysteresis(LVDCMPx);
   }
   return SUCCESS;
}

/**
 * @brief  Set each @ref LL_LVDInit_TypeDef field to default value.
 * @param  LVD_InitStruct Pointer to a @ref LL_LVDInit_TypeDef structure
 *                            whose fields will be set to default values.
 * @retval None
 */
void LL_LVDCMP_LVDStructInit (LL_LVDInit_TypeDef  *LVD_InitStruct)
{
   LVD_InitStruct->LVDVoltageThreshold = LL_LVDCMP_LVDTHRESHOLD_1P9;
   LVD_InitStruct->LVDFilter = 0x64U;
}
/**
 * @brief  Set each @ref LL_CMP1Init_TypeDef field to default value.
 * @param  CMP1_InitStruct Pointer to a @ref LL_CMP1Init_TypeDef structure
 *                            whose fields will be set to default values.
 * @retval None
 */
void LL_LVDCMP_CMP1StructInit (LL_CMP1Init_TypeDef  *CMP1_InitStruct)
{
   CMP1_InitStruct->CMP1ResistorMode = LL_LVDCMP_CMP1_RESISTOR_ON ;
   CMP1_InitStruct->CMP1Hysteresis = LL_LVDCMP_FUNCTION_DISABLE;
   CMP1_InitStruct->CMP1Filter = 0;
}
/**
 * @brief  Set each @ref LL_CMP2Init_TypeDef field to default value.
 * @param  CMP2_InitStruct Pointer to a @ref LL_CMP2Init_TypeDef structure
 *                            whose fields will be set to default values.
 * @retval None
 */
void LL_LVDCMP_CMP2StructInit (LL_CMP2Init_TypeDef  *CMP2_InitStruct)
{
   CMP2_InitStruct->CMP2ResistorMode = LL_LVDCMP_CMP2_RESISTOR_ON ;
   CMP2_InitStruct->CMP2Hysteresis = LL_LVDCMP_FUNCTION_DISABLE;
   CMP2_InitStruct->CMP2Filter = 0;
}
/**
 * @}
 */ /* end of group LVDCMP_LL_EF_Init */

/**
 * @}
 */ /* end of group LVDCMP_LL_Exported_Functions */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */ /* end of group LVDCMP_LL */
/**
 * @}
 */ /* end of group RN_LL_Driver */
