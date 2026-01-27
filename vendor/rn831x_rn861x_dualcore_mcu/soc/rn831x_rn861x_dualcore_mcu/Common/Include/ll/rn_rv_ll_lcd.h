/**
 *****************************************************************************
 * @file rn_rv_ll_lcd.h
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

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RN_RV_LL_LCD_H
#define __RN_RV_LL_LCD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_lcd.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined(LCD)

/** @defgroup LCD_LL LCD
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/** @defgroup LCD_LL_Private_Constants LCD Private Constants
 * @{
 */

/** @defgroup LCD_LL_PC_DBG_PASSWORD  LCD Debug Password
 * @{
 */
#define LL_LCD_DBG_PASSWORD                0xED3A0000U  /*!< DBG write access password  */
/**
 * @}
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup LCD_LL_Private_Macros LCD Private Macros
 * @{
 */

/** @defgroup LCD_LL_PM_WAITIDLE  Wait for write idle
 * @{
 */
#define LL_LCD_WAITIDLE(_INSTANCE_)            while(LCD_STATUS_BUSY == (READ_BIT(_INSTANCE_->STATUS, LCD_STATUS_BUSY)))
/**
 * @}
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup LCD_LL_Exported_Types LCD Exported Types
 * @{
 */

/**
 * @brief LL LCD Init Structure definition
 * @{
 */
