/*
 * Copyright (c) 2025 Renergy Technology
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renergy_rn8xxx_adc

#include <zephyr/drivers/adc>
#include <soc.h>
#include "rn_rv_hal_adc0.h"

struct adc_renergy_data {
	ADC_HandleTypeDef hadc;
};

static int adc_renergy_channel_setup(const struct device *dev,
				     const struct adc_channel_cfg *channel_cfg)
{
	/* 调用 HAL_ADC_ConfigChannel 实现通道使能 */
	return 0;
}

static int adc_renergy_read(const struct device *dev,
			    const struct adc_sequence *sequence)
{
	/* 阻塞或 DMA 读取实现 */
	return 0;
}

static const struct adc_driver_api adc_renergy_api = {
	.channel_setup = adc_renergy_channel_setup,
	.read = adc_renergy_read,
};

#define ADC_RENERGY_INIT(inst) \
	static struct adc_renergy_data adc_renergy_data_##inst; \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, \
			    &adc_renergy_data_##inst, NULL, \
			    POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, \
			    &adc_renergy_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_RENERGY_INIT)
