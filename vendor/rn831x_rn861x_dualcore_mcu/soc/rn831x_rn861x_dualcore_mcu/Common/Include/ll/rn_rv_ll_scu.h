/**
 *****************************************************************************
 * @file   rn_rv_ll_scu.h
 * @author Renergy Software Team
 * @brief  Header file of SCU LL module.
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
#ifndef __RN_LL_SCU_H
#define __RN_LL_SCU_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_scu.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (SCU)

/** @defgroup SCU_LL SCU
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup SCU_LL_Private_Constants SCU Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup SCU_LL_Private_Macros SCU Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup SCU_LL_ES_INIT SCU Exported Init structure
 * @{
 */

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup SCU_LL_Exported_Constants SCU Exported Constants
 * @{
 */

/** @defgroup SCU_LL_EC_SCU_CORE_ID SCU_CORE_ID
 * @{
 */
#define LL_SCU_SECURE                    0x00000001U                        /*!< Peripheral is Secure     */
#define LL_SCU_NON_SECURE                0x00000000U                        /*!< Peripheral is Non-Secure */
/**
 * @}
 */

/** @defgroup SCU_LL_EC_PERIPH0  PERIPH0
 * @{
 */
#define LL_SCU_PERIPH_UART0          SCU_PERIPH_SEC_CFG0_UART0SEC
#define LL_SCU_PERIPH_UART1          SCU_PERIPH_SEC_CFG0_UART1SEC
#define LL_SCU_PERIPH_UART2          SCU_PERIPH_SEC_CFG0_UART2SEC
#define LL_SCU_PERIPH_UART3          SCU_PERIPH_SEC_CFG0_UART3SEC
#define LL_SCU_PERIPH_UART4          SCU_PERIPH_SEC_CFG0_UART4SEC
#define LL_SCU_PERIPH_UART5          SCU_PERIPH_SEC_CFG0_UART5SEC
#define LL_SCU_PERIPH_UART6          SCU_PERIPH_SEC_CFG0_UART6SEC
#define LL_SCU_PERIPH_UART7          SCU_PERIPH_SEC_CFG0_UART7SEC
#define LL_SCU_PERIPH_LPUART         SCU_PERIPH_SEC_CFG0_LPUARTSEC
#define LL_SCU_PERIPH_USART0         SCU_PERIPH_SEC_CFG0_USART0SEC
#define LL_SCU_PERIPH_USART1         SCU_PERIPH_SEC_CFG0_USART1SEC
#define LL_SCU_PERIPH_ISO78160       SCU_PERIPH_SEC_CFG0_ISO78160SEC
#define LL_SCU_PERIPH_ISO78161       SCU_PERIPH_SEC_CFG0_ISO78161SEC
#define LL_SCU_PERIPH_SPI0           SCU_PERIPH_SEC_CFG0_SPI0SEC
#define LL_SCU_PERIPH_SPI1           SCU_PERIPH_SEC_CFG0_SPI1SEC
#define LL_SCU_PERIPH_SPI2           SCU_PERIPH_SEC_CFG0_SPI2SEC
#define LL_SCU_PERIPH_SPI3           SCU_PERIPH_SEC_CFG0_SPI3SEC
#define LL_SCU_PERIPH_I2C0           SCU_PERIPH_SEC_CFG0_I2C0SEC
#define LL_SCU_PERIPH_I2C1           SCU_PERIPH_SEC_CFG0_I2C1SEC
#define LL_SCU_PERIPH_CAN            SCU_PERIPH_SEC_CFG0_CANSEC
#define LL_SCU_PERIPH_IOCNT          SCU_PERIPH_SEC_CFG0_IOCNTSEC
#define LL_SCU_PERIPH_LCD            SCU_PERIPH_SEC_CFG0_LCDSEC
#define LL_SCU_PERIPH_DSP            SCU_PERIPH_SEC_CFG0_DSPSEC
#define LL_SCU_PERIPH_M2M            SCU_PERIPH_SEC_CFG0_M2MSEC
#define LL_SCU_PERIPH_D2F            SCU_PERIPH_SEC_CFG0_D2FSEC
#define LL_SCU_PERIPH_ADC0           SCU_PERIPH_SEC_CFG0_ADC0SEC
#define LL_SCU_PERIPH_DAC            SCU_PERIPH_SEC_CFG0_DACSEC
#define LL_SCU_PERIPH_TC0            SCU_PERIPH_SEC_CFG0_TC0SEC
#define LL_SCU_PERIPH_TC1            SCU_PERIPH_SEC_CFG0_TC1SEC
/**
 * @}
 */

/** @defgroup SCU_LL_EC_PERIPH1  PERIPH1
 * @{
 */
#define LL_SCU_PERIPH_SIMPTC0         SCU_PERIPH_SEC_CFG1_SIMPTC0SEC
#define LL_SCU_PERIPH_SIMPTC1         SCU_PERIPH_SEC_CFG1_SIMPTC1SEC
#define LL_SCU_PERIPH_SIMPTC2         SCU_PERIPH_SEC_CFG1_SIMPTC2SEC
#define LL_SCU_PERIPH_SIMPTC3         SCU_PERIPH_SEC_CFG1_SIMPTC3SEC
#define LL_SCU_PERIPH_RS_VDD          SCU_PERIPH_SEC_CFG1_RS_VDDSEC              /*!< Inlcude RTC, SARADC, LCSS_VBAT and GPIOL[3:0] */
#define LL_SCU_PERIPH_LSRX            SCU_PERIPH_SEC_CFG1_LSRXSEC
#define LL_SCU_PERIPH_KBI0            SCU_PERIPH_SEC_CFG1_KBI0SEC
#define LL_SCU_PERIPH_KBI1            SCU_PERIPH_SEC_CFG1_KBI1SEC
#define LL_SCU_PERIPH_LVDCMP          SCU_PERIPH_SEC_CFG1_LVD_CMPSEC
#define LL_SCU_PERIPH_CRC0            SCU_PERIPH_SEC_CFG1_CRC0SEC
/**
 * @}
 */

/** @defgroup SCU_LL_EC_PERIPH2  PERIPH2
 * @{
 */
#define LL_SCU_PERIPH_QSPI0           SCU_PERIPH_SEC_CFG2_QSPI0SEC
#define LL_SCU_PERIPH_QSPI1           SCU_PERIPH_SEC_CFG2_QSPI1SEC
#define LL_SCU_PERIPH_SEA             SCU_PERIPH_SEC_CFG2_SEASEC
#define LL_SCU_PERIPH_AES             SCU_PERIPH_SEC_CFG2_AESSEC
/**
 * @}
 */

/** @defgroup SCU_LL_EC_CFG_GPIO  CFG GPIO
 * @{
 */
#define LL_SCU_GPIO_A          0x00000000U        /*!< Select pin GPIOA */
#define LL_SCU_GPIO_B          0x00000001U        /*!< Select pin GPIOB */
#define LL_SCU_GPIO_C          0x00000002U        /*!< Select pin GPIOC */
#define LL_SCU_GPIO_D          0x00000003U        /*!< Select pin GPIOD */
#define LL_SCU_GPIO_E          0x00000004U        /*!< Select pin GPIOE */
#define LL_SCU_GPIO_F          0x00000005U        /*!< Select pin GPIOF */
#define LL_SCU_GPIO_G          0x00000006U        /*!< Select pin GPIOG */
#define LL_SCU_GPIO_H          0x00000007U        /*!< Select pin GPIOH */
/**
 * @}
 */

/** @defgroup SCU_LL_EC_CFG_GPIO  CFG GPIO
 * @{
 */
#define LL_SCU_GPIO_PIN_00             0x00000001U        /*!< Select pin 00 */
#define LL_SCU_GPIO_PIN_01             0x00000002U        /*!< Select pin 01 */
#define LL_SCU_GPIO_PIN_02             0x00000004U        /*!< Select pin 02 */
#define LL_SCU_GPIO_PIN_03             0x00000008U        /*!< Select pin 03 */
#define LL_SCU_GPIO_PIN_04             0x00000010U        /*!< Select pin 04 */
#define LL_SCU_GPIO_PIN_05             0x00000020U        /*!< Select pin 05 */
#define LL_SCU_GPIO_PIN_06             0x00000040U        /*!< Select pin 06 */
#define LL_SCU_GPIO_PIN_07             0x00000080U        /*!< Select pin 07 */
#define LL_SCU_GPIO_PIN_08             0x00000100U        /*!< Select pin 08 */
#define LL_SCU_GPIO_PIN_09             0x00000200U        /*!< Select pin 09 */
#define LL_SCU_GPIO_PIN_10             0x00000400U        /*!< Select pin 10 */
#define LL_SCU_GPIO_PIN_11             0x00000800U        /*!< Select pin 11 */
#define LL_SCU_GPIO_PIN_12             0x00001000U        /*!< Select pin 12 */
#define LL_SCU_GPIO_PIN_13             0x00002000U        /*!< Select pin 13 */
#define LL_SCU_GPIO_PIN_14             0x00004000U        /*!< Select pin 14 */
#define LL_SCU_GPIO_PIN_15             0x00008000U        /*!< Select pin 15 */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup SCU_LL_Exported_Macros SCU Exported Macros
 * @{
 */
/** @defgroup SCU_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in SCU register
 * @param  __INSTANCE__ SCU Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_SCU_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in SCU register
 * @param  __INSTANCE__ SCU Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_SCU_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup SCU_LL_Exported_Functions SCU Exported Functions
 * @{
 */

/** @defgroup SCU_LL_EF_Management Management
 * @{
 */

/**
 * @brief  Unlock Password protection
 * @note   Write operations to SCU related registers (04H~68H) are valid.
 * @rmtoll WKEY         WKEY         LL_SCU_UnlockWriteKey
 * @retval None
 */
__STATIC_INLINE void LL_SCU_UnlockWriteKey(void)
{
    MODIFY_REG(SCU_CORE0->WKEY, SCU_WKEY_WKEY, 0xA5387492U);
}

/**
 * @brief  Lock Password protection
 * @note   Write operations to SCU related registers (04H~68H) are invalid.
 * @rmtoll WKEY         WKEY         LL_SCU_LockWriteKey
 * @retval None
 */
__STATIC_INLINE void LL_SCU_LockWriteKey(void)
{
    MODIFY_REG(SCU_CORE0->WKEY, SCU_WKEY_WKEY, 0x00000000U);
}

/**
 * @brief  Check if Password protection has been Unloced or not
 * @rmtoll WKEY         WKEY         LL_SCU_IsUnlockedWriteKey
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SCU_IsUnlockedWriteKey(void)
{
    return (READ_BIT(SCU_CORE0->WKEY, SCU_WKEY_WKEY) == (0xA5387492U));
}

/**
 * @brief  Lock SCU security attribute (08H~5CH) configuration
 * @note   Once locked, it cannot be unlocked by writing other values,
 *         only reset can unlock it.
 * @rmtoll LCK         LCK         LL_GIC_LockSecureConfig
 * @retval None
 */
__STATIC_INLINE void LL_GIC_LockSecureConfig(void)
{
    MODIFY_REG(SCU_CORE0->LCK, SCU_LCK_LCK, 0xF52AB874U);
}

/**
 * @brief  Check if SCU security attribute (08H~5CH) configuration has been Loced or not.
 * @note   Once locked, it cannot be unlocked by writing other values,
 *         only reset can unlock it.
 * @rmtoll LCK         LCK         LL_SCU_IsLockedSecureConfig
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SCU_IsLockedSecureConfig(void)
{
    return (READ_BIT(SCU_CORE0->LCK, SCU_LCK_LCK) == (0xF52AB874U));
}

/**
 * @brief  Set Peripheral Security Configuration0
 * @note   Can only use in core0.
 * @rmtoll PERIPH_SEC_CFG0      UART0SEC           LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      UART1SEC           LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      UART2SEC           LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      UART3SEC           LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      UART4SEC           LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      UART5SEC           LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      UART6SEC           LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      UART7SEC           LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      LPUARTSEC          LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      USART0SEC          LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      USART1SEC          LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      ISO78160SEC        LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      ISO78161SEC        LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      SPI0SEC            LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      SPI1SEC            LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      SPI2SEC            LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      SPI3SEC            LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      I2C0SEC            LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      I2C1SEC            LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      CANSEC             LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      IOCNTSEC           LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      LCDSEC             LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      DSPSEC             LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      M2MSEC             LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      D2FSEC             LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      ADC0SEC            LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      DACSEC             LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      TC0SEC             LL_SCU_SetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      TC1SEC             LL_SCU_SetPeripheralSecurityConfig0
 * @param  Periph This parameter can be one of the following values:
 *         @arg @ref LL_SCU_PERIPH_UART0
 *         @arg @ref LL_SCU_PERIPH_UART1
 *         @arg @ref LL_SCU_PERIPH_UART2
 *         @arg @ref LL_SCU_PERIPH_UART3
 *         @arg @ref LL_SCU_PERIPH_UART4
 *         @arg @ref LL_SCU_PERIPH_UART5
 *         @arg @ref LL_SCU_PERIPH_UART6
 *         @arg @ref LL_SCU_PERIPH_UART7
 *         @arg @ref LL_SCU_PERIPH_LPUART
 *         @arg @ref LL_SCU_PERIPH_USART0
 *         @arg @ref LL_SCU_PERIPH_USART1
 *         @arg @ref LL_SCU_PERIPH_ISO78160
 *         @arg @ref LL_SCU_PERIPH_ISO78161
 *         @arg @ref LL_SCU_PERIPH_SPI0
 *         @arg @ref LL_SCU_PERIPH_SPI1
 *         @arg @ref LL_SCU_PERIPH_SPI2
 *         @arg @ref LL_SCU_PERIPH_SPI3
 *         @arg @ref LL_SCU_PERIPH_I2C0
 *         @arg @ref LL_SCU_PERIPH_I2C1
 *         @arg @ref LL_SCU_PERIPH_CAN
 *         @arg @ref LL_SCU_PERIPH_IOCNT
 *         @arg @ref LL_SCU_PERIPH_LCD
 *         @arg @ref LL_SCU_PERIPH_DSP
 *         @arg @ref LL_SCU_PERIPH_M2M
 *         @arg @ref LL_SCU_PERIPH_D2F
 *         @arg @ref LL_SCU_PERIPH_ADC0
 *         @arg @ref LL_SCU_PERIPH_DAC
 *         @arg @ref LL_SCU_PERIPH_TC0
 *         @arg @ref LL_SCU_PERIPH_TC1
 * @param  Security This parameter can be one of the following values:
 *         @arg @ref LL_SCU_SECURE
 *         @arg @ref LL_SCU_NON_SECURE
 * @retval None
 */
__STATIC_INLINE void LL_SCU_SetPeripheralSecurityConfig0(uint32_t Periph, uint32_t Security)
{
    MODIFY_REG(SCU_CORE0->PERIPH_SEC_CFG0, Periph, Security << (POSITION_VAL(Periph)));
}

/**
 * @brief  Get Peripheral Security Configuration0.
 * @note   Can use in core0 and core1.
 * @rmtoll PERIPH_SEC_CFG0      UART0SEC           LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      UART1SEC           LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      UART2SEC           LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      UART3SEC           LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      UART4SEC           LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      UART5SEC           LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      UART6SEC           LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      UART7SEC           LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      LPUARTSEC          LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      USART0SEC          LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      USART1SEC          LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      ISO78160SEC        LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      ISO78161SEC        LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      SPI0SEC            LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      SPI1SEC            LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      SPI2SEC            LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      SPI3SEC            LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      I2C0SEC            LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      I2C1SEC            LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      CANSEC             LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      IOCNTSEC           LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      LCDSEC             LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      DSPSEC             LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      M2MSEC             LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      D2FSEC             LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      ADC0SEC            LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      DACSEC             LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      TC0SEC             LL_SCU_GetPeripheralSecurityConfig0\n
 *         PERIPH_SEC_CFG0      TC1SEC             LL_SCU_GetPeripheralSecurityConfig0
 * @param  SCUx SCU Instance
 * @param  Periph This parameter can be one of the following values:
 *         @arg @ref LL_SCU_PERIPH_UART0
 *         @arg @ref LL_SCU_PERIPH_UART1
 *         @arg @ref LL_SCU_PERIPH_UART2
 *         @arg @ref LL_SCU_PERIPH_UART3
 *         @arg @ref LL_SCU_PERIPH_UART4
 *         @arg @ref LL_SCU_PERIPH_UART5
 *         @arg @ref LL_SCU_PERIPH_UART6
 *         @arg @ref LL_SCU_PERIPH_UART7
 *         @arg @ref LL_SCU_PERIPH_LPUART
 *         @arg @ref LL_SCU_PERIPH_USART0
 *         @arg @ref LL_SCU_PERIPH_USART1
 *         @arg @ref LL_SCU_PERIPH_ISO78160
 *         @arg @ref LL_SCU_PERIPH_ISO78161
 *         @arg @ref LL_SCU_PERIPH_SPI0
 *         @arg @ref LL_SCU_PERIPH_SPI1
 *         @arg @ref LL_SCU_PERIPH_SPI2
 *         @arg @ref LL_SCU_PERIPH_SPI3
 *         @arg @ref LL_SCU_PERIPH_I2C0
 *         @arg @ref LL_SCU_PERIPH_I2C1
 *         @arg @ref LL_SCU_PERIPH_CAN
 *         @arg @ref LL_SCU_PERIPH_IOCNT
 *         @arg @ref LL_SCU_PERIPH_LCD
 *         @arg @ref LL_SCU_PERIPH_DSP
 *         @arg @ref LL_SCU_PERIPH_M2M
 *         @arg @ref LL_SCU_PERIPH_D2F
 *         @arg @ref LL_SCU_PERIPH_ADC0
 *         @arg @ref LL_SCU_PERIPH_DAC
 *         @arg @ref LL_SCU_PERIPH_TC0
 *         @arg @ref LL_SCU_PERIPH_TC1
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SCU_SECURE
 *         @arg @ref LL_SCU_NON_SECURE
 */
__STATIC_INLINE uint32_t LL_SCU_GetPeripheralSecurityConfig0(SCU_TypeDef *SCUx, uint32_t Periph)
{
    return (uint32_t)(READ_BIT(SCUx->PERIPH_SEC_CFG0, Periph) >> (POSITION_VAL(Periph)));
}

/**
 * @brief  Set Peripheral Security Configuration1
 * @note   Can only use in core0.
 * @rmtoll PERIPH_SEC_CFG1        SIMPTC0SEC            LL_SCU_SetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        SIMPTC1SEC            LL_SCU_SetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        SIMPTC2SEC            LL_SCU_SetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        SIMPTC3SEC            LL_SCU_SetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        RS_VDDSEC             LL_SCU_SetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        LSRXSEC               LL_SCU_SetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        KBI0SEC               LL_SCU_SetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        KBI1SEC               LL_SCU_SetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        LVDCMPSEC             LL_SCU_SetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        CRC0SEC               LL_SCU_SetPeripheralSecurityConfig1
 * @param  Periph This parameter can be one of the following values:
 *         @arg @ref LL_SCU_PERIPH_SIMPTC0
 *         @arg @ref LL_SCU_PERIPH_SIMPTC1
 *         @arg @ref LL_SCU_PERIPH_SIMPTC2
 *         @arg @ref LL_SCU_PERIPH_SIMPTC3
 *         @arg @ref LL_SCU_PERIPH_RS_VDD
 *         @arg @ref LL_SCU_PERIPH_LSRX
 *         @arg @ref LL_SCU_PERIPH_KBI0
 *         @arg @ref LL_SCU_PERIPH_KBI1
 *         @arg @ref LL_SCU_PERIPH_LVDCMP
 *         @arg @ref LL_SCU_PERIPH_CRC0
 * @param  Security This parameter can be one of the following values:
 *         @arg @ref LL_SCU_SECURE
 *         @arg @ref LL_SCU_NON_SECURE
 * @retval None
 */
__STATIC_INLINE void LL_SCU_SetPeripheralSecurityConfig1(uint32_t Periph, uint32_t Security)
{
    MODIFY_REG(SCU_CORE0->PERIPH_SEC_CFG1, Periph, Security << (POSITION_VAL(Periph)));
}

/**
 * @brief  Get Peripheral Security Configuration1
 * @note   Can use in core0 and core1.
 * @rmtoll PERIPH_SEC_CFG1        SIMPTC0SEC            LL_SCU_GetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        SIMPTC1SEC            LL_SCU_GetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        SIMPTC2SEC            LL_SCU_GetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        SIMPTC3SEC            LL_SCU_GetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        RS_VDDSEC             LL_SCU_GetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        LSRXSEC               LL_SCU_GetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        KBI0SEC               LL_SCU_GetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        KBI1SEC               LL_SCU_GetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        LVDCMPSEC             LL_SCU_GetPeripheralSecurityConfig1\n
 *         PERIPH_SEC_CFG1        CRC0SEC               LL_SCU_GetPeripheralSecurityConfig1
 * @param  SCUx SCU Instance
 * @param  Periph This parameter can be one of the following values:
 *         @arg @ref LL_SCU_PERIPH_SIMPTC0
 *         @arg @ref LL_SCU_PERIPH_SIMPTC1
 *         @arg @ref LL_SCU_PERIPH_SIMPTC2
 *         @arg @ref LL_SCU_PERIPH_SIMPTC3
 *         @arg @ref LL_SCU_PERIPH_RS_VDD
 *         @arg @ref LL_SCU_PERIPH_LSRX
 *         @arg @ref LL_SCU_PERIPH_KBI0
 *         @arg @ref LL_SCU_PERIPH_KBI1
 *         @arg @ref LL_SCU_PERIPH_LVDCMP
 *         @arg @ref LL_SCU_PERIPH_CRC0
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SCU_SECURE
 *         @arg @ref LL_SCU_NON_SECURE
 */
__STATIC_INLINE uint32_t LL_SCU_GetPeripheralSecurityConfig1(SCU_TypeDef *SCUx, uint32_t Periph)
{
    return (uint32_t)(READ_BIT(SCUx->PERIPH_SEC_CFG1, Periph) >> (POSITION_VAL(Periph)));
}

/**
 * @brief  Set Peripheral Security Configuration2
 * @note   Can only use in core0.
 * @rmtoll PERIPH_SEC_CFG2      QSPI0SEC           LL_SCU_SetPeripheralSecurityConfig2\n
 *         PERIPH_SEC_CFG2      QSPI1SEC           LL_SCU_SetPeripheralSecurityConfig2\n
 *         PERIPH_SEC_CFG2      SEASEC             LL_SCU_SetPeripheralSecurityConfig2\n
 *         PERIPH_SEC_CFG2      AESSEC             LL_SCU_SetPeripheralSecurityConfig2
 * @param  Periph This parameter can be one of the following values:
 *         @arg @ref LL_SCU_PERIPH_QSPI0
 *         @arg @ref LL_SCU_PERIPH_QSPI1
 *         @arg @ref LL_SCU_PERIPH_SEA
 *         @arg @ref LL_SCU_PERIPH_AES
 * @param  Security This parameter can be one of the following values:
 *         @arg @ref LL_SCU_SECURE
 *         @arg @ref LL_SCU_NON_SECURE
 * @retval None
 */
__STATIC_INLINE void LL_SCU_SetPeripheralSecurityConfig2(uint32_t Periph, uint32_t Security)
{
    MODIFY_REG(SCU_CORE0->PERIPH_SEC_CFG2, Periph, Security << (POSITION_VAL(Periph)));
}

/**
 * @brief  Get Peripheral Security Configuration2.
 * @note   Can use in core0 ande core1.
 * @rmtoll PERIPH_SEC_CFG2      QSPI0SEC           LL_SCU_GetPeripheralSecurityConfig2\n
 *         PERIPH_SEC_CFG2      QSPI1SEC           LL_SCU_GetPeripheralSecurityConfig2\n
 *         PERIPH_SEC_CFG2      SEASEC             LL_SCU_GetPeripheralSecurityConfig2\n
 *         PERIPH_SEC_CFG2      AESSEC             LL_SCU_GetPeripheralSecurityConfig2
 * @param  SCUx SCU Instance
 * @param  Periph This parameter can be one of the following values:
 *         @arg @ref LL_SCU_PERIPH_QSPI0
 *         @arg @ref LL_SCU_PERIPH_QSPI1
 *         @arg @ref LL_SCU_PERIPH_SEA
 *         @arg @ref LL_SCU_PERIPH_AES
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SCU_SECURE
 *         @arg @ref LL_SCU_NON_SECURE
 */
__STATIC_INLINE uint32_t LL_SCU_GetPeripheralSecurityConfig2(SCU_TypeDef *SCUx, uint32_t Periph)
{
    return (uint32_t)(READ_BIT(SCUx->PERIPH_SEC_CFG0, Periph) >> (POSITION_VAL(Periph)));
}

/**
 * @brief  Set GPIOx Security Configuration.
 * @note   Can only use in core0.
 * @rmtoll SCU_GPIOx_SEC_CFG      Px_SEC           LL_SCU_SetGPIOSecurityConfig
 * @param  GPIOx This parameter can be one of the following values:
 *         @arg @ref LL_SCU_GPIO_A
 *         @arg @ref LL_SCU_GPIO_B
 *         @arg @ref LL_SCU_GPIO_C
 *         @arg @ref LL_SCU_GPIO_D
 *         @arg @ref LL_SCU_GPIO_E
 *         @arg @ref LL_SCU_GPIO_F
 *         @arg @ref LL_SCU_GPIO_G
 *         @arg @ref LL_SCU_GPIO_H  (*)
 *
 *         (*) GPIO_H only have PIN_0 and PIN_1.
 * @param  Pin This parameter can be one of the following values:
 *         @arg @ref LL_SCU_GPIO_PIN_00 (*)
 *         @arg @ref LL_SCU_GPIO_PIN_01 (*)
 *         @arg @ref LL_SCU_GPIO_PIN_02
 *         @arg @ref LL_SCU_GPIO_PIN_03
 *         @arg @ref LL_SCU_GPIO_PIN_04
 *         @arg @ref LL_SCU_GPIO_PIN_05
 *         @arg @ref LL_SCU_GPIO_PIN_06
 *         @arg @ref LL_SCU_GPIO_PIN_07
 *         @arg @ref LL_SCU_GPIO_PIN_08
 *         @arg @ref LL_SCU_GPIO_PIN_09
 *         @arg @ref LL_SCU_GPIO_PIN_10
 *         @arg @ref LL_SCU_GPIO_PIN_11
 *         @arg @ref LL_SCU_GPIO_PIN_12
 *         @arg @ref LL_SCU_GPIO_PIN_13
 *         @arg @ref LL_SCU_GPIO_PIN_14
 *         @arg @ref LL_SCU_GPIO_PIN_15
 *
 *         (*) GPIO_H only have PIN_00 and PIN_01.
 * @param  Security This parameter can be one of the following values:
 *         @arg @ref LL_SCU_SECURE
 *         @arg @ref LL_SCU_NON_SECURE
 * @retval None
 */
__STATIC_INLINE void LL_SCU_SetGPIOSecurityConfig(uint32_t GPIOx, uint32_t Pin, uint32_t Security)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&SCU_CORE0->GPIOA_SEC_CFG) + (4 * GPIOx)));
    MODIFY_REG(*pReg, Pin, Security << (POSITION_VAL(Pin)));
}

