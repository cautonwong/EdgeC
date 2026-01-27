/*
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
 */
/******************************************************************************
 * \file     startup_demosoc.S
 * \brief    NMSIS N200/N300 based Core Device Startup File for
 *  Device RN831X/RN861X
 *
 ******************************************************************************/

#include "riscv_encoding.h"

.macro DECLARE_INT_HANDLER  INT_HDL_NAME
#if defined(__riscv_xlen) && (__riscv_xlen == 32)
    .word \INT_HDL_NAME
#else
    .dword \INT_HDL_NAME
#endif
.endm

    /*
     * Put the interrupt vectors in this section
     */

    .section .vtable

    .weak   eclic_msip_handler
    .weak   eclic_mtip_handler

    .weak   INTER_CORE_HANDLER              /* 19: Interrupt 19 */
    .weak   LCD_HANDLER                     /* 46: Interrupt 46 */
    .weak   KBI1_HANDLER0                   /* 47: Interrupt 47 */
    .weak   KBI1_HANDLER1                   /* 48: Interrupt 48 */
    .weak   KBI1_HANDLER2                   /* 49: Interrupt 49 */
    .weak   KBI1_HANDLER3                   /* 50: Interrupt 50 */
    .weak   KBI1_HANDLER4                   /* 51: Interrupt 51 */
    .weak   KBI1_HANDLER5                   /* 52: Interrupt 52 */
    .weak   KBI1_HANDLER6                   /* 53: Interrupt 53 */
    .weak   KBI1_HANDLER7                   /* 54: Interrupt 54 */
    .weak   KBI1_HANDLER8                   /* 55: Interrupt 55 */
    .weak   KBI1_HANDLER9                   /* 56: Interrupt 56 */
    .weak   KBI1_HANDLER10                  /* 57: Interrupt 57 */
    .weak   KBI1_HANDLER11                  /* 58: Interrupt 58 */
    .weak   KBI1_HANDLER12                  /* 59: Interrupt 59 */
    .weak   KBI1_HANDLER13                  /* 60: Interrupt 60 */
    .weak   KBI1_HANDLER14                  /* 61: Interrupt 61 */
    .weak   KBI1_HANDLER15                  /* 62: Interrupt 62 */
    .weak   KBI1_ALL_HANDLER                /* 63: Interrupt 63 */
    .weak   KBI0_HANDLER0                   /* 64: Interrupt 64 */
    .weak   KBI0_HANDLER1                   /* 65: Interrupt 65 */
    .weak   KBI0_HANDLER2                   /* 66: Interrupt 66 */
    .weak   KBI0_HANDLER3                   /* 67: Interrupt 67 */
    .weak   KBI0_HANDLER4                   /* 68: Interrupt 68 */
    .weak   KBI0_HANDLER5                   /* 69: Interrupt 69 */
    .weak   KBI0_HANDLER6                   /* 70: Interrupt 70 */
    .weak   KBI0_HANDLER7                   /* 71: Interrupt 71 */
    .weak   KBI0_HANDLER8                   /* 72: Interrupt 72 */
    .weak   KBI0_HANDLER9                   /* 73: Interrupt 73 */
    .weak   KBI0_HANDLER10                  /* 74: Interrupt 74 */
    .weak   KBI0_HANDLER11                  /* 75: Interrupt 75 */
    .weak   KBI0_HANDLER12                  /* 76: Interrupt 76 */
    .weak   KBI0_HANDLER13                  /* 77: Interrupt 77 */
    .weak   KBI0_HANDLER14                  /* 78: Interrupt 78 */
    .weak   KBI0_HANDLER15                  /* 79: Interrupt 79 */
    .weak   KBI0_ALL_HANDLER                /* 80: Interrupt 80 */
    .weak   WDT0_HANDLER                    /* 81: Interrupt 81 */
    .weak   WDT1_HANDLER                    /* 82: Interrupt 82 */
    .weak   GDMA11_HANDLER                  /* 83: Interrupt 83 */
    .weak   GDMA10_HANDLER                  /* 84: Interrupt 84 */
    .weak   GDMA01_HANDLER                  /* 85: Interrupt 85 */
    .weak   GDMA00_HANDLER                  /* 86: Interrupt 86 */
    .weak   SIMPTC0_0_HANDLER               /* 88: Interrupt 88 */
    .weak   SIMPTC0_1_HANDLER               /* 89: Interrupt 89 */
    .weak   SIMPTC0_2_HANDLER               /* 90: Interrupt 90 */
    .weak   SIMPTC0_3_HANDLER               /* 91: Interrupt 91 */
    .weak   SIMPTC1_0_HANDLER               /* 92: Interrupt 92 */
    .weak   SIMPTC1_1_HANDLER               /* 93: Interrupt 93 */
    .weak   SIMPTC1_2_HANDLER               /* 94: Interrupt 94 */
    .weak   SIMPTC1_3_HANDLER               /* 95: Interrupt 95 */
    .weak   SIMPTC2_0_HANDLER               /* 96: Interrupt 96 */
    .weak   SIMPTC2_1_HANDLER               /* 97: Interrupt 97 */
    .weak   SIMPTC2_2_HANDLER               /* 98: Interrupt 98 */
    .weak   SIMPTC2_3_HANDLER               /* 99: Interrupt 99 */
    .weak   SIMPTC3_0_HANDLER               /*100: Interrupt 100 */
    .weak   SIMPTC3_1_HANDLER               /*101: Interrupt 101 */
    .weak   SIMPTC3_2_HANDLER               /*102: Interrupt 102 */
    .weak   SIMPTC3_3_HANDLER               /*103: Interrupt 103 */
    .weak   TC0_HANDLER                     /*104: Interrupt 104 */
    .weak   TC1_HANDLER                     /*105: Interrupt 105 */
    .weak   ISO78160_HANDLER                /*106: Interrupt 106 */
    .weak   ISO78161_HANDLER                /*107: Interrupt 107 */
    .weak   I2C0_HANDLER                    /*108: Interrupt 108 */
    .weak   I2C1_HANDLER                    /*109: Interrupt 109 */
    .weak   LPUART_HANDLER                  /*110: Interrupt 110 */
    .weak   LSRX_HANDLER                    /*111: Interrupt 111 */
    .weak   UART0_HANDLER                   /*112: Interrupt 112 */
    .weak   UART1_HANDLER                   /*113: Interrupt 113 */
    .weak   UART2_HANDLER                   /*114: Interrupt 114 */
    .weak   UART3_HANDLER                   /*115: Interrupt 115 */
    .weak   UART4_HANDLER                   /*116: Interrupt 116 */
    .weak   UART5_HANDLER                   /*117: Interrupt 117 */
    .weak   UART6_HANDLER                   /*118: Interrupt 118 */
    .weak   UART7_HANDLER                   /*119: Interrupt 119 */
    .weak   USART0_HANDLER                  /*120: Interrupt 120 */
    .weak   USART1_HANDLER                  /*121: Interrupt 121 */
    .weak   SPI0_HANDLER                    /*123: Interrupt 123 */
    .weak   SPI1_HANDLER                    /*124: Interrupt 124 */
    .weak   SPI2_HANDLER                    /*125: Interrupt 125 */
    .weak   SPI3_HANDLER                    /*126: Interrupt 126 */
    .weak   QSPI0_HANDLER                   /*127: Interrupt 127 */
    .weak   QSPI1_HANDLER                   /*128: Interrupt 128 */
    .weak   DSP_HANDLER                     /*129: Interrupt 129 */
    .weak   M2M_HANDLER                     /*130: Interrupt 130 */
    .weak   AES_HANDLER                     /*131: Interrupt 131 */
    .weak   SEA_HANDLER                     /*132: Interrupt 132 */
    .weak   KEYSTORE_HANDLER                /*133: Interrupt 133 */
    .weak   DAC_HANDLER                     /*134: Interrupt 134 */
    .weak   HSADC_HANDLER                   /*135: Interrupt 135 */
    .weak   D2F_HANDLER                     /*136: Interrupt 136 */
    .weak   RTC_HANDLER                     /*137: Interrupt 137 */
    .weak   VBAT_HANDLER                    /*138: Interrupt 138 */
    .weak   LVDCMP_HANDLER                  /*139: Interrupt 139 */
    .weak   LCSS_HANDLER                    /*140: Interrupt 140 */
    .weak   SCU_HANDLER                     /*141: Interrupt 141 */
    .weak   CORE1_RST_HANDLER               /*142: Interrupt 142 */
    .weak   CORE1_SYSREG_HANDLER            /*143: Interrupt 143 */
    .weak   CORE0_SYSREG_HANDLER            /*144: Interrupt 144 */
    .weak   MAILBOX_INTERNAL_HANDLER        /*145: Interrupt 145 */
    .weak   MAILBOX_HOST_HANDLER            /*146: Interrupt 146 */

    .globl  vector_base
    .type   vector_base, @object
