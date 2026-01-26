/**
 *****************************************************************************
 * @file rn_rv_ll_tc.h
 * @author Renergy Software Team
 * @brief Header file of TC LL module.
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
#ifndef __RN_LL_TC_H
#define __RN_LL_TC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_tc.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined(TC0_CORE0) || defined(TC1_CORE0) || defined(TC0_CORE1) || defined(TC1_CORE1)

/** @defgroup TC_LL TC
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/** @defgroup TC_LL_Private_Variables TC Private Variables
 * @{
 */
static const uint8_t OFFSET_TAB_CC_CHx_REG[] =
{
    0x00U,   /* 0: TCx_CH0 */
    0x04U    /* 1: TCx_CH1 */
};
/**
 * @}
 */

/* Private constants --------------------------------------------------------*/
/** @defgroup TC_LL_Private_Constants TC Private Constants
 * @{
 */
/* Defines used to perform compute and check in the macros *******************/
#define LL_TC_TIMEOUT_DEFAULT           1000U
/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup TC_LL_Private_Macros TC Private Macros
 * @{
 */

/**
 * @brief  Check if TC instance is valid.
 * @param  __INSTANCE__ TC instance
 * @retval SET (__INSTANCE__ is valid) or RESET (__INSTANCE__ is invalid)
 */
#define IS_LL_TC_ALL_INSTANCE(__INSTANCE__)     ((((__INSTANCE__) == TC0_CORE0)) || \
                                                 (((__INSTANCE__) == TC1_CORE0)) || \
                                                 (((__INSTANCE__) == TC0_CORE1)) || \
                                                 (((__INSTANCE__) == TC1_CORE1)))

#define IS_LL_TC_PRESCALER(__VALUE__)           ((__VALUE__) <= 0xFFFFU)

#define IS_LL_TC_AUTORELOAD(__VALUE__)          ((__VALUE__) <= 0xFFFFFFFFU)

#define IS_LL_TC_ONEPULSEMODE(__VALUE__)        (((__VALUE__) == LL_TC_ONEPULSEMODE_SINGLE) || \
                                                 ((__VALUE__) == LL_TC_ONEPULSEMODE_REPETITIVE))

#define IS_LL_TC_CLOCKSOURCE(__VALUE__)         (((__VALUE__) == LL_TC_CLOCKSOURCE_INTERNAL) || \
                                                 ((__VALUE__) == LL_TC_CLOCKSOURCE_EXTERNAL))

#define IS_LL_TC_EXTCLKMODE(__VALUE__)          (((__VALUE__) == LL_TC_EXTCLKMODE_POSEDGE) || \
                                                 ((__VALUE__) == LL_TC_EXTCLKMODE_NEGEDGE) || \
                                                 ((__VALUE__) == LL_TC_EXTCLKMODE_BOTHEDGE))

#define IS_LL_TC_SLAVEMODE(__VALUE__)           (((__VALUE__) == LL_TC_SLAVEMODE_DISABLED)   || \
                                                 ((__VALUE__) == LL_TC_SLAVEMODE_RESET)      || \
                                                 ((__VALUE__) == LL_TC_SLAVEMODE_GATED_HIGH) || \
                                                 ((__VALUE__) == LL_TC_SLAVEMODE_GATED_LOW)  || \
                                                 ((__VALUE__) == LL_TC_SLAVEMODE_TRIGGER))

#define IS_LL_TC_CHANNEL(__VALUE__)             (((__VALUE__) == LL_TC_CHANNEL_CH0) || \
                                                 ((__VALUE__) == LL_TC_CHANNEL_CH1))

#define IS_LL_TC_OCMODE(__VALUE__)              (((__VALUE__) == LL_TC_OCMODE_TRISTATE)        || \
                                                 ((__VALUE__) == LL_TC_OCMODE_ACTIVE)          || \
                                                 ((__VALUE__) == LL_TC_OCMODE_INACTIVE)        || \
                                                 ((__VALUE__) == LL_TC_OCMODE_TOGGLE)          || \
                                                 ((__VALUE__) == LL_TC_OCMODE_FORCED_ACTIVE)   || \
                                                 ((__VALUE__) == LL_TC_OCMODE_FORCED_INACTIVE) || \
                                                 ((__VALUE__) == LL_TC_OCMODE_PWM1)            || \
                                                 ((__VALUE__) == LL_TC_OCMODE_PWM2))

#define IS_LL_TC_OCSTATE(__VALUE__)             (((__VALUE__) == LL_TC_OCSTATE_DISABLE) || \
                                                 ((__VALUE__) == LL_TC_OCSTATE_ENABLE))

#define IS_LL_TC_COMPAREVALUE(__VALUE__)        ((__VALUE__) <= 0xFFFFFFFFU)

#define IS_LL_TC_POLARITY(__VALUE__)            (((__VALUE__) == LL_TC_OCPOLARITY_LOW) || \
                                                 ((__VALUE__) == LL_TC_OCPOLARITY_HIGH))

#define IS_LL_TC_OCIDLESTATE(__VALUE__)         (((__VALUE__) == LL_TC_OCIDLESTATE_LOW) || \
                                                 ((__VALUE__) == LL_TC_OCIDLESTATE_HIGH))

#define IS_LL_TC_DEADTIME(__VALUE__)            ((__VALUE__) <= 0x3FU)

#define IS_LL_TC_IC_FILTER(__VALUE__)           ((__VALUE__) <= 0xFFU)

#define IS_LL_TC_IC_POLARITY(__VALUE__)         (((__VALUE__) == LL_TC_IC_POLARITY_RISING) || \
                                                 ((__VALUE__) == LL_TC_IC_POLARITY_FALLING))

#define IS_LL_TC_PWMMOD_CONTROLMODE(__VALUE__)  (((__VALUE__) == LL_TC_PWMMOD_CONTROLMODE_SOFTWARE) || \
                                                 ((__VALUE__) == LL_TC_PWMMOD_CONTROLMODE_DMA))

#define IS_LL_TC_PWMMOD_DURATIONMODE(__VALUE__) (((__VALUE__) == LL_TC_PWMMOD_DURATIONMODE_CONTINUOUS) || \
                                                 ((__VALUE__) == LL_TC_PWMMOD_DURATIONMODE_LENGTH))

#define IS_LL_TC_PWMMOD_STOPMODE(__VALUE__)     (((__VALUE__) == LL_TC_PWMMOD_STOPMODE_COMPLETE) || \
                                                 ((__VALUE__) == LL_TC_PWMMOD_STOPMODE_BREAK))

#define IS_LL_TC_PWMMOD_WAVELENGTH(__VALUE__)   ((__VALUE__) <= 0x3FFU)

#define IS_LL_TC_PWMMOD_PULSEWIDTH(__VALUE__)   ((__VALUE__) <= 0xFFFFFFFFU)

