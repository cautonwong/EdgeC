/**
 ******************************************************************************
 * @file   rn_rv_hal_dma.c
 * @author Renergy Software Team
 * @brief  DMA HAL module driver.
 *         This file provides firmware functions to manage the following
 *         functionalities of the Direct Memory Access (DMA) peripheral:
 *           + Initialization and de-initialization functions
 *           + IO operation functions
 *           + Peripheral State and errors functions
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
#include "rn_rv_ll_usart.h"
#include "rn_rv_ll_qspi.h"
#include "rn_rv_ll_crc.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @defgroup DMA DMA
 * @brief     DMA HAL module driver
 * @{
 */

#ifdef HAL_DMA_MODULE_ENABLED

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/** @defgroup DMA_Private_Define DMA Private definations
 * @{
 */
/*!< DMA00 and DMA10, valid in core0 */
#define DMA_QSPI0_RX_PERIPH_BASE(__INSTANCE__)  QSPI0_RX_DATA_ADDR
#define DMA_QSPI0_TX_PERIPH_BASE(__INSTANCE__)  QSPI0_TX_DATA_ADDR
#define DMA_USART0_RX_PERIPH_BASE(__INSTANCE__) (((__INSTANCE__) == DMA00) \
                                                ? USART0_RX_DATA_CORE0_ADDR \
                                                : USART0_RX_DATA_CORE1_ADDR)
#define DMA_USART0_TX_PERIPH_BASE(__INSTANCE__) (((__INSTANCE__) == DMA00) \
                                                ? USART0_TX_DATA_CORE0_ADDR \
                                                : USART0_TX_DATA_CORE1_ADDR)

/*!< DMA01 and DMA11, valid in core1 */
#define DMA_QSPI1_RX_PERIPH_BASE(__INSTANCE__)  QSPI1_RX_DATA_ADDR
#define DMA_QSPI1_TX_PERIPH_BASE(__INSTANCE__)  QSPI1_TX_DATA_ADDR
#define DMA_USART1_RX_PERIPH_BASE(__INSTANCE__) (((__INSTANCE__) == DMA01) \
                                                ? USART1_RX_DATA_CORE0_ADDR \
                                                : USART1_RX_DATA_CORE1_ADDR)
#define DMA_USART1_TX_PERIPH_BASE(__INSTANCE__) (((__INSTANCE__) == DMA01) \
                                                ? USART1_TX_DATA_CORE0_ADDR \
                                                : USART1_TX_DATA_CORE1_ADDR)
#define DMA_CRC0_PERIPH_BASE(__INSTANCE__)      (((__INSTANCE__) == DMA01) \
                                                ? CRC0_DATA_CORE0_ADDR \
                                                : CRC0_DATA_CORE1_ADDR)
/**
 * @}
 */

/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/** @defgroup DMA_Private_Functions DMA Private Functions
 * @{
 */
