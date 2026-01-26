/**
 *****************************************************************************
 * @file rn_rv_ll_d2f.h
 * @author Renergy Software Team
 * @brief Header file of D2F LL module.
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
#ifndef __RN_LL_D2F_H
#define __RN_LL_D2F_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_d2f.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined(D2F)

/** @defgroup D2F_LL D2F
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup D2F_LL_ES_INIT D2F Exported Init structure
 * @{
 */

/**
 * @brief LL D2F Init Structure definition
 */
typedef struct
{
    uint32_t ClockMode;         /*!< Specifies the integrator clock mode for the D2F.
                                     This parameter can be any value of @ref D2F_LL_EC_CLKMODE */

    uint32_t PowerLoadType;     /*!< Specifies the load type of input power for the integration.
                                     This parameter can be a value of @ref D2F_LL_EC_LOADTYPE. */

    uint32_t EnergyType;        /*!< Specifies the handle type of output energy.
                                     This parameter can be a value of @ref D2F_LL_EC_ENGTYPE. */

    uint32_t CF0Channel;        /*!< Specifies the output channel of CF0.
                                     This parameter can be a value of @ref D2F_LL_EC_CF0. */

    uint32_t CF1Channel;        /*!< Specifies the output channel of CF1.
                                     This parameter can be a value of @ref D2F_LL_EC_CF1. */

    uint32_t CF2Channel;        /*!< Specifies the output channel of CF2.
                                     This parameter can be a value of @ref D2F_LL_EC_CF2. */

} LL_D2F_InitTypeDef;
/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/* Exported constants -------------------------------------------------------*/
/** @defgroup D2F_LL_Exported_Constants D2F Exported Constants
 * @{
 */

/** @defgroup D2F_LL_EC_CLKMODE Integrator clock
 * @{
 */
#define LL_D2F_CLKMODE_DEFAULT      D2F_CFG_CLK_SEL_DEFAULT  /*!< Select default D2F clock as integrator clock */
#define LL_D2F_CLKMODE_CONSTANT     D2F_CFG_CLK_SEL_CONSTANT /*!< Select constant D2F clock as integrator clock */
/**
 * @}
 */

/** @defgroup D2F_LL_EC_LOADTYPE  load type
 * @{
 */
#define LL_D2F_LOADTYPE_NORMAL      D2F_CFG_LOADTYPE_NORMAL     /*!< Select Normal load type of power */
#define LL_D2F_LOADTYPE_SIMPTC      D2F_CFG_LOADTYPE_SYN_SIMPTC /*!< Select Synchronized loading type based on SimpTC */
/**
 * @}
 */

/** @defgroup D2F_LL_EC_ENGTYPE  energy type
 * @{
 */
#define LL_D2F_ENGTYPE_R2CLR        D2F_CFG_ENGTYPE_R2CLR   /*!< Select read to clear type */
#define LL_D2F_ENGTYPE_COUNTER      D2F_CFG_ENGTYPE_COUNTER /*!< Select counter after read type */
/**
 * @}
 */

/** @defgroup D2F_LL_EC_CF0    CF0 configuration
 * @{
 */
#define LL_D2F_CF0_D2F00            D2F_CF_CFG_CF0_SEL_D2F00 /*!< Select D2F00 channel as CF0 */
#define LL_D2F_CF0_D2F01            D2F_CF_CFG_CF0_SEL_D2F01 /*!< Select D2F01 channel as CF0 */
#define LL_D2F_CF0_D2F02            D2F_CF_CFG_CF0_SEL_D2F02 /*!< Select D2F02 channel as CF0 */
#define LL_D2F_CF0_D2F03            D2F_CF_CFG_CF0_SEL_D2F03 /*!< Select D2F03 channel as CF0 */
/**
 * @}
 */

/** @defgroup D2F_LL_EC_CF1    CF1 configuration
 * @{
 */
#define LL_D2F_CF1_D2F04            D2F_CF_CFG_CF1_SEL_D2F04 /*!< Select D2F04 channel as CF1 */
#define LL_D2F_CF1_D2F05            D2F_CF_CFG_CF1_SEL_D2F05 /*!< Select D2F05 channel as CF1 */
#define LL_D2F_CF1_D2F06            D2F_CF_CFG_CF1_SEL_D2F06 /*!< Select D2F06 channel as CF1 */
#define LL_D2F_CF1_D2F07            D2F_CF_CFG_CF1_SEL_D2F07 /*!< Select D2F07 channel as CF1 */
/**
 * @}
 */

/** @defgroup D2F_LL_EC_CF2    CF2 configuration
 * @{
 */
#define LL_D2F_CF2_D2F08            D2F_CF_CFG_CF2_SEL_D2F08 /*!< Select D2F08 channel as CF2 */
#define LL_D2F_CF2_D2F09            D2F_CF_CFG_CF2_SEL_D2F09 /*!< Select D2F09 channel as CF2 */
#define LL_D2F_CF2_D2F10            D2F_CF_CFG_CF2_SEL_D2F10 /*!< Select D2F10 channel as CF2 */
#define LL_D2F_CF2_D2F11            D2F_CF_CFG_CF2_SEL_D2F11 /*!< Select D2F11 channel as CF2 */
/**
 * @}
 */

/** @defgroup D2F_LL_EC_IE    D2F interrupt
 * @{
 */
#define LL_D2F_IE_PCOV              D2F_IE_PCOV_IE
#define LL_D2F_IE_D2F11             D2F_IE_D2F11_IE
#define LL_D2F_IE_D2F10             D2F_IE_D2F10_IE
#define LL_D2F_IE_D2F09             D2F_IE_D2F09_IE
#define LL_D2F_IE_D2F08             D2F_IE_D2F08_IE
#define LL_D2F_IE_D2F07             D2F_IE_D2F07_IE
#define LL_D2F_IE_D2F06             D2F_IE_D2F06_IE
#define LL_D2F_IE_D2F05             D2F_IE_D2F05_IE
#define LL_D2F_IE_D2F04             D2F_IE_D2F04_IE
#define LL_D2F_IE_D2F03             D2F_IE_D2F03_IE
#define LL_D2F_IE_D2F02             D2F_IE_D2F02_IE
#define LL_D2F_IE_D2F01             D2F_IE_D2F01_IE
#define LL_D2F_IE_D2F00             D2F_IE_D2F00_IE
/**
 * @}
 */

/** @defgroup D2F_LL_EC_NLD    No-Load
 * @{
 */
