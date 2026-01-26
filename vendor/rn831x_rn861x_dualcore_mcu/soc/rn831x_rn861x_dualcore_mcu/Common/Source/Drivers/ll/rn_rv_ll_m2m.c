/**
 ******************************************************************************
 * @file   rn_rv_ll_m2m.c
 * @author Renergy Software Team
 * @brief  M2M LL module driver
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

#if defined(USE_FULL_LL_DRIVER)

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_ll_m2m.h"
#include "rn_rv_ll_system_core0.h"
#include "rn_rv_ll_system_core1.h"

#ifdef USE_FULL_ASSERT
#include "rn_rv_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (M2M)

/** @addtogroup M2M_LL
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup DMA_LL_Private_Macros
 * @{
 */

#define IS_LL_M2M_DATA_BYTE_VALID(__VALUE__)  (((__VALUE__) == LL_M2M_DATA_VALID_BYTE0) || \
                                               ((__VALUE__) == LL_M2M_DATA_VALID_BYTE1) || \
                                               ((__VALUE__) == LL_M2M_DATA_VALID_BYTE2) || \
                                               ((__VALUE__) == LL_M2M_DATA_VALID_BYTE3) || \
                                               ((__VALUE__) == LL_M2M_DATA_VALID_ALL))

#define IS_LL_M2M_DATA_REV_MODE(__VALUE__)    (((__VALUE__) == LL_M2M_DATA_REV_NONE) || \
                                               ((__VALUE__) == LL_M2M_DATA_REV_BYTEONLY) || \
                                               ((__VALUE__) == LL_M2M_DATA_REV_ADDR) || \
                                               ((__VALUE__) == LL_M2M_DATA_REV_ADDRONLY))

#define IS_LL_M2M_SRAM_ADDR_VALID(__VALUE__)  ((__VALUE__) <= 0x28000U)

/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup M2M_LL_Exported_Functions
 * @{
 */

/** @addtogroup M2M_LL_EF_Init
 * @{
 */

/**
 * @brief  Initialize the M2M registers according to the specified parameters in M2M_InitStruct.
 * @param  M2Mx M2M instance
 * @param  M2M_InitStruct M2M_InitStruct instance
 * @retval ErrorStatus
 */
ErrorStatus LL_M2M_Init(M2M_TypeDef *M2Mx, LL_M2M_InitTypeDef *M2M_InitStruct)
{
    ErrorStatus status = ERROR;

    /* Check the parameters */
    assert_param(IS_M2M_ALL_INSTANCE(M2Mx));
    assert_param(IS_LL_M2M_DATA_BYTE_VALID(M2M_InitStruct->SrcDataValidBit));
    assert_param(IS_LL_M2M_DATA_BYTE_VALID(M2M_InitStruct->DstDataValidBit));
    assert_param(IS_LL_M2M_DATA_REV_MODE(M2M_InitStruct->DstDataReverseMode));

    if (LL_M2M_IsEnabled(M2Mx) == 1U) {
        /* Set the MODE register. */
        LL_M2M_SetInputMode(M2Mx, M2M_InitStruct->SrcDataValidBit);
        LL_M2M_SetOutputMode(M2Mx, M2M_InitStruct->DstDataValidBit);
        if (M2M_InitStruct->DstDataReverseMode == LL_M2M_DATA_REV_BYTEONLY) {
            LL_M2M_EnableOutputRevByByte(M2Mx);
            LL_M2M_DisableOutputRevByAddress(M2Mx);
        } else if (M2M_InitStruct->DstDataReverseMode == LL_M2M_DATA_REV_ADDRONLY) {
            LL_M2M_DisableOutputRevByByte(M2Mx);
            LL_M2M_EnableOutputRevByAddress(M2Mx);
        } else if (M2M_InitStruct->DstDataReverseMode == LL_M2M_DATA_REV_ADDR) {
            LL_M2M_EnableOutputRevByByte(M2Mx);
            LL_M2M_EnableOutputRevByAddress(M2Mx);
        } else {
            LL_M2M_DisableOutputRevByByte(M2Mx);
            LL_M2M_DisableOutputRevByAddress(M2Mx);
        }

        /* Set source address register. */
        LL_M2M_SetSrcAddress(M2Mx, M2M_InitStruct->SrcAddress);

        /* Set destination address register. */
        LL_M2M_SetDstAddress(M2Mx, M2M_InitStruct->DstAddress);

        /* Set data length register. */
        LL_M2M_SetDataLength(M2Mx, M2M_InitStruct->NbData);

        if (M2M_InitStruct->DummyEnable == 1U) {
            /* Set dummy register. */
            LL_M2M_SetDummyValue(M2Mx, M2M_InitStruct->Dummy);
        }

        status = SUCCESS;
    }

    return status;
}

/**
 * @brief  De-initialize the M2M registers to their default reset values.
 * @param  M2Mx M2M instance
 * @retval ErrorStatus
 */
ErrorStatus LL_M2M_DeInit(M2M_TypeDef *M2Mx)
{
    ErrorStatus status = ERROR;

    /* Check the parameters */
    assert_param(IS_M2M_ALL_INSTANCE(M2Mx));

    if (LL_M2M_IsEnabled(M2Mx) == 0U) {
        /* Reset M2M register. */
        if (M2Mx == M2M_CORE0) {
            LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_M2M);
            LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_M2M);
        } else if (M2Mx == M2M_CORE1) {
            LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_M2M);
            LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_M2M);
        }

        status = SUCCESS;
    }

    return status;
}

/**
 * @brief  Set each @ref LL_M2M_InitTypeDef field to default value.
 * @param  M2M_InitStruct M2M_InitStruct instance
 * @retval None
 */
void LL_M2M_StructInit(LL_M2M_InitTypeDef *M2M_InitStruct)
{
    /* Set M2M_InitStruct fields to default values. */
    M2M_InitStruct->SrcDataValidBit       = LL_M2M_DATA_VALID_ALL;
    M2M_InitStruct->DstDataValidBit       = LL_M2M_DATA_VALID_ALL;
    M2M_InitStruct->DstDataReverseMode = LL_M2M_DATA_REV_NONE;
    M2M_InitStruct->DummyEnable        = DISABLE;
    M2M_InitStruct->Dummy              = 0x00000000U;
    M2M_InitStruct->NbData             = 0x00000000U;
    M2M_InitStruct->SrcAddress         = 0x00000000U;
    M2M_InitStruct->DstAddress         = 0x00000000U;
}

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#endif /* M2M */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
