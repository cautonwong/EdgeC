/**
 *****************************************************************************
 * @file rn_rv_ll_gpio.h
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
#ifndef __RN_RV_LL_GPIO_H
#define __RN_RV_LL_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_gpio.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (GPIOA) || defined (GPIOB) || defined (GPIOC) || defined (GPIOD) || defined (GPIOE) || defined (GPIOF) || defined (GPIOG) || defined (GPIOH) || defined (GPIOI) || defined (GPIOJ) || defined (GPIOK) || defined (GPIO_DBGx)

/** @defgroup GPIO_LL GPIO
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/** @defgroup GPIO_LL_Private_Constants  GPIO Private Constants
 * @{
 */

/** @defgroup GPIO_LL_PC_DBG_PASSWORD  GPIO Debug Password
 * @{
 */
#define LL_GPIO_DBG_ACCESS_PASSWORD1        0x55DD77EE
#define LL_GPIO_DBG_ACCESS_PASSWORD2        0x66883399
#define LL_GPIO_DBG_ACCESS_PASSWORD3        0xDDAABB11
/**
 * @}
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup GPIO_LL_Exported_Types GPIO Exported Types
 * @{
 */

/**
 * @defgroup LL GPIO Init Structure definition
 * @{
 */
typedef struct
{
    uint32_t Pin;           /*!< Specifies the GPIO pins to be configured.
                                This parameter can be any value of @ref GPIO_LL_EC_PIN */

    uint32_t Mode;          /*!< Specifies the operating mode for the selected pins.
                                This parameter can be a value of @ref GPIO_LL_EC_MODE.

                                GPIO HW configuration can be modified afterwards using unitary function @ref LL_GPIO_SetPinMode().*/

    uint32_t Speed;         /*!< Specifies the speed for the selected pins.
                                This parameter can be a value of @ref GPIO_LL_EC_SPEED.

                                GPIO HW configuration can be modified afterwards using unitary function @ref LL_GPIO_SetPinSpeed().*/

    uint32_t OutputType;    /*!< Specifies the operating output type for the selected pins.
                                This parameter can be a value of @ref GPIO_LL_EC_OUTPUT.

                                GPIO HW configuration can be modified afterwards using unitary function @ref LL_GPIO_SetPinOutputType().*/

    uint32_t InputType;     /*!< Specifies the operating input type for the selected pins.
                                This parameter can be a value of @ref GPIO_LL_EC_INPUT.

                                GPIO HW configuration can be modified afterwards using unitary function @ref LL_GPIO_SetPinInputType().*/

    uint32_t Pull;          /*!< Specifies the operating Pull-up/Pull down for the selected pins.
                                This parameter can be a value of @ref GPIO_LL_EC_PULL.

                                GPIO HW configuration can be modified afterwards using unitary function @ref LL_GPIO_SetPinPull().*/

    uint32_t Alternate;     /*!< Specifies the Peripheral to be connected to the selected pins.
                                This parameter can be a value of @ref GPIO_LL_EC_AF.

                                GPIO HW configuration can be modified afterwards using unitary function @ref LL_GPIO_SetAFPin_0_7() and LL_GPIO_SetAFPin_8_15().*/
} LL_GPIO_InitTypeDef;
/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup GPIO_LL_Exported_Constants GPIO Exported Constants
 * @{
 */

/** @defgroup GPIO_LL_EC_PIN PIN
 * @{
 */
#define LL_GPIO_PIN_0                      (BIT(0))  /*!< Select pin 0 */
#define LL_GPIO_PIN_1                      (BIT(1))  /*!< Select pin 1 */
#define LL_GPIO_PIN_2                      (BIT(2))  /*!< Select pin 2 */
#define LL_GPIO_PIN_3                      (BIT(3))  /*!< Select pin 3 */
#define LL_GPIO_PIN_4                      (BIT(4))  /*!< Select pin 4 */
#define LL_GPIO_PIN_5                      (BIT(5))  /*!< Select pin 5 */
#define LL_GPIO_PIN_6                      (BIT(6))  /*!< Select pin 6 */
#define LL_GPIO_PIN_7                      (BIT(7))  /*!< Select pin 7 */
#define LL_GPIO_PIN_8                      (BIT(8))  /*!< Select pin 8 */
#define LL_GPIO_PIN_9                      (BIT(9))  /*!< Select pin 9 */
#define LL_GPIO_PIN_10                     (BIT(10)) /*!< Select pin 10 */
#define LL_GPIO_PIN_11                     (BIT(11)) /*!< Select pin 11 */
#define LL_GPIO_PIN_12                     (BIT(12)) /*!< Select pin 12 */
#define LL_GPIO_PIN_13                     (BIT(13)) /*!< Select pin 13 */
#define LL_GPIO_PIN_14                     (BIT(14)) /*!< Select pin 14 */
#define LL_GPIO_PIN_15                     (BIT(15)) /*!< Select pin 15 */
#define LL_GPIO_PIN_ALL                    (BITS(0,15)) /*!< Select all pins */
/**
 * @}
 */

/** @defgroup GPIO_LL_EC_MODE Mode
 * @{
 */
#define LL_GPIO_MODE_INPUT                 GPIO_MODE_MODE0_INPUT      /*!< Select input mode */
#define LL_GPIO_MODE_OUTPUT                GPIO_MODE_MODE0_OUTPUT     /*!< Select output mode */
#define LL_GPIO_MODE_ALTERNATE             GPIO_MODE_MODE0_ALTERNATE  /*!< Select alternate function mode */
#define LL_GPIO_MODE_ANALOG                GPIO_MODE_MODE0_ANALOG     /*!< Select analog mode */
/**
 * @}
 */

/** @defgroup GPIO_LL_EC_OUTPUT Output Type
 * @{
 */
#define LL_GPIO_OUTPUT_PUSHPULL            GPIO_OTYPE_OTYPE0_NORMAL    /*!< Select push-pull as output type */
#define LL_GPIO_OUTPUT_OPENDRAIN           GPIO_OTYPE_OTYPE0_OPENDRAIN /*!< Select open-drain as output type */
/**
 * @}
 */

/**
 * @defgroup GPIO_LL_EC_INPUT Input Type
 * @{
 */
#define LL_GPIO_INPUT_CMOS                 GPIO_ITYPE_ITYPE0_CMOS /*!< Select CMOS buffer as input type, Vil=0.3Vcc, Vih=0.7Vcc */
#define LL_GPIO_INPUT_TTL                  GPIO_ITYPE_ITYPE0_TTL  /*!< Select TTL buffer as input type, Vil=0.16Vcc, Vih=0.4Vcc */
/**
 * @}
 */

/** @defgroup GPIO_LL_EC_SPEED Output Speed
 * @{
 */
#define LL_GPIO_SPEED_FREQ_NORMAL          GPIO_OSPEED_OSPEED0_NORMAL    /*!< Select I/O normal output speed */
#define LL_GPIO_SPEED_FREQ_HIGH            GPIO_OSPEED_OSPEED0_HIGHSPEED /*!< Select I/O fast output speed */
/**
 * @}
 */

/** @defgroup GPIO_LL_EC_PULL Pull Up Pull Down
 * @{
 */
#define LL_GPIO_PULL_NO                    GPIO_UPDN_UPDN0_FLOATING /*!< Select I/O no pull */
#define LL_GPIO_PULL_UP                    GPIO_UPDN_UPDN0_PULLUP   /*!< Select I/O pull up */
#define LL_GPIO_PULL_DOWN                  GPIO_UPDN_UPDN0_PULLDOWN /*!< Select I/O pull down */
/**
 * @}
 */

/** @defgroup GPIO_LL_EC_AF Alternate Function
 * @{
 */
#define LL_GPIO_AF_0                       (0x0000000U) /*!< Select alternate function 0 */
#define LL_GPIO_AF_1                       (0x0000001U) /*!< Select alternate function 1 */
#define LL_GPIO_AF_2                       (0x0000002U) /*!< Select alternate function 2 */
#define LL_GPIO_AF_3                       (0x0000003U) /*!< Select alternate function 3 */
#define LL_GPIO_AF_4                       (0x0000004U) /*!< Select alternate function 4 */
#define LL_GPIO_AF_5                       (0x0000005U) /*!< Select alternate function 5 */
#define LL_GPIO_AF_6                       (0x0000006U) /*!< Select alternate function 6 */
#define LL_GPIO_AF_7                       (0x0000007U) /*!< Select alternate function 7 */
#define LL_GPIO_AF_8                       (0x0000008U) /*!< Select alternate function 8 */
#define LL_GPIO_AF_9                       (0x0000009U) /*!< Select alternate function 9 */
#define LL_GPIO_AF_10                      (0x000000AU) /*!< Select alternate function 10 */
#define LL_GPIO_AF_11                      (0x000000BU) /*!< Select alternate function 11 */
#define LL_GPIO_AF_12                      (0x000000CU) /*!< Select alternate function 12 */
#define LL_GPIO_AF_13                      (0x000000DU) /*!< Select alternate function 13 */
#define LL_GPIO_AF_14                      (0x000000EU) /*!< Select alternate function 14 */
#define LL_GPIO_AF_15                      (0x000000FU) /*!< Select alternate function 15 */
/**
 * @}
 */

/** @defgroup GPIO_LL_EC_JTAG JTAG pin
 * @{
 */
#define LL_GPIO_JTDO                       GPIO_DBG_JTDO_EN /*!< Select JTDO pin of JTAG */
#define LL_GPIO_JTDI                       GPIO_DBG_JTDI_EN /*!< Select JTDI pin of JTAG */
#define LL_GPIO_JTMS                       GPIO_DBG_JTMS_EN /*!< Select JTMS pin of JTAG */
#define LL_GPIO_JTCK                       GPIO_DBG_JTCK_EN /*!< Select JTCK pin of JTAG */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup GPIO_LL_Exported_Macros GPIO Exported Macros
 * @{
 */

/** @defgroup GPIO_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */
/**
 * @brief  Write a value in GPIO register
 * @param  __INSTANCE__ GPIO Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @return None
 */
#define LL_GPIO_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in GPIO register
 * @param  __INSTANCE__ GPIO Instance
 * @param  __REG__ Register to be read
 * @return Register value
 */
#define LL_GPIO_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup GPIO_LL_Exported_Functions GPIO Exported Functions
 * @{
 */

/** @defgroup GPIO_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief Configure gpio mode for a dedicated pin on dedicated port.
 * @note  Warning: only one pin can be passed as parameter.
 * @note  GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 * @note  GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @note  PA02/PA03 default to CMP1/CMP2 function, GPIO_MODE configuration is invalid.
 * @note  PA04 defaults to analog mode, when enable LVD external detection funciton,
 *        GPIO_MODE configuration is invalid.
 * @note  PE14/PE15 default to analog mode, when HOSC_PD is set to 0, GPIO_MODE configuration is invalid.
 * @rmtoll MODE        MODE         LL_GPIO_SetPinMode
 * @param GPIOx GPIO Port
 * @param Pin This parameter can be one of the following values:
 *        @arg @ref LL_GPIO_PIN_0
 *        @arg @ref LL_GPIO_PIN_1
 *        @arg @ref LL_GPIO_PIN_2
 *        @arg @ref LL_GPIO_PIN_3
 *        @arg @ref LL_GPIO_PIN_4
 *        @arg @ref LL_GPIO_PIN_5
 *        @arg @ref LL_GPIO_PIN_6
 *        @arg @ref LL_GPIO_PIN_7
 *        @arg @ref LL_GPIO_PIN_8
 *        @arg @ref LL_GPIO_PIN_9
 *        @arg @ref LL_GPIO_PIN_10
 *        @arg @ref LL_GPIO_PIN_11
 *        @arg @ref LL_GPIO_PIN_12
 *        @arg @ref LL_GPIO_PIN_13
 *        @arg @ref LL_GPIO_PIN_14
 *        @arg @ref LL_GPIO_PIN_15
 * @param Mode This parameter can be one of the following values:
 *        @arg @ref LL_GPIO_MODE_INPUT
 *        @arg @ref LL_GPIO_MODE_OUTPUT
 *        @arg @ref LL_GPIO_MODE_ALTERNATE
 *        @arg @ref LL_GPIO_MODE_ANALOG
 * @return None
 */
__STATIC_INLINE void LL_GPIO_SetPinMode(GPIO_TypeDef *GPIOx, uint32_t Pin, uint32_t Mode)
{
    MODIFY_REG(GPIOx->MODE, (GPIO_MODE_MODE0 << (POSITION_VAL(Pin) * 2U)), (Mode << (POSITION_VAL(Pin) * 2U)));
}


/**
 * @brief  Return gpio mode for a dedicated pin on dedicated port.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @note   Warning: only one pin can be passed as parameter.
 * @rmtoll MODE        MODE         LL_GPIO_GetPinMode
 * @param  GPIOx GPIO Port
 * @param  Pin This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_GPIO_MODE_INPUT
 *         @arg @ref LL_GPIO_MODE_OUTPUT
 *         @arg @ref LL_GPIO_MODE_ALTERNATE
 *         @arg @ref LL_GPIO_MODE_ANALOG
 */
__STATIC_INLINE uint32_t LL_GPIO_GetPinMode(GPIO_TypeDef *GPIOx, uint32_t Pin)
{
    return (uint32_t)(READ_BIT(GPIOx->MODE, (GPIO_MODE_MODE0 << (POSITION_VAL(Pin) * 2U)))
                     >> (POSITION_VAL(Pin) * 2U));
}

/**
 * @brief  Configure gpio output type for several pins on dedicated port.
 * @note   Output type as to be set when gpio pin is in output or
 *         alternate modes. Possible type are Push-pull or Open-drain.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll OTYPE       OTYPE           LL_GPIO_SetPinOutputType
 * @param  GPIOx GPIO Port
 * @param  PinMask This parameter can be a combination of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @param  OutputType This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_OUTPUT_PUSHPULL
 *         @arg @ref LL_GPIO_OUTPUT_OPENDRAIN
 * @retval None
 */
__STATIC_INLINE void LL_GPIO_SetPinOutputType(GPIO_TypeDef *GPIOx, uint32_t PinMask, uint32_t OutputType)
{
    MODIFY_REG(GPIOx->OTYPE, PinMask, (PinMask * OutputType));
}

/**
 * @brief  Return gpio output type for dedicated pin on dedicated port.
 * @note   Warning: only one pin can be passed as parameter.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll OTYPE       OTYPE           LL_GPIO_GetPinOutputType
 * @param  GPIOx GPIO Port
 * @param  Pin This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_GPIO_OUTPUT_PUSHPULL
 *         @arg @ref LL_GPIO_OUTPUT_OPENDRAIN
 */
__STATIC_INLINE uint32_t LL_GPIO_GetPinOutputType(GPIO_TypeDef *GPIOx, uint32_t Pin)
{
    return (uint32_t)(READ_BIT(GPIOx->OTYPE, Pin) >> POSITION_VAL(Pin));
}

/**
 * @brief  Configure gpio input type for several pins on dedicated port.
 * @note   I/O input type can be CMOS type or TTL type.
 * @note   Refer to datasheet for voltage threshold for each input type.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll ITYPE       ITYPE           LL_GPIO_SetPinInputType
 * @param  GPIOx GPIO Port
 * @param  PinMask This parameter can be a combination of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @param  OutputType This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_INPUT_CMOS
 *         @arg @ref LL_GPIO_INPUT_TTL
 * @retval None
 */
__STATIC_INLINE void LL_GPIO_SetPinInputType(GPIO_TypeDef *GPIOx, uint32_t PinMask, uint32_t InputType)
{
    MODIFY_REG(GPIOx->ITYPE, PinMask, (PinMask * InputType));
}

/**
 * @brief  Return gpio input type for dedicated pin on dedicated port.
 * @note   Warning: only one pin can be passed as parameter.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll ITYPE       ITYPE           LL_GPIO_GetPinInputType
 * @param  GPIOx GPIO Port
 * @param  Pin This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_GPIO_INPUT_CMOS
 *         @arg @ref LL_GPIO_INPUT_TTL
 */
__STATIC_INLINE uint32_t LL_GPIO_GetPinInputType(GPIO_TypeDef *GPIOx, uint32_t Pin)
{
    return (uint32_t)(READ_BIT(GPIOx->ITYPE, Pin) >> POSITION_VAL(Pin));
}

/**
 * @brief  Configure gpio speed for a dedicated pin on dedicated port.
 * @note   Warning: only one pin can be passed as parameter.
 * @note   Refer to datasheet for frequency specifications and load conditions for each speed.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll OSPEED      OSPEED       LL_GPIO_SetPinSpeed
 * @param  GPIOx GPIO Port
 * @param  Pin This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 * @param  Speed This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_SPEED_FREQ_NORMAL
 *         @arg @ref LL_GPIO_SPEED_FREQ_HIGH
 * @retval None
 */
__STATIC_INLINE void LL_GPIO_SetPinSpeed(GPIO_TypeDef *GPIOx, uint32_t Pin, uint32_t  Speed)
{
    MODIFY_REG(GPIOx->OSPEED, Pin, (Pin * Speed));
}

/**
 * @brief  Return gpio speed for a dedicated pin on dedicated port.
 * @note   Warning: only one pin can be passed as parameter.
 * @note   Refer to datasheet for frequency specifications and load conditions for each speed.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll OSPEED      OSPEED       LL_GPIO_GetPinSpeed
 * @param  GPIOx GPIO Port
 * @param  Pin This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_GPIO_SPEED_FREQ_NORMAL
 *         @arg @ref LL_GPIO_SPEED_FREQ_HIGH
 */
__STATIC_INLINE uint32_t LL_GPIO_GetPinSpeed(GPIO_TypeDef *GPIOx, uint32_t Pin)
{
    return (uint32_t)(READ_BIT(GPIOx->OSPEED, Pin) >> POSITION_VAL(Pin));
}

/**
 * @brief  Configure gpio pull-up or pull-down for a dedicated pin on a dedicated port.
 * @note   Warning: only one pin can be passed as parameter.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll UPDN        UPDN         LL_GPIO_SetPinPull
 * @param  GPIOx GPIO Port
 * @param  Pin This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 * @param  Pull This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_PULL_NO
 *         @arg @ref LL_GPIO_PULL_UP
 *         @arg @ref LL_GPIO_PULL_DOWN
 * @retval None
 */
__STATIC_INLINE void LL_GPIO_SetPinPull(GPIO_TypeDef *GPIOx, uint32_t Pin, uint32_t Pull)
{
    MODIFY_REG(GPIOx->UPDN, (GPIO_UPDN_UPDN0 << (POSITION_VAL(Pin) * 2U)), (Pull << (POSITION_VAL(Pin) * 2U)));
}

/**
 * @brief  Return gpio pull-up or pull-down for a dedicated pin on a dedicated port
 * @note   Warning: only one pin can be passed as parameter.
 * @note   GPIOH only support LL_GPIO_PIN_0  LL_GPIO_PIN_1
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll UPDN        UPDN         LL_GPIO_GetPinPull
 * @param  GPIOx GPIO Port
 * @param  Pin This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_GPIO_PULL_NO
 *         @arg @ref LL_GPIO_PULL_UP
 *         @arg @ref LL_GPIO_PULL_DOWN
 */
__STATIC_INLINE uint32_t LL_GPIO_GetPinPull(GPIO_TypeDef *GPIOx, uint32_t Pin)
{
  return (uint32_t)(READ_BIT(GPIOx->UPDN, (GPIO_UPDN_UPDN0 << (POSITION_VAL(Pin) * 2U)))
                 >> (POSITION_VAL(Pin) * 2U));
}

/**
 * @brief  Configure gpio alternate function of a dedicated pin from 0 to 7 for a dedicated port.
 * @note   PL00~PL03 are VBAT IO, configuration priority: VBAT > GPIO, if config PL00~PL03 as VBAT funciton, the GPIO
 *         configuration will be disabled.
 * @note   PL01 (VBAT power domain) IO function priority: MADC IO Function > RTC IO function > Gernal GPIO
 * @note   Warning: only one pin can be passed as parameter.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll AFSEL0         SEL        LL_GPIO_SetAFPin_0_7
 * @param  GPIOx GPIO Port
 * @param  Pin This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 * @param  Alternate This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_AF_0
 *         @arg @ref LL_GPIO_AF_1
 *         @arg @ref LL_GPIO_AF_2
 *         @arg @ref LL_GPIO_AF_3
 *         @arg @ref LL_GPIO_AF_4
 *         @arg @ref LL_GPIO_AF_5
 *         @arg @ref LL_GPIO_AF_6
 *         @arg @ref LL_GPIO_AF_7
 *         @arg @ref LL_GPIO_AF_8
 *         @arg @ref LL_GPIO_AF_9
 *         @arg @ref LL_GPIO_AF_10
 *         @arg @ref LL_GPIO_AF_11
 *         @arg @ref LL_GPIO_AF_12
 *         @arg @ref LL_GPIO_AF_13
 *         @arg @ref LL_GPIO_AF_14
 *         @arg @ref LL_GPIO_AF_15
 * @retval None
 */
__STATIC_INLINE void LL_GPIO_SetAFPin_0_7(GPIO_TypeDef *GPIOx, uint32_t Pin, uint32_t Alternate)
{
    MODIFY_REG(GPIOx->AFSEL0, (GPIO_AFSEL0_SEL0 << (POSITION_VAL(Pin) * 4U)),
             (Alternate << (POSITION_VAL(Pin) * 4U)));
}

/**
 * @brief  Return gpio alternate function of a dedicated pin from 0 to 7 for a dedicated port.
 * @note   Warning: only one pin can be passed as parameter.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll AFSEL0         SEL        LL_GPIO_GetAFPin_0_7
 * @param  GPIOx GPIO Port
 * @param  Pin This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_GPIO_AF_0
 *         @arg @ref LL_GPIO_AF_1
 *         @arg @ref LL_GPIO_AF_2
 *         @arg @ref LL_GPIO_AF_3
 *         @arg @ref LL_GPIO_AF_4
 *         @arg @ref LL_GPIO_AF_5
 *         @arg @ref LL_GPIO_AF_6
 *         @arg @ref LL_GPIO_AF_7
 *         @arg @ref LL_GPIO_AF_8
 *         @arg @ref LL_GPIO_AF_9
 *         @arg @ref LL_GPIO_AF_10
 *         @arg @ref LL_GPIO_AF_11
 *         @arg @ref LL_GPIO_AF_12
 *         @arg @ref LL_GPIO_AF_13
 *         @arg @ref LL_GPIO_AF_14
 *         @arg @ref LL_GPIO_AF_15
 */
__STATIC_INLINE uint32_t LL_GPIO_GetAFPin_0_7(GPIO_TypeDef *GPIOx, uint32_t Pin)
{
    return (uint32_t)(READ_BIT(GPIOx->AFSEL0, (GPIO_AFSEL0_SEL0 << (POSITION_VAL(Pin) * 4U)))
                     >> (POSITION_VAL(Pin) * 4U));
}

/**
 * @brief  Configure gpio alternate function of a dedicated pin from 8 to 15 for a dedicated port.
 * @note   Warning: only one pin can be passed as parameter.
 * @rmtoll AFSEL1         SEL        LL_GPIO_SetAFPin_8_15
 * @param  GPIOx GPIO Port
 * @param  Pin This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 * @param  Alternate This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_AF_0
 *         @arg @ref LL_GPIO_AF_1
 *         @arg @ref LL_GPIO_AF_2
 *         @arg @ref LL_GPIO_AF_3
 *         @arg @ref LL_GPIO_AF_4
 *         @arg @ref LL_GPIO_AF_5
 *         @arg @ref LL_GPIO_AF_6
 *         @arg @ref LL_GPIO_AF_7
 *         @arg @ref LL_GPIO_AF_8
 *         @arg @ref LL_GPIO_AF_9
 *         @arg @ref LL_GPIO_AF_10
 *         @arg @ref LL_GPIO_AF_11
 *         @arg @ref LL_GPIO_AF_12
 *         @arg @ref LL_GPIO_AF_13
 *         @arg @ref LL_GPIO_AF_14
 *         @arg @ref LL_GPIO_AF_15
 * @retval None
 */
__STATIC_INLINE void LL_GPIO_SetAFPin_8_15(GPIO_TypeDef *GPIOx, uint32_t Pin, uint32_t Alternate)
{
    MODIFY_REG(GPIOx->AFSEL1, (GPIO_AFSEL1_SEL8 << (POSITION_VAL(Pin >> 8U) * 4U)),
             (Alternate << (POSITION_VAL(Pin >> 8U) * 4U)));
}

/**
 * @brief  Return gpio alternate function of a dedicated pin from 8 to 15 for a dedicated port.
 * @note   Warning: only one pin can be passed as parameter.
 * @rmtoll AFSEL1         SEL        LL_GPIO_GetAFPin_8_15
 * @param  GPIOx GPIO Port
 * @param  Pin This parameter can be one of the following values:
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_GPIO_AF_0
 *         @arg @ref LL_GPIO_AF_1
 *         @arg @ref LL_GPIO_AF_2
 *         @arg @ref LL_GPIO_AF_3
 *         @arg @ref LL_GPIO_AF_4
 *         @arg @ref LL_GPIO_AF_5
 *         @arg @ref LL_GPIO_AF_6
 *         @arg @ref LL_GPIO_AF_7
 *         @arg @ref LL_GPIO_AF_8
 *         @arg @ref LL_GPIO_AF_9
 *         @arg @ref LL_GPIO_AF_10
 *         @arg @ref LL_GPIO_AF_11
 *         @arg @ref LL_GPIO_AF_12
 *         @arg @ref LL_GPIO_AF_13
 *         @arg @ref LL_GPIO_AF_14
 *         @arg @ref LL_GPIO_AF_15
 */
__STATIC_INLINE uint32_t LL_GPIO_GetAFPin_8_15(GPIO_TypeDef *GPIOx, uint32_t Pin)
{
    return (uint32_t)(READ_BIT(GPIOx->AFSEL1, (GPIO_AFSEL1_SEL8 << (POSITION_VAL(Pin >> 8U) * 4U)))
                     >> (POSITION_VAL(Pin >> 8U) * 4U));
}

/**
 * @brief  Return full input data register value for a dedicated port.
 * @rmtoll IDATA          IDATA           LL_GPIO_ReadInputPort
 * @param  GPIOx GPIO Port
 * @retval Input data of port
 */
__STATIC_INLINE uint32_t LL_GPIO_ReadInputPort(GPIO_TypeDef *GPIOx)
{
    return (uint32_t)(READ_REG(GPIOx->IDATA));
}

/**
 * @brief  Return if input level for several pins of dedicated port is high or low.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll IDATA          IDATA           LL_GPIO_IsInputPinSet
 * @param  GPIOx GPIO Port
 * @param  PinMask This parameter can be a combination of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @retval State of bit, 1: set high level, 0: set low level.
 */
__STATIC_INLINE uint32_t LL_GPIO_IsInputPinSet(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
    return (READ_BIT(GPIOx->IDATA, PinMask) == (PinMask));
}

/**
 * @brief  Write output data for a dedicated port.
 * @note   GPIOH only support pin0 and pin1
 *         GPIOL only support pin0 ~ pin3
 * @rmtoll OATA          OATA           LL_GPIO_WriteOutputPort
 * @param  GPIOx GPIO Port
 * @param  PortValue Level value for each pin of the port, 0: set low level, 1: set high level
 * @retval None
 */
__STATIC_INLINE void LL_GPIO_WriteOutputPort(GPIO_TypeDef *GPIOx, uint32_t PortValue)
{
    WRITE_REG(GPIOx->ODATA, PortValue);
}

/**
 * @brief  Return full output data register value for a dedicated port.
 * @rmtoll OATA          OATA           LL_GPIO_ReadOutputPort
 * @param  GPIOx GPIO Port
 * @retval Output data register value of port
 */
__STATIC_INLINE uint32_t LL_GPIO_ReadOutputPort(GPIO_TypeDef *GPIOx)
{
    return (uint32_t)(READ_REG(GPIOx->ODATA));
}

/**
 * @brief  Return if Output data level for several pins of dedicated port is high or low.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll OATA          OATA           LL_GPIO_IsOutputPinSet
 * @param  GPIOx GPIO Port
 * @param  PinMask This parameter can be a combination of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @retval State of bit, 1: high level, 0: low level.
 */
__STATIC_INLINE uint32_t LL_GPIO_IsOutputPinSet(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
    return (READ_BIT(GPIOx->ODATA, PinMask) == (PinMask));
}

/**
 * @brief  Set several pins to high level on dedicated port.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll SET         SET           LL_GPIO_SetOutputPin
 * @param  GPIOx GPIO Port
 * @param  PinMask This parameter can be a combination of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @retval None
 */
__STATIC_INLINE void LL_GPIO_SetOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
    WRITE_REG(GPIOx->SET, PinMask);
}

/**
 * @brief  Set several pins to low level on dedicated port.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll CLR         CLR           LL_GPIO_ResetOutputPin
 * @param  GPIOx GPIO Port
 * @param  PinMask This parameter can be a combination of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @retval None
 */
__STATIC_INLINE void LL_GPIO_ResetOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
    WRITE_REG(GPIOx->CLR, PinMask);
}

/**
 * @brief  Toggle data value for several pin of dedicated port.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll TG         TG           LL_GPIO_TogglePin
 * @param  GPIOx GPIO Port
 * @param  PinMask This parameter can be a combination of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @retval None
 */
__STATIC_INLINE void LL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
    WRITE_REG(GPIOx->TG, PinMask);
}

