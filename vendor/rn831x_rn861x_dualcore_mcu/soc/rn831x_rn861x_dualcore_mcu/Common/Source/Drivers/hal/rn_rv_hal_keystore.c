/**
 ******************************************************************************
 * @file   rn_rv_hal_keystore.c
 * @author Renergy Software Team
 * @brief  KeyStore HAL module driver.
 *         This file provides firmware functions to manage the following
 *         functionalities of the KeyStore peripheral:
 *          + Control functions
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

/** @defgroup KEYSTORE KEYSTORE
 * @brief     KEYSTORE driver modules
 * @{
 */

#if defined(HAL_RPC_MODULE_ENABLED) && \
    defined(HAL_KEYSTORE_MODULE_ENABLED)

/* Private typedef ----------------------------------------------------------*/
/** @defgroup KEYSTORE_Private_Typedef KEYSTORE Private Typedef
 * @{
 */

/**
 * @brief KEYSTORE IAP interface defination.
 */
typedef struct
{
    uint32_t Reserved0[46];
    uint32_t (*KEYSTORE_ReadKey)        (uint32_t *K, uint32_t KeyLen, const uint8_t KeyIdx);
    uint32_t (*KEYSTORE_ReadKeyProperty)(KEYSTORE_InitTypeDef *KEYSTORE_InitStruct, const uint8_t KeyIdx);
    uint32_t (*KEYSTORE_WriteKey)       (KEYSTORE_InitTypeDef *KEYSTORE_InitStruct, const uint32_t *K, const uint8_t KeyIdx);
    uint32_t Reserved1;
    uint32_t (*KEYSTORE_Erase)          (uint32_t Passwd);
    uint32_t Reserved2;
    void     (*KEYSTORE_Unlock)         (void);
    uint32_t (*KEYSTORE_GetState)       (void);
} HAL_KEYSTORE_ApiTypeDef;

/**
 * @brief KeyStore IAP function pointer.
 */
typedef uint32_t (*pKEYSTORE_ReadKey)        (uint32_t *K, uint32_t KeyLen, const uint8_t KeyIdx);
typedef uint32_t (*pKEYSTORE_ReadKeyProperty)(KEYSTORE_InitTypeDef *KEYSTORE_InitStruct, const uint8_t KeyIdx);
typedef uint32_t (*pKEYSTORE_WriteKey)       (KEYSTORE_InitTypeDef *KEYSTORE_InitStruct, const uint32_t *K, const uint8_t KeyIdx);
typedef uint32_t (*pKEYSTORE_Erase)          (uint32_t Passwd);
typedef void     (*pKEYSTORE_Unlock)         (void);
typedef uint32_t (*pKEYSTORE_GetState)       (void);
/**
 * @}
 */

/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @defgroup KEYSTORE_Exported_Functions KEYSTORE Exported Functions
 * @{
 */

/** @defgroup KEYSTORE_Exported_Functions_Group1 IO operation functions
 * @brief     IO operation functions
 *
@verbatim
================================================================================
                    ##### IO operation functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) HAL_KEYSTORE_Start                    : Start the KEYSTORE module

@endverbatim
 * @{
 */

/**
 * @brief  Unlock the KeyStore write and read permission.
 *
 * @retval Returned value can be one of the following values:
 *         @arg @ref HAL_KEYSTORE_STA_SUCCESS
 *         @arg @ref HAL_KEYSTORE_STA_ERROR
 */
HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_Unlock(void)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_KEYSTORE_STA_ERROR;
    }
    HAL_KEYSTORE_ApiTypeDef *Iap_Func = (HAL_KEYSTORE_ApiTypeDef *)iap_addr;
    pKEYSTORE_Unlock func_ptr = (pKEYSTORE_Unlock)((uint32_t)Iap_Func->KEYSTORE_Unlock + ROM_BASE_ADDR);
    func_ptr();
    return HAL_KEYSTORE_STA_SUCCESS;
