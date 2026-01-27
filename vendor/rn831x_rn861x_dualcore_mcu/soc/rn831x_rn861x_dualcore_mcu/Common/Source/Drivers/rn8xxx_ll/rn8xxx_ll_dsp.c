/********************************************************************************
  * @file    rn8xxx_ll_dsp.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-08-29
  * @brief   DSP driver
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
					 ##### DSP peripheral features #####
  ==============================================================================
  [..]

					 ##### How to use this driver #####
  ==============================================================================
	[..]


	@endverbatim
  */

#include "rn8xxx_ll.h"
#ifdef LL_DSP_MODULE_ENABLED
#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) || defined(RN202x_RN7326_SOC_V2) ||\
	defined(RN831x_RN861x_MCU_V4)
/*********************************************************************************************************
** Function name:       LL_DSP_Int2Fp
** Descriptions:        ADC采样数据转换成单精度浮点数格式
** input parameters:    Dst:转换后数据存储目标地址
**						Src:转换数据源地址
**						Len: 数据长度，为4的整数倍
**						Div: 整数转浮点数时除以2^Div
**						0: 32bit data   1: 24bit data 2: 16bit data 3: 8bit data
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_Int2Fp(uint32_t Dst ,uint32_t Src , uint16_t Len , uint8_t Div , uint8_t DataMode)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)I2F_DMA_MODE;
	MacCtrl0.bitMACCtrl0.I2F_DIV = (uint8_t)Div;

	switch(DataMode)
	{
		case  0:
			MacCtrl0.bitMACCtrl0.I2F_PRE_EN = (uint8_t)I2F_PRE_32BIT;
			break;

		case  1:
			MacCtrl0.bitMACCtrl0.I2F_PRE_EN = (uint8_t)I2F_PRE_24BIT;
			break;
		case  2:
			MacCtrl0.bitMACCtrl0.I2F_PRE_EN = (uint8_t)I2F_PRE_16BIT;
			break;

		case  3:
			MacCtrl0.bitMACCtrl0.I2F_PRE_EN = (uint8_t)I2F_PRE_8BIT;
			break;

		default:
			break;

	}

	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->FLG = 0x02U;
	DSP->DMA_SRBADR = ((uint32_t)Src&0x1ffffU)/4U;
	DSP->DMA_TRBADR = ((uint32_t)Dst&0x1ffffU)/4U;
	DSP->DMA_LEN = (uint32_t)Len - 1U;

#ifdef RN202x_RN7326_SOC_V2
	DSP->MAC_CTL1 = (uint32_t)MAC_CTL_DMA_EN;
#else
	DSP->MAC_CTL2 = (uint32_t)MAC_CTL_DMA_EN;
#endif
	if(fnRegState_Detect((uint32_t)(&DSP->FLG) , 1U , 1U ,FFT_I2F_TIMEOUT) == ERN_ERROR)
	{
		return(ERN_ERROR);/* 等待转换完成 */
	}
	else
	{
		DSP->FLG = 0x02U;
		return(ERN_SUCCESS);
	}
}
/*********************************************************************************************************
** Function name:       LL_DSP_U16I2Fp
** Descriptions:        16位整型数据转换成单精度浮点数格式
** input parameters:    Dst:转换后数据存储目标地址
**						Src:转换数据源地址
**						Len: 数据长度，为4的整数倍
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_U16I2Fp(uint32_t *Dst ,const uint16_t *Src , uint16_t Len)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;
	uint16_t i;

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)I2F_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;

	for(i=0U;i<Len;i++)
	{
		DSP->MAC_IN0 = Src[i];
		Dst[i] = DSP->MAC_OUT0;
	}
	return(ERN_SUCCESS);
}
/*********************************************************************************************************
** Function name:       LL_DSP_Fp2Int
** Descriptions:        单精度浮点数格式转换成整数
** input parameters:    Dst:转换后数据存储目标地址
**						Src:转换数据源地址
**                      Mul: 放大倍数，表示放大2^Mul
**						Len: 数据长度，为4的整数倍
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_Fp2Int(uint32_t Dst ,uint32_t Src , uint8_t Mul , uint16_t Len)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)F2I_DMA_MODE;
	MacCtrl0.bitMACCtrl0.F2I_MUL = Mul;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;

	DSP->FLG = 0x10U;
	DSP->DMA_SRBADR = ((uint32_t)Src&0x1ffffU)/4U;
	DSP->DMA_TRBADR = ((uint32_t)Dst&0x1ffffU)/4U;
	DSP->DMA_LEN = (uint32_t)Len - 1U;

#ifdef RN202x_RN7326_SOC_V2
	DSP->MAC_CTL1 = (uint32_t)MAC_CTL_DMA_EN;
#else
	DSP->MAC_CTL2 = (uint32_t)MAC_CTL_DMA_EN;
#endif
	if(fnRegState_Detect((uint32_t)(&DSP->FLG) , 4U , 1U ,FFT_I2F_TIMEOUT) == ERN_ERROR)
	{
		return(ERN_ERROR);/* 等待转换完成 */
	}
	else
	{
		DSP->FLG = 0x10U;
		return(ERN_SUCCESS);
	}
}
/*********************************************************************************************************
** Function name:       LL_DSP_BitReverse
** Descriptions:        数据bit Reverse
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_BitReverse(uint32_t Dst ,uint32_t Src , uint16_t Len)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)BIT_RVE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->DMA_SRBADR = (Src&0x1ffffU)/4U;
	DSP->DMA_TRBADR = (Dst&0x1ffffU)/4U;
	DSP->DMA_LEN = (uint32_t)Len - 1U;
	DSP->FLG = 0x100U;
#ifdef RN202x_RN7326_SOC_V2
	DSP->MAC_CTL1 = (uint32_t)MAC_CTL_DMA_EN;
#else
	DSP->MAC_CTL2 = (uint32_t)MAC_CTL_DMA_EN;
#endif
	if(fnRegState_Detect((uint32_t)(&DSP->FLG) , 8U , 1U ,FFT_BITREV_TIMEOUT) == ERN_ERROR)
	{
		return(ERN_ERROR);/* 等待转换完成 */
	}
	else
	{
		DSP->FLG = 0x100U;
		return(ERN_SUCCESS);
	}
}
/*********************************************************************************************************
** Function name:       LL_DSP_FFT
** Descriptions:        数据FFT转换，进行叠形运算
** input parameters:    SrbAddr:数据实部地址
**						SibAddr：数据虚部地址
**						PrbAddr：表格cos值地址
**						PibAddr：表格sin值地址
**						Len：数据长度
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_FFT(uint32_t SrbAddr ,uint32_t SibAddr , uint32_t PrbAddr , uint32_t PibAddr , uint16_t Len)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FFT_MODE;
	if(PrbAddr == 0U) MacCtrl0.bitMACCtrl0.FFT_TB_EN = (uint8_t)TB_EN_EN;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->DMA_SRBADR = (SrbAddr&0x1ffffU)/4U;
	DSP->DMA_SIBADR = (SibAddr&0x1ffffU)/4U;

	DSP->DMA_TRBADR = DSP->DMA_SRBADR;		/*  目标地址与源地址使用同一地址，节省RAM空间  */
	DSP->DMA_TIBADR = DSP->DMA_SIBADR;
	DSP->DMA_PRBADR = (PrbAddr&0x1ffffU)/4U;
	DSP->DMA_PIBADR = (PibAddr&0x1ffffU)/4U;

	DSP->DMA_LEN = (uint32_t)Len - 1U;
	DSP->FLG = 0x80U;
#ifdef RN202x_RN7326_SOC_V2
	DSP->MAC_CTL1 = (uint32_t)MAC_CTL_DMA_EN;
#else
	DSP->MAC_CTL2 = (uint32_t)MAC_CTL_DMA_EN;
