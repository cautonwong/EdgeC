/********************************************************************************
  * @file    Rn8xxx_Common.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-08-29
  * @brief   common file for driver
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 Renergy.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  *@verbatim
  ==============================================================================
           ##### Features #####
  ==============================================================================
  [..]

           ##### How to use this driver #####
  ==============================================================================
  [..]


  @endverbatim
  */

/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll.h"
/*#include "rn8xxx_ll_utils.h"*/
/* Private define ------------------------------------------------------------*/

/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/

void SystemDelayUs(uint32_t i)
{
#ifdef RN831x_RN861x_MCU_V4
#ifdef CORE0
    if (((SYSTEM->SYSFREQ_STA >> 0) & 0x7u) != 0x3u)
#endif
#else
    if ((((unsigned int)(SYSCTL->OSC_CTRL1) >> 8) & 0x7U) != 0x3U)
#endif
    {
        for (; i > 0U; i--)
        {
            __NOP();
            __NOP();
            __NOP();
            __NOP();
            __NOP();
            __NOP();
            __NOP();
            __NOP();
        }
    }
    else
    {
        for (; i > 0U; i--)
        {
            __NOP();
            __NOP();
        }
    }
}
/*********************************************************************************************************
** Function name:     	fnState_Detect
** Descriptions:       	wait while the position of value in addr is not the same as sign
** input parameters:    addr:the address which need to be detected;
                        position:the position of the value needed to be detected sign: the valid state��timeout:timeout time us
** output parameters:   None
** Returned value:      true:valid state  false: state is invalid befor the function return
*********************************************************************************************************/
ErrorStatus fnRegState_Detect(uint32_t reg_addr, uint32_t position, uint32_t sign, uint32_t timeout)
{
    uint32_t i, val;

    val = ((unsigned int)1 << position);

    if (timeout == 0U) {
        return ERN_ERROR;
    }
    for (i = 0U; i < timeout; i++)

    {
        if (((sign == 1U) && ((*(uint32_t *)reg_addr & val) == val)) || 
            ((sign == 0U) && ((*(uint32_t *)reg_addr & val) != val)))   
        {
            return ERN_SUCCESS;
        }
        SystemDelayUs(1U);
    }
    return ERN_ERROR;
}

/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
