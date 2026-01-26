/**
 ******************************************************************************
 * @file   rn_rv_ll_lvdcmp.h
 * @author Renergy Software Team
 * @brief  Header file of LVDCMP LL module driver.
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
#ifndef __RN_RV_LL_LVDCMP_H
#define __RN_RV_LL_LVDCMP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu.h"
#include "rn831x_rn861x_mcu_lvdcmp.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

/** @defgroup LVDCMP_LL LVDCMP
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/** @defgroup LVDCMP_LL_ES_INIT LVDCMP Exported Init structure
 * @{
 */
/**
 * @brief LVD Initialization structure definition.
 */
typedef struct
{
    uint32_t LVDVoltageThreshold;        /*!< Specifies the lvd voltagethreshold value.
                                                This parameter can be a value of @ref LVDCMP_LL_EC_LVDVOLTAGETHRESHOLD.

                                                This feature can be modified afterwards using unitary function @ref LL_LVDCMP_SetLVDVoltageThreshold().*/
    uint32_t LVDFilter;                 /*!< Specifies the lvd filter value.
                                                This parameter must be a number between Min_Data=0x0 and Max_Data=255.

                                                This feature can be modified afterwards using unitary function @ref LL_LVDCMP_SetLVDFilter().*/
} LL_LVDInit_TypeDef;

/**
 * @brief CMP1 Initialization structure definition.
 */
typedef struct
{
    uint32_t CMP1ResistorMode;        /*!< Specifies the cmp1 resistor mode value.
                                       This parameter can be a value of @ref LVDCMP_LL_EC_CMP1RESISTORMODE.
                                       This feature can be modified afterwards using unitary function @ref LL_LVDCMP_SetCMP1ResistorMode().*/
    uint32_t CMP1Hysteresis;          /*!< Specifies the cmp1 hysteresis .
                                        This parameter can be a value of @ref LVDCMP_LL_EC_FUNCTION_MODE.
                                        Set to LL_LVDCMP_FUNCTION_DISABLE or 0, initial function will call @ref LL_LVDCMP_DisableCMP1Hysteresis()
                                        Set to LL_LVDCMP_FUNCTION_ENABLE  or 1, initial function will call @ref LL_LVDCMP_EnableCMP1Hysteresis() */
    uint32_t CMP1Filter;              /*!< Specifies the CMP1 filter value.
                                                This parameter must be a number between Min_Data=0x0 and Max_Data=255.

                                                This feature can be modified afterwards using unitary function @ref LL_LVDCMP_SetCMP1Filter().*/
} LL_CMP1Init_TypeDef;
/**
 * @brief CMP2 Initialization structure definition.
 */
typedef struct
{
    uint32_t CMP2ResistorMode;        /*!< Specifies the cmp2 resistor mode value.
                                       This parameter can be a value of @ref LVDCMP_LL_EC_CMP2RESISTORMODE.
                                       This feature can be modified afterwards using unitary function @ref LL_LVDCMP_SetCMP2ResistorMode().*/
    uint32_t CMP2Hysteresis;          /*!< Specifies the cmp2 hysteresis .
                                        This parameter can be a value of @ref LVDCMP_LL_EC_FUNCTION_MODE.
                                        Set to LL_LVDCMP_FUNCTION_DISABLE or 0, initial function will call @ref LL_LVDCMP_DisableCMP2Hysteresis()
                                        Set to LL_LVDCMP_FUNCTION_ENABLE  or 1, initial function will call @ref LL_LVDCMP_EnableCMP2Hysteresis() */
    uint32_t CMP2Filter;              /*!< Specifies the CMP2 filter value.
                                                This parameter must be a number between Min_Data=0x0 and Max_Data=255.

                                                This feature can be modified afterwards using unitary function @ref LL_LVDCMP_SetCMP2Filter().*/
} LL_CMP2Init_TypeDef;
/**
 * @}
 */ /* end of group LVDCMP_LL_ES_INIT */

/* Exported constants -------------------------------------------------------*/
/** @defgroup LVDCMP_LL_Exported_Constants LVDCMP Exported Constants
 * @{
 */