/**
 * @brief  Get GPIOx Security Configuration.
 * @note   Can use in core0 ande core1.
 * @rmtoll SCU_GPIOx_SEC_CFG      Px_SEC           LL_SCU_GetGPIOSecurityConfig
 * @param  SCUx SCU Instance
 * @param  GPIOx This parameter can be one of the following values:
 *         @arg @ref LL_SCU_GPIO_A
 *         @arg @ref LL_SCU_GPIO_B
 *         @arg @ref LL_SCU_GPIO_C
 *         @arg @ref LL_SCU_GPIO_D
 *         @arg @ref LL_SCU_GPIO_E
 *         @arg @ref LL_SCU_GPIO_F
 *         @arg @ref LL_SCU_GPIO_G
 *         @arg @ref LL_SCU_GPIO_H (*)
 *
 *         (*) GPIO_H only have PIN_0 and PIN_1.
 * @param  Pin This parameter can be one of the following values:
 *         @arg @ref LL_SCU_GPIO_PIN_00 (*)
 *         @arg @ref LL_SCU_GPIO_PIN_01 (*)
 *         @arg @ref LL_SCU_GPIO_PIN_02
 *         @arg @ref LL_SCU_GPIO_PIN_03
 *         @arg @ref LL_SCU_GPIO_PIN_04
 *         @arg @ref LL_SCU_GPIO_PIN_05
 *         @arg @ref LL_SCU_GPIO_PIN_06
 *         @arg @ref LL_SCU_GPIO_PIN_07
 *         @arg @ref LL_SCU_GPIO_PIN_08
 *         @arg @ref LL_SCU_GPIO_PIN_09
 *         @arg @ref LL_SCU_GPIO_PIN_10
 *         @arg @ref LL_SCU_GPIO_PIN_11
 *         @arg @ref LL_SCU_GPIO_PIN_12
 *         @arg @ref LL_SCU_GPIO_PIN_13
 *         @arg @ref LL_SCU_GPIO_PIN_14
 *         @arg @ref LL_SCU_GPIO_PIN_15
 *
 *         (*) GPIO_H only have PIN_00 and PIN_01.
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SCU_SECURE
 *         @arg @ref LL_SCU_NON_SECURE
 */
