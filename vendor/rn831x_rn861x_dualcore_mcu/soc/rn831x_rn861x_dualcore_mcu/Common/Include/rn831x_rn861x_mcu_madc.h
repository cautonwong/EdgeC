/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_madc.h
 * @author  Renergy Software Team
 * @brief   MADC module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_MADC_H
#define _RN831x_RN861x_MCU_MADC_H

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

/* MADC */
typedef struct
{
    __IO uint32_t AD_CTRL;                   /*!< offset: 0x000 info: SAR ADC control register */
    __IO uint32_t AD_START;                  /*!< offset: 0x004 info: SAR ADC start register */
    __IO uint32_t AD_STAT;                   /*!< offset: 0x008 info: SAR ADC status register */
    __IO uint32_t AD_DATA;                   /*!< offset: 0x00C info: SAR ADC data register */
    __IO uint32_t RESERVED0[2];              /*!< offset: 0x010 ~ 014 */
    __IO uint32_t AD_CTRL1;                  /*!< offset: 0x018 info: SAR ADC control register No.1 */
    __IO uint32_t AD_DATA2;                  /*!< offset: 0x01C info: SAR ADC data register 2 */
    __IO uint32_t AD_CTRL3;                  /*!< offset: 0x020 info: SAR ADC control register No.3 */
    __IO uint32_t AD_DATA3;                  /*!< offset: 0x024 info: SAR ADC data register 3 */
    __IO uint32_t ANA_RST_STAT;              /*!< offset: 0x028 info: ANA reset state register */
    __IO uint32_t RESERVED1[4];              /*!< offset: 0x02C ~ 038 */
    __IO uint32_t ANA_PAD;                   /*!< offset: 0x03C info: ANA_PAD register */
    __IO uint32_t ANA_RST;                   /*!< offset: 0x040 info: VBAT soft reset register */
} MADC_TypeDef;

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
#define MADC_CORE0_BASE (0x200B8000U)
#define MADC_CORE1_BASE (0xA00B8000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define MADC_CORE0 ((MADC_TypeDef *)     MADC_CORE0_BASE)
#define MADC_CORE1 ((MADC_TypeDef *)     MADC_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define MADC  MADC_CORE0
#endif
#if defined(CORE1)
#define MADC  MADC_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** MADC Instances *******************************/
#define IS_MADC_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == MADC_CORE0) || \
                                           ((INSTANCE) == MADC_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* MADC_AD_CTRL */
#define MADC_AD_CTRL_SAR_CH                  (BITS(0,2))                                         /*!< SAR ADC channel selection bits(part1) */
#define MADC_AD_CTRL_SAR_PGA                 (BITS(3,4))                                         /*!< SAR ADC PGA selection bits(old configuration used for compatibility) */
#define MADC_AD_CTRL_SAR_IE                  (BIT(5))                                            /*!< SAR ADC interrupt enable bit */
#define MADC_AD_CTRL_SAR_CH3                 (BIT(6))                                            /*!< SAR ADC channel selection bits(part2) */
#define MADC_AD_CTRL_SAR_WAIT                (BITS(7,11))                                        /*!< set SAR ADC duration config bits((SAR_WAIT+1)*30.5us) between enable and sample */
#define MADC_AD_CTRL_REF_WAIT                (BITS(12,16))                                       /*!< set voltage reference duration config bits((REF_WAIT+1)*30.5us) between REF enable and ADC enable */

/* MADC_AD_START */
#define MADC_AD_START_AD_START               (BIT(0))                                            /*!< SAR ADC start control bit, =1 start sar-measure once */

/* MADC_AD_STAT */
#define MADC_AD_STAT_DREADY                  (BIT(0))                                            /*!< SAR ADC data ready/pending flag */
#define MADC_AD_STAT_TPS_BUSY                (BIT(1))                                            /*!< automatic temperature measurement busy flag */

/* MADC_AD_DATA */
#define MADC_AD_DATA_AD_DATA                 (BITS(0,9))                                         /*!< SAR ADC result data bits */

/* MADC_AD_CTRL1 */
#define MADC_AD_CTRL1_SAR_PGA                (BITS(0,2))                                         /*!< SAR ADC PGA config bits */
#define MADC_AD_CTRL1_SAR_PGA_SEL            (BIT(3))                                            /*!< SAR ADC PGA selection bits */
#define MADC_AD_CTRL1_SAR_CONVERT            (BITS(8,14))                                        /*!< SAR ADC simple to DOUT wait time config bits */

/* MADC_AD_DATA2 */
#define MADC_AD_DATA2_AD_DATA2               (BITS(0,11))                                        /*!< SAR ADC 12 bit result data bits */

/* MADC_AD_CTRL3 */
#define MADC_AD_CTRL3_ACC_CNT                (BITS(0,7))                                         /*!< SAR ADC output data accumulation count configuration(for AD_DATA3) */

/* MADC_AD_DATA3 */
#define MADC_AD_DATA3_AD_DATA3               (BITS(0,15))                                        /*!< SAR ADC 16 bit result data bits */

/* MADC_ANA_RST_STAT */
#define MADC_ANA_RST_STAT_PORIF              (BIT(0))                                            /*!< POR reset interrupt flag(POR reset interrupt is a non-maskable interrupt) */
#define MADC_ANA_RST_STAT_BORIF              (BIT(1))                                            /*!< BOR reset interrupt flag */
#define MADC_ANA_RST_STAT_BORIE              (BIT(2))                                            /*!< BOR reset interrupt enable bit */

/* MADC_ANA_PAD */
#define MADC_ANA_PAD_PL01_AE                 (BIT(0))                                            /*!< Enable bit for SAR AIN5 analog input */
#define MADC_ANA_PAD_PL02_AE                 (BIT(1))                                            /*!< Enable bit for SAR AIN6 analog input */
#define MADC_ANA_PAD_PL03_AE                 (BIT(2))                                            /*!< Enable bit for SAR AIN7 analog input */

/* MADC_ANA_RST */
#define MADC_ANA_RST_ANA_RST                 (BITS(0,7))                                         /*!< Reset all resetable register of VBAT */
#define MADC_ANA_RST_PS                      (BITS(8,15))                                        /*!< password cfg bits of ANA_RST */

/* constants definitions */
/* MADC_AD_CTRL_SAR_CH */
#define MADC_AD_CTRL_SAR_CH_POS              (0U)                                                /*!< MADC_AD_CTRL_SAR_CH bit offset */

/* MADC_AD_CTRL_SAR_PGA */
#define MADC_AD_CTRL_SAR_PGA_POS             (3U)                                                /*!< MADC_AD_CTRL_SAR_PGA bit offset */

/* MADC_AD_CTRL_SAR_IE */
#define MADC_AD_CTRL_SAR_IE_POS              (5U)                                                /*!< MADC_AD_CTRL_SAR_IE bit offset */

/* MADC_AD_CTRL_SAR_CH3 */
#define MADC_AD_CTRL_SAR_CH3_POS             (6U)                                                /*!< MADC_AD_CTRL_SAR_CH3 bit offset */

/* MADC_AD_CTRL_SAR_WAIT */
#define MADC_AD_CTRL_SAR_WAIT_POS            (7U)                                                /*!< MADC_AD_CTRL_SAR_WAIT bit offset */

/* MADC_AD_CTRL_REF_WAIT */
#define MADC_AD_CTRL_REF_WAIT_POS            (12U)                                               /*!< MADC_AD_CTRL_REF_WAIT bit offset */

/* MADC_AD_START_AD_START */
#define MADC_AD_START_AD_START_POS           (0U)                                                /*!< MADC_AD_START_AD_START bit offset */

/* MADC_AD_STAT_DREADY */
#define MADC_AD_STAT_DREADY_POS              (0U)                                                /*!< MADC_AD_STAT_DREADY bit offset */
#define MADC_AD_STAT_DREADY_PEND             ((uint32_t)0x00000000U)                             /*!< SAR ADC data pending flag */
#define MADC_AD_STAT_DREADY_READY            (MADC_AD_STAT_DREADY)                               /*!< SAR ADC data ready flag */

/* MADC_AD_STAT_TPS_BUSY */
#define MADC_AD_STAT_TPS_BUSY_POS            (1U)                                                /*!< MADC_AD_STAT_TPS_BUSY bit offset */
#define MADC_AD_STAT_TPS_BUSY_IDLE           ((uint32_t)0x00000000U)                             /*!< no outstanding automatic temperature measurement */
#define MADC_AD_STAT_TPS_BUSY_BUSY           (MADC_AD_STAT_TPS_BUSY)                             /*!< pending automatic temperature measurement */

/* MADC_AD_DATA_AD_DATA */
#define MADC_AD_DATA_AD_DATA_POS             (0U)                                                /*!< MADC_AD_DATA_AD_DATA bit offset */

/* MADC_AD_CTRL1_SAR_PGA */
#define MADC_AD_CTRL1_SAR_PGA_POS            (0U)                                                /*!< MADC_AD_CTRL1_SAR_PGA bit offset */
#define MADC_AD_CTRL1_SAR_PGA_CFG(regval)  (MADC_AD_CTRL1_SAR_PGA & ((uint32_t)(regval) << MADC_AD_CTRL1_SAR_PGA_POS))
#define MADC_AD_CTRL1_SAR_PGA_0P5            (MADC_AD_CTRL1_SAR_PGA_CFG(0))                      /*!< PGA select as 0.5 */
#define MADC_AD_CTRL1_SAR_PGA_1P0            (MADC_AD_CTRL1_SAR_PGA_CFG(1))                      /*!< PGA select as 1.0 */
#define MADC_AD_CTRL1_SAR_PGA_1P5            (MADC_AD_CTRL1_SAR_PGA_CFG(2))                      /*!< PGA select as 1.5 */
#define MADC_AD_CTRL1_SAR_PGA_2P0            (MADC_AD_CTRL1_SAR_PGA_CFG(3))                      /*!< PGA select as 2.0 */
#define MADC_AD_CTRL1_SAR_PGA_0P25           (MADC_AD_CTRL1_SAR_PGA_CFG(4))                      /*!< PGA select as 0.25 */

/* MADC_AD_CTRL1_SAR_PGA_SEL */
#define MADC_AD_CTRL1_SAR_PGA_SEL_POS        (3U)                                                /*!< MADC_AD_CTRL1_SAR_PGA_SEL bit offset */
#define MADC_AD_CTRL1_SAR_PGA_SEL_CTRL_PGA   ((uint32_t)0x00000000U)                             /*!< select AD_CTRL[4:3] PGA as PGA */
#define MADC_AD_CTRL1_SAR_PGA_SEL_CTRL1_PGA  (MADC_AD_CTRL1_SAR_PGA_SEL)                         /*!< select AD_CTRL1[2:0] PGA as PGA */

/* MADC_AD_CTRL1_SAR_CONVERT */
#define MADC_AD_CTRL1_SAR_CONVERT_POS        (8U)                                                /*!< MADC_AD_CTRL1_SAR_CONVERT bit offset */

/* MADC_AD_DATA2_AD_DATA2 */
#define MADC_AD_DATA2_AD_DATA2_POS           (0U)                                                /*!< MADC_AD_DATA2_AD_DATA2 bit offset */

/* MADC_AD_CTRL3_ACC_CNT */
#define MADC_AD_CTRL3_ACC_CNT_POS            (0U)                                                /*!< MADC_AD_CTRL3_ACC_CNT bit offset */

/* MADC_AD_DATA3_AD_DATA3 */
#define MADC_AD_DATA3_AD_DATA3_POS           (0U)                                                /*!< MADC_AD_DATA3_AD_DATA3 bit offset */

/* MADC_ANA_RST_STAT_PORIF */
#define MADC_ANA_RST_STAT_PORIF_POS          (0U)                                                /*!< MADC_ANA_RST_STAT_PORIF bit offset */

/* MADC_ANA_RST_STAT_BORIF */
#define MADC_ANA_RST_STAT_BORIF_POS          (1U)                                                /*!< MADC_ANA_RST_STAT_BORIF bit offset */

/* MADC_ANA_RST_STAT_BORIE */
#define MADC_ANA_RST_STAT_BORIE_POS          (2U)                                                /*!< MADC_ANA_RST_STAT_BORIE bit offset */

/* MADC_ANA_PAD_PL01_AE */
#define MADC_ANA_PAD_PL01_AE_POS             (0U)                                                /*!< MADC_ANA_PAD_PL01_AE bit offset */
#define MADC_ANA_PAD_PL01_AE_DIS             ((uint32_t)0x00000000U)                             /*!< disable analog input */
#define MADC_ANA_PAD_PL01_AE_EN              (MADC_ANA_PAD_PL01_AE)                              /*!< enable analog input */

/* MADC_ANA_PAD_PL02_AE */
#define MADC_ANA_PAD_PL02_AE_POS             (1U)                                                /*!< MADC_ANA_PAD_PL02_AE bit offset */
#define MADC_ANA_PAD_PL02_AE_DIS             ((uint32_t)0x00000000U)                             /*!< disable analog input */
#define MADC_ANA_PAD_PL02_AE_EN              (MADC_ANA_PAD_PL02_AE)                              /*!< enable analog input */

/* MADC_ANA_PAD_PL03_AE */
#define MADC_ANA_PAD_PL03_AE_POS             (2U)                                                /*!< MADC_ANA_PAD_PL03_AE bit offset */
#define MADC_ANA_PAD_PL03_AE_DIS             ((uint32_t)0x00000000U)                             /*!< disable analog input */
#define MADC_ANA_PAD_PL03_AE_EN              (MADC_ANA_PAD_PL03_AE)                              /*!< enable analog input */

/* MADC_ANA_RST_ANA_RST */
#define MADC_ANA_RST_ANA_RST_POS             (0U)                                                /*!< MADC_ANA_RST_ANA_RST bit offset */
#define MADC_ANA_RST_ANA_RST_RST             ((uint32_t)0x000000A8U)                             /*!< Reset all resetable register of VBAT */
#define MADC_ANA_RST_ANA_RST_NUL             ((uint32_t)0x00000000U)                             /*!< Do not reset */

/* MADC_ANA_RST_PS */
#define MADC_ANA_RST_PS_POS                  (8U)                                                /*!< MADC_ANA_RST_PS bit offset */
#define MADC_ANA_RST_PS_PS                   ((uint32_t)0x0000A500U)                             /*!< password of ANA_RST */
#define MADC_ANA_RST_PS_DIS                  ((uint32_t)0x00000000U)                             /*!< disable [7:0]write */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_MADC_H */
