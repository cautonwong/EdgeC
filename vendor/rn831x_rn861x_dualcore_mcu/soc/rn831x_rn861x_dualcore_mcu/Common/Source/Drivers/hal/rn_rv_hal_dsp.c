/**
 ******************************************************************************
 * @file   rn_rv_ll_dsp.c
 * @author Renergy Software Team
 * @brief  DSP LL module driver
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @defgroup DSP DSP
 * @brief DSP HAL module driver
 * @{
 */

#ifdef HAL_DSP_MODULE_ENABLED

/* Private typedef ----------------------------------------------------------*/
/** @defgroup DSP_Private_Typedef DSP Private Typedef
 * @{
 */
#define DSP_GENERAL_TIMEOUT  2000U
/**
 * @}
 */

/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/** @defgroup DSP_Private_Functions DSP Private Functions
 * @{
 */
static void DSP_DeInitCTL1(void);
static void DSP_DeInitCTL2(void);
/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup DSP_Exported_Functions DSP Exported Functions
 * @{
 */

/** @defgroup DSP_Exported_Functions_Group1 Int to float and float to int functions
 *  @brief    Int to float and float to int functions
 *
@verbatim
 ===============================================================================
              ##### Int to float and float to int functions #####
 ===============================================================================
  [..]

@endverbatim
 * @{
 */

/**
 * @brief  Int-to-Float by DMA.
 * @param  DstAddr   Destination address
 * @param  SrcAddr   Source address
 * @param  Length    I2F data length
 * @param  DivOrder  Output data divided by 2^(DivOrder)
 * @param  ValidByte Valid bytes in every 4 bytes, this parameter can be one of the following values:
 *         @arg @ref DSP_VALID_4BYTE
 *         @arg @ref DSP_VALID_3BYTE
 *         @arg @ref DSP_VALID_2BYTE
 *         @arg @ref DSP_VALID_1BYTE
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_DSP_I2F_DMA(uint32_t DstAddr, uint32_t SrcAddr, uint32_t Length, uint32_t DivOrder, uint32_t ValidByte)
{
    HAL_StatusTypeDef funStat = HAL_OK;
    uint32_t cnt = 0;
    uint32_t timeOut;
    /* Init CTL1 and CTL2 */
    DSP_DeInitCTL1();
    DSP_DeInitCTL2();
    /* Configure I2F ValidByte */
    LL_DSP_SetI2FPreprocess(DSP, ValidByte);
    /* Configure divide order of output */
    LL_DSP_SetI2FDivOrder(DSP, DivOrder);
    /* Configure I2F DMA mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_I2F_DMA);
    /* Configure DMA parameters, address should be Word-aligned */
    LL_DSP_SetDMASourceRealAddr(DSP, (SrcAddr & 0x1ffff) >> 2);
    LL_DSP_SetDMATargetRealAddr(DSP, (DstAddr & 0x1ffff) >> 2);
    LL_DSP_SetDMADataLength(DSP, (Length - 1));
    /* Enable DMA */
    LL_DSP_EnableDMA(DSP);

    /* Wait for DMA done */
    timeOut = (Length << 1);
    while(0 == LL_DSP_IsActiveFlag_DMA(DSP))
    {
        cnt++;
        if(cnt > timeOut)
        {
            funStat = HAL_ERROR;
            break;
        }
    }
    return funStat;
}

/**
 * @brief  Float-to-Int by DMA.
 * @param  DstAddr   Destination address
 * @param  SrcAddr   Source address
 * @param  Length    I2F data length
 * @param  MulOrder  Output data multiplied by 2^(MulOrder)
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_DSP_F2I_DMA(uint32_t DstAddr, uint32_t SrcAddr, uint32_t Length, uint32_t MulOrder)
{
    HAL_StatusTypeDef funStat = HAL_OK;
    uint32_t cnt = 0;
    uint32_t timeOut;
    /* Init CTL1 and CTL2 */
    DSP_DeInitCTL1();
    DSP_DeInitCTL2();
    /* Configure multiply order of output */
    LL_DSP_SetF2IMulOrder(DSP, MulOrder);
    /* Configure F2I DMA mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_F2I_DMA);
    /* Configure DMA parameters, address should be Word-aligned */
    LL_DSP_SetDMASourceRealAddr(DSP, (SrcAddr & 0x1ffff) >> 2);
    LL_DSP_SetDMATargetRealAddr(DSP, (DstAddr & 0x1ffff) >> 2);
    LL_DSP_SetDMADataLength(DSP, (Length - 1));
    /* Enable DMA */
    LL_DSP_EnableDMA(DSP);

    /* Wait for DMA done */
    timeOut = (Length << 1);
    while(0 == LL_DSP_IsActiveFlag_DMA(DSP))
    {
        cnt++;
        if(cnt > timeOut)
        {
            funStat = HAL_ERROR;
            break;
        }
    }
    return funStat;
}

