/**
 ******************************************************************************
 * @file   rn_rv_hal_dma.h
 * @author Renergy Software Team
 * @brief  Header file of DMA HAL module.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

#ifndef __RN_RV_HAL_DMA_H
#define __RN_RV_HAL_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal_def.h"
#include "rn_rv_ll_dma.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup DMA
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup DMA_Exported_Types DMA Exported Types
 * @{
 */

/**
 * @brief DMA Init Configutation definition
 */
typedef struct
{
    uint32_t Direction;              /*!< Specifies if the data will be transferred from memory to peripheral,
                                        from memory to memory or from peripheral to memory.
                                        This parameter can be a value of @ref DMA_Data_transfer_direction */

    uint32_t PeriphDataAlignment;    /*!< Specifies the Peripheral data size alignment or Source data size alignment (byte, half word, word)
                                        in case of memory to memory transfer direction.
                                        This parameter can be a value of @ref DMA_Peripheral_data_size */

    uint32_t MemDataAlignment;       /*!< Specifies the Memory data size alignment or Destination data size alignment (byte, half word, word)
                                        in case of memory to memory transfer direction.
                                        This parameter can be a value of @ref DMA_Memory_data_size */

    uint32_t Mode;                   /*!< Specifies the normal or circular operation mode.
                                        This parameter can be a value of @ref DMA_mode */

    uint32_t Priority;               /*!< Specifies the channel priority level.
                                        This parameter can be a value of @ref DMA_Priority_level */

} DMA_InitTypeDef;

/**
 * @brief  DMA State structures definition
 */
typedef enum __attribute__((__packed__))
{
    HAL_DMA_STATE_RESET             = 0x00U,  /*!< DMA not yet initialized or disabled. */
    HAL_DMA_STATE_READY             = 0x01U,  /*!< DMA initialized and ready for use. */
    HAL_DMA_STATE_BUSY              = 0x02U,  /*!< DMA process is ongoing. */
    HAL_DMA_STATE_TIMEOUT           = 0x03U   /*!< DMA timeout state. */
} HAL_DMA_StateTypeDef;

/**
 * @brief  DMA Error Code structure definition
 */
typedef enum __attribute__((__packed__))
{
    HAL_DMA_FULL_TRANSFER           = 0x00U,  /*!< Full transfer. */
    HAL_DMA_HALF_TRANSFER           = 0x01U   /*!< Half Transfer. */
} HAL_DMA_LevelCompleteTypeDef;

/**
 * @brief  DMA Callback ID structure definition
 */
typedef enum __attribute__((__packed__))
{
    HAL_DMA_XFER_CPLT_CB_ID         = 0x00U,  /*!< Full transfer. */
    HAL_DMA_XFER_HALFCPLT_CB_ID     = 0x01U,  /*!< Half transfer. */
    HAL_DMA_XFER_ERROR_CB_ID        = 0x02U,  /*!< Error. */
    HAL_DMA_XFER_ABORT_CB_ID        = 0x03U,  /*!< Abort. */
    HAL_DMA_XFER_ALL_CB_ID          = 0x04U   /*!< All. */
} HAL_DMA_CallbackIDTypeDef;

/**
 * @brief  DMA Channel type structure definition
 */
typedef enum __attribute__((__packed__))
{
    HAL_DMA_CHANNEL_QSPI0_RX        = 0x00U,  /*!< DMA0 Channel 0. */
    HAL_DMA_CHANNEL_USART0_RX       = 0x01U,  /*!< DMA0 Channel 1. */
    HAL_DMA_CHANNEL_QSPI0_TX        = 0x02U,  /*!< DMA0 Channel 2. */
    HAL_DMA_CHANNEL_USART0_TX       = 0x03U,  /*!< DMA0 Channel 3. */
    HAL_DMA_CHANNEL_QSPI1_RX        = 0x04U,  /*!< DMA1 Channel 0. */
    HAL_DMA_CHANNEL_USART1_RX       = 0x05U,  /*!< DMA1 Channel 1. */
    HAL_DMA_CHANNEL_QSPI1_TX        = 0x06U,  /*!< DMA1 Channel 2. */
    HAL_DMA_CHANNEL_USART1_TX       = 0x07U,  /*!< DMA1 Channel 3. */
    HAL_DMA_CHANNEL_CRC             = 0x08U,  /*!< DMA1 Channel 4. */
    HAL_DMA_CHANNEL_SRAM            = 0x09U,  /*!< DMA1 Channel 5. */
} DMA_ChannelType_TypeDef;

