/**
 * @file utils.c
 * @author Renergy Technology Inc.
 * @brief common functions used by lib code
 *
 * @copyright Copyright (C) 2008, Renergy Technology Inc. All rights reserved.
 *
 */
#include "stddef.h"
#include "rn8xxx_ll_utils.h"

#if !defined(RN831x_RN861x_MCU_V4)
static unsigned int sumCheck(const unsigned int p_data[], unsigned int length);

#if defined(RN_CM0_PLATFORM)
/**
 * @brief Get the Chipid number
 *
 * @return rn_chipid_t: chipid number
 */
rn_chipid_t getChipid(void)
{
    rn_chipid_t chipid;
    unsigned char res;
    unsigned int temp;

    if ((*(volatile unsigned int *)(0x4003402cU) & 0xff00U) == 0x8200U)
    {
        res = get_sysreg_reliable(CHIP_RA9103D, ANA_DIEVER_9103_ADDR, &temp, 1U);
        if (res == 0U) {
            switch (temp)
            {
            case 0xc0U:
                chipid = CHIP_RA9103C;
                break;
            case 0x10U:
                chipid = CHIP_RA9103G;
                break;
            case 0x40U:
                chipid = CHIP_RA9702A;
                break;
            case 0x60U:
                chipid = CHIP_RA9702B;
                break;
            case 0xe0U:
                chipid = CHIP_RA9103E;
                break;
            case 0xf0U:
                chipid = CHIP_RA9103F;
                break;
            case 0x30U:
                chipid = CHIP_RA9701A;
                break;
            case 0x50U:
                chipid = CHIP_RA9701B;
                break;
            default:
                chipid = CHIP_RA9103D;
                break;
            }
        } else {
            chipid = CHIP_INVALID;
        }
    }
    else
    {
        /* RA9106 or RA9303 or 9703 */
        res = get_sysreg_reliable(CHIP_RA9303B, ANA_DIEVER_9303_ADDR, &temp, 1U);
        if (res == 0U) {
            switch (temp)
            {
            case 0xa0U:
                chipid = CHIP_RA9303A;
                break;
            case 0xb0U:
                chipid = CHIP_RA9303B;
                break;
            case 0xc0U:
                if (*(volatile unsigned int *)(0x40034038U) == 0x2025U)
                {
                    chipid = CHIP_RA9106B;
                }
                else
                {
                    chipid = CHIP_RA9106C;
                }
                break;
            default:
                res = get_sysreg_reliable(CHIP_RA9303B, ANA_DIEVER_9103_ADDR, &temp, 1U);
                if ((res == 0U) && (temp == 0xd0U)) {
                    chipid = CHIP_RA9703A;
                } else {
                    chipid = CHIP_INVALID;
                }
                break;
            }
        } else {
            chipid = CHIP_INVALID;
        }
    }

    return chipid;
}
#endif

/**
 * @brief Get the Mip Addr object
 *
 * @param mem_type: 0 = FLASH, 1 = EEPROM, 2 = ROM
 * @return unsigned int: memory base address
 */
#if defined(RN_CM0_PLATFORM)
unsigned int getMipAddr(unsigned char mem_type, rn_chipid_t chipid)
{
    unsigned int ee_addr, fl_addr, rom_addr;
    switch (MEM32(MAP_CTL_ADDR(chipid)) & 0x7U)
    {
    case 0:
        ee_addr = 0x08000000U;
        fl_addr = 0x0U;
        rom_addr = 0x18000000U;
        break;
    case 1:
        ee_addr = 0x0U;
        fl_addr = 0x08000000U;
        rom_addr = 0x18000000U;
        break;
    case 2:
        ee_addr = 0x08000000U;
        fl_addr = 0x10000000U;
        rom_addr = 0x18000000U;
        break;
    case 3:
        ee_addr = 0x08000000U;
        fl_addr = 0x18000000U;
        rom_addr = 0x0U;
        break;
    default:
        ee_addr = 0x08000000U;
        fl_addr = 0x0U;
        rom_addr = 0x18000000U;
        break;
    }
    return (mem_type == 0U) ? fl_addr : (mem_type == 1U) ? ee_addr
                                                       : rom_addr;
}
#endif

static const unsigned char freq_index_table[8] = {3U, 2U, 1U, 0U, 4U, 5U, 1U, 1U};
/**
 * @brief get current run clock index
 *
 * @return unsigned int: clock index
 */
unsigned char currentClk_idx(void)
{
    const unsigned char val = (unsigned char)((MEM32(OSC_CTRL1_ADDR) >> 8U) & 0x7U);

    return freq_index_table[val];
}

