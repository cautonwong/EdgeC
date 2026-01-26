/*
 * Copyright (c) 2025 Renergy Technology
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <soc.h>
#include "rn_rv_hal.h"

/* 适配 Renergy HAL 要求的 MSP (MCU Specific Package) 初始化 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	/* 根据 huart->Instance 开启对应的外设时钟 */
	/* 此处逻辑应从 DTS 获取，暂由 HAL 内部处理或手动使能 */
	__HAL_SYSTEM_PSW_UNLOCK();
	if (huart->Instance.Instance0_5 == UART0) {
		SYSCTRL->MOD_EN |= BIT(0); /* UART0 BIT */
	}
	__HAL_SYSTEM_PSW_LOCK();
}

static int renergy_rn8xxx_soc_init(void)
{
	/* 1. 设置 ECLIC 模式与向量表 */
	extern void __irq_wrapper(void);
	__asm__ volatile ("csrw 0x307, %0" :: "r"(&__irq_wrapper));

	/* 2. 调用 HAL 初始化锁状态 */
	HAL_Init();

	return 0;
}

SYS_INIT(renergy_rn8xxx_soc_init, PRE_KERNEL_1, 0);