__STATIC_INLINE uint32_t LL_SCU_GetGPIOSecurityConfig(SCU_TypeDef *SCUx, uint32_t GPIOx, uint32_t Pin)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&SCUx->GPIOA_SEC_CFG) + (4 * GPIOx)));
    return (uint32_t)(READ_BIT(*pReg, Pin) >> (POSITION_VAL(Pin)));
}

/**
 * @brief  Set the Secure SRAM region length configuration.
 * @note   can only use in core0. Secure SRAM region length is SRAM_SEC_LEN*16KB,
 *         Non-Secure SRAM region length is (10-SRAM_SEC_LEN)*16KB.
 *         Set to 0 or 4'hb~4'hf is invalid.
 * @rmtoll SRAM_SEC_WM_CFG      SRAM_SEC_LEN           LL_SCU_SetSecuritySRAMWatermark
 * @param  Size This parameter must be a number between Min_Data = 0x01 and Max_Data = 0x0A.
 * @retval None
 */
__STATIC_INLINE void LL_SCU_SetSecuritySRAMWatermark(uint32_t Size)
{
    MODIFY_REG(SCU_CORE0->SRAM_SEC_WM_CFG, SCU_SRAM_SEC_WM_CFG_SRAM_SEC_LEN, Size << SCU_SRAM_SEC_WM_CFG_SRAM_SEC_LEN_POS);
}

