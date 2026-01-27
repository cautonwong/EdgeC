/********************************************************************************
 * @file    rn8xxx_ll_def.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-09-11
 * @brief   Header file containing functions prototypes of common library.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2016 Renergy.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 *
 *******************************************************************************/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef RN8xxx_DEF_H_
#define RN8xxx_DEF_H_

#include <string.h>
#include <stdint.h>
#include <math.h>

#if defined(RN821x_RN721x_SOC_B) || \
    defined(RN821x_RN721x_SOC_C) || \
    defined(RN821x_RN721x_SOC_D) || \
    defined(RN821x_RN721x_SOC_V2) || \
    defined(RN821x_RN721x_SOC_V3)
#include "rn821x_rn721x_soc.h"
#include "system_rn821x_rn721x_soc.h"

#include "rn821x_rn721x_soc_crc.h"
#include "rn821x_rn721x_soc_d2f.h"
#include "rn821x_rn721x_soc_dsp.h"
#include "rn821x_rn721x_soc_ect.h"
#include "rn821x_rn721x_soc_emu_wave.h"
#include "rn821x_rn721x_soc_emu.h"
#include "rn821x_rn721x_soc_flk.h"
#include "rn821x_rn721x_soc_gpio.h"
#include "rn821x_rn721x_soc_i2c.h"
#include "rn821x_rn721x_soc_intc.h"
#include "rn821x_rn721x_soc_iocnt.h"
#include "rn821x_rn721x_soc_iso7816.h"
#include "rn821x_rn721x_soc_kbi.h"
#include "rn821x_rn721x_soc_lcd.h"
#include "rn821x_rn721x_soc_lpuart.h"
#include "rn821x_rn721x_soc_m2m.h"
#include "rn821x_rn721x_soc_madc.h"
#include "rn821x_rn721x_soc_nvm.h"
#include "rn821x_rn721x_soc_rtc.h"
#include "rn821x_rn721x_soc_simp_tc.h"
#include "rn821x_rn721x_soc_spi.h"
#include "rn821x_rn721x_soc_sysctl.h"
#include "rn821x_rn721x_soc_tc.h"
#include "rn821x_rn721x_soc_uart.h"
#include "rn821x_rn721x_soc_wdt.h"
#endif


#if defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3)
#include "rn831x_rn861x_mcu.h"
#include "system_rn831x_rn861x_mcu.h"

#include "rn831x_rn861x_mcu_d2f.h"
#include "rn831x_rn861x_mcu_dsp.h"
#include "rn831x_rn861x_mcu_gpio.h"
#include "rn831x_rn861x_mcu_i2c.h"
#include "rn831x_rn861x_mcu_intc.h"
#include "rn831x_rn861x_mcu_iso7816.h"
#include "rn831x_rn861x_mcu_kbi.h"
#include "rn831x_rn861x_mcu_lcd.h"
#include "rn831x_rn861x_mcu_m2m.h"
#include "rn831x_rn861x_mcu_madc.h"
#include "rn831x_rn861x_mcu_rtc.h"
#include "rn831x_rn861x_mcu_simp_tc.h"
#include "rn831x_rn861x_mcu_spi.h"
#include "rn831x_rn861x_mcu_sysctl.h"
#include "rn831x_rn861x_mcu_tc.h"
#include "rn831x_rn861x_mcu_uart.h"
#include "rn831x_rn861x_mcu_wdt.h"
#endif

#if defined(RN202x_RN7326_SOC_B) || \
    defined(RN202x_RN7326_SOC_V2)
#include "rn202x_rn7326_soc.h"
#include "rn202x_rn7326_soc_crc.h"
#include "rn202x_rn7326_soc_dsp.h"
#include "rn202x_rn7326_soc_ect.h"
#include "rn202x_rn7326_soc_emu.h"
#include "rn202x_rn7326_soc_flk.h"
#include "rn202x_rn7326_soc_gpadc.h"
#include "rn202x_rn7326_soc_gpio.h"
#include "rn202x_rn7326_soc_gpio_apb.h"
#include "rn202x_rn7326_soc_i2c.h"
#include "rn202x_rn7326_soc_intc.h"
#include "rn202x_rn7326_soc_iocnt.h"
#include "rn202x_rn7326_soc_iso7816.h"
#include "rn202x_rn7326_soc_kbi.h"
#include "rn202x_rn7326_soc_lpuart.h"
#include "rn202x_rn7326_soc_m2m.h"
#include "rn202x_rn7326_soc_madc.h"
#include "rn202x_rn7326_soc_nvm.h"
#include "rn202x_rn7326_soc_rtc.h"
#include "rn202x_rn7326_soc_simp_tc.h"
#include "rn202x_rn7326_soc_spi.h"
#include "rn202x_rn7326_soc_spi0.h"
#include "rn202x_rn7326_soc_spis.h"
#include "rn202x_rn7326_soc_sysctl.h"
#include "rn202x_rn7326_soc_tc.h"
#include "rn202x_rn7326_soc_uart.h"
#include "rn202x_rn7326_soc_wdt.h"
#include "system_rn202x_rn7326_soc.h"
#endif

