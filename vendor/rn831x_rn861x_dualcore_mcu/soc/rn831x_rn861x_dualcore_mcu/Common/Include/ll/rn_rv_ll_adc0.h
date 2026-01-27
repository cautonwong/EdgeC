/**
 ******************************************************************************
 * @file   rn_rv_ll_adc0.h
 * @author Renergy Software Team
 * @brief  Header file of ADC0 LL module driver.
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
#ifndef __RN_RV_LL_ADC0_H
#define __RN_RV_LL_ADC0_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu.h"
#include "rn831x_rn861x_mcu_adc0.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (ADC0_CORE0) || defined (ADC0_CORE1)

/** @defgroup ADC0_LL ADC0
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/** @defgroup ADC0_LL_Private_Constants ADC0 Private Constants
 * @{
 */

/**
 * @}
 */ /* end of group ADC0_LL_Private_Constants */

/* Private macros -----------------------------------------------------------*/
/** @defgroup ADC0_LL_Private_Macros ADC0 Private Macros
 * @{
 */

/* ADC internal channels related definitions */
/* Internal voltage reference VrefInt */
#define VREFINT_CAL_ADDR_OFFSET                       ((0x080U))
#define VREFINT_NORMAL_BGRCODE_ADDR_OFFSET            ((0x088U))                  /* Internal voltage reference, address offset of parameter Normal VREFINT_CAL: VrefInt ADC raw data acquired at  Vref+ = 3.3 V . */
#define VREFINT_BUFFER_BGRCODE_ADDR_OFFSET            ((0x08AU))                  /* Internal voltage reference, address offset of parameter Buffer VREFINT_CAL: VrefInt ADC raw data acquired at  Vref+ = 3.3 V . */
#define VREFINT_CAL_VREF                          ( 3300UL)                   /* Analog voltage reference (Vref+) value(unit: mV). */

/**
 * @}
 */ /* end of group ADC0_LL_Private_Macros */

/* Exported types -----------------------------------------------------------*/
/** @defgroup ADC0_LL_ES_INIT ADC0 Exported Init structure
 * @{
 */

/**
 * @brief  LL ADC0 basic initialization structure
 */
typedef struct
{
    uint32_t Resolution;            /*!< Specifies the resolution value.
                                       This parameter can be a value of @ref ADC0_LL_EC_RESOLUTION.
                                       This feature can be modified afterwards using unitary function @ref LL_ADC0_SetResolution().*/

    uint32_t DataAlignment;         /*!< Specifies the data alignment value.
                                       This parameter can be a value of @ref ADC0_LL_EC_DATA_ALIGN.

                                       This feature can be modified afterwards using unitary function @ref LL_ADC0_SetDataAlignment().*/
    uint32_t ClockDivision;         /*!< Specifies the clockdivision value.
                                       This parameter must be a number between Min_Data=0x0 and Max_Data=0x7F.
                                       This feature can be modified afterwards using unitary function @ref LL_ADC0_SetClockDivision() .*/

} LL_ADC0_InitTypeDef;

/**
 * @brief  LL ADC0 common configuration structure
 */
typedef struct
{
    uint32_t DACConversionDeal;      /*!< Specifies the arbitration behavior when DAC is converting.
                                        This parameter can be a value of @ref ADC0_LL_EC_DACCONV_DEALMODE.
                                        This feature can be modified afterwards using unitary function
                                        @ref LL_ADC0_SetDACConversionArbitrationDealMode() */

    uint32_t DummyConversionCount;   /*!< Specifies the number of dummy conversions after power-up.
                                        This parameter can be a value of @ref ADC0_LL_EC_DUMMY_CONV.
                                        This feature can be modified afterwards using unitary function
                                        @ref LL_ADC0_SetDummyConversionCount() */

    uint32_t ConversionIntervalTime; /*!< Specifies the inter-conversion interval in work-clock cycles.
                                        This parameter must be a number between Min_Data=0x00 and Max_Data=0x1F.
                                        This feature can be modified afterwards using unitary function
                                        @ref LL_ADC0_SetConversionIntervalTime() */

    uint32_t ADCPowerUpTime;         /*!< Specifies the ADC analog power-up stabilization time.
                                        This parameter must be a number between Min_Data=0x00 and Max_Data=0x03.
                                        This feature can be modified afterwards using unitary function
                                        @ref LL_ADC0_SetADCPowerUpTime() */

    uint32_t BufferPowerUpTime;      /*!< Specifies the input buffer power-up stabilization time.
                                        This parameter must be a number between Min_Data=0x00 and Max_Data=0xFF.
                                        This feature can be modified afterwards using unitary function
                                        @ref LL_ADC0_SetBufferPowerUpTime() */
} LL_ADC0_CommonInitTypeDef;

/**
 * @brief  LL ADC0 oversampling configuration structure
 * @note   If oversampling is enabled, data alignment is forced to right alignment by hardware.
 */
typedef struct
{
    uint32_t OverSampling;                  /*!< Enables or disables oversampling.
                                                This parameter can be a value of @ref ADC0_LL_EC_FUNCTION_MODE.
                                               Set to LL_ADC0_FUNCTION_DISABLE or 0, initial function will call @ref LL_ADC0_EnableOverSampling()
                                               Set to LL_ADC0_FUNCTION_ENABLE  or 1, initial function will call @ref LL_ADC0_DisableOverSampling() */

    uint32_t OverSamplingAccumulationCount; /*!< Specifies the oversampling accumulation count.
                                               This parameter can be a value of
                                               @ref ADC0_LL_EC_OverSamplingAccumulationCount.
                                               This feature can be modified afterwards using unitary function
                                               @ref LL_ADC0_SetOverSamplingAccumulationCount() */

    uint32_t OverSamplingRightShift;        /*!< Specifies the right-shift applied to the oversampling result.
                                               This parameter can be a value of
                                               @ref ADC0_LL_EC_OverSamplingRightShift.
                                               This feature can be modified afterwards using unitary function
                                               @ref LL_ADC0_SetOverSamplingRightShift() */

    uint32_t OverSamplingChannels;          /*!< Specifies the channel mask used for oversampling.
                                               This parameter can be a combination of @ref ADC0_LL_EC_CHANNEL.
                                               This feature can be modified afterwards using unitary function
                                               @ref LL_ADC0_SetOverSamplingChannels() */
} LL_ADC0_OversamplingInitTypeDef;

/**
 * @brief  LL ADC0 calibration and digital offset configuration structure
 */
typedef struct
{
    uint32_t NormalZEROT;   /*!< Specifies ZEROT calibration value when input buffer is disabled.
                               This parameter must be a number between Min_Data=0x000 and Max_Data=0xFFF.
                               This feature can be modified afterwards using unitary function
                               @ref LL_ADC0_SetNormalZEROT().
                               Helper macro @ref __LL_ADC0_CALC_NORMAL_ZEROT() can help calculate NormalZEROT value. */
    uint32_t BufferZEROT;   /*!< Specifies ZEROT calibration value when input buffer is enabled.
                               This parameter must be a number between Min_Data=0x000 and Max_Data=0xFFF.
                               This feature can be modified afterwards using unitary function
                               @ref LL_ADC0_SetBufferZEROT().
                               Helper macro @ref __LL_ADC0_CALC_BUFFER_ZEROT() can help calculate BufferZEROT value. */

    uint32_t DigitalOffset; /*!< Specifies the digital offset code (two's complement signed 12-bit).
                               This parameter must be a number between
                               (Min_Data=-2048 and Max_Data=2047 in two's complement).
                               This feature can be modified afterwards using unitary function
                               @ref LL_ADC0_SetDigitalOffset() */
} LL_ADC0_CalibrationTypeDef;

/**
 * @brief  LL ADC0 sequence configuration structure (common for Sequence A/B)
 * @note   In single-channel mode, ChannelSelection holds a channel index 0..13.
 *         In multi-channel mode, ChannelSelection holds a channel bit-mask composed with @ref ADC0_LL_EC_CHANNEL.
 */
typedef struct
{
    uint32_t MultiChannelsMode;    /*!< Enables or disables multi-channel scan mode for the addressed sequence.
                                        This parameter can be a value of @ref ADC0_LL_EC_FUNCTION_MODE.
                                        Set to LL_ADC0_FUNCTION_DISABLE or 0, initial function will call
                                                @ref LL_ADC0_SEQA_DisableMultiChannelsMode(), @ref LL_ADC0_SEQB_DisableMultiChannelsMode(),
                                        Set to LL_ADC0_FUNCTION_ENABLE  or 1, initial function will call
                                                @ref LL_ADC0_SEQA_EnableMultiChannelsMode(), @ref LL_ADC0_SEQA_EnableMultiChannelsMode()*/

    uint32_t ContinuousConversion; /*!< Enables or disables continuous conversion for the addressed sequence.
                                        This parameter can be a value of @ref ADC0_LL_EC_FUNCTION_MODE.
                                        Set to LL_ADC0_FUNCTION_DISABLE or 0, initial function will call
                                                @ref LL_ADC0_SEQA_DisableContinuousConversion(), @ref LL_ADC0_SEQB_DisableContinuousConversion(),
                                        Set to LL_ADC0_FUNCTION_ENABLE  or 1, initial function will call
                                                @ref LL_ADC0_SEQA_EnableContinuousConversion(), @ref LL_ADC0_SEQB_EnableContinuousConversion() */

    uint32_t ChannelSelection;     /*!< Specifies the channel selection for the addressed sequence.
                                      - If multi-channel mode is DISABLE: this field is a channel index between 0 and 13,
                                        and can be modified afterwards using unitary functions
                                        @ref LL_ADC0_SEQA_SetChannel() or @ref LL_ADC0_SEQB_SetChannel().
                                      - If multi-channel mode is ENABLE: this field is a channel mask composed with
                                        @ref ADC0_LL_EC_CHANNEL, and can be modified afterwards using unitary functions
                                        @ref LL_ADC0_SEQA_SetSequenceChannels() or @ref LL_ADC0_SEQB_SetSequenceChannels() */

    uint32_t SequenceDiscont;      /*!< Specifies the discontinuous scan configuration.
                                      This parameter can be a value of @ref ADC0_LL_EC_SEQUENCE_DISCONT.
                                      This feature can be modified afterwards using unitary functions
                                      @ref LL_ADC0_SEQA_SetSequenceDiscont() or @ref LL_ADC0_SEQB_SetSequenceDiscont() */

    uint32_t OverflowMode;         /*!< Specifies the overflow handling mode.
                                      This parameter can be a value of @ref ADC0_LL_EC_OVERFLOW_MODE.
                                      This feature can be modified afterwards using unitary functions
                                      @ref LL_ADC0_SEQA_SetOverflowMode() or @ref LL_ADC0_SEQB_SetOverflowMode() */

    uint32_t PreemptMode;          /*!< Specifies the preempt mode.
                                      This parameter can be a value of @ref ADC0_LL_EC_PREEMPT_MODE.
                                      Only applicable to Sequence B and can be modified afterwards using unitary function
                                      @ref LL_ADC0_SEQB_SetPreemptMode(); ignored for Sequence A */
} LL_ADC0_SeqInitTypeDef;

/**
 * @brief  LL ADC0 analog watchdog configuration structure (common for Sequence A/B)
 */
typedef struct
{
    uint32_t AWDMode;            /*!< Specifies the analog watchdog mode.
                                   This parameter can be a value of @ref ADC0_LL_EC_AWD_MODE.
                                   This feature can be modified afterwards using unitary functions
                                   @ref LL_ADC0_SEQA_SetAnalogWDMode() or @ref LL_ADC0_SEQB_SetAnalogWDMode() */

    uint32_t AWDThresholdHigh;   /*!< Specifies the analog watchdog high threshold.
                                   This parameter must be a number between Min_Data=0x0000 and Max_Data=0xFFFF.
                                   This feature can be modified afterwards using unitary functions
                                   @ref LL_ADC0_SEQA_SetAnalogWDThresholds() or @ref LL_ADC0_SEQB_SetAnalogWDThresholds() */

    uint32_t AWDThresholdLow;    /*!< Specifies the analog watchdog low threshold.
                                   This parameter must be a number between Min_Data=0x0000 and Max_Data=0xFFFF.
                                   This feature can be modified afterwards using unitary functions
                                   @ref LL_ADC0_SEQA_SetAnalogWDThresholds() or @ref LL_ADC0_SEQB_SetAnalogWDThresholds() */

    uint32_t AWDMonitorChannels; /*!< Specifies the channels to be monitored by the analog watchdog.
                                   This parameter can be a combination of @ref ADC0_LL_EC_CHANNEL.
                                   This feature can be modified afterwards using unitary functions
                                   @ref LL_ADC0_SEQA_SetAnalogWDMonitChannels() or @ref LL_ADC0_SEQB_SetAnalogWDMonitChannels() */
} LL_ADC0_SeqAWDInitTypeDef;

/**
 * @brief  LL ADC0 DMA configuration structure for a sequence (common for Sequence A/B)
 */
typedef struct
{
    uint32_t DMATransfer;    /*!< Specifies the DMA transfer configuration.
                                This parameter can be a value of @ref ADC0_LL_EC_DMA_TRANSFER.
                                This feature can be modified afterwards using unitary functions
                                @ref LL_ADC0_SEQA_SetDMATransfer() or @ref LL_ADC0_SEQB_SetDMATransfer() */

    uint32_t DMABaseAddress; /*!< Specifies the DMA receive base address for the addressed sequence.
                                This parameter must be a number between Min_Data=0x00000 and Max_Data=0x3FFFF.
                                This feature can be modified afterwards using unitary functions
                                @ref LL_ADC0_SEQA_SetDMABaseAddress() or @ref LL_ADC0_SEQB_SetDMABaseAddress() */

    uint32_t DMALength;      /*!< Specifies the DMA transfer length for the addressed sequence.
                                This parameter must be a number between Min_Data=0x00000 and Max_Data=0x3FFFF.
                                This feature can be modified afterwards using unitary functions
                                @ref LL_ADC0_SEQA_SetDMALength() or @ref LL_ADC0_SEQB_SetDMALength() */
} LL_ADC0_SeqDMAInitTypeDef;

/**
 * @}
 */ /* end of group ADC0_LL_ES_INIT */

/* Exported constants -------------------------------------------------------*/
/** @defgroup ADC0_LL_Exported_Constants ADC0 Exported Constants
 * @{
 */

/** @defgroup ADC0_LL_EC_SEQUENCE   Sequence select
 * @{
 */
#define LL_ADC0_SEQA   (1U)
#define LL_ADC0_SEQB   (2U)
/**
 * @}
 */ /* end of group ADC0_LL_EC_SEQUENCE   */

/** @defgroup ADC0_LL_EC_FUNCTION_MODE    Function mode
 * @{
 */
#define LL_ADC0_FUNCTION_DISABLE   (0U)
#define LL_ADC0_FUNCTION_ENABLE    (1U)
/**
 * @}
 */ /* end of group ADC0_LL_EC_FUNCTION_MODE */

/** @defgroup ADC0_LL_EC_THRESHOLD threshold
 * @{
 */
#define LL_ADC0_AWD_THRESHOLD_HIGH      (0U)
#define LL_ADC0_AWD_THRESHOLD_LOW       (1U)
/**
 * @}
 */ /* end of group ADC0_LL_EC_THRESHOLD */

