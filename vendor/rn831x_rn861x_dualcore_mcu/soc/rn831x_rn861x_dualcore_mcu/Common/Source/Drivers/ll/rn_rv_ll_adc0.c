/**
 ******************************************************************************
 * @file   rn_rv_ll_adc0.c
 * @author Renergy Software Team
 * @brief  ADC0 LL module driver.
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
#include "rn_rv_ll_adc0.h"
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

/** @addtogroup ADC0_LL
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
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

/** @addtogroup ADC0_LL_Private_Macros
 * @{
 */
#define IS_LL_ADC0_SEQ(__VALUE__)              (((__VALUE__)==LL_ADC0_SEQA) || ((__VALUE__)==LL_ADC0_SEQB))
#define IS_LL_ADC0_FUNCTION(__VALUE__)         (((__VALUE__)==LL_ADC0_FUNCTION_DISABLE) || ((__VALUE__)==LL_ADC0_FUNCTION_ENABLE))
#define IS_LL_ADC0_RESOLUTION(__VALUE__) \
  (((__VALUE__)==LL_ADC0_RESOLUTION_12B) || ((__VALUE__)==LL_ADC0_RESOLUTION_10B) || \
   ((__VALUE__)==LL_ADC0_RESOLUTION_8B)  || ((__VALUE__)==LL_ADC0_RESOLUTION_6B))

#define IS_LL_ADC0_DATA_ALIGN(__VALUE__) \
  (((__VALUE__)==LL_ADC0_DATA_ALIGN_RIGHT) || ((__VALUE__)==LL_ADC0_DATA_ALIGN_LEFT))

#define IS_LL_ADC0_CLKDIV(__VALUE__) ((__VALUE__) <= 0x7FU)

#define IS_LL_ADC0_DAC_DEALMODE(__VALUE__) \
  (((__VALUE__)==LL_ADC0_DACCONV_DEALMODE_CONTINUE) || ((__VALUE__)==LL_ADC0_DACCONV_DEALMODE_PAUSE))

#define IS_LL_ADC0_DUMMY_CONV(__VALUE__) \
  (((__VALUE__)==LL_ADC0_DUMMY_CONV_0) || ((__VALUE__)==LL_ADC0_DUMMY_CONV_1) || \
   ((__VALUE__)==LL_ADC0_DUMMY_CONV_2) || ((__VALUE__)==LL_ADC0_DUMMY_CONV_3))

#define IS_LL_ADC0_INTV_TIME(__VALUE__) ((__VALUE__) <= 0x1FU)
#define IS_LL_ADC0_PWRUPT_ADC(__VALUE__) ((__VALUE__) <= 0x3U)
#define IS_LL_ADC0_PWRUPT_BUFF(__VALUE__) ((__VALUE__) <= 0xFFU)

#define IS_LL_ADC0_OVS_ACC(__VALUE__) \
  (((__VALUE__)==LL_ADC0_OVERSAMPLING_ACCUMULATE_2)   || ((__VALUE__)==LL_ADC0_OVERSAMPLING_ACCUMULATE_4)   || \
   ((__VALUE__)==LL_ADC0_OVERSAMPLING_ACCUMULATE_8)   || ((__VALUE__)==LL_ADC0_OVERSAMPLING_ACCUMULATE_16)  || \
   ((__VALUE__)==LL_ADC0_OVERSAMPLING_ACCUMULATE_32)  || ((__VALUE__)==LL_ADC0_OVERSAMPLING_ACCUMULATE_64)  || \
   ((__VALUE__)==LL_ADC0_OVERSAMPLING_ACCUMULATE_128) || ((__VALUE__)==LL_ADC0_OVERSAMPLING_ACCUMULATE_256))

#define IS_LL_ADC0_OVS_SHIFT(__VALUE__) \
  (((__VALUE__)==LL_ADC0_OVERSAMPLING_SHIFT_NONE) || ((__VALUE__)==LL_ADC0_OVERSAMPLING_SHIFT_1) || \
   ((__VALUE__)==LL_ADC0_OVERSAMPLING_SHIFT_2)    || ((__VALUE__)==LL_ADC0_OVERSAMPLING_SHIFT_3) || \
   ((__VALUE__)==LL_ADC0_OVERSAMPLING_SHIFT_4)    || ((__VALUE__)==LL_ADC0_OVERSAMPLING_SHIFT_5) || \
   ((__VALUE__)==LL_ADC0_OVERSAMPLING_SHIFT_6)    || ((__VALUE__)==LL_ADC0_OVERSAMPLING_SHIFT_7) || \
   ((__VALUE__)==LL_ADC0_OVERSAMPLING_SHIFT_8))

