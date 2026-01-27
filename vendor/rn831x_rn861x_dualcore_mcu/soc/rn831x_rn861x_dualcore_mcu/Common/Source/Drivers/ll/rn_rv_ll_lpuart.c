/**
 *****************************************************************************
 * @file   rn_rv_ll_lpuart.c
 * @author Renergy Software Team
 * @brief  LPUART LL module driver.
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
#include "rn_rv_ll_lpuart.h"
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

#if defined (LPUART)

/** @addtogroup LPUART_LL LPUART
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup LPUART_LL_Private_Macros
 * @{
 */

/* __BAUDRATE__ The maximum Baud Rate is derived from the maximum clock available
 *              divided by the smallest DIV (i.e. 2)    */
#define IS_LL_LPUART_BAUDRATE(__BAUDRATE__) (((__BAUDRATE__) == LL_LPUART_BAUDRATE_9600) \
                                            || ((__BAUDRATE__) == LL_LPUART_BAUDRATE_4800) \
                                            || ((__BAUDRATE__) == LL_LPUART_BAUDRATE_2400) \
                                            || ((__BAUDRATE__) == LL_LPUART_BAUDRATE_1200) \
                                            || ((__BAUDRATE__) == LL_LPUART_BAUDRATE_600) \
                                            || ((__BAUDRATE__) == LL_LPUART_BAUDRATE_300))

#define IS_LL_LPUART_DATAWIDTH(__VALUE__) (((__VALUE__) == LL_LPUART_DATAWIDTH_5B) \
                                          || ((__VALUE__) == LL_LPUART_DATAWIDTH_6B) \
                                          || ((__VALUE__) == LL_LPUART_DATAWIDTH_7B) \
                                          || ((__VALUE__) == LL_LPUART_DATAWIDTH_8B))

#define IS_LL_LPUART_STOPBITS(__VALUE__) (((__VALUE__) == LL_LPUART_STOPBITS_1) \
                                         || ((__VALUE__) == LL_LPUART_STOPBITS_2))

#define IS_LL_LPUART_PARITY(__VALUE__) (((__VALUE__) == LL_LPUART_PARITY_NONE) \
                                       || ((__VALUE__) == LL_LPUART_PARITY_ODD) \
                                       || ((__VALUE__) == LL_LPUART_PARITY_EVEN) \
                                       || ((__VALUE__) == LL_LPUART_PARITY_ZERO) \
                                       || ((__VALUE__) == LL_LPUART_PARITY_ONE) \
                                       || ((__VALUE__) == LL_LPUART_PARITY_USER_DEFINED))

#define IS_LL_LPUART_DIRECTION(__VALUE__) (((__VALUE__) == LL_LPUART_DIRECTION_NONE) \
                                          || ((__VALUE__) == LL_LPUART_DIRECTION_RX) \
                                          || ((__VALUE__) == LL_LPUART_DIRECTION_TX) \
                                          || ((__VALUE__) == LL_LPUART_DIRECTION_TX_RX))

#define IS_LL_LPUART_BITORDER(__VALUE__) (((__VALUE__) == LL_LPUART_LSB_FIRST) \
                                         || ((__VALUE__) == LL_LPUART_MSB_FIRST))

#define IS_LL_LPUART_WAKEUPMETHOD(__VALUE__) (((__VALUE__) == LL_LPUART_WAKEUP_STARTBIT) \
                                             || ((__VALUE__) == LL_LPUART_WAKEUP_RXFALLINGEDGE) \
                                             || ((__VALUE__) == LL_LPUART_WAKEUP_RXCOMPLETE) \
                                             || ((__VALUE__) == LL_LPUART_WAKEUP_FRAMEMATCH))
