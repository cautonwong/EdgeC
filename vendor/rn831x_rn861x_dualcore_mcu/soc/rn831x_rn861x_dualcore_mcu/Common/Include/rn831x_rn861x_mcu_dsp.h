/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_dsp.h
 * @author  Renergy Software Team
 * @brief   DSP module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_DSP_H
#define _RN831x_RN861x_MCU_DSP_H

#include "rn831x_rn861x_mcu.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/*                Device Specific Peripheral registers structures             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Peripherals rn831x_rn861x_mcu Peripherals
 * @brief rn831x_rn861x_mcu Device Specific Peripheral registers structures
 * @{
 */

/* DSP */
typedef struct
{
    __IO uint32_t MAC_CTL0;                  /*!< offset: 0x000 info: Control register_0 of Multiply-Accumulate unit */
    __IO uint32_t MAC_CTL1;                  /*!< offset: 0x004 info: Control register_1 of Multiply-Accumulate unit */
    __IO uint32_t MAC_CTL2;                  /*!< offset: 0x008 info: Control register_2 of Multiply-Accumulate unit */
    __IO uint32_t MAC_IN0;                   /*!< offset: 0x00C info: Input register_0 of Multiply-Accumulate unit */
    __IO uint32_t MAC_IN1;                   /*!< offset: 0x010 info: Input register_1 of Multiply-Accumulate unit */
    __IO uint32_t MAC_IN2;                   /*!< offset: 0x014 info: Input register_2 of Multiply-Accumulate unit */
    __IO uint32_t MAC_IN3;                   /*!< offset: 0x018 info: Input register_3 of Multiply-Accumulate unit */
    __IO uint32_t MAC_IN4;                   /*!< offset: 0x01C info: Input register_4 of Multiply-Accumulate unit */
    __IO uint32_t MAC_IN5;                   /*!< offset: 0x020 info: Input register_5 of Multiply-Accumulate unit */
    __IO uint32_t MAC_OUT0;                  /*!< offset: 0x024 info: Output register_0 of Multiply-Accumulate unit */
    __IO uint32_t MAC_OUT1;                  /*!< offset: 0x028 info: Output register_1 of Multiply-Accumulate unit */
    __IO uint32_t MAC_OUT2;                  /*!< offset: 0x02C info: Output register_2 of Multiply-Accumulate unit */
    __IO uint32_t MAC_OUT3;                  /*!< offset: 0x030 info: Output register_3 of Multiply-Accumulate unit */
    __IO uint32_t DIV_IN0;                   /*!< offset: 0x034 info: Input register_0 of division unit */
    __IO uint32_t DIV_IN1;                   /*!< offset: 0x038 info: Input register_1 of division unit */
    __IO uint32_t DIV_OUT0;                  /*!< offset: 0x03C info: Output register_0 of division unit */
    __IO uint32_t DMA_SRBADR;                /*!< offset: 0x040 info: Source address register of DMA and FFT real part */
    __IO uint32_t DMA_SIBADR;                /*!< offset: 0x044 info: Source address register of FFT image part */
    __IO uint32_t DMA_PRBADR;                /*!< offset: 0x048 info: Address register of phase factor table of real part and FIR mode of parameter A */
    __IO uint32_t DMA_PIBADR;                /*!< offset: 0x04C info: Address register of phase factor table of image part */
    __IO uint32_t DMA_TRBADR;                /*!< offset: 0x050 info: Target address register of DMA and FFT real part */
    __IO uint32_t DMA_TIBADR;                /*!< offset: 0x054 info: Target address register of FFT image part */
    __IO uint32_t DMA_LEN;                   /*!< offset: 0x058 info: Length register of DMA */
    __IO uint32_t IE;                        /*!< offset: 0x05C info: Interrupt register of DSP */
    __IO uint32_t FLG;                       /*!< offset: 0x060 info: Interrupt flag register of DSP */
    __IO uint32_t ALU_STA0;                  /*!< offset: 0x064 info: Status register of Multiply-Accumulate unit and floating unit */
    __IO uint32_t ALU_STA1;                  /*!< offset: 0x068 info: Status register of multiplication and division units */
    __IO uint32_t CRD_CTL;                   /*!< offset: 0x06C info: Control register of cordic */
    __IO uint32_t CRD_XIN;                   /*!< offset: 0x070 info: Input register of data X of cordic rotation mode */
    __IO uint32_t CRD_YIN;                   /*!< offset: 0x074 info: Input register of data Y of cordic rotation mode */
    __IO uint32_t CRD_AMP;                   /*!< offset: 0x078 info: Output register of amplitude of cordic rotation mode */
    __IO uint32_t CRD_PHASE;                 /*!< offset: 0x07C info: Output register of phase of cordic rotation mode */
    __IO uint32_t CRD_ANGLE;                 /*!< offset: 0x080 info: Input register of angle of cordic vector mode */
    __IO uint32_t CRD_COSINE;                /*!< offset: 0x084 info: Output register of cosine of cordic vector mode */
    __IO uint32_t CRD_SINE;                  /*!< offset: 0x088 info: Output register of sine of cordic vector mode */
    __IO uint32_t CRD_IE;                    /*!< offset: 0x08C info: Interrupt register of cordic */
    __IO uint32_t CRD_FLG;                   /*!< offset: 0x090 info: Interrupt flag register of cordic */
    __IO uint32_t INTP_LEN;                  /*!< offset: 0x094 info: Length register of interp */
    __IO uint32_t INTP_LOC;                  /*!< offset: 0x098 info: Current location register of interp */
    __IO uint32_t INTP_STEP;                 /*!< offset: 0x09C info: Step register of interp */
} DSP_TypeDef;

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_Peripherals */

/******************************************************************************/
/*                         Peripheral memory map                              */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_MemoryMap rn831x_rn861x_mcu Memory Mapping
 * @{
 */

/* base address */
#define DSP_CORE0_BASE  (0x2006C000U)
#define DSP_CORE1_BASE  (0xA006C000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define DSP_CORE0  ((DSP_TypeDef *)      DSP_CORE0_BASE)
#define DSP_CORE1  ((DSP_TypeDef *)      DSP_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define DSP  DSP_CORE0
#endif
#if defined(CORE1)
#define DSP  DSP_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** DSP Instances *******************************/
#define IS_DSP_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == DSP_CORE0) || \
                                          ((INSTANCE) == DSP_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* DSP_MAC_CTL0 */
#define DSP_MAC_CTL0_MODE_SEL                (BITS(0,4))                                         /*!< Select bits of DSP mode */
#define DSP_MAC_CTL0_MUL_IN_FM               (BIT(5))                                            /*!< Input format bit of multiplication unit */
#define DSP_MAC_CTL0_MUL_OUT_FM              (BIT(6))                                            /*!< Output format bit of multiplication unit */
#define DSP_MAC_CTL0_ROUND_MODE              (BITS(7,9))                                         /*!< Round mode configuration bits */
#define DSP_MAC_CTL0_I2F_DIV                 (BITS(10,14))                                       /*!< Normalized order bit of floating point after Int-To-Floating */
#define DSP_MAC_CTL0_F2I_MUL                 (BITS(15,19))                                       /*!< Magnification order bit of floating point before Floating-To-Int */
#define DSP_MAC_CTL0_FFT_TB_EN               (BIT(20))                                           /*!< Enable bit of FFT internal table */
#define DSP_MAC_CTL0_I2F_PRE_EN              (BITS(21,22))                                       /*!< Enable bits of Floating-To-Int preprocess */

/* DSP_MAC_CTL1 */
#define DSP_MAC_CTL1_INTP_LAST_EN            (BIT(0))                                            /*!< Interpolation mode using last data of last period enable bit */
#define DSP_MAC_CTL1_DMA_STEP                (BITS(1,3))                                         /*!< Step of DMA read data from SRAM configuration bits (only available for IIR/FIR DMA mode ) */
#define DSP_MAC_CTL1_FIR_ORDER               (BITS(4,11))                                        /*!< FIR filter order configuration bits */

/* DSP_MAC_CTL2 */
#define DSP_MAC_CTL2_DMA_EN                  (BIT(0))                                            /*!< Enable bit of DMA */
#define DSP_MAC_CTL2_BTFY_ONCE_KICK          (BIT(1))                                            /*!< Enable bit of butterfly once calculate */
#define DSP_MAC_CTL2_DIV_KICK                (BIT(2))                                            /*!< Enable bit of division unit */

/* DSP_MAC_IN0 */
#define DSP_MAC_IN0_MAC_IN0                  (BITS(0,31))                                        /*!< Input register_0 bits of Multiply-Accumulate unit */

/* DSP_MAC_IN1 */
#define DSP_MAC_IN1_MAC_IN1                  (BITS(0,31))                                        /*!< Input register_1 bits of Multiply-Accumulate unit */

/* DSP_MAC_IN2 */
#define DSP_MAC_IN2_MAC_IN2                  (BITS(0,31))                                        /*!< Input register_2 bits of Multiply-Accumulate unit */

/* DSP_MAC_IN3 */
#define DSP_MAC_IN3_MAC_IN3                  (BITS(0,31))                                        /*!< Input register_3 bits of Multiply-Accumulate unit */

/* DSP_MAC_IN4 */
#define DSP_MAC_IN4_MAC_IN4                  (BITS(0,31))                                        /*!< Input register_4 bits of Multiply-Accumulate unit */

/* DSP_MAC_IN5 */
#define DSP_MAC_IN5_MAC_IN5                  (BITS(0,31))                                        /*!< Input register_5 bits of Multiply-Accumulate unit */

/* DSP_MAC_OUT0 */
#define DSP_MAC_OUT0_MAC_OUT0                (BITS(0,31))                                        /*!< Output register_0 bits of Multiply-Accumulate unit */

/* DSP_MAC_OUT1 */
#define DSP_MAC_OUT1_MAC_OUT1                (BITS(0,31))                                        /*!< Output register_1 bits of Multiply-Accumulate unit */

/* DSP_MAC_OUT2 */
#define DSP_MAC_OUT2_MAC_OUT2                (BITS(0,31))                                        /*!< Output register_2 bits of Multiply-Accumulate unit */

/* DSP_MAC_OUT3 */
#define DSP_MAC_OUT3_MAC_OUT3                (BITS(0,31))                                        /*!< Output register_3 bits of Multiply-Accumulate unit */

/* DSP_DIV_IN0 */
#define DSP_DIV_IN0_DIV_IN0                  (BITS(0,31))                                        /*!< Input register_0 bits of division unit */

/* DSP_DIV_IN1 */
#define DSP_DIV_IN1_DIV_IN1                  (BITS(0,31))                                        /*!< Input register_1 bits of division unit */

/* DSP_DIV_OUT0 */
#define DSP_DIV_OUT0_DIV_OUT0                (BITS(0,31))                                        /*!< Output register_0 bits of division unit */

/* DSP_DMA_SRBADR */
#define DSP_DMA_SRBADR_DMA_SRBADR            (BITS(0,15))                                        /*!< Source address register bits of DMA and FFT real part */

/* DSP_DMA_SIBADR */
#define DSP_DMA_SIBADR_DMA_SIBADR            (BITS(0,15))                                        /*!< Source address register bits of FFT image part */

/* DSP_DMA_PRBADR */
#define DSP_DMA_PRBADR_DMA_PRBADR            (BITS(0,15))                                        /*!< Address register bits of phase factor table of real part and FIR mode of parameter A */

/* DSP_DMA_PIBADR */
#define DSP_DMA_PIBADR_DMA_PIBADR            (BITS(0,15))                                        /*!< Address register bits of phase factor table of image part */

/* DSP_DMA_TRBADR */
#define DSP_DMA_TRBADR_DMA_TRBADR            (BITS(0,15))                                        /*!< Target address register bits of DMA and FFT real part */

/* DSP_DMA_TIBADR */
#define DSP_DMA_TIBADR_DMA_TIBADR            (BITS(0,15))                                        /*!< Target address register bits of FFT image part */

/* DSP_DMA_LEN */
#define DSP_DMA_LEN_DMA_LEN                  (BITS(0,9))                                         /*!< Length register bits of DMA */

/* DSP_IE */
#define DSP_IE_MAC_IE                        (BIT(0))                                            /*!< Interrupt enable bit of  Multiply-Accumulate */
#define DSP_IE_DMA_IE                        (BIT(1))                                            /*!< Interrupt enable bit of DMA */
#define DSP_IE_DIV_IE                        (BIT(2))                                            /*!< Interrupt enable bit of division */
#define DSP_IE_I2F_DMA_IE                    (BIT(3))                                            /*!< Interrupt enable bit of Int-To-Float DMA */
#define DSP_IE_F2I_DMA_IE                    (BIT(4))                                            /*!< Interrupt enable bit of Float-To-Int DMA */
#define DSP_IE_BTFY_ONCE_IE                  (BIT(5))                                            /*!< Interrupt enable bit of butterfly once */
#define DSP_IE_BTFY_DMA_IE                   (BIT(6))                                            /*!< Interrupt enable bit of butterfly DMA */
#define DSP_IE_FFT_IE                        (BIT(7))                                            /*!< Interrupt enable bit of FFT */
#define DSP_IE_BITREV_IE                     (BIT(8))                                            /*!< Interrupt enable bit of bit reverse */
#define DSP_IE_MULT_IE                       (BIT(9))                                            /*!< Interrupt enable bit of multiplication */
#define DSP_IE_IIR_ONCE_IE                   (BIT(10))                                           /*!< Interrupt enable bit of IIR once */
#define DSP_IE_IIR_DMA_IE                    (BIT(11))                                           /*!< Interrupt enable bit of IIR DMA */
#define DSP_IE_FIR_DMA_IE                    (BIT(12))                                           /*!< Interrupt enable bit of FIR DMA */
#define DSP_IE_LINE_DMA_IE                   (BIT(13))                                           /*!< Interrupt enable bit of linear interp DMA */
#define DSP_IE_LAGR_DMA_IE                   (BIT(14))                                           /*!< Interrupt enable bit of lagrange interp DMA */

/* DSP_FLG */
#define DSP_FLG_MAC_DONE                     (BIT(0))                                            /*!< Interrupt flag bit of  Multiply-Accumulate */
#define DSP_FLG_DMA_DONE                     (BIT(1))                                            /*!< Interrupt flag bit of DMA */
#define DSP_FLG_DIV_DONE                     (BIT(2))                                            /*!< Interrupt flag bit of division */
#define DSP_FLG_I2F_DMA_DONE                 (BIT(3))                                            /*!< Interrupt flag bit of Int-To-Float DMA */
#define DSP_FLG_F2I_DMA_DONE                 (BIT(4))                                            /*!< Interrupt flag bit of Float-To-Int DMA */
#define DSP_FLG_BTFY_ONCE_DONE               (BIT(5))                                            /*!< Interrupt flag bit of butterfly once */
#define DSP_FLG_BTFY_DMA_DONE                (BIT(6))                                            /*!< Interrupt flag bit of butterfly DMA */
#define DSP_FLG_FFT_DONE                     (BIT(7))                                            /*!< Interrupt flag bit of FFT */
#define DSP_FLG_BITREV_DONE                  (BIT(8))                                            /*!< Interrupt flag bit of bit reverse */
#define DSP_FLG_MULT_DONE                    (BIT(9))                                            /*!< Interrupt flag bit of multiplication */
#define DSP_FLG_IIR_ONCE_DONE                (BIT(10))                                           /*!< Interrupt flag bit of IIR once */
#define DSP_FLG_IIR_DMA_DONE                 (BIT(11))                                           /*!< Interrupt flag bit of IIR DMA */
#define DSP_FLG_FIR_DMA_DONE                 (BIT(12))                                           /*!< Interrupt flag bit of FIR DMA */
#define DSP_FLG_LINE_DMA_DONE                (BIT(13))                                           /*!< Interrupt flag bit of linear interp DMA */
#define DSP_FLG_LAGR_DMA_DONE                (BIT(14))                                           /*!< Interrupt flag bit of lagrange interp DMA */

/* DSP_ALU_STA0 */
#define DSP_ALU_STA0_ADDSUB0_OUT0            (BIT(0))                                            /*!< ADDSUB0 int or float point output is zero */
#define DSP_ALU_STA0_ADDSUB0_INFINTY         (BIT(1))                                            /*!< ADDSUB0 float output is infinity */
#define DSP_ALU_STA0_ADDSUB0_INVALID         (BIT(2))                                            /*!< ADDSUB0 float operation is invalid or one input is NAN */
#define DSP_ALU_STA0_ADDSUB0_FLOAT_TOOLESS   (BIT(3))                                            /*!< ADDSUB0 float number less than minimum normalized number */
#define DSP_ALU_STA0_ADDSUB0_FLOAT_TOOBIG    (BIT(4))                                            /*!< ADDSUB0 float number greater than maximum normalized number */
#define DSP_ALU_STA0_ADDSUB0_NOT_PRECISE     (BIT(5))                                            /*!< ADDSUB0 int or float output is not equal to the infinitely precise result */
#define DSP_ALU_STA0_ADDSUB0_INT_TOOBIG      (BIT(6))                                            /*!< ADDSUB0 int result after rounding has a magnitude greater than the largest representable two's complement integer with the same sign */
#define DSP_ALU_STA0_ADDSUB1_OUT0            (BIT(8))                                            /*!< ADDSUB1 int or float point output is zero */
#define DSP_ALU_STA0_ADDSUB1_INFINTY         (BIT(9))                                            /*!< ADDSUB1 float output is infinity */
#define DSP_ALU_STA0_ADDSUB1_INVALID         (BIT(10))                                           /*!< ADDSUB1 float operation is invalid or one input is NAN */
#define DSP_ALU_STA0_ADDSUB1_FLOAT_TOOLESS   (BIT(11))                                           /*!< ADDSUB1 float number less than minimum normalized number */
#define DSP_ALU_STA0_ADDSUB1_FLOAT_TOOBIG    (BIT(12))                                           /*!< ADDSUB1 float number greater than maximum normalized number */
#define DSP_ALU_STA0_ADDSUB1_NOT_PRECISE     (BIT(13))                                           /*!< ADDSUB1 int or float output is not equal to the infinitely precise result */
#define DSP_ALU_STA0_ADDSUB1_INT_TOOBIG      (BIT(14))                                           /*!< ADDSUB1 int result after rounding has a magnitude greater than the largest representable two's complement integer with the same sign */
#define DSP_ALU_STA0_I2F_OUT0                (BIT(16))                                           /*!< I2F int or float point output is zero */
#define DSP_ALU_STA0_I2F_INFINTY             (BIT(17))                                           /*!< I2F float output is infinity */
#define DSP_ALU_STA0_I2F_INVALID             (BIT(18))                                           /*!< I2F float operation is invalid or one input is NAN */
#define DSP_ALU_STA0_I2F_FLOAT_TOOLESS       (BIT(19))                                           /*!< I2F float number less than minimum normalized number */
#define DSP_ALU_STA0_I2F_FLOAT_TOOBIG        (BIT(20))                                           /*!< I2F float number greater than maximum normalized number */
#define DSP_ALU_STA0_I2F_NOT_PRECISE         (BIT(21))                                           /*!< I2F int or float output is not equal to the infinitely precise result */
#define DSP_ALU_STA0_I2F_INT_TOOBIG          (BIT(22))                                           /*!< I2F int result after rounding has a magnitude greater than the largest representable two's complement integer with the same sign */
#define DSP_ALU_STA0_F2I_OUT0                (BIT(24))                                           /*!< F2I int or float point output is zero */
#define DSP_ALU_STA0_F2I_INFINTY             (BIT(25))                                           /*!< F2I float output is infinity */
#define DSP_ALU_STA0_F2I_INVALID             (BIT(26))                                           /*!< F2I float operation is invalid or one input is NAN */
#define DSP_ALU_STA0_F2I_FLOAT_TOOLESS       (BIT(27))                                           /*!< F2I float number less than minimum normalized number */
#define DSP_ALU_STA0_F2I_FLOAT_TOOBIG        (BIT(28))                                           /*!< F2I float number greater than maximum normalized number */
#define DSP_ALU_STA0_F2I_NOT_PRECISE         (BIT(29))                                           /*!< F2I int or float output is not equal to the infinitely precise result */
#define DSP_ALU_STA0_F2I_INT_TOOBIG          (BIT(30))                                           /*!< F2I int result after rounding has a magnitude greater than the largest representable two's complement integer with the same sign */

/* DSP_ALU_STA1 */
#define DSP_ALU_STA1_MUL_OUT0                (BIT(0))                                            /*!< MUL inte or float point output is zero */
#define DSP_ALU_STA1_MUL_INFINTY             (BIT(1))                                            /*!< MUL float output is infinity */
#define DSP_ALU_STA1_MUL_INVALID             (BIT(2))                                            /*!< MUL float operation is invalid or one input is NAN */
#define DSP_ALU_STA1_MUL_FLOAT_TOOLESS       (BIT(3))                                            /*!< MUL float number less than minimum normalized number */
#define DSP_ALU_STA1_MUL_FLOAT_TOOBIG        (BIT(4))                                            /*!< MUL float number greater than maximum normalized number */
#define DSP_ALU_STA1_MUL_NOT_PRECISE         (BIT(5))                                            /*!< MUL int or float output is not equal to the infinitely precise result */
#define DSP_ALU_STA1_MUL_INT_TOOBIG          (BIT(6))                                            /*!< MUL int result after rounding has a magnitude greater than the largest representable two's complement integer with the same sign */
#define DSP_ALU_STA1_DIV_OUT0                (BIT(8))                                            /*!< DIV inte or float point output is zero */
#define DSP_ALU_STA1_DIV_INFINTY             (BIT(9))                                            /*!< DIV float output is infinity */
#define DSP_ALU_STA1_DIV_INVALID             (BIT(10))                                           /*!< DIV float operation is invalid or one input is NAN */
#define DSP_ALU_STA1_DIV_FLOAT_TOOLESS       (BIT(11))                                           /*!< DIV float number less than minimum normalized number */
#define DSP_ALU_STA1_DIV_FLOAT_TOOBIG        (BIT(12))                                           /*!< DIV float number greater than maximum normalized number */
#define DSP_ALU_STA1_DIV_NOT_PRECISE         (BIT(13))                                           /*!< DIV int or float output is not equal to the infinitely precise result */
#define DSP_ALU_STA1_DIV_INT_TOOBIG          (BIT(14))                                           /*!< DIV int result after rounding has a magnitude greater than the largest representable two's complement integer with the same sign */

/* DSP_CRD_CTL */
#define DSP_CRD_CTL_CRD_VEC_KICK             (BIT(0))                                            /*!< DSP cordic vector mode start bit */
#define DSP_CRD_CTL_CRD_ROT_KICK             (BIT(1))                                            /*!< DSP cordic rotation mode start bit */

/* DSP_CRD_XIN */
#define DSP_CRD_XIN_X_IN                     (BITS(0,31))                                        /*!< Input register bits of data X of cordic rotation mode */

/* DSP_CRD_YIN */
#define DSP_CRD_YIN_Y_IN                     (BITS(0,31))                                        /*!< Input register bits of data Y of cordic rotation mode */

/* DSP_CRD_AMP */
#define DSP_CRD_AMP_AMP_OUT                  (BITS(0,31))                                        /*!< Output register bits of amplitude of cordic rotation mode */

/* DSP_CRD_PHASE */
#define DSP_CRD_PHASE_PHASE_OUT              (BITS(0,31))                                        /*!< Output register bits of phase of cordic rotation mode */

/* DSP_CRD_ANGLE */
#define DSP_CRD_ANGLE_ANGLE_IN               (BITS(0,31))                                        /*!< Input register bits of angle of cordic vector mode */

/* DSP_CRD_COSINE */
#define DSP_CRD_COSINE_COSINE_OUT            (BITS(0,31))                                        /*!< Output register bits of cosine of cordic vector mode */

/* DSP_CRD_SINE */
#define DSP_CRD_SINE_SINE_OUT                (BITS(0,31))                                        /*!< Output register bits of sine of cordic vector mode */

/* DSP_CRD_IE */
#define DSP_CRD_IE_CRD_VEC_IE                (BIT(0))                                            /*!< Cordic vector mode completion interrupt enable bit */
#define DSP_CRD_IE_CRD_ROT_IE                (BIT(1))                                            /*!< Cordic rotation mode completion interrupt enable bit */

/* DSP_CRD_FLG */
#define DSP_CRD_FLG_CRD_VEC_DONE             (BIT(0))                                            /*!< Cordic vector operation completion flag */
#define DSP_CRD_FLG_CRD_ROT_DONE             (BIT(1))                                            /*!< Cordic rotation operation completion flag */

/* DSP_INTP_LEN */
#define DSP_INTP_LEN_INTP_LEN                (BITS(0,12))                                        /*!< Length register bits of interp */

/* DSP_INTP_LOC */
#define DSP_INTP_LOC_INTP_LOC                (BITS(0,31))                                        /*!< Current location register bits of interp */

/* DSP_INTP_STEP */
#define DSP_INTP_STEP_INTP_STEP              (BITS(0,31))                                        /*!< Step register bits of interp */

/* constants definitions */
/* DSP_MAC_CTL0_MODE_SEL */
#define DSP_MAC_CTL0_MODE_SEL_POS            (0U)                                                /*!< DSP_MAC_CTL0_MODE_SEL bit offset */
#define DSP_MAC_CTL0_MODE_SEL_CFG(regval)  (DSP_MAC_CTL0_MODE_SEL & ((uint32_t)(regval) << DSP_MAC_CTL0_MODE_SEL_POS))
#define DSP_MAC_CTL0_MODE_SEL_DIS            (DSP_MAC_CTL0_MODE_SEL_CFG(0))                      /*!< disable all mode of MAC unit */
#define DSP_MAC_CTL0_MODE_SEL_I2F_ONCE       (DSP_MAC_CTL0_MODE_SEL_CFG(1))                      /*!< int to float conversion one-shot mode enable */
#define DSP_MAC_CTL0_MODE_SEL_I2F_DMA        (DSP_MAC_CTL0_MODE_SEL_CFG(2))                      /*!< int to float conversion DMA mode enable */
#define DSP_MAC_CTL0_MODE_SEL_F2I_ONCE       (DSP_MAC_CTL0_MODE_SEL_CFG(3))                      /*!< float to int conversion one-shot mode enable */
#define DSP_MAC_CTL0_MODE_SEL_F2I_DMA        (DSP_MAC_CTL0_MODE_SEL_CFG(4))                      /*!< float to int conversion DMA mode enable */
#define DSP_MAC_CTL0_MODE_SEL_FP_MUL         (DSP_MAC_CTL0_MODE_SEL_CFG(5))                      /*!< float multiply mode enable */
#define DSP_MAC_CTL0_MODE_SEL_FP_ADD         (DSP_MAC_CTL0_MODE_SEL_CFG(6))                      /*!< float addition mode enable */
#define DSP_MAC_CTL0_MODE_SEL_FP_SUB         (DSP_MAC_CTL0_MODE_SEL_CFG(7))                      /*!< float subtraction mode enable */
#define DSP_MAC_CTL0_MODE_SEL_FP_MLAD        (DSP_MAC_CTL0_MODE_SEL_CFG(8))                      /*!< float multiplication and addition mode enable */
#define DSP_MAC_CTL0_MODE_SEL_BTFY_ONCE      (DSP_MAC_CTL0_MODE_SEL_CFG(9))                      /*!< Butterfly operation unit one-shot mode enable */
#define DSP_MAC_CTL0_MODE_SEL_BTFY_DMA       (DSP_MAC_CTL0_MODE_SEL_CFG(10))                     /*!< Butterfly operation unit DMA mode enable */
#define DSP_MAC_CTL0_MODE_SEL_FFT            (DSP_MAC_CTL0_MODE_SEL_CFG(11))                     /*!< FFT mode enable */
#define DSP_MAC_CTL0_MODE_SEL_BIT_RV         (DSP_MAC_CTL0_MODE_SEL_CFG(12))                     /*!< bit reverse mode enable */
#define DSP_MAC_CTL0_MODE_SEL_IIR_ONCE       (DSP_MAC_CTL0_MODE_SEL_CFG(13))                     /*!< 2nd IIR filter one-shot mode enable */
#define DSP_MAC_CTL0_MODE_SEL_IIR_DMA        (DSP_MAC_CTL0_MODE_SEL_CFG(14))                     /*!< 2nd IIR filter DMA mode enable */
#define DSP_MAC_CTL0_MODE_SEL_FIR_DMA        (DSP_MAC_CTL0_MODE_SEL_CFG(15))                     /*!< FIR filter mode enable */
#define DSP_MAC_CTL0_MODE_SEL_LINE_INTP      (DSP_MAC_CTL0_MODE_SEL_CFG(16))                     /*!< linear interpolation mode enable */
#define DSP_MAC_CTL0_MODE_SEL_LAGR_INTP      (DSP_MAC_CTL0_MODE_SEL_CFG(17))                     /*!< Lagrange interpolation mode enable */

/* DSP_MAC_CTL0_MUL_IN_FM */
#define DSP_MAC_CTL0_MUL_IN_FM_POS           (5U)                                                /*!< DSP_MAC_CTL0_MUL_IN_FM bit offset */
#define DSP_MAC_CTL0_MUL_IN_FM_FLOAT         ((uint32_t)0x00000000U)                             /*!< Select multiplication input format as float */
#define DSP_MAC_CTL0_MUL_IN_FM_INT           (DSP_MAC_CTL0_MUL_IN_FM)                            /*!< Select multiplication input format as int */

/* DSP_MAC_CTL0_MUL_OUT_FM */
#define DSP_MAC_CTL0_MUL_OUT_FM_POS          (6U)                                                /*!< DSP_MAC_CTL0_MUL_OUT_FM bit offset */
#define DSP_MAC_CTL0_MUL_OUT_FM_FLOAT        ((uint32_t)0x00000000U)                             /*!< Select multiplication output format as float */
#define DSP_MAC_CTL0_MUL_OUT_FM_INT          (DSP_MAC_CTL0_MUL_OUT_FM)                           /*!< Select multiplication output format as int */

/* DSP_MAC_CTL0_ROUND_MODE */
#define DSP_MAC_CTL0_ROUND_MODE_POS          (7U)                                                /*!< DSP_MAC_CTL0_ROUND_MODE bit offset */
#define DSP_MAC_CTL0_ROUND_MODE_CFG(regval)  (DSP_MAC_CTL0_ROUND_MODE & ((uint32_t)(regval) << DSP_MAC_CTL0_ROUND_MODE_POS))
#define DSP_MAC_CTL0_ROUND_MODE_NEAREST      (DSP_MAC_CTL0_ROUND_MODE_CFG(0))                    /*!< Select round mode as nearest */
#define DSP_MAC_CTL0_ROUND_MODE_ZERO         (DSP_MAC_CTL0_ROUND_MODE_CFG(1))                    /*!< Select round mode as zero */
#define DSP_MAC_CTL0_ROUND_MODE_P_INF        (DSP_MAC_CTL0_ROUND_MODE_CFG(2))                    /*!< Select round mode as positive infinity */
#define DSP_MAC_CTL0_ROUND_MODE_N_INF        (DSP_MAC_CTL0_ROUND_MODE_CFG(3))                    /*!< Select round mode as negative infinity */
#define DSP_MAC_CTL0_ROUND_MODE_UP           (DSP_MAC_CTL0_ROUND_MODE_CFG(4))                    /*!< Select round mode as nearst up */
#define DSP_MAC_CTL0_ROUND_MODE_AWAY         (DSP_MAC_CTL0_ROUND_MODE_CFG(5))                    /*!< Select round mode as away from zero */

/* DSP_MAC_CTL0_I2F_DIV */
#define DSP_MAC_CTL0_I2F_DIV_POS             (10U)                                               /*!< DSP_MAC_CTL0_I2F_DIV bit offset */

/* DSP_MAC_CTL0_F2I_MUL */
#define DSP_MAC_CTL0_F2I_MUL_POS             (15U)                                               /*!< DSP_MAC_CTL0_F2I_MUL bit offset */

/* DSP_MAC_CTL0_FFT_TB_EN */
#define DSP_MAC_CTL0_FFT_TB_EN_POS           (20U)                                               /*!< DSP_MAC_CTL0_FFT_TB_EN bit offset */

/* DSP_MAC_CTL0_I2F_PRE_EN */
#define DSP_MAC_CTL0_I2F_PRE_EN_POS          (21U)                                               /*!< DSP_MAC_CTL0_I2F_PRE_EN bit offset */
#define DSP_MAC_CTL0_I2F_PRE_EN_CFG(regval)  (DSP_MAC_CTL0_I2F_PRE_EN & ((uint32_t)(regval) << DSP_MAC_CTL0_I2F_PRE_EN_POS))
#define DSP_MAC_CTL0_I2F_PRE_EN_4BYTE        (DSP_MAC_CTL0_I2F_PRE_EN_CFG(0))                    /*!< I2F input data 4 byte available */
#define DSP_MAC_CTL0_I2F_PRE_EN_3BYTE        (DSP_MAC_CTL0_I2F_PRE_EN_CFG(1))                    /*!< I2F input data low 3 byte available */
#define DSP_MAC_CTL0_I2F_PRE_EN_2BYTE        (DSP_MAC_CTL0_I2F_PRE_EN_CFG(2))                    /*!< I2F input data low 2 byte available */
#define DSP_MAC_CTL0_I2F_PRE_EN_1BYTE        (DSP_MAC_CTL0_I2F_PRE_EN_CFG(3))                    /*!< I2F input data low 1 byte available */

/* DSP_MAC_CTL1_INTP_LAST_EN */
#define DSP_MAC_CTL1_INTP_LAST_EN_POS        (0U)                                                /*!< DSP_MAC_CTL1_INTP_LAST_EN bit offset */

/* DSP_MAC_CTL1_DMA_STEP */
#define DSP_MAC_CTL1_DMA_STEP_POS            (1U)                                                /*!< DSP_MAC_CTL1_DMA_STEP bit offset */
#define DSP_MAC_CTL1_DMA_STEP_CFG(regval)  (DSP_MAC_CTL1_DMA_STEP & ((uint32_t)(regval) << DSP_MAC_CTL1_DMA_STEP_POS))
#define DSP_MAC_CTL1_DMA_STEP_STEP_1         (DSP_MAC_CTL1_DMA_STEP_CFG(0))                      /*!< Configure DMA data step as 1 */
#define DSP_MAC_CTL1_DMA_STEP_STEP_2         (DSP_MAC_CTL1_DMA_STEP_CFG(1))                      /*!< Configure DMA data step as 2 */
#define DSP_MAC_CTL1_DMA_STEP_STEP_3         (DSP_MAC_CTL1_DMA_STEP_CFG(2))                      /*!< Configure DMA data step as 3 */
#define DSP_MAC_CTL1_DMA_STEP_STEP_4         (DSP_MAC_CTL1_DMA_STEP_CFG(3))                      /*!< Configure DMA data step as 4 */
#define DSP_MAC_CTL1_DMA_STEP_STEP_5         (DSP_MAC_CTL1_DMA_STEP_CFG(4))                      /*!< Configure DMA data step as 5 */
#define DSP_MAC_CTL1_DMA_STEP_STEP_6         (DSP_MAC_CTL1_DMA_STEP_CFG(5))                      /*!< Configure DMA data step as 6 */
#define DSP_MAC_CTL1_DMA_STEP_STEP_7         (DSP_MAC_CTL1_DMA_STEP_CFG(6))                      /*!< Configure DMA data step as 7 */
#define DSP_MAC_CTL1_DMA_STEP_STEP_8         (DSP_MAC_CTL1_DMA_STEP_CFG(7))                      /*!< Configure DMA data step as 8 */

/* DSP_MAC_CTL1_FIR_ORDER */
#define DSP_MAC_CTL1_FIR_ORDER_POS           (4U)                                                /*!< DSP_MAC_CTL1_FIR_ORDER bit offset */

/* DSP_MAC_CTL2_DMA_EN */
#define DSP_MAC_CTL2_DMA_EN_POS              (0U)                                                /*!< DSP_MAC_CTL2_DMA_EN bit offset */

/* DSP_MAC_CTL2_BTFY_ONCE_KICK */
#define DSP_MAC_CTL2_BTFY_ONCE_KICK_POS      (1U)                                                /*!< DSP_MAC_CTL2_BTFY_ONCE_KICK bit offset */

/* DSP_MAC_CTL2_DIV_KICK */
#define DSP_MAC_CTL2_DIV_KICK_POS            (2U)                                                /*!< DSP_MAC_CTL2_DIV_KICK bit offset */

/* DSP_MAC_IN0_MAC_IN0 */
#define DSP_MAC_IN0_MAC_IN0_POS              (0U)                                                /*!< DSP_MAC_IN0_MAC_IN0 bit offset */

/* DSP_MAC_IN1_MAC_IN1 */
#define DSP_MAC_IN1_MAC_IN1_POS              (0U)                                                /*!< DSP_MAC_IN1_MAC_IN1 bit offset */

/* DSP_MAC_IN2_MAC_IN2 */
#define DSP_MAC_IN2_MAC_IN2_POS              (0U)                                                /*!< DSP_MAC_IN2_MAC_IN2 bit offset */

/* DSP_MAC_IN3_MAC_IN3 */
#define DSP_MAC_IN3_MAC_IN3_POS              (0U)                                                /*!< DSP_MAC_IN3_MAC_IN3 bit offset */

/* DSP_MAC_IN4_MAC_IN4 */
#define DSP_MAC_IN4_MAC_IN4_POS              (0U)                                                /*!< DSP_MAC_IN4_MAC_IN4 bit offset */

/* DSP_MAC_IN5_MAC_IN5 */
#define DSP_MAC_IN5_MAC_IN5_POS              (0U)                                                /*!< DSP_MAC_IN5_MAC_IN5 bit offset */

/* DSP_MAC_OUT0_MAC_OUT0 */
#define DSP_MAC_OUT0_MAC_OUT0_POS            (0U)                                                /*!< DSP_MAC_OUT0_MAC_OUT0 bit offset */

/* DSP_MAC_OUT1_MAC_OUT1 */
#define DSP_MAC_OUT1_MAC_OUT1_POS            (0U)                                                /*!< DSP_MAC_OUT1_MAC_OUT1 bit offset */

/* DSP_MAC_OUT2_MAC_OUT2 */
#define DSP_MAC_OUT2_MAC_OUT2_POS            (0U)                                                /*!< DSP_MAC_OUT2_MAC_OUT2 bit offset */

/* DSP_MAC_OUT3_MAC_OUT3 */
#define DSP_MAC_OUT3_MAC_OUT3_POS            (0U)                                                /*!< DSP_MAC_OUT3_MAC_OUT3 bit offset */

/* DSP_DIV_IN0_DIV_IN0 */
#define DSP_DIV_IN0_DIV_IN0_POS              (0U)                                                /*!< DSP_DIV_IN0_DIV_IN0 bit offset */

/* DSP_DIV_IN1_DIV_IN1 */
#define DSP_DIV_IN1_DIV_IN1_POS              (0U)                                                /*!< DSP_DIV_IN1_DIV_IN1 bit offset */

/* DSP_DIV_OUT0_DIV_OUT0 */
#define DSP_DIV_OUT0_DIV_OUT0_POS            (0U)                                                /*!< DSP_DIV_OUT0_DIV_OUT0 bit offset */

/* DSP_DMA_SRBADR_DMA_SRBADR */
#define DSP_DMA_SRBADR_DMA_SRBADR_POS        (0U)                                                /*!< DSP_DMA_SRBADR_DMA_SRBADR bit offset */

/* DSP_DMA_SIBADR_DMA_SIBADR */
#define DSP_DMA_SIBADR_DMA_SIBADR_POS        (0U)                                                /*!< DSP_DMA_SIBADR_DMA_SIBADR bit offset */

/* DSP_DMA_PRBADR_DMA_PRBADR */
#define DSP_DMA_PRBADR_DMA_PRBADR_POS        (0U)                                                /*!< DSP_DMA_PRBADR_DMA_PRBADR bit offset */

/* DSP_DMA_PIBADR_DMA_PIBADR */
#define DSP_DMA_PIBADR_DMA_PIBADR_POS        (0U)                                                /*!< DSP_DMA_PIBADR_DMA_PIBADR bit offset */

/* DSP_DMA_TRBADR_DMA_TRBADR */
#define DSP_DMA_TRBADR_DMA_TRBADR_POS        (0U)                                                /*!< DSP_DMA_TRBADR_DMA_TRBADR bit offset */

/* DSP_DMA_TIBADR_DMA_TIBADR */
#define DSP_DMA_TIBADR_DMA_TIBADR_POS        (0U)                                                /*!< DSP_DMA_TIBADR_DMA_TIBADR bit offset */

/* DSP_DMA_LEN_DMA_LEN */
#define DSP_DMA_LEN_DMA_LEN_POS              (0U)                                                /*!< DSP_DMA_LEN_DMA_LEN bit offset */

/* DSP_IE_MAC_IE */
#define DSP_IE_MAC_IE_POS                    (0U)                                                /*!< DSP_IE_MAC_IE bit offset */

/* DSP_IE_DMA_IE */
#define DSP_IE_DMA_IE_POS                    (1U)                                                /*!< DSP_IE_DMA_IE bit offset */

/* DSP_IE_DIV_IE */
#define DSP_IE_DIV_IE_POS                    (2U)                                                /*!< DSP_IE_DIV_IE bit offset */

/* DSP_IE_I2F_DMA_IE */
#define DSP_IE_I2F_DMA_IE_POS                (3U)                                                /*!< DSP_IE_I2F_DMA_IE bit offset */

/* DSP_IE_F2I_DMA_IE */
#define DSP_IE_F2I_DMA_IE_POS                (4U)                                                /*!< DSP_IE_F2I_DMA_IE bit offset */

/* DSP_IE_BTFY_ONCE_IE */
#define DSP_IE_BTFY_ONCE_IE_POS              (5U)                                                /*!< DSP_IE_BTFY_ONCE_IE bit offset */

/* DSP_IE_BTFY_DMA_IE */
#define DSP_IE_BTFY_DMA_IE_POS               (6U)                                                /*!< DSP_IE_BTFY_DMA_IE bit offset */

/* DSP_IE_FFT_IE */
#define DSP_IE_FFT_IE_POS                    (7U)                                                /*!< DSP_IE_FFT_IE bit offset */

/* DSP_IE_BITREV_IE */
#define DSP_IE_BITREV_IE_POS                 (8U)                                                /*!< DSP_IE_BITREV_IE bit offset */

/* DSP_IE_MULT_IE */
#define DSP_IE_MULT_IE_POS                   (9U)                                                /*!< DSP_IE_MULT_IE bit offset */

/* DSP_IE_IIR_ONCE_IE */
#define DSP_IE_IIR_ONCE_IE_POS               (10U)                                               /*!< DSP_IE_IIR_ONCE_IE bit offset */

/* DSP_IE_IIR_DMA_IE */
#define DSP_IE_IIR_DMA_IE_POS                (11U)                                               /*!< DSP_IE_IIR_DMA_IE bit offset */

/* DSP_IE_FIR_DMA_IE */
#define DSP_IE_FIR_DMA_IE_POS                (12U)                                               /*!< DSP_IE_FIR_DMA_IE bit offset */

/* DSP_IE_LINE_DMA_IE */
#define DSP_IE_LINE_DMA_IE_POS               (13U)                                               /*!< DSP_IE_LINE_DMA_IE bit offset */

/* DSP_IE_LAGR_DMA_IE */
#define DSP_IE_LAGR_DMA_IE_POS               (14U)                                               /*!< DSP_IE_LAGR_DMA_IE bit offset */

/* DSP_FLG_MAC_DONE */
#define DSP_FLG_MAC_DONE_POS                 (0U)                                                /*!< DSP_FLG_MAC_DONE bit offset */

/* DSP_FLG_DMA_DONE */
#define DSP_FLG_DMA_DONE_POS                 (1U)                                                /*!< DSP_FLG_DMA_DONE bit offset */

/* DSP_FLG_DIV_DONE */
#define DSP_FLG_DIV_DONE_POS                 (2U)                                                /*!< DSP_FLG_DIV_DONE bit offset */

/* DSP_FLG_I2F_DMA_DONE */
#define DSP_FLG_I2F_DMA_DONE_POS             (3U)                                                /*!< DSP_FLG_I2F_DMA_DONE bit offset */

/* DSP_FLG_F2I_DMA_DONE */
#define DSP_FLG_F2I_DMA_DONE_POS             (4U)                                                /*!< DSP_FLG_F2I_DMA_DONE bit offset */

/* DSP_FLG_BTFY_ONCE_DONE */
#define DSP_FLG_BTFY_ONCE_DONE_POS           (5U)                                                /*!< DSP_FLG_BTFY_ONCE_DONE bit offset */

/* DSP_FLG_BTFY_DMA_DONE */
#define DSP_FLG_BTFY_DMA_DONE_POS            (6U)                                                /*!< DSP_FLG_BTFY_DMA_DONE bit offset */

/* DSP_FLG_FFT_DONE */
#define DSP_FLG_FFT_DONE_POS                 (7U)                                                /*!< DSP_FLG_FFT_DONE bit offset */

/* DSP_FLG_BITREV_DONE */
#define DSP_FLG_BITREV_DONE_POS              (8U)                                                /*!< DSP_FLG_BITREV_DONE bit offset */

/* DSP_FLG_MULT_DONE */
#define DSP_FLG_MULT_DONE_POS                (9U)                                                /*!< DSP_FLG_MULT_DONE bit offset */

/* DSP_FLG_IIR_ONCE_DONE */
#define DSP_FLG_IIR_ONCE_DONE_POS            (10U)                                               /*!< DSP_FLG_IIR_ONCE_DONE bit offset */

/* DSP_FLG_IIR_DMA_DONE */
#define DSP_FLG_IIR_DMA_DONE_POS             (11U)                                               /*!< DSP_FLG_IIR_DMA_DONE bit offset */

/* DSP_FLG_FIR_DMA_DONE */
#define DSP_FLG_FIR_DMA_DONE_POS             (12U)                                               /*!< DSP_FLG_FIR_DMA_DONE bit offset */

/* DSP_FLG_LINE_DMA_DONE */
#define DSP_FLG_LINE_DMA_DONE_POS            (13U)                                               /*!< DSP_FLG_LINE_DMA_DONE bit offset */

/* DSP_FLG_LAGR_DMA_DONE */
#define DSP_FLG_LAGR_DMA_DONE_POS            (14U)                                               /*!< DSP_FLG_LAGR_DMA_DONE bit offset */

/* DSP_ALU_STA0_ADDSUB0_OUT0 */
#define DSP_ALU_STA0_ADDSUB0_OUT0_POS        (0U)                                                /*!< DSP_ALU_STA0_ADDSUB0_OUT0 bit offset */

/* DSP_ALU_STA0_ADDSUB0_INFINTY */
#define DSP_ALU_STA0_ADDSUB0_INFINTY_POS     (1U)                                                /*!< DSP_ALU_STA0_ADDSUB0_INFINTY bit offset */

/* DSP_ALU_STA0_ADDSUB0_INVALID */
#define DSP_ALU_STA0_ADDSUB0_INVALID_POS     (2U)                                                /*!< DSP_ALU_STA0_ADDSUB0_INVALID bit offset */

/* DSP_ALU_STA0_ADDSUB0_FLOAT_TOOLESS */
#define DSP_ALU_STA0_ADDSUB0_FLOAT_TOOLESS_POS  (3U)                                                /*!< DSP_ALU_STA0_ADDSUB0_FLOAT_TOOLESS bit offset */

/* DSP_ALU_STA0_ADDSUB0_FLOAT_TOOBIG */
#define DSP_ALU_STA0_ADDSUB0_FLOAT_TOOBIG_POS  (4U)                                                /*!< DSP_ALU_STA0_ADDSUB0_FLOAT_TOOBIG bit offset */

/* DSP_ALU_STA0_ADDSUB0_NOT_PRECISE */
#define DSP_ALU_STA0_ADDSUB0_NOT_PRECISE_POS  (5U)                                                /*!< DSP_ALU_STA0_ADDSUB0_NOT_PRECISE bit offset */

/* DSP_ALU_STA0_ADDSUB0_INT_TOOBIG */
#define DSP_ALU_STA0_ADDSUB0_INT_TOOBIG_POS  (6U)                                                /*!< DSP_ALU_STA0_ADDSUB0_INT_TOOBIG bit offset */

/* DSP_ALU_STA0_ADDSUB1_OUT0 */
#define DSP_ALU_STA0_ADDSUB1_OUT0_POS        (8U)                                                /*!< DSP_ALU_STA0_ADDSUB1_OUT0 bit offset */

/* DSP_ALU_STA0_ADDSUB1_INFINTY */
#define DSP_ALU_STA0_ADDSUB1_INFINTY_POS     (9U)                                                /*!< DSP_ALU_STA0_ADDSUB1_INFINTY bit offset */

/* DSP_ALU_STA0_ADDSUB1_INVALID */
#define DSP_ALU_STA0_ADDSUB1_INVALID_POS     (10U)                                               /*!< DSP_ALU_STA0_ADDSUB1_INVALID bit offset */

/* DSP_ALU_STA0_ADDSUB1_FLOAT_TOOLESS */
#define DSP_ALU_STA0_ADDSUB1_FLOAT_TOOLESS_POS  (11U)                                               /*!< DSP_ALU_STA0_ADDSUB1_FLOAT_TOOLESS bit offset */

/* DSP_ALU_STA0_ADDSUB1_FLOAT_TOOBIG */
#define DSP_ALU_STA0_ADDSUB1_FLOAT_TOOBIG_POS  (12U)                                               /*!< DSP_ALU_STA0_ADDSUB1_FLOAT_TOOBIG bit offset */

/* DSP_ALU_STA0_ADDSUB1_NOT_PRECISE */
#define DSP_ALU_STA0_ADDSUB1_NOT_PRECISE_POS  (13U)                                               /*!< DSP_ALU_STA0_ADDSUB1_NOT_PRECISE bit offset */

/* DSP_ALU_STA0_ADDSUB1_INT_TOOBIG */
#define DSP_ALU_STA0_ADDSUB1_INT_TOOBIG_POS  (14U)                                               /*!< DSP_ALU_STA0_ADDSUB1_INT_TOOBIG bit offset */

/* DSP_ALU_STA0_I2F_OUT0 */
#define DSP_ALU_STA0_I2F_OUT0_POS            (16U)                                               /*!< DSP_ALU_STA0_I2F_OUT0 bit offset */

/* DSP_ALU_STA0_I2F_INFINTY */
#define DSP_ALU_STA0_I2F_INFINTY_POS         (17U)                                               /*!< DSP_ALU_STA0_I2F_INFINTY bit offset */

/* DSP_ALU_STA0_I2F_INVALID */
#define DSP_ALU_STA0_I2F_INVALID_POS         (18U)                                               /*!< DSP_ALU_STA0_I2F_INVALID bit offset */

/* DSP_ALU_STA0_I2F_FLOAT_TOOLESS */
#define DSP_ALU_STA0_I2F_FLOAT_TOOLESS_POS   (19U)                                               /*!< DSP_ALU_STA0_I2F_FLOAT_TOOLESS bit offset */

/* DSP_ALU_STA0_I2F_FLOAT_TOOBIG */
#define DSP_ALU_STA0_I2F_FLOAT_TOOBIG_POS    (20U)                                               /*!< DSP_ALU_STA0_I2F_FLOAT_TOOBIG bit offset */

/* DSP_ALU_STA0_I2F_NOT_PRECISE */
#define DSP_ALU_STA0_I2F_NOT_PRECISE_POS     (21U)                                               /*!< DSP_ALU_STA0_I2F_NOT_PRECISE bit offset */

/* DSP_ALU_STA0_I2F_INT_TOOBIG */
#define DSP_ALU_STA0_I2F_INT_TOOBIG_POS      (22U)                                               /*!< DSP_ALU_STA0_I2F_INT_TOOBIG bit offset */

/* DSP_ALU_STA0_F2I_OUT0 */
#define DSP_ALU_STA0_F2I_OUT0_POS            (24U)                                               /*!< DSP_ALU_STA0_F2I_OUT0 bit offset */

/* DSP_ALU_STA0_F2I_INFINTY */
#define DSP_ALU_STA0_F2I_INFINTY_POS         (25U)                                               /*!< DSP_ALU_STA0_F2I_INFINTY bit offset */

/* DSP_ALU_STA0_F2I_INVALID */
#define DSP_ALU_STA0_F2I_INVALID_POS         (26U)                                               /*!< DSP_ALU_STA0_F2I_INVALID bit offset */

/* DSP_ALU_STA0_F2I_FLOAT_TOOLESS */
#define DSP_ALU_STA0_F2I_FLOAT_TOOLESS_POS   (27U)                                               /*!< DSP_ALU_STA0_F2I_FLOAT_TOOLESS bit offset */

/* DSP_ALU_STA0_F2I_FLOAT_TOOBIG */
#define DSP_ALU_STA0_F2I_FLOAT_TOOBIG_POS    (28U)                                               /*!< DSP_ALU_STA0_F2I_FLOAT_TOOBIG bit offset */

/* DSP_ALU_STA0_F2I_NOT_PRECISE */
#define DSP_ALU_STA0_F2I_NOT_PRECISE_POS     (29U)                                               /*!< DSP_ALU_STA0_F2I_NOT_PRECISE bit offset */

/* DSP_ALU_STA0_F2I_INT_TOOBIG */
#define DSP_ALU_STA0_F2I_INT_TOOBIG_POS      (30U)                                               /*!< DSP_ALU_STA0_F2I_INT_TOOBIG bit offset */

/* DSP_ALU_STA1_MUL_OUT0 */
#define DSP_ALU_STA1_MUL_OUT0_POS            (0U)                                                /*!< DSP_ALU_STA1_MUL_OUT0 bit offset */

/* DSP_ALU_STA1_MUL_INFINTY */
#define DSP_ALU_STA1_MUL_INFINTY_POS         (1U)                                                /*!< DSP_ALU_STA1_MUL_INFINTY bit offset */

/* DSP_ALU_STA1_MUL_INVALID */
#define DSP_ALU_STA1_MUL_INVALID_POS         (2U)                                                /*!< DSP_ALU_STA1_MUL_INVALID bit offset */

/* DSP_ALU_STA1_MUL_FLOAT_TOOLESS */
#define DSP_ALU_STA1_MUL_FLOAT_TOOLESS_POS   (3U)                                                /*!< DSP_ALU_STA1_MUL_FLOAT_TOOLESS bit offset */

/* DSP_ALU_STA1_MUL_FLOAT_TOOBIG */
#define DSP_ALU_STA1_MUL_FLOAT_TOOBIG_POS    (4U)                                                /*!< DSP_ALU_STA1_MUL_FLOAT_TOOBIG bit offset */

/* DSP_ALU_STA1_MUL_NOT_PRECISE */
#define DSP_ALU_STA1_MUL_NOT_PRECISE_POS     (5U)                                                /*!< DSP_ALU_STA1_MUL_NOT_PRECISE bit offset */

/* DSP_ALU_STA1_MUL_INT_TOOBIG */
#define DSP_ALU_STA1_MUL_INT_TOOBIG_POS      (6U)                                                /*!< DSP_ALU_STA1_MUL_INT_TOOBIG bit offset */

/* DSP_ALU_STA1_DIV_OUT0 */
#define DSP_ALU_STA1_DIV_OUT0_POS            (8U)                                                /*!< DSP_ALU_STA1_DIV_OUT0 bit offset */

/* DSP_ALU_STA1_DIV_INFINTY */
#define DSP_ALU_STA1_DIV_INFINTY_POS         (9U)                                                /*!< DSP_ALU_STA1_DIV_INFINTY bit offset */

/* DSP_ALU_STA1_DIV_INVALID */
#define DSP_ALU_STA1_DIV_INVALID_POS         (10U)                                               /*!< DSP_ALU_STA1_DIV_INVALID bit offset */

/* DSP_ALU_STA1_DIV_FLOAT_TOOLESS */
#define DSP_ALU_STA1_DIV_FLOAT_TOOLESS_POS   (11U)                                               /*!< DSP_ALU_STA1_DIV_FLOAT_TOOLESS bit offset */

/* DSP_ALU_STA1_DIV_FLOAT_TOOBIG */
#define DSP_ALU_STA1_DIV_FLOAT_TOOBIG_POS    (12U)                                               /*!< DSP_ALU_STA1_DIV_FLOAT_TOOBIG bit offset */

/* DSP_ALU_STA1_DIV_NOT_PRECISE */
#define DSP_ALU_STA1_DIV_NOT_PRECISE_POS     (13U)                                               /*!< DSP_ALU_STA1_DIV_NOT_PRECISE bit offset */

/* DSP_ALU_STA1_DIV_INT_TOOBIG */
#define DSP_ALU_STA1_DIV_INT_TOOBIG_POS      (14U)                                               /*!< DSP_ALU_STA1_DIV_INT_TOOBIG bit offset */

/* DSP_CRD_CTL_CRD_VEC_KICK */
#define DSP_CRD_CTL_CRD_VEC_KICK_POS         (0U)                                                /*!< DSP_CRD_CTL_CRD_VEC_KICK bit offset */

/* DSP_CRD_CTL_CRD_ROT_KICK */
#define DSP_CRD_CTL_CRD_ROT_KICK_POS         (1U)                                                /*!< DSP_CRD_CTL_CRD_ROT_KICK bit offset */

/* DSP_CRD_XIN_X_IN */
#define DSP_CRD_XIN_X_IN_POS                 (0U)                                                /*!< DSP_CRD_XIN_X_IN bit offset */

/* DSP_CRD_YIN_Y_IN */
#define DSP_CRD_YIN_Y_IN_POS                 (0U)                                                /*!< DSP_CRD_YIN_Y_IN bit offset */

/* DSP_CRD_AMP_AMP_OUT */
#define DSP_CRD_AMP_AMP_OUT_POS              (0U)                                                /*!< DSP_CRD_AMP_AMP_OUT bit offset */

/* DSP_CRD_PHASE_PHASE_OUT */
#define DSP_CRD_PHASE_PHASE_OUT_POS          (0U)                                                /*!< DSP_CRD_PHASE_PHASE_OUT bit offset */

/* DSP_CRD_ANGLE_ANGLE_IN */
#define DSP_CRD_ANGLE_ANGLE_IN_POS           (0U)                                                /*!< DSP_CRD_ANGLE_ANGLE_IN bit offset */

/* DSP_CRD_COSINE_COSINE_OUT */
#define DSP_CRD_COSINE_COSINE_OUT_POS        (0U)                                                /*!< DSP_CRD_COSINE_COSINE_OUT bit offset */

/* DSP_CRD_SINE_SINE_OUT */
#define DSP_CRD_SINE_SINE_OUT_POS            (0U)                                                /*!< DSP_CRD_SINE_SINE_OUT bit offset */

/* DSP_CRD_IE_CRD_VEC_IE */
#define DSP_CRD_IE_CRD_VEC_IE_POS            (0U)                                                /*!< DSP_CRD_IE_CRD_VEC_IE bit offset */

/* DSP_CRD_IE_CRD_ROT_IE */
#define DSP_CRD_IE_CRD_ROT_IE_POS            (1U)                                                /*!< DSP_CRD_IE_CRD_ROT_IE bit offset */

/* DSP_CRD_FLG_CRD_VEC_DONE */
#define DSP_CRD_FLG_CRD_VEC_DONE_POS         (0U)                                                /*!< DSP_CRD_FLG_CRD_VEC_DONE bit offset */

/* DSP_CRD_FLG_CRD_ROT_DONE */
#define DSP_CRD_FLG_CRD_ROT_DONE_POS         (1U)                                                /*!< DSP_CRD_FLG_CRD_ROT_DONE bit offset */

/* DSP_INTP_LEN_INTP_LEN */
#define DSP_INTP_LEN_INTP_LEN_POS            (0U)                                                /*!< DSP_INTP_LEN_INTP_LEN bit offset */

/* DSP_INTP_LOC_INTP_LOC */
#define DSP_INTP_LOC_INTP_LOC_POS            (0U)                                                /*!< DSP_INTP_LOC_INTP_LOC bit offset */

/* DSP_INTP_STEP_INTP_STEP */
#define DSP_INTP_STEP_INTP_STEP_POS          (0U)                                                /*!< DSP_INTP_STEP_INTP_STEP bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_DSP_H */
