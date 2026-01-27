/**
 *****************************************************************************
 * @file   rn_rv_ll_lcss_vcc.h
 * @author Renergy Software Team
 * @brief  Header file of LCSS_VCC LL module.
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
#ifndef __RN_LL_LCSS_VCC_H
#define __RN_LL_LCSS_VCC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_lcss_vcc.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (LCSS_VCC)

/** @defgroup LCSS_VCC_LL LCSS_VCC
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup LCSS_VCC_LL_Private_Constants LCSS_VCC Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup LCSS_VCC_LL_Private_Macros LCSS_VCC Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup LCSS_VCC_LL_ES_INIT LCSS_VCC Exported Init structure
 * @{
 */

/**
 * @brief LL LCSS_VCC Init Structure definition
 */
typedef struct
{
    uint32_t HFThreshold;               /*!< Specifies the High frequency abnormal offset threshold.
                                            This parameter must be set to a value between Min_Data=0x00 and Max_Data=0xFF */

    uint32_t LFThreshold;               /*!< Specifies the low frequency abnormal offset threshold.
                                            This parameter must be set to a value between Min_Data=0x00 and Max_Data=0xFF */

    FunctionalState HFDetectionCommand; /*!< Specifies the high frequence detection.
                                            This parameter can be set either to ENABLE or DISABLE */

    FunctionalState LFDetectionCommand; /*!< Specifies the low frequence detection.
                                            This parameter can be set either to ENABLE or DISABLE */

    uint32_t ClockDivision;             /*!< Specifies the Clock frequency division.
                                            This parameter can be a value of @ref LCSS_VCC_LL_EC_Clock_Division.

                                            This feature can be modified afterwards using unitary function @ref LL_LCSS_VCC_SetClockDivision(). */

    uint32_t HFClockSelect;             /*!< Specifies the Clock selected of automatic switch to backup clock in high frequency abnormality.
                                            This parameter can be a value of @ref LCSS_VCC_LL_EC_HF. */

    uint32_t LFClockSelect;             /*!< Specifies the Clock selected of automatic switch to backup clock in low frequency abnormality.
                                            This parameter can be a value of @ref LCSS_VCC_LL_EC_LF. */

    uint32_t ARCVClockSelect;           /*!< Specifies the Clock selected of automatic switch to source clock in normal frequency.
                                            This parameter can be a value of @ref LCSS_VCC_LL_EC_ARCV. */

    uint32_t AnomalyCycles;             /*!< Specifies the High/low frequency abnormality threshold.
                                            This parameter must be set to a value between Min_Data=0x0 and Max_Data=0xF */

    uint32_t RecoveryCycles;            /*!< Specifies the LOSC recovery time configuration.
                                            This parameter must be set to a value between Min_Data=0x0000 and Max_Data=0xFFFF */
} LL_LCSS_VCC_InitTypeDef;

/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/* Exported constants -------------------------------------------------------*/
/** @defgroup LCSS_VCC_LL_Exported_Constants LCSS_VCC Exported Constants
 * @{
 */

/** @defgroup LCSS_VCC_LL_EC_HF HF
 * @{
 */
#define LL_LCSS_VCC_HF_SYSCLK_PLL_RCH                  LCSS_VCC_ASWH_HF_ASWH0                      /*!< (System clock: PLL to RCH) Enable automatic switch to backup clock in high frequency abnormality    */
#define LL_LCSS_VCC_HF_PLLSOURCE_LOSC_RCL              LCSS_VCC_ASWH_HF_ASWH1                      /*!< (PLL source: LOSC to RCL) Enable automatic switch to backup clock in high frequency abnormality     */
#define LL_LCSS_VCC_HF_KBI_LOSC_RCL                    LCSS_VCC_ASWH_HF_ASWH2                      /*!< (KBI Clock: LOSC to RCL) Enable automatic switch to backup clock in high frequency abnormality      */
#define LL_LCSS_VCC_HF_LPUART_LOSC_RCL                 LCSS_VCC_ASWH_HF_ASWH3                      /*!< (LPUART Clock: LOSC to RCL) Enable automatic switch to backup clock in high frequency abnormality   */
#define LL_LCSS_VCC_HF_LCD_LOSC_RCL                    LCSS_VCC_ASWH_HF_ASWH4                      /*!< (LCD Clock: LOSC to RCL) Enable automatic switch to backup clock in high frequency abnormality      */
#define LL_LCSS_VCC_HF_INFRARED_LOSC_RCL               LCSS_VCC_ASWH_HF_ASWH5                      /*!< (Infrared Clock: LOSC to RCL) Enable automatic switch to backup clock in high frequency abnormality */
#define LL_LCSS_VCC_HF_SYSCLK_LOSC_RCL                 LCSS_VCC_ASWH_HF_ASWH6                      /*!< (System clock: LOSC to RCL) Enable automatic switch to backup clock in high frequency abnormality   */
/**
 * @}
 */

/** @defgroup LCSS_VCC_LL_EC_LF LF
 * @{
 */
#define LL_LCSS_VCC_LF_SYSCLK_PLL_RCH                  LCSS_VCC_ASWH_LF_ASWH0                      /*!< (System clock: PLL to RCH) Enable automatic switch to backup clock in low frequency abnormality    */
#define LL_LCSS_VCC_LF_PLLSOURCE_LOSC_RCL              LCSS_VCC_ASWH_LF_ASWH1                      /*!< (PLL source: LOSC to RCL) Enable automatic switch to backup clock in low frequency abnormality     */
#define LL_LCSS_VCC_LF_KBI_LOSC_RCL                    LCSS_VCC_ASWH_LF_ASWH2                      /*!< (KBI Clock: LOSC to RCL) Enable automatic switch to backup clock in low frequency abnormality      */
#define LL_LCSS_VCC_LF_LPUART_LOSC_RCL                 LCSS_VCC_ASWH_LF_ASWH3                      /*!< (LPUART Clock: LOSC to RCL) Enable automatic switch to backup clock in low frequency abnormality   */
#define LL_LCSS_VCC_LF_LCD_LOSC_RCL                    LCSS_VCC_ASWH_LF_ASWH4                      /*!< (LCD Clock: LOSC to RCL) Enable automatic switch to backup clock in low frequency abnormality      */
#define LL_LCSS_VCC_LF_INFRARED_LOSC_RCL               LCSS_VCC_ASWH_LF_ASWH5                      /*!< (Infrared Clock: LOSC to RCL) Enable automatic switch to backup clock in low frequency abnormality */
#define LL_LCSS_VCC_LF_SYSCLK_LOSC_RCL                 LCSS_VCC_ASWH_LF_ASWH6                      /*!< (System clock: LOSC to RCL) Enable automatic switch to backup clock in low frequency abnormality   */
/**
 * @}
 */