/**
 * @brief  Get the Secure SRAM region length configuration.
 * @note   Can use in core0 ande core1. Secure SRAM region length is SRAM_SEC_LEN*16KB,
 *         Non-Secure SRAM region length is (10-SRAM_SEC_LEN)*16KB.
 *         Set to 0 or 4'hb~4'hf is invalid.
 * @rmtoll SRAM_SEC_WM_CFG      SRAM_SEC_LEN           LL_SCU_GetSecuritySRAMWatermark
 * @param  SCUx SCU Instance
 * @retval Returned value is a number between Min_Data = 0x01 and Max_Data = 0x0A.
 */
__STATIC_INLINE uint32_t LL_SCU_GetSecuritySRAMWatermark(SCU_TypeDef *SCUx)
{
    return (uint32_t)(READ_BIT(SCUx->SRAM_SEC_WM_CFG, SCU_SRAM_SEC_WM_CFG_SRAM_SEC_LEN) >> SCU_SRAM_SEC_WM_CFG_SRAM_SEC_LEN_POS);
}

/**
 * @}
 */

/** @defgroup SCU_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable SCU Peripheral Illegal Access Interrupt0
 * @rmtoll PERIPH_SEC_IE0      UART0IE           LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART1IE           LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART2IE           LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART3IE           LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART4IE           LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART5IE           LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART6IE           LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART7IE           LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      LPUARTIE          LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      USART0IE          LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      USART1IE          LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      ISO78160IE        LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      ISO78161IE        LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      SPI0IE            LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      SPI1IE            LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      SPI2IE            LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      SPI3IE            LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      I2C0IE            LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      I2C1IE            LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      CANIE             LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      IOCNTIE           LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      LCDIE             LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      DSPIE             LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      M2MIE             LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      D2FIE             LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      ADC0IE            LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      DACIE             LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      TC0IE             LL_SCU_EnablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      TC1IE             LL_SCU_EnablePeripheralSecurityIT_IE0
 * @param  Periph This parameter can be a combination of the following values:
 *         @arg @ref LL_SCU_PERIPH_UART0
 *         @arg @ref LL_SCU_PERIPH_UART1
 *         @arg @ref LL_SCU_PERIPH_UART2
 *         @arg @ref LL_SCU_PERIPH_UART3
 *         @arg @ref LL_SCU_PERIPH_UART4
 *         @arg @ref LL_SCU_PERIPH_UART5
 *         @arg @ref LL_SCU_PERIPH_UART6
 *         @arg @ref LL_SCU_PERIPH_UART7
 *         @arg @ref LL_SCU_PERIPH_LPUART
 *         @arg @ref LL_SCU_PERIPH_USART0
 *         @arg @ref LL_SCU_PERIPH_USART1
 *         @arg @ref LL_SCU_PERIPH_ISO78160
 *         @arg @ref LL_SCU_PERIPH_ISO78161
 *         @arg @ref LL_SCU_PERIPH_SPI0
 *         @arg @ref LL_SCU_PERIPH_SPI1
 *         @arg @ref LL_SCU_PERIPH_SPI2
 *         @arg @ref LL_SCU_PERIPH_SPI3
 *         @arg @ref LL_SCU_PERIPH_I2C0
 *         @arg @ref LL_SCU_PERIPH_I2C1
 *         @arg @ref LL_SCU_PERIPH_CAN
 *         @arg @ref LL_SCU_PERIPH_IOCNT
 *         @arg @ref LL_SCU_PERIPH_LCD
 *         @arg @ref LL_SCU_PERIPH_DSP
 *         @arg @ref LL_SCU_PERIPH_M2M
 *         @arg @ref LL_SCU_PERIPH_D2F
 *         @arg @ref LL_SCU_PERIPH_ADC0
 *         @arg @ref LL_SCU_PERIPH_DAC
 *         @arg @ref LL_SCU_PERIPH_TC0
 *         @arg @ref LL_SCU_PERIPH_TC1
 * @retval None
 */
