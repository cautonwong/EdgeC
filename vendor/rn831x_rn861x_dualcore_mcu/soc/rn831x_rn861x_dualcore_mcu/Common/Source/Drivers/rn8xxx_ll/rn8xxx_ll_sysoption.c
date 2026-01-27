/******************************************************************************
 * @file     rn8xxx_ll_sysoption.c
 * @brief    system option driver
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) 2008, Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#include "rn8xxx_ll_utils.h"
#include "rn8xxx_ll_sysoption.h"

#if !defined(RN831x_RN861x_MCU_V4)
#if !defined(NVRLIB_EN)
#if defined(RN_CM0_PLATFORM)
typedef enum
{
    REG_ANA_ADC = 0U,
    REG_ANA_CLK1,
    REG_ANA_CLK2,
    REG_ANA_LDO,
    REG_ANA_BGR,
    REG_ANA_PSW,
    REG_ANA_PWR,
    REG_ANA_CLK3,
    REG_ANA_SYS,
    REG_ANA_ULBGR1,
    REG_ANA_ULBGR2,
    REG_ANA_PWS,
    REG_ANA_PSM,
    REG_ANA_ADC2,
    REG_ANA_9702_BGR2,
    REG_ANA_9703_BGR2,
    REG_ANA_BGR2_9303,
    REG_ANA_PWR2,
    REG_ANA_PSM2,
    REG_ANA_CKTIM_9702,
    REG_ANA_CKTIM_9703,
    REG_ANA_CKTIM2,
    REG_ANA_LURLS_CFG_9701,
    REG_ANA_LURLS_EN_9701,
    REG_ANA_LURLS_CFG_9703,
    REG_ANA_LURLS_EN_9703,
    REG_MADC_TPSCTRL_9103,
    REG_MADC_TPSCTRL_9701,
    REG_MADC_TPSCTRL_9303,
    REG_MADC_ADTEST_9103,
    REG_MADC_ADTEST_9701,
    REG_MADC_ADTEST_9303,
    REG_MADC_ANABGR_9103,
    REG_MADC_ANABGR_9701,
    REG_MADC_ANABGR_9303,
    REG_MADC_ANALDO_9103,
    REG_MADC_ANALDO_9701,
    REG_MADC_ANALDO_9303,
    REG_MADC_ANARCH_9103,
    REG_MADC_ANARCH_9701,
    REG_MADC_ANARCH_9303,
    REG_MADC_SARCTRL2_9103,
    REG_MADC_SARCTRL2_9701,
    REG_MADC_SARCTRL2_9303,
    REG_MADC_TPSCTRL2,
    REG_MADC_TPSCTRL3,
    REG_RTC_MODE,
    REG_RTC_BETA,
    REG_RTC_TADJ_9103,
    REG_RTC_TADJ_9702,
    REG_RTC_ZT_9103,
    REG_RTC_ZT_9702,
    REG_RTC_ALPHAL,
    REG_RTC_ALPHAH,
    REG_RTC_XT0,
    REG_LOSC_CFG2,
    REG_WDT_CTRL,
    REG_WDT_HALT,
    REG_WDT_STBY,
    REG_TRNG_CTRL_9701,
    REG_TRNG_CTRL_9703,
    REG_TRNG_CTRL_9702B,
    REG_ANA_ADC_9303,
    REG_ANA_CLK1_9303,
    REG_ANA_LDO_9303,
    REG_ANA_LDO33_9303,
    REG_ANA_BGR_9303,
    REG_ANA_PSW_9303,
    REG_ANA_PWR_9303,
    REG_ANA_PWR2_9303,
    REG_ANA_TCLDO15_9303,
    REG_ANA_TCICC04_9303,
    REG_ANA_CKTIM_9303,
    REG_ANA_SYS_9303,
    REG_ANA_RC32M_CTL2_9303
} conf_reg_idx_e;
#elif defined(RN831x_RN861x_MCU_V1)
typedef enum
{
    REG_ANA_ADC = 0U,
    REG_ANA_CLK1,
    REG_ANA_CLK2,
    REG_ANA_LDO,
    REG_ANA_PSW,
    REG_ANA_CLK3,
    REG_ANA_SYS,
    REG_MADC_TPSCTRL_9103,
    REG_MADC_ADTEST_9103,
    REG_MADC_ANABGR_9103,
    REG_MADC_ANALDO_9103,
    REG_MADC_ANARCH_9103,
    REG_MADC_SARCTRL2_9103,
    REG_RTC_MODE,
    REG_RTC_BETA,
    REG_RTC_TADJ_9103,
    REG_RTC_ZT_9103,
    REG_LOSC_CFG2,
    REG_WDT_CTRL,
    REG_WDT_HALT,
    REG_WDT_STBY
} conf_reg_idx_e;
#elif defined(RN831x_RN861x_MCU_V2)
typedef enum
{
    REG_ANA_CLK1 = 0U,
    REG_ANA_CLK2,
    REG_ANA_LDO,
    REG_ANA_CLK3,
    REG_ANA_SYS,
    REG_ANA_ULBGR1,
    REG_MADC_TPSCTRL_9701,
    REG_MADC_ADTEST_9701,
    REG_MADC_ANABGR_9701,
    REG_MADC_ANALDO_9701,
    REG_MADC_ANARCH_9701,
    REG_MADC_SARCTRL2_9701,
    REG_RTC_MODE,
    REG_RTC_TADJ_9103,
    REG_RTC_ZT_9103,
    REG_RTC_ALPHAL,
    REG_RTC_ALPHAH,
    REG_RTC_XT0,
    REG_LOSC_CFG2,
    REG_WDT_CTRL,
    REG_WDT_HALT,
    REG_WDT_STBY,
    REG_TRNG_CTRL_9701
} conf_reg_idx_e;
#elif defined(RN831x_RN861x_MCU_V3)
typedef enum
{
    REG_ANA_CLK1 = 0U,
    REG_ANA_CLK2,
    REG_ANA_LDO,
    REG_ANA_CLK3,
    REG_ANA_SYS,
    REG_ANA_ULBGR1,
    REG_ANA_LURLS_CFG_9701,
    REG_ANA_LURLS_EN_9701,
    REG_MADC_TPSCTRL_9701,
    REG_MADC_ADTEST_9701,
    REG_MADC_ANABGR_9701,
    REG_MADC_ANALDO_9701,
    REG_MADC_ANARCH_9701,
    REG_MADC_SARCTRL2_9701,
    REG_RTC_MODE,
    REG_RTC_TADJ_9103,
    REG_RTC_ZT_9103,
    REG_RTC_ALPHAL,
    REG_RTC_ALPHAH,
    REG_RTC_XT0,
    REG_LOSC_CFG2,
    REG_WDT_CTRL,
    REG_WDT_HALT,
    REG_WDT_STBY
} conf_reg_idx_e;
#elif defined(RN821x_RN721x_SOC_B)
typedef enum
{
    REG_ANA_ADC = 0U,
    REG_ANA_CLK1,
    REG_ANA_CLK2,
    REG_ANA_LDO,
    REG_ANA_BGR,
    REG_ANA_PSW,
    REG_MADC_TPSCTRL_9103,
    REG_MADC_ADTEST_9103,
    REG_RTC_MODE,
    REG_RTC_TADJ_9103,
    REG_RTC_ZT_9103,
    REG_WDT_CTRL,
    REG_WDT_HALT,
    REG_WDT_STBY
} conf_reg_idx_e;
#elif defined(RN821x_RN721x_SOC_C)
typedef enum
{
    REG_ANA_ADC = 0U,
    REG_ANA_CLK1,
    REG_ANA_CLK2,
    REG_ANA_LDO,
    REG_ANA_BGR,
    REG_ANA_PSW,
    REG_ANA_PWR,
    REG_ANA_CLK3,
    REG_ANA_SYS,
    REG_MADC_TPSCTRL_9103,
    REG_MADC_ADTEST_9103,
    REG_RTC_MODE,
    REG_RTC_TADJ_9103,
    REG_RTC_ZT_9103,
    REG_WDT_CTRL,
    REG_WDT_HALT,
    REG_WDT_STBY
} conf_reg_idx_e;
#elif defined(RN821x_RN721x_SOC_D)
typedef enum
{
    REG_ANA_ADC = 0U,
    REG_ANA_CLK1,
    REG_ANA_CLK2,
    REG_ANA_LDO,
    REG_ANA_BGR,
    REG_ANA_PSW,
    REG_ANA_PWR,
    REG_ANA_CLK3,
    REG_ANA_SYS,
    REG_MADC_TPSCTRL_9103,
    REG_MADC_ADTEST_9103,
    REG_RTC_MODE,
    REG_RTC_TADJ_9103,
    REG_RTC_ZT_9103,
    REG_WDT_CTRL,
    REG_WDT_HALT,
    REG_WDT_STBY
} conf_reg_idx_e;
#elif defined(RN821x_RN721x_SOC_V2)
typedef enum
{
    REG_ANA_ADC = 0U,
    REG_ANA_CLK1,
    REG_ANA_CLK2,
    REG_ANA_LDO,
    REG_ANA_CLK3,
    REG_ANA_BGR,
    REG_ANA_PSW,
    REG_ANA_PWR,
    REG_ANA_SYS,
    REG_ANA_ULBGR1,
    REG_ANA_PWS,
    REG_ANA_PSM,
    REG_ANA_ADC2,
    REG_ANA_9702_BGR2,
    REG_ANA_PWR2,
    REG_ANA_PSM2,
    REG_ANA_CKTIM_9702,
    REG_ANA_CKTIM2,
    REG_ANA_LURLS_CFG_9701,
    REG_ANA_LURLS_EN_9701,
    REG_MADC_TPSCTRL_9701,
    REG_MADC_ADTEST_9701,
    REG_MADC_SARCTRL2_9701,
    REG_MADC_TPSCTRL2,
    REG_MADC_TPSCTRL3,
    REG_RTC_MODE,
    REG_RTC_TADJ_9103,
    REG_RTC_TADJ_9702,
    REG_RTC_ZT_9103,
    REG_RTC_ZT_9702,
    REG_RTC_ALPHAL,
    REG_RTC_ALPHAH,
    REG_RTC_XT0,
    REG_WDT_CTRL,
    REG_WDT_HALT,
    REG_WDT_STBY,
    REG_TRNG_CTRL_9701
} conf_reg_idx_e;
#elif defined(RN821x_RN721x_SOC_V3)
typedef enum
{
    REG_ANA_ADC = 0U,
    REG_ANA_CLK1,
    REG_ANA_CLK2,
    REG_ANA_LDO,
    REG_ANA_CLK3,
    REG_ANA_BGR,
    REG_ANA_PSW,
    REG_ANA_PWR,
    REG_ANA_SYS,
    REG_ANA_ULBGR1,
    REG_ANA_PWS,
    REG_ANA_PSM,
    REG_ANA_ADC2,
    REG_ANA_9702_BGR2,
    REG_ANA_PWR2,
    REG_ANA_PSM2,
    REG_ANA_CKTIM_9702,
    REG_ANA_CKTIM2,
    REG_ANA_LURLS_CFG_9701,
    REG_ANA_LURLS_EN_9701,
    REG_MADC_TPSCTRL_9701,
    REG_MADC_ADTEST_9701,
    REG_MADC_SARCTRL2_9701,
    REG_MADC_TPSCTRL2,
    REG_MADC_TPSCTRL3,
    REG_RTC_MODE,
    REG_RTC_TADJ_9103,
    REG_RTC_TADJ_9702,
    REG_RTC_ZT_9103,
    REG_RTC_ZT_9702,
    REG_RTC_ALPHAL,
    REG_RTC_ALPHAH,
    REG_RTC_XT0,
    REG_WDT_CTRL,
    REG_WDT_HALT,
    REG_WDT_STBY,
    REG_TRNG_CTRL_9702B
} conf_reg_idx_e;
#elif defined(RN202x_RN7326_SOC_B)
typedef enum
{
    REG_ANA_ADC_9303 = 0U,
    REG_ANA_CLK1_9303,
    REG_ANA_LDO_9303,
    REG_ANA_LDO33_9303,
    REG_ANA_BGR_9303,
    REG_ANA_BGR2_9303,
    REG_ANA_PSW_9303,
    REG_ANA_PWR_9303,
    REG_ANA_PWR2_9303,
    REG_ANA_TCLDO15_9303,
    REG_ANA_TCICC04_9303,
    REG_ANA_CKTIM_9303,
    REG_ANA_SYS_9303,
    REG_ANA_RC32M_CTL2_9303,
    REG_MADC_TPSCTRL_9303,
    REG_MADC_ADTEST_9303,
    REG_MADC_ANABGR_9303,
    REG_MADC_ANALDO_9303,
    REG_MADC_ANARCH_9303,
    REG_MADC_SARCTRL2_9303,
    REG_RTC_MODE,
    REG_RTC_TADJ_9103,
    REG_RTC_ZT_9103,
    REG_LOSC_CFG2
} conf_reg_idx_e;
#elif defined(RN202x_RN7326_SOC_V2)
typedef enum
{
    REG_ANA_ADC = 0U,
    REG_ANA_CLK1,
    REG_ANA_CLK2,
    REG_ANA_LDO,
    REG_ANA_CLK3,
    REG_ANA_BGR,
    REG_ANA_PWR,
    REG_ANA_SYS,
    REG_ANA_ULBGR1,
    REG_ANA_9703_BGR2,
    REG_ANA_CKTIM_9703,
    REG_ANA_LURLS_CFG_9703,
    REG_ANA_LURLS_EN_9703,
    REG_MADC_TPSCTRL_9701,
    REG_MADC_ADTEST_9701,
    REG_MADC_ANABGR_9701,
    REG_MADC_ANALDO_9701,
    REG_MADC_ANARCH_9701,
    REG_MADC_SARCTRL2_9701,
    REG_RTC_MODE,
    REG_RTC_TADJ_9103,
    REG_RTC_ZT_9103,
    REG_RTC_ALPHAL,
    REG_RTC_ALPHAH,
    REG_RTC_XT0,
    REG_LOSC_CFG2,
    REG_WDT_CTRL,
    REG_WDT_HALT,
    REG_WDT_STBY,
    REG_TRNG_CTRL_9703
} conf_reg_idx_e;
#endif

typedef struct
{
    conf_reg_idx_e  reg_id;
    /* 0 means spl0, 1 means spl1 */
    unsigned char   reg_spl_idx;
    /* offset in spl */
    unsigned char   reg_spl_addr;
    /* read and write data mask */
    unsigned short  reg_mask;
    unsigned short  reg_default;
    unsigned int    reg_addr;
    unsigned int    reg_passwd;
} conf_info_t;

/* IAP command define */
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
#define IAP_NVRE 0x70U
#define IAP_FW 0x58U
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN831x_RN861x_MCU_V1)
#define IAP_EW 0x59U
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_D) ||\
    defined(RN202x_RN7326_SOC_B) ||\
    defined(RN831x_RN861x_MCU_V1)
#define FL_CLR_HVPL   (0xfeed0000U | 0x4U)
#define FL_ERASE_PAGE (0xfeed0000U | 0xbU)
#define FL_PROG_PAGE  (0xfeed0000U | 0xfU)
#endif

#define CP0_MAGIC        (0x00000000U)
#define CP1_MAGIC        (0x12345678U)
#define CP2_MAGIC        (0x87654321U)
#define CP3_MAGIC        (0x4e647370U)
#define CP4_MAGIC        (0x9d61629eU)

#define SPL_CP_ADDR      (0x0)
#define SPL_PW_ADDR      (0x4)
#define SPL_DOTA0_ADDR   (0xaU)
#define SPL_ALPHAL_ADDR  (0xcU)
#define SPL_ALPHAH_ADDR  (0xeU)
#define SPL_XT0_ADDR     (0x10U)

/*
#define CP0_MAGIC        (0x00000000U)
#define CP1_MAGIC        (0x12345678U)
#define CP2_MAGIC        (0x87654321U)
#define CP3_MAGIC        (0x4e647370U)
#define CP4_MAGIC        (0x9d61629eU)
*/

#define REMAP4_MAGIC     (0x81a57e5aU)
#define REMAP1_MAGIC     (0x4a38967eU)

#if defined(RN_CM0_PLATFORM)
typedef enum {
    IN_FLASH = 0,
    IN_EEPROM = 1,
    IN_FLASH2 = 2
} spl_type_e;
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN202x_RN7326_SOC_B)
typedef enum {
    IN_FLASH = 0
} spl_type_e;
#elif defined(RN821x_RN721x_SOC_C) ||\
      defined(RN821x_RN721x_SOC_B)
typedef enum {
    IN_EEPROM = 1
} spl_type_e;
#elif defined(RN831x_RN861x_MCU_V1)
typedef enum {
    IN_EEPROM = 1,
    IN_FLASH2 = 2
} spl_type_e;
#else
typedef enum {
    IN_FLASH2 = 2
} spl_type_e;
#endif

typedef struct {
    unsigned int base_addr;
    unsigned int reg_addr;
    unsigned int spl0_offset;
    unsigned short page_size;
    spl_type_e spl_type;
} flash_info_t;

typedef struct {
    unsigned short offset_addr;
    unsigned short value;
} spl_data_t;

#if defined(RN_CM0_PLATFORM)
static unsigned int sar_measure(rn_chipid_t chipid);
static eSysoptionRet_TypeDef ldo_adj_a1(rn_chipid_t chipid);
static eSysoptionRet_TypeDef ldo_auto_adj(void);
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN202x_RN7326_SOC_V2)
static unsigned int sar_measure(void);
static eSysoptionRet_TypeDef ldo_adj_a1(void);
static eSysoptionRet_TypeDef ldo_auto_adj(void);
#endif

