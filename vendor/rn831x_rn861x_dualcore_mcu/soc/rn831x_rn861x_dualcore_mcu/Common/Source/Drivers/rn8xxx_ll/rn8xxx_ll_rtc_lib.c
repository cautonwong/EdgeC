/******************************************************************************
 * @file     rn8xxx_ll_rtc_lib.c
 * @brief    rtc driver
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) 2008, Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#include "rn8xxx_ll_utils.h"
#include "rn8xxx_ll_rtc_lib.h"
#include "rn8xxx_ll_sysoption.h"

#if defined(RN831x_RN861x_MCU_V4)
typedef struct
{
	unsigned int Reserved0[9];
    unsigned int (*RTC_GetRTCDota)               (void);
    unsigned int (*RTC_SetRTCDota)               (unsigned short RTCDota);
} RTC_ApiTypeDef;

typedef unsigned int (*pRTC_GetRTCDota)          (void);
typedef unsigned int (*pRTC_SetRTCDota)          (unsigned short RTCDota);
#else
#if !defined(NVRLIB_EN)
static void RtcWait(void);
static void RtcCalWait(void);

static void RtcWait(void)
{
    unsigned int i;
    for (i = 0U; i < 2000U; i++)
    {
        if (((MEM32(RTC_CTRL_ADDR) >> 9) & 0x1U) == 0x0U)
        {
            break;
        }
    }
}

static void RtcCalWait(void)
{
    unsigned int i;

    for (i = 0U; i < 20000U; i++)
    {
        if (((MEM32(RTC_CTRL_ADDR) >> 10) & 0x1U) == 0x0U)
        {
            break;
        }
    }
}
#endif
#endif

/**
 * @brief LL_RTC_ReadDota
 *
 * @param
 *    none
 * @return
 *    unsigned short dota: dota value of RTC
 *
 */
unsigned short LL_RTC_ReadDota(void)
{
#if !defined(RN831x_RN861x_MCU_V4)
#if  defined(NVRLIB_EN)
    return rn_lib->LL_RTC_ReadDota();
#else
    unsigned short i;
    unsigned int sys_ps;

#if defined(RN_CM0_PLATFORM)
    rn_chipid_t chipid;

    chipid = getChipid();
    sys_ps = MEM32(SYS_PS_ADDR(chipid));
    MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
    MEM32(MOD1_EN_ADDR(chipid)) |= ((unsigned int)1U << 10);
#elif (defined(RN202x_RN7326_SOC_V2) ||\
       defined(RN202x_RN7326_SOC_B))
    sys_ps = MEM32(SYS_PS_9303_ADDR);
    MEM32(SYS_PS_9303_ADDR) = 0x82U;
    MEM32(MOD1_EN_9303_ADDR) |= ((unsigned int)1U << 10);
#else
    sys_ps = MEM32(SYS_PS_9103_ADDR);
    MEM32(SYS_PS_9103_ADDR) = 0x82U;
    MEM32(MOD1_EN_9103_ADDR) |= ((unsigned int)1U << 10);
#endif

    i = (unsigned short)MEM32(RTC_DOTA_ADDR);

    /* FEED_WDT; */

#if defined(RN_CM0_PLATFORM)
    MEM32(SYS_PS_ADDR(chipid)) = sys_ps;
#elif (defined(RN202x_RN7326_SOC_V2) ||\
       defined(RN202x_RN7326_SOC_B))
    MEM32(SYS_PS_9303_ADDR) = sys_ps;
#else
    MEM32(SYS_PS_9103_ADDR) = sys_ps;
#endif

    return i;
#endif
#else
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return RTC_FAIL;
    }
    RTC_ApiTypeDef *Iap_Func = (RTC_ApiTypeDef *)iap_addr;
    pRTC_GetRTCDota func_ptr = (pRTC_GetRTCDota)((uint32_t)Iap_Func->RTC_GetRTCDota + ROM_BASE_ADDR);
    return func_ptr();
#endif
}

