/**
 *****************************************************************************
 * @file   rn_rv_ll_qspi.c
 * @author Renergy Software Team
 * @brief  QSPI LL module driver.
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
#include "rn_rv_ll_qspi.h"
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

#if defined (QSPI0) || defined (QSPI1)

/** @addtogroup QSPI_LL QSPI
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/** @defgroup QSPI_LL_Private_Constants QSPI Private Constants
 * @{
 */
/* QSPI registers Masks */
#define QSPI_CTRL_CLEAR_MASK                 (QSPI_CTRL_MAST_SLAV | QSPI_CTRL_WIDTH  | QSPI_CTRL_CPOL | \
                                              QSPI_CTRL_CPHA      | QSPI_CTRL_CLKDIV | QSPI_CTRL_LMSB)
/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @addtogroup QSPI_LL_Private_Macros QSPI Private Macros
 * @{
 */

#define IS_LL_QSPI_MODE(__VALUE__) (((__VALUE__) == LL_QSPI_MODE_MASTER) \
                                   || ((__VALUE__) == LL_QSPI_MODE_SLAVE))

#define IS_LL_QSPI_DATAWIDTH(__VALUE__) (((__VALUE__) == LL_QSPI_DATAWIDTH_8BIT) \
                                        || ((__VALUE__) == LL_QSPI_DATAWIDTH_16BIT) \
                                        || ((__VALUE__) == LL_QSPI_DATAWIDTH_32BIT))

#define IS_LL_QSPI_POLARITY(__VALUE__) (((__VALUE__) == LL_QSPI_POLARITY_LOW) \
                                       || ((__VALUE__) == LL_QSPI_POLARITY_HIGH))

#define IS_LL_QSPI_PHASE(__VALUE__) (((__VALUE__) == LL_QSPI_PHASE_1EDGE) \
                                    || ((__VALUE__) == LL_QSPI_PHASE_2EDGE))

#define IS_LL_QSPI_CLOCKDIVISION(__VALUE__) ((__VALUE__) <= 0xFFU)

#define IS_LL_QSPI_BITORDER(__VALUE__) (((__VALUE__) == LL_QSPI_BIT_LSB_FIRST) \
                                       || ((__VALUE__) == LL_QSPI_BIT_MSB_FIRST))

#define IS_LL_QSPI_BYTEORDER(__VALUE__) (((__VALUE__) == LL_QSPI_BYTE_LSB_FIRST) \
                                       || ((__VALUE__) == LL_QSPI_BYTE_MSB_FIRST))

