/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_lvdcmp.h
 * @author  Renergy Software Team
 * @brief   LVDCMP module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_LVDCMP_H
#define _RN831x_RN861x_MCU_LVDCMP_H

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

/* LVDCMP */
typedef struct
{
    __IO uint32_t LVDCTRL;                   /*!< offset: 0x000 info: LVD Control Register */
    __IO uint32_t CMPCTRL;                   /*!< offset: 0x004 info: CMP Control Register */
    __IO uint32_t IE;                        /*!< offset: 0x008 info: CMP, LVD Interrupt Enable Register */
    __IO uint32_t IF;                        /*!< offset: 0x00C info: CMP, LVD Interrupt Flag Register */
    __IO uint32_t LVDPWD;                    /*!< offset: 0x010 info: LVD Power Down Register */
    __IO uint32_t CMP2PWD;                   /*!< offset: 0x014 info: CMP2 Power Down Register */
    __IO uint32_t CMP1PWD;                   /*!< offset: 0x018 info: CMP1 Power Down Register */
} LVDCMP_TypeDef;

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
#define LVDCMP_CORE0_BASE (0x200BC000U)
#define LVDCMP_CORE1_BASE (0xA00BC000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define LVDCMP_CORE0 ((LVDCMP_TypeDef *)   LVDCMP_CORE0_BASE)
#define LVDCMP_CORE1 ((LVDCMP_TypeDef *)   LVDCMP_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define LVDCMP  LVDCMP_CORE0
#endif
#if defined(CORE1)
#define LVDCMP  LVDCMP_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** LVDCMP Instances *******************************/
#define IS_LVDCMP_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == LVDCMP_CORE0) || \
                                             ((INSTANCE) == LVDCMP_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* LVDCMP_LVDCTRL */
#define LVDCMP_LVDCTRL_LVDS                  (BITS(0,3))                                         /*!< LVD threshold voltage set bits */
#define LVDCMP_LVDCTRL_LVD_FLT_CNT           (BITS(4,11))                                        /*!< The filter can be configured from 0 to 255 clock cycles. Setting to 0 disables the filter. */

/* LVDCMP_CMPCTRL */
#define LVDCMP_CMPCTRL_MODE_CMP1R            (BIT(0))                                            /*!< switch bit of CMP1 internal 600K simpling resistor(=1 register off, =0 register on) */
#define LVDCMP_CMPCTRL_MODE_CMP2R            (BIT(1))                                            /*!< switch bit of CMP2 internal 600K simpling resistor(=1 register off, =0 register on) */
#define LVDCMP_CMPCTRL_HYSEN_CMP1            (BIT(2))                                            /*!< CMP1 hysteresis switch bit */
#define LVDCMP_CMPCTRL_HYSEN_CMP2            (BIT(3))                                            /*!< CMP2 hysteresis switch bit */
#define LVDCMP_CMPCTRL_CMP1_FLT_CNT          (BITS(4,11))                                        /*!< The filter can be configured from 0 to 255 clock cycles. Setting to 0 disables the filter. */
#define LVDCMP_CMPCTRL_CMP2_FLT_CNT          (BITS(12,19))                                       /*!< The filter can be configured from 0 to 255 clock cycles. Setting to 0 disables the filter. */

/* LVDCMP_IE */
#define LVDCMP_IE_CMP1IE                     (BIT(0))                                            /*!< comparator No.1 interrupt enable bit */
#define LVDCMP_IE_CMP2IE                     (BIT(1))                                            /*!< comparator No.2 interrupt enable bit */
#define LVDCMP_IE_LVDIE                      (BIT(2))                                            /*!< LVD interrupt enable bit */

/* LVDCMP_IF */
#define LVDCMP_IF_CMP1IF                     (BIT(0))                                            /*!< comparator NO.1 interrupt flag */
#define LVDCMP_IF_CMP2IF                     (BIT(1))                                            /*!< comparator No.2 interrupt flag */
#define LVDCMP_IF_LVDIF                      (BIT(2))                                            /*!< LVD interrupt flag */

/* LVDCMP_LVDPWD */
#define LVDCMP_LVDPWD_LVDPWD                 (BITS(0,31))                                        /*!< LVD Power Down Register bits */

/* LVDCMP_CMP2PWD */
#define LVDCMP_CMP2PWD_CMP2PWD               (BITS(0,31))                                        /*!< CMP2 Power Down Register bits */

/* LVDCMP_CMP1PWD */
#define LVDCMP_CMP1PWD_CMP1PWD               (BITS(0,31))                                        /*!< CMP1 Power Down Register bits */

/* constants definitions */
/* LVDCMP_LVDCTRL_LVDS */
#define LVDCMP_LVDCTRL_LVDS_POS              (0U)                                                /*!< LVDCMP_LVDCTRL_LVDS bit offset */
#define LVDCMP_LVDCTRL_LVDS_CFG(regval)  (LVDCMP_LVDCTRL_LVDS & ((uint32_t)(regval) << LVDCMP_LVDCTRL_LVDS_POS))
#define LVDCMP_LVDCTRL_LVDS_1P9              (LVDCMP_LVDCTRL_LVDS_CFG(0))                        /*!< select 1.9 as LVD Vil, 2.0 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_2P1              (LVDCMP_LVDCTRL_LVDS_CFG(1))                        /*!< select 2.1 as LVD Vil, 2.2 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_2P3              (LVDCMP_LVDCTRL_LVDS_CFG(2))                        /*!< select 2.3 as LVD Vil, 2.4 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_2P5              (LVDCMP_LVDCTRL_LVDS_CFG(3))                        /*!< select 2.5 as LVD Vil, 2.6 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_2P7              (LVDCMP_LVDCTRL_LVDS_CFG(4))                        /*!< select 2.7 as LVD Vil, 2.8 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_2P9              (LVDCMP_LVDCTRL_LVDS_CFG(5))                        /*!< select 2.9 as LVD Vil, 3.0 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_3P1              (LVDCMP_LVDCTRL_LVDS_CFG(6))                        /*!< select 3.1 as LVD Vil, 3.2 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_3P3              (LVDCMP_LVDCTRL_LVDS_CFG(7))                        /*!< select 3.3 as LVD Vil, 3.4 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_3P5              (LVDCMP_LVDCTRL_LVDS_CFG(8))                        /*!< select 3.5 as LVD Vil, 3.6 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_3P7              (LVDCMP_LVDCTRL_LVDS_CFG(9))                        /*!< select 3.7 as LVD Vil, 3.8 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_3P9              (LVDCMP_LVDCTRL_LVDS_CFG(10))                       /*!< select 3.9 as LVD Vil, 4.0 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_4P1              (LVDCMP_LVDCTRL_LVDS_CFG(11))                       /*!< select 4.1 as LVD Vil, 4.2 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_4P3              (LVDCMP_LVDCTRL_LVDS_CFG(12))                       /*!< select 4.3 as LVD Vil, 4.4 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_4P5              (LVDCMP_LVDCTRL_LVDS_CFG(13))                       /*!< select 4.5 as LVD Vil, 4.6 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_4P7              (LVDCMP_LVDCTRL_LVDS_CFG(14))                       /*!< select 4.7 as LVD Vil, 4.8 as LVD Vih */
#define LVDCMP_LVDCTRL_LVDS_LVDIN            (LVDCMP_LVDCTRL_LVDS_CFG(15))                       /*!< select 1.25 as LVD Vil, 1.45 as LVD Vih ,and compare with voltage found on LVDIN pin */

/* LVDCMP_LVDCTRL_LVD_FLT_CNT */
#define LVDCMP_LVDCTRL_LVD_FLT_CNT_POS       (4U)                                                /*!< LVDCMP_LVDCTRL_LVD_FLT_CNT bit offset */

/* LVDCMP_CMPCTRL_MODE_CMP1R */
#define LVDCMP_CMPCTRL_MODE_CMP1R_POS        (0U)                                                /*!< LVDCMP_CMPCTRL_MODE_CMP1R bit offset */

/* LVDCMP_CMPCTRL_MODE_CMP2R */
#define LVDCMP_CMPCTRL_MODE_CMP2R_POS        (1U)                                                /*!< LVDCMP_CMPCTRL_MODE_CMP2R bit offset */

/* LVDCMP_CMPCTRL_HYSEN_CMP1 */
#define LVDCMP_CMPCTRL_HYSEN_CMP1_POS        (2U)                                                /*!< LVDCMP_CMPCTRL_HYSEN_CMP1 bit offset */

/* LVDCMP_CMPCTRL_HYSEN_CMP2 */
#define LVDCMP_CMPCTRL_HYSEN_CMP2_POS        (3U)                                                /*!< LVDCMP_CMPCTRL_HYSEN_CMP2 bit offset */

/* LVDCMP_CMPCTRL_CMP1_FLT_CNT */
#define LVDCMP_CMPCTRL_CMP1_FLT_CNT_POS      (4U)                                                /*!< LVDCMP_CMPCTRL_CMP1_FLT_CNT bit offset */

/* LVDCMP_CMPCTRL_CMP2_FLT_CNT */
#define LVDCMP_CMPCTRL_CMP2_FLT_CNT_POS      (12U)                                               /*!< LVDCMP_CMPCTRL_CMP2_FLT_CNT bit offset */

/* LVDCMP_IE_CMP1IE */
#define LVDCMP_IE_CMP1IE_POS                 (0U)                                                /*!< LVDCMP_IE_CMP1IE bit offset */

/* LVDCMP_IE_CMP2IE */
#define LVDCMP_IE_CMP2IE_POS                 (1U)                                                /*!< LVDCMP_IE_CMP2IE bit offset */

/* LVDCMP_IE_LVDIE */
#define LVDCMP_IE_LVDIE_POS                  (2U)                                                /*!< LVDCMP_IE_LVDIE bit offset */

/* LVDCMP_IF_CMP1IF */
#define LVDCMP_IF_CMP1IF_POS                 (0U)                                                /*!< LVDCMP_IF_CMP1IF bit offset */

/* LVDCMP_IF_CMP2IF */
#define LVDCMP_IF_CMP2IF_POS                 (1U)                                                /*!< LVDCMP_IF_CMP2IF bit offset */

/* LVDCMP_IF_LVDIF */
#define LVDCMP_IF_LVDIF_POS                  (2U)                                                /*!< LVDCMP_IF_LVDIF bit offset */

/* LVDCMP_LVDPWD_LVDPWD */
#define LVDCMP_LVDPWD_LVDPWD_POS             (0U)                                                /*!< LVDCMP_LVDPWD_LVDPWD bit offset */
#define LVDCMP_LVDPWD_LVDPWD_PWDN            ((uint32_t)0x55AAAA55U)                             /*!< LVD off,Power down */
#define LVDCMP_LVDPWD_LVDPWD_EN              ((uint32_t)0x00000000U)                             /*!< LVD enable */

/* LVDCMP_CMP2PWD_CMP2PWD */
#define LVDCMP_CMP2PWD_CMP2PWD_POS           (0U)                                                /*!< LVDCMP_CMP2PWD_CMP2PWD bit offset */
#define LVDCMP_CMP2PWD_CMP2PWD_PWDN          ((uint32_t)0xAA5555AAU)                             /*!< cmp off,Power down */
#define LVDCMP_CMP2PWD_CMP2PWD_EN            ((uint32_t)0x00000000U)                             /*!< cmp enable */

/* LVDCMP_CMP1PWD_CMP1PWD */
#define LVDCMP_CMP1PWD_CMP1PWD_POS           (0U)                                                /*!< LVDCMP_CMP1PWD_CMP1PWD bit offset */
#define LVDCMP_CMP1PWD_CMP1PWD_PWDN          ((uint32_t)0xA5A55A5AU)                             /*!< cmp off,Power down */
#define LVDCMP_CMP1PWD_CMP1PWD_EN            ((uint32_t)0x00000000U)                             /*!< cmp enable */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_LVDCMP_H */
