/**
 ******************************************************************************
 * @file   rn_rv_ll_rtc.h
 * @author Renergy Software Team
 * @brief  Header file of RTC LL module driver.
 *         Realtime clock
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
#ifndef __RN_RV_LL_RTC_H
#define __RN_RV_LL_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu.h"
#include "rn831x_rn861x_mcu_rtc.h"
/** @addtogroup RN_LL_Driver
 * @{
 */

/** @defgroup RTC_LL RTC
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/** @defgroup RTC_LL_Private_Constants RTC Private Constants
 * @{
 */

/* Defines used to combine date & time **/
#define RTC_OFFSET_WEEKDAY            24U
#define RTC_OFFSET_DAY                16U
#define RTC_OFFSET_MONTH              8U
#define RTC_OFFSET_HOUR               16U
#define RTC_OFFSET_MINUTE             8U


/* Defines to select competation curve **/
#define RTC_MODE1_CURVE_SEL_QUADRATIC            0U          /* 2nd curve */
#define RTC_MODE1_CURVE_SEL_QUARTIC              0xEA65U     /* 4th curve */
/**
 * @}
 */ /* end of group RTC_LL_Private_Constants **/

/* Private macros -----------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
/** @defgroup RTC_LL_ES_INIT RTC Exported Init structure
 * @{
 */
/**
 * @brief  RTC Time structure definition
 */
typedef struct
{
    uint8_t Hours;       /*!< Specifies the RTC Time Hours.
                              This parameter must be a number between Min_Data = 0 and Max_Data = 23
                              This feature can be modified afterwards using unitary function @ref LL_RTC_TIME_SetHour(). */

    uint8_t Minutes;     /*!< Specifies the RTC Time Minutes.
                              This parameter must be a number between Min_Data = 0 and Max_Data = 59

                              This feature can be modified afterwards using unitary function @ref LL_RTC_TIME_SetMinute(). */

    uint8_t Seconds;     /*!< Specifies the RTC Time Seconds.
                              This parameter must be a number between Min_Data = 0 and Max_Data = 59

                              This feature can be modified afterwards using unitary function @ref LL_RTC_TIME_SetSecond(). */
} LL_RTC_TimeTypeDef;

/**
 * @brief  RTC Date structure definition
 */
typedef struct
{
    uint8_t WeekDay;  /*!< Specifies the RTC Date WeekDay.
                           This parameter can be a value of @ref RTC_LL_EC_WEEKDAY

                           This feature can be modified afterwards using unitary function @ref LL_RTC_DATE_SetWeekDay(). */

    uint8_t Month;    /*!< Specifies the RTC Date Month.
                           This parameter can be a value of @ref RTC_LL_EC_MONTH

                           This feature can be modified afterwards using unitary function @ref LL_RTC_DATE_SetMonth(). */

    uint8_t Day;      /*!< Specifies the RTC Date Day.
                           This parameter must be a number between Min_Data = 1 and Max_Data = 31

                           This feature can be modified afterwards using unitary function @ref LL_RTC_DATE_SetDay(). */

    uint8_t Year;     /*!< Specifies the RTC Date Year.
                           This parameter must be a number between Min_Data = 0 and Max_Data = 99

                           This feature can be modified afterwards using unitary function @ref LL_RTC_DATE_SetYear(). */
} LL_RTC_DateTypeDef;

/**
 * @brief  RTC Alarm structure definition
 */
typedef struct
{
    uint8_t Hours;       /*!< Specifies the RTC ALARM Hours.
                              This parameter must be a number between Min_Data = 0 and Max_Data = 23
                              This feature can be modified afterwards using unitary function @ref LL_RTC_ALARM_SetHour(). */

    uint8_t Minutes;     /*!< Specifies the RTC ALARM Minutes.
                              This parameter must be a number between Min_Data = 0 and Max_Data = 59

                              This feature can be modified afterwards using unitary function @ref LL_RTC_ALARM_SetMinute(). */

    uint8_t Seconds;     /*!< Specifies the RTC ALARM Seconds.
                              This parameter must be a number between Min_Data = 0 and Max_Data = 59

                              This feature can be modified afterwards using unitary function @ref LL_RTC_ALARM_SetSecond(). */
} LL_RTC_AlarmTypeDef;
/**
 * @}
 */ /* end of group RTC_LL_ES_INIT **/

/* Exported constants -------------------------------------------------------*/
/** @defgroup RTC_LL_Exported_Constants RTC Exported Constants
 * @{
 */

/** @defgroup RTC_LL_EC_WEEKDAY  WEEK DAY
 * @{
 */
#define LL_RTC_WEEKDAY_MONDAY              ((uint8_t)0x00U) /*!< Monday    **/
#define LL_RTC_WEEKDAY_TUESDAY             ((uint8_t)0x01U) /*!< Tuesday   **/
#define LL_RTC_WEEKDAY_WEDNESDAY           ((uint8_t)0x02U) /*!< Wednesday **/
#define LL_RTC_WEEKDAY_THURSDAY            ((uint8_t)0x03U) /*!< Thrusday  **/
#define LL_RTC_WEEKDAY_FRIDAY              ((uint8_t)0x04U) /*!< Friday    **/
#define LL_RTC_WEEKDAY_SATURDAY            ((uint8_t)0x05U) /*!< Saturday  **/
#define LL_RTC_WEEKDAY_SUNDAY              ((uint8_t)0x06U) /*!< Sunday    **/
/**
 * @}
 */

/** @defgroup RTC_LL_EC_MONTH  MONTH
 * @{
 */
#define LL_RTC_MONTH_JANUARY               ((uint8_t)0x01U)  /*!< January   **/
#define LL_RTC_MONTH_FEBRUARY              ((uint8_t)0x02U)  /*!< February  **/
#define LL_RTC_MONTH_MARCH                 ((uint8_t)0x03U)  /*!< March     **/
#define LL_RTC_MONTH_APRIL                 ((uint8_t)0x04U)  /*!< April     **/
#define LL_RTC_MONTH_MAY                   ((uint8_t)0x05U)  /*!< May       **/
#define LL_RTC_MONTH_JUNE                  ((uint8_t)0x06U)  /*!< June      **/
#define LL_RTC_MONTH_JULY                  ((uint8_t)0x07U)  /*!< July      **/
#define LL_RTC_MONTH_AUGUST                ((uint8_t)0x08U)  /*!< August    **/
#define LL_RTC_MONTH_SEPTEMBER             ((uint8_t)0x09U)  /*!< September **/
#define LL_RTC_MONTH_OCTOBER               ((uint8_t)0x10U)  /*!< October   **/
#define LL_RTC_MONTH_NOVEMBER              ((uint8_t)0x11U)  /*!< November  **/
#define LL_RTC_MONTH_DECEMBER              ((uint8_t)0x12U)  /*!< December  **/
/**
 * @}
 */

/** @defgroup RTC_LL_EC_DEBUGMODE  DEBUG MODE
 * @{
 */
#define LL_RTC_DEBUGMODE_CONTINUE          RTC_CNT2_DBGSTB_COUNTINUE  /*!< RTC counterx continues in debug mode **/
#define LL_RTC_DEBUGMODE_PAUSE             RTC_CNT2_DBGSTB_PAUSE      /*!< RTC counterx paused in debug mode **/
/**
 * @}
 */ /* end of group RTC_LL_EC_DEBUGMODE **/

/** @defgroup RTC_LL_EC_CLOCKSOURCE  CLOCK SOURCE
 * @{
 */
#define LL_RTC_CNT2CLOCKSOURCE_RCL         RTC_CNT2_CLK_SEL_RCL      /*!< RCL clock source **/
#define LL_RTC_CNT2CLOCKSOURCE_LOSC        RTC_CNT2_CLK_SEL_LOSC     /*!< 32768Hz LOSC clock source **/
/**
 * @}
 */ /* end of group RTC_LL_EC_CLOCKSOURCE **/

/** @defgroup RTC_LL_EC_1S_CLOCKSOURCE 1S Clock Source
 * @{
 */
#define LL_RTC_1S_CLOCKSOURCE_SYSCLK         RTC_IE_RTC_1S_SEL_SYSCLK           /*!< Interrupt 1s and Counter1 clocksource, Select by system mode (HCMM: PLL_1Hz, not HCMM: RTC_1Hz) **/
#define LL_RTC_1S_CLOCKSOURCE_RTC            RTC_IE_RTC_1S_SEL_RTC              /*!< Interrupt 1s and Counter1 clocksource, Select RTC_1Hz*/
/**
 * @}
 */ /* end of group RTC_LL_EC_1S_CLOCKSOURCE **/

/** @defgroup RTC_LL_EC_TAMPERTYPE TamperType
 * @{
 */
#define LL_RTC_TAMPERTYPE_TAMPER1N0              ((uint32_t)0x00U)  /*!< TAMPER1N0  TAMPER1 No.0 Negative edge **/
#define LL_RTC_TAMPERTYPE_TAMPER1P0              ((uint32_t)0x01U)  /*!< TAMPER1P0  TAMPER1 No.0 Positive edge **/
#define LL_RTC_TAMPERTYPE_TAMPER1N1              ((uint32_t)0x02U)  /*!< TAMPER1N1  TAMPER1 No.1 Negative edge **/
#define LL_RTC_TAMPERTYPE_TAMPER1P1              ((uint32_t)0x03U)  /*!< TAMPER1P1  TAMPER1 No.1 Positive edge **/
#define LL_RTC_TAMPERTYPE_TAMPER2N0              ((uint32_t)0x04U)  /*!< TAMPER2N0  TAMPER2 No.0 Negative edge **/
#define LL_RTC_TAMPERTYPE_TAMPER2P0              ((uint32_t)0x05U)  /*!< TAMPER2P0  TAMPER2 No.0 Positive edge **/
#define LL_RTC_TAMPERTYPE_TAMPER2N1              ((uint32_t)0x06U)  /*!< TAMPER2N1  TAMPER2 No.1 Negative edge **/
#define LL_RTC_TAMPERTYPE_TAMPER2P1              ((uint32_t)0x07U)  /*!< TAMPER2P1  TAMPER2 No.1 Positive edge **/
/**
 * @}
 */ /* end of group RTC_LL_EC_TAMPERTYPE **/

/** @defgroup RTC_LL_EC_BKP  BACKUP
 * @{
 */
#define LL_RTC_BKP_DR0                     0x00000000U
#define LL_RTC_BKP_DR1                     0x00000001U
#define LL_RTC_BKP_DR2                     0x00000002U
#define LL_RTC_BKP_DR3                     0x00000003U
#define LL_RTC_BKP_DR4                     0x00000004U
#define LL_RTC_BKP_DR5                     0x00000005U
#define LL_RTC_BKP_DR6                     0x00000006U
#define LL_RTC_BKP_DR7                     0x00000007U
#define LL_RTC_BKP_DR8                     0x00000008U
#define LL_RTC_BKP_DR9                     0x00000009U
#define LL_RTC_BKP_DR10                    0x0000000AU
#define LL_RTC_BKP_DR11                    0x0000000BU
#define LL_RTC_BKP_DR12                    0x0000000CU
#define LL_RTC_BKP_DR13                    0x0000000DU
#define LL_RTC_BKP_DR14                    0x0000000EU
#define LL_RTC_BKP_DR15                    0x0000000FU
/**
 * @}
 */ /* end of group RTC_LL_EC_BKP **/

/** @defgroup RTC_LL_EC_TempRange  TempRange
 * @{
 */
#define LL_RTC_TEMPRANGE_BELOW_MINUS30     0x00000000U        /*!< tempareture compension value for  T < -30°C **/
#define LL_RTC_TEMPRANGE_MINUS_30_20       0x00000001U        /*!< tempareture compension value for  -30°C ≤ T < -20°C **/
#define LL_RTC_TEMPRANGE_MINUS_20_10       0x00000002U        /*!< tempareture compension value for  -20°C ≤ T < -10°C **/
#define LL_RTC_TEMPRANGE_MINUS_10_0        0x00000003U        /*!< tempareture compension value for  -10°C ≤ T < 0°C **/
#define LL_RTC_TEMPRANGE_0_10              0x00000004U        /*!< tempareture compension value for  0°C ≤ T <= 10°C **/
#define LL_RTC_TEMPRANGE_35_45             0x00000005U        /*!< tempareture compension value for  35°C < T <= 45°C **/
#define LL_RTC_TEMPRANGE_45_55             0x00000006U        /*!< tempareture compension value for  45°C < T <= 55°C **/
#define LL_RTC_TEMPRANGE_55_65             0x00000007U        /*!< tempareture compension value for  55°C < T <= 65°C **/
#define LL_RTC_TEMPRANGE_65_75             0x00000008U        /*!< tempareture compension value for  65°C < T <= 75°C **/
#define LL_RTC_TEMPRANGE_ABOVE_75          0x00000009U        /*!< tempareture compension value for  T > 75°C **/
/**
 * @}
 */ /* end of group RTC_LL_EC_TempRange **/

/** @defgroup RTC_LL_EC_TEMPCOMPMODE Temperature compensation mode
 * @{
 */
#define LL_RTC_TEMPCOMPMODE_DIS          RTC_CTRL_TSE_DIS               /*!< Disable automatic temperature compensation **/
#define LL_RTC_TEMPCOMPMODE_AUTO         RTC_CTRL_TSE_AUTOEN            /*!< Enable automatic periodic temperature compensation **/
#define LL_RTC_TEMPCOMPMODE_USER0        RTC_CTRL_TSE_USER0EN           /*!< Enable user mode 0, user sets temperature to trigger automatic compensation **/
#define LL_RTC_TEMPCOMPMODE_USER1        RTC_CTRL_TSE_USER1EN           /*!< Enable user mode 1, trigger automatic compensation with fixed temperature **/
/**
 * @}
 */ /* end of group RTC_LL_EC_TEMPCOMPMODE */

/** @defgroup RTC_LL_EC_TEMPCOMPPERIOD Temperature compensation period
 * @{
 */
#define LL_RTC_TEMPCOMPPERIOD_2S           RTC_CTRL_TCP_2S              /*!< select temperature compensation period as 2 second(or 0.5s for 8025T) */
#define LL_RTC_TEMPCOMPPERIOD_10S          RTC_CTRL_TCP_10S             /*!< select temperature compensation period as 10 second(or 2s for 8025T) */
#define LL_RTC_TEMPCOMPPERIOD_20S          RTC_CTRL_TCP_20S             /*!< select temperature compensation period as 20 second(or 10s for 8025T) */
#define LL_RTC_TEMPCOMPPERIOD_30S          RTC_CTRL_TCP_30S             /*!< select temperature compensation period as 30 second(or 30s for 8025T) */
#define LL_RTC_TEMPCOMPPERIOD_1MIN         RTC_CTRL_TCP_1MIN            /*!< select temperature compensation period as 1 minute(not for 8025T) */
#define LL_RTC_TEMPCOMPPERIOD_2MIN         RTC_CTRL_TCP_2MIN            /*!< select temperature compensation period as 2 minute(not for 8025T) */
#define LL_RTC_TEMPCOMPPERIOD_5MIN         RTC_CTRL_TCP_5MIN            /*!< select temperature compensation period as 5 minute(not for 8025T) */
#define LL_RTC_TEMPCOMPPERIOD_10MIN        RTC_CTRL_TCP_10MIN           /*!< select temperature compensation period as 10 minute(not for 8025T) */
/**
 * @}
 */ /* end of group RTC_LL_EC_TEMPCOMPPERIOD */

/** @defgroup RTC_LL_EC_CurveType CurveType CURVETYPE
 * @{
 */
#define LL_RTC_CURVETYPE_QUADRATIC       0U                  /*!< select temperature compensation curve type as quadratic curve */
#define LL_RTC_CURVETYPE_QUARTIC         0xEA65U             /*!< select temperature compensation curve type as quartic  curve */
/**
 * @}
 */

/** @defgroup RTC_LL_EC_PL00FUNCTION PL00 IO Function
 * @{
 */
#define LL_RTC_PL00FUNCTION_RTCOUT             RTC_IOEN_PL00EN_RTCOUT                        /*!< (IO function) enable pin as  RTC_OUT */
#define LL_RTC_PL00FUNCTION_KEYOUT             RTC_IOEN_PL00EN_KEYOUT                        /*!< (IO function) select as KEYOUT output pin */
#define LL_RTC_PL00FUNCTION_NUL                RTC_IOEN_PL00EN_NUL                           /*!< Not function for RTC */
/**
 * @}
 */ /* end of group RTC_LL_EC_PL00FUNCTION */

/** @defgroup RTC_LL_EC_PL01FUNCTION PL01 IO Function
 * @{
 */
#define LL_RTC_PL01FUNCTION_IN                    RTC_IOEN_PL01EN_IN                              /*!< (IO function) enable pin as TAMPER1 input or KEY0[6] input */
#define LL_RTC_PL01FUNCTION_TAMPER1CLR            RTC_IOEN_PL01EN_TAMPER1CLR                      /*!< clean registers result of TAMPER1 timestamp */
#define LL_RTC_PL01FUNCTION_KEYOUT                RTC_IOEN_PL01EN_KEYOUT                          /*!< (IO function) select as KEYOUT output pin */
#define LL_RTC_PL01FUNCTION_NUL                   RTC_IOEN_PL01EN_NUL                             /*!< Not function for RTC */
/**
 * @}
 */ /* end of group RTC_LL_EC_PL01FUNCTION */

/** @defgroup RTC_LL_EC_PL02FUNCTION PL02 IO Function
 * @{
 */
#define LL_RTC_PL02FUNCTION_IN                    RTC_IOEN_PL02EN_IN                            /*!< (IO function) enable pin as TAMPER2 input or KEY0[7] input */
#define LL_RTC_PL02FUNCTION_TAMPER2CLR            RTC_IOEN_PL02EN_TAMPER2CLR                    /*!< clean registers result of TAMPER2 timestamp  */
#define LL_RTC_PL02FUNCTION_KEYOUT                RTC_IOEN_PL02EN_KEYOUT                        /*!< (IO function) select as KEYOUT output pin */
#define LL_RTC_PL02FUNCTION_NUL                   RTC_IOEN_PL02EN_NUL                           /*!< Not function for RTC */
/**
 * @}
 */ /* end of group RTC_LL_EC_PL02FUNCTION */

/** @defgroup RTC_LL_EC_PL03FUNCTION PL03 IO Function
 * @{
 */
#define LL_RTC_PL03FUNCTION_KEYOUT                RTC_IOEN_PL03EN_KEYOUT                   /*!< (IO function) select as KEYOUT output pin */
#define LL_RTC_PL03FUNCTION_NUL                   RTC_IOEN_PL03EN_NUL                      /*!< Not function for RTC */
/**
 * @}
 */ /* end of group RTC_LL_EC_PL03FUNCTION */

/** @defgroup RTC_LL_EC_TAMPERMODE TAMPER Detection Mode
 * @{
 */
#define LL_RTC_TAMPERMODE_EST              RTC_IOMODE_TAMPER1_MODE_EST               /*!< record the two earliest press key event */
#define LL_RTC_TAMPERMODE_LST              RTC_IOMODE_TAMPER1_MODE_LST               /*!< record the two lastest press key event */
/**
 * @}
 */ /* end of group RTC_LL_EC_TAMPERMODE */

/** @defgroup RTC_LL_EC_FILTER_PRECISION TAMPER Filter Precision
 * @{
 */
