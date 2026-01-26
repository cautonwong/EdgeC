/**
 ******************************************************************************
 * @file   rn_rv_hal_rpc.c
 * @author Renergy Software Team
 * @brief  RPC HAL module driver.
 *          This file provides firmware functions to manage the following
 *          functionalities of the internal RPC memory:
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

/** @defgroup RPC RPC
 * @brief     RPC driver modules
 * @{
 */

#ifdef HAL_RPC_MODULE_ENABLED

/* Private typedef ----------------------------------------------------------*/
/** @defgroup RPC_Private_Typedef RPC Private Typedef
 * @{
 */

/**
 * @brief  CORE0 and CORE1 ID enumeration
 */
typedef enum __attribute__((__packed__))
{
    ICC_CORE0 = 0,
    ICC_CORE1
} ICC_CoreIDTypeDef;

/**
 * @brief  ICC core status UNKNOWN and RUNNING enumeration
 */
typedef enum __attribute__((__packed__))
{
    ICC_CORE_UNKNOWN = 0,
    ICC_CORE_RUNNING
} ICC_CoreStateTypeDef;

/**
 * @brief  ICC Link status enumeration
 */
typedef enum __attribute__((__packed__))
{
    ICC_LINK_UNKNOWN = 0,
    ICC_LINK_DOWN    = 1,      /*!< Link is down. */
    ICC_LINK_UP      = 2       /*!< Link is up.   */

} ICC_LinkStateTypeDef;

/**
 * @brief  ICC Core Information structure definition
 */
typedef struct
{
    volatile ICC_CoreStateTypeDef  RemoteCoreState;
    volatile uint32_t              ICCInfo;
    volatile ICC_LinkStateTypeDef  LinkState;              /*!< state of the ICC link, up/down, link_state = 1 means that core0 and core1 are link-up */
} ICC_CoreInfoTypeDef;

/**
 * @brief  RPC API Call Information structure definition
 */
typedef struct {
    volatile uint32_t APICallID;                  /*!< rpc service api id           */
    volatile uint32_t APICallResult;              /*!< rpc service api return value */
    /*
     * for CORE1:
     *    Send: [0]   = mailbox_msg_type(BIT31:28) | mailbox_msg_len(BIT27:24) | api_id(BIT23:0),
     *          [1::] = input params
     *    Recv: [0]   = mailbox_msg_type | mailbox_msg_len | api_id,
     *          [1]   = func return
     * for CORE0:
     *    Recv: [0]   = mailbox_msg_type(BIT31:28) | mailbox_msg_len(BIT27:24) | api_id(BIT23:0),
     *          [1::] = input params
     *    Send: [0]   = mailbox_msg_type | mailbox_msg_len | api_id,
     *          [1]   = func return
     * max params_nums is (MAX_RPC_API_DATA_LENGTH - 1);
     */
    volatile uint32_t APIParam[15];               /*!< rpc api parameters, APIParam[0] contains the id and mailbox msg data length*/
} RPC_APICallInfoTypeDef;

/**
 * @brief  Share Memory config structure definition
 */
typedef struct
{
    uint32_t *ShmemBase;     /* shmem base addre */
    uint32_t TotalSize;      /* shmem total size */
    uint32_t BufSize;        /* buf size, default is 2K*/
}ShmemConfigTypeDef;

/**
 * @brief  Share Memory buf Addres structure definition
 */
typedef struct
{
    uint32_t *TxBufAddr;
    uint32_t *RxBufAddr;
}SHMEM_BufferAddrTypeDef;

/**
 * @brief  Share Memory Information structure definition
 */
typedef struct
{
    ShmemConfigTypeDef ShmemConfig;
    uint32_t MaxPayloadSize;
    ProcessCallback DataProcessCallback;
    SHMEM_BufferAddrTypeDef BufAddress;
    volatile uint32_t Core0ShmemState;
    volatile uint32_t Core1ShmemState;
    volatile uint32_t ShmemData[5];
} ShmemInfoTypeDef;

/**
 * @brief  Share Memory status INIT and DONE enumeration
 */
typedef enum __attribute__((__packed__))
{
    SHMEM_INIT = 1,
    SHMEM_DONE = 2
} SHMEM_StateTypeDef;

/**
 * @brief  Common header for all Share Memory message
 */
struct shmem_std_hdr
{
    uint32_t len;      /*!< shmem message length */
    uint32_t reserved; /*!< reserved for future use */
};

/**
 * @brief  Share Memory Information structure definition
 */
struct shmem_std_msg
{
    struct shmem_std_hdr hdr; /*!< shmem message header */
    uint8_t data[1];          /*!< bytes of message payload data */
};

/**
 * @}
 */
/* Private define -----------------------------------------------------------*/
/** @defgroup RPC_Private_Constants RPC Private Constants
 * @{
 */
#define RPC_MAX_API_ID          (1U << 24U)        /*!< MAX API ID */
#define RPC_MAX_API_PARAM_LEN   (1 << 4U)

#define MAILBOX_MSG_TYPE_MASK   (0xF0000000U)
#define MAILBOX_MSG_LEN_MASK    (0xFU)
#define MAILBOX_API_ID_MASK     (0x00FFFFFFU)
#define MAILBOX_API_ID_POS      (0U)
#define MAILBOX_MSG_LEN_POS     (24U)
#define MAILBOX_MSG_TYPE_POS    (28U)

