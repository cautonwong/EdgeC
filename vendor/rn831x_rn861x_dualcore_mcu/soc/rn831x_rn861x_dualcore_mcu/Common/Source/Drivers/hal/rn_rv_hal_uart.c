/**
 ******************************************************************************
 * @file   rn_rv_hal_uart.c
 * @author Renergy Software Team
 * @brief  UART HAL module driver.
 *          This file provides firmware functions to manage the following
 *          functionalities of the internal UART memory:
 *           + Program operations functions
 *           + Memory Control functions
 *           + Peripheral State functions
 *
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
#include "rn_rv_hal.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @defgroup UART UART
 * @brief     UART driver modules
 * @{
 */

#if defined(HAL_UART_MODULE_ENABLED)

/* Private typedef ----------------------------------------------------------*/
/** @defgroup UART_Private_Typedef UART Private Typedef
 * @{
 */


/**
 * @}
 */

/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/** @addtogroup UART_Private_Functions  UART Private Functions
 * @{
 */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
void UART_InitCallbacksToDefault(UART_HandleTypeDef *huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
static void UART_EndTxTransfer(UART_HandleTypeDef *huart);
static void UART_EndRxTransfer(UART_HandleTypeDef *huart);
static void UART_DMATransmitCplt(UART_DMA_HandleTypeDef *hdma);
static void UART_DMAReceiveCplt(UART_DMA_HandleTypeDef *hdma);
static void UART_DMATxHalfCplt(UART_DMA_HandleTypeDef *hdma);
static void UART_DMARxHalfCplt(UART_DMA_HandleTypeDef *hdma);
static void UART_DMAError(UART_DMA_HandleTypeDef *hdma);
static void UART_DMAAbortOnError(UART_DMA_HandleTypeDef *hdma);
static void UART_DMATxAbortCallback(UART_DMA_HandleTypeDef *hdma);
static void UART_DMARxAbortCallback(UART_DMA_HandleTypeDef *hdma);
static void UART_DMATxOnlyAbortCallback(UART_DMA_HandleTypeDef *hdma);
static void UART_DMARxOnlyAbortCallback(UART_DMA_HandleTypeDef *hdma);
static HAL_StatusTypeDef UART_Transmit_IT(UART_HandleTypeDef *huart);
static HAL_StatusTypeDef UART_EndTransmit_IT(UART_HandleTypeDef *huart);
static HAL_StatusTypeDef UART_Receive_IT(UART_HandleTypeDef *huart);
static HAL_StatusTypeDef UART_DMA_Start_TX_IT(UART_HandleTypeDef *huart, uint32_t Address, uint32_t DataLength);
static HAL_StatusTypeDef UART_DMA_Start_RX_IT(UART_HandleTypeDef *huart, uint32_t Address, uint32_t DataLength);
static HAL_StatusTypeDef UART_WaitOnFlagUntilTimeout(UART_HandleTypeDef *huart, uint32_t Flag, FlagStatus Status,
                                                     uint32_t Tickstart, uint32_t Timeout);
static HAL_StatusTypeDef UART_SetConfig(UART_HandleTypeDef *huart);
/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup UART_Exported_Functions UART Exported Functions
 * @{
 */

/** @defgroup UART_Exported_Functions_Group1 Initialization and de-initialization functions
 *  @brief    Initialization and Configuration functions
 *
@verbatim
 ===============================================================================
            ##### Initialization and Configuration functions #####
 ===============================================================================
    [..]
    This subsection provides a set of functions allowing to initialize the UART.
      (+) For the UART only these parameters can be configured:
        (++) Baud Rate
        (++) Data Width
        (++) Stop Bit
        (++) Parity: If the parity is enabled, then the MSB bit of the data written
             in the data register is transmitted but is changed by the parity bit.
             Depending on the frame length defined by the DATLEN bit (5-bits ~ 8-bits),
             please refer to Reference manual for possible UART frame formats.
        (++) Receiver Timeout value
    [..]
@endverbatim
  * @{
  */

/**
  * @brief  Initializes the UART mode according to the specified parameters in
  *         the UART_InitTypeDef and create the associated handle.
  * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart)
{
    /* Check the UART handle allocation */
    if (huart == NULL) {
        return HAL_ERROR;
    }

    /* Check the parameters */
    if ((huart->Instance.Instance6_7 == UART6) || (huart->Instance.Instance6_7 == UART7)) {
        assert_param(IS_UART6_7_ALL_INSTANCE(huart->Instance.Instance6_7));
        huart->InstanceType = UART_INSTANCETYPE_6_7;
    } else {
        assert_param(IS_UART0_5_ALL_INSTANCE(huart->Instance.Instance0_5));
        huart->InstanceType = UART_INSTANCETYPE_0_5;
    }

    if (huart->gState == HAL_UART_STATE_RESET) {
        /* Allocate lock resource and initialize it */
        huart->Lock = HAL_UNLOCKED;

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
        UART_InitCallbacksToDefault(huart);

        if (huart->MspInitCallback == NULL) {
            huart->MspInitCallback = HAL_UART_MspInit;
        }

        /* Init the low level hardware */
        huart->MspInitCallback(huart);
#else
        /* Init the low level hardware : GPIO, CLOCK */
        HAL_UART_MspInit(huart);
#endif /* (USE_HAL_UART_REGISTER_CALLBACKS) */
    }

    huart->gState = HAL_UART_STATE_BUSY;

    /* Disable the peripheral */
    __HAL_UART_DISABLE(huart);

    /* Set the UART Communication parameters */
    UART_SetConfig(huart);

    /* Enable the peripheral */
    __HAL_UART_ENABLE(huart);

    /* Initialize the UART state */
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->gState = HAL_UART_STATE_READY;
    huart->RxState = HAL_UART_STATE_READY;
    huart->RxEventType = HAL_UART_RXEVENT_TC;

    return HAL_OK;
}

/**
  * @brief  DeInitializes the UART peripheral.
  * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef *huart)
{
    /* Check the UART handle allocation */
    if (huart == NULL)
    {
        return HAL_ERROR;
    }

    /* Check the parameters */
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        assert_param(IS_UART0_5_ALL_INSTANCE(huart->Instance.Instance0_5));
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        assert_param(IS_UART6_7_ALL_INSTANCE(huart->Instance.Instance6_7));
    } else {
        return HAL_ERROR;
    }

    huart->gState = HAL_UART_STATE_BUSY;

    /* Disable the Peripheral */
    __HAL_UART_DISABLE(huart);

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
    if (huart->MspDeInitCallback == NULL)
    {
        huart->MspDeInitCallback = HAL_UART_MspDeInit;
    }
    /* DeInit the low level hardware */
    huart->MspDeInitCallback(huart);
#else
    /* DeInit the low level hardware */
    HAL_UART_MspDeInit(huart);
#endif /* (USE_HAL_UART_REGISTER_CALLBACKS) */

    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->gState = HAL_UART_STATE_RESET;
    huart->RxState = HAL_UART_STATE_RESET;
    huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;
    huart->RxEventType = HAL_UART_RXEVENT_TC;

    /* Process Unlock */
    __HAL_UNLOCK(huart);

  return HAL_OK;
}

/**
 * @brief  UART MSP Init.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval None
 */
__WEAK void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);
    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_UART_MspInit could be implemented in the user file
    */
}

/**
  * @brief  UART MSP DeInit.
  * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
__WEAK void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);
    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_UART_MspDeInit could be implemented in the user file
    */
}

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
/**
 * @brief  Register a User UART Callback
 *         To be used instead of the weak predefined callback
 * @note   The HAL_UART_RegisterCallback() may be called before HAL_UART_Init(), HAL_HalfDuplex_Init(), HAL_LIN_Init(),
 *         HAL_MultiProcessor_Init() to register callbacks for HAL_UART_MSPINIT_CB_ID and HAL_UART_MSPDEINIT_CB_ID
 * @param  huart uart handle
 * @param  CallbackID ID of the callback to be registered
 *         This parameter can be one of the following values:
 *           @arg @ref HAL_UART_TX_HALFCOMPLETE_CB_ID Tx Half Complete Callback ID
 *           @arg @ref HAL_UART_TX_COMPLETE_CB_ID Tx Complete Callback ID
 *           @arg @ref HAL_UART_RX_HALFCOMPLETE_CB_ID Rx Half Complete Callback ID
 *           @arg @ref HAL_UART_RX_COMPLETE_CB_ID Rx Complete Callback ID
 *           @arg @ref HAL_UART_ERROR_CB_ID Error Callback ID
 *           @arg @ref HAL_UART_ABORT_COMPLETE_CB_ID Abort Complete Callback ID
 *           @arg @ref HAL_UART_ABORT_TRANSMIT_COMPLETE_CB_ID Abort Transmit Complete Callback ID
 *           @arg @ref HAL_UART_ABORT_RECEIVE_COMPLETE_CB_ID Abort Receive Complete Callback ID
 *           @arg @ref HAL_UART_MSPINIT_CB_ID MspInit Callback ID
 *           @arg @ref HAL_UART_MSPDEINIT_CB_ID MspDeInit Callback ID
 * @param  pCallback pointer to the Callback function
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_RegisterCallback(UART_HandleTypeDef *huart, HAL_UART_CallbackIDTypeDef CallbackID,
                                            pUART_CallbackTypeDef pCallback)
{
    HAL_StatusTypeDef status = HAL_OK;

    if (pCallback == NULL) {
        /* Update the error code */
        huart->ErrorCode |= HAL_UART_ERROR_INVALID_CALLBACK;

        return HAL_ERROR;
    }

    if (huart->gState == HAL_UART_STATE_READY) {
        switch (CallbackID)
        {
            case HAL_UART_TX_HALFCOMPLETE_CB_ID :
                huart->TxHalfCpltCallback = pCallback;
                break;

            case HAL_UART_TX_COMPLETE_CB_ID :
                huart->TxCpltCallback = pCallback;
                break;

            case HAL_UART_RX_HALFCOMPLETE_CB_ID :
                huart->RxHalfCpltCallback = pCallback;
                break;

            case HAL_UART_RX_COMPLETE_CB_ID :
                huart->RxCpltCallback = pCallback;
                break;

            case HAL_UART_ERROR_CB_ID :
                huart->ErrorCallback = pCallback;
                break;

            case HAL_UART_ABORT_COMPLETE_CB_ID :
                huart->AbortCpltCallback = pCallback;
                break;

            case HAL_UART_ABORT_TRANSMIT_COMPLETE_CB_ID :
                huart->AbortTransmitCpltCallback = pCallback;
                break;

            case HAL_UART_ABORT_RECEIVE_COMPLETE_CB_ID :
                huart->AbortReceiveCpltCallback = pCallback;
                break;

            case HAL_UART_MSPINIT_CB_ID :
                huart->MspInitCallback = pCallback;
                break;

            case HAL_UART_MSPDEINIT_CB_ID :
                huart->MspDeInitCallback = pCallback;
                break;

            default :
                /* Update the error code */
                huart->ErrorCode |= HAL_UART_ERROR_INVALID_CALLBACK;

                /* Return error status */
                status =  HAL_ERROR;
                break;
        }
    } else if (huart->gState == HAL_UART_STATE_RESET) {
        switch (CallbackID)
        {
            case HAL_UART_MSPINIT_CB_ID :
                huart->MspInitCallback = pCallback;
                break;

            case HAL_UART_MSPDEINIT_CB_ID :
                huart->MspDeInitCallback = pCallback;
                break;

            default :
                /* Update the error code */
                huart->ErrorCode |= HAL_UART_ERROR_INVALID_CALLBACK;

                /* Return error status */
                status =  HAL_ERROR;
                break;
        }
    } else {
        /* Update the error code */
        huart->ErrorCode |= HAL_UART_ERROR_INVALID_CALLBACK;

        /* Return error status */
        status =  HAL_ERROR;
    }

    return status;
}

/**
 * @brief  Unregister an UART Callback
 *         UART callaback is redirected to the weak predefined callback
 * @note   The HAL_UART_UnRegisterCallback() may be called before HAL_UART_Init(), HAL_HalfDuplex_Init(),
 *         HAL_LIN_Init(), HAL_MultiProcessor_Init() to un-register callbacks for HAL_UART_MSPINIT_CB_ID
 *         and HAL_UART_MSPDEINIT_CB_ID
 * @param  huart uart handle
 * @param  CallbackID ID of the callback to be unregistered
 *         This parameter can be one of the following values:
 *           @arg @ref HAL_UART_TX_HALFCOMPLETE_CB_ID Tx Half Complete Callback ID
 *           @arg @ref HAL_UART_TX_COMPLETE_CB_ID Tx Complete Callback ID
 *           @arg @ref HAL_UART_RX_HALFCOMPLETE_CB_ID Rx Half Complete Callback ID
 *           @arg @ref HAL_UART_RX_COMPLETE_CB_ID Rx Complete Callback ID
 *           @arg @ref HAL_UART_ERROR_CB_ID Error Callback ID
 *           @arg @ref HAL_UART_ABORT_COMPLETE_CB_ID Abort Complete Callback ID
 *           @arg @ref HAL_UART_ABORT_TRANSMIT_COMPLETE_CB_ID Abort Transmit Complete Callback ID
 *           @arg @ref HAL_UART_ABORT_RECEIVE_COMPLETE_CB_ID Abort Receive Complete Callback ID
 *           @arg @ref HAL_UART_MSPINIT_CB_ID MspInit Callback ID
 *           @arg @ref HAL_UART_MSPDEINIT_CB_ID MspDeInit Callback ID
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_UnRegisterCallback(UART_HandleTypeDef *huart, HAL_UART_CallbackIDTypeDef CallbackID)
{
    HAL_StatusTypeDef status = HAL_OK;

    if (HAL_UART_STATE_READY == huart->gState) {
        switch (CallbackID)
        {
            case HAL_UART_TX_HALFCOMPLETE_CB_ID :
                huart->TxHalfCpltCallback = HAL_UART_TxHalfCpltCallback;               /* Legacy weak  TxHalfCpltCallback       */
                break;

            case HAL_UART_TX_COMPLETE_CB_ID :
                huart->TxCpltCallback = HAL_UART_TxCpltCallback;                       /* Legacy weak TxCpltCallback            */
                break;

            case HAL_UART_RX_HALFCOMPLETE_CB_ID :
                huart->RxHalfCpltCallback = HAL_UART_RxHalfCpltCallback;               /* Legacy weak RxHalfCpltCallback        */
                break;

            case HAL_UART_RX_COMPLETE_CB_ID :
                huart->RxCpltCallback = HAL_UART_RxCpltCallback;                       /* Legacy weak RxCpltCallback            */
                break;

            case HAL_UART_ERROR_CB_ID :
                huart->ErrorCallback = HAL_UART_ErrorCallback;                         /* Legacy weak ErrorCallback             */
                break;

            case HAL_UART_ABORT_COMPLETE_CB_ID :
                huart->AbortCpltCallback = HAL_UART_AbortCpltCallback;                 /* Legacy weak AbortCpltCallback         */
                break;

            case HAL_UART_ABORT_TRANSMIT_COMPLETE_CB_ID :
                huart->AbortTransmitCpltCallback = HAL_UART_AbortTransmitCpltCallback; /* Legacy weak AbortTransmitCpltCallback */
                break;

            case HAL_UART_ABORT_RECEIVE_COMPLETE_CB_ID :
                huart->AbortReceiveCpltCallback = HAL_UART_AbortReceiveCpltCallback;   /* Legacy weak AbortReceiveCpltCallback  */
                break;

            case HAL_UART_MSPINIT_CB_ID :
                huart->MspInitCallback = HAL_UART_MspInit;                             /* Legacy weak MspInitCallback           */
                break;

            case HAL_UART_MSPDEINIT_CB_ID :
                huart->MspDeInitCallback = HAL_UART_MspDeInit;                         /* Legacy weak MspDeInitCallback         */
                break;

            default :
                /* Update the error code */
                huart->ErrorCode |= HAL_UART_ERROR_INVALID_CALLBACK;

                /* Return error status */
                status =  HAL_ERROR;
                break;
        }
    } else if (HAL_UART_STATE_RESET == huart->gState) {
        switch (CallbackID)
        {
            case HAL_UART_MSPINIT_CB_ID :
                huart->MspInitCallback = HAL_UART_MspInit;
                break;

            case HAL_UART_MSPDEINIT_CB_ID :
                huart->MspDeInitCallback = HAL_UART_MspDeInit;
                break;

            default :
                /* Update the error code */
                huart->ErrorCode |= HAL_UART_ERROR_INVALID_CALLBACK;

                /* Return error status */
                status =  HAL_ERROR;
                break;
        }
    } else {
        /* Update the error code */
        huart->ErrorCode |= HAL_UART_ERROR_INVALID_CALLBACK;

        /* Return error status */
        status =  HAL_ERROR;
    }

    return status;
}