vector_base:
    j _start                                                /* 0: Reserved, Jump to _start when reset for vector table not remapped cases.*/
    .align LOG_REGBYTES                                     /*    Need to align 4 byte for RV32, 8 Byte for RV64 */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 1: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 2: Reserved */
    DECLARE_INT_HANDLER     eclic_msip_handler              /* 3: Machine software interrupt */

    DECLARE_INT_HANDLER     default_intexc_handler          /* 4: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 5: Reserved */
    DECLARE_INT_HANDLER     0                               /* 6: Don't modify(It's meaningful for programming verify) */
    DECLARE_INT_HANDLER     eclic_mtip_handler              /* 7: Machine timer interrupt */

    DECLARE_INT_HANDLER     default_intexc_handler          /* 8: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 9: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 10: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 11: Reserved */

    DECLARE_INT_HANDLER     default_intexc_handler          /* 12: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 13: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 14: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 15: Reserved */

    DECLARE_INT_HANDLER     default_intexc_handler          /* 16: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 17: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 18: Reserved */

    DECLARE_INT_HANDLER     INTER_CORE_HANDLER              /* 19: Interrupt 19 */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 20: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 21: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 22: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 23: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler        	/* 24: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler        	/* 25: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 26: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 27: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 28: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 29: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 30: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 31: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 32: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 33: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 34: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 35: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 36: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 37: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 38: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 39: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 40: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 41: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 42: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 43: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 44: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 45: Reserved */
    DECLARE_INT_HANDLER     LCD_HANDLER                    /* 46: Interrupt 46 */
    DECLARE_INT_HANDLER     KBI1_HANDLER0                  /* 47: Interrupt 47 */
    DECLARE_INT_HANDLER     KBI1_HANDLER1                  /* 48: Interrupt 48 */
    DECLARE_INT_HANDLER     KBI1_HANDLER2                  /* 49: Interrupt 49 */
    DECLARE_INT_HANDLER     KBI1_HANDLER3                  /* 50: Interrupt 50 */
    DECLARE_INT_HANDLER     KBI1_HANDLER4                  /* 51: Interrupt 51 */
    DECLARE_INT_HANDLER     KBI1_HANDLER5                  /* 52: Interrupt 52 */
    DECLARE_INT_HANDLER     KBI1_HANDLER6                  /* 53: Interrupt 53 */
    DECLARE_INT_HANDLER     KBI1_HANDLER7                  /* 54: Interrupt 54 */
    DECLARE_INT_HANDLER     KBI1_HANDLER8                  /* 55: Interrupt 55 */
    DECLARE_INT_HANDLER     KBI1_HANDLER9                  /* 56: Interrupt 56 */
    DECLARE_INT_HANDLER     KBI1_HANDLER10                 /* 57: Interrupt 57 */
    DECLARE_INT_HANDLER     KBI1_HANDLER11                 /* 58: Interrupt 58 */
    DECLARE_INT_HANDLER     KBI1_HANDLER12                 /* 59: Interrupt 59 */
    DECLARE_INT_HANDLER     KBI1_HANDLER13                 /* 60: Interrupt 60 */
    DECLARE_INT_HANDLER     KBI1_HANDLER14                 /* 61: Interrupt 61 */
    DECLARE_INT_HANDLER     KBI1_HANDLER15                 /* 62: Interrupt 62 */
    DECLARE_INT_HANDLER     KBI1_ALL_HANDLER               /* 63: Interrupt 63 */
    DECLARE_INT_HANDLER     KBI0_HANDLER0                  /* 64: Interrupt 64 */
    DECLARE_INT_HANDLER     KBI0_HANDLER1                  /* 65: Interrupt 65 */
    DECLARE_INT_HANDLER     KBI0_HANDLER2                  /* 66: Interrupt 66 */
    DECLARE_INT_HANDLER     KBI0_HANDLER3                  /* 67: Interrupt 67 */
    DECLARE_INT_HANDLER     KBI0_HANDLER4                  /* 68: Interrupt 68 */
    DECLARE_INT_HANDLER     KBI0_HANDLER5                  /* 69: Interrupt 69 */
    DECLARE_INT_HANDLER     KBI0_HANDLER6                  /* 70: Interrupt 70 */
    DECLARE_INT_HANDLER     KBI0_HANDLER7                  /* 71: Interrupt 71 */
    DECLARE_INT_HANDLER     KBI0_HANDLER8                  /* 72: Interrupt 72 */
    DECLARE_INT_HANDLER     KBI0_HANDLER9                  /* 73: Interrupt 73 */
    DECLARE_INT_HANDLER     KBI0_HANDLER10                 /* 74: Interrupt 74 */
    DECLARE_INT_HANDLER     KBI0_HANDLER11                 /* 75: Interrupt 75 */
    DECLARE_INT_HANDLER     KBI0_HANDLER12                 /* 76: Interrupt 76 */
    DECLARE_INT_HANDLER     KBI0_HANDLER13                 /* 77: Interrupt 77 */
    DECLARE_INT_HANDLER     KBI0_HANDLER14                 /* 78: Interrupt 78 */
    DECLARE_INT_HANDLER     KBI0_HANDLER15                 /* 79: Interrupt 79 */
    DECLARE_INT_HANDLER     KBI0_ALL_HANDLER               /* 80: Interrupt 80 */
    DECLARE_INT_HANDLER     WDT0_HANDLER                   /* 81: Interrupt 81 */
    DECLARE_INT_HANDLER     WDT1_HANDLER                   /* 82: Interrupt 82 */
    DECLARE_INT_HANDLER     GDMA11_HANDLER                 /* 83: Interrupt 83 */
    DECLARE_INT_HANDLER     GDMA10_HANDLER                 /* 84: Interrupt 84 */
    DECLARE_INT_HANDLER     GDMA01_HANDLER                 /* 85: Interrupt 85 */
    DECLARE_INT_HANDLER     GDMA00_HANDLER                 /* 86: Interrupt 86 */
    DECLARE_INT_HANDLER     default_intexc_handler         /* 87: Interrupt 87 */
    DECLARE_INT_HANDLER     SIMPTC0_0_HANDLER              /* 88: Interrupt 88 */
    DECLARE_INT_HANDLER     SIMPTC0_1_HANDLER              /* 89: Interrupt 89 */
    DECLARE_INT_HANDLER     SIMPTC0_2_HANDLER              /* 90: Interrupt 90 */
    DECLARE_INT_HANDLER     SIMPTC0_3_HANDLER              /* 91: Interrupt 91 */
    DECLARE_INT_HANDLER     SIMPTC1_0_HANDLER              /* 92: Interrupt 92 */
    DECLARE_INT_HANDLER     SIMPTC1_1_HANDLER              /* 93: Interrupt 93 */
    DECLARE_INT_HANDLER     SIMPTC1_2_HANDLER              /* 94: Interrupt 94 */
    DECLARE_INT_HANDLER     SIMPTC1_3_HANDLER              /* 95: Interrupt 95 */
    DECLARE_INT_HANDLER     SIMPTC2_0_HANDLER              /* 96: Interrupt 96 */
    DECLARE_INT_HANDLER     SIMPTC2_1_HANDLER              /* 97: Interrupt 97 */
    DECLARE_INT_HANDLER     SIMPTC2_2_HANDLER              /* 98: Interrupt 98 */
    DECLARE_INT_HANDLER     SIMPTC2_3_HANDLER              /* 99: Interrupt 98 */
    DECLARE_INT_HANDLER     SIMPTC3_0_HANDLER              /* 100: Interrupt 100 */
    DECLARE_INT_HANDLER     SIMPTC3_1_HANDLER              /* 101: Interrupt 101 */
    DECLARE_INT_HANDLER     SIMPTC3_2_HANDLER              /* 102: Interrupt 102 */
    DECLARE_INT_HANDLER     SIMPTC3_3_HANDLER              /* 103: Interrupt 103 */
    DECLARE_INT_HANDLER     TC0_HANDLER                    /* 104: Interrupt 104 */
    DECLARE_INT_HANDLER     TC1_HANDLER                    /* 105: Interrupt 105 */
    DECLARE_INT_HANDLER     ISO78160_HANDLER               /* 106: Interrupt 106 */
    DECLARE_INT_HANDLER     ISO78161_HANDLER               /* 107: Interrupt 107 */
    DECLARE_INT_HANDLER     I2C0_HANDLER                   /* 108: Interrupt 108 */
    DECLARE_INT_HANDLER     I2C1_HANDLER                   /* 109: Interrupt 109 */
    DECLARE_INT_HANDLER     LPUART_HANDLER                 /* 110: Interrupt 110 */
    DECLARE_INT_HANDLER     LSRX_HANDLER                   /* 111: Interrupt 111 */
    DECLARE_INT_HANDLER     UART0_HANDLER                  /* 112: Interrupt 112 */
    DECLARE_INT_HANDLER     UART1_HANDLER                  /* 113: Interrupt 113 */
    DECLARE_INT_HANDLER     UART2_HANDLER                  /* 114: Interrupt 114 */
    DECLARE_INT_HANDLER     UART3_HANDLER                  /* 115: Interrupt 115 */
    DECLARE_INT_HANDLER     UART4_HANDLER                  /* 116: Interrupt 116 */
    DECLARE_INT_HANDLER     UART5_HANDLER                  /* 117: Interrupt 117 */
    DECLARE_INT_HANDLER     UART6_HANDLER                  /* 118: Interrupt 118 */
    DECLARE_INT_HANDLER     UART7_HANDLER                  /* 119: Interrupt 119 */
    DECLARE_INT_HANDLER     USART0_HANDLER                 /* 120: Interrupt 120 */
    DECLARE_INT_HANDLER     USART1_HANDLER                 /* 121: Interrupt 121 */
    DECLARE_INT_HANDLER     default_intexc_handler         /* 122: Interrupt 122 */
    DECLARE_INT_HANDLER     SPI0_HANDLER                   /* 123: Interrupt 123 */
    DECLARE_INT_HANDLER     SPI1_HANDLER                   /* 124: Interrupt 124 */
    DECLARE_INT_HANDLER     SPI2_HANDLER                   /* 125: Interrupt 125 */
    DECLARE_INT_HANDLER     SPI3_HANDLER                   /* 126: Interrupt 126 */
    DECLARE_INT_HANDLER     QSPI0_HANDLER                  /* 127: Interrupt 127 */
    DECLARE_INT_HANDLER     QSPI1_HANDLER                  /* 128: Interrupt 128 */
    DECLARE_INT_HANDLER     DSP_HANDLER                    /* 129: Interrupt 129 */
    DECLARE_INT_HANDLER     M2M_HANDLER                    /* 130: Interrupt 130 */
    DECLARE_INT_HANDLER     AES_HANDLER                    /* 131: Interrupt 131 */
    DECLARE_INT_HANDLER     SEA_HANDLER                    /* 132: Interrupt 132 */
    DECLARE_INT_HANDLER     KEYSTORE_HANDLER               /* 133: Interrupt 133 */
    DECLARE_INT_HANDLER     DAC_HANDLER                    /* 134: Interrupt 134 */
    DECLARE_INT_HANDLER     HSADC_HANDLER                  /* 135: Interrupt 135 */
    DECLARE_INT_HANDLER     D2F_HANDLER                    /* 136: Interrupt 136 */
    DECLARE_INT_HANDLER     RTC_HANDLER                    /* 137: Interrupt 137 */
    DECLARE_INT_HANDLER     VBAT_HANDLER                   /* 138: Interrupt 138 */
    DECLARE_INT_HANDLER     LVDCMP_HANDLER                 /* 139: Interrupt 139 */
    DECLARE_INT_HANDLER     LCSS_HANDLER                   /* 140: Interrupt 140 */
    DECLARE_INT_HANDLER     SCU_HANDLER                    /* 141: Interrupt 141 */
    DECLARE_INT_HANDLER     CORE1_RST_HANDLER              /* 142: Interrupt 142 */
    DECLARE_INT_HANDLER     CORE1_SYSREG_HANDLER           /* 143: Interrupt 143 */
    DECLARE_INT_HANDLER     CORE0_SYSREG_HANDLER           /* 144: Interrupt 144 */
    DECLARE_INT_HANDLER     MAILBOX_INTERNAL_HANDLER       /* 145: Interrupt 145 */
    DECLARE_INT_HANDLER     MAILBOX_HOST_HANDLER           /* 146: Interrupt 146 */

    .section .text.init

    .globl _start
    .type _start, @function

