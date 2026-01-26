/**
 *****************************************************************************
 * @file   rn_rv_ll_usart.c
 * @author Renergy Software Team
 * @brief  USART LL module driver.
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
#include "rn_rv_ll_usart.h"
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

#if defined (USART0) || defined (USART1)

/** @addtogroup USART_LL USART
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup USART_LL_Private_Macros
 * @{
 */

/* __BAUDRATE__ The maximum Baud Rate is derived from the maximum clock available
 *              divided by the smallest DIV (i.e. 2)    */
#define IS_LL_USART_BAUDRATE(__BAUDRATE__) ((__BAUDRATE__) <= 14745600U)

/* __VALUE__ DIV content must be greater than or equal to 2d. */
#define IS_LL_USART_BRR_MIN(__VALUE__) ((__VALUE__) >= 2U)

#define IS_LL_USART_DIRECTION(__VALUE__) (((__VALUE__) == LL_USART_DIRECTION_NONE) \
                                          || ((__VALUE__) == LL_USART_DIRECTION_RX) \
                                          || ((__VALUE__) == LL_USART_DIRECTION_TX) \
                                          || ((__VALUE__) == LL_USART_DIRECTION_TX_RX))

#define IS_LL_USART_PARITY(__VALUE__) (((__VALUE__) == LL_USART_PARITY_NONE) \
                                       || ((__VALUE__) == LL_USART_PARITY_EVEN) \
                                       || ((__VALUE__) == LL_USART_PARITY_ODD))

#define IS_LL_USART_DATAWIDTH(__VALUE__) (((__VALUE__) == LL_USART_DATAWIDTH_5B) \
                                          || ((__VALUE__) == LL_USART_DATAWIDTH_6B) \
                                          || ((__VALUE__) == LL_USART_DATAWIDTH_7B) \
                                          || ((__VALUE__) == LL_USART_DATAWIDTH_8B) \
                                          || ((__VALUE__) == LL_USART_DATAWIDTH_9B))

#define IS_LL_USART_LASTBITCLKOUTPUT(__VALUE__) (((__VALUE__) == LL_USART_LASTCLKPULSE_NO_OUTPUT) \
                                                 || ((__VALUE__) == LL_USART_LASTCLKPULSE_OUTPUT))

#define IS_LL_USART_CLOCKPHASE(__VALUE__) (((__VALUE__) == LL_USART_PHASE_1EDGE) \
                                           || ((__VALUE__) == LL_USART_PHASE_2EDGE))

#define IS_LL_USART_CLOCKPOLARITY(__VALUE__) (((__VALUE__) == LL_USART_POLARITY_LOW) \
                                              || ((__VALUE__) == LL_USART_POLARITY_HIGH))

#define IS_LL_USART_CLOCKOUTPUT(__VALUE__) (((__VALUE__) == LL_USART_CLOCK_DISABLE) \
                                            || ((__VALUE__) == LL_USART_CLOCK_ENABLE))

#define IS_LL_USART_STOPBITS(__VALUE__) (((__VALUE__) == LL_USART_STOPBITS_0_5) \
                                         || ((__VALUE__) == LL_USART_STOPBITS_1) \
                                         || ((__VALUE__) == LL_USART_STOPBITS_1_5) \
                                         || ((__VALUE__) == LL_USART_STOPBITS_2))

#define IS_LL_USART_HWCONTROL(__VALUE__) (((__VALUE__) == LL_USART_HWCONTROL_NONE) \
                                          || ((__VALUE__) == LL_USART_HWCONTROL_RTS) \
                                          || ((__VALUE__) == LL_USART_HWCONTROL_CTS) \
                                          || ((__VALUE__) == LL_USART_HWCONTROL_RTS_CTS))