#define IS_LL_TC_PWMMOD_PULSEPERIOD(__VALUE__)  ((__VALUE__) <= 0x0000FFFFU)
/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup TC_LL_ES_INIT TC Exported Init structure
 * @{
 */

/**
 * @brief  TC Time base configuration structure definition.
 */
typedef struct
{
    uint16_t Prescaler;         /*!< Specifies the prescaler value used to divide the TC clock.
                                     This parameter can be a number between Min_Data=0x0000 and Max_Data=0xFFFF.

                                     This feature can be modified afterwards using unitary function
                                     @ref LL_TC_SetPrescaler().*/

    uint32_t Autoreload;        /*!< Specifies the auto reload value (target counter value) to be loaded into the active
                                   TC_DN Register.
                                   This parameter must be a number between Min_Data=0x0000 and Max_Data=0xFFFF.
                                   Some timer instances may support 32 bits counters. In that case this parameter must
                                   be a number between 0x0000 and 0xFFFFFFFF.

                                     This feature can be modified afterwards using unitary function
                                     @ref LL_TC_SetAutoReload().*/

    uint32_t DebugMode;            /*!< Specifies the debugmode value.
                                     This parameter can be a value of @ref TC_LL_EC_DebugMode.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_SetDebugMode()}.*/

    uint32_t CH0OCIdleState;       /*!< Specifies the TC channeel 0 Output Compare pin default state during Idle state
                                         before channel 0 compare/capture function enable .
                                     This parameter can be a value of @ref TC_LL_EC_OC_IDLESTATE.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_OC_SetIdleState().*/

    uint32_t CH1OCIdleState;       /*!< Specifies the TC channeel 1 Output Compare pin default state during Idle state
                                         before channel 1 compare/capture function enable .
                                     This parameter can be a value of @ref TC_LL_EC_OC_IDLESTATE.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_OC_SetIdleState().*/

} LL_TC_InitTypeDef;

/**
 * @brief  TC Output Compare configuration structure definition.
 */
typedef struct
{
    uint32_t OCMode;            /*!< Specifies the output duration mode.
                                     This parameter can be a value of @ref TC_LL_EC_OCMODE.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_OC_SetMode().*/

    uint32_t OCState;           /*!< Specifies the TC Output Compare state.
                                     This parameter can be a value of @ref TC_LL_EC_OCSTATE.

                                     This feature can be modified afterwards using unitary functions @ref LL_TC_CC_EnableChannel().*/

    uint32_t CompareValue;      /*!< Specifies the Compare value to be loaded into the Capture Compare Register.
                                     This parameter can be a value between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_OC_SetCompareCH0() or @ref LL_TC_OC_SetCompareCH1().*/

    uint32_t OCPolarity;        /*!< Specifies the output polarity.
                                     This parameter can be a value of @ref TC_LL_EC_OC_POLARITY.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_OC_SetPolarity().*/

    uint32_t DeadTime;          /*!< Specifies the dead time delay.
                                     This parameter can be a value between Min_Data=0x00 and Max_Data=0xFF.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_OC_SetDeadTime().*/

} LL_TC_OC_InitTypeDef;

/**
 * @brief  TC Input Capture configuration structure definition.
 */
typedef struct
{
    uint32_t ICPolarity;        /*!< Specifies the active edge of the input signal.
                                     This parameter can be a value of @ref TC_LL_EC_IC_POLARITY.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_IC_SetPolarity().*/

    uint32_t ICFilter;          /*!< Specifies the input capture filter.
                                     This parameter must be a number between Min_Data=0x00 and Max_Data=0xFF.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_IC_SetFilter().*/

} LL_TC_IC_InitTypeDef;

/**
 * @brief  TC PWM Modulation configuration structure definition.
 */
typedef struct
{
    uint32_t PWMMODMode;        /*!< Specifies the PWM control mode.
                                     This parameter can be a value of @ref TC_LL_EC_PWMMOD_CONTROLMODE.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_PWMMOD_SetControlMode().*/

    uint32_t StopMode;          /*!< Specifies the signal integrity selection.
                                     This parameter can be a value of @ref TC_LL_EC_PWMMOD_STOPMODE.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_PWMMOD_SetStopMode().*/

    uint32_t DurationMode;      /*!< Specifies the output signal mode.
                                     This parameter can be a value of @ref TC_LL_EC_PWMMOD_DURATIONMODE.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_PWMMOD_SetOutputDurationMode().*/

    uint32_t WaveLength;        /*!< Specifies Number of PWM wave cycles (WaveLength = output cycle number - 1).
                                     This parameter can be a value between Min_Data=0x0 and Max_Data=0x3FF.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_PWMMOD_SetWaveLength().*/

    uint32_t PulseWidth;       /*!< Specifies the pulse length.
                                     This parameter can be a value between Min_Data=0x0 and Max_Data=0xFFFFFFFF.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_OC_SetCompareCH1().*/

    uint32_t PulsePeriod;      /*!< Specifies the pulse period length.
                                     This parameter can be a value between Min_Data=0x00000000 and Max_Data=0xFFFF.

                                     This feature can be modified afterwards using unitary function @ref LL_TC_PWMMOD_SetPeriod().*/

} LL_TC_PWMMOD_InitTypeDef;
/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup TC_LL_Exported_Constants TC Exported Constants
 * @{
 */

/** @defgroup TC_LL_EC_CHANNEL TC Channel
 * @{
 */
#define LL_TC_CHANNEL_CH0                   0x00000000U                 /*!< TC Channel 0 */
#define LL_TC_CHANNEL_CH1                   0x00000001U                 /*!< TC Channel 1 */
/**
 * @}
 */

/** @defgroup TC_LL_EC_ONEPULSEMODE TC One Pulse Mode
 * @{
 */
#define LL_TC_ONEPULSEMODE_SINGLE           TC_CTRL_OPS                 /*!< Counter stops counting at chounting to target */
#define LL_TC_ONEPULSEMODE_REPETITIVE       0x00000000U                 /*!< Counter restart counting at chounting to target */
/**
 * @}
 */

/** @defgroup TC_LL_EC_DEBUGMODE TC debug mode
 * @{
 */
#define LL_TC_CTRL_DEBUGMODE_PAUSE        TC_CTRL_DBGSTBDIS_PAUSE       /*!< Pause counting during debug */
#define LL_TC_CTRL_DEBUGMODE_COUNTINUE    TC_CTRL_DBGSTBDIS_COUNTINUE   /*!< Continue counting during debug */
/**
 * @}
 */ /* end of group TC_LL_EC_DEBUGMODE */

/** @defgroup TC_LL_EC_CLOCKSOURCE TC Clock Source
 * @{
 */
#define LL_TC_CLOCKSOURCE_INTERNAL          0x00000000U                 /*!< Internal clock source */
#define LL_TC_CLOCKSOURCE_EXTERNAL          TC_CCFG_CM                  /*!< External clock source */
/**
 * @}
 */

/** @defgroup TC_LL_EC_EXTCLKMODE TC External Clock Mode
 * @{
 */
#define LL_TC_EXTCLKMODE_POSEDGE            TC_CCFG_ECLKMODE_PE         /*!< Positive edge trigger */
#define LL_TC_EXTCLKMODE_NEGEDGE            TC_CCFG_ECLKMODE_NE         /*!< Negative edge trigger */
#define LL_TC_EXTCLKMODE_BOTHEDGE           TC_CCFG_ECLKMODE_BE         /*!< Both edges trigger */
/**
 * @}
 */

/** @defgroup TC_LL_EC_SLAVEMODE TC Slave Mode
 * @{
 */
#define LL_TC_SLAVEMODE_DISABLED              0x00000000U                                                      /*!< Slave mode disabled */
#define LL_TC_SLAVEMODE_EDGE_PE              (TC_CTRL_SLVEN | TC_CTRL_SLVTRGMODE_PE)                          /*!< edge trigger mode  - external signal starts counter at posedge */
#define LL_TC_SLAVEMODE_EDGE_NE              (TC_CTRL_SLVEN | TC_CTRL_SLVTRGMODE_NE)                          /*!< edge trigger mode  - external signal starts counter at negedge */
#define LL_TC_SLAVEMODE_EDGE_BE              (TC_CTRL_SLVEN | TC_CTRL_SLVTRGMODE_BE)                          /*!< edge trigger mode  - external signal starts counter at both posedge and negedge */
#define LL_TC_SLAVEMODE_LVL_HIGH             (TC_CTRL_SLVEN | TC_CTRL_SLVTRGMODE_GM| TC_CTRL_SLVGATELVL_HIGH) /*!< level trigger mode - counter counts when external signal is high */
#define LL_TC_SLAVEMODE_LVL_LOW              (TC_CTRL_SLVEN | TC_CTRL_SLVTRGMODE_GM| TC_CTRL_SLVGATELVL_LOW)  /*!< level trigger mode - counter counts when external signal is low */
/**
 * @}
 */

/** @defgroup TC_LL_EC_OCMODE TC Output Compare Mode
 * @{
 */
#define LL_TC_OCMODE_TRISTATE               (TC_CM0_CCM_OC | TC_CM0_OM_NUL)               /*!< Tristate output */
#define LL_TC_OCMODE_ACTIVE                 (TC_CM0_CCM_OC | TC_CM0_OM_VL)                /*!< ACTIVE  level */
#define LL_TC_OCMODE_INACTIVE               (TC_CM0_CCM_OC | TC_CM0_OM_INVL)              /*!< INACTIVE level */
#define LL_TC_OCMODE_TOGGLE                 (TC_CM0_CCM_OC | TC_CM0_OM_TOG)               /*!< Toggle */
#define LL_TC_OCMODE_FORCED_ACTIVE          (TC_CM0_CCM_OC | TC_CM0_OM_FVL)               /*!< Forced ACTIVE  level */
#define LL_TC_OCMODE_FORCED_INACTIVE        (TC_CM0_CCM_OC | TC_CM0_OM_FINVL)             /*!< Forced INACTIVE level */
#define LL_TC_OCMODE_PWM1                   (TC_CM0_CCM_OC | TC_CM0_OM_PWM1)              /*!< PWM mode 1 */
#define LL_TC_OCMODE_PWM2                   (TC_CM0_CCM_OC | TC_CM0_OM_PWM2)              /*!< PWM mode 2 */
/**
 * @}
 */

/** @defgroup TC_LL_EC_OCSTATE TC Output Compare State
 * @{
 */
#define LL_TC_OCSTATE_DISABLE               0x00000000U                 /*!< OCx is not active */
#define LL_TC_OCSTATE_ENABLE                TC_CM0_ENABLE               /*!< OCx signal is output on the corresponding output pin */
/**
 * @}
 */

/** @defgroup TC_LL_EC_OC_POLARITY TC Output Polarity
 * @{
 */
#define LL_TC_OCPOLARITY_LOW                  0x00000000U                 /*!< Active low */
#define LL_TC_OCPOLARITY_HIGH                 TC_CM0_EFELVL               /*!< Active high */
/**
 * @}
 */

/** @defgroup TC_LL_EC_OC_IDLESTATE TC Idle State
 * @{
 */
#define LL_TC_OCIDLESTATE_LOW                0x00000000U                 /*!< Default level low */
#define LL_TC_OCIDLESTATE_HIGH               TC_CM0_DFTLVL               /*!< Default level high */
/**
 * @}
 */

/** @defgroup TC_LL_EC_IC_POLARITY TC Input Capture Polarity
 * @{
 */
#define LL_TC_IC_POLARITY_RISING            (TC_CM0_CCM_IC | TC_CM0_CPOL_PE) /*!< select as input capture Positive edge */
#define LL_TC_IC_POLARITY_FALLING           (TC_CM0_CCM_IC | TC_CM0_CPOL_NE) /*!< select as input capture Negative edge */
#define LL_TC_IC_POLARITY_BOTHEDGE          (TC_CM0_CCM_IC | TC_CM0_CPOL_BE) /*!< select as input capture Both Positive and Negative edge */
/**
 * @}
 */

/** @defgroup TC_LL_EC_PWMMOD_CONTROLMODE TC PWM Modulation Control Mode
 * @{
 */
#define LL_TC_PWMMOD_CONTROLMODE_SOFTWARE   0x00000000U                 /*!< Software control mode */
#define LL_TC_PWMMOD_CONTROLMODE_DMA        TC_PWM_CFG_CMS              /*!< DMA control mode */
/**
 * @}
 */

/** @defgroup TC_LL_EC_PWMMOD_DURATIONMODE TC PWM Modulation output duration mode
 * @{
 */
#define LL_TC_PWMMOD_DURATIONMODE_CONTINUOUS  TC_PWM_CFG_ORV              /*!< Output signal always valid - continuous mode */
#define LL_TC_PWMMOD_DURATIONMODE_LENGTH      0x00000000U                 /*!< Wave length configurable mode */
/**
 * @}
 */

/** @defgroup TC_LL_EC_PWMMOD_STOPMODE TC PWM Modulation stop mode
 * @{
 */
#define LL_TC_PWMMOD_STOPMODE_COMPLETE  0x00000000U               /*!< stop mode - stop after complete cycle */
#define LL_TC_PWMMOD_STOPMODE_BREAK     TC_PWM_CFG_CSS            /*!< Break signal - stop immediately */
/**
 * @}
 */

/** @defgroup TC_LL_EC_PWMMOD_FLAG TC PWM Modulation Flags
 * @{
 */
#define LL_TC_PWMMOD_SR_BUSY                TC_PWM_STA_BUSY             /*!< PWM output busy flag */
#define LL_TC_PWMMOD_SR_DONE                TC_PWM_STA_DONE             /*!< PWM output done interrupt flag */
#define LL_TC_PWMMOD_SR_DATAERR             TC_PWM_STA_DATERR           /*!< PWM data error interrupt flag */
#define LL_TC_PWMMOD_SR_BUFVLD              TC_PWM_STA_BUFVLD           /*!< PWM buffer empty interrupt flag */
#define LL_TC_PWMMOD_SR_CYCDONE             TC_PWM_STA_CYCDONE          /*!< PWM output cycle done interrupt flag */
/**
 * @}
 */

/** @defgroup TC_LL_EC_PWMMOD_IT TC PWM Modulation Interrupt
 * @{
 */
#define LL_TC_PWMMOD_IT_DONE                TC_PWM_CFG_DONEIE           /*!< PWM output done interrupt enable */
#define LL_TC_PWMMOD_IT_DATAERR             TC_PWM_CFG_DATERRIE         /*!< PWM data error interrupt enable */
#define LL_TC_PWMMOD_IT_BUFVLD              TC_PWM_CFG_BUFVLDIE         /*!< PWM buffer empty interrupt enable */
#define LL_TC_PWMMOD_IT_CYCDONE             TC_PWM_CFG_CYCDONEIE        /*!< PWM output cycle done interrupt enable */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup TC_LL_Exported_Macros TC Exported Macros
 * @{
 */

/** @defgroup TC_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */
/**
 * @brief  Write a value in TC register
 * @param  __INSTANCE__ TC Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_TC_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG((__INSTANCE__)->__REG__, (__VALUE__))

/**
 * @brief  Read a value in TC register
 * @param  __INSTANCE__ TC Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_TC_ReadReg(__INSTANCE__, __REG__) READ_REG((__INSTANCE__)->__REG__)

/**
 * @brief  HELPER macro calculating DL[0:7] in the TCx_CMx register to achieve the requested dead time duration.
 * @note ex: @ref __LL_TC_CALC_DEADTIME (18432000, @ref LL_TC_GetPrescaler (), 10);
 * @param  __TCCLK__ timer input clock frequency (in Hz)
 * @param  __DT__ deadtime duration (in us)
 * @retval DL[0:7]
 */
#define __LL_TC_CALC_DEADTIME(__TCCLK__, __PSC__, __DT__)  \
  ((uint32_t)(((uint64_t)(__TCCLK__) * (uint64_t)(__DT__)) \
              / ((uint64_t)1000000U * (uint64_t)((__PSC__) + 1U))))

/**
 * @brief  HELPER macro calculating the prescaler value to achieve the required counter clock frequency.
 * @note ex: @ref __LL_TC_CALC_PSC (18432000, 1000000);
 * @param  __TCCLK__ timer input clock frequency (in Hz)
 * @param  __CNTCLK__ counter clock frequency (in Hz)
 * @retval Prescaler value  (between Min_Data=0 and Max_Data=65535)
 */
#define __LL_TC_CALC_PSC(__TCCLK__, __CNTCLK__)   \
  (((__TCCLK__) >= (__CNTCLK__)) ? (uint32_t)((((__TCCLK__) + (__CNTCLK__)/2U)/(__CNTCLK__)) - 1U) : 0U)

/**
 * @brief  HELPER macro calculating the auto-reload value to achieve the required output signal frequency.
 * @note ex: @ref __LL_TC_CALC_ARR (1000000, @ref LL_TC_GetPrescaler (), 10000);
 * @param  __TCCLK__ timer input clock frequency (in Hz)
 * @param  __PSC__ prescaler
 * @param  __FREQ__ output signal frequency (in Hz)
 * @retval  Auto-reload value  (between Min_Data=0 and Max_Data=0xffffffff)
 */
#define __LL_TC_CALC_ARR(__TCCLK__, __PSC__, __FREQ__) \
  ((((__TCCLK__)/((__PSC__) + 1U)) >= (__FREQ__)) ? (((__TCCLK__)/((__FREQ__) * ((__PSC__) + 1U))) - 1U) : 0U)

/**
 * @brief  HELPER macro calculating the compare value required to achieve the required timer output compare
 *         active/inactive delay.
 * @note ex: @ref __LL_TC_CALC_DELAY (1000000, @ref LL_TC_GetPrescaler (), 10);
 * @param  __TCCLK__ timer input clock frequency (in Hz)
 * @param  __PSC__ prescaler
 * @param  __DELAY__ timer output compare active/inactive delay (in us)
 * @retval Compare value  (between Min_Data=0 and Max_Data=0xffffffff)
 */
#define __LL_TC_CALC_DELAY(__TCCLK__, __PSC__, __DELAY__)  \
  ((uint32_t)(((uint64_t)(__TCCLK__) * (uint64_t)(__DELAY__)) \
              / ((uint64_t)1000000U * (uint64_t)((__PSC__) + 1U))))

/**
 * @brief  HELPER macro calculating the auto-reload value to achieve the required pulse duration
 *         (when the timer operates in one pulse mode).
 * @note ex: @ref __LL_TC_CALC_PULSE (1000000, @ref LL_TC_GetPrescaler (), 10, 20);
 * @param  __TCCLK__ timer input clock frequency (in Hz)
 * @param  __PSC__ prescaler
 * @param  __DELAY__ timer output compare active/inactive delay (in us)
 * @param  __PULSE__ pulse duration (in us)
 * @retval Auto-reload value  (between Min_Data=0 and Max_Data=0xffffffff)
 */
#define __LL_TC_CALC_PULSE(__TCCLK__, __PSC__, __DELAY__, __PULSE__)  \
  ((uint32_t)(__LL_TC_CALC_DELAY((__TCCLK__), (__PSC__), (__PULSE__)) \
              + __LL_TC_CALC_DELAY((__TCCLK__), (__PSC__), (__DELAY__))))
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup TC_LL_Exported_Functions TC Exported Functions
 * @{
 */

/** @defgroup TC_LL_EF_Time_Base Time Base configuration
 * @{
 */

/**
 * @brief  Enable timer counter.
 * @rmtoll CTRL         START         LL_TC_EnableCounter
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_EnableCounter(TC_TypeDef *TCx)
{
    SET_BIT(TCx->CTRL, TC_CTRL_START);
}

/**
 * @brief  Disable timer counter.
 * @rmtoll CTRL         START         LL_TC_DisableCounter
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_DisableCounter(TC_TypeDef *TCx)
{
    CLEAR_BIT(TCx->CTRL, TC_CTRL_START);
}

/**
 * @brief  Indicates whether the timer counter is enabled.
 * @rmtoll CTRL         START         LL_TC_IsEnabledCounter
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_IsEnabledCounter(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->CTRL, TC_CTRL_START) == (TC_CTRL_START)) ? 1UL : 0UL);
}

/**
 * @brief  Set one pulse mode.
 * @rmtoll CTRL         OPS           LL_TC_SetOnePulseMode
 * @param  TCx TC Instance
 * @param  OnePulseMode This parameter can be one of the following values:
 *         @arg @ref LL_TC_ONEPULSEMODE_SINGLE
 *         @arg @ref LL_TC_ONEPULSEMODE_REPETITIVE
 * @retval None
 */
__STATIC_INLINE void LL_TC_SetOnePulseMode(TC_TypeDef *TCx, uint32_t OnePulseMode)
{
    MODIFY_REG(TCx->CTRL, TC_CTRL_OPS, OnePulseMode);
}

/**
 * @brief  Get the actual one pulse mode.
 * @rmtoll CTRL         OPS           LL_TC_GetOnePulseMode
 * @param  TCx TC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_TC_ONEPULSEMODE_SINGLE
 *         @arg @ref LL_TC_ONEPULSEMODE_REPETITIVE
 */
__STATIC_INLINE uint32_t LL_TC_GetOnePulseMode(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_BIT(TCx->CTRL, TC_CTRL_OPS));
}

/**
 * @brief  Set the auto-reload value (target counter value).
 * @rmtoll DN           DN            LL_TC_SetAutoReload
 * @param  TCx TC Instance
 * @param  AutoReload Auto-reload value (between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF)
 * @retval None
 */
__STATIC_INLINE void LL_TC_SetAutoReload(TC_TypeDef *TCx, uint32_t AutoReload)
{
    WRITE_REG(TCx->DN, AutoReload);
}

/**
 * @brief  Get the auto-reload value (target counter value).
 * @rmtoll DN           DN            LL_TC_GetAutoReload
 * @param  TCx TC Instance
 * @retval Auto-reload value (between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF)
 */
__STATIC_INLINE uint32_t LL_TC_GetAutoReload(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_REG(TCx->DN));
}

/**
 * @brief  Get the timer counter counting value.
 * @rmtoll CNT          CNT           LL_TC_GetCounter
 * @param  TCx TC Instance
 * @retval Counter value (between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF)
 */
__STATIC_INLINE uint32_t LL_TC_GetCounter(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_REG(TCx->CNT));
}

/**
 * @brief  Set the prescaler value.
 * @rmtoll PS           PS            LL_TC_SetPrescaler
 * @param  TCx TC Instance
 * @param  Prescaler Prescaler value (between Min_Data=0x0000 and Max_Data=0xFFFF)
 * @retval None
 */
__STATIC_INLINE void LL_TC_SetPrescaler(TC_TypeDef *TCx, uint32_t Prescaler)
{
    WRITE_REG(TCx->PS, (Prescaler & TC_PS_PS));
}

/**
 * @brief  Get the prescaler value.
 * @rmtoll PS           PS            LL_TC_GetPrescaler
 * @param  TCx TC Instance
 * @retval Prescaler value (between Min_Data=0x0000 and Max_Data=0xFFFF)
 */
__STATIC_INLINE uint32_t LL_TC_GetPrescaler(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_REG(TCx->PS));
}

