/**
 ******************************************************************************
 * @file   rn_rv_hal_uart.h
 * @author Renergy Software Team
 * @brief  Header file of UART HAL module.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RN_RV_HAL_UART_H
#define __RN_RV_HAL_UART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal_def.h"
#include "rn_rv_ll_uart0_5.h"
#include "rn_rv_ll_uart6_7.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup UART
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup UART_Exported_Types UART Exported Types
 * @{
 */

/**
 * @brief UART Init Structure definition
 */
typedef struct
{
    uint32_t BaudRate;                  /*!< This member configures the UART communication baud rate.
                                            The baud rate is computed using the following formula:
                                            - IntegerDivider = ((PCLKx) / (16 * (huart->Init.BaudRate)) - 1)
                                            - FractionalDivider = ((((PCLKx) / (16 * (huart->Init.BaudRate))) - ((uint32_t) ((PCLKx) / (16 * (huart->Init.BaudRate))))) * 2^14) + 0.5 */

    uint32_t DataWidth;                /*!< Specifies the number of data bits transmitted or received in a frame.
                                            This parameter can be a value of @ref UART_Data_Width */

    uint32_t StopBits;                  /*!< Specifies the number of stop bits transmitted.
                                            This parameter can be a value of @ref UART_Stop_Bits */

    uint32_t Parity;                    /*!< Specifies the parity mode.
                                            This parameter can be a value of @ref UART_Parity */

    uint32_t RxTimeout;                 /*!< Specifies the received timeout, for n Data Width. If set 0 is invalid.
                                            This parameter can be a value between Min_Data=0x00 and Max_Data=0xFF */
} UART_InitTypeDef;

/**
 * @brief HAL UART State structures definition
 * @note  HAL UART State value is a combination of 2 different substates: gState and RxState.
 *        - gState contains UART state information related to global Handle management
 *          and also information related to Tx operations.
 *          gState value coding follow below described bitmap :
 *          b7-b6  Error information
 *             00 : No Error
 *             01 : (Not Used)
 *             10 : Timeout
 *             11 : Error
 *          b5     Peripheral initialization status
 *             0  : Reset (Peripheral not initialized)
 *             1  : Init done (Peripheral initialized. HAL UART Init function already called)
 *          b4-b3  (not used)
 *             xx : Should be set to 00
 *          b2     Intrinsic process state
 *             0  : Ready
 *             1  : Busy (Peripheral busy with some configuration or internal operations)
 *          b1     (not used)
 *             x  : Should be set to 0
 *          b0     Tx state
 *             0  : Ready (no Tx operation ongoing)
 *             1  : Busy (Tx operation ongoing)
 *        - RxState contains information related to Rx operations.
 *          RxState value coding follow below described bitmap :
 *          b7-b6  (not used)
 *             xx : Should be set to 00
 *          b5     Peripheral initialization status
 *             0  : Reset (Peripheral not initialized)
 *             1  : Init done (Peripheral initialized)
 *          b4-b2  (not used)
 *            xxx : Should be set to 000
 *          b1     Rx state
 *             0  : Ready (no Rx operation ongoing)
 *             1  : Busy (Rx operation ongoing)
 *          b0     (not used)
 *             x  : Should be set to 0.
 */
typedef enum __attribute__((__packed__))
{
    HAL_UART_STATE_RESET             = 0x00U,    /*!< Peripheral is not yet Initialized
                                                    Value is allowed for gState and RxState */
    HAL_UART_STATE_READY             = 0x20U,    /*!< Peripheral Initialized and ready for use
                                                    Value is allowed for gState and RxState */
    HAL_UART_STATE_BUSY              = 0x24U,    /*!< an internal process is ongoing
                                                    Value is allowed for gState only */
    HAL_UART_STATE_BUSY_TX           = 0x21U,    /*!< Data Transmission process is ongoing
                                                    Value is allowed for gState only */
    HAL_UART_STATE_BUSY_RX           = 0x22U,    /*!< Data Reception process is ongoing
                                                    Value is allowed for RxState only */
    HAL_UART_STATE_BUSY_TX_RX        = 0x23U,    /*!< Data Transmission and Reception process is ongoing
                                                    Not to be used for neither gState nor RxState.
                                                    Value is result of combination (Or) between gState and RxState values */
    HAL_UART_STATE_TIMEOUT           = 0xA0U,    /*!< Timeout state
                                                    Value is allowed for gState only */
    HAL_UART_STATE_ERROR             = 0xE0U     /*!< Error
                                                    Value is allowed for gState only */
} HAL_UART_StateTypeDef;

/**
 * @brief HAL UART Reception type definition
 * @note  HAL UART Reception type value aims to identify which type of Reception is ongoing.
 *        This parameter can be a value of @ref UART_Reception_Type_Values :
 *           HAL_UART_RECEPTION_STANDARD         = 0x00U,
 *           HAL_UART_RECEPTION_TOIDLE           = 0x01U,
 */
