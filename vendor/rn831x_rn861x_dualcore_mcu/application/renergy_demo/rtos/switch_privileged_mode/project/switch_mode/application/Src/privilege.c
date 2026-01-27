/**
 ******************************************************************************
 * @file   privilege.c
 * @author Renergy Software Team
 * @brief
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

#include <stdio.h>
#include "privilege.h"

#define METAL_MSTATUS_MIE_OFFSET    3
#define METAL_MSTATUS_MPIE_OFFSET   7
#define METAL_MSTATUS_SIE_OFFSET    1
#define METAL_MSTATUS_SPIE_OFFSET   5

#define METAL_MSTATUS_MPP_OFFSET    11
#define METAL_MSTATUS_MPP_MASK      3

void metal_privilege_drop_from_M_to_mode(riscv_privilege_mode mode, struct metal_register_file regfile, metal_privilege_entry_point_t entry_point)
{
    register rv_csr_t mstatus;
    mstatus = __RV_CSR_READ(CSR_MSTATUS);

    /* Set xPIE bits based on current xIE bits */
    if (mstatus & (1 << METAL_MSTATUS_MIE_OFFSET)) {
        mstatus |= (1 << METAL_MSTATUS_MPIE_OFFSET);
    } else {
        mstatus &= ~(1 << METAL_MSTATUS_MPIE_OFFSET);
    }
    if (mstatus & (1 << METAL_MSTATUS_SIE_OFFSET)) {
        mstatus |= (1 << METAL_MSTATUS_SPIE_OFFSET);
    } else {
        mstatus &= ~(1 << METAL_MSTATUS_SPIE_OFFSET);
    }

    /* Set MPP to the requested privilege mode */
    mstatus &= ~(METAL_MSTATUS_MPP_MASK << METAL_MSTATUS_MPP_OFFSET);
    mstatus |= (mode << METAL_MSTATUS_MPP_OFFSET);
    //__asm__ volatile("csrw mstatus, %0" ::"r"(mstatus));
    __RV_CSR_WRITE(CSR_MSTATUS, mstatus);

    /* Set the entry point in MEPC */
    //__asm__ volatile("csrw mepc, %0" ::"r"(entry_point));
    __RV_CSR_WRITE(CSR_MEPC, entry_point);

    /* Set the register file */
    __asm__ volatile("mv ra, %0" ::"r"(regfile.ra));
    __asm__ volatile("mv sp, %0" ::"r"(regfile.sp));
    __asm__ volatile("mret");
}

void metal_privilege_update_from_U_to_M(struct metal_register_file regfile, metal_privilege_entry_point_t entry_point)
{
    /* Set MPP to the requested privilege mode */
    __RV_CSR_READ_SET(CSR_MSTATUS, 0x1800);

    /* Set the entry point in MEPC */
    __RV_CSR_WRITE(CSR_MEPC, entry_point);

    /* Set the register file */
    __asm__ volatile("mv ra, %0" ::"r"(regfile.ra));
    __asm__ volatile("mv sp, %0" ::"r"(regfile.sp));
    __asm__ volatile("mret");
}

#define METAL_SSTATUS_SIE_OFFSET    1
#define METAL_SSTATUS_SPIE_OFFSET   5
#define METAL_SSTATUS_SPP_OFFSET    8

void metal_privilege_drop_from_S_to_U(struct metal_register_file regfile, metal_privilege_entry_point_t entry_point)
{
    register rv_csr_t sstatus;
    sstatus = __RV_CSR_READ(CSR_SSTATUS);

    /* Set SPIE bits based on current SIE bits */
    if (sstatus & (1 << METAL_SSTATUS_SIE_OFFSET)) {
        sstatus |= (1 << METAL_SSTATUS_SPIE_OFFSET);
    } else {
        sstatus &= ~(1 << METAL_SSTATUS_SPIE_OFFSET);
    }

    /* Set SPP to 0 to sret to user mode */
    sstatus &= ~(1 << METAL_SSTATUS_SPP_OFFSET);
    __RV_CSR_WRITE(CSR_SSTATUS, sstatus);

    /* Set the entry point in MEPC */
    //__asm__ volatile("csrw mepc, %0" ::"r"(entry_point));
    __RV_CSR_WRITE(CSR_SEPC, entry_point);

    /* Set the register file */
    __asm__ volatile("mv ra, %0" ::"r"(regfile.ra));
    __asm__ volatile("mv sp, %0" ::"r"(regfile.sp));
    __asm__ volatile("sret");
}
