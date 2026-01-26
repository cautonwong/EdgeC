/**
 ******************************************************************************
 * @file   rn_rv_hal_dsp.h
 * @author Renergy Software Team
 * @brief  This file contains all the functions prototypes for the HAL
 *         module driver.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RN_RV_HAL_DSP_H
#define __RN_RV_HAL_DSP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal_def.h"
#include "rn_rv_ll_dsp.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup DSP
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup DSP_Exported_Types DSP Exported Types
 * @{
 */

/**
 * @brief  DSP float-int uion
 */
typedef union{
    float         F32Data;
    unsigned int  U32Data;
} FloatIntUnion;

/**
 * @brief  DSP Interp parameter Structure definition
 */
typedef struct{
    uint32_t SrcAddr;
    uint32_t DstAddr;
    uint16_t SrcLen;
    uint16_t DstLen;
    uint32_t InterpStep;
    uint32_t CurLoc;
    uint32_t LastEn;
    uint32_t LastData;
}DSP_InterpParaTypeDef;

/**
 * @brief  DSP Interp type definition
 */
typedef enum{
    LAGRANGE = 0,
    LINEAR   = 1,
}DSP_InterpTypeDef;

/**
 * @brief  DSP FFT type definition
 */
typedef struct
{
	uint32_t InterTabEn;
	uint32_t SrbAddr;
	uint32_t SibAddr;
	uint32_t DstRAddr;
	uint32_t DstIAddr;
	uint32_t PrbAddr;
	uint32_t PibAddr;
	uint16_t Length;
}DSP_FFTParaTypeDef;

/**
 * @brief  DSP DMA STEP type definition
 */
typedef enum{
    ReadStep_1 = 0,
    ReadStep_2 = 1,
    ReadStep_3 = 2,
    ReadStep_4 = 3,
    ReadStep_5 = 4,
    ReadStep_6 = 5,
    ReadStep_7 = 6,
    ReadStep_8 = 7,
}DSP_DMAStepTypeDef;

/**
 * @brief  DSP FIR type definition
 */
typedef struct
{
	uint32_t SrcAddr;
	uint32_t DstAddr;
	uint32_t ParaAddr;
	uint16_t Length;
    uint8_t  FirOrder;
    DSP_DMAStepTypeDef DMAStep;
}DSP_FIRParaTypeDef;

/**
 * @brief  DSP IIR Once type definition
 */
typedef struct
{
	uint32_t *CoeffA;
	uint32_t *CoeffB;
	uint32_t *DataX;
	uint32_t *PreDataY;
    uint32_t *OutputY;
}DSP_IIROnceParaTypeDef;


/**
 * @brief  DSP IIR type definition
 */
typedef struct
{
	uint32_t *CoeffA;
	uint32_t *CoeffB;
	uint32_t *PreDataY;
	uint32_t *PreDataX;
	uint32_t SrcAddr;
	uint32_t DstAddr;
	uint16_t Length;
    DSP_DMAStepTypeDef DMAStep;
}DSP_IIRDMAParaTypeDef;

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup DSP_Exported_Constants DSP Exported Constants
 * @{
 */

/** @defgroup DSP_validbyte_define DSP valid byte define
 * @brief DSP valid byte mode
 * @{
 */
#define   DSP_VALID_4BYTE          LL_DSP_VALID_4BYTE        /*!< Select I2F input data 4 byte as available */
#define   DSP_VALID_3BYTE          LL_DSP_VALID_3BYTE        /*!< Select I2F input data low 3 byte as available */
#define   DSP_VALID_2BYTE          LL_DSP_VALID_2BYTE        /*!< Select I2F input data low 2 byte as available */
#define   DSP_VALID_1BYTE          LL_DSP_VALID_1BYTE        /*!< Select I2F input data low 1 byte as available */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup DSP_Exported_Functions
 * @{
 */

/** @addtogroup DSP_Exported_Functions_Group1
 * @{
 */
/* Int to float and float to int functions ****************************/
HAL_StatusTypeDef HAL_DSP_I2F_DMA(uint32_t DstAddr, uint32_t SrcAddr, uint32_t Length, uint32_t DivOrder, uint32_t ValidByte);
HAL_StatusTypeDef HAL_DSP_F2I_DMA(uint32_t DstAddr, uint32_t SrcAddr, uint32_t Length, uint32_t MulOrder);
uint32_t HAL_DSP_I2F_Once(uint32_t DataIn);
uint32_t HAL_DSP_F2I_Once(uint32_t DataIn);
uint32_t HAL_DSP_F2I_OnceRoundMod(float DataIn, uint32_t RoundMode);
float HAL_DSP_I2F_OnceDiv(uint32_t DataIn, uint32_t DivOrder, uint32_t ValidByte);
uint32_t HAL_DSP_F2I_OnceMul(float DataIn, uint32_t MulOrder);
/**
 * @}
 */

/** @addtogroup DSP_Exported_Functions_Group2
 * @{
 */
/* Four fundamental operations functions ****************************************************/
uint32_t HAL_DSP_FloatAdd(uint32_t DataIn0, uint32_t DataIn1);
uint32_t HAL_DSP_FloatSub(uint32_t DataIn0, uint32_t DataIn1);
uint32_t HAL_DSP_FloatMul(uint32_t DataIn0, uint32_t DataIn1);
HAL_StatusTypeDef HAL_DSP_FloatDiv(uint32_t Dividend, uint32_t Divisor, uint32_t *Quotient);
/**
 * @}
 */

/** @addtogroup DSP_Exported_Functions_Group3
 * @{
 */
/* Bit reverse and Cordic functions ****************************/
HAL_StatusTypeDef HAL_DSP_BitReverse(uint32_t DstAddr, uint32_t SrcAddr, uint32_t Length);
HAL_StatusTypeDef HAL_DSP_CalSinCosRegVal(uint32_t Radian, uint32_t *CosOut, uint32_t *SinOut);
HAL_StatusTypeDef HAL_DSP_CalAmpArctanRegVal(uint32_t DataX , uint32_t DataY , uint32_t *AmpOut , uint32_t *ArctanOut);
HAL_StatusTypeDef HAL_DSP_CalCos(uint32_t Radian, uint32_t *CosOut);
HAL_StatusTypeDef HAL_DSP_CalAmpArctan(uint32_t DataX, uint32_t DataY, uint32_t *AmpOut, uint32_t *ArctanOut, uint32_t MulOrder);
/**
 * @}
 */

/** @addtogroup DSP_Exported_Functions_Group4
 * @{
 */
/* Interp, FFT and filter functions ****************************/
HAL_StatusTypeDef HAL_DSP_Interp(DSP_InterpParaTypeDef *InterpPara, DSP_InterpTypeDef InterpType);
HAL_StatusTypeDef HAL_DSP_FFT(DSP_FFTParaTypeDef *FFTPara);
HAL_StatusTypeDef HAL_DSP_FIR_DMA(DSP_FIRParaTypeDef *FIRPara);
HAL_StatusTypeDef HAL_DSP_IIR_Once(DSP_IIROnceParaTypeDef *IIRPara);
HAL_StatusTypeDef HAL_DSP_IIR_DMA(DSP_IIRDMAParaTypeDef *IIRPara);
/**
 * @}
 */

/**
 * @}
 */

/* Private functions --------------------------------------------------------*/

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_HAL_DSP_H */

