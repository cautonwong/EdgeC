/*******************************************************************************
* @file    rn8xxx_ll_dsp.h
* @author  Renergy Application Team
* @version V1.1.1
* @date    2023-09-07
* @brief   Header file containing functions prototypes of math by DSP HAL library.
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
 *
 ******************************************************************************
  * @ Revised records
  * V1.1.1   2023-09-07  Add   Add macro definition for geting register and register bit
  * V1.1.0               Initial
*/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef Rn8xxx_DSP_H
#define Rn8xxx_DSP_H

#include "rn8xxx_ll_def.h"

#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) || defined(RN202x_RN7326_SOC_V2) ||\
	defined(RN831x_RN861x_MCU_V4)

#define FFT_I2F_TIMEOUT		(0x3ffU)
#define FFT_BITREV_TIMEOUT 	(0x3ffU)
#define	DSP_TIMEOUT 	(0x2fffU)  /*"1024点3.84ms"*/

typedef struct
{
	uint32_t ftemp_Int;		/*"当前次谐波幅值ֵ"*/
	uint32_t fBase_Int;		/*"基波值"*/
	uint32_t Hpercent;		/*"计算的含量"*/
	uint32_t HRms;			/*"谐波有效值"*/
	uint32_t nMul;			/*"含量放大倍数"*/
	uint32_t EmuReg_Base;	/*"基波寄存器值"*/
}sDspMath_Percent_TypeDef;

typedef struct
{
	uint32_t ftemp_Int;		/*"当前次谐波幅值"*/
    uint32_t fBase_Int;		/*"基波值"*/
	uint32_t Hpercent;		/*"计算的含量"*/
	uint32_t HRms;			/*"谐波有效值"*/
    uint32_t nMul;			/*"含量放大倍数"*/
	uint32_t EmuReg_Base;	/*"基波寄存器值"*/
	uint32_t tPercent[10];	/*"10个间谐波含量中间值，用于计算每次间谐波群数据"*/
}sDspMath_InterPercent_TypeDef;

typedef struct
{
	uint32_t CosAngleU;
	uint32_t CosAngleI_Fu;
	uint32_t SinAngleU_Fi;
	uint32_t SinAngleI_Gp;
	uint32_t AngleU;			/*"谐波电压角度"*/
	uint32_t AngleI;			/*"谐波电流角度"*/
	uint16_t AngleH_UI;			/*"谐波相角"*/
}sDspMath_HPf_Pw_TypeDef;

typedef struct
{
   int32_t x;
   uint32_t y;
} sLagrangeS_TypeDef;

#ifdef RN202x_RN7326_SOC_V2
typedef enum
{
	NOTDO_MODE 		= 0U,
	I2F_ONCE_MODE 	= 1U,
	I2F_DMA_MODE  	= 2U,
	F2I_ONCE_MODE	= 4U,
	F2I_DMA_MODE 	= 8U,
	FP_MUL_MODE 	= 0x10U,
	FP_ADD_MODE		= 0x20U,
	FP_SUB_MODE 	= 0x40U,
	FP_MLAD_MODE 	= 0x80U,

	BTFY_ONCE_MODE 	= 0x100U,
	BTFY_DMA_MODE	= 0x200U,

	FFT_MODE 		= 0x400U,

	BIT_RVE_MODE 	= 0x800U,
	ALL_DIS_MODE 	= 0U,

	MUL_IN_FM_Fp 		= 0U,
	MUL_IN_FM_Int	 	= 1U,

	MUL_OUT_DFM_Fp 		= 0U,
	MUL_OUT_DFM_Int	 	= 1U,

	ROUND_MODE_NEAREST 	= 0U,
	ROUND_MODE_ZERO	 	= 1U,
	ROUND_MODE_PINFINITY= 2U,
	ROUND_MODE_NINFINITY= 3U,
	ROUND_MODE_UP		= 4U,
	ROUND_MODE_AWAY		= 5U,

	TB_EN_DIS 			= 0U,
	TB_EN_EN	 		= 1U,

	I2F_PRE_32BIT		= 0U,
	I2F_PRE_24BIT		= 1U,
	I2F_PRE_16BIT		= 2U,
	I2F_PRE_8BIT		= 3U,

} eDspCtrl0_TypeDef;

