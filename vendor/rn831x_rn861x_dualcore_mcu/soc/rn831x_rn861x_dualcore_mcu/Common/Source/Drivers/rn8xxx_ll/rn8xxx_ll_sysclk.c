/******************************************************************************
 * @file     rn8xxx_ll_sysclk.c
 * @brief    System clock driver
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) 2008, Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#include "rn8xxx_ll_utils.h"
#include "rn8xxx_ll_sysclk.h"
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1)
#include "stddef.h"
#endif

#if defined(RN831x_RN861x_MCU_V4)
typedef struct
{
	unsigned int Reserved0[4];
	unsigned int (*SYSCLK_ClockModeConfig)          (eSysclkMode_TypeDef mode, unsigned int sdiv, eSysclkDiv_TypeDef cdiv);
    void         (*SYSCLK_FastClockModeConfig)      (eSysclkMode_TypeDef mode);
    unsigned int Reserved1;
    unsigned int (*SYSCLK_PLLOutputFreqConfig)      (eSysclkMode_TypeDef PLLFrequency, eSycclkPLLSwitch_TypeDef PLLState);
    unsigned int (*SYSCLK_OptionConfigCheck)        (void);
} SYSCLK_ApiTypeDef;

typedef unsigned int (*pSYSCLK_ClockModeConfig)     (eSysclkMode_TypeDef mode, unsigned int sdiv, eSysclkDiv_TypeDef cdiv);
typedef void         (*pSYSCLK_FastClockModeConfig) (eSysclkMode_TypeDef mode);
typedef unsigned int (*pSYSCLK_PLLOutputFreqConfig) (eSysclkMode_TypeDef PLLFrequency, eSycclkPLLSwitch_TypeDef PLLState);
typedef unsigned int (*pSYSCLK_OptionConfigCheck)   (void);
#else
#if !defined(NVRLIB_EN)
/* us unit */
#define PLL_TIMEOUT 3000000U
#define HOSC_TIMEOUT 100000U
#define RCH_TIMEOUT 10000U
#define LOSC_TIMEOUT 3000000U
#define MIP_TIMEOUT 7000U
#define HOSC_CALC_TIMEOUT 10000U

#define IS_LC_MODE(mode) ((Clock_Losc == (mode)) || (Clock_RCL_32K == (mode)))
#define IS_RC_MODE(mode) ((Clock_RC_1M8 == (mode)) || /*(Clock_RC_3M7 == (mode)) || */(Clock_RC_29M5 == (mode)))
#define IS_RCM_MODE(mode) (Clock_RC_1M8 == (mode))
#define IS_RC29_MODE(mode) (Clock_RC_29M5 == (mode))
#define IS_PLL_MODE(mode) ((Clock_PLL_7M4 == (mode)) || (Clock_PLL_14M7 == (mode)) || (Clock_PLL_29M5 == (mode)) || (Clock_PLL_58M9_DIV2 == (mode)))
#define IS_HOSC_MODE(mode) (Clock_Hosc == (mode))

#define RC_ON  (MEM32(OSC_CTRL1_ADDR) &= (~((unsigned int)1U << 1)))
#define RCM_OFF (MEM32(OSC_CTRL1_ADDR) |= ((unsigned int)1U << 1))
#define PLL_ON (MEM32(OSC_CTRL1_ADDR) &= (~((unsigned int)1U << 0)))
#define PLL_OFF (MEM32(OSC_CTRL1_ADDR) |= ((unsigned int)1U << 0))

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
#define RC32M_OFF {\
    MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~(((unsigned int)0x7U << 21) | ((unsigned int)0x3U << 24) | ((unsigned int)1U << 20)))) | ((unsigned int)0x6U << 21);\
    MEM32(OSC_CTRL2_9303_ADDR) = MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x7U << 21));\
    }
#endif
#if defined(RN_CM0_PLATFORM)
#define HOSC_ON(chipid) (MEM32(OSC_CTRL2_ADDR(chipid)) &= (~((unsigned int)1U << 3)))
#define HOSC_OFF(chipid) (MEM32(OSC_CTRL2_ADDR(chipid)) |= ((unsigned int)1U << 3))
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
#define HOSC_ON() (MEM32(OSC_CTRL2_9303_ADDR) &= (~((unsigned int)1U << 3)))
#define HOSC_OFF() (MEM32(OSC_CTRL2_9303_ADDR) |= ((unsigned int)1U << 3))
#else
#define HOSC_ON() (MEM32(OSC_CTRL2_9103_ADDR) &= (~((unsigned int)1U << 3)))
#define HOSC_OFF() (MEM32(OSC_CTRL2_9103_ADDR) |= ((unsigned int)1U << 3))
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_D) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
typedef enum
{
    LIGHT_MODE = 0,
    HIGH_MODE = 1,
    SAFE_MODE = 2
} ldo_mode_t;
#endif

#if defined(RN_CM0_PLATFORM)
static void setMipTW(unsigned int freq, rn_chipid_t chipid);
static eSysclkRet_TypeDef loscCalc(rn_chipid_t chipid);
static eSysclkRet_TypeDef loscCheck(rn_chipid_t chipid);
static eSysclkRet_TypeDef PllEnable(rn_chipid_t chipid);
static eSysclkRet_TypeDef HoscEnable(rn_chipid_t chipid);
static eSysclkRet_TypeDef clock_valid_check(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div, rn_chipid_t chipid);
static unsigned int get_target_freq(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div, rn_chipid_t chipid);
static void set_ldo_mode(ldo_mode_t ldo_mode, rn_chipid_t chipid);
static eSysclkRet_TypeDef sysmodeWrite(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div, rn_chipid_t chipid);
static eSysclkRet_TypeDef sysmode_params_check(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div, rn_chipid_t chipid);
static eSysclkRet_TypeDef HoscValid(rn_chipid_t chipid);
#else
static void setMipTW(unsigned int freq);
static eSysclkRet_TypeDef loscCalc(void);
static eSysclkRet_TypeDef loscCheck(void);
static eSysclkRet_TypeDef PllEnable(void);
static eSysclkRet_TypeDef HoscEnable(void);
static eSysclkRet_TypeDef clock_valid_check(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div);
static unsigned int get_target_freq(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div);
#if defined(RN821x_RN721x_SOC_D) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
static void set_ldo_mode(ldo_mode_t ldo_mode);
#endif
static eSysclkRet_TypeDef sysmodeWrite(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div);
static eSysclkRet_TypeDef sysmode_params_check(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div);
static eSysclkRet_TypeDef HoscValid(void);
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B)
static const unsigned char fl_tw_98ns[4] = {/*FL_TW_32K_98NS, FL_TW_1M_98NS, */FL_TW_3M_98NS, FL_TW_7M_98NS, FL_TW_14M_98NS, FL_TW_29M_98NS};
static const unsigned char ee_tw_98ns[4] = {/*EE_TW_32K_98NS, EE_TW_1M_98NS, */EE_TW_3M_98NS, EE_TW_7M_98NS, EE_TW_14M_98NS, EE_TW_29M_98NS};
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN821x_RN721x_SOC_D) ||\
    defined(RN202x_RN7326_SOC_B)
static const unsigned char fl_tw_198ns[4] = {/*FL_TW_32K_198NS, FL_TW_1M_198NS, */FL_TW_3M_198NS, FL_TW_7M_198NS, FL_TW_14M_198NS, FL_TW_29M_198NS};
#if !defined(RN821x_RN721x_SOC_D) &&\
    !defined(RN202x_RN7326_SOC_B)
static const unsigned char ee_tw_198ns[4] = {/*EE_TW_32K_198NS, EE_TW_1M_198NS, */EE_TW_3M_198NS, EE_TW_7M_198NS, EE_TW_14M_198NS, EE_TW_29M_198NS};
#endif
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
static const unsigned char fl_tw_v2[4] = {/*FL_TW_32K_V2, FL_TW_1M_V2, */FL_TW_3M_V2, FL_TW_7M_V2, FL_TW_14M_V2, FL_TW_29M_V2};
#endif
#if defined(RN_CM0_PLATFORM)
static void setMipTW(unsigned int freq, rn_chipid_t chipid)
#else
static void setMipTW(unsigned int freq)
#endif
{
    volatile unsigned int *fltw_reg_addr;
    const unsigned char *fltw_ptemp;
    unsigned char fltw_value;
    unsigned int temp;
#if defined(RN_CM0_PLATFORM)
    volatile unsigned int *eetw_reg_addr = NULL;
#elif defined(RN821x_RN721x_SOC_B) ||\
      defined(RN821x_RN721x_SOC_C) ||\
      defined(RN831x_RN861x_MCU_V1)
    volatile unsigned int *eetw_reg_addr;
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN831x_RN861x_MCU_V1)
    const unsigned char *eetw_ptemp;
    unsigned char eetw_value;
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
    unsigned int spl_valid;
    unsigned int *spl_base;
    unsigned char fl_tw_spl[6];
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
    unsigned int i;
#endif

#if defined(RN_CM0_PLATFORM)
    if (/*(chipid == CHIP_RA9103A) || */(chipid == CHIP_RA9103D)) {
#else
    {
#endif
#if defined(RN_CM0_PLATFORM)
        fltw_reg_addr = (unsigned int *)(getMipAddr(0U, chipid) + 0x00800018U);
        eetw_reg_addr = (unsigned int *)(getMipAddr(1U, chipid) + 0x00800018U);
        fltw_ptemp = fl_tw_98ns;
        eetw_ptemp = ee_tw_98ns;
#endif
#if defined(RN821x_RN721x_SOC_B)
        fltw_reg_addr = (unsigned int *)(0x00800018U);
        eetw_reg_addr = (unsigned int *)(EEPROM_BASE_ADDRESS + 0x00800018U);
        fltw_ptemp = fl_tw_98ns;
        eetw_ptemp = ee_tw_98ns;
#endif
#if defined(RN_CM0_PLATFORM)
    } else if ((chipid == CHIP_RA9103E) || (chipid == CHIP_RA9103C)) {
#endif
#if defined(RN_CM0_PLATFORM)
        fltw_reg_addr = (unsigned int *)(getMipAddr(0U, chipid) + 0x00800018U);
        eetw_reg_addr = (unsigned int *)(getMipAddr(1U, chipid) + 0x00800018U);
        fltw_ptemp = fl_tw_198ns;
        eetw_ptemp = ee_tw_198ns;
#endif
#if defined(RN821x_RN721x_SOC_C)
        fltw_reg_addr = (unsigned int *)(0x00800018U);
        eetw_reg_addr = (unsigned int *)(EEPROM_BASE_ADDRESS + 0x00800018U);
        fltw_ptemp = fl_tw_198ns;
        eetw_ptemp = ee_tw_198ns;
#endif
#if defined(RN_CM0_PLATFORM)
    } else if (chipid == CHIP_RA9103F) {
#endif
#if defined(RN_CM0_PLATFORM)
        fltw_reg_addr = (unsigned int *)(0x50014040U);
        fltw_ptemp = fl_tw_198ns;
        eetw_ptemp = fl_tw_198ns;
#endif
#if defined(RN831x_RN861x_MCU_V1)
        if (get_mcu_v1_version() == CHIP_RA9103E) {
            fltw_reg_addr = (unsigned int *)(0x00800018U);
            eetw_reg_addr = (unsigned int *)(EEPROM_BASE_ADDRESS + 0x00800018U);
            fltw_ptemp = fl_tw_198ns;
            eetw_ptemp = ee_tw_198ns;
        } else {
            fltw_reg_addr = (unsigned int *)(0x50014040U);
            eetw_reg_addr = NULL;
            fltw_ptemp = fl_tw_198ns;
            eetw_ptemp = fl_tw_198ns;
        }
#endif
#if defined(RN_CM0_PLATFORM)
    } else if (chipid == CHIP_RA9103G) {
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_D)
        fltw_reg_addr = (unsigned int *)(0x00800040U);
        fltw_ptemp = fl_tw_198ns;
#endif
#if defined(RN_CM0_PLATFORM)
        eetw_ptemp = fl_tw_198ns;
#endif
#if defined(RN_CM0_PLATFORM)
    } else if (chipid == CHIP_RA9303B) {
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B)
        fltw_reg_addr = (unsigned int *)(0x50014040U);
        fltw_ptemp = fl_tw_198ns;
