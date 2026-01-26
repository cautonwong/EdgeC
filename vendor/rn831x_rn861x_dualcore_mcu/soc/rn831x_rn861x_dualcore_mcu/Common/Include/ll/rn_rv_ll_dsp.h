/**
 *****************************************************************************
 * @file rn_rv_ll_dsp.h
 * @author Renergy Software Team
 * @brief Header file of DSP LL module.
 *
 *****************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 *****************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RN_LL_DSP_H
#define __RN_LL_DSP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_dsp.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined(DSP)

/** @defgroup DSP_LL DSP
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup DSP_LL_ES_INIT DSP Exported Init structure
 * @{
 */

/**
 * @brief LL DSP Init Structure definition
 */
typedef struct
{
    uint32_t DSPMode;         /*!< Specifies the calculate mode for the DSP.
                                     This parameter can be any value of @ref DSP_LL_EC_MODE */

    uint32_t I2FValidByte;    /*!< Specifies the valid byte for the I2F.
                                     This parameter can be any value of @ref DSP_LL_EC_VALIDBYTE */

    uint32_t RoundMode;       /*!< Specifies the round mode for the float point.
                                     This parameter can be any value of @ref DSP_LL_EC_ROUNDMODE */

} LL_DSP_InitTypeDef;
/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/* Exported constants -------------------------------------------------------*/
/** @defgroup DSP_LL_Exported_Constants DSP Exported Constants
 * @{
 */

/** @defgroup DSP_LL_EC_VALIDBYTE  I2F preprocess
 * @{
 */
#define LL_DSP_VALID_4BYTE      DSP_MAC_CTL0_I2F_PRE_EN_4BYTE             /*!< Select I2F input data 4 byte as available */
#define LL_DSP_VALID_3BYTE      DSP_MAC_CTL0_I2F_PRE_EN_3BYTE             /*!< Select I2F input data low 3 byte as available */
#define LL_DSP_VALID_2BYTE      DSP_MAC_CTL0_I2F_PRE_EN_2BYTE             /*!< Select I2F input data low 2 byte as available */
#define LL_DSP_VALID_1BYTE      DSP_MAC_CTL0_I2F_PRE_EN_1BYTE             /*!< Select I2F input data low 1 byte as available */
/**
 * @}
 */

/** @defgroup DSP_LL_EC_MULOUTFORMAT  float multiply output data format
 * @{
 */
#define LL_DSP_MULOUTFORMAT_FLOAT      DSP_MAC_CTL0_MUL_OUT_FM_FLOAT       //*!< Select float multiply output data format as float */
#define LL_DSP_MULOUTFORMAT_INT        DSP_MAC_CTL0_MUL_OUT_FM_INT         //*!< Select float multiply output data format as int */
/**
 * @}
 */

/** @defgroup DSP_LL_EC_MULINFORMAT  float multiply input data format
 * @{
 */
#define LL_DSP_MULINFORMAT_FLOAT      DSP_MAC_CTL0_MUL_IN_FM_FLOAT       //*!< Select float multiply data format as float */
#define LL_DSP_MULINFORMAT_INT        DSP_MAC_CTL0_MUL_IN_FM_INT         //*!< Select float multiply data format as int */
/**
 * @}
 */

/** @defgroup DSP_LL_EC_ROUNDMODE float round mode
 * @{
 */
#define LL_DSP_ROUNDMODE_NEAREST      DSP_MAC_CTL0_ROUND_MODE_NEAREST   /*!< Select round mode as nearest */
#define LL_DSP_ROUNDMODE_ZERO         DSP_MAC_CTL0_ROUND_MODE_ZERO      /*!< Select round mode as zero */
#define LL_DSP_ROUNDMODE_POS_INF      DSP_MAC_CTL0_ROUND_MODE_P_INF     /*!< Select round mode as positive infinity */
#define LL_DSP_ROUNDMODE_NEG_INF      DSP_MAC_CTL0_ROUND_MODE_N_INF     /*!< Select round mode as negative infinity */
#define LL_DSP_ROUNDMODE_UP           DSP_MAC_CTL0_ROUND_MODE_UP        /*!< Select round mode as nearst up */
#define LL_DSP_ROUNDMODE_AWAY         DSP_MAC_CTL0_ROUND_MODE_AWAY      /*!< Select round mode as away from zero */
/**
 * @}
 */

/** @defgroup DSP_LL_EC_MODE  DSP mode
 * @{
 */
#define LL_DSP_MODE_DISABLE           DSP_MAC_CTL0_MODE_SEL_DIS         /*!< Select disable all mode of MAC unit */
#define LL_DSP_MODE_I2F_ONCE          DSP_MAC_CTL0_MODE_SEL_I2F_ONCE    /*!< Select int to float conversion one-shot mode  */
#define LL_DSP_MODE_I2F_DMA           DSP_MAC_CTL0_MODE_SEL_I2F_DMA     /*!< Select int to float conversion DMA mode  */
#define LL_DSP_MODE_F2I_ONCE          DSP_MAC_CTL0_MODE_SEL_F2I_ONCE    /*!< Select float to int conversion one-shot mode  */
#define LL_DSP_MODE_F2I_DMA           DSP_MAC_CTL0_MODE_SEL_F2I_DMA     /*!< Select float to int conversion DMA mode  */
#define LL_DSP_MODE_FMUL              DSP_MAC_CTL0_MODE_SEL_FP_MUL      /*!< Select float multiply mode  */
#define LL_DSP_MODE_FADD              DSP_MAC_CTL0_MODE_SEL_FP_ADD      /*!< Select float addition mode  */
#define LL_DSP_MODE_FSUB              DSP_MAC_CTL0_MODE_SEL_FP_SUB      /*!< Select float subtraction mode  */
#define LL_DSP_MODE_FMULADD           DSP_MAC_CTL0_MODE_SEL_FP_MLAD     /*!< Select float Multiply-Accumulate mode  */
#define LL_DSP_MODE_BTFY_ONCE         DSP_MAC_CTL0_MODE_SEL_BTFY_ONCE   /*!< Select Butterfly operation unit one-shot mode  */
#define LL_DSP_MODE_BTFY_DMA          DSP_MAC_CTL0_MODE_SEL_BTFY_DMA    /*!< Select Butterfly operation unit DMA mode  */
#define LL_DSP_MODE_FFT               DSP_MAC_CTL0_MODE_SEL_FFT         /*!< Select FFT mode  */
#define LL_DSP_MODE_BITREVERSE        DSP_MAC_CTL0_MODE_SEL_BIT_RV      /*!< Select bit reverse mode  */
#define LL_DSP_MODE_IIR_ONCE          DSP_MAC_CTL0_MODE_SEL_IIR_ONCE    /*!< Select 2nd IIR filter one-shot mode  */
#define LL_DSP_MODE_IIR_DMA           DSP_MAC_CTL0_MODE_SEL_IIR_DMA     /*!< Select 2nd IIR filter DMA mode  */
#define LL_DSP_MODE_FIR_DMA           DSP_MAC_CTL0_MODE_SEL_FIR_DMA     /*!< Select FIR filter mode  */
#define LL_DSP_MODE_LINEAR            DSP_MAC_CTL0_MODE_SEL_LINE_INTP   /*!< Select linear interpolation mode  */
#define LL_DSP_MODE_LAGRANGE          DSP_MAC_CTL0_MODE_SEL_LAGR_INTP   /*!< Select Lagrange interpolation mode  */
/**
 * @}
 */

/** @defgroup DSP_LL_EC_DMASTEP  DMA data step
 * @{
 */
#define LL_DSP_DMASTEP_1              DSP_MAC_CTL1_DMA_STEP_STEP_1      /*!< Select DMA data step as 1 */
#define LL_DSP_DMASTEP_2              DSP_MAC_CTL1_DMA_STEP_STEP_2      /*!< Select DMA data step as 2 */
#define LL_DSP_DMASTEP_3              DSP_MAC_CTL1_DMA_STEP_STEP_3      /*!< Select DMA data step as 3 */
#define LL_DSP_DMASTEP_4              DSP_MAC_CTL1_DMA_STEP_STEP_4      /*!< Select DMA data step as 4 */
#define LL_DSP_DMASTEP_5              DSP_MAC_CTL1_DMA_STEP_STEP_5      /*!< Select DMA data step as 5 */
#define LL_DSP_DMASTEP_6              DSP_MAC_CTL1_DMA_STEP_STEP_6      /*!< Select DMA data step as 6 */
#define LL_DSP_DMASTEP_7              DSP_MAC_CTL1_DMA_STEP_STEP_7      /*!< Select DMA data step as 7 */
#define LL_DSP_DMASTEP_8              DSP_MAC_CTL1_DMA_STEP_STEP_8      /*!< Select DMA data step as 8 */
/**
 * @}
 */

/** @defgroup DSP_LL_EC_F2I_STATUS  Float-To-Int unit status
 * @{
 */
#define LL_DSP_F2I_STA_INT_TOOBIG         DSP_ALU_STA0_F2I_INT_TOOBIG      /*!< F2I int result after rounding has a magnitude greater than the largest representable two'scomplement integer with the same sign */
#define LL_DSP_F2I_STA_NOT_PRECISE        DSP_ALU_STA0_F2I_NOT_PRECISE     /*!< F2I int or float output is not equal to the infinitely precise result */
#define LL_DSP_F2I_STA_FLOAT_TOOBIG       DSP_ALU_STA0_F2I_FLOAT_TOOBIG    /*!< F2I float number greater than maximum normalized number */
#define LL_DSP_F2I_STA_FLOAT_TOOLESS      DSP_ALU_STA0_F2I_FLOAT_TOOLESS   /*!< F2I float number less than minimum normalized number */
#define LL_DSP_F2I_STA_INVALID            DSP_ALU_STA0_F2I_INVALID         /*!< F2I float operation is invalid or one input is NAN */
#define LL_DSP_F2I_STA_INFINTY            DSP_ALU_STA0_F2I_INFINTY         /*!< F2I float output is infinity */
#define LL_DSP_F2I_STA_ZERO               DSP_ALU_STA0_F2I_OUT0            /*!< F2I inte or Floating-Point output is zero */
/**
 * @}
 */

/** @defgroup DSP_LL_EC_I2F_STA  Int-To-Float unit status
 * @{
 */
#define LL_DSP_I2F_STA_INT_TOOBIG         DSP_ALU_STA0_I2F_INT_TOOBIG      /*!< I2F int result after rounding has a magnitude greater than the largest representable two'scomplement integer with the same sign */
#define LL_DSP_I2F_STA_NOT_PRECISE        DSP_ALU_STA0_I2F_NOT_PRECISE     /*!< I2F int or float output is not equal to the infinitely precise result */
#define LL_DSP_I2F_STA_FLOAT_TOOBIG       DSP_ALU_STA0_I2F_FLOAT_TOOBIG    /*!< I2F float number greater than maximum normalized number */
#define LL_DSP_I2F_STA_FLOAT_TOOLESS      DSP_ALU_STA0_I2F_FLOAT_TOOLESS   /*!< I2F float number less than minimum normalized number */
#define LL_DSP_I2F_STA_INVALID            DSP_ALU_STA0_I2F_INVALID         /*!< I2F float operation is invalid or one input is NAN */
#define LL_DSP_I2F_STA_INFINTY            DSP_ALU_STA0_I2F_INFINTY         /*!< I2F float output is infinity */
#define LL_DSP_I2F_STA_ZERO               DSP_ALU_STA0_I2F_OUT0            /*!< I2F inte or Floating-Point output is zero */
/**
 * @}
 */

/** @defgroup DSP_LL_EC_ADDSUB1_STA  Adder and subtraction unit1 status
 * @{
 */