__STATIC_INLINE void LL_SCU_EnablePeripheralSecurityIT_IE0(uint32_t Periph)
{
    SET_BIT(SCU_CORE0->PERIPH_SEC_IE0, Periph);
}

/**
 * @brief  Disable SCU Peripheral Illegal Access Interrupt0
 * @rmtoll PERIPH_SEC_IE0      UART0IE           LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART1IE           LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART2IE           LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART3IE           LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART4IE           LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART5IE           LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART6IE           LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART7IE           LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      LPUARTIE          LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      USART0IE          LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      USART1IE          LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      ISO78160IE        LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      ISO78161IE        LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      SPI0IE            LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      SPI1IE            LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      SPI2IE            LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      SPI3IE            LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      I2C0IE            LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      I2C1IE            LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      CANIE             LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      IOCNTIE           LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      LCDIE             LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      DSPIE             LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      M2MIE             LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      D2FIE             LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      ADC0IE            LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      DACIE             LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      TC0IE             LL_SCU_DisablePeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      TC1IE             LL_SCU_DisablePeripheralSecurityIT_IE0
 * @param  Periph This parameter can be a combination of the following values:
 *         @arg @ref LL_SCU_PERIPH_UART0
 *         @arg @ref LL_SCU_PERIPH_UART1
 *         @arg @ref LL_SCU_PERIPH_UART2
 *         @arg @ref LL_SCU_PERIPH_UART3
 *         @arg @ref LL_SCU_PERIPH_UART4
 *         @arg @ref LL_SCU_PERIPH_UART5
 *         @arg @ref LL_SCU_PERIPH_UART6
 *         @arg @ref LL_SCU_PERIPH_UART7
 *         @arg @ref LL_SCU_PERIPH_LPUART
 *         @arg @ref LL_SCU_PERIPH_USART0
 *         @arg @ref LL_SCU_PERIPH_USART1
 *         @arg @ref LL_SCU_PERIPH_ISO78160
 *         @arg @ref LL_SCU_PERIPH_ISO78161
 *         @arg @ref LL_SCU_PERIPH_SPI0
 *         @arg @ref LL_SCU_PERIPH_SPI1
 *         @arg @ref LL_SCU_PERIPH_SPI2
 *         @arg @ref LL_SCU_PERIPH_SPI3
 *         @arg @ref LL_SCU_PERIPH_I2C0
 *         @arg @ref LL_SCU_PERIPH_I2C1
 *         @arg @ref LL_SCU_PERIPH_CAN
 *         @arg @ref LL_SCU_PERIPH_IOCNT
 *         @arg @ref LL_SCU_PERIPH_LCD
 *         @arg @ref LL_SCU_PERIPH_DSP
 *         @arg @ref LL_SCU_PERIPH_M2M
 *         @arg @ref LL_SCU_PERIPH_D2F
 *         @arg @ref LL_SCU_PERIPH_ADC0
 *         @arg @ref LL_SCU_PERIPH_DAC
 *         @arg @ref LL_SCU_PERIPH_TC0
 *         @arg @ref LL_SCU_PERIPH_TC1
 * @retval None
 */
__STATIC_INLINE void LL_SCU_DisablePeripheralSecurityIT_IE0(uint32_t Periph)
{
    CLEAR_BIT(SCU_CORE0->PERIPH_SEC_IE0, Periph);
}

