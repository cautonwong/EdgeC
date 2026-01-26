/**
 * @file utils.h
 * @author Renergy Technology Inc.
 * @brief common functions used by lib code
 *
 * @copyright Copyright (C) 2008, Renergy Technology Inc. All rights reserved.
 *
 */
#ifndef RN8XXX_LL_UTILS_H_
#define RN8XXX_LL_UTILS_H_

#include "rn8xxx_ll_devices.h"

#if !defined(RN831x_RN861x_MCU_V4)
#define ROM_BASE_ADDRESS      (0x18000000U)
#define FLASH_BASE_ADDRESS    (0x00000000U)
#define EEPROM_BASE_ADDRESS   (0x08000000U)

/* Register define */
#define SYSTICK_CTRL_ADDR     (0xe000e010U)
#define SYSTICK_LOAD_ADDR     (0xe000e014U)
#define SYSTICK_VAL_ADDR      (0xe000e018U)

/* compatible reg address */
#define OSC_CTRL1_ADDR        (0x40034000U)
#define SYS_MODE_ADDR         (0x40034004U)
#define ANA_ADC_ADDR          (0x40034048U)
#define ANA_CLK1_ADDR         (0x4003404cU)
#define ANA_CLK2_ADDR         (0x40034050U)
#define ANA_BGR_ADDR          (0x40034058U)
#define ANA_PSW_ADDR          (0x4003405cU)
#define ANA_PWR_ADDR          (0x40034068U)
#define ANA_CLK3_ADDR         (0x4003406cU)
#define ANA_SYS_ADDR          (0x40034070U)
#define ANA_ULBGR1_ADDR       (0x40034098U)
#define ANA_ULBGR2_ADDR       (0x4003409cU)
#define ANA_PWS_ADDR          (0x400340acU)
#define ANA_PSM_ADDR          (0x400340b0U)
#define ANA_ADC2_ADDR         (0x400340b4U)
#define ANA_PWR2_ADDR         (0x400340bcU)
#define ANA_PSM2_ADDR         (0x400340c0U)
#define ANA_CKTIM2_ADDR       (0x400340c8U)

#define DEBUG_CTRL_ADDR       (0x40034044U)
#define RAMB_CTRL_ADDR        (0x4003408cU)
#define FL_TRIM_ADDR          (0x40034094U)
#define ANA_TIEHL_ADDR        (0x4002c074U)

#define RTC_CTRL_ADDR         (0x4003c000U)
#define RTC_TEMP_ADDR         (0x4003c03cU)
#define RTC_PS_ADDR           (0x4003c040U)
#define RTC_DOTA_ADDR         (0x4003c048U)
#define RTC_MODE_ADDR         (0x4003c044U)
#define RTC_ALPHAL_ADDR       (0x4003c04cU)
#define RTC_ALPHAH_ADDR       (0x4003c050U)
#define RTC_XT0_ADDR          (0x4003c054U)
#define RTC_BETA_ADDR         (0x4003c05cU)
#define RTC_LOSC_CFG1_ADDR    (0x4003c06cU)
#define RTC_IOEN_ADDR         (0x4003c088U)
#define RTC_IOMODE_ADDR       (0x4003c08cU)
#define RTC_CALPS_ADDR        (0x4003c0ccU)
#define RTC_VBATCLKSEL_ADDR   (0x4003c0fcU)
#define RTC_MODE1_ADDR        (0x4003c130U)
#define LOSC_CFG2_ADDR        (0x4003c070U)

typedef struct {
    volatile unsigned int dota[10];
} dota_table_t;
#define RTC_DOTATBL           ((dota_table_t *)0x4003c0d0U)
#define RTC_DOTATBL0_ADDR     (0x4003c0d0U)
#define RTC_DOTATBL1_ADDR     (0x4003c0d4U)
#define RTC_DOTATBL2_ADDR     (0x4003c0d8U)
#define RTC_DOTATBL3_ADDR     (0x4003c0dcU)
#define RTC_DOTATBL4_ADDR     (0x4003c0e0U)
#define RTC_DOTATBL5_ADDR     (0x4003c0e4U)
#define RTC_DOTATBL6_ADDR     (0x4003c0e8U)
#define RTC_DOTATBL7_ADDR     (0x4003c0ecU)
#define RTC_DOTATBL8_ADDR     (0x4003c0f0U)
#define RTC_DOTATBL9_ADDR     (0x4003c0f4U)