/** @defgroup LVDCMP_LL_EC_FUNCTION_MODE    Function mode
 * @{
 */
#define LL_LVDCMP_FUNCTION_DISABLE   (0U)
#define LL_LVDCMP_FUNCTION_ENABLE    (1U)
/**
 * @}
 */ /* end of group LVDCMP_LL_EC_FUNCTION_MODE */
/** @defgroup LVDCMP_LL_EC_LVDVoltageThreshold LVD Voltage Threshold
 * @{
 */
#define LL_LVDCMP_LVDTHRESHOLD_1P9                  LVDCMP_LVDCTRL_LVDS_1P9      /*!< LVD threshold 1.9V/2.0V */
#define LL_LVDCMP_LVDTHRESHOLD_2P1                  LVDCMP_LVDCTRL_LVDS_2P1      /*!< LVD threshold 2.1V/2.2V */
#define LL_LVDCMP_LVDTHRESHOLD_2P3                  LVDCMP_LVDCTRL_LVDS_2P3      /*!< LVD threshold 2.3V/2.4V */
#define LL_LVDCMP_LVDTHRESHOLD_2P5                  LVDCMP_LVDCTRL_LVDS_2P5      /*!< LVD threshold 2.5V/2.6V */
#define LL_LVDCMP_LVDTHRESHOLD_2P7                  LVDCMP_LVDCTRL_LVDS_2P7      /*!< LVD threshold 2.7V/2.8V */
#define LL_LVDCMP_LVDTHRESHOLD_2P9                  LVDCMP_LVDCTRL_LVDS_2P9      /*!< LVD threshold 2.9V/3.0V */
#define LL_LVDCMP_LVDTHRESHOLD_3P1                  LVDCMP_LVDCTRL_LVDS_3P1      /*!< LVD threshold 3.1V/3.2V */
#define LL_LVDCMP_LVDTHRESHOLD_3P3                  LVDCMP_LVDCTRL_LVDS_3P3      /*!< LVD threshold 3.3V/3.4V */
#define LL_LVDCMP_LVDTHRESHOLD_3P5                  LVDCMP_LVDCTRL_LVDS_3P5      /*!< LVD threshold 3.5V/3.6V */
#define LL_LVDCMP_LVDTHRESHOLD_3P7                  LVDCMP_LVDCTRL_LVDS_3P7      /*!< LVD threshold 3.7V/3.8V */
#define LL_LVDCMP_LVDTHRESHOLD_3P9                  LVDCMP_LVDCTRL_LVDS_3P9      /*!< LVD threshold 3.9V/4.0V */
#define LL_LVDCMP_LVDTHRESHOLD_4P1                  LVDCMP_LVDCTRL_LVDS_4P1      /*!< LVD threshold 4.1V/4.2V */
#define LL_LVDCMP_LVDTHRESHOLD_4P3                  LVDCMP_LVDCTRL_LVDS_4P3      /*!< LVD threshold 4.3V/4.4V */
#define LL_LVDCMP_LVDTHRESHOLD_4P5                  LVDCMP_LVDCTRL_LVDS_4P5      /*!< LVD threshold 4.5V/4.6V */
#define LL_LVDCMP_LVDTHRESHOLD_4P7                  LVDCMP_LVDCTRL_LVDS_4P7      /*!< LVD threshold 4.7V/4.8V */
#define LL_LVDCMP_LVDTHRESHOLD_LVDIN                LVDCMP_LVDCTRL_LVDS_LVDIN    /*!< LVD threshold LVDIN pin */
/**
 * @}
 */

/** @defgroup LVDCMP_LL_EC_CMP1ResistorMode CMP1 Resistor Mode
 * @{
 */
#define LL_LVDCMP_CMP1_RESISTOR_ON                  0x00000000U                  /*!< CMP1 internal 600K resistor on */
#define LL_LVDCMP_CMP1_RESISTOR_OFF                 LVDCMP_CMPCTRL_MODE_CMP1R    /*!< CMP1 internal 600K resistor off */
/**
 * @}
 */

/** @defgroup LVDCMP_LL_EC_CMP2ResistorMode CMP2 Resistor Mode
 * @{
 */