/**
 * @brief  Set the counter mode at debug state.
 * @rmtoll CTRL         DBGSTBDIS            LL_TC_SetDebugMode
 * @param  TCx TC Instance
 * @param  DebugMode This parameter can be one of the following values:
 *         @arg @ref LL_TC_CTRL_DEBUGMODE_PAUSE
 *         @arg @ref LL_TC_CTRL_DEBUGMODE_COUNTINUE
 * @retval None
 */
__STATIC_INLINE void LL_TC_SetDebugMode(TC_TypeDef *TCx, uint32_t DebugMode)
{
    MODIFY_REG(TCx->CTRL, TC_CTRL_DBGSTBDIS, DebugMode);
}

/**
 * @brief  Get the counter mode at debug state.
 * @rmtoll CTRL         DBGSTBDIS            LL_TC_GetDebugMode
 * @param  TCx TC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_TC_CTRL_DEBUGMODE_PAUSE
 *         @arg @ref LL_TC_CTRL_DEBUGMODE_COUNTINUE
 */
__STATIC_INLINE uint32_t LL_TC_GetDebugMode(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_BIT(TCx->CTRL, TC_CTRL_DBGSTBDIS));
}

/**
 * @}
 */

/** @defgroup TC_LL_EF_Counter_Clock_Selection Counter clock selection
 * @{
 */