/**
 * @brief  Check if SCU Peripheral Illegal Access Interrupt0 is enabled
 * @rmtoll PERIPH_SEC_IE0      UART0IE           LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART1IE           LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART2IE           LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART3IE           LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART4IE           LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART5IE           LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART6IE           LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      UART7IE           LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      LPUARTIE          LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      USART0IE          LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      USART1IE          LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      ISO78160IE        LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      ISO78161IE        LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      SPI0IE            LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      SPI1IE            LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      SPI2IE            LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      SPI3IE            LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      I2C0IE            LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      I2C1IE            LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      CANIE             LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      IOCNTIE           LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      LCDIE             LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      DSPIE             LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      M2MIE             LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      D2FIE             LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      ADC0IE            LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      DACIE             LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      TC0IE             LL_SCU_IsEnabledPeripheralSecurityIT_IE0\n
 *         PERIPH_SEC_IE0      TC1IE             LL_SCU_IsEnabledPeripheralSecurityIT_IE0
 * @param  Periph This parameter can be a combination of the following values:
 *         @arg @ref LL_SCU_PERIPH_UART0
 *         @arg @ref LL_SCU_PERIPH_UART1
 *         @arg @ref LL_SCU_PERIPH_UART2
 *         @arg @ref LL_SCU_PERIPH_UART3
 *         @arg @ref LL_SCU_PERIPH_UART4
 *         @arg @ref LL_SCU_PERIPH_UART5
 *         @arg @ref LL_SCU_PERIPH_UART6
 *         @arg @ref LL_SCU_PERIPH_UART7
 *         @arg @ref LL_SCU_PERIPH_LPUART
 *         @arg @ref LL_SCU_PERIPH_USART0
 *         @arg @ref LL_SCU_PERIPH_USART1
 *         @arg @ref LL_SCU_PERIPH_ISO78160
 *         @arg @ref LL_SCU_PERIPH_ISO78161
 *         @arg @ref LL_SCU_PERIPH_SPI0
 *         @arg @ref LL_SCU_PERIPH_SPI1
 *         @arg @ref LL_SCU_PERIPH_SPI2
 *         @arg @ref LL_SCU_PERIPH_SPI3
 *         @arg @ref LL_SCU_PERIPH_I2C0
 *         @arg @ref LL_SCU_PERIPH_I2C1
 *         @arg @ref LL_SCU_PERIPH_CAN
 *         @arg @ref LL_SCU_PERIPH_IOCNT
 *         @arg @ref LL_SCU_PERIPH_LCD
 *         @arg @ref LL_SCU_PERIPH_DSP
 *         @arg @ref LL_SCU_PERIPH_M2M
 *         @arg @ref LL_SCU_PERIPH_D2F
 *         @arg @ref LL_SCU_PERIPH_ADC0
 *         @arg @ref LL_SCU_PERIPH_DAC
 *         @arg @ref LL_SCU_PERIPH_TC0
 *         @arg @ref LL_SCU_PERIPH_TC1
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SCU_IsEnabledPeripheralSecurityIT_IE0(uint32_t Periph)
{
    return ((READ_BIT(SCU_CORE0->PERIPH_SEC_IE0, Periph) == (Periph)) ? 1UL : 0UL);
}

/**
 * @brief  Enable SCU Peripheral Illegal Access Interrupt1
 * @rmtoll PERIPH_SEC_IE1        SIMPTC0IE            LL_SCU_EnablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        SIMPTC1IE            LL_SCU_EnablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        SIMPTC2IE            LL_SCU_EnablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        SIMPTC3IE            LL_SCU_EnablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        RS_VDDIE             LL_SCU_EnablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        LSRXIE               LL_SCU_EnablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        KBI0IE               LL_SCU_EnablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        KBI1IE               LL_SCU_EnablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        LVDCMPIE             LL_SCU_EnablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        CRC0IE               LL_SCU_EnablePeripheralSecurityIT_IE1
 * @param  Periph This parameter can be a combination of the following values:
 *         @arg @ref LL_SCU_PERIPH_SIMPTC0
 *         @arg @ref LL_SCU_PERIPH_SIMPTC1
 *         @arg @ref LL_SCU_PERIPH_SIMPTC2
 *         @arg @ref LL_SCU_PERIPH_SIMPTC3
 *         @arg @ref LL_SCU_PERIPH_RS_VDD
 *         @arg @ref LL_SCU_PERIPH_LSRX
 *         @arg @ref LL_SCU_PERIPH_KBI0
 *         @arg @ref LL_SCU_PERIPH_KBI1
 *         @arg @ref LL_SCU_PERIPH_LVDCMP
 *         @arg @ref LL_SCU_PERIPH_CRC0
 * @retval None
 */
__STATIC_INLINE void LL_SCU_EnablePeripheralSecurityIT_IE1(uint32_t Periph)
{
    SET_BIT(SCU_CORE0->PERIPH_SEC_IE1, Periph);
}

/**
 * @brief  Disable SCU Peripheral Illegal Access Interrupt1
 * @rmtoll PERIPH_SEC_IE1        SIMPTC0IE            LL_SCU_DisablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        SIMPTC1IE            LL_SCU_DisablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        SIMPTC2IE            LL_SCU_DisablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        SIMPTC3IE            LL_SCU_DisablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        RS_VDDIE             LL_SCU_DisablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        LSRXIE               LL_SCU_DisablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        KBI0IE               LL_SCU_DisablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        KBI1IE               LL_SCU_DisablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        LVDCMPIE             LL_SCU_DisablePeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        CRC0IE               LL_SCU_DisablePeripheralSecurityIT_IE1
 * @param  Periph This parameter can be a combination of the following values:
 *         @arg @ref LL_SCU_PERIPH_SIMPTC0
 *         @arg @ref LL_SCU_PERIPH_SIMPTC1
 *         @arg @ref LL_SCU_PERIPH_SIMPTC2
 *         @arg @ref LL_SCU_PERIPH_SIMPTC3
 *         @arg @ref LL_SCU_PERIPH_RS_VDD
 *         @arg @ref LL_SCU_PERIPH_LSRX
 *         @arg @ref LL_SCU_PERIPH_KBI0
 *         @arg @ref LL_SCU_PERIPH_KBI1
 *         @arg @ref LL_SCU_PERIPH_LVDCMP
 *         @arg @ref LL_SCU_PERIPH_CRC0
 * @retval None
 */
__STATIC_INLINE void LL_SCU_DisablePeripheralSecurityIT_IE1(uint32_t Periph)
{
    CLEAR_BIT(SCU_CORE0->PERIPH_SEC_IE1, Periph);
}

/**
 * @brief  Check if SCU Peripheral Illegal Access Interrupt1 is enabled
 * @rmtoll PERIPH_SEC_IE1        SIMPTC0IE            LL_SCU_IsEnabledPeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        SIMPTC1IE            LL_SCU_IsEnabledPeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        SIMPTC2IE            LL_SCU_IsEnabledPeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        SIMPTC3IE            LL_SCU_IsEnabledPeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        RS_VDDIE             LL_SCU_IsEnabledPeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        LSRXIE               LL_SCU_IsEnabledPeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        KBI0IE               LL_SCU_IsEnabledPeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        KBI1IE               LL_SCU_IsEnabledPeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        LVDCMPIE             LL_SCU_IsEnabledPeripheralSecurityIT_IE1\n
 *         PERIPH_SEC_IE1        CRC0IE               LL_SCU_IsEnabledPeripheralSecurityIT_IE1
 * @param  Periph This parameter can be a combination of the following values:
 *         @arg @ref LL_SCU_PERIPH_SIMPTC0
 *         @arg @ref LL_SCU_PERIPH_SIMPTC1
 *         @arg @ref LL_SCU_PERIPH_SIMPTC2
 *         @arg @ref LL_SCU_PERIPH_SIMPTC3
 *         @arg @ref LL_SCU_PERIPH_RS_VDD
 *         @arg @ref LL_SCU_PERIPH_LSRX
 *         @arg @ref LL_SCU_PERIPH_KBI0
 *         @arg @ref LL_SCU_PERIPH_KBI1
 *         @arg @ref LL_SCU_PERIPH_LVDCMP
 *         @arg @ref LL_SCU_PERIPH_CRC0
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SCU_IsEnabledPeripheralSecurityIT_IE1(uint32_t Periph)
{
    return ((READ_BIT(SCU_CORE0->PERIPH_SEC_IE1, Periph) == (Periph)) ? 1UL : 0UL);
}


/**
 * @brief  Enable SCU Peripheral Illegal Access Interrupt2
 * @rmtoll PERIPH_SEC_IE2        QSPI0IE           LL_SCU_EnablePeripheralSecurityIT_IE2\n
 *         PERIPH_SEC_IE2        QSPI1IE           LL_SCU_EnablePeripheralSecurityIT_IE2\n
 *         PERIPH_SEC_IE2        SEAIE             LL_SCU_EnablePeripheralSecurityIT_IE2\n
 *         PERIPH_SEC_IE2        AESIE             LL_SCU_EnablePeripheralSecurityIT_IE2
 * @param  Periph This parameter can be a combination of the following values:
 *         @arg @ref LL_SCU_PERIPH_QSPI0
 *         @arg @ref LL_SCU_PERIPH_QSPI1
 *         @arg @ref LL_SCU_PERIPH_SEA
 *         @arg @ref LL_SCU_PERIPH_AES
 * @retval None
 */
__STATIC_INLINE void LL_SCU_EnablePeripheralSecurityIT_IE2(uint32_t Periph)
{
    SET_BIT(SCU_CORE0->PERIPH_SEC_IE2, Periph);
}

