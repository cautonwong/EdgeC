/**
 *****************************************************************************
 * @file rn_rv_ll_iocnt.h
 * @author Renergy Software Team
 * @brief Header file of IOCNT LL module.
 *
 *****************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 *****************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RN_LL_IOCNT_H
#define __RN_LL_IOCNT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_iocnt.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined(IOCNT)

/** @defgroup IOCNT_LL IOCNT
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup IOCNT_LL_Exported_Types IOCNT Exported Types
 * @{
 */

/**
 * @brief LL IOCNT Init Structure definition
 * @{
 */
typedef struct
{
    uint32_t PulseInvert;           /*!< Specifies the pulse invert function for IOCNTx.
                                     This parameter can be any value of @ref IOCNT_LL_EC_INT */

    uint32_t Prescale;              /*!< Specifies the prescale of pulse for IOCNTx. */

    uint32_t InputChannel;          /*!< Specifies the input channel for IOCNTx.
                                     This parameter can be any value of @ref IOCNT_LL_EC_CHNL */

    uint32_t FilterCycle;           /*!< Specifies the filtering cycle of pulse for IOCNTx. */

    uint32_t FilterBypass;           /*!< Specifies the filter bypass select for IOCNTx.
                                     This parameter can be any value of @ref IOCNT_LL_EC_FLTBYPASS */

    uint32_t CounterType;           /*!< Specifies the counter type for IOCNTx.
                                     This parameter can be any value of @ref IOCNT_LL_EC_CNTTYPE */

    uint32_t CounterMode;           /*!< Specifies the counter mode for IOCNTx.
                                     This parameter can be any value of @ref IOCNT_LL_EC_MODE */

    uint32_t Copl;                  /*!< Specifies the output pulse configuration for IOCNTx.
                                     This parameter can be any value of @ref IOCNT_LL_EC_COPL */

    uint32_t Remd;             /*!< Specifies the input pulse Remainder for IOCNTx. */
} LL_IOCNT_InitTypeDef;
/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup IOCNT_LL_Exported_Constants IOCNT Exported Constants
 * @{
 */

/** @defgroup IOCNT_LL_EC_CH    IOCNT channel
 * @{
 */
#define LL_IOCNT_CH_0            (0x00000000U)  //Select IOCNT0
#define LL_IOCNT_CH_1            (0x00000001U)  //Select IOCNT1
#define LL_IOCNT_CH_2            (0x00000002U)  //Select IOCNT2
#define LL_IOCNT_CH_3            (0x00000003U)  //Select IOCNT3
#define LL_IOCNT_CH_4            (0x00000004U)  //Select IOCNT4
/**
 * @}
 */

/** @defgroup IOCNT_LL_EC_INT    IOCNT pulse invert
 * @{
 */
#define LL_IOCNT_INT_DISBALE     (0x00000000U)  //Direction of output pulse follow the input pulse
#define LL_IOCNT_INT_ENABLE      (0x00000001U)  //Invert the output pulse
/**
 * @}
 */

/** @defgroup IOCNT_LL_EC_FLTBYPASS    IOCNT filter bypass
 * @{
 */
#define LL_IOCNT_FLTBYPASS_DISBALE    (0x00000000U) //Disable filter bypass, filter by filter cycle conffguration
#define LL_IOCNT_FLTBYPASS_ENABLE     (0x00000001U) //Enable filter bypass
/**
 * @}
 */

/** @defgroup IOCNT_LL_EC_CHNL    Input type
 * @{
 */
#define LL_IOCNT_CHNL_EXTERNAL    (0x00000000U) //Select external signal as input
#define LL_IOCNT_CHNL_INTERNAL    (0x00000001U) //Select internal special commands as input
/**
 * @}
 */

/** @defgroup IOCNT_LL_EC_CNTTYPE      Counter clear type
 * @{
 */
#define LL_IOCNT_CNTTYPE_R2CLR     (0x00000000U) //Counter of IOCNT0~IOCNT4 will be clear after read.
#define LL_IOCNT_CNTTYPE_COUNT     (0x00000001U) //Counter of IOCNT0~IOCNT4 will continue to count afrer read
/**
 * @}
 */

/** @defgroup IOCNT_LL_EC_MODE      Counter mode
 * @{
 */
#define LL_IOCNT_MODE_RISEEDGE      (0x00000000U)  //Select rising edge trigger
#define LL_IOCNT_MODE_FALLEDGE      (0x00000001U)  //Select falling edge trigger
/**
 * @}
 */

/** @defgroup IOCNT_LL_EC_COPL    Output type
 * @{
 */
#define LL_IOCNT_COPL_0           (0x00000000U)   // Output the (n+1)th pulse at the (n*Prescaler+1)th input pulse.
#define LL_IOCNT_COPL_1           (0x00000001U)   // Output the (n)th pulse at the (n*Prescaler)th input pulse.
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup IOCNT_LL_Exported_Functions IOCNT Exported Functions
 * @{
 */

/** @defgroup IOCNT_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief Enable IOCNT0 output pulse invert.
 * @rmtoll CFG0   INT   LL_IOCNT_EnableInvert_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableInvert_IOCNT0(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CFG0, IOCNT_CFG0_INT);
}

/**
 * @brief Disable IOCNT0 output pulse invert.
 * @rmtoll CFG0   INT   LL_IOCNT_DisableInvert_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableInvert_IOCNT0(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CFG0, IOCNT_CFG0_INT);
}

/**
 * @brief Indicates whether IOCNT0 output pulse invert is enabled.
 * @rmtoll CFG0   INT  LL_IOCNT_IsEnabledInvert_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledInvert_IOCNT0(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG0, IOCNT_CFG0_INT) == IOCNT_CFG0_INT);
}

/**
 * @brief Enable IOCNT1 output pulse invert.
 * @rmtoll CFG1   INT   LL_IOCNT_EnableInvert_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableInvert_IOCNT1(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CFG1, IOCNT_CFG1_INT);
}

/**
 * @brief Disable IOCNT1 output pulse invert.
 * @rmtoll CFG1   INT   LL_IOCNT_DisableInvert_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableInvert_IOCNT1(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CFG1, IOCNT_CFG1_INT);
}

/**
 * @brief Indicates whether IOCNT1 output pulse invert is enabled.
 * @rmtoll CFG1   INT  LL_IOCNT_IsEnabledInvert_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledInvert_IOCNT1(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG1, IOCNT_CFG1_INT) == IOCNT_CFG1_INT);
}

/**
 * @brief Enable IOCNT2 output pulse invert.
 * @rmtoll CFG2   INT   LL_IOCNT_EnableInvert_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableInvert_IOCNT2(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CFG2, IOCNT_CFG2_INT);
}

/**
 * @brief Disable IOCNT2 output pulse invert.
 * @rmtoll CFG2   INT   LL_IOCNT_DisableInvert_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableInvert_IOCNT2(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CFG2, IOCNT_CFG2_INT);
}

/**
 * @brief Indicates whether IOCNT2 output pulse invert is enabled.
 * @rmtoll CFG2   INT  LL_IOCNT_IsEnabledInvert_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledInvert_IOCNT2(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG2, IOCNT_CFG2_INT) == IOCNT_CFG2_INT);
}

/**
 * @brief Enable IOCNT3 output pulse invert.
 * @rmtoll CFG3   INT   LL_IOCNT_EnableInvert_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableInvert_IOCNT3(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CFG3, IOCNT_CFG3_INT);
}

/**
 * @brief Disable IOCNT3 output pulse invert.
 * @rmtoll CFG3   INT   LL_IOCNT_DisableInvert_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableInvert_IOCNT3(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CFG3, IOCNT_CFG3_INT);
}

/**
 * @brief Indicates whether IOCNT3 output pulse invert is enabled.
 * @rmtoll CFG3   INT  LL_IOCNT_IsEnabledInvert_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledInvert_IOCNT3(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG3, IOCNT_CFG3_INT) == IOCNT_CFG3_INT);
}

/**
 * @brief Enable IOCNT4 output pulse invert.
 * @rmtoll CFG4   INT   LL_IOCNT_EnableInvert_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableInvert_IOCNT4(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CFG4, IOCNT_CFG4_INT);
}

/**
 * @brief Disable IOCNT4 output pulse invert.
 * @rmtoll CFG4   INT   LL_IOCNT_DisableInvert_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableInvert_IOCNT4(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CFG4, IOCNT_CFG4_INT);
}

/**
 * @brief Indicates whether IOCNT4 output pulse invert is enabled.
 * @rmtoll CFG4   INT  LL_IOCNT_IsEnabledInvert_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledInvert_IOCNT4(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG4, IOCNT_CFG4_INT) == IOCNT_CFG4_INT);
}

/**
 * @brief Configure pulse prescaler for IOCNT0.
 * @note  Real prescaler = Prescaler + 1,  0 <= Prescaler <= 2047
 * @note  The maximum high level time of the pulse after division is 80ms, and the excess part will be pulled down.
 * @rmtoll CFG0   CFG   LL_IOCNT_SetPrescaler_IOCNT0
 * @param IOCNTx    IOCNT Instance
 * @param Prescaler Pulse prescaler
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetPrescaler_IOCNT0(IOCNT_TypeDef *IOCNTx, uint32_t Prescaler)
{
    MODIFY_REG(IOCNTx->CFG0, IOCNT_CFG0_CFG, (Prescaler << IOCNT_CFG0_CFG_POS));
}

/**
 * @brief Return pulse prescaler for IOCNT0.
 * @note  Real prescaler = Prescaler + 1,  0 <= Prescaler <= 2047
 * @note  The maximum high level time of the pulse after division is 80ms, and the excess part will be pulled down.
 * @rmtoll CFG0   CFG   LL_IOCNT_GetPrescaler_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT0 pulse prescaler
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetPrescaler_IOCNT0(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG0, IOCNT_CFG0_CFG) >> IOCNT_CFG0_CFG_POS);
}

/**
 * @brief Configure pulse prescaler for IOCNT1.
 * @note  Real prescaler = Prescaler + 1,  0 <= Prescaler <= 2047
 * @note  The maximum high level time of the pulse after division is 80ms, and the excess part will be pulled down.
 * @rmtoll CFG1   CFG   LL_IOCNT_SetPrescaler_IOCNT1
 * @param IOCNTx    IOCNT Instance
 * @param Prescaler Pulse prescaler
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetPrescaler_IOCNT1(IOCNT_TypeDef *IOCNTx, uint32_t Prescaler)
{
    MODIFY_REG(IOCNTx->CFG1, IOCNT_CFG1_CFG, (Prescaler << IOCNT_CFG1_CFG_POS));
}

/**
 * @brief Return pulse prescaler for IOCNT1.
 * @note  Real prescaler = Prescaler + 1,  0 <= Prescaler <= 2047
 * @note  The maximum high level time of the pulse after division is 80ms, and the excess part will be pulled down.
 * @rmtoll CFG1   CFG   LL_IOCNT_GetPrescaler_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT1 pulse prescaler
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetPrescaler_IOCNT1(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG1, IOCNT_CFG1_CFG) >> IOCNT_CFG1_CFG_POS);
}

/**
 * @brief Configure pulse prescaler for IOCNT2.
 * @note  Real prescaler = Prescaler + 1,  0 <= Prescaler <= 2047
 * @note  The maximum high level time of the pulse after division is 80ms, and the excess part will be pulled down.
 * @rmtoll CFG2   CFG   LL_IOCNT_SetPrescaler_IOCNT2
 * @param IOCNTx    IOCNT Instance
 * @param Prescaler Pulse prescaler
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetPrescaler_IOCNT2(IOCNT_TypeDef *IOCNTx, uint32_t Prescaler)
{
    MODIFY_REG(IOCNTx->CFG2, IOCNT_CFG2_CFG, (Prescaler << IOCNT_CFG2_CFG_POS));
}

/**
 * @brief Return pulse prescaler for IOCNT2.
 * @note  Real prescaler = Prescaler + 1,  0 <= Prescaler <= 2047
 * @note  The maximum high level time of the pulse after division is 80ms, and the excess part will be pulled down.
 * @rmtoll CFG2   CFG   LL_IOCNT_GetPrescaler_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT2 pulse prescaler
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetPrescaler_IOCNT2(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG2, IOCNT_CFG2_CFG) >> IOCNT_CFG2_CFG_POS);
}

/**
 * @brief Configure pulse prescaler for IOCNT3.
 * @note  Real prescaler = Prescaler + 1,  0 <= Prescaler <= 2047
 * @note  The maximum high level time of the pulse after division is 80ms, and the excess part will be pulled down.
 * @rmtoll CFG3   CFG   LL_IOCNT_SetPrescaler_IOCNT3
 * @param IOCNTx    IOCNT Instance
 * @param Prescaler Pulse prescaler
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetPrescaler_IOCNT3(IOCNT_TypeDef *IOCNTx, uint32_t Prescaler)
{
    MODIFY_REG(IOCNTx->CFG3, IOCNT_CFG3_CFG, (Prescaler << IOCNT_CFG3_CFG_POS));
}

/**
 * @brief Return pulse prescaler for IOCNT3.
 * @note  Real prescaler = Prescaler + 1,  0 <= Prescaler <= 2047
 * @note  The maximum high level time of the pulse after division is 80ms, and the excess part will be pulled down.
 * @rmtoll CFG3   CFG   LL_IOCNT_GetPrescaler_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT3 pulse prescaler
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetPrescaler_IOCNT3(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG3, IOCNT_CFG3_CFG) >> IOCNT_CFG3_CFG_POS);
}

/**
 * @brief Configure pulse prescaler for IOCNT4.
 * @note  Real prescaler = Prescaler + 1,  0 <= Prescaler <= 2047
 * @note  The maximum high level time of the pulse after division is 80ms, and the excess part will be pulled down.
 * @rmtoll CFG4   CFG   LL_IOCNT_SetPrescaler_IOCNT4
 * @param IOCNTx    IOCNT Instance
 * @param Prescaler Pulse prescaler
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetPrescaler_IOCNT4(IOCNT_TypeDef *IOCNTx, uint32_t Prescaler)
{
    MODIFY_REG(IOCNTx->CFG4, IOCNT_CFG4_CFG, (Prescaler << IOCNT_CFG4_CFG_POS));
}

/**
 * @brief Return pulse prescaler for IOCNT4.
 * @note  Real prescaler = Prescaler + 1,  0 <= Prescaler <= 2047
 * @note  The maximum high level time of the pulse after division is 80ms, and the excess part will be pulled down.
 * @rmtoll CFG4   CFG   LL_IOCNT_GetPrescaler_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT4 pulse prescaler
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetPrescaler_IOCNT4(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG4, IOCNT_CFG4_CFG) >> IOCNT_CFG4_CFG_POS);
}

/**
 * @brief Return input pulse counter for IOCNT0.
 * @note  CNT will be cleared after read, it can be configured to accumulate after read.
 * @rmtoll CFG0   CNT   LL_IOCNT_GetInputCounter_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT0 input pulse counter
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetInputCounter_IOCNT0(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG0, IOCNT_CFG0_CNT));
}

/**
 * @brief Return input pulse counter for IOCNT1.
 * @note  CNT will be cleared after read, it can be configured to accumulate after read.
 * @rmtoll CFG1   CNT   LL_IOCNT_GetInputCounter_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT1 input pulse counter
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetInputCounter_IOCNT1(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG1, IOCNT_CFG1_CNT));
}

/**
 * @brief Return input pulse counter for IOCNT2.
 * @note  CNT will be cleared after read, it can be configured to accumulate after read.
 * @rmtoll CFG2   CNT   LL_IOCNT_GetInputCounter_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT2 input pulse counter
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetInputCounter_IOCNT2(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG2, IOCNT_CFG2_CNT));
}

/**
 * @brief Return input pulse counter for IOCNT3.
 * @note  CNT will be cleared after read, it can be configured to accumulate after read.
 * @rmtoll CFG3   CNT   LL_IOCNT_GetInputCounter_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT3 input pulse counter
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetInputCounter_IOCNT3(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG3, IOCNT_CFG3_CNT));
}

/**
 * @brief Return input pulse counter for IOCNT4.
 * @note  CNT will be cleared after read, it can be configured to accumulate after read.
 * @rmtoll CFG4   CNT   LL_IOCNT_GetInputCounter_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT4 input pulse counter
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetInputCounter_IOCNT4(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CFG4, IOCNT_CFG4_CNT));
}

/**
 * @brief Return output pulse counter for IOCNT0.
 * @note  CNT will be cleared after read, it can be configured to accumulate after read.
 * @rmtoll OUT0   OUT0   LL_IOCNT_GetInputCounter_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT0 input pulse counter
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetOutputCounter_IOCNT0(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->OUT0, IOCNT_OUT0_CNT));
}

/**
 * @brief Return output pulse counter for IOCNT1.
 * @note  CNT will be cleared after read, it can be configured to accumulate after read.
 * @rmtoll OUT1   OUT1   LL_IOCNT_GetInputCounter_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT1 input pulse counter
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetOutputCounter_IOCNT1(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->OUT1, IOCNT_OUT1_CNT));
}

/**
 * @brief Return output pulse counter for IOCNT2.
 * @note  CNT will be cleared after read, it can be configured to accumulate after read.
 * @rmtoll OUT2   OUT2   LL_IOCNT_GetInputCounter_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT2 input pulse counter
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetOutputCounter_IOCNT2(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->OUT2, IOCNT_OUT2_CNT));
}

/**
 * @brief Return output pulse counter for IOCNT3.
 * @note  CNT will be cleared after read, it can be configured to accumulate after read.
 * @rmtoll OUT3   OUT3   LL_IOCNT_GetInputCounter_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT3 input pulse counter
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetOutputCounter_IOCNT3(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->OUT3, IOCNT_OUT3_CNT));
}

/**
 * @brief Return output pulse counter for IOCNT4.
 * @note  CNT will be cleared after read, it can be configured to accumulate after read.
 * @rmtoll OUT4   OUT4   LL_IOCNT_GetInputCounter_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return IOCNT4 input pulse counter
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetOutputCounter_IOCNT4(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->OUT4, IOCNT_OUT4_CNT));
}

/**
 * @brief Configure input channel type for IOCNT0.
 * @note  The filtering function must be bypassed when select internal input command.
 * @rmtoll CHNL   IOCNT0_SEL   LL_IOCNT_SetinputChannel_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @param Type    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_CHNL_EXTERNAL
 *         @arg @ref LL_IOCNT_CHNL_INTERNAL
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetinputChannel_IOCNT0(IOCNT_TypeDef *IOCNTx, uint32_t Type)
{
    MODIFY_REG(IOCNTx->CHNL, IOCNT_CHNL_IOCNT0_SEL, (Type << IOCNT_CHNL_IOCNT0_SEL_POS));
}

/**
 * @brief Return input channel type for IOCNT0.
 * @note  The filtering function must be bypassed when select internal input command.
 * @rmtoll CHNL   IOCNT0_SEL   LL_IOCNT_GetinputChannel_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_CHNL_EXTERNAL
 *         @arg @ref LL_IOCNT_CHNL_INTERNAL
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetinputChannel_IOCNT0(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CHNL, IOCNT_CHNL_IOCNT0_SEL) >> IOCNT_CHNL_IOCNT0_SEL_POS);
}

/**
 * @brief Configure input channel type for IOCNT1.
 * @note  The filtering function must be bypassed when select internal input command.
 * @rmtoll CHNL   IOCNT1_SEL   LL_IOCNT_SetinputChannel_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @param Type    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_CHNL_EXTERNAL
 *         @arg @ref LL_IOCNT_CHNL_INTERNAL
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetinputChannel_IOCNT1(IOCNT_TypeDef *IOCNTx, uint32_t Type)
{
    MODIFY_REG(IOCNTx->CHNL, IOCNT_CHNL_IOCNT1_SEL, (Type << IOCNT_CHNL_IOCNT1_SEL_POS));
}

/**
 * @brief Return input channel type for IOCNT1.
 * @note  The filtering function must be bypassed when select internal input command.
 * @rmtoll CHNL   IOCNT1_SEL   LL_IOCNT_GetinputChannel_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_CHNL_EXTERNAL
 *         @arg @ref LL_IOCNT_CHNL_INTERNAL
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetinputChannel_IOCNT1(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CHNL, IOCNT_CHNL_IOCNT1_SEL) >> IOCNT_CHNL_IOCNT1_SEL_POS);
}

/**
 * @brief Configure input channel type for IOCNT2.
 * @note  The filtering function must be bypassed when select internal input command.
 * @rmtoll CHNL   IOCNT2_SEL   LL_IOCNT_SetinputChannel_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @param Type    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_CHNL_EXTERNAL
 *         @arg @ref LL_IOCNT_CHNL_INTERNAL
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetinputChannel_IOCNT2(IOCNT_TypeDef *IOCNTx, uint32_t Type)
{
    MODIFY_REG(IOCNTx->CHNL, IOCNT_CHNL_IOCNT2_SEL, (Type << IOCNT_CHNL_IOCNT2_SEL_POS));
}

/**
 * @brief Return input channel type for IOCNT2.
 * @note  The filtering function must be bypassed when select internal input command.
 * @rmtoll CHNL   IOCNT2_SEL   LL_IOCNT_GetinputChannel_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_CHNL_EXTERNAL
 *         @arg @ref LL_IOCNT_CHNL_INTERNAL
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetinputChannel_IOCNT2(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CHNL, IOCNT_CHNL_IOCNT2_SEL) >> IOCNT_CHNL_IOCNT2_SEL_POS);
}

/**
 * @brief Configure input channel type for IOCNT3.
 * @note  The filtering function must be bypassed when select internal input command.
 * @rmtoll CHNL   IOCNT3_SEL   LL_IOCNT_SetinputChannel_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @param Type    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_CHNL_EXTERNAL
 *         @arg @ref LL_IOCNT_CHNL_INTERNAL
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetinputChannel_IOCNT3(IOCNT_TypeDef *IOCNTx, uint32_t Type)
{
    MODIFY_REG(IOCNTx->CHNL, IOCNT_CHNL_IOCNT3_SEL, (Type << IOCNT_CHNL_IOCNT3_SEL_POS));
}

/**
 * @brief Return input channel type for IOCNT3.
 * @note  The filtering function must be bypassed when select internal input command.
 * @rmtoll CHNL   IOCNT3_SEL   LL_IOCNT_GetinputChannel_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_CHNL_EXTERNAL
 *         @arg @ref LL_IOCNT_CHNL_INTERNAL
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetinputChannel_IOCNT3(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CHNL, IOCNT_CHNL_IOCNT3_SEL) >> IOCNT_CHNL_IOCNT3_SEL_POS);
}

/**
 * @brief Configure input channel type for IOCNT4.
 * @note  The filtering function must be bypassed when select internal input command.
 * @rmtoll CHNL   IOCNT4_SEL   LL_IOCNT_SetinputChannel_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @param Type    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_CHNL_EXTERNAL
 *         @arg @ref LL_IOCNT_CHNL_INTERNAL
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetinputChannel_IOCNT4(IOCNT_TypeDef *IOCNTx, uint32_t Type)
{
    MODIFY_REG(IOCNTx->CHNL, IOCNT_CHNL_IOCNT4_SEL, (Type << IOCNT_CHNL_IOCNT4_SEL_POS));
}

/**
 * @brief Return input channel type for IOCNT4.
 * @note  The filtering function must be bypassed when select internal input command.
 * @rmtoll CHNL   IOCNT4_SEL   LL_IOCNT_GetinputChannel_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_CHNL_EXTERNAL
 *         @arg @ref LL_IOCNT_CHNL_INTERNAL
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetinputChannel_IOCNT4(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CHNL, IOCNT_CHNL_IOCNT4_SEL) >> IOCNT_CHNL_IOCNT4_SEL_POS);
}

/**
 * @brief Configure filtering cycle for IOCNT0~IOCNT4.
 * @note  Real filtering cycle = Cycle + 1, 0 <= Cycle <= 127.
 * @note  The filtering cycle is the system clock number. When the pulse width is less than
 *        filtering cycle(system clock number), it will be filtered out.
 * @rmtoll CTRL   FLT_CFG   LL_IOCNT_SetFilterCycle
 * @param IOCNTx  IOCNT Instance
 * @param Cycle   Filtering cycle
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetFilterCycle(IOCNT_TypeDef *IOCNTx, uint32_t Cycle)
{
    MODIFY_REG(IOCNTx->CTRL, IOCNT_CTRL_FLT_CFG, (Cycle << IOCNT_CTRL_FLT_CFG_POS));
}

/**
 * @brief Return filtering cycle for IOCNT0~IOCNT4.
 * @note  Real filtering cycle = Cycle + 1, 0 <= Cycle <= 127.
 * @note  The filtering cycle is the system clock number. When the pulse width is less than
 *        filtering cycle(system clock number), it will be filtered out.
 * @rmtoll CTRL   FLT_CFG   LL_IOCNT_GetFilterCycle
 * @param IOCNTx  IOCNT Instance
 * @return Filtering cycle
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetFilterCycle(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_CFG) >> IOCNT_CTRL_FLT_CFG_POS);
}

/**
 * @brief Enable bypass filter for IOCNT0.
 * @rmtoll CTRL   FLT_BYPASS0   LL_IOCNT_EnableFilterBypass_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableFilterBypass_IOCNT0(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS0);
}

/**
 * @brief Disable bypass filter for IOCNT0.
 * @rmtoll CTRL   FLT_BYPASS0   LL_IOCNT_DisableFilterBypass_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableFilterBypass_IOCNT0(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS0);
}

/**
 * @brief Indicates whether bypass filter for IOCNT0 is enabled.
 * @rmtoll CTRL   FLT_BYPASS0  LL_IOCNT_IsEnabledFilterBypass_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledFilterBypass_IOCNT0(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS0) == IOCNT_CTRL_FLT_BYPASS0);
}

/**
 * @brief Enable bypass filter for IOCNT1.
 * @rmtoll CTRL   FLT_BYPASS1   LL_IOCNT_EnableFilterBypass_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableFilterBypass_IOCNT1(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS1);
}

/**
 * @brief Disable bypass filter for IOCNT1.
 * @rmtoll CTRL   FLT_BYPASS1   LL_IOCNT_DisableFilterBypass_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableFilterBypass_IOCNT1(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS1);
}

/**
 * @brief Indicates whether bypass filter for IOCNT1 is enabled.
 * @rmtoll CTRL   FLT_BYPASS1  LL_IOCNT_IsEnabledFilterBypass_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledFilterBypass_IOCNT1(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS1) == IOCNT_CTRL_FLT_BYPASS1);
}

/**
 * @brief Enable bypass filter for IOCNT2.
 * @rmtoll CTRL   FLT_BYPASS2   LL_IOCNT_EnableFilterBypass_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableFilterBypass_IOCNT2(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS2);
}

/**
 * @brief Disable bypass filter for IOCNT2.
 * @rmtoll CTRL   FLT_BYPASS2   LL_IOCNT_DisableFilterBypass_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableFilterBypass_IOCNT2(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS2);
}

/**
 * @brief Indicates whether bypass filter for IOCNT2 is enabled.
 * @rmtoll CTRL   FLT_BYPASS2  LL_IOCNT_IsEnabledFilterBypass_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledFilterBypass_IOCNT2(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS2) == IOCNT_CTRL_FLT_BYPASS2);
}

/**
 * @brief Enable bypass filter for IOCNT3.
 * @rmtoll CTRL   FLT_BYPASS3   LL_IOCNT_EnableFilterBypass_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableFilterBypass_IOCNT3(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS3);
}

/**
 * @brief Disable bypass filter for IOCNT3.
 * @rmtoll CTRL   FLT_BYPASS3   LL_IOCNT_DisableFilterBypass_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableFilterBypass_IOCNT3(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS3);
}

/**
 * @brief Indicates whether bypass filter for IOCNT3 is enabled.
 * @rmtoll CTRL   FLT_BYPASS3  LL_IOCNT_IsEnabledFilterBypass_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledFilterBypass_IOCNT3(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS3) == IOCNT_CTRL_FLT_BYPASS3);
}

/**
 * @brief Enable bypass filter for IOCNT4.
 * @rmtoll CTRL   FLT_BYPASS4   LL_IOCNT_EnableFilterBypass_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableFilterBypass_IOCNT4(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS4);
}

/**
 * @brief Disable bypass filter for IOCNT4.
 * @rmtoll CTRL   FLT_BYPASS4   LL_IOCNT_DisableFilterBypass_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableFilterBypass_IOCNT4(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS4);
}

/**
 * @brief Indicates whether bypass filter for IOCNT4 is enabled.
 * @rmtoll CTRL   FLT_BYPASS4  LL_IOCNT_IsEnabledFilterBypass_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledFilterBypass_IOCNT4(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_FLT_BYPASS4) == IOCNT_CTRL_FLT_BYPASS4);
}

/**
 * @brief Configure all counter type for IOCNT0~IOCNT4.
 * @note  Configure all counter include input and output counter of IOCNT0~IOCNT4.
 * @note  The parameter Type can be LL_IOCNT_CNTTYPE_R2CLR or LL_IOCNT_CNTTYPE_COUNT,
 *        LL_IOCNT_CNTTYPE_R2CLR: Counter of IOCNT0~IOCNT4 will be clear after read.
 *        LL_IOCNT_CNTTYPE_COUNT: Counter of IOCNT0~IOCNT4 will continue to count afrer read.
 * @rmtoll CTRL   CNT_CLR   LL_IOCNT_SetCounterType
 * @param IOCNTx  IOCNT Instance
 * @param Type    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_CNTTYPE_R2CLR
 *         @arg @ref LL_IOCNT_CNTTYPE_COUNT
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetCounterType(IOCNT_TypeDef *IOCNTx, uint32_t Type)
{
    MODIFY_REG(IOCNTx->CTRL, IOCNT_CTRL_CNT_CLR, (Type << IOCNT_CTRL_CNT_CLR_POS));
}

/**
 * @brief Return counter type for IOCNT0~IOCNT4.
 * @rmtoll CTRL   CNT_CLR   LL_IOCNT_GetCounterType
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_CNTTYPE_R2CLR
 *         @arg @ref LL_IOCNT_CNTTYPE_COUNT
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetCounterType(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_CNT_CLR) >> IOCNT_CTRL_CNT_CLR_POS);
}

/**
 * @brief Configure counter mode for IOCNT0.
 * @note  The parameter Mode can be LL_IOCNT_MODE_RISEEDGE or LL_IOCNT_MODE_FALLEDGE,
 *        LL_IOCNT_MODE_RISEEDGE: rising edge triggers counting.
 *        LL_IOCNT_MODE_FALLEDGE: falling edge triggers counting.
 * @rmtoll CTRL   MODE0   LL_IOCNT_SetCounterMode_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @param Mode    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_MODE_RISEEDGE
 *         @arg @ref LL_IOCNT_MODE_FALLEDGE
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetCounterMode_IOCNT0(IOCNT_TypeDef *IOCNTx, uint32_t Mode)
{
    MODIFY_REG(IOCNTx->CTRL, IOCNT_CTRL_MODE0, (Mode << IOCNT_CTRL_MODE0_POS));
}

/**
 * @brief Return counter mode for IOCNT0.
 * @rmtoll CTRL   MODE0   LL_IOCNT_GetCounterMode_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_MODE_RISEEDGE
 *         @arg @ref LL_IOCNT_MODE_FALLEDGE
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetCounterMode_IOCNT0(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_MODE0) >> IOCNT_CTRL_MODE0_POS);
}

/**
 * @brief Configure counter mode for IOCNT1.
 * @note  The parameter Mode can be LL_IOCNT_MODE_RISEEDGE or LL_IOCNT_MODE_FALLEDGE,
 *        LL_IOCNT_MODE_RISEEDGE: rising edge triggers counting.
 *        LL_IOCNT_MODE_FALLEDGE: falling edge triggers counting.
 * @rmtoll CTRL   MODE1   LL_IOCNT_SetCounterMode_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @param Mode    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_MODE_RISEEDGE
 *         @arg @ref LL_IOCNT_MODE_FALLEDGE
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetCounterMode_IOCNT1(IOCNT_TypeDef *IOCNTx, uint32_t Mode)
{
    MODIFY_REG(IOCNTx->CTRL, IOCNT_CTRL_MODE1, (Mode << IOCNT_CTRL_MODE1_POS));
}

/**
 * @brief Return counter mode for IOCNT1.
 * @rmtoll CTRL   MODE1   LL_IOCNT_GetCounterMode_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_MODE_RISEEDGE
 *         @arg @ref LL_IOCNT_MODE_FALLEDGE
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetCounterMode_IOCNT1(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_MODE1) >> IOCNT_CTRL_MODE1_POS);
}

/**
 * @brief Configure counter mode for IOCNT2.
 * @note  The parameter Mode can be LL_IOCNT_MODE_RISEEDGE or LL_IOCNT_MODE_FALLEDGE,
 *        LL_IOCNT_MODE_RISEEDGE: rising edge triggers counting.
 *        LL_IOCNT_MODE_FALLEDGE: falling edge triggers counting.
 * @rmtoll CTRL   MODE2   LL_IOCNT_SetCounterMode_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @param Mode    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_MODE_RISEEDGE
 *         @arg @ref LL_IOCNT_MODE_FALLEDGE
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetCounterMode_IOCNT2(IOCNT_TypeDef *IOCNTx, uint32_t Mode)
{
    MODIFY_REG(IOCNTx->CTRL, IOCNT_CTRL_MODE2, (Mode << IOCNT_CTRL_MODE2_POS));
}

/**
 * @brief Return counter mode for IOCNT2.
 * @rmtoll CTRL   MODE2   LL_IOCNT_GetCounterMode_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_MODE_RISEEDGE
 *         @arg @ref LL_IOCNT_MODE_FALLEDGE
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetCounterMode_IOCNT2(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_MODE2) >> IOCNT_CTRL_MODE2_POS);
}

/**
 * @brief Configure counter mode for IOCNT3.
 * @note  The parameter Mode can be LL_IOCNT_MODE_RISEEDGE or LL_IOCNT_MODE_FALLEDGE,
 *        LL_IOCNT_MODE_RISEEDGE: rising edge triggers counting.
 *        LL_IOCNT_MODE_FALLEDGE: falling edge triggers counting.
 * @rmtoll CTRL   MODE3   LL_IOCNT_SetCounterMode_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @param Mode    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_MODE_RISEEDGE
 *         @arg @ref LL_IOCNT_MODE_FALLEDGE
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetCounterMode_IOCNT3(IOCNT_TypeDef *IOCNTx, uint32_t Mode)
{
    MODIFY_REG(IOCNTx->CTRL, IOCNT_CTRL_MODE3, (Mode << IOCNT_CTRL_MODE3_POS));
}

/**
 * @brief Return counter mode for IOCNT3.
 * @rmtoll CTRL   MODE3   LL_IOCNT_GetCounterMode_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_MODE_RISEEDGE
 *         @arg @ref LL_IOCNT_MODE_FALLEDGE
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetCounterMode_IOCNT3(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_MODE3) >> IOCNT_CTRL_MODE3_POS);
}

/**
 * @brief Configure counter mode for IOCNT4.
 * @note  The parameter Mode can be LL_IOCNT_MODE_RISEEDGE or LL_IOCNT_MODE_FALLEDGE,
 *        LL_IOCNT_MODE_RISEEDGE: rising edge triggers counting.
 *        LL_IOCNT_MODE_FALLEDGE: falling edge triggers counting.
 * @rmtoll CTRL   MODE4   LL_IOCNT_SetCounterMode_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @param Mode    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_MODE_RISEEDGE
 *         @arg @ref LL_IOCNT_MODE_FALLEDGE
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetCounterMode_IOCNT4(IOCNT_TypeDef *IOCNTx, uint32_t Mode)
{
    MODIFY_REG(IOCNTx->CTRL, IOCNT_CTRL_MODE4, (Mode << IOCNT_CTRL_MODE4_POS));
}

/**
 * @brief Return counter mode for IOCNT4.
 * @rmtoll CTRL   MODE4   LL_IOCNT_GetCounterMode_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_MODE4_RISEEDGE
 *         @arg @ref LL_IOCNT_MODE4_FALLEDGE
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetCounterMode_IOCNT4(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_MODE4) >> IOCNT_CTRL_MODE4_POS);
}

/**
 * @brief Enable IOCNT0.
 * @rmtoll CTRL   EN0   LL_IOCNT_EnableIOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableIOCNT0(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN0);
}

/**
 * @brief Disable IOCNT0.
 * @rmtoll CTRL   EN0   LL_IOCNT_DisableIOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableIOCNT0(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN0);
}

/**
 * @brief Indicates whether IOCNT0 is enabled.
 * @rmtoll CTRL   FLT_BYPASS0  LL_IOCNT_IsEnabledIOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledIOCNT0(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN0) == IOCNT_CTRL_EN0);
}

/**
 * @brief Enable IOCNT1.
 * @rmtoll CTRL   EN1   LL_IOCNT_EnableIOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableIOCNT1(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN1);
}

/**
 * @brief Disable IOCNT1.
 * @rmtoll CTRL   EN1   LL_IOCNT_DisableIOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableIOCNT1(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN1);
}

/**
 * @brief Indicates whether IOCNT1 is enabled.
 * @rmtoll CTRL   EN1  LL_IOCNT_IsEnabledIOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledIOCNT1(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN1) == IOCNT_CTRL_EN1);
}

/**
 * @brief Enable IOCNT2.
 * @rmtoll CTRL   EN2   LL_IOCNT_EnableIOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableIOCNT2(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN2);
}

/**
 * @brief Disable IOCNT2.
 * @rmtoll CTRL   EN2   LL_IOCNT_DisableIOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableIOCNT2(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN2);
}

/**
 * @brief Indicates whether IOCNT2 is enabled.
 * @rmtoll CTRL   EN2  LL_IOCNT_IsEnabledIOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledIOCNT2(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN2) == IOCNT_CTRL_EN2);
}

/**
 * @brief Enable IOCNT3.
 * @rmtoll CTRL   EN3   LL_IOCNT_EnableIOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableIOCNT3(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN3);
}

/**
 * @brief Disable IOCNT3.
 * @rmtoll CTRL   EN3   LL_IOCNT_DisableIOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableIOCNT3(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN3);
}

/**
 * @brief Indicates whether IOCNT3 is enabled.
 * @rmtoll CTRL   EN3  LL_IOCNT_IsEnabledIOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledIOCNT3(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN3) == IOCNT_CTRL_EN3);
}

/**
 * @brief Enable IOCNT4.
 * @rmtoll CTRL   EN4   LL_IOCNT_EnableIOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_EnableIOCNT4(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN4);
}

/**
 * @brief Disable IOCNT4.
 * @rmtoll CTRL   EN4   LL_IOCNT_DisableIOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_DisableIOCNT4(IOCNT_TypeDef *IOCNTx)
{
    CLEAR_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN4);
}

/**
 * @brief Indicates whether IOCNT4 is enabled.
 * @rmtoll CTRL   EN4  LL_IOCNT_IsEnabledIOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return State of bit, 0: disabled, 1: enabled.
 */
__STATIC_INLINE uint32_t LL_IOCNT_IsEnabledIOCNT4(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->CTRL, IOCNT_CTRL_EN4) == IOCNT_CTRL_EN4);
}