/**
 * @brief  Set the clock source.
 * @rmtoll CCFG         CM            LL_TC_SetClockSource
 * @param  TCx TC Instance
 * @param  ClockSource This parameter can be one of the following values:
 *         @arg @ref LL_TC_CLOCKSOURCE_INTERNAL
 *         @arg @ref LL_TC_CLOCKSOURCE_EXTERNAL
 * @retval None
 */
__STATIC_INLINE void LL_TC_SetClockSource(TC_TypeDef *TCx, uint32_t ClockSource)
{
    MODIFY_REG(TCx->CCFG, TC_CCFG_CM, ClockSource);
}

/**
 * @brief  Get the clock source.
 * @rmtoll CCFG         CM            LL_TC_GetClockSource
 * @param  TCx TC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_TC_CLOCKSOURCE_INTERNAL
 *         @arg @ref LL_TC_CLOCKSOURCE_EXTERNAL
 */
__STATIC_INLINE uint32_t LL_TC_GetClockSource(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_BIT(TCx->CCFG, TC_CCFG_CM));
}

/**
 * @brief  Set the external clock mode.
 * @rmtoll CCFG         ECLKMODE      LL_TC_SetExtClockMode
 * @param  TCx TC Instance
 * @param  ExtClockMode This parameter can be one of the following values:
 *         @arg @ref LL_TC_EXTCLKMODE_POSEDGE
 *         @arg @ref LL_TC_EXTCLKMODE_NEGEDGE
 *         @arg @ref LL_TC_EXTCLKMODE_BOTHEDGE
 * @retval None
 */
__STATIC_INLINE void LL_TC_SetExtClockMode(TC_TypeDef *TCx, uint32_t ExtClockMode)
{
    MODIFY_REG(TCx->CCFG, TC_CCFG_ECLKMODE, ExtClockMode);
}

/**
 * @brief  Get the external clock mode.
 * @rmtoll CCFG         ECLKMODE      LL_TC_GetExtClockMode
 * @param  TCx TC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_TC_EXTCLKMODE_POSEDGE
 *         @arg @ref LL_TC_EXTCLKMODE_NEGEDGE
 *         @arg @ref LL_TC_EXTCLKMODE_BOTHEDGE
 */
__STATIC_INLINE uint32_t LL_TC_GetExtClockMode(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_BIT(TCx->CCFG, TC_CCFG_ECLKMODE));
}

/**
 * @brief  Set the external clock filter.
 * @rmtoll CCFG         FLTEN,FLTOPT  LL_TC_SetExtClockFilter
 * @param  TCx TC Instance
 * @param  FilterValue between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_TC_SetExtClockFilter(TC_TypeDef *TCx, uint32_t FilterValue)
{
    MODIFY_REG( TCx->CCFG, (TC_CCFG_FLTEN|TC_CCFG_FLTOPT),\
               ((FilterValue > 0) ? (uint32_t)(TC_CCFG_FLTEN|(FilterValue <<TC_CCFG_FLTOPT_POS)) : 0x0U));
}

/**
 * @brief  Get the external clock filter.
 * @rmtoll CCFG         FLTEN,FLTOPT  LL_TC_GetExtClockFilter
 * @param  TCx TC Instance
 * @retval clock filter value
 */
__STATIC_INLINE uint32_t LL_TC_GetExtClockFilter(TC_TypeDef *TCx)
{
    return ((uint32_t)(READ_BIT(TCx->CCFG, TC_CCFG_FLTEN))>0 )? (uint32_t)(READ_BIT(TCx->CCFG, TC_CCFG_FLTOPT)>>TC_CCFG_FLTOPT_POS)  : 0x0U;
}

/**
 * @}
 */

/** @defgroup TC_LL_EF_Timer_Synchronisation Timer synchronisation configuration
 * @{
 */

/**
 * @brief  Set the slave mode.
 * @rmtoll CTRL         SLVEN,SLVTRGMODE,SLVGATELVL LL_TC_SetSlaveMode
 * @param  TCx TC Instance
 * @param  SlaveMode This parameter can be one of the following values:
 *         @arg @ref LL_TC_SLAVEMODE_DISABLED
 *         @arg @ref LL_TC_SLAVEMODE_EDGE_PE
 *         @arg @ref LL_TC_SLAVEMODE_EDGE_NE
 *         @arg @ref LL_TC_SLAVEMODE_EDGE_BE
 *         @arg @ref LL_TC_SLAVEMODE_LVL_HIGH
 *         @arg @ref LL_TC_SLAVEMODE_LVL_LOW
 * @retval None
 */
__STATIC_INLINE void LL_TC_SetSlaveMode(TC_TypeDef *TCx, uint32_t SlaveMode)
{
    MODIFY_REG(TCx->CTRL, TC_CTRL_SLVEN | TC_CTRL_SLVTRGMODE | TC_CTRL_SLVGATELVL, SlaveMode);
}

/**
 * @brief  Get the slave mode.
 * @rmtoll CTRL         SLVEN,SLVTRGMODE,SLVGATELVL LL_TC_GetSlaveMode
 * @param  TCx TC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_TC_SLAVEMODE_DISABLED
 *         @arg @ref LL_TC_SLAVEMODE_EDGE_PE
 *         @arg @ref LL_TC_SLAVEMODE_EDGE_NE
 *         @arg @ref LL_TC_SLAVEMODE_EDGE_BE
 *         @arg @ref LL_TC_SLAVEMODE_LVL_HIGH
 *         @arg @ref LL_TC_SLAVEMODE_LVL_LOW
 */
__STATIC_INLINE uint32_t LL_TC_GetSlaveMode(TC_TypeDef *TCx)
{
    return ((uint32_t)(READ_BIT(TCx->CTRL, TC_CTRL_SLVEN))>0) ? \
                      (uint32_t)(READ_BIT(TCx->CTRL, TC_CTRL_SLVEN | TC_CTRL_SLVTRGMODE | TC_CTRL_SLVGATELVL)): 0x0U;
}

/**
 * @brief  Set the slave filter.
 * @rmtoll CTRL         SLVFLTEN,SLVFLTOPT     LL_TC_SetSlaveFilter
 * @param  TCx TC Instance
 * @param  FilterValue Filter value (between Min_Data=0x00 and Max_Data=0xFF)
 * @retval None
 */
__STATIC_INLINE void LL_TC_SetSlaveFilter(TC_TypeDef *TCx, uint32_t FilterValue)
{
    MODIFY_REG( TCx->CTRL, TC_CTRL_SLVFLTEN|TC_CTRL_SLVFLTOPT,\
               ((FilterValue > 0) ? (uint32_t)(TC_CTRL_SLVFLTEN|(FilterValue <<TC_CTRL_SLVFLTOPT_POS)) : 0x0U));
}

/**
 * @brief  Get the slave filter.
 * @rmtoll CTRL         SLVFLTEN,SLVFLTOPT     LL_TC_GetSlaveFilter
 * @param  TCx TC Instance
 * @retval Filter value (between Min_Data=0x00 and Max_Data=0xFF)
 */
__STATIC_INLINE uint32_t LL_TC_GetSlaveFilter(TC_TypeDef *TCx)
{
    return ((uint32_t)(READ_BIT(TCx->CTRL, TC_CTRL_SLVFLTEN))>0 )? (uint32_t)(READ_BIT(TCx->CTRL, TC_CTRL_SLVFLTOPT)>>TC_CTRL_SLVFLTOPT_POS)  : 0x0U;
}

/**
 * @}
 */

/** @defgroup TC_LL_EF_Capture_Compare Capture Compare configuration
 * @{
 */

/**
 * @brief  Enable one of capture/compare channel.
 * @rmtoll CM0          ENABLE        LL_TC_CC_EnableChannel
 *         CM1          ENABLE        LL_TC_CC_EnableChannel
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @retval None
 */
__STATIC_INLINE void LL_TC_CC_EnableChannel(TC_TypeDef *TCx, uint32_t Channel)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    SET_BIT(*pReg, TC_CM0_ENABLE);
}

