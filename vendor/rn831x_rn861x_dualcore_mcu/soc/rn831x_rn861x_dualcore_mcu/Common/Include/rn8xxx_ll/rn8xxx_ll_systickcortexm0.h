/******************************************************************************
 * @file    rn8xxx_systickcortexm0.h
 * @author  Renergy Application Team
 * @version V1.1.0
 * @date    2023-08-29
 * @brief   Header file containing functions prototypes of SysTickCortexM0 HAL library.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2016 Renergy.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 *
 ******************************************************************************
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SysTickCortexM0_H_
#define SysTickCortexM0_H_

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll_def.h"
/* Exported define ------------------------------------------------------------*/

/* Exported enum ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
extern void Rn8xxx_SysTick_Init(uint32_t tUnit_ms);
extern void Rn8xxx_SysTick_Stop(void);
#endif

/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