/* unsigned char LL_SYSOPTION_SysConfCheck(void); */
/**
 * @brief LL_RTC_WriteDota
 *
 * @param
 *    in unsigned short dota: dota value of RTC
 * @return
 *    none
 *
 */
eRtcRet_TypeDef LL_RTC_WriteDota(unsigned short dota)
{
#if !defined(RN831x_RN861x_MCU_V4)
#if  defined(NVRLIB_EN)
    return rn_lib->LL_RTC_WriteDota(dota);
#else
    unsigned int sys_ps, rtc_ctrl, rtcps_bak;
    eRtcRet_TypeDef res = RTC_PASS;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
    sys_ps = MEM32(SYS_PS_ADDR(chipid));
    MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
    MEM32(MOD1_EN_ADDR(chipid)) |= ((unsigned int)1U << 10);
    /* MEM32(MOD1_EN_ADDR(chipid)) |= ((unsigned int)1U << 9); */ /* WDT APB */
#elif (defined(RN202x_RN7326_SOC_V2) ||\
       defined(RN202x_RN7326_SOC_B))
    sys_ps = MEM32(SYS_PS_9303_ADDR);
    MEM32(SYS_PS_9303_ADDR) = 0x82U;
    MEM32(MOD1_EN_9303_ADDR) |= ((unsigned int)1U << 10);
#else
    sys_ps = MEM32(SYS_PS_9103_ADDR);
    MEM32(SYS_PS_9103_ADDR) = 0x82U;
    MEM32(MOD1_EN_9103_ADDR) |= ((unsigned int)1U << 10);
#endif

    rtc_ctrl = MEM32(RTC_CTRL_ADDR);

    RtcWait();
    MEM32(RTC_CTRL_ADDR) |= 0x100U;

    RtcWait();
    rtcps_bak = MEM32(RTC_PS_ADDR);
    MEM32(RTC_PS_ADDR) = 0x8EU;

    if (SYSOPTION_FAIL == LL_SYSOPTION_SysConfCheck()) {
        res = RTC_FAIL;
    }

    RtcWait();
    if (0x400U == (MEM32(RTC_CTRL_ADDR) & 0x400U))
    {
        RtcCalWait();
    }
    MEM32(RTC_DOTA_ADDR) = dota;
    if (0x400U == (MEM32(RTC_CTRL_ADDR) & 0x400U))
    {
        RtcCalWait();
        MEM32(RTC_DOTA_ADDR) = dota;
    }

    if (MEM32(RTC_DOTA_ADDR) != dota)
    /* if (RtcCalCheck(chipid)) */
    {
        res = RTC_FAIL;
    }

    RtcWait();
    MEM32(RTC_CTRL_ADDR) = rtc_ctrl;

    RtcWait();
    MEM32(RTC_PS_ADDR) = rtcps_bak;
    // RtcWait();

    /* FEED_WDT; */

#if defined(RN_CM0_PLATFORM)
    MEM32(SYS_PS_ADDR(chipid)) = sys_ps;
#elif (defined(RN202x_RN7326_SOC_V2) ||\
       defined(RN202x_RN7326_SOC_B))
    MEM32(SYS_PS_9303_ADDR) = sys_ps;
#else
    MEM32(SYS_PS_9103_ADDR) = sys_ps;
#endif
    return res;
#endif
#else
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return RTC_FAIL;
    }
    RTC_ApiTypeDef *Iap_Func = (RTC_ApiTypeDef *)iap_addr;
    pRTC_SetRTCDota func_ptr = (pRTC_SetRTCDota)((uint32_t)Iap_Func->RTC_SetRTCDota + ROM_BASE_ADDR);
    return func_ptr(dota);
