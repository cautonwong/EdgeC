/**
 *****************************************************************************
 * @file   rn_rv_ll_uart6_7.c
 * @author Renergy Software Team
 * @brief  UART6_7 LL module driver.
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
#include "rn_rv_ll_uart6_7.h"
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

#if defined (UART6) || defined (UART7)

/** @addtogroup UART6_7_LL UART6_7
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup UART6_7_LL_Private_Macros
 * @{
 */

/* __BAUDRATE__ The maximum Baud Rate is derived from the maximum clock available
 *              divided by the smallest DIV (i.e. 2)    */
#define IS_LL_UART6_7_BAUDRATE(__BAUDRATE__) (((__BAUDRATE__) >= 300) && ((__BAUDRATE__) <= 3686400))

#define IS_LL_UART6_7_DATAWIDTH(__VALUE__) (((__VALUE__) == LL_UART6_7_DATAWIDTH_5B) \
                                          || ((__VALUE__) == LL_UART6_7_DATAWIDTH_6B) \
                                          || ((__VALUE__) == LL_UART6_7_DATAWIDTH_7B) \
                                          || ((__VALUE__) == LL_UART6_7_DATAWIDTH_8B))

#define IS_LL_UART6_7_STOPBITS(__VALUE__) (((__VALUE__) == LL_UART6_7_STOPBITS_1) \
                                         || ((__VALUE__) == LL_UART6_7_STOPBITS_2))

#define IS_LL_UART6_7_PARITY(__VALUE__) (((__VALUE__) == LL_UART6_7_PARITY_NONE) \
                                       || ((__VALUE__) == LL_UART6_7_PARITY_ODD) \
                                       || ((__VALUE__) == LL_UART6_7_PARITY_EVEN) \
                                       || ((__VALUE__) == LL_UART6_7_PARITY_ZERO) \
                                       || ((__VALUE__) == LL_UART6_7_PARITY_ONE) \
                                       || ((__VALUE__) == LL_UART6_7_PARITY_USER_DEFINED))

#define IS_LL_UART6_7_BITORDER(__VALUE__) (((__VALUE__) == LL_UART6_7_LSB_FIRST) \
                                         || ((__VALUE__) == LL_UART6_7_MSB_FIRST))

