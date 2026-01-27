/******************************************************************************
 * @file     clk_trim.c
 * @brief    trim clock to specified frequency
 * @author   Renergy Technology
 * @note
 * Copyright (C) 2024, Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#include "stddef.h"
#include "rn8xxx_ll_utils.h"
#include "rn8xxx_ll_clktrim.h"
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3)

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2)
#define RCH32_TARGET_FREQUENCY         (32768000U)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3)
#define RCH29_TARGET_FREQUENCY         (29491200U)
#endif
#define LOSC_SRC_FREQENCY             (32768U)
#define RCH_TRIM_SRC_CLK_NUM          (5U)
#if (RCH_TRIM_SRC_CLK_NUM < 3U)
    #error "rch trim src clock must exceed 3!"
#endif
#define RCH_TRIM_LOOPS                (3U)
/* 153us, 5013ticks@32M */
#define RCH_TRIM_TIMEOUT              (1250U)

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2)
#define RCH32_CLK_DESIRED_BASE_VALUE  ((unsigned int)(RCH32_TARGET_FREQUENCY / LOSC_SRC_FREQENCY))
#define RCH32_CLK_DESIRED_VALUE       ((unsigned int)(RCH_TRIM_SRC_CLK_NUM * RCH32_CLK_DESIRED_BASE_VALUE))
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3)
#define RCH29_CLK_DESIRED_BASE_VALUE  ((unsigned int)(RCH29_TARGET_FREQUENCY / LOSC_SRC_FREQENCY))
#define RCH29_CLK_DESIRED_VALUE       ((unsigned int)(RCH_TRIM_SRC_CLK_NUM * RCH29_CLK_DESIRED_BASE_VALUE))
#endif
/* 单次测量的有效范围，定义为±3.5%范围，软件实现的单位是RCH时钟数 */
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2)
#define RCH32_CLK_VALID_BASE_RANGE    ((unsigned int)((float)RCH32_CLK_DESIRED_BASE_VALUE * 0.035))
#define RCH32_CLK_VALID_RANGE         ((unsigned int)((float)RCH32_CLK_DESIRED_VALUE * 0.035))
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3)
#define RCH29_CLK_VALID_BASE_RANGE    ((unsigned int)((float)RCH29_CLK_DESIRED_BASE_VALUE * 0.035))
#define RCH29_CLK_VALID_RANGE         ((unsigned int)((float)RCH29_CLK_DESIRED_VALUE * 0.035))
#endif
/* trim启动阈值，定义为±0.3%，软件实现的单位是RCH时钟数 */
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2)
#define RCH32_TRIM_START_THRD         ((unsigned int)((float)RCH32_CLK_DESIRED_VALUE * 0.001))
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3)
#define RCH29_TRIM_START_THRD         ((unsigned int)((float)RCH29_CLK_DESIRED_VALUE * 0.001))
#endif
/* 多次测量间的误差跳差最大值，单位是RCH时钟数 */
#define RCH_TRIM_JITTER_MAX           (5U)
/* 一个RCH CLK对应的误差绝对值，单位十万分之一 */
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2)
#define RCH32_TRIM_STEP_ERR           ((unsigned int)(100000U / RCH32_CLK_DESIRED_VALUE))
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3)
#define RCH29_TRIM_STEP_ERR           ((unsigned int)(100000U / RCH29_CLK_DESIRED_VALUE))
#endif

/* 快速收敛的误差值，定义为±0.4%， 单位是RCH时钟数 */
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2)
#define RCH32_TRIM_FAST_ERR_MIN       ((unsigned int)((float)RCH32_CLK_DESIRED_BASE_VALUE * 0.004))
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3)
#define RCH29_TRIM_FAST_ERR_MIN       ((unsigned int)((float)RCH29_CLK_DESIRED_BASE_VALUE * 0.004))
#endif
#define RCH_TRIM_FAST_LOOPS           (20U)

#define RCL_TARGET_FREQUENCY          (38400U)
#define RCL_TRIM_SRC_CLK_NUM_RCH29M   (768000U)
#define RCL_TRIM_SRC_CLK_NUM_RCH32M   (53248U)
#define RCL_TRIM_LOOPS                (2U)
/* 26ms, 5013ticks@32M */
#define RCL_CLK_DESIRED_VALUE_RCH29M  ((unsigned int)((float)RCL_TRIM_SRC_CLK_NUM_RCH29M * (float)RCL_TARGET_FREQUENCY / (float)RCH29_TARGET_FREQUENCY))
#define RCL_CLK_DESIRED_VALUE_RCH32M  ((unsigned int)(RCL_TRIM_SRC_CLK_NUM_RCH32M * RCL_TARGET_FREQUENCY / 2048000U))
/* range: ±40% */
#define RCL_CLK_VALID_RANGE_RCH29M    ((unsigned int)((float)RCL_CLK_DESIRED_VALUE_RCH29M * 0.40))
#define RCL_CLK_VALID_RANGE_RCH32M    ((unsigned int)((float)RCL_CLK_DESIRED_VALUE_RCH32M * 0.40))
/* start thrd: 0.4% */
#define RCL_TRIM_START_THRD_RCH29M    ((unsigned int)((float)RCL_CLK_DESIRED_VALUE_RCH29M * 0.002))
#define RCL_TRIM_START_THRD_RCH32M    ((unsigned int)((float)RCL_CLK_DESIRED_VALUE_RCH32M * 0.002))
/* jitter max:10 rcl clock */
#define RCL_TRIM_JITTER_MAX           (10U)

#define PLL_TRIM_RCH_SRC_CLK_NUM_ASYNC     (1043332U)
#define PLL_TRIM_RCH_SRC_CLK_NUM_SYNC      (32768U)

#define CLK_TRIM_STOP_BIT             (27U)
#define CLK_TRIM_START_BIT            (26U)
#define CLK_TRIM_OV_BIT               (25U)
#define CLK_TRIM_DONE_BIT             (24U)

#define RCL_FLAG_BIT                  (14U)
#define RCH_FLAG_BIT                  (13U)
#define PLL_FLAG_BIT                  (12U)
#define LOSC_FLAG_BIT                 (11U)
#define RCH_PWRON_BIT                 (1U)

#define SYSREG_CLK3_PASSWD            (0xa5750000U)
#define SYSREG_CLK3_DEFAULT           (0x216U)

#define SYSREG_CLK2_PASSWD            (0xa5360000U)
#define SYSREG_CLK2_DEFAULT           (0x64cU)

static unsigned int i_err_calc(unsigned int a, unsigned int b)
{
    return (a > b) ? (a - b) : (b - a);
}

/* flag = 0, return max, flag = 1, return min */
static unsigned int imax_min(unsigned int *data, unsigned int len, unsigned int flag)
{
    unsigned int i = 0U;
    unsigned int data_max = data[0], data_min = data[0];

    for (i = 1U; i < len; i++) {
        if (data[i] > data_max) {
            data_max = data[i];
        }
        if (data[i] < data_min) {
            data_min = data[i];
        }
    }

    return (flag == 0U) ? data_max : data_min;
}

typedef enum {
    CALCLK_RCH = 0,
    CALCLK_RCL = 1
} eClkTrimCalcClk_TypeDef;

typedef enum {
    REFCLK_LOSC = 0,
    REFCLK_HOSC = 1,
    REFCLK_RCH = 2,
    REFCLK_PLL = 3
} eClkTrimRefClk_TypeDef;

/**
 * @brief configure and start clk_trim module
 *
 * @param chipid chipid
 * @param ref_clk 0:losc, 1:hosc, 2:rch, 3:pll
 * @param ref_cnt count of ref clk
 * @param calc_clk 0:rch,  1:rcl
 * @param time_out time out, unit: sysclk ticks
 * @param irq_en 0: no irq, 1: irq
 * @return unsigned int 0: timeout(or irq enable), others = calc_cnt
 */
