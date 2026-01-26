/**
 *****************************************************************************
 * @file   rn_rv_ll_gic.h
 * @author Renergy Software Team
 * @brief  Header file of GIC LL module.
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
#ifndef __RN_LL_GIC_H
#define __RN_LL_GIC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_gic.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (GIC)

/** @defgroup GIC_LL GIC
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup GIC_LL_Private_Constants GIC Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup GIC_LL_Private_Macros GIC Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
/** @defgroup GIC_LL_ES_INIT GIC Exported Init structure
 * @{
 */

/**
 * @}
 */

/* Exported constants -------------------------------------------------------*/
/** @defgroup GIC_LL_Exported_Constants GIC Exported Constants
 * @{
 */

/** @defgroup GIC_LL_EC_ICI_CORE_ID ICI_CORE_ID
 * @{
 */
#define LL_ICI_COREID_0                    0x00000000U                        /*!< Select CORE 0   */
#define LL_ICI_COREID_1                    0x00000001U                        /*!< Select CORE 1   */
/**
 * @}
 */

/** @defgroup GIC_LL_EC_IRQNUM IRQ Number
 * @{
 */
#define LL_GIC_IRQNUM_LCD_IRQn                   46
#define LL_GIC_IRQNUM_KBI1_IRQ0                  47
#define LL_GIC_IRQNUM_KBI1_IRQ1                  48
#define LL_GIC_IRQNUM_KBI1_IRQ2                  49
#define LL_GIC_IRQNUM_KBI1_IRQ3                  50
#define LL_GIC_IRQNUM_KBI1_IRQ4                  51
#define LL_GIC_IRQNUM_KBI1_IRQ5                  52
#define LL_GIC_IRQNUM_KBI1_IRQ6                  53
#define LL_GIC_IRQNUM_KBI1_IRQ7                  54
#define LL_GIC_IRQNUM_KBI1_IRQ8                  55
#define LL_GIC_IRQNUM_KBI1_IRQ9                  56
#define LL_GIC_IRQNUM_KBI1_IRQ10                 57
#define LL_GIC_IRQNUM_KBI1_IRQ11                 58
#define LL_GIC_IRQNUM_KBI1_IRQ12                 59
#define LL_GIC_IRQNUM_KBI1_IRQ13                 60
#define LL_GIC_IRQNUM_KBI1_IRQ14                 61
#define LL_GIC_IRQNUM_KBI1_IRQ15                 62
#define LL_GIC_IRQNUM_KBI1_IRQn                  63
#define LL_GIC_IRQNUM_KBI0_IRQ0                  64
#define LL_GIC_IRQNUM_KBI0_IRQ1                  65
#define LL_GIC_IRQNUM_KBI0_IRQ2                  66
#define LL_GIC_IRQNUM_KBI0_IRQ3                  67
#define LL_GIC_IRQNUM_KBI0_IRQ4                  68
#define LL_GIC_IRQNUM_KBI0_IRQ5                  69
#define LL_GIC_IRQNUM_KBI0_IRQ6                  70
#define LL_GIC_IRQNUM_KBI0_IRQ7                  71
#define LL_GIC_IRQNUM_KBI0_IRQ8                  72
#define LL_GIC_IRQNUM_KBI0_IRQ9                  73
#define LL_GIC_IRQNUM_KBI0_IRQ10                 74
#define LL_GIC_IRQNUM_KBI0_IRQ11                 75
#define LL_GIC_IRQNUM_KBI0_IRQ12                 76
#define LL_GIC_IRQNUM_KBI0_IRQ13                 77
#define LL_GIC_IRQNUM_KBI0_IRQ14                 78
#define LL_GIC_IRQNUM_KBI0_IRQ15                 79
#define LL_GIC_IRQNUM_KBI0_IRQn                  80
#define LL_GIC_IRQNUM_WDT0_IRQn                  81
#define LL_GIC_IRQNUM_WDT1_IRQn                  82
#define LL_GIC_IRQNUM_GDMA11_IRQn                83
#define LL_GIC_IRQNUM_GDMA10_IRQn                84
#define LL_GIC_IRQNUM_GDMA01_IRQn                85
#define LL_GIC_IRQNUM_GDMA00_IRQn                86
#define LL_GIC_IRQNUM_SYSTC_IRQn                 87
#define LL_GIC_IRQNUM_SIMPTC0_0_IRQ              88
#define LL_GIC_IRQNUM_SIMPTC0_1_IRQ              89
#define LL_GIC_IRQNUM_SIMPTC0_2_IRQ              90
#define LL_GIC_IRQNUM_SIMPTC0_3_IRQ              91
#define LL_GIC_IRQNUM_SIMPTC1_0_IRQ              92
#define LL_GIC_IRQNUM_SIMPTC1_1_IRQ              93
#define LL_GIC_IRQNUM_SIMPTC1_2_IRQ              94
#define LL_GIC_IRQNUM_SIMPTC1_3_IRQ              95
#define LL_GIC_IRQNUM_SIMPTC2_0_IRQ              96
#define LL_GIC_IRQNUM_SIMPTC2_1_IRQ              97
#define LL_GIC_IRQNUM_SIMPTC2_2_IRQ              98
#define LL_GIC_IRQNUM_SIMPTC2_3_IRQ              99
#define LL_GIC_IRQNUM_SIMPTC3_0_IRQ             100
#define LL_GIC_IRQNUM_SIMPTC3_1_IRQ             101
#define LL_GIC_IRQNUM_SIMPTC3_2_IRQ             102
#define LL_GIC_IRQNUM_SIMPTC3_3_IRQ             103
#define LL_GIC_IRQNUM_TC0_IRQn                  104
#define LL_GIC_IRQNUM_TC1_IRQn                  105
#define LL_GIC_IRQNUM_ISO78160_IRQn             106
#define LL_GIC_IRQNUM_ISO78161_IRQn             107
#define LL_GIC_IRQNUM_I2C0_IRQn                 108
#define LL_GIC_IRQNUM_I2C1_IRQn                 109
#define LL_GIC_IRQNUM_LPUART_IRQn               110
#define LL_GIC_IRQNUM_LSRX_IRQn                 111
#define LL_GIC_IRQNUM_UART0_IRQn                112
#define LL_GIC_IRQNUM_UART1_IRQn                113
#define LL_GIC_IRQNUM_UART2_IRQn                114
#define LL_GIC_IRQNUM_UART3_IRQn                115
#define LL_GIC_IRQNUM_UART4_IRQn                116
#define LL_GIC_IRQNUM_UART5_IRQn                117
#define LL_GIC_IRQNUM_UART6_IRQn                118
#define LL_GIC_IRQNUM_UART7_IRQn                119
#define LL_GIC_IRQNUM_USART0_IRQn               120
#define LL_GIC_IRQNUM_USART1_IRQn               121
#define LL_GIC_IRQNUM_CAN_IRQn                  122
#define LL_GIC_IRQNUM_SPI0_IRQn                 123
#define LL_GIC_IRQNUM_SPI1_IRQn                 124
#define LL_GIC_IRQNUM_SPI2_IRQn                 125
#define LL_GIC_IRQNUM_SPI3_IRQn                 126
#define LL_GIC_IRQNUM_QSPI0_IRQn                127
#define LL_GIC_IRQNUM_QSPI1_IRQn                128
#define LL_GIC_IRQNUM_DSP_IRQn                  129
#define LL_GIC_IRQNUM_M2M_IRQn                  130
#define LL_GIC_IRQNUM_AES_IRQn                  131
#define LL_GIC_IRQNUM_SEA_IRQn                  132
#define LL_GIC_IRQNUM_KS_IRQn                   133
#define LL_GIC_IRQNUM_DAC_IRQn                  134
#define LL_GIC_IRQNUM_ADC0_IRQn                 135
#define LL_GIC_IRQNUM_D2F_IRQn                  136
#define LL_GIC_IRQNUM_RTC_IRQn                  137
#define LL_GIC_IRQNUM_VBAT_IRQn                 138
#define LL_GIC_IRQNUM_CMPLVD_IRQn               139
#define LL_GIC_IRQNUM_LCSS_IRQn                 140
#define LL_GIC_IRQNUM_SCU_IRQn                  141
#define LL_GIC_IRQNUM_CORE1_RST_IRQn            142
#define LL_GIC_IRQNUM_CORE1_SYSREG_IRQn         143
#define LL_GIC_IRQNUM_CORE0_SYSREG_IRQn         144
#define LL_GIC_IRQNUM_MAILBOSINTER_IRQn         145
#define LL_GIC_IRQNUM_MAILBOSHOST0_IRQn         146
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup GIC_LL_Exported_Macros GIC Exported Macros
 * @{
 */
/** @defgroup GIC_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in GIC register
 * @param  __INSTANCE__ GIC Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_GIC_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in GIC register
 * @param  __INSTANCE__ GIC Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_GIC_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup GIC_LL_Exported_Functions GIC Exported Functions
 * @{
 */

/** @defgroup GIC_LL_EF_ICI_Management ICI_Management
 * @{
 */

/**
 * @brief  Get 8-bit message received by CORE0 when receiving inter-core interrupt from CORE0
 * @note   Only CORE0 host can read and write this register.
 * @rmtoll CORE0_ICI_STA0      ICI_INFO        LL_ICI_GetCore0ToCore0Message
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF
 */
__STATIC_INLINE uint8_t LL_ICI_GetCore0ToCore0Message(void)
{
    return (uint8_t)(READ_BIT(GIC_CORE0->CORE0_ICI_STA0, GIC_CORE0_ICI_STA0_ICI_INFO) >> GIC_CORE0_ICI_STA0_ICI_INFO_POS);
}

/**
 * @brief  Get 8-bit message received by CORE0 when receiving inter-core interrupt from CORE1
 * @note   Only CORE0 host can read and write this register.
 * @rmtoll CORE0_ICI_STA1      ICI_INFO        LL_ICI_GetCore1ToCore0Message
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF
 */
__STATIC_INLINE uint8_t LL_ICI_GetCore1ToCore0Message(void)
{
    return (uint8_t)(READ_BIT(GIC_CORE0->CORE0_ICI_STA1, GIC_CORE0_ICI_STA1_ICI_INFO) >> GIC_CORE0_ICI_STA1_ICI_INFO_POS);
}

/**
 * @brief  Get 8-bit message received by CORE1 when receiving inter-core interrupt from CORE0
 * @note   Only CORE1 host can read and write this register.
 * @rmtoll CORE1_ICI_STA0      ICI_INFO        LL_ICI_GetCore0ToCore1Message
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF
 */
__STATIC_INLINE uint8_t LL_ICI_GetCore0ToCore1Message(void)
{
    return (uint8_t)(READ_BIT(GIC_CORE1->CORE1_ICI_STA0, GIC_CORE1_ICI_STA0_ICI_INFO) >> GIC_CORE1_ICI_STA0_ICI_INFO_POS);
}

/**
 * @brief  Get 8-bit message received by CORE1 when receiving inter-core interrupt from CORE1
 * @note   Only CORE1 host can read and write this register.
 * @rmtoll CORE1_ICI_STA1      ICI_INFO        LL_ICI_GetCore1ToCore1Message
 * @retval Value between Min_Data=0x00 and Max_Data=0xFF
 */
__STATIC_INLINE uint8_t LL_ICI_GetCore1ToCore1Message(void)
{
    return (uint8_t)(READ_BIT(GIC_CORE1->CORE1_ICI_STA1, GIC_CORE1_ICI_STA1_ICI_INFO) >> GIC_CORE1_ICI_STA1_ICI_INFO_POS);
}

/**
 * @brief  Generate a inter-core interrupt to the core0 or core1
 * @note   Write only.
 * @rmtoll ICI_GEN          DESTINATION          LL_ICI_GenerateInterCoreInterrupt\n
 *         ICI_GEN          ICI_INT_INFO         LL_ICI_GenerateInterCoreInterrupt
 * @param  GICx GIC Instance.
 * @param  RceiveCoreId This parameter can be one of the following values:
 *         @arg @ref LL_ICI_COREID_0
 *         @arg @ref LL_ICI_COREID_1
 * @param  MessageInfo Value between Min_Data=0x0 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_ICI_GenerateInterCoreInterrupt(GIC_TypeDef *GICx, uint32_t RceiveCoreId, uint32_t MessageInfo)
{
    MODIFY_REG(GICx->ICI_GEN, GIC_ICI_GEN_DESTINATION | GIC_ICI_GEN_ICI_INT_INFO, RceiveCoreId | (MessageInfo << GIC_ICI_GEN_ICI_INT_INFO_POS));
}

/**
 * @}
 */

/** @defgroup GIC_LL_EF_Management Management
 * @{
 */

/**
 * @brief  Enable DMA last transfer.
 * @note   Only CORE0 host can read and write this register.
 *         Once locked, it cannot be unlocked by writing other values,
            only reset can unlock it.
 * @rmtoll CORE_IND_LOCK          CORE_IND_LOCK          LL_GIC_LockCoreInterruptConfig
 * @retval None
 */
__STATIC_INLINE void LL_GIC_LockCoreInterruptConfig(void)
{
    SET_BIT(GIC_CORE0->CORE_IND_LOCK, GIC_CORE_IND_LOCK_CORE_IND_LOCK);
}

/**
 * @brief  Check if DMA last transfer is enabled or disabled.
 * @note   Only CORE0 host can read and write this register.
 *         Once locked, it cannot be unlocked by writing other values,
            only reset can unlock it.
 * @rmtoll CORE_IND_LOCK          CORE_IND_LOCK          LL_GIC_IsLockedCoreInterruptConfig
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_GIC_IsLockedCoreInterruptConfig(void)
{
    return (READ_BIT(GIC_CORE0->CORE_IND_LOCK, GIC_CORE_IND_LOCK_CORE_IND_LOCK) == (GIC_CORE_IND_LOCK_CORE_IND_LOCK));
}

/**
 * @brief  Get the Number of cores in the system.
 * @rmtoll CORE_NUM      CORE_NUM        LL_GIC_GetCoreNum
 * @param  GICx GIC Instance.
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_GIC_GetCoreNum(GIC_TypeDef *GICx)
{
    return (uint32_t)(READ_BIT(GICx->CORE_NUM, GIC_CORE_NUM_CORE_NUM) >> GIC_CORE_NUM_CORE_NUM_POS);
}

/**
 * @brief  Get the Number of HART buses in the system.
 * @rmtoll HART_NUM      HART_NUM        LL_GIC_GetHartNum
 * @param  GICx GIC Instance.
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_GIC_GetHartNum(GIC_TypeDef *GICx)
{
    return (uint32_t)(READ_BIT(GICx->HART_NUM, GIC_HART_NUM_HART_NUM) >> GIC_HART_NUM_HART_NUM_POS);
}

/**
 * @brief  Get the Number of HART buses in the system.
 * @rmtoll SOC_IRQ_NUM      SOC_IRQ_NUM        LL_GIC_GetSocIrqNum
 * @param  GICx GIC Instance.
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_GIC_GetSocIrqNum(GIC_TypeDef *GICx)
{
    return (uint32_t)(READ_BIT(GICx->SOC_IRQ_NUM, GIC_SOC_IRQ_NUM_SOC_IRQ_NUM) >> GIC_SOC_IRQ_NUM_SOC_IRQ_NUM_POS);
}

/**
 * @brief  Assigned SOC Interrupt to CORE0.
 * @note   CORE0 can read/write this bit, CORE1 cannot read this bit.
 * @rmtoll SOC_INTn_INDICATOR          SOC_INT0_INDICATOR          LL_GIC_AssignIrqToCore0
 * @param  IRQNum The parameter can be one of the following values:
 *         @arg @ref LL_GIC_IRQNUM_LCD_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ0
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ1
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ2
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ3
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ4
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ5
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ6
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ7
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ8
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ9
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ10
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ11
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ12
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ13
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ14
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ15
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ0
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ1
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ2
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ3
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ4
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ5
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ6
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ7
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ8
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ9
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ10
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ11
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ12
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ13
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ14
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ15
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_WDT0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_WDT1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA11_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA10_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA01_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA00_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SYSTC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_TC0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_TC1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ISO78160_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ISO78161_IRQn
 *         @arg @ref LL_GIC_IRQNUM_I2C0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_I2C1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LPUART_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LSRX_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART2_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART3_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART4_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART5_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART6_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART7_IRQn
 *         @arg @ref LL_GIC_IRQNUM_USART0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_USART1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CAN_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI2_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI3_IRQn
 *         @arg @ref LL_GIC_IRQNUM_QSPI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_QSPI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_DSP_IRQn
 *         @arg @ref LL_GIC_IRQNUM_M2M_IRQn
 *         @arg @ref LL_GIC_IRQNUM_AES_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SEA_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KS_IRQn
 *         @arg @ref LL_GIC_IRQNUM_DAC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ADC0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_D2F_IRQn
 *         @arg @ref LL_GIC_IRQNUM_RTC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_VBAT_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CMPLVD_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LCSS_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SCU_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE1_RST_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE1_SYSREG_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE0_SYSREG_IRQn
 *         @arg @ref LL_GIC_IRQNUM_MAILBOSINTER_IRQn
 *         @arg @ref LL_GIC_IRQNUM_MAILBOSHOST0_IRQn
 * @retval None
 */
__STATIC_INLINE void LL_GIC_AssignIrqToCore0(uint32_t IRQNum)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&GIC_CORE0->INT20_IND_RESERVED17) + (4 * (IRQNum - 20))));
    SET_BIT(*pReg, GIC_INT20_IND_RESERVED17_SOC_INT0_INDICATOR);
}