/**
 * @brief get current run clock
 *
 * @param chipid: chip index
 * @return unsigned int: clock frequency
 */
#if defined(RN_CM0_PLATFORM)
unsigned int currentClk(rn_chipid_t chipid)
#else
unsigned int currentClk(void)
#endif
{
    unsigned int freq;
    unsigned char freq_scaler;

    freq_scaler = currentClk_idx();
#if defined(RN_CM0_PLATFORM)
    freq = (freq_scaler == 0U) ? 32768U : ((((CHIP_RA9303B == chipid) || (CHIP_RA9703A == chipid)) ? 2048000U : 1843200U) << (freq_scaler - 1U));
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    freq = (freq_scaler == 0U) ? 32768U : (2048000U << (freq_scaler - 1U));
#else
    freq = (freq_scaler == 0U) ? 32768U : (1843200U << (freq_scaler - 1U));
#endif
    return freq;
}

static const unsigned char us_freq[5] = {23U, 22U, 21U, 20U, 19U};
static const unsigned char us_cycle[5] = {2U, 4U, 8U, 15U, 30U};                           /* 1us = us_cycle[x] cycles */
/**
 * timeout unit: us
 * sign = 1, exit while val != ([addr] & val)
 * sign = 0, exit while val == ([addr] & val)
 */
#if defined(RN_CM0_PLATFORM)
void usTimeout(unsigned int addr, unsigned int val, unsigned int sign, unsigned int timeout, rn_chipid_t chipid)
#else
void usTimeout(unsigned int addr, unsigned int val, unsigned int sign, unsigned int timeout)
#endif
{
    /* const unsigned int us_freq[] = {9100000, 4550000, 2274000, 1137000, 568000}; 0xffffff = us_freq[x] us */
    /* use shift instead of divide */
    unsigned int i, loops;
    unsigned char freq;
    unsigned int ticks;
    unsigned int sysps_bak, mod0_bak;
    unsigned int cnt;

#if defined(RN_CM0_PLATFORM)
    sysps_bak = MEM32(SYS_PS_ADDR(chipid));
    mod0_bak = MEM32(MOD0_EN_ADDR(chipid));
    MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
    MEM32(MOD0_EN_ADDR(chipid)) |= ((unsigned int)1U << 0U);
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    sysps_bak = MEM32(SYS_PS_9303_ADDR);
    mod0_bak = MEM32(MOD0_EN_9303_ADDR);
    MEM32(SYS_PS_9303_ADDR) = 0x82U;
    MEM32(MOD0_EN_9303_ADDR) |= ((unsigned int)1U << 0U);
#else
    sysps_bak = MEM32(SYS_PS_9103_ADDR);
    mod0_bak = MEM32(MOD0_EN_9103_ADDR);
    MEM32(SYS_PS_9103_ADDR) = 0x82U;
    MEM32(MOD0_EN_9103_ADDR) |= ((unsigned int)1U << 0U);
#endif
    if (timeout != 0U) {
        if ((!((sign == 1U) && (((*(unsigned int *)addr) & val) != val))) &&
            (!((sign == 0U) && (((*(unsigned int *)addr) & val) == val)))) {
            /* SysTick range: 29M: 568ms; 14M:1.137s; 7M:2.274s; 3M:4.55s; 1M:9.1s; 32K: 511.7s */
            freq = currentClk_idx();

            if (freq == 0U) {
                loops = 1U;
            } else {
                /* loops = 1 + timeout / us_freq[freq - 1]; */
                loops = 1U + (timeout >> us_freq[freq - 1U]);
            }

            for (i = 0U; i < loops; i++) {
                MEM32(SYSTC_CTRL_ADDR) = 2U;
                if (i != (loops - 1U)) {
                    ticks = 0xffffffU;
                } else {
                    if (freq == 0U) {
                        ticks = timeout / 32U;
                    } else {
                        /* ticks = (timeout - (loops - 1) * us_freq[freq - 1]) * us_cycle[freq - 1]; */
                        ticks = (timeout - ((loops - 1U) << us_freq[freq - 1U])) * us_cycle[freq - 1U];
                        if (ticks == 0U)
                        {
                            break;
                        }
                    }
                }
                MEM32(SYSTC_LOAD_ADDR) = ticks;
                MEM32(SYSTC_CTRL_ADDR) = 3U;
                cnt = 0U;
                for ( ; ; ) {
                    cnt++;
                    if ((1U == (unsigned char)((MEM32(SYSTC_CTRL_ADDR) >> 1U) & 0x1U)) ||
                        ((sign == 1U) && (((*(unsigned int *)addr) & val) != val)) ||
                        ((sign == 0U) && (((*(unsigned int *)addr) & val) == val)) ||
                        (cnt > ticks)) {
                        break;
                    }
                    /* FEED_WDT; */
                }
            }

            MEM32(SYSTC_CTRL_ADDR) = 2U;
        }
    }
#if defined(RN_CM0_PLATFORM)
    MEM32(MOD0_EN_ADDR(chipid)) = mod0_bak;
    MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    MEM32(MOD0_EN_9303_ADDR) = mod0_bak;
    MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#else
    MEM32(MOD0_EN_9103_ADDR) = mod0_bak;
    MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif
    return;
}