typedef enum
{
	MAC_CTL_DMA_DIS   = (0U<<0U),
	MAC_CTL_DMA_EN    = (1U<<0U),

	BTFY_ONCE_KICK_DIS= (0U<<1U),
	BTFY_ONCE_KICK_EN = (1U<<1U),

	DIV_KICK_DIS      = (0U<<2U),
	DIV_KICK_EN       = (1U<<2U),
} eDspCtrl1_TypeDef;

typedef enum
{
	IIR_ONCE_MODE  	= 1U,
	IIR_DMA_MODE 	= 2U,
	FIR_DMA_MODE 	= 3U,
	LINEAR_MODE 	= 4U,
	LAGRANGE_MODE 	= 5U,
}  eDspCtrl2_TypeDef;

typedef union
{
	struct
	{
		uint32_t	MODE_SEL 	: 12;
		uint32_t	MUL_IN_FM  	: 1;
		uint32_t	MUL_OUT_FM  : 1;
		uint32_t	ROUND_MODE  : 3;
		uint32_t	I2F_DIV  	: 5;
		uint32_t	F2I_MUL  	: 5;
		uint32_t	FFT_TB_EN  	: 1;
		uint32_t 	I2F_PRE_EN	: 2;
		uint32_t  	RESERVEBIT  : 2;

	}bitMACCtrl0;
	uint32_t MACCtrl0;
}uDSPMacCtrl0_TypeDef;

typedef union
{
	struct
	{
		uint32_t	DMA_EN 			: 1;
		uint32_t	BTFY_ONCE_KICK  : 1;
		uint32_t	DIV_KICK  		: 1;
		uint32_t	Reserved  		: 29;
	}bitMACCtrl1;
	uint32_t MACCtrl1;
}uDSPMacCtrl1_TypeDef;

typedef union
{
	struct
	{
		uint32_t	MODE_SEL 		: 4;
		uint32_t	INTP_LAST_EN 	: 1;
		uint32_t	DMA_STEP  		: 3;
		uint32_t	FIR_ORDER  		: 8;
		uint32_t	Reserved  		: 16;
	}bitMACCtrl2;
	uint32_t MACCtrl2;
}uDSPMacCtrl2_TypeDef;

#else
/*MAC_CTL0 配置*/
typedef enum
{
	NOTDO_MODE 		= 0U,
	I2F_ONCE_MODE 	= 1U,
	I2F_DMA_MODE  	= 2U,
	F2I_ONCE_MODE	= 3U,
	F2I_DMA_MODE 	= 4U,
	FP_MUL_MODE 	= 5U,
	FP_ADD_MODE		= 6U,
	FP_SUB_MODE 	= 7U,
	FP_MLAD_MODE 	= 8U,

	BTFY_ONCE_MODE 	= 9U,
	BTFY_DMA_MODE	= 10U,

	FFT_MODE 		= 11U,

	BIT_RVE_MODE 	= 12U,
	IIR_ONCE_MODE  	= 13U,
	IIR_DMA_MODE 	= 14U,
	FIR_DMA_MODE 	= 15U,
	LINEAR_MODE 	= 16U,
	LAGRANGE_MODE 	= 17U,
	ALL_DIS_MODE 	= 18U,

	MUL_IN_FM_Fp 		= 0U,
	MUL_IN_FM_Int	 	= 1U,

	MUL_OUT_DFM_Fp 		= 0U,
	MUL_OUT_DFM_Int	 	= 1U,

	ROUND_MODE_NEAREST 	= 0U,
	ROUND_MODE_ZERO	 	= 1U,
	ROUND_MODE_PINFINITY= 2U,
	ROUND_MODE_NINFINITY= 3U,
	ROUND_MODE_UP		= 4U,
	ROUND_MODE_AWAY		= 5U,

	TB_EN_DIS 			= 0U,
	TB_EN_EN	 		= 1U,

	I2F_PRE_32BIT		= 0U,
	I2F_PRE_24BIT		= 1U,
	I2F_PRE_16BIT		= 2U,
	I2F_PRE_8BIT		= 3U,

} eDspCtrl0_TypeDef;