#define LL_RTC_TAMPER_FILTER_PREC_2      RTC_IOMODE_TAMPER_FLT_TS_2                /*!< Select 2ms filter precision */
#define LL_RTC_TAMPER_FILTER_PREC_250    RTC_IOMODE_TAMPER_FLT_TS_250              /*!< Select 250ms filter precision */
/**
 * @}
 */ /* end of group RTC_LL_EC_FILTER_PRECISION */

/** @defgroup RTC_LL_EC_KEYOUT_MODE KEYOUT Release Level
 * @{
 */
#define LL_RTC_KEYOUT_RELEASELEVEL_HIGH     RTC_IOMODE_KEYOUT_MODE_HIZ                /*!< Outputs low level when triggered; high-Z when released */
#define LL_RTC_KEYOUT_RELEASELEVEL_HIZ      RTC_IOMODE_KEYOUT_MODE_HL                 /*!< Outputs low level when triggered; high level when released */
/**
 * @}
 */ /* end of group RTC_LL_EC_KEYOUT_MODE */

/** @defgroup RTC_LL_EC_TIMED_TRIGGER_TIMEBASE KEYOUT Timed Trigger Time Base
 * @{
 */
#define LL_RTC_TIMEBASE_SEC                 RTC_KEYOUT_TK_CFG_TB_1MIN              /*!< Second as time base */
#define LL_RTC_TIMEBASE_MIN                 RTC_KEYOUT_TK_CFG_TB_1S                /*!< Minute as time base */
/**
 * @}
 */ /* end of group RTC_LL_EC_TIMED_TRIGGER_TIMEBASE */

/** @defgroup RTC_LL_OUTPUTFREQTYPE RTCOUT frequency type
 * @{
 */
#define LL_RTC_OUTPUTFREQTYPE_NUL            RTC_CTRL_FOUT_NUL                   /*!< RTCOUT forbidden */
#define LL_RTC_OUTPUTFREQTYPE_PLLOUT         RTC_CTRL_FOUT_PLLOUT                /*!< select RTCOUT as PLL, freq depend on PLLOUT_SEL */
#define LL_RTC_OUTPUTFREQTYPE_1_30HZ         RTC_CTRL_FOUT_1_30HZ                /*!< select RTCOUT as LOSC, frequency of 1/30Hz */
#define LL_RTC_OUTPUTFREQTYPE_32KHZ          RTC_CTRL_FOUT_32KHZ                 /*!< select RTCOUT as LOSC, frequency of 32768Hz */
#define LL_RTC_OUTPUTFREQTYPE_16HZ           RTC_CTRL_FOUT_16HZ                  /*!< select RTCOUT as LOSC, frequency of 16Hz */
#define LL_RTC_OUTPUTFREQTYPE_8HZ            RTC_CTRL_FOUT_8HZ                   /*!< select RTCOUT as LOSC, frequency of 8Hz */
#define LL_RTC_OUTPUTFREQTYPE_4HZ            RTC_CTRL_FOUT_4HZ                   /*!< select RTCOUT as LOSC, frequency of 4Hz */
#define LL_RTC_OUTPUTFREQTYPE_1HZ            RTC_CTRL_FOUT_1HZ                   /*!< select RTCOUT as LOSC, frequency of 1Hz */
/**
 * @}
 */ /* end of group RTC_LL_OUTPUTFREQTYPE */

/** @defgroup RTC_LL_PLLOUTFREQ  RTCOUT pll frequency
 * @{
 */
#define LL_RTC_PLLOUTFREQ_1HZ               RTC_PLLOUT_CFG_FREQ_SEL_1HZ              /*!< config pll output as 1HZ */
#define LL_RTC_PLLOUTFREQ_2HZ               RTC_PLLOUT_CFG_FREQ_SEL_2HZ              /*!< config pll output as 2HZ */
#define LL_RTC_PLLOUTFREQ_4HZ               RTC_PLLOUT_CFG_FREQ_SEL_4HZ              /*!< config pll output as 4HZ */
#define LL_RTC_PLLOUTFREQ_8HZ               RTC_PLLOUT_CFG_FREQ_SEL_8HZ              /*!< config pll output as 8HZ */
#define LL_RTC_PLLOUTFREQ_16HZ              RTC_PLLOUT_CFG_FREQ_SEL_16HZ             /*!< config pll output as 16HZ */
/**
 * @}
 */ /* end of group RTC_LL_PLLOUTFREQ */

/**
 * @}
 */ /* end of group RTC_LL_Exported_Constants **/

/* Exported macro -----------------------------------------------------------*/
/** @defgroup RTC_LL_Exported_Macros RTC Exported Macros
 * @{
 */

/** @defgroup RTC_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in RTC register
 * @param  __INSTANCE__ RTC Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_RTC_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in RTC register
 * @param  __INSTANCE__ RTC Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_RTC_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/** @defgroup RTC_LL_EM_Convert Convert helper Macros
 * @{
 */

/**
 * @brief  Helper macro to convert a value from 2 digit decimal format to BCD format
 * @param  __VALUE__ Byte to be converted
 * @retval Converted byte
 */
#define __LL_RTC_CONVERT_BIN2BCD(__VALUE__) (uint8_t)((((__VALUE__) / 10U) << 4U) | ((__VALUE__) % 10U))

/**
 * @brief  Helper macro to convert a value from BCD format to 2 digit decimal format
 * @param  __VALUE__ BCD value to be converted
 * @retval Converted byte
 */
#define __LL_RTC_CONVERT_BCD2BIN(__VALUE__) (uint8_t)(((uint8_t)((__VALUE__) & (uint8_t)0xF0U) >> (uint8_t)0x4U) * 10U + ((__VALUE__) & (uint8_t)0x0FU))

/**
 * @}
 */

/** @defgroup RTC_LL_EM_Date Date helper Macros
 * @{
 */

/**
 * @brief  Helper macro to retrieve weekday.
 * @param  __RTC_DATE__ Date returned by @ref  LL_RTC_DATE_Get function.
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_WEEKDAY_MONDAY
 *         @arg @ref LL_RTC_WEEKDAY_TUESDAY
 *         @arg @ref LL_RTC_WEEKDAY_WEDNESDAY
 *         @arg @ref LL_RTC_WEEKDAY_THURSDAY
 *         @arg @ref LL_RTC_WEEKDAY_FRIDAY
 *         @arg @ref LL_RTC_WEEKDAY_SATURDAY
 *         @arg @ref LL_RTC_WEEKDAY_SUNDAY
 */
#define __LL_RTC_GET_WEEKDAY(__RTC_DATE__) (((__RTC_DATE__) >> RTC_OFFSET_WEEKDAY) & 0x000000FFU)

/**
 * @brief  Helper macro to retrieve Year in BCD format
 * @param  __RTC_DATE__ Value returned by @ref  LL_RTC_DATE_Get
 * @retval Year in BCD format (0x00 . . . 0x99)
 */
#define __LL_RTC_GET_YEAR(__RTC_DATE__) ((__RTC_DATE__) & 0x000000FFU)

/**
 * @brief  Helper macro to retrieve Month in BCD format
 * @param  __RTC_DATE__ Value returned by @ref  LL_RTC_DATE_Get
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_MONTH_JANUARY
 *         @arg @ref LL_RTC_MONTH_FEBRUARY
 *         @arg @ref LL_RTC_MONTH_MARCH
 *         @arg @ref LL_RTC_MONTH_APRIL
 *         @arg @ref LL_RTC_MONTH_MAY
 *         @arg @ref LL_RTC_MONTH_JUNE
 *         @arg @ref LL_RTC_MONTH_JULY
 *         @arg @ref LL_RTC_MONTH_AUGUST
 *         @arg @ref LL_RTC_MONTH_SEPTEMBER
 *         @arg @ref LL_RTC_MONTH_OCTOBER
 *         @arg @ref LL_RTC_MONTH_NOVEMBER
 *         @arg @ref LL_RTC_MONTH_DECEMBER
 */
#define __LL_RTC_GET_MONTH(__RTC_DATE__) (((__RTC_DATE__) >>RTC_OFFSET_MONTH) & 0x000000FFU)

/**
 * @brief  Helper macro to retrieve Day in BCD format
 * @param  __RTC_DATE__ Value returned by @ref  LL_RTC_DATE_Get
 * @retval Day in BCD format (0x01 . . . 0x31)
 */
#define __LL_RTC_GET_DAY(__RTC_DATE__) (((__RTC_DATE__) >>RTC_OFFSET_DAY) & 0x000000FFU)

/**
 * @}
 */

/** @defgroup RTC_LL_EM_Time Time helper Macros
 * @{
 */

/**
 * @brief  Helper macro to retrieve hour in BCD format
 * @param  __RTC_TIME__ RTC time returned by @ref LL_RTC_TIME_Get function
 * @retval Hours in BCD format (0x01. . .0x12 or between Min_Data=0x00 and Max_Data=0x23)
 */
#define __LL_RTC_GET_HOUR(__RTC_TIME__) (((__RTC_TIME__) >> RTC_OFFSET_HOUR) & 0x000000FFU)

/**
 * @brief  Helper macro to retrieve minute in BCD format
 * @param  __RTC_TIME__ RTC time returned by @ref LL_RTC_TIME_Get function
 * @retval Minutes in BCD format (0x00. . .0x59)
 */
#define __LL_RTC_GET_MINUTE(__RTC_TIME__) (((__RTC_TIME__) >> RTC_OFFSET_MINUTE) & 0x000000FFU)

/**
 * @brief  Helper macro to retrieve second in BCD format
 * @param  __RTC_TIME__ RTC time returned by @ref LL_RTC_TIME_Get function
 * @retval Seconds in  format (0x00. . .0x59)
 */
#define __LL_RTC_GET_SECOND(__RTC_TIME__) ((__RTC_TIME__) & 0x000000FFU)

/**
 * @}
 */ /* end of group RTC_LL_EM_Time **/
/**
 * @}
 */ /* end of group RTC_LL_Exported_Macros **/

/* Exported functions -------------------------------------------------------*/
/** @defgroup RTC_LL_Exported_Functions RTC Exported Functions
 * @{
 */

/** @defgroup RTC_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief  Enable the write protection for RTC registers.
 * @rmtoll CTRL           WRTC           LL_RTC_EnableConfigWriteProtection
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_EnableConfigWriteProtection(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->CTRL, RTC_CTRL_WRTC_DIS);
}

/**
 * @brief  Disable the write protection for RTC registers.
 * @rmtoll CTRL           WRTC           LL_RTC_DisableConfigWriteProtection
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_DisableConfigWriteProtection(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->CTRL, RTC_CTRL_WRTC_EN);
}

/**
 * @brief  Set the RTCOUT output freq type.
 * @note   Bits is write-protected. @ref LL_RTC_DisableConfigWriteProtection function should be called before.
 * @rmtoll CTRL         FOUT            LL_RTC_SetOutputFreqType
 * @param  RTCx RTC Instance
 * @param  OutputFreqType This parameter can be one of the following values:
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_NUL
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_PLLOUT
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_1_30HZ
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_32KHZ
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_16HZ
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_8HZ
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_4HZ
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_1HZ
 * @retval None
 */
__STATIC_INLINE void LL_RTC_SetOutputFreqType(RTC_TypeDef *RTCx, uint32_t OutputFreqType)
{
    MODIFY_REG(RTCx->CTRL, RTC_CTRL_FOUT, OutputFreqType);
}

/**
 * @brief  Get RTCOUT the output freq type.
 * @rmtoll CTRL         FOUT            LL_RTC_GetOutputFreqType
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_NUL
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_PLLOUT
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_1_30HZ
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_32KHZ
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_16HZ
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_8HZ
 *         @arg @ref LL_RTC_OUTPUTFREQTYPE_4HZ
 */
__STATIC_INLINE uint32_t LL_RTC_GetOutputFreqType(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->CTRL, RTC_CTRL_FOUT));
}

/**
 * @brief  Set the RTCOUT PLLOUT type output freq.
 * @note   Bits is write-protected. @ref LL_RTC_DisableConfigWriteProtection function should be called before.
 * @note   Only available when setted LL_RTC_OUTPUTFREQTYPE_PLLOUT by @ref LL_RTC_SetOutputFreqType
 * @rmtoll PLLOUT_CFG         FREQ_SEL            LL_RTC_SetPLLOutFreq
 * @param  RTCx RTC Instance
 * @param  PLLOutFreq This parameter can be one of the following values:
 *         @arg @ref LL_RTC_PLLOUTFREQ_1HZ
 *         @arg @ref LL_RTC_PLLOUTFREQ_2HZ
 *         @arg @ref LL_RTC_PLLOUTFREQ_4HZ
 *         @arg @ref LL_RTC_PLLOUTFREQ_8HZ
 *         @arg @ref LL_RTC_PLLOUTFREQ_16HZ
 * @retval None
 */
__STATIC_INLINE void LL_RTC_SetPLLOutFreq(RTC_TypeDef *RTCx, uint32_t PLLOutFreq)
{
    MODIFY_REG(RTCx->PLLOUT_CFG, RTC_PLLOUT_CFG_FREQ_SEL, PLLOutFreq);
}

/**
 * @brief  Get the RTCOUT PLLOUT type output freq.
 * @rmtoll PLLOUT_CFG         FREQ_SEL            LL_RTC_GetPLLOutFreq
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_PLLOUTFREQ_1HZ
 *         @arg @ref LL_RTC_PLLOUTFREQ_2HZ
 *         @arg @ref LL_RTC_PLLOUTFREQ_4HZ
 *         @arg @ref LL_RTC_PLLOUTFREQ_8HZ
 *         @arg @ref LL_RTC_PLLOUTFREQ_16HZ
 */
__STATIC_INLINE uint32_t LL_RTC_GetPLLOutFreq(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->PLLOUT_CFG, RTC_PLLOUT_CFG_FREQ_SEL));
}

/**
 * @}
 */ /* end of group RTC_LL_EF_Configuration **/

/** @defgroup RTC_LL_EF_Time Time
 * @{
 */


/**
 * @brief  Set Hours in BCD format
 * @note   Bits is write-protected. @ref LL_RTC_DisableConfigWriteProtection function should be called before.
 * @note helper macro __LL_RTC_CONVERT_BIN2BCD is available to convert hour from binary to BCD format
 * @note   @ref LL_RTC_TIME_SetSecond  will actually write all Time and date configuration to chip hardware.
 * @rmtoll HR           HR            LL_RTC_TIME_SetHour
 * @param  RTCx RTC Instance
 * @param  Hours Value between Min_Data=0x00 and Max_Data=0x23
 * @retval None
 */
__STATIC_INLINE void LL_RTC_TIME_SetHour(RTC_TypeDef *RTCx, uint32_t Hours)
{
    MODIFY_REG(RTCx->HR, RTC_HR_HR, Hours);
}

/**
 * @brief  Get Hours in BCD format
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert hour from BCD to
 *       Binary format
 * @rmtoll HR           HR            LL_RTC_TIME_GetHour
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x01 and Max_Data=0x12 or between Min_Data=0x00 and Max_Data=0x23
 */
__STATIC_INLINE uint32_t LL_RTC_TIME_GetHour(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->HR, RTC_HR_HR)));
}


/**
 * @brief  Set Minutes in BCD format
 * @note   Bits is write-protected. @ref LL_RTC_DisableConfigWriteProtection function should be called before.
 * @note   helper macro __LL_RTC_CONVERT_BIN2BCD is available to convert Minutes from binary to BCD format
 * @note   @ref LL_RTC_TIME_SetSecond  will actually write all Time and date configuration to chip hardware.
 * @rmtoll MN           MN           LL_RTC_TIME_SetMinute
 * @param  RTCx RTC Instance
 * @param  Minutes Value between Min_Data=0x00 and Max_Data=0x59
 * @retval None
 */
__STATIC_INLINE void LL_RTC_TIME_SetMinute(RTC_TypeDef *RTCx, uint32_t Minutes)
{
    MODIFY_REG(RTCx->MN, RTC_MN_MN, Minutes );
}

/**
 * @brief  Get Minutes in BCD format
 * @note   helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert minute from BCD
 *       to Binary format
 * @rmtoll MN           MN           LL_RTC_TIME_GetMinute
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x59
 */
__STATIC_INLINE uint32_t LL_RTC_TIME_GetMinute(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->MN, RTC_MN_MN ));
}

/**
 * @brief  Set Seconds in BCD format
 * @note   Bits is write-protected. @ref LL_RTC_DisableConfigWriteProtection function should be called before.
 * @note   helper macro __LL_RTC_CONVERT_BIN2BCD is available to convert Seconds from binary to BCD format
 * @note   @ref LL_RTC_TIME_SetSecond  will actually write all Time and date configuration to chip hardware.
 * @rmtoll SC           SC           LL_RTC_TIME_SetSecond
 * @param  RTCx RTC Instance
 * @param  Seconds Value between Min_Data=0x00 and Max_Data=0x59
 * @retval None
 */
__STATIC_INLINE void LL_RTC_TIME_SetSecond(RTC_TypeDef *RTCx, uint32_t Seconds)
{
    MODIFY_REG(RTCx->SC, RTC_SC_SC, Seconds);
}

/**
 * @brief  Get Seconds in BCD format
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Seconds from BCD
 *       to Binary format
 * @rmtoll SC           SC           LL_RTC_TIME_GetSecond
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x59
 */
__STATIC_INLINE uint32_t LL_RTC_TIME_GetSecond(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->SC, RTC_SC_SC));
}


/**
 * @brief  Set time (hour, minute and second) in BCD format
 * @note   Bits is write-protected. @ref LL_RTC_DisableConfigWriteProtection function should be called before.
 * @note   @ref LL_RTC_TIME_SetSecond  will actually write all Time and date configuration to chip hardware.
 * @rmtoll HR           HR            LL_RTC_TIME_Config\n
 *         MN           MN            LL_RTC_TIME_Config\n
 *         SC           SC            LL_RTC_TIME_Config
 * @param  RTCx RTC Instance
 * @param  Hours Value between Min_Data=0x00 and Max_Data=0x23
 * @param  Minutes Value between Min_Data=0x00 and Max_Data=0x59
 * @param  Seconds Value between Min_Data=0x00 and Max_Data=0x59
 * @retval None
 */
__STATIC_INLINE void LL_RTC_TIME_Config(RTC_TypeDef *RTCx,  uint32_t Hours, uint32_t Minutes, uint32_t Seconds)
{
    MODIFY_REG(RTCx->HR, RTC_HR_HR, Hours);
    MODIFY_REG(RTCx->MN, RTC_MN_MN, Minutes );
    MODIFY_REG(RTCx->SC, RTC_SC_SC, Seconds);
}

/**
 * @brief  Get time (hour, minute and second) in BCD format
 * @note helper macros __LL_RTC_GET_HOUR, __LL_RTC_GET_MINUTE and __LL_RTC_GET_SECOND
 *       are available to get independently each parameter.
 * @rmtoll HR           HR            LL_RTC_TIME_Get\n
 *         MN           MN            LL_RTC_TIME_Get\n
 *         SC           SC            LL_RTC_TIME_Get
 * @param  RTCx RTC Instance
 * @retval Combination of hours, minutes and seconds (Format: 0x00HHMMSS).
 */
