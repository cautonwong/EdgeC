/*******************************************************************************
  * @file    Rn8xxx_SysTickCortexM0.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-08-29
  * @brief   SysTickCortexM0 driver
  *
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
  ******************************************************************************
  *@verbatim
  ==============================================================================
                     ##### SysTickCortexM0 peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]


    @endverbatim
  */

/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll.h"
#ifdef LL_SYSTICK_MODULE_ENABLED
// #include <Drive_Lib.h>

/* Private define ------------------------------------------------------------*/

/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/

/*********************************************************************************************************
** Function name:       fnRn8xxx_TC_TimeInit
** Descriptions:        定时器初始化
** input parameters:    tUnit_ms:定时器单位ms
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void Rn8xxx_SysTick_Init(uint32_t tUnit_ms)
{
    uint32_t ticks;

    ticks = SystemCoreClock * tUnit_ms / 1000;
    (void)SysTick_Config(ticks);
}

/*********************************************************************************************************
** Function name:       fnRn8xxx_TC_TimeStop
** Descriptions:        间隔定时器关闭
** input parameters:    None
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void Rn8xxx_SysTick_Stop(void)
{
    SysTick->CTRL = 0;
}
#endif /*LL_SYSTICK_MODULE_ENABLED*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
