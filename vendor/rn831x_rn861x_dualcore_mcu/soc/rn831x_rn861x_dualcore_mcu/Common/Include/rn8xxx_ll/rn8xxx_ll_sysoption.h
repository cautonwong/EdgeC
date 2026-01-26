/******************************************************************************
 * @file     rn8xxx_ll_sysoption.h
 * @brief    system option driver header
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#ifndef RN8XXX_LL_SYSOPTION_H_
#define RN8XXX_LL_SYSOPTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rn8xxx_ll_devices.h"

/**********************************************************
  api return
**********************************************************/
typedef enum
{
    SYSOPTION_PASS = 0,
    SYSOPTION_FAIL = 1
} eSysoptionRet_TypeDef;

typedef enum {
    MCU_VER_V21 = 0x21,
    MCU_VER_V22 = 0x22,
    MCU_VER_V3  = 0x30,
    MCU_VER_INVALID = 0xff
} eMcuVer_TypeDef;

/**
 * @brief update dota0 parameter in SPL memory
 *
 * @param dota0 rtc dota0 value
 * @return eSysoptionRet_TypeDef : SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_Dota0CfgSet(unsigned short dota0);
#define Dota0CfgUpdate LL_SYSOPTION_Dota0CfgSet

/**
 * @brief update alphal parameters in SPL memory
 *
 * @param alphal rtc alphal value
 * @return eSysoptionRet_TypeDef : SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_AlphalCfgSet(unsigned short alphal);
#define AlphalCfgUpdate LL_SYSOPTION_AlphalCfgSet

/**
 * @brief update alphah parameters in SPL memory
 *
 * @param alphah rtc alphah value
 * @return eSysoptionRet_TypeDef : SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_AlphahCfgSet(unsigned short alphah);
#define AlphahCfgUpdate LL_SYSOPTION_AlphahCfgSet

/**
 * @brief update xt0 parameters in SPL memory
 *
 * @param xt0 rtc xt0 value
 * @return eSysoptionRet_TypeDef : SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_Xt0CfgSet(unsigned short xt0);
#define Xt0CfgUpdate LL_SYSOPTION_Xt0CfgSet

/**
 * @brief LL_SYSOPTION_Remap4Sel
 *
 * @param
 *    in unsigned char sel: select remap4(sel = 0) or remap0(sel = 1)
 * @return eSysoptionRet_TypeDef: SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 *
 */
eSysoptionRet_TypeDef LL_SYSOPTION_Remap4Sel(unsigned char sel);
#define Remap4Sel LL_SYSOPTION_Remap4Sel

/**
 * @brief LL_SYSOPTION_CpLevelSet
 *
 * @param cp_level 0~3(RN831x_RN861x_MCU_V1/RN821x_RN721x_SOC_B~D/RN202x_RN7326_SOC_B) or
 *                 0~4(RN831x_RN861x_MCU_V2~3/RN821x_RN721x_SOC_V2~3/RN202x_RN7326_SOC_V2)
 * @param cp_passwd cp password
 * @return eSysoptionRet_TypeDef: SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_CpLevelSet(unsigned int cp_level, unsigned int cp_passwd);

/**
 * @brief read dota0 from SPL memory
 *
 * @param dota0 dota0 save pointer
 * @return eSysoptionRet_TypeDef: SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_Dota0CfgGet(unsigned short *dota0);
#define Dota0CfgRead LL_SYSOPTION_Dota0CfgGet

/**
 * @brief read alphal from SPL memory
 *
 * @param alphal alphal save pointer
 * @return eSysoptionRet_TypeDef: SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_AlphalCfgGet(unsigned short *alphal);
#define AlphalCfgRead LL_SYSOPTION_AlphalCfgGet

/**
 * @brief read alphah from SPL memory
 *
 * @param alphah alphah save pointer
 * @return eSysoptionRet_TypeDef: SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_AlphahCfgGet(unsigned short *alphah);
#define AlphahCfgRead LL_SYSOPTION_AlphahCfgGet

/**
 * @brief read xt0 from SPL memory
 *
 * @param xt0 xt0 save pointer
 * @return eSysoptionRet_TypeDef: SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_Xt0CfgGet(unsigned short *xt0);
#define Xt0CfgRead LL_SYSOPTION_Xt0CfgGet

/**
 * @brief read remap
 *
 * @param remap remap save pointer
 *      0: remap0
 *      1: remap1, deprecated
 *      4: remap4, flash is divided into two
 * @return eSysoptionRet_TypeDef: SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_RemapGet(unsigned short *remap);
#define RemapRead LL_SYSOPTION_RemapGet

/**
 * @brief read cp-level
 *
 * @param cp_level cp-level save pointer
 * @return eSysoptionRet_TypeDef: SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_CpLevelGet(unsigned int *cp_level);

/**
 * @brief system configuration parameters check
 *
 * @return eSysoptionRet_TypeDef: SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_SysConfCheck(void);
#define SysConfCheck LL_SYSOPTION_SysConfCheck

/**
 * @brief rtc configuration parameters check
 *
 * @return eSysoptionRet_TypeDef: SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_RtcConfCheck(void);

/**
 * @brief wdt configuration parameters check
 *
 * @return eSysoptionRet_TypeDef: SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_WdtConfCheck(void);

/**
 * @brief rch configuration parameters check
 *
 * @return eSysoptionRet_TypeDef: SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_RchConfCheck(void);

/**
 * @brief rcl configuration parameters check
 *
 * @return eSysoptionRet_TypeDef: SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_RclConfCheck(void);

/**
 * @brief flash voltage auto-trim
 *   Before erase or program flash, this function must be called!
 * @return eSysoptionRet_TypeDef: SYSCTRL_PASS = PASS, SYSCTRL_FAIL = FAIL
 */
eSysoptionRet_TypeDef LL_SYSOPTION_FlashVoltageTrim(void);
#define FlashVoltageTrim LL_SYSOPTION_FlashVoltageTrim

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3)
/**
 * @brief get mcu version
 *
 * @return eMcuVer_TypeDef mcu version
 */
eMcuVer_TypeDef LL_SYSOPTION_GetMcuVersion(void);
#define GetMcuVersion LL_SYSOPTION_GetMcuVersion
#endif

#ifdef __cplusplus
}
#endif

#endif
/* r2989 */
