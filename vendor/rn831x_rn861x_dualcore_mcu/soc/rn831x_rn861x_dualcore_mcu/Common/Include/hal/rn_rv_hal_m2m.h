/**
 ******************************************************************************
 * @file   rn_rv_hal_m2m.h
 * @author Renergy Software Team
 * @brief  Header file of M2M HAL module.
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
#ifndef __RN_RV_HAL_M2M_H
#define __RN_RV_HAL_M2M_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal_def.h"
#include "rn_rv_ll_m2m.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup M2M
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup M2M_Exported_Types M2M Exported Types
 * @{
 */

/**
 * @brief  M2M Init Structure definition
 */
typedef struct
{
    uint32_t SrcAddress;           /*!< Specifies the Source base address.
                                        This parameter must be a value between Min_Data = 0 and Max_Data = 0xFFFFFFFF. */

    uint32_t DstAddress;           /*!< Specifies the Destination base address.
                                        This parameter must be a value between Min_Data = 0 and Max_Data = 0xFFFFFFFF. */

    uint32_t SrcDataValidBit;      /*!< Specifies the Source base data valid bit.
                                        This parameter can be a value of @ref M2M_Data_Mode */

    uint32_t DstDataValidBit;      /*!< Specifies the Destination base data valid bit.
                                        This parameter can be a value of @ref M2M_Data_Mode */

    uint32_t DstDataReverseMode;   /*!< Specifies the data reverse mode.
                                        This parameter can be a value of @ref M2M_Reverse_Mode */

    uint32_t DummyEnable;          /*!< Specifies the dummy enable bit.
                                        This parameter must be 0 or 1. */

    uint32_t Dummy;                /*!< Specifies the dummy.
                                        This parameter must be a value between Min_Data = 0 and Max_Data = 0xFFFFFFFF. */

    uint32_t NbData;               /*!< Specifies the number of data to transfer, in data unit.
                                        This parameter must be a value between Min_Data = 0 and Max_Data = 0xFFFFF */

} M2M_InitTypeDef;

/**
 * @brief  M2M HAL State Structure definition
 */
typedef enum __attribute__((__packed__))
{
    HAL_M2M_STATE_RESET             = 0x00U,  /*!< M2M not yet initialized or disabled. */
    HAL_M2M_STATE_READY             = 0x01U,  /*!< M2M initialized and ready for use. */
    HAL_M2M_STATE_BUSY              = 0x02U,  /*!< M2M process is ongoing. */
    HAL_M2M_STATE_TIMEOUT           = 0x03U   /*!< M2M timeout state. */
} HAL_M2M_StateTypeDef;

/**
 * @brief  HAL M2M Callback ID structure definition
 */
typedef enum __attribute__((__packed__))
{
    HAL_M2M_XFER_CPLT_CB_ID         = 0x00U,   /*!< Full transfer. */
    HAL_M2M_XFER_ABORT_CB_ID        = 0x01U,   /*!< Abort. */
} HAL_M2M_CallbackIDTypeDef;

/**
 * @brief  M2M Handle Structure definition
 */
