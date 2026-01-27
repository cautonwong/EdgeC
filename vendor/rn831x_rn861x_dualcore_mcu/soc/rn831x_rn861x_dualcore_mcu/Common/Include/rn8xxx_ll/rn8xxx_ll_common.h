/********************************************************************************
 * @file    rn8xxx_common.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-09-11
 * @brief   Header file containing functions prototypes of common library.
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
#ifndef RN8xxx_Common_H_
#define RN8xxx_Common_H_
#include "rn8xxx_ll_def.h"

extern void SystemDelayUs(uint32_t i);
extern ErrorStatus fnRegState_Detect(uint32_t reg_addr, uint32_t position, uint32_t sign, uint32_t timeout);

#endif /*DATA_TYPES_H_*/