/* MAILBOX configuration key */
#define MAILBOX_KEY0_MAGIC         (0xA538E827U)
#define MAILBOX_KEY1_MAGIC         (0xA538EC63U)

/* Shared memory "allocator" parameters */
#define SH_MEM_BASE              (0x90010000U) /*total 80K, 0x90014000U           */
#define SH_MEM_TOTAL_SIZE        (10 * 1024U)  /* 10K 0x90010000U--0x90012800U    */
#define BUF_SZIE                 (2 * 1024U)   /* default 2K BUF, txBUF and rxBUF */
#define BUF_COUNT                (2U)          /* Number of the buffers, default is 2, transmit buf and receive buf */

/**
 * @}
 */
/* Private macro ------------------------------------------------------------*/
/** @defgroup RPC_Private_Macros RPC Private Macros
 * @{
 */

/**
 * @}
 */
/* Private variables --------------------------------------------------------*/

/* Private function prototypes ----------------------------------------------*/
/** @defgroup RPC_Private_Functions RPC Private Functions
 * @{
 */
/* Private functions for mailbox */
static HAL_StatusTypeDef MAILBOX_Init(MAILBOX_InitTypeDef *MAILBOX_Init, uint32_t Timeout);
static void MAILBOX_DeInit(void);

/* Private functions to handle flags during polling transfer */
static HAL_StatusTypeDef MAILBOX_WaitOnLinkFlagUntilTimeout(uint32_t Timeout, uint32_t Tickstart);

/* Mailbox Message operation functions  ***********************************************/
HAL_StatusTypeDef MAILBOX_TransmitMessage(const uint32_t *pData, uint16_t Size);

/* Private functions to handle API call from core1 */
static void RPC_APIProcessCallback(uint32_t FuncPtr, volatile uint32_t *Params);
HAL_StatusTypeDef RPC_APICall(uint8_t API_ID, uint32_t *Params, uint32_t Size, uint32_t Timeout);

/* Private functions to handle Share Mempry */
static void SHMEM_Notify(void);
static void SHMEM_NotifyReply(void);

/* global */
static RPC_APICallInfoTypeDef RPC_APICallInfo;
static ICC_CoreInfoTypeDef ICC_CoreInfo = {
    .RemoteCoreState = ICC_CORE_UNKNOWN,
    .ICCInfo = 0x0U,
    .LinkState = ICC_LINK_UNKNOWN
};

