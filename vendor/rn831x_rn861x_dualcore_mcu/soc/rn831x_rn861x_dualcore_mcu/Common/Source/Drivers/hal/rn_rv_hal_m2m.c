/**
 ******************************************************************************
 * @file   rn_rv_hal_m2m.c
 * @author Renergy Software Team
 * @brief  M2M HAL module driver.
 *         This file provides firmware functions to manage the following
 *         functionalities of the Memory to Memory (M2M) transfer peripheral:
 *          + Initialization and de-initialization functions
 *          + Configuration functions
 *          + Control functions
 *          + Interrupts management
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

/** @defgroup M2M M2M
 * @brief     M2M HAL module driver
 * @{
 */

#ifdef HAL_M2M_MODULE_ENABLED

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/** @defgroup M2M_Private_Functions M2M Private Functions
 * @{
 */
static void M2M_SetConfig(M2M_HandleTypeDef *hm2m, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);
/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup M2M_Exported_Functions M2M Exported Functions
 * @{
 */

/** @defgroup M2M_Exported_Functions_Group1 Initialization and de-initialization functions
 * @brief     Initialization and Configuration functions
 *
@verbatim
================================================================================
            ##### Initialization and de-initialization functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) Initialize and configure the M2M.
        (+) De-initialize the M2M.

@endverbatim
 * @{
 */

/**
 * @brief  Initialize the M2M according to the specified
 *         parameters in the M2M_InitTypeDef and create the associated handle.
 * @param  hm2m: pointer to a M2M_HandleTypeDef structure that contains
 *               the configuration information.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_M2M_Init(M2M_HandleTypeDef *hm2m)
{
    uint32_t mode_tmp;

    /* Check the M2M handle allocation. */
    if (hm2m == NULL) {
        return HAL_ERROR;
    }

    /* Change M2M peripheral state. */
    hm2m->State = HAL_M2M_STATE_BUSY;

    /* Init the low level hardware */
    HAL_M2M_MspInit(hm2m);

    mode_tmp = (hm2m->Init.SrcDataValidBit | (hm2m->Init.DstDataValidBit << 4U));
    if (hm2m->Init.DummyEnable == 1U) {
        mode_tmp |= M2M_MODE_DUMMY;
    }
    if (hm2m->Init.DstDataReverseMode == M2M_DATA_REV_BYTEONLY) {
        mode_tmp |= M2M_DATA_REV_BYTEONLY;
    } else if (hm2m->Init.DstDataReverseMode == M2M_DATA_REV_ADDRONLY) {
        mode_tmp |= M2M_DATA_REV_ADDRONLY;
    } else if (hm2m->Init.DstDataReverseMode == M2M_DATA_REV_ADDR) {
        mode_tmp |= M2M_DATA_REV_ADDR;
    } else {
        /* no care */
    }
    LL_M2M_WriteReg(hm2m->Instance, MODE, mode_tmp);

    /* Initialize the error code */
    hm2m->ErrorCode = HAL_M2M_ERROR_NONE;

    /* Initialize the M2M state. */
    hm2m->State = HAL_M2M_STATE_READY;

    /* Allocate lock resource and initialize it. */
    hm2m->Lock = HAL_UNLOCKED;

    /* Return function status. */
    return HAL_OK;
}

/**
 * @brief  DeInitialize the M2M peripheral.
 * @param  hm2m: pointer to a M2M_HandleTypeDef structure that contains
 *               the configuration information.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_M2M_DeInit(M2M_HandleTypeDef *hm2m)
{
    /* Check the M2M handle allocation. */
    if (hm2m == NULL) {
        return HAL_ERROR;
    }

    /* Check the parameters. */
    assert_param(IS_M2M_ALL_INSTANCE(hm2m->Instance));

    /* Check the M2M peripheral state. */
    if (hm2m->State == HAL_M2M_STATE_BUSY) {
        return HAL_BUSY;
    }

#ifdef USE_FULL_LL_DRIVER
    __HAL_SYSTEM_PSW_UNLOCK();
    LL_M2M_DeInit(hm2m->Instance);
    __HAL_SYSTEM_PSW_LOCK();
#endif

    /* DeInit the low level hardware */
    HAL_M2M_MspDeInit(hm2m);

    /* Clean all callbacks. */
    hm2m->XferCpltCallback = NULL;
    hm2m->XferAbortCallback = NULL;

    /* Initialize the error code */
    hm2m->ErrorCode = HAL_M2M_ERROR_NONE;

    /* Reset the M2M state. */
    hm2m->State = HAL_M2M_STATE_RESET;

    /* Release Lock. */
    __HAL_UNLOCK(hm2m);

    /* Return function status. */
    return HAL_OK;
}