#define MADC_SAR_CTL_ADDR     (0x4002c000U)
#define MADC_SAR_START_ADDR   (0x4002c004U)
#define MADC_SAR_STATUS_ADDR  (0x4002c008U)
#define MADC_SAR_DAT_ADDR     (0x4002c00cU)
#define MADC_SAR_DAT2_ADDR    (0x4002c01cU)
#define MADC_ANA_PAD_ADDR     (0x4002c03cU)

#define WDT_PS_ADDR           (0x40030008U)
#define WDT_EN_ADDR           (0x40030000U)
#define WDT_CTRL_ADDR         (0x40030004U)
#define WDT_HALT_ADDR         (0x40030014U)
#define WDT_STBY_ADDR         (0x40030018U)

#define SYSTC_CTRL_ADDR       (0x50010000U)
#define SYSTC_LOAD_ADDR       (0x50010004U)
#define SYSTC_VAL_ADDR        (0x50010008U)

#define GPIO_PCA1_ADDR        (0x5000000cU)
#define GPIO_PCA0_ADDR        (0x50000004U)
#define GPIO_PSW0_ADDR        (0x50000050U)

/* RA9701A(P141/P142):PUD must reverse
   RA9701B(P140/P141):PUD must reverse
   RA9702A(P140/P142):PUD must reverse
   RA9702B(P141/P147):PUD must reverse
*/
#define GPIO_PD_ADDR          (0x50000054U)
#define GPIO_BB_PD_ADDR       (0x52000000U + ((unsigned int)0x54U << 5))
#define GPIO_PCD_ADDR         (0x50000058U)
#define GPIO_BB_PCD_ADDR      (0x52000000U + ((unsigned int)0x58U << 5))
#define GPIO_PUD_ADDR         (0x5000005cU)
#define GPIO_BB_PUD_ADDR      (0x52000000U + ((unsigned int)0x5cU << 5))
#define GPIO_PCD2_ADDR        (0x500000a8U)
#define GPIO_BB_PCD2_ADDR     (0x52000000U + ((unsigned int)0xa8U << 5))
#define GPIO_PIED_ADDR        (0x50000084U)
#define GPIO_BB_PIED_ADDR     (0x52000000U + ((unsigned int)0x84U << 5))
#define GPIO_PIMD_ADDR        (0x50000088U)
#define GPIO_BB_PIMD_ADDR     (0x52000000U + ((unsigned int)0x88U << 5))
#define GPIO_PMD_ADDR         (0x50000050U)
#define GPIO_BB_PMD_ADDR      (0x52000000U + ((unsigned int)0x50U << 5))


/* RA9703A(P74/P75):PUD must reverse */
#define GPIO_PMB1_ADDR        (0x50000030U)
#define GPIO_BB_PMB1_ADDR     (0x52000000U + ((unsigned int)0x30U << 5))
#define GPIO_PUB1_ADDR        (0x50000038U)
#define GPIO_BB_PUB1_ADDR     (0x52000000U + ((unsigned int)0x38U << 5))
#define GPIO_PB1_ADDR         (0x50000044U)
#define GPIO_BB_PB1_ADDR      (0x52000000U + ((unsigned int)0x44U << 5))
#define GPIO_PCB1_ADDR        (0x50000058U)
#define GPIO_BB_PCB1_ADDR     (0x52000000U + ((unsigned int)0x58U << 5))
#define GPIO_PIEB1_ADDR       (0x50000040U)
#define GPIO_BB_PIEB1_ADDR    (0x52000000U + ((unsigned int)0x40U << 5))
#define GPIO_PIMB2_ADDR       (0x5000002cU)
#define GPIO_BB_PIMB2_ADDR    (0x52000000U + ((unsigned int)0x2cU << 5))

/* V1 reg address */
#define ANA_LDO_9103_ADDR       (0x40034054U)
#define OSC_CTRL2_9103_ADDR     (0x40034010U)
#define SYS_PS_9103_ADDR        (0x40034030U)
#define MAP_CTL_9103_ADDR       (0x40034018U)
#define MOD0_EN_9103_ADDR       (0x4003401cU)
#define MOD1_EN_9103_ADDR       (0x40034020U)
#define TRIM_CFG1_9103_ADDR     (0x40034078U)
#define TRIM_START_9103_ADDR    (0x4003407cU)
#define TEST_CTL_9103_ADDR      (0x40034040U)
#define ANA_SADDR_9701_ADDR     (0x400340a0U)
#define ANA_SDATA_9103_ADDR     (0x400340a4U)
#define ANA_CKTIM_9702_ADDR     (0x400340c4U)
#define ANA_LDOMOD_9103_ADDR    (0x40034080U)
#define ANA_DIEVER_9103_ADDR    (0x40034074U)
#define ANA_BGR2_9702_ADDR    (0x400340b8U)