typedef struct
{
    uint32_t Pump;          /*!< Specifies the LCD PUMP on-off to be configured.
                                This parameter can be any value of @ref LCD_LL_EC_PUMP */

    uint32_t Bias;          /*!< Specifies the LCD bias to be configured.
                                This parameter can be any value of @ref LCD_LL_EC_BIAS */

    uint32_t Duty;          /*!< Specifies the LCD duty to be configured.
                                This parameter can be a value of @ref LCD_LL_EC_DUTY   */

    uint32_t DriveType;     /*!< Specifies the drive type to be configured.
                                This parameter can be a value of @ref LCD_LL_EC_DRIVE_TYPE. */

    uint32_t Division;        /*!< Specifies the LCD clock division.  */
} LL_LCD_InitTypeDef;
/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup LCD_LL_Exported_Constants LCD Exported Constants
 * @{
 */

/** @defgroup LCD_LL_EC_AF  alternate function of SEG or COM
 * @{
 */
#define LL_LCD_AF_SEG                    (0x0000000U)  /*!< Select IO as SEG */
#define LL_LCD_AF_COM                    (0x0000001U)  /*!< Select IO as COM */
/**
 * @}
 */

/** @defgroup LCD_LL_EC_DRIVE_TYPE    Drive Type
 * @{
 */
#define LL_LCD_DRIVE_A                   LCD_CTRL_TYPE_A  /*!< Select drive type A */
#define LL_LCD_DRIVE_B                   LCD_CTRL_TYPE_B  /*!< Select drive type B */
/**
 * @}
 */

/** @defgroup LCD_LL_EC_PUMP    Pump on-off
 * @{
 */
#define LL_LCD_PUMP_ON                   LCD_CTRL_PWD_PUMP_EN   /*!< Pump on */
#define LL_LCD_PUMP_OFF                  LCD_CTRL_PWD_PUMP_DIS  /*!< Pump off, use resistance voltage disision */
/**
 * @}
 */

/** @defgroup LCD_LL_EC_BIAS    Bias Type
 * @{
 */
#define LL_LCD_BIAS_1_3                  LCD_CTRL_BIAS_1_3  /*!< Select LCD bias as 1/3 voltage bias */
#define LL_LCD_BIAS_1_4                  LCD_CTRL_BIAS_1_4  /*!< Select LCD bias as 1/4 voltage bias */
/**
 * @}
 */

/** @defgroup LCD_LL_EC_DUTY    Duty Type
 * @{
 */
#define LL_LCD_DUTY_STATIC                LCD_CTRL_DUTY_STA       /*!< Static output (COM0) */
#define LL_LCD_DUTY_1_2                   LCD_CTRL_DUTY_1_2       /*!< Duty cycle = 1/2 (COM0~1) */
#define LL_LCD_DUTY_1_3                   LCD_CTRL_DUTY_1_3       /*!< Duty cycle = 1/3 (COM0~2) */
#define LL_LCD_DUTY_1_4                   LCD_CTRL_DUTY_1_4       /*!< Duty cycle = 1/4 (COM0~3) */
#define LL_LCD_DUTY_1_6                   LCD_CTRL_DUTY_1_6       /*!< Duty cycle = 1/6 (COM0~5) */
#define LL_LCD_DUTY_1_8                   LCD_CTRL_DUTY_1_8       /*!< Duty cycle = 1/8 (COM0~7) */
/**
 * @}
 */

/** @defgroup LCD_LL_EC_OFFLEVEL     Output level after disable LCD
 * @{
 */
#define LL_LCD_OFFLEVEL_V0                 LCD_RESCTL_OFF_LVL_V0          /*!< Off output LCDV0  */
#define LL_LCD_OFFLEVEL_VA                 LCD_RESCTL_OFF_LVL_VA          /*!< Off output LCDVA  */
#define LL_LCD_OFFLEVEL_VB                 LCD_RESCTL_OFF_LVL_VB          /*!< Off output LCDVB  */
#define LL_LCD_OFFLEVEL_VC                 LCD_RESCTL_OFF_LVL_VC          /*!< Off output LCDVC  */
#define LL_LCD_OFFLEVEL_VD                 LCD_RESCTL_OFF_LVL_VD          /*!< Off output LCDVD  */
#define LL_LCD_OFFLEVEL_HIGHZ              LCD_RESCTL_OFF_LVL_HIGHZ       /*!< Off output High-Z */
/**
 * @}
 */

/** @defgroup LCD_LL_EC_LDOS        LDO output level
 * @{
 */
#define LL_LCD_LDOS_2P7                    (0U)            /*!< Select output 2.7V  as LCDVD level */
#define LL_LCD_LDOS_2P76                   (1U)           /*!< Select output 2.76V as LCDVD level */
#define LL_LCD_LDOS_2P82                   (2U)           /*!< Select output 2.82V as LCDVD level */
#define LL_LCD_LDOS_2P88                   (3U)           /*!< Select output 2.88V as LCDVD level */
#define LL_LCD_LDOS_2P94                   (4U)           /*!< Select output 2.94V as LCDVD level */
#define LL_LCD_LDOS_3P0                    (5U)            /*!< Select output 3.0V  as LCDVD level */
#define LL_LCD_LDOS_3P06                   (6U)           /*!< Select output 3.06V as LCDVD level */
#define LL_LCD_LDOS_3P12                   (7U)           /*!< Select output 3.12V as LCDVD level */
#define LL_LCD_LDOS_3P18                   (8U)           /*!< Select output 3.18V as LCDVD level */
#define LL_LCD_LDOS_3P24                   (9U)           /*!< Select output 3.24V as LCDVD level */
#define LL_LCD_LDOS_3P3                    (10U)            /*!< Select output 3.3V  as LCDVD level */
#define LL_LCD_LDOS_3P36                   (11U)           /*!< Select output 3.36V as LCDVD level */
#define LL_LCD_LDOS_3P42                   (12U)           /*!< Select output 3.42V as LCDVD level */
#define LL_LCD_LDOS_3P48                   (13U)           /*!< Select output 3.48V as LCDVD level */
#define LL_LCD_LDOS_3P54                   (14U)           /*!< Select output 3.54V as LCDVD level */
#define LL_LCD_LDOS_3P6                    (15U)            /*!< Select output 3.6V  as LCDVD level */
/**
 * @}
 */

/** @defgroup LCD_LL_EC_FASTCHARGE    Fast charge on-off
 * @{
 */
#define LL_LCD_FASTCHARGE_EN               (0x00000001U)                  /*!< Enable fast charge */
#define LL_LCD_FASTCHARGE_DIS              (0x00000000U)                  /*!< Disable fast charge */
/**
 * @}
 */

/** @defgroup LCD_LL_EC_RESTS    Resistor time-division drive on-off
 * @{
 */
#define LL_LCD_RESTS_EN                    (0x00000000U)                 /*!< Enable resistor time-division drive */
#define LL_LCD_RESTS_DIS                   (0x00000001U)                 /*!< Disable resistor time-division drive */
/**
 * @}
 */

/** @defgroup LCD_LL_EC_DRIVECNT   Resistor drive count
 * @{
 */
#define LL_LCD_DRIVECNT_1                  LCD_RESCTL_RES_FT_1            /*!< Driving once per cycle */
#define LL_LCD_DRIVECNT_2                  LCD_RESCTL_RES_FT_2            /*!< Driving 2 times per cycle */
#define LL_LCD_DRIVECNT_3                  LCD_RESCTL_RES_FT_3            /*!< Driving 3 times per cycle */
#define LL_LCD_DRIVECNT_4                  LCD_RESCTL_RES_FT_4            /*!< Driving 4 times per cycle */
/**
 * @}
 */

/** @defgroup LCD_LL_EC_INTERRES    Internal Resistor voltage division signal
 * @{
 */
#define LL_LCD_INTERRES_SMALL               LCD_RESCTL_RSM_SMALL          /*!< Select small resistor(20k) + open circuit */
#define LL_LCD_INTERRES_SWITCH              LCD_RESCTL_RSM_SWITCH         /*!< Select time-division switching between 20k resistor and 200k resistor */
/**
 * @}
 */

/** @defgroup LCD_LL_EC_RESMODE      Resistor voltage division Mode
 * @{
 */
#define LL_LCD_RESMODE_INTERRES             LCD_RESCTL_IRSN_INTERRES      /*!< Select internal resistor voltage division */
#define LL_LCD_RESMODE_RESERVED             LCD_RESCTL_IRSN_RESERVED      /*!< Reserved */
/**
 * @}
 */

/** @defgroup LCD_LL_EC_RESDRIVE      Resistor driving type
 * @{
 */
#define LL_LCD_RESDRIVE_TIMEDIV             LCD_RESCTL_RES_AO_TIMEDIV      /*!< Select resistors time-division driving */
#define LL_LCD_RESDRIVE_ALWAYS              LCD_RESCTL_RES_AO_ALWAYS       /*!< Select resistors always drive */
/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @defgroup LCD_LL_Exported_Functions LCD Exported Functions
 * @{
 */

/** @defgroup LCD_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief Configure SEG3 or COM7 alternate function.
 * @note  Warning: When LCD_BUSY flag is set to 1, this regitser bit can not be configured.
 * @rmtoll CTRL        AF3         LL_LCD_SetAF3
 * @param LCDx LCD instance
 * @param Alternate This parameter can be one of the following values:
 *        @arg @ref LL_LCD_AF_SEG
 *        @arg @ref LL_LCD_AF_COM
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetAF3(LCD_TypeDef *LCDx, uint32_t Alternate)
{
    LL_LCD_WAITIDLE(LCDx);
    MODIFY_REG(LCDx->CTRL, LCD_CTRL_AF3, (Alternate << LCD_CTRL_AF3_POS));
    LL_LCD_WAITIDLE(LCDx);
}

/**
 * @brief  Return SEG3 or COM7 alternate function.
 * @rmtoll CTRL        AF3         LL_LCD_GetAF3
 * @param  LCDx LCD instance
 * @retval Returned value can be one of the following values:
 *        @arg @ref LL_LCD_AF_SEG
 *        @arg @ref LL_LCD_AF_COM
 */
__STATIC_INLINE uint32_t LL_LCD_GetAF3(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->CTRL, LCD_CTRL_AF3) >> LCD_CTRL_AF3_POS);
}

/**
 * @brief Configure SEG2 or COM6 alternate function.
 * @note  Warning: When LCD_BUSY flag is set to 1, this regitser bit can not be configured.
 * @rmtoll CTRL        AF2         LL_LCD_SetAF2
 * @param LCDx LCD instance
 * @param Alternate This parameter can be one of the following values:
 *        @arg @ref LL_LCD_AF_SEG
 *        @arg @ref LL_LCD_AF_COM
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetAF2(LCD_TypeDef *LCDx, uint32_t Alternate)
{
    LL_LCD_WAITIDLE(LCDx);
    MODIFY_REG(LCDx->CTRL, LCD_CTRL_AF2, (Alternate << LCD_CTRL_AF2_POS));
    LL_LCD_WAITIDLE(LCDx);
}

/**
 * @brief  Return SEG2 or COM6 alternate function.
 * @rmtoll CTRL        AF2         LL_LCD_GetAF2
 * @param  LCDx LCD instance
 * @retval Returned value can be one of the following values:
 *        @arg @ref LL_LCD_AF_SEG
 *        @arg @ref LL_LCD_AF_COM
 */
