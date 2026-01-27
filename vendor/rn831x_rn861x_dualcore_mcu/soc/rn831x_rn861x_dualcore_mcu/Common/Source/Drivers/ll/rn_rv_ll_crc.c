/**
 ******************************************************************************
 * @file   rn_rv_ll_crc.c
 * @author Renergy Software Team
 * @brief  CRC LL module driver
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

#if defined(USE_FULL_LL_DRIVER)

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_ll_crc.h"
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

#if defined (CRC0) || defined (CRC1)

/** @defgroup CRC_LL CRC
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup CRC_LL_Exported_Functions
 * @{
 */

/** @addtogroup CRC_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize the CRC registers to their default reset values.
 *
 * @param  CRCx CRC instance
 * @retval ErrorStatus
 */
ErrorStatus LL_CRC_DeInit(CRC_TypeDef *CRCx)
{
    ErrorStatus status = ERROR;

    /* Check the parameters */
    assert_param(IS_CRC_ALL_INSTANCE(CRCx));

    if (LL_CRC_IsCalculationDone(CRCx) == 1U) {
        /* Reset CRC register. */
        if (CRCx == CRC1) {
            LL_SYSTEM_CORE0_PMOD0_ForceReset(LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_CRC1);
            LL_SYSTEM_CORE0_PMOD0_ReleaseReset(LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_CRC1);
        } else if (CRCx == CRC0_CORE0) {
            LL_SYSTEM_CORE0_MOD1_ForceReset(LL_SYSTEM_CORE0_MOD1_PERIPH_CRC0);
            LL_SYSTEM_CORE0_MOD1_ReleaseReset(LL_SYSTEM_CORE0_MOD1_PERIPH_CRC0);
        } else if (CRCx == CRC0_CORE1) {
            LL_SYSTEM_CORE1_MOD1_ForceReset(LL_SYSTEM_CORE1_MOD1_PERIPH_CRC0);
            LL_SYSTEM_CORE1_MOD1_ReleaseReset(LL_SYSTEM_CORE1_MOD1_PERIPH_CRC0);
        }

        status = SUCCESS;
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

#endif /* CRC0 || CRC1 */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