static ShmemInfoTypeDef ShmemInfo;
volatile bool sleep_requested = false;
extern uint32_t api_server_list[];
/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup RPC_Exported_Functions RPC Exported Functions
 * @{
 */

/** @defgroup RPC_Exported_Functions_Group1 Initialization and de-initialization functions
 *  @brief    Initialization and Configuration functions.
 *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
    [..]
        This section provides functions allowing to initialize and de-initialize the ICC
        to be ready for use.

@endverbatim
 * @{
 */

/**
 * @brief  Initializes the ICC (Inter-Core Communication),
 *         inlcude gic(ICI) mailbox.
 *
 * @param  Timeout Timeout duration.
 * @retval HAL_StatusTypeDef HAL RPC Status
 */
HAL_StatusTypeDef HAL_ICC_Init(uint32_t Timeout)
{
    /* Init tickstart for Timeout management*/
    volatile uint32_t tickstart = HAL_GetMillis();
    MAILBOX_InitTypeDef MailboxInit;

    /* Initializes ICC_CoreInfo */
    ICC_CoreInfo.RemoteCoreState = ICC_CORE_UNKNOWN;
    /* Enable/Disable mailbox interrupt */
    MailboxInit.InputFull_ITCommand = ENABLE;
    MailboxInit.OutputEmpty_ITCommand = DISABLE;
    MailboxInit.InputEmpty_ITCommand = DISABLE;
    MailboxInit.OutputFull_ITCommand = ENABLE;
    MailboxInit.Linkable_ITCommand = DISABLE;

    /* if ICC has link up, should not Initialize */
    if (ICC_CoreInfo.LinkState != ICC_LINK_UP) {
        /* Initializes gic module (include ICI) */
        HAL_GIC_ICI_Init(ECLIC_VECTOR_INTERRUPT, ECLIC_LEVEL_TRIGGER, 3, 2);
#ifdef CORE0
        while (ICC_CoreInfo.ICCInfo != HAL_CORE_STARTUP) {
           if (HAL_GIC_ICI_Transmit(1, HAL_CORE_STARTUP) != HAL_OK) {
               return HAL_ERROR;
           }
            HAL_Delay(1);
            /* Check the Timeout */
            if (Timeout != HAL_MAX_DELAY) {
                if ((Timeout == 0U) || ((HAL_GetMillis() - tickstart) > Timeout)) {
                    /* Return timeout status */
                    return HAL_TIMEOUT;
                }
            }
        }
       if (HAL_GIC_ICI_Transmit(1, HAL_CORE_HANDSHAKE) != HAL_OK) {
           return HAL_ERROR;
       }
        /* Initializes mailbox. note: core1 init mailbox must be done after core0 */
        if (MAILBOX_Init(&MailboxInit, HAL_RPC_TIMEOUT_FLAG) != HAL_OK) {
            return HAL_ERROR;
        }
#endif
#ifdef CORE1
        /* wait for CORE_STARTUP from core0 */
        while (ICC_CoreInfo.ICCInfo != HAL_CORE_STARTUP) {
            if (Timeout != HAL_MAX_DELAY) {
                if ((Timeout == 0U) || ((HAL_GetMillis() - tickstart) > Timeout)) {
                    /* Return timeout status */
                    return HAL_TIMEOUT;
                }
            }
        }
        /* aftetr receive CORE_STARTUP, core1 send CORE_STARTUP to core0 */
        if (HAL_GIC_ICI_Transmit(0, HAL_CORE_STARTUP) != HAL_OK) {
            return HAL_ERROR;
        }
        /* wait for CORE_HANDSHAKE from core0 */
        while(ICC_CoreInfo.ICCInfo != HAL_CORE_HANDSHAKE) {
            if (Timeout != HAL_MAX_DELAY) {
                if ((Timeout == 0U) || ((HAL_GetMillis() - tickstart) > Timeout)) {
                    /* Return timeout status */
                    return HAL_TIMEOUT;
                }
            }
        }
        /* Initializes mailbox. note: core1 init mailbox must be done after core0 */
        if (MAILBOX_Init(&MailboxInit, HAL_RPC_TIMEOUT_FLAG) != HAL_OK) {
            return HAL_ERROR;
        }
#endif
        ICC_CoreInfo.LinkState = ICC_LINK_UP;
    }
    /* remote core is running and link is up */
    ICC_CoreInfo.RemoteCoreState = ICC_CORE_RUNNING;

    return HAL_OK;
}

/**
 * @brief  DeInitialize the ICC.
 * @retval HAL status
 */
void HAL_ICC_DeInit(void)
{
    HAL_GIC_ICI_DeInit();
    MAILBOX_DeInit();
    /* icc state is link down*/
    ICC_CoreInfo.ICCInfo = 0x00U;
    ICC_CoreInfo.LinkState = ICC_LINK_DOWN;
}

/**
 * @brief  Initializes the Share Memory
 *
 * @param  ShmemAddr   Share memory base address.
 * @param  ShmemLength Share memory total size.
 * @param  BufSize     Default 2KBytes BUF, txBUF and rxBUF.
 * @param  Callback    User data process callback function.
 * @param  Timeout     Timeout duration.
 * @retval HAL_StatusTypeDef HAL Status
 */
#ifdef CORE0
HAL_StatusTypeDef HAL_SHMEM_Init(ProcessCallback Callback, uint32_t Timeout)
#endif
#ifdef CORE1
HAL_StatusTypeDef HAL_SHMEM_Init(uint32_t *ShmemAddr, uint32_t ShmemLength, uint32_t BufSize, ProcessCallback Callback, uint32_t Timeout)
#endif
{
    /* Init tickstart for Timeout management*/
    volatile uint32_t tickstart = HAL_GetMillis();
    uint32_t MessageSize;
    ShmemConfigTypeDef ShmemConfig;

    ShmemInfo.Core0ShmemState = SHMEM_INIT;
    ShmemInfo.Core1ShmemState = SHMEM_INIT;

#ifdef CORE0
    /* core0 waiting for core1 send to shared memory configuration info */
    while ((ShmemInfo.ShmemData[0] >> MAILBOX_MSG_TYPE_POS) != HAL_MAILBOX_SHMEM_CONFIG)
    {
        /* Check the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if ((Timeout == 0U) || ((HAL_GetMillis() - tickstart) > Timeout)) {
                /* Return timeout status */
                return HAL_TIMEOUT;
            }
        }
    }

    /* core0 initialized shared memory configurations */
    memcpy((void *)&ShmemConfig, (const void *)(ShmemInfo.ShmemData + 1), sizeof(ShmemConfig));
    ShmemInfo.ShmemConfig = ShmemConfig;
    ShmemInfo.MaxPayloadSize = ShmemConfig.BufSize - sizeof(struct shmem_std_hdr);
    ShmemInfo.DataProcessCallback = Callback;
    ShmemInfo.BufAddress.TxBufAddr = ShmemInfo.ShmemConfig.ShmemBase;
    ShmemInfo.BufAddress.RxBufAddr = (uint32_t *)((uint32_t)(ShmemInfo.ShmemConfig.ShmemBase) + ShmemInfo.ShmemConfig.BufSize);

    /* ShmemInfo.Core0ShmemState = SHMEM_DONE and send to core1 */
    memset((void *)ShmemInfo.ShmemData, 0, sizeof(ShmemInfo.ShmemData));
    ShmemInfo.Core0ShmemState = SHMEM_DONE;
    /* send Core0ShmemState to core1 */
    MessageSize = (sizeof(ShmemInfo.Core0ShmemState) / sizeof(uint32_t)) + 1;
    ShmemInfo.ShmemData[0] = (HAL_MAILBOX_SHMEM_CONFIG << MAILBOX_MSG_TYPE_POS) | (MessageSize << MAILBOX_MSG_LEN_POS);
    ShmemInfo.ShmemData[1] = ShmemInfo.Core0ShmemState;

    if (MAILBOX_TransmitMessage((const uint32_t *)ShmemInfo.ShmemData, MessageSize) != HAL_OK) {
        return HAL_ERROR;
    }

    /* then waiting for core1 send shared memory initialization state */
    while (ShmemInfo.Core1ShmemState != SHMEM_DONE)
    {
        /* Check the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if ((Timeout == 0U) || ((HAL_GetMillis() - tickstart) > Timeout)) {
                /* Return timeout status */
                return HAL_TIMEOUT;
            }
        }
    }