/**
 * @brief  Force enable input function for several pin of dedicated port.
 * @note   This function only valid when gpio pin is in output or alternate modes
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @note   Bit 0 : enable, bit 1 :disable
 * @rmtoll FIEN         FIEN           LL_GPIO_EnablePinForceInput
 * @param  GPIOx GPIO Port
 * @param  PinMask This parameter can be a combination of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @return None
 */
__STATIC_INLINE void LL_GPIO_EnablePinForceInput(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
    CLEAR_BIT(GPIOx->FIEN, PinMask);
}

/**
 * @brief  Disable force input function for several pin of dedicated port.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @note   Bit 0 : enable, bit 1 :disable
 * @rmtoll FIEN         FIEN           LL_GPIO_DisablePinForceInput
 * @param  GPIOx GPIO Port
 * @param  PinMask This parameter can be a combination of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @return None
 */
__STATIC_INLINE void LL_GPIO_DisablePinForceInput(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
    SET_BIT(GPIOx->FIEN, PinMask);
}

/**
 * @brief  Return if force inpout funciton for several pins of dedicated port is enable or disable.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll FIEN         FIEN           LL_GPIO_IsEnabledForceInput
 * @param  GPIOx GPIO Port
 * @param  PinMask This parameter can be a combination of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_GPIO_IsEnabledForceInput(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
    return ((READ_BIT(GPIOx->FIEN, PinMask) == PinMask) ? 0UL : 1UL);
}

/**
 * @brief  Enable Analog function for several pin of dedicated port.
 * @note   This function only valid when gpio pin is in output or alternate modes
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @note   Bit 0 : disable, bit 1 :enabale
 * @rmtoll AEN         ANA_EN           LL_GPIO_EnablePinAnalog
 * @param  GPIOx GPIO Port
 * @param  PinMask This parameter can be a combination of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @return None
 */
