/**
 *****************************************************************************
 * @file rn_rv_ll_iocnt.c
 * @author Renergy Software Team
 * @brief
 *
 *****************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 *****************************************************************************
 */

#if defined(USE_FULL_LL_DRIVER)

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_ll_iocnt.h"
#include "rn_rv_ll_system_core0.h"
#include "rn_rv_ll_system_core1.h"

#ifdef  USE_FULL_ASSERT
#include "rn_rv_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined(IOCNT_CORE0) || defined(IOCNT_CORE1)

/** @defgroup IOCNT_LL IOCNT
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup IOCNT_LL_Private_Macros
 * @{
 */
#define IS_LL_IOCNT_CH(__VALUE__)               (((__VALUE__) == LL_IOCNT_CH_0)   ||\
                                                 ((__VALUE__) == LL_IOCNT_CH_1)   ||\
                                                 ((__VALUE__) == LL_IOCNT_CH_2)   ||\
                                                 ((__VALUE__) == LL_IOCNT_CH_3)   ||\
                                                 ((__VALUE__) == LL_IOCNT_CH_4))

#define IS_LL_IOCNT_INVERT(__VALUE__)           (((__VALUE__) == LL_IOCNT_INT_DISBALE)   ||\
                                                 ((__VALUE__) == LL_IOCNT_INT_ENABLE ))

#define IS_LL_IOCNT_PRESCALE(__VALUE__)         ((__VALUE__) <= 2047)

#define IS_LL_IOCNT_CHNL(__VALUE__)             (((__VALUE__) == LL_IOCNT_CHNL_EXTERNAL)   ||\
                                                 ((__VALUE__) == LL_IOCNT_CHNL_INTERNAL))

#define IS_LL_IOCNT_FLTCYCLE(__VALUE__)         ((__VALUE__) <= 127)

#define IS_LL_IOCNT_FLTBYPASS(__VALUE__)        (((__VALUE__) == LL_IOCNT_FLTBYPASS_DISBALE)   ||\
                                                 ((__VALUE__) == LL_IOCNT_FLTBYPASS_ENABLE ))

#define IS_LL_IOCNT_CNTTYPE(__VALUE__)          (((__VALUE__) == LL_IOCNT_CNTTYPE_R2CLR)   ||\
                                                 ((__VALUE__) == LL_IOCNT_CNTTYPE_COUNT))

#define IS_LL_IOCNT_MODE(__VALUE__)             (((__VALUE__) == LL_IOCNT_MODE_RISEEDGE)   ||\
                                                 ((__VALUE__) == LL_IOCNT_MODE_FALLEDGE))

#define IS_LL_IOCNT_COPL(__VALUE__)             (((__VALUE__) == LL_IOCNT_COPL_0)   ||\
                                                 ((__VALUE__) == LL_IOCNT_COPL_1))

/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup IOCNT_LL_Exported_Functions
 * @{
 */

/** @addtogroup IOCNT_LL_EF_Init
 * @{
 */

/**
 * @brief De-initialize IOCNT registers
 * @param IOCNTx IOCNT Instance
 * @return ErrorStatus value:
 *          - SUCCESS: IOCNT registers are de-initialized
 *          - ERROR:   Wrong IOCNT Port
 */
ErrorStatus LL_IOCNT_DeInit(IOCNT_TypeDef *IOCNTx)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_IOCNT_ALL_INSTANCE(IOCNTx));

    if (IOCNTx == IOCNT_CORE0) {
        /* Force and Release reset on clock of IOCNTx Port */
        LL_SYSTEM_CORE0_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_IOCNT);
        LL_SYSTEM_CORE0_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_IOCNT);
    }
    else if(IOCNTx == IOCNT_CORE1)
    {
        /* Force and Release reset on clock of IOCNTx Port */
        LL_SYSTEM_CORE1_MOD0_ForceReset(LL_SYSTEM_CORE0_MOD0_PERIPH_IOCNT);
        LL_SYSTEM_CORE1_MOD0_ReleaseReset(LL_SYSTEM_CORE0_MOD0_PERIPH_IOCNT);
    }
    else {
        status = ERROR;
    }

    return status;
}

