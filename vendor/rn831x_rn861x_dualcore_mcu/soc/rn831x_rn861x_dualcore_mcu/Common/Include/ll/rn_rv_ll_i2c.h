/**
 *****************************************************************************
 * @file   rn_rv_ll_i2c.h
 * @author Renergy Software Team
 * @brief  Header file of I2C LL module.
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
#ifndef __RN_LL_I2C_H
#define __RN_LL_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_i2c.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (I2C0) || defined (I2C1)

/** @defgroup I2C_LL I2C
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup I2C_LL_Private_Constants I2C Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup I2C_LL_Private_Macros I2C Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup I2C_LL_ES_INIT I2C Exported Init structure
 * @{
 */

/**
 * @brief LL I2C Init Structure definition
 */
typedef struct
{
    uint32_t PeripheralMode;      /*!< Specifies the peripheral mode.
                                     This parameter can be a value of @ref I2C_LL_EC_PERIPHERAL_MODE.

                                     This feature can be modified afterwards using unitary function @ref LL_I2C_SetMode(). */

    uint32_t SpeedMode;           /*!< Specifies the speed mode range.
                                     This parameter can be a value of @ref I2C_LL_EC_SPEED_MODE.

                                     This feature can be modified afterwards using unitary function @ref LL_I2C_SetClockSpeedMode() */

    uint32_t Address;             /*!< Specifies the device address.
                                     This parameter must be a value between Min_Data = 0x00 and Max_Data = 0x7F.

                                     This feature can be modified afterwards using unitary function @ref LL_I2C_SetAddress(). */

    uint32_t TypeAcknowledge;     /*!< Specifies the ACKnowledge or Non ACKnowledge condition after the address receive match code or next received byte.
                                     This parameter can be a value of @ref I2C_LL_EC_I2C_ACKNOWLEDGE.

                                     This feature can be modified afterwards using unitary function @ref LL_I2C_AcknowledgeNextData(). */
} LL_I2C_InitTypeDef;

/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/* Exported constants -------------------------------------------------------*/
/** @defgroup I2C_LL_Exported_Constants I2C Exported Constants
 * @{
 */

/** @defgroup I2C_LL_EC_GET_FLAG Get Flags Defines
 * @brief    Flags defines which can be used with LL_I2C_ReadReg function
 * @{
 */
#define LL_I2C_STA_STPD                     I2C_STA_STPD              /*!< Stop signal detection interrupt flag               */
#define LL_I2C_STA_RXNACK                   I2C_STA_RX_NACK           /*!< Received NACK interrupt flag                       */
#define LL_I2C_STA_TRANC                    I2C_STA_TRANC             /*!< Transmission completion interrupt flag             */
#define LL_I2C_STA_TXEMPT                   I2C_STA_TXEMPT            /*!< Send data register empty error flag (transmitters) */
#define LL_I2C_STA_OVF                      I2C_STA_OVF               /*!< Receive overflow interrupt identifier bit flag     */
#define LL_I2C_STA_COL                      I2C_STA_COL               /*!< Send conflict interrupt identifier bit flag        */
#define LL_I2C_STA_BUSY                     I2C_STA_BUSY              /*!< Communication status flag                          */
#define LL_I2C_STA_MATCH                    I2C_STA_MATCH             /*!< Address matched flag                               */
#define LL_I2C_STA_DIR                      I2C_STA_DIR               /*!< Read/Write direction                               */
/**
 * @}
 */

/** @defgroup I2C_LL_EC_CLOCK_SPEED_MODE Clock Speed Mode
 * @{
 */
#define LL_I2C_CLOCK_SPEED_1M8_STANDARD_MODE       I2C_CLK_CLKDIV_1M8_STANDARD_MODE          /*!< Standard mode in system clock 1.8432MHz,
                                                                                                fast mode is not supported in 1.8432MHz    */