__STATIC_INLINE uint32_t LL_RTC_TIME_Get(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->HR, RTC_HR_HR) << RTC_OFFSET_HOUR)\
                    ||(READ_BIT(RTCx->MN, RTC_MN_MN) << RTC_OFFSET_MINUTE)\
                    ||(READ_BIT(RTCx->SC, RTC_SC_SC)));
}

/**
 * @brief  Get subseconds value in the synchronous prescaler counter.
 * @note   1 for 1/32768 s.
 * @rmtoll SSC          SSC            LL_RTC_TIME_GetSubSecond
 * @param  RTCx RTC Instance
 * @retval Subseconds value (number between 0 and 65535)
 */
__STATIC_INLINE uint32_t LL_RTC_TIME_GetSubSecond(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->SSC, RTC_SSC_SSC));
}
/**
 * @}
 */ /* end of group RTC_LL_EF_Time **/


/** @defgroup RTC_LL_EF_Date Date
 * @{
 */

/**
 * @brief  Set Year in BCD format
 * @note   Bits is write-protected. @ref LL_RTC_DisableConfigWriteProtection function should be called before.
 * @note   @ref LL_RTC_TIME_SetSecond  will actually write all Time and date configuration to chip hardware.
 * @note helper macro __LL_RTC_CONVERT_BIN2BCD is available to convert Year from binary to BCD format
 * @rmtoll YR           YR            LL_RTC_DATE_SetYear
 * @param  RTCx RTC Instance
 * @param  Year Value between Min_Data=0x00 and Max_Data=0x99
 * @retval None
 */
__STATIC_INLINE void LL_RTC_DATE_SetYear(RTC_TypeDef *RTCx, uint32_t Year)
{
    MODIFY_REG(RTCx->YR, RTC_YR_YR, Year);
}

/**
 * @brief  Get Year in BCD format
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Year from BCD to Binary format
 * @rmtoll YR           YR            LL_RTC_DATE_GetYear
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x99
 */
__STATIC_INLINE uint32_t LL_RTC_DATE_GetYear(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->YR, RTC_YR_YR));
}

/**
 * @brief  Set Week day
 * @note   Bits is write-protected. @ref LL_RTC_DisableConfigWriteProtection function should be called before.
 * @note   @ref LL_RTC_TIME_SetSecond  will actually write all Time and date configuration to chip hardware.
 * @rmtoll DW           DW           LL_RTC_DATE_SetWeekDay
 * @param  RTCx RTC Instance
 * @param  WeekDay This parameter can be one of the following values:
 *         @arg @ref LL_RTC_WEEKDAY_MONDAY
 *         @arg @ref LL_RTC_WEEKDAY_TUESDAY
 *         @arg @ref LL_RTC_WEEKDAY_WEDNESDAY
 *         @arg @ref LL_RTC_WEEKDAY_THURSDAY
 *         @arg @ref LL_RTC_WEEKDAY_FRIDAY
 *         @arg @ref LL_RTC_WEEKDAY_SATURDAY
 *         @arg @ref LL_RTC_WEEKDAY_SUNDAY
 * @retval None
 */
__STATIC_INLINE void LL_RTC_DATE_SetWeekDay(RTC_TypeDef *RTCx, uint32_t WeekDay)
{
    MODIFY_REG(RTCx->DW, RTC_DW_DW, WeekDay);
}

/**
 * @brief  Get Week day
 * @rmtoll DW           DW           LL_RTC_DATE_GetWeekDay
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_WEEKDAY_MONDAY
 *         @arg @ref LL_RTC_WEEKDAY_TUESDAY
 *         @arg @ref LL_RTC_WEEKDAY_WEDNESDAY
 *         @arg @ref LL_RTC_WEEKDAY_THURSDAY
 *         @arg @ref LL_RTC_WEEKDAY_FRIDAY
 *         @arg @ref LL_RTC_WEEKDAY_SATURDAY
 *         @arg @ref LL_RTC_WEEKDAY_SUNDAY
 */
__STATIC_INLINE uint32_t LL_RTC_DATE_GetWeekDay(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->DW, RTC_DW_DW));
}

/**
 * @brief  Set Month in BCD format
 * @note   Bits is write-protected. @ref LL_RTC_DisableConfigWriteProtection function should be called before.
 * @note   @ref LL_RTC_TIME_SetSecond  will actually write all Time and date configuration to chip hardware.
 * @note helper macro __LL_RTC_CONVERT_BIN2BCD is available to convert Month from binary to BCD format
 * @rmtoll MO           MO            LL_RTC_DATE_SetMonth\n
 *         MO           MU            LL_RTC_DATE_SetMonth
 * @param  RTCx RTC Instance
 * @param  Month This parameter can be one of the following values:
 *         @arg @ref LL_RTC_MONTH_JANUARY
 *         @arg @ref LL_RTC_MONTH_FEBRUARY
 *         @arg @ref LL_RTC_MONTH_MARCH
 *         @arg @ref LL_RTC_MONTH_APRIL
 *         @arg @ref LL_RTC_MONTH_MAY
 *         @arg @ref LL_RTC_MONTH_JUNE
 *         @arg @ref LL_RTC_MONTH_JULY
 *         @arg @ref LL_RTC_MONTH_AUGUST
 *         @arg @ref LL_RTC_MONTH_SEPTEMBER
 *         @arg @ref LL_RTC_MONTH_OCTOBER
 *         @arg @ref LL_RTC_MONTH_NOVEMBER
 *         @arg @ref LL_RTC_MONTH_DECEMBER
 * @retval None
 */

__STATIC_INLINE void LL_RTC_DATE_SetMonth(RTC_TypeDef *RTCx, uint32_t Month)
{
    MODIFY_REG(RTCx->MO, RTC_MO_MO, Month);
}
/**
 * @brief  Get Month in BCD format
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Month from BCD to Binary format
 * @rmtoll MO           MO            LL_RTC_DATE_GetMonth
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_MONTH_JANUARY
 *         @arg @ref LL_RTC_MONTH_FEBRUARY
 *         @arg @ref LL_RTC_MONTH_MARCH
 *         @arg @ref LL_RTC_MONTH_APRIL
 *         @arg @ref LL_RTC_MONTH_MAY
 *         @arg @ref LL_RTC_MONTH_JUNE
 *         @arg @ref LL_RTC_MONTH_JULY
 *         @arg @ref LL_RTC_MONTH_AUGUST
 *         @arg @ref LL_RTC_MONTH_SEPTEMBER
 *         @arg @ref LL_RTC_MONTH_OCTOBER
 *         @arg @ref LL_RTC_MONTH_NOVEMBER
 *         @arg @ref LL_RTC_MONTH_DECEMBER
 */
__STATIC_INLINE uint32_t LL_RTC_DATE_GetMonth(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->MO, RTC_MO_MO));
}

/**
 * @brief  Set Day in BCD format
 * @note   Bits is write-protected. @ref LL_RTC_DisableConfigWriteProtection function should be called before.
 * @note   @ref LL_RTC_TIME_SetSecond  will actually write all Time and date configuration to chip hardware.
 * @note helper macro __LL_RTC_CONVERT_BIN2BCD is available to convert Day from binary to BCD format
 * @rmtoll DT           DT            LL_RTC_DATE_SetDay
 * @param  RTCx RTC Instance
 * @param  Day Value between Min_Data=0x01 and Max_Data=0x31
 * @retval None
 */
__STATIC_INLINE void LL_RTC_DATE_SetDay(RTC_TypeDef *RTCx, uint32_t Day)
{
    MODIFY_REG(RTCx->DT, RTC_DT_DT, Day);
}

/**
 * @brief  Get Day in BCD format
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Day from BCD to Binary format
 * @rmtoll DT           DT            LL_RTC_DATE_GetDay
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x01 and Max_Data=0x31
 */
__STATIC_INLINE uint32_t LL_RTC_DATE_GetDay(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->DT, RTC_DT_DT));
}

/**
 * @brief  Set date (WeekDay, Day, Month and Year) in BCD format
 * @note   Bits is write-protected. @ref LL_RTC_DisableConfigWriteProtection function should be called before.
 * @note   @ref LL_RTC_TIME_SetSecond  will actually write all Time and date configuration to chip hardware.
 * @rmtoll DW           DW            LL_RTC_DATE_Config\n
 *         MO           MO            LL_RTC_DATE_Config\n
 *         DT           DT            LL_RTC_DATE_Config\n
 *         YR           YR            LL_RTC_DATE_Config
 * @param  RTCx RTC Instance
 * @param  WeekDay This parameter can be one of the following values:
 *         @arg @ref LL_RTC_WEEKDAY_MONDAY
 *         @arg @ref LL_RTC_WEEKDAY_TUESDAY
 *         @arg @ref LL_RTC_WEEKDAY_WEDNESDAY
 *         @arg @ref LL_RTC_WEEKDAY_THURSDAY
 *         @arg @ref LL_RTC_WEEKDAY_FRIDAY
 *         @arg @ref LL_RTC_WEEKDAY_SATURDAY
 *         @arg @ref LL_RTC_WEEKDAY_SUNDAY
 * @param  Day Value between Min_Data=0x01 and Max_Data=0x31
 * @param  Month This parameter can be one of the following values:
 *         @arg @ref LL_RTC_MONTH_JANUARY
 *         @arg @ref LL_RTC_MONTH_FEBRUARY
 *         @arg @ref LL_RTC_MONTH_MARCH
 *         @arg @ref LL_RTC_MONTH_APRIL
 *         @arg @ref LL_RTC_MONTH_MAY
 *         @arg @ref LL_RTC_MONTH_JUNE
 *         @arg @ref LL_RTC_MONTH_JULY
 *         @arg @ref LL_RTC_MONTH_AUGUST
 *         @arg @ref LL_RTC_MONTH_SEPTEMBER
 *         @arg @ref LL_RTC_MONTH_OCTOBER
 *         @arg @ref LL_RTC_MONTH_NOVEMBER
 *         @arg @ref LL_RTC_MONTH_DECEMBER
 * @param  Year Value between Min_Data=0x00 and Max_Data=0x99
 * @retval None
 */
__STATIC_INLINE void LL_RTC_DATE_Config(RTC_TypeDef *RTCx, uint32_t WeekDay, uint32_t Day, uint32_t Month, uint32_t Year)
{
    MODIFY_REG(RTCx->DW, RTC_DW_DW, WeekDay);
    MODIFY_REG(RTCx->YR, RTC_YR_YR, Year);
    MODIFY_REG(RTCx->MO, RTC_MO_MO, Month);
    MODIFY_REG(RTCx->DT, RTC_DT_DT, Day);
}

/**
 * @brief  Get date (WeekDay, Day, Month and Year) in BCD format
 * @note helper macros __LL_RTC_GET_WEEKDAY, __LL_RTC_GET_YEAR, __LL_RTC_GET_MONTH,and __LL_RTC_GET_DAY
 *  are available to get independently each parameter.
 * @rmtoll DW           DW           LL_RTC_DATE_Get\n
 *         MO           MO           LL_RTC_DATE_Get\n
 *         DT           DT           LL_RTC_DATE_Get\n
 *         YR           YR           LL_RTC_DATE_Get
 * @param  RTCx RTC Instance
 * @retval Combination of WeekDay, Day, Month and Year (Format: 0xWWDDMMYY).
 */
__STATIC_INLINE uint32_t LL_RTC_DATE_Get(RTC_TypeDef *RTCx)
{

    return (uint32_t)((READ_BIT(RTCx->DW, RTC_DW_DW) << RTC_OFFSET_WEEKDAY)\
                    ||(READ_BIT(RTCx->DT, RTC_DT_DT) << RTC_OFFSET_DAY)\
                    ||(READ_BIT(RTCx->MO, RTC_MO_MO) << RTC_OFFSET_MONTH)\
                    ||(READ_BIT(RTCx->YR, RTC_YR_YR)));
}

/**
 * @}
 */ /* end of group RTC_LL_EF_Date **/


/** @defgroup RTC_LL_EF_Counter Counter functions
 * @{
 */

/**
 * @brief  Set Counter1 target value
 * @rmtoll CNT1         CNT           LL_RTC_SetCounter1TargetValue
 * @param  RTCx RTC Instance
 * @param  TargetValue between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_RTC_SetCounter1TargetValue(RTC_TypeDef *RTCx, uint32_t TargetValue)
{
    MODIFY_REG(RTCx->CNT1, RTC_CNT1_CNT, TargetValue);
}

/**
 * @brief  Get Counter1 target value
 * @rmtoll CNT1         CNT           LL_RTC_GetCounter1TargetValue
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF
 */
__STATIC_INLINE uint32_t LL_RTC_GetCounter1TargetValue(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->CNT1, RTC_CNT1_CNT));
}

/**
 * @brief  Enable Counter1
 * @rmtoll CNT1         PD            LL_RTC_EnableCounter1
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_EnableCounter1(RTC_TypeDef *RTCx)
{
    MODIFY_REG(RTCx->CNT1, RTC_CNT1_PD, RTC_CNT1_PD_ON);
}

/**
 * @brief  Disable Counter1
 * @rmtoll CNT1         PD            LL_RTC_DisableCounter1
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_DisableCounter1(RTC_TypeDef *RTCx)
{
    MODIFY_REG(RTCx->CNT1, RTC_CNT1_PD, RTC_CNT1_PD_OFF);
}

/**
 * @brief  Check if Counter1 is enabled
 * @rmtoll CNT1         PD            LL_RTC_IsEnabledCounter1
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsEnabledCounter1(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->CNT1, RTC_CNT1_PD) == RTC_CNT1_PD_OFF) ? 1UL : 0UL);
}

/**
 * @brief  Set Counter1 debug mode
 * @rmtoll CNT1         DBGSTB        LL_RTC_SetCounter1DebugMode
 * @param  RTCx RTC Instance
 * @param  DebugMode This parameter can be one of the following values:
 *         @arg @ref LL_RTC_DEBUGMODE_CONTINUE
 *         @arg @ref LL_RTC_DEBUGMODE_PAUSE
 * @retval None
 */
__STATIC_INLINE void LL_RTC_SetCounter1DebugMode(RTC_TypeDef *RTCx, uint32_t DebugMode)
{
    MODIFY_REG(RTCx->CNT1, RTC_CNT1_DBGSTB, DebugMode << RTC_CNT1_DBGSTB_POS);
}

/**
 * @brief  Get Counter1 debug mode
 * @rmtoll CNT1         DBGSTB        LL_RTC_GetCounter1DebugMode
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_DEBUGMODE_CONTINUE
 *         @arg @ref LL_RTC_DEBUGMODE_PAUSE
 */
__STATIC_INLINE uint32_t LL_RTC_GetCounter1DebugMode(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->CNT1, RTC_CNT1_DBGSTB));
}

/**
 * @brief  Set Counter2 target value
 * @rmtoll CNT2         CNT           LL_RTC_SetCounter2TargetValue
 * @param  RTCx RTC Instance
 * @param  Value between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_RTC_SetCounter2TargetValue(RTC_TypeDef *RTCx, uint32_t Value)
{
    MODIFY_REG(RTCx->CNT2, RTC_CNT2_CNT, Value);
}

/**
 * @brief  Get Counter2 target value
 * @rmtoll CNT2         CNT           LL_RTC_GetCounter2TargetValue
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF
 */
__STATIC_INLINE uint32_t LL_RTC_GetCounter2TargetValue(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->CNT2, RTC_CNT2_CNT));
}

/**
 * @brief  Enable Counter2
 * @rmtoll CNT2         PD            LL_RTC_EnableCounter2
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_EnableCounter2(RTC_TypeDef *RTCx)
{
    MODIFY_REG(RTCx->CNT2, RTC_CNT2_PD, RTC_CNT2_PD_ON);
}

/**
 * @brief  Disable Counter2
 * @rmtoll CNT2         PD            LL_RTC_DisableCounter2
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_DisableCounter2(RTC_TypeDef *RTCx)
{
    MODIFY_REG(RTCx->CNT2, RTC_CNT2_PD, RTC_CNT2_PD_OFF);
}

/**
 * @brief  Check if Counter2 is enabled
 * @rmtoll CNT2         PD            LL_RTC_IsEnabledCounter2
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsEnabledCounter2(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->CNT2, RTC_CNT2_PD) == RTC_CNT2_PD_OFF) ? 1UL : 0UL);
}

/**
 * @brief  Set Counter2 clock source
 * @rmtoll CNT2         CLK_SEL       LL_RTC_SetCounter2ClockSource
 * @param  RTCx RTC Instance
 * @param  ClockSource This parameter can be one of the following values:
 *         @arg @ref LL_RTC_CNT2CLOCKSOURCE_RCL
 *         @arg @ref LL_RTC_CNT2CLOCKSOURCE_LOSC
 * @retval None
 */
__STATIC_INLINE void LL_RTC_SetCounter2ClockSource(RTC_TypeDef *RTCx, uint32_t ClockSource)
{
    MODIFY_REG(RTCx->CNT2, RTC_CNT2_CLK_SEL, ClockSource << RTC_CNT2_CLK_SEL_POS);
}
/**
 * @brief  Get Counter2 clock source
 * @rmtoll CNT2         CLK_SEL       LL_RTC_GetCounter2ClockSource
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_CNT2CLOCKSOURCE_RCL
 *         @arg @ref LL_RTC_CNT2CLOCKSOURCE_LOSC
 */
__STATIC_INLINE uint32_t LL_RTC_GetCounter2ClockSource(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->CNT2, RTC_CNT2_CLK_SEL));
}

/**
 * @brief  Set Counter2 debug mode
 * @rmtoll CNT2         DBGSTB        LL_RTC_SetCounter2DebugMode
 * @param  RTCx RTC Instance
 * @param  DebugMode This parameter can be one of the following values:
 *         @arg @ref LL_RTC_DEBUGMODE_CONTINUE
 *         @arg @ref LL_RTC_DEBUGMODE_PAUSE
 * @retval None
 */
__STATIC_INLINE void LL_RTC_SetCounter2DebugMode(RTC_TypeDef *RTCx, uint32_t DebugMode)
{
    MODIFY_REG(RTCx->CNT2, RTC_CNT2_DBGSTB, DebugMode << RTC_CNT2_DBGSTB_POS);
}

/**
 * @brief  Get Counter2 debug mode
 * @rmtoll CNT2         DBGSTB        LL_RTC_GetCounter2DebugMode
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_DEBUGMODE_CONTINUE
 *         @arg @ref LL_RTC_DEBUGMODE_PAUSE
 */
__STATIC_INLINE uint32_t LL_RTC_GetCounter2DebugMode(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->CNT2, RTC_CNT2_DBGSTB));
}

/**
 * @}
 */ /* end of group RTC_LL_EF_Counter **/

/** @defgroup RTC_LL_EF_Alarm Alarm functions
 * @{
 */

/**
 * @brief  Set Alarm Second in BCD format
 * @rmtoll SCA          SCA           LL_RTC_ALARM_SetSecond
 * @param  RTCx RTC Instance
 * @param  Seconds Value between Min_Data=0x00 and Max_Data=0x59
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ALARM_SetSecond(RTC_TypeDef *RTCx, uint32_t Seconds)
{
    MODIFY_REG(RTCx->SCA, RTC_SCA_SCA, Seconds);
}

/**
 * @brief  Get Alarm Second in BCD format
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Minutes from BCD to Binary format
 * @rmtoll SCA          SCA           LL_RTC_ALARM_GetSecond
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x59
 */