/**
 * @brief  Register a User UART Rx Event Callback
 *         To be used instead of the weak predefined callback
 * @param  huart     Uart handle
 * @param  pCallback Pointer to the Rx Event Callback function
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_RegisterRxEventCallback(UART_HandleTypeDef *huart, pUART_RxEventCallbackTypeDef pCallback)
{
    HAL_StatusTypeDef status = HAL_OK;

    if (pCallback == NULL) {
        huart->ErrorCode |= HAL_UART_ERROR_INVALID_CALLBACK;

        return HAL_ERROR;
    }

    /* Process locked */
    __HAL_LOCK(huart);

    if (huart->gState == HAL_UART_STATE_READY) {
        huart->RxEventCallback = pCallback;
    } else {
        huart->ErrorCode |= HAL_UART_ERROR_INVALID_CALLBACK;

        status =  HAL_ERROR;
    }

    /* Release Lock */
    __HAL_UNLOCK(huart);

    return status;
}

/**
 * @brief  UnRegister the UART Rx Event Callback
 *         UART Rx Event Callback is redirected to the weak HAL_UARTEx_RxEventCallback() predefined callback
 * @param  huart     Uart handle
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_UnRegisterRxEventCallback(UART_HandleTypeDef *huart)
{
    HAL_StatusTypeDef status = HAL_OK;

    /* Process locked */
    __HAL_LOCK(huart);

    if (huart->gState == HAL_UART_STATE_READY) {
        huart->RxEventCallback = HAL_UARTEx_RxEventCallback; /* Legacy weak UART Rx Event Callback  */
    } else {
        huart->ErrorCode |= HAL_UART_ERROR_INVALID_CALLBACK;

        status =  HAL_ERROR;
    }

    /* Release Lock */
    __HAL_UNLOCK(huart);
    return status;
}
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
/**
 * @}
 */

/** @defgroup UART_Exported_Functions_Group2 IO operation functions
 *  @brief UART Transmit and Receive functions
 *
@verbatim
 ===============================================================================
                      ##### IO operation functions #####
 ===============================================================================
    This subsection provides a set of functions allowing to manage the UART asynchronous data transfers.

    (#) There are two modes of transfer:
       (+) Blocking mode: The communication is performed in polling mode.
           The HAL status of all data processing is returned by the same function
           after finishing transfer.
       (+) Non-Blocking mode: The communication is performed using Interrupts
           or DMA, these API's return the HAL status.
           The end of the data processing will be indicated through the
           dedicated UART IRQ when using Interrupt mode or the DMA IRQ when
           using DMA mode.
           The HAL_UART_TxCpltCallback(), HAL_UART_RxCpltCallback() user callbacks
           will be executed respectively at the end of the transmit or receive process
           The HAL_UART_ErrorCallback()user callback will be executed when a communication error is detected.

    (#) Blocking mode API's are :
        (+) HAL_UART_Transmit()
        (+) HAL_UART_Receive()

    (#) Non-Blocking mode API's with Interrupt are :
        (+) HAL_UART_Transmit_IT()
        (+) HAL_UART_Receive_IT()
        (+) HAL_UART_IRQHandler()

    (#) Non-Blocking mode API's with DMA are :
        (+) HAL_UART_Transmit_DMA()
        (+) HAL_UART_Receive_DMA()
        (+) HAL_UART_DMAStop()

    (#) A set of Transfer Complete Callbacks are provided in Non_Blocking mode:
        (+) HAL_UART_TxHalfCpltCallback()
        (+) HAL_UART_TxCpltCallback()
        (+) HAL_UART_RxHalfCpltCallback()
        (+) HAL_UART_RxCpltCallback()
        (+) HAL_UART_ErrorCallback()

    (#) Non-Blocking mode transfers could be aborted using Abort API's :
        (+) HAL_UART_Abort()
        (+) HAL_UART_AbortTransmit()
        (+) HAL_UART_AbortReceive()
        (+) HAL_UART_Abort_IT()
        (+) HAL_UART_AbortTransmit_IT()
        (+) HAL_UART_AbortReceive_IT()

    (#) For Abort services based on interrupts (HAL_UART_Abortxxx_IT), a set of Abort Complete Callbacks are provided:
        (+) HAL_UART_AbortCpltCallback()
        (+) HAL_UART_AbortTransmitCpltCallback()
        (+) HAL_UART_AbortReceiveCpltCallback()

    (#) A Rx Event Reception Callback (Rx event notification) is available for Non_Blocking modes of enhanced reception services:
        (+) HAL_UARTEx_RxEventCallback()

    (#) In Non-Blocking mode transfers, possible errors are split into 2 categories.
        Errors are handled as follows :
       (+) Error is considered as Recoverable and non blocking : Transfer could go till end, but error severity is
           to be evaluated by user : this concerns Frame Error, Parity Error or Noise Error in Interrupt mode reception .
           Received character is then retrieved and stored in Rx buffer, Error code is set to allow user to identify error type,
           and HAL_UART_ErrorCallback() user callback is executed. Transfer is kept ongoing on UART side.
           If user wants to abort it, Abort services should be called by user.
       (+) Error is considered as Blocking : Transfer could not be completed properly and is aborted.
           This concerns Overrun Error In Interrupt mode reception and all errors in DMA mode.
           Error code is set to allow user to identify error type, and HAL_UART_ErrorCallback() user callback is executed.

@endverbatim
 * @{
 */

/**
 * @brief  Sends an amount of data in blocking mode.
 * @note   When UART parity is USER_DEFINED Parity, the sent data is handled as a set of u16.
 *          In this case, Size must indicate the number of u16 provided through pData.
 * @param  huart Pointer to a UART_HandleTypeDef structure that contains
 *               the configuration information for the specified UART module.
 * @param  pData Pointer to data buffer (u8 or u16 data elements).
 * @param  Size  Amount of data elements (u8 or u16) to be sent
 * @param  Timeout Timeout duration
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    const uint8_t  *pdata8bits;
    const uint16_t *pdata16bits;
    uint32_t tickstart = 0U;

    /* Check that a Tx process is not already ongoing */
    if (huart->gState == HAL_UART_STATE_READY) {
        if ((pData == NULL) || (Size == 0U)) {
            return  HAL_ERROR;
        }

        huart->ErrorCode = HAL_UART_ERROR_NONE;
        huart->gState = HAL_UART_STATE_BUSY_TX;

        /* Init tickstart for timeout management */
        tickstart = HAL_GetMillis();

        huart->TxXferSize = Size;
        huart->TxXferCount = Size;

        /* In case of USER_DEFINED Parity transfer, pData needs to be handled as a uint16_t pointer */
        if (huart->Init.Parity == UART_PARITY_USER_DEFINED) {
            pdata8bits  = NULL;
            pdata16bits = (const uint16_t *) pData;
        } else {
            pdata8bits  = pData;
            pdata16bits = NULL;
        }

        while (huart->TxXferCount > 0U)
        {
            if (UART_WaitOnFlagUntilTimeout(huart, UART_FLAG_FIFOF, SET, tickstart, Timeout) != HAL_OK) {
                huart->gState = HAL_UART_STATE_READY;

                return HAL_TIMEOUT;
            }
            if (pdata8bits == NULL) {
                if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                    LL_UART0_5_TransmitData9(huart->Instance.Instance0_5, (uint16_t)*pdata16bits);
                } else {
                    LL_UART6_7_TransmitData9(huart->Instance.Instance6_7, (uint16_t)*pdata16bits);
                }
                pdata16bits++;
            } else {
                if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                    LL_UART0_5_TransmitData8(huart->Instance.Instance0_5, (uint8_t)*pdata8bits);
                } else {
                    LL_UART6_7_TransmitData8(huart->Instance.Instance6_7, (uint8_t)*pdata8bits);
                }
                pdata8bits++;
            }
            huart->TxXferCount--;
        }
        if ((UART_WaitOnFlagUntilTimeout(huart, UART_FLAG_FIFOE, RESET, tickstart, Timeout) != HAL_OK) ||
                (UART_WaitOnFlagUntilTimeout(huart, UART_FLAG_TB, SET, tickstart, Timeout) != HAL_OK)) {
            huart->gState = HAL_UART_STATE_READY;

            return HAL_TIMEOUT;
        }

        /* At end of Tx process, restore huart->gState to Ready */
        huart->gState = HAL_UART_STATE_READY;

        return HAL_OK;
    } else {
        return HAL_BUSY;
    }
}

/**
 * @brief  Receives an amount of data in blocking mode.
 * @note   When UART parity is USER_DEFINED Parity, the received data is handled as a set of u16.
 *         In this case, Size must indicate the number of u16 available through pData.
 * @param  huart Pointer to a UART_HandleTypeDef structure that contains
 *               the configuration information for the specified UART module.
 * @param  pData Pointer to data buffer (u8 or u16 data elements).
 * @param  Size  Amount of data elements (u8 or u16) to be received.
 * @param  Timeout Timeout duration
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    uint8_t  *pdata8bits;
    uint16_t *pdata16bits;
    uint32_t tickstart = 0U;

    /* Check that a Rx process is not already ongoing */
    if (huart->RxState == HAL_UART_STATE_READY) {
        if ((pData == NULL) || (Size == 0U)) {
            return  HAL_ERROR;
        }

        huart->ErrorCode = HAL_UART_ERROR_NONE;
        huart->RxState = HAL_UART_STATE_BUSY_RX;
        huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

        /* Init tickstart for timeout management */
        tickstart = HAL_GetMillis();

        huart->RxXferSize = Size;
        huart->RxXferCount = Size;

        /* In case of USER_DEFINED Parity transfer, pRxData needs to be handled as a uint16_t pointer */
        if (huart->Init.Parity == UART_PARITY_USER_DEFINED) {
            pdata8bits  = NULL;
            pdata16bits = (uint16_t *) pData;
        } else {
            pdata8bits  = pData;
            pdata16bits = NULL;
        }

        /* Check the remain data to be received */
        while (huart->RxXferCount > 0U)
        {
            if (UART_WaitOnFlagUntilTimeout(huart, UART_FLAG_RX, RESET, tickstart, Timeout) != HAL_OK) {
                huart->RxState = HAL_UART_STATE_READY;

                return HAL_TIMEOUT;
            }
            if (pdata8bits == NULL) {
                /* RX flag can clear by read RXDATA register */
                if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                    *pdata16bits = (uint16_t)(LL_UART0_5_ReceiveData9(huart->Instance.Instance0_5) & 0x01FF);
                } else {
                    *pdata16bits = (uint16_t)(LL_UART6_7_ReceiveData9(huart->Instance.Instance6_7) & 0x01FF);
                }
                pdata16bits++;
            } else {
                /* RX flag can clear by read RXDATA register */
                if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                    *pdata8bits = (uint8_t)LL_UART0_5_ReceiveData8(huart->Instance.Instance0_5);
                } else {
                    *pdata8bits = (uint8_t)LL_UART6_7_ReceiveData8(huart->Instance.Instance6_7);
                }
                pdata8bits++;
            }
            huart->RxXferCount--;
            /* RX flag can also clear by read RXDATA register */
            __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_RX);
        }

        /* At end of Rx process, restore huart->RxState to Ready */
        huart->RxState = HAL_UART_STATE_READY;

        return HAL_OK;
    } else {
        return HAL_BUSY;
    }
}

/**
 * @brief Send an amount of data in non blocking mode (interrupt mode).
 * @note   When UART parity is USER_DEFINED Parity, the sent data is handled as a set of u16.
 *          In this case, Size must indicate the number of u16 provided through pData.
 * @param huart UART handle.
 * @param pData Pointer to data buffer (u8 or u16 data elements).
 * @param Size  Amount of data elements (u8 or u16) to be sent.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size)
{
    const uint16_t *tmp;

    /* Check that a Tx process is not already ongoing */
    if (huart->gState == HAL_UART_STATE_READY) {
        if ((pData == NULL) || (Size == 0U)) {
            return HAL_ERROR;
        }
        /* clear TX flag */
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_TX);

        huart->pTxBuffPtr = pData;
        huart->TxXferSize = Size;
        huart->TxXferCount = Size;

        huart->ErrorCode = HAL_UART_ERROR_NONE;
        huart->gState = HAL_UART_STATE_BUSY_TX;

        /* Send first data to trigger TX interrupt */
        if (huart->Init.Parity == UART_PARITY_USER_DEFINED) {
            tmp = (const uint16_t *) huart->pTxBuffPtr;
            if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                LL_UART0_5_TransmitData9(huart->Instance.Instance0_5, (uint16_t)(*tmp & (uint16_t)0x01FF));
            } else {
                LL_UART6_7_TransmitData9(huart->Instance.Instance6_7, (uint16_t)(*tmp & (uint16_t)0x01FF));
            }
            huart->pTxBuffPtr += 2U;
        } else {
            if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                LL_UART0_5_TransmitData8(huart->Instance.Instance0_5, (uint8_t)(*huart->pTxBuffPtr++ & (uint8_t)0x00FF));
            } else {
                LL_UART6_7_TransmitData8(huart->Instance.Instance6_7, (uint8_t)(*huart->pTxBuffPtr++ & (uint8_t)0x00FF));
            }
        }
        --huart->TxXferCount;
        /* Enable the UART Transmit Interrupt */
        __HAL_UART_ENABLE_IT(huart, UART_IT_TX);
        return HAL_OK;
    } else {
        return HAL_BUSY;
    }
}

/**
 * @brief  Receives an amount of data in non blocking mode (interrupt mode).
 * @note   When UART parity is USER_DEFINED Parity, the received data is handled as a set of u16.
 *          In this case, Size must indicate the number of u16 available through pData.
 * @param  huart Pointer to a UART_HandleTypeDef structure that contains
 *               the configuration information for the specified UART module.
 * @param  pData Pointer to data buffer (u8 or u16 data elements).
 * @param  Size  Amount of data elements (u8 or u16) to be received.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    /* Check that a Rx process is not already ongoing */
    if (huart->RxState == HAL_UART_STATE_READY) {
        if ((pData == NULL) || (Size == 0U)) {
            return HAL_ERROR;
        }

        /* Set Reception type to Standard reception */
        huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

        return (UART_Start_Receive_IT(huart, pData, Size));
    } else {
        return HAL_BUSY;
    }
}