#define LL_LVDCMP_CMP2_RESISTOR_ON                  0x00000000U                  /*!< CMP2 internal 600K resistor on */
#define LL_LVDCMP_CMP2_RESISTOR_OFF                 LVDCMP_CMPCTRL_MODE_CMP2R    /*!< CMP2 internal 600K resistor off */
/**
 * @}
 */

/**
 * @}
 */ /* end of group LVDCMP_LL_Exported_Constants */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup LVDCMP_LL_Exported_Macros LVDCMP Exported Macros
 * @{
 */
/** @defgroup LVDCMP_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in LVDCMP register
 * @param  __INSTANCE__ LVDCMP Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_LVDCMP_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in LVDCMP register
 * @param  __INSTANCE__ LVDCMP Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_LVDCMP_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)

/**
 * @}
 */
/**
 * @}
 */ /* end of group LVDCMP_LL_Exported_Macros */

/* Exported functions -------------------------------------------------------*/
/** @defgroup LVDCMP_LL_Exported_Functions LVDCMP Exported Functions
 * @{
 */


/** @defgroup LVDCMP_LL_EF_LVD_Configuration LVD Configuration functions
 * @{
 */

/**
 * @brief  Set the LVD threshold voltage
 * @rmtoll LVDCTRL         LVDS            LL_LVDCMP_SetLVDVoltageThreshold
 * @param  LVDCMPx LVDCMP Instance
 * @param  VoltageThreshold This parameter can be one of the following values:
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_1P9
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_2P1
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_2P3
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_2P5
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_2P7
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_2P9
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_3P1
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_3P3
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_3P5
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_3P7
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_3P9
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_4P1
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_4P3
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_4P5
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_4P7
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_LVDIN
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_SetLVDVoltageThreshold(LVDCMP_TypeDef *LVDCMPx, uint32_t VoltageThreshold)
{
    MODIFY_REG(LVDCMPx->LVDCTRL, LVDCMP_LVDCTRL_LVDS, VoltageThreshold);
}

/**
 * @brief  Get the LVD threshold voltage
 * @rmtoll LVDCTRL         LVDS            LL_LVDCMP_GetLVDVoltageThreshold
 * @param  LVDCMPx LVDCMP Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_1P9
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_2P1
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_2P3
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_2P5
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_2P7
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_2P9
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_3P1
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_3P3
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_3P5
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_3P7
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_3P9
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_4P1
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_4P3
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_4P5
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_4P7
 *         @arg @ref LL_LVDCMP_LVDTHRESHOLD_LVDIN
 */
__STATIC_INLINE uint32_t LL_LVDCMP_GetLVDVoltageThreshold(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)(READ_BIT(LVDCMPx->LVDCTRL, LVDCMP_LVDCTRL_LVDS));
}

/**
 * @brief  Set the LVD filter configuration
 * @note   The filter can be configured from 0 to 255 clock cycles. Setting to 0 disables the filter
 * @rmtoll LVDCTRL         LVD_FLT_CNT            LL_LVDCMP_SetLVDFilter
 * @param  LVDCMPx LVDCMP Instance
 * @param  FilterCount Between 0 to 255
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_SetLVDFilter(LVDCMP_TypeDef *LVDCMPx, uint32_t FilterCount)
{
    MODIFY_REG(LVDCMPx->LVDCTRL, LVDCMP_LVDCTRL_LVD_FLT_CNT, (FilterCount << LVDCMP_LVDCTRL_LVD_FLT_CNT_POS));
}

/**
 * @brief  Get the LVD filter configuration
 * @rmtoll LVDCTRL         LVD_FLT_CNT            LL_LVDCMP_GetLVDFilter
 * @param  LVDCMPx LVDCMP Instance
 * @retval LVD filter count (0 to 255)
 */
__STATIC_INLINE uint32_t LL_LVDCMP_GetLVDFilter(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)(READ_BIT(LVDCMPx->LVDCTRL, LVDCMP_LVDCTRL_LVD_FLT_CNT) >> LVDCMP_LVDCTRL_LVD_FLT_CNT_POS);
}

