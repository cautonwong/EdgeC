/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_iocnt.h
 * @author  Renergy Software Team
 * @brief   IOCNT module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_IOCNT_H
#define _RN831x_RN861x_MCU_IOCNT_H

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

/* IOCNT */
typedef struct
{
    __IO uint32_t CFG0;                      /*!< offset: 0x000 info: IOCNT0 configuration register */
    __IO uint32_t CFG1;                      /*!< offset: 0x004 info: IOCNT1 configuration register */
    __IO uint32_t CFG2;                      /*!< offset: 0x008 info: IOCNT2 configuration register */
    __IO uint32_t CFG3;                      /*!< offset: 0x00C info: IOCNT3 configuration register */
    __IO uint32_t CFG4;                      /*!< offset: 0x010 info: IOCNT4 configuration register */
    __IO uint32_t RESERVED0[3];              /*!< offset: 0x014 ~ 01C */
    __IO uint32_t OUT0;                      /*!< offset: 0x020 info: IOCNT0 output pulse counter register */
    __IO uint32_t OUT1;                      /*!< offset: 0x024 info: IOCNT1 output pulse counter register */
    __IO uint32_t OUT2;                      /*!< offset: 0x028 info: IOCNT2 output pulse counter register */
    __IO uint32_t OUT3;                      /*!< offset: 0x02C info: IOCNT3 output pulse counter register */
    __IO uint32_t OUT4;                      /*!< offset: 0x030 info: IOCNT4 output pulse counter register */
    __IO uint32_t RESERVED1[3];              /*!< offset: 0x034 ~ 03C */
    __IO uint32_t CHNL;                      /*!< offset: 0x040 info: IOCNTx input channel select register */
    __IO uint32_t RESERVED2;                 /*!< offset: 0x044 */
    __IO uint32_t CTRL;                      /*!< offset: 0x048 info: IOCNTx Control register */
    __IO uint32_t COPL;                      /*!< offset: 0x04C info: IOCNTx output pulse configuration register */
    __IO uint32_t CMD;                       /*!< offset: 0x050 info: IOCNTx input command register */
    __IO uint32_t RESERVED3[3];              /*!< offset: 0x054 ~ 05C */
    __IO uint32_t REMD0;                     /*!< offset: 0x060 info: IOCNT0 input pulse remainder register */
    __IO uint32_t REMD1;                     /*!< offset: 0x064 info: IOCNT1 input pulse remainder register */
    __IO uint32_t REMD2;                     /*!< offset: 0x068 info: IOCNT2 input pulse remainder register */
    __IO uint32_t REMD3;                     /*!< offset: 0x06C info: IOCNT3 input pulse remainder register */
    __IO uint32_t REMD4;                     /*!< offset: 0x070 info: IOCNT4 input pulse remainder register */
} IOCNT_TypeDef;

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
#define IOCNT_CORE0_BASE (0x2005C000U)
#define IOCNT_CORE1_BASE (0xA005C000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define IOCNT_CORE0 ((IOCNT_TypeDef *)    IOCNT_CORE0_BASE)
#define IOCNT_CORE1 ((IOCNT_TypeDef *)    IOCNT_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define IOCNT  IOCNT_CORE0
#endif
#if defined(CORE1)
#define IOCNT  IOCNT_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** IOCNT Instances *******************************/
#define IS_IOCNT_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == IOCNT_CORE0) || \
                                            ((INSTANCE) == IOCNT_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* IOCNT_CFG0 */
#define IOCNT_CFG0_CNT                       (BITS(0,19))                                        /*!< Input pulse counter */
#define IOCNT_CFG0_CFG                       (BITS(20,30))                                       /*!< Pulse prescale confige bit */
#define IOCNT_CFG0_INT                       (BIT(31))                                           /*!< Pulse invert enable bit */

/* IOCNT_CFG1 */
#define IOCNT_CFG1_CNT                       (BITS(0,19))                                        /*!< Input pulse counter */
#define IOCNT_CFG1_CFG                       (BITS(20,30))                                       /*!< Pulse prescale confige bit */
#define IOCNT_CFG1_INT                       (BIT(31))                                           /*!< Pulse invert enable bit */

/* IOCNT_CFG2 */
#define IOCNT_CFG2_CNT                       (BITS(0,19))                                        /*!< Input pulse counter */
#define IOCNT_CFG2_CFG                       (BITS(20,30))                                       /*!< Pulse prescale confige bit */
#define IOCNT_CFG2_INT                       (BIT(31))                                           /*!< Pulse invert enable bit */

/* IOCNT_CFG3 */
#define IOCNT_CFG3_CNT                       (BITS(0,19))                                        /*!< Input pulse counter */
#define IOCNT_CFG3_CFG                       (BITS(20,30))                                       /*!< Pulse prescale confige bit */
#define IOCNT_CFG3_INT                       (BIT(31))                                           /*!< Pulse invert enable bit */

/* IOCNT_CFG4 */
#define IOCNT_CFG4_CNT                       (BITS(0,19))                                        /*!< Input pulse counter */
#define IOCNT_CFG4_CFG                       (BITS(20,30))                                       /*!< Pulse prescale confige bit */
#define IOCNT_CFG4_INT                       (BIT(31))                                           /*!< Pulse invert enable bit */

/* IOCNT_OUT0 */
#define IOCNT_OUT0_CNT                       (BITS(0,19))                                        /*!< IOCNT0 output pulse counter bits */

/* IOCNT_OUT1 */
#define IOCNT_OUT1_CNT                       (BITS(0,19))                                        /*!< IOCNT1 output pulse counter bits */

/* IOCNT_OUT2 */
#define IOCNT_OUT2_CNT                       (BITS(0,19))                                        /*!< IOCNT2 output pulse counter bits */

/* IOCNT_OUT3 */
#define IOCNT_OUT3_CNT                       (BITS(0,19))                                        /*!< IOCNT3 output pulse counter bits */

/* IOCNT_OUT4 */
#define IOCNT_OUT4_CNT                       (BITS(0,19))                                        /*!< IOCNT4 output pulse counter bits */

/* IOCNT_CHNL */
#define IOCNT_CHNL_IOCNT0_SEL                (BITS(0,5))                                         /*!< Input data type select bit of IOCNT0 */
#define IOCNT_CHNL_IOCNT1_SEL                (BITS(6,11))                                        /*!< Input data type select bit of IOCNT1 */
#define IOCNT_CHNL_IOCNT2_SEL                (BITS(12,17))                                       /*!< Input data type select bit of IOCNT2 */
#define IOCNT_CHNL_IOCNT3_SEL                (BITS(18,23))                                       /*!< Input data type select bit of IOCNT3 */
#define IOCNT_CHNL_IOCNT4_SEL                (BITS(24,29))                                       /*!< Input data type select bit of IOCNT4 */

/* IOCNT_CTRL */
#define IOCNT_CTRL_EN0                       (BIT(0))                                            /*!< IOCNT0 enable bit */
#define IOCNT_CTRL_EN1                       (BIT(1))                                            /*!< IOCNT1 enable bit */
#define IOCNT_CTRL_EN2                       (BIT(2))                                            /*!< IOCNT2 enable bit */
#define IOCNT_CTRL_EN3                       (BIT(3))                                            /*!< IOCNT3 enable bit */
#define IOCNT_CTRL_EN4                       (BIT(4))                                            /*!< IOCNT4 enable bit */
#define IOCNT_CTRL_MODE0                     (BIT(8))                                            /*!< IOCNT0 pulse counter mode */
#define IOCNT_CTRL_MODE1                     (BIT(9))                                            /*!< IOCNT1 pulse counter mode */
#define IOCNT_CTRL_MODE2                     (BIT(10))                                           /*!< IOCNT2 pulse counter mode */
#define IOCNT_CTRL_MODE3                     (BIT(11))                                           /*!< IOCNT3 pulse counter mode */
#define IOCNT_CTRL_MODE4                     (BIT(12))                                           /*!< IOCNT4 pulse counter mode */
#define IOCNT_CTRL_CNT_CLR                   (BIT(16))                                           /*!< Pulse counter read to clear confige bit */
#define IOCNT_CTRL_FLT_BYPASS0               (BIT(17))                                           /*!< Filter bypass of IOCNT0 */
#define IOCNT_CTRL_FLT_BYPASS1               (BIT(18))                                           /*!< Filter bypass of IOCNT1 */
#define IOCNT_CTRL_FLT_BYPASS2               (BIT(19))                                           /*!< Filter bypass of IOCNT2 */
#define IOCNT_CTRL_FLT_BYPASS3               (BIT(20))                                           /*!< Filter bypass of IOCNT3 */
#define IOCNT_CTRL_FLT_BYPASS4               (BIT(21))                                           /*!< Filter bypass of IOCNT4 */
#define IOCNT_CTRL_FLT_CFG                   (BITS(25,31))                                       /*!< IOCNT4~IOCNT0 filter cycle configuration */

/* IOCNT_COPL */
#define IOCNT_COPL_COPL0                     (BIT(0))                                            /*!< IOCNT0 output pulse confige bit */
#define IOCNT_COPL_COPL1                     (BIT(1))                                            /*!< IOCNT1 output pulse confige bit */
#define IOCNT_COPL_COPL2                     (BIT(2))                                            /*!< IOCNT2 output pulse confige bit */
#define IOCNT_COPL_COPL3                     (BIT(3))                                            /*!< IOCNT3 output pulse confige bit */
#define IOCNT_COPL_COPL4                     (BIT(4))                                            /*!< IOCNT4 output pulse confige bit */

/* IOCNT_CMD */
#define IOCNT_CMD_IOCNT0_KICK                (BIT(0))                                            /*!< IOCNT8 software pulse trigger bit */
#define IOCNT_CMD_IOCNT1_KICK                (BIT(1))                                            /*!< IOCNT7 software pulse trigger bit */
#define IOCNT_CMD_IOCNT2_KICK                (BIT(2))                                            /*!< IOCNT6 software pulse trigger bit */
#define IOCNT_CMD_IOCNT3_KICK                (BIT(3))                                            /*!< IOCNT5 software pulse trigger bit */
#define IOCNT_CMD_IOCNT4_KICK                (BIT(4))                                            /*!< IOCNT4 software pulse trigger bit */

/* IOCNT_REMD0 */
#define IOCNT_REMD0_REMD0                    (BITS(0,10))                                        /*!< IOCNT0 input pulse remainder bits */

/* IOCNT_REMD1 */
#define IOCNT_REMD1_REMD1                    (BITS(0,10))                                        /*!< IOCNT1 input pulse remainder bits */

/* IOCNT_REMD2 */
#define IOCNT_REMD2_REMD2                    (BITS(0,10))                                        /*!< IOCNT2 input pulse remainder bits */

/* IOCNT_REMD3 */
#define IOCNT_REMD3_REMD3                    (BITS(0,10))                                        /*!< IOCNT3 input pulse remainder bits */

/* IOCNT_REMD4 */
#define IOCNT_REMD4_REMD4                    (BITS(0,10))                                        /*!< IOCNT4 input pulse remainder bits */

/* constants definitions */
/* IOCNT_CFG0_CNT */
#define IOCNT_CFG0_CNT_POS                   (0U)                                                /*!< IOCNT_CFG0_CNT bit offset */

/* IOCNT_CFG0_CFG */
#define IOCNT_CFG0_CFG_POS                   (20U)                                               /*!< IOCNT_CFG0_CFG bit offset */

/* IOCNT_CFG0_INT */
#define IOCNT_CFG0_INT_POS                   (31U)                                               /*!< IOCNT_CFG0_INT bit offset */
#define IOCNT_CFG0_INT_NORM                  ((uint32_t)0x00000000U)                             /*!< Output pulse level direction follow with input pulse level */
#define IOCNT_CFG0_INT_INT                   (IOCNT_CFG0_INT)                                    /*!< Invert output pulse level */

/* IOCNT_CFG1_CNT */
#define IOCNT_CFG1_CNT_POS                   (0U)                                                /*!< IOCNT_CFG1_CNT bit offset */

/* IOCNT_CFG1_CFG */
#define IOCNT_CFG1_CFG_POS                   (20U)                                               /*!< IOCNT_CFG1_CFG bit offset */

/* IOCNT_CFG1_INT */
#define IOCNT_CFG1_INT_POS                   (31U)                                               /*!< IOCNT_CFG1_INT bit offset */
#define IOCNT_CFG1_INT_NORM                  ((uint32_t)0x00000000U)                             /*!< Output pulse level direction follow with input pulse level */
#define IOCNT_CFG1_INT_INT                   (IOCNT_CFG1_INT)                                    /*!< Invert output pulse level */

/* IOCNT_CFG2_CNT */
#define IOCNT_CFG2_CNT_POS                   (0U)                                                /*!< IOCNT_CFG2_CNT bit offset */

/* IOCNT_CFG2_CFG */
#define IOCNT_CFG2_CFG_POS                   (20U)                                               /*!< IOCNT_CFG2_CFG bit offset */

/* IOCNT_CFG2_INT */
#define IOCNT_CFG2_INT_POS                   (31U)                                               /*!< IOCNT_CFG2_INT bit offset */
#define IOCNT_CFG2_INT_NORM                  ((uint32_t)0x00000000U)                             /*!< Output pulse level direction follow with input pulse level */
#define IOCNT_CFG2_INT_INT                   (IOCNT_CFG2_INT)                                    /*!< Invert output pulse level */

/* IOCNT_CFG3_CNT */
#define IOCNT_CFG3_CNT_POS                   (0U)                                                /*!< IOCNT_CFG3_CNT bit offset */

/* IOCNT_CFG3_CFG */
#define IOCNT_CFG3_CFG_POS                   (20U)                                               /*!< IOCNT_CFG3_CFG bit offset */

/* IOCNT_CFG3_INT */
#define IOCNT_CFG3_INT_POS                   (31U)                                               /*!< IOCNT_CFG3_INT bit offset */
#define IOCNT_CFG3_INT_NORM                  ((uint32_t)0x00000000U)                             /*!< Output pulse level direction follow with input pulse level */
#define IOCNT_CFG3_INT_INT                   (IOCNT_CFG3_INT)                                    /*!< Invert output pulse level */

/* IOCNT_CFG4_CNT */
#define IOCNT_CFG4_CNT_POS                   (0U)                                                /*!< IOCNT_CFG4_CNT bit offset */

/* IOCNT_CFG4_CFG */
#define IOCNT_CFG4_CFG_POS                   (20U)                                               /*!< IOCNT_CFG4_CFG bit offset */

/* IOCNT_CFG4_INT */
#define IOCNT_CFG4_INT_POS                   (31U)                                               /*!< IOCNT_CFG4_INT bit offset */
#define IOCNT_CFG4_INT_NORM                  ((uint32_t)0x00000000U)                             /*!< Output pulse level direction follow with input pulse level */
#define IOCNT_CFG4_INT_INT                   (IOCNT_CFG4_INT)                                    /*!< Invert output pulse level */

/* IOCNT_OUT0_CNT */
#define IOCNT_OUT0_CNT_POS                   (0U)                                                /*!< IOCNT_OUT0_CNT bit offset */

/* IOCNT_OUT1_CNT */
#define IOCNT_OUT1_CNT_POS                   (0U)                                                /*!< IOCNT_OUT1_CNT bit offset */

/* IOCNT_OUT2_CNT */
#define IOCNT_OUT2_CNT_POS                   (0U)                                                /*!< IOCNT_OUT2_CNT bit offset */

/* IOCNT_OUT3_CNT */
#define IOCNT_OUT3_CNT_POS                   (0U)                                                /*!< IOCNT_OUT3_CNT bit offset */

/* IOCNT_OUT4_CNT */
#define IOCNT_OUT4_CNT_POS                   (0U)                                                /*!< IOCNT_OUT4_CNT bit offset */

/* IOCNT_CHNL_IOCNT0_SEL */
#define IOCNT_CHNL_IOCNT0_SEL_POS            (0U)                                                /*!< IOCNT_CHNL_IOCNT0_SEL bit offset */

/* IOCNT_CHNL_IOCNT1_SEL */
#define IOCNT_CHNL_IOCNT1_SEL_POS            (6U)                                                /*!< IOCNT_CHNL_IOCNT1_SEL bit offset */

/* IOCNT_CHNL_IOCNT2_SEL */
#define IOCNT_CHNL_IOCNT2_SEL_POS            (12U)                                               /*!< IOCNT_CHNL_IOCNT2_SEL bit offset */

/* IOCNT_CHNL_IOCNT3_SEL */
#define IOCNT_CHNL_IOCNT3_SEL_POS            (18U)                                               /*!< IOCNT_CHNL_IOCNT3_SEL bit offset */

/* IOCNT_CHNL_IOCNT4_SEL */
#define IOCNT_CHNL_IOCNT4_SEL_POS            (24U)                                               /*!< IOCNT_CHNL_IOCNT4_SEL bit offset */

/* IOCNT_CTRL_EN0 */
#define IOCNT_CTRL_EN0_POS                   (0U)                                                /*!< IOCNT_CTRL_EN0 bit offset */

/* IOCNT_CTRL_EN1 */
#define IOCNT_CTRL_EN1_POS                   (1U)                                                /*!< IOCNT_CTRL_EN1 bit offset */

/* IOCNT_CTRL_EN2 */
#define IOCNT_CTRL_EN2_POS                   (2U)                                                /*!< IOCNT_CTRL_EN2 bit offset */

/* IOCNT_CTRL_EN3 */
#define IOCNT_CTRL_EN3_POS                   (3U)                                                /*!< IOCNT_CTRL_EN3 bit offset */

/* IOCNT_CTRL_EN4 */
#define IOCNT_CTRL_EN4_POS                   (4U)                                                /*!< IOCNT_CTRL_EN4 bit offset */

/* IOCNT_CTRL_MODE0 */
#define IOCNT_CTRL_MODE0_POS                 (8U)                                                /*!< IOCNT_CTRL_MODE0 bit offset */
#define IOCNT_CTRL_MODE0_RISEEDGE            ((uint32_t)0x00000000U)                             /*!< Rising edge triggered */
#define IOCNT_CTRL_MODE0_FALLEDGE            (IOCNT_CTRL_MODE0)                                  /*!< Falling edge triggered */

/* IOCNT_CTRL_MODE1 */
#define IOCNT_CTRL_MODE1_POS                 (9U)                                                /*!< IOCNT_CTRL_MODE1 bit offset */
#define IOCNT_CTRL_MODE1_RISEEDGE            ((uint32_t)0x00000000U)                             /*!< Rising edge triggered */
#define IOCNT_CTRL_MODE1_FALLEDGE            (IOCNT_CTRL_MODE1)                                  /*!< Falling edge triggered */

/* IOCNT_CTRL_MODE2 */
#define IOCNT_CTRL_MODE2_POS                 (10U)                                               /*!< IOCNT_CTRL_MODE2 bit offset */
#define IOCNT_CTRL_MODE2_RISEEDGE            ((uint32_t)0x00000000U)                             /*!< Rising edge triggered */
#define IOCNT_CTRL_MODE2_FALLEDGE            (IOCNT_CTRL_MODE2)                                  /*!< Falling edge triggered */

/* IOCNT_CTRL_MODE3 */
#define IOCNT_CTRL_MODE3_POS                 (11U)                                               /*!< IOCNT_CTRL_MODE3 bit offset */
#define IOCNT_CTRL_MODE3_RISEEDGE            ((uint32_t)0x00000000U)                             /*!< Rising edge triggered */
#define IOCNT_CTRL_MODE3_FALLEDGE            (IOCNT_CTRL_MODE3)                                  /*!< Falling edge triggered */

/* IOCNT_CTRL_MODE4 */
#define IOCNT_CTRL_MODE4_POS                 (12U)                                               /*!< IOCNT_CTRL_MODE4 bit offset */
#define IOCNT_CTRL_MODE4_RISEEDGE            ((uint32_t)0x00000000U)                             /*!< Rising edge triggered */
#define IOCNT_CTRL_MODE4_FALLEDGE            (IOCNT_CTRL_MODE4)                                  /*!< Falling edge triggered */

/* IOCNT_CTRL_CNT_CLR */
#define IOCNT_CTRL_CNT_CLR_POS               (16U)                                               /*!< IOCNT_CTRL_CNT_CLR bit offset */
#define IOCNT_CTRL_CNT_CLR_R2CLR             ((uint32_t)0x00000000U)                             /*!< Read to clear type */
#define IOCNT_CTRL_CNT_CLR_COUNT             (IOCNT_CTRL_CNT_CLR)                                /*!< Continue to count after read */

/* IOCNT_CTRL_FLT_BYPASS0 */
#define IOCNT_CTRL_FLT_BYPASS0_POS           (17U)                                               /*!< IOCNT_CTRL_FLT_BYPASS0 bit offset */

/* IOCNT_CTRL_FLT_BYPASS1 */
#define IOCNT_CTRL_FLT_BYPASS1_POS           (18U)                                               /*!< IOCNT_CTRL_FLT_BYPASS1 bit offset */

/* IOCNT_CTRL_FLT_BYPASS2 */
#define IOCNT_CTRL_FLT_BYPASS2_POS           (19U)                                               /*!< IOCNT_CTRL_FLT_BYPASS2 bit offset */

/* IOCNT_CTRL_FLT_BYPASS3 */
#define IOCNT_CTRL_FLT_BYPASS3_POS           (20U)                                               /*!< IOCNT_CTRL_FLT_BYPASS3 bit offset */

/* IOCNT_CTRL_FLT_BYPASS4 */
#define IOCNT_CTRL_FLT_BYPASS4_POS           (21U)                                               /*!< IOCNT_CTRL_FLT_BYPASS4 bit offset */

/* IOCNT_CTRL_FLT_CFG */
#define IOCNT_CTRL_FLT_CFG_POS               (25U)                                               /*!< IOCNT_CTRL_FLT_CFG bit offset */

/* IOCNT_COPL_COPL0 */
#define IOCNT_COPL_COPL0_POS                 (0U)                                                /*!< IOCNT_COPL_COPL0 bit offset */
#define IOCNT_COPL_COPL0_0                   ((uint32_t)0x00000000U)                             /*!< Output the (n+1)th pulse at the (n*CNT+1)th input pulse */
#define IOCNT_COPL_COPL0_1                   (IOCNT_COPL_COPL0)                                  /*!< Output the (n)th pulse at the (n*CNT)th input pulse */

/* IOCNT_COPL_COPL1 */
#define IOCNT_COPL_COPL1_POS                 (1U)                                                /*!< IOCNT_COPL_COPL1 bit offset */
#define IOCNT_COPL_COPL1_0                   ((uint32_t)0x00000000U)                             /*!< Output the (n+1)th pulse at the (n*CNT+1)th input pulse */
#define IOCNT_COPL_COPL1_1                   (IOCNT_COPL_COPL1)                                  /*!< Output the (n)th pulse at the (n*CNT)th input pulse */

/* IOCNT_COPL_COPL2 */
#define IOCNT_COPL_COPL2_POS                 (2U)                                                /*!< IOCNT_COPL_COPL2 bit offset */
#define IOCNT_COPL_COPL2_0                   ((uint32_t)0x00000000U)                             /*!< Output the (n+1)th pulse at the (n*CNT+1)th input pulse */
#define IOCNT_COPL_COPL2_1                   (IOCNT_COPL_COPL2)                                  /*!< Output the (n)th pulse at the (n*CNT)th input pulse */

/* IOCNT_COPL_COPL3 */
#define IOCNT_COPL_COPL3_POS                 (3U)                                                /*!< IOCNT_COPL_COPL3 bit offset */
#define IOCNT_COPL_COPL3_0                   ((uint32_t)0x00000000U)                             /*!< Output the (n+1)th pulse at the (n*CNT+1)th input pulse */
#define IOCNT_COPL_COPL3_1                   (IOCNT_COPL_COPL3)                                  /*!< Output the (n)th pulse at the (n*CNT)th input pulse */

/* IOCNT_COPL_COPL4 */
#define IOCNT_COPL_COPL4_POS                 (4U)                                                /*!< IOCNT_COPL_COPL4 bit offset */
#define IOCNT_COPL_COPL4_0                   ((uint32_t)0x00000000U)                             /*!< Output the (n+1)th pulse at the (n*CNT+1)th input pulse */
#define IOCNT_COPL_COPL4_1                   (IOCNT_COPL_COPL4)                                  /*!< Output the (n)th pulse at the (n*CNT)th input pulse */

/* IOCNT_CMD_IOCNT0_KICK */
#define IOCNT_CMD_IOCNT0_KICK_POS            (0U)                                                /*!< IOCNT_CMD_IOCNT0_KICK bit offset */

/* IOCNT_CMD_IOCNT1_KICK */
#define IOCNT_CMD_IOCNT1_KICK_POS            (1U)                                                /*!< IOCNT_CMD_IOCNT1_KICK bit offset */

/* IOCNT_CMD_IOCNT2_KICK */
#define IOCNT_CMD_IOCNT2_KICK_POS            (2U)                                                /*!< IOCNT_CMD_IOCNT2_KICK bit offset */

/* IOCNT_CMD_IOCNT3_KICK */
#define IOCNT_CMD_IOCNT3_KICK_POS            (3U)                                                /*!< IOCNT_CMD_IOCNT3_KICK bit offset */

/* IOCNT_CMD_IOCNT4_KICK */
#define IOCNT_CMD_IOCNT4_KICK_POS            (4U)                                                /*!< IOCNT_CMD_IOCNT4_KICK bit offset */

/* IOCNT_REMD0_REMD0 */
#define IOCNT_REMD0_REMD0_POS                (0U)                                                /*!< IOCNT_REMD0_REMD0 bit offset */

/* IOCNT_REMD1_REMD1 */
#define IOCNT_REMD1_REMD1_POS                (0U)                                                /*!< IOCNT_REMD1_REMD1 bit offset */

/* IOCNT_REMD2_REMD2 */
#define IOCNT_REMD2_REMD2_POS                (0U)                                                /*!< IOCNT_REMD2_REMD2 bit offset */

/* IOCNT_REMD3_REMD3 */
#define IOCNT_REMD3_REMD3_POS                (0U)                                                /*!< IOCNT_REMD3_REMD3 bit offset */

/* IOCNT_REMD4_REMD4 */
#define IOCNT_REMD4_REMD4_POS                (0U)                                                /*!< IOCNT_REMD4_REMD4 bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_IOCNT_H */
