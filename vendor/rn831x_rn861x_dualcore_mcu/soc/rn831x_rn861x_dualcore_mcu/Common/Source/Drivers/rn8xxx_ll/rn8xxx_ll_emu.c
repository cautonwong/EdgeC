/*******************************************************************************
  * @file    Rn8xxx_EMU.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2024-09-03
  * @brief   计量模块
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
                     ##### EMU peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]


    @endverbatim
*/

/* Includes ------------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/
#include "rn8xxx_ll.h"
#ifdef LL_EMU_MODULE_ENABLED

/* Private define ------------------------------------------------------------*/

/* Private enum -------------------------------------------------------------*/
typedef union
{
    uint8_t u8Data[4];
    uint16_t u16Data[2];
    uint32_t u32Data;
} uEmuCommBuf;

/* Private data ---------------------------------------------------------*/
static uEmuCommBuf EmuCommbuf;


    #if defined(RN821x_RN721x_SOC_B) || \
    defined(RN821x_RN721x_SOC_C) || \
    defined(RN821x_RN721x_SOC_D) || \
    defined(RN821x_RN721x_SOC_V2) || \
	defined(RN821x_RN721x_SOC_V3)

    const sEmuCheckConfigReg_TypeDef EmuConfigReg[] =
    {
        /*效验寄存器指针			寄存器默认值*/
        {(uint32_t *)&EMU->EMUCON, 0x0007},
        {(uint32_t *)&EMU->EMUCON2, 0x0000},
        {(uint32_t *)&EMU->HFConst, 0x1000},
        {(uint32_t *)&EMU->PStart, 0x0060},
        {(uint32_t *)&EMU->QStart, 0x0120},
        {(uint32_t *)&EMU->GPQA, 0x0000},
        {(uint32_t *)&EMU->GPQB, 0x0000},
        {(uint32_t *)&EMU->PhsA, 0x0000},
        {(uint32_t *)&EMU->PhsB, 0x0000},
        {(uint32_t *)&EMU->QPhsCal, 0x0000},
        {(uint32_t *)&EMU->APOSA, 0x0000},
        {(uint32_t *)&EMU->APOSB, 0x0000},
        {(uint32_t *)&EMU->RPOSA, 0x0000},
        {(uint32_t *)&EMU->RPOSB, 0x0000},
        {(uint32_t *)&EMU->IARMSOS, 0x0000},
        {(uint32_t *)&EMU->IBRMSOS, 0x0000},
        {(uint32_t *)&EMU->URMSOS, 0x0000},
        {(uint32_t *)&EMU->IAGAIN, 0x0000},
        {(uint32_t *)&EMU->IBGAIN, 0x0000},
        {(uint32_t *)&EMU->UGAIN, 0x0000},
        {(uint32_t *)&EMU->IADCOS, 0x0000},
        {(uint32_t *)&EMU->IBDCOS, 0x0000},
        {(uint32_t *)&EMU->UDCOS, 0x0000},
        {(uint32_t *)&EMU->UADD, 0x0000},
        {(uint32_t *)&EMU->USAG, 0x0000},
        {(uint32_t *)&EMU->IAPEAK, 0x0000},
        {(uint32_t *)&EMU->IBPEAK, 0x0000},
        {(uint32_t *)&EMU->UPEAK, 0x0000},
        {(uint32_t *)&EMU->D2FP, 0x0000},
    };

#endif //defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_D) || defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)

