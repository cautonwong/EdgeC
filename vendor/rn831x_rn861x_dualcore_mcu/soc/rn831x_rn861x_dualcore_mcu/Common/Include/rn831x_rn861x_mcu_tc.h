/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_tc.h
 * @author  Renergy Software Team
 * @brief   TC module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_TC_H
#define _RN831x_RN861x_MCU_TC_H

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

/* TC */
typedef struct
{
    __IO uint32_t CNT;                       /*!< offset: 0x000 info: current counting value register */
    __IO uint32_t PS;                        /*!< offset: 0x004 info: prescaler value register of the counter clock */
    __IO uint32_t RESERVED0;                 /*!< offset: 0x008 */
    __IO uint32_t DN;                        /*!< offset: 0x00C info: target counting value register */
    __IO uint32_t RESERVED1;                 /*!< offset: 0x010 */
    __IO uint32_t CCD0;                      /*!< offset: 0x014 info: capture(compare) channel No.0 data register */
    __IO uint32_t CCD1;                      /*!< offset: 0x018 info: capture(compare) channel No.1 data register */
    __IO uint32_t CCFG;                      /*!< offset: 0x01C info: clock config register */
    __IO uint32_t CTRL;                      /*!< offset: 0x020 info: control register */
    __IO uint32_t CM0;                       /*!< offset: 0x024 info: capture(compare) channel No.0 mode register */
    __IO uint32_t CM1;                       /*!< offset: 0x028 info: capture(compare) channel No.1 mode register */
    __IO uint32_t IE;                        /*!< offset: 0x02C info: Interrupt enable register */
    __IO uint32_t STA;                       /*!< offset: 0x030 info: status register */
    __IO uint32_t PWM_CFG;                   /*!< offset: 0x034 info: PWM config register */
    __IO uint32_t PWM_CTRL;                  /*!< offset: 0x038 info: PWM control register */
    __IO uint32_t PWM_STA;                   /*!< offset: 0x03C info: PWM status register */
    __IO uint32_t PWM_CNT;                   /*!< offset: 0x040 info: PWM current counting value register */
    __IO uint32_t PWM_DMA_BADR;              /*!< offset: 0x044 info: PWM DMA base address register */
    __IO uint32_t PWM_DMA_LEN;               /*!< offset: 0x048 info: PWM DMA length register */
    __IO uint32_t PWM_DMA_ADR;               /*!< offset: 0x04C info: PWM DMA current address register */
} TC_TypeDef;

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
#define TC0_CORE0_BASE  (0x2009C000U)
#define TC1_CORE0_BASE  (0x200A0000U)
#define TC0_CORE1_BASE  (0xA009C000U)
#define TC1_CORE1_BASE  (0xA00A0000U)
#define TC0_CORE0_BB_BASE (0x23380000U)
#define TC1_CORE0_BB_BASE (0x23400000U)
#define TC0_CORE1_BB_BASE (0xA3380000U)
#define TC1_CORE1_BB_BASE (0xA3400000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define TC0_CORE0  ((TC_TypeDef *)       TC0_CORE0_BASE)
#define TC1_CORE0  ((TC_TypeDef *)       TC1_CORE0_BASE)
#define TC0_CORE1  ((TC_TypeDef *)       TC0_CORE1_BASE)
#define TC1_CORE1  ((TC_TypeDef *)       TC1_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define TC0  TC0_CORE0
#define TC1  TC1_CORE0
#endif
#if defined(CORE1)
#define TC0  TC0_CORE1
#define TC1  TC1_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** TC Instances *******************************/
#define IS_TC_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == TC0_CORE0) || \
                                         ((INSTANCE) == TC1_CORE0) || \
                                         ((INSTANCE) == TC0_CORE1) || \
                                         ((INSTANCE) == TC1_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* TC_CNT */
#define TC_CNT_CNT                           (BITS(0,31))                                        /*!< current Timer counter value bits */

/* TC_PS */
#define TC_PS_PS                             (BITS(0,15))                                        /*!< prescaler value bits of the counter clock */

/* TC_DN */
#define TC_DN_DN                             (BITS(0,31))                                        /*!< target counting value bits */

/* TC_CCD0 */
#define TC_CCD0_CCD0                         (BITS(0,31))                                        /*!< capture(compare) channel No.0 data bits */

/* TC_CCD1 */
#define TC_CCD1_CCD1                         (BITS(0,31))                                        /*!< capture(compare) channel No.1 data bits */

/* TC_CCFG */
#define TC_CCFG_CM                           (BIT(0))                                            /*!< clock mode selection bit */
#define TC_CCFG_FLTEN                        (BIT(1))                                            /*!< external clock source filter enable bit */
#define TC_CCFG_ECLKMODE                     (BITS(2,3))                                         /*!< external clock mode selection bits */
#define TC_CCFG_FLTOPT                       (BITS(4,11))                                        /*!< external clock source filter clock cycles set bits */

/* TC_CTRL */
#define TC_CTRL_START                        (BIT(0))                                            /*!< timer start bit */
#define TC_CTRL_SLVEN                        (BIT(2))                                            /*!< slave mode enable */
#define TC_CTRL_SLVFLTEN                     (BIT(3))                                            /*!< slave mode input filter enable */
#define TC_CTRL_OPS                          (BIT(4))                                            /*!< one pulse mode selection bit */
#define TC_CTRL_SLVTRGMODE                   (BITS(5,6))                                         /*!< slave mode polarity control selection bits */
#define TC_CTRL_SLVFLTOPT                    (BITS(7,14))                                        /*!< slave mode input filter parameter configuration bits */
#define TC_CTRL_SLVGATELVL                   (BIT(16))                                           /*!< slave mode gating level control bit */
#define TC_CTRL_DBGSTBDIS                    (BIT(17))                                           /*!< debugger halt standby enable/disable bit */

/* TC_CM0 */
#define TC_CM0_ENABLE                        (BIT(0))                                            /*!< channel enable bit */
#define TC_CM0_CCM                           (BIT(1))                                            /*!< capture(compare) selection bit */
#define TC_CM0_DIEN                          (BIT(2))                                            /*!< compare dead zone insertion enable bit (only for PWM1/PWM2) */
#define TC_CM0_DL                            (BITS(3,8))                                         /*!< compare dead zone length configuration bits (only for PWM1/PWM2) */
#define TC_CM0_CPOL                          (BITS(9,10))                                        /*!< capture polarity selection bits */
#define TC_CM0_FLTOPT                        (BITS(11,18))                                       /*!< capture filter parameter set bits */
#define TC_CM0_FLTEN                         (BIT(20))                                           /*!< capture filter enable bit */
#define TC_CM0_OM                            (BITS(21,23))                                       /*!< compare output mode cfg bits */
#define TC_CM0_EFELVL                        (BIT(24))                                           /*!< compare valid output level set bit */
#define TC_CM0_DFTLVL                        (BIT(25))                                           /*!< compare valid default level set bit */

/* TC_CM1 */
#define TC_CM1_ENABLE                        (BIT(0))                                            /*!< channel enable bit */
#define TC_CM1_CCM                           (BIT(1))                                            /*!< capture(compare) selection bit */
#define TC_CM1_DIEN                          (BIT(2))                                            /*!< compare dead zone insertion enable bit (only for PWM1/PWM2) */
#define TC_CM1_DL                            (BITS(3,8))                                         /*!< compare dead zone length configuration bits (only for PWM1/PWM2) */
#define TC_CM1_CPOL                          (BITS(9,10))                                        /*!< capture polarity selection bits */
#define TC_CM1_FLTOPT                        (BITS(11,18))                                       /*!< capture filter parameter set bits */
#define TC_CM1_FLTEN                         (BIT(20))                                           /*!< capture filter enable bit */
#define TC_CM1_OM                            (BITS(21,23))                                       /*!< compare output mode cfg bits */
#define TC_CM1_EFELVL                        (BIT(24))                                           /*!< compare valid output level set bit */
#define TC_CM1_DFTLVL                        (BIT(25))                                           /*!< compare valid default level set bit */

/* TC_IE */
#define TC_IE_OVIE                           (BIT(0))                                            /*!< overflow interrupt enable */
#define TC_IE_CC0IE                          (BIT(1))                                            /*!< compare(caputure) channel No.0 interrupt enable */
#define TC_IE_CC1IE                          (BIT(2))                                            /*!< compare(caputure) channel No.1 interrupt enable */
#define TC_IE_SLVIE                          (BIT(3))                                            /*!< slave interrupt enable */

/* TC_STA */
#define TC_STA_OVF                           (BIT(0))                                            /*!< overflow interrupt flag */
#define TC_STA_CC0F                          (BIT(1))                                            /*!< compare(capture) channel No.0 flag */
#define TC_STA_CC1F                          (BIT(2))                                            /*!< compare(capture) channel No.1 flag */
#define TC_STA_SLVF                          (BIT(3))                                            /*!< slave event flag */

/* TC_PWM_CFG */
#define TC_PWM_CFG_DONEIE                    (BIT(0))                                            /*!< PWM output done interrupt enable bit */
#define TC_PWM_CFG_DATERRIE                  (BIT(1))                                            /*!< data error interrupt enable bit */
#define TC_PWM_CFG_BUFVLDIE                  (BIT(2))                                            /*!< PWM buffer empty interrupt enable bit (only soft mode available) */
#define TC_PWM_CFG_CYCDONEIE                 (BIT(3))                                            /*!< output cycle done interrupt enable bit */
#define TC_PWM_CFG_CMS                       (BIT(4))                                            /*!< PWM control mode selection bit */
#define TC_PWM_CFG_CSS                       (BIT(5))                                            /*!< PWM complete signal selection bit */
#define TC_PWM_CFG_ORV                       (BIT(6))                                            /*!< PWM output remain period mode selection bit */
#define TC_PWM_CFG_WL                        (BITS(7,16))                                        /*!< PWM wave length config bits (available when ORV=0) */

/* TC_PWM_CTRL */
#define TC_PWM_CTRL_EN                       (BIT(0))                                            /*!< PWM enable bit */

/* TC_PWM_STA */
#define TC_PWM_STA_BUSY                      (BIT(0))                                            /*!< PWM output busy flag */
#define TC_PWM_STA_DONE                      (BIT(1))                                            /*!< PWM output done interrupt flag */
#define TC_PWM_STA_DATERR                    (BIT(2))                                            /*!< PWM data error interrupt flag */
#define TC_PWM_STA_BUFVLD                    (BIT(3))                                            /*!< PWM buffer empty interrupt flag */
#define TC_PWM_STA_CYCDONE                   (BIT(4))                                            /*!< PWM output cycle done interrupt flag */

/* TC_PWM_CNT */
#define TC_PWM_CNT_N_PWM                     (BITS(0,15))                                        /*!< PWM cycle Timer counter value bits */

/* TC_PWM_DMA_BADR */
#define TC_PWM_DMA_BADR_BADR                 (BITS(0,15))                                        /*!< DMA begin addr configuration bits */

/* TC_PWM_DMA_LEN */
#define TC_PWM_DMA_LEN_LEN                   (BITS(0,15))                                        /*!< DMA length configuration bits */

/* TC_PWM_DMA_ADR */
#define TC_PWM_DMA_ADR_ADR                   (BITS(0,15))                                        /*!< current DMA addr bits */

/* constants definitions */
/* TC_CNT_CNT */
#define TC_CNT_CNT_POS                       (0U)                                                /*!< TC_CNT_CNT bit offset */

/* TC_PS_PS */
#define TC_PS_PS_POS                         (0U)                                                /*!< TC_PS_PS bit offset */

/* TC_DN_DN */
#define TC_DN_DN_POS                         (0U)                                                /*!< TC_DN_DN bit offset */

/* TC_CCD0_CCD0 */
#define TC_CCD0_CCD0_POS                     (0U)                                                /*!< TC_CCD0_CCD0 bit offset */

/* TC_CCD1_CCD1 */
#define TC_CCD1_CCD1_POS                     (0U)                                                /*!< TC_CCD1_CCD1 bit offset */

/* TC_CCFG_CM */
#define TC_CCFG_CM_POS                       (0U)                                                /*!< TC_CCFG_CM bit offset */
#define TC_CCFG_CM_INT                       ((uint32_t)0x00000000U)                             /*!< using on-chip system clock as input counting clock */
#define TC_CCFG_CM_EXT                       (TC_CCFG_CM)                                        /*!< using external clock(to the timer) as input counting clock(refer to bitfield CS of register TC_CCFG for clock selection) */

/* TC_CCFG_FLTEN */
#define TC_CCFG_FLTEN_POS                    (1U)                                                /*!< TC_CCFG_FLTEN bit offset */

/* TC_CCFG_ECLKMODE */
#define TC_CCFG_ECLKMODE_POS                 (2U)                                                /*!< TC_CCFG_ECLKMODE bit offset */
#define TC_CCFG_ECLKMODE_CFG(regval)  (TC_CCFG_ECLKMODE & ((uint32_t)(regval) << TC_CCFG_ECLKMODE_POS))
#define TC_CCFG_ECLKMODE_PE                  (TC_CCFG_ECLKMODE_CFG(0))                           /*!< every posedge found on input recognized as a couting event */
#define TC_CCFG_ECLKMODE_NE                  (TC_CCFG_ECLKMODE_CFG(1))                           /*!< every negedge found on input recognized as a couting event */
#define TC_CCFG_ECLKMODE_BE                  (TC_CCFG_ECLKMODE_CFG(2))                           /*!< both posedge and negedge found on input recognized as a as a couting event */

/* TC_CCFG_FLTOPT */
#define TC_CCFG_FLTOPT_POS                   (4U)                                                /*!< TC_CCFG_FLTOPT bit offset */

/* TC_CTRL_START */
#define TC_CTRL_START_POS                    (0U)                                                /*!< TC_CTRL_START bit offset */

/* TC_CTRL_SLVEN */
#define TC_CTRL_SLVEN_POS                    (2U)                                                /*!< TC_CTRL_SLVEN bit offset */

/* TC_CTRL_SLVFLTEN */
#define TC_CTRL_SLVFLTEN_POS                 (3U)                                                /*!< TC_CTRL_SLVFLTEN bit offset */

/* TC_CTRL_OPS */
#define TC_CTRL_OPS_POS                      (4U)                                                /*!< TC_CTRL_OPS bit offset */
#define TC_CTRL_OPS_DIS                      ((uint32_t)0x00000000U)                             /*!< one pulse mode disabled(timer continue counting when overflow) */
#define TC_CTRL_OPS_EN                       (TC_CTRL_OPS)                                       /*!< one pulse mode enabled(timer stop counting when overflow) */

/* TC_CTRL_SLVTRGMODE */
#define TC_CTRL_SLVTRGMODE_POS               (5U)                                                /*!< TC_CTRL_SLVTRGMODE bit offset */
#define TC_CTRL_SLVTRGMODE_CFG(regval)  (TC_CTRL_SLVTRGMODE & ((uint32_t)(regval) << TC_CTRL_SLVTRGMODE_POS))
#define TC_CTRL_SLVTRGMODE_PE                (TC_CTRL_SLVTRGMODE_CFG(0))                         /*!< clean internal counter at posedge */
#define TC_CTRL_SLVTRGMODE_NE                (TC_CTRL_SLVTRGMODE_CFG(1))                         /*!< clean internal counter at negedge */
#define TC_CTRL_SLVTRGMODE_BE                (TC_CTRL_SLVTRGMODE_CFG(2))                         /*!< clean internal counter at both posedge and negedge */
#define TC_CTRL_SLVTRGMODE_GM                (TC_CTRL_SLVTRGMODE_CFG(3))                         /*!< level sensitive gate control */

/* TC_CTRL_SLVFLTOPT */
#define TC_CTRL_SLVFLTOPT_POS                (7U)                                                /*!< TC_CTRL_SLVFLTOPT bit offset */

/* TC_CTRL_SLVGATELVL */
#define TC_CTRL_SLVGATELVL_POS               (16U)                                               /*!< TC_CTRL_SLVGATELVL bit offset */
#define TC_CTRL_SLVGATELVL_LOW               ((uint32_t)0x00000000U)                             /*!< select low level sensitive gate */
#define TC_CTRL_SLVGATELVL_HIGH              (TC_CTRL_SLVGATELVL)                                /*!< select high level sensitive gate */

/* TC_CTRL_DBGSTBDIS */
#define TC_CTRL_DBGSTBDIS_POS                (17U)                                               /*!< TC_CTRL_DBGSTBDIS bit offset */
#define TC_CTRL_DBGSTBDIS_PAUSE              ((uint32_t)0x00000000U)                             /*!< halt counting when SoC halted by debugger */
#define TC_CTRL_DBGSTBDIS_COUNTINUE          (TC_CTRL_DBGSTBDIS)                                 /*!< keep counting when SoC halted by debugger */

/* TC_CM0_ENABLE */
#define TC_CM0_ENABLE_POS                    (0U)                                                /*!< TC_CM0_ENABLE bit offset */

/* TC_CM0_CCM */
#define TC_CM0_CCM_POS                       (1U)                                                /*!< TC_CM0_CCM bit offset */
#define TC_CM0_CCM_IC                        ((uint32_t)0x00000000U)                             /*!< select as input capture */
#define TC_CM0_CCM_OC                        (TC_CM0_CCM)                                        /*!< select as output compare */

/* TC_CM0_DIEN */
#define TC_CM0_DIEN_POS                      (2U)                                                /*!< TC_CM0_DIEN bit offset */

/* TC_CM0_DL */
#define TC_CM0_DL_POS                        (3U)                                                /*!< TC_CM0_DL bit offset */

/* TC_CM0_CPOL */
#define TC_CM0_CPOL_POS                      (9U)                                                /*!< TC_CM0_CPOL bit offset */
#define TC_CM0_CPOL_CFG(regval)  (TC_CM0_CPOL & ((uint32_t)(regval) << TC_CM0_CPOL_POS))
#define TC_CM0_CPOL_PE                       (TC_CM0_CPOL_CFG(0))                                /*!< capture on posedge */
#define TC_CM0_CPOL_NE                       (TC_CM0_CPOL_CFG(1))                                /*!< capture on negedge */
#define TC_CM0_CPOL_BE                       (TC_CM0_CPOL_CFG(2))                                /*!< capture on both posedge and negedge */

/* TC_CM0_FLTOPT */
#define TC_CM0_FLTOPT_POS                    (11U)                                               /*!< TC_CM0_FLTOPT bit offset */

/* TC_CM0_FLTEN */
#define TC_CM0_FLTEN_POS                     (20U)                                               /*!< TC_CM0_FLTEN bit offset */

/* TC_CM0_OM */
#define TC_CM0_OM_POS                        (21U)                                               /*!< TC_CM0_OM bit offset */
#define TC_CM0_OM_CFG(regval)  (TC_CM0_OM & ((uint32_t)(regval) << TC_CM0_OM_POS))
#define TC_CM0_OM_NUL                        (TC_CM0_OM_CFG(0))                                  /*!< no output generated */
#define TC_CM0_OM_VL                         (TC_CM0_OM_CFG(1))                                  /*!< set as valid level */
#define TC_CM0_OM_INVL                       (TC_CM0_OM_CFG(2))                                  /*!< set as invalid level */
#define TC_CM0_OM_TOG                        (TC_CM0_OM_CFG(3))                                  /*!< toggle output */
#define TC_CM0_OM_FVL                        (TC_CM0_OM_CFG(4))                                  /*!< force as valid level */
#define TC_CM0_OM_FINVL                      (TC_CM0_OM_CFG(5))                                  /*!< force as invalid level */
#define TC_CM0_OM_PWM1                       (TC_CM0_OM_CFG(6))                                  /*!< when tc_cnt less than tc_ccdx, set output as valid level */
#define TC_CM0_OM_PWM2                       (TC_CM0_OM_CFG(7))                                  /*!< when tc_cnt greater than tc_ccdx, set output as valid level */

/* TC_CM0_EFELVL */
#define TC_CM0_EFELVL_POS                    (24U)                                               /*!< TC_CM0_EFELVL bit offset */

/* TC_CM0_DFTLVL */
#define TC_CM0_DFTLVL_POS                    (25U)                                               /*!< TC_CM0_DFTLVL bit offset */

/* TC_CM1_ENABLE */
#define TC_CM1_ENABLE_POS                    (0U)                                                /*!< TC_CM1_ENABLE bit offset */

/* TC_CM1_CCM */
#define TC_CM1_CCM_POS                       (1U)                                                /*!< TC_CM1_CCM bit offset */
#define TC_CM1_CCM_IC                        ((uint32_t)0x00000000U)                             /*!< select as input capture */
#define TC_CM1_CCM_OC                        (TC_CM1_CCM)                                        /*!< select as output compare */

/* TC_CM1_DIEN */
#define TC_CM1_DIEN_POS                      (2U)                                                /*!< TC_CM1_DIEN bit offset */

/* TC_CM1_DL */
#define TC_CM1_DL_POS                        (3U)                                                /*!< TC_CM1_DL bit offset */

/* TC_CM1_CPOL */
#define TC_CM1_CPOL_POS                      (9U)                                                /*!< TC_CM1_CPOL bit offset */
#define TC_CM1_CPOL_CFG(regval)  (TC_CM1_CPOL & ((uint32_t)(regval) << TC_CM1_CPOL_POS))
#define TC_CM1_CPOL_PE                       (TC_CM1_CPOL_CFG(0))                                /*!< capture on posedge */
#define TC_CM1_CPOL_NE                       (TC_CM1_CPOL_CFG(1))                                /*!< capture on negedge */
#define TC_CM1_CPOL_BE                       (TC_CM1_CPOL_CFG(2))                                /*!< capture on both posedge and negedge */

/* TC_CM1_FLTOPT */
#define TC_CM1_FLTOPT_POS                    (11U)                                               /*!< TC_CM1_FLTOPT bit offset */

/* TC_CM1_FLTEN */
#define TC_CM1_FLTEN_POS                     (20U)                                               /*!< TC_CM1_FLTEN bit offset */

/* TC_CM1_OM */
#define TC_CM1_OM_POS                        (21U)                                               /*!< TC_CM1_OM bit offset */
#define TC_CM1_OM_CFG(regval)  (TC_CM1_OM & ((uint32_t)(regval) << TC_CM1_OM_POS))
#define TC_CM1_OM_NUL                        (TC_CM1_OM_CFG(0))                                  /*!< no output generated */
#define TC_CM1_OM_VL                         (TC_CM1_OM_CFG(1))                                  /*!< set as valid level */
#define TC_CM1_OM_INVL                       (TC_CM1_OM_CFG(2))                                  /*!< set as invalid level */
#define TC_CM1_OM_TOG                        (TC_CM1_OM_CFG(3))                                  /*!< toggle output */
#define TC_CM1_OM_FVL                        (TC_CM1_OM_CFG(4))                                  /*!< force as valid level */
#define TC_CM1_OM_FINVL                      (TC_CM1_OM_CFG(5))                                  /*!< force as invalid level */
#define TC_CM1_OM_PWM1                       (TC_CM1_OM_CFG(6))                                  /*!< when tc_cnt less than tc_ccdx, set output as valid level */
#define TC_CM1_OM_PWM2                       (TC_CM1_OM_CFG(7))                                  /*!< when tc_cnt greater than tc_ccdx, set output as valid level */

/* TC_CM1_EFELVL */
#define TC_CM1_EFELVL_POS                    (24U)                                               /*!< TC_CM1_EFELVL bit offset */

/* TC_CM1_DFTLVL */
#define TC_CM1_DFTLVL_POS                    (25U)                                               /*!< TC_CM1_DFTLVL bit offset */

/* TC_IE_OVIE */
#define TC_IE_OVIE_POS                       (0U)                                                /*!< TC_IE_OVIE bit offset */

/* TC_IE_CC0IE */
#define TC_IE_CC0IE_POS                      (1U)                                                /*!< TC_IE_CC0IE bit offset */

/* TC_IE_CC1IE */
#define TC_IE_CC1IE_POS                      (2U)                                                /*!< TC_IE_CC1IE bit offset */

/* TC_IE_SLVIE */
#define TC_IE_SLVIE_POS                      (3U)                                                /*!< TC_IE_SLVIE bit offset */

/* TC_STA_OVF */
#define TC_STA_OVF_POS                       (0U)                                                /*!< TC_STA_OVF bit offset */

/* TC_STA_CC0F */
#define TC_STA_CC0F_POS                      (1U)                                                /*!< TC_STA_CC0F bit offset */

/* TC_STA_CC1F */
#define TC_STA_CC1F_POS                      (2U)                                                /*!< TC_STA_CC1F bit offset */

/* TC_STA_SLVF */
#define TC_STA_SLVF_POS                      (3U)                                                /*!< TC_STA_SLVF bit offset */

/* TC_PWM_CFG_DONEIE */
#define TC_PWM_CFG_DONEIE_POS                (0U)                                                /*!< TC_PWM_CFG_DONEIE bit offset */
#define TC_PWM_CFG_DONEIE_DIS                ((uint32_t)0x00000000U)                             /*!< disable */
#define TC_PWM_CFG_DONEIE_EN                 (TC_PWM_CFG_DONEIE)                                 /*!< enable */

/* TC_PWM_CFG_DATERRIE */
#define TC_PWM_CFG_DATERRIE_POS              (1U)                                                /*!< TC_PWM_CFG_DATERRIE bit offset */
#define TC_PWM_CFG_DATERRIE_DIS              ((uint32_t)0x00000000U)                             /*!< disable */
#define TC_PWM_CFG_DATERRIE_EN               (TC_PWM_CFG_DATERRIE)                               /*!< enable */

/* TC_PWM_CFG_BUFVLDIE */
#define TC_PWM_CFG_BUFVLDIE_POS              (2U)                                                /*!< TC_PWM_CFG_BUFVLDIE bit offset */
#define TC_PWM_CFG_BUFVLDIE_DIS              ((uint32_t)0x00000000U)                             /*!< disable */
#define TC_PWM_CFG_BUFVLDIE_EN               (TC_PWM_CFG_BUFVLDIE)                               /*!< enable */

/* TC_PWM_CFG_CYCDONEIE */
#define TC_PWM_CFG_CYCDONEIE_POS             (3U)                                                /*!< TC_PWM_CFG_CYCDONEIE bit offset */
#define TC_PWM_CFG_CYCDONEIE_DIS             ((uint32_t)0x00000000U)                             /*!< disable */
#define TC_PWM_CFG_CYCDONEIE_EN              (TC_PWM_CFG_CYCDONEIE)                              /*!< enable */

/* TC_PWM_CFG_CMS */
#define TC_PWM_CFG_CMS_POS                   (4U)                                                /*!< TC_PWM_CFG_CMS bit offset */
#define TC_PWM_CFG_CMS_DMA                   ((uint32_t)0x00000000U)                             /*!< select as DMA mode */
#define TC_PWM_CFG_CMS_SOFT                  (TC_PWM_CFG_CMS)                                    /*!< select as soft control mode */

/* TC_PWM_CFG_CSS */
#define TC_PWM_CFG_CSS_POS                   (5U)                                                /*!< TC_PWM_CFG_CSS bit offset */
#define TC_PWM_CFG_CSS_CS                    ((uint32_t)0x00000000U)                             /*!< select to finish output last complete signal while module disabled */
#define TC_PWM_CFG_CSS_BS                    (TC_PWM_CFG_CSS)                                    /*!< select to break output signal while module disabled */

/* TC_PWM_CFG_ORV */
#define TC_PWM_CFG_ORV_POS                   (6U)                                                /*!< TC_PWM_CFG_ORV bit offset */
#define TC_PWM_CFG_ORV_WLC                   ((uint32_t)0x00000000U)                             /*!< wave length configurable */
#define TC_PWM_CFG_ORV_ORV                   (TC_PWM_CFG_ORV)                                    /*!< output remain valid */

/* TC_PWM_CFG_WL */
#define TC_PWM_CFG_WL_POS                    (7U)                                                /*!< TC_PWM_CFG_WL bit offset */

/* TC_PWM_CTRL_EN */
#define TC_PWM_CTRL_EN_POS                   (0U)                                                /*!< TC_PWM_CTRL_EN bit offset */

/* TC_PWM_STA_BUSY */
#define TC_PWM_STA_BUSY_POS                  (0U)                                                /*!< TC_PWM_STA_BUSY bit offset */

/* TC_PWM_STA_DONE */
#define TC_PWM_STA_DONE_POS                  (1U)                                                /*!< TC_PWM_STA_DONE bit offset */

/* TC_PWM_STA_DATERR */
#define TC_PWM_STA_DATERR_POS                (2U)                                                /*!< TC_PWM_STA_DATERR bit offset */

/* TC_PWM_STA_BUFVLD */
#define TC_PWM_STA_BUFVLD_POS                (3U)                                                /*!< TC_PWM_STA_BUFVLD bit offset */

/* TC_PWM_STA_CYCDONE */
#define TC_PWM_STA_CYCDONE_POS               (4U)                                                /*!< TC_PWM_STA_CYCDONE bit offset */

/* TC_PWM_CNT_N_PWM */
#define TC_PWM_CNT_N_PWM_POS                 (0U)                                                /*!< TC_PWM_CNT_N_PWM bit offset */

/* TC_PWM_DMA_BADR_BADR */
#define TC_PWM_DMA_BADR_BADR_POS             (0U)                                                /*!< TC_PWM_DMA_BADR_BADR bit offset */

/* TC_PWM_DMA_LEN_LEN */
#define TC_PWM_DMA_LEN_LEN_POS               (0U)                                                /*!< TC_PWM_DMA_LEN_LEN bit offset */

/* TC_PWM_DMA_ADR_ADR */
#define TC_PWM_DMA_ADR_ADR_POS               (0U)                                                /*!< TC_PWM_DMA_ADR_ADR bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_TC_H */