static void DMA_SetConfig(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);
static void DMA_EnableChannel(DMA_HandleTypeDef *hdma);
static void DMA_DisableChannel(DMA_HandleTypeDef *hdma);
/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup DMA_Exported_Functions DMA Exported Functions
 * @{
 */

/** @defgroup DMA_Exported_Functions_Group1 Initialization and de-initialization functions
 * @brief     Initialization and Configuration functions
 *
@verbatim
================================================================================
            ##### Initialization and de-initialization functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) Initialize and configure the DMA.
        (+) De-initialize the DMA.

@endverbatim
 * @{
 */

/**
 * @brief  Initialize the DMA according to the specified parameters in the
 *         DMA_InitTypeDef and initialize the associated handle.
 * @param  hdma: Pointer to a DMA_HandleTypeDef structure that contains
 *               the configuration information for the specified DMA Channel.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef *hdma)
{
    uint32_t cr_tmp = 0U;

    /* Check the DMA handle allocation. */
    if (hdma == NULL) {
        return HAL_ERROR;
    }

    /* Check the DMA instance with its direction. */
    if ((hdma->Init.Direction == DMA_MEMORY_TO_MEMORY) && \
        ((hdma->Instance == DMA00) || (hdma->Instance == DMA10))) {
        return HAL_ERROR;
    }

    assert_param(IS_DMA_ALL_INSTANCE(hdma->Instance));
    assert_param(IS_DMA_DIRECTION(hdma->Instance, hdma->Init.Direction));
    assert_param(IS_DMA_MODE(hdma->Instance, hdma->Init.Mode));
    assert_param(IS_DMA_PRIORITY(hdma->Instance, hdma->Init.Priority));
    assert_param(IS_DMA_PERIPHERAL_DATA_SIZE(hdma->Instance, hdma->Init.PeriphDataAlignment));
    assert_param(IS_DMA_MEMORY_DATA_SIZE(hdma->Instance, hdma->Init.MemDataAlignment));
    assert_param(IS_DMA_CHANNEL_DIR_VALID(hdma->Instance, hdma->Init.Direction));

    /* Change DMA peripheral state. */
    hdma->State = HAL_DMA_STATE_BUSY;

    /* Call the initialization function. */
    HAL_DMA_MspInit(hdma);

    /* Get the preset configuration. */
    switch (hdma->Channel.Type) {
        case (HAL_DMA_CHANNEL_QSPI0_RX):
            hdma->Init.Direction                = DMA_PERIPH_TO_MEMORY;
            hdma->Channel.Index                 = DMA_CHANNEL_0;
            break;

        case HAL_DMA_CHANNEL_USART0_RX:
            hdma->Init.PeriphDataAlignment      = DMA_PDATAALIGN_BYTE;
            hdma->Init.Direction                = DMA_PERIPH_TO_MEMORY;
            hdma->Channel.Index                 = DMA_CHANNEL_1;
            break;

        case HAL_DMA_CHANNEL_QSPI0_TX:
            hdma->Init.Direction                = DMA_MEMORY_TO_PERIPH;
            hdma->Channel.Index                 = DMA_CHANNEL_2;
            break;

        case HAL_DMA_CHANNEL_USART0_TX:
            hdma->Init.PeriphDataAlignment      = DMA_PDATAALIGN_BYTE;
            hdma->Init.Direction                = DMA_MEMORY_TO_PERIPH;
            hdma->Channel.Index                 = DMA_CHANNEL_3;
            break;

        case HAL_DMA_CHANNEL_QSPI1_RX:
            hdma->Init.Direction                = DMA_PERIPH_TO_MEMORY;
            hdma->Channel.Index                 = DMA_CHANNEL_0;
            break;

        case HAL_DMA_CHANNEL_USART1_RX:
            hdma->Init.PeriphDataAlignment      = DMA_PDATAALIGN_BYTE;
            hdma->Init.Direction                = DMA_PERIPH_TO_MEMORY;
            hdma->Channel.Index                 = DMA_CHANNEL_1;
            break;

        case HAL_DMA_CHANNEL_QSPI1_TX:
            hdma->Init.Direction                = DMA_MEMORY_TO_PERIPH;
            hdma->Channel.Index                 = DMA_CHANNEL_2;
            break;

        case HAL_DMA_CHANNEL_USART1_TX:
            hdma->Init.PeriphDataAlignment      = DMA_PDATAALIGN_BYTE;
            hdma->Init.Direction                = DMA_MEMORY_TO_PERIPH;
            hdma->Channel.Index                 = DMA_CHANNEL_3;
            break;

        case HAL_DMA_CHANNEL_SRAM:
            hdma->Init.Direction                = DMA_MEMORY_TO_MEMORY;
            hdma->Channel.Index                 = DMA_CHANNEL_5;
            break;

        case HAL_DMA_CHANNEL_CRC:
            hdma->Init.PeriphDataAlignment      = DMA_PDATAALIGN_BYTE;
            hdma->Init.MemDataAlignment         = DMA_PDATAALIGN_BYTE;
            hdma->Init.Direction                = DMA_MEMORY_TO_MEMORY;
            hdma->Channel.Index                 = DMA_CHANNEL_6;
            break;

        default:
            return HAL_ERROR;
    }

    /* Get the specified mode. */
    if (hdma->Init.Mode != DMA_CIRCULAR) {
        cr_tmp |= DMA_NORMAL;
    } else {
        cr_tmp |= DMA_CIRCULAR;
    }

    /* Get the specified direction. */
    if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
        cr_tmp |= hdma->Init.Direction;
    }

    /* Get the specified priority. */
    if ((hdma->Init.Priority != DMA_PRIORITY_VERY_HIGH) && \
        (hdma->Init.Priority != DMA_PRIORITY_HIGH) && \
        (hdma->Init.Priority != DMA_PRIORITY_LOW)) {
        cr_tmp |= DMA_PRIORITY_MEDIUM;
    } else {
        cr_tmp |= hdma->Init.Priority;
    }

    if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
        /* Get the specified data alignment. */
        if ((hdma->Init.PeriphDataAlignment != DMA_PDATAALIGN_HALFWORD) && \
            (hdma->Init.PeriphDataAlignment != DMA_PDATAALIGN_WORD)) {
            cr_tmp |= DMA_PDATAALIGN_BYTE;
        } else {
            cr_tmp |= hdma->Init.PeriphDataAlignment;
        }

        /* Set the channel control register. */
        LL_DMA_ConfigTransfer(hdma->Instance, hdma->Channel.Index, cr_tmp);

    } else {
        /* Get the specified data alignment. */
        if ((hdma->Init.MemDataAlignment != DMA_MDATAALIGN_HALFWORD) && \
            (hdma->Init.MemDataAlignment != DMA_MDATAALIGN_WORD)) {
            cr_tmp |= DMA_MDATAALIGN_BYTE;
        } else {
            cr_tmp |= hdma->Init.MemDataAlignment;
        }

        /* Set the channel control register. */
        LL_DMA_ConfigM2MTransfer(hdma->Instance, hdma->Channel.Index, cr_tmp);
    }

    /* Initialize the error code */
    hdma->ErrorCode = HAL_DMA_ERROR_NONE;

    /* Initialize the DMA state. */
    hdma->State = HAL_DMA_STATE_READY;

    /* Allocate lock resource and initialize it. */
    hdma->Lock = HAL_UNLOCKED;

    return HAL_OK;
}

/**
 * @brief  De-Initialize the DMA peripheral.
 * @param  hdma: pointer to a DMA_HandleTypeDef structure that contains
 *               the configuration information for the specified DMA Channel.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_DMA_DeInit(DMA_HandleTypeDef *hdma)
{
    /* Check the DMA handle allocation */
    if (hdma == NULL) {
        return HAL_ERROR;
    }

#ifdef USE_FULL_LL_DRIVER
    __HAL_SYSTEM_PSW_UNLOCK();
    LL_DMA_DeInit(hdma->Instance);
    __HAL_SYSTEM_PSW_LOCK();