/**
 * @brief sumCheck
 *   校验和计算，按照32位宽计算
 * @param p_data 源数据起始地址
 * @param length 数据长度（word单位）
 * @return unsigned int 返回校验和值
 */
static unsigned int sumCheck(const unsigned int p_data[], unsigned int length)
{
    unsigned int i;
    unsigned int sum = 0U;
    for (i = 0U; i < length; i++) {
        sum = sum + p_data[i];
    }

    return sum;
}

/**
 * @brief check_spl_valid
 *   检查NVR数据合法性（32位累加和）
 * @param p_addr NVR的起始地址
 * @param len NVR的长度
 * @param flag 校验和位置标记(0：最后一个word存放校验和，1：最后第二个word存放校验和(最后一个word不参与计算))
 * @return unsigned char 返回结果
 *   1：正确
 *   0：错误
 */
unsigned char check_spl_valid(const unsigned int *p_addr, unsigned int len, unsigned char flag)
{
    unsigned int checksum;
    unsigned char sp_valid = 0x0U;
    unsigned int temp;

    if (p_addr != NULL) {
        checksum = sumCheck(p_addr, (flag == 0x1U) ? ((len >> 2) - 1U) : (len >> 2));
        temp = (unsigned int)(p_addr);
        temp += (len - (3U*4U));
        if ((*(unsigned int *)(temp) == 0xffffffffU) && (checksum == 0U))
        {
            sp_valid = 0x1U;
        }
    }

    return sp_valid;
}

/**
 * @brief Enable IAP NVM operation
 *
 * @param nvm_type: 0 = FLASH, 1 = EEPROM
 * @return unsigned char: ref IAP interface in datasheet
 */
unsigned char IapMemEnable(rn_iap_mem_en_t nvm_type)
{
    rn_iap_t iap;
    rn_iap_entry_t ptr_union;

    ptr_union.addr = ROM_BASE_ADDRESS + IAP_OFFSET;
    iap.cmd = 0x5aU;
    iap.par[0] = (unsigned int)nvm_type;
    ptr_union.iap_func(&iap.cmd, &iap.stat);
    return (unsigned char)iap.stat;
}

/* max 30ms. */
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1)
static const unsigned int CAL_BUSY_TIMEOUT[6] = {40, 56 * 40, 2 * 56 * 40, 2 * 2 * 56 * 40, 2 * 2 * 2 * 56 * 40, 2 * 2 * 2 * 2 * 56 * 40};
#endif
/**
 * @brief rtc wrong state check
 *   1. check if some of VBAT power domain registers are unexcepted
 *   2. check if rtc calc busy is always on(if yes, VBAT power domain is in wrong state)
 * @return unsigned char: 0 = OK, 1 = FAIL
 */
#if defined(RN_CM0_PLATFORM)
unsigned char RtcCalCheck(rn_chipid_t chipid)
#else
unsigned char RtcCalCheck(void)
#endif
{
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1)
    unsigned int i;
    unsigned int t;
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
    unsigned int ana_test;
    unsigned int ana_ctl;
    unsigned int losc_cfg1;
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_V2)
    unsigned int passwd_bak;
    unsigned int ana_ctl2;
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN202x_RN7326_SOC_B)
    unsigned int calc_ps;
#endif
    unsigned char res = 0U;