/**
 * @brief  DMA Channel structure definition
 */
typedef struct
{
    DMA_ChannelType_TypeDef Type;
    uint32_t                Index;
} DMA_ChannelTypeDef;

/**
 * @brief  DMA handle Structure definition
 */
typedef struct __DMA_HandleTypeDef
{
    DMA_TypeDef              *Instance;                                                   /*!< DMA Channel Base Address. */

    DMA_InitTypeDef           Init;                                                       /*!< DMA communication parameters. */

    HAL_LockTypeDef           Lock;                                                       /*!< DMA locking object. */

    __IO HAL_DMA_StateTypeDef State;                                                      /*!< DMA transfer state. */

    void                     (*XferCpltCallback)(struct __DMA_HandleTypeDef *hdma);       /*!< DMA transfer complete callback. */

    void                     (*XferHalfCpltCallback)(struct __DMA_HandleTypeDef *hdma);   /*!< DMA Half transfer complete callback. */

    void                     (*XferErrorCallback)(struct __DMA_HandleTypeDef *hdma);      /*!< DMA transfer error callback. */

    void                     (*XferAbortCallback)(struct __DMA_HandleTypeDef *hdma);      /*!< DMA transfer abort callback. */

    __IO uint32_t             ErrorCode;                                                  /*!< DMA Error code. */

    DMA_ChannelTypeDef        Channel;                                                    /*!< DMA Channel Type. */

} DMA_HandleTypeDef;

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup DMA_Exported_Constants DMA Exported Constants
 * @{
 */

/** @defgroup DMA_Channer_Index Channel Index definition
 * @{
 */
#define DMA_CHANNEL_0                LL_DMA_CHANNEL_0  /*!< DMA Channel 0 in DMA<x>0 or DMA<x>1. */
#define DMA_CHANNEL_1                LL_DMA_CHANNEL_1  /*!< DMA Channel 1 in DMA<x>0 or DMA<x>1. */
#define DMA_CHANNEL_2                LL_DMA_CHANNEL_2  /*!< DMA Channel 2 in DMA<x>0 or DMA<x>1. */
#define DMA_CHANNEL_3                LL_DMA_CHANNEL_3  /*!< DMA Channel 3 in DMA<x>0 or DMA<x>1. */
#define DMA_CHANNEL_4                LL_DMA_CHANNEL_4  /*!< DMA Channel 4, only valid in DMA<x>0. */
#define DMA_CHANNEL_5                LL_DMA_CHANNEL_5  /*!< DMA M2M Channel 0, only valid in DMA<x>1. */
#define DMA_CHANNEL_6                LL_DMA_CHANNEL_6  /*!< DMA M2M Channel 1, only valid in DMA<x>1. */
/**
 * @}
 */

/** @defgroup DMA_Error_Code DMA Error Code
 * @{
 */
#define HAL_DMA_ERROR_NONE           0x00000000U    /*!< No error. */
#define HAL_DMA_ERROR_TE             0x00000001U    /*!< Transfer error. */
#define HAL_DMA_ERROR_NO_XFER        0x00000002U    /*!< no ongoing transfer. */
#define HAL_DMA_ERROR_TIMEOUT        0x00000003U    /*!< Timeout error. */
#define HAL_DMA_ERROR_NOT_SUPPORTED  0x00000004U    /*!< Not supported mode. */
/**
 * @}
 */

/** @defgroup DMA_Data_transfer_direction DMA Data transfer direction
 * @{
 */
#define DMA_PERIPH_TO_MEMORY         LL_DMA_DIRECTION_PERIPH2MEMORY  /*!< Peripheral to memory direction. */
#define DMA_MEMORY_TO_PERIPH         LL_DMA_DIRECTION_MEMORY2PERIPH  /*!< Memory to peripheral direction. */
#define DMA_MEMORY_TO_MEMORY         0x00000002                      /*!< Memory to memory direction. */
/**
 * @}
 */

/** @defgroup DMA_Peripheral_data_size DMA Peripheral data size
 * @{
 */
#define DMA_PDATAALIGN_BYTE          LL_DMA_DATAALIGN_BYTE      /*!< Peripheral data alignment: Byte. */
#define DMA_PDATAALIGN_HALFWORD      LL_DMA_DATAALIGN_HALFWORD  /*!< Peripheral data alignment: HalfWord. */
#define DMA_PDATAALIGN_WORD          LL_DMA_DATAALIGN_WORD      /*!< Peripheral data alignment: Word. */
/**
 * @}
 */

/** @defgroup DMA_Memory_data_size DMA Memory data size
 * @{
 */
#define DMA_MDATAALIGN_BYTE          LL_DMA_DATAALIGN_BYTE      /*!< Memory data alignment: Byte. */
#define DMA_MDATAALIGN_HALFWORD      LL_DMA_DATAALIGN_HALFWORD  /*!< Memory data alignment: HalfWord. */
#define DMA_MDATAALIGN_WORD          LL_DMA_DATAALIGN_WORD      /*!< Memory data alignment: Word. */
/**
 * @}
 */

/** @defgroup DMA_mode DMA mode
 * @{
 */
#define DMA_NORMAL                   LL_DMA_MODE_NORMAL         /*!< Normal mode. */
#define DMA_CIRCULAR                 LL_DMA_MODE_CIRCULAR       /*!< Circular mode. */
/**
 * @}
 */

/** @defgroup DMA_Priority_level DMA Priority level
 * @{
 */
#define DMA_PRIORITY_LOW             LL_DMA_PRIORITY_LOW        /*!< Priority level : Low. */
#define DMA_PRIORITY_MEDIUM          LL_DMA_PRIORITY_MEDIUM     /*!< Priority level : Medium. */
#define DMA_PRIORITY_HIGH            LL_DMA_PRIORITY_HIGH       /*!< Priority level : High. */
#define DMA_PRIORITY_VERY_HIGH       LL_DMA_PRIORITY_VERYHIGH   /*!< Priority level : Very_High. */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macros ----------------------------------------------------------*/
/** @defgroup DMA_Exported_Macros DMA Exported Macros
 * @{
 */

/**
 * @brief  Return the current DMA Channel half transfer complete flag.
 * @param  __HANDLE__: DMA handle
 * @retval The specified half transfer complete flag index.
 */
#define __HAL_DMA_GET_HT_FLAG_INDEX(__HANDLE__) \
    (((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_0)) ? DMA_IF_HTIF0  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_1)) ? DMA_IF_HTIF1  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_2)) ? DMA_IF_HTIF2  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_3)) ? DMA_IF_HTIF3  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_4)) ? DMA_IF_HTIF4  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_5)) ? DMA_MIF_HTIF0 : \
     DMA_MIF_HTIF1)