#endif

    /* Call the De-initialization function. */
    HAL_DMA_MspDeInit(hdma);

    /* Clean all callbacks. */
    hdma->XferCpltCallback = NULL;
    hdma->XferHalfCpltCallback = NULL;
    hdma->XferErrorCallback = NULL;
    hdma->XferAbortCallback = NULL;

    /* Reset the error code. */
    hdma->ErrorCode = HAL_DMA_ERROR_NONE;

    /* Reset the DMA state. */
    hdma->State = HAL_DMA_STATE_RESET;

    /* Release Lock. */
    __HAL_UNLOCK(hdma);

    return HAL_OK;
}

/**
 * @brief  Initializes the DMA MSP.
 * @param  hdma: pointer to a DMA_HandleTypeDef structure that contains
 *               the configuration information for the specified DMA Channel.
 * @retval None
 */
__WEAK void HAL_DMA_MspInit(DMA_HandleTypeDef *hdma)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hdma);

    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_DMA_MspInit can be implemented in the user file. */
}

/**
 * @brief  DeInitialize the DMA MSP.
 * @param  hdma: pointer to a DMA_HandleTypeDef structure that contains
 *               the configuration information for the specified DMA Channel.
 * @retval None
 */
__WEAK void HAL_DMA_MspDeInit(DMA_HandleTypeDef *hdma)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hdma);

    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_DMA_MspDeInit can be implemented in the user file. */
}

/**
 * @}
 */

/** @defgroup DMA_Exported_Functions_Group2 IO operation functions
 * @brief     IO operation functions.
 *
@verbatim
================================================================================
                    ##### IO operation functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) Configure the source, destination address and data length and Start DMA transfer
        (+) Configure the source, destination address and data length and Start DMA transfer with interrupt
        (+) Abort DMA transfer
        (+) Poll for transfer complete
        (+) Handle DMA interrupt request

@endverbatim
 * @{
 */

/**
 * @brief  Start the DMA Transfer.
 * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
 *                     the configuration information for the specified DMA Channel.
 * @param  SrcAddress: The source memory Buffer address
 * @param  DstAddress: The destination memory Buffer address
 * @param  DataLength: The length of data to be transferred from source to destination
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_DMA_StartChannel(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t channel = hdma->Channel.Index;

    /* Check the parameters. */
    assert_param(IS_DMA_BUFFER_SIZE(DataLength));

    /* Process locked. */
    __HAL_LOCK(hdma);

    channel = ((hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) \
                ? (channel * 4) \
                : ((channel - 5) * 4)); // interrupt flag offset

    if (HAL_DMA_STATE_READY == hdma->State) {
        /* Change DMA peripheral state. */
        hdma->State = HAL_DMA_STATE_BUSY;
        hdma->ErrorCode = HAL_DMA_ERROR_NONE;

        /* Clear all flags */
        if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
            LL_DMA_ClearFlag(hdma->Instance, ((LL_DMA_IF_HTIF0 | LL_DMA_IF_TCIF0 | LL_DMA_IF_TEIF0) << channel));
        } else {
            LL_DMA_ClearM2MFlag(hdma->Instance, ((LL_DMA_MIF_HTIF0 | LL_DMA_MIF_TCIF0 | LL_DMA_MIF_TEIF0) << channel));
        }

        /* Disable the peripheral. */
        DMA_DisableChannel(hdma);

        /* Configure the source, destination address and the data length & clear flags. */
        DMA_SetConfig(hdma, SrcAddress, DstAddress, DataLength);

        /* Enable the Peripheral. */
        DMA_EnableChannel(hdma);

    } else {
        /* Process Unlocked. */
        __HAL_UNLOCK(hdma);

        status = HAL_BUSY;
    }

    return status;
}

/**
 * @brief  Start the DMA Transfer with interrupt enabled.
 * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
 *                     the configuration information for the specified DMA Channel.
 * @param  SrcAddress: The source memory Buffer address
 * @param  DstAddress: The destination memory Buffer address
 * @param  DataLength: The length of data to be transferred from source to destination
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_DMA_StartChannel_IT(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t channel = hdma->Channel.Index;

    /* Check the parameters. */
    assert_param(IS_DMA_BUFFER_SIZE(DataLength));
    assert_param(IS_DMA_CHANNEL_DIR_VALID(channel, hdma->Init.Direction));

    /* Process locked. */
    __HAL_LOCK(hdma);

    channel = ((hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) \
                ? (channel * 4) \
                : ((channel - 5) * 4)); // interrupt flag offset

    if (HAL_DMA_STATE_READY == hdma->State) {
        /* Change DMA peripheral state. */
        hdma->State = HAL_DMA_STATE_BUSY;
        hdma->ErrorCode = HAL_DMA_ERROR_NONE;

        /* Clear all flags */
        if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
            LL_DMA_ClearFlag(hdma->Instance, ((LL_DMA_IF_HTIF0 | LL_DMA_IF_TCIF0 | LL_DMA_IF_TEIF0) << channel));
        } else {
            LL_DMA_ClearM2MFlag(hdma->Instance, ((LL_DMA_MIF_HTIF0 | LL_DMA_MIF_TCIF0 | LL_DMA_MIF_TEIF0) << channel));
        }

        /* Disable the peripheral. */
        DMA_DisableChannel(hdma);

        /* Configure the source, destination address and the data length & clear flags. */
        DMA_SetConfig(hdma, SrcAddress, DstAddress, DataLength);

        /* Enable transfer complete interrupt and transfer Error interrupt. */
        if (NULL != hdma->XferHalfCpltCallback) {
            /* Enable the Half transfer complete interrupt as well. */
            if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
                LL_DMA_EnableIT(hdma->Instance, ((LL_DMA_IE_HTIE0 | LL_DMA_IE_TCIE0 | LL_DMA_IE_TEIE0) << channel));
            } else {
                LL_DMA_EnableM2MIT(hdma->Instance, ((LL_DMA_MIE_HTIE0 | LL_DMA_MIE_TCIE0 | LL_DMA_MIE_TEIE0) << channel));
            }
        } else {
            if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
                LL_DMA_EnableIT(hdma->Instance, ((LL_DMA_IE_TCIE0 | LL_DMA_IE_TEIE0) << channel));
            } else {
                LL_DMA_EnableM2MIT(hdma->Instance, ((LL_DMA_MIE_TCIE0 | LL_DMA_MIE_TEIE0) << channel));
            }
        }

        /* Enable the Peripheral. */
        DMA_EnableChannel(hdma);

    } else {
        /* Process Unlocked. */
        __HAL_UNLOCK(hdma);

        /* Remain BUSY. */
        status = HAL_BUSY;
    }

    return status;
}

