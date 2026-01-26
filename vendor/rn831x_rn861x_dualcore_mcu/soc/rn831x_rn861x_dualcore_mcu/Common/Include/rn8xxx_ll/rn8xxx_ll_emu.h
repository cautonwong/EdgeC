/********************************************************************************
 * @file    Rn8xxx_EMU.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-09-04
 * @brief   Header file containing functions prototypes of EMU HAL library.
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
#ifndef Rn8xxx_EMU_H
#define Rn8xxx_EMU_H

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll_def.h"
/* Exported define ------------------------------------------------------------*/
#define EMU_TIMEOUT 0x3fff

#define SPCMD_EN 0xE5       /* EMU 使能打开值 */
#define SPCMD_DIS 0xDC      /* emu 使能关闭值 */
#define SPCMD_ChannelA 0x5A /* 电流通道A */
#define SPCMD_ChannelB 0xA5 /* 电流通道B */

#ifdef RN821x_RN721x_SOC_D
#define EMU_ClearIF(x) (EMU->IF = x, EMU->IF = x)
#endif
#if defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
#define EMU_ClearIF(x) (EMU->IF = x)
#endif

/* #define HAL_EMU_SPCMD_AD		            ((uint32_t )&EMU->SPCMD		              & 0xfff)	*/
#define LL_EMU_EMUCON ((uint32_t)&EMU->EMUCON & 0xfff)
#define LL_EMU_EMUCON2 ((uint32_t)&EMU->EMUCON2 & 0xfff)
#if  defined(RN202x_RN7326_SOC_V2)
#define LL_EMU_HFConst1 ((uint32_t)&EMU->HFCONST1 & 0xfff)
#define LL_EMU_HFConst2 ((uint32_t)&EMU->HFCONST2 & 0xfff)
#define LL_EMU_UA ((uint32_t)&EMU->UA & 0xfff)
#else
#define LL_EMU_HFConst ((uint32_t)&EMU->HFConst & 0xfff)
#endif
#define LL_EMU_PStart ((uint32_t)&EMU->PStart & 0xfff)
#define LL_EMU_QStart ((uint32_t)&EMU->QStart & 0xfff)
#define LL_EMU_GPQA ((uint32_t)&EMU->GPQA & 0xfff)
#define LL_EMU_GPQB ((uint32_t)&EMU->GPQB & 0xfff)
#define LL_EMU_PhsA ((uint32_t)&EMU->PhsA & 0xfff)
#define LL_EMU_PhsB ((uint32_t)&EMU->PhsB & 0xfff)
#define LL_EMU_QPhsCal ((uint32_t)&EMU->QPhsCal & 0xfff)
#define LL_EMU_APOSA ((uint32_t)&EMU->APOSA & 0xfff)
#define LL_EMU_APOSB ((uint32_t)&EMU->APOSB & 0xfff)
#define LL_EMU_RPOSA ((uint32_t)&EMU->RPOSA & 0xfff)
#define LL_EMU_RPOSB ((uint32_t)&EMU->RPOSB & 0xfff)
#define LL_EMU_IARMSOS ((uint32_t)&EMU->IARMSOS & 0xfff)
#define LL_EMU_IBRMSOS ((uint32_t)&EMU->IBRMSOS & 0xfff)
#define LL_EMU_URMSOS ((uint32_t)&EMU->URMSOS & 0xfff)
#define LL_EMU_IAGAIN ((uint32_t)&EMU->IAGAIN & 0xfff)
#define LL_EMU_IBGAIN ((uint32_t)&EMU->IBGAIN & 0xfff)
#define LL_EMU_UGAIN ((uint32_t)&EMU->UGAIN & 0xfff)
#define LL_EMU_IADCOS ((uint32_t)&EMU->IADCOS & 0xfff)
#define LL_EMU_IBDCOS ((uint32_t)&EMU->IBDCOS & 0xfff)
#define LL_EMU_UDCOS ((uint32_t)&EMU->UDCOS & 0xfff)
#define LL_EMU_UADD ((uint32_t)&EMU->UADD & 0xfff)
#define LL_EMU_USAG ((uint32_t)&EMU->USAG & 0xfff)
#define LL_EMU_IAPEAK ((uint32_t)&EMU->IAPEAK & 0xfff)
#define LL_EMU_IBPEAK ((uint32_t)&EMU->IBPEAK & 0xfff)
#define LL_EMU_UPEAK ((uint32_t)&EMU->UPEAK & 0xfff)
#define LL_EMU_D2FP ((uint32_t)&EMU->D2FP & 0xfff)
#if  defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
#define LL_EMU_EMUCON3 ((uint32_t)&EMU->EMUCON3 & 0x0fff)
#define LL_EMU_EMUCON4 ((uint32_t)&EMU->EMUCON4 & 0x0fff)
#define LL_EMU_EMUCON5 ((uint32_t)&EMU->EMUCON5 & 0x0fff)
#define LL_EMU_CF_CFG ((uint32_t)&EMU->CF_CFG & 0x0fff)
#define LL_EMU_HWRMS_CFG ((uint32_t)&EMU->HWRMS_CFG & 0x0fff)
#define LL_EMU_HWP_CFG ((uint32_t)&EMU->HWP_CFG & 0x0fff)
#define LL_EMU_HWFP_CFG ((uint32_t)&EMU->HWFP_CFG & 0x0fff)
#define LL_EMU_HWQ_CFG ((uint32_t)&EMU->HWQ_CFG & 0x0fff)
#define LL_EMU_HFCONST2 ((uint32_t)&EMU->HFCONST2 & 0x0fff)
#define LL_EMU_HFCONST3 ((uint32_t)&EMU->HFCONST3 & 0x0fff)
#define LL_EMU_Neg1bit_EN ((uint32_t)&EMU->Neg1bit_EN & 0x0fff)
#define LL_EMU_EMUMODE ((uint32_t)&EMU->EMUMODE & 0x0fff)
#define LL_EMU_ATCHOP_CFG ((uint32_t)&EMU->ATCHOP_CFG & 0x0fff)
#define LL_EMU_FGain ((uint32_t)&EMU->FGain & 0x0fff)
#define LL_EMU_APOSFA ((uint32_t)&EMU->APOSFA & 0x0fff)
#define LL_EMU_APOSFB ((uint32_t)&EMU->APOSFB & 0x0fff)
#define LL_EMU_IAHWRMSOS ((uint32_t)&EMU->IAHWRMSOS & 0x0fff)
#define LL_EMU_IBHWRMSOS ((uint32_t)&EMU->IBHWRMSOS & 0x0fff)
#define LL_EMU_UHWRMSOS ((uint32_t)&EMU->UHWRMSOS & 0x0fff)
#endif
#define LL_EMU_PF2Cnt ((uint32_t)&EMU->PF2Cnt & 0xfff)
#define LL_EMU_QF2Cnt ((uint32_t)&EMU->QF2Cnt & 0xfff)
#define LL_EMU_SF2Cnt ((uint32_t)&EMU->SF2Cnt & 0xfff)
#if  defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
#define LL_EMU_PF2Cnt2 ((uint32_t)&EMU->PF2Cnt2 & 0x0fff)
#define LL_EMU_QF2Cnt2 ((uint32_t)&EMU->QF2Cnt2 & 0x0fff)
#define LL_EMU_PF2Cnt3 ((uint32_t)&EMU->PF2Cnt3 & 0x0fff)
#define LL_EMU_QF2Cnt3 ((uint32_t)&EMU->QF2Cnt3 & 0x0fff)
#define LL_EMU_PFCnt2 ((uint32_t)&EMU->PFCnt2 & 0x0fff)
#define LL_EMU_QFCnt2 ((uint32_t)&EMU->QFCnt2 & 0x0fff)
#define LL_EMU_PFCnt3 ((uint32_t)&EMU->PFCnt3 & 0x0fff)
#define LL_EMU_QFCnt3 ((uint32_t)&EMU->QFCnt3 & 0x0fff)
#endif
#define LL_EMU_PFCnt ((uint32_t)&EMU->PFCnt & 0xfff)
#define LL_EMU_QFCnt ((uint32_t)&EMU->QFCnt & 0xfff)
#define LL_EMU_SFCnt ((uint32_t)&EMU->SFCnt & 0xfff)
#define LL_EMU_IARMS ((uint32_t)&EMU->IARMS & 0xfff)
#define LL_EMU_IBRMS ((uint32_t)&EMU->IBRMS & 0xfff)
#define LL_EMU_URMS ((uint32_t)&EMU->URMS & 0xfff)
#define LL_EMU_Ufreq ((uint32_t)&EMU->Ufreq & 0xfff)
#define LL_EMU_PowerPA ((uint32_t)&EMU->PowerPA & 0xfff)
#define LL_EMU_PowerPB ((uint32_t)&EMU->PowerPB & 0xfff)
#define LL_EMU_PowerQA ((uint32_t)&EMU->PowerQA & 0xfff)
#define LL_EMU_PowerQB ((uint32_t)&EMU->PowerQB & 0xfff)
#define LL_EMU_PowerSA ((uint32_t)&EMU->PowerSA & 0xfff)
#define LL_EMU_PowerSB ((uint32_t)&EMU->PowerSB & 0xfff)
#define LL_EMU_EnergyP ((uint32_t)&EMU->EnergyP & 0xfff)
#define LL_EMU_EnergyP2 ((uint32_t)&EMU->EnergyP2 & 0xfff)
#define LL_EMU_EnergyQ ((uint32_t)&EMU->EnergyQ & 0xfff)
#define LL_EMU_EnergyQ2 ((uint32_t)&EMU->EnergyQ2 & 0xfff)
#define LL_EMU_EnergyS ((uint32_t)&EMU->EnergyS & 0xfff)
#define LL_EMU_PFA ((uint32_t)&EMU->PFA & 0xfff)
#define LL_EMU_PFB ((uint32_t)&EMU->PFB & 0xfff)
#define LL_EMU_ANGLEA ((uint32_t)&EMU->ANGLEA & 0xfff)
#define LL_EMU_ANGLEB ((uint32_t)&EMU->ANGLEB & 0xfff)
#define LL_EMU_EMUStatus ((uint32_t)&EMU->EMUStatus & 0xfff)
#define LL_EMU_SPL_IA ((uint32_t)&EMU->SPL_IA & 0xfff)
#define LL_EMU_SPL_IB ((uint32_t)&EMU->SPL_IB & 0xfff)
#define LL_EMU_SPL_U ((uint32_t)&EMU->SPL_U & 0xfff)
#define LL_EMU_PowerPA2 ((uint32_t)&EMU->PowerPA2 & 0xfff)
#define LL_EMU_PowerPB2 ((uint32_t)&EMU->PowerPB2 & 0xfff)
#define LL_EMU_EnergyS2 ((uint32_t)&EMU->EnergyS2 & 0xfff)
#define LL_EMU_SPL_PA ((uint32_t)&EMU->SPL_PA & 0xfff)
#define LL_EMU_SPL_PB ((uint32_t)&EMU->SPL_PB & 0xfff)
#define LL_EMU_SPL_QA ((uint32_t)&EMU->SPL_QA & 0xfff)
#define LL_EMU_SPL_QB ((uint32_t)&EMU->SPL_QB & 0xfff)
#define LL_EMU_TRNG ((uint32_t)&EMU->TRNG & 0xfff)
#define LL_EMU_EMUStatus2 ((uint32_t)&EMU->EMUStatus2 & 0xfff)
#define LL_EMU_IE ((uint32_t)&EMU->IE & 0xfff)
#define LL_EMU_IF ((uint32_t)&EMU->IF & 0xfff)
#define LL_EMU_DMAEN ((uint32_t)&EMU->DMAEN & 0xfff)
#define LL_EMU_Rdata ((uint32_t)&EMU->Rdata & 0xfff)
#define LL_EMU_Wdata ((uint32_t)&EMU->Wdata & 0xfff)
/************************************************************************************************/
#define LL_EMU_SPCMD ((uint32_t)&EMU->SPCMD & 0x0fff)
#if  defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
#define LL_EMU_EMUStatus3 ((uint32_t)&EMU->EMUStatus3 & 0x0fff)
#define LL_EMU_IE3 ((uint32_t)&EMU->IE3 & 0x0fff)
#define LL_EMU_IF3 ((uint32_t)&EMU->IF3 & 0x0fff)
#define LL_EMU_EMUStatus4 ((uint32_t)&EMU->EMUStatus4 & 0x0fff)
#define LL_EMU_HW_RMSIA ((uint32_t)&EMU->HW_RMSIA & 0x0fff)
#define LL_EMU_HW_RMSIB ((uint32_t)&EMU->HW_RMSIB & 0x0fff)
#define LL_EMU_HW_RMSU ((uint32_t)&EMU->HW_RMSU & 0x0fff)
#define LL_EMU_HW_FPA ((uint32_t)&EMU->HW_FPA & 0x0fff)
#define LL_EMU_HW_FPB ((uint32_t)&EMU->HW_FPB & 0x0fff)
#define LL_EMU_SPL_IA2 ((uint32_t)&EMU->SPL_IA2 & 0x0fff)
#define LL_EMU_SPL_IB2 ((uint32_t)&EMU->SPL_IB2 & 0x0fff)
#define LL_EMU_SPL_U2 ((uint32_t)&EMU->SPL_U2 & 0x0fff)
#define LL_EMU_SPL_FIA ((uint32_t)&EMU->SPL_FIA & 0x0fff)
#define LL_EMU_SPL_FIB ((uint32_t)&EMU->SPL_FIB & 0x0fff)
#define LL_EMU_SPL_FU ((uint32_t)&EMU->SPL_FU & 0x0fff)
#define LL_EMU_EnergyP_2 ((uint32_t)&EMU->EnergyP_2 & 0x0fff)
#define LL_EMU_EnergyP2_2 ((uint32_t)&EMU->EnergyP2_2 & 0x0fff)
#define LL_EMU_EnergyQ_2 ((uint32_t)&EMU->EnergyQ_2 & 0x0fff)
#define LL_EMU_EnergyQ2_2 ((uint32_t)&EMU->EnergyQ2_2 & 0x0fff)
#define LL_EMU_EnergyP_3 ((uint32_t)&EMU->EnergyP_3 & 0x0fff)
#define LL_EMU_EnergyP2_3 ((uint32_t)&EMU->EnergyP2_3 & 0x0fff)
#define LL_EMU_EnergyQ_3 ((uint32_t)&EMU->EnergyQ_3 & 0x0fff)
#define LL_EMU_EnergyQ2_3 ((uint32_t)&EMU->EnergyQ2_3 & 0x0fff)
#define LL_EMU_IADCOS_Calc ((uint32_t)&EMU->IADCOS_Calc & 0x0fff)
#define LL_EMU_IBDCOS_Calc ((uint32_t)&EMU->IBDCOS_Calc & 0x0fff)
#define LL_EMU_UDCOS_Calc ((uint32_t)&EMU->UDCOS_Calc & 0x0fff)
#define LL_EMU_ZXOTCFG ((uint32_t)&EMU->ZXOTCFG & 0x0fff)
#define LL_EMU_ZXOTI ((uint32_t)&EMU->ZXOTI & 0x0fff)
#define LL_EMU_ZXOTU ((uint32_t)&EMU->ZXOTU & 0x0fff)
#define LL_EMU_ROS_CTRL ((uint32_t)&EMU->ROS_CTRL & 0x0fff)
#define LL_EMU_ROS_DCATTC ((uint32_t)&EMU->ROS_DCATTC & 0x0fff)
#define LL_EMU_ROS_Tran_K ((uint32_t)&EMU->ROS_Tran_K & 0x0fff)
#define LL_EMU_PQSRUN ((uint32_t)&EMU->PQSRUN & 0x0fff)
#define LL_EMU_ECT_EN ((uint32_t)&EMU->ECT_EN & 0x0fff)
#define LL_EMU_ECT_IAGain ((uint32_t)&EMU->ECT_IAGain & 0x0fff)
#define LL_EMU_ECT_IBGain ((uint32_t)&EMU->ECT_IBGain & 0x0fff)
#define LL_EMU_ECT_UGain ((uint32_t)&EMU->ECT_UGain & 0x0fff)
#define LL_EMU_DMA_WAVE_CFG ((uint32_t)&EMU->DMA_WAVE_CFG & 0x0fff)
#define LL_EMU_DMA_BUF_CTRL ((uint32_t)&EMU->DMA_BUF_CTRL & 0x0fff)
#define LL_EMU_DMA_BUF_BADDR ((uint32_t)&EMU->DMA_BUF_BADDR & 0x0fff)
#define LL_EMU_DMA_BUF_DEPTH ((uint32_t)&EMU->DMA_BUF_DEPTH & 0x0fff)
#define LL_EMU_DMA_GAP_CFG ((uint32_t)&EMU->DMA_GAP_CFG & 0x0fff)
#define LL_EMU_DMA_BUFF_ADDR ((uint32_t)&EMU->DMA_BUFF_ADDR & 0x0fff)
#define LL_EMU_DMA_ERR_ADDR ((uint32_t)&EMU->DMA_ERR_ADDR & 0x0fff)
#define LL_EMU_DMA_CHECKSUM ((uint32_t)&EMU->DMA_CHECKSUM & 0x0fff)
#define LL_EMU_DMA_RCD_CFG ((uint32_t)&EMU->DMA_RCD_CFG & 0x0fff)
#define LL_EMU_DMA_BUF_RCD_CTRL ((uint32_t)&EMU->DMA_BUF_RCD_CTRL & 0x0fff)
#define LL_EMU_DMA_BUF_RCD_BADDR ((uint32_t)&EMU->DMA_BUF_RCD_BADDR & 0x0fff)
#define LL_EMU_DMA_BUF_RCD_DEPTH ((uint32_t)&EMU->DMA_BUF_RCD_DEPTH & 0x0fff)
#define LL_EMU_DMA_RCD_GAP_DEPTH ((uint32_t)&EMU->DMA_RCD_GAP_DEPTH & 0x0fff)
#define LL_EMU_DMA_BUF_RCD_ADDR ((uint32_t)&EMU->DMA_BUF_RCD_ADDR & 0x0fff)
#define LL_EMU_DMA_BUF_RCD_ERR_ADDR ((uint32_t)&EMU->DMA_BUF_RCD_ERR_ADDR & 0x0fff)
#define LL_EMU_RCD_CTRL ((uint32_t)&EMU->RCD_CTRL & 0x0fff)
#define LL_EMU_RCD_EN ((uint32_t)&EMU->RCD_EN & 0x0fff)
#define LL_EMU_RCD_ISTH ((uint32_t)&EMU->RCD_ISTH & 0x0fff)
#define LL_EMU_RCD_INTCFGA ((uint32_t)&EMU->RCD_INTCFGA & 0x0fff)
#define LL_EMU_RCD_INTCFGB ((uint32_t)&EMU->RCD_INTCFGB & 0x0fff)
#define LL_EMU_RCD_INTRESA ((uint32_t)&EMU->RCD_INTRESA & 0x0fff)
#define LL_EMU_RCD_INTRESB ((uint32_t)&EMU->RCD_INTRESB & 0x0fff)
#define LL_EMU_RCD_IE ((uint32_t)&EMU->RCD_IE & 0x0fff)
#define LL_EMU_RCD_IF ((uint32_t)&EMU->RCD_IF & 0x0fff)
#define LL_EMU_RCD_STA ((uint32_t)&EMU->RCD_STA & 0x0fff)
#define LL_EMU_TRIG_CTRL ((uint32_t)&EMU->TRIG_CTRL & 0x0fff)
#define LL_EMU_TIRG_EN ((uint32_t)&EMU->TIRG_EN & 0x0fff)
#define LL_EMU_TRIG_STOP ((uint32_t)&EMU->TRIG_STOP & 0x0fff)
#define LL_EMU_TRIG_LEN ((uint32_t)&EMU->TRIG_LEN & 0x0fff)
#define LL_EMU_TRIG_DLY ((uint32_t)&EMU->TRIG_DLY & 0x0fff)
#define LL_EMU_TRIG_STA ((uint32_t)&EMU->TRIG_STA & 0x0fff)
#define LL_EMU_TRIG_LEN2 ((uint32_t)&EMU->TRIG_LEN2 & 0x0fff)
#define LL_EMU_TRIG_STA2 ((uint32_t)&EMU->TRIG_STA2 & 0x0fff)
#endif

#define GET_REG_EMUStatus(databits) (EMU->EMUStatus & GET_DATA_MASK(databits))
#define GET_REG_EMUStatus2(databits) (EMU->EMUStatus2 & GET_DATA_MASK(databits))
#if  defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)
#define GET_REG_EMUStatus3(databits) (EMU->EMUStatus3 & GET_DATA_MASK(databits))
#endif
/* Exported enum ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef struct
{
    uint32_t *RegAd;
    uint32_t DefaultValue;
} sEmuCheckConfigReg_TypeDef;

/* Exported data ----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
extern void LL_EMU_RstConfigReg(void);
extern ErrorStatus LL_EMU_RegWrite(uint16_t wReg, uint8_t *pBuf, uint8_t ucLen);
extern ErrorStatus LL_EMU_RegRead(uint16_t wReg, uint8_t *pBuf, uint8_t ucLen);
#endif /* Rn8xxx_EMU_H */

/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