/**
 * @brief  Returns the current DMA Channel transfer complete flag.
 * @param  __HANDLE__: DMA handle
 * @retval The specified transfer complete flag index.
 */
#define __HAL_DMA_GET_TC_FLAG_INDEX(__HANDLE__) \
    (((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_0)) ? DMA_IF_TCIF0  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_1)) ? DMA_IF_TCIF1  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_2)) ? DMA_IF_TCIF2  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_3)) ? DMA_IF_TCIF3  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_4)) ? DMA_IF_TCIF4  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_5)) ? DMA_MIF_TCIF0 : \
     DMA_MIF_TCIF1)

/**
 * @brief  Return the current DMA Channel transfer error flag.
 * @param  __HANDLE__: DMA handle
 * @retval The specified transfer error flag index.
 */
#define __HAL_DMA_GET_TE_FLAG_INDEX(__HANDLE__) \
    (((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_0)) ? DMA_IF_TEIF0  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_1)) ? DMA_IF_TEIF1  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_2)) ? DMA_IF_TEIF2  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_3)) ? DMA_IF_TEIF3  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_4)) ? DMA_IF_TEIF4  : \
     ((uint32_t)((__HANDLE__)->Instance) == ((uint32_t)DMA_CHANNEL_5)) ? DMA_MIF_TEIF0 : \
     DMA_MIF_TEIF1)

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup DMA_Exported_Functions DMA Exported Functions
 * @{
 */

/** @addtogroup DMA_Exported_Functions_Group1
 * @{
 */
/* Initialization and Configuration functions ********************************/
HAL_StatusTypeDef    HAL_DMA_Init(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef    HAL_DMA_DeInit(DMA_HandleTypeDef *hdma);
void                 HAL_DMA_MspInit(DMA_HandleTypeDef *hdma);
void                 HAL_DMA_MspDeInit(DMA_HandleTypeDef *hdma);
/**
 * @}
 */

/** @addtogroup DMA_Exported_Functions_Group2
 * @{
 */
HAL_StatusTypeDef    HAL_DMA_StartChannel(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);
HAL_StatusTypeDef    HAL_DMA_StartChannel_IT(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);
HAL_StatusTypeDef    HAL_DMA_Abort(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef    HAL_DMA_Abort_IT(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef    HAL_DMA_PollForTransfer(DMA_HandleTypeDef *hdma, uint32_t CompleteLevel, uint32_t Timeout);
/* Interrupt functions *******************************************************/
void                 HAL_DMA_IRQHandler(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef    HAL_DMA_RegisterCallback(DMA_HandleTypeDef *hdma, HAL_DMA_CallbackIDTypeDef CallbackID, void (* pCallback)( DMA_HandleTypeDef * _hdma));
HAL_StatusTypeDef    HAL_DMA_UnRegisterCallback(DMA_HandleTypeDef *hdma, HAL_DMA_CallbackIDTypeDef CallbackID);
/**
 * @}
 */

/** @addtogroup DMA_Exported_Functions_Group3
 * @{
 */
/* State functions ***********************************************************/
HAL_DMA_StateTypeDef HAL_DMA_GetState(DMA_HandleTypeDef *hdma);
uint32_t             HAL_DMA_GetError(DMA_HandleTypeDef *hdma);
/**
 * @}
 */

/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/** @defgroup DMA_Private_Types DMA Private Types
 * @{
 */

/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/** @defgroup DMA_Private_Variables DMA Private Variables
 * @{
 */

/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup DMA_Private_Constants DMA Private Constants
 * @{
 */

/**
 * @}
 */

/* Private Macros -----------------------------------------------------------*/
/** @defgroup DMA_Private_Macros DMA Private Macros
 * @{
 */

#define IS_DMA_DIRECTION(DIRECTION)       (((DIRECTION) == DMA_PERIPH_TO_MEMORY) || \
                                           ((DIRECTION) == DMA_MEMORY_TO_PERIPH))

#define IS_DMA_BUFFER_SIZE(SIZE)          ((SIZE) <= 0xFFFFFU)

#define IS_DMA_PERIPHERAL_DATA_SIZE(SIZE) (((SIZE) == DMA_PDATAALIGN_BYTE) || \
                                           ((SIZE) == DMA_PDATAALIGN_HALFWORD) || \
                                           ((SIZE) == DMA_PDATAALIGN_WORD))

#define IS_DMA_MEMORY_DATA_SIZE(SIZE)     (((SIZE) == DMA_MDATAALIGN_BYTE) || \
                                           ((SIZE) == DMA_MDATAALIGN_HALFWORD) || \
                                           ((SIZE) == DMA_MDATAALIGN_WORD ))

#define IS_DMA_MODE(MODE)                 (((MODE) == DMA_NORMAL) || \
                                           ((MODE) == DMA_CIRCULAR))

#define IS_DMA_PRIORITY(PRIORITY)         (((PRIORITY) == DMA_PRIORITY_LOW ) || \
                                           ((PRIORITY) == DMA_PRIORITY_MEDIUM) || \
                                           ((PRIORITY) == DMA_PRIORITY_HIGH) || \
                                           ((PRIORITY) == DMA_PRIORITY_VERY_HIGH))

#define IS_DMA_CHANNEL_DIR_VALID(CHANNEL, DIRECTION) \
                                          (((((CHANNEL) == DMA_CHANNEL_0) || \
                                             ((CHANNEL) == DMA_CHANNEL_1) || \
                                             ((CHANNEL) == DMA_CHANNEL_4) || \
                                             ((CHANNEL) == DMA_CHANNEL_5)) && \
                                             ((DIRECTION) == DMA_PERIPH_TO_MEMORY)) || \
                                           ((((CHANNEL) == DMA_CHANNEL_2) || \
                                             ((CHANNEL) == DMA_CHANNEL_3)) && \
                                             ((DIRECTION) == DMA_MEMORY_TO_PERIPH)) || \
                                            (((CHANNEL) == DMA_CHANNEL_6) && \
                                             ((DIRECTION) == DMA_MEMORY_TO_MEMORY)))

#define QSPI0_RX_DATA_ADDR                      (0xB0088048U)
#define QSPI0_TX_DATA_ADDR                      (0xB008804CU)

#define QSPI1_RX_DATA_ADDR                      (0xB008C048U)
#define QSPI1_TX_DATA_ADDR                      (0xB008C04CU)

#define USART0_RX_DATA_CORE0_ADDR               (0x20024004U)
#define USART0_RX_DATA_CORE1_ADDR               (0xA0024004U)
#define USART0_TX_DATA_CORE0_ADDR               (0x20024000U)
#define USART0_TX_DATA_CORE1_ADDR               (0xA0024000U)

#define USART1_RX_DATA_CORE0_ADDR               (0x20028004U)
#define USART1_RX_DATA_CORE1_ADDR               (0xA0028004U)
#define USART1_TX_DATA_CORE0_ADDR               (0x20028000U)
#define USART1_TX_DATA_CORE1_ADDR               (0xA0028000U)

#define CRC0_DATA_CORE0_ADDR                    (0x20070000U)
#define CRC0_DATA_CORE1_ADDR                    (0xA0070000U)

#define IS_QSPI0_RX_PERIPH_BASE(__INSTANCE__)   ((__INSTANCE__) == QSPI0_RX_DATA_ADDR)
#define IS_QSPI0_TX_PERIPH_BASE(__INSTANCE__)   ((__INSTANCE__) == QSPI0_TX_DATA_ADDR)

#define IS_QSPI1_RX_PERIPH_BASE(__INSTANCE__)   ((__INSTANCE__) == QSPI1_RX_DATA_ADDR)
#define IS_QSPI1_TX_PERIPH_BASE(__INSTANCE__)   ((__INSTANCE__) == QSPI1_TX_DATA_ADDR)

#define IS_USART0_RX_PERIPH_BASE(__INSTANCE__)  (((__INSTANCE__) == USART0_RX_DATA_CORE0_ADDR) || \
                                                 ((__INSTANCE__) == USART0_RX_DATA_CORE1_ADDR))
#define IS_USART0_TX_PERIPH_BASE(__INSTANCE__)  (((__INSTANCE__) == USART0_TX_DATA_CORE0_ADDR) || \
                                                 ((__INSTANCE__) == USART0_TX_DATA_CORE1_ADDR))

#define IS_USART1_RX_PERIPH_BASE(__INSTANCE__)  (((__INSTANCE__) == USART1_RX_DATA_CORE0_ADDR) || \
                                                 ((__INSTANCE__) == USART1_RX_DATA_CORE1_ADDR))
#define IS_USART1_TX_PERIPH_BASE(__INSTANCE__)  (((__INSTANCE__) == USART1_TX_DATA_CORE0_ADDR) || \
                                                 ((__INSTANCE__) == USART1_TX_DATA_CORE1_ADDR))

#define IS_CRC0_PERIPH_BASE(__INSTANCE__)       (((__INSTANCE__) == CRC0_DATA_CORE0_ADDR) || \
                                                 ((__INSTANCE__) == CRC0_DATA_CORE1_ADDR))
/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup DMA_Private_Functions DMA Private Functions
 * @{
 */

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

#endif /* __RN_RV_HAL_DMA_H */