/**
 * @brief  Abort the DMA Transfer.
 * @param  hdma: pointer to a DMA_HandleTypeDef structure that contains
 *               the configuration information for the specified DMA Channel.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_DMA_Abort(DMA_HandleTypeDef *hdma)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t channel = hdma->Channel.Index;

    channel = ((hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) \
                ? (channel * 4) \
                : ((channel - 5) * 4)); // interrupt flag offset

    if (hdma->State != HAL_DMA_STATE_BUSY) {
        /* no transfer ongoing */
        hdma->ErrorCode = HAL_DMA_ERROR_NO_XFER;

        /* Process Unlocked */
        __HAL_UNLOCK(hdma);

        return HAL_ERROR;

    } else {
        /* Disable DMA IT */
        if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
            LL_DMA_DisableIT(hdma->Instance, ((LL_DMA_IE_HTIE0 | LL_DMA_IE_TCIE0 | LL_DMA_IE_TEIE0) << channel));
        } else {
            LL_DMA_DisableM2MIT(hdma->Instance, ((LL_DMA_MIE_HTIE0 | LL_DMA_MIE_TCIE0 | LL_DMA_MIE_TEIE0) << channel));
        }

        /* Disable the channel */
        DMA_DisableChannel(hdma);

        /* Clear all flags */
        if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
            LL_DMA_ClearFlag(hdma->Instance, ((LL_DMA_IF_HTIF0 | LL_DMA_IF_TCIF0 | LL_DMA_IF_TEIF0) << channel));
        } else {
            LL_DMA_ClearM2MFlag(hdma->Instance, ((LL_DMA_MIF_HTIF0 | LL_DMA_MIF_TCIF0 | LL_DMA_MIF_TEIF0) << channel));
        }
    }

    /* Change the DMA state */
    hdma->State = HAL_DMA_STATE_READY;

    /* Process Unlocked */
    __HAL_UNLOCK(hdma);

    return status;
}

/**
 * @brief  Aborts the DMA Transfer in Interrupt mode.
 * @param  hdma: pointer to a DMA_HandleTypeDef structure that contains
 *               the configuration information for the specified DMA Channel.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_DMA_Abort_IT(DMA_HandleTypeDef *hdma)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t channel = hdma->Channel.Index;

    channel = ((hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) \
                ? (channel * 4) \
                : ((channel - 5) * 4)); // interrupt flag offset

    if (hdma->State != HAL_DMA_STATE_BUSY) {
        /* no transfer ongoing */
        hdma->ErrorCode = HAL_DMA_ERROR_NO_XFER;

        /* Process Unlocked */
        __HAL_UNLOCK(hdma);

        return HAL_ERROR;

    } else {
        /* Disable DMA IT */
        if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
            LL_DMA_DisableIT(hdma->Instance, ((LL_DMA_IE_HTIE0 | LL_DMA_IE_TCIE0 | LL_DMA_IE_TEIE0) << channel));
        } else {
            LL_DMA_DisableM2MIT(hdma->Instance, ((LL_DMA_MIE_HTIE0 | LL_DMA_MIE_TCIE0 | LL_DMA_MIE_TEIE0) << channel));
        }

        /* Disable the channel */
        DMA_DisableChannel(hdma);

        /* Clear all flags */
        if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
            LL_DMA_ClearFlag(hdma->Instance, ((LL_DMA_IF_HTIF0 | LL_DMA_IF_TCIF0 | LL_DMA_IF_TEIF0) << channel));
        } else {
            LL_DMA_ClearM2MFlag(hdma->Instance, ((LL_DMA_MIF_HTIF0 | LL_DMA_MIF_TCIF0 | LL_DMA_MIF_TEIF0) << channel));
        }

        /* Change the DMA state */
        hdma->State = HAL_DMA_STATE_READY;

        /* Process Unlocked */
        __HAL_UNLOCK(hdma);

        /* Call User Abort callback */
        if (hdma->XferAbortCallback != NULL) {
            hdma->XferAbortCallback(hdma);
        }
    }

    return status;
}