/** @defgroup ADC0_LL_EC_RESOLUTION Resolution
 * @{
 */
#define LL_ADC0_RESOLUTION_12B         ADC0_CFG_RESC_12BIT          /*!< ADC resolution 12 bits */
#define LL_ADC0_RESOLUTION_10B         ADC0_CFG_RESC_10BIT          /*!< ADC resolution 10 bits */
#define LL_ADC0_RESOLUTION_8B          ADC0_CFG_RESC_8BIT           /*!< ADC resolution  8 bits */
#define LL_ADC0_RESOLUTION_6B          ADC0_CFG_RESC_6BIT           /*!< ADC resolution  6 bits */

/**
 * @}
 */ /* end of group ADC0_LL_EC_RESOLUTION */

/** @defgroup ADC0_LL_EC_DATA_ALIGN Data alignment
 * @{
 */
#define LL_ADC0_DATA_ALIGN_RIGHT       ADC0_CFG_DFMT_RA                  /*!< Right-aligned data */
#define LL_ADC0_DATA_ALIGN_LEFT        ADC0_CFG_DFMT_LA                  /*!< Left-aligned data */
/**
 * @}
 */ /* end of group ADC0_LL_EC_DATA_ALIGN */

/** @defgroup ADC0_LL_EC_DACCONV_DEALMODE ADC deal mode when adc is converting
 * @{
 */
#define LL_ADC0_DACCONV_DEALMODE_CONTINUE    ADC0_CFG_HALT_2DAC_CONTINUE     /*!< ADC0 operates normally regardless of DAC status */
#define LL_ADC0_DACCONV_DEALMODE_PAUSE       ADC0_CFG_HALT_2DAC_PAUSE        /*!< ADC0 pauses conversion when DAC is in conversion state */

/**
 * @}
 */ /* end of group ADC0_LL_EC_DACCONV_DEALMODE */

/** @defgroup ADC0_LL_EC_OverSamplingAccumulationCount OverSampling Accumulation Count
 * @{
 */
#define LL_ADC0_OVERSAMPLING_ACCUMULATE_2           ADC0_OVSCFG_OVSR_ACC2        /*!< Accumulate 2 times */
#define LL_ADC0_OVERSAMPLING_ACCUMULATE_4           ADC0_OVSCFG_OVSR_ACC4        /*!< Accumulate 4 times */
#define LL_ADC0_OVERSAMPLING_ACCUMULATE_8           ADC0_OVSCFG_OVSR_ACC8        /*!< Accumulate 8 times */
#define LL_ADC0_OVERSAMPLING_ACCUMULATE_16          ADC0_OVSCFG_OVSR_ACC16       /*!< Accumulate 16 times */
#define LL_ADC0_OVERSAMPLING_ACCUMULATE_32          ADC0_OVSCFG_OVSR_ACC32       /*!< Accumulate 32 times */
#define LL_ADC0_OVERSAMPLING_ACCUMULATE_64          ADC0_OVSCFG_OVSR_ACC64       /*!< Accumulate 64 times */
#define LL_ADC0_OVERSAMPLING_ACCUMULATE_128         ADC0_OVSCFG_OVSR_ACC128      /*!< Accumulate 128 times */
#define LL_ADC0_OVERSAMPLING_ACCUMULATE_256         ADC0_OVSCFG_OVSR_ACC256      /*!< Accumulate 256 times */
/**
 * @}
 */

/** @defgroup ADC0_LL_EC_OverSamplingRightShift OverSampling Right Shift
 * @{
 */
#define LL_ADC0_OVERSAMPLING_SHIFT_NONE             ADC0_OVSCFG_OVSS_SHLNUL      /*!< No shift */
#define LL_ADC0_OVERSAMPLING_SHIFT_1                ADC0_OVSCFG_OVSS_SHL1        /*!< Shift 1 bit */
#define LL_ADC0_OVERSAMPLING_SHIFT_2                ADC0_OVSCFG_OVSS_SHL2        /*!< Shift 2 bits */
#define LL_ADC0_OVERSAMPLING_SHIFT_3                ADC0_OVSCFG_OVSS_SHL3        /*!< Shift 3 bits */
#define LL_ADC0_OVERSAMPLING_SHIFT_4                ADC0_OVSCFG_OVSS_SHL4        /*!< Shift 4 bits */
#define LL_ADC0_OVERSAMPLING_SHIFT_5                ADC0_OVSCFG_OVSS_SHL5        /*!< Shift 5 bits */
#define LL_ADC0_OVERSAMPLING_SHIFT_6                ADC0_OVSCFG_OVSS_SHL6        /*!< Shift 6 bits */
#define LL_ADC0_OVERSAMPLING_SHIFT_7                ADC0_OVSCFG_OVSS_SHL7        /*!< Shift 7 bits */
#define LL_ADC0_OVERSAMPLING_SHIFT_8                ADC0_OVSCFG_OVSS_SHL8        /*!< Shift 8 bits */
/**
 * @}
 */

/** @defgroup ADC0_LL_EC_CHANNEL ADC Channel numbers.
 * @{
 */
#define LL_ADC0_CHANNEL_NULL         ((uint32_t)0x00000000U)                     /*!< select null channel */
#define LL_ADC0_CHANNEL_ALL        ((uint32_t)0x00003FFFU)                     /*!< select all channels */
#define LL_ADC0_CHANNEL_0           ((uint32_t)((uint32_t)0x01U << (0 )))       /*!< select ADC Channel 0 */
#define LL_ADC0_CHANNEL_1           ((uint32_t)((uint32_t)0x01U << (1 )))       /*!< select ADC Channel 1 */
#define LL_ADC0_CHANNEL_2           ((uint32_t)((uint32_t)0x01U << (2 )))       /*!< select ADC Channel 2 */
#define LL_ADC0_CHANNEL_3           ((uint32_t)((uint32_t)0x01U << (3 )))       /*!< select ADC Channel 3 */
#define LL_ADC0_CHANNEL_4           ((uint32_t)((uint32_t)0x01U << (4 )))       /*!< select ADC Channel 4 */
#define LL_ADC0_CHANNEL_5           ((uint32_t)((uint32_t)0x01U << (5 )))       /*!< select ADC Channel 5 */
#define LL_ADC0_CHANNEL_6           ((uint32_t)((uint32_t)0x01U << (6 )))       /*!< select ADC Channel 6 */
#define LL_ADC0_CHANNEL_7           ((uint32_t)((uint32_t)0x01U << (7 )))       /*!< select ADC Channel 7 */
#define LL_ADC0_CHANNEL_8           ((uint32_t)((uint32_t)0x01U << (8 )))       /*!< select ADC Channel 8 */
#define LL_ADC0_CHANNEL_9           ((uint32_t)((uint32_t)0x01U << (9 )))       /*!< select ADC Channel 9 */
#define LL_ADC0_CHANNEL_10          ((uint32_t)((uint32_t)0x01U << (10)))       /*!< select ADC Channel 10 */
#define LL_ADC0_CHANNEL_11          ((uint32_t)((uint32_t)0x01U << (11)))       /*!< select ADC Channel 11 */
#define LL_ADC0_CHANNEL_12          ((uint32_t)((uint32_t)0x01U << (12)))       /*!< select ADC Channel 12 */
#define LL_ADC0_CHANNEL_13          ((uint32_t)((uint32_t)0x01U << (13)))       /*!< select ADC Channel 13 */
/**
 * @}
 */ /* end of group ADC0_LL_EC_CHANNEL */

/** @defgroup ADC0_LL_EC_ADCPowerMode ADC Power Mode
 * @{
 */
#define LL_ADC0_POWER_MODE_ALWAYS_ON                ADC0_PWDCTRL_ADC0_AON_ALWAYSON   /*!< ADC analog circuit always on */
#define LL_ADC0_POWER_MODE_DEMAND_ON                ADC0_PWDCTRL_ADC0_AON_DEMANDON   /*!< ADC analog circuit only turns on when working */
/**
 * @}
 */

/** @defgroup ADC0_LL_EC_BufferPowerMode Buffer Power Mode
 * @{
 */
#define LL_ADC0_BUFFER_POWER_MODE_ALWAYS_ON         ADC0_PWDCTRL_BUFF_AON_ALWAYSON   /*!< Buffer analog circuit always on */
#define LL_ADC0_BUFFER_POWER_MODE_DEMAND_ON         ADC0_PWDCTRL_BUFF_AON_DEMANDON   /*!< Buffer analog circuit only turns on when working */
/**
 * @}
 */

/** @defgroup ADC0_LL_EC_DUMMY_CONV Dummy Conversion Count
 * @{
 */
#define LL_ADC0_DUMMY_CONV_0                 (0x00000000U)                    /*!< No dummy conversion */
#define LL_ADC0_DUMMY_CONV_1                 (0x00000001U)                    /*!< 1 dummy conversion */
#define LL_ADC0_DUMMY_CONV_2                 (0x00000002U)                    /*!< 2 dummy conversions */
#define LL_ADC0_DUMMY_CONV_3                 (0x00000003U)                    /*!< 3 dummy conversions */
/**
 * @}
 */ /* end of group ADC0_LL_EC_DUMMY_CONV */

/** @defgroup ADC0_LL_EC_SEQUENCE_DISCONT Sequence Discontinuous Configuration
 * @{
 */
#define LL_ADC0_SEQUENCE_DISCONT_DISABLE     ((uint32_t)0x00000000U)                    /*!< Discontinuous scan disabled */
#define LL_ADC0_SEQUENCE_DISCONT_1CH         (ADC0_CTRLA_DSICEN | (0U << ADC0_CTRLA_DSICNUM_POS))  /*!< Discontinuous scan 1 channel */
#define LL_ADC0_SEQUENCE_DISCONT_2CH         (ADC0_CTRLA_DSICEN | (1U << ADC0_CTRLA_DSICNUM_POS))  /*!< Discontinuous scan 2 channels */
#define LL_ADC0_SEQUENCE_DISCONT_3CH         (ADC0_CTRLA_DSICEN | (2U << ADC0_CTRLA_DSICNUM_POS))  /*!< Discontinuous scan 3 channels */
#define LL_ADC0_SEQUENCE_DISCONT_4CH         (ADC0_CTRLA_DSICEN | (3U << ADC0_CTRLA_DSICNUM_POS))  /*!< Discontinuous scan 4 channels */
/**
 * @}
 */ /* end of group ADC0_LL_EC_SEQUENCE_DISCONT */

/** @defgroup ADC0_LL_EC_DMA_TRANSFER DMA Transfer Configuration
 * @{
 */
#define LL_ADC0_DMA_TRANSFER_DISABLE         ((uint32_t)0x00000000U)                    /*!< DMA transfer disabled */
#define LL_ADC0_DMA_TRANSFER_ONCE            (ADC0_CTRLA_DMAEN)                         /*!< DMA transfer once */
#define LL_ADC0_DMA_TRANSFER_CONTINUOUS      (ADC0_CTRLA_DMAEN | ADC0_CTRLA_DMA_MODE)   /*!< DMA transfer continuous */
/**
 * @}
 */ /* end of group ADC0_LL_EC_DMA_TRANSFER */

/** @defgroup ADC0_LL_EC_OVERFLOW_MODE Overflow Mode
 * @{
 */
#define LL_ADC0_OVERFLOW_MODE_NO_RECOVER     (ADC0_CTRLA_OVRMOD_NUL)                    /*!< Do not recover data when overflow */
#define LL_ADC0_OVERFLOW_MODE_RECOVER        (ADC0_CTRLA_OVRMOD_RECOVER)                /*!< Recover data when overflow */
/**
 * @}
 */ /* end of group ADC0_LL_EC_OVERFLOW_MODE */

/** @defgroup ADC0_LL_EC_AWD_MODE Analog Watchdog Mode
 * @{
 */
#define LL_ADC0_AWD_MODE_DISABLE_AWD         ((uint32_t)0x00000000U)                       /*!< Disable AWD */
#define LL_ADC0_AWD_MODE_BETWEEN_HIGH_LOW    (ADC0_CTRLA_AWDEN | ADC0_CTRLA_AWDMOD_PHL)   /*!< enable AWD Protection range: AWD Low threshold < DATA < AWD High threshold */
#define LL_ADC0_AWD_MODE_ABOVE_LOW           (ADC0_CTRLA_AWDEN | ADC0_CTRLA_AWDMOD_PL)    /*!< enable AWD Protection range: AWD Low threshold < DATA */
#define LL_ADC0_AWD_MODE_BELOW_HIGH          (ADC0_CTRLA_AWDEN | ADC0_CTRLA_AWDMOD_PH)    /*!< enable AWD Protection range: DATA < AWD High threshold */
/**
 * @}
 */ /* end of group ADC0_LL_EC_AWD_MODE */

/** @defgroup ADC0_LL_EC_PREEMPT_MODE Preempt Mode
 * @{
 */
#define LL_ADC0_PREEMPT_MODE_STOP            (ADC0_CTRLB_SMODE_STOP)                    /*!< Force abort conversion and switch to sequence A */
#define LL_ADC0_PREEMPT_MODE_WAIT            (ADC0_CTRLB_SMODE_WAIT)                    /*!< Wait for current conversion to complete before switching to sequence A */
/**
 * @}
 */ /* end of group ADC0_LL_EC_PREEMPT_MODE */

/** @defgroup ADC0_LL_EC_BASEADDR base address
 * @{
 */
#define LL_ADC0_CORE0_BASE            (ADC0_CORE0_BASE)
#define LL_ADC0_CORE1_BASE            (ADC0_CORE1_BASE)
/**
 * @}
 */ /* end of group ADC0_LL_EC_BASEADDR */

/** @defgroup ADC0_LL_EC_DATA_FULLSCALE  data full scale
 * @{
 */
#define LL_ADC0_DATA_FULLSCALE_6B            ((uint32_t)((uint32_t)0x01U << (6U )))         /*!< data full scale = 2^6 */
#define LL_ADC0_DATA_FULLSCALE_8B            ((uint32_t)((uint32_t)0x01U << (8U )))         /*!< data full scale = 2^8 */
#define LL_ADC0_DATA_FULLSCALE_10B           ((uint32_t)((uint32_t)0x01U << (10U)))         /*!< data full scale = 2^10 */
#define LL_ADC0_DATA_FULLSCALE_12B           ((uint32_t)((uint32_t)0x01U << (12U)))         /*!< data full scale = 2^12 */
#define LL_ADC0_DATA_FULLSCALE_13B           ((uint32_t)((uint32_t)0x01U << (13U)))         /*!< data full scale = 2^13 */
#define LL_ADC0_DATA_FULLSCALE_14B           ((uint32_t)((uint32_t)0x01U << (14U)))         /*!< data full scale = 2^14 */
#define LL_ADC0_DATA_FULLSCALE_15B           ((uint32_t)((uint32_t)0x01U << (15U)))         /*!< data full scale = 2^15 */
#define LL_ADC0_DATA_FULLSCALE_16B           ((uint32_t)((uint32_t)0x01U << (16U)))         /*!< data full scale = 2^16 */
/**
 * @}
 */ /* end of group ADC0_LL_EC_BASEADDR */