/**
 * Reset Handler called on controller reset
 */
_start:
    /* ===== Startup Stage 1 ===== */
    /* Disable Global Interrupt */
    csrc CSR_MSTATUS, MSTATUS_MIE

    /* Initialize GP, TP and SP */
    .option push
    .option norelax
    la gp, __global_pointer$
    la tp, __tls_base
    .option pop
    la sp, _sp

    /*
     * Intialize ECLIC vector interrupt
     * base address mtvt to vector_base
     */
    la t0, vector_base
    csrw CSR_MTVT, t0

    /*
     * Set ECLIC non-vector entry to be controlled
     * by mtvt2 CSR register.
     * Intialize ECLIC non-vector interrupt
     * base address mtvt2 to irq_entry.
     */
    la t0, irq_entry
    csrw CSR_MTVT2, t0
    csrs CSR_MTVT2, 0x1

    /*
     * Set Exception Entry MTVEC to early_exc_entry
     * Due to settings above, Exception and NMI
     * will share common entry.
     * This early_exc_entry is only used during early
     * boot stage before main
     */
    la t0, early_exc_entry
    csrw CSR_MTVEC, t0

    /* Set the interrupt processing mode to ECLIC mode */
    li t0, 0x3f
    csrc CSR_MTVEC, t0
    csrs CSR_MTVEC, 0x3

    /* ===== Startup Stage 2 ===== */

    /* Enable FPU and Vector Unit if f/d/v exist in march */
