/********************************************************************************
  * @file    rn8xxx_ll_m2m.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-08-29
  * @brief   M2M driver
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
                     ##### M2M peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]


    @endverbatim
  */

#include "rn8xxx_ll.h"
#ifdef LL_M2M_MODULE_ENABLED
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V4)
/*********************************************************************************************************
** Function name:          LL_M2M_Move
** Descriptions:           M2M 数据搬运
** input parameters:      sMem_Init:搬运参数配置
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_M2M_Move(const sll_M2M_InitTypeDef *sMem_Init)
{
#if defined(RN831x_RN861x_MCU_V2)
    if( ((sMem_Init->SAddr % 4) != 0) || ((sMem_Init->DAddr % 4) != 0))
    {
      return ERN_ERROR;
    }
#endif

    if(sMem_Init->TxLen == 0)
    {
      return ERN_ERROR;
    }

#if !defined(RN831x_RN861x_MCU_V4)
    M2M->CTL = 0; // 停止上一次搬运;
#else
    M2M->CTRL = 0; // 停止上一次搬运;
#endif
    M2M->MODE = sMem_Init->Ctrl.M2MMode;
    M2M->DUMMY	= 0x00;

    if(0 != sMem_Init->Ctrl.bitM2MMode.DUMMY_EN)
    {
      M2M->DUMMY	= sMem_Init->DUMMY;
    }

    M2M->SADDR = (sMem_Init->SAddr & 0x0001FFFF); // 数据源地址
    M2M->DADDR = (sMem_Init->DAddr & 0x0001FFFF); // 目标地址

    M2M->ILEN = sMem_Init->TxLen;
#if !defined(RN831x_RN861x_MCU_V4)
    M2M->CTL = 1;     // 启动搬运
#else
    M2M->CTRL = 1;     // 启动搬运
#endif

    if (fnRegState_Detect((uint32_t)(&M2M->IF), 0, 1, M2M_TIMEOUT) == ERN_ERROR)
        return (ERN_ERROR); // 等待转换完成

    M2M->IF = 0x01; // clear If

    return ERN_SUCCESS;
}
#endif
#endif /*LL_M2M_MODULE_ENABLED*/
//------------------------------------------------------------------------------