#if defined(RN_CM0_PLATFORM)
static void get_flash_info(rn_chipid_t chipid, flash_info_t * fl_info);
#else
static void get_flash_info(flash_info_t * fl_info);
#endif
static eSysoptionRet_TypeDef ReadSpl1Data(flash_info_t *fl_info, unsigned int data[]);
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_D) ||\
    defined(RN831x_RN861x_MCU_V1)
static void WaitFalsh(const volatile unsigned int *fl_sta);
#if defined(RN_CM0_PLATFORM)
static eSysoptionRet_TypeDef UpdateSpl1(rn_chipid_t chipid, flash_info_t *fl_info, const unsigned int data[]);
#elif defined(RN831x_RN861x_MCU_V1) ||\
      defined(RN821x_RN721x_SOC_D) ||\
      defined(RN202x_RN7326_SOC_B)
static eSysoptionRet_TypeDef UpdateSpl1(flash_info_t *fl_info, const unsigned int data[]);
#endif
#endif
static eSysoptionRet_TypeDef UpdateObjects(const spl_data_t spl_data[], unsigned short len);
static eSysoptionRet_TypeDef ReadObjects(spl_data_t spl_data[], unsigned short len);

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
#define ERR_RANGE  (8U)
#define TARGET_V  (800U)
/* max measure loop */
#define N  (10U)
/* ulbgr stable time(us unit) when vsel modify=7ms (1300) */
#define ULBGR_STABLE_T  (1300U)
/* SAR measure timeout(us unit) Ttps=11.2ms  --> [3*Ttps] (35000) */
#define SAR_T (35000U)
/* +/-10% 20230613 fix! */
#define V0P8_MIN (720U)
#define V0P8_MAX (880U)

#if defined(RN_CM0_PLATFORM)
static unsigned int sar_measure(rn_chipid_t chipid)
#else
static unsigned int sar_measure(void)
#endif
{
    unsigned int i;
    unsigned int vol = 0U;

    for (i = 0U; i < 500U; i++) {
        MEM32(MADC_SAR_STATUS_ADDR) = 1U;
        if ((MEM32(MADC_SAR_STATUS_ADDR) & 0x1U) == 0x0U) {
            break;
        }
    }
    MEM32(MADC_SAR_START_ADDR) = 1U;
    /* T=360us, timeout=4*T */
#if defined(RN_CM0_PLATFORM)
    usTimeout(MADC_SAR_STATUS_ADDR, 1U, 0U, SAR_T, chipid);
#else
    usTimeout(MADC_SAR_STATUS_ADDR, 1U, 0U, SAR_T);
#endif
    if (0x1U == (MEM32(MADC_SAR_STATUS_ADDR) & 0x1U)) {
        vol = (5000U * MEM32(MADC_SAR_DAT2_ADDR)) / 16384U;
        if ((vol < V0P8_MIN) || (vol > V0P8_MAX)) {
            vol = 0U;
        }
        MEM32(MADC_SAR_STATUS_ADDR) = 0x1U;
    }

    return vol;
}

#if defined(RN_CM0_PLATFORM)
static eSysoptionRet_TypeDef ldo_adj_a1(rn_chipid_t chipid)
#else
static eSysoptionRet_TypeDef ldo_adj_a1(void)
#endif
{
#if defined(RN_CM0_PLATFORM)
    const unsigned int spl0_base = getMipAddr(0U, chipid) + (512U * 1024U);
    const unsigned int sysps_bak = MEM32(SYS_PS_ADDR(chipid));
    const unsigned int mod1_bak = MEM32(MOD1_EN_ADDR(chipid));
#elif defined(RN202x_RN7326_SOC_V2)
    const unsigned int spl0_base = 512U * 1024U;
    const unsigned int sysps_bak = MEM32(SYS_PS_9303_ADDR);
    const unsigned int mod1_bak = MEM32(MOD1_EN_9303_ADDR);
#else
    const unsigned int spl0_base = 512U * 1024U;
    const unsigned int sysps_bak = MEM32(SYS_PS_9103_ADDR);
    const unsigned int mod1_bak = MEM32(MOD1_EN_9103_ADDR);
#endif
    unsigned int sarctl_bak;
    unsigned int vol = 0U;
    unsigned int vol_pre;
    unsigned int ulbgr2;
    unsigned int vsel;
    unsigned char sar_restore;
    unsigned char retry, i;
    unsigned char dir;
    unsigned char sar_matched = 0U;
    eSysoptionRet_TypeDef res = SYSOPTION_PASS;

    if (0x1324U == *(unsigned short *)(spl0_base + 0x76U)) {
        res = SYSOPTION_PASS;
    } else {
#if defined(RN_CM0_PLATFORM)
        MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
        MEM32(MOD1_EN_ADDR(chipid)) |= (((unsigned int)1U << 10) | ((unsigned int)1U << 11));

        MEM32(SYS_PS_ADDR(chipid)) = 0x75U;
        /* vref_0p8 testen */
        MEM32(TEST_CTL_ADDR(chipid)) = ((CHIP_RA9701A == chipid) || (CHIP_RA9701B == chipid)) ?
                                      (0xa5730000U | ((unsigned int)1U << 6) | ((unsigned int)1U << 2))
                                    : ((0xa5730000U | ((unsigned int)1U << 6) | ((unsigned int)1U << 2)) | ((unsigned int)0xa5 << 8));
#elif defined(RN202x_RN7326_SOC_V2)
        MEM32(SYS_PS_9303_ADDR) = 0x82U;
        MEM32(MOD1_EN_9303_ADDR) |= (((unsigned int)1U << 10) | ((unsigned int)1U << 11));

        MEM32(SYS_PS_9303_ADDR) = 0x75U;
        /* vref_0p8 testen */
        MEM32(TEST_CTL_9703_ADDR) = (0xa5730000U | ((unsigned int)1U << 6) | ((unsigned int)1U << 2)) | ((unsigned int)0xa5 << 8);
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3)
        MEM32(SYS_PS_ADDR) = 0x82U;
        MEM32(MOD1_EN_ADDR) |= (((unsigned int)1U << 10) | ((unsigned int)1U << 11));

        MEM32(SYS_PS_ADDR) = 0x75U;
        /* vref_0p8 testen */
        MEM32(TEST_CTL_ADDR) = (0xa5730000U | ((unsigned int)1U << 6) | ((unsigned int)1U << 2));
#elif defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
        MEM32(SYS_PS_ADDR) = 0x82U;
        MEM32(MOD1_EN_ADDR) |= (((unsigned int)1U << 10) | ((unsigned int)1U << 11));

        MEM32(SYS_PS_ADDR) = 0x75U;
        /* vref_0p8 testen */
        MEM32(TEST_CTL_ADDR) = (0xa5730000U | ((unsigned int)1U << 6) | ((unsigned int)1U << 2)) | ((unsigned int)0xa5 << 8);
#endif
        sarctl_bak = MEM32(MADC_SAR_CTL_ADDR);
        if (0x1U == MEM32(MADC_SAR_START_ADDR)) {
            sar_restore = 1U;
#if defined(RN_CM0_PLATFORM)
            usTimeout(MADC_SAR_START_ADDR, 1U, 1U, SAR_T, chipid);
#else
            usTimeout(MADC_SAR_START_ADDR, 1U, 1U, SAR_T);
#endif
        } else if (0x1U == (MEM32(MADC_SAR_STATUS_ADDR) & 0x1U)) {
            sar_restore = 1U;
        } else {
            sar_restore = 0U;
        }

        /* ch=0xd, pga=1 */
        MEM32(MADC_SAR_CTL_ADDR) = ((unsigned int)0x5U << 12) | ((unsigned int)0xeU << 7) | ((unsigned int)1U << 6) | ((unsigned int)5U << 0) | ((unsigned int)1U << 3);
        for (i = 0U; i < 2U; i++) {
            retry = 0U;
            do {
#if defined(RN_CM0_PLATFORM)
                vol = sar_measure(chipid);
#else
                vol = sar_measure();
#endif
            } while((retry++ < 3U) && (vol == 0U));
            if (vol == 0U) {
#if defined(RN_CM0_PLATFORM)
                open_dig5(chipid);
#else
                open_dig5();
#endif
                if (i == 0U) {
                    MEM32(ANA_ULBGR2_ADDR) = 0xa5670000U | (*(volatile unsigned short *)(0x00080024U));
                } else {
                    MEM32(ANA_ULBGR2_ADDR) = 0xa5670000U | 0x2041U;
                }
#if defined(RN_CM0_PLATFORM)
                close_dig5(chipid);
#else
                close_dig5();
#endif
            } else {
                break;
            }
        }
        if ( ((vol > 0U) && (vol < (TARGET_V - ERR_RANGE))) || (vol > (TARGET_V + ERR_RANGE))) {
#if defined(RN_CM0_PLATFORM)
            MEM32(SYS_PS_ADDR(chipid)) = 0x75U;
#elif defined(RN202x_RN7326_SOC_V2)
            MEM32(SYS_PS_9303_ADDR) = 0x75U;
#else
            MEM32(SYS_PS_9103_ADDR) = 0x75U;
#endif
#if defined(RN_CM0_PLATFORM)
            open_dig5(chipid);
#else
            open_dig5();
#endif

            ulbgr2 = MEM32(ANA_ULBGR2_ADDR);
            vsel = (ulbgr2 >> 1) & 0x3fU;
            ulbgr2 = ulbgr2 & (~((unsigned int)0x3fU << 1));

            vol_pre = vol;
            /* 1=downward, 0=upward */
            dir = (vol > TARGET_V) ? 1U : 0U;

            for (retry = 0U; retry < N; retry++) {
#if defined(RN_CM0_PLATFORM)
                vol = sar_measure(chipid);
#else
                vol = sar_measure();
#endif
                if (vol == TARGET_V) {
                    sar_matched = 1U;
                } else if (vol == 0U) {
                    /* sar measure is invalid, do it again! */
                } else {
                    if (dir == 1U) {
                        /* downward */
                        if (vol < TARGET_V) {
                            if ((TARGET_V - vol) > (vol_pre - TARGET_V)) {
                                vsel++;
                                MEM32(ANA_ULBGR2_ADDR) = 0xa5670000U | ulbgr2 | (vsel << 1);
                            }
                            sar_matched = 1U;
                        } else {
                            vsel--;
                            if (vsel == 0U) {
                                res = SYSOPTION_FAIL;
                                sar_matched = 1U;
                            }
                        }
                    } else {
                        /* upward */
                        if (vol > TARGET_V) {
                            if ((vol - TARGET_V) > (TARGET_V - vol_pre)) {
                                vsel--;
                                MEM32(ANA_ULBGR2_ADDR) = 0xa5670000U | ulbgr2 | (vsel << 1);
                            }
                            sar_matched = 1U;
                        } else {
                            vsel++;
                            if (vsel >= 0x3fU) {
                                res = SYSOPTION_FAIL;
                                sar_matched = 1U;
                            }
                        }
                    }
                    if (1U != sar_matched) {
                        vol_pre = vol;
                        MEM32(ANA_ULBGR2_ADDR) = 0xa5670000U | ulbgr2 | (vsel << 1);
#if defined(RN_CM0_PLATFORM)
                        usTimeout(0x4U, 0U, 0U, ULBGR_STABLE_T, chipid);
#else
                        usTimeout(0x4U, 0U, 0U, ULBGR_STABLE_T);
#endif
                    }
                }

                if (1U == sar_matched) {
                    break;
                }
            }
#if defined(RN_CM0_PLATFORM)
            close_dig5(chipid);
#else
            close_dig5();
#endif
        }
#if defined(RN_CM0_PLATFORM)
        MEM32(TEST_CTL_ADDR(chipid))= 0xa5730000U;
#elif defined(RN202x_RN7326_SOC_V2)
        MEM32(TEST_CTL_9703_ADDR)= 0xa5730000U;
#else
        MEM32(TEST_CTL_9103_ADDR)= 0xa5730000U;
#endif

        MEM32(MADC_SAR_CTL_ADDR) = sarctl_bak;
        if (1U == sar_restore) {
            MEM32(MADC_SAR_START_ADDR) = 1U;
        }
#if defined(RN_CM0_PLATFORM)
        MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
        MEM32(MOD1_EN_ADDR(chipid)) = mod1_bak;
        MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
#elif defined(RN202x_RN7326_SOC_V2)
        MEM32(SYS_PS_9303_ADDR) = 0x82U;
        MEM32(MOD1_EN_9303_ADDR) = mod1_bak;
        MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#else
        MEM32(SYS_PS_9103_ADDR) = 0x82U;
        MEM32(MOD1_EN_9103_ADDR) = mod1_bak;
        MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif
    }

    return res;
}

static eSysoptionRet_TypeDef ldo_auto_adj(void)
{
#if defined(RN_CM0_PLATFORM)
    eSysoptionRet_TypeDef res = SYSOPTION_PASS;
    const rn_chipid_t chipid = getChipid();

    if ((CHIP_RA9701A == chipid) ||\
        (CHIP_RA9701B == chipid) ||\
        (CHIP_RA9702A == chipid) ||\
        (CHIP_RA9702B == chipid) ||\
        (CHIP_RA9703A == chipid)) {
        res = ldo_adj_a1(chipid);
    }
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN202x_RN7326_SOC_V2)
    eSysoptionRet_TypeDef res;
    res = ldo_adj_a1();
#else
    res = SYSOPTION_PASS;
#endif
    return res;
}
#endif
#endif /* #if !defined(NVRLIB_EN) */

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
eSysoptionRet_TypeDef LL_SYSOPTION_FlashVoltageTrim(void)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_FlashVoltageTrim();
#else
    return ldo_auto_adj();
#endif
}
#endif

#if !defined(NVRLIB_EN)
#if defined(RN_CM0_PLATFORM) || defined(RN821x_RN721x_SOC_B)
static const conf_info_t spl_ra9103d_conf[14] = {
    {REG_ANA_ADC            , 0U, 0x10U, 0x03ffU, 0x0040U, ANA_ADC_ADDR           , 0xa5770000U},
    {REG_ANA_CLK1           , 0U, 0x12U, 0x7fffU, 0x0420U, ANA_CLK1_ADDR          , 0xa5270000U},
    {REG_ANA_CLK2           , 0U, 0x14U, 0xffffU, 0x4e14U, ANA_CLK2_ADDR          , 0xa5360000U},
    {REG_ANA_LDO            , 0U, 0x16U, 0x3ff0U, 0x0850U, ANA_LDO_9103_ADDR      , 0xa5580000U},
    {REG_ANA_BGR            , 0U, 0x18U, 0xffffU, 0x615aU, ANA_BGR_ADDR           , 0xa5280000U},
    {REG_ANA_PSW            , 0U, 0x1aU, 0x01ffU, 0x0010U, ANA_PSW_ADDR           , 0xa5790000U},
    {REG_MADC_TPSCTRL_9103  , 0U, 0x20U, 0x1fffU, 0x193bU, MADC_TPSCTRL_9103_ADDR , 0x0000e000U},
    {REG_MADC_ADTEST_9103   , 0U, 0x22U, 0x003fU, 0x0028U, MADC_ADTEST_9103_ADDR  , 0x0000a000U},
    {REG_RTC_MODE           , 1U, 0x08U, 0x0001U, 0x0001U, RTC_MODE_ADDR          , 0x00000000U},
    {REG_RTC_TADJ_9103      , 1U, 0x1cU, 0x07ffU, 0x0680U, RTC_TADJ_9103_ADDR     , 0x00000000U},
    {REG_RTC_ZT_9103        , 1U, 0x1eU, 0x7fffU, 0x0116U, RTC_ZT_9103_ADDR       , 0x00000000U},
    {REG_WDT_CTRL           , 1U, 0x14U, 0x00eeU, 0x0068U, WDT_CTRL_ADDR          , 0x00000000U},
    {REG_WDT_HALT           , 1U, 0x18U, 0xffffU, 0x7e5aU, WDT_HALT_ADDR          , 0x00000000U},
    {REG_WDT_STBY           , 1U, 0x1aU, 0xffffU, 0xa538U, WDT_STBY_ADDR          , 0x00000000U},
};
#endif

#if defined(RN_CM0_PLATFORM) || defined(RN821x_RN721x_SOC_C)
static const conf_info_t spl_ra9103c_conf[17] = {
    {REG_ANA_ADC            , 0U, 0x10U, 0x7fffU, 0x3440U, ANA_ADC_ADDR           , 0xa5770000U},
    {REG_ANA_CLK1           , 0U, 0x12U, 0x7fffU, 0x0060U, ANA_CLK1_ADDR          , 0xa5270000U},
    {REG_ANA_CLK2           , 0U, 0x14U, 0x1fffU, 0x0914U, ANA_CLK2_ADDR          , 0xa5360000U},
    {REG_ANA_LDO            , 0U, 0x16U, 0x3fffU, 0x145fU, ANA_LDO_9103_ADDR      , 0xa5580000U},
    {REG_ANA_BGR            , 0U, 0x18U, 0xffffU, 0x612aU, ANA_BGR_ADDR           , 0xa5280000U},
    {REG_ANA_PSW            , 0U, 0x1aU, 0x0088U, 0x0000U, ANA_PSW_ADDR           , 0xa5790000U},
    {REG_ANA_PWR            , 0U, 0x26U, 0x7fffU, 0x7000U, ANA_PWR_ADDR           , 0xa5430000U},
    {REG_ANA_CLK3           , 0U, 0x28U, 0xffffU, 0x3ce4U, ANA_CLK3_ADDR          , 0xa5750000U},
    {REG_ANA_SYS            , 0U, 0x2aU, 0x7fffU, 0x6c22U, ANA_SYS_ADDR           , 0xa5370000U},
    {REG_MADC_TPSCTRL_9103  , 0U, 0x20U, 0x1fffU, 0x193bU, MADC_TPSCTRL_9103_ADDR , 0x0000e000U},
    {REG_MADC_ADTEST_9103   , 0U, 0x22U, 0x003fU, 0x0028U, MADC_ADTEST_9103_ADDR  , 0x0000a000U},
    {REG_RTC_MODE           , 1U, 0x08U, 0x0001U, 0x0001U, RTC_MODE_ADDR          , 0x00000000U},
    {REG_RTC_TADJ_9103      , 1U, 0x1cU, 0x07ffU, 0x0680U, RTC_TADJ_9103_ADDR     , 0x00000000U},
    {REG_RTC_ZT_9103        , 1U, 0x1eU, 0x7fffU, 0x0116U, RTC_ZT_9103_ADDR       , 0x00000000U},
    {REG_WDT_CTRL           , 1U, 0x14U, 0x00eeU, 0x0068U, WDT_CTRL_ADDR          , 0x00000000U},
    {REG_WDT_HALT           , 1U, 0x18U, 0xffffU, 0x7e5aU, WDT_HALT_ADDR          , 0x00000000U},
    {REG_WDT_STBY           , 1U, 0x1aU, 0xffffU, 0xa538U, WDT_STBY_ADDR          , 0x00000000U},
};
#endif