#define LL_DSP_ADDSUB1_STA_INT_TOOBIG         DSP_ALU_STA0_ADDSUB1_INT_TOOBIG      /*!< ADDSUB1 int result after rounding has a magnitude greater than the largest representable two'scomplement integer with the same sign */
#define LL_DSP_ADDSUB1_STA_NOT_PRECISE        DSP_ALU_STA0_ADDSUB1_NOT_PRECISE     /*!< ADDSUB1 int or float output is not equal to the infinitely precise result */
#define LL_DSP_ADDSUB1_STA_FLOAT_TOOBIG       DSP_ALU_STA0_ADDSUB1_FLOAT_TOOBIG    /*!< ADDSUB1 float number greater than maximum normalized number */
#define LL_DSP_ADDSUB1_STA_FLOAT_TOOLESS      DSP_ALU_STA0_ADDSUB1_FLOAT_TOOLESS   /*!< ADDSUB1 float number less than minimum normalized number */
#define LL_DSP_ADDSUB1_STA_INVALID            DSP_ALU_STA0_ADDSUB1_INVALID         /*!< ADDSUB1 float operation is invalid or one input is NAN */
#define LL_DSP_ADDSUB1_STA_INFINTY            DSP_ALU_STA0_ADDSUB1_INFINTY         /*!< ADDSUB1 float output is infinity */
#define LL_DSP_ADDSUB1_STA_ZERO               DSP_ALU_STA0_ADDSUB1_OUT0            /*!< ADDSUB1 inte or Floating-Point output is zero */
/**
 * @}
 */

/** @defgroup DSP_LL_EC_ADDSUB0_STA  Adder and subtraction unit0 status
 * @{
 */
#define LL_DSP_ADDSUB0_STA_INT_TOOBIG         DSP_ALU_STA0_ADDSUB0_INT_TOOBIG      /*!< ADDSUB0 int result after rounding has a magnitude greater than the largest representable two'scomplement integer with the same sign */
#define LL_DSP_ADDSUB0_STA_NOT_PRECISE        DSP_ALU_STA0_ADDSUB0_NOT_PRECISE     /*!< ADDSUB0 int or float output is not equal to the infinitely precise result */
#define LL_DSP_ADDSUB0_STA_FLOAT_TOOBIG       DSP_ALU_STA0_ADDSUB0_FLOAT_TOOBIG    /*!< ADDSUB0 float number greater than maximum normalized number */
#define LL_DSP_ADDSUB0_STA_FLOAT_TOOLESS      DSP_ALU_STA0_ADDSUB0_FLOAT_TOOLESS   /*!< ADDSUB0 float number less than minimum normalized number */
#define LL_DSP_ADDSUB0_STA_INVALID            DSP_ALU_STA0_ADDSUB0_INVALID         /*!< ADDSUB0 float operation is invalid or one input is NAN */
#define LL_DSP_ADDSUB0_STA_INFINTY            DSP_ALU_STA0_ADDSUB0_INFINTY         /*!< ADDSUB0 float output is infinity */
#define LL_DSP_ADDSUB0_STA_ZERO               DSP_ALU_STA0_ADDSUB0_OUT0            /*!< ADDSUB0 inte or Floating-Point output is zero */
/**
 * @}
 */

/** @defgroup DSP_LL_EC_DIV_STA  Division status
 * @{
 */
#define LL_DSP_DIV_STA_INT_TOOBIG         DSP_ALU_STA0_MUL_INT_TOOBIG      /*!< DIV int result after rounding has a magnitude greater than the largest representable two'scomplement integer with the same sign */
#define LL_DSP_DIV_STA_NOT_PRECISE        DSP_ALU_STA0_MUL_NOT_PRECISE     /*!< DIV int or float output is not equal to the infinitely precise result */
#define LL_DSP_DIV_STA_FLOAT_TOOBIG       DSP_ALU_STA0_MUL_FLOAT_TOOBIG    /*!< DIV float number greater than maximum normalized number */
#define LL_DSP_DIV_STA_FLOAT_TOOLESS      DSP_ALU_STA0_MUL_FLOAT_TOOLESS   /*!< DIV float number less than minimum normalized number */
#define LL_DSP_DIV_STA_INVALID            DSP_ALU_STA0_MUL_INVALID         /*!< DIV float operation is invalid or one input is NAN */
#define LL_DSP_DIV_STA_INFINTY            DSP_ALU_STA0_MUL_INFINTY         /*!< DIV float output is infinity */
#define LL_DSP_DIV_STA_ZERO               DSP_ALU_STA0_MUL_OUT0            /*!< DIV inte or Floating-Point output is zero */
/**
 * @}
 */

/** @defgroup DSP_LL_EC_MUL_STA  Multiply unit status
 * @{
 */
#define LL_DSP_MUL_STA_INT_TOOBIG         DSP_ALU_STA0_MUL_INT_TOOBIG      /*!< MUL int result after rounding has a magnitude greater than the largest representable two'scomplement integer with the same sign */
#define LL_DSP_MUL_STA_NOT_PRECISE        DSP_ALU_STA0_MUL_NOT_PRECISE     /*!< MUL int or float output is not equal to the infinitely precise result */
#define LL_DSP_MUL_STA_FLOAT_TOOBIG       DSP_ALU_STA0_MUL_FLOAT_TOOBIG    /*!< MUL float number greater than maximum normalized number */
#define LL_DSP_MUL_STA_FLOAT_TOOLESS      DSP_ALU_STA0_MUL_FLOAT_TOOLESS   /*!< MUL float number less than minimum normalized number */
#define LL_DSP_MUL_STA_INVALID            DSP_ALU_STA0_MUL_INVALID         /*!< MUL float operation is invalid or one input is NAN */
#define LL_DSP_MUL_STA_INFINTY            DSP_ALU_STA0_MUL_INFINTY         /*!< MUL float output is infinity */
#define LL_DSP_MUL_STA_ZERO               DSP_ALU_STA0_MUL_OUT0            /*!< MUL inte or Floating-Point output is zero */
/**
 * @}
 */

/** @defgroup DSP_LL_EC_IE    DSP interrupt
 * @{
 */
#define LL_DSP_IE_LAGR_DMA                 DSP_IE_LAGR_DMA_IE              /*!< Interrupt of lagrange interp DMA */
#define LL_DSP_IE_LINE_DMA                 DSP_IE_LINE_DMA_IE              /*!< Interrupt of linear interp DMA */
#define LL_DSP_IE_FIR_DMA                  DSP_IE_FIR_DMA_IE               /*!< Interrupt of FIR DMA */
#define LL_DSP_IE_IIR_DMA                  DSP_IE_IIR_DMA_IE               /*!< Interrupt of IIR DMA */
#define LL_DSP_IE_IIR_ONCE                 DSP_IE_IIR_ONCE_IE              /*!< Interrupt of IIR once */
#define LL_DSP_IE_MUL                      DSP_IE_MULT_IE                  /*!< Interrupt of Multiply unit */
#define LL_DSP_IE_BITREV                   DSP_IE_BITREV_IE                /*!< Interrupt of bit reverse */
#define LL_DSP_IE_FFT                      DSP_IE_FFT_IE                   /*!< Interrupt of FFT */
#define LL_DSP_IE_BTFY_DMA                 DSP_IE_BTFY_DMA_IE              /*!< Interrupt of butterfly DMA */
#define LL_DSP_IE_BTFY_ONCE                DSP_IE_BTFY_ONCE_IE             /*!< Interrupt of butterfly once */
#define LL_DSP_IE_F2I_DMA                  DSP_IE_F2I_DMA_IE               /*!< Interrupt of Float-To-Int DMA */
#define LL_DSP_IE_I2F_DMA                  DSP_IE_I2F_DMA_IE               /*!< Interrupt of Int-To-Float DMA */
#define LL_DSP_IE_DIV                      DSP_IE_DIV_IE                   /*!< Interrupt of division */
#define LL_DSP_IE_DMA                      DSP_IE_DMA_IE                   /*!< Interrupt of DMA */
#define LL_DSP_IE_MAC                      DSP_IE_MAC_IE                   /*!< Interrupt of  Multiply-Accumulate */
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup DSP_LL_Exported_Functions DSP Exported Functions
 * @{
 */

/** @defgroup DSP_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief Configure Int-To-Float input data valid byte.
 * @note  Parameter ValidByte can be configured as LL_DSP_VALID_4BYTE/LL_DSP_VALID_3BYTE/LL_DSP_VALID_2BYTE/LL_DSP_VALID_1BYTE,
 *        represent 4 bytes valid / low 3 bytes valid / low 2 bytes valid / low 1 byte valid.
 * @rmtoll MAC_CTL0   I2F_PRE_EN   LL_DSP_SetI2FPreprocess
 * @param DSPx    DSP Instance
 * @param ValidByte This parameter can be one of the following values:
 *         @arg @ref LL_DSP_VALID_4BYTE
 *         @arg @ref LL_DSP_VALID_3BYTE
 *         @arg @ref LL_DSP_VALID_2BYTE
 *         @arg @ref LL_DSP_VALID_1BYTE
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetI2FPreprocess(DSP_TypeDef *DSPx, uint32_t ValidByte)
{
    MODIFY_REG(DSPx->MAC_CTL0, DSP_MAC_CTL0_I2F_PRE_EN, ValidByte);
}

/**
 * @brief Return Int-To-Float input data valid byte.
 * @rmtoll MAC_CTL0   I2F_PRE_EN   LL_DSP_GetI2FPreprocess
 * @param DSPx  DSP Instance
 * @return This parameter can be one of the following values:
 *         @arg @ref LL_DSP_VALID_4BYTE
 *         @arg @ref LL_DSP_VALID_3BYTE
 *         @arg @ref LL_DSP_VALID_2BYTE
 *         @arg @ref LL_DSP_VALID_1BYTE
 */
__STATIC_INLINE uint32_t LL_DSP_GetI2FPreprocess(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->MAC_CTL0, DSP_MAC_CTL0_I2F_PRE_EN));
}

/**
 * @brief Enable FFT internal table, only valid at 64 points and 128 points.
 * @rmtoll MAC_CTL0   FFT_TB_EN   LL_DSP_EnableFFTTable
 * @param DSPx    DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableFFTTable(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->MAC_CTL0, DSP_MAC_CTL0_FFT_TB_EN);
}

/**
 * @brief Disable FFT internal table.
 * @rmtoll MAC_CTL0   FFT_TB_EN   LL_DSP_DisableFFTTable
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableFFTTable(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->MAC_CTL0, DSP_MAC_CTL0_FFT_TB_EN);
}

/**
 * @brief Indicates whether the FFT internal table is enabled.
 * @rmtoll MAC_CTL0   FFT_TB_EN   LL_DSP_IsEnabledFFTTable
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledFFTTable(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->MAC_CTL0, DSP_MAC_CTL0_FFT_TB_EN) == DSP_MAC_CTL0_FFT_TB_EN) ? 1U : 0U);
}

/**
 * @brief Configure multiply order of floating point before Float-To-Int.
 * @rmtoll MAC_CTL0   F2I_MUL   LL_DSP_SetF2IMulOrder
 * @param DSPx    DSP Instance
 * @param Order   multiply order of floating point, fp_in = fp_in*2^Order, 0 <= MulOrder <= 31,
 *                 maximum value is 31
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetF2IMulOrder(DSP_TypeDef *DSPx, uint32_t Order)
{
    MODIFY_REG(DSPx->MAC_CTL0, DSP_MAC_CTL0_F2I_MUL, (Order << DSP_MAC_CTL0_F2I_MUL_POS));
}

/**
 * @brief Return multiply order before Float-To-Int.
 * @note  Input flaot data will be multiplied 2^Order.
 * @rmtoll MAC_CTL0   F2I_MUL   LL_DSP_GetF2IMulOrder
 * @param DSPx  DSP Instance
 * @return Magnification order
 */
__STATIC_INLINE uint32_t LL_DSP_GetF2IMulOrder(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->MAC_CTL0, DSP_MAC_CTL0_F2I_MUL) >> DSP_MAC_CTL0_F2I_MUL_POS);
}

/**
 * @brief Configure normalized order of floating point after Int-To-Float.
 * @note  Output flaot data will be divided by 2^Order.
 * @rmtoll MAC_CTL0   I2F_DIV   LL_DSP_SetI2FDivOrder
 * @param DSPx  DSP Instance
 * @param Order Normalized order of floating point, fp_out = fp_out / 2^MulOrder, 0 <= MulOrder <= 31,
 *                 maximum value is 31
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetI2FDivOrder(DSP_TypeDef *DSPx, uint32_t Order)
{
    MODIFY_REG(DSPx->MAC_CTL0, DSP_MAC_CTL0_I2F_DIV, (Order << DSP_MAC_CTL0_I2F_DIV_POS));
}

/**
 * @brief Return normalized order of floating point after Int-To-Float.
 * @rmtoll MAC_CTL0   I2F_DIV   LL_DSP_GetI2FDivOrder
 * @param DSPx  DSP Instance
 * @return Normalized order
 */