#endif
	if(fnRegState_Detect((uint32_t)(&DSP->FLG) , 7U , 1U ,DSP_TIMEOUT) == ERN_ERROR)
	{
		return(ERN_ERROR);/* 等待转换完成 */
	}
	else
	{
		DSP->FLG = 0x80U;
		return(ERN_SUCCESS);
	}
}
/*********************************************************************************************************
** Function name:       LL_DSP_FpSqrt
** Descriptions:        浮点数均方根及反正切计算
** input parameters:    X_In:输入数据实部
**						Y_In：输入数据虚部
**						Amp_Out：输出sqrt值
**						Arctan_Out：Y/X反正切值
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_FpSqrt(uint32_t *X_In , uint32_t *Y_In , uint32_t *Amp_Out , uint32_t *Arctan_Out)
{
	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	DSP->CRD_XIN = *X_In;
	DSP->CRD_YIN = *Y_In;
	DSP->CRD_ANGLE = 0U;
	DSP->CRD_FLG = 0x01U;
	DSP->CRD_CTL = 0x1U;	/* cordic 向量模式启动信号 */
	if(fnRegState_Detect((uint32_t)(&DSP->CRD_FLG) , 0U , 1U ,DSP_TIMEOUT) == ERN_ERROR)
	{
		return(ERN_ERROR);/* 等待转换完成 */
	}
	else
	{
		DSP->CRD_FLG = 0x01U;
		*Amp_Out = DSP->CRD_AMP;
		if((*X_In & 0x80000000U) != 0U)
		{
			*Arctan_Out = (DSP->CRD_PHASE +  0x80000000U);
		}
		else
		{
			*Arctan_Out = DSP->CRD_PHASE;
		}
		return(ERN_SUCCESS);
	}
}
/*********************************************************************************************************
** Function name:       LL_DSP__FpMult
** Descriptions:        浮点数乘法
** input parameters:    In0:输入数据0
**						In1:输入数据1
**						Out:结果输出数据
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_FpMult(uint32_t In0 , uint32_t In1 , uint32_t *Out )
{
	uDSPMacCtrl0_TypeDef MacCtrl0;

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_MUL_MODE;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MUL_IN_FM = (uint8_t)MUL_IN_FM_Fp;			/*  输入为浮点模式  */
	MacCtrl0.bitMACCtrl0.MUL_OUT_FM = (uint8_t)MUL_OUT_DFM_Fp;		/*  输出为浮点模式  */
	MacCtrl0.bitMACCtrl0.I2F_PRE_EN = (uint8_t)I2F_PRE_24BIT;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = In0;
	DSP->MAC_IN1 = In1;
	DSP->FLG = 0x01U;
	*Out = DSP->MAC_OUT0;
	return(ERN_SUCCESS);
}
/*********************************************************************************************************
** Function name:       LL_DSP_FpAdd
** Descriptions:        浮点数加法
** input parameters:    In0:输入数据0
**						In1:输入数据1
**						Out:结果输出数据
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_FpAdd(uint32_t In0 , uint32_t In1 , uint32_t *Out )
{
	uDSPMacCtrl0_TypeDef MacCtrl0;

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_ADD_MODE;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = In0;
	DSP->MAC_IN1 = In1;
	*Out = DSP->MAC_OUT0;
	return(ERN_SUCCESS);
}
/*********************************************************************************************************
** Function name:       LL_DSP_FpSub
** Descriptions:        浮点数减法
** input parameters:    In0:输入数据0
**						In1:输入数据1
**						Out:结果输出数据
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_FpSub(uint32_t In0 , uint32_t In1 , uint32_t *Out )
{
	uDSPMacCtrl0_TypeDef MacCtrl0;

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_SUB_MODE;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = In0;
	DSP->MAC_IN1 = In1;
	*Out = DSP->MAC_OUT0;
	return(ERN_SUCCESS);
}
/*********************************************************************************************************
** Function name:       LL_DSP_Sin_Cos
** Descriptions:        旋转模式:Arctan(y/x) *2^31/pi
** input parameters:    In0:输入数据, 输入寄存器代表含义：(Angle/180)*2^31   Angle为角度值单位：度
**						CosOut  输出数据为 cos(Angle)*2^29/(2*PI)
**						SinOut  输出数据为 sin(Angle)*2^29/(2*PI)
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_Sin_Cos(uint32_t In0 , uint32_t *CosOut , uint32_t *SinOut )
{
	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;
	DSP->CRD_ANGLE = In0;
	DSP->CRD_CTL = 0x02U;	 /*  cordic旋转模式启动  */
	if(fnRegState_Detect((uint32_t)(&DSP->CRD_FLG) , 1U , 1U , DSP_TIMEOUT) == ERN_ERROR)
	{
		return(ERN_ERROR);/*  等待转换完成  */
	}
	else
	{
		DSP->CRD_FLG = 0x02U;
#ifdef RN831x_RN861x_MCU_V4
		*CosOut = DSP->CRD_COSINE;
		*SinOut = DSP->CRD_SINE;
		return(ERN_SUCCESS);
#else
		*CosOut = DSP->CRD_COS;
		*SinOut = DSP->CRD_SIN;
		return(ERN_SUCCESS);
#endif
	}
}
/*********************************************************************************************************
** Function name:       LL_DSP_IntDiv
** Descriptions:        整形数除法
** input parameters:    In0:被除数
**						In1:除数
**						Out:结果输出数据
**						Mul_N:放大倍数，放大2^Mul_N ,Mul_N<= 31;
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_IntDiv(uint32_t In0 , uint32_t In1 , uint32_t *Out , uint16_t Mul_N)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;
	
	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	DSP->MAC_CTL0 = 0U;
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)I2F_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = In1;
	DSP->DIV_IN1 = DSP->MAC_OUT0;
	DSP->MAC_IN0 = In0;
	DSP->DIV_IN0 = DSP->MAC_OUT0;
	DSP->FLG = 0x04U;

	/*  启动除法  */
#ifdef RN202x_RN7326_SOC_V2
	DSP->MAC_CTL1 = (uint32_t)DIV_KICK_EN;
#else
	DSP->MAC_CTL2 = (uint32_t)DIV_KICK_EN;
#endif
	if(fnRegState_Detect((uint32_t)(&DSP->FLG) , 2U , 1U ,DSP_TIMEOUT) == ERN_ERROR)
	{
		return(ERN_ERROR);/* 等待转换完成 */
	}

	DSP->FLG = 0x04U;
	*Out = DSP->DIV_OUT0;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)F2I_ONCE_MODE;
	MacCtrl0.bitMACCtrl0.F2I_MUL = (uint8_t)Mul_N;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = *Out;

	*Out = DSP->MAC_OUT0;
	return(ERN_SUCCESS);

}
/*********************************************************************************************************
** Function name:       LL_DSP_FlDiv
** Descriptions:        浮点数除法
** input parameters:    In0:被除数
**						In1:除数
**						Out:结果输出数据
**						Mul_N:放大倍数
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
 ErrorStatus LL_DSP_FlDiv(uint32_t In0 , uint32_t In1 , uint32_t *Out)
{

	uDSPMacCtrl0_TypeDef MacCtrl0;
	
	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->DIV_IN1 = In1;
	DSP->DIV_IN0 = In0;
	DSP->FLG = 0x04U;
	/*  启动除法  */
#ifdef RN202x_RN7326_SOC_V2
	DSP->MAC_CTL1 = (uint32_t)DIV_KICK_EN;
#else
	DSP->MAC_CTL2 = (uint32_t)DIV_KICK_EN;
#endif
	if(fnRegState_Detect((uint32_t)(&DSP->FLG) , 2U , 1U ,DSP_TIMEOUT) == ERN_ERROR)
	{
		return(ERN_ERROR);/* 等待转换完成 */
	}

	*Out = DSP->DIV_OUT0;
	return(ERN_SUCCESS);
}
/*********************************************************************************************************
** Function name:       LL_DSP_FIR
** Descriptions:        FIR运算
** input parameters:    InData:输入数据指针,包含前10个点数据
**						OutData:插值后数据指针
**						InLen:输入数据长度
**						OutLen:输出数据长度
**                                InData  和OutData不能使用同一个地址,否则会造成转换结果错误
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_FIR(uint32_t *InData , uint32_t *OutData , uint32_t *ParaDtat , uint16_t InLen , uint8_t FirOrder)
{
	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

#ifdef RN202x_RN7326_SOC_V2
	DSP->MAC_CTL2 = (((uint32_t)FirOrder<<8U)|(uint32_t)FIR_DMA_MODE);			/*  配置FIR阶数及步进  */
#else
	DSP->MAC_CTL1 = ((uint32_t)FirOrder<<4U);			/*  配置FIR阶数及步进  */
	DSP->MAC_CTL0 = (uint32_t)FIR_DMA_MODE;
#endif

	
	DSP->DMA_SRBADR = (uint32_t)((uint32_t*)InData)>>2U;
	DSP->DMA_TRBADR = (uint32_t)((uint32_t*)OutData)>>2U;
	DSP->DMA_PRBADR = (uint32_t)((uint32_t*)ParaDtat)>>2U;
	DSP->DMA_LEN = (uint32_t)InLen - 1U;
	DSP->FLG = 0x1002U;

#ifdef RN202x_RN7326_SOC_V2
	DSP->MAC_CTL1 = (uint32_t)MAC_CTL_DMA_EN;
#else
	DSP->MAC_CTL2 = (uint32_t)MAC_CTL_DMA_EN;