#if defined(RN_CM0_PLATFORM) || defined(RN831x_RN861x_MCU_V1)
static const conf_info_t spl_ra9103e_conf[20] = {
    {REG_ANA_ADC            , 0U, 0x10U, 0x7c00U, 0x3400U, ANA_ADC_ADDR           , 0xa5770000U},
    {REG_ANA_CLK1           , 0U, 0x12U, 0x79ffU, 0x0060U, ANA_CLK1_ADDR          , 0xa5270000U},
    {REG_ANA_CLK2           , 0U, 0x14U, 0x1f00U, 0x0900U, ANA_CLK2_ADDR          , 0xa5360000U},
    {REG_ANA_LDO            , 0U, 0x16U, 0x7fffU, 0x29ffU, ANA_LDO_9103_ADDR      , 0xa5580000U},
    {REG_ANA_PSW            , 0U, 0x1aU, 0x0088U, 0x0000U, ANA_PSW_ADDR           , 0xa5790000U},
    {REG_ANA_SYS            , 0U, 0x2aU, 0xffffU, 0x2c22U, ANA_CLK3_ADDR          , 0xa5750000U},
    {REG_MADC_TPSCTRL_9103  , 0U, 0x20U, 0x1fffU, 0x083bU, MADC_TPSCTRL_9103_ADDR , 0x0000e000U},
    {REG_MADC_ADTEST_9103   , 0U, 0x22U, 0x0fffU, 0x03e8U, MADC_ADTEST_9103_ADDR  , 0x0000a000U},
    {REG_MADC_ANABGR_9103   , 0U, 0x54U, 0xfff3U, 0x6150U, MADC_ANABGR_9103_ADDR  , 0x00010000U},
    {REG_MADC_ANALDO_9103   , 0U, 0x58U, 0x000fU, 0x0004U, MADC_ANALDO_9103_ADDR  , 0x00010000U},
    {REG_MADC_ANARCH_9103   , 0U, 0x5aU, 0x3fffU, 0x0300U, MADC_ANARCH_9103_ADDR  , 0x00010000U},
    {REG_MADC_SARCTRL2_9103 , 0U, 0x5cU, 0x001fU, 0x0003U, MADC_SARCTRL2_9103_ADDR, 0x0000e000U},
    {REG_RTC_MODE           , 1U, 0x08U, 0x0001U, 0x0001U, RTC_MODE_ADDR          , 0x00000000U},
    {REG_RTC_BETA           , 1U, 0x12U, 0x03ffU, 0x00ffU, RTC_BETA_ADDR          , 0x00000000U},
    {REG_RTC_TADJ_9103      , 1U, 0x1cU, 0x07ffU, 0x0680U, RTC_TADJ_9103_ADDR     , 0x00000000U},
    {REG_RTC_ZT_9103        , 1U, 0x1eU, 0x7fffU, 0x0116U, RTC_ZT_9103_ADDR       , 0x00000000U},
    {REG_LOSC_CFG2          , 0U, 0x52U, 0xffffU, 0x6428U, LOSC_CFG2_ADDR         , 0x00010000U},
    {REG_WDT_CTRL           , 1U, 0x14U, 0x00eeU, 0x0068U, WDT_CTRL_ADDR          , 0x00000000U},
    {REG_WDT_HALT           , 1U, 0x18U, 0xffffU, 0x7e5aU, WDT_HALT_ADDR          , 0x00000000U},
    {REG_WDT_STBY           , 1U, 0x1aU, 0xffffU, 0xa538U, WDT_STBY_ADDR          , 0x00000000U},
};

static const conf_info_t spl_ra9103f_conf[21] = {
    {REG_ANA_ADC            , 0U, 0x10U, 0x7c00U, 0x3400U, ANA_ADC_ADDR           , 0xa5770000U},
    {REG_ANA_CLK1           , 0U, 0x12U, 0x79ffU, 0x0060U, ANA_CLK1_ADDR          , 0xa5270000U},
    {REG_ANA_CLK2           , 0U, 0x14U, 0x1f00U, 0x0900U, ANA_CLK2_ADDR          , 0xa5360000U},
    {REG_ANA_LDO            , 0U, 0x16U, 0x7fffU, 0x29ffU, ANA_LDO_9103_ADDR      , 0xa5580000U},
    {REG_ANA_PSW            , 0U, 0x1aU, 0x0088U, 0x0000U, ANA_PSW_ADDR           , 0xa5790000U},
    {REG_ANA_CLK3           , 0U, 0x28U, 0x0380U, 0x0080U, ANA_CLK3_ADDR          , 0xa5750000U},
    {REG_ANA_SYS            , 0U, 0x2aU, 0xffffU, 0x2c22U, ANA_SYS_ADDR           , 0xa5370000U},
    {REG_MADC_TPSCTRL_9103  , 0U, 0x20U, 0x1fffU, 0x083bU, MADC_TPSCTRL_9103_ADDR , 0x0000e000U},
    {REG_MADC_ADTEST_9103   , 0U, 0x22U, 0x0fffU, 0x03e8U, MADC_ADTEST_9103_ADDR  , 0x0000a000U},
    {REG_MADC_ANABGR_9103   , 0U, 0x54U, 0xfff3U, 0x6150U, MADC_ANABGR_9103_ADDR  , 0x00010000U},
    {REG_MADC_ANALDO_9103   , 0U, 0x58U, 0x000fU, 0x0004U, MADC_ANALDO_9103_ADDR  , 0x00010000U},
    {REG_MADC_ANARCH_9103   , 0U, 0x5aU, 0x3fffU, 0x0300U, MADC_ANARCH_9103_ADDR  , 0x00010000U},
    {REG_MADC_SARCTRL2_9103 , 0U, 0x5cU, 0x001fU, 0x0003U, MADC_SARCTRL2_9103_ADDR, 0x0000e000U},
    {REG_RTC_MODE           , 1U, 0x08U, 0x0001U, 0x0001U, RTC_MODE_ADDR          , 0x00000000U},
    {REG_RTC_BETA           , 1U, 0x12U, 0x03ffU, 0x00ffU, RTC_BETA_ADDR          , 0x00000000U},
    {REG_RTC_TADJ_9103      , 1U, 0x1cU, 0x07ffU, 0x0680U, RTC_TADJ_9103_ADDR     , 0x00000000U},
    {REG_RTC_ZT_9103        , 1U, 0x1eU, 0x7fffU, 0x0116U, RTC_ZT_9103_ADDR       , 0x00000000U},
    {REG_LOSC_CFG2          , 0U, 0x52U, 0xffffU, 0x6428U, LOSC_CFG2_ADDR         , 0x00010000U},
    {REG_WDT_CTRL           , 1U, 0x14U, 0x00eeU, 0x0068U, WDT_CTRL_ADDR          , 0x00000000U},
    {REG_WDT_HALT           , 1U, 0x18U, 0xffffU, 0x7e5aU, WDT_HALT_ADDR          , 0x00000000U},
    {REG_WDT_STBY           , 1U, 0x1aU, 0xffffU, 0xa538U, WDT_STBY_ADDR          , 0x00000000U},
};
#endif

#if defined(RN_CM0_PLATFORM) || defined(RN821x_RN721x_SOC_D)
static const conf_info_t spl_ra9103g_conf[17] = {
    {REG_ANA_ADC            , 0U, 0x10U, 0x7fffU, 0x3400U, ANA_ADC_ADDR           , 0xa5770000U},
    {REG_ANA_CLK1           , 0U, 0x12U, 0x79ffU, 0x0060U, ANA_CLK1_ADDR          , 0xa5270000U},
    {REG_ANA_CLK2           , 0U, 0x14U, 0x1fffU, 0x0914U, ANA_CLK2_ADDR          , 0xa5360000U},
    {REG_ANA_LDO            , 0U, 0x16U, 0x3fffU, 0x145fU, ANA_LDO_9103_ADDR      , 0xa5580000U},
    {REG_ANA_BGR            , 0U, 0x18U, 0xffffU, 0x612aU, ANA_BGR_ADDR           , 0xa5280000U},
    {REG_ANA_PSW            , 0U, 0x1aU, 0x3088U, 0x0000U, ANA_PSW_ADDR           , 0xa5790000U},
    {REG_ANA_PWR            , 0U, 0x26U, 0x7fffU, 0x7000U, ANA_PWR_ADDR           , 0xa5430000U},
    {REG_ANA_CLK3           , 0U, 0x28U, 0xffffU, 0x3ce4U, ANA_CLK3_ADDR          , 0xa5750000U},
    {REG_ANA_SYS            , 0U, 0x2aU, 0x0007U, 0x0007U, ANA_SYS_ADDR           , 0xa5370000U},
    {REG_MADC_TPSCTRL_9103  , 0U, 0x20U, 0x1fffU, 0x193bU, MADC_TPSCTRL_9103_ADDR , 0x0000e000U},
    {REG_MADC_ADTEST_9103   , 0U, 0x22U, 0x003fU, 0x0028U, MADC_ADTEST_9103_ADDR  , 0x0000a000U},
    {REG_RTC_MODE           , 1U, 0x08U, 0x0001U, 0x0001U, RTC_MODE_ADDR          , 0x00000000U},
    {REG_RTC_TADJ_9103      , 1U, 0x1cU, 0x07ffU, 0x0680U, RTC_TADJ_9103_ADDR     , 0x00000000U},
    {REG_RTC_ZT_9103        , 1U, 0x1eU, 0x7fffU, 0x0116U, RTC_ZT_9103_ADDR       , 0x00000000U},
    {REG_WDT_CTRL           , 1U, 0x14U, 0x00eeU, 0x0068U, WDT_CTRL_ADDR          , 0x00000000U},
    {REG_WDT_HALT           , 1U, 0x18U, 0xffffU, 0x7e5aU, WDT_HALT_ADDR          , 0x00000000U},
    {REG_WDT_STBY           , 1U, 0x1aU, 0xffffU, 0xa538U, WDT_STBY_ADDR          , 0x00000000U},
};
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2)
static const conf_info_t spl_ra9701a_conf[] = {
    {REG_ANA_CLK1           , 0U, 0x12U, 0x07ffU, 0x0009U, ANA_CLK1_ADDR          , 0xa5270000U},
    {REG_ANA_LDO            , 0U, 0x16U, 0xff00U, 0x8800U, ANA_LDO_9103_ADDR      , 0xa5580000U},
    {REG_ANA_SYS            , 0U, 0x20U, 0x07ffU, 0x041cU, ANA_SYS_ADDR           , 0xa5370000U},
    {REG_ANA_ULBGR1         , 0U, 0x22U, 0x7fffU, 0x5388U, ANA_ULBGR1_ADDR        , 0xa5380000U},
    {REG_MADC_TPSCTRL_9701  , 0U, 0x44U, 0x1fffU, 0x083bU, MADC_TPSCTRL_9701_ADDR , 0x0000e000U},
    {REG_MADC_ADTEST_9701   , 0U, 0x42U, 0x7fffU, 0x5b58U, MADC_ADTEST_9701_ADDR  , 0x00010000U},
    {REG_MADC_ANABGR_9701   , 0U, 0x46U, 0xfff1U, 0x8200U, MADC_ANABGR_9701_ADDR  , 0x00010000U},
    {REG_MADC_ANALDO_9701   , 0U, 0x48U, 0x000fU, 0x0004U, MADC_ANALDO_9701_ADDR  , 0x00010000U},
    {REG_MADC_ANARCH_9701   , 0U, 0x4aU, 0x3fffU, 0x0200U, MADC_ANARCH_9701_ADDR  , 0x00010000U},
    {REG_MADC_SARCTRL2_9701 , 0U, 0x40U, 0x7fffU, 0x2904U, MADC_SARCTRL2_9701_ADDR, 0x00010000U},
    {REG_RTC_MODE           , 0U, 0x50U, 0x0001U, 0x0001U, RTC_MODE_ADDR          , 0x00000000U},
    {REG_RTC_TADJ_9103      , 0U, 0x4eU, 0x07ffU, 0x0664U, RTC_TADJ_9103_ADDR     , 0x00000000U},
    {REG_RTC_ZT_9103        , 0U, 0x52U, 0x7fffU, 0x0d11U, RTC_ZT_9103_ADDR       , 0x00000000U},
    {REG_LOSC_CFG2          , 0U, 0x4cU, 0xffffU, 0x6118U, LOSC_CFG2_ADDR         , 0x00010000U},
    {REG_TRNG_CTRL_9701     , 0U, 0x54U, 0xffffU, 0x00e4U, TRNG_CTRL_9701_ADDR    , 0x00000000U},
};

static const conf_info_t spl_ra9701a_conf_rtc[] = {
    {REG_RTC_ALPHAL         , 1U, 0x0cU, 0x07ffU, 0x03eeU, RTC_ALPHAL_ADDR        , 0x00000000U},
    {REG_RTC_ALPHAH         , 1U, 0x0eU, 0x07ffU, 0x04cfU, RTC_ALPHAH_ADDR        , 0x00000000U},
    {REG_RTC_XT0            , 1U, 0x10U, 0x007fU, 0x0064U, RTC_XT0_ADDR           , 0x00000000U},
};

static const conf_info_t spl_ra9701a_conf_wdt[] = {
    {REG_WDT_CTRL           , 1U, 0x14U, 0x00eeU, 0x0068U, WDT_CTRL_ADDR          , 0x00000000U},
    {REG_WDT_HALT           , 1U, 0x18U, 0xffffU, 0x7e5aU, WDT_HALT_ADDR          , 0x00000000U},
    {REG_WDT_STBY           , 1U, 0x1aU, 0xffffU, 0xa538U, WDT_STBY_ADDR          , 0x00000000U},
};

static const conf_info_t spl_ra9701a_conf_rch[] = {
    {REG_ANA_CLK3           , 0U, 0x1eU, 0x03ffU, 0x0216U, ANA_CLK3_ADDR          , 0xa5750000U},
};

static const conf_info_t spl_ra9701a_conf_rcl[] = {
    {REG_ANA_CLK2           , 0U, 0x14U, 0x3fffU, 0x064cU, ANA_CLK2_ADDR          , 0xa5360000U},
};

