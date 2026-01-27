/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_gpio.h
 * @author  Renergy Software Team
 * @brief   GPIO module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_GPIO_H
#define _RN831x_RN861x_MCU_GPIO_H

#include "rn831x_rn861x_mcu.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/*                Device Specific Peripheral registers structures             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Peripherals rn831x_rn861x_mcu Peripherals
 * @brief rn831x_rn861x_mcu Device Specific Peripheral registers structures
 * @{
 */

/* GPIO */
typedef struct
{
    __IO uint32_t MODE;                      /*!< offset: 0x000 info: GPIO mode register */
    __IO uint32_t AEN;                       /*!< offset: 0x004 info: Analog enable register */
    __IO uint32_t OTYPE;                     /*!< offset: 0x008 info: Output type register */
    __IO uint32_t OSPEED;                    /*!< offset: 0x00C info: Output speed register */
    __IO uint32_t FIEN;                      /*!< offset: 0x010 info: Force input enable register */
    __IO uint32_t ITYPE;                     /*!< offset: 0x014 info: Input type register */
    __IO uint32_t UPDN;                      /*!< offset: 0x018 info: Pull up or pull down register */
    __IO uint32_t IDATA;                     /*!< offset: 0x01C info: Input data */
    __IO uint32_t ODATA;                     /*!< offset: 0x020 info: Output data */
    __IO uint32_t AFSEL0;                    /*!< offset: 0x024 info: Alternate function seclect */
    __IO uint32_t AFSEL1;                    /*!< offset: 0x028 info: Alternate function seclect */
    __IO uint32_t SET;                       /*!< offset: 0x02C info: Bit set */
    __IO uint32_t CLR;                       /*!< offset: 0x030 info: Bit clear */
    __IO uint32_t TG;                        /*!< offset: 0x034 info: Bit toggle */
} GPIO_TypeDef;

/* GPIO_DBG */
typedef struct
{
    __IO uint32_t DBG_KEY;                   /*!< offset: 0x070 info: Key of debug register */
    __IO uint32_t DBG;                       /*!< offset: 0x074 info: Debug register */
} GPIO_DBG_TypeDef;

/******************************************************************************/
/*                Device Specific Peripheral registers structures             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Peripherals rn831x_rn861x_mcu Peripherals
 * @brief rn831x_rn861x_mcu Device Specific Peripheral registers structures
 * @{
 */

/* GPIO_BB */
typedef struct
{
    __IO uint32_t BB_MODE[32];               /*!< offset: 0x0000 */
    __IO uint32_t BB_AEN[32];                /*!< offset: 0x0080 */
    __IO uint32_t BB_OTYPE[32];              /*!< offset: 0x0100 */
    __IO uint32_t BB_OSPEED[32];             /*!< offset: 0x0180 */
    __IO uint32_t BB_FIEN[32];               /*!< offset: 0x0200 */
    __IO uint32_t BB_ITYPE[32];              /*!< offset: 0x0280 */
    __IO uint32_t BB_UPDN[32];               /*!< offset: 0x0300 */
    __IO uint32_t BB_IDATA[32];              /*!< offset: 0x0380 */
    __IO uint32_t BB_ODATA[32];              /*!< offset: 0x0400 */
    __IO uint32_t BB_AFSEL0[32];             /*!< offset: 0x0480 */
    __IO uint32_t BB_AFSEL1[32];             /*!< offset: 0x0500 */
    __IO uint32_t BB_SET[32];                /*!< offset: 0x0580 */
    __IO uint32_t BB_CLR[32];                /*!< offset: 0x0600 */
    __IO uint32_t BB_TG[32];                 /*!< offset: 0x0680 */
} GPIO_BB_TypeDef;

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_Peripherals */

/******************************************************************************/
/*                         Peripheral memory map                              */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_MemoryMap rn831x_rn861x_mcu Memory Mapping
 * @{
 */

/* base address */
#define GPIOA_BASE      (0xB0080000U)
#define GPIOB_BASE      (0xB0080400U)
#define GPIOC_BASE      (0xB0080800U)
#define GPIOD_BASE      (0xB0080C00U)
#define GPIOE_BASE      (0xB0081000U)
#define GPIOF_BASE      (0xB0081400U)
#define GPIOG_BASE      (0xB0081800U)
#define GPIOH_BASE      (0xB0081C00U)
#define GPIOL_BASE      (0xB0082C00U)
#define GPIO_DBG_BASE   (0xB0080070U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define GPIOA      ((GPIO_TypeDef *)     GPIOA_BASE)
#define GPIOB      ((GPIO_TypeDef *)     GPIOB_BASE)
#define GPIOC      ((GPIO_TypeDef *)     GPIOC_BASE)
#define GPIOD      ((GPIO_TypeDef *)     GPIOD_BASE)
#define GPIOE      ((GPIO_TypeDef *)     GPIOE_BASE)
#define GPIOF      ((GPIO_TypeDef *)     GPIOF_BASE)
#define GPIOG      ((GPIO_TypeDef *)     GPIOG_BASE)
#define GPIOH      ((GPIO_TypeDef *)     GPIOH_BASE)
#define GPIOL      ((GPIO_TypeDef *)     GPIOL_BASE)
#define GPIO_DBG   ((GPIO_DBG_TypeDef *) GPIOL_BASE)

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** GPIO Instances *******************************/
#define IS_GPIO_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == GPIOA) || \
                                           ((INSTANCE) == GPIOB) || \
                                           ((INSTANCE) == GPIOC) || \
                                           ((INSTANCE) == GPIOD) || \
                                           ((INSTANCE) == GPIOE) || \
                                           ((INSTANCE) == GPIOF) || \
                                           ((INSTANCE) == GPIOG) || \
                                           ((INSTANCE) == GPIOH) || \
                                           ((INSTANCE) == GPIOL) || \
                                           ((GPIO_DBG_TypeDef *)(INSTANCE) == GPIO_DBG))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* GPIO_MODE */
#define GPIO_MODE_MODE0                      (BITS(0,1))                                         /*!< mode control bits of Px00 */
#define GPIO_MODE_MODE1                      (BITS(2,3))                                         /*!< mode control bits of Px01 */
#define GPIO_MODE_MODE2                      (BITS(4,5))                                         /*!< mode control bits of Px02 */
#define GPIO_MODE_MODE3                      (BITS(6,7))                                         /*!< mode control bits of Px03 */
#define GPIO_MODE_MODE4                      (BITS(8,9))                                         /*!< mode control bits of Px04 */
#define GPIO_MODE_MODE5                      (BITS(10,11))                                       /*!< mode control bits of Px05 */
#define GPIO_MODE_MODE6                      (BITS(12,13))                                       /*!< mode control bits of Px06 */
#define GPIO_MODE_MODE7                      (BITS(14,15))                                       /*!< mode control bits of Px07 */
#define GPIO_MODE_MODE8                      (BITS(16,17))                                       /*!< mode control bits of Px08 */
#define GPIO_MODE_MODE9                      (BITS(18,19))                                       /*!< mode control bits of Px09 */
#define GPIO_MODE_MODE10                     (BITS(20,21))                                       /*!< mode control bits of Px10 */
#define GPIO_MODE_MODE11                     (BITS(22,23))                                       /*!< mode control bits of Px11 */
#define GPIO_MODE_MODE12                     (BITS(24,25))                                       /*!< mode control bits of Px12 */
#define GPIO_MODE_MODE13                     (BITS(26,27))                                       /*!< mode control bits of Px13 */
#define GPIO_MODE_MODE14                     (BITS(28,29))                                       /*!< mode control bits of Px14 */
#define GPIO_MODE_MODE15                     (BITS(30,31))                                       /*!< mode control bits of Px15 */

/* GPIO_AEN */
#define GPIO_AEN_ANA_EN0                     (BIT(0))                                            /*!< Analog funtion enable bit of Px00 */
#define GPIO_AEN_ANA_EN1                     (BIT(1))                                            /*!< Analog funtion enable bit of Px01 */
#define GPIO_AEN_ANA_EN2                     (BIT(2))                                            /*!< Analog funtion enable bit of Px02 */
#define GPIO_AEN_ANA_EN3                     (BIT(3))                                            /*!< Analog funtion enable bit of Px03 */
#define GPIO_AEN_ANA_EN4                     (BIT(4))                                            /*!< Analog funtion enable bit of Px04 */
#define GPIO_AEN_ANA_EN5                     (BIT(5))                                            /*!< Analog funtion enable bit of Px05 */
#define GPIO_AEN_ANA_EN6                     (BIT(6))                                            /*!< Analog funtion enable bit of Px06 */
#define GPIO_AEN_ANA_EN7                     (BIT(7))                                            /*!< Analog funtion enable bit of Px07 */
#define GPIO_AEN_ANA_EN8                     (BIT(8))                                            /*!< Analog funtion enable bit of Px08 */
#define GPIO_AEN_ANA_EN9                     (BIT(9))                                            /*!< Analog funtion enable bit of Px09 */
#define GPIO_AEN_ANA_EN10                    (BIT(10))                                           /*!< Analog funtion enable bit of Px10 */
#define GPIO_AEN_ANA_EN11                    (BIT(11))                                           /*!< Analog funtion enable bit of Px11 */
#define GPIO_AEN_ANA_EN12                    (BIT(12))                                           /*!< Analog funtion enable bit of Px12 */
#define GPIO_AEN_ANA_EN13                    (BIT(13))                                           /*!< Analog funtion enable bit of Px13 */
#define GPIO_AEN_ANA_EN14                    (BIT(14))                                           /*!< Analog funtion enable bit of Px14 */
#define GPIO_AEN_ANA_EN15                    (BIT(15))                                           /*!< Analog funtion enable bit of Px15 */

/* GPIO_OTYPE */
#define GPIO_OTYPE_OTYPE0                    (BIT(0))                                            /*!< Output type control bit of Px00 */
#define GPIO_OTYPE_OTYPE1                    (BIT(1))                                            /*!< Output type control bit of Px01 */
#define GPIO_OTYPE_OTYPE2                    (BIT(2))                                            /*!< Output type control bit of Px02 */
#define GPIO_OTYPE_OTYPE3                    (BIT(3))                                            /*!< Output type control bit of Px03 */
#define GPIO_OTYPE_OTYPE4                    (BIT(4))                                            /*!< Output type control bit of Px04 */
#define GPIO_OTYPE_OTYPE5                    (BIT(5))                                            /*!< Output type control bit of Px05 */
#define GPIO_OTYPE_OTYPE6                    (BIT(6))                                            /*!< Output type control bit of Px06 */
#define GPIO_OTYPE_OTYPE7                    (BIT(7))                                            /*!< Output type control bit of Px07 */
#define GPIO_OTYPE_OTYPE8                    (BIT(8))                                            /*!< Output type control bit of Px08 */
#define GPIO_OTYPE_OTYPE9                    (BIT(9))                                            /*!< Output type control bit of Px09 */
#define GPIO_OTYPE_OTYPE10                   (BIT(10))                                           /*!< Output type control bit of Px10 */
#define GPIO_OTYPE_OTYPE11                   (BIT(11))                                           /*!< Output type control bit of Px11 */
#define GPIO_OTYPE_OTYPE12                   (BIT(12))                                           /*!< Output type control bit of Px12 */
#define GPIO_OTYPE_OTYPE13                   (BIT(13))                                           /*!< Output type control bit of Px13 */
#define GPIO_OTYPE_OTYPE14                   (BIT(14))                                           /*!< Output type control bit of Px14 */
#define GPIO_OTYPE_OTYPE15                   (BIT(15))                                           /*!< Output type control bit of Px15 */

/* GPIO_OSPEED */
#define GPIO_OSPEED_OSPEED0                  (BIT(0))                                            /*!< output speed control bit of Px00 */
#define GPIO_OSPEED_OSPEED1                  (BIT(1))                                            /*!< output speed control bit of Px01 */
#define GPIO_OSPEED_OSPEED2                  (BIT(2))                                            /*!< output speed control bit of Px02 */
#define GPIO_OSPEED_OSPEED3                  (BIT(3))                                            /*!< output speed control bit of Px03 */
#define GPIO_OSPEED_OSPEED4                  (BIT(4))                                            /*!< output speed control bit of Px04 */
#define GPIO_OSPEED_OSPEED5                  (BIT(5))                                            /*!< output speed control bit of Px05 */
#define GPIO_OSPEED_OSPEED6                  (BIT(6))                                            /*!< output speed control bit of Px06 */
#define GPIO_OSPEED_OSPEED7                  (BIT(7))                                            /*!< output speed control bit of Px07 */
#define GPIO_OSPEED_OSPEED8                  (BIT(8))                                            /*!< output speed control bit of Px08 */
#define GPIO_OSPEED_OSPEED9                  (BIT(9))                                            /*!< output speed control bit of Px09 */
#define GPIO_OSPEED_OSPEED10                 (BIT(10))                                           /*!< output speed control bit of Px10 */
#define GPIO_OSPEED_OSPEED11                 (BIT(11))                                           /*!< output speed control bit of Px11 */
#define GPIO_OSPEED_OSPEED12                 (BIT(12))                                           /*!< output speed control bit of Px12 */
#define GPIO_OSPEED_OSPEED13                 (BIT(13))                                           /*!< output speed control bit of Px13 */
#define GPIO_OSPEED_OSPEED14                 (BIT(14))                                           /*!< output speed control bit of Px14 */
#define GPIO_OSPEED_OSPEED15                 (BIT(15))                                           /*!< output speed control bit of Px15 */

/* GPIO_FIEN */
#define GPIO_FIEN_FIEN0                      (BIT(0))                                            /*!< Force input enable bit of Px00 */
#define GPIO_FIEN_FIEN1                      (BIT(1))                                            /*!< Force input enable bit of Px01 */
#define GPIO_FIEN_FIEN2                      (BIT(2))                                            /*!< Force input enable bit of Px02 */
#define GPIO_FIEN_FIEN3                      (BIT(3))                                            /*!< Force input enable bit of Px03 */
#define GPIO_FIEN_FIEN4                      (BIT(4))                                            /*!< Force input enable bit of Px04 */
#define GPIO_FIEN_FIEN5                      (BIT(5))                                            /*!< Force input enable bit of Px05 */
#define GPIO_FIEN_FIEN6                      (BIT(6))                                            /*!< Force input enable bit of Px06 */
#define GPIO_FIEN_FIEN7                      (BIT(7))                                            /*!< Force input enable bit of Px07 */
#define GPIO_FIEN_FIEN8                      (BIT(8))                                            /*!< Force input enable bit of Px08 */
#define GPIO_FIEN_FIEN9                      (BIT(9))                                            /*!< Force input enable bit of Px09 */
#define GPIO_FIEN_FIEN10                     (BIT(10))                                           /*!< Force input enable bit of Px10 */
#define GPIO_FIEN_FIEN11                     (BIT(11))                                           /*!< Force input enable bit of Px11 */
#define GPIO_FIEN_FIEN12                     (BIT(12))                                           /*!< Force input enable bit of Px12 */
#define GPIO_FIEN_FIEN13                     (BIT(13))                                           /*!< Force input enable bit of Px13 */
#define GPIO_FIEN_FIEN14                     (BIT(14))                                           /*!< Force input enable bit of Px14 */
#define GPIO_FIEN_FIEN15                     (BIT(15))                                           /*!< Force input enable bit of Px15 */

/* GPIO_ITYPE */
#define GPIO_ITYPE_ITYPE0                    (BIT(0))                                            /*!< Input type control bit of Px00 */
#define GPIO_ITYPE_ITYPE1                    (BIT(1))                                            /*!< Input type control bit of Px01 */
#define GPIO_ITYPE_ITYPE2                    (BIT(2))                                            /*!< Input type control bit of Px02 */
#define GPIO_ITYPE_ITYPE3                    (BIT(3))                                            /*!< Input type control bit of Px03 */
#define GPIO_ITYPE_ITYPE4                    (BIT(4))                                            /*!< Input type control bit of Px04 */
#define GPIO_ITYPE_ITYPE5                    (BIT(5))                                            /*!< Input type control bit of Px05 */
#define GPIO_ITYPE_ITYPE6                    (BIT(6))                                            /*!< Input type control bit of Px06 */
#define GPIO_ITYPE_ITYPE7                    (BIT(7))                                            /*!< Input type control bit of Px07 */
#define GPIO_ITYPE_ITYPE8                    (BIT(8))                                            /*!< Input type control bit of Px08 */
#define GPIO_ITYPE_ITYPE9                    (BIT(9))                                            /*!< Input type control bit of Px09 */
#define GPIO_ITYPE_ITYPE10                   (BIT(10))                                           /*!< Input type control bit of Px10 */
#define GPIO_ITYPE_ITYPE11                   (BIT(11))                                           /*!< Input type control bit of Px11 */
#define GPIO_ITYPE_ITYPE12                   (BIT(12))                                           /*!< Input type control bit of Px12 */
#define GPIO_ITYPE_ITYPE13                   (BIT(13))                                           /*!< Input type control bit of Px13 */
#define GPIO_ITYPE_ITYPE14                   (BIT(14))                                           /*!< Input type control bit of Px14 */
#define GPIO_ITYPE_ITYPE15                   (BIT(15))                                           /*!< Input type control bit of Px15 */

/* GPIO_UPDN */
#define GPIO_UPDN_UPDN0                      (BITS(0,1))                                         /*!< Pull up/pull down control bits of Px00 */
#define GPIO_UPDN_UPDN1                      (BITS(2,3))                                         /*!< Pull up/pull down control bits of Px01 */
#define GPIO_UPDN_UPDN2                      (BITS(4,5))                                         /*!< Pull up/pull down control bits of Px02 */
#define GPIO_UPDN_UPDN3                      (BITS(6,7))                                         /*!< Pull up/pull down control bits of Px03 */
#define GPIO_UPDN_UPDN4                      (BITS(8,9))                                         /*!< Pull up/pull down control bits of Px04 */
#define GPIO_UPDN_UPDN5                      (BITS(10,11))                                       /*!< Pull up/pull down control bits of Px05 */
#define GPIO_UPDN_UPDN6                      (BITS(12,13))                                       /*!< Pull up/pull down control bits of Px06 */
#define GPIO_UPDN_UPDN7                      (BITS(14,15))                                       /*!< Pull up/pull down control bits of Px07 */
#define GPIO_UPDN_UPDN8                      (BITS(16,17))                                       /*!< Pull up/pull down control bits of Px08 */
#define GPIO_UPDN_UPDN9                      (BITS(18,19))                                       /*!< Pull up/pull down control bits of Px09 */
#define GPIO_UPDN_UPDN10                     (BITS(20,21))                                       /*!< Pull up/pull down control bits of Px10 */
#define GPIO_UPDN_UPDN11                     (BITS(22,23))                                       /*!< Pull up/pull down control bits of Px11 */
#define GPIO_UPDN_UPDN12                     (BITS(24,25))                                       /*!< Pull up/pull down control bits of Px12 */
#define GPIO_UPDN_UPDN13                     (BITS(26,27))                                       /*!< Pull up/pull down control bits of Px13 */
#define GPIO_UPDN_UPDN14                     (BITS(28,29))                                       /*!< Pull up/pull down control bits of Px14 */
#define GPIO_UPDN_UPDN15                     (BITS(30,31))                                       /*!< Pull up/pull down control bits of Px15 */

/* GPIO_IDATA */
#define GPIO_IDATA_IDATA0                    (BIT(0))                                            /*!< Input data bit of Px00 */
#define GPIO_IDATA_IDATA1                    (BIT(1))                                            /*!< Input data bit of Px01 */
#define GPIO_IDATA_IDATA2                    (BIT(2))                                            /*!< Input data bit of Px02 */
#define GPIO_IDATA_IDATA3                    (BIT(3))                                            /*!< Input data bit of Px03 */
#define GPIO_IDATA_IDATA4                    (BIT(4))                                            /*!< Input data bit of Px04 */
#define GPIO_IDATA_IDATA5                    (BIT(5))                                            /*!< Input data bit of Px05 */
#define GPIO_IDATA_IDATA6                    (BIT(6))                                            /*!< Input data bit of Px06 */
#define GPIO_IDATA_IDATA7                    (BIT(7))                                            /*!< Input data bit of Px07 */
#define GPIO_IDATA_IDATA8                    (BIT(8))                                            /*!< Input data bit of Px08 */
#define GPIO_IDATA_IDATA9                    (BIT(9))                                            /*!< Input data bit of Px09 */
#define GPIO_IDATA_IDATA10                   (BIT(10))                                           /*!< Input data bit of Px10 */
#define GPIO_IDATA_IDATA11                   (BIT(11))                                           /*!< Input data bit of Px11 */
#define GPIO_IDATA_IDATA12                   (BIT(12))                                           /*!< Input data bit of Px12 */
#define GPIO_IDATA_IDATA13                   (BIT(13))                                           /*!< Input data bit of Px13 */
#define GPIO_IDATA_IDATA14                   (BIT(14))                                           /*!< Input data bit of Px14 */
#define GPIO_IDATA_IDATA15                   (BIT(15))                                           /*!< Input data bit of Px15 */

/* GPIO_ODATA */
#define GPIO_ODATA_ODATA0                    (BIT(0))                                            /*!< Output data of Px00 */
#define GPIO_ODATA_ODATA1                    (BIT(1))                                            /*!< Output data of Px01 */
#define GPIO_ODATA_ODATA2                    (BIT(2))                                            /*!< Output data of Px02 */
#define GPIO_ODATA_ODATA3                    (BIT(3))                                            /*!< Output data of Px03 */
#define GPIO_ODATA_ODATA4                    (BIT(4))                                            /*!< Output data of Px04 */
#define GPIO_ODATA_ODATA5                    (BIT(5))                                            /*!< Output data of Px05 */
#define GPIO_ODATA_ODATA6                    (BIT(6))                                            /*!< Output data of Px06 */
#define GPIO_ODATA_ODATA7                    (BIT(7))                                            /*!< Output data of Px07 */
#define GPIO_ODATA_ODATA8                    (BIT(8))                                            /*!< Output data of Px08 */
#define GPIO_ODATA_ODATA9                    (BIT(9))                                            /*!< Output data of Px09 */
#define GPIO_ODATA_ODATA10                   (BIT(10))                                           /*!< Output data of Px10 */
#define GPIO_ODATA_ODATA11                   (BIT(11))                                           /*!< Output data of Px11 */
#define GPIO_ODATA_ODATA12                   (BIT(12))                                           /*!< Output data of Px12 */
#define GPIO_ODATA_ODATA13                   (BIT(13))                                           /*!< Output data of Px13 */
#define GPIO_ODATA_ODATA14                   (BIT(14))                                           /*!< Output data of Px14 */
#define GPIO_ODATA_ODATA15                   (BIT(15))                                           /*!< Output data of Px15 */

/* GPIO_AFSEL0 */
#define GPIO_AFSEL0_SEL0                     (BITS(0,3))                                         /*!< Alternate function bits of Px00 */
#define GPIO_AFSEL0_SEL1                     (BITS(4,7))                                         /*!< Alternate function bits of Px01 */
#define GPIO_AFSEL0_SEL2                     (BITS(8,11))                                        /*!< Alternate function bits of Px02 */
#define GPIO_AFSEL0_SEL3                     (BITS(12,15))                                       /*!< Alternate function bits of Px03 */
#define GPIO_AFSEL0_SEL4                     (BITS(16,19))                                       /*!< Alternate function bits of Px04 */
#define GPIO_AFSEL0_SEL5                     (BITS(20,23))                                       /*!< Alternate function bits of Px05 */
#define GPIO_AFSEL0_SEL6                     (BITS(24,27))                                       /*!< Alternate function bits of Px06 */
#define GPIO_AFSEL0_SEL7                     (BITS(28,31))                                       /*!< Alternate function bits of Px07 */

/* GPIO_AFSEL1 */
#define GPIO_AFSEL1_SEL8                     (BITS(0,3))                                         /*!< Alternate function bits of Px08 */
#define GPIO_AFSEL1_SEL9                     (BITS(4,7))                                         /*!< Alternate function bits of Px09 */
#define GPIO_AFSEL1_SEL10                    (BITS(8,11))                                        /*!< Alternate function bits of Px10 */
#define GPIO_AFSEL1_SEL11                    (BITS(12,15))                                       /*!< Alternate function bits of Px11 */
#define GPIO_AFSEL1_SEL12                    (BITS(16,19))                                       /*!< Alternate function bits of Px12 */
#define GPIO_AFSEL1_SEL13                    (BITS(20,23))                                       /*!< Alternate function bits of Px13 */
#define GPIO_AFSEL1_SEL14                    (BITS(24,27))                                       /*!< Alternate function bits of Px14 */
#define GPIO_AFSEL1_SEL15                    (BITS(28,31))                                       /*!< Alternate function bits of Px15 */

/* GPIO_SET */
#define GPIO_SET_SET0                        (BIT(0))                                            /*!< Bit set of Px00 */
#define GPIO_SET_SET1                        (BIT(1))                                            /*!< Bit set of Px01 */
#define GPIO_SET_SET2                        (BIT(2))                                            /*!< Bit set of Px02 */
#define GPIO_SET_SET3                        (BIT(3))                                            /*!< Bit set of Px03 */
#define GPIO_SET_SET4                        (BIT(4))                                            /*!< Bit set of Px04 */
#define GPIO_SET_SET5                        (BIT(5))                                            /*!< Bit set of Px05 */
#define GPIO_SET_SET6                        (BIT(6))                                            /*!< Bit set of Px06 */
#define GPIO_SET_SET7                        (BIT(7))                                            /*!< Bit set of Px07 */
#define GPIO_SET_SET8                        (BIT(8))                                            /*!< Bit set of Px08 */
#define GPIO_SET_SET9                        (BIT(9))                                            /*!< Bit set of Px09 */
#define GPIO_SET_SET10                       (BIT(10))                                           /*!< Bit set of Px10 */
#define GPIO_SET_SET11                       (BIT(11))                                           /*!< Bit set of Px11 */
#define GPIO_SET_SET12                       (BIT(12))                                           /*!< Bit set of Px12 */
#define GPIO_SET_SET13                       (BIT(13))                                           /*!< Bit set of Px13 */
#define GPIO_SET_SET14                       (BIT(14))                                           /*!< Bit set of Px14 */
#define GPIO_SET_SET15                       (BIT(15))                                           /*!< Bit set of Px15 */

/* GPIO_CLR */
#define GPIO_CLR_CLR0                        (BIT(0))                                            /*!< Bit clear of Px00 */
#define GPIO_CLR_CLR1                        (BIT(1))                                            /*!< Bit clear of Px01 */
#define GPIO_CLR_CLR2                        (BIT(2))                                            /*!< Bit clear of Px02 */
#define GPIO_CLR_CLR3                        (BIT(3))                                            /*!< Bit clear of Px03 */
#define GPIO_CLR_CLR4                        (BIT(4))                                            /*!< Bit clear of Px04 */
#define GPIO_CLR_CLR5                        (BIT(5))                                            /*!< Bit clear of Px05 */
#define GPIO_CLR_CLR6                        (BIT(6))                                            /*!< Bit clear of Px06 */
#define GPIO_CLR_CLR7                        (BIT(7))                                            /*!< Bit clear of Px07 */
#define GPIO_CLR_CLR8                        (BIT(8))                                            /*!< Bit clear of Px08 */
#define GPIO_CLR_CLR9                        (BIT(9))                                            /*!< Bit clear of Px09 */
#define GPIO_CLR_CLR10                       (BIT(10))                                           /*!< Bit clear of Px10 */
#define GPIO_CLR_CLR11                       (BIT(11))                                           /*!< Bit clear of Px11 */
#define GPIO_CLR_CLR12                       (BIT(12))                                           /*!< Bit clear of Px12 */
#define GPIO_CLR_CLR13                       (BIT(13))                                           /*!< Bit clear of Px13 */
#define GPIO_CLR_CLR14                       (BIT(14))                                           /*!< Bit clear of Px14 */
#define GPIO_CLR_CLR15                       (BIT(15))                                           /*!< Bit clear of Px15 */

/* GPIO_TG */
#define GPIO_TG_TG0                          (BIT(0))                                            /*!< Bit toggle of Px00 */
#define GPIO_TG_TG1                          (BIT(1))                                            /*!< Bit toggle of Px01 */
#define GPIO_TG_TG2                          (BIT(2))                                            /*!< Bit toggle of Px02 */
#define GPIO_TG_TG3                          (BIT(3))                                            /*!< Bit toggle of Px03 */
#define GPIO_TG_TG4                          (BIT(4))                                            /*!< Bit toggle of Px04 */
#define GPIO_TG_TG5                          (BIT(5))                                            /*!< Bit toggle of Px05 */
#define GPIO_TG_TG6                          (BIT(6))                                            /*!< Bit toggle of Px06 */
#define GPIO_TG_TG7                          (BIT(7))                                            /*!< Bit toggle of Px07 */
#define GPIO_TG_TG8                          (BIT(8))                                            /*!< Bit toggle of Px08 */
#define GPIO_TG_TG9                          (BIT(9))                                            /*!< Bit toggle of Px09 */
#define GPIO_TG_TG10                         (BIT(10))                                           /*!< Bit toggle of Px10 */
#define GPIO_TG_TG11                         (BIT(11))                                           /*!< Bit toggle of Px11 */
#define GPIO_TG_TG12                         (BIT(12))                                           /*!< Bit toggle of Px12 */
#define GPIO_TG_TG13                         (BIT(13))                                           /*!< Bit toggle of Px13 */
#define GPIO_TG_TG14                         (BIT(14))                                           /*!< Bit toggle of Px14 */
#define GPIO_TG_TG15                         (BIT(15))                                           /*!< Bit toggle of Px15 */

/* GPIO_DBG_KEY */
#define GPIO_DBG_KEY_KEY                     (BITS(0,31))                                        /*!< Key of debug register bit */

/* GPIO_DBG */
#define GPIO_DBG_JTDO_EN                     (BIT(3))                                            /*!< enable bit of JTDO */
#define GPIO_DBG_JTDI_EN                     (BIT(2))                                            /*!< enable bit of JTDI */
#define GPIO_DBG_JTMS_EN                     (BIT(1))                                            /*!< enable bit of JTMS */
#define GPIO_DBG_JTCK_EN                     (BIT(0))                                            /*!< enable bit of JTCK */

/* constants definitions */
/* GPIO_MODE_MODE0 */
#define GPIO_MODE_MODE0_POS                  (0U)                                                /*!< GPIO_MODE_MODE0 bit offset */
#define GPIO_MODE_MODE0_CFG(regval)  (GPIO_MODE_MODE0 & ((uint32_t)(regval) << GPIO_MODE_MODE0_POS))
#define GPIO_MODE_MODE0_INPUT                (GPIO_MODE_MODE0_CFG(0))                            /*!< Input mode */
#define GPIO_MODE_MODE0_OUTPUT               (GPIO_MODE_MODE0_CFG(1))                            /*!< Output mode */
#define GPIO_MODE_MODE0_ALTERNATE            (GPIO_MODE_MODE0_CFG(2))                            /*!< Alternate funtion mode */
#define GPIO_MODE_MODE0_ANALOG               (GPIO_MODE_MODE0_CFG(3))                            /*!< Analog mode */

/* GPIO_MODE_MODE1 */
#define GPIO_MODE_MODE1_POS                  (2U)                                                /*!< GPIO_MODE_MODE1 bit offset */
#define GPIO_MODE_MODE1_CFG(regval)  (GPIO_MODE_MODE1 & ((uint32_t)(regval) << GPIO_MODE_MODE1_POS))
#define GPIO_MODE_MODE1_INPUT                (GPIO_MODE_MODE1_CFG(0))                            /*!< Input mode */
#define GPIO_MODE_MODE1_OUTPUT               (GPIO_MODE_MODE1_CFG(1))                            /*!< Output mode */
#define GPIO_MODE_MODE1_ALTERNATE            (GPIO_MODE_MODE1_CFG(2))                            /*!< Alternate funtion mode */
#define GPIO_MODE_MODE1_ANALOG               (GPIO_MODE_MODE1_CFG(3))                            /*!< Analog mode */

/* GPIO_MODE_MODE2 */
#define GPIO_MODE_MODE2_POS                  (4U)                                                /*!< GPIO_MODE_MODE2 bit offset */
#define GPIO_MODE_MODE2_CFG(regval)  (GPIO_MODE_MODE2 & ((uint32_t)(regval) << GPIO_MODE_MODE2_POS))
#define GPIO_MODE_MODE2_INPUT                (GPIO_MODE_MODE2_CFG(0))                            /*!< Input mode */
#define GPIO_MODE_MODE2_OUTPUT               (GPIO_MODE_MODE2_CFG(1))                            /*!< Output mode */
#define GPIO_MODE_MODE2_ALTERNATE            (GPIO_MODE_MODE2_CFG(2))                            /*!< Alternate funtion mode */
#define GPIO_MODE_MODE2_ANALOG               (GPIO_MODE_MODE2_CFG(3))                            /*!< Analog mode */

/* GPIO_MODE_MODE3 */
#define GPIO_MODE_MODE3_POS                  (6U)                                                /*!< GPIO_MODE_MODE3 bit offset */
#define GPIO_MODE_MODE3_CFG(regval)  (GPIO_MODE_MODE3 & ((uint32_t)(regval) << GPIO_MODE_MODE3_POS))
#define GPIO_MODE_MODE3_INPUT                (GPIO_MODE_MODE3_CFG(0))                            /*!< Input mode */
#define GPIO_MODE_MODE3_OUTPUT               (GPIO_MODE_MODE3_CFG(1))                            /*!< Output mode */
#define GPIO_MODE_MODE3_ALTERNATE            (GPIO_MODE_MODE3_CFG(2))                            /*!< Alternate funtion mode */
#define GPIO_MODE_MODE3_ANALOG               (GPIO_MODE_MODE3_CFG(3))                            /*!< Analog mode */

/* GPIO_MODE_MODE4 */
#define GPIO_MODE_MODE4_POS                  (8U)                                                /*!< GPIO_MODE_MODE4 bit offset */
#define GPIO_MODE_MODE4_CFG(regval)  (GPIO_MODE_MODE4 & ((uint32_t)(regval) << GPIO_MODE_MODE4_POS))
#define GPIO_MODE_MODE4_INPUT                (GPIO_MODE_MODE4_CFG(0))                            /*!< Input mode */
#define GPIO_MODE_MODE4_OUTPUT               (GPIO_MODE_MODE4_CFG(1))                            /*!< Output mode */
#define GPIO_MODE_MODE4_ALTERNATE            (GPIO_MODE_MODE4_CFG(2))                            /*!< Alternate funtion mode */
#define GPIO_MODE_MODE4_ANALOG               (GPIO_MODE_MODE4_CFG(3))                            /*!< Analog mode */

/* GPIO_MODE_MODE5 */
#define GPIO_MODE_MODE5_POS                  (10U)                                               /*!< GPIO_MODE_MODE5 bit offset */
#define GPIO_MODE_MODE5_CFG(regval)  (GPIO_MODE_MODE5 & ((uint32_t)(regval) << GPIO_MODE_MODE5_POS))
#define GPIO_MODE_MODE5_INPUT                (GPIO_MODE_MODE5_CFG(0))                            /*!< Input mode */
#define GPIO_MODE_MODE5_OUTPUT               (GPIO_MODE_MODE5_CFG(1))                            /*!< Output mode */
#define GPIO_MODE_MODE5_ALTERNATE            (GPIO_MODE_MODE5_CFG(2))                            /*!< Alternate funtion mode */
#define GPIO_MODE_MODE5_ANALOG               (GPIO_MODE_MODE5_CFG(3))                            /*!< Analog mode */

/* GPIO_MODE_MODE6 */
#define GPIO_MODE_MODE6_POS                  (12U)                                               /*!< GPIO_MODE_MODE6 bit offset */
#define GPIO_MODE_MODE6_CFG(regval)  (GPIO_MODE_MODE6 & ((uint32_t)(regval) << GPIO_MODE_MODE6_POS))
#define GPIO_MODE_MODE6_INPUT                (GPIO_MODE_MODE6_CFG(0))                            /*!< Input mode */
#define GPIO_MODE_MODE6_OUTPUT               (GPIO_MODE_MODE6_CFG(1))                            /*!< Output mode */
#define GPIO_MODE_MODE6_ALTERNATE            (GPIO_MODE_MODE6_CFG(2))                            /*!< Alternate funtion mode */
#define GPIO_MODE_MODE6_ANALOG               (GPIO_MODE_MODE6_CFG(3))                            /*!< Analog mode */

/* GPIO_MODE_MODE7 */
#define GPIO_MODE_MODE7_POS                  (14U)                                               /*!< GPIO_MODE_MODE7 bit offset */
#define GPIO_MODE_MODE7_CFG(regval)  (GPIO_MODE_MODE7 & ((uint32_t)(regval) << GPIO_MODE_MODE7_POS))
#define GPIO_MODE_MODE7_INPUT                (GPIO_MODE_MODE7_CFG(0))                            /*!< Input mode */
#define GPIO_MODE_MODE7_OUTPUT               (GPIO_MODE_MODE7_CFG(1))                            /*!< Output mode */
#define GPIO_MODE_MODE7_ALTERNATE            (GPIO_MODE_MODE7_CFG(2))                            /*!< Alternate funtion mode */
#define GPIO_MODE_MODE7_ANALOG               (GPIO_MODE_MODE7_CFG(3))                            /*!< Analog mode */

/* GPIO_MODE_MODE8 */
#define GPIO_MODE_MODE8_POS                  (16U)                                               /*!< GPIO_MODE_MODE8 bit offset */
#define GPIO_MODE_MODE8_CFG(regval)  (GPIO_MODE_MODE8 & ((uint32_t)(regval) << GPIO_MODE_MODE8_POS))
#define GPIO_MODE_MODE8_INPUT                (GPIO_MODE_MODE8_CFG(0))                            /*!< Input mode */
#define GPIO_MODE_MODE8_OUTPUT               (GPIO_MODE_MODE8_CFG(1))                            /*!< Output mode */
#define GPIO_MODE_MODE8_ALTERNATE            (GPIO_MODE_MODE8_CFG(2))                            /*!< Alternate funtion mode */
#define GPIO_MODE_MODE8_ANALOG               (GPIO_MODE_MODE8_CFG(3))                            /*!< Analog mode */

/* GPIO_MODE_MODE9 */
#define GPIO_MODE_MODE9_POS                  (18U)                                               /*!< GPIO_MODE_MODE9 bit offset */
#define GPIO_MODE_MODE9_CFG(regval)  (GPIO_MODE_MODE9 & ((uint32_t)(regval) << GPIO_MODE_MODE9_POS))
#define GPIO_MODE_MODE9_INPUT                (GPIO_MODE_MODE9_CFG(0))                            /*!< Input mode */
#define GPIO_MODE_MODE9_OUTPUT               (GPIO_MODE_MODE9_CFG(1))                            /*!< Output mode */
#define GPIO_MODE_MODE9_ALTERNATE            (GPIO_MODE_MODE9_CFG(2))                            /*!< Alternate funtion mode */
#define GPIO_MODE_MODE9_ANALOG               (GPIO_MODE_MODE9_CFG(3))                            /*!< Analog mode */

/* GPIO_MODE_MODE10 */
#define GPIO_MODE_MODE10_POS                 (20U)                                               /*!< GPIO_MODE_MODE10 bit offset */
#define GPIO_MODE_MODE10_CFG(regval)  (GPIO_MODE_MODE10 & ((uint32_t)(regval) << GPIO_MODE_MODE10_POS))
#define GPIO_MODE_MODE10_INPUT               (GPIO_MODE_MODE10_CFG(0))                           /*!< Input mode */
#define GPIO_MODE_MODE10_OUTPUT              (GPIO_MODE_MODE10_CFG(1))                           /*!< Output mode */
#define GPIO_MODE_MODE10_ALTERNATE           (GPIO_MODE_MODE10_CFG(2))                           /*!< Alternate funtion mode */
#define GPIO_MODE_MODE10_ANALOG              (GPIO_MODE_MODE10_CFG(3))                           /*!< Analog mode */

/* GPIO_MODE_MODE11 */
#define GPIO_MODE_MODE11_POS                 (22U)                                               /*!< GPIO_MODE_MODE11 bit offset */
#define GPIO_MODE_MODE11_CFG(regval)  (GPIO_MODE_MODE11 & ((uint32_t)(regval) << GPIO_MODE_MODE11_POS))
#define GPIO_MODE_MODE11_INPUT               (GPIO_MODE_MODE11_CFG(0))                           /*!< Input mode */
#define GPIO_MODE_MODE11_OUTPUT              (GPIO_MODE_MODE11_CFG(1))                           /*!< Output mode */
#define GPIO_MODE_MODE11_ALTERNATE           (GPIO_MODE_MODE11_CFG(2))                           /*!< Alternate funtion mode */
#define GPIO_MODE_MODE11_ANALOG              (GPIO_MODE_MODE11_CFG(3))                           /*!< Analog mode */

/* GPIO_MODE_MODE12 */
#define GPIO_MODE_MODE12_POS                 (24U)                                               /*!< GPIO_MODE_MODE12 bit offset */
#define GPIO_MODE_MODE12_CFG(regval)  (GPIO_MODE_MODE12 & ((uint32_t)(regval) << GPIO_MODE_MODE12_POS))
#define GPIO_MODE_MODE12_INPUT               (GPIO_MODE_MODE12_CFG(0))                           /*!< Input mode */
#define GPIO_MODE_MODE12_OUTPUT              (GPIO_MODE_MODE12_CFG(1))                           /*!< Output mode */
#define GPIO_MODE_MODE12_ALTERNATE           (GPIO_MODE_MODE12_CFG(2))                           /*!< Alternate funtion mode */
#define GPIO_MODE_MODE12_ANALOG              (GPIO_MODE_MODE12_CFG(3))                           /*!< Analog mode */

/* GPIO_MODE_MODE13 */
#define GPIO_MODE_MODE13_POS                 (26U)                                               /*!< GPIO_MODE_MODE13 bit offset */
#define GPIO_MODE_MODE13_CFG(regval)  (GPIO_MODE_MODE13 & ((uint32_t)(regval) << GPIO_MODE_MODE13_POS))
#define GPIO_MODE_MODE13_INPUT               (GPIO_MODE_MODE13_CFG(0))                           /*!< Input mode */
#define GPIO_MODE_MODE13_OUTPUT              (GPIO_MODE_MODE13_CFG(1))                           /*!< Output mode */
#define GPIO_MODE_MODE13_ALTERNATE           (GPIO_MODE_MODE13_CFG(2))                           /*!< Alternate funtion mode */
#define GPIO_MODE_MODE13_ANALOG              (GPIO_MODE_MODE13_CFG(3))                           /*!< Analog mode */

/* GPIO_MODE_MODE14 */
#define GPIO_MODE_MODE14_POS                 (28U)                                               /*!< GPIO_MODE_MODE14 bit offset */
#define GPIO_MODE_MODE14_CFG(regval)  (GPIO_MODE_MODE14 & ((uint32_t)(regval) << GPIO_MODE_MODE14_POS))
#define GPIO_MODE_MODE14_INPUT               (GPIO_MODE_MODE14_CFG(0))                           /*!< Input mode */
#define GPIO_MODE_MODE14_OUTPUT              (GPIO_MODE_MODE14_CFG(1))                           /*!< Output mode */
#define GPIO_MODE_MODE14_ALTERNATE           (GPIO_MODE_MODE14_CFG(2))                           /*!< Alternate funtion mode */
#define GPIO_MODE_MODE14_ANALOG              (GPIO_MODE_MODE14_CFG(3))                           /*!< Analog mode */

/* GPIO_MODE_MODE15 */
#define GPIO_MODE_MODE15_POS                 (30U)                                               /*!< GPIO_MODE_MODE15 bit offset */
#define GPIO_MODE_MODE15_CFG(regval)  (GPIO_MODE_MODE15 & ((uint32_t)(regval) << GPIO_MODE_MODE15_POS))
#define GPIO_MODE_MODE15_INPUT               (GPIO_MODE_MODE15_CFG(0))                           /*!< Input mode */
#define GPIO_MODE_MODE15_OUTPUT              (GPIO_MODE_MODE15_CFG(1))                           /*!< Output mode */
#define GPIO_MODE_MODE15_ALTERNATE           (GPIO_MODE_MODE15_CFG(2))                           /*!< Alternate funtion mode */
#define GPIO_MODE_MODE15_ANALOG              (GPIO_MODE_MODE15_CFG(3))                           /*!< Analog mode */

/* GPIO_AEN_ANA_EN0 */
#define GPIO_AEN_ANA_EN0_POS                 (0U)                                                /*!< GPIO_AEN_ANA_EN0 bit offset */

/* GPIO_AEN_ANA_EN1 */
#define GPIO_AEN_ANA_EN1_POS                 (1U)                                                /*!< GPIO_AEN_ANA_EN1 bit offset */

/* GPIO_AEN_ANA_EN2 */
#define GPIO_AEN_ANA_EN2_POS                 (2U)                                                /*!< GPIO_AEN_ANA_EN2 bit offset */

/* GPIO_AEN_ANA_EN3 */
#define GPIO_AEN_ANA_EN3_POS                 (3U)                                                /*!< GPIO_AEN_ANA_EN3 bit offset */

/* GPIO_AEN_ANA_EN4 */
#define GPIO_AEN_ANA_EN4_POS                 (4U)                                                /*!< GPIO_AEN_ANA_EN4 bit offset */

/* GPIO_AEN_ANA_EN5 */
#define GPIO_AEN_ANA_EN5_POS                 (5U)                                                /*!< GPIO_AEN_ANA_EN5 bit offset */

/* GPIO_AEN_ANA_EN6 */
#define GPIO_AEN_ANA_EN6_POS                 (6U)                                                /*!< GPIO_AEN_ANA_EN6 bit offset */

/* GPIO_AEN_ANA_EN7 */
#define GPIO_AEN_ANA_EN7_POS                 (7U)                                                /*!< GPIO_AEN_ANA_EN7 bit offset */

/* GPIO_AEN_ANA_EN8 */
#define GPIO_AEN_ANA_EN8_POS                 (8U)                                                /*!< GPIO_AEN_ANA_EN8 bit offset */

/* GPIO_AEN_ANA_EN9 */
#define GPIO_AEN_ANA_EN9_POS                 (9U)                                                /*!< GPIO_AEN_ANA_EN9 bit offset */

/* GPIO_AEN_ANA_EN10 */
#define GPIO_AEN_ANA_EN10_POS                (10U)                                               /*!< GPIO_AEN_ANA_EN10 bit offset */

/* GPIO_AEN_ANA_EN11 */
#define GPIO_AEN_ANA_EN11_POS                (11U)                                               /*!< GPIO_AEN_ANA_EN11 bit offset */

/* GPIO_AEN_ANA_EN12 */
#define GPIO_AEN_ANA_EN12_POS                (12U)                                               /*!< GPIO_AEN_ANA_EN12 bit offset */

/* GPIO_AEN_ANA_EN13 */
#define GPIO_AEN_ANA_EN13_POS                (13U)                                               /*!< GPIO_AEN_ANA_EN13 bit offset */

/* GPIO_AEN_ANA_EN14 */
#define GPIO_AEN_ANA_EN14_POS                (14U)                                               /*!< GPIO_AEN_ANA_EN14 bit offset */

/* GPIO_AEN_ANA_EN15 */
#define GPIO_AEN_ANA_EN15_POS                (15U)                                               /*!< GPIO_AEN_ANA_EN15 bit offset */

/* GPIO_OTYPE_OTYPE0 */
#define GPIO_OTYPE_OTYPE0_POS                (0U)                                                /*!< GPIO_OTYPE_OTYPE0 bit offset */
#define GPIO_OTYPE_OTYPE0_NORMAL             ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE0_OPENDRAIN          (GPIO_OTYPE_OTYPE0)                                 /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE1 */
#define GPIO_OTYPE_OTYPE1_POS                (1U)                                                /*!< GPIO_OTYPE_OTYPE1 bit offset */
#define GPIO_OTYPE_OTYPE1_NORMAL             ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE1_OPENDRAIN          (GPIO_OTYPE_OTYPE1)                                 /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE2 */
#define GPIO_OTYPE_OTYPE2_POS                (2U)                                                /*!< GPIO_OTYPE_OTYPE2 bit offset */
#define GPIO_OTYPE_OTYPE2_NORMAL             ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE2_OPENDRAIN          (GPIO_OTYPE_OTYPE2)                                 /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE3 */
#define GPIO_OTYPE_OTYPE3_POS                (3U)                                                /*!< GPIO_OTYPE_OTYPE3 bit offset */
#define GPIO_OTYPE_OTYPE3_NORMAL             ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE3_OPENDRAIN          (GPIO_OTYPE_OTYPE3)                                 /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE4 */
#define GPIO_OTYPE_OTYPE4_POS                (4U)                                                /*!< GPIO_OTYPE_OTYPE4 bit offset */
#define GPIO_OTYPE_OTYPE4_NORMAL             ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE4_OPENDRAIN          (GPIO_OTYPE_OTYPE4)                                 /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE5 */
#define GPIO_OTYPE_OTYPE5_POS                (5U)                                                /*!< GPIO_OTYPE_OTYPE5 bit offset */
#define GPIO_OTYPE_OTYPE5_NORMAL             ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE5_OPENDRAIN          (GPIO_OTYPE_OTYPE5)                                 /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE6 */
#define GPIO_OTYPE_OTYPE6_POS                (6U)                                                /*!< GPIO_OTYPE_OTYPE6 bit offset */
#define GPIO_OTYPE_OTYPE6_NORMAL             ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE6_OPENDRAIN          (GPIO_OTYPE_OTYPE6)                                 /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE7 */
#define GPIO_OTYPE_OTYPE7_POS                (7U)                                                /*!< GPIO_OTYPE_OTYPE7 bit offset */
#define GPIO_OTYPE_OTYPE7_NORMAL             ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE7_OPENDRAIN          (GPIO_OTYPE_OTYPE7)                                 /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE8 */
#define GPIO_OTYPE_OTYPE8_POS                (8U)                                                /*!< GPIO_OTYPE_OTYPE8 bit offset */
#define GPIO_OTYPE_OTYPE8_NORMAL             ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE8_OPENDRAIN          (GPIO_OTYPE_OTYPE8)                                 /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE9 */
#define GPIO_OTYPE_OTYPE9_POS                (9U)                                                /*!< GPIO_OTYPE_OTYPE9 bit offset */
#define GPIO_OTYPE_OTYPE9_NORMAL             ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE9_OPENDRAIN          (GPIO_OTYPE_OTYPE9)                                 /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE10 */
#define GPIO_OTYPE_OTYPE10_POS               (10U)                                               /*!< GPIO_OTYPE_OTYPE10 bit offset */
#define GPIO_OTYPE_OTYPE10_NORMAL            ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE10_OPENDRAIN         (GPIO_OTYPE_OTYPE10)                                /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE11 */
#define GPIO_OTYPE_OTYPE11_POS               (11U)                                               /*!< GPIO_OTYPE_OTYPE11 bit offset */
#define GPIO_OTYPE_OTYPE11_NORMAL            ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE11_OPENDRAIN         (GPIO_OTYPE_OTYPE11)                                /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE12 */
#define GPIO_OTYPE_OTYPE12_POS               (12U)                                               /*!< GPIO_OTYPE_OTYPE12 bit offset */
#define GPIO_OTYPE_OTYPE12_NORMAL            ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE12_OPENDRAIN         (GPIO_OTYPE_OTYPE12)                                /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE13 */
#define GPIO_OTYPE_OTYPE13_POS               (13U)                                               /*!< GPIO_OTYPE_OTYPE13 bit offset */
#define GPIO_OTYPE_OTYPE13_NORMAL            ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE13_OPENDRAIN         (GPIO_OTYPE_OTYPE13)                                /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE14 */
#define GPIO_OTYPE_OTYPE14_POS               (14U)                                               /*!< GPIO_OTYPE_OTYPE14 bit offset */
#define GPIO_OTYPE_OTYPE14_NORMAL            ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE14_OPENDRAIN         (GPIO_OTYPE_OTYPE14)                                /*!< open-drain output type */

/* GPIO_OTYPE_OTYPE15 */
#define GPIO_OTYPE_OTYPE15_POS               (15U)                                               /*!< GPIO_OTYPE_OTYPE15 bit offset */
#define GPIO_OTYPE_OTYPE15_NORMAL            ((uint32_t)0x00000000U)                             /*!< normal output type */
#define GPIO_OTYPE_OTYPE15_OPENDRAIN         (GPIO_OTYPE_OTYPE15)                                /*!< open-drain output type */

/* GPIO_OSPEED_OSPEED0 */
#define GPIO_OSPEED_OSPEED0_POS              (0U)                                                /*!< GPIO_OSPEED_OSPEED0 bit offset */
#define GPIO_OSPEED_OSPEED0_NORMAL           ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED0_HIGHSPEED        (GPIO_OSPEED_OSPEED0)                               /*!< High speed */

/* GPIO_OSPEED_OSPEED1 */
#define GPIO_OSPEED_OSPEED1_POS              (1U)                                                /*!< GPIO_OSPEED_OSPEED1 bit offset */
#define GPIO_OSPEED_OSPEED1_NORMAL           ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED1_HIGHSPEED        (GPIO_OSPEED_OSPEED1)                               /*!< High speed */

/* GPIO_OSPEED_OSPEED2 */
#define GPIO_OSPEED_OSPEED2_POS              (2U)                                                /*!< GPIO_OSPEED_OSPEED2 bit offset */
#define GPIO_OSPEED_OSPEED2_NORMAL           ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED2_HIGHSPEED        (GPIO_OSPEED_OSPEED2)                               /*!< High speed */

/* GPIO_OSPEED_OSPEED3 */
#define GPIO_OSPEED_OSPEED3_POS              (3U)                                                /*!< GPIO_OSPEED_OSPEED3 bit offset */
#define GPIO_OSPEED_OSPEED3_NORMAL           ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED3_HIGHSPEED        (GPIO_OSPEED_OSPEED3)                               /*!< High speed */

/* GPIO_OSPEED_OSPEED4 */
#define GPIO_OSPEED_OSPEED4_POS              (4U)                                                /*!< GPIO_OSPEED_OSPEED4 bit offset */
#define GPIO_OSPEED_OSPEED4_NORMAL           ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED4_HIGHSPEED        (GPIO_OSPEED_OSPEED4)                               /*!< High speed */

/* GPIO_OSPEED_OSPEED5 */
#define GPIO_OSPEED_OSPEED5_POS              (5U)                                                /*!< GPIO_OSPEED_OSPEED5 bit offset */
#define GPIO_OSPEED_OSPEED5_NORMAL           ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED5_HIGHSPEED        (GPIO_OSPEED_OSPEED5)                               /*!< High speed */

/* GPIO_OSPEED_OSPEED6 */
#define GPIO_OSPEED_OSPEED6_POS              (6U)                                                /*!< GPIO_OSPEED_OSPEED6 bit offset */
#define GPIO_OSPEED_OSPEED6_NORMAL           ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED6_HIGHSPEED        (GPIO_OSPEED_OSPEED6)                               /*!< High speed */

/* GPIO_OSPEED_OSPEED7 */
#define GPIO_OSPEED_OSPEED7_POS              (7U)                                                /*!< GPIO_OSPEED_OSPEED7 bit offset */
#define GPIO_OSPEED_OSPEED7_NORMAL           ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED7_HIGHSPEED        (GPIO_OSPEED_OSPEED7)                               /*!< High speed */

/* GPIO_OSPEED_OSPEED8 */
#define GPIO_OSPEED_OSPEED8_POS              (8U)                                                /*!< GPIO_OSPEED_OSPEED8 bit offset */
#define GPIO_OSPEED_OSPEED8_NORMAL           ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED8_HIGHSPEED        (GPIO_OSPEED_OSPEED8)                               /*!< High speed */

/* GPIO_OSPEED_OSPEED9 */
#define GPIO_OSPEED_OSPEED9_POS              (9U)                                                /*!< GPIO_OSPEED_OSPEED9 bit offset */
#define GPIO_OSPEED_OSPEED9_NORMAL           ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED9_HIGHSPEED        (GPIO_OSPEED_OSPEED9)                               /*!< High speed */

/* GPIO_OSPEED_OSPEED10 */
#define GPIO_OSPEED_OSPEED10_POS             (10U)                                               /*!< GPIO_OSPEED_OSPEED10 bit offset */
#define GPIO_OSPEED_OSPEED10_NORMAL          ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED10_HIGHSPEED       (GPIO_OSPEED_OSPEED10)                              /*!< High speed */

/* GPIO_OSPEED_OSPEED11 */
#define GPIO_OSPEED_OSPEED11_POS             (11U)                                               /*!< GPIO_OSPEED_OSPEED11 bit offset */
#define GPIO_OSPEED_OSPEED11_NORMAL          ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED11_HIGHSPEED       (GPIO_OSPEED_OSPEED11)                              /*!< High speed */

/* GPIO_OSPEED_OSPEED12 */
#define GPIO_OSPEED_OSPEED12_POS             (12U)                                               /*!< GPIO_OSPEED_OSPEED12 bit offset */
#define GPIO_OSPEED_OSPEED12_NORMAL          ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED12_HIGHSPEED       (GPIO_OSPEED_OSPEED12)                              /*!< High speed */

/* GPIO_OSPEED_OSPEED13 */
#define GPIO_OSPEED_OSPEED13_POS             (13U)                                               /*!< GPIO_OSPEED_OSPEED13 bit offset */
#define GPIO_OSPEED_OSPEED13_NORMAL          ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED13_HIGHSPEED       (GPIO_OSPEED_OSPEED13)                              /*!< High speed */

/* GPIO_OSPEED_OSPEED14 */
#define GPIO_OSPEED_OSPEED14_POS             (14U)                                               /*!< GPIO_OSPEED_OSPEED14 bit offset */
#define GPIO_OSPEED_OSPEED14_NORMAL          ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED14_HIGHSPEED       (GPIO_OSPEED_OSPEED14)                              /*!< High speed */

/* GPIO_OSPEED_OSPEED15 */
#define GPIO_OSPEED_OSPEED15_POS             (15U)                                               /*!< GPIO_OSPEED_OSPEED15 bit offset */
#define GPIO_OSPEED_OSPEED15_NORMAL          ((uint32_t)0x00000000U)                             /*!< Normal speed */
#define GPIO_OSPEED_OSPEED15_HIGHSPEED       (GPIO_OSPEED_OSPEED15)                              /*!< High speed */

/* GPIO_FIEN_FIEN0 */
#define GPIO_FIEN_FIEN0_POS                  (0U)                                                /*!< GPIO_FIEN_FIEN0 bit offset */

/* GPIO_FIEN_FIEN1 */
#define GPIO_FIEN_FIEN1_POS                  (1U)                                                /*!< GPIO_FIEN_FIEN1 bit offset */

/* GPIO_FIEN_FIEN2 */
#define GPIO_FIEN_FIEN2_POS                  (2U)                                                /*!< GPIO_FIEN_FIEN2 bit offset */

/* GPIO_FIEN_FIEN3 */
#define GPIO_FIEN_FIEN3_POS                  (3U)                                                /*!< GPIO_FIEN_FIEN3 bit offset */

/* GPIO_FIEN_FIEN4 */
#define GPIO_FIEN_FIEN4_POS                  (4U)                                                /*!< GPIO_FIEN_FIEN4 bit offset */

/* GPIO_FIEN_FIEN5 */
#define GPIO_FIEN_FIEN5_POS                  (5U)                                                /*!< GPIO_FIEN_FIEN5 bit offset */

/* GPIO_FIEN_FIEN6 */
#define GPIO_FIEN_FIEN6_POS                  (6U)                                                /*!< GPIO_FIEN_FIEN6 bit offset */

/* GPIO_FIEN_FIEN7 */
#define GPIO_FIEN_FIEN7_POS                  (7U)                                                /*!< GPIO_FIEN_FIEN7 bit offset */

/* GPIO_FIEN_FIEN8 */
#define GPIO_FIEN_FIEN8_POS                  (8U)                                                /*!< GPIO_FIEN_FIEN8 bit offset */

/* GPIO_FIEN_FIEN9 */
#define GPIO_FIEN_FIEN9_POS                  (9U)                                                /*!< GPIO_FIEN_FIEN9 bit offset */

/* GPIO_FIEN_FIEN10 */
#define GPIO_FIEN_FIEN10_POS                 (10U)                                               /*!< GPIO_FIEN_FIEN10 bit offset */

/* GPIO_FIEN_FIEN11 */
#define GPIO_FIEN_FIEN11_POS                 (11U)                                               /*!< GPIO_FIEN_FIEN11 bit offset */

/* GPIO_FIEN_FIEN12 */
#define GPIO_FIEN_FIEN12_POS                 (12U)                                               /*!< GPIO_FIEN_FIEN12 bit offset */

/* GPIO_FIEN_FIEN13 */
#define GPIO_FIEN_FIEN13_POS                 (13U)                                               /*!< GPIO_FIEN_FIEN13 bit offset */

/* GPIO_FIEN_FIEN14 */
#define GPIO_FIEN_FIEN14_POS                 (14U)                                               /*!< GPIO_FIEN_FIEN14 bit offset */

/* GPIO_FIEN_FIEN15 */
#define GPIO_FIEN_FIEN15_POS                 (15U)                                               /*!< GPIO_FIEN_FIEN15 bit offset */

/* GPIO_ITYPE_ITYPE0 */
#define GPIO_ITYPE_ITYPE0_POS                (0U)                                                /*!< GPIO_ITYPE_ITYPE0 bit offset */
#define GPIO_ITYPE_ITYPE0_CMOS               ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE0_TTL                (GPIO_ITYPE_ITYPE0)                                 /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE1 */
#define GPIO_ITYPE_ITYPE1_POS                (1U)                                                /*!< GPIO_ITYPE_ITYPE1 bit offset */
#define GPIO_ITYPE_ITYPE1_CMOS               ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE1_TTL                (GPIO_ITYPE_ITYPE1)                                 /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE2 */
#define GPIO_ITYPE_ITYPE2_POS                (2U)                                                /*!< GPIO_ITYPE_ITYPE2 bit offset */
#define GPIO_ITYPE_ITYPE2_CMOS               ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE2_TTL                (GPIO_ITYPE_ITYPE2)                                 /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE3 */
#define GPIO_ITYPE_ITYPE3_POS                (3U)                                                /*!< GPIO_ITYPE_ITYPE3 bit offset */
#define GPIO_ITYPE_ITYPE3_CMOS               ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE3_TTL                (GPIO_ITYPE_ITYPE3)                                 /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE4 */
#define GPIO_ITYPE_ITYPE4_POS                (4U)                                                /*!< GPIO_ITYPE_ITYPE4 bit offset */
#define GPIO_ITYPE_ITYPE4_CMOS               ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE4_TTL                (GPIO_ITYPE_ITYPE4)                                 /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE5 */
#define GPIO_ITYPE_ITYPE5_POS                (5U)                                                /*!< GPIO_ITYPE_ITYPE5 bit offset */
#define GPIO_ITYPE_ITYPE5_CMOS               ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE5_TTL                (GPIO_ITYPE_ITYPE5)                                 /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE6 */
#define GPIO_ITYPE_ITYPE6_POS                (6U)                                                /*!< GPIO_ITYPE_ITYPE6 bit offset */
#define GPIO_ITYPE_ITYPE6_CMOS               ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE6_TTL                (GPIO_ITYPE_ITYPE6)                                 /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE7 */
#define GPIO_ITYPE_ITYPE7_POS                (7U)                                                /*!< GPIO_ITYPE_ITYPE7 bit offset */
#define GPIO_ITYPE_ITYPE7_CMOS               ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE7_TTL                (GPIO_ITYPE_ITYPE7)                                 /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE8 */
#define GPIO_ITYPE_ITYPE8_POS                (8U)                                                /*!< GPIO_ITYPE_ITYPE8 bit offset */
#define GPIO_ITYPE_ITYPE8_CMOS               ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE8_TTL                (GPIO_ITYPE_ITYPE8)                                 /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE9 */
#define GPIO_ITYPE_ITYPE9_POS                (9U)                                                /*!< GPIO_ITYPE_ITYPE9 bit offset */
#define GPIO_ITYPE_ITYPE9_CMOS               ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE9_TTL                (GPIO_ITYPE_ITYPE9)                                 /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE10 */
#define GPIO_ITYPE_ITYPE10_POS               (10U)                                               /*!< GPIO_ITYPE_ITYPE10 bit offset */
#define GPIO_ITYPE_ITYPE10_CMOS              ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE10_TTL               (GPIO_ITYPE_ITYPE10)                                /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE11 */
#define GPIO_ITYPE_ITYPE11_POS               (11U)                                               /*!< GPIO_ITYPE_ITYPE11 bit offset */
#define GPIO_ITYPE_ITYPE11_CMOS              ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE11_TTL               (GPIO_ITYPE_ITYPE11)                                /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE12 */
#define GPIO_ITYPE_ITYPE12_POS               (12U)                                               /*!< GPIO_ITYPE_ITYPE12 bit offset */
#define GPIO_ITYPE_ITYPE12_CMOS              ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE12_TTL               (GPIO_ITYPE_ITYPE12)                                /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE13 */
#define GPIO_ITYPE_ITYPE13_POS               (13U)                                               /*!< GPIO_ITYPE_ITYPE13 bit offset */
#define GPIO_ITYPE_ITYPE13_CMOS              ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE13_TTL               (GPIO_ITYPE_ITYPE13)                                /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE14 */
#define GPIO_ITYPE_ITYPE14_POS               (14U)                                               /*!< GPIO_ITYPE_ITYPE14 bit offset */
#define GPIO_ITYPE_ITYPE14_CMOS              ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE14_TTL               (GPIO_ITYPE_ITYPE14)                                /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_ITYPE_ITYPE15 */
#define GPIO_ITYPE_ITYPE15_POS               (15U)                                               /*!< GPIO_ITYPE_ITYPE15 bit offset */
#define GPIO_ITYPE_ITYPE15_CMOS              ((uint32_t)0x00000000U)                             /*!< CMOS buffer, Vil=0.3Vcc, Vih=0.7Vcc */
#define GPIO_ITYPE_ITYPE15_TTL               (GPIO_ITYPE_ITYPE15)                                /*!< TTL buffer, Vil=0.16Vcc, Vih=0.4Vcc */

/* GPIO_UPDN_UPDN0 */
#define GPIO_UPDN_UPDN0_POS                  (0U)                                                /*!< GPIO_UPDN_UPDN0 bit offset */
#define GPIO_UPDN_UPDN0_CFG(regval)  (GPIO_UPDN_UPDN0 & ((uint32_t)(regval) << GPIO_UPDN_UPDN0_POS))
#define GPIO_UPDN_UPDN0_FLOATING             (GPIO_UPDN_UPDN0_CFG(0))                            /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN0_PULLUP               (GPIO_UPDN_UPDN0_CFG(1))                            /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN0_PULLDOWN             (GPIO_UPDN_UPDN0_CFG(2))                            /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN0_RESERVED             (GPIO_UPDN_UPDN0_CFG(3))                            /*!< Reserved */

/* GPIO_UPDN_UPDN1 */
#define GPIO_UPDN_UPDN1_POS                  (2U)                                                /*!< GPIO_UPDN_UPDN1 bit offset */
#define GPIO_UPDN_UPDN1_CFG(regval)  (GPIO_UPDN_UPDN1 & ((uint32_t)(regval) << GPIO_UPDN_UPDN1_POS))
#define GPIO_UPDN_UPDN1_FLOATING             (GPIO_UPDN_UPDN1_CFG(0))                            /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN1_PULLUP               (GPIO_UPDN_UPDN1_CFG(1))                            /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN1_PULLDOWN             (GPIO_UPDN_UPDN1_CFG(2))                            /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN1_RESERVED             (GPIO_UPDN_UPDN1_CFG(3))                            /*!< Reserved */

/* GPIO_UPDN_UPDN2 */
#define GPIO_UPDN_UPDN2_POS                  (4U)                                                /*!< GPIO_UPDN_UPDN2 bit offset */
#define GPIO_UPDN_UPDN2_CFG(regval)  (GPIO_UPDN_UPDN2 & ((uint32_t)(regval) << GPIO_UPDN_UPDN2_POS))
#define GPIO_UPDN_UPDN2_FLOATING             (GPIO_UPDN_UPDN2_CFG(0))                            /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN2_PULLUP               (GPIO_UPDN_UPDN2_CFG(1))                            /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN2_PULLDOWN             (GPIO_UPDN_UPDN2_CFG(2))                            /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN2_RESERVED             (GPIO_UPDN_UPDN2_CFG(3))                            /*!< Reserved */

/* GPIO_UPDN_UPDN3 */
#define GPIO_UPDN_UPDN3_POS                  (6U)                                                /*!< GPIO_UPDN_UPDN3 bit offset */
#define GPIO_UPDN_UPDN3_CFG(regval)  (GPIO_UPDN_UPDN3 & ((uint32_t)(regval) << GPIO_UPDN_UPDN3_POS))
#define GPIO_UPDN_UPDN3_FLOATING             (GPIO_UPDN_UPDN3_CFG(0))                            /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN3_PULLUP               (GPIO_UPDN_UPDN3_CFG(1))                            /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN3_PULLDOWN             (GPIO_UPDN_UPDN3_CFG(2))                            /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN3_RESERVED             (GPIO_UPDN_UPDN3_CFG(3))                            /*!< Reserved */

/* GPIO_UPDN_UPDN4 */
#define GPIO_UPDN_UPDN4_POS                  (8U)                                                /*!< GPIO_UPDN_UPDN4 bit offset */
#define GPIO_UPDN_UPDN4_CFG(regval)  (GPIO_UPDN_UPDN4 & ((uint32_t)(regval) << GPIO_UPDN_UPDN4_POS))
#define GPIO_UPDN_UPDN4_FLOATING             (GPIO_UPDN_UPDN4_CFG(0))                            /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN4_PULLUP               (GPIO_UPDN_UPDN4_CFG(1))                            /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN4_PULLDOWN             (GPIO_UPDN_UPDN4_CFG(2))                            /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN4_RESERVED             (GPIO_UPDN_UPDN4_CFG(3))                            /*!< Reserved */

/* GPIO_UPDN_UPDN5 */
#define GPIO_UPDN_UPDN5_POS                  (10U)                                               /*!< GPIO_UPDN_UPDN5 bit offset */
#define GPIO_UPDN_UPDN5_CFG(regval)  (GPIO_UPDN_UPDN5 & ((uint32_t)(regval) << GPIO_UPDN_UPDN5_POS))
#define GPIO_UPDN_UPDN5_FLOATING             (GPIO_UPDN_UPDN5_CFG(0))                            /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN5_PULLUP               (GPIO_UPDN_UPDN5_CFG(1))                            /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN5_PULLDOWN             (GPIO_UPDN_UPDN5_CFG(2))                            /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN5_RESERVED             (GPIO_UPDN_UPDN5_CFG(3))                            /*!< Reserved */

/* GPIO_UPDN_UPDN6 */
#define GPIO_UPDN_UPDN6_POS                  (12U)                                               /*!< GPIO_UPDN_UPDN6 bit offset */
#define GPIO_UPDN_UPDN6_CFG(regval)  (GPIO_UPDN_UPDN6 & ((uint32_t)(regval) << GPIO_UPDN_UPDN6_POS))
#define GPIO_UPDN_UPDN6_FLOATING             (GPIO_UPDN_UPDN6_CFG(0))                            /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN6_PULLUP               (GPIO_UPDN_UPDN6_CFG(1))                            /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN6_PULLDOWN             (GPIO_UPDN_UPDN6_CFG(2))                            /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN6_RESERVED             (GPIO_UPDN_UPDN6_CFG(3))                            /*!< Reserved */

/* GPIO_UPDN_UPDN7 */
#define GPIO_UPDN_UPDN7_POS                  (14U)                                               /*!< GPIO_UPDN_UPDN7 bit offset */
#define GPIO_UPDN_UPDN7_CFG(regval)  (GPIO_UPDN_UPDN7 & ((uint32_t)(regval) << GPIO_UPDN_UPDN7_POS))
#define GPIO_UPDN_UPDN7_FLOATING             (GPIO_UPDN_UPDN7_CFG(0))                            /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN7_PULLUP               (GPIO_UPDN_UPDN7_CFG(1))                            /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN7_PULLDOWN             (GPIO_UPDN_UPDN7_CFG(2))                            /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN7_RESERVED             (GPIO_UPDN_UPDN7_CFG(3))                            /*!< Reserved */

/* GPIO_UPDN_UPDN8 */
#define GPIO_UPDN_UPDN8_POS                  (16U)                                               /*!< GPIO_UPDN_UPDN8 bit offset */
#define GPIO_UPDN_UPDN8_CFG(regval)  (GPIO_UPDN_UPDN8 & ((uint32_t)(regval) << GPIO_UPDN_UPDN8_POS))
#define GPIO_UPDN_UPDN8_FLOATING             (GPIO_UPDN_UPDN8_CFG(0))                            /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN8_PULLUP               (GPIO_UPDN_UPDN8_CFG(1))                            /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN8_PULLDOWN             (GPIO_UPDN_UPDN8_CFG(2))                            /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN8_RESERVED             (GPIO_UPDN_UPDN8_CFG(3))                            /*!< Reserved */

/* GPIO_UPDN_UPDN9 */
#define GPIO_UPDN_UPDN9_POS                  (18U)                                               /*!< GPIO_UPDN_UPDN9 bit offset */
#define GPIO_UPDN_UPDN9_CFG(regval)  (GPIO_UPDN_UPDN9 & ((uint32_t)(regval) << GPIO_UPDN_UPDN9_POS))
#define GPIO_UPDN_UPDN9_FLOATING             (GPIO_UPDN_UPDN9_CFG(0))                            /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN9_PULLUP               (GPIO_UPDN_UPDN9_CFG(1))                            /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN9_PULLDOWN             (GPIO_UPDN_UPDN9_CFG(2))                            /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN9_RESERVED             (GPIO_UPDN_UPDN9_CFG(3))                            /*!< Reserved */

/* GPIO_UPDN_UPDN10 */
#define GPIO_UPDN_UPDN10_POS                 (20U)                                               /*!< GPIO_UPDN_UPDN10 bit offset */
#define GPIO_UPDN_UPDN10_CFG(regval)  (GPIO_UPDN_UPDN10 & ((uint32_t)(regval) << GPIO_UPDN_UPDN10_POS))
#define GPIO_UPDN_UPDN10_FLOATING            (GPIO_UPDN_UPDN10_CFG(0))                           /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN10_PULLUP              (GPIO_UPDN_UPDN10_CFG(1))                           /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN10_PULLDOWN            (GPIO_UPDN_UPDN10_CFG(2))                           /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN10_RESERVED            (GPIO_UPDN_UPDN10_CFG(3))                           /*!< Reserved */

/* GPIO_UPDN_UPDN11 */
#define GPIO_UPDN_UPDN11_POS                 (22U)                                               /*!< GPIO_UPDN_UPDN11 bit offset */
#define GPIO_UPDN_UPDN11_CFG(regval)  (GPIO_UPDN_UPDN11 & ((uint32_t)(regval) << GPIO_UPDN_UPDN11_POS))
#define GPIO_UPDN_UPDN11_FLOATING            (GPIO_UPDN_UPDN11_CFG(0))                           /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN11_PULLUP              (GPIO_UPDN_UPDN11_CFG(1))                           /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN11_PULLDOWN            (GPIO_UPDN_UPDN11_CFG(2))                           /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN11_RESERVED            (GPIO_UPDN_UPDN11_CFG(3))                           /*!< Reserved */

/* GPIO_UPDN_UPDN12 */
#define GPIO_UPDN_UPDN12_POS                 (24U)                                               /*!< GPIO_UPDN_UPDN12 bit offset */
#define GPIO_UPDN_UPDN12_CFG(regval)  (GPIO_UPDN_UPDN12 & ((uint32_t)(regval) << GPIO_UPDN_UPDN12_POS))
#define GPIO_UPDN_UPDN12_FLOATING            (GPIO_UPDN_UPDN12_CFG(0))                           /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN12_PULLUP              (GPIO_UPDN_UPDN12_CFG(1))                           /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN12_PULLDOWN            (GPIO_UPDN_UPDN12_CFG(2))                           /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN12_RESERVED            (GPIO_UPDN_UPDN12_CFG(3))                           /*!< Reserved */

/* GPIO_UPDN_UPDN13 */
#define GPIO_UPDN_UPDN13_POS                 (26U)                                               /*!< GPIO_UPDN_UPDN13 bit offset */
#define GPIO_UPDN_UPDN13_CFG(regval)  (GPIO_UPDN_UPDN13 & ((uint32_t)(regval) << GPIO_UPDN_UPDN13_POS))
#define GPIO_UPDN_UPDN13_FLOATING            (GPIO_UPDN_UPDN13_CFG(0))                           /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN13_PULLUP              (GPIO_UPDN_UPDN13_CFG(1))                           /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN13_PULLDOWN            (GPIO_UPDN_UPDN13_CFG(2))                           /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN13_RESERVED            (GPIO_UPDN_UPDN13_CFG(3))                           /*!< Reserved */

/* GPIO_UPDN_UPDN14 */
#define GPIO_UPDN_UPDN14_POS                 (28U)                                               /*!< GPIO_UPDN_UPDN14 bit offset */
#define GPIO_UPDN_UPDN14_CFG(regval)  (GPIO_UPDN_UPDN14 & ((uint32_t)(regval) << GPIO_UPDN_UPDN14_POS))
#define GPIO_UPDN_UPDN14_FLOATING            (GPIO_UPDN_UPDN14_CFG(0))                           /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN14_PULLUP              (GPIO_UPDN_UPDN14_CFG(1))                           /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN14_PULLDOWN            (GPIO_UPDN_UPDN14_CFG(2))                           /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN14_RESERVED            (GPIO_UPDN_UPDN14_CFG(3))                           /*!< Reserved */

/* GPIO_UPDN_UPDN15 */
#define GPIO_UPDN_UPDN15_POS                 (30U)                                               /*!< GPIO_UPDN_UPDN15 bit offset */
#define GPIO_UPDN_UPDN15_CFG(regval)  (GPIO_UPDN_UPDN15 & ((uint32_t)(regval) << GPIO_UPDN_UPDN15_POS))
#define GPIO_UPDN_UPDN15_FLOATING            (GPIO_UPDN_UPDN15_CFG(0))                           /*!< Select I/O no pull */
#define GPIO_UPDN_UPDN15_PULLUP              (GPIO_UPDN_UPDN15_CFG(1))                           /*!< Select I/O pull up */
#define GPIO_UPDN_UPDN15_PULLDOWN            (GPIO_UPDN_UPDN15_CFG(2))                           /*!< Select I/O pull down */
#define GPIO_UPDN_UPDN15_RESERVED            (GPIO_UPDN_UPDN15_CFG(3))                           /*!< Reserved */

/* GPIO_IDATA_IDATA0 */
#define GPIO_IDATA_IDATA0_POS                (0U)                                                /*!< GPIO_IDATA_IDATA0 bit offset */
#define GPIO_IDATA_IDATA0_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA0_HIGH               (GPIO_IDATA_IDATA0)                                 /*!< High level */

/* GPIO_IDATA_IDATA1 */
#define GPIO_IDATA_IDATA1_POS                (1U)                                                /*!< GPIO_IDATA_IDATA1 bit offset */
#define GPIO_IDATA_IDATA1_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA1_HIGH               (GPIO_IDATA_IDATA1)                                 /*!< High level */

/* GPIO_IDATA_IDATA2 */
#define GPIO_IDATA_IDATA2_POS                (2U)                                                /*!< GPIO_IDATA_IDATA2 bit offset */
#define GPIO_IDATA_IDATA2_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA2_HIGH               (GPIO_IDATA_IDATA2)                                 /*!< High level */

/* GPIO_IDATA_IDATA3 */
#define GPIO_IDATA_IDATA3_POS                (3U)                                                /*!< GPIO_IDATA_IDATA3 bit offset */
#define GPIO_IDATA_IDATA3_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA3_HIGH               (GPIO_IDATA_IDATA3)                                 /*!< High level */

/* GPIO_IDATA_IDATA4 */
#define GPIO_IDATA_IDATA4_POS                (4U)                                                /*!< GPIO_IDATA_IDATA4 bit offset */
#define GPIO_IDATA_IDATA4_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA4_HIGH               (GPIO_IDATA_IDATA4)                                 /*!< High level */

/* GPIO_IDATA_IDATA5 */
#define GPIO_IDATA_IDATA5_POS                (5U)                                                /*!< GPIO_IDATA_IDATA5 bit offset */
#define GPIO_IDATA_IDATA5_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA5_HIGH               (GPIO_IDATA_IDATA5)                                 /*!< High level */

/* GPIO_IDATA_IDATA6 */
#define GPIO_IDATA_IDATA6_POS                (6U)                                                /*!< GPIO_IDATA_IDATA6 bit offset */
#define GPIO_IDATA_IDATA6_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA6_HIGH               (GPIO_IDATA_IDATA6)                                 /*!< High level */

/* GPIO_IDATA_IDATA7 */
#define GPIO_IDATA_IDATA7_POS                (7U)                                                /*!< GPIO_IDATA_IDATA7 bit offset */
#define GPIO_IDATA_IDATA7_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA7_HIGH               (GPIO_IDATA_IDATA7)                                 /*!< High level */

/* GPIO_IDATA_IDATA8 */
#define GPIO_IDATA_IDATA8_POS                (8U)                                                /*!< GPIO_IDATA_IDATA8 bit offset */
#define GPIO_IDATA_IDATA8_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA8_HIGH               (GPIO_IDATA_IDATA8)                                 /*!< High level */

/* GPIO_IDATA_IDATA9 */
#define GPIO_IDATA_IDATA9_POS                (9U)                                                /*!< GPIO_IDATA_IDATA9 bit offset */
#define GPIO_IDATA_IDATA9_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA9_HIGH               (GPIO_IDATA_IDATA9)                                 /*!< High level */

/* GPIO_IDATA_IDATA10 */
#define GPIO_IDATA_IDATA10_POS               (10U)                                               /*!< GPIO_IDATA_IDATA10 bit offset */
#define GPIO_IDATA_IDATA10_LOW               ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA10_HIGH              (GPIO_IDATA_IDATA10)                                /*!< High level */

/* GPIO_IDATA_IDATA11 */
#define GPIO_IDATA_IDATA11_POS               (11U)                                               /*!< GPIO_IDATA_IDATA11 bit offset */
#define GPIO_IDATA_IDATA11_LOW               ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA11_HIGH              (GPIO_IDATA_IDATA11)                                /*!< High level */

/* GPIO_IDATA_IDATA12 */
#define GPIO_IDATA_IDATA12_POS               (12U)                                               /*!< GPIO_IDATA_IDATA12 bit offset */
#define GPIO_IDATA_IDATA12_LOW               ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA12_HIGH              (GPIO_IDATA_IDATA12)                                /*!< High level */

/* GPIO_IDATA_IDATA13 */
#define GPIO_IDATA_IDATA13_POS               (13U)                                               /*!< GPIO_IDATA_IDATA13 bit offset */
#define GPIO_IDATA_IDATA13_LOW               ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA13_HIGH              (GPIO_IDATA_IDATA13)                                /*!< High level */

/* GPIO_IDATA_IDATA14 */
#define GPIO_IDATA_IDATA14_POS               (14U)                                               /*!< GPIO_IDATA_IDATA14 bit offset */
#define GPIO_IDATA_IDATA14_LOW               ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA14_HIGH              (GPIO_IDATA_IDATA14)                                /*!< High level */

/* GPIO_IDATA_IDATA15 */
#define GPIO_IDATA_IDATA15_POS               (15U)                                               /*!< GPIO_IDATA_IDATA15 bit offset */
#define GPIO_IDATA_IDATA15_LOW               ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_IDATA_IDATA15_HIGH              (GPIO_IDATA_IDATA15)                                /*!< High level */

/* GPIO_ODATA_ODATA0 */
#define GPIO_ODATA_ODATA0_POS                (0U)                                                /*!< GPIO_ODATA_ODATA0 bit offset */
#define GPIO_ODATA_ODATA0_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA0_HIGH               (GPIO_ODATA_ODATA0)                                 /*!< High level */

/* GPIO_ODATA_ODATA1 */
#define GPIO_ODATA_ODATA1_POS                (1U)                                                /*!< GPIO_ODATA_ODATA1 bit offset */
#define GPIO_ODATA_ODATA1_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA1_HIGH               (GPIO_ODATA_ODATA1)                                 /*!< High level */

/* GPIO_ODATA_ODATA2 */
#define GPIO_ODATA_ODATA2_POS                (2U)                                                /*!< GPIO_ODATA_ODATA2 bit offset */
#define GPIO_ODATA_ODATA2_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA2_HIGH               (GPIO_ODATA_ODATA2)                                 /*!< High level */

/* GPIO_ODATA_ODATA3 */
#define GPIO_ODATA_ODATA3_POS                (3U)                                                /*!< GPIO_ODATA_ODATA3 bit offset */
#define GPIO_ODATA_ODATA3_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA3_HIGH               (GPIO_ODATA_ODATA3)                                 /*!< High level */

/* GPIO_ODATA_ODATA4 */
#define GPIO_ODATA_ODATA4_POS                (4U)                                                /*!< GPIO_ODATA_ODATA4 bit offset */
#define GPIO_ODATA_ODATA4_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA4_HIGH               (GPIO_ODATA_ODATA4)                                 /*!< High level */

/* GPIO_ODATA_ODATA5 */
#define GPIO_ODATA_ODATA5_POS                (5U)                                                /*!< GPIO_ODATA_ODATA5 bit offset */
#define GPIO_ODATA_ODATA5_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA5_HIGH               (GPIO_ODATA_ODATA5)                                 /*!< High level */

/* GPIO_ODATA_ODATA6 */
#define GPIO_ODATA_ODATA6_POS                (6U)                                                /*!< GPIO_ODATA_ODATA6 bit offset */
#define GPIO_ODATA_ODATA6_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA6_HIGH               (GPIO_ODATA_ODATA6)                                 /*!< High level */

/* GPIO_ODATA_ODATA7 */
#define GPIO_ODATA_ODATA7_POS                (7U)                                                /*!< GPIO_ODATA_ODATA7 bit offset */
#define GPIO_ODATA_ODATA7_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA7_HIGH               (GPIO_ODATA_ODATA7)                                 /*!< High level */

/* GPIO_ODATA_ODATA8 */
#define GPIO_ODATA_ODATA8_POS                (8U)                                                /*!< GPIO_ODATA_ODATA8 bit offset */
#define GPIO_ODATA_ODATA8_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA8_HIGH               (GPIO_ODATA_ODATA8)                                 /*!< High level */

/* GPIO_ODATA_ODATA9 */
#define GPIO_ODATA_ODATA9_POS                (9U)                                                /*!< GPIO_ODATA_ODATA9 bit offset */
#define GPIO_ODATA_ODATA9_LOW                ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA9_HIGH               (GPIO_ODATA_ODATA9)                                 /*!< High level */

/* GPIO_ODATA_ODATA10 */
#define GPIO_ODATA_ODATA10_POS               (10U)                                               /*!< GPIO_ODATA_ODATA10 bit offset */
#define GPIO_ODATA_ODATA10_LOW               ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA10_HIGH              (GPIO_ODATA_ODATA10)                                /*!< High level */

/* GPIO_ODATA_ODATA11 */
#define GPIO_ODATA_ODATA11_POS               (11U)                                               /*!< GPIO_ODATA_ODATA11 bit offset */
#define GPIO_ODATA_ODATA11_LOW               ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA11_HIGH              (GPIO_ODATA_ODATA11)                                /*!< High level */

/* GPIO_ODATA_ODATA12 */
#define GPIO_ODATA_ODATA12_POS               (12U)                                               /*!< GPIO_ODATA_ODATA12 bit offset */
#define GPIO_ODATA_ODATA12_LOW               ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA12_HIGH              (GPIO_ODATA_ODATA12)                                /*!< High level */

/* GPIO_ODATA_ODATA13 */
#define GPIO_ODATA_ODATA13_POS               (13U)                                               /*!< GPIO_ODATA_ODATA13 bit offset */
#define GPIO_ODATA_ODATA13_LOW               ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA13_HIGH              (GPIO_ODATA_ODATA13)                                /*!< High level */

/* GPIO_ODATA_ODATA14 */
#define GPIO_ODATA_ODATA14_POS               (14U)                                               /*!< GPIO_ODATA_ODATA14 bit offset */
#define GPIO_ODATA_ODATA14_LOW               ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA14_HIGH              (GPIO_ODATA_ODATA14)                                /*!< High level */

/* GPIO_ODATA_ODATA15 */
#define GPIO_ODATA_ODATA15_POS               (15U)                                               /*!< GPIO_ODATA_ODATA15 bit offset */
#define GPIO_ODATA_ODATA15_LOW               ((uint32_t)0x00000000U)                             /*!< Low level */
#define GPIO_ODATA_ODATA15_HIGH              (GPIO_ODATA_ODATA15)                                /*!< High level */

/* GPIO_AFSEL0_SEL0 */
#define GPIO_AFSEL0_SEL0_POS                 (0U)                                                /*!< GPIO_AFSEL0_SEL0 bit offset */
#define GPIO_AFSEL0_SEL0_CFG(regval)  (GPIO_AFSEL0_SEL0 & ((uint32_t)(regval) << GPIO_AFSEL0_SEL0_POS))
#define GPIO_AFSEL0_SEL0_AF0                 (GPIO_AFSEL0_SEL0_CFG(0))                           /*!< Select alternate function 0 */
#define GPIO_AFSEL0_SEL0_AF1                 (GPIO_AFSEL0_SEL0_CFG(1))                           /*!< Select alternate function 1 */
#define GPIO_AFSEL0_SEL0_AF2                 (GPIO_AFSEL0_SEL0_CFG(2))                           /*!< Select alternate function 2 */
#define GPIO_AFSEL0_SEL0_AF3                 (GPIO_AFSEL0_SEL0_CFG(3))                           /*!< Select alternate function 3 */
#define GPIO_AFSEL0_SEL0_AF4                 (GPIO_AFSEL0_SEL0_CFG(4))                           /*!< Select alternate function 4 */
#define GPIO_AFSEL0_SEL0_AF5                 (GPIO_AFSEL0_SEL0_CFG(5))                           /*!< Select alternate function 5 */
#define GPIO_AFSEL0_SEL0_AF6                 (GPIO_AFSEL0_SEL0_CFG(6))                           /*!< Select alternate function 6 */
#define GPIO_AFSEL0_SEL0_AF7                 (GPIO_AFSEL0_SEL0_CFG(7))                           /*!< Select alternate function 7 */
#define GPIO_AFSEL0_SEL0_AF8                 (GPIO_AFSEL0_SEL0_CFG(8))                           /*!< Select alternate function 8 */
#define GPIO_AFSEL0_SEL0_AF9                 (GPIO_AFSEL0_SEL0_CFG(9))                           /*!< Select alternate function 9 */
#define GPIO_AFSEL0_SEL0_AF10                (GPIO_AFSEL0_SEL0_CFG(10))                          /*!< Select alternate function 10 */
#define GPIO_AFSEL0_SEL0_AF11                (GPIO_AFSEL0_SEL0_CFG(11))                          /*!< Select alternate function 11 */
#define GPIO_AFSEL0_SEL0_AF12                (GPIO_AFSEL0_SEL0_CFG(12))                          /*!< Select alternate function 12 */
#define GPIO_AFSEL0_SEL0_AF13                (GPIO_AFSEL0_SEL0_CFG(13))                          /*!< Select alternate function 13 */
#define GPIO_AFSEL0_SEL0_AF14                (GPIO_AFSEL0_SEL0_CFG(14))                          /*!< Select alternate function 14 */
#define GPIO_AFSEL0_SEL0_AF15                (GPIO_AFSEL0_SEL0_CFG(15))                          /*!< Select alternate function 15 */

/* GPIO_AFSEL0_SEL1 */
#define GPIO_AFSEL0_SEL1_POS                 (4U)                                                /*!< GPIO_AFSEL0_SEL1 bit offset */
#define GPIO_AFSEL0_SEL1_CFG(regval)  (GPIO_AFSEL0_SEL1 & ((uint32_t)(regval) << GPIO_AFSEL0_SEL1_POS))
#define GPIO_AFSEL0_SEL1_AF0                 (GPIO_AFSEL0_SEL1_CFG(0))                           /*!< Select alternate function 0 */
#define GPIO_AFSEL0_SEL1_AF1                 (GPIO_AFSEL0_SEL1_CFG(1))                           /*!< Select alternate function 1 */
#define GPIO_AFSEL0_SEL1_AF2                 (GPIO_AFSEL0_SEL1_CFG(2))                           /*!< Select alternate function 2 */
#define GPIO_AFSEL0_SEL1_AF3                 (GPIO_AFSEL0_SEL1_CFG(3))                           /*!< Select alternate function 3 */
#define GPIO_AFSEL0_SEL1_AF4                 (GPIO_AFSEL0_SEL1_CFG(4))                           /*!< Select alternate function 4 */
#define GPIO_AFSEL0_SEL1_AF5                 (GPIO_AFSEL0_SEL1_CFG(5))                           /*!< Select alternate function 5 */
#define GPIO_AFSEL0_SEL1_AF6                 (GPIO_AFSEL0_SEL1_CFG(6))                           /*!< Select alternate function 6 */
#define GPIO_AFSEL0_SEL1_AF7                 (GPIO_AFSEL0_SEL1_CFG(7))                           /*!< Select alternate function 7 */
#define GPIO_AFSEL0_SEL1_AF8                 (GPIO_AFSEL0_SEL1_CFG(8))                           /*!< Select alternate function 8 */
#define GPIO_AFSEL0_SEL1_AF9                 (GPIO_AFSEL0_SEL1_CFG(9))                           /*!< Select alternate function 9 */
#define GPIO_AFSEL0_SEL1_AF10                (GPIO_AFSEL0_SEL1_CFG(10))                          /*!< Select alternate function 10 */
#define GPIO_AFSEL0_SEL1_AF11                (GPIO_AFSEL0_SEL1_CFG(11))                          /*!< Select alternate function 11 */
#define GPIO_AFSEL0_SEL1_AF12                (GPIO_AFSEL0_SEL1_CFG(12))                          /*!< Select alternate function 12 */
#define GPIO_AFSEL0_SEL1_AF13                (GPIO_AFSEL0_SEL1_CFG(13))                          /*!< Select alternate function 13 */
#define GPIO_AFSEL0_SEL1_AF14                (GPIO_AFSEL0_SEL1_CFG(14))                          /*!< Select alternate function 14 */
#define GPIO_AFSEL0_SEL1_AF15                (GPIO_AFSEL0_SEL1_CFG(15))                          /*!< Select alternate function 15 */

/* GPIO_AFSEL0_SEL2 */
#define GPIO_AFSEL0_SEL2_POS                 (8U)                                                /*!< GPIO_AFSEL0_SEL2 bit offset */
#define GPIO_AFSEL0_SEL2_CFG(regval)  (GPIO_AFSEL0_SEL2 & ((uint32_t)(regval) << GPIO_AFSEL0_SEL2_POS))
#define GPIO_AFSEL0_SEL2_AF0                 (GPIO_AFSEL0_SEL2_CFG(0))                           /*!< Select alternate function 0 */
#define GPIO_AFSEL0_SEL2_AF1                 (GPIO_AFSEL0_SEL2_CFG(1))                           /*!< Select alternate function 1 */
#define GPIO_AFSEL0_SEL2_AF2                 (GPIO_AFSEL0_SEL2_CFG(2))                           /*!< Select alternate function 2 */
#define GPIO_AFSEL0_SEL2_AF3                 (GPIO_AFSEL0_SEL2_CFG(3))                           /*!< Select alternate function 3 */
#define GPIO_AFSEL0_SEL2_AF4                 (GPIO_AFSEL0_SEL2_CFG(4))                           /*!< Select alternate function 4 */
#define GPIO_AFSEL0_SEL2_AF5                 (GPIO_AFSEL0_SEL2_CFG(5))                           /*!< Select alternate function 5 */
#define GPIO_AFSEL0_SEL2_AF6                 (GPIO_AFSEL0_SEL2_CFG(6))                           /*!< Select alternate function 6 */
#define GPIO_AFSEL0_SEL2_AF7                 (GPIO_AFSEL0_SEL2_CFG(7))                           /*!< Select alternate function 7 */
#define GPIO_AFSEL0_SEL2_AF8                 (GPIO_AFSEL0_SEL2_CFG(8))                           /*!< Select alternate function 8 */
#define GPIO_AFSEL0_SEL2_AF9                 (GPIO_AFSEL0_SEL2_CFG(9))                           /*!< Select alternate function 9 */
#define GPIO_AFSEL0_SEL2_AF10                (GPIO_AFSEL0_SEL2_CFG(10))                          /*!< Select alternate function 10 */
#define GPIO_AFSEL0_SEL2_AF11                (GPIO_AFSEL0_SEL2_CFG(11))                          /*!< Select alternate function 11 */
#define GPIO_AFSEL0_SEL2_AF12                (GPIO_AFSEL0_SEL2_CFG(12))                          /*!< Select alternate function 12 */
#define GPIO_AFSEL0_SEL2_AF13                (GPIO_AFSEL0_SEL2_CFG(13))                          /*!< Select alternate function 13 */
#define GPIO_AFSEL0_SEL2_AF14                (GPIO_AFSEL0_SEL2_CFG(14))                          /*!< Select alternate function 14 */
#define GPIO_AFSEL0_SEL2_AF15                (GPIO_AFSEL0_SEL2_CFG(15))                          /*!< Select alternate function 15 */

/* GPIO_AFSEL0_SEL3 */
#define GPIO_AFSEL0_SEL3_POS                 (12U)                                               /*!< GPIO_AFSEL0_SEL3 bit offset */
#define GPIO_AFSEL0_SEL3_CFG(regval)  (GPIO_AFSEL0_SEL3 & ((uint32_t)(regval) << GPIO_AFSEL0_SEL3_POS))
#define GPIO_AFSEL0_SEL3_AF0                 (GPIO_AFSEL0_SEL3_CFG(0))                           /*!< Select alternate function 0 */
#define GPIO_AFSEL0_SEL3_AF1                 (GPIO_AFSEL0_SEL3_CFG(1))                           /*!< Select alternate function 1 */
#define GPIO_AFSEL0_SEL3_AF2                 (GPIO_AFSEL0_SEL3_CFG(2))                           /*!< Select alternate function 2 */
#define GPIO_AFSEL0_SEL3_AF3                 (GPIO_AFSEL0_SEL3_CFG(3))                           /*!< Select alternate function 3 */
#define GPIO_AFSEL0_SEL3_AF4                 (GPIO_AFSEL0_SEL3_CFG(4))                           /*!< Select alternate function 4 */
#define GPIO_AFSEL0_SEL3_AF5                 (GPIO_AFSEL0_SEL3_CFG(5))                           /*!< Select alternate function 5 */
#define GPIO_AFSEL0_SEL3_AF6                 (GPIO_AFSEL0_SEL3_CFG(6))                           /*!< Select alternate function 6 */
#define GPIO_AFSEL0_SEL3_AF7                 (GPIO_AFSEL0_SEL3_CFG(7))                           /*!< Select alternate function 7 */
#define GPIO_AFSEL0_SEL3_AF8                 (GPIO_AFSEL0_SEL3_CFG(8))                           /*!< Select alternate function 8 */
#define GPIO_AFSEL0_SEL3_AF9                 (GPIO_AFSEL0_SEL3_CFG(9))                           /*!< Select alternate function 9 */
#define GPIO_AFSEL0_SEL3_AF10                (GPIO_AFSEL0_SEL3_CFG(10))                          /*!< Select alternate function 10 */
#define GPIO_AFSEL0_SEL3_AF11                (GPIO_AFSEL0_SEL3_CFG(11))                          /*!< Select alternate function 11 */
#define GPIO_AFSEL0_SEL3_AF12                (GPIO_AFSEL0_SEL3_CFG(12))                          /*!< Select alternate function 12 */
#define GPIO_AFSEL0_SEL3_AF13                (GPIO_AFSEL0_SEL3_CFG(13))                          /*!< Select alternate function 13 */
#define GPIO_AFSEL0_SEL3_AF14                (GPIO_AFSEL0_SEL3_CFG(14))                          /*!< Select alternate function 14 */
#define GPIO_AFSEL0_SEL3_AF15                (GPIO_AFSEL0_SEL3_CFG(15))                          /*!< Select alternate function 15 */

/* GPIO_AFSEL0_SEL4 */
#define GPIO_AFSEL0_SEL4_POS                 (16U)                                               /*!< GPIO_AFSEL0_SEL4 bit offset */
#define GPIO_AFSEL0_SEL4_CFG(regval)  (GPIO_AFSEL0_SEL4 & ((uint32_t)(regval) << GPIO_AFSEL0_SEL4_POS))
#define GPIO_AFSEL0_SEL4_AF0                 (GPIO_AFSEL0_SEL4_CFG(0))                           /*!< Select alternate function 0 */
#define GPIO_AFSEL0_SEL4_AF1                 (GPIO_AFSEL0_SEL4_CFG(1))                           /*!< Select alternate function 1 */
#define GPIO_AFSEL0_SEL4_AF2                 (GPIO_AFSEL0_SEL4_CFG(2))                           /*!< Select alternate function 2 */
#define GPIO_AFSEL0_SEL4_AF3                 (GPIO_AFSEL0_SEL4_CFG(3))                           /*!< Select alternate function 3 */
#define GPIO_AFSEL0_SEL4_AF4                 (GPIO_AFSEL0_SEL4_CFG(4))                           /*!< Select alternate function 4 */
#define GPIO_AFSEL0_SEL4_AF5                 (GPIO_AFSEL0_SEL4_CFG(5))                           /*!< Select alternate function 5 */
#define GPIO_AFSEL0_SEL4_AF6                 (GPIO_AFSEL0_SEL4_CFG(6))                           /*!< Select alternate function 6 */
#define GPIO_AFSEL0_SEL4_AF7                 (GPIO_AFSEL0_SEL4_CFG(7))                           /*!< Select alternate function 7 */
#define GPIO_AFSEL0_SEL4_AF8                 (GPIO_AFSEL0_SEL4_CFG(8))                           /*!< Select alternate function 8 */
#define GPIO_AFSEL0_SEL4_AF9                 (GPIO_AFSEL0_SEL4_CFG(9))                           /*!< Select alternate function 9 */
#define GPIO_AFSEL0_SEL4_AF10                (GPIO_AFSEL0_SEL4_CFG(10))                          /*!< Select alternate function 10 */
#define GPIO_AFSEL0_SEL4_AF11                (GPIO_AFSEL0_SEL4_CFG(11))                          /*!< Select alternate function 11 */
#define GPIO_AFSEL0_SEL4_AF12                (GPIO_AFSEL0_SEL4_CFG(12))                          /*!< Select alternate function 12 */
#define GPIO_AFSEL0_SEL4_AF13                (GPIO_AFSEL0_SEL4_CFG(13))                          /*!< Select alternate function 13 */
#define GPIO_AFSEL0_SEL4_AF14                (GPIO_AFSEL0_SEL4_CFG(14))                          /*!< Select alternate function 14 */
#define GPIO_AFSEL0_SEL4_AF15                (GPIO_AFSEL0_SEL4_CFG(15))                          /*!< Select alternate function 15 */

/* GPIO_AFSEL0_SEL5 */
#define GPIO_AFSEL0_SEL5_POS                 (20U)                                               /*!< GPIO_AFSEL0_SEL5 bit offset */
#define GPIO_AFSEL0_SEL5_CFG(regval)  (GPIO_AFSEL0_SEL5 & ((uint32_t)(regval) << GPIO_AFSEL0_SEL5_POS))
#define GPIO_AFSEL0_SEL5_AF0                 (GPIO_AFSEL0_SEL5_CFG(0))                           /*!< Select alternate function 0 */
#define GPIO_AFSEL0_SEL5_AF1                 (GPIO_AFSEL0_SEL5_CFG(1))                           /*!< Select alternate function 1 */
#define GPIO_AFSEL0_SEL5_AF2                 (GPIO_AFSEL0_SEL5_CFG(2))                           /*!< Select alternate function 2 */
#define GPIO_AFSEL0_SEL5_AF3                 (GPIO_AFSEL0_SEL5_CFG(3))                           /*!< Select alternate function 3 */
#define GPIO_AFSEL0_SEL5_AF4                 (GPIO_AFSEL0_SEL5_CFG(4))                           /*!< Select alternate function 4 */
#define GPIO_AFSEL0_SEL5_AF5                 (GPIO_AFSEL0_SEL5_CFG(5))                           /*!< Select alternate function 5 */
#define GPIO_AFSEL0_SEL5_AF6                 (GPIO_AFSEL0_SEL5_CFG(6))                           /*!< Select alternate function 6 */
#define GPIO_AFSEL0_SEL5_AF7                 (GPIO_AFSEL0_SEL5_CFG(7))                           /*!< Select alternate function 7 */
#define GPIO_AFSEL0_SEL5_AF8                 (GPIO_AFSEL0_SEL5_CFG(8))                           /*!< Select alternate function 8 */
#define GPIO_AFSEL0_SEL5_AF9                 (GPIO_AFSEL0_SEL5_CFG(9))                           /*!< Select alternate function 9 */
#define GPIO_AFSEL0_SEL5_AF10                (GPIO_AFSEL0_SEL5_CFG(10))                          /*!< Select alternate function 10 */
#define GPIO_AFSEL0_SEL5_AF11                (GPIO_AFSEL0_SEL5_CFG(11))                          /*!< Select alternate function 11 */
#define GPIO_AFSEL0_SEL5_AF12                (GPIO_AFSEL0_SEL5_CFG(12))                          /*!< Select alternate function 12 */
#define GPIO_AFSEL0_SEL5_AF13                (GPIO_AFSEL0_SEL5_CFG(13))                          /*!< Select alternate function 13 */
#define GPIO_AFSEL0_SEL5_AF14                (GPIO_AFSEL0_SEL5_CFG(14))                          /*!< Select alternate function 14 */
#define GPIO_AFSEL0_SEL5_AF15                (GPIO_AFSEL0_SEL5_CFG(15))                          /*!< Select alternate function 15 */

/* GPIO_AFSEL0_SEL6 */
#define GPIO_AFSEL0_SEL6_POS                 (24U)                                               /*!< GPIO_AFSEL0_SEL6 bit offset */
#define GPIO_AFSEL0_SEL6_CFG(regval)  (GPIO_AFSEL0_SEL6 & ((uint32_t)(regval) << GPIO_AFSEL0_SEL6_POS))
#define GPIO_AFSEL0_SEL6_AF0                 (GPIO_AFSEL0_SEL6_CFG(0))                           /*!< Select alternate function 0 */
#define GPIO_AFSEL0_SEL6_AF1                 (GPIO_AFSEL0_SEL6_CFG(1))                           /*!< Select alternate function 1 */
#define GPIO_AFSEL0_SEL6_AF2                 (GPIO_AFSEL0_SEL6_CFG(2))                           /*!< Select alternate function 2 */
#define GPIO_AFSEL0_SEL6_AF3                 (GPIO_AFSEL0_SEL6_CFG(3))                           /*!< Select alternate function 3 */
#define GPIO_AFSEL0_SEL6_AF4                 (GPIO_AFSEL0_SEL6_CFG(4))                           /*!< Select alternate function 4 */
#define GPIO_AFSEL0_SEL6_AF5                 (GPIO_AFSEL0_SEL6_CFG(5))                           /*!< Select alternate function 5 */
#define GPIO_AFSEL0_SEL6_AF6                 (GPIO_AFSEL0_SEL6_CFG(6))                           /*!< Select alternate function 6 */
#define GPIO_AFSEL0_SEL6_AF7                 (GPIO_AFSEL0_SEL6_CFG(7))                           /*!< Select alternate function 7 */
#define GPIO_AFSEL0_SEL6_AF8                 (GPIO_AFSEL0_SEL6_CFG(8))                           /*!< Select alternate function 8 */
#define GPIO_AFSEL0_SEL6_AF9                 (GPIO_AFSEL0_SEL6_CFG(9))                           /*!< Select alternate function 9 */
#define GPIO_AFSEL0_SEL6_AF10                (GPIO_AFSEL0_SEL6_CFG(10))                          /*!< Select alternate function 10 */
#define GPIO_AFSEL0_SEL6_AF11                (GPIO_AFSEL0_SEL6_CFG(11))                          /*!< Select alternate function 11 */
#define GPIO_AFSEL0_SEL6_AF12                (GPIO_AFSEL0_SEL6_CFG(12))                          /*!< Select alternate function 12 */
#define GPIO_AFSEL0_SEL6_AF13                (GPIO_AFSEL0_SEL6_CFG(13))                          /*!< Select alternate function 13 */
#define GPIO_AFSEL0_SEL6_AF14                (GPIO_AFSEL0_SEL6_CFG(14))                          /*!< Select alternate function 14 */
#define GPIO_AFSEL0_SEL6_AF15                (GPIO_AFSEL0_SEL6_CFG(15))                          /*!< Select alternate function 15 */

/* GPIO_AFSEL0_SEL7 */
#define GPIO_AFSEL0_SEL7_POS                 (28U)                                               /*!< GPIO_AFSEL0_SEL7 bit offset */
#define GPIO_AFSEL0_SEL7_CFG(regval)  (GPIO_AFSEL0_SEL7 & ((uint32_t)(regval) << GPIO_AFSEL0_SEL7_POS))
#define GPIO_AFSEL0_SEL7_AF0                 (GPIO_AFSEL0_SEL7_CFG(0))                           /*!< Select alternate function 0 */
#define GPIO_AFSEL0_SEL7_AF1                 (GPIO_AFSEL0_SEL7_CFG(1))                           /*!< Select alternate function 1 */
#define GPIO_AFSEL0_SEL7_AF2                 (GPIO_AFSEL0_SEL7_CFG(2))                           /*!< Select alternate function 2 */
#define GPIO_AFSEL0_SEL7_AF3                 (GPIO_AFSEL0_SEL7_CFG(3))                           /*!< Select alternate function 3 */
#define GPIO_AFSEL0_SEL7_AF4                 (GPIO_AFSEL0_SEL7_CFG(4))                           /*!< Select alternate function 4 */
#define GPIO_AFSEL0_SEL7_AF5                 (GPIO_AFSEL0_SEL7_CFG(5))                           /*!< Select alternate function 5 */
#define GPIO_AFSEL0_SEL7_AF6                 (GPIO_AFSEL0_SEL7_CFG(6))                           /*!< Select alternate function 6 */
#define GPIO_AFSEL0_SEL7_AF7                 (GPIO_AFSEL0_SEL7_CFG(7))                           /*!< Select alternate function 7 */
#define GPIO_AFSEL0_SEL7_AF8                 (GPIO_AFSEL0_SEL7_CFG(8))                           /*!< Select alternate function 8 */
#define GPIO_AFSEL0_SEL7_AF9                 (GPIO_AFSEL0_SEL7_CFG(9))                           /*!< Select alternate function 9 */
#define GPIO_AFSEL0_SEL7_AF10                (GPIO_AFSEL0_SEL7_CFG(10))                          /*!< Select alternate function 10 */
#define GPIO_AFSEL0_SEL7_AF11                (GPIO_AFSEL0_SEL7_CFG(11))                          /*!< Select alternate function 11 */
#define GPIO_AFSEL0_SEL7_AF12                (GPIO_AFSEL0_SEL7_CFG(12))                          /*!< Select alternate function 12 */
#define GPIO_AFSEL0_SEL7_AF13                (GPIO_AFSEL0_SEL7_CFG(13))                          /*!< Select alternate function 13 */
#define GPIO_AFSEL0_SEL7_AF14                (GPIO_AFSEL0_SEL7_CFG(14))                          /*!< Select alternate function 14 */
#define GPIO_AFSEL0_SEL7_AF15                (GPIO_AFSEL0_SEL7_CFG(15))                          /*!< Select alternate function 15 */

/* GPIO_AFSEL1_SEL8 */
#define GPIO_AFSEL1_SEL8_POS                 (0U)                                                /*!< GPIO_AFSEL1_SEL8 bit offset */
#define GPIO_AFSEL1_SEL8_CFG(regval)  (GPIO_AFSEL1_SEL8 & ((uint32_t)(regval) << GPIO_AFSEL1_SEL8_POS))
#define GPIO_AFSEL1_SEL8_AF0                 (GPIO_AFSEL1_SEL8_CFG(0))                           /*!< Select alternate function 0 */
#define GPIO_AFSEL1_SEL8_AF1                 (GPIO_AFSEL1_SEL8_CFG(1))                           /*!< Select alternate function 1 */
#define GPIO_AFSEL1_SEL8_AF2                 (GPIO_AFSEL1_SEL8_CFG(2))                           /*!< Select alternate function 2 */
#define GPIO_AFSEL1_SEL8_AF3                 (GPIO_AFSEL1_SEL8_CFG(3))                           /*!< Select alternate function 3 */
#define GPIO_AFSEL1_SEL8_AF4                 (GPIO_AFSEL1_SEL8_CFG(4))                           /*!< Select alternate function 4 */
#define GPIO_AFSEL1_SEL8_AF5                 (GPIO_AFSEL1_SEL8_CFG(5))                           /*!< Select alternate function 5 */
#define GPIO_AFSEL1_SEL8_AF6                 (GPIO_AFSEL1_SEL8_CFG(6))                           /*!< Select alternate function 6 */
#define GPIO_AFSEL1_SEL8_AF7                 (GPIO_AFSEL1_SEL8_CFG(7))                           /*!< Select alternate function 7 */
#define GPIO_AFSEL1_SEL8_AF8                 (GPIO_AFSEL1_SEL8_CFG(8))                           /*!< Select alternate function 8 */
#define GPIO_AFSEL1_SEL8_AF9                 (GPIO_AFSEL1_SEL8_CFG(9))                           /*!< Select alternate function 9 */
#define GPIO_AFSEL1_SEL8_AF10                (GPIO_AFSEL1_SEL8_CFG(10))                          /*!< Select alternate function 10 */
#define GPIO_AFSEL1_SEL8_AF11                (GPIO_AFSEL1_SEL8_CFG(11))                          /*!< Select alternate function 11 */
#define GPIO_AFSEL1_SEL8_AF12                (GPIO_AFSEL1_SEL8_CFG(12))                          /*!< Select alternate function 12 */
#define GPIO_AFSEL1_SEL8_AF13                (GPIO_AFSEL1_SEL8_CFG(13))                          /*!< Select alternate function 13 */
#define GPIO_AFSEL1_SEL8_AF14                (GPIO_AFSEL1_SEL8_CFG(14))                          /*!< Select alternate function 14 */
#define GPIO_AFSEL1_SEL8_AF15                (GPIO_AFSEL1_SEL8_CFG(15))                          /*!< Select alternate function 15 */

/* GPIO_AFSEL1_SEL9 */
#define GPIO_AFSEL1_SEL9_POS                 (4U)                                                /*!< GPIO_AFSEL1_SEL9 bit offset */
#define GPIO_AFSEL1_SEL9_CFG(regval)  (GPIO_AFSEL1_SEL9 & ((uint32_t)(regval) << GPIO_AFSEL1_SEL9_POS))
#define GPIO_AFSEL1_SEL9_AF0                 (GPIO_AFSEL1_SEL9_CFG(0))                           /*!< Select alternate function 0 */
#define GPIO_AFSEL1_SEL9_AF1                 (GPIO_AFSEL1_SEL9_CFG(1))                           /*!< Select alternate function 1 */
#define GPIO_AFSEL1_SEL9_AF2                 (GPIO_AFSEL1_SEL9_CFG(2))                           /*!< Select alternate function 2 */
#define GPIO_AFSEL1_SEL9_AF3                 (GPIO_AFSEL1_SEL9_CFG(3))                           /*!< Select alternate function 3 */
#define GPIO_AFSEL1_SEL9_AF4                 (GPIO_AFSEL1_SEL9_CFG(4))                           /*!< Select alternate function 4 */
#define GPIO_AFSEL1_SEL9_AF5                 (GPIO_AFSEL1_SEL9_CFG(5))                           /*!< Select alternate function 5 */
#define GPIO_AFSEL1_SEL9_AF6                 (GPIO_AFSEL1_SEL9_CFG(6))                           /*!< Select alternate function 6 */
#define GPIO_AFSEL1_SEL9_AF7                 (GPIO_AFSEL1_SEL9_CFG(7))                           /*!< Select alternate function 7 */
#define GPIO_AFSEL1_SEL9_AF8                 (GPIO_AFSEL1_SEL9_CFG(8))                           /*!< Select alternate function 8 */
#define GPIO_AFSEL1_SEL9_AF9                 (GPIO_AFSEL1_SEL9_CFG(9))                           /*!< Select alternate function 9 */
#define GPIO_AFSEL1_SEL9_AF10                (GPIO_AFSEL1_SEL9_CFG(10))                          /*!< Select alternate function 10 */
#define GPIO_AFSEL1_SEL9_AF11                (GPIO_AFSEL1_SEL9_CFG(11))                          /*!< Select alternate function 11 */
#define GPIO_AFSEL1_SEL9_AF12                (GPIO_AFSEL1_SEL9_CFG(12))                          /*!< Select alternate function 12 */
#define GPIO_AFSEL1_SEL9_AF13                (GPIO_AFSEL1_SEL9_CFG(13))                          /*!< Select alternate function 13 */
#define GPIO_AFSEL1_SEL9_AF14                (GPIO_AFSEL1_SEL9_CFG(14))                          /*!< Select alternate function 14 */
#define GPIO_AFSEL1_SEL9_AF15                (GPIO_AFSEL1_SEL9_CFG(15))                          /*!< Select alternate function 15 */

/* GPIO_AFSEL1_SEL10 */
#define GPIO_AFSEL1_SEL10_POS                (8U)                                                /*!< GPIO_AFSEL1_SEL10 bit offset */
#define GPIO_AFSEL1_SEL10_CFG(regval)  (GPIO_AFSEL1_SEL10 & ((uint32_t)(regval) << GPIO_AFSEL1_SEL10_POS))
#define GPIO_AFSEL1_SEL10_AF0                (GPIO_AFSEL1_SEL10_CFG(0))                          /*!< Select alternate function 0 */
#define GPIO_AFSEL1_SEL10_AF1                (GPIO_AFSEL1_SEL10_CFG(1))                          /*!< Select alternate function 1 */
#define GPIO_AFSEL1_SEL10_AF2                (GPIO_AFSEL1_SEL10_CFG(2))                          /*!< Select alternate function 2 */
#define GPIO_AFSEL1_SEL10_AF3                (GPIO_AFSEL1_SEL10_CFG(3))                          /*!< Select alternate function 3 */
#define GPIO_AFSEL1_SEL10_AF4                (GPIO_AFSEL1_SEL10_CFG(4))                          /*!< Select alternate function 4 */
#define GPIO_AFSEL1_SEL10_AF5                (GPIO_AFSEL1_SEL10_CFG(5))                          /*!< Select alternate function 5 */
#define GPIO_AFSEL1_SEL10_AF6                (GPIO_AFSEL1_SEL10_CFG(6))                          /*!< Select alternate function 6 */
#define GPIO_AFSEL1_SEL10_AF7                (GPIO_AFSEL1_SEL10_CFG(7))                          /*!< Select alternate function 7 */
#define GPIO_AFSEL1_SEL10_AF8                (GPIO_AFSEL1_SEL10_CFG(8))                          /*!< Select alternate function 8 */
#define GPIO_AFSEL1_SEL10_AF9                (GPIO_AFSEL1_SEL10_CFG(9))                          /*!< Select alternate function 9 */
#define GPIO_AFSEL1_SEL10_AF10               (GPIO_AFSEL1_SEL10_CFG(10))                         /*!< Select alternate function 10 */
#define GPIO_AFSEL1_SEL10_AF11               (GPIO_AFSEL1_SEL10_CFG(11))                         /*!< Select alternate function 11 */
#define GPIO_AFSEL1_SEL10_AF12               (GPIO_AFSEL1_SEL10_CFG(12))                         /*!< Select alternate function 12 */
#define GPIO_AFSEL1_SEL10_AF13               (GPIO_AFSEL1_SEL10_CFG(13))                         /*!< Select alternate function 13 */
#define GPIO_AFSEL1_SEL10_AF14               (GPIO_AFSEL1_SEL10_CFG(14))                         /*!< Select alternate function 14 */
#define GPIO_AFSEL1_SEL10_AF15               (GPIO_AFSEL1_SEL10_CFG(15))                         /*!< Select alternate function 15 */

/* GPIO_AFSEL1_SEL11 */
#define GPIO_AFSEL1_SEL11_POS                (12U)                                               /*!< GPIO_AFSEL1_SEL11 bit offset */
#define GPIO_AFSEL1_SEL11_CFG(regval)  (GPIO_AFSEL1_SEL11 & ((uint32_t)(regval) << GPIO_AFSEL1_SEL11_POS))
#define GPIO_AFSEL1_SEL11_AF0                (GPIO_AFSEL1_SEL11_CFG(0))                          /*!< Select alternate function 0 */
#define GPIO_AFSEL1_SEL11_AF1                (GPIO_AFSEL1_SEL11_CFG(1))                          /*!< Select alternate function 1 */
#define GPIO_AFSEL1_SEL11_AF2                (GPIO_AFSEL1_SEL11_CFG(2))                          /*!< Select alternate function 2 */
#define GPIO_AFSEL1_SEL11_AF3                (GPIO_AFSEL1_SEL11_CFG(3))                          /*!< Select alternate function 3 */
#define GPIO_AFSEL1_SEL11_AF4                (GPIO_AFSEL1_SEL11_CFG(4))                          /*!< Select alternate function 4 */
#define GPIO_AFSEL1_SEL11_AF5                (GPIO_AFSEL1_SEL11_CFG(5))                          /*!< Select alternate function 5 */
#define GPIO_AFSEL1_SEL11_AF6                (GPIO_AFSEL1_SEL11_CFG(6))                          /*!< Select alternate function 6 */
#define GPIO_AFSEL1_SEL11_AF7                (GPIO_AFSEL1_SEL11_CFG(7))                          /*!< Select alternate function 7 */
#define GPIO_AFSEL1_SEL11_AF8                (GPIO_AFSEL1_SEL11_CFG(8))                          /*!< Select alternate function 8 */
#define GPIO_AFSEL1_SEL11_AF9                (GPIO_AFSEL1_SEL11_CFG(9))                          /*!< Select alternate function 9 */
#define GPIO_AFSEL1_SEL11_AF10               (GPIO_AFSEL1_SEL11_CFG(10))                         /*!< Select alternate function 10 */
#define GPIO_AFSEL1_SEL11_AF11               (GPIO_AFSEL1_SEL11_CFG(11))                         /*!< Select alternate function 11 */
#define GPIO_AFSEL1_SEL11_AF12               (GPIO_AFSEL1_SEL11_CFG(12))                         /*!< Select alternate function 12 */
#define GPIO_AFSEL1_SEL11_AF13               (GPIO_AFSEL1_SEL11_CFG(13))                         /*!< Select alternate function 13 */
#define GPIO_AFSEL1_SEL11_AF14               (GPIO_AFSEL1_SEL11_CFG(14))                         /*!< Select alternate function 14 */
#define GPIO_AFSEL1_SEL11_AF15               (GPIO_AFSEL1_SEL11_CFG(15))                         /*!< Select alternate function 15 */

/* GPIO_AFSEL1_SEL12 */
#define GPIO_AFSEL1_SEL12_POS                (16U)                                               /*!< GPIO_AFSEL1_SEL12 bit offset */
#define GPIO_AFSEL1_SEL12_CFG(regval)  (GPIO_AFSEL1_SEL12 & ((uint32_t)(regval) << GPIO_AFSEL1_SEL12_POS))
#define GPIO_AFSEL1_SEL12_AF0                (GPIO_AFSEL1_SEL12_CFG(0))                          /*!< Select alternate function 0 */
#define GPIO_AFSEL1_SEL12_AF1                (GPIO_AFSEL1_SEL12_CFG(1))                          /*!< Select alternate function 1 */
#define GPIO_AFSEL1_SEL12_AF2                (GPIO_AFSEL1_SEL12_CFG(2))                          /*!< Select alternate function 2 */
#define GPIO_AFSEL1_SEL12_AF3                (GPIO_AFSEL1_SEL12_CFG(3))                          /*!< Select alternate function 3 */
#define GPIO_AFSEL1_SEL12_AF4                (GPIO_AFSEL1_SEL12_CFG(4))                          /*!< Select alternate function 4 */
#define GPIO_AFSEL1_SEL12_AF5                (GPIO_AFSEL1_SEL12_CFG(5))                          /*!< Select alternate function 5 */
#define GPIO_AFSEL1_SEL12_AF6                (GPIO_AFSEL1_SEL12_CFG(6))                          /*!< Select alternate function 6 */
#define GPIO_AFSEL1_SEL12_AF7                (GPIO_AFSEL1_SEL12_CFG(7))                          /*!< Select alternate function 7 */
#define GPIO_AFSEL1_SEL12_AF8                (GPIO_AFSEL1_SEL12_CFG(8))                          /*!< Select alternate function 8 */
#define GPIO_AFSEL1_SEL12_AF9                (GPIO_AFSEL1_SEL12_CFG(9))                          /*!< Select alternate function 9 */
#define GPIO_AFSEL1_SEL12_AF10               (GPIO_AFSEL1_SEL12_CFG(10))                         /*!< Select alternate function 10 */
#define GPIO_AFSEL1_SEL12_AF11               (GPIO_AFSEL1_SEL12_CFG(11))                         /*!< Select alternate function 11 */
#define GPIO_AFSEL1_SEL12_AF12               (GPIO_AFSEL1_SEL12_CFG(12))                         /*!< Select alternate function 12 */
#define GPIO_AFSEL1_SEL12_AF13               (GPIO_AFSEL1_SEL12_CFG(13))                         /*!< Select alternate function 13 */
#define GPIO_AFSEL1_SEL12_AF14               (GPIO_AFSEL1_SEL12_CFG(14))                         /*!< Select alternate function 14 */
#define GPIO_AFSEL1_SEL12_AF15               (GPIO_AFSEL1_SEL12_CFG(15))                         /*!< Select alternate function 15 */

/* GPIO_AFSEL1_SEL13 */
#define GPIO_AFSEL1_SEL13_POS                (20U)                                               /*!< GPIO_AFSEL1_SEL13 bit offset */
#define GPIO_AFSEL1_SEL13_CFG(regval)  (GPIO_AFSEL1_SEL13 & ((uint32_t)(regval) << GPIO_AFSEL1_SEL13_POS))
#define GPIO_AFSEL1_SEL13_AF0                (GPIO_AFSEL1_SEL13_CFG(0))                          /*!< Select alternate function 0 */
#define GPIO_AFSEL1_SEL13_AF1                (GPIO_AFSEL1_SEL13_CFG(1))                          /*!< Select alternate function 1 */
#define GPIO_AFSEL1_SEL13_AF2                (GPIO_AFSEL1_SEL13_CFG(2))                          /*!< Select alternate function 2 */
#define GPIO_AFSEL1_SEL13_AF3                (GPIO_AFSEL1_SEL13_CFG(3))                          /*!< Select alternate function 3 */
#define GPIO_AFSEL1_SEL13_AF4                (GPIO_AFSEL1_SEL13_CFG(4))                          /*!< Select alternate function 4 */
#define GPIO_AFSEL1_SEL13_AF5                (GPIO_AFSEL1_SEL13_CFG(5))                          /*!< Select alternate function 5 */
#define GPIO_AFSEL1_SEL13_AF6                (GPIO_AFSEL1_SEL13_CFG(6))                          /*!< Select alternate function 6 */
#define GPIO_AFSEL1_SEL13_AF7                (GPIO_AFSEL1_SEL13_CFG(7))                          /*!< Select alternate function 7 */
#define GPIO_AFSEL1_SEL13_AF8                (GPIO_AFSEL1_SEL13_CFG(8))                          /*!< Select alternate function 8 */
#define GPIO_AFSEL1_SEL13_AF9                (GPIO_AFSEL1_SEL13_CFG(9))                          /*!< Select alternate function 9 */
#define GPIO_AFSEL1_SEL13_AF10               (GPIO_AFSEL1_SEL13_CFG(10))                         /*!< Select alternate function 10 */
#define GPIO_AFSEL1_SEL13_AF11               (GPIO_AFSEL1_SEL13_CFG(11))                         /*!< Select alternate function 11 */
#define GPIO_AFSEL1_SEL13_AF12               (GPIO_AFSEL1_SEL13_CFG(12))                         /*!< Select alternate function 12 */
#define GPIO_AFSEL1_SEL13_AF13               (GPIO_AFSEL1_SEL13_CFG(13))                         /*!< Select alternate function 13 */
#define GPIO_AFSEL1_SEL13_AF14               (GPIO_AFSEL1_SEL13_CFG(14))                         /*!< Select alternate function 14 */
#define GPIO_AFSEL1_SEL13_AF15               (GPIO_AFSEL1_SEL13_CFG(15))                         /*!< Select alternate function 15 */

/* GPIO_AFSEL1_SEL14 */
#define GPIO_AFSEL1_SEL14_POS                (24U)                                               /*!< GPIO_AFSEL1_SEL14 bit offset */
#define GPIO_AFSEL1_SEL14_CFG(regval)  (GPIO_AFSEL1_SEL14 & ((uint32_t)(regval) << GPIO_AFSEL1_SEL14_POS))
#define GPIO_AFSEL1_SEL14_AF0                (GPIO_AFSEL1_SEL14_CFG(0))                          /*!< Select alternate function 0 */
#define GPIO_AFSEL1_SEL14_AF1                (GPIO_AFSEL1_SEL14_CFG(1))                          /*!< Select alternate function 1 */
#define GPIO_AFSEL1_SEL14_AF2                (GPIO_AFSEL1_SEL14_CFG(2))                          /*!< Select alternate function 2 */
#define GPIO_AFSEL1_SEL14_AF3                (GPIO_AFSEL1_SEL14_CFG(3))                          /*!< Select alternate function 3 */
#define GPIO_AFSEL1_SEL14_AF4                (GPIO_AFSEL1_SEL14_CFG(4))                          /*!< Select alternate function 4 */
#define GPIO_AFSEL1_SEL14_AF5                (GPIO_AFSEL1_SEL14_CFG(5))                          /*!< Select alternate function 5 */
#define GPIO_AFSEL1_SEL14_AF6                (GPIO_AFSEL1_SEL14_CFG(6))                          /*!< Select alternate function 6 */
#define GPIO_AFSEL1_SEL14_AF7                (GPIO_AFSEL1_SEL14_CFG(7))                          /*!< Select alternate function 7 */
#define GPIO_AFSEL1_SEL14_AF8                (GPIO_AFSEL1_SEL14_CFG(8))                          /*!< Select alternate function 8 */
#define GPIO_AFSEL1_SEL14_AF9                (GPIO_AFSEL1_SEL14_CFG(9))                          /*!< Select alternate function 9 */
#define GPIO_AFSEL1_SEL14_AF10               (GPIO_AFSEL1_SEL14_CFG(10))                         /*!< Select alternate function 10 */
#define GPIO_AFSEL1_SEL14_AF11               (GPIO_AFSEL1_SEL14_CFG(11))                         /*!< Select alternate function 11 */
#define GPIO_AFSEL1_SEL14_AF12               (GPIO_AFSEL1_SEL14_CFG(12))                         /*!< Select alternate function 12 */
#define GPIO_AFSEL1_SEL14_AF13               (GPIO_AFSEL1_SEL14_CFG(13))                         /*!< Select alternate function 13 */
#define GPIO_AFSEL1_SEL14_AF14               (GPIO_AFSEL1_SEL14_CFG(14))                         /*!< Select alternate function 14 */
#define GPIO_AFSEL1_SEL14_AF15               (GPIO_AFSEL1_SEL14_CFG(15))                         /*!< Select alternate function 15 */

/* GPIO_AFSEL1_SEL15 */
#define GPIO_AFSEL1_SEL15_POS                (28U)                                               /*!< GPIO_AFSEL1_SEL15 bit offset */
#define GPIO_AFSEL1_SEL15_CFG(regval)  (GPIO_AFSEL1_SEL15 & ((uint32_t)(regval) << GPIO_AFSEL1_SEL15_POS))
#define GPIO_AFSEL1_SEL15_AF0                (GPIO_AFSEL1_SEL15_CFG(0))                          /*!< Select alternate function 0 */
#define GPIO_AFSEL1_SEL15_AF1                (GPIO_AFSEL1_SEL15_CFG(1))                          /*!< Select alternate function 1 */
#define GPIO_AFSEL1_SEL15_AF2                (GPIO_AFSEL1_SEL15_CFG(2))                          /*!< Select alternate function 2 */
#define GPIO_AFSEL1_SEL15_AF3                (GPIO_AFSEL1_SEL15_CFG(3))                          /*!< Select alternate function 3 */
#define GPIO_AFSEL1_SEL15_AF4                (GPIO_AFSEL1_SEL15_CFG(4))                          /*!< Select alternate function 4 */
#define GPIO_AFSEL1_SEL15_AF5                (GPIO_AFSEL1_SEL15_CFG(5))                          /*!< Select alternate function 5 */
#define GPIO_AFSEL1_SEL15_AF6                (GPIO_AFSEL1_SEL15_CFG(6))                          /*!< Select alternate function 6 */
#define GPIO_AFSEL1_SEL15_AF7                (GPIO_AFSEL1_SEL15_CFG(7))                          /*!< Select alternate function 7 */
#define GPIO_AFSEL1_SEL15_AF8                (GPIO_AFSEL1_SEL15_CFG(8))                          /*!< Select alternate function 8 */
#define GPIO_AFSEL1_SEL15_AF9                (GPIO_AFSEL1_SEL15_CFG(9))                          /*!< Select alternate function 9 */
#define GPIO_AFSEL1_SEL15_AF10               (GPIO_AFSEL1_SEL15_CFG(10))                         /*!< Select alternate function 10 */
#define GPIO_AFSEL1_SEL15_AF11               (GPIO_AFSEL1_SEL15_CFG(11))                         /*!< Select alternate function 11 */
#define GPIO_AFSEL1_SEL15_AF12               (GPIO_AFSEL1_SEL15_CFG(12))                         /*!< Select alternate function 12 */
#define GPIO_AFSEL1_SEL15_AF13               (GPIO_AFSEL1_SEL15_CFG(13))                         /*!< Select alternate function 13 */
#define GPIO_AFSEL1_SEL15_AF14               (GPIO_AFSEL1_SEL15_CFG(14))                         /*!< Select alternate function 14 */
#define GPIO_AFSEL1_SEL15_AF15               (GPIO_AFSEL1_SEL15_CFG(15))                         /*!< Select alternate function 15 */

/* GPIO_SET_SET0 */
#define GPIO_SET_SET0_POS                    (0U)                                                /*!< GPIO_SET_SET0 bit offset */

/* GPIO_SET_SET1 */
#define GPIO_SET_SET1_POS                    (1U)                                                /*!< GPIO_SET_SET1 bit offset */

/* GPIO_SET_SET2 */
#define GPIO_SET_SET2_POS                    (2U)                                                /*!< GPIO_SET_SET2 bit offset */

/* GPIO_SET_SET3 */
#define GPIO_SET_SET3_POS                    (3U)                                                /*!< GPIO_SET_SET3 bit offset */

/* GPIO_SET_SET4 */
#define GPIO_SET_SET4_POS                    (4U)                                                /*!< GPIO_SET_SET4 bit offset */

/* GPIO_SET_SET5 */
#define GPIO_SET_SET5_POS                    (5U)                                                /*!< GPIO_SET_SET5 bit offset */

/* GPIO_SET_SET6 */
#define GPIO_SET_SET6_POS                    (6U)                                                /*!< GPIO_SET_SET6 bit offset */

/* GPIO_SET_SET7 */
#define GPIO_SET_SET7_POS                    (7U)                                                /*!< GPIO_SET_SET7 bit offset */

/* GPIO_SET_SET8 */
#define GPIO_SET_SET8_POS                    (8U)                                                /*!< GPIO_SET_SET8 bit offset */

/* GPIO_SET_SET9 */
#define GPIO_SET_SET9_POS                    (9U)                                                /*!< GPIO_SET_SET9 bit offset */

/* GPIO_SET_SET10 */
#define GPIO_SET_SET10_POS                   (10U)                                               /*!< GPIO_SET_SET10 bit offset */

/* GPIO_SET_SET11 */
#define GPIO_SET_SET11_POS                   (11U)                                               /*!< GPIO_SET_SET11 bit offset */

/* GPIO_SET_SET12 */
#define GPIO_SET_SET12_POS                   (12U)                                               /*!< GPIO_SET_SET12 bit offset */

/* GPIO_SET_SET13 */
#define GPIO_SET_SET13_POS                   (13U)                                               /*!< GPIO_SET_SET13 bit offset */

/* GPIO_SET_SET14 */
#define GPIO_SET_SET14_POS                   (14U)                                               /*!< GPIO_SET_SET14 bit offset */

/* GPIO_SET_SET15 */
#define GPIO_SET_SET15_POS                   (15U)                                               /*!< GPIO_SET_SET15 bit offset */

/* GPIO_CLR_CLR0 */
#define GPIO_CLR_CLR0_POS                    (0U)                                                /*!< GPIO_CLR_CLR0 bit offset */

/* GPIO_CLR_CLR1 */
#define GPIO_CLR_CLR1_POS                    (1U)                                                /*!< GPIO_CLR_CLR1 bit offset */

/* GPIO_CLR_CLR2 */
#define GPIO_CLR_CLR2_POS                    (2U)                                                /*!< GPIO_CLR_CLR2 bit offset */

/* GPIO_CLR_CLR3 */
#define GPIO_CLR_CLR3_POS                    (3U)                                                /*!< GPIO_CLR_CLR3 bit offset */

/* GPIO_CLR_CLR4 */
#define GPIO_CLR_CLR4_POS                    (4U)                                                /*!< GPIO_CLR_CLR4 bit offset */

/* GPIO_CLR_CLR5 */
#define GPIO_CLR_CLR5_POS                    (5U)                                                /*!< GPIO_CLR_CLR5 bit offset */

/* GPIO_CLR_CLR6 */
#define GPIO_CLR_CLR6_POS                    (6U)                                                /*!< GPIO_CLR_CLR6 bit offset */

/* GPIO_CLR_CLR7 */
#define GPIO_CLR_CLR7_POS                    (7U)                                                /*!< GPIO_CLR_CLR7 bit offset */

/* GPIO_CLR_CLR8 */
#define GPIO_CLR_CLR8_POS                    (8U)                                                /*!< GPIO_CLR_CLR8 bit offset */

/* GPIO_CLR_CLR9 */
#define GPIO_CLR_CLR9_POS                    (9U)                                                /*!< GPIO_CLR_CLR9 bit offset */

/* GPIO_CLR_CLR10 */
#define GPIO_CLR_CLR10_POS                   (10U)                                               /*!< GPIO_CLR_CLR10 bit offset */

/* GPIO_CLR_CLR11 */
#define GPIO_CLR_CLR11_POS                   (11U)                                               /*!< GPIO_CLR_CLR11 bit offset */

/* GPIO_CLR_CLR12 */
#define GPIO_CLR_CLR12_POS                   (12U)                                               /*!< GPIO_CLR_CLR12 bit offset */

/* GPIO_CLR_CLR13 */
#define GPIO_CLR_CLR13_POS                   (13U)                                               /*!< GPIO_CLR_CLR13 bit offset */

/* GPIO_CLR_CLR14 */
#define GPIO_CLR_CLR14_POS                   (14U)                                               /*!< GPIO_CLR_CLR14 bit offset */

/* GPIO_CLR_CLR15 */
#define GPIO_CLR_CLR15_POS                   (15U)                                               /*!< GPIO_CLR_CLR15 bit offset */

/* GPIO_TG_TG0 */
#define GPIO_TG_TG0_POS                      (0U)                                                /*!< GPIO_TG_TG0 bit offset */

/* GPIO_TG_TG1 */
#define GPIO_TG_TG1_POS                      (1U)                                                /*!< GPIO_TG_TG1 bit offset */

/* GPIO_TG_TG2 */
#define GPIO_TG_TG2_POS                      (2U)                                                /*!< GPIO_TG_TG2 bit offset */

/* GPIO_TG_TG3 */
#define GPIO_TG_TG3_POS                      (3U)                                                /*!< GPIO_TG_TG3 bit offset */

/* GPIO_TG_TG4 */
#define GPIO_TG_TG4_POS                      (4U)                                                /*!< GPIO_TG_TG4 bit offset */

/* GPIO_TG_TG5 */
#define GPIO_TG_TG5_POS                      (5U)                                                /*!< GPIO_TG_TG5 bit offset */

/* GPIO_TG_TG6 */
#define GPIO_TG_TG6_POS                      (6U)                                                /*!< GPIO_TG_TG6 bit offset */

/* GPIO_TG_TG7 */
#define GPIO_TG_TG7_POS                      (7U)                                                /*!< GPIO_TG_TG7 bit offset */

/* GPIO_TG_TG8 */
#define GPIO_TG_TG8_POS                      (8U)                                                /*!< GPIO_TG_TG8 bit offset */

/* GPIO_TG_TG9 */
#define GPIO_TG_TG9_POS                      (9U)                                                /*!< GPIO_TG_TG9 bit offset */

/* GPIO_TG_TG10 */
#define GPIO_TG_TG10_POS                     (10U)                                               /*!< GPIO_TG_TG10 bit offset */

/* GPIO_TG_TG11 */
#define GPIO_TG_TG11_POS                     (11U)                                               /*!< GPIO_TG_TG11 bit offset */

/* GPIO_TG_TG12 */
#define GPIO_TG_TG12_POS                     (12U)                                               /*!< GPIO_TG_TG12 bit offset */

/* GPIO_TG_TG13 */
#define GPIO_TG_TG13_POS                     (13U)                                               /*!< GPIO_TG_TG13 bit offset */

/* GPIO_TG_TG14 */
#define GPIO_TG_TG14_POS                     (14U)                                               /*!< GPIO_TG_TG14 bit offset */

/* GPIO_TG_TG15 */
#define GPIO_TG_TG15_POS                     (15U)                                               /*!< GPIO_TG_TG15 bit offset */

/* GPIO_DBG_KEY_KEY */
#define GPIO_DBG_KEY_KEY_POS                 (0U)                                                /*!< GPIO_DBG_KEY_KEY bit offset */

/* GPIO_DBG_JTDO_EN */
#define GPIO_DBG_JTDO_EN_POS                 (3U)                                                /*!< GPIO_DBG_JTDO_EN bit offset */

/* GPIO_DBG_JTDI_EN */
#define GPIO_DBG_JTDI_EN_POS                 (2U)                                                /*!< GPIO_DBG_JTDI_EN bit offset */

/* GPIO_DBG_JTMS_EN */
#define GPIO_DBG_JTMS_EN_POS                 (1U)                                                /*!< GPIO_DBG_JTMS_EN bit offset */

/* GPIO_DBG_JTCK_EN */
#define GPIO_DBG_JTCK_EN_POS                 (0U)                                                /*!< GPIO_DBG_JTCK_EN bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_GPIO_H */