#endif
#ifdef CORE1
    uint32_t api_id = KEYSTORE_UNLOCK_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 0, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_KEYSTORE_StatusTypeDef)result;
    }

    return (HAL_KEYSTORE_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Reading the key from the specified key slot.
 *
 * @param  PKey pointer to key buffer.
 * @param  KeyLen size of key.
 * @param  KeyIdx key index in KeyStore.
 * @retval Returned value can be one of the following values:
 *         @arg @ref HAL_KEYSTORE_STA_SUCCESS
 *         @arg @ref HAL_KEYSTORE_STA_ERROR
 *         @arg @ref HAL_KEYSTORE_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_SEA_BUSY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_KINDEX_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_KEY_NULL_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_SECTOR_INDEX_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_RD_OR_CS_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_NVR_NOEMPTY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_PARM_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_ROOTKY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_RVK_EMPTY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_ERR_PWD_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_STA_CLR_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_FLASH_CLK_FREQ_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_FLASH_RD_ONLY_FAILURE
 */
HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_ReadKey(uint32_t *PKey, uint32_t KeyLen, const uint8_t KeyIdx)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_KEYSTORE_STA_ERROR;
    }
    HAL_KEYSTORE_ApiTypeDef *Iap_Func = (HAL_KEYSTORE_ApiTypeDef *)iap_addr;
    pKEYSTORE_ReadKey func_ptr = (pKEYSTORE_ReadKey)((uint32_t)Iap_Func->KEYSTORE_ReadKey + ROM_BASE_ADDR);
    return func_ptr(PKey, KeyLen, KeyIdx);
#endif
#ifdef CORE1
    uint32_t api_id = KEYSTORE_RD_KEY_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)PKey;
    api_params[2] = KeyLen;
    api_params[3] = (uint32_t)KeyIdx;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 3, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_KEYSTORE_StatusTypeDef)result;
    }

    return (HAL_KEYSTORE_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Reading the key infomation from the specified key slot.
 *
 * @param  KEYSTORE_InitStruct KeyStore init instance
 * @param  KeyIdx key index in KeyStore.
 * @retval Returned value can be one of the following values:
 *         @arg @ref HAL_KEYSTORE_STA_SUCCESS
 *         @arg @ref HAL_KEYSTORE_STA_ERROR
 *         @arg @ref HAL_KEYSTORE_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_SEA_BUSY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_KINDEX_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_KEY_NULL_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_SECTOR_INDEX_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_RD_OR_CS_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_NVR_NOEMPTY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_PARM_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_ROOTKY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_RVK_EMPTY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_ERR_PWD_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_STA_CLR_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_FLASH_CLK_FREQ_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_FLASH_RD_ONLY_FAILURE
 */
HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_ReadKeyProperty(KEYSTORE_InitTypeDef *KEYSTORE_InitStruct, const uint8_t KeyIdx)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_KEYSTORE_STA_ERROR;
    }
    HAL_KEYSTORE_ApiTypeDef *Iap_Func = (HAL_KEYSTORE_ApiTypeDef *)iap_addr;
    pKEYSTORE_ReadKeyProperty func_ptr = (pKEYSTORE_ReadKeyProperty)((uint32_t)Iap_Func->KEYSTORE_ReadKeyProperty + ROM_BASE_ADDR);
    return func_ptr(KEYSTORE_InitStruct, KeyIdx);
#endif
#ifdef CORE1
    uint32_t api_id = KEYSTORE_RD_KEY_INFO_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)KEYSTORE_InitStruct;
    api_params[2] = (uint32_t)KeyIdx;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 2, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_KEYSTORE_StatusTypeDef)result;
    }

    return (HAL_KEYSTORE_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Writing the key into the specified key slot.
 *
 * @param  KEYSTORE_InitStruct KeyStore init instance
 * @param  PKey pointer to key buffer.
 * @param  KeyIdx key index in KeyStore.
 * @retval Returned value can be one of the following values:
 *         @arg @ref HAL_KEYSTORE_STA_SUCCESS
 *         @arg @ref HAL_KEYSTORE_STA_ERROR
 *         @arg @ref HAL_KEYSTORE_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_SEA_BUSY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_KINDEX_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_KEY_NULL_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_SECTOR_INDEX_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_RD_OR_CS_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_NVR_NOEMPTY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_PARM_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_ROOTKY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_RVK_EMPTY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_ERR_PWD_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_STA_CLR_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_FLASH_CLK_FREQ_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_FLASH_RD_ONLY_FAILURE
 */
HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_WriteKey(KEYSTORE_InitTypeDef *KEYSTORE_InitStruct, const uint32_t *PKey, const uint8_t KeyIdx)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_KEYSTORE_STA_ERROR;
    }
    HAL_KEYSTORE_ApiTypeDef *Iap_Func = (HAL_KEYSTORE_ApiTypeDef *)iap_addr;
    pKEYSTORE_WriteKey func_ptr = (pKEYSTORE_WriteKey)((uint32_t)Iap_Func->KEYSTORE_WriteKey + ROM_BASE_ADDR);
    return func_ptr(KEYSTORE_InitStruct, PKey, KeyIdx);