#define LL_I2C_CLOCK_SPEED_3M6_STANDARD_MODE       I2C_CLK_CLKDIV_3M6_STANDARD_MODE          /*!< Standard mode in system clock 3.6864MHz,
                                                                                                fast mode is not supported in 3.6864MHz    */
#define LL_I2C_CLOCK_SPEED_7M3_STANDARD_MODE       I2C_CLK_CLKDIV_7M3_STANDARD_MODE          /*!< Standard mode in system clock 7.3728MHz  */
#define LL_I2C_CLOCK_SPEED_14M7_STANDARD_MODE      I2C_CLK_CLKDIV_14M7_STANDARD_MODE         /*!< Standard mode in system clock 14.7456MHz */
#define LL_I2C_CLOCK_SPEED_29M4_STANDARD_MODE      I2C_CLK_CLKDIV_29M4_STANDARD_MODE         /*!< Standard mode in system clock 29.4912MHz */
#define LL_I2C_CLOCK_SPEED_58M9_STANDARD_MODE      I2C_CLK_CLKDIV_58M9_STANDARD_MODE         /*!< Standard mode in system clock 58.9824MHz */
#define LL_I2C_CLOCK_SPEED_7M3_FAST_MODE           I2C_CLK_CLKDIV_7M3_FAST_MODE              /*!< Fast mode in system clock 7.3728MHz      */
#define LL_I2C_CLOCK_SPEED_14M7_FAST_MODE          I2C_CLK_CLKDIV_14M7_FAST_MODE             /*!< Fast mode in system clock 14.7456MHz     */
#define LL_I2C_CLOCK_SPEED_29M4_FAST_MODE          I2C_CLK_CLKDIV_29M4_FAST_MODE             /*!< Fast mode in system clock 29.4912MHz     */
#define LL_I2C_CLOCK_SPEED_58M9_FAST_MODE          I2C_CLK_CLKDIV_58M9_FAST_MODE             /*!< Fast mode in system clock 58.9824MHz     */
/**
 * @}
 */

/** @defgroup I2C_LL_EC_SPEED_MODE Clock Speed mode Range
 * @{
 */
#define LL_I2C_SPEED_STANDARD_MODE               0x00000000U                               /*!< Master clock speed range is standard mode, used for @LL_I2C_Init() */
#define LL_I2C_SPEED_FAST_MODE                   0x00000001U                               /*!< Master clock speed range is fast mode, used for @LL_I2C_Init()     */
/**
 * @}
 */

/** @defgroup I2C_LL_EC_PERIPHERAL_MODE Peripheral Mode
 * @{
 */
#define LL_I2C_MODE_MASTER                  I2C_CTRL_MODE             /*!< I2C Master mode */
#define LL_I2C_MODE_SLAVE                   0x00000000U               /*!< I2C Slave mode  */
/**
 * @}
 */

/** @defgroup I2C_LL_EC_I2C_ACKNOWLEDGE Acknowledge Generation
 * @{
 */
#define LL_I2C_ACK                          I2C_CTRL_ACK             /*!< ACK is sent after current received byte  */
#define LL_I2C_NACK                         0x00000000U              /*!< NACK is sent after current received byte */
/**
 * @}
 */

/** @defgroup I2C_LL_EC_DIRECTION Read Write Direction
 * @{
 */
