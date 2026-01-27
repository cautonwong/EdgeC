/**
 ******************************************************************************
 * @file   rn831x_rn861x_mcu.h
 * @author Renergy Software Team
 * @brief  NMSIS Core Peripheral Access Layer Header File for Nuclei SoC
 *         which support Nuclei N/NX class cores.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2019 Nuclei Limited. All rights reserved.
 * Copyright (c) 2025 Renergy Technology Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

#ifndef __RN831X_RN861X_MCU_H__
#define __RN831X_RN861X_MCU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief This SDK library code is suitable for 32-bit RISC-V dual-core MCUs,
 * of which Core1 is the application core, the performance is equivalent to M33,
 * and the highest frequency is 58.9824MHz. Core0 is a security core with
 * the same performance as M23 and meets the TEE architecture.
 * The highest frequency is 29.4912MHz.
 *
 * It is recommended to use Core0 for single-core applications and
 * add the macros "-DCORE0 -DUSE_FULL_LL_DIRVER" to the project.
 *
 * If you need a higher frequency and the FPU function, you can choose to
 * use Core1 and specify the macros "-DCORE1 -DUSE_FULL_LL_DIRVER".
 *
 * We do not recommand adding macros to the code. We recommend executing
 * "Extra Common Flags" or "Extra C Flags" in Nuclei Studio.
 *
 * Note that only one of CORE0 or CORE1 can be used, not at the same time,
 * otherwise a usage error will occur.
 */
// #define CORE0
// #define CORE1

/** @addtogroup Nuclei
 * @{
 */

/** @addtogroup rn831x_rn861x_mcu
 * @{
 */

/** @addtogroup Configuration_of_NMSIS
 * @{
 */

/**
 * @brief   SoC Download mode definition
 */
typedef enum
{
    DOWNLOAD_MODE_FLASH = 0U,                    /*!< Flash download mode */
    DOWNLOAD_MODE_SRAM  = 1U,                    /*!< sram download mode */
    DOWNLOAD_MODE_MAX,
} DownloadMode_Type;

/**
 * @brief   CPU Internal Region Information
 */
typedef struct IRegion_Info
{
    unsigned long iregion_base;                  /*!< Internal region base address */
    unsigned long eclic_base;                    /*!< eclic base address */
    unsigned long systimer_base;                 /*!< system timer base address */
    unsigned long smp_base;                      /*!< smp base address */
    unsigned long idu_base;                      /*!< idu base address */
} IRegion_Info_Type;

/* =========================================================================================================================== */
/* ================                                Interrupt Number Definition                                ================ */
/* =========================================================================================================================== */