/**
 * @}
 */ /* end of group LVDCMP_LL_EF_LVD_Configuration */

/** @defgroup LVDCMP_LL_EF_CMP_Configuration CMP Configuration functions
 * @{
 */

/**
 * @brief  Set the CMP1 internal resistor mode
 * @note   CMP1 internal 600K resistor sampling mode switch
 *         LL_LVDCMP_CMP1_RESISTOR_ON   must call @ref LL_LVDCMP_DisableCMP1Hysteresis
 *         LL_LVDCMP_CMP1_RESISTOR_OFF  must call @ref LL_LVDCMP_EnableCMP1Hysteresis
 * @rmtoll CMPCTRL         MODE_CMP1R            LL_LVDCMP_SetCMP1ResistorMode
 * @param  LVDCMPx LVDCMP Instance
 * @param  ResistorMode This parameter can be one of the following values:
 *         @arg @ref LL_LVDCMP_CMP1_RESISTOR_ON
 *         @arg @ref LL_LVDCMP_CMP1_RESISTOR_OFF
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_SetCMP1ResistorMode(LVDCMP_TypeDef *LVDCMPx, uint32_t ResistorMode)
{
    MODIFY_REG(LVDCMPx->CMPCTRL, LVDCMP_CMPCTRL_MODE_CMP1R, ResistorMode);
}

/**
 * @brief  Get the CMP1 internal resistor mode
 * @rmtoll CMPCTRL         MODE_CMP1R            LL_LVDCMP_GetCMP1ResistorMode
 * @param  LVDCMPx LVDCMP Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LVDCMP_CMP1_RESISTOR_ON
 *         @arg @ref LL_LVDCMP_CMP1_RESISTOR_OFF
 */
__STATIC_INLINE uint32_t LL_LVDCMP_GetCMP1ResistorMode(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)(READ_BIT(LVDCMPx->CMPCTRL, LVDCMP_CMPCTRL_MODE_CMP1R));
}

/**
 * @brief  Set the CMP2 internal resistor mode
 * @note   CMP2 internal 600K resistor sampling mode switch
 *         LL_LVDCMP_CMP2_RESISTOR_ON   must call @ref LL_LVDCMP_DisableCMP2Hysteresis
 *         LL_LVDCMP_CMP2_RESISTOR_OFF  must call @ref LL_LVDCMP_EnableCMP2Hysteresis
 * @rmtoll CMPCTRL         MODE_CMP2R            LL_LVDCMP_SetCMP2ResistorMode
 * @param  LVDCMPx LVDCMP Instance
 * @param  ResistorMode This parameter can be one of the following values:
 *         @arg @ref LL_LVDCMP_CMP2_RESISTOR_ON
 *         @arg @ref LL_LVDCMP_CMP2_RESISTOR_OFF
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_SetCMP2ResistorMode(LVDCMP_TypeDef *LVDCMPx, uint32_t ResistorMode)
{
    MODIFY_REG(LVDCMPx->CMPCTRL, LVDCMP_CMPCTRL_MODE_CMP2R, ResistorMode);
}

/**
 * @brief  Get the CMP2 internal resistor mode
 * @rmtoll CMPCTRL         MODE_CMP2R            LL_LVDCMP_GetCMP2ResistorMode
 * @param  LVDCMPx LVDCMP Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LVDCMP_CMP2_RESISTOR_ON
 *         @arg @ref LL_LVDCMP_CMP2_RESISTOR_OFF
 */
__STATIC_INLINE uint32_t LL_LVDCMP_GetCMP2ResistorMode(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)(READ_BIT(LVDCMPx->CMPCTRL, LVDCMP_CMPCTRL_MODE_CMP2R));
}

