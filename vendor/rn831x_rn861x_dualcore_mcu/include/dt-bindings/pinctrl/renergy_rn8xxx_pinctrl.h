/*
 * Renergy RN831X/RN861X Pinmux definitions
 */

#ifndef _RENERGY_RN8XXX_PINCTRL_H_
#define _RENERGY_RN8XXX_PINCTRL_H_

/* 宏定义：引脚编号 + 功能选择 (依据芯片手册) */
#define RNERGY_PINMUX(pin, func) (((func) << 8) | (pin))

/* 示例：UART0 TX 在 PA0, 功能为 1 */
#define UART0_TX_PA0 RNERGY_PINMUX(0, 1)
#define UART0_RX_PA1 RNERGY_PINMUX(1, 1)

#endif