/** @defgroup LCSS_VCC_LL_EC_ARCV Auto Recovery
 * @{
 */
#define LL_LCSS_VCC_ARCV_SYSCLK_PLL_RCH                LCSS_VCC_ASWH_CK_ARCV0                      /*!< (System clock: PLL to RCH) Enable automatic switch to backup clock in low frequency abnormality */
#define LL_LCSS_VCC_ARCV_PLLSOURCE_LOSC_RCL            LCSS_VCC_ASWH_CK_ARCV1                      /*!< (PLL source: LOSC to RCL)  Enable automatic switch to source clock in normal frequency          */
#define LL_LCSS_VCC_ARCV_KBI_LOSC_RCL                  LCSS_VCC_ASWH_CK_ARCV2                      /*!< (KBI Clock: LOSC to RCL)  Enable automatic switch to source clock in normal frequency           */
#define LL_LCSS_VCC_ARCV_LPUART_LOSC_RCL               LCSS_VCC_ASWH_CK_ARCV3                      /*!< (LPUART Clock: LOSC to RCL)  Enable automatic switch to source clock in normal frequency        */
#define LL_LCSS_VCC_ARCV_LCD_LOSC_RCL                  LCSS_VCC_ASWH_CK_ARCV4                      /*!< (LCD Clock: LOSC to RCL)  Enable automatic switch to source clock in normal frequency           */
#define LL_LCSS_VCC_ARCV_INFRARED_LOSC_RCL             LCSS_VCC_ASWH_CK_ARCV5                      /*!< (Infrared Clock: LOSC to RCL)  Enable automatic switch to source clock in normal frequency      */
#define LL_LCSS_VCC_ARCV_SYSCLK_LOSC_RCL               LCSS_VCC_ASWH_CK_ARCV6                      /*!< (System clock: LOSC to RCL)  Enable automatic switch to source clock in normal frequency        */
/**
 * @}
 */

/** @defgroup LCSS_VCC_LL_EC_Clock_Division Clock_Division
 * @{
 */
#define LL_LCSS_VCC_CLOCK_DIV_NONE                     LCSS_VCC_CTRL_DETD_DIV_NONE                 /*!< No Frequency division */
#define LL_LCSS_VCC_CLOCK_DIV_2                        LCSS_VCC_CTRL_DETD_DIV_2                    /*!< 2 Frequency division  */
#define LL_LCSS_VCC_CLOCK_DIV_4                        LCSS_VCC_CTRL_DETD_DIV_4                    /*!< 4 Frequency division  */
#define LL_LCSS_VCC_CLOCK_DIV_8                        LCSS_VCC_CTRL_DETD_DIV_8                    /*!< 8 Frequency division  */
/**
 * @}
 */

/** @defgroup LCSS_VCC_LL_EC_Software_Switch_Backup Software_Switch_Backup
 * @{
 */
#define LL_LCSS_VCC_SW_BACKUP_SYSCLK_PLL_RCH                LCSS_VCC_CLKSWH_CLK0_SWH1                      /*!< (System clock: PLL to RCH) software switch to backup clock    */
#define LL_LCSS_VCC_SW_BACKUP_PLLSOURCE_LOSC_RCL            LCSS_VCC_CLKSWH_CLK1_SWH1                      /*!< (PLL source: LOSC to RCL) software switch to backup clock     */
#define LL_LCSS_VCC_SW_BACKUP_KBI_LOSC_RCL                  LCSS_VCC_CLKSWH_CLK2_SWH1                      /*!< (KBI Clock: LOSC to RCL) software switch to backup clock      */
#define LL_LCSS_VCC_SW_BACKUP_LPUART_LOSC_RCL               LCSS_VCC_CLKSWH_CLK3_SWH1                      /*!< (LPUART Clock: LOSC to RCL) software switch to backup clock   */
#define LL_LCSS_VCC_SW_BACKUP_LCD_LOSC_RCL                  LCSS_VCC_CLKSWH_CLK4_SWH1                      /*!< (LCD Clock: LOSC to RCL) software switch to backup clock      */
#define LL_LCSS_VCC_SW_BACKUP_INFRARED_LOSC_RCL             LCSS_VCC_CLKSWH_CLK5_SWH1                      /*!< (Infrared Clock: LOSC to RCL) software switch to backup clock */
#define LL_LCSS_VCC_SW_BACKUP_SYSCLK_LOSC_RCL               LCSS_VCC_CLKSWH_CLK6_SWH1                      /*!< (System clock: LOSC to RCL) software switch to backup clock   */
/**
 * @}
 */

/** @defgroup LCSS_VCC_LL_EC_Software_Switch_Source Software_Switch_Source
 * @{
 */