__STATIC_INLINE uint32_t LL_DSP_GetI2FDivOrder(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->MAC_CTL0, DSP_MAC_CTL0_I2F_DIV) >> DSP_MAC_CTL0_I2F_DIV_POS);
}

/**
 * @brief Configure round mode of floating point.
 * @note  Refer to datasheet for RoundMode of floating point.
 * @rmtoll MAC_CTL0   ROUND_MODE   LL_DSP_SetRoundMode
 * @param DSPx    DSP Instance
 * @param RoundMode This parameter can be one of the following values:
 *         @arg @ref LL_DSP_ROUNDMODE_NEAREST
 *         @arg @ref LL_DSP_ROUNDMODE_ZERO
 *         @arg @ref LL_DSP_ROUNDMODE_POS_INF
 *         @arg @ref LL_DSP_ROUNDMODE_NEG_INF
 *         @arg @ref LL_DSP_ROUNDMODE_UP
 *         @arg @ref LL_DSP_ROUNDMODE_AWAY
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetRoundMode(DSP_TypeDef *DSPx, uint32_t RoundMode)
{
    MODIFY_REG(DSPx->MAC_CTL0, DSP_MAC_CTL0_ROUND_MODE, RoundMode);
}

/**
 * @brief Return round mode of floating point.
 * @rmtoll MAC_CTL0   ROUND_MODE   LL_DSP_GetRoundMode
 * @param DSPx  DSP Instance
 * @return Returned value can be one of the following values:
 *         @arg @ref LL_DSP_ROUNDMODE_NEAREST
 *         @arg @ref LL_DSP_ROUNDMODE_ZERO
 *         @arg @ref LL_DSP_ROUNDMODE_POS_INF
 *         @arg @ref LL_DSP_ROUNDMODE_NEG_INF
 *         @arg @ref LL_DSP_ROUNDMODE_UP
 *         @arg @ref LL_DSP_ROUNDMODE_AWAY
 */
__STATIC_INLINE uint32_t LL_DSP_GetRoundMode(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->MAC_CTL0, DSP_MAC_CTL0_ROUND_MODE));
}

/**
 * @brief Configure output data format for Multiply unit.
 * @rmtoll MAC_CTL0   MUL_OUT_FM   LL_DSP_SetMulOutputFormat
 * @param DSPx    DSP Instance
 * @param Format This parameter can be one of the following values:
 *         @arg @ref LL_DSP_MULOUTFORMAT_FLOAT
 *         @arg @ref LL_DSP_MULOUTFORMAT_INT
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetMulOutputFormat(DSP_TypeDef *DSPx, uint32_t Format)
{
    MODIFY_REG(DSPx->MAC_CTL0, DSP_MAC_CTL0_MUL_OUT_FM, Format);
}

/**
 * @brief Return output data format for Multiply unit.
 * @rmtoll MAC_CTL0   MUL_OUT_FM   LL_DSP_GetMulOutputFormat
 * @param DSPx  DSP Instance
 * @return Returned value can be one of the following values:
 *         @arg @ref LL_DSP_MULOUTFORMAT_FLOAT
 *         @arg @ref LL_DSP_MULOUTFORMAT_INT
 */
__STATIC_INLINE uint32_t LL_DSP_GetMulOutputFormat(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->MAC_CTL0, DSP_MAC_CTL0_MUL_OUT_FM));
}

/**
 * @brief Configure input data format for Multiply unit.
 * @rmtoll MAC_CTL0   MUL_IN_FM   LL_DSP_SetMulInputFormat
 * @param DSPx    DSP Instance
 * @param Format This parameter can be one of the following values:
 *         @arg @ref LL_DSP_MULINFORMAT_FLOAT
 *         @arg @ref LL_DSP_MULINFORMAT_INT
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetMulInputFormat(DSP_TypeDef *DSPx, uint32_t Format)
{
    MODIFY_REG(DSPx->MAC_CTL0, DSP_MAC_CTL0_MUL_IN_FM, Format);
}

/**
 * @brief Return input data format for Multiply unit.
 * @rmtoll MAC_CTL0   MUL_IN_FM   LL_DSP_GetMulInputFormat
 * @param DSPx  DSP Instance
 * @return Returned value can be one of the following values:
 *         @arg @ref LL_DSP_MULINFORMAT_FLOAT
 *         @arg @ref LL_DSP_MULINFORMAT_INT
 */
__STATIC_INLINE uint32_t LL_DSP_GetMulInputFormat(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->MAC_CTL0, DSP_MAC_CTL0_MUL_IN_FM));
}

/**
 * @brief Configure calculate mode of DSP.
 * @rmtoll MAC_CTL0   MODE_SEL   LL_DSP_SetDSPMode
 * @param DSPx DSP Instance
 * @param Mode This parameter can be one of the following values:
 *         @arg @ref LL_DSP_MODE_DISABLE
 *         @arg @ref LL_DSP_MODE_I2F_ONCE
 *         @arg @ref LL_DSP_MODE_I2F_DMA
 *         @arg @ref LL_DSP_MODE_F2I_ONCE
 *         @arg @ref LL_DSP_MODE_F2I_DMA
 *         @arg @ref LL_DSP_MODE_FMUL
 *         @arg @ref LL_DSP_MODE_FADD
 *         @arg @ref LL_DSP_MODE_FSUB
 *         @arg @ref LL_DSP_MODE_FMULADD
 *         @arg @ref LL_DSP_MODE_BTFY_ONCE
 *         @arg @ref LL_DSP_MODE_BTFY_DMA
 *         @arg @ref LL_DSP_MODE_FFT
 *         @arg @ref LL_DSP_MODE_BITREVERSE
 *         @arg @ref LL_DSP_MODE_IIR_ONCE
 *         @arg @ref LL_DSP_MODE_IIR_DMA
 *         @arg @ref LL_DSP_MODE_FIR_DMA
 *         @arg @ref LL_DSP_MODE_LINEAR
 *         @arg @ref LL_DSP_MODE_LAGRANGE
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetDSPMode(DSP_TypeDef *DSPx, uint32_t Mode)
{
    MODIFY_REG(DSPx->MAC_CTL0, DSP_MAC_CTL0_MODE_SEL, Mode);
}

/**
 * @brief Return calculate mode of DSP.
 * @rmtoll MAC_CTL0   MODE_SEL   LL_DSP_GetDSPMode
 * @param DSPx  DSP Instance
 * @return Returned value can be one of the following values:
 *         @arg @ref LL_DSP_MODE_DISABLE
 *         @arg @ref LL_DSP_MODE_I2F_ONCE
 *         @arg @ref LL_DSP_MODE_I2F_DMA
 *         @arg @ref LL_DSP_MODE_F2I_ONCE
 *         @arg @ref LL_DSP_MODE_F2I_DMA
 *         @arg @ref LL_DSP_MODE_FMUL
 *         @arg @ref LL_DSP_MODE_FADD
 *         @arg @ref LL_DSP_MODE_FSUB
 *         @arg @ref LL_DSP_MODE_FMULADD
 *         @arg @ref LL_DSP_MODE_BTFY_ONCE
 *         @arg @ref LL_DSP_MODE_BTFY_DMA
 *         @arg @ref LL_DSP_MODE_FFT
 *         @arg @ref LL_DSP_MODE_BITREVERSE
 *         @arg @ref LL_DSP_MODE_IIR_ONCE
 *         @arg @ref LL_DSP_MODE_IIR_DMA
 *         @arg @ref LL_DSP_MODE_FIR_DMA
 *         @arg @ref LL_DSP_MODE_LINEAR
 *         @arg @ref LL_DSP_MODE_LAGRANGE
 */
__STATIC_INLINE uint32_t LL_DSP_GetDSPMode(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->MAC_CTL0, DSP_MAC_CTL0_MODE_SEL));
}

/**
 * @brief Configure the order of FIR filter.
 * @rmtoll MAC_CTL1   FIR_ORDER   LL_DSP_SetFIROrder
 * @param DSPx DSP Instance
 * @param Order FIR filter order, 0~3: order 3, 4: order 4, 5: order 5, ... , 255: order 255,
 *              maximum value is 255
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetFIROrder(DSP_TypeDef *DSPx, uint32_t Order)
{
    MODIFY_REG(DSPx->MAC_CTL1, DSP_MAC_CTL1_FIR_ORDER, (Order << DSP_MAC_CTL1_FIR_ORDER_POS));
}

/**
 * @brief Return the order of FIR filter.
 * @rmtoll MAC_CTL1   FIR_ORDER   LL_DSP_GetFIROrder
 * @param DSPx  DSP Instance
 * @return Order of FIR.
 */
__STATIC_INLINE uint32_t LL_DSP_GetFIROrder(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->MAC_CTL1, DSP_MAC_CTL1_FIR_ORDER) >> DSP_MAC_CTL1_FIR_ORDER_POS);
}

/**
 * @brief Configure Step of DMA read data from SRAM.
 * @note  Only available for IIR/FIR DMA mode.
 * @rmtoll MAC_CTL1   DMA_STEP   LL_DSP_SetFilterDMAStep
 * @param DSPx DSP Instance
 * @param Step This parameter can be one of the following values:
 *         @arg @ref LL_DSP_DMASTEP_1
 *         @arg @ref LL_DSP_DMASTEP_2
 *         @arg @ref LL_DSP_DMASTEP_3
 *         @arg @ref LL_DSP_DMASTEP_4
 *         @arg @ref LL_DSP_DMASTEP_5
 *         @arg @ref LL_DSP_DMASTEP_6
 *         @arg @ref LL_DSP_DMASTEP_7
 *         @arg @ref LL_DSP_DMASTEP_8
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetFilterDMAStep(DSP_TypeDef *DSPx, uint32_t Step)
{
    MODIFY_REG(DSPx->MAC_CTL1, DSP_MAC_CTL1_DMA_STEP, Step);
}

/**
 * @brief Return the Step of DMA read data from SRAM.
 * @note  Only available for IIR/FIR DMA mode.
 * @rmtoll MAC_CTL1   DMA_STEP   LL_DSP_GetFilterDMAStep
 * @param DSPx  DSP Instance
 * @return Returned value can be one of the following values:
 *         @arg @ref LL_DSP_DMASTEP_1
 *         @arg @ref LL_DSP_DMASTEP_2
 *         @arg @ref LL_DSP_DMASTEP_3
 *         @arg @ref LL_DSP_DMASTEP_4
 *         @arg @ref LL_DSP_DMASTEP_5
 *         @arg @ref LL_DSP_DMASTEP_6
 *         @arg @ref LL_DSP_DMASTEP_7
 *         @arg @ref LL_DSP_DMASTEP_8
 */
__STATIC_INLINE uint32_t LL_DSP_GetFilterDMAStep(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->MAC_CTL1, DSP_MAC_CTL1_DMA_STEP));
}