/**
 * @brief  Int-to-Float once.
 * @param  DataIn   Input integer data
 * @retval Return floating-point data
 */
uint32_t HAL_DSP_I2F_Once(uint32_t DataIn)
{
    /* Configure I2F once mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_I2F_ONCE);
    /* Configure input data */
    LL_DSP_SetInput0(DSP, DataIn);
    /* Get result */
    return LL_DSP_GetOutput0(DSP);
}

/**
 * @brief  Float-to-Int once.
 * @param  DataIn   Input float data
 * @retval Return integer data
 */
uint32_t HAL_DSP_F2I_Once(uint32_t DataIn)
{
    /* Configure F2T once mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_F2I_ONCE);
    /* Configure input data */
    LL_DSP_SetInput0(DSP, DataIn);
    /* Get result */
    return LL_DSP_GetOutput0(DSP);
}

/**
 * @brief  Float-to-Int once with configurable RoundMode.
 * @param  DataIn   Input float data
 * @param  RoundMod This parameter can be one of the following values:
 *         @arg @ref LL_DSP_ROUNDMODE_NEAREST
 *         @arg @ref LL_DSP_ROUNDMODE_ZERO
 *         @arg @ref LL_DSP_ROUNDMODE_POS_INF
 *         @arg @ref LL_DSP_ROUNDMODE_NEG_INF
 *         @arg @ref LL_DSP_ROUNDMODE_UP
 *         @arg @ref LL_DSP_ROUNDMODE_AWAY
 * @retval Result integer data
 */
uint32_t HAL_DSP_F2I_OnceRoundMod(float DataIn, uint32_t RoundMode)
{
    FloatIntUnion dataUnion;
    /* Configure F2T once mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_F2I_ONCE);
    /* Configure Round Mode */
    LL_DSP_SetRoundMode(DSP, RoundMode);
    /* Configure input data */
    dataUnion.F32Data = DataIn;
    LL_DSP_SetInput0(DSP, dataUnion.U32Data);
    /* Get result */
    return LL_DSP_GetOutput0(DSP);
}

/**
 * @brief  Int-to-Float once with configurable divide order and valid bytes.
 * @param  DataIn   Input integer data
 * @param  DivOrder  Output data divided by 2^(DivOrder)
 * @param  ValidByte Valid bytes in every 4 bytes, this parameter can be one of the following values:
 *         @arg @ref DSP_VALID_4BYTE
 *         @arg @ref DSP_VALID_3BYTE
 *         @arg @ref DSP_VALID_2BYTE
 *         @arg @ref DSP_VALID_1BYTE
 * @retval Return floating-point data
 */
float HAL_DSP_I2F_OnceDiv(uint32_t DataIn, uint32_t DivOrder, uint32_t ValidByte)
{
    FloatIntUnion dataUnion;
    /* Configure I2F once mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_I2F_ONCE);
    /* Configure I2F ValidByte */
    LL_DSP_SetI2FPreprocess(DSP, ValidByte);
    /* Configure DivOrder */
    LL_DSP_SetI2FDivOrder(DSP, DivOrder);
    /* Configure input data */
    LL_DSP_SetInput0(DSP, DataIn);
    /* Get result */
    dataUnion.U32Data = LL_DSP_GetOutput0(DSP);
    return dataUnion.F32Data;
}

/**
 * @brief  Float-to-Int once with configurable muliplication order.
 * @param  DataIn   Input float data
 * @param  MulOrder Output data multiplied by 2^(MulOrder)
 * @retval Return integer data
 */
uint32_t HAL_DSP_F2I_OnceMul(float DataIn, uint32_t MulOrder)
{
    FloatIntUnion dataUnion;
    /* Configure F2T once mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_F2I_ONCE);
    /* Configure multiply order of output */
    LL_DSP_SetF2IMulOrder(DSP, MulOrder);
    /* Configure input data */
    dataUnion.F32Data = DataIn;
    LL_DSP_SetInput0(DSP, dataUnion.U32Data);
    /* Get result */
    return LL_DSP_GetOutput0(DSP);
}

/**
 * @}
 */

/** @defgroup DSP_Exported_Functions_Group2 Four fundamental operations functions
 *  @brief   Screen operation
 *
@verbatim
 ===============================================================================
                       ##### Four fundamental operations functions #####
 ===============================================================================

@endverbatim
 * @{
 */