typedef uint32_t HAL_UART_RxTypeTypeDef;

/**
 * @brief HAL UART Rx Event type definition
 * @note  HAL UART Rx Event type value aims to identify which type of Event has occurred
 *        leading to call of the RxEvent callback.
 *        This parameter can be a value of @ref UART_RxEvent_Type_Values :
 *           HAL_UART_RXEVENT_TC                 = 0x00U,
 *           HAL_UART_RXEVENT_HT                 = 0x01U,
 *           HAL_UART_RXEVENT_IDLE               = 0x02U,
 */
typedef uint32_t HAL_UART_RxEventTypeTypeDef;

/**
 * @brief  UART DMA handle Structure definition
 */
typedef struct __UART_DMA_HandleTypeDef
{
    uint32_t                    Mode;                                                                 /*!< UART DMA mode @ref UART_DMA_Mode.      */
    
    uint32_t                    Parity;                                                               /*!< UART DMA Parity @ref UART_DMA_Parity  */

    void                       *Parent;                                                               /*!< Parent object state                    */

    void                       (* XferCpltCallback)( struct __UART_DMA_HandleTypeDef * hdma);         /*!< DMA transfer complete callback         */

    void                       (* XferHalfCpltCallback)( struct __UART_DMA_HandleTypeDef * hdma);     /*!< DMA Half transfer complete callback    */

    void                       (* XferErrorCallback)( struct __UART_DMA_HandleTypeDef * hdma);        /*!< DMA transfer error callback            */

    void                       (* XferAbortCallback)( struct __UART_DMA_HandleTypeDef * hdma);        /*!< DMA transfer Abort callback            */

} UART_DMA_HandleTypeDef;

/**
 * @brief  UART handle Structure definition
 */
typedef struct __UART_HandleTypeDef
{
    union {
        UART0_5_TypeDef      *Instance0_5;             /*!< UART0_5 registers base address     */
        UART6_7_TypeDef      *Instance6_7;             /*!< UART6_7 registers base address     */
    } Instance;

    uint8_t                  InstanceType;             /*!< UART0_5 or UART6_7 Instance        */

    UART_InitTypeDef         Init;                     /*!< UART communication parameters      */

    const uint8_t            *pTxBuffPtr;              /*!< Pointer to UART Tx transfer Buffer */

    uint16_t                 TxXferSize;               /*!< UART Tx Transfer size              */

    __IO uint16_t            TxXferCount;              /*!< UART Tx Transfer Counter           */

    uint8_t                  *pRxBuffPtr;              /*!< Pointer to UART Rx transfer Buffer */

    uint16_t                 RxXferSize;               /*!< UART Rx Transfer size              */

    __IO uint16_t            RxXferCount;              /*!< UART Rx Transfer Counter           */

    __IO HAL_UART_RxTypeTypeDef ReceptionType;         /*!< Type of ongoing reception          */

    __IO HAL_UART_RxEventTypeTypeDef RxEventType;      /*!< Type of Rx Event                   */

    UART_DMA_HandleTypeDef       hdmatx;                  /*!< UART Tx DMA Handle parameters      */

    UART_DMA_HandleTypeDef       hdmarx;                  /*!< UART Rx DMA Handle parameters      */

    HAL_LockTypeDef              Lock;                    /*!< Locking object                     */

    __IO HAL_UART_StateTypeDef    gState;              /*!< UART state information related to global Handle management
                                                            and also related to Tx operations. This parameter
                                                            can be a value of @ref HAL_UART_StateTypeDef */

    __IO HAL_UART_StateTypeDef    RxState;             /*!< UART state information related to Rx operations. This
                                                            parameter can be a value of @ref HAL_UART_StateTypeDef */

    __IO uint32_t                 ErrorCode;           /*!< UART Error code                    */

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
    void (* TxHalfCpltCallback)(struct __UART_HandleTypeDef *huart);        /*!< UART Tx Half Complete Callback        */
    void (* TxCpltCallback)(struct __UART_HandleTypeDef *huart);            /*!< UART Tx Complete Callback             */
    void (* RxHalfCpltCallback)(struct __UART_HandleTypeDef *huart);        /*!< UART Rx Half Complete Callback        */
    void (* RxCpltCallback)(struct __UART_HandleTypeDef *huart);            /*!< UART Rx Complete Callback             */
    void (* ErrorCallback)(struct __UART_HandleTypeDef *huart);             /*!< UART Error Callback                   */
    void (* AbortCpltCallback)(struct __UART_HandleTypeDef *huart);         /*!< UART Abort Complete Callback          */
    void (* AbortTransmitCpltCallback)(struct __UART_HandleTypeDef *huart); /*!< UART Abort Transmit Complete Callback */
    void (* AbortReceiveCpltCallback)(struct __UART_HandleTypeDef *huart);  /*!< UART Abort Receive Complete Callback  */
    void (* RxEventCallback)(struct __UART_HandleTypeDef *huart, uint16_t Pos); /*!< UART Reception Event Callback     */

    void (* MspInitCallback)(struct __UART_HandleTypeDef *huart);           /*!< UART Msp Init callback                */
    void (* MspDeInitCallback)(struct __UART_HandleTypeDef *huart);         /*!< UART Msp DeInit callback              */
#endif  /* USE_HAL_UART_REGISTER_CALLBACKS */

} UART_HandleTypeDef;

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
/**
 * @brief  HAL UART Callback ID enumeration definition
 */