/**
 * @}
 */ /* end of group ADC0_LL_Exported_Constants */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup ADC0_LL_Exported_Macros ADC0 Exported Macros
 * @{
 */

/** @defgroup ADC0_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in ADC0 register
 * @param  __INSTANCE__ ADC0 Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_ADC0_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in ADC0 register
 * @param  __INSTANCE__ ADC0 Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_ADC0_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)

/**
 * @}
 */

/** @defgroup ADC_LL_EM_HELPER_MACRO ADC helper macro
 * @{
 */
/**
 * @brief  Helper macro to calculate the normal voltage (unit: mVolt)
 *         (for calculate voltage without enable input buffer, hardware calibration enabled)
 * @param  __ADC0_DATA__ ADC conversion data (unit: digital value).
 *         data geted by @ref LL_ADC0_ReadConversionData.
 * @param  __ADC0_DATA_FULLSCALE__ Full scale of the data after conversion
 *         This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_6B
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_8B
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_10B
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_12B
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_13B
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_14B
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_15B
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_16B
 * @param  __ADC0_CORE_OFFSET__ This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_CORE0_BASE
 *         @arg @ref LL_ADC0_CORE1_BASE
 * @retval ADC conversion data equivalent voltage value (unit: mVolt)
 */
#define __LL_ADC0_CALC_NORMAL_DATA_TO_VOLTAGE(__ADC0_DATA__,__ADC0_DATA_FULLSCALE__,\
                                      __ADC0_CORE_OFFSET__)                      \
  (((__ADC0_DATA__)  * VREFINT_CAL_VREF)                                   \
   / ((__ADC0_DATA_FULLSCALE__) * (((uint32_t)(*((uint32_t*) (__ADC0_CORE_OFFSET__ + VREFINT_CAL_ADDR_OFFSET))) >> ADC0_CAL_ZTG_POS ) & ADC0_CAL_ZTG)))
/**
 * @brief  Helper macro to calculate the buffer voltage (unit: mVolt)
 *         (for calculate voltage enabled input buffer, hardware calibration enabled)
 * @param  __ADC0_DATA__ ADC conversion data (unit: digital value).
 *         data geted by @ref LL_ADC0_ReadConversionData.
 * @param  __ADC0_DATA_FULLSCALE__ Full scale of the data after conversion
 *         This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_6B
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_8B
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_10B
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_12B
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_13B
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_14B
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_15B
 *         @arg @ref LL_ADC0_DATA_FULLSCALE_16B
 * @param  __ADC0_CORE_OFFSET__ This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_CORE0_BASE
 *         @arg @ref LL_ADC0_CORE1_BASE
 * @retval ADC conversion data equivalent voltage value (unit: mVolt)
 */
#define __LL_ADC0_CALC_BUFFER_DATA_TO_VOLTAGE(__ADC0_DATA__,__ADC0_DATA_FULLSCALE__,\
                                      __ADC0_CORE_OFFSET__)                      \
  (((__ADC0_DATA__)  * VREFINT_CAL_VREF)                                   \
   / ((__ADC0_DATA_FULLSCALE__) * (((uint32_t)(*((uint32_t*) (__ADC0_CORE_OFFSET__ + VREFINT_CAL_ADDR_OFFSET))) >> ADC0_CAL_ZTBG_POS ) & ADC0_CAL_ZTBG)))

/**
 * @brief  Helper macro to calculate the calibration NORMAL ZEROT
 *         (for calculate voltage disabled input buffer, hardware calibration disabled, oversampling enabled)
 * @param  __ADC0_DATA__ ADC conversion data (unit: digital value).
 *         data geted by @ref LL_ADC0_ReadConversionData.
 * @param  __ADC0_RIGHTSHIFT__ This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_NONE
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_1
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_2
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_3
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_4
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_5
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_6
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_7
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_8
 * @param  __ADC0_ACCUMULATIONCOUNT__ This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_2
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_4
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_8
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_16
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_32
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_64
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_128
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_256
 * @param  __ADC0_CORE_OFFSET__ This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_CORE0_BASE
 *         @arg @ref LL_ADC0_CORE1_BASE
 * @retval ADC conversion data equivalent voltage value (unit: mVolt)
 */
#define __LL_ADC0_CALC_NORMAL_ZEROT(__ADC0_DATA__,__ADC0_DATA_FULLSCALE__,\
                                      __ADC0_CORE_OFFSET__)                      \
  (((__ADC0_DATA__) *(1U << (__ADC0_RIGHTSHIFT__ + 4U)) )                                   \
   / ((1U <<((__ADC0_ACCUMULATIONCOUNT__>>1U) +1U)) * ((uint32_t)(*((uint16_t*) (__ADC0_CORE_OFFSET__ + VREFINT_NORMAL_BGRCODE_ADDR_OFFSET))))))

/**
 * @brief  Helper macro to calculate the calibration BUFFER ZEROT
 *         (for calculate voltage enabled input buffer, hardware calibration disabled, oversampling enabled)
 * @param  __ADC0_DATA__ ADC conversion data (unit: digital value).
 *         data geted by @ref LL_ADC0_ReadConversionData.
 * @param  __ADC0_RIGHTSHIFT__ This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_NONE
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_1
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_2
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_3
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_4
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_5
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_6
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_7
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_8
 * @param  __ADC0_ACCUMULATIONCOUNT__ This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_2
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_4
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_8
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_16
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_32
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_64
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_128
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_256
 * @param  __ADC0_CORE_OFFSET__ This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_CORE0_BASE
 *         @arg @ref LL_ADC0_CORE1_BASE
 * @retval ADC conversion data equivalent voltage value (unit: mVolt)
 */
#define __LL_ADC0_CALC_BUFFER_ZEROT(__ADC0_DATA__,__ADC0_DATA_FULLSCALE__,\
                                      __ADC0_CORE_OFFSET__)                      \
  (((__ADC0_DATA__) *(1U << (__ADC0_RIGHTSHIFT__ + 4U)) )                                   \
   / ((1U <<((__ADC0_ACCUMULATIONCOUNT__>>1U) +1U)) * ((uint32_t)(*((uint16_t*) (__ADC0_CORE_OFFSET__ + VREFINT_BUFFER_BGRCODE_ADDR_OFFSET))))))

/**
 * @}
 */ /* end of group ADC_LL_EM_HELPER_MACRO */

/**
 * @}
 */ /* end of group ADC0_LL_Exported_Macros */

/* Exported functions -------------------------------------------------------*/
/** @defgroup ADC0_LL_Exported_Functions ADC0 Exported Functions
 * @{
 */

/** @defgroup ADC0_LL_EF_Configuration Configuration functions
 * @{
 */
/**
 * @brief  Set the ADC0 resolution.
 *         Refer to reference manual for alignments formats
 *         dependencies to ADC resolutions.
 * @rmtoll CFG         RESC            LL_ADC0_SetResolution
 * @param  ADC0x ADC0 Instance
 * @param  Resolution This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_RESOLUTION_12B
 *         @arg @ref LL_ADC0_RESOLUTION_10B
 *         @arg @ref LL_ADC0_RESOLUTION_8B
 *         @arg @ref LL_ADC0_RESOLUTION_6B
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetResolution(ADC0_TypeDef *ADC0x, uint32_t Resolution)
{
    MODIFY_REG(ADC0x->CFG, ADC0_CFG_RESC, Resolution);
}

/**
 * @brief  Get ADC resolution.
 *         Refer to reference manual for alignments formats
 *         dependencies to ADC resolutions.
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion and  @ref LL_ADC0_SEQB_StopConversion called before.
 * @rmtoll CFG         RESC            LL_ADC0_GetResolution
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_RESOLUTION_12B
 *         @arg @ref LL_ADC0_RESOLUTION_10B
 *         @arg @ref LL_ADC0_RESOLUTION_8B
 *         @arg @ref LL_ADC0_RESOLUTION_6B
 */
__STATIC_INLINE uint32_t LL_ADC0_GetResolution(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CFG, ADC0_CFG_RESC));
}

/**
 * @brief  Set the conversion data alignment.
 * @rmtoll CFG         DFMT            LL_ADC0_SetDataAlignment
 * @param  ADC0x ADC0 Instance
 * @param  DataAlignment This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_DATA_ALIGN_RIGHT
 *         @arg @ref LL_ADC0_DATA_ALIGN_LEFT
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetDataAlignment(ADC0_TypeDef *ADC0x, uint32_t DataAlignment)
{
    MODIFY_REG(ADC0x->CFG, ADC0_CFG_DFMT, DataAlignment);
}

/**
 * @brief  Get the conversion data alignment.
 * @rmtoll CFG         DFMT            LL_ADC0_GetDataAlignment
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_DATA_ALIGN_RIGHT
 *         @arg @ref LL_ADC0_DATA_ALIGN_LEFT
 */
__STATIC_INLINE uint32_t LL_ADC0_GetDataAlignment(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CFG, ADC0_CFG_DFMT));
}

/**
 * @brief  Enable Data register auto-clear.
 * @rmtoll CFG         DCLREN         LL_ADC0_EnableAutoClear
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableAutoClear(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->CFG, ADC0_CFG_DCLREN);
}

/**
 * @brief  Disable Data register auto-clear.
 * @rmtoll CFG         DCLREN         LL_ADC0_DisableAutoClear
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableAutoClear(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->CFG, ADC0_CFG_DCLREN);
}

/**
 * @brief  Indicates whether the Data register auto-clear is enabled.
 * @rmtoll CFG         DCLREN         LL_ADC0_IsEnabledAutoClear
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledAutoClear(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->CFG, ADC0_CFG_DCLREN) == (ADC0_CFG_DCLREN)) ? 1UL : 0UL);
}

/**
 * @brief  Set the ADC deal mode when DAC is Converting.
 * @rmtoll CFG         HALT_2DAC            LL_ADC0_SetDACConversionArbitrationDealMode
 * @param  ADC0x ADC0 Instance
 * @param  DealMode This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_DACCONV_DEALMODE_CONTINUE
 *         @arg @ref LL_ADC0_DACCONV_DEALMODE_PAUSE
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetDACConversionArbitrationDealMode(ADC0_TypeDef *ADC0x, uint32_t DealMode)
{
    MODIFY_REG(ADC0x->CFG, ADC0_CFG_HALT_2DAC, DealMode);
}

/**
 * @brief  Get the ADC deal mode when DAC is Converting.
 * @rmtoll CFG         HALT_2DAC            LL_ADC0_GetDACConversionArbitrationDealMode
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_DACCONV_DEALMODE_CONTINUE
 *         @arg @ref LL_ADC0_DACCONV_DEALMODE_PAUSE
 */
__STATIC_INLINE uint32_t LL_ADC0_GetDACConversionArbitrationDealMode(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CFG, ADC0_CFG_HALT_2DAC));
}

/**
 * @brief  Enable input buffer for the ADC.
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion and  @ref LL_ADC0_SEQB_StopConversion called before.
 * @rmtoll CFG         BUFF_EN         LL_ADC0_EnableBuffer
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableBuffer(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->CFG, ADC0_CFG_BUFF_EN);
}

/**
 * @brief  Disable input buffer for the ADC.
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion and  @ref LL_ADC0_SEQB_StopConversion called before.
 * @rmtoll CFG         BUFF_EN         LL_ADC0_DisableBuffer
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableBuffer(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->CFG, ADC0_CFG_BUFF_EN);
}

/**
 * @brief  Indicates whether the input buffer for the ADC is enabled.
 * @rmtoll CFG         BUFF_EN         LL_ADC0_IsEnabledBuffer
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledBuffer(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->CFG, ADC0_CFG_BUFF_EN) == (ADC0_CFG_BUFF_EN)) ? 1UL : 0UL);
}

/**
 * @brief  Set the BUFFER power-up stabilization time.
 * @note   Time =(value+32)*Tworkclk （Refer to reference manual for recommend value）
 * @rmtoll CFG         BUFF_PWRUPT            LL_ADC0_SetBufferPowerUpTime
 * @param  ADC0x ADC0 Instance
 * @param  BufferPowerUpTime Between 0 to 0xFF.
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetBufferPowerUpTime(ADC0_TypeDef *ADC0x, uint32_t BufferPowerUpTime)
{
    MODIFY_REG(ADC0x->CFG, ADC0_CFG_BUFF_PWRUPT, BufferPowerUpTime);
}

/**
 * @brief  Get the BUFFER power-up stabilization time.
 * @rmtoll CFG         BUFF_PWRUPT            LL_ADC0_GetBufferPowerUpTime
 * @param  ADC0x ADC0 Instance
 * @retval BUFFER power-up stabilization time
 */
__STATIC_INLINE uint32_t LL_ADC0_GetBufferPowerUpTime(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CFG, ADC0_CFG_BUFF_PWRUPT));
}

/**
 * @brief  Set the ADC0 power-up stabilization time.
 * @note   Time = (value+32)*Tworkclk(refer to reference manual for detail)
 * @rmtoll CFG         ADC0_PWRUPT            LL_ADC0_SetADCPowerUpTime
 * @param  ADC0x ADC0 Instance.
 * @param  ADCPowerUpTime Between 0 to 0x3.
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetADCPowerUpTime(ADC0_TypeDef *ADC0x, uint32_t ADCPowerUpTime)
{
    MODIFY_REG(ADC0x->CFG, ADC0_CFG_ADC0_PWRUPT, (ADCPowerUpTime << ADC0_CFG_ADC0_PWRUPT_POS));
}

/**
 * @brief  Get the ADCPowerUpTime.
 * @rmtoll CFG         ADC0_PWRUPT            LL_ADC0_GetADCPowerUpTime
 * @param  ADC0x ADC0 Instance.
 * @retval ADC0 power-up stabilization time(Time =(value+32)*Tworkclk)
 */
__STATIC_INLINE uint32_t LL_ADC0_GetADCPowerUpTime(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CFG, ADC0_CFG_ADC0_PWRUPT) >> ADC0_CFG_ADC0_PWRUPT_POS);
}

/**
 * @brief  Set the conversion interval time.
 *         configs Wait Tworkclks between conversions
 * @rmtoll CFG         INTV_TIME            LL_ADC0_SetConversionIntervalTime
 * @param  ADC0x ADC0 Instance.
 * @param  ConversionIntervalTime Between 0 to 0xF.
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetConversionIntervalTime(ADC0_TypeDef *ADC0x, uint32_t ConversionIntervalTime)
{
    MODIFY_REG(ADC0x->CFG, ADC0_CFG_INTV_TIME, (ConversionIntervalTime << ADC0_CFG_INTV_TIME_POS));
}

/**
 * @brief  Get the ConversionIntervalTime.
 * @rmtoll CFG         INTV_TIME            LL_ADC0_GetConversionIntervalTime
 * @param  ADC0x ADC0 Instance.
 * @retval conversion interval time
 */
__STATIC_INLINE uint32_t LL_ADC0_GetConversionIntervalTime(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CFG, ADC0_CFG_INTV_TIME) >> ADC0_CFG_INTV_TIME_POS);
}

