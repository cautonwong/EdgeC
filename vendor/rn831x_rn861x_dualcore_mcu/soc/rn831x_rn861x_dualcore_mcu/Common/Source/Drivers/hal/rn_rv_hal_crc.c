/**
 ******************************************************************************
 * @file   rn_rv_hal_crc.c
 * @author Renergy Software Team
 * @brief  CRC HAL module driver.
 *         This file provides firmware functions to manage the following
 *         functionalities of the Cyclic Redundancy Check (CRC) peripheral:
 *          + Initialization and de-initialization functions
 *          + Peripheral Control functions
 *          + Peripheral State functions
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
#include <string.h>

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @defgroup CRC CRC
 * @brief     CRC HAL module driver
 * @{
 */

#ifdef HAL_CRC_MODULE_ENABLED

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @defgroup CRC_Exported_Functions CRC Exported Functions
 * @{
 */

/** @defgroup CRC_Exported_Functions_Group1 Initialization and de-initialization functions
 * @brief     Initialization and Configuration functions
 *
@verbatim
================================================================================
            ##### Initialization and de-initialization functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) Initialize and configure the CRC.
        (+) De-initialize the CRC.

@endverbatim
 * @{
 */

/**
 * @brief  Initialize the CRC according to the specified
 *         parameters in the CRC_InitTypeDef and create the associated handle.
 * @param  hcrc CRC handle
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_CRC_Init(CRC_HandleTypeDef *hcrc)
{
    uint32_t tmp;

    /* Check the CRC handle allocation. */
    if (hcrc == NULL) {
        return HAL_ERROR;
    }

    /* Check the parameters. */
    assert_param(IS_CRC_ALL_INSTANCE(hcrc->Instance));

    if (LL_CRC_IsCalculationDone(hcrc->Instance) == 1U) {
        /* Allocate lock resource and initialize it */
        __HAL_UNLOCK(hcrc);

        /* Init the low level hardware */
        HAL_CRC_MspInit(hcrc);
    }

    /* Check the parameters. */
    assert_param(IS_CRC_POLYSIZE(hcrc->Init.PolynomialSize));
    assert_param(IS_CRC_REVERSE_TYPE(hcrc->Init.InputRevMode));
    assert_param(IS_CRC_DATA_WIDTH(hcrc->Init.DataAlignment));

    /* Clear the control register. */
    tmp = LL_CRC_ReadReg(hcrc->Instance, CTRL);
    tmp &= ((uint32_t)~(CRC_CTRL_POLYSIZE | CRC_CTRL_XOR | CRC_CTRL_REVOUT | CRC_CTRL_REVIN | CRC_CTRL_OPDW));
    LL_CRC_WriteReg(hcrc->Instance, CTRL, tmp);

    /* Set the init value. */
    if (hcrc->Init.InitialValue != 0U) {
        LL_CRC_SetInitValue(hcrc->Instance, hcrc->Init.InitialValue);
    } else {
        LL_CRC_SetInitValue(hcrc->Instance, 0U);
    }

    /* Set the polynomial size(CRC7/CRC8/CRC16/CRC32). */
    LL_CRC_SetPolynomialSize(hcrc->Instance, hcrc->Init.PolynomialSize);

    /**
     * If no initial value is configured before the current CRC calculation
     * starts, the initial value of the remainder will use the result of the
     * previous data calculation.
     * When enable DMA, the value will be automatically used as the initial
     * value of the remainder, and if necessary, you can change this value
     * before calculation.
     */
    if (hcrc->Init.XorValue != 0U) {
        LL_CRC_EnableXor(hcrc->Instance);
        LL_CRC_SetExclusiveORValue(hcrc->Instance, hcrc->Init.XorValue);
    } else {
        LL_CRC_DisableXor(hcrc->Instance);
    }

    /* Enable the output reverse. */
    if (hcrc->Init.OutputRevEnable == CRC_OUTPUT_REVERSE_ENABLE) {
        LL_CRC_EnableOutputReverse(hcrc->Instance);
    } else {
        LL_CRC_DisableOutputReverse(hcrc->Instance);
    }

    /* Configure the input reverse mode. */
    if ((hcrc->Init.InputRevMode != CRC_REVERSE_IN_BYTE) && \
        (hcrc->Init.InputRevMode != CRC_REVERSE_IN_HALFWORD) && \
        (hcrc->Init.InputRevMode != CRC_REVERSE_IN_WORD)) {
        LL_CRC_SetInputRevMode(hcrc->Instance, CRC_NOREVERSE);
    } else {
        LL_CRC_SetInputRevMode(hcrc->Instance, hcrc->Init.InputRevMode);
    }

    /* Set the data width of CRC operation. */
    LL_CRC_SetDataWidth(hcrc->Instance, hcrc->Init.DataAlignment);

    /* Set the polynomial. */
    LL_CRC_SetPolynomial(hcrc->Instance, hcrc->Init.Polynomial);

    /* Set the dma mode. */
    if (hcrc->Init.DmaMode != CRC_DMA_DISABLE) {
        if (hcrc->Instance == CRC1) {
            /* CRC1 does not support DMA. */
            return HAL_ERROR;
        }

        if (hcrc->Init.DmaMode == CRC_DMA_MODE_CHKSUM) {
            LL_CRC_SetDMAMode(hcrc->Instance, CRC_DMA_MODE_CHKSUM);
        } else {
            LL_CRC_SetDMAMode(hcrc->Instance, CRC_DMA_MODE_CRC);
        }
        LL_CRC_EnableDMA(hcrc->Instance);

    } else {
        LL_CRC_DisableDMA(hcrc->Instance);
    }

    /* Change CRC peripheral state */
    hcrc->State = HAL_CRC_STATE_READY;

    /* Return function status */
    return HAL_OK;
}