#if defined(RN202x_RN7326_SOC_V2)
    const sEmuCheckConfigReg_TypeDef EmuConfigReg[] =
    {
        /*效验寄存器指针			寄存器默认值*/
        {(uint32_t *)&EMU->HFCONST1, 0x1000},
        {(uint32_t *)&EMU->HFCONST2, 0x1000},
        {(uint32_t *)&EMU->PSTART, 0x0250},
        {(uint32_t *)&EMU->QSTART, 0x0250},
        {(uint32_t *)&EMU->ZXOT, 0x0073},

        {(uint32_t *)&EMU->PRTH1L, 0x0000},
        {(uint32_t *)&EMU->PRTH1H, 0x0000},
        {(uint32_t *)&EMU->PRTH2L, 0x0000},
        {(uint32_t *)&EMU->PRTH2H, 0x0000},

        {(uint32_t *)&EMU->PHSUA, 0x0000},
        {(uint32_t *)&EMU->PHSUB, 0x0000},
        {(uint32_t *)&EMU->PHSUC, 0x0000},
        {(uint32_t *)&EMU->PHSIA, 0x0000},
        {(uint32_t *)&EMU->PHSIB, 0x0000},
        {(uint32_t *)&EMU->PHSIC, 0x0000},

        {(uint32_t *)&EMU->GSUA, 0x0000},
        {(uint32_t *)&EMU->GSUB, 0x0000},
        {(uint32_t *)&EMU->GSUC, 0x0000},
        {(uint32_t *)&EMU->GSIA, 0x0000},
        {(uint32_t *)&EMU->GSIB, 0x0000},
        {(uint32_t *)&EMU->GSIC, 0x0000},

        {(uint32_t *)&EMU->DCOS_IA, 0x0000},
        {(uint32_t *)&EMU->DCOS_IB, 0x0000},
        {(uint32_t *)&EMU->DCOS_IC, 0x0000},

        {(uint32_t *)&EMU->IA_OS, 0x0000},
        {(uint32_t *)&EMU->IB_OS, 0x0000},
        {(uint32_t *)&EMU->IC_OS, 0x0000},

        {(uint32_t *)&EMU->GPA, 0x0000},
        {(uint32_t *)&EMU->GPB, 0x0000},
        {(uint32_t *)&EMU->GPC, 0x0000},  
        {(uint32_t *)&EMU->GQA, 0x0000},
        {(uint32_t *)&EMU->GQB, 0x0000},
        {(uint32_t *)&EMU->GQC, 0x0000},    
        {(uint32_t *)&EMU->GSA, 0x0000},
        {(uint32_t *)&EMU->GSB, 0x0000},
        {(uint32_t *)&EMU->GSC, 0x0000},

        {(uint32_t *)&EMU->PA_PHSL, 0x0000},
        {(uint32_t *)&EMU->PB_PHSL, 0x0000},
        {(uint32_t *)&EMU->PC_PHSL, 0x0000},
        {(uint32_t *)&EMU->QA_PHSL, 0x0000},
        {(uint32_t *)&EMU->QB_PHSL, 0x0000},
        {(uint32_t *)&EMU->QC_PHSL, 0x0000},

        {(uint32_t *)&EMU->PA_OS, 0x0000},
        {(uint32_t *)&EMU->PB_OS, 0x0000},
        {(uint32_t *)&EMU->PC_OS, 0x0000},
        {(uint32_t *)&EMU->QA_OS, 0x0000},
        {(uint32_t *)&EMU->QB_OS, 0x0000},
        {(uint32_t *)&EMU->QC_OS, 0x0000},  
        /*LL库默认未用到基波，基波寄存器清零屏蔽*/
/*       {(uint32_t *)&EMU->FIA_OS, 0x0000},
        {(uint32_t *)&EMU->FIB_OS, 0x0000},
        {(uint32_t *)&EMU->FIC_OS, 0x0000},  

        {(uint32_t *)&EMU->GFPA, 0x0000},
        {(uint32_t *)&EMU->GFPB, 0x0000},
        {(uint32_t *)&EMU->GFPC, 0x0000},
        {(uint32_t *)&EMU->GFQA, 0x0000},
        {(uint32_t *)&EMU->GFQB, 0x0000},
        {(uint32_t *)&EMU->GFQC, 0x0000},
        {(uint32_t *)&EMU->GFSA, 0x0000},
        {(uint32_t *)&EMU->GFSB, 0x0000},
        {(uint32_t *)&EMU->GFSC, 0x0000},

        {(uint32_t *)&EMU->FPA_PHS, 0x0000},
        {(uint32_t *)&EMU->FPB_PHS, 0x0000},
        {(uint32_t *)&EMU->FPC_PHS, 0x0000},      
        {(uint32_t *)&EMU->FQA_PHS, 0x0000},
        {(uint32_t *)&EMU->FQB_PHS, 0x0000},
        {(uint32_t *)&EMU->FQC_PHS, 0x0000},         
        {(uint32_t *)&EMU->FPA_OS, 0x0000},
        {(uint32_t *)&EMU->FPB_OS, 0x0000},
        {(uint32_t *)&EMU->FPC_OS, 0x0000}, 
        {(uint32_t *)&EMU->FQA_OS, 0x0000},
        {(uint32_t *)&EMU->FQB_OS, 0x0000},
        {(uint32_t *)&EMU->FQC_OS, 0x0000},   */  
        {(uint32_t *)&EMU->SAGCFG, 0x0000},  

        {(uint32_t *)&EMU->CFCFG, 0x43210}, 
        {(uint32_t *)&EMU->EMUCFG, 0x400000},   
        {(uint32_t *)&EMU->EMUCON, 0x0000}, 
        {(uint32_t *)&EMU->EMUIE, 0x0000},          
        {(uint32_t *)&EMU->ZXOTU, 0x0000}, 
        {(uint32_t *)&EMU->ZXOTCFG, 0x0000},  
        {(uint32_t *)&EMU->D2FCFG, 0x0000}, 
        {(uint32_t *)&EMU->HFCONST3, 0x0000},
        {(uint32_t *)&EMU->PA_PHSM_D2FP00, 0x0000},
        {(uint32_t *)&EMU->PA_PHSH_D2FP01, 0x0000},
        {(uint32_t *)&EMU->PB_PHSM_D2FP02, 0x0000},
        {(uint32_t *)&EMU->PB_PHSH_D2FP03, 0x0000},
        {(uint32_t *)&EMU->PC_PHSM_D2FP04, 0x0000},
        {(uint32_t *)&EMU->PC_PHSH_D2FP05, 0x0000},
        {(uint32_t *)&EMU->QA_PHSM_D2FP06, 0x0000},
        {(uint32_t *)&EMU->QA_PHSH_D2FP07, 0x0000},
        {(uint32_t *)&EMU->QB_PHSM_D2FP08, 0x0000},
        {(uint32_t *)&EMU->QB_PHSH_D2FP09, 0x0000},
        {(uint32_t *)&EMU->QC_PHSM_D2FP10, 0x0000},
        {(uint32_t *)&EMU->QC_PHSH_D2FP11, 0x0000},

        {(uint32_t *)&EMU->PA_PHSM, 0x0000},
        {(uint32_t *)&EMU->PA_PHSH, 0x0000},
        {(uint32_t *)&EMU->PB_PHSM, 0x0000},
        {(uint32_t *)&EMU->PB_PHSH, 0x0000},
        {(uint32_t *)&EMU->PC_PHSM, 0x0000},
        {(uint32_t *)&EMU->PC_PHSH, 0x0000},
        {(uint32_t *)&EMU->QA_PHSM, 0x0000},
        {(uint32_t *)&EMU->QA_PHSH, 0x0000},
        {(uint32_t *)&EMU->QB_PHSM, 0x0000},
        {(uint32_t *)&EMU->QB_PHSH, 0x0000},
        {(uint32_t *)&EMU->QC_PHSM, 0x0000},
        {(uint32_t *)&EMU->QC_PHSH, 0x0000},

        {(uint32_t *)&EMU->EMU_CFG2, 0x00000004},
    };

    /* Private function prototypes -----------------------------------------------*/

    /* Private functions ---------------------------------------------------------*/

    /* Exported functions ----------------------------------------------------------*/
    /*****************************************************************************
    ** Function name:LL_EMU_RstConfigReg
    **
    ** Description:将设置寄存器配置为初始复位值
    **
    ** Parameters:NONE
    **
    ** Returned value:NONE
    **
    ******************************************************************************/
    void LL_EMU_RstConfigReg(void)
    {
        uint16_t i;
        EMU->SPCMD = SPCMD_EN; /* 打开EMU写使能 */
        #ifdef RN821x_RN721x_SOC_D
        EMU->SPCMD = SPCMD_EN;
        #endif
        for (i = 0; i < sizeof(EmuConfigReg) / sizeof(sEmuCheckConfigReg_TypeDef); i++)
        {
            /* EMU的设置寄存器设置为默认值 */
            *((volatile uint32_t *)EmuConfigReg[i].RegAd) = EmuConfigReg[i].DefaultValue;
        #ifdef RN821x_RN721x_SOC_D
                *((volatile uint32_t *)EmuConfigReg[i].RegAd) = EmuConfigReg[i].DefaultValue;
        #endif
        }
        EMU->SPCMD = SPCMD_DIS; /* 关闭EMU写使能 */
        #ifdef RN821x_RN721x_SOC_D
        EMU->SPCMD = SPCMD_DIS;
        #endif
    }