/**
 * @brief  Enable CMP1 hysteresis
 * @rmtoll CMPCTRL         HYSEN_CMP1            LL_LVDCMP_EnableHysteresis
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_EnableCMP1Hysteresis(LVDCMP_TypeDef *LVDCMPx)
{
    SET_BIT(LVDCMPx->CMPCTRL, LVDCMP_CMPCTRL_HYSEN_CMP1);
}

/**
 * @brief  Disable CMP1 hysteresis
 * @rmtoll CMPCTRL         HYSEN_CMP1            LL_LVDCMP_DisableHysteresis
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_DisableCMP1Hysteresis(LVDCMP_TypeDef *LVDCMPx)
{
    CLEAR_BIT(LVDCMPx->CMPCTRL, LVDCMP_CMPCTRL_HYSEN_CMP1);
}

/**
 * @brief  Indicates whether the CMP1 hysteresis is enabled
 * @rmtoll CMPCTRL         HYSEN_CMP1            LL_LVDCMP_IsEnabledHysteresis
 * @param  LVDCMPx LVDCMP Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_LVDCMP_IsEnabledCMP1Hysteresis(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)((READ_BIT(LVDCMPx->CMPCTRL, LVDCMP_CMPCTRL_HYSEN_CMP1) == (LVDCMP_CMPCTRL_HYSEN_CMP1)) ? 1UL : 0UL);
}
/**
 * @brief  Enable CMP2 hysteresis
 * @rmtoll CMPCTRL         HYSEN_CMP2            LL_LVDCMP_EnableHysteresis
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_EnableCMP2Hysteresis(LVDCMP_TypeDef *LVDCMPx)
{
    SET_BIT(LVDCMPx->CMPCTRL, LVDCMP_CMPCTRL_HYSEN_CMP2);
}

/**
 * @brief  Disable CMP2 hysteresis
 * @rmtoll CMPCTRL         HYSEN_CMP2            LL_LVDCMP_DisableHysteresis
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_DisableCMP2Hysteresis(LVDCMP_TypeDef *LVDCMPx)
{
    CLEAR_BIT(LVDCMPx->CMPCTRL, LVDCMP_CMPCTRL_HYSEN_CMP2);
}

/**
 * @brief  Indicates whether the CMP2 hysteresis is enabled
 * @rmtoll CMPCTRL         HYSEN_CMP2            LL_LVDCMP_IsEnabledHysteresis
 * @param  LVDCMPx LVDCMP Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_LVDCMP_IsEnabledCMP2Hysteresis(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)((READ_BIT(LVDCMPx->CMPCTRL, LVDCMP_CMPCTRL_HYSEN_CMP2) == (LVDCMP_CMPCTRL_HYSEN_CMP2)) ? 1UL : 0UL);
}

/**
 * @brief  Set the CMP1 filter configuration
 * @note   The filter can be configured from 0 to 255 clock cycles. Setting to 0 disables the filter
 * @rmtoll CMPCTRL         CMP1_FLT_CNT            LL_LVDCMP_SetCMP1Filter
 * @param  LVDCMPx LVDCMP Instance
 * @param  FilterCount Between 0 to 255
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_SetCMP1Filter(LVDCMP_TypeDef *LVDCMPx, uint32_t FilterCount)
{
    MODIFY_REG(LVDCMPx->CMPCTRL, LVDCMP_CMPCTRL_CMP1_FLT_CNT, (FilterCount << LVDCMP_CMPCTRL_CMP1_FLT_CNT_POS));
}

/**
 * @brief  Get the CMP1 filter configuration
 * @rmtoll CMPCTRL         CMP1_FLT_CNT            LL_LVDCMP_GetCMP1Filter
 * @param  LVDCMPx LVDCMP Instance
 * @retval CMP1 filter count (0 to 255)
 */
__STATIC_INLINE uint32_t LL_LVDCMP_GetCMP1Filter(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)(READ_BIT(LVDCMPx->CMPCTRL, LVDCMP_CMPCTRL_CMP1_FLT_CNT) >> LVDCMP_CMPCTRL_CMP1_FLT_CNT_POS);
}

