/**
 *****************************************************************************
 * @file   rn_rv_ll_uart0_5.c
 * @author Renergy Software Team
 * @brief  UART0_5 LL module driver.
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
#include "rn_rv_ll_uart0_5.h"
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

#if defined (UART0) || defined (UART1) || defined (UART2) || defined (UART3) || defined (UART4) || defined (UART5)

/** @addtogroup UART0_5_LL UART0_5
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup UART0_5_LL_Private_Macros
 * @{
 */

/* __BAUDRATE__ The maximum Baud Rate is derived from the maximum clock available
 *              divided by the smallest DIV (i.e. 2)    */
#define IS_LL_UART0_5_BAUDRATE(__BAUDRATE__) (((__BAUDRATE__) >= 300) && ((__BAUDRATE__) <= 3686400))

#define IS_LL_UART0_5_DATAWIDTH(__VALUE__) (((__VALUE__) == LL_UART0_5_DATAWIDTH_5B) \
                                          || ((__VALUE__) == LL_UART0_5_DATAWIDTH_6B) \
                                          || ((__VALUE__) == LL_UART0_5_DATAWIDTH_7B) \
                                          || ((__VALUE__) == LL_UART0_5_DATAWIDTH_8B))

#define IS_LL_UART0_5_STOPBITS(__VALUE__) (((__VALUE__) == LL_UART0_5_STOPBITS_1) \
                                         || ((__VALUE__) == LL_UART0_5_STOPBITS_2))

#define IS_LL_UART0_5_PARITY(__VALUE__) (((__VALUE__) == LL_UART0_5_PARITY_NONE) \
                                       || ((__VALUE__) == LL_UART0_5_PARITY_ODD) \
                                       || ((__VALUE__) == LL_UART0_5_PARITY_EVEN) \
                                       || ((__VALUE__) == LL_UART0_5_PARITY_ZERO) \
                                       || ((__VALUE__) == LL_UART0_5_PARITY_ONE) \
                                       || ((__VALUE__) == LL_UART0_5_PARITY_USER_DEFINED))

#define IS_LL_UART0_5_BITORDER(__VALUE__) (((__VALUE__) == LL_UART0_5_LSB_FIRST) \
                                         || ((__VALUE__) == LL_UART0_5_MSB_FIRST))