/**
 * @brief  Polling for transfer complete.
 * @param  hdma:          pointer to a DMA_HandleTypeDef structure that contains
 *                        the configuration information for the specified DMA Channel.
 * @param  CompleteLevel: Specifies the DMA level complete,
 *                        this parameter can be one of the following values:
 *                          @arg @ref HAL_DMA_FULL_TRANSFER
 *                          @arg @ref HAL_DMA_HALF_TRANSFER
 * @param  Timeout:       Timeout duration in milliseconds.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_DMA_PollForTransfer(DMA_HandleTypeDef *hdma, uint32_t CompleteLevel, uint32_t Timeout)
{
    uint32_t temp, tickstart = 0U;
    uint8_t channel = hdma->Channel.Index;

    if (hdma->State != HAL_DMA_STATE_BUSY) {
        /* no transfer ongoing. */
        hdma->ErrorCode = HAL_DMA_ERROR_NO_XFER;

        /* Process Unlocked. */
        __HAL_UNLOCK(hdma);

        return HAL_ERROR;
    }

    /* Polling mode not supported in circular mode. */
    if (((hdma->Init.Direction == DMA_MEMORY_TO_MEMORY) && \
        (LL_DMA_GetM2MMode(hdma->Instance, hdma->Channel.Index) == DMA_CIRCULAR)) || \
        ((hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) && \
        (LL_DMA_GetMode(hdma->Instance, hdma->Channel.Index) == DMA_CIRCULAR))) {

        /* no support. */
        hdma->ErrorCode = HAL_DMA_ERROR_NOT_SUPPORTED;

        return HAL_ERROR;
    }

    /* Get the level transfer complete flag. */
    temp = ((CompleteLevel == HAL_DMA_FULL_TRANSFER) ? LL_DMA_IF_TCIF0 : LL_DMA_IF_HTIF0);

    channel = ((hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) \
                ? (channel * 4) \
                : ((channel - 5) * 4)); // interrupt flag offset

    /* Get tick. */
    tickstart = HAL_GetMillis();
    if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
        while (LL_DMA_IsActiveFlag(hdma->Instance, (temp << channel)) != 1U) {
            if (LL_DMA_IsActiveFlag(hdma->Instance, (LL_DMA_IF_TEIF0 << channel))) {
                /* Clear all flags. */
                LL_DMA_ClearFlag(hdma->Instance, ((LL_DMA_IF_HTIF0 | LL_DMA_IF_TCIF0 | LL_DMA_IF_TEIF0) << channel));

                /* Update error code. */
                hdma->ErrorCode = HAL_DMA_ERROR_TE;

                /* Change the DMA state. */
                hdma->State= HAL_DMA_STATE_READY;

                /* Process Unlocked. */
                __HAL_UNLOCK(hdma);

                return HAL_ERROR;
            }

            /* Check for the Timeout. */
            if (Timeout != HAL_MAX_DELAY) {
                if ((Timeout == 0U) || ((HAL_GetMillis() - tickstart) > Timeout)) {
                    /* Update error code. */
                    hdma->ErrorCode = HAL_DMA_ERROR_TIMEOUT;

                    /* Change the DMA state. */
                    hdma->State = HAL_DMA_STATE_READY;

                    /* Process Unlocked. */
                    __HAL_UNLOCK(hdma);

                    return HAL_ERROR;
                }
            }
        }

    } else {
        while (LL_DMA_IsActiveM2MFlag(hdma->Instance, (temp << channel)) != 1U) {
            if (LL_DMA_IsActiveM2MFlag(hdma->Instance, (LL_DMA_MIF_TEIF0 << channel))) {
                /* Clear all flags. */
                LL_DMA_ClearM2MFlag(hdma->Instance, ((LL_DMA_MIF_HTIF0 | LL_DMA_MIF_TCIF0 | LL_DMA_MIF_TEIF0) << channel));

                /* Update error code. */
                hdma->ErrorCode = HAL_DMA_ERROR_TE;

                /* Change the DMA state. */
                hdma->State= HAL_DMA_STATE_READY;

                /* Process Unlocked. */
                __HAL_UNLOCK(hdma);

                return HAL_ERROR;
            }

            /* Check for the Timeout. */
            if (Timeout != HAL_MAX_DELAY) {
                if ((Timeout == 0U) || ((HAL_GetMillis() - tickstart) > Timeout)) {
                    /* Update error code. */
                    hdma->ErrorCode = HAL_DMA_ERROR_TIMEOUT;

                    /* Change the DMA state. */
                    hdma->State = HAL_DMA_STATE_READY;

                    /* Process Unlocked. */
                    __HAL_UNLOCK(hdma);

                    return HAL_ERROR;
                }
            }
        }
    }

    if (CompleteLevel == HAL_DMA_FULL_TRANSFER) {
        /* Clear the transfer complete flag. */
        if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
            LL_DMA_ClearFlag(hdma->Instance, (LL_DMA_IF_TCIF0 << channel));
        } else {
            LL_DMA_ClearM2MFlag(hdma->Instance, (LL_DMA_MIF_TCIF0 << channel));
        }

        /* The selected Channelx EN bit is cleared (DMA is disabled and all transfers are complete). */
        hdma->State = HAL_DMA_STATE_READY;

    } else {
        /* Clear the half transfer complete flag. */
        if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
            LL_DMA_ClearFlag(hdma->Instance, (LL_DMA_IF_HTIF0 << channel));
        } else {
            LL_DMA_ClearM2MFlag(hdma->Instance, (LL_DMA_IF_HTIF0 << channel));
        }
    }

    if (hdma->Channel.Index == DMA_CHANNEL_6) {
        LL_CRC_DisableDMA(CRC0); // Disable CRC0 DMA_CTRL register finally
    }

    /* Process unlocked */
    __HAL_UNLOCK(hdma);

    return HAL_OK;
}