/**
 * @brief  Sends an amount of data in DMA mode.
 * @note   When UART parity is UART_PARITY_USER_DEFINED, the sent data is handled as a set of u16.
 *          In this case, Size must indicate the number of u16 provided through pData.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @param  pData Pointer to data buffer (u8 or u16 data elements).
 * @param  Size  Amount of data elements (u8 or u16) to be sent
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size)
{
    const uint32_t *tmp;

    /* Check that a Tx process is not already ongoing */
    if (huart->gState == HAL_UART_STATE_READY) {
        if ((pData == NULL) || (Size == 0U)) {
            return HAL_ERROR;
        }

        huart->pTxBuffPtr = pData;
        huart->TxXferSize = Size;
        huart->TxXferCount = Size;

        huart->ErrorCode = HAL_UART_ERROR_NONE;
        huart->gState = HAL_UART_STATE_BUSY_TX;

        /* Check the parameters */
        assert_param(IS_UART_DMA_MODE(huart->hdmatx.Mode));
        if (huart->Init.Parity == UART_PARITY_USER_DEFINED) {
            assert_param(IS_UART_DMA_PARITY(huart->hdmatx.Parity));
        }
        /* Initialize the UART DMA */
        __HAL_UART_DISABLE(huart);

        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            /* If UART DMA is in Circular mode */
            if (huart->hdmatx.Mode == UART_DMA_CIRCULAR) {
                LL_UART0_5_DMA_EnableTxCircularMode(huart->Instance.Instance0_5);
            } else {
                LL_UART0_5_DMA_DisableTxCircularMode(huart->Instance.Instance0_5);
            }
            /* If UART DMA is UART_PARITY_USER_DEFINED */
            if (huart->Init.Parity == UART_PARITY_USER_DEFINED) {
                LL_UART0_5_DMA_SetParity(huart->Instance.Instance0_5, huart->hdmatx.Parity);
            }
        } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
            /* If UART DMA is in Circular mode */
            if (huart->hdmatx.Mode == UART_DMA_CIRCULAR) {
                LL_UART6_7_DMA_EnableTxCircularMode(huart->Instance.Instance6_7);
            } else {
                LL_UART6_7_DMA_DisableTxCircularMode(huart->Instance.Instance6_7);
            }
            /* If UART DMA is UART_PARITY_USER_DEFINED */
            if (huart->Init.Parity == UART_PARITY_USER_DEFINED) {
                LL_UART6_7_DMA_SetParity(huart->Instance.Instance6_7, huart->hdmatx.Parity);
            }
        } else {
            return HAL_ERROR;
        }

        /* Link DMA */
        huart->hdmatx.Parent = huart;

        /* Set the UART DMA transfer complete callback */
        huart->hdmatx.XferCpltCallback = UART_DMATransmitCplt;

        /* Set the UART DMA Half transfer complete callback */
        huart->hdmatx.XferHalfCpltCallback = UART_DMATxHalfCplt;

        /* Set the DMA error callback */
        huart->hdmatx.XferErrorCallback = UART_DMAError;

        /* Set the DMA abort callback */
        huart->hdmatx.XferAbortCallback = NULL;

        /* Enable the UART transmit DMA stream */
        tmp = (const uint32_t *)&pData;
        if (UART_DMA_Start_TX_IT(huart, *(const uint32_t *)tmp, Size) != HAL_OK) {
            /* Set error code to DMA */
            huart->ErrorCode = HAL_UART_ERROR_DMA;

            /* Restore huart->gState to ready */
            huart->gState = HAL_UART_STATE_READY;

            return HAL_ERROR;
        }
        /* Clear the TC flag */
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_TX);

        /* Enable the DMA transfer for transmit request by enabling UART */
        __HAL_UART_ENABLE(huart);

        return HAL_OK;
    } else {
        return HAL_BUSY;
    }
}

/**
 * @brief  Receives an amount of data in DMA mode.
 * @note   When UART parity is UART_PARITY_USER_DEFINED, the sent data is handled as a set of u16.
 *          In this case, Size must indicate the number of u16 provided through pData.
 * @param  huart Pointer to a UART_HandleTypeDef structure that contains
 *               the configuration information for the specified UART module.
 * @param  pData Pointer to data buffer (u8 or u16 data elements).
 * @param  Size  Amount of data elements (u8 or u16) to be received.
 * @note   When the UART parity is enabled (PCE = 1) the received data contains the parity bit.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    /* Check that a Rx process is not already ongoing */
    if (huart->RxState == HAL_UART_STATE_READY) {
        if ((pData == NULL) || (Size == 0U)) {
            return HAL_ERROR;
        }

        /* Check the parameters */
        assert_param(IS_UART_DMA_MODE(huart->hdmatx.Mode));
        if (huart->Init.Parity == UART_PARITY_USER_DEFINED) {
            assert_param(IS_UART_DMA_PARITY(huart->hdmatx.Parity));
        }
        /* Initialize the UART DMA */
        __HAL_UART_DISABLE(huart);

        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            /* If UART DMA is in Circular mode */
            if (huart->hdmatx.Mode == UART_DMA_CIRCULAR) {
                LL_UART0_5_DMA_EnableTxCircularMode(huart->Instance.Instance0_5);
            } else {
                LL_UART0_5_DMA_DisableTxCircularMode(huart->Instance.Instance0_5);
            }
            /* If UART DMA is UART_PARITY_USER_DEFINED */
            if (huart->Init.Parity == UART_PARITY_USER_DEFINED) {
                LL_UART0_5_DMA_SetParity(huart->Instance.Instance0_5, huart->hdmatx.Parity);
            }
        } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
            /* If UART DMA is in Circular mode */
            if (huart->hdmatx.Mode == UART_DMA_CIRCULAR) {
                LL_UART6_7_DMA_EnableTxCircularMode(huart->Instance.Instance6_7);
            } else {
                LL_UART6_7_DMA_DisableTxCircularMode(huart->Instance.Instance6_7);
            }
            /* If UART DMA is UART_PARITY_USER_DEFINED */
            if (huart->Init.Parity == UART_PARITY_USER_DEFINED) {
                LL_UART6_7_DMA_SetParity(huart->Instance.Instance6_7, huart->hdmatx.Parity);
            }
        } else {
            return HAL_ERROR;
        }
        /* Link DMA */
        huart->hdmarx.Parent = huart;

        /* Set Reception type to Standard reception */
        huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

        if (UART_Start_Receive_DMA(huart, pData, Size) != HAL_OK) {
            return HAL_ERROR;
        }
         /* Enable the DMA transfer for transmit request by enabling UART */
         __HAL_UART_ENABLE(huart);

        return HAL_OK;
    } else {
        return HAL_BUSY;
    }
}

/**
  * @brief Stops the DMA Transfer.
  * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_UART_DMAStop(UART_HandleTypeDef *huart)
{
    uint32_t dmarequest = 0x00U;
    /* The Lock is not implemented on this API to allow the user application
        to call the HAL UART API under callbacks HAL_UART_TxCpltCallback() / HAL_UART_RxCpltCallback():
        when the DMA TX/RX Transfer complete interrupt is generated and the correspond call back is executed
        HAL_UART_TxCpltCallback() / HAL_UART_RxCpltCallback()
        */

    /* Stop UART DMA Tx request if ongoing */
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        dmarequest = LL_UART0_5_IsEnabledDMAReq_TX(huart->Instance.Instance0_5);
    } else {
        dmarequest = LL_UART6_7_IsEnabledDMAReq_TX(huart->Instance.Instance6_7);
    }
    if ((huart->gState == HAL_UART_STATE_BUSY_TX) && dmarequest) {
        /* Abort the UART DMA */
        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            /* Disable UART DMA IT */
            __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_TXF | UART_DMA_IT_TXH | UART_DMA_IT_TXE);

            LL_UART0_5_DisableDMAReq_TX(huart->Instance.Instance0_5);

            /* Clear all flags */
            __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_TXF | UART_DMA_FLAG_TXH);
        } else {
            /* Disable UART DMA IT */
            __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_TXF | UART_DMA_IT_TXH | UART_DMA_IT_TXE);

            LL_UART6_7_DisableDMAReq_TX(huart->Instance.Instance6_7);

            /* Clear all flags */
            __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_TXF | UART_DMA_FLAG_TXH);
        }

        UART_EndTxTransfer(huart);
    }

    /* Stop UART DMA Rx request if ongoing */
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        dmarequest = LL_UART0_5_IsEnabledDMAReq_RX(huart->Instance.Instance0_5);
    } else {
        dmarequest = LL_UART6_7_IsEnabledDMAReq_RX(huart->Instance.Instance6_7);
    }

    if ((huart->RxState == HAL_UART_STATE_BUSY_RX) && dmarequest)
    {
        /* Abort the UART DMA */
        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            LL_UART0_5_DisableDMAReq_RX(huart->Instance.Instance0_5);
            /* Disable UART DMA IT */
            __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_RXF | UART_DMA_IT_RXH | UART_DMA_IT_RXE);

            /* Clear all flags */
            __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_RXF | UART_DMA_FLAG_RXH | UART_DMA_IT_RXE | UART_DMA_IT_RXDONE);
            // LL_UART0_5_DMA_ClearFlag_RXHALF(huart->Instance.Instance0_5);
            // LL_UART0_5_DMA_ClearFlag_RXFULL(huart->Instance.Instance0_5);
        } else {
            LL_UART6_7_DisableDMAReq_RX(huart->Instance.Instance6_7);
            /* Disable UART DMA IT */
            __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_RXF | UART_DMA_IT_RXH | UART_DMA_IT_RXE);

            /* Clear all flags */
            __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_RXF | UART_DMA_FLAG_RXH | UART_DMA_IT_RXE | UART_DMA_IT_RXDONE);
        }

        UART_EndRxTransfer(huart);
    }

    return HAL_OK;
}

/**
 * @brief Receive an amount of data in blocking mode till either the expected number of data is received or an IDLE event occurs.
 * @note   HAL_OK is returned if reception is completed (expected number of data has been received)
 *         or if reception is stopped after IDLE event (less than the expected number of data has been received)
 *         In this case, RxLen output parameter indicates number of data available in reception buffer.
 * @note   When UART parity is UART_PARITY_USER_DEFINED, the received data is handled as a set of uint16_t.
 *          In this case, Size must indicate the number of uint16_t available through pData.
 * @param huart   UART handle.
 * @param pData   Pointer to data buffer (uint8_t or uint16_t data elements).
 * @param Size    Amount of data elements (uint8_t or uint16_t) to be received.
 * @param RxLen   Number of data elements finally received (could be lower than Size, in case reception ends on IDLE event)
 * @param Timeout Timeout duration expressed in ms (covers the whole reception sequence).
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint16_t *RxLen, uint32_t Timeout)
{
    uint8_t  *pdata8bits;
    uint16_t *pdata16bits;
    uint32_t tickstart;

    /* Check that a Rx process is not already ongoing */
    if (huart->RxState == HAL_UART_STATE_READY) {
        if ((pData == NULL) || (Size == 0U)) {
            return  HAL_ERROR;
        }

        huart->ErrorCode = HAL_UART_ERROR_NONE;
        huart->RxState = HAL_UART_STATE_BUSY_RX;
        huart->ReceptionType = HAL_UART_RECEPTION_TOIDLE;
        huart->RxEventType = HAL_UART_RXEVENT_TC;

        /* Init tickstart for timeout management */
        tickstart = HAL_GetMillis();

        huart->RxXferSize  = Size;
        huart->RxXferCount = Size;

        /* In case of USER_DEFINED Parity transfer, pData needs to be handled as a uint16_t pointer */
        if (huart->Init.Parity == UART_PARITY_USER_DEFINED) {
            pdata8bits  = NULL;
            pdata16bits = (uint16_t *) pData;
        } else {
            pdata8bits  = pData;
            pdata16bits = NULL;
        }

       /* Initialize output number of received elements */
       *RxLen = 0U;

       /* as long as data have to be received */
       while (huart->RxXferCount > 0U) {
           /* Check if IDLE(RXTO) flag is set */
           if (__HAL_UART_GET_FLAG(huart, UART_FLAG_RXTO)) {
               /* Clear IDLE flag in ISR */
               __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_RXTO);

               /* If Set, but no data ever received, clear flag without exiting loop */
               /* If Set, and data has already been received, this means Idle Event is valid : End reception */
               if (*RxLen > 0U) {
                    huart->RxEventType = HAL_UART_RXEVENT_IDLE;
                    huart->RxState = HAL_UART_STATE_READY;

                    return HAL_OK;
               }
           }

           /* Check if RX flag is set */
           if (__HAL_UART_GET_FLAG(huart, UART_FLAG_RX)) {
                /* Clear RX flag in ISR */
                __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_RX);

                if (pdata8bits == NULL) {
                    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                        *pdata16bits = LL_UART0_5_ReceiveData9(huart->Instance.Instance0_5) & 0x01FF;
                    } else {
                        *pdata16bits = LL_UART6_7_ReceiveData9(huart->Instance.Instance6_7) & 0x01FF;
                    }

                    pdata16bits++;
                } else {
                    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                        *pdata8bits = LL_UART0_5_ReceiveData8(huart->Instance.Instance0_5);
                    } else {
                        *pdata8bits = LL_UART6_7_ReceiveData8(huart->Instance.Instance6_7);
                    }

                    pdata8bits++;
                }
                /* Increment number of received elements */
                *RxLen += 1U;
                huart->RxXferCount--;
           }

           /* Check for the Timeout */
           if (Timeout != HAL_MAX_DELAY) {
                if (((HAL_GetMillis() - tickstart) > Timeout) || (Timeout == 0U)) {
                    huart->RxState = HAL_UART_STATE_READY;

                    return HAL_TIMEOUT;
                }
           }
       }

        /* Set number of received elements in output parameter : RxLen */
        *RxLen = huart->RxXferSize - huart->RxXferCount;
        /* At end of Rx process, restore huart->RxState to Ready */
        huart->RxState = HAL_UART_STATE_READY;

        return HAL_OK;
    } else {
        return HAL_BUSY;
    }
}

/**
 * @brief  Receive an amount of data in interrupt mode till either the expected number of data is received or an IDLE event occurs.
 * @note   Reception is initiated by this function call. Further progress of reception is achieved thanks
 *         to UART interrupts raised by RX and IDLE events. Callback is called at end of reception indicating
 *         number of received data elements.
 * @note   When UART parity is UART_PARITY_USER_DEFINED, the received data is handled as a set of uint16_t.
 *         In this case, Size must indicate the number of uint16_t available through pData.
 * @param huart UART handle.
 * @param pData Pointer to data buffer (uint8_t or uint16_t data elements).
 * @param Size  Amount of data elements (uint8_t or uint16_t) to be received.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    HAL_StatusTypeDef status;

    /* Check that a Rx process is not already ongoing */
    if (huart->RxState == HAL_UART_STATE_READY) {
        if ((pData == NULL) || (Size == 0U)) {
            return HAL_ERROR;
        }

        /* Set Reception type to reception till IDLE Event*/
        huart->ReceptionType = HAL_UART_RECEPTION_TOIDLE;
        huart->RxEventType = HAL_UART_RXEVENT_TC;

        status =  UART_Start_Receive_IT(huart, pData, Size);

        /* Check Rx process has been successfully started */
        if (status == HAL_OK) {
            if (huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE) {
                __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_RXTO);

                /* enable IDLE(RX_TO_IE) Interrupt */
                __HAL_UART_ENABLE_IT(huart, UART_IT_RXTO);
            } else {
                /* In case of errors already pending when reception is started,
                Interrupts may have already been raised and lead to reception abortion.
                (Overrun error for instance).
                In such case Reception Type has been reset to HAL_UART_RECEPTION_STANDARD. */
                status = HAL_ERROR;
            }
        }

        return status;
    } else {
        return HAL_BUSY;
    }
}