/**
 * @brief  DeInitialize the CRC peripheral.
 * @param  hcrc CRC handle
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_CRC_DeInit(CRC_HandleTypeDef *hcrc)
{
    /* Check the CRC handle allocation. */
    if (hcrc == NULL) {
        return HAL_ERROR;
    }

    /* Check the parameters. */
    assert_param(IS_CRC_ALL_INSTANCE(hcrc->Instance));

    /* Check the CRC peripheral state. */
    if (hcrc->State == HAL_CRC_STATE_BUSY) {
        return HAL_BUSY;
    }

    /* Change CRC peripheral state */
    hcrc->State = HAL_CRC_STATE_BUSY;

#ifdef USE_FULL_LL_DRIVER
    __HAL_SYSTEM_PSW_UNLOCK();
    LL_CRC_DeInit(hcrc->Instance);
    __HAL_SYSTEM_PSW_LOCK();
#endif

    /* DeInit the low level hardware */
    HAL_CRC_MspDeInit(hcrc);

    /* Change CRC peripheral state */
    hcrc->State = HAL_CRC_STATE_RESET;

    /* Process unlocked */
    __HAL_UNLOCK(hcrc);

    /* Return function status. */
    return HAL_OK;
}

/**
 * @brief  Initializes the CRC MSP.
 * @param  hcrc CRC handle
 * @retval None
 */
__WEAK void HAL_CRC_MspInit(CRC_HandleTypeDef *hcrc)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hcrc);

    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_CRC_MspInit can be implemented in the user file. */
}

/**
 * @brief  DeInitialize the CRC MSP.
 * @param  hcrc CRC handle
 * @retval None
 */
__WEAK void HAL_CRC_MspDeInit(CRC_HandleTypeDef *hcrc)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hcrc);

    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_CRC_MspDeInit can be implemented in the user file. */
}

/**
 * @}
 */

/** @defgroup CRC_Exported_Functions_Group2 IO operation functions
 * @brief     IO operation functions.
 *
@verbatim
================================================================================
                    ##### IO operation functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) HAL_CRC_Accumulate                  : Compute CRC
        (+) HAL_CRC_Calculate                   : Compute CRC with init value

@endverbatim
 * @{
 */

/**
 * @brief  Compute the CRC value of a data buffer starting with the previously computed CRC as initialization value.
 * @param  hcrc CRC handle
 * @param  pBuffer pointer to the input data buffer.
 * @param  BufferLength input data buffer length in bytes.
 * @retval uint32_t CRC (returned value LSBs for CRC shorter than 32 bits)
 */
uint32_t HAL_CRC_Accumulate(CRC_HandleTypeDef *hcrc, uint8_t pBuffer[], uint32_t BufferLength)
{
    uint32_t index;      /* CRC input data buffer index. */
    uint32_t temp = 0U;  /* CRC output (read from hcrc->Instance->DR register). */
    uint32_t data_words_len, extra_data_len;

    /* Lock it */
    __HAL_LOCK(hcrc);

    /* Enter Data to the CRC calculator. */
    if (hcrc->Init.DataAlignment == CRC_POLYSIZE_BIT32) {
        /* Add data in word. */
        extra_data_len = 0U;
        data_words_len = (BufferLength >> 2U);
    } else if (hcrc->Init.DataAlignment == CRC_POLYSIZE_BIT16) {
        /* Add data in half-word. */
        extra_data_len = (BufferLength & 0x1);
        data_words_len = ((BufferLength - extra_data_len) >> 2U);
    } else {
        /* Default: Add data in byte. */
        extra_data_len = (BufferLength & 0x3);
        data_words_len = ((BufferLength - extra_data_len) >> 2U);
    }
    for (index = 0U; index < data_words_len; index++) {
        memcpy(&temp, (pBuffer + (index * 4)), 4);
        LL_CRC_FeedData32(hcrc->Instance, temp);
        __NOP();
    }

    if (extra_data_len != 0U) {
        temp = 0U;
        memcpy(&temp, (pBuffer + (data_words_len * 4)), extra_data_len);
        LL_CRC_FeedData32(hcrc->Instance, temp);
    }

    while (LL_CRC_IsCalculationDone(hcrc->Instance) != HAL_CRC_STATE_READY);
    hcrc->State = HAL_CRC_STATE_READY;

    if (hcrc->Init.PolynomialSize == CRC_POLYSIZE_BIT7) {
        temp = (uint32_t)(LL_CRC_ReadData32(hcrc->Instance) & 0x7FU);
    } else if (hcrc->Init.PolynomialSize == CRC_POLYSIZE_BIT8) {
        temp = (uint32_t)(LL_CRC_ReadData32(hcrc->Instance) & 0xFFU);
    } else if (hcrc->Init.PolynomialSize == CRC_POLYSIZE_BIT16) {
        temp = (uint32_t)(LL_CRC_ReadData32(hcrc->Instance) & 0xFFFFU);
    } else {
        temp = (uint32_t)LL_CRC_ReadData32(hcrc->Instance);
    }

    /* Release it */
    __HAL_UNLOCK(hcrc);

    /* Return the CRC computed value. */
    return temp;
}