__STATIC_INLINE uint32_t LL_RTC_ALARM_GetSecond(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->SCA, RTC_SCA_SCA));
}

/**
 * @brief  Set Alarm Minute in BCD format
 * @rmtoll MNA          MNA           LL_RTC_ALARM_SetMinute
 * @param  RTCx RTC Instance
 * @param  Minutes Value between Min_Data=0x00 and Max_Data=0x59
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ALARM_SetMinute(RTC_TypeDef *RTCx, uint32_t Minutes)
{
    MODIFY_REG(RTCx->MNA, RTC_MNA_MNA, Minutes);
}

/**
 * @brief  Get Alarm Minute in BCD format
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Minutes from BCD to Binary format
 * @rmtoll MNA          MNA           LL_RTC_ALARM_GetMinute
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x59
 */
__STATIC_INLINE uint32_t LL_RTC_ALARM_GetMinute(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->MNA, RTC_MNA_MNA));
}

/**
 * @brief  Set Alarm Hour in BCD format
 * @rmtoll HRA          HRA           LL_RTC_ALARM_SetHour
 * @param  RTCx RTC Instance
 * @param  Hours Value between Min_Data=0x00 and Max_Data=0x23
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ALARM_SetHour(RTC_TypeDef *RTCx, uint32_t Hours)
{
    MODIFY_REG(RTCx->HRA, RTC_HRA_HRA, Hours);
}

/**
 * @brief  Get Alarm Hour in BCD format
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Minutes from BCD to Binary format
 * @rmtoll HRA          HRA           LL_RTC_ALARM_GetHour
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x23
 */
__STATIC_INLINE uint32_t LL_RTC_ALARM_GetHour(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->HRA, RTC_HRA_HRA));
}
/**
 * @}
 */ /* end of group RTC_LL_EF_Alarm **/

/** @defgroup RTC_LL_EF_Timestamp Timestamp
 * @{
 */

/**
 * @brief  Enable Soft Freezed Timestamp
 * @note   Bits is write-protected. @ref LL_RTC_DisableConfigWriteProtection function should be called before.
 * @rmtoll CTRL           STF_TRIG           LL_RTC_TS_Enable
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_TS_EnableSoftFreezeTimestamp(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->CTRL, RTC_CTRL_STF_TRIG);
}

/**
 * @brief  Get Soft Freezed Timestamp Hours in BCD format.
 * @note   Only available when get 1 by @ref LL_RTC_IsActiveFlag_STF and get 0 by @ref LL_RTC_IsActiveFlag_TAMPER2P
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Hours from BCD to Binary format
 * @rmtoll TAMPER2P1_TIME1       HOUR           LL_RTC_TS_GetHour
 * @param  RTCx RTC Instance
 * @retval Value between between Min_Data=0x00 and Max_Data=0x23
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetHour(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->TAMPER2P1_TIME1, RTC_TAMPER2P1_TIME1_HOUR) >> RTC_TAMPER2P1_TIME1_HOUR_POS);
}

/**
 * @brief  Get Soft Freezed Timestamp Minutes in BCD format
 * @note   Only available when get 1 by @ref LL_RTC_IsActiveFlag_STF and get 0 by @ref LL_RTC_IsActiveFlag_TAMPER2P
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Minutes from BCD to Binary format
 * @rmtoll TAMPER2P1_TIME0       MIN            LL_RTC_TS_GetMinute
 * @param  RTCx RTC Instance.
 * @retval Element_description
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetMinute(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->TAMPER2P1_TIME0, RTC_TAMPER2P1_TIME0_MIN) >> RTC_TAMPER2P1_TIME0_MIN_POS);
}


/**
 * @brief  Get Soft Freezed Timestamp Seconds in BCD format
 * @note   Only available when get 1 by @ref LL_RTC_IsActiveFlag_STF and get 0 by @ref LL_RTC_IsActiveFlag_TAMPER2P
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Seconds from BCD to Binary format
 * @rmtoll TAMPER2P1_TIME0       SEC            LL_RTC_TS_GetSecond
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x59
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetSecond(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->TAMPER2P1_TIME0, RTC_TAMPER2P1_TIME0_SEC) >> RTC_TAMPER2P1_TIME0_SEC_POS);
}

/**
 * @brief  Get Soft Freezed Timestamp subseconds value
 * @note   Only available when get 1 by @ref LL_RTC_IsActiveFlag_STF and get 0 by @ref LL_RTC_IsActiveFlag_TAMPER2N
 * @rmtoll TAMPER2N1_TIME2        MT            LL_RTC_TS_GetSubSecond
 *         TAMPER2N1_TIME2        YR            LL_RTC_TS_GetSubSecond
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFF. (1 for 1/32768 s)
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetSubSecond(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->TAMPER2N1_TIME2, RTC_TAMPER2N1_TIME2_MT|RTC_TAMPER2N1_TIME2_YR));
}

/**
 * @brief  Get Soft Freezed Timestamp Month in BCD format
 * @note   Only available when get 1 by @ref LL_RTC_IsActiveFlag_STF and get 0 by @ref LL_RTC_IsActiveFlag_TAMPER2P
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Month from BCD to Binary format
 * @rmtoll TAMPER2P1_TIME2         MT            LL_RTC_TS_GetMonth
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_MONTH_JANUARY
 *         @arg @ref LL_RTC_MONTH_FEBRUARY
 *         @arg @ref LL_RTC_MONTH_MARCH
 *         @arg @ref LL_RTC_MONTH_APRIL
 *         @arg @ref LL_RTC_MONTH_MAY
 *         @arg @ref LL_RTC_MONTH_JUNE
 *         @arg @ref LL_RTC_MONTH_JULY
 *         @arg @ref LL_RTC_MONTH_AUGUST
 *         @arg @ref LL_RTC_MONTH_SEPTEMBER
 *         @arg @ref LL_RTC_MONTH_OCTOBER
 *         @arg @ref LL_RTC_MONTH_NOVEMBER
 *         @arg @ref LL_RTC_MONTH_DECEMBER
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetMonth(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->TAMPER2P1_TIME2, RTC_TAMPER2P1_TIME2_MT) >> RTC_TAMPER2P1_TIME2_MT_POS);
}

/**
 * @brief  Get Soft Freezed Timestamp Day in BCD format
 * @note   Only available when get 1 by @ref LL_RTC_IsActiveFlag_STF and get 0 by @ref LL_RTC_IsActiveFlag_TAMPER2P
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Day from BCD to Binary format
 * @rmtoll TAMPER2P1_TIME1         DAY            LL_RTC_TS_GetDay
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x01 and Max_Data=0x31
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetDay(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->TAMPER2P1_TIME1, RTC_TAMPER2P1_TIME1_DAY) >> RTC_TAMPER2P1_TIME1_DAY_POS);
}

/**
 * @brief  Get Soft Freezed Timestamp Year in BCD format
 * @note   Only available when get 1 by @ref LL_RTC_IsActiveFlag_STF and get 0 by @ref LL_RTC_IsActiveFlag_TAMPER2P
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Year from BCD to Binary format
 * @rmtoll TAMPER2N1_TIME2         YR            LL_RTC_TS_GetYear
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x99
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetYear(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->TAMPER2N1_TIME2, RTC_TAMPER2N1_TIME2_YR) >> RTC_TAMPER2N1_TIME2_YR_POS);
}

/**
 * @brief  Get Soft Freezed Timestamp date ( Day, Month and Year) in BCD format
 * @note   Only available when get 1 by @ref LL_RTC_IsActiveFlag_STF and get 0 by @ref LL_RTC_IsActiveFlag_TAMPER2P
 * @note helper macros  __LL_RTC_GET_YEAR, __LL_RTC_GET_MONTH, and __LL_RTC_GET_DAY
 * are available to get independently each parameter.
 * @rmtoll TAMPER2P1_TIME1         DAY            LL_RTC_TS_GetDay
 *         TAMPER2P1_TIME2         MT             LL_RTC_TS_GetMonth
 *         TAMPER2N1_TIME2         YR             LL_RTC_TS_GetYear
 * @param  RTCx RTC Instance
 * @retval Combination of  Day, Month and Year (Format: 0x00DDMMYY).
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetDate(RTC_TypeDef *RTCx)
{
    return (uint32_t)(((READ_BIT(RTCx->TAMPER2P1_TIME1, RTC_TAMPER2P1_TIME1_DAY) >> RTC_TAMPER2P1_TIME1_DAY_POS)<< RTC_OFFSET_DAY)\
                    ||((READ_BIT(RTCx->TAMPER2P1_TIME2, RTC_TAMPER2P1_TIME2_MT) >> RTC_TAMPER2P1_TIME2_MT_POS)<< RTC_OFFSET_MONTH)\
                    ||((READ_BIT(RTCx->TAMPER2N1_TIME2, RTC_TAMPER2N1_TIME2_YR) >> RTC_TAMPER2N1_TIME2_YR_POS)));
}


/**
 * @brief  Get Soft Freezed Timestamp time (hour, minute and second) in BCD format
 * @note   Only available when get 1 by @ref LL_RTC_IsActiveFlag_STF and get 0 by @ref LL_RTC_IsActiveFlag_TAMPER2P
 * @note helper macros __LL_RTC_GET_HOUR, __LL_RTC_GET_MINUTE and __LL_RTC_GET_SECOND
 * are available to get independently each parameter.
 * @rmtoll TAMPER2P1_TIME0       SEC            LL_RTC_TS_GetSecond\n
 *         TAMPER2P1_TIME1       HOUR           LL_RTC_TS_GetHour\n
 *         TAMPER2N1_TIME0       MIN            LL_RTC_TS_GetMinute
 * @param  RTCx RTC Instance
 * @retval Combination of hours, minutes and seconds.(Format: 0x00HHMMSS)
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetTime(RTC_TypeDef *RTCx)
{
    return (uint32_t)(((READ_BIT(RTCx->TAMPER2P1_TIME1, RTC_TAMPER2P1_TIME1_HOUR) >> RTC_TAMPER2P1_TIME1_HOUR_POS) << RTC_OFFSET_HOUR)\
                    ||((READ_BIT(RTCx->TAMPER2N1_TIME0, RTC_TAMPER2N1_TIME0_MIN) >> RTC_TAMPER2N1_TIME0_MIN_POS) << RTC_OFFSET_MINUTE)\
                    ||((READ_BIT(RTCx->TAMPER2P1_TIME0, RTC_TAMPER2P1_TIME0_SEC) >> RTC_TAMPER2P1_TIME0_SEC_POS)));
}

/**
 * @brief  Get the Tamper Timestamp Seconds in BCD format
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Seconds from BCD to Binary format
 * @rmtoll TAMPER1N0_TIME0         SEC            LL_RTC_TS_GetTamperSecond
 *         TAMPER1P0_TIME0         SEC            LL_RTC_TS_GetTamperSecond
 *         TAMPER1N1_TIME0         SEC            LL_RTC_TS_GetTamperSecond
 *         TAMPER1P1_TIME0         SEC            LL_RTC_TS_GetTamperSecond
 *         TAMPER2N0_TIME0         SEC            LL_RTC_TS_GetTamperSecond
 *         TAMPER2P0_TIME0         SEC            LL_RTC_TS_GetTamperSecond
 *         TAMPER2N1_TIME0         SEC            LL_RTC_TS_GetTamperSecond
 *         TAMPER2P1_TIME0         SEC            LL_RTC_TS_GetTamperSecond
 * @param  RTCx RTC Instance.
 * @param  TamperType This parameter can be one of the following values:
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1N0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1P0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1N1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1P1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2N0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2P0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2N1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2P1
 * @retval  Value between Min_Data=0x00 and Max_Data=0x59
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetTamperSecond(RTC_TypeDef *RTCx, uint32_t TamperType)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&RTCx->TAMPER1N0_TIME0) + (TamperType * 12U)));
    return (uint32_t)(READ_BIT(*pReg, RTC_TAMPER1N0_TIME0_SEC) >> RTC_TAMPER1N0_TIME0_SEC_POS);
}

/**
 * @brief  Get the Tamper Timestamp Minutes in BCD format
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Minutes from BCD to Binary format
 * @rmtoll TAMPER1N0_TIME0         MIN            LL_RTC_TS_GetTamperMinute
 *         TAMPER1P0_TIME0         MIN            LL_RTC_TS_GetTamperMinute
 *         TAMPER1N1_TIME0         MIN            LL_RTC_TS_GetTamperMinute
 *         TAMPER1P1_TIME0         MIN            LL_RTC_TS_GetTamperMinute
 *         TAMPER2N0_TIME0         MIN            LL_RTC_TS_GetTamperMinute
 *         TAMPER2P0_TIME0         MIN            LL_RTC_TS_GetTamperMinute
 *         TAMPER2N1_TIME0         MIN            LL_RTC_TS_GetTamperMinute
 *         TAMPER2P1_TIME0         MIN            LL_RTC_TS_GetTamperMinute
 * @param  RTCx RTC Instance.
 * @param  TamperType This parameter can be one of the following values:
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1N0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1P0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1N1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1P1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2N0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2P0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2N1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2P1
 * @retval  Value between Min_Data=0x00 and Max_Data=0x59
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetTamperMinute(RTC_TypeDef *RTCx, uint32_t TamperType)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&RTCx->TAMPER1N0_TIME0) + (TamperType * 12U)));
    return (uint32_t)(READ_BIT(*pReg, RTC_TAMPER1N0_TIME0_MIN) >> RTC_TAMPER1N0_TIME0_MIN_POS);
}

/**
 * @brief  Get the Tamper Timestamp Hours in BCD format
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Hours from BCD to Binary format
 * @rmtoll TAMPER1N0_TIME1         HOUR            LL_RTC_TS_GetTamperHour
 *         TAMPER1P0_TIME1         HOUR            LL_RTC_TS_GetTamperHour
 *         TAMPER1N1_TIME1         HOUR            LL_RTC_TS_GetTamperHour
 *         TAMPER1P1_TIME1         HOUR            LL_RTC_TS_GetTamperHour
 *         TAMPER2N0_TIME1         HOUR            LL_RTC_TS_GetTamperHour
 *         TAMPER2P0_TIME1         HOUR            LL_RTC_TS_GetTamperHour
 *         TAMPER2N1_TIME1         HOUR            LL_RTC_TS_GetTamperHour
 *         TAMPER2P1_TIME1         HOUR            LL_RTC_TS_GetTamperHour
 * @param  RTCx RTC Instance.
 * @param  TamperType This parameter can be one of the following values:
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1N0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1P0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1N1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1P1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2N0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2P0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2N1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2P1
 * @retval Value between  Min_Data=0x00 and Max_Data=0x23
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetTamperHour(RTC_TypeDef *RTCx, uint32_t TamperType)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&RTCx->TAMPER1N0_TIME1) + (TamperType * 12U)));
    return (uint32_t)(READ_BIT(*pReg, RTC_TAMPER1N0_TIME1_HOUR) >> RTC_TAMPER1N0_TIME1_HOUR_POS);
}

/**
 * @brief  Get the Tamper Timestamp Days in BCD format
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Days from BCD to Binary format
 * @rmtoll TAMPER1N0_TIME1         DAY            LL_RTC_TS_GetTamperDay
 *         TAMPER1P0_TIME1         DAY            LL_RTC_TS_GetTamperDay
 *         TAMPER1N1_TIME1         DAY            LL_RTC_TS_GetTamperDay
 *         TAMPER1P1_TIME1         DAY            LL_RTC_TS_GetTamperDay
 *         TAMPER2N0_TIME1         DAY            LL_RTC_TS_GetTamperDay
 *         TAMPER2P0_TIME1         DAY            LL_RTC_TS_GetTamperDay
 *         TAMPER2N1_TIME1         DAY            LL_RTC_TS_GetTamperDay
 *         TAMPER2P1_TIME1         DAY            LL_RTC_TS_GetTamperDay
 * @param  RTCx RTC Instance.
 * @param  TamperType This parameter can be one of the following values:
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1N0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1P0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1N1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1P1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2N0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2P0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2N1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2P1
 * @retval Value between Min_Data=0x01 and Max_Data=0x31
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetTamperDay(RTC_TypeDef *RTCx, uint32_t TamperType)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&RTCx->TAMPER1N0_TIME1) + (TamperType * 12U)));
    return (uint32_t)(READ_BIT(*pReg, RTC_TAMPER1N0_TIME1_DAY) >> RTC_TAMPER1N0_TIME1_DAY_POS);
}

/**
 * @brief  Get the Tamper Timestamp Months in BCD format
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Months from BCD to Binary format
 * @rmtoll TAMPER1N0_TIME2         MT            LL_RTC_TS_GetTamperMonth
 *         TAMPER1P0_TIME2         MT            LL_RTC_TS_GetTamperMonth
 *         TAMPER1N1_TIME2         MT            LL_RTC_TS_GetTamperMonth
 *         TAMPER1P1_TIME2         MT            LL_RTC_TS_GetTamperMonth
 *         TAMPER2N0_TIME2         MT            LL_RTC_TS_GetTamperMonth
 *         TAMPER2P0_TIME2         MT            LL_RTC_TS_GetTamperMonth
 *         TAMPER2N1_TIME2         MT            LL_RTC_TS_GetTamperMonth
 *         TAMPER2P1_TIME2         MT            LL_RTC_TS_GetTamperMonth
 * @param  RTCx RTC Instance.
 * @param  TamperType This parameter can be one of the following values:
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1N0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1P0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1N1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1P1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2N0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2P0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2N1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2P1
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_MONTH_JANUARY
 *         @arg @ref LL_RTC_MONTH_FEBRUARY
 *         @arg @ref LL_RTC_MONTH_MARCH
 *         @arg @ref LL_RTC_MONTH_APRIL
 *         @arg @ref LL_RTC_MONTH_MAY
 *         @arg @ref LL_RTC_MONTH_JUNE
 *         @arg @ref LL_RTC_MONTH_JULY
 *         @arg @ref LL_RTC_MONTH_AUGUST
 *         @arg @ref LL_RTC_MONTH_SEPTEMBER
 *         @arg @ref LL_RTC_MONTH_OCTOBER
 *         @arg @ref LL_RTC_MONTH_NOVEMBER
 *         @arg @ref LL_RTC_MONTH_DECEMBER
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetTamperMonth(RTC_TypeDef *RTCx, uint32_t TamperType)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&RTCx->TAMPER1N0_TIME2) + (TamperType * 12U)));
    return (uint32_t)(READ_BIT(*pReg, RTC_TAMPER1N0_TIME2_MT) >> RTC_TAMPER1N0_TIME2_MT_POS);
}

/**
 * @brief  Get the Tamper Timestamp Years in BCD format
 * @note helper macro __LL_RTC_CONVERT_BCD2BIN is available to convert Years from BCD to Binary format
 * @rmtoll TAMPER1N0_TIME2         YR            LL_RTC_TS_GetTamperYear
 *         TAMPER1P0_TIME2         YR            LL_RTC_TS_GetTamperYear
 *         TAMPER1N1_TIME2         YR            LL_RTC_TS_GetTamperYear
 *         TAMPER1P1_TIME2         YR            LL_RTC_TS_GetTamperYear
 *         TAMPER2N0_TIME2         YR            LL_RTC_TS_GetTamperYear
 *         TAMPER2P0_TIME2         YR            LL_RTC_TS_GetTamperYear
 *         TAMPER2N1_TIME2         YR            LL_RTC_TS_GetTamperYear
 *         TAMPER2P1_TIME2         YR            LL_RTC_TS_GetTamperYear
 * @param  RTCx RTC Instance.
 * @param  TamperType This parameter can be one of the following values:
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1N0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1P0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1N1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER1P1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2N0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2P0
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2N1
 *          @arg @ref LL_RTC_TAMPERTYPE_TAMPER2P1
 * @retval Value between Min_Data=0x00 and Max_Data=0x99
 */