typedef enum
{
    HAL_UART_TX_HALFCOMPLETE_CB_ID         = 0x00U,    /*!< UART Tx Half Complete Callback ID        */
    HAL_UART_TX_COMPLETE_CB_ID             = 0x01U,    /*!< UART Tx Complete Callback ID             */
    HAL_UART_RX_HALFCOMPLETE_CB_ID         = 0x02U,    /*!< UART Rx Half Complete Callback ID        */
    HAL_UART_RX_COMPLETE_CB_ID             = 0x03U,    /*!< UART Rx Complete Callback ID             */
    HAL_UART_ERROR_CB_ID                   = 0x04U,    /*!< UART Error Callback ID                   */
    HAL_UART_ABORT_COMPLETE_CB_ID          = 0x05U,    /*!< UART Abort Complete Callback ID          */
    HAL_UART_ABORT_TRANSMIT_COMPLETE_CB_ID = 0x06U,    /*!< UART Abort Transmit Complete Callback ID */
    HAL_UART_ABORT_RECEIVE_COMPLETE_CB_ID  = 0x07U,    /*!< UART Abort Receive Complete Callback ID  */

    HAL_UART_MSPINIT_CB_ID                 = 0x0BU,    /*!< UART MspInit callback ID                 */
    HAL_UART_MSPDEINIT_CB_ID               = 0x0CU     /*!< UART MspDeInit callback ID               */

} HAL_UART_CallbackIDTypeDef;

/**
 * @brief  HAL UART Callback pointer definition
 */
typedef  void (*pUART_CallbackTypeDef)(UART_HandleTypeDef *huart); /*!< pointer to an UART callback function */
typedef  void (*pUART_RxEventCallbackTypeDef)(struct __UART_HandleTypeDef *huart, uint16_t Pos); /*!< pointer to a UART Rx Event specific callback function */

