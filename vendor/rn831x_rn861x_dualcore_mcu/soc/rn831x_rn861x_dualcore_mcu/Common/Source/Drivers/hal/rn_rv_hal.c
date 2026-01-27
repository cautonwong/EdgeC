/**
 ******************************************************************************
 * @file   rn_rv_hal.c
 * @author Renergy Software Team
 * @brief  HAL module driver.
 *         This is the common part of the HAL initialization.
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
#include "rn_rv_hal.h"
#include "rn_rv_ll_system_core0.h"
#include "rn_rv_ll_system_core1.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @defgroup HAL HAL
 * @brief     HAL module driver.
 * @{
 */

#ifdef HAL_MODULE_ENABLED

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/** @defgroup HAL_Private_Constants HAL Private Constants
 * @{
 */

/**
 * @brief RN HAL Driver version number
 */
#define __RN_HAL_VERSION_MAIN   (0x00U) /*!< [31:24] main version */
#define __RN_HAL_VERSION_SUB1   (0x00U) /*!< [23:16] sub1 version */
#define __RN_HAL_VERSION_SUB2   (0x02U) /*!< [15:8]  sub2 version */
#define __RN_HAL_VERSION        ((__RN_HAL_VERSION_MAIN << 16) | \
                                 (__RN_HAL_VERSION_SUB1 << 8) | \
                                 (__RN_HAL_VERSION_SUB2))
/**
 * @}
 */

/* Private macro ------------------------------------------------------------*/
/** @defgroup HAL_Private_Macros HAL Private Macros
 * @{
 */
#define USEC_TO_CYCLES(__us__, __cpuFreqHz__)   (us * (__cpuFreqHz__ / 1000000UL))
#define MSEC_TO_CYCLES(__ms__, __cpuFreqHz__)   (ms * (__cpuFreqHz__ / 1000UL))
#define SEC_TO_CYCLES(__s__, __cpuFreqHz__)     (s * (__cpuFreqHz__))
/**
 * @}
 */

/* Private variables --------------------------------------------------------*/
/** @defgroup HAL_Private_Variables HAL Private Variables
 * @{
 */
__IO uint32_t uwTick;
const uint32_t SysCoreClk[] = {32768UL, 1843200UL, 3686400UL, 7372800UL, 14745600UL, 29491200UL, 58982400UL};
/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/** @defgroup HAL_Private_Functions HAL Private Functions
 * @{
 */

/**
 * @brief  Get the IAP function address
 * @note   This function is only use to obtain iap header address for hal interface,
 *         and the modules involved are SYSTEM, SEA, KEYSTORE, FLASH.
 * @param  Address: pointer to IAP function address.
 * @retval IAP function address for remap1
 */