#if defined(RN_CM0_PLATFORM)
static unsigned int clk_trim_set(rn_chipid_t chipid, eClkTrimRefClk_TypeDef ref_clk, unsigned int ref_cnt, eClkTrimCalcClk_TypeDef calc_clk, unsigned int time_out, unsigned char irq_en)
#else
static unsigned int clk_trim_set(eClkTrimRefClk_TypeDef ref_clk, unsigned int ref_cnt, eClkTrimCalcClk_TypeDef calc_clk, unsigned int time_out, unsigned char irq_en)
#endif
{
    unsigned int sysps_bak, trim_cfg_bak, clk_trim_restore;
    unsigned int i;

#if defined(RN_CM0_PLATFORM)
    sysps_bak = MEM32(SYS_PS_ADDR(chipid));
    MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
    if (((MEM32(TRIM_START_ADDR(chipid)) >> CLK_TRIM_DONE_BIT) & 0x7U) == 0x0U)
#elif defined(RN202x_RN7326_SOC_V2)
    sysps_bak = MEM32(SYS_PS_9303_ADDR);
    MEM32(SYS_PS_9303_ADDR) = 0x82U;
    if (((MEM32(TRIM_START_9303_ADDR) >> CLK_TRIM_DONE_BIT) & 0x7U) == 0x0U)
#else
    sysps_bak = MEM32(SYS_PS_9103_ADDR);
    MEM32(SYS_PS_9103_ADDR) = 0x82U;
    if (((MEM32(TRIM_START_9103_ADDR) >> CLK_TRIM_DONE_BIT) & 0x7U) == 0x0U)
#endif
    {
        clk_trim_restore = 0U;
    } else {
        clk_trim_restore = 1U;
    }
#if defined(RN_CM0_PLATFORM)
    trim_cfg_bak = MEM32(TRIM_CFG1_ADDR(chipid));
#elif defined(RN202x_RN7326_SOC_V2)
    trim_cfg_bak = MEM32(TRIM_CFG1_9303_ADDR);
#else
    trim_cfg_bak = MEM32(TRIM_CFG1_9103_ADDR);
#endif

    /* clear status */
#if defined(RN_CM0_PLATFORM)
    MEM32(TRIM_START_ADDR(chipid)) = ((unsigned int)1U << CLK_TRIM_STOP_BIT) | ((unsigned int)1U << CLK_TRIM_OV_BIT) | ((unsigned int)1U << CLK_TRIM_DONE_BIT);
    MEM32(TRIM_CFG1_ADDR(chipid)) = ref_cnt | ((unsigned int)ref_clk << 24) | ((((chipid == CHIP_RA9703A) && (calc_clk == CALCLK_RCH)) ? 0x10U : (unsigned int)calc_clk) << 26) | (irq_en << 27) | (irq_en << 28);
    MEM32(TRIM_START_ADDR(chipid)) = (unsigned int)1U << CLK_TRIM_START_BIT;
#elif defined(RN202x_RN7326_SOC_V2)
    MEM32(TRIM_START_9303_ADDR) = ((unsigned int)1U << CLK_TRIM_STOP_BIT) | ((unsigned int)1U << CLK_TRIM_OV_BIT) | ((unsigned int)1U << CLK_TRIM_DONE_BIT);
    MEM32(TRIM_CFG1_9303_ADDR) = ref_cnt | ((unsigned int)ref_clk << 24) | (((calc_clk == CALCLK_RCH) ? 0x10U : (unsigned int)calc_clk) << 26) | (irq_en << 27) | (irq_en << 28);
    MEM32(TRIM_START_9303_ADDR) = (unsigned int)1U << CLK_TRIM_START_BIT;
#else
    MEM32(TRIM_START_9103_ADDR) = ((unsigned int)1U << CLK_TRIM_STOP_BIT) | ((unsigned int)1U << CLK_TRIM_OV_BIT) | ((unsigned int)1U << CLK_TRIM_DONE_BIT);
    MEM32(TRIM_CFG1_9103_ADDR) = ref_cnt | ((unsigned int)ref_clk << 24) | ((unsigned int)calc_clk << 26) | (irq_en << 27) | (irq_en << 28);
    MEM32(TRIM_START_9103_ADDR) = (unsigned int)1U << CLK_TRIM_START_BIT;
#endif
    if (irq_en == 0U) {
        for (i = 0U; i < time_out; i++) {
#if defined(RN_CM0_PLATFORM)
            if (((MEM32(TRIM_START_ADDR(chipid)) >> CLK_TRIM_DONE_BIT) & 0x1U) == 0x1U)
#elif defined(RN202x_RN7326_SOC_V2)
            if (((MEM32(TRIM_START_9303_ADDR) >> CLK_TRIM_DONE_BIT) & 0x1U) == 0x1U)
#else
            if (((MEM32(TRIM_START_9103_ADDR) >> CLK_TRIM_DONE_BIT) & 0x1U) == 0x1U)
#endif
            {
                break;
            }
        }

#if defined(RN_CM0_PLATFORM)
        if ((((MEM32(TRIM_START_ADDR(chipid)) >> CLK_TRIM_OV_BIT) & 0x1U) == 0x1U) || (i == time_out))
#elif defined(RN202x_RN7326_SOC_V2)
        if ((((MEM32(TRIM_START_9303_ADDR) >> CLK_TRIM_OV_BIT) & 0x1U) == 0x1U) || (i == time_out))
#else
        if ((((MEM32(TRIM_START_9103_ADDR) >> CLK_TRIM_OV_BIT) & 0x1U) == 0x1U) || (i == time_out))
#endif
        {
            i = 0x0U;
        } else {
#if defined(RN_CM0_PLATFORM)
            i = MEM32(TRIM_START_ADDR(chipid)) & 0xfffffU;
#elif defined(RN202x_RN7326_SOC_V2)
            i = MEM32(TRIM_START_9303_ADDR) & 0xfffffU;
#else
            i = MEM32(TRIM_START_9103_ADDR) & 0xfffffU;
#endif
        }
        /* clear status */
#if defined(RN_CM0_PLATFORM)
        MEM32(TRIM_START_ADDR(chipid)) = ((unsigned int)1U << CLK_TRIM_STOP_BIT) | ((unsigned int)1U << CLK_TRIM_OV_BIT) | ((unsigned int)1U << CLK_TRIM_DONE_BIT);
#elif defined(RN202x_RN7326_SOC_V2)
        MEM32(TRIM_START_9303_ADDR) = ((unsigned int)1U << CLK_TRIM_STOP_BIT) | ((unsigned int)1U << CLK_TRIM_OV_BIT) | ((unsigned int)1U << CLK_TRIM_DONE_BIT);
#else
        MEM32(TRIM_START_9103_ADDR) = ((unsigned int)1U << CLK_TRIM_STOP_BIT) | ((unsigned int)1U << CLK_TRIM_OV_BIT) | ((unsigned int)1U << CLK_TRIM_DONE_BIT);
#endif
        if (clk_trim_restore == 1U) {
#if defined(RN_CM0_PLATFORM)
            MEM32(TRIM_CFG1_ADDR(chipid)) = trim_cfg_bak;
            MEM32(TRIM_START_ADDR(chipid)) = (unsigned int)1U << CLK_TRIM_START_BIT;
#elif defined(RN202x_RN7326_SOC_V2)
            MEM32(TRIM_CFG1_9303_ADDR) = trim_cfg_bak;
            MEM32(TRIM_START_9303_ADDR) = (unsigned int)1U << CLK_TRIM_START_BIT;
#else
            MEM32(TRIM_CFG1_9103_ADDR) = trim_cfg_bak;
            MEM32(TRIM_START_9103_ADDR) = (unsigned int)1U << CLK_TRIM_START_BIT;
#endif
        }
    } else {
        i = 0x0U;
    }
#if defined(RN_CM0_PLATFORM)
    MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
#elif defined(RN202x_RN7326_SOC_V2)
    MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#else
    MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif

    return i;
}

/* 153us=282ticks@1.84M, 282/4=70(4ticks per loop) */
#define  RCH_BASE_TIMEOUT   (70U)