__STATIC_INLINE void LL_GPIO_EnablePinAnalog(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
    SET_BIT(GPIOx->AEN, PinMask);
}

/**
 * @brief  Disable Analog function for several pin of dedicated port.
 * @note   This function only valid when gpio pin is in output or alternate modes
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @note   Bit 0 : disable, bit 1 :enabale
 * @rmtoll AEN         ANA_EN           LL_GPIO_DisablePinAnalog
 * @param  GPIOx GPIO Port
 * @param  PinMask This parameter can be a combination of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @return None
 */
__STATIC_INLINE void LL_GPIO_DisablePinAnalog(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
    CLEAR_BIT(GPIOx->AEN, PinMask);
}

/**
 * @brief  Return if analog funciton for several pins of dedicated port is enable or disable.
 * @note   GPIOH only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_1,
 *         GPIOL only support LL_GPIO_PIN_0 ~ LL_GPIO_PIN_3
 * @rmtoll AEN         ANA_EN           LL_GPIO_IsEnabledAnalog
 * @param  GPIOx GPIO Port
 * @param  PinMask This parameter can be a combination of the following values:
 *         @arg @ref LL_GPIO_PIN_0
 *         @arg @ref LL_GPIO_PIN_1
 *         @arg @ref LL_GPIO_PIN_2
 *         @arg @ref LL_GPIO_PIN_3
 *         @arg @ref LL_GPIO_PIN_4
 *         @arg @ref LL_GPIO_PIN_5
 *         @arg @ref LL_GPIO_PIN_6
 *         @arg @ref LL_GPIO_PIN_7
 *         @arg @ref LL_GPIO_PIN_8
 *         @arg @ref LL_GPIO_PIN_9
 *         @arg @ref LL_GPIO_PIN_10
 *         @arg @ref LL_GPIO_PIN_11
 *         @arg @ref LL_GPIO_PIN_12
 *         @arg @ref LL_GPIO_PIN_13
 *         @arg @ref LL_GPIO_PIN_14
 *         @arg @ref LL_GPIO_PIN_15
 *         @arg @ref LL_GPIO_PIN_ALL
 * @retval State of bit, 1: enabled, 0: disabled
 */
