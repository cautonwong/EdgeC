/*******************************************************************************
  * @file    rn8xxx_ll_iocnt.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-12-27
  * @brief   iocnt drive
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

  [..]

           ##### How to use this driver #####
  ==============================================================================
  [..]


  @endverbatim
  */

/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll.h"
#ifdef LL_IOCNT_MODULE_ENABLED
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) ||\
	defined(RN831x_RN861x_MCU_V4)
/* Private define ------------------------------------------------------------*/

/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/
#if defined(RN821x_RN721x_SOC_V2)
/*********************************************************************************************************
** Function name:     LL_IOCNT_IOCNTxInit
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		null
*********************************************************************************************************/
ErrorStatus LL_IOCNT_IOCNTxInit(sLL_IOCNT_InitTypeDef *sIOCNT_Init , eIOCNT_ID_TypeDef IOCNT_id)
{
    ErrorStatus res = ERN_SUCCESS;
    uIOCNT_CTL_TypeDef Ctrl;

    Ctrl.CTL = GET_REG_IOCNT_CTL(IOCNT , 31); /* 获取原控制寄存器 */

    Ctrl.bitCTL.CNT_CLR = sIOCNT_Init->Ctrl_CntClr;  /* 脉冲数类型配置，所有通道共用 */

    if( (sIOCNT_Init->Ctrl_FLTCfg == 0)||(sIOCNT_Init->Ctrl_FLTCfg > 128) )/*滤波周期数配置，所有通道共用*/
      Ctrl.bitCTL.FLT_CFG = 0x1F; /* 滤波周期数非法时配置为默认值 */
    else
      Ctrl.bitCTL.FLT_CFG = sIOCNT_Init->Ctrl_FLTCfg;

    switch (IOCNT_id)
    {
    case INTC_ID0:

      if(GET_BIT_IOCNT_CTL(IOCNT , BIT0)) /* 模块使能情况下，不允许修改寄存器 */
        res = ERN_ERROR;
      else
      {
        SET_REG_IOCNT_CFG0(IOCNT , sIOCNT_Init->CFGx.IOCNTCFG);
        IOCNT->CHNL&= 0xFFFFFFC0;
        IOCNT->CHNL|= sIOCNT_Init->CHNLx.CHNL;

        Ctrl.bitCTL.IOCNT0_EN = sIOCNT_Init->Ctrl_EN;
        Ctrl.bitCTL.IOCNT0_MODE = sIOCNT_Init->Ctrl_Mode;
        Ctrl.bitCTL.FLT_BYPASS0 = sIOCNT_Init->Ctrl_FLTEN;

        SET_REG_IOCNT_CTL(IOCNT , Ctrl.CTL);
      }
      break;

    case INTC_ID1:
      if(GET_BIT_IOCNT_CTL(IOCNT , BIT1)) /* 模块使能情况下，不允许修改寄存器 */
        res = ERN_ERROR;
      else
      {
        SET_REG_IOCNT_CFG1(IOCNT , sIOCNT_Init->CFGx.IOCNTCFG);

        IOCNT->CHNL&= 0xFFFFF03F;
        IOCNT->CHNL|= sIOCNT_Init->CHNLx.CHNL;

        Ctrl.bitCTL.IOCNT1_EN = sIOCNT_Init->Ctrl_EN;
        Ctrl.bitCTL.IOCNT1_MODE = sIOCNT_Init->Ctrl_Mode;
        Ctrl.bitCTL.FLT_BYPASS1 = sIOCNT_Init->Ctrl_FLTEN;

        SET_REG_IOCNT_CTL(IOCNT , Ctrl.CTL);
      }
      break;

    case INTC_ID2:
      if(GET_BIT_IOCNT_CTL(IOCNT , BIT2)) /* 模块使能情况下，不允许修改寄存器 */
        res = ERN_ERROR;
      else
      {
        SET_REG_IOCNT_CFG2(IOCNT , sIOCNT_Init->CFGx.IOCNTCFG);

        IOCNT->CHNL&= 0xFFFC0FFF;
        IOCNT->CHNL|= sIOCNT_Init->CHNLx.CHNL;

        Ctrl.bitCTL.IOCNT2_EN = sIOCNT_Init->Ctrl_EN;
        Ctrl.bitCTL.IOCNT2_MODE = sIOCNT_Init->Ctrl_Mode;
        Ctrl.bitCTL.FLT_BYPASS2 = sIOCNT_Init->Ctrl_FLTEN;

        SET_REG_IOCNT_CTL(IOCNT , Ctrl.CTL);
      }
      break;
    case INTC_ID3:
      if(GET_BIT_IOCNT_CTL(IOCNT , BIT3)) /* 模块使能情况下，不允许修改寄存器 */
        res = ERN_ERROR;
      else
      {
        SET_REG_IOCNT_CFG3(IOCNT , sIOCNT_Init->CFGx.IOCNTCFG);

        IOCNT->CHNL&= 0xFF03FFFF;
        IOCNT->CHNL|= sIOCNT_Init->CHNLx.CHNL;

        Ctrl.bitCTL.IOCNT3_EN = sIOCNT_Init->Ctrl_EN;
        Ctrl.bitCTL.IOCNT3_MODE = sIOCNT_Init->Ctrl_Mode;
        Ctrl.bitCTL.FLT_BYPASS3 = sIOCNT_Init->Ctrl_FLTEN;

        SET_REG_IOCNT_CTL(IOCNT , Ctrl.CTL);
      }
      break;

    case INTC_ID4:
      if(GET_BIT_IOCNT_CTL(IOCNT , BIT4)) /* 模块使能情况下，不允许修改寄存器 */
        res = ERN_ERROR;
      else
      {
        SET_REG_IOCNT_CFG4(IOCNT , sIOCNT_Init->CFGx.IOCNTCFG);

        IOCNT->CHNL&= 0xFFFC0FFF;
        IOCNT->CHNL|= sIOCNT_Init->CHNLx.CHNL;

        Ctrl.bitCTL.IOCNT4_EN = sIOCNT_Init->Ctrl_EN;
        Ctrl.bitCTL.IOCNT4_MODE = sIOCNT_Init->Ctrl_Mode;
        Ctrl.bitCTL.FLT_BYPASS4 = sIOCNT_Init->Ctrl_FLTEN;

        SET_REG_IOCNT_CTL(IOCNT , Ctrl.CTL);
      }
      break;

    default:
      res = ERN_ERROR;
      break;
    }

    return res;
}
/*********************************************************************************************************
** Function name:       LL_IOCNT_Disable
** Descriptions:        外部中断模块
** input parameters:    INTC_Id
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_IOCNT_Disable(eIOCNT_ID_TypeDef IoCnt_id)
{
    uint32_t eId=0;

    eId = (uint32_t)IoCnt_id;
    SET_BIT_IOCNT_CTL(IOCNT , (BIT0<<eId));
}
#else
#if defined(RN831x_RN861x_MCU_V2)
/*********************************************************************************************************
** Function name:     LL_IOCNT_IOCNTxInit
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		null
*********************************************************************************************************/
ErrorStatus LL_IOCNT_IOCNTxInit(sLL_IOCNT_InitTypeDef *sIOCNT_Init , eIOCNT_ID_TypeDef IOCNT_id)
{
    ErrorStatus res = ERN_SUCCESS;
    uIOCNT_CTL_TypeDef Ctrl;

    Ctrl.CTL = GET_REG_IOCNT_CTL(IOCNT , 16); /* 获取原控制寄存器 */

    Ctrl.bitCTL.CNT_CLR = sIOCNT_Init->Ctrl_CntClr;  /* 脉冲数类型配置，所有通道共用 */

    switch (IOCNT_id)
    {
    case INTC_ID0:
      if(GET_BIT_IOCNT_CTL(IOCNT , BIT0)) /* 模块使能情况下，不允许修改寄存器 */
        res = ERN_ERROR;
      else
      {
        SET_REG_IOCNT_CFG0(IOCNT , sIOCNT_Init->CFGx.IOCNTCFG);
        Ctrl.bitCTL.IOCNT0_EN = sIOCNT_Init->Ctrl_EN;
        Ctrl.bitCTL.IOCNT0_MODE = sIOCNT_Init->Ctrl_Mode;

        SET_REG_IOCNT_CTL(IOCNT , Ctrl.CTL);
      }
      break;

    case INTC_ID1:
      if(GET_BIT_IOCNT_CTL(IOCNT , BIT1)) /* 模块使能情况下，不允许修改寄存器 */
        res = ERN_ERROR;
      else
      {
        SET_REG_IOCNT_CFG1(IOCNT , sIOCNT_Init->CFGx.IOCNTCFG);
        Ctrl.bitCTL.IOCNT1_EN = sIOCNT_Init->Ctrl_EN;
        Ctrl.bitCTL.IOCNT1_MODE = sIOCNT_Init->Ctrl_Mode;

        SET_REG_IOCNT_CTL(IOCNT , Ctrl.CTL);
      }
      break;

    case INTC_ID2:
      if(GET_BIT_IOCNT_CTL(IOCNT , BIT2)) /* 模块使能情况下，不允许修改寄存器 */
        res = ERN_ERROR;
      else
      {
        SET_REG_IOCNT_CFG2(IOCNT , sIOCNT_Init->CFGx.IOCNTCFG);

        Ctrl.bitCTL.IOCNT2_EN = sIOCNT_Init->Ctrl_EN;
        Ctrl.bitCTL.IOCNT2_MODE = sIOCNT_Init->Ctrl_Mode;

        SET_REG_IOCNT_CTL(IOCNT , Ctrl.CTL);
      }
      break;

    default:
      res = ERN_ERROR;
      break;
    }

    return res;
}
/*********************************************************************************************************
** Function name:       LL_IOCNT_Disable
** Descriptions:        外部中断模块
** input parameters:    INTC_Id
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_IOCNT_Disable(eIOCNT_ID_TypeDef IOCNT_id)
{
    uint32_t eId=0;

    eId = (uint32_t)IOCNT_id;
    SET_BIT_IOCNT_CTL(IOCNT , (BIT0<<eId));

}
#else
#if defined(RN831x_RN861x_MCU_V4)
/*********************************************************************************************************
** Function name:     LL_IOCNT_IOCNTxInit
** Descriptions:
** input parameters:
** output parameters: null
** Returned value: 		null
*********************************************************************************************************/
ErrorStatus LL_IOCNT_IOCNTxInit(sLL_IOCNT_InitTypeDef *sIOCNT_Init , eIOCNT_ID_TypeDef IOCNT_id)
{
    ErrorStatus res = ERN_SUCCESS;
    uIOCNT_CTL_TypeDef Ctrl;
    uint32_t copl_bak;

    Ctrl.CTL = GET_REG_IOCNT_CTL(IOCNT , 31); /* 获取原控制寄存器 */
    copl_bak = GET_REG_IOCNT_COPL(IOCNT , 31); /* 获取脉冲输出配置寄存器 */

    Ctrl.bitCTL.CNT_CLR = sIOCNT_Init->Ctrl_CntClr;  /* 脉冲数类型配置，所有通道共用 */

    if( (sIOCNT_Init->Ctrl_FLTCfg == 0)||(sIOCNT_Init->Ctrl_FLTCfg > 128) )/*滤波周期数配置，所有通道共用*/
        Ctrl.bitCTL.FLT_CFG = 0x1F; /* 滤波周期数非法时配置为默认值 */
    else
        Ctrl.bitCTL.FLT_CFG = sIOCNT_Init->Ctrl_FLTCfg;

    switch (IOCNT_id)
    {
        case IOCNT_ID0:
            if(GET_BIT_IOCNT_CTL(IOCNT , BIT0)) /* 模块使能情况下，不允许修改寄存器 */
                res = ERN_ERROR;
            else
            {
                SET_REG_IOCNT_CFG0(IOCNT , sIOCNT_Init->CFGx.IOCNTCFG);
                IOCNT->CHNL &= 0xFFFFFFC0;
                IOCNT->CHNL |= sIOCNT_Init->CHNLx.CHNL;  /* 配置输入通道 */

                SET_REG_IOCNT_COPL(IOCNT, (copl_bak | (sIOCNT_Init->COPL << IOCNT_id))); /* 配置输出脉冲 */

                SET_REG_IOCNT_REMD0(IOCNT, sIOCNT_Init->REMD); /* 配置输入脉冲余数 */

                Ctrl.bitCTL.IOCNT0_EN = sIOCNT_Init->Ctrl_EN;   /* 配置CTRL */
                Ctrl.bitCTL.IOCNT0_MODE = sIOCNT_Init->Ctrl_Mode;
                Ctrl.bitCTL.FLT_BYPASS0 = sIOCNT_Init->Ctrl_FLTEN;
                SET_REG_IOCNT_CTL(IOCNT , Ctrl.CTL);
            }
            break;

        case IOCNT_ID1:
            if(GET_BIT_IOCNT_CTL(IOCNT , BIT1)) /* 模块使能情况下，不允许修改寄存器 */
                res = ERN_ERROR;
            else
            {
                SET_REG_IOCNT_CFG1(IOCNT , sIOCNT_Init->CFGx.IOCNTCFG);

                IOCNT->CHNL &= 0xFFFFF03F;
                IOCNT->CHNL |= sIOCNT_Init->CHNLx.CHNL;

                SET_REG_IOCNT_COPL(IOCNT, (copl_bak | (sIOCNT_Init->COPL << IOCNT_id))); /* 配置输出脉冲 */

                SET_REG_IOCNT_REMD1(IOCNT, sIOCNT_Init->REMD); /* 配置输入脉冲余数 */

                Ctrl.bitCTL.IOCNT1_EN = sIOCNT_Init->Ctrl_EN;
                Ctrl.bitCTL.IOCNT1_MODE = sIOCNT_Init->Ctrl_Mode;
                Ctrl.bitCTL.FLT_BYPASS1 = sIOCNT_Init->Ctrl_FLTEN;
                SET_REG_IOCNT_CTL(IOCNT , Ctrl.CTL);
            }
            break;

        case IOCNT_ID2:
            if(GET_BIT_IOCNT_CTL(IOCNT , BIT2)) /* 模块使能情况下，不允许修改寄存器 */
                res = ERN_ERROR;
            else
            {
                SET_REG_IOCNT_CFG2(IOCNT , sIOCNT_Init->CFGx.IOCNTCFG);

                IOCNT->CHNL&= 0xFFFC0FFF;
                IOCNT->CHNL|= sIOCNT_Init->CHNLx.CHNL;

                SET_REG_IOCNT_COPL(IOCNT, (copl_bak | (sIOCNT_Init->COPL << IOCNT_id))); /* 配置输出脉冲 */

                SET_REG_IOCNT_REMD2(IOCNT, sIOCNT_Init->REMD); /* 配置输入脉冲余数 */

                Ctrl.bitCTL.IOCNT2_EN = sIOCNT_Init->Ctrl_EN;
                Ctrl.bitCTL.IOCNT2_MODE = sIOCNT_Init->Ctrl_Mode;
                Ctrl.bitCTL.FLT_BYPASS2 = sIOCNT_Init->Ctrl_FLTEN;

                SET_REG_IOCNT_CTL(IOCNT , Ctrl.CTL);
            }
            break;

        case IOCNT_ID3:
            if(GET_BIT_IOCNT_CTL(IOCNT , BIT3)) /* 模块使能情况下，不允许修改寄存器 */
                res = ERN_ERROR;
            else
            {
                SET_REG_IOCNT_CFG3(IOCNT , sIOCNT_Init->CFGx.IOCNTCFG);

                IOCNT->CHNL&= 0xFF03FFFF;
                IOCNT->CHNL|= sIOCNT_Init->CHNLx.CHNL;

                SET_REG_IOCNT_COPL(IOCNT, (copl_bak | (sIOCNT_Init->COPL << IOCNT_id))); /* 配置输出脉冲 */

                SET_REG_IOCNT_REMD3(IOCNT, sIOCNT_Init->REMD); /* 配置输入脉冲余数 */

                Ctrl.bitCTL.IOCNT3_EN = sIOCNT_Init->Ctrl_EN;
                Ctrl.bitCTL.IOCNT3_MODE = sIOCNT_Init->Ctrl_Mode;
                Ctrl.bitCTL.FLT_BYPASS3 = sIOCNT_Init->Ctrl_FLTEN;

                SET_REG_IOCNT_CTL(IOCNT , Ctrl.CTL);
            }
            break;

        case IOCNT_ID4:
            if(GET_BIT_IOCNT_CTL(IOCNT , BIT4)) /* 模块使能情况下，不允许修改寄存器 */
                res = ERN_ERROR;
            else
            {
                SET_REG_IOCNT_CFG4(IOCNT , sIOCNT_Init->CFGx.IOCNTCFG);

                IOCNT->CHNL&= 0xFFFC0FFF;
                IOCNT->CHNL|= sIOCNT_Init->CHNLx.CHNL;

                SET_REG_IOCNT_COPL(IOCNT, (copl_bak | (sIOCNT_Init->COPL << IOCNT_id))); /* 配置输出脉冲 */

                SET_REG_IOCNT_REMD4(IOCNT, sIOCNT_Init->REMD); /* 配置输入脉冲余数 */

                Ctrl.bitCTL.IOCNT4_EN = sIOCNT_Init->Ctrl_EN;
                Ctrl.bitCTL.IOCNT4_MODE = sIOCNT_Init->Ctrl_Mode;
                Ctrl.bitCTL.FLT_BYPASS4 = sIOCNT_Init->Ctrl_FLTEN;

                SET_REG_IOCNT_CTL(IOCNT , Ctrl.CTL);
            }
            break;

        default:
            res = ERN_ERROR;
            break;
    }

    return res;
}
/*********************************************************************************************************
** Function name:       LL_IOCNT_Disable
** Descriptions:        IOCNT除能
** input parameters:    INTC_Id
** output parameters:   None
** Returned value:      None
*********************************************************************************************************/
void LL_IOCNT_Disable(eIOCNT_ID_TypeDef IOCNT_id)
{
    uint32_t eId=0;
    eId = (uint32_t)IOCNT_id;
    CLR_BIT_IOCNT_CTL(IOCNT , (BIT0<<eId));
}
#endif
#endif
#endif
#endif
#endif /*LL_IOCNT_MODULE_ENABLED*/
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