#endif
#if defined(RN_CM0_PLATFORM)
        eetw_ptemp = fl_tw_198ns;
#endif
#if defined(RN_CM0_PLATFORM)
    } else/* if ((chipid == CHIP_RA9701A) || (chip_id == CHIP_RA9702A) || (CHIP_RA9703A == chipid) || (CHIP_RA9701B == chipid) || (CHIP_RA9702B == chipid))*/ {
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
        fltw_reg_addr = (unsigned int *)(0x50014014U);
        if (freq == 32768U) {
            /* speed up LOSC */
            spl_valid = 0U;
        } else {
#if defined(RN_CM0_PLATFORM)
            spl_base = (unsigned int *)(getMipAddr(0U, chipid) + (((CHIP_RA9701B == chipid) || (CHIP_RA9702B == chipid)) ? 0x00040000U : 0x00080000U));
#elif defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V3)
            spl_base = (unsigned int *)(0x00040000U);
#else
            spl_base = (unsigned int *)(0x00080000U);
#endif
            spl_valid = check_spl_valid(spl_base, 512U, 0U);
            if (1U == spl_valid) {
                temp = (unsigned int)spl_base;
                temp += SPL_TW_3M_ADDR;
                for (i = 0U; i < 4U; i++) {
                    fl_tw_spl[i] = *(volatile unsigned char *)(temp);
                    if (fl_tw_spl[i] > 3U) {
                        spl_valid = 0U;
                        break;
                    }
                    temp += 2U;
                }
            }
        }
        fltw_ptemp = (1U == spl_valid) ? fl_tw_spl : fl_tw_v2;
#if defined(RN_CM0_PLATFORM)
        eetw_ptemp = (1U == spl_valid) ? fl_tw_spl : fl_tw_v2;
#endif
#endif
    }

    temp = (unsigned int)fltw_ptemp;
    if (freq > 2048000U) {
        switch (freq) {
#if defined(RN_CM0_PLATFORM)
            case 3686400U:
            case 4096000U:
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
            case 4096000U:
#else
            case 3686400U:
#endif
                fltw_value = *(unsigned char *)(temp);
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN831x_RN861x_MCU_V1)
                temp = (unsigned int)eetw_ptemp;
                eetw_value = *(unsigned char *)(temp);
#endif
                break;
#if defined(RN_CM0_PLATFORM)
            case 7372800U:
            case 8192000U:
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
            case 8192000U:
#else
            case 7372800U:
#endif
                temp += 1U;
                fltw_value = *(unsigned char *)(temp);
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN831x_RN861x_MCU_V1)
                temp = (unsigned int)eetw_ptemp;
                temp += 1U;
                eetw_value = *(unsigned char *)(temp);
#endif
                break;
#if defined(RN_CM0_PLATFORM)
            case 14745600U:
            case 16384000U:
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
            case 16384000U:
#else
            case 14745600U:
#endif
                temp += 2U;
                fltw_value = *(unsigned char *)(temp);
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN831x_RN861x_MCU_V1)
                temp = (unsigned int)eetw_ptemp;
                temp += 2U;
                eetw_value = *(unsigned char *)(temp);
#endif
                break;
#if defined(RN_CM0_PLATFORM)
            case 29491200U:
            case 32768000U:
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
            case 32768000U:
#else
            case 29491200U:
#endif
                temp += 3U;
                fltw_value = *(unsigned char *)(temp);
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN831x_RN861x_MCU_V1)
                temp = (unsigned int)eetw_ptemp;
                temp += 3U;
                eetw_value = *(unsigned char *)(temp);
#endif
                break;
            default:
                temp += 3U;
                fltw_value = *(unsigned char *)(temp);
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN831x_RN861x_MCU_V1)
                temp = (unsigned int)eetw_ptemp;
                temp += 3U;
                eetw_value = *(unsigned char *)(temp);
#endif
                break;
        }
    } else {
        fltw_value = 0U;
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN831x_RN861x_MCU_V1)
        eetw_value = 0U;
#endif
    }

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN831x_RN861x_MCU_V1)
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1)
    if (NULL != eetw_reg_addr)
#endif
    {
        *eetw_reg_addr = eetw_value << 16U;
    }
#endif

#if defined(RN_CM0_PLATFORM)
    if (IS_RA97_SERIES(chipid))
#endif
    {
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
        *fltw_reg_addr = (*fltw_reg_addr & (~((unsigned int)0x3U << 2))) | ((fltw_value & (unsigned int)0x3U) << 2);
#endif
#if defined(RN_CM0_PLATFORM)
    } else {
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN821x_RN721x_SOC_D) ||\
    defined(RN202x_RN7326_SOC_B)
        *fltw_reg_addr = fltw_value << 16;
#endif
    }
}

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B) ||\
    defined(RN202x_RN7326_SOC_V2)
#if defined(RN_CM0_PLATFORM)
static void en_rc32m_clk(rn_chipid_t chipid)
#else
static void en_rc32m_clk(void)
#endif
{
#if defined(RN_CM0_PLATFORM)
    MEM32(OSC_CTRL2_ADDR(chipid)) = (MEM32(OSC_CTRL2_ADDR(chipid)) & (~((unsigned int)0x3U << 24))) | ((unsigned int)2U << 24);
    MEM32(OSC_CTRL2_ADDR(chipid)) = (MEM32(OSC_CTRL2_ADDR(chipid)) & (~((unsigned int)0x7U << 21))) | ((unsigned int)0x1U << 21);
    MEM32(OSC_CTRL2_ADDR(chipid)) = (MEM32(OSC_CTRL2_ADDR(chipid)) & (~((unsigned int)0x7U << 21))) | ((unsigned int)0x3U << 21);
    MEM32(OSC_CTRL2_ADDR(chipid)) = (MEM32(OSC_CTRL2_ADDR(chipid)) & (~((unsigned int)0x7U << 21))) | ((unsigned int)0x7U << 21);
    usTimeout(OSC_CTRL2_ADDR(chipid), ((unsigned int)3U << 26), 0U, RCH_TIMEOUT, chipid); /* wait RCM32 flag */
#elif defined(RN202x_RN7326_SOC_B)
    MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x3U << 24))) | ((unsigned int)2U << 24);
    MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x7U << 21))) | ((unsigned int)0x1U << 21);
    MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x7U << 21))) | ((unsigned int)0x3U << 21);
    MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x7U << 21))) | ((unsigned int)0x7U << 21);
    usTimeout(OSC_CTRL2_9303_ADDR, ((unsigned int)3U << 26), 0U, RCH_TIMEOUT); /* wait RCM32 flag */
#elif defined(RN202x_RN7326_SOC_V2)
    MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x3U << 24))) | ((unsigned int)2U << 24);
    MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x7U << 21))) | ((unsigned int)0x1U << 21);
#else
    MEM32(OSC_CTRL2_9103_ADDR) = (MEM32(OSC_CTRL2_9103_ADDR) & (~((unsigned int)0x3U << 24))) | ((unsigned int)2U << 24);
    MEM32(OSC_CTRL2_9103_ADDR) = (MEM32(OSC_CTRL2_9103_ADDR) & (~((unsigned int)0x7U << 21))) | ((unsigned int)0x1U << 21);
    MEM32(OSC_CTRL2_9103_ADDR) = (MEM32(OSC_CTRL2_9103_ADDR) & (~((unsigned int)0x7U << 21))) | ((unsigned int)0x3U << 21);
    MEM32(OSC_CTRL2_9103_ADDR) = (MEM32(OSC_CTRL2_9103_ADDR) & (~((unsigned int)0x7U << 21))) | ((unsigned int)0x7U << 21);
    usTimeout(OSC_CTRL2_9103_ADDR, ((unsigned int)3U << 26), 0U, RCH_TIMEOUT); /* wait RCM32 flag */
#endif
}
#endif

/* use CLK_TRIM to check LOSC */
/* 500ms总时间，每1ms检查losc能否计数超过10个节拍，如是，则认为losc存在，如否，超过500ms总时间后错误返回 */
/* 优化方案：使用LOSC测试RCH
     1ms测试时长，ref_cnt=32，被测时钟RCH离散度按照最低频率计算，即2.5M/25M，再按照10%余量得到calc_cnt=2250/22500
*/
#if defined(RN_CM0_PLATFORM)
#define  RCH_1MS_CNT_2G (1620U)  /* 1.8M */
#define  RCH_1MS_CNT    (2250U)  /* 2.5M */
#define  RCH_1MS_CNT_71 (22500U) /* 25M */
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
#define  RCH_1MS_CNT_2G (1620U)  /* 1.8M */
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
#define  RCH_1MS_CNT_71 (22500U) /* 25M */
#else
#define  RCH_1MS_CNT    (2250U)  /* 2.5M */
#endif