/**
 * @brief  Floating-point addition function.
 * @param  DataIn0 Input data0
 * @param  DataIn1 Input data1
 * @retval Return floating-point result
 */
uint32_t HAL_DSP_FloatAdd(uint32_t DataIn0, uint32_t DataIn1)
{
    /* Configure floating addition mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_FADD);
    /* Configure input data */
    LL_DSP_SetInput0(DSP, DataIn0);
    LL_DSP_SetInput1(DSP, DataIn1);
    /* Get result */
    return LL_DSP_GetOutput0(DSP);
}

/**
 * @brief  Floating-point subtraction function.
 * @param  DataIn0 Minuend data
 * @param  DataIn1 Subtrahend data
 * @retval Return floating-point result
 */
uint32_t HAL_DSP_FloatSub(uint32_t DataIn0, uint32_t DataIn1)
{
    /* Configure floating subtraction mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_FSUB);
    /* Configure input data */
    LL_DSP_SetInput0(DSP, DataIn0);
    LL_DSP_SetInput1(DSP, DataIn1);
    /* Get result */
    return LL_DSP_GetOutput0(DSP);
}

/**
 * @brief  Floating-point multiplication function.
 * @param  DataIn0 Input data0
 * @param  DataIn1 Input data1
 * @retval Return floating-point result
 */
uint32_t HAL_DSP_FloatMul(uint32_t DataIn0, uint32_t DataIn1)
{
    /* Configure floating multiplication mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_FMUL);
    /* Configure input data */
    LL_DSP_SetInput0(DSP, DataIn0);
    LL_DSP_SetInput1(DSP, DataIn1);
    /* Get result */
    return LL_DSP_GetOutput0(DSP);
}

/**
 * @brief  Floating-point Division function.
 * @param  Dividend Dividend data
 * @param  Divisor  Divisor  data
 * @param  Quotient Pointer to quotient
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_DSP_FloatDiv(uint32_t Dividend, uint32_t Divisor, uint32_t *Quotient)
{
    HAL_StatusTypeDef funStat = HAL_OK;
    uint32_t cnt = 0;
    /* Init CTL1 and CTL2 */
    DSP_DeInitCTL1();
    DSP_DeInitCTL2();
    /* CLear all flag */
    LL_DSP_ClearFlag(DSP, 0x7FFFU);

    /* Configure input data of Division */
    LL_DSP_SetDivInput0(DSP, Dividend);
    LL_DSP_SetDivInput1(DSP, Divisor);
    /* Enable Dividsion calculate */
    LL_DSP_EnableFpDiv(DSP);
    /* Wait for Divide done */
    while(0 == LL_DSP_IsActiveFlag_DIV(DSP))
    {
        cnt++;
        if(cnt > DSP_GENERAL_TIMEOUT)
        {
            funStat = HAL_ERROR;
            break;
        }
    }
    /* Get result */
    *Quotient = LL_DSP_GetDivOutput0(DSP);
    return funStat;
}

/**
 * @}
 */

/** @defgroup DSP_Exported_Functions_Group3 bit reverse and Cordic functions
 *  @brief    Bit reverse and Cordic function
 *
@verbatim
 ===============================================================================
            ##### Bit reverse and Cordic functions #####
 ===============================================================================

@endverbatim
 * @{
 */

/**
 * @brief  Bit reverse.
 * @param  DstAddr   Destination address
 * @param  SrcAddr   Source address
 * @param  Length    Data length
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_DSP_BitReverse(uint32_t DstAddr, uint32_t SrcAddr, uint32_t Length)
{
    HAL_StatusTypeDef funStat = HAL_OK;
    uint32_t cnt = 0;
    uint32_t timeOut;
    /* Init CTL1 and CTL2 */
    DSP_DeInitCTL1();
    DSP_DeInitCTL2();
    /* Configure bit reverse DMA mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_BITREVERSE);
    /* Configure DMA parameters, address should be Word-aligned */
    LL_DSP_SetDMASourceRealAddr(DSP, (SrcAddr & 0x1ffff) >> 2);
    LL_DSP_SetDMATargetRealAddr(DSP, (DstAddr & 0x1ffff) >> 2);
    LL_DSP_SetDMADataLength(DSP, (Length - 1));
    /* Enable DMA */
    LL_DSP_EnableDMA(DSP);

    /* Wait for DMA done */
    timeOut = (Length & 0xFFFFU << 2);
    while(0 == LL_DSP_IsActiveFlag_BITREV(DSP))
    {
        cnt++;
        if(cnt > timeOut)
        {
            funStat = HAL_ERROR;
            break;
        }
    }
    return funStat;
}