/**
  * @brief Receive an amount of data in DMA mode till either the expected number of data is received or an IDLE event occurs.
  * @note   Reception is initiated by this function call. Further progress of reception is achieved thanks
  *         to DMA services, transferring automatically received data elements in user reception buffer and
  *         calling registered callbacks at half/end of reception. UART IDLE events are also used to consider
  *         reception phase as ended. In all cases, callback execution will indicate number of received data elements.
  * @note   When the UART parity is enabled (PCE = 1), the received data contain
  *         the parity bit (MSB position).
  * @note   When UART parity is not enabled (PCE = 0), and Word Length is configured to 9 bits (M = 01),
  *         the received data is handled as a set of uint16_t. In this case, Size must indicate the number
  *         of uint16_t available through pData.
  * @param huart UART handle.
  * @param pData Pointer to data buffer (uint8_t or uint16_t data elements).
  * @param Size  Amount of data elements (uint8_t or uint16_t) to be received.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    HAL_StatusTypeDef status;

    /* Check that a Rx process is not already ongoing */
    if (huart->RxState == HAL_UART_STATE_READY) {
        if ((pData == NULL) || (Size == 0U)) {
            return HAL_ERROR;
        }

        /* Set Reception type to reception till IDLE Event*/
        huart->ReceptionType = HAL_UART_RECEPTION_TOIDLE;
        huart->RxEventType = HAL_UART_RXEVENT_TC;
        /* Initialize the UART DMA */
        __HAL_UART_DISABLE(huart);

        status =  UART_Start_Receive_DMA(huart, pData, Size);

        /* Check Rx process has been successfully started */
        if (huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE) {
            __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_RXTO);

            /* enable IDLE(RX_TO_IE) Interrupt */
            __HAL_UART_ENABLE_IT(huart, UART_IT_RXTO);
        } else {
            /* In case of errors already pending when reception is started,
                Interrupts may have already been raised and lead to reception abortion.
                (Overrun error for instance).
                In such case Reception Type has been reset to HAL_UART_RECEPTION_STANDARD. */
            status = HAL_ERROR;
        }
        /* Enable the DMA transfer for transmit request by enabling UART */
        __HAL_UART_ENABLE(huart);

        return status;
    } else {
        return HAL_BUSY;
    }
}

/**
 * @brief Provide Rx Event type that has lead to RxEvent callback execution.
 * @note  When HAL_UARTEx_ReceiveToIdle_IT() or HAL_UARTEx_ReceiveToIdle_DMA() API are called, progress
 *        of reception process is provided to application through calls of Rx Event callback (either default one
 *        HAL_UARTEx_RxEventCallback() or user registered one). As several types of events could occur (IDLE event,
 *        Half Transfer, or Transfer Complete), this function allows to retrieve the Rx Event type that has lead
 *        to Rx Event callback execution.
 * @note  This function is expected to be called within the user implementation of Rx Event Callback,
 *        in order to provide the accurate value :
 *        In Interrupt Mode :
 *           - HAL_UART_RXEVENT_TC : when Reception has been completed (expected nb of data has been received)
 *           - HAL_UART_RXEVENT_IDLE : when Idle event occurred prior reception has been completed (nb of
 *             received data is lower than expected one)
 *        In DMA Mode :
 *           - HAL_UART_RXEVENT_TC : when Reception has been completed (expected nb of data has been received)
 *           - HAL_UART_RXEVENT_HT : when half of expected nb of data has been received
 *           - HAL_UART_RXEVENT_IDLE : when Idle event occurred prior reception has been completed (nb of
 *             received data is lower than expected one).
 *        In DMA mode, RxEvent callback could be called several times;
 *        When DMA is configured in Normal Mode, HT event does not stop Reception process;
 *        When DMA is configured in Circular Mode, HT, TC or IDLE events don't stop Reception process;
 * @param  huart UART handle.
 * @retval Rx Event Type (returned value will be a value of @ref UART_RxEvent_Type_Values)
 */
HAL_UART_RxEventTypeTypeDef HAL_UARTEx_GetRxEventType(UART_HandleTypeDef *huart)
{
    /* Return Rx Event type value, as stored in UART handle */
    return(huart->RxEventType);
}

/**
 * @brief  Abort ongoing transfers (blocking mode).
 * @param  huart UART handle.
 * @note   This procedure could be used for aborting any ongoing transfer started in Interrupt or DMA mode.
 *         This procedure performs following operations :
 *           - Disable UART Interrupts (Tx and Rx)
 *           - Disable the DMA transfer in the peripheral register (if enabled)
 *           - Abort DMA transfer by setting TX_DMA_EN/RX_DMA_EN to 0 (in case of transfer in DMA mode)
 *           - Set handle State to READY
 * @note   This procedure is executed in blocking mode : when exiting function, Abort is considered as completed.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_Abort(UART_HandleTypeDef *huart)
{
    uint32_t dmarequest = 0x00U;

    /* Disable TXIE, RXIE, ERRIE(Frame error, parity error, overrun error) interrupts */
    __HAL_UART_DISABLE_IT(huart, UART_IT_TX);
    __HAL_UART_DISABLE_IT(huart, UART_IT_RX);
    __HAL_UART_DISABLE_IT(huart, UART_IT_ERR);

    /* If Reception till IDLE(RXTO) event was ongoing, disable IDLEIE(RX_TO_IE) interrupt */
    if (huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE) {
        /* Disable IDLE(RX_TO_IE) Interrupt */
        __HAL_UART_DISABLE_IT(huart, UART_IT_RXTO);
    }

    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        dmarequest = LL_UART0_5_IsEnabledDMAReq_TX(huart->Instance.Instance0_5);
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        dmarequest = LL_UART6_7_IsEnabledDMAReq_TX(huart->Instance.Instance6_7);
    } else {
        return HAL_ERROR;
    }
    /* Disable the UART DMA Tx request if enabled */
    if (dmarequest) {
        /* Abort the UART DMA */
        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            LL_UART0_5_DisableDMAReq_TX(huart->Instance.Instance0_5);
        } else {
            LL_UART6_7_DisableDMAReq_TX(huart->Instance.Instance6_7);
        }
        /* Abort the UART DMA Tx channel: use blocking DMA Abort API (no callback) */
        /* Set the UART DMA Abort callback to Null.
            No call back execution at end of DMA abort procedure */
        huart->hdmatx.XferAbortCallback = NULL;

        /* Disable UART DMA IT */
        __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_TXF | UART_DMA_IT_TXH | UART_DMA_IT_TXE);

        /* Clear all flags */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_TXF | UART_DMA_FLAG_TXH);
    }

    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        dmarequest = LL_UART0_5_IsEnabledDMAReq_RX(huart->Instance.Instance0_5);
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        dmarequest = LL_UART6_7_IsEnabledDMAReq_RX(huart->Instance.Instance6_7);
    } else {
        return HAL_ERROR;
    }
    /* Disable the UART DMA Rx request if enabled */
    if (dmarequest) {
        /* Abort the UART DMA */
        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            LL_UART0_5_DisableDMAReq_RX(huart->Instance.Instance0_5);
        } else {
            LL_UART6_7_DisableDMAReq_RX(huart->Instance.Instance6_7);
        }

        /* Abort the UART DMA Rx channel: use blocking DMA Abort API (no callback) */
        /* Set the UART DMA Abort callback to Null.
            No call back execution at end of DMA abort procedure */
        huart->hdmarx.XferAbortCallback = NULL;

        /* Disable UART DMA IT */
        __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_RXF | UART_DMA_IT_RXH | UART_DMA_IT_RXE);

        /* Clear all flags */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_RXF | UART_DMA_FLAG_RXH | UART_DMA_FLAG_RXE | UART_DMA_FLAG_RXDONE);
    }

    /* Reset Tx and Rx transfer counters */
    huart->TxXferCount = 0x00U;
    huart->RxXferCount = 0x00U;

    /* Reset ErrorCode */
    huart->ErrorCode = HAL_UART_ERROR_NONE;

    /* Restore huart->RxState and huart->gState to Ready */
    huart->RxState = HAL_UART_STATE_READY;
    huart->gState = HAL_UART_STATE_READY;
    huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

    return HAL_OK;
}

/**
 * @brief  Abort ongoing Transmit transfer (blocking mode).
 * @param  huart UART handle.
 * @note   This procedure could be used for aborting any ongoing Tx transfer started in Interrupt or DMA mode.
 *         This procedure performs following operations :
 *           - Disable UART Interrupts (Tx)
 *           - Disable the DMA transfer in the peripheral register (if enabled)
 *           - Abort DMA transfer by calling HAL_DMA_Abort (in case of transfer in DMA mode)
 *           - Set handle State to READY
 * @note   This procedure is executed in blocking mode : when exiting function, Abort is considered as completed.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_AbortTransmit(UART_HandleTypeDef *huart)
{
    uint32_t dmarequest = 0x00U;
    /* Disable TXIE interrupt */
    __HAL_UART_DISABLE_IT(huart, UART_IT_TX);

    /* Disable the UART DMA Tx request if enabled */
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        dmarequest = LL_UART0_5_IsEnabledDMAReq_TX(huart->Instance.Instance0_5);
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        dmarequest = LL_UART6_7_IsEnabledDMAReq_TX(huart->Instance.Instance6_7);
    } else {
        return HAL_ERROR;
    }
    if (dmarequest) {
        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            /* Abort the UART DMA Tx : use blocking DMA Abort API (no callback) */
            LL_UART0_5_DisableDMAReq_TX(huart->Instance.Instance0_5);
        } else {
            LL_UART6_7_DisableDMAReq_TX(huart->Instance.Instance6_7);
        }
        /* Disable UART DMA IT */
        __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_TXF | UART_DMA_IT_TXH | UART_DMA_IT_TXE);

        /* Clear all flags */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_TXF | UART_DMA_FLAG_TXH);
        /* Abort the UART DMA Tx channel : use blocking DMA Abort API (no callback) */
        /* Set the UART DMA Abort callback to Null.
            No call back execution at end of DMA abort procedure */
        huart->hdmatx.XferAbortCallback = NULL;
    }

    /* Reset Tx transfer counter */
    huart->TxXferCount = 0x00U;

    /* Restore huart->gState to Ready */
    huart->gState = HAL_UART_STATE_READY;

    return HAL_OK;
}

/**
 * @brief  Abort ongoing Receive transfer (blocking mode).
 * @param  huart UART handle.
 * @note   This procedure could be used for aborting any ongoing Rx transfer started in Interrupt or DMA mode.
 *         This procedure performs following operations :
 *           - Disable UART Interrupts (Rx)
 *           - Disable the DMA transfer in the peripheral register (if enabled)
 *           - Abort DMA transfer by calling HAL_DMA_Abort (in case of transfer in DMA mode)
 *           - Set handle State to READY
 * @note   This procedure is executed in blocking mode : when exiting function, Abort is considered as completed.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *huart)
{
    uint32_t dmarequest = 0x00U;

    /* Disable RX, and ERR (Frame error, parity error, overrun error) interrupts */
    __HAL_UART_DISABLE_IT(huart, UART_IT_RX);
    __HAL_UART_DISABLE_IT(huart, UART_IT_ERR);

    /* If Reception till IDLE(RXTO) event was ongoing, disable RX_TO_IE interrupt */
    if (huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE) {
        /* Disable IDLE(RXTO) Interrupt */
        __HAL_UART_DISABLE_IT(huart, UART_IT_RXTO);
    }

    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        dmarequest = LL_UART0_5_IsEnabledDMAReq_RX(huart->Instance.Instance0_5);
    } else {
        dmarequest = LL_UART6_7_IsEnabledDMAReq_RX(huart->Instance.Instance6_7);
    }
    /* Disable the UART DMA Rx request if enabled */
    if (dmarequest) {
        /* Abort the UART DMA */
        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            LL_UART0_5_DisableDMAReq_RX(huart->Instance.Instance0_5);
        } else {
            LL_UART6_7_DisableDMAReq_RX(huart->Instance.Instance6_7);
        }

        /* Disable UART DMA IT */
        __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_RXF | UART_DMA_IT_RXH | UART_DMA_IT_RXE );

        /* Clear all flags */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_RXF | UART_DMA_FLAG_RXH | UART_DMA_FLAG_RXE | UART_DMA_FLAG_RXDONE);
        /* Abort the UART DMA Rx channel : use blocking DMA Abort API (no callback) */
        /* Set the UART DMA Abort callback to Null.
            No call back execution at end of DMA abort procedure */
        huart->hdmarx.XferAbortCallback = NULL;
    }

    /* Reset Rx transfer counter */
    huart->RxXferCount = 0x00U;

    /* Restore huart->RxState to Ready */
    huart->RxState = HAL_UART_STATE_READY;
    huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

    return HAL_OK;
}

/**
 * @brief  Abort ongoing transfers (Interrupt mode).
 * @param  huart UART handle.
 * @note   This procedure could be used for aborting any ongoing transfer started in Interrupt or DMA mode.
 *         This procedure performs following operations :
 *           - Disable UART Interrupts (Tx and Rx)
 *           - Disable the DMA transfer in the peripheral register (if enabled)
 *           - Abort DMA transfer by calling HAL_DMA_Abort_IT (in case of transfer in DMA mode)
 *           - Set handle State to READY
 *           - At abort completion, call user abort complete callback
 * @note   This procedure is executed in Interrupt mode, meaning that abort procedure could be
 *         considered as completed only when user abort complete callback is executed (not when exiting function).
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_Abort_IT(UART_HandleTypeDef *huart)
{
    uint32_t AbortCplt = 0x01U;
    uint32_t dmarequestRx = 0x00U, dmarequestTx = 0x00U;

    /* Disable RXIE, TXIE, ERRIE (Frame error, parity error, overrun error) interrupts */
    __HAL_UART_DISABLE_IT(huart, UART_IT_TX);
    __HAL_UART_DISABLE_IT(huart, UART_IT_RX);
    __HAL_UART_DISABLE_IT(huart, UART_IT_ERR);

    /* If Reception till IDLE event was ongoing, disable IDLEIE interrupt */
    if (huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE) {
        /* Disable IDLE(RX_TO_IE) Interrupt */
        __HAL_UART_DISABLE_IT(huart, UART_IT_RXTO);
    }

    /* If DMA Tx and/or DMA Rx Handles are associated to UART Handle, DMA Abort complete callbacks should be initialised
        before any call to DMA Abort functions */
    /* DMA Tx Handle is valid */
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        dmarequestTx = LL_UART0_5_IsEnabledDMAReq_TX(huart->Instance.Instance0_5);
        dmarequestRx = LL_UART0_5_IsEnabledDMAReq_RX(huart->Instance.Instance0_5);
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        dmarequestTx = LL_UART6_7_IsEnabledDMAReq_TX(huart->Instance.Instance6_7);
        dmarequestRx = LL_UART6_7_IsEnabledDMAReq_RX(huart->Instance.Instance6_7);
    } else {
        return HAL_ERROR;
    }
    /* Set DMA Abort Complete callback if UART DMA Tx request if enabled.
    Otherwise, set it to NULL */
    if (dmarequestTx) {
        huart->hdmatx.XferAbortCallback = UART_DMATxAbortCallback;
    } else {
        huart->hdmatx.XferAbortCallback = NULL;
    }
    if (dmarequestRx) {
        huart->hdmarx.XferAbortCallback = UART_DMARxAbortCallback;
    } else {
        huart->hdmarx.XferAbortCallback = NULL;
    }

    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        dmarequestTx = LL_UART0_5_IsEnabledDMAReq_TX(huart->Instance.Instance0_5);
        dmarequestRx = LL_UART0_5_IsEnabledDMAReq_RX(huart->Instance.Instance0_5);
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        dmarequestTx = LL_UART6_7_IsEnabledDMAReq_TX(huart->Instance.Instance6_7);
        dmarequestRx = LL_UART6_7_IsEnabledDMAReq_RX(huart->Instance.Instance6_7);
    } else {
        return HAL_ERROR;
    }
    /* Disable the UART DMA Tx/Rx request if enabled */
    if (dmarequestTx) {
        /* Abort the UART DMA */
        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            LL_UART0_5_DisableDMAReq_TX(huart->Instance.Instance0_5);
        } else {
            LL_UART6_7_DisableDMAReq_TX(huart->Instance.Instance6_7);
        }
        /* Disable UART DMA IT */
        __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_TXF | UART_DMA_IT_TXH | UART_DMA_IT_TXE);

        /* Clear all flags */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_TXF | UART_DMA_FLAG_TXH);

        /* Call User Abort callback */
        if(huart->hdmatx.XferAbortCallback != NULL) {
            huart->hdmatx.XferAbortCallback(&huart->hdmatx);
        }

        AbortCplt = 0x00U;
    } else {
        huart->hdmatx.XferAbortCallback = NULL;
    }

    /* Set DMA Abort Complete callback if UART DMA Rx request if enabled.
    Otherwise, set it to NULL */
    if (dmarequestRx) {
        /* Abort the UART DMA */
        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            LL_UART0_5_DisableDMAReq_RX(huart->Instance.Instance0_5);
        } else {
            LL_UART6_7_DisableDMAReq_RX(huart->Instance.Instance6_7);
        }
        /* Disable UART DMA IT */
        __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_RXF | UART_DMA_IT_RXH | UART_DMA_IT_RXE);

        /* Clear all flags */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_RXF | UART_DMA_FLAG_RXH | UART_DMA_FLAG_RXE | UART_DMA_FLAG_RXDONE);

        /* Call User Abort callback */
        if(huart->hdmarx.XferAbortCallback != NULL) {
            huart->hdmarx.XferAbortCallback(&huart->hdmarx);
        }
        AbortCplt = 0x00U;
    } else {
        huart->hdmarx.XferAbortCallback = NULL;
    }

    /* if no DMA abort complete callback execution is required => call user Abort Complete callback */
    if (AbortCplt == 0x01U) {
        /* Reset Tx and Rx transfer counters */
        huart->TxXferCount = 0x00U;
        huart->RxXferCount = 0x00U;

        /* Reset ErrorCode */
        huart->ErrorCode = HAL_UART_ERROR_NONE;

        /* Restore huart->gState and huart->RxState to Ready */
        huart->gState  = HAL_UART_STATE_READY;
        huart->RxState = HAL_UART_STATE_READY;
        huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

        /* As no DMA to be aborted, call directly user Abort complete callback */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
        /* Call registered Abort complete callback */
        huart->AbortCpltCallback(huart);
#else
        /* Call legacy weak Abort complete callback */
        HAL_UART_AbortCpltCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
    }

    return HAL_OK;
}

