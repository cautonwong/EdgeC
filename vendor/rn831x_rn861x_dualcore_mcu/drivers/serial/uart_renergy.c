/*
 * Copyright (c) 2025 Renergy Technology
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renergy_rn8xxx_uart

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include "rn_rv_hal_uart.h"

LOG_MODULE_REGISTER(uart_renergy, CONFIG_UART_LOG_LEVEL);

struct uart_renergy_config {
	UART_TypeDef *instance;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pincfg;
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct uart_renergy_data {
	UART_HandleTypeDef huart;
	struct uart_config uart_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
		uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif
};

/* --- 核心配置 API --- */
static int uart_renergy_configure(const struct device *dev,
				  const struct uart_config *cfg)
{
	struct uart_renergy_data *data = dev->data;
	HAL_StatusTypeDef status;

	/* 映射波特率 */
	data->huart.Init.BaudRate = cfg->baud_rate;

	/* 映射数据位 */
	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_8:
		data->huart.Init.WordLength = UART_WORDLENGTH_8B;
		break;
	case UART_CFG_DATA_BITS_9:
		data->huart.Init.WordLength = UART_WORDLENGTH_9B;
		break;
	default:
		return -ENOTSUP;
	}

	/* 映射停止位 */
	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		data->huart.Init.StopBits = UART_STOPBITS_1;
		break;
	case UART_CFG_STOP_BITS_2:
		data->huart.Init.StopBits = UART_STOPBITS_2;
		break;
	default:
		return -ENOTSUP;
	}

	/* 映射校验位 */
	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		data->huart.Init.Parity = UART_PARITY_NONE;
		break;
	case UART_CFG_PARITY_EVEN:
		data->huart.Init.Parity = UART_PARITY_EVEN;
		break;
	case UART_CFG_PARITY_ODD:
		data->huart.Init.Parity = UART_PARITY_ODD;
		break;
	default:
		return -ENOTSUP;
	}

	status = HAL_UART_Init(&data->huart);
	if (status != HAL_OK) {
		return -EIO;
	}

	data->uart_cfg = *cfg;
	return 0;
}

static int uart_renergy_config_get(const struct device *dev,
				   struct uart_config *cfg)
{
	struct uart_renergy_data *data = dev->data;
	*cfg = data->uart_cfg;
	return 0;
}

/* --- 轮询 API --- */
static int uart_renergy_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct uart_renergy_data *data = dev->data;
	if (HAL_UART_Receive(&data->huart, p_char, 1, 0) == HAL_OK) {
		return 0;
	}
	return -1;
}

static void uart_renergy_poll_out(const struct device *dev, unsigned char out_char)
{
	struct uart_renergy_data *data = dev->data;
	HAL_UART_Transmit(&data->huart, &out_char, 1, HAL_MAX_DELAY);
}

/* --- 中断驱动 API --- */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_renergy_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	struct uart_renergy_data *data = dev->data;
	uint32_t num_tx = 0;

	while ((num_tx < len) && __HAL_UART_GET_FLAG(&data->huart, UART_FLAG_TXE)) {
		/* 直接写入寄存器以保证效率，绕过 HAL 的状态检查 */
		if (data->huart.InstanceType == UART_INSTANCETYPE_0_5) {
			data->huart.Instance.Instance0_5->TXD = tx_data[num_tx++];
		} else {
			data->huart.Instance.Instance6_7->TXD = tx_data[num_tx++];
		}
	}
	return num_tx;
}

static int uart_renergy_fifo_read(const struct device *dev, uint8_t *rx_data, const int len)
{
	struct uart_renergy_data *data = dev->data;
	uint32_t num_rx = 0;

	while ((num_rx < len) && __HAL_UART_GET_FLAG(&data->huart, UART_FLAG_RXNE)) {
		if (data->huart.InstanceType == UART_INSTANCETYPE_0_5) {
			rx_data[num_rx++] = (uint8_t)data->huart.Instance.Instance0_5->RXD;
		} else {
			rx_data[num_rx++] = (uint8_t)data->huart.Instance.Instance6_7->RXD;
		}
	}
	return num_rx;
}

static void uart_renergy_irq_tx_enable(const struct device *dev)
{
	__HAL_UART_ENABLE_IT(&((struct uart_renergy_data *)dev->data)->huart, UART_IT_TXE);
}

static void uart_renergy_irq_tx_disable(const struct device *dev)
{
	__HAL_UART_DISABLE_IT(&((struct uart_renergy_data *)dev->data)->huart, UART_IT_TXE);
}

static void uart_renergy_irq_rx_enable(const struct device *dev)
{
	__HAL_UART_ENABLE_IT(&((struct uart_renergy_data *)dev->data)->huart, UART_IT_RXNE);
}

static void uart_renergy_irq_rx_disable(const struct device *dev)
{
	__HAL_UART_DISABLE_IT(&((struct uart_renergy_data *)dev->data)->huart, UART_IT_RXNE);
}