typedef struct __M2M_HandleTypeDef
{
    M2M_TypeDef              *Instance;                                                /*!< Register base address. */

    M2M_InitTypeDef           Init;                                                    /*!< M2M Init configuration. */

    HAL_LockTypeDef           Lock;                                                    /*!< M2M locking object. */

    __IO HAL_M2M_StateTypeDef State;                                                   /*!< M2M tranfer state. */

    void                      (*XferCpltCallback)(struct __M2M_HandleTypeDef *hm2m);   /*!< M2M transfer complete callback. */

    void                      (*XferAbortCallback)(struct __M2M_HandleTypeDef *hm2m);  /*!< M2M transfer abort callback. */

    __IO uint32_t             ErrorCode;                                               /*!< M2M Error code. */

} M2M_HandleTypeDef;

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup M2M_Exported_Constants M2M Exported Constants
 * @{
 */

/** @defgroup M2M_Error_Code M2M Error Code
 * @{
 */
#define HAL_M2M_ERROR_NONE                  0x00000000U    /*!< No error. */
#define HAL_M2M_ERROR_TE                    0x00000001U    /*!< Transfer error. */
#define HAL_M2M_ERROR_NO_XFER               0x00000002U    /*!< no ongoing transfer. */
#define HAL_M2M_ERROR_TIMEOUT               0x00000003U    /*!< Timeout error. */
#define HAL_M2M_ERROR_NOT_SUPPORTED         0x00000004U    /*!< Not supported mode. */
/**
 * @}
 */

/** @defgroup M2M_Data_Mode Data Mode
 * @{
 */
#define M2M_DATA_VALID_BYTE0                LL_M2M_DATA_VALID_BYTE0
#define M2M_DATA_VALID_BYTE1                LL_M2M_DATA_VALID_BYTE1
#define M2M_DATA_VALID_BYTE2                LL_M2M_DATA_VALID_BYTE2
#define M2M_DATA_VALID_BYTE3                LL_M2M_DATA_VALID_BYTE3
#define M2M_DATA_VALID_ALL                  LL_M2M_DATA_VALID_ALL
/**
 * @}
 */

/** @defgroup M2M_Reverse_Mode Reverse Mode
 * @{
 */
#define M2M_DATA_REV_NONE                   LL_M2M_DATA_REV_NONE      /*!< Reverse Mode : no reverse */
#define M2M_DATA_REV_BYTEONLY               LL_M2M_DATA_REV_BYTEONLY  /*!< Reverse Mode : reverse in byte only */
#define M2M_DATA_REV_ADDR                   LL_M2M_DATA_REV_ADDR      /*!< Reverse Mode : reverse in address, including byte reverse */
#define M2M_DATA_REV_ADDRONLY               LL_M2M_DATA_REV_ADDRONLY  /*!< Reverse Mode : reverse in address only */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macros ----------------------------------------------------------*/
/** @defgroup M2M_Exported_Macros M2M Exported Macros
 * @{
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup M2M_Exported_Functions M2M Exported Functions
 * @{
 */

/** @addtogroup M2M_Exported_Functions_Group1
 * @{
 */
/* Initialization and Configuration functions ********************************/
HAL_StatusTypeDef HAL_M2M_Init(M2M_HandleTypeDef *hm2m);
HAL_StatusTypeDef HAL_M2M_DeInit(M2M_HandleTypeDef *hm2m);
void              HAL_M2M_MspInit(M2M_HandleTypeDef *hm2m);
void              HAL_M2M_MspDeInit(M2M_HandleTypeDef *hm2m);
/**
 * @}
 */

/** @addtogroup M2M_Exported_Functions_Group2
 * @{
 */
/* IO operation functions ****************************************************/
HAL_StatusTypeDef HAL_M2M_Start(M2M_HandleTypeDef *hm2m, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);
HAL_StatusTypeDef HAL_M2M_Start_IT(M2M_HandleTypeDef *hm2m, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);
HAL_StatusTypeDef HAL_M2M_Abort(M2M_HandleTypeDef *hm2m);
HAL_StatusTypeDef HAL_M2M_Abort_IT(M2M_HandleTypeDef *hm2m);
HAL_StatusTypeDef HAL_M2M_PollForTransfer(M2M_HandleTypeDef *hm2m, uint32_t Timeout);
void              HAL_M2M_IRQHandler(M2M_HandleTypeDef *hm2m);
HAL_StatusTypeDef HAL_M2M_RegisterCallback(M2M_HandleTypeDef *hm2m, HAL_M2M_CallbackIDTypeDef CallbackID, void (* pCallback)( M2M_HandleTypeDef * _hm2m));
HAL_StatusTypeDef HAL_M2M_UnRegisterCallback(M2M_HandleTypeDef *hm2m, HAL_M2M_CallbackIDTypeDef CallbackID);
/**
 * @}
 */

/** @addtogroup M2M_Exported_Functions_Group3
 * @{
 */
/* State functions ***********************************************************/
HAL_M2M_StateTypeDef HAL_M2M_GetState(const M2M_HandleTypeDef *hm2m);
/**
 * @}
 */

/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/** @defgroup M2M_Private_Types M2M Private Types
 * @{
 */

/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/** @defgroup M2M_Private_Variables M2M Private Variables
 * @{
 */

/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup M2M_Private_Constants M2M Private Constants
 * @{
 */

/**
 * @}
 */

/* Private Macros -----------------------------------------------------------*/
/** @defgroup M2M_Private_Macros M2M Private Macros
 * @{
 */

#define IS_M2M_DATA_BYTE_VALID(__VALUE__)  (((__VALUE__) == M2M_DATA_VALID_BYTE0) || \
                                            ((__VALUE__) == M2M_DATA_VALID_BYTE1) || \
                                            ((__VALUE__) == M2M_DATA_VALID_BYTE2) || \
                                            ((__VALUE__) == M2M_DATA_VALID_BYTE3) || \
                                            ((__VALUE__) == M2M_DATA_VALID_ALL))

#define IS_M2M_DATA_REV_MODE(__VALUE__)    (((__VALUE__) == M2M_DATA_REV_NONE) || \
                                            ((__VALUE__) == M2M_DATA_REV_BYTEONLY) || \
                                            ((__VALUE__) == M2M_DATA_REV_ADDR) || \
                                            ((__VALUE__) == M2M_DATA_REV_ADDRONLY))

#define IS_M2M_SRAM_ADDR_VALID(__VALUE__)  ((__VALUE__) <= 0x28000U)

#define IS_M2M_BUFFER_SIZE(__VALUE__)      (((__VALUE__) >= 0x1U) && ((__VALUE__) < 0x10000U))

/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup M2M_Private_Functions M2M Private Functions
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

#endif /* __RN_RV_HAL_M2M_H */