/**
 * @brief  Disable one of capture/compare channel.
 * @rmtoll CM0          ENABLE        LL_TC_CC_DisableChannel
 *         CM1          ENABLE        LL_TC_CC_DisableChannel
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @retval None
 */
__STATIC_INLINE void LL_TC_CC_DisableChannel(TC_TypeDef *TCx, uint32_t Channel)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    CLEAR_BIT(*pReg, TC_CM0_ENABLE);
}

/**
 * @brief  Indicate whether channel(s) is(are) enabled.
 * @rmtoll CM0          ENABLE        LL_TC_CC_IsEnabledChannel
 *         CM1          ENABLE        LL_TC_CC_IsEnabledChannel
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_CC_IsEnabledChannel(TC_TypeDef *TCx, uint32_t Channel)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    return ((READ_BIT(*pReg, TC_CM0_ENABLE) == TC_CM0_ENABLE) ? 1UL : 0UL);
}

/**
 * @}
 */

/** @defgroup TC_LL_EF_Output_Channel Output channel configuration
 * @{
 */

/**
 * @brief  Configure output channel.
 * @rmtoll CM0          CCM,OM,EFELVL,DFTLVL LL_TC_OC_ConfigOutput
 *         CM1          CCM,OM,EFELVL,DFTLVL LL_TC_OC_ConfigOutput
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @param  Configuration This parameter must be a combination of all the following values:
 *         @arg @ref LL_TC_OCMODE_TRISTATE or ... or @ref LL_TC_OCMODE_PWM2
 *         @arg @ref LL_TC_OCPOLARITY_HIGH or @ref LL_TC_OCPOLARITY_LOW
 * @retval None
 */
__STATIC_INLINE void LL_TC_OC_ConfigOutput(TC_TypeDef *TCx, uint32_t Channel, uint32_t Configuration)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    MODIFY_REG(*pReg, TC_CM0_CCM | TC_CM0_OM | TC_CM0_EFELVL , Configuration);
}

/**
 * @brief  Set the output compare mode.
 * @rmtoll CM0          CCM,OM        LL_TC_OC_SetMode
 *         CM1          CCM,OM        LL_TC_OC_SetMode
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @param  Mode This parameter can be one of the following values:
 *         @arg @ref LL_TC_OCMODE_TRISTATE
 *         @arg @ref LL_TC_OCMODE_ACTIVE
 *         @arg @ref LL_TC_OCMODE_INACTIVE
 *         @arg @ref LL_TC_OCMODE_TOGGLE
 *         @arg @ref LL_TC_OCMODE_FORCED_ACTIVE
 *         @arg @ref LL_TC_OCMODE_FORCED_INACTIVE
 *         @arg @ref LL_TC_OCMODE_PWM1
 *         @arg @ref LL_TC_OCMODE_PWM2
 * @retval None
 */
__STATIC_INLINE void LL_TC_OC_SetMode(TC_TypeDef *TCx, uint32_t Channel, uint32_t Mode)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    MODIFY_REG(*pReg, TC_CM0_CCM | TC_CM0_OM, Mode);
}

/**
 * @brief  Get the output compare mode.
 * @rmtoll CM0          CCM,OM        LL_TC_OC_GetMode
 *         CM1          CCM,OM        LL_TC_OC_GetMode
 * @param  TCx TC instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_TC_OCMODE_TRISTATE
 *         @arg @ref LL_TC_OCMODE_ACTIVE
 *         @arg @ref LL_TC_OCMODE_INACTIVE
 *         @arg @ref LL_TC_OCMODE_TOGGLE
 *         @arg @ref LL_TC_OCMODE_FORCED_ACTIVE
 *         @arg @ref LL_TC_OCMODE_FORCED_INACTIVE
 *         @arg @ref LL_TC_OCMODE_PWM1
 *         @arg @ref LL_TC_OCMODE_PWM2
 */
__STATIC_INLINE uint32_t LL_TC_OC_GetMode(TC_TypeDef *TCx, uint32_t Channel)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    return (uint32_t)(READ_BIT(*pReg,  TC_CM0_CCM | TC_CM0_OM));
}

/**
 * @brief  Set the polarity of an output channel.
 * @rmtoll CM0          EFELVL        LL_TC_OC_SetPolarity
 *         CM1          EFELVL        LL_TC_OC_SetPolarity
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @param  Polarity This parameter can be one of the following values:
 *         @arg @ref LL_TC_OCPOLARITY_HIGH
 *         @arg @ref LL_TC_OCPOLARITY_LOW
 * @retval None
 */
__STATIC_INLINE void LL_TC_OC_SetPolarity(TC_TypeDef *TCx, uint32_t Channel, uint32_t Polarity)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    MODIFY_REG(*pReg,TC_CM0_EFELVL, Polarity);
}

/**
 * @brief  Get the polarity of an output channel.
 * @rmtoll CM0          EFELVL        LL_TC_OC_GetPolarity
 *         CM1          EFELVL        LL_TC_OC_GetPolarity
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_TC_OCPOLARITY_HIGH
 *         @arg @ref LL_TC_OCPOLARITY_LOW
 */
__STATIC_INLINE uint32_t LL_TC_OC_GetPolarity(TC_TypeDef *TCx, uint32_t Channel)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    return (uint32_t)(READ_BIT(*pReg, TC_CM0_EFELVL));
}

/**
 * @brief  Set the IDLE state of an output channel (output level when compare/capture function disable).
 * @rmtoll CM0          DFTLVL        LL_TC_OC_SetIdleState
 *         CM1          DFTLVL        LL_TC_OC_SetIdleState
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @param  IdleState This parameter can be one of the following values:
 *         @arg @ref LL_TC_OCIDLESTATE_LOW
 *         @arg @ref LL_TC_OCIDLESTATE_HIGH
 * @retval None
 */
__STATIC_INLINE void LL_TC_OC_SetIdleState(TC_TypeDef *TCx, uint32_t Channel, uint32_t IdleState)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    MODIFY_REG(*pReg, TC_CM0_DFTLVL, IdleState);
}

/**
 * @brief  Get the IDLE state of an output channel.
 * @rmtoll CM0          DFTLVL        LL_TC_OC_GetIdleState
 *         CM1          DFTLVL        LL_TC_OC_GetIdleState
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_TC_OCIDLESTATE_LOW
 *         @arg @ref LL_TC_OCIDLESTATE_HIGH
 */
__STATIC_INLINE uint32_t LL_TC_OC_GetIdleState(TC_TypeDef *TCx, uint32_t Channel)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    return (uint32_t)(READ_BIT(*pReg, TC_CM0_DFTLVL));
}

/**
 * @brief  Set the dead time delay.
 * @rmtoll CM0         DIEN,DL            LL_TC_OC_SetDeadTime
 *         CM1         DIEN,DL            LL_TC_OC_SetDeadTime
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @param  DeadTime Dead time delay (between Min_Data=0x00 and Max_Data=0x3F)
 * @retval None
 */
__STATIC_INLINE void LL_TC_OC_SetDeadTime(TC_TypeDef *TCx, uint32_t Channel, uint32_t DeadTime)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    MODIFY_REG(*pReg, TC_CM0_DIEN | TC_CM0_DL,\
               ((DeadTime > 0) ? (uint32_t)(TC_CM0_DIEN|(DeadTime << TC_CM0_DL_POS)) : 0x0U));
}

/**
 * @brief  Get the dead time delay.
 * @rmtoll CM0         DIEN,DL            LL_TC_OC_GetDeadTime
 *         CM1         DIEN,DL            LL_TC_OC_GetDeadTime
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @retval Dead time delay (between Min_Data=0x00 and Max_Data=0x3F)
 */
__STATIC_INLINE uint32_t LL_TC_OC_GetDeadTime(TC_TypeDef *TCx, uint32_t Channel)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    return ((uint32_t)(READ_BIT(*pReg, TC_CM0_DIEN)) ? (uint32_t)(READ_BIT(*pReg, TC_CM0_DL) >> TC_CM0_DL_POS) : 0U);
}

/**
 * @brief  Set the compare value for output channel 0.
 * @rmtoll CCD0         CCD0          LL_TC_OC_SetCompareCH0
 * @param  TCx TC Instance
 * @param  CompareValue Compare value (between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF)
 * @retval None
 */
__STATIC_INLINE void LL_TC_OC_SetCompareCH0(TC_TypeDef *TCx, uint32_t CompareValue)
{
    WRITE_REG(TCx->CCD0, CompareValue);
}

/**
 * @brief  Get the compare value for output channel 0.
 * @rmtoll CCD0         CCD0          LL_TC_OC_GetCompareCH0
 * @param  TCx TC Instance
 * @retval Compare value (between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF)
 */
__STATIC_INLINE uint32_t LL_TC_OC_GetCompareCH0(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_REG(TCx->CCD0));
}

/**
 * @brief  Set the compare value for output channel 1.
 * @rmtoll CCD1         CCD1          LL_TC_OC_SetCompareCH1
 * @param  TCx TC Instance
 * @param  CompareValue Compare value (between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF)
 * @retval None
 */
__STATIC_INLINE void LL_TC_OC_SetCompareCH1(TC_TypeDef *TCx, uint32_t CompareValue)
{
    WRITE_REG(TCx->CCD1, CompareValue);
}

/**
 * @brief  Get the compare value for output channel 1.
 * @rmtoll CCD1         CCD1          LL_TC_OC_GetCompareCH1
 * @param  TCx TC Instance
 * @retval Compare value (between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF)
 */
__STATIC_INLINE uint32_t LL_TC_OC_GetCompareCH1(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_REG(TCx->CCD1));
}

/**
 * @}
 */

/** @defgroup TC_LL_EF_Input_Channel Input channel configuration
 * @{
 */

