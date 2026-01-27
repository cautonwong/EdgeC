/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_rtc.h
 * @author  Renergy Software Team
 * @brief   RTC module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_RTC_H
#define _RN831x_RN861x_MCU_RTC_H

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

/* RTC */
typedef struct
{
    __IO uint32_t CTRL;                      /*!< offset: 0x000 info: control register */
    __IO uint32_t SC;                        /*!< offset: 0x004 info: seconds register */
    __IO uint32_t MN;                        /*!< offset: 0x008 info: minutes register */
    __IO uint32_t HR;                        /*!< offset: 0x00C info: hours register */
    __IO uint32_t DT;                        /*!< offset: 0x010 info: date register */
    __IO uint32_t MO;                        /*!< offset: 0x014 info: month register */
    __IO uint32_t YR;                        /*!< offset: 0x018 info: year register */
    __IO uint32_t DW;                        /*!< offset: 0x01C info: day of the week register */
    __IO uint32_t CNT1;                      /*!< offset: 0x020 info: RTC counter1 register */
    __IO uint32_t CNT2;                      /*!< offset: 0x024 info: RTC counter2 register */
    __IO uint32_t SCA;                       /*!< offset: 0x028 info: alarm seconds register */
    __IO uint32_t MNA;                       /*!< offset: 0x02C info: alarm minitues register */
    __IO uint32_t HRA;                       /*!< offset: 0x030 info: alarm hours register */
    __IO uint32_t IE;                        /*!< offset: 0x034 info: interrupt enable register */
    __IO uint32_t IF;                        /*!< offset: 0x038 info: RTC interrupt flags register */
    __IO uint32_t TEMP;                      /*!< offset: 0x03C info: current chip temperature register No.1.The bit field range depends on RTC_MODE1 (0xEA65: [15:0], others: [9:0]), and the field access depends on RTC_CTRL_TPS (0: invalid, 1: R, 2: RW, 3: W). */
    __IO uint32_t PS;                        /*!< offset: 0x040 info: RTC compensation curve config registers(0x44-0x60/0x130-0x148) password register */
    __IO uint32_t RESERVED0;                 /*!< offset: 0x044 */
    __IO uint32_t DOTA0;                     /*!< offset: 0x048 info: initial freq deviation register */
    __IO uint32_t ALPHAL;                    /*!< offset: 0x04C info: RTC low temperature coefficient ALPHA config register(Coefficient of x^2) */
    __IO uint32_t ALPHAH;                    /*!< offset: 0x050 info: RTC high temperature coefficient ALPHA config register(Coefficient of x^2) */
    __IO uint32_t XT0;                       /*!< offset: 0x054 info: RTC temperature curve vertex temperature XT0 register */
    __IO uint32_t RESERVED1[3];              /*!< offset: 0x058 ~ 060 */
    __IO uint32_t DOTAT;                     /*!< offset: 0x064 info: RTC compensation DOTAT register. */
    __IO uint32_t RESERVED2;                 /*!< offset: 0x068 */
    __IO uint32_t LOSC_CFG1;                 /*!< offset: 0x06C info: LOSC config register */
    __IO uint32_t RESERVED3;                 /*!< offset: 0x070 */
    __IO uint32_t FPTR;                      /*!< offset: 0x074 info: RTC PLL compensation register */
    __IO uint32_t FDTR1S;                    /*!< offset: 0x078 info: RTC compensation register(per 1 seconds) */
    __IO uint32_t FDTR30S;                   /*!< offset: 0x07C info: RTC compensation register(per 30 seconds) */
    __IO uint32_t RESERVED4;                 /*!< offset: 0x080 */
    __IO uint32_t FDTR120S;                  /*!< offset: 0x084 info: RTC compensation register(per 120 seconds) */
    __IO uint32_t IOEN;                      /*!< offset: 0x088 info: IO config enable register */
    __IO uint32_t IOMODE;                    /*!< offset: 0x08C info: IO mode selection register */
    __IO uint32_t TAMPER1N0_TIME0;           /*!< offset: 0x090 info: TAMPER1 1st negedge freeze time register No.0 (seconds minute) */
    __IO uint32_t TAMPER1N0_TIME1;           /*!< offset: 0x094 info: TAMPER1 1st negedge freeze time register No.1 (hours date) */
    __IO uint32_t TAMPER1N0_TIME2;           /*!< offset: 0x098 info: TAMPER1 1st negedge freeze time register No.2 (month year) */
    __IO uint32_t TAMPER1P0_TIME0;           /*!< offset: 0x09C info: TAMPER1 1st posedge freeze time register No.0 (seconds minute) */
    __IO uint32_t TAMPER1P0_TIME1;           /*!< offset: 0x0A0 info: TAMPER1 1st posedge freeze time register No.1 (hours date) */
    __IO uint32_t TAMPER1P0_TIME2;           /*!< offset: 0x0A4 info: TAMPER1 1st posedge freeze time register No.2 (month year) */
    __IO uint32_t TAMPER1N1_TIME0;           /*!< offset: 0x0A8 info: TAMPER1 2st negedge freeze time register No.0 (seconds minute) */
    __IO uint32_t TAMPER1N1_TIME1;           /*!< offset: 0x0AC info: TAMPER1 2st negedge freeze time register No.1 (hours date) */
    __IO uint32_t TAMPER1N1_TIME2;           /*!< offset: 0x0B0 info: TAMPER1 2st negedge freeze time register No.2 (month year) */
    __IO uint32_t TAMPER1P1_TIME0;           /*!< offset: 0x0B4 info: TAMPER1 2st posedge freeze time register No.0 (seconds minute) */
    __IO uint32_t TAMPER1P1_TIME1;           /*!< offset: 0x0B8 info: TAMPER1 2st posedge freeze time register No.1 (hours date) */
    __IO uint32_t TAMPER1P1_TIME2;           /*!< offset: 0x0BC info: TAMPER1 2st posedge freeze time register No.2 (month year) */
    __IO uint32_t RESERVED5;                 /*!< offset: 0x0C0 */
    __IO uint32_t TEMPOS;                    /*!< offset: 0x0C4 info: auto compensation mode temperature offset register */
    __IO uint32_t TPSIN;                     /*!< offset: 0x0C8 info: RTC auto temperature compensation ADC measure value input register */
    __IO uint32_t CALPS;                     /*!< offset: 0x0CC info: Password register for RTC calibration registers(T0-T9) */
    __IO uint32_t CAL_T0;                    /*!< offset: 0x0D0 info: RTC calibration register T0 */
    __IO uint32_t CAL_T1;                    /*!< offset: 0x0D4 info: RTC calibration register T1 */
    __IO uint32_t CAL_T2;                    /*!< offset: 0x0D8 info: RTC calibration register T2 */
    __IO uint32_t CAL_T3;                    /*!< offset: 0x0DC info: RTC calibration register T3 */
    __IO uint32_t CAL_T4;                    /*!< offset: 0x0E0 info: RTC calibration register T4 */
    __IO uint32_t CAL_T5;                    /*!< offset: 0x0E4 info: RTC calibration register T5 */
    __IO uint32_t CAL_T6;                    /*!< offset: 0x0E8 info: RTC calibration register T6 */
    __IO uint32_t CAL_T7;                    /*!< offset: 0x0EC info: RTC calibration register T7 */
    __IO uint32_t CAL_T8;                    /*!< offset: 0x0F0 info: RTC calibration register T8 */
    __IO uint32_t CAL_T9;                    /*!< offset: 0x0F4 info: RTC calibration register T9 */
    __IO uint32_t TEMP2;                     /*!< offset: 0x0F8 info: current chip temperature register No.2 */
    __IO uint32_t RESERVED6;                 /*!< offset: 0x0FC */
    __IO uint32_t TAMPER2N0_TIME0;           /*!< offset: 0x100 info: TAMPER2 1st negedge freeze time register No.0 (seconds minute) */
    __IO uint32_t TAMPER2N0_TIME1;           /*!< offset: 0x104 info: TAMPER2 1st negedge freeze time register No.1 (hours date) */
    __IO uint32_t TAMPER2N0_TIME2;           /*!< offset: 0x108 info: TAMPER2 1st negedge freeze time register No.2 (month year) */
    __IO uint32_t TAMPER2P0_TIME0;           /*!< offset: 0x10C info: TAMPER2 1st posedge freeze time register No.0 (seconds minute) */
    __IO uint32_t TAMPER2P0_TIME1;           /*!< offset: 0x110 info: TAMPER2 1st posedge freeze time register No.1 (hours date) */
    __IO uint32_t TAMPER2P0_TIME2;           /*!< offset: 0x114 info: TAMPER2 1st posedge freeze time register No.2 (month year) */
    __IO uint32_t TAMPER2N1_TIME0;           /*!< offset: 0x118 info: TAMPER2 2st negedge freeze time register No.0 (seconds minute) */
    __IO uint32_t TAMPER2N1_TIME1;           /*!< offset: 0x11C info: TAMPER2 2st negedge freeze time register No.1 (hours date) */
    __IO uint32_t TAMPER2N1_TIME2;           /*!< offset: 0x120 info: TAMPER2 2st negedge freeze time register No.2 (month year).When TAMPER_IF[4] = 1, the software-triggered time freeze(sub-seconds) */
    __IO uint32_t TAMPER2P1_TIME0;           /*!< offset: 0x124 info: TAMPER2 2st posedge freeze time register No.0 (seconds minute). When TAMPER_IF[4] = 1, the software-triggered time freeze (seconds minute). */
    __IO uint32_t TAMPER2P1_TIME1;           /*!< offset: 0x128 info: TAMPER2 2st posedge freeze time register No.1 (hours date). When TAMPER_IF[4] = 1, the software-triggered time freeze (hours date). */
    __IO uint32_t TAMPER2P1_TIME2;           /*!< offset: 0x12C info: TAMPER2 2st posedge freeze time register No.2 (month year). When TAMPER_IF[4] = 1, the software-triggered time freeze (month year). */
    __IO uint32_t MODE1;                     /*!< offset: 0x130 info: temperature compensation mode seletion register No.1 */
    __IO uint32_t XT1;                       /*!< offset: 0x134 info: RTC temperature curve vertex temperature XT1 register */
    __IO uint32_t ALPHA;                     /*!< offset: 0x138 info: temperature coefficient ALPHA config register(Coefficient of x^1) */
    __IO uint32_t BETA;                      /*!< offset: 0x13C info: temperature coefficient BETA config register(Coefficient of x^2) */
    __IO uint32_t GAMMA;                     /*!< offset: 0x140 info: temperature coefficient GAMMA config register(Coefficient of x^3) */
    __IO uint32_t ZETA;                      /*!< offset: 0x144 info: temperature coefficient ZETA config register(part1)(Coefficient of x^4 part1) */
    __IO uint32_t ZETAH;                     /*!< offset: 0x148 info: temperature coefficient ZETA config register(part2)(Coefficient of x^4 part2) */
    __IO uint32_t RESERVED7;                 /*!< offset: 0x14C */
    __IO uint32_t TPS_START;                 /*!< offset: 0x150 info: RTC temperature measure start register */
    __IO uint32_t TEMP_CAL;                  /*!< offset: 0x154 info: RTC temperature register No.3(when config as RTC_CTRL_TSE=2,the register is temperature measure result after setted RTC_TPS_START and got flag of RTC_CTRL_CAL_BUSY=0 ) */
    __IO uint32_t SSC;                       /*!< offset: 0x158 info: Sub second value register */
    __IO uint32_t RESERVED8;                 /*!< offset: 0x15C */
    __IO uint32_t TAMPERIE;                  /*!< offset: 0x160 info: Cover Detection Mechanism(tamper detection) interrupt enable register */
    __IO uint32_t TAMPERIF;                  /*!< offset: 0x164 info: Cover Detection Mechanism(tamper detection) interrupt flag register */
    __IO uint32_t PLLOUT_CFG;                /*!< offset: 0x168 info: output configuration from pll clock source */
    __IO uint32_t RESERVED9[9];              /*!< offset: 0x16C ~ 18C */
    __IO uint32_t BKPPS;                     /*!< offset: 0x190 info: bakeup registers password(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP0;                      /*!< offset: 0x194 info: bakeup register No.0(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP1;                      /*!< offset: 0x198 info: bakeup register No.1(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP2;                      /*!< offset: 0x19C info: bakeup register No.2(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP3;                      /*!< offset: 0x1A0 info: bakeup register No.3(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP4;                      /*!< offset: 0x1A4 info: bakeup register No.4(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP5;                      /*!< offset: 0x1A8 info: bakeup register No.5(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP6;                      /*!< offset: 0x1AC info: bakeup register No.6(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP7;                      /*!< offset: 0x1B0 info: bakeup register No.7(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP8;                      /*!< offset: 0x1B4 info: bakeup register No.8(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP9;                      /*!< offset: 0x1B8 info: bakeup register No.9(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP10;                     /*!< offset: 0x1BC info: bakeup register No.10(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP11;                     /*!< offset: 0x1C0 info: bakeup register No.11(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP12;                     /*!< offset: 0x1C4 info: bakeup register No.12(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP13;                     /*!< offset: 0x1C8 info: bakeup register No.13(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP14;                     /*!< offset: 0x1CC info: bakeup register No.14(this register only reset when VBAT POR reset) */
    __IO uint32_t BKP15;                     /*!< offset: 0x1D0 info: bakeup register No.15(this register only reset when VBAT POR reset) */
    __IO uint32_t KEYOUT_TK_CFG;             /*!< offset: 0x1D4 info: KEYOUT Periodic Trigger Configuration Register */
    __IO uint32_t KEYOUT_SREL;               /*!< offset: 0x1D8 info: KEYOUT Software release register */
} RTC_TypeDef;

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
#define RTC_CORE0_BASE  (0x200B4000U)
#define RTC_CORE1_BASE  (0xA00B4000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define RTC_CORE0  ((RTC_TypeDef *)      RTC_CORE0_BASE)
#define RTC_CORE1  ((RTC_TypeDef *)      RTC_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define RTC  RTC_CORE0
#endif
#if defined(CORE1)
#define RTC  RTC_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** RTC Instances *******************************/
#define IS_RTC_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == RTC_CORE0) || \
                                          ((INSTANCE) == RTC_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* RTC_CTRL */
#define RTC_CTRL_FOUT                        (BITS(0,2))                                         /*!< RTC output frequency selections FOUT bits. */
#define RTC_CTRL_TCP                         (BITS(3,5))                                         /*!< temperature compensation period bits */
#define RTC_CTRL_TSE                         (BITS(6,7))                                         /*!< temparature compentation mode selection config bits. */
#define RTC_CTRL_WRTC                        (BIT(8))                                            /*!< RTC register write enable/disable bit(controls offset 0x00-0x1C/0x3C/0xC4/0x168, except this field.) */
#define RTC_CTRL_WRBUSY                      (BIT(9))                                            /*!< RTC register writing busy flag (RTC register only can be changed or set when Wr_busy is 0) */
#define RTC_CTRL_CALBUSY                     (BIT(10))                                           /*!< RTC calibration busy flag */
#define RTC_CTRL_STF_TRIG                    (BIT(12))                                           /*!< software time freeze trigger bit(freezed timestamp will be save to 0x120~0x12C) */

/* RTC_SC */
#define RTC_SC_SC                            (BITS(0,6))                                         /*!< seconds bits (BCD format, SC[6:4] standing for 10 seconds,SC[3:0] standing seconds) */

/* RTC_MN */
#define RTC_MN_MN                            (BITS(0,6))                                         /*!< minutes bits (BCD format, MN[6:4] standing for 10 minutes,MN[3:0] standing for minutes) */

/* RTC_HR */
#define RTC_HR_HR                            (BITS(0,5))                                         /*!< hours bits (BCD format, HR[5:4] standing for 10 hours,HR[3:0] standing for hours) */

/* RTC_DT */
#define RTC_DT_DT                            (BITS(0,5))                                         /*!< date bits (BCD format, DT[5:4] standing for 10 date,DT[3:0] standing for date) */

/* RTC_MO */
#define RTC_MO_MO                            (BITS(0,4))                                         /*!< month bits (BCD format, MO[5] standing for 10 month,MO[3:0] standing for month) */

/* RTC_YR */
#define RTC_YR_YR                            (BITS(0,7))                                         /*!< year bits (BCD format, YR[7:4] standing for 10 year,YR[3:0] standing for year) */

/* RTC_DW */
#define RTC_DW_DW                            (BITS(0,2))                                         /*!< day of the week bits */

/* RTC_CNT1 */
#define RTC_CNT1_CNT                         (BITS(0,7))                                         /*!< RTC counter1 perset value bits(set CNT1 flag every (CNT+1) seconds) */
#define RTC_CNT1_PD                          (BIT(8))                                            /*!< RTC CNT1 ON/OFF control bit */
#define RTC_CNT1_DBGSTB                      (BIT(10))                                           /*!< RTC_CNT1 debugmode halt counting enable/disable bit */

/* RTC_CNT2 */
#define RTC_CNT2_CNT                         (BITS(0,7))                                         /*!< RTC counter2 perset value bits(set counter2 flag every (CNT+1)/256 seconds) */
#define RTC_CNT2_PD                          (BIT(8))                                            /*!< RTC CNT2 ON/OFF control bit */
#define RTC_CNT2_CLK_SEL                     (BIT(9))                                            /*!< CNT2 CLK selection */
#define RTC_CNT2_DBGSTB                      (BIT(10))                                           /*!< RTC_CNT2 debugmode halt counting enable/disable bit */

/* RTC_SCA */
#define RTC_SCA_SCA                          (BITS(0,6))                                         /*!< alarm seconds bits (BCD format,SCA[6:4] standing for 10 seconds,SCA[3:0] standing for seconds) */

/* RTC_MNA */
#define RTC_MNA_MNA                          (BITS(0,6))                                         /*!< alarm minitues bits (BCD format,MNA[6:4] standing for 10 minitues,MNA[3:0] standing for minitues) */

/* RTC_HRA */
#define RTC_HRA_HRA                          (BITS(0,5))                                         /*!< alarm hours bits (BCD format,MNA[5:4] standing for 10 hours,MNA[3:0] standing for hours) */

/* RTC_IE */
#define RTC_IE_ALMIE                         (BIT(0))                                            /*!< alarm interrupt enable bit */
#define RTC_IE_CNT1IE                        (BIT(1))                                            /*!< RTC counter1 interrupt enable bit */
#define RTC_IE_CNT2IE                        (BIT(2))                                            /*!< RTC counter2 interrupt enable bit */
#define RTC_IE_SCIE                          (BIT(3))                                            /*!< sencond interrupt enable bit */
#define RTC_IE_MNIE                          (BIT(4))                                            /*!< minute interrupt enable bit */
#define RTC_IE_HRIE                          (BIT(5))                                            /*!< hour interrupt enable bit */
#define RTC_IE_DTIE                          (BIT(6))                                            /*!< date interrupt enable bit */
#define RTC_IE_MOIE                          (BIT(7))                                            /*!< month interrupt enable bit */
#define RTC_IE_IECLKEN                       (BIT(8))                                            /*!< interrupt clock enable bit.enabled when any bit of RTC_IE[8:0] is high */
#define RTC_IE_RTC_1S_SEL                    (BIT(9))                                            /*!< RTC_1S clock source selection bit */

/* RTC_IF */
#define RTC_IF_ALMIF                         (BIT(0))                                            /*!< alarm event interrupt flag */
#define RTC_IF_CNT1IF                        (BIT(1))                                            /*!< RTC counter1 event interrupt flag */
#define RTC_IF_CNT2IF                        (BIT(2))                                            /*!< RTC counter2 event interrupt flag */
#define RTC_IF_SCIF                          (BIT(3))                                            /*!< sencond event interrupt flag */
#define RTC_IF_MNIF                          (BIT(4))                                            /*!< minute event interrupt flag */
#define RTC_IF_HRIF                          (BIT(5))                                            /*!< hour event interrupt flag */
#define RTC_IF_DTIF                          (BIT(6))                                            /*!< date event interrupt flag */
#define RTC_IF_MOIF                          (BIT(7))                                            /*!< month event interrupt flag */

/* RTC_TEMP */
#define RTC_TEMP_TEMP                        (BITS(0,15))                                        /*!< current chip temperature bits.[9:0]([15:0]):TEMP[9]([15]) for sign, TEMP[8:2]([15:6]) for integer part,TEMP[1:0]([5:0]) for decimal part) */

/* RTC_PS */
#define RTC_PS_PS                            (BITS(0,7))                                         /*!< Password bits for competition registers(44H~54H,130H~148H) */

/* RTC_DOTA0 */
#define RTC_DOTA0_DOTA0                      (BITS(0,15))                                        /*!< initial freq deviation bits */

/* RTC_ALPHAL */
#define RTC_ALPHAL_ALPHAL                    (BITS(0,10))                                        /*!< RTC low temperature coefficient ALPHA config  bits */

/* RTC_ALPHAH */
#define RTC_ALPHAH_ALPHAH                    (BITS(0,10))                                        /*!< RTC high temperature coefficient ALPHA config  bits */

/* RTC_XT0 */
#define RTC_XT0_XT0                          (BITS(0,6))                                         /*!< RTC temperature curve vertex temperature bits */

/* RTC_DOTAT */
#define RTC_DOTAT_DOTAT                      (BITS(0,15))                                        /*!< RTC compensation calculation result DOTAT bits */

/* RTC_LOSC_CFG1 */
#define RTC_LOSC_CFG1_LOSC_PD                (BITS(0,7))                                         /*!< LOSC enable/disable bits */
#define RTC_LOSC_CFG1_LOSC_WEN               (BIT(8))                                            /*!< LOSC enable bits (RTC_LOSC_CFG1_PD) write enable bit */
#define RTC_LOSC_CFG1_LOSC_FAIL              (BIT(9))                                            /*!< LOSC running fail flag */
#define RTC_LOSC_CFG1_VCC_BOI_FLAG           (BIT(10))                                           /*!< VCC Brown Out Indication flag */

/* RTC_FPTR */
#define RTC_FPTR_FPTR                        (BITS(0,16))                                        /*!< PLL compensation bits */

/* RTC_FDTR1S */
#define RTC_FDTR1S_FDTR1S                    (BITS(0,5))                                         /*!< RTC compensation bits */

/* RTC_FDTR30S */
#define RTC_FDTR30S_FDTR30S                  (BITS(0,6))                                         /*!< RTC compensation bits */

/* RTC_FDTR120S */
#define RTC_FDTR120S_FDTR120S                (BITS(0,8))                                         /*!< RTC compensation bits */

/* RTC_IOEN */
#define RTC_IOEN_PL01EN                      (BITS(0,3))                                         /*!< PL01 function configuration bits(only available when MADC_ANA_PAD_PL01_AE field =0 ) */
#define RTC_IOEN_PL00EN                      (BITS(4,7))                                         /*!< PL00 function configuration bits(higher priority than the VCC domain) */
#define RTC_IOEN_PL02EN                      (BITS(8,11))                                        /*!< PL02 function configuration bits(only available when MADC_ANA_PAD_PL02_AE field =0 ) */
#define RTC_IOEN_PL03EN                      (BITS(12,15))                                       /*!< PL03 function configuration bits(only available when MADC_ANA_PAD_PL03_AE field =0 ) */

/* RTC_IOMODE */
#define RTC_IOMODE_TAMPER1_MODE              (BIT(0))                                            /*!< TAMPER1 open cover detection mode selection */
#define RTC_IOMODE_TAMPER2_MODE              (BIT(1))                                            /*!< TAMPER2 open cover detection mode selection */
#define RTC_IOMODE_KEYOUT_MODE               (BIT(2))                                            /*!< KEYOUT output mode config() */
#define RTC_IOMODE_PADEN_PS                  (BITS(4,7))                                         /*!< RTC_IOEN write enable/disable configuration */
#define RTC_IOMODE_TAMPER_FLT_CNT            (BITS(8,12))                                        /*!< TAMPER1/TAMPER2 filter cycles length selection */
#define RTC_IOMODE_TAMPER_FLT_TS             (BIT(13))                                           /*!< TAMPER1/TAMPER2 filter precision selection */
#define RTC_IOMODE_TAMPER_FLT_PS             (BIT(14))                                           /*!< Write enable bit for TAMPER_FLT_TS and TAMPER_FLT_CNT */

/* RTC_TAMPER1N0_TIME0 */
#define RTC_TAMPER1N0_TIME0_SEC              (BITS(0,7))                                         /*!< 1st negedge seconds timestamp */
#define RTC_TAMPER1N0_TIME0_MIN              (BITS(8,15))                                        /*!< 1st negedge minute timestamp */

/* RTC_TAMPER1N0_TIME1 */
#define RTC_TAMPER1N0_TIME1_HOUR             (BITS(0,7))                                         /*!< 1st negedge hours timestamp */
#define RTC_TAMPER1N0_TIME1_DAY              (BITS(8,15))                                        /*!< 1st negedge day timestamp */

/* RTC_TAMPER1N0_TIME2 */
#define RTC_TAMPER1N0_TIME2_MT               (BITS(0,7))                                         /*!< 1st negedge month timestamp */
#define RTC_TAMPER1N0_TIME2_YR               (BITS(8,15))                                        /*!< 1st negedge year timestamp */

/* RTC_TAMPER1P0_TIME0 */
#define RTC_TAMPER1P0_TIME0_SEC              (BITS(0,7))                                         /*!< 1st posedge seconds timestamp */
#define RTC_TAMPER1P0_TIME0_MIN              (BITS(8,15))                                        /*!< 1st posedge minute timestamp */

/* RTC_TAMPER1P0_TIME1 */
#define RTC_TAMPER1P0_TIME1_HOUR             (BITS(0,7))                                         /*!< 1st posedge hours timestamp */
#define RTC_TAMPER1P0_TIME1_DAY              (BITS(8,15))                                        /*!< 1st posedge day timestamp */

/* RTC_TAMPER1P0_TIME2 */
#define RTC_TAMPER1P0_TIME2_MT               (BITS(0,7))                                         /*!< 1st posedge month timestamp */
#define RTC_TAMPER1P0_TIME2_YR               (BITS(8,15))                                        /*!< 1st posedge year timestamp */

/* RTC_TAMPER1N1_TIME0 */
#define RTC_TAMPER1N1_TIME0_SEC              (BITS(0,7))                                         /*!< 2nd negedge seconds timestamp */
#define RTC_TAMPER1N1_TIME0_MIN              (BITS(8,15))                                        /*!< 2nd negedge minute timestamp */

/* RTC_TAMPER1N1_TIME1 */
#define RTC_TAMPER1N1_TIME1_HOUR             (BITS(0,7))                                         /*!< 2nd negedge hours timestamp */
#define RTC_TAMPER1N1_TIME1_DAY              (BITS(8,15))                                        /*!< 2nd negedge day timestamp */

/* RTC_TAMPER1N1_TIME2 */
#define RTC_TAMPER1N1_TIME2_MT               (BITS(0,7))                                         /*!< 2nd negedge month timestamp */
#define RTC_TAMPER1N1_TIME2_YR               (BITS(8,15))                                        /*!< 2nd negedge year timestamp */

/* RTC_TAMPER1P1_TIME0 */
#define RTC_TAMPER1P1_TIME0_SEC              (BITS(0,7))                                         /*!< 2nd posedge seconds timestamp */
#define RTC_TAMPER1P1_TIME0_MIN              (BITS(8,15))                                        /*!< 2nd posedge minute timestamp */

/* RTC_TAMPER1P1_TIME1 */
#define RTC_TAMPER1P1_TIME1_HOUR             (BITS(0,7))                                         /*!< 2nd posedge hours timestamp */
#define RTC_TAMPER1P1_TIME1_DAY              (BITS(8,15))                                        /*!< 2nd posedge day timestamp */

/* RTC_TAMPER1P1_TIME2 */
#define RTC_TAMPER1P1_TIME2_MT               (BITS(0,7))                                         /*!< 2nd posedge month timestamp */
#define RTC_TAMPER1P1_TIME2_YR               (BITS(8,15))                                        /*!< 2nd posedge year timestamp */

/* RTC_TEMPOS */
#define RTC_TEMPOS_TEMPOS                    (BITS(0,5))                                         /*!< auto compensation mode temperature offset bits */

/* RTC_TPSIN */
#define RTC_TPSIN_TPSIN                      (BITS(0,12))                                        /*!< RTC auto temperature compensation ADC measure value input bits */

/* RTC_CALPS */
#define RTC_CALPS_CALPS                      (BITS(0,7))                                         /*!< Enable RTC secondary calibration registers */

/* RTC_CAL_T0 */
#define RTC_CAL_T0_CAL_T0                    (BITS(0,7))                                         /*!< RTC calibration register T0 bits (to calibrate RTC for 0.25ppm per bit when temprature is lower than -30 degree) */

/* RTC_CAL_T1 */
#define RTC_CAL_T1_CAL_T1                    (BITS(0,7))                                         /*!< RTC calibration register T1 bits (to calibrate RTC for 0.25ppm per bit when temprature is between -30 and -20 degree) */

/* RTC_CAL_T2 */
#define RTC_CAL_T2_CAL_T2                    (BITS(0,7))                                         /*!< RTC calibration register T2 bits (to calibrate RTC for 0.25ppm per bit when temprature is between -20 and -10 degree) */

/* RTC_CAL_T3 */
#define RTC_CAL_T3_CAL_T3                    (BITS(0,7))                                         /*!< RTC calibration register T3 bits (to calibrate RTC for 0.25ppm per bit when temprature is between -10 and 0 degree) */

/* RTC_CAL_T4 */
#define RTC_CAL_T4_CAL_T4                    (BITS(0,7))                                         /*!< RTC calibration register T4 bits (to calibrate RTC for 0.25ppm per bit when temprature is between 0 and 10 degree) */

/* RTC_CAL_T5 */
#define RTC_CAL_T5_CAL_T5                    (BITS(0,7))                                         /*!< RTC calibration register T5 bits (to calibrate RTC for 0.25ppm per bit when temprature is between 35 and 45 degree) */

/* RTC_CAL_T6 */
#define RTC_CAL_T6_CAL_T6                    (BITS(0,7))                                         /*!< RTC calibration register T6 bits (to calibrate RTC for 0.25ppm per bit when temprature is between 45 and 55 degree) */

/* RTC_CAL_T7 */
#define RTC_CAL_T7_CAL_T7                    (BITS(0,7))                                         /*!< RTC calibration register T7 bits (to calibrate RTC for 0.25ppm per bit when temprature is between 55 and 65 degree) */

/* RTC_CAL_T8 */
#define RTC_CAL_T8_CAL_T8                    (BITS(0,7))                                         /*!< RTC calibration register T8 bits (to calibrate RTC for 0.25ppm per bit when temprature is between 65 and 75 degree) */

/* RTC_CAL_T9 */
#define RTC_CAL_T9_CAL_T9                    (BITS(0,7))                                         /*!< RTC calibration register T9 bits (to calibrate RTC for 0.25ppm per bit when temprature is higher than 75 degree) */

/* RTC_TEMP2 */
#define RTC_TEMP2_TEMP2                      (BITS(0,11))                                        /*!< current chipte mperature bits(TEMP[11] for sign, TEMP[10:4] for integer part,TEMP[3:0] for decimal part) */

/* RTC_TAMPER2N0_TIME0 */
#define RTC_TAMPER2N0_TIME0_SEC              (BITS(0,7))                                         /*!< 1st negedge seconds timestamp */
#define RTC_TAMPER2N0_TIME0_MIN              (BITS(8,15))                                        /*!< 1st negedge minute timestamp */

/* RTC_TAMPER2N0_TIME1 */
#define RTC_TAMPER2N0_TIME1_HOUR             (BITS(0,7))                                         /*!< 1st negedge hours timestamp */
#define RTC_TAMPER2N0_TIME1_DAY              (BITS(8,15))                                        /*!< 1st negedge day timestamp */

/* RTC_TAMPER2N0_TIME2 */
#define RTC_TAMPER2N0_TIME2_MT               (BITS(0,7))                                         /*!< 1st negedge month timestamp */
#define RTC_TAMPER2N0_TIME2_YR               (BITS(8,15))                                        /*!< 1st negedge year timestamp */

/* RTC_TAMPER2P0_TIME0 */
#define RTC_TAMPER2P0_TIME0_SEC              (BITS(0,7))                                         /*!< 1st posedge seconds timestamp */
#define RTC_TAMPER2P0_TIME0_MIN              (BITS(8,15))                                        /*!< 1st posedge minute timestamp */

/* RTC_TAMPER2P0_TIME1 */
#define RTC_TAMPER2P0_TIME1_HOUR             (BITS(0,7))                                         /*!< 1st posedge hours timestamp */
#define RTC_TAMPER2P0_TIME1_DAY              (BITS(8,15))                                        /*!< 1st posedge day timestamp */

/* RTC_TAMPER2P0_TIME2 */
#define RTC_TAMPER2P0_TIME2_MT               (BITS(0,7))                                         /*!< 1st posedge month timestamp */
#define RTC_TAMPER2P0_TIME2_YR               (BITS(8,15))                                        /*!< 1st posedge year timestamp */

/* RTC_TAMPER2N1_TIME0 */
#define RTC_TAMPER2N1_TIME0_SEC              (BITS(0,7))                                         /*!< 2nd negedge seconds timestamp */
#define RTC_TAMPER2N1_TIME0_MIN              (BITS(8,15))                                        /*!< 2nd negedge minute timestamp */

/* RTC_TAMPER2N1_TIME1 */
#define RTC_TAMPER2N1_TIME1_HOUR             (BITS(0,7))                                         /*!< 2nd negedge hours timestamp */
#define RTC_TAMPER2N1_TIME1_DAY              (BITS(8,15))                                        /*!< 2nd negedge day timestamp */

/* RTC_TAMPER2N1_TIME2 */
#define RTC_TAMPER2N1_TIME2_MT               (BITS(0,7))                                         /*!< 2nd negedge month timestamp */
#define RTC_TAMPER2N1_TIME2_YR               (BITS(8,15))                                        /*!< 2nd negedge year timestamp */

/* RTC_TAMPER2P1_TIME0 */
#define RTC_TAMPER2P1_TIME0_SEC              (BITS(0,7))                                         /*!< 2nd posedge(or software) freeze seconds timestamp */
#define RTC_TAMPER2P1_TIME0_MIN              (BITS(8,15))                                        /*!< 2nd posedge(or software) freeze minute timestamp */

/* RTC_TAMPER2P1_TIME1 */
#define RTC_TAMPER2P1_TIME1_HOUR             (BITS(0,7))                                         /*!< 2nd posedge(or software) freeze hours timestamp */
#define RTC_TAMPER2P1_TIME1_DAY              (BITS(8,15))                                        /*!< 2nd posedge(or software) freeze day timestamp */

/* RTC_TAMPER2P1_TIME2 */
#define RTC_TAMPER2P1_TIME2_MT               (BITS(0,7))                                         /*!< 2nd posedge(or software) freeze month timestamp */
#define RTC_TAMPER2P1_TIME2_YR               (BITS(8,15))                                        /*!< 2nd posedge(or software) freeze year timestamp */

/* RTC_MODE1 */
#define RTC_MODE1_CURVE_SEL                  (BITS(0,15))                                        /*!< temperature compensation mode seletion bits */

/* RTC_XT1 */
#define RTC_XT1_XT1                          (BITS(0,12))                                        /*!< RTC temperature curve vertex temperature bits */

/* RTC_ALPHA */
#define RTC_ALPHA_ALPHA                      (BITS(0,15))                                        /*!< temperature coefficient ALPHA config bits */

/* RTC_BETA */
#define RTC_BETA_BETA                        (BITS(0,14))                                        /*!< temperature coefficient BETA config bits */

/* RTC_GAMMA */
#define RTC_GAMMA_GAMMA                      (BITS(0,15))                                        /*!< temperature coefficient GAMMA config bits */

/* RTC_ZETA */
#define RTC_ZETA_ZETA                        (BITS(0,15))                                        /*!< temperature coefficient ZETA config bits(part1) */

/* RTC_ZETAH */
#define RTC_ZETAH_ZETAH                      (BITS(0,1))                                         /*!< temperature coefficient ZETA config bits(part2) */

/* RTC_TPS_START */
#define RTC_TPS_START_TPS_START              (BIT(0))                                            /*!< RTC temperature measure start bit */

/* RTC_TEMP_CAL */
#define RTC_TEMP_CAL_TEMP_CAL                (BITS(0,11))                                        /*!< RTC temperature register No.3 bits */

/* RTC_SSC */
#define RTC_SSC_SSC                          (BITS(0,15))                                        /*!< Sub second value bits(1LSB=30.518us=1/32768s) */

/* RTC_TAMPERIE */
#define RTC_TAMPERIE_TAMPER1PIE              (BIT(0))                                            /*!< TAMPER1 Positive edge interrupt enable bit */
#define RTC_TAMPERIE_TAMPER1NIE              (BIT(1))                                            /*!< TAMPER1 Negative edge  interrupt enable bit */
#define RTC_TAMPERIE_TAMPER2PIE              (BIT(2))                                            /*!< TAMPER2 Positive edge interrupt enable bit */
#define RTC_TAMPERIE_TAMPER2NIE              (BIT(3))                                            /*!< TAMPER2 Negative edge  interrupt enable bit */
#define RTC_TAMPERIE_STFIE                   (BIT(4))                                            /*!< Software time freeze interrupt enable bit */
#define RTC_TAMPERIE_KEYOTKIE                (BIT(5))                                            /*!< KEYOUT timed trigger interrupt enable bit */

/* RTC_TAMPERIF */
#define RTC_TAMPERIF_TAMPER1PIF              (BIT(0))                                            /*!< TAMPER1 Positive edge interrupt flag bit(when get flag, wait >35us write to clear) */
#define RTC_TAMPERIF_TAMPER1NIF              (BIT(1))                                            /*!< TAMPER1 Negative edge  interrupt flag bit(when get flag, wait >35us write to clear) */
#define RTC_TAMPERIF_TAMPER2PIF              (BIT(2))                                            /*!< TAMPER2 Positive edge interrupt flag bit(when get flag, wait >35us write to clear) */
#define RTC_TAMPERIF_TAMPER2NIF              (BIT(3))                                            /*!< TAMPER2 Negative edge  interrupt flag bit(when get flag, wait >35us write to clear) */
#define RTC_TAMPERIF_STFIF                   (BIT(4))                                            /*!< Software time freeze interrupt flag bit */
#define RTC_TAMPERIF_KEYOTKIF                (BIT(5))                                            /*!< KEYOUT timed trigger interrupt flag bit(when get flag, wait >35us write to clear) */

/* RTC_PLLOUT_CFG */
#define RTC_PLLOUT_CFG_FREQ_SEL              (BIT(0))                                            /*!< RTCOUT output signal frequency selection, when source clock was selected as PLL */

/* RTC_BKPPS */
#define RTC_BKPPS_PS                         (BITS(0,7))                                         /*!< Password bits for RTC user defines registers */

/* RTC_BKP0 */
#define RTC_BKP0_BKP                         (BITS(0,15))                                        /*!< bakeup register No.0 bits */

/* RTC_BKP1 */
#define RTC_BKP1_BKP                         (BITS(0,15))                                        /*!< bakeup register No.1 bits */

/* RTC_BKP2 */
#define RTC_BKP2_BKP                         (BITS(0,15))                                        /*!< bakeup register No.2 bits */

/* RTC_BKP3 */
#define RTC_BKP3_BKP                         (BITS(0,15))                                        /*!< bakeup register No.3 bits */

/* RTC_BKP4 */
#define RTC_BKP4_BKP                         (BITS(0,15))                                        /*!< bakeup register No.4 bits */

/* RTC_BKP5 */
#define RTC_BKP5_BKP                         (BITS(0,15))                                        /*!< bakeup register No.5 bits */

/* RTC_BKP6 */
#define RTC_BKP6_BKP                         (BITS(0,15))                                        /*!< bakeup register No.6 bits */

/* RTC_BKP7 */
#define RTC_BKP7_BKP                         (BITS(0,15))                                        /*!< bakeup register No.7 bits */

/* RTC_BKP8 */
#define RTC_BKP8_BKP                         (BITS(0,15))                                        /*!< bakeup register No.8 bits */

/* RTC_BKP9 */
#define RTC_BKP9_BKP                         (BITS(0,15))                                        /*!< bakeup register No.9 bits */

/* RTC_BKP10 */
#define RTC_BKP10_BKP                        (BITS(0,15))                                        /*!< bakeup register No.10 bits */

/* RTC_BKP11 */
#define RTC_BKP11_BKP                        (BITS(0,15))                                        /*!< bakeup register No.11 bits */

/* RTC_BKP12 */
#define RTC_BKP12_BKP                        (BITS(0,15))                                        /*!< bakeup register No.12 bits */

/* RTC_BKP13 */
#define RTC_BKP13_BKP                        (BITS(0,15))                                        /*!< bakeup register No.13 bits */

/* RTC_BKP14 */
#define RTC_BKP14_BKP                        (BITS(0,15))                                        /*!< bakeup register No.14 bits */

/* RTC_BKP15 */
#define RTC_BKP15_BKP                        (BITS(0,15))                                        /*!< bakeup register No.15 bits */

/* RTC_KEYOUT_TK_CFG */
#define RTC_KEYOUT_TK_CFG_EN                 (BITS(0,7))                                         /*!< KEYOUT timed trigger enable bits */
#define RTC_KEYOUT_TK_CFG_TB                 (BIT(8))                                            /*!< KEYOUT timing base config bit */
#define RTC_KEYOUT_TK_CFG_CNT                (BITS(9,14))                                        /*!< KEYOUT timing count target value config bits. KEYOUT timed trigger time is (CNT+1)*TS */
#define RTC_KEYOUT_TK_CFG_PS                 (BIT(16))                                           /*!< Write enable bit for other configuration bits of KEYOUT_TK_CFG */

/* RTC_KEYOUT_SREL */
#define RTC_KEYOUT_SREL_SREL                 (BITS(0,7))                                         /*!< KEYOUT software release control bits */
#define RTC_KEYOUT_SREL_PS                   (BITS(8,15))                                        /*!< SREL field write enable bits */

/* constants definitions */
/* RTC_CTRL_FOUT */
#define RTC_CTRL_FOUT_POS                    (0U)                                                /*!< RTC_CTRL_FOUT bit offset */
#define RTC_CTRL_FOUT_CFG(regval)  (RTC_CTRL_FOUT & ((uint32_t)(regval) << RTC_CTRL_FOUT_POS))
#define RTC_CTRL_FOUT_NUL                    (RTC_CTRL_FOUT_CFG(0))                              /*!< RTCOUT forbidden */
#define RTC_CTRL_FOUT_PLLOUT                 (RTC_CTRL_FOUT_CFG(1))                              /*!< select RTCOUT as PLL, freq depend on PLLOUT_SEL */
#define RTC_CTRL_FOUT_1_30HZ                 (RTC_CTRL_FOUT_CFG(2))                              /*!< select RTCOUT as LOSC, frequency of 1/30Hz */
#define RTC_CTRL_FOUT_32KHZ                  (RTC_CTRL_FOUT_CFG(3))                              /*!< select RTCOUT as LOSC, frequency of 32768Hz */
#define RTC_CTRL_FOUT_16HZ                   (RTC_CTRL_FOUT_CFG(4))                              /*!< select RTCOUT as LOSC, frequency of 16Hz */
#define RTC_CTRL_FOUT_8HZ                    (RTC_CTRL_FOUT_CFG(5))                              /*!< select RTCOUT as LOSC, frequency of 8Hz */
#define RTC_CTRL_FOUT_4HZ                    (RTC_CTRL_FOUT_CFG(6))                              /*!< select RTCOUT as LOSC, frequency of 4Hz */
#define RTC_CTRL_FOUT_1HZ                    (RTC_CTRL_FOUT_CFG(7))                              /*!< select RTCOUT as LOSC, frequency of 1Hz */

/* RTC_CTRL_TCP */
#define RTC_CTRL_TCP_POS                     (3U)                                                /*!< RTC_CTRL_TCP bit offset */
#define RTC_CTRL_TCP_CFG(regval)  (RTC_CTRL_TCP & ((uint32_t)(regval) << RTC_CTRL_TCP_POS))
#define RTC_CTRL_TCP_2S                      (RTC_CTRL_TCP_CFG(0))                               /*!< select temperature compensation period as 2 second(or 0.5s for 8025T) */
#define RTC_CTRL_TCP_10S                     (RTC_CTRL_TCP_CFG(1))                               /*!< select temperature compensation period as 10 second(or 2s for 8025T) */
#define RTC_CTRL_TCP_20S                     (RTC_CTRL_TCP_CFG(2))                               /*!< select temperature compensation period as 20 second(or 10s for 8025T) */
#define RTC_CTRL_TCP_30S                     (RTC_CTRL_TCP_CFG(3))                               /*!< select temperature compensation period as 30 second(or 30s for 8025T) */
#define RTC_CTRL_TCP_1MIN                    (RTC_CTRL_TCP_CFG(4))                               /*!< select temperature compensation period as 1 minute(not for 8025T) */
#define RTC_CTRL_TCP_2MIN                    (RTC_CTRL_TCP_CFG(5))                               /*!< select temperature compensation period as 2 minute(not for 8025T) */
#define RTC_CTRL_TCP_5MIN                    (RTC_CTRL_TCP_CFG(6))                               /*!< select temperature compensation period as 5 minute(not for 8025T) */
#define RTC_CTRL_TCP_10MIN                   (RTC_CTRL_TCP_CFG(7))                               /*!< select temperature compensation period as 10 minute(not for 8025T) */

/* RTC_CTRL_TSE */
#define RTC_CTRL_TSE_POS                     (6U)                                                /*!< RTC_CTRL_TSE bit offset */
#define RTC_CTRL_TSE_CFG(regval)  (RTC_CTRL_TSE & ((uint32_t)(regval) << RTC_CTRL_TSE_POS))
#define RTC_CTRL_TSE_DIS                     (RTC_CTRL_TSE_CFG(0))                               /*!< disable automatic temperature compensation */
#define RTC_CTRL_TSE_AUTOEN                  (RTC_CTRL_TSE_CFG(1))                               /*!< enable automatic cyclic temperature compensation */
#define RTC_CTRL_TSE_USER0EN                 (RTC_CTRL_TSE_CFG(2))                               /*!< enable user mode 0, automatic temperature compensation triggered by setting temperature(temperature set by user) */
#define RTC_CTRL_TSE_USER1EN                 (RTC_CTRL_TSE_CFG(3))                               /*!< enable user mode 1, automatic temperature compensation triggered by setting temperature(temperature unchangeable) */

/* RTC_CTRL_WRTC */
#define RTC_CTRL_WRTC_POS                    (8U)                                                /*!< RTC_CTRL_WRTC bit offset */
#define RTC_CTRL_WRTC_DIS                    ((uint32_t)0x00000000U)                             /*!< RTC register set as read_only */
#define RTC_CTRL_WRTC_EN                     (RTC_CTRL_WRTC)                                     /*!< RTC register set as writable */

/* RTC_CTRL_WRBUSY */
#define RTC_CTRL_WRBUSY_POS                  (9U)                                                /*!< RTC_CTRL_WRBUSY bit offset */

/* RTC_CTRL_CALBUSY */
#define RTC_CTRL_CALBUSY_POS                 (10U)                                               /*!< RTC_CTRL_CALBUSY bit offset */

/* RTC_CTRL_STF_TRIG */
#define RTC_CTRL_STF_TRIG_POS                (12U)                                               /*!< RTC_CTRL_STF_TRIG bit offset */

/* RTC_SC_SC */
#define RTC_SC_SC_POS                        (0U)                                                /*!< RTC_SC_SC bit offset */

/* RTC_MN_MN */
#define RTC_MN_MN_POS                        (0U)                                                /*!< RTC_MN_MN bit offset */

/* RTC_HR_HR */
#define RTC_HR_HR_POS                        (0U)                                                /*!< RTC_HR_HR bit offset */

/* RTC_DT_DT */
#define RTC_DT_DT_POS                        (0U)                                                /*!< RTC_DT_DT bit offset */

/* RTC_MO_MO */
#define RTC_MO_MO_POS                        (0U)                                                /*!< RTC_MO_MO bit offset */

/* RTC_YR_YR */
#define RTC_YR_YR_POS                        (0U)                                                /*!< RTC_YR_YR bit offset */

/* RTC_DW_DW */
#define RTC_DW_DW_POS                        (0U)                                                /*!< RTC_DW_DW bit offset */

/* RTC_CNT1_CNT */
#define RTC_CNT1_CNT_POS                     (0U)                                                /*!< RTC_CNT1_CNT bit offset */

/* RTC_CNT1_PD */
#define RTC_CNT1_PD_POS                      (8U)                                                /*!< RTC_CNT1_PD bit offset */
#define RTC_CNT1_PD_ON                       ((uint32_t)0x00000000U)                             /*!< counter1 on (from perpetual calendar second interrupt) */
#define RTC_CNT1_PD_OFF                      (RTC_CNT1_PD)                                       /*!< counter1 off, reset the counter value and restart */

/* RTC_CNT1_DBGSTB */
#define RTC_CNT1_DBGSTB_POS                  (10U)                                               /*!< RTC_CNT1_DBGSTB bit offset */
#define RTC_CNT1_DBGSTB_PAUSE                (RTC_CNT1_DBGSTB)                                   /*!< halt counting when SoC halted by debugger */
#define RTC_CNT1_DBGSTB_COUNTINUE            ((uint32_t)0x00000000U)                             /*!< keep counting when SoC halted by debugger */

/* RTC_CNT2_CNT */
#define RTC_CNT2_CNT_POS                     (0U)                                                /*!< RTC_CNT2_CNT bit offset */

/* RTC_CNT2_PD */
#define RTC_CNT2_PD_POS                      (8U)                                                /*!< RTC_CNT2_PD bit offset */
#define RTC_CNT2_PD_ON                       ((uint32_t)0x00000000U)                             /*!< counter2 on(from internal 1/256 s interrupt) */
#define RTC_CNT2_PD_OFF                      (RTC_CNT2_PD)                                       /*!< counter2 off, reset the counter value */

/* RTC_CNT2_CLK_SEL */
#define RTC_CNT2_CLK_SEL_POS                 (9U)                                                /*!< RTC_CNT2_CLK_SEL bit offset */
#define RTC_CNT2_CLK_SEL_LOSC                ((uint32_t)0x00000000U)                             /*!< select LOSC as clock source of cnt2 */
#define RTC_CNT2_CLK_SEL_RCL                 (RTC_CNT2_CLK_SEL)                                  /*!< select RCL as clock source of cnt2 */

/* RTC_CNT2_DBGSTB */
#define RTC_CNT2_DBGSTB_POS                  (10U)                                               /*!< RTC_CNT2_DBGSTB bit offset */
#define RTC_CNT2_DBGSTB_PAUSE                (RTC_CNT2_DBGSTB)                                   /*!< halt counting when SoC halted by debugger */
#define RTC_CNT2_DBGSTB_COUNTINUE            ((uint32_t)0x00000000U)                             /*!< keep counting when SoC halted by debugger */

/* RTC_SCA_SCA */
#define RTC_SCA_SCA_POS                      (0U)                                                /*!< RTC_SCA_SCA bit offset */

/* RTC_MNA_MNA */
#define RTC_MNA_MNA_POS                      (0U)                                                /*!< RTC_MNA_MNA bit offset */

/* RTC_HRA_HRA */
#define RTC_HRA_HRA_POS                      (0U)                                                /*!< RTC_HRA_HRA bit offset */

/* RTC_IE_ALMIE */
#define RTC_IE_ALMIE_POS                     (0U)                                                /*!< RTC_IE_ALMIE bit offset */

/* RTC_IE_CNT1IE */
#define RTC_IE_CNT1IE_POS                    (1U)                                                /*!< RTC_IE_CNT1IE bit offset */

/* RTC_IE_CNT2IE */
#define RTC_IE_CNT2IE_POS                    (2U)                                                /*!< RTC_IE_CNT2IE bit offset */

/* RTC_IE_SCIE */
#define RTC_IE_SCIE_POS                      (3U)                                                /*!< RTC_IE_SCIE bit offset */

/* RTC_IE_MNIE */
#define RTC_IE_MNIE_POS                      (4U)                                                /*!< RTC_IE_MNIE bit offset */

/* RTC_IE_HRIE */
#define RTC_IE_HRIE_POS                      (5U)                                                /*!< RTC_IE_HRIE bit offset */

/* RTC_IE_DTIE */
#define RTC_IE_DTIE_POS                      (6U)                                                /*!< RTC_IE_DTIE bit offset */

/* RTC_IE_MOIE */
#define RTC_IE_MOIE_POS                      (7U)                                                /*!< RTC_IE_MOIE bit offset */

/* RTC_IE_IECLKEN */
#define RTC_IE_IECLKEN_POS                   (8U)                                                /*!< RTC_IE_IECLKEN bit offset */

/* RTC_IE_RTC_1S_SEL */
#define RTC_IE_RTC_1S_SEL_POS                (9U)                                                /*!< RTC_IE_RTC_1S_SEL bit offset */
#define RTC_IE_RTC_1S_SEL_SYSCLK             ((uint32_t)0x00000000U)                             /*!< select by system mode (hcmm pll_1hz, not hcmm RTC_1hz) */
#define RTC_IE_RTC_1S_SEL_RTC                (RTC_IE_RTC_1S_SEL)                                 /*!< select RTC_1hz */

/* RTC_IF_ALMIF */
#define RTC_IF_ALMIF_POS                     (0U)                                                /*!< RTC_IF_ALMIF bit offset */

/* RTC_IF_CNT1IF */
#define RTC_IF_CNT1IF_POS                    (1U)                                                /*!< RTC_IF_CNT1IF bit offset */

/* RTC_IF_CNT2IF */
#define RTC_IF_CNT2IF_POS                    (2U)                                                /*!< RTC_IF_CNT2IF bit offset */

/* RTC_IF_SCIF */
#define RTC_IF_SCIF_POS                      (3U)                                                /*!< RTC_IF_SCIF bit offset */

/* RTC_IF_MNIF */
#define RTC_IF_MNIF_POS                      (4U)                                                /*!< RTC_IF_MNIF bit offset */

/* RTC_IF_HRIF */
#define RTC_IF_HRIF_POS                      (5U)                                                /*!< RTC_IF_HRIF bit offset */

/* RTC_IF_DTIF */
#define RTC_IF_DTIF_POS                      (6U)                                                /*!< RTC_IF_DTIF bit offset */

/* RTC_IF_MOIF */
#define RTC_IF_MOIF_POS                      (7U)                                                /*!< RTC_IF_MOIF bit offset */

/* RTC_TEMP_TEMP */
#define RTC_TEMP_TEMP_POS                    (0U)                                                /*!< RTC_TEMP_TEMP bit offset */

/* RTC_PS_PS */
#define RTC_PS_PS_POS                        (0U)                                                /*!< RTC_PS_PS bit offset */
#define RTC_PS_PS_ENWT                       ((uint32_t)0x0000008EU)                             /*!< enable write of RTC competition registers */
#define RTC_PS_PS_DISWT                      ((uint32_t)0x00000000U)                             /*!< RTC competition registers write disable */

/* RTC_DOTA0_DOTA0 */
#define RTC_DOTA0_DOTA0_POS                  (0U)                                                /*!< RTC_DOTA0_DOTA0 bit offset */

/* RTC_ALPHAL_ALPHAL */
#define RTC_ALPHAL_ALPHAL_POS                (0U)                                                /*!< RTC_ALPHAL_ALPHAL bit offset */

/* RTC_ALPHAH_ALPHAH */
#define RTC_ALPHAH_ALPHAH_POS                (0U)                                                /*!< RTC_ALPHAH_ALPHAH bit offset */

/* RTC_XT0_XT0 */
#define RTC_XT0_XT0_POS                      (0U)                                                /*!< RTC_XT0_XT0 bit offset */

/* RTC_DOTAT_DOTAT */
#define RTC_DOTAT_DOTAT_POS                  (0U)                                                /*!< RTC_DOTAT_DOTAT bit offset */

/* RTC_LOSC_CFG1_LOSC_PD */
#define RTC_LOSC_CFG1_LOSC_PD_POS            (0U)                                                /*!< RTC_LOSC_CFG1_LOSC_PD bit offset */
#define RTC_LOSC_CFG1_LOSC_PD_ON             ((uint32_t)0x00000000U)                             /*!< enable LOSC */
#define RTC_LOSC_CFG1_LOSC_PD_OFF            ((uint32_t)0x000000A8U)                             /*!< disable LOSC */

/* RTC_LOSC_CFG1_LOSC_WEN */
#define RTC_LOSC_CFG1_LOSC_WEN_POS           (8U)                                                /*!< RTC_LOSC_CFG1_LOSC_WEN bit offset */
#define RTC_LOSC_CFG1_LOSC_WEN_DIS           ((uint32_t)0x00000000U)                             /*!< LOSC enable bits (RTC_LOSC_CFG1_PD) write disable */
#define RTC_LOSC_CFG1_LOSC_WEN_EN            (RTC_LOSC_CFG1_LOSC_WEN)                            /*!< LOSC enable bits (RTC_LOSC_CFG1_PD) write enable */

/* RTC_LOSC_CFG1_LOSC_FAIL */
#define RTC_LOSC_CFG1_LOSC_FAIL_POS          (9U)                                                /*!< RTC_LOSC_CFG1_LOSC_FAIL bit offset */
#define RTC_LOSC_CFG1_LOSC_FAIL_NORM         ((uint32_t)0x00000000U)                             /*!< LOSC run normally */
#define RTC_LOSC_CFG1_LOSC_FAIL_FAIL         (RTC_LOSC_CFG1_LOSC_FAIL)                           /*!< LOSC power down occured */

/* RTC_LOSC_CFG1_VCC_BOI_FLAG */
#define RTC_LOSC_CFG1_VCC_BOI_FLAG_POS       (10U)                                               /*!< RTC_LOSC_CFG1_VCC_BOI_FLAG bit offset */
#define RTC_LOSC_CFG1_VCC_BOI_FLAG_NORM      ((uint32_t)0x00000000U)                             /*!< VCC run normally */
#define RTC_LOSC_CFG1_VCC_BOI_FLAG_PD        (RTC_LOSC_CFG1_VCC_BOI_FLAG)                        /*!< VCC power down occured */

/* RTC_FPTR_FPTR */
#define RTC_FPTR_FPTR_POS                    (0U)                                                /*!< RTC_FPTR_FPTR bit offset */

/* RTC_FDTR1S_FDTR1S */
#define RTC_FDTR1S_FDTR1S_POS                (0U)                                                /*!< RTC_FDTR1S_FDTR1S bit offset */

/* RTC_FDTR30S_FDTR30S */
#define RTC_FDTR30S_FDTR30S_POS              (0U)                                                /*!< RTC_FDTR30S_FDTR30S bit offset */

/* RTC_FDTR120S_FDTR120S */
#define RTC_FDTR120S_FDTR120S_POS            (0U)                                                /*!< RTC_FDTR120S_FDTR120S bit offset */

/* RTC_IOEN_PL01EN */
#define RTC_IOEN_PL01EN_POS                  (0U)                                                /*!< RTC_IOEN_PL01EN bit offset */
#define RTC_IOEN_PL01EN_CFG(regval)  (RTC_IOEN_PL01EN & ((uint32_t)(regval) << RTC_IOEN_PL01EN_POS))
#define RTC_IOEN_PL01EN_IN                   (RTC_IOEN_PL01EN_CFG(0x5))                          /*!< enable pin as TAMPER1 input or KEY0[6] input */
#define RTC_IOEN_PL01EN_TAMPER1CLR           (RTC_IOEN_PL01EN_CFG(0xE))                          /*!< clean registers result of offset address 0x90-0xBC(TAMPER1) */
#define RTC_IOEN_PL01EN_KEYOUT               (RTC_IOEN_PL01EN_CFG(0xB))                          /*!< select as KEYOUT output pin */
#define RTC_IOEN_PL01EN_NUL                  (RTC_IOEN_PL01EN_CFG(0))                            /*!< Not function for RTC */

/* RTC_IOEN_PL00EN */
#define RTC_IOEN_PL00EN_POS                  (4U)                                                /*!< RTC_IOEN_PL00EN bit offset */
#define RTC_IOEN_PL00EN_CFG(regval)  (RTC_IOEN_PL00EN & ((uint32_t)(regval) << RTC_IOEN_PL00EN_POS))
#define RTC_IOEN_PL00EN_RTCOUT               (RTC_IOEN_PL00EN_CFG(0xA))                          /*!< enable pin as  RTC_OUT */
#define RTC_IOEN_PL00EN_KEYOUT               (RTC_IOEN_PL00EN_CFG(0xB))                          /*!< select as KEYOUT output pin */
#define RTC_IOEN_PL00EN_NUL                  (RTC_IOEN_PL00EN_CFG(0))                            /*!< Not function for RTC */

/* RTC_IOEN_PL02EN */
#define RTC_IOEN_PL02EN_POS                  (8U)                                                /*!< RTC_IOEN_PL02EN bit offset */
#define RTC_IOEN_PL02EN_CFG(regval)  (RTC_IOEN_PL02EN & ((uint32_t)(regval) << RTC_IOEN_PL02EN_POS))
#define RTC_IOEN_PL02EN_IN                   (RTC_IOEN_PL02EN_CFG(0x5))                          /*!< enable pin as TAMPER2 input or KEY0[7] input */
#define RTC_IOEN_PL02EN_TAMPER2CLR           (RTC_IOEN_PL02EN_CFG(0xE))                          /*!< clean registers result of offset address 0x100~ 0x12C(TAMPER2) */
#define RTC_IOEN_PL02EN_KEYOUT               (RTC_IOEN_PL02EN_CFG(0xB))                          /*!< select as KEYOUT output pin */
#define RTC_IOEN_PL02EN_NUL                  (RTC_IOEN_PL02EN_CFG(0))                            /*!< Not function for RTC */

/* RTC_IOEN_PL03EN */
#define RTC_IOEN_PL03EN_POS                  (12U)                                               /*!< RTC_IOEN_PL03EN bit offset */
#define RTC_IOEN_PL03EN_KEYOUT               ((uint32_t)0x0000B000U)                             /*!< select as KEYOUT output pin */
#define RTC_IOEN_PL03EN_NUL                  ((uint32_t)0x00000000U)                             /*!< Not function for RTC */

/* RTC_IOMODE_TAMPER1_MODE */
#define RTC_IOMODE_TAMPER1_MODE_POS          (0U)                                                /*!< RTC_IOMODE_TAMPER1_MODE bit offset */
#define RTC_IOMODE_TAMPER1_MODE_EST          ((uint32_t)0x00000000U)                             /*!< record the two earliest press key event */
#define RTC_IOMODE_TAMPER1_MODE_LST          (RTC_IOMODE_TAMPER1_MODE)                           /*!< record the two lastest press key event */

/* RTC_IOMODE_TAMPER2_MODE */
#define RTC_IOMODE_TAMPER2_MODE_POS          (1U)                                                /*!< RTC_IOMODE_TAMPER2_MODE bit offset */
#define RTC_IOMODE_TAMPER2_MODE_EST          ((uint32_t)0x00000000U)                             /*!< record the two earliest press key event */
#define RTC_IOMODE_TAMPER2_MODE_LST          (RTC_IOMODE_TAMPER2_MODE)                           /*!< record the two lastest press key event */

/* RTC_IOMODE_KEYOUT_MODE */
#define RTC_IOMODE_KEYOUT_MODE_POS           (2U)                                                /*!< RTC_IOMODE_KEYOUT_MODE bit offset */
#define RTC_IOMODE_KEYOUT_MODE_HIZ           ((uint32_t)0x00000000U)                             /*!< Outputs low level when triggered; high-Z when released */
#define RTC_IOMODE_KEYOUT_MODE_HL            (RTC_IOMODE_KEYOUT_MODE)                            /*!< Outputs low level when triggered; high level when released */

/* RTC_IOMODE_PADEN_PS */
#define RTC_IOMODE_PADEN_PS_POS              (4U)                                                /*!< RTC_IOMODE_PADEN_PS bit offset */
#define RTC_IOMODE_PADEN_PS_WEN              ((uint32_t)0x000000B0U)                             /*!< RTC_IOEN write enable */
#define RTC_IOMODE_PADEN_PS_WDIS             ((uint32_t)0x00000000U)                             /*!< RTC_IOEN write disable */

/* RTC_IOMODE_TAMPER_FLT_CNT */
#define RTC_IOMODE_TAMPER_FLT_CNT_POS        (8U)                                                /*!< RTC_IOMODE_TAMPER_FLT_CNT bit offset */

/* RTC_IOMODE_TAMPER_FLT_TS */
#define RTC_IOMODE_TAMPER_FLT_TS_POS         (13U)                                               /*!< RTC_IOMODE_TAMPER_FLT_TS bit offset */
#define RTC_IOMODE_TAMPER_FLT_TS_2           ((uint32_t)0x00000000U)                             /*!< Select 2ms filter precision */
#define RTC_IOMODE_TAMPER_FLT_TS_250         (RTC_IOMODE_TAMPER_FLT_TS)                          /*!< Select 250ms filter precision */

/* RTC_IOMODE_TAMPER_FLT_PS */
#define RTC_IOMODE_TAMPER_FLT_PS_POS         (14U)                                               /*!< RTC_IOMODE_TAMPER_FLT_PS bit offset */

/* RTC_TAMPER1N0_TIME0_SEC */
#define RTC_TAMPER1N0_TIME0_SEC_POS          (0U)                                                /*!< RTC_TAMPER1N0_TIME0_SEC bit offset */

/* RTC_TAMPER1N0_TIME0_MIN */
#define RTC_TAMPER1N0_TIME0_MIN_POS          (8U)                                                /*!< RTC_TAMPER1N0_TIME0_MIN bit offset */

/* RTC_TAMPER1N0_TIME1_HOUR */
#define RTC_TAMPER1N0_TIME1_HOUR_POS         (0U)                                                /*!< RTC_TAMPER1N0_TIME1_HOUR bit offset */

/* RTC_TAMPER1N0_TIME1_DAY */
#define RTC_TAMPER1N0_TIME1_DAY_POS          (8U)                                                /*!< RTC_TAMPER1N0_TIME1_DAY bit offset */

/* RTC_TAMPER1N0_TIME2_MT */
#define RTC_TAMPER1N0_TIME2_MT_POS           (0U)                                                /*!< RTC_TAMPER1N0_TIME2_MT bit offset */

/* RTC_TAMPER1N0_TIME2_YR */
#define RTC_TAMPER1N0_TIME2_YR_POS           (8U)                                                /*!< RTC_TAMPER1N0_TIME2_YR bit offset */

/* RTC_TAMPER1P0_TIME0_SEC */
#define RTC_TAMPER1P0_TIME0_SEC_POS          (0U)                                                /*!< RTC_TAMPER1P0_TIME0_SEC bit offset */

/* RTC_TAMPER1P0_TIME0_MIN */
#define RTC_TAMPER1P0_TIME0_MIN_POS          (8U)                                                /*!< RTC_TAMPER1P0_TIME0_MIN bit offset */

/* RTC_TAMPER1P0_TIME1_HOUR */
#define RTC_TAMPER1P0_TIME1_HOUR_POS         (0U)                                                /*!< RTC_TAMPER1P0_TIME1_HOUR bit offset */

/* RTC_TAMPER1P0_TIME1_DAY */
#define RTC_TAMPER1P0_TIME1_DAY_POS          (8U)                                                /*!< RTC_TAMPER1P0_TIME1_DAY bit offset */

/* RTC_TAMPER1P0_TIME2_MT */
#define RTC_TAMPER1P0_TIME2_MT_POS           (0U)                                                /*!< RTC_TAMPER1P0_TIME2_MT bit offset */

/* RTC_TAMPER1P0_TIME2_YR */
#define RTC_TAMPER1P0_TIME2_YR_POS           (8U)                                                /*!< RTC_TAMPER1P0_TIME2_YR bit offset */

/* RTC_TAMPER1N1_TIME0_SEC */
#define RTC_TAMPER1N1_TIME0_SEC_POS          (0U)                                                /*!< RTC_TAMPER1N1_TIME0_SEC bit offset */

/* RTC_TAMPER1N1_TIME0_MIN */
#define RTC_TAMPER1N1_TIME0_MIN_POS          (8U)                                                /*!< RTC_TAMPER1N1_TIME0_MIN bit offset */

/* RTC_TAMPER1N1_TIME1_HOUR */
#define RTC_TAMPER1N1_TIME1_HOUR_POS         (0U)                                                /*!< RTC_TAMPER1N1_TIME1_HOUR bit offset */

/* RTC_TAMPER1N1_TIME1_DAY */
#define RTC_TAMPER1N1_TIME1_DAY_POS          (8U)                                                /*!< RTC_TAMPER1N1_TIME1_DAY bit offset */

/* RTC_TAMPER1N1_TIME2_MT */
#define RTC_TAMPER1N1_TIME2_MT_POS           (0U)                                                /*!< RTC_TAMPER1N1_TIME2_MT bit offset */

/* RTC_TAMPER1N1_TIME2_YR */
#define RTC_TAMPER1N1_TIME2_YR_POS           (8U)                                                /*!< RTC_TAMPER1N1_TIME2_YR bit offset */

/* RTC_TAMPER1P1_TIME0_SEC */
#define RTC_TAMPER1P1_TIME0_SEC_POS          (0U)                                                /*!< RTC_TAMPER1P1_TIME0_SEC bit offset */

/* RTC_TAMPER1P1_TIME0_MIN */
#define RTC_TAMPER1P1_TIME0_MIN_POS          (8U)                                                /*!< RTC_TAMPER1P1_TIME0_MIN bit offset */

/* RTC_TAMPER1P1_TIME1_HOUR */
#define RTC_TAMPER1P1_TIME1_HOUR_POS         (0U)                                                /*!< RTC_TAMPER1P1_TIME1_HOUR bit offset */

/* RTC_TAMPER1P1_TIME1_DAY */
#define RTC_TAMPER1P1_TIME1_DAY_POS          (8U)                                                /*!< RTC_TAMPER1P1_TIME1_DAY bit offset */

/* RTC_TAMPER1P1_TIME2_MT */
#define RTC_TAMPER1P1_TIME2_MT_POS           (0U)                                                /*!< RTC_TAMPER1P1_TIME2_MT bit offset */

/* RTC_TAMPER1P1_TIME2_YR */
#define RTC_TAMPER1P1_TIME2_YR_POS           (8U)                                                /*!< RTC_TAMPER1P1_TIME2_YR bit offset */

/* RTC_TEMPOS_TEMPOS */
#define RTC_TEMPOS_TEMPOS_POS                (0U)                                                /*!< RTC_TEMPOS_TEMPOS bit offset */

/* RTC_TPSIN_TPSIN */
#define RTC_TPSIN_TPSIN_POS                  (0U)                                                /*!< RTC_TPSIN_TPSIN bit offset */

/* RTC_CALPS_CALPS */
#define RTC_CALPS_CALPS_POS                  (0U)                                                /*!< RTC_CALPS_CALPS bit offset */
#define RTC_CALPS_CALPS_ENABLE               ((uint32_t)0x000000A8U)                             /*!< enable RTC secondary calibration */
#define RTC_CALPS_CALPS_DISABLE              ((uint32_t)0x00000000U)                             /*!< RTC secondary calibration disable */

/* RTC_CAL_T0_CAL_T0 */
#define RTC_CAL_T0_CAL_T0_POS                (0U)                                                /*!< RTC_CAL_T0_CAL_T0 bit offset */

/* RTC_CAL_T1_CAL_T1 */
#define RTC_CAL_T1_CAL_T1_POS                (0U)                                                /*!< RTC_CAL_T1_CAL_T1 bit offset */

/* RTC_CAL_T2_CAL_T2 */
#define RTC_CAL_T2_CAL_T2_POS                (0U)                                                /*!< RTC_CAL_T2_CAL_T2 bit offset */

/* RTC_CAL_T3_CAL_T3 */
#define RTC_CAL_T3_CAL_T3_POS                (0U)                                                /*!< RTC_CAL_T3_CAL_T3 bit offset */

/* RTC_CAL_T4_CAL_T4 */
#define RTC_CAL_T4_CAL_T4_POS                (0U)                                                /*!< RTC_CAL_T4_CAL_T4 bit offset */

/* RTC_CAL_T5_CAL_T5 */
#define RTC_CAL_T5_CAL_T5_POS                (0U)                                                /*!< RTC_CAL_T5_CAL_T5 bit offset */

/* RTC_CAL_T6_CAL_T6 */
#define RTC_CAL_T6_CAL_T6_POS                (0U)                                                /*!< RTC_CAL_T6_CAL_T6 bit offset */

/* RTC_CAL_T7_CAL_T7 */
#define RTC_CAL_T7_CAL_T7_POS                (0U)                                                /*!< RTC_CAL_T7_CAL_T7 bit offset */

/* RTC_CAL_T8_CAL_T8 */
#define RTC_CAL_T8_CAL_T8_POS                (0U)                                                /*!< RTC_CAL_T8_CAL_T8 bit offset */

/* RTC_CAL_T9_CAL_T9 */
#define RTC_CAL_T9_CAL_T9_POS                (0U)                                                /*!< RTC_CAL_T9_CAL_T9 bit offset */

/* RTC_TEMP2_TEMP2 */
#define RTC_TEMP2_TEMP2_POS                  (0U)                                                /*!< RTC_TEMP2_TEMP2 bit offset */

/* RTC_TAMPER2N0_TIME0_SEC */
#define RTC_TAMPER2N0_TIME0_SEC_POS          (0U)                                                /*!< RTC_TAMPER2N0_TIME0_SEC bit offset */

/* RTC_TAMPER2N0_TIME0_MIN */
#define RTC_TAMPER2N0_TIME0_MIN_POS          (8U)                                                /*!< RTC_TAMPER2N0_TIME0_MIN bit offset */

/* RTC_TAMPER2N0_TIME1_HOUR */
#define RTC_TAMPER2N0_TIME1_HOUR_POS         (0U)                                                /*!< RTC_TAMPER2N0_TIME1_HOUR bit offset */

/* RTC_TAMPER2N0_TIME1_DAY */
#define RTC_TAMPER2N0_TIME1_DAY_POS          (8U)                                                /*!< RTC_TAMPER2N0_TIME1_DAY bit offset */

/* RTC_TAMPER2N0_TIME2_MT */
#define RTC_TAMPER2N0_TIME2_MT_POS           (0U)                                                /*!< RTC_TAMPER2N0_TIME2_MT bit offset */

/* RTC_TAMPER2N0_TIME2_YR */
#define RTC_TAMPER2N0_TIME2_YR_POS           (8U)                                                /*!< RTC_TAMPER2N0_TIME2_YR bit offset */

/* RTC_TAMPER2P0_TIME0_SEC */
#define RTC_TAMPER2P0_TIME0_SEC_POS          (0U)                                                /*!< RTC_TAMPER2P0_TIME0_SEC bit offset */

/* RTC_TAMPER2P0_TIME0_MIN */
#define RTC_TAMPER2P0_TIME0_MIN_POS          (8U)                                                /*!< RTC_TAMPER2P0_TIME0_MIN bit offset */

/* RTC_TAMPER2P0_TIME1_HOUR */
#define RTC_TAMPER2P0_TIME1_HOUR_POS         (0U)                                                /*!< RTC_TAMPER2P0_TIME1_HOUR bit offset */

/* RTC_TAMPER2P0_TIME1_DAY */
#define RTC_TAMPER2P0_TIME1_DAY_POS          (8U)                                                /*!< RTC_TAMPER2P0_TIME1_DAY bit offset */

/* RTC_TAMPER2P0_TIME2_MT */
#define RTC_TAMPER2P0_TIME2_MT_POS           (0U)                                                /*!< RTC_TAMPER2P0_TIME2_MT bit offset */

/* RTC_TAMPER2P0_TIME2_YR */
#define RTC_TAMPER2P0_TIME2_YR_POS           (8U)                                                /*!< RTC_TAMPER2P0_TIME2_YR bit offset */

/* RTC_TAMPER2N1_TIME0_SEC */
#define RTC_TAMPER2N1_TIME0_SEC_POS          (0U)                                                /*!< RTC_TAMPER2N1_TIME0_SEC bit offset */

/* RTC_TAMPER2N1_TIME0_MIN */
#define RTC_TAMPER2N1_TIME0_MIN_POS          (8U)                                                /*!< RTC_TAMPER2N1_TIME0_MIN bit offset */

/* RTC_TAMPER2N1_TIME1_HOUR */
#define RTC_TAMPER2N1_TIME1_HOUR_POS         (0U)                                                /*!< RTC_TAMPER2N1_TIME1_HOUR bit offset */

/* RTC_TAMPER2N1_TIME1_DAY */
#define RTC_TAMPER2N1_TIME1_DAY_POS          (8U)                                                /*!< RTC_TAMPER2N1_TIME1_DAY bit offset */

/* RTC_TAMPER2N1_TIME2_MT */
#define RTC_TAMPER2N1_TIME2_MT_POS           (0U)                                                /*!< RTC_TAMPER2N1_TIME2_MT bit offset */

/* RTC_TAMPER2N1_TIME2_YR */
#define RTC_TAMPER2N1_TIME2_YR_POS           (8U)                                                /*!< RTC_TAMPER2N1_TIME2_YR bit offset */

/* RTC_TAMPER2P1_TIME0_SEC */
#define RTC_TAMPER2P1_TIME0_SEC_POS          (0U)                                                /*!< RTC_TAMPER2P1_TIME0_SEC bit offset */

/* RTC_TAMPER2P1_TIME0_MIN */
#define RTC_TAMPER2P1_TIME0_MIN_POS          (8U)                                                /*!< RTC_TAMPER2P1_TIME0_MIN bit offset */

/* RTC_TAMPER2P1_TIME1_HOUR */
#define RTC_TAMPER2P1_TIME1_HOUR_POS         (0U)                                                /*!< RTC_TAMPER2P1_TIME1_HOUR bit offset */

/* RTC_TAMPER2P1_TIME1_DAY */
#define RTC_TAMPER2P1_TIME1_DAY_POS          (8U)                                                /*!< RTC_TAMPER2P1_TIME1_DAY bit offset */

/* RTC_TAMPER2P1_TIME2_MT */
#define RTC_TAMPER2P1_TIME2_MT_POS           (0U)                                                /*!< RTC_TAMPER2P1_TIME2_MT bit offset */

/* RTC_TAMPER2P1_TIME2_YR */
#define RTC_TAMPER2P1_TIME2_YR_POS           (8U)                                                /*!< RTC_TAMPER2P1_TIME2_YR bit offset */

/* RTC_MODE1_CURVE_SEL */
#define RTC_MODE1_CURVE_SEL_POS              (0U)                                                /*!< RTC_MODE1_CURVE_SEL bit offset */

/* RTC_XT1_XT1 */
#define RTC_XT1_XT1_POS                      (0U)                                                /*!< RTC_XT1_XT1 bit offset */

/* RTC_ALPHA_ALPHA */
#define RTC_ALPHA_ALPHA_POS                  (0U)                                                /*!< RTC_ALPHA_ALPHA bit offset */

/* RTC_BETA_BETA */
#define RTC_BETA_BETA_POS                    (0U)                                                /*!< RTC_BETA_BETA bit offset */

/* RTC_GAMMA_GAMMA */
#define RTC_GAMMA_GAMMA_POS                  (0U)                                                /*!< RTC_GAMMA_GAMMA bit offset */

/* RTC_ZETA_ZETA */
#define RTC_ZETA_ZETA_POS                    (0U)                                                /*!< RTC_ZETA_ZETA bit offset */

/* RTC_ZETAH_ZETAH */
#define RTC_ZETAH_ZETAH_POS                  (0U)                                                /*!< RTC_ZETAH_ZETAH bit offset */

/* RTC_TPS_START_TPS_START */
#define RTC_TPS_START_TPS_START_POS          (0U)                                                /*!< RTC_TPS_START_TPS_START bit offset */

/* RTC_TEMP_CAL_TEMP_CAL */
#define RTC_TEMP_CAL_TEMP_CAL_POS            (0U)                                                /*!< RTC_TEMP_CAL_TEMP_CAL bit offset */

/* RTC_SSC_SSC */
#define RTC_SSC_SSC_POS                      (0U)                                                /*!< RTC_SSC_SSC bit offset */

/* RTC_TAMPERIE_TAMPER1PIE */
#define RTC_TAMPERIE_TAMPER1PIE_POS          (0U)                                                /*!< RTC_TAMPERIE_TAMPER1PIE bit offset */

/* RTC_TAMPERIE_TAMPER1NIE */
#define RTC_TAMPERIE_TAMPER1NIE_POS          (1U)                                                /*!< RTC_TAMPERIE_TAMPER1NIE bit offset */

/* RTC_TAMPERIE_TAMPER2PIE */
#define RTC_TAMPERIE_TAMPER2PIE_POS          (2U)                                                /*!< RTC_TAMPERIE_TAMPER2PIE bit offset */

/* RTC_TAMPERIE_TAMPER2NIE */
#define RTC_TAMPERIE_TAMPER2NIE_POS          (3U)                                                /*!< RTC_TAMPERIE_TAMPER2NIE bit offset */

/* RTC_TAMPERIE_STFIE */
#define RTC_TAMPERIE_STFIE_POS               (4U)                                                /*!< RTC_TAMPERIE_STFIE bit offset */

/* RTC_TAMPERIE_KEYOTKIE */
#define RTC_TAMPERIE_KEYOTKIE_POS            (5U)                                                /*!< RTC_TAMPERIE_KEYOTKIE bit offset */

/* RTC_TAMPERIF_TAMPER1PIF */
#define RTC_TAMPERIF_TAMPER1PIF_POS          (0U)                                                /*!< RTC_TAMPERIF_TAMPER1PIF bit offset */

/* RTC_TAMPERIF_TAMPER1NIF */
#define RTC_TAMPERIF_TAMPER1NIF_POS          (1U)                                                /*!< RTC_TAMPERIF_TAMPER1NIF bit offset */

/* RTC_TAMPERIF_TAMPER2PIF */
#define RTC_TAMPERIF_TAMPER2PIF_POS          (2U)                                                /*!< RTC_TAMPERIF_TAMPER2PIF bit offset */

/* RTC_TAMPERIF_TAMPER2NIF */
#define RTC_TAMPERIF_TAMPER2NIF_POS          (3U)                                                /*!< RTC_TAMPERIF_TAMPER2NIF bit offset */

/* RTC_TAMPERIF_STFIF */
#define RTC_TAMPERIF_STFIF_POS               (4U)                                                /*!< RTC_TAMPERIF_STFIF bit offset */

/* RTC_TAMPERIF_KEYOTKIF */
#define RTC_TAMPERIF_KEYOTKIF_POS            (5U)                                                /*!< RTC_TAMPERIF_KEYOTKIF bit offset */

/* RTC_PLLOUT_CFG_FREQ_SEL */
#define RTC_PLLOUT_CFG_FREQ_SEL_POS          (0U)                                                /*!< RTC_PLLOUT_CFG_FREQ_SEL bit offset */
#define RTC_PLLOUT_CFG_FREQ_SEL_CFG(regval)  (RTC_PLLOUT_CFG_FREQ_SEL & ((uint32_t)(regval) << RTC_PLLOUT_CFG_FREQ_SEL_POS))
#define RTC_PLLOUT_CFG_FREQ_SEL_1HZ          (RTC_PLLOUT_CFG_FREQ_SEL_CFG(0))                    /*!< config pll output as 1HZ */
#define RTC_PLLOUT_CFG_FREQ_SEL_2HZ          (RTC_PLLOUT_CFG_FREQ_SEL_CFG(1))                    /*!< config pll output as 2HZ */
#define RTC_PLLOUT_CFG_FREQ_SEL_4HZ          (RTC_PLLOUT_CFG_FREQ_SEL_CFG(2))                    /*!< config pll output as 4HZ */
#define RTC_PLLOUT_CFG_FREQ_SEL_8HZ          (RTC_PLLOUT_CFG_FREQ_SEL_CFG(3))                    /*!< config pll output as 8HZ */
#define RTC_PLLOUT_CFG_FREQ_SEL_16HZ         (RTC_PLLOUT_CFG_FREQ_SEL_CFG(4))                    /*!< config pll output as 16HZ */

/* RTC_BKPPS_PS */
#define RTC_BKPPS_PS_POS                     (0U)                                                /*!< RTC_BKPPS_PS bit offset */
#define RTC_BKPPS_PS_ENWT                    ((uint32_t)0x000000FAU)                             /*!< enable write of RTC user defines registers */
#define RTC_BKPPS_PS_DISWT                   ((uint32_t)0x00000000U)                             /*!< RTC user defines registers write disable */

/* RTC_BKP0_BKP */
#define RTC_BKP0_BKP_POS                     (0U)                                                /*!< RTC_BKP0_BKP bit offset */

/* RTC_BKP1_BKP */
#define RTC_BKP1_BKP_POS                     (0U)                                                /*!< RTC_BKP1_BKP bit offset */

/* RTC_BKP2_BKP */
#define RTC_BKP2_BKP_POS                     (0U)                                                /*!< RTC_BKP2_BKP bit offset */

/* RTC_BKP3_BKP */
#define RTC_BKP3_BKP_POS                     (0U)                                                /*!< RTC_BKP3_BKP bit offset */

/* RTC_BKP4_BKP */
#define RTC_BKP4_BKP_POS                     (0U)                                                /*!< RTC_BKP4_BKP bit offset */

/* RTC_BKP5_BKP */
#define RTC_BKP5_BKP_POS                     (0U)                                                /*!< RTC_BKP5_BKP bit offset */

/* RTC_BKP6_BKP */
#define RTC_BKP6_BKP_POS                     (0U)                                                /*!< RTC_BKP6_BKP bit offset */

/* RTC_BKP7_BKP */
#define RTC_BKP7_BKP_POS                     (0U)                                                /*!< RTC_BKP7_BKP bit offset */

/* RTC_BKP8_BKP */
#define RTC_BKP8_BKP_POS                     (0U)                                                /*!< RTC_BKP8_BKP bit offset */

/* RTC_BKP9_BKP */
#define RTC_BKP9_BKP_POS                     (0U)                                                /*!< RTC_BKP9_BKP bit offset */

/* RTC_BKP10_BKP */
#define RTC_BKP10_BKP_POS                    (0U)                                                /*!< RTC_BKP10_BKP bit offset */

/* RTC_BKP11_BKP */
#define RTC_BKP11_BKP_POS                    (0U)                                                /*!< RTC_BKP11_BKP bit offset */

/* RTC_BKP12_BKP */
#define RTC_BKP12_BKP_POS                    (0U)                                                /*!< RTC_BKP12_BKP bit offset */

/* RTC_BKP13_BKP */
#define RTC_BKP13_BKP_POS                    (0U)                                                /*!< RTC_BKP13_BKP bit offset */

/* RTC_BKP14_BKP */
#define RTC_BKP14_BKP_POS                    (0U)                                                /*!< RTC_BKP14_BKP bit offset */

/* RTC_BKP15_BKP */
#define RTC_BKP15_BKP_POS                    (0U)                                                /*!< RTC_BKP15_BKP bit offset */

/* RTC_KEYOUT_TK_CFG_EN */
#define RTC_KEYOUT_TK_CFG_EN_POS             (0U)                                                /*!< RTC_KEYOUT_TK_CFG_EN bit offset */

/* RTC_KEYOUT_TK_CFG_TB */
#define RTC_KEYOUT_TK_CFG_TB_POS             (8U)                                                /*!< RTC_KEYOUT_TK_CFG_TB bit offset */
#define RTC_KEYOUT_TK_CFG_TB_1MIN            ((uint32_t)0x00000000U)                             /*!< select as 1 minute */
#define RTC_KEYOUT_TK_CFG_TB_1S              (RTC_KEYOUT_TK_CFG_TB)                              /*!< select as 1 second */

/* RTC_KEYOUT_TK_CFG_CNT */
#define RTC_KEYOUT_TK_CFG_CNT_POS            (9U)                                                /*!< RTC_KEYOUT_TK_CFG_CNT bit offset */

/* RTC_KEYOUT_TK_CFG_PS */
#define RTC_KEYOUT_TK_CFG_PS_POS             (16U)                                               /*!< RTC_KEYOUT_TK_CFG_PS bit offset */

/* RTC_KEYOUT_SREL_SREL */
#define RTC_KEYOUT_SREL_SREL_POS             (0U)                                                /*!< RTC_KEYOUT_SREL_SREL bit offset */
#define RTC_KEYOUT_SREL_SREL_RELEASE         ((uint32_t)0x00000000U)                             /*!< release KEYOUT */
#define RTC_KEYOUT_SREL_SREL_NUL             ((uint32_t)0x00000000U)                             /*!< no use */

/* RTC_KEYOUT_SREL_PS */
#define RTC_KEYOUT_SREL_PS_POS               (8U)                                                /*!< RTC_KEYOUT_SREL_PS bit offset */
#define RTC_KEYOUT_SREL_PS_ENWT              ((uint32_t)0x0000EC00U)                             /*!< enable write SREL */
#define RTC_KEYOUT_SREL_PS_DISWT             ((uint32_t)0x00000000U)                             /*!< disable write SREL */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_RTC_H */
