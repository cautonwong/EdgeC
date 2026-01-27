/**
 ******************************************************************************
 * @file   rn_rv_hal_flash.c
 * @author Renergy Software Team
 * @brief  FLASH HAL module driver.
 *          This file provides firmware functions to manage the following
 *          functionalities of the internal FLASH memory:
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

/** @defgroup FLASH FLASH
 * @brief     FLASH driver modules
 * @{
 */

#if defined(HAL_FLASH_MODULE_ENABLED) && defined(HAL_RPC_MODULE_ENABLED)

/* Private typedef ----------------------------------------------------------*/
/** @defgroup FLASH_Private_Typedef FLASH Private Typedef
 * @{
 */

/**
 * @brief  FLASH IAP interface Structure definition
 */
typedef struct
{
    uint32_t Reserved0;
    uint32_t (*FLASH_Unlock)        (HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t ByteLength);
    uint32_t (*FLASH_Erase)         (HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t ByteLength);
    uint32_t (*FLASH_Program)       (HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t SrcAddress, uint32_t ByteLength);
    uint32_t Reserved1[54];
    uint32_t (*FLASH_ClearSoftISP)  (uint32_t ISPProtect);
    uint32_t (*FLASH_EnableSoftISP) (uint32_t ISPCommand, uint32_t UartIOCommand);
    uint32_t (*FLASH_ClearCP)       (uint32_t JtagMode, uint32_t Core0JtagEnable, uint32_t Core1JtagEnable);
} FLASH_ApiTypeDef;

/**
 * @brief  HAL FLASH IAP function pointer definition
 */
typedef uint32_t (*pFLASH_Unlock)        (HAL_FLASH_IDTypeDef , uint32_t , uint32_t );            /*!< pointer to FLASH_Unlock function        */
typedef uint32_t (*pFLASH_Erase)         (HAL_FLASH_IDTypeDef , uint32_t , uint32_t );            /*!< pointer to FLASH_Erase function         */
typedef uint32_t (*pFLASH_Program)       (HAL_FLASH_IDTypeDef , uint32_t , uint32_t , uint32_t ); /*!< pointer to FLASH_Program function       */
typedef uint32_t (*pFLASH_ClearSoftISP)  (uint32_t );                                             /*!< pointer to FLASH_ClearSoftISP function  */
typedef uint32_t (*pFLASH_EnableSoftISP) (uint32_t , uint32_t );                                  /*!< pointer to FLASH_EnableSoftISP function */
typedef uint32_t (*pFLASH_ClearCP)       (uint32_t , uint32_t , uint32_t );                       /*!< pointer to FLASH_ClearCP function       */
/**
 * @}
 */

/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @defgroup FLASH_Exported_Functions FLASH Exported Functions
 * @{
 */

/** @defgroup FLASH_Exported_Functions_Group1 Peripheral Control functions
 *  @brief     management functions.
 *
@verbatim
================================================================================
                    ##### Peripheral Control functions #####
================================================================================
    [..]
      This subsection provides a set of functions allowing to control the FLASH
      memory operations.

@endverbatim
 * @{
 */

/**
 * @brief  Unlock the write access of FLASH at specified address and byte length.
 * @note   DstAddress must be page-aligned (1 page = 512 bytes).
 *
 * @param  FlashID: Indicate the object to be unlocked.
 *                  This parameter can be a value of @ref HAL_FLASH_IDTypeDef
 * @param  DstAddress:    Specify the flash destination address to be unlockd.
 * @param  ByteLength: Specify the byte length to be unlockd.
 *         @note   If ByteLength = 0, enable chip erase
 *         @note   When FlashID is selected as HAL_FLASH_ID_NVR, ByteLength must be a multiple of 512.
 * @retval HAL_FLASH_StatusTypeDef HAL FLASH Status
 */
HAL_FLASH_StatusTypeDef HAL_FLASH_Unlock(HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t ByteLength)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_FALSH_INVALID_COMMAND;
    }
    FLASH_ApiTypeDef *Iap_Func = (FLASH_ApiTypeDef *)iap_addr;
    pFLASH_Unlock func_ptr = (pFLASH_Unlock)((uint32_t)Iap_Func->FLASH_Unlock + ROM_BASE_ADDR);
    return func_ptr(FlashID, DstAddress, ByteLength);
#endif /* CORE0 */
#ifdef CORE1
    uint32_t api_id = FLASH_UNLOCK_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)FlashID;
    api_params[2] = DstAddress;
    api_params[3] = ByteLength;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 3, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_FLASH_StatusTypeDef)result;
    }

    return (HAL_FLASH_StatusTypeDef)api_params[1];