/**
 * @brief  Abort ongoing Transmit transfer (Interrupt mode).
 * @param  huart UART handle.
 * @note   This procedure could be used for aborting any ongoing Tx transfer started in Interrupt or DMA mode.
 *         This procedure performs following operations :
 *           - Disable UART Interrupts (Tx)
 *           - Disable the DMA transfer in the peripheral register (if enabled)
 *           - Abort DMA transfer by calling HAL_DMA_Abort_IT (in case of transfer in DMA mode)
 *           - Set handle State to READY
 *           - At abort completion, call user abort complete callback
 * @note   This procedure is executed in Interrupt mode, meaning that abort procedure could be
 *         considered as completed only when user abort complete callback is executed (not when exiting function).
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_AbortTransmit_IT(UART_HandleTypeDef *huart)
{
    uint32_t dmarequest = 0x00U;

    /* Disable TX interrupts */
    __HAL_UART_DISABLE_IT(huart, UART_IT_TX);

    /* Disable the UART DMA Tx request if enabled */
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        dmarequest = LL_UART0_5_IsEnabledDMAReq_TX(huart->Instance.Instance0_5);
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        dmarequest = LL_UART6_7_IsEnabledDMAReq_TX(huart->Instance.Instance6_7);
    } else {
        return HAL_ERROR;
    }
    if (dmarequest) {
        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            LL_UART0_5_DisableDMAReq_TX(huart->Instance.Instance0_5);
        } else {
            LL_UART6_7_DisableDMAReq_TX(huart->Instance.Instance6_7);
        }
        /* Abort the UART DMA Tx channel : use blocking DMA Abort API (no callback) */
        /* Set the UART DMA Abort callback :
            will lead to call HAL_UART_AbortCpltCallback() at end of DMA abort procedure */
        huart->hdmatx.XferAbortCallback = UART_DMATxOnlyAbortCallback;

        /* Abort DMA TX */
        /* Disable UART DMA IT */
        __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_TXF | UART_DMA_IT_TXH | UART_DMA_IT_TXE);

        /* Clear all flags */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_TXF | UART_DMA_FLAG_TXH);

        /* Call Directly huart->hdmatx->XferAbortCallback function in case of error */
        huart->hdmatx.XferAbortCallback(&huart->hdmatx);
    } else {
        /* Reset Tx transfer counter */
        huart->TxXferCount = 0x00U;

        /* Restore huart->gState to Ready */
        huart->gState = HAL_UART_STATE_READY;

        /* As no DMA to be aborted, call directly user Abort complete callback */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
        /* Call registered Abort Transmit Complete Callback */
        huart->AbortTransmitCpltCallback(huart);
#else
        /* Call legacy weak Abort Transmit Complete Callback */
        HAL_UART_AbortTransmitCpltCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
    }

  return HAL_OK;
}

/**
 * @brief  Abort ongoing Receive transfer (Interrupt mode).
 * @param  huart UART handle.
 * @note   This procedure could be used for aborting any ongoing Rx transfer started in Interrupt or DMA mode.
 *         This procedure performs following operations :
 *           - Disable UART Interrupts (Rx)
 *           - Disable the DMA transfer in the peripheral register (if enabled)
 *           - Abort DMA transfer by calling HAL_DMA_Abort_IT (in case of transfer in DMA mode)
 *           - Set handle State to READY
 *           - At abort completion, call user abort complete callback
 * @note   This procedure is executed in Interrupt mode, meaning that abort procedure could be
 *         considered as completed only when user abort complete callback is executed (not when exiting function).
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_AbortReceive_IT(UART_HandleTypeDef *huart)
{
    uint32_t dmarequest = 0x00U;

    /* Disable RXIE and ERRIE (Frame error, parity error, overrun error) interrupts */
    __HAL_UART_DISABLE_IT(huart, UART_IT_RX);
    __HAL_UART_DISABLE_IT(huart, UART_IT_ERR);

    /* If Reception till IDLE event was ongoing, disable IDLEIE interrupt */
    if (huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE) {
        /* Disable IDLE(RX_TO_IE) Interrupt */
        __HAL_UART_DISABLE_IT(huart, UART_IT_RXTO);
    }

    /* Disable the UART DMA Rx request if enabled */
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        dmarequest = LL_UART0_5_IsEnabledDMAReq_RX(huart->Instance.Instance0_5);
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        dmarequest = LL_UART6_7_IsEnabledDMAReq_RX(huart->Instance.Instance6_7);
    } else {
        return HAL_ERROR;
    }
    if (dmarequest) {
        /* Abort the UART DMA Rx channel : use blocking DMA Abort API (no callback) */
        /* Set the UART DMA Abort callback :
            will lead to call HAL_UART_AbortCpltCallback() at end of DMA abort procedure */
        huart->hdmarx.XferAbortCallback = UART_DMARxOnlyAbortCallback;

        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            LL_UART0_5_DisableDMAReq_RX(huart->Instance.Instance0_5);
        } else {
            LL_UART6_7_DisableDMAReq_RX(huart->Instance.Instance6_7);
        }
        /* Abort DMA RX */
        /* Disable UART DMA IT */
        __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_RXF | UART_DMA_IT_RXH | UART_DMA_IT_RXE);

        /* Clear all flags */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_RXF | UART_DMA_FLAG_RXH | UART_DMA_FLAG_RXE | UART_DMA_FLAG_RXDONE);

        /* Call Directly huart->hdmarx->XferAbortCallback function in case of error */
        huart->hdmarx.XferAbortCallback(&huart->hdmarx);
     } else {
        /* Reset Rx transfer counter */
        huart->RxXferCount = 0x00U;

        /* Restore huart->RxState to Ready */
        huart->RxState = HAL_UART_STATE_READY;
        huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

        /* As no DMA to be aborted, call directly user Abort complete callback */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
        /* Call registered Abort Receive Complete Callback */
        huart->AbortReceiveCpltCallback(huart);
#else
        /* Call legacy weak Abort Receive Complete Callback */
        HAL_UART_AbortReceiveCpltCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
    }

  return HAL_OK;
}

/**
 * @brief  This function handles UART interrupt request.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval None
 */
void HAL_UART_IRQHandler(UART_HandleTypeDef *huart)
{
    uint32_t isrflags, dmaisrflags;
    uint32_t ctrlits, dmactrlits;
    uint32_t errorflags = 0x00U;
    uint32_t dmarequest = 0x00U;
    uint32_t dmamode    = 0x00U;

    if ((huart->InstanceType == UART_INSTANCETYPE_0_5)) {
        isrflags = READ_REG(huart->Instance.Instance0_5->STA);
        ctrlits = READ_REG(huart->Instance.Instance0_5->CTRL);
        dmaisrflags = READ_REG(huart->Instance.Instance0_5->DMA_IF);
        dmactrlits = READ_REG(huart->Instance.Instance0_5->DMA_IE);
    } else {
        isrflags = READ_REG(huart->Instance.Instance6_7->STA);
        ctrlits = READ_REG(huart->Instance.Instance6_7->CTRL);
        dmaisrflags = READ_REG(huart->Instance.Instance6_7->DMA_IF);
        dmactrlits = READ_REG(huart->Instance.Instance6_7->DMA_IE);
    }

    /* If no error occurs */
    errorflags = (isrflags & (uint32_t)(UART_FLAG_PE | UART_FLAG_FE | UART_FLAG_OE | UART_FLAG_DE));
    if (errorflags == RESET) {
        /* UART in mode Receiver -------------------------------------------------*/
        if (((isrflags & UART_FLAG_RX) != RESET) && ((ctrlits & UART_IT_RX) != RESET)) {
            UART_Receive_IT(huart);
            return;
        }
    }

    /* If some errors occur */
    if ((errorflags != RESET) && (((ctrlits & (UART_IT_ERR | UART_IT_RX)) != RESET))) {
        /* UART parity error interrupt occurred ----------------------------------*/
        if (((isrflags & UART_FLAG_PE) != RESET) && ((ctrlits & UART_IT_ERR) != RESET)) {
            huart->ErrorCode |= HAL_UART_ERROR_PE;
        }

        /* UART frame error interrupt occurred -----------------------------------*/
        if (((isrflags & UART_FLAG_FE) != RESET) && ((ctrlits & UART_IT_ERR) != RESET)) {
            huart->ErrorCode |= HAL_UART_ERROR_FE;
        }

        /* UART Over-Run interrupt occurred --------------------------------------*/
        if (((isrflags & UART_FLAG_OE) != RESET) && (((ctrlits & (UART_IT_RX | UART_IT_ERR)) != RESET))) {
            huart->ErrorCode |= HAL_UART_ERROR_OE;
        }

        /* Call UART Error Call back function if need be --------------------------*/
        if (huart->ErrorCode != HAL_UART_ERROR_NONE) {
            /* UART in mode Receiver -----------------------------------------------*/
            if (((isrflags & UART_FLAG_RX) != RESET) && ((ctrlits & UART_IT_RX) != RESET)) {
                UART_Receive_IT(huart);
            }

            /* If Overrun error occurs, or if any error occurs in DMA mode reception,
                consider error as blocking */
            if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                dmarequest = LL_UART0_5_IsEnabledDMAReq_RX(huart->Instance.Instance0_5);
            } else {
                dmarequest = LL_UART6_7_IsEnabledDMAReq_RX(huart->Instance.Instance6_7);
            }
            if (((huart->ErrorCode & HAL_UART_ERROR_OE) != RESET) || dmarequest) {
                /* Blocking error : transfer is aborted
                Set the UART state ready to be able to start again the process,
                Disable Rx Interrupts, and disable Rx DMA request, if ongoing */
                UART_EndRxTransfer(huart);

                /* Disable the UART DMA Rx request if enabled, if dma transfer end, hardware will auto disable dmarequest */
                if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                    dmarequest = LL_UART0_5_IsEnabledDMAReq_RX(huart->Instance.Instance0_5);
                } else {
                    dmarequest = LL_UART6_7_IsEnabledDMAReq_RX(huart->Instance.Instance6_7);
                }
                /* if has overflow, DMA full flag will be set */
                if (__HAL_UART_GET_DMA_FLAG(huart, UART_DMA_IT_RXF) || dmarequest) {
                    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                        LL_UART0_5_DisableDMAReq_RX(huart->Instance.Instance0_5);
                    } else {
                        LL_UART6_7_DisableDMAReq_RX(huart->Instance.Instance6_7);
                    }
                    /* Set the UART DMA Abort callback :
                        will lead to call HAL_UART_ErrorCallback() at end of DMA abort procedure */
                    huart->hdmarx.XferAbortCallback = UART_DMAAbortOnError;

                    /* Abort the UART DMA Rx channel */
                    /* Disable UART DMA IT */
                    __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_RXF | UART_DMA_IT_RXH | UART_DMA_IT_RXE);
                    /* Clear all flags */
                    __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_RXF | UART_DMA_FLAG_RXH | UART_DMA_FLAG_RXE | UART_DMA_FLAG_RXDONE);

                    /* Call user error callback */
                    huart->hdmarx.XferAbortCallback(&huart->hdmarx);
                } else {
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
                    /*Call registered error callback*/
                    huart->ErrorCallback(huart);
#else
                    /*Call legacy weak error callback*/
                    HAL_UART_ErrorCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
                }
            } else {
                /* Non Blocking error : transfer could go on.
                Error is notified to user through user error callback */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
                /*Call registered error callback*/
                huart->ErrorCallback(huart);
#else
                /*Call legacy weak error callback*/
                HAL_UART_ErrorCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */

                huart->ErrorCode = HAL_UART_ERROR_NONE;
            }
        }
        return;
    } /* End if some error occurs */

    /* Check current reception Mode :
        If Reception till IDLE event has been selected : */
    if ((huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE) && ((isrflags & UART_FLAG_RXTO) != RESET) && ((ctrlits & UART_IT_RXTO) != RESET)) {
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_RXTO);

        /* Check if DMA mode is enabled in UART */
        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            dmarequest = LL_UART0_5_IsEnabledDMAReq_RX(huart->Instance.Instance0_5);
        } else {
            dmarequest = LL_UART6_7_IsEnabledDMAReq_RX(huart->Instance.Instance6_7);
        }
        if (dmarequest == 1U) {
            /* DMA mode enabled */
            /* Check received length : If all expected data are received, do nothing,
                (DMA cplt callback will be called).
                Otherwise, if at least one data has already been received, IDLE event is to be notified to user */
            uint16_t nb_remaining_rx_data = (uint16_t) __HAL_DMA_GET_COUNTER(huart);
            if ((nb_remaining_rx_data > 0U) && (nb_remaining_rx_data < huart->RxXferSize)) {
                /* Reception is not complete */
                huart->RxXferCount = nb_remaining_rx_data;

                /* In Normal mode, end DMA xfer and HAL UART Rx process*/
                if (huart->hdmarx.Mode != UART_DMA_CIRCULAR) {
                    /* Disable ERR (Frame error, parity error, overrun error) interrupts */
                    __HAL_UART_DISABLE_IT(huart, UART_IT_ERR);

                    /* Disable the DMA transfer for the receiver request */
                    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                        LL_UART0_5_DisableDMAReq_RX(huart->Instance.Instance0_5);
                    } else {
                        LL_UART6_7_DisableDMAReq_RX(huart->Instance.Instance6_7);
                    }

                    /* At end of Rx process, restore huart->RxState to Ready */
                    huart->RxState = HAL_UART_STATE_READY;
                    huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

                    /* Disable IDLE(RXTO) interrupt */
                    __HAL_UART_DISABLE_IT(huart, UART_IT_RXTO);

                    /* Last bytes received, so no need as the abort is immediate */
                    /* Abort the UART DMA Rx channel */

                    /* Disable UART DMA IT */
                    __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_RXF | UART_DMA_IT_RXH | UART_DMA_IT_RXE);
                    /* Clear all flags */
                    __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_RXF | UART_DMA_FLAG_RXH | UART_DMA_FLAG_RXE | UART_DMA_FLAG_RXDONE);
                }

                /* Initialize type of RxEvent that correspond to RxEvent callback execution;
                In this case, Rx Event type is Idle Event */
                huart->RxEventType = HAL_UART_RXEVENT_IDLE;

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
                /*Call registered Rx Event callback*/
                huart->RxEventCallback(huart, (huart->RxXferSize - huart->RxXferCount));