/**
 * @brief Enable Interpolation mode for using last data of last period.
 * @rmtoll MAC_CTL1   INTP_LAST_EN   LL_DSP_EnableInterpLastData
 * @param DSPx    DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableInterpLastData(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->MAC_CTL1, DSP_MAC_CTL1_INTP_LAST_EN);
}

/**
 * @brief Disable Interpolation mode for using last data of last period.
 * @rmtoll MAC_CTL1   INTP_LAST_EN   LL_DSP_DisableInterpLastData
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableInterpLastData(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->MAC_CTL1, DSP_MAC_CTL1_INTP_LAST_EN);
}

/**
 * @brief Indicates whether using last data of last period of interpolation mode is enabled.
 * @rmtoll MAC_CTL1   INTP_LAST_EN   LL_DSP_IsEnabledInterpLastData
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledInterpLastData(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->MAC_CTL1, DSP_MAC_CTL1_INTP_LAST_EN) == DSP_MAC_CTL1_INTP_LAST_EN) ? 1U : 0U);
}

/**
 * @brief Enable Flaoting-Point divider
 * @note  The register write only.
 * @rmtoll MAC_CTL2   DIV_KICK   LL_DSP_EnableFpDiv
 * @param DSPx    DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableFpDiv(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->MAC_CTL2, DSP_MAC_CTL2_DIV_KICK);
}

/**
 * @brief Disable Flaoting-Point divider
 * @note  The register write only.
 * @rmtoll MAC_CTL2   DIV_KICK   LL_DSP_DisableFpDiv
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableFpDiv(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->MAC_CTL2, DSP_MAC_CTL2_DIV_KICK);
}

/**
 * @brief Enable butterfly once mode, write only
 * @rmtoll MAC_CTL2   BTFY_ONCE_KICK   LL_DSP_EnableBTFYONCE
 * @param DSPx    DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableBTFYONCE(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->MAC_CTL2, DSP_MAC_CTL2_BTFY_ONCE_KICK);
}

/**
 * @brief Disable butterfly once mode, write only
 * @rmtoll MAC_CTL2   BTFY_ONCE_KICK   LL_DSP_DisableBTFYONCE
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableBTFYONCE(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->MAC_CTL2, DSP_MAC_CTL2_BTFY_ONCE_KICK);
}

/**
 * @brief Enable DMA of DSP.
 * @note  All DMA related operation are enabled through this register bit,
 *        and this bit will be cleared after DMA completion.
 * @rmtoll MAC_CTL2   DMA_EN   LL_DSP_EnableDMA
 * @param DSPx    DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableDMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->MAC_CTL2, DSP_MAC_CTL2_DMA_EN);
}

/**
 * @brief Disable DMA of DSP.
 * @note  All DMA related operation are enabled through this register bit,
 *        and this bit will be cleared after DMA completion.
 * @rmtoll MAC_CTL2   DMA_EN   LL_DSP_DisableInterpLastData
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableDMA(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->MAC_CTL2, DSP_MAC_CTL2_DMA_EN);
}

/**
 * @brief Indicates whether DMA of DSP is enabled.
 * @note  All DMA related operation are enabled through this register bit,
 *        and this bit will be cleared after DMA completion.
 * @rmtoll MAC_CTL2   DMA_EN   LL_DSP_IsEnabledInterpLastData
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledDMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->MAC_CTL2, DSP_MAC_CTL2_DMA_EN) == DSP_MAC_CTL2_DMA_EN) ? 1U : 0U);
}

/**
 * @brief Configure input0 of DSP.
 * @note  Input0 can be following data: input of Int-To-Float, Float-To-Int,
 *        parameter 0 of float multiply and addition, multiplier of Multiply-Accumulate, minuend of subtraction,
 *        real part of source 1 of the butterfly, current input data x[n] of IIR.
 * @rmtoll MAC_IN0   MAC_IN0   LL_DSP_SetInput0
 * @param DSPx DSP Instance
 * @param Data Data should be 32 bits
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetInput0(DSP_TypeDef *DSPx, uint32_t Data)
{
    WRITE_REG(DSPx->MAC_IN0, Data);
}

/**
 * @brief Return input0 data of DSP.
 * @note  Input0 can be following data: input of Int-To-Float, Float-To-Int,
 *        input0 of float multiply and addition, multiplier0 of Multiply-Accumulate, minuend of subtraction,
 *        real part of source 1 of the butterfly, current input data x[n] of IIR.
 * @rmtoll MAC_IN0   MAC_IN0   LL_DSP_GetInput0
 * @param DSPx  DSP Instance
 * @return input value
 */
__STATIC_INLINE uint32_t LL_DSP_GetInput0(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->MAC_IN0));
}

/**
 * @brief Configure input1 of DSP.
 * @note  Input1 can be following data: multiplier1 of Multiply-Accumulate, subtraction of subtraction,
 *        iamge part of source 1 of the butterfly, last input data x[n-1] of IIR.
 * @rmtoll MAC_IN1   MAC_IN1   LL_DSP_SetInput1
 * @param DSPx DSP Instance
 * @param Data Data should be 32 bits
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetInput1(DSP_TypeDef *DSPx, uint32_t Data)
{
    WRITE_REG(DSPx->MAC_IN1, Data);
}

/**
 * @brief Return input1 data of DSP.
 * @note  Input1 can be following data: multiplier1 of Multiply-Accumulate, subtraction of subtraction,
 *        iamge part of source 1 of the butterfly, last input data x[n-1] of IIR.
 * @rmtoll MAC_IN1   MAC_IN1   LL_DSP_GetInput1
 * @param DSPx  DSP Instance
 * @return input value
 */
__STATIC_INLINE uint32_t LL_DSP_GetInput1(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->MAC_IN1));
}

/**
 * @brief Configure input2 of DSP.
 * @note  Input2 can be following data: addend of Multiply-Accumulate, real part of source 2 of the butterfly,
 *        last input data x[n-2] of IIR.
 * @rmtoll MAC_IN2   MAC_IN2   LL_DSP_SetInput2
 * @param DSPx DSP Instance
 * @param Data Data should be 32 bits
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetInput2(DSP_TypeDef *DSPx, uint32_t Data)
{
    WRITE_REG(DSPx->MAC_IN2, Data);
}

/**
 * @brief Return input2 data of DSP.
 * @note  Input2 can be following data: addend of Multiply-Accumulate, real part of source 2 of the butterfly,
 *        last input data x[n-2] of IIR.
 * @rmtoll MAC_IN2   MAC_IN2   LL_DSP_GetInput2
 * @param DSPx  DSP Instance
 * @return input value
 */
__STATIC_INLINE uint32_t LL_DSP_GetInput2(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->MAC_IN2));
}

/**
 * @brief Configure input3 of DSP.
 * @note  Input3 can be the imag part of source 2 of the butterfly.
 * @rmtoll MAC_IN3   MAC_IN3   LL_DSP_SetInput3
 * @param DSPx DSP Instance
 * @param Data Data should be 32 bits
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetInput3(DSP_TypeDef *DSPx, uint32_t Data)
{
    WRITE_REG(DSPx->MAC_IN3, Data);
}

/**
 * @brief Return input3 data of DSP.
 * @note  Input3 can be the imag part of source 2 of the butterfly.
 * @rmtoll MAC_IN3   MAC_IN3   LL_DSP_GetInput3
 * @param DSPx  DSP Instance
 * @return input value
 */
__STATIC_INLINE uint32_t LL_DSP_GetInput3(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->MAC_IN3));
}

/**
 * @brief Configure input4 of DSP.
 * @note  Input4 can be the real part of table of the butterfly.
 * @rmtoll MAC_IN4   MAC_IN4   LL_DSP_SetInput4
 * @param DSPx DSP Instance
 * @param Data Data should be 32 bits
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetInput4(DSP_TypeDef *DSPx, uint32_t Data)
{
    WRITE_REG(DSPx->MAC_IN4, Data);
}

/**
 * @brief Return input4 data of DSP.
 * @note  Input4 can be the real part of table of the butterfly.
 * @rmtoll MAC_IN4   MAC_IN4   LL_DSP_GetInput4
 * @param DSPx  DSP Instance
 * @return input value
 */
__STATIC_INLINE uint32_t LL_DSP_GetInput4(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->MAC_IN4));
}

/**
 * @brief Configure input5 of DSP.
 * @note  Input5 can be the imag part of table of the butterfly.
 * @rmtoll MAC_IN5   MAC_IN5   LL_DSP_SetInput5
 * @param DSPx DSP Instance
 * @param Data Data should be 32 bits
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetInput5(DSP_TypeDef *DSPx, uint32_t Data)
{
    WRITE_REG(DSPx->MAC_IN5, Data);
}

/**
 * @brief Return input5 data of DSP.
 * @note  Input5 can be the imag part of table of the butterfly and the last data of interpolation.
 * @rmtoll MAC_IN5   MAC_IN5   LL_DSP_GetInput5
 * @param DSPx  DSP Instance
 * @return input value
 */
__STATIC_INLINE uint32_t LL_DSP_GetInput5(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->MAC_IN5));
}

/**
 * @brief Configure output0 of DSP.
 * @note  Output0 can be following data: result of Int-To-Float, Float-To-Int, float multiply, addition,
 *        Multiply-Accumulate and subtraction, result of data1 of the butterfly, result of data y[n] of IIR.
 * @rmtoll MAC_OUT0   MAC_OUT0   LL_DSP_SetOutput0
 * @param DSPx DSP Instance
 * @param Data Data should be 32 bits
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetOutput0(DSP_TypeDef *DSPx, uint32_t Data)
{
    WRITE_REG(DSPx->MAC_OUT0, Data);
}

/**
 * @brief Return output0 data of DSP.
 * @note  Output0 can be following data: result of Int-To-Float, Float-To-Int, float multiply, addition,
 *        Multiply-Accumulate and subtraction, result of the data1's real part of butterfly , result of data y[n] of IIR.
 * @rmtoll MAC_OUT0   MAC_OUT0   LL_DSP_GetOutput0
 * @param DSPx  DSP Instance
 * @return Output value
 */
__STATIC_INLINE uint32_t LL_DSP_GetOutput0(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->MAC_OUT0));
}

/**
 * @brief Configure output1 of DSP.
 * @note  Output1 is the result of the result of the data1's imag part of butterfly, result of data y[n-1] of IIR.
 * @rmtoll MAC_OUT1   MAC_OUT1   LL_DSP_SetOutput1
 * @param DSPx DSP Instance
 * @param Data Data should be 32 bits
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetOutput1(DSP_TypeDef *DSPx, uint32_t Data)
{
    WRITE_REG(DSPx->MAC_OUT1, Data);
}

/**
 * @brief Return output1 data of DSP.
 * @note  Output1 is the result of the result of the data1's imag part of butterfly and the result of data y[n-1] of IIR.
 * @rmtoll MAC_OUT1   MAC_OUT1   LL_DSP_GetOutput1
 * @param DSPx  DSP Instance
 * @return Output value
 */
__STATIC_INLINE uint32_t LL_DSP_GetOutput1(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->MAC_OUT1));
}

/**
 * @brief Return output2 data of DSP.
 * @note  Output2 is the result of the result of the data2's real part of butterfly.
 * @rmtoll MAC_OUT2   MAC_OUT2   LL_DSP_GetOutput2
 * @param DSPx  DSP Instance
 * @return Output value
 */
__STATIC_INLINE uint32_t LL_DSP_GetOutput2(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->MAC_OUT2));
}

/**
 * @brief Return output3 data of DSP.
 * @note  Output3 is the result of the result of the data2's imag part of butterfly.
 * @rmtoll MAC_OUT3   MAC_OUT3   LL_DSP_GetOutput3
 * @param DSPx  DSP Instance
 * @return Output value
 */
__STATIC_INLINE uint32_t LL_DSP_GetOutput3(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->MAC_OUT3));
}

/**
 * @brief Configure input0 of division unit.
 * @note  Input 0 can be the dividend of division uint and the input of parameter a1 of IIR.
 * @rmtoll DIV_IN0   DIV_IN0   LL_DSP_SetDivInput0
 * @param DSPx DSP Instance
 * @param Data Data should be 32 bits
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetDivInput0(DSP_TypeDef *DSPx, uint32_t Data)
{
    WRITE_REG(DSPx->DIV_IN0, Data);
}

/**
 * @brief Return input0 of division unit.
 * @note  Input 0 can be the dividend of division uint and the input of parameter a1 of IIR.
 * @rmtoll DIV_IN0   DIV_IN0   LL_DSP_GetDivInput0
 * @param DSPx  DSP Instance
 * @return Input value
 */
__STATIC_INLINE uint32_t LL_DSP_GetDivInput0(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->DIV_IN0));
}

/**
 * @brief Configure input 1 of division unit.
 * @note  Input 1 can be the divisor of division uint and the input of parameter a2 of IIR.
 * @rmtoll DIV_IN1   DIV_IN1   LL_DSP_SetDivInput1
 * @param DSPx DSP Instance
 * @param Data Data should be 32 bits
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetDivInput1(DSP_TypeDef *DSPx, uint32_t Data)
{
    WRITE_REG(DSPx->DIV_IN1, Data);
}

/**
 * @brief Return input 1 of division unit.
 * @note  Input 1 can be the divisor of division uint and the input of parameter a2 of IIR.
 * @rmtoll DIV_IN1   DIV_IN1   LL_DSP_GetDivInput1
 * @param DSPx  DSP Instance
 * @return Input value
 */
__STATIC_INLINE uint32_t LL_DSP_GetDivInput1(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->DIV_IN1));
}

/**
 * @brief Return output 0 of division unit.
 * @note  Output 0 is the output of quotient of division uint.
 * @rmtoll DIV_OUT0   DIV_OUT0   LL_DSP_GetDivOutput0
 * @param DSPx  DSP Instance
 * @return Output value
 */