#define MADC_TPSCTRL_9103_ADDR  (0x4002c018U)
#define MADC_ADTEST_9103_ADDR   (0x4002c01cU)
#define MADC_ANABGR_9103_ADDR   (0x4002c020U)
#define MADC_ANALDO_9103_ADDR   (0x4002c028U)
#define MADC_ANARCH_9103_ADDR   (0x4002c02cU)
#define MADC_SARCTRL2_9103_ADDR (0x4002c030U)
#define MADC_TPSCTRL2_9702_ADDR (0x4002c054U)
#define MADC_TPSCTRL3_9702_ADDR (0x4002c058U)
#define MADC_ANA_RST_9103_ADDR  (0x4002c040U)
#define MADC_ANA_CTL_9103_ADDR  (0x4002c024U)
#define MADC_ANA_TEST_9103_ADDR (0x4002c034U)
#define MADC_ANA_CTL2_9303_ADDR (0x4002c038U)
#define MADC_ANA_PS_9303_ADDR   (0x4002c020U)

#define RTC_TADJ_9103_ADDR      (0x4003c058U)
#define RTC_ZT_9103_ADDR        (0x4003c060U)

#define TRNG_CTRL_9701_ADDR     (0x5000c3c0U)
/* DIG5 reg address */
typedef enum {
    DIG5_ANA_LDO           =  (0x15U),
    DIG5_ANA_ULBGR1        =  (0x26U),
    DIG5_ANA_ULBGR2        =  (0x27U),
    DIG5_LURLS_CFG         =  (0x28U),
    DIG5_LURLS_EN          =  (0x29U),
    DIG5_WKEY              =  (0x38U)
} DIG5_REG_ADDR_TYPEDEF;

/* V2 reg address */
#define ANA_LDO_9303_ADDR       (0x40034064U)
#define OSC_CTRL2_9303_ADDR     (0x4003401CU)
#define SYS_PS_9303_ADDR        (0x4003403cU)
#define MAP_CTL_9303_ADDR       (0x40034024U)
#define MOD0_EN_9303_ADDR       (0x40034028U)
#define MOD1_EN_9303_ADDR       (0x4003402cU)
#define TRIM_CFG1_9303_ADDR     (0x400340a0U)
#define TRIM_START_9303_ADDR    (0x400340a4U)
#define TEST_CTL_9703_ADDR      (0x40034120U)
#define ANA_SADDR_9703_ADDR     (0x40034128U)
#define ANA_SDATA_9703_ADDR     (0x4003412cU)
#define ANA_CKTIM_9703_ADDR     (0x40034130U)
#define ANA_LDOMOD_9701_ADDR    (0x400340a8U)
#define ANA_DIEVER_9303_ADDR    (0x4003409cU)
#define ANA_BGR2_9303_ADDR      (0x40034074U)

#define MADC_TPSCTRL_9701_ADDR  (0x4002c050U)
#define MADC_ADTEST_9701_ADDR   (0x4002c04cU)
#define MADC_ANABGR_9701_ADDR   (0x4002c054U)
#define MADC_ANALDO_9701_ADDR   (0x4002c060U)
#define MADC_ANARCH_9701_ADDR   (0x4002c064U)
#define MADC_SARCTRL2_9701_ADDR (0x4002c048U)
#define MADC_ANA_RST_9303_ADDR  (0x4002c054U)
#define MADC_ANA_CTL_9701_ADDR  (0x4002c058U)
#define MADC_ANA_TEST_9303_ADDR (0x4002c044U)
#define MADC_ANA_CTL2_9701_ADDR (0x4002c05cU)
#define MADC_ANA_PS_9701_ADDR   (0x4002c044U)

#define RTC_TADJ_9702_ADDR      (0x4003c180U)
#define RTC_ZT_9702_ADDR        (0x4003c184U)

#define TRNG_CTRL_9703_ADDR     (0x500083c0U)
/* V3 reg address */
#define ANA_LDOMOD_9703_ADDR    (0x400340d4U)
#define MADC_TPSCTRL_9303_ADDR  (0x4002c02cU)
#define MADC_ADTEST_9303_ADDR   (0x4002c028U)
#define MADC_ANABGR_9303_ADDR   (0x4002c030U)
#define MADC_ANALDO_9303_ADDR   (0x4002c03cU)
#define MADC_ANARCH_9303_ADDR   (0x4002c040U)
#define MADC_SARCTRL2_9303_ADDR (0x4002c024U)
#define MADC_ANA_CTL_9303_ADDR  (0x4002c034U)