/* 超时按照2倍余量 */
#define  RCH_LOSC_CALC_TIMEOUT  (2000U)
#if defined(RN_CM0_PLATFORM)
static eSysclkRet_TypeDef loscCalc(rn_chipid_t chipid)
#else
static eSysclkRet_TypeDef loscCalc(void)
#endif
{
    unsigned int trim_cfg1_bak/*, sys_ps_bak*/;
    unsigned int temp;
    unsigned int rch_cnt;
    eSysclkRet_TypeDef res = SYSCLK_PASS;
    unsigned char clk_trim_restore = 0;

#if defined(RN_CM0_PLATFORM)
    // sys_ps_bak = MEM32(SYS_PS_ADDR(chipid));
    // MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
    if (((MEM32(TRIM_START_ADDR(chipid)) >> 24) & 0x7U) == 0x0U) {
        clk_trim_restore = 0U;
    } else {
        clk_trim_restore = 1U;
    }
    trim_cfg1_bak = MEM32(TRIM_CFG1_ADDR(chipid));
#elif defined(RN202x_RN7326_SOC_V2)
    // sys_ps_bak = MEM32(SYS_PS_9303_ADDR);
    // MEM32(SYS_PS_9303_ADDR) = 0x82U;
    if (((MEM32(TRIM_START_9303_ADDR) >> 24) & 0x7U) == 0x0U) {
        clk_trim_restore = 0U;
    } else {
        clk_trim_restore = 1U;
    }
    trim_cfg1_bak = MEM32(TRIM_CFG1_9303_ADDR);
#else
    // sys_ps_bak = MEM32(SYS_PS_9103_ADDR);
    // MEM32(SYS_PS_9103_ADDR) = 0x82U;
    if (((MEM32(TRIM_START_9103_ADDR) >> 24) & 0x7U) == 0x0U) {
        clk_trim_restore = 0U;
    } else {
        clk_trim_restore = 1U;
    }
    trim_cfg1_bak = MEM32(TRIM_CFG1_9103_ADDR);
#endif

    /* enable RC to calculate HOSC! */
    // if (1U == ((MEM32(OSC_CTRL1_ADDR) >> 1) & 1U)) {
    RC_ON;
    // }

#if defined(RN_CM0_PLATFORM)
    if (IS_G2_SERIES(chipid)) {
        rch_cnt = RCH_1MS_CNT_2G;  /* 2.048M RCH */
    } else if (IS_RA97_SERIES(chipid)) {
        rch_cnt = RCH_1MS_CNT_71; /* 29.4912M RCH */
    } else {
        rch_cnt = RCH_1MS_CNT;  /* 3.6864M RCH */
    }
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    rch_cnt = RCH_1MS_CNT_2G;  /* 2.048M RCH */
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
    rch_cnt = RCH_1MS_CNT_71; /* 29.4912M RCH */
#else
    rch_cnt = RCH_1MS_CNT;  /* 3.6864M RCH */
#endif

#if defined(RN_CM0_PLATFORM)
    MEM32(TRIM_START_ADDR(chipid)) = (unsigned int)1U << 27;
    MEM32(TRIM_CFG1_ADDR(chipid)) = (unsigned int)32U << 0; /* use losc to calc rch */
    MEM32(TRIM_START_ADDR(chipid)) = (unsigned int)0x7U << 24;
    usTimeout(TRIM_START_ADDR(chipid), ((unsigned int)1U << 24), 0U, RCH_LOSC_CALC_TIMEOUT, chipid);
    temp = MEM32(TRIM_START_ADDR(chipid)) & 0xfffffU;
    if ((((MEM32(TRIM_START_ADDR(chipid)) >> 24) & 0x1U) == 0x0U) ||
        (temp < rch_cnt)){
        res = SYSCLK_FAIL;
    }
    MEM32(TRIM_START_ADDR(chipid)) = (unsigned int)0x1U << 27;
    MEM32(TRIM_START_ADDR(chipid)) = (unsigned int)0x3U << 24;

    if (clk_trim_restore == 1U) {
        MEM32(TRIM_CFG1_ADDR(chipid)) = trim_cfg1_bak;
        MEM32(TRIM_START_ADDR(chipid)) = (unsigned int)1U << 26;
    }
    // MEM32(SYS_PS_ADDR(chipid)) = sys_ps_bak;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    MEM32(TRIM_START_9303_ADDR) = (unsigned int)1U << 27;
    MEM32(TRIM_CFG1_9303_ADDR) = (unsigned int)32U << 0; /* use losc to calc rch */
    MEM32(TRIM_START_9303_ADDR) = (unsigned int)0x7U << 24;
    usTimeout(TRIM_START_9303_ADDR, ((unsigned int)1U << 24), 0U, RCH_LOSC_CALC_TIMEOUT);
    temp = MEM32(TRIM_START_9303_ADDR) & 0xfffffU;
    if ((((MEM32(TRIM_START_9303_ADDR) >> 24) & 0x1U) == 0x0U) ||
        (temp < rch_cnt)){
        res = SYSCLK_FAIL;
    }
    MEM32(TRIM_START_9303_ADDR) = (unsigned int)0x1U << 27;
    MEM32(TRIM_START_9303_ADDR) = (unsigned int)0x3U << 24;

    if (clk_trim_restore == 1U) {
        MEM32(TRIM_CFG1_9303_ADDR) = trim_cfg1_bak;
        MEM32(TRIM_START_9303_ADDR) = (unsigned int)1U << 26;
    }
    // MEM32(SYS_PS_9303_ADDR) = sys_ps_bak;
#else
    MEM32(TRIM_START_9103_ADDR) = (unsigned int)1U << 27;
    MEM32(TRIM_CFG1_9103_ADDR) = (unsigned int)32U << 0; /* use losc to calc rch */
    MEM32(TRIM_START_9103_ADDR) = (unsigned int)0x7U << 24;
    usTimeout(TRIM_START_9103_ADDR, ((unsigned int)1U << 24), 0U, RCH_LOSC_CALC_TIMEOUT);
    temp = MEM32(TRIM_START_9103_ADDR) & 0xfffffU;
    if ((((MEM32(TRIM_START_9103_ADDR) >> 24) & 0x1U) == 0x0U) ||
        (temp < rch_cnt)){
        res = SYSCLK_FAIL;
    }
    MEM32(TRIM_START_9103_ADDR) = (unsigned int)0x1U << 27;
    MEM32(TRIM_START_9103_ADDR) = (unsigned int)0x3U << 24;

    if (clk_trim_restore == 1U) {
        MEM32(TRIM_CFG1_9103_ADDR) = trim_cfg1_bak;
        MEM32(TRIM_START_9103_ADDR) = (unsigned int)1U << 26;
    }
    // MEM32(SYS_PS_9103_ADDR) = sys_ps_bak;
#endif
    return res;
}

#if defined(RN_CM0_PLATFORM)
static eSysclkRet_TypeDef loscCheck(rn_chipid_t chipid)
#else
static eSysclkRet_TypeDef loscCheck(void)
#endif
{
    unsigned int i;
    eSysclkRet_TypeDef res = SYSCLK_FAIL;

    for (i = 0U; i < 500U; i++) {
#if defined(RN_CM0_PLATFORM)
        if (SYSCLK_PASS == loscCalc(chipid))
#else
        if (SYSCLK_PASS == loscCalc())
#endif
        {
            res = SYSCLK_PASS;
            break;
        }
        FEED_WDT;
    }
    return res;
}

/* check: PLL lock and LOSC flag(if LOSC pwr-down, check RTC write) */
#if defined(RN_CM0_PLATFORM)
static eSysclkRet_TypeDef PllEnable(rn_chipid_t chipid)
#else
static eSysclkRet_TypeDef PllEnable(void)
#endif
{
    unsigned int flag, osc1_ctrl_bak, osc2_ctrl_bak/*, sys_ps_bak*/;
    eSysclkRet_TypeDef res = SYSCLK_PASS;

#if defined(RN_CM0_PLATFORM)
    // sys_ps_bak = MEM32(SYS_PS_ADDR(chipid));
    // MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
    osc2_ctrl_bak = MEM32(OSC_CTRL2_ADDR(chipid));
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    // sys_ps_bak = MEM32(SYS_PS_9303_ADDR);
    // MEM32(SYS_PS_9303_ADDR) = 0x82U;
    osc2_ctrl_bak = MEM32(OSC_CTRL2_9303_ADDR);
#else
    // sys_ps_bak = MEM32(SYS_PS_9103_ADDR);
    // MEM32(SYS_PS_9103_ADDR) = 0x82U;
    osc2_ctrl_bak = MEM32(OSC_CTRL2_9103_ADDR);
#endif
    osc1_ctrl_bak = MEM32(OSC_CTRL1_ADDR);

    /* pll as source */
#if defined(RN_CM0_PLATFORM)
    MEM32(OSC_CTRL2_ADDR(chipid)) &= (~((unsigned int)1U << 4));
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    MEM32(OSC_CTRL2_9303_ADDR) &= (~((unsigned int)1U << 4));
#else
    MEM32(OSC_CTRL2_9103_ADDR) &= (~((unsigned int)1U << 4));
#endif
    PLL_ON;
#if defined(RN_CM0_PLATFORM)
    usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 7), 0U, PLL_TIMEOUT, chipid);
    /* delay 1ms here(total 2ms for PLL stable) */
    usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 7), 1U, 2000U, chipid);
#else
    usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 7), 0U, PLL_TIMEOUT);
    /* delay 1ms here(total 2ms for PLL stable) */
    usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 7), 1U, 2000U);
#endif

    if (((MEM32(OSC_CTRL1_ADDR) >> 7) & 0x1U) == 0x0U) {
        res = SYSCLK_FAIL;
    } else {
#if defined(RN_CM0_PLATFORM)
        usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 12), 0U, PLL_TIMEOUT, chipid); /* wait PLL flag*/
#else
        usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 12), 0U, PLL_TIMEOUT); /* wait PLL flag*/
#endif
        flag = MEM32(OSC_CTRL1_ADDR);
        if (((flag >> 12) & 0x1U) == 0x0U) {
            res = SYSCLK_FAIL;
        }
    }

    if (SYSCLK_FAIL == res) {
#if defined(RN_CM0_PLATFORM)
        MEM32(OSC_CTRL2_ADDR(chipid)) = osc2_ctrl_bak;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
        MEM32(OSC_CTRL2_9303_ADDR) = osc2_ctrl_bak;
#else
        MEM32(OSC_CTRL2_9103_ADDR) = osc2_ctrl_bak;
#endif
        MEM32(OSC_CTRL1_ADDR) = osc1_ctrl_bak;
    }

#if defined(RN_CM0_PLATFORM)
    // MEM32(SYS_PS_ADDR(chipid)) = sys_ps_bak;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    // MEM32(SYS_PS_9303_ADDR) = sys_ps_bak;
#else
    // MEM32(SYS_PS_9103_ADDR) = sys_ps_bak;
#endif
    return res;
}

/* 20180205 add */
/*
hosc max freq = 1.5*

RCH          HOSC(7.3728M)             HOSC(14.7456M)      HOSC(29.4912M)
avr 3.132    3.132*7373/2000=11.55M    23.09M              46.18M
max 3.292    12.14M                    24.27M              48.54M
min 2.724    10.04M                    20.08M              40.17M

RA9701A, RCH avr=29.4912, scaler=29.4912/3.132=9
*/
#if defined(RN_CM0_PLATFORM)
#define RCH_MAX_2G  (1617U)  /* 2.048*1.3 */
#define RCH_MAX     (2000U)
#define RCH_MAX_71  (9U*2000U)
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
#define RCH_MAX_2G  (1617U)  /* 2.048*1.3 */
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
#define RCH_MAX_71  (9U*2000U)
#else
#define RCH_MAX     (2000U)
#endif