#endif /* CORE1 */
}

/**
 * @}
 */

/** @defgroup FLASH_Exported_Functions_Group2 Programming operation functions
 *  @brief    Programming and Erasing operation functions
 *
@verbatim
@endverbatim
 * @{
 */

/**
 * @brief  Program specified byte length at destination address of FLASH from source address.
 * @note   DstAddress must be page-aligned (1 page = 512 bytes).
 * @note   The function HAL_FLASH_Unlock() should be called before programming the FLASH.
 *
 * @note   FLASH should be previously erased before a new programmation.
 *
 * @param  FlashID:    Indicate the object to be programmed.
 *                     This parameter can be a value of @ref HAL_FLASH_IDTypeDef
 * @param  DstAddress: Specify the FLASH destination address to be programmed.
 * @param  SrcAddress: Specify the source data to be programmed.
 * @param  ByteLength: Specify the length to be programmed.
 *         @note   When FlashID is selected as HAL_FLASH_ID_NVR, ByteLength must be a multiple of 512.
 *
 * @retval HAL_FLASH_StatusTypeDef HAL FLASH Status
 */
HAL_FLASH_StatusTypeDef HAL_FLASH_Program(HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t SrcAddress, uint32_t ByteLength)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_FALSH_INVALID_COMMAND;
    }
    FLASH_ApiTypeDef *Iap_Func = (FLASH_ApiTypeDef *)iap_addr;
    pFLASH_Program func_ptr = (pFLASH_Program)((uint32_t)Iap_Func->FLASH_Program + ROM_BASE_ADDR);
    return func_ptr(FlashID, DstAddress, SrcAddress, ByteLength);
#endif /* CORE0 */
#ifdef CORE1
    uint32_t api_id = FLASH_PROGRAM_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)FlashID;
    api_params[2] = DstAddress;
    api_params[3] = SrcAddress;
    api_params[4] = ByteLength;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 4, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_FLASH_StatusTypeDef)result;
    }

    return (HAL_FLASH_StatusTypeDef)api_params[1];
#endif /* CORE1 */
}

/**
 * @brief  Erase specified byte length at destination address of FLASH from source address.
 * @note   DstAddress must be page-aligned (1 page = 512 bytes).
 * @note   The function HAL_FLASH_Unlock() should be called before erasing the FLASH.
 *
 * @param  FlashID: Indicate the object to be erased.
 *                  This parameter can be a value of @ref HAL_FLASH_IDTypeDef
 * @param  DstAddress:    Specify the address to be erased.
 * @param  ByteLength: Specify the byte length to be erased.
 *         @note   When FlashID is selected as HAL_FLASH_ID_NVR, ByteLength must be a multiple of 512.
 *
 * @retval HAL_FLASH_StatusTypeDef HAL FLASH Status
 */
HAL_FLASH_StatusTypeDef HAL_FLASH_Erase(HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t ByteLength)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_FALSH_INVALID_COMMAND;
    }
    FLASH_ApiTypeDef *Iap_Func = (FLASH_ApiTypeDef *)iap_addr;
    pFLASH_Erase func_ptr = (pFLASH_Erase)((uint32_t)Iap_Func->FLASH_Erase + ROM_BASE_ADDR);
    return func_ptr(FlashID, DstAddress, ByteLength);
#endif /* CORE0 */
#ifdef CORE1
    uint32_t api_id = FLASH_ERASE_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)FlashID;
    api_params[2] = DstAddress;
    api_params[3] = ByteLength;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 3, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_FLASH_StatusTypeDef)result;
    }

    return (HAL_FLASH_StatusTypeDef)api_params[1];
#endif /* CORE1 */
}

/**
 * @}
 */

/** @defgroup FLASH_Exported_Functions_Group3 ISP and CP management functions
 *  @brief     ISP and CP management functions
 *
@verbatim
================================================================================
                    ##### ISP and CP management functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) HAL_FLASH_IspClear                   : Soft ISP clear
        (+) HAL_FLASH_SoftIspEn                  : Soft ISP enable
        (+) HAL_FLASH_CpClear                    : CP clear
@endverbatim
 * @{
 */

/**
 * @brief  Clear ISP configuration parameters and configure ISP protection for NVR2.
 * @note   Before using this function, the function HAL_FLASH_Unlock() should be called to unlock NVR2.
 *
 * @param  ISPProtect: ISP protection value, 0x69789687 indicates enable ISP protect,
 *                     other values indicate disable ISP protect.
 *
 * @retval HAL_FLASH_StatusTypeDef HAL FLASH Status
 */