#else
                /*Call legacy weak Rx Event callback*/
                HAL_UARTEx_RxEventCallback(huart, (huart->RxXferSize - huart->RxXferCount));
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
            }
            return;
        } else {
            /* DMA mode not enabled */
            /* Check received length : If all expected data are received, do nothing.
                Otherwise, if at least one data has already been received, IDLE event is to be notified to user */
            uint16_t nb_rx_data = huart->RxXferSize - huart->RxXferCount;
            if ((huart->RxXferCount > 0U) && (nb_rx_data > 0U)) {
                /* Disable the UART Error Interrupt and RX interrupts */
                __HAL_UART_DISABLE_IT(huart, UART_IT_RX);

                /* Disable the UART Error Interrupt: (Frame error, parity error, overrun error) */
                __HAL_UART_DISABLE_IT(huart, UART_IT_ERR);

                /* Rx process is completed, restore huart->RxState to Ready */
                huart->RxState = HAL_UART_STATE_READY;
                huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

                /* Disable IDLE(RXTO) interrupt */
                __HAL_UART_DISABLE_IT(huart, UART_IT_RXTO);

                /* Initialize type of RxEvent that correspond to RxEvent callback execution;
                In this case, Rx Event type is Idle Event */
                huart->RxEventType = HAL_UART_RXEVENT_IDLE;

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
                /*Call registered Rx complete callback*/
                huart->RxEventCallback(huart, nb_rx_data);
#else
                /*Call legacy weak Rx Event callback*/
                HAL_UARTEx_RxEventCallback(huart, nb_rx_data);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
            }
            return;
        }
    }

    /* UART in mode Transmitter ------------------------------------------------*/
    if (((isrflags & UART_FLAG_TX) != RESET) && ((ctrlits & UART_IT_TX) != RESET) && (huart->TxXferCount != 0)) {
        UART_Transmit_IT(huart);
        return;
    }
    /* UART in mode Transmitter end, also DMA mode  --------------------------------------------*/
    if (((isrflags & UART_FLAG_TX) != RESET) && ((ctrlits & UART_IT_TX) != RESET) && (huart->TxXferCount == 0)) {
        UART_EndTransmit_IT(huart);
        return;
    }

    /* UART in DMA mode Transmitter ------------------------------------------------*/
    /* Check if DMA mode is enabled in UART.DMA mode enable bit will disable when send complete! */
    /* DMAReq_TX will be disable when data send complete */
    /* Half Transfer Complete Interrupt management ******************************/
    if (((dmaisrflags & UART_DMA_FLAG_TXH) != RESET) && ((dmactrlits & UART_DMA_IT_TXH) != RESET)) {
        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            dmamode = LL_UART0_5_DMA_IsEnabledTxCircularMode(huart->Instance.Instance0_5);
        } else {
            dmamode = LL_UART6_7_DMA_IsEnabledTxCircularMode(huart->Instance.Instance6_7);
        }
        /* Disable the half transfer interrupt if the DMA mode is not CIRCULAR */
        if (dmamode == 0U) {
            /* Disable the half transfer interrupt */
            __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_TXH);
        }
        /* Clear the half transfer complete flag */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_TXH);

        /* DMA peripheral state is not updated in Half Transfer */
        /* but in Transfer Complete case */
        if (huart->hdmatx.XferHalfCpltCallback != NULL) {
            /* Half transfer callback */
            huart->hdmatx.XferHalfCpltCallback(&huart->hdmatx);
        }
    }
    /* Transfer Complete Interrupt management ***********************************/
    else if (((dmaisrflags & UART_DMA_FLAG_TXF) != RESET) && ((dmactrlits & UART_DMA_IT_TXF) != RESET)) {
        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            dmamode = LL_UART0_5_DMA_IsEnabledTxCircularMode(huart->Instance.Instance0_5);
        } else {
            dmamode = LL_UART6_7_DMA_IsEnabledTxCircularMode(huart->Instance.Instance6_7);
        }
        if (dmamode == 0U) {
            /* Disable the transfer complete and error interrupt */
            __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_TXF | UART_DMA_IT_TXE);
        }
        /* Clear the transfer complete flag */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_TXF);

        if(huart->hdmatx.XferCpltCallback != NULL) {
            /* Transfer complete callback */
            huart->hdmatx.XferCpltCallback(&huart->hdmatx);
        }
    }

    /* UART in DMA mode Receiver ------------------------------------------------*/
    /* Check if DMA mode is enabled in UART */
    /* Half Transfer Complete Interrupt management ******************************/
    if (((dmaisrflags & UART_DMA_FLAG_RXH) != RESET) && ((dmactrlits & UART_DMA_IT_RXH) != RESET)) {
        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            dmamode = LL_UART0_5_DMA_IsEnabledRxCircularMode(huart->Instance.Instance0_5);
        } else {
            dmamode = LL_UART6_7_DMA_IsEnabledRxCircularMode(huart->Instance.Instance6_7);
        }
        /* Disable the half transfer interrupt if the DMA mode is not CIRCULAR */
        if (dmamode == 0U) {
            /* Disable the half transfer interrupt */
            __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_RXH);
        }
        /* Clear the half transfer complete flag */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_RXH);

        /* DMA peripheral state is not updated in Half Transfer */
        /* but in Transfer Complete case */
        if (huart->hdmarx.XferHalfCpltCallback != NULL) {
            /* Half transfer callback */
            huart->hdmarx.XferHalfCpltCallback(&huart->hdmarx);
        }
    }
    /* Transfer Complete Interrupt management ***********************************/
    else if (((dmaisrflags & UART_DMA_FLAG_RXF) != RESET) && ((dmactrlits & UART_DMA_IT_RXF) != RESET)) {
        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            dmamode = LL_UART0_5_DMA_IsEnabledRxCircularMode(huart->Instance.Instance0_5);
        } else {
            dmamode = LL_UART6_7_DMA_IsEnabledRxCircularMode(huart->Instance.Instance6_7);
        }
        if (dmamode == 0U) {
            /* Disable the transfer complete and error interrupt */
            __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_RXF | UART_DMA_IT_RXE);
        }
        /* Clear the transfer complete flag */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_RXF);

        if(huart->hdmarx.XferCpltCallback != NULL) {
            /* Transfer complete callback */
            huart->hdmarx.XferCpltCallback(&huart->hdmarx);
        }
    }
    /* Transfer Error Interrupt management **************************************/
    else if (((dmaisrflags & UART_DMA_FLAG_RXE) != RESET) && ((dmactrlits & UART_DMA_IT_RXE) != RESET)) {
        /* When a DMA transfer error occurs */
        /* A hardware clear of its EN bits is performed */
        /* Disable ALL DMA IT */
        __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_RXF | UART_DMA_IT_RXH | UART_DMA_IT_RXE);

        /* Clear all flags */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_RXF | UART_DMA_FLAG_RXH | UART_DMA_FLAG_RXE);

        if (huart->hdmarx.XferErrorCallback != NULL) {
            /* Transfer error callback */
            huart->hdmarx.XferErrorCallback(&huart->hdmarx);
        }
    }
}

/**
 * @brief  Tx Transfer completed callbacks.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval None
 */
__WEAK void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);
    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_UART_TxCpltCallback could be implemented in the user file
    */
}

/**
 * @brief  Tx Half Transfer completed callbacks.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval None
 */
__WEAK void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);
    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_UART_TxHalfCpltCallback could be implemented in the user file
    */
}

/**
 * @brief  Rx Transfer completed callbacks.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval None
 */
__WEAK void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);
    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_UART_RxCpltCallback could be implemented in the user file
    */
}

/**
 * @brief  Rx Half Transfer completed callbacks.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval None
 */
__WEAK void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);
    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_UART_RxHalfCpltCallback could be implemented in the user file
    */
}

/**
 * @brief  UART error callbacks.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval None
 */
__WEAK void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);
    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_UART_ErrorCallback could be implemented in the user file
    */
}

/**
 * @brief  UART Abort Complete callback.
 * @param  huart UART handle.
 * @retval None
 */
__WEAK void HAL_UART_AbortCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    /* NOTE: This function should not be modified, when the callback is needed,
                the HAL_UART_AbortCpltCallback can be implemented in the user file.
    */
}

/**
 * @brief  UART Abort Complete callback.
 * @param  huart UART handle.
 * @retval None
 */
__WEAK void HAL_UART_AbortTransmitCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    /* NOTE: This function should not be modified, when the callback is needed,
                the HAL_UART_AbortTransmitCpltCallback can be implemented in the user file.
    */
}

/**
 * @brief  UART Abort Receive Complete callback.
 * @param  huart UART handle.
 * @retval None
 */
__WEAK void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);

    /* NOTE: This function should not be modified, when the callback is needed,
                the HAL_UART_AbortReceiveCpltCallback can be implemented in the user file.
    */
}

/**
 * @brief  Reception Event Callback (Rx event notification called after use of advanced reception service).
 * @param  huart UART handle
 * @param  Size  Number of data available in application reception buffer (indicates a position in
 *               reception buffer until which, data are available)
 * @retval None
 */
__WEAK void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);
    UNUSED(Size);

    /* NOTE: This function should not be modified, when the callback is needed,
                the HAL_UARTEx_RxEventCallback can be implemented in the user file.
    */
}

/**
 * @}
 */

/** @defgroup UART_Exported_Functions_Group3 Peripheral Control functions
 *  @brief   UART control functions
 *
@verbatim
 ===============================================================================
                      ##### Peripheral Control functions #####
 ===============================================================================
    [..]
    This subsection provides a set of functions allowing to control the UART.
     (+) HAL_UART_ReceiverTimeout_Config() API allows to configure the receiver timeout value
     (+) HAL_UART_EnableReceiverTimeout() API enables the receiver timeout feature
     (+) HAL_UART_DisableReceiverTimeout() API disables the receiver timeout feature
@endverbatim
  * @{
  */

/**
 * @brief  Update on the fly the receiver timeout value in RTOR register.
 * @param  huart Pointer to a UART_HandleTypeDef structure that contains
 *                    the configuration information for the specified UART module.
 * @param  TimeoutValue receiver timeout value in number of baud blocks. The timeout
 *                     value must be less or equal to 0x00FF.
 * @retval None
 */
void HAL_UART_ReceiverTimeout_Config(UART_HandleTypeDef *huart, uint32_t TimeoutValue)
{
    if ((IS_UART6_7_ALL_INSTANCE(huart->Instance.Instance6_7) || IS_UART0_5_ALL_INSTANCE(huart->Instance.Instance0_5))) {
        assert_param(IS_UART_RXTIMEOUT(TimeoutValue));

        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            LL_UART0_5_SetTimeoutCount(huart->Instance.Instance0_5, TimeoutValue);
        } else {
            LL_UART6_7_SetTimeoutCount(huart->Instance.Instance6_7, TimeoutValue);
        }
    }
}

/**
 * @brief  Enable the UART receiver timeout feature.
 * @param  huart Pointer to a UART_HandleTypeDef structure that contains
 *                    the configuration information for the specified UART module.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_EnableReceiverTimeout(UART_HandleTypeDef *huart)
{
    if ((IS_UART6_7_ALL_INSTANCE(huart->Instance.Instance6_7) || IS_UART0_5_ALL_INSTANCE(huart->Instance.Instance0_5))) {
        if (huart->gState == HAL_UART_STATE_READY) {
            /* Process Locked */
            __HAL_LOCK(huart);

            huart->gState = HAL_UART_STATE_BUSY;

            /* Set the UART RX_TO_IE bit */
            __HAL_UART_ENABLE_IT(huart, UART_IT_RXTO);

            huart->gState = HAL_UART_STATE_READY;

            /* Process Unlocked */
            __HAL_UNLOCK(huart);

            return HAL_OK;
        } else {
            return HAL_BUSY;
        }
    } else {
        return HAL_ERROR;
    }
}

/**
 * @brief  Disable the UART receiver timeout feature.
 * @param  huart Pointer to a UART_HandleTypeDef structure that contains
 *                    the configuration information for the specified UART module.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_DisableReceiverTimeout(UART_HandleTypeDef *huart)
{
    if ((IS_UART6_7_ALL_INSTANCE(huart->Instance.Instance6_7) || IS_UART0_5_ALL_INSTANCE(huart->Instance.Instance0_5))) {
        if (huart->gState == HAL_UART_STATE_READY) {
            /* Process Locked */
            __HAL_LOCK(huart);

            huart->gState = HAL_UART_STATE_BUSY;

            /* Clear the UART RX_TO_IE bit */
            __HAL_UART_DISABLE_IT(huart, UART_IT_RXTO);

            huart->gState = HAL_UART_STATE_READY;

            /* Process Unlocked */
            __HAL_UNLOCK(huart);

            return HAL_OK;
        } else {
            return HAL_BUSY;
        }
    } else {
        return HAL_ERROR;
    }
}

/**
 * @}
 */

/** @defgroup UART_Exported_Functions_Group4 Peripheral State and Errors functions
 *  @brief   UART State and Errors functions
 *
@verbatim
  ==============================================================================
                 ##### Peripheral State and Errors functions #####
  ==============================================================================
 [..]
   This subsection provides a set of functions allowing to return the State of
   UART communication process, return Peripheral Errors occurred during communication
   process
   (+) HAL_UART_GetState() API can be helpful to check in run-time the state of the UART peripheral.
   (+) HAL_UART_GetError() check in run-time errors that could be occurred during communication.

@endverbatim
  * @{
  */

/**
 * @brief  Returns the UART state.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval HAL state
 */
HAL_UART_StateTypeDef HAL_UART_GetState(const UART_HandleTypeDef *huart)
{
    uint32_t temp1 = 0x00U, temp2 = 0x00U;
    temp1 = huart->gState;
    temp2 = huart->RxState;

    return (HAL_UART_StateTypeDef)(temp1 | temp2);
}

/**
 * @brief  Return the UART error code
 * @param  huart Pointer to a UART_HandleTypeDef structure that contains
 *               the configuration information for the specified UART.
 * @retval UART Error Code
 */
uint32_t HAL_UART_GetError(const UART_HandleTypeDef *huart)
{
    return huart->ErrorCode;
}

/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup UART_Private_Functions UART Private Functions
 * @{
 */

/**
 * @brief  Initialize the callbacks to their default values.
 * @param  huart UART handle.
 * @retval none
 */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
void UART_InitCallbacksToDefault(UART_HandleTypeDef *huart)
{
    /* Init the UART Callback settings */
    huart->TxHalfCpltCallback        = HAL_UART_TxHalfCpltCallback;        /* Legacy weak TxHalfCpltCallback        */
    huart->TxCpltCallback            = HAL_UART_TxCpltCallback;            /* Legacy weak TxCpltCallback            */
    huart->RxHalfCpltCallback        = HAL_UART_RxHalfCpltCallback;        /* Legacy weak RxHalfCpltCallback        */
    huart->RxCpltCallback            = HAL_UART_RxCpltCallback;            /* Legacy weak RxCpltCallback            */
    huart->ErrorCallback             = HAL_UART_ErrorCallback;             /* Legacy weak ErrorCallback             */
    huart->AbortCpltCallback         = HAL_UART_AbortCpltCallback;         /* Legacy weak AbortCpltCallback         */
    huart->AbortTransmitCpltCallback = HAL_UART_AbortTransmitCpltCallback; /* Legacy weak AbortTransmitCpltCallback */
    huart->AbortReceiveCpltCallback  = HAL_UART_AbortReceiveCpltCallback;  /* Legacy weak AbortReceiveCpltCallback  */
    huart->RxEventCallback           = HAL_UARTEx_RxEventCallback;         /* Legacy weak RxEventCallback           */
}
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */

/**
 * @brief  This function handles UART Communication Timeout. It waits
 *         until a flag is no longer in the specified status.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @param  Flag specifies the UART flag to check.
 * @param  Status The actual Flag status (SET or RESET).
 * @param  Tickstart Tick start value
 * @param  Timeout Timeout duration
 * @retval HAL status
 */
static HAL_StatusTypeDef UART_WaitOnFlagUntilTimeout(UART_HandleTypeDef *huart, uint32_t Flag, FlagStatus Status,
                                                     uint32_t Tickstart, uint32_t Timeout)
{
    /* Wait until flag is set */
    while ((__HAL_UART_GET_FLAG(huart, Flag) ? SET : RESET) == Status)
    {
        /* Check for the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if (((HAL_GetMillis() - Tickstart) > Timeout) || (Timeout == 0U)) {

                return HAL_TIMEOUT;
            }

            if ((Flag != UART_FLAG_TX) && (Flag != UART_FLAG_TB) && (Flag != UART_FLAG_FIFOF) && (Flag != UART_FLAG_FIFOE)) {
                if (__HAL_UART_GET_FLAG(huart, UART_FLAG_OE) == SET) {
                    /* Clear Overrun Error flag*/
                    __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_OE);

                    /* Blocking error : transfer is aborted
                    Set the UART state ready to be able to start again the process,
                    Disable Rx Interrupts if ongoing */
                    UART_EndRxTransfer(huart);

                    huart->ErrorCode = HAL_UART_ERROR_OE;

                    /* Process Unlocked */
                    __HAL_UNLOCK(huart);

                    return HAL_ERROR;
                }
            }
        }
    }
    return HAL_OK;
}