/**
 * @brief  Initializes the M2M MSP.
 * @param  hm2m: pointer to a M2M_HandleTypeDef structure that contains
 *               the configuration information.
 * @retval None
 */
__WEAK void HAL_M2M_MspInit(M2M_HandleTypeDef *hm2m)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hm2m);

    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_M2M_MspInit can be implemented in the user file. */
}

/**
 * @brief  DeInitialize the M2M MSP.
 * @param  hm2m: pointer to a M2M_HandleTypeDef structure that contains
 *               the configuration information.
 * @retval None
 */
__WEAK void HAL_M2M_MspDeInit(M2M_HandleTypeDef *hm2m)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hm2m);

    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_M2M_MspDeInit can be implemented in the user file. */
}
/**
 * @}
 */

/** @defgroup M2M_Exported_Functions_Group2 IO operation functions
 * @brief     IO operation functions.
 *
@verbatim
================================================================================
                    ##### IO operation functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) Configure the source, destination address and data length and Start M2M transfer
        (+) Configure the source, destination address and data length and Start M2M transfer with interrupt
        (+) Abort M2M transfer
        (+) Handle M2M interrupt request

@endverbatim
 * @{
 */

/**
 * @brief  Start the M2M Transfer.
 * @param  hm2m:       pointer to a M2M_HandleTypeDef structure that contains
 *                     the configuration information.
 * @param  SrcAddress: The source memory Buffer address
 * @param  DstAddress: The destination memory Buffer address
 * @param  DataLength: The length of data to be transferred from source to destination
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_M2M_Start(M2M_HandleTypeDef *hm2m, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
    HAL_StatusTypeDef status = HAL_OK;

    /* Check the parameters. */
    assert_param(IS_M2M_BUFFER_SIZE(DataLength));

    /* Process locked. */
    __HAL_LOCK(hm2m);

    if (HAL_M2M_STATE_READY == hm2m->State) {
        /* Change M2M peripheral state. */
        hm2m->State = HAL_M2M_STATE_BUSY;
        hm2m->ErrorCode = HAL_M2M_ERROR_NONE;

        /* Disable the peripheral. */
        LL_M2M_Disable(hm2m->Instance);

        /* Configure the source, destination address and the data length & clear flags. */
        M2M_SetConfig(hm2m, SrcAddress, DstAddress, DataLength);

        /* Enable the Peripheral. */
        LL_M2M_Enable(hm2m->Instance);
    } else {
        /* Process Unlocked. */
        __HAL_UNLOCK(hm2m);

        status = HAL_BUSY;
    }

    return status;
}

/**
 * @brief  Start the M2M Transfer with interrupt enabled.
 * @param  hm2m:       pointer to a M2M_HandleTypeDef structure that contains
 *                     the configuration information.
 * @param  SrcAddress: The source memory Buffer address
 * @param  DstAddress: The destination memory Buffer address
 * @param  DataLength: The length of data to be transferred from source to destination
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_M2M_Start_IT(M2M_HandleTypeDef *hm2m, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
    HAL_StatusTypeDef status = HAL_OK;

    /* Check the parameters. */
    assert_param(IS_M2M_BUFFER_SIZE(DataLength));

    /* Process locked. */
    __HAL_LOCK(hm2m);

    if (HAL_M2M_STATE_READY == hm2m->State) {
        /* Change M2M peripheral state. */
        hm2m->State = HAL_M2M_STATE_BUSY;
        hm2m->ErrorCode = HAL_M2M_ERROR_NONE;

        /* Disable the peripheral. */
        LL_M2M_Disable(hm2m->Instance);

        /* Configure the source, destination address and the data length & clear flags. */
        M2M_SetConfig(hm2m, SrcAddress, DstAddress, DataLength);

        /* Enable transfer complete interrupt. */
        LL_M2M_EnableIT(hm2m->Instance);

        /* Enable the Peripheral. */
        LL_M2M_Enable(hm2m->Instance);
    } else {
        /* Process Unlocked. */
        __HAL_UNLOCK(hm2m);

        status = HAL_BUSY;
    }

    return status;
}