#define IS_LL_ADC0_CHANNEL_MASK14(__VALUE__) (((__VALUE__) & ~0x3FFFU) == 0U)
#define IS_LL_ADC0_CHANNEL_INDEX(__VALUE__)  ((__VALUE__) <= 13U)

#define IS_LL_ADC0_SEQDISCONT(__VALUE__) \
  (((__VALUE__)==LL_ADC0_SEQUENCE_DISCONT_DISABLE) || ((__VALUE__)==LL_ADC0_SEQUENCE_DISCONT_1CH) || \
   ((__VALUE__)==LL_ADC0_SEQUENCE_DISCONT_2CH)     || ((__VALUE__)==LL_ADC0_SEQUENCE_DISCONT_3CH) || \
   ((__VALUE__)==LL_ADC0_SEQUENCE_DISCONT_4CH))

#define IS_LL_ADC0_OVERFLOW_MODE(__VALUE__) \
  (((__VALUE__)==LL_ADC0_OVERFLOW_MODE_NO_RECOVER) || ((__VALUE__)==LL_ADC0_OVERFLOW_MODE_RECOVER))

#define IS_LL_ADC0_PREEMPT_MODE(__VALUE__) \
  (((__VALUE__)==LL_ADC0_PREEMPT_MODE_STOP) || ((__VALUE__)==LL_ADC0_PREEMPT_MODE_WAIT))

#define IS_LL_ADC0_AWD_MODE(__VALUE__) (((__VALUE__)==LL_ADC0_AWD_MODE_DISABLE_AWD)\
                             || ((__VALUE__)==LL_ADC0_AWD_MODE_BETWEEN_HIGH_LOW)\
                             || ((__VALUE__)==LL_ADC0_AWD_MODE_ABOVE_LOW)\
                             || ((__VALUE__)==LL_ADC0_AWD_MODE_BELOW_HIGH))

#define IS_LL_ADC0_DMA_TRANSFER(__VALUE__) \
  (((__VALUE__)==LL_ADC0_DMA_TRANSFER_DISABLE) || ((__VALUE__)==LL_ADC0_DMA_TRANSFER_ONCE) || \
   ((__VALUE__)==LL_ADC0_DMA_TRANSFER_CONTINUOUS))

#define IS_LL_ADC0_DMA_ADDR_LEN(__VALUE__) ((__VALUE__) <= 0x3FFFFU)
/**
 * @}
 */ /* end of group ADC0_LL_Private_Macros */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @addtogroup ADC0_LL_Exported_Functions
 * @{
 */

/** @addtogroup ADC0_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize registers of ADC0 instances to default reset values.
 * @param  ADC0x ADC0 instance
 * @retval An ErrorStatus enumeration value:
 *          - SUCCESS: ADC0 registers are de-initialized
 *          - ERROR: not applicable
 */
ErrorStatus LL_ADC0_DeInit(ADC0_TypeDef *ADC0x)
{
   /* Check the parameters */
   assert_param(IS_ADC0_ALL_INSTANCE(ADC0x));

   if (ADC0x == ADC0_CORE0) {
      /* Force reset of ADC0 clock (core clock) */
      LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_ADC0);
      /* Release reset of ADC0 clock (core clock) */
      LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_ADC0);
   } else if (ADC0x == ADC0_CORE1) {
      LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE1_MOD0_PERIPH_ADC0);
      /* Release reset of ADC0 clock (core clock) */
      LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE1_MOD0_PERIPH_ADC0);
   } else {
      return ERROR;
   }

   return SUCCESS;
}

/**
 * @brief  Initialize the ADC0 basic configuration.
 * @note   Must be called when both sequences are stopped.
 * @param  ADC0x ADC0 instance
 * @param  Init Pointer to LL_ADC0_InitTypeDef
 * @retval SUCCESS or ERROR
 */
ErrorStatus LL_ADC0_BasicInit(ADC0_TypeDef *ADC0x, LL_ADC0_InitTypeDef *Init)
{
   assert_param(IS_LL_ADC0_RESOLUTION(Init->Resolution));
   assert_param(IS_LL_ADC0_DATA_ALIGN(Init->DataAlignment));
   assert_param(IS_LL_ADC0_CLKDIV(Init->ClockDivision));

   LL_ADC0_SetResolution(ADC0x, Init->Resolution);
   LL_ADC0_SetDataAlignment(ADC0x, Init->DataAlignment);
   LL_ADC0_SetClockDivision(ADC0x, Init->ClockDivision);
   return SUCCESS;
}