static int uart_renergy_irq_is_pending(const struct device *dev)
{
	struct uart_renergy_data *data = dev->data;
	return (__HAL_UART_GET_FLAG(&data->huart, UART_FLAG_RXNE) && __HAL_UART_GET_IT_SOURCE(&data->huart, UART_IT_RXNE)) ||
	       (__HAL_UART_GET_FLAG(&data->huart, UART_FLAG_TXE) && __HAL_UART_GET_IT_SOURCE(&data->huart, UART_IT_TXE));
}

static int uart_renergy_irq_update(const struct device *dev) { return 1; }

static void uart_renergy_irq_callback_set(const struct device *dev,
					  uart_irq_callback_user_data_t cb,
					  void *user_data)
{
	struct uart_renergy_data *data = dev->data;
	data->user_cb = cb;
	data->user_data = user_data;
}

static void uart_renergy_isr(const struct device *dev)
{
	struct uart_renergy_data *data = dev->data;
	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_renergy_err_check(const struct device *dev)
{
	struct uart_renergy_data *data = dev->data;
	uint32_t errors = 0;
	uint32_t sta = data->huart.InstanceType == UART_INSTANCETYPE_0_5 ? 
		       data->huart.Instance.Instance0_5->STA : data->huart.Instance.Instance6_7->STA;

	if (sta & UART_FLAG_ORE) errors |= UART_ERROR_OVERRUN;
	if (sta & UART_FLAG_FE)  errors |= UART_ERROR_FRAMING;
	if (sta & UART_FLAG_PE)  errors |= UART_ERROR_PARITY;

	return errors;
}

static int uart_renergy_init(const struct device *dev)
{
	const struct uart_renergy_config *cfg = dev->config;
	struct uart_renergy_data *data = dev->data;
	int ret;

	/* 1. 时钟开启 */
	clock_control_on(cfg->clock_dev, cfg->clock_subsys);

	/* 2. Pinctrl 应用 */
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) return ret;

	/* 3. 初始化 HAL 句柄与硬件 */
	data->huart.Instance.Instance0_5 = (UART0_5_TypeDef *)cfg->instance;
	ret = uart_renergy_configure(dev, &data->uart_cfg);
	if (ret < 0) return ret;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->irq_config_func(dev);
#endif

	LOG_DBG("UART %s init complete", dev->name);
	return 0;
}

static const struct uart_driver_api uart_renergy_driver_api = {
	.poll_in = uart_renergy_poll_in,
	.poll_out = uart_renergy_poll_out,
	.err_check = uart_renergy_err_check,
	.configure = uart_renergy_configure,
	.config_get = uart_renergy_config_get,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_renergy_fifo_fill,
	.fifo_read = uart_renergy_fifo_read,
	.irq_tx_enable = uart_renergy_irq_tx_enable,
	.irq_tx_disable = uart_renergy_irq_tx_disable,
	.irq_rx_enable = uart_renergy_irq_rx_enable,
	.irq_rx_disable = uart_renergy_irq_rx_disable,
	.irq_is_pending = uart_renergy_irq_is_pending,
	.irq_update = uart_renergy_irq_update,
	.irq_callback_set = uart_renergy_irq_callback_set,
#endif
};

#define UART_RENERGY_INIT(inst) 
	PINCTRL_DT_INST_DEFINE(inst); 
	static void uart_renergy_irq_cfg_##inst(const struct device *dev) 
	{
		IRQ_CONNECT(DT_INST_IRQN(inst), 
			    DT_INST_IRQ(inst, priority), 
			    uart_renergy_isr, 
			    DEVICE_DT_INST_GET(inst), 0); 
		irq_enable(DT_INST_IRQN(inst)); 
	} 
	static const struct uart_renergy_config uart_renergy_cfg_##inst = { 
		.instance = (UART_TypeDef *)DT_INST_REG_ADDR(inst), 
		.clock_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, clocks)), 
		.clock_subsys = (clock_control_subsys_t)DT_INST_PHA(inst, clocks, clk_id), 
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst), 
		.baud_rate = DT_INST_PROP(inst, current_speed), 
		.irq_config_func = uart_renergy_irq_cfg_##inst, 
	};
	static struct uart_renergy_data uart_renergy_data_##inst = { 
		.uart_cfg = { 
			.baud_rate = DT_INST_PROP(inst, current_speed), 
			.data_bits = UART_CFG_DATA_BITS_8, 
			.stop_bits = UART_CFG_STOP_BITS_1, 
			.parity = UART_CFG_PARITY_NONE, 
			.flow_ctrl = UART_CFG_FLOW_CTRL_NONE, 
		},
	};
	DEVICE_DT_INST_DEFINE(inst, &uart_renergy_init, NULL, 
			    &uart_renergy_data_##inst, &uart_renergy_cfg_##inst, 
			    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, 
			    &uart_renergy_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RENERGY_INIT)