#if defined(RN_CM0_PLATFORM)
    if (!(/*(chipid == CHIP_RA9103A) ||*/
          (chipid == CHIP_RA9103C) ||
          (chipid == CHIP_RA9103D) ||
          (chipid == CHIP_RA9106C) ||
          (chipid == CHIP_RA9103G) ||
          (chipid == CHIP_RA9702A) ||
          (chipid == CHIP_RA9702B)))
    {
        if ((chipid == CHIP_RA9701A) ||
            (chipid == CHIP_RA9701B) ||
            (chipid == CHIP_RA9703A)) {
#endif
#if defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN_CM0_PLATFORM)
            /* registers are re-designed */
            ana_test = MEM32(MADC_ANA_TEST_9701_ADDR);
            ana_ctl = MEM32(MADC_ANA_CTL_9701_ADDR);
            ana_ctl2 = MEM32(MADC_ANA_CTL2_9701_ADDR);
            losc_cfg1 = MEM32(RTC_LOSC_CFG1_ADDR);
#if defined(RN_CM0_PLATFORM)
            if (chipid == CHIP_RA9703A) {
                if (((losc_cfg1 & 0x7fffU) != 0U) && ((losc_cfg1 & 0x7fffU) != 0x75a8U)) {
                    losc_cfg1 = 1U;
                } else {
                    losc_cfg1 = 0U;
                }
            } else {
                if ((((losc_cfg1 & 0xffU) != 0U) && ((losc_cfg1 & 0xffU) != 0xa8U))) {
                    losc_cfg1 = 1U;
                } else {
                    losc_cfg1 = 0U;
                }
            }
#elif defined(RN202x_RN7326_SOC_V2)
            if (((losc_cfg1 & 0x7fffU) != 0U) && ((losc_cfg1 & 0x7fffU) != 0x75a8U)) {
                losc_cfg1 = 1U;
            } else {
                losc_cfg1 = 0U;
            }
#else
            if (((losc_cfg1 & 0xffU) != 0U) && ((losc_cfg1 & 0xffU) != 0xa8U))  {
                losc_cfg1 = 1U;
            } else {
                losc_cfg1 = 0U;
            }
#endif
            /* RA9701 calc_ps only has por reset */
            if (/*((calc_ps != 0x0U) && (calc_ps != 0xa8U)) ||*/
                (0U != losc_cfg1) ||
                (0U != ana_test) ||
                (0U != ana_ctl) ||
                (0U != ana_ctl2))
            {
                passwd_bak = MEM32(MADC_ANA_PS_9701_ADDR);
                MEM32(MADC_ANA_PS_9701_ADDR) = 0x8eU;
                MEM32(MADC_ANA_CTL2_9701_ADDR) = 0x1b5a5U;
                MEM32(MADC_ANA_CTL2_9701_ADDR) = 0x1b5a5U;
                ana_ctl2 = MEM32(MADC_ANA_CTL2_9701_ADDR);
                if ((MEM32(MADC_ANA_PS_9701_ADDR) != 0x8eU) ||
                    (ana_ctl2 != 0x1b5a5U))
                {
                    res = 1U;
                }
                MEM32(MADC_ANA_CTL2_9701_ADDR) = 0x0U;
                MEM32(MADC_ANA_PS_9701_ADDR) = passwd_bak;
            }
#endif
#if defined(RN_CM0_PLATFORM)
        } else {
#endif
#if defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN202x_RN7326_SOC_B) ||\
    defined(RN_CM0_PLATFORM)
            /**
             * those registers should keep unchanged, otherwise means vbat power domain in wrong state
             * CALPS defines second-temp-calc password
             * LOSC_CFG1 defines losc_pd password
             * ANA_CTL defines RCH/BGR test switch password
             * ANA_TEST defines other test switch
            */
#if defined(RN_CM0_PLATFORM)
            ana_test = MEM32(MADC_ANA_TEST_ADDR(chipid));
            ana_ctl = MEM32(MADC_ANA_CTL_ADDR(chipid));
#elif defined(RN202x_RN7326_SOC_B)
            ana_test = MEM32(MADC_ANA_TEST_9303_ADDR);
            ana_ctl = MEM32(MADC_ANA_CTL_9303_ADDR);
#else
            ana_test = MEM32(MADC_ANA_TEST_9103_ADDR);
            ana_ctl = MEM32(MADC_ANA_CTL_9103_ADDR);
#endif
            losc_cfg1 = MEM32(RTC_LOSC_CFG1_ADDR);
            calc_ps = MEM32(RTC_CALPS_ADDR);
            if (((calc_ps != 0x0U) && (calc_ps != 0xa8U)) ||
                (((losc_cfg1 & 0xffU) != 0x0U) && ((losc_cfg1 & 0xffU) != 0xa8U)) ||
                ((ana_test & 0xfffeU) != 0x0U) ||
                (ana_ctl != 0x0U))
            {
                res = 1U;
            }
#endif
#if defined(RN_CM0_PLATFORM)
        }
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1)
        if (0U == res) {
            if (((MEM32(RTC_CTRL_ADDR) >> 10) & 0x1U) == 0x1U) {
                t = CAL_BUSY_TIMEOUT[currentClk_idx()];
                for (i = 0U; i < t; i++) {
                    if (((MEM32(RTC_CTRL_ADDR) >> 10) & 0x1U) == 0x0U) {
                        break;
                    }
                }

                if (i == t) {
                    res = 1U;
                }
            }
        }
