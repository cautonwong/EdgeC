/*
 * Copyright (c) 2025 Renergy Technology
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renergy_rn8xxx_mailbox

#include <zephyr/kernel.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/irq.h>
#include <soc.h>
#include "rn_rv_hal_mailbox.h"

struct ipm_renergy_data {
	ipm_callback_t callback;
	void *user_data;
};

static int ipm_renergy_send(const struct device *dev, int wait, uint32_t id,
			    const void *data, int size)
{
	/* 调用 HAL 层将数据存入 Mailbox 寄存器并触发中断给对方核心 */
	/* 依据 SDK 实现：id 映射至 APICallID 或 MessageID */
	if (size > 16) { /* 典型 Mailbox 硬件缓冲区大小 */
		return -EMSGSIZE;
	}

	HAL_MAILBOX_Send(id, (uint32_t *)data, (size + 3) / 4);
	return 0;
}

static void ipm_renergy_register_callback(const struct device *dev,
					  ipm_callback_t cb, void *user_data)
{
	struct ipm_renergy_data *data = dev->data;
	data->callback = cb;
	data->user_data = user_data;
}

static int ipm_renergy_max_data_size_get(const struct device *dev)
{
	return 16; /* 硬件固定值 */
}

static uint32_t ipm_renergy_max_id_val_get(const struct device *dev)
{
	return 255; /* ID 范围 */
}

static void ipm_renergy_isr(const struct device *dev)
{
	struct ipm_renergy_data *data = dev->data;
	uint32_t msg_id;
	uint32_t buffer[4];

	/* 读取 Mailbox */
	if (HAL_MAILBOX_Receive(&msg_id, buffer, 4) == HAL_OK) {
		if (data->callback) {
			data->callback(dev, data->user_data, msg_id, buffer);
		}
	}
}

static const struct ipm_driver_api ipm_renergy_driver_api = {
	.send = ipm_renergy_send,
	.register_callback = ipm_renergy_register_callback,
	.max_data_size_get = ipm_renergy_max_data_size_get,
	.max_id_val_get = ipm_renergy_max_id_val_get,
};

static int ipm_renergy_init(const struct device *dev)
{
	/* 绑定中断 */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    ipm_renergy_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	HAL_MAILBOX_Init();
	return 0;
}

static struct ipm_renergy_data ipm_data_0;

DEVICE_DT_INST_DEFINE(0, &ipm_renergy_init, NULL,
		      &ipm_data_0, NULL, PRE_KERNEL_1,
		      CONFIG_IPM_INIT_PRIORITY, &ipm_renergy_driver_api);