/**
 * @brief  Set the CMP2 filter configuration
 * @note   The filter can be configured from 0 to 255 clock cycles. Setting to 0 disables the filter
 * @rmtoll CMPCTRL         CMP2_FLT_CNT            LL_LVDCMP_SetCMP2Filter
 * @param  LVDCMPx LVDCMP Instance
 * @param  FilterCount Between 0 to 255
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_SetCMP2Filter(LVDCMP_TypeDef *LVDCMPx, uint32_t FilterCount)
{
    MODIFY_REG(LVDCMPx->CMPCTRL, LVDCMP_CMPCTRL_CMP2_FLT_CNT, (FilterCount << LVDCMP_CMPCTRL_CMP2_FLT_CNT_POS));
}

/**
 * @brief  Get the CMP2 filter configuration
 * @rmtoll CMPCTRL         CMP2_FLT_CNT            LL_LVDCMP_GetCMP2Filter
 * @param  LVDCMPx LVDCMP Instance
 * @retval CMP2 filter count (0 to 255)
 */
__STATIC_INLINE uint32_t LL_LVDCMP_GetCMP2Filter(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)(READ_BIT(LVDCMPx->CMPCTRL, LVDCMP_CMPCTRL_CMP2_FLT_CNT) >> LVDCMP_CMPCTRL_CMP2_FLT_CNT_POS);
}

/**
 * @}
 */ /* end of group LVDCMP_LL_EF_CMP_Configuration */

/** @defgroup LVDCMP_LL_EF_IT_Management IT Management functions
 * @{
 */

/**
 * @brief  Enable CMP1 interrupt
 * @rmtoll IE         CMP1IE            LL_LVDCMP_EnableIT_CMP1
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_EnableIT_CMP1(LVDCMP_TypeDef *LVDCMPx)
{
    SET_BIT(LVDCMPx->IE, LVDCMP_IE_CMP1IE);
}

/**
 * @brief  Disable CMP1 interrupt
 * @rmtoll IE         CMP1IE            LL_LVDCMP_DisableIT_CMP1
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_DisableIT_CMP1(LVDCMP_TypeDef *LVDCMPx)
{
    CLEAR_BIT(LVDCMPx->IE, LVDCMP_IE_CMP1IE);
}

/**
 * @brief  Indicates whether the CMP1 interrupt is enabled
 * @rmtoll IE         CMP1IE            LL_LVDCMP_IsEnabledIT_CMP1
 * @param  LVDCMPx LVDCMP Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_LVDCMP_IsEnabledIT_CMP1(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)((READ_BIT(LVDCMPx->IE, LVDCMP_IE_CMP1IE) == (LVDCMP_IE_CMP1IE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable CMP2 interrupt
 * @rmtoll IE         CMP2IE            LL_LVDCMP_EnableIT_CMP2
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_EnableIT_CMP2(LVDCMP_TypeDef *LVDCMPx)
{
    SET_BIT(LVDCMPx->IE, LVDCMP_IE_CMP2IE);
}

/**
 * @brief  Disable CMP2 interrupt
 * @rmtoll IE         CMP2IE            LL_LVDCMP_DisableIT_CMP2
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_DisableIT_CMP2(LVDCMP_TypeDef *LVDCMPx)
{
    CLEAR_BIT(LVDCMPx->IE, LVDCMP_IE_CMP2IE);
}

/**
 * @brief  Indicates whether the CMP2 interrupt is enabled
 * @rmtoll IE         CMP2IE            LL_LVDCMP_IsEnabledIT_CMP2
 * @param  LVDCMPx LVDCMP Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_LVDCMP_IsEnabledIT_CMP2(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)((READ_BIT(LVDCMPx->IE, LVDCMP_IE_CMP2IE) == (LVDCMP_IE_CMP2IE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable LVD interrupt
 * @rmtoll IE         LVDIE            LL_LVDCMP_EnableIT_LVD
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_EnableIT_LVD(LVDCMP_TypeDef *LVDCMPx)
{
    SET_BIT(LVDCMPx->IE, LVDCMP_IE_LVDIE);
}

/**
 * @brief  Disable LVD interrupt
 * @rmtoll IE         LVDIE            LL_LVDCMP_DisableIT_LVD
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_DisableIT_LVD(LVDCMP_TypeDef *LVDCMPx)
{
    CLEAR_BIT(LVDCMPx->IE, LVDCMP_IE_LVDIE);
}

/**
 * @brief  Indicates whether the LVD interrupt is enabled
 * @rmtoll IE         LVDIE            LL_LVDCMP_IsEnabledIT_LVD
 * @param  LVDCMPx LVDCMP Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_LVDCMP_IsEnabledIT_LVD(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)((READ_BIT(LVDCMPx->IE, LVDCMP_IE_LVDIE) == (LVDCMP_IE_LVDIE)) ? 1UL : 0UL);
}

/**
 * @}
 */ /* end of group LVDCMP_LL_EF_IT_Management */