#endif
}

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3)
void LL_RTC_ClkSel(rtc_clk_e clk_sel)
{
#if  defined(NVRLIB_EN)
    rn_lib->LL_RTC_ClkSel(clk_sel);
#else
#if defined(RN_CM0_PLATFORM)
    unsigned int i;
    unsigned int sysps_bak, rtcctl_bak;

    rn_chipid_t chipid = getChipid();
    if ((chipid == CHIP_RA9701A) ||
        (chipid == CHIP_RA9701B) ||
        (chipid == CHIP_RA9702A) ||
        (chipid == CHIP_RA9702B) ||
        (chipid == CHIP_RA9703A))
    {
        sysps_bak = MEM32(SYS_PS_ADDR(chipid));
        MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
        rtcctl_bak = MEM32(MOD1_EN_ADDR(chipid));
        MEM32(MOD1_EN_ADDR(chipid)) = rtcctl_bak | (1U << 10);

        if ((chipid == CHIP_RA9702A) || (chipid == CHIP_RA9702B)) {
            i = MEM32(OSC_CTRL2_ADDR(chipid));
            if (RTC_CLK_IS_LOSC == clk_sel) {
                MEM32(OSC_CTRL2_ADDR(chipid)) = i & (~(1 << 14));
            } else {
                MEM32(OSC_CTRL2_ADDR(chipid)) = i | (1U << 14);
            }
        } else {
            rtcctl_bak = MEM32(RTC_CTRL_ADDR);
            i = MEM32(RTC_CTRL_ADDR);
            MEM32(RTC_CTRL_ADDR) = i | (1U << 8);
            if (RTC_CLK_IS_LOSC == clk_sel) {
                /* from RCL to LOSC*/
                i = MEM32(OSC_CTRL2_ADDR(chipid));
                if (chipid == CHIP_RA9703A) {
                    MEM32(OSC_CTRL2_ADDR(chipid)) = i & (~(1U << 28)); /* rcl path off */
                } else {
                    MEM32(OSC_CTRL2_ADDR(chipid)) = i & (~(1U << 19)); /* rcl path off */
                }
                for (i = 0; i < (884U >> 2); i++) {
                    MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
                }
                MEM32(RTC_VBATCLKSEL_ADDR) = 0U;
            } else {
                /* from LOSC to RCL */
                i = MEM32(OSC_CTRL2_ADDR(chipid));
                if (chipid == CHIP_RA9703A) {
                    MEM32(OSC_CTRL2_ADDR(chipid)) = i & (~(1U << 28)); /* rcl path off */
                } else {
                    MEM32(OSC_CTRL2_ADDR(chipid)) = i & (~(1U << 19)); /* rcl path off */
                }
                for (i = 0; i < (884U >> 2); i++) {
                    MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
                }
                MEM32(RTC_VBATCLKSEL_ADDR) = 0x75U << 8;
                MEM32(RTC_VBATCLKSEL_ADDR) = 0x75a3U;
                i = MEM32(OSC_CTRL2_ADDR(chipid));
                if (chipid == CHIP_RA9703A) {
                    MEM32(OSC_CTRL2_ADDR(chipid)) = i | (1U << 28); /* rcl path on */
                } else {
                    MEM32(OSC_CTRL2_ADDR(chipid)) = i | (1U << 19); /* rcl path on */
                }
            }
            MEM32(RTC_CTRL_ADDR) = rtcctl_bak;
        }

        MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
    }
#elif defined(RN202x_RN7326_SOC_V2)
    unsigned int i;
    unsigned int sysps_bak, rtcctl_bak;

    sysps_bak = MEM32(SYS_PS_9303_ADDR);
    MEM32(SYS_PS_9303_ADDR) = 0x82U;
    rtcctl_bak = MEM32(MOD1_EN_9303_ADDR);
    MEM32(MOD1_EN_9303_ADDR) = rtcctl_bak | (1U << 10);
    rtcctl_bak = MEM32(RTC_CTRL_ADDR);

    i = MEM32(RTC_CTRL_ADDR);
    MEM32(RTC_CTRL_ADDR) = i | (1U << 8);

    if (RTC_CLK_IS_LOSC == clk_sel) {
        /* from RCL to LOSC*/
        i = MEM32(OSC_CTRL2_9303_ADDR);
        MEM32(OSC_CTRL2_9303_ADDR) = i & (~(1U << 28)); /* rcl path off */
        for (i = 0; i < (884U >> 2); i++) {
            MEM32(SYS_PS_9303_ADDR) = 0x82U;
        }
        MEM32(RTC_VBATCLKSEL_ADDR) = 0U;
    } else {
        /* from LOSC to RCL */
        i = MEM32(OSC_CTRL2_9303_ADDR);
        MEM32(OSC_CTRL2_9303_ADDR) = i & (~(1U << 28)); /* rcl path off */
        for (i = 0; i < (884U >> 2); i++) {
            MEM32(SYS_PS_9303_ADDR) = 0x82U;
        }
        MEM32(RTC_VBATCLKSEL_ADDR) = 0x75U << 8;
        MEM32(RTC_VBATCLKSEL_ADDR) = 0x75a3U;
        i = MEM32(OSC_CTRL2_9303_ADDR);
        MEM32(OSC_CTRL2_9303_ADDR) = i | (1U << 28); /* rcl path on */
    }
    MEM32(RTC_CTRL_ADDR) = rtcctl_bak;

    MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#elif (defined(RN831x_RN861x_MCU_V2) ||\
       defined(RN831x_RN861x_MCU_V3))
    unsigned int i;
    unsigned int sysps_bak, rtcctl_bak;

    sysps_bak = MEM32(SYS_PS_9103_ADDR);
    MEM32(SYS_PS_9103_ADDR) = 0x82U;
    rtcctl_bak = MEM32(MOD1_EN_9103_ADDR);
    MEM32(MOD1_EN_9103_ADDR) = rtcctl_bak | (1U << 10);
    rtcctl_bak = MEM32(RTC_CTRL_ADDR);

    i = MEM32(RTC_CTRL_ADDR);
    MEM32(RTC_CTRL_ADDR) = i | (1U << 8);

    if (RTC_CLK_IS_LOSC == clk_sel) {
        /* from RCL to LOSC*/
        i = MEM32(OSC_CTRL2_9103_ADDR);
        MEM32(OSC_CTRL2_9103_ADDR) = i & (~(1U << 19)); /* rcl path off */
        for (i = 0; i < (884U >> 2); i++) {
            MEM32(SYS_PS_9103_ADDR) = 0x82U;
        }
        MEM32(RTC_VBATCLKSEL_ADDR) = 0U;
    } else {
        /* from LOSC to RCL */
        i = MEM32(OSC_CTRL2_9103_ADDR);
        MEM32(OSC_CTRL2_9103_ADDR) = i & (~(1U << 19)); /* rcl path off */
        for (i = 0; i < (884U >> 2); i++) {
            MEM32(SYS_PS_9103_ADDR) = 0x82U;
        }
        MEM32(RTC_VBATCLKSEL_ADDR) = 0x75U << 8;
        MEM32(RTC_VBATCLKSEL_ADDR) = 0x75a3U;
        i = MEM32(OSC_CTRL2_9103_ADDR);
        MEM32(OSC_CTRL2_9103_ADDR) = i | (1U << 19); /* rcl path on */
    }
    MEM32(RTC_CTRL_ADDR) = rtcctl_bak;

    MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#elif (defined(RN821x_RN721x_SOC_V2) ||\
       defined(RN821x_RN721x_SOC_V3))
    unsigned int i;
    unsigned int sysps_bak;

    sysps_bak = MEM32(SYS_PS_9103_ADDR);
    MEM32(SYS_PS_9103_ADDR) = 0x82U;

    i = MEM32(OSC_CTRL2_9103_ADDR);
    if (RTC_CLK_IS_LOSC == clk_sel) {
        MEM32(OSC_CTRL2_9103_ADDR) = i & (~(1 << 14));
    } else {
        MEM32(OSC_CTRL2_9103_ADDR) = i | (1U << 14);
    }

    MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif
#endif
}
#endif
/* r2989 */
