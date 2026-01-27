/********************************************************************************
  * @file    Rn8xxx_SMTC.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-08-29
  * @brief   Timer drive
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
                     ##### timer peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]


    @endverbatim
  */

/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll.h"
#ifdef LL_SIMPTC_MODULE_ENABLED

/* Private define ------------------------------------------------------------*/

/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/
#if defined(RN821x_RN721x_SOC_V2)||defined(RN821x_RN721x_SOC_V3) \
  ||defined(RN202x_RN7326_SOC_V2)
#define REG_LOAD_MAXVALUE 0xFFFFFFFF
#elif defined(RN831x_RN861x_MCU_V2)
#define REG_LOAD_MAXVALUE 0xFFFF
#elif defined(RN831x_RN861x_MCU_V3)
#define REG_LOAD_MAXVALUE 0xFFFF
#define REG_LOAD_MAXVALUE1 0xFFFFFFFF
#elif defined(RN831x_RN861x_MCU_V4)
#define REG_LOAD_MAXVALUE 0xFFFFFFFF
#define REG_LOAD_MAXVALUE1 0xFFFFFFFF
#endif
/*********************************************************************************************************
** Function name:       LL_SIMPTC_Init
** Descriptions:        简单定时器初始化
** input parameters:    Timer：定时器模块
                        sSIMPTC_Init: 定时器配置
** output parameters:   None
** Returned value:      ERN_SUCCESS配置成功，ERN_ERROR配置失败
*********************************************************************************************************/
#if defined(RN831x_RN861x_MCU_V4)
ErrorStatus LL_SIMPTC_Init(SIMPTC_TypeDef *SIMP_TCx, const sLL_SIMPTC_InitTypeDef *sSIMPTC_Init)
#else
ErrorStatus LL_SIMPTC_Init(SIMP_TC_TypeDef *SIMP_TCx, const sLL_SIMPTC_InitTypeDef *sSIMPTC_Init)
#endif
{
    uint64_t loadTempU64;
    uint32_t loadTempU32;
    if (NULL == SIMP_TCx)
    {
      return ERN_ERROR;
    }
    loadTempU64 = (((uint64_t)((sSIMPTC_Init->mS * 1000) + sSIMPTC_Init->uS) * SystemCoreClock) / 1000000) - 1;

#if !defined(RN831x_RN861x_MCU_V3) || !defined(RN831x_RN861x_MCU_V4)
    if (loadTempU64 > REG_LOAD_MAXVALUE)
    {
      return ERN_ERROR;
    }
    loadTempU32 = (uint32_t)(loadTempU64 & REG_LOAD_MAXVALUE);
#else
    if(SIMP_TC4 == SIMP_TCx)
    {
      if (loadTempU64 > REG_LOAD_MAXVALUE1)
      {
        return ERN_ERROR;
      }
      loadTempU32 = (uint32_t)(loadTempU64 & REG_LOAD_MAXVALUE1);
    }
    else
    {
      if (loadTempU64 > REG_LOAD_MAXVALUE)
      {
        return ERN_ERROR;
      }
      loadTempU32 = (uint32_t)(loadTempU64 & REG_LOAD_MAXVALUE);
    }
#endif
    /* Set the simple timer configuration */
    SET_REG_SIMPTC_CTRL(SIMP_TCx, sSIMPTC_Init->Ctrl.SimpTcCtrl);
    SET_REG_SIMPTC_LOAD(SIMP_TCx, loadTempU32);
    return ERN_SUCCESS;
}

/*********************************************************************************************************
** Function name:       LL_SIMPTC_Disable
** Descriptions:        关闭简单定时器模块
** input parameters:    Timer：定时器模块
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
#if defined(RN831x_RN861x_MCU_V4)
void LL_SIMPTC_Disable(SIMPTC_TypeDef *SIMP_TCx)
#else
void LL_SIMPTC_Disable(SIMP_TC_TypeDef *SIMP_TCx)
#endif
{
  SET_REG_SIMPTC_CTRL(SIMP_TCx, 0x00);
}
#endif /*LL_SIMPTC_MODULE_ENABLED*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