#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V3)
static const conf_info_t spl_ra9701b_conf[] = {
    {REG_ANA_CLK1           , 0U, 0x12U, 0x07ffU, 0x0009U, ANA_CLK1_ADDR          , 0xa5270000U},
    {REG_ANA_LDO            , 0U, 0x16U, 0xff00U, 0x8800U, ANA_LDO_9103_ADDR      , 0xa5580000U},
    {REG_ANA_SYS            , 0U, 0x20U, 0x07ffU, 0x041cU, ANA_SYS_ADDR           , 0xa5370000U},
    {REG_ANA_ULBGR1         , 0U, 0x22U, 0x7fffU, 0x5388U, ANA_ULBGR1_ADDR        , 0xa5380000U},
    {REG_ANA_LURLS_CFG_9701 , 0U, 0x26U, 0x0003U, 0x0000U, ANA_SADDR_9701_ADDR    , 0xa5470000U},
    {REG_ANA_LURLS_EN_9701  , 0U, 0x28U, 0x00ffU, 0x00c5U, ANA_SADDR_9701_ADDR    , 0xa5470000U},
    {REG_MADC_TPSCTRL_9701  , 0U, 0x44U, 0x1fffU, 0x083bU, MADC_TPSCTRL_9701_ADDR , 0x0000e000U},
    {REG_MADC_ADTEST_9701   , 0U, 0x42U, 0x7fffU, 0x5b58U, MADC_ADTEST_9701_ADDR  , 0x00010000U},
    {REG_MADC_ANABGR_9701   , 0U, 0x46U, 0xfff1U, 0x8200U, MADC_ANABGR_9701_ADDR  , 0x00010000U},
    {REG_MADC_ANALDO_9701   , 0U, 0x48U, 0x000fU, 0x0004U, MADC_ANALDO_9701_ADDR  , 0x00010000U},
    {REG_MADC_ANARCH_9701   , 0U, 0x4aU, 0x3fffU, 0x0200U, MADC_ANARCH_9701_ADDR  , 0x00010000U},
    {REG_MADC_SARCTRL2_9701 , 0U, 0x40U, 0x7fffU, 0x2904U, MADC_SARCTRL2_9701_ADDR, 0x00010000U},
    {REG_RTC_MODE           , 0U, 0x50U, 0x0001U, 0x0001U, RTC_MODE_ADDR          , 0x00000000U},
    {REG_RTC_TADJ_9103      , 0U, 0x4eU, 0x07ffU, 0x0664U, RTC_TADJ_9103_ADDR     , 0x00000000U},
    {REG_RTC_ZT_9103        , 0U, 0x52U, 0x7fffU, 0x0d11U, RTC_ZT_9103_ADDR       , 0x00000000U},
    {REG_LOSC_CFG2          , 0U, 0x4cU, 0xffffU, 0x6118U, LOSC_CFG2_ADDR         , 0x00010000U}
};
static const conf_info_t spl_ra9701b_conf_rtc[] = {
    {REG_RTC_ALPHAL         , 1U, 0x0cU, 0x07ffU, 0x03eeU, RTC_ALPHAL_ADDR        , 0x00000000U},
    {REG_RTC_ALPHAH         , 1U, 0x0eU, 0x07ffU, 0x04cfU, RTC_ALPHAH_ADDR        , 0x00000000U},
    {REG_RTC_XT0            , 1U, 0x10U, 0x007fU, 0x0064U, RTC_XT0_ADDR           , 0x00000000U},
};
static const conf_info_t spl_ra9701b_conf_wdt[] = {
    {REG_WDT_CTRL           , 1U, 0x14U, 0x00eeU, 0x0068U, WDT_CTRL_ADDR          , 0x00000000U},
    {REG_WDT_HALT           , 1U, 0x18U, 0xffffU, 0x7e5aU, WDT_HALT_ADDR          , 0x00000000U},
    {REG_WDT_STBY           , 1U, 0x1aU, 0xffffU, 0xa538U, WDT_STBY_ADDR          , 0x00000000U}
};
static const conf_info_t spl_ra9701b_conf_rch[] = {
    {REG_ANA_CLK3           , 0U, 0x1eU, 0x03ffU, 0x0216U, ANA_CLK3_ADDR          , 0xa5750000U},
};
static const conf_info_t spl_ra9701b_conf_rcl[] = {
    {REG_ANA_CLK2           , 0U, 0x14U, 0x3fffU, 0x064cU, ANA_CLK2_ADDR          , 0xa5360000U},
};
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_V2)
static const conf_info_t spl_ra9702a_conf[] = {
    {REG_ANA_ADC            , 0U, 0x10U, 0x7fffU, 0x0000U, ANA_ADC_ADDR           , 0xa5770000U},
    {REG_ANA_CLK1           , 0U, 0x12U, 0x07ffU, 0x0009U, ANA_CLK1_ADDR          , 0xa5270000U},
    {REG_ANA_LDO            , 0U, 0x16U, 0xff00U, 0x8800U, ANA_LDO_9103_ADDR      , 0xa5580000U},
    {REG_ANA_BGR            , 0U, 0x18U, 0xffffU, 0x612aU, ANA_BGR_ADDR           , 0xa5280000U},
    {REG_ANA_PSW            , 0U, 0x1aU, 0xffffU, 0x6428U, ANA_PSW_ADDR           , 0xa5790000U},
    {REG_ANA_PWR            , 0U, 0x1cU, 0x0fffU, 0x0000U, ANA_PWR_ADDR           , 0xa5430000U},
    {REG_ANA_SYS            , 0U, 0x20U, 0x7fffU, 0x540cU, ANA_SYS_ADDR           , 0xa5370000U},
    {REG_ANA_ULBGR1         , 0U, 0x22U, 0x7fffU, 0x3d88U, ANA_ULBGR1_ADDR        , 0xa5380000U},
    {REG_ANA_PWS            , 0U, 0x94U, 0x01ffU, 0x0034U, ANA_PWS_ADDR           , 0xa59a0000U},
    {REG_ANA_PSM            , 0U, 0x96U, 0x003fU, 0x0000U, ANA_PSM_ADDR           , 0xa5980000U},
    {REG_ANA_ADC2           , 0U, 0x98U, 0x03ffU, 0x0000U, ANA_ADC2_ADDR          , 0xa5770000U},
    {REG_ANA_9702_BGR2      , 0U, 0x9aU, 0xffffU, 0x8202U, ANA_BGR2_9702_ADDR     , 0xa5880000U},
    {REG_ANA_PWR2           , 0U, 0x9cU, 0x0fffU, 0x0faaU, ANA_PWR2_ADDR          , 0xa5430000U},
    {REG_ANA_PSM2           , 0U, 0x9eU, 0x003fU, 0x0030U, ANA_PSM2_ADDR          , 0xa5980000U},
    {REG_ANA_CKTIM_9702     , 0U, 0xa0U, 0x0fffU, 0x0A26U, ANA_CKTIM_9702_ADDR    , 0xc5790000U},
    {REG_ANA_CKTIM2         , 0U, 0xa2U, 0x0fffU, 0x0912U, ANA_CKTIM2_ADDR        , 0xc5770000U},
    {REG_ANA_LURLS_CFG_9701 , 0U, 0x26U, 0x0003U, 0x0000U, ANA_SADDR_9701_ADDR    , 0xa5470000U},
    {REG_ANA_LURLS_EN_9701  , 0U, 0x28U, 0x00ffU, 0x00c5U, ANA_SADDR_9701_ADDR    , 0xa5470000U},
    {REG_MADC_TPSCTRL_9701  , 0U, 0x44U, 0x1fffU, 0x193BU, MADC_TPSCTRL_9701_ADDR , 0x0000e000U},
    {REG_MADC_ADTEST_9701   , 0U, 0x42U, 0x0fffU, 0x0B58U, MADC_ADTEST_9701_ADDR  , 0x00010000U},
    {REG_MADC_SARCTRL2_9701 , 0U, 0x40U, 0x03ffU, 0x01FFU, MADC_SARCTRL2_9701_ADDR, 0x00010000U},
    {REG_MADC_TPSCTRL2      , 0U, 0xa6U, 0x1fffU, 0x0148U, MADC_TPSCTRL2_9702_ADDR, 0x00010000U},
    {REG_MADC_TPSCTRL3      , 0U, 0xa8U, 0x1fffU, 0x0000U, MADC_TPSCTRL3_9702_ADDR, 0x00010000U},
    {REG_RTC_MODE           , 0U, 0x50U, 0x0001U, 0x0001U, RTC_MODE_ADDR          , 0x00000000U},
    {REG_RTC_TADJ_9103      , 0U, 0x4eU, 0x07ffU, 0x0664U, RTC_TADJ_9103_ADDR     , 0x00000000U},
    {REG_RTC_TADJ_9702      , 0U, 0xaaU, 0x07ffU, 0x0664U, RTC_TADJ_9702_ADDR     , 0x00000000U},
    {REG_RTC_ZT_9103        , 0U, 0x52U, 0x7fffU, 0x0d11U, RTC_ZT_9103_ADDR       , 0x00000000U},
    {REG_RTC_ZT_9702        , 0U, 0xacU, 0x7fffU, 0x0d11U, RTC_ZT_9702_ADDR       , 0x00000000U},
    {REG_TRNG_CTRL_9701     , 0U, 0x54U, 0xffffU, 0x00e4U, TRNG_CTRL_9701_ADDR    , 0x00000000U},
};
static const conf_info_t spl_ra9702a_conf_rtc[] = {
    {REG_RTC_ALPHAL         , 1U, 0x0cU, 0x07ffU, 0x03eeU, RTC_ALPHAL_ADDR        , 0x00000000U},
    {REG_RTC_ALPHAH         , 1U, 0x0eU, 0x07ffU, 0x04cfU, RTC_ALPHAH_ADDR        , 0x00000000U},
    {REG_RTC_XT0            , 1U, 0x10U, 0x007fU, 0x0064U, RTC_XT0_ADDR           , 0x00000000U},
};
static const conf_info_t spl_ra9702a_conf_wdt[] = {
    {REG_WDT_CTRL           , 1U, 0x14U, 0x00eeU, 0x0068U, WDT_CTRL_ADDR          , 0x00000000U},
    {REG_WDT_HALT           , 1U, 0x18U, 0xffffU, 0x7e5aU, WDT_HALT_ADDR          , 0x00000000U},
    {REG_WDT_STBY           , 1U, 0x1aU, 0xffffU, 0xa538U, WDT_STBY_ADDR          , 0x00000000U},
};
static const conf_info_t spl_ra9702a_conf_rch[] = {
    {REG_ANA_CLK3           , 0U, 0x1eU, 0x7fffU, 0x2216U, ANA_CLK3_ADDR          , 0xa5750000U},
};
static const conf_info_t spl_ra9702a_conf_rcl[] = {
    {REG_ANA_CLK2           , 0U, 0x14U, 0x3fffU, 0x064cU, ANA_CLK2_ADDR          , 0xa5360000U},
};
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_V3)
static const conf_info_t spl_ra9702b_conf[] = {
    {REG_ANA_ADC            , 0U, 0x10U, 0x7fffU, 0x0000U, ANA_ADC_ADDR            , 0xa5770000U},
    {REG_ANA_CLK1           , 0U, 0x12U, 0x07ffU, 0x0009U, ANA_CLK1_ADDR           , 0xa5270000U},
    {REG_ANA_LDO            , 0U, 0x16U, 0xff00U, 0x8800U, ANA_LDO_9103_ADDR       , 0xa5580000U},
    {REG_ANA_BGR            , 0U, 0x18U, 0xffffU, 0x612aU, ANA_BGR_ADDR            , 0xa5280000U},
    {REG_ANA_PSW            , 0U, 0x1aU, 0xffffU, 0x6428U, ANA_PSW_ADDR            , 0xa5790000U},
    {REG_ANA_PWR            , 0U, 0x1cU, 0x0fffU, 0x0000U, ANA_PWR_ADDR            , 0xa5430000U},
    {REG_ANA_SYS            , 0U, 0x20U, 0x7fffU, 0x540cU, ANA_SYS_ADDR            , 0xa5370000U},
    {REG_ANA_ULBGR1         , 0U, 0x22U, 0x7f00U, 0x3d00U, ANA_ULBGR1_ADDR         , 0xa5380000U},
    {REG_ANA_PWS            , 0U, 0x94U, 0x01ffU, 0x0034U, ANA_PWS_ADDR            , 0xa59a0000U},
    {REG_ANA_PSM            , 0U, 0x96U, 0x003fU, 0x0000U, ANA_PSM_ADDR            , 0xa5980000U},
    {REG_ANA_ADC2           , 0U, 0x98U, 0x03ffU, 0x0000U, ANA_ADC2_ADDR           , 0xa5770000U},
    {REG_ANA_9702_BGR2      , 0U, 0x9aU, 0xffffU, 0x8202U, ANA_BGR2_9702_ADDR      , 0xa5880000U},
    {REG_ANA_PWR2           , 0U, 0x9cU, 0x0fffU, 0x0faaU, ANA_PWR2_ADDR           , 0xa5430000U},
    {REG_ANA_PSM2           , 0U, 0x9eU, 0x003fU, 0x0030U, ANA_PSM2_ADDR           , 0xa5980000U},
    {REG_ANA_CKTIM_9702     , 0U, 0xa0U, 0x0fffU, 0x0A26U, ANA_CKTIM_9702_ADDR     , 0xc5790000U},
    {REG_ANA_CKTIM2         , 0U, 0xa2U, 0x0fffU, 0x0912U, ANA_CKTIM2_ADDR         , 0xc5770000U},
    {REG_ANA_LURLS_CFG_9701 , 0U, 0x26U, 0x0003U, 0x0000U, ANA_SADDR_9701_ADDR     , 0xa5470000U},
    {REG_ANA_LURLS_EN_9701  , 0U, 0x28U, 0x00ffU, 0x00c5U, ANA_SADDR_9701_ADDR     , 0xa5470000U},
    {REG_MADC_TPSCTRL_9701  , 0U, 0x44U, 0x1fffU, 0x193BU, MADC_TPSCTRL_9701_ADDR  , 0x0000e000U},
    {REG_MADC_ADTEST_9701   , 0U, 0x42U, 0x0fffU, 0x0B58U, MADC_ADTEST_9701_ADDR   , 0x00010000U},
    {REG_MADC_SARCTRL2_9701 , 0U, 0x40U, 0x03ffU, 0x01FFU, MADC_SARCTRL2_9701_ADDR , 0x00010000U},
    {REG_MADC_TPSCTRL2      , 0U, 0xa6U, 0x1fffU, 0x0148U, MADC_TPSCTRL2_9702_ADDR , 0x00010000U},
    {REG_MADC_TPSCTRL3      , 0U, 0xa8U, 0x1fffU, 0x0000U, MADC_TPSCTRL3_9702_ADDR , 0x00010000U},
    {REG_RTC_MODE           , 0U, 0x50U, 0x0001U, 0x0001U, RTC_MODE_ADDR           , 0x00000000U},
    {REG_RTC_TADJ_9103      , 0U, 0x4eU, 0x07ffU, 0x0664U, RTC_TADJ_9103_ADDR      , 0x00000000U},
    {REG_RTC_TADJ_9702      , 0U, 0xaaU, 0x07ffU, 0x0664U, RTC_TADJ_9702_ADDR      , 0x00000000U},
    {REG_RTC_ZT_9103        , 0U, 0x52U, 0x7fffU, 0x0d11U, RTC_ZT_9103_ADDR        , 0x00000000U},
    {REG_RTC_ZT_9702        , 0U, 0xacU, 0x7fffU, 0x0d11U, RTC_ZT_9702_ADDR        , 0x00000000U},
    {REG_TRNG_CTRL_9702B    , 0U, 0x54U, 0xffffU, 0x00e4U, TRNG_CTRL_9702B_ADDR    , 0x00000000U},
};
static const conf_info_t spl_ra9702b_conf_rtc[] = {
    {REG_RTC_ALPHAL         , 1U, 0x0cU, 0x07ffU, 0x03eeU, RTC_ALPHAL_ADDR         , 0x00000000U},
    {REG_RTC_ALPHAH         , 1U, 0x0eU, 0x07ffU, 0x04cfU, RTC_ALPHAH_ADDR         , 0x00000000U},
    {REG_RTC_XT0            , 1U, 0x10U, 0x007fU, 0x0064U, RTC_XT0_ADDR            , 0x00000000U},
};
static const conf_info_t spl_ra9702b_conf_wdt[] = {
    {REG_WDT_CTRL           , 1U, 0x14U, 0x00eeU, 0x0068U, WDT_CTRL_ADDR           , 0x00000000U},
    {REG_WDT_HALT           , 1U, 0x18U, 0xffffU, 0x7e5aU, WDT_HALT_ADDR           , 0x00000000U},
    {REG_WDT_STBY           , 1U, 0x1aU, 0xffffU, 0xa538U, WDT_STBY_ADDR           , 0x00000000U},
};
static const conf_info_t spl_ra9702b_conf_rch[] = {
    {REG_ANA_CLK3           , 0U, 0x1eU, 0x7fffU, 0x2216U, ANA_CLK3_ADDR           , 0xa5750000U},
};
static const conf_info_t spl_ra9702b_conf_rcl[] = {
    {REG_ANA_CLK2           , 0U, 0x14U, 0x3fffU, 0x064cU, ANA_CLK2_ADDR           , 0xa5360000U},
};
#endif

#if defined(RN_CM0_PLATFORM) || defined(RN202x_RN7326_SOC_B)
static const conf_info_t spl_ra9303b_conf[24] = {
    {REG_ANA_ADC_9303        , 0U, 0x10U, 0x3fffU, 0x1a00U, ANA_ADC_9303_ADDR       , 0xa5770000U},
    {REG_ANA_CLK1_9303       , 0U, 0x12U, 0xf9ffU, 0x1860U, ANA_CLK1_9303_ADDR      , 0xa5270000U},
    {REG_ANA_LDO_9303        , 0U, 0x14U, 0x7f00U, 0x2903U, ANA_LDO_9303_ADDR       , 0xa5580000U},
    {REG_ANA_LDO33_9303      , 0U, 0x16U, 0x001fU, 0x000BU, ANA_LDO33_9303_ADDR     , 0x25230000U},
    {REG_ANA_BGR_9303        , 0U, 0x18U, 0xffffU, 0x6120U, ANA_BGR_9303_ADDR       , 0xa5280000U},
    {REG_ANA_BGR2_9303       , 0U, 0x1aU, 0xfff7U, 0x6120U, ANA_BGR2_9303_ADDR      , 0x45300000U},
    {REG_ANA_PSW_9303        , 0U, 0x1cU, 0x0080U, 0x0000U, ANA_PSW_9303_ADDR       , 0xa5790000U},
    {REG_ANA_PWR_9303        , 0U, 0x1eU, 0xffffU, 0x0000U, ANA_PWR_9303_ADDR       , 0xa5430000U},
    {REG_ANA_PWR2_9303       , 0U, 0x20U, 0x0fffU, 0x0000U, ANA_PWR2_9303_ADDR      , 0x65430000U},
    {REG_ANA_TCLDO15_9303    , 0U, 0x22U, 0x00ffU, 0x0077U, ANA_TCLDO15_9303_ADDR   , 0x85540000U},
    {REG_ANA_TCICC04_9303    , 0U, 0x24U, 0x03ffU, 0x0077U, ANA_TCICC04_9303_ADDR   , 0xa5670000U},
    {REG_ANA_CKTIM_9303      , 0U, 0x26U, 0xf777U, 0x2227U, ANA_CKTIM_9303_ADDR     , 0xc5790000U},
    {REG_ANA_SYS_9303        , 0U, 0x28U, 0x1c00U, 0x1C00U, ANA_SYS_9303_ADDR       , 0xa5370000U},
    {REG_ANA_RC32M_CTL2_9303 , 0U, 0x3cU, 0xffffU, 0x8080U, ANA_RC32M_CTL2_9303_ADDR, 0xa5320000U},
    {REG_MADC_TPSCTRL_9303   , 0U, 0x30U, 0x1fffU, 0x083BU, MADC_TPSCTRL_9303_ADDR  , 0x0000e000U},
    {REG_MADC_ADTEST_9303    , 0U, 0x2eU, 0x07ecU, 0x03E8U, MADC_ADTEST_9303_ADDR   , 0x0000a000U},
    {REG_MADC_ANABGR_9303    , 0U, 0x32U, 0xfff1U, 0x6120U, MADC_ANABGR_9303_ADDR   , 0x00010000U},
    {REG_MADC_ANALDO_9303    , 0U, 0x34U, 0x000fU, 0x0004U, MADC_ANALDO_9303_ADDR   , 0x00010000U},
    {REG_MADC_ANARCH_9303    , 0U, 0x36U, 0x3fffU, 0x0300U, MADC_ANARCH_9303_ADDR   , 0x00010000U},
    {REG_MADC_SARCTRL2_9303  , 0U, 0x2cU, 0x001fU, 0x0003U, MADC_SARCTRL2_9303_ADDR , 0x00010000U},
    {REG_RTC_MODE            , 0U, 0x8aU, 0x0001U, 0x0001U, RTC_MODE_ADDR           , 0x00000000U},
    {REG_RTC_TADJ_9103       , 0U, 0x8cU, 0x07ffU, 0x0680U, RTC_TADJ_9103_ADDR      , 0x00000000U},
    {REG_RTC_ZT_9103         , 0U, 0x8eU, 0x7fffU, 0x0116U, RTC_ZT_9103_ADDR        , 0x00000000U},
    {REG_LOSC_CFG2           , 0U, 0x38U, 0xffffU, 0x6428U, LOSC_CFG2_ADDR          , 0x00010000U}
};
#endif

