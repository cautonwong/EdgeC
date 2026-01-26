/*
 * Copyright (c) 2025 Renergy Technology
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renergy_rn8xxx_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include "rn_rv_hal_gpio.h"

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	for (uint8_t i = 0; i < pin_cnt; i++) {
		uint32_t pin_id = pins[i] & 0xFF;
		uint32_t func = (pins[i] >> 8) & 0xFF;

		/* 调用 HAL 层的 PinMux 配置，处理不同核心的视图 */
		HAL_GPIO_PinMuxConfig(pin_id, func);
	}
	return 0;
}

static int pinctrl_renergy_init(const struct device *dev) { return 0; }

DEVICE_DT_INST_DEFINE(0, &pinctrl_renergy_init, NULL, NULL, NULL,
		      PRE_KERNEL_1, CONFIG_PINCTRL_INIT_PRIORITY, NULL);