#endif

	if(fnRegState_Detect((uint32_t)(&DSP->FLG) , 12U , 1U ,DSP_TIMEOUT) == ERN_ERROR)
	{
		return(ERN_ERROR);/*  等待转换完成  */
	}

	DSP->FLG = 0x1002U;

    return(ERN_SUCCESS);
}
/*********************************************************************************************************
** Function name:       LL_DSP_HWCalc_4Wave
** Descriptions:        半波有效值运算
** input parameters:    InData:输入2周波128点数据
**						OutData:半波有效值，平方和值，如需得到真实半波，需开根号
**
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_HWCalc(uint32_t *InData , uint32_t *OutData , uint32_t *LastHwData)
{
	uint8_t i;
	uDSPMacCtrl0_TypeDef MacCtrl0;
	uint32_t HwDatatemp;
	
	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_MLAD_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;

	DSP->MAC_OUT0 = 0U;
	for(i=0U;i<32U;i++)
	{
		DSP->MAC_IN0 = InData[i];
		DSP->MAC_IN1 = InData[i];
		DSP->MAC_IN2 = DSP->MAC_OUT0;
	}
	HwDatatemp = DSP->MAC_OUT0;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = 0x3F800000U;              /*  *1+lasthwdata  */
	DSP->MAC_IN2 = *LastHwData;

	DSP->MAC_IN0 = DSP->MAC_OUT0;          /*  *1/64  */
	DSP->MAC_IN1 = 0x3c800000U;
	DSP->MAC_IN2 = 0U;
	OutData[0] = DSP->MAC_OUT0;
	*LastHwData = HwDatatemp;

	DSP->MAC_OUT0 = 0U;
	for(i=32U;i<64U;i++)
	{
		DSP->MAC_IN0 = InData[i];
		DSP->MAC_IN1 = InData[i];
		DSP->MAC_IN2 = DSP->MAC_OUT0;
	}
	HwDatatemp = DSP->MAC_OUT0;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = 0x3F800000U;              /*  *1+lasthwdata  */
	DSP->MAC_IN2 = *LastHwData;

	DSP->MAC_IN0 = DSP->MAC_OUT0;          /*  *1/64  */
	DSP->MAC_IN1 = 0x3c800000U;
	DSP->MAC_IN2 = 0U;
	OutData[1] = DSP->MAC_OUT0;
	*LastHwData = HwDatatemp;

	DSP->MAC_OUT0 = 0U;
	for(i=64U;i<96U;i++)
	{
		DSP->MAC_IN0 = InData[i];
		DSP->MAC_IN1 = InData[i];
		DSP->MAC_IN2 = DSP->MAC_OUT0;
	}
	HwDatatemp = DSP->MAC_OUT0;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = 0x3F800000U;              /*  *1+lasthwdata  */
	DSP->MAC_IN2 = *LastHwData;

	DSP->MAC_IN0 = DSP->MAC_OUT0;          /*  *1/64  */
	DSP->MAC_IN1 = 0x3c800000U;
	DSP->MAC_IN2 = 0U;
	OutData[2] = DSP->MAC_OUT0;
	*LastHwData = HwDatatemp;

	DSP->MAC_OUT0 = 0U;
	for(i=96U;i<128U;i++)
	{
		DSP->MAC_IN0 = InData[i];
		DSP->MAC_IN1 = InData[i];
		DSP->MAC_IN2 = DSP->MAC_OUT0;
	}
	HwDatatemp = DSP->MAC_OUT0;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = 0x3F800000U;              /*  *1+lasthwdata  */
	DSP->MAC_IN2 = *LastHwData;

	DSP->MAC_IN0 = DSP->MAC_OUT0;          /*  *1/64  */
	DSP->MAC_IN1 = 0x3c800000U;
	DSP->MAC_IN2 = 0U;
	OutData[3] = DSP->MAC_OUT0;
	*LastHwData = HwDatatemp;
	/*  浮点转整型  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)F2I_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = OutData[0];
	OutData[0] = DSP->MAC_OUT0;
	DSP->MAC_IN0 = OutData[1];
	OutData[1] = DSP->MAC_OUT0;
	DSP->MAC_IN0 = OutData[2];
	OutData[2] = DSP->MAC_OUT0;
	DSP->MAC_IN0 = OutData[3];
	OutData[3] = DSP->MAC_OUT0;
	return(ERN_SUCCESS);
}
/*********************************************************************************************************
** Function name:       LL_DSP_HWCalc_2Wave
** Descriptions:        半波有效值运算
** input parameters:    InData:输入1周波64点或128点数据
**						OutData:半波有效值，平方和值，如需得到真实半波，需开根号
**						Flag: 0: 1周波64点数据   1:1周波128点数据
**
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_HWCalc_2Wave(uint32_t *InData , uint32_t *OutData , uint32_t *LastHwData , uint8_t PointFlag)
{
	uint8_t i,j,k;
	uint8_t tstep;
	uDSPMacCtrl0_TypeDef MacCtrl0;
	uint32_t HwDatatemp;
	
	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_MLAD_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;

	if(PointFlag != 0U)
	{
		/*  128点1周波  */
		j = 64U;
		k = 128U;
		tstep = 2U;
	}
	else
	{
		/*  64点1周波  */
		j = 32U;
		k = 64U;
		tstep = 1U;
	}
	DSP->MAC_OUT0 = 0U;
	for(i=0U;i<j;i += tstep)
	{
		DSP->MAC_IN0 = InData[i];
		DSP->MAC_IN1 = InData[i];
		DSP->MAC_IN2 = DSP->MAC_OUT0;
	}
	HwDatatemp = DSP->MAC_OUT0;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = 0x3F800000U;              /*  *1+lasthwdata  */
	DSP->MAC_IN2 = *LastHwData;

	DSP->MAC_IN0 = DSP->MAC_OUT0;          /*  *1/64/128/128  */
	DSP->MAC_IN1 = 0x35800000U;
	DSP->MAC_IN2 = 0U;
	OutData[0] = DSP->MAC_OUT0;
	*LastHwData = HwDatatemp;

	DSP->MAC_OUT0 = 0U;
	for(;i<k;i += tstep)
	{
		DSP->MAC_IN0 = InData[i];
		DSP->MAC_IN1 = InData[i];
		DSP->MAC_IN2 = DSP->MAC_OUT0;

	}
	HwDatatemp = DSP->MAC_OUT0;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = 0x3F800000U;              /*  *1+lasthwdata  */
	DSP->MAC_IN2 = *LastHwData;

	DSP->MAC_IN0 = DSP->MAC_OUT0;          /*  *1/64/128/128  */
	DSP->MAC_IN1 = 0x35800000U;
	DSP->MAC_IN2 = 0U;
	OutData[1] = DSP->MAC_OUT0;
	*LastHwData = HwDatatemp;

	/*  浮点转整型  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)F2I_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = OutData[0];
	OutData[0] = DSP->MAC_OUT0;
	DSP->MAC_IN0 = OutData[1];
	OutData[1] = DSP->MAC_OUT0;
	return(ERN_SUCCESS);
}
/*********************************************************************************************************
** Function name:       LL_DSP_fBaseCalc
** Descriptions:        基波值运算
** input parameters:    InData: 输入基波值，为整形数，在开根号后输出
**						OutData_F:基波值，浮点数
**						OutData_Int:基波值，整型数
**						FWaveData:前4次基波值和，与当前值累加后求平均值
**
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_fBaseCalc(uint32_t *InData , uint32_t *OutData_F , uint32_t KMlu , uint8_t AveFlag)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;
	uint32_t fBaseTemp;

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;
	/*  转成浮点单元  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)I2F_ONCE_MODE;
	MacCtrl0.bitMACCtrl0.I2F_DIV = 0U;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = *InData;
	fBaseTemp = DSP->MAC_OUT0;


	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MUL_IN_FM = (uint8_t)MUL_IN_FM_Fp;
	MacCtrl0.bitMACCtrl0.MUL_OUT_FM = (uint8_t)MUL_OUT_DFM_Fp;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_MUL_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;

	DSP->MAC_IN0 = fBaseTemp;
	DSP->MAC_IN1 = 0x3F5BD95DU;				/*  此值代表4*P=4*0.607253 为计算谐波有效值需除2*sqrt(2),此值为 4*P/(2*sqrt(2)) = 2*0.607253/1.414  */
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = KMlu;

	if(AveFlag != 0U)
	{
		/*  求5点平均，将数据乘0.2 */
		DSP->MAC_IN0 = DSP->MAC_OUT0;
		DSP->MAC_IN1 = 0x3E4CCCCDU;		/* 0.2 */
	}
	fBaseTemp = DSP->MAC_OUT0;
	DSP->FLG = 0x200U;

	*OutData_F = fBaseTemp;
	return(ERN_SUCCESS);

}
/*********************************************************************************************************
** Function name:       LL_DSP_Lagrange
** Descriptions:        拉格朗日插值
** input parameters:    InData:输入数据指针
**						OutData:插值后数据指针
**						InLen:输入数据长度
**						OutLen:输出数据长度
**                            插值前InData和插值后OutData不能使用同一个地址,否则会造成转换结果错误
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
 ErrorStatus LL_DSP_Lagrange(uint32_t *InData , uint32_t *OutData , uint16_t InLen , uint16_t OutLen , uint32_t LastData, uint16_t Frequency , uint32_t FWaveRate)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;

	uint32_t f_step ;

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)I2F_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;

	DSP->MAC_IN0 = Frequency;  /*  f,频率，2位小数  */
	f_step = DSP->MAC_OUT0;

	DSP->MAC_IN0 = OutLen;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MUL_IN_FM = (uint8_t)MUL_IN_FM_Fp;
	MacCtrl0.bitMACCtrl0.MUL_OUT_FM = (uint8_t)MUL_OUT_DFM_Fp;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_MUL_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->FLG = 0x200U;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = f_step;

	/*  除法 Step = 3579545/2/128/(Frequency*OutLen)  */
	DSP->MAC_CTL0 = 0U;
	DSP->DIV_IN0 = FWaveRate;     /*  0x49AAAF9E为100*3579545/2/128浮点数,Frequency放大100  */
	DSP->DIV_IN1 = DSP->MAC_OUT0;
	DSP->FLG = 0x04U;
	/*  启动除法  */
#ifdef RN202x_RN7326_SOC_V2
	DSP->MAC_CTL1 = (uint32_t)DIV_KICK_EN;
#else
	DSP->MAC_CTL2 = (uint32_t)DIV_KICK_EN;
#endif

	if(fnRegState_Detect((uint32_t)(&DSP->FLG) , 2U , 1U ,DSP_TIMEOUT) == ERN_ERROR)
	{
		return(ERN_ERROR);/* 等待转换完成 */
	}

	DSP->FLG = 0x04U;
	f_step = DSP->DIV_OUT0;

	/*  启动加法 INTP_LOC = f_step + 1  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_ADD_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;

	DSP->MAC_IN0 = f_step;
	DSP->MAC_IN1 = 0x3F800000U;		/*  +1  */
	DSP->INTP_LOC = DSP->MAC_OUT0;

	DSP->FLG = 0x0201U;
#ifdef RN202x_RN7326_SOC_V2
	DSP->MAC_CTL1 = 0U;
	DSP->MAC_CTL2 = 0x10U;       /* 插值模式使用上周期的最后一个数据，使能信号  */
#else
	DSP->MAC_CTL2 = 0U;
	DSP->MAC_CTL1 = 0x01U;       /* 插值模式使用上周期的最后一个数据，使能信号  */
#endif
	DSP->MAC_CTL0 = (uint32_t)LAGRANGE_MODE;
	DSP->INTP_LEN=InLen;
	DSP->DMA_LEN=(uint32_t)OutLen-1U;

	DSP->INTP_STEP= f_step;
	DSP->MAC_IN5 = LastData;  /* 上一次值 */

	DSP->DMA_SRBADR=(uint32_t)((uint32_t*)InData)>>2;
	DSP->DMA_TRBADR=(uint32_t)((uint32_t*)OutData)>>2;

#ifdef RN202x_RN7326_SOC_V2
	DSP->MAC_CTL1 = (uint32_t)MAC_CTL_DMA_EN;
#else
	DSP->MAC_CTL2 = (uint32_t)MAC_CTL_DMA_EN;
#endif

	if(fnRegState_Detect((uint32_t)(&DSP->FLG) , 14U , 1U ,DSP_TIMEOUT) == ERN_ERROR)
	{
		return(ERN_ERROR);/* 等待转换完成 */
	}

	DSP->FLG = 0x4002U;

	return(ERN_SUCCESS);
}
/*********************************************************************************************************
** Function name:       LL_DSP_HPercent
** Descriptions:        整形数除法,用于计算各次谐波的有效值及含量
** input parameters:    HPercent:结构体数据，计算的谐波含量及有效值存放
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
/*  使用EMU基波有效值计算谐波有效值  */
ErrorStatus LL_DSP_HPercent(sDspMath_Percent_TypeDef *HPercent)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;
	uint32_t PercentTemp;
	
	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)I2F_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;

	DSP->MAC_IN0 = HPercent->fBase_Int;
	DSP->DIV_IN1 = DSP->MAC_OUT0;
	DSP->MAC_IN0 = HPercent->ftemp_Int;
	DSP->DIV_IN0 = DSP->MAC_OUT0;

	DSP->FLG = 0x04U;
	/*  启动除法  */