/**
 * @brief  Calculate sine and cosine.
 * @note   The result is register value that has not been convert to actual value.
 * @param  Radian  Input radian, register's value = Input value*pi/(2^31),
 *                 Radian = 2^31 represent pi, Radian = -2^31 represent -pi.
 * @param  CosOut Output cosine, cos(Angle)*(2^29)/(2*PI)
 * @param  SinOut Output sine, sin(Angle)*(2^29)/(2*PI)
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_DSP_CalSinCosRegVal(uint32_t Radian, uint32_t *CosOut, uint32_t *SinOut)
{
    HAL_StatusTypeDef funStat = HAL_OK;
    uint32_t cnt = 0;
    /* Clear flag */
    LL_DSP_ClearFlag_ROTATION(DSP);
    /* Configure input angle */
    LL_DSP_SetRotationAngle(DSP, Radian);
    /* Enable calculate */
    LL_DSP_EnableRotation(DSP);
    /* Wait for calculate done */
    while(0 == LL_DSP_IsActiveFlag_ROTATION(DSP))
    {
        cnt++;
        if(cnt > DSP_GENERAL_TIMEOUT)
        {
            funStat = HAL_ERROR;
            break;
        }
    }

    *CosOut = LL_DSP_GetRotationCosine(DSP);
    *SinOut = LL_DSP_GetRotationSine(DSP);
    return funStat;
}

/**
 * @brief  Calculate amplitude and arctangent.
 * @note   The result is register value that has not been convert to actual value.
 * @param  DataX  Pointer to Input data X
 * @param  DataY  Pointer to Input data Y
 * @param  AmpOut Pointer to output amplitude, sqrt(X^2 + Y^2) /4*P
 * @param  ArctanOut Pointer to output arctangent, arctan(Y/X)*2^31/pi
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_DSP_CalAmpArctanRegVal(uint32_t DataX, uint32_t DataY , uint32_t *AmpOut , uint32_t *ArctanOut)
{
    HAL_StatusTypeDef funStat = HAL_OK;
    uint32_t cnt = 0;

    /* Clear flag */
    LL_DSP_ClearFlag_VECTOR(DSP);
    /* Configure input data */
    LL_DSP_SetVectorInputX(DSP, DataX);
    LL_DSP_SetVectorInputY(DSP, DataY);
    /* Enable vector mode */
    LL_DSP_EnableVector(DSP);
    /* Wait for calculate done */
    while(0 == LL_DSP_IsActiveFlag_VECTOR(DSP))
    {
        cnt++;
        if(cnt > DSP_GENERAL_TIMEOUT)
        {
            funStat = HAL_ERROR;
            break;
        }
    }

    /* Get AmpOut */
    *AmpOut = LL_DSP_GetVectorAmplitude(DSP);
    /* Get ArctanOut, if real part of input data is nagtive, the sign of ArctanOut should be reversed */
    if(0 != (DataX & 0x80000000))
    {
        *ArctanOut = 0x80000000 + LL_DSP_GetRotationPhase(DSP);
    }
    else
    {
        *ArctanOut = LL_DSP_GetRotationPhase(DSP);
    }
    return funStat;
}