/**
 * @brief  Not assigned SOC Interrupt to CORE0.
 * @note   CORE0 can read/write this bit, CORE1 cannot read this bit.
 * @rmtoll SOC_INTn_INDICATOR          SOC_INT0_INDICATOR          LL_GIC_UnassignIrqToCore0
 * @param  IRQNum The parameter can be one of the following values:
 *         @arg @ref LL_GIC_IRQNUM_LCD_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ0
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ1
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ2
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ3
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ4
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ5
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ6
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ7
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ8
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ9
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ10
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ11
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ12
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ13
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ14
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ15
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ0
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ1
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ2
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ3
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ4
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ5
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ6
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ7
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ8
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ9
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ10
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ11
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ12
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ13
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ14
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ15
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_WDT0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_WDT1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA11_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA10_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA01_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA00_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SYSTC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_TC0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_TC1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ISO78160_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ISO78161_IRQn
 *         @arg @ref LL_GIC_IRQNUM_I2C0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_I2C1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LPUART_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LSRX_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART2_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART3_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART4_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART5_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART6_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART7_IRQn
 *         @arg @ref LL_GIC_IRQNUM_USART0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_USART1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CAN_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI2_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI3_IRQn
 *         @arg @ref LL_GIC_IRQNUM_QSPI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_QSPI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_DSP_IRQn
 *         @arg @ref LL_GIC_IRQNUM_M2M_IRQn
 *         @arg @ref LL_GIC_IRQNUM_AES_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SEA_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KS_IRQn
 *         @arg @ref LL_GIC_IRQNUM_DAC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ADC0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_D2F_IRQn
 *         @arg @ref LL_GIC_IRQNUM_RTC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_VBAT_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CMPLVD_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LCSS_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SCU_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE1_RST_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE1_SYSREG_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE0_SYSREG_IRQn
 *         @arg @ref LL_GIC_IRQNUM_MAILBOSINTER_IRQn
 *         @arg @ref LL_GIC_IRQNUM_MAILBOSHOST0_IRQn
 * @retval None
 */