#ifdef RN202x_RN7326_SOC_V2
	DSP->MAC_CTL1 = (uint32_t)DIV_KICK_EN;
#else
	DSP->MAC_CTL2 = (uint32_t)DIV_KICK_EN;
#endif
	if(fnRegState_Detect((uint32_t)(&DSP->FLG) , 2U , 1U ,FFT_I2F_TIMEOUT) == ERN_ERROR)
	{
		return(ERN_ERROR);/* 等待转换完成 */
	}

	DSP->FLG = 0x04U;
	HPercent->Hpercent = DSP->DIV_OUT0;
	/*  启动乘法  */
	/*  FFT->MAC_IN0 = HPercent->nMul  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MUL_IN_FM = (uint8_t)MUL_IN_FM_Fp;
	MacCtrl0.bitMACCtrl0.MUL_OUT_FM = (uint8_t)MUL_OUT_DFM_Fp;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_MUL_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->FLG = 0x200U;
	DSP->MAC_IN0 = HPercent->nMul;
	DSP->MAC_IN1 = HPercent->Hpercent;

	DSP->FLG = 0x200U;
	PercentTemp = DSP->MAC_OUT0;
	/*  将计算的含量乘以10000的放大倍数  */
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = 0x461C4000U;						/*  10000的浮点数表示  */
	HPercent->Hpercent = DSP->MAC_OUT0;
	DSP->FLG = 0x200U;
	/* 启动乘法，计算谐波含量幅值 */
	DSP->MAC_IN0 = HPercent->EmuReg_Base;				/*  将EMU基波有效值转成浮点数  */
	DSP->MAC_IN1 = PercentTemp;

	DSP->FLG = 0x201U;
	HPercent->HRms = DSP->MAC_OUT0;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;			/*  浮点转整型数  */
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)F2I_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = HPercent->Hpercent;
	HPercent->Hpercent = DSP->MAC_OUT0;
	DSP->MAC_IN0 = HPercent->HRms;

	HPercent->HRms = DSP->MAC_OUT0;
	return(ERN_SUCCESS);
}
/*********************************************************************************************************
** Function name:       LL_DSP_InterHPercent
** Descriptions:        整形数除法,用于计算间谐波含量
** input parameters:    HPercent:结构体数据，计算的间谐波含量
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_InterHPercent(sDspMath_InterPercent_TypeDef *HPercent)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;
	
	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)I2F_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;

	DSP->MAC_IN0 = HPercent->fBase_Int;
	DSP->DIV_IN1 = DSP->MAC_OUT0;
	DSP->MAC_IN0 = HPercent->ftemp_Int;
	DSP->DIV_IN0 = DSP->MAC_OUT0;

	DSP->FLG = 0x04U;
	/*  启动除法  */
#ifdef RN202x_RN7326_SOC_V2
	DSP->MAC_CTL1 = (uint32_t)DIV_KICK_EN;
#else
	DSP->MAC_CTL2 = (uint32_t)DIV_KICK_EN;
