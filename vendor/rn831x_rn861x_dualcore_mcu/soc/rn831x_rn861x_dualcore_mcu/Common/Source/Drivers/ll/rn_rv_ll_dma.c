/**
 ******************************************************************************
 * @file   rn_rv_ll_dma.c
 * @author Renergy Software Team
 * @brief  DMA LL module driver
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
#include "rn_rv_ll_dma.h"
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

#if defined (DMA00) || defined (DMA10) || defined (DMA01) || defined (DMA11)

/** @defgroup DMA_LL DMA
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup DMA_LL_Private_Macros
 * @{
 */

#define IS_LL_DMA_CHANNEL_TYPE(__VALUE__)       (((__VALUE__) == LL_DMA_CHANNEL_TYPE_NORMAL) || \
                                                 ((__VALUE__) == LL_DMA_CHANNEL_TYPE_M2M))

#define IS_LL_DMA_DIRECTION(__VALUE__)          (((__VALUE__) == LL_DMA_DIRECTION_PERIPH2MEMORY) || \
                                                 ((__VALUE__) == LL_DMA_DIRECTION_MEMORY2PERIPH))

#define IS_LL_DMA_MODE(__VALUE__)               (((__VALUE__) == LL_DMA_MODE_NORMAL) || \
                                                 ((__VALUE__) == LL_DMA_MODE_CIRCULAR))

#define IS_LL_DMA_MEMORYDATASIZE(__VALUE__)     (((__VALUE__) == LL_DMA_DATAALIGN_BYTE)      || \
                                                 ((__VALUE__) == LL_DMA_DATAALIGN_HALFWORD)  || \
                                                 ((__VALUE__) == LL_DMA_DATAALIGN_WORD))

#define IS_LL_DMA_PRIORITY(__VALUE__)           (((__VALUE__) == LL_DMA_PRIORITY_LOW)    || \
                                                 ((__VALUE__) == LL_DMA_PRIORITY_MEDIUM) || \
                                                 ((__VALUE__) == LL_DMA_PRIORITY_HIGH)   || \
                                                 ((__VALUE__) == LL_DMA_PRIORITY_VERYHIGH))

#define IS_LL_DMA_ALL_CHANNEL_INSTANCE(INSTANCE, CHANNEL)  (((((INSTANCE) == DMA00) || \
                                                              ((INSTANCE) == DMA10)) && \
                                                            (((CHANNEL) == LL_DMA_CHANNEL_0) || \
                                                             ((CHANNEL) == LL_DMA_CHANNEL_1) || \
                                                             ((CHANNEL) == LL_DMA_CHANNEL_2) || \
                                                             ((CHANNEL) == LL_DMA_CHANNEL_3) || \
                                                             ((CHANNEL) == LL_DMA_CHANNEL_4))) || \
                                                            ((((INSTANCE) == DMA01) || \
                                                              ((INSTANCE) == DMA11)) && \
                                                            (((CHANNEL) == LL_DMA_CHANNEL_0) || \
                                                             ((CHANNEL) == LL_DMA_CHANNEL_1) || \
                                                             ((CHANNEL) == LL_DMA_CHANNEL_2) || \
                                                             ((CHANNEL) == LL_DMA_CHANNEL_3) || \
                                                             ((CHANNEL) == LL_DMA_CHANNEL_5) || \
                                                             ((CHANNEL) == LL_DMA_CHANNEL_6))))

