/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_simptc.h
 * @author  Renergy Software Team
 * @brief   SIMPTC module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_SIMPTC_H
#define _RN831x_RN861x_MCU_SIMPTC_H

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

/* SIMPTC */
typedef struct
{
    __IO uint32_t CTRL;                      /*!< offset: 0x000 info: SIMPTC control register */
    __IO uint32_t DN;                        /*!< offset: 0x004 info: SIMPTC target counter register */
    __IO uint32_t VAL;                       /*!< offset: 0x008 info: SIMPTC counter register bits */
    __IO uint32_t IF;                        /*!< offset: 0x00C info: SIMPTC interrupt flag bits */
} SIMPTC_TypeDef;

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
#define SIMPTC0_0_CORE0_BASE (0x200A4000U)
#define SIMPTC0_1_CORE0_BASE (0x200A4010U)
#define SIMPTC0_2_CORE0_BASE (0x200A4020U)
#define SIMPTC0_3_CORE0_BASE (0x200A4030U)
#define SIMPTC1_0_CORE0_BASE (0x200A8000U)
#define SIMPTC1_1_CORE0_BASE (0x200A8010U)
#define SIMPTC1_2_CORE0_BASE (0x200A8020U)
#define SIMPTC1_3_CORE0_BASE (0x200A8030U)
#define SIMPTC2_0_CORE0_BASE (0x200AC000U)
#define SIMPTC2_1_CORE0_BASE (0x200AC010U)
#define SIMPTC2_2_CORE0_BASE (0x200AC020U)
#define SIMPTC2_3_CORE0_BASE (0x200AC030U)
#define SIMPTC3_0_CORE0_BASE (0x200B0000U)
#define SIMPTC3_1_CORE0_BASE (0x200B0010U)
#define SIMPTC3_2_CORE0_BASE (0x200B0020U)
#define SIMPTC3_3_CORE0_BASE (0x200B0030U)
#define SIMPTC0_0_CORE1_BASE (0xA00A4000U)
#define SIMPTC0_1_CORE1_BASE (0xA00A4010U)
#define SIMPTC0_2_CORE1_BASE (0xA00A4020U)
#define SIMPTC0_3_CORE1_BASE (0xA00A4030U)
#define SIMPTC1_0_CORE1_BASE (0xA00A8000U)
#define SIMPTC1_1_CORE1_BASE (0xA00A8010U)
#define SIMPTC1_2_CORE1_BASE (0xA00A8020U)
#define SIMPTC1_3_CORE1_BASE (0xA00A8030U)
#define SIMPTC2_0_CORE1_BASE (0xA00AC000U)
#define SIMPTC2_1_CORE1_BASE (0xA00AC010U)
#define SIMPTC2_2_CORE1_BASE (0xA00AC020U)
#define SIMPTC2_3_CORE1_BASE (0xA00AC030U)
#define SIMPTC3_0_CORE1_BASE (0xA00B0000U)
#define SIMPTC3_1_CORE1_BASE (0xA00B0010U)
#define SIMPTC3_2_CORE1_BASE (0xA00B0020U)
#define SIMPTC3_3_CORE1_BASE (0xA00B0030U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define SIMPTC0_0_CORE0 ((SIMPTC_TypeDef *)   SIMPTC0_0_CORE0_BASE)
#define SIMPTC0_1_CORE0 ((SIMPTC_TypeDef *)   SIMPTC0_1_CORE0_BASE)
#define SIMPTC0_2_CORE0 ((SIMPTC_TypeDef *)   SIMPTC0_2_CORE0_BASE)
#define SIMPTC0_3_CORE0 ((SIMPTC_TypeDef *)   SIMPTC0_3_CORE0_BASE)
#define SIMPTC1_0_CORE0 ((SIMPTC_TypeDef *)   SIMPTC1_0_CORE0_BASE)
#define SIMPTC1_1_CORE0 ((SIMPTC_TypeDef *)   SIMPTC1_1_CORE0_BASE)
#define SIMPTC1_2_CORE0 ((SIMPTC_TypeDef *)   SIMPTC1_2_CORE0_BASE)
#define SIMPTC1_3_CORE0 ((SIMPTC_TypeDef *)   SIMPTC1_3_CORE0_BASE)
#define SIMPTC2_0_CORE0 ((SIMPTC_TypeDef *)   SIMPTC2_0_CORE0_BASE)
#define SIMPTC2_1_CORE0 ((SIMPTC_TypeDef *)   SIMPTC2_1_CORE0_BASE)
#define SIMPTC2_2_CORE0 ((SIMPTC_TypeDef *)   SIMPTC2_2_CORE0_BASE)
#define SIMPTC2_3_CORE0 ((SIMPTC_TypeDef *)   SIMPTC2_3_CORE0_BASE)
#define SIMPTC3_0_CORE0 ((SIMPTC_TypeDef *)   SIMPTC3_0_CORE0_BASE)
#define SIMPTC3_1_CORE0 ((SIMPTC_TypeDef *)   SIMPTC3_1_CORE0_BASE)
#define SIMPTC3_2_CORE0 ((SIMPTC_TypeDef *)   SIMPTC3_2_CORE0_BASE)
#define SIMPTC3_3_CORE0 ((SIMPTC_TypeDef *)   SIMPTC3_3_CORE0_BASE)
#define SIMPTC0_0_CORE1 ((SIMPTC_TypeDef *)   SIMPTC0_0_CORE1_BASE)
#define SIMPTC0_1_CORE1 ((SIMPTC_TypeDef *)   SIMPTC0_1_CORE1_BASE)
#define SIMPTC0_2_CORE1 ((SIMPTC_TypeDef *)   SIMPTC0_2_CORE1_BASE)
#define SIMPTC0_3_CORE1 ((SIMPTC_TypeDef *)   SIMPTC0_3_CORE1_BASE)
#define SIMPTC1_0_CORE1 ((SIMPTC_TypeDef *)   SIMPTC1_0_CORE1_BASE)
#define SIMPTC1_1_CORE1 ((SIMPTC_TypeDef *)   SIMPTC1_1_CORE1_BASE)
#define SIMPTC1_2_CORE1 ((SIMPTC_TypeDef *)   SIMPTC1_2_CORE1_BASE)
#define SIMPTC1_3_CORE1 ((SIMPTC_TypeDef *)   SIMPTC1_3_CORE1_BASE)
#define SIMPTC2_0_CORE1 ((SIMPTC_TypeDef *)   SIMPTC2_0_CORE1_BASE)
#define SIMPTC2_1_CORE1 ((SIMPTC_TypeDef *)   SIMPTC2_1_CORE1_BASE)
#define SIMPTC2_2_CORE1 ((SIMPTC_TypeDef *)   SIMPTC2_2_CORE1_BASE)
#define SIMPTC2_3_CORE1 ((SIMPTC_TypeDef *)   SIMPTC2_3_CORE1_BASE)
#define SIMPTC3_0_CORE1 ((SIMPTC_TypeDef *)   SIMPTC3_0_CORE1_BASE)
#define SIMPTC3_1_CORE1 ((SIMPTC_TypeDef *)   SIMPTC3_1_CORE1_BASE)
#define SIMPTC3_2_CORE1 ((SIMPTC_TypeDef *)   SIMPTC3_2_CORE1_BASE)
#define SIMPTC3_3_CORE1 ((SIMPTC_TypeDef *)   SIMPTC3_3_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define SIMPTC0_0  SIMPTC0_0_CORE0
#define SIMPTC0_1  SIMPTC0_1_CORE0
#define SIMPTC0_2  SIMPTC0_2_CORE0
#define SIMPTC0_3  SIMPTC0_3_CORE0
#define SIMPTC1_0  SIMPTC1_0_CORE0
#define SIMPTC1_1  SIMPTC1_1_CORE0
#define SIMPTC1_2  SIMPTC1_2_CORE0
#define SIMPTC1_3  SIMPTC1_3_CORE0
#define SIMPTC2_0  SIMPTC2_0_CORE0
#define SIMPTC2_1  SIMPTC2_1_CORE0
#define SIMPTC2_2  SIMPTC2_2_CORE0
#define SIMPTC2_3  SIMPTC2_3_CORE0
#define SIMPTC3_0  SIMPTC3_0_CORE0
#define SIMPTC3_1  SIMPTC3_1_CORE0
#define SIMPTC3_2  SIMPTC3_2_CORE0
#define SIMPTC3_3  SIMPTC3_3_CORE0
#endif
#if defined(CORE1)
#define SIMPTC0_0  SIMPTC0_0_CORE1
#define SIMPTC0_1  SIMPTC0_1_CORE1
#define SIMPTC0_2  SIMPTC0_2_CORE1
#define SIMPTC0_3  SIMPTC0_3_CORE1
#define SIMPTC1_0  SIMPTC1_0_CORE1
#define SIMPTC1_1  SIMPTC1_1_CORE1
#define SIMPTC1_2  SIMPTC1_2_CORE1
#define SIMPTC1_3  SIMPTC1_3_CORE1
#define SIMPTC2_0  SIMPTC2_0_CORE1
#define SIMPTC2_1  SIMPTC2_1_CORE1
#define SIMPTC2_2  SIMPTC2_2_CORE1
#define SIMPTC2_3  SIMPTC2_3_CORE1
#define SIMPTC3_0  SIMPTC3_0_CORE1
#define SIMPTC3_1  SIMPTC3_1_CORE1
#define SIMPTC3_2  SIMPTC3_2_CORE1
#define SIMPTC3_3  SIMPTC3_3_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** SIMPTC Instances *******************************/
#define IS_SIMPTC_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == SIMPTC0_0_CORE0) || \
                                             ((INSTANCE) == SIMPTC0_1_CORE0) || \
                                             ((INSTANCE) == SIMPTC0_2_CORE0) || \
                                             ((INSTANCE) == SIMPTC0_3_CORE0) || \
                                             ((INSTANCE) == SIMPTC1_0_CORE0) || \
                                             ((INSTANCE) == SIMPTC1_1_CORE0) || \
                                             ((INSTANCE) == SIMPTC1_2_CORE0) || \
                                             ((INSTANCE) == SIMPTC1_3_CORE0) || \
                                             ((INSTANCE) == SIMPTC2_0_CORE0) || \
                                             ((INSTANCE) == SIMPTC2_1_CORE0) || \
                                             ((INSTANCE) == SIMPTC2_2_CORE0) || \
                                             ((INSTANCE) == SIMPTC2_3_CORE0) || \
                                             ((INSTANCE) == SIMPTC3_0_CORE0) || \
                                             ((INSTANCE) == SIMPTC3_1_CORE0) || \
                                             ((INSTANCE) == SIMPTC3_2_CORE0) || \
                                             ((INSTANCE) == SIMPTC3_3_CORE0) || \
                                             ((INSTANCE) == SIMPTC0_0_CORE1) || \
                                             ((INSTANCE) == SIMPTC0_1_CORE1) || \
                                             ((INSTANCE) == SIMPTC0_2_CORE1) || \
                                             ((INSTANCE) == SIMPTC0_3_CORE1) || \
                                             ((INSTANCE) == SIMPTC1_0_CORE1) || \
                                             ((INSTANCE) == SIMPTC1_1_CORE1) || \
                                             ((INSTANCE) == SIMPTC1_2_CORE1) || \
                                             ((INSTANCE) == SIMPTC1_3_CORE1) || \
                                             ((INSTANCE) == SIMPTC2_0_CORE1) || \
                                             ((INSTANCE) == SIMPTC2_1_CORE1) || \
                                             ((INSTANCE) == SIMPTC2_2_CORE1) || \
                                             ((INSTANCE) == SIMPTC2_3_CORE1) || \
                                             ((INSTANCE) == SIMPTC3_0_CORE1) || \
                                             ((INSTANCE) == SIMPTC3_1_CORE1) || \
                                             ((INSTANCE) == SIMPTC3_2_CORE1) || \
                                             ((INSTANCE) == SIMPTC3_3_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* SIMPTC_CTRL */
#define SIMPTC_CTRL_CNT_SFT_EN               (BIT(0))                                            /*!< SIMP_TCx_x counter soft enable bit */
#define SIMPTC_CTRL_CNT_SEL                  (BIT(1))                                            /*!< SIMP_TCx_x counter enable source select */
#define SIMPTC_CTRL_DBGSTBDIS                (BIT(2))                                            /*!< SIMP_TCx_x debug standby disable bit */
#define SIMPTC_CTRL_MODE                     (BIT(3))                                            /*!< SIMP_TCx_x count mode selection bit */
#define SIMPTC_CTRL_IRQEN                    (BIT(4))                                            /*!< SIMP_TCx_x interrupt enable bit */

/* SIMPTC_DN */
#define SIMPTC_DN_DN                         (BITS(0,31))                                        /*!< SIMPTC target counting value setting bits */

/* SIMPTC_VAL */
#define SIMPTC_VAL_VAL                       (BITS(0,31))                                        /*!< SIMPTC current count register bits */

/* SIMPTC_IF */
#define SIMPTC_IF_OV                         (BIT(0))                                            /*!< SIMPTC counter overflow interrrupt flag */

/* constants definitions */
/* SIMPTC_CTRL_CNT_SFT_EN */
#define SIMPTC_CTRL_CNT_SFT_EN_POS           (0U)                                                /*!< SIMPTC_CTRL_CNT_SFT_EN bit offset */
#define SIMPTC_CTRL_CNT_SFT_EN_EN            ((uint32_t)0x00000000U)                             /*!< enable */
#define SIMPTC_CTRL_CNT_SFT_EN_DIS           (SIMPTC_CTRL_CNT_SFT_EN)                            /*!< disable */

/* SIMPTC_CTRL_CNT_SEL */
#define SIMPTC_CTRL_CNT_SEL_POS              (1U)                                                /*!< SIMPTC_CTRL_CNT_SEL bit offset */
#define SIMPTC_CTRL_CNT_SEL_SFT              ((uint32_t)0x00000000U)                             /*!< enable counter source select as soft config */
#define SIMPTC_CTRL_CNT_SEL_AOS              (SIMPTC_CTRL_CNT_SEL)                               /*!< enable counter source select as AOS */

/* SIMPTC_CTRL_DBGSTBDIS */
#define SIMPTC_CTRL_DBGSTBDIS_POS            (2U)                                                /*!< SIMPTC_CTRL_DBGSTBDIS bit offset */
#define SIMPTC_CTRL_DBGSTBDIS_PAUSE          ((uint32_t)0x00000000U)                             /*!< halt counting when SoC halted by debugger */
#define SIMPTC_CTRL_DBGSTBDIS_COUNTINUE      (SIMPTC_CTRL_DBGSTBDIS)                             /*!< keep counting when SoC halted by debugger */

/* SIMPTC_CTRL_MODE */
#define SIMPTC_CTRL_MODE_POS                 (3U)                                                /*!< SIMPTC_CTRL_MODE bit offset */
#define SIMPTC_CTRL_MODE_SINGLE              ((uint32_t)0x00000000U)                             /*!< SIMP_TCx_x select as Single Count */
#define SIMPTC_CTRL_MODE_CONTINUE            (SIMPTC_CTRL_MODE)                                  /*!< SIMP_TCx_x  select as Continuous Count */

/* SIMPTC_CTRL_IRQEN */
#define SIMPTC_CTRL_IRQEN_POS                (4U)                                                /*!< SIMPTC_CTRL_IRQEN bit offset */

/* SIMPTC_DN_DN */
#define SIMPTC_DN_DN_POS                     (0U)                                                /*!< SIMPTC_DN_DN bit offset */

/* SIMPTC_VAL_VAL */
#define SIMPTC_VAL_VAL_POS                   (0U)                                                /*!< SIMPTC_VAL_VAL bit offset */

/* SIMPTC_IF_OV */
#define SIMPTC_IF_OV_POS                     (0U)                                                /*!< SIMPTC_IF_OV bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_SIMPTC_H */