__STATIC_INLINE void LL_GIC_UnassignIrqToCore0(uint32_t IRQNum)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&GIC_CORE0->INT20_IND_RESERVED17) + (4 * (IRQNum - 20))));
    CLEAR_BIT(*pReg, GIC_INT20_IND_RESERVED17_SOC_INT0_INDICATOR);
}

/**
 * @brief  Check if Assigned SOC Interrupt to CORE0 or not.
 * @note   CORE0 can read/write this bit, CORE1 cannot read this bit.
 * @rmtoll SOC_INTn_INDICATOR          SOC_INT0_INDICATOR          LL_GIC_IsAssignedIrqToCore0
 * @param  IRQNum The parameter can be one of the following values:
 *         @arg @ref LL_GIC_IRQNUM_LCD_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ0
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ1
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ2
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ3
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ4
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ5
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ6
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ7
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ8
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ9
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ10
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ11
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ12
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ13
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ14
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ15
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ0
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ1
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ2
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ3
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ4
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ5
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ6
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ7
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ8
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ9
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ10
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ11
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ12
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ13
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ14
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ15
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_WDT0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_WDT1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA11_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA10_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA01_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA00_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SYSTC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_TC0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_TC1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ISO78160_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ISO78161_IRQn
 *         @arg @ref LL_GIC_IRQNUM_I2C0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_I2C1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LPUART_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LSRX_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART2_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART3_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART4_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART5_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART6_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART7_IRQn
 *         @arg @ref LL_GIC_IRQNUM_USART0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_USART1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CAN_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI2_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI3_IRQn
 *         @arg @ref LL_GIC_IRQNUM_QSPI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_QSPI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_DSP_IRQn
 *         @arg @ref LL_GIC_IRQNUM_M2M_IRQn
 *         @arg @ref LL_GIC_IRQNUM_AES_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SEA_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KS_IRQn
 *         @arg @ref LL_GIC_IRQNUM_DAC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ADC0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_D2F_IRQn
 *         @arg @ref LL_GIC_IRQNUM_RTC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_VBAT_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CMPLVD_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LCSS_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SCU_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE1_RST_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE1_SYSREG_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE0_SYSREG_IRQn
 *         @arg @ref LL_GIC_IRQNUM_MAILBOSINTER_IRQn
 *         @arg @ref LL_GIC_IRQNUM_MAILBOSHOST0_IRQn
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_GIC_IsAssignedIrqToCore0(uint32_t IRQNum)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&GIC_CORE0->INT20_IND_RESERVED17) + (4 * (IRQNum - 20))));
    return (READ_BIT(*pReg, GIC_INT20_IND_RESERVED17_SOC_INT0_INDICATOR) == (GIC_INT20_IND_RESERVED17_SOC_INT0_INDICATOR));
}

/**
 * @brief  Assigned SOC Interrupt to CORE1.
 * @note   CORE0 can read/write this bit, CORE1 only can read this bit.
 * @rmtoll SOC_INTn_INDICATOR          SOC_INT1_INDICATOR          LL_GIC_AssignIrqToCore1
 * @param  IRQNum The parameter can be one of the following values:
 *         @arg @ref LL_GIC_IRQNUM_LCD_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ0
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ1
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ2
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ3
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ4
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ5
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ6
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ7
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ8
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ9
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ10
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ11
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ12
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ13
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ14
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ15
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ0
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ1
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ2
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ3
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ4
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ5
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ6
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ7
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ8
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ9
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ10
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ11
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ12
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ13
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ14
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ15
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_WDT0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_WDT1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA11_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA10_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA01_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA00_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SYSTC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_TC0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_TC1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ISO78160_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ISO78161_IRQn
 *         @arg @ref LL_GIC_IRQNUM_I2C0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_I2C1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LPUART_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LSRX_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART2_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART3_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART4_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART5_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART6_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART7_IRQn
 *         @arg @ref LL_GIC_IRQNUM_USART0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_USART1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CAN_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI2_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI3_IRQn
 *         @arg @ref LL_GIC_IRQNUM_QSPI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_QSPI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_DSP_IRQn
 *         @arg @ref LL_GIC_IRQNUM_M2M_IRQn
 *         @arg @ref LL_GIC_IRQNUM_AES_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SEA_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KS_IRQn
 *         @arg @ref LL_GIC_IRQNUM_DAC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ADC0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_D2F_IRQn
 *         @arg @ref LL_GIC_IRQNUM_RTC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_VBAT_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CMPLVD_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LCSS_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SCU_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE1_RST_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE1_SYSREG_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE0_SYSREG_IRQn
 *         @arg @ref LL_GIC_IRQNUM_MAILBOSINTER_IRQn
 *         @arg @ref LL_GIC_IRQNUM_MAILBOSHOST0_IRQn
 * @retval None
 */