/**
 * @brief  Disable SCU Peripheral Illegal Access Interrupt2
 * @rmtoll PERIPH_SEC_IE2        QSPI0IE           LL_SCU_DisablePeripheralSecurityIT_IE2\n
 *         PERIPH_SEC_IE2        QSPI1IE           LL_SCU_DisablePeripheralSecurityIT_IE2\n
 *         PERIPH_SEC_IE2        SEAIE             LL_SCU_DisablePeripheralSecurityIT_IE2\n
 *         PERIPH_SEC_IE2        AESIE             LL_SCU_DisablePeripheralSecurityIT_IE2
 * @param  Periph This parameter can be a combination of the following values:
 *         @arg @ref LL_SCU_PERIPH_QSPI0
 *         @arg @ref LL_SCU_PERIPH_QSPI1
 *         @arg @ref LL_SCU_PERIPH_SEA
 *         @arg @ref LL_SCU_PERIPH_AES
 * @retval None
 */
__STATIC_INLINE void LL_SCU_DisablePeripheralSecurityIT_IE2(uint32_t Periph)
{
    CLEAR_BIT(SCU_CORE0->PERIPH_SEC_IE2, Periph);
}

/**
 * @brief  Check if SCU Peripheral Illegal Access Interrupt2 is enabled
 * @rmtoll PERIPH_SEC_IE2        QSPI0IE           LL_SCU_IsEnabledPeripheralSecurityIT_IE2\n
 *         PERIPH_SEC_IE2        QSPI1IE           LL_SCU_IsEnabledPeripheralSecurityIT_IE2\n
 *         PERIPH_SEC_IE2        SEAIE             LL_SCU_IsEnabledPeripheralSecurityIT_IE2\n
 *         PERIPH_SEC_IE2        AESIE             LL_SCU_IsEnabledPeripheralSecurityIT_IE2
 * @param  Periph This parameter can be a combination of the following values:
 *         @arg @ref LL_SCU_PERIPH_QSPI0
 *         @arg @ref LL_SCU_PERIPH_QSPI1
 *         @arg @ref LL_SCU_PERIPH_SEA
 *         @arg @ref LL_SCU_PERIPH_AES
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SCU_IsEnabledPeripheralSecurityIT_IE2(uint32_t Periph)
{
    return ((READ_BIT(SCU_CORE0->PERIPH_SEC_IE2, Periph) == (Periph)) ? 1UL : 0UL);
}

/**
 * @}
 */

/** @defgroup SCU_LL_EF_FLAG_management FLAG_management
 * @{
 */

/**
 * @brief  Check if the Peripheral Illegal Access Interrupt Flag0 is set or not.
 * @rmtoll PERIPH_SEC_IF0      UART0IF           LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      UART1IF           LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      UART2IF           LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      UART3IF           LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      UART4IF           LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      UART5IF           LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      UART6IF           LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      UART7IF           LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      LPUARTIF          LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      USART0IF          LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      USART1IF          LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      ISO78160IF        LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      ISO78161IF        LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      SPI0IF            LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      SPI1IF            LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      SPI2IF            LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      SPI3IF            LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      I2C0IF            LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      I2C1IF            LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      CANIF             LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      IOCNTIF           LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      LCDIF             LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      DSPIF             LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      M2MIF             LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      D2FIF             LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      ADC0IF            LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      DACIF             LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      TC0IF             LL_SPI_IsPeripheralSecurityActiveFlag_IF0\n
 *         PERIPH_SEC_IF0      TC1IF             LL_SPI_IsPeripheralSecurityActiveFlag_IF0
 * @param  Periph This parameter can be one of the following values:
 *         @arg @ref LL_SCU_PERIPH_UART0
 *         @arg @ref LL_SCU_PERIPH_UART1
 *         @arg @ref LL_SCU_PERIPH_UART2
 *         @arg @ref LL_SCU_PERIPH_UART3
 *         @arg @ref LL_SCU_PERIPH_UART4
 *         @arg @ref LL_SCU_PERIPH_UART5
 *         @arg @ref LL_SCU_PERIPH_UART6
 *         @arg @ref LL_SCU_PERIPH_UART7
 *         @arg @ref LL_SCU_PERIPH_LPUART
 *         @arg @ref LL_SCU_PERIPH_USART0
 *         @arg @ref LL_SCU_PERIPH_USART1
 *         @arg @ref LL_SCU_PERIPH_ISO78160
 *         @arg @ref LL_SCU_PERIPH_ISO78161
 *         @arg @ref LL_SCU_PERIPH_SPI0
 *         @arg @ref LL_SCU_PERIPH_SPI1
 *         @arg @ref LL_SCU_PERIPH_SPI2
 *         @arg @ref LL_SCU_PERIPH_SPI3
 *         @arg @ref LL_SCU_PERIPH_I2C0
 *         @arg @ref LL_SCU_PERIPH_I2C1
 *         @arg @ref LL_SCU_PERIPH_CAN
 *         @arg @ref LL_SCU_PERIPH_IOCNT
 *         @arg @ref LL_SCU_PERIPH_LCD
 *         @arg @ref LL_SCU_PERIPH_DSP
 *         @arg @ref LL_SCU_PERIPH_M2M
 *         @arg @ref LL_SCU_PERIPH_D2F
 *         @arg @ref LL_SCU_PERIPH_ADC0
 *         @arg @ref LL_SCU_PERIPH_DAC
 *         @arg @ref LL_SCU_PERIPH_TC0
 *         @arg @ref LL_SCU_PERIPH_TC1
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsPeripheralSecurityActiveFlag_IF0(uint32_t Periph)
{
    return ((READ_BIT(SCU_CORE0->PERIPH_SEC_IF0, Periph) == (Periph)) ? 1UL : 0UL);
}

/**
 * @brief  Clear the Peripheral Illegal Access Interrupt Flag0.
 * @note   Can use in core0 and core1.
 * @rmtoll PERIPH_SEC_IF0      UART0IF           LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      UART1IF           LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      UART2IF           LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      UART3IF           LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      UART4IF           LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      UART5IF           LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      UART6IF           LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      UART7IF           LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      LPUARTIF          LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      USART0IF          LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      USART1IF          LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      ISO78160IF        LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      ISO78161IF        LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      SPI0IF            LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      SPI1IF            LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      SPI2IF            LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      SPI3IF            LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      I2C0IF            LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      I2C1IF            LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      CANIF             LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      IOCNTIF           LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      LCDIF             LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      DSPIF             LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      M2MIF             LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      D2FIF             LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      ADC0IF            LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      DACIF             LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      TC0IF             LL_SPI_ClearPeripheralSecurityFlag_IF0\n
 *         PERIPH_SEC_IF0      TC1IF             LL_SPI_ClearPeripheralSecurityFlag_IF0
 * @param  Periph This parameter can be one of the following values:
 *         @arg @ref LL_SCU_PERIPH_UART0
 *         @arg @ref LL_SCU_PERIPH_UART1
 *         @arg @ref LL_SCU_PERIPH_UART2
 *         @arg @ref LL_SCU_PERIPH_UART3
 *         @arg @ref LL_SCU_PERIPH_UART4
 *         @arg @ref LL_SCU_PERIPH_UART5
 *         @arg @ref LL_SCU_PERIPH_UART6
 *         @arg @ref LL_SCU_PERIPH_UART7
 *         @arg @ref LL_SCU_PERIPH_LPUART
 *         @arg @ref LL_SCU_PERIPH_USART0
 *         @arg @ref LL_SCU_PERIPH_USART1
 *         @arg @ref LL_SCU_PERIPH_ISO78160
 *         @arg @ref LL_SCU_PERIPH_ISO78161
 *         @arg @ref LL_SCU_PERIPH_SPI0
 *         @arg @ref LL_SCU_PERIPH_SPI1
 *         @arg @ref LL_SCU_PERIPH_SPI2
 *         @arg @ref LL_SCU_PERIPH_SPI3
 *         @arg @ref LL_SCU_PERIPH_I2C0
 *         @arg @ref LL_SCU_PERIPH_I2C1
 *         @arg @ref LL_SCU_PERIPH_CAN
 *         @arg @ref LL_SCU_PERIPH_IOCNT
 *         @arg @ref LL_SCU_PERIPH_LCD
 *         @arg @ref LL_SCU_PERIPH_DSP
 *         @arg @ref LL_SCU_PERIPH_M2M
 *         @arg @ref LL_SCU_PERIPH_D2F
 *         @arg @ref LL_SCU_PERIPH_ADC0
 *         @arg @ref LL_SCU_PERIPH_DAC
 *         @arg @ref LL_SCU_PERIPH_TC0
 *         @arg @ref LL_SCU_PERIPH_TC1
 * @retval None
 */