/**
 * @brief  Handles DMA interrupt request.
 * @param  hdma: pointer to a DMA_HandleTypeDef structure that contains
 *               the configuration information for the specified DMA Channel.
 * @retval None
 */
void HAL_DMA_IRQHandler(DMA_HandleTypeDef *hdma)
{
    uint8_t channel = hdma->Channel.Index;

    channel = ((hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) \
                ? (channel * 4) \
                : ((channel - 5) * 4)); // interrupt flag offset

    if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
        /* Half Transfer Complete Interrupt management. */
        if ((LL_DMA_IsActiveFlag(hdma->Instance, (LL_DMA_IF_HTIF0 << channel)) != 0U) && \
            (LL_DMA_IsEnabledIT(hdma->Instance, (LL_DMA_IE_HTIE0 << channel)) != 0U)) {
            /* Disable the half transfer interrupt if the DMA mode is not CIRCULAR. */
            if (LL_DMA_GetMode(hdma->Instance, hdma->Channel.Index) != DMA_CIRCULAR) {
                /* Disable the half transfer interrupt. */
                LL_DMA_DisableIT(hdma->Instance, (LL_DMA_IE_HTIE0 << channel));
            }

            /* Clear the half transfer complete flag. */
            LL_DMA_ClearFlag(hdma->Instance, (LL_DMA_IF_HTIF0 << channel));

            /* DMA peripheral state is not updated in Half Transfer, */
            /* but in Transfer Complete case. */

            if (hdma->XferHalfCpltCallback != NULL) {
                /* Half transfer callback. */
                hdma->XferHalfCpltCallback(hdma);
            }

        } else if ((LL_DMA_IsActiveFlag(hdma->Instance, (LL_DMA_IF_TCIF0 << channel)) != 0U) && \
                   (LL_DMA_IsEnabledIT(hdma->Instance, (LL_DMA_IE_TCIE0 << channel)) != 0U)) {
            /* Transfer Complete Interrupt management. */
            if (LL_DMA_GetMode(hdma->Instance, hdma->Channel.Index) != DMA_CIRCULAR) {
                /* Disable the transfer complete and error interrupt. */
                LL_DMA_DisableIT(hdma->Instance, ((LL_DMA_IE_TCIE0 | LL_DMA_IE_TEIE0) << channel));

                /* Change the DMA state. */
                hdma->State = HAL_DMA_STATE_READY;
            }

            /* Clear the transfer complete flag. */
            LL_DMA_ClearFlag(hdma->Instance, (LL_DMA_IF_TCIF0 << channel));

            /* Process Unlocked */
            __HAL_UNLOCK(hdma);

            if(hdma->XferCpltCallback != NULL) {
                /* Transfer complete callback */
                hdma->XferCpltCallback(hdma);
            }

        } else if ((LL_DMA_IsActiveFlag(hdma->Instance, (LL_DMA_IF_TEIF0 << channel)) != 0U) && \
                   (LL_DMA_IsEnabledIT(hdma->Instance, (LL_DMA_IE_TEIE0 << channel)) != 0U)) {
            /* Transfer Error Interrupt management. */
            /* When a DMA transfer error occurs, a hardware clear of its EN bits is performed. */
            /* Disable all DMA ITs. */
            LL_DMA_DisableIT(hdma->Instance, ((LL_DMA_IE_HTIE0 | LL_DMA_IE_TCIE0 | LL_DMA_IE_TEIE0) << channel));

            /* Clear all flags. */
            LL_DMA_ClearFlag(hdma->Instance, ((LL_DMA_IF_TCIF0 | LL_DMA_IF_TCIF0 | LL_DMA_IF_TEIF0) << channel));

            /* Update error code. */
            hdma->ErrorCode = HAL_DMA_ERROR_TE;

            /* Change the DMA state. */
            hdma->State = HAL_DMA_STATE_READY;

            /* Process Unlocked. */
            __HAL_UNLOCK(hdma);

            if (hdma->XferErrorCallback != NULL) {
                /* Transfer error callback. */
                hdma->XferErrorCallback(hdma);
            }
        }

    } else {
        /* M2M Half Transfer Complete Interrupt management. */
        if ((LL_DMA_IsActiveM2MFlag(hdma->Instance, (LL_DMA_MIF_HTIF0 << channel)) != 0U) && \
            (LL_DMA_IsEnabledM2MIT(hdma->Instance, (LL_DMA_MIE_HTIE0 << channel)) != 0U)) {
            /* Disable the half transfer interrupt if the DMA mode is not CIRCULAR. */
            if (LL_DMA_GetM2MMode(hdma->Instance, hdma->Channel.Index) != DMA_CIRCULAR) {
                /* Disable the half transfer interrupt. */
                LL_DMA_DisableM2MIT(hdma->Instance, (LL_DMA_MIE_HTIE0 << channel));
            }

            /* Clear the half transfer complete flag. */
            LL_DMA_ClearFlag(hdma->Instance, (LL_DMA_MIF_HTIF0 << channel));

            /* DMA peripheral state is not updated in Half Transfer. */
            /* but in Transfer Complete case */

            if (hdma->XferHalfCpltCallback != NULL) {
                /* Half transfer callback. */
                hdma->XferHalfCpltCallback(hdma);
            }

        } else if ((LL_DMA_IsActiveM2MFlag(hdma->Instance, (LL_DMA_MIF_TCIF0 << channel)) != 0U) && \
                   (LL_DMA_IsEnabledM2MIT(hdma->Instance, (LL_DMA_MIE_TCIE0 << channel)) != 0U)) {
            /* M2M Transfer Complete Interrupt management. */
            if (LL_DMA_GetMode(hdma->Instance, hdma->Channel.Index) != DMA_CIRCULAR) {
                /* Disable the transfer complete and error interrupt. */
                LL_DMA_DisableIT(hdma->Instance, ((LL_DMA_MIE_TCIE0 | LL_DMA_MIE_TEIE0) << channel));

                /* Change the DMA state. */
                hdma->State = HAL_DMA_STATE_READY;
            }

            /* Clear the transfer complete flag. */
            LL_DMA_ClearFlag(hdma->Instance, (LL_DMA_MIF_TCIF0 << channel));

            /* Process Unlocked */
            __HAL_UNLOCK(hdma);

            if(hdma->XferCpltCallback != NULL) {
                /* Transfer complete callback */
                hdma->XferCpltCallback(hdma);
            }

        } else if ((LL_DMA_IsActiveM2MFlag(hdma->Instance, (LL_DMA_MIF_TEIF0 << channel)) != 0U) && \
                   (LL_DMA_IsEnabledM2MIT(hdma->Instance, (LL_DMA_MIE_TEIE0 << channel)) != 0U)) {
            /* M2M Transfer Error Interrupt management. */
            /* When a DMA transfer error occurs, a hardware clear of its EN bits is performed. */
            /* Disable all DMA ITs. */
            LL_DMA_DisableIT(hdma->Instance, ((LL_DMA_MIE_HTIE0 | LL_DMA_MIE_TCIE0 | LL_DMA_MIE_TEIE0) << channel));

            /* Clear all flags. */
            LL_DMA_ClearFlag(hdma->Instance, ((LL_DMA_MIF_HTIF0 | LL_DMA_MIF_TCIF0 | LL_DMA_MIF_TEIF0) << channel));

            /* Update error code. */
            hdma->ErrorCode = HAL_DMA_ERROR_TE;

            /* Change the DMA state. */
            hdma->State = HAL_DMA_STATE_READY;

            /* Process Unlocked. */
            __HAL_UNLOCK(hdma);

            if (hdma->XferErrorCallback != NULL) {
                /* Transfer error callback. */
                hdma->XferErrorCallback(hdma);
            }
        }
    }

    return;
}