/**
 * @brief  Set the clock division
 * @note   ADC0 module working clock is bus clock divided by (CLKDIV+1)
 * @rmtoll CFG         CLKDIV            LL_ADC0_SetClockDivision
 * @param  ADC0x ADC0 Instance
 * @param  ClockDivision Between 0 to 0x7F
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetClockDivision(ADC0_TypeDef *ADC0x, uint32_t ClockDivision)
{
    MODIFY_REG(ADC0x->CFG, ADC0_CFG_CLKDIV, (ClockDivision << ADC0_CFG_CLKDIV_POS));
}

/**
 * @brief  Get the clock division
 * @rmtoll CFG         CLKDIV            LL_ADC0_GetClockDivision
 * @param  ADC0x ADC0 Instance
 * @retval Clock division configuration
 */
__STATIC_INLINE uint32_t LL_ADC0_GetClockDivision(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CFG, ADC0_CFG_CLKDIV) >> ADC0_CFG_CLKDIV_POS);
}

/**
 * @brief  Set the dummy conversion count
 * @rmtoll DCONV         DCONV            LL_ADC0_SetDummyConversionCount
 * @param  ADC0x ADC0 Instance
 * @param  DummyConversionCount This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_DUMMY_CONV_0
 *         @arg @ref LL_ADC0_DUMMY_CONV_1
 *         @arg @ref LL_ADC0_DUMMY_CONV_2
 *         @arg @ref LL_ADC0_DUMMY_CONV_3
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetDummyConversionCount(ADC0_TypeDef *ADC0x, uint32_t DummyConversionCount)
{
    MODIFY_REG(ADC0x->DCONV, ADC0_DCONV_DCONV, DummyConversionCount);
}

/**
 * @brief  Get the dummy conversion count
 * @rmtoll DCONV         DCONV            LL_ADC0_GetDummyConversionCount
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_DUMMY_CONV_0
 *         @arg @ref LL_ADC0_DUMMY_CONV_1
 *         @arg @ref LL_ADC0_DUMMY_CONV_2
 *         @arg @ref LL_ADC0_DUMMY_CONV_3
 */
__STATIC_INLINE uint32_t LL_ADC0_GetDummyConversionCount(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->DCONV, ADC0_DCONV_DCONV));
}

/**
 * @}
 */ /* end of group ADC0_LL_EF_Configuration */

/** @defgroup ADC0_LL_EF_Oversampling Oversampling functions
 * @{
 */

/**
 * @brief  Enable oversampling
 * @note   If enabled oversampling DataAlignment will forced RIGHT
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion and  @ref LL_ADC0_SEQB_StopConversion called before.
 * @rmtoll OVSCFG         OVSEN            LL_ADC0_EnableOverSampling
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableOverSampling(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->OVSCFG, ADC0_OVSCFG_OVSEN);
}

/**
 * @brief  Disable oversampling
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion and  @ref LL_ADC0_SEQB_StopConversion called before.
 * @rmtoll OVSCFG         OVSEN            LL_ADC0_DisableOverSampling
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableOverSampling(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->OVSCFG, ADC0_OVSCFG_OVSEN);
}

/**
 * @brief  Indicates whether the oversampling is enabled
 * @rmtoll OVSCFG         OVSEN            LL_ADC0_IsEnabledOverSampling
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledOverSampling(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->OVSCFG, ADC0_OVSCFG_OVSEN) == (ADC0_OVSCFG_OVSEN)) ? 1UL : 0UL);
}

/**
 * @brief  Set the oversampling accumulation count
 * @rmtoll OVSCFG         OVSR            LL_ADC0_SetOverSamplingAccumulationCount
 * @param  ADC0x ADC0 Instance
 * @param  AccumulationCount This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_2
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_4
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_8
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_16
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_32
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_64
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_128
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_256
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetOverSamplingAccumulationCount(ADC0_TypeDef *ADC0x, uint32_t AccumulationCount)
{
    MODIFY_REG(ADC0x->OVSCFG, ADC0_OVSCFG_OVSR, AccumulationCount);
}

/**
 * @brief  Get the oversampling accumulation count
 * @rmtoll OVSCFG         OVSR            LL_ADC0_GetOverSamplingAccumulationCount
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_2
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_4
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_8
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_16
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_32
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_64
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_128
 *         @arg @ref LL_ADC0_OVERSAMPLING_ACCUMULATE_256
 */
__STATIC_INLINE uint32_t LL_ADC0_GetOverSamplingAccumulationCount(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->OVSCFG, ADC0_OVSCFG_OVSR));
}

/**
 * @brief  Set the oversampling right shift
 * @rmtoll OVSCFG         OVSS            LL_ADC0_SetOverSamplingRightShift
 * @param  ADC0x ADC0 Instance
 * @param  RightShift This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_NONE
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_1
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_2
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_3
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_4
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_5
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_6
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_7
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_8
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetOverSamplingRightShift(ADC0_TypeDef *ADC0x, uint32_t RightShift)
{
    MODIFY_REG(ADC0x->OVSCFG, ADC0_OVSCFG_OVSS, RightShift);
}

/**
 * @brief  Get the oversampling right shift
 * @rmtoll OVSCFG         OVSS            LL_ADC0_GetOverSamplingRightShift
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_NONE
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_1
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_2
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_3
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_4
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_5
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_6
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_7
 *         @arg @ref LL_ADC0_OVERSAMPLING_SHIFT_8
 */
__STATIC_INLINE uint32_t LL_ADC0_GetOverSamplingRightShift(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->OVSCFG, ADC0_OVSCFG_OVSS));
}

/**
 * @brief  Set the oversample channels
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion and  @ref LL_ADC0_SEQB_StopConversion called before.
 * @rmtoll OVSCHSEL         CH0            LL_ADC0_SetOverSamplingChannels
 * @param  ADC0x ADC0 Instance
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_ADC0_CHANNEL_ALL
 *         @arg @ref LL_ADC0_CHANNEL_NULL
 *         @arg @ref LL_ADC0_CHANNEL_0
 *         @arg @ref LL_ADC0_CHANNEL_1
 *         @arg @ref LL_ADC0_CHANNEL_2
 *         @arg @ref LL_ADC0_CHANNEL_3
 *         @arg @ref LL_ADC0_CHANNEL_4
 *         @arg @ref LL_ADC0_CHANNEL_5
 *         @arg @ref LL_ADC0_CHANNEL_6
 *         @arg @ref LL_ADC0_CHANNEL_7
 *         @arg @ref LL_ADC0_CHANNEL_8
 *         @arg @ref LL_ADC0_CHANNEL_9
 *         @arg @ref LL_ADC0_CHANNEL_10
 *         @arg @ref LL_ADC0_CHANNEL_11
 *         @arg @ref LL_ADC0_CHANNEL_12
 *         @arg @ref LL_ADC0_CHANNEL_13
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetOverSamplingChannels(ADC0_TypeDef *ADC0x, uint32_t Channels)
{
    WRITE_REG(ADC0x->OVSCHSEL, (Channels & 0x3FFF));
}

/**
 * @brief  Get the oversample channels
 * @rmtoll OVSCHSEL         CH0            LL_ADC0_GetOverSamplingChannels
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be a combination of the following values:
 *         @arg @ref LL_ADC0_CHANNEL_ALL
 *         @arg @ref LL_ADC0_CHANNEL_NULL
 *         @arg @ref LL_ADC0_CHANNEL_0
 *         @arg @ref LL_ADC0_CHANNEL_1
 *         @arg @ref LL_ADC0_CHANNEL_2
 *         @arg @ref LL_ADC0_CHANNEL_3
 *         @arg @ref LL_ADC0_CHANNEL_4
 *         @arg @ref LL_ADC0_CHANNEL_5
 *         @arg @ref LL_ADC0_CHANNEL_6
 *         @arg @ref LL_ADC0_CHANNEL_7
 *         @arg @ref LL_ADC0_CHANNEL_8
 *         @arg @ref LL_ADC0_CHANNEL_9
 *         @arg @ref LL_ADC0_CHANNEL_10
 *         @arg @ref LL_ADC0_CHANNEL_11
 *         @arg @ref LL_ADC0_CHANNEL_12
 *         @arg @ref LL_ADC0_CHANNEL_13
 */
__STATIC_INLINE uint32_t LL_ADC0_GetOverSamplingChannels(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_REG(ADC0x->OVSCHSEL) & 0x3FFF);
}


/**
 * @}
 */ /* end of group ADC0_LL_EF_Oversampling */

/** @defgroup ADC0_LL_EF_CAL_Configuration Calibration Configuration functions
 * @{
 */

/**
 * @brief  Set the normal ZEROT calibration value
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion and  @ref LL_ADC0_SEQB_StopConversion called before.
 * @note   ZEROT calibration value can be calculated by using helper macro @ref __LL_ADC0_CALC_NORMAL_ZEROT()
 * @rmtoll CAL         ZTG            LL_ADC0_SetNormalZEROT
 * @param  ADC0x ADC0 Instance
 * @param  NormalZEROT Between 0 to 0xFFF
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetNormalZEROT(ADC0_TypeDef *ADC0x, uint32_t NormalZEROT)
{
    MODIFY_REG(ADC0x->CAL, ADC0_CAL_ZTG, (NormalZEROT << ADC0_CAL_ZTG_POS));
}

/**
 * @brief  Get the normal ZEROT calibration value
 * @rmtoll CAL         ZTG            LL_ADC0_GetNormalZEROT
 * @param  ADC0x ADC0 Instance
 * @retval ZEROT calibration value for buffer disabled condition
 */
__STATIC_INLINE uint32_t LL_ADC0_GetNormalZEROT(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CAL, ADC0_CAL_ZTG) >> ADC0_CAL_ZTG_POS);
}

/**
 * @brief  Set the buffer ZEROT calibration value
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion and  @ref LL_ADC0_SEQB_StopConversion called before.
 * @note   ZEROT calibration value can be calculated by using helper macro @ref __LL_ADC0_CALC_BUFFER_ZEROT()
 * @rmtoll CAL         ZTBG            LL_ADC0_SetBufferZEROT
 * @param  ADC0x ADC0 Instance
 * @param  BufferZEROT Between 0 to 0xFFF
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetBufferZEROT(ADC0_TypeDef *ADC0x, uint32_t BufferZEROT)
{
    MODIFY_REG(ADC0x->CAL, ADC0_CAL_ZTBG, (BufferZEROT << ADC0_CAL_ZTBG_POS));
}

/**
 * @brief  Get the buffer ZEROT calibration value
 * @rmtoll CAL         ZTBG            LL_ADC0_GetBufferZEROT
 * @param  ADC0x ADC0 Instance
 * @retval ZEROT calibration value for buffer enabled condition
 */
__STATIC_INLINE uint32_t LL_ADC0_GetBufferZEROT(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CAL, ADC0_CAL_ZTBG) >> ADC0_CAL_ZTBG_POS);
}

/**
 * @brief  Enable hardware calibration
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion and  @ref LL_ADC0_SEQB_StopConversion called before.
 * @rmtoll CAL         CAL_EN            LL_ADC0_EnableCalibration
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableCalibration(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->CAL, ADC0_CAL_CAL_EN);
}

/**
 * @brief  Disable hardware calibration
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion and  @ref LL_ADC0_SEQB_StopConversion called before.
 * @rmtoll CAL         CAL_EN            LL_ADC0_DisableCalibration
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableCalibration(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->CAL, ADC0_CAL_CAL_EN);
}

/**
 * @brief  Indicates whether the hardware calibration is enabled
 * @rmtoll CAL         CAL_EN            LL_ADC0_IsEnabledCalibration
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledCalibration(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->CAL, ADC0_CAL_CAL_EN) == (ADC0_CAL_CAL_EN)) ? 1UL : 0UL);
}

/**
 * @brief  Set the digital offset
 * @rmtoll DOS         DOS            LL_ADC0_SetDigitalOffset
 * @param  ADC0x ADC0 Instance
 * @param  DigitalOffset  Between 0 to 0xFFF. (Two's complement signed value between -2048 to 2047)
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetDigitalOffset(ADC0_TypeDef *ADC0x, uint32_t DigitalOffset)
{
    MODIFY_REG(ADC0x->DOS, ADC0_DOS_DOS, (DigitalOffset << ADC0_DOS_DOS_POS));
}

/**
 * @brief  Get the digital offset
 * @rmtoll DOS         DOS            LL_ADC0_GetDigitalOffset
 * @param  ADC0x ADC0 Instance
 * @retval Digital offset code value (Between 0 to 0xFFF,two's complement signed value)
 */
__STATIC_INLINE uint32_t LL_ADC0_GetDigitalOffset(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->DOS, ADC0_DOS_DOS) >> ADC0_DOS_DOS_POS);
}

/**
 * @brief  Get the normal BGRLP code
 * @rmtoll BGRLP         CORE            LL_ADC0_GetNormalBGRLPCode
 * @param  ADC0x ADC0 Instance
 * @retval Accumulated code theory value when buffer disabled
 */
__STATIC_INLINE uint32_t LL_ADC0_GetNormalBGRLPCode(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->BGRLP, ADC0_BGRLP_CORE) >> ADC0_BGRLP_CORE_POS);
}

/**
 * @brief  Get the buffer BGRLP code
 * @rmtoll BGRLP         COREB            LL_ADC0_GetBufferBGRLPCode
 * @param  ADC0x ADC0 Instance
 * @retval Accumulated code theory value when buffer enabled
 */
__STATIC_INLINE uint32_t LL_ADC0_GetBufferBGRLPCode(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->BGRLP, ADC0_BGRLP_COREB) >> ADC0_BGRLP_COREB_POS);
}

/**
 * @}
 */ /* end of group ADC0_LL_EF_CAL_Configuration */


/** @defgroup ADC0_LL_EF_PowerManagement Power Management functions
 * @{
 */

/**
 * @brief  Set the ADC power mode
 * @rmtoll PWDCTRL         ADC0_AON            LL_ADC0_SetADCPowerMode
 * @param  ADC0x ADC0 Instance
 * @param  PowerMode This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_POWER_MODE_ALWAYS_ON
 *         @arg @ref LL_ADC0_POWER_MODE_DEMAND_ON
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetADCPowerMode(ADC0_TypeDef *ADC0x, uint32_t PowerMode)
{
    MODIFY_REG(ADC0x->PWDCTRL, ADC0_PWDCTRL_ADC0_AON, PowerMode);
}

/**
 * @brief  Get the ADC power mode
 * @rmtoll PWDCTRL         ADC0_AON            LL_ADC0_GetADCPowerMode
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_POWER_MODE_ALWAYS_ON
 *         @arg @ref LL_ADC0_POWER_MODE_DEMAND_ON
 */
__STATIC_INLINE uint32_t LL_ADC0_GetADCPowerMode(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->PWDCTRL, ADC0_PWDCTRL_ADC0_AON));
}

/**
 * @brief  Set the buffer power mode
 * @rmtoll PWDCTRL         BUFF_AON            LL_ADC0_SetBufferPowerMode
 * @param  ADC0x ADC0 Instance
 * @param  PowerMode This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_BUFFER_POWER_MODE_ALWAYS_ON
 *         @arg @ref LL_ADC0_BUFFER_POWER_MODE_DEMAND_ON
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SetBufferPowerMode(ADC0_TypeDef *ADC0x, uint32_t PowerMode)
{
    MODIFY_REG(ADC0x->PWDCTRL, ADC0_PWDCTRL_BUFF_AON, PowerMode);
}

/**
 * @brief  Get the buffer power mode
 * @rmtoll PWDCTRL         BUFF_AON            LL_ADC0_GetBufferPowerMode
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_BUFFER_POWER_MODE_ALWAYS_ON
 *         @arg @ref LL_ADC0_BUFFER_POWER_MODE_DEMAND_ON
 */