/**
 * @brief  Initialize ADC0 oversampling configuration.
 * @note   Must be called when both sequences are stopped.
 * @param  ADC0x ADC0 instance
 * @param  Ovs Pointer to LL_ADC0_OversamplingInitTypeDef
 * @retval SUCCESS or ERROR
 */
ErrorStatus LL_ADC0_OversamplingInit(ADC0_TypeDef *ADC0x, LL_ADC0_OversamplingInitTypeDef *Ovs)
{
   assert_param(IS_LL_ADC0_OVS_ACC(Ovs->OverSamplingAccumulationCount));
   assert_param(IS_LL_ADC0_OVS_SHIFT(Ovs->OverSamplingRightShift));
   assert_param(IS_LL_ADC0_CHANNEL_MASK14(Ovs->OverSamplingChannels));
   assert_param(IS_LL_ADC0_FUNCTION(Ovs->OverSampling));

   if (Ovs->OverSampling) {
      LL_ADC0_EnableOverSampling(ADC0x);
   } else {
      LL_ADC0_DisableOverSampling(ADC0x);
   }
   LL_ADC0_SetOverSamplingAccumulationCount(ADC0x, Ovs->OverSamplingAccumulationCount);
   LL_ADC0_SetOverSamplingRightShift(ADC0x, Ovs->OverSamplingRightShift);
   LL_ADC0_SetOverSamplingChannels(ADC0x, Ovs->OverSamplingChannels);
   return SUCCESS;
}
/**
 * @brief  Initialize ADC0 common working parameters.
 * @note   Must be called when both sequences are stopped.
 * @param  ADC0x ADC0 instance
 * @param  Common Pointer to LL_ADC0_CommonInitTypeDef
 * @retval SUCCESS or ERROR
 */
ErrorStatus LL_ADC0_CommonInit(ADC0_TypeDef *ADC0x, LL_ADC0_CommonInitTypeDef *Common)
{
   assert_param(IS_LL_ADC0_DAC_DEALMODE(Common->DACConversionDeal));
   assert_param(IS_LL_ADC0_DUMMY_CONV(Common->DummyConversionCount));
   assert_param(IS_LL_ADC0_INTV_TIME(Common->ConversionIntervalTime));
   assert_param(IS_LL_ADC0_PWRUPT_ADC(Common->ADCPowerUpTime));
   assert_param(IS_LL_ADC0_PWRUPT_BUFF(Common->BufferPowerUpTime));
   LL_ADC0_SetDACConversionArbitrationDealMode(ADC0x, Common->DACConversionDeal);
   LL_ADC0_SetDummyConversionCount(ADC0x, Common->DummyConversionCount);
   LL_ADC0_SetConversionIntervalTime(ADC0x, Common->ConversionIntervalTime);
   LL_ADC0_SetADCPowerUpTime(ADC0x, Common->ADCPowerUpTime);
   LL_ADC0_SetBufferPowerUpTime(ADC0x, Common->BufferPowerUpTime);
   return SUCCESS;
}
/**
 * @brief  Configure calibration constants and digital offset.
 * @note   Must be called when both sequences are stopped.
 * @param  ADC0x ADC0 instance
 * @param  Cal Pointer to LL_ADC0_CalibrationTypeDef
 * @retval SUCCESS or ERROR
 */
ErrorStatus LL_ADC0_CalibrationConfig(ADC0_TypeDef *ADC0x, LL_ADC0_CalibrationTypeDef *Cal)
{
   LL_ADC0_SetNormalZEROT(ADC0x, Cal->NormalZEROT);
   LL_ADC0_SetBufferZEROT(ADC0x, Cal->BufferZEROT);
   LL_ADC0_SetDigitalOffset(ADC0x, Cal->DigitalOffset);
   return SUCCESS;
}

/**
 * @brief  Initialize an ADC0 sequence configuration (Sequence A or B).
 * @param  ADC0x ADC0 instance
 * @param  Seq This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_SEQA
 *         @arg @ref LL_ADC0_SEQB
 * @param  SeqInit Pointer to LL_ADC0_SeqInitTypeDef
 * @retval SUCCESS or ERROR
 */