#endif

#ifdef CORE1
    if((ShmemAddr == NULL) || (ShmemLength > SH_MEM_TOTAL_SIZE) || (BufSize > BUF_SZIE))
    {
        return HAL_ERROR;
    }
    /* initialized shared memory configurations, then send to core0 */
    ShmemConfig.ShmemBase = ShmemAddr;
    ShmemConfig.TotalSize = ShmemLength;
    ShmemConfig.BufSize = BufSize;
    /* send Config to core0 */
    MessageSize = (sizeof(ShmemConfig) / sizeof(uint32_t)) + 1;
    ShmemInfo.ShmemData[0] = (HAL_MAILBOX_SHMEM_CONFIG << MAILBOX_MSG_TYPE_POS) | (MessageSize << MAILBOX_MSG_LEN_POS);
    memcpy((void *)(ShmemInfo.ShmemData + 1), &ShmemConfig, sizeof(ShmemConfig));

    /* send Config to core0  use mailbox */
    if (MAILBOX_TransmitMessage((const uint32_t *)ShmemInfo.ShmemData, MessageSize) != HAL_OK) {
        return HAL_ERROR;
    }

    ShmemInfo.ShmemConfig = ShmemConfig;
    ShmemInfo.MaxPayloadSize = ShmemConfig.BufSize - sizeof(struct shmem_std_hdr);
    ShmemInfo.DataProcessCallback = Callback;
    ShmemInfo.BufAddress.TxBufAddr = (uint32_t *)((uint32_t)(ShmemInfo.ShmemConfig.ShmemBase) + ShmemInfo.ShmemConfig.BufSize);
    ShmemInfo.BufAddress.RxBufAddr = ShmemInfo.ShmemConfig.ShmemBase;

    /* then waiting for core0 send shared memory initialization state back */
    while (ShmemInfo.Core0ShmemState != SHMEM_DONE) {
        /* Check the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if ((Timeout == 0U) || ((HAL_GetMillis() - tickstart) > Timeout)) {
                /* Return timeout status */
                return HAL_TIMEOUT;
            }
        }
    }

    /* send core1 shared memory initialization state back */
    memset((void *)ShmemInfo.ShmemData, 0, sizeof(ShmemInfo.ShmemData));
    ShmemInfo.Core1ShmemState = SHMEM_DONE;
    MessageSize = (sizeof(ShmemInfo.Core1ShmemState) / sizeof(uint32_t)) + 1;
    ShmemInfo.ShmemData[0] = (HAL_MAILBOX_SHMEM_CONFIG << MAILBOX_MSG_TYPE_POS) | (MessageSize << MAILBOX_MSG_LEN_POS);
    ShmemInfo.ShmemData[1] = ShmemInfo.Core1ShmemState;

    /* send Config to core0 use mailbox */
    if (MAILBOX_TransmitMessage((const uint32_t *)ShmemInfo.ShmemData, MessageSize) != HAL_OK) {
        return HAL_ERROR;
    }
#endif
    return HAL_OK;
}

/**
 * @brief  DeInitialize the SHMEM.
 *
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_SHMEM_DeInit(void)
{
    /*  */

    return HAL_OK;
}
/**
 * @}
 */

/** @defgroup RPC_Exported_Functions_Group2 Programming operation functions
 *  @brief     Programming and Erasing operation functions
 *
@verbatim
@endverbatim
 * @{
 */

/**
 * @brief  Mailbox Transmits an amount of data.
 *
 * @param  pData   Pointer to Transmit data
 * @param  Size    Number of messgae.
 * @retval None
 */
HAL_StatusTypeDef HAL_SHMEM_Transmit(const uint8_t *pData, uint32_t Size)
{
    struct shmem_std_msg *ShmemMessage;

    if ((pData == NULL) || (Size > ShmemInfo.MaxPayloadSize))
    {
        return HAL_ERROR;
    }
    ICC_CoreInfo.ICCInfo = 0x0U;

    ShmemMessage = (struct shmem_std_msg *)ShmemInfo.BufAddress.TxBufAddr;
    ShmemMessage->hdr.len = Size;
    /* Copy data to shared memory txbuffer. */
    memcpy(ShmemMessage->data, pData, Size);

    /* Let the other side know that there is a job to process. */
    SHMEM_Notify();

    /* waiting for SH_MEM_RECEIVED info from other core */
    while (ICC_CoreInfo.ICCInfo != HAL_SHMEM_RECEIVED)
    {
        ;
    }
    return HAL_OK;
}

/**
 * @}
 */

/** @defgroup RPC_Exported_Functions_Group3 IRQ Handler and Callbacks
 *  @brief     IRQ Handler and Callbacks functions
 *
@verbatim
================================================================================
            ##### IRQ Handler and Callbacks functions #####
================================================================================
    [..]  This section provides functions allowing to:

@endverbatim
 * @{
 */

/**
 * @brief  Core1 Send To Core0 interrupt callback
 * @retval none
 */