#if defined(RN_CM0_PLATFORM)
static eClkTrimRet_TypeDef rch_multi_calc(const rn_chipid_t chipid, unsigned int *data)
#else
static eClkTrimRet_TypeDef rch_multi_calc(unsigned int *data)
#endif
{
    unsigned char loops;
    unsigned int rch_cnt[RCH_TRIM_LOOPS];
#if defined(RN_CM0_PLATFORM)
    const unsigned int rch_desired_value = (chipid == CHIP_RA9703A) ? RCH32_CLK_DESIRED_VALUE : RCH29_CLK_DESIRED_VALUE;
    const unsigned int rch_valid_range = (chipid == CHIP_RA9703A) ? RCH32_CLK_VALID_RANGE : RCH29_CLK_VALID_RANGE;
#elif defined(RN202x_RN7326_SOC_V2)
    const unsigned int rch_desired_value = RCH32_CLK_DESIRED_VALUE;
    const unsigned int rch_valid_range = RCH32_CLK_VALID_RANGE;
#else
    const unsigned int rch_desired_value = RCH29_CLK_DESIRED_VALUE;
    const unsigned int rch_valid_range = RCH29_CLK_VALID_RANGE;
#endif
    unsigned int temp;
    eClkTrimRet_TypeDef res;

    for (loops = 0U; loops < RCH_TRIM_LOOPS; loops++) {
#if defined(RN_CM0_PLATFORM)
        rch_cnt[loops] = clk_trim_set(chipid, REFCLK_LOSC, RCH_TRIM_SRC_CLK_NUM - 1U, CALCLK_RCH, RCH_BASE_TIMEOUT << currentClk_idx(), 0U);
#else
        rch_cnt[loops] = clk_trim_set(REFCLK_LOSC, RCH_TRIM_SRC_CLK_NUM - 1U, CALCLK_RCH, RCH_BASE_TIMEOUT << currentClk_idx(), 0U);
#endif
        if (rch_cnt[loops] == 0U) {
            res = CLK_TRIM_TIMEOUT;
        } /*else if (i_err_calc(rch_cnt[loops], rch_desired_value) > rch_valid_range) {
            res = CLK_TRIM_OUT_OF_RANGE;
        }*/ else {
            res = CLK_TRIM_PASS;
        }
        if (res != CLK_TRIM_PASS) {
            break;
        }
    }

    if (res == CLK_TRIM_PASS) {
        temp = imax_min(rch_cnt, RCH_TRIM_LOOPS, 0U);
        temp -= imax_min(rch_cnt, RCH_TRIM_LOOPS, 1U);
        if (temp > RCH_TRIM_JITTER_MAX) {
            res = CLK_TRIM_UNSTABLE;
        } else {
            *data = rch_cnt[0];
            for (loops = 1U; loops < RCH_TRIM_LOOPS; loops++) {
                *data = (*data + rch_cnt[loops]) / 2U;
            }
        }
    }

    if (res == CLK_TRIM_PASS) {
        if (i_err_calc(*data, rch_desired_value) > rch_valid_range) {
            res = CLK_TRIM_OUT_OF_RANGE;
        }
    }

    return res;
}

/* flag = 0, up step, flag = 1, down step */
#if defined(RN_CM0_PLATFORM)
static unsigned char set_rch_clk_trim_single(rn_chipid_t chipid, unsigned char flag)
#else
static unsigned char set_rch_clk_trim_single(unsigned char flag)
#endif
{
    unsigned int reg_prev;
    unsigned char res = 1U;

#if defined(RN_CM0_PLATFORM)
    if (get_sysreg_reliable(chipid, ANA_CLK3_ADDR, &reg_prev, 1U) == 0U)
#else
    if (get_sysreg_reliable(ANA_CLK3_ADDR, &reg_prev, 1U) == 0U)
#endif
    {
        reg_prev = (flag == 0U) ? (reg_prev + 1U) : (reg_prev - 1U);
#if defined(RN_CM0_PLATFORM)
        if (set_sysreg_reliable(chipid, ANA_CLK3_ADDR, SYSREG_CLK3_PASSWD + reg_prev, 0x3ffU, 1U) == 0U)
#else
        if (set_sysreg_reliable(ANA_CLK3_ADDR, SYSREG_CLK3_PASSWD + reg_prev, 0x3ffU, 1U) == 0U)
#endif
        {
            res = 0U;
        }
    }

    return res;
}

#define  SPL_INVALID_MAX   (10U)
#if defined(RN_CM0_PLATFORM)
static eClkTrimRet_TypeDef set_rch_trim_by_default(rn_chipid_t chipid)
#else
static eClkTrimRet_TypeDef set_rch_trim_by_default(void)
#endif
{
#if defined(RN_CM0_PLATFORM)
    const unsigned int nvr_base_addr = ((chipid == CHIP_RA9701B) || (chipid == CHIP_RA9702B)) ? 0x00040000U : 0x00080000U;
#elif defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V3)
    const unsigned int nvr_base_addr = 0x00040000U;
#else
    const unsigned int nvr_base_addr = 0x00080000U;
#endif
    unsigned int temp = 0xffffffffU;
    unsigned char res = 0U;
    static unsigned char spl_invalid_cnt = 0U;

    if (check_spl_valid((unsigned int *)nvr_base_addr, 512U, 0U) == 1U) {
        /*temp = *(volatile unsigned short *)(nvr_base_addr + 0x1eU);*/
#if defined(RN_CM0_PLATFORM)
        (void)get_sysreg_reliable(chipid, nvr_base_addr + 0x1cU, &temp, 0U);
#else
        (void)get_sysreg_reliable(nvr_base_addr + 0x1cU, &temp, 0U);
#endif
        temp = (unsigned int)temp >> 16;
        spl_invalid_cnt = 0U;
    } else {
        spl_invalid_cnt++;
        if (spl_invalid_cnt > SPL_INVALID_MAX) {
            spl_invalid_cnt = 0U;
            temp = SYSREG_CLK3_DEFAULT;
        }
    }

    if (temp != 0xffffffffU) {
#if defined(RN_CM0_PLATFORM)
        res = set_sysreg_reliable(chipid, ANA_CLK3_ADDR, SYSREG_CLK3_PASSWD + temp, 0x3ffU, 1U);
#else
        res = set_sysreg_reliable(ANA_CLK3_ADDR, SYSREG_CLK3_PASSWD + temp, 0x3ffU, 1U);
#endif
    }
    return (res == 0U) ? CLK_TRIM_PASS : CLK_TRIM_TIMEOUT;
}