#endif /* USE_HAL_UART_REGISTER_CALLBACKS */

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup UART_Exported_Constants UART Exported Constants
 * @{
 */

/** @defgroup UART_Error_Code UART Error Code
 * @{
 */
#define HAL_UART_ERROR_NONE              0x00000000U   /*!< No error            */
#define HAL_UART_ERROR_PE                0x00000004U   /*!< Parity error        */
#define HAL_UART_ERROR_OE                0x00000008U   /*!< Overflow error      */
#define HAL_UART_ERROR_FE                0x00000010U   /*!< Frame error         */
#define HAL_UART_ERROR_DE                0x00000020U   /*!< Data error          */
#define HAL_UART_ERROR_DMA               0x00000040U   /*!< DMA transfer error  */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
#define  HAL_UART_ERROR_INVALID_CALLBACK 0x00000080U   /*!< Invalid Callback error  */
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
/**
 * @}
 */

/** @defgroup UART_Data_Width UART Word Length
 * @{
 */
#define UART_DATAWIDTH_5B                  (LL_UART0_5_DATAWIDTH_5B)
#define UART_DATAWIDTH_6B                  (LL_UART0_5_DATAWIDTH_6B)
#define UART_DATAWIDTH_7B                  (LL_UART0_5_DATAWIDTH_7B)
#define UART_DATAWIDTH_8B                  (LL_UART0_5_DATAWIDTH_8B)
/**
 * @}
 */

/** @defgroup UART_Stop_Bits   UART Number of Stop Bits
 * @{
 */
#define UART_STOPBITS_1                     (LL_UART0_5_STOPBITS_1)                    /*!< UART frame with 1 stop bit    */
#define UART_STOPBITS_2                     (LL_UART0_5_STOPBITS_2)                     /*!< UART frame with 2 stop bits   */
/**
 * @}
 */

/** @defgroup UART_Parity UART Parity
 * @{
 */
#define UART_PARITY_NONE                    (LL_UART0_5_PARITY_NONE)
#define UART_PARITY_ODD                     (LL_UART0_5_PARITY_ODD)
#define UART_PARITY_EVEN                    (LL_UART0_5_PARITY_EVEN)
#define UART_PARITY_ZERO                    (LL_UART0_5_PARITY_ZERO)
#define UART_PARITY_ONE                     (LL_UART0_5_PARITY_ONE)
#define UART_PARITY_USER_DEFINED            (LL_UART0_5_PARITY_USER_DEFINED)
/**
 * @}
 */

/** @defgroup UART_Instance_Type_Values  UART Instance type
 * @{
 */
#define UART_INSTANCETYPE_0_5               ((uint8_t)(0x01U))             /*!< Instance use uart0_5 */
#define UART_INSTANCETYPE_6_7               ((uint8_t)(0x10U))             /*!< Instance use uart6_7 */
/**
 * @}
 */

/** @defgroup UART_DMA_Mode UART DMA Mode
 * @{
 */
#define UART_DMA_NORMAL         0x00000000U                                /*!< Normal mode                  */
#define UART_DMA_CIRCULAR       ((uint32_t)UART0_5_DMA_CTRL_TX_CYC_MODE)   /*!< Circular mode                */
/**
 * @}
 */

/** @defgroup UART_DMA_Parity UART DMA Parity
 * @{
 */
#define UART_DMA_PARITY_ZERO    (LL_UART0_5_DMA_PARITY_ZERO)               /*!< DMA Transmit Fixed to 0 parity */
#define UART_DMA_PARITY_ONE     (LL_UART0_5_DMA_PARITY_ONE)                /*!< DMA Transmit Fixed to 1 parity */
/**
 * @}
 */

/** @defgroup UART_Flags   UART FLags
 * @{
 */
#define UART_FLAG_RXTO                      ((uint32_t)UART0_5_STA_RX_TO)
#define UART_FLAG_FIFOF                     ((uint32_t)UART0_5_STA_TX_FIFO_FULL)
#define UART_FLAG_FIFOE                     ((uint32_t)UART0_5_STA_TX_FIFO_EMPTY)
#define UART_FLAG_TB                        ((uint32_t)UART0_5_STA_TB)
#define UART_FLAG_RB                        ((uint32_t)UART0_5_STA_RB)
#define UART_FLAG_DE                        ((uint32_t)UART0_5_STA_DE)
#define UART_FLAG_FE                        ((uint32_t)UART0_5_STA_FE)
#define UART_FLAG_OE                        ((uint32_t)UART0_5_STA_OE)
#define UART_FLAG_PE                        ((uint32_t)UART0_5_STA_PE)
#define UART_FLAG_TX                        ((uint32_t)UART0_5_STA_TX)
#define UART_FLAG_RX                        ((uint32_t)UART0_5_STA_RX)

#define UART_DMA_FLAG_TXF                   ((uint32_t)UART0_5_DMA_IF_TX_FDONE)
#define UART_DMA_FLAG_TXH                   ((uint32_t)UART0_5_DMA_IF_TX_HDONE)

#define UART_DMA_FLAG_RXF                   ((uint32_t)UART0_5_DMA_IF_RX_FDONE)
#define UART_DMA_FLAG_RXH                   ((uint32_t)UART0_5_DMA_IF_RX_HDONE)
#define UART_DMA_FLAG_RXE                   ((uint32_t)UART0_5_DMA_IF_RX_ERR)
#define UART_DMA_FLAG_RXDONE                ((uint32_t)UART0_5_DMA_IF_RX_DONE)
/**
 * @}
 */

/** @defgroup UART_Interrupt_definition  UART Interrupt Definitions
 * @{
 */

#define UART_IT_TX                       ((uint32_t)UART0_5_CTRL_TXIE)
#define UART_IT_RX                       ((uint32_t)UART0_5_CTRL_RXIE)
#define UART_IT_ERR                      ((uint32_t)UART0_5_CTRL_ERRIE)
#define UART_IT_RXTO                     ((uint32_t)UART0_5_CTRL_RX_TO_IE)

#define UART_IT_PF                       ((uint32_t)UART6_7_CTRL_PF_IE)
#define UART_IT_QF                       ((uint32_t)UART6_7_CTRL_QF_IE)
#define UART_IT_FPF                      ((uint32_t)UART6_7_CTRL_FPF_IE)
#define UART_IT_CFERR                    ((uint32_t)UART6_7_CTRL_CF_ERR_IE)
#define UART_IT_TW                       ((uint32_t)UART6_7_CTRL_TW_IE)
#define UART_IT_CAL                      ((uint32_t)UART6_7_CTRL_CAL_IE)
#define UART_IT_IRQ                      ((uint32_t)UART6_7_CTRL_RX_IRQ_IE)

#define UART_DMA_IT_TXF                  ((uint32_t)UART0_5_DMA_IE_TX_FIE)
#define UART_DMA_IT_TXH                  ((uint32_t)UART0_5_DMA_IE_TX_HIE)
#define UART_DMA_IT_TXE                  ((uint32_t)UART0_5_DMA_IE_TX_ERR_IE)

#define UART_DMA_IT_RXF                  ((uint32_t)UART0_5_DMA_IE_RX_FIE)
#define UART_DMA_IT_RXH                  ((uint32_t)UART0_5_DMA_IE_RX_HIE)
#define UART_DMA_IT_RXE                  ((uint32_t)UART0_5_DMA_IE_RX_ERR_IE)
#define UART_DMA_IT_RXDONE               ((uint32_t)UART0_5_DMA_IE_RX_DONE_IE)
/**
  * @}
  */



/** @defgroup UART_Reception_Type_Values  UART Reception type values
 * @{
 */
#define HAL_UART_RECEPTION_STANDARD          (0x00000000U)             /*!< Standard reception                       */
#define HAL_UART_RECEPTION_TOIDLE            (0x00000001U)             /*!< Reception till completion or IDLE event  */
/**
 * @}
 */

/** @defgroup UART_RxEvent_Type_Values  UART RxEvent type values
 * @{
 */
#define HAL_UART_RXEVENT_TC                  (0x00000000U)             /*!< RxEvent linked to Transfer Complete event */
#define HAL_UART_RXEVENT_HT                  (0x00000001U)             /*!< RxEvent linked to Half Transfer event     */
#define HAL_UART_RXEVENT_IDLE                (0x00000002U)             /*!< RxEvent linked to IDLE event              */
/**
 * @}
 */

/** @defgroup UART_MASK_POS  UART MASK POS
 * @{
 */
#define UART_CTRL_EN                         (UART0_5_CTRL_EN)         /*!< RxEvent linked to Transfer Complete event */
#define UART_RXEVENT_HT                      (0x00000001U)             /*!< RxEvent linked to Half Transfer event     */
#define UART_RXEVENT_IDLE                    (0x00000002U)             /*!< RxEvent linked to IDLE event              */
/**
 * @}
 */

/**
 * @}
 */
/* Exported macros ----------------------------------------------------------*/
/** @defgroup UART_Exported_Macros UART Exported Macros
 * @{
 */

/** @brief  Checks whether the specified UART flag is set or not.
 *  @param  __HANDLE__ specifies the UART Handle.
 *         UART Handle selects the USARTx or UARTy peripheral
 *         (USART,UART availability and x,y values depending on device).
 *  @param  __FLAG__ specifies the flag to check.
 *        This parameter can be one of the following values:
 *            @arg UART_FLAG_RXTO:   Receive Timeout Flag.
 *            @arg UART_FLAG_FIFOF:  Transmit FIFO Full Flag.
 *            @arg UART_FLAG_FIFOE:  Transmit FIFO Empty Flag.
 *            @arg UART_FLAG_TB:     Transmission Busy Flag.
 *            @arg UART_FLAG_RB:     Reception Busy Flag.
 *            @arg UART_FLAG_DE:     Data Error Flag.
 *            @arg UART_FLAG_FE:     Frame Error Flag.
 *            @arg UART_FLAG_OE:     Overrun Error Flag.
 *            @arg UART_FLAG_PE:     Parity Error Flag.
 *            @arg UART_FLAG_TX:     Transmission Complete Flag.
 *            @arg UART_FLAG_RX:     Reception Complete Flag.
 *  @retval The new state of __FLAG__ (TRUE or FALSE).
 */
#define __HAL_UART_GET_FLAG(__HANDLE__, __FLAG__)   (((__HANDLE__)->InstanceType == UART_INSTANCETYPE_0_5) ? \
                                                        ((__HANDLE__)->Instance.Instance0_5->STA & (__FLAG__)) == (__FLAG__) :\
                                                        ((__HANDLE__)->Instance.Instance6_7->STA & (__FLAG__)) == (__FLAG__))