#endif
#ifdef CORE1
    uint32_t api_id = KEYSTORE_WR_KEY_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = (uint32_t)KEYSTORE_InitStruct;
    api_params[2] = (uint32_t)PKey;
    api_params[3] = (uint32_t)KeyIdx;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 3, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_KEYSTORE_StatusTypeDef)result;
    }

    return (HAL_KEYSTORE_StatusTypeDef)api_params[1];
#endif
}

/**
 * @brief  Erasing all KeyStore NVRs.
 *
 * @param  Passwd password.
 * @retval Returned value can be one of the following values:
 *         @arg @ref HAL_KEYSTORE_STA_SUCCESS
 *         @arg @ref HAL_KEYSTORE_STA_ERROR
 *         @arg @ref HAL_KEYSTORE_STA_TIMEOUT_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_SEA_BUSY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_KINDEX_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_KEY_NULL_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_SECTOR_INDEX_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_RD_OR_CS_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_NVR_NOEMPTY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_PARM_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_ROOTKY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_RVK_EMPTY_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_ERR_PWD_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_STA_CLR_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_FLASH_CLK_FREQ_FAILURE
 *         @arg @ref HAL_KEYSTORE_STA_FLASH_RD_ONLY_FAILURE
 */
HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_Erase(uint32_t Passwd)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_KEYSTORE_STA_ERROR;
    }
    HAL_KEYSTORE_ApiTypeDef *Iap_Func = (HAL_KEYSTORE_ApiTypeDef *)iap_addr;
    pKEYSTORE_Erase func_ptr = (pKEYSTORE_Erase)((uint32_t)Iap_Func->KEYSTORE_Erase + ROM_BASE_ADDR);
    return func_ptr(Passwd);
#endif
#ifdef CORE1
    uint32_t api_id = KEYSTORE_ERASE_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = Passwd;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 1, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (HAL_KEYSTORE_StatusTypeDef)result;
    }

    return (HAL_KEYSTORE_StatusTypeDef)api_params[1];
#endif
}

/**
 * @}
 */

/** @defgroup KEYSTORE_Exported_Functions_Group2 State functions
 * @brief     State functions
 *
@verbatim
================================================================================
                    ##### State functions #####
================================================================================
    [..]  This subsection provides functions allowing to :
        (+) HAL_KEYSTORE_GetState()  : Return the KEYSTORE state.

@endverbatim
 * @{
 */

/**
 * @brief  Get the KeyStore module status.
 *
 * @retval Between 0 to 0x3F.
 */
uint32_t HAL_KEYSTORE_GetState(void)
{
#ifdef CORE0
    uint32_t iap_addr;
    if (HAL_OK != GetIAPHeaderAddress(&iap_addr)) {
        return HAL_KEYSTORE_STA_ERROR;
    }
    HAL_KEYSTORE_ApiTypeDef *Iap_Func = (HAL_KEYSTORE_ApiTypeDef *)iap_addr;
    pKEYSTORE_GetState func_ptr = (pKEYSTORE_GetState)((uint32_t)Iap_Func->KEYSTORE_GetState + ROM_BASE_ADDR);
    return func_ptr();
#endif
#ifdef CORE1
    uint32_t api_id = KEYSTORE_GET_STA_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 0, HAL_RPC_TIMEOUT_FLAG);
    if (HAL_OK != result) {
        return (uint32_t)result;
    }

    return api_params[1];
#endif
}

/**
 * @}
 */

/**
 * @}
 */

#endif /* HAL_KEYSTORE_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
