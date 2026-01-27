/**
 ******************************************************************************
 * @file   main.c
 * @author Renergy Software Team
 * @brief  Hello world demo.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include "main.h"
#include "uart.h"
#include "gpio.h"
#include "privilege.h"

/* Private includes ---------------------------------------------------------*/
#include "rn_rv_ll_wdt.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

/* Create a stack for User Mode execution. */
uint8_t user_stack[768] __attribute__((aligned(16)));

/* Create the register file for User Mode execution. */
struct metal_register_file my_u_mode_regfile =
{
	.sp = (uintptr_t)(user_stack + sizeof(user_stack)),
};

/* Create a stack for Supervisor Mode execution. */
uint8_t supervisor_stack[768] __attribute__((aligned(16)));

/* Create the register file for Supervisor Mode execution. */
struct metal_register_file my_smode_regfile =
{
	.sp = (uintptr_t)(supervisor_stack + sizeof(supervisor_stack)),
};

void print_sp_judge_privilege_mode(void)
{
	metal_xreg_t sp;

	__asm__ volatile("add  %0, x0, sp" :"=r"(sp));
	printf("Current sp is %p,  ", sp);

	if (((uint8_t *)sp <= &(supervisor_stack[767])) && ((uint8_t *)sp >= &(supervisor_stack[0]))) {
		printf("So it is in Supervisor Mode! \r\n");
	} else if (((uint8_t *)sp <= &(user_stack[767])) && ((uint8_t *)sp >= &(user_stack[0]))) {
		printf("So it is in User Mode! \r\n");
	} else {
		printf("So it is in Machine Mode! \r\n");
	}
}

void user_mode_entry_point(void)
{
	printf("[IN U-MODE ENTRY POINT] Hello User Mode!!!\r\n");

	print_sp_judge_privilege_mode();

	// __ECALL();

	/* Here change privilege mode to M-mode. */
	print_sp_judge_privilege_mode();

    while (1) {
        LL_WDT_ReloadCounter(WDT); // Feed wdt
    }
}

void supervisor_mode_entry_point(void)
{
	printf("[IN S-MODE ENTRY POINT] Hello Supervisor Mode!!!\r\n");

	print_sp_judge_privilege_mode();

	/* Drop to U mode */

    while (1) {
        LL_WDT_ReloadCounter(WDT); // Feed wdt
    }
}

void machine_mode_entry_point(void)
{
	printf("[IN M-MODE ENTRY POINT] Hello Machine Mode!!!\r\n");

	print_sp_judge_privilege_mode();

    while (1) {
        LL_WDT_ReloadCounter(WDT); // Feed wdt
    }
}

void ecall_from_u_mode_exception_handler(unsigned long mcause, unsigned long sp)
{
    printf("ecall from U-mode success!!!\r\n");

	metal_privilege_update_from_U_to_M(my_u_mode_regfile, machine_mode_entry_point);
}

void ecall_from_m_mode_exception_handler(unsigned long mcause, unsigned long sp)
{
    printf("ecall from M-mode success!!!\r\n");
}

int main(void)
{
    /* Reset of all peripherals, Initializes the Systick. */
    HAL_Init();

    /* Enable WDT clock */
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_WDT_CLK_ENABLE();
    __HAL_SYSTEM_PSW_LOCK();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_UART0_Init();

    printf("\r\nHart %d, MISA: 0x%lx\r\n", __RV_CSR_READ(CSR_MHARTID), __RV_CSR_READ(CSR_MISA));

	/* Set pmp, so S-mode and U-mode can access all address range. */
    /* Configuration of execution region*/
    // pmp_config pmpconfig = {
    //     /*
    //      * L bit will lock PMP entries remain locked until the hart is reset,
    //      * the L bit also indicates whether the R/W/X permissions are enforced on M-mode accesses lo
    //      */
    //     .protection = (PMP_R | PMP_W | PMP_X),
    //     /* Initial protected excutable address range is 2^12 = 4K bytes */
    //     .order = 12,
    //     /* initial base address is 0, change it to your memory assignment */
    //     .base_addr = 0,
    // };
    __set_PMPxCFG(0, 0x1F);
    __set_PMPADDRx(0, 0xFFFFFFFF);

    /* Register exception handler. */
    // Exception_Register_EXC(8,  (uint32_t)ecall_from_u_mode_exception_handler);
    // Exception_Register_EXC(11, (uint32_t)ecall_from_m_mode_exception_handler);

    print_sp_judge_privilege_mode();

    /* Drop to U-mode, valid in core0 or core1. */
    // metal_privilege_drop_from_M_to_mode(PRIVILEGE_USER_MODE, my_u_mode_regfile, user_mode_entry_point);

    /* Drop to S-mode, only valid in core1. */
     metal_privilege_drop_from_M_to_mode(PRIVILEGE_SUPERVISOR_MODE, my_smode_regfile, supervisor_mode_entry_point);

    while (1) {
        LL_WDT_ReloadCounter(WDT); // Feed wdt
    }
}

/**
 * @brief  This function is weak function, redirect printf output to UART.
 * @retval Size
 */
ssize_t _write(int fd, const void* ptr, size_t len)
{
    const uint8_t* writebuf = (const uint8_t*)ptr;
    HAL_UART_Transmit(&huart0, (uint8_t *)writebuf, len, 0xFF);
    return len;
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {

    };
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
        ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