/** @brief  Clears the specified UART pending flag.
 * @param  __HANDLE__ specifies the UART Handle.
 *         UART Handle selects the UARTx peripheral
 * @param  __FLAG__ specifies the flag to check.
 *          This parameter can be any combination of the following values:
 *            @arg UART_FLAG_RXTO:   Receive Timeout Flag.
 *            @arg UART_FLAG_DE:     Data Error Flag.
 *            @arg UART_FLAG_FE:     Frame Error Flag.
 *            @arg UART_FLAG_OE:     Overrun Error Flag.
 *            @arg UART_FLAG_PE:     Parity Error Flag.
 *            @arg UART_FLAG_TX:     Transmission Complete Flag.
 *            @arg UART_FLAG_RX:     Reception Complete Flag.
 *
 * @note   FIFOF (Transmit FIFO Full), FIFOE (Transmit FIFO Empty), TB (Transmission Busy Flag) and
 *          RB (Reception Busy Flag) flags are only read.
 *
 * @retval None
 */
#define __HAL_UART_CLEAR_FLAG(__HANDLE__, __FLAG__) (((__HANDLE__)->InstanceType == UART_INSTANCETYPE_0_5) ? \
                                                        ((__HANDLE__)->Instance.Instance0_5->STA = (__FLAG__)) :\
                                                        ((__HANDLE__)->Instance.Instance6_7->STA = (__FLAG__)))

/** @brief  Enable the specified UART interrupt.
 *  @param  __HANDLE__ specifies the UART Handle.
 *         UART Handle selects the UART peripheral.
 *  @param  __INTERRUPT__ specifies the UART interrupt source to enable.
 *          This parameter can be one of the following values:
 *            @arg UART_IT_TX:   Transmit Data Interrupt
 *            @arg UART_IT_RX:   Receive Data Interrupt
 *            @arg UART_IT_ERR:  Error Interrupt
 *            @arg UART_IT_RXTO: Receive Timeout Interrupt
 *  @retval None
 */