#endif
#if defined(RN_CM0_PLATFORM)
    }
#endif
    return res;
}

// static const unsigned int POR_TIME[] = {2, 56 * 2, 2 * 56 * 2, 2 * 2 * 56 * 2, 2 * 2 * 2 * 56 * 2, 2 * 2 * 2 * 2 * 56 * 2};
/**
 * @brief soft reset vbat power domain
 *
 */
#if defined(RN_CM0_PLATFORM)
void vbat_reset(rn_chipid_t chipid)
#else
void vbat_reset(void)
#endif
{
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3)
    unsigned int rtc_ioen_bak;
    unsigned int rtc_iomode_bak;
    unsigned int ana_pad_bak;
#endif
    unsigned int dota_table_bak[10];
    unsigned int dota_ps_bak;
    unsigned int losc_cfg1_bak;
    unsigned int i, t;
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
    unsigned int vbat_gpio_s[11];
#endif

#if defined(RN_CM0_PLATFORM)
    rtc_ioen_bak = 0x0U;
    rtc_iomode_bak = 0x0U;
    ana_pad_bak = 0x0U;
#endif
    /**
     * reset vbat domain if rtc-state-check is wrong
     * before reset, we should backup those registers:
     *   RTC_IOEN/IOMODE/CALPS/CAL_T0~9/LOSCCFG1
     */
#if defined(RN_CM0_PLATFORM)
    if ((chipid == CHIP_RA9103E) || (chipid == CHIP_RA9103F)/* || (chipid == CHIP_RA9103H)*/ || (chipid == CHIP_RA9701A) || (chipid == CHIP_RA9701B))
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3)
    {
        /* 9103E/9103F/9701A/9701B vbat pad save */
        rtc_ioen_bak = MEM32(RTC_IOEN_ADDR);
        rtc_iomode_bak = MEM32(RTC_IOMODE_ADDR);
        ana_pad_bak = MEM32(MADC_ANA_PAD_ADDR);
    }
#endif
#if defined(RN_CM0_PLATFORM)
    else
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
    {
        /* 9106/9303/9703 vbat pad save */
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2)
        for (i = 0U; i < 11U; i++)
#else
        for (i = 0U; i < 10U; i++)
#endif
        {
            vbat_gpio_s[i] = MEM32(0x4002c100U + (4U * i));
#if defined(RN_CM0_PLATFORM)
            if ((chipid == CHIP_RA9303B) && (i == 10U)) {
                break;
            }
#endif
        }
    }
#endif

    /* rtc save */
    dota_ps_bak = MEM32(RTC_CALPS_ADDR);
    for (i = 0U; i < 10U; i++)
    {
        dota_table_bak[i] = RTC_DOTATBL->dota[i];
    }
    losc_cfg1_bak = MEM32(RTC_LOSC_CFG1_ADDR);

    /* vbat reset */
    // t = POR_TIME[currentClk_idx()];
    t = currentClk_idx();
    t = (t == 0U) ? 2U : ((56U * 2U) << (t - 1U));
#if defined(RN_CM0_PLATFORM)
    MEM32(ANA_RST_ADDR(chipid)) = ((unsigned int)0xa5U << 8) | 0xa8U; /* reset RTC */
    MEM32(ANA_RST_ADDR(chipid)) = ((unsigned int)0xa5U << 8) | 0xa8U;
#elif defined(RN202x_RN7326_SOC_B)
    MEM32(MADC_ANA_RST_9303_ADDR) = ((unsigned int)0xa5U << 8) | 0xa8U; /* reset RTC */
    MEM32(MADC_ANA_RST_9303_ADDR) = ((unsigned int)0xa5U << 8) | 0xa8U;
#else
    MEM32(MADC_ANA_RST_9103_ADDR) = ((unsigned int)0xa5U << 8) | 0xa8U; /* reset RTC */
    MEM32(MADC_ANA_RST_9103_ADDR) = ((unsigned int)0xa5U << 8) | 0xa8U;
#endif
    for (i = 0U; i < t; i++)
    {
        ;
    }
#if defined(RN_CM0_PLATFORM)
    MEM32(ANA_RST_ADDR(chipid)) = ((unsigned int)0xa5U << 8);
    MEM32(ANA_RST_ADDR(chipid)) = ((unsigned int)0xa5U << 8);
    MEM32(ANA_RST_ADDR(chipid)) = 0U;
