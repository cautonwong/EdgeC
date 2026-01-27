/**
 ******************************************************************************
 * @file   rn_rv_ll_iso7816.c
 * @author Renergy Software Team
 * @brief  ISO7816 LL module driver.
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
#include "rn_rv_ll_iso7816.h"
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

#if defined (ISO78160) || defined (ISO78161)

/** @addtogroup ISO7816_LL ISO7816
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup ISO7816_LL_Private_Macros
 * @{
 */

#define IS_LL_ISO7816_CLOCK_DIVISION(__VALUE__)    (((__VALUE__) == LL_ISO7816_CLOCKDIV_NONE) \
                                                    || ((__VALUE__) == LL_ISO7816_CLOCKDIV_2) \
                                                    || ((__VALUE__) == LL_ISO7816_CLOCKDIV_4) \
                                                    || ((__VALUE__) == LL_ISO7816_CLOCKDIV_8) \
                                                    || ((__VALUE__) == LL_ISO7816_CLOCKDIV_16) \
                                                    || ((__VALUE__) == LL_ISO7816_CLOCKDIV_32))

#define IS_LL_ISO7816_ERROR_WIDTH(__VALUE__)       (((__VALUE__) == LL_ISO7816_ERRWIDTH_2ETU) \
                                                    || ((__VALUE__) == LL_ISO7816_ERRWIDTH_1ETU) \
                                                    || ((__VALUE__) == LL_ISO7816_ERRWIDTH_1_5ETU))

#define IS_LL_ISO7816_PARITY(__VALUE__)            (((__VALUE__) == LL_ISO7816_PARITY_NONE) \
                                                    || ((__VALUE__) == LL_ISO7816_PARITY_ODD) \
                                                    || ((__VALUE__) == LL_ISO7816_PARITY_EVEN) \
                                                    || ((__VALUE__) == LL_ISO7816_PARITY_ZERO) \
                                                    || ((__VALUE__) == LL_ISO7816_PARITY_ONE) \
                                                    || ((__VALUE__) == LL_ISO7816_PARITY_USER_DEFINED))

#define IS_LL_ISO7816_DIRECTION(__VALUE__)         (((__VALUE__) == LL_ISO7816_DIRECTION_NONE) \
                                                    || ((__VALUE__) == LL_ISO7816_DIRECTION_TX) \
                                                    || ((__VALUE__) == LL_ISO7816_DIRECTION_RX) \
                                                    || ((__VALUE__) == LL_ISO7816_DIRECTION_TX_RX))

#define IS_LL_ISO7816_RETRY_COUNT(__VALUE__)       (((__VALUE__) == LL_ISO7816_RETRYCOUNT_NONE) \
                                                    || ((__VALUE__) == LL_ISO7816_RETRYCOUNT_1) \
                                                    || ((__VALUE__) == LL_ISO7816_RETRYCOUNT_2) \
                                                    || ((__VALUE__) == LL_ISO7816_RETRYCOUNT_3) \
                                                    || ((__VALUE__) == LL_ISO7816_RETRYCOUNT_5) \
                                                    || ((__VALUE__) == LL_ISO7816_RETRYCOUNT_6) \
                                                    || ((__VALUE__) == LL_ISO7816_RETRYCOUNT_7))

#define IS_LL_ISO7816_GUARD_TIME_TX(__VALUE__)     (((__VALUE__) == LL_ISO7816_GUARDTIME_TX_2ETU) || \
                                                    ((__VALUE__) == LL_ISO7816_GUARDTIME_TX_1ETU))

#define IS_LL_ISO7816_GUARD_TIME_RX(__VALUE__)     (((__VALUE__) == LL_ISO7816_GUARDTIME_RX_2ETU) || \
                                                    ((__VALUE__) == LL_ISO7816_GUARDTIME_RX_1ETU))