__STATIC_INLINE void LL_GIC_AssignIrqToCore1(uint32_t IRQNum)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&GIC_CORE0->INT20_IND_RESERVED17) + (4 * (IRQNum - 20))));
    SET_BIT(*pReg, GIC_INT20_IND_RESERVED17_SOC_INT1_INDICATOR);
}

/**
 * @brief  Not assigned SOC Interrupt to CORE1.
 * @note   CORE0 can read/write this bit, CORE1 only can read this bit.
 * @rmtoll SOC_INTn_INDICATOR          SOC_INT1_INDICATOR          LL_GIC_UnassignIrqToCore1
 * @param  IRQNum The parameter can be one of the following values:
 *         @arg @ref LL_GIC_IRQNUM_LCD_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ0
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ1
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ2
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ3
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ4
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ5
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ6
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ7
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ8
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ9
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ10
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ11
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ12
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ13
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ14
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ15
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ0
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ1
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ2
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ3
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ4
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ5
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ6
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ7
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ8
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ9
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ10
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ11
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ12
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ13
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ14
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ15
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_WDT0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_WDT1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA11_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA10_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA01_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA00_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SYSTC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_TC0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_TC1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ISO78160_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ISO78161_IRQn
 *         @arg @ref LL_GIC_IRQNUM_I2C0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_I2C1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LPUART_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LSRX_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART2_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART3_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART4_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART5_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART6_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART7_IRQn
 *         @arg @ref LL_GIC_IRQNUM_USART0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_USART1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CAN_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI2_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI3_IRQn
 *         @arg @ref LL_GIC_IRQNUM_QSPI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_QSPI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_DSP_IRQn
 *         @arg @ref LL_GIC_IRQNUM_M2M_IRQn
 *         @arg @ref LL_GIC_IRQNUM_AES_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SEA_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KS_IRQn
 *         @arg @ref LL_GIC_IRQNUM_DAC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ADC0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_D2F_IRQn
 *         @arg @ref LL_GIC_IRQNUM_RTC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_VBAT_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CMPLVD_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LCSS_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SCU_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE1_RST_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE1_SYSREG_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE0_SYSREG_IRQn
 *         @arg @ref LL_GIC_IRQNUM_MAILBOSINTER_IRQn
 *         @arg @ref LL_GIC_IRQNUM_MAILBOSHOST0_IRQn
 * @retval None
 */