ErrorStatus LL_ADC0_SeqInit(ADC0_TypeDef *ADC0x, uint32_t Seq, LL_ADC0_SeqInitTypeDef *SeqInit)
{
   assert_param(IS_LL_ADC0_SEQ(Seq));
   assert_param(IS_LL_ADC0_FUNCTION(SeqInit->MultiChannelsMode));
   assert_param(IS_LL_ADC0_FUNCTION(SeqInit->ContinuousConversion));
   assert_param(IS_LL_ADC0_SEQDISCONT(SeqInit->SequenceDiscont));
   assert_param(IS_LL_ADC0_OVERFLOW_MODE(SeqInit->OverflowMode));
   if (SeqInit->MultiChannelsMode){
      assert_param(IS_LL_ADC0_CHANNEL_INDEX(SeqInit->ChannelSelection));
   }
   else{
      assert_param(IS_LL_ADC0_CHANNEL_MASK14(SeqInit->ChannelSelection));
   }
   /* Multi-channels mode */
   if (Seq == LL_ADC0_SEQA) {
      if (SeqInit->MultiChannelsMode ) {
         LL_ADC0_SEQA_EnableMultiChannelsMode(ADC0x);
         LL_ADC0_SEQA_SetSequenceChannels(ADC0x, SeqInit->ChannelSelection);
      }
      else {
         LL_ADC0_SEQA_DisableMultiChannelsMode(ADC0x);
         LL_ADC0_SEQA_SetChannel(ADC0x, SeqInit->ChannelSelection);
      }
      if (SeqInit->ContinuousConversion ) LL_ADC0_SEQA_EnableContinuousConversion(ADC0x);
      else                               LL_ADC0_SEQA_DisableContinuousConversion(ADC0x);
      LL_ADC0_SEQA_SetSequenceDiscont(ADC0x, SeqInit->SequenceDiscont);
      LL_ADC0_SEQA_SetOverflowMode(ADC0x, SeqInit->OverflowMode);
   }
   if (Seq == LL_ADC0_SEQB) {
      assert_param(IS_LL_ADC0_PREEMPT_MODE(SeqInit->PreemptMode));
      if (SeqInit->MultiChannelsMode ) {
         LL_ADC0_SEQB_EnableMultiChannelsMode(ADC0x);
         LL_ADC0_SEQB_SetSequenceChannels(ADC0x, SeqInit->ChannelSelection);
      }
      else {
         LL_ADC0_SEQB_DisableMultiChannelsMode(ADC0x);
         LL_ADC0_SEQB_SetChannel(ADC0x, SeqInit->ChannelSelection);
      }
      if (SeqInit->ContinuousConversion ) LL_ADC0_SEQB_EnableContinuousConversion(ADC0x);
      else                               LL_ADC0_SEQB_DisableContinuousConversion(ADC0x);
      LL_ADC0_SEQB_SetSequenceDiscont(ADC0x, SeqInit->SequenceDiscont);
      LL_ADC0_SEQB_SetOverflowMode(ADC0x, SeqInit->OverflowMode);
      LL_ADC0_SEQB_SetPreemptMode(ADC0x, SeqInit->PreemptMode);
   }
   return SUCCESS;
}
/**
 * @brief  Initialize analog watchdog configuration for a sequence.
 * @param  ADC0x ADC0 instance
 * @param  Seq This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_SEQA
 *         @arg @ref LL_ADC0_SEQB
 * @param  SeqAWDInit Pointer to LL_ADC0_SeqAWDInitTypeDef
 * @retval SUCCESS or ERROR
 */
ErrorStatus LL_ADC0_SeqAWDInit(ADC0_TypeDef *ADC0x, uint32_t Seq, LL_ADC0_SeqAWDInitTypeDef *SeqAWDInit)
{
   assert_param(IS_LL_ADC0_SEQ(Seq));
   assert_param(IS_LL_ADC0_AWD_MODE(SeqAWDInit->AWDMode));
   assert_param(IS_LL_ADC0_CHANNEL_MASK14(SeqAWDInit->AWDMonitorChannels));
   if (Seq == LL_ADC0_SEQA) {
      LL_ADC0_SEQA_SetAnalogWDMode(ADC0x, SeqAWDInit->AWDMode);
      LL_ADC0_SEQA_SetAnalogWDThresholds(ADC0x, LL_ADC0_AWD_THRESHOLD_HIGH, SeqAWDInit->AWDThresholdHigh);
      LL_ADC0_SEQA_SetAnalogWDThresholds(ADC0x, LL_ADC0_AWD_THRESHOLD_LOW, SeqAWDInit->AWDThresholdLow);
      LL_ADC0_SEQA_SetAnalogWDMonitChannels(ADC0x, SeqAWDInit->AWDMonitorChannels);
   }
   if (Seq == LL_ADC0_SEQB){
      LL_ADC0_SEQB_SetAnalogWDMode(ADC0x, SeqAWDInit->AWDMode);
      LL_ADC0_SEQB_SetAnalogWDThresholds(ADC0x, LL_ADC0_AWD_THRESHOLD_HIGH, SeqAWDInit->AWDThresholdHigh);
      LL_ADC0_SEQB_SetAnalogWDThresholds(ADC0x, LL_ADC0_AWD_THRESHOLD_LOW, SeqAWDInit->AWDThresholdLow);
      LL_ADC0_SEQB_SetAnalogWDMonitChannels(ADC0x, SeqAWDInit->AWDMonitorChannels);
   }

   return SUCCESS;
}

