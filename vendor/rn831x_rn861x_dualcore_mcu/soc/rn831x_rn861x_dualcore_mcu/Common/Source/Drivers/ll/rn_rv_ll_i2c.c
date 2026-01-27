/**
 *****************************************************************************
 * @file   rn_rv_ll_i2c.c
 * @author Renergy Software Team
 * @brief  I2C LL module driver.
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
#include "rn_rv_ll_i2c.h"
#include "rn_rv_ll_system_core0.h"
#include "rn_rv_ll_system_core1.h"

#ifdef  USE_FULL_ASSERT
#include "rn_rv_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (I2C0) || defined (I2C1)

/** @addtogroup I2C_LL I2C
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup I2C_LL_Private_Macros
 * @{
 */

#define IS_LL_I2C_PERIPHERAL_MODE(__VALUE__)    (((__VALUE__) == LL_I2C_MODE_MASTER)          || \
                                                 ((__VALUE__) == LL_I2C_MODE_SLAVE))

#define IS_LL_I2C_SPEED_MODE(__VALUE__)        (((__VALUE__) == LL_I2C_SPEED_STANDARD_MODE) ||\
                                                 ((__VALUE__) == LL_I2C_SPEED_FAST_MODE))

#define IS_LL_I2C_ADDRESS(__VALUE__)            ((__VALUE__) <= 0x0000007FU)

#define IS_LL_I2C_TYPE_ACKNOWLEDGE(__VALUE__)   (((__VALUE__) == LL_I2C_ACK) || \
                                                 ((__VALUE__) == LL_I2C_NACK))