#define LL_LCSS_VCC_SW_SOURCE_SYSCLK_RCH_PLL                LCSS_VCC_CLKSWH_CLK0_SWH0                      /*!< (System clock: RCH to PLL) software switch to source clock    */
#define LL_LCSS_VCC_SW_SOURCE_PLLSOURCE_RCL_LOSC            LCSS_VCC_CLKSWH_CLK1_SWH0                      /*!< (PLL source: RCL to LOSC) software switch to source clock     */
#define LL_LCSS_VCC_SW_SOURCE_KBI_RCL_LOSC                  LCSS_VCC_CLKSWH_CLK2_SWH0                      /*!< (KBI Clock: RCL to LOSC) software switch to source clock      */
#define LL_LCSS_VCC_SW_SOURCE_LPUART_RCL_LOSC               LCSS_VCC_CLKSWH_CLK3_SWH0                      /*!< (LPUART Clock: RCL to LOSC) software switch to source clock   */
#define LL_LCSS_VCC_SW_SOURCE_LCD_RCL_LOSC                  LCSS_VCC_CLKSWH_CLK4_SWH0                      /*!< (LCD Clock: RCL to LOSC) software switch to source clock      */
#define LL_LCSS_VCC_SW_SOURCE_INFRARED_RCL_LOSC             LCSS_VCC_CLKSWH_CLK5_SWH0                      /*!< (Infrared Clock: RCL to LOSC) software switch to source clock */
#define LL_LCSS_VCC_SW_SOURCE_SYSCLK_RCL_LOSC               LCSS_VCC_CLKSWH_CLK6_SWH0                      /*!< (System clock: RCL to LOSC) software switch to source clock   */
/**
 * @}
 */

/** @defgroup LCSS_VCC_LL_EC_Clock_Select Clock_Select
 * @{
 */
#define LL_LCSS_VCC_CLKSEL_SYSCLK_RCH_PLL                   LCSS_VCC_CLKSEL_CLKSEL0                      /*!< (System clock: PLL to RCH) Indicate the current clock selection, source clock or backup clock    */
#define LL_LCSS_VCC_CLKSEL_PLLSOURCE_RCL_LOSC               LCSS_VCC_CLKSEL_CLKSEL1                      /*!< (PLL source: LOSC to RCL) Indicate the current clock selection, source clock or backup clock     */
#define LL_LCSS_VCC_CLKSEL_KBI_RCL_LOSC                     LCSS_VCC_CLKSEL_CLKSEL2                      /*!< (KBI Clock: LOSC to RCL) Indicate the current clock selection, source clock or backup clock      */
#define LL_LCSS_VCC_CLKSEL_LPUART_RCL_LOSC                  LCSS_VCC_CLKSEL_CLKSEL3                      /*!< (LPUART Clock: LOSC to RCL) Indicate the current clock selection, source clock or backup clock   */
#define LL_LCSS_VCC_CLKSEL_LCD_RCL_LOSC                     LCSS_VCC_CLKSEL_CLKSEL4                      /*!< (LCD Clock: LOSC to RCL) Indicate the current clock selection, source clock or backup clock      */
#define LL_LCSS_VCC_CLKSEL_INFRARED_RCL_LOSC                LCSS_VCC_CLKSEL_CLKSEL5                      /*!< (Infrared Clock: LOSC to RCL) Indicate the current clock selection, source clock or backup clock */
#define LL_LCSS_VCC_CLKSEL_SYSCLK_RCL_LOSC                  LCSS_VCC_CLKSEL_CLKSEL6                      /*!< (System clock: LOSC to RCL) Indicate the current clock selection, source clock or backup clock   */
/**
 * @}
 */

/** @defgroup LCSS_VCC_LL_EC_Clock_Source_Backup Source_Backup
 * @{
 */