/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup UART6_7_LL_Exported_Functions
 * @{
 */

/** @addtogroup UART6_7_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize UART6_7 registers (Registers restored to their default values).
 * @param  UART6_7x UART6_7 Instance
 * @retval An ErrStatus enumeration value:
 *          - SUCCESS: UART6_7 registers are de-initialized
 *          - ERROR:   UART6_7 registers are not de-initialized
 */
ErrorStatus LL_UART6_7_DeInit(UART6_7_TypeDef *UART6_7x)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_UART6_7_ALL_INSTANCE(UART6_7x));

    if (UART6_7x == UART6_CORE0) {
        /* Force reset of UART6_7 clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART6);

        /* Release reset of UART6_7 clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART6);
    } else if (UART6_7x == UART7_CORE0) {
        /* Force reset of UART6_7 clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART7);

        /* Release reset of UART6_7 clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART7);
    } else if (UART6_7x == UART6_CORE1) {
        /* Force reset of UART6_7 clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART6);

        /* Release reset of UART6_7 clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART6);
    } else if (UART6_7x == UART7_CORE1) {
        /* Force reset of UART6_7 clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART7);

        /* Release reset of UART6_7 clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART7);
    } else {
        status = ERROR;
    }

    return status;
}

/**
 * @brief  Initialize UART6_7 registers according to the specified
 *         parameters in UART6_7_InitStruct.
 * @note   Baud rate value stored in UART6_7_InitStruct BaudRate field, should be valid.
 * @param  UART6_7x UART6_7 Instance
 * @param  UART6_7_InitStruct pointer to a @ref LL_UART6_7_InitTypeDef structure
 *         that contains the configuration information for the specified UART6_7 peripheral.
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: UART6_7 registers are initialized according to UART6_7_InitStruct content
 *          - ERROR: Problem occurred during UART6_7 Registers initialization
 */
ErrorStatus LL_UART6_7_Init(UART6_7_TypeDef *UART6_7x, const LL_UART6_7_InitTypeDef *UART6_7_InitStruct)
{
    ErrorStatus status = ERROR;
    uint32_t periphclk = 0U;

    /* Check the parameters */
    assert_param(IS_UART6_7_ALL_INSTANCE(UART6_7x));
    assert_param(IS_LL_UART6_7_BAUDRATE(UART6_7_InitStruct->BaudRate));
    assert_param(IS_LL_UART6_7_DATAWIDTH(UART6_7_InitStruct->DataWidth));
    assert_param(IS_LL_UART6_7_STOPBITS(UART6_7_InitStruct->StopBits));
    assert_param(IS_LL_UART6_7_PARITY(UART6_7_InitStruct->Parity));
    assert_param(IS_LL_UART6_7_BITORDER(UART6_7_InitStruct->TransferBitOrder));

    /* Disable the selected UART6_7x Peripheral */
    LL_UART6_7_Disable(UART6_7x);

    /*---------------------------- UART6_7 CTRL Configuration -----------------------
     * Configure UART6_7x SETUP (UART6_7 Word Length, Stop, Parity, and BitOrder bits) with parameters:
     * - DataWidth:             UART6_7_MODE_DATLEN bits according to UART6_7_InitStruct->DataWidth value
     * - Stop Bits:             UART6_7_MODE_STOPS bit according to UART6_7_InitStruct->StopBits value
     * - Parity:                UART6_7_MODE_PARS bits according to UART6_7_InitStruct->Parity value
     * - TransferBitOrder:      UART6_7_MODE_LMSB, bit according to UART6_7_InitStruct->TransferBitOrder value
     */
    MODIFY_REG(UART6_7x->CTRL,
               (UART6_7_CTRL_DATLEN | UART6_7_CTRL_STOPS | UART6_7_CTRL_PARS | UART6_7_CTRL_LMSB),
               (UART6_7_InitStruct->DataWidth | UART6_7_InitStruct->StopBits |
                UART6_7_InitStruct->Parity | UART6_7_InitStruct->TransferBitOrder));

    /* Retrieve Clock frequency used for UART6_7 Peripheral */
    if ((UART6_7x == UART6_CORE0) || (UART6_7x == UART7_CORE0)) {
        periphclk = __LL_SYSTEM_CORE0_CALC_CORE0_CLK_FREQ(LL_SYSTEM_CORE0_GetCore0SystemClockFreq());
    } else if ((UART6_7x == UART6_CORE1) || (UART6_7x == UART7_CORE1)) {
        periphclk = __LL_SYSTEM_CORE1_CALC_CORE1_CLK_FREQ(LL_SYSTEM_CORE1_GetCore1SystemClockFreq());
    } else {
        status = ERROR;
    }

    /*---------------------------- UART6_7 BAUD and FDIV Configuration -----------------------
     * Configure the UART6_7 Baud Rate :
       - valid baud rate value is required
    */
    if ((periphclk != 0) && (UART6_7_InitStruct->BaudRate != 0U)) {
        status = SUCCESS;
        LL_UART6_7_SetBaudRate(UART6_7x, periphclk, UART6_7_InitStruct->BaudRate);
    }
    /* Enable the selected UART6_7x Peripheral */
    LL_UART6_7_Enable(UART6_7x);

    return (status);
}

/**
 * @brief Set each @ref LL_UART6_7_InitTypeDef field to default value.
 * @param UART6_7_InitStruct Pointer to a @ref LL_UART6_7_InitTypeDef structure
 *                         whose fields will be set to default values.
 * @retval None
 */
void LL_UART6_7_StructInit(LL_UART6_7_InitTypeDef *UART6_7_InitStruct)
{
    /* Set UART6_7_InitStruct fields to default values */
    UART6_7_InitStruct->BaudRate            = 115200U;
    UART6_7_InitStruct->DataWidth           = LL_UART6_7_DATAWIDTH_8B;
    UART6_7_InitStruct->StopBits            = LL_UART6_7_STOPBITS_1;
    UART6_7_InitStruct->Parity              = LL_UART6_7_PARITY_NONE;
    UART6_7_InitStruct->TransferBitOrder    = LL_UART6_7_LSB_FIRST;
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

#endif /* UART6 || UART7 */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