#define LL_D2F_NLD_D2F11            D2F_NOLOAD_D2F11_NLD
#define LL_D2F_NLD_D2F10            D2F_NOLOAD_D2F10_NLD
#define LL_D2F_NLD_D2F09            D2F_NOLOAD_D2F09_NLD
#define LL_D2F_NLD_D2F08            D2F_NOLOAD_D2F08_NLD
#define LL_D2F_NLD_D2F07            D2F_NOLOAD_D2F07_NLD
#define LL_D2F_NLD_D2F06            D2F_NOLOAD_D2F06_NLD
#define LL_D2F_NLD_D2F05            D2F_NOLOAD_D2F05_NLD
#define LL_D2F_NLD_D2F04            D2F_NOLOAD_D2F04_NLD
#define LL_D2F_NLD_D2F03            D2F_NOLOAD_D2F03_NLD
#define LL_D2F_NLD_D2F02            D2F_NOLOAD_D2F02_NLD
#define LL_D2F_NLD_D2F01            D2F_NOLOAD_D2F01_NLD
#define LL_D2F_NLD_D2F00            D2F_NOLOAD_D2F00_NLD
/**
 * @}
 */

/** @defgroup D2F_LL_EC_POWER    Power
 * @{
 */
#define LL_D2F_POWER_00            (0x00000000U)
#define LL_D2F_POWER_01            (0x00000001U)
#define LL_D2F_POWER_02            (0x00000002U)
#define LL_D2F_POWER_03            (0x00000003U)
#define LL_D2F_POWER_04            (0x00000004U)
#define LL_D2F_POWER_05            (0x00000005U)
#define LL_D2F_POWER_06            (0x00000006U)
#define LL_D2F_POWER_07            (0x00000007U)
#define LL_D2F_POWER_08            (0x00000008U)
#define LL_D2F_POWER_09            (0x00000009U)
#define LL_D2F_POWER_10            (0x0000000AU)
#define LL_D2F_POWER_11            (0x0000000BU)
/**
 * @}
 */

/** @defgroup D2F_LL_EC_ENERGY    Energy
 * @{
 */
#define LL_D2F_ENERGY_00            (0x00000000U)
#define LL_D2F_ENERGY_01            (0x00000001U)
#define LL_D2F_ENERGY_02            (0x00000002U)
#define LL_D2F_ENERGY_03            (0x00000003U)
#define LL_D2F_ENERGY_04            (0x00000004U)
#define LL_D2F_ENERGY_05            (0x00000005U)
#define LL_D2F_ENERGY_06            (0x00000006U)
#define LL_D2F_ENERGY_07            (0x00000007U)
#define LL_D2F_ENERGY_08            (0x00000008U)
#define LL_D2F_ENERGY_09            (0x00000009U)
#define LL_D2F_ENERGY_10            (0x0000000AU)
#define LL_D2F_ENERGY_11            (0x0000000BU)
/**
 * @}
 */

/** @defgroup D2F_LL_EC_TH    Start threshold
 * @{
 */