__STATIC_INLINE uint32_t LL_LCD_GetAF2(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->CTRL, LCD_CTRL_AF2) >> LCD_CTRL_AF2_POS);
}

/**
 * @brief Configure SEG1 or COM5 alternate function.
 * @note  Warning: When LCD_BUSY flag is set to 1, this regitser bit can not be configured.
 * @rmtoll CTRL        AF1         LL_LCD_SetAF1
 * @param LCDx LCD instance
 * @param Alternate This parameter can be one of the following values:
 *        @arg @ref LL_LCD_AF_SEG
 *        @arg @ref LL_LCD_AF_COM
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetAF1(LCD_TypeDef *LCDx, uint32_t Alternate)
{
    LL_LCD_WAITIDLE(LCDx);
    MODIFY_REG(LCDx->CTRL, LCD_CTRL_AF1, (Alternate << LCD_CTRL_AF1_POS));
    LL_LCD_WAITIDLE(LCDx);
}

/**
 * @brief  Return SEG1 or COM5 alternate function.
 * @rmtoll CTRL        AF1         LL_LCD_GetAF1
 * @param  LCDx LCD instance
 * @retval Returned value can be one of the following values:
 *        @arg @ref LL_LCD_AF_SEG
 *        @arg @ref LL_LCD_AF_COM
 */
__STATIC_INLINE uint32_t LL_LCD_GetAF1(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->CTRL, LCD_CTRL_AF1) >> LCD_CTRL_AF1_POS);
}

/**
 * @brief Configure SEG0 or COM4 alternate function.
 * @note  Warning: When LCD_BUSY flag is set to 1, this regitser bit can not be configured.
 * @rmtoll CTRL        AF0         LL_LCD_SetAF0
 * @param LCDx LCD instance
 * @param Alternate This parameter can be one of the following values:
 *        @arg @ref LL_LCD_AF_SEG
 *        @arg @ref LL_LCD_AF_COM
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetAF0(LCD_TypeDef *LCDx, uint32_t Alternate)
{
    LL_LCD_WAITIDLE(LCDx);
    MODIFY_REG(LCDx->CTRL, LCD_CTRL_AF0, (Alternate << LCD_CTRL_AF0_POS));
    LL_LCD_WAITIDLE(LCDx);
}

/**
 * @brief  Return SEG0 or COM4 alternate function.
 * @rmtoll CTRL        AF0         LL_LCD_GetAF0
 * @param  LCDx LCD instance
 * @retval Returned value can be one of the following values:
 *        @arg @ref LL_LCD_AF_SEG
 *        @arg @ref LL_LCD_AF_COM
 */
__STATIC_INLINE uint32_t LL_LCD_GetAF0(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->CTRL, LCD_CTRL_AF0) >> LCD_CTRL_AF0_POS);
}

/**
 * @brief Set PUMP on-off.
 * @note  PUMP on: LCD voltage generated by internal PUMP.
 * @note  PUMP off: LCD voltage generated by dividing resistors.
 * @rmtoll CTRL         PWD_PUMP           LL_LCD_SetPumpOnOff
 * @param  LCDx LCD instance
 * @param  PumpOnOff This parameter can be one of the following values:
 *        @arg @ref LL_LCD_PUMP_ON
 *        @arg @ref LL_LCD_PUMP_OFF
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetPumpOnOff(LCD_TypeDef *LCDx, uint32_t PumpOnOff)
{
    LL_LCD_WAITIDLE(LCDx);
    MODIFY_REG(LCDx->CTRL, LCD_CTRL_PWD_PUMP, PumpOnOff);
    LL_LCD_WAITIDLE(LCDx);
}

/**
 * @brief Get PUMP on-off.
 * @note  PUMP on: LCD voltage generated by internal PUMP.
 * @note  PUMP off: LCD voltage generated by dividing resistors.
 * @rmtoll CTRL         PWD_PUMP           LL_LCD_GetPumpOnOff
 * @param  LCDx LCD instance
 * @retval Returned value can be one of the following values:
 *        @arg @ref LL_LCD_PUMP_ON
 *        @arg @ref LL_LCD_PUMP_OFF
 */
__STATIC_INLINE uint32_t LL_LCD_GetPumpOnOff(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->CTRL, LCD_CTRL_PWD_PUMP));
}

/**
 * @brief Configure LCD drive type.
 * @rmtoll CTRL        TYPE         LL_LCD_SetDriveType
 * @param LCDx LCD instance
 * @param Type This parameter can be one of the following values:
 *        @arg @ref LL_LCD_DRIVE_A
 *        @arg @ref LL_LCD_DRIVE_B
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetDriveType(LCD_TypeDef *LCDx, uint32_t Type)
{
    LL_LCD_WAITIDLE(LCDx);
    MODIFY_REG(LCDx->CTRL, LCD_CTRL_TYPE, Type);
    LL_LCD_WAITIDLE(LCDx);
}

/**
 * @brief  Return LCD drive type.
 * @rmtoll CTRL        TYPE         LL_LCD_GetDriveType
 * @param  LCDx LCD instance
 * @retval Returned value can be one of the following values:
 *        @arg @ref LL_LCD_DRIVE_A
 *        @arg @ref LL_LCD_DRIVE_B
 */
__STATIC_INLINE uint32_t LL_LCD_GetDriveType(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->CTRL, LCD_CTRL_TYPE));
}