/**
 * @brief  Calculate cosine function.
 * @note   The result is converted to actual value.
 * @param  Radian  Input radian, register's value = Input value*pi/(2^31),
 *                 Radian = 2^31 represent pi, Radian = -2^31 represent -pi.
 * @param  CosOut Pointer to output cos(Rad)
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_DSP_CalCos(uint32_t Radian, uint32_t *CosOut)
{
    HAL_StatusTypeDef funStat = HAL_OK;
    uint32_t regSinOut, regCosOut;
    uint32_t calData, tmpData1;
    FloatIntUnion constBit31, const1DivPi, const1Div2Pi, constPi, const2Pi, const1DivBit29;

    const float fConstBit31      = 2147483648.0f;
    const float fConst1DivPi     = 0.318309f;
    const float fConst1Div2Pi    = 0.159154f;
    const float fConstPi         = 3.141592f;
    const float fConst2Pi        = 6.283185f;
    const float fConst1DivBit29  = 1.86E-09f;

    constBit31.F32Data     = fConstBit31;
    const1DivPi.F32Data    = fConst1DivPi;
    const1Div2Pi.F32Data   = fConst1Div2Pi;
    constPi.F32Data        = fConstPi;
    const2Pi.F32Data       = fConst2Pi;
    const1DivBit29.F32Data = fConst1DivBit29;

    /* rad / (2*pi) */
    calData = HAL_DSP_FloatMul(Radian, const1Div2Pi.U32Data);

    calData = HAL_DSP_F2I_Once(calData);
    calData = HAL_DSP_I2F_Once(calData);
    /* multiply 2pi */
    calData = HAL_DSP_FloatMul(calData, const2Pi.U32Data);
    /* rad - temp0 - pi */
    calData = HAL_DSP_FloatSub(Radian, calData);
    tmpData1 = HAL_DSP_FloatSub(calData, constPi.U32Data);

    /* if tmpData1 > 0 */
    if(0 == ((tmpData1 >> 31) & 0x1U))
    {
        /* temp0 - 2pi */
        calData = HAL_DSP_FloatSub(calData, const2Pi.U32Data);
    }

    /* temp0 / pi * 2^31 */
    calData = HAL_DSP_FloatMul(calData, const1DivPi.U32Data);
    calData = HAL_DSP_FloatMul(calData, constBit31.U32Data);

    calData = HAL_DSP_F2I_Once(calData);
    if(HAL_OK != HAL_DSP_CalSinCosRegVal(calData, &regCosOut, &regSinOut))
    {
        return HAL_ERROR;
    }
    calData = HAL_DSP_I2F_Once(regCosOut);
    /* tmp0/2^29 * 2pi */
    calData = HAL_DSP_FloatMul(calData, const1DivBit29.U32Data);
    calData = HAL_DSP_FloatMul(calData, const2Pi.U32Data);
    *CosOut = calData;

    return funStat;
}


/**
 * @brief  Calculate amplitude and arctangent.
 * @note   The result is converted to actual value.
 * @param  DataX  Input data X
 * @param  DataY  Input data Y
 * @param  AmpOut Pointer to output amplitude, sqrt(X^2 + Y^2)
 * @param  ArctanOut Pointer to output arctangent, arctan(Y/X)
 * @param  MulOrder  Float-to-Int 2^(MulOrder)
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_DSP_CalAmpArctan(uint32_t DataX, uint32_t DataY, uint32_t *AmpOut, uint32_t *ArctanOut, uint32_t MulOrder)
{
    HAL_StatusTypeDef funStat = HAL_OK;
    uint32_t ampProc, phaseProc, uIntX, uIntY;
    uint32_t ampReg, phaseReg;
    FloatIntUnion const4P, constPi;
    const float fConst4P = 2.429f;
    const float fConstPi = 3.141592f;

    const4P.F32Data = fConst4P;
    constPi.F32Data = fConstPi;

    if(MulOrder > 63)
    {
        return HAL_ERROR;
    }

    /* Float-to-Int */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_F2I_ONCE);
    LL_DSP_SetF2IMulOrder(DSP, MulOrder);
    LL_DSP_SetInput0(DSP, DataX);
    uIntX = LL_DSP_GetOutput0(DSP);
    LL_DSP_SetInput0(DSP, DataY);
    uIntY = LL_DSP_GetOutput0(DSP);

    /* calculate amplitude and arctangent register result */
    funStat = HAL_DSP_CalAmpArctanRegVal(uIntX, uIntY, &ampProc, &phaseProc);
    /* amplitude register value change to real value, AmpOut = ampReg*4*P(P=0.60725)*/
    ampReg = HAL_DSP_I2F_OnceDiv(ampProc, MulOrder, DSP_VALID_4BYTE);
    *AmpOut = HAL_DSP_FloatMul(ampReg, const4P.U32Data);

    /* arctan register value change to real value, ArctanOut = phaseReg/2^31*PI */
    phaseReg = HAL_DSP_I2F_OnceDiv(phaseProc, 31, DSP_VALID_4BYTE);
    *ArctanOut= HAL_DSP_FloatMul(phaseReg, constPi.U32Data);

    return funStat;
}

/**
 * @}
 */




/** @defgroup DSP_Exported_Functions_Group4 Interp, FFT and filter functions
 *  @brief   Interp, FFT, FIR and IIR functions
 *
@verbatim
 ===============================================================================
                       ##### Interpulation, FFT and filter functions #####
 ===============================================================================

@endverbatim
 * @{
 */