__STATIC_INLINE uint32_t LL_RTC_TS_GetTamperYear(RTC_TypeDef *RTCx, uint32_t TamperType)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&RTCx->TAMPER1N0_TIME2) + (TamperType * 12U)));
    return (uint32_t)(READ_BIT(*pReg, RTC_TAMPER1N0_TIME2_YR) >> RTC_TAMPER1N0_TIME2_YR_POS);
}

/**
 * @}
 */ /* end of group RTC_LL_EF_Timestamp **/

/** @defgroup RTC_LL_EF_Calibration Calibration
 * @{
 */
/**
 * @brief  Start RTC manual temperature measurement Once
 * @note   Only available when setted LL_RTC_TEMPCOMPMODE_USER0 as TempCompMode by @ref LL_RTC_CAL_SetTempCompMode.
 * @rmtoll TPS_START   TPS_START   LL_RTC_CAL_ManualTempMeasureStart
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_EnableManualTempMeasureStart(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->TPS_START, RTC_TPS_START_TPS_START);
}

/**
 * @brief  Get RTC manual temperature measurement value
 * @note   Only available when setted LL_RTC_TEMPCOMPMODE_USER0 as TempCompMode by @ref LL_RTC_CAL_SetTempCompMode.
 * @note   Only available when started manual temperature measurement by @ref LL_RTC_CAL_EnableManualTempMeasureStart
 *   and get the correct flag(get each flag as 1 frist and than as 0) by @ref LL_MADC_IsActiveFlag_TPSBUSY and @ref LL_RTC_IsActiveFlag_CALBUSY.
 *   refer to the reference manual for details.
 * @note   Refer to the reference manual for details.
 * @rmtoll TEMP_CAL   TEMP_CAL   LL_RTC_CAL_GetManualTempMeasureValue
 * @param  RTCx RTC Instance
 * @retval  Value between Min_Data=0x00 and Max_Data=0xFFF (12bit two's complement)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetManualTempMeasureValue(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->TEMP_CAL, RTC_TEMP_CAL_TEMP_CAL));
}

/**
 * @brief  Set RTC temperature measurement value 1
 * @note   Only available when setted LL_RTC_TEMPCOMPMODE_USER0 or LL_RTC_TEMPCOMPMODE_USER1 as TempCompMode by @ref LL_RTC_CAL_SetTempCompMode.
 * @note   Bit field range depends on whether quartic temperature compensation is used (16bit if yes, 10bit if no) set by @ref LL_RTC_CAL_SetCurveType
 * @note   Refer to the reference manual for details.
 * @rmtoll TEMP   TEMP   LL_RTC_CAL_SetTempMeasureValue1
 * @param  RTCx RTC Instance
 * @param  TempValue Between Min_Data=0x00 and Max_Data=0xFFFF or Max_Data=0x3FF (10bit or 16bit two's complement or start enable bit)
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetTempMeasureValue1(RTC_TypeDef *RTCx, uint32_t TempValue)
{
    MODIFY_REG(RTCx->TEMP, RTC_TEMP_TEMP, TempValue);
}

/**
 * @brief  Get RTC temperature measurement value 1
 * @note   Bit field range depends on whether quartic temperature compensation is used (16bit if yes, 10bit if no) set by @ref LL_RTC_CAL_SetCurveType
 * @note   Refer to the reference manual for details.
 * @rmtoll TEMP   TEMP   LL_RTC_CAL_GetTempMeasureValue1
 * @param  RTCx RTC Instance
 * @retval Between Min_Data=0x00 and Max_Data=0xFFFF or Max_Data=0x3FF (10bit or 16bit two's complement)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetTempMeasureValue1(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->TEMP, RTC_TEMP_TEMP));
}

/**
 * @brief  Get current chip temperature measurement value 2
 * @rmtoll TEMP2   TEMP2   LL_RTC_CAL_GetTempMeasureValue2
 * @param  RTCx RTC Instance
 * @retval
 * @retval  Value between Min_Data=0x00 and Max_Data=0xFFF (12bit two's complement)
 *      where bit11 is the sign bit and LSB = 0.0625°C.Range: -128°C ~ +127.75°C
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetTempMeasureValue2(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->TEMP2, RTC_TEMP2_TEMP2));
}

/**
 * @brief  Set RTC temperature compensation period
 * @note   Bits is write-protected. @ref LL_RTC_DisableConfigWriteProtection function should be called before.
 * @rmtoll CTRL   TCP   LL_RTC_CAL_SetTempCompPeriod
 * @param  RTCx RTC Instance
 * @param  TempCompPeriod Temperature compensation period value
 *         This parameter can be one of the following values:
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_2S
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_10S
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_20S
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_30S
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_1MIN
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_2MIN
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_5MIN
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_10MIN
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetTempCompPeriod(RTC_TypeDef *RTCx, uint32_t TempCompPeriod)
{
    MODIFY_REG(RTCx->CTRL, RTC_CTRL_TCP, (TempCompPeriod << RTC_CTRL_TCP_POS));
}

/**
 * @brief  Get RTC temperature compensation period
 * @rmtoll CTRL   TCP   LL_RTC_CAL_GetTempCompPeriod
 * @param  RTCx RTC Instance
 * @retval  Returned value can be one of the following values:
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_2S
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_10S
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_20S
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_30S
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_1MIN
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_2MIN
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_5MIN
 *          @arg @ref LL_RTC_TEMPCOMPPERIOD_10MIN
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetTempCompPeriod(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->CTRL, RTC_CTRL_TCP)) >> RTC_CTRL_TCP_POS);
}

/**
 * @brief  Set RTC temperature compensation mode
 * @note   Bits is write-protected. @ref LL_RTC_DisableConfigWriteProtection function should be called before.
 * @rmtoll CTRL   TSE   LL_RTC_CAL_SetTempCompMode
 * @param  RTCx RTC Instance
 * @param  TempCompMode Temperature compensation mode, can be one of the following values:
 *         @arg @ref LL_RTC_TEMPCOMPMODE_DIS
 *         @arg @ref LL_RTC_TEMPCOMPMODE_AUTO
 *         @arg @ref LL_RTC_TEMPCOMPMODE_USER0
 *         @arg @ref LL_RTC_TEMPCOMPMODE_USER1
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetTempCompMode(RTC_TypeDef *RTCx, uint32_t TempCompMode)
{
    MODIFY_REG(RTCx->CTRL, RTC_CTRL_TSE, TempCompMode);
}

/**
 * @brief  Get RTC temperature compensation mode
 * @rmtoll CTRL   TSE   LL_RTC_CAL_GetTempCompMode
 * @param  RTCx RTC Instance
 * @retval Return value can be one of the following values:
 *         @arg @ref LL_RTC_TEMPCOMPMODE_DIS
 *         @arg @ref LL_RTC_TEMPCOMPMODE_AUTO
 *         @arg @ref LL_RTC_TEMPCOMPMODE_USER0
 *         @arg @ref LL_RTC_TEMPCOMPMODE_USER1
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetTempCompMode(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->CTRL, RTC_CTRL_TSE));
}

/**
 * @brief  Set RTC automatic compensation mode temperature offset
 * @rmtoll TEMPOS   TEMPOS   LL_RTC_CAL_SetTempOffset
 * @param  RTCx RTC Instance
 * @param  TempOffset  Value between Min_Data=0x0 and Max_Data=0x3F (6bit two's complement)
 *  Temperature Offset value(6-bit two's complement value, Range: -8°C to +8°C.
 *  The meaning varies based on TempCompMode; refer to the reference manual for details.
 * @retval
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetTempOffset(RTC_TypeDef *RTCx, uint32_t TempOffset)
{
    MODIFY_REG(RTCx->TEMPOS, RTC_TEMPOS_TEMPOS, TempOffset);
}

/**
 * @brief  Get RTC automatic compensation mode temperature offset
 * @rmtoll TEMPOS   TEMPOS   LL_RTC_CAL_GetTempOffset
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x0 and Max_Data=0x3F (6bit two's complement)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetTempOffset(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->TEMPOS, RTC_TEMPOS_TEMPOS));
}

/**
 * @brief  Get RTC automatic temperature compensation ADC measurement value
 * @rmtoll TPSIN   TPSIN   LL_RTC_CAL_GetTempADCValue
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x0 and Max_Data=0xFFF
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetTempADCValue(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->TPSIN, RTC_TPSIN_TPSIN));
}

/**
 * @brief  Get RTC compensation calculation result value DOTAT
 * @rmtoll DOTAT   DOTAT   LL_RTC_CAL_GetCompValue
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x0 and Max_Data=0xFFFF (16bit two's complement)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetCompValue(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->DOTAT, RTC_DOTAT_DOTAT));
}

/**
 * @brief  Get PLL compensation value
 * @rmtoll FPTR   FPTR   LL_RTC_CAL_GetFreqCompValuePLL
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x0 and Max_Data=0x1FFFF (17bit two's complement)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetFreqCompValuePLL(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->FPTR, RTC_FPTR_FPTR));
}

/**
 * @brief  Get RTC compensation value per 1 second
 * @rmtoll FDTR1S   FDTR1S   LL_RTC_CAL_GetFreqCompValue1S
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x0 and Max_Data=0x3F (6bit two's complement)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetFreqCompValue1S(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->FDTR1S, RTC_FDTR1S_FDTR1S));
}

/**
 * @brief  Get RTC compensation value per 30 seconds
 * @rmtoll FDTR30S   FDTR30S   LL_RTC_CAL_GetFreqCompValue30S
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x0 and Max_Data=0x7F (7bit two's complement)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetFreqCompValue30S(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->FDTR30S, RTC_FDTR30S_FDTR30S));
}

/**
 * @brief  Get RTC compensation value per 120 seconds
 * @rmtoll FDTR120S   FDTR120S   LL_RTC_CAL_GetFreqCompValue120S
 * @param  RTCx RTC Instance
 * @retval Value between Min_Data=0x0 and Max_Data=0x1FF (9bit two's complement)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetFreqCompValue120S(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->FDTR120S, RTC_FDTR120S_FDTR120S));
}

/** @defgroup RTC_LL_EF_CALCurve temperature compensation curves
 * @{
 */

/**
 * @brief  Enable the write protection for RTC temperature compensation curve.
 * @rmtoll PS   PS   LL_RTC_CAL_EnableCurveWriteProtection
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_EnableCurveWriteProtection(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->PS, RTC_PS_PS_DISWT);
}

/**
 * @brief  Disable the write protection for RTC temperature compensation curve.
 * @rmtoll PS   PS   LL_RTC_CAL_DisableCurveWriteProtection
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_DisableCurveWriteProtection(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->PS, RTC_PS_PS_ENWT);
}

/**
 * @brief  Set initial frequency deviation DOTA0
 * @note   Bits is write-protected. @ref LL_RTC_CAL_DisableCurveWriteProtection function should be called before.
 * @rmtoll DOTA0   DOTA0   LL_RTC_CAL_SetCurveDOTA0
 * @param  RTCx RTC Instance
 * @param  DOTA0 between Min_Data=0x0 and Max_Data=0xFFFF (16bit two's complement)
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetCurveDOTA0(RTC_TypeDef *RTCx, uint32_t DOTA0)
{
    MODIFY_REG(RTCx->DOTA0, RTC_DOTA0_DOTA0, DOTA0);
}

/**
 * @brief  Get initial frequency deviation
 * @rmtoll DOTA0   DOTA0   LL_RTC_CAL_GetCurveDOTA0
 * @param  RTCx RTC Instance
 * @retval Between Min_Data=0x0 and Max_Data=0xFFFF (16bit two's complement)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetCurveDOTA0(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->DOTA0, RTC_DOTA0_DOTA0));
}

/**
 * @brief  Set temperature compensation curve type
 * @note   Bits is write-protected. @ref LL_RTC_CAL_DisableCurveWriteProtection function should be called before.
 * @rmtoll MODE1   CURVE_SEL   LL_RTC_CAL_SetCurveType
 * @param  RTCx RTC Instance
 * @param  CurveType This parameter can be one of the following values:
 *         @arg @ref LL_RTC_CURVETYPE_QUADRATIC
 *         @arg @ref LL_RTC_CURVETYPE_QUARTIC
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetCurveType(RTC_TypeDef *RTCx, uint32_t CurveType)
{
    MODIFY_REG(RTCx->MODE1, RTC_MODE1_CURVE_SEL, CurveType);
}

/**
 * @brief  Get temperature compensation curve type
 * @rmtoll MODE1   CURVE_SEL   LL_RTC_CAL_GetCurveType
 * @param  RTCx RTC Instance
 * @retval Curve type value
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetCurveType(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->MODE1, RTC_MODE1_CURVE_SEL));
}

/**
 * @brief  Set quadratic curve vertex temperature XT0(unsigned)
 * @note   Bits is write-protected. @ref LL_RTC_CAL_DisableCurveWriteProtection function should be called before.
 * @rmtoll XT0   XT0   LL_RTC_CAL_SetQuadCurveVertexXT0
 * @param  RTCx RTC Instance
 * @param  XT0 Vertex temperature value
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetQuadCurveVertexXT0(RTC_TypeDef *RTCx, uint32_t XT0)
{
    MODIFY_REG(RTCx->XT0, RTC_XT0_XT0, XT0);
}

/**
 * @brief  Get quadratic curve vertex temperature XT0
 * @rmtoll XT0   XT0   LL_RTC_CAL_GetQuadCurveVertexXT0
 * @param  RTCx RTC Instance
 * @retval Vertex temperature value
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetQuadCurveVertexXT0(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->XT0, RTC_XT0_XT0));
}

/**
 * @brief  Set quadratic curve low temperature coefficient ALPHAL(Coefficient of x^2)
 * @note   Bits is write-protected. @ref LL_RTC_CAL_DisableCurveWriteProtection function should be called before.
 * @rmtoll ALPHAL   ALPHAL   LL_RTC_CAL_SetQuadCurveCoeffALPHAL
 * @param  RTCx RTC Instance
 * @param  ALPHAL between Min_Data=0x0 and Max_Data=0x7FFF (unsigned)
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetQuadCurveCoeffALPHAL(RTC_TypeDef *RTCx, uint32_t ALPHAL)
{
    MODIFY_REG(RTCx->ALPHAL, RTC_ALPHAL_ALPHAL, ALPHAL);
}

/**
 * @brief  Get quadratic curve low temperature coefficient ALPHAL(Coefficient of x^2)
 * @rmtoll ALPHAL   ALPHAL   LL_RTC_CAL_GetQuadCurveCoeffALPHAL
 * @param  RTCx RTC Instance
 * @retval Between Min_Data=0x0 and Max_Data=0x7FFF (unsigned)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetQuadCurveCoeffALPHAL(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->ALPHAL, RTC_ALPHAL_ALPHAL));
}

/**
 * @brief  Set quadratic curve high temperature coefficient ALPHAH(Coefficient of x^2)
 * @note   Bits is write-protected. @ref LL_RTC_CAL_DisableCurveWriteProtection function should be called before.
 * @rmtoll ALPHAH   ALPHAH   LL_RTC_CAL_SetQuadCurveCoeffALPHAH
 * @param  RTCx RTC Instance
 * @param  ALPHAH Between Min_Data=0x0 and Max_Data=0x7FFF (unsigned)
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetQuadCurveCoeffALPHAH(RTC_TypeDef *RTCx, uint32_t ALPHAH)
{
    MODIFY_REG(RTCx->ALPHAH, RTC_ALPHAH_ALPHAH, ALPHAH);
}

/**
 * @brief  Get quadratic curve high temperature coefficient ALPHAH(Coefficient of x^2)
 * @rmtoll ALPHAH   ALPHAH   LL_RTC_CAL_GetQuadCurveCoeffALPHAH
 * @param  RTCx RTC Instance
 * @retval Between Min_Data=0x0 and Max_Data=0x7FFF (unsigned)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetQuadCurveCoeffALPHAH(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->ALPHAH, RTC_ALPHAH_ALPHAH));
}

/**
 * @brief  Set quartic curve vertex temperature XT1
 * @note   Bits is write-protected. @ref LL_RTC_CAL_DisableCurveWriteProtection function should be called before.
 * @rmtoll XT1   XT1   LL_RTC_CAL_SetQuartCurveVertexXT1
 * @param  RTCx RTC Instance
 * @param  XT1 Between Min_Data=0x0 and Max_Data=0x1FFF (unsigned)
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetQuartCurveVertexXT1(RTC_TypeDef *RTCx, uint32_t XT1)
{
    MODIFY_REG(RTCx->XT1, RTC_XT1_XT1, XT1);
}

/**
 * @brief  Get quartic curve vertex temperature XT1
 * @rmtoll XT1   XT1   LL_RTC_CAL_GetQuartCurveVertexXT1
 * @param  RTCx RTC Instance
 * @retval Between Min_Data=0x0 and Max_Data=0x1FFF (unsigned)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetQuartCurveVertexXT1(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->XT1, RTC_XT1_XT1));
}

/**
 * @brief  Set quartic curve temperature coefficient ALPHA
 * @note   Bits is write-protected. @ref LL_RTC_CAL_DisableCurveWriteProtection function should be called before.
 * @rmtoll ALPHA   ALPHA   LL_RTC_CAL_SetQuartCurveCoeffALPHA
 * @param  RTCx RTC Instance
 * @param  ALPHA  Between Min_Data=0x0 and Max_Data=0xFFFF (16bit two's complement)
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetQuartCurveCoeffALPHA(RTC_TypeDef *RTCx, uint32_t ALPHA)
{
    MODIFY_REG(RTCx->ALPHA, RTC_ALPHA_ALPHA, ALPHA);
}

/**
 * @brief  Get quartic curve temperature coefficient ALPHA
 * @rmtoll ALPHA   ALPHA   LL_RTC_CAL_GetQuartCurveCoeffALPHA
 * @param  RTCx RTC Instance
 * @retval Between Min_Data=0x0 and Max_Data=0xFFFF (16bit two's complement)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetQuartCurveCoeffALPHA(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->ALPHA, RTC_ALPHA_ALPHA));
}

/**
 * @brief  Set quartic curve temperature coefficient BETA
 * @note   Bits is write-protected. @ref LL_RTC_CAL_DisableCurveWriteProtection function should be called before.
 * @rmtoll BETA   BETA   LL_RTC_CAL_SetQuartCurveCoeffBETA
 * @param  RTCx RTC Instance
 * @param  BETA Between Min_Data=0x0 and Max_Data=0x7FFF (unsigned)
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetQuartCurveCoeffBETA(RTC_TypeDef *RTCx, uint32_t BETA)
{
    MODIFY_REG(RTCx->BETA, RTC_BETA_BETA, BETA);
}

/**
 * @brief  Get quartic curve temperature coefficient BETA
 * @rmtoll BETA   BETA   LL_RTC_CAL_GetQuartCurveCoeffBETA
 * @param  RTCx RTC Instance
 * @retval Between Min_Data=0x0 and Max_Data=0x7FFF (unsigned)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetQuartCurveCoeffBETA(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->BETA, RTC_BETA_BETA));
}

/**
 * @brief  Set quartic curve temperature coefficient GAMMA
 * @note   Bits is write-protected. @ref LL_RTC_CAL_DisableCurveWriteProtection function should be called before.
 * @rmtoll GAMMA   GAMMA   LL_RTC_CAL_SetQuartCurveCoeffGAMMA
 * @param  RTCx RTC Instance
 * @param  GAMMA Between Min_Data=0x0 and Max_Data=0xFFFF (16bit two's complement)
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetQuartCurveCoeffGAMMA(RTC_TypeDef *RTCx, uint32_t GAMMA)
{
    MODIFY_REG(RTCx->GAMMA, RTC_GAMMA_GAMMA, GAMMA);
}

/**
 * @brief  Get quartic curve temperature coefficient GAMMA
 * @rmtoll GAMMA   GAMMA   LL_RTC_CAL_GetQuartCurveCoeffGAMMA
 * @param  RTCx RTC Instance
 * @retval Between Min_Data=0x0 and Max_Data=0xFFFF (16bit two's complement)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetQuartCurveCoeffGAMMA(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->GAMMA, RTC_GAMMA_GAMMA));
}

/**
 * @brief  Set quartic curve temperature coefficient ZETA
 * @note   Bits is write-protected. @ref LL_RTC_CAL_DisableCurveWriteProtection function should be called before.
 * @rmtoll ZETA    ZETA    LL_RTC_CAL_SetQuartCurveCoeffZETA\n
 *         ZETAH   ZETAH   LL_RTC_CAL_GetQuartCurveCoeffZETA
 * @param  RTCx RTC Instance
 * @param  ZETA Between Min_Data=0x0 and Max_Data=0x3FFFF (18bit two's complement)
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetQuartCurveCoeffZETA(RTC_TypeDef *RTCx, uint32_t ZETA)
{
    MODIFY_REG(RTCx->ZETA, RTC_ZETA_ZETA, ZETA);
    MODIFY_REG(RTCx->ZETAH, RTC_ZETAH_ZETAH, ZETA >> 16U);
}

/**
 * @brief  Get quartic curve temperature coefficient ZETA
 * @rmtoll ZETA    ZETA    LL_RTC_CAL_GetQuartCurveCoeffZETA\n
 *         ZETAH   ZETAH   LL_RTC_CAL_GetQuartCurveCoeffZETA
 * @param  RTCx RTC Instance
 * @retval Between Min_Data=0x0 and Max_Data=0x3FFFF (18bit two's complement)
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetQuartCurveCoeffZETA(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->ZETA, RTC_ZETA_ZETA)|(READ_BIT(RTCx->ZETAH, RTC_ZETAH_ZETAH) << 16U));
}

/**
 * @}
 */ /* end of group RTC_LL_EF_CALCurve */