void HAL_GIC_ICI_Core1ToCore0Callback(void)
{
    if (HAL_GIC_ICI_Receive(1, (uint8_t *)&ICC_CoreInfo.ICCInfo) == HAL_OK) {
        if (ICC_CoreInfo.ICCInfo == HAL_SHMEM_TRANSMITD) {
            HAL_SHMEM_MessageReceiveCallback();
        }
        else if (ICC_CoreInfo.ICCInfo == HAL_CORE_SLEEP) {
            sleep_requested = true;
        }
        else if (ICC_CoreInfo.ICCInfo == HAL_CORE_WAKEUP) {
            /* do nothing */
        }
    }
}

/**
 * @brief  Core0 Send To Core1 interrupt callback
 * @retval none
 */
void HAL_GIC_ICI_Core0ToCore1Callback(void)
{
    if (HAL_GIC_ICI_Receive(0, (uint8_t *)&ICC_CoreInfo.ICCInfo) == HAL_OK) {
        if (ICC_CoreInfo.ICCInfo == HAL_SHMEM_TRANSMITD) {
            HAL_SHMEM_MessageReceiveCallback();
        }
    }
}

/**
 * @brief  This function handles core0 mailbox event interrupt request.
 * @note   This API should be called under the MAILBOX_INTERNAL_HANDLER().
 * @retval None
 */
void HAL_MAILBOX_CORE0_IRQHandler(void)
{
    uint32_t MessageFirst, MailboxMsgType, MessageSize;

    /* output mailbox empty interrupt in core0 */
    if (((MAILBOX->INTST >> MAILBOX_INTST_MBX_DONE_POS) & 0x1U) != RESET) {
        /* add output mailbox empty callback here */
    }

    /* input mailbox full interrupt in core0 */
    if (((MAILBOX->INTST >> MAILBOX_INTST_MBX_IFRE_POS) & 0x1U) != RESET) {
        MessageFirst = MAILBOX->MESSAGE[0];
        MailboxMsgType = MessageFirst >> MAILBOX_MSG_TYPE_POS;
        MessageSize = (MessageFirst >> MAILBOX_MSG_LEN_POS) & MAILBOX_MSG_LEN_MASK;
        /* Mailbox Message Type */
        if (MailboxMsgType == HAL_MAILBOX_API_CALL) {
            RPC_APICallInfo.APICallID = MessageFirst & MAILBOX_API_ID_MASK;
            /* read mailbox message */
            HAL_MAILBOX_MessageFullReadCallback((uint32_t *)RPC_APICallInfo.APIParam, MessageSize);
            /* core0 call rpc_callback to call function*/
            RPC_APIProcessCallback(api_server_list[RPC_APICallInfo.APICallID], RPC_APICallInfo.APIParam);
        } else if (MailboxMsgType == HAL_MAILBOX_SHMEM_CONFIG) {
            HAL_MAILBOX_MessageFullReadCallback((uint32_t *)ShmemInfo.ShmemData, MessageSize);
            if (MessageSize == 2) {
                ShmemInfo.Core1ShmemState = ShmemInfo.ShmemData[1];
            }
        }
        /* clear input mailbox full flag */
        MAILBOX->INTST = 0x1U << MAILBOX_INTST_MBX_IFRE_POS;
        /* Clear CSR.MBX_IN_FULL flag, indicate input mailbox is empty */
        MAILBOX->CSR = 0x1U << MAILBOX_CSR_MBX_IN_FULL_POS;
    }
}

/**
 * @brief  This function handles core1 mailbox event interrupt request.
 * @note   This API should be called under the MAILBOX_HOST_HANDLER().
 * @retval None
 */
void HAL_MAILBOX_CORE1_IRQHandler(void)
{
    uint32_t MessageFirst, MailboxMsgType, MessageSize;

    /* mailbox Linkable interrupt in core1 */
    if (((MAILBOX->INTST >> MAILBOX_INTST_MBX_LINKABLE_POS) & 0x1U) != RESET) {
        /* add mailbox Linkable callback here */
    }

    /* output mailbox full interrupt in core1 */
    if (((MAILBOX->INTST >> MAILBOX_INTST_MBX_DONE_POS) & 0x1U) != RESET) {
        /* get mailbox message type, api ID, message size */
        MessageFirst = MAILBOX->MESSAGE[0];
        MailboxMsgType = MessageFirst >> MAILBOX_MSG_TYPE_POS;
        MessageSize = (MessageFirst >> MAILBOX_MSG_LEN_POS) & MAILBOX_MSG_LEN_MASK;
        /* Process Mailbox Message Type */
        if (MailboxMsgType == HAL_MAILBOX_API_CALL) {
            RPC_APICallInfo.APICallID = MessageFirst & MAILBOX_API_ID_MASK;
            HAL_MAILBOX_MessageFullReadCallback((uint32_t *)RPC_APICallInfo.APIParam, MessageSize);
        } else if (MailboxMsgType == HAL_MAILBOX_SHMEM_CONFIG) {
            HAL_MAILBOX_MessageFullReadCallback((uint32_t *)ShmemInfo.ShmemData, MessageSize);
            /* for three-way handshake */
            if (MessageSize == 2) {
                ShmemInfo.Core0ShmemState = ShmemInfo.ShmemData[1];
            }
        }
        /* clear output mailbox full flag */
        MAILBOX->INTST = 0x1U << MAILBOX_INTST_MBX_DONE_POS;
        /* Clear CSR.MBX_OUT_FULL flag, indicate output mailbox is empty */
        MAILBOX->CSR = 0x1U << MAILBOX_CSR_MBX_OUT_FULL_POS;
    }

    /* input mailbox empty interrupt in core1 */
    if (((MAILBOX->INTST >> MAILBOX_INTST_MBX_IFRE_POS) & 0x1U) != RESET) {
        /* add input mailbox empty callback here */
    }
}

