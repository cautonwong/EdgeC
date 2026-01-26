/**
 ******************************************************************************
 * @file   rn831x_rn861x_mcu_core0_it.h
 * @author Renergy Software Team
 * @brief  This file contains the headers of the interrupt handlers.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RN831X_RN861X_MCU_CORE0_IT_H
#define __RN831X_RN861X_MCU_CORE0_IT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Private includes ---------------------------------------------------------*/
#include "rn831x_rn861x_mcu.h"

/* Exported types -----------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/
/* Exported macro -----------------------------------------------------------*/
/* Exported functions prototypes --------------------------------------------*/
__INTERRUPT void UART0_HANDLER(void);

#ifdef __cplusplus
}
#endif

#endif /* __RN831X_RN861X_MCU_CORE0_IT_H */
