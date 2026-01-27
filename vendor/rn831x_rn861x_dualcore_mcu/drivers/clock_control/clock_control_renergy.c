/*
 * Copyright (c) 2025 Renergy Technology
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renergy_rn8xxx_clock

#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/sys_io.h>
#include <soc.h>
#include "rn_rv_hal_system.h"

struct rn8xxx_clock_data {
	struct k_spinlock lock;
};

static int rn8xxx_clock_on(const struct device *dev,
			   clock_control_subsys_t sys)
{
	struct rn8xxx_clock_data *data = dev->data;
	uint32_t id = POINTER_TO_UINT(sys);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	__HAL_SYSTEM_PSW_UNLOCK();
	SYSCTRL->MOD_EN |= BIT(id);
	__HAL_SYSTEM_PSW_LOCK();

	k_spin_unlock(&data->lock, key);
	return 0;
}

static int rn8xxx_clock_off(const struct device *dev,
			    clock_control_subsys_t sys)
{
	struct rn8xxx_clock_data *data = dev->data;
	uint32_t id = POINTER_TO_UINT(sys);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	__HAL_SYSTEM_PSW_UNLOCK();
	SYSCTRL->MOD_EN &= ~BIT(id);
	__HAL_SYSTEM_PSW_LOCK();

	k_spin_unlock(&data->lock, key);
	return 0;
}

static int rn8xxx_clock_get_rate(const struct device *dev,
				 clock_control_subsys_t sys,
				 uint32_t *rate)
{
	*rate = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
	return 0;
}

static const struct clock_control_driver_api rn8xxx_clock_api = {
	.on = rn8xxx_clock_on,
	.off = rn8xxx_clock_off,
	.get_rate = rn8xxx_clock_get_rate,
};

static struct rn8xxx_clock_data scu_data;

DEVICE_DT_INST_DEFINE(0, NULL, NULL, &scu_data, NULL,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &rn8xxx_clock_api);