#endif
	if(fnRegState_Detect((uint32_t)(&DSP->FLG) , 2U , 1U ,FFT_I2F_TIMEOUT) == ERN_ERROR)
	{
		return(ERN_ERROR);/* 等待转换完成 */
	}
	else
	{
		DSP->FLG = 0x04U;
		HPercent->Hpercent = DSP->DIV_OUT0;
		return(ERN_SUCCESS);
	}
}
/*********************************************************************************************************
** Function name:       LL_DSP_Isgrp
** Descriptions:        用于计算：1：启用，乘补偿系数，补偿系数放大10000倍
** output parameters:   NONE
** Returned value:      SUCCESS
** input parameters:    HPercent:结构体数据，计算间谐波子群含量
**						将5hz为单位的10次含量计算总的间谐波群含量
**						FftCoret: 间谐波补偿系数，系数*10000，以此达到计算出的含量为xx.xx%
**						Mode: 补偿模式是否启用，0：不启用，直接乘10000固定值
**												1：启用，乘补偿系数，补偿系数放大10000倍
** output parameters:   NONE
** Returned value:      SUCCESEE  or ERROR
*********************************************************************************************************/
ErrorStatus LL_DSP_Isgrp(sDspMath_InterPercent_TypeDef *HPercent ,  uint32_t *FftCoret , uint8_t Mode)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;
	uint32_t Isgrp;
	uint8_t i;

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MUL_IN_FM = (uint8_t)MUL_IN_FM_Fp;
	MacCtrl0.bitMACCtrl0.MUL_OUT_FM = (uint8_t)MUL_OUT_DFM_Fp;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_MUL_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->FLG = 0x200U;
	for(i=1;i<10U;i++)
	{

		 DSP->MAC_IN0 = HPercent->tPercent[i];
		if(Mode != 0U) DSP->MAC_IN1 = FftCoret[i];
		else DSP->MAC_IN1 = 0x461C4000U;						/* 0x461C4000为10000的浮点数表示 */
		HPercent->tPercent[i] = DSP->MAC_OUT0;
		DSP->MAC_IN0 = HPercent->tPercent[i];
		DSP->MAC_IN1 = HPercent->tPercent[i];
		HPercent->tPercent[i] = DSP->MAC_OUT0;

	}
	/*  启动加法  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_ADD_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	Isgrp = HPercent->tPercent[1];
	for(i=2U;i<10U;i++)
	{
		DSP->MAC_IN0 = HPercent->tPercent[i];
		DSP->MAC_IN1 = Isgrp;
		Isgrp = DSP->MAC_OUT0;
	}
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)F2I_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = Isgrp;
	Isgrp = DSP->MAC_OUT0;
	Isgrp = (uint32_t)sqrt((double)Isgrp);
	HPercent->Hpercent = Isgrp;
	return ERN_SUCCESS;
}
/*********************************************************************************************************
** Function name:       LL_DSP_CalcThd
** Descriptions:        计算谐波总含有率
** input parameters:    Src:各次谐波含有率  Num:计算谐波总次数
**						Dst:临时变量，存储含有率转浮点及自乘后的值
** output parameters:   NONE
** Returned value:      THD
*********************************************************************************************************/
uint32_t LL_DSP_CalcThd(uint32_t *Dst , uint8_t Num)
{
	uint8_t i;
	uDSPMacCtrl0_TypeDef MacCtrl0;
	uint32_t TempThd=0U;

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	/*  启动乘法  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MUL_IN_FM = (uint8_t)MUL_IN_FM_Fp;
	MacCtrl0.bitMACCtrl0.MUL_OUT_FM = (uint8_t)MUL_OUT_DFM_Fp;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_MUL_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->FLG = 0x200U;
	for(i=0;i<Num;i++)
	{
		DSP->MAC_IN0 = Dst[i];
		DSP->MAC_IN1 = Dst[i];
		Dst[i] = DSP->MAC_OUT0;
		DSP->FLG = 0x200U;
	}
	/*  启动加法  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_ADD_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	for(i=0U;i<Num;i++)
	{
		DSP->MAC_IN0 = Dst[i];
		DSP->MAC_IN1 = TempThd;
		TempThd = DSP->MAC_OUT0;
	}
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)F2I_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = TempThd;
	TempThd = DSP->MAC_OUT0;
	TempThd = (uint32_t)sqrt((double)TempThd);
	return((uint16_t)TempThd);
}
/*********************************************************************************************************
** Function name:       LL_DSP_CalcThdRms
** Descriptions:        计算谐波总含量
** input parameters:    Thd:总谐波含有率，整型数，单位XX.XX%
**						FRMS:基波有效值，浮点数
** output parameters:   NONE
** Returned value:      总谐波含有量
*********************************************************************************************************/
uint32_t LL_DSP_CalcThdRms(uint32_t Thd , uint32_t FRMS)
{
	uint32_t temp_thd;
	uDSPMacCtrl0_TypeDef MacCtrl0;

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)I2F_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;

	DSP->MAC_IN0 = Thd;
	temp_thd = DSP->MAC_OUT0;
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MUL_IN_FM = (uint8_t)MUL_IN_FM_Fp;
	MacCtrl0.bitMACCtrl0.MUL_OUT_FM = (uint8_t)MUL_OUT_DFM_Fp;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_MUL_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = FRMS;
	DSP->MAC_IN1 = 0x38d1b717U;					/*  0.0001  */
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = temp_thd;
	temp_thd = DSP->MAC_OUT0;
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)F2I_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = temp_thd;
	temp_thd = DSP->MAC_OUT0;
	return(temp_thd);
}
/*********************************************************************************************************
** Function name:       LL_DSP_HPf
** Descriptions:        谐波功率因数计算，返回各次谐波的功率因数
** input parameters:    HPf:结构体数据，计算的cos  sin值存放
** output parameters:   NONE
** Returned value:      当前功率因数值*10000,为整数值，浮点功率因数存在CosAngleU变量中
*********************************************************************************************************/
int16_t LL_DSP_HPf(sDspMath_HPf_Pw_TypeDef *HPf)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;
	int16_t AngleU,AngleI;

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	/*  整形转浮点数，并除2^29  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)I2F_ONCE_MODE;
	MacCtrl0.bitMACCtrl0.I2F_DIV = 29U;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = HPf->CosAngleU;
	HPf->CosAngleU = DSP->MAC_OUT0;
	DSP->MAC_IN0 = HPf->CosAngleI_Fu;
	HPf->CosAngleI_Fu = DSP->MAC_OUT0;
	DSP->MAC_IN0 = HPf->SinAngleU_Fi;
	HPf->SinAngleU_Fi = DSP->MAC_OUT0;
	DSP->MAC_IN0 = HPf->SinAngleI_Gp;
	HPf->SinAngleI_Gp = DSP->MAC_OUT0;
	/*  相角值转换  */
	DSP->MAC_IN0 = HPf->AngleU;
	HPf->AngleU = DSP->MAC_OUT0;
	DSP->MAC_IN0 = HPf->AngleI;
	HPf->AngleI = DSP->MAC_OUT0;
	/*  乘法CosAngleU*CosAngleI  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_MLAD_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = HPf->CosAngleU;
	DSP->MAC_IN1 = HPf->CosAngleI_Fu;
	DSP->MAC_IN2 = 0x0U;
	HPf->CosAngleU = DSP->MAC_OUT0;
	/*  SinAngleU*SinAngleI + CosAngleU*CosAngleI  */
	DSP->MAC_IN0 = HPf->SinAngleU_Fi;
	DSP->MAC_IN1 = HPf->SinAngleI_Gp;
	DSP->MAC_IN2 = HPf->CosAngleU;
	/* (SinAngleU*SinAngleI + CosAngleU*CosAngleI)*2PI*2PI  SinAngleU=SinAngleU/2PI */
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = 0x421DE9E6U;		/*  此值代表2*PI*2*PI=39.47842  */
	DSP->MAC_IN2 = 0x0U;
	HPf->CosAngleU = DSP->MAC_OUT0;
	/*  将功率因数*100，并转成整数  */
	DSP->MAC_IN0 = DSP->MAC_OUT0;;
	DSP->MAC_IN1 = 0x42C80000;		/*  此值代表100  */
	DSP->MAC_IN2 = 0x0;
	HPf->SinAngleU_Fi = DSP->MAC_OUT0;
	/*  计算相角，CRD_PHASE*pi/2^31*180/pi = CRD_PHASE*45/2^29  */
	DSP->MAC_IN0 = HPf->AngleU;		/* 参数HPf->AngleU为CRD_PHASE */
	DSP->MAC_IN1 = 0x43E10000U;		/*  此值代表450  */
	DSP->MAC_IN2 = 0x0U;
	HPf->AngleU = DSP->MAC_OUT0;
	DSP->MAC_IN0 = HPf->AngleI;
	DSP->MAC_IN1 = 0x43E10000U;		/*  此值代表450  */
	DSP->MAC_IN2 = 0x0U;
	HPf->AngleI = DSP->MAC_OUT0;

	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)F2I_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	/*  计算相角  */
	DSP->MAC_IN0 = HPf->AngleU;
	AngleU =(int16_t)(DSP->MAC_OUT0 & 0xffffU);
	if(AngleU < 0) AngleU = 3600 + AngleU;
	DSP->MAC_IN0 = HPf->AngleI;
	AngleI = (int16_t)(DSP->MAC_OUT0 & 0xffffU);
	if(AngleI < 0) AngleI = 3600 + AngleI;
	if(AngleI > AngleU)
	{
		HPf->AngleH_UI = (uint16_t)(AngleI - AngleU);
	}
	else
	{
		HPf->AngleH_UI = (uint16_t)(AngleI+ 3600 -AngleU);
	}
	/*  计算功率因数  */
	DSP->MAC_IN0 = HPf->SinAngleU_Fi;
	return((int16_t)DSP->MAC_OUT0);
}
/*********************************************************************************************************
** Function name:       LL_DSP_Pw
** Descriptions:        谐波功率计算，返回各次谐波的功率
** input parameters:    HPf:结构体数据，计算的FU,FI,cos存放;
** output parameters:   NONE
** Returned value:      当前功率值为整数值
*********************************************************************************************************/
int32_t LL_DSP_Pw(sDspMath_HPf_Pw_TypeDef *HPf)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;	
	
	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	/*  整形转浮点数  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)I2F_ONCE_MODE;
	MacCtrl0.bitMACCtrl0.I2F_PRE_EN = (uint8_t)I2F_PRE_32BIT;
	
#if	defined(RN202x_RN7326_SOC_V2)
	MacCtrl0.bitMACCtrl0.I2F_DIV = 27U;
#else
	MacCtrl0.bitMACCtrl0.I2F_DIV = 23U;
#endif
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = HPf->CosAngleI_Fu;
	HPf->CosAngleI_Fu = DSP->MAC_OUT0;


	DSP->MAC_IN0 = HPf->SinAngleU_Fi;
	HPf->SinAngleU_Fi = DSP->MAC_OUT0;
	
	if(HPf->SinAngleI_Gp != 0U)
	{
		MacCtrl0.MACCtrl0 = 0U;
		MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
		MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)I2F_ONCE_MODE;
		MacCtrl0.bitMACCtrl0.I2F_PRE_EN = (uint8_t)I2F_PRE_32BIT;

		if(HPf->SinAngleI_Gp > 0x7fffU) HPf->SinAngleI_Gp = (HPf->SinAngleI_Gp&0xffffU)|0xffff0000U;
		MacCtrl0.bitMACCtrl0.I2F_DIV = 15U;
		DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
		DSP->MAC_IN0 = HPf->SinAngleI_Gp;
		HPf->SinAngleI_Gp = DSP->MAC_OUT0;
		/* (1 +　GP) */
		MacCtrl0.MACCtrl0 = 0U;
		MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
		MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_ADD_MODE;
		DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
		DSP->MAC_IN0 = HPf->SinAngleI_Gp;
		DSP->MAC_IN1 = 0x3F800000U;
		HPf->SinAngleI_Gp = DSP->MAC_OUT0;
	}
	else
	{
		HPf->SinAngleI_Gp = 0x3F800000U;			/*  GP为0时，值置浮点数1  */
	}
	/*  乘法FU*FI*COSΦ*(1+GP)  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)FP_MUL_MODE;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MUL_IN_FM = (uint8_t)MUL_IN_FM_Fp;		/*  输入为浮点模式 */
	MacCtrl0.bitMACCtrl0.MUL_OUT_FM = (uint8_t)MUL_OUT_DFM_Fp;		/*  输出为浮点模式  */
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;

	DSP->MAC_IN0 = HPf->CosAngleI_Fu;
	DSP->MAC_IN1 = HPf->SinAngleU_Fi;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = HPf->CosAngleU;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = HPf->SinAngleI_Gp;
	HPf->SinAngleU_Fi = DSP->MAC_OUT0;
	/*  将功率*2^31，转成整数  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint16_t)F2I_ONCE_MODE;
	MacCtrl0.bitMACCtrl0.F2I_MUL = 31U;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = HPf->SinAngleU_Fi;
	return ((int32_t)DSP->MAC_OUT0);
}
#if(1)
/* 以下几个函数尚未测试验证，待验证后开放 */
/*********************************************************************************************************
** Function name:       LL_DSP_VectorCalc3P4
** Descriptions:        三相四线电压、电流正序、负序、零序向量计算
**						Angle 为寄存器值，转换角度公式为 Angle/2^24*360，计算cos,sin填入值应为 角度值/180*2^31
**						两者结合，填入寄存器值为 Angle*2^8
**						cos,sin值需 *2PI/2^29,为减少计算量，有效值在转浮点时需除2^29/2
**						RmsReg:0-3分别对应A,B,C相电压或电流寄存器值
**						VectorOut：0-3分别对应正序、负序、零序电压或电流寄存器值
** input parameters:
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
ErrorStatus LL_DSP_VectorCalc3P4(uint32_t *AngleReg , uint32_t *RmsReg , uint32_t *VectorOut)
{
	uint8_t i;
	uDSPMacCtrl0_TypeDef MacCtrl0;
	sVector3P4_TempData_TypeDef *pTemp;
	ErrorStatus	retVal = ERN_ERROR;

	uint32_t FftMathLibBuf[20];	
	uPtrContainer3P4_TypeDef ptrContainer;
	
	ptrContainer.uint32Ptr = FftMathLibBuf;
	pTemp = ptrContainer.vector3P4Ptr;  // 直接获取，无需强制转换

	retVal = LL_DSP_Int2Fp((uint32_t)&RmsReg[0] , (uint32_t)&RmsReg[0] , 3U , 0U , 0U);
	
	if(retVal == ERN_ERROR)
	{
		return (ERN_ERROR);
	}

	pTemp->Phs_A_Angle = ((16777216U - AngleReg[0])<<8);
	pTemp->Phs_B_Angle[2] = ((16777216U - AngleReg[1])<<8);
	pTemp->Phs_C_Angle[2] = ((16777216U - AngleReg[2])<<8);
	AngleReg[0] = pTemp->Phs_A_Angle;
	AngleReg[1] = pTemp->Phs_B_Angle[2];
	AngleReg[2] = pTemp->Phs_C_Angle[2];

	if(AngleReg[1]> 2863311530U) 			/*  2863311530 = 2^32-1-1431655765   B相夹角+120  */
	{
		pTemp->Phs_B_Angle[0] = AngleReg[1] -2863311531U;
	}
	else
	{
		pTemp->Phs_B_Angle[0] = AngleReg[1] + 1431655765U;
	}
	if(AngleReg[2]> 2863311530U) 			/*  2863311530 = 2^32-1-1431655765   B相夹角+120  */
	{
		pTemp->Phs_C_Angle[1] = AngleReg[2] -2863311531U;
	}
	else
	{
		pTemp->Phs_C_Angle[1] = AngleReg[2] + 1431655765U;
	}
	/*  B,C相夹角+240  */
	if(AngleReg[1]> 1431655764U) 			/*  1431655764 = 2^32-1-2863311531   B相夹角+240  */
	{
		pTemp->Phs_B_Angle[1] = AngleReg[1] -1431655765U;
	}
	else
	{
		pTemp->Phs_B_Angle[1] = AngleReg[1] + 2863311531U;
	}
	if(AngleReg[2]> 1431655764U) 			/*  C相夹角+240  */
	{
		pTemp->Phs_C_Angle[0] = AngleReg[2] -1431655765U;
	}
	else
	{
		pTemp->Phs_C_Angle[0] = AngleReg[2] + 2863311531U;
	}
	retVal = LL_DSP_Sin_Cos(pTemp->Phs_A_Angle , &pTemp->Cos_PhsA ,  &pTemp->Sin_PhsA);
	if(retVal == ERN_ERROR)
	{
		return (ERN_ERROR);
	}

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)I2F_ONCE_MODE;
	MacCtrl0.bitMACCtrl0.I2F_DIV = 28U;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = pTemp->Cos_PhsA;
	pTemp->Cos_PhsA = DSP->MAC_OUT0;
	DSP->MAC_IN0 = pTemp->Sin_PhsA;
	pTemp->Sin_PhsA = DSP->MAC_OUT0;
	
	for(i=0;i<3;i++)
	{
		retVal = LL_DSP_Sin_Cos(pTemp->Phs_B_Angle[i] , &pTemp->Cos_PhsB ,  &pTemp->Sin_PhsB);
		if(retVal == ERN_SUCCESS)
		{				
			retVal = LL_DSP_Sin_Cos(pTemp->Phs_C_Angle[i] , &pTemp->Cos_PhsC ,  &pTemp->Sin_PhsC);		
		}
		/*  将cos,sin的整数转浮点数  */
		MacCtrl0.MACCtrl0 = 0U;
		MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
		MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)I2F_ONCE_MODE;
		MacCtrl0.bitMACCtrl0.I2F_DIV = 28U;
		DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
		DSP->MAC_IN0 = pTemp->Cos_PhsB;
		pTemp->Cos_PhsB = DSP->MAC_OUT0;
		DSP->MAC_IN0 = pTemp->Sin_PhsB;
		pTemp->Sin_PhsB = DSP->MAC_OUT0;
		
		DSP->MAC_IN0 = pTemp->Cos_PhsC;
		pTemp->Cos_PhsC = DSP->MAC_OUT0;
		DSP->MAC_IN0 = pTemp->Sin_PhsC;
		pTemp->Sin_PhsC = DSP->MAC_OUT0;
		/*  计算 UaCos(Angle0)+Ubcos(Angle1) + UcCos(Angle2) “*/
		MacCtrl0.MACCtrl0 = 0U;
		MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MLAD_MODE;
		DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
		DSP->MAC_IN0 = pTemp->Cos_PhsA;
		DSP->MAC_IN1 = RmsReg[0];
		DSP->MAC_IN2 = 0x0U;

		DSP->MAC_IN0 = pTemp->Cos_PhsB;
		DSP->MAC_IN1 = RmsReg[1];
		DSP->MAC_IN2 = DSP->MAC_OUT0;

		DSP->MAC_IN0 = pTemp->Cos_PhsC;
		DSP->MAC_IN1 = RmsReg[2];
		DSP->MAC_IN2 = DSP->MAC_OUT0;
		pTemp->temp_a = DSP->MAC_OUT0;

		DSP->MAC_IN0 = pTemp->Sin_PhsA;
		DSP->MAC_IN1 = RmsReg[0];
		DSP->MAC_IN2 = 0x0U;

		DSP->MAC_IN0 = pTemp->Sin_PhsB;
		DSP->MAC_IN1 = RmsReg[1];
		DSP->MAC_IN2 = DSP->MAC_OUT0;

		DSP->MAC_IN0 = pTemp->Sin_PhsC;
		DSP->MAC_IN1 = RmsReg[2];
		DSP->MAC_IN2 = DSP->MAC_OUT0;
		pTemp->temp_b = DSP->MAC_OUT0;
		/*  将需开根号数据转成整型数据  */
		MacCtrl0.MACCtrl0 = 0U;
		MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
		MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)F2I_ONCE_MODE;
		DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
		DSP->MAC_IN0 = pTemp->temp_a;
		pTemp->temp_a = DSP->MAC_OUT0;
		DSP->MAC_IN0 = pTemp->temp_b;
		pTemp->temp_b = DSP->MAC_OUT0;
		if(retVal == ERN_SUCCESS)
		{
			retVal = LL_DSP_FpSqrt((uint32_t *)&pTemp->temp_a , (uint32_t *)&pTemp->temp_b , &pTemp->Cos_PhsC , &pTemp->Sin_PhsC);
		}
		
		VectorOut[i] = pTemp->Cos_PhsC;
		/*  将数据转成浮点数  */
		MacCtrl0.MACCtrl0 = 0U;
		MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
		MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)I2F_ONCE_MODE;
		DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
		DSP->MAC_IN0 = VectorOut[i];
		VectorOut[i] = DSP->MAC_OUT0;
		
		MacCtrl0.MACCtrl0 = 0U;
		MacCtrl0.bitMACCtrl0.MUL_IN_FM = (uint8_t)MUL_IN_FM_Fp;
		MacCtrl0.bitMACCtrl0.MUL_OUT_FM = (uint8_t)MUL_OUT_DFM_Int;
		MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MUL_MODE;
		DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
		DSP->MAC_IN0 = VectorOut[i];
		DSP->MAC_IN1 = 0x4022CB40U;						/*  PI*4*P/3 = 4*0.607253/3*pi = 2.543655418  */
		DSP->FLG = 0x200U;
		VectorOut[i] = DSP->MAC_OUT0;

		if(retVal == ERN_ERROR)
		{
			break;
		}
	}

	return (retVal);
	
}
/*********************************************************************************************************
** Function name:       LL_DSP_VectorCalc3P3_U
** Descriptions:        三相三线电压正序、负序、零序向量计算
**						Angle 为寄存器值，转换角度公式为 Angle/2^24*360 0-3分别代表 A,B,C的角度值
**						cos,sin值需 *2PI/2^29,为减少计算量，有效值在转浮点时需除2^29/2
**						RmsReg:0-3分别对应A,B,C相电压寄存器值
**						VectorOut：0-3分别对应正序、负序、零序电压寄存器值，三相三线无零序
** input parameters:
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
ErrorStatus LL_DSP_VectorCalc3P3_U(uint32_t *AngleReg , uint32_t *RmsReg , uint32_t *VectorOut)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;
	uint32_t Temp_a,Temp_b,Temp_U[3];
	float ftemp_a,ftemp_b;
	ErrorStatus	retVal = ERN_ERROR;
	uFloatUint32_TypeDef converter;
	
	retVal = LL_DSP_Int2Fp((uint32_t)&RmsReg[0] , (uint32_t)&RmsReg[0] , 3U , 0U , 0U);
	if(retVal == ERN_ERROR)
	{
		return (ERN_ERROR);
	}
	/*  计算Uac  */
	AngleReg[2] = ((16777216U -AngleReg[2])<<8);
	retVal = LL_DSP_Sin_Cos(AngleReg[2] , &Temp_a ,  &Temp_b);

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)I2F_ONCE_MODE;
	MacCtrl0.bitMACCtrl0.I2F_DIV = 28U;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = Temp_a;
	Temp_a = DSP->MAC_OUT0;
	DSP->MAC_IN0 = Temp_b;
	Temp_b = DSP->MAC_OUT0;
	
	/*  计算 Uac电压 = sqrt((Uab-Ucb*cos(Angle2))^2 + (-Ucb*sin(Angle2))^2)  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MUL_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = Temp_a;
	DSP->MAC_IN1 = RmsReg[2];
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = 0x40490FDBU;				/*  *PI  */
	Temp_a = DSP->MAC_OUT0;
	DSP->MAC_IN0 = Temp_b;
	DSP->MAC_IN1 = RmsReg[2];
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = 0x40490FDBU;				/*  *PI  */
	Temp_b = DSP->MAC_OUT0;
	
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_SUB_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = RmsReg[0];
	DSP->MAC_IN1 = Temp_a;
	Temp_a = DSP->MAC_OUT0;
	
	/*  将需开根号数据转成整型数据  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)F2I_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = Temp_a;
	Temp_a = DSP->MAC_OUT0;
	DSP->MAC_IN0 = Temp_b;
	Temp_b = DSP->MAC_OUT0;
	retVal = LL_DSP_FpSqrt((uint32_t *)&Temp_a , (uint32_t *)&Temp_b , &RmsReg[1] , &Temp_U[0]);
	if(retVal == ERN_ERROR)
	{
		return (ERN_ERROR);
	}
	/*  将数据转成浮点数  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)I2F_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = RmsReg[1];
	RmsReg[1] = DSP->MAC_OUT0;
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MUL_IN_FM = (uint8_t)MUL_IN_FM_Fp;
	MacCtrl0.bitMACCtrl0.MUL_OUT_FM = (uint8_t)MUL_OUT_DFM_Fp;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MUL_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = RmsReg[1];
	DSP->MAC_IN1 = 0x401B74EFU;						/*  4*P = 4*0.607253 = 2.429012  */
	DSP->FLG = 0x200U;
	RmsReg[1] = DSP->MAC_OUT0;
	
	/*  计算正序，负序电压 */
	/*  Temp_b = Uab+Ucb+Uac  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_ADD_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = RmsReg[0];
	DSP->MAC_IN1 = RmsReg[1];
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = RmsReg[2];
	Temp_b = DSP->MAC_OUT0;
	/*  Temp_b = Temp_b/2  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MUL_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = Temp_b;
	DSP->MAC_IN1 = 0x3F000000U;					/*  0.5 = 0x3f000000  */
	Temp_b = DSP->MAC_OUT0;
	/*  Temp_U[0] = Temp_b-Uab ; Temp_U[1] = Temp_b-Ucb ; Temp_U[2] = Temp_b-Uac  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_SUB_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = Temp_b;
	DSP->MAC_IN1 = RmsReg[0];
	Temp_U[0] = DSP->MAC_OUT0;
	DSP->MAC_IN0 = Temp_b;
	DSP->MAC_IN1 = RmsReg[1];
	Temp_U[1] = DSP->MAC_OUT0;
	DSP->MAC_IN0 = Temp_b;
	DSP->MAC_IN1 = RmsReg[2];
	Temp_U[2] = DSP->MAC_OUT0;
	/*  Temp_b = Temp_b*Temp_U[0]*Temp_U[1]*Temp_U[2]  */
	MacCtrl0.MACCtrl0 = 0;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MUL_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = Temp_b;
	DSP->MAC_IN1 = Temp_U[0];
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = Temp_U[1];
	Temp_b = DSP->MAC_OUT0;
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MUL_MODE;
	MacCtrl0.bitMACCtrl0.MUL_OUT_FM = (uint8_t)MUL_OUT_DFM_Fp;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MUL_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = Temp_b;
	DSP->MAC_IN1 = Temp_U[2];
	converter.u = DSP->MAC_OUT0;
	ftemp_a = sqrtf(converter.f);
	
	/*  计算 Uab^2+Ucb^2+Uac^2  */
	MacCtrl0.MACCtrl0 = 0U;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MLAD_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = RmsReg[0];
	DSP->MAC_IN1 = RmsReg[0];
	DSP->MAC_IN2 = 0x0U;
	
	DSP->MAC_IN0 = RmsReg[1];
	DSP->MAC_IN1 = RmsReg[1];
	DSP->MAC_IN2 = DSP->MAC_OUT0;
	DSP->MAC_IN0 = RmsReg[2];
	DSP->MAC_IN1 = RmsReg[2];
	DSP->MAC_IN2 = DSP->MAC_OUT0;
	converter.u = DSP->MAC_OUT0;
	ftemp_b = converter.f;
	ftemp_a = ftemp_a*1.1547f;
	ftemp_b = ftemp_b/6.0f;
	VectorOut[0] = (uint32_t)sqrt(ftemp_b+ftemp_a);
	VectorOut[1] = (uint32_t)sqrt(ftemp_b-ftemp_a);

	return (retVal);
}
/*********************************************************************************************************
** Function name:       LL_DSP_VectorCalc3P3_U
** Descriptions:        三相三线电流正序、负序、零序向量计算
**						Angle 为寄存器值，转换角度公式为 Angle/2^24*360 0-3分别代表 A,B,C的角度值
**						cos,sin值需 *2PI/2^29,为减少计算量，有效值在转浮点时需除2^29/2
**						RmsReg:0-3分别对应A,B,C相电流寄存器值
**						VectorOut：0-3分别对应正序、负序、零序电流寄存器值，三相三线无零序
** input parameters:
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
ErrorStatus LL_DSP_VectorCalc3P3_I(uint32_t *AngleReg , uint32_t *RmsReg , uint32_t *VectorOut)
{
	uint8_t i;
	ErrorStatus	retVal = ERN_ERROR;
	
	uDSPMacCtrl0_TypeDef MacCtrl0;
	sVector3P3_ITemp_TypeDef *pTemp;
    uint32_t FftMathLibBuf[20];		
	uPtrContainer3P3_TypeDef ptrContainer;
	
	ptrContainer.uint32Ptr = FftMathLibBuf;
	pTemp = ptrContainer.vector3P3Ptr;  // 直接获取，无需强制转换
	
	retVal = LL_DSP_Int2Fp((uint32_t)&RmsReg[0] , (uint32_t)&RmsReg[0] , 3 , 0 , 0);
	if(retVal == ERN_ERROR)
	{
		return (ERN_ERROR);
	}
	pTemp->Phs_C_Angle[0] = ((16777216U - AngleReg[2])<<8);
	AngleReg[2] = pTemp->Phs_C_Angle[0];
	
	if(AngleReg[2]> 2863311530U) 			/* 2863311530 = 2^32-1-1431655765   B相夹角+120  */
	{
		pTemp->Phs_C_Angle[1] = AngleReg[2] -2863311531U;			
	}
	else
	{
		pTemp->Phs_C_Angle[1] = AngleReg[2] + 1431655765U;
	}
	/*  C相夹角+240  */
	if(AngleReg[2]> 1431655764U) 			/*  C相夹角+240  */
	{
		pTemp->Phs_C_Angle[2] = AngleReg[2] -1431655765U;			
	}
	else
	{
		pTemp->Phs_C_Angle[2] = AngleReg[2] + 2863311531U;
	}
	retVal = LL_DSP_Sin_Cos(pTemp->Phs_C_Angle[0] , &pTemp->Cos_Phs[0] ,  &pTemp->Sin_Phs[0]);
	if(retVal == ERN_ERROR)
	{
		return (ERN_ERROR);
	}

	retVal = LL_DSP_Sin_Cos(pTemp->Phs_C_Angle[1] , &pTemp->Cos_Phs[1] ,  &pTemp->Sin_Phs[1]);
	if(retVal == ERN_ERROR)
	{
		return (ERN_ERROR);
	}

	retVal = LL_DSP_Sin_Cos(pTemp->Phs_C_Angle[2] , &pTemp->Cos_Phs[2] ,  &pTemp->Sin_Phs[2]);
	if(retVal == ERN_ERROR)
	{
		return (ERN_ERROR);
	}

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)I2F_ONCE_MODE;
	MacCtrl0.bitMACCtrl0.I2F_DIV = 28;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = pTemp->Cos_Phs[0];
	pTemp->Cos_Phs[0] = DSP->MAC_OUT0;
	DSP->MAC_IN0 = pTemp->Sin_Phs[0];
	pTemp->Sin_Phs[0] = DSP->MAC_OUT0;
	
	DSP->MAC_IN0 = pTemp->Cos_Phs[1];
	pTemp->Cos_Phs[1] = DSP->MAC_OUT0;
	DSP->MAC_IN0 = pTemp->Sin_Phs[1];
	pTemp->Sin_Phs[1] = DSP->MAC_OUT0;
	
	DSP->MAC_IN0 = pTemp->Cos_Phs[2];
	pTemp->Cos_Phs[2] = DSP->MAC_OUT0;
	DSP->MAC_IN0 = pTemp->Sin_Phs[2];
	pTemp->Sin_Phs[2] = DSP->MAC_OUT0;
	
	MacCtrl0.MACCtrl0 = 0;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MUL_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	for(i=0;i<3;i++)
	{
		DSP->MAC_IN0 = pTemp->Cos_Phs[i];
		DSP->MAC_IN1 = 0x40490FDBU;						/*  *pi = 3.141593  */
		DSP->FLG = 0x200;
		pTemp->Cos_Phs[i] = DSP->MAC_OUT0;
		
		DSP->MAC_IN0 = pTemp->Sin_Phs[i];
		DSP->MAC_IN1 = 0x40490FDBU;						/*  *pi = 3.141593  */
		DSP->FLG = 0x200U;
		pTemp->Sin_Phs[i] = DSP->MAC_OUT0;
	}
	/*  计算Iba = -Ia-Ic*cos(Angle)   ; Ibb = -Ic*sin(Angle)  */
	/*  负序 = sqrt(a^2+b^2)/3  */
	MacCtrl0.MACCtrl0 = 0;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MLAD_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = pTemp->Cos_Phs[0];
	DSP->MAC_IN1 = (RmsReg[2] | 0x80000000U);			/*  *-Ic  */
	DSP->MAC_IN2 = (RmsReg[0] | 0x80000000U);			/*  加 -Ia  */
	pTemp->temp_iba = DSP->MAC_OUT0;
	DSP->MAC_IN0 = pTemp->Sin_Phs[0];
	DSP->MAC_IN1 = (RmsReg[2] | 0x80000000U);
	DSP->MAC_IN2 = 0x0;
	pTemp->temp_ibb = DSP->MAC_OUT0;
	/*  a = (-0.866*Ibb-0.5*Iba) + Ia + Ic*Cos(Angle+240)  */
	DSP->MAC_IN0 = pTemp->temp_ibb;
	DSP->MAC_IN1 = 0xBF5DB22DU;							/*  -0.866  */
	DSP->MAC_IN2 = 0x0;
	DSP->MAC_IN0 = pTemp->temp_iba;
	DSP->MAC_IN1 = 0xBF000000U;							/*  -0.5  */
	DSP->MAC_IN2 = DSP->MAC_OUT0;
	DSP->MAC_IN0 = RmsReg[2];
	DSP->MAC_IN1 = pTemp->Cos_Phs[2];				
	DSP->MAC_IN2 = DSP->MAC_OUT0;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = 0x3F800000U;						  /*  *1  */					
	DSP->MAC_IN2 = RmsReg[0];
	pTemp->temp_a = DSP->MAC_OUT0;
	/*  b = (0.866*Iba-0.5*Ibb) + Ic*sin(Angle+240)  */
	DSP->MAC_IN0 = pTemp->temp_iba;
	DSP->MAC_IN1 = 0x3F5DB22DU;							/*  -0.866  */
	DSP->MAC_IN2 = 0x0;
	DSP->MAC_IN0 = pTemp->temp_ibb;
	DSP->MAC_IN1 = 0xBF000000U;							/*  -0.5  */
	DSP->MAC_IN2 = DSP->MAC_OUT0;
	DSP->MAC_IN0 = RmsReg[2];
	DSP->MAC_IN1 = pTemp->Sin_Phs[2];				
	DSP->MAC_IN2 = DSP->MAC_OUT0;
	pTemp->temp_b = DSP->MAC_OUT0;
	
	/*  将需开根号数据转成整型数据  */
	MacCtrl0.MACCtrl0 = 0;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)F2I_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = pTemp->temp_a;
	pTemp->temp_a = DSP->MAC_OUT0;
	DSP->MAC_IN0 = pTemp->temp_b;
	pTemp->temp_b = DSP->MAC_OUT0;
	retVal = LL_DSP_FpSqrt((uint32_t *)&pTemp->temp_a , (uint32_t *)&pTemp->temp_b , &VectorOut[0] , &VectorOut[2]);
	if(retVal == ERN_ERROR)
	{
		return (ERN_ERROR);
	}
	/*  正序计算  */
	MacCtrl0.MACCtrl0 = 0;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MLAD_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	/*  a = (0.866*Ibb-0.5*Iba) + Ia + Ic*Cos(Angle+120)  */
	DSP->MAC_IN0 = pTemp->temp_ibb;
	DSP->MAC_IN1 = 0x3F5DB22DU;							/*  -0.866  */
	DSP->MAC_IN2 = 0x0;
	DSP->MAC_IN0 = pTemp->temp_iba;
	DSP->MAC_IN1 = 0xBF000000U;							/*  -0.5  */
	DSP->MAC_IN2 = DSP->MAC_OUT0;
	DSP->MAC_IN0 = RmsReg[2];
	DSP->MAC_IN1 = pTemp->Cos_Phs[1];				
	DSP->MAC_IN2 = DSP->MAC_OUT0;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = 0x3F800000U;						  /*  *1  */					
	DSP->MAC_IN2 = RmsReg[0];
	pTemp->temp_a = DSP->MAC_OUT0;
	/* b = (-0.866*Iba-0.5*Ibb) + Ic*sin(Angle+120) */
	DSP->MAC_IN0 = pTemp->temp_iba;
	DSP->MAC_IN1 = 0xBF5DB22DU;							/*  -0.866  */
	DSP->MAC_IN2 = 0x0;
	DSP->MAC_IN0 = pTemp->temp_ibb;
	DSP->MAC_IN1 = 0xBF000000U;							/*  -0.5  */
	DSP->MAC_IN2 = DSP->MAC_OUT0;
	DSP->MAC_IN0 = RmsReg[2];
	DSP->MAC_IN1 = pTemp->Sin_Phs[1];				
	DSP->MAC_IN2 = DSP->MAC_OUT0;
	pTemp->temp_b = DSP->MAC_OUT0;
	/*  将需开根号数据转成整型数据  */
	MacCtrl0.MACCtrl0 = 0;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)F2I_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = pTemp->temp_a;
	pTemp->temp_a = DSP->MAC_OUT0;
	DSP->MAC_IN0 = pTemp->temp_b;
	pTemp->temp_b = DSP->MAC_OUT0;
	retVal = LL_DSP_FpSqrt((uint32_t *)&pTemp->temp_a , (uint32_t *)&pTemp->temp_b , &VectorOut[1] , &VectorOut[2]);
	if(retVal == ERN_ERROR)
	{
		return (ERN_ERROR);
	}

	VectorOut[2] = 0;
	
	/*  将数据转成浮点数  */
	MacCtrl0.MACCtrl0 = 0;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)I2F_ONCE_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = VectorOut[0];
	VectorOut[0] = DSP->MAC_OUT0;
	DSP->MAC_IN0 = VectorOut[1];
	VectorOut[1] = DSP->MAC_OUT0;
		
	MacCtrl0.MACCtrl0 = 0;
	MacCtrl0.bitMACCtrl0.MUL_IN_FM = (uint8_t)MUL_IN_FM_Fp;
	MacCtrl0.bitMACCtrl0.MUL_OUT_FM = (uint8_t)MUL_OUT_DFM_Int;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MUL_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = VectorOut[0];
	DSP->MAC_IN1 = 0x3F4F4694U;						/*  4*P/3 = 4*0.607253/3 = 0.809670667  */
	VectorOut[0] = DSP->MAC_OUT0;
	DSP->MAC_IN0 = VectorOut[1];
	DSP->MAC_IN1 = 0x3F4F4694U;	
	VectorOut[1] = DSP->MAC_OUT0;
	DSP->FLG = 0x200U;
	
	return (retVal);
}
/*********************************************************************************************************
** Function name:       LL_DSP_CalcPst
** Descriptions:        闪变短时闪变计算
** input parameters:    PstTemp:结构体数据，计算的各临时量存放
** output parameters:   NONE
** Returned value:      当前短时闪变值
*********************************************************************************************************/
/*
	p_0_1    = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 1 * S_DEPTH)  ));
	p_0_7    = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 7 * S_DEPTH)  ));
	p_1      = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 10 * S_DEPTH) ));
	p_1_5    = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 15 * S_DEPTH) ));
	p_2_2    = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 22 * S_DEPTH) ));
	p_3      = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 30 * S_DEPTH) ));
	p_4      = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 40 * S_DEPTH) ));
	p_6      = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 60 * S_DEPTH) ));
	p_8      = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 80 * S_DEPTH) ));
	p_10     = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 100 * S_DEPTH)));
	p_13     = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 130 * S_DEPTH)));
	p_17     = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 170 * S_DEPTH)));
	p_30     = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 300 * S_DEPTH)));
	p_50     = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 500 * S_DEPTH)));
	p_80     = s_min + adc_flicker[UA].s_step * ((float)(get_same_int(cpf[UA], S_CLASS, 800 * S_DEPTH)));
	p_50s_fx = (p_30 + p_50 + p_80) / 3.0;
	p_10s_fx = (p_6 + p_8 + p_10 + p_13 + p_17) / 5.0;
	p_3s_fx  = (p_2_2 + p_3 + p_4) / 3.0;
	p_1s_fx  = (p_0_7 + p_1 + p_1_5) / 3.0;
	p_0_1_fx = p_0_1;
	adc_flicker[UA].pst = (sqrt(0.0314 * p_0_1_fx + 0.0525 * p_1s_fx + 0.0657 * p_3s_fx + 0.28 * p_10s_fx + 0.08 * p_50s_fx) / 512.0) / 2.0;

*/
uint32_t LL_DSP_CalcPst(sFlick_TempData_TypeDef *PstTemp)
{
	uDSPMacCtrl0_TypeDef MacCtrl0;
	uint32_t *pDat;
	uint32_t p_1s_fx,p_3s_fx,p_10s_fx,p_50s_fx;
	uint8_t i;
	uFloatUint32_TypeDef PstErr;
	uFloatUint32_TypeDef converter;
	ErrorStatus	retVal = ERN_ERROR;

	pDat = &PstTemp->Pst_0_1;
	/*  先将数据转换为浮点数  */
	retVal = LL_DSP_Int2Fp((uint32_t)PstTemp ,(uint32_t)PstTemp , 17 , 0 , 0);
	if(retVal == ERN_ERROR)
	{
		PstErr.u = 0x00U;
		return (PstErr.u);
	}

	DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;

	MacCtrl0.MACCtrl0 = 0;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MLAD_MODE;
	/*  计算 P_0_1....P_80  */
	for(i=0;i<15;i++)
	{
		DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
		DSP->MAC_IN0 = *pDat;
		DSP->MAC_IN1 = PstTemp->S_Step_Temp;
		DSP->MAC_IN2 = PstTemp->S_Min_Temp;
		*pDat = DSP->MAC_OUT0;
		pDat++;
	}
	/*  浮点数加法  */
	MacCtrl0.MACCtrl0 = 0;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_ADD_MODE;
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = PstTemp->Pst_0_7;
	DSP->MAC_IN1 = PstTemp->Pst_1;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = PstTemp->Pst_1_5;
	p_1s_fx = DSP->MAC_OUT0;		/*  p_1s_fx = Pst_0_7 + Pst_1 + Pst_1_5  */
	
	DSP->MAC_IN0 = PstTemp->Pst_2_2;
	DSP->MAC_IN1 = PstTemp->Pst_3;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = PstTemp->Pst_4;
	p_3s_fx = DSP->MAC_OUT0;		/*  p_3s_fx = Pst_2_2 + Pst_3 + Pst_4  */
	
	DSP->MAC_IN0 = PstTemp->Pst_6;
	DSP->MAC_IN1 = PstTemp->Pst_8;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = PstTemp->Pst_10;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = PstTemp->Pst_13;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = PstTemp->Pst_17;
	p_10s_fx = DSP->MAC_OUT0;		/*' p_10s_fx = Pst_8 + Pst_8 + Pst_10 + Pst_13 + Pst_17 */
	
	DSP->MAC_IN0 = PstTemp->Pst_30;
	DSP->MAC_IN1 = PstTemp->Pst_50;
	DSP->MAC_IN0 = DSP->MAC_OUT0;
	DSP->MAC_IN1 = PstTemp->Pst_80;
	p_50s_fx = DSP->MAC_OUT0;		/*  p_50s_fx = Pst_30 + Pst_50 + Pst_80 */
	
	/*  浮点数乘加  */
	MacCtrl0.MACCtrl0 = 0;
	MacCtrl0.bitMACCtrl0.ROUND_MODE = (uint8_t)ROUND_MODE_NEAREST;
	MacCtrl0.bitMACCtrl0.MODE_SEL = (uint8_t)FP_MLAD_MODE;
	/*  计算0.0314 * p_0_1_fx + 0.0525 * p_1s_fx + 0.0657 * p_3s_fx + 0.28 * p_10s_fx + 0.08 * p_50s_fx  */
	DSP->MAC_CTL0 = MacCtrl0.MACCtrl0;
	DSP->MAC_IN0 = PstTemp->Pst_0_1;
	DSP->MAC_IN1 = 0x3D009D49U;	/* 0.0314 */
	DSP->MAC_IN2 = 0x0;
	
	DSP->MAC_IN0 = p_1s_fx;
	DSP->MAC_IN1 = 0x3C8F5C29U;	/* 0.0525/3 = 0.0175 */
	DSP->MAC_IN2 = DSP->MAC_OUT0;
	
	DSP->MAC_IN0 = p_3s_fx;
	DSP->MAC_IN1 = 0x3CB367A1U;	/* 0.0657/3 = 0.0219 */
	DSP->MAC_IN2 = DSP->MAC_OUT0;
	
	DSP->MAC_IN0 = p_10s_fx;
	DSP->MAC_IN1 = 0x3D656042U;	/* 0.28/5 = 0.056 */
	DSP->MAC_IN2 = DSP->MAC_OUT0;
	
	DSP->MAC_IN0 = p_50s_fx;
	DSP->MAC_IN1 = 0x3CDA740EU;	/* 0.08/3 = 0.026666667 */
	DSP->MAC_IN2 = DSP->MAC_OUT0;
	converter.u = DSP->MAC_OUT0;
	PstErr.f = sqrtf(converter.f)/10.24f;
	return(PstErr.u);
}
#endif
/*********************************************************************************************************
** Function name:     	LL_DSP_Disable
** Descriptions:       	关闭DSP
** input parameters:    None
** output parameters:       None
** Returned value:     None
*********************************************************************************************************/
void LL_DSP_Disable(void)
{
    DSP->MAC_CTL0 = 0x00U;
	DSP->MAC_CTL1 = 0x00U;
	DSP->MAC_CTL2 = 0x00U;
	DSP->FLG = 0x7FFFU;
}
#endif
#endif /* LL_DSP_MODULE_ENABLED*/

/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/