/**
 * @brief  This function handles core1 mailbox event interrupt request.
 *
 * @param  pData  Pointer to receive data
 * @param  Size   Number of messgae.
 * @retval None
 */
void HAL_MAILBOX_MessageFullReadCallback(uint32_t *pData, uint16_t Size)
{
    uint32_t i;

    for (i = 0U; i < Size; i++) {
        pData[i] = MAILBOX->MESSAGE[i];
    }
}

void HAL_SHMEM_MessageReceiveCallback(void)
{
    struct shmem_std_msg *ShmemMessage;

    /* Process the received data from remote node */
    ShmemMessage = (struct shmem_std_msg *)ShmemInfo.BufAddress.RxBufAddr;
    if (ShmemInfo.DataProcessCallback != NULL)
    {
        ShmemInfo.DataProcessCallback((uint32_t *)ShmemMessage->data, ShmemMessage->hdr.len, ShmemInfo.MaxPayloadSize);
    }
    /* reply remote side has processed the data */
    SHMEM_NotifyReply();
}

/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup RPC_Private_Function
 * @{
 */
/**
 * @brief  ICI message and interrupt send.
 *
 * @param  MAILBOX_Init: Pointer to a MAILBOX_InitTypeDef structure that contains
 * @param  Timeout: Timeout duration.
 *
 * @retval HAL_StatusTypeDef HAL RPC Status
 */
HAL_StatusTypeDef MAILBOX_Init(MAILBOX_InitTypeDef *MAILBOX_Init, uint32_t Timeout)
{
    /* Get timeout */
    uint32_t tickstart = HAL_GetMillis();

    if (MAILBOX_Init == NULL) {
        return HAL_ERROR;
    }

    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_MAILBOX_CLK_ENABLE();
    __HAL_SYSTEM_GIC_CLK_ENABLE();
    __HAL_SYSTEM_PSW_LOCK();
#ifdef CORE0
    MAILBOX->MBX_KEY0 = MAILBOX_KEY0_MAGIC;
    MAILBOX->RESET = (0x1U << MAILBOX_RESET_MBX_OUT_EMPTY_POS) | (0x1U << MAILBOX_RESET_MBX_UNLINK_POS);
    /* Froze core1 as master for mailbox */
    MAILBOX->FROZE = 0x1U;
    MAILBOX->ID_VAL = 0x0U;
    MAILBOX->ID_MASK = 0x0U;   /* no need to mask master id, only one master for one mailbox */
    /* clear flag */
    MAILBOX->INTST = 0x3U;
    /* enable interrupt */
    MAILBOX->INTEN = (MAILBOX_Init->InputFull_ITCommand << MAILBOX_INTEN_MBX_IFRE_POS) |\
                        (MAILBOX_Init->OutputEmpty_ITCommand << MAILBOX_INTEN_MBX_DONE_POS);
    /* wait core1 mailbox link up */
    if (MAILBOX_WaitOnLinkFlagUntilTimeout(Timeout, tickstart) != HAL_OK) {
        return HAL_TIMEOUT;
    }
    /* Clear CSR.MBX_IN_FULL flag, indicate input mailbox is empty */
    MAILBOX->CSR = 0x1U << MAILBOX_CSR_MBX_IN_FULL_POS;

    /* Assign interrupt use GIC */
    if (MAILBOX_Init->InputFull_ITCommand | MAILBOX_Init->OutputEmpty_ITCommand) {
        /* Assign MAILBOX Irq to core0 and core1 */
        HAL_GIC_SetIrqTargetCore(MAILBOXINTER_IRQn, HAL_GIC_IRQ_TARGET_CORE0);
        HAL_GIC_SetIrqTargetCore(MAILBOXHOST0_IRQn, HAL_GIC_IRQ_TARGET_CORE1);
        /* Initialize ECLIC for interrupt */
        ECLIC_Register_IRQ(MAILBOXINTER_IRQn, ECLIC_VECTOR_INTERRUPT, ECLIC_LEVEL_TRIGGER, 7, 1, NULL);
        /* enable global interrupt */
        __enable_irq();
    }
#endif
#ifdef CORE1
    MAILBOX->MBX_KEY1 = MAILBOX_KEY1_MAGIC;
    /* link up core0 mailbox */
    MAILBOX->CSR = (0x1 << MAILBOX_CSR_MBX_LINK_POS);
    /* clear flag */
    MAILBOX->INTST = 0x10003U;
    /* enable interrupt */
    MAILBOX->INTEN = (MAILBOX_Init->InputEmpty_ITCommand << MAILBOX_INTEN_MBX_IFRE_POS) |\
                        (MAILBOX_Init->OutputFull_ITCommand << MAILBOX_INTEN_MBX_DONE_POS) |\
                            (MAILBOX_Init->Linkable_ITCommand << MAILBOX_INTEN_MBX_LINKABLE_POS);
    /* check mailbox is link up */
    if(MAILBOX_WaitOnLinkFlagUntilTimeout(Timeout, tickstart) != HAL_OK) {
        return HAL_TIMEOUT;
    }
    /* Clear CSR.MBX_OUT_FULL flag, indicate output mailbox is empty */
    MAILBOX->CSR = 0x1U << MAILBOX_CSR_MBX_OUT_FULL_POS;

    if (MAILBOX_Init->InputEmpty_ITCommand | MAILBOX_Init->OutputFull_ITCommand | MAILBOX_Init->Linkable_ITCommand) {
        ECLIC_Register_IRQ(MAILBOXHOST0_IRQn, ECLIC_VECTOR_INTERRUPT, ECLIC_LEVEL_TRIGGER, 7, 1, NULL);
        __enable_irq();
    }
#endif

    return HAL_OK;
}

/**
 * @brief  DeInitialize the MAILBOX
 *
 * @retval None
 */
static void MAILBOX_DeInit(void)
{
#ifdef CORE0
    MAILBOX->MBX_KEY0 = MAILBOX_KEY0_MAGIC;
    /* clear flag */
    MAILBOX->INTST = 0x3U;
    MAILBOX->RESET = (0x1U << MAILBOX_RESET_MBX_OUT_EMPTY_POS) | (0x1U << MAILBOX_RESET_MBX_UNLINK_POS);
    MAILBOX->CSR = 0x01U;
    /* DeInit the low level hardware: GPIO, CLOCK, ECLIC */
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_MAILBOX_CLK_DISABLE();
    __HAL_SYSTEM_PSW_LOCK();
    __ECLIC_DisableIRQ(MAILBOXINTER_IRQn);
#endif
#ifdef CORE1
    MAILBOX->MBX_KEY1 = MAILBOX_KEY1_MAGIC;
    /* clear flag */
    MAILBOX->INTST = 0x10003U;
    MAILBOX->CSR = 0x02U;
    /* DeInit the low level hardware: GPIO, CLOCK, ECLIC */
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_MAILBOX_CLK_DISABLE();
    __HAL_SYSTEM_PSW_LOCK();
    __ECLIC_DisableIRQ(MAILBOXHOST0_IRQn);
#endif
}

/**
 * @brief  This function handles MAILBOX link Timeout for specific usage of LINK mailbox flag.
 * @param  Timeout Timeout duration
 * @param  Tickstart Tick start value
 * @retval HAL status
 */
static HAL_StatusTypeDef MAILBOX_WaitOnLinkFlagUntilTimeout(uint32_t Timeout, uint32_t Tickstart)
{
    while (((MAILBOX->CSR >> MAILBOX_CSR_MBX_LINK_POS) & 0x1U) == RESET) {
        /* Check for the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if (((HAL_GetMillis() - Tickstart) > Timeout) || (Timeout == 0U)) {
                if (((MAILBOX->CSR >> MAILBOX_CSR_MBX_LINK_POS) & 0x1U) == RESET) {
                    return HAL_TIMEOUT;
                }
            }
        }
    }
    return HAL_OK;
}

/**
 * @brief  RPC API remote call function
 *
 * @param  API_ID  remote call function ID.
 * @param  Params  Pointer to Params.
 * @param  Size    Number of Params.
 * @param  Timeout Timeout duration.
 * @retval None
 */
HAL_StatusTypeDef RPC_APICall(uint8_t API_ID, uint32_t *Params, uint32_t Size, uint32_t Timeout)
{
    /* Init tickstart for Timeout management*/
    uint64_t tickstart = HAL_GetTicks();
    uint32_t MessageSize;
    uint32_t i;

    RPC_APICallInfo.APICallID = RPC_MAX_API_ID;
    RPC_APICallInfo.APICallResult = RPC_MAX_API_PARAM_LEN;
    /* mailbox msg len = api_parms_length + 1(rpc_api_data[0] contains the id and mailbox msg data length)*/
    MessageSize = Size + 1;
    /* msg_type(BIT31:28) | msg_len(BIT27:24) | api_id(BIT23:0) */
    RPC_APICallInfo.APIParam[0] = (HAL_MAILBOX_API_CALL << MAILBOX_MSG_TYPE_POS) \
                                | (MessageSize << MAILBOX_MSG_LEN_POS)| (API_ID << MAILBOX_API_ID_POS);
    /* APIParam[0] include type, length, api ID */
    for (i = 0U; i < Size; i++) {
        RPC_APICallInfo.APIParam[1 + i] = Params[1 + i];
    }
    /*Transmit Message to core0 include api ID and api Params */
    if (MAILBOX_TransmitMessage((const uint32_t *)RPC_APICallInfo.APIParam, MessageSize) != HAL_OK) {
        return HAL_ERROR;
    }

    /* wait for core0 process api call and return result */
    while (RPC_APICallInfo.APICallID != API_ID) {
        /* Check the Timeout */
        if (Timeout != HAL_MAX_DELAY) {
            if ((Timeout == 0U) || ((((HAL_GetTicks() - tickstart) *1000 / __LL_SYSTEM_CORE1_CALC_CORE1_CLK_FREQ(LL_SYSTEM_CORE1_GetCore1SystemClockFreq()))) > Timeout)) {
                /* Return timeout status */
                return HAL_TIMEOUT;
            }
        }
    }

    if (RPC_APICallInfo.APICallID == API_ID) {
        RPC_APICallInfo.APICallResult = RPC_APICallInfo.APIParam[1];
        Params[1] = RPC_APICallInfo.APICallResult;
    } else {
        return HAL_ERROR;
    }
    return HAL_OK;
}

/**
 * @brief  RPC API Process Callback function
 *
 * @retval None
 */
static void RPC_APIProcessCallback(uint32_t FuncPtr, volatile uint32_t *Params)
{
    __ASM volatile (
        "addi sp, sp, -8\n"
        "sw   ra, 0(sp)\n"
        "sw   a1, 4(sp)\n"

        "mv t1, a0\n"       //# backup a0 & a1
        "mv t2, a1\n"
        "lw t0, 4(a1)\n"
        "mv a0, t0\n"
        "lw t0, 12(a1)\n"
        "mv a2, t0\n"
        "lw t0, 16(a1)\n"
        "mv a3, t0\n"
        "lw t0, 20(a1)\n"
        "mv a4, t0\n"
        "lw t0, 24(a1)\n"
        "mv a5, t0\n"
        "lw t0, 28(a1)\n"
        "mv a6, t0\n"
        "lw t0, 32(a1)\n"
        "mv a7, t0\n"
        "lw t0, 8(a1)\n"
        "mv a1, t0\n"

        "jalr t1\n"

        "lw   t0, 4(sp)\n"
        "sw   a0, 4(t0)\n"

        "lw   ra, 0(sp)\n"
        "addi sp, sp, 8\n"

        :
        :"r"(FuncPtr), "r"(Params)
        : "memory", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
            "t0", "t1", "t2"
    );

    /* api_ret is rpc_info.rpc_api_data[1], then echo the return value to core1, include the original data[0] */
    RPC_APICallInfo.APICallResult = RPC_APICallInfo.APIParam[1];
    RPC_APICallInfo.APIParam[0] = (HAL_MAILBOX_API_CALL << MAILBOX_MSG_TYPE_POS) \
                                | (0x2U << MAILBOX_MSG_LEN_POS)| (RPC_APICallInfo.APICallID << MAILBOX_API_ID_POS);
    /* echo result and api ID to core1 */
    MAILBOX_TransmitMessage((const uint32_t *)RPC_APICallInfo.APIParam, 0x2U);
}

/**
 * @brief  Mailbox Transmits an amount of data.
 *
 * @param  pData   Pointer to Transmit data
 * @param  Size    Number of messgae.
 * @retval None
 */
HAL_StatusTypeDef MAILBOX_TransmitMessage(const uint32_t *pData, uint16_t Size)
{
    uint32_t i;

    if (pData == NULL) {
        return HAL_ERROR;
    }

#ifdef CORE0
    /* mailbox link up and output mailbox is empty */
    if ((((MAILBOX->CSR & MAILBOX_CSR_MBX_LINK) >> MAILBOX_CSR_MBX_LINK_POS) != RESET)  &&
        (((MAILBOX->INTST & MAILBOX_INTST_MBX_DONE) >> MAILBOX_INTST_MBX_DONE_POS) != RESET)) {
        for (i = 0U; i < Size; i++) {
            MAILBOX->MESSAGE[i] = pData[i];
        }
        /* write 1 to set output mailbox is full */
        MAILBOX->CSR = 0x1U << MAILBOX_CSR_MBX_OUT_FULL_POS;
        /* clear output mailbox empty flag */
        MAILBOX->INTST = 0x1U << MAILBOX_INTST_MBX_DONE_POS;
    } else {
        return HAL_ERROR;
    }
#endif

#ifdef CORE1
    /* mailbox link up and output mailbox is empty */
    if ((((MAILBOX->CSR & MAILBOX_CSR_MBX_LINK) >> MAILBOX_CSR_MBX_LINK_POS) != RESET) &&
        (((MAILBOX->INTST & MAILBOX_INTST_MBX_IFRE) >> MAILBOX_INTST_MBX_IFRE_POS) != RESET)) {
        for (i = 0; i < Size; i++) {
            MAILBOX->MESSAGE[i] = pData[i];
        }
        /* write 1 to set input mailbox is full */
        MAILBOX->CSR = 0x1U << MAILBOX_CSR_MBX_IN_FULL_POS;
        /* clear input mailbox empty flag */
        MAILBOX->INTST = 0x1U << MAILBOX_INTST_MBX_IFRE_POS;
    } else {
        return HAL_ERROR;
    }
#endif
    return HAL_OK;
}

/**
 * @brief  Notify the other side know that there is a job to process.
 *
 * @retval None
 */
static void SHMEM_Notify(void)
{
#ifdef CORE0
    HAL_GIC_ICI_Transmit(1, HAL_SHMEM_TRANSMITD);
#endif
#ifdef CORE1
    HAL_GIC_ICI_Transmit(0, HAL_SHMEM_TRANSMITD);
#endif
}

/**
 * @brief  Notify reply the other side know that there is a job to process.
 *
 * @retval None
 */
static void SHMEM_NotifyReply(void)
{
#ifdef CORE0
    HAL_GIC_ICI_Transmit(1, HAL_SHMEM_RECEIVED);
#endif
#ifdef CORE1
    HAL_GIC_ICI_Transmit(0, HAL_SHMEM_RECEIVED);
#endif
}
/**
 * @}
 */

#endif /* HAL_RPC_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