__STATIC_INLINE uint32_t LL_ADC0_GetBufferPowerMode(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->PWDCTRL, ADC0_PWDCTRL_BUFF_AON));
}

/**
 * @}
 */ /* end of group ADC0_LL_EF_PowerManagement */

/** @defgroup ADC0_LL_EF_SEQA_Configuration SEQA Configuration functions
 * @{
 */

/**
 * @brief  Enable multi-channels mode
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion called before.
 * @rmtoll CTRLA         CHMOD            LL_ADC0_SEQA_EnableMultiChannelsMode
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_EnableMultiChannelsMode(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->CTRLA, ADC0_CTRLA_CHMOD);
}

/**
 * @brief  Disable multi-channels mode
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion called before.
 * @rmtoll CTRLA         CHMOD            LL_ADC0_SEQA_DisableMultiChannelsMode
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_DisableMultiChannelsMode(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->CTRLA, ADC0_CTRLA_CHMOD);
}

/**
 * @brief  Indicates whether the multi-channels mode is enabled
 * @rmtoll CTRLA         CHMOD            LL_ADC0_SEQA_IsEnabledMultiChannelsMode
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQA_IsEnabledMultiChannelsMode(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->CTRLA, ADC0_CTRLA_CHMOD) == (ADC0_CTRLA_CHMOD)) ? 1UL : 0UL);
}

/**
 * @brief  Enable continuous conversion
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion called before.
 * @rmtoll CTRLA         CONT            LL_ADC0_SEQA_EnableContinuousConversion
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_EnableContinuousConversion(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->CTRLA, ADC0_CTRLA_CONT);
}

/**
 * @brief  Disable continuous conversion
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion called before.
 * @rmtoll CTRLA         CONT            LL_ADC0_SEQA_DisableContinuousConversion
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_DisableContinuousConversion(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->CTRLA, ADC0_CTRLA_CONT);
}

/**
 * @brief  Indicates whether the continuous conversion is enabled
 * @rmtoll CTRLA         CONT            LL_ADC0_SEQA_IsEnabledContinuousConversion
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQA_IsEnabledContinuousConversion(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->CTRLA, ADC0_CTRLA_CONT) == (ADC0_CTRLA_CONT)) ? 1UL : 0UL);
}

/**
 * @brief  Set the sequence discontinuous configuration
 * @note   Only can be configured when @ref LL_ADC0_SEQA_StopConversion called before.
 * @rmtoll CTRLA         DSICEN,DSICNUM            LL_ADC0_SEQA_SetSequenceDiscont
 * @param  ADC0x ADC0 Instance
 * @param  SequenceDiscont This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_DISABLE
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_1CH
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_2CH
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_3CH
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_4CH
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_SetSequenceDiscont(ADC0_TypeDef *ADC0x, uint32_t SequenceDiscont)
{
    MODIFY_REG(ADC0x->CTRLA, (ADC0_CTRLA_DSICEN | ADC0_CTRLA_DSICNUM), SequenceDiscont);
}

/**
 * @brief  Get the sequence discontinuous configuration
 * @rmtoll CTRLA         DSICEN,DSICNUM            LL_ADC0_SEQA_GetSequenceDiscont
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_DISABLE
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_1CH
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_2CH
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_3CH
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_4CH
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQA_GetSequenceDiscont(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CTRLA, (ADC0_CTRLA_DSICEN | ADC0_CTRLA_DSICNUM)));
}

/**
 * @brief  Set the overflow mode
 * @rmtoll CTRLA         OVRMOD            LL_ADC0_SEQA_SetOverflowMode
 * @param  ADC0x ADC0 Instance
 * @param  OverflowMode This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_OVERFLOW_MODE_NO_RECOVER
 *         @arg @ref LL_ADC0_OVERFLOW_MODE_RECOVER
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_SetOverflowMode(ADC0_TypeDef *ADC0x, uint32_t OverflowMode)
{
    MODIFY_REG(ADC0x->CTRLA, ADC0_CTRLA_OVRMOD, OverflowMode);
}

/**
 * @brief  Get the overflow mode
 * @rmtoll CTRLA         OVRMOD            LL_ADC0_SEQA_GetOverflowMode
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_OVERFLOW_MODE_NO_RECOVER
 *         @arg @ref LL_ADC0_OVERFLOW_MODE_RECOVER
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQA_GetOverflowMode(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CTRLA, ADC0_CTRLA_OVRMOD));
}

/**
 * @brief  Set the analog watchdog mode
 * @rmtoll CTRLA         AWDMOD            LL_ADC0_SEQA_SetAnalogWDMode
 *         CTRLA         AWDEN             LL_ADC0_SEQA_SetAnalogWDMode
 * @param  ADC0x ADC0 Instance
 * @param  AWDMode This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_AWD_MODE_DISABLE_AWD
 *         @arg @ref LL_ADC0_AWD_MODE_BETWEEN_HIGH_LOW
 *         @arg @ref LL_ADC0_AWD_MODE_ABOVE_LOW
 *         @arg @ref LL_ADC0_AWD_MODE_BELOW_HIGH
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_SetAnalogWDMode(ADC0_TypeDef *ADC0x, uint32_t AWDMode)
{
    MODIFY_REG(ADC0x->CTRLA, (ADC0_CTRLA_AWDMOD|ADC0_CTRLA_AWDEN), AWDMode);
}

/**
 * @brief  Get the analog watchdog mode
 * @rmtoll CTRLA         AWDMOD            LL_ADC0_SEQA_GetAnalogWDMode
 *         CTRLA         AWDEN             LL_ADC0_SEQA_GetAnalogWDMode
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_AWD_MODE_DISABLE_AWD
 *         @arg @ref LL_ADC0_AWD_MODE_BETWEEN_HIGH_LOW
 *         @arg @ref LL_ADC0_AWD_MODE_ABOVE_LOW
 *         @arg @ref LL_ADC0_AWD_MODE_BELOW_HIGH
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQA_GetAnalogWDMode(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CTRLA, (ADC0_CTRLA_AWDMOD|ADC0_CTRLA_AWDEN)));
}

/**
 * @brief  Set the sequence channels
 * @note   Only valid when ADC0_CTRLA_CHMOD=1 (multi-channel mode)
 * @rmtoll CHSELA         CH0            LL_ADC0_SEQA_SetSequenceChannels
 * @param  ADC0x ADC0 Instance
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_ADC0_CHANNEL_ALL
 *         @arg @ref LL_ADC0_CHANNEL_NULL
 *         @arg @ref LL_ADC0_CHANNEL_0
 *         @arg @ref LL_ADC0_CHANNEL_1
 *         @arg @ref LL_ADC0_CHANNEL_2
 *         @arg @ref LL_ADC0_CHANNEL_3
 *         @arg @ref LL_ADC0_CHANNEL_4
 *         @arg @ref LL_ADC0_CHANNEL_5
 *         @arg @ref LL_ADC0_CHANNEL_6
 *         @arg @ref LL_ADC0_CHANNEL_7
 *         @arg @ref LL_ADC0_CHANNEL_8
 *         @arg @ref LL_ADC0_CHANNEL_9
 *         @arg @ref LL_ADC0_CHANNEL_10
 *         @arg @ref LL_ADC0_CHANNEL_11
 *         @arg @ref LL_ADC0_CHANNEL_12
 *         @arg @ref LL_ADC0_CHANNEL_13
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_SetSequenceChannels(ADC0_TypeDef *ADC0x, uint32_t Channels)
{
    WRITE_REG(ADC0x->CHSELA, (Channels & 0x3FFF));
}

/**
 * @brief  Get the sequence channels
 * @rmtoll CHSELA         CH0            LL_ADC0_SEQA_GetSequenceChannels
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be a combination of the following values:
 *         @arg @ref LL_ADC0_CHANNEL_ALL
 *         @arg @ref LL_ADC0_CHANNEL_NULL
 *         @arg @ref LL_ADC0_CHANNEL_0
 *         @arg @ref LL_ADC0_CHANNEL_1
 *         @arg @ref LL_ADC0_CHANNEL_2
 *         @arg @ref LL_ADC0_CHANNEL_3
 *         @arg @ref LL_ADC0_CHANNEL_4
 *         @arg @ref LL_ADC0_CHANNEL_5
 *         @arg @ref LL_ADC0_CHANNEL_6
 *         @arg @ref LL_ADC0_CHANNEL_7
 *         @arg @ref LL_ADC0_CHANNEL_8
 *         @arg @ref LL_ADC0_CHANNEL_9
 *         @arg @ref LL_ADC0_CHANNEL_10
 *         @arg @ref LL_ADC0_CHANNEL_11
 *         @arg @ref LL_ADC0_CHANNEL_12
 *         @arg @ref LL_ADC0_CHANNEL_13
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQA_GetSequenceChannels(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_REG(ADC0x->CHSELA) & 0x3FFF);
}

/**
 * @brief  Start conversion with software start
 * @rmtoll STARTA         STARTA            LL_ADC0_SEQA_StartConversionSWStart
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_StartConversionSWStart(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->STARTA, ADC0_STARTA_STARTA);
}

/**
 * @brief  Stop conversion
 * @rmtoll STARTA         STARTA            LL_ADC0_SEQA_StopConversion
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_StopConversion(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->STARTA, ADC0_STARTA_STARTA);
}

/**
 * @brief  Read conversion data
 * @note   Format and available bit range is different,
 *       depend on @ref LL_ADC0_EnableOverSampling, @ref LL_ADC0_SetResolution and @ref LL_ADC0_SetDataAlignment.
 *       Refer to reference manual for detail.
 * @rmtoll DATA         DAT            LL_ADC0_ReadConversionData
 * @param  ADC0x ADC0 Instance
 * @retval Conversion data (16-bit)
 */
__STATIC_INLINE uint32_t LL_ADC0_ReadConversionData(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->DATA, ADC0_DATA_DAT) >> ADC0_DATA_DAT_POS);
}

/**
 * @brief  Set channel
 * @note   Only valid when ADC0_CTRLA_CHMOD=0 (single channel mode)
 * @rmtoll DATA         CHN            LL_ADC0_SEQA_SetChannel
 * @param  ADC0x ADC0 Instance
 * @param  Channel Channel number (0-13)
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_SetChannel(ADC0_TypeDef *ADC0x, uint32_t Channel)
{
    MODIFY_REG(ADC0x->DATA, ADC0_DATA_CHN, (Channel << ADC0_DATA_CHN_POS));
}

/**
 * @brief  Get current channel
 * @rmtoll DATA         CHN            LL_ADC0_SEQA_GetCurrentChannel
 * @param  ADC0x ADC0 Instance
 * @retval Current channel number
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQA_GetCurrentChannel(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->DATA, ADC0_DATA_CHN) >> ADC0_DATA_CHN_POS);
}

/**
 * @brief  Set analog watchdog thresholds
 *         Data will directly compare with  ADC0_DATA
 * @rmtoll AWDHLA         HT,LT            LL_ADC0_SEQA_SetAnalogWDThresholds
 * @param  ADC0x ADC0 Instance
 * @param  AWDThresholdsHighLow This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_AWD_THRESHOLD_HIGH
 *         @arg @ref LL_ADC0_AWD_THRESHOLD_LOW
 * @param  AWDThresholdValue Value between Min_Data=0x000 and Max_Data=0xFFFF (range depend on data format)
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_SetAnalogWDThresholds(ADC0_TypeDef *ADC0x, uint32_t AWDThresholdsHighLow, uint32_t AWDThresholdValue)
{
    if (AWDThresholdsHighLow == LL_ADC0_AWD_THRESHOLD_HIGH) {
        /* Set high threshold */
        MODIFY_REG(ADC0x->AWDHLA, ADC0_AWDHLA_HT, (AWDThresholdValue << ADC0_AWDHLA_HT_POS));
    }
    if (AWDThresholdsHighLow == LL_ADC0_AWD_THRESHOLD_LOW) {
        /* Set low threshold */
        MODIFY_REG(ADC0x->AWDHLA, ADC0_AWDHLA_LT, (AWDThresholdValue << ADC0_AWDHLA_LT_POS));
    }
}

/**
 * @brief  Get analog watchdog thresholds
 * @rmtoll AWDHLA         HT,LT            LL_ADC0_SEQA_GetAnalogWDThresholds
 * @param  ADC0x ADC0 Instance
 * @param  AWDThresholdsHighLow This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_AWD_THRESHOLD_HIGH
 *         @arg @ref LL_ADC0_AWD_THRESHOLD_LOW
 * @retval Threshold value
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQA_GetAnalogWDThresholds(ADC0_TypeDef *ADC0x, uint32_t AWDThresholdsHighLow)
{
    if (AWDThresholdsHighLow == LL_ADC0_AWD_THRESHOLD_HIGH) {
        /* Get high threshold */
        return (uint32_t)(READ_BIT(ADC0x->AWDHLA, ADC0_AWDHLA_HT) >> ADC0_AWDHLA_HT_POS);
    }
    if (AWDThresholdsHighLow == LL_ADC0_AWD_THRESHOLD_LOW) {
        /* Get low threshold */
        return (uint32_t)(READ_BIT(ADC0x->AWDHLA, ADC0_AWDHLA_LT) >> ADC0_AWDHLA_LT_POS);
    }
}

/**
 * @brief  Set analog watchdog monitored channels
 * @rmtoll WCHSELA         CH0            LL_ADC0_SEQA_SetAnalogWDMonitChannels
 * @param  ADC0x ADC0 Instance
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_ADC0_CHANNEL_ALL
 *         @arg @ref LL_ADC0_CHANNEL_NULL
 *         @arg @ref LL_ADC0_CHANNEL_0
 *         @arg @ref LL_ADC0_CHANNEL_1
 *         @arg @ref LL_ADC0_CHANNEL_2
 *         @arg @ref LL_ADC0_CHANNEL_3
 *         @arg @ref LL_ADC0_CHANNEL_4
 *         @arg @ref LL_ADC0_CHANNEL_5
 *         @arg @ref LL_ADC0_CHANNEL_6
 *         @arg @ref LL_ADC0_CHANNEL_7
 *         @arg @ref LL_ADC0_CHANNEL_8
 *         @arg @ref LL_ADC0_CHANNEL_9
 *         @arg @ref LL_ADC0_CHANNEL_10
 *         @arg @ref LL_ADC0_CHANNEL_11
 *         @arg @ref LL_ADC0_CHANNEL_12
 *         @arg @ref LL_ADC0_CHANNEL_13
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_SetAnalogWDMonitChannels(ADC0_TypeDef *ADC0x, uint32_t Channels)
{
    WRITE_REG(ADC0x->WCHSELA, (Channels & 0x3FFF));
}

/**
 * @brief  Get analog watchdog monitored channels
 * @rmtoll WCHSELA         CH0            LL_ADC0_SEQA_GetAnalogWDMonitChannels
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be a combination of the following values:
 *         @arg @ref LL_ADC0_CHANNEL_ALL
 *         @arg @ref LL_ADC0_CHANNEL_NULL
 *         @arg @ref LL_ADC0_CHANNEL_0
 *         @arg @ref LL_ADC0_CHANNEL_1
 *         @arg @ref LL_ADC0_CHANNEL_2
 *         @arg @ref LL_ADC0_CHANNEL_3
 *         @arg @ref LL_ADC0_CHANNEL_4
 *         @arg @ref LL_ADC0_CHANNEL_5
 *         @arg @ref LL_ADC0_CHANNEL_6
 *         @arg @ref LL_ADC0_CHANNEL_7
 *         @arg @ref LL_ADC0_CHANNEL_8
 *         @arg @ref LL_ADC0_CHANNEL_9
 *         @arg @ref LL_ADC0_CHANNEL_10
 *         @arg @ref LL_ADC0_CHANNEL_11
 *         @arg @ref LL_ADC0_CHANNEL_12
 *         @arg @ref LL_ADC0_CHANNEL_13
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQA_GetAnalogWDMonitChannels(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_REG(ADC0x->WCHSELA) & 0x3FFF);
}

/**
 * @}
 */ /* end of group ADC0_LL_EF_SEQA_Configuration */
