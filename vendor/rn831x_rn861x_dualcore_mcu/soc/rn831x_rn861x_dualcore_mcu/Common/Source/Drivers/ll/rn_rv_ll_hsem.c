/**
 *****************************************************************************
 * @file   rn_rv_ll_hsem.c
 * @author Renergy Software Team
 * @brief  HSEM LL module driver.
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
#include "rn_rv_ll_hsem.h"
#include "rn_rv_ll_system_core1.h"

#ifdef  USE_FULL_ASSERT
#include "rn_rv_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (HSEM)

/** @addtogroup HSEM_LL HSEM
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup HSEM_LL_Private_Macros
 * @{
 */

/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup HSEM_LL_Exported_Functions
 * @{
 */

/** @addtogroup HSEM_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize HSEM registers (Registers restored to their default values).
 * @param  HSEMx HSEM Instance
 * @retval An LL_ErrStatus enumeration value:
 *          - LL_SUCCESS: HSEM registers are de-initialized
 *          - LL_ERROR:   HSEM registers are not de-initialized
 */
ErrorStatus LL_HSEM_DeInit(HSEM_TypeDef *HSEMx)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_HSEM_ALL_INSTANCE(HSEMx));

    if (HSEMx == HSEM) {
        /* Force reset of HSEM clock */
        LL_SYSTEM_CORE1_PMOD0_ForceReset(LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_HSEM);

        /* Release reset of HSEM clock */
        LL_SYSTEM_CORE1_PMOD0_ReleaseReset(LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_HSEM);
    } else {
        status = ERROR;
    }

    return status;
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

#endif /* HSEM */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