/**
 * @brief  Register callbacks
 * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
 *                     the configuration information for the specified DMA Channel.
 * @param  CallbackID: User Callback identifier
 *                     a HAL_DMA_CallbackIDTypeDef ENUM as parameter.
 * @param  pCallback:  pointer to private callback function which has pointer to
 *                     a DMA_HandleTypeDef structure as parameter.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_DMA_RegisterCallback(DMA_HandleTypeDef *hdma, HAL_DMA_CallbackIDTypeDef CallbackID, void (* pCallback)(DMA_HandleTypeDef *_hdma))
{
    HAL_StatusTypeDef status = HAL_OK;

    /* Process locked */
    __HAL_LOCK(hdma);

    if(HAL_DMA_STATE_READY == hdma->State) {
        switch (CallbackID) {
            case HAL_DMA_XFER_CPLT_CB_ID:
                hdma->XferCpltCallback      = pCallback;
                break;

            case HAL_DMA_XFER_HALFCPLT_CB_ID:
                hdma->XferHalfCpltCallback  = pCallback;
                break;

            case HAL_DMA_XFER_ERROR_CB_ID:
                hdma->XferErrorCallback     = pCallback;
                break;

            case HAL_DMA_XFER_ABORT_CB_ID:
                hdma->XferAbortCallback     = pCallback;
                break;

            default:
                status = HAL_ERROR;
                break;
        }
    } else {
        status = HAL_ERROR;
    }

    /* Release Lock */
    __HAL_UNLOCK(hdma);

    return status;
}

/**
 * @brief  UnRegister callbacks
 * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
 *                     the configuration information for the specified DMA Channel.
 * @param  CallbackID: User Callback identifier
 *                     a HAL_DMA_CallbackIDTypeDef ENUM as parameter.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_DMA_UnRegisterCallback(DMA_HandleTypeDef *hdma, HAL_DMA_CallbackIDTypeDef CallbackID)
{
    HAL_StatusTypeDef status = HAL_OK;

    /* Process locked */
    __HAL_LOCK(hdma);

    if (HAL_DMA_STATE_READY == hdma->State) {
        switch (CallbackID) {
            case HAL_DMA_XFER_CPLT_CB_ID:
                hdma->XferCpltCallback      = NULL;
                break;

            case HAL_DMA_XFER_HALFCPLT_CB_ID:
                hdma->XferHalfCpltCallback  = NULL;
                break;

            case HAL_DMA_XFER_ERROR_CB_ID:
                hdma->XferErrorCallback     = NULL;
                break;

            case HAL_DMA_XFER_ABORT_CB_ID:
                hdma->XferAbortCallback     = NULL;
                break;

            case HAL_DMA_XFER_ALL_CB_ID:
                hdma->XferCpltCallback      = NULL;
                hdma->XferHalfCpltCallback  = NULL;
                hdma->XferErrorCallback     = NULL;
                hdma->XferAbortCallback     = NULL;
                break;

            default:
                status = HAL_ERROR;
                break;
        }
    } else {
        status = HAL_ERROR;
    }

    /* Release Lock */
    __HAL_UNLOCK(hdma);

    return status;
}

/**
 * @}
 */