#define TRNG_CTRL_9702B_ADDR    (0x5000c054U)
/* V4 reg address */
#define MADC_ANA_TEST_9701_ADDR (0x4002c068U)
#define ANA_LDOMOD_9303_ADDR    (0x40034064U)


#define ANA_ADC_9303_ADDR       (0x40034058U)
#define ANA_CLK1_9303_ADDR      (0x4003405cU)
#define ANA_LDO33_9303_ADDR     (0x4003406cU)
#define ANA_BGR_9303_ADDR       (0x40034070U)
#define ANA_PSW_9303_ADDR       (0x40034078U)
#define ANA_PWR_9303_ADDR       (0x40034084U)
#define ANA_PWR2_9303_ADDR      (0x40034088U)
#define ANA_TCLDO15_9303_ADDR   (0x4003408cU)
#define ANA_TCICC04_9303_ADDR   (0x40034090U)
#define ANA_CKTIM_9303_ADDR     (0x40034094U)
#define ANA_SYS_9303_ADDR       (0x40034098U)
#define ANA_RC32M_CTL2_9303_ADDR (0x400340acU)

#define VDET_CTRL_9303_ADDR      (0x40034014U)
#define UEDTVB_CTRL_9303_ADDR    (0x400340B0U)

/*********************************************************/
#define MEM32(addr)           (*(volatile unsigned int *)(addr))
#define MEM16(addr)           (*(volatile unsigned short *)(addr))

#if defined(RN_CM0_PLATFORM)
#define SYS_PS_ADDR(ver)         ((((ver) == CHIP_RA9703A) || ((ver) == CHIP_RA9303B)) ? SYS_PS_9303_ADDR    : SYS_PS_9103_ADDR)
#define OSC_CTRL2_ADDR(ver)      ((((ver) == CHIP_RA9703A) || ((ver) == CHIP_RA9303B)) ? OSC_CTRL2_9303_ADDR : OSC_CTRL2_9103_ADDR)
#define MAP_CTL_ADDR(ver)        ((((ver) == CHIP_RA9703A) || ((ver) == CHIP_RA9303B)) ? MAP_CTL_9303_ADDR   : MAP_CTL_9103_ADDR)
#define MOD0_EN_ADDR(ver)        ((((ver) == CHIP_RA9703A) || ((ver) == CHIP_RA9303B)) ? MOD0_EN_9303_ADDR   : MOD0_EN_9103_ADDR)
#define MOD1_EN_ADDR(ver)        ((((ver) == CHIP_RA9703A) || ((ver) == CHIP_RA9303B)) ? MOD1_EN_9303_ADDR   : MOD1_EN_9103_ADDR)
#define TRIM_CFG1_ADDR(ver)      ((((ver) == CHIP_RA9703A) || ((ver) == CHIP_RA9303B)) ? TRIM_CFG1_9303_ADDR : TRIM_CFG1_9103_ADDR)
#define TRIM_START_ADDR(ver)     ((((ver) == CHIP_RA9703A) || ((ver) == CHIP_RA9303B)) ? TRIM_START_9303_ADDR: TRIM_START_9103_ADDR)
#define ANA_RST_ADDR(ver)        ((((ver) == CHIP_RA9106B) || ((ver) == CHIP_RA9303B)) ? MADC_ANA_RST_9303_ADDR : MADC_ANA_RST_9103_ADDR)
#define TEST_CTL_ADDR(ver)       (((ver) == CHIP_RA9703A) ? TEST_CTL_9703_ADDR : TEST_CTL_9103_ADDR)
#define ANA_SADDR_ADDR(ver)      (((ver) == CHIP_RA9703A) ? ANA_SADDR_9703_ADDR : ANA_SADDR_9701_ADDR)
#define ANA_SDATA_ADDR(ver)      (((ver) == CHIP_RA9703A) ? ANA_SDATA_9703_ADDR : ANA_SDATA_9103_ADDR)
#define ANA_LDOMOD_ADDR(ver)     (((ver) == CHIP_RA9103G) ? ANA_LDOMOD_9103_ADDR : (((ver) == CHIP_RA9703A) ? ANA_LDOMOD_9703_ADDR : (((ver) == CHIP_RA9303B) ? ANA_LDOMOD_9303_ADDR : ANA_LDOMOD_9701_ADDR)))
#define ANA_LDO_ADDR(ver)        (((ver) == CHIP_RA9303B) ? ANA_LDO_9303_ADDR : (((ver) == CHIP_RA9103G) ? ANA_LDOMOD_9103_ADDR : ANA_LDO_9103_ADDR))