HAL_StatusTypeDef HAL_DSP_Interp(DSP_InterpParaTypeDef *InterpPara, DSP_InterpTypeDef InterpType)
{
    HAL_StatusTypeDef funStat = HAL_OK;
    uint32_t timeOut, cnt = 0;

    /* Disable DMA */
    LL_DSP_DisableDMA(DSP);
    /* Config interp mode */
    if(InterpType == LAGRANGE){
        LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_LAGRANGE);
    }else{
        LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_LINEAR);
    }
    /* Config DMA address */
    LL_DSP_SetDMASourceRealAddr(DSP, (InterpPara->SrcAddr >> 2));
    LL_DSP_SetDMATargetRealAddr(DSP, (InterpPara->DstAddr >> 2));
    /* Confirm whether to use the last data from the previous cycle  */
    if(InterpPara->LastEn == 0U){
        LL_DSP_DisableInterpLastData(DSP);
    }else{
        LL_DSP_SetInput5(DSP, InterpPara->LastData);
        LL_DSP_EnableInterpLastData(DSP);
    }
    /* Config length, interp begain location and interp step */
    LL_DSP_SetInterpLength(DSP, (uint32_t)(InterpPara->SrcLen - 1));
    LL_DSP_SetDMADataLength(DSP, (uint32_t)(InterpPara->DstLen - 1));
    LL_DSP_SetInterpLocation(DSP, InterpPara->CurLoc);
    LL_DSP_SetInterpStep(DSP, InterpPara->InterpStep);
    /* Enable interp DMA*/
    LL_DSP_EnableDMA(DSP);

    timeOut = (InterpPara->SrcLen > InterpPara->DstLen)?(InterpPara->SrcLen << 2):(InterpPara->DstLen << 2);
    if(InterpType == LAGRANGE)
    {
        while(0 == LL_DSP_IsActiveFlag_LAGR(DSP))
        {
            cnt++;
            if(cnt > timeOut)
            {
                funStat = HAL_ERROR;
                break;
            }
        }
    }
    else
    {
        while(0 == LL_DSP_IsActiveFlag_LINE(DSP))
        {
            cnt++;
            if(cnt > timeOut)
            {
                funStat = HAL_ERROR;
                break;
            }
        }
    }
    return funStat;
}

/**
 * @brief  FFT function.
 * @note   if InterTabEn = 1, FFT will use internal parameter table, external table address: PrbAddr and PibAddr will not be used,
 *         if InterTabEn = 0, PrbAddr and PibAddr should be initialized.
 * @param  FFTPara Pointer to DSP_FFTParaTypeDef, include following parameters:
 *                       InterTabEn: Internal parameter table enable select, 0: disable, 1: enable.
 *                       SrbAddr:    Source real part data address
 *                       SibAddr:    Source image part data address
 *                       dstRAddr:   Dstination real part data address
 *                       dstIAddr:   Dstination image part data address
 *                       PrbAddr:    Real part Parameteter table address
 *                       PibAddr:    Image part Parameteter table address
 *                       Length:     FFT Data length
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_DSP_FFT(DSP_FFTParaTypeDef *FFTPara)
{
    HAL_StatusTypeDef funStat = HAL_OK;
    uint32_t cnt = 0;
    uint32_t timeOut;

    /* Init CTL1 and CTL2 */
    DSP_DeInitCTL1();
    DSP_DeInitCTL2();
    /* Configure FFT mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_FFT);
    /* Configure parameter table */
    if(FFTPara->InterTabEn == 1){
        LL_DSP_EnableFFTTable(DSP);
    }
    else
    {
    	LL_DSP_SetDMAParaRealAddr(DSP, (FFTPara->PrbAddr & 0x1ffff) >> 2);
    	LL_DSP_SetDMAParaImagAddr(DSP, (FFTPara->PibAddr & 0x1ffff) >> 2);
    }

    /* Configure DMA parameters, address should be Word-aligned */
    LL_DSP_SetDMASourceRealAddr(DSP, (FFTPara->SrbAddr & 0x1ffff) >> 2);
    LL_DSP_SetDMASourceImagAddr(DSP, (FFTPara->SibAddr & 0x1ffff) >> 2);
    LL_DSP_SetDMATargetRealAddr(DSP, (FFTPara->DstRAddr & 0x1ffff) >> 2);
    LL_DSP_SetDMATargetImagAddr(DSP, (FFTPara->DstIAddr & 0x1ffff) >> 2);
    LL_DSP_SetDMADataLength(DSP, (uint32_t)(FFTPara->Length - 1));
    /* Enable DMA */
    LL_DSP_EnableDMA(DSP);

    timeOut = (uint32_t)((FFTPara->Length & 0xffff) << 6);
    while(0 == LL_DSP_IsActiveFlag_FFT(DSP))
    {
        cnt++;
        if(cnt > timeOut)
        {
            funStat = HAL_ERROR;
            break;
        }
    }
    return funStat;
}

