/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_lcd.h
 * @author  Renergy Software Team
 * @brief   LCD module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_LCD_H
#define _RN831x_RN861x_MCU_LCD_H

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

/* LCD */
typedef struct
{
    __IO uint32_t CTRL;                      /*!< offset: 0x000 info: LCD control register */
    __IO uint32_t STATUS;                    /*!< offset: 0x004 info: LCD status register */
    __IO uint32_t CLKDIV;                    /*!< offset: 0x008 info: LCD clock control register */
    __IO uint32_t BLINK;                     /*!< offset: 0x00C info: LCD blink register */
    __IO uint32_t PS;                        /*!< offset: 0x010 info: Pump setup time register */
    __IO uint32_t RESCTL;                    /*!< offset: 0x014 info: Resistor control register */
    __IO uint32_t RESERVED0[2];              /*!< offset: 0x018 ~ 01C */
    __IO uint8_t  BUF[38];                   /*!< offset: 0x020~0x045 info: LCD buffer0~buffer37 register */
} LCD_TypeDef;

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
#define LCD_CORE0_BASE  (0x20060000U)
#define LCD_CORE1_BASE  (0xA0060000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define LCD_CORE0  ((LCD_TypeDef *)      LCD_CORE0_BASE)
#define LCD_CORE1  ((LCD_TypeDef *)      LCD_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define LCD  LCD_CORE0
#endif
#if defined(CORE1)
#define LCD  LCD_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** LCD Instances *******************************/
#define IS_LCD_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == LCD_CORE0) || \
                                          ((INSTANCE) == LCD_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* LCD_CTRL */
#define LCD_CTRL_EN                          (BIT(0))                                            /*!< LCD enable bit */
#define LCD_CTRL_DUTY                        (BITS(1,3))                                         /*!< LCD duty control bit */
#define LCD_CTRL_BIAS                        (BIT(4))                                            /*!< LCD bias control bit */
#define LCD_CTRL_BIASLVL                     (BITS(5,10))                                        /*!< Charge pump bias level control bit */
#define LCD_CTRL_TYPE                        (BIT(11))                                           /*!< LCD driver wave type bit */
#define LCD_CTRL_PWD_PUMP                    (BIT(12))                                           /*!< LCD charge pump enable bit */
#define LCD_CTRL_AF0                         (BIT(13))                                           /*!< Alternate funciton of SEG0 and COM4 */
#define LCD_CTRL_AF1                         (BIT(14))                                           /*!< Alternate funciton of SEG1 and COM5 */
#define LCD_CTRL_AF2                         (BIT(15))                                           /*!< Alternate funciton of SEG2 and COM6 */
#define LCD_CTRL_AF3                         (BIT(16))                                           /*!< Alternate funciton of SEG3 and COM7 */

/* LCD_STATUS */
#define LCD_STATUS_DON                       (BIT(0))                                            /*!< Display off pending bit */
#define LCD_STATUS_DOFF                      (BIT(1))                                            /*!< Display on pending bit */
#define LCD_STATUS_IRQONEN                   (BIT(2))                                            /*!< Display on IRQ enble bit */
#define LCD_STATUS_IRQOFFEN                  (BIT(3))                                            /*!< Display off IRQ enble bit */
#define LCD_STATUS_BUSY                      (BIT(6))                                            /*!< LCD busy bit */

/* LCD_CLKDIV */
#define LCD_CLKDIV_CLKDIV                    (BITS(0,7))                                         /*!< LCD clock prescale configuration bits */

/* LCD_BLINK */
#define LCD_BLINK_TON                        (BITS(0,8))                                         /*!< Display on IRQ enble bit */
#define LCD_BLINK_TOFF                       (BITS(9,17))                                        /*!< Display on pending bit */
#define LCD_BLINK_BLINK_TIME                 (BITS(18,25))                                       /*!< Display off pending bit */

/* LCD_PS */
#define LCD_PS_PS                            (BITS(0,15))                                        /*!< Pump setup time bits */

/* LCD_RESCTL */
#define LCD_RESCTL_IRSN                      (BIT(0))                                            /*!< Resistor division mode selection bit */
#define LCD_RESCTL_RSM                       (BIT(1))                                            /*!< Internal resistor division mode signal selection bit */
#define LCD_RESCTL_RES_FT                    (BITS(2,3))                                         /*!< 20k resistor driving count configuration bits */
#define LCD_RESCTL_RES_DT                    (BITS(4,6))                                         /*!< 20k resistor driving time configuration bits */
#define LCD_RESCTL_RES_AO                    (BIT(7))                                            /*!< Resistor string time-division driving type configuration bit */
#define LCD_RESCTL_FCC                       (BIT(8))                                            /*!< Fast charging control bit */
#define LCD_RESCTL_LDOS                      (BITS(9,12))                                        /*!< LDO output level selection bits */
#define LCD_RESCTL_OFF_LVL                   (BITS(13,15))                                       /*!< IO output after disable LCD */

/* LCD_BUF0 */
#define LCD_BUF0_BUF0                        (BITS(0,7))                                         /*!< Data bits of SEG(0)  COM[7:0] */

/* LCD_BUF1 */
#define LCD_BUF1_BUF1                        (BITS(0,7))                                         /*!< Data bits of SEG(1)  COM[7:0] */

/* LCD_BUF2 */
#define LCD_BUF2_BUF2                        (BITS(0,7))                                         /*!< Data bits of SEG(2)  COM[7:0] */

/* LCD_BUF3 */
#define LCD_BUF3_BUF3                        (BITS(0,7))                                         /*!< Data bits of SEG(3)  COM[7:0] */

/* LCD_BUF4 */
#define LCD_BUF4_BUF4                        (BITS(0,7))                                         /*!< Data bits of SEG(4)  COM[7:0] */

/* LCD_BUF5 */
#define LCD_BUF5_BUF5                        (BITS(0,7))                                         /*!< Data bits of SEG(5)  COM[7:0] */

/* LCD_BUF6 */
#define LCD_BUF6_BUF6                        (BITS(0,7))                                         /*!< Data bits of SEG(6)  COM[7:0] */

/* LCD_BUF7 */
#define LCD_BUF7_BUF7                        (BITS(0,7))                                         /*!< Data bits of SEG(7)  COM[7:0] */

/* LCD_BUF8 */
#define LCD_BUF8_BUF8                        (BITS(0,7))                                         /*!< Data bits of SEG(8)  COM[7:0] */

/* LCD_BUF9 */
#define LCD_BUF9_BUF9                        (BITS(0,7))                                         /*!< Data bits of SEG(9)  COM[7:0] */

/* LCD_BUF10 */
#define LCD_BUF10_BUF10                      (BITS(0,7))                                         /*!< Data bits of SEG(10)  COM[7:0] */

/* LCD_BUF11 */
#define LCD_BUF11_BUF11                      (BITS(0,7))                                         /*!< Data bits of SEG(11)  COM[7:0] */

/* LCD_BUF12 */
#define LCD_BUF12_BUF12                      (BITS(0,7))                                         /*!< Data bits of SEG(12)  COM[7:0] */

/* LCD_BUF13 */
#define LCD_BUF13_BUF13                      (BITS(0,7))                                         /*!< Data bits of SEG(13)  COM[7:0] */

/* LCD_BUF14 */
#define LCD_BUF14_BUF14                      (BITS(0,7))                                         /*!< Data bits of SEG(14)  COM[7:0] */

/* LCD_BUF15 */
#define LCD_BUF15_BUF15                      (BITS(0,7))                                         /*!< Data bits of SEG(15)  COM[7:0] */

/* LCD_BUF16 */
#define LCD_BUF16_BUF16                      (BITS(0,7))                                         /*!< Data bits of SEG(16)  COM[7:0] */

/* LCD_BUF17 */
#define LCD_BUF17_BUF17                      (BITS(0,7))                                         /*!< Data bits of SEG(17)  COM[7:0] */

/* LCD_BUF18 */
#define LCD_BUF18_BUF18                      (BITS(0,7))                                         /*!< Data bits of SEG(18)  COM[7:0] */

/* LCD_BUF19 */
#define LCD_BUF19_BUF19                      (BITS(0,7))                                         /*!< Data bits of SEG(19)  COM[7:0] */

/* LCD_BUF20 */
#define LCD_BUF20_BUF20                      (BITS(0,7))                                         /*!< Data bits of SEG(20)  COM[7:0] */

/* LCD_BUF21 */
#define LCD_BUF21_BUF21                      (BITS(0,7))                                         /*!< Data bits of SEG(21)  COM[7:0] */

/* LCD_BUF22 */
#define LCD_BUF22_BUF22                      (BITS(0,7))                                         /*!< Data bits of SEG(22)  COM[7:0] */

/* LCD_BUF23 */
#define LCD_BUF23_BUF23                      (BITS(0,7))                                         /*!< Data bits of SEG(23)  COM[7:0] */

/* LCD_BUF24 */
#define LCD_BUF24_BUF24                      (BITS(0,7))                                         /*!< Data bits of SEG(24)  COM[7:0] */

/* LCD_BUF25 */
#define LCD_BUF25_BUF25                      (BITS(0,7))                                         /*!< Data bits of SEG(25)  COM[7:0] */

/* LCD_BUF26 */
#define LCD_BUF26_BUF26                      (BITS(0,7))                                         /*!< Data bits of SEG(26)  COM[7:0] */

/* LCD_BUF27 */
#define LCD_BUF27_BUF27                      (BITS(0,7))                                         /*!< Data bits of SEG(27)  COM[7:0] */

/* LCD_BUF28 */
#define LCD_BUF28_BUF28                      (BITS(0,7))                                         /*!< Data bits of SEG(28)  COM[7:0] */

/* LCD_BUF29 */
#define LCD_BUF29_BUF29                      (BITS(0,7))                                         /*!< Data bits of SEG(29)  COM[7:0] */

/* LCD_BUF30 */
#define LCD_BUF30_BUF30                      (BITS(0,7))                                         /*!< Data bits of SEG(30)  COM[7:0] */

/* LCD_BUF31 */
#define LCD_BUF31_BUF31                      (BITS(0,7))                                         /*!< Data bits of SEG(31)  COM[7:0] */

/* LCD_BUF32 */
#define LCD_BUF32_BUF32                      (BITS(0,7))                                         /*!< Data bits of SEG(32)  COM[7:0] */

/* LCD_BUF33 */
#define LCD_BUF33_BUF33                      (BITS(0,7))                                         /*!< Data bits of SEG(33)  COM[7:0] */

/* LCD_BUF34 */
#define LCD_BUF34_BUF34                      (BITS(0,7))                                         /*!< Data bits of SEG(34)  COM[7:0] */

/* LCD_BUF35 */
#define LCD_BUF35_BUF35                      (BITS(0,7))                                         /*!< Data bits of SEG(35)  COM[7:0] */

/* LCD_BUF36 */
#define LCD_BUF36_BUF36                      (BITS(0,7))                                         /*!< Data bits of SEG(36)  COM[7:0] */

/* LCD_BUF37 */
#define LCD_BUF37_BUF37                      (BITS(0,7))                                         /*!< Data bits of SEG(37)  COM[7:0] */

/* constants definitions */
/* LCD_CTRL_EN */
#define LCD_CTRL_EN_POS                      (0U)                                                /*!< LCD_CTRL_EN bit offset */

/* LCD_CTRL_DUTY */
#define LCD_CTRL_DUTY_POS                    (1U)                                                /*!< LCD_CTRL_DUTY bit offset */
#define LCD_CTRL_DUTY_CFG(regval)  (LCD_CTRL_DUTY & ((uint32_t)(regval) << LCD_CTRL_DUTY_POS))
#define LCD_CTRL_DUTY_STA                    (LCD_CTRL_DUTY_CFG(0))                              /*!< static output (COM0) */
#define LCD_CTRL_DUTY_1_2                    (LCD_CTRL_DUTY_CFG(1))                              /*!< duty cycle = 1/2 (COM0~1) */
#define LCD_CTRL_DUTY_1_3                    (LCD_CTRL_DUTY_CFG(2))                              /*!< duty cycle = 1/3 (COM0~2) */
#define LCD_CTRL_DUTY_1_4                    (LCD_CTRL_DUTY_CFG(3))                              /*!< duty cycle = 1/4 (COM0~3) */
#define LCD_CTRL_DUTY_1_6                    (LCD_CTRL_DUTY_CFG(4))                              /*!< duty cycle = 1/6 (COM0~5) */
#define LCD_CTRL_DUTY_1_8                    (LCD_CTRL_DUTY_CFG(5))                              /*!< duty cycle = 1/8 (COM0~7) */

/* LCD_CTRL_BIAS */
#define LCD_CTRL_BIAS_POS                    (4U)                                                /*!< LCD_CTRL_BIAS bit offset */
#define LCD_CTRL_BIAS_1_3                    ((uint32_t)0x00000000U)                             /*!< Select LCD bias as 1/3 voltage bias */
#define LCD_CTRL_BIAS_1_4                    (LCD_CTRL_BIAS)                                     /*!< Select LCD bias as 1/4 voltage bias */

/* LCD_CTRL_BIASLVL */
#define LCD_CTRL_BIASLVL_POS                 (5U)                                                /*!< LCD_CTRL_BIASLVL bit offset */

/* LCD_CTRL_TYPE */
#define LCD_CTRL_TYPE_POS                    (11U)                                               /*!< LCD_CTRL_TYPE bit offset */
#define LCD_CTRL_TYPE_A                      ((uint32_t)0x00000000U)                             /*!< Select LCD drive type A */
#define LCD_CTRL_TYPE_B                      (LCD_CTRL_TYPE)                                     /*!< Select LCD drive type B */

/* LCD_CTRL_PWD_PUMP */
#define LCD_CTRL_PWD_PUMP_POS                (12U)                                               /*!< LCD_CTRL_PWD_PUMP bit offset */
#define LCD_CTRL_PWD_PUMP_EN                 ((uint32_t)0x00000000U)                             /*!< LCD PUMP enable */
#define LCD_CTRL_PWD_PUMP_DIS                (LCD_CTRL_PWD_PUMP)                                 /*!< LCD PUMP disable */

/* LCD_CTRL_AF0 */
#define LCD_CTRL_AF0_POS                     (13U)                                               /*!< LCD_CTRL_AF0 bit offset */
#define LCD_CTRL_AF0_SEG                     ((uint32_t)0x00000000U)                             /*!< Select IO as SEG */
#define LCD_CTRL_AF0_COM                     (LCD_CTRL_AF0)                                      /*!< Select IO as COM */

/* LCD_CTRL_AF1 */
#define LCD_CTRL_AF1_POS                     (14U)                                               /*!< LCD_CTRL_AF1 bit offset */
#define LCD_CTRL_AF1_SEG                     ((uint32_t)0x00000000U)                             /*!< Select IO as SEG */
#define LCD_CTRL_AF1_COM                     (LCD_CTRL_AF1)                                      /*!< Select IO as COM */

/* LCD_CTRL_AF2 */
#define LCD_CTRL_AF2_POS                     (15U)                                               /*!< LCD_CTRL_AF2 bit offset */
#define LCD_CTRL_AF2_SEG                     ((uint32_t)0x00000000U)                             /*!< Select IO as SEG */
#define LCD_CTRL_AF2_COM                     (LCD_CTRL_AF2)                                      /*!< Select IO as COM */

/* LCD_CTRL_AF3 */
#define LCD_CTRL_AF3_POS                     (16U)                                               /*!< LCD_CTRL_AF3 bit offset */
#define LCD_CTRL_AF3_SEG                     ((uint32_t)0x00000000U)                             /*!< Select IO as SEG */
#define LCD_CTRL_AF3_COM                     (LCD_CTRL_AF3)                                      /*!< Select IO as COM */

/* LCD_STATUS_DON */
#define LCD_STATUS_DON_POS                   (0U)                                                /*!< LCD_STATUS_DON bit offset */
#define LCD_STATUS_DON_NUL                   ((uint32_t)0x00000000U)                             /*!< LCD display on did not occured */
#define LCD_STATUS_DON_OCC                   (LCD_STATUS_DON)                                    /*!< LCD display on occured */

/* LCD_STATUS_DOFF */
#define LCD_STATUS_DOFF_POS                  (1U)                                                /*!< LCD_STATUS_DOFF bit offset */
#define LCD_STATUS_DOFF_NUL                  ((uint32_t)0x00000000U)                             /*!< LCD display off did not occured */
#define LCD_STATUS_DOFF_OCC                  (LCD_STATUS_DOFF)                                   /*!< LCD display off occured */

/* LCD_STATUS_IRQONEN */
#define LCD_STATUS_IRQONEN_POS               (2U)                                                /*!< LCD_STATUS_IRQONEN bit offset */

/* LCD_STATUS_IRQOFFEN */
#define LCD_STATUS_IRQOFFEN_POS              (3U)                                                /*!< LCD_STATUS_IRQOFFEN bit offset */

/* LCD_STATUS_BUSY */
#define LCD_STATUS_BUSY_POS                  (6U)                                                /*!< LCD_STATUS_BUSY bit offset */

/* LCD_CLKDIV_CLKDIV */
#define LCD_CLKDIV_CLKDIV_POS                (0U)                                                /*!< LCD_CLKDIV_CLKDIV bit offset */

/* LCD_BLINK_TON */
#define LCD_BLINK_TON_POS                    (0U)                                                /*!< LCD_BLINK_TON bit offset */

/* LCD_BLINK_TOFF */
#define LCD_BLINK_TOFF_POS                   (9U)                                                /*!< LCD_BLINK_TOFF bit offset */

/* LCD_BLINK_BLINK_TIME */
#define LCD_BLINK_BLINK_TIME_POS             (18U)                                               /*!< LCD_BLINK_BLINK_TIME bit offset */

/* LCD_PS_PS */
#define LCD_PS_PS_POS                        (0U)                                                /*!< LCD_PS_PS bit offset */

/* LCD_RESCTL_IRSN */
#define LCD_RESCTL_IRSN_POS                  (0U)                                                /*!< LCD_RESCTL_IRSN bit offset */
#define LCD_RESCTL_IRSN_INTERRES             ((uint32_t)0x00000000U)                             /*!< Internal resistor division mode */
#define LCD_RESCTL_IRSN_RESERVED             (LCD_RESCTL_IRSN)                                   /*!< Reserved */

/* LCD_RESCTL_RSM */
#define LCD_RESCTL_RSM_POS                   (1U)                                                /*!< LCD_RESCTL_RSM bit offset */
#define LCD_RESCTL_RSM_SMALL                 ((uint32_t)0x00000000U)                             /*!< 20k resistor time-division connected, 200k resistor short circuited */
#define LCD_RESCTL_RSM_SWITCH                (LCD_RESCTL_RSM)                                    /*!< Time-division switching between 20k resistor and 200k resistor */

/* LCD_RESCTL_RES_FT */
#define LCD_RESCTL_RES_FT_POS                (2U)                                                /*!< LCD_RESCTL_RES_FT bit offset */
#define LCD_RESCTL_RES_FT_CFG(regval)  (LCD_RESCTL_RES_FT & ((uint32_t)(regval) << LCD_RESCTL_RES_FT_POS))
#define LCD_RESCTL_RES_FT_1                  (LCD_RESCTL_RES_FT_CFG(0))                          /*!< Driving once per cycle */
#define LCD_RESCTL_RES_FT_2                  (LCD_RESCTL_RES_FT_CFG(1))                          /*!< Driving 2 times per cycle */
#define LCD_RESCTL_RES_FT_3                  (LCD_RESCTL_RES_FT_CFG(2))                          /*!< Driving 3 times per cycle */
#define LCD_RESCTL_RES_FT_4                  (LCD_RESCTL_RES_FT_CFG(3))                          /*!< Driving 4 times per cycle */

/* LCD_RESCTL_RES_DT */
#define LCD_RESCTL_RES_DT_POS                (4U)                                                /*!< LCD_RESCTL_RES_DT bit offset */

/* LCD_RESCTL_RES_AO */
#define LCD_RESCTL_RES_AO_POS                (7U)                                                /*!< LCD_RESCTL_RES_AO bit offset */
#define LCD_RESCTL_RES_AO_TIMEDIV            ((uint32_t)0x00000000U)                             /*!< Resistors time-division driving */
#define LCD_RESCTL_RES_AO_ALWAYS             (LCD_RESCTL_RES_AO)                                 /*!< Resistors always drive */

/* LCD_RESCTL_FCC */
#define LCD_RESCTL_FCC_POS                   (8U)                                                /*!< LCD_RESCTL_FCC bit offset */
#define LCD_RESCTL_FCC_OFF                   ((uint32_t)0x00000000U)                             /*!< Turn off fast charging function */
#define LCD_RESCTL_FCC_ON                    (LCD_RESCTL_FCC)                                    /*!< Turn on fast charging function */

/* LCD_RESCTL_LDOS */
#define LCD_RESCTL_LDOS_POS                  (9U)                                                /*!< LCD_RESCTL_LDOS bit offset */
#define LCD_RESCTL_LDOS_CFG(regval)  (LCD_RESCTL_LDOS & ((uint32_t)(regval) << LCD_RESCTL_LDOS_POS))
#define LCD_RESCTL_LDOS_2P7                  (LCD_RESCTL_LDOS_CFG(0))                            /*!< Select LDO 2.7V as output level */
#define LCD_RESCTL_LDOS_2P76                 (LCD_RESCTL_LDOS_CFG(1))                            /*!< Select LDO 2.76V as output level */
#define LCD_RESCTL_LDOS_2P82                 (LCD_RESCTL_LDOS_CFG(2))                            /*!< Select LDO 2.82V as output level */
#define LCD_RESCTL_LDOS_2P88                 (LCD_RESCTL_LDOS_CFG(3))                            /*!< Select LDO 2.88V as output level */
#define LCD_RESCTL_LDOS_2P94                 (LCD_RESCTL_LDOS_CFG(4))                            /*!< Select LDO 2.94V as output level */
#define LCD_RESCTL_LDOS_3P0                  (LCD_RESCTL_LDOS_CFG(5))                            /*!< Select LDO 3.0V  as output level */
#define LCD_RESCTL_LDOS_3P06                 (LCD_RESCTL_LDOS_CFG(6))                            /*!< Select LDO 3.06V as output level */
#define LCD_RESCTL_LDOS_3P12                 (LCD_RESCTL_LDOS_CFG(7))                            /*!< Select LDO 3.12V as output level */
#define LCD_RESCTL_LDOS_3P18                 (LCD_RESCTL_LDOS_CFG(8))                            /*!< Select LDO 3.18V as output level */
#define LCD_RESCTL_LDOS_3P24                 (LCD_RESCTL_LDOS_CFG(9))                            /*!< Select LDO 3.24V as output level */
#define LCD_RESCTL_LDOS_3P3                  (LCD_RESCTL_LDOS_CFG(10))                           /*!< Select LDO 3.3V  as output level */
#define LCD_RESCTL_LDOS_3P36                 (LCD_RESCTL_LDOS_CFG(11))                           /*!< Select LDO 3.36V as output level */
#define LCD_RESCTL_LDOS_3P42                 (LCD_RESCTL_LDOS_CFG(12))                           /*!< Select LDO 3.42V as output level */
#define LCD_RESCTL_LDOS_3P48                 (LCD_RESCTL_LDOS_CFG(13))                           /*!< Select LDO 3.48V as output level */
#define LCD_RESCTL_LDOS_3P54                 (LCD_RESCTL_LDOS_CFG(14))                           /*!< Select LDO 3.54V as output level */
#define LCD_RESCTL_LDOS_3P6                  (LCD_RESCTL_LDOS_CFG(15))                           /*!< Select LDO 3.6V  as output level */

/* LCD_RESCTL_OFF_LVL */
#define LCD_RESCTL_OFF_LVL_POS               (13U)                                               /*!< LCD_RESCTL_OFF_LVL bit offset */
#define LCD_RESCTL_OFF_LVL_CFG(regval)  (LCD_RESCTL_OFF_LVL & ((uint32_t)(regval) << LCD_RESCTL_OFF_LVL_POS))
#define LCD_RESCTL_OFF_LVL_V0                (LCD_RESCTL_OFF_LVL_CFG(0))                         /*!< OFF_LVL = 0/6/7 output LCDV0 */
#define LCD_RESCTL_OFF_LVL_VA                (LCD_RESCTL_OFF_LVL_CFG(1))                         /*!< Output LCDVA */
#define LCD_RESCTL_OFF_LVL_VB                (LCD_RESCTL_OFF_LVL_CFG(2))                         /*!< Output LCDVB */
#define LCD_RESCTL_OFF_LVL_VC                (LCD_RESCTL_OFF_LVL_CFG(3))                         /*!< Output LCDVC */
#define LCD_RESCTL_OFF_LVL_VD                (LCD_RESCTL_OFF_LVL_CFG(4))                         /*!< Output LCDVD */
#define LCD_RESCTL_OFF_LVL_HIGHZ             (LCD_RESCTL_OFF_LVL_CFG(5))                         /*!< Output High-Z */

/* LCD_BUF0_BUF0 */
#define LCD_BUF0_BUF0_POS                    (0U)                                                /*!< LCD_BUF0_BUF0 bit offset */

/* LCD_BUF1_BUF1 */
#define LCD_BUF1_BUF1_POS                    (0U)                                                /*!< LCD_BUF1_BUF1 bit offset */

/* LCD_BUF2_BUF2 */
#define LCD_BUF2_BUF2_POS                    (0U)                                                /*!< LCD_BUF2_BUF2 bit offset */

/* LCD_BUF3_BUF3 */
#define LCD_BUF3_BUF3_POS                    (0U)                                                /*!< LCD_BUF3_BUF3 bit offset */

/* LCD_BUF4_BUF4 */
#define LCD_BUF4_BUF4_POS                    (0U)                                                /*!< LCD_BUF4_BUF4 bit offset */

/* LCD_BUF5_BUF5 */
#define LCD_BUF5_BUF5_POS                    (0U)                                                /*!< LCD_BUF5_BUF5 bit offset */

/* LCD_BUF6_BUF6 */
#define LCD_BUF6_BUF6_POS                    (0U)                                                /*!< LCD_BUF6_BUF6 bit offset */

/* LCD_BUF7_BUF7 */
#define LCD_BUF7_BUF7_POS                    (0U)                                                /*!< LCD_BUF7_BUF7 bit offset */

/* LCD_BUF8_BUF8 */
#define LCD_BUF8_BUF8_POS                    (0U)                                                /*!< LCD_BUF8_BUF8 bit offset */

/* LCD_BUF9_BUF9 */
#define LCD_BUF9_BUF9_POS                    (0U)                                                /*!< LCD_BUF9_BUF9 bit offset */

/* LCD_BUF10_BUF10 */
#define LCD_BUF10_BUF10_POS                  (0U)                                                /*!< LCD_BUF10_BUF10 bit offset */

/* LCD_BUF11_BUF11 */
#define LCD_BUF11_BUF11_POS                  (0U)                                                /*!< LCD_BUF11_BUF11 bit offset */

/* LCD_BUF12_BUF12 */
#define LCD_BUF12_BUF12_POS                  (0U)                                                /*!< LCD_BUF12_BUF12 bit offset */

/* LCD_BUF13_BUF13 */
#define LCD_BUF13_BUF13_POS                  (0U)                                                /*!< LCD_BUF13_BUF13 bit offset */

/* LCD_BUF14_BUF14 */
#define LCD_BUF14_BUF14_POS                  (0U)                                                /*!< LCD_BUF14_BUF14 bit offset */

/* LCD_BUF15_BUF15 */
#define LCD_BUF15_BUF15_POS                  (0U)                                                /*!< LCD_BUF15_BUF15 bit offset */

/* LCD_BUF16_BUF16 */
#define LCD_BUF16_BUF16_POS                  (0U)                                                /*!< LCD_BUF16_BUF16 bit offset */

/* LCD_BUF17_BUF17 */
#define LCD_BUF17_BUF17_POS                  (0U)                                                /*!< LCD_BUF17_BUF17 bit offset */

/* LCD_BUF18_BUF18 */
#define LCD_BUF18_BUF18_POS                  (0U)                                                /*!< LCD_BUF18_BUF18 bit offset */

/* LCD_BUF19_BUF19 */
#define LCD_BUF19_BUF19_POS                  (0U)                                                /*!< LCD_BUF19_BUF19 bit offset */

/* LCD_BUF20_BUF20 */
#define LCD_BUF20_BUF20_POS                  (0U)                                                /*!< LCD_BUF20_BUF20 bit offset */

/* LCD_BUF21_BUF21 */
#define LCD_BUF21_BUF21_POS                  (0U)                                                /*!< LCD_BUF21_BUF21 bit offset */

/* LCD_BUF22_BUF22 */
#define LCD_BUF22_BUF22_POS                  (0U)                                                /*!< LCD_BUF22_BUF22 bit offset */

/* LCD_BUF23_BUF23 */
#define LCD_BUF23_BUF23_POS                  (0U)                                                /*!< LCD_BUF23_BUF23 bit offset */

/* LCD_BUF24_BUF24 */
#define LCD_BUF24_BUF24_POS                  (0U)                                                /*!< LCD_BUF24_BUF24 bit offset */

/* LCD_BUF25_BUF25 */
#define LCD_BUF25_BUF25_POS                  (0U)                                                /*!< LCD_BUF25_BUF25 bit offset */

/* LCD_BUF26_BUF26 */
#define LCD_BUF26_BUF26_POS                  (0U)                                                /*!< LCD_BUF26_BUF26 bit offset */

/* LCD_BUF27_BUF27 */
#define LCD_BUF27_BUF27_POS                  (0U)                                                /*!< LCD_BUF27_BUF27 bit offset */

/* LCD_BUF28_BUF28 */
#define LCD_BUF28_BUF28_POS                  (0U)                                                /*!< LCD_BUF28_BUF28 bit offset */

/* LCD_BUF29_BUF29 */
#define LCD_BUF29_BUF29_POS                  (0U)                                                /*!< LCD_BUF29_BUF29 bit offset */

/* LCD_BUF30_BUF30 */
#define LCD_BUF30_BUF30_POS                  (0U)                                                /*!< LCD_BUF30_BUF30 bit offset */

/* LCD_BUF31_BUF31 */
#define LCD_BUF31_BUF31_POS                  (0U)                                                /*!< LCD_BUF31_BUF31 bit offset */

/* LCD_BUF32_BUF32 */
#define LCD_BUF32_BUF32_POS                  (0U)                                                /*!< LCD_BUF32_BUF32 bit offset */

/* LCD_BUF33_BUF33 */
#define LCD_BUF33_BUF33_POS                  (0U)                                                /*!< LCD_BUF33_BUF33 bit offset */

/* LCD_BUF34_BUF34 */
#define LCD_BUF34_BUF34_POS                  (0U)                                                /*!< LCD_BUF34_BUF34 bit offset */

/* LCD_BUF35_BUF35 */
#define LCD_BUF35_BUF35_POS                  (0U)                                                /*!< LCD_BUF35_BUF35 bit offset */

/* LCD_BUF36_BUF36 */
#define LCD_BUF36_BUF36_POS                  (0U)                                                /*!< LCD_BUF36_BUF36 bit offset */

/* LCD_BUF37_BUF37 */
#define LCD_BUF37_BUF37_POS                  (0U)                                                /*!< LCD_BUF37_BUF37 bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_LCD_H */