/**
 * @brief Configure LCD bias level of charge pump to adjust contrast.
 * @note  Warning: When LCD_BUSY flag is set to 1, this regitser bit can not be configured.
 * @note   Refer to the bias voltage calculation formula in the datasheet.
 * @rmtoll CTRL        BIASLVL         LL_LCD_SetPumpBiasLevel
 * @param LCDx LCD instance
 * @param Level Output bias level, 6 bits
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetPumpBiasLevel(LCD_TypeDef *LCDx, uint32_t Level)
{
    LL_LCD_WAITIDLE(LCDx);
    MODIFY_REG(LCDx->CTRL, LCD_CTRL_BIASLVL, (Level << LCD_CTRL_BIASLVL_POS));
    LL_LCD_WAITIDLE(LCDx);
}

/**
 * @brief  Return LCD bias level of charge pump.
 * @note   Refer to the bias voltage calculation formula in the datasheet.
 * @rmtoll CTRL        BIASLVL         LL_LCD_GetPumpBiasLevel
 * @param  LCDx LCD instance
 * @retval Bias level of charge pump
 */
__STATIC_INLINE uint32_t LL_LCD_GetPumpBiasLevel(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->CTRL, LCD_CTRL_BIASLVL) >> LCD_CTRL_BIASLVL_POS);
}

/**
 * @brief Configure LCD bias.
 * @note  Warning: When LCD_BUSY flag is set to 1, this regitser bit can not be configured.
 * @rmtoll CTRL        BIAS         LL_LCD_SetBias
 * @param LCDx LCD instance
 * @param Bias This parameter can be one of the following values:
 *        @arg @ref LL_LCD_BIAS_1_3
 *        @arg @ref LL_LCD_BIAS_1_4
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetBias(LCD_TypeDef *LCDx, uint32_t Bias)
{
    LL_LCD_WAITIDLE(LCDx);
    MODIFY_REG(LCDx->CTRL, LCD_CTRL_BIAS, Bias);
    LL_LCD_WAITIDLE(LCDx);
}

/**
 * @brief  Return LCD bias.
 * @rmtoll CTRL        BIAS         LL_LCD_GetBias
 * @param  LCDx LCD instance
 * @retval Returned value can be one of the following values:
 *        @arg @ref LL_LCD_BIAS_1_3
 *        @arg @ref LL_LCD_BIAS_1_4
 */
__STATIC_INLINE uint32_t LL_LCD_GetBias(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->CTRL, LCD_CTRL_BIAS));
}

/**
 * @brief Configure LCD duty.
 * @note  Warning: When LCD_BUSY flag is set to 1, this regitser bit can not be configured.
 * @rmtoll CTRL        DUTY         LL_LCD_SetDuty
 * @param LCDx LCD instance
 * @param Duty This parameter can be one of the following values:
 *        @arg @ref LL_LCD_DUTY_STATIC
 *        @arg @ref LL_LCD_DUTY_1_2
 *        @arg @ref LL_LCD_DUTY_1_3
 *        @arg @ref LL_LCD_DUTY_1_4
 *        @arg @ref LL_LCD_DUTY_1_6
 *        @arg @ref LL_LCD_DUTY_1_8
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetDuty(LCD_TypeDef *LCDx, uint32_t Duty)
{
    LL_LCD_WAITIDLE(LCDx);
    MODIFY_REG(LCDx->CTRL, LCD_CTRL_DUTY, Duty);
    LL_LCD_WAITIDLE(LCDx);
}

/**
 * @brief  Return LCD duty.
 * @rmtoll CTRL        DUTY         LL_LCD_GetDuty
 * @param  LCDx LCD instance
 * @retval Returned value can be one of the following values:
 *        @arg @ref LL_LCD_DUTY_STATIC
 *        @arg @ref LL_LCD_DUTY_1_2
 *        @arg @ref LL_LCD_DUTY_1_3
 *        @arg @ref LL_LCD_DUTY_1_4
 *        @arg @ref LL_LCD_DUTY_1_6
 *        @arg @ref LL_LCD_DUTY_1_8
 */
__STATIC_INLINE uint32_t LL_LCD_GetDuty(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->CTRL, LCD_CTRL_DUTY));
}

/**
 * @brief  Enable LCD.
 * @note   Bit 0 : disable, bit 1 :enable
 * @rmtoll CTRL         EN           LL_LCD_Enable
 * @param  LCDx LCD instance
 * @return None
 */
__STATIC_INLINE void LL_LCD_Enable(LCD_TypeDef *LCDx)
{
    SET_BIT(LCDx->CTRL, LCD_CTRL_EN);
}

/**
 * @brief  Disable LCD.
 * @note   Bit 0 : disable, bit 1 :enable
 * @rmtoll CTRL         EN           LL_LCD_Disable
 * @param  LCDx LCD instance
 * @return None
 */
__STATIC_INLINE void LL_LCD_Disable(LCD_TypeDef *LCDx)
{
    CLEAR_BIT(LCDx->CTRL, LCD_CTRL_EN);
}

/**
 * @brief  Return if LCD is enable or disable.
 * @rmtoll CTRL         EN           LL_LCD_IsEnabled
 * @param  LCDx LCD instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LCD_IsEnabled(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->CTRL, LCD_CTRL_EN) == LCD_CTRL_EN);
}

/**
 * @brief Indicates whether the busy flag is set.
 * @note  If the busy flag is set to 1, the CTRL(Except EN and TYPE bit), CLKDIV, BLINK and PS registers cannot be writed.
 *        They can only be written to when the busy flag is 0.
 * @rmtoll STATUS   BUSY   LL_LCD_IsActiveFlag_BUSY
 * @param LCDx    LCD Instance
 * @return State of bit, 1: busy, 0: idle.
 */
__STATIC_INLINE uint32_t LL_LCD_IsActiveFlag_BUSY(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->STATUS, LCD_STATUS_BUSY));
}

/**
 * @brief  Enable display off interrupt.
 * @note   Bit 0 : disable, bit 1 :enable
 * @rmtoll STATUS   IRQOFFEN    LL_LCD_EnableIT_DOFF
 * @param  LCDx LCD instance
 * @return None
 */
__STATIC_INLINE void LL_LCD_EnableIT_DOFF(LCD_TypeDef *LCDx)
{
    SET_BIT(LCDx->STATUS, LCD_STATUS_IRQOFFEN);
}