/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup UART0_5_LL_Exported_Functions
 * @{
 */

/** @addtogroup UART0_5_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize UART0_5 registers (Registers restored to their default values).
 * @param  UART0_5x UART0_5 Instance
 * @retval An ErrStatus enumeration value:
 *          - SUCCESS: UART0_5 registers are de-initialized
 *          - ERROR:   UART0_5 registers are not de-initialized
 */
ErrorStatus LL_UART0_5_DeInit(UART0_5_TypeDef *UART0_5x)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_UART0_5_ALL_INSTANCE(UART0_5x));

    if (UART0_5x == UART0_CORE0) {
        /* Force reset of UART0_5 clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART0);

        /* Release reset of UART0_5 clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART0);
    } else if (UART0_5x == UART1_CORE0) {
        /* Force reset of UART0_5 clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART1);

        /* Release reset of UART0_5 clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART1);
    } else if (UART0_5x == UART2_CORE0) {
        /* Force reset of UART0_5 clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART2);

        /* Release reset of UART0_5 clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART2);
    } else if (UART0_5x == UART3_CORE0) {
        /* Force reset of UART0_5 clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART3);

        /* Release reset of UART0_5 clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART3);
    } else if (UART0_5x == UART4_CORE0) {
        /* Force reset of UART0_5 clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART4);

        /* Release reset of UART0_5 clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART4);
    } else if (UART0_5x == UART5_CORE0) {
        /* Force reset of UART0_5 clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART5);

        /* Release reset of UART0_5 clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_UART5);
    } else if (UART0_5x == UART0_CORE1) {
        /* Force reset of UART0_5 clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART0);

        /* Release reset of UART0_5 clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART0);
    } else if (UART0_5x == UART1_CORE1) {
        /* Force reset of UART0_5 clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART1);

        /* Release reset of UART0_5 clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART1);
    } else if (UART0_5x == UART2_CORE1) {
        /* Force reset of UART0_5 clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART2);

        /* Release reset of UART0_5 clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART2);
    } else if (UART0_5x == UART3_CORE1) {
        /* Force reset of UART0_5 clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART3);

        /* Release reset of UART0_5 clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART3);
    } else if (UART0_5x == UART4_CORE1) {
        /* Force reset of UART0_5 clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART4);

        /* Release reset of UART0_5 clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART4);
    } else if (UART0_5x == UART5_CORE1) {
        /* Force reset of UART0_5 clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART5);

        /* Release reset of UART0_5 clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_UART5);
    } else {
        status = ERROR;
    }

    return status;
}

/**
 * @brief  Initialize UART0_5 registers according to the specified
 *         parameters in UART0_5_InitStruct.
 * @note   Baud rate value stored in UART0_5_InitStruct BaudRate field, should be valid.
 * @param  UART0_5x UART0_5 Instance
 * @param  UART0_5_InitStruct pointer to a @ref LL_UART0_5_InitTypeDef structure
 *         that contains the configuration information for the specified UART0_5 peripheral.
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: UART0_5 registers are initialized according to UART0_5_InitStruct content
 *          - ERROR: Problem occurred during UART0_5 Registers initialization
 */
ErrorStatus LL_UART0_5_Init(UART0_5_TypeDef *UART0_5x, const LL_UART0_5_InitTypeDef *UART0_5_InitStruct)
{
    ErrorStatus status = ERROR;
    uint32_t periphclk = 0U;

    /* Check the parameters */
    assert_param(IS_UART0_5_ALL_INSTANCE(UART0_5x));
    assert_param(IS_LL_UART0_5_BAUDRATE(UART0_5_InitStruct->BaudRate));
    assert_param(IS_LL_UART0_5_DATAWIDTH(UART0_5_InitStruct->DataWidth));
    assert_param(IS_LL_UART0_5_STOPBITS(UART0_5_InitStruct->StopBits));
    assert_param(IS_LL_UART0_5_PARITY(UART0_5_InitStruct->Parity));
    assert_param(IS_LL_UART0_5_BITORDER(UART0_5_InitStruct->TransferBitOrder));

    /* Disable the selected UART0_5x Peripheral */
    LL_UART0_5_Disable(UART0_5x);

    /*---------------------------- UART0_5 CTRL Configuration -----------------------
     * Configure UART0_5x SETUP (UART0_5 Word Length, Stop, Parity, and BitOrder bits) with parameters:
     * - DataWidth:             UART0_5_MODE_DATLEN bits according to UART0_5_InitStruct->DataWidth value
     * - Stop Bits:             UART0_5_MODE_STOPS bit according to UART0_5_InitStruct->StopBits value
     * - Parity:                UART0_5_MODE_PARS bits according to UART0_5_InitStruct->Parity value
     * - TransferBitOrder:      UART0_5_MODE_LMSB, bit according to UART0_5_InitStruct->TransferBitOrder value
     */
    MODIFY_REG(UART0_5x->CTRL,
               (UART0_5_CTRL_DATLEN | UART0_5_CTRL_STOPS | UART0_5_CTRL_PARS | UART0_5_CTRL_LMSB),
               (UART0_5_InitStruct->DataWidth | UART0_5_InitStruct->StopBits |
                UART0_5_InitStruct->Parity | UART0_5_InitStruct->TransferBitOrder));

    /* Retrieve Clock frequency used for UART0_5 Peripheral */
    if ((UART0_5x == UART0_CORE0) || (UART0_5x == UART1_CORE0) || (UART0_5x == UART2_CORE0) ||
        (UART0_5x == UART3_CORE0) || (UART0_5x == UART4_CORE0) || (UART0_5x == UART5_CORE0)) {
        periphclk = __LL_SYSTEM_CORE0_CALC_CORE0_CLK_FREQ(LL_SYSTEM_CORE0_GetCore0SystemClockFreq());
    } else if ((UART0_5x == UART0_CORE1) || (UART0_5x == UART1_CORE1) || (UART0_5x == UART2_CORE1) ||
                (UART0_5x == UART3_CORE1) || (UART0_5x == UART4_CORE1) || (UART0_5x == UART5_CORE1)) {
        periphclk = __LL_SYSTEM_CORE1_CALC_CORE1_CLK_FREQ(LL_SYSTEM_CORE1_GetCore1SystemClockFreq());
    } else {
        status = ERROR;
    }

    /*---------------------------- UART0_5 BAUD and FDIV Configuration -----------------------
     * Configure the UART0_5 Baud Rate :
       - valid baud rate value is required
    */
   if ((periphclk != 0) && (UART0_5_InitStruct->BaudRate != 0U)) {
        status = SUCCESS;
        LL_UART0_5_SetBaudRate(UART0_5x, periphclk, UART0_5_InitStruct->BaudRate);
    }

    /* Enable the selected UART0_5x Peripheral */
    LL_UART0_5_Enable(UART0_5x);

    return (status);
}

/**
 * @brief Set each @ref LL_UART0_5_InitTypeDef field to default value.
 * @param UART0_5_InitStruct Pointer to a @ref LL_UART0_5_InitTypeDef structure
 *                         whose fields will be set to default values.
 * @retval None
 */
void LL_UART0_5_StructInit(LL_UART0_5_InitTypeDef *UART0_5_InitStruct)
{
    /* Set UART0_5_InitStruct fields to default values */
    UART0_5_InitStruct->BaudRate            = 115200U;
    UART0_5_InitStruct->DataWidth           = LL_UART0_5_DATAWIDTH_8B;
    UART0_5_InitStruct->StopBits            = LL_UART0_5_STOPBITS_1;
    UART0_5_InitStruct->Parity              = LL_UART0_5_PARITY_NONE;
    UART0_5_InitStruct->TransferBitOrder    = LL_UART0_5_LSB_FIRST;
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

#endif /* UART0 || UART1 || UART2 || UART3 || UART4 || UART5 */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