__STATIC_INLINE uint32_t LL_DSP_GetDivOutput0(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->DIV_OUT0));
}

/**
 * @brief Configure source data address of DMA.
 * @note  Address input can be the source address of real part of FFT/BTFY_DMA modes,
 *        source address of I2F_DMA/F2I_DMA/BIT_REVERSE modes, input data x[n] address of IIR_DMA/FIR_DMA/LINE_DMA/LAGR_DMA modes.
 * @note  Word-aligned
 * @rmtoll DMA_SRBADR   DMA_SRBADR   LL_DSP_SetDMASourceRealAddr
 * @param DSPx DSP Instance
 * @param Address Source data address of DMA, address is 15 bits.
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetDMASourceRealAddr(DSP_TypeDef *DSPx, uint32_t Address)
{
    WRITE_REG(DSPx->DMA_SRBADR, (DSP_DMA_SRBADR_DMA_SRBADR & Address));
}

/**
 * @brief Return source data address of DMA.
 * @note  Address input can be the source address of real part of FFT/BTFY_DMA modes,
 *        source address of I2F_DMA/F2I_DMA/BIT_REVERSE modes, input data x[n] address of IIR_DMA/FIR_DMA/LINE_DMA/LAGR_DMA modes.
 * @note  Word-aligned
 * @rmtoll DMA_SRBADR   DMA_SRBADR   LL_DSP_GetDMASourceRealAddr
 * @param DSPx  DSP Instance
 * @return source data address of DMA.
 */
__STATIC_INLINE uint32_t LL_DSP_GetDMASourceRealAddr(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->DMA_SRBADR) & DSP_DMA_SRBADR_DMA_SRBADR);
}

/**
 * @brief Configure source data address of DMA.
 * @note  Address input is the source address of imag part of FFT/BTFY_DMA modes.
 * @note  Word-aligned
 * @rmtoll DMA_SIBADR   DMA_SIBADR   LL_DSP_SetDMASourceImagAddr
 * @param DSPx DSP Instance
 * @param Address Source data address of DMA, address is 15 bits.
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetDMASourceImagAddr(DSP_TypeDef *DSPx, uint32_t Address)
{
    WRITE_REG(DSPx->DMA_SIBADR, (Address & DSP_DMA_SIBADR_DMA_SIBADR));
}

/**
 * @brief Return source data address of DMA.
 * @note  Address input is the source address of imag part of FFT/BTFY_DMA modes.
 * @note  Word-aligned
 * @rmtoll DMA_SIBADR   DMA_SIBADR   LL_DSP_GetDMASourceImagAddr
 * @param DSPx  DSP Instance
 * @return source data address of DMA
 */
__STATIC_INLINE uint32_t LL_DSP_GetDMASourceImagAddr(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->DMA_SIBADR) & DSP_DMA_SIBADR_DMA_SIBADR);
}

/**
 * @brief Configure parameter address of DMA.
 * @note  Address input can be the real part address of twiddle factor's parameter table(cosine address of table)
 *        of FFT/BTFY_DMA modes and the address of parameter a of FIR_DMA mode.
 * @note  Word-aligned
 * @rmtoll DMA_PRBADR   DMA_PRBADR   LL_DSP_SetDMAParaRealAddr
 * @param DSPx DSP Instance
 * @param Address parameter address of DMA, address is 15 bits.
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetDMAParaRealAddr(DSP_TypeDef *DSPx, uint32_t Address)
{
    WRITE_REG(DSPx->DMA_PRBADR, (Address & DSP_DMA_PRBADR_DMA_PRBADR));
}

/**
 * @brief Return parameter address of DMA.
 * @note  Address input can be the real part address of twiddle factor's parameter table(cosine address of table)
 *        of FFT/BTFY_DMA modes and the address of parameter a of FIR_DMA mode.
 * @note  Word-aligned
 * @rmtoll DMA_PRBADR   DMA_PRBADR   LL_DSP_GetDMAParaRealAddr
 * @param DSPx  DSP Instance
 * @return parameter address of DMA
 */
__STATIC_INLINE uint32_t LL_DSP_GetDMAParaRealAddr(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->DMA_PRBADR) & DSP_DMA_PRBADR_DMA_PRBADR);
}

/**
 * @brief Configure parameter address of DMA.
 * @note  Address input is the imag part address of twiddle factor's parameter table(sine address of table)
 *        of FFT/BTFY_DMA modes.
 * @note  Word-aligned
 * @rmtoll DMA_PIBADR   DMA_PIBADR   LL_DSP_SetDMAParaImagAddr
 * @param DSPx DSP Instance
 * @param Address parameter address of DMA, address is 15 bits.
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetDMAParaImagAddr(DSP_TypeDef *DSPx, uint32_t Address)
{
    WRITE_REG(DSPx->DMA_PIBADR, (Address & DSP_DMA_PIBADR_DMA_PIBADR));
}

/**
 * @brief Return parameter address of DMA.
 * @note  Address input can be the imag part address of twiddle factor's parameter table(sine address of table)
 *        of FFT/BTFY_DMA modes.
 * @note  Word-aligned
 * @rmtoll DMA_PIBADR   DMA_PIBADR   LL_DSP_GetDMAParaImagAddr
 * @param DSPx  DSP Instance
 * @return parameter address of DMA
 */
__STATIC_INLINE uint32_t LL_DSP_GetDMAParaImagAddr(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->DMA_PIBADR) & DSP_DMA_PIBADR_DMA_PIBADR);
}

/**
 * @brief Configure target address of DMA.
 * @note  Address input can be the target address of real part of FFT/BTFY_DMA modes, target address of
 *        I2F_DMA/F2I_DMA/BIT_REVERSE modes, output data y[n] address of IIR_DMA/FIR_DMA/LINE_DMA/LAGR_DMA modes.
 * @note  Word-aligned
 * @rmtoll DMA_TRBADR   DMA_TRBADR   LL_DSP_SetDMATargetRealAddr
 * @param DSPx DSP Instance
 * @param Address Target address of DMA, address is 15 bits.
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetDMATargetRealAddr(DSP_TypeDef *DSPx, uint32_t Address)
{
    WRITE_REG(DSPx->DMA_TRBADR, (Address & DSP_DMA_TRBADR_DMA_TRBADR));
}

/**
 * @brief Return target address of DMA.
 * @note  Address input can be the target address of real part of FFT/BTFY_DMA modes, target address of
 *        I2F_DMA/F2I_DMA/BIT_REVERSE modes, output data y[n] address of IIR_DMA/FIR_DMA/LINE_DMA/LAGR_DMA modes.
 * @note  Word-aligned
 * @rmtoll DMA_TRBADR   DMA_TRBADR   LL_DSP_GetDMATargetRealAddr
 * @param DSPx  DSP Instance
 * @return Target address of DMA
 */
__STATIC_INLINE uint32_t LL_DSP_GetDMATargetRealAddr(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->DMA_TRBADR) & DSP_DMA_TRBADR_DMA_TRBADR);
}

/**
 * @brief Configure target address of DMA.
 * @note  Address input can be the target address of imag part of FFT/BTFY_DMA modes.
 * @note  Word-aligned
 * @rmtoll DMA_TIBADR   DMA_TIBADR   LL_DSP_SetDMATargetImagAddr
 * @param DSPx DSP Instance
 * @param Address Target address of DMA, address is 15 bits.
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetDMATargetImagAddr(DSP_TypeDef *DSPx, uint32_t Address)
{
    WRITE_REG(DSPx->DMA_TIBADR, (Address & DSP_DMA_TIBADR_DMA_TIBADR));
}

/**
 * @brief Return target address of DMA.
 * @note  Address input can be the target address of imag part of FFT/BTFY_DMA modes.
 * @note  Word-aligned
 * @rmtoll DMA_TIBADR   DMA_TIBADR   LL_DSP_GetDMATargetImagAddr
 * @param DSPx  DSP Instance
 * @return Target address of DMA
 */
__STATIC_INLINE uint32_t LL_DSP_GetDMATargetImagAddr(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->DMA_TIBADR) & DSP_DMA_TIBADR_DMA_TIBADR);
}

/**
 * @brief Configure data length of DMA.
 * @note  Length points configuration, DMA_LEN = length + 1,
 *        I2F_DMA/F2I_DMA/BTFY_DMA modes: support any length configuration
 *        BIT_REVERSE mode: only support valid points: 4、8、16、32、64、128、256、512、1024.
 *        FFT mode: only support valid points: 64、128、256、512、1024.
 * @rmtoll DMA_LEN   DMA_LEN   LL_DSP_SetDMADataLength
 * @param DSPx DSP Instance
 * @param Length Data length is 10 bits.
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetDMADataLength(DSP_TypeDef *DSPx, uint32_t Length)
{
    WRITE_REG(DSPx->DMA_LEN, (Length & DSP_DMA_LEN_DMA_LEN));
}

/**
 * @brief Return data length of DMA.
 * @note  Length points configuration, DMA_LEN = length + 1,
 *        I2F_DMA/F2I_DMA/BTFY_DMA modes: support any length configuration
 *        BIT_REVERSE mode: only support valid points: 4、8、16、32、64、128、256、512、1024.
 *        FFT mode: only support valid points: 64、128、256、512、1024.
 * @rmtoll DMA_LEN   DMA_LEN   LL_DSP_GetDMADataLength
 * @param DSPx  DSP Instance
 * @return Data length of DMA
 */
__STATIC_INLINE uint32_t LL_DSP_GetDMADataLength(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->DMA_LEN) & DSP_DMA_LEN_DMA_LEN);
}

