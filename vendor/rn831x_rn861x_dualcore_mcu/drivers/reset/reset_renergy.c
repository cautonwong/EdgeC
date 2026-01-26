/*
 * Copyright (c) 2025 Renergy Technology
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renergy_rn8xxx_reset

#include <zephyr/drivers/reset.h>
#include <soc.h>

#define REG_SYS_MOD_RST   0x08

struct rn8xxx_reset_config {
	uintptr_t base;
};

static int rn8xxx_reset_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct rn8xxx_reset_config *config = dev->config;
	uint32_t val = *(volatile uint32_t *)(config->base + REG_SYS_MOD_RST);
	*status = (val & BIT(id)) ? 1 : 0;
	return 0;
}

static int rn8xxx_reset_line_assert(const struct device *dev, uint32_t id)
{
	const struct rn8xxx_reset_config *config = dev->config;
	*(volatile uint32_t *)(config->base + REG_SYS_MOD_RST) |= BIT(id);
	return 0;
}

static int rn8xxx_reset_line_deassert(const struct device *dev, uint32_t id)
{
	const struct rn8xxx_reset_config *config = dev->config;
	*(volatile uint32_t *)(config->base + REG_SYS_MOD_RST) &= ~BIT(id);
	return 0;
}

static const struct reset_driver_api rn8xxx_reset_api = {
	.status = rn8xxx_reset_status,
	.line_assert = rn8xxx_reset_line_assert,
	.line_deassert = rn8xxx_reset_line_deassert,
};

static int rn8xxx_reset_init(const struct device *dev) { return 0; }

#define RN8XXX_RESET_INIT(inst) \
	static const struct rn8xxx_reset_config rn8xxx_reset_config_##inst = { \
		.base = DT_INST_REG_ADDR(inst), \
	};	\
	DEVICE_DT_INST_DEFINE(inst, &rn8xxx_reset_init, NULL, NULL, \
			    &rn8xxx_reset_config_##inst, PRE_KERNEL_1, \
			    CONFIG_RESET_INIT_PRIORITY, &rn8xxx_reset_api);

DT_INST_FOREACH_STATUS_OKAY(RN8XXX_RESET_INIT)
