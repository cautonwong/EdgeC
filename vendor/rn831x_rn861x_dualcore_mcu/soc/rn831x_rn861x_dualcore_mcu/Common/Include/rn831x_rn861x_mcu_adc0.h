/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_adc0.h
 * @author  Renergy Software Team
 * @brief   ADC0 module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_ADC0_H
#define _RN831x_RN861x_MCU_ADC0_H

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

/* ADC0 */
typedef struct
{
    __IO uint32_t CFG;                       /*!< offset: 0x000 info: Configuration Register */
    __IO uint32_t OVSCFG;                    /*!< offset: 0x004 info: Oversampling Configuration Register */
    __IO uint32_t OVSCHSEL;                  /*!< offset: 0x008 info: Oversampling Channel Configuration Register.Select whether each channel is oversampled.(Only can be configured when ADC0_STARTA=0 and ADC0_STARTB =0.) */
    __IO uint32_t PWDCTRL;                   /*!< offset: 0x00C info: Power Switch Control Register */
    __IO uint32_t SAMPT0;                    /*!< offset: 0x010 info: Sampling Time Configuration Register for Input Channels 0~3 */
    __IO uint32_t SAMPT1;                    /*!< offset: 0x014 info: Sampling Time Configuration Register for Input Channels 4~7 */
    __IO uint32_t SAMPT2;                    /*!< offset: 0x018 info: Sampling Time Configuration Register for Input Channels 8~11 */
    __IO uint32_t SAMPT3;                    /*!< offset: 0x01C info: Sampling Time Configuration Register for Input Channels 12~13 */
    __IO uint32_t CTRLA;                     /*!< offset: 0x020 info: Sequence A Control Register */
    __IO uint32_t CHSELA;                    /*!< offset: 0x024 info: Sequence A Channel Selection Register */
    __IO uint32_t STARTA;                    /*!< offset: 0x028 info: Sequence A Start Register */
    __IO uint32_t DATA;                      /*!< offset: 0x02C info: Sequence A Data Register */
    __IO uint32_t AWDHLA;                    /*!< offset: 0x030 info: Sequence A Watchdog Threshold Register */
    __IO uint32_t WCHSELA;                   /*!< offset: 0x034 info: Sequence A Watchdog Channel Selection Register */
    __IO uint32_t DMA_RBADRA;                /*!< offset: 0x038 info: Sequence A DMA Receive Base Address Register */
    __IO uint32_t DMA_RLENA;                 /*!< offset: 0x03C info: Sequence A DMA Receive Length Register */
    __IO uint32_t DMA_RADRA;                 /*!< offset: 0x040 info: Sequence A DMA Current Address Register */
    __IO uint32_t IEA;                       /*!< offset: 0x044 info: Sequence A Interrupt Enable Register */
    __IO uint32_t IFA;                       /*!< offset: 0x048 info: Sequence A Interrupt Flag Register */
    __IO uint32_t RESERVED0;                 /*!< offset: 0x04C */
    __IO uint32_t CTRLB;                     /*!< offset: 0x050 info: Sequence B Control Register */
    __IO uint32_t CHSELB;                    /*!< offset: 0x054 info: Sequence B Channel Selection Register */
    __IO uint32_t STARTB;                    /*!< offset: 0x058 info: Sequence B Start Register */
    __IO uint32_t DATB;                      /*!< offset: 0x05C info: Sequence B Data Register */
    __IO uint32_t AWDHLB;                    /*!< offset: 0x060 info: Sequence B Watchdog Threshold Register */
    __IO uint32_t WCHSELB;                   /*!< offset: 0x064 info: Sequence B Watchdog Channel Selection Register */
    __IO uint32_t DMA_RBADRB;                /*!< offset: 0x068 info: Sequence B DMA Receive Base Address Register */
    __IO uint32_t DMA_RLENB;                 /*!< offset: 0x06C info: Sequence B DMA Receive Length Register */
    __IO uint32_t DMA_RADRB;                 /*!< offset: 0x070 info: Sequence B DMA Current Address Register */
    __IO uint32_t IEB;                       /*!< offset: 0x074 info: Sequence B Interrupt Enable Register */
    __IO uint32_t IFB;                       /*!< offset: 0x078 info: Sequence B Interrupt Flag Register */
    __IO uint32_t RESERVED1;                 /*!< offset: 0x07C */
    __IO uint32_t CAL;                       /*!< offset: 0x080 info: Calibration config Register */
    __IO uint32_t DOS;                       /*!< offset: 0x084 info: Digital Offset Code Value Configuration Register */
    __IO uint32_t BGRLP;                     /*!< offset: 0x088 info: BGRLP Channel Ideal Code Value Register */
    __IO uint32_t RESERVED2[2];              /*!< offset: 0x08C ~ 090 */
    __IO uint32_t DCONV;                     /*!< offset: 0x094 info: Dummy Conversion Configuration Register */
} ADC0_TypeDef;

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
#define ADC0_CORE0_BASE (0x20094000U)
#define ADC0_CORE1_BASE (0xA0094000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define ADC0_CORE0 ((ADC0_TypeDef *)     ADC0_CORE0_BASE)
#define ADC0_CORE1 ((ADC0_TypeDef *)     ADC0_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define ADC0  ADC0_CORE0
#endif
#if defined(CORE1)
#define ADC0  ADC0_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** ADC0 Instances *******************************/
#define IS_ADC0_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == ADC0_CORE0) || \
                                           ((INSTANCE) == ADC0_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* ADC0_CFG */
#define ADC0_CFG_RESC                        (BITS(0,1))                                         /*!< ADC resolution selection bits (Only can be configured when ADC0_STARTA=0 and ADC0_STARTB =0.) */
#define ADC0_CFG_DFMT                        (BIT(2))                                            /*!< Data format selection bits */
#define ADC0_CFG_DCLREN                      (BIT(3))                                            /*!< Data register auto-clear enable bit */
#define ADC0_CFG_BUFF_EN                     (BIT(4))                                            /*!< channel input signal connects to ADC0 after amplification through BUFFER,function enable bit. (disable for  signal connects directly to ADC)(Only can be configured when ADC0_STARTA=0 and ADC0_STARTB =0.) */
#define ADC0_CFG_HALT_2DAC                   (BIT(5))                                            /*!< DAC linkage configuration bit（ADC state select when DAC coverting） */
#define ADC0_CFG_BUFF_PWRUPT                 (BITS(8,15))                                        /*!< BUFFER power-up stabilization time bits (Time=BUFF_PWRUPT+32)(refer to reference manual for detail) */
#define ADC0_CFG_ADC0_PWRUPT                 (BITS(16,17))                                       /*!< power-up stabilization time bits (Time=ADC0_PWRUPT+32)(refer to reference manual for detail) */
#define ADC0_CFG_INTV_TIME                   (BITS(18,22))                                       /*!< Conversion interval wait cycles configuration bits. */
#define ADC0_CFG_CLKDIV                      (BITS(23,29))                                       /*!< ADC0 module working clock is bus clock divided by (CLKDIV+1) */

/* ADC0_OVSCFG */
#define ADC0_OVSCFG_OVSEN                    (BIT(0))                                            /*!< Oversampling enable bit.(Only can be configured when ADC0_STARTA=0 and ADC0_STARTB =0.) */
#define ADC0_OVSCFG_OVSR                     (BITS(1,3))                                         /*!< Oversampling accumulation times */
#define ADC0_OVSCFG_OVSS                     (BITS(4,7))                                         /*!< Oversampling shift bits(Right Shift) */

/* ADC0_OVSCHSEL */
#define ADC0_OVSCHSEL_CH0                    (BIT(0))                                            /*!< CH0 Oversampling enable bit. */
#define ADC0_OVSCHSEL_CH1                    (BIT(1))                                            /*!< CH1 Oversampling enable bit. */
#define ADC0_OVSCHSEL_CH2                    (BIT(2))                                            /*!< CH2 Oversampling enable bit. */
#define ADC0_OVSCHSEL_CH3                    (BIT(3))                                            /*!< CH3 Oversampling enable bit. */
#define ADC0_OVSCHSEL_CH4                    (BIT(4))                                            /*!< CH4 Oversampling enable bit. */
#define ADC0_OVSCHSEL_CH5                    (BIT(5))                                            /*!< CH5 Oversampling enable bit. */
#define ADC0_OVSCHSEL_CH6                    (BIT(6))                                            /*!< CH6 Oversampling enable bit. */
#define ADC0_OVSCHSEL_CH7                    (BIT(7))                                            /*!< CH7 Oversampling enable bit. */
#define ADC0_OVSCHSEL_CH8                    (BIT(8))                                            /*!< CH8 Oversampling enable bit. */
#define ADC0_OVSCHSEL_CH9                    (BIT(9))                                            /*!< CH9 Oversampling enable bit. */
#define ADC0_OVSCHSEL_CH10                   (BIT(10))                                           /*!< CH10 Oversampling enable bit. */
#define ADC0_OVSCHSEL_CH11                   (BIT(11))                                           /*!< CH11 Oversampling enable bit. */
#define ADC0_OVSCHSEL_CH12                   (BIT(12))                                           /*!< CH12 Oversampling enable bit. */
#define ADC0_OVSCHSEL_CH13                   (BIT(13))                                           /*!< CH13 Oversampling enable bit. */

/* ADC0_PWDCTRL */
#define ADC0_PWDCTRL_BUFF_AON                (BITS(0,7))                                         /*!< ADC0 BUFFER's analog circuit switch configuration bit */
#define ADC0_PWDCTRL_ADC0_AON                (BITS(8,15))                                        /*!< ADC0's analog circuit switch configuration bit */

/* ADC0_SAMPT0 */
#define ADC0_SAMPT0_CH0_SMPT                 (BITS(0,7))                                         /*!< channel 0 sampling clock cycles configuration bits. Cycles = (CHN0_SMPT+1). */
#define ADC0_SAMPT0_CH1_SMPT                 (BITS(8,15))                                        /*!< channel 1 sampling clock cycles configuration bits. Cycles = (CHN1_SMPT+1). */
#define ADC0_SAMPT0_CH2_SMPT                 (BITS(16,23))                                       /*!< channel 2 sampling clock cycles configuration bits. Cycles = (CHN2_SMPT+1). */
#define ADC0_SAMPT0_CH3_SMPT                 (BITS(24,31))                                       /*!< channel 3 sampling clock cycles configuration bits. Cycles = (CHN3_SMPT+1). */

/* ADC0_SAMPT1 */
#define ADC0_SAMPT1_CH4_SMPT                 (BITS(0,7))                                         /*!< channel 4 sampling clock cycles configuration bits. Cycles = (CH4_SMPT+1). */
#define ADC0_SAMPT1_CH5_SMPT                 (BITS(8,15))                                        /*!< channel 5 sampling clock cycles configuration bits. Cycles = (CH5_SMPT+1). */
#define ADC0_SAMPT1_CH6_SMPT                 (BITS(16,23))                                       /*!< channel 6 sampling clock cycles configuration bits. Cycles = (CH6_SMPT+1). */
#define ADC0_SAMPT1_CH7_SMPT                 (BITS(24,31))                                       /*!< channel 7 sampling clock cycles configuration bits. Cycles = (CH7_SMPT+1). */

/* ADC0_SAMPT2 */
#define ADC0_SAMPT2_CH8_SMPT                 (BITS(0,7))                                         /*!< channel 8 sampling clock cycles configuration bits. Cycles = (CH8_SMPT+1). */
#define ADC0_SAMPT2_CH9_SMPT                 (BITS(8,15))                                        /*!< channel 9 sampling clock cycles configuration bits. Cycles = (CH9_SMPT+1). */
#define ADC0_SAMPT2_CH10_SMPT                (BITS(16,23))                                       /*!< channel 10 sampling clock cycles configuration bits. Cycles = (CH10_SMPT+1). */
#define ADC0_SAMPT2_CH11_SMPT                (BITS(24,31))                                       /*!< channel 11 sampling clock cycles configuration bits. Cycles = (CH11_SMPT+1). */

/* ADC0_SAMPT3 */
#define ADC0_SAMPT3_CH12_SMPT                (BITS(0,7))                                         /*!< channel 12 sampling clock cycles configuration bits. Cycles = (CH12_SMPT+1). */
#define ADC0_SAMPT3_CH13_SMPT                (BITS(8,15))                                        /*!< channel 13 sampling clock cycles configuration bits. Cycles = (CH13_SMPT+1). */

/* ADC0_CTRLA */
#define ADC0_CTRLA_CHMOD                     (BIT(0))                                            /*!< Sequence A channel mode selection bit */
#define ADC0_CTRLA_CONT                      (BIT(1))                                            /*!< Sequence A continuous conversion enable bit */
#define ADC0_CTRLA_DSICEN                    (BIT(2))                                            /*!< ADC0 discontinuous scan enable bit */
#define ADC0_CTRLA_DSICNUM                   (BITS(3,6))                                         /*!< ADC0 discontinuous scan count configuration bits. count=(DSICNUM+1) */
#define ADC0_CTRLA_DMAEN                     (BIT(8))                                            /*!< Start DMA data transfer(only supports 16bit transfer) */
#define ADC0_CTRLA_DMA_MODE                  (BIT(9))                                            /*!< DMA mode configuration bit */
#define ADC0_CTRLA_OVRMOD                    (BIT(10))                                           /*!< Data overflow handling method seletion bit */
#define ADC0_CTRLA_AWDEN                     (BIT(11))                                           /*!< Sequence A Analog Watchdog enable bit */
#define ADC0_CTRLA_AWDMOD                    (BITS(12,13))                                       /*!< Sequence A Analog Watchdog mode selection(Watchdog protection range,when data not in protection range AWD flag will be set) */

/* ADC0_CHSELA */
#define ADC0_CHSELA_CH0                      (BIT(0))                                            /*!< Channel 0 enable bit for Sequence A. */
#define ADC0_CHSELA_CH1                      (BIT(1))                                            /*!< Channel 1 enable bit for Sequence A. */
#define ADC0_CHSELA_CH2                      (BIT(2))                                            /*!< Channel 2 enable bit for Sequence A. */
#define ADC0_CHSELA_CH3                      (BIT(3))                                            /*!< Channel 3 enable bit for Sequence A. */
#define ADC0_CHSELA_CH4                      (BIT(4))                                            /*!< Channel 4 enable bit for Sequence A. */
#define ADC0_CHSELA_CH5                      (BIT(5))                                            /*!< Channel 5 enable bit for Sequence A. */
#define ADC0_CHSELA_CH6                      (BIT(6))                                            /*!< Channel 6 enable bit for Sequence A. */
#define ADC0_CHSELA_CH7                      (BIT(7))                                            /*!< Channel 7 enable bit for Sequence A. */
#define ADC0_CHSELA_CH8                      (BIT(8))                                            /*!< Channel 8 enable bit for Sequence A. */
#define ADC0_CHSELA_CH9                      (BIT(9))                                            /*!< Channel 9 enable bit for Sequence A. */
#define ADC0_CHSELA_CH10                     (BIT(10))                                           /*!< Channel 10 enable bit for Sequence A. */
#define ADC0_CHSELA_CH11                     (BIT(11))                                           /*!< Channel 11 enable bit for Sequence A. */
#define ADC0_CHSELA_CH12                     (BIT(12))                                           /*!< Channel 12 enable bit for Sequence A. */
#define ADC0_CHSELA_CH13                     (BIT(13))                                           /*!< Channel 13 enable bit for Sequence A. */

/* ADC0_STARTA */
#define ADC0_STARTA_STARTA                   (BIT(0))                                            /*!< Sequence A Start bit */

/* ADC0_DATA */
#define ADC0_DATA_DAT                        (BITS(0,15))                                        /*!< Sequence A current converting data */
#define ADC0_DATA_CHN                        (BITS(16,19))                                       /*!< Sequence A current convert channel selection(only available when ADC0_CTRLA_CHMOD =0) */

/* ADC0_AWDHLA */
#define ADC0_AWDHLA_HT                       (BITS(0,15))                                        /*!< Watchdog high threshold */
#define ADC0_AWDHLA_LT                       (BITS(16,31))                                       /*!< Watchdog low threshold */

/* ADC0_WCHSELA */
#define ADC0_WCHSELA_CH0                     (BIT(0))                                            /*!< Channel 0 analog watch dog enable bit for Sequence A. */
#define ADC0_WCHSELA_CH1                     (BIT(1))                                            /*!< Channel 1 analog watch dog enable bit for Sequence A. */
#define ADC0_WCHSELA_CH2                     (BIT(2))                                            /*!< Channel 2 analog watch dog enable bit for Sequence A. */
#define ADC0_WCHSELA_CH3                     (BIT(3))                                            /*!< Channel 3 analog watch dog enable bit for Sequence A. */
#define ADC0_WCHSELA_CH4                     (BIT(4))                                            /*!< Channel 4 analog watch dog enable bit for Sequence A. */
#define ADC0_WCHSELA_CH5                     (BIT(5))                                            /*!< Channel 5 analog watch dog enable bit for Sequence A. */
#define ADC0_WCHSELA_CH6                     (BIT(6))                                            /*!< Channel 6 analog watch dog enable bit for Sequence A. */
#define ADC0_WCHSELA_CH7                     (BIT(7))                                            /*!< Channel 7 analog watch dog enable bit for Sequence A. */
#define ADC0_WCHSELA_CH8                     (BIT(8))                                            /*!< Channel 8 analog watch dog enable bit for Sequence A. */
#define ADC0_WCHSELA_CH9                     (BIT(9))                                            /*!< Channel 9 analog watch dog enable bit for Sequence A. */
#define ADC0_WCHSELA_CH10                    (BIT(10))                                           /*!< Channel 10 analog watch dog enable bit for Sequence A. */
#define ADC0_WCHSELA_CH11                    (BIT(11))                                           /*!< Channel 11 analog watch dog enable bit for Sequence A. */
#define ADC0_WCHSELA_CH12                    (BIT(12))                                           /*!< Channel 12 analog watch dog enable bit for Sequence A. */
#define ADC0_WCHSELA_CH13                    (BIT(13))                                           /*!< Channel 13 analog watch dog enable bit for Sequence A. */

/* ADC0_DMA_RBADRA */
#define ADC0_DMA_RBADRA_DMA_RBADRA           (BITS(0,17))                                        /*!< Sequence A DMA Receive Base Address bits */

/* ADC0_DMA_RLENA */
#define ADC0_DMA_RLENA_DMA_RLENA             (BITS(0,17))                                        /*!< Sequence A DMA Receive Length bits */

/* ADC0_DMA_RADRA */
#define ADC0_DMA_RADRA_DMA_RADRA             (BITS(0,17))                                        /*!< Sequence A DMA Current Address bits */

/* ADC0_IEA */
#define ADC0_IEA_EOCIE                       (BIT(0))                                            /*!< Sequence A End Of Conversion Channel Interrupt Enable bit */
#define ADC0_IEA_EOCSIE                      (BIT(1))                                            /*!< Sequence A End Of Conversion Sequence Interrupt Enable bit */
#define ADC0_IEA_AWDIE                       (BIT(2))                                            /*!< Sequence A watchdog event interrupt enable bit */
#define ADC0_IEA_DMA_HIE                     (BIT(3))                                            /*!< Sequence A DMA receive data half-full interrupt enable bit */
#define ADC0_IEA_DMA_FIE                     (BIT(4))                                            /*!< Sequence A DMA receive data full interrupt enable bit */
#define ADC0_IEA_DMA_ERRIE                   (BIT(5))                                            /*!< Sequence A DMA receive data overwrite interrupt enable bit */

/* ADC0_IFA */
#define ADC0_IFA_EOC                         (BIT(0))                                            /*!< Sequence A End Of Conversion Channel Flag */
#define ADC0_IFA_EOCS                        (BIT(1))                                            /*!< Sequence A End Of Conversion Sequence Flag */
#define ADC0_IFA_AWD                         (BIT(2))                                            /*!< Sequence A watchdog event flag */
#define ADC0_IFA_DMA_HDONE                   (BIT(3))                                            /*!< Sequence A DMA receive data half-full flag */
#define ADC0_IFA_DMA_FDONE                   (BIT(4))                                            /*!< Sequence A DMA receive data full flag */
#define ADC0_IFA_DMA_ERR                     (BIT(5))                                            /*!< Sequence A DMA receive data overwrite flag */

/* ADC0_CTRLB */
#define ADC0_CTRLB_CHMOD                     (BIT(0))                                            /*!< Sequence B channel mode selection bit */
#define ADC0_CTRLB_CONT                      (BIT(1))                                            /*!< Sequence B continuous conversion enable bit */
#define ADC0_CTRLB_DSICEN                    (BIT(2))                                            /*!< ADC0 discontinuous scan enable bit */
#define ADC0_CTRLB_DSICNUM                   (BITS(3,6))                                         /*!< ADC0 discontinuous scan count configuration bits. count=(DSICNUM+1) */
#define ADC0_CTRLB_DMAEN                     (BIT(8))                                            /*!< Start DMA data transfer(only supports 16bit transfer) */
#define ADC0_CTRLB_DMA_MODE                  (BIT(9))                                            /*!< DMA mode configuration bit */
#define ADC0_CTRLB_OVRMOD                    (BIT(10))                                           /*!< Data overflow handling method seletion bit */
#define ADC0_CTRLB_AWDEN                     (BIT(11))                                           /*!< Sequence B Analog Watchdog enable bit */
#define ADC0_CTRLB_AWDMOD                    (BITS(12,13))                                       /*!< Sequence B Analog Watchdog mode selection(Watchdog protection range,when data not in protection range AWD flag will be set) */
#define ADC0_CTRLB_SMODE                     (BIT(14))                                           /*!< Processing method selection when sequence B is preempted */

/* ADC0_CHSELB */
#define ADC0_CHSELB_CH0                      (BIT(0))                                            /*!< Channel 0 enable bit for Sequence B. */
#define ADC0_CHSELB_CH1                      (BIT(1))                                            /*!< Channel 1 enable bit for Sequence B. */
#define ADC0_CHSELB_CH2                      (BIT(2))                                            /*!< Channel 2 enable bit for Sequence B. */
#define ADC0_CHSELB_CH3                      (BIT(3))                                            /*!< Channel 3 enable bit for Sequence B. */
#define ADC0_CHSELB_CH4                      (BIT(4))                                            /*!< Channel 4 enable bit for Sequence B. */
#define ADC0_CHSELB_CH5                      (BIT(5))                                            /*!< Channel 5 enable bit for Sequence B. */
#define ADC0_CHSELB_CH6                      (BIT(6))                                            /*!< Channel 6 enable bit for Sequence B. */
#define ADC0_CHSELB_CH7                      (BIT(7))                                            /*!< Channel 7 enable bit for Sequence B. */
#define ADC0_CHSELB_CH8                      (BIT(8))                                            /*!< Channel 8 enable bit for Sequence B. */
#define ADC0_CHSELB_CH9                      (BIT(9))                                            /*!< Channel 9 enable bit for Sequence B. */
#define ADC0_CHSELB_CH10                     (BIT(10))                                           /*!< Channel 10 enable bit for Sequence B. */
#define ADC0_CHSELB_CH11                     (BIT(11))                                           /*!< Channel 11 enable bit for Sequence B. */
#define ADC0_CHSELB_CH12                     (BIT(12))                                           /*!< Channel 12 enable bit for Sequence B. */
#define ADC0_CHSELB_CH13                     (BIT(13))                                           /*!< Channel 13 enable bit for Sequence B. */

/* ADC0_STARTB */
#define ADC0_STARTB_STARTB                   (BIT(0))                                            /*!< Sequence B Start bit */

/* ADC0_DATB */
#define ADC0_DATB_DAT                        (BITS(0,15))                                        /*!< Sequence B current converting data */
#define ADC0_DATB_CHN                        (BITS(16,19))                                       /*!< Sequence B current convert channel selection(only available when ADC0_CTRLB_CHMOD =0) */

/* ADC0_AWDHLB */
#define ADC0_AWDHLB_HT                       (BITS(0,15))                                        /*!< Watchdog high threshold */
#define ADC0_AWDHLB_LT                       (BITS(16,31))                                       /*!< Watchdog low threshold */

/* ADC0_WCHSELB */
#define ADC0_WCHSELB_CH0                     (BIT(0))                                            /*!< Channel 0 analog watch dog enable bit for Sequence B. */
#define ADC0_WCHSELB_CH1                     (BIT(1))                                            /*!< Channel 1 analog watch dog enable bit for Sequence B. */
#define ADC0_WCHSELB_CH2                     (BIT(2))                                            /*!< Channel 2 analog watch dog enable bit for Sequence B. */
#define ADC0_WCHSELB_CH3                     (BIT(3))                                            /*!< Channel 3 analog watch dog enable bit for Sequence B. */
#define ADC0_WCHSELB_CH4                     (BIT(4))                                            /*!< Channel 4 analog watch dog enable bit for Sequence B. */
#define ADC0_WCHSELB_CH5                     (BIT(5))                                            /*!< Channel 5 analog watch dog enable bit for Sequence B. */
#define ADC0_WCHSELB_CH6                     (BIT(6))                                            /*!< Channel 6 analog watch dog enable bit for Sequence B. */
#define ADC0_WCHSELB_CH7                     (BIT(7))                                            /*!< Channel 7 analog watch dog enable bit for Sequence B. */
#define ADC0_WCHSELB_CH8                     (BIT(8))                                            /*!< Channel 8 analog watch dog enable bit for Sequence B. */
#define ADC0_WCHSELB_CH9                     (BIT(9))                                            /*!< Channel 9 analog watch dog enable bit for Sequence B. */
#define ADC0_WCHSELB_CH10                    (BIT(10))                                           /*!< Channel 10 analog watch dog enable bit for Sequence B. */
#define ADC0_WCHSELB_CH11                    (BIT(11))                                           /*!< Channel 11 analog watch dog enable bit for Sequence B. */
#define ADC0_WCHSELB_CH12                    (BIT(12))                                           /*!< Channel 12 analog watch dog enable bit for Sequence B. */
#define ADC0_WCHSELB_CH13                    (BIT(13))                                           /*!< Channel 13 analog watch dog enable bit for Sequence B. */

/* ADC0_DMA_RBADRB */
#define ADC0_DMA_RBADRB_DMA_RBADRB           (BITS(0,17))                                        /*!< Sequence B DMA Receive Base Address bits */

/* ADC0_DMA_RLENB */
#define ADC0_DMA_RLENB_DMA_RLENB             (BITS(0,17))                                        /*!< Sequence B DMA Receive Length bits */

/* ADC0_DMA_RADRB */
#define ADC0_DMA_RADRB_DMA_RADRB             (BITS(0,17))                                        /*!< Sequence B DMA Current Address bits */

/* ADC0_IEB */
#define ADC0_IEB_EOCIE                       (BIT(0))                                            /*!< Sequence B End Of Conversion Channel Interrupt Enable bit */
#define ADC0_IEB_EOCSIE                      (BIT(1))                                            /*!< Sequence B End Of Conversion Sequence Interrupt Enable bit */
#define ADC0_IEB_AWDIE                       (BIT(2))                                            /*!< Sequence B watchdog event interrupt enable bit */
#define ADC0_IEB_DMA_HIE                     (BIT(3))                                            /*!< Sequence B DMA receive data half-full interrupt enable bit */
#define ADC0_IEB_DMA_FIE                     (BIT(4))                                            /*!< Sequence B DMA receive data full interrupt enable bit */
#define ADC0_IEB_DMA_ERRIE                   (BIT(5))                                            /*!< Sequence B DMA receive data overwrite interrupt enable bit */

/* ADC0_IFB */
#define ADC0_IFB_EOC                         (BIT(0))                                            /*!< Sequence B End Of Conversion Channel Flag */
#define ADC0_IFB_EOCS                        (BIT(1))                                            /*!< Sequence B End Of Conversion Sequence Flag */
#define ADC0_IFB_AWD                         (BIT(2))                                            /*!< Sequence B watchdog event flag */
#define ADC0_IFB_DMA_HDONE                   (BIT(3))                                            /*!< Sequence B DMA receive data half-full flag */
#define ADC0_IFB_DMA_FDONE                   (BIT(4))                                            /*!< Sequence B DMA receive data full flag */
#define ADC0_IFB_DMA_ERR                     (BIT(5))                                            /*!< Sequence B DMA receive data overwrite flag */

/* ADC0_CAL */
#define ADC0_CAL_ZTG                         (BITS(0,11))                                        /*!< ZEROT calabration value for the condition that BUFFER is disabled (Only can be configured when ADC0_STARTA=0 and ADC0_STARTB =0.) */
#define ADC0_CAL_ZTBG                        (BITS(12,23))                                       /*!< ZEROT calabration value for the condition that BUFFER is enabled (Only can be configured when ADC0_STARTA=0 and ADC0_STARTB =0.) */
#define ADC0_CAL_CAL_EN                      (BIT(24))                                           /*!< Hardware calibration enable bit(Only can be configured when ADC0_STARTA=0 and ADC0_STARTB =0.) */

/* ADC0_DOS */
#define ADC0_DOS_DOS                         (BITS(0,11))                                        /*!< (Two's complement signed value) Digital Offset Code Value Configuration bits */

/* ADC0_BGRLP */
#define ADC0_BGRLP_CORE                      (BITS(0,15))                                        /*!< accumulated code theroy value, when buffer disabled */
#define ADC0_BGRLP_COREB                     (BITS(16,31))                                       /*!< accumulated code theroy value, when buffer enabled */

/* ADC0_DCONV */
#define ADC0_DCONV_DCONV                     (BITS(0,1))                                         /*!< Configuration of dummy conversion count after power-on. */

/* constants definitions */
/* ADC0_CFG_RESC */
#define ADC0_CFG_RESC_POS                    (0U)                                                /*!< ADC0_CFG_RESC bit offset */
#define ADC0_CFG_RESC_CFG(regval)  (ADC0_CFG_RESC & ((uint32_t)(regval) << ADC0_CFG_RESC_POS))
#define ADC0_CFG_RESC_12BIT                  (ADC0_CFG_RESC_CFG(0))                              /*!< select as 12-bit */
#define ADC0_CFG_RESC_10BIT                  (ADC0_CFG_RESC_CFG(1))                              /*!< select as 10-bit */
#define ADC0_CFG_RESC_8BIT                   (ADC0_CFG_RESC_CFG(2))                              /*!< select as 8-bit */
#define ADC0_CFG_RESC_6BIT                   (ADC0_CFG_RESC_CFG(3))                              /*!< select as 6-bit */

/* ADC0_CFG_DFMT */
#define ADC0_CFG_DFMT_POS                    (2U)                                                /*!< ADC0_CFG_DFMT bit offset */
#define ADC0_CFG_DFMT_RA                     ((uint32_t)0x00000000U)                             /*!< Right-aligned data */
#define ADC0_CFG_DFMT_LA                     (ADC0_CFG_DFMT)                                     /*!< Left-aligned data */

/* ADC0_CFG_DCLREN */
#define ADC0_CFG_DCLREN_POS                  (3U)                                                /*!< ADC0_CFG_DCLREN bit offset */

/* ADC0_CFG_BUFF_EN */
#define ADC0_CFG_BUFF_EN_POS                 (4U)                                                /*!< ADC0_CFG_BUFF_EN bit offset */

/* ADC0_CFG_HALT_2DAC */
#define ADC0_CFG_HALT_2DAC_POS               (5U)                                                /*!< ADC0_CFG_HALT_2DAC bit offset */
#define ADC0_CFG_HALT_2DAC_CONTINUE          ((uint32_t)0x00000000U)                             /*!< ADC0 operates normally regardless of DAC status */
#define ADC0_CFG_HALT_2DAC_PAUSE             (ADC0_CFG_HALT_2DAC)                                /*!< ADC0 pauses conversion when DAC is in conversion state */

/* ADC0_CFG_BUFF_PWRUPT */
#define ADC0_CFG_BUFF_PWRUPT_POS             (8U)                                                /*!< ADC0_CFG_BUFF_PWRUPT bit offset */

/* ADC0_CFG_ADC0_PWRUPT */
#define ADC0_CFG_ADC0_PWRUPT_POS             (16U)                                               /*!< ADC0_CFG_ADC0_PWRUPT bit offset */

/* ADC0_CFG_INTV_TIME */
#define ADC0_CFG_INTV_TIME_POS               (18U)                                               /*!< ADC0_CFG_INTV_TIME bit offset */

/* ADC0_CFG_CLKDIV */
#define ADC0_CFG_CLKDIV_POS                  (23U)                                               /*!< ADC0_CFG_CLKDIV bit offset */

/* ADC0_OVSCFG_OVSEN */
#define ADC0_OVSCFG_OVSEN_POS                (0U)                                                /*!< ADC0_OVSCFG_OVSEN bit offset */

/* ADC0_OVSCFG_OVSR */
#define ADC0_OVSCFG_OVSR_POS                 (1U)                                                /*!< ADC0_OVSCFG_OVSR bit offset */
#define ADC0_OVSCFG_OVSR_CFG(regval)  (ADC0_OVSCFG_OVSR & ((uint32_t)(regval) << ADC0_OVSCFG_OVSR_POS))
#define ADC0_OVSCFG_OVSR_ACC2                (ADC0_OVSCFG_OVSR_CFG(0))                           /*!< Accumulate 2 times */
#define ADC0_OVSCFG_OVSR_ACC4                (ADC0_OVSCFG_OVSR_CFG(1))                           /*!< Accumulate 4 times */
#define ADC0_OVSCFG_OVSR_ACC8                (ADC0_OVSCFG_OVSR_CFG(2))                           /*!< Accumulate 8 times */
#define ADC0_OVSCFG_OVSR_ACC16               (ADC0_OVSCFG_OVSR_CFG(3))                           /*!< Accumulate 16 times */
#define ADC0_OVSCFG_OVSR_ACC32               (ADC0_OVSCFG_OVSR_CFG(4))                           /*!< Accumulate 32 times */
#define ADC0_OVSCFG_OVSR_ACC64               (ADC0_OVSCFG_OVSR_CFG(5))                           /*!< Accumulate 64 times */
#define ADC0_OVSCFG_OVSR_ACC128              (ADC0_OVSCFG_OVSR_CFG(6))                           /*!< Accumulate 128 times */
#define ADC0_OVSCFG_OVSR_ACC256              (ADC0_OVSCFG_OVSR_CFG(7))                           /*!< Accumulate 256 times */

/* ADC0_OVSCFG_OVSS */
#define ADC0_OVSCFG_OVSS_POS                 (4U)                                                /*!< ADC0_OVSCFG_OVSS bit offset */
#define ADC0_OVSCFG_OVSS_CFG(regval)  (ADC0_OVSCFG_OVSS & ((uint32_t)(regval) << ADC0_OVSCFG_OVSS_POS))
#define ADC0_OVSCFG_OVSS_SHLNUL              (ADC0_OVSCFG_OVSS_CFG(0))                           /*!< No shift */
#define ADC0_OVSCFG_OVSS_SHL1                (ADC0_OVSCFG_OVSS_CFG(1))                           /*!< Shift 1 bit */
#define ADC0_OVSCFG_OVSS_SHL2                (ADC0_OVSCFG_OVSS_CFG(2))                           /*!< Shift 2 bits */
#define ADC0_OVSCFG_OVSS_SHL3                (ADC0_OVSCFG_OVSS_CFG(3))                           /*!< Shift 3 bits */
#define ADC0_OVSCFG_OVSS_SHL4                (ADC0_OVSCFG_OVSS_CFG(4))                           /*!< Shift 4 bits */
#define ADC0_OVSCFG_OVSS_SHL5                (ADC0_OVSCFG_OVSS_CFG(5))                           /*!< Shift 5 bits */
#define ADC0_OVSCFG_OVSS_SHL6                (ADC0_OVSCFG_OVSS_CFG(6))                           /*!< Shift 6 bits */
#define ADC0_OVSCFG_OVSS_SHL7                (ADC0_OVSCFG_OVSS_CFG(7))                           /*!< Shift 7 bits */
#define ADC0_OVSCFG_OVSS_SHL8                (ADC0_OVSCFG_OVSS_CFG(8))                           /*!< Shift 8 bits */

/* ADC0_OVSCHSEL_CH0 */
#define ADC0_OVSCHSEL_CH0_POS                (0U)                                                /*!< ADC0_OVSCHSEL_CH0 bit offset */

/* ADC0_OVSCHSEL_CH1 */
#define ADC0_OVSCHSEL_CH1_POS                (1U)                                                /*!< ADC0_OVSCHSEL_CH1 bit offset */

/* ADC0_OVSCHSEL_CH2 */
#define ADC0_OVSCHSEL_CH2_POS                (2U)                                                /*!< ADC0_OVSCHSEL_CH2 bit offset */

/* ADC0_OVSCHSEL_CH3 */
#define ADC0_OVSCHSEL_CH3_POS                (3U)                                                /*!< ADC0_OVSCHSEL_CH3 bit offset */

/* ADC0_OVSCHSEL_CH4 */
#define ADC0_OVSCHSEL_CH4_POS                (4U)                                                /*!< ADC0_OVSCHSEL_CH4 bit offset */

/* ADC0_OVSCHSEL_CH5 */
#define ADC0_OVSCHSEL_CH5_POS                (5U)                                                /*!< ADC0_OVSCHSEL_CH5 bit offset */

/* ADC0_OVSCHSEL_CH6 */
#define ADC0_OVSCHSEL_CH6_POS                (6U)                                                /*!< ADC0_OVSCHSEL_CH6 bit offset */

/* ADC0_OVSCHSEL_CH7 */
#define ADC0_OVSCHSEL_CH7_POS                (7U)                                                /*!< ADC0_OVSCHSEL_CH7 bit offset */

/* ADC0_OVSCHSEL_CH8 */
#define ADC0_OVSCHSEL_CH8_POS                (8U)                                                /*!< ADC0_OVSCHSEL_CH8 bit offset */

/* ADC0_OVSCHSEL_CH9 */
#define ADC0_OVSCHSEL_CH9_POS                (9U)                                                /*!< ADC0_OVSCHSEL_CH9 bit offset */

/* ADC0_OVSCHSEL_CH10 */
#define ADC0_OVSCHSEL_CH10_POS               (10U)                                               /*!< ADC0_OVSCHSEL_CH10 bit offset */

/* ADC0_OVSCHSEL_CH11 */
#define ADC0_OVSCHSEL_CH11_POS               (11U)                                               /*!< ADC0_OVSCHSEL_CH11 bit offset */

/* ADC0_OVSCHSEL_CH12 */
#define ADC0_OVSCHSEL_CH12_POS               (12U)                                               /*!< ADC0_OVSCHSEL_CH12 bit offset */

/* ADC0_OVSCHSEL_CH13 */
#define ADC0_OVSCHSEL_CH13_POS               (13U)                                               /*!< ADC0_OVSCHSEL_CH13 bit offset */

/* ADC0_PWDCTRL_BUFF_AON */
#define ADC0_PWDCTRL_BUFF_AON_POS            (0U)                                                /*!< ADC0_PWDCTRL_BUFF_AON bit offset */
#define ADC0_PWDCTRL_BUFF_AON_ALWAYSON       ((uint32_t)0x000000ACU)                             /*!< BUFFER analog circuit always on */
#define ADC0_PWDCTRL_BUFF_AON_DEMANDON       ((uint32_t)0x00000000U)                             /*!< BUFFER analog circuit only turns on when working */

/* ADC0_PWDCTRL_ADC0_AON */
#define ADC0_PWDCTRL_ADC0_AON_POS            (8U)                                                /*!< ADC0_PWDCTRL_ADC0_AON bit offset */
#define ADC0_PWDCTRL_ADC0_AON_ALWAYSON       ((uint32_t)0x0000A500U)                             /*!< ADC0 analog circuit always on */
#define ADC0_PWDCTRL_ADC0_AON_DEMANDON       ((uint32_t)0x00000000U)                             /*!< ADC0 analog circuit only turns on when working */

/* ADC0_SAMPT0_CH0_SMPT */
#define ADC0_SAMPT0_CH0_SMPT_POS             (0U)                                                /*!< ADC0_SAMPT0_CH0_SMPT bit offset */

/* ADC0_SAMPT0_CH1_SMPT */
#define ADC0_SAMPT0_CH1_SMPT_POS             (8U)                                                /*!< ADC0_SAMPT0_CH1_SMPT bit offset */

/* ADC0_SAMPT0_CH2_SMPT */
#define ADC0_SAMPT0_CH2_SMPT_POS             (16U)                                               /*!< ADC0_SAMPT0_CH2_SMPT bit offset */

/* ADC0_SAMPT0_CH3_SMPT */
#define ADC0_SAMPT0_CH3_SMPT_POS             (24U)                                               /*!< ADC0_SAMPT0_CH3_SMPT bit offset */

/* ADC0_SAMPT1_CH4_SMPT */
#define ADC0_SAMPT1_CH4_SMPT_POS             (0U)                                                /*!< ADC0_SAMPT1_CH4_SMPT bit offset */

/* ADC0_SAMPT1_CH5_SMPT */
#define ADC0_SAMPT1_CH5_SMPT_POS             (8U)                                                /*!< ADC0_SAMPT1_CH5_SMPT bit offset */

/* ADC0_SAMPT1_CH6_SMPT */
#define ADC0_SAMPT1_CH6_SMPT_POS             (16U)                                               /*!< ADC0_SAMPT1_CH6_SMPT bit offset */

/* ADC0_SAMPT1_CH7_SMPT */
#define ADC0_SAMPT1_CH7_SMPT_POS             (24U)                                               /*!< ADC0_SAMPT1_CH7_SMPT bit offset */

/* ADC0_SAMPT2_CH8_SMPT */
#define ADC0_SAMPT2_CH8_SMPT_POS             (0U)                                                /*!< ADC0_SAMPT2_CH8_SMPT bit offset */

/* ADC0_SAMPT2_CH9_SMPT */
#define ADC0_SAMPT2_CH9_SMPT_POS             (8U)                                                /*!< ADC0_SAMPT2_CH9_SMPT bit offset */

/* ADC0_SAMPT2_CH10_SMPT */
#define ADC0_SAMPT2_CH10_SMPT_POS            (16U)                                               /*!< ADC0_SAMPT2_CH10_SMPT bit offset */

/* ADC0_SAMPT2_CH11_SMPT */
#define ADC0_SAMPT2_CH11_SMPT_POS            (24U)                                               /*!< ADC0_SAMPT2_CH11_SMPT bit offset */

/* ADC0_SAMPT3_CH12_SMPT */
#define ADC0_SAMPT3_CH12_SMPT_POS            (0U)                                                /*!< ADC0_SAMPT3_CH12_SMPT bit offset */

/* ADC0_SAMPT3_CH13_SMPT */
#define ADC0_SAMPT3_CH13_SMPT_POS            (8U)                                                /*!< ADC0_SAMPT3_CH13_SMPT bit offset */

/* ADC0_CTRLA_CHMOD */
#define ADC0_CTRLA_CHMOD_POS                 (0U)                                                /*!< ADC0_CTRLA_CHMOD bit offset */
#define ADC0_CTRLA_CHMOD_DATCH               ((uint32_t)0x00000000U)                             /*!< Sequence AChannel configuration depend on ADC0_DATA_CHN */
#define ADC0_CTRLA_CHMOD_CHSEL               (ADC0_CTRLA_CHMOD)                                  /*!< Sequence AChannel configuration depend on ADC0_CHSELA */

/* ADC0_CTRLA_CONT */
#define ADC0_CTRLA_CONT_POS                  (1U)                                                /*!< ADC0_CTRLA_CONT bit offset */
#define ADC0_CTRLA_CONT_ONCE                 ((uint32_t)0x00000000U)                             /*!< ADC0 converts the specified channel(s) once */
#define ADC0_CTRLA_CONT_CONTIUE              (ADC0_CTRLA_CONT)                                   /*!< ADC0 continuously converts the specified channel(s) until software writes 0 to ADC0_STARTA */

/* ADC0_CTRLA_DSICEN */
#define ADC0_CTRLA_DSICEN_POS                (2U)                                                /*!< ADC0_CTRLA_DSICEN bit offset */

/* ADC0_CTRLA_DSICNUM */
#define ADC0_CTRLA_DSICNUM_POS               (3U)                                                /*!< ADC0_CTRLA_DSICNUM bit offset */

/* ADC0_CTRLA_DMAEN */
#define ADC0_CTRLA_DMAEN_POS                 (8U)                                                /*!< ADC0_CTRLA_DMAEN bit offset */

/* ADC0_CTRLA_DMA_MODE */
#define ADC0_CTRLA_DMA_MODE_POS              (9U)                                                /*!< ADC0_CTRLA_DMA_MODE bit offset */
#define ADC0_CTRLA_DMA_MODE_ONCE             ((uint32_t)0x00000000U)                             /*!< select DMA convert mode as convert once */
#define ADC0_CTRLA_DMA_MODE_CONTIUE          (ADC0_CTRLA_DMA_MODE)                               /*!< select DMA convert mode as continuous convert */

/* ADC0_CTRLA_OVRMOD */
#define ADC0_CTRLA_OVRMOD_POS                (10U)                                               /*!< ADC0_CTRLA_OVRMOD bit offset */
#define ADC0_CTRLA_OVRMOD_NUL                ((uint32_t)0x00000000U)                             /*!< Do not recover data when overflow */
#define ADC0_CTRLA_OVRMOD_RECOVER            (ADC0_CTRLA_OVRMOD)                                 /*!< recover data when overflow */

/* ADC0_CTRLA_AWDEN */
#define ADC0_CTRLA_AWDEN_POS                 (11U)                                               /*!< ADC0_CTRLA_AWDEN bit offset */

/* ADC0_CTRLA_AWDMOD */
#define ADC0_CTRLA_AWDMOD_POS                (12U)                                               /*!< ADC0_CTRLA_AWDMOD bit offset */
#define ADC0_CTRLA_AWDMOD_CFG(regval)  (ADC0_CTRLA_AWDMOD & ((uint32_t)(regval) << ADC0_CTRLA_AWDMOD_POS))
#define ADC0_CTRLA_AWDMOD_PHL                (ADC0_CTRLA_AWDMOD_CFG(0))                          /*!< Protection range: AWD Low threshold< DATA < AWD High threshold */
#define ADC0_CTRLA_AWDMOD_PL                 (ADC0_CTRLA_AWDMOD_CFG(1))                          /*!< Protection range: AWD Low threshold< DATA */
#define ADC0_CTRLA_AWDMOD_PH                 (ADC0_CTRLA_AWDMOD_CFG(2))                          /*!< Protection range: DATA > AWD High threshold */

/* ADC0_CHSELA_CH0 */
#define ADC0_CHSELA_CH0_POS                  (0U)                                                /*!< ADC0_CHSELA_CH0 bit offset */

/* ADC0_CHSELA_CH1 */
#define ADC0_CHSELA_CH1_POS                  (1U)                                                /*!< ADC0_CHSELA_CH1 bit offset */

/* ADC0_CHSELA_CH2 */
#define ADC0_CHSELA_CH2_POS                  (2U)                                                /*!< ADC0_CHSELA_CH2 bit offset */

/* ADC0_CHSELA_CH3 */
#define ADC0_CHSELA_CH3_POS                  (3U)                                                /*!< ADC0_CHSELA_CH3 bit offset */

/* ADC0_CHSELA_CH4 */
#define ADC0_CHSELA_CH4_POS                  (4U)                                                /*!< ADC0_CHSELA_CH4 bit offset */

/* ADC0_CHSELA_CH5 */
#define ADC0_CHSELA_CH5_POS                  (5U)                                                /*!< ADC0_CHSELA_CH5 bit offset */

/* ADC0_CHSELA_CH6 */
#define ADC0_CHSELA_CH6_POS                  (6U)                                                /*!< ADC0_CHSELA_CH6 bit offset */

/* ADC0_CHSELA_CH7 */
#define ADC0_CHSELA_CH7_POS                  (7U)                                                /*!< ADC0_CHSELA_CH7 bit offset */

/* ADC0_CHSELA_CH8 */
#define ADC0_CHSELA_CH8_POS                  (8U)                                                /*!< ADC0_CHSELA_CH8 bit offset */

/* ADC0_CHSELA_CH9 */
#define ADC0_CHSELA_CH9_POS                  (9U)                                                /*!< ADC0_CHSELA_CH9 bit offset */

/* ADC0_CHSELA_CH10 */
#define ADC0_CHSELA_CH10_POS                 (10U)                                               /*!< ADC0_CHSELA_CH10 bit offset */

/* ADC0_CHSELA_CH11 */
#define ADC0_CHSELA_CH11_POS                 (11U)                                               /*!< ADC0_CHSELA_CH11 bit offset */

/* ADC0_CHSELA_CH12 */
#define ADC0_CHSELA_CH12_POS                 (12U)                                               /*!< ADC0_CHSELA_CH12 bit offset */

/* ADC0_CHSELA_CH13 */
#define ADC0_CHSELA_CH13_POS                 (13U)                                               /*!< ADC0_CHSELA_CH13 bit offset */

/* ADC0_STARTA_STARTA */
#define ADC0_STARTA_STARTA_POS               (0U)                                                /*!< ADC0_STARTA_STARTA bit offset */

/* ADC0_DATA_DAT */
#define ADC0_DATA_DAT_POS                    (0U)                                                /*!< ADC0_DATA_DAT bit offset */

/* ADC0_DATA_CHN */
#define ADC0_DATA_CHN_POS                    (16U)                                               /*!< ADC0_DATA_CHN bit offset */

/* ADC0_AWDHLA_HT */
#define ADC0_AWDHLA_HT_POS                   (0U)                                                /*!< ADC0_AWDHLA_HT bit offset */

/* ADC0_AWDHLA_LT */
#define ADC0_AWDHLA_LT_POS                   (16U)                                               /*!< ADC0_AWDHLA_LT bit offset */

/* ADC0_WCHSELA_CH0 */
#define ADC0_WCHSELA_CH0_POS                 (0U)                                                /*!< ADC0_WCHSELA_CH0 bit offset */

/* ADC0_WCHSELA_CH1 */
#define ADC0_WCHSELA_CH1_POS                 (1U)                                                /*!< ADC0_WCHSELA_CH1 bit offset */

/* ADC0_WCHSELA_CH2 */
#define ADC0_WCHSELA_CH2_POS                 (2U)                                                /*!< ADC0_WCHSELA_CH2 bit offset */

/* ADC0_WCHSELA_CH3 */
#define ADC0_WCHSELA_CH3_POS                 (3U)                                                /*!< ADC0_WCHSELA_CH3 bit offset */

/* ADC0_WCHSELA_CH4 */
#define ADC0_WCHSELA_CH4_POS                 (4U)                                                /*!< ADC0_WCHSELA_CH4 bit offset */

/* ADC0_WCHSELA_CH5 */
#define ADC0_WCHSELA_CH5_POS                 (5U)                                                /*!< ADC0_WCHSELA_CH5 bit offset */

/* ADC0_WCHSELA_CH6 */
#define ADC0_WCHSELA_CH6_POS                 (6U)                                                /*!< ADC0_WCHSELA_CH6 bit offset */

/* ADC0_WCHSELA_CH7 */
#define ADC0_WCHSELA_CH7_POS                 (7U)                                                /*!< ADC0_WCHSELA_CH7 bit offset */

/* ADC0_WCHSELA_CH8 */
#define ADC0_WCHSELA_CH8_POS                 (8U)                                                /*!< ADC0_WCHSELA_CH8 bit offset */

/* ADC0_WCHSELA_CH9 */
#define ADC0_WCHSELA_CH9_POS                 (9U)                                                /*!< ADC0_WCHSELA_CH9 bit offset */

/* ADC0_WCHSELA_CH10 */
#define ADC0_WCHSELA_CH10_POS                (10U)                                               /*!< ADC0_WCHSELA_CH10 bit offset */

/* ADC0_WCHSELA_CH11 */
#define ADC0_WCHSELA_CH11_POS                (11U)                                               /*!< ADC0_WCHSELA_CH11 bit offset */

/* ADC0_WCHSELA_CH12 */
#define ADC0_WCHSELA_CH12_POS                (12U)                                               /*!< ADC0_WCHSELA_CH12 bit offset */

/* ADC0_WCHSELA_CH13 */
#define ADC0_WCHSELA_CH13_POS                (13U)                                               /*!< ADC0_WCHSELA_CH13 bit offset */

/* ADC0_DMA_RBADRA_DMA_RBADRA */
#define ADC0_DMA_RBADRA_DMA_RBADRA_POS       (0U)                                                /*!< ADC0_DMA_RBADRA_DMA_RBADRA bit offset */

/* ADC0_DMA_RLENA_DMA_RLENA */
#define ADC0_DMA_RLENA_DMA_RLENA_POS         (0U)                                                /*!< ADC0_DMA_RLENA_DMA_RLENA bit offset */

/* ADC0_DMA_RADRA_DMA_RADRA */
#define ADC0_DMA_RADRA_DMA_RADRA_POS         (0U)                                                /*!< ADC0_DMA_RADRA_DMA_RADRA bit offset */

/* ADC0_IEA_EOCIE */
#define ADC0_IEA_EOCIE_POS                   (0U)                                                /*!< ADC0_IEA_EOCIE bit offset */

/* ADC0_IEA_EOCSIE */
#define ADC0_IEA_EOCSIE_POS                  (1U)                                                /*!< ADC0_IEA_EOCSIE bit offset */

/* ADC0_IEA_AWDIE */
#define ADC0_IEA_AWDIE_POS                   (2U)                                                /*!< ADC0_IEA_AWDIE bit offset */

/* ADC0_IEA_DMA_HIE */
#define ADC0_IEA_DMA_HIE_POS                 (3U)                                                /*!< ADC0_IEA_DMA_HIE bit offset */

/* ADC0_IEA_DMA_FIE */
#define ADC0_IEA_DMA_FIE_POS                 (4U)                                                /*!< ADC0_IEA_DMA_FIE bit offset */

/* ADC0_IEA_DMA_ERRIE */
#define ADC0_IEA_DMA_ERRIE_POS               (5U)                                                /*!< ADC0_IEA_DMA_ERRIE bit offset */

/* ADC0_IFA_EOC */
#define ADC0_IFA_EOC_POS                     (0U)                                                /*!< ADC0_IFA_EOC bit offset */

/* ADC0_IFA_EOCS */
#define ADC0_IFA_EOCS_POS                    (1U)                                                /*!< ADC0_IFA_EOCS bit offset */

/* ADC0_IFA_AWD */
#define ADC0_IFA_AWD_POS                     (2U)                                                /*!< ADC0_IFA_AWD bit offset */

/* ADC0_IFA_DMA_HDONE */
#define ADC0_IFA_DMA_HDONE_POS               (3U)                                                /*!< ADC0_IFA_DMA_HDONE bit offset */

/* ADC0_IFA_DMA_FDONE */
#define ADC0_IFA_DMA_FDONE_POS               (4U)                                                /*!< ADC0_IFA_DMA_FDONE bit offset */

/* ADC0_IFA_DMA_ERR */
#define ADC0_IFA_DMA_ERR_POS                 (5U)                                                /*!< ADC0_IFA_DMA_ERR bit offset */

/* ADC0_CTRLB_CHMOD */
#define ADC0_CTRLB_CHMOD_POS                 (0U)                                                /*!< ADC0_CTRLB_CHMOD bit offset */
#define ADC0_CTRLB_CHMOD_DATCH               ((uint32_t)0x00000000U)                             /*!< Sequence B Channel configuration depend on ADC0_DATB_CHN */
#define ADC0_CTRLB_CHMOD_CHSEL               (ADC0_CTRLB_CHMOD)                                  /*!< Sequence B Channel configuration depend on ADC0_CHSELB */

/* ADC0_CTRLB_CONT */
#define ADC0_CTRLB_CONT_POS                  (1U)                                                /*!< ADC0_CTRLB_CONT bit offset */
#define ADC0_CTRLB_CONT_ONCE                 ((uint32_t)0x00000000U)                             /*!< ADC0 converts the specified channel(s) once */
#define ADC0_CTRLB_CONT_CONTIUE              (ADC0_CTRLB_CONT)                                   /*!< ADC0 continuously converts the specified channel(s) until software writes 0 to ADC0_STARTA */

/* ADC0_CTRLB_DSICEN */
#define ADC0_CTRLB_DSICEN_POS                (2U)                                                /*!< ADC0_CTRLB_DSICEN bit offset */

/* ADC0_CTRLB_DSICNUM */
#define ADC0_CTRLB_DSICNUM_POS               (3U)                                                /*!< ADC0_CTRLB_DSICNUM bit offset */

/* ADC0_CTRLB_DMAEN */
#define ADC0_CTRLB_DMAEN_POS                 (8U)                                                /*!< ADC0_CTRLB_DMAEN bit offset */

/* ADC0_CTRLB_DMA_MODE */
#define ADC0_CTRLB_DMA_MODE_POS              (9U)                                                /*!< ADC0_CTRLB_DMA_MODE bit offset */
#define ADC0_CTRLB_DMA_MODE_ONCE             ((uint32_t)0x00000000U)                             /*!< select DMA convert mode as convert once */
#define ADC0_CTRLB_DMA_MODE_CONTIUE          (ADC0_CTRLB_DMA_MODE)                               /*!< select DMA convert mode as continuous convert */

/* ADC0_CTRLB_OVRMOD */
#define ADC0_CTRLB_OVRMOD_POS                (10U)                                               /*!< ADC0_CTRLB_OVRMOD bit offset */
#define ADC0_CTRLB_OVRMOD_NUL                ((uint32_t)0x00000000U)                             /*!< Do not recover data when overflow */
#define ADC0_CTRLB_OVRMOD_RECOVER            (ADC0_CTRLB_OVRMOD)                                 /*!< recover data when overflow */

/* ADC0_CTRLB_AWDEN */
#define ADC0_CTRLB_AWDEN_POS                 (11U)                                               /*!< ADC0_CTRLB_AWDEN bit offset */

/* ADC0_CTRLB_AWDMOD */
#define ADC0_CTRLB_AWDMOD_POS                (12U)                                               /*!< ADC0_CTRLB_AWDMOD bit offset */
#define ADC0_CTRLB_AWDMOD_CFG(regval)  (ADC0_CTRLB_AWDMOD & ((uint32_t)(regval) << ADC0_CTRLB_AWDMOD_POS))
#define ADC0_CTRLB_AWDMOD_PHL                (ADC0_CTRLB_AWDMOD_CFG(0))                          /*!< Protection range: AWD Low threshold< DATA < AWD High threshold */
#define ADC0_CTRLB_AWDMOD_PL                 (ADC0_CTRLB_AWDMOD_CFG(1))                          /*!< Protection range: AWD Low threshold< DATA */
#define ADC0_CTRLB_AWDMOD_PH                 (ADC0_CTRLB_AWDMOD_CFG(2))                          /*!< Protection range: DATA > AWD High threshold */

/* ADC0_CTRLB_SMODE */
#define ADC0_CTRLB_SMODE_POS                 (14U)                                               /*!< ADC0_CTRLB_SMODE bit offset */
#define ADC0_CTRLB_SMODE_STOP                ((uint32_t)0x00000000U)                             /*!< Force abort conversion and switch to sequence A */
#define ADC0_CTRLB_SMODE_WAIT                (ADC0_CTRLB_SMODE)                                  /*!< Wait for current conversion to complete before switching to sequence A */

/* ADC0_CHSELB_CH0 */
#define ADC0_CHSELB_CH0_POS                  (0U)                                                /*!< ADC0_CHSELB_CH0 bit offset */

/* ADC0_CHSELB_CH1 */
#define ADC0_CHSELB_CH1_POS                  (1U)                                                /*!< ADC0_CHSELB_CH1 bit offset */

/* ADC0_CHSELB_CH2 */
#define ADC0_CHSELB_CH2_POS                  (2U)                                                /*!< ADC0_CHSELB_CH2 bit offset */

/* ADC0_CHSELB_CH3 */
#define ADC0_CHSELB_CH3_POS                  (3U)                                                /*!< ADC0_CHSELB_CH3 bit offset */

/* ADC0_CHSELB_CH4 */
#define ADC0_CHSELB_CH4_POS                  (4U)                                                /*!< ADC0_CHSELB_CH4 bit offset */

/* ADC0_CHSELB_CH5 */
#define ADC0_CHSELB_CH5_POS                  (5U)                                                /*!< ADC0_CHSELB_CH5 bit offset */

/* ADC0_CHSELB_CH6 */
#define ADC0_CHSELB_CH6_POS                  (6U)                                                /*!< ADC0_CHSELB_CH6 bit offset */

/* ADC0_CHSELB_CH7 */
#define ADC0_CHSELB_CH7_POS                  (7U)                                                /*!< ADC0_CHSELB_CH7 bit offset */

/* ADC0_CHSELB_CH8 */
#define ADC0_CHSELB_CH8_POS                  (8U)                                                /*!< ADC0_CHSELB_CH8 bit offset */

/* ADC0_CHSELB_CH9 */
#define ADC0_CHSELB_CH9_POS                  (9U)                                                /*!< ADC0_CHSELB_CH9 bit offset */

/* ADC0_CHSELB_CH10 */
#define ADC0_CHSELB_CH10_POS                 (10U)                                               /*!< ADC0_CHSELB_CH10 bit offset */

/* ADC0_CHSELB_CH11 */
#define ADC0_CHSELB_CH11_POS                 (11U)                                               /*!< ADC0_CHSELB_CH11 bit offset */

/* ADC0_CHSELB_CH12 */
#define ADC0_CHSELB_CH12_POS                 (12U)                                               /*!< ADC0_CHSELB_CH12 bit offset */

/* ADC0_CHSELB_CH13 */
#define ADC0_CHSELB_CH13_POS                 (13U)                                               /*!< ADC0_CHSELB_CH13 bit offset */

/* ADC0_STARTB_STARTB */
#define ADC0_STARTB_STARTB_POS               (0U)                                                /*!< ADC0_STARTB_STARTB bit offset */

/* ADC0_DATB_DAT */
#define ADC0_DATB_DAT_POS                    (0U)                                                /*!< ADC0_DATB_DAT bit offset */

/* ADC0_DATB_CHN */
#define ADC0_DATB_CHN_POS                    (16U)                                               /*!< ADC0_DATB_CHN bit offset */

/* ADC0_AWDHLB_HT */
#define ADC0_AWDHLB_HT_POS                   (0U)                                                /*!< ADC0_AWDHLB_HT bit offset */

/* ADC0_AWDHLB_LT */
#define ADC0_AWDHLB_LT_POS                   (16U)                                               /*!< ADC0_AWDHLB_LT bit offset */

/* ADC0_WCHSELB_CH0 */
#define ADC0_WCHSELB_CH0_POS                 (0U)                                                /*!< ADC0_WCHSELB_CH0 bit offset */

/* ADC0_WCHSELB_CH1 */
#define ADC0_WCHSELB_CH1_POS                 (1U)                                                /*!< ADC0_WCHSELB_CH1 bit offset */

/* ADC0_WCHSELB_CH2 */
#define ADC0_WCHSELB_CH2_POS                 (2U)                                                /*!< ADC0_WCHSELB_CH2 bit offset */

/* ADC0_WCHSELB_CH3 */
#define ADC0_WCHSELB_CH3_POS                 (3U)                                                /*!< ADC0_WCHSELB_CH3 bit offset */

/* ADC0_WCHSELB_CH4 */
#define ADC0_WCHSELB_CH4_POS                 (4U)                                                /*!< ADC0_WCHSELB_CH4 bit offset */

/* ADC0_WCHSELB_CH5 */
#define ADC0_WCHSELB_CH5_POS                 (5U)                                                /*!< ADC0_WCHSELB_CH5 bit offset */

/* ADC0_WCHSELB_CH6 */
#define ADC0_WCHSELB_CH6_POS                 (6U)                                                /*!< ADC0_WCHSELB_CH6 bit offset */

/* ADC0_WCHSELB_CH7 */
#define ADC0_WCHSELB_CH7_POS                 (7U)                                                /*!< ADC0_WCHSELB_CH7 bit offset */

/* ADC0_WCHSELB_CH8 */
#define ADC0_WCHSELB_CH8_POS                 (8U)                                                /*!< ADC0_WCHSELB_CH8 bit offset */

/* ADC0_WCHSELB_CH9 */
#define ADC0_WCHSELB_CH9_POS                 (9U)                                                /*!< ADC0_WCHSELB_CH9 bit offset */

/* ADC0_WCHSELB_CH10 */
#define ADC0_WCHSELB_CH10_POS                (10U)                                               /*!< ADC0_WCHSELB_CH10 bit offset */

/* ADC0_WCHSELB_CH11 */
#define ADC0_WCHSELB_CH11_POS                (11U)                                               /*!< ADC0_WCHSELB_CH11 bit offset */

/* ADC0_WCHSELB_CH12 */
#define ADC0_WCHSELB_CH12_POS                (12U)                                               /*!< ADC0_WCHSELB_CH12 bit offset */

/* ADC0_WCHSELB_CH13 */
#define ADC0_WCHSELB_CH13_POS                (13U)                                               /*!< ADC0_WCHSELB_CH13 bit offset */

/* ADC0_DMA_RBADRB_DMA_RBADRB */
#define ADC0_DMA_RBADRB_DMA_RBADRB_POS       (0U)                                                /*!< ADC0_DMA_RBADRB_DMA_RBADRB bit offset */

/* ADC0_DMA_RLENB_DMA_RLENB */
#define ADC0_DMA_RLENB_DMA_RLENB_POS         (0U)                                                /*!< ADC0_DMA_RLENB_DMA_RLENB bit offset */

/* ADC0_DMA_RADRB_DMA_RADRB */
#define ADC0_DMA_RADRB_DMA_RADRB_POS         (0U)                                                /*!< ADC0_DMA_RADRB_DMA_RADRB bit offset */

/* ADC0_IEB_EOCIE */
#define ADC0_IEB_EOCIE_POS                   (0U)                                                /*!< ADC0_IEB_EOCIE bit offset */

/* ADC0_IEB_EOCSIE */
#define ADC0_IEB_EOCSIE_POS                  (1U)                                                /*!< ADC0_IEB_EOCSIE bit offset */

/* ADC0_IEB_AWDIE */
#define ADC0_IEB_AWDIE_POS                   (2U)                                                /*!< ADC0_IEB_AWDIE bit offset */

/* ADC0_IEB_DMA_HIE */
#define ADC0_IEB_DMA_HIE_POS                 (3U)                                                /*!< ADC0_IEB_DMA_HIE bit offset */

/* ADC0_IEB_DMA_FIE */
#define ADC0_IEB_DMA_FIE_POS                 (4U)                                                /*!< ADC0_IEB_DMA_FIE bit offset */

/* ADC0_IEB_DMA_ERRIE */
#define ADC0_IEB_DMA_ERRIE_POS               (5U)                                                /*!< ADC0_IEB_DMA_ERRIE bit offset */

/* ADC0_IFB_EOC */
#define ADC0_IFB_EOC_POS                     (0U)                                                /*!< ADC0_IFB_EOC bit offset */

/* ADC0_IFB_EOCS */
#define ADC0_IFB_EOCS_POS                    (1U)                                                /*!< ADC0_IFB_EOCS bit offset */

/* ADC0_IFB_AWD */
#define ADC0_IFB_AWD_POS                     (2U)                                                /*!< ADC0_IFB_AWD bit offset */

/* ADC0_IFB_DMA_HDONE */
#define ADC0_IFB_DMA_HDONE_POS               (3U)                                                /*!< ADC0_IFB_DMA_HDONE bit offset */

/* ADC0_IFB_DMA_FDONE */
#define ADC0_IFB_DMA_FDONE_POS               (4U)                                                /*!< ADC0_IFB_DMA_FDONE bit offset */

/* ADC0_IFB_DMA_ERR */
#define ADC0_IFB_DMA_ERR_POS                 (5U)                                                /*!< ADC0_IFB_DMA_ERR bit offset */

/* ADC0_CAL_ZTG */
#define ADC0_CAL_ZTG_POS                     (0U)                                                /*!< ADC0_CAL_ZTG bit offset */

/* ADC0_CAL_ZTBG */
#define ADC0_CAL_ZTBG_POS                    (12U)                                               /*!< ADC0_CAL_ZTBG bit offset */

/* ADC0_CAL_CAL_EN */
#define ADC0_CAL_CAL_EN_POS                  (24U)                                               /*!< ADC0_CAL_CAL_EN bit offset */

/* ADC0_DOS_DOS */
#define ADC0_DOS_DOS_POS                     (0U)                                                /*!< ADC0_DOS_DOS bit offset */

/* ADC0_BGRLP_CORE */
#define ADC0_BGRLP_CORE_POS                  (0U)                                                /*!< ADC0_BGRLP_CORE bit offset */

/* ADC0_BGRLP_COREB */
#define ADC0_BGRLP_COREB_POS                 (16U)                                               /*!< ADC0_BGRLP_COREB bit offset */

/* ADC0_DCONV_DCONV */
#define ADC0_DCONV_DCONV_POS                 (0U)                                                /*!< ADC0_DCONV_DCONV bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_ADC0_H */