typedef enum IRQn
{
/* ========================================  Nuclei Core Specific Interrupt Numbers  ========================================= */
    Reserved0_IRQn            = 0,               /*!< Internal reserved */
    Reserved1_IRQn            = 1,               /*!< Internal reserved */
    Reserved2_IRQn            = 2,               /*!< Internal reserved */
    SysTimerSW_IRQn           = 3,               /*!< System Timer SW interrupt for both M/S mode in ECLIC */
    Reserved3_IRQn            = 4,               /*!< Internal reserved */
    Reserved4_IRQn            = 5,               /*!< Internal reserved */
    Reserved5_IRQn            = 6,               /*!< Internal reserved */
    SysTimer_IRQn             = 7,               /*!< System Timer Interrupt for both M/S mode in ECLIC */
    Reserved6_IRQn            = 8,               /*!< Internal reserved */
    Reserved7_IRQn            = 9,               /*!< Internal reserved */
    Reserved8_IRQn            = 10,              /*!< Internal reserved */
    Reserved9_IRQn            = 11,              /*!< Internal reserved */
    Reserved10_IRQn           = 12,              /*!< Internal reserved */
    Reserved11_IRQn           = 13,              /*!< Internal reserved */
    Reserved12_IRQn           = 14,              /*!< Internal reserved */
    Reserved13_IRQn           = 15,              /*!< Internal reserved */
    InterCore_IRQn            = 16,              /*!< CIDU Inter Core Interrupt */
    Reserved15_IRQn           = 17,              /*!< Internal reserved */
    Reserved16_IRQn           = 18,              /*!< Internal reserved */
/* ======================================  rn831x_rn861x_mcu Specific Interrupt Numbers  ===================================== */
    INTERCORE_IRQn            = 19,              /*!< Device Interrupt */
    Reserved17_IRQn           = 20,              /*!< Device Interrupt */
    Reserved18_IRQn           = 21,              /*!< Device Interrupt */
    Reserved19_IRQn           = 22,              /*!< Device Interrupt */
    Reserved20_IRQn           = 23,              /*!< Device Interrupt */
    Reserved21_IRQn           = 24,              /*!< Device Interrupt */
    Reserved22_IRQn           = 25,              /*!< Device Interrupt */
    Reserved23_IRQn           = 26,              /*!< Device Interrupt */
    Reserved24_IRQn           = 27,              /*!< Device Interrupt */
    Reserved25_IRQn           = 28,              /*!< Device Interrupt */
    Reserved26_IRQn           = 29,              /*!< Device Interrupt */
    Reserved27_IRQn           = 30,              /*!< Device Interrupt */
    Reserved28_IRQn           = 31,              /*!< Device Interrupt */
    Reserved29_IRQn           = 32,              /*!< Device Interrupt */
    Reserved30_IRQn           = 33,              /*!< Device Interrupt */
    Reserved31_IRQn           = 34,              /*!< Device Interrupt */
    Reserved32_IRQn           = 35,              /*!< Device Interrupt */
    Reserved33_IRQn           = 36,              /*!< Device Interrupt */
    Reserved34_IRQn           = 37,              /*!< Device Interrupt */
    Reserved35_IRQn           = 38,              /*!< Device Interrupt */
    Reserved36_IRQn           = 39,              /*!< Device Interrupt */
    Reserved37_IRQn           = 40,              /*!< Device Interrupt */
    Reserved38_IRQn           = 41,              /*!< Device Interrupt */
    Reserved39_IRQn           = 42,              /*!< Device Interrupt */
    Reserved40_IRQn           = 43,              /*!< Device Interrupt */
    Reserved41_IRQn           = 44,              /*!< Device Interrupt */
    Reserved42_IRQn           = 45,              /*!< Device Interrupt */
    LCD_IRQn                  = 46,              /*!< Device Interrupt */
    KBI1_IRQ0                 = 47,              /*!< Device Interrupt */
    KBI1_IRQ1                 = 48,              /*!< Device Interrupt */
    KBI1_IRQ2                 = 49,              /*!< Device Interrupt */
    KBI1_IRQ3                 = 50,              /*!< Device Interrupt */
    KBI1_IRQ4                 = 51,              /*!< Device Interrupt */
    KBI1_IRQ5                 = 52,              /*!< Device Interrupt */
    KBI1_IRQ6                 = 53,              /*!< Device Interrupt */
    KBI1_IRQ7                 = 54,              /*!< Device Interrupt */
    KBI1_IRQ8                 = 55,              /*!< Device Interrupt */
    KBI1_IRQ9                 = 56,              /*!< Device Interrupt */
    KBI1_IRQ10                = 57,              /*!< Device Interrupt */
    KBI1_IRQ11                = 58,              /*!< Device Interrupt */
    KBI1_IRQ12                = 59,              /*!< Device Interrupt */
    KBI1_IRQ13                = 60,              /*!< Device Interrupt */
    KBI1_IRQ14                = 61,              /*!< Device Interrupt */
    KBI1_IRQ15                = 62,              /*!< Device Interrupt */
    KBI1_IRQn                 = 63,              /*!< Device Interrupt */
    KBI0_IRQ0                 = 64,              /*!< Device Interrupt */
    KBI0_IRQ1                 = 65,              /*!< Device Interrupt */
    KBI0_IRQ2                 = 66,              /*!< Device Interrupt */
    KBI0_IRQ3                 = 67,              /*!< Device Interrupt */
    KBI0_IRQ4                 = 68,              /*!< Device Interrupt */
    KBI0_IRQ5                 = 69,              /*!< Device Interrupt */
    KBI0_IRQ6                 = 70,              /*!< Device Interrupt */
    KBI0_IRQ7                 = 71,              /*!< Device Interrupt */
    KBI0_IRQ8                 = 72,              /*!< Device Interrupt */
    KBI0_IRQ9                 = 73,              /*!< Device Interrupt */
    KBI0_IRQ10                = 74,              /*!< Device Interrupt */
    KBI0_IRQ11                = 75,              /*!< Device Interrupt */
    KBI0_IRQ12                = 76,              /*!< Device Interrupt */
    KBI0_IRQ13                = 77,              /*!< Device Interrupt */
    KBI0_IRQ14                = 78,              /*!< Device Interrupt */
    KBI0_IRQ15                = 79,              /*!< Device Interrupt */
    KBI0_IRQn                 = 80,              /*!< Device Interrupt */
    WDT0_IRQn                 = 81,              /*!< Device Interrupt */
    WDT1_IRQn                 = 82,              /*!< Device Interrupt */
    GDMA11_IRQn               = 83,              /*!< Device Interrupt */
    GDMA10_IRQn               = 84,              /*!< Device Interrupt */
    GDMA01_IRQn               = 85,              /*!< Device Interrupt */
    GDMA00_IRQn               = 86,              /*!< Device Interrupt */
    Reserved84_IRQn           = 87,              /*!< Device Interrupt */
    SIMPTC0_0_IRQ             = 88,              /*!< Device Interrupt */
    SIMPTC0_1_IRQ             = 89,              /*!< Device Interrupt */
    SIMPTC0_2_IRQ             = 90,              /*!< Device Interrupt */
    SIMPTC0_3_IRQ             = 91,              /*!< Device Interrupt */
    SIMPTC1_0_IRQ             = 92,              /*!< Device Interrupt */
    SIMPTC1_1_IRQ             = 93,              /*!< Device Interrupt */
    SIMPTC1_2_IRQ             = 94,              /*!< Device Interrupt */
    SIMPTC1_3_IRQ             = 95,              /*!< Device Interrupt */
    SIMPTC2_0_IRQ             = 96,              /*!< Device Interrupt */
    SIMPTC2_1_IRQ             = 97,              /*!< Device Interrupt */
    SIMPTC2_2_IRQ             = 98,              /*!< Device Interrupt */
    SIMPTC2_3_IRQ             = 99,              /*!< Device Interrupt */
    SIMPTC3_0_IRQ             = 100,             /*!< Device Interrupt */
    SIMPTC3_1_IRQ             = 101,             /*!< Device Interrupt */
    SIMPTC3_2_IRQ             = 102,             /*!< Device Interrupt */
    SIMPTC3_3_IRQ             = 103,             /*!< Device Interrupt */
    TC0_IRQn                  = 104,             /*!< Device Interrupt */
    TC1_IRQn                  = 105,             /*!< Device Interrupt */
    ISO78160_IRQn             = 106,             /*!< Device Interrupt */
    ISO78161_IRQn             = 107,             /*!< Device Interrupt */
    I2C0_IRQn                 = 108,             /*!< Device Interrupt */
    I2C1_IRQn                 = 109,             /*!< Device Interrupt */
    LPUART_IRQn               = 110,             /*!< Device Interrupt */
    LSRX_IRQn                 = 111,             /*!< Device Interrupt */
    UART0_IRQn                = 112,             /*!< Device Interrupt */
    UART1_IRQn                = 113,             /*!< Device Interrupt */
    UART2_IRQn                = 114,             /*!< Device Interrupt */
    UART3_IRQn                = 115,             /*!< Device Interrupt */
    UART4_IRQn                = 116,             /*!< Device Interrupt */
    UART5_IRQn                = 117,             /*!< Device Interrupt */
    UART6_IRQn                = 118,             /*!< Device Interrupt */
    UART7_IRQn                = 119,             /*!< Device Interrupt */
    USART0_IRQn               = 120,             /*!< Device Interrupt */
    USART1_IRQn               = 121,             /*!< Device Interrupt */
    Reserved119_IRQn          = 122,             /*!< Device Interrupt */
    SPI0_IRQn                 = 123,             /*!< Device Interrupt */
    SPI1_IRQn                 = 124,             /*!< Device Interrupt */
    SPI2_IRQn                 = 125,             /*!< Device Interrupt */
    SPI3_IRQn                 = 126,             /*!< Device Interrupt */
    QSPI0_IRQn                = 127,             /*!< Device Interrupt */
    QSPI1_IRQn                = 128,             /*!< Device Interrupt */
    DSP_IRQn                  = 129,             /*!< Device Interrupt */
    M2M_IRQn                  = 130,             /*!< Device Interrupt */
    AES_IRQn                  = 131,             /*!< Device Interrupt */
    SEA_IRQn                  = 132,             /*!< Device Interrupt */
    KEYSTORE_IRQn             = 133,             /*!< Device Interrupt */
    DAC_IRQn                  = 134,             /*!< Device Interrupt */
    ADC0_IRQn                 = 135,             /*!< Device Interrupt */
    D2F_IRQn                  = 136,             /*!< Device Interrupt */
    RTC_IRQn                  = 137,             /*!< Device Interrupt */
    VBAT_IRQn                 = 138,             /*!< Device Interrupt */
    CMPLVD_IRQn               = 139,             /*!< Device Interrupt */
    LCSS_IRQn                 = 140,             /*!< Device Interrupt */
    SCU_IRQn                  = 141,             /*!< Device Interrupt */
    CORE1_RST_IRQn            = 142,             /*!< Device Interrupt */
    CORE1_SYSREG_IRQn         = 143,             /*!< Device Interrupt */
    CORE0_SYSREG_IRQn         = 144,             /*!< Device Interrupt */
    MAILBOXINTER_IRQn         = 145,             /*!< Device Interrupt */
    MAILBOXHOST0_IRQn         = 146,             /*!< Device Interrupt */
    IRQn_MAX,
} IRQn_Type;