/**
 * @brief  Disable display off interrupt.
 * @note   Bit 0 : disable, bit 1 :enable
 * @rmtoll STATUS   IRQOFFEN     LL_LCD_DisableIT_DOFF
 * @param  LCDx LCD instance
 * @return None
 */
__STATIC_INLINE void LL_LCD_DisableIT_DOFF(LCD_TypeDef *LCDx)
{
    CLEAR_BIT(LCDx->STATUS, LCD_STATUS_IRQOFFEN);
}

/**
 * @brief  Return if display off interrupt is enable or disable.
 * @rmtoll STATUS   IRQOFFEN      LL_LCD_IsEnabledIT_DOFF
 * @param  LCDx LCD instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LCD_IsEnabledIT_DOFF(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->STATUS, LCD_STATUS_IRQOFFEN) == LCD_STATUS_IRQOFFEN);
}

/**
 * @brief  Enable display on interrupt.
 * @note   Bit 0 : disable, bit 1 : enable
 * @rmtoll STATUS   IRQONEN    LL_LCD_EnableIT_DON
 * @param  LCDx LCD instance
 * @return None
 */
__STATIC_INLINE void LL_LCD_EnableIT_DON(LCD_TypeDef *LCDx)
{
    SET_BIT(LCDx->STATUS, LCD_STATUS_IRQONEN);
}

/**
 * @brief  Disable display on interrupt.
 * @note   Bit 0 : disable, bit 1 : enable
 * @rmtoll STATUS   IRQONEN     LL_LCD_DisableIT_DON
 * @param  LCDx LCD instance
 * @return None
 */
__STATIC_INLINE void LL_LCD_DisableIT_DON(LCD_TypeDef *LCDx)
{
    CLEAR_BIT(LCDx->STATUS, LCD_STATUS_IRQONEN);
}

/**
 * @brief  Return if display on interrupt is enable or disable.
 * @rmtoll STATUS   IRQONEN      LL_LCD_IsEnabledIT_DON
 * @param  LCDx LCD instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LCD_IsEnabledIT_DON(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->STATUS, LCD_STATUS_IRQONEN) == LCD_STATUS_IRQONEN);
}

/**
 * @brief Indicates whether the display off interrupt flag is set.
 * @rmtoll STATUS   DOFF   LL_LCD_IsActiveFlag_DOFF
 * @param LCDx    LCD Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_LCD_IsActiveFlag_DOFF(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->STATUS, LCD_STATUS_DOFF));
}

/**
 * @brief Clear the display off interrupt and flag.
 * @rmtoll STATUS   DOFF   LL_LCD_ClearFlag_DOFF
 * @param LCDx    LCD Instance
 */
__STATIC_INLINE void LL_LCD_ClearFlag_DOFF(LCD_TypeDef *LCDx)
{
    SET_BIT(LCDx->STATUS, LCD_STATUS_DOFF);
}

/**
 * @brief Indicates whether the display on interrupt flag is set.
 * @rmtoll STATUS   DON   LL_LCD_IsActiveFlag_DON
 * @param LCDx    LCD Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_LCD_IsActiveFlag_DON(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->STATUS, LCD_STATUS_DON));
}

/**
 * @brief Clear the display on interrupt and flag.
 * @rmtoll STATUS   DON   LL_LCD_ClearFlag_DON
 * @param LCDx    LCD Instance
 */
__STATIC_INLINE void LL_LCD_ClearFlag_DON(LCD_TypeDef *LCDx)
{
    SET_BIT(LCDx->STATUS, LCD_STATUS_DON);
}

/**
 * @brief Configure LCD clock division.
 * @note  Warning: When LCD_BUSY flag is set to 1, this regitser can not be configured.
 * @rmtoll CLKDIV    CLKDIV    LL_LCD_SetClockDivision
 * @param LCDx LCD instance
 * @param Division LCD clock division, 8bit, 0<=Division<=255
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetClockDivision(LCD_TypeDef *LCDx, uint32_t Division)
{
    LL_LCD_WAITIDLE(LCDx);
    WRITE_REG(LCDx->CLKDIV, Division);
    LL_LCD_WAITIDLE(LCDx);
}

/**
 * @brief  Return LCD clock division.
 * @rmtoll CLKDIV    CLKDIV    LL_LCD_GetClockDivision
 * @param  LCDx LCD instance
 * @retval Clock division
 */
__STATIC_INLINE uint32_t LL_LCD_GetClockDivision(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->CLKDIV, LCD_CLKDIV_CLKDIV));
}

/**
 * @brief Configure LCD blink time.
 * @note  Warning: When LCD_BUSY flag is set to 1, this regitser bit can not be configured.
 * @note  Config step is 0.25S(0.25*Time), support time 0~63.75S (0<=Time<=255).
 * @note  The time for turn on and off is 0.25* BLINK_TIME during one TON display cycle.
 * @note  If BLINK_TIME is 0, LCD will remain on during the TON cycle without blink. If BLINK_TIME > 0,
 *        TON must be BLINK_TIME*2n.
 * @rmtoll BLINK    BLINK_TIME    LL_LCD_SetBlinkTime
 * @param LCDx LCD instance
 * @param Time Blink time
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetBlinkTime(LCD_TypeDef *LCDx, uint32_t Time)
{
    LL_LCD_WAITIDLE(LCDx);
    MODIFY_REG(LCDx->BLINK, LCD_BLINK_BLINK_TIME, (Time << LCD_BLINK_BLINK_TIME_POS));
    LL_LCD_WAITIDLE(LCDx);
}

/**
 * @brief  Return LCD blink time.
 * @rmtoll BLINK    BLINK_TIME    LL_LCD_GetBlinkTime
 * @param  LCDx LCD instance
 * @retval Blink time
 */
__STATIC_INLINE uint32_t LL_LCD_GetBlinkTime(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->BLINK, LCD_BLINK_BLINK_TIME) >> LCD_BLINK_BLINK_TIME_POS);
}