#define LL_D2F_TH_00                (0x00000000U)
#define LL_D2F_TH_01                (0x00000001U)
#define LL_D2F_TH_02                (0x00000002U)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup D2F_LL_Exported_Functions D2F Exported Functions
 * @{
 */

/** @defgroup D2F_LL_EF_Configuration Configuration
 * @{
 */
/**
 * @brief Configure HFConst0 for E00~E03 and CF0.
 * @note  HFConst is a 16 bit unsigned integer.
 * @rmtoll HFCONST0   HFCONST0   LL_D2F_SetHFConst0
 * @param D2Fx    D2F Instance
 * @param HFConst high frequency constant
 * @return None
 */
__STATIC_INLINE void LL_D2F_SetHFConst0(D2F_TypeDef *D2Fx, uint32_t HFConst)
{
    WRITE_REG(D2Fx->HFCONST0, (D2F_HFCONST0_HFCONST0 & HFConst));
}

/**
 * @brief Return HFConst0 of E00~E03 and CF0.
 * @note  HFConst is a 16 bit unsigned integer.
 * @rmtoll HFCONST0   HFCONST0   LL_D2F_GetHFConst0
 * @param D2Fx  D2F Instance
 * @return HFCONST0 value
 */
__STATIC_INLINE uint32_t LL_D2F_GetHFConst0(D2F_TypeDef *D2Fx)
{
    return (uint32_t)(READ_REG(D2Fx->HFCONST0));
}

/**
 * @brief Configure HFConst1 for E04~E07 and CF1.
 * @note  HFConst is a 16 bit unsigned integer.
 * @rmtoll HFCONST1   HFCONST1   LL_D2F_SetHFConst1
 * @param D2Fx    D2F Instance
 * @param HFConst high frequency constant
 * @return None
 */
__STATIC_INLINE void LL_D2F_SetHFConst1(D2F_TypeDef *D2Fx, uint32_t HFConst)
{
    WRITE_REG(D2Fx->HFCONST1, (D2F_HFCONST1_HFCONST1 & HFConst));
}

/**
 * @brief Return HFConst1 of E04~E07 and CF1.
 * @note  HFConst is a 16 bit unsigned integer.
 * @rmtoll HFCONST1   HFCONST1   LL_D2F_GetHFConst1
 * @param D2Fx  D2F Instance
 * @return HFCONST1 value
 */
__STATIC_INLINE uint32_t LL_D2F_GetHFConst1(D2F_TypeDef *D2Fx)
{
    return (uint32_t)(READ_REG(D2Fx->HFCONST1));
}

/**
 * @brief Configure HFConst2 for E08~E11 and CF2.
 * @note  HFConst is a 16 bit unsigned integer.
 * @rmtoll HFCONST2   HFCONST2   LL_D2F_SetHFConst2
 * @param D2Fx    D2F Instance
 * @param HFConst high frequency constant
 * @return None
 */
__STATIC_INLINE void LL_D2F_SetHFConst2(D2F_TypeDef *D2Fx, uint32_t HFConst)
{
    WRITE_REG(D2Fx->HFCONST2, (D2F_HFCONST2_HFCONST2 & HFConst));
}

/**
 * @brief Return HFConst2 of E08~E11 and CF2.
 * @note  HFConst is a 16 bit unsigned integer.
 * @rmtoll HFCONST2   HFCONST2   LL_D2F_GetHFConst2
 * @param D2Fx  D2F Instance
 * @return HFCONST2 value
 */
__STATIC_INLINE uint32_t LL_D2F_GetHFConst2(D2F_TypeDef *D2Fx)
{
    return (uint32_t)(READ_REG(D2Fx->HFCONST2));
}

/**
 * @brief Enable D2F module.
 * @note  write 0 to enable D2F module.
 * @rmtoll CTRL   EN   LL_D2F_Enable
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_Enable(D2F_TypeDef *D2Fx)
{
    CLEAR_BIT(D2Fx->CTRL, D2F_CTRL_EN);
}

/**
 * @brief Disable D2F module.
 * @note  Stop the D2F calculate and reset all the counter and numerical operation unit,
 *        but do not reset the register.
 * @note  write 1 to disable D2F module.
 * @rmtoll CTRL   EN   LL_D2F_Disable
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_Disable(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->CTRL, D2F_CTRL_EN);
}

/**
 * @brief Indicates whether the D2F module is enabled.
 * @rmtoll CTRL   EN   LL_D2F_IsEnabled
 * @param D2Fx  D2F Instance
 * @return State of bit, 0: D2F is enabled, 1: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsEnabled(D2F_TypeDef *D2Fx)
{
    return (uint32_t)(READ_REG(D2Fx->CTRL) == 0);
}

/**
 * @brief Configure the integrator clock mode for D2F.
 * @note  Integrator clock mode can be default mode or constant mode.
 * @note  Refer to datasheet for clock frequency for the two integrator clock mode.
 * @rmtoll CFG   CLK_SEL   LL_D2F_SetIntegratorClkMode
 * @param D2Fx  D2F Instance
 * @param IntegratorClkMode  This parameter can be one of the following values:
 *         @arg @ref LL_D2F_CLKMODE_DEFAULT
 *         @arg @ref LL_D2F_CLKMODE_CONSTANT
 * @return None
 */
__STATIC_INLINE void LL_D2F_SetIntegratorClkMode(D2F_TypeDef *D2Fx, uint32_t IntegratorClkMode)
{
    MODIFY_REG(D2Fx->CFG, D2F_CFG_CLK_SEL, IntegratorClkMode);
}

/**
 * @brief Get the integrator clock mode.
 * @note  Refer to datasheet for clock frequency for the two integrator clock mode.
 * @rmtoll CFG   CLK_SEL   LL_D2F_GetIntegratorClkMode
 * @param D2Fx  D2F Instance
 * @return Returned value can be one of the following values:
 *         @arg @ref LL_D2F_CLKMODE_DEFAULT
 *         @arg @ref LL_D2F_CLKMODE_CONSTANT
 */
__STATIC_INLINE uint32_t LL_D2F_GetIntegratorClkMode(D2F_TypeDef *D2Fx)
{
    return (uint32_t)(READ_BIT(D2Fx->CFG, D2F_CFG_CLK_SEL));
}

/**
 * @brief Configure the load type of input power for D2F.
 * @note  Load type mode can be normal type or Synchronized loading type base on SimpTC.
 * @note  Select the Synchronized loading type base on SimpTC should also configure AOS module: SimpTC triggle D2F.
 * @rmtoll CFG   LOADTYPE   LL_D2F_SetPowerLoadType
 * @param D2Fx  D2F Instance
 * @param LoadType  This parameter can be one of the following values:
 *         @arg @ref LL_D2F_LOADTYPE_NORMAL
 *         @arg @ref LL_D2F_LOADTYPE_SIMPTC
 * @return None
 */
__STATIC_INLINE void LL_D2F_SetPowerLoadType(D2F_TypeDef *D2Fx, uint32_t LoadType)
{
    MODIFY_REG(D2Fx->CFG, D2F_CFG_LOADTYPE, LoadType);
}

/**
 * @brief Get the load type of input power for D2F.
 * @note  Load type mode can be normal type or Synchronized loading type base on SimpTC.
 * @rmtoll CFG   LOADTYPE   LL_D2F_GetPowerLoadType
 * @param D2Fx  D2F Instance
 * @return Returned value can be one of the following values:
 *         @arg @ref LL_D2F_LOADTYPE_NORMAL
 *         @arg @ref LL_D2F_LOADTYPE_SIMPTC
 */
__STATIC_INLINE uint32_t LL_D2F_GetPowerLoadType(D2F_TypeDef *D2Fx)
{
    return (uint32_t)(READ_BIT(D2Fx->CFG, D2F_CFG_LOADTYPE));
}

/**
 * @brief Configure the load type of input power for D2F.
 * @note  Energy type mode can be read to clear(default) or counter types.
 * @rmtoll CFG   ENGTYPE   LL_D2F_SetEnergyType
 * @param D2Fx  D2F Instance
 * @param EnergyType  This parameter can be one of the following values:
 *         @arg @ref  LL_D2F_ENGTYPE_R2CLR
 *         @arg @ref  LL_D2F_ENGTYPE_COUNTER
 * @return None
 */
__STATIC_INLINE void LL_D2F_SetEnergyType(D2F_TypeDef *D2Fx, uint32_t EnergyType)
{
    MODIFY_REG(D2Fx->CFG, D2F_CFG_ENGTYPE, EnergyType);
}

/**
 * @brief Get the load type of input power for D2F.
 * @note  Energy type mode can be read to clear(default) or counter types.
 * @rmtoll CFG   ENGTYPE   LL_D2F_GetEnergyType
 * @param D2Fx  D2F Instance
 * @return Returned value can be one of the following values:
 *         @arg @ref  LL_D2F_ENGTYPE_R2CLR
 *         @arg @ref  LL_D2F_ENGTYPE_COUNTER
 */
__STATIC_INLINE uint32_t LL_D2F_GetEnergyType(D2F_TypeDef *D2Fx)
{
    return (uint32_t)(READ_BIT(D2Fx->CFG, D2F_CFG_ENGTYPE));
}

/**
 * @brief Configure the D2F channel for CF2.
 * @note  CF2 is shared by D2F08, D2F09, D2F10 and D2F11 Channels as pulse output.
 * @note  Need to configure the alternate function of I/O as CF2.
 * @rmtoll CF_CFG   CF2_SEL   LL_D2F_SetCF2Channel
 * @param D2Fx  D2F Instance
 * @param Channel  This parameter can be one of the following values:
 *         @arg @ref  LL_D2F_CF2_D2F08
 *         @arg @ref  LL_D2F_CF2_D2F09
 *         @arg @ref  LL_D2F_CF2_D2F10
 *         @arg @ref  LL_D2F_CF2_D2F11
 * @return None
 */
__STATIC_INLINE void LL_D2F_SetCF2Channel(D2F_TypeDef *D2Fx, uint32_t Channel)
{
    MODIFY_REG(D2Fx->CF_CFG, D2F_CF_CFG_CF2_SEL, Channel);
}

/**
 * @brief Get the D2F channel for CF2.
 * @note  CF2 is shared by D2F08, D2F09, D2F10 and D2F11 Channels as pulse output.
 * @note  CF can only be used by one channel at the same time.
 * @note  Need to configure the alternate function of I/O as CF2.
 * @rmtoll CF_CFG   CF2_SEL   LL_D2F_GetCF2Channel
 * @param D2Fx  D2F Instance
 * @return Return value can be one of the following values:
 *         @arg @ref  LL_D2F_CF2_D2F08
 *         @arg @ref  LL_D2F_CF2_D2F09
 *         @arg @ref  LL_D2F_CF2_D2F10
 *         @arg @ref  LL_D2F_CF2_D2F11
 */
__STATIC_INLINE uint32_t LL_D2F_GetCF2Channel(D2F_TypeDef *D2Fx)
{
    return (uint32_t)(READ_BIT(D2Fx->CF_CFG, D2F_CF_CFG_CF2_SEL));
}

/**
 * @brief Configure the D2F channel for CF1.
 * @note  CF1 is shared by D2F04, D2F05, D2F06 and D2F07 Channels as pulse output.
 * @note  CF can only be used by one channel at the same time.
 * @note  Need to configure the alternate function of I/O as CF1.
 * @rmtoll CF_CFG   CF1_SEL   LL_D2F_SetCF1Channel
 * @param D2Fx  D2F Instance
 * @param Channel  This parameter can be one of the following values:
 *         @arg @ref  LL_D2F_CF1_D2F04
 *         @arg @ref  LL_D2F_CF1_D2F05
 *         @arg @ref  LL_D2F_CF1_D2F06
 *         @arg @ref  LL_D2F_CF1_D2F07
 * @return None
 */
__STATIC_INLINE void LL_D2F_SetCF1Channel(D2F_TypeDef *D2Fx, uint32_t Channel)
{
    MODIFY_REG(D2Fx->CF_CFG, D2F_CF_CFG_CF1_SEL, Channel);
}

/**
 * @brief Get the D2F channel for CF1.
 * @note  CF1 is shared by D2F04, D2F05, D2F06 and D2F07 Channels as pulse output.
 * @note  CF can only be used by one channel at the same time.
 * @note  Need to configure the alternate function of I/O as CF1.
 * @rmtoll CF_CFG   CF1_SEL   LL_D2F_GetCF1Channel
 * @param D2Fx  D2F Instance
 * @return Return value can be one of the following values:
 *         @arg @ref  LL_D2F_CF1_D2F04
 *         @arg @ref  LL_D2F_CF1_D2F05
 *         @arg @ref  LL_D2F_CF1_D2F06
 *         @arg @ref  LL_D2F_CF1_D2F07
 */
__STATIC_INLINE uint32_t LL_D2F_GetCF1Channel(D2F_TypeDef *D2Fx)
{
    return (uint32_t)(READ_BIT(D2Fx->CF_CFG, D2F_CF_CFG_CF1_SEL));
}

/**
 * @brief Configure the D2F channel for CF0.
 * @note  CF0 is shared by D2F00, D2F01, D2F02 and D2F03 Channels as pulse output.
 * @note  CF can only be used by one channel at the same time.
 * @note  Need to configure the alternate function of I/O as CF0.
 * @rmtoll CF_CFG   CF0_SEL   LL_D2F_SetCF0Channel
 * @param D2Fx  D2F Instance
 * @param Channel  This parameter can be one of the following values:
 *         @arg @ref  LL_D2F_CF0_D2F00
 *         @arg @ref  LL_D2F_CF0_D2F01
 *         @arg @ref  LL_D2F_CF0_D2F02
 *         @arg @ref  LL_D2F_CF0_D2F03
 * @return None
 */
__STATIC_INLINE void LL_D2F_SetCF0Channel(D2F_TypeDef *D2Fx, uint32_t Channel)
{
    MODIFY_REG(D2Fx->CF_CFG, D2F_CF_CFG_CF0_SEL, Channel);
}

/**
 * @brief Get the D2F channel for CF0.
 * @note  CF0 is shared by D2F00, D2F01, D2F02 and D2F03 Channels as pulse output.
 * @note  CF can only be used by one channel at the same time.
 * @note  Need to configure the alternate function of I/O as CF0.
 * @rmtoll CF_CFG   CF0_SEL   LL_D2F_GetCF0Channel
 * @param D2Fx  D2F Instance
 * @return Return value can be one of the following values:
 *         @arg @ref  LL_D2F_CF0_D2F00
 *         @arg @ref  LL_D2F_CF0_D2F01
 *         @arg @ref  LL_D2F_CF0_D2F02
 *         @arg @ref  LL_D2F_CF0_D2F03
 */
__STATIC_INLINE uint32_t LL_D2F_GetCF0Channel(D2F_TypeDef *D2Fx)
{
    return (uint32_t)(READ_BIT(D2Fx->CF_CFG, D2F_CF_CFG_CF0_SEL));
}

/**
 * @brief Enable power cover interrupt.
 * @rmtoll IE   PCOV_IE   LL_D2F_EnableIT_PowerCover
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_EnableIT_PCOV(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IE, LL_D2F_IE_PCOV);
}

/**
 * @brief Disable power cover interrupt.
 * @rmtoll IE   PCOV_IE   LL_D2F_DisableIT_PowerCover
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_DisableIT_PCOV(D2F_TypeDef *D2Fx)
{
    CLEAR_BIT(D2Fx->IE, LL_D2F_IE_PCOV);
}

/**
 * @brief Indicates whether the power cover interrupt is enabled.
 * @rmtoll IE   PCOV_IE   LL_D2F_IsEnabledIT_PowerCover
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsEnabledIT_PCOV(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IE, LL_D2F_IE_PCOV) == LL_D2F_IE_PCOV));
}

/**
 * @brief Enable D2F11 interrupt.
 * @rmtoll IE   D2F11_IE   LL_D2F_EnableIT_D2F11
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_EnableIT_D2F11(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IE, LL_D2F_IE_D2F11);
}

/**
 * @brief Disable D2F11 interrupt.
 * @rmtoll IE   D2F11_IE   LL_D2F_DisableIT_D2F11
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_DisableIT_D2F11(D2F_TypeDef *D2Fx)
{
    CLEAR_BIT(D2Fx->IE, LL_D2F_IE_D2F11);
}

/**
 * @brief Indicates whether the D2F11 interrupt is enabled.
 * @rmtoll IE   D2F11_IE   LL_D2F_IsEnabledIT_D2F11
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsEnabledIT_D2F11(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IE, LL_D2F_IE_D2F11) == LL_D2F_IE_D2F11));
}

/**
 * @brief Enable D2F10 interrupt.
 * @rmtoll IE   D2F10_IE   LL_D2F_EnableIT_D2F10
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_EnableIT_D2F10(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IE, LL_D2F_IE_D2F10);
}

/**
 * @brief Disable D2F10 interrupt.
 * @rmtoll IE   D2F10_IE   LL_D2F_DisableIT_D2F10
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_DisableIT_D2F10(D2F_TypeDef *D2Fx)
{
    CLEAR_BIT(D2Fx->IE, LL_D2F_IE_D2F10);
}

/**
 * @brief Indicates whether the D2F10 interrupt is enabled.
 * @rmtoll IE   D2F10_IE   LL_D2F_IsEnabledIT_D2F10
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsEnabledIT_D2F10(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IE, LL_D2F_IE_D2F10) == LL_D2F_IE_D2F10));
}

/**
 * @brief Enable D2F09 interrupt.
 * @rmtoll IE   D2F09_IE   LL_D2F_EnableIT_D2F09
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_EnableIT_D2F09(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IE, LL_D2F_IE_D2F09);
}

/**
 * @brief Disable D2F09 interrupt.
 * @rmtoll IE   D2F09_IE   LL_D2F_DisableIT_D2F09
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_DisableIT_D2F09(D2F_TypeDef *D2Fx)
{
    CLEAR_BIT(D2Fx->IE, LL_D2F_IE_D2F09);
}

/**
 * @brief Indicates whether the D2F09 interrupt is enabled.
 * @rmtoll IE   D2F09_IE   LL_D2F_IsEnabledIT_D2F09
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsEnabledIT_D2F09(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IE, LL_D2F_IE_D2F09) == LL_D2F_IE_D2F09));
}

/**
 * @brief Enable D2F08 interrupt.
 * @rmtoll IE   D2F08_IE   LL_D2F_EnableIT_D2F08
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_EnableIT_D2F08(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IE, LL_D2F_IE_D2F08);
}

/**
 * @brief Disable D2F08 interrupt.
 * @rmtoll IE   D2F08_IE   LL_D2F_DisableIT_D2F08
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_DisableIT_D2F08(D2F_TypeDef *D2Fx)
{
    CLEAR_BIT(D2Fx->IE, LL_D2F_IE_D2F08);
}

/**
 * @brief Indicates whether the D2F08 interrupt is enabled.
 * @rmtoll IE   D2F08_IE   LL_D2F_IsEnabledIT_D2F08
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsEnabledIT_D2F08(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IE, LL_D2F_IE_D2F08) == LL_D2F_IE_D2F08));
}

/**
 * @brief Enable D2F07 interrupt.
 * @rmtoll IE   D2F07_IE   LL_D2F_EnableIT_D2F07
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_EnableIT_D2F07(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IE, LL_D2F_IE_D2F07);
}

/**
 * @brief Disable D2F07 interrupt.
 * @rmtoll IE   D2F07_IE   LL_D2F_DisableIT_D2F07
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_DisableIT_D2F07(D2F_TypeDef *D2Fx)
{
    CLEAR_BIT(D2Fx->IE, LL_D2F_IE_D2F07);
}

/**
 * @brief Indicates whether the D2F07 interrupt is enabled.
 * @rmtoll IE   D2F07_IE   LL_D2F_IsEnabledIT_D2F07
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsEnabledIT_D2F07(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IE, LL_D2F_IE_D2F07) == LL_D2F_IE_D2F07));
}

/**
 * @brief Enable D2F06 interrupt.
 * @rmtoll IE   D2F06_IE   LL_D2F_EnableIT_D2F06
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_EnableIT_D2F06(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IE, LL_D2F_IE_D2F06);
}

/**
 * @brief Disable D2F06 interrupt.
 * @rmtoll IE   D2F06_IE   LL_D2F_DisableIT_D2F06
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_DisableIT_D2F06(D2F_TypeDef *D2Fx)
{
    CLEAR_BIT(D2Fx->IE, LL_D2F_IE_D2F06);
}

/**
 * @brief Indicates whether the D2F06 interrupt is enabled.
 * @rmtoll IE   D2F06_IE   LL_D2F_IsEnabledIT_D2F06
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsEnabledIT_D2F06(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IE, LL_D2F_IE_D2F06) == LL_D2F_IE_D2F06));
}

/**
 * @brief Enable D2F05 interrupt.
 * @rmtoll IE   D2F05_IE   LL_D2F_EnableIT_D2F05
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_EnableIT_D2F05(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IE, LL_D2F_IE_D2F05);
}

/**
 * @brief Disable D2F05 interrupt.
 * @rmtoll IE   D2F05_IE   LL_D2F_DisableIT_D2F05
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_DisableIT_D2F05(D2F_TypeDef *D2Fx)
{
    CLEAR_BIT(D2Fx->IE, LL_D2F_IE_D2F05);
}

/**
 * @brief Indicates whether the D2F05 interrupt is enabled.
 * @rmtoll IE   D2F05_IE   LL_D2F_IsEnabledIT_D2F05
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsEnabledIT_D2F05(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IE, LL_D2F_IE_D2F05) == LL_D2F_IE_D2F05));
}

/**
 * @brief Enable D2F04 interrupt.
 * @rmtoll IE   D2F04_IE   LL_D2F_EnableIT_D2F04
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_EnableIT_D2F04(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IE, LL_D2F_IE_D2F04);
}

/**
 * @brief Disable D2F04 interrupt.
 * @rmtoll IE   D2F04_IE   LL_D2F_DisableIT_D2F04
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_DisableIT_D2F04(D2F_TypeDef *D2Fx)
{
    CLEAR_BIT(D2Fx->IE, LL_D2F_IE_D2F04);
}

/**
 * @brief Indicates whether the D2F04 interrupt is enabled.
 * @rmtoll IE   D2F04_IE   LL_D2F_IsEnabledIT_D2F04
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsEnabledIT_D2F04(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IE, LL_D2F_IE_D2F04) == LL_D2F_IE_D2F04));
}

/**
 * @brief Enable D2F03 interrupt.
 * @rmtoll IE   D2F03_IE   LL_D2F_EnableIT_D2F03
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_EnableIT_D2F03(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IE, LL_D2F_IE_D2F03);
}

/**
 * @brief Disable D2F03 interrupt.
 * @rmtoll IE   D2F03_IE   LL_D2F_DisableIT_D2F03
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_DisableIT_D2F03(D2F_TypeDef *D2Fx)
{
    CLEAR_BIT(D2Fx->IE, LL_D2F_IE_D2F03);
}

/**
 * @brief Indicates whether the D2F03 interrupt is enabled.
 * @rmtoll IE   D2F03_IE   LL_D2F_IsEnabledIT_D2F03
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsEnabledIT_D2F03(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IE, LL_D2F_IE_D2F03) == LL_D2F_IE_D2F03));
}

/**
 * @brief Enable D2F02 interrupt.
 * @rmtoll IE   D2F02_IE   LL_D2F_EnableIT_D2F02
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_EnableIT_D2F02(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IE, LL_D2F_IE_D2F02);
}

/**
 * @brief Disable D2F02 interrupt.
 * @rmtoll IE   D2F02_IE   LL_D2F_DisableIT_D2F02
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_DisableIT_D2F02(D2F_TypeDef *D2Fx)
{
    CLEAR_BIT(D2Fx->IE, LL_D2F_IE_D2F02);
}

/**
 * @brief Indicates whether the D2F02 interrupt is enabled.
 * @rmtoll IE   D2F02_IE   LL_D2F_IsEnabledIT_D2F02
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsEnabledIT_D2F02(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IE, LL_D2F_IE_D2F02) == LL_D2F_IE_D2F02));
}

/**
 * @brief Enable D2F01 interrupt.
 * @rmtoll IE   D2F01_IE   LL_D2F_EnableIT_D2F01
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_EnableIT_D2F01(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IE, LL_D2F_IE_D2F01);
}

/**
 * @brief Disable D2F01 interrupt.
 * @rmtoll IE   D2F01_IE   LL_D2F_DisableIT_D2F01
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_DisableIT_D2F01(D2F_TypeDef *D2Fx)
{
    CLEAR_BIT(D2Fx->IE, LL_D2F_IE_D2F01);
}

/**
 * @brief Indicates whether the D2F01 interrupt is enabled.
 * @rmtoll IE   D2F01_IE   LL_D2F_IsEnabledIT_D2F01
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsEnabledIT_D2F01(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IE, LL_D2F_IE_D2F01) == LL_D2F_IE_D2F01));
}

/**
 * @brief Enable D2F00 interrupt.
 * @rmtoll IE   D2F00_IE   LL_D2F_EnableIT_D2F00
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_EnableIT_D2F00(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IE, LL_D2F_IE_D2F00);
}

/**
 * @brief Disable D2F00 interrupt.
 * @rmtoll IE   D2F00_IE   LL_D2F_DisableIT_D2F00
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_DisableIT_D2F00(D2F_TypeDef *D2Fx)
{
    CLEAR_BIT(D2Fx->IE, LL_D2F_IE_D2F00);
}

/**
 * @brief Indicates whether the D2F00 interrupt is enabled.
 * @rmtoll IE   D2F00_IE   LL_D2F_IsEnabledIT_D2F00
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsEnabledIT_D2F00(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IE, LL_D2F_IE_D2F00) == LL_D2F_IE_D2F00));
}

/**
 * @brief Indicates whether the power cover interrupt is enabled.
 * @rmtoll IF   PCOV_IF   LL_D2F_IsActiveFlag_PCOV
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_PCOV(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IF, LL_D2F_IE_PCOV) == LL_D2F_IE_PCOV));
}

/**
 * @brief Clear power cover interrupt and flag.
 * @rmtoll IF   PCOV_IF   LL_D2F_ClearFlag_PCOV
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_ClearFlag_PCOV(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IF, LL_D2F_IE_PCOV);
}

/**
 * @brief Indicates whether the D2F11 interrupt is enabled.
 * @rmtoll IF   D2F11_IF   LL_D2F_IsActiveFlag_D2F11
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F11(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IF, LL_D2F_IE_D2F11) == LL_D2F_IE_D2F11));
}

/**
 * @brief Clear D2F11 interrupt and flag.
 * @rmtoll IF   D2F11_IF   LL_D2F_ClearFlag_D2F11
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_ClearFlag_D2F11(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IF, LL_D2F_IE_D2F11);
}

/**
 * @brief Indicates whether the D2F10 interrupt is enabled.
 * @rmtoll IF   D2F10_IF   LL_D2F_IsActiveFlag_D2F10
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F10(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IF, LL_D2F_IE_D2F10) == LL_D2F_IE_D2F10));
}

/**
 * @brief Clear D2F10 interrupt and flag.
 * @rmtoll IF   D2F10_IF   LL_D2F_ClearFlag_D2F10
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_ClearFlag_D2F10(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IF, LL_D2F_IE_D2F10);
}

/**
 * @brief Indicates whether the D2F09 interrupt is enabled.
 * @rmtoll IF   D2F09_IF   LL_D2F_IsActiveFlag_D2F09
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F09(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IF, LL_D2F_IE_D2F09) == LL_D2F_IE_D2F09));
}

/**
 * @brief Clear D2F09 interrupt and flag.
 * @rmtoll IF   D2F09_IF   LL_D2F_ClearFlag_D2F09
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_ClearFlag_D2F09(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IF, LL_D2F_IE_D2F09);
}

/**
 * @brief Indicates whether the D2F08 interrupt is enabled.
 * @rmtoll IF   D2F08_IF   LL_D2F_IsActiveFlag_D2F08
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F08(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IF, LL_D2F_IE_D2F08) == LL_D2F_IE_D2F08));
}

/**
 * @brief Clear D2F08 interrupt and flag.
 * @rmtoll IF   D2F08_IF   LL_D2F_ClearFlag_D2F08
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_ClearFlag_D2F08(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IF, LL_D2F_IE_D2F08);
}

/**
 * @brief Indicates whether the D2F07 interrupt is enabled.
 * @rmtoll IF   D2F07_IF   LL_D2F_IsActiveFlag_D2F07
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F07(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IF, LL_D2F_IE_D2F07) == LL_D2F_IE_D2F07));
}

/**
 * @brief Clear D2F07 interrupt and flag.
 * @rmtoll IF   D2F07_IF   LL_D2F_ClearFlag_D2F07
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_ClearFlag_D2F07(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IF, LL_D2F_IE_D2F07);
}

/**
 * @brief Indicates whether the D2F06 interrupt is enabled.
 * @rmtoll IF   D2F06_IF   LL_D2F_IsActiveFlag_D2F06
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F06(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IF, LL_D2F_IE_D2F06) == LL_D2F_IE_D2F06));
}

/**
 * @brief Clear D2F06 interrupt and flag.
 * @rmtoll IF   D2F06_IF   LL_D2F_ClearFlag_D2F06
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_ClearFlag_D2F06(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IF, LL_D2F_IE_D2F06);
}

/**
 * @brief Indicates whether the D2F05 interrupt is enabled.
 * @rmtoll IF   D2F05_IF   LL_D2F_IsActiveFlag_D2F05
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F05(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IF, LL_D2F_IE_D2F05) == LL_D2F_IE_D2F05));
}

/**
 * @brief Clear D2F05 interrupt and flag.
 * @rmtoll IF   D2F05_IF   LL_D2F_ClearFlag_D2F05
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_ClearFlag_D2F05(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IF, LL_D2F_IE_D2F05);
}

/**
 * @brief Indicates whether the D2F04 interrupt is enabled.
 * @rmtoll IF   D2F04_IF   LL_D2F_IsActiveFlag_D2F04
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F04(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IF, LL_D2F_IE_D2F04) == LL_D2F_IE_D2F04));
}

/**
 * @brief Clear D2F04 interrupt and flag.
 * @rmtoll IF   D2F04_IF   LL_D2F_ClearFlag_D2F04
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_ClearFlag_D2F04(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IF, LL_D2F_IE_D2F04);
}

/**
 * @brief Indicates whether the D2F03 interrupt is enabled.
 * @rmtoll IF   D2F03_IF   LL_D2F_IsActiveFlag_D2F03
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F03(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IF, LL_D2F_IE_D2F03) == LL_D2F_IE_D2F03));
}

/**
 * @brief Clear D2F03 interrupt and flag.
 * @rmtoll IF   D2F03_IF   LL_D2F_ClearFlag_D2F03
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_ClearFlag_D2F03(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IF, LL_D2F_IE_D2F03);
}

/**
 * @brief Indicates whether the D2F02 interrupt is enabled.
 * @rmtoll IF   D2F02_IF   LL_D2F_IsActiveFlag_D2F02
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F02(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IF, LL_D2F_IE_D2F02) == LL_D2F_IE_D2F02));
}

/**
 * @brief Clear D2F02 interrupt and flag.
 * @rmtoll IF   D2F02_IF   LL_D2F_ClearFlag_D2F02
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_ClearFlag_D2F02(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IF, LL_D2F_IE_D2F02);
}

/**
 * @brief Indicates whether the D2F01 interrupt is enabled.
 * @rmtoll IF   D2F01_IF   LL_D2F_IsActiveFlag_D2F01
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F01(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IF, LL_D2F_IE_D2F01) == LL_D2F_IE_D2F01));
}

/**
 * @brief Clear D2F01 interrupt and flag.
 * @rmtoll IF   D2F01_IF   LL_D2F_ClearFlag_D2F01
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_ClearFlag_D2F01(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IF, LL_D2F_IE_D2F01);
}

/**
 * @brief Indicates whether the D2F00 interrupt is enabled.
 * @rmtoll IF   D2F00_IF   LL_D2F_IsActiveFlag_D2F00
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F00(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->IF, LL_D2F_IE_D2F00) == LL_D2F_IE_D2F00));
}

/**
 * @brief Clear D2F00 interrupt and flag.
 * @rmtoll IF   D2F00_IF   LL_D2F_ClearFlag_D2F00
 * @param D2Fx  D2F Instance
 * @return None
 */
__STATIC_INLINE void LL_D2F_ClearFlag_D2F00(D2F_TypeDef *D2Fx)
{
    SET_BIT(D2Fx->IF, LL_D2F_IE_D2F00);
}

/**
 * @brief Indicates whether the D2F11 channel is in No-Load status.
 * @note  NOLOAD register is read only.
 * @rmtoll NOLOAD   D2F11_NLD   LL_D2F_IsActiveFlag_D2F11_NLD
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: No-Load status, 0: normal status.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F11_NLD(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->NOLOAD, LL_D2F_NLD_D2F11) == LL_D2F_NLD_D2F11));
}

/**
 * @brief Indicates whether the D2F10 channel is in No-Load status.
 * @note  NOLOAD register is read only.
 * @rmtoll NOLOAD   D2F10_NLD   LL_D2F_IsActiveFlag_D2F10_NLD
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: No-Load status, 0: normal status.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F10_NLD(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->NOLOAD, LL_D2F_NLD_D2F10) == LL_D2F_NLD_D2F10));
}

/**
 * @brief Indicates whether the D2F09 channel is in No-Load status.
 * @note  NOLOAD register is read only.
 * @rmtoll NOLOAD   D2F09_NLD   LL_D2F_IsActiveFlag_D2F09_NLD
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: No-Load status, 0: normal status.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F09_NLD(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->NOLOAD, LL_D2F_NLD_D2F09) == LL_D2F_NLD_D2F09));
}

/**
 * @brief Indicates whether the D2F08 channel is in No-Load status.
 * @note  NOLOAD register is read only.
 * @rmtoll NOLOAD   D2F08_NLD   LL_D2F_IsActiveFlag_D2F08_NLD
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: No-Load status, 0: normal status.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F08_NLD(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->NOLOAD, LL_D2F_NLD_D2F08) == LL_D2F_NLD_D2F08));
}

/**
 * @brief Indicates whether the D2F07 channel is in No-Load status.
 * @note  NOLOAD register is read only.
 * @rmtoll NOLOAD   D2F07_NLD   LL_D2F_IsActiveFlag_D2F07_NLD
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: No-Load status, 0: normal status.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F07_NLD(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->NOLOAD, LL_D2F_NLD_D2F07) == LL_D2F_NLD_D2F07));
}

/**
 * @brief Indicates whether the D2F06 channel is in No-Load status.
 * @note  NOLOAD register is read only.
 * @rmtoll NOLOAD   D2F06_NLD   LL_D2F_IsActiveFlag_D2F06_NLD
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: No-Load status, 0: normal status.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F06_NLD(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->NOLOAD, LL_D2F_NLD_D2F06) == LL_D2F_NLD_D2F06));
}

/**
 * @brief Indicates whether the D2F05 channel is in No-Load status.
 * @note  NOLOAD register is read only.
 * @rmtoll NOLOAD   D2F05_NLD   LL_D2F_IsActiveFlag_D2F05_NLD
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: No-Load status, 0: normal status.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F05_NLD(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->NOLOAD, LL_D2F_NLD_D2F05) == LL_D2F_NLD_D2F05));
}

/**
 * @brief Indicates whether the D2F04 channel is in No-Load status.
 * @note  NOLOAD register is read only.
 * @rmtoll NOLOAD   D2F04_NLD   LL_D2F_IsActiveFlag_D2F04_NLD
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: No-Load status, 0: normal status.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F04_NLD(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->NOLOAD, LL_D2F_NLD_D2F04) == LL_D2F_NLD_D2F04));
}

/**
 * @brief Indicates whether the D2F03 channel is in No-Load status.
 * @note  NOLOAD register is read only.
 * @rmtoll NOLOAD   D2F03_NLD   LL_D2F_IsActiveFlag_D2F03_NLD
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: No-Load status, 0: normal status.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F03_NLD(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->NOLOAD, LL_D2F_NLD_D2F03) == LL_D2F_NLD_D2F03));
}

/**
 * @brief Indicates whether the D2F02 channel is in No-Load status.
 * @note  NOLOAD register is read only.
 * @rmtoll NOLOAD   D2F02_NLD   LL_D2F_IsActiveFlag_D2F02_NLD
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: No-Load status, 0: normal status.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F02_NLD(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->NOLOAD, LL_D2F_NLD_D2F02) == LL_D2F_NLD_D2F02));
}

/**
 * @brief Indicates whether the D2F01 channel is in No-Load status.
 * @note  NOLOAD register is read only.
 * @rmtoll NOLOAD   D2F01_NLD   LL_D2F_IsActiveFlag_D2F01_NLD
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: No-Load status, 0: normal status.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F01_NLD(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->NOLOAD, LL_D2F_NLD_D2F01) == LL_D2F_NLD_D2F01));
}

/**
 * @brief Indicates whether the D2F00 channel is in No-Load status.
 * @note  NOLOAD register is read only.
 * @rmtoll NOLOAD   D2F00_NLD   LL_D2F_IsActiveFlag_D2F00_NLD
 * @param D2Fx  D2F Instance
 * @return State of bit, 1: No-Load status, 0: normal status.
 */
__STATIC_INLINE uint32_t LL_D2F_IsActiveFlag_D2F00_NLD(D2F_TypeDef *D2Fx)
{
    return (uint32_t)((READ_BIT(D2Fx->NOLOAD, LL_D2F_NLD_D2F00) == LL_D2F_NLD_D2F00));
}

/**
 * @brief Write input power for D2F.
 * @note  Input power should be 32-bit signed integer.
 * @rmtoll P00~P11   P00~P11   LL_D2F_SetPower
 * @param D2Fx  D2F Instance
 * @param PowerPos  This parameter can be one of the following values:
 *         @arg @ref LL_D2F_POWER_00
 *         @arg @ref LL_D2F_POWER_01
 *         @arg @ref LL_D2F_POWER_02
 *         @arg @ref LL_D2F_POWER_03
 *         @arg @ref LL_D2F_POWER_04
 *         @arg @ref LL_D2F_POWER_05
 *         @arg @ref LL_D2F_POWER_06
 *         @arg @ref LL_D2F_POWER_07
 *         @arg @ref LL_D2F_POWER_08
 *         @arg @ref LL_D2F_POWER_09
 *         @arg @ref LL_D2F_POWER_10
 *         @arg @ref LL_D2F_POWER_11
 * @param Power   Input power value
 * @return None
 */
__STATIC_INLINE void LL_D2F_SetPower(D2F_TypeDef *D2Fx, uint32_t PowerPos, uint32_t Power)
{
    WRITE_REG(D2Fx->D2FP[PowerPos], Power);
}

/**
 * @brief Read input power for D2F.
 * @rmtoll P00~P11   P00~P11   LL_D2F_GetPower
 * @param D2Fx  D2F Instance
 * @param PowerPos  This parameter can be one of the following values:
 *         @arg @ref LL_D2F_POWER_00
 *         @arg @ref LL_D2F_POWER_01
 *         @arg @ref LL_D2F_POWER_02
 *         @arg @ref LL_D2F_POWER_03
 *         @arg @ref LL_D2F_POWER_04
 *         @arg @ref LL_D2F_POWER_05
 *         @arg @ref LL_D2F_POWER_06
 *         @arg @ref LL_D2F_POWER_07
 *         @arg @ref LL_D2F_POWER_08
 *         @arg @ref LL_D2F_POWER_09
 *         @arg @ref LL_D2F_POWER_10
 *         @arg @ref LL_D2F_POWER_11
 * @return Return Power
 */
__STATIC_INLINE uint32_t LL_D2F_GetPower(D2F_TypeDef *D2Fx, uint32_t PowerPos)
{
    return (uint32_t)(READ_REG(D2Fx->D2FP[PowerPos]));
}

/**
 * @brief Read output energy for D2F.
 * @note  Energy should be 32-bit unsigned integer, read only.
 * @note  The minimum unit of the energy register is 1/EC(kWh), and EC is the meter constant.
 * @rmtoll E00~E11   E00~E11   LL_D2F_GetEnergy
 * @param D2Fx  D2F Instance
 * @param EnergyPos  This parameter can be one of the following values:
 *         @arg @ref LL_D2F_ENERGY_00
 *         @arg @ref LL_D2F_ENERGY_01
 *         @arg @ref LL_D2F_ENERGY_02
 *         @arg @ref LL_D2F_ENERGY_03
 *         @arg @ref LL_D2F_ENERGY_04
 *         @arg @ref LL_D2F_ENERGY_05
 *         @arg @ref LL_D2F_ENERGY_06
 *         @arg @ref LL_D2F_ENERGY_07
 *         @arg @ref LL_D2F_ENERGY_08
 *         @arg @ref LL_D2F_ENERGY_09
 *         @arg @ref LL_D2F_ENERGY_10
 *         @arg @ref LL_D2F_ENERGY_11
 * @return Return energy
 */
__STATIC_INLINE uint32_t LL_D2F_GetEnergy(D2F_TypeDef *D2Fx, uint32_t EnergyPos)
{
    return (uint32_t)(READ_REG(D2Fx->D2FE[EnergyPos]));
}

/**
 * @brief Write start threshold of D2FP00~D2FP03.
 * @note  Start threshold should be 32-bit unsigned integer, if input power smaller than start threshold,
 *        the power is not interated, and a zero power will be interated.
 * @rmtoll STARTTH00   STARTTH00   LL_D2F_SetStartThreshold00
 * @param D2Fx  D2F Instance
 * @param Threshold   Start threshold of power
 * @return None
 */
__STATIC_INLINE void LL_D2F_SetStartThreshold00(D2F_TypeDef *D2Fx, uint32_t Threshold)
{
    WRITE_REG(D2Fx->STARTTH00, Threshold);
}

/**
 * @brief Read start threshold of D2FP00~D2FP03.
 * @rmtoll STARTTH00   STARTTH00   LL_D2F_GetStartThreshold00
 * @param D2Fx  D2F Instance
 * @return Return start threshold0 value
 */
__STATIC_INLINE uint32_t LL_D2F_GetStartThreshold00(D2F_TypeDef *D2Fx)
{
    return (uint32_t)(READ_REG(D2Fx->STARTTH00));
}

/**
 * @brief Write start threshold of D2FP04~D2FP07.
 * @note  Start threshold should be 32-bit unsigned integer, if input power smaller than start threshold,
 *        the power is not interated, and a zero power will be interated.
 * @rmtoll STARTTH01   STARTTH01   LL_D2F_SetStartThreshold01
 * @param D2Fx       D2F Instance
 * @param Threshold  Start threshold of power
 * @return None
 */
__STATIC_INLINE void LL_D2F_SetStartThreshold01(D2F_TypeDef *D2Fx, uint32_t Threshold)
{
    WRITE_REG(D2Fx->STARTTH01, Threshold);
}

/**
 * @brief Read start threshold of D2FP04~D2FP07.
 * @rmtoll STARTTH01   STARTTH01   LL_D2F_GetStartThreshold01
 * @param D2Fx  D2F Instance
 * @return Return start threshold0 value
 */
__STATIC_INLINE uint32_t LL_D2F_GetStartThreshold01(D2F_TypeDef *D2Fx)
{
    return (uint32_t)(READ_REG(D2Fx->STARTTH01));
}

/**
 * @brief Write start threshold of D2FP08~D2FP11.
 * @note  Start threshold should be 32-bit unsigned integer, if input power smaller than start threshold,
 *        the power is not interated, and a zero power will be interated.
 * @rmtoll STARTTH02   STARTTH02   LL_D2F_SetStartThreshold02
 * @param D2Fx       D2F Instance
 * @param Threshold  Start threshold of power
 * @return None
 */
__STATIC_INLINE void LL_D2F_SetStartThreshold02(D2F_TypeDef *D2Fx, uint32_t Threshold)
{
    WRITE_REG(D2Fx->STARTTH02, Threshold);
}

/**
 * @brief Read start threshold of D2FP08~D2FP11.
 * @rmtoll STARTTH02   STARTTH02   LL_D2F_GetStartThreshold02
 * @param D2Fx  D2F Instance
 * @return Return start threshold0 value
 */
__STATIC_INLINE uint32_t LL_D2F_GetStartThreshold02(D2F_TypeDef *D2Fx)
{
    return (uint32_t)(READ_REG(D2Fx->STARTTH02));
}
/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup D2F_LL_EF_Init Initialization and de-initialization functions
 * @{
 */
ErrorStatus LL_D2F_DeInit(D2F_TypeDef *D2Fx);
ErrorStatus LL_D2F_Init(D2F_TypeDef *D2Fx, LL_D2F_InitTypeDef *D2F_InitStruct);
void        LL_D2F_StructInit(LL_D2F_InitTypeDef *D2F_InitStruct);
/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */

#endif /* D2F */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_D2F_H */
