/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_gic.h
 * @author  Renergy Software Team
 * @brief   GIC module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_GIC_H
#define _RN831x_RN861x_MCU_GIC_H

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

/* GIC */
typedef struct
{
    __IO uint32_t CORE0_ICI_STA0;            /*!< offset: 0x000 info: CORE0 Inter-Core Interrupt Status Register 0 */
    __IO uint32_t CORE0_ICI_STA1;            /*!< offset: 0x004 info: CORE0 Inter-Core Interrupt Status Register 1 */
    __IO uint32_t RESERVED0[2];              /*!< offset: 0x008 ~ 00C */
    __IO uint32_t CORE1_ICI_STA0;            /*!< offset: 0x010 info: CORE1 Inter-Core Interrupt Status Register 0 */
    __IO uint32_t CORE1_ICI_STA1;            /*!< offset: 0x014 info: CORE1 Inter-Core Interrupt Status Register 1 */
    __IO uint32_t RESERVED1[2];              /*!< offset: 0x018 ~ 01C */
    __IO uint32_t ICI_GEN;                   /*!< offset: 0x020 info: Inter-Core Interrupt Control Register */
    __IO uint32_t CORE_IND_LOCK;             /*!< offset: 0x024 info: Core Interrupt Lock Register */
    __IO uint32_t CORE_NUM;                  /*!< offset: 0x028 info: Core Number Register */
    __IO uint32_t HART_NUM;                  /*!< offset: 0x02C info: HART Number Register */
    __IO uint32_t SOC_IRQ_NUM;               /*!< offset: 0x030 info: Interrupt Number Register */
    __IO uint32_t RESERVED2[3];              /*!< offset: 0x034 ~ 03C */
    __IO uint32_t INT20_IND_RESERVED17;      /*!< offset: 0x040 info: SOC Interrupt20 Indicator Register */
    __IO uint32_t INT21_IND_RESERVED18;      /*!< offset: 0x044 info: SOC Interrupt21 Indicator Register */
    __IO uint32_t INT22_IND_RESERVED19;      /*!< offset: 0x048 info: SOC Interrupt22 Indicator Register */
    __IO uint32_t INT23_IND_RESERVED20;      /*!< offset: 0x04C info: SOC Interrupt23 Indicator Register */
    __IO uint32_t INT24_IND_RESERVED21;      /*!< offset: 0x050 info: SOC Interrupt24 Indicator Register */
    __IO uint32_t INT25_IND_RESERVED22;      /*!< offset: 0x054 info: SOC Interrupt25 Indicator Register */
    __IO uint32_t INT26_IND_RESERVED23;      /*!< offset: 0x058 info: SOC Interrupt26 Indicator Register */
    __IO uint32_t INT27_IND_RESERVED24;      /*!< offset: 0x05C info: SOC Interrupt27 Indicator Register */
    __IO uint32_t INT28_IND_RESERVED25;      /*!< offset: 0x060 info: SOC Interrupt28 Indicator Register */
    __IO uint32_t INT29_IND_RESERVED26;      /*!< offset: 0x064 info: SOC Interrupt29 Indicator Register */
    __IO uint32_t INT30_IND_RESERVED27;      /*!< offset: 0x068 info: SOC Interrupt30 Indicator Register */
    __IO uint32_t INT31_IND_RESERVED28;      /*!< offset: 0x06C info: SOC Interrupt31 Indicator Register */
    __IO uint32_t INT32_IND_RESERVED29;      /*!< offset: 0x070 info: SOC Interrupt32 Indicator Register */
    __IO uint32_t INT33_IND_RESERVED30;      /*!< offset: 0x074 info: SOC Interrupt33 Indicator Register */
    __IO uint32_t INT34_IND_RESERVED31;      /*!< offset: 0x078 info: SOC Interrupt34 Indicator Register */
    __IO uint32_t INT35_IND_RESERVED32;      /*!< offset: 0x07C info: SOC Interrupt35 Indicator Register */
    __IO uint32_t INT36_IND_RESERVED33;      /*!< offset: 0x080 info: SOC Interrupt36 Indicator Register */
    __IO uint32_t INT37_IND_RESERVED34;      /*!< offset: 0x084 info: SOC Interrupt37 Indicator Register */
    __IO uint32_t INT38_IND_RESERVED35;      /*!< offset: 0x088 info: SOC Interrupt38 Indicator Register */
    __IO uint32_t INT39_IND_RESERVED36;      /*!< offset: 0x08C info: SOC Interrupt39 Indicator Register */
    __IO uint32_t INT40_IND_RESERVED37;      /*!< offset: 0x090 info: SOC Interrupt40 Indicator Register */
    __IO uint32_t INT41_IND_RESERVED38;      /*!< offset: 0x094 info: SOC Interrupt41 Indicator Register */
    __IO uint32_t INT42_IND_RESERVED39;      /*!< offset: 0x098 info: SOC Interrupt42 Indicator Register */
    __IO uint32_t INT43_IND_RESERVED40;      /*!< offset: 0x09C info: SOC Interrupt43 Indicator Register */
    __IO uint32_t INT44_IND_RESERVED41;      /*!< offset: 0x0A0 info: SOC Interrupt44 Indicator Register */
    __IO uint32_t INT45_IND_RESERVED42;      /*!< offset: 0x0A4 info: SOC Interrupt45 Indicator Register */
    __IO uint32_t INT46_IND_LCD;             /*!< offset: 0x0A8 info: SOC Interrupt46 Indicator Register */
    __IO uint32_t INT47_IND_KBI1_IRQ0;       /*!< offset: 0x0AC info: SOC Interrupt47 Indicator Register */
    __IO uint32_t INT48_IND_KBI1_IRQ1;       /*!< offset: 0x0B0 info: SOC Interrupt48 Indicator Register */
    __IO uint32_t INT49_IND_KBI1_IRQ2;       /*!< offset: 0x0B4 info: SOC Interrupt49 Indicator Register */
    __IO uint32_t INT50_IND_KBI1_IRQ3;       /*!< offset: 0x0B8 info: SOC Interrupt50 Indicator Register */
    __IO uint32_t INT51_IND_KBI1_IRQ4;       /*!< offset: 0x0BC info: SOC Interrupt51 Indicator Register */
    __IO uint32_t INT52_IND_KBI1_IRQ5;       /*!< offset: 0x0C0 info: SOC Interrupt52 Indicator Register */
    __IO uint32_t INT53_IND_KBI1_IRQ6;       /*!< offset: 0x0C4 info: SOC Interrupt53 Indicator Register */
    __IO uint32_t INT54_IND_KBI1_IRQ7;       /*!< offset: 0x0C8 info: SOC Interrupt54 Indicator Register */
    __IO uint32_t INT55_IND_KBI1_IRQ8;       /*!< offset: 0x0CC info: SOC Interrupt55 Indicator Register */
    __IO uint32_t INT56_IND_KBI1_IRQ9;       /*!< offset: 0x0D0 info: SOC Interrupt56 Indicator Register */
    __IO uint32_t INT57_IND_KBI1_IRQ10;      /*!< offset: 0x0D4 info: SOC Interrupt57 Indicator Register */
    __IO uint32_t INT58_IND_KBI1_IRQ11;      /*!< offset: 0x0D8 info: SOC Interrupt58 Indicator Register */
    __IO uint32_t INT59_IND_KBI1_IRQ12;      /*!< offset: 0x0DC info: SOC Interrupt59 Indicator Register */
    __IO uint32_t INT60_IND_KBI1_IRQ13;      /*!< offset: 0x0E0 info: SOC Interrupt60 Indicator Register */
    __IO uint32_t INT61_IND_KBI1_IRQ14;      /*!< offset: 0x0E4 info: SOC Interrupt61 Indicator Register */
    __IO uint32_t INT62_IND_KBI1_IRQ15;      /*!< offset: 0x0E8 info: SOC Interrupt62 Indicator Register */
    __IO uint32_t INT63_IND_KBI1_IRQN;       /*!< offset: 0x0EC info: SOC Interrupt63 Indicator Register */
    __IO uint32_t INT64_IND_KBI0_IRQ0;       /*!< offset: 0x0F0 info: SOC Interrupt64 Indicator Register */
    __IO uint32_t INT65_IND_KBI0_IRQ1;       /*!< offset: 0x0F4 info: SOC Interrupt65 Indicator Register */
    __IO uint32_t INT66_IND_KBI0_IRQ2;       /*!< offset: 0x0F8 info: SOC Interrupt66 Indicator Register */
    __IO uint32_t INT67_IND_KBI0_IRQ3;       /*!< offset: 0x0FC info: SOC Interrupt67 Indicator Register */
    __IO uint32_t INT68_IND_KBI0_IRQ4;       /*!< offset: 0x100 info: SOC Interrupt68 Indicator Register */
    __IO uint32_t INT69_IND_KBI0_IRQ5;       /*!< offset: 0x104 info: SOC Interrupt69 Indicator Register */
    __IO uint32_t INT70_IND_KBI0_IRQ6;       /*!< offset: 0x108 info: SOC Interrupt70 Indicator Register */
    __IO uint32_t INT71_IND_KBI0_IRQ7;       /*!< offset: 0x10C info: SOC Interrupt71 Indicator Register */
    __IO uint32_t INT72_IND_KBI0_IRQ8;       /*!< offset: 0x110 info: SOC Interrupt72 Indicator Register */
    __IO uint32_t INT73_IND_KBI0_IRQ9;       /*!< offset: 0x114 info: SOC Interrupt73 Indicator Register */
    __IO uint32_t INT74_IND_KBI0_IRQ10;      /*!< offset: 0x118 info: SOC Interrupt74 Indicator Register */
    __IO uint32_t INT75_IND_KBI0_IRQ11;      /*!< offset: 0x11C info: SOC Interrupt75 Indicator Register */
    __IO uint32_t INT76_IND_KBI0_IRQ12;      /*!< offset: 0x120 info: SOC Interrupt76 Indicator Register */
    __IO uint32_t INT77_IND_KBI0_IRQ13;      /*!< offset: 0x124 info: SOC Interrupt77 Indicator Register */
    __IO uint32_t INT78_IND_KBI0_IRQ14;      /*!< offset: 0x128 info: SOC Interrupt78 Indicator Register */
    __IO uint32_t INT79_IND_KBI0_IRQ15;      /*!< offset: 0x12C info: SOC Interrupt79 Indicator Register */
    __IO uint32_t INT80_IND_KBI0_IRQN;       /*!< offset: 0x130 info: SOC Interrupt80 Indicator Register */
    __IO uint32_t INT81_IND_WDT0;            /*!< offset: 0x134 info: SOC Interrupt81 Indicator Register */
    __IO uint32_t INT82_IND_WDT1;            /*!< offset: 0x138 info: SOC Interrupt82 Indicator Register */
    __IO uint32_t INT83_IND_GDMA11;          /*!< offset: 0x13C info: SOC Interrupt83 Indicator Register */
    __IO uint32_t INT84_IND_GDMA10;          /*!< offset: 0x140 info: SOC Interrupt84 Indicator Register */
    __IO uint32_t INT85_IND_GDMA01;          /*!< offset: 0x144 info: SOC Interrupt85 Indicator Register */
    __IO uint32_t INT86_IND_GDMA00;          /*!< offset: 0x148 info: SOC Interrupt86 Indicator Register */
    __IO uint32_t RESERVED3;                 /*!< offset: 0x14C */
    __IO uint32_t INT88_IND_SIMPTC0_0;       /*!< offset: 0x150 info: SOC Interrupt88 Indicator Register */
    __IO uint32_t INT89_IND_SIMPTC0_1;       /*!< offset: 0x154 info: SOC Interrupt89 Indicator Register */
    __IO uint32_t INT90_IND_SIMPTC0_2;       /*!< offset: 0x158 info: SOC Interrupt90 Indicator Register */
    __IO uint32_t INT91_IND_SIMPTC0_3;       /*!< offset: 0x15C info: SOC Interrupt91 Indicator Register */
    __IO uint32_t INT92_IND_SIMPTC1_0;       /*!< offset: 0x160 info: SOC Interrupt92 Indicator Register */
    __IO uint32_t INT93_IND_SIMPTC1_1;       /*!< offset: 0x164 info: SOC Interrupt93 Indicator Register */
    __IO uint32_t INT94_IND_SIMPTC1_2;       /*!< offset: 0x168 info: SOC Interrupt94 Indicator Register */
    __IO uint32_t INT95_IND_SIMPTC1_3;       /*!< offset: 0x16C info: SOC Interrupt95 Indicator Register */
    __IO uint32_t INT96_IND_SIMPTC2_0;       /*!< offset: 0x170 info: SOC Interrupt96 Indicator Register */
    __IO uint32_t INT97_IND_SIMPTC2_1;       /*!< offset: 0x174 info: SOC Interrupt97 Indicator Register */
    __IO uint32_t INT98_IND_SIMPTC2_2;       /*!< offset: 0x178 info: SOC Interrupt98 Indicator Register */
    __IO uint32_t INT99_IND_SIMPTC2_3;       /*!< offset: 0x17C info: SOC Interrupt99 Indicator Register */
    __IO uint32_t INT100_IND_SIMPTC3_0;      /*!< offset: 0x180 info: SOC Interrupt100 Indicator Register */
    __IO uint32_t INT101_IND_SIMPTC3_1;      /*!< offset: 0x184 info: SOC Interrupt101 Indicator Register */
    __IO uint32_t INT102_IND_SIMPTC3_2;      /*!< offset: 0x188 info: SOC Interrupt102 Indicator Register */
    __IO uint32_t INT103_IND_SIMPTC3_3;      /*!< offset: 0x18C info: SOC Interrupt103 Indicator Register */
    __IO uint32_t INT104_IND_TC0;            /*!< offset: 0x190 info: SOC Interrupt104 Indicator Register */
    __IO uint32_t INT105_IND_TC1;            /*!< offset: 0x194 info: SOC Interrupt105 Indicator Register */
    __IO uint32_t INT106_IND_ISO78160;       /*!< offset: 0x198 info: SOC Interrupt106 Indicator Register */
    __IO uint32_t INT107_IND_ISO78161;       /*!< offset: 0x19C info: SOC Interrupt107 Indicator Register */
    __IO uint32_t INT108_IND_I2C0;           /*!< offset: 0x1A0 info: SOC Interrupt108 Indicator Register */
    __IO uint32_t INT109_IND_I2C1;           /*!< offset: 0x1A4 info: SOC Interrupt109 Indicator Register */
    __IO uint32_t INT110_IND_LPUART;         /*!< offset: 0x1A8 info: SOC Interrupt110 Indicator Register */
    __IO uint32_t INT111_IND_LSRX;           /*!< offset: 0x1AC info: SOC Interrupt111 Indicator Register */
    __IO uint32_t INT112_IND_UART0;          /*!< offset: 0x1B0 info: SOC Interrupt112 Indicator Register */
    __IO uint32_t INT113_IND_UART1;          /*!< offset: 0x1B4 info: SOC Interrupt113 Indicator Register */
    __IO uint32_t INT114_IND_UART2;          /*!< offset: 0x1B8 info: SOC Interrupt114 Indicator Register */
    __IO uint32_t INT115_IND_UART3;          /*!< offset: 0x1BC info: SOC Interrupt115 Indicator Register */
    __IO uint32_t INT116_IND_UART4;          /*!< offset: 0x1C0 info: SOC Interrupt116 Indicator Register */
    __IO uint32_t INT117_IND_UART5;          /*!< offset: 0x1C4 info: SOC Interrupt117 Indicator Register */
    __IO uint32_t INT118_IND_UART6;          /*!< offset: 0x1C8 info: SOC Interrupt118 Indicator Register */
    __IO uint32_t INT119_IND_UART7;          /*!< offset: 0x1CC info: SOC Interrupt119 Indicator Register */
    __IO uint32_t INT120_IND_USART0;         /*!< offset: 0x1D0 info: SOC Interrupt120 Indicator Register */
    __IO uint32_t INT121_IND_USART1;         /*!< offset: 0x1D4 info: SOC Interrupt121 Indicator Register */
    __IO uint32_t RESERVED4;                 /*!< offset: 0x1D8 */
    __IO uint32_t INT123_IND_SPI0;           /*!< offset: 0x1DC info: SOC Interrupt123 Indicator Register */
    __IO uint32_t INT124_IND_SPI1;           /*!< offset: 0x1E0 info: SOC Interrupt124 Indicator Register */
    __IO uint32_t INT125_IND_SPI2;           /*!< offset: 0x1E4 info: SOC Interrupt125 Indicator Register */
    __IO uint32_t INT126_IND_SPI3;           /*!< offset: 0x1E8 info: SOC Interrupt126 Indicator Register */
    __IO uint32_t INT127_IND_QSPI0;          /*!< offset: 0x1EC info: SOC Interrupt127 Indicator Register */
    __IO uint32_t INT128_IND_QSPI1;          /*!< offset: 0x1F0 info: SOC Interrupt128 Indicator Register */
    __IO uint32_t INT129_IND_DSP;            /*!< offset: 0x1F4 info: SOC Interrupt129 Indicator Register */
    __IO uint32_t INT130_IND_M2M;            /*!< offset: 0x1F8 info: SOC Interrupt130 Indicator Register */
    __IO uint32_t INT131_IND_AES;            /*!< offset: 0x1FC info: SOC Interrupt131 Indicator Register */
    __IO uint32_t INT132_IND_SEA;            /*!< offset: 0x200 info: SOC Interrupt132 Indicator Register */
    __IO uint32_t INT133_IND_KEYSTORE;       /*!< offset: 0x204 info: SOC Interrupt133 Indicator Register */
    __IO uint32_t INT134_IND_DAC;            /*!< offset: 0x208 info: SOC Interrupt134 Indicator Register */
    __IO uint32_t INT135_IND_ADC0;           /*!< offset: 0x20C info: SOC Interrupt135 Indicator Register */
    __IO uint32_t INT136_IND_D2F;            /*!< offset: 0x210 info: SOC Interrupt136 Indicator Register */
    __IO uint32_t INT137_IND_RTC;            /*!< offset: 0x214 info: SOC Interrupt137 Indicator Register */
    __IO uint32_t INT138_IND_VBAT;           /*!< offset: 0x218 info: SOC Interrupt138 Indicator Register */
    __IO uint32_t INT139_IND_CMPLVD;         /*!< offset: 0x21C info: SOC Interrupt139 Indicator Register */
    __IO uint32_t INT140_IND_LCSS;           /*!< offset: 0x220 info: SOC Interrupt140 Indicator Register */
    __IO uint32_t INT141_IND_SCU;            /*!< offset: 0x224 info: SOC Interrupt141 Indicator Register */
    __IO uint32_t INT142_IND_CORE1_RST;      /*!< offset: 0x228 info: SOC Interrupt142 Indicator Register */
    __IO uint32_t INT143_IND_CORE1_SYSREG;   /*!< offset: 0x22C info: SOC Interrupt143 Indicator Register */
    __IO uint32_t INT144_IND_CORE0_SYSREG;   /*!< offset: 0x230 info: SOC Interrupt144 Indicator Register */
    __IO uint32_t INT145_IND_MAILBOXINTER;   /*!< offset: 0x234 info: SOC Interrupt145 Indicator Register */
    __IO uint32_t INT146_IND_MAILBOXHOST0;   /*!< offset: 0x238 info: SOC Interrupt146 Indicator Register */
} GIC_TypeDef;

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
#define GIC_CORE0_BASE  (0x3001C000U)
#define GIC_CORE1_BASE  (0xB001C000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define GIC_CORE0  ((GIC_TypeDef *)      GIC_CORE0_BASE)
#define GIC_CORE1  ((GIC_TypeDef *)      GIC_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define GIC  GIC_CORE0
#endif
#if defined(CORE1)
#define GIC  GIC_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** GIC Instances *******************************/
#define IS_GIC_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == GIC_CORE0) || \
                                          ((INSTANCE) == GIC_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* GIC_CORE0_ICI_STA0 */
#define GIC_CORE0_ICI_STA0_ICI_STATUS        (BIT(0))                                            /*!< Inter-core interrupt status bit sent from CORE0 to CORE0 */
#define GIC_CORE0_ICI_STA0_ICI_INFO          (BITS(24,31))                                       /*!< 8-bit message received by CORE0 when receiving inter-core interrupt from CORE0 */

/* GIC_CORE0_ICI_STA1 */
#define GIC_CORE0_ICI_STA1_ICI_STATUS        (BIT(0))                                            /*!< Inter-core interrupt status bit sent from CORE1 to CORE0 */
#define GIC_CORE0_ICI_STA1_ICI_INFO          (BITS(24,31))                                       /*!< 8-bit message received by CORE0 when receiving inter-core interrupt from CORE1 */

/* GIC_CORE1_ICI_STA0 */
#define GIC_CORE1_ICI_STA0_ICI_STATUS        (BIT(0))                                            /*!< Inter-core interrupt status bit sent from CORE0 to CORE0 */
#define GIC_CORE1_ICI_STA0_ICI_INFO          (BITS(24,31))                                       /*!< 8-bit message received by CORE0 when receiving inter-core interrupt from CORE1 */

/* GIC_CORE1_ICI_STA1 */
#define GIC_CORE1_ICI_STA1_ICI_STATUS        (BIT(0))                                            /*!< Inter-core interrupt status bit sent from CORE0 to CORE0 */
#define GIC_CORE1_ICI_STA1_ICI_INFO          (BITS(24,31))                                       /*!< 8-bit message received by CORE0 when receiving inter-core interrupt from CORE0 */

/* GIC_ICI_GEN */
#define GIC_ICI_GEN_DESTINATION              (BIT(0))                                            /*!< Write the CORE_id of the core that will receive the inter-core interrupt */
#define GIC_ICI_GEN_ICI_INT_INFO             (BITS(24,31))                                       /*!< 8-bit message sent to the core receiving inter-core interrupt */

/* GIC_CORE_IND_LOCK */
#define GIC_CORE_IND_LOCK_CORE_IND_LOCK      (BITS(0,31))                                        /*!< Core interrupt lock configuration */

/* GIC_CORE_NUM */
#define GIC_CORE_NUM_CORE_NUM                (BITS(0,31))                                        /*!< Number of cores in the system */

/* GIC_HART_NUM */
#define GIC_HART_NUM_HART_NUM                (BITS(0,31))                                        /*!< Number of HART buses in the system */

/* GIC_SOC_IRQ_NUM */
#define GIC_SOC_IRQ_NUM_SOC_IRQ_NUM          (BITS(0,31))                                        /*!< Number of external interrupts in the system */

/* GIC_INT20_IND_RESERVED17 */
#define GIC_INT20_IND_RESERVED17_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT20_IND_RESERVED17_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT21_IND_RESERVED18 */
#define GIC_INT21_IND_RESERVED18_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT21_IND_RESERVED18_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT22_IND_RESERVED19 */
#define GIC_INT22_IND_RESERVED19_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT22_IND_RESERVED19_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT23_IND_RESERVED20 */
#define GIC_INT23_IND_RESERVED20_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT23_IND_RESERVED20_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT24_IND_RESERVED21 */
#define GIC_INT24_IND_RESERVED21_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT24_IND_RESERVED21_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT25_IND_RESERVED22 */
#define GIC_INT25_IND_RESERVED22_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT25_IND_RESERVED22_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT26_IND_RESERVED23 */
#define GIC_INT26_IND_RESERVED23_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT26_IND_RESERVED23_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT27_IND_RESERVED24 */
#define GIC_INT27_IND_RESERVED24_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT27_IND_RESERVED24_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT28_IND_RESERVED25 */
#define GIC_INT28_IND_RESERVED25_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT28_IND_RESERVED25_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT29_IND_RESERVED26 */
#define GIC_INT29_IND_RESERVED26_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT29_IND_RESERVED26_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT30_IND_RESERVED27 */
#define GIC_INT30_IND_RESERVED27_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT30_IND_RESERVED27_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT31_IND_RESERVED28 */
#define GIC_INT31_IND_RESERVED28_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT31_IND_RESERVED28_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT32_IND_RESERVED29 */
#define GIC_INT32_IND_RESERVED29_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT32_IND_RESERVED29_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT33_IND_RESERVED30 */
#define GIC_INT33_IND_RESERVED30_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT33_IND_RESERVED30_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT34_IND_RESERVED31 */
#define GIC_INT34_IND_RESERVED31_SOC_INT_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0/CORE1 */

/* GIC_INT35_IND_RESERVED32 */
#define GIC_INT35_IND_RESERVED32_SOC_INT_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0/CORE1 */

/* GIC_INT36_IND_RESERVED33 */
#define GIC_INT36_IND_RESERVED33_SOC_INT_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0/CORE1 */

/* GIC_INT37_IND_RESERVED34 */
#define GIC_INT37_IND_RESERVED34_SOC_INT_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0/CORE1 */

/* GIC_INT38_IND_RESERVED35 */
#define GIC_INT38_IND_RESERVED35_SOC_INT_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0/CORE1 */

/* GIC_INT39_IND_RESERVED36 */
#define GIC_INT39_IND_RESERVED36_SOC_INT_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0/CORE1 */

/* GIC_INT40_IND_RESERVED37 */
#define GIC_INT40_IND_RESERVED37_SOC_INT_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0/CORE1 */

/* GIC_INT41_IND_RESERVED38 */
#define GIC_INT41_IND_RESERVED38_SOC_INT_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0/CORE1 */

/* GIC_INT42_IND_RESERVED39 */
#define GIC_INT42_IND_RESERVED39_SOC_INT_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0/CORE1 */

/* GIC_INT43_IND_RESERVED40 */
#define GIC_INT43_IND_RESERVED40_SOC_INT_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0/CORE1 */

/* GIC_INT44_IND_RESERVED41 */
#define GIC_INT44_IND_RESERVED41_SOC_INT_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0/CORE1 */

/* GIC_INT45_IND_RESERVED42 */
#define GIC_INT45_IND_RESERVED42_SOC_INT_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0/CORE1 */

/* GIC_INT46_IND_LCD */
#define GIC_INT46_IND_LCD_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT46_IND_LCD_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT47_IND_KBI1_IRQ0 */
#define GIC_INT47_IND_KBI1_IRQ0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT47_IND_KBI1_IRQ0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT48_IND_KBI1_IRQ1 */
#define GIC_INT48_IND_KBI1_IRQ1_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT48_IND_KBI1_IRQ1_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT49_IND_KBI1_IRQ2 */
#define GIC_INT49_IND_KBI1_IRQ2_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT49_IND_KBI1_IRQ2_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT50_IND_KBI1_IRQ3 */
#define GIC_INT50_IND_KBI1_IRQ3_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT50_IND_KBI1_IRQ3_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT51_IND_KBI1_IRQ4 */
#define GIC_INT51_IND_KBI1_IRQ4_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT51_IND_KBI1_IRQ4_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT52_IND_KBI1_IRQ5 */
#define GIC_INT52_IND_KBI1_IRQ5_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT52_IND_KBI1_IRQ5_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT53_IND_KBI1_IRQ6 */
#define GIC_INT53_IND_KBI1_IRQ6_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT53_IND_KBI1_IRQ6_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT54_IND_KBI1_IRQ7 */
#define GIC_INT54_IND_KBI1_IRQ7_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT54_IND_KBI1_IRQ7_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT55_IND_KBI1_IRQ8 */
#define GIC_INT55_IND_KBI1_IRQ8_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT55_IND_KBI1_IRQ8_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT56_IND_KBI1_IRQ9 */
#define GIC_INT56_IND_KBI1_IRQ9_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT56_IND_KBI1_IRQ9_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT57_IND_KBI1_IRQ10 */
#define GIC_INT57_IND_KBI1_IRQ10_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT57_IND_KBI1_IRQ10_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT58_IND_KBI1_IRQ11 */
#define GIC_INT58_IND_KBI1_IRQ11_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT58_IND_KBI1_IRQ11_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT59_IND_KBI1_IRQ12 */
#define GIC_INT59_IND_KBI1_IRQ12_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT59_IND_KBI1_IRQ12_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT60_IND_KBI1_IRQ13 */
#define GIC_INT60_IND_KBI1_IRQ13_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT60_IND_KBI1_IRQ13_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT61_IND_KBI1_IRQ14 */
#define GIC_INT61_IND_KBI1_IRQ14_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT61_IND_KBI1_IRQ14_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT62_IND_KBI1_IRQ15 */
#define GIC_INT62_IND_KBI1_IRQ15_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT62_IND_KBI1_IRQ15_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT63_IND_KBI1_IRQN */
#define GIC_INT63_IND_KBI1_IRQN_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT63_IND_KBI1_IRQN_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT64_IND_KBI0_IRQ0 */
#define GIC_INT64_IND_KBI0_IRQ0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT64_IND_KBI0_IRQ0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT65_IND_KBI0_IRQ1 */
#define GIC_INT65_IND_KBI0_IRQ1_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT65_IND_KBI0_IRQ1_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT66_IND_KBI0_IRQ2 */
#define GIC_INT66_IND_KBI0_IRQ2_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT66_IND_KBI0_IRQ2_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT67_IND_KBI0_IRQ3 */
#define GIC_INT67_IND_KBI0_IRQ3_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT67_IND_KBI0_IRQ3_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT68_IND_KBI0_IRQ4 */
#define GIC_INT68_IND_KBI0_IRQ4_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT68_IND_KBI0_IRQ4_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT69_IND_KBI0_IRQ5 */
#define GIC_INT69_IND_KBI0_IRQ5_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT69_IND_KBI0_IRQ5_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT70_IND_KBI0_IRQ6 */
#define GIC_INT70_IND_KBI0_IRQ6_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT70_IND_KBI0_IRQ6_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT71_IND_KBI0_IRQ7 */
#define GIC_INT71_IND_KBI0_IRQ7_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT71_IND_KBI0_IRQ7_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT72_IND_KBI0_IRQ8 */
#define GIC_INT72_IND_KBI0_IRQ8_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT72_IND_KBI0_IRQ8_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT73_IND_KBI0_IRQ9 */
#define GIC_INT73_IND_KBI0_IRQ9_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT73_IND_KBI0_IRQ9_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT74_IND_KBI0_IRQ10 */
#define GIC_INT74_IND_KBI0_IRQ10_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT74_IND_KBI0_IRQ10_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT75_IND_KBI0_IRQ11 */
#define GIC_INT75_IND_KBI0_IRQ11_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT75_IND_KBI0_IRQ11_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT76_IND_KBI0_IRQ12 */
#define GIC_INT76_IND_KBI0_IRQ12_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT76_IND_KBI0_IRQ12_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT77_IND_KBI0_IRQ13 */
#define GIC_INT77_IND_KBI0_IRQ13_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT77_IND_KBI0_IRQ13_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT78_IND_KBI0_IRQ14 */
#define GIC_INT78_IND_KBI0_IRQ14_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT78_IND_KBI0_IRQ14_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT79_IND_KBI0_IRQ15 */
#define GIC_INT79_IND_KBI0_IRQ15_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT79_IND_KBI0_IRQ15_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT80_IND_KBI0_IRQN */
#define GIC_INT80_IND_KBI0_IRQN_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT80_IND_KBI0_IRQN_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT81_IND_WDT0 */
#define GIC_INT81_IND_WDT0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT81_IND_WDT0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT82_IND_WDT1 */
#define GIC_INT82_IND_WDT1_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT82_IND_WDT1_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT83_IND_GDMA11 */
#define GIC_INT83_IND_GDMA11_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT83_IND_GDMA11_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT84_IND_GDMA10 */
#define GIC_INT84_IND_GDMA10_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT84_IND_GDMA10_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT85_IND_GDMA01 */
#define GIC_INT85_IND_GDMA01_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT85_IND_GDMA01_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT86_IND_GDMA00 */
#define GIC_INT86_IND_GDMA00_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT86_IND_GDMA00_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT88_IND_SIMPTC0_0 */
#define GIC_INT88_IND_SIMPTC0_0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT88_IND_SIMPTC0_0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT89_IND_SIMPTC0_1 */
#define GIC_INT89_IND_SIMPTC0_1_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT89_IND_SIMPTC0_1_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT90_IND_SIMPTC0_2 */
#define GIC_INT90_IND_SIMPTC0_2_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT90_IND_SIMPTC0_2_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT91_IND_SIMPTC0_3 */
#define GIC_INT91_IND_SIMPTC0_3_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT91_IND_SIMPTC0_3_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT92_IND_SIMPTC1_0 */
#define GIC_INT92_IND_SIMPTC1_0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT92_IND_SIMPTC1_0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT93_IND_SIMPTC1_1 */
#define GIC_INT93_IND_SIMPTC1_1_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT93_IND_SIMPTC1_1_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT94_IND_SIMPTC1_2 */
#define GIC_INT94_IND_SIMPTC1_2_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT94_IND_SIMPTC1_2_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT95_IND_SIMPTC1_3 */
#define GIC_INT95_IND_SIMPTC1_3_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT95_IND_SIMPTC1_3_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT96_IND_SIMPTC2_0 */
#define GIC_INT96_IND_SIMPTC2_0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT96_IND_SIMPTC2_0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT97_IND_SIMPTC2_1 */
#define GIC_INT97_IND_SIMPTC2_1_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT97_IND_SIMPTC2_1_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT98_IND_SIMPTC2_2 */
#define GIC_INT98_IND_SIMPTC2_2_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT98_IND_SIMPTC2_2_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT99_IND_SIMPTC2_3 */
#define GIC_INT99_IND_SIMPTC2_3_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT99_IND_SIMPTC2_3_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT100_IND_SIMPTC3_0 */
#define GIC_INT100_IND_SIMPTC3_0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT100_IND_SIMPTC3_0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT101_IND_SIMPTC3_1 */
#define GIC_INT101_IND_SIMPTC3_1_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT101_IND_SIMPTC3_1_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT102_IND_SIMPTC3_2 */
#define GIC_INT102_IND_SIMPTC3_2_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT102_IND_SIMPTC3_2_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT103_IND_SIMPTC3_3 */
#define GIC_INT103_IND_SIMPTC3_3_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT103_IND_SIMPTC3_3_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT104_IND_TC0 */
#define GIC_INT104_IND_TC0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT104_IND_TC0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT105_IND_TC1 */
#define GIC_INT105_IND_TC1_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT105_IND_TC1_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT106_IND_ISO78160 */
#define GIC_INT106_IND_ISO78160_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT106_IND_ISO78160_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT107_IND_ISO78161 */
#define GIC_INT107_IND_ISO78161_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT107_IND_ISO78161_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT108_IND_I2C0 */
#define GIC_INT108_IND_I2C0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT108_IND_I2C0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT109_IND_I2C1 */
#define GIC_INT109_IND_I2C1_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT109_IND_I2C1_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT110_IND_LPUART */
#define GIC_INT110_IND_LPUART_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT110_IND_LPUART_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT111_IND_LSRX */
#define GIC_INT111_IND_LSRX_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT111_IND_LSRX_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT112_IND_UART0 */
#define GIC_INT112_IND_UART0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT112_IND_UART0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT113_IND_UART1 */
#define GIC_INT113_IND_UART1_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT113_IND_UART1_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT114_IND_UART2 */
#define GIC_INT114_IND_UART2_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT114_IND_UART2_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT115_IND_UART3 */
#define GIC_INT115_IND_UART3_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT115_IND_UART3_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT116_IND_UART4 */
#define GIC_INT116_IND_UART4_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT116_IND_UART4_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT117_IND_UART5 */
#define GIC_INT117_IND_UART5_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT117_IND_UART5_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT118_IND_UART6 */
#define GIC_INT118_IND_UART6_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT118_IND_UART6_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT119_IND_UART7 */
#define GIC_INT119_IND_UART7_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT119_IND_UART7_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT120_IND_USART0 */
#define GIC_INT120_IND_USART0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT120_IND_USART0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT121_IND_USART1 */
#define GIC_INT121_IND_USART1_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT121_IND_USART1_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT123_IND_SPI0 */
#define GIC_INT123_IND_SPI0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT123_IND_SPI0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT124_IND_SPI1 */
#define GIC_INT124_IND_SPI1_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT124_IND_SPI1_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT125_IND_SPI2 */
#define GIC_INT125_IND_SPI2_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT125_IND_SPI2_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT126_IND_SPI3 */
#define GIC_INT126_IND_SPI3_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT126_IND_SPI3_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT127_IND_QSPI0 */
#define GIC_INT127_IND_QSPI0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT127_IND_QSPI0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT128_IND_QSPI1 */
#define GIC_INT128_IND_QSPI1_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT128_IND_QSPI1_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT129_IND_DSP */
#define GIC_INT129_IND_DSP_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT129_IND_DSP_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT130_IND_M2M */
#define GIC_INT130_IND_M2M_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT130_IND_M2M_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT131_IND_AES */
#define GIC_INT131_IND_AES_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT131_IND_AES_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT132_IND_SEA */
#define GIC_INT132_IND_SEA_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT132_IND_SEA_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT133_IND_KEYSTORE */
#define GIC_INT133_IND_KEYSTORE_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT133_IND_KEYSTORE_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT134_IND_DAC */
#define GIC_INT134_IND_DAC_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT134_IND_DAC_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT135_IND_ADC0 */
#define GIC_INT135_IND_ADC0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT135_IND_ADC0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT136_IND_D2F */
#define GIC_INT136_IND_D2F_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT136_IND_D2F_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT137_IND_RTC */
#define GIC_INT137_IND_RTC_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT137_IND_RTC_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT138_IND_VBAT */
#define GIC_INT138_IND_VBAT_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT138_IND_VBAT_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT139_IND_CMPLVD */
#define GIC_INT139_IND_CMPLVD_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT139_IND_CMPLVD_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT140_IND_LCSS */
#define GIC_INT140_IND_LCSS_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT140_IND_LCSS_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT141_IND_SCU */
#define GIC_INT141_IND_SCU_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT141_IND_SCU_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT142_IND_CORE1_RST */
#define GIC_INT142_IND_CORE1_RST_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT142_IND_CORE1_RST_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT143_IND_CORE1_SYSREG */
#define GIC_INT143_IND_CORE1_SYSREG_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT143_IND_CORE1_SYSREG_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT144_IND_CORE0_SYSREG */
#define GIC_INT144_IND_CORE0_SYSREG_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT144_IND_CORE0_SYSREG_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT145_IND_MAILBOXINTER */
#define GIC_INT145_IND_MAILBOXINTER_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT145_IND_MAILBOXINTER_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* GIC_INT146_IND_MAILBOXHOST0 */
#define GIC_INT146_IND_MAILBOXHOST0_SOC_INT0_INDICATOR  (BIT(0))                                            /*!< Whether external interrupt n is assigned to CORE0 */
#define GIC_INT146_IND_MAILBOXHOST0_SOC_INT1_INDICATOR  (BIT(1))                                            /*!< Whether external interrupt n is assigned to CORE1 */

/* constants definitions */
/* GIC_CORE0_ICI_STA0_ICI_STATUS */
#define GIC_CORE0_ICI_STA0_ICI_STATUS_POS    (0U)                                                /*!< GIC_CORE0_ICI_STA0_ICI_STATUS bit offset */

/* GIC_CORE0_ICI_STA0_ICI_INFO */
#define GIC_CORE0_ICI_STA0_ICI_INFO_POS      (24U)                                               /*!< GIC_CORE0_ICI_STA0_ICI_INFO bit offset */

/* GIC_CORE0_ICI_STA1_ICI_STATUS */
#define GIC_CORE0_ICI_STA1_ICI_STATUS_POS    (0U)                                                /*!< GIC_CORE0_ICI_STA1_ICI_STATUS bit offset */

/* GIC_CORE0_ICI_STA1_ICI_INFO */
#define GIC_CORE0_ICI_STA1_ICI_INFO_POS      (24U)                                               /*!< GIC_CORE0_ICI_STA1_ICI_INFO bit offset */

/* GIC_CORE1_ICI_STA0_ICI_STATUS */
#define GIC_CORE1_ICI_STA0_ICI_STATUS_POS    (0U)                                                /*!< GIC_CORE1_ICI_STA0_ICI_STATUS bit offset */

/* GIC_CORE1_ICI_STA0_ICI_INFO */
#define GIC_CORE1_ICI_STA0_ICI_INFO_POS      (24U)                                               /*!< GIC_CORE1_ICI_STA0_ICI_INFO bit offset */

/* GIC_CORE1_ICI_STA1_ICI_STATUS */
#define GIC_CORE1_ICI_STA1_ICI_STATUS_POS    (0U)                                                /*!< GIC_CORE1_ICI_STA1_ICI_STATUS bit offset */

/* GIC_CORE1_ICI_STA1_ICI_INFO */
#define GIC_CORE1_ICI_STA1_ICI_INFO_POS      (24U)                                               /*!< GIC_CORE1_ICI_STA1_ICI_INFO bit offset */

/* GIC_ICI_GEN_DESTINATION */
#define GIC_ICI_GEN_DESTINATION_POS          (0U)                                                /*!< GIC_ICI_GEN_DESTINATION bit offset */

/* GIC_ICI_GEN_ICI_INT_INFO */
#define GIC_ICI_GEN_ICI_INT_INFO_POS         (24U)                                               /*!< GIC_ICI_GEN_ICI_INT_INFO bit offset */

/* GIC_CORE_IND_LOCK_CORE_IND_LOCK */
#define GIC_CORE_IND_LOCK_CORE_IND_LOCK_POS  (0U)                                                /*!< GIC_CORE_IND_LOCK_CORE_IND_LOCK bit offset */

/* GIC_CORE_NUM_CORE_NUM */
#define GIC_CORE_NUM_CORE_NUM_POS            (0U)                                                /*!< GIC_CORE_NUM_CORE_NUM bit offset */

/* GIC_HART_NUM_HART_NUM */
#define GIC_HART_NUM_HART_NUM_POS            (0U)                                                /*!< GIC_HART_NUM_HART_NUM bit offset */

/* GIC_SOC_IRQ_NUM_SOC_IRQ_NUM */
#define GIC_SOC_IRQ_NUM_SOC_IRQ_NUM_POS      (0U)                                                /*!< GIC_SOC_IRQ_NUM_SOC_IRQ_NUM bit offset */

/* GIC_INT20_IND_RESERVED17_SOC_INT0_INDICATOR */
#define GIC_INT20_IND_RESERVED17_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT20_IND_RESERVED17_SOC_INT0_INDICATOR bit offset */

/* GIC_INT20_IND_RESERVED17_SOC_INT1_INDICATOR */
#define GIC_INT20_IND_RESERVED17_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT20_IND_RESERVED17_SOC_INT1_INDICATOR bit offset */

/* GIC_INT21_IND_RESERVED18_SOC_INT0_INDICATOR */
#define GIC_INT21_IND_RESERVED18_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT21_IND_RESERVED18_SOC_INT0_INDICATOR bit offset */

/* GIC_INT21_IND_RESERVED18_SOC_INT1_INDICATOR */
#define GIC_INT21_IND_RESERVED18_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT21_IND_RESERVED18_SOC_INT1_INDICATOR bit offset */

/* GIC_INT22_IND_RESERVED19_SOC_INT0_INDICATOR */
#define GIC_INT22_IND_RESERVED19_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT22_IND_RESERVED19_SOC_INT0_INDICATOR bit offset */

/* GIC_INT22_IND_RESERVED19_SOC_INT1_INDICATOR */
#define GIC_INT22_IND_RESERVED19_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT22_IND_RESERVED19_SOC_INT1_INDICATOR bit offset */

/* GIC_INT23_IND_RESERVED20_SOC_INT0_INDICATOR */
#define GIC_INT23_IND_RESERVED20_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT23_IND_RESERVED20_SOC_INT0_INDICATOR bit offset */

/* GIC_INT23_IND_RESERVED20_SOC_INT1_INDICATOR */
#define GIC_INT23_IND_RESERVED20_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT23_IND_RESERVED20_SOC_INT1_INDICATOR bit offset */

/* GIC_INT24_IND_RESERVED21_SOC_INT0_INDICATOR */
#define GIC_INT24_IND_RESERVED21_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT24_IND_RESERVED21_SOC_INT0_INDICATOR bit offset */

/* GIC_INT24_IND_RESERVED21_SOC_INT1_INDICATOR */
#define GIC_INT24_IND_RESERVED21_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT24_IND_RESERVED21_SOC_INT1_INDICATOR bit offset */

/* GIC_INT25_IND_RESERVED22_SOC_INT0_INDICATOR */
#define GIC_INT25_IND_RESERVED22_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT25_IND_RESERVED22_SOC_INT0_INDICATOR bit offset */

/* GIC_INT25_IND_RESERVED22_SOC_INT1_INDICATOR */
#define GIC_INT25_IND_RESERVED22_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT25_IND_RESERVED22_SOC_INT1_INDICATOR bit offset */

/* GIC_INT26_IND_RESERVED23_SOC_INT0_INDICATOR */
#define GIC_INT26_IND_RESERVED23_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT26_IND_RESERVED23_SOC_INT0_INDICATOR bit offset */

/* GIC_INT26_IND_RESERVED23_SOC_INT1_INDICATOR */
#define GIC_INT26_IND_RESERVED23_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT26_IND_RESERVED23_SOC_INT1_INDICATOR bit offset */

/* GIC_INT27_IND_RESERVED24_SOC_INT0_INDICATOR */
#define GIC_INT27_IND_RESERVED24_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT27_IND_RESERVED24_SOC_INT0_INDICATOR bit offset */

/* GIC_INT27_IND_RESERVED24_SOC_INT1_INDICATOR */
#define GIC_INT27_IND_RESERVED24_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT27_IND_RESERVED24_SOC_INT1_INDICATOR bit offset */

/* GIC_INT28_IND_RESERVED25_SOC_INT0_INDICATOR */
#define GIC_INT28_IND_RESERVED25_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT28_IND_RESERVED25_SOC_INT0_INDICATOR bit offset */

/* GIC_INT28_IND_RESERVED25_SOC_INT1_INDICATOR */
#define GIC_INT28_IND_RESERVED25_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT28_IND_RESERVED25_SOC_INT1_INDICATOR bit offset */

/* GIC_INT29_IND_RESERVED26_SOC_INT0_INDICATOR */
#define GIC_INT29_IND_RESERVED26_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT29_IND_RESERVED26_SOC_INT0_INDICATOR bit offset */

/* GIC_INT29_IND_RESERVED26_SOC_INT1_INDICATOR */
#define GIC_INT29_IND_RESERVED26_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT29_IND_RESERVED26_SOC_INT1_INDICATOR bit offset */

/* GIC_INT30_IND_RESERVED27_SOC_INT0_INDICATOR */
#define GIC_INT30_IND_RESERVED27_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT30_IND_RESERVED27_SOC_INT0_INDICATOR bit offset */

/* GIC_INT30_IND_RESERVED27_SOC_INT1_INDICATOR */
#define GIC_INT30_IND_RESERVED27_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT30_IND_RESERVED27_SOC_INT1_INDICATOR bit offset */

/* GIC_INT31_IND_RESERVED28_SOC_INT0_INDICATOR */
#define GIC_INT31_IND_RESERVED28_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT31_IND_RESERVED28_SOC_INT0_INDICATOR bit offset */

/* GIC_INT31_IND_RESERVED28_SOC_INT1_INDICATOR */
#define GIC_INT31_IND_RESERVED28_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT31_IND_RESERVED28_SOC_INT1_INDICATOR bit offset */

/* GIC_INT32_IND_RESERVED29_SOC_INT0_INDICATOR */
#define GIC_INT32_IND_RESERVED29_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT32_IND_RESERVED29_SOC_INT0_INDICATOR bit offset */

/* GIC_INT32_IND_RESERVED29_SOC_INT1_INDICATOR */
#define GIC_INT32_IND_RESERVED29_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT32_IND_RESERVED29_SOC_INT1_INDICATOR bit offset */

/* GIC_INT33_IND_RESERVED30_SOC_INT0_INDICATOR */
#define GIC_INT33_IND_RESERVED30_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT33_IND_RESERVED30_SOC_INT0_INDICATOR bit offset */

/* GIC_INT33_IND_RESERVED30_SOC_INT1_INDICATOR */
#define GIC_INT33_IND_RESERVED30_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT33_IND_RESERVED30_SOC_INT1_INDICATOR bit offset */

/* GIC_INT34_IND_RESERVED31_SOC_INT_INDICATOR */
#define GIC_INT34_IND_RESERVED31_SOC_INT_INDICATOR_POS  (0U)                                                /*!< GIC_INT34_IND_RESERVED31_SOC_INT_INDICATOR bit offset */

/* GIC_INT35_IND_RESERVED32_SOC_INT_INDICATOR */
#define GIC_INT35_IND_RESERVED32_SOC_INT_INDICATOR_POS  (0U)                                                /*!< GIC_INT35_IND_RESERVED32_SOC_INT_INDICATOR bit offset */

/* GIC_INT36_IND_RESERVED33_SOC_INT_INDICATOR */
#define GIC_INT36_IND_RESERVED33_SOC_INT_INDICATOR_POS  (0U)                                                /*!< GIC_INT36_IND_RESERVED33_SOC_INT_INDICATOR bit offset */

/* GIC_INT37_IND_RESERVED34_SOC_INT_INDICATOR */
#define GIC_INT37_IND_RESERVED34_SOC_INT_INDICATOR_POS  (0U)                                                /*!< GIC_INT37_IND_RESERVED34_SOC_INT_INDICATOR bit offset */

/* GIC_INT38_IND_RESERVED35_SOC_INT_INDICATOR */
#define GIC_INT38_IND_RESERVED35_SOC_INT_INDICATOR_POS  (0U)                                                /*!< GIC_INT38_IND_RESERVED35_SOC_INT_INDICATOR bit offset */

/* GIC_INT39_IND_RESERVED36_SOC_INT_INDICATOR */
#define GIC_INT39_IND_RESERVED36_SOC_INT_INDICATOR_POS  (0U)                                                /*!< GIC_INT39_IND_RESERVED36_SOC_INT_INDICATOR bit offset */

/* GIC_INT40_IND_RESERVED37_SOC_INT_INDICATOR */
#define GIC_INT40_IND_RESERVED37_SOC_INT_INDICATOR_POS  (0U)                                                /*!< GIC_INT40_IND_RESERVED37_SOC_INT_INDICATOR bit offset */

/* GIC_INT41_IND_RESERVED38_SOC_INT_INDICATOR */
#define GIC_INT41_IND_RESERVED38_SOC_INT_INDICATOR_POS  (0U)                                                /*!< GIC_INT41_IND_RESERVED38_SOC_INT_INDICATOR bit offset */

/* GIC_INT42_IND_RESERVED39_SOC_INT_INDICATOR */
#define GIC_INT42_IND_RESERVED39_SOC_INT_INDICATOR_POS  (0U)                                                /*!< GIC_INT42_IND_RESERVED39_SOC_INT_INDICATOR bit offset */

/* GIC_INT43_IND_RESERVED40_SOC_INT_INDICATOR */
#define GIC_INT43_IND_RESERVED40_SOC_INT_INDICATOR_POS  (0U)                                                /*!< GIC_INT43_IND_RESERVED40_SOC_INT_INDICATOR bit offset */

/* GIC_INT44_IND_RESERVED41_SOC_INT_INDICATOR */
#define GIC_INT44_IND_RESERVED41_SOC_INT_INDICATOR_POS  (0U)                                                /*!< GIC_INT44_IND_RESERVED41_SOC_INT_INDICATOR bit offset */

/* GIC_INT45_IND_RESERVED42_SOC_INT_INDICATOR */
#define GIC_INT45_IND_RESERVED42_SOC_INT_INDICATOR_POS  (0U)                                                /*!< GIC_INT45_IND_RESERVED42_SOC_INT_INDICATOR bit offset */

/* GIC_INT46_IND_LCD_SOC_INT0_INDICATOR */
#define GIC_INT46_IND_LCD_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT46_IND_LCD_SOC_INT0_INDICATOR bit offset */

/* GIC_INT46_IND_LCD_SOC_INT1_INDICATOR */
#define GIC_INT46_IND_LCD_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT46_IND_LCD_SOC_INT1_INDICATOR bit offset */

/* GIC_INT47_IND_KBI1_IRQ0_SOC_INT0_INDICATOR */
#define GIC_INT47_IND_KBI1_IRQ0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT47_IND_KBI1_IRQ0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT47_IND_KBI1_IRQ0_SOC_INT1_INDICATOR */
#define GIC_INT47_IND_KBI1_IRQ0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT47_IND_KBI1_IRQ0_SOC_INT1_INDICATOR bit offset */

/* GIC_INT48_IND_KBI1_IRQ1_SOC_INT0_INDICATOR */
#define GIC_INT48_IND_KBI1_IRQ1_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT48_IND_KBI1_IRQ1_SOC_INT0_INDICATOR bit offset */

/* GIC_INT48_IND_KBI1_IRQ1_SOC_INT1_INDICATOR */
#define GIC_INT48_IND_KBI1_IRQ1_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT48_IND_KBI1_IRQ1_SOC_INT1_INDICATOR bit offset */

/* GIC_INT49_IND_KBI1_IRQ2_SOC_INT0_INDICATOR */
#define GIC_INT49_IND_KBI1_IRQ2_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT49_IND_KBI1_IRQ2_SOC_INT0_INDICATOR bit offset */

/* GIC_INT49_IND_KBI1_IRQ2_SOC_INT1_INDICATOR */
#define GIC_INT49_IND_KBI1_IRQ2_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT49_IND_KBI1_IRQ2_SOC_INT1_INDICATOR bit offset */

/* GIC_INT50_IND_KBI1_IRQ3_SOC_INT0_INDICATOR */
#define GIC_INT50_IND_KBI1_IRQ3_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT50_IND_KBI1_IRQ3_SOC_INT0_INDICATOR bit offset */

/* GIC_INT50_IND_KBI1_IRQ3_SOC_INT1_INDICATOR */
#define GIC_INT50_IND_KBI1_IRQ3_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT50_IND_KBI1_IRQ3_SOC_INT1_INDICATOR bit offset */

/* GIC_INT51_IND_KBI1_IRQ4_SOC_INT0_INDICATOR */
#define GIC_INT51_IND_KBI1_IRQ4_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT51_IND_KBI1_IRQ4_SOC_INT0_INDICATOR bit offset */

/* GIC_INT51_IND_KBI1_IRQ4_SOC_INT1_INDICATOR */
#define GIC_INT51_IND_KBI1_IRQ4_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT51_IND_KBI1_IRQ4_SOC_INT1_INDICATOR bit offset */

/* GIC_INT52_IND_KBI1_IRQ5_SOC_INT0_INDICATOR */
#define GIC_INT52_IND_KBI1_IRQ5_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT52_IND_KBI1_IRQ5_SOC_INT0_INDICATOR bit offset */

/* GIC_INT52_IND_KBI1_IRQ5_SOC_INT1_INDICATOR */
#define GIC_INT52_IND_KBI1_IRQ5_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT52_IND_KBI1_IRQ5_SOC_INT1_INDICATOR bit offset */

/* GIC_INT53_IND_KBI1_IRQ6_SOC_INT0_INDICATOR */
#define GIC_INT53_IND_KBI1_IRQ6_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT53_IND_KBI1_IRQ6_SOC_INT0_INDICATOR bit offset */

/* GIC_INT53_IND_KBI1_IRQ6_SOC_INT1_INDICATOR */
#define GIC_INT53_IND_KBI1_IRQ6_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT53_IND_KBI1_IRQ6_SOC_INT1_INDICATOR bit offset */

/* GIC_INT54_IND_KBI1_IRQ7_SOC_INT0_INDICATOR */
#define GIC_INT54_IND_KBI1_IRQ7_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT54_IND_KBI1_IRQ7_SOC_INT0_INDICATOR bit offset */

/* GIC_INT54_IND_KBI1_IRQ7_SOC_INT1_INDICATOR */
#define GIC_INT54_IND_KBI1_IRQ7_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT54_IND_KBI1_IRQ7_SOC_INT1_INDICATOR bit offset */

/* GIC_INT55_IND_KBI1_IRQ8_SOC_INT0_INDICATOR */
#define GIC_INT55_IND_KBI1_IRQ8_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT55_IND_KBI1_IRQ8_SOC_INT0_INDICATOR bit offset */

/* GIC_INT55_IND_KBI1_IRQ8_SOC_INT1_INDICATOR */
#define GIC_INT55_IND_KBI1_IRQ8_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT55_IND_KBI1_IRQ8_SOC_INT1_INDICATOR bit offset */

/* GIC_INT56_IND_KBI1_IRQ9_SOC_INT0_INDICATOR */
#define GIC_INT56_IND_KBI1_IRQ9_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT56_IND_KBI1_IRQ9_SOC_INT0_INDICATOR bit offset */

/* GIC_INT56_IND_KBI1_IRQ9_SOC_INT1_INDICATOR */
#define GIC_INT56_IND_KBI1_IRQ9_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT56_IND_KBI1_IRQ9_SOC_INT1_INDICATOR bit offset */

/* GIC_INT57_IND_KBI1_IRQ10_SOC_INT0_INDICATOR */
#define GIC_INT57_IND_KBI1_IRQ10_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT57_IND_KBI1_IRQ10_SOC_INT0_INDICATOR bit offset */

/* GIC_INT57_IND_KBI1_IRQ10_SOC_INT1_INDICATOR */
#define GIC_INT57_IND_KBI1_IRQ10_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT57_IND_KBI1_IRQ10_SOC_INT1_INDICATOR bit offset */

/* GIC_INT58_IND_KBI1_IRQ11_SOC_INT0_INDICATOR */
#define GIC_INT58_IND_KBI1_IRQ11_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT58_IND_KBI1_IRQ11_SOC_INT0_INDICATOR bit offset */

/* GIC_INT58_IND_KBI1_IRQ11_SOC_INT1_INDICATOR */
#define GIC_INT58_IND_KBI1_IRQ11_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT58_IND_KBI1_IRQ11_SOC_INT1_INDICATOR bit offset */

/* GIC_INT59_IND_KBI1_IRQ12_SOC_INT0_INDICATOR */
#define GIC_INT59_IND_KBI1_IRQ12_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT59_IND_KBI1_IRQ12_SOC_INT0_INDICATOR bit offset */

/* GIC_INT59_IND_KBI1_IRQ12_SOC_INT1_INDICATOR */
#define GIC_INT59_IND_KBI1_IRQ12_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT59_IND_KBI1_IRQ12_SOC_INT1_INDICATOR bit offset */

/* GIC_INT60_IND_KBI1_IRQ13_SOC_INT0_INDICATOR */
#define GIC_INT60_IND_KBI1_IRQ13_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT60_IND_KBI1_IRQ13_SOC_INT0_INDICATOR bit offset */

/* GIC_INT60_IND_KBI1_IRQ13_SOC_INT1_INDICATOR */
#define GIC_INT60_IND_KBI1_IRQ13_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT60_IND_KBI1_IRQ13_SOC_INT1_INDICATOR bit offset */

/* GIC_INT61_IND_KBI1_IRQ14_SOC_INT0_INDICATOR */
#define GIC_INT61_IND_KBI1_IRQ14_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT61_IND_KBI1_IRQ14_SOC_INT0_INDICATOR bit offset */

/* GIC_INT61_IND_KBI1_IRQ14_SOC_INT1_INDICATOR */
#define GIC_INT61_IND_KBI1_IRQ14_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT61_IND_KBI1_IRQ14_SOC_INT1_INDICATOR bit offset */

/* GIC_INT62_IND_KBI1_IRQ15_SOC_INT0_INDICATOR */
#define GIC_INT62_IND_KBI1_IRQ15_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT62_IND_KBI1_IRQ15_SOC_INT0_INDICATOR bit offset */

/* GIC_INT62_IND_KBI1_IRQ15_SOC_INT1_INDICATOR */
#define GIC_INT62_IND_KBI1_IRQ15_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT62_IND_KBI1_IRQ15_SOC_INT1_INDICATOR bit offset */

/* GIC_INT63_IND_KBI1_IRQN_SOC_INT0_INDICATOR */
#define GIC_INT63_IND_KBI1_IRQN_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT63_IND_KBI1_IRQN_SOC_INT0_INDICATOR bit offset */

/* GIC_INT63_IND_KBI1_IRQN_SOC_INT1_INDICATOR */
#define GIC_INT63_IND_KBI1_IRQN_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT63_IND_KBI1_IRQN_SOC_INT1_INDICATOR bit offset */

/* GIC_INT64_IND_KBI0_IRQ0_SOC_INT0_INDICATOR */
#define GIC_INT64_IND_KBI0_IRQ0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT64_IND_KBI0_IRQ0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT64_IND_KBI0_IRQ0_SOC_INT1_INDICATOR */
#define GIC_INT64_IND_KBI0_IRQ0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT64_IND_KBI0_IRQ0_SOC_INT1_INDICATOR bit offset */

/* GIC_INT65_IND_KBI0_IRQ1_SOC_INT0_INDICATOR */
#define GIC_INT65_IND_KBI0_IRQ1_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT65_IND_KBI0_IRQ1_SOC_INT0_INDICATOR bit offset */

/* GIC_INT65_IND_KBI0_IRQ1_SOC_INT1_INDICATOR */
#define GIC_INT65_IND_KBI0_IRQ1_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT65_IND_KBI0_IRQ1_SOC_INT1_INDICATOR bit offset */

/* GIC_INT66_IND_KBI0_IRQ2_SOC_INT0_INDICATOR */
#define GIC_INT66_IND_KBI0_IRQ2_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT66_IND_KBI0_IRQ2_SOC_INT0_INDICATOR bit offset */

/* GIC_INT66_IND_KBI0_IRQ2_SOC_INT1_INDICATOR */
#define GIC_INT66_IND_KBI0_IRQ2_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT66_IND_KBI0_IRQ2_SOC_INT1_INDICATOR bit offset */

/* GIC_INT67_IND_KBI0_IRQ3_SOC_INT0_INDICATOR */
#define GIC_INT67_IND_KBI0_IRQ3_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT67_IND_KBI0_IRQ3_SOC_INT0_INDICATOR bit offset */

/* GIC_INT67_IND_KBI0_IRQ3_SOC_INT1_INDICATOR */
#define GIC_INT67_IND_KBI0_IRQ3_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT67_IND_KBI0_IRQ3_SOC_INT1_INDICATOR bit offset */

/* GIC_INT68_IND_KBI0_IRQ4_SOC_INT0_INDICATOR */
#define GIC_INT68_IND_KBI0_IRQ4_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT68_IND_KBI0_IRQ4_SOC_INT0_INDICATOR bit offset */

/* GIC_INT68_IND_KBI0_IRQ4_SOC_INT1_INDICATOR */
#define GIC_INT68_IND_KBI0_IRQ4_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT68_IND_KBI0_IRQ4_SOC_INT1_INDICATOR bit offset */

/* GIC_INT69_IND_KBI0_IRQ5_SOC_INT0_INDICATOR */
#define GIC_INT69_IND_KBI0_IRQ5_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT69_IND_KBI0_IRQ5_SOC_INT0_INDICATOR bit offset */

/* GIC_INT69_IND_KBI0_IRQ5_SOC_INT1_INDICATOR */
#define GIC_INT69_IND_KBI0_IRQ5_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT69_IND_KBI0_IRQ5_SOC_INT1_INDICATOR bit offset */

/* GIC_INT70_IND_KBI0_IRQ6_SOC_INT0_INDICATOR */
#define GIC_INT70_IND_KBI0_IRQ6_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT70_IND_KBI0_IRQ6_SOC_INT0_INDICATOR bit offset */

/* GIC_INT70_IND_KBI0_IRQ6_SOC_INT1_INDICATOR */
#define GIC_INT70_IND_KBI0_IRQ6_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT70_IND_KBI0_IRQ6_SOC_INT1_INDICATOR bit offset */

/* GIC_INT71_IND_KBI0_IRQ7_SOC_INT0_INDICATOR */
#define GIC_INT71_IND_KBI0_IRQ7_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT71_IND_KBI0_IRQ7_SOC_INT0_INDICATOR bit offset */

/* GIC_INT71_IND_KBI0_IRQ7_SOC_INT1_INDICATOR */
#define GIC_INT71_IND_KBI0_IRQ7_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT71_IND_KBI0_IRQ7_SOC_INT1_INDICATOR bit offset */

/* GIC_INT72_IND_KBI0_IRQ8_SOC_INT0_INDICATOR */
#define GIC_INT72_IND_KBI0_IRQ8_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT72_IND_KBI0_IRQ8_SOC_INT0_INDICATOR bit offset */

/* GIC_INT72_IND_KBI0_IRQ8_SOC_INT1_INDICATOR */
#define GIC_INT72_IND_KBI0_IRQ8_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT72_IND_KBI0_IRQ8_SOC_INT1_INDICATOR bit offset */

/* GIC_INT73_IND_KBI0_IRQ9_SOC_INT0_INDICATOR */
#define GIC_INT73_IND_KBI0_IRQ9_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT73_IND_KBI0_IRQ9_SOC_INT0_INDICATOR bit offset */

/* GIC_INT73_IND_KBI0_IRQ9_SOC_INT1_INDICATOR */
#define GIC_INT73_IND_KBI0_IRQ9_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT73_IND_KBI0_IRQ9_SOC_INT1_INDICATOR bit offset */

/* GIC_INT74_IND_KBI0_IRQ10_SOC_INT0_INDICATOR */
#define GIC_INT74_IND_KBI0_IRQ10_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT74_IND_KBI0_IRQ10_SOC_INT0_INDICATOR bit offset */

/* GIC_INT74_IND_KBI0_IRQ10_SOC_INT1_INDICATOR */
#define GIC_INT74_IND_KBI0_IRQ10_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT74_IND_KBI0_IRQ10_SOC_INT1_INDICATOR bit offset */

/* GIC_INT75_IND_KBI0_IRQ11_SOC_INT0_INDICATOR */
#define GIC_INT75_IND_KBI0_IRQ11_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT75_IND_KBI0_IRQ11_SOC_INT0_INDICATOR bit offset */

/* GIC_INT75_IND_KBI0_IRQ11_SOC_INT1_INDICATOR */
#define GIC_INT75_IND_KBI0_IRQ11_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT75_IND_KBI0_IRQ11_SOC_INT1_INDICATOR bit offset */

/* GIC_INT76_IND_KBI0_IRQ12_SOC_INT0_INDICATOR */
#define GIC_INT76_IND_KBI0_IRQ12_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT76_IND_KBI0_IRQ12_SOC_INT0_INDICATOR bit offset */

/* GIC_INT76_IND_KBI0_IRQ12_SOC_INT1_INDICATOR */
#define GIC_INT76_IND_KBI0_IRQ12_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT76_IND_KBI0_IRQ12_SOC_INT1_INDICATOR bit offset */

/* GIC_INT77_IND_KBI0_IRQ13_SOC_INT0_INDICATOR */
#define GIC_INT77_IND_KBI0_IRQ13_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT77_IND_KBI0_IRQ13_SOC_INT0_INDICATOR bit offset */

/* GIC_INT77_IND_KBI0_IRQ13_SOC_INT1_INDICATOR */
#define GIC_INT77_IND_KBI0_IRQ13_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT77_IND_KBI0_IRQ13_SOC_INT1_INDICATOR bit offset */

/* GIC_INT78_IND_KBI0_IRQ14_SOC_INT0_INDICATOR */
#define GIC_INT78_IND_KBI0_IRQ14_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT78_IND_KBI0_IRQ14_SOC_INT0_INDICATOR bit offset */

/* GIC_INT78_IND_KBI0_IRQ14_SOC_INT1_INDICATOR */
#define GIC_INT78_IND_KBI0_IRQ14_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT78_IND_KBI0_IRQ14_SOC_INT1_INDICATOR bit offset */

/* GIC_INT79_IND_KBI0_IRQ15_SOC_INT0_INDICATOR */
#define GIC_INT79_IND_KBI0_IRQ15_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT79_IND_KBI0_IRQ15_SOC_INT0_INDICATOR bit offset */

/* GIC_INT79_IND_KBI0_IRQ15_SOC_INT1_INDICATOR */
#define GIC_INT79_IND_KBI0_IRQ15_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT79_IND_KBI0_IRQ15_SOC_INT1_INDICATOR bit offset */

/* GIC_INT80_IND_KBI0_IRQN_SOC_INT0_INDICATOR */
#define GIC_INT80_IND_KBI0_IRQN_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT80_IND_KBI0_IRQN_SOC_INT0_INDICATOR bit offset */

/* GIC_INT80_IND_KBI0_IRQN_SOC_INT1_INDICATOR */
#define GIC_INT80_IND_KBI0_IRQN_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT80_IND_KBI0_IRQN_SOC_INT1_INDICATOR bit offset */

/* GIC_INT81_IND_WDT0_SOC_INT0_INDICATOR */
#define GIC_INT81_IND_WDT0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT81_IND_WDT0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT81_IND_WDT0_SOC_INT1_INDICATOR */
#define GIC_INT81_IND_WDT0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT81_IND_WDT0_SOC_INT1_INDICATOR bit offset */

/* GIC_INT82_IND_WDT1_SOC_INT0_INDICATOR */
#define GIC_INT82_IND_WDT1_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT82_IND_WDT1_SOC_INT0_INDICATOR bit offset */

/* GIC_INT82_IND_WDT1_SOC_INT1_INDICATOR */
#define GIC_INT82_IND_WDT1_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT82_IND_WDT1_SOC_INT1_INDICATOR bit offset */

/* GIC_INT83_IND_GDMA11_SOC_INT0_INDICATOR */
#define GIC_INT83_IND_GDMA11_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT83_IND_GDMA11_SOC_INT0_INDICATOR bit offset */

/* GIC_INT83_IND_GDMA11_SOC_INT1_INDICATOR */
#define GIC_INT83_IND_GDMA11_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT83_IND_GDMA11_SOC_INT1_INDICATOR bit offset */

/* GIC_INT84_IND_GDMA10_SOC_INT0_INDICATOR */
#define GIC_INT84_IND_GDMA10_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT84_IND_GDMA10_SOC_INT0_INDICATOR bit offset */

/* GIC_INT84_IND_GDMA10_SOC_INT1_INDICATOR */
#define GIC_INT84_IND_GDMA10_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT84_IND_GDMA10_SOC_INT1_INDICATOR bit offset */

/* GIC_INT85_IND_GDMA01_SOC_INT0_INDICATOR */
#define GIC_INT85_IND_GDMA01_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT85_IND_GDMA01_SOC_INT0_INDICATOR bit offset */

/* GIC_INT85_IND_GDMA01_SOC_INT1_INDICATOR */
#define GIC_INT85_IND_GDMA01_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT85_IND_GDMA01_SOC_INT1_INDICATOR bit offset */

/* GIC_INT86_IND_GDMA00_SOC_INT0_INDICATOR */
#define GIC_INT86_IND_GDMA00_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT86_IND_GDMA00_SOC_INT0_INDICATOR bit offset */

/* GIC_INT86_IND_GDMA00_SOC_INT1_INDICATOR */
#define GIC_INT86_IND_GDMA00_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT86_IND_GDMA00_SOC_INT1_INDICATOR bit offset */

/* GIC_INT88_IND_SIMPTC0_0_SOC_INT0_INDICATOR */
#define GIC_INT88_IND_SIMPTC0_0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT88_IND_SIMPTC0_0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT88_IND_SIMPTC0_0_SOC_INT1_INDICATOR */
#define GIC_INT88_IND_SIMPTC0_0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT88_IND_SIMPTC0_0_SOC_INT1_INDICATOR bit offset */

/* GIC_INT89_IND_SIMPTC0_1_SOC_INT0_INDICATOR */
#define GIC_INT89_IND_SIMPTC0_1_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT89_IND_SIMPTC0_1_SOC_INT0_INDICATOR bit offset */

/* GIC_INT89_IND_SIMPTC0_1_SOC_INT1_INDICATOR */
#define GIC_INT89_IND_SIMPTC0_1_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT89_IND_SIMPTC0_1_SOC_INT1_INDICATOR bit offset */

/* GIC_INT90_IND_SIMPTC0_2_SOC_INT0_INDICATOR */
#define GIC_INT90_IND_SIMPTC0_2_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT90_IND_SIMPTC0_2_SOC_INT0_INDICATOR bit offset */

/* GIC_INT90_IND_SIMPTC0_2_SOC_INT1_INDICATOR */
#define GIC_INT90_IND_SIMPTC0_2_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT90_IND_SIMPTC0_2_SOC_INT1_INDICATOR bit offset */

/* GIC_INT91_IND_SIMPTC0_3_SOC_INT0_INDICATOR */
#define GIC_INT91_IND_SIMPTC0_3_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT91_IND_SIMPTC0_3_SOC_INT0_INDICATOR bit offset */

/* GIC_INT91_IND_SIMPTC0_3_SOC_INT1_INDICATOR */
#define GIC_INT91_IND_SIMPTC0_3_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT91_IND_SIMPTC0_3_SOC_INT1_INDICATOR bit offset */

/* GIC_INT92_IND_SIMPTC1_0_SOC_INT0_INDICATOR */
#define GIC_INT92_IND_SIMPTC1_0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT92_IND_SIMPTC1_0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT92_IND_SIMPTC1_0_SOC_INT1_INDICATOR */
#define GIC_INT92_IND_SIMPTC1_0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT92_IND_SIMPTC1_0_SOC_INT1_INDICATOR bit offset */

/* GIC_INT93_IND_SIMPTC1_1_SOC_INT0_INDICATOR */
#define GIC_INT93_IND_SIMPTC1_1_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT93_IND_SIMPTC1_1_SOC_INT0_INDICATOR bit offset */

/* GIC_INT93_IND_SIMPTC1_1_SOC_INT1_INDICATOR */
#define GIC_INT93_IND_SIMPTC1_1_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT93_IND_SIMPTC1_1_SOC_INT1_INDICATOR bit offset */

/* GIC_INT94_IND_SIMPTC1_2_SOC_INT0_INDICATOR */
#define GIC_INT94_IND_SIMPTC1_2_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT94_IND_SIMPTC1_2_SOC_INT0_INDICATOR bit offset */

/* GIC_INT94_IND_SIMPTC1_2_SOC_INT1_INDICATOR */
#define GIC_INT94_IND_SIMPTC1_2_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT94_IND_SIMPTC1_2_SOC_INT1_INDICATOR bit offset */

/* GIC_INT95_IND_SIMPTC1_3_SOC_INT0_INDICATOR */
#define GIC_INT95_IND_SIMPTC1_3_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT95_IND_SIMPTC1_3_SOC_INT0_INDICATOR bit offset */

/* GIC_INT95_IND_SIMPTC1_3_SOC_INT1_INDICATOR */
#define GIC_INT95_IND_SIMPTC1_3_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT95_IND_SIMPTC1_3_SOC_INT1_INDICATOR bit offset */

/* GIC_INT96_IND_SIMPTC2_0_SOC_INT0_INDICATOR */
#define GIC_INT96_IND_SIMPTC2_0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT96_IND_SIMPTC2_0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT96_IND_SIMPTC2_0_SOC_INT1_INDICATOR */
#define GIC_INT96_IND_SIMPTC2_0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT96_IND_SIMPTC2_0_SOC_INT1_INDICATOR bit offset */

/* GIC_INT97_IND_SIMPTC2_1_SOC_INT0_INDICATOR */
#define GIC_INT97_IND_SIMPTC2_1_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT97_IND_SIMPTC2_1_SOC_INT0_INDICATOR bit offset */

/* GIC_INT97_IND_SIMPTC2_1_SOC_INT1_INDICATOR */
#define GIC_INT97_IND_SIMPTC2_1_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT97_IND_SIMPTC2_1_SOC_INT1_INDICATOR bit offset */

/* GIC_INT98_IND_SIMPTC2_2_SOC_INT0_INDICATOR */
#define GIC_INT98_IND_SIMPTC2_2_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT98_IND_SIMPTC2_2_SOC_INT0_INDICATOR bit offset */

/* GIC_INT98_IND_SIMPTC2_2_SOC_INT1_INDICATOR */
#define GIC_INT98_IND_SIMPTC2_2_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT98_IND_SIMPTC2_2_SOC_INT1_INDICATOR bit offset */

/* GIC_INT99_IND_SIMPTC2_3_SOC_INT0_INDICATOR */
#define GIC_INT99_IND_SIMPTC2_3_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT99_IND_SIMPTC2_3_SOC_INT0_INDICATOR bit offset */

/* GIC_INT99_IND_SIMPTC2_3_SOC_INT1_INDICATOR */
#define GIC_INT99_IND_SIMPTC2_3_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT99_IND_SIMPTC2_3_SOC_INT1_INDICATOR bit offset */

/* GIC_INT100_IND_SIMPTC3_0_SOC_INT0_INDICATOR */
#define GIC_INT100_IND_SIMPTC3_0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT100_IND_SIMPTC3_0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT100_IND_SIMPTC3_0_SOC_INT1_INDICATOR */
#define GIC_INT100_IND_SIMPTC3_0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT100_IND_SIMPTC3_0_SOC_INT1_INDICATOR bit offset */

/* GIC_INT101_IND_SIMPTC3_1_SOC_INT0_INDICATOR */
#define GIC_INT101_IND_SIMPTC3_1_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT101_IND_SIMPTC3_1_SOC_INT0_INDICATOR bit offset */

/* GIC_INT101_IND_SIMPTC3_1_SOC_INT1_INDICATOR */
#define GIC_INT101_IND_SIMPTC3_1_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT101_IND_SIMPTC3_1_SOC_INT1_INDICATOR bit offset */

/* GIC_INT102_IND_SIMPTC3_2_SOC_INT0_INDICATOR */
#define GIC_INT102_IND_SIMPTC3_2_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT102_IND_SIMPTC3_2_SOC_INT0_INDICATOR bit offset */

/* GIC_INT102_IND_SIMPTC3_2_SOC_INT1_INDICATOR */
#define GIC_INT102_IND_SIMPTC3_2_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT102_IND_SIMPTC3_2_SOC_INT1_INDICATOR bit offset */

/* GIC_INT103_IND_SIMPTC3_3_SOC_INT0_INDICATOR */
#define GIC_INT103_IND_SIMPTC3_3_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT103_IND_SIMPTC3_3_SOC_INT0_INDICATOR bit offset */

/* GIC_INT103_IND_SIMPTC3_3_SOC_INT1_INDICATOR */
#define GIC_INT103_IND_SIMPTC3_3_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT103_IND_SIMPTC3_3_SOC_INT1_INDICATOR bit offset */

/* GIC_INT104_IND_TC0_SOC_INT0_INDICATOR */
#define GIC_INT104_IND_TC0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT104_IND_TC0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT104_IND_TC0_SOC_INT1_INDICATOR */
#define GIC_INT104_IND_TC0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT104_IND_TC0_SOC_INT1_INDICATOR bit offset */

/* GIC_INT105_IND_TC1_SOC_INT0_INDICATOR */
#define GIC_INT105_IND_TC1_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT105_IND_TC1_SOC_INT0_INDICATOR bit offset */

/* GIC_INT105_IND_TC1_SOC_INT1_INDICATOR */
#define GIC_INT105_IND_TC1_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT105_IND_TC1_SOC_INT1_INDICATOR bit offset */

/* GIC_INT106_IND_ISO78160_SOC_INT0_INDICATOR */
#define GIC_INT106_IND_ISO78160_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT106_IND_ISO78160_SOC_INT0_INDICATOR bit offset */

/* GIC_INT106_IND_ISO78160_SOC_INT1_INDICATOR */
#define GIC_INT106_IND_ISO78160_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT106_IND_ISO78160_SOC_INT1_INDICATOR bit offset */

/* GIC_INT107_IND_ISO78161_SOC_INT0_INDICATOR */
#define GIC_INT107_IND_ISO78161_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT107_IND_ISO78161_SOC_INT0_INDICATOR bit offset */

/* GIC_INT107_IND_ISO78161_SOC_INT1_INDICATOR */
#define GIC_INT107_IND_ISO78161_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT107_IND_ISO78161_SOC_INT1_INDICATOR bit offset */

/* GIC_INT108_IND_I2C0_SOC_INT0_INDICATOR */
#define GIC_INT108_IND_I2C0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT108_IND_I2C0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT108_IND_I2C0_SOC_INT1_INDICATOR */
#define GIC_INT108_IND_I2C0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT108_IND_I2C0_SOC_INT1_INDICATOR bit offset */

/* GIC_INT109_IND_I2C1_SOC_INT0_INDICATOR */
#define GIC_INT109_IND_I2C1_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT109_IND_I2C1_SOC_INT0_INDICATOR bit offset */

/* GIC_INT109_IND_I2C1_SOC_INT1_INDICATOR */
#define GIC_INT109_IND_I2C1_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT109_IND_I2C1_SOC_INT1_INDICATOR bit offset */

/* GIC_INT110_IND_LPUART_SOC_INT0_INDICATOR */
#define GIC_INT110_IND_LPUART_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT110_IND_LPUART_SOC_INT0_INDICATOR bit offset */

/* GIC_INT110_IND_LPUART_SOC_INT1_INDICATOR */
#define GIC_INT110_IND_LPUART_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT110_IND_LPUART_SOC_INT1_INDICATOR bit offset */

/* GIC_INT111_IND_LSRX_SOC_INT0_INDICATOR */
#define GIC_INT111_IND_LSRX_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT111_IND_LSRX_SOC_INT0_INDICATOR bit offset */

/* GIC_INT111_IND_LSRX_SOC_INT1_INDICATOR */
#define GIC_INT111_IND_LSRX_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT111_IND_LSRX_SOC_INT1_INDICATOR bit offset */

/* GIC_INT112_IND_UART0_SOC_INT0_INDICATOR */
#define GIC_INT112_IND_UART0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT112_IND_UART0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT112_IND_UART0_SOC_INT1_INDICATOR */
#define GIC_INT112_IND_UART0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT112_IND_UART0_SOC_INT1_INDICATOR bit offset */

/* GIC_INT113_IND_UART1_SOC_INT0_INDICATOR */
#define GIC_INT113_IND_UART1_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT113_IND_UART1_SOC_INT0_INDICATOR bit offset */

/* GIC_INT113_IND_UART1_SOC_INT1_INDICATOR */
#define GIC_INT113_IND_UART1_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT113_IND_UART1_SOC_INT1_INDICATOR bit offset */

/* GIC_INT114_IND_UART2_SOC_INT0_INDICATOR */
#define GIC_INT114_IND_UART2_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT114_IND_UART2_SOC_INT0_INDICATOR bit offset */

/* GIC_INT114_IND_UART2_SOC_INT1_INDICATOR */
#define GIC_INT114_IND_UART2_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT114_IND_UART2_SOC_INT1_INDICATOR bit offset */

/* GIC_INT115_IND_UART3_SOC_INT0_INDICATOR */
#define GIC_INT115_IND_UART3_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT115_IND_UART3_SOC_INT0_INDICATOR bit offset */

/* GIC_INT115_IND_UART3_SOC_INT1_INDICATOR */
#define GIC_INT115_IND_UART3_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT115_IND_UART3_SOC_INT1_INDICATOR bit offset */

/* GIC_INT116_IND_UART4_SOC_INT0_INDICATOR */
#define GIC_INT116_IND_UART4_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT116_IND_UART4_SOC_INT0_INDICATOR bit offset */

/* GIC_INT116_IND_UART4_SOC_INT1_INDICATOR */
#define GIC_INT116_IND_UART4_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT116_IND_UART4_SOC_INT1_INDICATOR bit offset */

/* GIC_INT117_IND_UART5_SOC_INT0_INDICATOR */
#define GIC_INT117_IND_UART5_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT117_IND_UART5_SOC_INT0_INDICATOR bit offset */

/* GIC_INT117_IND_UART5_SOC_INT1_INDICATOR */
#define GIC_INT117_IND_UART5_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT117_IND_UART5_SOC_INT1_INDICATOR bit offset */

/* GIC_INT118_IND_UART6_SOC_INT0_INDICATOR */
#define GIC_INT118_IND_UART6_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT118_IND_UART6_SOC_INT0_INDICATOR bit offset */

/* GIC_INT118_IND_UART6_SOC_INT1_INDICATOR */
#define GIC_INT118_IND_UART6_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT118_IND_UART6_SOC_INT1_INDICATOR bit offset */

/* GIC_INT119_IND_UART7_SOC_INT0_INDICATOR */
#define GIC_INT119_IND_UART7_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT119_IND_UART7_SOC_INT0_INDICATOR bit offset */

/* GIC_INT119_IND_UART7_SOC_INT1_INDICATOR */
#define GIC_INT119_IND_UART7_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT119_IND_UART7_SOC_INT1_INDICATOR bit offset */

/* GIC_INT120_IND_USART0_SOC_INT0_INDICATOR */
#define GIC_INT120_IND_USART0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT120_IND_USART0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT120_IND_USART0_SOC_INT1_INDICATOR */
#define GIC_INT120_IND_USART0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT120_IND_USART0_SOC_INT1_INDICATOR bit offset */

/* GIC_INT121_IND_USART1_SOC_INT0_INDICATOR */
#define GIC_INT121_IND_USART1_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT121_IND_USART1_SOC_INT0_INDICATOR bit offset */

/* GIC_INT121_IND_USART1_SOC_INT1_INDICATOR */
#define GIC_INT121_IND_USART1_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT121_IND_USART1_SOC_INT1_INDICATOR bit offset */

/* GIC_INT123_IND_SPI0_SOC_INT0_INDICATOR */
#define GIC_INT123_IND_SPI0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT123_IND_SPI0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT123_IND_SPI0_SOC_INT1_INDICATOR */
#define GIC_INT123_IND_SPI0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT123_IND_SPI0_SOC_INT1_INDICATOR bit offset */

/* GIC_INT124_IND_SPI1_SOC_INT0_INDICATOR */
#define GIC_INT124_IND_SPI1_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT124_IND_SPI1_SOC_INT0_INDICATOR bit offset */

/* GIC_INT124_IND_SPI1_SOC_INT1_INDICATOR */
#define GIC_INT124_IND_SPI1_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT124_IND_SPI1_SOC_INT1_INDICATOR bit offset */

/* GIC_INT125_IND_SPI2_SOC_INT0_INDICATOR */
#define GIC_INT125_IND_SPI2_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT125_IND_SPI2_SOC_INT0_INDICATOR bit offset */

/* GIC_INT125_IND_SPI2_SOC_INT1_INDICATOR */
#define GIC_INT125_IND_SPI2_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT125_IND_SPI2_SOC_INT1_INDICATOR bit offset */

/* GIC_INT126_IND_SPI3_SOC_INT0_INDICATOR */
#define GIC_INT126_IND_SPI3_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT126_IND_SPI3_SOC_INT0_INDICATOR bit offset */

/* GIC_INT126_IND_SPI3_SOC_INT1_INDICATOR */
#define GIC_INT126_IND_SPI3_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT126_IND_SPI3_SOC_INT1_INDICATOR bit offset */

/* GIC_INT127_IND_QSPI0_SOC_INT0_INDICATOR */
#define GIC_INT127_IND_QSPI0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT127_IND_QSPI0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT127_IND_QSPI0_SOC_INT1_INDICATOR */
#define GIC_INT127_IND_QSPI0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT127_IND_QSPI0_SOC_INT1_INDICATOR bit offset */

/* GIC_INT128_IND_QSPI1_SOC_INT0_INDICATOR */
#define GIC_INT128_IND_QSPI1_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT128_IND_QSPI1_SOC_INT0_INDICATOR bit offset */

/* GIC_INT128_IND_QSPI1_SOC_INT1_INDICATOR */
#define GIC_INT128_IND_QSPI1_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT128_IND_QSPI1_SOC_INT1_INDICATOR bit offset */

/* GIC_INT129_IND_DSP_SOC_INT0_INDICATOR */
#define GIC_INT129_IND_DSP_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT129_IND_DSP_SOC_INT0_INDICATOR bit offset */

/* GIC_INT129_IND_DSP_SOC_INT1_INDICATOR */
#define GIC_INT129_IND_DSP_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT129_IND_DSP_SOC_INT1_INDICATOR bit offset */

/* GIC_INT130_IND_M2M_SOC_INT0_INDICATOR */
#define GIC_INT130_IND_M2M_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT130_IND_M2M_SOC_INT0_INDICATOR bit offset */

/* GIC_INT130_IND_M2M_SOC_INT1_INDICATOR */
#define GIC_INT130_IND_M2M_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT130_IND_M2M_SOC_INT1_INDICATOR bit offset */

/* GIC_INT131_IND_AES_SOC_INT0_INDICATOR */
#define GIC_INT131_IND_AES_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT131_IND_AES_SOC_INT0_INDICATOR bit offset */

/* GIC_INT131_IND_AES_SOC_INT1_INDICATOR */
#define GIC_INT131_IND_AES_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT131_IND_AES_SOC_INT1_INDICATOR bit offset */

/* GIC_INT132_IND_SEA_SOC_INT0_INDICATOR */
#define GIC_INT132_IND_SEA_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT132_IND_SEA_SOC_INT0_INDICATOR bit offset */

/* GIC_INT132_IND_SEA_SOC_INT1_INDICATOR */
#define GIC_INT132_IND_SEA_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT132_IND_SEA_SOC_INT1_INDICATOR bit offset */

/* GIC_INT133_IND_KEYSTORE_SOC_INT0_INDICATOR */
#define GIC_INT133_IND_KEYSTORE_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT133_IND_KEYSTORE_SOC_INT0_INDICATOR bit offset */

/* GIC_INT133_IND_KEYSTORE_SOC_INT1_INDICATOR */
#define GIC_INT133_IND_KEYSTORE_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT133_IND_KEYSTORE_SOC_INT1_INDICATOR bit offset */

/* GIC_INT134_IND_DAC_SOC_INT0_INDICATOR */
#define GIC_INT134_IND_DAC_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT134_IND_DAC_SOC_INT0_INDICATOR bit offset */

/* GIC_INT134_IND_DAC_SOC_INT1_INDICATOR */
#define GIC_INT134_IND_DAC_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT134_IND_DAC_SOC_INT1_INDICATOR bit offset */

/* GIC_INT135_IND_ADC0_SOC_INT0_INDICATOR */
#define GIC_INT135_IND_ADC0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT135_IND_ADC0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT135_IND_ADC0_SOC_INT1_INDICATOR */
#define GIC_INT135_IND_ADC0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT135_IND_ADC0_SOC_INT1_INDICATOR bit offset */

/* GIC_INT136_IND_D2F_SOC_INT0_INDICATOR */
#define GIC_INT136_IND_D2F_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT136_IND_D2F_SOC_INT0_INDICATOR bit offset */

/* GIC_INT136_IND_D2F_SOC_INT1_INDICATOR */
#define GIC_INT136_IND_D2F_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT136_IND_D2F_SOC_INT1_INDICATOR bit offset */

/* GIC_INT137_IND_RTC_SOC_INT0_INDICATOR */
#define GIC_INT137_IND_RTC_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT137_IND_RTC_SOC_INT0_INDICATOR bit offset */

/* GIC_INT137_IND_RTC_SOC_INT1_INDICATOR */
#define GIC_INT137_IND_RTC_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT137_IND_RTC_SOC_INT1_INDICATOR bit offset */

/* GIC_INT138_IND_VBAT_SOC_INT0_INDICATOR */
#define GIC_INT138_IND_VBAT_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT138_IND_VBAT_SOC_INT0_INDICATOR bit offset */

/* GIC_INT138_IND_VBAT_SOC_INT1_INDICATOR */
#define GIC_INT138_IND_VBAT_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT138_IND_VBAT_SOC_INT1_INDICATOR bit offset */

/* GIC_INT139_IND_CMPLVD_SOC_INT0_INDICATOR */
#define GIC_INT139_IND_CMPLVD_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT139_IND_CMPLVD_SOC_INT0_INDICATOR bit offset */

/* GIC_INT139_IND_CMPLVD_SOC_INT1_INDICATOR */
#define GIC_INT139_IND_CMPLVD_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT139_IND_CMPLVD_SOC_INT1_INDICATOR bit offset */

/* GIC_INT140_IND_LCSS_SOC_INT0_INDICATOR */
#define GIC_INT140_IND_LCSS_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT140_IND_LCSS_SOC_INT0_INDICATOR bit offset */

/* GIC_INT140_IND_LCSS_SOC_INT1_INDICATOR */
#define GIC_INT140_IND_LCSS_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT140_IND_LCSS_SOC_INT1_INDICATOR bit offset */

/* GIC_INT141_IND_SCU_SOC_INT0_INDICATOR */
#define GIC_INT141_IND_SCU_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT141_IND_SCU_SOC_INT0_INDICATOR bit offset */

/* GIC_INT141_IND_SCU_SOC_INT1_INDICATOR */
#define GIC_INT141_IND_SCU_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT141_IND_SCU_SOC_INT1_INDICATOR bit offset */

/* GIC_INT142_IND_CORE1_RST_SOC_INT0_INDICATOR */
#define GIC_INT142_IND_CORE1_RST_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT142_IND_CORE1_RST_SOC_INT0_INDICATOR bit offset */

/* GIC_INT142_IND_CORE1_RST_SOC_INT1_INDICATOR */
#define GIC_INT142_IND_CORE1_RST_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT142_IND_CORE1_RST_SOC_INT1_INDICATOR bit offset */

/* GIC_INT143_IND_CORE1_SYSREG_SOC_INT0_INDICATOR */
#define GIC_INT143_IND_CORE1_SYSREG_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT143_IND_CORE1_SYSREG_SOC_INT0_INDICATOR bit offset */

/* GIC_INT143_IND_CORE1_SYSREG_SOC_INT1_INDICATOR */
#define GIC_INT143_IND_CORE1_SYSREG_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT143_IND_CORE1_SYSREG_SOC_INT1_INDICATOR bit offset */

/* GIC_INT144_IND_CORE0_SYSREG_SOC_INT0_INDICATOR */
#define GIC_INT144_IND_CORE0_SYSREG_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT144_IND_CORE0_SYSREG_SOC_INT0_INDICATOR bit offset */

/* GIC_INT144_IND_CORE0_SYSREG_SOC_INT1_INDICATOR */
#define GIC_INT144_IND_CORE0_SYSREG_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT144_IND_CORE0_SYSREG_SOC_INT1_INDICATOR bit offset */

/* GIC_INT145_IND_MAILBOXINTER_SOC_INT0_INDICATOR */
#define GIC_INT145_IND_MAILBOXINTER_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT145_IND_MAILBOXINTER_SOC_INT0_INDICATOR bit offset */

/* GIC_INT145_IND_MAILBOXINTER_SOC_INT1_INDICATOR */
#define GIC_INT145_IND_MAILBOXINTER_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT145_IND_MAILBOXINTER_SOC_INT1_INDICATOR bit offset */

/* GIC_INT146_IND_MAILBOXHOST0_SOC_INT0_INDICATOR */
#define GIC_INT146_IND_MAILBOXHOST0_SOC_INT0_INDICATOR_POS  (0U)                                                /*!< GIC_INT146_IND_MAILBOXHOST0_SOC_INT0_INDICATOR bit offset */

/* GIC_INT146_IND_MAILBOXHOST0_SOC_INT1_INDICATOR */
#define GIC_INT146_IND_MAILBOXHOST0_SOC_INT1_INDICATOR_POS  (1U)                                                /*!< GIC_INT146_IND_MAILBOXHOST0_SOC_INT1_INDICATOR bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_GIC_H */