#if defined(RN_CM0_PLATFORM) || defined(RN202x_RN7326_SOC_V2)
static const conf_info_t spl_ra9703a_conf[] = {
    {REG_ANA_ADC            , 0U, 0x10U, 0x7fffU, 0x0000U, ANA_ADC_ADDR           , 0xa5770000U},
    {REG_ANA_CLK1           , 0U, 0x12U, 0x013fU, 0x0000U, ANA_CLK1_ADDR          , 0xa5270000U},
    {REG_ANA_LDO            , 0U, 0x16U, 0xff00U, 0x8800U, ANA_LDO_9103_ADDR      , 0xa5580000U},
    {REG_ANA_BGR            , 0U, 0x18U, 0xffffU, 0x7e68U, ANA_BGR_ADDR           , 0xa5280000U},
    {REG_ANA_PWR            , 0U, 0x1cU, 0x7fffU, 0x0000U, ANA_PWR_ADDR           , 0xa5430000U},
    {REG_ANA_SYS            , 0U, 0x20U, 0x7fffU, 0x540cU, ANA_SYS_ADDR           , 0xa5370000U},
    {REG_ANA_ULBGR1         , 0U, 0x22U, 0x7fffU, 0x3d88U, ANA_ULBGR1_ADDR        , 0xa5380000U},
    {REG_ANA_9703_BGR2      , 0U, 0x9aU, 0x0001U, 0x0000U, ANA_BGR2_9702_ADDR     , 0xa5280000U},
    {REG_ANA_CKTIM_9703     , 0U, 0xa0U, 0x2227U, 0x7fffU, ANA_CKTIM_9703_ADDR    , 0xc5790000U},
    {REG_ANA_LURLS_CFG_9703 , 0U, 0x26U, 0x0003U, 0x0000U, ANA_SADDR_9703_ADDR    , 0xa5470000U},
    {REG_ANA_LURLS_EN_9703  , 0U, 0x28U, 0x00ffU, 0x00c5U, ANA_SADDR_9703_ADDR    , 0xa5470000U},
    {REG_MADC_TPSCTRL_9701  , 0U, 0x44U, 0x1fffU, 0x193bU, MADC_TPSCTRL_9701_ADDR , 0x0000e000U},
    {REG_MADC_ADTEST_9701   , 0U, 0x42U, 0x7fffU, 0x5b78U, MADC_ADTEST_9701_ADDR  , 0x00010000U},
    {REG_MADC_ANABGR_9701   , 0U, 0x46U, 0xfff1U, 0x81e0U, MADC_ANABGR_9701_ADDR  , 0x00010000U},
    {REG_MADC_ANALDO_9701   , 0U, 0x48U, 0x000fU, 0x0004U, MADC_ANALDO_9701_ADDR  , 0x00010000U},
    {REG_MADC_ANARCH_9701   , 0U, 0x4aU, 0x3fffU, 0x0180U, MADC_ANARCH_9701_ADDR  , 0x00010000U},
    {REG_MADC_SARCTRL2_9701 , 0U, 0x40U, 0x7fffU, 0x2904U, MADC_SARCTRL2_9701_ADDR, 0x00010000U},
    {REG_RTC_MODE           , 0U, 0x50U, 0x0001U, 0x0001U, RTC_MODE_ADDR          , 0x00000000U},
    {REG_RTC_TADJ_9103      , 0U, 0x4eU, 0x07ffU, 0x0664U, RTC_TADJ_9103_ADDR     , 0x00000000U},
    {REG_RTC_ZT_9103        , 0U, 0x52U, 0x7fffU, 0x0d11U, RTC_ZT_9103_ADDR       , 0x00000000U},
    {REG_LOSC_CFG2          , 0U, 0x4cU, 0xffffU, 0x6118U, LOSC_CFG2_ADDR         , 0x00010000U},
    {REG_TRNG_CTRL_9703     , 0U, 0x54U, 0xffffU, 0x00e4U, TRNG_CTRL_9703_ADDR    , 0x00000000U},
};
static const conf_info_t spl_ra9703a_conf_rtc[] = {
    {REG_RTC_ALPHAL         , 1U, 0x0cU, 0x07ffU, 0x03eeU, RTC_ALPHAL_ADDR        , 0x00000000U},
    {REG_RTC_ALPHAH         , 1U, 0x0eU, 0x07ffU, 0x04cfU, RTC_ALPHAH_ADDR        , 0x00000000U},
    {REG_RTC_XT0            , 1U, 0x10U, 0x007fU, 0x0064U, RTC_XT0_ADDR           , 0x00000000U},
};
static const conf_info_t spl_ra9703a_conf_wdt[] = {
    {REG_WDT_CTRL           , 1U, 0x14U, 0x00eeU, 0x0068U, WDT_CTRL_ADDR          , 0x00000000U},
    {REG_WDT_HALT           , 1U, 0x18U, 0xffffU, 0x7e5aU, WDT_HALT_ADDR          , 0x00000000U},
    {REG_WDT_STBY           , 1U, 0x1aU, 0xffffU, 0xa538U, WDT_STBY_ADDR          , 0x00000000U},
};
static const conf_info_t spl_ra9703a_conf_rch[] = {
    {REG_ANA_CLK3           , 0U, 0x1eU, 0x03ffU, 0x0216U, ANA_CLK3_ADDR           , 0xa5750000},
};
static const conf_info_t spl_ra9703a_conf_rcl[] = {
    {REG_ANA_CLK2           , 0U, 0x14U, 0x3fffU, 0x0680U, ANA_CLK2_ADDR          , 0xa5360000U},
};
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
#if defined(RN_CM0_PLATFORM)
static void get_conf_reg(unsigned char cfg_type, rn_chipid_t chipid, const conf_info_t ** spl_conf, unsigned int *spl_conf_len)
#else
static void get_conf_reg(unsigned char cfg_type, const conf_info_t ** spl_conf, unsigned int *spl_conf_len)
#endif
{
#if defined(RN_CM0_PLATFORM)
    if (CHIP_RA9701A == chipid)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2)
    {
        if (cfg_type == 0) {
            *spl_conf = spl_ra9701a_conf;
            *spl_conf_len = sizeof(spl_ra9701a_conf) / sizeof(conf_info_t);
        } else if (cfg_type == 1) {
            *spl_conf = spl_ra9701a_conf_rtc;
            *spl_conf_len = 3U;
        } else if (cfg_type == 2) {
            *spl_conf = spl_ra9701a_conf_wdt;
            *spl_conf_len = 3U;
        } else if (cfg_type == 3) {
            *spl_conf = spl_ra9701a_conf_rch;
            *spl_conf_len = 1U;
        } else/* if (cfg_type == 4)*/ {
            *spl_conf = spl_ra9701a_conf_rcl;
            *spl_conf_len = 1U;
        }
    }
#endif
#if defined(RN_CM0_PLATFORM)
    else if (CHIP_RA9701B == chipid)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V3)
    {
        if (cfg_type == 0) {
            *spl_conf = spl_ra9701b_conf;
            *spl_conf_len = sizeof(spl_ra9701b_conf) / sizeof(conf_info_t);
        } else if (cfg_type == 1) {
            *spl_conf = spl_ra9701b_conf_rtc;
            *spl_conf_len = 3U;
        } else if (cfg_type == 2) {
            *spl_conf = spl_ra9701b_conf_wdt;
            *spl_conf_len = 3U;
        } else if (cfg_type == 3) {
            *spl_conf = spl_ra9701b_conf_rch;
            *spl_conf_len = 1U;
        } else/* if (cfg_type == 4)*/ {
            *spl_conf = spl_ra9701b_conf_rcl;
            *spl_conf_len = 1U;
        }
    }
#endif
#if defined(RN_CM0_PLATFORM)
    else if (CHIP_RA9702A == chipid)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_V2)
    {
        if (cfg_type == 0) {
            *spl_conf = spl_ra9702a_conf;
            *spl_conf_len = sizeof(spl_ra9702a_conf) / sizeof(conf_info_t);
        } else if (cfg_type == 1) {
            *spl_conf = spl_ra9702a_conf_rtc;
            *spl_conf_len = 3U;
        } else if (cfg_type == 2) {
            *spl_conf = spl_ra9702a_conf_wdt;
            *spl_conf_len = 3U;
        } else if (cfg_type == 3) {
            *spl_conf = spl_ra9702a_conf_rch;
            *spl_conf_len = 1U;
        } else/* if (cfg_type == 4)*/ {
            *spl_conf = spl_ra9702a_conf_rcl;
            *spl_conf_len = 1U;
        }
    }
#endif
#if defined(RN_CM0_PLATFORM)
    else if (CHIP_RA9702B == chipid)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_V3)
    {
        if (cfg_type == 0) {
            *spl_conf = spl_ra9702b_conf;
            *spl_conf_len = sizeof(spl_ra9702b_conf) / sizeof(conf_info_t);
        } else if (cfg_type == 1) {
            *spl_conf = spl_ra9702b_conf_rtc;
            *spl_conf_len = 3U;
        } else if (cfg_type == 2) {
            *spl_conf = spl_ra9702b_conf_wdt;
            *spl_conf_len = 3U;
        } else if (cfg_type == 3) {
            *spl_conf = spl_ra9702b_conf_rch;
            *spl_conf_len = 1U;
        } else/* if (cfg_type == 4)*/ {
            *spl_conf = spl_ra9702b_conf_rcl;
            *spl_conf_len = 1U;
        }
    }
#endif
#if defined(RN_CM0_PLATFORM)
    else if (CHIP_RA9703A == chipid)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2)
    {
        if (cfg_type == 0) {
            *spl_conf = spl_ra9703a_conf;
            *spl_conf_len = sizeof(spl_ra9703a_conf) / sizeof(conf_info_t);
        } else if (cfg_type == 1) {
            *spl_conf = spl_ra9703a_conf_rtc;
            *spl_conf_len = 3U;
        } else if (cfg_type == 2) {
            *spl_conf = spl_ra9703a_conf_wdt;
            *spl_conf_len = 3U;
        } else if (cfg_type == 3) {
            *spl_conf = spl_ra9703a_conf_rch;
            *spl_conf_len = 1U;
        } else/* if (cfg_type == 4)*/ {
            *spl_conf = spl_ra9703a_conf_rcl;
            *spl_conf_len = 1U;
        }
    }
#endif
    (void)spl_conf;
    (void)spl_conf_len;
}
#endif

/**
 * @brief sysconf_check
 *
 * @param cfg_type : 0=sys_conf, 1=rtc_conf, 2=wdt_conf, 3=rch_conf,4=rcl_conf
 * @return eSysoptionRet_TypeDef SYSOPTION_PASS or SYSOPTION_FAIL
 */
#if defined(RN_CM0_PLATFORM)
static eSysoptionRet_TypeDef sysconf_check(rn_chipid_t chipid, unsigned char cfg_type)
#else
static eSysoptionRet_TypeDef sysconf_check(unsigned char cfg_type)
#endif
{
    unsigned int *spl_base;
    unsigned int spl_len;
    unsigned char spl_valid[2];
    const conf_info_t *spl_conf_p;
    const conf_info_t *spl_conf;
    unsigned int spl_conf_len;
    unsigned int i, p;
    eSysoptionRet_TypeDef res = SYSOPTION_PASS;
#if defined(RN_CM0_PLATFORM)
    const unsigned int sysps_bak = MEM32(SYS_PS_ADDR(chipid));
    const unsigned int mod0_bak = MEM32(MOD0_EN_ADDR(chipid));
    const unsigned int mod1_bak = MEM32(MOD1_EN_ADDR(chipid));
    MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    const unsigned int sysps_bak = MEM32(SYS_PS_9303_ADDR);
    const unsigned int mod0_bak = MEM32(MOD0_EN_9303_ADDR);
    const unsigned int mod1_bak = MEM32(MOD1_EN_9303_ADDR);
    MEM32(SYS_PS_9303_ADDR) = 0x82U;
#else
    const unsigned int sysps_bak = MEM32(SYS_PS_9103_ADDR);
    const unsigned int mod0_bak = MEM32(MOD0_EN_9103_ADDR);
    const unsigned int mod1_bak = MEM32(MOD1_EN_9103_ADDR);
    MEM32(SYS_PS_9103_ADDR) = 0x82U;
#endif
    unsigned int rtcps_bak;
    unsigned short cnt;
#if defined(RN_CM0_PLATFORM)
    unsigned char vbat_flag = 0U;
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
    unsigned int anaps_bak = 0x0U;
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN202x_RN7326_SOC_B)
    volatile unsigned int tie_addr = 0x4U;
#endif
    unsigned int tt;
    unsigned int reg_addr;
    unsigned int reg_value;
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
    unsigned char spc_flag;
    unsigned int dig5_addr = 0x0U;
    unsigned int osc_ctrl1_bak;
