/**
 ******************************************************************************
 * @file   privilege.h
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

#ifndef APPLICATION_PRIVILEGE_H_
#define APPLICATION_PRIVILEGE_H_

#include "renergy_sdk_mcu.h"

typedef enum {
    PRIVILEGE_USER_MODE         = 0U,
    PRIVILEGE_SUPERVISOR_MODE   = 1U,
    PRIVILEGE_HYPERVISOR_MODE   = 2U,
    PRIVILEGE_MACHINE_MODE      = 3U,
} riscv_privilege_mode;

#if __RISCV_XLEN == 32
typedef uint32_t metal_xreg_t;
#elif __RISCV_XLEN == 64
typedef uint64_t metal_xreg_t;
#endif

struct metal_register_file {
    metal_xreg_t ra;
    metal_xreg_t sp;
};

typedef void (*metal_privilege_entry_point_t)(void);

void metal_privilege_drop_from_M_to_mode(riscv_privilege_mode mode, struct metal_register_file regfile, metal_privilege_entry_point_t entry_point);
void metal_privilege_drop_from_S_to_U(struct metal_register_file regfile, metal_privilege_entry_point_t entry_point);
void metal_privilege_update_from_U_to_M(struct metal_register_file regfile, metal_privilege_entry_point_t entry_point);

#endif /* APPLICATION_PRIVILEGE_H_ */