/**
 * @brief Initialize IOCNT registers according to the specified parameters in IOCNT_InitStruct
 * @param IOCNTx IOCNT Instance
 * @param IOCNT_InitStruct pointer to LL_IOCNT_InitTypeDef structure
 * @return ErrorStatus:
 *          - SUCCESS: IOCNT registers are de-initialized
 *          - ERROR:   Not applicable
 */
ErrorStatus LL_IOCNT_Init(IOCNT_TypeDef *IOCNTx, LL_IOCNT_InitTypeDef *IOCNT_InitStruct, uint32_t Iocnt_ch)
{
    ErrorStatus status = SUCCESS;

    /* Check the parameters */
    assert_param(IS_IOCNT_ALL_INSTANCE(IOCNTx));
    assert_param(IS_LL_IOCNT_CH(Iocnt_ch));
    assert_param(IS_LL_IOCNT_INVERT(IOCNT_InitStruct->PulseInvert));
    assert_param(IS_LL_IOCNT_PRESCALE(IOCNT_InitStruct->Prescale));
    assert_param(IS_LL_IOCNT_CHNL(IOCNT_InitStruct->InputChannel));
    assert_param(IS_LL_IOCNT_FLTCYCLE(IOCNT_InitStruct->FilterCycle));
    assert_param(IS_LL_IOCNT_FLTBYPASS(IOCNT_InitStruct->FilterBypass));
    assert_param(IS_LL_IOCNT_CNTTYPE(IOCNT_InitStruct->CounterType));
    assert_param(IS_LL_IOCNT_MODE(IOCNT_InitStruct->CounterMode));
    assert_param(IS_LL_IOCNT_COPL(IOCNT_InitStruct->Copl));

    if( (IOCNT_InitStruct->Prescale > 2047U) ||
        (IOCNT_InitStruct->FilterCycle > 127U))
    {
        status = ERROR;
    }

    /*  - FilterCycle configuration     */
    LL_IOCNT_SetFilterCycle(IOCNTx, IOCNT_InitStruct->FilterCycle);
    /*  - CounterType configuration     */
    LL_IOCNT_SetCounterType(IOCNTx, IOCNT_InitStruct->CounterType);

    /* Note: Hardware constraint (refer to description of this function)    */
    /*       IOCNT instance must be disabled when change the configuration. */
    switch(Iocnt_ch)
    {
        case LL_IOCNT_CH_0:
            if(LL_IOCNT_IsEnabledIOCNT0(IOCNTx) == 0UL)
            {
                /*  - Output pulse invert configuration */
                if(IOCNT_InitStruct->PulseInvert == LL_IOCNT_INT_DISBALE)
                {
                    LL_IOCNT_DisableInvert_IOCNT0(IOCNTx);
                } else {
                    LL_IOCNT_EnableInvert_IOCNT0(IOCNTx);
                }
                /*  - Prescale configuration  */
                LL_IOCNT_SetPrescaler_IOCNT0(IOCNTx, IOCNT_InitStruct->Prescale);
                /*  - Input Channel configuration  */
                LL_IOCNT_SetinputChannel_IOCNT0(IOCNTx, IOCNT_InitStruct->InputChannel);
                /*  - FilterBypass configuration  */
                if(IOCNT_InitStruct->FilterBypass == LL_IOCNT_FLTBYPASS_DISBALE)
                {
                    LL_IOCNT_DisableFilterBypass_IOCNT0(IOCNTx);
                } else {
                    LL_IOCNT_EnableFilterBypass_IOCNT0(IOCNTx);
                }
                /*  - CounterMode configuration */
                LL_IOCNT_SetCounterMode_IOCNT0(IOCNTx, IOCNT_InitStruct->CounterMode);
                /*  - Output pulse type configuration */
                LL_IOCNT_SetOutputType_IOCNT0(IOCNTx, IOCNT_InitStruct->Copl);
                /*  - Input pulse Remainder configuration */
                LL_IOCNT_SetRemainder_IOCNT0(IOCNTx, IOCNT_InitStruct->Remd);

                /*  - Enable IOCNT Peripheral */
                LL_IOCNT_EnableIOCNT0(IOCNTx);
            }
            else
            {
                status = ERROR;
            }
            break;
        case LL_IOCNT_CH_1:
            if(LL_IOCNT_IsEnabledIOCNT1(IOCNTx) == 0UL)
            {
                /*  - Output pulse invert configuration */
                if(IOCNT_InitStruct->PulseInvert == LL_IOCNT_INT_DISBALE)
                {
                    LL_IOCNT_DisableInvert_IOCNT1(IOCNTx);
                } else {
                    LL_IOCNT_EnableInvert_IOCNT1(IOCNTx);
                }
                /*  - Prescale configuration  */
                LL_IOCNT_SetPrescaler_IOCNT1(IOCNTx, IOCNT_InitStruct->Prescale);
                /*  - Input Channel configuration  */
                LL_IOCNT_SetinputChannel_IOCNT1(IOCNTx, IOCNT_InitStruct->InputChannel);
                /*  - FilterBypass configuration  */
                if(IOCNT_InitStruct->FilterBypass == LL_IOCNT_FLTBYPASS_DISBALE)
                {
                    LL_IOCNT_DisableFilterBypass_IOCNT1(IOCNTx);
                } else {
                    LL_IOCNT_EnableFilterBypass_IOCNT1(IOCNTx);
                }
                /*  - CounterMode configuration */
                LL_IOCNT_SetCounterMode_IOCNT1(IOCNTx, IOCNT_InitStruct->CounterMode);
                /*  - Output pulse type configuration */
                LL_IOCNT_SetOutputType_IOCNT1(IOCNTx, IOCNT_InitStruct->Copl);
                /*  - Input pulse Remainder configuration */
                LL_IOCNT_SetRemainder_IOCNT1(IOCNTx, IOCNT_InitStruct->Remd);

                /*  - Enable IOCNT Peripheral */
                LL_IOCNT_EnableIOCNT1(IOCNTx);
            }
            else
            {
                status = ERROR;
            }
            break;
        case LL_IOCNT_CH_2:
            if(LL_IOCNT_IsEnabledIOCNT2(IOCNTx) == 0UL)
            {
                /*  - Output pulse invert configuration */
                if(IOCNT_InitStruct->PulseInvert == LL_IOCNT_INT_DISBALE)
                {
                    LL_IOCNT_DisableInvert_IOCNT2(IOCNTx);
                } else {
                    LL_IOCNT_EnableInvert_IOCNT2(IOCNTx);
                }
                /*  - Prescale configuration  */
                LL_IOCNT_SetPrescaler_IOCNT2(IOCNTx, IOCNT_InitStruct->Prescale);
                /*  - Input Channel configuration  */
                LL_IOCNT_SetinputChannel_IOCNT2(IOCNTx, IOCNT_InitStruct->InputChannel);
                /*  - FilterBypass configuration  */
                if(IOCNT_InitStruct->FilterBypass == LL_IOCNT_FLTBYPASS_DISBALE)
                {
                    LL_IOCNT_DisableFilterBypass_IOCNT2(IOCNTx);
                } else {
                    LL_IOCNT_EnableFilterBypass_IOCNT2(IOCNTx);
                }
                /*  - CounterMode configuration */
                LL_IOCNT_SetCounterMode_IOCNT2(IOCNTx, IOCNT_InitStruct->CounterMode);
                /*  - Output pulse type configuration */
                LL_IOCNT_SetOutputType_IOCNT2(IOCNTx, IOCNT_InitStruct->Copl);
                /*  - Input pulse Remainder configuration */
                LL_IOCNT_SetRemainder_IOCNT2(IOCNTx, IOCNT_InitStruct->Remd);

                /*  - Enable IOCNT Peripheral */
                LL_IOCNT_EnableIOCNT2(IOCNTx);
            }
            else
            {
                status = ERROR;
            }
            break;
        case LL_IOCNT_CH_3:
            if(LL_IOCNT_IsEnabledIOCNT3(IOCNTx) == 0UL)
            {
                /*  - Output pulse invert configuration */
                if(IOCNT_InitStruct->PulseInvert == LL_IOCNT_INT_DISBALE)
                {
                    LL_IOCNT_DisableInvert_IOCNT3(IOCNTx);
                } else {
                    LL_IOCNT_EnableInvert_IOCNT3(IOCNTx);
                }
                /*  - Prescale configuration  */
                LL_IOCNT_SetPrescaler_IOCNT3(IOCNTx, IOCNT_InitStruct->Prescale);
                /*  - Input Channel configuration  */
                LL_IOCNT_SetinputChannel_IOCNT3(IOCNTx, IOCNT_InitStruct->InputChannel);
                /*  - FilterBypass configuration  */
                if(IOCNT_InitStruct->FilterBypass == LL_IOCNT_FLTBYPASS_DISBALE)
                {
                    LL_IOCNT_DisableFilterBypass_IOCNT3(IOCNTx);
                } else {
                    LL_IOCNT_EnableFilterBypass_IOCNT3(IOCNTx);
                }
                /*  - CounterMode configuration */
                LL_IOCNT_SetCounterMode_IOCNT3(IOCNTx, IOCNT_InitStruct->CounterMode);
                /*  - Output pulse type configuration */
                LL_IOCNT_SetOutputType_IOCNT3(IOCNTx, IOCNT_InitStruct->Copl);
                /*  - Input pulse Remainder configuration */
                LL_IOCNT_SetRemainder_IOCNT3(IOCNTx, IOCNT_InitStruct->Remd);

                /*  - Enable IOCNT Peripheral */
                LL_IOCNT_EnableIOCNT3(IOCNTx);
            }
            else
            {
                status = ERROR;
            }
            break;
        case LL_IOCNT_CH_4:
            if(LL_IOCNT_IsEnabledIOCNT4(IOCNTx) == 0UL)
            {
                /*  - Output pulse invert configuration */
                if(IOCNT_InitStruct->PulseInvert == LL_IOCNT_INT_DISBALE)
                {
                    LL_IOCNT_DisableInvert_IOCNT4(IOCNTx);
                } else {
                    LL_IOCNT_EnableInvert_IOCNT4(IOCNTx);
                }
                /*  - Prescale configuration  */
                LL_IOCNT_SetPrescaler_IOCNT4(IOCNTx, IOCNT_InitStruct->Prescale);
                /*  - Input Channel configuration  */
                LL_IOCNT_SetinputChannel_IOCNT4(IOCNTx, IOCNT_InitStruct->InputChannel);
                /*  - FilterBypass configuration  */
                if(IOCNT_InitStruct->FilterBypass == LL_IOCNT_FLTBYPASS_DISBALE)
                {
                    LL_IOCNT_DisableFilterBypass_IOCNT4(IOCNTx);
                } else {
                    LL_IOCNT_EnableFilterBypass_IOCNT4(IOCNTx);
                }
                /*  - CounterMode configuration */
                LL_IOCNT_SetCounterMode_IOCNT4(IOCNTx, IOCNT_InitStruct->CounterMode);
                /*  - Output pulse type configuration */
                LL_IOCNT_SetOutputType_IOCNT4(IOCNTx, IOCNT_InitStruct->Copl);
                /*  - Input pulse Remainder configuration */
                LL_IOCNT_SetRemainder_IOCNT4(IOCNTx, IOCNT_InitStruct->Remd);

                /*  - Enable IOCNT Peripheral */
                LL_IOCNT_EnableIOCNT4(IOCNTx);
            }
            else
            {
                status = ERROR;
            }
            break;
        default : break;
    }


    return status;
}

/**
 * @brief Set each @ref LL_IOCNT_InitTypeDef field to default value.
 * @param IOCNT_InitStruct pointer to LL_IOCNT_InitTypeDef structure
 * @retval None
 */
void LL_IOCNT_StructInit(LL_IOCNT_InitTypeDef *IOCNT_InitStruct)
{
    /* Initialize IOCNT structure parameters to default value */
    IOCNT_InitStruct->PulseInvert  = LL_IOCNT_INT_DISBALE;
    IOCNT_InitStruct->Prescale     = 0U;
    IOCNT_InitStruct->InputChannel = LL_IOCNT_CHNL_EXTERNAL;
    IOCNT_InitStruct->FilterCycle  = 0x1FU;
    IOCNT_InitStruct->FilterBypass = LL_IOCNT_FLTBYPASS_DISBALE;
    IOCNT_InitStruct->CounterType  = LL_IOCNT_CNTTYPE_R2CLR;
    IOCNT_InitStruct->CounterMode  = LL_IOCNT_MODE_RISEEDGE;
    IOCNT_InitStruct->Copl         = LL_IOCNT_COPL_0;
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

#endif /* IOCNT_CORE0 || IOCNT_CORE1 */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