__STATIC_INLINE void LL_GIC_UnassignIrqToCore1(uint32_t IRQNum)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&GIC_CORE0->INT20_IND_RESERVED17) + (4 * (IRQNum - 20))));
    CLEAR_BIT(*pReg, GIC_INT20_IND_RESERVED17_SOC_INT1_INDICATOR);
}

/**
 * @brief  Check if Assigned SOC Interrupt to CORE1 or not.
 * @note   CORE0 can read/write this bit, CORE1 only can read this bit.
 * @rmtoll SOC_INTn_INDICATOR          SOC_INT1_INDICATOR          LL_GIC_IsAssignedIrqToCore1
 * @param  GICx GIC Instance
 * @param  IRQNum The parameter can be one of the following values:
 *         @arg @ref LL_GIC_IRQNUM_LCD_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ0
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ1
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ2
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ3
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ4
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ5
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ6
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ7
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ8
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ9
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ10
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ11
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ12
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ13
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ14
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQ15
 *         @arg @ref LL_GIC_IRQNUM_KBI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ0
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ1
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ2
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ3
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ4
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ5
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ6
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ7
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ8
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ9
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ10
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ11
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ12
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ13
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ14
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQ15
 *         @arg @ref LL_GIC_IRQNUM_KBI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_WDT0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_WDT1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA11_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA10_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA01_IRQn
 *         @arg @ref LL_GIC_IRQNUM_GDMA00_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SYSTC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC0_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC1_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC2_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_0_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_1_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_2_IRQ
 *         @arg @ref LL_GIC_IRQNUM_SIMPTC3_3_IRQ
 *         @arg @ref LL_GIC_IRQNUM_TC0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_TC1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ISO78160_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ISO78161_IRQn
 *         @arg @ref LL_GIC_IRQNUM_I2C0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_I2C1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LPUART_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LSRX_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART2_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART3_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART4_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART5_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART6_IRQn
 *         @arg @ref LL_GIC_IRQNUM_UART7_IRQn
 *         @arg @ref LL_GIC_IRQNUM_USART0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_USART1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CAN_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI2_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SPI3_IRQn
 *         @arg @ref LL_GIC_IRQNUM_QSPI0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_QSPI1_IRQn
 *         @arg @ref LL_GIC_IRQNUM_DSP_IRQn
 *         @arg @ref LL_GIC_IRQNUM_M2M_IRQn
 *         @arg @ref LL_GIC_IRQNUM_AES_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SEA_IRQn
 *         @arg @ref LL_GIC_IRQNUM_KS_IRQn
 *         @arg @ref LL_GIC_IRQNUM_DAC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_ADC0_IRQn
 *         @arg @ref LL_GIC_IRQNUM_D2F_IRQn
 *         @arg @ref LL_GIC_IRQNUM_RTC_IRQn
 *         @arg @ref LL_GIC_IRQNUM_VBAT_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CMPLVD_IRQn
 *         @arg @ref LL_GIC_IRQNUM_LCSS_IRQn
 *         @arg @ref LL_GIC_IRQNUM_SCU_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE1_RST_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE1_SYSREG_IRQn
 *         @arg @ref LL_GIC_IRQNUM_CORE0_SYSREG_IRQn
 *         @arg @ref LL_GIC_IRQNUM_MAILBOSINTER_IRQn
 *         @arg @ref LL_GIC_IRQNUM_MAILBOSHOST0_IRQn
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_GIC_IsAssignedIrqToCore1(GIC_TypeDef *GICx, uint32_t IRQNum)
{
    __IO uint32_t *pReg = (__IO uint32_t *)((uint32_t)((uint32_t)(&GICx->INT20_IND_RESERVED17) + (4 * (IRQNum - 20))));
    return (READ_BIT(*pReg, GIC_INT20_IND_RESERVED17_SOC_INT1_INDICATOR) == (GIC_INT20_IND_RESERVED17_SOC_INT1_INDICATOR));
}

/**
 * @}
 */

