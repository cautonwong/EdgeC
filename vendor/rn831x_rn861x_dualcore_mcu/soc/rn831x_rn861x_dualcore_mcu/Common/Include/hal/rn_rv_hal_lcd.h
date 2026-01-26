/**
 ******************************************************************************
 * @file   rn_rv_hal_lcd.h
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
#ifndef __RN_RV_HAL_LCD_H
#define __RN_RV_HAL_LCD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal_def.h"
#include "rn_rv_ll_lcd.h"
#include "rn_rv_ll_gpio.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @addtogroup LCD
 * @{
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup LCD_Exported_Types LCD Exported Types
 * @{
 */

/**
 * @brief  LCD GPIO Config definition
 */
typedef enum{
    LCD_IT_Disable = 0,
    LCD_IT_Enable  = 1
} LCD_IT_State;

/**
 * @brief  LCD GPIO Config definition
 */
typedef struct{
    GPIO_TypeDef *GPIOx;
    uint32_t      Pin;
} LCD_GPIO_Config;

/**
 * @brief LCD Init structure definition
 */
typedef struct
{
    uint32_t Pump;          /*!< Specifies the LCD PUMP on-off to be configured.
                                This parameter can be any value of @ref LCD_pump_define */

    uint32_t Bias;          /*!< Specifies the LCD bias to be configured.
                                This parameter can be any value of @ref LCD_bias_define */

    uint32_t Duty;          /*!< Specifies the LCD duty to be configured.
                                This parameter can be a value of @ref LCD_duty_define   */

    uint32_t DriveType;     /*!< Specifies the drive type to be configured.
                                This parameter can be a value of @ref LCD_drive_type_define. */
} LCD_InitTypeDef;

/**
 * @brief  LCD Handle Structure definition
 */
typedef struct
{
    LCD_TypeDef                *Instance;        /*!< Register base address               */

    LCD_InitTypeDef             Init;            /*!< LCD Init configuration              */

    const LCD_GPIO_Config      *LcdGpioArray;    /*!< LCD GPIO configuration array        */

    uint32_t                    ArrayLength;     /*!< LCD GPIO configuration array length */

    uint32_t                    LcdValtage;      /*!< LCD screen valtage, unit is millivolt */

#if (USE_HAL_LCD_REGISTER_CALLBACKS == 1)
    void (* DisplayOffCallback)(struct __LCD_HandleTypeDef *hlcd);  /*!< LCD Display off Callback */
    void (* DisplayOnCallback)(struct __LCD_HandleTypeDef *hlcd);   /*!< LCD Display on Callback */

    void (* MspInitCallback)(struct __LCD_HandleTypeDef *husart);   /*!< LCD Msp Init callback                */
    void (* MspDeInitCallback)(struct __LCD_HandleTypeDef *husart); /*!< LCD Msp DeInit callback              */
#endif

} LCD_HandleTypeDef;

#if (USE_HAL_LCD_REGISTER_CALLBACKS == 1)
/**
  * @brief  HAL LCD Callback ID enumeration definition
  */
typedef enum
{
    HAL_LCD_DOFF_CB_ID       = 0x00U,    /*!< LCD display off call back ID  */
    HAL_LCD_DON_CB_ID        = 0x01U     /*!< LCD display on callback ID    */
    HAL_LCD_MSPINIT_CB_ID    = 0x02U,    /*!< LCD MspInit callback ID       */
    HAL_LCD_MSPDEINIT_CB_ID  = 0x03U     /*!< LCD MspDeInit callback ID     */
} HAL_LCD_CallbackIDTypeDef;

/**
  * @brief  HAL LCD Callback pointer definition
  */
typedef  void (*pLCD_CallbackTypeDef)(LCD_HandleTypeDef *hlcd);  /*!< pointer to an LCD callback function */