#define __HAL_UART_ENABLE_IT(__HANDLE__, __INTERRUPT__)   (((__HANDLE__)->InstanceType == UART_INSTANCETYPE_0_5) ? \
                                                            ((__HANDLE__)->Instance.Instance0_5->CTRL |= (__INTERRUPT__)) :\
                                                            ((__INTERRUPT__ == UART_IT_RXTO) ? ((__HANDLE__)->Instance.Instance6_7->CTRL |= (__INTERRUPT__ << UART_RXTO_BIT_POS)) :\
                                                            ((__HANDLE__)->Instance.Instance6_7->CTRL |= (__INTERRUPT__))))

/** @brief  Disable the specified UART interrupt.
 *  @param  __HANDLE__ specifies the UART Handle.
 *         UART Handle selects the UART peripheral.
 *  @param  __INTERRUPT__ specifies the UART interrupt source to disable.
 *          This parameter can be one of the following values:
 *            @arg UART_IT_TX:   Transmit Data Interrupt
 *            @arg UART_IT_RX:   Receive Data Interrupt
 *            @arg UART_IT_ERR:  Error Interrupt
 *            @arg UART_IT_RXTO: Receive Timeout Interrupt
 *  @retval None
 */
#define __HAL_UART_DISABLE_IT(__HANDLE__, __INTERRUPT__)  (((__HANDLE__)->InstanceType == UART_INSTANCETYPE_0_5) ? \
                                                            ((__HANDLE__)->Instance.Instance0_5->CTRL &= ~(__INTERRUPT__)) :\
                                                            ((__INTERRUPT__ == UART_IT_RXTO) ? ((__HANDLE__)->Instance.Instance6_7->CTRL &= ~(__INTERRUPT__ << UART_RXTO_BIT_POS)) :\
                                                            ((__HANDLE__)->Instance.Instance6_7->CTRL &= ~(__INTERRUPT__))))

/** @brief  Checks whether the specified UART DMA flag is set or not.
 *  @param  __HANDLE__ specifies the UART Handle.
 *         UART Handle selects the UARTx peripheral
 *  @param  __FLAG__ specifies the flag to check.
 *        This parameter can be one of the following values:
 *            @arg UART_DMA_FLAG_TXF:    Transmit Complete Flag.
 *            @arg UART_DMA_FLAG_TXH:    Transmit Half Complete Flag.
 *            @arg UART_DMA_FLAG_RXF:    Reception Complete Flag.
 *            @arg UART_DMA_FLAG_RXH:    Reception Half Complete Flag.
 *            @arg UART_DMA_FLAG_RXE:    Reception Error Flag.
 *            @arg UART_DMA_FLAG_RXDONE: Reception Done Flag.
 *  @retval The new state of __FLAG__ (TRUE or FALSE).
 */
#define __HAL_UART_GET_DMA_FLAG(__HANDLE__, __FLAG__)   (((__HANDLE__)->InstanceType == UART_INSTANCETYPE_0_5) ? \
                                                        ((__HANDLE__)->Instance.Instance0_5->DMA_IF & (__FLAG__)) == (__FLAG__) :\
                                                        ((__HANDLE__)->Instance.Instance6_7->DMA_IF & (__FLAG__)) == (__FLAG__))

/** @brief  Clears the specified UART DMA pending flag.
 *  @param  __HANDLE__ specifies the UART Handle.
 *         UART Handle selects the UARTx peripheral
 *  @param  __FLAG__ specifies the flag to check.
 *          This parameter can be any combination of the following values:
 *            @arg UART_DMA_FLAG_TXF:    Transmit Complete Flag.
 *            @arg UART_DMA_FLAG_TXH:    Transmit Half Complete Flag.
 *            @arg UART_DMA_FLAG_RXF:    Reception Complete Flag.
 *            @arg UART_DMA_FLAG_RXH:    Reception Half Complete Flag.
 *            @arg UART_DMA_FLAG_RXE:    Reception Error Flag.
 *            @arg UART_DMA_FLAG_RXDONE: Reception Done Flag.
 *  @retval None
 */
#define __HAL_UART_CLEAR_DMA_FLAG(__HANDLE__, __FLAG__) (((__HANDLE__)->InstanceType == UART_INSTANCETYPE_0_5) ? \
                                                        ((__HANDLE__)->Instance.Instance0_5->DMA_IF = (__FLAG__)) :\
                                                        ((__HANDLE__)->Instance.Instance6_7->DMA_IF = (__FLAG__)))