/* =========================================================================================================================== */
/* ================                                  Exception Code Definition                                ================ */
/* =========================================================================================================================== */
typedef enum EXCn
{
/* ==========================================  Nuclei N/NX Specific Exception Code  ========================================== */
    InsUnalign_EXCn          =  0,               /*!< Instruction address misaligned */
    InsAccFault_EXCn         =  1,               /*!< Instruction access fault */
    IlleIns_EXCn             =  2,               /*!< Illegal instruction */
    Break_EXCn               =  3,               /*!< Beakpoint */
    LdAddrUnalign_EXCn       =  4,               /*!< Load address misaligned */
    LdFault_EXCn             =  5,               /*!< Load access fault */
    StAddrUnalign_EXCn       =  6,               /*!< Store or AMO address misaligned */
    StAccessFault_EXCn       =  7,               /*!< Store or AMO access fault */
    UmodeEcall_EXCn          =  8,               /*!< Environment call from User mode */
    SmodeEcall_EXCn          =  9,               /*!< Environment call from S-mode */
    MmodeEcall_EXCn          =  11,              /*!< Environment call from Machine mode */
    InsPageFault_EXCn        =  12,              /*!< Instruction page fault */
    LdPageFault_EXCn         =  13,              /*!< Load page fault */
    StPageFault_EXCn         =  15,              /*!< Store or AMO page fault */
    StackOverflow_EXCn       =  24,              /*!< Stack overflow fault */
    StackUnderflow_EXCn      =  25,              /*!< Stack underflow fault */
    NMI_EXCn                 =  0xFFF,           /*!< NMI interrupt */
} EXCn_Type;

