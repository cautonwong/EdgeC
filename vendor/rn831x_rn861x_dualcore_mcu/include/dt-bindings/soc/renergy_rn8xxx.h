/*
 * Renergy RN831X/RN861X SoC Register Base and IRQ Definitions
 * This file is shared between C and Devicetree.
 */

#ifndef _RENERGY_RN8XXX_DT_BINDINGS_H_
#define _RENERGY_RN8XXX_DT_BINDINGS_H_

/* --- Memory Regions --- */
#define FLASH_CORE0_BASE        0x30000000
#define FLASH_CORE1_BASE        0x80000000
#define RAM_CORE0_BASE          0x10000000
#define RAM_CORE1_BASE          0x90000000

/* --- System & Control --- */
#define SCU_CORE0_BASE          0x30018000
#define SCU_CORE1_BASE          0xB0018000
#define AOS_CORE0_BASE          0x20088000
#define AOS_CORE1_BASE          0xA0088000
#define MAILBOX_CORE0_BASE      0x3000C000
#define MAILBOX_CORE1_BASE      0xB000C000
#define HSEM_BASE               0xB0084000

/* --- Security --- */
#define SEA_CORE0_BASE          0x30008000
#define SEA_CORE1_BASE          0xB0008000
#define AES_CORE0_BASE          0x30014000
#define AES_CORE1_BASE          0xB0014000
#define KEYSTORE_CORE0_BASE     0x3000A000
#define KEYSTORE_CORE1_BASE     0xB000A000

/* --- Communications (Core 1 View) --- */
#define UART0_CORE1_BASE        0xA0000000
#define UART1_CORE1_BASE        0xA0004000
#define UART2_CORE1_BASE        0xA0008000
#define UART3_CORE1_BASE        0xA000C000
#define UART4_CORE1_BASE        0xA0010000
#define UART5_CORE1_BASE        0xA0014000
#define UART6_CORE1_BASE        0xA0018000
#define UART7_CORE1_BASE        0xA001C000
#define USART0_CORE1_BASE       0xA0024000
#define USART1_CORE1_BASE       0xA0028000
#define LPUART_CORE1_BASE       0xA0020000

#define SPI0_CORE1_BASE         0xA0038000
#define SPI1_CORE1_BASE         0xA003C000
#define SPI2_CORE1_BASE         0xA0040000
#define SPI3_CORE1_BASE         0xA0044000

#define I2C0_CORE1_BASE         0xA0050000
#define I2C1_CORE1_BASE         0xA0054000

/* --- Communications (Core 0 View) --- */
#define UART0_CORE0_BASE        0x20000000
#define UART1_CORE0_BASE        0x20004000
#define UART2_CORE0_BASE        0x20008000
#define UART3_CORE0_BASE        0x2000C000
#define UART4_CORE0_BASE        0x20010000
#define UART5_CORE0_BASE        0x20014000
#define UART6_CORE0_BASE        0x20018000
#define UART7_CORE0_BASE        0x2001C000
#define USART0_CORE0_BASE       0x20024000
#define USART1_CORE0_BASE       0x20028000
#define LPUART_CORE0_BASE       0x20020000

/* --- Analog & Others --- */
#define ADC0_CORE1_BASE         0xA0094000
#define ADC0_CORE0_BASE         0x20094000
#define RTC_CORE1_BASE          0xA00B4000
#define RTC_CORE0_BASE          0x200B4000
#define DSP_CORE1_BASE          0xA006C000
#define DSP_CORE0_BASE          0x2006C000

#define GPIOA_BASE              0xB0080000
#define GPIOB_BASE              0xB0080400
#define GPIOC_BASE              0xB0080800
#define GPIOD_BASE              0xB0080C00
#define GPIOE_BASE              0xB0081000
#define GPIOF_BASE              0xB0081400
#define GPIOG_BASE              0xB0081800
#define GPIOH_BASE              0xB0081C00

/* --- IRQs --- */
#define IRQ_UART0               112
#define IRQ_UART1               113
#define IRQ_UART2               114
#define IRQ_UART3               115
#define IRQ_UART4               116
#define IRQ_UART5               117
#define IRQ_UART6               118
#define IRQ_UART7               119
#define IRQ_SPI0                123
#define IRQ_SPI1                124
#define IRQ_I2C0                108
#define IRQ_I2C1                109
#define IRQ_ADC0                135
#define IRQ_RTC                 137
#define IRQ_MAILBOX             145
#define IRQ_SEA                 132
#define IRQ_AES                 131

#endif