/** @defgroup GIC_LL_EF_ICI_FLAG_management ICI_FLAG_management
 * @{
 */

/**
 * @brief  Check if the Inter-core interrupt status (sent from CORE0 to CORE0) is set or not.
 * @note   Only CORE0 host can read and write this register.
 * @rmtoll CORE0_ICI_STA0          ICI_STATUS           LL_ICI_IsActiveFlag_CORE0ToCORE0
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ICI_IsActiveFlag_CORE0ToCORE0(void)
{
    return (READ_BIT(GIC_CORE0->CORE0_ICI_STA0, GIC_CORE0_ICI_STA0_ICI_STATUS) == (GIC_CORE0_ICI_STA0_ICI_STATUS));
}

/**
 * @brief  Clear Inter-core interrupt status (sent from CORE0 to CORE0) Flag.
 * @note   Only CORE0 host can read and write this register.
 * @rmtoll CORE0_ICI_STA0          ICI_STATUS           LL_ICI_ClearFlag_CORE0ToCORE0
 * @retval None
 */
__STATIC_INLINE void LL_ICI_ClearFlag_CORE0ToCORE0(void)
{
    SET_BIT(GIC_CORE0->CORE0_ICI_STA0, GIC_CORE0_ICI_STA0_ICI_STATUS);
}