/**
 * @brief Configure output signal type for IOCNT0.
 * @note  The output type can be LL_IOCNT_COPL_0 or LL_IOCNT_COPL_1,
 *        LL_IOCNT_COPL_0: Output the (n+1)th pulse at the (n*Prescaler+1)th input pulse.
 *        LL_IOCNT_COPL_1: Output the (n)th pulse at the (n*Prescaler)th input pulse.
 * @rmtoll CTL1   COPL0   LL_IOCNT_SetOutputType_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @param Type    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_COPL_0
 *         @arg @ref LL_IOCNT_COPL_1
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetOutputType_IOCNT0(IOCNT_TypeDef *IOCNTx, uint32_t Type)
{
    MODIFY_REG(IOCNTx->COPL, IOCNT_COPL_COPL0, (Type << IOCNT_COPL_COPL0_POS));
}

/**
 * @brief Return output signal type for IOCNT0.
 * @note  The output type can be LL_IOCNT_COPL_0 or LL_IOCNT_COPL_1,
 *        LL_IOCNT_COPL_0: Output the (n+1)th pulse at the (n*Prescaler+1)th input pulse.
 *        LL_IOCNT_COPL_1: Output the (n)th pulse at the (n*Prescaler)th input pulse.
 * @rmtoll CTL1   COPL0   LL_IOCNT_GetOutputType_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_COPL_0
 *         @arg @ref LL_IOCNT_COPL_1
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetOutputType_IOCNT0(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->COPL, IOCNT_COPL_COPL0) >> IOCNT_COPL_COPL0_POS);
}

/**
 * @brief Configure output signal type for IOCNT1.
 * @note  The output type can be LL_IOCNT_COPL_0 or LL_IOCNT_COPL_1,
 *        LL_IOCNT_COPL_0: Output the (n+1)th pulse at the (n*Prescaler+1)th input pulse.
 *        LL_IOCNT_COPL_1: Output the (n)th pulse at the (n*Prescaler)th input pulse.
 * @rmtoll CTL1   COPL1   LL_IOCNT_SetOutputType_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @param Type    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_COPL_0
 *         @arg @ref LL_IOCNT_COPL_1
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetOutputType_IOCNT1(IOCNT_TypeDef *IOCNTx, uint32_t Type)
{
    MODIFY_REG(IOCNTx->COPL, IOCNT_COPL_COPL1, (Type << IOCNT_COPL_COPL1_POS));
}

/**
 * @brief Return output signal type for IOCNT1.
 * @note  The output type can be LL_IOCNT_COPL_0 or LL_IOCNT_COPL_1,
 *        LL_IOCNT_COPL_0: Output the (n+1)th pulse at the (n*Prescaler+1)th input pulse.
 *        LL_IOCNT_COPL_1: Output the (n)th pulse at the (n*Prescaler)th input pulse.
 * @rmtoll CTL1   COPL1   LL_IOCNT_GetOutputType_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_COPL_0
 *         @arg @ref LL_IOCNT_COPL_1
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetOutputType_IOCNT1(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->COPL, IOCNT_COPL_COPL1) >> IOCNT_COPL_COPL1_POS);
}

/**
 * @brief Configure output signal type for IOCNT2.
 * @note  The output type can be LL_IOCNT_COPL_0 or LL_IOCNT_COPL_1,
 *        LL_IOCNT_COPL_0: Output the (n+1)th pulse at the (n*Prescaler+1)th input pulse.
 *        LL_IOCNT_COPL_1: Output the (n)th pulse at the (n*Prescaler)th input pulse.
 * @rmtoll CTL1   COPL2   LL_IOCNT_SetOutputType_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @param Type    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_COPL_0
 *         @arg @ref LL_IOCNT_COPL_1
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetOutputType_IOCNT2(IOCNT_TypeDef *IOCNTx, uint32_t Type)
{
    MODIFY_REG(IOCNTx->COPL, IOCNT_COPL_COPL2, (Type << IOCNT_COPL_COPL2_POS));
}

/**
 * @brief Return output signal type for IOCNT2.
 * @note  The output type can be LL_IOCNT_COPL_0 or LL_IOCNT_COPL_1,
 *        LL_IOCNT_COPL_0: Output the (n+1)th pulse at the (n*Prescaler+1)th input pulse.
 *        LL_IOCNT_COPL_1: Output the (n)th pulse at the (n*Prescaler)th input pulse.
 * @rmtoll CTL1   COPL2   LL_IOCNT_GetOutputType_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_COPL_0
 *         @arg @ref LL_IOCNT_COPL_1
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetOutputType_IOCNT2(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->COPL, IOCNT_COPL_COPL2) >> IOCNT_COPL_COPL2_POS);
}

/**
 * @brief Configure output signal type for IOCNT3.
 * @note  The output type can be LL_IOCNT_COPL_0 or LL_IOCNT_COPL_1,
 *        LL_IOCNT_COPL_0: Output the (n+1)th pulse at the (n*Prescaler+1)th input pulse.
 *        LL_IOCNT_COPL_1: Output the (n)th pulse at the (n*Prescaler)th input pulse.
 * @rmtoll CTL1   COPL3   LL_IOCNT_SetOutputType_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @param Type    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_COPL_0
 *         @arg @ref LL_IOCNT_COPL_1
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetOutputType_IOCNT3(IOCNT_TypeDef *IOCNTx, uint32_t Type)
{
    MODIFY_REG(IOCNTx->COPL, IOCNT_COPL_COPL3, (Type << IOCNT_COPL_COPL3_POS));
}

/**
 * @brief Return output signal type for IOCNT3.
 * @note  The output type can be LL_IOCNT_COPL_0 or LL_IOCNT_COPL_1,
 *        LL_IOCNT_COPL_0: Output the (n+1)th pulse at the (n*Prescaler+1)th input pulse.
 *        LL_IOCNT_COPL_1: Output the (n)th pulse at the (n*Prescaler)th input pulse.
 * @rmtoll CTL1   COPL3   LL_IOCNT_GetOutputType_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_COPL_0
 *         @arg @ref LL_IOCNT_COPL_1
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetOutputType_IOCNT3(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->COPL, IOCNT_COPL_COPL3) >> IOCNT_COPL_COPL3_POS);
}

/**
 * @brief Configure output signal type for IOCNT4.
 * @note  The output type can be LL_IOCNT_COPL_0 or LL_IOCNT_COPL_1,
 *        LL_IOCNT_COPL_0: Output the (n+1)th pulse at the (n*Prescaler+1)th input pulse.
 *        LL_IOCNT_COPL_1: Output the (n)th pulse at the (n*Prescaler)th input pulse.
 * @rmtoll CTL1   COPL4   LL_IOCNT_SetOutputType_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @param Type    This parameter can be one of the following values:
 *         @arg @ref LL_IOCNT_COPL_0
 *         @arg @ref LL_IOCNT_COPL_1
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetOutputType_IOCNT4(IOCNT_TypeDef *IOCNTx, uint32_t Type)
{
    MODIFY_REG(IOCNTx->COPL, IOCNT_COPL_COPL4, (Type << IOCNT_COPL_COPL4_POS));
}

/**
 * @brief Return output signal type for IOCNT4.
 * @note  The output type can be LL_IOCNT_COPL_0 or LL_IOCNT_COPL_1,
 *        LL_IOCNT_COPL_0: Output the (n+1)th pulse at the (n*Prescaler+1)th input pulse.
 *        LL_IOCNT_COPL_1: Output the (n)th pulse at the (n*Prescaler)th input pulse.
 * @rmtoll CTL1   COPL4   LL_IOCNT_GetOutputType_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_IOCNT_COPL_0
 *         @arg @ref LL_IOCNT_COPL_1
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetOutputType_IOCNT4(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_BIT(IOCNTx->COPL, IOCNT_COPL_COPL4) >> IOCNT_COPL_COPL4_POS);
}

/**
 * @brief Trigger an input pulse by internal command for IOCNT0.
 * @note  Warning: The filtering function must be bypassed when use internal command.
 * @rmtoll CMD   IOCNT0_KICK   LL_IOCNT_TriggerInput_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_TriggerInput_IOCNT0(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CMD, IOCNT_CMD_IOCNT0_KICK);
}

/**
 * @brief Trigger an input pulse by internal command for IOCNT1.
 * @note  Warning: The filtering function must be bypassed when use internal command.
 * @rmtoll CMD   IOCNT1_KICK   LL_IOCNT_TriggerInput_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_TriggerInput_IOCNT1(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CMD, IOCNT_CMD_IOCNT1_KICK);
}

/**
 * @brief Trigger an input pulse by internal command for IOCNT2.
 * @note  Warning: The filtering function must be bypassed when use internal command.
 * @rmtoll CMD   IOCNT2_KICK   LL_IOCNT_TriggerInput_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_TriggerInput_IOCNT2(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CMD, IOCNT_CMD_IOCNT2_KICK);
}

/**
 * @brief Trigger an input pulse by internal command for IOCNT3.
 * @note  Warning: The filtering function must be bypassed when use internal command.
 * @rmtoll CMD   IOCNT3_KICK   LL_IOCNT_TriggerInput_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_TriggerInput_IOCNT3(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CMD, IOCNT_CMD_IOCNT3_KICK);
}

/**
 * @brief Trigger an input pulse by internal command for IOCNT4.
 * @note  Warning: The filtering function must be bypassed when use internal command.
 * @rmtoll CMD   IOCNT4_KICK   LL_IOCNT_TriggerInput_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_TriggerInput_IOCNT4(IOCNT_TypeDef *IOCNTx)
{
    SET_BIT(IOCNTx->CMD, IOCNT_CMD_IOCNT4_KICK);
}

/**
 * @brief Configure Remainder of counter for IOCNT0.
 * @note  Warning: When IOCNT is enabled, Remainder register is read only.
 *        When IOCNT is disabled, Remainder register will be cleared automatically, and it can be writed.
 * @note  REMD = (CNT_in + Remainder) % (prescaler + 1)
 * @note  Configure the remainder and restart the IOCNT, the next output pulse count will accumulate on this remainder.
 * @rmtoll REMD0   REMD0   LL_IOCNT_SetRemainder_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @param Remainder Counter remainder
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetRemainder_IOCNT0(IOCNT_TypeDef *IOCNTx, uint32_t Remainder)
{
    WRITE_REG(IOCNTx->REMD0, (Remainder & IOCNT_REMD0_REMD0));
}

/**
 * @brief Return Remainder of counter for IOCNT0.
 * @note  Warning: When IOCNT is enabled, Remainder register is read only.
 *        When IOCNT is disabled, Remainder register will be cleared automatically, and it can be writed.
 * @note  REMD = (CNT_in + Remainder) % (prescaler + 1)
 * @note  Configure the remainder and restart the IOCNT, the next output pulse count will accumulate on this remainder.
 * @rmtoll REMD0   REMD0   LL_IOCNT_GetRemainder_IOCNT0
 * @param IOCNTx  IOCNT Instance
 * @return Remainder of counter for IOCNT0
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetRemainder_IOCNT0(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_REG(IOCNTx->REMD0));
}

/**
 * @brief Configure Remainder of counter for IOCNT1.
 * @note  Warning: When IOCNT is enabled, Remainder register is read only.
 *        When IOCNT is disabled, Remainder register will be cleared automatically, and it can be writed.
 * @note  REMD = (CNT_in + Remainder) % (prescaler + 1)
 * @note  Configure the remainder and restart the IOCNT, the next output pulse count will accumulate on this remainder.
 * @rmtoll REMD1   REMD1   LL_IOCNT_SetRemainder_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @param Remainder Counter remainder
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetRemainder_IOCNT1(IOCNT_TypeDef *IOCNTx, uint32_t Remainder)
{
    WRITE_REG(IOCNTx->REMD1, (Remainder & IOCNT_REMD1_REMD1));
}

/**
 * @brief Return Remainder of counter for IOCNT1.
 * @note  Warning: When IOCNT is enabled, Remainder register is read only.
 *        When IOCNT is disabled, Remainder register will be cleared automatically, and it can be writed.
 * @note  REMD = (CNT_in + Remainder) % (prescaler + 1)
 * @note  Configure the remainder and restart the IOCNT, the next output pulse count will accumulate on this remainder.
 * @rmtoll REMD1   REMD1   LL_IOCNT_GetRemainder_IOCNT1
 * @param IOCNTx  IOCNT Instance
 * @return Remainder of counter for IOCNT1
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetRemainder_IOCNT1(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_REG(IOCNTx->REMD1));
}

/**
 * @brief Configure Remainder of counter for IOCNT2.
 * @note  Warning: When IOCNT is enabled, Remainder register is read only.
 *        When IOCNT is disabled, Remainder register will be cleared automatically, and it can be writed.
 * @note  REMD = (CNT_in + Remainder) % (prescaler + 1)
 * @note  Configure the remainder and restart the IOCNT, the next output pulse count will accumulate on this remainder.
 * @rmtoll REMD2   REMD2   LL_IOCNT_SetRemainder_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @param Remainder Counter remainder
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetRemainder_IOCNT2(IOCNT_TypeDef *IOCNTx, uint32_t Remainder)
{
    WRITE_REG(IOCNTx->REMD2, (Remainder & IOCNT_REMD2_REMD2));
}

/**
 * @brief Return Remainder of counter for IOCNT2.
 * @note  Warning: When IOCNT is enabled, Remainder register is read only.
 *        When IOCNT is disabled, Remainder register will be cleared automatically, and it can be writed.
 * @note  REMD = (CNT_in + Remainder) % (prescaler + 1)
 * @note  Configure the remainder and restart the IOCNT, the next output pulse count will accumulate on this remainder.
 * @rmtoll REMD2   REMD2   LL_IOCNT_GetRemainder_IOCNT2
 * @param IOCNTx  IOCNT Instance
 * @return Remainder of counter for IOCNT2
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetRemainder_IOCNT2(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_REG(IOCNTx->REMD2));
}

/**
 * @brief Configure Remainder of counter for IOCNT3.
 * @note  Warning: When IOCNT is enabled, Remainder register is read only.
 *        When IOCNT is disabled, Remainder register will be cleared automatically, and it can be writed.
 * @note  REMD = (CNT_in + Remainder) % (prescaler + 1)
 * @note  Configure the remainder and restart the IOCNT, the next output pulse count will accumulate on this remainder.
 * @rmtoll REMD3   REMD3   LL_IOCNT_SetRemainder_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @param Remainder Counter remainder
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetRemainder_IOCNT3(IOCNT_TypeDef *IOCNTx, uint32_t Remainder)
{
    WRITE_REG(IOCNTx->REMD3, (Remainder & IOCNT_REMD3_REMD3));
}

/**
 * @brief Return Remainder of counter for IOCNT3.
 * @note  Warning: When IOCNT is enabled, Remainder register is read only.
 *        When IOCNT is disabled, Remainder register will be cleared automatically, and it can be writed.
 * @note  REMD = (CNT_in + Remainder) % (prescaler + 1)
 * @note  Configure the remainder and restart the IOCNT, the next output pulse count will accumulate on this remainder.
 * @rmtoll REMD3   REMD3   LL_IOCNT_GetRemainder_IOCNT3
 * @param IOCNTx  IOCNT Instance
 * @return Remainder of counter for IOCNT3
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetRemainder_IOCNT3(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_REG(IOCNTx->REMD3));
}

/**
 * @brief Configure Remainder of counter for IOCNT4.
 * @note  Warning: When IOCNT is enabled, Remainder register is read only.
 *        When IOCNT is disabled, Remainder register will be cleared automatically, and it can be writed.
 * @note  REMD = (CNT_in + Remainder) % (prescaler + 1)
 * @note  Configure the remainder and restart the IOCNT, the next output pulse count will accumulate on this remainder.
 * @rmtoll REMD4   REMD4   LL_IOCNT_SetRemainder_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @param Remainder Counter remainder
 * @return None
 */