#endif /* USE_HAL_LCD_REGISTER_CALLBACKS */

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup LCD_Exported_Constants LCD Exported Constants
 * @{
 */

/** @defgroup LCD_pump_define LCD pump define
 * @brief LCD Charge Pump on-off
 * @{
 */
#define  LCD_PUMP_ON       LL_LCD_PUMP_ON           /*!< Pump on  */
#define  LCD_PUMP_OFF      LL_LCD_PUMP_OFF          /*!< Pump off */
/**
 * @}
 */

/** @defgroup LCD_bias_define  LCD bias define
 * @brief LCD Bias Type
 * @{
 */
#define LCD_BIAS_1_3         LL_LCD_BIAS_1_3        /*!< 1/3 voltage bias */
#define LCD_BIAS_1_4         LL_LCD_BIAS_1_4        /*!< 1/4 voltage bias */
/**
 * @}
 */

/** @defgroup LCD_duty_define  LCD duty define
 * @brief LCD Output Duty Type
 * @{
 */
#define LCD_DUTY_STATIC       LL_LCD_DUTY_STATIC       /*!< Static output (COM0) */
#define LCD_DUTY_1_2          LL_LCD_DUTY_1_2          /*!< Duty cycle = 1/2 (COM0~1) */
#define LCD_DUTY_1_3          LL_LCD_DUTY_1_3          /*!< Duty cycle = 1/3 (COM0~2) */
#define LCD_DUTY_1_4          LL_LCD_DUTY_1_4          /*!< Duty cycle = 1/4 (COM0~3) */
#define LCD_DUTY_1_6          LL_LCD_DUTY_1_6          /*!< Duty cycle = 1/6 (COM0~5) */
#define LCD_DUTY_1_8          LL_LCD_DUTY_1_8          /*!< Duty cycle = 1/8 (COM0~7) */
/**
 * @}
 */

/** @defgroup LCD_drive_type_define LCD drive type define
 * @brief LCD Drive Type Select
 * @{
 */
#define LCD_DRIVE_A           LL_LCD_DRIVE_A           /*!< Drive type A */
#define LCD_DRIVE_B           LL_LCD_DRIVE_B           /*!< Drive type B */
/**
 * @}
 */

/** @defgroup LCD_offlevel_define  Output level after disable LCD
 * @brief LCD off output level
 * @{
 */
#define LCD_OFFLEVEL_V0                 LL_LCD_OFFLEVEL_V0          /*!< Off output LCDV0  */
#define LCD_OFFLEVEL_VA                 LL_LCD_OFFLEVEL_VA          /*!< Off output LCDVA  */
#define LCD_OFFLEVEL_VB                 LL_LCD_OFFLEVEL_VB          /*!< Off output LCDVB  */
#define LCD_OFFLEVEL_VC                 LL_LCD_OFFLEVEL_VC          /*!< Off output LCDVC  */
#define LCD_OFFLEVEL_VD                 LL_LCD_OFFLEVEL_VD          /*!< Off output LCDVD  */
#define LCD_OFFLEVEL_HIGHZ              LL_LCD_OFFLEVEL_HIGHZ       /*!< Off output High-Z */
/**
 * @}
 */

/** @defgroup LCD_ldos_define  LDO output level
 * @brief LCD ldo output level
 * @{
 */
#define LCD_LDOS_2P7                    LL_LCD_LDOS_2P7            /*!< Select output 2.7V  as LCDVD level */
#define LCD_LDOS_2P76                   LL_LCD_LDOS_2P76           /*!< Select output 2.76V as LCDVD level */
#define LCD_LDOS_2P82                   LL_LCD_LDOS_2P82           /*!< Select output 2.82V as LCDVD level */
#define LCD_LDOS_2P88                   LL_LCD_LDOS_2P88           /*!< Select output 2.88V as LCDVD level */
#define LCD_LDOS_2P94                   LL_LCD_LDOS_2P94           /*!< Select output 2.94V as LCDVD level */
#define LCD_LDOS_3P0                    LL_LCD_LDOS_3P0            /*!< Select output 3.0V  as LCDVD level */
#define LCD_LDOS_3P06                   LL_LCD_LDOS_3P06           /*!< Select output 3.06V as LCDVD level */
#define LCD_LDOS_3P12                   LL_LCD_LDOS_3P12           /*!< Select output 3.12V as LCDVD level */
#define LCD_LDOS_3P18                   LL_LCD_LDOS_3P18           /*!< Select output 3.18V as LCDVD level */
#define LCD_LDOS_3P24                   LL_LCD_LDOS_3P24           /*!< Select output 3.24V as LCDVD level */
#define LCD_LDOS_3P3                    LL_LCD_LDOS_3P3            /*!< Select output 3.3V  as LCDVD level */
#define LCD_LDOS_3P36                   LL_LCD_LDOS_3P36           /*!< Select output 3.36V as LCDVD level */
#define LCD_LDOS_3P42                   LL_LCD_LDOS_3P42           /*!< Select output 3.42V as LCDVD level */
#define LCD_LDOS_3P48                   LL_LCD_LDOS_3P48           /*!< Select output 3.48V as LCDVD level */
#define LCD_LDOS_3P54                   LL_LCD_LDOS_3P54           /*!< Select output 3.54V as LCDVD level */
#define LCD_LDOS_3P6                    LL_LCD_LDOS_3P6            /*!< Select output 3.6V  as LCDVD level */
/**
 * @}
 */

/** @defgroup LCD_fastcharge_define   Fast charge on-off
 * @brief Resistor Fast charge on-off
 * @{
 */
#define LCD_FASTCHARGE_EN               LL_LCD_FASTCHARGE_EN       /*!< Enable fast charge */
#define LCD_FASTCHARGE_DIS              LL_LCD_FASTCHARGE_DIS      /*!< Disable fast charge */
/**
 * @}
 */

/** @defgroup LCD_rests_define    Resistor time sharing drive on-off
 * @brief Resistor time sharing drive on-off
 * @{
 */
#define LCD_RESTS_EN                    LL_LCD_RESTS_EN           /*!< Enable resistor time sharing drive */
#define LCD_RESTS_DIS                   LL_LCD_RESTS_DIS          /*!< Disable resistor time sharing drive */
/**
 * @}
 */

/** @defgroup LCD_drivecnt_define   Resistor drive count
 * @brief Resistor drive count
 * @{
 */
#define LCD_DRIVECNT_1                  LL_LCD_DRIVECNT_1         /*!< Driving once per cycle */
#define LCD_DRIVECNT_2                  LL_LCD_DRIVECNT_2         /*!< Driving 2 times per cycle */
#define LCD_DRIVECNT_3                  LL_LCD_DRIVECNT_3         /*!< Driving 3 times per cycle */
#define LCD_DRIVECNT_4                  LL_LCD_DRIVECNT_4         /*!< Driving 4 times per cycle */
/**
 * @}
 */

/** @defgroup LCD_interres_define    Internal Resistor division signal
 * @brief Internal Resistor Division Signal
 * @{
 */
#define LCD_INTERRES_SMALL            LL_LCD_INTERRES_SMALL        /*!< Select small resistor(20k) + open circuit */
#define LCD_INTERRES_SWITCH           LL_LCD_INTERRES_SWITCH       /*!< Select time sharing switching between 20k resistor and 200k resistor */
/**
 * @}
 */

/** @defgroup LCD_resmode_define     Resistor Division Mode
 * @brief Resistor Division Mode
 * @{
 */
#define LCD_RESMODE_INTERRES           LL_LCD_RESMODE_INTERRES      /*!< Select internal resistor voltage division mode */
#define LCD_RESMODE_RESERVED           LL_LCD_RESMODE_RESERVED      /*!< Reserved, no function */
/**
 * @}
 */

/** @defgroup LCD_segopration_define    LCD segment operation
 * @brief Turn on/off the segment
 * @{
 */
#define LCD_SEGMENT_ON               (0x00000001U)               /*!< Turn on the segment */
#define LCD_SEGMENT_OFF              (0x00000000U)               /*!< Turn off the segment */
/**
 * @}
 */

/** @defgroup LCD_lcdbufbit_define    LCD_BUF Data Bit
 * @brief LCD_BUF data bit
 * @{
 */
#define LCD_BUF_BIT0                  (0x00000001U)
#define LCD_BUF_BIT1                  (0x00000002U)
#define LCD_BUF_BIT2                  (0x00000004U)
#define LCD_BUF_BIT3                  (0x00000008U)
#define LCD_BUF_BIT4                  (0x00000010U)
#define LCD_BUF_BIT5                  (0x00000020U)
#define LCD_BUF_BIT6                  (0x00000040U)
#define LCD_BUF_BIT7                  (0x00000080U)
/**
 * @}
 */

/** @defgroup LCD_buf_define    LCD_BUF number
 * @brief LCD_BUF number
 * @{
 */
#define LCD_BUF_0                      (0U)
#define LCD_BUF_1                      (1U)
#define LCD_BUF_2                      (2U)
#define LCD_BUF_3                      (3U)
#define LCD_BUF_4                      (4U)
#define LCD_BUF_5                      (5U)
#define LCD_BUF_6                      (6U)
#define LCD_BUF_7                      (7U)
#define LCD_BUF_8                      (8U)
#define LCD_BUF_9                      (9U)
#define LCD_BUF_10                     (10U)
#define LCD_BUF_11                     (11U)
#define LCD_BUF_12                     (12U)
#define LCD_BUF_13                     (13U)
#define LCD_BUF_14                     (14U)
#define LCD_BUF_15                     (15U)
#define LCD_BUF_16                     (16U)
#define LCD_BUF_17                     (17U)
#define LCD_BUF_18                     (18U)
#define LCD_BUF_19                     (19U)
#define LCD_BUF_20                     (20U)
#define LCD_BUF_21                     (21U)
#define LCD_BUF_22                     (22U)
#define LCD_BUF_23                     (23U)
#define LCD_BUF_24                     (24U)
#define LCD_BUF_25                     (25U)
#define LCD_BUF_26                     (26U)
#define LCD_BUF_27                     (27U)
#define LCD_BUF_28                     (28U)
#define LCD_BUF_29                     (29U)
#define LCD_BUF_30                     (30U)
#define LCD_BUF_31                     (31U)
#define LCD_BUF_32                     (32U)
#define LCD_BUF_33                     (33U)
#define LCD_BUF_34                     (34U)
#define LCD_BUF_35                     (35U)
#define LCD_BUF_36                     (36U)
#define LCD_BUF_37                     (37U)
/**
 * @}
 */

/** @defgroup LCD_interrupt    LCD interrupt
 * @{
 */
#define LCD_IT_DOFF                     LCD_STATUS_IRQOFFEN
#define LCD_IT_DON                      LCD_STATUS_IRQONEN
/**
 * @}
 */

/** @defgroup LCD_flag    LCD flag
 * @{
 */
#define LCD_FLAG_IT_DOFF                   LCD_STATUS_DOFF
#define LCD_FLAG_IT_DON                    LCD_STATUS_DON
#define LCD_FLAG_BUSY                   LCD_STATUS_BUSY
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup LCD_Exported_Macros LCD Exported Macros
  * @{
  */

/**
  * @brief  Enable the LCD peripheral.
  * @param  __HANDLE__  LCD handle
  * @retval None
  */
#define __HAL_LCD_ENABLE(__HANDLE__)                       SET_BIT(__HANDLE__->Instance->CTRL, LCD_CTRL_EN)

/**
  * @brief  Disable the LCD peripheral.
  * @param  __HANDLE__  LCD handle
  * @retval None
  */
#define __HAL_LCD_DISABLE(__HANDLE__)                      CLEAR_BIT(__HANDLE__->Instance->CTRL, LCD_CTRL_EN)


/** @brief  Enable the specified LCD interrupt.
  * @param  __HANDLE__ specifies the LCD Handle.
  * @param  __INTERRUPT__ specifies the LCD interrupt source to enable.
  *          This parameter can be one of the following values:
  *            @arg LCD_IT_DOFF: Display off interrupt
  *            @arg LCD_IT_DON:  Display on interrupt
  * @retval None
  */
#define __HAL_LCD_ENABLE_IT(__HANDLE__, __INTERRUPT__)     SET_BIT((__HANDLE__)->Instance->STATUS, (__INTERRUPT__))

/** @brief  Disable the specified LCD interrupt.
  * @param  __HANDLE__ specifies the LCD Handle.
  * @param  __INTERRUPT__ specifies the LCD interrupt source to disable.
  *          This parameter can be one of the following values:
  *            @arg LCD_IT_DOFF: Display off interrupt
  *            @arg LCD_IT_DON:  Display on interrupt
  * @retval None
  */
#define __HAL_LCD_DISABLE_IT(__HANDLE__, __INTERRUPT__)   CLEAR_BIT((__HANDLE__)->Instance->STATUS, (__INTERRUPT__))

/** @brief  Check whether the selected LCD interrupt has occurred or not.
  * @param  __HANDLE__ specifies the LCD Handle.
  * @param  __INTERRUPT__ specifies the LCD interrupt to check.
  *          This parameter can be one of the following values:
  *            @arg LCD_IT_DOFF: Display off interrupt
  *            @arg LCD_IT_DON:  Display on interrupt
  * @retval None
  */
#define __HAL_LCD_GET_IT(__HANDLE__, __INTERRUPT__)       (((__HANDLE__)->Instance->STATUS & (__INTERRUPT__)) == (__INTERRUPT__))

/** @brief  Check whether the specified LCD flag is set or not.
  * @param  __HANDLE__ specifies the LCD Handle.
  * @param  __FLAG__ specifies the flag to check.
  *          This parameter can be one of the following values:
  *            @arg LCD_FLAG_IT_DOFF: Set when screen changes from on to off
  *            @arg LCD_FLAG_IT_DON:  Set when screen changes from off to on
  *            @arg LCD_FLAG_BUSY: LCD busy flag, when BUSY is 1, register CTRL, CLKDIV, BLINK and PS cannot be modifed
  * @retval None
  */
#define __HAL_LCD_GET_FLAG(__HANDLE__, __FLAG__)          (((__HANDLE__)->Instance->STATUS & (__FLAG__)) == (__FLAG__))

/** @brief  Check whether the specified LCD flag is set or not, the LCD_BUSY flag is read only.
  * @param  __HANDLE__ specifies the LCD Handle.
  * @param  __FLAG__ specifies the flag to check.
  *          This parameter can be one of the following values:
  *            @arg LCD_FLAG_IT_DOFF: Set when screen changes from on to off
  *            @arg LCD_FLAG_IT_DON:  Set when screen changes from off to on
  * @retval None
  */
#define __HAL_LCD_CLEAR_FLAG(__HANDLE__, __FLAG__)        SET_BIT((__HANDLE__)->Instance->STATUS, (__FLAG__))
/**
 * @}
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @defgroup LCD_Private_Macros LCD Private Macros
 * @{
 */
#define IS_LCD_PUMP(PUMP)              (((PUMP) == LCD_PUMP_ON ) || ((PUMP) == LCD_PUMP_OFF))

#define IS_LCD_BIAS(BIAS)              (((BIAS) == LCD_BIAS_1_3) || ((BIAS) == LCD_BIAS_1_4))

#define IS_LCD_DUTY(DUTY)              (((DUTY) == LCD_DUTY_STATIC)   ||\
                                        ((DUTY) == LCD_DUTY_1_2   )   ||\
                                        ((DUTY) == LCD_DUTY_1_3   )   ||\
                                        ((DUTY) == LCD_DUTY_1_4   )   ||\
                                        ((DUTY) == LCD_DUTY_1_6   )   ||\
                                        ((DUTY) == LCD_DUTY_1_8   ))

#define IS_LCD_DRIVE_TYPE(DRIVE_TYPE)  (((DRIVE_TYPE) == LCD_DRIVE_A) || ((DRIVE_TYPE) == LCD_DRIVE_B))
/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @addtogroup LCD_Exported_Functions
 * @{
 */

/** @addtogroup LCD_Exported_Functions_Group1
 * @{
 */
/* Initialization and de-initialization functions ****************************/
HAL_StatusTypeDef HAL_LCD_Init(LCD_HandleTypeDef *hlcd);
HAL_StatusTypeDef HAL_LCD_DeInit(LCD_HandleTypeDef *hlcd);
void HAL_LCD_MspInit(LCD_HandleTypeDef *hlcd);
void HAL_LCD_MspDeInit(LCD_HandleTypeDef *hlcd);

/* Callbacks Register/UnRegister functions  ***********************************/
#if (USE_HAL_LCD_REGISTER_CALLBACKS == 1)
HAL_StatusTypeDef HAL_LCD_RegisterCallback(LCD_HandleTypeDef *hlcd, HAL_LCD_CallbackIDTypeDef CallbackID,
                                             pLCD_CallbackTypeDef pCallback);
HAL_StatusTypeDef HAL_LCD_UnRegisterCallback(LCD_HandleTypeDef *hlcd, HAL_LCD_CallbackIDTypeDef CallbackID);
#endif

/**
 * @}
 */

/** @addtogroup LCD_Exported_Functions_Group2
 * @{
 */
/* Screen operation functions *************************************************/
void HAL_LCD_ClearScreen(uint8_t *DisplayBuf, uint32_t BufSize);
void HAL_LCD_DisplayAll(uint8_t *DisplayBuf, uint32_t BufSize);
void HAL_LCD_RefreshScreen(uint8_t *DisplayBuf, uint32_t BufSize);
void HAL_LCD_ClearDisplayBuf(uint8_t *DisplayBuf, uint32_t BufSize);
void HAL_LCD_SetOneChar(uint8_t *DisplayBuf, uint16_t LcdCharMap, uint8_t TurnOn);
void HAL_LCD_SetOneDigit(uint8_t *DisplayBuf, const uint16_t DigitalTable[][7], uint8_t DigitalPos, uint8_t DisplayNumber);
/**
 * @}
 */

/** @addtogroup LCD_Exported_Functions_Group3
 * @{
 */
/* Blink configuration functions ************************************************************/
void HAL_LCD_SetBlinkTime(LCD_HandleTypeDef *hlcd, uint32_t Time);
void HAL_LCD_SetTurnOffTime(LCD_HandleTypeDef *hlcd, uint32_t Time);
void HAL_LCD_SetTurnOnTime(LCD_HandleTypeDef *hlcd, uint32_t Time);
void HAL_LCD_SetPumpSetupTime(LCD_HandleTypeDef *hlcd, uint32_t Time);
/**
 * @}
 */

/** @addtogroup LCD_Exported_Functions_Group4
 * @{
 */
/* Interrupt handle functions ************************************************************/
void HAL_LCD_IRQHandler(LCD_HandleTypeDef *hlcd);
void HAL_LCD_DisplayOffCallback(LCD_HandleTypeDef *hlcd);
void HAL_LCD_DisplayOnCallback(LCD_HandleTypeDef *hlcd);
/**
 * @}
 */

/**
 * @}
 */

/* Private functions --------------------------------------------------------*/
/** @defgroup LCD_Private_Functions LCD Private Functions
 * @{
 */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_HAL_LCD_H */