/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup DMA_LL_Exported_Functions
 * @{
 */

/** @addtogroup DMA_LL_EF_Init
 * @{
 */

/**
 * @brief  Initialize the DMA registers according to the specified parameters in DMA_InitStruct.
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @param  DMA_InitStruct DMA_InitStruct instance
 * @retval ErrorStatus
 */
ErrorStatus LL_DMA_Init(DMA_TypeDef *DMAx, uint32_t Channel, LL_DMA_InitTypeDef *DMA_InitStruct)
{
    ErrorStatus status = ERROR;
    uint32_t channel_type = 0U;

    /* Check the parameters */
    assert_param(IS_DMA_ALL_INSTANCE(DMAx));
    assert_param(IS_LL_DMA_DIRECTION(DMA_InitStruct->Direction));
    assert_param(IS_LL_DMA_MODE(DMA_InitStruct->Mode));
    assert_param(IS_LL_DMA_PRIORITY(DMA_InitStruct->Priority));
    assert_param(IS_LL_DMA_ALL_CHANNEL_INSTANCE(DMAx, Channel));

    if ((Channel == LL_DMA_CHANNEL_5) || (Channel == LL_DMA_CHANNEL_6)) {
        channel_type = 1U;
    }

    if (channel_type == 0U) {
        /* Select the DMA Channel 0 ~ 4. */
        assert_param(IS_LL_DMA_MEMORYDATASIZE(DMA_InitStruct->PeriphDataAlignment));

        if (LL_DMA_IsEnabledChannel(DMAx, Channel)) {
            /* Note that Channel 4 is only valid in DMA0x(x=0,1). */

            /**
             * Sets the data transfer direction, data transfer mode, peripheral
             * and memory data alignment and priority level.
             */
            LL_DMA_ConfigTransfer(DMAx, Channel, (DMA_InitStruct->Mode | \
                                                DMA_InitStruct->Priority | \
                                                DMA_InitStruct->PeriphDataAlignment | \
                                                DMA_InitStruct->Direction));

            /* Set Number of data to transfer. */
            LL_DMA_SetDataLength(DMAx, Channel, DMA_InitStruct->NbData);

            /* Set the Memory address. */
            LL_DMA_SetMemoryAddress(DMAx, Channel, DMA_InitStruct->MemoryOrM2MDstAddress);

            /* Set the Peripheral address. */
            LL_DMA_SetPeriphAddress(DMAx, Channel, DMA_InitStruct->PeriphOrM2MSrcAddress);

            status = SUCCESS;
        }
    } else {
        /* Select the DMA Channel 5 ~ 6. */
        assert_param(IS_LL_DMA_MEMORYDATASIZE(DMA_InitStruct->PeriphDataAlignment));
        assert_param(IS_LL_DMA_MEMORYDATASIZE(DMA_InitStruct->MemDataAlignment));

        if (LL_DMA_IsEnabledM2MChannel(DMAx, Channel)) {
            /* Select the DMA M2M Channel 0 or Channel 1, only valid in DMA1x(x=0,1). */

            /**
             * Sets the data transfer mode, source and destination data size alignment,
             * and priority level.
             */
            LL_DMA_ConfigM2MTransfer(DMAx, Channel, (DMA_InitStruct->Mode | \
                                                    DMA_InitStruct->Priority | \
                                                    DMA_InitStruct->PeriphDataAlignment | \
                                                    DMA_InitStruct->MemDataAlignment));

            /* Set Number of data to transfer. */
            LL_DMA_SetM2MDataLength(DMAx, Channel, DMA_InitStruct->NbData);

            /* Set the Memory source address. */
            LL_DMA_SetM2MSrcAddress(DMAx, Channel, DMA_InitStruct->PeriphOrM2MSrcAddress);

            /* Set the Memory destination address. */
            LL_DMA_SetM2MDstAddress(DMAx, Channel, DMA_InitStruct->MemoryOrM2MDstAddress);

            status = SUCCESS; /* It means that the initialization is complete. */
        }
    }

    return status;
}

/**
 * @brief  De-initialize the DMA registers to their default reset values.
 * @param  DMAx DMA instance
 * @retval ErrorStatus
 */
ErrorStatus LL_DMA_DeInit(DMA_TypeDef *DMAx)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_DMA_ALL_INSTANCE(DMAx));

    /* Reset DMA register */
    if (DMAx == DMA00) {
        LL_SYSTEM_CORE0_PMOD0_ForceReset(LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GDMA00);
        LL_SYSTEM_CORE0_PMOD0_ReleaseReset(LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GDMA00);
    } else if (DMAx == DMA01) {
        LL_SYSTEM_CORE0_PMOD0_ForceReset(LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GDMA01);
        LL_SYSTEM_CORE0_PMOD0_ReleaseReset(LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GDMA01);
    } else if (DMAx == DMA10) {
        LL_SYSTEM_CORE1_PMOD0_ForceReset(LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_GDMA10);
        LL_SYSTEM_CORE1_PMOD0_ReleaseReset(LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_GDMA10);
    } else if (DMAx == DMA11) {
        LL_SYSTEM_CORE1_PMOD0_ForceReset(LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_GDMA11);
        LL_SYSTEM_CORE1_PMOD0_ReleaseReset(LL_SYSTEM_CORE1_PMOD0_RST_PERIPH_GDMA11);
    } else {
        status = ERROR;
    }

    return status;
}

/**
 * @brief  Set each @ref LL_DMA_InitTypeDef field to default value.
 * @param  DMA_InitStruct DMA_InitStruct instance
 * @retval None
 */
void LL_DMA_StructInit(LL_DMA_InitTypeDef *DMA_InitStruct)
{
    /* Set DMA_InitStruct fields to default values. */
    DMA_InitStruct->Mode                  = LL_DMA_MODE_NORMAL;
    DMA_InitStruct->Priority              = LL_DMA_PRIORITY_LOW;
    DMA_InitStruct->PeriphDataAlignment   = LL_DMA_DATAALIGN_BYTE;
    DMA_InitStruct->MemDataAlignment      = LL_DMA_DATAALIGN_BYTE;
    DMA_InitStruct->Direction             = LL_DMA_DIRECTION_PERIPH2MEMORY;
    DMA_InitStruct->NbData                = 0x00000000U;
    DMA_InitStruct->PeriphOrM2MSrcAddress = 0x00000000U;
    DMA_InitStruct->MemoryOrM2MDstAddress = 0x00000000U;
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

#endif /* DMA00 || DMA10 || DMA01 || DMA11 */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