#define LL_LCSS_VCC_CLKSEL_BACKUP                   LCSS_VCC_CLKSEL_CLKSEL0                      /*!< clock select backup   */
#define LL_LCSS_VCC_CLKSEL_SOURCE                   0x00000000U                                  /*!< clock select source   */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup LCSS_VCC_LL_Exported_Macros LCSS_VCC Exported Macros
 * @{
 */
/** @defgroup LCSS_VCC_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in LCSS_VCC register
 * @param  __INSTANCE__ LCSS_VCC Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_LCSS_VCC_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in LCSS_VCC register
 * @param  __INSTANCE__ LCSS_VCC Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_LCSS_VCC_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup LCSS_VCC_LL_Exported_Functions LCSS_VCC Exported Functions
 * @{
 */

/** @defgroup LCSS_VCC_LL_EF_Unlock UnlocK
 * @{
 */

/**
 * @brief  UnlocK registers include LCSS_EN/LCSS_ASWH/LCSS_CTRL/LCSS_RCL_OS/LCSS_HFDLA_TH/LCSS_LFDLA_TH/LCSS_IE
 * @rmtoll PSW0         PSW           LL_LCSS_VCC_Unlock_PSW0
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_UnlockPSW0(void)
{
    MODIFY_REG(LCSS_VCC_CORE0->PSW0, LCSS_VCC_PSW0_PSW, 0x853CU);
}

/**
 * @brief  Lock registers include LCSS_EN/LCSS_ASWH/LCSS_CTRL/LCSS_RCL_OS/LCSS_HFDLA_TH/LCSS_LFDLA_TH/LCSS_IE
 * @rmtoll PSW0         PSW           LL_LCSS_VCC_Lock_PSW0
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_LockPSW0(void)
{
    MODIFY_REG(LCSS_VCC_CORE0->PSW0, LCSS_VCC_PSW0_PSW, 0x0000U);
}

/**
 * @brief  Indicate if the PSW registers is Unlocked include LCSS_EN/LCSS_ASWH/LCSS_CTRL/LCSS_RCL_OS/LCSS_HFDLA_TH/LCSS_LFDLA_TH/LCSS_IE
 * @rmtoll PSW0         PSW           LL_LCSS_VCC_IsUnlocked_PSW0
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_IsUnlockedPSW0(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->PSW0, LCSS_VCC_PSW0_PSW) == (0x853CU));
}

/**
 * @brief  UnlocK registers include LCSS_CLKSWH
 * @rmtoll PSW1         PSW           LL_LCSS_VCC_Unlock_PSW1
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_UnlockPSW1(void)
{
    MODIFY_REG(LCSS_VCC_CORE0->PSW1, LCSS_VCC_PSW1_PSW, 0x8529U);
}

/**
 * @brief  Lock registers include LCSS_CLKSWH
 * @rmtoll PSW1         PSW           LL_LCSS_VCC_Lock_PSW1
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_LockPSW1(void)
{
    MODIFY_REG(LCSS_VCC_CORE0->PSW1, LCSS_VCC_PSW1_PSW, 0x0000U);
}

/**
 * @brief  Indicate if the PSW registers is Unlocked include LCSS_CLKSWH
 * @rmtoll PSW1         PSW           LL_LCSS_VCC_IsUnlocked_PSW1
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_IsUnlockedPSW1(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->PSW1, LCSS_VCC_PSW1_PSW) == (0x8529U));
}

/**
 * @}
 */

/** @defgroup I2C_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief  Enable high frequence detection
 * @rmtoll EN         HFDT_EN           LL_LCSS_VCC_EnableHFDetection
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_EnableHFDetection(void)
{
    SET_BIT(LCSS_VCC_CORE0->EN, LCSS_VCC_EN_HFDT_EN);
}

/**
 * @brief  Disable high frequence detection
 * @rmtoll EN         HFDT_EN           LL_LCSS_VCC_DisableHFDetection
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_DisableHFDetection(void)
{
    CLEAR_BIT(LCSS_VCC_CORE0->EN, LCSS_VCC_EN_HFDT_EN);
}

/**
 * @brief  Indicate if the high frequence detection is enabled
 * @rmtoll EN         HFDT_EN           LL_LCSS_VCC_IsEnabledHFDetection
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_IsEnabledHFDetection(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->EN, LCSS_VCC_EN_HFDT_EN) == (LCSS_VCC_EN_HFDT_EN));
}

/**
 * @brief  Enable low frequence detection
 * @rmtoll EN         HFDT_EN           LL_LCSS_VCC_EnableLFDetection
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_EnableLFDetection(void)
{
    SET_BIT(LCSS_VCC_CORE0->EN, LCSS_VCC_EN_LFDT_EN);
}

/**
 * @brief  Disable low frequence detection
 * @rmtoll EN         HFDT_EN           LL_LCSS_VCC_DisableLFDetection
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_DisableLFDetection(void)
{
    CLEAR_BIT(LCSS_VCC_CORE0->EN, LCSS_VCC_EN_LFDT_EN);
}

/**
 * @brief  Indicate if the low frequence detection is enabled
 * @rmtoll EN         HFDT_EN           LL_LCSS_VCC_IsEnabledLFDetection
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_IsEnabledLFDetection(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->EN, LCSS_VCC_EN_LFDT_EN) == (LCSS_VCC_EN_LFDT_EN));
}

/**
 * @brief  Enable automatic switch to backup clock in high frequency abnormality
 * @rmtoll ASWH         HF_ASWHx           LL_LCSS_VCC_EnableHFAutoSwitch
 * @param  HFClockSelect This parameter can be a combination of the following values:
 *         @arg @ref LL_LCSS_VCC_HF_SYSCLK_PLL_RCH
 *         @arg @ref LL_LCSS_VCC_HF_PLLSOURCE_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_KBI_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_LPUART_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_LCD_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_INFRARED_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_SYSCLK_LOSC_RCL
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_EnableHFAutoSwitch(uint32_t HFClockSelect)
{
    SET_BIT(LCSS_VCC_CORE0->ASWH, HFClockSelect);
}

/**
 * @brief  Disable automatic switch to backup clock in high frequency abnormality
 * @rmtoll ASWH         HF_ASWHx           LL_LCSS_VCC_DisableHFAutoSwitch
 * @param  HFClockSelect This parameter can be a combination of the following values:
 *         @arg @ref LL_LCSS_VCC_HF_SYSCLK_PLL_RCH
 *         @arg @ref LL_LCSS_VCC_HF_PLLSOURCE_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_KBI_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_LPUART_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_LCD_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_INFRARED_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_SYSCLK_LOSC_RCL
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_DisableHFAutoSwitch(uint32_t HFClockSelect)
{
    CLEAR_BIT(LCSS_VCC_CORE0->ASWH, HFClockSelect);
}

/**
 * @brief  Indicate if automatic switch to backup clock in high frequency abnormality is enabled
 * @rmtoll ASWH         HF_ASWHx           LL_LCSS_VCC_IsEnabledHFAutoSwitch
 * @param  HFClockSelect This parameter can be a combination of the following values:
 *         @arg @ref LL_LCSS_VCC_HF_SYSCLK_PLL_RCH
 *         @arg @ref LL_LCSS_VCC_HF_PLLSOURCE_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_KBI_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_LPUART_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_LCD_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_INFRARED_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_HF_SYSCLK_LOSC_RCL
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_IsEnabledHFAutoSwitch(uint32_t HFClockSelect)
{
    return (READ_BIT(LCSS_VCC_CORE0->ASWH, HFClockSelect) == (HFClockSelect));
}

/**
 * @brief  Enable automatic switch to backup clock in low frequency abnormality
 * @rmtoll ASWH         LF_ASWHx           LL_LCSS_VCC_EnableLFAutoSwitch
 * @param  LFClockSelect This parameter can be a combination of the following values:
 *         @arg @ref LL_LCSS_VCC_LF_SYSCLK_PLL_RCH
 *         @arg @ref LL_LCSS_VCC_LF_PLLSOURCE_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_KBI_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_LPUART_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_LCD_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_INFRARED_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_SYSCLK_LOSC_RCL
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_EnableLFAutoSwitch(uint32_t LFClockSelect)
{
    SET_BIT(LCSS_VCC_CORE0->ASWH, LFClockSelect);
}

/**
 * @brief  Disable automatic switch to backup clock in low frequency abnormality
 * @rmtoll ASWH         LF_ASWHx           LL_LCSS_VCC_DisableLFAutoSwitch
 * @param  LFClockSelect This parameter can be a combination of the following values:
 *         @arg @ref LL_LCSS_VCC_LF_SYSCLK_PLL_RCH
 *         @arg @ref LL_LCSS_VCC_LF_PLLSOURCE_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_KBI_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_LPUART_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_LCD_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_INFRARED_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_SYSCLK_LOSC_RCL
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_DisableLFAutoSwitch(uint32_t LFClockSelect)
{
    CLEAR_BIT(LCSS_VCC_CORE0->ASWH, LFClockSelect);
}

/**
 * @brief  Indicate if automatic switch to backup clock in low frequency abnormality is enabled
 * @rmtoll ASWH         LF_ASWHx           LL_LCSS_VCC_IsEnabledLFAutoSwitch
 * @param  LFClockSelect This parameter can be a combination of the following values:
 *         @arg @ref LL_LCSS_VCC_LF_SYSCLK_PLL_RCH
 *         @arg @ref LL_LCSS_VCC_LF_PLLSOURCE_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_KBI_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_LPUART_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_LCD_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_INFRARED_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_LF_SYSCLK_LOSC_RCL
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_IsEnabledLFAutoSwitch(uint32_t LFClockSelect)
{
    return (READ_BIT(LCSS_VCC_CORE0->ASWH, LFClockSelect) == (LFClockSelect));
}

/**
 * @brief  Enable automatic switch to source clock in normal frequency
 * @rmtoll ASWH         CK_ARCVx           LL_LCSS_VCC_EnableAutoRecovery
 * @param  ARCVClockSelect This parameter can be a combination of the following values:
 *         @arg @ref LL_LCSS_VCC_ARCV_SYSCLK_PLL_RCH
 *         @arg @ref LL_LCSS_VCC_ARCV_PLLSOURCE_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_KBI_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_LPUART_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_LCD_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_INFRARED_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_SYSCLK_LOSC_RCL
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_EnableAutoRecovery(uint32_t ARCVClockSelect)
{
    SET_BIT(LCSS_VCC_CORE0->ASWH, ARCVClockSelect);
}

/**
 * @brief  Disable automatic switch to source clock in normal frequency
 * @rmtoll ASWH         CK_ARCVx           LL_LCSS_VCC_DisableAutoRecovery
 * @param  ARCVClockSelect This parameter can be a combination of the following values:
 *         @arg @ref LL_LCSS_VCC_ARCV_SYSCLK_PLL_RCH
 *         @arg @ref LL_LCSS_VCC_ARCV_PLLSOURCE_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_KBI_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_LPUART_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_LCD_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_INFRARED_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_SYSCLK_LOSC_RCL
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_DisableAutoRecovery(uint32_t ARCVClockSelect)
{
    CLEAR_BIT(LCSS_VCC_CORE0->ASWH, ARCVClockSelect);
}

/**
 * @brief  Indicate if automatic switch to source clock in normal frequency is enabled
 * @rmtoll ASWH         CK_ARCVx           LL_LCSS_VCC_IsEnabledAutoRecovery
 * @param  ARCVClockSelect This parameter can be a combination of the following values:
 *         @arg @ref LL_LCSS_VCC_ARCV_SYSCLK_PLL_RCH
 *         @arg @ref LL_LCSS_VCC_ARCV_PLLSOURCE_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_KBI_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_LPUART_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_LCD_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_INFRARED_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_ARCV_SYSCLK_LOSC_RCL
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_IsEnabledAutoRecovery(uint32_t ARCVClockSelect)
{
    return (READ_BIT(LCSS_VCC_CORE0->ASWH, ARCVClockSelect) == (ARCVClockSelect));
}

/**
 * @brief  Set Clock Division
 * @rmtoll CTRL          DETD_DIV           LL_LCSS_VCC_SetClockDivision
 * @param  ClockDivision This parameter can be one of the following values:
 *         @arg @ref LL_LCSS_VCC_CLOCK_DIV_NONE
 *         @arg @ref LL_LCSS_VCC_CLOCK_DIV_2
 *         @arg @ref LL_LCSS_VCC_CLOCK_DIV_4
 *         @arg @ref LL_LCSS_VCC_CLOCK_DIV_8
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_SetClockDivision(uint32_t ClockDivision)
{
    MODIFY_REG(LCSS_VCC_CORE0->CTRL, LCSS_VCC_CTRL_DETD_DIV, ClockDivision);
}

/**
 * @brief  Get Clock Division
 * @rmtoll CTRL          DETD_DIV           LL_LCSS_VCC_GetClockDivision
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LCSS_VCC_CLOCK_DIV_NONE
 *         @arg @ref LL_LCSS_VCC_CLOCK_DIV_2
 *         @arg @ref LL_LCSS_VCC_CLOCK_DIV_4
 *         @arg @ref LL_LCSS_VCC_CLOCK_DIV_8
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_GetClockDivision(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->CTRL, LCSS_VCC_CTRL_DETD_DIV));
}

/**
 * @brief  Set High/low frequency abnormality threshold
 * @rmtoll CTRL          HF_CYC           LL_LCSS_VCC_SetAnomalyCycles
 * @param  AnomalyCycles Value between Min_Data=0x0 and Max_Data=0xF
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_SetAnomalyCycles(uint32_t AnomalyCycles)
{
    MODIFY_REG(LCSS_VCC_CORE0->CTRL, LCSS_VCC_CTRL_HF_CYC, AnomalyCycles << LCSS_VCC_CTRL_HF_CYC_POS);
}

/**
 * @brief  Get High/low frequency abnormality threshold
 * @rmtoll CTRL          HF_CYC           LL_LCSS_VCC_GetAnomalyCycles
 * @retval Anomaly Cycles (Value between Min_Data=0x0 and Max_Data=0xF)
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_GetAnomalyCycles(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->CTRL, LCSS_VCC_CTRL_HF_CYC) >> LCSS_VCC_CTRL_HF_CYC_POS);
}

/**
 * @brief  Set LOSC recovery time configuration
 * @rmtoll CTRL          RCV_CYC           LL_LCSS_VCC_SetRecoveryCycles
 * @param  RecoveryCycles Value between Min_Data=0x0000 and Max_Data=0xFFFF
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_SetRecoveryCycles(uint32_t RecoveryCycles)
{
    MODIFY_REG(LCSS_VCC_CORE0->CTRL, LCSS_VCC_CTRL_RCV_CYC, RecoveryCycles << LCSS_VCC_CTRL_RCV_CYC_POS);
}

/**
 * @brief  Get LOSC recovery time configuration
 * @rmtoll CTRL          RCV_CYC           LL_LCSS_VCC_GetRecoveryCycles
 * @retval Recovery Cycles (Value between Min_Data=0x0000 and Max_Data=0xFFFF)
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_GetRecoveryCycles(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->CTRL, LCSS_VCC_CTRL_RCV_CYC) >> LCSS_VCC_CTRL_RCV_CYC_POS);
}

/**
 * @brief  Set RCL32K frequency offset
 * @rmtoll RCL_OS          RCL_OS           LL_LCSS_VCC_SetRCLOffset
 * @param  Offset Value between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_SetRCLOffset(uint32_t Offset)
{
    MODIFY_REG(LCSS_VCC_CORE0->RCL_OS, LCSS_VCC_RCL_OS_RCL_OS, Offset << LCSS_VCC_RCL_OS_RCL_OS_POS);
}

/**
 * @brief  Get RCL32K frequency offset
 * @rmtoll RCL_OS          RCL_OS           LL_LCSS_VCC_GetRCLOffset
 * @retval RCL32K Offset (Value between Min_Data=0x00 and Max_Data=0xFF)
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_GetRCLOffset(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->RCL_OS, LCSS_VCC_RCL_OS_RCL_OS) >> LCSS_VCC_RCL_OS_RCL_OS_POS);
}

/**
 * @brief  Set High frequency abnormal offset threshold
 * @note   when (RCNT - TCNT) > HFDLA_TH, determined as LOSC is high frequency.
 *         RCNT: The sampling value obtained by RCL307K using the RCL frequency division signal;
            TCNT: The sampling value obtained by RCL307K using the LOSC frequency division signal;
 * @rmtoll HFDLA_TH          HFDLA_TH           LL_LCSS_VCC_SetHFThreshold
 * @param  HFThreshold Value between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_SetHFThreshold(uint32_t HFThreshold)
{
    MODIFY_REG(LCSS_VCC_CORE0->HFDLA_TH, LCSS_VCC_HFDLA_TH_HFDLA_TH, HFThreshold << LCSS_VCC_HFDLA_TH_HFDLA_TH_POS);
}

/**
 * @brief  Get High frequency abnormal offset threshold
 * @note   when (RCNT - TCNT) > HFDLA_TH, determined as LOSC is high frequency.
 *         RCNT: The sampling value obtained by RCL307K using the RCL frequency division signal;
            TCNT: The sampling value obtained by RCL307K using the LOSC frequency division signal;
 * @rmtoll HFDLA_TH          HFDLA_TH           LL_LCSS_VCC_GetHFThreshold
 * @retval HF Threshold (Value between Min_Data=0x00 and Max_Data=0xFF)
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_GetHFThreshold(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->HFDLA_TH, LCSS_VCC_HFDLA_TH_HFDLA_TH) >> LCSS_VCC_HFDLA_TH_HFDLA_TH_POS);
}

/**
 * @brief  Set low frequency abnormal offset threshold
 * @note   when (TCNT - RCNT) > LFDLA_TH, determined as LOSC is low frequency.
 *         RCNT: The sampling value obtained by RCL307K using the RCL frequency division signal;
            TCNT: The sampling value obtained by RCL307K using the LOSC frequency division signal;
 * @rmtoll LFDLA_TH          LFDLA_TH           LL_LCSS_VCC_SetLFThreshold
 * @param  LFThreshold Value between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_SetLFThreshold(uint32_t LFThreshold)
{
    MODIFY_REG(LCSS_VCC_CORE0->LFDLA_TH, LCSS_VCC_LFDLA_TH_LFDLA_TH, LFThreshold << LCSS_VCC_LFDLA_TH_LFDLA_TH_POS);
}

/**
 * @brief  Get low frequency abnormal offset threshold
 * @note   when (TCNT - RCNT) > LFDLA_TH, determined as LOSC is low frequency.
 *         RCNT: The sampling value obtained by RCL307K using the RCL frequency division signal;
            TCNT: The sampling value obtained by RCL307K using the LOSC frequency division signal;
 * @rmtoll LFDLA_TH          LFDLA_TH           LL_LCSS_VCC_GetLFThreshold
 * @retval LF Threshold (Value between Min_Data=0x00 and Max_Data=0xFF)
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_GetLFThreshold(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->LFDLA_TH, LCSS_VCC_LFDLA_TH_LFDLA_TH) >> LCSS_VCC_LFDLA_TH_LFDLA_TH_POS);
}

/**
 * @brief  Software switch to backup clock
 * @rmtoll CLKSWH          CLKx_SWH1           LL_LCSS_VCC_SwitchToBackupClock
 * @param  ClockSelect This parameter can be one of the following values:
 *         @arg @ref LL_LCSS_VCC_SW_BACKUP_SYSCLK_PLL_RCH
 *         @arg @ref LL_LCSS_VCC_SW_BACKUP_PLLSOURCE_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_SW_BACKUP_KBI_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_SW_BACKUP_LPUART_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_SW_BACKUP_LCD_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_SW_BACKUP_INFRARED_LOSC_RCL
 *         @arg @ref LL_LCSS_VCC_SW_BACKUP_SYSCLK_LOSC_RCL
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_SwitchToBackupClock(uint32_t ClockSelect)
{
    SET_BIT(LCSS_VCC_CORE0->CLKSWH, ClockSelect);
}

/**
 * @brief  Software switch to source clock
 * @rmtoll CLKSWH          CLKx_SWH0           LL_LCSS_VCC_SwitchToSourceClock
 * @param  ClockSelect This parameter can be one of the following values:
 *         @arg @ref LL_LCSS_VCC_SW_SOURCE_SYSCLK_RCH_PLL
 *         @arg @ref LL_LCSS_VCC_SW_SOURCE_PLLSOURCE_RCL_LOSC
 *         @arg @ref LL_LCSS_VCC_SW_SOURCE_KBI_RCL_LOSC
 *         @arg @ref LL_LCSS_VCC_SW_SOURCE_LPUART_RCL_LOSC
 *         @arg @ref LL_LCSS_VCC_SW_SOURCE_LCD_RCL_LOSC
 *         @arg @ref LL_LCSS_VCC_SW_SOURCE_INFRARED_RCL_LOSC
 *         @arg @ref LL_LCSS_VCC_SW_SOURCE_SYSCLK_RCL_LOSC
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_SwitchToSourceClock(uint32_t ClockSelect)
{
    SET_BIT(LCSS_VCC_CORE0->CLKSWH, ClockSelect);
}

/**
 * @brief  Indicate the current clock selection, source clock or backup clock
 * @rmtoll CLKSEL          CLKSELx           LL_LCSS_VCC_GetClockStatus
 * @param  ClockSelect This parameter can be one of the following values:
 *         @arg @ref LL_LCSS_VCC_CLKSEL_SYSCLK_RCH_PLL
 *         @arg @ref LL_LCSS_VCC_CLKSEL_PLLSOURCE_RCL_LOSC
 *         @arg @ref LL_LCSS_VCC_CLKSEL_KBI_RCL_LOSC
 *         @arg @ref LL_LCSS_VCC_CLKSEL_LPUART_RCL_LOSC
 *         @arg @ref LL_LCSS_VCC_CLKSEL_LCD_RCL_LOSC
 *         @arg @ref LL_LCSS_VCC_CLKSEL_INFRARED_RCL_LOSC
 *         @arg @ref LL_LCSS_VCC_CLKSEL_SYSCLK_RCL_LOSC
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_LCSS_VCC_CLKSEL_BACKUP
 *         @arg @ref LL_LCSS_VCC_CLKSEL_SOURCE
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_GetClockStatus(uint32_t ClockSelect)
{
    return (READ_BIT(LCSS_VCC_CORE0->CLKSEL, ClockSelect) == (ClockSelect));
}

/**
 * @}
 */

/** @defgroup LCSS_VCC_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable Clock high frequency abnormal interrupt
 * @rmtoll IE         HFREQ_IE           LL_LCSS_VCC_EnableIT_HF
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_EnableIT_HF(void)
{
    SET_BIT(LCSS_VCC_CORE0->IE, LCSS_VCC_IE_HFREQ_IE);
}

/**
 * @brief  Disable Clock high frequency abnormal interrupt
 * @rmtoll IE         HFREQ_IE           LL_LCSS_VCC_DisableIT_HF
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_DisableIT_HF(void)
{
    CLEAR_BIT(LCSS_VCC_CORE0->IE, LCSS_VCC_IE_HFREQ_IE);
}

/**
 * @brief  Indicate if the Clock high frequency abnormal interrupt is enabled
 * @rmtoll IE         HFREQ_IE           LL_LCSS_VCC_IsEnabledIT_HF
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_IsEnabledIT_HF(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->IE, LCSS_VCC_IE_HFREQ_IE) == (LCSS_VCC_IE_HFREQ_IE));
}

/**
 * @brief  Enable Clock low frequency abnormal interrupt
 * @rmtoll IE         LFREQ_IE           LL_LCSS_VCC_EnableIT_LF
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_EnableIT_LF(void)
{
    SET_BIT(LCSS_VCC_CORE0->IE, LCSS_VCC_IE_LFREQ_IE);
}

/**
 * @brief  Disable Clock low frequency abnormal interrupt
 * @rmtoll IE         LFREQ_IE           LL_LCSS_VCC_DisableIT_LF
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_DisableIT_LF(void)
{
    CLEAR_BIT(LCSS_VCC_CORE0->IE, LCSS_VCC_IE_LFREQ_IE);
}

/**
 * @brief  Indicate if the Clock low frequency abnormal interrupt is enabled
 * @rmtoll IE         LFREQ_IE           LL_LCSS_VCC_IsEnabledIT_LF
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_IsEnabledIT_LF(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->IE, LCSS_VCC_IE_LFREQ_IE) == (LCSS_VCC_IE_LFREQ_IE));
}

/**
 * @brief  Enable LOSC clock is OK interrupt
 * @rmtoll IE         LOSCOK_IE           LL_LCSS_VCC_EnableIT_LOSCOK
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_EnableIT_LOSCOK(void)
{
    SET_BIT(LCSS_VCC_CORE0->IE, LCSS_VCC_IE_LOSCOK_IE);
}

/**
 * @brief  Disable LOSC clock is OK interrupt
 * @rmtoll IE         LOSCOK_IE           LL_LCSS_VCC_DisableIT_LOSCOK
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_DisableIT_LOSCOK(void)
{
    CLEAR_BIT(LCSS_VCC_CORE0->IE, LCSS_VCC_IE_LOSCOK_IE);
}

/**
 * @brief  Indicate if the LOSC clock is OK interrupt is enabled
 * @rmtoll IE         LOSCOK_IE           LL_LCSS_VCC_IsEnabledIT_LOSCOK
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_IsEnabledIT_LOSCOK(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->IE, LCSS_VCC_IE_LOSCOK_IE) == (LCSS_VCC_IE_LOSCOK_IE));
}

/**
 * @}
 */

/** @defgroup LCSS_VCC_LL_EF_FLAG_management FLAG_management
 * @{
 */

/**
 * @brief  Check if the high-frequency abnormal event of the clock Flag is set or not
 * @note   This bit is cleared by writing a 1 to the bit.
 * @rmtoll IF          HFREQ_IF           LL_LCSS_VCC_IsActiveFlag_HF
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_IsActiveFlag_HF(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->IF, LCSS_VCC_IF_HFREQ_IF) == (LCSS_VCC_IF_HFREQ_IF));
}

/**
 * @brief  Clear the high-frequency abnormal event of the clock Flags
 * @note   This bit is cleared by writing a 1 to the bit.
 * @rmtoll IF          HFREQ_IF           LL_LCSS_VCC_ClearFlag_HF
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_ClearFlag_HF(void)
{
    WRITE_REG(LCSS_VCC_CORE0->IF, LCSS_VCC_IF_HFREQ_IF);
}

/**
 * @brief  Check if the low-frequency abnormal event of the clock Flag is set or not
 * @note   This bit is cleared by writing a 1 to the bit.
 * @rmtoll IF          LFREQ_IF           LL_LCSS_VCC_IsActiveFlag_LF
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_IsActiveFlag_LF(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->IF, LCSS_VCC_IF_LFREQ_IF) == (LCSS_VCC_IF_LFREQ_IF));
}

/**
 * @brief  Clear the low-frequency abnormal event of the clock Flags
 * @note   This bit is cleared by writing a 1 to the bit.
 * @rmtoll IF          LFREQ_IF           LL_LCSS_VCC_ClearFlag_LF
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_ClearFlag_LF(void)
{
    WRITE_REG(LCSS_VCC_CORE0->IF, LCSS_VCC_IF_LFREQ_IF);
}

/**
 * @brief  Check if the normal (recovery) frequency event of the clock Flag is set or not
 * @note   This bit is cleared by writing a 1 to the bit.
 * @rmtoll IF          FRCV_IF           LL_LCSS_VCC_IsActiveFlag_FRCV
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_IsActiveFlag_FRCV(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->IF, LCSS_VCC_IF_FRCV_IF) == (LCSS_VCC_IF_FRCV_IF));
}

/**
 * @brief  Clear the normal (recovery) frequency event of the clock Flags
 * @note   This bit is cleared by writing a 1 to the bit.
 * @rmtoll IF          FRCV_IF           LL_LCSS_VCC_ClearFlag_FRCV
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_ClearFlag_FRCV(void)
{
    WRITE_REG(LCSS_VCC_CORE0->IF, LCSS_VCC_IF_FRCV_IF);
}

/**
 * @brief  Check if the current LOSC clock frequency status Flag is set or not
 * @note   This bit is read only.
 * @rmtoll IF          LOSC_OK           LL_LCSS_VCC_IsActiveFlag_LOSCOK
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_IsActiveFlag_LOSCOK(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->IF, LCSS_VCC_IF_LOSC_OK) == (LCSS_VCC_IF_LOSC_OK));
}

/**
 * @}
 */

/** @defgroup LCSS_VCC_LL_EF_Abnormal_Frequency_Management Abnormal_Frequency_Management
 * @{
 */

/**
 * @brief  Set maximum high frequency abnormal frequency value
 * @rmtoll HFREQ          HFREQ           LL_LCSS_VCC_SetMaxHFAnomaly
 * @param  MaximumHF Value between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_SetMaxHFAnomaly(uint32_t MaximumHF)
{
    MODIFY_REG(LCSS_VCC_CORE0->HFREQ, LCSS_VCC_HFREQ_HFREQ, MaximumHF << LCSS_VCC_HFREQ_HFREQ_POS);
}

/**
 * @brief  Get maximum high frequency abnormal frequency value
 * @rmtoll HFREQ          HFREQ           LL_LCSS_VCC_GetMaxHFAnomaly
 * @retval Threshold (Value between Min_Data=0x00 and Max_Data=0xFF)
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_GetMaxHFAnomaly(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->HFREQ, LCSS_VCC_HFREQ_HFREQ) >> LCSS_VCC_HFREQ_HFREQ_POS);
}

/**
 * @brief  Set minimum low frequency abnormal frequency value
 * @rmtoll LFREQ          LFREQ           LL_LCSS_VCC_SetMiniLFAnomaly
 * @param  MinimumLF Value between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_LCSS_VCC_SetMiniLFAnomaly(uint32_t MinimumLF)
{
    MODIFY_REG(LCSS_VCC_CORE0->LFREQ, LCSS_VCC_LFREQ_LFREQ, MinimumLF << LCSS_VCC_LFREQ_LFREQ_POS);
}

/**
 * @brief  Get minimum low frequency abnormal frequency value
 * @rmtoll LFREQ          LFREQ           LL_LCSS_VCC_GetMiniLFAnomaly
 * @retval Threshold (Value between Min_Data=0x00 and Max_Data=0xFF)
 */
__STATIC_INLINE uint32_t LL_LCSS_VCC_GetMiniLFAnomaly(void)
{
    return (READ_BIT(LCSS_VCC_CORE0->LFREQ, LCSS_VCC_LFREQ_LFREQ) >> LCSS_VCC_LFREQ_LFREQ_POS);
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup LCSS_VCC_LL_EF_Init Initialization and de-initialization functions
 * @{
 */

// The initialization function can have more than one argument.
ErrorStatus LL_LCSS_VCC_Init(LL_LCSS_VCC_InitTypeDef *LCSS_VCC_InitStruct);
ErrorStatus LL_LCSS_VCC_DeInit(LCSS_VCC_TypeDef *LCSS_VCCx);
void LL_LCSS_VCC_StructInit(LL_LCSS_VCC_InitTypeDef *LCSS_VCC_InitStruct); // If the module is not suitable for the default configuration, you could not defines this function.

/**
 * @}
 */
#endif /* USE_FULL_LL_DRIVER */

/**
 * @}
 */

/**
 * @}
 */

#endif /* LCSS_VCC */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_LCSS_VCC_H */