/**
 * @brief  FIR filter function.
 * @note   No support using the same address for SrcAddr and dstAddr.
 * @param  FIRPara Pointer to DSP_FIRParaTypeDef, include following parameters:
 *                       SrcAddr:    Source data address
 *                       dstAddr:   Dstination data address
 *                       ParaAddr:   FIR parameter "a" address
 *                       Length:     Data length
 *                       FirOrder:   Filter order
 *                       DMAStep:    DMA read SRAM step, default value is step 1. this parameter can be one of the following values:
 *                                    @arg @ref ReadStep_1
 *                                    @arg @ref ReadStep_2
 *                                    @arg @ref ReadStep_3
 *                                    @arg @ref ReadStep_4
 *                                    @arg @ref ReadStep_5
 *                                    @arg @ref ReadStep_6
 *                                    @arg @ref ReadStep_7
 *                                    @arg @ref ReadStep_8
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_DSP_FIR_DMA(DSP_FIRParaTypeDef *FIRPara)
{
    HAL_StatusTypeDef funStat = HAL_OK;
    uint32_t cnt = 0;
    uint32_t timeOut;

    DSP_DeInitCTL1();
    DSP_DeInitCTL2();
    /* Set FIR order */
    LL_DSP_SetFIROrder(DSP, FIRPara->FirOrder);
    /* Configure FFT mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_FIR_DMA);
    /* Configure DMA address and data length */
    LL_DSP_SetDMASourceRealAddr(DSP, (FIRPara->SrcAddr >> 2));
    LL_DSP_SetDMATargetRealAddr(DSP, (FIRPara->DstAddr >> 2));
    LL_DSP_SetDMAParaRealAddr(DSP, (FIRPara->ParaAddr >> 2));
    LL_DSP_SetDMADataLength(DSP, (uint32_t)(FIRPara->Length - 1));
    LL_DSP_SetFilterDMAStep(DSP, FIRPara->DMAStep);
    /* Clear flag */
    LL_DSP_ClearFlag_FIRDMA(DSP);
    LL_DSP_ClearFlag_DMA(DSP);
    /* Enable DMA */
    LL_DSP_EnableDMA(DSP);
    timeOut = (uint32_t)((FIRPara->Length & 0xffff) << 6);
    while(0 == LL_DSP_IsActiveFlag_FFT(DSP))
    {
        cnt++;
        if(cnt > timeOut)
        {
            funStat = HAL_ERROR;
            break;
        }
    }
    return funStat;
}

/**
 * @brief  Second order IIR filter function.
 *         Second order IIR difference equation: y[n] = (b0*x[n] + b1*x[n-1] + b2*x[n-2]) - (a1*y[n-1] + a2*y[n-2])
 * @param  IIROncePara Pointer to DSP_IIROnceParaTypeDef, include following parameters:
 *                       CoeffA:   Pointer to coefficient a: a1, a2
 *                       CoeffB:   Pointer to coefficient b: b0, b1, b2
 *                       DataX:    Pointer to input data  x: x[n], x[n-1], x[n-2]
 *                       PreDataY: Pointer to previous data y: y[n-1], y[n-2]
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_DSP_IIR_Once(DSP_IIROnceParaTypeDef *IIROncePara)
{
    HAL_StatusTypeDef funStat = HAL_OK;
    uint32_t cnt = 0;
    uint32_t *result = IIROncePara->OutputY;

    /*Clear flag*/
    LL_DSP_ClearFlag_IIRONCE(DSP);

    /* Configure FFT mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_IIR_ONCE);
    /* Configure IIR parameter */
    /* a1, a2 */
    LL_DSP_SetDivInput0(DSP, IIROncePara->CoeffA[0]);
    LL_DSP_SetDivInput1(DSP, IIROncePara->CoeffA[1]);
    /* b0, b1, b2 */
    LL_DSP_SetInput3(DSP, IIROncePara->CoeffB[0]);
    LL_DSP_SetInput4(DSP, IIROncePara->CoeffB[1]);
    LL_DSP_SetInput5(DSP, IIROncePara->CoeffB[2]);
    /* y[n-2], y[n-1] */
    LL_DSP_SetOutput1(DSP, IIROncePara->PreDataY[1]);
    LL_DSP_SetOutput0(DSP, IIROncePara->PreDataY[0]);
    /* x[n-2], x[n-1], x[n] */
    LL_DSP_SetInput2(DSP, IIROncePara->DataX[2]);
    LL_DSP_SetInput1(DSP, IIROncePara->DataX[1]);
    LL_DSP_SetInput0(DSP, IIROncePara->DataX[0]);

    while(0 == LL_DSP_IsActiveFlag_IIRONCE(DSP))
    {
        cnt++;
        if(cnt > DSP_GENERAL_TIMEOUT)
        {
            funStat = HAL_ERROR;
            break;
        }
    }
    /* Get output data y[n] */
    *result = LL_DSP_GetOutput0(DSP);

    return funStat;
}