#define MADC_ANA_PS_ADDR(ver)    ((((ver) == CHIP_RA9106B) || ((ver) == CHIP_RA9303B)) ? MADC_ANA_PS_9303_ADDR : MADC_ANA_PS_9701_ADDR)
#define MADC_TPSCTRL_ADDR(ver)   ((((ver) == CHIP_RA9106B) || ((ver) == CHIP_RA9303B)) ? MADC_TPSCTRL_9303_ADDR : (((ver) < CHIP_RA9106A) ? MADC_TPSCTRL_9103_ADDR : MADC_TPSCTRL_9701_ADDR))
#define MADC_ADTEST_ADDR(ver)    ((((ver) == CHIP_RA9106B) || ((ver) == CHIP_RA9303B)) ? MADC_ADTEST_9303_ADDR : (((ver) < CHIP_RA9106A) ? MADC_ADTEST_9103_ADDR : MADC_ADTEST_9701_ADDR))
#define MADC_ANABGR_ADDR(ver)    ((((ver) == CHIP_RA9106B) || ((ver) == CHIP_RA9303B)) ? MADC_ANABGR_9303_ADDR : (((ver) < CHIP_RA9106A) ? MADC_ANABGR_9103_ADDR : MADC_ANABGR_9701_ADDR))
#define MADC_ANALDO_ADDR(ver)    ((((ver) == CHIP_RA9106B) || ((ver) == CHIP_RA9303B)) ? MADC_ANALDO_9303_ADDR : (((ver) < CHIP_RA9106A) ? MADC_ANALDO_9103_ADDR : MADC_ANALDO_9701_ADDR))
#define MADC_ANARCH_ADDR(ver)    ((((ver) == CHIP_RA9106B) || ((ver) == CHIP_RA9303B)) ? MADC_ANARCH_9303_ADDR : (((ver) < CHIP_RA9106A) ? MADC_ANARCH_9103_ADDR : MADC_ANARCH_9701_ADDR))
#define MADC_SARCTRL2_ADDR(ver)  ((((ver) == CHIP_RA9106B) || ((ver) == CHIP_RA9303B)) ? MADC_SARCTRL2_9303_ADDR : (((ver) < CHIP_RA9106A) ? MADC_SARCTRL2_9103_ADDR : MADC_SARCTRL2_9701_ADDR))
#define MADC_ANA_RST_ADDR(ver)   ((((ver) == CHIP_RA9106B) || ((ver) == CHIP_RA9303B)) ? MADC_ANA_RST_9303_ADDR : MADC_ANA_RST_9103_ADDR)
#define MADC_ANA_TEST_ADDR(ver)  ((((ver) == CHIP_RA9103E) || ((ver) == CHIP_RA9103F)) ? MADC_ANA_TEST_9103_ADDR : (((ver) == CHIP_RA9303B) ? MADC_ANA_TEST_9303_ADDR : MADC_ANA_TEST_9701_ADDR))
#define MADC_ANA_CTL_ADDR(ver)   ((((ver) == CHIP_RA9103E) || ((ver) == CHIP_RA9103F)) ? MADC_ANA_CTL_9103_ADDR : (((ver) == CHIP_RA9303B) ? MADC_ANA_CTL_9303_ADDR : MADC_ANA_CTL_9701_ADDR))
#define TRNG_CTRL_ADDR(ver)      (((ver) == CHIP_RA9703A) ? TRNG_CTRL_9703_ADDR : (((ver) == CHIP_RA9702B) ? TRNG_CTRL_9702B_ADDR : TRNG_CTRL_9701_ADDR))
#endif

#if defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_B)  ||\
    defined(RN821x_RN721x_SOC_C)  ||\
    defined(RN821x_RN721x_SOC_D)  ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3)
#define SYS_PS_ADDR         (SYS_PS_9103_ADDR)
#define OSC_CTRL2_ADDR      (OSC_CTRL2_9103_ADDR)
#define MAP_CTL_ADDR        (MAP_CTL_9103_ADDR)
#define MOD0_EN_ADDR        (MOD0_EN_9103_ADDR)
#define MOD1_EN_ADDR        (MOD1_EN_9103_ADDR)
#define TRIM_CFG1_ADDR      (TRIM_CFG1_9103_ADDR)
#define TRIM_START_ADDR     (TRIM_START_9103_ADDR)
#define TEST_CTL_ADDR       (TEST_CTL_9103_ADDR)
#elif defined(RN202x_RN7326_SOC_V2)
#define SYS_PS_ADDR         (SYS_PS_9303_ADDR)
#define OSC_CTRL2_ADDR      (OSC_CTRL2_9303_ADDR)
#define MAP_CTL_ADDR        (MAP_CTL_9303_ADDR)
#define MOD0_EN_ADDR        (MOD0_EN_9303_ADDR)
#define MOD1_EN_ADDR        (MOD1_EN_9303_ADDR)
#define TRIM_CFG1_ADDR      (TRIM_CFG1_9303_ADDR)
#define TRIM_START_ADDR     (TRIM_START_9303_ADDR)
#define TEST_CTL_ADDR       (TEST_CTL_9703_ADDR)
#endif

