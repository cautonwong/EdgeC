/******************************************************************************
 * @file        system_demo_drv.h
 * @brief
 * @author      Renergy Technology
 * @note
 *
 * Copyright (C), Renergy Technology Inc. All rights reserved.
 *
 *****************************************************************************/


#ifndef __SYSTEM_DEMO_DRV_H__
#define __SYSTEM_DEMO_DRV_H__

#include "rn_rv_ll_system_core0.h"
#include "rn_rv_hal_system.h"

#define  ASSERT_ERRSET(a, b) \
{ \
    if ((a) != 0) { \
        return (b); \
    } \
}

uint8_t hal_system_control_func(void);

#endif