static const unsigned short HOSC_CYCLES[4] = {7373U, 14746U, 29491U, 29491U};
#if defined(RN_CM0_PLATFORM)
static eSysclkRet_TypeDef HoscValid(rn_chipid_t chipid)
#else
static eSysclkRet_TypeDef HoscValid(void)
#endif
{
    unsigned int trim_cfg1_bak = 0U, sys_ps_bak;
    eSysclkRet_TypeDef res = SYSCLK_PASS;
    unsigned int rch_max;
    unsigned int temp;
    unsigned char clk_trim_restore;

#if defined(RN_CM0_PLATFORM)
    if (IS_G2_SERIES(chipid)) {
        rch_max = RCH_MAX_2G;
    } else if (IS_RA97_SERIES(chipid)) {
        rch_max = RCH_MAX_71;
    } else {
        rch_max = RCH_MAX;
    }
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    rch_max = RCH_MAX_2G;
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
    rch_max = RCH_MAX_71;
#else
    rch_max = RCH_MAX;
#endif

#if defined(RN_CM0_PLATFORM)
    sys_ps_bak = MEM32(SYS_PS_ADDR(chipid));
    MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
    if (((MEM32(TRIM_START_ADDR(chipid)) >> 24) & 0x7U) == 0x0U) {
        clk_trim_restore = 0U;
    } else {
        clk_trim_restore = 1U;
    }
    trim_cfg1_bak = MEM32(TRIM_CFG1_ADDR(chipid));

    MEM32(TRIM_CFG1_ADDR(chipid)) = ((unsigned int)1U << 24) | (HOSC_CYCLES[MEM32(OSC_CTRL2_ADDR(chipid)) & 0x3U]); /* use hosc to calc rch(1ms) */
    MEM32(TRIM_START_ADDR(chipid)) = (unsigned int)1U << 27;
    MEM32(TRIM_START_ADDR(chipid)) = (unsigned int)0x7U << 24;
    usTimeout(TRIM_START_ADDR(chipid), ((unsigned int)1U << 24), 0U, HOSC_CALC_TIMEOUT, chipid); /* 10ms timeout */
    temp = MEM32(TRIM_START_ADDR(chipid));
    if ((((temp >> 24) & 0x1U) == 0x0U) || ((temp & 0xfffffU) < rch_max))
    {
        res = SYSCLK_FAIL;
    }
    MEM32(TRIM_START_ADDR(chipid)) = (unsigned int)0x1U << 27;
    MEM32(TRIM_START_ADDR(chipid)) = (unsigned int)0x3U << 24;

    if (clk_trim_restore == 1U) {
        MEM32(TRIM_CFG1_ADDR(chipid)) = trim_cfg1_bak;
        MEM32(TRIM_START_ADDR(chipid)) = (unsigned int)1U << 26;
    }
    MEM32(SYS_PS_ADDR(chipid)) = sys_ps_bak;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    sys_ps_bak = MEM32(SYS_PS_9303_ADDR);
    MEM32(SYS_PS_9303_ADDR) = 0x82U;
    if (((MEM32(TRIM_START_9303_ADDR) >> 24) & 0x7U) == 0x0U) {
        clk_trim_restore = 0U;
    } else {
        clk_trim_restore = 1U;
        trim_cfg1_bak = MEM32(TRIM_CFG1_9303_ADDR);
    }
    MEM32(TRIM_CFG1_9303_ADDR) = ((unsigned int)1U << 24) | (HOSC_CYCLES[MEM32(OSC_CTRL2_9303_ADDR) & 0x3U]); /* use hosc to calc rch(1ms) */
    MEM32(TRIM_START_9303_ADDR) = (unsigned int)1U << 27;
    MEM32(TRIM_START_9303_ADDR) = (unsigned int)0x7U << 24;
    usTimeout(TRIM_START_9303_ADDR, ((unsigned int)1U << 24), 0U, HOSC_CALC_TIMEOUT); /* 10ms timeout */
    temp = MEM32(TRIM_START_9303_ADDR);
    if ((((temp >> 24) & 0x1U) == 0x0U) || ((temp & 0xfffffU) < rch_max))
    {
        res = SYSCLK_FAIL;
    }
    MEM32(TRIM_START_9303_ADDR) = (unsigned int)0x1U << 27;
    MEM32(TRIM_START_9303_ADDR) = (unsigned int)0x3U << 24;

    if (clk_trim_restore == 1U) {
        MEM32(TRIM_CFG1_9303_ADDR) = trim_cfg1_bak;
        MEM32(TRIM_START_9303_ADDR) = (unsigned int)1U << 26;
    }
    MEM32(SYS_PS_9303_ADDR) = sys_ps_bak;
#else
    sys_ps_bak = MEM32(SYS_PS_9103_ADDR);
    MEM32(SYS_PS_9103_ADDR) = 0x82U;
    if (((MEM32(TRIM_START_9103_ADDR) >> 24) & 0x7U) == 0x0U) {
        clk_trim_restore = 0U;
    } else {
        clk_trim_restore = 1U;
        trim_cfg1_bak = MEM32(TRIM_CFG1_9103_ADDR);
    }

    MEM32(TRIM_CFG1_9103_ADDR) = ((unsigned int)1U << 24) | (HOSC_CYCLES[MEM32(OSC_CTRL2_9103_ADDR) & 0x3U]); /* use hosc to calc rch(1ms) */
    MEM32(TRIM_START_9103_ADDR) = (unsigned int)1U << 27;
    MEM32(TRIM_START_9103_ADDR) = (unsigned int)0x7U << 24;
    usTimeout(TRIM_START_9103_ADDR, ((unsigned int)1U << 24), 0U, HOSC_CALC_TIMEOUT); /* 10ms timeout */
    temp = MEM32(TRIM_START_9103_ADDR);
    if ((((temp >> 24) & 0x1U) == 0x0U) || ((temp & 0xfffffU) < rch_max))
    {
        res = SYSCLK_FAIL;
    }
    MEM32(TRIM_START_9103_ADDR) = (unsigned int)0x1U << 27;
    MEM32(TRIM_START_9103_ADDR) = (unsigned int)0x3U << 24;

    if (clk_trim_restore == 1U) {
        MEM32(TRIM_CFG1_9103_ADDR) = trim_cfg1_bak;
        MEM32(TRIM_START_9103_ADDR) = (unsigned int)1U << 26;
    }
    MEM32(SYS_PS_9103_ADDR) = sys_ps_bak;
#endif
    return res;
}

#if defined(RN_CM0_PLATFORM)
static eSysclkRet_TypeDef HoscEnable(rn_chipid_t chipid)
#else
static eSysclkRet_TypeDef HoscEnable(void)
#endif
{
    unsigned int flag, mod1_en;
    unsigned int osc_ctrl2_bak/*, sys_ps_bak*/;
    eSysclkRet_TypeDef res = SYSCLK_PASS;
#if defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
    unsigned int gpio_psw_bak;
#endif

#if defined(RN_CM0_PLATFORM)
    // sys_ps_bak = MEM32(SYS_PS_ADDR(chipid));
    // MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
    /* enable pll(14M) for rtc in hosc mode */
    MEM32(OSC_CTRL2_ADDR(chipid)) = (MEM32(OSC_CTRL2_ADDR(chipid)) & (~((unsigned int)0x7U << 5))) | ((unsigned int)0x1U << 5);
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    // sys_ps_bak = MEM32(SYS_PS_9303_ADDR);
    // MEM32(SYS_PS_9303_ADDR) = 0x82U;
    /* enable pll(14M) for rtc in hosc mode */
    MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x7U << 5))) | ((unsigned int)0x1U << 5);
#else
    // sys_ps_bak = MEM32(SYS_PS_9103_ADDR);
    // MEM32(SYS_PS_9103_ADDR) = 0x82U;
    /* enable pll(14M) for rtc in hosc mode */
    MEM32(OSC_CTRL2_9103_ADDR) = (MEM32(OSC_CTRL2_9103_ADDR) & (~((unsigned int)0x7U << 5))) | ((unsigned int)0x1U << 5);
#endif
    PLL_ON;

    /* enable RC to calculate HOSC! */
    if (1U == ((MEM32(OSC_CTRL1_ADDR) >> 1) & 1U)) {
        RC_ON;
    }

#if defined(RN_CM0_PLATFORM)
    mod1_en = MEM32(MOD1_EN_ADDR(chipid));
    MEM32(MOD1_EN_ADDR(chipid)) |= ((unsigned int)1U << 5);
    osc_ctrl2_bak = MEM32(OSC_CTRL2_ADDR(chipid));
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    mod1_en = MEM32(MOD1_EN_9303_ADDR);
    MEM32(MOD1_EN_9303_ADDR) |= ((unsigned int)1U << 5);
    osc_ctrl2_bak = MEM32(OSC_CTRL2_9303_ADDR);
#else
    mod1_en = MEM32(MOD1_EN_9103_ADDR);
    MEM32(MOD1_EN_9103_ADDR) |= ((unsigned int)1U << 5);
    osc_ctrl2_bak = MEM32(OSC_CTRL2_9103_ADDR);
#endif
#if defined(RN_CM0_PLATFORM)
    if (CHIP_RA9703A == chipid) {
#endif
#if defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
        gpio_psw_bak = MEM32(GPIO_PSW0_ADDR);
        MEM32(GPIO_PSW0_ADDR) = 0x82U;
        MEM32(GPIO_PCA0_ADDR) |= ((unsigned int)0xaU << 25);
        MEM32(GPIO_PSW0_ADDR) = gpio_psw_bak;
#endif
#if defined(RN_CM0_PLATFORM)
    } else {
#endif
#if !defined(RN202x_RN7326_SOC_V2) &&\
    !defined(RN202x_RN7326_SOC_B)
        MEM32(GPIO_PCA1_ADDR) |= ((unsigned int)0xfU << 12);
#endif
#if defined(RN_CM0_PLATFORM)
    }
#endif
#if defined(RN_CM0_PLATFORM)
    HOSC_ON(chipid);
#else
    HOSC_ON();
#endif
#if defined(RN_CM0_PLATFORM)
    MEM32(OSC_CTRL2_ADDR(chipid)) |= ((unsigned int)1U << 4);          /* hosc as source */
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    MEM32(OSC_CTRL2_9303_ADDR) |= ((unsigned int)1U << 4);          /* hosc as source */
#else
    MEM32(OSC_CTRL2_9103_ADDR) |= ((unsigned int)1U << 4);          /* hosc as source */
#endif
#if defined(RN_CM0_PLATFORM)
    usTimeout(OSC_CTRL1_ADDR, (unsigned int)1U << 15, 0U, HOSC_TIMEOUT, chipid); /* wait HOSC flag */
#else
    usTimeout(OSC_CTRL1_ADDR, (unsigned int)1U << 15, 0U, HOSC_TIMEOUT); /* wait HOSC flag */
#endif


    flag = MEM32(OSC_CTRL1_ADDR);
    if (((flag >> 15) & 0x1U) == 0x0U) {
        /* only check HOSC flag(pll bypass) */
#if defined(RN_CM0_PLATFORM)
        MEM32(OSC_CTRL2_ADDR(chipid)) = osc_ctrl2_bak;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
        MEM32(OSC_CTRL2_9303_ADDR) = osc_ctrl2_bak;
#else
        MEM32(OSC_CTRL2_9103_ADDR) = osc_ctrl2_bak;
#endif
        res = SYSCLK_FAIL;
    }

    if (SYSCLK_PASS == res) {
#if defined(RN_CM0_PLATFORM)
        usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 15), 1U, 10000U, chipid); /* delay 5ms for hosc stable */
        res = HoscValid(chipid);                                            /* measure hosc with rch */
#else
        usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 15), 1U, 10000U); /* delay 5ms for hosc stable */
        res = HoscValid();                                            /* measure hosc with rch */
#endif
    }

#if defined(RN_CM0_PLATFORM)
    MEM32(MOD1_EN_ADDR(chipid)) = mod1_en;
    // MEM32(SYS_PS_ADDR(chipid)) = sys_ps_bak;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    MEM32(MOD1_EN_9303_ADDR) = mod1_en;
    // MEM32(SYS_PS_9303_ADDR) = sys_ps_bak;
#else
    MEM32(MOD1_EN_9103_ADDR) = mod1_en;
    // MEM32(SYS_PS_9103_ADDR) = sys_ps_bak;
#endif

    return res;
}

/**
 * @brief check if target clock is valid(stable)
 *  Don't need add clock wait time after called this function
 * @param mode: desired system mode
 * @param chipid: chip index
 * @return unsigned char: 0 = PASS, 1 = FAIL
 */
#if defined(RN_CM0_PLATFORM)
static eSysclkRet_TypeDef clock_valid_check(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div, rn_chipid_t chipid)
#else
static eSysclkRet_TypeDef clock_valid_check(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div)
#endif
{
    eSysclkRet_TypeDef res = SYSCLK_PASS;
    unsigned int temp;

    if (IS_LC_MODE(mode) /* && (SYSCLK_FAIL == loscCheck(chipid))*/) {
        /* 20180404 skip loscCheck while switching to RCL */
        if (Clock_Losc == mode) {
#if defined(RN_CM0_PLATFORM)
            res = loscCheck(chipid);
#else
            res = loscCheck();
#endif
#if defined(RN_CM0_PLATFORM)
            if ((((MEM32(OSC_CTRL2_ADDR(chipid)) >> 12) & 0x1U) == 0U) && (SYSCLK_FAIL == res)) {
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
            if ((((MEM32(OSC_CTRL2_9303_ADDR) >> 12) & 0x1U) == 0U) && (SYSCLK_FAIL == res)) {
#else
            if ((((MEM32(OSC_CTRL2_9103_ADDR) >> 12) & 0x1U) == 0U) && (SYSCLK_FAIL == res)) {
#endif
                res = SYSCLK_FAIL;
            }
        }
    }
    else if (IS_RC_MODE(mode)) {
        if (Clock_RC_1M8 == mode) {
#if defined(RN_CM0_PLATFORM)
            if (IS_G2_SERIES(chipid)) {
                MEM32(OSC_CTRL2_ADDR(chipid)) &= (~((unsigned int)0x1U << 16));
            } else {
                MEM32(OSC_CTRL2_ADDR(chipid)) &= (~((unsigned int)0x7U << 16));
            }
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
            MEM32(OSC_CTRL2_9303_ADDR) &= (~((unsigned int)0x1 << 16));
#else
            MEM32(OSC_CTRL2_9103_ADDR) &= (~((unsigned int)0x7 << 16));
#endif
            RC_ON;
            /* wait RCH flag */
#if defined(RN_CM0_PLATFORM)
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 13), 0U, RCH_TIMEOUT, chipid);
#else
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 13), 0U, RCH_TIMEOUT);
#endif
            if (((MEM32(OSC_CTRL1_ADDR) >> 13) & 0x1U) == 0x0U) {
                res = SYSCLK_FAIL;
            }
        } else/* if (Clock_RC_29M5 == mode)*/ {
#if defined(RN_CM0_PLATFORM)
            if (IS_G2_SERIES(chipid)) {
#endif
#if defined(RN_CM0_PLATFORM)
                en_rc32m_clk(chipid);
                if((MEM32(OSC_CTRL2_ADDR(chipid)) & ((unsigned int)3U << 26)) != ((unsigned int)3U << 26)) {
                    res = SYSCLK_FAIL;
                }
                /* wait RCH flag */
                usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 16), 0U, RCH_TIMEOUT, chipid);
                if (((MEM32(OSC_CTRL1_ADDR) >> 16) & 0x1U) == 0x0U) {
                    res = SYSCLK_FAIL;
                }