/* flag = 0, up step, flag = 1, down step */
#if defined(RN_CM0_PLATFORM)
static unsigned char set_rch_clk_trim_fast(rn_chipid_t chipid, unsigned char flag)
#else
static unsigned char set_rch_clk_trim_fast(unsigned char flag)
#endif
{
    unsigned int reg_prev;
    unsigned int temp_freq;
    unsigned int temp;
#if defined(RN_CM0_PLATFORM)
    const unsigned int rch_desired_value = (chipid == CHIP_RA9703A) ? RCH32_CLK_DESIRED_BASE_VALUE : RCH29_CLK_DESIRED_BASE_VALUE;
    const unsigned int rch_valid_range = (chipid == CHIP_RA9703A) ? RCH32_CLK_VALID_BASE_RANGE : RCH29_CLK_VALID_BASE_RANGE;
    const unsigned int rch_fast_err_min = (chipid == CHIP_RA9703A) ? RCH32_TRIM_FAST_ERR_MIN : RCH29_TRIM_FAST_ERR_MIN;
#elif defined(RN202x_RN7326_SOC_V2)
    const unsigned int rch_desired_value = RCH32_CLK_DESIRED_BASE_VALUE;
    const unsigned int rch_valid_range = RCH32_CLK_VALID_BASE_RANGE;
    const unsigned int rch_fast_err_min = RCH32_TRIM_FAST_ERR_MIN;
#else
    const unsigned int rch_desired_value = RCH29_CLK_DESIRED_BASE_VALUE;
    const unsigned int rch_valid_range = RCH29_CLK_VALID_BASE_RANGE;
    const unsigned int rch_fast_err_min = RCH29_TRIM_FAST_ERR_MIN;
#endif
    unsigned char retry;
    unsigned char res = 1U;

#if defined(RN_CM0_PLATFORM)
    if (get_sysreg_reliable(chipid, ANA_CLK3_ADDR, &reg_prev, 1U) == 0U)
#else
    if (get_sysreg_reliable(ANA_CLK3_ADDR, &reg_prev, 1U) == 0U)
#endif
    {
        temp = ((unsigned int)reg_prev >> 6) & 0xfU;
        if (((flag == 0U) && (temp != 0xfU)) ||
            ((flag == 1U) && (temp != 0x0U))) {
            /* 粗调档位修改，细调档位根据初始偏差决定：
                 当初始偏差为正，即时钟偏快，此时细调档位偏慢16步，以防调快导致系统异常；
                 当初始为负，即时钟偏慢，此时细调档位不再偏慢，而是调到典型中心值位置，防止偏慢后超出量程 */
            temp = ((flag == 0U) ? (temp + 1U) : (temp - 1U)) << 6;
            /*temp += (flag == 0U) ? 0x20U : 0x20U;*/
            temp += 0x20U;
#if defined(RN_CM0_PLATFORM)
            if (set_sysreg_reliable(chipid, ANA_CLK3_ADDR, SYSREG_CLK3_PASSWD + temp, 0x3ffU, 1U) == 0U)
#else
            if (set_sysreg_reliable(ANA_CLK3_ADDR, SYSREG_CLK3_PASSWD + temp, 0x3ffU, 1U) == 0U)
#endif
            {
#if defined(RN_CM0_PLATFORM)
                temp_freq = clk_trim_set(chipid, REFCLK_LOSC, 0U, CALCLK_RCH, RCH_BASE_TIMEOUT << currentClk_idx(), 0U);
#else
                temp_freq = clk_trim_set(REFCLK_LOSC, 0U, CALCLK_RCH, RCH_BASE_TIMEOUT << currentClk_idx(), 0U);
#endif
                res = 2U;
                if (i_err_calc(temp_freq, rch_desired_value) < rch_valid_range) {
                    flag = (temp_freq > rch_desired_value) ? 1U : 0U;
                    for (retry = 0U; retry < RCH_TRIM_FAST_LOOPS; retry++) {
#if defined(RN_CM0_PLATFORM)
                        if (set_rch_clk_trim_single(chipid, flag) != 1U)
#else
                        if (set_rch_clk_trim_single(flag) != 1U)
#endif
                        {
#if defined(RN_CM0_PLATFORM)
                            temp_freq = clk_trim_set(chipid, REFCLK_LOSC, 0U, CALCLK_RCH, RCH_BASE_TIMEOUT << currentClk_idx(), 0U);
#else
                            temp_freq = clk_trim_set(REFCLK_LOSC, 0U, CALCLK_RCH, RCH_BASE_TIMEOUT << currentClk_idx(), 0U);
#endif
                            /*if (i_err_calc(temp_freq, rch_desired_value) > rch_valid_range) {
                                res = 2U;
                                break;
                            } else */
                            if (i_err_calc(temp_freq, rch_desired_value) < rch_fast_err_min) {
                                res = 0U;
                                break;
                            }
                        }
                    }
                    if (retry == RCH_TRIM_FAST_LOOPS) {
                        res = 1U;
                    }
                }
                if (res == 2U) {
                    /* restore default value! */
                    /* res = 0U; */
#if defined(RN_CM0_PLATFORM)
                    res = (unsigned char)set_rch_trim_by_default(chipid);
#else
                    res = (unsigned char)set_rch_trim_by_default();
#endif
                }
            }
        }
    }

    return res;
}

/* 26ms=47923ticks@1.84M, 47923/4=11980(4ticks per loop) */
#define RCL_BASE_TIMEOUT   (11980U)
#if defined(RN_CM0_PLATFORM)
static eClkTrimRet_TypeDef rcl_multi_calc(rn_chipid_t chipid, unsigned int *data)
#else
static eClkTrimRet_TypeDef rcl_multi_calc(unsigned int *data)
#endif
{
    unsigned char loops;
    unsigned int rcl_cnt[RCL_TRIM_LOOPS];
#if defined(RN_CM0_PLATFORM)
    const unsigned int rcl_desired_value = (chipid == CHIP_RA9703A) ? RCL_CLK_DESIRED_VALUE_RCH32M : RCL_CLK_DESIRED_VALUE_RCH29M;
    const unsigned int rcl_valid_range = (chipid == CHIP_RA9703A) ? RCL_CLK_VALID_RANGE_RCH32M : RCL_CLK_VALID_RANGE_RCH29M;
#elif defined(RN202x_RN7326_SOC_V2)
    const unsigned int rcl_desired_value = RCL_CLK_DESIRED_VALUE_RCH32M;
    const unsigned int rcl_valid_range = RCL_CLK_VALID_RANGE_RCH32M;
#else
    const unsigned int rcl_desired_value = RCL_CLK_DESIRED_VALUE_RCH29M;
    const unsigned int rcl_valid_range = RCL_CLK_VALID_RANGE_RCH29M;
#endif
    unsigned int temp;
    eClkTrimRet_TypeDef res;

    for (loops = 0U; loops < RCL_TRIM_LOOPS; loops++) {
#if defined(RN_CM0_PLATFORM)
        rcl_cnt[loops] = clk_trim_set(chipid, REFCLK_RCH, ((chipid == CHIP_RA9703A) ? RCL_TRIM_SRC_CLK_NUM_RCH32M : RCL_TRIM_SRC_CLK_NUM_RCH29M) - 1U, CALCLK_RCL, RCL_BASE_TIMEOUT << currentClk_idx(), 0U);
#elif defined(RN202x_RN7326_SOC_V2)
        rcl_cnt[loops] = clk_trim_set(REFCLK_RCH, RCL_TRIM_SRC_CLK_NUM_RCH32M - 1U, CALCLK_RCL, RCL_BASE_TIMEOUT << currentClk_idx(), 0U);
#else
        rcl_cnt[loops] = clk_trim_set(REFCLK_RCH, RCL_TRIM_SRC_CLK_NUM_RCH29M - 1U, CALCLK_RCL, RCL_BASE_TIMEOUT << currentClk_idx(), 0U);
#endif
        if (rcl_cnt[loops] == 0U) {
            res = CLK_TRIM_TIMEOUT;
        } /*else if (i_err_calc(rch_cnt[loops], rch_desired_value) > rch_valid_range) {
            res = CLK_TRIM_OUT_OF_RANGE;
        }*/ else {
            res = CLK_TRIM_PASS;
        }
        if (res != CLK_TRIM_PASS) {
            break;
        }
    }

    if (res == CLK_TRIM_PASS) {
        temp = imax_min(rcl_cnt, RCL_TRIM_LOOPS, 0U);
        temp -= imax_min(rcl_cnt, RCL_TRIM_LOOPS, 1U);
        if (temp > RCL_TRIM_JITTER_MAX) {
            // printf("unstable:%d, %d\n", rcl_cnt[0], rcl_cnt[1]);
            res = CLK_TRIM_UNSTABLE;
        } else {
            *data = rcl_cnt[0];
            for (loops = 1U; loops < RCL_TRIM_LOOPS; loops++) {
                *data = (*data + rcl_cnt[loops]) / 2U;
            }
        }
    }

    if (res == CLK_TRIM_PASS) {
        if (i_err_calc(*data, rcl_desired_value) > rcl_valid_range) {
            // printf("out of range:%d, %d\n", rcl_cnt[0], rcl_cnt[1]);
            res = CLK_TRIM_OUT_OF_RANGE;
        }
    }

    return res;
}

/* flag = 0, up step, flag = 1, down step */
#if defined(RN_CM0_PLATFORM)
static unsigned char set_rcl_clk_trim_single(rn_chipid_t chipid, unsigned char flag)
#else
static unsigned char set_rcl_clk_trim_single(unsigned char flag)
#endif
{
    unsigned int reg_prev;
    unsigned char res = 1U;

#if defined(RN_CM0_PLATFORM)
    if (get_sysreg_reliable(chipid, ANA_CLK2_ADDR, &reg_prev, 1U) == 0U)
#else
    if (get_sysreg_reliable(ANA_CLK2_ADDR, &reg_prev, 1U) == 0U)
#endif
    {
        reg_prev = (flag == 0U) ? (reg_prev + 1U) : (reg_prev - 1U);
#if defined(RN_CM0_PLATFORM)
        if (set_sysreg_reliable(chipid, ANA_CLK2_ADDR, SYSREG_CLK2_PASSWD + reg_prev, 0x3fffU, 1U) == 0U)
#else
        if (set_sysreg_reliable(ANA_CLK2_ADDR, SYSREG_CLK2_PASSWD + reg_prev, 0x3fffU, 1U) == 0U)
#endif
        {
            res = 0U;
        }
    }

    return res;
}

