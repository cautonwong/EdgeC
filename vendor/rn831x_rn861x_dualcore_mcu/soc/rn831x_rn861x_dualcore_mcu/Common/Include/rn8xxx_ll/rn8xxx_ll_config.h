/********************************************************************************
 * @file    Rn8xxx_Common.h
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
 ******************************************************************************
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef RN8xxx_Config_H_
#define RN8xxx_Config_H_

#include "rn8xxx_v2_lib.h"

#define LL_D2F_MODULE_ENABLED

#if defined(RN821x_RN721x_SOC_V2) || \
    defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V4) || \
    defined(RN202x_RN7326_SOC_V2)
#define LL_DSP_MODULE_ENABLED /*MCU_V2 SOC_V2才支持DSP模块*/
#endif

#if defined(RN821x_RN721x_SOC_B)  || \
    defined(RN821x_RN721x_SOC_C)  || \
    defined(RN821x_RN721x_SOC_D)  || \
    defined(RN821x_RN721x_SOC_V2) || \
    defined(RN821x_RN721x_SOC_V3) || \
    defined(RN202x_RN7326_SOC_V2)
#define LL_EMU_MODULE_ENABLED /*SOC才支持EMU模块*/
#endif

#if !defined(RN831x_RN861x_MCU_V4)
#define LL_INTC_MODULE_ENABLED
#endif
#define LL_GPIO_MODULE_ENABLED
#define LL_IIC_MODULE_ENABLED
#define LL_IOCNT_MODULE_ENABLED
#define LL_KBI_MODULE_ENABLED

#if defined(RN821x_RN721x_SOC_B)  || \
    defined(RN821x_RN721x_SOC_C)  || \
    defined(RN821x_RN721x_SOC_D)  || \
    defined(RN821x_RN721x_SOC_V2) || \
    defined(RN821x_RN721x_SOC_V3) || \
    defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3) || \
    defined(RN831x_RN861x_MCU_V4)
#define LL_LCD_MODULE_ENABLED
#endif


#if defined(RN821x_RN721x_SOC_V2) || \
    defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V4)
#define LL_M2M_MODULE_ENABLED /*MCU_V2 SOC_V2才支持M2M模块*/
#endif

#define LL_MADC_MODULE_ENABLED /*LvdCmpSar*/

#if defined(RN821x_RN721x_SOC_B)  || \
    defined(RN821x_RN721x_SOC_C)  || \
    defined(RN821x_RN721x_SOC_D)  || \
    defined(RN821x_RN721x_SOC_V2) || \
    defined(RN821x_RN721x_SOC_V3) || \
    defined(RN202x_RN7326_SOC_V2)
#define LL_NVM_MODULE_ENABLED /*SOC才支持全失压模块*/
#endif

#define LL_RTC_MODULE_ENABLED

#if defined(RN821x_RN721x_SOC_V2) || \
    defined(RN821x_RN721x_SOC_V3) || \
    defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3) || \
    defined(RN831x_RN861x_MCU_V4) || \
    defined(RN202x_RN7326_SOC_V2)
#define LL_SIMPTC_MODULE_ENABLED
#endif

#define LL_SPI_MODULE_ENABLED
#define LL_SYSC_MODULE_ENABLED

#if !defined(RN831x_RN861x_MCU_V4)
#define LL_SYSTICK_MODULE_ENABLED
#endif
#define LL_TC_MODULE_ENABLED
#define LL_UART_MODULE_ENABLED
#define LL_WDT_MODULE_ENABLED
#define LL_INTC_MODULE_ENABLED

#if !defined(RN821x_RN721x_SOC_V3)
#define LL_ISO7816_MODULE_ENABLED /*MCU_V3 不支持ISO7816模块*/
#endif

#if defined(RN821x_RN721x_SOC_V2) || \
    defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V4) || \
    defined(RN202x_RN7326_SOC_V2)
#define LL_SEA_MODULE_ENABLED
#endif

/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll_common.h"



#ifdef LL_D2F_MODULE_ENABLED
#include "rn8xxx_ll_d2f.h"
#endif

#ifdef LL_DSP_MODULE_ENABLED
#include "rn8xxx_ll_dsp.h"
#endif

#ifdef LL_EMU_MODULE_ENABLED
#include "rn8xxx_ll_emu.h"
#endif

#ifdef LL_GPIO_MODULE_ENABLED
#include "rn8xxx_ll_gpio.h"
#endif

#ifdef LL_IIC_MODULE_ENABLED
#include "rn8xxx_ll_iic.h"
#endif

#ifdef LL_INTC_MODULE_ENABLED
#include "rn8xxx_ll_intc.h"
#endif

#ifdef LL_IOCNT_MODULE_ENABLED
#include "rn8xxx_ll_iocnt.h"
#endif

#ifdef LL_ISO7816_MODULE_ENABLED
#include "rn8xxx_ll_iso7816.h"
#endif

#ifdef LL_KBI_MODULE_ENABLED
#include "rn8xxx_ll_kbi.h"
#endif

#ifdef LL_LCD_MODULE_ENABLED
#include "rn8xxx_ll_lcd.h"
#endif

#ifdef LL_M2M_MODULE_ENABLED
#include "rn8xxx_ll_m2m.h"
#endif


#ifdef LL_MADC_MODULE_ENABLED
#include "rn8xxx_ll_madc.h"
#endif

#ifdef LL_NVM_MODULE_ENABLED
#include "rn8xxx_ll_novoltagem.h"
#endif

#ifdef LL_RTC_MODULE_ENABLED
#include "rn8xxx_ll_rtc.h"
#endif

#ifdef LL_SIMPTC_MODULE_ENABLED
#include "rn8xxx_ll_simptc.h"
#endif

#ifdef LL_SPI_MODULE_ENABLED
#include "rn8xxx_ll_spi.h"
#endif

#ifdef LL_SYSC_MODULE_ENABLED
#include "rn8xxx_ll_sysc.h"
#endif

#ifdef LL_SYSTICK_MODULE_ENABLED
#include "rn8xxx_ll_systickcortexm0.h"
#endif

#ifdef LL_TC_MODULE_ENABLED
#include "rn8xxx_ll_tc.h"
#endif

#ifdef LL_UART_MODULE_ENABLED
#include "rn8xxx_ll_uart.h"
#endif

#ifdef LL_WDT_MODULE_ENABLED
#include "rn8xxx_ll_wdt.h"
#endif


#endif /*_RN8xxx_Config_H*/
