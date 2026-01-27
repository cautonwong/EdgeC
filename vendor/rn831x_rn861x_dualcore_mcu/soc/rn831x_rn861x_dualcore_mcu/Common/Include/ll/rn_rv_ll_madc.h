/**
 ******************************************************************************
 * @file   rn_rv_ll_madc.h
 * @author Renergy Software Team
 * @brief  Header file of MADC LL module driver.
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
#ifndef __RN_RV_LL_MADC_H
#define __RN_RV_LL_MADC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu.h"
#include "rn831x_rn861x_mcu_madc.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

/** @defgroup MADC_LL MADC
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/** @defgroup MADC_LL_ES_INIT MADC Exported Init structure
 * @{
 */
/**
 * @brief MADC Initialization structure definition.
 */
typedef struct
{
    uint32_t Channel;        /*!< Specifies the channel value.
                                       This parameter can be a value of @ref MADC_LL_EC_CHANNEL.

                                       This feature can be modified afterwards using unitary function @ref LL_MADC_SetChannel().*/
    uint32_t PGASelection;        /*!< Specifies the selectpga value.
                                       This parameter can be a value of @ref MADC_LL_EC_PGA.

                                       This feature can be modified afterwards using unitary function @ref LL_MADC_SetPGASelection().*/

    uint32_t ConversionTime;        /*!< Specifies the conversiontime value.
                                       This parameter must be a number between Min_Data=0x20 and Max_Data=0x7F.

                                       This feature can be modified afterwards using unitary function @ref LL_MADC_SetConversionTime()}.*/
    uint32_t AccumulationNumber;        /*!< Specifies the accumulationnumber value.
                                       This parameter must be a number between Min_Data=0x0 and Max_Data=255.

                                       This feature can be modified afterwards using unitary function @ref LL_MADC_SetAccumulationNumber().*/
} LL_MADC_InitTypeDef;
/**
 * @}
 */ /* end of group MADC_LL_ES_INIT */

/* Exported constants -------------------------------------------------------*/
/** @defgroup MADC_LL_Exported_Constants MADC Exported Constants
 * @{
 */

/** @defgroup MADC_LL_EC_Channel Channel Selection
 * @{
 */

#define LL_MADC_CHANNEL_TEMP                       0x00000000U                            /*!< selected as TEMP Sensor*/
#define LL_MADC_CHANNEL_VBAT                       0x00000001U                            /*!< selected as VBAT (after 1/2 voltage division)  */
#define LL_MADC_CHANNEL_AIN0                       0x00000002U                            /*!< selected as AIN0, input from PA00 */
#define LL_MADC_CHANNEL_AIN1                       0x00000003U                            /*!< selected as AIN1, input from PA01 */
#define LL_MADC_CHANNEL_AIN2                       0x00000004U                            /*!< selected as AIN2, input from PA02 */
#define LL_MADC_CHANNEL_AIN3                       0x00000005U                            /*!< selected as AIN3, input from PA03 */
#define LL_MADC_CHANNEL_AIN4                       0x00000006U                            /*!< selected as AIN4, input from PA04 */
#define LL_MADC_CHANNEL_AIN5                       0x00000007U                            /*!< selected as AIN5, input from PL01 */
#define LL_MADC_CHANNEL_AIN6                       (MADC_AD_CTRL_SAR_CH3|0x00000000U)     /*!< selected as AIN6, input from PL02 (after 1/2 voltage division) */
#define LL_MADC_CHANNEL_AIN7                       (MADC_AD_CTRL_SAR_CH3|0x00000001U)     /*!< selected as AIN7, input from PL03 */
#define LL_MADC_CHANNEL_AIN8                       (MADC_AD_CTRL_SAR_CH3|0x00000002U)     /*!< selected as AIN8, input from PA11 */
#define LL_MADC_CHANNEL_AIN9                       (MADC_AD_CTRL_SAR_CH3|0x00000003U)     /*!< selected as AIN9, input from PA12 */
/**
 * @}
 */

/** @defgroup MADC_LL_EC_PGA PGA Selection
 * @{
 */