/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup QSPI_LL_Exported_Functions
 * @{
 */

/** @addtogroup QSPI_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize QSPI registers (Registers restored to their default values).
 * @param  QSPIx QSPI Instance
 * @retval An ErrStatus enumeration value:
 *          - SUCCESS: QSPI registers are de-initialized
 *          - ERROR:   QSPI registers are not de-initialized
 */
ErrorStatus LL_QSPI_DeInit(QSPI_TypeDef *QSPIx)
{
    ErrorStatus status = SUCCESS;
    uint32_t core_id;

    /* Check the parameters */
    assert_param(IS_QSPI_ALL_INSTANCE(QSPIx));

    /* get run in core0 or core1 */
    core_id = __get_hart_id();

    if (QSPIx == QSPI0) {
        if (core_id == 0) {
            /* Force reset of QSPI clock */
            LL_SYSTEM_CORE0_MOD2_ForceReset(LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI0);

            /* Release reset of QSPI clock */
            LL_SYSTEM_CORE0_MOD2_ReleaseReset(LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI0);
        } else {
            /* Force reset of QSPI clock */
            LL_SYSTEM_CORE1_MOD2_ForceReset(LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI0);

            /* Release reset of QSPI clock */
            LL_SYSTEM_CORE1_MOD2_ReleaseReset(LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI0);
        }
    } else if (QSPIx == QSPI1) {
        if (core_id == 0) {
            /* Force reset of QSPI clock */
            LL_SYSTEM_CORE0_MOD2_ForceReset(LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI1);

            /* Release reset of QSPI clock */
            LL_SYSTEM_CORE0_MOD2_ReleaseReset(LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI1);
        } else {
            /* Force reset of QSPI clock */
            LL_SYSTEM_CORE1_MOD2_ForceReset(LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI1);

            /* Release reset of QSPI clock */
            LL_SYSTEM_CORE1_MOD2_ReleaseReset(LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI1);
        }
    } else {
        status = ERROR;
    }

    return status;
}

/**
 * @brief  Initialize QSPI registers according to the specified
 *         parameters in QSPI_InitStruct.
 * @param  QSPIx QSPI Instance
 * @param  QSPI_InitStruct pointer to a @ref LL_QSPI_InitTypeDef structure
 *         that contains the configuration information for the specified QSPI peripheral.
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: QSPI registers are initialized according to QSPI_InitStruct content
 *          - ERROR: Problem occurred during QSPI Registers initialization
 */
ErrorStatus LL_QSPI_Init(QSPI_TypeDef *QSPIx, const LL_QSPI_InitTypeDef *QSPI_InitStruct)
{
    ErrorStatus status = ERROR;

    /* Check the QSPI Instance QSPIx*/
    assert_param(IS_QSPI_ALL_INSTANCE(QSPIx));

    /* Check the QSPI parameters from QSPI_InitStruct*/
    assert_param(IS_LL_QSPI_MODE(QSPI_InitStruct->Mode));
    assert_param(IS_LL_QSPI_DATAWIDTH(QSPI_InitStruct->DataWidth));
    assert_param(IS_LL_QSPI_POLARITY(QSPI_InitStruct->ClockPolarity));
    assert_param(IS_LL_QSPI_PHASE(QSPI_InitStruct->ClockPhase));
    assert_param(IS_LL_QSPI_CLOCKDIVISION(QSPI_InitStruct->ClockDivision));
    assert_param(IS_LL_QSPI_BITORDER(QSPI_InitStruct->BitOrder));
    assert_param(IS_LL_QSPI_BITORDER(QSPI_InitStruct->ByteOrder));

    if (LL_QSPI_XIP_IsEnabled(QSPIx) == 0x00000000U)
    {
        /*---------------------------- QSPIx FMT Configuration ------------------------
        * Configure QSPIx FMT with parameters:
        * - DataWidth:          QSPI_FMT_LEN bits
        */
        LL_QSPI_SetDataWidth(QSPIx, QSPI_InitStruct->DataWidth);

        /*---------------------------- QSPIx SCKMODE Configuration ------------------------
        * Configure QSPIx SCKMODE with parameters:
            * - ClockPolarity:      QSPI_SCKMODE_CPOL bit
            * - ClockPhase:         QSPI_SCKMODE_CPHA bit
        */
        MODIFY_REG(QSPIx->SCKMODE,
                    QSPI_SCKMODE_CPOL | QSPI_SCKMODE_CPHA,
                    QSPI_InitStruct->ClockPolarity | QSPI_InitStruct->ClockPhase);

        /*---------------------------- QSPIx SCKDIV Configuration ------------------------
        * Configure QSPIx SCKDIV with parameters:
            * - BaudRate prescaler:      QSPI_SCKDIV_SCKDIV bit
        */
        LL_QSPI_SetBaudRatePrescaler(QSPIx, QSPI_InitStruct->ClockDivision);

        /*---------------------------- QSPIx FMT Configuration ------------------------
        * Configure QSPIx FMT with parameters:
            * - BitOrder:      QSPI_FMT_ENDIAN bit
            * - ByteOrder:     QSPI_FMT_ENDIAN_BYTE bit
        */
        MODIFY_REG(QSPIx->FMT,
                    QSPI_FMT_ENDIAN | QSPI_FMT_ENDIAN_BYTE,
                    QSPI_InitStruct->BitOrder | QSPI_InitStruct->ByteOrder);

        /*---------------------------- QSPIx CR Configuration ------------------------
        * Configure QSPIx CR with parameters:
        * - Master/Slave Mode:  QSPI_CR_MSTER bit
        */
        LL_QSPI_SetMode(QSPIx, QSPI_InitStruct->Mode);

        status = SUCCESS;
    }

  return status;
}

/**
 * @brief  Set each @ref LL_QSPI_InitTypeDef field to default value.
 * @param  QSPI_InitStruct pointer to a @ref LL_QSPI_InitTypeDef structure
 *         whose fields will be set to default values.
 * @retval None
 */
void LL_QSPI_StructInit(LL_QSPI_InitTypeDef *QSPI_InitStruct)
{
    /* Set QSPI_InitStruct fields to default values */
    QSPI_InitStruct->Mode              = LL_QSPI_MODE_MASTER;
    QSPI_InitStruct->DataWidth         = LL_QSPI_DATAWIDTH_8BIT;
    QSPI_InitStruct->ClockPolarity     = LL_QSPI_POLARITY_LOW;
    QSPI_InitStruct->ClockPhase        = LL_QSPI_PHASE_1EDGE;
    QSPI_InitStruct->ClockDivision     = 0U;
    QSPI_InitStruct->BitOrder          = LL_QSPI_BIT_MSB_FIRST;
    QSPI_InitStruct->ByteOrder         = LL_QSPI_BYTE_LSB_FIRST;
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

#endif /* QSPI0 || QSPI1 */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
