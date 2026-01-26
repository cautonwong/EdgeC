/**
 ******************************************************************************
 * @file   rn_rv_ll_madc.c
 * @author Renergy Software Team
 * @brief  MADC LL module driver.
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
#include "rn_rv_ll_madc.h"

#ifdef USE_FULL_ASSERT
#include "rn_rv_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup RN_LL_Driver
 * @{
 */

/** @addtogroup MADC_LL
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup MADC_LL_Private_Macros
 * @{
 */

/**
 * @}
 */ /* end of group MADC_LL_Private_Macros */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @addtogroup MADC_LL_Exported_Functions
 * @{
 */

/** @addtogroup MADC_LL_EF_Init
 * @{
 */
/**
 * @brief  De-initialize registers of MADC instances to default reset values.
 * @param  MADCx MADC instance
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: MADC registers are de-initialized
 *          - ERROR: not applicable
 */
ErrorStatus LL_MADC_DeInit(MADC_TypeDef *MADCx)
{
   /* Check the parameters */
   assert_param(IS_MADC_ALL_INSTANCE(MADCx));
   /* MADC not reset PERIPH */
   if (MADCx == MADC_CORE0) {
   } else if (MADCx == MADC_CORE1) {
   } else {
      return ERROR;
   }
   return SUCCESS;
}
/**
 * @brief  Initialize MADC
 * @param  MADCx MADC instance
 * @param  MADC_InitStruct Pointer to LL_MADC_InitTypeDef
 * @retval SUCCESS or ERROR
 */
ErrorStatus LL_MADC_Init(MADC_TypeDef *MADCx, LL_MADC_InitTypeDef *MADC_InitStruct)
{
    LL_MADC_SetChannel(MADCx, MADC_InitStruct->Channel);
    LL_MADC_SetPGASelection(MADCx, MADC_InitStruct->PGASelection);
    LL_MADC_SetConversionTime(MADCx, MADC_InitStruct->ConversionTime);
    LL_MADC_SetAccumulationNumber(MADCx, MADC_InitStruct->AccumulationNumber);
    return SUCCESS;
}
/**
 * @brief  Set each @ref LL_MADC_StructInit field to default value.
 * @param  MADC_InitStruct Pointer to a @ref LL_MADC_StructInit structure
 *                            whose fields will be set to default values.
 * @retval None
 */
void LL_MADC_StructInit(LL_MADC_InitTypeDef *MADC_InitStruct)
{
    MADC_InitStruct->Channel            = LL_MADC_CHANNEL_TEMP;
    MADC_InitStruct->PGASelection       = LL_MADC_PGA_1P0;
    MADC_InitStruct->ConversionTime     = 0x29U;
    MADC_InitStruct->AccumulationNumber = 0U;
}

/**
 * @}
 */ /* end of group MADC_LL_EF_Init */

/**
 * @}
 */ /* end of group MADC_LL_Exported_Functions */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */ /* end of group MADC_LL */
/**
 * @}
 */ /* end of group RN_LL_Driver */