/**
 * @brief Configure time of display off.
 * @note  Warning: When LCD_BUSY flag is set to 1, this regitser bit can not be configured.
 * @note  Config step is 0.25S(0.25*Time), support time 0~127.5S (0<=Time<=511). Please set to be greater than 3S.
 * @rmtoll BLINK    TOFF    LL_LCD_SetTimeOff
 * @param LCDx LCD instance
 * @param Time Display off time
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetTimeOff(LCD_TypeDef *LCDx, uint32_t Time)
{
    LL_LCD_WAITIDLE(LCDx);
    MODIFY_REG(LCDx->BLINK, LCD_BLINK_TOFF, (Time << LCD_BLINK_TOFF_POS));
    LL_LCD_WAITIDLE(LCDx);
}

/**
 * @brief  Return time of display off.
 * @rmtoll BLINK    TOFF    LL_LCD_GetTimeOff
 * @param  LCDx LCD instance
 * @retval Display off time
 */
__STATIC_INLINE uint32_t LL_LCD_GetTimeOff(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->BLINK, LCD_BLINK_TOFF) >> LCD_BLINK_TOFF_POS);
}

/**
 * @brief Configure time of display on.
 * @note  Warning: When LCD_BUSY flag is set to 1, this regitser bit can not be configured.
 * @note  Config step is 0.25S(0.25*Time), support time 0~127.5S. Please set to be greater than 3S.
 * @rmtoll BLINK    TON    LL_LCD_SetTimeOn
 * @param LCDx LCD instance
 * @param Time Display on time
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetTimeOn(LCD_TypeDef *LCDx, uint32_t Time)
{
    LL_LCD_WAITIDLE(LCDx);
    MODIFY_REG(LCDx->BLINK, LCD_BLINK_TON, Time);
    LL_LCD_WAITIDLE(LCDx);
}

/**
 * @brief  Return time of display on.
 * @rmtoll BLINK    TON    LL_LCD_GetTimeOn
 * @param  LCDx LCD instance
 * @retval Display on time
 */
__STATIC_INLINE uint32_t LL_LCD_GetTimeOn(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->BLINK, LCD_BLINK_TON));
}

/**
 * @brief Configure LCD pump setup time.
 * @note  Warning: When LCD_BUSY flag is set to 1, this regitser can not be configured.
 * @note  Setup time = 30.5us * (PS+4), 0<=time<=65535.
 * @rmtoll PS    PS    LL_LCD_SetPumpSetupTime
 * @param LCDx LCD instance
 * @param Time Pump setup time
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetPumpSetupTime(LCD_TypeDef *LCDx, uint32_t Time)
{
    LL_LCD_WAITIDLE(LCDx);
    WRITE_REG(LCDx->PS, (Time & LCD_PS_PS));
    LL_LCD_WAITIDLE(LCDx);
}

/**
 * @brief  Return LCD pump setup time.
 * @note   Setup time = 30.5us * (PS+4), default 100ms.
 * @rmtoll PS    PS    LL_LCD_GetPumpSetupTime
 * @param  LCDx LCD instance
 * @retval Pump setup time
 */
__STATIC_INLINE uint32_t LL_LCD_GetPumpSetupTime(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->PS, LCD_PS_PS));
}

/**
 * @brief Configure output voltage when disable LCD.
 * @rmtoll RESCTL    OFF_LVL    LL_LCD_SetOffOutputLevel
 * @param LCDx  LCD instance
 * @param Level This parameter can be one of the following values:
 *        @arg @ref LL_LCD_OFFLEVEL_V0
 *        @arg @ref LL_LCD_OFFLEVEL_VA
 *        @arg @ref LL_LCD_OFFLEVEL_VB
 *        @arg @ref LL_LCD_OFFLEVEL_VC
 *        @arg @ref LL_LCD_OFFLEVEL_VD
 *        @arg @ref LL_LCD_OFFLEVEL_HIGHZ
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetOffOutputLevel(LCD_TypeDef *LCDx, uint32_t Level)
{
    MODIFY_REG(LCDx->RESCTL, LCD_RESCTL_OFF_LVL, Level);
}

/**
 * @brief  Return output voltage when disable LCD.
 * @rmtoll RESCTL    OFF_LVL    LL_LCD_GetOffOutputLevel
 * @param  LCDx LCD instance
 * @retval Return value can be one of the following values:
 *        @arg @ref LL_LCD_OFFLEVEL_V0
 *        @arg @ref LL_LCD_OFFLEVEL_VA
 *        @arg @ref LL_LCD_OFFLEVEL_VB
 *        @arg @ref LL_LCD_OFFLEVEL_VC
 *        @arg @ref LL_LCD_OFFLEVEL_VD
 *        @arg @ref LL_LCD_OFFLEVEL_HIGHZ
 */
__STATIC_INLINE uint32_t LL_LCD_GetOffOutputLevel(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->RESCTL, LCD_RESCTL_OFF_LVL));
}

/**
 * @brief Configure output level of LDO.
 * @note  LDO output is the voltage of LCDVD, and generate LCDVA, LCDVB, LCDVC through resistor voltage division.
 * @note  LDO output support 2.7V ~ 3.6V, step 0.06V.
 * @rmtoll RESCTL    LDOS    LL_LCD_SetOutputLDO
 * @param LCDx  LCD instance
 * @param Level This parameter can be one of the following values:
 *        @arg @ref LL_LCD_LDOS_2P7
 *        @arg @ref LL_LCD_LDOS_2P76
 *        @arg @ref LL_LCD_LDOS_2P82
 *        @arg @ref LL_LCD_LDOS_2P88
 *        @arg @ref LL_LCD_LDOS_2P94
 *        @arg @ref LL_LCD_LDOS_3P0
 *        @arg @ref LL_LCD_LDOS_3P06
 *        @arg @ref LL_LCD_LDOS_3P12
 *        @arg @ref LL_LCD_LDOS_3P18
 *        @arg @ref LL_LCD_LDOS_3P24
 *        @arg @ref LL_LCD_LDOS_3P3
 *        @arg @ref LL_LCD_LDOS_3P36
 *        @arg @ref LL_LCD_LDOS_3P42
 *        @arg @ref LL_LCD_LDOS_3P48
 *        @arg @ref LL_LCD_LDOS_3P54
 *        @arg @ref LL_LCD_LDOS_3P6
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetOutputLDO(LCD_TypeDef *LCDx, uint32_t Level)
{
    MODIFY_REG(LCDx->RESCTL, LCD_RESCTL_LDOS, (Level << LCD_RESCTL_LDOS_POS));
}

/**
 * @brief  Return output level of LDO.
 * @rmtoll RESCTL    LDOS    LL_LCD_GetOutputLDO
 * @param  LCDx LCD instance
 * @retval Return value can be one of the following values:
 *        @arg @ref LL_LCD_LDOS_2P7
 *        @arg @ref LL_LCD_LDOS_2P76
 *        @arg @ref LL_LCD_LDOS_2P82
 *        @arg @ref LL_LCD_LDOS_2P88
 *        @arg @ref LL_LCD_LDOS_2P94
 *        @arg @ref LL_LCD_LDOS_3P0
 *        @arg @ref LL_LCD_LDOS_3P06
 *        @arg @ref LL_LCD_LDOS_3P12
 *        @arg @ref LL_LCD_LDOS_3P18
 *        @arg @ref LL_LCD_LDOS_3P24
 *        @arg @ref LL_LCD_LDOS_3P3
 *        @arg @ref LL_LCD_LDOS_3P36
 *        @arg @ref LL_LCD_LDOS_3P42
 *        @arg @ref LL_LCD_LDOS_3P48
 *        @arg @ref LL_LCD_LDOS_3P54
 *        @arg @ref LL_LCD_LDOS_3P6
 */
