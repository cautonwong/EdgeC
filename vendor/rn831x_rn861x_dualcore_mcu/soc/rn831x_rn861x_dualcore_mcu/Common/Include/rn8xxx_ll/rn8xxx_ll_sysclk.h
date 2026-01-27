/******************************************************************************
 * @file     rn8xxx_ll_sysclk.h
 * @brief    System clock driver header
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#ifndef RN8XXX_LL_SYSCLK_H_
#define RN8XXX_LL_SYSCLK_H_

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************
  api return
**********************************************************/
typedef enum
{
    SYSCLK_PASS = 0,
    SYSCLK_FAIL = 1
} eSysclkRet_TypeDef;

/**********************************************************
  system control
**********************************************************/
typedef enum
{
#if !defined(RN831x_RN861x_MCU_V4)
    Clock_Losc          = 0x00U,
    Clock_RC_1M8        = 0x01U,
    Clock_PLL_7M4       = 0x02U,
    Clock_Hosc          = 0x03U,
    Clock_PLL_14M7      = 0x05U,
    Clock_RC_29M5       = 0x06U,
    Clock_PLL_29M5      = 0x07U,
    Clock_RCL_32K       = 0x08U,
    Clock_PLL_58M9_DIV2 = 0x09U,
    Clock_CHG_MAX       = 0x0AU,
#else
    Clock_Losc          = 0x00U,
    Clock_RCL_32K       = 0x01U,
    Clock_RC_1M8        = 0x02U,
    Clock_RC_59M        = 0x03U,
    Clock_PLL_7M4       = 0x04U,
    Clock_PLL_14M7      = 0x05U,
    Clock_PLL_29M5      = 0x06U,
    Clock_PLL_59M       = 0x07U,
    Clock_HOSC_7M4      = 0x08U,
    Clock_HOSC_14M7     = 0x09U,
    Clock_HOSC_29M5     = 0x0AU,
    Clock_END,
#endif
} eSysclkMode_TypeDef;

typedef enum
{
    Clock_Div_1 = 0x00U,
    Clock_Div_2 = 0x01U,
    Clock_Div_4 = 0x02U,
    Clock_Div_8 = 0x03U,
    Clock_Div_MAX = 0x4U
} eSysclkDiv_TypeDef;

/**
 * @brief system run mode change
 * @param mode: target system mode value:
 *   0 = LC, 8 = RCL
 *   1 = RC(1.8432M), 6 = RCH(29.4912M)
 *   2 = PLL(7.3728M), 5 = PLL(14.7456M), 7 = PLL(29.4912M)
 *   3 = HOSC(depends on OSC_CTRL2[2:0])
 *
 *   V1-A/B support mode=[0-3]
 *   V1-C/D support mode=[0-3, 5]
 *   V2 supports mode=[0-3, 5-8]
 * @param cdiv: clock div
 * @return eSysclkRet_TypeDef: SYSCLK_PASS = PASS, SYSCLK_FAIL = FAIL
 */
eSysclkRet_TypeDef LL_SYSCLK_SysModeChg(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef cdiv);
#define SysModeChg LL_SYSCLK_SysModeChg

/**
 * @brief fast system run mode change
 * @param mode: target system mode value, ONLY support Clock_Losc & Clock_RC_1M8 mode!!!
 * @return none
 */
void LL_SYSCLK_FastSysModeChg(eSysclkMode_TypeDef mode);
#define FastSysModeChg LL_SYSCLK_FastSysModeChg

/**
 * @brief nvm power mode active when system run in LC mode
 *  please call the API if system running in LC mode before using NVM
 */
void LL_SYSCLK_NvmActive(void);

/**
 * @brief nvm power mode exit for power save when system run in LC mode
 *  please call the API if system running in LC mode after using NVM for power save
 */
void LL_SYSCLK_NvmSleep(void);

unsigned int LL_SYSCLK_GetLibVersion(void);
#define GetLibVersion LL_SYSCLK_GetLibVersion
unsigned int LL_SYSCLK_GetCodeVersion(void);
#define GetCodeVersion LL_SYSCLK_GetCodeVersion

#if defined(RN831x_RN861x_MCU_V4)
typedef enum
{
    Clock_PLL_ON                = 0x00U,      /*!< PLL activation   */
    Clock_PLL_OFF               = 0x01U,      /*!< PLL deactivation */
    Clock_PLL_END
} eSycclkPLLSwitch_TypeDef;
eSysclkRet_TypeDef LL_SYSCLK_PLLConfig(eSysclkMode_TypeDef PLLFrequency, eSycclkPLLSwitch_TypeDef PLLState);
eSysclkRet_TypeDef LL_SYSCLK_OptionConfigCheck(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
/* r2989 */