__STATIC_INLINE uint32_t LL_GPIO_IsEnabledAnalog(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
    return ((READ_BIT(GPIOx->AEN, PinMask) == PinMask) ? 1UL : 0UL);
}

/**
 * @brief  Write key of DBG register.
 * @note   The following key sequence must be written in order: LL_GPIO_DBG_ACCESS_PASSWORD1
 *         , LL_GPIO_DBG_ACCESS_PASSWORD2, LL_GPIO_DBG_ACCESS_PASSWORD3
 *         Register only valid when the key sequence are right.
 * @note   write only
 * @rmtoll DBG_KEY         KEY           LL_GPIO_WriteDebugKey
 * @param  GPIO_DBGx GPIO_DBGx Instance
 * @param  KeyValue The key value
 * @return None
 */
__STATIC_INLINE void LL_GPIO_WriteDebugKey(GPIO_DBG_TypeDef *GPIO_DBGx, uint32_t KeyValue)
{
    WRITE_REG(GPIO_DBGx->DBG_KEY, KeyValue);
}

/**
 * @brief  Enable JTDO function of PG10
 * @note   IO priority: JTAG IO > GPIO. Enable JTAG function, the GPIO configuration will be disabled
 * @rmtoll DBG         JTDO_EN           LL_GPIO_EnablePinJtag
 * @param  GPIO_DBGx GPIO_DBGx Instance
 * @return  None
 */