#if defined(RN831x_RN861x_MCU_V4)
#include "rn831x_rn861x_mcu.h"
#include "system_rn831x_rn861x_mcu.h"

#include "rn831x_rn861x_mcu_adc0.h"
#include "rn831x_rn861x_mcu_aes.h"
#include "rn831x_rn861x_mcu_aos.h"
#include "rn831x_rn861x_mcu_can.h"
#include "rn831x_rn861x_mcu_crc.h"
#include "rn831x_rn861x_mcu_d2f.h"
#include "rn831x_rn861x_mcu_dac.h"
#include "rn831x_rn861x_mcu_dma.h"
#include "rn831x_rn861x_mcu_dsp.h"
#include "rn831x_rn861x_mcu_flash.h"
#include "rn831x_rn861x_mcu_gic.h"
#include "rn831x_rn861x_mcu_gpio.h"
#include "rn831x_rn861x_mcu_hsem.h"
#include "rn831x_rn861x_mcu_i2c.h"
#include "rn831x_rn861x_mcu_iocnt.h"
#include "rn831x_rn861x_mcu_iso7816.h"
#include "rn831x_rn861x_mcu_kbi.h"
#include "rn831x_rn861x_mcu_keystore.h"
#include "rn831x_rn861x_mcu_lcd.h"
#include "rn831x_rn861x_mcu_lcss_vbat.h"
#include "rn831x_rn861x_mcu_lcss_vcc.h"
#include "rn831x_rn861x_mcu_lpuart.h"
#include "rn831x_rn861x_mcu_lsrx.h"
#include "rn831x_rn861x_mcu_lvdcmp.h"
#include "rn831x_rn861x_mcu_m2m.h"
#include "rn831x_rn861x_mcu_madc.h"
#include "rn831x_rn861x_mcu_mailbox.h"
#include "rn831x_rn861x_mcu_qspi.h"
#include "rn831x_rn861x_mcu_rtc.h"
#include "rn831x_rn861x_mcu_scu.h"
#include "rn831x_rn861x_mcu_sea.h"
#include "rn831x_rn861x_mcu_simptc.h"
#include "rn831x_rn861x_mcu_spi.h"
#include "rn831x_rn861x_mcu_system_core0.h"
#include "rn831x_rn861x_mcu_system_core1.h"
#include "rn831x_rn861x_mcu_tc.h"
#include "rn831x_rn861x_mcu_uart0_5.h"
#include "rn831x_rn861x_mcu_uart6_7.h"
#include "rn831x_rn861x_mcu_usart.h"
#include "rn831x_rn861x_mcu_wdt.h"
#endif
#ifndef LL_ZERO
    #define LL_ZERO    0
#endif

#if !defined(RN831x_RN861x_MCU_V4)
typedef enum
{
    ERN_ERROR = 0,
    ERN_SUCCESS = !ERN_ERROR
} ErrorStatus;
typedef enum
{
    ERN_DISABLE = 0,
    ERN_ENABLE = !ERN_DISABLE
} FunctionalState;
#else
#define ERN_ERROR   (0)
#define ERN_SUCCESS (!ERN_ERROR)
#define ERN_DISABLE (0)
#define ERN_ENABLE  (!ERN_DISABLE)
#endif
typedef enum
{
    ERN_FALSE = 0,
    ERN_TRUE = !ERN_FALSE
} BoolStatus;

/**
 * @brief  HAL Status structures definition
 */
