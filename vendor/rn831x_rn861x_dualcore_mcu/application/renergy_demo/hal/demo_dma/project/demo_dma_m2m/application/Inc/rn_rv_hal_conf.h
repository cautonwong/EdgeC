/**
 ******************************************************************************
 * @file   rn_rv_hal.conf.h
 * @author Renergy Software Team
 * @brief  HAL configuration template file.
 *         This file should be copied to the application folder and renamed
 *         to rn_rv_hal_conf.h.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RN_HAL_CONF_H
#define __RN_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Exported types -----------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/

/* ########################## Module Selection ############################# */
/**
 * @brief This is the list of modules to be used in the HAL driver
 */
#define HAL_MODULE_ENABLED
// #define HAL_KEYSTORE_MODULE_ENABLED
// #define HAL_CAN_MODULE_ENABLED
#define HAL_CRC_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_M2M_MODULE_ENABLED
#define HAL_SEA_MODULE_ENABLED
// #define HAL_AES_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_SYSTEM_MODULE_ENABLED
#define HAL_RPC_MODULE_ENABLED
// #define HAL_HSEM_MODULE_ENABLED
#define HAL_GIC_MODULE_ENABLED
// #define HAL_LCD_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_SCU_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
// #define HAL_DSP_MODULE_ENABLED

/* ########################## Register Callbacks selection ############################## */
/**
  * @brief This is the list of modules where register callback can be used
  */
#define USE_HAL_I2C_REGISTER_CALLBACKS          0U
#define USE_HAL_ISO7816_REGISTER_CALLBACKS      0U
#define USE_HAL_SPI_REGISTER_CALLBACKS          0U
#define USE_HAL_UART_REGISTER_CALLBACKS         0U
#define USE_HAL_USART_REGISTER_CALLBACKS        0U

/* ################## SPI peripheral configuration ######################### */

/* CRC FEATURE: Use to activate CRC feature inside HAL SPI Driver
 * Activated: CRC code is present inside driver
 * Deactivated: CRC code cleaned from driver
 */

#define USE_SPI_CRC                     1U

/* Includes -----------------------------------------------------------------*/
/**
 * @brief Include module's header file
 */
#ifdef HAL_KEYSTORE_MODULE_ENABLED
#include "rn_rv_hal_keystore.h"
#endif /* HAL_KEYSTORE_MODULE_ENABLED */

#ifdef HAL_CAN_MODULE_ENABLED
#include "rn_rv_hal_can.h"
#endif /* HAL_CAN_MODULE_ENABLED */

#ifdef HAL_CRC_MODULE_ENABLED
#include "rn_rv_hal_crc.h"
#endif /* HAL_CRC_MODULE_ENABLED */

#ifdef HAL_DMA_MODULE_ENABLED
#include "rn_rv_hal_dma.h"
#endif /* HAL_DMA_MODULE_ENABLED */

#ifdef HAL_M2M_MODULE_ENABLED
#include "rn_rv_hal_m2m.h"
#endif /* HAL_M2M_MODULE_ENABLED */

#ifdef HAL_SEA_MODULE_ENABLED
#include "rn_rv_hal_sea.h"
#endif /* HAL_SEA_MODULE_ENABLED */

#ifdef HAL_AES_MODULE_ENABLED
#include "rn_rv_hal_aes.h"
#endif /* HAL_AES_MODULE_ENABLED */

#ifdef HAL_FLASH_MODULE_ENABLED
#include "rn_rv_hal_flash.h"
#endif /* HAL_FLASH_MODULE_ENABLED */

#ifdef HAL_SYSTEM_MODULE_ENABLED
#include "rn_rv_hal_system.h"
#endif /* HAL_SYSTEM_MODULE_ENABLED */

#ifdef HAL_RPC_MODULE_ENABLED
#include "rn_rv_hal_rpc.h"
#endif /* HAL_RPC_MODULE_ENABLED */

#ifdef HAL_HSEM_MODULE_ENABLED
#include "rn_rv_hal_hsem.h"
#endif /* HAL_HSEM_MODULE_ENABLED */

#ifdef HAL_GIC_MODULE_ENABLED
#include "rn_rv_hal_gic.h"
#endif /* HAL_GIC_MODULE_ENABLED */

#ifdef HAL_LCD_MODULE_ENABLED
#include "rn_rv_hal_lcd.h"
#endif /* HAL_LCD_MODULE_ENABLED */

#ifdef HAL_GPIO_MODULE_ENABLED
#include "rn_rv_hal_gpio.h"
#endif /* HAL_GPIO_MODULE_ENABLED */

#ifdef HAL_SCU_MODULE_ENABLED
#include "rn_rv_hal_scu.h"
#endif /* HAL_SCU_MODULE_ENABLED */

#ifdef HAL_UART_MODULE_ENABLED
#include "rn_rv_hal_uart.h"
#endif /* HAL_UART_MODULE_ENABLED */

#ifdef HAL_DSP_MODULE_ENABLED
#include "rn_rv_hal_dsp.h"
#endif /* HAL_DSP_MODULE_ENABLED */

#define ROM_SIZE                        (48 * 1024)
#define SRAM_TOTAL_SIZE                 (160 * 1024)
#define RN_LIB_OFFSET                   (9U * 4U)
#define NVR_ERASE_PASSWD                (0x12345678U)
#define CHIP_ERASE_PASSWD               (0x12345678U)
#define ROM_BASE_ADDR                   (0x200000U)

/* Exported macro -----------------------------------------------------------*/
#include "rn_rv_assert.h" // use assert_param() function

#ifdef __cplusplus
}
#endif

#endif /* __RN_HAL_CONF_H */