__STATIC_INLINE void LL_GPIO_EnablePinJTDO(GPIO_DBG_TypeDef *GPIO_DBGx)
{
    SET_BIT(GPIO_DBGx->DBG, LL_GPIO_JTDO);
}

/**
 * @brief  Disable JTDO function of PG10, PG10 is used to a general IO
 * @rmtoll DBG         JTDO_EN           LL_GPIO_DisablePinJTDO
 * @param  GPIO_DBGx GPIO_DBGx Instance
 * @return  None
 */
__STATIC_INLINE void LL_GPIO_DisablePinJTDO(GPIO_DBG_TypeDef *GPIO_DBGx)
{
    CLEAR_BIT(GPIO_DBGx->DBG, LL_GPIO_JTDO);
}

/**
 * @brief  Return 1 if PG10 is enabled to JTDO. else return 0.
 * @rmtoll DBG         JTDO_EN           LL_GPIO_IsEnabledPinJTDO
 * @param  GPIO_DBGx GPIO_DBGx Instance
 * @return State of bit, 1: enable, 0: disable.
 */
__STATIC_INLINE uint32_t LL_GPIO_IsEnabledPinJTDO(GPIO_DBG_TypeDef *GPIO_DBGx)
{
    return (READ_BIT(GPIO_DBGx->DBG, LL_GPIO_JTDO) == (LL_GPIO_JTDO));
}