/**
 * @brief Enable lagrange interp DMA interrupt.
 * @rmtoll IE   LAGR_DMA_IE   LL_DSP_EnableIT_LAGRDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_LAGRDMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_LAGR_DMA_IE);
}

/**
 * @brief Disable lagrange interp DMA interrupt.
 * @rmtoll IE   LAGR_DMA_IE   LL_DSP_DisableIT_PowerCover
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_LAGRDMA(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_LAGR_DMA_IE);
}

/**
 * @brief Indicates whether the lagrange interp DMA interrupt is enabled.
 * @rmtoll IE   LAGR_DMA_IE   LL_DSP_IsEnabledIT_LAGRDMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_LAGRDMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_LAGR_DMA_IE) == DSP_IE_LAGR_DMA_IE));
}

/**
 * @brief Enable linear interp DMA interrupt.
 * @rmtoll IE   LINE_DMA_IE   LL_DSP_EnableIT_LINEDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_LINEDMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_LINE_DMA_IE);
}

/**
 * @brief Disable linear interp DMA interrupt.
 * @rmtoll IE   LINE_DMA_IE   LL_DSP_DisableIT_LINEDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_LINEDMA(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_LINE_DMA_IE);
}

/**
 * @brief Indicates whether the linear interp DMA interrupt is enabled.
 * @rmtoll IE   LINE_DMA_IE   LL_DSP_IsEnabledIT_LINEDMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_LINEDMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_LINE_DMA_IE) == DSP_IE_LINE_DMA_IE));
}

/**
 * @brief Enable FIR filter DMA interrupt.
 * @rmtoll IE   FIR_DMA_IE   LL_DSP_EnableIT_FIRDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_FIRDMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_FIR_DMA_IE);
}

/**
 * @brief Disable FIR filter DMA interrupt.
 * @rmtoll IE   FIR_DMA_IE   LL_DSP_DisableIT_FIRDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_FIRDMA(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_FIR_DMA_IE);
}

/**
 * @brief Indicates whether the FIR filter DMA interrupt is enabled.
 * @rmtoll IE   FIR_DMA_IE   LL_DSP_IsEnabledIT_FIRDMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_FIRDMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_FIR_DMA_IE) == DSP_IE_FIR_DMA_IE));
}

/**
 * @brief Enable IIR filter DMA interrupt.
 * @rmtoll IE   IIR_DMA_IE   LL_DSP_EnableIT_IIRDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_IIRDMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_IIR_DMA_IE);
}

/**
 * @brief Disable IIR filter DMA interrupt.
 * @rmtoll IE   FIR_DMA_IE   LL_DSP_DisableIT_IIRDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_IIRDMA(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_IIR_DMA_IE);
}

/**
 * @brief Indicates whether the IIR filter DMA interrupt is enabled.
 * @rmtoll IE   IIR_DMA_IE   LL_DSP_IsEnabledIT_IIRDMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_IIRDMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_IIR_DMA_IE) == DSP_IE_IIR_DMA_IE));
}

/**
 * @brief Enable once IIR filter interrupt.
 * @rmtoll IE   IIR_ONCE_IE   LL_DSP_EnableIT_IIRONCE
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_IIRONCE(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_IIR_ONCE_IE);
}

/**
 * @brief Disable once IIR filter interrupt.
 * @rmtoll IE   IIR_ONCE_IE   LL_DSP_DisableIT_IIRONCE
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_IIRONCE(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_IIR_ONCE_IE);
}

/**
 * @brief Indicates whether the once IIR filter interrupt is enabled.
 * @rmtoll IE   IIR_ONCE_IE   LL_DSP_IsEnabledIT_IIRONCE
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_IIRONCE(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_IIR_ONCE_IE) == DSP_IE_IIR_ONCE_IE));
}

/**
 * @brief Enable float multiply interrupt.
 * @rmtoll IE   IIR_ONCE_IE   LL_DSP_EnableIT_MUL
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_MUL(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_MULT_IE);
}

/**
 * @brief Disable float multiply interrupt.
 * @rmtoll IE   IIR_ONCE_IE   LL_DSP_DisableIT_MUL
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_MUL(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_MULT_IE);
}

/**
 * @brief Indicates whether the float multiply interrupt is enabled.
 * @rmtoll IE   IIR_ONCE_IE   LL_DSP_IsEnabledIT_MUL
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_MUL(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_MULT_IE) == DSP_IE_MULT_IE));
}

/**
 * @brief Enable bit reverse interrupt.
 * @rmtoll IE   BITREV_IE   LL_DSP_EnableIT_BITREV
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_BITREV(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_BITREV_IE);
}

/**
 * @brief Disable bit reverse interrupt.
 * @rmtoll IE   BITREV_IE   LL_DSP_DisableIT_BITREV
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_BITREV(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_BITREV_IE);
}

/**
 * @brief Indicates whether the bit reverse interrupt is enabled.
 * @rmtoll IE   BITREV_IE   LL_DSP_IsEnabledIT_BITREV
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_BITREV(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_BITREV_IE) == DSP_IE_BITREV_IE));
}

/**
 * @brief Enable FFT interrupt.
 * @rmtoll IE   FFT_IE   LL_DSP_EnableIT_FFT
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_FFT(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_FFT_IE);
}

/**
 * @brief Disable FFT interrupt.
 * @rmtoll IE   FFT_IE   LL_DSP_DisableIT_FFT
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_FFT(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_FFT_IE);
}

/**
 * @brief Indicates whether the FFT interrupt is enabled.
 * @rmtoll IE   FFT_IE   LL_DSP_IsEnabledIT_FFT
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_FFT(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_FFT_IE) == DSP_IE_FFT_IE));
}

/**
 * @brief Enable Butterfly DMA interrupt.
 * @rmtoll IE   BTFY_DMA_IE   LL_DSP_EnableIT_BTFYDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_BTFYDMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_BTFY_DMA_IE);
}

/**
 * @brief Disable Butterfly DMA interrupt.
 * @rmtoll IE   BTFY_DMA_IE   LL_DSP_DisableIT_BTFYDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_BTFYDMA(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_BTFY_DMA_IE);
}

/**
 * @brief Indicates whether the Butterfly DMA interrupt is enabled.
 * @rmtoll IE   BTFY_DMA_IE   LL_DSP_IsEnabledIT_BTFYDMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_BTFYDMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_BTFY_DMA_IE) == DSP_IE_BTFY_DMA_IE));
}

/**
 * @brief Enable once Butterfly interrupt.
 * @rmtoll IE   BTFY_ONCE_IE   LL_DSP_EnableIT_BTFYONCE
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_BTFYONCE(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_BTFY_ONCE_IE);
}

/**
 * @brief Disable once Butterfly interrupt.
 * @rmtoll IE   BTFY_ONCE_IE   LL_DSP_DisableIT_BTFYONCE
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_BTFYONCE(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_BTFY_ONCE_IE);
}

/**
 * @brief Indicates whether the once Butterfly interrupt is enabled.
 * @rmtoll IE   BTFY_ONCE_IE   LL_DSP_IsEnabledIT_BTFYONCE
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_BTFYONCE(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_BTFY_ONCE_IE) == DSP_IE_BTFY_ONCE_IE));
}

/**
 * @brief Enable Float-To-Int DMA interrupt.
 * @rmtoll IE   F2I_DMA_IE   LL_DSP_EnableIT_F2IDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_F2IDMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_F2I_DMA_IE);
}

/**
 * @brief Disable Float-To-Int DMA interrupt.
 * @rmtoll IE   F2I_DMA_IE   LL_DSP_DisableIT_F2IDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_F2IDMA(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_F2I_DMA_IE);
}

/**
 * @brief Indicates whether the Float-To-Int DMA interrupt is enabled.
 * @rmtoll IE   F2I_DMA_IE   LL_DSP_IsEnabledIT_F2IDMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_F2IDMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_F2I_DMA_IE) == DSP_IE_F2I_DMA_IE));
}

/**
 * @brief Enable Int-To-Float DMA interrupt.
 * @rmtoll IE   I2F_DMA_IE   LL_DSP_EnableIT_I2FDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_I2FDMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_I2F_DMA_IE);
}

/**
 * @brief Disable Int-To-Float DMA interrupt.
 * @rmtoll IE   I2F_DMA_IE   LL_DSP_DisableIT_I2FDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_I2FDMA(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_I2F_DMA_IE);
}

/**
 * @brief Indicates whether the Int-To-Float DMA interrupt is enabled.
 * @rmtoll IE   I2F_DMA_IE   LL_DSP_IsEnabledIT_I2FDMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_I2FDMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_I2F_DMA_IE) == DSP_IE_I2F_DMA_IE));
}

/**
 * @brief Enable Flaoting-Point divider interrupt.
 * @rmtoll IE   DIV_IE   LL_DSP_EnableIT_DIV
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_DIV(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_DIV_IE);
}

/**
 * @brief Disable Flaoting-Point divider interrupt.
 * @rmtoll IE   DIV_IE   LL_DSP_DisableIT_DIV
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_DIV(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_DIV_IE);
}

/**
 * @brief Indicates whether the Flaoting-Point divider interrupt is enabled.
 * @rmtoll IE   DIV_IE   LL_DSP_IsEnabledIT_DIV
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_DIV(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_DIV_IE) == DSP_IE_DIV_IE));
}

/**
 * @brief Enable DMA interrupt.
 * @rmtoll IE   DMA_IE   LL_DSP_EnableIT_DMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_DMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_DMA_IE);
}

/**
 * @brief Disable DMA interrupt.
 * @rmtoll IE   DMA_IE   LL_DSP_DisableIT_DMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_DMA(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_DMA_IE);
}

/**
 * @brief Indicates whether the DMA interrupt is enabled.
 * @rmtoll IE   DMA_IE   LL_DSP_IsEnabledIT_DMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_DMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_DMA_IE) == DSP_IE_DMA_IE));
}

/**
 * @brief Enable float Multiply-Accumulate unit interrupt.
 * @rmtoll IE   MAC_IE   LL_DSP_EnableIT_MAC
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_MAC(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->IE, DSP_IE_MAC_IE);
}

/**
 * @brief Disable float Multiply-Accumulate unit interrupt.
 * @rmtoll IE   MAC_IE   LL_DSP_DisableIT_MAC
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_MAC(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->IE, DSP_IE_MAC_IE);
}

/**
 * @brief Indicates whether the float Multiply-Accumulate unit interrupt is enabled.
 * @rmtoll IE   MAC_IE   LL_DSP_IsEnabledIT_MAC
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_MAC(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->IE, DSP_IE_MAC_IE) == DSP_IE_MAC_IE));
}

/**
 * @brief Indicates whether the lagrange interp complete interrupt flag is set.
 * @rmtoll FLG   LAGR_DMA_DONE   LL_DSP_IsActiveFlag_LAGR
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_LAGR(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_LAGR_DMA_DONE) == DSP_FLG_LAGR_DMA_DONE));
}

/**
 * @brief Clear lagrange interp interrupt and flag.
 * @rmtoll FLG   LAGR_DMA_DONE   LL_DSP_ClearFlag_LAGR
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_LAGR(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_LAGR_DMA_DONE);
}

/**
 * @brief Indicates whether the linear interp complete interrupt flag is set.
 * @rmtoll FLG   LINE_DMA_DONE   LL_DSP_IsActiveFlag_LINE
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_LINE(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_LINE_DMA_DONE) == DSP_FLG_LINE_DMA_DONE));
}

/**
 * @brief Clear linear interp interrupt and flag.
 * @rmtoll FLG   LINE_DMA_DONE   LL_DSP_ClearFlag_LINE
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_LINE(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_LINE_DMA_DONE);
}

/**
 * @brief Indicates whether the FIR DMA complete interrupt flag is set.
 * @rmtoll FLG   FIR_DMA_DONE   LL_DSP_IsActiveFlag_FIRDMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_FIRDMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_FIR_DMA_DONE) == DSP_FLG_FIR_DMA_DONE));
}

/**
 * @brief Clear FIR DMA interrupt and flag.
 * @rmtoll FLG   FIR_DMA_DONE   LL_DSP_ClearFlag_FIRDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_FIRDMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_FIR_DMA_DONE);
}

/**
 * @brief Indicates whether the IIR DMA complete interrupt flag is set.
 * @rmtoll FLG   IIR_DMA_DONE   LL_DSP_IsActiveFlag_IIRDMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_IIRDMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_IIR_DMA_DONE) == DSP_FLG_IIR_DMA_DONE));
}

/**
 * @brief Clear IIR DMA interrupt and flag.
 * @rmtoll FLG   IIR_DMA_DONE   LL_DSP_ClearFlag_IIRDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_IIRDMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_IIR_DMA_DONE);
}

/**
 * @brief Indicates whether the IIR once complete interrupt flag is set.
 * @rmtoll FLG   IIR_ONCE_DONE   LL_DSP_IsActiveFlag_IIRONCE
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_IIRONCE(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_IIR_ONCE_DONE) == DSP_FLG_IIR_ONCE_DONE));
}

/**
 * @brief Clear IIR once interrupt and flag.
 * @rmtoll FLG   IIR_ONCE_DONE   LL_DSP_ClearFlag_IIRONCE
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_IIRONCE(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_IIR_ONCE_DONE);
}

/**
 * @brief Indicates whether the Floating-Point Multiplier complete interrupt flag is set.
 * @rmtoll FLG   MULT_DONE   LL_DSP_IsActiveFlag_MUL
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_MUL(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_MULT_DONE) == DSP_FLG_MULT_DONE));
}

/**
 * @brief Clear the Floating-Point Multiplier interrupt and flag.
 * @rmtoll FLG   MULT_DONE   LL_DSP_ClearFlag_MUL
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_MUL(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_MULT_DONE);
}

/**
 * @brief Indicates whether the bit reverse complete interrupt flag is set.
 * @rmtoll FLG   BITREV_DONE   LL_DSP_IsActiveFlag_BITREV
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_BITREV(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_BITREV_DONE) == DSP_FLG_BITREV_DONE));
}

/**
 * @brief Clear the bit reverse interrupt and flag.
 * @rmtoll FLG   BITREV_DONE   LL_DSP_ClearFlag_BITREV
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_BITREV(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_BITREV_DONE);
}

/**
 * @brief Indicates whether the FFT complete interrupt flag is set.
 * @rmtoll FLG   FFT_DONE   LL_DSP_IsActiveFlag_FFT
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_FFT(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_FFT_DONE) == DSP_FLG_FFT_DONE));
}

/**
 * @brief Clear the FFT interrupt and flag.
 * @rmtoll FLG   FFT_DONE   LL_DSP_ClearFlag_FFT
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_FFT(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_FFT_DONE);
}

/**
 * @brief Indicates whether the butterfly DMA complete interrupt flag is set.
 * @rmtoll FLG   BTFY_DMA_DONE   LL_DSP_IsActiveFlag_BTFYDMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_BTFYDMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_BTFY_DMA_DONE) == DSP_FLG_BTFY_DMA_DONE));
}

/**
 * @brief Clear the butterfly DMA interrupt and flag.
 * @rmtoll FLG   BTFY_DMA_DONE   LL_DSP_ClearFlag_BTFYDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_BTFYDMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_BTFY_DMA_DONE);
}

/**
 * @brief Indicates whether the butterfly once complete interrupt flag is set.
 * @rmtoll FLG   BTFY_ONCE_DONE   LL_DSP_IsActiveFlag_BTFYDMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_BTFYONCE(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_BTFY_ONCE_DONE) == DSP_FLG_BTFY_ONCE_DONE));
}

/**
 * @brief Clear the butterfly once interrupt and flag.
 * @rmtoll FLG   BTFY_ONCE_DONE   LL_DSP_ClearFlag_BTFYDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_BTFYONCE(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_BTFY_ONCE_DONE);
}

/**
 * @brief Indicates whether the Float-To-Int DMA complete interrupt flag is set.
 * @rmtoll FLG   F2I_DMA_DONE   LL_DSP_IsActiveFlag_F2IDMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_F2IDMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_F2I_DMA_DONE) == DSP_FLG_F2I_DMA_DONE));
}

/**
 * @brief Clear the Float-To-Int DMA interrupt and flag.
 * @rmtoll FLG   F2I_DMA_DONE   LL_DSP_ClearFlag_F2IDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_F2IDMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_F2I_DMA_DONE);
}

/**
 * @brief Indicates whether the Int-To-Float DMA complete interrupt flag is set.
 * @rmtoll FLG   I2F_DMA_DONE   LL_DSP_IsActiveFlag_I2FDMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_I2FDMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_I2F_DMA_DONE) == DSP_FLG_I2F_DMA_DONE));
}

/**
 * @brief Clear the Int-To-Float DMA interrupt and flag.
 * @rmtoll FLG   I2F_DMA_DONE   LL_DSP_ClearFlag_I2FDMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_I2FDMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_I2F_DMA_DONE);
}

/**
 * @brief Indicates whether the Floating-Point divider complete interrupt flag is set.
 * @rmtoll FLG   DIV_DONE   LL_DSP_IsActiveFlag_DIV
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_DIV(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_DIV_DONE) == DSP_FLG_DIV_DONE));
}

/**
 * @brief Clear the Floating-Point divider interrupt and flag.
 * @rmtoll FLG   DIV_DONE   LL_DSP_ClearFlag_DIV
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_DIV(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_DIV_DONE);
}

/**
 * @brief Indicates whether the DMA complete interrupt flag is set.
 * @rmtoll FLG   DMA_DONE   LL_DSP_IsActiveFlag_DMA
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_DMA(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_DMA_DONE) == DSP_FLG_DMA_DONE));
}

/**
 * @brief Clear the DMA interrupt and flag.
 * @rmtoll FLG   DMA_DONE   LL_DSP_ClearFlag_DMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_DMA(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_DMA_DONE);
}

/**
 * @brief Indicates whether the Multiply-Accumulate complete interrupt flag is set.
 * @rmtoll FLG   MAC_DONE   LL_DSP_IsActiveFlag_MAC
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_MAC(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->FLG, DSP_FLG_MAC_DONE) == DSP_FLG_MAC_DONE));
}

/**
 * @brief Clear the Multiply-Accumulate interrupt and flag.
 * @rmtoll FLG   MAC_DONE   LL_DSP_ClearFlag_DMA
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_MAC(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->FLG, DSP_FLG_MAC_DONE);
}

/**
 * @brief Clear DSP flag by ClearBit.
 * @rmtoll FLG   MAC_DONE   LL_DSP_ClearFlag
 * @param DSPx      DSP Instance
 * @param ClearBit  Bits to be cleared
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag(DSP_TypeDef *DSPx, uint32_t ClearBit)
{
    MODIFY_REG(DSPx->FLG, BITS(0, 14), ClearBit);
}

/**
 * @brief Get Float-To-Int unit status.
 * @note  Only support the following bits:
 *        bit30: LL_DSP_F2I_STA_INT_TOOBIG, Int result after rounding has a magnitude greater than the largest
 *               representable two's complement integer with the same sign.
 *        bit29: LL_DSP_F2I_STA_NOT_PRECISE, Int or float output is not equal to the infinitely precise result.
 *        bit27: LL_DSP_F2I_STA_FLOAT_TOOLESS, Float number less than minimum normalized number.
 *        bit26: LL_DSP_F2I_STA_INVALID, Float operation is invalid or one input is NAN.
 *        bit24: LL_DSP_F2I_STA_ZERO, Int or float point output is zero.
 * @rmtoll ALU_STA0   F2I_STATUS   LL_DSP_GetF2IStatus
 * @param DSPx  DSP Instance
 * @return Returned value can be a combination of the following values:
 *         @arg @ref LL_DSP_F2I_STA_INT_TOOBIG
 *         @arg @ref LL_DSP_F2I_STA_NOT_PRECISE
 *         @arg @ref LL_DSP_F2I_STA_FLOAT_TOOLESS
 *         @arg @ref LL_DSP_F2I_STA_INVALID
 *         @arg @ref LL_DSP_F2I_STA_ZERO
 */