/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup USART_LL_Exported_Functions
 * @{
 */

/** @addtogroup USART_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize USART registers (Registers restored to their default values).
 * @param  USARTx USART Instance
 * @retval An ErrStatus enumeration value:
 *          - SUCCESS: USART registers are de-initialized
 *          - ERROR:   USART registers are not de-initialized
 */
ErrorStatus LL_USART_DeInit(USART_TypeDef *USARTx)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_USART_ALL_INSTANCE(USARTx));

    if (USARTx == USART0_CORE0) {
        /* Force reset of USART clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_USART0);

        /* Release reset of USART clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_USART0);
    } else if (USARTx == USART1_CORE0) {
        /* Force reset of USART clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_USART1);

        /* Release reset of USART clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_USART1);
    } else if (USARTx == USART0_CORE1) {
        /* Force reset of USART clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_USART0);

        /* Release reset of USART clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_USART0);
    } else if (USARTx == USART1_CORE1) {
        /* Force reset of USART clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_USART1);

        /* Release reset of USART clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_USART1);
    } else {
        status = ERROR;
    }

    return status;
}

/**
 * @brief  Initialize USART registers according to the specified
 *         parameters in USART_InitStruct.
 * @note   Baud rate value stored in USART_InitStruct BaudRate field, should be valid (different from 0 or 1).
 * @param  USARTx USART Instance
 * @param  USART_InitStruct pointer to a @ref LL_USART_InitTypeDef structure
 *         that contains the configuration information for the specified USART peripheral.
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: USART registers are initialized according to USART_InitStruct content
 *          - ERROR: Problem occurred during USART Registers initialization
 */
ErrorStatus LL_USART_Init(USART_TypeDef *USARTx, const LL_USART_InitTypeDef *USART_InitStruct)
{
    ErrorStatus status = ERROR;
    uint32_t periphclk = 0x0U;

    /* Check the parameters */
    assert_param(IS_USART_ALL_INSTANCE(USARTx));
    assert_param(IS_LL_USART_BAUDRATE(USART_InitStruct->BaudRate));
    assert_param(IS_LL_USART_DATAWIDTH(USART_InitStruct->DataWidth));
    assert_param(IS_LL_USART_STOPBITS(USART_InitStruct->StopBits));
    assert_param(IS_LL_USART_PARITY(USART_InitStruct->Parity));
    assert_param(IS_LL_USART_DIRECTION(USART_InitStruct->TransferDirection));
    assert_param(IS_LL_USART_HWCONTROL(USART_InitStruct->HardwareFlowControl));

    /*---------------------------- USART SETUP Configuration -----------------------
     * Configure USARTx SETUP (USART Word Length, Parity and Hardware Flow Control bits) with parameters:
     * - DataWidth:             USART_SETUP_BIT_LENGTH bits according to USART_InitStruct->DataWidth value
     * - Parity:                USART_SETUP_PARITY_EN, USART_SETUP_PARITY_SEL bits according to USART_InitStruct->Parity value
     * - HardwareFlowControl:   USART_SETUP_RTSE, USART_SETUP_CTSE bits according to USART_InitStruct->HardwareFlowControl value
     */
    MODIFY_REG(USARTx->SETUP,
               (USART_SETUP_BIT_LENGTH | USART_SETUP_PARITY_EN | USART_SETUP_PARITY_SEL |
                USART_SETUP_RTSE | USART_SETUP_CTSE),
               (USART_InitStruct->DataWidth | USART_InitStruct->Parity |
                USART_InitStruct->HardwareFlowControl));

    /*---------------------------- USART TXCTRL RXCTRL Configuration -----------------------
     * Configure USARTx TXCTRL RXCTRL (USART Transfer Direction, Stop bits) with parameters:
     * - TransferDirection:  USART_TXCTRL_TXEN, USART_RXCTRL_RXEN bits according to USART_InitStruct->TransferDirection value
     * - Stop Bits:          USART_TXCTRL_CFG_STOP_BIT bits according to USART_InitStruct->StopBits value.
     */
    if (USART_InitStruct->TransferDirection == LL_USART_DIRECTION_NONE) {
        LL_USART_DisableDirectionTx(USARTx);
        LL_USART_DisableDirectionRx(USARTx);
    } else if (USART_InitStruct->TransferDirection == LL_USART_DIRECTION_RX) {
        LL_USART_EnableDirectionRx(USARTx);
        LL_USART_DisableDirectionTx(USARTx);
    } else if (USART_InitStruct->TransferDirection == LL_USART_DIRECTION_TX) {
        LL_USART_DisableDirectionRx(USARTx);
        LL_USART_EnableDirectionTx(USARTx);
    } else if (USART_InitStruct->TransferDirection == LL_USART_DIRECTION_TX_RX) {
        LL_USART_EnableDirectionTx(USARTx);
        LL_USART_EnableDirectionRx(USARTx);
    }
    LL_USART_SetStopBitsLength(USARTx, USART_InitStruct->StopBits);

    /*---------------------------- USART DIV Configuration -----------------------
     * Retrieve Clock frequency used for USART Peripheral
     */

    /* Retrieve Clock frequency used for USART Peripheral */
    if ((USARTx == USART0_CORE0) || (USARTx == USART1_CORE0)) {
        periphclk = __LL_SYSTEM_CORE0_CALC_CORE0_CLK_FREQ(LL_SYSTEM_CORE0_GetCore0SystemClockFreq());
    } else if ((USARTx == USART0_CORE1) || (USARTx == USART1_CORE1)) {
        periphclk = __LL_SYSTEM_CORE1_CALC_CORE1_CLK_FREQ(LL_SYSTEM_CORE1_GetCore1SystemClockFreq());
    } else {
        status = ERROR;
    }

    /* Configure the USART Baud Rate :
       - valid baud rate value (different from 0) is required
       - Peripheral clock as returned by System service, should be valid (different from 0).
    */
    if ((periphclk != 0) && (USART_InitStruct->BaudRate != 0U)) {
        status = SUCCESS;
        LL_USART_SetBaudRate(USARTx, periphclk, USART_InitStruct->BaudRate);

        /* Check DIV is greater than or equal to 2d */
        assert_param(IS_LL_USART_BRR_MIN(USARTx->DIV));
    }

    return (status);
}

/**
 * @brief Set each @ref LL_USART_InitTypeDef field to default value.
 * @param USART_InitStruct Pointer to a @ref LL_USART_InitTypeDef structure
 *                         whose fields will be set to default values.
 * @retval None
 */
void LL_USART_StructInit(LL_USART_InitTypeDef *USART_InitStruct)
{
    /* Set USART_InitStruct fields to default values */
    USART_InitStruct->BaudRate            = 115200U;
    USART_InitStruct->DataWidth           = LL_USART_DATAWIDTH_8B;
    USART_InitStruct->StopBits            = LL_USART_STOPBITS_1;
    USART_InitStruct->Parity              = LL_USART_PARITY_NONE;
    USART_InitStruct->TransferDirection   = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct->HardwareFlowControl = LL_USART_HWCONTROL_NONE;
}

/**
 * @brief  Initialize USART Clock related settings according to the
 *         specified parameters in the USART_ClockInitStruct.
 * @param  USARTx USART Instance
 * @param  USART_ClockInitStruct Pointer to a @ref LL_USART_ClockInitTypeDef structure
 *         that contains the Clock configuration information for the specified USART peripheral.
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: USART registers related to Clock settings are initialized according to USART_ClockInitStruct content
 *          - ERROR: Problem occurred during USART Registers initialization
 */
ErrorStatus LL_USART_ClockInit(USART_TypeDef *USARTx, const LL_USART_ClockInitTypeDef *USART_ClockInitStruct)
{
    ErrorStatus status = SUCCESS;

    /* Check USART Instance and Clock signal output parameters */
    assert_param(IS_USART_ALL_INSTANCE(USARTx));
    assert_param(IS_LL_USART_CLOCKOUTPUT(USART_ClockInitStruct->ClockOutput));

    /*---------------------------- USART SETUP Configuration -----------------------*/
    /* If Clock signal has to be output */
    if (USART_ClockInitStruct->ClockOutput == LL_USART_CLOCK_DISABLE) {
        /* Deactivate Clock signal delivery :
        * - Disable Clock Output:        USART_SETUP_CLK_EN cleared
        */
        LL_USART_DisableSCLKOutput(USARTx);
    } else {
        /* Check clock related parameters */
        assert_param(IS_LL_USART_CLOCKPOLARITY(USART_ClockInitStruct->ClockPolarity));
        assert_param(IS_LL_USART_CLOCKPHASE(USART_ClockInitStruct->ClockPhase));
        assert_param(IS_LL_USART_LASTBITCLKOUTPUT(USART_ClockInitStruct->LastBitClockPulse));

        /*---------------------------- USART SETUP Configuration -----------------------
        * Configure USARTx SETUP (Clock signal related bits) with parameters:
        * - Enable Clock Output:         USART_SETUP_CLK_EN set
        * - Clock Polarity:              USART_SETUP_CPOL bit according to USART_ClockInitStruct->ClockPolarity value
        * - Clock Phase:                 USART_SETUP_CPHA bit according to USART_ClockInitStruct->ClockPhase value
        * - Last Bit Clock Pulse Output: USART_SETUP_LBCL bit according to USART_ClockInitStruct->LastBitClockPulse value.
        */
        MODIFY_REG(USARTx->SETUP,
                  USART_SETUP_CLK_EN | USART_SETUP_CPHA | USART_SETUP_CPOL | USART_SETUP_LBCL,
                  USART_SETUP_CLK_EN | USART_ClockInitStruct->ClockPolarity |
                  USART_ClockInitStruct->ClockPhase | USART_ClockInitStruct->LastBitClockPulse);
    }

    return (status);
}

/**
 * @brief Set each field of a @ref LL_USART_ClockInitTypeDef type structure to default value.
 * @param USART_ClockInitStruct Pointer to a @ref LL_USART_ClockInitTypeDef structure
 *                              whose fields will be set to default values.
 * @retval None
 */
void LL_USART_ClockStructInit(LL_USART_ClockInitTypeDef *USART_ClockInitStruct)
{
    /* Set LL_USART_ClockInitStruct fields with default values */
    USART_ClockInitStruct->ClockOutput       = LL_USART_CLOCK_DISABLE;
    USART_ClockInitStruct->ClockPolarity     = LL_USART_POLARITY_LOW;            /* Not relevant when ClockOutput = LL_USART_CLOCK_DISABLE */
    USART_ClockInitStruct->ClockPhase        = LL_USART_PHASE_1EDGE;             /* Not relevant when ClockOutput = LL_USART_CLOCK_DISABLE */
    USART_ClockInitStruct->LastBitClockPulse = LL_USART_LASTCLKPULSE_NO_OUTPUT;  /* Not relevant when ClockOutput = LL_USART_CLOCK_DISABLE */
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

#endif /* USART0 || USART1 */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
