/**
 ******************************************************************************
 * @file   rn_rv_hal_crc.h
 * @author Renergy Software Team
 * @brief  Header file of CRC HAL module.
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
#ifndef __RN_RV_HAL_CRC_H
#define __RN_RV_HAL_CRC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal_def.h"
#include "rn_rv_ll_crc.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup CRC
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup CRC_Exported_Types CRC Exported Types
 * @{
 */

/**
 * @brief  CRC HAL State Structure definition
 */
typedef enum
{
    HAL_CRC_STATE_RESET     = 0x00U,  /*!< CRC not yet initialized or disabled */
    HAL_CRC_STATE_READY     = 0x01U,  /*!< CRC initialized and ready for use   */
    HAL_CRC_STATE_BUSY      = 0x02U,  /*!< CRC internal process is ongoing     */
    HAL_CRC_STATE_TIMEOUT   = 0x03U,  /*!< CRC timeout state                   */
    HAL_CRC_STATE_ERROR     = 0x04U   /*!< CRC error state                     */
} HAL_CRC_StateTypeDef;

/**
 * @brief  CRC Init Structure definition
 */
typedef struct
{
    uint32_t InputRevMode;     /*!< Reverse input data. */

    uint32_t OutputRevEnable;  /*!< Reverse output data. */

    uint32_t DataAlignment;    /*!< Specifies the Peripheral data width. */

    uint32_t PolynomialSize;   /*!< Size of Polynomial value. */

    uint32_t InitialValue;     /*!< The Initial value for CRC operation. */

    uint32_t Polynomial;       /*!< Polynomial value. */

    uint32_t XorValue;         /*!< Default value of XOR operation. */

    uint32_t DmaMode;          /*!< CRC dma mode. */

} CRC_InitTypeDef;

/**
 * @brief  CRC Handle Structure definition
 */
