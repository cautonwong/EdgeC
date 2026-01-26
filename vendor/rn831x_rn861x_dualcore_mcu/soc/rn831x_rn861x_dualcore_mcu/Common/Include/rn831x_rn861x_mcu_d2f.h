/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_d2f.h
 * @author  Renergy Software Team
 * @brief   D2F module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_D2F_H
#define _RN831x_RN861x_MCU_D2F_H

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

/* D2F */
typedef struct
{
    __IO uint32_t HFCONST0;         /*!< offset: 0x000 info: High frequency pulse constant  of E00~E03 */
    __IO uint32_t HFCONST1;         /*!< offset: 0x004 info: High frequency pulse constant  of E04~E07 */
    __IO uint32_t HFCONST2;         /*!< offset: 0x008 info: High frequency pulse constant  of E08~E11 */
    __IO uint32_t RESERVED0[9];     /*!< offset: 0x00C ~ 02C */
    __IO uint32_t CTRL;             /*!< offset: 0x030 info: D2F control register */
    __IO uint32_t CFG;              /*!< offset: 0x034 info: D2F configeration register */
    __IO uint32_t RESERVED1[2];     /*!< offset: 0x038 ~ 03C */
    __IO uint32_t CF_CFG;           /*!< offset: 0x040 info: CF0~CF2 configeration register */
    __IO uint32_t RESERVED2[3];     /*!< offset: 0x044 ~ 04C */
    __IO uint32_t IE;               /*!< offset: 0x050 info: D2F interrupt enable register */
    __IO uint32_t IF;               /*!< offset: 0x054 info: D2F interrupt flag register */
    __IO uint32_t NOLOAD;           /*!< offset: 0x058 info: No-Load creeping status register */
    __IO uint32_t RESERVED3;        /*!< offset: 0x05C */
    __IO uint32_t D2FP[12];         /*!< offset: 0x060 ~ 0x08C info: Input power of number 00 ~ 11 */
    __IO uint32_t RESERVED4[36];    /*!< offset: 0x090 ~ 11C */
    __IO uint32_t D2FE[12];         /*!< offset: 0x120 ~ 0x14C info: Output energy of number 00 ~ 11 */
    __IO uint32_t RESERVED5[36];    /*!< offset: 0x150 ~ 1DC */
    __IO uint32_t STARTTH00;        /*!< offset: 0x1E0 info:Start threshold of D2FP00~D2FP03 */
    __IO uint32_t STARTTH01;        /*!< offset: 0x1E4 info:Start threshold of D2FP04~D2FP07 */
    __IO uint32_t STARTTH02;        /*!< offset: 0x1E8 info:Start threshold of D2FP08~D2FP11 */
} D2F_TypeDef;

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
#define D2F_CORE0_BASE  (0x2007C000U)
#define D2F_CORE1_BASE  (0xA007C000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define D2F_CORE0  ((D2F_TypeDef *)      D2F_CORE0_BASE)
#define D2F_CORE1  ((D2F_TypeDef *)      D2F_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define D2F  D2F_CORE0
#endif
#if defined(CORE1)
#define D2F  D2F_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** D2F Instances *******************************/
#define IS_D2F_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == D2F_CORE0) || \
                                          ((INSTANCE) == D2F_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* D2F_HFCONST0 */
#define D2F_HFCONST0_HFCONST0                (BITS(0,15))                                        /*!< High frequency pulse constant bits of E00~E03 */

/* D2F_HFCONST1 */
#define D2F_HFCONST1_HFCONST1                (BITS(0,15))                                        /*!< High frequency pulse constant bits of E04~E07 */

/* D2F_HFCONST2 */
#define D2F_HFCONST2_HFCONST2                (BITS(0,15))                                        /*!< High frequency pulse constant bits of E08~E11 */

/* D2F_CTRL */
#define D2F_CTRL_EN                          (BIT(0))                                            /*!< D2F enable bit */

/* D2F_CFG */
#define D2F_CFG_ENGTYPE                      (BIT(0))                                            /*!< Energy register type bit */
#define D2F_CFG_LOADTYPE                     (BIT(1))                                            /*!< Power load type bit */
#define D2F_CFG_CLK_SEL                      (BIT(2))                                            /*!< Integrator clk select bit */

/* D2F_CF_CFG */
#define D2F_CF_CFG_CF0_SEL                   (BITS(0,1))                                         /*!< Select bit for D2F channel of CF0 */
#define D2F_CF_CFG_CF1_SEL                   (BITS(4,5))                                         /*!< Select bit for D2F channel of CF1 */
#define D2F_CF_CFG_CF2_SEL                   (BITS(8,9))                                         /*!< Select bit for D2F channel of CF2 */

/* D2F_IE */
#define D2F_IE_D2F00_IE                      (BIT(0))                                            /*!< D2F00 interrupt enable bit */
#define D2F_IE_D2F01_IE                      (BIT(1))                                            /*!< D2F01 interrupt enable bit */
#define D2F_IE_D2F02_IE                      (BIT(2))                                            /*!< D2F02 interrupt enable bit */
#define D2F_IE_D2F03_IE                      (BIT(3))                                            /*!< D2F03 interrupt enable bit */
#define D2F_IE_D2F04_IE                      (BIT(4))                                            /*!< D2F04 interrupt enable bit */
#define D2F_IE_D2F05_IE                      (BIT(5))                                            /*!< D2F05 interrupt enable bit */
#define D2F_IE_D2F06_IE                      (BIT(6))                                            /*!< D2F06 interrupt enable bit */
#define D2F_IE_D2F07_IE                      (BIT(7))                                            /*!< D2F07 interrupt enable bit */
#define D2F_IE_D2F08_IE                      (BIT(8))                                            /*!< D2F08 interrupt enable bit */
#define D2F_IE_D2F09_IE                      (BIT(9))                                            /*!< D2F09 interrupt enable bit */
#define D2F_IE_D2F10_IE                      (BIT(10))                                           /*!< D2F10 interrupt enable bit */
#define D2F_IE_D2F11_IE                      (BIT(11))                                           /*!< D2F11 interrupt enable bit */
#define D2F_IE_PCOV_IE                       (BIT(24))                                           /*!< Power cover interrupt enable bit */

/* D2F_IF */
#define D2F_IF_D2F00_IF                      (BIT(0))                                            /*!< D2F00 interrupt flag bit */
#define D2F_IF_D2F01_IF                      (BIT(1))                                            /*!< D2F01 interrupt flag bit */
#define D2F_IF_D2F02_IF                      (BIT(2))                                            /*!< D2F02 interrupt flag bit */
#define D2F_IF_D2F03_IF                      (BIT(3))                                            /*!< D2F03 interrupt flag bit */
#define D2F_IF_D2F04_IF                      (BIT(4))                                            /*!< D2F04 interrupt flag bit */
#define D2F_IF_D2F05_IF                      (BIT(5))                                            /*!< D2F05 interrupt flag bit */
#define D2F_IF_D2F06_IF                      (BIT(6))                                            /*!< D2F06 interrupt flag bit */
#define D2F_IF_D2F07_IF                      (BIT(7))                                            /*!< D2F07 interrupt flag bit */
#define D2F_IF_D2F08_IF                      (BIT(8))                                            /*!< D2F08 interrupt flag bit */
#define D2F_IF_D2F09_IF                      (BIT(9))                                            /*!< D2F09 interrupt flag bit */
#define D2F_IF_D2F10_IF                      (BIT(10))                                           /*!< D2F10 interrupt flag bit */
#define D2F_IF_D2F11_IF                      (BIT(11))                                           /*!< D2F11 interrupt flag bit */
#define D2F_IF_PCOV_IF                       (BIT(24))                                           /*!< Power cover interrupt flag bit */

/* D2F_NOLOAD */
#define D2F_NOLOAD_D2F00_NLD                 (BIT(0))                                            /*!< No-Load status flag of D2F00 */
#define D2F_NOLOAD_D2F01_NLD                 (BIT(1))                                            /*!< No-Load status flag of D2F01 */
#define D2F_NOLOAD_D2F02_NLD                 (BIT(2))                                            /*!< No-Load status flag of D2F02 */
#define D2F_NOLOAD_D2F03_NLD                 (BIT(3))                                            /*!< No-Load status flag of D2F03 */
#define D2F_NOLOAD_D2F04_NLD                 (BIT(4))                                            /*!< No-Load status flag of D2F04 */
#define D2F_NOLOAD_D2F05_NLD                 (BIT(5))                                            /*!< No-Load status flag of D2F05 */
#define D2F_NOLOAD_D2F06_NLD                 (BIT(6))                                            /*!< No-Load status flag of D2F06 */
#define D2F_NOLOAD_D2F07_NLD                 (BIT(7))                                            /*!< No-Load status flag of D2F07 */
#define D2F_NOLOAD_D2F08_NLD                 (BIT(8))                                            /*!< No-Load status flag of D2F08 */
#define D2F_NOLOAD_D2F09_NLD                 (BIT(9))                                            /*!< No-Load status flag of D2F09 */
#define D2F_NOLOAD_D2F10_NLD                 (BIT(10))                                           /*!< No-Load status flag of D2F10 */
#define D2F_NOLOAD_D2F11_NLD                 (BIT(11))                                           /*!< No-Load status flag of D2F11 */

/* D2F_D2FP00 */
#define D2F_D2FP00_D2FP00                    (BITS(0,31))                                        /*!< Input power bits of D2F00 */

/* D2F_D2FP01 */
#define D2F_D2FP01_D2FP01                    (BITS(0,31))                                        /*!< Input power bits of D2F01 */

/* D2F_D2FP02 */
#define D2F_D2FP02_D2FP02                    (BITS(0,31))                                        /*!< Input power bits of D2F02 */

/* D2F_D2FP03 */
#define D2F_D2FP03_D2FP03                    (BITS(0,31))                                        /*!< Input power bits of D2F03 */

/* D2F_D2FP04 */
#define D2F_D2FP04_D2FP04                    (BITS(0,31))                                        /*!< Input power bits of D2F04 */

/* D2F_D2FP05 */
#define D2F_D2FP05_D2FP05                    (BITS(0,31))                                        /*!< Input power bits of D2F05 */

/* D2F_D2FP06 */
#define D2F_D2FP06_D2FP06                    (BITS(0,31))                                        /*!< Input power bits of D2F06 */

/* D2F_D2FP07 */
#define D2F_D2FP07_D2FP07                    (BITS(0,31))                                        /*!< Input power bits of D2F07 */

/* D2F_D2FP08 */
#define D2F_D2FP08_D2FP08                    (BITS(0,31))                                        /*!< Input power bits of D2F08 */

/* D2F_D2FP09 */
#define D2F_D2FP09_D2FP09                    (BITS(0,31))                                        /*!< Input power bits of D2F09 */

/* D2F_D2FP10 */
#define D2F_D2FP10_D2FP10                    (BITS(0,31))                                        /*!< Input power bits of D2F10 */

/* D2F_D2FP11 */
#define D2F_D2FP11_D2FP11                    (BITS(0,31))                                        /*!< Input power bits of D2F11 */

/* D2F_D2FE00 */
#define D2F_D2FE00_D2FE00                    (BITS(0,31))                                        /*!< Output energy bits of D2F00 */

/* D2F_D2FE01 */
#define D2F_D2FE01_D2FE01                    (BITS(0,31))                                        /*!< Output energy bits of D2F01 */

/* D2F_D2FE02 */
#define D2F_D2FE02_D2FE02                    (BITS(0,31))                                        /*!< Output energy bits of D2F02 */

/* D2F_D2FE03 */
#define D2F_D2FE03_D2FE03                    (BITS(0,31))                                        /*!< Output energy bits of D2F03 */

/* D2F_D2FE04 */
#define D2F_D2FE04_D2FE04                    (BITS(0,31))                                        /*!< Output energy bits of D2F04 */

/* D2F_D2FE05 */
#define D2F_D2FE05_D2FE05                    (BITS(0,31))                                        /*!< Output energy bits of D2F05 */

/* D2F_D2FE06 */
#define D2F_D2FE06_D2FE06                    (BITS(0,31))                                        /*!< Output energy bits of D2F06 */

/* D2F_D2FE07 */
#define D2F_D2FE07_D2FE07                    (BITS(0,31))                                        /*!< Output energy bits of D2F07 */

/* D2F_D2FE08 */
#define D2F_D2FE08_D2FE08                    (BITS(0,31))                                        /*!< Output energy bits of D2F08 */

/* D2F_D2FE09 */
#define D2F_D2FE09_D2FE09                    (BITS(0,31))                                        /*!< Output energy bits of D2F09 */

/* D2F_D2FE10 */
#define D2F_D2FE10_D2FE10                    (BITS(0,31))                                        /*!< Output energy bits of D2F10 */

/* D2F_D2FE11 */
#define D2F_D2FE11_D2FE11                    (BITS(0,31))                                        /*!< Output energy bits of D2F11 */

/* D2F_STARTTH00 */
#define D2F_STARTTH00_STARTTH00              (BITS(0,30))                                        /*!< Start threshold bits of D2FP00~D2FP03 */

/* D2F_STARTTH01 */
#define D2F_STARTTH01_STARTTH01              (BITS(0,30))                                        /*!< Start threshold bits of D2FP04~D2FP07 */

/* D2F_STARTTH02 */
#define D2F_STARTTH02_STARTTH02              (BITS(0,30))                                        /*!< Start threshold bits of D2FP08~D2FP11 */

/* constants definitions */
/* D2F_HFCONST0_HFCONST0 */
#define D2F_HFCONST0_HFCONST0_POS            (0U)                                                /*!< D2F_HFCONST0_HFCONST0 bit offset */

/* D2F_HFCONST1_HFCONST1 */
#define D2F_HFCONST1_HFCONST1_POS            (0U)                                                /*!< D2F_HFCONST1_HFCONST1 bit offset */

/* D2F_HFCONST2_HFCONST2 */
#define D2F_HFCONST2_HFCONST2_POS            (0U)                                                /*!< D2F_HFCONST2_HFCONST2 bit offset */

/* D2F_CTRL_EN */
#define D2F_CTRL_EN_POS                      (0U)                                                /*!< D2F_CTRL_EN bit offset */

/* D2F_CFG_ENGTYPE */
#define D2F_CFG_ENGTYPE_POS                  (0U)                                                /*!< D2F_CFG_ENGTYPE bit offset */
#define D2F_CFG_ENGTYPE_R2CLR                ((uint32_t)0x00000000U)                             /*!< read to clear */
#define D2F_CFG_ENGTYPE_COUNTER              (D2F_CFG_ENGTYPE)                                   /*!< counter after read */

/* D2F_CFG_LOADTYPE */
#define D2F_CFG_LOADTYPE_POS                 (1U)                                                /*!< D2F_CFG_LOADTYPE bit offset */
#define D2F_CFG_LOADTYPE_NORMAL              ((uint32_t)0x00000000U)                             /*!< Normal load type of power */
#define D2F_CFG_LOADTYPE_SYN_SIMPTC          (D2F_CFG_LOADTYPE)                                  /*!< Synchronized loading type on SimpTC */

/* D2F_CFG_CLK_SEL */
#define D2F_CFG_CLK_SEL_POS                  (2U)                                                /*!< D2F_CFG_CLK_SEL bit offset */
#define D2F_CFG_CLK_SEL_DEFAULT              ((uint32_t)0x00000000U)                             /*!< Select default D2F clock as integrator clock */
#define D2F_CFG_CLK_SEL_CONSTANT             (D2F_CFG_CLK_SEL)                                   /*!< Select constant D2F clock as integrator clock */

/* D2F_CF_CFG_CF0_SEL */
#define D2F_CF_CFG_CF0_SEL_POS               (0U)                                                /*!< D2F_CF_CFG_CF0_SEL bit offset */
#define D2F_CF_CFG_CF0_SEL_CFG(regval)  (D2F_CF_CFG_CF0_SEL & ((uint32_t)(regval) << D2F_CF_CFG_CF0_SEL_POS))
#define D2F_CF_CFG_CF0_SEL_D2F00             (D2F_CF_CFG_CF0_SEL_CFG(0))                         /*!< Select D2F00 channel as CF0 */
#define D2F_CF_CFG_CF0_SEL_D2F01             (D2F_CF_CFG_CF0_SEL_CFG(1))                         /*!< Select D2F01 channel as CF0 */
#define D2F_CF_CFG_CF0_SEL_D2F02             (D2F_CF_CFG_CF0_SEL_CFG(2))                         /*!< Select D2F02 channel as CF0 */
#define D2F_CF_CFG_CF0_SEL_D2F03             (D2F_CF_CFG_CF0_SEL_CFG(3))                         /*!< Select D2F03 channel as CF0 */

/* D2F_CF_CFG_CF1_SEL */
#define D2F_CF_CFG_CF1_SEL_POS               (4U)                                                /*!< D2F_CF_CFG_CF1_SEL bit offset */
#define D2F_CF_CFG_CF1_SEL_CFG(regval)  (D2F_CF_CFG_CF1_SEL & ((uint32_t)(regval) << D2F_CF_CFG_CF1_SEL_POS))
#define D2F_CF_CFG_CF1_SEL_D2F04             (D2F_CF_CFG_CF1_SEL_CFG(0))                         /*!< Select D2F04 channel as CF1 */
#define D2F_CF_CFG_CF1_SEL_D2F05             (D2F_CF_CFG_CF1_SEL_CFG(1))                         /*!< Select D2F05 channel as CF1 */
#define D2F_CF_CFG_CF1_SEL_D2F06             (D2F_CF_CFG_CF1_SEL_CFG(2))                         /*!< Select D2F06 channel as CF1 */
#define D2F_CF_CFG_CF1_SEL_D2F07             (D2F_CF_CFG_CF1_SEL_CFG(3))                         /*!< Select D2F07 channel as CF1 */

/* D2F_CF_CFG_CF2_SEL */
#define D2F_CF_CFG_CF2_SEL_POS               (8U)                                                /*!< D2F_CF_CFG_CF2_SEL bit offset */
#define D2F_CF_CFG_CF2_SEL_CFG(regval)  (D2F_CF_CFG_CF2_SEL & ((uint32_t)(regval) << D2F_CF_CFG_CF2_SEL_POS))
#define D2F_CF_CFG_CF2_SEL_D2F08             (D2F_CF_CFG_CF2_SEL_CFG(0))                         /*!< Select D2F08 channel as CF2 */
#define D2F_CF_CFG_CF2_SEL_D2F09             (D2F_CF_CFG_CF2_SEL_CFG(1))                         /*!< Select D2F09 channel as CF2 */
#define D2F_CF_CFG_CF2_SEL_D2F10             (D2F_CF_CFG_CF2_SEL_CFG(2))                         /*!< Select D2F10 channel as CF2 */
#define D2F_CF_CFG_CF2_SEL_D2F11             (D2F_CF_CFG_CF2_SEL_CFG(3))                         /*!< Select D2F11 channel as CF2 */

/* D2F_IE_D2F00_IE */
#define D2F_IE_D2F00_IE_POS                  (0U)                                                /*!< D2F_IE_D2F00_IE bit offset */

/* D2F_IE_D2F01_IE */
#define D2F_IE_D2F01_IE_POS                  (1U)                                                /*!< D2F_IE_D2F01_IE bit offset */

/* D2F_IE_D2F02_IE */
#define D2F_IE_D2F02_IE_POS                  (2U)                                                /*!< D2F_IE_D2F02_IE bit offset */

/* D2F_IE_D2F03_IE */
#define D2F_IE_D2F03_IE_POS                  (3U)                                                /*!< D2F_IE_D2F03_IE bit offset */

/* D2F_IE_D2F04_IE */
#define D2F_IE_D2F04_IE_POS                  (4U)                                                /*!< D2F_IE_D2F04_IE bit offset */

/* D2F_IE_D2F05_IE */
#define D2F_IE_D2F05_IE_POS                  (5U)                                                /*!< D2F_IE_D2F05_IE bit offset */

/* D2F_IE_D2F06_IE */
#define D2F_IE_D2F06_IE_POS                  (6U)                                                /*!< D2F_IE_D2F06_IE bit offset */

/* D2F_IE_D2F07_IE */
#define D2F_IE_D2F07_IE_POS                  (7U)                                                /*!< D2F_IE_D2F07_IE bit offset */

/* D2F_IE_D2F08_IE */
#define D2F_IE_D2F08_IE_POS                  (8U)                                                /*!< D2F_IE_D2F08_IE bit offset */

/* D2F_IE_D2F09_IE */
#define D2F_IE_D2F09_IE_POS                  (9U)                                                /*!< D2F_IE_D2F09_IE bit offset */

/* D2F_IE_D2F10_IE */
#define D2F_IE_D2F10_IE_POS                  (10U)                                               /*!< D2F_IE_D2F10_IE bit offset */

/* D2F_IE_D2F11_IE */
#define D2F_IE_D2F11_IE_POS                  (11U)                                               /*!< D2F_IE_D2F11_IE bit offset */

/* D2F_IE_PCOV_IE */
#define D2F_IE_PCOV_IE_POS                   (24U)                                               /*!< D2F_IE_PCOV_IE bit offset */

/* D2F_IF_D2F00_IF */
#define D2F_IF_D2F00_IF_POS                  (0U)                                                /*!< D2F_IF_D2F00_IF bit offset */

/* D2F_IF_D2F01_IF */
#define D2F_IF_D2F01_IF_POS                  (1U)                                                /*!< D2F_IF_D2F01_IF bit offset */

/* D2F_IF_D2F02_IF */
#define D2F_IF_D2F02_IF_POS                  (2U)                                                /*!< D2F_IF_D2F02_IF bit offset */

/* D2F_IF_D2F03_IF */
#define D2F_IF_D2F03_IF_POS                  (3U)                                                /*!< D2F_IF_D2F03_IF bit offset */

/* D2F_IF_D2F04_IF */
#define D2F_IF_D2F04_IF_POS                  (4U)                                                /*!< D2F_IF_D2F04_IF bit offset */

/* D2F_IF_D2F05_IF */
#define D2F_IF_D2F05_IF_POS                  (5U)                                                /*!< D2F_IF_D2F05_IF bit offset */

/* D2F_IF_D2F06_IF */
#define D2F_IF_D2F06_IF_POS                  (6U)                                                /*!< D2F_IF_D2F06_IF bit offset */

/* D2F_IF_D2F07_IF */
#define D2F_IF_D2F07_IF_POS                  (7U)                                                /*!< D2F_IF_D2F07_IF bit offset */

/* D2F_IF_D2F08_IF */
#define D2F_IF_D2F08_IF_POS                  (8U)                                                /*!< D2F_IF_D2F08_IF bit offset */

/* D2F_IF_D2F09_IF */
#define D2F_IF_D2F09_IF_POS                  (9U)                                                /*!< D2F_IF_D2F09_IF bit offset */

/* D2F_IF_D2F10_IF */
#define D2F_IF_D2F10_IF_POS                  (10U)                                               /*!< D2F_IF_D2F10_IF bit offset */

/* D2F_IF_D2F11_IF */
#define D2F_IF_D2F11_IF_POS                  (11U)                                               /*!< D2F_IF_D2F11_IF bit offset */

/* D2F_IF_PCOV_IF */
#define D2F_IF_PCOV_IF_POS                   (24U)                                               /*!< D2F_IF_PCOV_IF bit offset */

/* D2F_NOLOAD_D2F00_NLD */
#define D2F_NOLOAD_D2F00_NLD_POS             (0U)                                                /*!< D2F_NOLOAD_D2F00_NLD bit offset */

/* D2F_NOLOAD_D2F01_NLD */
#define D2F_NOLOAD_D2F01_NLD_POS             (1U)                                                /*!< D2F_NOLOAD_D2F01_NLD bit offset */

/* D2F_NOLOAD_D2F02_NLD */
#define D2F_NOLOAD_D2F02_NLD_POS             (2U)                                                /*!< D2F_NOLOAD_D2F02_NLD bit offset */

/* D2F_NOLOAD_D2F03_NLD */
#define D2F_NOLOAD_D2F03_NLD_POS             (3U)                                                /*!< D2F_NOLOAD_D2F03_NLD bit offset */

/* D2F_NOLOAD_D2F04_NLD */
#define D2F_NOLOAD_D2F04_NLD_POS             (4U)                                                /*!< D2F_NOLOAD_D2F04_NLD bit offset */

/* D2F_NOLOAD_D2F05_NLD */
#define D2F_NOLOAD_D2F05_NLD_POS             (5U)                                                /*!< D2F_NOLOAD_D2F05_NLD bit offset */

/* D2F_NOLOAD_D2F06_NLD */
#define D2F_NOLOAD_D2F06_NLD_POS             (6U)                                                /*!< D2F_NOLOAD_D2F06_NLD bit offset */

/* D2F_NOLOAD_D2F07_NLD */
#define D2F_NOLOAD_D2F07_NLD_POS             (7U)                                                /*!< D2F_NOLOAD_D2F07_NLD bit offset */

/* D2F_NOLOAD_D2F08_NLD */
#define D2F_NOLOAD_D2F08_NLD_POS             (8U)                                                /*!< D2F_NOLOAD_D2F08_NLD bit offset */

/* D2F_NOLOAD_D2F09_NLD */
#define D2F_NOLOAD_D2F09_NLD_POS             (9U)                                                /*!< D2F_NOLOAD_D2F09_NLD bit offset */

/* D2F_NOLOAD_D2F10_NLD */
#define D2F_NOLOAD_D2F10_NLD_POS             (10U)                                               /*!< D2F_NOLOAD_D2F10_NLD bit offset */

/* D2F_NOLOAD_D2F11_NLD */
#define D2F_NOLOAD_D2F11_NLD_POS             (11U)                                               /*!< D2F_NOLOAD_D2F11_NLD bit offset */

/* D2F_D2FP00_D2FP00 */
#define D2F_D2FP00_D2FP00_POS                (0U)                                                /*!< D2F_D2FP00_D2FP00 bit offset */

/* D2F_D2FP01_D2FP01 */
#define D2F_D2FP01_D2FP01_POS                (0U)                                                /*!< D2F_D2FP01_D2FP01 bit offset */

/* D2F_D2FP02_D2FP02 */
#define D2F_D2FP02_D2FP02_POS                (0U)                                                /*!< D2F_D2FP02_D2FP02 bit offset */

/* D2F_D2FP03_D2FP03 */
#define D2F_D2FP03_D2FP03_POS                (0U)                                                /*!< D2F_D2FP03_D2FP03 bit offset */

/* D2F_D2FP04_D2FP04 */
#define D2F_D2FP04_D2FP04_POS                (0U)                                                /*!< D2F_D2FP04_D2FP04 bit offset */

/* D2F_D2FP05_D2FP05 */
#define D2F_D2FP05_D2FP05_POS                (0U)                                                /*!< D2F_D2FP05_D2FP05 bit offset */

/* D2F_D2FP06_D2FP06 */
#define D2F_D2FP06_D2FP06_POS                (0U)                                                /*!< D2F_D2FP06_D2FP06 bit offset */

/* D2F_D2FP07_D2FP07 */
#define D2F_D2FP07_D2FP07_POS                (0U)                                                /*!< D2F_D2FP07_D2FP07 bit offset */

/* D2F_D2FP08_D2FP08 */
#define D2F_D2FP08_D2FP08_POS                (0U)                                                /*!< D2F_D2FP08_D2FP08 bit offset */

/* D2F_D2FP09_D2FP09 */
#define D2F_D2FP09_D2FP09_POS                (0U)                                                /*!< D2F_D2FP09_D2FP09 bit offset */

/* D2F_D2FP10_D2FP10 */
#define D2F_D2FP10_D2FP10_POS                (0U)                                                /*!< D2F_D2FP10_D2FP10 bit offset */

/* D2F_D2FP11_D2FP11 */
#define D2F_D2FP11_D2FP11_POS                (0U)                                                /*!< D2F_D2FP11_D2FP11 bit offset */

/* D2F_D2FE00_D2FE00 */
#define D2F_D2FE00_D2FE00_POS                (0U)                                                /*!< D2F_D2FE00_D2FE00 bit offset */

/* D2F_D2FE01_D2FE01 */
#define D2F_D2FE01_D2FE01_POS                (0U)                                                /*!< D2F_D2FE01_D2FE01 bit offset */

/* D2F_D2FE02_D2FE02 */
#define D2F_D2FE02_D2FE02_POS                (0U)                                                /*!< D2F_D2FE02_D2FE02 bit offset */

/* D2F_D2FE03_D2FE03 */
#define D2F_D2FE03_D2FE03_POS                (0U)                                                /*!< D2F_D2FE03_D2FE03 bit offset */

/* D2F_D2FE04_D2FE04 */
#define D2F_D2FE04_D2FE04_POS                (0U)                                                /*!< D2F_D2FE04_D2FE04 bit offset */

/* D2F_D2FE05_D2FE05 */
#define D2F_D2FE05_D2FE05_POS                (0U)                                                /*!< D2F_D2FE05_D2FE05 bit offset */

/* D2F_D2FE06_D2FE06 */
#define D2F_D2FE06_D2FE06_POS                (0U)                                                /*!< D2F_D2FE06_D2FE06 bit offset */

/* D2F_D2FE07_D2FE07 */
#define D2F_D2FE07_D2FE07_POS                (0U)                                                /*!< D2F_D2FE07_D2FE07 bit offset */

/* D2F_D2FE08_D2FE08 */
#define D2F_D2FE08_D2FE08_POS                (0U)                                                /*!< D2F_D2FE08_D2FE08 bit offset */

/* D2F_D2FE09_D2FE09 */
#define D2F_D2FE09_D2FE09_POS                (0U)                                                /*!< D2F_D2FE09_D2FE09 bit offset */

/* D2F_D2FE10_D2FE10 */
#define D2F_D2FE10_D2FE10_POS                (0U)                                                /*!< D2F_D2FE10_D2FE10 bit offset */

/* D2F_D2FE11_D2FE11 */
#define D2F_D2FE11_D2FE11_POS                (0U)                                                /*!< D2F_D2FE11_D2FE11 bit offset */

/* D2F_STARTTH00_STARTTH00 */
#define D2F_STARTTH00_STARTTH00_POS          (0U)                                                /*!< D2F_STARTTH00_STARTTH00 bit offset */

/* D2F_STARTTH01_STARTTH01 */
#define D2F_STARTTH01_STARTTH01_POS          (0U)                                                /*!< D2F_STARTTH01_STARTTH01 bit offset */

/* D2F_STARTTH02_STARTTH02 */
#define D2F_STARTTH02_STARTTH02_POS          (0U)                                                /*!< D2F_STARTTH02_STARTTH02 bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_D2F_H */