/**
 * @brief  Abort the M2M Transfer.
 * @param  hm2m: pointer to a M2M_HandleTypeDef structure that contains
 *               the configuration information.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_M2M_Abort(M2M_HandleTypeDef *hm2m)
{
    HAL_StatusTypeDef status = HAL_OK;

    if (hm2m->State != HAL_M2M_STATE_BUSY) {
        /* no transfer ongoing */
        hm2m->ErrorCode = HAL_M2M_ERROR_NO_XFER;

        /* Process Unlocked. */
        __HAL_UNLOCK(hm2m);

        return HAL_ERROR;
    } else {
        /* Disable transfer complete interrupt. */
        LL_M2M_DisableIT(hm2m->Instance);

        /* Disable the channel. */
        LL_M2M_Disable(hm2m->Instance);

        /* Clear all flags. */
        LL_M2M_ClearFlag(hm2m->Instance);

        /* Change the M2M state */
        hm2m->State = HAL_M2M_STATE_READY;

        /* Process Unlocked */
        __HAL_UNLOCK(hm2m);
    }

    return status;
}

/**
 * @brief  Aborts the M2M Transfer in Interrupt mode.
 * @param  hm2m: pointer to a M2M_HandleTypeDef structure that contains
 *               the configuration information.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_M2M_Abort_IT(M2M_HandleTypeDef *hm2m)
{
    HAL_StatusTypeDef status = HAL_OK;

    if (hm2m->State != HAL_M2M_STATE_BUSY) {
        /* no transfer ongoing */
        hm2m->ErrorCode = HAL_M2M_ERROR_NO_XFER;

        /* Process Unlocked. */
        __HAL_UNLOCK(hm2m);

        return HAL_ERROR;
    } else {
        /* Disable transfer complete interrupt. */
        LL_M2M_DisableIT(hm2m->Instance);

        /* Disable the channel. */
        LL_M2M_Disable(hm2m->Instance);

        /* Clear all flags. */
        LL_M2M_ClearFlag(hm2m->Instance);

        /* Change the M2M state */
        hm2m->State = HAL_M2M_STATE_READY;

        /* Process Unlocked */
        __HAL_UNLOCK(hm2m);

        /* Call User Abort callback */
        if (hm2m->XferAbortCallback != NULL) {
            hm2m->XferAbortCallback(hm2m);
        }
    }

    return status;
}

/**
 * @brief  Polling for transfer complete.
 * @param  hm2m:    pointer to a M2M_HandleTypeDef structure that contains
 *                  the configuration information.
 * @param  Timeout: Timeout duration in milliseconds.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_M2M_PollForTransfer(M2M_HandleTypeDef *hm2m, uint32_t Timeout)
{
    uint32_t tickstart = 0U;

    if (HAL_M2M_STATE_BUSY != hm2m->State) {
        /* no transfer ongoing. */
        hm2m->ErrorCode = HAL_M2M_ERROR_NO_XFER;

        /* Process Unlocked. */
        __HAL_UNLOCK(hm2m);

        return HAL_ERROR;
    }

    /* Get tick. */
    tickstart = HAL_GetMillis();
    while (LL_M2M_IsActiveFlag(hm2m->Instance) != 1U) {
        /* Check for the Timeout. */
        if (Timeout != HAL_MAX_DELAY) {
            if ((Timeout == 0U) || ((HAL_GetMillis() - tickstart) > Timeout)) {
                /* Update error code. */
                hm2m->ErrorCode = HAL_M2M_ERROR_TIMEOUT;

                /* Change the M2M state. */
                hm2m->State = HAL_M2M_STATE_READY;

                /* Process Unlocked. */
                __HAL_UNLOCK(hm2m);

                return HAL_ERROR;
            }
        }
    }

    /* Change the M2M transfer complete flag. */
    LL_M2M_ClearFlag(hm2m->Instance);

    /* Change the M2M state. */
    hm2m->State = HAL_M2M_STATE_READY;

    /* Process unlocked */
    __HAL_UNLOCK(hm2m);

    return HAL_OK;
}

/**
 * @brief  Handles M2M interrupt request.
 * @param  hm2m: pointer to a M2M_HandleTypeDef structure that contains
 *               the configuration information.
 * @retval None
 */
void HAL_M2M_IRQHandler(M2M_HandleTypeDef *hm2m)
{
    /* Clear the transfer complete flag. */
    LL_M2M_ClearFlag(hm2m->Instance);

    /* Process Unlocked */
    __HAL_UNLOCK(hm2m);

    if(hm2m->XferCpltCallback != NULL) {
        /* Transfer complete callback */
        hm2m->XferCpltCallback(hm2m);
    }
}

