/**
 *****************************************************************************
 * @file   rn_rv_ll_spi.c
 * @author Renergy Software Team
 * @brief  SPI LL module driver.
 *
 *****************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 *****************************************************************************
 */
#if defined(USE_FULL_LL_DRIVER)

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_ll_spi.h"
#include "rn_rv_ll_system_core0.h"
#include "rn_rv_ll_system_core1.h"

#ifdef  USE_FULL_ASSERT
#include "rn_rv_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (SPI0) || defined (SPI1) || defined (SPI2) || defined (SPI3)

/** @addtogroup SPI_LL SPI
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/** @defgroup SPI_LL_Private_Constants SPI Private Constants
 * @{
 */
/* SPI registers Masks */
#define SPI_CTRL_CLEAR_MASK                 (SPI_CTRL_MAST_SLAV | SPI_CTRL_WIDTH  | SPI_CTRL_CPOL | \
                                             SPI_CTRL_CPHA      | SPI_CTRL_CLKDIV | SPI_CTRL_LMSB)
/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @addtogroup SPI_LL_Private_Macros SPI Private Macros
 * @{
 */

#define IS_LL_SPI_MODE(__VALUE__) (((__VALUE__) == LL_SPI_MODE_MASTER) \
                                   || ((__VALUE__) == LL_SPI_MODE_SLAVE))

#define IS_LL_SPI_DATAWIDTH(__VALUE__) (((__VALUE__) == LL_SPI_DATAWIDTH_8BIT) \
                                        || ((__VALUE__) == LL_SPI_DATAWIDTH_16BIT) \
                                        || ((__VALUE__) == LL_SPI_DATAWIDTH_32BIT))

#define IS_LL_SPI_POLARITY(__VALUE__) (((__VALUE__) == LL_SPI_POLARITY_LOW) \
                                       || ((__VALUE__) == LL_SPI_POLARITY_HIGH))

#define IS_LL_SPI_PHASE(__VALUE__) (((__VALUE__) == LL_SPI_PHASE_1EDGE) \
                                    || ((__VALUE__) == LL_SPI_PHASE_2EDGE))

#define IS_LL_SPI_CLOCKDIVISION(__VALUE__) ((__VALUE__) <= 0xFFU)

#define IS_LL_SPI_BITORDER(__VALUE__) (((__VALUE__) == LL_SPI_LSB_FIRST) \
                                       || ((__VALUE__) == LL_SPI_MSB_FIRST))

/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup SPI_LL_Exported_Functions
 * @{
 */

/** @addtogroup SPI_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize SPI registers (Registers restored to their default values).
 * @param  SPIx SPI Instance
 * @retval An ErrStatus enumeration value:
 *          - SUCCESS: SPI registers are de-initialized
 *          - ERROR:   SPI registers are not de-initialized
 */
ErrorStatus LL_SPI_DeInit(SPI_TypeDef *SPIx)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_SPI_ALL_INSTANCE(SPIx));

    if (SPIx == SPI0_CORE0) {
        /* Force reset of SPI clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_SPI0);

        /* Release reset of SPI clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_SPI0);
    } else if (SPIx == SPI1_CORE0) {
        /* Force reset of SPI clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_SPI1);

        /* Release reset of SPI clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_SPI1);
    } else if (SPIx == SPI2_CORE0) {
        /* Force reset of SPI clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_SPI2);

        /* Release reset of SPI clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_SPI2);
    } else if (SPIx == SPI3_CORE0) {
        /* Force reset of SPI clock */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_SPI3);

        /* Release reset of SPI clock */
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_SPI3);
    } else if (SPIx == SPI0_CORE1) {
        /* Force reset of SPI clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_SPI0);

        /* Release reset of SPI clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_SPI0);
    } else if (SPIx == SPI1_CORE1) {
        /* Force reset of SPI clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_SPI1);

        /* Release reset of SPI clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_SPI1);
    } else if (SPIx == SPI2_CORE1) {
        /* Force reset of SPI clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_SPI2);

        /* Release reset of SPI clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_SPI2);
    } else if (SPIx == SPI3_CORE1) {
        /* Force reset of SPI clock */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_SPI3);

        /* Release reset of SPI clock */
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_SPI3);
    } else {
        status = ERROR;
    }

    return status;
}