/**
 * @brief  Check if the Inter-core interrupt status (sent from CORE1 to CORE0) is set or not.
 * @note   Only CORE0 host can read and write this register.
 * @rmtoll CORE0_ICI_STA1          ICI_STATUS           LL_ICI_IsActiveFlag_CORE1ToCORE0
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ICI_IsActiveFlag_CORE1ToCORE0(void)
{
    return (READ_BIT(GIC_CORE0->CORE0_ICI_STA1, GIC_CORE0_ICI_STA1_ICI_STATUS) == (GIC_CORE0_ICI_STA1_ICI_STATUS));
}

/**
 * @brief  Clear Inter-core interrupt status (sent from CORE1 to CORE0) Flag.
 * @note   Only CORE0 host can read and write this register.
 * @rmtoll CORE0_ICI_STA1          ICI_STATUS           LL_ICI_ClearFlag_CORE1ToCORE0
 * @retval None
 */
__STATIC_INLINE void LL_ICI_ClearFlag_CORE1ToCORE0(void)
{
    SET_BIT(GIC_CORE0->CORE0_ICI_STA1, GIC_CORE0_ICI_STA1_ICI_STATUS);
}

/**
 * @brief  Check if the Inter-core interrupt status (sent from CORE0 to CORE1) is set or not.
 * @note   Only CORE1 host can read and write this register.
 * @rmtoll CORE1_ICI_STA0          ICI_STATUS           LL_ICI_IsActiveFlag_CORE0ToCORE1
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ICI_IsActiveFlag_CORE0ToCORE1(void)
{
    return (READ_BIT(GIC_CORE1->CORE1_ICI_STA0, GIC_CORE1_ICI_STA0_ICI_STATUS) == (GIC_CORE1_ICI_STA0_ICI_STATUS));
}

/**
 * @brief  Clear Inter-core interrupt status (sent from CORE0 to CORE1) Flag.
 * @note   Only CORE1 host can read and write this register.
 * @rmtoll CORE1_ICI_STA0          ICI_STATUS           LL_ICI_ClearFlag_CORE0ToCORE1
 * @retval None
 */