#elif defined(RN202x_RN7326_SOC_B)
            MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x3U << 24))) | ((unsigned int)2U << 24); /* always set  RC32M frequecy = 32M */
            MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x7U << 21))) | ((unsigned int)0x1U << 21);
            MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x7U << 21))) | ((unsigned int)0x3U << 21);
            MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x7U << 21))) | ((unsigned int)0x7U << 21);
            usTimeout(OSC_CTRL2_9303_ADDR, ((unsigned int)3U << 26), 0U, RCH_TIMEOUT); /* wait RCM32 flag */
            if((MEM32(OSC_CTRL2_9303_ADDR) & ((unsigned int)3U << 26)) != ((unsigned int)3U << 26)) {
                res = SYSCLK_FAIL;
            }
            /* wait RCH flag */
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 16), 0U, RCH_TIMEOUT);
            if (((MEM32(OSC_CTRL1_ADDR) >> 16) & 0x1U) == 0x0U) {
                res = SYSCLK_FAIL;
            }
#elif defined(RN202x_RN7326_SOC_V2)
            MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x3U << 24))) | ((unsigned int)2U << 24); /* always set  RC32M frequecy = 32M */
            MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x7U << 21))) | ((unsigned int)0x1U << 21);
            if((MEM32(OSC_CTRL2_9303_ADDR) & ((unsigned int)3U << 26)) != ((unsigned int)3U << 26)) {
                res = SYSCLK_FAIL;
            }
            /* wait RCH flag */
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 16), 0U, RCH_TIMEOUT);
            if (((MEM32(OSC_CTRL1_ADDR) >> 16) & 0x1U) == 0x0U) {
                res = SYSCLK_FAIL;
            }
#endif
#if defined(RN_CM0_PLATFORM)
            } else {
#endif
#if defined(RN_CM0_PLATFORM)
                temp = (unsigned int)clk_div;
                temp = 4U - temp;
                MEM32(OSC_CTRL2_ADDR(chipid)) = (MEM32(OSC_CTRL2_ADDR(chipid)) & (~((unsigned int)0x7U << 16))) | (temp << 16);
                RC_ON;
                /* wait RCH flag */
                usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 13), 0U, RCH_TIMEOUT, chipid);
                if (((MEM32(OSC_CTRL1_ADDR) >> 13) & 0x1U) == 0x0U) {
                    res = SYSCLK_FAIL;
                }
#elif !defined(RN202x_RN7326_SOC_V2) &&\
      !defined(RN202x_RN7326_SOC_B)
            temp = (unsigned int)clk_div;
            temp = 4U - temp;
            MEM32(OSC_CTRL2_9103_ADDR) = (MEM32(OSC_CTRL2_9103_ADDR) & (~((unsigned int)0x7U << 16))) | (temp << 16);
            RC_ON;
            /* wait RCH flag */
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 13), 0U, RCH_TIMEOUT);
            if (((MEM32(OSC_CTRL1_ADDR) >> 13) & 0x1U) == 0x0U) {
                res = SYSCLK_FAIL;
            }
#endif
#if defined(RN_CM0_PLATFORM)
            }
#endif
        }
    }
    else if (IS_PLL_MODE(mode)) {
        if (Clock_PLL_7M4 == mode) {
            temp = 0U;
        } else if (Clock_PLL_14M7 == mode) {
            temp = 1U;
        } else if (Clock_PLL_29M5 == mode) {
            temp = 2U;
        }
        else/* if (Clock_PLL_29M5 == mode)*/ {
            temp = 3U;
        }
#if defined(RN_CM0_PLATFORM)
        MEM32(OSC_CTRL2_ADDR(chipid)) = (MEM32(OSC_CTRL2_ADDR(chipid)) & (~((unsigned int)0x7U << 5))) | (temp << 5);
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
        MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x7U << 5))) | (temp << 5);
#else
        MEM32(OSC_CTRL2_9103_ADDR) = (MEM32(OSC_CTRL2_9103_ADDR) & (~((unsigned int)0x7U << 5))) | (temp << 5);
#endif
#if defined(RN_CM0_PLATFORM)
        res = loscCheck(chipid);
        if (SYSCLK_PASS == res) {
            res = PllEnable(chipid);
        }
#else
        res = loscCheck();
        if (SYSCLK_PASS == res) {
            res = PllEnable();
        }
#endif
    }
    else/* if (IS_HOSC_MODE(mode))*/ {
#if defined(RN_CM0_PLATFORM)
        res = HoscEnable(chipid);
#else
        res = HoscEnable();
#endif
    }
    return res;
}

#if defined(RN_CM0_PLATFORM)
static unsigned int get_target_freq(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div, rn_chipid_t chipid)
#else
static unsigned int get_target_freq(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div)
#endif
{
    unsigned int freq;
    if ((Clock_Losc == mode) || (Clock_RCL_32K == mode)) {
        freq = 0U;
    } else if (Clock_RC_1M8 == mode) {
        freq = 1U;
    } else if (Clock_PLL_7M4 == mode) {
        freq = 3U - (unsigned int)clk_div;
    } else if (Clock_Hosc == mode) {
#if defined(RN_CM0_PLATFORM)
        freq = MEM32(OSC_CTRL2_ADDR(chipid)) & 0x3U;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
        freq = MEM32(OSC_CTRL2_9303_ADDR) & 0x3U;
#else
        freq = MEM32(OSC_CTRL2_9103_ADDR) & 0x3U;
#endif
        if (freq == 0U) {
            freq = 3U;
        } else if (freq == 1U) {
            freq = 4U;
        } else {
            freq = 5U;
        }
        freq = freq - (unsigned int)clk_div;
    } else if (Clock_PLL_14M7 == mode) {
        freq = 4U - (unsigned int)clk_div;
    } else if ((Clock_RC_29M5 == mode) || ((Clock_PLL_29M5 == mode))|| (Clock_PLL_58M9_DIV2 == mode)) {
        freq = 5U - (unsigned int)clk_div;
    } else {
        freq = 1U;
    }

#if defined(RN_CM0_PLATFORM)
    return (freq == 0U) ? 32768U : ((((CHIP_RA9303B == chipid) || (CHIP_RA9703A == chipid)) ? 2048000U : 1843200U) << ((freq - 1U) & 0x7U));
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    return (freq == 0U) ? 32768U : (2048000U << ((freq - 1U) & 0x7U));
#else
    return (freq == 0U) ? 32768U : (1843200U << ((freq - 1U) & 0x7U));
#endif
}

/**
 * @brief Set the ldo mode
 *
 * @param ldo_mode: 0 = light-load, 1 = high-load, 2 = safe-load mode
 * @return unsigned char: 0 = PASS, 1 = FAIL
 */
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_D) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
#if defined(RN_CM0_PLATFORM)
static void set_ldo_mode(ldo_mode_t ldo_mode, rn_chipid_t chipid)
#else
static void set_ldo_mode(ldo_mode_t ldo_mode)
#endif
{
#if defined(RN_CM0_PLATFORM)
    const unsigned int sys_ps_bak = MEM32(SYS_PS_ADDR(chipid));
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    const unsigned int sys_ps_bak = MEM32(SYS_PS_9303_ADDR);
#else
    const unsigned int sys_ps_bak = MEM32(SYS_PS_9103_ADDR);
#endif
#if !defined(RN821x_RN721x_SOC_B) &&\
    !defined(RN821x_RN721x_SOC_C) &&\
    !defined(RN831x_RN861x_MCU_V1)
    const unsigned char ldo_set = (ldo_mode == LIGHT_MODE) ? 0x69U : ((ldo_mode == HIGH_MODE) ? 0x96U : 0x0U);
#endif

#if defined(RN_CM0_PLATFORM)
    MEM32(SYS_PS_ADDR(chipid)) = 0x75U;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    MEM32(SYS_PS_9303_ADDR) = 0x75U;
#else
    MEM32(SYS_PS_9103_ADDR) = 0x75U;
#endif

#if defined(RN_CM0_PLATFORM)
    open_dig5(chipid);
#else
    open_dig5();
#endif

#if defined(RN_CM0_PLATFORM)
    if (chipid > CHIP_RA9103F) {
        MEM32(ANA_LDOMOD_ADDR(chipid)) = (MEM32(ANA_LDOMOD_ADDR(chipid)) & (~((unsigned int)0xffU << 0))) | (0xa5650000U | ldo_set);
        MEM32(ANA_LDO_ADDR(chipid)) = (MEM32(ANA_LDO_ADDR(chipid)) & (~((unsigned int)0xffU << 0))) | (0xa5580000U | ldo_set);
    }
#elif defined(RN821x_RN721x_SOC_D)
    MEM32(ANA_LDOMOD_9103_ADDR) = 0xa5580000U | ldo_set;
#elif defined(RN831x_RN861x_MCU_V2)
    MEM32(ANA_LDO_9103_ADDR) = (MEM32(ANA_LDO_9103_ADDR) & (~((unsigned int)0xffU << 0))) | (0xa5580000U | ldo_set);
#elif defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
    MEM32(ANA_LDO_9103_ADDR) = (MEM32(ANA_LDO_9103_ADDR) & (~((unsigned int)0xffU << 0))) | (0xa5580000U | ldo_set);
    MEM32(ANA_LDOMOD_9701_ADDR) = 0xa5650000U | ldo_set;
#elif defined(RN202x_RN7326_SOC_V2)
    MEM32(ANA_LDO_9103_ADDR) = (MEM32(ANA_LDO_9103_ADDR) & (~((unsigned int)0xffU << 0))) | (0xa5580000U | ldo_set);
    MEM32(ANA_LDOMOD_9703_ADDR) = 0xa5650000U | ldo_set;
#elif defined(RN202x_RN7326_SOC_B)
    MEM32(ANA_LDO_9303_ADDR) = (MEM32(ANA_LDO_9303_ADDR) & (~((unsigned int)0xffU << 0))) | (0xa5580000U | ldo_set);
#endif
#if defined(RN_CM0_PLATFORM)
    close_dig5(chipid);
#else
    close_dig5();
#endif
#if defined(RN_CM0_PLATFORM)
    MEM32(SYS_PS_ADDR(chipid)) = sys_ps_bak;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    MEM32(SYS_PS_9303_ADDR) = sys_ps_bak;
#else
    MEM32(SYS_PS_9103_ADDR) = sys_ps_bak;
#endif
}
#endif

/**
 * @brief system run mode set
 *
 * @param mode: target system mode value:
 *   0 = LC, 1/4 = RC, 2/5 = PLL, 3 = HOSC
 * @param chipid: chip id index
 * @param cur_freq: target system frequency value
 * @return unsigned char: 0 = PASS, 1 = FAIL
 */