#define LL_MADC_PGA_0P5                         (MADC_AD_CTRL1_SAR_PGA_SEL_CTRL1_PGA|MADC_AD_CTRL1_SAR_PGA_0P5)    /*!< PGA select as 0.5 */
#define LL_MADC_PGA_1P0                         (MADC_AD_CTRL1_SAR_PGA_SEL_CTRL1_PGA|MADC_AD_CTRL1_SAR_PGA_1P0)    /*!< PGA select as 1.0 */
#define LL_MADC_PGA_1P5                         (MADC_AD_CTRL1_SAR_PGA_SEL_CTRL1_PGA|MADC_AD_CTRL1_SAR_PGA_1P5)    /*!< PGA select as 1.5 */
#define LL_MADC_PGA_2P0                         (MADC_AD_CTRL1_SAR_PGA_SEL_CTRL1_PGA|MADC_AD_CTRL1_SAR_PGA_2P0)    /*!< PGA select as 2.0 */
#define LL_MADC_PGA_0P25                        (MADC_AD_CTRL1_SAR_PGA_SEL_CTRL1_PGA|MADC_AD_CTRL1_SAR_PGA_0P25)   /*!< PGA select as 0.25 */
/**
 * @}
 */

/** @defgroup MADC_LL_EC_PL01FUNCTION PL01 IO Function
 * @{
 */
#define LL_MADC_PL01FUNCTION_NUL                   MADC_ANA_PAD_PL01_AE_DIS                             /*!< Not function for MADC */
#define LL_MADC_PL01FUNCTION_AIN5                  MADC_ANA_PAD_PL01_AE_EN                              /*!< (IO function) enable pin as AIN5 input */
/**
 * @}
 */ /* end of group RTC_LL_EC_PL01FUNCTION */


/** @defgroup MADC_LL_EC_PL02FUNCTION PL02 IO Function
 * @{
 */
#define LL_MADC_PL02FUNCTION_NUL                   MADC_ANA_PAD_PL02_AE_DIS                             /*!< Not function for MADC */
#define LL_MADC_PL02FUNCTION_AIN6                  MADC_ANA_PAD_PL02_AE_EN                              /*!< (IO function) enable pin as AIN6 input */
/**
 * @}
 */ /* end of group RTC_LL_EC_PL02FUNCTION */

/** @defgroup MADC_LL_EC_PL03FUNCTION PL03 IO Function
 * @{
 */
#define LL_MADC_PL03FUNCTION_NUL                   MADC_ANA_PAD_PL03_AE_DIS                             /*!< Not function for MADC */
#define LL_MADC_PL03FUNCTION_AIN7                  MADC_ANA_PAD_PL03_AE_EN                              /*!< (IO function) enable pin as AIN7 input */
/**
 * @}
 */ /* end of group RTC_LL_EC_PL03FUNCTION */

/**
 * @}
 */ /* end of group MADC_LL_Exported_Constants */