#endif
    /*****************************************************************************
    ** Function name:LL_EMU_RegWrite
    **
    ** Description:EMU 寄存器设置
    **
    ** Parameters:wReg：寄存器偏移地址
    **			  pBuf：设置数据指针
    **			  ucLen：设置数据长度
    ** Returned value:ERN_SUCCESS:设置成功  ERN_ERROR:设置失败
    **
    ******************************************************************************/
    ErrorStatus LL_EMU_RegWrite(uint16_t wReg, uint8_t *pBuf, uint8_t ucLen)
    {
    #ifdef RN821x_RN721x_SOC_D
        uint8_t i;
        uint32_t TempData, mask, Status;
        if ((ucLen == 0) || (ucLen > 4))
            return (ERN_ERROR);
        EmuCommbuf.u32Data = 0;
        memcpy(&EmuCommbuf.u8Data[0], &pBuf[0], ucLen);
        for (i = 0; i < 3; i++)
        {
            /* 寄存器写入 */
            *((volatile uint32_t *)(EMU_BASE + wReg)) = EmuCommbuf.u32Data;
            *((volatile uint32_t *)(EMU_BASE + wReg)) = EmuCommbuf.u32Data;

            if (wReg == LL_EMU_SPCMD)
            {
                TempData = *((volatile uint32_t *)(EMU_BASE + LL_EMU_EMUStatus));
                TempData = *((volatile uint32_t *)(EMU_BASE + LL_EMU_EMUStatus));

                switch (EmuCommbuf.u32Data)
                {
                case SPCMD_EN:
                    mask = 0x40000000;
                    Status = 0x40000000;
                    break;
                case SPCMD_DIS:
                    mask = 0x40000000;
                    Status = 0x00000000;
                    break;
                case SPCMD_ChannelA:
                    mask = 0x20000000;
                    Status = 0x00000000;
                    break;
                case SPCMD_ChannelB:
                    mask = 0x20000000;
                    Status = 0x20000000;
                    break;
                default:
                    return (ERN_ERROR);
                }

                if ((TempData & mask) == Status)
                {
                    return (ERN_SUCCESS);
                }
                else
                {
                    continue;
                }
            }
            else
            {
                TempData = *((volatile uint32_t *)(EMU_BASE + wReg));
                TempData = *((volatile uint32_t *)(EMU_BASE + wReg));
                if (TempData != EmuCommbuf.u32Data)
                    continue;
                else
                    return (ERN_SUCCESS);
            }
        }
        return (ERN_ERROR);
   
    #elif defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_V2)|| defined(RN821x_RN721x_SOC_V3)||\
        defined(RN202x_RN7326_SOC_V2)
        if ((ucLen == 0) || (ucLen > 4))    return (ERN_ERROR);
        EmuCommbuf.u32Data = 0;
        memcpy(&EmuCommbuf.u8Data[0], &pBuf[0], ucLen);
        *((volatile uint32_t *)(EMU_BASE + wReg)) = EmuCommbuf.u32Data;
        return (ERN_SUCCESS);
    #else 
        return (ERN_ERROR);
    #endif
    }
    /*****************************************************************************
    ** Function name:LL_EMU_RegRead
    **
    ** Description:EMU 寄存器读取
    **
    ** Parameters:wReg：寄存器偏移地址
    **			  pBuf：读取数据存放指针
    **			  ucLen：读取数据长度
    ** Returned value:ERN_SUCCESS:读取成功  ERN_ERROR:读取失败
    **
    ******************************************************************************/
    ErrorStatus LL_EMU_RegRead(uint16_t rReg, uint8_t *pBuf, uint8_t ucLen)
    {
    #ifdef RN821x_RN721x_SOC_D
        uint8_t i;
        uint32_t TempData;
        EmuCommbuf.u32Data = 0;
        if (ucLen == 0)
            return (ERN_ERROR);
        for (i = 0; i < 3; i++)
        {
            TempData = *((volatile uint32_t *)(EMU_BASE + rReg));
            TempData = *((volatile uint32_t *)(EMU_BASE + rReg));
            EmuCommbuf.u32Data = *((volatile uint32_t *)(EMU_BASE + rReg));
            if (TempData != EmuCommbuf.u32Data)
                continue;
            else
            {
                memcpy(&pBuf[0], &EmuCommbuf.u8Data[0], ucLen);
                return (ERN_SUCCESS);
            }
        }
        return (ERN_ERROR);
    #elif defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3) ||\
        defined(RN202x_RN7326_SOC_V2)
        EmuCommbuf.u32Data = *((volatile uint32_t *)(EMU_BASE + rReg));
        memcpy(&pBuf[0], &EmuCommbuf.u8Data[0], ucLen);
        return (ERN_SUCCESS);
    #else 
        return (ERN_ERROR);
    #endif
    }
#endif //LL_EMU_MODULE_ENABLED
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/


