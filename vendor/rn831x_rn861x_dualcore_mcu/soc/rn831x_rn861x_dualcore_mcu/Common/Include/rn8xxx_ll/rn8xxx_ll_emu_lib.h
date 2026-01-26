/******************************************************************************
 * @file     rn8xxx_ll_emu_lib.h
 * @brief    emu configuration header
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#ifndef RN8XXX_LL_EMU_LIB_H_
#define RN8XXX_LL_EMU_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "renergy_chip_config.h"
#include "rn8xxx_ll_devices.h"

/**********************************************************
  api return
**********************************************************/
typedef enum
{
    EMU_LIB_PASS = 0,
    EMU_LIB_FAIL = 1
} eEmuLibRet_TypeDef;

typedef enum
{
	UEDTModeSel_U400 	= 0x0,
	UEDTModeSel_Vbisa 	= 0x1
} eUEDTModeSel_TypeDef;

typedef enum
{
  EXTREF_ENABLE = 0,
  EXTREF_DISABLE = 1
} eExtRefSel_TypeDef;

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B)
/**
 * @brief LL_EMU_ReadUEdtVBCtlReg
 *
 * @param
 *    in unsigned int EmuFlag: select EMU Enable(EmuFlag = 1) or EMU Disbale(EmuFlag = 0)
 *		 unsigned char Usp400Flag:test_en_vref_icc04_vdet Enable(Usp400Flag = 1) or Disbale(Usp400Flag = 0)
 * @return
 *    SYSCTRL_PASS: CMP UEdtVB_CTL pass
 *    SYSCTRL_FAIL: CMP UEdtVB_CTL fail
 *
 */
eEmuLibRet_TypeDef LL_EMU_ReadUEdtVBCtlReg(unsigned int RegData , unsigned char ModeSel);
#define ReadUEdtVBCtlReg LL_EMU_ReadUEdtVBCtlReg

/**
 * @brief LL_EMU_WriteUEdtVBCtlReg
 *
 * @param
 *    in unsigned int EmuFlag: select EMU Enable(EmuFlag = 1) or EMU Disbale(EmuFlag = 0)
 *		 unsigned char Usp400Flag:test_en_vref_icc04_vdet Enable(Usp400Flag = 1) or Disbale(Usp400Flag = 0)
 * @return
 *    SYSCTRL_PASS: set UEdtVB_CTL pass
 *    SYSCTRL_FAIL: set UEdtVB_CTL fail
 *
 */
eEmuLibRet_TypeDef LL_EMU_WriteUEdtVBCtlReg(unsigned int RegData , unsigned char ModeSel);
#define WriteUEdtVBCtlReg LL_EMU_WriteUEdtVBCtlReg
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B) ||\
    defined(RN202x_RN7326_SOC_V2)
/**
 * @brief EMU external reference voltage configuration
 *
 * @param ref_sel EXTREF_ENABLE: use external Vref;
 *                EXTREF_DISABLE: use internal Vref
 * @return eEmuLibRet_TypeDef pass or fail
 */
eEmuLibRet_TypeDef LL_EMU_ExtRefConf(eExtRefSel_TypeDef ref_sel);

#endif


#ifdef __cplusplus
}
#endif

#endif
/* r2989 */
