/*
 * Copyright (c) 2025 Renergy Technology
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renergy_rn8xxx_gpio

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <soc.h>
#include "rn_rv_hal_gpio.h"

struct gpio_renergy_config {
	struct gpio_driver_config common;
	GPIO_TypeDef *instance;
	void (*irq_config_func)(const struct device *dev);
};

struct gpio_renergy_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

static int gpio_renergy_configure(const struct device *dev,
				  gpio_pin_t pin,
				  gpio_flags_t flags)
{
	const struct gpio_renergy_config *cfg = dev->config;
	GPIO_InitTypeDef init = {0};

	init.Pin = BIT(pin);
	init.Mode = (flags & GPIO_OUTPUT) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
	init.Pull = (flags & GPIO_PULL_UP) ? GPIO_PULLUP :
		    (flags & GPIO_PULL_DOWN) ? GPIO_PULLDOWN : GPIO_NOPULL;

	HAL_GPIO_Init(cfg->instance, &init);

	if (flags & GPIO_OUTPUT) {
		HAL_GPIO_WritePin(cfg->instance, (uint16_t)BIT(pin),
				 (flags & GPIO_OUTPUT_INIT_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	}

	return 0;
}

static int gpio_renergy_port_get_raw(const struct device *dev,
				     gpio_port_value_t *value)
{
	const struct gpio_renergy_config *cfg = dev->config;
	*value = HAL_GPIO_ReadPort(cfg->instance);
	return 0;
}

static int gpio_renergy_port_set_masked_raw(const struct device *dev,
					    gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	const struct gpio_renergy_config *cfg = dev->config;
	uint32_t current = HAL_GPIO_ReadPort(cfg->instance);
	HAL_GPIO_WritePort(cfg->instance, (current & ~mask) | (value & mask));
	return 0;
}

static int gpio_renergy_port_set_bits_raw(const struct device *dev,
					  gpio_port_pins_t mask)
{
	const struct gpio_renergy_config *cfg = dev->config;
	HAL_GPIO_WritePin(cfg->instance, (uint16_t)mask, GPIO_PIN_SET);
	return 0;
}

static int gpio_renergy_port_clear_bits_raw(const struct device *dev,
					    gpio_port_pins_t mask)
{
	const struct gpio_renergy_config *cfg = dev->config;
	HAL_GPIO_WritePin(cfg->instance, (uint16_t)mask, GPIO_PIN_RESET);
	return 0;
}

static int gpio_renergy_pin_interrupt_configure(const struct device *dev,
							gpio_pin_t pin,
							enum gpio_int_mode mode,
							enum gpio_int_trig trig)
{
	const struct gpio_renergy_config *cfg = dev->config;

	if (mode == GPIO_INT_MODE_DISABLED) {
		cfg->instance->IE &= ~BIT(pin);
		return 0;
	}

	/* 补全：根据 mode/trig 设置电平或边沿触发 */
	/* Renergy 硬件通常通过 KBI 进行边沿配置 */
	
	cfg->instance->IE |= BIT(pin);
	return 0;
}

static int gpio_renergy_manage_callback(const struct device *dev,
					struct gpio_callback *cb,
					bool set)
{
	struct gpio_renergy_data *data = dev->data;
	return gpio_manage_callback(&data->callbacks, cb, set);
}

static void gpio_renergy_isr(const struct device *dev)
{
	const struct gpio_renergy_config *cfg = dev->config;
	struct gpio_renergy_data *data = dev->data;
	uint32_t status = cfg->instance->IF;

	/* 清除标志位 */
	cfg->instance->IF = status;

	/* 触发链表回调 */
	gpio_fire_callbacks(&data->callbacks, dev, status);
}

static const struct gpio_driver_api gpio_renergy_api = {
	.pin_configure = gpio_renergy_configure,
	.port_get_raw = gpio_renergy_port_get_raw,
	.port_set_masked_raw = gpio_renergy_port_set_masked_raw,
	.port_set_bits_raw = gpio_renergy_port_set_bits_raw,
	.port_clear_bits_raw = gpio_renergy_port_clear_bits_raw,
	.pin_interrupt_configure = gpio_renergy_pin_interrupt_configure,
	.manage_callback = gpio_renergy_manage_callback,
};

#define GPIO_RENERGY_INIT(inst) \
	static void gpio_renergy_irq_config_##inst(const struct device *dev) \
	{\
		IRQ_CONNECT(DT_INST_IRQN(inst), \
			    DT_INST_IRQ(inst, priority), \
			    gpio_renergy_isr, \
			    DEVICE_DT_INST_GET(inst), 0); \
		irq_enable(DT_INST_IRQN(inst)); \
	} \
	static const struct gpio_renergy_config gpio_renergy_cfg_##inst = { \
		.common = { .port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst) }, \
		.instance = (GPIO_TypeDef *)DT_INST_REG_ADDR(inst), \
		.irq_config_func = gpio_renergy_irq_config_##inst, \
	};	static struct gpio_renergy_data gpio_renergy_data_##inst;	static int gpio_renergy_init_##inst(const struct device *dev) \
	{\
		const struct gpio_renergy_config *cfg = dev->config; \
		cfg->irq_config_func(dev); \
		return 0; \
	} \
	DEVICE_DT_INST_DEFINE(inst, &gpio_renergy_init_##inst, NULL, \
			    &gpio_renergy_data_##inst, &gpio_renergy_cfg_##inst, \
			    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, \
			    &gpio_renergy_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_RENERGY_INIT)