/** @defgroup LVDCMP_LL_EF_FLAG_Management Flag Management functions
 * @{
 */

/**
 * @brief  Indicate whether CMP1 interrupt flag is active
 * @rmtoll IF         CMP1IF            LL_LVDCMP_IsActiveFlag_CMP1
 * @param  LVDCMPx LVDCMP Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_LVDCMP_IsActiveFlag_CMP1(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)((READ_BIT(LVDCMPx->IF, LVDCMP_IF_CMP1IF) == (LVDCMP_IF_CMP1IF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the CMP1 interrupt flag
 * @rmtoll IF         CMP1IF            LL_LVDCMP_ClearFlag_CMP1
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_ClearFlag_CMP1(LVDCMP_TypeDef *LVDCMPx)
{
    WRITE_REG(LVDCMPx->IF, LVDCMP_IF_CMP1IF);
}

/**
 * @brief  Indicate whether CMP2 interrupt flag is active
 * @rmtoll IF         CMP2IF            LL_LVDCMP_IsActiveFlag_CMP2
 * @param  LVDCMPx LVDCMP Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_LVDCMP_IsActiveFlag_CMP2(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)((READ_BIT(LVDCMPx->IF, LVDCMP_IF_CMP2IF) == (LVDCMP_IF_CMP2IF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the CMP2 interrupt flag
 * @rmtoll IF         CMP2IF            LL_LVDCMP_ClearFlag_CMP2
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_ClearFlag_CMP2(LVDCMP_TypeDef *LVDCMPx)
{
    WRITE_REG(LVDCMPx->IF, LVDCMP_IF_CMP2IF);
}

/**
 * @brief  Indicate whether LVD interrupt flag is active
 * @rmtoll IF         LVDIF            LL_LVDCMP_IsActiveFlag_LVD
 * @param  LVDCMPx LVDCMP Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_LVDCMP_IsActiveFlag_LVD(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)((READ_BIT(LVDCMPx->IF, LVDCMP_IF_LVDIF) == (LVDCMP_IF_LVDIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the LVD interrupt flag
 * @rmtoll IF         LVDIF            LL_LVDCMP_ClearFlag_LVD
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_ClearFlag_LVD(LVDCMP_TypeDef *LVDCMPx)
{
    WRITE_REG(LVDCMPx->IF, LVDCMP_IF_LVDIF);
}

/**
 * @}
 */ /* end of group LVDCMP_LL_EF_FLAG_Management */

/** @defgroup LVDCMP_LL_EF_PowerManagement Power Management functions
 * @{
 */

