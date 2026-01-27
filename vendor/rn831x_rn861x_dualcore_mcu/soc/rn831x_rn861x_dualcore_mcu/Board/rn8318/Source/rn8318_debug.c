#include "rn8318_debug.h"

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
    GPIO_InitTypeDef GPIO_Init;

#ifdef CORE0
    __HAL_SYSTEM_PSW_UNLOCK();
    /* enable SCU GPIO UART0 clock */
    __HAL_SYSTEM_GPIO_CLK_ENABLE();
    __HAL_SYSTEM_SCU_CLK_ENABLE();
    __HAL_SYSTEM_UART0_CLK_ENABLE();
    __HAL_SYSTEM_WDT_CLK_ENABLE();
    __HAL_SYSTEM_PSW_LOCK();

    __HAL_SCU_WKEY_UNLOCK();
    __HAL_SCU_UART0_SET_SECURE();

    /* uart1, PE04/05 for core1 */
    __HAL_SCU_UART1_SET_NONSECURE();
    __HAL_SCU_GPIOE_SET_NONSECURE(HAL_SCU_GPIO_PIN_04 | HAL_SCU_GPIO_PIN_05);

    /* UART GPIO need to change!!! */
    /* PE02(TX)/01(RX)     */
    /* PE02(TX)/01(RX) AF6 */
    GPIO_Init.Pin = GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_Init.Mode = GPIO_MODE_AF;
    GPIO_Init.Alternate = GPIO_AF6_UART0_2_4_6;
    HAL_GPIO_Init(GPIOE, &GPIO_Init);

    /* set baudrate */
    freq = HAL_SYSTEM_GetSysClockFreq(HAL_SYSTEM_CLK_CORE0);
    LL_UART0_5_SetBaudRate(UART0_CORE0, freq, UART_BAUDRATE);
    if (freq > 0) {
        /* set 8bit and enable uart0 */
        LL_UART0_5_SetDataWidth(UART0_CORE0, LL_UART0_5_DATAWIDTH_8B);
        LL_UART0_5_Enable(UART0_CORE0);
    }
#endif
#ifdef CORE1
    __HAL_SYSTEM_PSW_UNLOCK();
    /* enable SCU GPIO UART0 clock */
    __HAL_SYSTEM_GPIO_CLK_ENABLE();
    __HAL_SYSTEM_SCU_CLK_ENABLE();
    __HAL_SYSTEM_UART1_CLK_ENABLE();
    __HAL_SYSTEM_PSW_LOCK();

    /* uart1 PE04(RX)/PE05(TX)*/
    GPIO_Init.Pin = GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_Init.Mode = GPIO_MODE_AF;
    GPIO_Init.Alternate = GPIO_AF7_UART1_3_5_7;
    HAL_GPIO_Init(GPIOE, &GPIO_Init);

    /* set baudrate */
    freq = HAL_SYSTEM_GetSysClockFreq(HAL_SYSTEM_CLK_CORE1);
    LL_UART0_5_SetBaudRate(UART1_CORE1, freq, UART_BAUDRATE);
    if (freq > 0) {
        /* set 8bit and enable uart1 */
        LL_UART0_5_SetDataWidth(UART1_CORE1, LL_UART0_5_DATAWIDTH_8B);
        LL_UART0_5_Enable(UART1_CORE1);
    }
#endif
    WDT_FEED;
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
    UNUSED(fd);
    for (i = 0; i < len; i++) {
        send_char(ptr[i]);
    }
    return len;
}

int _read(int fd, char* ptr, int len)
{
    int i;
    UNUSED(fd);
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