/**
 * @brief  Enable JTDI function of PG11
 * @note   IO priority: JTAG IO > GPIO. Enable JTAG function, the GPIO configuration will be disabled
 * @rmtoll DBG         JTDI_EN           LL_GPIO_EnablePinJTDI
 * @param  GPIO_DBGx GPIO_DBGx Instance
 * @return  None
 */
__STATIC_INLINE void LL_GPIO_EnablePinJTDI(GPIO_DBG_TypeDef *GPIO_DBGx)
{
    SET_BIT(GPIO_DBGx->DBG, LL_GPIO_JTDI);
}

/**
 * @brief  Disable JTDI function of PG11, PG11 is used to a general IO
 * @rmtoll DBG         JTDI_EN           LL_GPIO_DisablePinJTDI
 * @param  GPIO_DBGx GPIO_DBGx Instance
 * @return  None
 */
__STATIC_INLINE void LL_GPIO_DisablePinJTDI(GPIO_DBG_TypeDef *GPIO_DBGx)
{
    CLEAR_BIT(GPIO_DBGx->DBG, LL_GPIO_JTDI);
}

/**
 * @brief  Return 1 if PG11 is enabled to JTDI. else return 0.
 * @rmtoll DBG         JTDI_EN           LL_GPIO_IsEnabledPinJTDI
 * @param  GPIO_DBGx GPIO_DBGx Instance
 * @return State of bit, 1: enable, 0: disable.
 */