__STATIC_INLINE void LL_IOCNT_SetRemainder_IOCNT4(IOCNT_TypeDef *IOCNTx, uint32_t Remainder)
{
    WRITE_REG(IOCNTx->REMD4, (Remainder & IOCNT_REMD4_REMD4));
}

/**
 * @brief Return Remainder of counter for IOCNT4.
 * @note  Warning: When IOCNT is enabled, Remainder register is read only.
 *        When IOCNT is disabled, Remainder register will be cleared automatically, and it can be writed.
 * @note  REMD = (CNT_in + Remainder) % (prescaler + 1)
 * @note  Configure the remainder and restart the IOCNT, the next output pulse count will accumulate on this remainder.
 * @rmtoll REMD4   REMD4   LL_IOCNT_GetRemainder_IOCNT4
 * @param IOCNTx  IOCNT Instance
 * @return Remainder of counter for IOCNT4
 */
__STATIC_INLINE uint32_t LL_IOCNT_GetRemainder_IOCNT4(IOCNT_TypeDef *IOCNTx)
{
    return (uint32_t)(READ_REG(IOCNTx->REMD4));
}
/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup IOCNT_LL_EF_Init Initialization and de-initialization functions
 * @{
 */
ErrorStatus LL_IOCNT_DeInit(IOCNT_TypeDef *IOCNTx);
ErrorStatus LL_IOCNT_Init(IOCNT_TypeDef *IOCNTx, LL_IOCNT_InitTypeDef *IOCNT_InitStruct, uint32_t Iocnt_ch);
void        LL_IOCNT_StructInit(LL_IOCNT_InitTypeDef *IOCNT_InitStruct);
/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */

/**
 * @}
 */

#endif /* IOCNT */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_IOCNT_H */
