/*
 * Copyright (c) 2025 Renergy Technology
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/riscv/common/linker.ld>

/* Renergy RN8xxx 特有的内存保护或共享区域定义 */
SECTIONS
{
    /* 可以在这里添加双核共享内存段 (Shared RAM) */
    .shared_ram (NOLOAD) :
    {
        . = ALIGN(4);
        _shared_ram_start = .;
        KEEP(*(.shared_ram))
        _shared_ram_end = .;
    } > RAM
}
