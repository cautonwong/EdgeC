/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_dac.h
 * @author  Renergy Software Team
 * @brief   DAC module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_DAC_H
#define _RN831x_RN861x_MCU_DAC_H

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

/* DAC */
typedef struct
{
    __IO uint32_t CFG;                       /*!< offset: 0x000 info: DAC Control Register */
    __IO uint32_t ADPOT;                     /*!< offset: 0x004 info: DAC A/D Conversion Priority Timeout Register */
    __IO uint32_t SHT;                       /*!< offset: 0x008 info: DAC Sample Hold Time Register */
    __IO uint32_t OTRIM;                     /*!< offset: 0x00C info: DAC Buffer Calibration Register */
    __IO uint32_t SWT;                       /*!< offset: 0x010 info: DAC Software Trigger Register */
    __IO uint32_t IE;                        /*!< offset: 0x014 info: DAC Interrupt Enable Register */
    __IO uint32_t IF;                        /*!< offset: 0x018 info: DAC Interrupt Flag Register */
    __IO uint32_t RESERVED0;                 /*!< offset: 0x01C */
    __IO uint32_t DHOLD;                     /*!< offset: 0x020 info: DAC Data Hold Register */
    __IO uint32_t DOUT;                      /*!< offset: 0x024 info: DAC Data Output Register */
} DAC_TypeDef;

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
#define DAC_CORE0_BASE  (0x20098000U)
#define DAC_CORE1_BASE  (0xA0098000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define DAC_CORE0  ((DAC_TypeDef *)      DAC_CORE0_BASE)
#define DAC_CORE1  ((DAC_TypeDef *)      DAC_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define DAC  DAC_CORE0
#endif
#if defined(CORE1)
#define DAC  DAC_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** DAC Instances *******************************/
#define IS_DAC_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == DAC_CORE0) || \
                                          ((INSTANCE) == DAC_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* DAC_CFG */
#define DAC_CFG_DAC_EN                       (BIT(0))                                            /*!< DAC channel conversion enable bit. */
#define DAC_CFG_BUFF_EN                      (BIT(1))                                            /*!< Output buffer enable bit. */
#define DAC_CFG_SEL_BUFF                     (BIT(2))                                            /*!< Output buffer selection enable bit. */
#define DAC_CFG_INIT_MODE                    (BIT(3))                                            /*!< Initial output state selection bit. */
#define DAC_CFG_DMA_EN                       (BIT(4))                                            /*!< DMA enable bit (when DMA enable,must config TRIG_CTRL as External trigger). */
#define DAC_CFG_TRIG_CTRL                    (BITS(5,6))                                         /*!< DAC trigger mode configuration bits. */
#define DAC_CFG_SH_MODE                      (BIT(7))                                            /*!< Sample and hold mode enable bit. */
#define DAC_CFG_DPSEL                        (BIT(8))                                            /*!< Data alignment format configuration bit. */
#define DAC_CFG_ADPEN                        (BIT(9))                                            /*!< A/D conversion priority function enable bit. */
#define DAC_CFG_ADP_CTRL                     (BIT(10))                                           /*!< Timeout handling method seletion bit, when A/D conversion priority function is enabled */
#define DAC_CFG_TBUFF                        (BITS(11,12))                                       /*!< Configuration bits for output buffer power-up stabilization wait time(in conversion clock cycles Toclk). */
#define DAC_CFG_OCH_EN                       (BIT(13))                                           /*!< Analog output channel enable bit. */
#define DAC_CFG_OCH_SEL                      (BITS(14,15))                                       /*!< Analog output channel selection bits. */
#define DAC_CFG_CONV_DIV                     (BITS(16,31))                                       /*!< Output conversion clock division configuration bits.Toclk=Tsys_clk/(CONV_DIV+1) (refer to user mannual for recommend value ) */

/* DAC_ADPOT */
#define DAC_ADPOT_ADP_OVT                    (BITS(0,15))                                        /*!< A/D conversion priority function timeout configuration bits.(in conversion clock cycles Toclk) */
#define DAC_ADPOT_DAC_BUSYT                  (BITS(16,19))                                       /*!< DAC busy flag assertion time setting bits (in system clock cycles Tsys_clk) */

/* DAC_SHT */
#define DAC_SHT_TSMPL                        (BITS(0,15))                                        /*!< Sampling time configuration bits, for sample and hold mode. Sample Time = (TSMPL+1)*Toclk */
#define DAC_SHT_THOLD                        (BITS(16,31))                                       /*!< Hold time configuration bits, for sample and hold mode. Hold Time = (THOLD+1)*Toclk */

/* DAC_OTRIM */
#define DAC_OTRIM_OTRIM                      (BITS(0,7))                                         /*!< (two's complement)When output buffer is enabled, the compensation value for output voltage. */

/* DAC_SWT */
#define DAC_SWT_SWT                          (BIT(0))                                            /*!< D/A conversion software trigger bit. */

/* DAC_IE */
#define DAC_IE_DOUT_IE                       (BIT(0))                                            /*!< Data output register update interrupt enable bit. */
#define DAC_IE_DERR_IE                       (BIT(1))                                            /*!< DMA error interrupt enable bit. */
#define DAC_IE_ADPOV_IE                      (BIT(2))                                            /*!< A/D conversion wait timeout interrupt enable bit. */

/* DAC_IF */
#define DAC_IF_DOUT_IF                       (BIT(0))                                            /*!< Data output register update interrupt flag. */
#define DAC_IF_DERR_IF                       (BIT(1))                                            /*!< DMA error interrupt flag. */
#define DAC_IF_ADPOV_IF                      (BIT(2))                                            /*!< A/D conversion wait timeout interrupt flag. */

/* DAC_DHOLD */
#define DAC_DHOLD_DHOLD                      (BITS(0,15))                                        /*!< When DAC_CFG.DPSEL=0 (data right-aligned), write the data to be converted to DHOLD[11:0];When DAC_CFG.DPSEL=1 (data left-aligned), write the data to be converted to DHOLD[15:4]. */

/* DAC_DOUT */
#define DAC_DOUT_DOUT                        (BITS(0,11))                                        /*!< DAC Data Output bits */

/* constants definitions */
/* DAC_CFG_DAC_EN */
#define DAC_CFG_DAC_EN_POS                   (0U)                                                /*!< DAC_CFG_DAC_EN bit offset */

/* DAC_CFG_BUFF_EN */
#define DAC_CFG_BUFF_EN_POS                  (1U)                                                /*!< DAC_CFG_BUFF_EN bit offset */

/* DAC_CFG_SEL_BUFF */
#define DAC_CFG_SEL_BUFF_POS                 (2U)                                                /*!< DAC_CFG_SEL_BUFF bit offset */
#define DAC_CFG_SEL_BUFF_BUFF                ((uint32_t)0x00000000U)                             /*!< Output not through buffer; */
#define DAC_CFG_SEL_BUFF_NO_BUFF             (DAC_CFG_SEL_BUFF)                                  /*!< Output through buffer; */

/* DAC_CFG_INIT_MODE */
#define DAC_CFG_INIT_MODE_POS                (3U)                                                /*!< DAC_CFG_INIT_MODE bit offset */
#define DAC_CFG_INIT_MODE_HIGHZ              ((uint32_t)0x00000000U)                             /*!< Analog output in high-Z state when channel disabled; */
#define DAC_CFG_INIT_MODE_GND                (DAC_CFG_INIT_MODE)                                 /*!< Analog output connected to ground when channel disabled; */

/* DAC_CFG_DMA_EN */
#define DAC_CFG_DMA_EN_POS                   (4U)                                                /*!< DAC_CFG_DMA_EN bit offset */

/* DAC_CFG_TRIG_CTRL */
#define DAC_CFG_TRIG_CTRL_POS                (5U)                                                /*!< DAC_CFG_TRIG_CTRL bit offset */
#define DAC_CFG_TRIG_CTRL_CFG(regval)  (DAC_CFG_TRIG_CTRL & ((uint32_t)(regval) << DAC_CFG_TRIG_CTRL_POS))
#define DAC_CFG_TRIG_CTRL_COUTINUE           (DAC_CFG_TRIG_CTRL_CFG(0))                          /*!< Continuous output mode; */
#define DAC_CFG_TRIG_CTRL_SWT                (DAC_CFG_TRIG_CTRL_CFG(1))                          /*!< Software trigger, writing to DAC_SWT register triggers one conversion; */
#define DAC_CFG_TRIG_CTRL_EXT                (DAC_CFG_TRIG_CTRL_CFG(2))                          /*!< External trigger, external trigger source configured by AOS module; */

/* DAC_CFG_SH_MODE */
#define DAC_CFG_SH_MODE_POS                  (7U)                                                /*!< DAC_CFG_SH_MODE bit offset */
#define DAC_CFG_SH_MODE_NORM                 ((uint32_t)0x00000000U)                             /*!< Sample and hold mode disabled, Normal mode; */
#define DAC_CFG_SH_MODE_SPHD                 (DAC_CFG_SH_MODE)                                   /*!< Sample and hold mode enabled; */

/* DAC_CFG_DPSEL */
#define DAC_CFG_DPSEL_POS                    (8U)                                                /*!< DAC_CFG_DPSEL bit offset */
#define DAC_CFG_DPSEL_RA                     ((uint32_t)0x00000000U)                             /*!< Right-aligned data */
#define DAC_CFG_DPSEL_LA                     (DAC_CFG_DPSEL)                                     /*!< Left-aligned data */

/* DAC_CFG_ADPEN */
#define DAC_CFG_ADPEN_POS                    (9U)                                                /*!< DAC_CFG_ADPEN bit offset */

/* DAC_CFG_ADP_CTRL */
#define DAC_CFG_ADP_CTRL_POS                 (10U)                                               /*!< DAC_CFG_ADP_CTRL bit offset */
#define DAC_CFG_ADP_CTRL_WAITAD_DONE         ((uint32_t)0x00000000U)                             /*!< After timeout ADPOV Flag set as 1, continue waiting until A/D conversion completes, then proceed to next D/A conversion; */
#define DAC_CFG_ADP_CTRL_ONLYTIMEOUT         (DAC_CFG_ADP_CTRL)                                  /*!< After timeout ADPOV Flag set as 1, do not wait for A/D conversion completion, immediately proceed to next D/A conversion; */

/* DAC_CFG_TBUFF */
#define DAC_CFG_TBUFF_POS                    (11U)                                               /*!< DAC_CFG_TBUFF bit offset */
#define DAC_CFG_TBUFF_CFG(regval)  (DAC_CFG_TBUFF & ((uint32_t)(regval) << DAC_CFG_TBUFF_POS))
#define DAC_CFG_TBUFF_1TOCLK                 (DAC_CFG_TBUFF_CFG(0))                              /*!< 1Toclk */
#define DAC_CFG_TBUFF_2TOCLK                 (DAC_CFG_TBUFF_CFG(1))                              /*!< 2Toclk (default) */
#define DAC_CFG_TBUFF_3TOCLK                 (DAC_CFG_TBUFF_CFG(2))                              /*!< 3Toclk */
#define DAC_CFG_TBUFF_4TOCLK                 (DAC_CFG_TBUFF_CFG(3))                              /*!< 4Toclk */

/* DAC_CFG_OCH_EN */
#define DAC_CFG_OCH_EN_POS                   (13U)                                               /*!< DAC_CFG_OCH_EN bit offset */
#define DAC_CFG_OCH_EN_EN                    ((uint32_t)0x00000000U)                             /*!< All output channels are closed; */
#define DAC_CFG_OCH_EN_DIS                   (DAC_CFG_OCH_EN)                                    /*!< Select DAC output to PAD or ADC0  by OCH_SEL; */

/* DAC_CFG_OCH_SEL */
#define DAC_CFG_OCH_SEL_POS                  (14U)                                               /*!< DAC_CFG_OCH_SEL bit offset */
#define DAC_CFG_OCH_SEL_CFG(regval)  (DAC_CFG_OCH_SEL & ((uint32_t)(regval) << DAC_CFG_OCH_SEL_POS))
#define DAC_CFG_OCH_SEL_PAD0                 (DAC_CFG_OCH_SEL_CFG(0))                            /*!< Output to PAD0 (P104) */
#define DAC_CFG_OCH_SEL_PAD1                 (DAC_CFG_OCH_SEL_CFG(1))                            /*!< Output to PAD1 (P140) */
#define DAC_CFG_OCH_SEL_ADC0                 (DAC_CFG_OCH_SEL_CFG(2))                            /*!< Output to ADC0 */

/* DAC_CFG_CONV_DIV */
#define DAC_CFG_CONV_DIV_POS                 (16U)                                               /*!< DAC_CFG_CONV_DIV bit offset */

/* DAC_ADPOT_ADP_OVT */
#define DAC_ADPOT_ADP_OVT_POS                (0U)                                                /*!< DAC_ADPOT_ADP_OVT bit offset */

/* DAC_ADPOT_DAC_BUSYT */
#define DAC_ADPOT_DAC_BUSYT_POS              (16U)                                               /*!< DAC_ADPOT_DAC_BUSYT bit offset */

/* DAC_SHT_TSMPL */
#define DAC_SHT_TSMPL_POS                    (0U)                                                /*!< DAC_SHT_TSMPL bit offset */

/* DAC_SHT_THOLD */
#define DAC_SHT_THOLD_POS                    (16U)                                               /*!< DAC_SHT_THOLD bit offset */

/* DAC_OTRIM_OTRIM */
#define DAC_OTRIM_OTRIM_POS                  (0U)                                                /*!< DAC_OTRIM_OTRIM bit offset */

/* DAC_SWT_SWT */
#define DAC_SWT_SWT_POS                      (0U)                                                /*!< DAC_SWT_SWT bit offset */

/* DAC_IE_DOUT_IE */
#define DAC_IE_DOUT_IE_POS                   (0U)                                                /*!< DAC_IE_DOUT_IE bit offset */

/* DAC_IE_DERR_IE */
#define DAC_IE_DERR_IE_POS                   (1U)                                                /*!< DAC_IE_DERR_IE bit offset */

/* DAC_IE_ADPOV_IE */
#define DAC_IE_ADPOV_IE_POS                  (2U)                                                /*!< DAC_IE_ADPOV_IE bit offset */

/* DAC_IF_DOUT_IF */
#define DAC_IF_DOUT_IF_POS                   (0U)                                                /*!< DAC_IF_DOUT_IF bit offset */

/* DAC_IF_DERR_IF */
#define DAC_IF_DERR_IF_POS                   (1U)                                                /*!< DAC_IF_DERR_IF bit offset */

/* DAC_IF_ADPOV_IF */
#define DAC_IF_ADPOV_IF_POS                  (2U)                                                /*!< DAC_IF_ADPOV_IF bit offset */

/* DAC_DHOLD_DHOLD */
#define DAC_DHOLD_DHOLD_POS                  (0U)                                                /*!< DAC_DHOLD_DHOLD bit offset */

/* DAC_DOUT_DOUT */
#define DAC_DOUT_DOUT_POS                    (0U)                                                /*!< DAC_DOUT_DOUT bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_DAC_H */