/**
 * @brief Configure input channel.
 * @rmtoll CM0          CCM,CPOL,FLTEN LL_TC_IC_Config
 *         CM1          CCM,CPOL,FLTEN LL_TC_IC_Config
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @param  Configuration This parameter can be one of the following values:
 *         @arg @ref LL_TC_IC_POLARITY_RISING
 *         @arg @ref LL_TC_IC_POLARITY_FALLING
 *         @arg @ref LL_TC_IC_POLARITY_BOTHEDGE
 * @param  FilterValue between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_TC_IC_Config(TC_TypeDef *TCx, uint32_t Channel, uint32_t Configuration,uint32_t FilterValue)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    MODIFY_REG( *pReg,  (TC_CM0_CCM | TC_CM0_CPOL| TC_CM0_FLTEN | TC_CM0_FLTOPT),\
               ((FilterValue > 0) ? (uint32_t)(Configuration|TC_CM0_FLTEN|(FilterValue << TC_CM0_FLTOPT_POS)) : Configuration));
}

/**
 * @brief  Set the polarity of the input channel.
 * @rmtoll CM0          CPOL,CCM          LL_TC_IC_SetPolarity
 *         CM1          CPOL,CCM          LL_TC_IC_SetPolarity
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @param  ICPolarity This parameter can be one of the following values:
 *         @arg @ref LL_TC_IC_POLARITY_RISING
 *         @arg @ref LL_TC_IC_POLARITY_FALLING
 *         @arg @ref LL_TC_IC_POLARITY_BOTHEDGE
 * @retval None
 */
__STATIC_INLINE void LL_TC_IC_SetPolarity(TC_TypeDef *TCx, uint32_t Channel, uint32_t ICPolarity)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    MODIFY_REG( *pReg, (TC_CM0_CCM|TC_CM0_CPOL), ICPolarity);
}

/**
 * @brief  Get the polarity of the input channel.
 * @rmtoll CM0          CPOL,CCM          LL_TC_IC_GetPolarity
 *         CM1          CPOL,CCM          LL_TC_IC_GetPolarity
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_TC_IC_POLARITY_RISING
 *         @arg @ref LL_TC_IC_POLARITY_FALLING
 */
__STATIC_INLINE uint32_t LL_TC_IC_GetPolarity(TC_TypeDef *TCx, uint32_t Channel)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    return (uint32_t)(READ_BIT(*pReg, TC_CM0_CPOL));
}

/**
 * @brief  Set the input capture filter.
 * @rmtoll CM0          FLTEN,FLTOPT  LL_TC_IC_SetFilter
 *         CM1          FLTEN,FLTOPT  LL_TC_IC_SetFilter
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @param  FilterValue between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_TC_IC_SetFilter(TC_TypeDef *TCx, uint32_t Channel, uint32_t FilterValue)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    MODIFY_REG( *pReg,  TC_CM0_FLTEN | TC_CM0_FLTOPT,\
               ((FilterValue > 0) ? (uint32_t)(TC_CM0_FLTEN|(FilterValue << TC_CM0_FLTOPT_POS)) : 0x0U));
}

/**
 * @brief  Get the input capture filter.
 * @rmtoll CM0          FLTEN,FLTOPT  LL_TC_IC_GetFilter
 *         CM1          FLTEN,FLTOPT  LL_TC_IC_GetFilter
 * @param  TCx TC Instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_TC_CHANNEL_CH0
 *         @arg @ref LL_TC_CHANNEL_CH1
 * @retval Filter value (between Min_Data=0x00 and Max_Data=0xFF)
 */
__STATIC_INLINE uint32_t LL_TC_IC_GetFilter(TC_TypeDef *TCx, uint32_t Channel)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&TCx->CM0) + OFFSET_TAB_CC_CHx_REG[Channel]));
    return ((uint32_t)(READ_BIT(*pReg, *pReg))>0 )? (uint32_t)(READ_BIT(*pReg, TC_CM0_FLTOPT)>>TC_CM0_FLTOPT_POS) : 0x0U;
}

/**
 * @brief  Get the capture value for input channel 0.
 * @rmtoll CCD0         CCD0          LL_TC_IC_GetCaptureCH0
 * @param  TCx TC Instance
 * @retval Capture value (between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF)
 */
__STATIC_INLINE uint32_t LL_TC_IC_GetCaptureCH0(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_REG(TCx->CCD0));
}

/**
 * @brief  Get the capture value for input channel 1.
 * @rmtoll CCD1         CCD1          LL_TC_IC_GetCaptureCH1
 * @param  TCx TC Instance
 * @retval Capture value (between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF)
 */
__STATIC_INLINE uint32_t LL_TC_IC_GetCaptureCH1(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_REG(TCx->CCD1));
}

/**
 * @}
 */

/** @defgroup TC_LL_EF_FLAG_Management FLAG Management
 * @{
 */

/**
 * @brief  Indicate whether overflow interrupt flag (UIF) is set (overflow interrupt is pending).
 * @rmtoll STA          OVF           LL_TC_IsActiveFlag_OV
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_IsActiveFlag_OV(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->STA, TC_STA_OVF) == (TC_STA_OVF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the overflow interrupt flag (UIF).
 * @rmtoll STA          OVF           LL_TC_ClearFlag_OV
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_ClearFlag_OV(TC_TypeDef *TCx)
{
    WRITE_REG(TCx->STA, (TC_STA_OVF));
}

/**
 * @brief  Indicate whether Capture/Compare 0 interrupt flag (CC0F) is set (Capture/Compare 0 interrupt is pending).
 * @rmtoll STA          CC0F          LL_TC_IsActiveFlag_CC0
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_IsActiveFlag_CC0(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->STA, TC_STA_CC0F) == (TC_STA_CC0F)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the Capture/Compare 0 interrupt flag (CC0F).
 * @rmtoll STA          CC0F          LL_TC_ClearFlag_CC0
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_ClearFlag_CC0(TC_TypeDef *TCx)
{
    WRITE_REG(TCx->STA, (TC_STA_CC0F));
}

/**
 * @brief  Indicate whether Capture/Compare 1 interrupt flag (CC1F) is set (Capture/Compare 1 interrupt is pending).
 * @rmtoll STA          CC1F          LL_TC_IsActiveFlag_CC1
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_IsActiveFlag_CC1(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->STA, TC_STA_CC1F) == (TC_STA_CC1F)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the Capture/Compare 1 interrupt flag (CC1F).
 * @rmtoll STA          CC1F          LL_TC_ClearFlag_CC1
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_ClearFlag_CC1(TC_TypeDef *TCx)
{
    WRITE_REG(TCx->STA, (TC_STA_CC1F));
}

/**
 * @brief  Indicate whether slave mode interrupt flag (SLVF) is set (slave mode interrupt is pending).
 * @rmtoll STA          SLVF          LL_TC_IsActiveFlag_SLVF
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_IsActiveFlag_SLVF(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->STA, TC_STA_SLVF) == (TC_STA_SLVF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the slave mode interrupt flag (SLVF).
 * @rmtoll STA          SLVF          LL_TC_ClearFlag_SLVF
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_ClearFlag_SLVF(TC_TypeDef *TCx)
{
    WRITE_REG(TCx->STA, (TC_STA_SLVF));
}

/**
 * @}
 */

/** @defgroup TC_LL_EF_IT_Management IT Management
 * @{
 */

/**
 * @brief  Enable overflow interrupt (UIE).
 * @rmtoll IE           OVIE          LL_TC_EnableIT_OV
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_EnableIT_OV(TC_TypeDef *TCx)
{
    SET_BIT(TCx->IE, TC_IE_OVIE);
}

/**
 * @brief  Disable overflow interrupt (UIE).
 * @rmtoll IE           OVIE          LL_TC_DisableIT_OV
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_DisableIT_OV(TC_TypeDef *TCx)
{
    CLEAR_BIT(TCx->IE, TC_IE_OVIE);
}

/**
 * @brief  Indicates whether the overflow interrupt (UIE) is enabled.
 * @rmtoll IE           OVIE          LL_TC_IsEnabledIT_OV
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_IsEnabledIT_OV(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->IE, TC_IE_OVIE) == (TC_IE_OVIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable capture/compare 0 interrupt (CC0IE).
 * @rmtoll IE           CC0IE         LL_TC_EnableIT_CC0
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_EnableIT_CC0(TC_TypeDef *TCx)
{
    SET_BIT(TCx->IE, TC_IE_CC0IE);
}

/**
 * @brief  Disable capture/compare 0 interrupt (CC0IE).
 * @rmtoll IE           CC0IE         LL_TC_DisableIT_CC0
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_DisableIT_CC0(TC_TypeDef *TCx)
{
    CLEAR_BIT(TCx->IE, TC_IE_CC0IE);
}

/**
 * @brief  Indicates whether the capture/compare 0 interrupt (CC0IE) is enabled.
 * @rmtoll IE           CC0IE         LL_TC_IsEnabledIT_CC0
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_IsEnabledIT_CC0(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->IE, TC_IE_CC0IE) == (TC_IE_CC0IE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable capture/compare 1 interrupt (CC1IE).
 * @rmtoll IE           CC1IE         LL_TC_EnableIT_CC1
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_EnableIT_CC1(TC_TypeDef *TCx)
{
    SET_BIT(TCx->IE, TC_IE_CC1IE);
}

/**
 * @brief  Disable capture/compare 1 interrupt (CC1IE).
 * @rmtoll IE           CC1IE         LL_TC_DisableIT_CC1
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_DisableIT_CC1(TC_TypeDef *TCx)
{
    CLEAR_BIT(TCx->IE, TC_IE_CC1IE);
}

/**
 * @brief  Indicates whether the capture/compare 1 interrupt (CC1IE) is enabled.
 * @rmtoll IE           CC1IE         LL_TC_IsEnabledIT_CC1
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_IsEnabledIT_CC1(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->IE, TC_IE_CC1IE) == (TC_IE_CC1IE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable slave mode interrupt (SLVIE).
 * @rmtoll IE           SLVIE         LL_TC_EnableIT_SLVF
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_EnableIT_SLVF(TC_TypeDef *TCx)
{
    SET_BIT(TCx->IE, TC_IE_SLVIE);
}

/**
 * @brief  Disable slave mode interrupt (SLVIE).
 * @rmtoll IE           SLVIE         LL_TC_DisableIT_SLVF
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_DisableIT_SLVF(TC_TypeDef *TCx)
{
    CLEAR_BIT(TCx->IE, TC_IE_SLVIE);
}

/**
 * @brief  Indicates whether the slave mode interrupt (SLVIE) is enabled.
 * @rmtoll IE           SLVIE         LL_TC_IsEnabledIT_SLVF
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_IsEnabledIT_SLVF(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->IE, TC_IE_SLVIE) == (TC_IE_SLVIE)) ? 1UL : 0UL);
}

/**
 * @}
 */