#if defined(RN821x_RN721x_SOC_D)
#define ANA_LDOMOD_ADDR     (ANA_LDOMOD_9103_ADDR)
#endif

#if defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3)
#define ANA_SADDR_ADDR      (ANA_SADDR_9701_ADDR)
#define ANA_SDATA_ADDR      (ANA_SDATA_9103_ADDR)
#define ANA_LDOMOD_ADDR     (ANA_LDOMOD_9701_ADDR)
#elif defined(RN202x_RN7326_SOC_V2)
#define ANA_SADDR_ADDR      (ANA_SADDR_9703_ADDR)
#define ANA_SDATA_ADDR      (ANA_SDATA_9703_ADDR)
#define ANA_LDOMOD_ADDR     (ANA_LDOMOD_9703_ADDR)
#endif

/* Register define */
#define FEED_WDT (MEM32(WDT_EN_ADDR) = 0xbbU)

/* for RA9103A/D */
#define FL_TW_32K_98NS  (0U)
#define FL_TW_1M_98NS   (0U)
#define FL_TW_3M_98NS   (0U)
#define FL_TW_7M_98NS   (2U)
#define FL_TW_14M_98NS  (2U)
#define FL_TW_29M_98NS  (5U)

/* for RA9103C/E/F */
#define FL_TW_32K_198NS  (0U)
#define FL_TW_1M_198NS   (0U)
#define FL_TW_3M_198NS   (1U)
#define FL_TW_7M_198NS   (2U)
#define FL_TW_14M_198NS  (4U)
#define FL_TW_29M_198NS  (8U)

#define EE_TW_32K_98NS   (0U)
#define EE_TW_1M_98NS    (0U)
#define EE_TW_3M_98NS    (0U)
#define EE_TW_7M_98NS    (1U)
#define EE_TW_14M_98NS   (2U)
#define EE_TW_29M_98NS   (5U)

#define EE_TW_32K_198NS  (0U)
#define EE_TW_1M_198NS   (0U)
#define EE_TW_3M_198NS   (1U)
#define EE_TW_7M_198NS   (2U)
#define EE_TW_14M_198NS  (5U)
#define EE_TW_29M_198NS  (10U)

/* for RA9701A */
#define FL_TW_32K_V2  (0U)
#define FL_TW_1M_V2   (0U)
#define FL_TW_3M_V2   (0U)
#define FL_TW_7M_V2   (0U)
#define FL_TW_14M_V2  (0U)
#define FL_TW_29M_V2  (1U)

#define SPL_TW_32K_ADDR (0x88U)
#define SPL_TW_1M_ADDR  (0x8aU)
#define SPL_TW_3M_ADDR  (0x8cU)
#define SPL_TW_7M_ADDR  (0x8eU)
#define SPL_TW_14M_ADDR (0x90U)
#define SPL_TW_29M_ADDR (0x92U)

#define WR_HC_MODE   (0xDU)
#define WR_RC_MODE   (0xEU)
#define WR_LC_MODE   (0xFU)

/* DON'T CHANGED existed chipid value for compatibility */
typedef enum {
    /*CHIP_RA9103A  = 0U,*/
    CHIP_RA9103D  = 1U,
    CHIP_RA9103C  = 2U,
    CHIP_RA9103E  = 3U,
    CHIP_RA9103F  = 4U,
    CHIP_RA9103G  = 5U,
    /*CHIP_RA9103H  = 6U,*/
    CHIP_RA9106A  = 7U,
    CHIP_RA9106B  = 8U,
    CHIP_RA9106C  = 9U,
    CHIP_RA9303A  = 10U,
    CHIP_RA9303B  = 11U,
    CHIP_RA9701A  = 12U,
    /* CHIP_RA9701A1 = 13U,*/
    CHIP_RA9702A  = 14U,
    CHIP_RA9703A  = 15U,
    CHIP_RA9701B  = 16U,
    CHIP_RA9702B  = 17U,
    CHIP_INVALID  = 18U
} rn_chipid_t;