#if defined(RN_CM0_PLATFORM)
static unsigned char set_rcl_trim_by_default(rn_chipid_t chipid)
#else
static unsigned char set_rcl_trim_by_default(void)
#endif
{
#if defined(RN_CM0_PLATFORM)
    const unsigned int nvr_base_addr = ((chipid == CHIP_RA9701B) || (chipid == CHIP_RA9702B)) ? 0x00040000U : 0x00080000U;
#elif defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V3)
    const unsigned int nvr_base_addr = 0x00040000U;
#else
    const unsigned int nvr_base_addr = 0x00080000U;
#endif
    unsigned int temp = 0xffffffffU;
    unsigned char res = 0U;
    static unsigned char spl_invalid_cnt = 0U;

    if (check_spl_valid((unsigned int *)nvr_base_addr, 512U, 0U) == 1U) {
        /*temp = *(volatile unsigned short *)(nvr_base_addr + 0x14U);*/
#if defined(RN_CM0_PLATFORM)
        (void)get_sysreg_reliable(chipid, nvr_base_addr + 0x14U, &temp, 0U);
#else
        (void)get_sysreg_reliable(nvr_base_addr + 0x14U, &temp, 0U);
#endif
        temp = (unsigned int)temp & 0xffffU;
        spl_invalid_cnt = 0U;
    } else {
        spl_invalid_cnt++;
        if (spl_invalid_cnt > SPL_INVALID_MAX) {
            spl_invalid_cnt = 0U;
            temp = SYSREG_CLK2_DEFAULT;
        }
    }

    if (temp != 0xffffffffU) {
#if defined(RN_CM0_PLATFORM)
        res = set_sysreg_reliable(chipid, ANA_CLK2_ADDR, SYSREG_CLK2_PASSWD + temp, 0x3fffU, 1U);
#else
        res = set_sysreg_reliable(ANA_CLK2_ADDR, SYSREG_CLK2_PASSWD + temp, 0x3fffU, 1U);
#endif
    }
    return res;
}

eClkTrimRet_TypeDef LL_CLKTRIM_RchTrimByLosc(int *err_thousandth)
{
    unsigned int sysps_bak;
    unsigned int temp;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif
#if defined(RN_CM0_PLATFORM)
    const unsigned int rch_desired_value = (chipid == CHIP_RA9703A) ? RCH32_CLK_DESIRED_VALUE : RCH29_CLK_DESIRED_VALUE;
    const unsigned int rch_start_thrd = (chipid == CHIP_RA9703A) ? RCH32_TRIM_START_THRD : RCH29_TRIM_START_THRD;
    const int rch_step_err = (chipid == CHIP_RA9703A) ? (int)RCH32_TRIM_STEP_ERR : (int)RCH29_TRIM_STEP_ERR;
#elif defined(RN202x_RN7326_SOC_V2)
    const unsigned int rch_desired_value = RCH32_CLK_DESIRED_VALUE;
    const unsigned int rch_start_thrd = RCH32_TRIM_START_THRD;
    const int rch_step_err = (int)RCH32_TRIM_STEP_ERR;
#else
    const unsigned int rch_desired_value = RCH29_CLK_DESIRED_VALUE;
    const unsigned int rch_start_thrd = RCH29_TRIM_START_THRD;
    const int rch_step_err = (int)RCH29_TRIM_STEP_ERR;
#endif
    unsigned int i;
    eClkTrimRet_TypeDef res;

#if defined(RN_CM0_PLATFORM)
    if (chipid >= CHIP_RA9701A)
#endif
    {
        temp = MEM32(OSC_CTRL1_ADDR);
        if (((((unsigned int)temp >> 11) & 0x1U) == 0x1U) && ((((unsigned int)temp >> 13) & 0x1U) == 0x1U)) {
#if defined(RN_CM0_PLATFORM)
            sysps_bak = MEM32(SYS_PS_ADDR(chipid));
            MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
            sysps_bak = MEM32(SYS_PS_9303_ADDR);
            MEM32(SYS_PS_9303_ADDR) = 0x82U;
#else
            sysps_bak = MEM32(SYS_PS_9103_ADDR);
            MEM32(SYS_PS_9103_ADDR) = 0x82U;
#endif

#if defined(RN_CM0_PLATFORM)
            res = rch_multi_calc(chipid, &temp);
#else
            res = rch_multi_calc(&temp);
#endif
            if (res == CLK_TRIM_OUT_OF_RANGE) {
                /* 1. out of range: restore default value! */
#if defined(RN_CM0_PLATFORM)
                res = set_rch_trim_by_default(chipid);
#else
                res = set_rch_trim_by_default();
#endif
                if ((res == CLK_TRIM_PASS) && (err_thousandth != NULL)) {
#if defined(RN_CM0_PLATFORM)
                    res = rch_multi_calc(chipid, &temp);
#else
                    res = rch_multi_calc(&temp);
#endif
                    if (res == CLK_TRIM_PASS) {
                        *err_thousandth = (int)temp - (int)rch_desired_value;
                        *err_thousandth *= rch_step_err;
                    }
                }
            } else {
                if (res == CLK_TRIM_PASS) {
                    if (i_err_calc(temp, rch_desired_value) > rch_start_thrd) {
                        /* 2. step trim */
#if defined(RN_CM0_PLATFORM)
                        if (0U != get_sysreg_reliable(chipid, ANA_CLK3_ADDR, (unsigned int *)&i, 1U))
#else
                        if (0U != get_sysreg_reliable(ANA_CLK3_ADDR, (unsigned int *)&i, 1U))
#endif
                        {
                            res = CLK_TRIM_TIMEOUT;
                        } else  {
                            if (temp  == rch_desired_value) {
                                /* clk is accurate */
                                if (err_thousandth != NULL) {
                                    *err_thousandth = 0;
                                }
                            } else if (temp > rch_desired_value) {
                                if ((i & 0x3fU) == 0x0U) {
                                    /* down and fast mode */
#if defined(RN_CM0_PLATFORM)
                                    if (set_rch_clk_trim_fast(chipid, 1U) == 1U)
#else
                                    if (set_rch_clk_trim_fast(1U) == 1U)
#endif
                                    {
                                        res = CLK_TRIM_REG_ERROR;
                                    }
                                } else {
                                    /* down mode */
#if defined(RN_CM0_PLATFORM)
                                    if (set_rch_clk_trim_single(chipid, 1U) == 1U)
#else
                                    if (set_rch_clk_trim_single(1U) == 1U)
#endif
                                    {
                                        res = CLK_TRIM_REG_ERROR;
                                    }
                                }
                            } else {
                                if ((i & 0x3fU) == 0x3fU) {
                                    /* up and fast mode */
#if defined(RN_CM0_PLATFORM)
                                    if (set_rch_clk_trim_fast(chipid, 0U) == 1U)
#else
                                    if (set_rch_clk_trim_fast(0U) == 1U)
#endif
                                    {
                                        res = CLK_TRIM_REG_ERROR;
                                    }
                                } else {
                                    /* oup mode */
#if defined(RN_CM0_PLATFORM)
                                    if (set_rch_clk_trim_single(chipid, 0U) == 1U)
#else
                                    if (set_rch_clk_trim_single(0U) == 1U)
#endif
                                    {
                                        res = CLK_TRIM_REG_ERROR;
                                    }
                                }
                            }
                        }
                        if ((res == CLK_TRIM_PASS) && (err_thousandth != NULL)) {
#if defined(RN_CM0_PLATFORM)
                            res = rch_multi_calc(chipid, &temp);
#else
                            res = rch_multi_calc(&temp);
#endif
                            if (res == CLK_TRIM_PASS) {
                                *err_thousandth = (int)temp - (int)rch_desired_value;
                                *err_thousandth *= rch_step_err;
                            }
                        }
                    } else {
                        if (err_thousandth != NULL) {
                            *err_thousandth = (int)temp - (int)rch_desired_value;
                            *err_thousandth *= rch_step_err;
                        }
                    }
                }
                /* else {}*/
                /* 3. timeout or not stable: do nothing */
            }

#if defined(RN_CM0_PLATFORM)
            MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
#elif defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
            MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#else
            MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif
        } else {
            res = CLK_TRIM_NO_CLK;
        }
    }
#if defined(RN_CM0_PLATFORM)
    else {
        res = CLK_TRIM_PASS;
    }
#endif
    return res;
}