/** @brief  Enable the specified UART DMA interrupt.
 *  @param  __HANDLE__ specifies the UART Handle.
 *         UART Handle selects the UARTx peripheral.
 *  @param  __INTERRUPT__ specifies the UART interrupt source to enable.
 *          This parameter can be any combination of the following values:
 *            @arg UART_DMA_IT_TXF:    Transmit Complete Interrupt.
 *            @arg UART_DMA_IT_TXH:    Transmit Half Complete Interrupt.
 *            @arg UART_DMA_IT_TXE:    Transmit ERROR Interrupt.
 *            @arg UART_DMA_IT_RXF:    Reception Complete Interrupt.
 *            @arg UART_DMA_IT_RXH:    Reception Half Complete Interrupt.
 *            @arg UART_DMA_IT_RXE:    Reception Error Interrupt.
 *            @arg UART_DMA_IT_RXDONE: Reception Done Interrupt.
 * @retval None
 */
#define __HAL_UART_ENABLE_DMA_IT(__HANDLE__, __INTERRUPT__)   (((__HANDLE__)->InstanceType == UART_INSTANCETYPE_0_5) ? \
                                                               ((__HANDLE__)->Instance.Instance0_5->DMA_IE |= (__INTERRUPT__)) :\
                                                               ((__HANDLE__)->Instance.Instance6_7->DMA_IE |= (__INTERRUPT__)))

/** @brief  Disable the specified UART DMA interrupt.
 *  @param  __HANDLE__ specifies the UART Handle.
 *         UART Handle selects the UARTx peripheral.
 *  @param  __INTERRUPT__ specifies the UART interrupt source to disable.
 *          This parameter can be any combination of the following values:
 *            @arg UART_DMA_IT_TXF:    Transmit Complete Interrupt.
 *            @arg UART_DMA_IT_TXH:    Transmit Half Complete Interrupt.
 *            @arg UART_DMA_IT_TXE:    Transmit ERROR Interrupt.
 *            @arg UART_DMA_IT_RXF:    Reception Complete Interrupt.
 *            @arg UART_DMA_IT_RXH:    Reception Half Complete Interrupt.
 *            @arg UART_DMA_IT_RXE:    Reception Error Interrupt.
 *            @arg UART_DMA_IT_RXDONE: Reception Done Interrupt.
 *  @retval None
 */
#define __HAL_UART_DISABLE_DMA_IT(__HANDLE__, __INTERRUPT__)  (((__HANDLE__)->InstanceType == UART_INSTANCETYPE_0_5) ? \
                                                               ((__HANDLE__)->Instance.Instance0_5->DMA_IE &= ~(__INTERRUPT__)) :\
                                                               ((__HANDLE__)->Instance.Instance6_7->DMA_IE &= ~(__INTERRUPT__)))

/** @brief  Enable UART.
 * @param  __HANDLE__ specifies the UART Handle.
 * @retval None
 */
#define __HAL_UART_ENABLE(__HANDLE__)               (((__HANDLE__)->InstanceType == UART_INSTANCETYPE_0_5) ? \
                                                        ((__HANDLE__)->Instance.Instance0_5->CTRL |= UART_CTRL_EN) :\
                                                        ((__HANDLE__)->Instance.Instance6_7->CTRL |= UART_CTRL_EN))

/** @brief  Disable UART.
 * @param  __HANDLE__ specifies the UART Handle.
 * @retval None
 */
#define __HAL_UART_DISABLE(__HANDLE__)              (((__HANDLE__)->InstanceType == UART_INSTANCETYPE_0_5) ? \
                                                        ((__HANDLE__)->Instance.Instance0_5->CTRL &= ~UART_CTRL_EN) :\
                                                        ((__HANDLE__)->Instance.Instance6_7->CTRL &= ~UART_CTRL_EN))

/**
 * @brief  Return the number of remaining data units in the current UART DMA transfer.
 * @param  __HANDLE__: DMA handle
 * @retval The number of remaining data units in the current UART DMA transfer.
 */
#define __HAL_DMA_GET_COUNTER(__HANDLE__)           (((__HANDLE__)->InstanceType == UART_INSTANCETYPE_0_5) ? \
                                                        ((__HANDLE__)->Instance.Instance0_5->DMA_RLEN \
                                                        - ((__HANDLE__)->Instance.Instance0_5->DMA_RADR \
                                                        - (__HANDLE__)->Instance.Instance0_5->DMA_RBADR) ) :\
                                                        ((__HANDLE__)->Instance.Instance6_7->DMA_RLEN \
                                                        - ((__HANDLE__)->Instance.Instance6_7->DMA_RADR \
                                                        - (__HANDLE__)->Instance.Instance6_7->DMA_RBADR) ))

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup UART_Exported_Functions UART Exported Functions
 * @{
 */

/** @addtogroup UART_Exported_Functions_Group1 Initialization and de-initialization functions
 * @{
 */

/* Initialization/de-initialization functions  **********************************/
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef *huart);

void              HAL_UART_MspInit(UART_HandleTypeDef *huart);
void              HAL_UART_MspDeInit(UART_HandleTypeDef *huart);

/* Callbacks Register/UnRegister functions  ***********************************/
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
HAL_StatusTypeDef HAL_UART_RegisterCallback(UART_HandleTypeDef *huart, HAL_UART_CallbackIDTypeDef CallbackID,
                                            pUART_CallbackTypeDef pCallback);