#endif
    unsigned int reg_check_error;
    unsigned int reg_spl_addr;

    if (((MEM32(OSC_CTRL1_ADDR) >> 4) & 0x1U) == 0x0U) {
#if defined(RN_CM0_PLATFORM)
        {
            switch (chipid) {
                case CHIP_RA9103D:
                    spl_base = (unsigned int *)(getMipAddr(1U, chipid) + 0x00c08000U);
                    spl_len = 64U;
                    spl_conf_p = spl_ra9103d_conf;
                    spl_conf_len = sizeof(spl_ra9103d_conf) / sizeof(conf_info_t);
                    break;
                case CHIP_RA9103C:
                    spl_base = (unsigned int *)(getMipAddr(1U, chipid) + 0x00c08000U);
                    spl_len = 128U;
                    spl_conf_p = spl_ra9103c_conf;
                    spl_conf_len = sizeof(spl_ra9103c_conf) / sizeof(conf_info_t);
                    break;
                case CHIP_RA9103E:
                    spl_base = (unsigned int *)(getMipAddr(1U, chipid) + 0x00c08000U);
                    spl_len = 128U;
                    spl_conf_p = spl_ra9103e_conf;
                    spl_conf_len = sizeof(spl_ra9103e_conf) / sizeof(conf_info_t);
                    vbat_flag = 1U;
                    tie_addr = 0x4002C038U;
                    break;
                case CHIP_RA9103F:
                    spl_base = (unsigned int *)(getMipAddr(0U, chipid) + 0x00080000U);
                    spl_len = 256U;
                    spl_conf_p = spl_ra9103f_conf;
                    spl_conf_len = sizeof(spl_ra9103f_conf) / sizeof(conf_info_t);
                    vbat_flag = 1U;
                    tie_addr = 0x4002C038U;
                    break;
                case CHIP_RA9103G:
                    spl_base = (unsigned int *)(getMipAddr(0U, chipid) + 0x00080000U);
                    spl_len = 256U;
                    spl_conf_p = spl_ra9103g_conf;
                    spl_conf_len = sizeof(spl_ra9103g_conf) / sizeof(conf_info_t);
                    break;
                case CHIP_RA9303B:
                    spl_base = (unsigned int *)(getMipAddr(0U, chipid) + 0x00040000U);
                    spl_len = 256U;
                    spl_conf_p = spl_ra9303b_conf;
                    spl_conf_len = sizeof(spl_ra9303b_conf) / sizeof(conf_info_t);
                    vbat_flag = 1U;
                    tie_addr = 0x4002C050U;
                    break;
                case CHIP_RA9701A:
                    spl_base = (unsigned int *)(getMipAddr(0U, chipid) + 0x00080000U);
                    spl_len = 512U;
                    get_conf_reg(cfg_type, CHIP_RA9701A, &spl_conf_p, &spl_conf_len);
                    vbat_flag = 1U;
                    tie_addr = 0x4002C074U;
                    break;
                case CHIP_RA9701B:
                    spl_base = (unsigned int *)(getMipAddr(0U, chipid) + 0x00040000U);
                    spl_len = 512U;
                    get_conf_reg(cfg_type, CHIP_RA9701B, &spl_conf_p, &spl_conf_len);
                    vbat_flag = 1U;
                    tie_addr = 0x4002C074U;
                    break;
                case CHIP_RA9702A:
                    spl_base = (unsigned int *)(getMipAddr(0U, chipid) + 0x00080000U);
                    spl_len = 512U;
                    get_conf_reg(cfg_type, CHIP_RA9702A, &spl_conf_p, &spl_conf_len);
                    break;
                case CHIP_RA9702B:
                    spl_base = (unsigned int *)(getMipAddr(0U, chipid) + 0x00040000U);
                    spl_len = 512U;
                    get_conf_reg(cfg_type, CHIP_RA9702B, &spl_conf_p, &spl_conf_len);
                    break;
                case CHIP_RA9703A:
                    spl_base = (unsigned int *)(getMipAddr(0U, chipid) + 0x00080000U);
                    spl_len = 512U;
                    get_conf_reg(cfg_type, CHIP_RA9703A, &spl_conf_p, &spl_conf_len);
                    vbat_flag = 1U;
                    break;
                case CHIP_RA9106A:
                case CHIP_RA9106B:
                case CHIP_RA9106C:
                case CHIP_RA9303A:
                case CHIP_INVALID:
                default:
                    spl_base = (unsigned int *)(0x00080000U);;
                    spl_len = 0x0U;
                    spl_conf_len = 0x0U;
                    spl_conf_p = spl_ra9103d_conf; /* meaningless */
                    res = SYSOPTION_FAIL;
                    break;
            }
#elif defined(RN831x_RN861x_MCU_V1)
        i = (unsigned int)get_mcu_v1_version();
        if (i == (unsigned int)CHIP_INVALID) {
            res = SYSOPTION_FAIL;
        } else {
            if (i == (unsigned int)CHIP_RA9103E) {
                spl_base = (unsigned int *)(EEPROM_BASE_ADDRESS + 0x00c08000U);
                spl_len = 128U;
                spl_conf_p = spl_ra9103e_conf;
                spl_conf_len = sizeof(spl_ra9103e_conf) / sizeof(conf_info_t);
                tie_addr = 0x4002C038U;
            } else {
                spl_base = (unsigned int *)(0x00080000U);
                spl_len = 256U;
                spl_conf_p = spl_ra9103f_conf;
                spl_conf_len = sizeof(spl_ra9103f_conf) / sizeof(conf_info_t);
                tie_addr = 0x4002C038U;
            }
#elif defined(RN831x_RN861x_MCU_V2)
        {
            spl_base = (unsigned int *)(0x00080000U);
            spl_len = 512U;
            get_conf_reg(cfg_type, &spl_conf_p, &spl_conf_len);
            tie_addr = 0x4002C074U;
#elif defined(RN831x_RN861x_MCU_V3)
        {
            spl_base = (unsigned int *)(0x00040000U);
            spl_len = 512U;
            get_conf_reg(cfg_type, &spl_conf_p, &spl_conf_len);
            tie_addr = 0x4002C074U;
#elif defined(RN821x_RN721x_SOC_B)
        {
            spl_base = (unsigned int *)(EEPROM_BASE_ADDRESS + 0x00c08000U);
            spl_len = 64U;
            spl_conf_p = spl_ra9103d_conf;
            spl_conf_len = sizeof(spl_ra9103d_conf) / sizeof(conf_info_t);
#elif defined(RN821x_RN721x_SOC_C)
        {
            spl_base = (unsigned int *)(EEPROM_BASE_ADDRESS + 0x00c08000U);
            spl_len = 128U;
            spl_conf_p = spl_ra9103c_conf;
            spl_conf_len = sizeof(spl_ra9103c_conf) / sizeof(conf_info_t);
#elif defined(RN821x_RN721x_SOC_D)
        {
            spl_base = (unsigned int *)(0x00080000U);
            spl_len = 256U;
            spl_conf_p = spl_ra9103g_conf;
            spl_conf_len = sizeof(spl_ra9103g_conf) / sizeof(conf_info_t);
#elif defined(RN821x_RN721x_SOC_V2)
        {
            spl_base = (unsigned int *)(0x00080000U);
            spl_len = 512U;
            get_conf_reg(cfg_type, &spl_conf_p, &spl_conf_len);
#elif defined(RN821x_RN721x_SOC_V3)
        {
            spl_base = (unsigned int *)(0x00040000U);
            spl_len = 512U;
            get_conf_reg(cfg_type, &spl_conf_p, &spl_conf_len);
#elif defined(RN202x_RN7326_SOC_B)
        {
            spl_base = (unsigned int *)(0x00040000U);
            spl_len = 256U;
            spl_conf_p = spl_ra9303b_conf;
            spl_conf_len = sizeof(spl_ra9303b_conf) / sizeof(conf_info_t);
            tie_addr = 0x4002C050U;
#elif defined(RN202x_RN7326_SOC_V2)
        {
            spl_base = (unsigned int *)(0x00080000U);
            spl_len = 512U;
            get_conf_reg(cfg_type, &spl_conf_p, &spl_conf_len);
#endif
#if defined(RN_CM0_PLATFORM)
            if (SYSOPTION_PASS == res) {
                /* EMB */
                MEM32(MOD0_EN_ADDR(chipid)) |= (((unsigned int)1U << 3) | ((unsigned int)1U << 23));
                /* RTC */
                MEM32(MOD1_EN_ADDR(chipid)) |= (((unsigned int)1U << 11) | ((unsigned int)1U << 10) | ((unsigned int)1U << 9));
                rtcps_bak = MEM32(RTC_PS_ADDR);
                MEM32(SYS_PS_ADDR(chipid)) = 0x75U;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
            {
                /* EMB */
                MEM32(MOD0_EN_9303_ADDR) |= (((unsigned int)1U << 3) | ((unsigned int)1U << 23));
                /* RTC */
                MEM32(MOD1_EN_9303_ADDR) |= (((unsigned int)1U << 11) | ((unsigned int)1U << 10) | ((unsigned int)1U << 9));
                rtcps_bak = MEM32(RTC_PS_ADDR);
                MEM32(SYS_PS_9303_ADDR) = 0x75U;
#else
            {
                /* EMB */
                MEM32(MOD0_EN_9103_ADDR) |= (((unsigned int)1U << 3) | ((unsigned int)1U << 23));
                /* RTC */
                MEM32(MOD1_EN_9103_ADDR) |= (((unsigned int)1U << 11) | ((unsigned int)1U << 10) | ((unsigned int)1U << 9));
                rtcps_bak = MEM32(RTC_PS_ADDR);
                MEM32(SYS_PS_9103_ADDR) = 0x75U;
#endif
                MEM32(RTC_PS_ADDR) = 0x8eU;

                /* vbat power domain check */
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
#if defined(RN_CM0_PLATFORM)
                if ((1U == vbat_flag) && (0U == cfg_type)) {
#else
                if (0U == cfg_type) {
#endif
#if defined(RN_CM0_PLATFORM)
                    if (chipid > CHIP_RA9103G)
                    {
                        anaps_bak = MEM32(MADC_ANA_PS_ADDR(chipid));
                        MEM32(MADC_ANA_PS_ADDR(chipid)) = 0x8eU;
                    }
#elif defined(RN202x_RN7326_SOC_B)
                    anaps_bak = MEM32(MADC_ANA_PS_9303_ADDR);
                    MEM32(MADC_ANA_PS_9303_ADDR) = 0x8eU;
#else
                    anaps_bak = MEM32(MADC_ANA_PS_9701_ADDR);
                    MEM32(MADC_ANA_PS_9701_ADDR) = 0x8eU;
#endif
#if defined(RN_CM0_PLATFORM)
                    if (1U == RtcCalCheck(chipid)) {
                        vbat_reset(chipid);
                    }
#else
                    if (1U == RtcCalCheck()) {
                        vbat_reset();
                    }
#endif
                    MEM32(RTC_PS_ADDR) = 0x8eU;
#if !defined(RN202x_RN7326_SOC_V2)
                    MEM32(tie_addr) = (unsigned int)0xaU << 12;
                    MEM32(tie_addr) = 0U;
#endif
                }
#endif
                MEM32(WDT_PS_ADDR) = 0x70U;
                MEM32(WDT_PS_ADDR) = 0x7eU;
                spl_valid[0U] = check_spl_valid(spl_base, spl_len, 0U);
                tt = (unsigned int)spl_base;
                tt +=spl_len;
                spl_valid[1U] = check_spl_valid((unsigned int *)tt, spl_len, 1U);

                /* configuration info registers check */
                for (i = 0U; i < spl_conf_len; i++) {
                    p = (unsigned int)(spl_conf_p);
                    p += i * (unsigned int)(sizeof(conf_info_t));
                    spl_conf = (const conf_info_t *)(p);

                    reg_addr = spl_conf->reg_addr;
                    reg_value = (MEM32(reg_addr)) & (unsigned int)(spl_conf->reg_mask);

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
                    /* spc1: for dig5 regs */
#if defined(RN_CM0_PLATFORM)
                    if ((spl_conf->reg_id == REG_ANA_LURLS_CFG_9701) || (spl_conf->reg_id == REG_ANA_LURLS_CFG_9703)) {
                        spc_flag = 1U;
                        dig5_addr= (unsigned int)DIG5_LURLS_CFG;
                    } else if ((spl_conf->reg_id == REG_ANA_LURLS_EN_9701) || (spl_conf->reg_id == REG_ANA_LURLS_EN_9703)) {
                        spc_flag = 2U;
                        dig5_addr = (unsigned int)DIG5_LURLS_EN;
                    } else if ((spl_conf->reg_id == REG_ANA_LDO) && IS_RA97_SERIES(chipid)) {
                        spc_flag = 3U;
                        dig5_addr = (unsigned int)DIG5_ANA_LDO;
                    } else if (spl_conf->reg_id == REG_ANA_ULBGR1) {
                        spc_flag = 3U;
                        dig5_addr = (unsigned int)DIG5_ANA_ULBGR1;
                    } else if (spl_conf->reg_id == REG_ANA_ULBGR2) {
                        spc_flag = 3U;
                        dig5_addr = (unsigned int)DIG5_ANA_ULBGR2;
                    } else {
                        spc_flag = 0U;
                    }
#elif defined(RN831x_RN861x_MCU_V2)
                    if (spl_conf->reg_id == REG_ANA_LDO) {
                        spc_flag = 3U;
                        dig5_addr = (unsigned int)DIG5_ANA_LDO;
                    } else if (spl_conf->reg_id == REG_ANA_ULBGR1) {
                        spc_flag = 3U;
                        dig5_addr = (unsigned int)DIG5_ANA_ULBGR1;
                    } else {
                        spc_flag = 0U;
                    }
#elif defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN831x_RN861x_MCU_V3)
                    if (spl_conf->reg_id == REG_ANA_LURLS_CFG_9701) {
                        spc_flag = 1U;
                        dig5_addr= (unsigned int)DIG5_LURLS_CFG;
                    } else if (spl_conf->reg_id == REG_ANA_LURLS_EN_9701) {
                        spc_flag = 2U;
                        dig5_addr = (unsigned int)DIG5_LURLS_EN;
                    } else if (spl_conf->reg_id == REG_ANA_LDO) {
                        spc_flag = 3U;
                        dig5_addr = (unsigned int)DIG5_ANA_LDO;
                    } else if (spl_conf->reg_id == REG_ANA_ULBGR1) {
                        spc_flag = 3U;
                        dig5_addr = (unsigned int)DIG5_ANA_ULBGR1;
                    } else {
                        spc_flag = 0U;
                    }
#elif defined(RN202x_RN7326_SOC_V2)
                    if (spl_conf->reg_id == REG_ANA_LURLS_CFG_9703) {
                        spc_flag = 1U;
                        dig5_addr= (unsigned int)DIG5_LURLS_CFG;
                    } else if (spl_conf->reg_id == REG_ANA_LURLS_EN_9703) {
                        spc_flag = 2U;
                        dig5_addr = (unsigned int)DIG5_LURLS_EN;
                    } else if (spl_conf->reg_id == REG_ANA_LDO) {
                        spc_flag = 3U;
                        dig5_addr = (unsigned int)DIG5_ANA_LDO;
                    } else if (spl_conf->reg_id == REG_ANA_ULBGR1) {
                        spc_flag = 3U;
                        dig5_addr = (unsigned int)DIG5_ANA_ULBGR1;
                    } else {
                        spc_flag = 0U;
                    }
#else
                    spc_flag = 0U;
#endif
                    if (spc_flag != 0x0U) {
                        /* 9701 can't read by saddr, so read directly by reg address except for lurls_cfg/en  */
                        if (spc_flag == 3U) {
                            reg_value = (unsigned short)MEM32(reg_addr);
                        } else {
#if defined(RN_CM0_PLATFORM)
                            MEM32(ANA_SADDR_ADDR(chipid)) = 0xa5470000U + dig5_addr;
                            reg_value = MEM32(ANA_SDATA_ADDR(chipid));
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
                            MEM32(ANA_SADDR_9701_ADDR) = 0xa5470000U + dig5_addr;
                            reg_value = MEM32(ANA_SDATA_9103_ADDR);
#else
                            MEM32(ANA_SADDR_9703_ADDR) = 0xa5470000U + dig5_addr;
                            reg_value = MEM32(ANA_SDATA_9703_ADDR);
#endif
                        }
                        reg_value &= spl_conf->reg_mask;
                        if (spc_flag == 2U) {
                            reg_value = (reg_value == 1U) ? (unsigned int)0xc5U : (unsigned int)0x0U;
                        }
                    }
#endif
                    reg_spl_addr = (unsigned int)spl_base;
                    reg_spl_addr += (spl_conf->reg_spl_idx * spl_len) + spl_conf->reg_spl_addr;
                    tt = (unsigned int)MEM16(reg_spl_addr);
                    if ((0U == spl_valid[spl_conf->reg_spl_idx]) && (reg_value != ((unsigned int)spl_conf->reg_default & (unsigned int)spl_conf->reg_mask))) {
                        reg_check_error = 1U;
                    } else if ((1U == spl_valid[spl_conf->reg_spl_idx]) && (reg_value != (tt & (unsigned int)spl_conf->reg_mask))) {
                        reg_check_error = 2U;
                    } else {
                        reg_check_error = 0U;
                    }

                    if (reg_check_error != 0U) {
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
                        if (spc_flag != 0U) {
#if defined(RN_CM0_PLATFORM)
                            open_dig5(chipid);
#else
                            open_dig5();
#endif
                            if (spc_flag == 3U) {
                                MEM32(reg_addr) = (((reg_check_error == 1U) ? (unsigned int)spl_conf->reg_default : (unsigned int)MEM16(reg_spl_addr)) & (unsigned int)spl_conf->reg_mask) | spl_conf->reg_passwd;
                                reg_value = (unsigned short)MEM32(reg_addr);
                            } else {
#if defined(RN_CM0_PLATFORM)
                                MEM32(ANA_SADDR_ADDR(chipid)) = 0xa5470000U | dig5_addr;
                                MEM32(ANA_SDATA_ADDR(chipid)) = 0xa5470000U |(((reg_check_error == 1U) ? (unsigned int)spl_conf->reg_default : (unsigned int)MEM16(reg_spl_addr)) & (unsigned int)spl_conf->reg_mask);
                                reg_value = MEM32(ANA_SDATA_ADDR(chipid));
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
                                MEM32(ANA_SADDR_9701_ADDR) = 0xa5470000U | dig5_addr;
                                MEM32(ANA_SDATA_9103_ADDR) = 0xa5470000U |(((reg_check_error == 1U) ? (unsigned int)spl_conf->reg_default : (unsigned int)MEM16(reg_spl_addr)) & (unsigned int)spl_conf->reg_mask);
                                reg_value = MEM32(ANA_SDATA_9103_ADDR);
#else
                                MEM32(ANA_SADDR_9703_ADDR) = 0xa5470000U | dig5_addr;
                                MEM32(ANA_SDATA_9703_ADDR) = 0xa5470000U | (((reg_check_error == 1U) ? (unsigned int)spl_conf->reg_default : (unsigned int)MEM16(reg_spl_addr)) & (unsigned int)spl_conf->reg_mask);
                                reg_value = MEM32(ANA_SDATA_9703_ADDR);
#endif
                            }
                            reg_value &= spl_conf->reg_mask;
                            if (spc_flag == 2U) {
                                reg_value = (reg_value == 1U) ? (unsigned int)0xc5U : (unsigned int)0x0U;
                            }
#if defined(RN_CM0_PLATFORM)
                            close_dig5(chipid);
#else
                            close_dig5();
#endif
                        } else
#endif
                        {
                            p = spl_conf->reg_passwd;
                            tt = ((reg_check_error == 1U) ? (unsigned int)spl_conf->reg_default : (unsigned int)MEM16(reg_spl_addr)) & (unsigned int)spl_conf->reg_mask;
                            if (0U != p) {
                                MEM32(reg_addr) = tt | p;
                            }
                            MEM32(reg_addr) = tt;

                            /* spc2: need to wait sync when access wdt */
                            if ((reg_addr & 0xffffff00U) == (WDT_CTRL_ADDR & 0xffffff00U)) {
                                /* 10Khz, 5 ticks= 500us; 14700 sysclk@29Mhz, loops=14700/28(1 loop)=525 */
                                for (cnt = 0U; cnt < 1000U; cnt++) {
                                    if (0U == (MEM32(WDT_EN_ADDR) & ((unsigned int)1U << 8))) {
                                        break;
                                    }
                                }
                            }
                            reg_value = MEM32(reg_addr) & (unsigned int)spl_conf->reg_mask;
                        }

                        if (reg_value != (((reg_check_error == 1U) ? (unsigned int)spl_conf->reg_default : (unsigned int)MEM16(reg_spl_addr)) & (unsigned int)spl_conf->reg_mask)) {
                            res = SYSOPTION_FAIL;
                        }
                    }
                }

                /* test&debug regs */
#if defined(RN_CM0_PLATFORM)
                if (IS_RA97_SERIES(chipid))
                {
                    if (MEM32(DEBUG_CTRL_ADDR) != 0x0U) {
                        MEM32(DEBUG_CTRL_ADDR) = 0xa50c0000U;
                    }
                    if (MEM32(RAMB_CTRL_ADDR) != 0x0U) {
                        MEM32(RAMB_CTRL_ADDR) = 0xd1630000U;
                    }
                    if (MEM32(FL_TRIM_ADDR) != 0x0U) {
                        MEM32(FL_TRIM_ADDR) = 0x79a50000U;
                    }
                    if ((((chipid) == CHIP_RA9701A) || ((chipid) == CHIP_RA9701B)) && (MEM32(ANA_TIEHL_ADDR) != 0x0U)) {
                        MEM32(ANA_TIEHL_ADDR) = 0x0000a000U;
                        MEM32(ANA_TIEHL_ADDR) = 0x00000000U;
                    }
                }
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3)
                {
                    if (MEM32(DEBUG_CTRL_ADDR) != 0x0U) {
                        MEM32(DEBUG_CTRL_ADDR) = 0xa50c0000U;
                    }
                    if (MEM32(RAMB_CTRL_ADDR) != 0x0U) {
                        MEM32(RAMB_CTRL_ADDR) = 0xd1630000U;
                    }
                    if (MEM32(FL_TRIM_ADDR) != 0x0U) {
                        MEM32(FL_TRIM_ADDR) = 0x79a50000U;
                    }
                    if (MEM32(ANA_TIEHL_ADDR) != 0x0U) {
                        MEM32(ANA_TIEHL_ADDR) = 0x0000a000U;
                        MEM32(ANA_TIEHL_ADDR) = 0x00000000U;
                    }
                }
#elif defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN202x_RN7326_SOC_V2)
                {
                    MEM32(DEBUG_CTRL_ADDR) = 0xa50c0000U;
                    MEM32(RAMB_CTRL_ADDR) = 0xd1630000U;
                    MEM32(FL_TRIM_ADDR) = 0x79a50000U;
                }
#endif
                MEM32(RTC_PS_ADDR) = rtcps_bak;
            }
        }
#if defined(RN_CM0_PLATFORM)
        if (((chipid == CHIP_RA9701A) || (chipid == CHIP_RA9701B)) && (cfg_type == 0U)){
            if (SYSOPTION_FAIL == res) {
                (void)ldo_auto_adj();
            } else {
                res = ldo_auto_adj();
            }
        }
#elif  defined(RN831x_RN861x_MCU_V2) ||\
       defined(RN831x_RN861x_MCU_V3)
       if (cfg_type == 0U) {
            if (SYSOPTION_FAIL == res) {
                (void)ldo_auto_adj();
            } else {
                res = ldo_auto_adj();
            }
       }
#endif
    }
#if defined(RN_CM0_PLATFORM)
    else {
        if (IS_RA97_SERIES(chipid)) {
            osc_ctrl1_bak = MEM32(OSC_CTRL1_ADDR);
            MEM32(OSC_CTRL1_ADDR) &= (~(1U << 1));
            for (i = 0U; i < 8U; i++) {
                MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
            }
            MEM32(OSC_CTRL1_ADDR) = osc_ctrl1_bak;
        }
    }
#elif defined(RN202x_RN7326_SOC_V2)
    else {
        osc_ctrl1_bak = MEM32(OSC_CTRL1_ADDR);
        MEM32(OSC_CTRL1_ADDR) &= (~(1U << 1));
        for (i = 0U; i < 8U; i++) {
            MEM32(SYS_PS_9303_ADDR) = 0x82U;
        }
        MEM32(OSC_CTRL1_ADDR) = osc_ctrl1_bak;
    }
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3)
    else {
        osc_ctrl1_bak = MEM32(OSC_CTRL1_ADDR);
        MEM32(OSC_CTRL1_ADDR) &= (~(1U << 1));
        for (i = 0U; i < 8U; i++) {
            MEM32(SYS_PS_9103_ADDR) = 0x82U;
        }
        MEM32(OSC_CTRL1_ADDR) = osc_ctrl1_bak;
    }
#endif

    MEM32(WDT_PS_ADDR) = 0x0U;
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN202x_RN7326_SOC_B)
#if defined(RN_CM0_PLATFORM)
    if (1U == vbat_flag)
#endif
    {
#if defined(RN_CM0_PLATFORM)
        MEM32(MADC_ANA_PS_ADDR(chipid)) = anaps_bak;
#elif defined(RN202x_RN7326_SOC_B)
        MEM32(MADC_ANA_PS_9303_ADDR) = anaps_bak;
#else
        MEM32(MADC_ANA_PS_9701_ADDR) = anaps_bak;
#endif
    }
#endif
#if defined(RN_CM0_PLATFORM)
    MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
    MEM32(MOD0_EN_ADDR(chipid)) = mod0_bak;
    MEM32(MOD1_EN_ADDR(chipid)) = mod1_bak;
    MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
#elif defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_B)
    MEM32(SYS_PS_9303_ADDR) = 0x82U;
    MEM32(MOD0_EN_9303_ADDR) = mod0_bak;
    MEM32(MOD1_EN_9303_ADDR) = mod1_bak;
    MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#else
    MEM32(SYS_PS_9103_ADDR) = 0x82U;
    MEM32(MOD0_EN_9103_ADDR) = mod0_bak;
    MEM32(MOD1_EN_9103_ADDR) = mod1_bak;
    MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif

    return res;
}

#if defined(RN_CM0_PLATFORM)
static void get_flash_info(rn_chipid_t chipid, flash_info_t * fl_info)
#else
static void get_flash_info(flash_info_t * fl_info)
#endif
{
#if defined(RN_CM0_PLATFORM)
    if ((chipid == CHIP_RA9103F) || (chipid == CHIP_RA9103G)) {
        /* no IAP */
        fl_info->base_addr = getMipAddr(0U, chipid);
        fl_info->reg_addr = (chipid == CHIP_RA9103F) ? 0x50014000U : 0x00800000U;
        fl_info->page_size = 256U;
        fl_info->spl_type = IN_FLASH2;
        /* flash2 spl0_offset = 0x80000U + 0x40000U */
        fl_info->spl0_offset = 0x00080000U;
    } else if (/*(chipid == CHIP_RA9103A) || */(chipid == CHIP_RA9103D)) {
        /* IAP */
        fl_info->base_addr = getMipAddr(1U, chipid);
        fl_info->reg_addr = (fl_info->base_addr + 0x00800000U);
        fl_info->page_size = 64U;
        fl_info->spl_type = IN_EEPROM;
        fl_info->spl0_offset = 0x00c08000U;
    } else if ((chipid == CHIP_RA9103C) || (chipid == CHIP_RA9103E)) {
        /* IAP */
        fl_info->base_addr = getMipAddr(1U, chipid);
        fl_info->reg_addr = (fl_info->base_addr + 0x00800000U);
        fl_info->page_size = 128U;
        fl_info->spl_type = IN_EEPROM;
        fl_info->spl0_offset = 0x00c08000U;
    } else if (CHIP_RA9303B == chipid) {
        /* IAP */
        fl_info->base_addr = getMipAddr(0U, chipid);
        fl_info->reg_addr = 0x50014000U;
        fl_info->page_size = 256U;
        fl_info->spl_type = IN_FLASH;
        fl_info->spl0_offset = 256U * 1024U;
    } else if ((CHIP_RA9701B == chipid) || (CHIP_RA9702B == chipid)) {
        /* IAP */
        fl_info->base_addr = getMipAddr(0U, chipid);
        fl_info->reg_addr = 0x50014000U;
        fl_info->page_size = 512U;
        fl_info->spl_type = IN_FLASH;
        fl_info->spl0_offset = 256U * 1024U;
    } else/* if ((CHIP_RA9701A == chipid) || (CHIP_RA9702A == chipid) || (CHIP_RA9703A == chipid))*/ {
        /* IAP */
        fl_info->base_addr = getMipAddr(0U, chipid);
        fl_info->reg_addr = 0x50014000U;
        fl_info->page_size = 512U;
        fl_info->spl_type = IN_FLASH;
        fl_info->spl0_offset = 512U * 1024U;
    }
#elif defined(RN831x_RN861x_MCU_V1)
    if (get_mcu_v1_version() == CHIP_RA9103F) {
        fl_info->base_addr = FLASH_BASE_ADDRESS;
        fl_info->reg_addr = 0x50014000U;
        fl_info->page_size = 256U;
        fl_info->spl_type = IN_FLASH2;
        /* flash2 spl0_offset = 0x80000U + 0x40000U */
        fl_info->spl0_offset = 0x00080000U;
    } else {
        fl_info->base_addr = EEPROM_BASE_ADDRESS;
        fl_info->reg_addr = (fl_info->base_addr + 0x00800000U);
        fl_info->page_size = 128U;
        fl_info->spl_type = IN_EEPROM;
        fl_info->spl0_offset = 0x00c08000U;
    }
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_V2)
    fl_info->base_addr = FLASH_BASE_ADDRESS;
    fl_info->reg_addr = 0x50014000U;
    fl_info->page_size = 512U;
    fl_info->spl_type = IN_FLASH;
    fl_info->spl0_offset = 512U * 1024U;
#elif defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V3)
    fl_info->base_addr = FLASH_BASE_ADDRESS;
    fl_info->reg_addr = 0x50014000U;
    fl_info->page_size = 512U;
    fl_info->spl_type = IN_FLASH;
    fl_info->spl0_offset = 256U * 1024U;
#elif defined(RN821x_RN721x_SOC_C)
    fl_info->base_addr = EEPROM_BASE_ADDRESS;
    fl_info->reg_addr = (fl_info->base_addr + 0x00800000U);
    fl_info->page_size = 128U;
    fl_info->spl_type = IN_EEPROM;
    fl_info->spl0_offset = 0x00c08000U;
#elif defined(RN821x_RN721x_SOC_B)
    fl_info->base_addr = EEPROM_BASE_ADDRESS;
    fl_info->reg_addr = (fl_info->base_addr + 0x00800000U);
    fl_info->page_size = 64U;
    fl_info->spl_type = IN_EEPROM;
    fl_info->spl0_offset = 0x00c08000U;
#elif defined(RN821x_RN721x_SOC_D)
    fl_info->base_addr = FLASH_BASE_ADDRESS;
    fl_info->reg_addr = 0x00800000U;
    fl_info->page_size = 256U;
    fl_info->spl_type = IN_FLASH2;
    /* flash2 spl0_offset = 0x80000U + 0x40000U */
    fl_info->spl0_offset = 0x00080000U;
#elif defined(RN202x_RN7326_SOC_B)
    fl_info->base_addr = FLASH_BASE_ADDRESS;
    fl_info->reg_addr = 0x50014000U;
    fl_info->page_size = 256U;
    fl_info->spl_type = IN_FLASH;
    fl_info->spl0_offset = 256U * 1024U;
#endif
}

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_D) ||\
    defined(RN831x_RN861x_MCU_V1)
static eSysoptionRet_TypeDef ReadSpl1Data(flash_info_t *fl_info, unsigned int data[])
{
    unsigned int i;
    unsigned int cksum;
    unsigned int spl1_addr;
    unsigned char mem_type;
    eSysoptionRet_TypeDef res = SYSOPTION_PASS;

    spl1_addr = fl_info->base_addr + fl_info->spl0_offset + fl_info->page_size;
    for (mem_type = 0U; mem_type < 2U; mem_type++) {
        cksum = 0U;
        for (i = 0U; i < (unsigned short)((fl_info->page_size / 4U) - 1U); i++) {
            data[i] = *(unsigned int *)(spl1_addr + (4U * i));
            cksum += data[i];
        }
        data[i] = *(unsigned int *)(spl1_addr + (4U * i));
        if ((cksum == 0U) && (data[(fl_info->page_size / 4U) - 3U] == 0xffffffffU)) {
            res = SYSOPTION_PASS;
        } else {
            res = SYSOPTION_FAIL;
        }
        if ((IN_FLASH2 == fl_info->spl_type) && (SYSOPTION_FAIL == res)) {
            spl1_addr += 0x00040000U;
        } else {
            break;
        }
    }

    return res;
}
#endif

#if defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN202x_RN7326_SOC_B)
static eSysoptionRet_TypeDef ReadSpl1Data(flash_info_t *fl_info, unsigned int data[])
{
    unsigned int i;
    unsigned int cksum;
    unsigned int spl1_addr;
    eSysoptionRet_TypeDef res = SYSOPTION_PASS;

    spl1_addr = fl_info->base_addr + fl_info->spl0_offset + fl_info->page_size;
    cksum = 0U;
    for (i = 0U; i < (unsigned short)((fl_info->page_size / 4U) - 1U); i++) {
        data[i] = *(unsigned int *)(spl1_addr + (4U * i));
        cksum += data[i];
    }
    data[i] = *(unsigned int *)(spl1_addr + (4U * i));
    if ((cksum != 0U) || (data[(fl_info->page_size / 4U) - 3U] != 0xffffffffU)) {
        res = SYSOPTION_FAIL;
    }

    return res;
}
#endif
#endif /* #if !defined(NVRLIB_EN) */

eSysoptionRet_TypeDef LL_SYSOPTION_SysConfCheck(void)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_SysConfCheck();
#else
#if defined(RN_CM0_PLATFORM)

    rn_chipid_t chipid = getChipid();
    return sysconf_check(chipid, 0U);

#else

    return sysconf_check(0U);
#endif
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_RtcConfCheck(void)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_RtcConfCheck();
#else
#if defined(RN_CM0_PLATFORM)

    rn_chipid_t chipid = getChipid();
    return sysconf_check(chipid, 1U);

#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN202x_RN7326_SOC_V2)

    return sysconf_check(1U);
#else
    return SYSOPTION_PASS;
#endif
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_WdtConfCheck(void)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_WdtConfCheck();
#else
#if defined(RN_CM0_PLATFORM)

    rn_chipid_t chipid = getChipid();
    return sysconf_check(chipid, 2U);

#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN202x_RN7326_SOC_V2)

    return sysconf_check(2U);
#else
    return SYSOPTION_PASS;
#endif
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_RchConfCheck(void)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_RchConfCheck();
#else
#if defined(RN_CM0_PLATFORM)

    rn_chipid_t chipid = getChipid();
    return sysconf_check(chipid, 3U);

#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN202x_RN7326_SOC_V2)

    return sysconf_check(3U);
#else
    return SYSOPTION_PASS;
#endif
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_RclConfCheck(void)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_RclConfCheck();
#else
#if defined(RN_CM0_PLATFORM)

    rn_chipid_t chipid = getChipid();
    return sysconf_check(chipid, 4U);

#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN202x_RN7326_SOC_V2)

    return sysconf_check(4U);
#else
    return SYSOPTION_PASS;
#endif
#endif
}

#if !defined(NVRLIB_EN)
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_D) ||\
    defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN202x_RN7326_SOC_B)
static void WaitFalsh(const volatile unsigned int *fl_sta)
{
    unsigned int i;

    /* 10ms timeout: 327680 / 6 = 54000 (32.768Mhz) */
    for (i = 0U; i < 54000U; i++) {
        if ((*fl_sta & 0x1U) == 0x0U) {
            break;
        }
    }
}

/* !!!Only for NO IAP(RA9103F & 9103G & 9303B) */
#if defined(RN_CM0_PLATFORM)
static eSysoptionRet_TypeDef UpdateSpl1(rn_chipid_t chipid, flash_info_t *fl_info, const unsigned int data[])
#elif defined(RN831x_RN861x_MCU_V1) ||\
      defined(RN821x_RN721x_SOC_D) ||\
      defined(RN202x_RN7326_SOC_B)
static eSysoptionRet_TypeDef UpdateSpl1(flash_info_t *fl_info, const unsigned int data[])
#endif
{
    unsigned int i;
    unsigned int temp;
    const unsigned int buf = (unsigned int)data;
    unsigned char flash_num;
    eSysoptionRet_TypeDef res;

    *(volatile unsigned int *)(fl_info->reg_addr + 0x14U) = 0xfeed0000U;
    *(volatile unsigned int *)(fl_info->reg_addr + 0x4U) = 0x401U;
#if defined(RN_CM0_PLATFORM)
    if (chipid != CHIP_RA9303B)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN821x_RN721x_SOC_D)
    {
        *(volatile unsigned int *)(fl_info->reg_addr + 0x34U) = 0xfeed0000U;
        *(volatile unsigned int *)(fl_info->reg_addr + 0x24U) = 0x401U;
    }
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1) ||\
    defined(RN821x_RN721x_SOC_D)
    for (flash_num = 0U; flash_num < 2U; flash_num++)
#else
    for (flash_num = 0U; flash_num < 1U; flash_num++)
#endif
    {
        res = SYSOPTION_PASS;
        *(volatile unsigned int *)(fl_info->reg_addr + (0x20U * (unsigned int)flash_num)) = FL_ERASE_PAGE;
        WaitFalsh((volatile unsigned int *)(fl_info->reg_addr + 0x8U + (0x20U * (unsigned int)flash_num)));
        *(volatile unsigned int *)(fl_info->reg_addr + (0x20U * (unsigned int)flash_num)) = FL_CLR_HVPL;
        for (i = 0U; i < fl_info->page_size; i++) {
            *((volatile unsigned char *)(fl_info->base_addr + (0x100000U * (unsigned int)flash_num)  + i)) = *(unsigned char *)(buf + i);
        }
        *(volatile unsigned int *)(fl_info->reg_addr + (0x20U * (unsigned int)flash_num)) = FL_PROG_PAGE;
        WaitFalsh((volatile unsigned int *)(fl_info->reg_addr + 0x8U + (0x20U * (unsigned int)flash_num)));

        temp = fl_info->base_addr + fl_info->spl0_offset + (0x00040000U * (unsigned int)flash_num) + fl_info->page_size;
        for (i = 0U; i < ((unsigned int)fl_info->page_size / 4U); i++) {
            if (*(unsigned int *)(temp) != data[i]) {
                res = SYSOPTION_FAIL;
                break;
            }
            temp += 4U;
        }
#if defined(RN_CM0_PLATFORM)
        if ((SYSOPTION_FAIL == res) || (chipid == CHIP_RA9303B))
        {
            break;
        }
#elif defined(RN831x_RN861x_MCU_V1) ||\
      defined(RN821x_RN721x_SOC_D)
        if (SYSOPTION_FAIL == res)
        {
            break;
        }
#endif
    }

    return res;
}
#endif

static eSysoptionRet_TypeDef UpdateObjects(const spl_data_t spl_data[], unsigned short len)
{
    flash_info_t fl_info;
    unsigned int sp1[512U / 4U];
    unsigned int i;
    unsigned int cksum = 0U;
    unsigned int temp;

    unsigned char retry;
    eSysoptionRet_TypeDef res;
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V1)
    rn_chipid_t chipid;
#endif
#if !defined(RN821x_RN721x_SOC_D) &&\
    !defined(RN202x_RN7326_SOC_B)
    rn_iap_t iap;
#endif
#if !defined(RN821x_RN721x_SOC_D) &&\
    !defined(RN202x_RN7326_SOC_B)
    rn_iap_entry_t ptr_union;
    ptr_union.addr = ROM_BASE_ADDRESS + IAP_OFFSET;
#endif

    if (((MEM32(OSC_CTRL1_ADDR) >> 4) & 0x1U) == 0x1U) {
        res = SYSOPTION_FAIL;
    } else {
#if defined(RN_CM0_PLATFORM)
        chipid = getChipid();
        get_flash_info(chipid, &fl_info);
#else
        get_flash_info(&fl_info);
#endif

#if defined(RN831x_RN861x_MCU_V1)
        chipid = get_mcu_v1_version();
#endif
        res = ReadSpl1Data(&fl_info, sp1);
        if (SYSOPTION_PASS == res) {
            temp = (unsigned int)spl_data;
            for (i = 0U; i < len; i++) {
                *(unsigned short *)((unsigned int)sp1 + ((spl_data_t *)temp)->offset_addr) = ((spl_data_t *)temp)->value;
                temp += sizeof(spl_data_t);
            }
            for (i = 0U; i < (((unsigned int)fl_info.page_size / 4U) - 2U); i++) {
                cksum += sp1[i];
            }
            sp1[(fl_info.page_size / 4U) - 2U] = 0U - cksum;

            for (retry = 0U; retry < 3U; retry++) {
                res = SYSOPTION_PASS;
#if defined(RN_CM0_PLATFORM)
                if ((CHIP_RA9103G == chipid) || (CHIP_RA9103F == chipid) || (CHIP_RA9303B == chipid)) {
                    (void)UpdateSpl1(chipid, &fl_info, sp1);
#elif defined(RN821x_RN721x_SOC_D)
                (void)UpdateSpl1(&fl_info, sp1);
#elif defined(RN202x_RN7326_SOC_B)
                (void)UpdateSpl1(&fl_info, sp1);
#elif defined(RN831x_RN861x_MCU_V1)
                if (chipid == CHIP_RA9103F) {
                    (void)UpdateSpl1(&fl_info, sp1);
#endif
#if defined(RN_CM0_PLATFORM)
                } else if (/*(CHIP_RA9103A == chipid) || */(CHIP_RA9103D == chipid) || (CHIP_RA9103C == chipid) || (CHIP_RA9103E == chipid)) {
                    (void)IapMemEnable(EE_MEM_EN);
                    iap.cmd = IAP_EW;
                    iap.par[0] = fl_info.base_addr + fl_info.spl0_offset + fl_info.page_size;
                    iap.par[1] = (unsigned int)sp1;
                    iap.par[2] = fl_info.page_size;
                    ptr_union.iap_func(&iap.cmd, &iap.stat);
#elif defined(RN821x_RN721x_SOC_B) ||\
      defined(RN821x_RN721x_SOC_C)
                (void)IapMemEnable(EE_MEM_EN);
                iap.cmd = IAP_EW;
                iap.par[0] = fl_info.base_addr + fl_info.spl0_offset + fl_info.page_size;
                iap.par[1] = (unsigned int)sp1;
                iap.par[2] = fl_info.page_size;
                ptr_union.iap_func(&iap.cmd, &iap.stat);
#elif defined(RN831x_RN861x_MCU_V1)
                } else if (chipid == CHIP_RA9103E) {
                    (void)IapMemEnable(EE_MEM_EN);
                    iap.cmd = IAP_EW;
                    iap.par[0] = fl_info.base_addr + fl_info.spl0_offset + fl_info.page_size;
                    iap.par[1] = (unsigned int)sp1;
                    iap.par[2] = fl_info.page_size;
                    ptr_union.iap_func(&iap.cmd, &iap.stat);
                } else {
                    res = SYSOPTION_FAIL;
                }
#endif
#if defined(RN_CM0_PLATFORM)
                } else/* if ((CHIP_RA9701A == chipid) || (CHIP_RA9702A == chipid) || (CHIP_RA9703A == chipid) || (CHIP_RA9701B == chipid) || (CHIP_RA9702B == chipid))*/ {
                    (void)IapMemEnable(FL_MEM_EN);
                    iap.cmd = IAP_NVRE;
                    iap.par[0] = 1U;
                    ptr_union.iap_func(&iap.cmd, &iap.stat);
                    (void)IapMemEnable(FL_MEM_EN);
                    iap.cmd = IAP_FW;
                    iap.par[0] = fl_info.base_addr + fl_info.spl0_offset + (1U * (unsigned int)fl_info.page_size);
                    iap.par[1] = (unsigned int)(sp1);
                    iap.par[2] = fl_info.page_size;
                    ptr_union.iap_func(&iap.cmd, &iap.stat);
                }
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN202x_RN7326_SOC_V2)
                (void)IapMemEnable(FL_MEM_EN);
                iap.cmd = IAP_NVRE;
                iap.par[0] = 1U;
                ptr_union.iap_func(&iap.cmd, &iap.stat);
                (void)IapMemEnable(FL_MEM_EN);
                iap.cmd = IAP_FW;
                iap.par[0] = fl_info.base_addr + fl_info.spl0_offset + (1U * (unsigned int)fl_info.page_size);
                iap.par[1] = (unsigned int)(sp1);
                iap.par[2] = fl_info.page_size;
                ptr_union.iap_func(&iap.cmd, &iap.stat);
#endif

                temp = fl_info.base_addr + fl_info.spl0_offset + (1U * (unsigned int)fl_info.page_size);
                for (i = 0U; i < ((unsigned int)fl_info.page_size / 4U); i++) {
                    if (*(volatile unsigned int *)temp != sp1[i]) {
                        res = SYSOPTION_FAIL;
                        break;
                    }
                    temp += 4U;
                }

                if (SYSOPTION_PASS == res) {
                    break;
                }
            }
        }
    }

    return res;
}