/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup ISO7816_LL_Exported_Functions
 * @{
 */

/** @addtogroup ISO7816_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize ISO7816 registers (Registers restored to their default values).
 * @param  ISO7816x ISO7816 Instance
 * @retval An ErrStatus enumeration value:
 *          - SUCCESS: ISO7816 registers are de-initialized
 *          - ERROR:   ISO7816 registers are not de-initialized
 */
ErrorStatus LL_ISO7816_DeInit(ISO7816_TypeDef *ISO7816x)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_ISO7816_ALL_INSTANCE(ISO7816x));

    if (ISO7816x == ISO78160_CORE0) {
        /* Force reset of ISO7816 clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78160);

        /* Release reset of ISO7816 clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78160);
    } else if (ISO7816x == ISO78161_CORE0) {
        /* Force reset of ISO7816 clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78161);

        /* Release reset of ISO7816 clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_ISO78161);
    } else if (ISO7816x == ISO78160_CORE1) {
        /* Force reset of ISO7816 clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78160);

        /* Release reset of ISO7816 clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78160);
    } else if (ISO7816x == ISO78161_CORE1) {
        /* Force reset of ISO7816 clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78161);

        /* Release reset of ISO7816 clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_ISO78161);
    } else {
        status = ERROR;
    }

    return status;
}

/**
 * @brief  Initialize the ISO7816 registers according to the specified parameters in ISO7816_InitStruct.
 * @param  ISO7816x ISO7816x Instance
 * @param  ISO7816_InitStruct pointer to a @ref LL_ISO7816_InitTypeDef structure.
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: ISO7816 registers are initialized
 *          - ERROR:   Not applicable
 */
ErrorStatus LL_ISO7816_Init(ISO7816_TypeDef *ISO7816x, LL_ISO7816_InitTypeDef *ISO7816_InitStruct)
{
    ErrorStatus status = SUCCESS;

    /* Check the ISO7816 Instance ISO7816x */
    assert_param(IS_ISO7816_ALL_INSTANCE(ISO7816x));

    /* Check the ISO7816 parameters from ISO7816_InitStruct */
    assert_param(IS_LL_ISO7816_CLOCK_DIVISION(ISO7816_InitStruct->ClockDivision));
    assert_param(IS_LL_ISO7816_ERROR_WIDTH(ISO7816_InitStruct->ErrorWidth));
    assert_param(IS_LL_ISO7816_PARITY(ISO7816_InitStruct->Parity));
    assert_param(IS_LL_ISO7816_DIRECTION(ISO7816_InitStruct->TransferDirection));
    assert_param(IS_LL_ISO7816_RETRY_COUNT(ISO7816_InitStruct->AutoRetryCount));
    assert_param(IS_LL_ISO7816_GUARD_TIME_TX(ISO7816_InitStruct->TXGuardTime));
    assert_param(IS_LL_ISO7816_GUARD_TIME_RX(ISO7816_InitStruct->RXGuardTime));

    /* Disable the selected ISO7816x Peripheral */
    LL_ISO7816_Disable(ISO7816x);

    /*---------------------------- ISO7816 CTRL Configuration -----------------------
     * Configure ISO7816x CTRL (ISO7816 Error Width, Parity, Transfer Direction Automatic Retransmission Count,
                 Transmit guard time and Receive guard timebits) with parameters:
     * - ErrorWidth:         ISO7816_CTRL_ERRWTH bits according to ISO7816_InitStruct->ErrorWidth value
     * - Parity:             ISO7816_CTRL_PARSEL bits according to ISO7816_InitStruct->Parity value
     * - TransferDirection:  ISO7816_CTRL_TX_EN, ISO7816_CTRL_RX_EN bits according to ISO7816_InitStruct->TransferDirection value
     * - AutoRetryCount:     ISO7816_CTRL_REP_CNT bits according to ISO7816_InitStruct->AutoRetryCount value.
     * - TXGuardTime:        ISO7816_CTRL_TX_GT bit according to ISO7816_InitStruct->TXGuardTime value.
     * - RXGuardTime:        ISO7816_CTRL_RX_GT bits according to ISO7816_InitStruct->RXGuardTime value.
     */
    LL_ISO7816_SetErrorWidth(ISO7816x, ISO7816_InitStruct->ErrorWidth);
    LL_ISO7816_SetParity(ISO7816x, ISO7816_InitStruct->Parity);
    LL_ISO7816_SetTransferDirection(ISO7816x, ISO7816_InitStruct->TransferDirection);
    LL_ISO7816_SetAutoRetransmissionCount(ISO7816x, ISO7816_InitStruct->AutoRetryCount);
    LL_ISO7816_SetGuardTime_TX(ISO7816x, ISO7816_InitStruct->TXGuardTime);
    LL_ISO7816_SetGuardTime_RX(ISO7816x, ISO7816_InitStruct->RXGuardTime);

    /*---------------------------- ISO7816x Clock Configuration ------------------------
    * Configure the clock output division coefficient with parameter :
    * - ClockDivision: ISO7816_CLK_CLKDIV bits according to ISO7816_InitStruct->ClockDivision value
    */
    LL_ISO7816_SetClockDivision(ISO7816x, ISO7816_InitStruct->ClockDivision);

    /* Enable Card Clock Output (CLKO_EN = 1). */
    LL_ISO7816_EnableClockOutput(ISO7816x);

    /* Enable the selected ISO7816x Peripheral */
    LL_ISO7816_Enable(ISO7816x);

    return status;
}

/**
 * @brief  Set each @ref LL_ISO7816_InitTypeDef field to default value.
 * @param  ISO7816_InitStruct Pointer to a @ref LL_ISO7816_InitTypeDef structure.
 * @retval None
 */
void LL_ISO7816_StructInit(LL_ISO7816_InitTypeDef *ISO7816_InitStruct)
{
  /* Set ISO7816_InitStruct fields to default values */
    ISO7816_InitStruct->ClockDivision     = LL_ISO7816_CLOCKDIV_4;
    ISO7816_InitStruct->ErrorWidth        = LL_ISO7816_ERRWIDTH_1ETU;
    ISO7816_InitStruct->Parity            = LL_ISO7816_PARITY_EVEN;
    ISO7816_InitStruct->TransferDirection = LL_ISO7816_DIRECTION_TX_RX;
    ISO7816_InitStruct->AutoRetryCount    = LL_ISO7816_RETRYCOUNT_3;
    ISO7816_InitStruct->TXGuardTime       = LL_ISO7816_GUARDTIME_TX_1ETU;
    ISO7816_InitStruct->RXGuardTime       = LL_ISO7816_GUARDTIME_RX_1ETU;
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

#endif /* ISO78160 || ISO78161 */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