typedef struct EXC_Frame
{
    unsigned long ra;                            /*!< ra: x1, return address for jump */
    unsigned long tp;                            /*!< tp: x4, thread pointer */
    unsigned long t0;                            /*!< t0: x5, temporary register 0 */
    unsigned long t1;                            /*!< t1: x6, temporary register 1 */
    unsigned long t2;                            /*!< t2: x7, temporary register 2 */
    unsigned long a0;                            /*!< a0: x10, return value or function argument 0 */
    unsigned long a1;                            /*!< a1: x11, return value or function argument 1 */
    unsigned long a2;                            /*!< a2: x12, function argument 2 */
    unsigned long a3;                            /*!< a3: x13, function argument 3 */
    unsigned long a4;                            /*!< a4: x14, function argument 4 */
    unsigned long a5;                            /*!< a5: x15, function argument 5 */
    unsigned long cause;                         /*!< cause: machine/supervisor mode cause csr register */
    unsigned long epc;                           /*!< epc: machine/ supervisor mode exception program counter csr register */
    unsigned long msubm;                         /*!< msubm: machine sub-mode csr register, nuclei customized, exclusive to machine mode */
#ifndef __riscv_32e
    unsigned long a6;                            /*!< a6: x16, function argument 6 */
    unsigned long a7;                            /*!< a7: x17, function argument 7 */
    unsigned long t3;                            /*!< t3: x28, temporary register 3 */
    unsigned long t4;                            /*!< t4: x29, temporary register 4 */
    unsigned long t5;                            /*!< t5: x30, temporary register 5 */
    unsigned long t6;                            /*!< t6: x31, temporary register 6 */
#endif
} EXC_Frame_Type;