static eSysoptionRet_TypeDef ReadObjects(spl_data_t spl_data[], unsigned short len)
{
    flash_info_t fl_info;
    unsigned short i;
    unsigned int sp1[512U / 4U];
    unsigned int pvalue;
    unsigned int temp;
    eSysoptionRet_TypeDef res;
#if defined(RN_CM0_PLATFORM)
    rn_chipid_t chipid;
    chipid = getChipid();
    get_flash_info(chipid, &fl_info);
#else
    get_flash_info(&fl_info);
#endif

    res = ReadSpl1Data(&fl_info, sp1);
    if (SYSOPTION_PASS == res) {
        pvalue = (unsigned int)sp1;
        temp = (unsigned int)spl_data;
        for (i = 0U; i < len; i++) {
            ((spl_data_t *)temp)->value = *(unsigned short *)(pvalue + ((spl_data_t *)temp)->offset_addr);
            temp += sizeof(spl_data_t);
        }
    }

    return res;
}
#endif /* RN821x_RN721x_SOC_V2 */

eSysoptionRet_TypeDef LL_SYSOPTION_Dota0CfgSet(unsigned short dota0)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_Dota0CfgSet(dota0);
#else
    spl_data_t spl_data[1];
    spl_data[0].offset_addr = SPL_DOTA0_ADDR;
    spl_data[0].value = dota0 & 0x1fffU;
    return UpdateObjects(spl_data, 1U);
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_AlphalCfgSet(unsigned short alphal)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_AlphalCfgSet(alphal);
#else
    spl_data_t spl_data[1];
    spl_data[0].offset_addr = SPL_ALPHAL_ADDR;
    spl_data[0].value = alphal & 0x7ffU;
    return UpdateObjects(spl_data, 1U);
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_AlphahCfgSet(unsigned short alphah)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_AlphahCfgSet(alphah);
#else
    spl_data_t spl_data[1];
    spl_data[0].offset_addr = SPL_ALPHAH_ADDR;
    spl_data[0].value = alphah & 0x7ffU;
    return UpdateObjects(spl_data, 1U);
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_Xt0CfgSet(unsigned short xt0)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_Xt0CfgSet(xt0);
#else
    spl_data_t spl_data[1];
    spl_data[0].offset_addr = SPL_XT0_ADDR;
    spl_data[0].value = xt0 & 0x7fU;
    return UpdateObjects(spl_data, 1U);
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_Remap4Sel(unsigned char sel)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_Remap4Sel(sel);
#else
    flash_info_t fl_info;
    spl_data_t spl_data[2];
    unsigned int remap_value;
    eSysoptionRet_TypeDef res = SYSOPTION_FAIL;