#define  IS_G2_SERIES(ver)    (((ver) == CHIP_RA9303B) || ((ver) == CHIP_RA9703A) || ((ver) == CHIP_RA9106B))
#define  IS_RA97_SERIES(ver)  (((ver) == CHIP_RA9701A) || ((ver) == CHIP_RA9701B) || ((ver) == CHIP_RA9702A) || ((ver) == CHIP_RA9702B) || ((ver) == CHIP_RA9703A))

typedef enum {
    RET_INDEX = 0,
    RET_FREQ = 1
} rn_sys_freq_t;

#if defined(RN_CM0_PLATFORM)
/**
 * @brief Get the Chipid number
 *
 * @return rn_chipid_t: chipid number
 */
rn_chipid_t getChipid(void);
#endif

#if defined(RN_CM0_PLATFORM)
/**
 * @brief Get the Mip Addr object
 *
 * @param mem_type: 0 = FLASH, 1 = EEPROM, 2 = ROM
 * @return unsigned int: memory base address
 */
unsigned int getMipAddr(unsigned char mem_type, rn_chipid_t chipid);
#endif

unsigned char currentClk_idx(void);
#if defined(RN_CM0_PLATFORM)
/**
 * @brief get current run clock
 *
 * @param chipid: chip index
 * @return unsigned int: clock frequency
 */
unsigned int currentClk(rn_chipid_t chipid);
#else
unsigned int currentClk(void);
#endif

#if defined(RN_CM0_PLATFORM)
/**
 * @brief compare register with val in timeout
 * @param addr register address
 * @param val compared data
 * @param sign compared mode:
 *   sign = 1, exit while val != ([addr] & val)
 *   sign = 0, exit while val == ([addr] & val)
 * @param timeout compared timeout(us unit)
 * @param chipid chip id
 */
void usTimeout(unsigned int addr, unsigned int val, unsigned int sign, unsigned int timeout, rn_chipid_t chipid);
#else
void usTimeout(unsigned int addr, unsigned int val, unsigned int sign, unsigned int timeout);
#endif

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
unsigned char check_spl_valid(const unsigned int *p_addr, unsigned int len, unsigned char flag);

#if defined(RN_CM0_PLATFORM)
/**
 * @brief rtc wrong state check
 *   1. check if some of VBAT power domain registers are unexcepted
 *   2. check if rtc calc busy is always on(if yes, VBAT power domain is in wrong state)
 * @return unsigned char: 0 = OK, 1 = FAIL
 */
unsigned char RtcCalCheck(rn_chipid_t chipid);
#else
unsigned char RtcCalCheck(void);
#endif

#if defined(RN_CM0_PLATFORM)
/**
 * @brief soft reset vbat power domain
 *
 */
void vbat_reset(rn_chipid_t chipid);
#else
void vbat_reset(void);
#endif

typedef void (*IAP_Entry)(unsigned long *cmd, unsigned long *stat);
typedef union {
    IAP_Entry iap_func;
    unsigned int addr;
} rn_iap_entry_t;

#define IAP_OFFSET 0x1c01U
typedef struct {
    unsigned long cmd;
    unsigned long par[4];
    unsigned long stat;
} rn_iap_t;

typedef enum {
    FL_MEM_EN = 0U,
    EE_MEM_EN = 1U
} rn_iap_mem_en_t;

/**
 * @brief Enable IAP NVM operation
 *
 * @param nvm_type: 0 = FLASH, 1 = EEPROM
 * @return unsigned char: ref IAP interface in datasheet
 */
unsigned char IapMemEnable(rn_iap_mem_en_t nvm_type);

#if defined(RN_CM0_PLATFORM)
/**
 * @brief open dig5 write window
 *
 * @param chipid chip index
 */
void open_dig5(rn_chipid_t chipid);
#else
void open_dig5(void);
#endif

#if defined(RN_CM0_PLATFORM)
/**
 * @brief close dig5 write window
 *
 * @param chipid chip index
 */
void close_dig5(rn_chipid_t chipid);
#else
void close_dig5(void);
#endif

#if defined(RN831x_RN861x_MCU_V1)
rn_chipid_t get_mcu_v1_version(void);
#endif

#if 0
#if defined(RN202x_RN7326_SOC_B) ||\
    defined(RN202x_RN7326_SOC_V2)
rn_chipid_t get_three_phase_soc_version(void);
#endif
#endif

#if defined(RN_CM0_PLATFORM)
unsigned char get_sysreg_reliable(rn_chipid_t chipid, unsigned int addr, unsigned int *data, unsigned char flag);
#else
unsigned char get_sysreg_reliable(unsigned int addr, unsigned int *data, unsigned char flag);
#endif