/**
 * @brief  Register callbacks
 * @param  hm2m:       pointer to a M2M_HandleTypeDef structure that contains
 *                     the configuration information.
 * @param  CallbackID: User Callback identifier
 *                     a HAL_M2M_CallbackIDTypeDef ENUM as parameter.
 * @param  pCallback:  pointer to private callback function which has pointer to
 *                     a M2M_HandleTypeDef structure as parameter.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_M2M_RegisterCallback(M2M_HandleTypeDef *hm2m, HAL_M2M_CallbackIDTypeDef CallbackID, void (* pCallback)( M2M_HandleTypeDef * _hm2m))
{
    HAL_StatusTypeDef status = HAL_OK;

    /* Process locked */
    __HAL_LOCK(hm2m);

    if(HAL_M2M_STATE_READY == hm2m->State) {
        switch (CallbackID) {
            case HAL_M2M_XFER_CPLT_CB_ID:
                hm2m->XferCpltCallback = pCallback;
                break;
            case HAL_M2M_XFER_ABORT_CB_ID:
                hm2m->XferAbortCallback = pCallback;
                break;
            default:
                status = HAL_ERROR;
                break;
        }
    } else {
        status = HAL_ERROR;
    }

    /* Release Lock */
    __HAL_UNLOCK(hm2m);

    return status;
}

/**
 * @brief  UnRegister callbacks
 * @param  hm2m:       pointer to a M2M_HandleTypeDef structure that contains
 *                     the configuration information.
 * @param  CallbackID: User Callback identifier
 *                     a HAL_M2M_CallbackIDTypeDef ENUM as parameter.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_M2M_UnRegisterCallback(M2M_HandleTypeDef *hm2m, HAL_M2M_CallbackIDTypeDef CallbackID)
{
    HAL_StatusTypeDef status = HAL_OK;

    /* Process locked */
    __HAL_LOCK(hm2m);

    if (HAL_M2M_STATE_READY == hm2m->State) {
        switch (CallbackID) {
            case HAL_M2M_XFER_CPLT_CB_ID:
                hm2m->XferCpltCallback = NULL;
                break;
            case HAL_M2M_XFER_ABORT_CB_ID:
                hm2m->XferAbortCallback = NULL;
                break;
            default:
                status = HAL_ERROR;
                break;
        }
    } else {
        status = HAL_ERROR;
    }

    /* Release Lock */
    __HAL_UNLOCK(hm2m);

    return status;
}

/**
 * @}
 */

/** @defgroup M2M_Exported_Functions_Group3 State functions
 * @brief     State functions
 *
@verbatim
================================================================================
                    ##### State functions #####
================================================================================
    [..]  This subsection provides functions allowing to :
        (+) HAL_M2M_GetState()  : Return the M2M state.

@endverbatim
 * @{
 */

/**
 * @brief  Return the M2M handle state.
 * @param  hm2m: pointer to a M2M_HandleTypeDef structure that contains
 *               the configuration information.
 * @retval Return HAL M2M status.
 */
HAL_M2M_StateTypeDef HAL_M2M_GetState(const M2M_HandleTypeDef *hm2m)
{
    /* Return M2M handle state. */
    return hm2m->State;
}
/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup M2M_Private_Functions
 * @{
 */

/**
 * @brief  Sets the M2M Transfer parameter.
 * @param  hm2m:       pointer to a M2M_HandleTypeDef structure that contains
 *                     the configuration information.
 * @param  SrcAddress: The source memory Buffer address
 * @param  DstAddress: The destination memory Buffer address
 * @param  DataLength: The length of data to be transferred from source to destination
 * @retval None
 */
static void M2M_SetConfig(M2M_HandleTypeDef *hm2m, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
    /* Clear all flags. */
    LL_M2M_ClearFlag(hm2m->Instance);

    /* Set Dummy. */
    if (LL_M2M_IsEnabledDummy(hm2m->Instance) == 1U) {
        LL_M2M_SetDummyValue(hm2m->Instance, hm2m->Init.Dummy);
    }

    /* Configure M2M source address and destination address. */
    LL_M2M_SetSrcAddress(hm2m->Instance, SrcAddress);
    LL_M2M_SetDstAddress(hm2m->Instance, DstAddress);

    /* Configure M2M data length. */
    LL_M2M_SetDataLength(hm2m->Instance, DataLength);
}
/**
 * @}
 */

#endif /* HAL_M2M_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