/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup I2C_LL_Exported_Functions
 * @{
 */

/** @addtogroup I2C_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize I2C registers (Registers restored to their default values).
 * @param  I2Cx I2C Instance
 * @retval An ErrStatus enumeration value:
 *          - SUCCESS: I2C registers are de-initialized
 *          - ERROR:   I2C registers are not de-initialized
 */
ErrorStatus LL_I2C_DeInit(I2C_TypeDef *I2Cx)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_I2C_ALL_INSTANCE(I2Cx));

    if (I2Cx == I2C0_CORE0) {
        /* Force reset of I2C clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_I2C0);

        /* Release reset of I2C clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_I2C0);
    } else if (I2Cx == I2C1_CORE0) {
        /* Force reset of I2C clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_I2C1);

        /* Release reset of I2C clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_I2C1);
    } else if (I2Cx == I2C0_CORE1) {
        /* Force reset of I2C clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_I2C0);

        /* Release reset of I2C clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_I2C0);
    } else if (I2Cx == I2C1_CORE1) {
        /* Force reset of I2C clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_I2C1);

        /* Release reset of I2C clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_I2C1);
    } else {
        status = ERROR;
    }

    return status;
}

/**
 * @brief  Initialize the I2C registers according to the specified parameters in I2C_InitStruct.
 * @param  I2Cx I2Cx Instance
 * @param  I2C_InitStruct pointer to a @ref LL_I2C_InitTypeDef structure.
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: I2C registers are initialized
 *          - ERROR:   Not applicable
 */
ErrorStatus LL_I2C_Init(I2C_TypeDef *I2Cx, LL_I2C_InitTypeDef *I2C_InitStruct)
{
    ErrorStatus status = SUCCESS;
    uint32_t system_clock = 0;

    /* Check the I2C Instance I2Cx */
    assert_param(IS_I2C_ALL_INSTANCE(I2Cx));

    /* Check the I2C parameters from I2C_InitStruct */
    assert_param(IS_LL_I2C_PERIPHERAL_MODE(I2C_InitStruct->PeripheralMode));
    assert_param(IS_LL_I2C_SPEED_MODE(I2C_InitStruct->SpeedMode));
    assert_param(IS_LL_I2C_ADDRESS(I2C_InitStruct->Address));
    assert_param(IS_LL_I2C_TYPE_ACKNOWLEDGE(I2C_InitStruct->TypeAcknowledge));

    /* Disable the selected I2Cx Peripheral */
    LL_I2C_Disable(I2Cx);

    /*---------------------------- I2Cx MODE Configuration -----------------------
    * Configure I2Cx peripheral mode with parameter :
    * - PeripheralMode: I2C_CTRL_MODE bit
    */
    LL_I2C_SetMode(I2Cx, I2C_InitStruct->PeripheralMode);

    /*---------------------------- I2Cx ACK Configuration ------------------------
    * Configure the ACKnowledge or Non ACKnowledge condition
    * after the address receive match code or next received byte with parameter :
    * - TypeAcknowledge: I2C_CTRL_ACK bit
    */
    LL_I2C_AcknowledgeNextData(I2Cx, I2C_InitStruct->TypeAcknowledge);

    /*---------------------------- I2Cx SCL Clock Speed Mode Configuration ------------
    * Configure the SCL speed :
    * - ClockSpeed: I2C_CLK_CLKDIV bits
    */
    if ((I2Cx == I2C0_CORE0) || (I2Cx == I2C1_CORE0)) {
        /* Retrieve System Clock frequencies */
        system_clock = LL_SYSTEM_CORE0_GetCore0SystemClockFreq();
        /* Set I2C Clock Speed Mode */
        if (I2C_InitStruct-> SpeedMode== LL_I2C_SPEED_STANDARD_MODE) {
            if (system_clock == LL_SYSTEM_CORE0_CORE0FREQ_1M8432) {
                /* Set I2C Clock Speed Mode in system 1.8432MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_1M8_STANDARD_MODE);
            } else if (system_clock == LL_SYSTEM_CORE0_CORE0FREQ_3M6864) {
                /* Set I2C Clock Speed Mode in system 3.6864MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_3M6_STANDARD_MODE);
            } else if (system_clock == LL_SYSTEM_CORE0_CORE0FREQ_7M3728) {
                /* Set I2C Clock Speed Mode in system 7.3728MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_7M3_STANDARD_MODE);
            } else if (system_clock == LL_SYSTEM_CORE0_CORE0FREQ_14M7456) {
                /* Set I2C Clock Speed Mode in system 14.7456MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_14M7_STANDARD_MODE);
            } else if (system_clock == LL_SYSTEM_CORE0_CORE0FREQ_29M4912) {
                /* Set I2C Clock Speed Mode in system 29.4912MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_29M4_STANDARD_MODE);
            } else {
                status = ERROR;
            }
        } else if (I2C_InitStruct->SpeedMode == LL_I2C_SPEED_FAST_MODE) {
            if (system_clock == LL_SYSTEM_CORE0_CORE0FREQ_7M3728) {
                /* Set I2C Clock Speed Mode in system 7.3728MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_7M3_FAST_MODE);
            } else if (system_clock == LL_SYSTEM_CORE0_CORE0FREQ_14M7456) {
                /* Set I2C Clock Speed Mode in system 14.7456MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_14M7_FAST_MODE);
            } else if (system_clock == LL_SYSTEM_CORE0_CORE0FREQ_29M4912) {
                /* Set I2C Clock Speed Mode in system 29.4912MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_29M4_FAST_MODE);
            } else {
                status = ERROR;
            }
        } else {
            status = ERROR;
        }
    } else if ((I2Cx == I2C0_CORE1) || (I2Cx == I2C1_CORE1)) {
        /* Retrieve System Clock frequencies */
        system_clock = LL_SYSTEM_CORE1_GetCore1SystemClockFreq();
        /* Set I2C Clock Speed Mode */
        if (I2C_InitStruct-> SpeedMode== LL_I2C_SPEED_STANDARD_MODE) {
            if (system_clock == LL_SYSTEM_CORE1_CORE1FREQ_1M8432) {
                /* Set I2C Clock Speed Mode in system 1.8432MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_1M8_STANDARD_MODE);
            } else if (system_clock == LL_SYSTEM_CORE1_CORE1FREQ_3M6864) {
                /* Set I2C Clock Speed Mode in system 3.6864MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_3M6_STANDARD_MODE);
            } else if (system_clock == LL_SYSTEM_CORE1_CORE1FREQ_7M3728) {
                /* Set I2C Clock Speed Mode in system 7.3728MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_7M3_STANDARD_MODE);
            } else if (system_clock == LL_SYSTEM_CORE1_CORE1FREQ_14M7456) {
                /* Set I2C Clock Speed Mode in system 14.7456MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_14M7_STANDARD_MODE);
            } else if (system_clock == LL_SYSTEM_CORE1_CORE1FREQ_29M4912) {
                /* Set I2C Clock Speed Mode in system 29.4912MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_29M4_STANDARD_MODE);
            } else if (system_clock == LL_SYSTEM_CORE1_CORE1FREQ_58M9824) {
                /* Set I2C Clock Speed Mode in system 58.9824MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_29M4_STANDARD_MODE);
            } else {
                status = ERROR;
            }
        } else if (I2C_InitStruct->SpeedMode == LL_I2C_SPEED_FAST_MODE) {
            if (system_clock == LL_SYSTEM_CORE1_CORE1FREQ_7M3728) {
                /* Set I2C Clock Speed Mode in system 7.3728MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_7M3_FAST_MODE);
            } else if (system_clock == LL_SYSTEM_CORE1_CORE1FREQ_14M7456) {
                /* Set I2C Clock Speed Mode in system 14.7456MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_14M7_FAST_MODE);
            } else if (system_clock == LL_SYSTEM_CORE1_CORE1FREQ_29M4912) {
                /* Set I2C Clock Speed Mode in system 29.4912MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_29M4_FAST_MODE);
            } else if (system_clock == LL_SYSTEM_CORE1_CORE1FREQ_58M9824) {
                /* Set I2C Clock Speed Mode in system 58.9824MHz */
                LL_I2C_SetClockSpeedMode(I2Cx, LL_I2C_CLOCK_SPEED_58M9_FAST_MODE);
            } else {
                status = ERROR;
            }
        } else {
            status = ERROR;
        }
    } else {
        status = ERROR;
    }

    /* Enable the selected I2Cx Peripheral */
    LL_I2C_Enable(I2Cx);

    /*---------------------------- I2Cx Address Configuration -----------------------
    * Configure I2Cx device address with parameters :
    * - OwnAddress1: I2C_ADDR_SADR bits
    */
    LL_I2C_SetAddress(I2Cx, I2C_InitStruct->Address);

    return status;
}

/**
 * @brief  Set each @ref LL_I2C_InitTypeDef field to default value.
 * @param  I2C_InitStruct Pointer to a @ref LL_I2C_InitTypeDef structure.
 * @retval None
 */
void LL_I2C_StructInit(LL_I2C_InitTypeDef *I2C_InitStruct)
{
  /* Set I2C_InitStruct fields to default values */
    I2C_InitStruct->PeripheralMode  = LL_I2C_MODE_SLAVE;
    I2C_InitStruct->SpeedMode       = LL_I2C_SPEED_STANDARD_MODE;
    I2C_InitStruct->Address         = 0x18U;
    I2C_InitStruct->TypeAcknowledge = LL_I2C_ACK;
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

#endif /* I2C0 || I2C1 */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