#if defined(__riscv_flen) && __riscv_flen > 0
    /* Enable FPU, and set state to initial */
    li t0, MSTATUS_FS
    csrc mstatus, t0
    li t0, MSTATUS_FS_INITIAL
    csrs mstatus, t0
#endif

    /* Enable mcycle and minstret counter */
    csrci CSR_MCOUNTINHIBIT, 0x5

    /* ===== Startup Stage 3 ===== */
    /*
     * Load text section from CODE ROM to CODE RAM
     * when text LMA is different with VMA
     */
    la a0, _text_lma
    la a1, _text
    /* If text LMA and VMA are equal
     * then no need to copy text section */
    beq a0, a1, 2f
    la a2, _etext
    bgeu a1, a2, 2f

1:
    /* Load code section if necessary */
    lw t0, (a0)
    sw t0, (a1)
    addi a0, a0, 4
    addi a1, a1, 4
    bltu a1, a2, 1b
2:
    /* Load data section */
    la a0, _data_lma
    la a1, _data
    /* If data vma=lma, no need to copy */
    beq a0, a1, 2f
    la a2, _edata
    bgeu a1, a2, 2f
1:
    lw t0, (a0)
    sw t0, (a1)
    addi a0, a0, 4
    addi a1, a1, 4
    bltu a1, a2, 1b