/**
 * @brief  Initialize SPI registers according to the specified
 *         parameters in SPI_InitStruct.
 * @param  SPIx SPI Instance
 * @param  SPI_InitStruct pointer to a @ref LL_SPI_InitTypeDef structure
 *         that contains the configuration information for the specified SPI peripheral.
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: SPI registers are initialized according to SPI_InitStruct content
 *          - ERROR: Problem occurred during SPI Registers initialization
 */
ErrorStatus LL_SPI_Init(SPI_TypeDef *SPIx, const LL_SPI_InitTypeDef *SPI_InitStruct)
{
    ErrorStatus status = ERROR;

    /* Check the SPI Instance SPIx*/
    assert_param(IS_SPI_ALL_INSTANCE(SPIx));

    /* Check the SPI parameters from SPI_InitStruct*/
    assert_param(IS_LL_SPI_MODE(SPI_InitStruct->Mode));
    assert_param(IS_LL_SPI_DATAWIDTH(SPI_InitStruct->DataWidth));
    assert_param(IS_LL_SPI_POLARITY(SPI_InitStruct->ClockPolarity));
    assert_param(IS_LL_SPI_PHASE(SPI_InitStruct->ClockPhase));
    assert_param(IS_LL_SPI_CLOCKDIVISION(SPI_InitStruct->ClockDivision));
    assert_param(IS_LL_SPI_BITORDER(SPI_InitStruct->BitOrder));

    if (LL_SPI_IsEnabled(SPIx) == 0x00000000U)
    {
        /*---------------------------- SPIx CTRL Configuration ------------------------
        * Configure SPIx CTRL with parameters:
        * - Master/Slave Mode:  SPI_CTRL_MAST_SLAV bit
        * - DataWidth:          SPI_CTRL_WIDTH bits
        * - ClockPolarity:      SPI_CTRL_CPOL bit
        * - ClockPhase:         SPI_CTRL_CPHA bit
        * - BaudRate prescaler: SPI_CTRL_CLKDIV bits
        * - BitOrder:           SPI_CTRL_LMSB bit
        */
        MODIFY_REG(SPIx->CTRL,
                SPI_CTRL_CLEAR_MASK,
                SPI_InitStruct->Mode | SPI_InitStruct->DataWidth |
                SPI_InitStruct->ClockPolarity | SPI_InitStruct->ClockPhase |
                SPI_InitStruct->ClockDivision | SPI_InitStruct->BitOrder);

        /* ENABLE SPIx peripheral */
        LL_SPI_Enable(SPIx);

        status = SUCCESS;
    }

  return status;
}

/**
 * @brief  Set each @ref LL_SPI_InitTypeDef field to default value.
 * @param  SPI_InitStruct pointer to a @ref LL_SPI_InitTypeDef structure
 *         whose fields will be set to default values.
 * @retval None
 */
void LL_SPI_StructInit(LL_SPI_InitTypeDef *SPI_InitStruct)
{
    /* Set SPI_InitStruct fields to default values */
    SPI_InitStruct->Mode              = LL_SPI_MODE_MASTER;
    SPI_InitStruct->DataWidth         = LL_SPI_DATAWIDTH_8BIT;
    SPI_InitStruct->ClockPolarity     = LL_SPI_POLARITY_LOW;
    SPI_InitStruct->ClockPhase        = LL_SPI_PHASE_1EDGE;
    SPI_InitStruct->ClockDivision     = 0U;
    SPI_InitStruct->BitOrder          = LL_SPI_MSB_FIRST;
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

#endif /* SPI0 || SPI1 || SPI2 || SPI3 */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