/** @defgroup RTC_LL_EF_CALFineTune FineTune for different temperature
 * @{
 */

/**
 * @brief  Enable RTC secondery calibration, fine tune by temperatures.
 * @rmtoll CALPS         CALPS         LL_RTC_CAL_EnableFineTune
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_EnableFineTune(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->CALPS, RTC_CALPS_CALPS_ENABLE);
}

/**
 * @brief  Disable RTC secondery calibration, fine tune by temperatures.
 * @rmtoll CALPS         CALPS         LL_RTC_CAL_DisableFineTune
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_DisableFineTune(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->CALPS, RTC_CALPS_CALPS_ENABLE);
}

/**
 * @brief  Indicates whether the RTC secondery calibration, fine tune by temperatures is enabled.
 * @rmtoll CALPS         CALPS         LL_RTC_CAL_IsEnabledFineTune
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_IsEnabledFineTune(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->CALPS, RTC_CALPS_CALPS_ENABLE) == (RTC_CALPS_CALPS_ENABLE)) ? 1UL : 0UL);
}

/**
 * @brief  Set the FineTuneValue for temperature range.
 * @rmtoll CAL_T0         CAL_T0            LL_RTC_SetFineTuneValue
 * @param  RTCx RTC Instance.
 * @param  TempRange This parameter can be one of the following values:
 *         @arg @ref LL_RTC_TEMPRANGE_BELOW_MINUS30
 *         @arg @ref LL_RTC_TEMPRANGE_MINUS_30_20
 *         @arg @ref LL_RTC_TEMPRANGE_MINUS_20_10
 *         @arg @ref LL_RTC_TEMPRANGE_MINUS_10_0
 *         @arg @ref LL_RTC_TEMPRANGE_0_10
 *         @arg @ref LL_RTC_TEMPRANGE_35_45
 *         @arg @ref LL_RTC_TEMPRANGE_45_55
 *         @arg @ref LL_RTC_TEMPRANGE_55_65
 *         @arg @ref LL_RTC_TEMPRANGE_65_75
 *         @arg @ref LL_RTC_TEMPRANGE_ABOVE_75
 * @param  FineTuneValue Between 0x0 and 0xFF(8bit Two's complement,bit8 is the sign bit).
 * @retval None
 */
__STATIC_INLINE void LL_RTC_CAL_SetFineTuneValue(RTC_TypeDef *RTCx, uint32_t TempRange, uint32_t FineTuneValue)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&RTCx->CAL_T0) + (TempRange * 4U)));
    MODIFY_REG( *pReg, RTC_CAL_T0_CAL_T0, (FineTuneValue << RTC_CAL_T0_CAL_T0_POS));
}

/**
 * @brief  Get target temperature range FineTuneValue.
 * @rmtoll CAL_T0         CAL_T0            LL_RTC_GetFineTuneValue
 * @param  RTCx RTC Instance.
 * @param  TempRange This parameter can be one of the following values:
 *         @arg @ref LL_RTC_TEMPRANGE_BELOW_MINUS30
 *         @arg @ref LL_RTC_TEMPRANGE_MINUS_30_20
 *         @arg @ref LL_RTC_TEMPRANGE_MINUS_20_10
 *         @arg @ref LL_RTC_TEMPRANGE_MINUS_10_0
 *         @arg @ref LL_RTC_TEMPRANGE_0_10
 *         @arg @ref LL_RTC_TEMPRANGE_35_45
 *         @arg @ref LL_RTC_TEMPRANGE_45_55
 *         @arg @ref LL_RTC_TEMPRANGE_55_65
 *         @arg @ref LL_RTC_TEMPRANGE_65_75
 *         @arg @ref LL_RTC_TEMPRANGE_ABOVE_75
 * @retval FineTuneValue
 */
__STATIC_INLINE uint32_t LL_RTC_CAL_GetFineTuneValue(RTC_TypeDef *RTCx, uint32_t TempRange)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&RTCx->CAL_T0) + (TempRange * 4U)));
    return (uint32_t)(READ_BIT(*pReg, RTC_CAL_T0_CAL_T0) >> RTC_CAL_T0_CAL_T0_POS);
}


/**
 * @}
 */ /* end of group RTC_LL_EF_CALFineTune **/

/**
 * @}
 */ /* end of group RTC_LL_EF_Calibration **/

/** @defgroup RTC_LL_EF_IO IO Configuration Functions
 * @{
 */

/**
 * @brief  Enable IO function write protection
 * @rmtoll IOMODE      PADEN_PS      LL_RTC_IO_EnableFunctionWriteProtection
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IO_EnableFunctionWriteProtection(RTC_TypeDef *RTCx)
{
    MODIFY_REG(RTCx->IOMODE, RTC_IOMODE_PADEN_PS, RTC_IOMODE_PADEN_PS_WDIS);
}

/**
 * @brief  Disable IO function write protection
 * @rmtoll IOMODE      PADEN_PS      LL_RTC_IO_DisableFunctionWriteProtection
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IO_DisableFunctionWriteProtection(RTC_TypeDef *RTCx)
{
    MODIFY_REG(RTCx->IOMODE, RTC_IOMODE_PADEN_PS, RTC_IOMODE_PADEN_PS_WEN);
}

/**
 * @brief  Set PL00 function configuration
 * @note   Bits is write-protected. @ref LL_RTC_IO_DisableFunctionWriteProtection function should be called before.
 * @note   PL00 (VBAT power domain) IO function priority: RTC IO function > GPIO IO function
 * @rmtoll IOEN        PL00EN        LL_RTC_IO_SetFunctionPL00
 * @param  RTCx RTC Instance
 * @param  Function This parameter can be one of the following values:
 *         @arg LL_RTC_PL00FUNCTION_RTCOUT
 *         @arg LL_RTC_PL00FUNCTION_KEYOUT
 *         @arg LL_RTC_PL00FUNCTION_NUL
 * LL_RTC_IO_SetFunctionPL00
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IO_SetFunctionPL00(RTC_TypeDef *RTCx, uint32_t Function)
{
    MODIFY_REG(RTCx->IOEN, RTC_IOEN_PL00EN, Function);
}

/**
 * @brief  Get PL00 function configuration
 * @rmtoll IOEN        PL00EN        LL_RTC_IO_GetFunctionPL00
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg LL_RTC_PL00FUNCTION_RTCOUT
 *         @arg LL_RTC_PL00FUNCTION_KEYOUT
 *         @arg LL_RTC_PL00FUNCTION_NUL
 */
__STATIC_INLINE uint32_t LL_RTC_IO_GetFunctionPL00(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->IOEN, RTC_IOEN_PL00EN));
}

/**
 * @brief  Set PL01 function configuration
 * @note   Bits is write-protected. @ref LL_RTC_IO_DisableFunctionWriteProtection function should be called before.
 * @note   PL01 (VBAT power domain) IO function priority: MADC IO Function > RTC IO function > GPIO IO function
 * @note   Only available when setted LL_MADC_PL01FUNCTION_NUL by @ref LL_MADC_IO_SetFunctionPL01.
 * @rmtoll IOEN        PL01EN        LL_RTC_IO_SetFunctionPL01
 * @param  RTCx RTC Instance
 * @param  Function This parameter can be one of the following values:
 *         @arg LL_RTC_PL01FUNCTION_IN
 *         @arg LL_RTC_PL01FUNCTION_TAMPER1CLR
 *         @arg LL_RTC_PL01FUNCTION_KEYOUT
 *         @arg LL_RTC_PL01FUNCTION_NUL
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IO_SetFunctionPL01(RTC_TypeDef *RTCx, uint32_t Function)
{
    MODIFY_REG(RTCx->IOEN, RTC_IOEN_PL01EN, Function);
}

/**
 * @brief  Get PL01 function configuration
 * @rmtoll IOEN        PL01EN        LL_RTC_IO_GetFunctionPL01
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg LL_RTC_PL01FUNCTION_IN
 *         @arg LL_RTC_PL01FUNCTION_TAMPER1CLR
 *         @arg LL_RTC_PL01FUNCTION_KEYOUT
 *         @arg LL_RTC_PL01FUNCTION_NUL
 */
__STATIC_INLINE uint32_t LL_RTC_IO_GetFunctionPL01(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->IOEN, RTC_IOEN_PL01EN));
}

/**
 * @brief  Set PL02 function configuration
 * @note   Bits is write-protected. @ref LL_RTC_IO_DisableFunctionWriteProtection function should be called before.
 * @note   PL02 (VBAT power domain) IO function priority: MADC IO Function > RTC IO function > GPIO IO function
 * @note   Only available when setted LL_MADC_PL02FUNCTION_NUL by @ref LL_MADC_IO_SetFunctionPL02.
 * @rmtoll IOEN        PL02EN        LL_RTC_IO_SetFunctionPL02
 * @param  RTCx RTC Instance
 * @param  Function This parameter can be one of the following values:
 *         @arg LL_RTC_PL02FUNCTION_IN
 *         @arg LL_RTC_PL02FUNCTION_TAMPER2CLR
 *         @arg LL_RTC_PL02FUNCTION_KEYOUT
 *         @arg LL_RTC_PL02FUNCTION_NUL
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IO_SetFunctionPL02(RTC_TypeDef *RTCx, uint32_t Function)
{
    MODIFY_REG(RTCx->IOEN, RTC_IOEN_PL02EN, Function);
}

/**
 * @brief  Get PL02 function configuration
 * @rmtoll IOEN        PL02EN        LL_RTC_IO_GetFunctionPL02
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg LL_RTC_PL02FUNCTION_IN
 *         @arg LL_RTC_PL02FUNCTION_TAMPER2CLR
 *         @arg LL_RTC_PL02FUNCTION_KEYOUT
 *         @arg LL_RTC_PL02FUNCTION_NUL
 */
__STATIC_INLINE uint32_t LL_RTC_IO_GetFunctionPL02(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->IOEN, RTC_IOEN_PL02EN));
}

/**
 * @brief  Set PL03 function configuration
 * @note   Bits is write-protected. @ref LL_RTC_IO_DisableFunctionWriteProtection function should be called before.
 * @note   PL03 (VBAT power domain) IO function priority: MADC IO Function > RTC IO function > GPIO IO function
 * @note   Only available when setted LL_MADC_PL03FUNCTION_NUL by @ref LL_MADC_IO_SetFunctionPL03.
 * @rmtoll IOEN        PL03EN        LL_RTC_IO_SetFunctionPL03
 * @param  RTCx RTC Instance
 * @param  Function This parameter can be one of the following values:
 *         @arg LL_RTC_PL03FUNCTION_KEYOUT
 *         @arg LL_RTC_PL03FUNCTION_NUL
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IO_SetFunctionPL03(RTC_TypeDef *RTCx, uint32_t Function)
{
    MODIFY_REG(RTCx->IOEN, RTC_IOEN_PL03EN, Function);
}

/**
 * @brief  Get PL03 function configuration
 * @rmtoll IOEN        PL03EN        LL_RTC_IO_GetFunctionPL03
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg LL_RTC_PL03FUNCTION_KEYOUT
 *         @arg LL_RTC_PL03FUNCTION_NUL
 */
__STATIC_INLINE uint32_t LL_RTC_IO_GetFunctionPL03(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->IOEN, RTC_IOEN_PL03EN));
}

/**
 * @}
 */ /* end of group RTC_LL_EF_IO */

/** @defgroup RTC_LL_EF_TAMPER TAMPER Configuration Functions
 * @{
 */

/**
 * @brief  Set TAMPER1 detection mode
 * @note   TAMPER1 detection mode requires PL01 configured as TAMPER1 input
 * @rmtoll IOMODE      TAMPER1_MODE  LL_RTC_TAMPER_SetTAMPER1Mode
 * @param  RTCx RTC Instance
 * @param  Mode This parameter can be one of the following values:
 *         @arg @ref LL_RTC_TAMPERMODE_EST
 *         @arg @ref LL_RTC_TAMPERMODE_LST
 * @retval None
 */
__STATIC_INLINE void LL_RTC_TAMPER_SetTAMPER1Mode(RTC_TypeDef *RTCx, uint32_t Mode)
{
    MODIFY_REG(RTCx->IOMODE, RTC_IOMODE_TAMPER1_MODE, Mode);
}

/**
 * @brief  Get TAMPER1 detection mode
 * @rmtoll IOMODE      TAMPER1_MODE  LL_RTC_TAMPER_GetTAMPER1Mode
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_TAMPERMODE_EST
 *         @arg @ref LL_RTC_TAMPERMODE_LST
 */
__STATIC_INLINE uint32_t LL_RTC_TAMPER_GetTAMPER1Mode(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->IOMODE, RTC_IOMODE_TAMPER1_MODE));
}

/**
 * @brief  Set TAMPER2 detection mode
 * @note   TAMPER2 detection mode requires PL02 configured as TAMPER2 input
 * @rmtoll IOMODE      TAMPER2_MODE  LL_RTC_TAMPER_SetTAMPER2Mode
 * @param  RTCx RTC Instance
 * @param  Mode This parameter can be one of the following values:
 *         @arg @ref LL_RTC_TAMPERMODE_EST
 *         @arg @ref LL_RTC_TAMPERMODE_LST
 * @retval None
 */
__STATIC_INLINE void LL_RTC_TAMPER_SetTAMPER2Mode(RTC_TypeDef *RTCx, uint32_t Mode)
{
    MODIFY_REG(RTCx->IOMODE, RTC_IOMODE_TAMPER2_MODE, Mode);
}

/**
 * @brief  Get TAMPER2 detection mode
 * @rmtoll IOMODE      TAMPER2_MODE  LL_RTC_TAMPER_GetTAMPER2Mode
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_TAMPERMODE_EST
 *         @arg @ref LL_RTC_TAMPERMODE_LST
 */
__STATIC_INLINE uint32_t LL_RTC_TAMPER_GetTAMPER2Mode(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->IOMODE, RTC_IOMODE_TAMPER2_MODE));
}

/**
 * @brief  Enable TAMPER filter write protection
 * @rmtoll IOMODE      TAMPER_FLT_PS LL_RTC_TAMPER_EnableFilterWriteProtection
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_TAMPER_EnableFilterWriteProtection(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->IOMODE, RTC_IOMODE_TAMPER_FLT_PS);
}

/**
 * @brief  Disable TAMPER filter write protection
 * @rmtoll IOMODE      TAMPER_FLT_PS LL_RTC_TAMPER_DisableFilterWriteProtection
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_TAMPER_DisableFilterWriteProtection(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->IOMODE, RTC_IOMODE_TAMPER_FLT_PS);
}

/**
 * @brief  Set TAMPER filter count
 * @note   Bits is write-protected. @ref LL_RTC_TAMPER_DisableFilterWriteProtection function should be called before.
 * @rmtoll IOMODE      TAMPER_FLT_CNT LL_RTC_TAMPER_SetFilterCount
 * @param  RTCx RTC Instance
 * @param  Count Filter count (5-bit value, range: 0-31)
 * @retval None
 */
__STATIC_INLINE void LL_RTC_TAMPER_SetFilterCount(RTC_TypeDef *RTCx, uint32_t Count)
{
    MODIFY_REG(RTCx->IOMODE, RTC_IOMODE_TAMPER_FLT_CNT, Count << RTC_IOMODE_TAMPER_FLT_CNT_POS);
}

/**
 * @brief  Get TAMPER filter count
 * @rmtoll IOMODE      TAMPER_FLT_CNT LL_RTC_TAMPER_GetFilterCount
 * @param  RTCx RTC Instance
 * @retval Filter count (5-bit value, range: 0-31)
 */
__STATIC_INLINE uint32_t LL_RTC_TAMPER_GetFilterCount(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IOMODE, RTC_IOMODE_TAMPER_FLT_CNT)) >> RTC_IOMODE_TAMPER_FLT_CNT_POS);
}

/**
 * @brief  Set TAMPER filter precision
 * @note   Bits is write-protected. @ref LL_RTC_TAMPER_DisableFilterWriteProtection function should be called before.
 * @rmtoll IOMODE      TAMPER_FLT_TS LL_RTC_TAMPER_SetFilterPrecision
 * @param  RTCx RTC Instance
 * @param  Precision This parameter can be one of the following values:
 *         @arg @ref LL_RTC_TAMPER_FILTER_PREC_2
 *         @arg @ref LL_RTC_TAMPER_FILTER_PREC_250
 * @retval None
 */
__STATIC_INLINE void LL_RTC_TAMPER_SetFilterPrecision(RTC_TypeDef *RTCx, uint32_t Precision)
{
    MODIFY_REG(RTCx->IOMODE, RTC_IOMODE_TAMPER_FLT_TS, Precision);
}