/**
 * @brief  Initialize DMA configuration for a sequence.
 * @param  ADC0x ADC0 instance
 * @param  Seq This parameter can be one of the following values:
 *         @arg @ref LL_ADC0_SEQA
 *         @arg @ref LL_ADC0_SEQB
 * @param  SeqDMAInit Pointer to LL_ADC0_SeqDMAInitTypeDef
 * @retval SUCCESS or ERROR
 */
ErrorStatus LL_ADC0_SeqDMAInit(ADC0_TypeDef *ADC0x, uint32_t Seq, LL_ADC0_SeqDMAInitTypeDef *SeqDMAInit)
{
   assert_param(IS_LL_ADC0_SEQ(Seq));
   assert_param(IS_LL_ADC0_DMA_TRANSFER(SeqDMAInit->DMATransfer));
   assert_param(IS_LL_ADC0_DMA_ADDR_LEN(SeqDMAInit->DMABaseAddress));
   assert_param(IS_LL_ADC0_DMA_ADDR_LEN(SeqDMAInit->DMALength));

   if (Seq == LL_ADC0_SEQA) {
      LL_ADC0_SEQA_SetDMATransfer(ADC0x, SeqDMAInit->DMATransfer);
      LL_ADC0_SEQA_SetDMABaseAddress(ADC0x, SeqDMAInit->DMABaseAddress);
      LL_ADC0_SEQA_SetDMALength(ADC0x, SeqDMAInit->DMALength);
   }
   if (Seq == LL_ADC0_SEQB){
      LL_ADC0_SEQB_SetDMATransfer(ADC0x, SeqDMAInit->DMATransfer);
      LL_ADC0_SEQB_SetDMABaseAddress(ADC0x, SeqDMAInit->DMABaseAddress);
      LL_ADC0_SEQB_SetDMALength(ADC0x, SeqDMAInit->DMALength);
   }

    return SUCCESS;
}
/**
 * @brief  Set each @ref LL_ADC0_InitTypeDef field to default value.
 * @param  Init Pointer to a @ref LL_ADC0_InitTypeDef structure
 *                            whose fields will be set to default values.
 * @retval None
 */
void LL_ADC0_BasicStructInit(LL_ADC0_InitTypeDef *Init)
{
   assert_param(Init != NULL);
   /* Resolution/DataAlignment per exported constants */
   Init->Resolution    = LL_ADC0_RESOLUTION_12B;   /* 12-bit default */
   Init->DataAlignment = LL_ADC0_DATA_ALIGN_RIGHT; /* Right alignment */
   /* Bus clk/(CLKDIV+1), choose 0 => undivided */
   Init->ClockDivision = 0U;
}
/**
 * @brief  Set each @ref LL_ADC0_CommonInitTypeDef field to default value.
 * @param  Common Pointer to a @ref LL_ADC0_CommonInitTypeDef structure
 *                            whose fields will be set to default values.
 * @retval None
 */
void LL_ADC0_CommonStructInit(LL_ADC0_CommonInitTypeDef *Common)
{
   assert_param(Common != NULL);
   /* DAC arbitration: continue converting by default */
   Common->DACConversionDeal      = LL_ADC0_DACCONV_DEALMODE_CONTINUE;
   /* Dummy conversions: 0 time */
   Common->DummyConversionCount   = 1U;
   /* Inter-conversion wait clocks: 0 */
   Common->ConversionIntervalTime = 0U;
   /* Power-up stabilization times: 0 */
   Common->ADCPowerUpTime         = 0x01U;
   Common->BufferPowerUpTime      = 0x4FU;
}
/**
 * @brief  Set each @ref LL_ADC0_OversamplingInitTypeDef field to default value.
 * @param  Ovs Pointer to a @ref LL_ADC0_OversamplingInitTypeDef structure
 *                            whose fields will be set to default values.
 * @retval None
 */
