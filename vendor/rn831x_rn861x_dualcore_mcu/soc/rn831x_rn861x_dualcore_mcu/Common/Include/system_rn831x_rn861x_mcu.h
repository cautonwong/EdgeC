/**
 ******************************************************************************
 * @file   system_rn831x_rn861x_mcu.h
 * @author Renergy Software Team
 * @brief  NMSIS Nuclei N/NX Device Peripheral Access Layer Header File.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2009-2018 Arm Limited. All rights reserved.
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

#ifndef __SYSTEM_RN831X_RN861X_MCU_H__
#define __SYSTEM_RN831X_RN861X_MCU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern volatile uint32_t SystemCoreClock; /*!< System Clock Frequency (Core Clock) */

/**
 * \brief Dump Exception Frame
 */
void Exception_DumpFrame(unsigned long sp, uint8_t mode);

/**
 * \brief Initialize eclic config
 */
extern void ECLIC_Init(void);

/**
 * \brief  Initialize a specific IRQ and register the handler
 * \details
 * This function set vector mode, trigger mode and polarity, interrupt level and priority,
 * assign handler for specific IRQn.
 */
extern int32_t ECLIC_Register_IRQ(IRQn_Type IRQn, uint8_t shv, ECLIC_TRIGGER_Type trig_mode, uint8_t lvl, uint8_t priority, void* handler);
extern int32_t ECLIC_Configure_IRQ(IRQn_Type IRQn, uint8_t shv, ECLIC_TRIGGER_Type trig_mode, uint8_t lvl, uint8_t priority);
extern void Exception_Register_EXC(uint32_t EXCn, unsigned long exc_handler);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_RN831X_RN861X_MCU_H__ */