typedef struct
{
    CRC_TypeDef                *Instance;    /*!< Register base address   */

    CRC_InitTypeDef             Init;        /*!< CRC Init configuration  */

    HAL_LockTypeDef             Lock;        /*!< CRC Locking object      */

    __IO HAL_CRC_StateTypeDef   State;       /*!< CRC communication state */

} CRC_HandleTypeDef;

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup CRC_Exported_Constants CRC Exported Constants
 * @{
 */

/** @defgroup CRC_Polynomial_Size Polynomial Size definition
 * @{
 */
#define CRC_POLYSIZE_BIT32              LL_CRC_POLYSIZE_BIT32
#define CRC_POLYSIZE_BIT16              LL_CRC_POLYSIZE_BIT16
#define CRC_POLYSIZE_BIT8               LL_CRC_POLYSIZE_BIT8
#define CRC_POLYSIZE_BIT7               LL_CRC_POLYSIZE_BIT7
/**
 * @}
 */

/** @defgroup CRC_ReverseOutput_Config ReverseOutput Configuration definition
 * @{
 */
#define CRC_OUTPUT_REVERSE_DISABLE      (0x00000000)
#define CRC_OUTPUT_REVERSE_ENABLE       (0x00000001)
/**
 * @}
 */

/** @defgroup CRC_Reverse_Mode Input Reverse Mode definition
 * @{
 */
#define CRC_NOREVERSE                   LL_CRC_INPUT_NOREVERSE
#define CRC_REVERSE_IN_BYTE             LL_CRC_INPUT_REVERSE_IN_BYTE
#define CRC_REVERSE_IN_HALFWORD         LL_CRC_INPUT_REVERSE_IN_HALFWORD
#define CRC_REVERSE_IN_WORD             LL_CRC_INPUT_REVERSE_IN_WORD
/**
 * @}
 */

/** @defgroup CRC_Data_Width Data Width definition
 * @{
 */
#define CRC_OPT_DATA_WIDTH_IN_WORD      LL_CRC_OPT_DATA_WIDTH_IN_WORD
#define CRC_OPT_DATA_WIDTH_IN_HALFWORD  LL_CRC_OPT_DATA_WIDTH_IN_HALFWORD
#define CRC_OPT_DATA_WIDTH_IN_BYTE      LL_CRC_OPT_DATA_WIDTH_IN_BYTE
/**
 * @}
 */

/** @defgroup CRC_Dma_Running_Mode DMA Running Mode definition
 * @{
 */
#define CRC_DMA_MODE_CRC                LL_CRC_DMA_MODE_CRC
#define CRC_DMA_MODE_CHKSUM             LL_CRC_DMA_MODE_CHKSUM
#define CRC_DMA_DISABLE                 (0x0000000F)
/**
 * @}
 */

/**
 * @}
 */

/* Exported macros ----------------------------------------------------------*/
/** @defgroup CRC_Exported_Macros CRC Exported Macros
 * @{
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup CRC_Exported_Functions CRC Exported Functions
 * @{
 */

/** @addtogroup CRC_Exported_Functions_Group1
 * @{
 */
/* Initialization and Configuration functions ********************************/
HAL_StatusTypeDef    HAL_CRC_Init(CRC_HandleTypeDef *hcrc);
HAL_StatusTypeDef    HAL_CRC_DeInit(CRC_HandleTypeDef *hcrc);
void                 HAL_CRC_MspInit(CRC_HandleTypeDef *hcrc);
void                 HAL_CRC_MspDeInit(CRC_HandleTypeDef *hcrc);
/**
 * @}
 */

/** @addtogroup CRC_Exported_Functions_Group2
 * @{
 */
/* IO operation functions ****************************************************/
uint32_t             HAL_CRC_Accumulate(CRC_HandleTypeDef *hcrc, uint8_t pBuffer[], uint32_t BufferLength);
uint32_t             HAL_CRC_Calculate(CRC_HandleTypeDef *hcrc, uint8_t pBuffer[], uint32_t BufferLength);
/**
 * @}
 */

/** @addtogroup CRC_Exported_Functions_Group3
 * @{
 */
/* State functions ***********************************************************/
HAL_CRC_StateTypeDef HAL_CRC_GetState(const CRC_HandleTypeDef *hcrc);
/**
 * @}
 */

/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/** @defgroup CRC_Private_Types CRC Private Types
 * @{
 */

/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/** @defgroup CRC_Private_Variables CRC Private Variables
 * @{
 */

/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup CRC_Private_Constants CRC Private Constants
 * @{
 */

/**
 * @}
 */

/* Private Macros -----------------------------------------------------------*/
/** @defgroup CRC_Private_Macros CRC Private Macros
 * @{
 */

#define IS_CRC_POLYSIZE(__VALUE__)       (((__VALUE__) == CRC_POLYSIZE_BIT32) || \
                                          ((__VALUE__) == CRC_POLYSIZE_BIT16) || \
                                          ((__VALUE__) == CRC_POLYSIZE_BIT8) || \
                                          ((__VALUE__) == CRC_POLYSIZE_BIT7))

#define IS_CRC_REVERSE_TYPE(__VALUE__)   (((__VALUE__) == CRC_NOREVERSE) || \
                                          ((__VALUE__) == CRC_REVERSE_IN_BYTE) || \
                                          ((__VALUE__) == CRC_REVERSE_IN_HALFWORD) || \
                                          ((__VALUE__) == CRC_REVERSE_IN_WORD))

#define IS_CRC_DATA_WIDTH(__VALUE__)     (((__VALUE__) == CRC_OPT_DATA_WIDTH_IN_WORD) || \
                                          ((__VALUE__) == CRC_OPT_DATA_WIDTH_IN_HALFWORD) || \
                                          ((__VALUE__) == CRC_OPT_DATA_WIDTH_IN_BYTE))

#define IS_CRC_DMA_MODE(__VALUE__)       (((__VALUE__) == CRC_DMA_MODE_CRC) || \
                                          ((__VALUE__) == CRC_DMA_MODE_CHKSUM))

/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup CRC_Private_Functions CRC Private Functions
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

#endif /* __RN_RV_HAL_CRC_H */