/* =========================================================================================================================== */
/* ================                           Processor and Core Peripheral Section                           ================ */
/* =========================================================================================================================== */
#define __NUCLEI_N_REV            0x0104         /*!< Core Revision r1p4 */

#define __NUCLEI_CPU_REV          0x030600       /*!< Nuclei CPU Core Revision, version X.Y.Z, this is for the CPU Core Version, you get from Nuclei, eg. N300 v3.10.1, it should be 0x030A01 */
#define __NUCLEI_CPU_SERIES       0x0300         /*!< Nuclei CPU Series, such as 200/300/600/900, eg. 900 will be 0x0900 */
#define __ECLIC_PRESENT           1              /*!< Set to 1 if ECLIC is present */
#define __ECLIC_BASEADDR          0xe0020000UL   /*!< Set to ECLIC baseaddr of your device */

#define __ECLIC_INTCTLBITS        3              /*!< Set to 1 - 8, the number of hardware bits are actually implemented in the clicintctl registers. */
#define __ECLIC_INTNUM            147            /*!< Set to 1 - 1024, total interrupt number of ECLIC Unit */
#define __SYSTIMER_PRESENT        1              /*!< Set to 1 if System Timer is present */
#define __SYSTIMER_BASEADDR       0xe0030000UL   /*!< Set to SysTimer baseaddr of your device */
#define __Vendor_SysTickConfig    0              /*!< Set to 0 if need systick configuration */

/*!< Set to 0, 1, or 2, 0 not present, 1 single floating point unit present, 2 double floating point unit present */
#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define __FPU_PRESENT             0
#endif

#if defined(CORE1)
#define __FPU_PRESENT             1
#endif

/* __riscv_bitmanip/__riscv_dsp/__riscv_vector is introduced
 * in nuclei gcc 10.2 when b/p/v extension compiler option is selected.
 * For example:
 * -march=rv32imacb -mabi=ilp32 : __riscv_bitmanip macro will be defined
 * -march=rv32imacp -mabi=ilp32 : __riscv_dsp macro will be defined
 * -march=rv64imacv -mabi=lp64 : __riscv_vector macro will be defined
 */
#define __BITMANIP_PRESENT        1              /*!< Set to 1 if Bitmainpulation extension is present */
#define __DSP_PRESENT             0              /*!< Set to 1 if Partial SIMD(DSP) extension is present */
#define __VECTOR_PRESENT          0              /*!< Set to 1 if Vector extension is present */

#define __PMP_PRESENT             1              /*!< Set to 1 if PMP is present */
#define __PMP_ENTRY_NUM           8              /*!< Set to 8 or 16, the number of PMP entries */

#define __SPMP_PRESENT            0              /*!< Set to 1 if SPMP is present */
#define __TEE_PRESENT             0              /*!< Set to 1 if TEE is present */


#define __ICACHE_PRESENT          1              /*!< Set to 1 if I-Cache is present */
#if defined(CORE0)
#define __DCACHE_PRESENT          0              /*!< Set to 1 if D-Cache is present */
#endif
#if defined(CORE1)
#define __DCACHE_PRESENT          1              /*!< Set to 1 if D-Cache is present */
#endif
#define __CCM_PRESENT             1              /*!< Set to 1 if Cache Control and Mantainence Unit is present */

#include "nmsis_core.h"                          /*!< Nuclei N/NX class processor and core peripherals */
#include "system_rn831x_rn861x_mcu.h"            /*!< System Header file */

/* ========================================  Start of section using anonymous unions  ======================================== */
#define RTC_FREQ                  32768
#define SOC_TIMER_FREQ            RTC_FREQ       /*!< The TIMER frequency is just the RTC frequency */

/**
 * @}
 */

/** @addtogroup Exported_types
 * @{
 */
typedef enum __attribute__((__packed__))
{
    RESET   = 0,
    SET     = !RESET
} FlagStatus, ITStatus;

typedef enum __attribute__((__packed__))
{
    DISABLE = 0,
    ENABLE  = !DISABLE
} FunctionalState;