/**
  * @brief  Start Receive operation in interrupt mode.
  * @note   This function could be called by all HAL UART API providing reception in Interrupt mode.
  * @note   When calling this function, parameters validity is considered as already checked,
  *         i.e. Rx State, buffer address, ...
  *         UART Handle is assumed as Locked.
  * @param  huart UART handle.
  * @param  pData Pointer to data buffer (u8 or u16 data elements).
  * @param  Size  Amount of data elements (u8 or u16) to be received.
  * @retval HAL status
  */
HAL_StatusTypeDef UART_Start_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    huart->pRxBuffPtr = pData;
    huart->RxXferSize = Size;
    huart->RxXferCount = Size;

    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->RxState = HAL_UART_STATE_BUSY_RX;

    /* Enable the UART Error Interrupt: (Frame error, parity error, overrun error) */
    __HAL_UART_ENABLE_IT(huart, UART_IT_ERR);

    /* Enable the UART Data Receive Interrupt */
    __HAL_UART_ENABLE_IT(huart, UART_IT_RX);

    return HAL_OK;
}

/**
  * @brief  Start Receive operation in DMA mode.
  * @note   This function could be called by all HAL UART API providing reception in DMA mode.
  * @note   When calling this function, parameters validity is considered as already checked,
  *         i.e. Rx State, buffer address, ...
  *         UART Handle is assumed as Locked.
  * @param  huart UART handle.
  * @param  pData Pointer to data buffer (u8 or u16 data elements).
  * @param  Size  Amount of data elements (u8 or u16) to be received.
  * @retval HAL status
  */
HAL_StatusTypeDef UART_Start_Receive_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    uint32_t *tmp;

    huart->pRxBuffPtr = pData;
    huart->RxXferSize = Size;

    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->RxState = HAL_UART_STATE_BUSY_RX;

    /* Set the UART DMA transfer complete callback */
    huart->hdmarx.XferCpltCallback = UART_DMAReceiveCplt;

    /* Set the UART DMA Half transfer complete callback */
    huart->hdmarx.XferHalfCpltCallback = UART_DMARxHalfCplt;

    /* Set the DMA error callback */
    huart->hdmarx.XferErrorCallback = UART_DMAError;

    /* Set the DMA abort callback */
    huart->hdmarx.XferAbortCallback = NULL;

    /* Enable the DMA stream */
    tmp = (uint32_t *)&pData;
    if (UART_DMA_Start_RX_IT(huart, *(uint32_t *)tmp, Size) != HAL_OK) {
        /* Set error code to DMA */
        huart->ErrorCode = HAL_UART_ERROR_DMA;

        /* Restore huart->RxState to ready */
        huart->RxState = HAL_UART_STATE_READY;

        return HAL_ERROR;
    }
    /* Clear the Overrun flag just before enabling the DMA Rx request: can be mandatory for the second transfer */
    __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_OE);

    /* Enable the UART Error Interrupt: (Frame error, parity error, overrun error) */
    __HAL_UART_ENABLE_IT(huart, UART_IT_ERR);

    return HAL_OK;
}

/**
 * @brief  End ongoing Tx transfer on UART peripheral (following error detection or Transmit completion).
 * @param  huart UART handle.
 * @retval None
 */
static void UART_EndTxTransfer(UART_HandleTypeDef *huart)
{
    /* Disable TX interrupts */
    __HAL_UART_DISABLE_IT(huart, UART_IT_TX);

    /* At end of Tx process, restore huart->gState to Ready */
    huart->gState = HAL_UART_STATE_READY;
}

/**
 * @brief  End ongoing Rx transfer on UART peripheral (following error detection or Reception completion).
 * @param  huart UART handle.
 * @retval None
 */
static void UART_EndRxTransfer(UART_HandleTypeDef *huart)
{
    /* Clear all receive flags, st is auto clear by hardware */
    __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_RX | UART_FLAG_PE | UART_FLAG_OE | UART_FLAG_FE);

    /* Disable RXIE, ERRIE (Frame error, parity error, overrun error, data error) interrupts */
    __HAL_UART_DISABLE_IT(huart, UART_IT_RX);
    __HAL_UART_DISABLE_IT(huart, UART_IT_ERR);

    /* In case of reception waiting for IDLE event, disable also the IDLE IE interrupt source */
    if (huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE) {
        __HAL_UART_DISABLE_IT(huart, UART_IT_RXTO);
    }

    /* At end of Rx process, restore huart->RxState to Ready */
    huart->RxState = HAL_UART_STATE_READY;
    huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;
}

/**
  * @brief  DMA UART Tx communication abort callback, when initiated by user
  *         (To be called at end of DMA Tx Abort procedure following user abort request).
  * @note   When this callback is executed, User Abort complete call back is called only if no
  *         Abort still ongoing for Rx DMA Handle.
  * @param  hdma  Pointer to a UART_DMA_HandleTypeDef structure that contains
  *               the configuration information for the specified DMA module.
  * @retval None
  */
static void UART_DMATxAbortCallback(UART_DMA_HandleTypeDef *hdma)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)((UART_DMA_HandleTypeDef *)hdma)->Parent;

    huart->hdmatx.XferAbortCallback = NULL;

    /* Check if an Abort process is still ongoing */
    if (huart->hdmarx.XferAbortCallback != NULL) {
        return;
    }

    /* No Abort process still ongoing : All DMA channels are aborted, call user Abort Complete callback */
    huart->TxXferCount = 0x00U;
    huart->RxXferCount = 0x00U;

    /* Reset ErrorCode */
    huart->ErrorCode = HAL_UART_ERROR_NONE;

    /* Restore huart->gState and huart->RxState to Ready */
    huart->gState  = HAL_UART_STATE_READY;
    huart->RxState = HAL_UART_STATE_READY;
    huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

    /* Call user Abort complete callback */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
    /* Call registered Abort complete callback */
    huart->AbortCpltCallback(huart);
#else
    /* Call legacy weak Abort complete callback */
    HAL_UART_AbortCpltCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
}

/**
  * @brief  DMA UART Rx communication abort callback, when initiated by user
  *         (To be called at end of DMA Rx Abort procedure following user abort request).
  * @note   When this callback is executed, User Abort complete call back is called only if no
  *         Abort still ongoing for Tx DMA Handle.
  * @param  hdma  Pointer to a UART_DMA_HandleTypeDef structure that contains
  *               the configuration information for the specified DMA module.
  * @retval None
  */
static void UART_DMARxAbortCallback(UART_DMA_HandleTypeDef *hdma)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)((UART_DMA_HandleTypeDef *)hdma)->Parent;

    huart->hdmarx.XferAbortCallback = NULL;

    /* Check if an Abort process is still ongoing */
    if (huart->hdmatx.XferAbortCallback != NULL) {
        return;
    }

    /* No Abort process still ongoing : All DMA channels are aborted, call user Abort Complete callback */
    huart->TxXferCount = 0x00U;
    huart->RxXferCount = 0x00U;

    /* Reset ErrorCode */
    huart->ErrorCode = HAL_UART_ERROR_NONE;

    /* Restore huart->gState and huart->RxState to Ready */
    huart->gState  = HAL_UART_STATE_READY;
    huart->RxState = HAL_UART_STATE_READY;
    huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

    /* Call user Abort complete callback */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
    /* Call registered Abort complete callback */
    huart->AbortCpltCallback(huart);
#else
    /* Call legacy weak Abort complete callback */
    HAL_UART_AbortCpltCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
}

/**
 * @brief  DMA UART Tx communication abort callback, when initiated by user by a call to
 *         HAL_UART_AbortTransmit_IT API (Abort only Tx transfer)
 *         (This callback is executed at end of DMA Tx Abort procedure following user abort request,
 *         and leads to user Tx Abort Complete callback execution).
 * @param  hdma  Pointer to a UART_DMA_HandleTypeDef structure that contains
 *               the configuration information for the specified DMA module.
 * @retval None
 */
static void UART_DMATxOnlyAbortCallback(UART_DMA_HandleTypeDef *hdma)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)((UART_DMA_HandleTypeDef *)hdma)->Parent;

    huart->TxXferCount = 0x00U;

    /* Restore huart->gState to Ready */
    huart->gState = HAL_UART_STATE_READY;

    /* Call user Abort complete callback */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
    /* Call registered Abort Transmit Complete Callback */
    huart->AbortTransmitCpltCallback(huart);
#else
    /* Call legacy weak Abort Transmit Complete Callback */
    HAL_UART_AbortTransmitCpltCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
}

/**
 * @brief  DMA UART Rx communication abort callback, when initiated by user by a call to
 *         HAL_UART_AbortReceive_IT API (Abort only Rx transfer)
 *         (This callback is executed at end of DMA Rx Abort procedure following user abort request,
 *         and leads to user Rx Abort Complete callback execution).
 * @param  hdma  Pointer to a UART_DMA_HandleTypeDef structure that contains
 *               the configuration information for the specified DMA module.
 * @retval None
 */
static void UART_DMARxOnlyAbortCallback(UART_DMA_HandleTypeDef *hdma)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)((UART_DMA_HandleTypeDef *)hdma)->Parent;

    huart->RxXferCount = 0x00U;

    /* Restore huart->RxState to Ready */
    huart->RxState = HAL_UART_STATE_READY;
    huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

    /* Call user Abort complete callback */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
    /* Call registered Abort Receive Complete Callback */
    huart->AbortReceiveCpltCallback(huart);
#else
    /* Call legacy weak Abort Receive Complete Callback */
    HAL_UART_AbortReceiveCpltCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
}

/**
 * @brief  Sends an amount of data in non blocking mode.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval HAL status
 */
static HAL_StatusTypeDef UART_Transmit_IT(UART_HandleTypeDef *huart)
{
    const uint16_t *tmp;

    /* Check that a Tx process is ongoing */
    if (huart->gState == HAL_UART_STATE_BUSY_TX) {
        /* clear TX flag */
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_TX);
        __HAL_UART_DISABLE_IT(huart, UART_IT_TX);

        if (huart->Init.Parity == UART_PARITY_USER_DEFINED) {
            tmp = (const uint16_t *) huart->pTxBuffPtr;
            if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                LL_UART0_5_TransmitData9(huart->Instance.Instance0_5, (uint16_t)(*tmp & (uint16_t)0x01FF));
            } else {
                LL_UART6_7_TransmitData9(huart->Instance.Instance6_7, (uint16_t)(*tmp & (uint16_t)0x01FF));
            }
            huart->pTxBuffPtr += 2U;
        } else {
            if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                LL_UART0_5_TransmitData8(huart->Instance.Instance0_5, (uint8_t)(*huart->pTxBuffPtr++ & (uint8_t)0x00FF));
            } else {
                LL_UART6_7_TransmitData8(huart->Instance.Instance6_7, (uint8_t)(*huart->pTxBuffPtr++ & (uint8_t)0x00FF));
            }
        }
        --huart->TxXferCount;
        __HAL_UART_ENABLE_IT(huart, UART_IT_TX);
        return HAL_OK;
    } else {
        return HAL_BUSY;
    }
}

/**
 * @brief  Wraps up transmission in non blocking mode.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval HAL status
 */
static HAL_StatusTypeDef UART_EndTransmit_IT(UART_HandleTypeDef *huart)
{
    /* Disable the UART Transmit Complete Interrupt */
    __HAL_UART_DISABLE_IT(huart, UART_IT_TX);

    /* Tx process is ended, restore huart->gState to Ready */
    huart->gState = HAL_UART_STATE_READY;

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
    /*Call registered Tx complete callback*/
    huart->TxCpltCallback(huart);
#else
    /*Call legacy weak Tx complete callback*/
    HAL_UART_TxCpltCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */

    return HAL_OK;
}

/**
 * @brief  Receives an amount of data in non blocking mode
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval HAL status
 */
static HAL_StatusTypeDef UART_Receive_IT(UART_HandleTypeDef *huart)
{
    uint8_t  *pdata8bits;
    uint16_t *pdata16bits;

    /* Check that a Rx process is ongoing */
    if (huart->RxState == HAL_UART_STATE_BUSY_RX) {
        if (huart->Init.Parity == UART_PARITY_USER_DEFINED) {
            pdata8bits  = NULL;
            pdata16bits = (uint16_t *) huart->pRxBuffPtr;
            if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                    *pdata16bits = (uint16_t)(LL_UART0_5_ReceiveData9(huart->Instance.Instance0_5) & (uint16_t)0x01FF);
                } else {
                    *pdata16bits = (uint16_t)(LL_UART6_7_ReceiveData9(huart->Instance.Instance6_7) & (uint16_t)0x01FF);
                }
            huart->pRxBuffPtr += 2U;
        } else {
            pdata8bits = (uint8_t *) huart->pRxBuffPtr;
            pdata16bits  = NULL;

            if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
                *pdata8bits = (uint8_t)LL_UART0_5_ReceiveData8(huart->Instance.Instance0_5);
            } else {
                *pdata8bits = (uint8_t)LL_UART6_7_ReceiveData8(huart->Instance.Instance6_7);
            }
            huart->pRxBuffPtr += 1U;
        }

        if (--huart->RxXferCount == 0U) {
            /* Clear all receive flags */
            __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_RX | UART_FLAG_PE | UART_FLAG_OE | UART_FLAG_FE);

            /* Disable the UART Data Register Receive Interrupt */
            __HAL_UART_DISABLE_IT(huart, UART_IT_RX);

            /* Disable the UART Error Interrupt: (Frame error, Parity error, overrun error) */
            __HAL_UART_DISABLE_IT(huart, UART_IT_ERR);

            /* Rx process is completed, restore huart->RxState to Ready */
            huart->RxState = HAL_UART_STATE_READY;

            /* Initialize type of RxEvent to Transfer Complete */
            huart->RxEventType = HAL_UART_RXEVENT_TC;

            /* Check current reception Mode :
                If Reception till IDLE event has been selected : */
            if (huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE) {
                /* Set reception type to Standard */
                huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

                /* Disable IDLE(RXTO) interrupt */
                __HAL_UART_DISABLE_IT(huart, UART_IT_RXTO);

                /* Check if IDLE flag is set */
                if (__HAL_UART_GET_FLAG(huart, UART_IT_RXTO)) {
                    /* Clear IDLE flag in ISR */
                    __HAL_UART_CLEAR_FLAG(huart, UART_IT_RXTO);
                }

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
                /*Call registered Rx Event callback*/
                huart->RxEventCallback(huart, huart->RxXferSize);
#else
                /*Call legacy weak Rx Event callback*/
                HAL_UARTEx_RxEventCallback(huart, huart->RxXferSize);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
            } else {
                /* Standard reception API called */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
                /*Call registered Rx complete callback*/
                huart->RxCpltCallback(huart);
#else
                /*Call legacy weak Rx complete callback*/
                HAL_UART_RxCpltCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
            }

            return HAL_OK;
        }
        return HAL_OK;
    } else {
        return HAL_BUSY;
    }
}