#define LL_I2C_DIRECTION_WRITE              0x00000000U              /*!< Bus is in write transfer */
#define LL_I2C_DIRECTION_READ               I2C_ADDR_RW              /*!< Bus is in read transfer  */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup I2C_LL_Exported_Macros I2C Exported Macros
 * @{
 */
/** @defgroup I2C_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in I2C register
 * @param  __INSTANCE__ I2C Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_I2C_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in I2C register
 * @param  __INSTANCE__ I2C Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_I2C_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup I2C_LL_Exported_Functions I2C Exported Functions
 * @{
 */

/** @defgroup I2C_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief  Enable I2C peripheral (EN = 1).
 * @param  I2Cx I2C Instance.
 * @retval None
 */
__STATIC_INLINE void LL_I2C_Enable(I2C_TypeDef *I2Cx)
{
    SET_BIT(I2Cx->CTRL, I2C_CTRL_EN);
}

/**
 * @brief  Disable I2C peripheral (EN = 0).
 * @param  I2Cx I2C Instance.
 * @retval None
 */
__STATIC_INLINE void LL_I2C_Disable(I2C_TypeDef *I2Cx)
{
    CLEAR_BIT(I2Cx->CTRL, I2C_CTRL_EN);
}

/**
 * @brief  Check if the I2C peripheral is enabled or disabled.
 * @param  I2Cx I2C Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_I2C_IsEnabled(I2C_TypeDef *I2Cx)
{
    return (READ_BIT(I2Cx->CTRL, I2C_CTRL_EN) == (I2C_CTRL_EN));
}

/**
 * @brief  Configure the I2C clock speed mode.
 * @note   Set the clock speed mode should according to the system clock and communication rate requirements.
 * @param  I2Cx I2C Instance.
 * @param  ClockSpeedMode This parameter can be one of the following values:
 *         @arg @ref LL_I2C_CLOCK_SPEED_1M8_STANDARD_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_3M6_STANDARD_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_7M3_STANDARD_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_14M7_STANDARD_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_29M4_STANDARD_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_58M9_STANDARD_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_7M3_FAST_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_14M7_FAST_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_29M4_FAST_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_58M9_FAST_MODE
 * @retval None
 */
__STATIC_INLINE void LL_I2C_SetClockSpeedMode(I2C_TypeDef *I2Cx, uint32_t ClockSpeedMode)
{
    MODIFY_REG(I2Cx->CLK, I2C_CLK_CLKDIV, ClockSpeedMode);
}

/**
 * @brief  Get the the I2C master speed mode.
 * @param  I2Cx I2C Instance.
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_I2C_CLOCK_SPEED_1M8_STANDARD_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_3M6_STANDARD_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_7M3_STANDARD_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_14M7_STANDARD_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_29M4_STANDARD_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_58M9_STANDARD_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_7M3_FAST_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_14M7_FAST_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_29M4_FAST_MODE
 *         @arg @ref LL_I2C_CLOCK_SPEED_58M9_FAST_MODE
 */
__STATIC_INLINE uint32_t LL_I2C_GetClockSpeedMode(I2C_TypeDef *I2Cx)
{
    return (uint32_t)(READ_BIT(I2Cx->CLK, I2C_CLK_CLKDIV));
}

/**
 * @brief  Configure I2C peripheral mode.
 * @param  I2Cx I2C Instance.
 * @param  PeripheralMode This parameter can be one of the following values:
 *         @arg @ref LL_I2C_MODE_MASTER
 *         @arg @ref LL_I2C_MODE_SLAVE
 * @retval None
 */
__STATIC_INLINE void LL_I2C_SetMode(I2C_TypeDef *I2Cx, uint32_t PeripheralMode)
{
    MODIFY_REG(I2Cx->CTRL, I2C_CTRL_MODE, PeripheralMode);
}

/**
 * @brief  Get peripheral mode.
 * @param  I2Cx I2C Instance.
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_I2C_MODE_MASTER
 *         @arg @ref LL_I2C_MODE_SLAVE
 */
__STATIC_INLINE uint32_t LL_I2C_GetMode(I2C_TypeDef *I2Cx)
{
    return (uint32_t)(READ_BIT(I2Cx->CTRL, I2C_CTRL_MODE));
}

/**
 * @brief  Set the Device Address.
 * @note   Cannot be written to this register during the address transmission period.
 *         Master mode, it indicates the address of the slave device.
 *         Slave mode, this address is used to compare with the address sent by master device.
 * @param  I2Cx I2C Instance.
 * @param  Address This parameter must be a value between Min_Data=0 and Max_Data=0x7F.
 * @retval None
 */
__STATIC_INLINE void LL_I2C_SetAddress(I2C_TypeDef *I2Cx, uint32_t Address)
{
    MODIFY_REG(I2Cx->ADDR, I2C_ADDR_SADR, Address << I2C_ADDR_SADR_POS);
}

/**
 * @}
 */

/** @defgroup I2C_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable I2C interrupt.
 * @param  I2Cx I2C Instance.
 * @retval None
 */
__STATIC_INLINE void LL_I2C_EnableIT(I2C_TypeDef *I2Cx)
{
    SET_BIT(I2Cx->CTRL, I2C_CTRL_IRQE);
}

/**
 * @brief  Disable I2C interrupt.
 * @param  I2Cx I2C Instance.
 * @retval None
 */
__STATIC_INLINE void LL_I2C_DisableIT(I2C_TypeDef *I2Cx)
{
    CLEAR_BIT(I2Cx->CTRL, I2C_CTRL_IRQE);
}

/**
 * @brief  Check if the I2C Interrupt is enabled or disabled.
 * @param  I2Cx I2C Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_I2C_IsEnabledIT(I2C_TypeDef *I2Cx)
{
    return (READ_BIT(I2Cx->CTRL, I2C_CTRL_IRQE) == (I2C_CTRL_IRQE));
}

/**
 * @}
 */

/** @defgroup I2C_LL_EF_FLAG_management FLAG_management
 * @{
 */

/**
 * @brief  Indicate the status of Transfer Direction flag (read only).
 * @param  I2Cx I2C Instance.
 * @retval State of bit (1 for READ or 0 for WRITE).
 */
__STATIC_INLINE uint32_t LL_I2C_IsActiveFlag_DIR(I2C_TypeDef *I2Cx)
{
    return (READ_BIT(I2Cx->STA, I2C_STA_DIR) == (I2C_STA_DIR));
}

/**
 * @brief  Indicate the status of Address Matched flag (read only).
 * @note   RESET: When start or stop condition is detected will be cleared.
 *         SET: When address is matched.
 * @param  I2Cx I2C Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_I2C_IsActiveFlag_MATCH(I2C_TypeDef *I2Cx)
{
    return (READ_BIT(I2Cx->STA, I2C_STA_MATCH) == (I2C_STA_MATCH));
}

/**
 * @brief  Indicate the status of Communication Busy flag (read only).
 * @note   RESET: Only read.
 *         SET: When a Start condition is detected.
 * @param  I2Cx I2C Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_I2C_IsActiveFlag_BUSY(I2C_TypeDef *I2Cx)
{
    return (READ_BIT(I2Cx->STA, I2C_STA_BUSY) == (I2C_STA_BUSY));
}

/**
 * @brief  Indicate the status of Send/Receive Conflict Interrupt flag (slave mode).
 * @note   RESET: Write one to clear.
 *         SET: When a send/receive conflict occurs (slave mode).
 * @param  I2Cx I2C Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_I2C_IsActiveFlag_COL(I2C_TypeDef *I2Cx)
{
    return (READ_BIT(I2Cx->STA, I2C_STA_COL) == (I2C_STA_COL));
}

/**
 * @brief  Clear Send Conflict Interrupt flag.
 * @note   Write one to clear.
 * @param  I2Cx I2C Instance.
 * @retval None
 */
__STATIC_INLINE void LL_I2C_ClearFlag_COL(I2C_TypeDef *I2Cx)
{
    WRITE_REG(I2Cx->STA, I2C_STA_COL);
}

/**
 * @brief  Indicate the status of Overflow Interrupt flag.
 * @note   RESET: Write one to clear.
 *         SET: When a new byte received before the previous is taken away, an overflow error occurs.
 * @param  I2Cx I2C Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_I2C_IsActiveFlag_OVF(I2C_TypeDef *I2Cx)
{
    return (READ_BIT(I2Cx->STA, I2C_STA_OVF) == (I2C_STA_OVF));
}

/**
 * @brief  Clear Overflow flag.
 * @note   Write one to clear.
 * @param  I2Cx I2C Instance.
 * @retval None
 */
__STATIC_INLINE void LL_I2C_ClearFlag_OVF(I2C_TypeDef *I2Cx)
{
    WRITE_REG(I2Cx->STA, I2C_STA_OVF);
}

/**
 * @brief  Indicate the status of Transmit Data Register Empty Error Interrupt flag (slave mode).
 * @note   RESET: Write one to clear.
 *         SET: When transmit data register is empty.
 * @param  I2Cx I2C Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_I2C_IsActiveFlag_TXE(I2C_TypeDef *I2Cx)
{
    return (READ_BIT(I2Cx->STA, I2C_STA_TXEMPT) == (I2C_STA_TXEMPT));
}

/**
 * @brief  Clear Transmit Data Register Empty Error Interrupt flag.
 * @note   Write one to clear.
 * @param  I2Cx I2C Instance.
 * @retval None
 */
__STATIC_INLINE void LL_I2C_ClearFlag_TXE(I2C_TypeDef *I2Cx)
{
    WRITE_REG(I2Cx->STA, I2C_STA_TXEMPT);
}

/**
 * @brief  Indicate the status of Byte Transfer Finished Interrupt flag.
 *         RESET: Write one to clear.
 *         SET: When data byte transfer finished.
 * @param  I2Cx I2C Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_I2C_IsActiveFlag_TRANC(I2C_TypeDef *I2Cx)
{
    return (READ_BIT(I2Cx->STA, I2C_STA_TRANC) == (I2C_STA_TRANC));
}

/**
 * @brief  Clear Transfer Finished interrupt flag.
 * @note   write one to Clear.
 * @param  I2Cx I2C Instance.
 * @retval None
 */
__STATIC_INLINE void LL_I2C_ClearFlag_TRANC(I2C_TypeDef *I2Cx)
{
    WRITE_REG(I2Cx->STA, I2C_STA_TRANC);
}

/**
 * @brief  Indicate the status of Receiving a Non Acknowledge Interrupt flag.
 * @note   RESET: Write one to Clear.
 *         SET: When a Non Acknowledge is received after a byte transmission.
 * @param  I2Cx I2C Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_I2C_IsActiveFlag_RXNACK(I2C_TypeDef *I2Cx)
{
    return (READ_BIT(I2Cx->STA, I2C_STA_RX_NACK) == (I2C_STA_RX_NACK));
}

/**
 * @brief  Clear receiving Non Acknowledge interrupt flag.
 * @note   write one to Clear.
 * @param  I2Cx I2C Instance.
 * @retval None
 */
__STATIC_INLINE void LL_I2C_ClearFlag_RXNACK(I2C_TypeDef *I2Cx)
{
    WRITE_REG(I2Cx->STA, I2C_STA_RX_NACK);
}

/**
 * @brief  Indicate the status of Stop Detection Interrupt flag.
 * @note   RESET: Disable I2C module or received Start condition will be cleared.
 *         SET: When a stop condition is detected.
 * @param  I2Cx I2C Instance.
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_I2C_IsActiveFlag_STOP(I2C_TypeDef *I2Cx)
{
    return (READ_BIT(I2Cx->STA, I2C_STA_STPD) == (I2C_STA_STPD));
}

/**
 * @brief  Clear Stop Detection flag.
 * @note   write one to Clear, disable I2C module or received Start condition will be cleared.
 * @param  I2Cx I2C Instance.
 * @retval None
 */
__STATIC_INLINE void LL_I2C_ClearFlag_STOP(I2C_TypeDef *I2Cx)
{
    WRITE_REG(I2Cx->STA, I2C_STA_STPD);
}

/**
 * @}
 */

/** @defgroup I2C_LL_EF_Data_Management Data_Management
 * @{
 */

/**
 * @brief  Generate a START or RESTART condition
 * @note   The START bits can be set even if bus is not BUSY or I2C is in master mode transmit.
 * @param  I2Cx I2C Instance.
 * @retval None
 */
__STATIC_INLINE void LL_I2C_GenerateStartCondition(I2C_TypeDef *I2Cx)
{
    MODIFY_REG(I2Cx->CTRL, I2C_CTRL_BUSCON, I2C_CTRL_BUSCON_START);
}

/**
 * @brief  Generate a STOP condition after the current byte transfer.
 * @param  I2Cx I2C Instance.
 * @retval None
 */
__STATIC_INLINE void LL_I2C_GenerateStopCondition(I2C_TypeDef *I2Cx)
{
    MODIFY_REG(I2Cx->CTRL, I2C_CTRL_BUSCON, I2C_CTRL_BUSCON_STOP);
}

/**
 * @brief  Prepare the generation of a ACKnowledge or Non ACKnowledge condition after the address receive match code or next received byte.
 * @note   Usage in Slave or Master mode.
 * @param  I2Cx I2C Instance.
 * @param  TypeAcknowledge This parameter can be one of the following values:
 *         @arg @ref LL_I2C_ACK
 *         @arg @ref LL_I2C_NACK
 * @retval None
 */
__STATIC_INLINE void LL_I2C_AcknowledgeNextData(I2C_TypeDef *I2Cx, uint32_t TypeAcknowledge)
{
    MODIFY_REG(I2Cx->CTRL, I2C_CTRL_ACK, TypeAcknowledge);
}

/**
 * @brief  Set the Transfer Direction (master mode).
 * @param  I2Cx I2C Instance.
 * @param  Direction This parameter can be one of the following values:
 *         @arg @ref LL_I2C_DIRECTION_WRITE
 *         @arg @ref LL_I2C_DIRECTION_READ
 * @retval None
 */
__STATIC_INLINE void LL_I2C_SetTransferDirection(I2C_TypeDef *I2Cx, uint32_t Direction)
{
    MODIFY_REG(I2Cx->ADDR, I2C_ADDR_RW, Direction);
}

/**
 * @brief  Indicate the value of transfer direction.
 * @note   RESET: Bus is in write transfer (peripheral point of view).
 *         SET: Bus is in read transfer (peripheral point of view).
 * @param  I2Cx I2C Instance.
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_I2C_DIRECTION_WRITE
 *         @arg @ref LL_I2C_DIRECTION_READ
 */
__STATIC_INLINE uint32_t LL_I2C_GetTransferDirection(I2C_TypeDef *I2Cx)
{
    return (uint32_t)(READ_BIT(I2Cx->ADDR, I2C_ADDR_RW));
}

/**
 * @brief  Read Receive Data register.
 * @param  I2Cx I2C Instance.
 * @retval Value between Min_Data=0x0 and Max_Data=0xFF
 */
__STATIC_INLINE uint8_t LL_I2C_ReceiveData8(I2C_TypeDef *I2Cx)
{
    return (uint8_t)(READ_BIT(I2Cx->DATA, I2C_DATA_TRDATA));
}

/**
 * @brief  Write in Transmit Data Register.
 * @param  I2Cx I2C Instance.
 * @param  Data Value between Min_Data=0x0 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_I2C_TransmitData8(I2C_TypeDef *I2Cx, uint8_t Data)
{
    MODIFY_REG(I2Cx->DATA, I2C_DATA_TRDATA, Data);
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup I2C_LL_EF_Init Initialization and de-initialization functions
 * @{
 */

// The initialization function can have more than one argument.
ErrorStatus LL_I2C_Init(I2C_TypeDef *I2Cx, LL_I2C_InitTypeDef *I2C_InitStruct);
ErrorStatus LL_I2C_DeInit(I2C_TypeDef *I2Cx);
void LL_I2C_StructInit(LL_I2C_InitTypeDef *I2C_InitStruct); // If the module is not suitable for the default configuration, you could not defines this function.

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

#endif /* I2C0 || I2C1 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_I2C_H */