typedef enum
{
	MAC_CTL_DMA_DIS   = (0U<<0U),
	MAC_CTL_DMA_EN    = (1U<<0U),

	BTFY_ONCE_KICK_DIS= (0U<<1U),
	BTFY_ONCE_KICK_EN = (1U<<1U),

	DIV_KICK_DIS      = (0U<<2U),
	DIV_KICK_EN       = (1U<<2U),
} eDspCtrl2_TypeDef;

typedef union
{
	struct
	{
		uint32_t	MODE_SEL 	: 5;
		uint32_t	MUL_IN_FM  	: 1;
		uint32_t	MUL_OUT_FM  : 1;
		uint32_t	ROUND_MODE  : 3;
		uint32_t	I2F_DIV  	: 5;
		uint32_t	F2I_MUL  	: 5;
		uint32_t	FFT_TB_EN  	: 1;
		uint32_t 	I2F_PRE_EN	: 2;
		uint32_t  	RESERVEBIT  : 9;

	}bitMACCtrl0;
	uint32_t MACCtrl0;
}uDSPMacCtrl0_TypeDef;

typedef union
{
	struct
	{
		uint32_t	DMA_EN 			: 1;
		uint32_t	BTFY_ONCE_KICK  : 1;
		uint32_t	DIV_KICK  		: 1;
		uint32_t	Reserved  		: 29;
	}bitMACCtrl2;
	uint32_t MACCtrl2;
}uDSPMacCtrl2_TypeDef;

#endif

typedef union
{
	float 		f;
	uint32_t 	u;
} uFloatUint32_TypeDef;

typedef struct
{
	uint32_t Phs_A_Angle;
	uint32_t Phs_B_Angle[3];
	uint32_t Phs_C_Angle[3];
	uint32_t Cos_PhsA;
	uint32_t Sin_PhsA;
	uint32_t Cos_PhsB;
	uint32_t Sin_PhsB;
	uint32_t Cos_PhsC;
	uint32_t Sin_PhsC;
	uint32_t temp_a;
	uint32_t temp_b;
}sVector3P4_TempData_TypeDef;

typedef struct
{
	uint32_t Phs_C_Angle[3];				/*" 0 Angle_c  1: Angle_c+120  2: Angle_c+240 "*/
	uint32_t temp_iba;
	uint32_t temp_ibb;
	uint32_t Cos_Phs[3];
	uint32_t Sin_Phs[3];
	uint32_t temp_a;
	uint32_t temp_b;
}sVector3P3_ITemp_TypeDef;

typedef union {
    uint32_t* uint32Ptr;
    sVector3P4_TempData_TypeDef* vector3P4Ptr;
} uPtrContainer3P4_TypeDef;

typedef union {
    uint32_t* uint32Ptr;
    sVector3P3_ITemp_TypeDef* vector3P3Ptr;
} uPtrContainer3P3_TypeDef;

typedef struct
{
	uint32_t S_Min_Temp;
	uint32_t S_Step_Temp;
	uint32_t Pst_0_1;
	uint32_t Pst_0_7;
	uint32_t Pst_1;
	uint32_t Pst_1_5;
	uint32_t Pst_2_2;
	uint32_t Pst_3;
	uint32_t Pst_4;
	uint32_t Pst_6;
	uint32_t Pst_8;
	uint32_t Pst_10;
	uint32_t Pst_13;
	uint32_t Pst_17;
	uint32_t Pst_30;
	uint32_t Pst_50;
	uint32_t Pst_80;
}sFlick_TempData_TypeDef;

/* Exported define ------------------------------------------------------------*/

#define GET_DSP_INTP_LOC(dspx,bit)  ((dspx)->INTP_LOC & GET_DATA_MASK(databits))
#define GET_DSP_INTP_STEP(dspx,bit)  ((dspx)->INTP_STEP & GET_DATA_MASK(databits))

#define GET_DSP_FLAG(dspx, bit)  (((dspx)->FLG & (bit)) ? (1U) : (0U))
#define GET_DSP_ALU_STA0(dspx, bit)  (((dspx)->ALU_STA0 & (bit)) ? (1U) : (0U))
#define GET_DSP_ALU_STA1(dspx, bit)  (((dspx)->ALU_STA1 & (bit)) ? (1U) : (0U))
#define GET_DSP_CRD_FLAG(dspx, bit)  (((dspx)->CRD_FLG & (bit)) ? (1U) : (0U))