#if defined(RN_CM0_PLATFORM)
static eSysclkRet_TypeDef sysmodeWrite(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div, rn_chipid_t chipid)
#else
static eSysclkRet_TypeDef sysmodeWrite(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div)
#endif
{
#if defined(RN_CM0_PLATFORM)
    const unsigned int tar_freq = get_target_freq(mode, clk_div, chipid);
    const unsigned int cur_freq = currentClk(chipid);
#else
    const unsigned int tar_freq = get_target_freq(mode, clk_div);
    const unsigned int cur_freq = currentClk();
#endif
    const unsigned int temp = (unsigned int)clk_div;
    eSysclkRet_TypeDef res = SYSCLK_PASS;

    if (tar_freq > cur_freq) {
#if defined(RN_CM0_PLATFORM)
        setMipTW(tar_freq, chipid);
#else
        setMipTW(tar_freq);
#endif
    }

#if defined(RN_CM0_PLATFORM)
    if (SYSCLK_FAIL == clock_valid_check(mode, clk_div, chipid)) {
#else
    if (SYSCLK_FAIL == clock_valid_check(mode, clk_div)) {
#endif
        res = SYSCLK_FAIL;
    } else {
        /* wait until flash not busy */
#if defined(RN_CM0_PLATFORM)
        usTimeout(SYS_MODE_ADDR, 0x10U, 1U, MIP_TIMEOUT, chipid);
        usTimeout(SYS_MODE_ADDR, 0x20U, 1U, MIP_TIMEOUT, chipid);
#else
        usTimeout(SYS_MODE_ADDR, 0x10U, 1U, MIP_TIMEOUT);
        usTimeout(SYS_MODE_ADDR, 0x20U, 1U, MIP_TIMEOUT);
#endif

        if (IS_LC_MODE(mode)) {
            MEM32(SYS_MODE_ADDR) = WR_LC_MODE;
            /* @20180504 wait clk sync before changing TW */
#if defined(RN_CM0_PLATFORM)
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 4), 0U, LOSC_TIMEOUT, chipid);
#else
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 4), 0U, LOSC_TIMEOUT);
#endif
        }
        else if (IS_RCM_MODE(mode)) {
            MEM32(SYS_MODE_ADDR) = WR_RC_MODE;
            /* @20180504 wait clk sync before changing TW */
#if defined(RN_CM0_PLATFORM)
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 5), 0U, LOSC_TIMEOUT, chipid);
#else
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 5), 0U, LOSC_TIMEOUT);
#endif
        } else if (IS_RC29_MODE(mode)) {
#if defined(RN_CM0_PLATFORM)
            if (IS_G2_SERIES(chipid)) {
                MEM32(OSC_CTRL1_ADDR) = (MEM32(OSC_CTRL1_ADDR) & (~((unsigned int)0x3U << 2))) | (temp << 2);
                MEM32(OSC_CTRL2_9303_ADDR) &= (~((unsigned int)1U << 4));
                MEM32(OSC_CTRL2_9303_ADDR) |= ((unsigned int)1U << 20);
                MEM32(SYS_MODE_ADDR) = WR_HC_MODE;
                usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 6), 0U, LOSC_TIMEOUT, chipid);
            } else {
                MEM32(SYS_MODE_ADDR) = WR_RC_MODE;
                /* @20180504 wait clk sync before changing TW */
                usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 5), 0U, LOSC_TIMEOUT, chipid);
            }
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
            MEM32(OSC_CTRL1_ADDR) = (MEM32(OSC_CTRL1_ADDR) & (~((unsigned int)0x3U << 2))) | (temp << 2);
            MEM32(OSC_CTRL2_9303_ADDR) &= (~((unsigned int)1U << 4));
            MEM32(OSC_CTRL2_9303_ADDR) |= ((unsigned int)1U << 20);
            MEM32(SYS_MODE_ADDR) = WR_HC_MODE;
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 6), 0U, LOSC_TIMEOUT);
#else
            MEM32(SYS_MODE_ADDR) = WR_RC_MODE;
            /* @20180504 wait clk sync before changing TW */
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 5), 0U, LOSC_TIMEOUT);
#endif
        }
        else if (IS_PLL_MODE(mode)) {
            MEM32(OSC_CTRL1_ADDR) = (MEM32(OSC_CTRL1_ADDR) & (~((unsigned int)0x3U << 2))) | (temp << 2);
            MEM32(SYS_MODE_ADDR) = WR_HC_MODE;
#if defined(RN_CM0_PLATFORM)
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 6), 0U, LOSC_TIMEOUT, chipid);
#else
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 6), 0U, LOSC_TIMEOUT);
#endif
        }
        else if (IS_HOSC_MODE(mode)) {
            MEM32(OSC_CTRL1_ADDR) = (MEM32(OSC_CTRL1_ADDR) & (~((unsigned int)0x3U << 2))) | (temp << 2);
            MEM32(SYS_MODE_ADDR) = WR_HC_MODE;
#if defined(RN_CM0_PLATFORM)
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 6), 0U, LOSC_TIMEOUT, chipid);
#else
            usTimeout(OSC_CTRL1_ADDR, ((unsigned int)1U << 6), 0U, LOSC_TIMEOUT);
#endif
        }
        else {
            res = SYSCLK_FAIL;
        }

        if ((SYSCLK_PASS == res) && (tar_freq < cur_freq)) {
#if defined(RN_CM0_PLATFORM)
            setMipTW(tar_freq, chipid);
#else
            setMipTW(tar_freq);
#endif
        }
    }

    return res;
}

/**
 * @brief system run mode parameters check
 * @param mode: target system mode value:
 *   0 = LC, 8 = RCL
 *   1 = RC(1.8432M), 4 = RC(3.6864M), 6 = RC(29.4912M)
 *   2 = PLL(7.3728M), 5 = PLL(14.7456M), 7 = PLL(29.4912M)
 *   3 = HOSC(depends on OSC_CTRL2_ADDR[2:0])
 *
 *   RA9103A/D support mode=[0-3]
 *   RA9103C/G/E/F support mode=[0-5]
 *   RA9701 supports mode=[0-9]
 * @param clk_div: clock clk_div(only for HOSC/PLL mode)
 * @param chipid: chip index
 * @return unsigned char: 0 = PASS, 1 = FAIL
 */
#if defined(RN_CM0_PLATFORM)
static eSysclkRet_TypeDef sysmode_params_check(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div, rn_chipid_t chipid)
#else
static eSysclkRet_TypeDef sysmode_params_check(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef clk_div)
#endif
{
    eSysclkRet_TypeDef res;
#if defined(RN_CM0_PLATFORM)
    const unsigned int temp = MEM32(OSC_CTRL2_ADDR(chipid)) & 0x7U;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    const unsigned int temp = MEM32(OSC_CTRL2_9303_ADDR) & 0x7U;
#else
    const unsigned int temp = MEM32(OSC_CTRL2_9103_ADDR) & 0x7U;
#endif
    /* speed up losc */
    if (((Clock_Losc == mode) || (Clock_RCL_32K == mode)) && (Clock_Div_1 == clk_div))
    {
        res = SYSCLK_PASS;
    }
    else if ((((Clock_Losc == mode) || (Clock_RC_1M8 == mode)) && (clk_div > Clock_Div_1)) ||
             ((Clock_PLL_7M4 == mode) && (clk_div > Clock_Div_4)) ||
             ((Clock_Hosc == mode) && (clk_div >= Clock_Div_MAX)) ||
             ((Clock_Hosc == mode) && (Clock_Div_8 == clk_div) && (temp == 0x0U)) ||
             (4U == (unsigned int)mode))
    {
        res = SYSCLK_FAIL;
    }
#if defined(RN_CM0_PLATFORM)
    else if (/*(CHIP_RA9103A == chipid) || */(CHIP_RA9103D == chipid))
#elif defined(RN821x_RN721x_SOC_B)
    else
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B)
    {
        if (mode > Clock_Hosc) {
            res = SYSCLK_FAIL;
        } else {
            res = SYSCLK_PASS;
        }
    }
#endif
#if defined(RN_CM0_PLATFORM)
    else if ((CHIP_RA9103C == chipid) ||
             (CHIP_RA9103E == chipid) ||
             (CHIP_RA9103F == chipid) ||
             (CHIP_RA9103G == chipid))
#elif defined(RN821x_RN721x_SOC_C) ||\
      defined(RN821x_RN721x_SOC_D) ||\
      defined(RN831x_RN861x_MCU_V1)
    else
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN821x_RN721x_SOC_D) ||\
    defined(RN831x_RN861x_MCU_V1)
    {
        if (((Clock_PLL_14M7 == mode) && (clk_div >= Clock_Div_MAX)) || (mode > Clock_PLL_14M7))
        {
            res = SYSCLK_FAIL;
        } else {
            res = SYSCLK_PASS;
        }
    }
#endif
#if defined(RN_CM0_PLATFORM)
    else if (CHIP_RA9702B == chipid)
#elif defined(RN821x_RN721x_SOC_V3)
    else
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_V3)
    {
        if ((Clock_Hosc == mode) ||
            (((Clock_RCL_32K == mode)) && (clk_div > Clock_Div_1)) ||
            (((Clock_PLL_14M7 == mode) || (Clock_RC_29M5 == mode) || (Clock_PLL_29M5 == mode) || (Clock_PLL_58M9_DIV2 == mode))&& (clk_div >= Clock_Div_MAX)) ||
            (mode >= Clock_CHG_MAX))
        {
            res = SYSCLK_FAIL;
        }
        else {
            res = SYSCLK_PASS;
        }
    }
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
    else/* if ((CHIP_RA9701A == chipid) || (CHIP_RA9702A == chipid) || (CHIP_RA9703A == chipid) || (CHIP_RA9701B == chipid) || (CHIP_RA9702B == chipid))*/ {
        if ((((Clock_RCL_32K == mode)) && (clk_div > Clock_Div_1)) ||
            (((Clock_PLL_14M7 == mode) || (Clock_RC_29M5 == mode) || (Clock_PLL_29M5 == mode) || (Clock_PLL_58M9_DIV2 == mode))&& (clk_div >= Clock_Div_MAX)) ||
            (mode >= Clock_CHG_MAX))
        {
            res = SYSCLK_FAIL;
        }
        else {
            res = SYSCLK_PASS;
        }
    }
#endif
    return res;
}
#endif
#endif /* RN831x_RN861x_MCU_V4 */

/**
 * @brief system run mode change
 * @param mode: target system mode value:
 *   0 = LC, 8 = RCL
 *   1 = RC(1.8432M),  6 = RCH(29.4912M)
 *   2 = PLL(7.3728M), 5 = PLL(14.7456M), 7 = PLL(29.4912M)
 *   3 = HOSC(depends on OSC_CTRL2_ADDR[2:0])
 *
 *   V1-A/B support mode=[0-3]
 *   V1-C/D support mode=[0-3, 5]
 *   V2 supports mode=[0-3, 5-8]
 * @param cdiv: clock clk_div(only for HOSC/PLL/RCH mode)
 * @return unsigned char: 0 = PASS, 1 = FAIL
 */