/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup LPUART_LL_Exported_Functions
 * @{
 */

/** @addtogroup LPUART_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize LPUART registers (Registers restored to their default values).
 * @param  LPUARTx LPUART Instance
 * @retval An ErrStatus enumeration value:
 *          - SUCCESS: LPUART registers are de-initialized
 *          - ERROR:   LPUART registers are not de-initialized
 */
ErrorStatus LL_LPUART_DeInit(LPUART_TypeDef *LPUARTx)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_LPUART_ALL_INSTANCE(LPUARTx));

    if (LPUARTx == LPUART_CORE0) {
        /* Force reset of LPUART clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_LPUART);

        /* Release reset of LPUART clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_LPUART);
    } else if (LPUARTx == LPUART_CORE1) {
        /* Force reset of LPUART clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_LPUART);

        /* Release reset of LPUART clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_LPUART);
    } else {
        status = ERROR;
    }

    return status;
}

/**
 * @brief  Initialize LPUART registers according to the specified
 *         parameters in LPUART_InitStruct.
 * @note   Baud rate value stored in LPUART_InitStruct BaudRate field, should be valid.
 * @param  LPUARTx LPUART Instance
 * @param  LPUART_InitStruct pointer to a @ref LL_LPUART_InitTypeDef structure
 *         that contains the configuration information for the specified LPUART peripheral.
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: LPUART registers are initialized according to LPUART_InitStruct content
 *          - ERROR: Problem occurred during LPUART Registers initialization
 */
ErrorStatus LL_LPUART_Init(LPUART_TypeDef *LPUARTx, const LL_LPUART_InitTypeDef *LPUART_InitStruct)
{
    ErrorStatus status = ERROR;

    /* Check the parameters */
    assert_param(IS_LPUART_ALL_INSTANCE(LPUARTx));
    assert_param(IS_LL_LPUART_BAUDRATE(LPUART_InitStruct->BaudRate));
    assert_param(IS_LL_LPUART_DATAWIDTH(LPUART_InitStruct->DataWidth));
    assert_param(IS_LL_LPUART_STOPBITS(LPUART_InitStruct->StopBits));
    assert_param(IS_LL_LPUART_PARITY(LPUART_InitStruct->Parity));
    assert_param(IS_LL_LPUART_DIRECTION(LPUART_InitStruct->TransferDirection));
    assert_param(IS_LL_LPUART_BITORDER(LPUART_InitStruct->TransferBitOrder));
    assert_param(IS_LL_LPUART_WAKEUPMETHOD(LPUART_InitStruct->WakeUpMethod));

    /*---------------------------- LPUART MODE Configuration -----------------------
     * Configure LPUARTx SETUP (LPUART Word Length, Stop, Parity, Direction, BitOrder and WakeUpMethod bits) with parameters:
     * - DataWidth:             LPUART_MODE_DATLEN bits according to LPUART_InitStruct->DataWidth value
     * - Stop Bits:             LPUART_MODE_STOPS bit according to USART_InitStruct->StopBits value.
     * - Parity:                LPUART_MODE_PARS bits according to LPUART_InitStruct->Parity value
     * - TransferDirection:     LPUART_MODE_RE, LPUART_MODE_TE bits according to USART_InitStruct->TransferDirection value
     * - TransferBitOrder:      LPUART_MODE_LMSB, bit according to LPUART_InitStruct->TransferBitOrder value
     * - WakeUpMethod:          LPUART_MODE_WUSEL bits according to LPUART_InitStruct->WakeUpMethod value
     */
    MODIFY_REG(LPUARTx->MODE,
               (LPUART_MODE_DATLEN | LPUART_MODE_STOPS | LPUART_MODE_PARS |
                LPUART_MODE_RE | LPUART_MODE_TE | LPUART_MODE_LMSB | LPUART_MODE_WUSEL),
               (LPUART_InitStruct->DataWidth | LPUART_InitStruct->StopBits |
                LPUART_InitStruct->Parity | LPUART_InitStruct->TransferDirection |
                LPUART_InitStruct->TransferBitOrder | LPUART_InitStruct->WakeUpMethod));

    /*---------------------------- LPUART BAUD Configuration -----------------------
     * Configure the LPUART Baud Rate :
       - valid baud rate value is required
    */
    LL_LPUART_SetBaudRate(LPUARTx, LPUART_InitStruct->BaudRate);

    return (status);
}

/**
 * @brief Set each @ref LL_LPUART_InitTypeDef field to default value.
 * @param LPUART_InitStruct Pointer to a @ref LL_LPUART_InitTypeDef structure
 *                         whose fields will be set to default values.
 * @retval None
 */
void LL_LPUART_StructInit(LL_LPUART_InitTypeDef *LPUART_InitStruct)
{
    /* Set LPUART_InitStruct fields to default values */
    LPUART_InitStruct->BaudRate            = LL_LPUART_BAUDRATE_9600;
    LPUART_InitStruct->DataWidth           = LL_LPUART_DATAWIDTH_8B;
    LPUART_InitStruct->StopBits            = LL_LPUART_STOPBITS_1;
    LPUART_InitStruct->Parity              = LL_LPUART_PARITY_NONE;
    LPUART_InitStruct->TransferDirection   = LL_LPUART_DIRECTION_TX_RX;
    LPUART_InitStruct->TransferBitOrder    = LL_LPUART_LSB_FIRST;
    LPUART_InitStruct->WakeUpMethod        = LL_LPUART_WAKEUP_STARTBIT;
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

#endif /* LPUART */

/**
  * @}
  */

#endif /* USE_FULL_LL_DRIVER */