/** @defgroup ADC0_LL_EF_SEQB_Configuration SEQB Configuration functions
 * @{
 */

/**
 * @brief  Enable multi-channels mode
 * @note   Only can be configured when @ref LL_ADC0_SEQB_StopConversion called before.
 * @rmtoll CTRLB         CHMOD            LL_ADC0_SEQB_EnableMultiChannelsMode
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_EnableMultiChannelsMode(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->CTRLB, ADC0_CTRLB_CHMOD);
}

/**
 * @brief  Disable multi-channels mode
 * @note   Only can be configured when @ref LL_ADC0_SEQB_StopConversion called before.
 * @rmtoll CTRLB         CHMOD            LL_ADC0_SEQB_DisableMultiChannelsMode
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_DisableMultiChannelsMode(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->CTRLB, ADC0_CTRLB_CHMOD);
}

/**
 * @brief  Indicates whether the multi-channels mode is enabled
 * @rmtoll CTRLB         CHMOD            LL_ADC0_SEQB_IsEnabledMultiChannelsMode
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQB_IsEnabledMultiChannelsMode(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->CTRLB, ADC0_CTRLB_CHMOD) == (ADC0_CTRLB_CHMOD)) ? 1UL : 0UL);
}

/**
 * @brief  Enable continuous conversion
 * @rmtoll CTRLB         CONT            LL_ADC0_SEQB_EnableContinuousConversion
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_EnableContinuousConversion(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->CTRLB, ADC0_CTRLB_CONT);
}

/**
 * @brief  Disable continuous conversion
 * @note   Only can be configured when @ref LL_ADC0_SEQB_StopConversion called before.
 * @rmtoll CTRLB         CONT            LL_ADC0_SEQB_DisableContinuousConversion
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_DisableContinuousConversion(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->CTRLB, ADC0_CTRLB_CONT);
}

/**
 * @brief  Indicates whether the continuous conversion is enabled
 * @note   Only can be configured when @ref LL_ADC0_SEQB_StopConversion called before.
 * @rmtoll CTRLB         CONT            LL_ADC0_SEQB_IsEnabledContinuousConversion
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQB_IsEnabledContinuousConversion(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->CTRLB, ADC0_CTRLB_CONT) == (ADC0_CTRLB_CONT)) ? 1UL : 0UL);
}

/**
 * @brief  Set the sequence discontinuous configuration
 * @note   Only can be configured when @ref LL_ADC0_SEQB_StopConversion called before.
 * @rmtoll CTRLB         DSICEN,DSICNUM            LL_ADC0_SEQB_SetSequenceDiscont
 * @param  ADC0x ADC0 Instance
 * @param  SequenceDiscont This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_DISABLE
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_1CH
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_2CH
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_3CH
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_4CH
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_SetSequenceDiscont(ADC0_TypeDef *ADC0x, uint32_t SequenceDiscont)
{
    MODIFY_REG(ADC0x->CTRLB, (ADC0_CTRLB_DSICEN | ADC0_CTRLB_DSICNUM), SequenceDiscont);
}

/**
 * @brief  Get the sequence discontinuous configuration
 * @rmtoll CTRLB         DSICEN,DSICNUM            LL_ADC0_SEQB_GetSequenceDiscont
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_DISABLE
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_1CH
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_2CH
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_3CH
 *         @arg @ref LL_ADC0_SEQUENCE_DISCONT_4CH
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQB_GetSequenceDiscont(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CTRLB, (ADC0_CTRLB_DSICEN | ADC0_CTRLB_DSICNUM)));
}

/**
 * @brief  Set the overflow mode
 * @rmtoll CTRLB         OVRMOD            LL_ADC0_SEQB_SetOverflowMode
 * @param  ADC0x ADC0 Instance
 * @param  OverflowMode This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_OVERFLOW_MODE_NO_RECOVER
 *         @arg @ref LL_ADC0_OVERFLOW_MODE_RECOVER
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_SetOverflowMode(ADC0_TypeDef *ADC0x, uint32_t OverflowMode)
{
    MODIFY_REG(ADC0x->CTRLB, ADC0_CTRLB_OVRMOD, OverflowMode);
}

/**
 * @brief  Get the overflow mode
 * @rmtoll CTRLB         OVRMOD            LL_ADC0_SEQB_GetOverflowMode
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_OVERFLOW_MODE_NO_RECOVER
 *         @arg @ref LL_ADC0_OVERFLOW_MODE_RECOVER
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQB_GetOverflowMode(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CTRLB, ADC0_CTRLB_OVRMOD));
}

/**
 * @brief  Set the analog watchdog mode
 * @rmtoll CTRLB         AWDMOD            LL_ADC0_SEQB_SetAnalogWDMode
 *         CTRLB         AWDEN             LL_ADC0_SEQB_SetAnalogWDMode
 * @param  ADC0x ADC0 Instance
 * @param  AWDMode This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_AWD_MODE_DISABLE_AWD
 *         @arg @ref LL_ADC0_AWD_MODE_BETWEEN_HIGH_LOW
 *         @arg @ref LL_ADC0_AWD_MODE_ABOVE_LOW
 *         @arg @ref LL_ADC0_AWD_MODE_BELOW_HIGH
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_SetAnalogWDMode(ADC0_TypeDef *ADC0x, uint32_t AWDMode)
{
    MODIFY_REG(ADC0x->CTRLB, (ADC0_CTRLB_AWDMOD|ADC0_CTRLB_AWDEN), AWDMode);
}

/**
 * @brief  Get the analog watchdog mode
 * @rmtoll CTRLB         AWDMOD            LL_ADC0_SEQB_GetAnalogWDMode
 *         CTRLB         AWDEN             LL_ADC0_SEQB_GetAnalogWDMode
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_AWD_MODE_DISABLE_AWD
 *         @arg @ref LL_ADC0_AWD_MODE_BETWEEN_HIGH_LOW
 *         @arg @ref LL_ADC0_AWD_MODE_ABOVE_LOW
 *         @arg @ref LL_ADC0_AWD_MODE_BELOW_HIGH
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQB_GetAnalogWDMode(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CTRLB, (ADC0_CTRLB_AWDMOD|ADC0_CTRLB_AWDEN)));
}

/**
 * @brief  Set the sequence channels
 * @note   Only valid when ADC0_CTRLB_CHMOD=1 (multi-channel mode)
 * @rmtoll CHSELB         CH0            LL_ADC0_SEQB_SetSequenceChannels
 * @param  ADC0x ADC0 Instance
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_ADC0_CHANNEL_ALL
 *         @arg @ref LL_ADC0_CHANNEL_NULL
 *         @arg @ref LL_ADC0_CHANNEL_0
 *         @arg @ref LL_ADC0_CHANNEL_1
 *         @arg @ref LL_ADC0_CHANNEL_2
 *         @arg @ref LL_ADC0_CHANNEL_3
 *         @arg @ref LL_ADC0_CHANNEL_4
 *         @arg @ref LL_ADC0_CHANNEL_5
 *         @arg @ref LL_ADC0_CHANNEL_6
 *         @arg @ref LL_ADC0_CHANNEL_7
 *         @arg @ref LL_ADC0_CHANNEL_8
 *         @arg @ref LL_ADC0_CHANNEL_9
 *         @arg @ref LL_ADC0_CHANNEL_10
 *         @arg @ref LL_ADC0_CHANNEL_11
 *         @arg @ref LL_ADC0_CHANNEL_12
 *         @arg @ref LL_ADC0_CHANNEL_13
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_SetSequenceChannels(ADC0_TypeDef *ADC0x, uint32_t Channels)
{
    WRITE_REG(ADC0x->CHSELB, (Channels & 0x3FFF));
}

/**
 * @brief  Get the sequence channels
 * @rmtoll CHSELB         CH0            LL_ADC0_SEQB_GetSequenceChannels
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be a combination of the following values:
 *         @arg @ref LL_ADC0_CHANNEL_ALL
 *         @arg @ref LL_ADC0_CHANNEL_NULL
 *         @arg @ref LL_ADC0_CHANNEL_0
 *         @arg @ref LL_ADC0_CHANNEL_1
 *         @arg @ref LL_ADC0_CHANNEL_2
 *         @arg @ref LL_ADC0_CHANNEL_3
 *         @arg @ref LL_ADC0_CHANNEL_4
 *         @arg @ref LL_ADC0_CHANNEL_5
 *         @arg @ref LL_ADC0_CHANNEL_6
 *         @arg @ref LL_ADC0_CHANNEL_7
 *         @arg @ref LL_ADC0_CHANNEL_8
 *         @arg @ref LL_ADC0_CHANNEL_9
 *         @arg @ref LL_ADC0_CHANNEL_10
 *         @arg @ref LL_ADC0_CHANNEL_11
 *         @arg @ref LL_ADC0_CHANNEL_12
 *         @arg @ref LL_ADC0_CHANNEL_13
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQB_GetSequenceChannels(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_REG(ADC0x->CHSELB) & 0x3FFF);
}

/**
 * @brief  Start conversion with software start
 * @rmtoll STARTB         STARTB            LL_ADC0_SEQB_StartConversionSWStart
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_StartConversionSWStart(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->STARTB, ADC0_STARTB_STARTB);
}

/**
 * @brief  Stop conversion
 * @rmtoll STARTB         STARTB            LL_ADC0_SEQB_StopConversion
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_StopConversion(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->STARTB, ADC0_STARTB_STARTB);
}

/**
 * @brief  Read conversion DATB
 * @note   Format and available bit range is different,
 *       depend on @ref LL_ADC0_EnableOverSampling, @ref LL_ADC0_SetResolution and @ref LL_ADC0_SetDATBAlignment.
 *       Refer to reference manual for detail.
 * @rmtoll DATB         DAT            LL_ADC0_ReadConversionDATB
 * @param  ADC0x ADC0 Instance
 * @retval Conversion DATB (16-bit)
 */
__STATIC_INLINE uint32_t LL_ADC0_ReadConversionDATB(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->DATB, ADC0_DATB_DAT) >> ADC0_DATB_DAT_POS);
}

/**
 * @brief  Set channel
 * @note   Only valid when ADC0_CTRLB_CHMOD=0 (single channel mode)
 * @rmtoll DATB         CHN            LL_ADC0_SEQB_SetChannel
 * @param  ADC0x ADC0 Instance
 * @param  Channel Channel number (0-13)
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_SetChannel(ADC0_TypeDef *ADC0x, uint32_t Channel)
{
    MODIFY_REG(ADC0x->DATB, ADC0_DATB_CHN, (Channel << ADC0_DATB_CHN_POS));
}

/**
 * @brief  Get current channel
 * @rmtoll DATB         CHN            LL_ADC0_SEQB_GetCurrentChannel
 * @param  ADC0x ADC0 Instance
 * @retval Current channel number
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQB_GetCurrentChannel(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->DATB, ADC0_DATB_CHN) >> ADC0_DATB_CHN_POS);
}

/**
 * @brief  Set analog watchdog thresholds
 *         DATB will directly compare with  ADC0_DATB
 * @rmtoll AWDHLB         HT,LT            LL_ADC0_SEQB_SetAnalogWDThresholds
 * @param  ADC0x ADC0 Instance
 * @param  AWDThresholdsHighLow This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_AWD_THRESHOLD_HIGH
 *         @arg @ref LL_ADC0_AWD_THRESHOLD_LOW
 * @param  AWDThresholdValue Value between Min_DATB=0x000 and Max_DATB=0xFFFF (range depend on DATB format)
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_SetAnalogWDThresholds(ADC0_TypeDef *ADC0x, uint32_t AWDThresholdsHighLow, uint32_t AWDThresholdValue)
{
    if (AWDThresholdsHighLow == LL_ADC0_AWD_THRESHOLD_HIGH) {
        /* Set high threshold */
        MODIFY_REG(ADC0x->AWDHLB, ADC0_AWDHLB_HT, (AWDThresholdValue << ADC0_AWDHLB_HT_POS));
    }
    if (AWDThresholdsHighLow == LL_ADC0_AWD_THRESHOLD_LOW) {
        /* Set low threshold */
        MODIFY_REG(ADC0x->AWDHLB, ADC0_AWDHLB_LT, (AWDThresholdValue << ADC0_AWDHLB_LT_POS));
    }
}

/**
 * @brief  Get analog watchdog thresholds
 * @rmtoll AWDHLB         HT,LT            LL_ADC0_SEQB_GetAnalogWDThresholds
 * @param  ADC0x ADC0 Instance
 * @param  AWDThresholdsHighLow This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_AWD_THRESHOLD_HIGH
 *         @arg @ref LL_ADC0_AWD_THRESHOLD_LOW
 * @retval Threshold value
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQB_GetAnalogWDThresholds(ADC0_TypeDef *ADC0x, uint32_t AWDThresholdsHighLow)
{
    if (AWDThresholdsHighLow == LL_ADC0_AWD_THRESHOLD_HIGH) {
        /* Get high threshold */
        return (uint32_t)(READ_BIT(ADC0x->AWDHLB, ADC0_AWDHLB_HT) >> ADC0_AWDHLB_HT_POS);
    }
    if (AWDThresholdsHighLow == LL_ADC0_AWD_THRESHOLD_LOW) {
        /* Get low threshold */
        return (uint32_t)(READ_BIT(ADC0x->AWDHLB, ADC0_AWDHLB_LT) >> ADC0_AWDHLB_LT_POS);
    }
}