eSysclkRet_TypeDef LL_SYSCLK_SysModeChg(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef cdiv)
{
#if !defined(RN831x_RN861x_MCU_V4)
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSCLK_SysModeChg(mode, cdiv);
#else
    unsigned int sys_ps;
    eSysclkRet_TypeDef res = SYSCLK_PASS;
    unsigned int temp;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
    unsigned char is_g2_chips;

    if (IS_G2_SERIES(chipid)) {
        is_g2_chips = 1U;
    } else {
        is_g2_chips = 0U;
    }
#endif

#if defined(RN_CM0_PLATFORM)
    if (SYSCLK_FAIL == sysmode_params_check(mode, cdiv, chipid))
#else
    if (SYSCLK_FAIL == sysmode_params_check(mode, cdiv))
#endif
    {
        res = SYSCLK_FAIL;
    } else {
#if defined(RN_CM0_PLATFORM)
        sys_ps = MEM32(SYS_PS_ADDR(chipid));
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
        sys_ps = MEM32(SYS_PS_9303_ADDR);
#else
        sys_ps = MEM32(SYS_PS_9103_ADDR);
#endif
        if (((MEM32(OSC_CTRL1_ADDR) >> 4) & 0x1U) == 0x1U) {
            /* exit light-mode of ldo(enter safe-mode) if current mode is LC! */
#if defined(RN_CM0_PLATFORM)
            set_ldo_mode(SAFE_MODE, chipid);
#elif defined(RN821x_RN721x_SOC_D) ||\
      defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
            set_ldo_mode(SAFE_MODE);
#endif
        }
#if defined(RN_CM0_PLATFORM)
        MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
        /* always enable SYS_TC/RTC/WDT apb clock */
        MEM32(MOD1_EN_ADDR(chipid)) |= (((unsigned int)1U << 10) | ((unsigned int)1U << 9));
        MEM32(MOD0_EN_ADDR(chipid)) |= ((unsigned int)1U << 0);
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
        MEM32(SYS_PS_9303_ADDR) = 0x82U;
        /* always enable SYS_TC/RTC/WDT apb clock */
        MEM32(MOD1_EN_9303_ADDR) |= (((unsigned int)1U << 10) | ((unsigned int)1U << 9));
        MEM32(MOD0_EN_9303_ADDR) |= ((unsigned int)1U << 0);
#else
        MEM32(SYS_PS_9103_ADDR) = 0x82U;
        /* always enable SYS_TC/RTC/WDT apb clock */
        MEM32(MOD1_EN_9103_ADDR) |= (((unsigned int)1U << 10) | ((unsigned int)1U << 9));
        MEM32(MOD0_EN_9103_ADDR) |= ((unsigned int)1U << 0);
#endif

        if (IS_LC_MODE(mode)) {
            if (1U == ((MEM32(OSC_CTRL1_ADDR) >> 4) & 0x1U)) {
#if defined(RN_CM0_PLATFORM)
                temp = (MEM32(OSC_CTRL2_ADDR(chipid)) >> 12) & 0x1U;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
                temp = (MEM32(OSC_CTRL2_9303_ADDR) >> 12) & 0x1U;
#else
                temp = (MEM32(OSC_CTRL2_9103_ADDR) >> 12) & 0x1U;
#endif
                if (((mode == Clock_RCL_32K) && (0U == temp))
                || ((mode == Clock_Losc) && (1U == temp))) {
                    /* switch to RC firstly if current mode and target mode are diffrent LC mode */
#if defined(RN_CM0_PLATFORM)
                    res = sysmodeWrite(Clock_RC_1M8, Clock_Div_1, chipid);
#else
                    res = sysmodeWrite(Clock_RC_1M8, Clock_Div_1);
#endif
                }
            }

            if (SYSCLK_PASS == res) {
#if defined(RN_CM0_PLATFORM)
                if (Clock_RCL_32K == mode) {
                    MEM32(OSC_CTRL2_ADDR(chipid)) |= ((unsigned int)1U << 12);
                } else {
                    MEM32(OSC_CTRL2_ADDR(chipid)) &= (~((unsigned int)1U << 12));
                }
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
                if (Clock_RCL_32K == mode) {
                    MEM32(OSC_CTRL2_9303_ADDR) |= ((unsigned int)1U << 12);
                } else {
                    MEM32(OSC_CTRL2_9303_ADDR) &= (~((unsigned int)1U << 12));
                }
#else
                if (Clock_RCL_32K == mode) {
                    MEM32(OSC_CTRL2_9103_ADDR) |= ((unsigned int)1U << 12);
                } else {
                    MEM32(OSC_CTRL2_9103_ADDR) &= (~((unsigned int)1U << 12));
                }
#endif
#if defined(RN_CM0_PLATFORM)
                res = sysmodeWrite(mode, cdiv, chipid);
#else
                res = sysmodeWrite(mode, cdiv);
#endif
            }
            if (SYSCLK_FAIL == res) {
                res = SYSCLK_FAIL;
            } else {
                /* close RC&PLL&HOSC */
                RCM_OFF;
                PLL_OFF;
#if defined(RN_CM0_PLATFORM)
                HOSC_OFF(chipid);
#else
                HOSC_OFF();
#endif

#if defined(RN_CM0_PLATFORM)
                if (is_g2_chips == 1U) {
                    RC32M_OFF;
                }
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
                RC32M_OFF;
#endif

#if defined(RN_CM0_PLATFORM)
                set_ldo_mode(LIGHT_MODE, chipid);
#elif defined(RN821x_RN721x_SOC_D) ||\
      defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
                set_ldo_mode(LIGHT_MODE);
#endif
            }
        } else if (IS_RCM_MODE(mode)) {
#if defined(RN_CM0_PLATFORM)
            if (SYSCLK_FAIL == sysmodeWrite(mode, cdiv, chipid)) {
#else
            if (SYSCLK_FAIL == sysmodeWrite(mode, cdiv)) {
#endif
                res = SYSCLK_FAIL;
            } else {
                PLL_OFF;
#if defined(RN_CM0_PLATFORM)
                HOSC_OFF(chipid);
#else
                HOSC_OFF();
#endif

#if defined(RN_CM0_PLATFORM)
                if (is_g2_chips == 1U) {
                    RC32M_OFF;
                }
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
                RC32M_OFF;
#endif
            }
        } else if (IS_RC29_MODE(mode)) {
#if defined(RN_CM0_PLATFORM)
            if (is_g2_chips == 1U) {
                if (((MEM32(OSC_CTRL1_ADDR) >> 6) & 0x1U) == 0x1U) {
                    /* switch to RC firstly if current mode and target mode are HC mode */
                    res = sysmodeWrite(Clock_RC_1M8, Clock_Div_1, chipid);
                }
            }
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
            if (((MEM32(OSC_CTRL1_ADDR) >> 6) & 0x1U) == 0x1U) {
                /* switch to RC firstly if current mode and target mode are HC mode */
                res = sysmodeWrite(Clock_RC_1M8, Clock_Div_1);
            }
#endif

#if defined(RN_CM0_PLATFORM)
            if (SYSCLK_FAIL == sysmodeWrite(mode, cdiv, chipid)) {
#else
            if (SYSCLK_FAIL == sysmodeWrite(mode, cdiv)) {
#endif
                res = SYSCLK_FAIL;
            } else {
                /* enable PLL14 if RC29M mode for PLL1Hz. power consumption of PLL14 is 120-160UA */
#if defined(RN_CM0_PLATFORM)
                MEM32(OSC_CTRL2_ADDR(chipid)) = (MEM32(OSC_CTRL2_ADDR(chipid)) & (~((unsigned int)0x7U << 5))) | ((unsigned int)0x1U << 5);
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
                MEM32(OSC_CTRL2_9303_ADDR) = (MEM32(OSC_CTRL2_9303_ADDR) & (~((unsigned int)0x7U << 5))) | ((unsigned int)0x1U << 5);
#else
                MEM32(OSC_CTRL2_9103_ADDR) = (MEM32(OSC_CTRL2_9103_ADDR) & (~((unsigned int)0x7U << 5))) | ((unsigned int)0x1U << 5);
#endif
                PLL_ON;
                /* close HOSC */
#if defined(RN_CM0_PLATFORM)
                HOSC_OFF(chipid);
#else
                HOSC_OFF();
#endif
            }
        } else {
            /* HOSC/PLL */
#if defined(RN_CM0_PLATFORM)
            if (((chipid == CHIP_RA9701A) || (chipid == CHIP_RA9701B) || (chipid ==CHIP_RA9703A)) && IS_PLL_MODE(mode)) {
                temp = MEM32(MOD1_EN_ADDR(chipid));
                MEM32(MOD1_EN_ADDR(chipid)) = temp | (1 << 10U);
                if (MEM32(RTC_VBATCLKSEL_ADDR) == 0x75a3U) {
                    res = SYSCLK_FAIL;
                }
                MEM32(MOD1_EN_ADDR(chipid)) = temp;
            }
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3)
            if (IS_PLL_MODE(mode)) {
                temp = MEM32(MOD1_EN_9103_ADDR);
                MEM32(MOD1_EN_9103_ADDR) = temp | (1 << 10U);
                if (MEM32(RTC_VBATCLKSEL_ADDR) == 0x75a3U) {
                    res = SYSCLK_FAIL;
                }
                MEM32(MOD1_EN_9103_ADDR) = temp;
            }
#elif defined(RN202x_RN7326_SOC_V2)
            if (IS_PLL_MODE(mode)) {
                temp = MEM32(MOD1_EN_9303_ADDR);
                MEM32(MOD1_EN_9303_ADDR) = temp | (1 << 10U);
                if (MEM32(RTC_VBATCLKSEL_ADDR) == 0x75a3U) {
                    res = SYSCLK_FAIL;
                }
                MEM32(MOD1_EN_9303_ADDR) = temp;
            }
#endif

            if ((res == SYSCLK_PASS) && (1U == ((MEM32(OSC_CTRL1_ADDR) >> 6) & 1U))) {
                /* switch to RC firstly if current mode and target mode are HC mode */
#if defined(RN_CM0_PLATFORM)
                res = sysmodeWrite(Clock_RC_1M8, Clock_Div_1, chipid);
#else
                res = sysmodeWrite(Clock_RC_1M8, Clock_Div_1);
#endif
#if defined(RN_CM0_PLATFORM)
                if (is_g2_chips == 1U) {
                    /* unset rc32m as hc clock source */
                    MEM32(OSC_CTRL2_ADDR(chipid)) &= (~((unsigned int)1U << 20));
                }
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
                MEM32(OSC_CTRL2_9303_ADDR) &= (~((unsigned int)1U << 20));
#endif
            }

            if (SYSCLK_PASS == res) {
#if defined(RN_CM0_PLATFORM)
                res = sysmodeWrite(mode, cdiv, chipid);
#else
                res = sysmodeWrite(mode, cdiv);
#endif
            }

            if (SYSCLK_PASS == res) {
                /* HCM enable RCH for sysclk monitor */
#if defined(RN_CM0_PLATFORM)
                if (CHIP_RA9703A == chipid) {
                    en_rc32m_clk(chipid);
                } else if (IS_RA97_SERIES(chipid)) {
                    MEM32(OSC_CTRL2_ADDR(chipid)) = (MEM32(OSC_CTRL2_ADDR(chipid)) & (~((unsigned int)0x7U << 16))) | ((unsigned int)4U << 24);
                    RC_ON;
                } else {
                    RCM_OFF;
                    if (is_g2_chips == 1U) {
                        RC32M_OFF;
                    }
                }
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
                en_rc32m_clk();
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
                MEM32(OSC_CTRL2_9103_ADDR) = (MEM32(OSC_CTRL2_9103_ADDR) & (~((unsigned int)0x7U << 16))) | ((unsigned int)4U << 24);
                RC_ON;
#elif defined(RN202x_RN7326_SOC_B)
                RCM_OFF;
                RC32M_OFF;
#else
                RCM_OFF;
#endif

                if (Clock_Hosc != mode) {
#if defined(RN_CM0_PLATFORM)
                    HOSC_OFF(chipid);
#else
                    HOSC_OFF();
#endif
                }
            }
        }

#if defined(RN_CM0_PLATFORM)
        MEM32(SYS_PS_ADDR(chipid)) = sys_ps;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
        MEM32(SYS_PS_9303_ADDR) = sys_ps;
#else
        MEM32(SYS_PS_9103_ADDR) = sys_ps;
#endif
    }
    return res;
#endif
#else
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SYSCLK_FAIL;
    }
    SYSCLK_ApiTypeDef *Iap_Func = (SYSCLK_ApiTypeDef *)iap_addr;
    pSYSCLK_ClockModeConfig func_ptr = (pSYSCLK_ClockModeConfig)((uint32_t)Iap_Func->SYSCLK_ClockModeConfig + ROM_BASE_ADDR);
    return func_ptr(mode, 0U, cdiv);
#endif
}

void LL_SYSCLK_FastSysModeChg(eSysclkMode_TypeDef mode)
{
#if !defined(RN831x_RN861x_MCU_V4)
#if  defined(NVRLIB_EN)
    rn_lib->LL_SYSCLK_FastSysModeChg(mode);
#else
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif
#if defined(RN_CM0_PLATFORM)
    const unsigned int sysps_bak = MEM32(SYS_PS_ADDR(chipid));
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    const unsigned int sysps_bak = MEM32(SYS_PS_9303_ADDR);
#else
    const unsigned int sysps_bak = MEM32(SYS_PS_9103_ADDR);
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_D) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
    volatile unsigned int tmp_read_ldomod = 0x0U;
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
    volatile unsigned int tmp_read = 0x0U;