#else
    MEM32(MADC_ANA_RST_9103_ADDR) = ((unsigned int)0xa5U << 8);
    MEM32(MADC_ANA_RST_9103_ADDR) = ((unsigned int)0xa5U << 8);
    MEM32(MADC_ANA_RST_9103_ADDR) = 0U;
#endif

    MEM32(RTC_PS_ADDR) = 0x8eU;
    for (i = 0U; i < 10U; i++)
    {
        RTC_DOTATBL->dota[i] = dota_table_bak[i];
    }
    if (dota_ps_bak == 0xa8U)
    {
        MEM32(RTC_CALPS_ADDR) = 0xa8U;
    }
#if defined(RN_CM0_PLATFORM)
    if (chipid == CHIP_RA9703A) {
#else
    {
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
        MEM32(RTC_LOSC_CFG1_ADDR) = 0x8000U;
        if ((losc_cfg1_bak & 0x7fffU) == 0x75a8U) {
            MEM32(RTC_LOSC_CFG1_ADDR) = losc_cfg1_bak;
        } else {
            MEM32(RTC_LOSC_CFG1_ADDR) = losc_cfg1_bak & (~(unsigned int)0x7fffU);
        }
#endif
#if defined(RN_CM0_PLATFORM)
    } else {
#endif
#if !defined(RN202x_RN7326_SOC_V2) &&\
    !defined(RN202x_RN7326_SOC_B)
        MEM32(RTC_LOSC_CFG1_ADDR) = 0x100U;
        if ((losc_cfg1_bak & 0xffU) == 0xa8U) {
            MEM32(RTC_LOSC_CFG1_ADDR) = losc_cfg1_bak;
        } else {
            MEM32(RTC_LOSC_CFG1_ADDR) = losc_cfg1_bak & (~(unsigned int)0xffU);
        }
#endif
    }

#if defined(RN_CM0_PLATFORM)
    if ((chipid == CHIP_RA9103E) || (chipid == CHIP_RA9103F) || (chipid == CHIP_RA9701A) || (chipid == CHIP_RA9701B)) {
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3)
        MEM32(RTC_IOMODE_ADDR) = rtc_iomode_bak | ((unsigned int)0xbU << 4);
        MEM32(RTC_IOEN_ADDR) = rtc_ioen_bak;
        MEM32(RTC_IOMODE_ADDR) = rtc_iomode_bak;
        MEM32(MADC_ANA_PAD_ADDR) = ana_pad_bak;
#endif
#if defined(RN_CM0_PLATFORM)
    } else {
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
        MEM32(0x4002c100U + 0x120U) = 0x82U;
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2)
        for (i = 0U; i < 11U; i++)
#else
        for (i = 0U; i < 10U; i++)
#endif
        {
            if (i != 8U) {
                MEM32(0x4002c100U + (4U * i)) = vbat_gpio_s[i];
#if defined(RN_CM0_PLATFORM)
                if ((chipid == CHIP_RA9303B) && (i == 10U)) {
                    break;
                }
#endif
            }
        }
        MEM32(0x4002c100U + 0x120U) = vbat_gpio_s[8];
#endif
#if defined(RN_CM0_PLATFORM)
    }
#endif
}

#if defined(RN_CM0_PLATFORM)
void open_dig5(rn_chipid_t chipid)
#else
void open_dig5(void)
#endif
{
#if defined(RN_CM0_PLATFORM)
    if ((CHIP_RA9701A == chipid) ||\
        (CHIP_RA9701B == chipid) ||\
        (CHIP_RA9702A == chipid) ||\
        (CHIP_RA9702B == chipid) ||\
        (CHIP_RA9703A == chipid))
    {
        if (CHIP_RA9701A == chipid) {
            MEM32(ANA_SADDR_ADDR(chipid)) = 0xa5470000U | ((unsigned int)2U << 8) | (unsigned int)0x55U;
            MEM32(ANA_SADDR_ADDR(chipid)) = 0xa5470000U | ((unsigned int)2U << 8) | (unsigned int)0x55U | ((unsigned int)0x23U << 10);
        } else {
            MEM32(ANA_SADDR_ADDR(chipid)) = 0xa5470000U | 0x38U;
            MEM32(ANA_SDATA_ADDR(chipid)) = 0xa5470000U | 0x55aaU;
        }
#elif defined(RN831x_RN861x_MCU_V2)
    MEM32(ANA_SADDR_9701_ADDR) = 0xa5470000U | ((unsigned int)2U << 8) | (unsigned int)0x55U;
    MEM32(ANA_SADDR_9701_ADDR) = 0xa5470000U | ((unsigned int)2U << 8) | 0x55U | ((unsigned int)0x23U << 10);
#elif defined(RN202x_RN7326_SOC_V2)
    MEM32(ANA_SADDR_9703_ADDR) = 0xa5470000U | 0x38U;
    MEM32(ANA_SDATA_9703_ADDR) = 0xa5470000U | 0x55aaU;
#elif defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
    MEM32(ANA_SADDR_9701_ADDR) = 0xa5470000U | 0x38U;
    MEM32(ANA_SDATA_9103_ADDR) = 0xa5470000U | 0x55aaU;
#endif
#if defined(RN_CM0_PLATFORM)
    }
#endif
}

