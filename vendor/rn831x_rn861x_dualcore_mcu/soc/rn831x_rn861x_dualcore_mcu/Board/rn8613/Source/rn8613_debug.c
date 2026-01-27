#include "rn8613_debug.h"

#ifdef  DEBUG_ENABLE

#ifdef CORE0
#define _UART_CONCAT(n)   UART##n##_CORE0
#endif

#ifdef CORE1
#define _UART_CONCAT(n)   UART##n##_CORE1
#endif

#define UART_DEBUG(n)     _UART_CONCAT(n)
#define UART_DIV_1P8M     (1843200 / (16 * UART_BAUDRATE))
#define UART_DIV_29M	  (29491200 / (16 * UART_BAUDRATE))

void debug_init(void)
{
	uint32_t freq;

#ifdef CORE0
    LL_SYSTEM_CORE0_UnlockSystemPSW();
    /* enable SCU GPIO UART0 clock */
    LL_SYSTEM_CORE0_PMOD0_EnableClock(LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GPIO | LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_SCU);
    LL_SYSTEM_CORE0_MOD0_EnableClock(LL_SYSTEM_CORE0_MOD0_PERIPH_UART0);

    LL_SCU_UnlockWriteKey();
    LL_SCU_SetPeripheralSecurityConfig0(LL_SCU_PERIPH_UART0, LL_SCU_SECURE);

    /* uart1, PE04/05 for core1 */
    LL_SCU_SetPeripheralSecurityConfig0(LL_SCU_PERIPH_UART1, LL_SCU_NON_SECURE);
    LL_SCU_SetGPIOSecurityConfig(LL_SCU_GPIO_E, LL_SCU_GPIO_PIN_04, LL_SCU_NON_SECURE);
    LL_SCU_SetGPIOSecurityConfig(LL_SCU_GPIO_E, LL_SCU_GPIO_PIN_05, LL_SCU_NON_SECURE);

    /* UART GPIO need to change!!! */
    /* PE02(TX)/01(RX) */
    LL_GPIO_SetPinMode(GPIOE, LL_GPIO_PIN_1, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetPinMode(GPIOE, LL_GPIO_PIN_2, LL_GPIO_MODE_ALTERNATE);
	/*PE02(TX)/01(RX) AF6*/
	LL_GPIO_SetAFPin_0_7(GPIOE, LL_GPIO_PIN_1, LL_GPIO_AF_6);
	LL_GPIO_SetAFPin_0_7(GPIOE, LL_GPIO_PIN_2, LL_GPIO_AF_6);

	/* set baudrate */
	freq = __LL_SYSTEM_CORE0_CALC_CORE0_CLK_FREQ(LL_SYSTEM_CORE0_GetCore0SystemClockFreq());
	LL_UART0_5_SetBaudRate(UART0_CORE0, freq, UART_BAUDRATE);
    if (freq > 0) {
    	/* set 8bit and enable uart0 */
    	LL_UART0_5_SetDataWidth(UART0_CORE0, LL_UART0_5_DATAWIDTH_8B);
    	LL_UART0_5_Enable(UART0_CORE0);
    }
#endif
#ifdef CORE1
    LL_SYSTEM_CORE1_UnlockSystemPSW();
    /* enable SCU GPIO UART1 clock */
    LL_SYSTEM_CORE1_PMOD0_EnableClock(LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_GPIO | LL_SYSTEM_CORE1_PMOD0_CKE_PERIPH_SCU);
    LL_SYSTEM_CORE1_MOD0_EnableClock(LL_SYSTEM_CORE1_MOD0_PERIPH_UART1);

	LL_GPIO_SetPinMode(GPIOE, LL_GPIO_PIN_4, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetPinMode(GPIOE, LL_GPIO_PIN_5, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetAFPin_0_7(GPIOE, LL_GPIO_PIN_4, LL_GPIO_AF_7);
	LL_GPIO_SetAFPin_0_7(GPIOE, LL_GPIO_PIN_5, LL_GPIO_AF_7);

	/* set baudrate */
	freq = __LL_SYSTEM_CORE1_CALC_CORE1_CLK_FREQ(LL_SYSTEM_CORE1_GetCore1SystemClockFreq());
	LL_UART0_5_SetBaudRate(UART1_CORE1, freq, UART_BAUDRATE);
    if (freq > 0) {
    	/* set 8bit and enable uart1 */
    	LL_UART0_5_SetDataWidth(UART1_CORE1, LL_UART0_5_DATAWIDTH_8B);
    	LL_UART0_5_Enable(UART1_CORE1);
    }
#endif
}

void send_char(char ch)
{
    while (LL_UART0_5_IsActiveFlag_TB(UART_DEBUG(UART_ID)) != RESET);
    LL_UART0_5_TransmitData8(UART_DEBUG(UART_ID), ch);
}

char get_char(void)
{
    while (LL_UART0_5_IsActiveFlag_RX(UART_DEBUG(UART_ID)) == RESET);
    return (char)LL_UART0_5_ReceiveData8(UART_DEBUG(UART_ID));
}

int _write(int fd, char *ptr, int len)
{
	int i;
	for (i = 0; i < len; i++) {
        send_char(ptr[i]);
    }
    return len;
}

int _read(int fd, char* ptr, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        ptr[i] = get_char();
        if (ptr[i] == '\n') {
            return i;
        }
    }
    return i;
}

void send_str(char *s)
{
    while (1) {
        if (*s == '\0') {
            break;
        } else {
            send_char(*s);
            s++;
        }
    }
}

void send_int_by_hex(unsigned int d)
{
    unsigned char i, t;
    send_str("0x");
    for (i = 0; i < 8; i++) {
        t = (d >> (4 * (7 - i))) & 0xf;
        if (t > 0x9) {
            send_char('a' + t - 10);
        } else {
            send_char('0' + t);
        }
    }
}

#endif