eClkTrimRet_TypeDef LL_CLKTRIM_RclTrimByRch(void)
{
    unsigned int sysps_bak;
    unsigned int rch_closed = 0U;
    unsigned int temp;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif
#if defined(RN_CM0_PLATFORM)
    const unsigned int rcl_desired_value = (chipid == CHIP_RA9703A) ? RCL_CLK_DESIRED_VALUE_RCH32M : RCL_CLK_DESIRED_VALUE_RCH29M;
    const unsigned int rcl_start_thrd = (chipid == CHIP_RA9703A) ? RCL_TRIM_START_THRD_RCH32M : RCL_TRIM_START_THRD_RCH29M;
#elif defined(RN202x_RN7326_SOC_V2)
    const unsigned int rcl_desired_value = RCL_CLK_DESIRED_VALUE_RCH32M;
    const unsigned int rcl_start_thrd = RCL_TRIM_START_THRD_RCH32M;
#else
    const unsigned int rcl_desired_value = RCL_CLK_DESIRED_VALUE_RCH29M;
    const unsigned int rcl_start_thrd = RCL_TRIM_START_THRD_RCH29M;
#endif
    unsigned int i;
    eClkTrimRet_TypeDef res;

#if defined(RN_CM0_PLATFORM)
    if (chipid >= CHIP_RA9701A)
#endif
    {
#if defined(RN_CM0_PLATFORM)
        sysps_bak = MEM32(SYS_PS_ADDR(chipid));
        MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
#elif defined(RN202x_RN7326_SOC_V2)
        sysps_bak = MEM32(SYS_PS_9303_ADDR);
        MEM32(SYS_PS_9303_ADDR) = 0x82U;
#else
        sysps_bak = MEM32(SYS_PS_9103_ADDR);
        MEM32(SYS_PS_9103_ADDR) = 0x82U;
#endif
        temp = MEM32(OSC_CTRL1_ADDR);
        if ((((unsigned int)temp >> 1) & 0x1U) == 0x1U) {
            rch_closed = 1U;
            MEM32(OSC_CTRL1_ADDR) = temp & ((~((unsigned int)1U << 1)));
            for (i = 0U; i < 20U; i++) {
                MEM32(OSC_CTRL1_ADDR) = MEM32(OSC_CTRL1_ADDR);
            }
            temp = MEM32(OSC_CTRL1_ADDR);
        }
        if (((((unsigned int)temp >> 13) & 0x1U) == 0x1U) && ((((unsigned int)temp >> 14) & 0x1U) == 0x1U)) {
#if defined(RN_CM0_PLATFORM)
            res = rcl_multi_calc(chipid, &temp);
#else
            res = rcl_multi_calc(&temp);
#endif
            if (res == CLK_TRIM_OUT_OF_RANGE) {
                /* 1. out of range: restore default value! */
#if defined(RN_CM0_PLATFORM)
                if (0U == set_rcl_trim_by_default(chipid))
#else
                if (0U == set_rcl_trim_by_default())
#endif
                {
                    res = CLK_TRIM_PASS;
                } else {
                    res = CLK_TRIM_TIMEOUT;
                }
            } else {
                if (res == CLK_TRIM_PASS) {
                    if (i_err_calc(temp, rcl_desired_value) > rcl_start_thrd) {
                        /* 2. step trim */
#if defined(RN_CM0_PLATFORM)
                        if (0U != get_sysreg_reliable(chipid, ANA_CLK2_ADDR, (unsigned int *)&i, 1U))
#else
                        if (0U != get_sysreg_reliable(ANA_CLK2_ADDR, (unsigned int *)&i, 1U))
#endif
                        {
                            res = CLK_TRIM_REG_ERROR;
                        } else {
                            if (temp  == rcl_desired_value) {
                                /* clk is accurate */
                            } else if (temp > rcl_desired_value) {
                                if ((i & 0xffU) == 0x0U) {
                                    /* can't trim, restore default! */
#if defined(RN_CM0_PLATFORM)
                                    if (0U != set_rcl_trim_by_default(chipid))
#else
                                    if (0U !=set_rcl_trim_by_default())
#endif
                                    {
                                        res = CLK_TRIM_TIMEOUT;
                                    }
                                } else {
                                    /* down mode */
#if defined(RN_CM0_PLATFORM)
                                    if (set_rcl_clk_trim_single(chipid, 1U) == 1U)
#else
                                    if (set_rcl_clk_trim_single(1U) == 1U)
#endif
                                    {
                                        res = CLK_TRIM_REG_ERROR;
                                    }
                                }
                            } else {
                                if ((i & 0xffU) == 0xffU) {
                                    /* can't trim, restore default! */
#if defined(RN_CM0_PLATFORM)
                                    if (0U != set_rcl_trim_by_default(chipid))
#else
                                    if (0U != set_rcl_trim_by_default())
#endif
                                    {
                                        res = CLK_TRIM_TIMEOUT;
                                    }
                                } else {
                                    /* oup mode */
#if defined(RN_CM0_PLATFORM)
                                    if (set_rcl_clk_trim_single(chipid, 0U) == 1U)
#else
                                    if (set_rcl_clk_trim_single(0U) == 1U)
#endif
                                    {
                                        res = CLK_TRIM_REG_ERROR;
                                    }
                                }
                            }
                        }
                    } else {
                        /* skip trim */
                    }
                }
                /* else {}*/
                /* 3. timeout or not stable: do nothing */
            }
        } else {
            res = CLK_TRIM_NO_CLK;
        }
        if (rch_closed == 1U) {
            temp = MEM32(OSC_CTRL1_ADDR);
            MEM32(OSC_CTRL1_ADDR) = temp | ((unsigned int)1U << 1);
        }

#if defined(RN_CM0_PLATFORM)
        MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
#elif defined(RN202x_RN7326_SOC_V2)
        MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#else
        MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif
    }
#if defined(RN_CM0_PLATFORM)
    else {
        res = CLK_TRIM_PASS;
    }
#endif
    return res;
}

eClkTrimRet_TypeDef LL_CLKTRIM_SysClkMonitorAsync(void)
{
    volatile unsigned int temp;
    unsigned int sysps_bak;
    unsigned int rch_closed = 0U;
    unsigned int pll_freq, pll_scaler;
    eClkTrimRet_TypeDef res = CLK_TRIM_NO_CLK;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif

#if defined(RN_CM0_PLATFORM)
    sysps_bak = MEM32(SYS_PS_ADDR(chipid));
    MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
#elif defined(RN202x_RN7326_SOC_V2)
    sysps_bak = MEM32(SYS_PS_9303_ADDR);
    MEM32(SYS_PS_9303_ADDR) = 0x82U;
#else
    sysps_bak = MEM32(SYS_PS_9103_ADDR);
    MEM32(SYS_PS_9103_ADDR) = 0x82U;
#endif
    temp = MEM32(OSC_CTRL1_ADDR);
    if ((((unsigned int)temp >> 1) & 0x1U) == 0x1U) {
        rch_closed = 1U;
        MEM32(OSC_CTRL1_ADDR) = temp & ((~((unsigned int)1U << 1)));
        for (temp = 0U; temp < 80U; temp++) {
            ;
        }
        temp = MEM32(OSC_CTRL1_ADDR);
    }

    if (((((unsigned int)temp >> RCH_FLAG_BIT) & 0x1U) == 0x1U) && ((((unsigned int)temp >> PLL_FLAG_BIT) & 0x1U) == 0x1U)) {
#if defined(RN_CM0_PLATFORM)
        pll_scaler = ((MEM32(OSC_CTRL2_ADDR(chipid)) >> 0x5) & 0x7U);
        pll_scaler = (pll_scaler > 2) ? 2 : pll_scaler;
        if (chipid == CHIP_RA9703A) {
            pll_freq = 8192000U << pll_scaler;
        } else {
            pll_freq = 7372800U << pll_scaler;
        }
        /* 32ms=58982ticks@1.84M, 58982/4=14745(4ticks per loop) */
        (void)clk_trim_set(chipid, REFCLK_PLL, (pll_freq >> 5) - 1U, CALCLK_RCH, 14745U << currentClk_idx(), 1U);
#elif  defined(RN202x_RN7326_SOC_V2)
        pll_scaler = ((MEM32(OSC_CTRL2_9303_ADDR) >> 0x5) & 0x7U);
        pll_scaler = (pll_scaler > 2) ? 2 : pll_scaler;
        pll_freq = 8192000U << pll_scaler;
        (void)clk_trim_set(REFCLK_PLL, (pll_freq >> 5) - 1U, CALCLK_RCH, 14745U << currentClk_idx(), 1U);
#elif  defined(RN831x_RN861x_MCU_V2) ||\
       defined(RN831x_RN861x_MCU_V3) ||\
       defined(RN821x_RN721x_SOC_V2) ||\
       defined(RN821x_RN721x_SOC_V3)
        pll_scaler = ((MEM32(OSC_CTRL2_9103_ADDR) >> 0x5) & 0x7U);
        pll_scaler = (pll_scaler > 2) ? 2 : pll_scaler;
        pll_freq = 7372800U << pll_scaler;
        (void)clk_trim_set(REFCLK_PLL, (pll_freq >> 5) - 1U, CALCLK_RCH, 14745U << currentClk_idx(), 1U);
#endif
        res = CLK_TRIM_PASS;
    }

    if (rch_closed == 1U) {
        temp = MEM32(OSC_CTRL1_ADDR);
        MEM32(OSC_CTRL1_ADDR) = temp | ((unsigned int)1U << 1);
    }

#if defined(RN_CM0_PLATFORM)
    MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
#elif defined(RN202x_RN7326_SOC_V2)
    MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#else
    MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif
    return res;
}