__STATIC_INLINE uint32_t LL_DSP_GetF2IStatus(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->ALU_STA0, 0xFF000000U));
}

/**
 * @brief Get Int-To-Float unit status.
 * @note  Only support the following bits:
 *        bit21: LL_DSP_I2F_STA_NOT_PRECISE, Int or float output is not equal to the infinitely precise result.
 *        bit20: LL_DSP_I2F_STA_FLOAT_TOOBIG, Float number greater than maximum normalized number.
 *        bit17: LL_DSP_I2F_STA_INFINTY, Float output is infinity.
 *        bit16: LL_DSP_I2F_STA_ZERO, Int or float point output is zero.
 * @rmtoll ALU_STA0   I2F_STATUS   LL_DSP_GetI2FStatus
 * @param DSPx  DSP Instance
 * @return Returned value can be a combination of the following values:
 *         @arg @ref LL_DSP_I2F_STA_NOT_PRECISE
 *         @arg @ref LL_DSP_I2F_STA_FLOAT_TOOBIG
 *         @arg @ref LL_DSP_I2F_STA_INFINTY
 *         @arg @ref LL_DSP_I2F_STA_ZERO
 */
__STATIC_INLINE uint32_t LL_DSP_GetI2FStatus(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->ALU_STA0, 0x00FF0000U));
}

/**
 * @brief Get Floating-Point adder and subtraction unit1 status.
 * @note  Only support the following bits:
 *        bit13: LL_DSP_ADDSUB1_STA_NOT_PRECISE, Int or float output is not equal to the infinitely precise result.
 *        bit12: LL_DSP_ADDSUB1_STA_FLOAT_TOOBIG, Float number greater than maximum normalized number.
 *        bit11: LL_DSP_ADDSUB1_STA_FLOAT_TOOLESS, Float number less than minimum normalized number.
 *        bit10: LL_DSP_ADDSUB1_STA_INVALID, Float operation is invalid or one input is NAN.
 *        bit9:  LL_DSP_ADDSUB1_STA_INFINTY, Float output is infinity.
 *        bit8:  LL_DSP_ADDSUB1_STA_ZERO, Int or float point output is zero.
 * @rmtoll ALU_STA0   ADDSUB1_STATUS   LL_DSP_GetADDSUB1Status
 * @param DSPx  DSP Instance
 * @return Returned value can be a combination of the following values:
 *         @arg @ref LL_DSP_ADDSUB1_STA_NOT_PRECISE
 *         @arg @ref LL_DSP_ADDSUB1_STA_FLOAT_TOOBIG
 *         @arg @ref LL_DSP_ADDSUB1_STA_FLOAT_TOOLESS
 *         @arg @ref LL_DSP_ADDSUB1_STA_INVALID
 *         @arg @ref LL_DSP_ADDSUB1_STA_INFINTY
 *         @arg @ref LL_DSP_ADDSUB1_STA_ZERO
 */
__STATIC_INLINE uint32_t LL_DSP_GetADDSUB1Status(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->ALU_STA0, 0xFF00U));
}

/**
 * @brief Get Floating-Point adder and subtraction unit0 status.
 * @note  Only support the following bits:
 *        bit5: LL_DSP_ADDSUB0_STA_NOT_PRECISE, Int or float output is not equal to the infinitely precise result.
 *        bit4: LL_DSP_ADDSUB0_STA_FLOAT_TOOBIG, Float number greater than maximum normalized number.
 *        bit3: LL_DSP_ADDSUB0_STA_FLOAT_TOOLESS, Float number less than minimum normalized number.
 *        bit2: LL_DSP_ADDSUB0_STA_INVALID, Float operation is invalid or one input is NAN.
 *        bit1: LL_DSP_ADDSUB0_STA_INFINTY, Float output is infinity.
 *        bit0: LL_DSP_ADDSUB0_STA_ZERO, Int or float point output is zero.
 * @rmtoll ALU_STA0   ADDSUB0_STATUS   LL_DSP_GetADDSUB0Status
 * @param DSPx  DSP Instance
 * @return Returned value can be a combination of the following values:
 *         @arg @ref LL_DSP_ADDSUB0_STA_NOT_PRECISE
 *         @arg @ref LL_DSP_ADDSUB0_STA_FLOAT_TOOBIG
 *         @arg @ref LL_DSP_ADDSUB0_STA_FLOAT_TOOLESS
 *         @arg @ref LL_DSP_ADDSUB0_STA_INVALID
 *         @arg @ref LL_DSP_ADDSUB0_STA_INFINTY
 *         @arg @ref LL_DSP_ADDSUB0_STA_ZERO
 */
__STATIC_INLINE uint32_t LL_DSP_GetADDSUB0Status(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->ALU_STA0, 0xFFU));
}

/**
 * @brief Get Floating-point Divider unit status.
 * @note  Only support the following bits:
 *        bit13: LL_DSP_DIV_STA_NOT_PRECISE, Int or float output is not equal to the infinitely precise result.
 *        bit12: LL_DSP_DIV_STA_FLOAT_TOOBIG, Float number greater than maximum normalized number.
 *        bit11: LL_DSP_DIV_STA_FLOAT_TOOLESS, Float number less than minimum normalized number.
 *        bit10: LL_DSP_DIV_STA_INVALID, Float operation is invalid or one input is NAN.
 *        bit9:  LL_DSP_DIV_STA_INFINTY, Float output is infinity.
 *        bit8:  LL_DSP_DIV_STA_ZERO, Int or float point output is zero.
 * @rmtoll ALU_STA1   DIV_STATUS   LL_DSP_GetDividerStatus
 * @param DSPx  DSP Instance
 * @return Returned value can be a combination of the following values:
 *         @arg @ref LL_DSP_DIV_STA_NOT_PRECISE
 *         @arg @ref LL_DSP_DIV_STA_FLOAT_TOOBIG
 *         @arg @ref LL_DSP_DIV_STA_FLOAT_TOOLESS
 *         @arg @ref LL_DSP_DIV_STA_INVALID
 *         @arg @ref LL_DSP_DIV_STA_INFINTY
 *         @arg @ref LL_DSP_DIV_STA_ZERO
 */
__STATIC_INLINE uint32_t LL_DSP_GetDividerStatus(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->ALU_STA1, 0xFF00U));
}

/**
 * @brief Get Floating-point Multiplier unit status.
 * @note  Only support the following bits:
 *        bit13: LL_DSP_MUL_STA_NOT_PRECISE, Int or float output is not equal to the infinitely precise result.
 *        bit12: LL_DSP_MUL_STA_FLOAT_TOOBIG, Float number greater than maximum normalized number.
 *        bit11: LL_DSP_MUL_STA_FLOAT_TOOLESS, Float number less than minimum normalized number.
 *        bit10: LL_DSP_MUL_STA_INVALID, Float operation is invalid or one input is NAN.
 *        bit9:  LL_DSP_MUL_STA_INFINTY, Float output is infinity.
 *        bit8:  LL_DSP_MUL_STA_ZERO, Int or float point output is zero.
 * @rmtoll ALU_STA1   MUL_STATUS   LL_DSP_GetMultiplierStatus
 * @param DSPx  DSP Instance
 * @return Returned value can be a combination of the following values:
 *         @arg @ref LL_DSP_MUL_STA_NOT_PRECISE
 *         @arg @ref LL_DSP_MUL_STA_FLOAT_TOOBIG
 *         @arg @ref LL_DSP_MUL_STA_FLOAT_TOOLESS
 *         @arg @ref LL_DSP_MUL_STA_INVALID
 *         @arg @ref LL_DSP_MUL_STA_INFINTY
 *         @arg @ref LL_DSP_MUL_STA_ZERO
 */
