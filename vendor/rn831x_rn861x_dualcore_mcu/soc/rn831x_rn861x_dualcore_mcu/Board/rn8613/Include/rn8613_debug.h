// See LICENSE for license details.

#ifndef _RN8613_DEBUG_H
#define _RN8613_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include "rn_rv_ll_system_core0.h"
#include "rn_rv_ll_system_core1.h"
#include "rn_rv_ll_gpio.h"
#include "rn_rv_ll_uart0_5.h"
#include "rn_rv_ll_uart6_7.h"
#include "rn_rv_ll_scu.h"
#include "rn_rv_ll_gpio.h"


#define DEBUG_ENABLE

#ifdef DEBUG_ENABLE

#ifdef CORE0
#define UART_ID         0
#define UART_BAUDRATE   (115200)
#endif

#ifdef CORE1
#define UART_ID         1
#define UART_BAUDRATE   (115200)
#endif

void debug_init(void);
void send_char(char ch);
void send_str(char *s);
void send_int_by_hex(unsigned int d);
// void send_int(unsigned int d);

#define UART_INIT(...) debug_init(__VA_ARGS__)
#define SEND_CHAR(...) send_char(__VA_ARGS__)
#define SEND_STR(...) send_str(__VA_ARGS__)
#define SEND_INT_BY_HEX(...) send_int_by_hex(__VA_ARGS__)
// #define SEND_INT(...) send_int(__VA_ARGS__)

#else

#define UART_INIT(...)
#define SEND_CHAR(...)
#define SEND_STR(...)
#define SEND_INT_BY_HEX(...)
// #define SEND_INT(...)

#endif

#ifdef __cplusplus
}
#endif

#endif /* _EVALSOC_UART_H */