#ifdef CORE0
HAL_StatusTypeDef GetIAPHeaderAddress(uint32_t *Address)
{
    uint32_t iap_addr_offset = *(volatile uint32_t*)(ROM_BASE_ADDR + RN_LIB_OFFSET);
    if ((iap_addr_offset < ROM_SIZE) && ((iap_addr_offset & 0x1) == 0U)) {
        *Address = (uint32_t)(iap_addr_offset + ROM_BASE_ADDR);
    } else {
        return HAL_ERROR;
    }
    return HAL_OK;
}
#endif
/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup HAL_Exported_Functions HAL Exported Functions
 * @{
 */

/** @defgroup HAL_Exported_Functions_Group1 Initialization and de-initialization Functions
 *  @brief    Initialization and de-initialization functions
 *
@verbatim
===============================================================================
            ##### Initialization and de-initialization functions #####
===============================================================================
    [..]  This section provides functions allowing to:
        (+) Initializes the Flash interface, the NVIC allocation and initial clock
            configuration. It initializes the systick also when timeout is needed
            and the backup domain when enabled.
        (+) de-Initializes common part of the HAL.
        (+) Configure The time base source to have 1ms time base with a dedicated
            Tick interrupt priority.
        (++) SysTick timer is used by default as source of time base, but user
            can eventually implement his proper time base source (a general purpose
            timer for example or other time source), keeping in mind that Time base
            duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
            handled in milliseconds basis.
        (++) Time base configuration function (HAL_InitTick()) is called automatically
            at the beginning of the program after reset by HAL_Init() or at any time
            when clock is configured, by HAL_RCC_ClockConfig().
        (++) Source of time base is configured to generate interrupts at regular
            time intervals. Care must be taken if HAL_Delay() is called from a
            peripheral ISR process, the Tick interrupt line must have higher priority
            (numerically lower) than the peripheral interrupt. Otherwise the caller
            ISR process will be blocked.
        (++) functions affecting time base configurations are declared as __WEAK
            to make override possible in case of other implementations in user file.
@endverbatim
 * @{
 */

/**
 * @brief  This function is used to initialize the HAL Library; it must be the first
 *         instruction to be executed in the main program (before to call any other
 *         HAL function), it performs the following:
 *           Configure the Flash prefetch.
 *           Configures the SysTick to generate an interrupt each 1 millisecond,
 *           which is clocked by the HSI (at this stage, the clock is not yet
 *           configured and thus the system is running from the internal HSI at 16 MHz).
 *           Set NVIC Group Priority to 4.
 *           Calls the HAL_MspInit() callback function defined in user file
 *           "stm32f1xx_hal_msp.c" to do the global low level hardware initialization
 *
 * @note   SysTick is used as time base for the HAL_Delay() function, the application
 *         need to ensure that the SysTick time base is always set to 1 millisecond
 *         to have correct HAL operation.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_Init(void)
{
    /* Configure Flash prefetch. */
    /* Prefetch buffer is not available on value line devices. */

    /* TODO: Set Interrupt Group Priority. */
    // HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    /* Use systick as time base source and configure 1ms tick (default clock after Reset is HSI). */
    HAL_InitTick();

    /* Init the low level hardware. */
    HAL_MspInit();

    /* Return function status */
    return HAL_OK;
}

/**
 * @brief  This function de-Initializes common part of the HAL and stops the systick.
 *         of time base.
 * @note   This function is optional.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
HAL_StatusTypeDef HAL_DeInit(void)
{
    /* TODO: Reset of all peripherals */

    /* De-Init the low level hardware */
    HAL_MspDeInit();

    /* Return function status */
    return HAL_OK;
}

/**
 * @brief  Initialize the MSP.
 * @retval None
 */
__WEAK void HAL_MspInit(void)
{
    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_MspInit could be implemented in the user file. */
}

/**
 * @brief  DeInitializes the MSP.
 * @retval None
 */
__WEAK void HAL_MspDeInit(void)
{
    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_MspDeInit could be implemented in the user file. */
}

/**
 * @brief  This function configures the source of the time base.
 *         The time source is configured to have 1ms time base with a dedicated
 *         Tick interrupt priority.
 * @note   This function is called automatically at the beginning of program after
 *         reset by HAL_Init() or at any time when clock is reconfigured by HAL_RCC_ClockConfig().
 * @note   In the default implementation, SysTick timer is the source of time base.
 *         It is used to generate interrupts at regular time intervals.
 *         Care must be taken if HAL_Delay() is called from a peripheral ISR process,
 *         The SysTick interrupt must have higher priority (numerically lower)
 *         than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
 *         The function is declared as __WEAK to be overwritten in case of other
 *         implementation in user file.
 * @retval Return HAL status: HAL_OK or HAL_ERROR.
 */
__WEAK HAL_StatusTypeDef HAL_InitTick(void)
{
    uint32_t mcounteren_val;
    __ASM volatile ("csrr %0, mcounteren" : "=r"(mcounteren_val));
    mcounteren_val |= (1U << 0U);
    __ASM volatile ("csrw mcounteren, %0" : : "r"(mcounteren_val));
    uwTick = HAL_GetTicks();
    return HAL_OK;
}