/**
 * @brief  Enable CMP1 power
 * @rmtoll CMP1PWD         CMP1PWD            LL_LVDCMP_EnableCMP1
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_EnableCMP1(LVDCMP_TypeDef *LVDCMPx)
{
    WRITE_REG(LVDCMPx->CMP1PWD, LVDCMP_CMP1PWD_CMP1PWD_EN);
}

/**
 * @brief  Disable CMP1 power (power down)
 * @rmtoll CMP1PWD         CMP1PWD            LL_LVDCMP_DisableCMP1
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_DisableCMP1(LVDCMP_TypeDef *LVDCMPx)
{
    WRITE_REG(LVDCMPx->CMP1PWD, LVDCMP_CMP1PWD_CMP1PWD_PWDN);
}

/**
 * @brief  Indicates whether the CMP1 is enabled
 * @rmtoll CMP1PWD         CMP1PWD            LL_LVDCMP_IsEnabledCMP1
 * @param  LVDCMPx LVDCMP Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_LVDCMP_IsEnabledCMP1(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)((READ_REG(LVDCMPx->CMP1PWD) == LVDCMP_CMP1PWD_CMP1PWD_EN) ? 1UL : 0UL);
}

/**
 * @brief  Enable CMP2 power
 * @rmtoll CMP2PWD         CMP2PWD            LL_LVDCMP_EnableCMP2
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_EnableCMP2(LVDCMP_TypeDef *LVDCMPx)
{
    WRITE_REG(LVDCMPx->CMP2PWD, LVDCMP_CMP2PWD_CMP2PWD_EN);
}

/**
 * @brief  Disable CMP2 power (power down)
 * @rmtoll CMP2PWD         CMP2PWD            LL_LVDCMP_DisableCMP2
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_DisableCMP2(LVDCMP_TypeDef *LVDCMPx)
{
    WRITE_REG(LVDCMPx->CMP2PWD, LVDCMP_CMP2PWD_CMP2PWD_PWDN);
}

/**
 * @brief  Indicates whether the CMP2 is enabled
 * @rmtoll CMP2PWD         CMP2PWD            LL_LVDCMP_IsEnabledCMP2
 * @param  LVDCMPx LVDCMP Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_LVDCMP_IsEnabledCMP2(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)((READ_REG(LVDCMPx->CMP2PWD) == LVDCMP_CMP2PWD_CMP2PWD_EN) ? 1UL : 0UL);
}

/**
 * @brief  Enable LVD power
 * @rmtoll LVDPWD         LVDPWD            LL_LVDCMP_EnableLVD
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_EnableLVD(LVDCMP_TypeDef *LVDCMPx)
{
    WRITE_REG(LVDCMPx->LVDPWD, LVDCMP_LVDPWD_LVDPWD_EN);
}

/**
 * @brief  Disable LVD power (power down)
 * @rmtoll LVDPWD         LVDPWD            LL_LVDCMP_DisableLVD
 * @param  LVDCMPx LVDCMP Instance
 * @retval None
 */
__STATIC_INLINE void LL_LVDCMP_DisableLVD(LVDCMP_TypeDef *LVDCMPx)
{
    WRITE_REG(LVDCMPx->LVDPWD, LVDCMP_LVDPWD_LVDPWD_PWDN);
}

/**
 * @brief  Indicates whether the LVD is enabled
 * @rmtoll LVDPWD         LVDPWD            LL_LVDCMP_IsEnabledLVD
 * @param  LVDCMPx LVDCMP Instance
 * @retval State of bit (1 or 0)
 */
__STATIC_INLINE uint32_t LL_LVDCMP_IsEnabledLVD(LVDCMP_TypeDef *LVDCMPx)
{
    return (uint32_t)((READ_REG(LVDCMPx->LVDPWD) == LVDCMP_LVDPWD_LVDPWD_EN) ? 1UL : 0UL);
}

/**
 * @}
 */ /* end of group LVDCMP_LL_EF_PowerManagement */
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup LVDCMP_LL_EF_Init Initialization and de-initialization functions
 * @{
 */
ErrorStatus LL_LVDCMP_DeInit( LVDCMP_TypeDef *LVDCMPx);
ErrorStatus LL_LVDCMP_LVDInit(LVDCMP_TypeDef *LVDCMPx, LL_LVDInit_TypeDef *LVD_InitStruct);
ErrorStatus LL_LVDCMP_CMP1Init(LVDCMP_TypeDef *LVDCMPx, LL_CMP1Init_TypeDef *CMP1_InitStruct);
ErrorStatus LL_LVDCMP_CMP2Init(LVDCMP_TypeDef *LVDCMPx, LL_CMP2Init_TypeDef *CMP2_InitStruct);
void        LL_LVDCMP_LVDStructInit (LL_LVDInit_TypeDef *LVD_InitStruct);
void        LL_LVDCMP_CMP1StructInit(LL_CMP1Init_TypeDef *CMP1_InitStruct);
void        LL_LVDCMP_CMP2StructInit(LL_CMP2Init_TypeDef *CMP2_InitStruct);
/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */ /* end of group LVDCMP_LL_Exported_Functions */

/**
 * @}
 */ /* end of group LVDCMP_LL */

/**
 * @}
 */ /* end of group RN_LL_Driver */

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_LL_LVDCMP_H */