/** @defgroup TC_LL_EF_PWMMOD_Configuration PWM Modulation control and Configuration
 * @{
 */

/**
 * @brief  Enable PWM modulation.
 * @rmtoll PWM_CTRL     ENABLE        LL_TC_PWMMOD_Enable
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_Enable(TC_TypeDef *TCx)
{
    SET_BIT(TCx->PWM_CTRL, TC_PWM_CTRL_EN);
}

/**
 * @brief  Disable PWM modulation.
 * @rmtoll PWM_CTRL     ENABLE        LL_TC_PWMMOD_Disable
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_Disable(TC_TypeDef *TCx)
{
    CLEAR_BIT(TCx->PWM_CTRL, TC_PWM_CTRL_EN);
}

/**
 * @brief  Indicates whether PWM modulation is enabled.
 * @rmtoll PWM_CTRL     ENABLE        LL_TC_PWMMOD_IsEnabled
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_IsEnabled(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->PWM_CTRL, TC_PWM_CTRL_EN) == (TC_PWM_CTRL_EN)) ? 1UL : 0UL);
}

/**
 * @brief  Set PWM modulation control mode.
 * @rmtoll PWM_CFG      CMS           LL_TC_PWMMOD_SetControlMode
 * @param  TCx TC Instance
 * @param  ControlMode This parameter can be one of the following values:
 *         @arg @ref LL_TC_PWMMOD_CONTROLMODE_SOFTWARE
 *         @arg @ref LL_TC_PWMMOD_CONTROLMODE_DMA
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_SetControlMode(TC_TypeDef *TCx, uint32_t ControlMode)
{
    MODIFY_REG(TCx->PWM_CFG, TC_PWM_CFG_CMS, ControlMode);
}

/**
 * @brief  Get PWM modulation control mode.
 * @rmtoll PWM_CFG      CMS           LL_TC_PWMMOD_GetControlMode
 * @param  TCx TC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_TC_PWMMOD_CONTROLMODE_SOFTWARE
 *         @arg @ref LL_TC_PWMMOD_CONTROLMODE_DMA
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_GetControlMode(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_BIT(TCx->PWM_CFG, TC_PWM_CFG_CMS));
}

/**
 * @brief  Set PWM modulation output duration mode.
 * @rmtoll PWM_CFG      ORV           LL_TC_PWMMOD_SetOutputDurationMode
 * @param  TCx TC Instance
 * @param  DurationMode This parameter can be one of the following values:
 *         @arg @ref LL_TC_PWMMOD_DURATIONMODE_CONTINUOUS
 *         @arg @ref LL_TC_PWMMOD_DURATIONMODE_LENGTH
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_SetOutputDurationMode(TC_TypeDef *TCx, uint32_t DurationMode)
{
    MODIFY_REG(TCx->PWM_CFG, TC_PWM_CFG_ORV, DurationMode);
}

/**
 * @brief  Get PWM modulation output duration mode.
 * @rmtoll PWM_CFG      ORV           LL_TC_PWMMOD_GetOutputDurationMode
 * @param  TCx TC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_TC_PWMMOD_DURATIONMODE_CONTINUOUS
 *         @arg @ref LL_TC_PWMMOD_DURATIONMODE_LENGTH
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_GetOutputDurationMode(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_BIT(TCx->PWM_CFG, TC_PWM_CFG_ORV));
}


/**
 * @brief  Set PWM wave length (output cycle number - 1).
 * @rmtoll PWM_CFG      WL            LL_TC_PWMMOD_SetWaveLength
 * @param  TCx TC Instance
 * @param  WaveLength Wave length value (between Min_Data=0x0= and Max_Data=0x3FF)
 * @note   This parameter is only valid in length mode (LL_TC_PWMMOD_DURATIONMODE_LENGTH)
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_SetWaveLength(TC_TypeDef *TCx, uint32_t WaveLength)
{
    MODIFY_REG(TCx->PWM_CFG, TC_PWM_CFG_WL, WaveLength << TC_PWM_CFG_WL_POS);
}

/**
 * @brief  Get PWM wave length.
 * @rmtoll PWM_CFG      WL            LL_TC_PWMMOD_GetWaveLength
 * @param  TCx TC Instance
 * @retval Wave length value (between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF)
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_GetWaveLength(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_BIT(TCx->PWM_CFG, TC_PWM_CFG_WL) >> TC_PWM_CFG_WL_POS);
}

/**
 * @brief  Set PWM modulation stop mode mode.
 * @rmtoll PWM_CFG      CSS           LL_TC_PWMMOD_SetStopMode
 * @param  TCx TC Instance
 * @param  StopMode This parameter can be one of the following values:
 *         @arg @ref LL_TC_PWMMOD_STOPMODE_COMPLETE
 *         @arg @ref LL_TC_PWMMOD_STOPMODE_BREAK
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_SetStopMode(TC_TypeDef *TCx, uint32_t StopMode)
{
    MODIFY_REG(TCx->PWM_CFG, TC_PWM_CFG_CSS, StopMode);
}

/**
 * @brief  Get PWM modulation stop mode mode.
 * @rmtoll PWM_CFG      CSS           LL_TC_PWMMOD_GetStopMode
 * @param  TCx TC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_TC_PWMMOD_STOPMODE_COMPLETE
 *         @arg @ref LL_TC_PWMMOD_STOPMODE_BREAK
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_GetStopMode(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_BIT(TCx->PWM_CFG, TC_PWM_CFG_CSS));
}

/**
 * @brief  Set PWM pulse period length.
 * @rmtoll PWM_CNT      N_PWM         LL_TC_PWMMOD_SetPeriod
 * @param  TCx TC Instance
 * @param  PulsePeriod Period length value (between Min_Data=0x00000000 and Max_Data=0x0000FFFF)
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_SetPeriod(TC_TypeDef *TCx, uint32_t PulsePeriod)
{
    WRITE_REG(TCx->PWM_CNT, (TC_PWM_CNT_N_PWM & PulsePeriod));
}

/**
 * @brief  Get PWM pulse period length.
 * @rmtoll PWM_CNT      N_PWM         LL_TC_PWMMOD_GetPeriod
 * @param  TCx TC Instance
 * @retval Period length value (between Min_Data=0x00000000 and Max_Data=0x0000FFFF)
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_GetPeriod(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_BIT(TCx->PWM_CNT, TC_PWM_CNT_N_PWM));
}

/**
 * @}
 */

/** @defgroup TC_LL_EF_PWMMOD_DMA PWM Modulation DMA Management
 * @{
 */

/**
 * @brief  Set DMA base address.
 * @rmtoll PWM_DMA_BADR BADR          LL_TC_PWMMOD_SetDMABaseAddress
 * @param  TCx TC Instance
 * @param  BaseAddress DMA base address (between Min_Data=0x0000 and Max_Data=0xFFFF)
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_SetDMABaseAddress(TC_TypeDef *TCx, uint32_t BaseAddress)
{
    WRITE_REG(TCx->PWM_DMA_BADR, (TC_PWM_DMA_BADR_BADR & BaseAddress));
}

/**
 * @brief  Get DMA base address.
 * @rmtoll PWM_DMA_BADR BADR          LL_TC_PWMMOD_GetDMABaseAddress
 * @param  TCx TC Instance
 * @retval DMA base address (between Min_Data=0x0000 and Max_Data=0xFFFF)
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_GetDMABaseAddress(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_BIT(TCx->PWM_DMA_BADR, TC_PWM_DMA_BADR_BADR));
}

/**
 * @brief  Set DMA transfer length.
 * @rmtoll PWM_DMA_LEN  LEN           LL_TC_PWMMOD_SetDMALength
 * @param  TCx TC Instance
 * @param  Length DMA transfer length (between Min_Data=0x0000 and Max_Data=0xFFFF)
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_SetDMALength(TC_TypeDef *TCx, uint32_t Length)
{
    WRITE_REG(TCx->PWM_DMA_LEN, (TC_PWM_DMA_LEN_LEN & Length));
}

/**
 * @brief  Get DMA transfer length.
 * @rmtoll PWM_DMA_LEN  LEN           LL_TC_PWMMOD_GetDMALength
 * @param  TCx TC Instance
 * @retval DMA transfer length (between Min_Data=0x0000 and Max_Data=0xFFFF)
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_GetDMALength(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_BIT(TCx->PWM_DMA_LEN, TC_PWM_DMA_LEN_LEN));
}

/**
 * @brief  Get current DMA address.
 * @rmtoll PWM_DMA_ADR  ADR           LL_TC_PWMMOD_GetDMACurrentAddress
 * @param  TCx TC Instance
 * @retval Current DMA address (between Min_Data=0x0000 and Max_Data=0xFFFF)
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_GetDMACurrentAddress(TC_TypeDef *TCx)
{
    return (uint32_t)(READ_BIT(TCx->PWM_DMA_ADR, TC_PWM_DMA_ADR_ADR));
}

/**
 * @}
 */