#if defined(RN_CM0_PLATFORM)
    rn_chipid_t chipid;
#endif

    if (sel < 2U) {
        remap_value = (sel == 0U) ? REMAP4_MAGIC : 0xffffffffU;
#if defined(RN_CM0_PLATFORM)
        chipid = getChipid();
        get_flash_info(chipid, &fl_info);
#else
        get_flash_info(&fl_info);
#endif
        spl_data[0].offset_addr = fl_info.page_size - 0x4U;
        spl_data[1].offset_addr = spl_data[0].offset_addr + 0x2U;
        spl_data[0].value = (unsigned short)(remap_value & 0xffffU);
        spl_data[1].value = (unsigned short)((remap_value >> 16) & 0xffffU);
        res = UpdateObjects(spl_data, 0x2U);
    }

    return res;
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_CpLevelSet(unsigned int cp_level, unsigned int cp_passwd)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_CpLevelSet(cp_level, cp_passwd);
#else
    spl_data_t spl_data[4];
    eSysoptionRet_TypeDef res = SYSOPTION_PASS;
    unsigned int t0;

#if   defined(RN831x_RN861x_MCU_V1) ||\
      defined(RN821x_RN721x_SOC_B)  ||\
      defined(RN821x_RN721x_SOC_C)  ||\
      defined(RN821x_RN721x_SOC_D)  ||\
      defined(RN202x_RN7326_SOC_B)
    if (cp_level > 3U) {
        res = SYSOPTION_FAIL;
    }
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3) ||\
      defined(RN202x_RN7326_SOC_V2)
    if (cp_level > 4U) {
        res = SYSOPTION_FAIL;
    }
#else
    res = SYSOPTION_FAIL;
#endif
    if (res == SYSOPTION_PASS) {
        spl_data[0].offset_addr = SPL_CP_ADDR;
        spl_data[1].offset_addr = SPL_CP_ADDR + 0x2;
        spl_data[2].offset_addr = SPL_PW_ADDR;
        spl_data[3].offset_addr = SPL_PW_ADDR + 0x2;

        t0 = (cp_level == 1) ? CP1_MAGIC :
              (cp_level == 2) ? CP2_MAGIC :
               (cp_level == 3) ? CP3_MAGIC :
                (cp_level == 4) ? CP4_MAGIC : CP0_MAGIC;
        spl_data[0].value = (unsigned short)(t0 & 0xffff);
        spl_data[1].value = (unsigned short)((t0 >> 16) & 0xffff);
        spl_data[2].value = (unsigned short)(cp_passwd & 0xffffU);
        spl_data[3].value = (unsigned short)((cp_passwd >> 16) & 0xffffU);
        res = UpdateObjects(spl_data, 4);
    }

    return res;
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_Dota0CfgGet(unsigned short *dota0)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_Dota0CfgGet(dota0);
#else
    spl_data_t spl_data;
    eSysoptionRet_TypeDef res = SYSOPTION_FAIL;

    spl_data.offset_addr = SPL_DOTA0_ADDR;
    res = ReadObjects(&spl_data, 1U);
    *dota0 = spl_data.value;

    return res;
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_AlphalCfgGet(unsigned short *alphal)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_AlphalCfgGet(alphal);
#else
    spl_data_t spl_data;
    eSysoptionRet_TypeDef res;

    spl_data.offset_addr = SPL_ALPHAL_ADDR;
    res = ReadObjects(&spl_data, 1U);
    *alphal = spl_data.value;

    return res;
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_AlphahCfgGet(unsigned short *alphah)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_AlphahCfgGet(alphah);
#else
    spl_data_t spl_data;
    eSysoptionRet_TypeDef res = SYSOPTION_FAIL;

    spl_data.offset_addr = SPL_ALPHAH_ADDR;
    res = ReadObjects(&spl_data, 1U);
    *alphah = spl_data.value;

    return res;
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_Xt0CfgGet(unsigned short *xt0)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_Xt0CfgGet(xt0);
#else
    spl_data_t spl_data;
    eSysoptionRet_TypeDef res = SYSOPTION_FAIL;

    spl_data.offset_addr = SPL_XT0_ADDR;
    res = ReadObjects(&spl_data, 1U);
    *xt0 = spl_data.value;

    return res;
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_RemapGet(unsigned short *remap)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_RemapGet(remap);
#else
    flash_info_t fl_info;
    spl_data_t spl_data[2];
    unsigned int value;
    eSysoptionRet_TypeDef res = SYSOPTION_FAIL;
#if defined(RN_CM0_PLATFORM)
    rn_chipid_t chipid;
#endif

#if defined(RN_CM0_PLATFORM)
    chipid = getChipid();
    get_flash_info(chipid, &fl_info);
#else
    get_flash_info(&fl_info);
#endif
    spl_data[0].offset_addr = fl_info.page_size - 0x4U;
    spl_data[1].offset_addr = spl_data[0].offset_addr + 0x2U;
    res = ReadObjects(spl_data, 2U);
    if (SYSOPTION_PASS == res) {
        value = (unsigned int)spl_data[0].value | ((unsigned int)spl_data[1].value << 16);
        if (REMAP4_MAGIC == value) {
            *remap = 4U;
        } else if (REMAP1_MAGIC == value) {
            *remap = 1U;
        } else {
            *remap = 0U;
        }
    }

    return res;
#endif
}

eSysoptionRet_TypeDef LL_SYSOPTION_CpLevelGet(unsigned int *cp_level)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_CpLevelGet(cp_level);
#else
    spl_data_t spl_data[2];
    unsigned int value;
    eSysoptionRet_TypeDef res = SYSOPTION_FAIL;

    spl_data[0].offset_addr = SPL_CP_ADDR;
    spl_data[1].offset_addr = SPL_CP_ADDR + 2;
    res = ReadObjects(spl_data, 2U);
    if (SYSOPTION_PASS == res) {
        value = (unsigned int)spl_data[0].value | ((unsigned int)spl_data[1].value << 16);
        *cp_level = (value == CP1_MAGIC) ? 1U :
                     (value == CP2_MAGIC) ? 2U :
                      (value == CP3_MAGIC) ? 3U :
                       (value == CP4_MAGIC) ? 4U : 0U;
    }

    return res;
#endif
}

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3)
#define  MCU_VER_DIFF_ADDR      (0x180002c0U)
#define  MCU_VER_V21_MAGIC      (0x02e39802U)
#define  MCU_VER_V22_MAGIC      (0x24001c6dU)
#define  MCU_VER_V3_MAGIC       (0x21537001U)
/**
 * @brief get mcu version
 *
 * @return eMcuVer_TypeDef mcu version
 */
eMcuVer_TypeDef LL_SYSOPTION_GetMcuVersion(void)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_SYSOPTION_GetMcuVersion();
#else
    eMcuVer_TypeDef ver;
#if defined(RN_CM0_PLATFORM)
    rn_chipid_t chipid = getChipid();
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2)
    unsigned int flag[3], i;
#endif

#if defined(RN_CM0_PLATFORM)
    for (i = 0U; i < 3U; i++) {
        flag[i] = *(volatile unsigned int *)(MCU_VER_DIFF_ADDR);
    }
    if ((flag[0] == flag[1]) && (flag[1] == flag[2])) {
        if (chipid == CHIP_RA9701A) {
            if (flag[0] == MCU_VER_V21_MAGIC) {
                ver = MCU_VER_V21;
            } else/* if (flag[0] == MCU_VER_V22_MAGIC) */ {
                ver = MCU_VER_V22;
            }
        } else if (chipid == CHIP_RA9701B) {
            ver = MCU_VER_V3;
        } else {
            ver = MCU_VER_INVALID;
        }
    } else {
        ver = MCU_VER_INVALID;
    }
#elif defined(RN831x_RN861x_MCU_V2)
    for (i = 0U; i < 3U; i++) {
        flag[i] = *(volatile unsigned int *)(MCU_VER_DIFF_ADDR);
    }
    if ((flag[0] == flag[1]) && (flag[1] == flag[2])) {
        if (flag[0] == MCU_VER_V21_MAGIC) {
            ver = MCU_VER_V21;
        } else/* if (flag[0] == MCU_VER_V22_MAGIC) */ {
            ver = MCU_VER_V22;
        }
    } else {
        ver = MCU_VER_INVALID;
    }
#elif defined(RN831x_RN861x_MCU_V3)
    ver = MCU_VER_V3;
#else
    ver = MCU_VER_INVALID;
#endif

    return ver;
#endif
}
#endif
#endif
/* r2989 */