/**
 * @brief  Start the UART DMA Transfer with interrupt enabled.
 * @param  hdma       pointer to a UART_DMA_HandleTypeDef structure that contains
 *                     the configuration information for the specified DMA Stream.
 * @param  Address    The source memory Buffer address
 * @param  DataLength The length of data to be transferred from source to destination
 * @retval HAL status
 */
static HAL_StatusTypeDef UART_DMA_Start_TX_IT(UART_HandleTypeDef *huart, uint32_t Address, uint32_t DataLength)
{
    /* Check the parameters */
    assert_param(IS_UART_DMA_BUFFER_SIZE(DataLength));

    /* Disable the UART DMA */
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        LL_UART0_5_DisableDMAReq_TX(huart->Instance.Instance0_5);
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        LL_UART6_7_DisableDMAReq_TX(huart->Instance.Instance6_7);
    } else {
        return HAL_ERROR;
    }

    /* Configure the source and the data length & clear flags*/
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        /* Clear TX all flags */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_TXF | UART_DMA_FLAG_TXH);

        /* Configure DMA data length */
        LL_UART0_5_DMA_SetTxLength(huart->Instance.Instance0_5, DataLength);

        /* Configure DMA address */
        LL_UART0_5_DMA_SetTxStartAddress(huart->Instance.Instance0_5, Address);
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        /* Clear TX all flags */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_TXF | UART_DMA_FLAG_TXH);

        /* Configure DMA data length */
        LL_UART6_7_DMA_SetTxLength(huart->Instance.Instance6_7, DataLength);

        /* Configure DMA address */
        LL_UART6_7_DMA_SetTxStartAddress(huart->Instance.Instance6_7, Address);
    } else {
        return HAL_ERROR;
    }

    /* Enable the transfer complete interrupt */
    if (NULL != huart->hdmatx.XferHalfCpltCallback) {
        /* Enable the Half transfer complete interrupt as well */
        __HAL_UART_ENABLE_DMA_IT(huart, UART_DMA_IT_TXF | UART_DMA_IT_TXH | UART_DMA_IT_TXE);
    } else {
        __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_TXH);
        __HAL_UART_ENABLE_DMA_IT(huart, UART_DMA_IT_TXF | UART_DMA_IT_TXE);
    }
    /* Enable the UART TX DMA */
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        LL_UART0_5_EnableDMAReq_TX(huart->Instance.Instance0_5);
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        LL_UART6_7_EnableDMAReq_TX(huart->Instance.Instance6_7);
    } else {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief  Start the UART DMA Receive with interrupt enabled.
 * @param  hdma       pointer to a UART_DMA_HandleTypeDef structure that contains
 *                     the configuration information for the specified DMA Stream.
 * @param  Address    The source memory Buffer address
 * @param  DataLength The length of data to be Received from source to destination
 * @retval HAL status
 */
static HAL_StatusTypeDef UART_DMA_Start_RX_IT(UART_HandleTypeDef *huart, uint32_t Address, uint32_t DataLength)
{
    /* Check the parameters */
    assert_param(IS_UART_DMA_BUFFER_SIZE(DataLength));

    /* Disable the UART DMA */
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        LL_UART0_5_DisableDMAReq_RX(huart->Instance.Instance0_5);
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        LL_UART6_7_DisableDMAReq_RX(huart->Instance.Instance6_7);
    } else {
        return HAL_ERROR;
    }

    /* Configure the source and the data length & clear flags*/
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        /* Clear RX all flags */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_RXF | UART_DMA_FLAG_RXH | UART_DMA_FLAG_RXE);

        /* Configure DMA data length */
        LL_UART0_5_DMA_SetRxLength(huart->Instance.Instance0_5, DataLength);

        /* Configure DMA address */
        LL_UART0_5_DMA_SetRxStartAddress(huart->Instance.Instance0_5, Address);
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        /* Clear RX all flags */
        __HAL_UART_CLEAR_DMA_FLAG(huart, UART_DMA_FLAG_RXF | UART_DMA_FLAG_RXH | UART_DMA_FLAG_RXE);

        /* Configure DMA data length */
        LL_UART6_7_DMA_SetRxLength(huart->Instance.Instance6_7, DataLength);

        /* Configure DMA address */
        LL_UART6_7_DMA_SetRxStartAddress(huart->Instance.Instance6_7, Address);
    } else {
        return HAL_ERROR;
    }

    /* Enable the transfer complete interrupt */
    if (NULL != huart->hdmarx.XferHalfCpltCallback) {
        /* Enable the Half transfer complete interrupt as well */
        __HAL_UART_ENABLE_DMA_IT(huart, UART_DMA_IT_RXF | UART_DMA_IT_RXH | UART_DMA_IT_RXE);
    } else {
        __HAL_UART_DISABLE_DMA_IT(huart, UART_DMA_IT_RXH);
        __HAL_UART_ENABLE_DMA_IT(huart, UART_DMA_IT_RXF | UART_DMA_IT_RXE);
    }
    /* Enable the UART RX DMA */
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        LL_UART0_5_EnableDMAReq_RX(huart->Instance.Instance0_5);
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        LL_UART6_7_EnableDMAReq_RX(huart->Instance.Instance6_7);
    } else {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief DMA UART transmit process complete callback.
 * @param hdma DMA handle.
 * @retval None
 */
static void UART_DMATransmitCplt(UART_DMA_HandleTypeDef *hdma)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)((UART_DMA_HandleTypeDef *)hdma)->Parent;
    /* UART DMA Normal mode*/
    if (hdma->Mode != UART_DMA_CIRCULAR) {
        huart->TxXferCount = 0x00U;

        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            /* Disable the DMA transfer for transmit request by setting the TX_DMA_EN bit */
            /* Auto disable by hardware */
            LL_UART0_5_DisableDMAReq_TX(huart->Instance.Instance0_5);
        } else {
            /* Disable the DMA transfer for transmit request by setting the TX_DMA_EN bit */
            /* Auto disable by hardware */
            LL_UART6_7_DisableDMAReq_TX(huart->Instance.Instance6_7);
        }
        /* Enable the UART Transmit Complete Interrupt */
        __HAL_UART_ENABLE_IT(huart, UART_IT_TX);
    }
    /* UART DMA Circular mode */
    else {
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
        /*Call registered Tx complete callback*/
        huart->TxCpltCallback(huart);
#else
        /*Call legacy weak Tx complete callback*/
        HAL_UART_TxCpltCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
    }
}

/**
 * @brief DMA UART transmit process half complete callback.
 * @param hdma DMA handle.
 * @retval None
 */
static void UART_DMATxHalfCplt(UART_DMA_HandleTypeDef *hdma)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)(hdma->Parent);

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
    /*Call registered Tx Half complete callback*/
    huart->TxHalfCpltCallback(huart);
#else
    /*Call legacy weak Tx Half complete callback*/
    HAL_UART_TxHalfCpltCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
}

/**
 * @brief  DMA UART communication error callback.
 * @param  hdma  Pointer to a UART_DMA_HandleTypeDef structure that contains
 *               the configuration information for the specified DMA module.
 * @retval None
 */
static void UART_DMAError(UART_DMA_HandleTypeDef *hdma)
{
    uint32_t dmarequest = 0x00U;
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)((UART_DMA_HandleTypeDef *)hdma)->Parent;

    /* Stop UART DMA Tx request if ongoing */
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        dmarequest = LL_UART0_5_IsEnabledDMAReq_TX(huart->Instance.Instance0_5);
    } else {
        dmarequest = LL_UART6_7_IsEnabledDMAReq_TX(huart->Instance.Instance6_7);
    }
    if ((huart->gState == HAL_UART_STATE_BUSY_TX) && dmarequest) {
        huart->TxXferCount = 0x00U;
        UART_EndTxTransfer(huart);
    }

    /* Stop UART DMA Rx request if ongoing */
    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        dmarequest = LL_UART0_5_IsEnabledDMAReq_RX(huart->Instance.Instance0_5);
    } else {
        dmarequest = LL_UART6_7_IsEnabledDMAReq_RX(huart->Instance.Instance6_7);
    }
    if ((huart->RxState == HAL_UART_STATE_BUSY_RX) && dmarequest)
    {
        huart->RxXferCount = 0x00U;
        UART_EndRxTransfer(huart);
    }

    huart->ErrorCode |= HAL_UART_ERROR_DMA;
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
    /*Call registered error callback*/
    huart->ErrorCallback(huart);
#else
    /*Call legacy weak error callback*/
    HAL_UART_ErrorCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
}

/**
 * @brief  DMA UART communication abort callback, when initiated by HAL services on Error
 *         (To be called at end of DMA Abort procedure following error occurrence).
 * @param  hdma  Pointer to a UART_DMA_HandleTypeDef structure that contains
 *               the configuration information for the specified DMA module.
 * @retval None
 */
static void UART_DMAAbortOnError(UART_DMA_HandleTypeDef *hdma)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)((UART_DMA_HandleTypeDef *)hdma)->Parent;
    huart->RxXferCount = 0x00U;
    huart->TxXferCount = 0x00U;

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
    /*Call registered error callback*/
    huart->ErrorCallback(huart);
#else
    /*Call legacy weak error callback*/
    HAL_UART_ErrorCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
}

/**
 * @brief DMA UART receive process complete callback.
 * @param hdma DMA handle.
 * @retval None
 */
static void UART_DMAReceiveCplt(UART_DMA_HandleTypeDef *hdma)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)((UART_DMA_HandleTypeDef *)hdma)->Parent;

    /* Check if DMA in circular mode */
    if (hdma->Mode != UART_DMA_CIRCULAR) {
        huart->RxXferCount = 0U;

        /* Disable PE and ERR (Frame error, parity error, overrun error) interrupts */
        __HAL_UART_DISABLE_IT(huart, UART_IT_ERR);

        if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
            /* Disable the DMA transfer for transmit request by setting the RX_DMA_EN bit */
            /* Can disable by hardware */
            LL_UART0_5_DisableDMAReq_RX(huart->Instance.Instance0_5);
        } else {
            /* Disable the DMA transfer for transmit request by setting the RX_DMA_EN bit */
            /* Can disable by hardware */
            LL_UART6_7_DisableDMAReq_RX(huart->Instance.Instance6_7);
        }

        /* At end of Rx process, restore huart->RxState to Ready */
        huart->RxState = HAL_UART_STATE_READY;

        /* If Reception till IDLE event has been selected, Disable IDLE Interrupt */
        if (huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE) {
            __HAL_UART_DISABLE_IT(huart, UART_IT_RXTO);
        }
    }

    /* Initialize type of RxEvent that correspond to RxEvent callback execution;
        In this case, Rx Event type is Transfer Complete */
    huart->RxEventType = HAL_UART_RXEVENT_TC;

    /* Check current reception Mode :
        If Reception till IDLE event has been selected : use Rx Event callback */
    if (huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE) {
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
        /*Call registered Rx Event callback*/
        huart->RxEventCallback(huart, huart->RxXferSize);
#else
        /*Call legacy weak Rx Event callback*/
        HAL_UARTEx_RxEventCallback(huart, huart->RxXferSize);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
    } else {
        /* In other cases : use Rx Complete callback */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
        /*Call registered Rx complete callback*/
        huart->RxCpltCallback(huart);
#else
        /*Call legacy weak Rx complete callback*/
        HAL_UART_RxCpltCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
    }
}

/**
  * @brief DMA UART receive process half complete callback
  * @param  hdma  Pointer to a UART_DMA_HandleTypeDef structure that contains
  *               the configuration information for the specified DMA module.
  * @retval None
  */
static void UART_DMARxHalfCplt(UART_DMA_HandleTypeDef *hdma)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)((UART_DMA_HandleTypeDef *)hdma)->Parent;

    /* Initialize type of RxEvent that correspond to RxEvent callback execution;
        In this case, Rx Event type is Half Transfer */
    huart->RxEventType = HAL_UART_RXEVENT_HT;

    /* Check current reception Mode :
        If Reception till IDLE event has been selected : use Rx Event callback */
    if (huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE) {
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
        /*Call registered Rx Event callback*/
        huart->RxEventCallback(huart, huart->RxXferSize / 2U);
#else
        /*Call legacy weak Rx Event callback*/
        HAL_UARTEx_RxEventCallback(huart, huart->RxXferSize / 2U);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
    } else {
        /* In other cases : use Rx Half Complete callback */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
        /*Call registered Rx Half complete callback*/
        huart->RxHalfCpltCallback(huart);
#else
        /*Call legacy weak Rx Half complete callback*/
        HAL_UART_RxHalfCpltCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
    }
}

/**
 * @brief  Configures the UART peripheral.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval HAL status
 */
static HAL_StatusTypeDef UART_SetConfig(UART_HandleTypeDef *huart)
{
    uint32_t pclk;
    uint32_t currentcore;

    /* Check the parameters */
    assert_param(IS_UART_DATAWIDTH(huart->Init.DataWidth));
    assert_param(IS_UART_STOPBITS(huart->Init.StopBits));
    assert_param(IS_UART_PARITY(huart->Init.Parity));
    assert_param(IS_UART_BAUDRATE(huart->Init.BaudRate));
    assert_param(IS_UART_RXTIMEOUT(huart->Init.RxTimeout));

    /* get current core is CORE0 or CORE1 */
    currentcore = HAL_GetCoreId();
    if(currentcore == 0x0U) {
        pclk = HAL_SYSTEM_GetSysClockFreq(HAL_SYSTEM_CLK_CORE0);
    } else if (currentcore == 0x1U) {
        pclk = HAL_SYSTEM_GetSysClockFreq(HAL_SYSTEM_CLK_CORE1);
    } else {
        return HAL_ERROR;
    }

    if (huart->InstanceType == UART_INSTANCETYPE_0_5) {
        /*-------------------------- UART CTRL Configuration -----------------------*/
        /* Configure the UART Word Length, Parity and mode:
            Set the DATLEN bits according to huart->Init.DataWidth value
            Set the STOPS bits according to huart->Init.StopBits value
            Set PARS bits according to huart->Init.Parity value
            Set TIMEOUT_CNT bits according to huart->RxTimeout value */
        LL_UART0_5_ConfigCharacter(huart->Instance.Instance0_5, huart->Init.DataWidth, huart->Init.Parity, huart->Init.StopBits);
        LL_UART0_5_SetTimeoutCount(huart->Instance.Instance0_5, huart->Init.RxTimeout);

        /*-------------------------- UART BAUD Configuration -----------------------*/
        /* Configure the UART Baud Rate: Set CLKDIV and FDIV bits according to huart->Init.BaudRate value */
        /* If proper clock source reported */
        if (pclk != 0U) {
            LL_UART0_5_SetBaudRate(huart->Instance.Instance0_5, pclk, huart->Init.BaudRate);
        }
    } else if (huart->InstanceType == UART_INSTANCETYPE_6_7) {
        /*-------------------------- UART CTRL Configuration -----------------------*/
        /* Configure the UART Word Length, Parity and mode:
            Set the DATLEN bits according to huart->Init.DataWidth value
            Set the STOPS bits according to huart->Init.StopBits value
            Set PARS bits according to huart->Init.Parity value
            Set TIMEOUT_CNT bits according to huart->RxTimeout value */
        LL_UART6_7_ConfigCharacter(huart->Instance.Instance6_7, huart->Init.DataWidth, huart->Init.Parity, huart->Init.StopBits);
        LL_UART6_7_SetTimeoutCount(huart->Instance.Instance6_7, huart->Init.RxTimeout);

        /*-------------------------- UART BAUD Configuration -----------------------*/
        /* Configure the UART Baud Rate: Set CLKDIV and FDIV bits according to huart->Init.BaudRate value */
        /* If proper clock source reported */
        if (pclk != 0U) {
            LL_UART6_7_SetBaudRate(huart->Instance.Instance6_7, pclk, huart->Init.BaudRate);
        }
    } else {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @}
 */

#endif /* HAL_UART_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