HAL_StatusTypeDef HAL_UART_UnRegisterCallback(UART_HandleTypeDef *huart, HAL_UART_CallbackIDTypeDef CallbackID);

HAL_StatusTypeDef HAL_UART_RegisterRxEventCallback(UART_HandleTypeDef *huart, pUART_RxEventCallbackTypeDef pCallback);
HAL_StatusTypeDef HAL_UART_UnRegisterRxEventCallback(UART_HandleTypeDef *huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
/**
 * @}
 */

/** @addtogroup UART_Exported_Functions_Group2 IO operation functions
 * @{
 */

/* IO operation functions *******************************************************/
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_DMAStop(UART_HandleTypeDef *huart);

HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint16_t *RxLen, uint32_t Timeout);
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);

HAL_UART_RxEventTypeTypeDef HAL_UARTEx_GetRxEventType(UART_HandleTypeDef *huart);

/* Transfer Abort functions */
HAL_StatusTypeDef HAL_UART_Abort(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_AbortTransmit(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_Abort_IT(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_AbortTransmit_IT(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_AbortReceive_IT(UART_HandleTypeDef *huart);

void              HAL_UART_IRQHandler(UART_HandleTypeDef *huart);
void              HAL_UART_DMA_IRQHandler(UART_HandleTypeDef *huart);
void              HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void              HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart);
void              HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void              HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart);
void              HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);
void              HAL_UART_AbortCpltCallback(UART_HandleTypeDef *huart);
void              HAL_UART_AbortTransmitCpltCallback(UART_HandleTypeDef *huart);
void              HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef *huart);

void              HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);

/**
 * @}
 */

/** @addtogroup UART_Exported_Functions_Group3
 * @{
 */

/* Peripheral Control functions  ************************************************/
void HAL_UART_ReceiverTimeout_Config(UART_HandleTypeDef *huart, uint32_t TimeoutValue);
HAL_StatusTypeDef HAL_UART_EnableReceiverTimeout(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_DisableReceiverTimeout(UART_HandleTypeDef *huart);

/**
 * @}
 */

/** @addtogroup UART_Exported_Functions_Group4
 * @{
 */
/* Peripheral State functions  **************************************************/
HAL_UART_StateTypeDef HAL_UART_GetState(const UART_HandleTypeDef *huart);
uint32_t              HAL_UART_GetError(const UART_HandleTypeDef *huart);
/**
 * @}
 */

/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/** @defgroup UART_Private_Types UART Private Types
 * @{
 */

/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/** @defgroup UART_Private_Variables UART Private Variables
 * @{
 */

/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup UART_Private_Constants UART Private Constants
 * @{
 */
/** @brief UART interruptions flag mask and position
  *
  */
#define UART_RXTO_BIT_POS               6U


/**
 * @}
 */

/* Private Macros -----------------------------------------------------------*/
/** @defgroup UART_Private_Macros UART Private Macros
 * @{
 */
#define IS_UART_DATAWIDTH(LENGTH)       (((LENGTH) == UART_DATAWIDTH_5B) \
                                            || ((LENGTH) == UART_DATAWIDTH_6B) \
                                            || ((LENGTH) == UART_DATAWIDTH_7B) \
                                            || ((LENGTH) == UART_DATAWIDTH_8B))

#define IS_UART_STOPBITS(STOPBITS)      (((STOPBITS) == UART_STOPBITS_1) \
                                            || ((STOPBITS) == UART_STOPBITS_2))

#define IS_UART_PARITY(PARITY)          (((PARITY) == UART_PARITY_NONE) \
                                            || ((PARITY) == UART_PARITY_ODD) \
                                            || ((PARITY) == UART_PARITY_EVEN) \
                                            || ((PARITY) == UART_PARITY_ZERO) \
                                            || ((PARITY) == UART_PARITY_ONE) \
                                            || ((PARITY) == UART_PARITY_USER_DEFINED))

#define IS_UART_BAUDRATE(BAUDRATE)      (((BAUDRATE) >= 300) && ((BAUDRATE) <= 3686400))

#define IS_UART_RXTIMEOUT(TIMEOUT)      ((TIMEOUT) <= 0xFFU)

#define IS_UART_DMA_BUFFER_SIZE(SIZE)   (((SIZE) >= 0x1U) && ((SIZE) < 0x10000U))

#define IS_UART_DMA_MODE(MODE)          (((MODE) == UART_DMA_NORMAL) \
                                            || ((MODE) == UART_DMA_CIRCULAR))

#define IS_UART_DMA_PARITY(PARITY)      (((PARITY) == UART_DMA_PARITY_ZERO) \
                                            || ((PARITY) == UART_DMA_PARITY_ONE))
/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup UART_Private_Functions UART Private Functions
 * @{
 */
HAL_StatusTypeDef UART_Start_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef UART_Start_Receive_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_HAL_UART_H */