__STATIC_INLINE uint32_t LL_GPIO_IsEnabledPinJTDI(GPIO_DBG_TypeDef *GPIO_DBGx)
{
    return (READ_BIT(GPIO_DBGx->DBG, LL_GPIO_JTDI) == (LL_GPIO_JTDI));
}

/**
 * @brief  Enable JTMS function of PG15
 * @note   IO priority: JTAG IO > GPIO. Enable JTAG function, the GPIO configuration will be disabled
 * @rmtoll DBG         JTMS_EN           LL_GPIO_EnablePinJTMS
 * @param  GPIO_DBGx GPIO_DBGx Instance
 * @return  None
 */
__STATIC_INLINE void LL_GPIO_EnablePinJTMS(GPIO_DBG_TypeDef *GPIO_DBGx)
{
    SET_BIT(GPIO_DBGx->DBG, LL_GPIO_JTMS);
}

/**
 * @brief  Disable JTMS function of PG15, PG15 is used to a general IO
 * @rmtoll DBG         JTMS_EN           LL_GPIO_DisablePinJTMS
 * @param  GPIO_DBGx GPIO_DBGx Instance
 * @return  None
 */
__STATIC_INLINE void LL_GPIO_DisablePinJTMS(GPIO_DBG_TypeDef *GPIO_DBGx)
{
    CLEAR_BIT(GPIO_DBGx->DBG, LL_GPIO_JTMS);
}