/**
 * @brief  Get TAMPER filter precision
 * @rmtoll IOMODE      TAMPER_FLT_TS LL_RTC_TAMPER_GetFilterPrecision
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_TAMPER_FILTER_PREC_2
 *         @arg @ref LL_RTC_TAMPER_FILTER_PREC_250
 */
__STATIC_INLINE uint32_t LL_RTC_TAMPER_GetFilterPrecision(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->IOMODE, RTC_IOMODE_TAMPER_FLT_TS));
}

/**
 * @}
 */ /* end of group RTC_LL_EF_TAMPER */

/** @defgroup RTC_LL_EF_KEYOUT KEYOUT Configuration Functions
 * @{
 */

/**
 * @brief  Set KEYOUT output release level
 * @rmtoll IOMODE      KEYOUT_MODE   LL_RTC_KEYOUT_SetReleaseLevel
 * @param  RTCx RTC Instance
 * @param  Level This parameter can be one of the following values:
 *         @arg @ref LL_RTC_KEYOUT_RELEASELEVEL_HIGH
 *         @arg @ref LL_RTC_KEYOUT_RELEASELEVEL_HIZ
 * @retval None
 */
__STATIC_INLINE void LL_RTC_KEYOUT_SetReleaseLevel(RTC_TypeDef *RTCx, uint32_t Level)
{
    MODIFY_REG(RTCx->IOMODE, RTC_IOMODE_KEYOUT_MODE, Level);
}

/**
 * @brief  Get KEYOUT output release level
 * @rmtoll IOMODE      KEYOUT_MODE   LL_RTC_KEYOUT_GetReleaseLevel
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_KEYOUT_RELEASELEVEL_HIGH
 *         @arg @ref LL_RTC_KEYOUT_RELEASELEVEL_HIZ
 */
__STATIC_INLINE uint32_t LL_RTC_KEYOUT_GetReleaseLevel(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->IOMODE, RTC_IOMODE_KEYOUT_MODE));
}

/**
 * @brief  Enable KEYOUT timed trigger write protection
 * @rmtoll KEYOUT_TK_CFG PS           LL_RTC_KEYOUT_EnableTimedTriggerWriteProtection
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_KEYOUT_EnableTimedTriggerWriteProtection(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->KEYOUT_TK_CFG, RTC_KEYOUT_TK_CFG_PS);
}

/**
 * @brief  Disable KEYOUT timed trigger write protection
 * @rmtoll KEYOUT_TK_CFG PS           LL_RTC_KEYOUT_DisableTimedTriggerWriteProtection
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_KEYOUT_DisableTimedTriggerWriteProtection(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->KEYOUT_TK_CFG, RTC_KEYOUT_TK_CFG_PS);
}
/**
 * @brief  Enable KEYOUT timed trigger
 * @note   Bits is write-protected. @ref LL_RTC_KEYOUT_DisableTimedTriggerWriteProtection function should be called before.
 * @rmtoll KEYOUT_TK_CFG EN           LL_RTC_KEYOUT_EnableTimedTrigger
 * @param  RTCx RTC Instance
 * @param  Channels Timed trigger channels to enable (bit mask of channels)
 * @retval None
 */
__STATIC_INLINE void LL_RTC_KEYOUT_EnableTimedTrigger(RTC_TypeDef *RTCx, uint32_t Channels)
{
    SET_BIT(RTCx->KEYOUT_TK_CFG, Channels & RTC_KEYOUT_TK_CFG_EN);
}

/**
 * @brief  Disable KEYOUT timed trigger
 * @note   Bits is write-protected. @ref LL_RTC_KEYOUT_DisableTimedTriggerWriteProtection function should be called before.
 * @rmtoll KEYOUT_TK_CFG EN           LL_RTC_KEYOUT_DisableTimedTrigger
 * @param  RTCx RTC Instance
 * @param  Channels Timed trigger channels to disable (bit mask of channels)
 * @retval None
 */
__STATIC_INLINE void LL_RTC_KEYOUT_DisableTimedTrigger(RTC_TypeDef *RTCx, uint32_t Channels)
{
    CLEAR_BIT(RTCx->KEYOUT_TK_CFG, Channels & RTC_KEYOUT_TK_CFG_EN);
}

/**
 * @brief  Check if KEYOUT timed trigger is enabled
 * @rmtoll KEYOUT_TK_CFG EN           LL_RTC_KEYOUT_IsEnabledTimedTrigger
 * @param  RTCx RTC Instance
 * @param  Channel Timed trigger channel to check
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_KEYOUT_IsEnabledTimedTrigger(RTC_TypeDef *RTCx, uint32_t Channel)
{
    return (uint32_t)((READ_BIT(RTCx->KEYOUT_TK_CFG, Channel & RTC_KEYOUT_TK_CFG_EN) == (Channel & RTC_KEYOUT_TK_CFG_EN)) ? 1UL : 0UL);
}

/**
 * @brief  Set KEYOUT timed trigger time base
 * @note   Bits is write-protected. @ref LL_RTC_KEYOUT_DisableTimedTriggerWriteProtection function should be called before.
 * @rmtoll KEYOUT_TK_CFG TB           LL_RTC_KEYOUT_SetTimedTriggerTimeBase
 * @param  RTCx RTC Instance
 * @param  TimeBase This parameter can be one of the following values:
 *         @arg @ref LL_RTC_TIMEBASE_SEC
 *         @arg @ref LL_RTC_TIMEBASE_MIN
 * @retval None
 */
__STATIC_INLINE void LL_RTC_KEYOUT_SetTimedTriggerTimeBase(RTC_TypeDef *RTCx, uint32_t TimeBase)
{
    MODIFY_REG(RTCx->KEYOUT_TK_CFG, RTC_KEYOUT_TK_CFG_TB, TimeBase);
}

/**
 * @brief  Get KEYOUT timed trigger time base
 * @rmtoll KEYOUT_TK_CFG TB           LL_RTC_KEYOUT_GetTimedTriggerTimeBase
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_TIMEBASE_SEC
 *         @arg @ref LL_RTC_TIMEBASE_MIN
 */
__STATIC_INLINE uint32_t LL_RTC_KEYOUT_GetTimedTriggerTimeBase(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->KEYOUT_TK_CFG, RTC_KEYOUT_TK_CFG_TB));
}

/**
 * @brief  Set KEYOUT timed trigger target value
 * @note   Bits is write-protected. @ref LL_RTC_KEYOUT_DisableTimedTriggerWriteProtection function should be called before.
 * @rmtoll KEYOUT_TK_CFG CNT          LL_RTC_KEYOUT_SetTimedTriggerTargetValue
 * @param  RTCx RTC Instance
 * @param  Value Target value (6-bit value, range: 0-63)
 * @retval None
 */
__STATIC_INLINE void LL_RTC_KEYOUT_SetTimedTriggerTargetValue(RTC_TypeDef *RTCx, uint32_t Value)
{
    MODIFY_REG(RTCx->KEYOUT_TK_CFG, RTC_KEYOUT_TK_CFG_CNT, Value << RTC_KEYOUT_TK_CFG_CNT_POS);
}

/**
 * @brief  Get KEYOUT timed trigger target value
 * @rmtoll KEYOUT_TK_CFG CNT          LL_RTC_KEYOUT_GetTimedTriggerTargetValue
 * @param  RTCx RTC Instance
 * @retval Target value (6-bit value, range: 0-63)
 */
__STATIC_INLINE uint32_t LL_RTC_KEYOUT_GetTimedTriggerTargetValue(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->KEYOUT_TK_CFG, RTC_KEYOUT_TK_CFG_CNT)) >> RTC_KEYOUT_TK_CFG_CNT_POS);
}

/**
 * @brief  Disable KEYOUT soft release write protection
 * @rmtoll KEYOUT_SREL  PS           LL_RTC_KEYOUT_DisableSoftReleaseWriteProtection
 * @param  RTCx RTC Instances
 * @retval None
 */
__STATIC_INLINE void LL_RTC_KEYOUT_DisableSoftReleaseWriteProtection(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->KEYOUT_SREL, RTC_KEYOUT_SREL_PS_ENWT);
}

/**
 * @brief  Enable KEYOUT soft release write protection password
 * @rmtoll KEYOUT_SREL  PS           LL_RTC_KEYOUT_EnableSoftReleaseWriteProtection
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_KEYOUT_EnableSoftReleaseWriteProtection(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->KEYOUT_SREL, RTC_KEYOUT_SREL_PS_DISWT);
}

/**
 * @brief  Set KEYOUT soft release value
 * @note   Bits is write-protected. @ref LL_RTC_KEYOUT_DisableSoftReleaseWriteProtection function should be called before.
 * @rmtoll KEYOUT_SREL  SREL         LL_RTC_KEYOUT_SetSoftRelease
 * @param  RTCx RTC Instance
 * @param  Value Soft release value (8-bit value, range: 0-255)
 * @retval None
 */
__STATIC_INLINE void LL_RTC_KEYOUT_SetSoftRelease(RTC_TypeDef *RTCx, uint32_t Value)
{
    MODIFY_REG(RTCx->KEYOUT_SREL, RTC_KEYOUT_SREL_SREL, Value);
}

/**
 * @brief  Get KEYOUT soft release value
 * @rmtoll KEYOUT_SREL  SREL         LL_RTC_KEYOUT_GetSoftRelease
 * @param  RTCx RTC Instance
 * @retval Soft release value (8-bit value, range: 0-255)
 */
__STATIC_INLINE uint32_t LL_RTC_KEYOUT_GetSoftRelease(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->KEYOUT_SREL, RTC_KEYOUT_SREL_SREL));
}

/**
 * @}
 */ /* end of group RTC_LL_EF_KEYOUT */


/** @defgroup RTC_LL_EF_Backup_Registers Backup_Registers
 * @{
 */

/**
 * @brief  Enable the write protection for RTC registers.
 * @rmtoll BKPPS           PS           LL_RTC_BAK_EnableWriteProtection
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_BAK_EnableWriteProtection(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->BKPPS, RTC_BKPPS_PS_DISWT);
}

/**
 * @brief  Disable the write protection for RTC registers.
 * @rmtoll BKPPS           PS           LL_RTC_BAK_DisableWriteProtection
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_BAK_DisableWriteProtection(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->BKPPS, RTC_BKPPS_PS_ENWT);
}

/**
 * @brief  Writes a data in a specified RTC Backup data register.
 * @note   Bits is write-protected. @ref LL_RTC_BAK_DisableWriteProtection function should be called before.
 * @rmtoll BKPx        BKP           LL_RTC_BAK_SetRegister
 * @param  RTCx RTC Instance
 * @param  BackupRegister This parameter can be one of the following values:
 *         @arg @ref LL_RTC_BKP_DR0
 *         @arg @ref LL_RTC_BKP_DR1
 *         @arg @ref LL_RTC_BKP_DR2
 *         @arg @ref LL_RTC_BKP_DR3
 *         @arg @ref LL_RTC_BKP_DR4
 *         @arg @ref LL_RTC_BKP_DR5
 *         @arg @ref LL_RTC_BKP_DR6
 *         @arg @ref LL_RTC_BKP_DR7
 *         @arg @ref LL_RTC_BKP_DR8
 *         @arg @ref LL_RTC_BKP_DR9
 *         @arg @ref LL_RTC_BKP_DR10
 *         @arg @ref LL_RTC_BKP_DR11
 *         @arg @ref LL_RTC_BKP_DR12
 *         @arg @ref LL_RTC_BKP_DR13
 *         @arg @ref LL_RTC_BKP_DR14
 *         @arg @ref LL_RTC_BKP_DR15
 * @param  Data Value between Min_Data=0x00 and Max_Data=0xFFFF
 * @retval None
 */
__STATIC_INLINE void LL_RTC_BAK_SetRegister(RTC_TypeDef *RTCx, uint32_t BackupRegister, uint32_t Data)
{
    uint32_t temp;

    temp = (uint32_t)(&(RTCx->BKP0));
    temp += (BackupRegister * 4U);

    /* Write the specified register **/
    *(__IO uint32_t *)temp = (uint32_t)Data;
}

/**
 * @brief  Reads data from the specified RTC Backup data Register.
 * @rmtoll BKPxR        BKP           LL_RTC_BAK_GetRegister
 * @param  RTCx RTC Instance
 * @param  BackupRegister This parameter can be one of the following values:
 *         @arg @ref LL_RTC_BKP_DR0
 *         @arg @ref LL_RTC_BKP_DR1
 *         @arg @ref LL_RTC_BKP_DR2
 *         @arg @ref LL_RTC_BKP_DR3
 *         @arg @ref LL_RTC_BKP_DR4
 *         @arg @ref LL_RTC_BKP_DR5
 *         @arg @ref LL_RTC_BKP_DR6
 *         @arg @ref LL_RTC_BKP_DR7
 *         @arg @ref LL_RTC_BKP_DR8
 *         @arg @ref LL_RTC_BKP_DR9
 *         @arg @ref LL_RTC_BKP_DR10
 *         @arg @ref LL_RTC_BKP_DR11
 *         @arg @ref LL_RTC_BKP_DR12
 *         @arg @ref LL_RTC_BKP_DR13
 *         @arg @ref LL_RTC_BKP_DR14
 *         @arg @ref LL_RTC_BKP_DR15
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFF
 */
__STATIC_INLINE uint32_t LL_RTC_BAK_GetRegister(RTC_TypeDef *RTCx, uint32_t BackupRegister)
{
    uint32_t temp;

    temp = (uint32_t)(&(RTCx->BKP0));
    temp += (BackupRegister * 4U);

    /* Read the specified register **/
    return (*(__IO uint32_t *)temp);
}

/**
 * @}
 */ /* end of group RTC_LL_EF_Backup_Registers **/

/** @defgroup RTC_LL_EF_IT IT Management functions
 * @{
 */

/**
 * @brief  Set 1S Clock Source
 * @rmtoll IE          RTC_1S_SEL     LL_RTC_IT_SetInterupt1SClockSource
 * @param  RTCx RTC Instance
 * @param  ClockSource This parameter can be one of the following values:
 *         @arg @ref LL_RTC_1S_CLOCKSOURCE_SYSCLK
 *         @arg @ref LL_RTC_1S_CLOCKSOURCE_RTC
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_SetInterupt1SClockSource(RTC_TypeDef *RTCx, uint32_t ClockSource)
{
    MODIFY_REG(RTCx->IE, RTC_IE_RTC_1S_SEL, ClockSource);
}

/**
 * @brief  Get 1S Clock Source
 * @rmtoll IE          RTC_1S_SEL     LL_RTC_IT_GetInterupt1SClockSource
 * @param  RTCx RTC Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_RTC_1S_CLOCKSOURCE_SYSCLK
 *         @arg @ref LL_RTC_1S_CLOCKSOURCE_RTC
 */
__STATIC_INLINE uint32_t LL_RTC_IT_GetInterupt1SClockSource(RTC_TypeDef *RTCx)
{
    return (uint32_t)(READ_BIT(RTCx->IE, RTC_IE_RTC_1S_SEL));
}