void LL_ADC0_OversamplingStructInit(LL_ADC0_OversamplingInitTypeDef *Ovs)
{
   assert_param(Ovs != NULL);
   /* Oversampling disabled by default */
   Ovs->OverSampling                     = 0U;
   Ovs->OverSamplingAccumulationCount    = LL_ADC0_OVERSAMPLING_ACCUMULATE_2;
   Ovs->OverSamplingRightShift           = LL_ADC0_OVERSAMPLING_SHIFT_NONE;
   /* Channels bitmask: 0 => none selected */
   Ovs->OverSamplingChannels             = LL_ADC0_CHANNEL_NULL;
}
/**
 * @brief  Set each @ref LL_ADC0_CalibrationStructInit field to default value.
 * @param  Cal Pointer to a @ref LL_ADC0_CalibrationStructInit structure
 *                            whose fields will be set to default values.
 * @retval None
 */
void LL_ADC0_CalibrationStructInit(LL_ADC0_CalibrationTypeDef *Cal)
{
   assert_param(Cal != NULL);
   Cal->NormalZEROT   = 0x800U;   /* 0..0xFFF 的 12 位范围内安全缺省值，参见 LL_ADC0_SetNormalZEROT 的约束[^33] */
   Cal->BufferZEROT   = 0x800U;   /* 0..0xFFF 的 12 位范围内安全缺省值，参见 LL_ADC0_SetBufferZEROT 的约束[^34] */
   Cal->DigitalOffset = 0U;   /* 12 位二补码窗口内的 0，参见 LL_ADC0_SetDigitalOffset 的约束[^36] */
}

/**
 * @brief  Set each @ref LL_ADC0_SeqStructInit field to default value.
 * @param  SeqInit Pointer to a @ref LL_ADC0_SeqStructInit structure
 *                            whose fields will be set to default values.
 * @retval None
 */
void LL_ADC0_SeqStructInit(LL_ADC0_SeqInitTypeDef *SeqInit)
{
   assert_param(SeqInit != NULL);
   SeqInit->MultiChannelsMode    = 0U;
   SeqInit->ContinuousConversion = 0U;
   SeqInit->ChannelSelection     = 0U;
   SeqInit->SequenceDiscont      = LL_ADC0_SEQUENCE_DISCONT_DISABLE;
   SeqInit->OverflowMode         = LL_ADC0_OVERFLOW_MODE_NO_RECOVER;
   SeqInit->PreemptMode          = LL_ADC0_CHANNEL_NULL;
}

/**
 * @brief  Set each @ref LL_ADC0_SeqAWDStructInit field to default value.
 * @param  SeqAWDInit Pointer to a @ref LL_ADC0_SeqAWDStructInit structure
 *                            whose fields will be set to default values.
 * @retval None
 */
void LL_ADC0_SeqAWDStructInit(LL_ADC0_SeqAWDInitTypeDef *SeqAWDInit)
{
   assert_param(SeqAWDInit != NULL);
   SeqAWDInit->AWDMode            = LL_ADC0_AWD_MODE_DISABLE_AWD;
   SeqAWDInit->AWDThresholdHigh   = 0xFFFFU;
   SeqAWDInit->AWDThresholdLow    = 0x0000U;
   SeqAWDInit->AWDMonitorChannels = LL_ADC0_CHANNEL_NULL;
}

/**
 * @brief  Set each @ref LL_ADC0_SeqDMAStructInit field to default value.
 * @param  SeqDMAInit Pointer to a @ref LL_ADC0_SeqDMAStructInit structure
 *                            whose fields will be set to default values.
 * @retval None
 */
void LL_ADC0_SeqDMAStructInit(LL_ADC0_SeqDMAInitTypeDef *SeqDMAInit)
{
   assert_param(SeqDMAInit != NULL);
   SeqDMAInit->DMATransfer    = LL_ADC0_DMA_TRANSFER_DISABLE;
   SeqDMAInit->DMABaseAddress = 0U;
   SeqDMAInit->DMALength      = 0U;
}
/**
 * @}
 */ /* end of group ADC0_LL_EF_Init */

/**
 * @}
 */ /* end of group ADC0_LL_Exported_Functions */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */ /* end of group ADC0_LL */
/**
 * @}
 */ /* end of group RN_LL_Driver */