__STATIC_INLINE void LL_ICI_ClearFlag_CORE0ToCORE1(void)
{
    SET_BIT(GIC_CORE1->CORE1_ICI_STA0, GIC_CORE1_ICI_STA0_ICI_STATUS);
}

/**
 * @brief  Check if the Inter-core interrupt status (sent from CORE1 to CORE1) is set or not.
 * @note   Only CORE1 host can read and write this register.
 * @rmtoll CORE1_ICI_STA1          ICI_STATUS           LL_ICI_IsActiveFlag_CORE1ToCORE1
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_ICI_IsActiveFlag_CORE1ToCORE1(void)
{
    return (READ_BIT(GIC_CORE1->CORE1_ICI_STA1, GIC_CORE1_ICI_STA1_ICI_STATUS) == (GIC_CORE1_ICI_STA1_ICI_STATUS));
}

/**
 * @brief  Clear Inter-core interrupt status (sent from CORE1 to CORE1) Flag.
 * @note   Only CORE1 host can read and write this register.
 * @rmtoll CORE1_ICI_STA1          ICI_STATUS           LL_ICI_ClearFlag_CORE1ToCORE1
 * @retval None
 */
__STATIC_INLINE void LL_ICI_ClearFlag_CORE1ToCORE1(void)
{
    SET_BIT(GIC_CORE1->CORE1_ICI_STA1, GIC_CORE1_ICI_STA1_ICI_STATUS);
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

#endif /* GIC */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_GIC_H */