/** @defgroup TC_LL_EF_PWMMOD_FLAG_Management PWM Modulation FLAG Management
 * @{
 */

/**
 * @brief  Indicate whether PWM output is busy.
 * @rmtoll PWM_STA      BUSY          LL_TC_PWMMOD_IsActiveFlag_BUSY
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_IsActiveFlag_BUSY(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->PWM_STA, TC_PWM_STA_BUSY) == (TC_PWM_STA_BUSY)) ? 1UL : 0UL);
}

/**
 * @brief  Indicate whether PWM output done interrupt flag is set.
 * @rmtoll PWM_STA      DONE          LL_TC_PWMMOD_IsActiveFlag_DONE
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_IsActiveFlag_DONE(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->PWM_STA, TC_PWM_STA_DONE) == (TC_PWM_STA_DONE)) ? 1UL : 0UL);
}

/**
 * @brief  Clear PWM output done interrupt flag.
 * @rmtoll PWM_STA      DONE          LL_TC_PWMMOD_ClearFlag_DONE
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_ClearFlag_DONE(TC_TypeDef *TCx)
{
    WRITE_REG(TCx->PWM_STA, TC_PWM_STA_DONE);
}

/**
 * @brief  Indicate whether PWM data error interrupt flag is set.
 * @rmtoll PWM_STA      DATERR        LL_TC_PWMMOD_IsActiveFlag_DATAERR
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_IsActiveFlag_DATAERR(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->PWM_STA, TC_PWM_STA_DATERR) == (TC_PWM_STA_DATERR)) ? 1UL : 0UL);
}

/**
 * @brief  Clear PWM data error interrupt flag.
 * @rmtoll PWM_STA      DATERR        LL_TC_PWMMOD_ClearFlag_DATAERR
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_ClearFlag_DATAERR(TC_TypeDef *TCx)
{
    WRITE_REG(TCx->PWM_STA, TC_PWM_STA_DATERR);
}

/**
 * @brief  Indicate whether PWM buffer empty interrupt flag is set.
 * @rmtoll PWM_STA      BUFVLD        LL_TC_PWMMOD_IsActiveFlag_BUFVLD
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_IsActiveFlag_BUFVLD(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->PWM_STA, TC_PWM_STA_BUFVLD) == (TC_PWM_STA_BUFVLD)) ? 1UL : 0UL);
}

/**
 * @brief  Clear PWM buffer empty interrupt flag.
 * @rmtoll PWM_STA      BUFVLD        LL_TC_PWMMOD_ClearFlag_BUFVLD
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_ClearFlag_BUFVLD(TC_TypeDef *TCx)
{
    WRITE_REG(TCx->PWM_STA, TC_PWM_STA_BUFVLD);
}

/**
 * @brief  Indicate whether PWM output cycle done interrupt flag is set.
 * @rmtoll PWM_STA      CYCDONE       LL_TC_PWMMOD_IsActiveFlag_CYCDONE
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_IsActiveFlag_CYCDONE(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->PWM_STA, TC_PWM_STA_CYCDONE) == (TC_PWM_STA_CYCDONE)) ? 1UL : 0UL);
}

/**
 * @brief  Clear PWM output cycle done interrupt flag.
 * @rmtoll PWM_STA      CYCDONE       LL_TC_PWMMOD_ClearFlag_CYCDONE
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_ClearFlag_CYCDONE(TC_TypeDef *TCx)
{
    WRITE_REG(TCx->PWM_STA, TC_PWM_STA_CYCDONE);
}

/**
 * @}
 */

/** @defgroup TC_LL_EF_PWMMOD_IT_Management PWM Modulation IT Management
 * @{
 */

/**
 * @brief  Enable PWM output done interrupt.
 * @rmtoll PWM_CFG      DONEIE        LL_TC_PWMMOD_EnableIT_DONE
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_EnableIT_DONE(TC_TypeDef *TCx)
{
    SET_BIT(TCx->PWM_CFG, TC_PWM_CFG_DONEIE);
}

/**
 * @brief  Disable PWM output done interrupt.
 * @rmtoll PWM_CFG      DONEIE        LL_TC_PWMMOD_DisableIT_DONE
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_DisableIT_DONE(TC_TypeDef *TCx)
{
    CLEAR_BIT(TCx->PWM_CFG, TC_PWM_CFG_DONEIE);
}

/**
 * @brief  Indicates whether PWM output done interrupt is enabled.
 * @rmtoll PWM_CFG      DONEIE        LL_TC_PWMMOD_IsEnabledIT_DONE
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_IsEnabledIT_DONE(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->PWM_CFG, TC_PWM_CFG_DONEIE) == (TC_PWM_CFG_DONEIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable PWM data error interrupt.
 * @rmtoll PWM_CFG      DATERRIE      LL_TC_PWMMOD_EnableIT_DATAERR
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_EnableIT_DATAERR(TC_TypeDef *TCx)
{
    SET_BIT(TCx->PWM_CFG, TC_PWM_CFG_DATERRIE);
}

/**
 * @brief  Disable PWM data error interrupt.
 * @rmtoll PWM_CFG      DATERRIE      LL_TC_PWMMOD_DisableIT_DATAERR
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_DisableIT_DATAERR(TC_TypeDef *TCx)
{
    CLEAR_BIT(TCx->PWM_CFG, TC_PWM_CFG_DATERRIE);
}

/**
 * @brief  Indicates whether PWM data error interrupt is enabled.
 * @rmtoll PWM_CFG      DATERRIE      LL_TC_PWMMOD_IsEnabledIT_DATAERR
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_IsEnabledIT_DATAERR(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->PWM_CFG, TC_PWM_CFG_DATERRIE) == (TC_PWM_CFG_DATERRIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable PWM buffer empty interrupt.
 * @rmtoll PWM_CFG      BUFVLDIE      LL_TC_PWMMOD_EnableIT_BUFVLD
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_EnableIT_BUFVLD(TC_TypeDef *TCx)
{
    SET_BIT(TCx->PWM_CFG, TC_PWM_CFG_BUFVLDIE);
}

/**
 * @brief  Disable PWM buffer empty interrupt.
 * @rmtoll PWM_CFG      BUFVLDIE      LL_TC_PWMMOD_DisableIT_BUFVLD
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_DisableIT_BUFVLD(TC_TypeDef *TCx)
{
    CLEAR_BIT(TCx->PWM_CFG, TC_PWM_CFG_BUFVLDIE);
}

/**
 * @brief  Indicates whether PWM buffer empty interrupt is enabled.
 * @rmtoll PWM_CFG      BUFVLDIE      LL_TC_PWMMOD_IsEnabledIT_BUFVLD
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_IsEnabledIT_BUFVLD(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->PWM_CFG, TC_PWM_CFG_BUFVLDIE) == (TC_PWM_CFG_BUFVLDIE)) ? 1UL : 0UL);
}

/**
 * @brief  Enable PWM output cycle done interrupt.
 * @rmtoll PWM_CFG      CYCDONEIE     LL_TC_PWMMOD_EnableIT_CYCDONE
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_EnableIT_CYCDONE(TC_TypeDef *TCx)
{
    SET_BIT(TCx->PWM_CFG, TC_PWM_CFG_CYCDONEIE);
}

/**
 * @brief  Disable PWM output cycle done interrupt.
 * @rmtoll PWM_CFG      CYCDONEIE     LL_TC_PWMMOD_DisableIT_CYCDONE
 * @param  TCx TC Instance
 * @retval None
 */
__STATIC_INLINE void LL_TC_PWMMOD_DisableIT_CYCDONE(TC_TypeDef *TCx)
{
    CLEAR_BIT(TCx->PWM_CFG, TC_PWM_CFG_CYCDONEIE);
}

/**
 * @brief  Indicates whether PWM output cycle done interrupt is enabled.
 * @rmtoll PWM_CFG      CYCDONEIE     LL_TC_PWMMOD_IsEnabledIT_CYCDONE
 * @param  TCx TC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_TC_PWMMOD_IsEnabledIT_CYCDONE(TC_TypeDef *TCx)
{
    return ((READ_BIT(TCx->PWM_CFG, TC_PWM_CFG_CYCDONEIE) == (TC_PWM_CFG_CYCDONEIE)) ? 1UL : 0UL);
}
/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup TC_LL_EF_Init Initialisation and deinitialisation functions
 * @{
 */
ErrorStatus LL_TC_DeInit(TC_TypeDef *TCx);
void LL_TC_StructInit(LL_TC_InitTypeDef *TC_InitStruct);
ErrorStatus LL_TC_Init(TC_TypeDef *TCx, LL_TC_InitTypeDef *TC_InitStruct);
void LL_TC_OC_StructInit(LL_TC_OC_InitTypeDef *TC_OC_InitStruct);
ErrorStatus LL_TC_OC_Init(TC_TypeDef *TCx, uint32_t Channel, LL_TC_OC_InitTypeDef *TC_OC_InitStruct);
void LL_TC_IC_StructInit(LL_TC_IC_InitTypeDef *TC_IC_InitStruct);
ErrorStatus LL_TC_IC_Init(TC_TypeDef *TCx, uint32_t Channel, LL_TC_IC_InitTypeDef *TC_IC_InitStruct);
void LL_TC_PWMMOD_StructInit(LL_TC_PWMMOD_InitTypeDef *TC_PWMMOD_InitStruct);
ErrorStatus LL_TC_PWMMOD_Init(TC_TypeDef *TCx, LL_TC_PWMMOD_InitTypeDef *TC_PWMMOD_InitStruct);
/**
 * @}
 */
#endif /* USE_FULL_LL_DRIVER */

/**
 * @}
 */

/**
 * @}
 */

#endif /* TC0_CORE0 || TC1_CORE0 || TC0_CORE1 || TC1_CORE1 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_TC_H */