__STATIC_INLINE uint32_t LL_LCD_GetOutputLDO(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->RESCTL, LCD_RESCTL_LDOS) >> LCD_RESCTL_LDOS_POS);
}

/**
 * @brief Enable fast charge function.
 * @note  Fast charge function: Adjust internal resistor to 5K and remain for 100ms to fastly charge the capacitor.
 * @rmtoll RESCTL    FCC      LL_LCD_EnableFastCharge
 * @param  LCDx LCD instance
 * @return None
 */
__STATIC_INLINE void LL_LCD_EnableFastCharge(LCD_TypeDef *LCDx)
{
    SET_BIT(LCDx->RESCTL, LCD_RESCTL_FCC);
}

/**
 * @brief  Disable charge pump voltage division mode.
 * @note   Fast charge function: Adjust internal resistor to 5K and remain for 100ms to fastly charge the capacitor.
 * @rmtoll RESCTL    FCC      LL_LCD_DisableFastCharge
 * @param  LCDx LCD instance
 * @return None
 */
__STATIC_INLINE void LL_LCD_DisableFastCharge(LCD_TypeDef *LCDx)
{
    CLEAR_BIT(LCDx->RESCTL, LCD_RESCTL_FCC);
}

/**
 * @brief  Return if charge pump voltage division mode is enable or disable.
 * @rmtoll RESCTL    FCC      LL_LCD_IsEnabledFastCharge
 * @param  LCDx LCD instance
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_LCD_IsEnabledFastCharge(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->RESCTL, LCD_RESCTL_FCC) == LCD_RESCTL_FCC);
}

/**
 * @brief Configure resistor string drive type.
 * @note  Type can be LL_LCD_RESDRIVE_TIMEDIV or LL_LCD_RESDRIVE_ALWAYS,
 *        LL_LCD_RESDRIVE_TIMEDIV: Resistor time-division drive type, drive signal is generated from RES_DT adn RES_FT.
 *        LL_LCD_RESDRIVE_ALWAYS : Resistor always be driven, configuration of RES_DT adn RES_FT are invalid.
 * @note  When LCD DRIVE TYPE select TYPE-A, this Configuration will always be LL_LCD_RESDRIVE_ALWAYS.
 * @rmtoll RESCTL    RES_AO      LL_LCD_SetResDriveType
 * @param LCDx LCD instance
 * @param Type This parameter can be one of the following values:
 *        @arg @ref LL_LCD_RESDRIVE_TIMEDIV
 *        @arg @ref LL_LCD_RESDRIVE_ALWAYS
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetResDriveType(LCD_TypeDef *LCDx, uint32_t Type)
{
    MODIFY_REG(LCDx->RESCTL, LCD_RESCTL_RES_AO, Type);
}

/**
 * @brief Get resistor string drive type.
 * @rmtoll RESCTL    RES_AO      LL_LCD_GetResDriveType
 * @param  LCDx LCD instance
 * @retval The return value can be one of the following values:
 *        @arg @ref LL_LCD_RESDRIVE_TIMEDIV
 *        @arg @ref LL_LCD_RESDRIVE_ALWAYS
 */
__STATIC_INLINE uint32_t LL_LCD_GetResDriveType(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->RESCTL, LCD_RESCTL_RES_AO));
}

/**
 * @brief Configure resistor drive duration.
 * @note  20K resistors drive time configuration during one update screen cycle for resistor division mode.
 * @rmtoll RESCTL    RES_DT    LL_LCD_SetResDriveDuration
 * @param LCDx LCD instance
 * @param Duration Drive duration, 3bit, 0<=Duration<=7
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetResDriveDuration(LCD_TypeDef *LCDx, uint32_t Duration)
{
    MODIFY_REG(LCDx->RESCTL, LCD_RESCTL_RES_DT, (Duration << LCD_RESCTL_RES_DT_POS));
}

/**
 * @brief Return LCD pump setup time.
 * @note  20K resistors drive time configuration during one update screen cycle for resistor division mode.
 * @rmtoll RESCTL    RES_DT    LL_LCD_GetResDriveDuration
 * @param  LCDx LCD instance
 * @retval Resistor drive duration
 */
__STATIC_INLINE uint32_t LL_LCD_GetResDriveDuration(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->RESCTL, LCD_RESCTL_RES_DT) >> LCD_RESCTL_RES_DT_POS);
}