/**
 * @brief  Set analog watchdog monitored channels
 * @rmtoll WCHSELB         CH0            LL_ADC0_SEQB_SetAnalogWDMonitChannels
 * @param  ADC0x ADC0 Instance
 * @param  Channels This parameter can be a combination of the following values:
 *         @arg @ref LL_ADC0_CHANNEL_ALL
 *         @arg @ref LL_ADC0_CHANNEL_NULL
 *         @arg @ref LL_ADC0_CHANNEL_0
 *         @arg @ref LL_ADC0_CHANNEL_1
 *         @arg @ref LL_ADC0_CHANNEL_2
 *         @arg @ref LL_ADC0_CHANNEL_3
 *         @arg @ref LL_ADC0_CHANNEL_4
 *         @arg @ref LL_ADC0_CHANNEL_5
 *         @arg @ref LL_ADC0_CHANNEL_6
 *         @arg @ref LL_ADC0_CHANNEL_7
 *         @arg @ref LL_ADC0_CHANNEL_8
 *         @arg @ref LL_ADC0_CHANNEL_9
 *         @arg @ref LL_ADC0_CHANNEL_10
 *         @arg @ref LL_ADC0_CHANNEL_11
 *         @arg @ref LL_ADC0_CHANNEL_12
 *         @arg @ref LL_ADC0_CHANNEL_13
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_SetAnalogWDMonitChannels(ADC0_TypeDef *ADC0x, uint32_t Channels)
{
    WRITE_REG(ADC0x->WCHSELB, (Channels & 0x3FFF));
}

/**
 * @brief  Get analog watchdog monitored channels
 * @rmtoll WCHSELB         CH0            LL_ADC0_SEQB_GetAnalogWDMonitChannels
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be a combination of the following values:
 *         @arg @ref LL_ADC0_CHANNEL_ALL
 *         @arg @ref LL_ADC0_CHANNEL_NULL
 *         @arg @ref LL_ADC0_CHANNEL_0
 *         @arg @ref LL_ADC0_CHANNEL_1
 *         @arg @ref LL_ADC0_CHANNEL_2
 *         @arg @ref LL_ADC0_CHANNEL_3
 *         @arg @ref LL_ADC0_CHANNEL_4
 *         @arg @ref LL_ADC0_CHANNEL_5
 *         @arg @ref LL_ADC0_CHANNEL_6
 *         @arg @ref LL_ADC0_CHANNEL_7
 *         @arg @ref LL_ADC0_CHANNEL_8
 *         @arg @ref LL_ADC0_CHANNEL_9
 *         @arg @ref LL_ADC0_CHANNEL_10
 *         @arg @ref LL_ADC0_CHANNEL_11
 *         @arg @ref LL_ADC0_CHANNEL_12
 *         @arg @ref LL_ADC0_CHANNEL_13
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQB_GetAnalogWDMonitChannels(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_REG(ADC0x->WCHSELB) & 0x3FFF);
}

/**
 * @brief  Set the preempt mode
 * @rmtoll CTRLB         SMODE            LL_ADC0_SEQA_SetPreemptMode
 * @param  ADC0x ADC0 Instance
 * @param  PreemptMode This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_PREEMPT_MODE_STOP
 *         @arg @ref LL_ADC0_PREEMPT_MODE_WAIT
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_SetPreemptMode(ADC0_TypeDef *ADC0x, uint32_t PreemptMode)
{
    MODIFY_REG(ADC0x->CTRLB, ADC0_CTRLB_SMODE, PreemptMode);
}

/**
 * @brief  Get the preempt mode
 * @rmtoll CTRLB         SMODE            LL_ADC0_SEQA_GetPreemptMode
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_PREEMPT_MODE_STOP
 *         @arg @ref LL_ADC0_PREEMPT_MODE_WAIT
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQB_GetPreemptMode(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CTRLB, ADC0_CTRLB_SMODE));
}
/**
 * @}
 */ /* end of group ADC0_LL_EF_SEQB_Configuration */

/** @defgroup ADC0_LL_EF_IT_Management Interrupt Management functions
 * @{
 */
/**
 * @brief  Enable SEQA EOC interrupt
 * @rmtoll IEA         EOCIE            LL_ADC0_EnableIT_AEOC
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableIT_AEOC(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IEA, ADC0_IEA_EOCIE);
}

/**
 * @brief  Disable SEQA EOC interrupt
 * @rmtoll IEA         EOCIE            LL_ADC0_DisableIT_AEOC
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableIT_AEOC(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->IEA, ADC0_IEA_EOCIE);
}

/**
 * @brief  Indicates whether the SEQA EOC interrupt is enabled
 * @rmtoll IEA         EOCIE            LL_ADC0_IsEnabledIT_AEOC
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledIT_AEOC(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IEA, ADC0_IEA_EOCIE) == (ADC0_IEA_EOCIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable SEQA EOCS interrupt
 * @rmtoll IEA         EOCSIE            LL_ADC0_EnableIT_AEOCS
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableIT_AEOCS(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IEA, ADC0_IEA_EOCSIE);
}

/**
 * @brief  Disable SEQA EOCS interrupt
 * @rmtoll IEA         EOCSIE            LL_ADC0_DisableIT_AEOCS
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableIT_AEOCS(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->IEA, ADC0_IEA_EOCSIE);
}

/**
 * @brief  Indicates whether the SEQA EOCS interrupt is enabled
 * @rmtoll IEA         EOCSIE            LL_ADC0_IsEnabledIT_AEOCS
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledIT_AEOCS(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IEA, ADC0_IEA_EOCSIE) == (ADC0_IEA_EOCSIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable SEQA AWD interrupt
 * @rmtoll IEA         AWDIE            LL_ADC0_EnableIT_AAWD
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableIT_AAWD(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IEA, ADC0_IEA_AWDIE);
}

/**
 * @brief  Disable SEQA AWD interrupt
 * @rmtoll IEA         AWDIE            LL_ADC0_DisableIT_AAWD
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableIT_AAWD(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->IEA, ADC0_IEA_AWDIE);
}

/**
 * @brief  Indicates whether the SEQA AWD interrupt is enabled
 * @rmtoll IEA         AWDIE            LL_ADC0_IsEnabledIT_AAWD
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledIT_AAWD(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IEA, ADC0_IEA_AWDIE) == (ADC0_IEA_AWDIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable SEQA DMAH interrupt
 * @rmtoll IEA         DMA_HIE            LL_ADC0_EnableIT_ADMAH
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableIT_ADMAH(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IEA, ADC0_IEA_DMA_HIE);
}

/**
 * @brief  Disable SEQA DMAH interrupt
 * @rmtoll IEA         DMA_HIE            LL_ADC0_DisableIT_ADMAH
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableIT_ADMAH(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->IEA, ADC0_IEA_DMA_HIE);
}

/**
 * @brief  Indicates whether the SEQA DMAH interrupt is enabled
 * @rmtoll IEA         DMA_HIE            LL_ADC0_IsEnabledIT_ADMAH
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledIT_ADMAH(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IEA, ADC0_IEA_DMA_HIE) == (ADC0_IEA_DMA_HIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable SEQA DMAF interrupt
 * @rmtoll IEA         DMA_FIE            LL_ADC0_EnableIT_ADMAF
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableIT_ADMAF(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IEA, ADC0_IEA_DMA_FIE);
}

/**
 * @brief  Disable SEQA DMAF interrupt
 * @rmtoll IEA         DMA_FIE            LL_ADC0_DisableIT_ADMAF
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableIT_ADMAF(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->IEA, ADC0_IEA_DMA_FIE);
}

/**
 * @brief  Indicates whether the SEQA DMAF interrupt is enabled
 * @rmtoll IEA         DMA_FIE            LL_ADC0_IsEnabledIT_ADMAF
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledIT_ADMAF(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IEA, ADC0_IEA_DMA_FIE) == (ADC0_IEA_DMA_FIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable SEQA DMAERR interrupt
 * @rmtoll IEA         DMA_ERRIE            LL_ADC0_EnableIT_ADMAERR
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableIT_ADMAERR(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IEA, ADC0_IEA_DMA_ERRIE);
}

/**
 * @brief  Disable SEQA DMAERR interrupt
 * @rmtoll IEA         DMA_ERRIE            LL_ADC0_DisableIT_ADMAERR
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableIT_ADMAERR(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->IEA, ADC0_IEA_DMA_ERRIE);
}

/**
 * @brief  Indicates whether the SEQA DMAERR interrupt is enabled
 * @rmtoll IEA         DMA_ERRIE            LL_ADC0_IsEnabledIT_ADMAERR
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledIT_ADMAERR(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IEA, ADC0_IEA_DMA_ERRIE) == (ADC0_IEA_DMA_ERRIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable SEQB EOC interrupt
 * @rmtoll IEB         EOCIE            LL_ADC0_EnableIT_BEOC
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableIT_BEOC(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IEB, ADC0_IEB_EOCIE);
}

/**
 * @brief  Disable SEQB EOC interrupt
 * @rmtoll IEB         EOCIE            LL_ADC0_DisableIT_BEOC
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableIT_BEOC(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->IEB, ADC0_IEB_EOCIE);
}

/**
 * @brief  Indicates whether the SEQB EOC interrupt is enabled
 * @rmtoll IEB         EOCIE            LL_ADC0_IsEnabledIT_BEOC
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledIT_BEOC(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IEB, ADC0_IEB_EOCIE) == (ADC0_IEB_EOCIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable SEQB EOCS interrupt
 * @rmtoll IEB         EOCSIE            LL_ADC0_EnableIT_BEOCS
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableIT_BEOCS(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IEB, ADC0_IEB_EOCSIE);
}

/**
 * @brief  Disable SEQB EOCS interrupt
 * @rmtoll IEB         EOCSIE            LL_ADC0_DisableIT_BEOCS
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableIT_BEOCS(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->IEB, ADC0_IEB_EOCSIE);
}

/**
 * @brief  Indicates whether the SEQB EOCS interrupt is enabled
 * @rmtoll IEB         EOCSIE            LL_ADC0_IsEnabledIT_BEOCS
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledIT_BEOCS(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IEB, ADC0_IEB_EOCSIE) == (ADC0_IEB_EOCSIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable SEQB AWD interrupt
 * @rmtoll IEB         AWDIE            LL_ADC0_EnableIT_BAWD
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableIT_BAWD(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IEB, ADC0_IEB_AWDIE);
}

/**
 * @brief  Disable SEQB AWD interrupt
 * @rmtoll IEB         AWDIE            LL_ADC0_DisableIT_BAWD
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableIT_BAWD(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->IEB, ADC0_IEB_AWDIE);
}

/**
 * @brief  Indicates whether the SEQB AWD interrupt is enabled
 * @rmtoll IEB         AWDIE            LL_ADC0_IsEnabledIT_BAWD
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledIT_BAWD(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IEB, ADC0_IEB_AWDIE) == (ADC0_IEB_AWDIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable SEQB DMAH interrupt
 * @rmtoll IEB         DMA_HIE            LL_ADC0_EnableIT_BDMAH
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableIT_BDMAH(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IEB, ADC0_IEB_DMA_HIE);
}

/**
 * @brief  Disable SEQB DMAH interrupt
 * @rmtoll IEB         DMA_HIE            LL_ADC0_DisableIT_BDMAH
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableIT_BDMAH(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->IEB, ADC0_IEB_DMA_HIE);
}

/**
 * @brief  Indicates whether the SEQB DMAH interrupt is enabled
 * @rmtoll IEB         DMA_HIE            LL_ADC0_IsEnabledIT_BDMAH
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledIT_BDMAH(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IEB, ADC0_IEB_DMA_HIE) == (ADC0_IEB_DMA_HIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable SEQB DMAF interrupt
 * @rmtoll IEB         DMA_FIE            LL_ADC0_EnableIT_BDMAF
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableIT_BDMAF(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IEB, ADC0_IEB_DMA_FIE);
}

/**
 * @brief  Disable SEQB DMAF interrupt
 * @rmtoll IEB         DMA_FIE            LL_ADC0_DisableIT_BDMAF
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableIT_BDMAF(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->IEB, ADC0_IEB_DMA_FIE);
}

/**
 * @brief  Indicates whether the SEQB DMAF interrupt is enabled
 * @rmtoll IEB         DMA_FIE            LL_ADC0_IsEnabledIT_BDMAF
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledIT_BDMAF(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IEB, ADC0_IEB_DMA_FIE) == (ADC0_IEB_DMA_FIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable SEQB DMAERR interrupt
 * @rmtoll IEB         DMA_ERRIE            LL_ADC0_EnableIT_BDMAERR
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_EnableIT_BDMAERR(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IEB, ADC0_IEB_DMA_ERRIE);
}

/**
 * @brief  Disable SEQB DMAERR interrupt
 * @rmtoll IEB         DMA_ERRIE            LL_ADC0_DisableIT_BDMAERR
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_DisableIT_BDMAERR(ADC0_TypeDef *ADC0x)
{
    CLEAR_BIT(ADC0x->IEB, ADC0_IEB_DMA_ERRIE);
}

/**
 * @brief  Indicates whether the SEQB DMAERR interrupt is enabled
 * @rmtoll IEB         DMA_ERRIE            LL_ADC0_IsEnabledIT_BDMAERR
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsEnabledIT_BDMAERR(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IEB, ADC0_IEB_DMA_ERRIE) == (ADC0_IEB_DMA_ERRIE)) ? 1UL : 0UL);
}

/**
 * @}
 */ /* end of group ADC0_LL_EF_IT_Management */

/** @defgroup ADC0_LL_EF_FLAG_Management Flag Management functions
 * @{
 */