typedef enum
{
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;
/**
 * @brief  The assert_param macro is used for function's parameters check.
 * @param  expr If expr is false, it calls assert_failed function
 *         which reports the name of the source file and the source
 *         line number of the call that failed.
 *         If expr is true, it returns no value.
 * @retval None
 */
#ifndef RN831x_RN861x_MCU_V4
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
/* Exported functions ------------------------------------------------------- */
void assert_failed(uint8_t *file, uint32_t line);
#endif

#define BIT0 (0x01U)
#define BIT1 (0x02U)
#define BIT2 (0x04U)
#define BIT3 (0x08U)
#define BIT4 (0x10U)
#define BIT5 (0x20U)
#define BIT6 (0x40U)
#define BIT7 (0x80U)
#define BIT8 (0x100U)
#define BIT9 (0x200U)
#define BIT10 (0x400U)
#define BIT11 (0x800U)
#define BIT12 (0x1000U)
#define BIT13 (0x2000U)
#define BIT14 (0x4000U)
#define BIT15 (0x8000U)
#define BIT16 (0x10000U)
#define BIT17 (0x20000U)
#define BIT18 (0x40000U)
#define BIT19 (0x80000U)
#define BIT20 (0x100000U)
#define BIT21 (0x200000U)
#define BIT22 (0x400000U)
#define BIT23 (0x800000U)
#define BIT24 (0x1000000U)
#define BIT25 (0x2000000U)
#define BIT26 (0x4000000U)
#define BIT27 (0x8000000U)
#define BIT28 (0x10000000U)
#define BIT29 (0x20000000U)
#define BIT30 (0x40000000U)
#define BIT31 (0x80000000U)

/* 寄存器地址为addr的bitnum位的bitband地址获取 */
#define BITBAND_ADDR(addr, bitnum) (*((__IO uint32_t *)(((addr)&0xFC000000U) + 0x02000000U + ((uint32_t)(bitnum) << 2U) + (((uint32_t)((addr)&0x07FFFFFFU) << 5U) & 0x03FFFFFFU))))

#define GET_DATA_MASK(databits) (~(0xfffffffe << (databits)))   /* databits范围：0~31，如获取16bit数据则databits=15 */


#define RW_MEM8(addr)                   (*(volatile uint8_t *)(addr))
#define RW_MEM16(addr)                  (*(volatile uint16_t *)(addr))
#define RW_MEM32(addr)                  (*(volatile uint32_t *)(addr))

#define SET_REG_BIT(reg, bit)           ((reg) |= (bit))
#define SET_REG8_BIT(reg, bit)          ((reg) |= ((uint8_t)(bit)))
#define SET_REG16_BIT(reg, bit)         ((reg) |= ((uint16_t)(bit)))
#define SET_REG32_BIT(reg, bit)         ((reg) |= ((uint32_t)(bit)))

#define CLR_REG_BIT(reg, bit)           ((reg) &= (~(bit)))
#define CLR_REG8_BIT(reg, bit)          ((reg) &= ((uint8_t)(~((uint8_t)(bit)))))
#define CLR_REG16_BIT(reg, bit)         ((reg) &= ((uint16_t)(~((uint16_t)(bit)))))
#define CLR_REG32_BIT(reg, bit)         ((reg) &= ((uint32_t)(~((uint32_t)(bit)))))

#define READ_REG_BIT(reg, bit)          ((reg) & (bit))
#define READ_REG8_BIT(reg, bit)         ((reg) & ((uint8_t)(bit)))
#define READ_REG16_BIT(reg, bit)        ((reg) & ((uint16_t)(bit)))
#define READ_REG32_BIT(reg, bit)        ((reg) & ((uint32_t)(bit)))

#define CLR_REG(reg)                    ((reg) = (0U))
#define CLR_REG8(reg)                   ((reg) = ((uint8_t)(0U)))
#define CLR_REG16(reg)                  ((reg) = ((uint16_t)(0U)))
#define CLR_REG32(reg)                  ((reg) = ((uint32_t)(0UL)))

#ifndef RN831x_RN861x_MCU_V4
#define WRITE_REG(reg, VAL)             ((reg) = (VAL))
#define WRITE_REG8(reg, VAL)            ((reg) = ((uint8_t)(VAL)))
#define WRITE_REG16(reg, VAL)           ((reg) = ((uint16_t)(VAL)))
#define WRITE_REG32(reg, VAL)           ((reg) = ((uint32_t)(VAL)))

#define READ_REG(reg)                   (reg)
#define READ_REG8(reg)                  (reg)
#define READ_REG16(reg)                 (reg)
#define READ_REG32(reg)                 (reg)

#define MODIFY_REG(REGS, CLRMASK, SETMASK)    (WRITE_REG((REGS), (((READ_REG(REGS)) & (~(CLRMASK))) | ((SETMASK) & (CLRMASK)))))
#define MODIFY_REG8(REGS, CLRMASK, SETMASK)   (WRITE_REG8((REGS), (((READ_REG8((REGS))) & ((uint8_t)(~((uint8_t)(CLRMASK))))) | ((uint8_t)(SETMASK) & (uint8_t)(CLRMASK)))))
#define MODIFY_REG16(REGS, CLRMASK, SETMASK)  (WRITE_REG16((REGS), (((READ_REG16((REGS))) & ((uint16_t)(~((uint16_t)(CLRMASK))))) | ((uint16_t)(SETMASK) & (uint16_t)(CLRMASK)))))
#define MODIFY_REG32(REGS, CLRMASK, SETMASK)  (WRITE_REG32((REGS), (((READ_REG32((REGS))) & ((uint32_t)(~((uint32_t)(CLRMASK))))) | ((uint32_t)(SETMASK) & (uint32_t)(CLRMASK)))))
#else
#ifdef  USE_FULL_ASSERT
/**
  * @brief  The assert_param macro is used for function's parameters check.
  * @param  expr If expr is false, it calls assert_failed function
  *         which reports the name of the source file and the source
  *         line number of the call that failed.
  *         If expr is true, it returns no value.
  * @retval None
  */
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
/* Exported functions ------------------------------------------------------- */
void assert_failed(uint8_t* file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif /* USE_FULL_ASSERT */
#endif

#endif /* RN8xxx_Def_H_ */
