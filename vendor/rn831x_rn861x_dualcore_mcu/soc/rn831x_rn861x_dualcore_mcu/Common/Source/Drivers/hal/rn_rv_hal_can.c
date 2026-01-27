/**
 ******************************************************************************
 * @file   rn_rv_hal_can.c
 * @author Renergy Software Team
 * @brief  CAN HAL module driver.
 *         This file provides firmware functions to manage the following
 *         functionalities of the Controller Area Network (CAN) peripheral:
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
#include <string.h>

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @defgroup CAN CAN
 * @brief     CAN driver modules
 * @{
 */

#ifdef HAL_CAN_MODULE_ENABLED

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/** @defgroup CAN_Private_Constants CAN Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/** @defgroup CAN_Private_Functions CAN Private Functions
 * @{
 */
static void can_reset(CAN_TypeDef *can);
static void can_baudRate_set(CAN_TypeDef *can, CAN_TimingConfigTypeDef timingConfig);
/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup CAN_Exported_Functions CAN Exported Functions
 * @{
 */

/** @defgroup CAN_Exported_Functions_Group1 Initialization and de-initialization functions
 * @brief     Initialization and Configuration functions
 *
@verbatim
================================================================================
            ##### Initialization and de-initialization functions #####
================================================================================
    [..] This section provides functions allowing to:
        (+) Initialize and configure the CAN.
        (+) De-initialize the CAN.

@endverbatim
 * @{
 */

/**
 * @brief  Initializes a CAN instance according to the specified parameters in CAN_InitStruct.
 * @note   This function initializes the CAN module with user-defined settings.
 *         This example shows how to set up the HAL_CAN_InitTypeDef parameters
 *         and how to call the HAL_CAN_Init function by passing in these parameters.
 * @code
 *         HAL_CAN_InitTypeDef flexcanConfig;
 *         flexcanConfig.clkSrc               = CAN_ClkSrcPllOsc_async;
 *         flexcanConfig.maxMbNum             = 16;
 *         flexcanConfig.enableLoopBack       = DISABLE;
 *         flexcanConfig.enableSelfWakeup     = DISABLE;
 *         flexcanConfig.enableIndividMask    = DISABLE;
 *         flexcanConfig.disableSelfReception = DISABLE;
 *         flexcanConfig.enableListenOnlyMode = DISABLE;
 *         flexcanConfig.enableDoze           = DISABLE;
 *         flexcanConfig.timingConfig         = timingConfig;
 *         HAL_CAN_Init(CAN0, &flexcanConfig);
 * @endcode
 * @param  can CAN instance
 * @param  pConfig Pointer to the configuration structure.
 * @retval None
 */
void HAL_CAN_Init(CAN_TypeDef *can, const CAN_ConfigTypeDef *pConfig)
{
    uint32_t mcrTemp, ctrl1Temp;

    /*!< Enable CAN Module for configuration. */
    HAL_CAN_Start(can);

    /*!< Reset to known status. */
    can_reset(can);

    /*!< Save current CTRL1 value and enable to enter Freeze mode(enabled by default). */
    ctrl1Temp = can->CTRL1;

    /*!< Save current MCR value and enable to enter Freeze mode(enabled by default). */
    mcrTemp = can->MCR;

    /*!< Enable Loop Back Mode? */
    ctrl1Temp = ((pConfig->enableLoopBack == (uint8_t)ENABLE)
                ? (ctrl1Temp | CAN_CTRL1_LPB)
                : (ctrl1Temp & ~CAN_CTRL1_LPB));

    /*!< Enable Timer Sync? */
    ctrl1Temp = ((pConfig->enableTimerSync == (uint8_t)ENABLE)
                ? (ctrl1Temp | CAN_CTRL1_TSYN)
                : (ctrl1Temp & ~CAN_CTRL1_TSYN));

    /*!< Enable Listen Only Mode? */
    ctrl1Temp = ((pConfig->enableListenOnlyMode == (uint8_t)ENABLE)
                ? (ctrl1Temp | CAN_CTRL1_LOM)
                : (ctrl1Temp & ~CAN_CTRL1_LOM));

    /*!< Set the maximum number of Message Buffers */
    mcrTemp = ((mcrTemp & ~CAN_MCR_MAXMB) | (((uint32_t)pConfig->maxMbNum - 1U) << CAN_MCR_MAXMB_POS));

    /*!< Enable Self Wake Up Mode and configure the wake up source. */
    mcrTemp = ((pConfig->enableSelfWakeup == (uint8_t)ENABLE)
                ? (mcrTemp | CAN_MCR_SLFWAK)
                : (mcrTemp & ~CAN_MCR_SLFWAK));
    mcrTemp = ((pConfig->wakeupSrc == CAN_WAKEUP_SOURCE_FILTERED)
                ? (mcrTemp | CAN_MCR_WAKSRC)
                : (mcrTemp & ~CAN_MCR_WAKSRC));

    /*!< Enable Individual Rx Masking? */
    mcrTemp = ((pConfig->enableIndividMask == (uint8_t)ENABLE)
                ? (mcrTemp | CAN_MCR_IRMQ)
                : (mcrTemp & ~CAN_MCR_IRMQ));

    /*!< Disable Self Reception? */
    mcrTemp = ((pConfig->disableSelfReception == (uint8_t)ENABLE)
                ? (mcrTemp | CAN_MCR_SRXDIS)
                : (mcrTemp & ~CAN_MCR_SRXDIS));

    /*!< Write back CTRL1 Configuration to register. */
    can->CTRL1 = ctrl1Temp;

    /*!< Write back MCR Configuration to register. */
    can->MCR = mcrTemp;

    /*!< Baud Rate Configuration. */
    can_baudRate_set(can, pConfig->timingConfig);
}

/**
 * @brief  De-initialize the CAN registers to their default reset values.
 * @param  can CAN instance
 * @retval None.
 */
void HAL_CAN_DeInit(CAN_TypeDef *can)
{
    *((__IOM uint32_t *)0x20080000U) = 0x82U;
    *((__IOM uint32_t *)0x2008013CU) &= ~((uint32_t)1U << 19);
    *((__IOM uint32_t *)0x20080000U) = 0x00U;

    /*!< Reset all Register Contents. */
    can_reset(can);

    /*!< Disable CAN module. */
    HAL_CAN_Stop(can);
}

/**
 * @}
 */

/** @defgroup CAN_Exported_Functions_Group2 IO Operation functions
 * @brief     IO Operation functions
 *
@verbatim
================================================================================
                    ##### IO Operation functions #####
================================================================================
    [..] This section provides functions allowing to:
        (+) HAL_CAN_Start                         : Start the CAN module
        (+) HAL_CAN_Stop                          : Stop the CAN module
        (+) HAL_CAN_EnterFreezeMode               : Enter the CAN freeze mode
        (+) HAL_CAN_ExitFreezeMode                : Exit the CAN freeze mode
        (+) HAL_CAN_AddTxMessage                  : Add the Tx message
        (+) HAL_CAN_GetRxMessage                  : Get the Rx message
        (+) HAL_CAN_GetRxFifoMessage              : Get the Rx FIFO message
        (+) HAL_CAN_TransferSendBlocking          : Start transfer Tx blocking
        (+) HAL_CAN_TransferReceiveBlocking       : Start transfer Rx blocking
        (+) HAL_CAN_TransferReceiveFifoBlocking   : Start transfer Rx FIFO blocking
        (+) HAL_CAN_EnableRxFifoDMA               : Enable the Rx FIFO DMA
        (+) HAL_CAN_DisableRxFifoDMA              : Disable the Rx FIFO DMA

@endverbatim
 * @{
 */

/**
 * @brief  Enable the CAN module.
 * @param  can CAN instance
 * @param  State new State of the ADC peripheral.
 * @retval None
 */
void HAL_CAN_Start(CAN_TypeDef *can)
{
    can->MCR &= ~CAN_MCR_MDIS;

    /*!< Wait CAN exit from low-power mode. */
    while (0U != (can->MCR & CAN_MCR_LPMACK)) {
    }
}

/**
 * @brief  Stop the CAN module.
 * @param  can CAN instance
 * @param  State new State of the ADC peripheral.
 * @retval None
 */
void HAL_CAN_Stop(CAN_TypeDef *can)
{
    can->MCR |= CAN_MCR_MDIS;

    /*!< Wait CAN enter low-power mode. */
    while (0U == (can->MCR & CAN_MCR_LPMACK)) {
    }
}

/**
 * @brief  Enter the CAN freeze mode.
 * @param  can CAN instance
 * @retval None
 */
void HAL_CAN_EnterFreezeMode(CAN_TypeDef *can)
{
    /*!< Set Freeze, Halt bits. */
    can->MCR |= CAN_MCR_FRZ;
    can->MCR |= CAN_MCR_HALT;

    while (0U == (can->MCR & CAN_MCR_FRZACK)) {
    }
}

/**
 * @brief  Exit the CAN freeze mode.
 * @param  can CAN instance
 * @retval None
 */
void HAL_CAN_ExitFreezeMode(CAN_TypeDef *can)
{
    /*!< Clear Freeze, Halt bits. */
    can->MCR &= ~CAN_MCR_HALT;
    can->MCR &= ~CAN_MCR_FRZ;

    /*!< Wait until the CAN Module exit freeze mode. */
    while (0U != (can->MCR & CAN_MCR_FRZACK)) {
    }
}

/**
 * @brief  Writes a CAN Message to the Transmit MailBox.
 * @note   This function writes a CAN Message to the specified Transmit MailBox
 *         and changes the MailBox state to start CAN Message transmit. After
 *         that the function returns immediately.
 * @param  can CAN instance
 * @param  MbIdx The CAN MailBox index.
 * @param  pTxFrame: Pointer to CAN message frame to be sent.
 * @retval CAN_STATUS_SUCCESS - Write Tx MailBox Successfully.
 *         CAN_STATUS_FAIL    - Tx MailBox is currently in use.
 */
uint32_t HAL_CAN_AddTxMessage(CAN_TypeDef *can, uint8_t MbIdx, const CAN_FrameTypeDef *pTxFrame)
{
    uint32_t cs_temp = 0U, status, tmp_epreg;

    /*!< Check if Message Buffer is available. */
    if ((((uint32_t)CAN_TXMBDATAORREMOTE) << CAN_CS_CODE_POS) != (can->MB[MbIdx].CS & CAN_CS_CODE)) {
        /*!< Inactive Tx Message Buffer. */
        tmp_epreg = can->MB[MbIdx].CS & ~CAN_CS_CODE;
        can->MB[MbIdx].CS = (tmp_epreg | (((uint32_t)CAN_TXMBINACTIVE) << CAN_CS_CODE_POS));

        /*!< Fill Message ID field. */
        can->MB[MbIdx].ID = pTxFrame->id;

        /*!< Fill Message Format field. */
        if ((uint32_t)CAN_FRAME_FORMAT_EXTEND == pTxFrame->format) {
            cs_temp |= (CAN_CS_SRR | CAN_CS_IDE);
        }

        /*!< Fill Message Type field. */
        if ((uint32_t)CAN_FRAME_TYPE_REMOTE == pTxFrame->type) {
            cs_temp |= CAN_CS_RTR;
        }

        cs_temp |= ((((uint32_t)CAN_TXMBDATAORREMOTE) << CAN_CS_CODE_POS) |
                    ((uint32_t)(pTxFrame->length) << CAN_CS_DLC_POS));

        /*!< Load Message Payload. */
        can->MB[MbIdx].WORD0 = pTxFrame->dataWord0;
        can->MB[MbIdx].WORD1 = pTxFrame->dataWord1;

        /*!< Activate Tx Message Buffer. */
        can->MB[MbIdx].CS = cs_temp;

        status = CAN_STATUS_SUCCESS;
    } else {
        /*!< Tx Message Buffer is activated, return immediately. */
        status = CAN_STATUS_FAIL;
    }

    return status;
}

/**
 * @brief  Reads a CAN Message from Receive MailBox.
 * @note   This function reads a CAN message from a specified Receive MailBox.
 *         The function fills a receive CAN message frame structure with just
 *         received data and activates the MailBox again.
 *         The function returns immediately.
 * @param  can CAN instance
 * @param  MbIdx: The CAN Message Buffer index.
 * @param  pRxFrame Pointer to CAN message frame structure for reception.
 * @retval CAN_STATUS_SUCCESS    - Rx MailBox is full and has been read successfully.
 *         CAN_STATUS_RXOVERFLOW - Rx MailBox is already overflowed and has been read successfully.
 *         CAN_STATUS_FAIL       - Rx MailBox is empty.
 */
uint32_t HAL_CAN_GetRxMessage(CAN_TypeDef *can, uint8_t MbIdx, CAN_FrameTypeDef *pRxFrame)
{
    uint32_t cs_temp, rx_code, status;

    /*!< Read CS field of Rx Message Buffer to lock Message Buffer. */
    cs_temp = can->MB[MbIdx].CS;

    /*!< Get Rx Message Buffer Code field. */
    rx_code = ((cs_temp & CAN_CS_CODE) >> CAN_CS_CODE_POS);

    /*!< Check to see if Rx Message Buffer is full. */
    if (((uint32_t)CAN_RXMBFULL == rx_code) ||
        ((uint32_t)CAN_RXMBOVERRUN == rx_code)) {
        /*!< Store Message ID. */
        pRxFrame->id = (can->MB[MbIdx].ID & (CAN_ID_EXT | CAN_ID_STD));

        /*!< Get the message ID and format. */
        pRxFrame->format = (((cs_temp & CAN_CS_IDE) != 0U)
                            ? (uint8_t)CAN_FRAME_FORMAT_EXTEND
                            : (uint8_t)CAN_FRAME_FORMAT_STANDARD);

        /*!< Get the message type. */
        pRxFrame->type = (((cs_temp & CAN_CS_RTR) != 0U)
                          ? (uint8_t)CAN_FRAME_TYPE_REMOTE
                          : (uint8_t)CAN_FRAME_TYPE_DATA);

        /*!< Get the message length. */
        pRxFrame->length = (uint8_t)((cs_temp & CAN_CS_DLC) >> CAN_CS_DLC_POS);

        /*!< Get the time stamp. */
        pRxFrame->timestamp = (uint16_t)((cs_temp & CAN_CS_TIME_STAMP) >> CAN_CS_TIME_STAMP_POS);

        /*!< Store Message Payload. */
        pRxFrame->dataWord0 = can->MB[MbIdx].WORD0;
        pRxFrame->dataWord1 = can->MB[MbIdx].WORD1;

        /*!< Read free-running timer to unlock Rx Message Buffer. */
        (void)can->TIMER;

        if ((uint32_t)CAN_RXMBFULL == rx_code) {
            status = CAN_STATUS_SUCCESS;
        } else {
            status = CAN_STATUS_RXOVERFLOW;
        }
    } else {
        /*!< Read free-running timer to unlock Rx Message Buffer. */
        (void)can->TIMER;

        status = CAN_STATUS_FAIL;
    }

    return status;
}

/**
 * @brief  Reads a CAN Message from Rx FIFO.
 * @note   This function reads a CAN message from the CAN build-in Rx FIFO.
 * @param  can CAN instance
 * @param  pRxFrame Pointer to CAN message frame structure for reception.
 * @retval CAN_STATUS_SUCCESS - Read Message from Rx FIFO successfully.
 *         CAN_STATUS_FAIL    - Rx FIFO is not enabled.
 */
uint32_t HAL_CAN_GetRxFifoMessage(CAN_TypeDef *can, CAN_FrameTypeDef *pRxFrame)
{
    uint32_t cs_temp, status;

    /*!< Check if Rx FIFO is Enabled. */
    if (0U != (can->MCR & CAN_MCR_RFEN)) {
        /*!< Read CS field of Rx Message Buffer to lock Message Buffer. */
        cs_temp = can->MB[0].CS;

        /*!< Read data from Rx FIFO output port. */
        /*!< Store Message ID. */
        pRxFrame->id = (can->MB[0].ID & (CAN_ID_EXT | CAN_ID_STD));

        /*!< Get the message ID and format. */
        pRxFrame->format = (((cs_temp & CAN_CS_IDE) != 0U)
                            ? (uint8_t)CAN_FRAME_FORMAT_EXTEND
                            : (uint8_t)CAN_FRAME_FORMAT_STANDARD);

        /*!< Get the message type. */
        pRxFrame->type = (((cs_temp & CAN_CS_RTR) != 0U)
                          ? (uint8_t)CAN_FRAME_TYPE_REMOTE
                          : (uint8_t)CAN_FRAME_TYPE_DATA);

        /*!< Get the message length. */
        pRxFrame->length = (uint8_t)((cs_temp & CAN_CS_DLC) >> CAN_CS_DLC_POS);

        /*!< Get the time stamp. */
        pRxFrame->timestamp = (uint16_t)((cs_temp & CAN_CS_TIME_STAMP) >> CAN_CS_TIME_STAMP_POS);

        /*!< Store Message Payload. */
        pRxFrame->dataWord0 = can->MB[0].WORD0;
        pRxFrame->dataWord1 = can->MB[0].WORD1;

        /*!< Store ID Filter Hit Index. */
        pRxFrame->idhit = (uint16_t)(can->RXFIR & CAN_RXFIR_IDHIT);

        /*!< Read free-running timer to unlock Rx Message Buffer. */
        (void)can->TIMER;

        status = CAN_STATUS_SUCCESS;
    } else {
        status = CAN_STATUS_FAIL;
    }

    return status;
}

/**
 * @brief  Performs a polling send transaction on the CAN bus.
 * @note   Note that a transfer handle does not need to be created before calling this API.
 * @param  can CAN instance
 * @param  MbIdx The CAN Message Buffer index.
 * @param  pTxFrame Pointer to CAN message frame to be sent.
 * @retval CAN_STATUS_SUCCESS - Write Tx Message Buffer Successfully.
 *         CAN_STATUS_FAIL    - Tx Message Buffer is currently in use.
 */
uint32_t HAL_CAN_TransferSendBlocking(CAN_TypeDef *can, uint8_t MbIdx, CAN_FrameTypeDef *pTxFrame)
{
    uint32_t status = CAN_STATUS_SUCCESS;

    /*!< Write Tx Message Buffer to initiate a data sending. */
    if (CAN_STATUS_SUCCESS ==
        HAL_CAN_AddTxMessage(can, MbIdx, (const CAN_FrameTypeDef *)(uint32_t)pTxFrame)) {
        /*!< Wait until CAN Message send out. */
        uint32_t u32flag = 1U;

        while (RESET == HAL_CAN_GetMbState(can, (u32flag << MbIdx))) {
            if (SET == HAL_CAN_GetState(can, CAN_FLAG_TXERRORWARNING)) {
                status = CAN_STATUS_FAIL;
                break;
            }
        }

        /*!< Clean Tx Message Buffer Flag. */
        HAL_CAN_ClearMbFlag(can, (u32flag << MbIdx));

        /*!< After TX MB tranfered success, update the Timestamp from MB[MbIdx].CS register */
        pTxFrame->timestamp =
            (uint16_t)((can->MB[MbIdx].CS & CAN_CS_TIME_STAMP) >> CAN_CS_TIME_STAMP_POS);
    } else {
        status = CAN_STATUS_FAIL;
    }

    return status;
}

/**
 * @brief  Performs a polling receive transaction on the CAN bus.
 * @note   Note that a transfer handle does not need to be created before calling this API.
 * @param  can CAN instance
 * @param  MbIdx The CAN Message Buffer index.
 * @param  pRxFrame Pointer to CAN message frame structure for reception.
 * @retval CAN_STATUS_SUCCESS    - Rx Message Buffer is full and has been read successfully.
 *         CAN_STATUS_RXOVERFLOW - Rx Message Buffer is already overflowed and has been read successfully.
 *         CAN_STATUS_FAIL       - Rx Message Buffer is empty.
 */
uint32_t HAL_CAN_TransferReceiveBlocking(CAN_TypeDef *can, uint8_t MbIdx, CAN_FrameTypeDef *pRxFrame)
{
    /*!< Wait until Rx Message Buffer non-empty. */
    uint32_t u32flag = 1U;
    while (RESET == HAL_CAN_GetMbState(can, (u32flag << MbIdx))) {
    }

    /*!< Clean Rx Message Buffer Flag. */
    HAL_CAN_ClearMbFlag(can, (u32flag << MbIdx));

    /*!< Read Received CAN Message. */
    return HAL_CAN_GetRxMessage(can, MbIdx, pRxFrame);
}

/**
 * @brief  Performs a polling receive transaction from Rx FIFO on the CAN bus.
 * @note   Note that a transfer handle does not need to be created before calling this API.
 * @param  can CAN instance
 * @param  pRxFrame Pointer to CAN message frame structure for reception.
 * @retval CAN_STATUS_SUCCESS - Read Message from Rx FIFO successfully.
 *         CAN_STATUS_FAIL    - Rx FIFO is not enabled.
 */
uint32_t HAL_CAN_TransferReceiveFifoBlocking(CAN_TypeDef *can, CAN_FrameTypeDef *pRxFrame)
{
    uint32_t rxFifoStatus;

    /*!< Wait until Rx FIFO non-empty. */
    while (RESET == HAL_CAN_GetMbState(can, (uint32_t)CAN_RXFIFOFRAMEAVLFLAG)) {
    }
    rxFifoStatus = HAL_CAN_GetRxFifoMessage(can, pRxFrame);

    /*!< Clean Rx Fifo available flag. */
    HAL_CAN_ClearMbFlag(can, (uint32_t)CAN_RXFIFOFRAMEAVLFLAG);

    return rxFifoStatus;
}

/**
 * @brief  Enable the CAN Rx FIFO DMA request.
 * @note   This function enables the DMA feature of CAN build-in Rx FIFO.
 * @param  can CAN instance
 * @param  State This parameter can be: ENABLE or DISABLE.
 * @retval None
 */
void HAL_CAN_EnableRxFifoDMA(CAN_TypeDef *can)
{
    /*!< Enter Freeze Mode. */
    HAL_CAN_EnterFreezeMode(can);

    /*!< Enable CAN DMA. */
    can->MCR |= CAN_MCR_DMA;

    /*!< Exit Freeze Mode. */
    HAL_CAN_ExitFreezeMode(can);
}

/**
 * @brief  Disable the CAN Rx FIFO DMA request.
 * @note   This function disables the DMA feature of CAN build-in Rx FIFO.
 * @param  can CAN instance
 * @param  State This parameter can be: ENABLE or DISABLE.
 * @retval None
 */
void HAL_CAN_DisableRxFifoDMA(CAN_TypeDef *can)
{
    /*!< Enter Freeze Mode. */
    HAL_CAN_EnterFreezeMode(can);

    /*!< Disable CAN DMA. */
    can->MCR &= ~CAN_MCR_DMA;

    /*!< Exit Freeze Mode. */
    HAL_CAN_ExitFreezeMode(can);
}

/**
 * @}
 */

/** @defgroup CAN_Exported_Functions_Group3 Interrupt functions
 * @brief     Interrupt functions
 *
@verbatim
================================================================================
            ##### Interrupt functions #####
================================================================================
    [..] This subsection provides functions allowing to:
        (+) HAL_CAN_GetState                     : Get the CAN module status flag
        (+) HAL_CAN_GetDMAState                  : Get the DMA status flag
        (+) HAL_CAN_GetMbState                   : Get the Mb status flag
        (+) HAL_CAN_ClearFlag                    : Clear the CAN module statue flag
        (+) HAL_CAN_ClearDMAFlag                 : Clear the DMA status flag
        (+) HAL_CAN_ClearMbFlag                  : Clear the Mb status flag
        (+) HAL_CAN_ConfigInterrupt              : Set the CAN module interrupt
        (+) HAL_CAN_ConfigDMAInterrupt           : Set the DMA interrupt
        (+) HAL_CAN_ConfigMbInterrupt            : Set the Mb interrupt

@endverbatim
 * @{
 */

/**
 * @brief  Gets the CAN module flags.
 * @param  can CAN instance
 * @param  Flag specifies the flag to check:
 *           @arg CAN_FLAG_SYNCH
 *           @arg CAN_FLAG_TX_WARNINGINT
 *           @arg CAN_FLAG_RX_WARNINGINT
 *           @arg CAN_FLAG_TXERRORWARNING
 *           @arg CAN_FLAG_RXERRORWARNING
 *           @arg CAN_FLAG_IDLE
 *           @arg CAN_FLAG_FAULTCONFINEMENT
 *           @arg CAN_FLAG_TRANSMITTING
 *           @arg CAN_FLAG_RECEIVING
 *           @arg CAN_FLAG_BUSOFFINT
 *           @arg CAN_FLAG_ERRORINT
 *           @arg CAN_FLAG_WAKEUPINT

 *           @arg CAN_ERROR_STUFFING
 *           @arg CAN_ERROR_FORM
 *           @arg CAN_ERROR_CRC
 *           @arg CAN_ERROR_ACK
 *           @arg CAN_ERROR_BIT0
 *           @arg CAN_ERROR_BIT1
 * @retval The New State of adc_flag (SET or RESET).
 */
FlagStatus HAL_CAN_GetState(CAN_TypeDef *can, uint32_t Flag)
{
    return (((can->ESR1 & Flag) != 0U) ? SET : RESET);
}

/**
 * @brief  Gets the CAN Rx DMA interrupt flags.
 * @note   This function gets the interrupt flags of a given DMA IF.
 * @param  can CAN instance
 * @param  DMAFlag The CAN DMA IF Mask.
 * @retval The New State of dma_flag (SET or RESET).
 */
FlagStatus HAL_CAN_GetDMAState(CAN_TypeDef *can, uint32_t DMAFlag)
{
    return (((can->DMA_IF & DMAFlag) != 0U) ? SET : RESET);
}

/**
 * @brief  Gets the CAN Message Buffer interrupt flags.
 * @note   This function gets the interrupt flags of a given Message Buffers.
 * @param  can CAN instance
 * @param  MbFlag The CAN Message Buffer Mask.
 * @retval The New State of mb_flag (SET or RESET).
 */
FlagStatus HAL_CAN_GetMbState(CAN_TypeDef *can, uint32_t MbFlag)
{
    return (((can->IFLAG1 & MbFlag) != 0U) ? SET : RESET);
}

/**
 * @brief  Clear status flags with the provided mask.
 * @note   This function clears the CAN status flags with a provided mask.
 *         An automatically cleared Flag can't be cleared by this function.
 * @param  can CAN instance
 * @param  Flag specifies the flag to check.
 *           @arg CAN_FLAG_SYNCH
 *           @arg CAN_FLAG_TX_WARNINGINT
 *           @arg CAN_FLAG_RX_WARNINGINT
 *           @arg CAN_FLAG_TXERRORWARNING
 *           @arg CAN_FLAG_RXERRORWARNING
 *           @arg CAN_FLAG_IDLE
 *           @arg CAN_FLAG_FAULTCONFINEMENT
 *           @arg CAN_FLAG_TRANSMITTING
 *           @arg CAN_FLAG_RECEIVING
 *           @arg CAN_FLAG_BUSOFFINT
 *           @arg CAN_FLAG_ERRORINT
 *           @arg CAN_FLAG_WAKEUPINT

 *           @arg CAN_ERROR_STUFFING
 *           @arg CAN_ERROR_FORM
 *           @arg CAN_ERROR_CRC
 *           @arg CAN_ERROR_ACK
 *           @arg CAN_ERROR_BIT0
 *           @arg CAN_ERROR_BIT1
 * @retval None
 */
void HAL_CAN_ClearFlag(CAN_TypeDef *can, uint32_t Flag)
{
    can->ESR1 = Flag;
}

/**
 * @brief  Clear the Rx DMA interrupt flags.
 * @note   This function clears the interrupt flags of a given DMA IF.
 * @param  can CAN instance
 * @param  DMAFlag The ORed CAN DMA IF Mask.
 * @retval None
 */
void HAL_CAN_ClearDMAFlag(CAN_TypeDef *can, uint32_t DMAFlag)
{
    can->DMA_IF = DMAFlag;
}

/**
 * @brief  Clear the CAN MailBox interrupt flags.
 * @note   This function clears the interrupt flags of a given MailBoxes.
 * @param  can CAN instance
 * @param  MbFlag The ORed CAN Message Buffer Mask.
 * @retval None
 */
void HAL_CAN_ClearMbFlag(CAN_TypeDef *can, uint32_t MbFlag)
{
    can->IFLAG1 = MbFlag;
}

/**
 * @brief  Enables or disables CAN interrupts.
 * @param  can CAN instance
 * @param  ITs The interrupts to enable.
 * @param  State New State of the specified CAN interrupts.
 */
void HAL_CAN_ConfigInterrupt(CAN_TypeDef *can, uint32_t ITs, FunctionalState State)
{
    if (State == ENABLE) {
        /*!< Solve Wake Up Interrupt. */
        if (CAN_IT_WAKEUP == (ITs & CAN_IT_WAKEUP)) {
            can->MCR |= CAN_MCR_WAKMSK;
        }

        /*!< Solve others. */
        can->CTRL1 |= (ITs & (~CAN_IT_WAKEUP));
    } else {
        /*!< Solve Wake Up Interrupt. */
        if (CAN_IT_WAKEUP == (ITs & CAN_IT_WAKEUP)) {
            can->MCR &= ~CAN_MCR_WAKMSK;
        }

        /*!< Solve others. */
        can->CTRL1 &= ~(ITs & (~CAN_IT_WAKEUP));
    }
}

/**
 * @brief  Enables or disables the CAN Message Buffer interrupts.
 * @param  can CAN instance
 * @param  MbITs the CAN Message Buffer interrupts
 * @param  State new State of the adc peripheral.
 * @retval None
 */
void HAL_CAN_ConfigMbInterrupt(CAN_TypeDef *can, uint32_t MbITs, FunctionalState State)
{
    ((State == ENABLE) ? (can->IMASK1 |= MbITs) : (can->IMASK1 &= ~MbITs));
}

/**
 * @brief  Enables or disables the CAN DMA interrupts.
 * @param  can CAN instance
 * @param  DMAITs the CAN DMA interrupts
 * @param  State new State of the adc peripheral.
 * @retval None
 */
void HAL_CAN_ConfigDMAInterrupt(CAN_TypeDef *can, uint32_t DMAITs, FunctionalState State)
{
    ((State == ENABLE) ? (can->DMA_IE |= DMAITs) : (can->DMA_IE &= ~DMAITs));
}

/**
 * @}
 */

/** @defgroup CAN_Exported_Functions_Group4 Configuration functions
 * @brief     Configuration functions.
 *
@verbatim
================================================================================
                    ##### Configuration functions #####
================================================================================
    [..] This section provides functions allowing to:
        (+) HAL_CAN_GetDefaultConfig                : Get the default configuration
        (+) HAL_CAN_GetRxFifoHeadAddress            : Get the Rx FIFO head address
        (+) HAL_CAN_GetBusErrorCount                : Get the module bus error count
        (+) HAL_CAN_SetTimingConfig                 : Set the timing configuration
        (+) HAL_CAN_SetRxMbGlobalMask               : Set the Rx Mb global mask
        (+) HAL_CAN_SetRxFifoGlobalMask             : Set the Rx FIFO global mask
        (+) HAL_CAN_SetRxIndividualMask             : Set the Rx individual mask
        (+) HAL_CAN_ConfigTxMb                      : Set the Tx Mb configuration
        (+) HAL_CAN_ConfigRxMb                      : Set the Rx Mb configuration
        (+) HAL_CAN_ConfigRxFifo                    : Set the Rx FIFO configuration
        (+) HAL_CAN_ConfigRxDMA                     : Set the Rx DMA configuration

@endverbatim
 * @{
 */

/**
 * @brief  Gets the default configuration structure.
 * @note   This function initializes the CAN configuration structure to default
 *         values.
 * @code
 *         flexcanConfig->clkSrc               = CAN_ClkSrcPllOsc_sync;
 *         flexcanConfig->maxMbNum             = 16;
 *         flexcanConfig->enableLoopBack       = DISABLE;
 *         flexcanConfig->enableSelfWakeup     = DISABLE;
 *         flexcanConfig->enableIndividMask    = ENABLE;
 *         flexcanConfig->disableSelfReception = ENABLE;
 *         flexcanConfig->enableListenOnlyMode = DISABLE;
 *         flexcanConfig->enableDoze           = DISABLE;
 *         flexcanConfig.timingConfig          = timingConfig;
 * @endcode
 * @param  pConfig Pointer to the CAN configuration structure.
 * @param  TrMode This parameter can be one of the following values:
 *         @arg @ref CAN_TRANSMIT_MODE_NORMAL
 *         @arg @ref CAN_TRANSMIT_MODE_LISTENONLY
 *         @arg @ref CAN_TRANSMIT_MODE_LOOPBACK
 *         @arg @ref CAN_TRANSMIT_MODE_LOOPBACKONLY
 * @retval None
 */
ErrorStatus HAL_CAN_GetDefaultConfig(CAN_ConfigTypeDef *pConfig, CAN_TransmitModeTypeDef TrMode)
{
    ErrorStatus tmp_err = SUCCESS;

    /*!< Initializes the configure structure to zero. */
    (void)memset(pConfig, 0, sizeof(*pConfig));

    /*!< Initialize CAN Module config struct with default value. */
    pConfig->clkSrc = CAN_CLOCK_SOURCE_PLL_OSC_SYNC;
    pConfig->maxMbNum = 16U;

    pConfig->enableTimerSync = (uint8_t)ENABLE;
    pConfig->enableSelfWakeup = (uint8_t)DISABLE;
    pConfig->wakeupSrc = CAN_WAKEUP_SOURCE_UNFILTERED;
    pConfig->enableIndividMask = (uint8_t)ENABLE;

    pConfig->enableLoopBack = (uint8_t)DISABLE;
    pConfig->disableSelfReception = (uint8_t)ENABLE;
    pConfig->enableListenOnlyMode = (uint8_t)DISABLE;

    /*!< Enable Listen Only Mode. */
    if (CAN_TRANSMIT_MODE_LISTENONLY == TrMode) {
        pConfig->enableLoopBack = (uint8_t)DISABLE;
        pConfig->disableSelfReception = (uint8_t)ENABLE;
        pConfig->enableListenOnlyMode = (uint8_t)ENABLE;
        /*!< Enable Loop Back Self Test Mode. */
    } else if (CAN_TRANSMIT_MODE_LOOPBACKONLY == TrMode) {
        pConfig->enableLoopBack = (uint8_t)ENABLE;
        pConfig->disableSelfReception = (uint8_t)DISABLE;
        pConfig->enableListenOnlyMode = (uint8_t)DISABLE;
        /*!< Enable Transmit LoopBack Mode. */
    } else if (CAN_TRANSMIT_MODE_LOOPBACK == TrMode) {
        pConfig->enableLoopBack = (uint8_t)DISABLE;
        pConfig->disableSelfReception = (uint8_t)DISABLE;
        pConfig->enableListenOnlyMode = (uint8_t)DISABLE;
        /*!< Enable Transmit Normal Mode. */
    } else {
        pConfig->enableLoopBack = (uint8_t)DISABLE;
        pConfig->disableSelfReception = (uint8_t)ENABLE;
        pConfig->enableListenOnlyMode = (uint8_t)DISABLE;
    }

    return tmp_err;
}

/**
 * @brief  Gets the Rx FIFO Head address.
 * @note   This function returns the CAN Rx FIFO Head address, which is
 *         mainly used for the DMA/eDMA use case.
 * @param  can CAN instance
 * @return CAN Rx FIFO Head address.
 */
uint32_t HAL_CAN_GetRxFifoHeadAddress(CAN_TypeDef *can)
{
    return ((uint32_t)&(can->MB[0].CS));
}

/**
 * @brief  Get the CAN Bus Error Counter value.
 * @note   This function gets the CAN Bus Error Counter value for both Tx and
 *         Rx direction. These values may be needed in the upper layer error handling.
 * @param  can CAN instance
 * @param  pTxErrBuf Buffer to store Tx Error Counter value.
 * @param  pRxErrBuf Buffer to store Rx Error Counter value.
 * @retval None
 */
void HAL_CAN_GetBusErrorCount(CAN_TypeDef *can, uint8_t *pTxErrBuf, uint8_t *pRxErrBuf)
{
    if (NULL != pTxErrBuf) {
        *pTxErrBuf = (uint8_t)((can->ECR & CAN_ECR_TXERRCNT) >> CAN_ECR_TXERRCNT_POS);
    }
    if (NULL != pRxErrBuf) {
        *pRxErrBuf = (uint8_t)((can->ECR & CAN_ECR_RXERRCNT) >> CAN_ECR_RXERRCNT_POS);
    }
}

/**
 * @brief  Sets the CAN protocol timing characteristic.
 * @param  can CAN instance
 * @param  pConfig Pointer to the timing configuration structure.
 * @retval None
 */
void HAL_CAN_SetTimingConfig(CAN_TypeDef *can, const CAN_TimingConfigTypeDef *pConfig)
{
    /*!< Enter Freeze Mode. */
    HAL_CAN_EnterFreezeMode(can);

    can->CTRL2 |= CAN_CTRL2_BTEEN;
    can->EPRS = (((uint32_t)pConfig->preDivider - 1U) << CAN_EPRS_ENPRESDIV_POS);
    can->ECBT =
        ((((uint32_t)pConfig->propSeg + (uint32_t)pConfig->phaseSeg1 - 1U) << CAN_ECBT_NTSEG1_POS) |
         (((uint32_t)pConfig->phaseSeg2 - 1U) << CAN_ECBT_NTSEG2_POS) |
         (((uint32_t)pConfig->reSyncJumpwidth - 1U) << CAN_ECBT_NRJW_POS));

    /*!< Exit Freeze Mode. */
    HAL_CAN_ExitFreezeMode(can);
}

/**
 * @brief  Set the CAN Rx Mailbox global mask.
 * @note   This function sets the global mask for the CAN Mailboxes in a matching process.
 *         The configuration is only effective when the Rx individual mask is
 *         disabled in the HAL_CAN_Init().
 * @param  can CAN instance
 * @param  Mask Rx MailBox Global Mask value.
 * @retval None
 */
void HAL_CAN_SetRxMbGlobalMask(CAN_TypeDef *can, uint32_t Mask)
{
    /*!< Enter Freeze Mode. */
    HAL_CAN_EnterFreezeMode(can);

    /*!< Setting Rx Message Buffer Global Mask value. */
    can->RXMGMASK = Mask;
    can->RX14MASK = Mask;
    can->RX15MASK = Mask;

    /*!< Exit Freeze Mode. */
    HAL_CAN_ExitFreezeMode(can);
}

/**
 * @brief  Set the CAN Rx FIFO global mask.
 * @note   This function sets the global mask for CAN FIFO in a matching process.
 * @param  can CAN instance
 * @param  Mask Rx Fifo Global Mask value.
 * @retval None
 */
void HAL_CAN_SetRxFifoGlobalMask(CAN_TypeDef *can, uint32_t Mask)
{
    /*!< Enter Freeze Mode. */
    HAL_CAN_EnterFreezeMode(can);

    /*!< Setting Rx FIFO Global Mask value. */
    can->RXFGMASK = Mask;

    /*!< Exit Freeze Mode. */
    HAL_CAN_ExitFreezeMode(can);
}

/**
 * @brief  Set the CAN Rx individual mask.
 * @note   This function sets the individual mask for the CAN matching process.
 *         The configuration is only effective when the Rx individual mask is
 *         enabled in the HAL_CAN_Init().
 *         a.If the Rx FIFO is disabled, the individual mask is applied to the
 *         corresponding MailBox.
 *         b.If the Rx FIFO is enabled, the individual mask for Rx FIFO occupied
 *         MailBox is applied to the Rx Filter with the same index.
 *         Note that only the first 32 individual masks can be used as the
 *         Rx FIFO filter mask.
 * @param  can CAN instance
 * @param  MaskIdx The Index of individual Mask(0~15).
 * @param  Mask Rx Individual Mask value.
 * @retval None
 */
void HAL_CAN_SetRxIndividualMask(CAN_TypeDef *can, uint8_t MaskIdx, uint32_t Mask)
{
    /*!< Enter Freeze Mode. */
    HAL_CAN_EnterFreezeMode(can);

    /*!< Setting Rx Individual Mask value. */
    can->RXIMR[MaskIdx] = Mask;

    /*!< Exit Freeze Mode. */
    HAL_CAN_ExitFreezeMode(can);
}

/**
 * @brief  Configures the CAN Tx mailbox.
 * @note   This function aborts the previous transmission, cleans the MailBox,
 *         and configures it as a Transmit MailBox.
 * @param  can CAN instance
 * @param  MbIdx The MailBox index.
 * @param  State This parameter can be: ENABLE or DISABLE.
 * @retval None
 */
void HAL_CAN_ConfigTxMb(CAN_TypeDef *can, uint8_t MbIdx, FunctionalState State)
{
    /*!< Inactivate Message Buffer. */
    if (State == ENABLE) {
        can->MB[MbIdx].CS = (((uint32_t)CAN_TXMBINACTIVE) << CAN_CS_CODE_POS);
    } else {
        can->MB[MbIdx].CS = 0U;
    }

    /*!< Clean Message Buffer content. */
    can->MB[MbIdx].ID = 0U;
    can->MB[MbIdx].WORD0 = 0U;
    can->MB[MbIdx].WORD1 = 0U;
}

/**
 * @brief  Configures the CAN Rx MailBox.
 * @note   This function cleans a CAN build-in MailBox and configures it as
 *         a Receive MailBox.
 * @param  can CAN instance
 * @param  MbIdx The MailBox index.
 * @param  pRxMbConfig Pointer to the CAN MailBox configuration structure.
 * @param  State This parameter can be: ENABLE or DISABLE.
 * @retval None
 */
void HAL_CAN_ConfigRxMb(CAN_TypeDef *can, uint8_t MbIdx, const CAN_RxMbConfigTypeDef *pRxMbConfig, FunctionalState State)
{
    uint32_t cs_temp = 0U;

    /*!< Inactivate Message Buffer. */
    can->MB[MbIdx].CS = 0U;

    /*!< Clean Message Buffer content. */
    can->MB[MbIdx].ID = 0U;
    can->MB[MbIdx].WORD0 = 0U;
    can->MB[MbIdx].WORD1 = 0U;

    if (State == ENABLE) {
        /*!< Setup Message Buffer ID. */
        can->MB[MbIdx].ID = pRxMbConfig->id;

        /*!< Setup Message Buffer format. */
        if (CAN_FRAME_FORMAT_EXTEND == pRxMbConfig->format) {
            cs_temp |= CAN_CS_IDE;
        }

        /*!< Setup Message Buffer type. */
        if (CAN_FRAME_TYPE_REMOTE == pRxMbConfig->type) {
            cs_temp |= CAN_CS_RTR;
        }

        /*!< Activate Rx Message Buffer. */
        cs_temp |= (((uint32_t)CAN_RXMBEMPTY) << CAN_CS_CODE_POS);
        can->MB[MbIdx].CS = cs_temp;
    }
}

/**
 * @brief  Configure the CAN Rx FIFO.
 * @note   This function configures the Rx FIFO with given Rx FIFO configuration.
 * @param  can CAN instance
 * @param  pRxFifoConfig Pointer to the CAN Rx FIFO configuration structure.
 * @param  State This parameter can be: ENABLE or DISABLE.
 * @retval None
 */
void HAL_CAN_ConfigRxFifo(CAN_TypeDef *can, const CAN_RxFifoConfigTypeDef *pRxFifoConfig, FunctionalState State)
{
    uint32_t k, tmpAddr_base, tmpAddr;
    uint8_t i, j, rffn = 0U, numMbOccupy;
    uint32_t tmp_epreg, setup_mb = 0U;

    /*!< Enter Freeze Mode. */
    HAL_CAN_EnterFreezeMode(can);
    if (State == ENABLE) {
        /*!< Get the setup_mb value. */
        setup_mb = (uint8_t)((can->MCR & CAN_MCR_MAXMB) >> CAN_MCR_MAXMB_POS);
        setup_mb = ((setup_mb < (uint32_t)CAN_HAS_MB_MAX_NUM)
                    ? setup_mb
                    : (uint32_t)CAN_HAS_MB_MAX_NUM);

        /*!< Determine RFFN value, max = 4, 8 message box. */
        for (i = 0; i <= 0x4U; i++) {
            if ((8U * (i + 1U)) >= pRxFifoConfig->idFilterNum) {
                rffn = i;
                /*!< assert(((setup_mb - 8U) - (2U * rffn)) > 0U); */
                can->CTRL2 = ((can->CTRL2 & ~CAN_CTRL2_RFFN) | ((uint32_t)rffn << CAN_CTRL2_RFFN_POS));
                break;
            }
        }

        /*!< caculate the Number of Mailboxes occupied by RX Legacy FIFO and the filter. */
        numMbOccupy = (6U + (rffn + 1U) * 2U);

        /*!< Copy ID filter table to Message Buffer Region (Fix MISRA_C-2012 Rule 18.1). */
        j = 0U;

        for (i = 6U; i < numMbOccupy; i++) {
            /*!< Get address for current mail box. */
            tmpAddr_base = (uint32_t)&(can->MB[i].CS);

            /*!< One Mail box contain 4U DWORD registers. */
            for (k = 0; k < 4U; k++) {
                /*!< Fill all valid filter in the mail box occupied by filter. */
                /*!< Disable unused Rx FIFO Filter, the other rest of register in the last Mail box occupied by fiter set as 0xffffffff. */
                tmpAddr = (tmpAddr_base + k * 4U);
                *(volatile uint32_t *)(tmpAddr) = ((j < pRxFifoConfig->idFilterNum)
                                                   ? (pRxFifoConfig->idFilterTable[j])
                                                   : 0xFFFFFFFFU);
                /*!< Try to fill next filter in current Mail Box. */
                j++;
            }
        }

        /*!< Setup ID Fitlter Type. */
        switch (pRxFifoConfig->idFilterType) {
        case CAN_RXFIFO_FILTER_TYPEA:
            can->MCR = ((can->MCR & ~CAN_MCR_IDAM) | (((uint32_t)0x0U) << CAN_MCR_IDAM_POS));
            break;
        case CAN_RXFIFO_FILTER_TYPEB:
            can->MCR = ((can->MCR & ~CAN_MCR_IDAM) | (((uint32_t)0x1U) << CAN_MCR_IDAM_POS));
            break;
        case CAN_RXFIFO_FILTER_TYPEC:
            can->MCR = ((can->MCR & ~CAN_MCR_IDAM) | (((uint32_t)0x2U) << CAN_MCR_IDAM_POS));
            break;
        case CAN_RXFIFO_FILTER_TYPED:
            /*!< All frames rejected. */
            can->MCR = ((can->MCR & ~CAN_MCR_IDAM) | (((uint32_t)0x3U) << CAN_MCR_IDAM_POS));
            break;
        default:
            /*!< All the cases have been listed above, the default clause should not be reached. */
            break;
        }

        /*!< Setting Message Reception Priority. */
        tmp_epreg = can->CTRL2;
        can->CTRL2 = ((pRxFifoConfig->priority == CAN_RXFIFO_PRIORITY_HIGH)
                      ? (tmp_epreg & ~CAN_CTRL2_MRP)
                      : (tmp_epreg | CAN_CTRL2_MRP));

        /*!< Enable Rx Message FIFO. */
        can->MCR |= CAN_MCR_RFEN;
    } else {
        rffn = (uint8_t)((can->CTRL2 & CAN_CTRL2_RFFN) >> CAN_CTRL2_RFFN_POS);
        /*!< caculate the Number of Mailboxes occupied by RX Legacy FIFO and the filter. */
        numMbOccupy = (6U + (rffn + 1U) * 2U);

        /*!< Disable Rx Message FIFO. */
        can->MCR &= ~CAN_MCR_RFEN;

        /*!< Clean MB0 ~ MB5 and all MB occupied by ID filters (Fix MISRA_C-2012 Rule 18.1). */
        for (i = 0; i < numMbOccupy; i++) {
            HAL_CAN_ConfigRxMb(can, i, NULL, DISABLE);
        }
    }

    /*!< Exit Freeze Mode. */
    HAL_CAN_ExitFreezeMode(can);
}

/**
 * @brief  Configure the CAN Rx DMA.
 * @note   This function configures Rx DMA.
 * @param  can CAN instance
 * @param  Frame Pointer to the dma rx Buffer.
 * @param  FrameNum FrameNumber * sizeof(HAL_CAN_FrameTypeDef) in bytes.
 * @retval None
 */
void HAL_CAN_ConfigRxDMA(CAN_TypeDef *can, CAN_FrameTypeDef *Frame, uint32_t FrameNum)
{
    /*!< Enter Freeze Mode. */
    HAL_CAN_EnterFreezeMode(can);

    can->DMA_RBADR = (uint32_t)Frame;
    can->DMA_RLEN = (FrameNum * sizeof(CAN_FrameTypeDef));

    /*!< Exit Freeze Mode. */
    HAL_CAN_ExitFreezeMode(can);
}

/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup CAN_Private_Functions
 * @{
 */

/**
 * @brief  Reset the CAN register to default values.
 * @param  can CAN instance
 * @retval None
 */
static void can_reset(CAN_TypeDef *can)
{
    /*!< The module must should be first exit from low power mode, and then soft reset can be applied. */
    uint8_t i;

    /*!< Wait until CAN exit from any Low Power Mode. */
    while (0U != (can->MCR & CAN_MCR_LPMACK)) {
    }

    /*!< assert Soft Reset Signal. */
    can->MCR |= CAN_MCR_SOFTRST;

    /*!< Wait until CAN reset completes. */
    while (0U != (can->MCR & CAN_MCR_SOFTRST)) {
    }

    /*!< Reset MCR register. */
    can->MCR |= (CAN_MCR_WRNEN | (((uint32_t)CAN_HAS_MB_MAX_NUM - 1U) << CAN_MCR_MAXMB_POS));

    /*!< Reset CTRL1 and CTRL2 register. */
    can->CTRL1 = CAN_CTRL1_SMP;
    can->CTRL2 = (((uint32_t)0x16U << CAN_CTRL2_TASD_POS) | CAN_CTRL2_RRS | CAN_CTRL2_EACEN);

    /*!< Clean all individual Rx Mask of Message Buffers. */
    for (i = 0; i < (uint32_t)CAN_HAS_MB_MAX_NUM; i++) {
        can->RXIMR[i] = 0x3FFFFFFFU;
    }

    /*!< Clean Global Mask of Message Buffers. */
    can->RXMGMASK = 0x3FFFFFFF;
    /*!< Clean Global Mask of Message Buffer 14. */
    can->RX14MASK = 0x3FFFFFFF;
    /*!< Clean Global Mask of Message Buffer 15. */
    can->RX15MASK = 0x3FFFFFFF;
    /*!< Clean Global Mask of Rx FIFO. */
    can->RXFGMASK = 0x3FFFFFFF;

    /*!< Clean all Message Buffer CS fields. */
    for (i = 0; i < (uint32_t)CAN_HAS_MB_MAX_NUM; i++) {
        can->MB[i].CS = 0U;
    }
}

/**
 * @brief  Set the CAN Baud rate.
 * @param  can CAN instance
 * @param  timingConfig CAN timing configuration.
 * @retval None
 */
static void can_baudRate_set(CAN_TypeDef *can, CAN_TimingConfigTypeDef timingConfig)
{
    /*!< Update actual timing characteristic. */
    HAL_CAN_SetTimingConfig(can, (const CAN_TimingConfigTypeDef *)(uint32_t)&timingConfig);
}

/**
 * @}
 */

#endif /* HAL_CAN_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