#if defined(RN_CM0_PLATFORM)
unsigned char set_sysreg_reliable(rn_chipid_t chipid, unsigned int addr, unsigned int data, unsigned int mask, unsigned char flag);
#else
unsigned char set_sysreg_reliable(unsigned int addr, unsigned int data, unsigned int mask, unsigned char flag);
#endif

#if (defined(RN831x_RN861x_MCU_V2) ||\
     defined(RN831x_RN861x_MCU_V3) ||\
     defined(RN821x_RN721x_SOC_V2) ||\
     defined(RN821x_RN721x_SOC_V3) ||\
     defined(RN202x_RN7326_SOC_V2)) && defined(NVRLIB_EN)

#include "rn8xxx_v2_lib.h"
typedef struct RN_LIB_T_ {
    unsigned int            (*LL_SYSCLK_GetLibVersion)(void);
    unsigned int            (*LL_SYSCLK_GetCodeVersion)(void);
    eSysclkRet_TypeDef      (*LL_SYSCLK_SysModeChg)(eSysclkMode_TypeDef mode, eSysclkDiv_TypeDef cdiv);
    void                    (*LL_SYSCLK_FastSysModeChg)(eSysclkMode_TypeDef mode);
    void                    (*LL_SYSCLK_NvmActive)(void);
    void                    (*LL_SYSCLK_NvmSleep)(void);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_Dota0CfgSet)(unsigned short dota0);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_AlphalCfgSet)(unsigned short alphal);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_AlphahCfgSet)(unsigned short alphah);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_Xt0CfgSet)(unsigned short xt0);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_Remap4Sel)(unsigned char sel);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_Dota0CfgGet)(unsigned short *dota0);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_AlphalCfgGet)(unsigned short *alphal);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_AlphahCfgGet)(unsigned short *alphah);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_Xt0CfgGet)(unsigned short *xt0);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_RemapGet)(unsigned short *remap);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_SysConfCheck)(void);
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_FlashVoltageTrim)(void);
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3)
    eMcuVer_TypeDef         (*LL_SYSOPTION_GetMcuVersion)(void);
#endif
    unsigned short          (*LL_RTC_ReadDota)(void);
    eRtcRet_TypeDef         (*LL_RTC_WriteDota)(unsigned short dota);
    eFlashRet_TypeDef       (*LL_FLASH_PageErase)(unsigned int pg);
    eFlashRet_TypeDef       (*LL_FLASH_SectorErase)(unsigned int sec);
    eFlashRet_TypeDef       (*LL_FLASH_Program)(unsigned int dst_addr, unsigned int src_addr, unsigned int len);
    /* Don't add or delete any api until here! */
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B) ||\
    defined(RN202x_RN7326_SOC_V2)
    eEmuLibRet_TypeDef      (*LL_EMU_ExtRefConf)(eExtRefSel_TypeDef ref_sel);
    eGpadcLibRet_TypeDef    (*LL_GPADC_TadjGet)(unsigned int *tadj);
#endif
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_RtcConfCheck)(void);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_WdtConfCheck)(void);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_RchConfCheck)(void);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_RclConfCheck)(void);
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN831x_RN861x_MCU_V1)
    eEepromRet_TypeDef      (*LL_EEPROM_Program)(unsigned int dst_addr, unsigned int src_addr, unsigned int len);
    void                    (*LL_EEPROM_Standby)(void);
    void                    (*LL_EEPROM_Wakeup)(void);
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B)
    eEmuLibRet_TypeDef      (*LL_EMU_ReadUEdtVBCtlReg)(unsigned int RegData , unsigned char ModeSel);
    eEmuLibRet_TypeDef      (*LL_EMU_WriteUEdtVBCtlReg)(unsigned int RegData , unsigned char ModeSel);
#endif
#if !(defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3))
    void                    (*LL_FLASH_CacheOn)(void);
    void                    (*LL_FLASH_CacheOff)(void);
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3)
    void                    (*LL_RTC_ClkSel)(rtc_clk_e clk_sel);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_CpLevelSet)(unsigned int cp_level, unsigned int cp_passwd);
    eSysoptionRet_TypeDef   (*LL_SYSOPTION_CpLevelGet)(unsigned int *cp_level);
#endif
} rn_lib_t;

extern const rn_lib_t  *rn_lib;
#endif
#else
#include <stdint.h>

#define ROM_SIZE                        (48 * 1024)
#define RN_LIB_OFFSET                   (9U * 4U)
#define ROM_BASE_ADDR                   (0x200000U)

typedef enum
{
    IAP_PASS = 0,
    IAP_FAIL = 1
} IapCheckStatus;

IapCheckStatus getIAPHeaderAddress(uint32_t *Address);
#endif
#endif
/* r2989 */