#if defined(RN_CM0_PLATFORM)
void close_dig5(rn_chipid_t chipid)
#else
void close_dig5(void)
#endif
{
#if defined(RN_CM0_PLATFORM)
    if ((CHIP_RA9701A == chipid) ||\
        (CHIP_RA9701B == chipid) ||\
        (CHIP_RA9702A == chipid) ||\
        (CHIP_RA9702B == chipid) ||\
        (CHIP_RA9703A == chipid))
    {
        if (CHIP_RA9701A == chipid) {
            MEM32(ANA_SADDR_ADDR(chipid)) = 0xa5470000U | ((unsigned int)2U << 8);
            MEM32(ANA_SADDR_ADDR(chipid)) = 0xa5470000U | ((unsigned int)2U << 8) | ((unsigned int)0x23U << 10);
        } else {
            MEM32(ANA_SADDR_ADDR(chipid)) = 0xa5470000U | (unsigned int)0x38U;
            MEM32(ANA_SDATA_ADDR(chipid)) = 0xa5470000U | (unsigned int)0xaa55U;
        }
#elif defined(RN831x_RN861x_MCU_V2)
    MEM32(ANA_SADDR_9701_ADDR) = 0xa5470000U | ((unsigned int)2U << 8);
    MEM32(ANA_SADDR_9701_ADDR) = 0xa5470000U | ((unsigned int)2U << 8) | ((unsigned int)0x23U << 10);
#elif defined(RN202x_RN7326_SOC_V2)
    MEM32(ANA_SADDR_9701_ADDR) = 0xa5470000U | 0x38U;
    MEM32(ANA_SDATA_9103_ADDR) = 0xa5470000U | 0xaa55U;
#elif defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
    MEM32(ANA_SADDR_9703_ADDR) = 0xa5470000U | 0x38U;
    MEM32(ANA_SDATA_9703_ADDR) = 0xa5470000U | 0xaa55U;
#endif
#if defined(RN_CM0_PLATFORM)
    }
#endif
}

#if defined(RN831x_RN861x_MCU_V1)
rn_chipid_t get_mcu_v1_version(void)
{
    unsigned int retry, die = 0x0U, sysps_bak;

    sysps_bak = MEM32(SYS_PS_9103_ADDR);
    for (retry = 0U; retry < 3U; retry++) {
        MEM32(SYS_PS_9103_ADDR) = 0x75U;
        die = MEM32(ANA_DIEVER_9103_ADDR);
        if ((die == 0xe0U) || (die == 0xf0U)) {
            break;
        }
    }
    MEM32(SYS_PS_9103_ADDR) = sysps_bak;

    return (die == 0xe0U) ? CHIP_RA9103E : ((die == 0xf0U) ? CHIP_RA9103F : CHIP_INVALID);
}
#endif

#if 0
#if defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
rn_chipid_t get_three_phase_soc_version(void)
{
    unsigned int retry, die1 = 0U, die2 = 0U, sysps_bak;

    sysps_bak = MEM32(SYS_PS_9303_ADDR);
    for (retry = 0U; retry < 3U; retry++) {
        MEM32(SYS_PS_9103_ADDR) = 0x75U;
        die1 = MEM32(ANA_DIEVER_9103_ADDR);
        die2 = MEM32(ANA_DIEVER_9303_ADDR);
        if ((die1 == 0xd0U) || (die2 == 0xb0U)) {
            break;
        }
    }
    MEM32(SYS_PS_9303_ADDR) = sysps_bak;

    return (die1 == 0xd0U) ? CHIP_RA9703A : ((die2 == 0xb0U) ? CHIP_RA9303B : CHIP_INVALID);
}
#endif
#endif