__STATIC_INLINE uint32_t LL_DSP_GetMultiplierStatus(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_BIT(DSPx->ALU_STA1, 0xFFU));
}

/**
 * @brief Enable Rotation mode of CORDIC.
 * @rmtoll CRD_CTL   CRD_VEC_KICK   LL_DSP_EnableRotation
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableRotation(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->CRD_CTL, DSP_CRD_CTL_CRD_ROT_KICK);
}

/**
 * @brief Disable Rotation mode of CORDIC.
 * @rmtoll CRD_CTL   CRD_VEC_KICK   LL_DSP_DisableRotation
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableRotation(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->CRD_CTL, DSP_CRD_CTL_CRD_ROT_KICK);
}

/**
 * @brief Enable Vector mode of CORDIC.
 * @rmtoll CRD_CTL   CRD_VEC_KICK   LL_DSP_EnableVector
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableVector(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->CRD_CTL, DSP_CRD_CTL_CRD_VEC_KICK);
}

/**
 * @brief Disable Vector mode of CORDIC.
 * @rmtoll CRD_CTL   CRD_VEC_KICK   LL_DSP_DisableVector
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableVector(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->CRD_CTL, DSP_CRD_CTL_CRD_VEC_KICK);
}

/**
 * @brief Configure input data X of Vector mode of CORDIC.
 * @rmtoll CRD_XIN   X_IN   LL_DSP_SetVectorInputX
 * @param DSPx DSP Instance
 * @param Data Input data X which is 32 bits.
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetVectorInputX(DSP_TypeDef *DSPx, uint32_t Data)
{
    WRITE_REG(DSPx->CRD_XIN, Data);
}

/**
 * @brief Return input data X of Vector mode of CORDIC.
 * @rmtoll CRD_XIN   X_IN   LL_DSP_GetVectorInputX
 * @param DSPx  DSP Instance
 * @return Input data X
 */
__STATIC_INLINE uint32_t LL_DSP_GetVectorInputX(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->CRD_XIN));
}

/**
 * @brief Configure input data Y of Vector mode of CORDIC.
 * @rmtoll CRD_YIN   X_IN   LL_DSP_SetVectorInputY
 * @param DSPx DSP Instance
 * @param Data Input data Y which is 32 bits.
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetVectorInputY(DSP_TypeDef *DSPx, uint32_t Data)
{
    WRITE_REG(DSPx->CRD_YIN, Data);
}

/**
 * @brief Return input data Y of Vector mode of CORDIC.
 * @rmtoll CRD_YIN   Y_IN   LL_DSP_GetVectorInputY
 * @param DSPx  DSP Instance
 * @return Input data Y
 */
__STATIC_INLINE uint32_t LL_DSP_GetVectorInputY(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->CRD_YIN));
}

/**
 * @brief Return output amplitude of Vector mode of CORDIC.
 * @rmtoll CRD_AMP   AMP_OUT   LL_DSP_GetVectorAmplitude
 * @param DSPx  DSP Instance
 * @return Output amplitude
 */
__STATIC_INLINE uint32_t LL_DSP_GetVectorAmplitude(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->CRD_AMP));
}

/**
 * @brief Return output phase of Rotation mode of CORDIC.
 * @rmtoll CRD_PHASE   PHASE_OUT   LL_DSP_GetRotationPhase
 * @param DSPx  DSP Instance
 * @return Output amplitude
 */
__STATIC_INLINE uint32_t LL_DSP_GetRotationPhase(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->CRD_PHASE));
}

/**
 * @brief Configure input angle of Rotation mode of CORDIC.
 * @rmtoll CRD_ANGLE   CRD_ANGLE   LL_DSP_SetRotationAngle
 * @param DSPx DSP Instance
 * @param Radian Input angle which is radian, register's value = input*pi/(2^31),
 *               Radian = 2^31 represent pi, Radian = -2^31 represent -pi.
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetRotationAngle(DSP_TypeDef *DSPx, uint32_t Radian)
{
    WRITE_REG(DSPx->CRD_ANGLE, Radian);
}

/**
 * @brief Return input angle of Rotation mode of CORDIC.
 * @rmtoll CRD_ANGLE   CRD_ANGLE   LL_DSP_GetRotationAngle
 * @param DSPx  DSP Instance
 * @return Angle
 */
__STATIC_INLINE uint32_t LL_DSP_GetRotationAngle(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->CRD_ANGLE));
}

/**
 * @brief Return output cosine of Rotation mode of CORDIC.
 * @rmtoll CRD_COSINE   COSINE_OUT   LL_DSP_GetRotationCosine
 * @param DSPx  DSP Instance
 * @return Output cosine, output value = cos(angle)*(2^(31-2))/(2*pi), angle=radian*180/pi
 */
__STATIC_INLINE uint32_t LL_DSP_GetRotationCosine(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->CRD_COSINE));
}

/**
 * @brief Return output sine of Rotation mode of CORDIC.
 * @rmtoll CRD_SINE   SINE_OUT   LL_DSP_GetRotationSine
 * @param DSPx  DSP Instance
 * @return Output sine, output value = sin(angle)*(2^(31-2))/(2*pi), angle=radian*180/pi
 */
__STATIC_INLINE uint32_t LL_DSP_GetRotationSine(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->CRD_SINE));
}

/**
 * @brief Enable Rotation mode interrupt of CORDIC.
 * @rmtoll CRD_IE   CRD_ROT_IE   LL_DSP_EnableIT_ROTATION
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_ROTATION(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->CRD_IE, DSP_CRD_IE_CRD_ROT_IE);
}

/**
 * @brief Disable Rotation mode interrupt of CORDIC.
 * @rmtoll CRD_IE   CRD_ROT_IE   LL_DSP_DisableIT_ROTATION
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_ROTATION(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->CRD_IE, DSP_CRD_IE_CRD_ROT_IE);
}

/**
 * @brief Indicates whether Rotation mode interrupt of CORDIC is enabled.
 * @rmtoll CRD_IE   CRD_ROT_IE   LL_DSP_IsEnabledIT_ROTATION
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_ROTATION(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->CRD_IE, DSP_CRD_IE_CRD_ROT_IE) == DSP_CRD_IE_CRD_ROT_IE));
}

/**
 * @brief Enable Vector mode interrupt of CORDIC.
 * @rmtoll CRD_IE   CRD_VEC_IE   LL_DSP_EnableIT_VECTOR
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_EnableIT_VECTOR(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->CRD_IE, DSP_CRD_IE_CRD_VEC_IE);
}

/**
 * @brief Disable Vector mode interrupt of CORDIC.
 * @rmtoll CRD_IE   CRD_VEC_IE   LL_DSP_DisableIT_VECTOR
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_DisableIT_VECTOR(DSP_TypeDef *DSPx)
{
    CLEAR_BIT(DSPx->CRD_IE, DSP_CRD_IE_CRD_VEC_IE);
}

/**
 * @brief Indicates whether Vector mode interrupt of CORDIC is enabled.
 * @rmtoll CRD_IE   CRD_VEC_IE   LL_DSP_IsEnabledVECTOR
 * @param DSPx  DSP Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_DSP_IsEnabledIT_VECTOR(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->CRD_IE, DSP_CRD_IE_CRD_VEC_IE) == DSP_CRD_IE_CRD_VEC_IE));
}

/**
 * @brief Indicates whether the complete interrupt flag of Rotation mode is set.
 * @rmtoll CRD_FLG   CRD_ROT_DONE   LL_DSP_IsActiveFlag_ROTATION
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_ROTATION(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->CRD_FLG, DSP_CRD_FLG_CRD_ROT_DONE) == DSP_CRD_FLG_CRD_ROT_DONE));
}

/**
 * @brief Clear Rotation mode interrupt and flag.
 * @rmtoll CRD_FLG   CRD_ROT_DONE   LL_DSP_ClearFlag_ROTATION
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_ROTATION(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->CRD_FLG, DSP_CRD_FLG_CRD_ROT_DONE);
}

/**
 * @brief Indicates whether the complete interrupt flag of Vector mode is set.
 * @rmtoll CRD_FLG   CRD_VEC_DONE   LL_DSP_IsActiveFlag_VECTOR
 * @param DSPx  DSP Instance
 * @return State of bit, 1: Done, 0: undone.
 */
__STATIC_INLINE uint32_t LL_DSP_IsActiveFlag_VECTOR(DSP_TypeDef *DSPx)
{
    return (uint32_t)((READ_BIT(DSPx->CRD_FLG, DSP_CRD_FLG_CRD_VEC_DONE) == DSP_CRD_FLG_CRD_VEC_DONE));
}

/**
 * @brief Clear Vector mode interrupt and flag.
 * @rmtoll CRD_FLG   CRD_VEC_DONE   LL_DSP_ClearFlag_VECTOR
 * @param DSPx  DSP Instance
 * @return None
 */
__STATIC_INLINE void LL_DSP_ClearFlag_VECTOR(DSP_TypeDef *DSPx)
{
    SET_BIT(DSPx->CRD_FLG, DSP_CRD_FLG_CRD_VEC_DONE);
}

/**
 * @brief Configure input data length of interp.
 * @note  The input data length is integer format, 13 bits, actual length = input length + 1
 * @rmtoll INTP_LEN   INTP_LEN   LL_DSP_SetInterpLength
 * @param DSPx DSP Instance
 * @param Length Input data length.
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetInterpLength(DSP_TypeDef *DSPx, uint32_t Length)
{
    WRITE_REG(DSPx->INTP_LEN, (DSP_INTP_LEN_INTP_LEN & Length));
}

/**
 * @brief Return input data length of interp.
 * @note  The input data length is integer format, 13 bits, actual length = input length + 1
 * @rmtoll INTP_LEN   INTP_LEN   LL_DSP_GetInterpLength
 * @param DSPx  DSP Instance
 * @return Input data length
 */
__STATIC_INLINE uint32_t LL_DSP_GetInterpLength(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->INTP_LEN));
}

/**
 * @brief Configure the current interp location of interpolation algorithm.
 * @note  The Location is float format, 32 bits.
 * @rmtoll INTP_LOC   INTP_LOC   LL_DSP_SetInterpLocation
 * @param DSPx DSP Instance
 * @param Location Current interp location.
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetInterpLocation(DSP_TypeDef *DSPx, uint32_t Location)
{
    WRITE_REG(DSPx->INTP_LOC, Location);
}

/**
 * @brief Return the current interp location of interpolation algorithm.
 * @note  The Location is float format, 32 bits.
 * @rmtoll INTP_LOC   INTP_LOC   LL_DSP_GetInterpLocation
 * @param DSPx  DSP Instance
 * @return The current interp location
 */
__STATIC_INLINE uint32_t LL_DSP_GetInterpLocation(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->INTP_LOC));
}

/**
 * @brief Configure the interp step.
 * @note  The input step is float format, 32 bits.
 * @rmtoll INTP_STEP   INTP_STEP   LL_DSP_SetInterpStep
 * @param DSPx DSP Instance
 * @param Step Interp step.
 * @return None
 */
__STATIC_INLINE void LL_DSP_SetInterpStep(DSP_TypeDef *DSPx, uint32_t Step)
{
    WRITE_REG(DSPx->INTP_STEP, Step);
}

/**
 * @brief Return the interp step.
 * @note  The input step is float format, 32 bits.
 * @rmtoll INTP_STEP   INTP_STEP   LL_DSP_GetInterpStep
 * @param DSPx  DSP Instance
 * @return The interp step
 */
__STATIC_INLINE uint32_t LL_DSP_GetInterpStep(DSP_TypeDef *DSPx)
{
    return (uint32_t)(READ_REG(DSPx->INTP_STEP));
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup DSP_LL_EF_Init Initialization and de-initialization functions
 * @{
 */
ErrorStatus LL_DSP_DeInit(DSP_TypeDef *DSPx);
ErrorStatus LL_DSP_Init(DSP_TypeDef *DSPx, LL_DSP_InitTypeDef *DSP_InitStruct);
void        LL_DSP_StructInit(LL_DSP_InitTypeDef *DSP_InitStruct);
/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */

/**
 * @}
 */

#endif /* DSP */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_DSP_H */