/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @defgroup MADC_LL_Exported_Functions MADC Exported Functions
 * @{
 */



/** @defgroup MADC_LL_EF_Configuration Configuration functions
 * @{
 */

/**
 * @brief  Set the SAR ADC channel selection
 * @rmtoll AD_CTRL         SAR_CH            LL_MADC_SetChannel
 *         AD_CTRL         SAR_CH3           LL_MADC_SetChannel
 * @param  MADCx MADC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_MADC_CHANNEL_TEMP
 *         @arg @ref LL_MADC_CHANNEL_VBAT
 *         @arg @ref LL_MADC_CHANNEL_AIN0
 *         @arg @ref LL_MADC_CHANNEL_AIN1
 *         @arg @ref LL_MADC_CHANNEL_AIN2
 *         @arg @ref LL_MADC_CHANNEL_AIN3
 *         @arg @ref LL_MADC_CHANNEL_AIN4
 *         @arg @ref LL_MADC_CHANNEL_AIN5
 *         @arg @ref LL_MADC_CHANNEL_AIN6
 *         @arg @ref LL_MADC_CHANNEL_AIN7
 *         @arg @ref LL_MADC_CHANNEL_AIN8
 *         @arg @ref LL_MADC_CHANNEL_AIN9
 * @retval None
 */
__STATIC_INLINE void LL_MADC_SetChannel(MADC_TypeDef *MADCx, uint32_t Channel)
{
    MODIFY_REG(MADCx->AD_CTRL, MADC_AD_CTRL_SAR_CH|MADC_AD_CTRL_SAR_CH3, Channel);
}

/**
 * @brief  Get the SAR ADC channel selection
 * @rmtoll AD_CTRL         SAR_CH            LL_MADC_GetChannel
 *         AD_CTRL         SAR_CH3           LL_MADC_GetChannel
 * @param  MADCx MADC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_MADC_CHANNEL_TEMP
 *         @arg @ref LL_MADC_CHANNEL_VBAT
 *         @arg @ref LL_MADC_CHANNEL_AIN0
 *         @arg @ref LL_MADC_CHANNEL_AIN1
 *         @arg @ref LL_MADC_CHANNEL_AIN2
 *         @arg @ref LL_MADC_CHANNEL_AIN3
 *         @arg @ref LL_MADC_CHANNEL_AIN4
 *         @arg @ref LL_MADC_CHANNEL_AIN5
 *         @arg @ref LL_MADC_CHANNEL_AIN6
 *         @arg @ref LL_MADC_CHANNEL_AIN7
 *         @arg @ref LL_MADC_CHANNEL_AIN8
 *         @arg @ref LL_MADC_CHANNEL_AIN9
 */
__STATIC_INLINE uint32_t LL_MADC_GetChannel(MADC_TypeDef *MADCx)
{
    return (uint32_t)(READ_BIT(MADCx->AD_CTRL, MADC_AD_CTRL_SAR_CH|MADC_AD_CTRL_SAR_CH3));
}

/**
 * @brief  Set the MADC PGA configuration
 * @rmtoll AD_CTRL1         SAR_PGA            LL_MADC_SetPGASelection
 *         AD_CTRL1         PGA_SEL            LL_MADC_SetPGASelection
 * @param  MADCx MADC Instance
 * @param  PGA This parameter can be one of the following values:
 *         @arg @ref LL_MADC_PGA_0P5
 *         @arg @ref LL_MADC_PGA_1P0
 *         @arg @ref LL_MADC_PGA_1P5
 *         @arg @ref LL_MADC_PGA_2P0
 *         @arg @ref LL_MADC_PGA_0P25
 * @retval None
 */
__STATIC_INLINE void LL_MADC_SetPGASelection(MADC_TypeDef *MADCx, uint32_t PGA)
{
    MODIFY_REG(MADCx->AD_CTRL1, (MADC_AD_CTRL1_SAR_PGA|MADC_AD_CTRL1_SAR_PGA_SEL), PGA);
}
/**
 * @brief  Get the MADC PGA configuration .
 * @rmtoll AD_CTRL1         SAR_PGA            LL_MADC_GetPGASelection
 *         AD_CTRL1         PGA_SEL            LL_MADC_GetPGASelection
 * @param  MADCx MADC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_MADC_PGA_0P5
 *         @arg @ref LL_MADC_PGA_1P0
 *         @arg @ref LL_MADC_PGA_1P5
 *         @arg @ref LL_MADC_PGA_2P0
 *         @arg @ref LL_MADC_PGA_0P25
 */
__STATIC_INLINE uint32_t LL_MADC_GetPGASelection(MADC_TypeDef *MADCx)
{
    return (uint32_t)(READ_BIT(MADCx->AD_CTRL1, (MADC_AD_CTRL1_SAR_PGA|MADC_AD_CTRL1_SAR_PGA_SEL)));
}

/**
 * @brief  Set the SAR ADC conversion time configuration
 * @note   Value must be greater or equal to 0x20
 * @rmtoll AD_CTRL1         SAR_CONVERT            LL_MADC_SetConversionTime
 * @param  MADCx MADC Instance
 * @param  ConversionTime Between 0x20 to 0x7F
 * @retval None
 */
__STATIC_INLINE void LL_MADC_SetConversionTime(MADC_TypeDef *MADCx, uint32_t ConversionTime)
{
    MODIFY_REG(MADCx->AD_CTRL1, MADC_AD_CTRL1_SAR_CONVERT, (ConversionTime << MADC_AD_CTRL1_SAR_CONVERT_POS));
}

/**
 * @brief  Get the SAR ADC conversion time configuration
 * @rmtoll AD_CTRL1         SAR_CONVERT            LL_MADC_GetConversionTime
 * @param  MADCx MADC Instance
 * @retval SAR ADC conversion time configuration
 */
__STATIC_INLINE uint32_t LL_MADC_GetConversionTime(MADC_TypeDef *MADCx)
{
    return (uint32_t)(READ_BIT(MADCx->AD_CTRL1, MADC_AD_CTRL1_SAR_CONVERT) >> MADC_AD_CTRL1_SAR_CONVERT_POS);
}

/**
 * @brief  Set the SAR ADC accumulation number configuration
 * @rmtoll AD_CTRL3         ACC_CNT            LL_MADC_SetAccumulationNumber
 * @param  MADCx MADC Instance
 * @param  AccumulationNumber Between 0 to 255
 * @retval None
 */
__STATIC_INLINE void LL_MADC_SetAccumulationNumber(MADC_TypeDef *MADCx, uint32_t AccumulationNumber)
{
    MODIFY_REG(MADCx->AD_CTRL3, MADC_AD_CTRL3_ACC_CNT, (AccumulationNumber));
}

/**
 * @brief  Get the SAR ADC accumulation number configuration
 * @rmtoll AD_CTRL3         ACC_CNT            LL_MADC_GetAccumulationNumber
 * @param  MADCx MADC Instance
 * @retval SAR ADC accumulation number configuration
 */
__STATIC_INLINE uint32_t LL_MADC_GetAccumulationNumber(MADC_TypeDef *MADCx)
{
    return (uint32_t)(READ_BIT(MADCx->AD_CTRL3, MADC_AD_CTRL3_ACC_CNT));
}

/**
 * @}
 */ /* end of group MADC_LL_EF_Configuration */


/** @defgroup MADC_LL_EF_DataOperation Data Operation functions
 * @{
 */

/**
 * @brief  start SAR ADC conversion
 * @note only available when getted flag =0 by @ref LL_MADC_IsActiveFlag_TPSBUSY
 * @rmtoll AD_START         AD_START            LL_MADC_EnableStartConversion
 * @param  MADCx MADC Instance
 * @retval None
 */
__STATIC_INLINE void LL_MADC_EnableStartConversion(MADC_TypeDef *MADCx)
{
    SET_BIT(MADCx->AD_START, MADC_AD_START_AD_START);
}

/**
 * @brief  Get the SAR ADC 10-bit result data
 * @note only available when getted flag =1 by @ref LL_MADC_IsActiveFlag_DREADY
 * @rmtoll AD_DATA         AD_DATA            LL_MADC_GetData10bit
 * @param  MADCx MADC Instance
 * @retval SAR ADC 10-bit result data
 */
__STATIC_INLINE uint32_t LL_MADC_GetData10bit(MADC_TypeDef *MADCx)
{
    return (uint32_t)(READ_BIT(MADCx->AD_DATA, MADC_AD_DATA_AD_DATA));
}

/**
 * @brief  Get the SAR ADC 12-bit result data
 * @note only available when getted flag =1 by @ref LL_MADC_IsActiveFlag_DREADY
 * @rmtoll AD_DATA2         AD_DATA2            LL_MADC_GetData12bit
 * @param  MADCx MADC Instance
 * @retval SAR ADC 12-bit result data
 */
__STATIC_INLINE uint32_t LL_MADC_GetData12bit(MADC_TypeDef *MADCx)
{
    return (uint32_t)(READ_BIT(MADCx->AD_DATA2, MADC_AD_DATA2_AD_DATA2));
}

/**
 * @brief  Get the SAR ADC accumulated result data
 * @note only available when getted flag =1 by @ref LL_MADC_IsActiveFlag_DREADY
 * @note   AccumulationNumber setted by @ref LL_MADC_SetAccumulationNumber
 * @rmtoll AD_DATA3         AD_DATA3            LL_MADC_GetDataAccumulated
 * @param  MADCx MADC Instance
 * @retval SAR ADC 16-bit accumulated result data
 */
__STATIC_INLINE uint32_t LL_MADC_GetDataAccumulated(MADC_TypeDef *MADCx)
{
    return (uint32_t)(READ_BIT(MADCx->AD_DATA3, MADC_AD_DATA3_AD_DATA3));
}

/**
 * @}
 */ /* end of group MADC_LL_EF_DataOperation */
/** @defgroup MADC_LL_EF_IO IO functions
 * @{
 */

/**
 * @brief  Set the PL01 analog input function
 * @note   PL01 (VBAT power domain) IO function priority: MADC IO Function > RTC IO function > GPIO IO function
 * @rmtoll ANA_PAD         PL01_AE            LL_MADC_SetFunctionPL01
 * @param  MADCx MADC Instance
 * @param  FunctionPL01 This parameter can be one of the following values:
 *         @arg @ref LL_MADC_PL01FUNCTION_NUL
 *         @arg @ref LL_MADC_PL01FUNCTION_AIN5
 * @retval None
 */
__STATIC_INLINE void LL_MADC_SetFunctionPL01(MADC_TypeDef *MADCx, uint32_t FunctionPL01)
{
    MODIFY_REG(MADCx->ANA_PAD, MADC_ANA_PAD_PL01_AE, FunctionPL01);
}

/**
 * @brief  Get the PL01 analog input function
 * @note   PL01 IO function priority: MADC IO Function > RTC IO function > GPIO IO function
 * @rmtoll ANA_PAD         PL01_AE            LL_MADC_GetFunctionPL01
 * @param  MADCx MADC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref  LL_MADC_PL01FUNCTION_NUL
 *         @arg @ref  LL_MADC_PL01FUNCTION_AIN5
 */
__STATIC_INLINE uint32_t LL_MADC_GetFunctionPL01(MADC_TypeDef *MADCx)
{
    return (uint32_t)(READ_BIT(MADCx->ANA_PAD, MADC_ANA_PAD_PL01_AE));
}

/**
 * @brief  Set the PL02 analog input function
 * @note   PL02 (VBAT power domain) IO function priority: MADC IO Function > RTC IO function > GPIO IO function
 * @rmtoll ANA_PAD         PL02_AE            LL_MADC_SetFunctionPL02
 * @param  MADCx MADC Instance
 * @param  FunctionPL02 This parameter can be one of the following values:
 *         @arg @ref LL_MADC_PL02FUNCTION_NUL
 *         @arg @ref LL_MADC_PL02FUNCTION_AIN6
 * @retval None
 */
__STATIC_INLINE void LL_MADC_SetFunctionPL02(MADC_TypeDef *MADCx, uint32_t FunctionPL02)
{
    MODIFY_REG(MADCx->ANA_PAD, MADC_ANA_PAD_PL02_AE, FunctionPL02);
}

/**
 * @brief  Get the PL02 analog input function
 * @rmtoll ANA_PAD         PL02_AE            LL_MADC_GetFunctionPL02
 * @param  MADCx MADC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_MADC_PL02FUNCTION_NUL
 *         @arg @ref LL_MADC_PL02FUNCTION_AIN6
 */
__STATIC_INLINE uint32_t LL_MADC_GetFunctionPL02(MADC_TypeDef *MADCx)
{
    return (uint32_t)(READ_BIT(MADCx->ANA_PAD, MADC_ANA_PAD_PL02_AE));
}

/**
 * @brief  Set the PL03 analog input function
 * @note   PL03 (VBAT power domain) IO function priority: MADC IO Function > RTC IO function > GPIO IO function
 * @rmtoll ANA_PAD         PL03_AE            LL_MADC_SetFunctionPL03
 * @param  MADCx MADC Instance
 * @param  FunctionPL03 This parameter can be one of the following values:
 *         @arg @ref LL_MADC_PL03FUNCTION_NUL
 *         @arg @ref LL_MADC_PL03FUNCTION_AIN7
 * @retval None
 */
__STATIC_INLINE void LL_MADC_SetFunctionPL03(MADC_TypeDef *MADCx, uint32_t FunctionPL03)
{
    MODIFY_REG(MADCx->ANA_PAD, MADC_ANA_PAD_PL03_AE, FunctionPL03);
}

/**
 * @brief  Get the PL03 analog input function
 * @rmtoll ANA_PAD         PL03_AE            LL_MADC_GetFunctionPL03
 * @param  MADCx MADC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_MADC_PL03FUNCTION_NUL
 *         @arg @ref LL_MADC_PL03FUNCTION_AIN7
 */
__STATIC_INLINE uint32_t LL_MADC_GetFunctionPL03(MADC_TypeDef *MADCx)
{
    return (uint32_t)(READ_BIT(MADCx->ANA_PAD, MADC_ANA_PAD_PL03_AE));
}

/**
 * @}
 */ /* end of group MADC_LL_EF_IO */

/** @defgroup MADC_LL_EF_Reset Reset functions
 * @{
 */

/**
 * @brief  Enable VBAT reset write protection
 * @note   This is a multi-bit enable function
 * @rmtoll ANA_RST         PS            LL_MADC_EnableVBATResetWriteProtection
 * @param  MADCx MADC Instance
 * @retval None
 */
__STATIC_INLINE void LL_MADC_EnableVBATResetWriteProtection(MADC_TypeDef *MADCx)
{
    MODIFY_REG(MADCx->ANA_RST, MADC_ANA_RST_PS, MADC_ANA_RST_PS_DIS);
}

/**
 * @brief  Disable VBAT reset write protection
 * @rmtoll ANA_RST         PS            LL_MADC_DisableVBATResetWriteProtection
 * @param  MADCx MADC Instance
 * @retval None
 */
__STATIC_INLINE void LL_MADC_DisableVBATResetWriteProtection(MADC_TypeDef *MADCx)
{
    MODIFY_REG(MADCx->ANA_RST, MADC_ANA_RST_PS, MADC_ANA_RST_PS_PS);
}

/**
 * @brief  Enable VBAT reset
 * @note   Bits is write-protected. @ref LL_MADC_DisableVBATResetWriteProtection function should be called before.
 * @rmtoll ANA_RST         ANA_RST            LL_MADC_EnableVBATReset
 * @param  MADCx MADC Instance
 * @retval None
 */
__STATIC_INLINE void LL_MADC_EnableVBATReset(MADC_TypeDef *MADCx)
{
    MODIFY_REG(MADCx->ANA_RST, MADC_ANA_RST_ANA_RST, MADC_ANA_RST_ANA_RST_RST);
}

/**
 * @}
 */ /* end of group MADC_LL_EF_Reset */

/** @defgroup MADC_LL_EF_IT_Management IT Management functions
 * @{
 */

/**
 * @brief  Enable SAR ADC interrupt
 * @rmtoll AD_CTRL         SAR_IE            LL_MADC_EnableIT_SAR
 * @param  MADCx MADC Instance
 * @retval None
 */
__STATIC_INLINE void LL_MADC_EnableIT_SAR(MADC_TypeDef *MADCx)
{
    SET_BIT(MADCx->AD_CTRL, MADC_AD_CTRL_SAR_IE);
}

/**
 * @brief  Disable SAR ADC interrupt
 * @rmtoll AD_CTRL         SAR_IE            LL_MADC_DisableIT_SAR
 * @param  MADCx MADC Instance
 * @retval None
 */
__STATIC_INLINE void LL_MADC_DisableIT_SAR(MADC_TypeDef *MADCx)
{
    CLEAR_BIT(MADCx->AD_CTRL, MADC_AD_CTRL_SAR_IE);
}

/**
 * @brief  Indicates whether the SAR ADC interrupt is enabled
 * @rmtoll AD_CTRL         SAR_IE            LL_MADC_IsEnabledIT_SAR
 * @param  MADCx MADC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_MADC_IsEnabledIT_SAR(MADC_TypeDef *MADCx)
{
    return (uint32_t)((READ_BIT(MADCx->AD_CTRL, MADC_AD_CTRL_SAR_IE) == (MADC_AD_CTRL_SAR_IE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable BOR reset interrupt
 * @rmtoll ANA_RST_STAT         BORIE            LL_MADC_EnableIT_BOR
 * @param  MADCx MADC Instance
 * @retval None
 */
__STATIC_INLINE void LL_MADC_EnableIT_BOR(MADC_TypeDef *MADCx)
{
    SET_BIT(MADCx->ANA_RST_STAT, MADC_ANA_RST_STAT_BORIE);
}

/**
 * @brief  Disable BOR reset interrupt
 * @rmtoll ANA_RST_STAT         BORIE            LL_MADC_DisableIT_BOR
 * @param  MADCx MADC Instance
 * @retval None
 */
__STATIC_INLINE void LL_MADC_DisableIT_BOR(MADC_TypeDef *MADCx)
{
    CLEAR_BIT(MADCx->ANA_RST_STAT, MADC_ANA_RST_STAT_BORIE);
}

/**
 * @brief  Indicates whether the BOR reset interrupt is enabled
 * @rmtoll ANA_RST_STAT         BORIE            LL_MADC_IsEnabledIT_BOR
 * @param  MADCx MADC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_MADC_IsEnabledIT_BOR(MADC_TypeDef *MADCx)
{
    return (uint32_t)((READ_BIT(MADCx->ANA_RST_STAT, MADC_ANA_RST_STAT_BORIE) == (MADC_ANA_RST_STAT_BORIE)) ? 1UL : 0UL);
}

/**
 * @}
 */ /* end of group MADC_LL_EF_IT_Management */

/** @defgroup MADC_LL_EF_FLAG_Management Flag Management functions
 * @{
 */

/**
 * @brief  Indicate whether POR reset interrupt flag is active
 * @note   POR reset interrupt is a non-maskable interrupt
 * @rmtoll ANA_RST_STAT         PORIF            LL_MADC_IsActiveFlag_POR
 * @param  MADCx MADC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_MADC_IsActiveFlag_POR(MADC_TypeDef *MADCx)
{
    return (uint32_t)((READ_BIT(MADCx->ANA_RST_STAT, MADC_ANA_RST_STAT_PORIF) == (MADC_ANA_RST_STAT_PORIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the POR reset interrupt flag
 * @rmtoll ANA_RST_STAT         PORIF            LL_MADC_ClearFlag_POR
 * @param  MADCx MADC Instance
 * @retval None
 */
__STATIC_INLINE void LL_MADC_ClearFlag_POR(MADC_TypeDef *MADCx)
{
    WRITE_REG(MADCx->ANA_RST_STAT, MADC_ANA_RST_STAT_PORIF);
}

/**
 * @brief  Indicate whether BOR reset interrupt flag is active
 * @rmtoll ANA_RST_STAT         BORIF            LL_MADC_IsActiveFlag_BOR
 * @param  MADCx MADC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_MADC_IsActiveFlag_BOR(MADC_TypeDef *MADCx)
{
    return (uint32_t)((READ_BIT(MADCx->ANA_RST_STAT, MADC_ANA_RST_STAT_BORIF) == (MADC_ANA_RST_STAT_BORIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the BOR reset interrupt flag
 * @rmtoll ANA_RST_STAT         BORIF            LL_MADC_ClearFlag_BOR
 * @param  MADCx MADC Instance
 * @retval None
 */
__STATIC_INLINE void LL_MADC_ClearFlag_BOR(MADC_TypeDef *MADCx)
{
    WRITE_REG(MADCx->ANA_RST_STAT, MADC_ANA_RST_STAT_BORIF);
}

/**
 * @brief  Indicate whether temperature measurement is busy
 * @note   ADC temperature measurement busy flag and sar adc measure busy flag, read only
 * @rmtoll AD_STAT         TPS_BUSY            LL_MADC_IsActiveFlag_TPSBUSY
 * @param  MADCx MADC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_MADC_IsActiveFlag_TPSBUSY(MADC_TypeDef *MADCx)
{
    return (uint32_t)((READ_BIT(MADCx->AD_STAT, MADC_AD_STAT_TPS_BUSY) == (MADC_AD_STAT_TPS_BUSY)) ? 1UL : 0UL);
}

/**
 * @brief  Indicate whether SAR ADC data is ready
 * @note   Automatic temperature measurement has higher priority
 * @note   DREADY is set when the slowest of the three data registers is complete
 * @rmtoll AD_STAT         DREADY            LL_MADC_IsActiveFlag_DREADY
 * @param  MADCx MADC Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_MADC_IsActiveFlag_DREADY(MADC_TypeDef *MADCx)
{
    return (uint32_t)((READ_BIT(MADCx->AD_STAT, MADC_AD_STAT_DREADY) == (MADC_AD_STAT_DREADY)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the SAR ADC data ready flag
 * @rmtoll AD_STAT         DREADY            LL_MADC_ClearFlag_DREADY
 * @param  MADCx MADC Instance
 * @retval None
 */
__STATIC_INLINE void LL_MADC_ClearFlag_DREADY(MADC_TypeDef *MADCx)
{
    WRITE_REG(MADCx->AD_STAT, MADC_AD_STAT_DREADY);
}

/**
 * @}
 */ /* end of group MADC_LL_EF_FLAG_Management */
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup MADC_LL_EF_Init Initialization and de-initialization functions
 * @{
 */
ErrorStatus LL_MADC_DeInit(MADC_TypeDef *MADCx);
ErrorStatus LL_MADC_Init(MADC_TypeDef *MADCx, LL_MADC_InitTypeDef *MADC_InitStruct);
void        LL_MADC_StructInit(LL_MADC_InitTypeDef *MADC_InitStruct);
/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/


/**
 * @}
 */ /* end of group MADC_LL_Exported_Functions */

/**
 * @}
 */ /* end of group MADC_LL */

/**
 * @}
 */ /* end of group RN_LL_Driver */

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_LL_MADC_H */
