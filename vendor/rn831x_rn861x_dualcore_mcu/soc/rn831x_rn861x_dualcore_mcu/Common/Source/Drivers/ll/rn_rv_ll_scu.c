/**
 *****************************************************************************
 * @file   rn_rv_ll_scu.c
 * @author Renergy Software Team
 * @brief  SCU LL module driver.
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
#include "rn_rv_ll_scu.h"

#ifdef  USE_FULL_ASSERT
#include "rn_rv_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (SCU)

/** @addtogroup SCU_LL SCU
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup SCU_LL_Private_Macros
 * @{
 */

/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup SCU_LL_Exported_Functions
 * @{
 */

/** @addtogroup SCU_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize SCU registers (Registers restored to their default values).
 * @param  SCUx SCU Instance
 * @retval An LL_ErrStatus enumeration value:
 *          - LL_SUCCESS: SCU registers are de-initialized
 *          - LL_ERROR:   SCU registers are not de-initialized
 */
ErrorStatus LL_SCU_DeInit(SCU_TypeDef *SCUx)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_SCU_ALL_INSTANCE(SCUx));
    if (SCUx == SCU_CORE0) {
        /* Unlock SCU PSW register */
        LL_SCU_UnlockWriteKey();

        /* Reset SCU Peripheral Security Configuration Register 0 */
        LL_SCU_WriteReg(SCUx, PERIPH_SEC_CFG0, 0xFFFFFFFFU);

        /* Reset SCU Peripheral Security Configuration Register 1 */
        LL_SCU_WriteReg(SCUx, PERIPH_SEC_CFG1, 0xFFFFFFFFU);

        /* Reset SCU Peripheral Security Configuration Register 2 */
        LL_SCU_WriteReg(SCUx, PERIPH_SEC_CFG2, 0xFFFFFFFFU);

        /* Reset SCU GPIOA Security Configuration Register */
        LL_SCU_WriteReg(SCUx, GPIOA_SEC_CFG, 0xFFFFU);

        /* Reset SCU GPIOB Security Configuration Register */
        LL_SCU_WriteReg(SCUx, GPIOB_SEC_CFG, 0xFFFFU);

        /* Reset SCU GPIOC Security Configuration Register */
        LL_SCU_WriteReg(SCUx, GPIOC_SEC_CFG, 0xFFFFU);

        /* Reset SCU GPIOD Security Configuration Register */
        LL_SCU_WriteReg(SCUx, GPIOD_SEC_CFG, 0xFFFFU);

        /* Reset SCU GPIOE Security Configuration Register */
        LL_SCU_WriteReg(SCUx, GPIOE_SEC_CFG, 0xFFFFU);

        /* Reset SCU GPIOF Security Configuration Register */
        LL_SCU_WriteReg(SCUx, GPIOF_SEC_CFG, 0xFFFFU);

        /* Reset SCU GPIOG Security Configuration Register */
        LL_SCU_WriteReg(SCUx, GPIOG_SEC_CFG, 0xFFFFU);

        /* Reset SCU GPIOH Security Configuration Register */
        LL_SCU_WriteReg(SCUx, GPIOH_SEC_CFG, 0x3U);

        /* Reset SCU Peripheral Illegal Access Interrupt Enable Register 0 */
        LL_SCU_WriteReg(SCUx, PERIPH_SEC_IE0, 0x0U);

        /* Reset SCU Peripheral Illegal Access Interrupt Enable Register 1 */
        LL_SCU_WriteReg(SCUx, PERIPH_SEC_IE1, 0x0U);

        /* Reset SCU Peripheral Illegal Access Interrupt Enable Register 2 */
        LL_SCU_WriteReg(SCUx, PERIPH_SEC_IE2, 0x0U);

        /* Reset SCU Peripheral Illegal Access Interrupt Flag Register 0 */
        LL_SCU_WriteReg(SCUx, PERIPH_SEC_IF0, 0xFFFFFFFFU);

        /* Reset SCU Peripheral Illegal Access Interrupt Flag Register 1 */
        LL_SCU_WriteReg(SCUx, PERIPH_SEC_IF1, 0xFFFFFFFFU);

        /* Reset SCU Peripheral Illegal Access Interrupt Flag Register 2 */
        LL_SCU_WriteReg(SCUx, PERIPH_SEC_IF2, 0xFFFFFFFFU);

        /* Lock SCU PSW register */
        LL_SCU_LockWriteKey();
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

#endif /* SCU */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