/**
 * @brief  Enable Interrupt Clock
 * @rmtoll IE          IECLKEN        LL_RTC_IT_EnableInteruptClock
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableInteruptClock(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->IE, RTC_IE_IECLKEN);
}

/**
 * @brief  Disable Interrupt Clock
 * @rmtoll IE          IECLKEN        LL_RTC_IT_DisableInteruptClock
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableInteruptClock(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->IE, RTC_IE_IECLKEN);
}

/**
 * @brief  Check if Interrupt Clock is enabled
 * @rmtoll IE          IECLKEN        LL_RTC_IT_IsEnabledInteruptClock
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledInteruptClock(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IE, RTC_IE_IECLKEN) == RTC_IE_IECLKEN) ? 1UL : 0UL);
}

/**
 * @brief  Enable Alarm interrupt
 * @note   Bit is enabled only when RTC_IE_IECLKEN is also set
 * @rmtoll IE          ALMIE          LL_RTC_IT_EnableALM
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableALM(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->IE, RTC_IE_ALMIE | RTC_IE_IECLKEN);
}

/**
 * @brief  Disable Alarm interrupt
 * @rmtoll IE          ALMIE          LL_RTC_IT_DisableALM
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableALM(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->IE, RTC_IE_ALMIE);
}

/**
 * @brief  Check if Alarm interrupt is enabled
 * @rmtoll IE          ALMIE          LL_RTC_IT_IsEnabledALM
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledALM(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IE, RTC_IE_ALMIE) == RTC_IE_ALMIE) ? 1UL : 0UL);
}

/**
 * @brief  Enable Counter1 interrupt
 * @note   Bit is enabled only when RTC_IE_IECLKEN is also set
 * @rmtoll IE          CNT1IE         LL_RTC_IT_EnableCNT1
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableCNT1(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->IE, RTC_IE_CNT1IE | RTC_IE_IECLKEN);
}

/**
 * @brief  Disable Counter1 interrupt
 * @rmtoll IE          CNT1IE         LL_RTC_IT_DisableCNT1
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableCNT1(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->IE, RTC_IE_CNT1IE);
}

/**
 * @brief  Check if Counter1 interrupt is enabled
 * @rmtoll IE          CNT1IE         LL_RTC_IT_IsEnabledCNT1
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledCNT1(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IE, RTC_IE_CNT1IE) == RTC_IE_CNT1IE) ? 1UL : 0UL);
}

/**
 * @brief  Enable Counter2 interrupt
 * @note   Bit is enabled only when RTC_IE_IECLKEN is also set
 * @rmtoll IE          CNT2IE         LL_RTC_IT_EnableCNT2
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableCNT2(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->IE, RTC_IE_CNT2IE | RTC_IE_IECLKEN);
}

/**
 * @brief  Disable Counter2 interrupt
 * @rmtoll IE          CNT2IE         LL_RTC_IT_DisableCNT2
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableCNT2(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->IE, RTC_IE_CNT2IE);
}

/**
 * @brief  Check if Counter2 interrupt is enabled
 * @rmtoll IE          CNT2IE         LL_RTC_IT_IsEnabledCNT2
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledCNT2(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IE, RTC_IE_CNT2IE) == RTC_IE_CNT2IE) ? 1UL : 0UL);
}

/**
 * @brief  Enable Second interrupt
 * @note   Bit is enabled only when RTC_IE_IECLKEN is also set
 * @rmtoll IE          SCIE           LL_RTC_IT_EnableSC
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableSC(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->IE, RTC_IE_SCIE | RTC_IE_IECLKEN);
}

/**
 * @brief  Disable Second interrupt
 * @rmtoll IE          SCIE           LL_RTC_IT_DisableSC
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableSC(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->IE, RTC_IE_SCIE);
}

/**
 * @brief  Check if Second interrupt is enabled
 * @rmtoll IE          SCIE           LL_RTC_IT_IsEnabledSC
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledSC(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IE, RTC_IE_SCIE) == RTC_IE_SCIE) ? 1UL : 0UL);
}

/**
 * @brief  Enable Minute interrupt
 * @note   Bit is enabled only when RTC_IE_IECLKEN is also set
 * @rmtoll IE          MNIE           LL_RTC_IT_EnableMN
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableMN(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->IE, RTC_IE_MNIE | RTC_IE_IECLKEN);
}

/**
 * @brief  Disable Minute interrupt
 * @rmtoll IE          MNIE           LL_RTC_IT_DisableMN
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableMN(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->IE, RTC_IE_MNIE);
}

/**
 * @brief  Check if Minute interrupt is enabled
 * @rmtoll IE          MNIE           LL_RTC_IT_IsEnabledMN
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledMN(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IE, RTC_IE_MNIE) == RTC_IE_MNIE) ? 1UL : 0UL);
}

/**
 * @brief  Enable Hour interrupt
 * @note   Bit is enabled only when RTC_IE_IECLKEN is also set
 * @rmtoll IE          HRIE           LL_RTC_IT_EnableHR
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableHR(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->IE, RTC_IE_HRIE | RTC_IE_IECLKEN);
}

/**
 * @brief  Disable Hour interrupt
 * @rmtoll IE          HRIE           LL_RTC_IT_DisableHR
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableHR(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->IE, RTC_IE_HRIE);
}

/**
 * @brief  Check if Hour interrupt is enabled
 * @rmtoll IE          HRIE           LL_RTC_IT_IsEnabledHR
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledHR(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IE, RTC_IE_HRIE) == RTC_IE_HRIE) ? 1UL : 0UL);
}

/**
 * @brief  Enable Date interrupt
 * @rmtoll IE          DTIE           LL_RTC_IT_EnableDT
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableDT(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->IE, RTC_IE_DTIE | RTC_IE_IECLKEN);
}

/**
 * @brief  Disable Date interrupt
 * @rmtoll IE          DTIE           LL_RTC_IT_DisableDT
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableDT(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->IE, RTC_IE_DTIE);
}

/**
 * @brief  Check if Date interrupt is enabled
 * @rmtoll IE          DTIE           LL_RTC_IT_IsEnabledDT
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledDT(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IE, RTC_IE_DTIE) == RTC_IE_DTIE) ? 1UL : 0UL);
}

/**
 * @brief  Enable Month interrupt
 * @rmtoll IE          MOIE           LL_RTC_IT_EnableMO
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableMO(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->IE, RTC_IE_MOIE | RTC_IE_IECLKEN);
}

/**
 * @brief  Disable Month interrupt
 * @rmtoll IE          MOIE           LL_RTC_IT_DisableMO
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableMO(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->IE, RTC_IE_MOIE);
}

/**
 * @brief  Check if Month interrupt is enabled
 * @rmtoll IE          MOIE           LL_RTC_IT_IsEnabledMO
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledMO(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IE, RTC_IE_MOIE) == RTC_IE_MOIE) ? 1UL : 0UL);
}

/**
 * @brief  Enable TAMPER1 Positive edge interrupt
 * @rmtoll TAMPERIE    TAMPER1PIE     LL_RTC_IT_EnableTAMPER1P
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableTAMPER1P(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_TAMPER1PIE);
}

/**
 * @brief  Disable TAMPER1 Positive edge interrupt
 * @rmtoll TAMPERIE    TAMPER1PIE     LL_RTC_IT_DisableTAMPER1P
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableTAMPER1P(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_TAMPER1PIE);
}

/**
 * @brief  Check if TAMPER1 Positive edge interrupt is enabled
 * @rmtoll TAMPERIE    TAMPER1PIE     LL_RTC_IT_IsEnabledTAMPER1P
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledTAMPER1P(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_TAMPER1PIE) == RTC_TAMPERIE_TAMPER1PIE) ? 1UL : 0UL);
}

/**
 * @brief  Enable TAMPER1 Negative edge interrupt
 * @rmtoll TAMPERIE    TAMPER1NIE     LL_RTC_IT_EnableTAMPER1N
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableTAMPER1N(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_TAMPER1NIE);
}

/**
 * @brief  Disable TAMPER1 Negative edge interrupt
 * @rmtoll TAMPERIE    TAMPER1NIE     LL_RTC_IT_DisableTAMPER1N
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableTAMPER1N(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_TAMPER1NIE);
}

/**
 * @brief  Check if TAMPER1 Negative edge interrupt is enabled
 * @rmtoll TAMPERIE    TAMPER1NIE     LL_RTC_IT_IsEnabledTAMPER1N
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledTAMPER1N(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_TAMPER1NIE) == RTC_TAMPERIE_TAMPER1NIE) ? 1UL : 0UL);
}

/**
 * @brief  Enable TAMPER2 Positive edge interrupt
 * @rmtoll TAMPERIE    TAMPER2PIE     LL_RTC_IT_EnableTAMPER2P
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableTAMPER2P(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_TAMPER2PIE);
}

/**
 * @brief  Disable TAMPER2 Positive edge interrupt
 * @rmtoll TAMPERIE    TAMPER2PIE     LL_RTC_IT_DisableTAMPER2P
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableTAMPER2P(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_TAMPER2PIE);
}

/**
 * @brief  Check if TAMPER2 Positive edge interrupt is enabled
 * @rmtoll TAMPERIE    TAMPER2PIE     LL_RTC_IT_IsEnabledTAMPER2P
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledTAMPER2P(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_TAMPER2PIE) == RTC_TAMPERIE_TAMPER2PIE) ? 1UL : 0UL);
}

/**
 * @brief  Enable TAMPER2 Negative edge interrupt
 * @rmtoll TAMPERIE    TAMPER2NIE     LL_RTC_IT_EnableTAMPER2N
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableTAMPER2N(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_TAMPER2NIE);
}

/**
 * @brief  Disable TAMPER2 Negative edge interrupt
 * @rmtoll TAMPERIE    TAMPER2NIE     LL_RTC_IT_DisableTAMPER2N
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableTAMPER2N(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_TAMPER2NIE);
}

/**
 * @brief  Check if TAMPER2 Negative edge interrupt is enabled
 * @rmtoll TAMPERIE    TAMPER2NIE     LL_RTC_IT_IsEnabledTAMPER2N
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledTAMPER2N(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_TAMPER2NIE) == RTC_TAMPERIE_TAMPER2NIE) ? 1UL : 0UL);
}

/**
 * @brief  Enable Software time freeze interrupt
 * @rmtoll TAMPERIE    STFIE          LL_RTC_IT_EnableSTF
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableSTF(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_STFIE);
}

/**
 * @brief  Disable Software time freeze interrupt
 * @rmtoll TAMPERIE    STFIE          LL_RTC_IT_DisableSTF
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableSTF(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_STFIE);
}

/**
 * @brief  Check if Software time freeze interrupt is enabled
 * @rmtoll TAMPERIE    STFIE          LL_RTC_IT_IsEnabledSTF
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledSTF(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_STFIE) == RTC_TAMPERIE_STFIE) ? 1UL : 0UL);
}

/**
 * @brief  Enable KEYOUT timed trigger interrupt
 * @rmtoll TAMPERIE    KEYOTKIE       LL_RTC_IT_EnableKEYOTK
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_EnableKEYOTK(RTC_TypeDef *RTCx)
{
    SET_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_KEYOTKIE);
}

/**
 * @brief  Disable KEYOUT timed trigger interrupt
 * @rmtoll TAMPERIE    KEYOTKIE       LL_RTC_IT_DisableKEYOTK
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_IT_DisableKEYOTK(RTC_TypeDef *RTCx)
{
    CLEAR_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_KEYOTKIE);
}

/**
 * @brief  Check if KEYOUT timed trigger interrupt is enabled
 * @rmtoll TAMPERIE    KEYOTKIE       LL_RTC_IT_IsEnabledKEYOTK
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IT_IsEnabledKEYOTK(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->TAMPERIE, RTC_TAMPERIE_KEYOTKIE) == RTC_TAMPERIE_KEYOTKIE) ? 1UL : 0UL);
}

/**
 * @}
 */ /* end of group RTC_LL_EF_IT **/

/** @defgroup RTC_LL_EF_FLAG_Management FLAG_Management
 * @{
 */

/**
 * @brief  Check if TAMPER1 Positive edge interrupt flag is set or not
 * @rmtoll TAMPERIF      TAMPER1PIF      LL_RTC_IsActiveFlag_TAMPER1P
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_TAMPER1P(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->TAMPERIF, RTC_TAMPERIF_TAMPER1PIF) == (RTC_TAMPERIF_TAMPER1PIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear TAMPER1 Positive edge interrupt flag
 * @rmtoll TAMPERIF      TAMPER1PIF      LL_RTC_ClearFlag_TAMPER1P
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ClearFlag_TAMPER1P(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->TAMPERIF, (RTC_TAMPERIF_TAMPER1PIF));
}

/**
 * @brief  Check if TAMPER1 Negative edge interrupt flag is set or not
 * @rmtoll TAMPERIF      TAMPER1NIF      LL_RTC_IsActiveFlag_TAMPER1N
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_TAMPER1N(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->TAMPERIF, RTC_TAMPERIF_TAMPER1NIF) == (RTC_TAMPERIF_TAMPER1NIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear TAMPER1 Negative edge interrupt flag
 * @rmtoll TAMPERIF      TAMPER1NIF      LL_RTC_ClearFlag_TAMPER1N
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ClearFlag_TAMPER1N(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->TAMPERIF, (RTC_TAMPERIF_TAMPER1NIF));
}

/**
 * @brief  Check if TAMPER2 Positive edge interrupt flag is set or not
 * @rmtoll TAMPERIF      TAMPER2PIF      LL_RTC_IsActiveFlag_TAMPER2P
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_TAMPER2P(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->TAMPERIF, RTC_TAMPERIF_TAMPER2PIF) == (RTC_TAMPERIF_TAMPER2PIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear TAMPER2 Positive edge interrupt flag
 * @rmtoll TAMPERIF      TAMPER2PIF      LL_RTC_ClearFlag_TAMPER2P
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ClearFlag_TAMPER2P(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->TAMPERIF, (RTC_TAMPERIF_TAMPER2PIF));
}

/**
 * @brief  Check if TAMPER2 Negative edge interrupt flag is set or not
 * @rmtoll TAMPERIF      TAMPER2NIF      LL_RTC_IsActiveFlag_TAMPER2N
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_TAMPER2N(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->TAMPERIF, RTC_TAMPERIF_TAMPER2NIF) == (RTC_TAMPERIF_TAMPER2NIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear TAMPER2 Negative edge interrupt flag
 * @rmtoll TAMPERIF      TAMPER2NIF      LL_RTC_ClearFlag_TAMPER2N
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ClearFlag_TAMPER2N(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->TAMPERIF, (RTC_TAMPERIF_TAMPER2NIF));
}

/**
 * @brief  Check if Software time freeze interrupt flag is set or not
 * @rmtoll TAMPERIF      STFIF          LL_RTC_IsActiveFlag_STF
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_STF(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->TAMPERIF, RTC_TAMPERIF_STFIF) == (RTC_TAMPERIF_STFIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear Software time freeze interrupt flag
 * @rmtoll TAMPERIF      STFIF          LL_RTC_ClearFlag_STF
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ClearFlag_STF(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->TAMPERIF, (RTC_TAMPERIF_STFIF));
}

/**
 * @brief  Check if KEYOUT timed trigger interrupt flag is set or not
 * @rmtoll TAMPERIF      KEYOTKIF       LL_RTC_IsActiveFlag_KEYOTK
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_KEYOTK(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->TAMPERIF, RTC_TAMPERIF_KEYOTKIF) == (RTC_TAMPERIF_KEYOTKIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear KEYOUT timed trigger interrupt flag
 * @rmtoll TAMPERIF      KEYOTKIF       LL_RTC_ClearFlag_KEYOTK
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ClearFlag_KEYOTK(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->TAMPERIF, (RTC_TAMPERIF_KEYOTKIF));
}
/**
 * @brief  Check if Alarm event interrupt flag is set or not
 * @note   The flag bit only becomes active after the @ref LL_RTC_IT_EnableALM or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            ALMIF          LL_RTC_IsActiveFlag_ALM
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_ALM(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IF, RTC_IF_ALMIF) == (RTC_IF_ALMIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear Alarm event interrupt flag
 * @note   The flag bit only becomes active after the @ref LL_RTC_IT_EnableALM or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            ALMIF          LL_RTC_ClearFlag_ALM
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ClearFlag_ALM(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->IF, (RTC_IF_ALMIF));
}

/**
 * @brief  Check if RTC counter1 event interrupt flag is set or not
 * @note   The flag bit only becomes active after the @ref  LL_RTC_IT_EnableCNT1 or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            CNT1IF         LL_RTC_IsActiveFlag_CNT1
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_CNT1(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IF, RTC_IF_CNT1IF) == (RTC_IF_CNT1IF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear RTC counter1 event interrupt flag
 * @rmtoll IF            CNT1IF         LL_RTC_ClearFlag_CNT1
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ClearFlag_CNT1(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->IF, (RTC_IF_CNT1IF));
}

/**
 * @brief  Check if RTC counter2 event interrupt flag is set or not
 * @note   The flag bit only becomes active after the @ref  LL_RTC_IT_EnableCNT2 or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            CNT2IF         LL_RTC_IsActiveFlag_CNT2
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_CNT2(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IF, RTC_IF_CNT2IF) == (RTC_IF_CNT2IF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear RTC counter2 event interrupt flag
 * @note   The flag bit only becomes active after the @ref  LL_RTC_IT_EnableCNT2 or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            CNT2IF         LL_RTC_ClearFlag_CNT2
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ClearFlag_CNT2(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->IF, (RTC_IF_CNT2IF));
}

/**
 * @brief  Check if Second event interrupt flag is set or not
 * @note   The flag bit only becomes active after the @ref  LL_RTC_IT_EnableSC or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            SCIF           LL_RTC_IsActiveFlag_SC
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_SC(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IF, RTC_IF_SCIF) == (RTC_IF_SCIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear Second event interrupt flag
 * @note   The flag bit only becomes active after the @ref  LL_RTC_IT_EnableSC or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            SCIF           LL_RTC_ClearFlag_SC
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ClearFlag_SC(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->IF, (RTC_IF_SCIF));
}

/**
 * @brief  Check if Minute event interrupt flag is set or not
 * @note   The flag bit only becomes active after the @ref  LL_RTC_IT_EnableMN or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            MNIF           LL_RTC_IsActiveFlag_MN
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_MN(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IF, RTC_IF_MNIF) == (RTC_IF_MNIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear Minute event interrupt flag
 * @note   The flag bit only becomes active after the @ref  LL_RTC_IT_EnableMN or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            MNIF           LL_RTC_ClearFlag_MN
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ClearFlag_MN(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->IF, (RTC_IF_MNIF));
}

/**
 * @brief  Check if Hour event interrupt flag is set or not
 * @note   The flag bit only becomes active after the @ref  LL_RTC_IT_EnableHR or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            HRIF           LL_RTC_IsActiveFlag_HR
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_HR(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IF, RTC_IF_HRIF) == (RTC_IF_HRIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear Hour event interrupt flag
 * @note   The flag bit only becomes active after the @ref  LL_RTC_IT_EnableHR or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            HRIF           LL_RTC_ClearFlag_HR
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ClearFlag_HR(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->IF, (RTC_IF_HRIF));
}

/**
 * @brief  Check if Date event interrupt flag is set or not
 * @note   The flag bit only becomes active after the @ref  LL_RTC_IT_EnableDT or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            DTIF           LL_RTC_IsActiveFlag_DT
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_DT(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IF, RTC_IF_DTIF) == (RTC_IF_DTIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear Date event interrupt flag
 * @note   The flag bit only becomes active after the @ref  LL_RTC_IT_EnableDT or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            DTIF           LL_RTC_ClearFlag_DT
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ClearFlag_DT(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->IF, (RTC_IF_DTIF));
}

/**
 * @brief  Check if Month event interrupt flag is set or not
 * @note   The flag bit only becomes active after the @ref  LL_RTC_IT_EnableMO or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            MOIF           LL_RTC_IsActiveFlag_MO
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_MO(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->IF, RTC_IF_MOIF) == (RTC_IF_MOIF)) ? 1UL : 0UL);
}

/**
 * @brief  Clear Month event interrupt flag
 * @note   The flag bit only becomes active after the @ref  LL_RTC_IT_EnableMO or @ref LL_RTC_IT_EnableInteruptClock function is called.
 * @rmtoll IF            MOIF           LL_RTC_ClearFlag_MO
 * @param  RTCx RTC Instance
 * @retval None
 */
__STATIC_INLINE void LL_RTC_ClearFlag_MO(RTC_TypeDef *RTCx)
{
    WRITE_REG(RTCx->IF, (RTC_IF_MOIF));
}

/**
 * @brief  Check if RTC register writing busy flag is set or not
 * @rmtoll CTRL          WRBUSY         LL_RTC_IsActiveFlag_WRBUSY
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_WRBUSY(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->CTRL, RTC_CTRL_WRBUSY) == (RTC_CTRL_WRBUSY)) ? 1UL : 0UL);
}

/**
 * @brief  Check if RTC calibration busy flag is set or not
 * @rmtoll CTRL          CALBUSY        LL_RTC_IsActiveFlag_CALBUSY
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_CALBUSY(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->CTRL, RTC_CTRL_CALBUSY) == (RTC_CTRL_CALBUSY)) ? 1UL : 0UL);
}

/**
 * @brief  Check if LOSC running fail flag is set or not
 * @rmtoll LOSC_CFG1     LOSC_FAIL      LL_RTC_IsActiveFlag_LOSCFAIL
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_LOSCFAIL(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->LOSC_CFG1, RTC_LOSC_CFG1_LOSC_FAIL) == (RTC_LOSC_CFG1_LOSC_FAIL)) ? 1UL : 0UL);
}

/**
 * @brief  Check if VCC running flag is set or not
 * @rmtoll LOSC_CFG1     VCC_BOI_FLAG   LL_RTC_IsActiveFlag_VCCBOIFLAG
 * @param  RTCx RTC Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_RTC_IsActiveFlag_VCCBOIFLAG(RTC_TypeDef *RTCx)
{
    return (uint32_t)((READ_BIT(RTCx->LOSC_CFG1, RTC_LOSC_CFG1_VCC_BOI_FLAG) == (RTC_LOSC_CFG1_VCC_BOI_FLAG)) ? 1UL : 0UL);
}

/**
 * @}
 */ /* end of group RTC_LL_EF_FLAG_Management **/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup RTC_LL_EF_Init Initialization and de-initialization functions
 * @{
 */
ErrorStatus LL_RTC_DeInit(RTC_TypeDef *RTCx);
ErrorStatus LL_RTC_TIME_DATE_Init(RTC_TypeDef *RTCx,  LL_RTC_TimeTypeDef *RTC_TimeStruct, LL_RTC_DateTypeDef *RTC_DateStruct);
ErrorStatus LL_RTC_ALARM_Init(RTC_TypeDef *RTCx, uint32_t RTC_Format, LL_RTC_AlarmTypeDef *RTC_AlarmStruct);
void        LL_RTC_TIME_StructInit(LL_RTC_TimeTypeDef *RTC_TimeStruct);
void        LL_RTC_DATE_StructInit(LL_RTC_DateTypeDef *RTC_DateStruct);
void        LL_RTC_ALARM_StructInit(LL_RTC_AlarmTypeDef *RTC_AlarmStruct);
/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */ /* end of group RTC_LL_Exported_Functions **/

/**
 * @}
 */ /* end of group RTC_LL **/

/**
 * @}
 */ /* end of group RN_LL_Driver **/

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_LL_RTC_H **/