__STATIC_INLINE void LL_SPI_ClearPeripheralSecurityFlag_IF0(uint32_t Periph)
{
    WRITE_REG(SCU_CORE0->PERIPH_SEC_IF0, Periph);
}

/**
 * @brief  Set Peripheral Security Configuration1
 * @rmtoll PERIPH_SEC_CFG1        SIMPTC0IF            LL_SPI_IsPeripheralSecurityActiveFlag_IF1\n
 *         PERIPH_SEC_CFG1        SIMPTC1IF            LL_SPI_IsPeripheralSecurityActiveFlag_IF1\n
 *         PERIPH_SEC_CFG1        SIMPTC2IF            LL_SPI_IsPeripheralSecurityActiveFlag_IF1\n
 *         PERIPH_SEC_CFG1        SIMPTC3IF            LL_SPI_IsPeripheralSecurityActiveFlag_IF1\n
 *         PERIPH_SEC_CFG1        RS_VDDIF             LL_SPI_IsPeripheralSecurityActiveFlag_IF1\n
 *         PERIPH_SEC_CFG1        LSRXIF               LL_SPI_IsPeripheralSecurityActiveFlag_IF1\n
 *         PERIPH_SEC_CFG1        KBI0IF               LL_SPI_IsPeripheralSecurityActiveFlag_IF1\n
 *         PERIPH_SEC_CFG1        KBI1IF               LL_SPI_IsPeripheralSecurityActiveFlag_IF1\n
 *         PERIPH_SEC_CFG1        LVDCMPIF             LL_SPI_IsPeripheralSecurityActiveFlag_IF1\n
 *         PERIPH_SEC_CFG1        CRC0IF               LL_SPI_IsPeripheralSecurityActiveFlag_IF1
 * @param  Periph This parameter can be one of the following values:
 *         @arg @ref LL_SCU_PERIPH_SIMPTC0
 *         @arg @ref LL_SCU_PERIPH_SIMPTC1
 *         @arg @ref LL_SCU_PERIPH_SIMPTC2
 *         @arg @ref LL_SCU_PERIPH_SIMPTC3
 *         @arg @ref LL_SCU_PERIPH_RS_VDD
 *         @arg @ref LL_SCU_PERIPH_LSRX
 *         @arg @ref LL_SCU_PERIPH_KBI0
 *         @arg @ref LL_SCU_PERIPH_KBI1
 *         @arg @ref LL_SCU_PERIPH_LVDCMP
 *         @arg @ref LL_SCU_PERIPH_CRC0
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsPeripheralSecurityActiveFlag_IF1(uint32_t Periph)
{
    return ((READ_BIT(SCU_CORE0->PERIPH_SEC_IF1, Periph) == (Periph)) ? 1UL : 0UL);
}

/**
 * @brief  Get Peripheral Security Configuration1
 * @rmtoll PERIPH_SEC_IF1        SIMPTC0IF            LL_SPI_ClearPeripheralSecurityFlag_IF1\n
 *         PERIPH_SEC_IF1        SIMPTC1IF            LL_SPI_ClearPeripheralSecurityFlag_IF1\n
 *         PERIPH_SEC_IF1        SIMPTC2IF            LL_SPI_ClearPeripheralSecurityFlag_IF1\n
 *         PERIPH_SEC_IF1        SIMPTC3IF            LL_SPI_ClearPeripheralSecurityFlag_IF1\n
 *         PERIPH_SEC_IF1        RS_VDDIF             LL_SPI_ClearPeripheralSecurityFlag_IF1\n
 *         PERIPH_SEC_IF1        LSRXIF               LL_SPI_ClearPeripheralSecurityFlag_IF1\n
 *         PERIPH_SEC_IF1        KBI0IF               LL_SPI_ClearPeripheralSecurityFlag_IF1\n
 *         PERIPH_SEC_IF1        KBI1IF               LL_SPI_ClearPeripheralSecurityFlag_IF1\n
 *         PERIPH_SEC_IF1        LVDCMPIF             LL_SPI_ClearPeripheralSecurityFlag_IF1\n
 *         PERIPH_SEC_IF1        CRC0IF               LL_SPI_ClearPeripheralSecurityFlag_IF1
 * @param  Periph This parameter can be one of the following values:
 *         @arg @ref LL_SCU_PERIPH_SIMPTC0
 *         @arg @ref LL_SCU_PERIPH_SIMPTC1
 *         @arg @ref LL_SCU_PERIPH_SIMPTC2
 *         @arg @ref LL_SCU_PERIPH_SIMPTC3
 *         @arg @ref LL_SCU_PERIPH_RS_VDD
 *         @arg @ref LL_SCU_PERIPH_LSRX
 *         @arg @ref LL_SCU_PERIPH_KBI0
 *         @arg @ref LL_SCU_PERIPH_KBI1
 *         @arg @ref LL_SCU_PERIPH_LVDCMP
 *         @arg @ref LL_SCU_PERIPH_CRC0
 * @retval None
 */
__STATIC_INLINE void LL_SPI_ClearPeripheralSecurityFlag_IF1(uint32_t Periph)
{
    WRITE_REG(SCU_CORE0->PERIPH_SEC_IF1, Periph);
}

/**
 * @brief  Set Peripheral Security Configuration2
 * @rmtoll PERIPH_SEC_IF2      QSPI0IF           LL_SPI_IsPeripheralSecurityActiveFlag_IF2\n
 *         PERIPH_SEC_IF2      QSPI1IF           LL_SPI_IsPeripheralSecurityActiveFlag_IF2\n
 *         PERIPH_SEC_IF2      SEAIF             LL_SPI_IsPeripheralSecurityActiveFlag_IF2\n
 *         PERIPH_SEC_IF2      AESIF             LL_SPI_IsPeripheralSecurityActiveFlag_IF2
 * @param  Periph This parameter can be one of the following values:
 *         @arg @ref LL_SCU_PERIPH_QSPI0
 *         @arg @ref LL_SCU_PERIPH_QSPI1
 *         @arg @ref LL_SCU_PERIPH_SEA
 *         @arg @ref LL_SCU_PERIPH_AES
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsPeripheralSecurityActiveFlag_IF2(uint32_t Periph)
{
    return ((READ_BIT(SCU_CORE0->PERIPH_SEC_IF2, Periph) == (Periph)) ? 1UL : 0UL);
}

/**
 * @brief  Get Peripheral Security Configuration2.
 * @note   Can use in core0 ande core1.
 * @rmtoll PERIPH_SEC_IF2      QSPI0IF           LL_SPI_ClearPeripheralSecurityFlag_IF2\n
 *         PERIPH_SEC_IF2      QSPI1IF           LL_SPI_ClearPeripheralSecurityFlag_IF2\n
 *         PERIPH_SEC_IF2      SEAIF             LL_SPI_ClearPeripheralSecurityFlag_IF2\n
 *         PERIPH_SEC_IF2      AESIF             LL_SPI_ClearPeripheralSecurityFlag_IF2
 * @param  SCUx SCU Instance
 * @param  Periph This parameter can be one of the following values:
 *         @arg @ref LL_SCU_PERIPH_QSPI0
 *         @arg @ref LL_SCU_PERIPH_QSPI1
 *         @arg @ref LL_SCU_PERIPH_SEA
 *         @arg @ref LL_SCU_PERIPH_AES
 * @retval None
 */
__STATIC_INLINE void LL_SPI_ClearPeripheralSecurityFlag_IF2(uint32_t Periph)
{
    WRITE_REG(SCU_CORE0->PERIPH_SEC_IF2, Periph);
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup SCU_LL_EF_Init Initialization and de-initialization functions
 * @{
 */

ErrorStatus LL_SCU_DeInit(SCU_TypeDef *SCUx);

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

#endif /* SCU */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_SCU_H */