eClkTrimRet_TypeDef LL_CLKTRIM_SysClkMonitorAsyncStatus(int *clk_jitter_ppm)
{
    unsigned int trim_start_temp, trim_cfg_temp;
    eClkTrimRet_TypeDef res = CLK_TRIM_INPROCESS;
    unsigned int sysps_bak;
    unsigned int pll_freq, pll_scaler;
    unsigned int rch_cnt;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif

#if defined(RN_CM0_PLATFORM)
    sysps_bak = MEM32(SYS_PS_ADDR(chipid));
    MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
    trim_start_temp = MEM32(TRIM_START_ADDR(chipid));
    trim_cfg_temp = MEM32(TRIM_CFG1_ADDR(chipid));
#elif defined(RN202x_RN7326_SOC_V2)
    sysps_bak = MEM32(SYS_PS_9303_ADDR);
    MEM32(SYS_PS_9303_ADDR) = 0x82U;
    trim_start_temp = MEM32(TRIM_START_9303_ADDR);
    trim_cfg_temp = MEM32(TRIM_CFG1_9303_ADDR);
#else
    sysps_bak = MEM32(SYS_PS_9103_ADDR);
    MEM32(SYS_PS_9103_ADDR) = 0x82U;
    trim_start_temp = MEM32(TRIM_START_9103_ADDR);
    trim_cfg_temp = MEM32(TRIM_CFG1_9103_ADDR);
#endif

    if (((trim_start_temp >> CLK_TRIM_DONE_BIT) & 0x3) != 0x0) {
        if (((trim_start_temp >> CLK_TRIM_DONE_BIT) & 0x1U) == 0x1U) {
            if (((trim_cfg_temp >> 24) & 0x7) == 0x3U) {
#if defined(RN_CM0_PLATFORM)
                pll_scaler = ((MEM32(OSC_CTRL2_ADDR(chipid)) >> 0x5) & 0x7U);
                pll_scaler = (pll_scaler > 2) ? 2 : pll_scaler;
                if (chipid == CHIP_RA9703A) {
                    pll_freq = 8192000U << pll_scaler;
                } else {
                    pll_freq = 7372800U << pll_scaler;
                }
                rch_cnt = (CHIP_RA9703A == chipid) ? 32768000U : 29491200U;
#elif  defined(RN202x_RN7326_SOC_V2)
                pll_scaler = ((MEM32(OSC_CTRL2_9303_ADDR) >> 0x5) & 0x7U);
                pll_scaler = (pll_scaler > 2) ? 2 : pll_scaler;
                pll_freq = 8192000U << pll_scaler;
                rch_cnt = 32768000U;
#else
                pll_scaler = ((MEM32(OSC_CTRL2_9103_ADDR) >> 0x5) & 0x7U);
                pll_scaler = (pll_scaler > 2) ? 2 : pll_scaler;
                pll_freq = 7372800U << pll_scaler;
                rch_cnt = 29491200U;
#endif
                if ((trim_cfg_temp & 0xfffffU) == ((pll_freq >> 5) - 1U)) {
                    *clk_jitter_ppm = (int)((trim_start_temp & 0xfffffU) - (rch_cnt >> 5));
                    res = CLK_TRIM_PASS;
                } else {
                    /* maybe clk_trim module is aborted by others */
                    res = CLK_TRIM_TIMEOUT;
                }
            }
        } else {
            res = CLK_TRIM_TIMEOUT;
        }
#if defined(RN_CM0_PLATFORM)
        MEM32(TRIM_START_ADDR(chipid)) = trim_start_temp;
#elif defined(RN202x_RN7326_SOC_V2)
        MEM32(TRIM_START_9303_ADDR) = trim_start_temp;
#else
        MEM32(TRIM_START_9103_ADDR) = trim_start_temp;
#endif
    } else {
        res = CLK_TRIM_INPROCESS;
    }

#if defined(RN_CM0_PLATFORM)
    MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
#elif defined(RN202x_RN7326_SOC_V2)
    MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#else
    MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif

    return res;
}

/* 1ms=1843ticks@1.84M, 1843/4=461(4ticks per loop) */
#define MONITOR_SYNC_BASE_TIMEOUT (461U)
eClkTrimRet_TypeDef LL_CLKTRIM_SysClkMonitorSync(int *clk_jitter_ppm)
{
    volatile unsigned int temp;
    unsigned int sysps_bak;
    unsigned int rch_closed = 0U;
    unsigned int pll_freq, pll_scaler;
    unsigned int rch_cnt;
    eClkTrimRet_TypeDef res = CLK_TRIM_NO_CLK;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif

#if defined(RN_CM0_PLATFORM)
    sysps_bak = MEM32(SYS_PS_ADDR(chipid));
    MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
#elif defined(RN202x_RN7326_SOC_V2)
    sysps_bak = MEM32(SYS_PS_9303_ADDR);
    MEM32(SYS_PS_9303_ADDR) = 0x82U;
#else
    sysps_bak = MEM32(SYS_PS_9103_ADDR);
    MEM32(SYS_PS_9103_ADDR) = 0x82U;
#endif
    temp = MEM32(OSC_CTRL1_ADDR);
    if ((((unsigned int)temp >> 1) & 0x1U) == 0x1U) {
        rch_closed = 1U;
        MEM32(OSC_CTRL1_ADDR) = temp & ((~((unsigned int)1U << 1)));
        for (temp = 0U; temp < 80U; temp++) {
            ;
        }
        temp = MEM32(OSC_CTRL1_ADDR);
    }

    if (((((unsigned int)temp >> RCH_FLAG_BIT) & 0x1U) == 0x1U) && ((((unsigned int)temp >> PLL_FLAG_BIT) & 0x1U) == 0x1U)) {
#if defined(RN_CM0_PLATFORM)
        pll_scaler = ((MEM32(OSC_CTRL2_ADDR(chipid)) >> 0x5) & 0x7U);
        pll_scaler = (pll_scaler > 2) ? 2 : pll_scaler;
        if (chipid == CHIP_RA9703A) {
            pll_freq = 8192000U << pll_scaler;
        } else {
            pll_freq = 7372800U << pll_scaler;
        }
        rch_cnt = (CHIP_RA9703A == chipid) ? 32768000U : 29491200U;
        temp = clk_trim_set(chipid, REFCLK_PLL, pll_freq >> 10, CALCLK_RCH, MONITOR_SYNC_BASE_TIMEOUT << currentClk_idx(), 0U);
#elif defined(RN202x_RN7326_SOC_V2)
        pll_scaler = ((MEM32(OSC_CTRL2_9303_ADDR) >> 0x5) & 0x7U);
        pll_scaler = (pll_scaler > 2) ? 2 : pll_scaler;
        pll_freq = 8192000U << pll_scaler;
        rch_cnt = 32768000U;
        temp = clk_trim_set(REFCLK_PLL, pll_freq >> 10, CALCLK_RCH, MONITOR_SYNC_BASE_TIMEOUT << currentClk_idx(), 0U);
#else
        pll_scaler = ((MEM32(OSC_CTRL2_9103_ADDR) >> 0x5) & 0x7U);
        pll_scaler = (pll_scaler > 2) ? 2 : pll_scaler;
        pll_freq = 7372800U << pll_scaler;
        rch_cnt = 29491200U;
        temp = clk_trim_set(REFCLK_PLL, pll_freq >> 10, CALCLK_RCH, MONITOR_SYNC_BASE_TIMEOUT << currentClk_idx(), 0U);
#endif
        if (temp == 0) {
            res = CLK_TRIM_TIMEOUT;
        } else {
            res = CLK_TRIM_PASS;
            *clk_jitter_ppm = (int)(temp - (rch_cnt >> 10));
            *clk_jitter_ppm *= 31;
        }
    } else {
        res = CLK_TRIM_NO_CLK;
    }

    if (rch_closed == 1U) {
        temp = MEM32(OSC_CTRL1_ADDR);
        MEM32(OSC_CTRL1_ADDR) = temp | ((unsigned int)1U << 1);
    }

#if defined(RN_CM0_PLATFORM)
    MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
#elif defined(RN202x_RN7326_SOC_V2)
    MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#else
    MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif

    return res;
}