#if defined(RN_CM0_PLATFORM)
unsigned char get_sysreg_reliable(rn_chipid_t chipid, unsigned int addr, unsigned int *data, unsigned char flag)
#else
unsigned char get_sysreg_reliable(unsigned int addr, unsigned int *data, unsigned char flag)
#endif
{
    unsigned int sysps_bak;
    unsigned int temp[3];
    unsigned char retry;
    unsigned char res = 0U;

#if defined(RN_CM0_PLATFORM)
    sysps_bak = MEM32(SYS_PS_ADDR(chipid));
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    sysps_bak = MEM32(SYS_PS_9303_ADDR);
#else
    sysps_bak = MEM32(SYS_PS_9103_ADDR);
#endif
    for (retry = 0U; retry < 3U; retry++) {
#if defined(RN_CM0_PLATFORM)
        MEM32(SYS_PS_ADDR(chipid)) = (flag == 0U) ? (unsigned int)0x82U : (unsigned int)0x75U;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
        MEM32(SYS_PS_9303_ADDR) = (flag == 0U) ? (unsigned int)0x82U : (unsigned int)0x75U;
#else
        MEM32(SYS_PS_9103_ADDR) = (flag == 0U) ? (unsigned int)0x82U : (unsigned int)0x75U;
#endif
        temp[retry] = MEM32(addr);
    }

    for (retry = 1U; retry < 3U; retry++) {
        if (temp[retry] != temp[retry - 1U]) {
            res = 1U;
            break;
        }
    }

    if (res == 0U) {
        *data = temp[0];
    }

#if defined(RN_CM0_PLATFORM)
    MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#else
    MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif
    return res;
}

#if defined(RN_CM0_PLATFORM)
unsigned char set_sysreg_reliable(rn_chipid_t chipid, unsigned int addr, unsigned int data, unsigned int mask, unsigned char flag)
#else
unsigned char set_sysreg_reliable(unsigned int addr, unsigned int data, unsigned int mask, unsigned char flag)
#endif
{
    unsigned int sysps_bak;
    unsigned int temp;
    unsigned char retry;
    unsigned char res = 1U;

#if defined(RN_CM0_PLATFORM)
    sysps_bak = MEM32(SYS_PS_ADDR(chipid));
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    sysps_bak = MEM32(SYS_PS_9303_ADDR);
#else
    sysps_bak = MEM32(SYS_PS_9103_ADDR);
#endif
    for (retry = 0U; retry < 3U; retry++) {
#if defined(RN_CM0_PLATFORM)
        MEM32(SYS_PS_ADDR(chipid)) = (flag == 0U) ? (unsigned int)0x82U : (unsigned int)0x75U;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
        MEM32(SYS_PS_9303_ADDR) = (flag == 0U) ? (unsigned int)0x82U : (unsigned int)0x75U;
#else
        MEM32(SYS_PS_9103_ADDR) = (flag == 0U) ? (unsigned int)0x82U : (unsigned int)0x75U;
#endif
        MEM32(addr) = data;
        temp = MEM32(addr) & mask;
        if (temp == (data & mask)) {
            res = 0U;
            break;
        }
    }

#if defined(RN_CM0_PLATFORM)
    MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#else
    MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif
    return res;
}

#if defined(RN831x_RN861x_MCU_V2) && defined(NVRLIB_EN)
const rn_lib_t  *rn_lib = (rn_lib_t  *)0x80600;
#elif defined(RN831x_RN861x_MCU_V3) && defined(NVRLIB_EN)
const rn_lib_t  *rn_lib = (rn_lib_t  *)0x40600;
#elif defined(RN821x_RN721x_SOC_V2) && defined(NVRLIB_EN)
const rn_lib_t  *rn_lib = (rn_lib_t  *)0x80600;
#elif defined(RN821x_RN721x_SOC_V3) && defined(NVRLIB_EN)
const rn_lib_t  *rn_lib = (rn_lib_t  *)0x40600;
#elif defined(RN202x_RN7326_SOC_V2) && defined(NVRLIB_EN)
const rn_lib_t  *rn_lib = (rn_lib_t  *)0x80600;
#endif
#else
IapCheckStatus getIAPHeaderAddress(uint32_t *Address)
{
    IapCheckStatus status = IAP_PASS;
    uint32_t iap_addr_offset = *(volatile uint32_t*)(ROM_BASE_ADDR + RN_LIB_OFFSET);
    if ((iap_addr_offset < ROM_SIZE) && ((iap_addr_offset & 0x1) == 0U)) {
        *Address = (uint32_t)(iap_addr_offset + ROM_BASE_ADDR);
    } else {
        status = IAP_FAIL;
    }
    return status;
}
#endif
/* r2989 */
