/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_lsrx.h
 * @author  Renergy Software Team
 * @brief   LSRX module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_LSRX_H
#define _RN831x_RN861x_MCU_LSRX_H

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

/* LSRX */
typedef struct
{
    __IO uint32_t FTOP;                      /*!< offset: 0x000 info: LSRX identify frame header register */
    __IO uint32_t FHD;                       /*!< offset: 0x004 info: LSRX identify frame header enable register */
    __IO uint32_t CTL;                       /*!< offset: 0x008 info: LSRX control register */
    __IO uint32_t CAL_CFG;                   /*!< offset: 0x00C info: Calibration configuration register */
    __IO uint32_t EN;                        /*!< offset: 0x010 info: LSRX enable register */
    __IO uint32_t IE;                        /*!< offset: 0x014 info: LSRX interrupt enable register */
    __IO uint32_t IF;                        /*!< offset: 0x018 info: LSRX interrupt flag register */
    __IO uint32_t CH0FH_TIME;                /*!< offset: 0x01C info: Channel0 frame header time register */
    __IO uint32_t CH1FH_TIME;                /*!< offset: 0x020 info: Channel1 frame header time register */
    __IO uint32_t CH2FH_TIME;                /*!< offset: 0x024 info: Channel2 frame header time register */
    __IO uint32_t CH3FH_TIME;                /*!< offset: 0x028 info: Channel3 frame header time register */
} LSRX_TypeDef;

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
#define LSRX_CORE0_BASE (0x2002C000U)
#define LSRX_CORE1_BASE (0xA002C000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define LSRX_CORE0 ((LSRX_TypeDef *)     LSRX_CORE0_BASE)
#define LSRX_CORE1 ((LSRX_TypeDef *)     LSRX_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define LSRX  LSRX_CORE0
#endif
#if defined(CORE1)
#define LSRX  LSRX_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** LSRX Instances *******************************/
#define IS_LSRX_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == LSRX_CORE0) || \
                                           ((INSTANCE) == LSRX_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* LSRX_FTOP */
#define LSRX_FTOP_FTOP                       (BITS(0,31))                                        /*!< LSRX identify frame header bits */

/* LSRX_FHD */
#define LSRX_FHD_B0CHKEN                     (BIT(0))                                            /*!< First byte check funcstion of frame header enable bit */
#define LSRX_FHD_B0PRTEN                     (BIT(1))                                            /*!< First byte parity function of frame enable bit */
#define LSRX_FHD_B1CHKEN                     (BIT(2))                                            /*!< Second byte of check funcstion of frame header enable bit */
#define LSRX_FHD_B1PRTEN                     (BIT(3))                                            /*!< Second byte parity function of frame enable bit */
#define LSRX_FHD_B2CHKEN                     (BIT(4))                                            /*!< Third byte of check funcstion of frame header enable bit */
#define LSRX_FHD_B2PRTEN                     (BIT(5))                                            /*!< Third byte parity function of frame enable bit */
#define LSRX_FHD_B3CHKEN                     (BIT(6))                                            /*!< Fourth byte of check funcstion of frame header enable bit */
#define LSRX_FHD_B3PRTEN                     (BIT(7))                                            /*!< Fourth byte parity function of frame enable bit */

/* LSRX_CTL */
#define LSRX_CTL_CHL0_SEL                    (BITS(0,3))                                         /*!< Channel0 select bits */
#define LSRX_CTL_CHL1_SEL                    (BITS(4,7))                                         /*!< Channel1 select bits */
#define LSRX_CTL_CHL2_SEL                    (BITS(8,11))                                        /*!< Channel2 select bits */
#define LSRX_CTL_CHL3_SEL                    (BITS(12,15))                                       /*!< Channel3 select bits */
#define LSRX_CTL_IDLE_DET                    (BIT(16))                                           /*!< LSRX idle check enable bit */
#define LSRX_CTL_CHL0EN                      (BIT(17))                                           /*!< Channel0 receive enable bit */
#define LSRX_CTL_CHL1EN                      (BIT(18))                                           /*!< Channel1 receive enable bit */
#define LSRX_CTL_CHL2EN                      (BIT(19))                                           /*!< Channel2 receive enable bit */
#define LSRX_CTL_CHL3EN                      (BIT(20))                                           /*!< Channel3 receive enable bit */
#define LSRX_CTL_TIME_TH                     (BITS(21,25))                                       /*!< Threshold time config bits */

/* LSRX_CAL_CFG */
#define LSRX_CAL_CFG_CAL_CNT                 (BITS(0,22))                                        /*!< Calibration wave output cycle config bits */
#define LSRX_CAL_CFG_PAR_DATA                (BITS(23,30))                                       /*!< Parity data bits of calibration wave */

/* LSRX_EN */
#define LSRX_EN_EN                           (BIT(0))                                            /*!< LSRX enable bit */

/* LSRX_IE */
#define LSRX_IE_CAL_FINISH_IE                (BIT(0))                                            /*!< Calibration wave transmit finish enable bit */
#define LSRX_IE_CAL_UPDATE_IE                (BIT(1))                                            /*!< Calibration update interrupt enable bit */

/* LSRX_IF */
#define LSRX_IF_CAL_FINISH_IF                (BIT(0))                                            /*!< Calibration wave transmit finish interrupt flag bit */
#define LSRX_IF_CHL0_UPDATE_IF               (BIT(1))                                            /*!< Channel0 update interrupt flag bit */
#define LSRX_IF_CHL1_UPDATE_IF               (BIT(2))                                            /*!< Channel1 update interrupt flag bit */
#define LSRX_IF_CHL2_UPDATE_IF               (BIT(3))                                            /*!< Channel2 update interrupt flag bit */
#define LSRX_IF_CHL3_UPDATE_IF               (BIT(4))                                            /*!< Channel3 update interrupt flag bit */
#define LSRX_IF_CHL0_VALID_IF                (BIT(5))                                            /*!< Channel0 receive timeout flag bit */
#define LSRX_IF_CHL1_VALID_IF                (BIT(6))                                            /*!< Channel1 receive timeout flag bit */
#define LSRX_IF_CHL2_VALID_IF                (BIT(7))                                            /*!< Channel2 receive timeout flag bit */
#define LSRX_IF_CHL3_VALID_IF                (BIT(8))                                            /*!< Channel3 receive timeout flag bit */

/* LSRX_CH0FH_TIME */
#define LSRX_CH0FH_TIME_CH0FH_TIME           (BITS(0,24))                                        /*!< Channel0 frame header time bits */

/* LSRX_CH1FH_TIME */
#define LSRX_CH1FH_TIME_CH1FH_TIME           (BITS(0,24))                                        /*!< Channel1 frame header time bits */

/* LSRX_CH2FH_TIME */
#define LSRX_CH2FH_TIME_CH2FH_TIME           (BITS(0,24))                                        /*!< Channel2 frame header time bits */

/* LSRX_CH3FH_TIME */
#define LSRX_CH3FH_TIME_CH3FH_TIME           (BITS(0,24))                                        /*!< Channel3 frame header time bits */

/* constants definitions */
/* LSRX_FTOP_FTOP */
#define LSRX_FTOP_FTOP_POS                   (0U)                                                /*!< LSRX_FTOP_FTOP bit offset */

/* LSRX_FHD_B0CHKEN */
#define LSRX_FHD_B0CHKEN_POS                 (0U)                                                /*!< LSRX_FHD_B0CHKEN bit offset */

/* LSRX_FHD_B0PRTEN */
#define LSRX_FHD_B0PRTEN_POS                 (1U)                                                /*!< LSRX_FHD_B0PRTEN bit offset */

/* LSRX_FHD_B1CHKEN */
#define LSRX_FHD_B1CHKEN_POS                 (2U)                                                /*!< LSRX_FHD_B1CHKEN bit offset */

/* LSRX_FHD_B1PRTEN */
#define LSRX_FHD_B1PRTEN_POS                 (3U)                                                /*!< LSRX_FHD_B1PRTEN bit offset */

/* LSRX_FHD_B2CHKEN */
#define LSRX_FHD_B2CHKEN_POS                 (4U)                                                /*!< LSRX_FHD_B2CHKEN bit offset */

/* LSRX_FHD_B2PRTEN */
#define LSRX_FHD_B2PRTEN_POS                 (5U)                                                /*!< LSRX_FHD_B2PRTEN bit offset */

/* LSRX_FHD_B3CHKEN */
#define LSRX_FHD_B3CHKEN_POS                 (6U)                                                /*!< LSRX_FHD_B3CHKEN bit offset */

/* LSRX_FHD_B3PRTEN */
#define LSRX_FHD_B3PRTEN_POS                 (7U)                                                /*!< LSRX_FHD_B3PRTEN bit offset */

/* LSRX_CTL_CHL0_SEL */
#define LSRX_CTL_CHL0_SEL_POS                (0U)                                                /*!< LSRX_CTL_CHL0_SEL bit offset */
#define LSRX_CTL_CHL0_SEL_CFG(regval)  (LSRX_CTL_CHL0_SEL & ((uint32_t)(regval) << LSRX_CTL_CHL0_SEL_POS))
#define LSRX_CTL_CHL0_SEL_UART0              (LSRX_CTL_CHL0_SEL_CFG(0))                          /*!< Channel select UART0 */
#define LSRX_CTL_CHL0_SEL_UART1              (LSRX_CTL_CHL0_SEL_CFG(1))                          /*!< Channel select UART1 */
#define LSRX_CTL_CHL0_SEL_UART2              (LSRX_CTL_CHL0_SEL_CFG(2))                          /*!< Channel select UART2 */
#define LSRX_CTL_CHL0_SEL_UART3              (LSRX_CTL_CHL0_SEL_CFG(3))                          /*!< Channel select UART3 */
#define LSRX_CTL_CHL0_SEL_UART4              (LSRX_CTL_CHL0_SEL_CFG(4))                          /*!< Channel select UART4 */
#define LSRX_CTL_CHL0_SEL_UART5              (LSRX_CTL_CHL0_SEL_CFG(5))                          /*!< Channel select UART5 */
#define LSRX_CTL_CHL0_SEL_UART6              (LSRX_CTL_CHL0_SEL_CFG(6))                          /*!< Channel select UART6 */
#define LSRX_CTL_CHL0_SEL_UART7              (LSRX_CTL_CHL0_SEL_CFG(7))                          /*!< Channel select UART7 */

/* LSRX_CTL_CHL1_SEL */
#define LSRX_CTL_CHL1_SEL_POS                (4U)                                                /*!< LSRX_CTL_CHL1_SEL bit offset */
#define LSRX_CTL_CHL1_SEL_CFG(regval)  (LSRX_CTL_CHL1_SEL & ((uint32_t)(regval) << LSRX_CTL_CHL1_SEL_POS))
#define LSRX_CTL_CHL1_SEL_UART0              (LSRX_CTL_CHL1_SEL_CFG(0))                          /*!< Channel select UART0 */
#define LSRX_CTL_CHL1_SEL_UART1              (LSRX_CTL_CHL1_SEL_CFG(1))                          /*!< Channel select UART1 */
#define LSRX_CTL_CHL1_SEL_UART2              (LSRX_CTL_CHL1_SEL_CFG(2))                          /*!< Channel select UART2 */
#define LSRX_CTL_CHL1_SEL_UART3              (LSRX_CTL_CHL1_SEL_CFG(3))                          /*!< Channel select UART3 */
#define LSRX_CTL_CHL1_SEL_UART4              (LSRX_CTL_CHL1_SEL_CFG(4))                          /*!< Channel select UART4 */
#define LSRX_CTL_CHL1_SEL_UART5              (LSRX_CTL_CHL1_SEL_CFG(5))                          /*!< Channel select UART5 */
#define LSRX_CTL_CHL1_SEL_UART6              (LSRX_CTL_CHL1_SEL_CFG(6))                          /*!< Channel select UART6 */
#define LSRX_CTL_CHL1_SEL_UART7              (LSRX_CTL_CHL1_SEL_CFG(7))                          /*!< Channel select UART7 */

/* LSRX_CTL_CHL2_SEL */
#define LSRX_CTL_CHL2_SEL_POS                (8U)                                                /*!< LSRX_CTL_CHL2_SEL bit offset */
#define LSRX_CTL_CHL2_SEL_CFG(regval)  (LSRX_CTL_CHL2_SEL & ((uint32_t)(regval) << LSRX_CTL_CHL2_SEL_POS))
#define LSRX_CTL_CHL2_SEL_UART0              (LSRX_CTL_CHL2_SEL_CFG(0))                          /*!< Channel select UART0 */
#define LSRX_CTL_CHL2_SEL_UART1              (LSRX_CTL_CHL2_SEL_CFG(1))                          /*!< Channel select UART1 */
#define LSRX_CTL_CHL2_SEL_UART2              (LSRX_CTL_CHL2_SEL_CFG(2))                          /*!< Channel select UART2 */
#define LSRX_CTL_CHL2_SEL_UART3              (LSRX_CTL_CHL2_SEL_CFG(3))                          /*!< Channel select UART3 */
#define LSRX_CTL_CHL2_SEL_UART4              (LSRX_CTL_CHL2_SEL_CFG(4))                          /*!< Channel select UART4 */
#define LSRX_CTL_CHL2_SEL_UART5              (LSRX_CTL_CHL2_SEL_CFG(5))                          /*!< Channel select UART5 */
#define LSRX_CTL_CHL2_SEL_UART6              (LSRX_CTL_CHL2_SEL_CFG(6))                          /*!< Channel select UART6 */
#define LSRX_CTL_CHL2_SEL_UART7              (LSRX_CTL_CHL2_SEL_CFG(7))                          /*!< Channel select UART7 */

/* LSRX_CTL_CHL3_SEL */
#define LSRX_CTL_CHL3_SEL_POS                (12U)                                               /*!< LSRX_CTL_CHL3_SEL bit offset */
#define LSRX_CTL_CHL3_SEL_CFG(regval)  (LSRX_CTL_CHL3_SEL & ((uint32_t)(regval) << LSRX_CTL_CHL3_SEL_POS))
#define LSRX_CTL_CHL3_SEL_UART0              (LSRX_CTL_CHL3_SEL_CFG(0))                          /*!< Channel select UART0 */
#define LSRX_CTL_CHL3_SEL_UART1              (LSRX_CTL_CHL3_SEL_CFG(1))                          /*!< Channel select UART1 */
#define LSRX_CTL_CHL3_SEL_UART2              (LSRX_CTL_CHL3_SEL_CFG(2))                          /*!< Channel select UART2 */
#define LSRX_CTL_CHL3_SEL_UART3              (LSRX_CTL_CHL3_SEL_CFG(3))                          /*!< Channel select UART3 */
#define LSRX_CTL_CHL3_SEL_UART4              (LSRX_CTL_CHL3_SEL_CFG(4))                          /*!< Channel select UART4 */
#define LSRX_CTL_CHL3_SEL_UART5              (LSRX_CTL_CHL3_SEL_CFG(5))                          /*!< Channel select UART5 */
#define LSRX_CTL_CHL3_SEL_UART6              (LSRX_CTL_CHL3_SEL_CFG(6))                          /*!< Channel select UART6 */
#define LSRX_CTL_CHL3_SEL_UART7              (LSRX_CTL_CHL3_SEL_CFG(7))                          /*!< Channel select UART7 */

/* LSRX_CTL_IDLE_DET */
#define LSRX_CTL_IDLE_DET_POS                (16U)                                               /*!< LSRX_CTL_IDLE_DET bit offset */

/* LSRX_CTL_CHL0EN */
#define LSRX_CTL_CHL0EN_POS                  (17U)                                               /*!< LSRX_CTL_CHL0EN bit offset */

/* LSRX_CTL_CHL1EN */
#define LSRX_CTL_CHL1EN_POS                  (18U)                                               /*!< LSRX_CTL_CHL1EN bit offset */

/* LSRX_CTL_CHL2EN */
#define LSRX_CTL_CHL2EN_POS                  (19U)                                               /*!< LSRX_CTL_CHL2EN bit offset */

/* LSRX_CTL_CHL3EN */
#define LSRX_CTL_CHL3EN_POS                  (20U)                                               /*!< LSRX_CTL_CHL3EN bit offset */

/* LSRX_CTL_TIME_TH */
#define LSRX_CTL_TIME_TH_POS                 (21U)                                               /*!< LSRX_CTL_TIME_TH bit offset */

/* LSRX_CAL_CFG_CAL_CNT */
#define LSRX_CAL_CFG_CAL_CNT_POS             (0U)                                                /*!< LSRX_CAL_CFG_CAL_CNT bit offset */

/* LSRX_CAL_CFG_PAR_DATA */
#define LSRX_CAL_CFG_PAR_DATA_POS            (23U)                                               /*!< LSRX_CAL_CFG_PAR_DATA bit offset */

/* LSRX_EN_EN */
#define LSRX_EN_EN_POS                       (0U)                                                /*!< LSRX_EN_EN bit offset */

/* LSRX_IE_CAL_FINISH_IE */
#define LSRX_IE_CAL_FINISH_IE_POS            (0U)                                                /*!< LSRX_IE_CAL_FINISH_IE bit offset */

/* LSRX_IE_CAL_UPDATE_IE */
#define LSRX_IE_CAL_UPDATE_IE_POS            (1U)                                                /*!< LSRX_IE_CAL_UPDATE_IE bit offset */

/* LSRX_IF_CAL_FINISH_IF */
#define LSRX_IF_CAL_FINISH_IF_POS            (0U)                                                /*!< LSRX_IF_CAL_FINISH_IF bit offset */

/* LSRX_IF_CHL0_UPDATE_IF */
#define LSRX_IF_CHL0_UPDATE_IF_POS           (1U)                                                /*!< LSRX_IF_CHL0_UPDATE_IF bit offset */

/* LSRX_IF_CHL1_UPDATE_IF */
#define LSRX_IF_CHL1_UPDATE_IF_POS           (2U)                                                /*!< LSRX_IF_CHL1_UPDATE_IF bit offset */

/* LSRX_IF_CHL2_UPDATE_IF */
#define LSRX_IF_CHL2_UPDATE_IF_POS           (3U)                                                /*!< LSRX_IF_CHL2_UPDATE_IF bit offset */

/* LSRX_IF_CHL3_UPDATE_IF */
#define LSRX_IF_CHL3_UPDATE_IF_POS           (4U)                                                /*!< LSRX_IF_CHL3_UPDATE_IF bit offset */

/* LSRX_IF_CHL0_VALID_IF */
#define LSRX_IF_CHL0_VALID_IF_POS            (5U)                                                /*!< LSRX_IF_CHL0_VALID_IF bit offset */

/* LSRX_IF_CHL1_VALID_IF */
#define LSRX_IF_CHL1_VALID_IF_POS            (6U)                                                /*!< LSRX_IF_CHL1_VALID_IF bit offset */

/* LSRX_IF_CHL2_VALID_IF */
#define LSRX_IF_CHL2_VALID_IF_POS            (7U)                                                /*!< LSRX_IF_CHL2_VALID_IF bit offset */

/* LSRX_IF_CHL3_VALID_IF */
#define LSRX_IF_CHL3_VALID_IF_POS            (8U)                                                /*!< LSRX_IF_CHL3_VALID_IF bit offset */

/* LSRX_CH0FH_TIME_CH0FH_TIME */
#define LSRX_CH0FH_TIME_CH0FH_TIME_POS       (0U)                                                /*!< LSRX_CH0FH_TIME_CH0FH_TIME bit offset */

/* LSRX_CH1FH_TIME_CH1FH_TIME */
#define LSRX_CH1FH_TIME_CH1FH_TIME_POS       (0U)                                                /*!< LSRX_CH1FH_TIME_CH1FH_TIME bit offset */

/* LSRX_CH2FH_TIME_CH2FH_TIME */
#define LSRX_CH2FH_TIME_CH2FH_TIME_POS       (0U)                                                /*!< LSRX_CH2FH_TIME_CH2FH_TIME bit offset */

/* LSRX_CH3FH_TIME_CH3FH_TIME */
#define LSRX_CH3FH_TIME_CH3FH_TIME_POS       (0U)                                                /*!< LSRX_CH3FH_TIME_CH3FH_TIME bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_LSRX_H */