HAL_FLASH_StatusTypeDef HAL_FLASH_ClearSoftISP(uint32_t ISPProtect)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_FALSH_INVALID_COMMAND;
    }
    FLASH_ApiTypeDef *Iap_Func = (FLASH_ApiTypeDef *)iap_addr;
    pFLASH_ClearSoftISP func_ptr = (pFLASH_ClearSoftISP)((uint32_t)Iap_Func->FLASH_ClearSoftISP + ROM_BASE_ADDR);
    return func_ptr(ISPProtect);
#endif /* CORE0 */
#ifdef CORE1
    uint32_t api_id = FLASH_CLEARSOFTISP_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = ISPProtect;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 1, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_FLASH_StatusTypeDef)result;
    }

    return (HAL_FLASH_StatusTypeDef)api_params[1];
#endif /* CORE1 */
}

/**
 * @brief  Set the ISP configuration parameters of NVR2.
 * @note   Before using this function, the function HAL_FLASH_Unlock() should be called to unlock NVR2.
 *
 * @param  ISPCommand:    Soft ISP enable configuration, 0xe7a5185a indicates enable soft ISP,
 *                        other values indicate disable soft ISP.
 * @param  UartIOCommand: ISP UART configuration, this paramater can be 0~16.
 *                        Please refer to user manual for the specific mapping relationship with UARTx.
 *
 * @retval HAL_FLASH_StatusTypeDef HAL FLASH Status
 */
HAL_FLASH_StatusTypeDef HAL_FLASH_EnableSoftISP(uint32_t ISPCommand, uint32_t UartIOCommand)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_FALSH_INVALID_COMMAND;
    }
    FLASH_ApiTypeDef *Iap_Func = (FLASH_ApiTypeDef *)iap_addr;
    pFLASH_EnableSoftISP func_ptr = (pFLASH_EnableSoftISP)((uint32_t)Iap_Func->FLASH_EnableSoftISP + ROM_BASE_ADDR);
    return func_ptr(ISPCommand, UartIOCommand);
#endif /* CORE0 */
#ifdef CORE1
    uint32_t api_id = FLASH_ENABLESOFTISP_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = ISPCommand;
    api_params[2] = UartIOCommand;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 2, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_FLASH_StatusTypeDef)result;
    }

    return (HAL_FLASH_StatusTypeDef)api_params[1];
#endif /* CORE1 */
}


/**
 * @brief  Clear the CP configuration and enable core0/core1 jtag.
 * @note   If jtag is selected to enabled, the lock configuration of registers will be cleared.
 *
 * @param  JtagMode:        Jtag mode configuration, JtagMode = 1: configure as two lines mode,
 *                          JtagMode = other value: no modify and maintain the original mode.
 * @param  Core0JtagEnable: CORE0 jtag enable configuration, Core0JtagEnable = 1: enable core0 jtag,
 *                          Core0JtagEnable = other value: no modify and maintain the original value.
 * @param  Core1JtagEnable: CORE1 jtag enable configuration, Core1JtagEnable = 1: enable core0 jtag,
 *                          Core1JtagEnable = other value: no modify and maintain the original value.
 *
 * @retval HAL_FLASH_StatusTypeDef HAL FLASH Status
 */
HAL_FLASH_StatusTypeDef HAL_FLASH_ClearCP(uint32_t JtagMode, uint32_t Core0JtagEnable, uint32_t Core1JtagEnable)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_FALSH_INVALID_COMMAND;
    }
    FLASH_ApiTypeDef *Iap_Func = (FLASH_ApiTypeDef *)iap_addr;
    pFLASH_ClearCP func_ptr = (pFLASH_ClearCP)((uint32_t)Iap_Func->FLASH_ClearCP + ROM_BASE_ADDR);
    return func_ptr(JtagMode, Core0JtagEnable, Core1JtagEnable);
#endif /* CORE0 */
#ifdef CORE1
    uint32_t api_id = FLASH_CLEARCP_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = JtagMode;
    api_params[2] = Core0JtagEnable;
    api_params[3] = Core1JtagEnable;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 3, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_FLASH_StatusTypeDef)result;
    }

    return (HAL_FLASH_StatusTypeDef)api_params[1];
#endif /* CORE1 */
}

/**
 * @}
 */

/**
 * @}
 */

#endif /* HAL_FLASH_MODULE_ENABLED && HAL_RPC_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