/**
 * @brief  Indicate whether SEQA EOC flag is set
 * @rmtoll IFA         EOC            LL_ADC0_IsActiveFlag_AEOC
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsActiveFlag_AEOC(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IFA, ADC0_IFA_EOC) == (ADC0_IFA_EOC)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the SEQA  EOC flag
 * @rmtoll IFA         EOC            LL_ADC0_ClearFlag_AEOC
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_ClearFlag_AEOC(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IFA, ADC0_IFA_EOC);
}

/**
 * @brief  Indicate whether SEQA EOCS flag is set
 * @rmtoll IFA         EOCS            LL_ADC0_IsActiveFlag_AEOCS
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsActiveFlag_AEOCS(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IFA, ADC0_IFA_EOCS) == (ADC0_IFA_EOCS)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the SEQA  EOCS flag
 * @rmtoll IFA         EOCS            LL_ADC0_ClearFlag_AEOCS
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_ClearFlag_AEOCS(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IFA, ADC0_IFA_EOCS);
}

/**
 * @brief  Indicate whether SEQA AWD flag is set
 * @rmtoll IFA         AWD            LL_ADC0_IsActiveFlag_AAWD
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsActiveFlag_AAWD(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IFA, ADC0_IFA_AWD) == (ADC0_IFA_AWD)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the SEQA  AWD flag
 * @rmtoll IFA         AWD            LL_ADC0_ClearFlag_AAWD
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_ClearFlag_AAWD(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IFA, ADC0_IFA_AWD);
}

/**
 * @brief  Indicate whether SEQA DMAH flag is set
 * @rmtoll IFA         DMA_HDONE            LL_ADC0_IsActiveFlag_ADMAH
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsActiveFlag_ADMAH(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IFA, ADC0_IFA_DMA_HDONE) == (ADC0_IFA_DMA_HDONE)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the SEQA  DMAH flag
 * @rmtoll IFA         DMA_HDONE            LL_ADC0_ClearFlag_ADMAH
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_ClearFlag_ADMAH(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IFA, ADC0_IFA_DMA_HDONE);
}

/**
 * @brief  Indicate whether SEQA DMAF flag is set
 * @rmtoll IFA         DMA_FDONE            LL_ADC0_IsActiveFlag_ADMAF
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsActiveFlag_ADMAF(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IFA, ADC0_IFA_DMA_FDONE) == (ADC0_IFA_DMA_FDONE)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the SEQA  DMAF flag
 * @rmtoll IFA         DMA_FDONE            LL_ADC0_ClearFlag_ADMAF
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_ClearFlag_ADMAF(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IFA, ADC0_IFA_DMA_FDONE);
}

/**
 * @brief  Indicate whether SEQA DMAERR flag is set
 * @rmtoll IFA         DMA_ERR            LL_ADC0_IsActiveFlag_ADMAERR
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsActiveFlag_ADMAERR(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IFA, ADC0_IFA_DMA_ERR) == (ADC0_IFA_DMA_ERR)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the SEQA  DMAERR flag
 * @rmtoll IFA         DMA_ERR            LL_ADC0_ClearFlag_ADMAERR
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_ClearFlag_ADMAERR(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IFA, ADC0_IFA_DMA_ERR);
}

/**
 * @brief  Indicate whether SEQB EOC flag is set
 * @rmtoll IFB         EOC            LL_ADC0_IsActiveFlag_BEOC
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsActiveFlag_BEOC(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IFB, ADC0_IFB_EOC) == (ADC0_IFB_EOC)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the SEQB  EOC flag
 * @rmtoll IFB         EOC            LL_ADC0_ClearFlag_BEOC
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_ClearFlag_BEOC(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IFB, ADC0_IFB_EOC);
}

/**
 * @brief  Indicate whether SEQB EOCS flag is set
 * @rmtoll IFB         EOCS            LL_ADC0_IsActiveFlag_BEOCS
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsActiveFlag_BEOCS(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IFB, ADC0_IFB_EOCS) == (ADC0_IFB_EOCS)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the SEQB  EOCS flag
 * @rmtoll IFB         EOCS            LL_ADC0_ClearFlag_BEOCS
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_ClearFlag_BEOCS(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IFB, ADC0_IFB_EOCS);
}

/**
 * @brief  Indicate whether SEQB AWD flag is set
 * @rmtoll IFB         AWD            LL_ADC0_IsActiveFlag_BAWD
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsActiveFlag_BAWD(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IFB, ADC0_IFB_AWD) == (ADC0_IFB_AWD)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the SEQB  AWD flag
 * @rmtoll IFB         AWD            LL_ADC0_ClearFlag_BAWD
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_ClearFlag_BAWD(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IFB, ADC0_IFB_AWD);
}

/**
 * @brief  Indicate whether SEQB DMAH flag is set
 * @rmtoll IFB         DMA_HDONE            LL_ADC0_IsActiveFlag_BDMAH
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsActiveFlag_BDMAH(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IFB, ADC0_IFB_DMA_HDONE) == (ADC0_IFB_DMA_HDONE)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the SEQB  DMAH flag
 * @rmtoll IFB         DMA_HDONE            LL_ADC0_ClearFlag_BDMAH
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_ClearFlag_BDMAH(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IFB, ADC0_IFB_DMA_HDONE);
}

/**
 * @brief  Indicate whether SEQB DMAF flag is set
 * @rmtoll IFB         DMA_FDONE            LL_ADC0_IsActiveFlag_BDMAF
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsActiveFlag_BDMAF(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IFB, ADC0_IFB_DMA_FDONE) == (ADC0_IFB_DMA_FDONE)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the SEQB  DMAF flag
 * @rmtoll IFB         DMA_FDONE            LL_ADC0_ClearFlag_BDMAF
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_ClearFlag_BDMAF(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IFB, ADC0_IFB_DMA_FDONE);
}

/**
 * @brief  Indicate whether SEQB DMAERR flag is set
 * @rmtoll IFB         DMA_ERR            LL_ADC0_IsActiveFlag_BDMAERR
 * @param  ADC0x ADC0 Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_ADC0_IsActiveFlag_BDMAERR(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)((READ_BIT(ADC0x->IFB, ADC0_IFB_DMA_ERR) == (ADC0_IFB_DMA_ERR)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the SEQB  DMAERR flag
 * @rmtoll IFB         DMA_ERR            LL_ADC0_ClearFlag_BDMAERR
 * @param  ADC0x ADC0 Instance
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_ClearFlag_BDMAERR(ADC0_TypeDef *ADC0x)
{
    SET_BIT(ADC0x->IFB, ADC0_IFB_DMA_ERR);
}

/**
 * @}
 */ /* end of group ADC0_LL_EF_FLAG_Management */


/** @defgroup ADC0_LL_EF_DMA_Configuration DMA Configuration functions
 * @{
 */

/**
 * @brief  Set the SEQA DMA transfer configuration
 * @note   This function handles both DMAEN and DMA_MODE bits
 * @rmtoll CTRLA         DMAEN,DMA_MODE            LL_ADC0_SEQA_SetDMATransfer
 * @param  ADC0x ADC0 Instance
 * @param  DMATransfer This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_DMA_TRANSFER_DISABLE
 *         @arg @ref LL_ADC0_DMA_TRANSFER_ONCE
 *         @arg @ref LL_ADC0_DMA_TRANSFER_CONTINUOUS
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_SetDMATransfer(ADC0_TypeDef *ADC0x, uint32_t DMATransfer)
{
    MODIFY_REG(ADC0x->CTRLA, (ADC0_CTRLA_DMAEN | ADC0_CTRLA_DMA_MODE), DMATransfer);
}

/**
 * @brief  Get the SEQA DMA transfer configuration
 * @rmtoll CTRLA         DMAEN,DMA_MODE            LL_ADC0_SEQA_GetDMATransfer
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_DMA_TRANSFER_DISABLE
 *         @arg @ref LL_ADC0_DMA_TRANSFER_ONCE
 *         @arg @ref LL_ADC0_DMA_TRANSFER_CONTINUOUS
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQA_GetDMATransfer(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CTRLA, (ADC0_CTRLA_DMAEN | ADC0_CTRLA_DMA_MODE)));
}

/**
 * @brief  Set the SEQA DMA base address
 * @rmtoll DMA_RBADRA         DMA_RBADRA            LL_ADC0_SEQA_SetDMABaseAddress
 * @param  ADC0x ADC0 Instance
 * @param  BaseAddress Between 0 to 0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_SetDMABaseAddress(ADC0_TypeDef *ADC0x, uint32_t BaseAddress)
{
    MODIFY_REG(ADC0x->DMA_RBADRA, ADC0_DMA_RBADRA_DMA_RBADRA, (BaseAddress << ADC0_DMA_RBADRA_DMA_RBADRA_POS));
}

/**
 * @brief  Get the SEQA DMA base address
 * @rmtoll DMA_RBADRA         DMA_RBADRA            LL_ADC0_SEQA_GetDMABaseAddress
 * @param  ADC0x ADC0 Instance
 * @retval SEQA DMA base address
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQA_GetDMABaseAddress(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->DMA_RBADRA, ADC0_DMA_RBADRA_DMA_RBADRA) >> ADC0_DMA_RBADRA_DMA_RBADRA_POS);
}

/**
 * @brief  Set the SEQA DMA length
 * @rmtoll DMA_RLENA         DMA_RLENA            LL_ADC0_SEQA_SetDMALength
 * @param  ADC0x ADC0 Instance
 * @param  Length Between 0 to 0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQA_SetDMALength(ADC0_TypeDef *ADC0x, uint32_t Length)
{
    MODIFY_REG(ADC0x->DMA_RLENA, ADC0_DMA_RLENA_DMA_RLENA, (Length << ADC0_DMA_RLENA_DMA_RLENA_POS));
}

/**
 * @brief  Get the SEQA DMA length
 * @rmtoll DMA_RLENA         DMA_RLENA            LL_ADC0_SEQA_GetDMALength
 * @param  ADC0x ADC0 Instance
 * @retval SEQA DMA length
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQA_GetDMALength(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->DMA_RLENA, ADC0_DMA_RLENA_DMA_RLENA) >> ADC0_DMA_RLENA_DMA_RLENA_POS);
}

/**
 * @brief  Get the SEQA DMA current address
 * @rmtoll DMA_RADRA         DMA_RADRA            LL_ADC0_SEQA_GetDMACurrentAddress
 * @param  ADC0x ADC0 Instance
 * @retval SEQA DMA current address
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQA_GetDMACurrentAddress(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->DMA_RADRA, ADC0_DMA_RADRA_DMA_RADRA) >> ADC0_DMA_RADRA_DMA_RADRA_POS);
}

/**
 * @brief  Set the SEQB DMA transfer configuration
 * @note   This function handles both DMAEN and DMA_MODE bits
 * @rmtoll CTRLB         DMAEN,DMA_MODE            LL_ADC0_SEQB_SetDMATransfer
 * @param  ADC0x ADC0 Instance
 * @param  DMATransfer This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_DMA_TRANSFER_DISABLE
 *         @arg @ref LL_ADC0_DMA_TRANSFER_ONCE
 *         @arg @ref LL_ADC0_DMA_TRANSFER_CONTINUOUS
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_SetDMATransfer(ADC0_TypeDef *ADC0x, uint32_t DMATransfer)
{
    MODIFY_REG(ADC0x->CTRLB, (ADC0_CTRLB_DMAEN | ADC0_CTRLB_DMA_MODE), DMATransfer);
}

/**
 * @brief  Get the SEQB DMA transfer configuration
 * @rmtoll CTRLB         DMAEN,DMA_MODE            LL_ADC0_SEQB_GetDMATransfer
 * @param  ADC0x ADC0 Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_ADC0_DMA_TRANSFER_DISABLE
 *         @arg @ref LL_ADC0_DMA_TRANSFER_ONCE
 *         @arg @ref LL_ADC0_DMA_TRANSFER_CONTINUOUS
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQB_GetDMATransfer(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->CTRLB, (ADC0_CTRLB_DMAEN | ADC0_CTRLB_DMA_MODE)));
}

/**
 * @brief  Set the SEQB DMA base address
 * @rmtoll DMA_RBADRB         DMA_RBADRB            LL_ADC0_SEQB_SetDMABaseAddress
 * @param  ADC0x ADC0 Instance
 * @param  BaseAddress Between 0 to 0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_SetDMABaseAddress(ADC0_TypeDef *ADC0x, uint32_t BaseAddress)
{
    MODIFY_REG(ADC0x->DMA_RBADRB, ADC0_DMA_RBADRB_DMA_RBADRB, (BaseAddress << ADC0_DMA_RBADRB_DMA_RBADRB_POS));
}

/**
 * @brief  Get the SEQB DMA base address
 * @rmtoll DMA_RBADRB         DMA_RBADRB            LL_ADC0_SEQB_GetDMABaseAddress
 * @param  ADC0x ADC0 Instance
 * @retval SEQB DMA base address
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQB_GetDMABaseAddress(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->DMA_RBADRB, ADC0_DMA_RBADRB_DMA_RBADRB) >> ADC0_DMA_RBADRB_DMA_RBADRB_POS);
}

/**
 * @brief  Set the SEQB DMA length
 * @rmtoll DMA_RLENB         DMA_RLENB            LL_ADC0_SEQB_SetDMALength
 * @param  ADC0x ADC0 Instance
 * @param  Length Between 0 to 0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_ADC0_SEQB_SetDMALength(ADC0_TypeDef *ADC0x, uint32_t Length)
{
    MODIFY_REG(ADC0x->DMA_RLENB, ADC0_DMA_RLENB_DMA_RLENB, (Length << ADC0_DMA_RLENB_DMA_RLENB_POS));
}

/**
 * @brief  Get the SEQB DMA length
 * @rmtoll DMA_RLENB         DMA_RLENB            LL_ADC0_SEQB_GetDMALength
 * @param  ADC0x ADC0 Instance
 * @retval SEQB DMA length
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQB_GetDMALength(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->DMA_RLENB, ADC0_DMA_RLENB_DMA_RLENB) >> ADC0_DMA_RLENB_DMA_RLENB_POS);
}

/**
 * @brief  Get the SEQB DMA current address
 * @rmtoll DMA_RADRB         DMA_RADRB            LL_ADC0_SEQB_GetDMACurrentAddress
 * @param  ADC0x ADC0 Instance
 * @retval SEQB DMA current address
 */
__STATIC_INLINE uint32_t LL_ADC0_SEQB_GetDMACurrentAddress(ADC0_TypeDef *ADC0x)
{
    return (uint32_t)(READ_BIT(ADC0x->DMA_RADRB, ADC0_DMA_RADRB_DMA_RADRB) >> ADC0_DMA_RADRB_DMA_RADRB_POS);
}

/**
 * @}
 */ /* end of group ADC0_LL_EF_DMA_Configuration */
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup ADC_LL_EF_Init Initialization and de-initialization functions
 * @{
 */
ErrorStatus LL_ADC0_BasicInit(ADC0_TypeDef *ADC0x, LL_ADC0_InitTypeDef *Init);
ErrorStatus LL_ADC0_CommonInit(ADC0_TypeDef *ADC0x, LL_ADC0_CommonInitTypeDef *Common);
ErrorStatus LL_ADC0_OversamplingInit(ADC0_TypeDef *ADC0x, LL_ADC0_OversamplingInitTypeDef *Ovs);
ErrorStatus LL_ADC0_CalibrationConfig(ADC0_TypeDef *ADC0x, LL_ADC0_CalibrationTypeDef *Cal);
ErrorStatus LL_ADC0_SeqInit(ADC0_TypeDef *ADC0x, uint32_t Seq, LL_ADC0_SeqInitTypeDef *SeqInit);
ErrorStatus LL_ADC0_SeqAWDInit(ADC0_TypeDef *ADC0x, uint32_t Seq, LL_ADC0_SeqAWDInitTypeDef *SeqAWDInit);
ErrorStatus LL_ADC0_SeqDMAInit(ADC0_TypeDef *ADC0x, uint32_t Seq, LL_ADC0_SeqDMAInitTypeDef *SeqDMAInit);
ErrorStatus LL_ADC0_DeInit(ADC0_TypeDef *ADC0x);
void        LL_ADC0_BasicStructInit(LL_ADC0_InitTypeDef *Init);
void        LL_ADC0_CommonStructInit(LL_ADC0_CommonInitTypeDef *Common);
void        LL_ADC0_OversamplingStructInit(LL_ADC0_OversamplingInitTypeDef *Ovs);
void        LL_ADC0_CalibrationStructInit(LL_ADC0_CalibrationTypeDef *Cal);
void        LL_ADC0_SeqStructInit(LL_ADC0_SeqInitTypeDef *SeqInit);
void        LL_ADC0_SeqAWDStructInit(LL_ADC0_SeqAWDInitTypeDef *SeqAWDInit);
void        LL_ADC0_SeqDMAStructInit( LL_ADC0_SeqDMAInitTypeDef *SeqDMAInit);
/**
 * @}
 */ /* end of group ADC_LL_EF_Init */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */

/**
 * @}
 */

#endif /* ADC0_CORE0 || ADC0_CORE1 */

/**
 * @}
 */ /* end of group RN_LL_Driver */

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_LL_ADC0_H */