extern ErrorStatus LL_DSP_Int2Fp(uint32_t Dst ,uint32_t Src , uint16_t Len , uint8_t Div , uint8_t DataMode);
extern ErrorStatus LL_DSP_U16I2Fp(uint32_t *Dst ,const uint16_t *Src , uint16_t Len);
extern ErrorStatus LL_DSP_Fp2Int(uint32_t Dst ,uint32_t Src , uint8_t Mul , uint16_t Len);
extern ErrorStatus LL_DSP_BitReverse(uint32_t Dst ,uint32_t Src , uint16_t Len);
extern ErrorStatus LL_DSP_FFT(uint32_t SrbAddr ,uint32_t SibAddr , uint32_t PrbAddr , uint32_t PibAddr , uint16_t Len);
extern ErrorStatus LL_DSP_FpSqrt(uint32_t *X_In , uint32_t *Y_In , uint32_t *Amp_Out , uint32_t *Arctan_Out);
extern ErrorStatus LL_DSP_FpMult(uint32_t In0 , uint32_t In1 , uint32_t *Out );
extern ErrorStatus LL_DSP_FpAdd(uint32_t In0 , uint32_t In1 , uint32_t *Out );
extern ErrorStatus LL_DSP_FpSub(uint32_t In0 , uint32_t In1 , uint32_t *Out );
extern ErrorStatus LL_DSP_Sin_Cos(uint32_t In0 , uint32_t *CosOut , uint32_t *SinOut );
extern ErrorStatus LL_DSP_IntDiv(uint32_t In0 , uint32_t In1 , uint32_t *Out , uint16_t Mul_N);
extern ErrorStatus LL_DSP_FlDiv(uint32_t In0 , uint32_t In1 , uint32_t *Out);
extern ErrorStatus LL_DSP_FIR(uint32_t *InData , uint32_t *OutData , uint32_t *ParaDtat , uint16_t InLen , uint8_t FirOrder);
extern ErrorStatus LL_DSP_HWCalc(uint32_t *InData , uint32_t *OutData , uint32_t *LastHwData);
extern ErrorStatus LL_DSP_HWCalc_2Wave(uint32_t *InData , uint32_t *OutData , uint32_t *LastHwData , uint8_t PointFlag);
extern ErrorStatus LL_DSP_fBaseCalc(uint32_t *InData , uint32_t *OutData_F , uint32_t KMlu , uint8_t AveFlag);
extern ErrorStatus LL_DSP_Lagrange(uint32_t *InData , uint32_t *OutData , uint16_t InLen , uint16_t OutLen , uint32_t LastData, uint16_t Frequency , uint32_t FWaveRate);
extern ErrorStatus LL_DSP_HPercent(sDspMath_Percent_TypeDef *HPercent);
extern ErrorStatus LL_DSP_InterHPercent(sDspMath_InterPercent_TypeDef *HPercent);
extern ErrorStatus LL_DSP_Isgrp(sDspMath_InterPercent_TypeDef *HPercent ,  uint32_t *FftCoret , uint8_t Mode);
extern uint32_t LL_DSP_CalcThd(uint32_t *Dst , uint8_t Num);
extern uint32_t LL_DSP_CalcThdRms(uint32_t Thd , uint32_t FRMS);
extern int16_t LL_DSP_HPf(sDspMath_HPf_Pw_TypeDef *HPf);
extern int32_t LL_DSP_Pw(sDspMath_HPf_Pw_TypeDef *HPf);
extern ErrorStatus LL_DSP_VectorCalc3P4(uint32_t *AngleReg , uint32_t *RmsReg , uint32_t *VectorOut);
extern ErrorStatus LL_DSP_VectorCalc3P3_U(uint32_t *AngleReg , uint32_t *RmsReg , uint32_t *VectorOut);
extern ErrorStatus LL_DSP_VectorCalc3P3_I(uint32_t *AngleReg , uint32_t *RmsReg , uint32_t *VectorOut);
extern uint32_t LL_DSP_CalcPst(sFlick_TempData_TypeDef *PstTemp);
extern void LL_DSP_Disable(void);
#endif
#endif
/****************************  (C) COPYRIGHT Renergy  ****************************/
/****************************          END OF FILE          ****************************/