/**
 * @brief  Second order IIR filter DMA function.
 * @note   No support using the same address for SrcAddr and dstAddr.
 *         Second order IIR difference equation: y[n] = (b0*x[n] + b1*x[n-1] + b2*x[n-2]) - (a1*y[n-1] + a2*y[n-2])
 * @param  IIRPara Pointer to DSP_IIRDMAParaTypeDef, include following parameters:
 *                       CoeffA:    Pointer to coefficient a: a1, a2
 *                       CoeffB:    Pointer to coefficient b: b0, b1, b2
 *                       PreDataY:  Pointer to previous data y: y[n-1], y[n-2]
 *                       PreDataX:  Pointer to previous data x: x[n-1], x[n-2]
 *                       SrcAddr:   Data x[n] address
 *                       dstAddr:   Data y[n] address
 *                       Length:    Data length
 *                       DMAStep:   DMA read SRAM step, default value is step 1. this parameter can be one of the following values:
 *                                   @arg @ref ReadStep_1
 *                                   @arg @ref ReadStep_2
 *                                   @arg @ref ReadStep_3
 *                                   @arg @ref ReadStep_4
 *                                   @arg @ref ReadStep_5
 *                                   @arg @ref ReadStep_6
 *                                   @arg @ref ReadStep_7
 *                                   @arg @ref ReadStep_8
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_DSP_IIR_DMA(DSP_IIRDMAParaTypeDef *IIRPara)
{
    HAL_StatusTypeDef funStat = HAL_OK;
    uint32_t cnt = 0;
    uint32_t timeOut;

    DSP_DeInitCTL1();
    DSP_DeInitCTL2();
    /* Configure FFT mode */
    LL_DSP_SetDSPMode(DSP, LL_DSP_MODE_IIR_DMA);

    /* Configure IIR parameter */
    /* a1, a2 */
    LL_DSP_SetDivInput0(DSP, IIRPara->CoeffA[0]);
    LL_DSP_SetDivInput1(DSP, IIRPara->CoeffA[1]);
    /* b0, b1, b2 */
    LL_DSP_SetInput3(DSP, IIRPara->CoeffB[0]);
    LL_DSP_SetInput4(DSP, IIRPara->CoeffB[1]);
    LL_DSP_SetInput5(DSP, IIRPara->CoeffB[2]);
    /* y[n-2], y[n-1] */
    LL_DSP_SetOutput1(DSP, IIRPara->PreDataY[1]);
    LL_DSP_SetOutput0(DSP, IIRPara->PreDataY[0]);
    /* x[n-2], x[n-1] */
    LL_DSP_SetInput2(DSP, IIRPara->PreDataX[1]);
    LL_DSP_SetInput1(DSP, IIRPara->PreDataX[0]);

    /* Configure DMA address and data length */
    LL_DSP_SetDMASourceRealAddr(DSP, (IIRPara->SrcAddr >> 2));
    LL_DSP_SetDMATargetRealAddr(DSP, (IIRPara->DstAddr >> 2));
    LL_DSP_SetDMADataLength(DSP, (uint32_t)(IIRPara->Length - 1));
    LL_DSP_SetFilterDMAStep(DSP, IIRPara->DMAStep);
    /* Clear flag */
    LL_DSP_ClearFlag_IIRDMA(DSP);
    LL_DSP_ClearFlag_DMA(DSP);
    /* Enable DMA */
    LL_DSP_EnableDMA(DSP);
    timeOut = (uint32_t)((IIRPara->Length & 0xffff) << 4);
    while(0 == LL_DSP_IsActiveFlag_IIRDMA(DSP))
    {
        cnt++;
        if(cnt > timeOut)
        {
            funStat = HAL_ERROR;
            break;
        }
    }
    return funStat;
}


/** @addtogroup DSP_Private_Functions
 * @{
 */

/**
 * @brief  DeInit CTL1 register, set to default value.
 * @param  None
 * @retval None
 */
static void DSP_DeInitCTL1(void)
{
    DSP->MAC_CTL1 = 0;
}

/**
 * @brief  DeInit CTL2 register, set to default value.
 * @param  None
 * @retval None
 */
static void DSP_DeInitCTL2(void)
{
    DSP->MAC_CTL2 = 0;
}
/**
 * @}
 */


/**
 * @}
 */


/**
 * @}
 */

#endif /* HAL_DSP_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