/**
 * @brief  Return 1 if PG15 is enabled to JTMS. else return 0.
 * @rmtoll DBG         JTMS_EN           LL_GPIO_IsEnabledPinJTMS
 * @param  GPIO_DBGx GPIO_DBGx Instance
 * @return State of bit, 1: enable, 0: disable.
 */
__STATIC_INLINE uint32_t LL_GPIO_IsEnabledPinJTMS(GPIO_DBG_TypeDef *GPIO_DBGx)
{
    return (READ_BIT(GPIO_DBGx->DBG, LL_GPIO_JTMS) == (LL_GPIO_JTMS));
}

/**
 * @brief  Enable JTCK function of PG14
 * @note   IO priority: JTAG IO > GPIO. Enable JTAG function, the GPIO configuration will be disabled
 * @rmtoll DBG         JTCK_EN           LL_GPIO_EnablePinJTCK
 * @param  GPIO_DBGx GPIO_DBGx Instance
 * @return  None
 */
__STATIC_INLINE void LL_GPIO_EnablePinJTCK(GPIO_DBG_TypeDef *GPIO_DBGx)
{
    SET_BIT(GPIO_DBGx->DBG, LL_GPIO_JTCK);
}

/**
 * @brief  Disable JTCK function of PG14, PG14 is used to a general IO
 * @rmtoll DBG         JTCK_EN           LL_GPIO_DisablePinJTCK
 * @param  GPIO_DBGx GPIO_DBGx Instance
 * @return  None
 */
__STATIC_INLINE void LL_GPIO_DisablePinJTCK(GPIO_DBG_TypeDef *GPIO_DBGx)
{
    CLEAR_BIT(GPIO_DBGx->DBG, LL_GPIO_JTCK);
}

/**
 * @brief  Return 1 if PG14 is enabled to JTCK. else return 0.
 * @rmtoll DBG         JTCK_EN           LL_GPIO_IsEnabledPinJTCK
 * @param  GPIO_DBGx GPIO_DBGx Instance
 * @return State of bit, 1: enable, 0: disable.
 */
__STATIC_INLINE uint32_t LL_GPIO_IsEnabledPinJTCK(GPIO_DBG_TypeDef *GPIO_DBGx)
{
    return (READ_BIT(GPIO_DBGx->DBG, LL_GPIO_JTCK) == (LL_GPIO_JTCK));
}
/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup GPIO_LL_EF_Init Initialization and de-initialization functions
 * @{
 */
ErrorStatus LL_GPIO_DeInit(GPIO_TypeDef *GPIOx);
ErrorStatus LL_GPIO_Init(GPIO_TypeDef *GPIOx, LL_GPIO_InitTypeDef *GPIO_InitStruct);
void        LL_GPIO_StructInit(LL_GPIO_InitTypeDef *GPIO_InitStruct);
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

#endif /* GPIOA || GPIOB || GPIOC || GPIOD || GPIOE || GPIOF || GPIOG || GPIOH || GPIOI || GPIOJ || GPIOK || GPIO_DBG */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_RV_LL_GPIO_H */