typedef enum __attribute__((__packed__))
{
    SUCCESS = 0U,
    ERROR   = !SUCCESS
} ErrorStatus;

#define IS_FUNCTIONAL_STATE(STATE) (((STATE) == DISABLE) || ((STATE) == ENABLE))

#if !defined(UNUSED)
#define UNUSED(X) (void)X      /* To avoid gcc/g++ warnings */
#endif /* UNUSED */

/**
 * @}
 */

/** @addtogroup Exported_macro
 * @{
 */

#ifdef USE_FULL_ASSERT
/**
 * @brief  The assert_param macro is used for function's parameters check.
 * @param  expr If expr is false, it calls assert_failed function
 *         which reports the name of the source file and the source
 *         line number of the call that failed.
 *         If expr is true, it returns no value.
 * @retval None
 */
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
/* Exported functions ------------------------------------------------------- */
void assert_failed(uint8_t* file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif /* USE_FULL_ASSERT */

/* bit operations */
#define REG32(addr)                         (*(volatile uint32_t *)(uint32_t)(addr))
#define REG16(addr)                         (*(volatile uint16_t *)(uint32_t)(addr))
#define REG8(addr)                          (*(volatile uint8_t *)(uint32_t)(addr))
#define BIT(x)                              ((uint32_t)((uint32_t)0x01U << (x)))
#define BITS(start, end)                    (((0xFFFFFFFFUL >> start) << (start)) & (0xFFFFFFFFUL >> (31U - (uint32_t)(end))))
#define GET_BITS(regval, start, end)        (((regval)&BITS((start), (end))) >> (start))

#define SET_BIT(REG, BIT)                   ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT)                 ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT)                  ((REG) & (BIT))
#define CLEAR_REG(REG)                      ((REG) = (0x0))
#define WRITE_REG(REG, VAL)                 ((REG) = (VAL))
#define READ_REG(REG)                       ((REG))
#define MODIFY_REG(REG, CLEARMASK, SETMASK) WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | ((SETMASK) & (CLEARMASK))))
// #define MODIFY_REG(REG, CLEARMASK, SETMASK) WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | SETMASK))
#define POSITION_VAL(VAL)                   (__CLZ(__RBIT(VAL)))

/* 32-bit atomic load exclusive */
__STATIC_FORCEINLINE uint32_t __RV_LRW(volatile uint32_t *addr)
{
    uint32_t result;
    __ASM volatile ("lr.w %0, (%1)"  \
                    : "=r" (result)  \
                    : "r" (addr)     \
                    : "memory");
    return result;
}

/* 32-bit atomic store exclusive */
__STATIC_FORCEINLINE uint32_t __RV_SCW(uint32_t value, volatile uint32_t *addr)
{
    uint32_t result;
    __ASM volatile ("sc.w %0, %1, (%2)"        \
                    : "=&r" (result)           \
                    : "r" (value), "r" (addr)  \
                    : "memory");
    return result; /* 0: success, others: failure */
}

/* Atomic 32-bit register access macro to set one or several bits */
#define ATOMIC_SET_BIT(REG, BIT)                                                 \
    do {                                                                         \
        uint32_t val;                                                            \
        do {                                                                     \
            val = __RV_LRW((__IO uint32_t *)&(REG)) | (BIT);                     \
        } while ((__RV_SCW(val,(__IO uint32_t *)&(REG))) != 0U);                 \
    } while(0)

/* Atomic 32-bit register access macro to clear one or several bits */
#define ATOMIC_CLEAR_BIT(REG, BIT)                                               \
    do {                                                                         \
        uint32_t val;                                                            \
        do {                                                                     \
            val = __RV_LRW((__IO uint32_t *)&(REG)) & ~(BIT);                    \
        } while ((__RV_SCW(val,(__IO uint32_t *)&(REG))) != 0U);                 \
    } while(0)

/* Atomic 32-bit register access macro to clear and set one or several bits */
#define ATOMIC_MODIFY_REG(REG, CLEARMSK, SETMASK)                                \
    do {                                                                         \
        uint32_t val;                                                            \
        do {                                                                     \
            val = (__RV_LRW((__IO uint32_t *)&(REG)) & ~(CLEARMSK)) | (SETMASK); \
        } while ((__RV_SCW(val,(__IO uint32_t *)&(REG))) != 0U);                 \
    } while(0)

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* __RN831X_RN861X_MCU_H__ */