2:
    /* Clear bss section */
    la a0, __bss_start
    la a1, _end
    bgeu a0, a1, 2f
1:
    sw zero, (a0)
    addi a0, a0, 4
    bltu a0, a1, 1b
2:

    /*
     * Call vendor defined SystemInit to
     * initialize the micro-controller system
     * SystemInit will just be called by boot cpu
     */
    call SystemInit

    /* Call global constructors */
    //la a0, __libc_fini_array
    //call atexit
    /* Call C/C++ constructor start up code */
    //call __libc_init_array

    /* do pre-init steps before main */
    call _premain_init

    /*
     * When all initialization steps done
     * set exception entry to correct exception
     * entry and jump to main.
     * And set the interrupt processing mode to
     * ECLIC mode
     */
    la t0, exc_entry
    csrw CSR_MTVEC, t0
    li t0, 0x3f
    csrc CSR_MTVEC, t0
    csrs CSR_MTVEC, 0x3

    /* ===== Call SMP Main Function  ===== */
    /* argc = argv = 0 */
    li a0, 0
    li a1, 0
#ifdef RTOS_RTTHREAD
    call entry
#else
    call main
#endif
    /* do post-main steps after main
     * this function will be called by each cpu */
    call _postmain_fini

1:
    j 1b

/* Early boot exception entry before main */
.align 6
.global early_exc_entry
.type early_exc_entry, @function
early_exc_entry:
    wfi
    j early_exc_entry