#endif

    if (Clock_Losc == mode) {
#if defined(RN_CM0_PLATFORM)
        if (chipid > CHIP_RA9103F) {
            /* open write-window */
            MEM32(SYS_PS_ADDR(chipid)) = 0x75U;
            open_dig5(chipid);
            tmp_read = 0xa5580069U | (MEM32(ANA_LDO_ADDR(chipid)) & 0xff00U);
            tmp_read_ldomod = 0xa5650069U | (MEM32(ANA_LDOMOD_ADDR(chipid)) & 0xff00U);
        }
#elif defined(RN821x_RN721x_SOC_D)
        MEM32(SYS_PS_9103_ADDR) = 0x75U;
        tmp_read_ldomod = 0xa5580069U;
#elif defined(RN202x_RN7326_SOC_V2)
        MEM32(SYS_PS_9303_ADDR) = 0x75U;
        open_dig5();
        tmp_read = 0xa5580069U | (MEM32(ANA_LDO_9103_ADDR) & 0xff00U);
        tmp_read_ldomod = 0xa5650069U;
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
        MEM32(SYS_PS_9103_ADDR) = 0x75U;
        open_dig5();
        tmp_read = 0xa5580069U | (MEM32(ANA_LDO_9103_ADDR) & 0xff00U);
        tmp_read_ldomod = 0xa5650069U;
#elif defined(RN202x_RN7326_SOC_B)
        MEM32(SYS_PS_9303_ADDR) = 0x75U;
        tmp_read = 0xa5580069U | (MEM32(ANA_LDO_9303_ADDR) & 0xff00U);
#endif

        /*Switch to LOSC*/
#if defined(RN_CM0_PLATFORM)
        MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
        MEM32(SYS_PS_9303_ADDR) = 0x82U;
#else
        MEM32(SYS_PS_9103_ADDR) = 0x82U;
#endif
        MEM32(SYS_MODE_ADDR) = WR_LC_MODE;
        RCM_OFF;
#if defined(RN_CM0_PLATFORM)
        if (chipid > CHIP_RA9103F) {
            /* close write-window */
            MEM32(SYS_PS_ADDR(chipid))   = 0x75U;
            MEM32(ANA_LDO_ADDR(chipid)) = tmp_read;
            MEM32(ANA_LDOMOD_ADDR(chipid)) = tmp_read_ldomod;
            close_dig5(chipid);
        }
#elif defined(RN821x_RN721x_SOC_D)
        MEM32(SYS_PS_9103_ADDR)   = 0x75U;
        MEM32(ANA_LDOMOD_9103_ADDR) = tmp_read_ldomod;
#elif defined(RN202x_RN7326_SOC_V2)
        MEM32(SYS_PS_9303_ADDR)   = 0x75U;
        MEM32(ANA_LDO_9103_ADDR) = tmp_read;
        MEM32(ANA_LDOMOD_9703_ADDR) = tmp_read_ldomod;
        close_dig5();
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
        MEM32(SYS_PS_9103_ADDR)   = 0x75U;
        MEM32(ANA_LDO_9103_ADDR) = tmp_read;
        MEM32(ANA_LDOMOD_9701_ADDR) = tmp_read_ldomod;
        close_dig5();
#elif defined(RN202x_RN7326_SOC_B)
        MEM32(SYS_PS_9303_ADDR)   = 0x75U;
        MEM32(ANA_LDOMOD_9303_ADDR) = tmp_read;
#endif
    } else if (Clock_RC_1M8 == (mode)) {
#if defined(RN_CM0_PLATFORM)
        MEM32(SYS_PS_ADDR(chipid))   = 0x82U;
        MEM32(OSC_CTRL2_ADDR(chipid)) &= (~((unsigned int)0x7U << 16));
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
        MEM32(SYS_PS_9303_ADDR) = 0x82U;
        MEM32(OSC_CTRL2_9303_ADDR) &= (~((unsigned int)0x7U << 16));
#else
        MEM32(SYS_PS_9103_ADDR) = 0x82U;
        MEM32(OSC_CTRL2_9103_ADDR) &= (~((unsigned int)0x7U << 16));
#endif
        RC_ON;
#if defined(RN_CM0_PLATFORM)
        if (chipid > CHIP_RA9103F) {
            /* open write-window*/
            MEM32(SYS_PS_ADDR(chipid)) = 0x75U;
            open_dig5(chipid);
            tmp_read = 0xa5580000U | (MEM32(ANA_LDO_ADDR(chipid)) & 0xff00U);
            tmp_read_ldomod = 0xa5650000U | (MEM32(ANA_LDOMOD_ADDR(chipid)) & 0xff00U);
            MEM32(ANA_LDO_ADDR(chipid)) = tmp_read;
            MEM32(ANA_LDOMOD_ADDR(chipid)) = tmp_read_ldomod;
            MEM32(SYS_PS_ADDR(chipid))   = 0x82U;
        }
        MEM32(SYS_MODE_ADDR) = WR_RC_MODE;
        if (chipid > CHIP_RA9303B) {
            /* close write-window */
            MEM32(SYS_PS_ADDR(chipid)) = 0x75U;
            close_dig5(chipid);
        }
    } else {
        ;/*res = SYSCLK_FAIL;*/
    }
#elif defined(RN821x_RN721x_SOC_D)
        MEM32(SYS_PS_9103_ADDR) = 0x75U;
        tmp_read_ldomod = 0xa5580000U;
        MEM32(ANA_LDOMOD_9103_ADDR) = tmp_read_ldomod;
        MEM32(SYS_PS_9103_ADDR)   = 0x82U;
        MEM32(SYS_MODE_ADDR) = WR_RC_MODE;
    } else {
        ;/*res = SYSCLK_FAIL;*/
    }
#elif defined(RN202x_RN7326_SOC_V2)
        /* open write-window*/
        MEM32(SYS_PS_9303_ADDR) = 0x75U;
        open_dig5();
        tmp_read = 0xa5580000U | (MEM32(ANA_LDO_9103_ADDR) & 0xff00U);
        tmp_read_ldomod = 0xa5650000U;
        MEM32(ANA_LDO_9103_ADDR) = tmp_read;
        MEM32(ANA_LDOMOD_9703_ADDR) = tmp_read_ldomod;
        MEM32(SYS_PS_9303_ADDR)   = 0x82U;
        MEM32(SYS_MODE_ADDR) = WR_RC_MODE;
        /* close write-window */
        MEM32(SYS_PS_9303_ADDR) = 0x75U;
        close_dig5();
    } else {
        ;/*res = SYSCLK_FAIL;*/
    }
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
        /* open write-window*/
        MEM32(SYS_PS_9103_ADDR) = 0x75U;
        open_dig5();
        tmp_read = 0xa5580000U | (MEM32(ANA_LDO_9103_ADDR) & 0xff00U);
        tmp_read_ldomod = 0xa5650000U;
        MEM32(ANA_LDO_9103_ADDR) = tmp_read;
        MEM32(ANA_LDOMOD_9701_ADDR) = tmp_read_ldomod;
        MEM32(SYS_PS_9103_ADDR)   = 0x82U;
        MEM32(SYS_MODE_ADDR) = WR_RC_MODE;
        /* close write-window */
        MEM32(SYS_PS_9103_ADDR) = 0x75U;
        close_dig5();
    } else {
        ;/*res = SYSCLK_FAIL;*/
    }
#elif defined(RN202x_RN7326_SOC_B)
        /* open write-window*/
        MEM32(SYS_PS_9303_ADDR) = 0x75U;
        tmp_read = 0xa5580000U | (MEM32(ANA_LDOMOD_9303_ADDR) & 0xff00U);
        MEM32(ANA_LDOMOD_9303_ADDR) = tmp_read;
        MEM32(SYS_PS_9303_ADDR)   = 0x82U;
        MEM32(SYS_MODE_ADDR) = WR_RC_MODE;
        /* close write-window */
        MEM32(SYS_PS_9303_ADDR) = 0x75U;
    } else {
        ;/*res = SYSCLK_FAIL;*/
    }
#else
        MEM32(SYS_MODE_ADDR) = WR_RC_MODE;
    } else {
        ;/*res = SYSCLK_FAIL;*/
    }
#endif
#if defined(RN_CM0_PLATFORM)
    MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#else
    MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif
#endif
#else
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return;
    }
    SYSCLK_ApiTypeDef *Iap_Func = (SYSCLK_ApiTypeDef *)iap_addr;
    pSYSCLK_FastClockModeConfig func_ptr = (pSYSCLK_FastClockModeConfig)((uint32_t)Iap_Func->SYSCLK_FastClockModeConfig + ROM_BASE_ADDR);
    func_ptr(mode);
#endif
}

#if !defined(RN831x_RN861x_MCU_V4)
void LL_SYSCLK_NvmActive(void)
{
#if  defined(NVRLIB_EN)
    rn_lib->LL_SYSCLK_NvmActive();
#else
#if defined(RN_CM0_PLATFORM)
    rn_chipid_t chipid;
#endif
    if ((MEM32(OSC_CTRL1_ADDR) & ((unsigned int)1U << 4)) != 0U) {
#if defined(RN_CM0_PLATFORM)
        chipid = getChipid();
        set_ldo_mode(HIGH_MODE, chipid);
#elif defined(RN821x_RN721x_SOC_D) ||\
      defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
        set_ldo_mode(HIGH_MODE);
#endif
    }
#endif
}

void LL_SYSCLK_NvmSleep(void)
{
#if  defined(NVRLIB_EN)
    rn_lib->LL_SYSCLK_NvmSleep();
#else
#if defined(RN_CM0_PLATFORM)
    rn_chipid_t chipid;
#endif
    if ((MEM32(OSC_CTRL1_ADDR) & ((unsigned int)1U << 4)) != 0U) {
#if defined(RN_CM0_PLATFORM)
        chipid = getChipid();
        set_ldo_mode(LIGHT_MODE, chipid);
#elif defined(RN821x_RN721x_SOC_D) ||\
      defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
        set_ldo_mode(LIGHT_MODE);
#endif
    }
#endif
}
#endif

/* Do not change SOFT_VER/CODE_VER which generated by build script */
#ifndef  SOFT_VER
#define  SOFT_VER 0x10208U
#endif
unsigned int LL_SYSCLK_GetLibVersion(void)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSCLK_GetLibVersion();
#else
    return SOFT_VER;
#endif
}

#ifndef  CODE_VER
#define  CODE_VER 3009U
#endif
unsigned int LL_SYSCLK_GetCodeVersion(void)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSCLK_GetCodeVersion();
#else
    return CODE_VER;
#endif
}

#if defined(RN831x_RN861x_MCU_V4)
eSysclkRet_TypeDef LL_SYSCLK_PLLConfig(eSysclkMode_TypeDef PLLFrequency, eSycclkPLLSwitch_TypeDef PLLState)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SYSCLK_FAIL;
    }
    SYSCLK_ApiTypeDef *Iap_Func = (SYSCLK_ApiTypeDef *)iap_addr;
    pSYSCLK_PLLOutputFreqConfig func_ptr = (pSYSCLK_PLLOutputFreqConfig)((uint32_t)Iap_Func->SYSCLK_PLLOutputFreqConfig + ROM_BASE_ADDR);
    return func_ptr(PLLFrequency, PLLState);
}

eSysclkRet_TypeDef LL_SYSCLK_OptionConfigCheck(void)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SYSCLK_FAIL;
    }
    SYSCLK_ApiTypeDef *Iap_Func = (SYSCLK_ApiTypeDef *)iap_addr;
    pSYSCLK_OptionConfigCheck func_ptr = (pSYSCLK_OptionConfigCheck)((uint32_t)Iap_Func->SYSCLK_OptionConfigCheck + ROM_BASE_ADDR);
    return func_ptr();
}
#endif
/* r2989 */