/**
 * @brief Configure resistor drive count.
 * @note  20K resistors drive count configuration during one update screen cycle for resistor division mode.
 * @rmtoll RESCTL    RES_FT    LL_LCD_SetResDriveCount
 * @param LCDx LCD instance
 * @param Count This parameter can be one of the following values:
 *        @arg @ref LL_LCD_DRIVECNT_1
 *        @arg @ref LL_LCD_DRIVECNT_2
 *        @arg @ref LL_LCD_DRIVECNT_3
 *        @arg @ref LL_LCD_DRIVECNT_4
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetResDriveCount(LCD_TypeDef *LCDx, uint32_t Count)
{
    MODIFY_REG(LCDx->RESCTL, LCD_RESCTL_RES_FT, Count);
}

/**
 * @brief Return resistor drive count.
 * @note  20K resistors drive count configuration during one update screen cycle for resistor division mode.
 * @rmtoll RESCTL    RES_FT    LL_LCD_GetResDriveCount
 * @param  LCDx LCD instance
 * @retval The return value can be one of the following values:
 *        @arg @ref LL_LCD_DRIVECNT_1
 *        @arg @ref LL_LCD_DRIVECNT_2
 *        @arg @ref LL_LCD_DRIVECNT_3
 *        @arg @ref LL_LCD_DRIVECNT_4
 */
__STATIC_INLINE uint32_t LL_LCD_GetResDriveCount(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->RESCTL, LCD_RESCTL_RES_FT));
}

/**
 * @brief Configure internal resistor divider signal.
 * @note  Signal can be LL_LCD_INTERRES_SMALL or LL_LCD_INTERRES_SWITCH,
 *        LL_LCD_INTERRES_SMALL:  20k resistor time-division connected, 200k resistor short circuited.
 *        LL_LCD_INTERRES_SWITCH: Time-division switching between 20k resistor and 200k resistor.
 * @rmtoll RESCTL    RSM    LL_LCD_SetInterResDivSignal
 * @param LCDx LCD instance
 * @param Signal This parameter can be one of the following values:
 *        @arg @ref LL_LCD_INTERRES_SMALL
 *        @arg @ref LL_LCD_INTERRES_SWITCH
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetInterResDivSignal(LCD_TypeDef *LCDx, uint32_t Signal)
{
    MODIFY_REG(LCDx->RESCTL, LCD_RESCTL_RSM, Signal);
}

/**
 * @brief Return internal resistor divider signal.
 * @note  Signal can be LL_LCD_INTERRES_SMALL or LL_LCD_INTERRES_SWITCH,
 *        LL_LCD_INTERRES_SMALL:  20k resistor time-division connected, 200k resistor short circuited.
 *        LL_LCD_INTERRES_SWITCH: Time-division switching between 20k resistor and 200k resistor.
 * @rmtoll RESCTL    RSM    LL_LCD_GetInterResDivSignal
 * @param  LCDx LCD instance
 * @retval The return value can be one of the following values:
 *        @arg @ref LL_LCD_INTERRES_SMALL
 *        @arg @ref LL_LCD_INTERRES_SWITCH
 */
__STATIC_INLINE uint32_t LL_LCD_GetInterResDivSignal(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->RESCTL, LCD_RESCTL_RSM));
}

/**
 * @brief Configure resistor divider mode.
 * @note  Mode can be LL_LCD_RESMODE_INTERRES or LL_LCD_RESMODE_RESERVED,
 *        LL_LCD_RESMODE_INTERRES: Internal resistor division mode.
 *        LL_LCD_RESMODE_RESERVED: reserved, no function.
 * @rmtoll RESCTL    IRSN      LL_LCD_SetResDivMode
 * @param  LCDx LCD instance
 * @param Signal This parameter can be one of the following values:
 *        @arg @ref LL_LCD_RESMODE_INTERRES
 *        @arg @ref LL_LCD_RESMODE_RESERVED
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetResDivMode(LCD_TypeDef *LCDx, uint32_t Mode)
{
    MODIFY_REG(LCDx->RESCTL, LCD_RESCTL_IRSN, Mode);
}

/**
 * @brief Return resistor divider mode.
 * @rmtoll RESCTL    IRSN    LL_LCD_GetInterResDivSignal
 * @param  LCDx LCD instance
 * @retval The return value can be one of the following values:
 *        @arg @ref LL_LCD_RESMODE_INTERRES
 *        @arg @ref LL_LCD_RESMODE_RESERVED
 */
__STATIC_INLINE uint32_t LL_LCD_GetResDivMode(LCD_TypeDef *LCDx)
{
    return (uint32_t)(READ_BIT(LCDx->RESCTL, LCD_RESCTL_IRSN));
}

/**
 * @brief Configure LCD display buffer.
 * @note  0 <= Number <= 37, BUF[0] ~ BUF[37] = SEG0 ~ SEG37.
 * @note  Input 8bits data buffer, bit0 ~ bit7 = COM0 ~ COM7,
 *        bit is 1: display segment, bit is 0: do not display segment
 * @rmtoll BUF    BUF    LL_LCD_SetLCDBuffer
 * @param LCDx LCD instance
 * @param Number LCD buffer number, 0 <= Number <= 37.
 * @param Data   Display data, 8bit
 * @return None
 */
__STATIC_INLINE void LL_LCD_SetLCDBuffer(LCD_TypeDef *LCDx, uint32_t Number, uint8_t Data)
{
    WRITE_REG(LCDx->BUF[Number], Data);
}

/**
 * @brief Return LCD buffer data.
 * @note  0 <= Number <= 37, BUF[0] ~ BUF[37] = SEG0 ~ SEG37.
 * @note  Read 8bits Data, bit0 ~ bit7 = COM0 ~ COM7, bit is 1: display segment, bit is 0: no display segment
 * @rmtoll BUF    BUF    LL_LCD_GetLCDBuffer
 * @param  LCDx LCD instance
 * @retval LCD Buffer data
 */
__STATIC_INLINE uint32_t LL_LCD_GetLCDBuffer(LCD_TypeDef *LCDx, uint32_t Number)
{
    return (uint32_t)(READ_REG(LCDx->BUF[Number]));
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup LCD_LL_EF_Init Initialization and de-initialization functions
 */
ErrorStatus LL_LCD_DeInit(LCD_TypeDef *LCDx);
ErrorStatus LL_LCD_Init(LCD_TypeDef *LCDx, LL_LCD_InitTypeDef *LCD_InitStruct);
void        LL_LCD_StructInit(LL_LCD_InitTypeDef *LCD_InitStruct);
/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */

#endif/* LCD */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_LL_LCD_H */