/**
 * @brief  Compute the CRC value of a data buffer starting with hcrc->Instance->INIT as initialization value.
 * @param  hcrc CRC handle
 * @param  pBuffer pointer to the input data buffer.
 * @param  BufferLength input data buffer length in bytes.
 * @retval uint32_t CRC (returned value LSBs for CRC shorter than 32 bits)
 */
uint32_t HAL_CRC_Calculate(CRC_HandleTypeDef *hcrc, uint8_t pBuffer[], uint32_t BufferLength)
{
    uint32_t index;      /* CRC input data buffer index. */
    uint32_t temp = 0U;  /* CRC output (read from hcrc->Instance->DR register). */
    uint32_t data_words_len, extra_data_len;

    /* Lock it */
    __HAL_LOCK(hcrc);

    /* Reset CRC Calculation Unit (hcrc->Instance->INIT. */
    LL_CRC_SetInitValue(hcrc->Instance, hcrc->Init.InitialValue);

    /* Enter Data to the CRC calculator. */
    if (hcrc->Init.DataAlignment == CRC_POLYSIZE_BIT32) {
        /* Add data in word. */
        extra_data_len = 0U;
        data_words_len = (BufferLength >> 2U);
    } else if (hcrc->Init.DataAlignment == CRC_POLYSIZE_BIT16) {
        /* Add data in half-word. */
        extra_data_len = (BufferLength & 0x1);
        data_words_len = ((BufferLength - extra_data_len) >> 2U);
    } else {
        /* Default: Add data in byte. */
        extra_data_len = (BufferLength & 0x3);
        data_words_len = ((BufferLength - extra_data_len) >> 2U);
    }
    for (index = 0U; index < data_words_len; index++) {
        memcpy(&temp, (pBuffer + (index * 4)), 4);
        LL_CRC_FeedData32(hcrc->Instance, temp);
        __NOP();
    }

    if (extra_data_len != 0U) {
        temp = 0U;
        memcpy(&temp, (pBuffer + (data_words_len * 4)), extra_data_len);
        LL_CRC_FeedData32(hcrc->Instance, temp);
    }

    while (LL_CRC_IsCalculationDone(hcrc->Instance) != HAL_CRC_STATE_READY);
    hcrc->State = HAL_CRC_STATE_READY;

    if (hcrc->Init.PolynomialSize == CRC_POLYSIZE_BIT7) {
        temp = (uint32_t)(LL_CRC_ReadData32(hcrc->Instance) & 0x7FU);
    } else if (hcrc->Init.PolynomialSize == CRC_POLYSIZE_BIT8) {
        temp = (uint32_t)(LL_CRC_ReadData32(hcrc->Instance) & 0xFFU);
    } else if (hcrc->Init.PolynomialSize == CRC_POLYSIZE_BIT16) {
        temp = (uint32_t)(LL_CRC_ReadData32(hcrc->Instance) & 0xFFFFU);
    } else {
        temp = (uint32_t)LL_CRC_ReadData32(hcrc->Instance);
    }

    /* Release it */
    __HAL_UNLOCK(hcrc);

    /* Return the CRC computed value. */
    return temp;
}

/**
 * @}
 */

/** @defgroup CRC_Exported_Functions_Group3 State functions
 * @brief     State functions
 *
@verbatim
================================================================================
                    ##### State functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) HAL_CRC_GetState                    : Get the state

@endverbatim
 * @{
 */

/**
 * @brief  Return the CRC handle state.
 * @param  hcrc CRC handle
 * @retval HAL state
 */
HAL_CRC_StateTypeDef HAL_CRC_GetState(const CRC_HandleTypeDef *hcrc)
{
    /* Return CRC handle state. */
    return hcrc->State;
}

/**
 * @}
 */

/**
 * @}
 */

#endif /* HAL_CRC_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