/** @defgroup DMA_Exported_Functions_Group3 State functions
 * @brief     State functions
 *
@verbatim
================================================================================
                    ##### State functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) HAL_DMA_GetState                    : Check the DMA state
        (+) HAL_DMA_GetError                    : Get error code

@endverbatim
 * @{
 */

/**
 * @brief  Return the DMA handle state.
 * @param  hdma: pointer to a DMA_HandleTypeDef structure that contains
 *               the configuration information for the specified DMA Channel.
 * @retval Return HAL DMA status.
 */
HAL_DMA_StateTypeDef HAL_DMA_GetState(DMA_HandleTypeDef *hdma)
{
    /* Return DMA handle state */
    return hdma->State;
}

/**
 * @brief  Return the DMA error code.
 * @param  hdma: pointer to a DMA_HandleTypeDef structure that contains
 *               the configuration information for the specified DMA Channel.
 * @retval Return DMA Error Code.
 */
uint32_t HAL_DMA_GetError(DMA_HandleTypeDef *hdma)
{
    return hdma->ErrorCode;
}

/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup DMA_Private_Functions
 * @{
 */

/**
 * @brief  Sets the DMA Transfer parameter.
 * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
 *                     the configuration information for the specified DMA Channel.
 * @param  SrcAddress: The source memory Buffer address
 * @param  DstAddress: The destination memory Buffer address
 * @param  DataLength: The length of data to be transferred from source to destination
 * @retval None
 */
static void DMA_SetConfig(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
    uint32_t addr;
    if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
        /* Get the Peripheral address. */
        switch (hdma->Channel.Type) {
            case (HAL_DMA_CHANNEL_QSPI0_RX):
                addr = DMA_QSPI0_RX_PERIPH_BASE(hdma->Instance);
                break;

            case HAL_DMA_CHANNEL_USART0_RX:
                addr = DMA_USART0_RX_PERIPH_BASE(hdma->Instance);
                break;

            case HAL_DMA_CHANNEL_QSPI0_TX:
                addr = DMA_QSPI0_TX_PERIPH_BASE(hdma->Instance);
                break;

            case HAL_DMA_CHANNEL_USART0_TX:
                addr = DMA_USART0_TX_PERIPH_BASE(hdma->Instance);
                break;

            case HAL_DMA_CHANNEL_QSPI1_RX:
                addr = DMA_QSPI1_RX_PERIPH_BASE(hdma->Instance);
                break;

            case HAL_DMA_CHANNEL_USART1_RX:
                addr = DMA_USART1_RX_PERIPH_BASE(hdma->Instance);
                break;

            case HAL_DMA_CHANNEL_QSPI1_TX:
                addr = DMA_QSPI1_TX_PERIPH_BASE(hdma->Instance);
                break;

            case HAL_DMA_CHANNEL_USART1_TX:
                addr = DMA_USART1_TX_PERIPH_BASE(hdma->Instance);
                break;

            default:
                break;
        }

        /* Configure DMA Channel source address and destination address. */
        if (hdma->Init.Direction == DMA_PERIPH_TO_MEMORY) {
            LL_DMA_SetMemoryAddress(hdma->Instance, hdma->Channel.Index, DstAddress);
        } else {
            LL_DMA_SetMemoryAddress(hdma->Instance, hdma->Channel.Index, SrcAddress);
        }
        LL_DMA_SetPeriphAddress(hdma->Instance, hdma->Channel.Index, addr);

        /* Configure DMA Channel data length. */
        LL_DMA_SetDataLength(hdma->Instance, hdma->Channel.Index, DataLength);
    } else {
        /* Configure DMA M2M Channel source address and destination address. */
        if (hdma->Channel.Type == HAL_DMA_CHANNEL_CRC) {
            LL_DMA_SetM2MSrcAddress(hdma->Instance, hdma->Channel.Index, SrcAddress);
            LL_DMA_SetM2MDstAddress(hdma->Instance, hdma->Channel.Index, DMA_CRC0_PERIPH_BASE(hdma->Instance));
        } else {
            LL_DMA_SetM2MSrcAddress(hdma->Instance, hdma->Channel.Index, SrcAddress);
            LL_DMA_SetM2MDstAddress(hdma->Instance, hdma->Channel.Index, DstAddress);
        }

        /* Configure DMA M2M Channel data length. */
        LL_DMA_SetM2MDataLength(hdma->Instance, hdma->Channel.Index, DataLength);
    }
}

/**
 * @brief  Enable the specified DMA channel.
 * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
 *                     the configuration information for the specified DMA Channel.
 * @retval None
 */
static void DMA_EnableChannel(DMA_HandleTypeDef *hdma)
{
    if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
        LL_DMA_EnableChannel(hdma->Instance, hdma->Channel.Index);
    } else {
        LL_DMA_EnableM2MChannel(hdma->Instance, hdma->Channel.Index);
    }
}

/**
 * @brief  Disable the specified DMA channel.
 * @param  hdma:       pointer to a DMA_HandleTypeDef structure that contains
 *                     the configuration information for the specified DMA Channel.
 * @retval None
 */
static void DMA_DisableChannel(DMA_HandleTypeDef *hdma)
{
    if (hdma->Init.Direction != DMA_MEMORY_TO_MEMORY) {
        LL_DMA_DisableChannel(hdma->Instance, hdma->Channel.Index);
    } else {
        LL_DMA_DisableM2MChannel(hdma->Instance, hdma->Channel.Index);
    }
}
/**
 * @}
 */

#endif /* HAL_DMA_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