#define  LOSC_FLAG_REG_ADDR_9701    (0x4003c06cU)
#define  LOSC_FLAG_REG_ADDR_9703    (0x4002c078U)
#define  ANA_PASSWD_REG_ADDR_9703   (0x4002c044U)
#define  LOSC_1MS_REF_CNT           (9U)
#define  LOSC_1MS_RCH_CNT_9701      ((29491200U * (LOSC_1MS_REF_CNT + 1U) / 32768U) - 1U)
#define  LOSC_1MS_RCH_CNT_9703      ((32768000U * (LOSC_1MS_REF_CNT + 1U) / 32768U) - 1U)
#define  LOSC_1MS_RCH_ERR_9701      ((unsigned int)(0.05 * LOSC_1MS_RCH_CNT_9701))
#define  LOSC_1MS_RCH_ERR_9703      ((unsigned int)(0.05 * LOSC_1MS_RCH_CNT_9703))
eClkTrimRet_TypeDef LL_CLKTRIM_LoscStopCheck(void)
{
    volatile unsigned int temp;
    unsigned int sysps_bak, mod1_bak;
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2)
    unsigned int anaps_bak;
#endif
    unsigned int rch_closed = 0U;
    unsigned int rch_cnt, rch_stop_cnt;
    eClkTrimRet_TypeDef res = CLK_TRIM_NO_CLK;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif

#if defined(RN_CM0_PLATFORM)
    sysps_bak = MEM32(SYS_PS_ADDR(chipid));
    MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
    mod1_bak = MEM32(MOD1_EN_ADDR(chipid));
    MEM32(MOD1_EN_ADDR(chipid)) |= ((unsigned int)1U << 10);
#elif defined(RN202x_RN7326_SOC_V2)
    sysps_bak = MEM32(SYS_PS_9303_ADDR);
    MEM32(SYS_PS_9303_ADDR) = 0x82U;
    mod1_bak = MEM32(MOD1_EN_9303_ADDR);
    MEM32(MOD1_EN_9303_ADDR) |= ((unsigned int)1U << 10);
#else
    sysps_bak = MEM32(SYS_PS_9103_ADDR);
    MEM32(SYS_PS_9103_ADDR) = 0x82U;
    mod1_bak = MEM32(MOD1_EN_9103_ADDR);
    MEM32(MOD1_EN_9103_ADDR) |= ((unsigned int)1U << 10);
#endif

#if defined(RN_CM0_PLATFORM)
    if ((chipid == CHIP_RA9701A) || (chipid == CHIP_RA9701B)) {
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3)
        temp = (MEM32(LOSC_FLAG_REG_ADDR_9701) >> 9) & 0x1U;
        MEM32(LOSC_FLAG_REG_ADDR_9701) = (unsigned int)1U << 9;
#endif
#if defined(RN_CM0_PLATFORM)
    } else if (chipid == CHIP_RA9703A) {
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2)
        anaps_bak = MEM32(ANA_PASSWD_REG_ADDR_9703);
        temp = (MEM32(LOSC_FLAG_REG_ADDR_9703) >> 3) & 0x1U;
        MEM32(ANA_PASSWD_REG_ADDR_9703) = 0x8e;
        MEM32(LOSC_FLAG_REG_ADDR_9703) = (unsigned int)1U << 3;
        MEM32(ANA_PASSWD_REG_ADDR_9703) = anaps_bak;
#endif
#if defined(RN_CM0_PLATFORM)
    } else {
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3)
        temp = 0x1U;
#endif
#if defined(RN_CM0_PLATFORM)
    }
#endif

    if (temp == 0x1U) {
        /* need check losc_flag */
        temp = MEM32(OSC_CTRL1_ADDR);
        if ((((unsigned int)temp >> LOSC_FLAG_BIT) & 0x1U) == 0x1U) {
            if((((unsigned int)temp >> RCH_PWRON_BIT) & 0x1U) == 0x1U) {
                /* open rch */
                rch_closed = 1U;
                MEM32(OSC_CTRL1_ADDR) = temp & ((~((unsigned int)1U << 1)));
                for (temp = 0U; temp < 80U; temp++) {
                    ;
                }
            }
            temp = MEM32(OSC_CTRL1_ADDR);
            if ((((unsigned int)temp >> RCH_FLAG_BIT) & 0x1U) == 0x1U) {
#if defined(RN_CM0_PLATFORM)
                rch_cnt = (chipid == CHIP_RA9703A) ? LOSC_1MS_RCH_CNT_9703 : LOSC_1MS_RCH_CNT_9701;
                rch_stop_cnt = (chipid == CHIP_RA9703A) ? LOSC_1MS_RCH_ERR_9703 : LOSC_1MS_RCH_ERR_9701; /* 5% */
#elif defined(RN202x_RN7326_SOC_V2)
                rch_cnt = LOSC_1MS_RCH_CNT_9703;
                rch_stop_cnt = LOSC_1MS_RCH_ERR_9703;
#else
                rch_cnt = LOSC_1MS_RCH_CNT_9701;
                rch_stop_cnt = LOSC_1MS_RCH_ERR_9701;
#endif
                /* 1ms=2048ticks@2.048M, 2048/4=512(4ticks per loop) */
#if defined(RN_CM0_PLATFORM)
                temp = clk_trim_set(chipid, REFCLK_LOSC, LOSC_1MS_REF_CNT, CALCLK_RCH, 512U << currentClk_idx(), 0U);
#else
                temp = clk_trim_set(REFCLK_LOSC, LOSC_1MS_REF_CNT, CALCLK_RCH, 512U << currentClk_idx(), 0U);
#endif
                if (i_err_calc(temp, rch_cnt) < rch_stop_cnt) {
                    res = CLK_TRIM_PASS;
                } else {
                    res = CLK_TRIM_UNSTABLE;
                }
            }

            if (rch_closed == 1U) {
                MEM32(OSC_CTRL1_ADDR) = MEM32(OSC_CTRL1_ADDR) | (((unsigned int)1U << RCH_PWRON_BIT));
            }
        }
    } else {
        res = CLK_TRIM_PASS;
    }

#if defined(RN_CM0_PLATFORM)
    MEM32(MOD1_EN_ADDR(chipid)) = mod1_bak;
    MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
#elif defined(RN202x_RN7326_SOC_V2)
    MEM32(MOD1_EN_9303_ADDR) = mod1_bak;
    MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#else
    MEM32(MOD1_EN_9103_ADDR) = mod1_bak;
    MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif

    return res;
}

#endif
/* r2989 */