/**
 * @}
 */

/** @defgroup HAL_Exported_Functions_Group2 HAL Control functions
 *  @brief    HAL Control functions
 *
@verbatim
===============================================================================
                    ##### HAL Control functions #####
===============================================================================
    [..]  This section provides functions allowing to:
        (+) Provide a tick value in millisecond
        (+) Provide millisecond
        (+) Get the HAL API driver version

@endverbatim
 * @{
 */

/**
 * @brief  Provides a tick value in millisecond.
 * @note   This function is declared as __WEAK to be overwritten in case of other
 *         implementations in user file.
 * @retval tick value
 */
__WEAK uint64_t HAL_GetTicks(void)
{
    uint64_t cycles;
#if __riscv_xlen == 32
        uint32_t cycles_low, cycles_high, cycles_high_2;
        do {
            __ASM volatile ("rdcycleh %0" : "=r"(cycles_high));
            __ASM volatile ("rdcycle %0" : "=r"(cycles_low));
            __ASM volatile ("rdcycleh %0" : "=r"(cycles_high_2));
        } while (cycles_high != cycles_high_2);
        cycles = (((uint64_t)cycles_high << 32) | cycles_low);
#else
        __ASM volatile ("rdcycle %0" : "=r"(cycles));
#endif
    return cycles;
}

/**
 * @brief  Provides millisecond.
 * @note   This function is declared as __WEAK to be overwritten in case of other
 *         implementations in user file.
 * @retval Millisecond value
 */
__WEAK uint32_t HAL_GetMillis(void)
{
    return (uint32_t)((__get_hart_id() == 0U) \
            ? ((uint64_t)((uint64_t)(HAL_GetTicks() - uwTick) * 1000) / SysCoreClk[LL_SYSTEM_CORE0_GetCore0SystemClockFreq()]) \
            : ((uint64_t)((uint64_t)(HAL_GetTicks() - uwTick) * 1000) / SysCoreClk[LL_SYSTEM_CORE1_GetCore1SystemClockFreq() >> 4U]));
}

/**
 * @brief  Provides microsecond.
 * @note   This function is declared as __WEAK to be overwritten in case of other
 *         implementations in user file.
 * @retval Microsecond value
 */
__WEAK uint32_t HAL_GetMicros(void)
{
    return (uint32_t)((__get_hart_id() == 0U) \
            ? ((uint64_t)((uint64_t)(HAL_GetTicks() - uwTick) * 1000000) / SysCoreClk[LL_SYSTEM_CORE0_GetCore0SystemClockFreq()]) \
            : ((uint64_t)((uint64_t)(HAL_GetTicks() - uwTick) * 1000000) / SysCoreClk[LL_SYSTEM_CORE1_GetCore1SystemClockFreq() >> 4U]));
}

/**
 * @brief  Return current running core id.
 * @retval Core id
 */
uint32_t HAL_GetCoreId(void)
{
    return __get_hart_id();
}

/**
 * @brief  This function provides minimum delay (in milliseconds) based
 *         on variable incremented.
 * @note   In the default implementation, SysTick timer is the source of time base.
 *         It is used to generate interrupts at regular time intervals where uwTick
 *         is incremented.
 * @note   This function is declared as __WEAK to be overwritten in case of other
 *         implementations in user file.
 * @param  Delay specifies the delay time length, in milliseconds.
 * @retval None
 */
__WEAK void HAL_Delay(uint32_t Delay)
{
    uint32_t start_ms = HAL_GetMillis();
    while ((uint32_t)(HAL_GetMillis() - start_ms) < Delay);
}

/**
 * @brief  Returns the HAL revision.
 * @retval version 0xXYZR (8bits for each decimal, R for RC).
 */
uint32_t HAL_GetHalVersion(void)
{
    return __RN_HAL_VERSION;
}

/**
 * @}
 */

/**
 * @}
 */

#endif /* HAL_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
