/**
 *****************************************************************************
 * @file rn_rv_ll_wdt.h
 * @author Renergy Software Team
 * @brief Header file of WDT LL module.
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
#ifndef __RN_LL_WDT_H
#define __RN_LL_WDT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_wdt.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined(WDT)

/** @defgroup WDT_LL WDT
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/** @defgroup WDT_LL_Private_Constants  WDT Private Constants
 * @{
 */

/** @defgroup WDT_LL_PC_RELOAD_PASSWORD  WDT reload value and password
 * @{
 */
#define LL_WDT_WDTE_RELOAD                 0x000000BBU               /*!< WDT Reload Counter Enable   */
#define LL_WDT_PASS_WR_ACCESS_PASSWORD1    0x00000070U               /*!< WDT PASS Write Access Enable password1  */
#define LL_WDT_PASS_WR_ACCESS_PASSWORD2    0x0000007EU               /*!< WDT PASS Write Access Enable password2  */
#define LL_WDT_PASS_WR_ACCESS_DISABLE      0x00000000U               /*!< WDT PASS Write Access Disable  */
#define LL_WDT_HALT_ENABLE                 0xA5A5A581U               /*!< WDT HALT Enable  */
#define LL_WDT_HALT_DISABLE                0x5A5A5A7EU               /*!< WDT HALT Disable  */
#define LL_WDT_STBY_ENABLE                 0xA5A5A5C7U               /*!< WDT STBY Enable  */
#define LL_WDT_STBY_DISABLE                0x5A5A5A38U               /*!< WDT STBY Disable  */
#define LL_WDT_STOP_ENABLE                 0xAA5555AAU               /*!< WDT STOP Enable  */
#define LL_WDT_STOP_DISABLE                0x55AAAA55U               /*!< WDT STOP Disable  */
/**
 * @}
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup WDT_LL_Private_Macros  WDT Private Macros
 * @{
 */

/** @defgroup WDT_LL_PM_WAITIDLE  Wait for write idle
 * @{
 */
#define LL_WDT_WAITIDLE(_INSTANCE_)            while((READ_BIT(_INSTANCE_->EN, WDT_EN_WR_BUSY) == WDT_EN_WR_BUSY))
/**
 * @}
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/
/** @defgroup WDT_LL_Exported_Constants  WDT Exported Constants
 * @{
 */

/** @defgroup WDT_LL_EC_PRESCALER  Prescaler Divider
 * @{
 */
#define LL_WDT_PRESCALER_NONE          WDT_CTL_CLKDIV_DIV0             /*!< Select clock no division */
#define LL_WDT_PRESCALER_2             WDT_CTL_CLKDIV_DIV2             /*!< Select clock divider by 2 */
#define LL_WDT_PRESCALER_4             WDT_CTL_CLKDIV_DIV4             /*!< Select clock divider by 4 */
#define LL_WDT_PRESCALER_8             WDT_CTL_CLKDIV_DIV8             /*!< Select clock divider by 8 */
#define LL_WDT_PRESCALER_16            WDT_CTL_CLKDIV_DIV16            /*!< Select clock divider by 16 */
#define LL_WDT_PRESCALER_32            WDT_CTL_CLKDIV_DIV32            /*!< Select clock divider by 32 */
#define LL_WDT_PRESCALER_64            WDT_CTL_CLKDIV_DIV64            /*!< Select clock divider by 64 */
#define LL_WDT_PRESCALER_128           WDT_CTL_CLKDIV_DIV128           /*!< Select clock divider by 128 */
/**
 * @}
 */

/** @defgroup WDT_LL_EC_WINDOW  Window of counter
 * @{
 */
#define LL_WDT_WINDOW_PERSENT_25       WDT_CTL_WINDOW_ONEQUARTER       /*!< Select 25% of overflow time */
#define LL_WDT_WINDOW_PERSENT_50       WDT_CTL_WINDOW_HALF             /*!< Select 50% of overflow time */
#define LL_WDT_WINDOW_PERSENT_75       WDT_CTL_WINDOW_THREEQUARTERS    /*!< Select 75% of overflow time */
#define LL_WDT_WINDOW_PERSENT_100      WDT_CTL_WINDOW_FULL             /*!< Select 100% of overflow time */
/**
 * @}
 */

/** @defgroup WDT_LL_EC_OVFTIME  Overflow time
 * @{
 */
#define LL_WDT_OVFTIME_15_625MS     WDT_CTL_WDCS_0                  /*!< Select Overflow time 15.625ms */
#define LL_WDT_OVFTIME_31_25MS      WDT_CTL_WDCS_1                  /*!< Select Overflow time 31.25ms */
#define LL_WDT_OVFTIME_125MS        WDT_CTL_WDCS_2                  /*!< Select Overflow time 125ms */
#define LL_WDT_OVFTIME_500MS        WDT_CTL_WDCS_3                  /*!< Select Overflow time 500ms */
#define LL_WDT_OVFTIME_1S           WDT_CTL_WDCS_4                  /*!< Select Overflow time 1s */
#define LL_WDT_OVFTIME_2S           WDT_CTL_WDCS_5                  /*!< Select Overflow time 2s */
#define LL_WDT_OVFTIME_4S           WDT_CTL_WDCS_6                  /*!< Select Overflow time 4s */
#define LL_WDT_OVFTIME_8S           WDT_CTL_WDCS_7                  /*!< Select Overflow time 8s */
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup WDT_LL_Exported_Functions  WDT Exported Functions
 * @{
 */

/** @defgroup WDT_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief Indicates whether the write busy flag is set.
 * @note  If the busy flag is set to 1, the WDT_EN and WDT_CTL registers cannot be writed.
 *        They can only be written to when the write busy flag is 0.
 * @rmtoll EN   WR_BUSY   LL_WDT_IsActiveFlag_WRBUSY
 * @param WDTx    WDT Instance
 * @return State of bit, 1: busy, 0: idle.
 */
__STATIC_INLINE uint32_t LL_WDT_IsActiveFlag_WRBUSY(WDT_TypeDef *WDTx)
{
    return (uint32_t)(READ_BIT(WDTx->EN, WDT_EN_WR_BUSY));
}

/**
 * @brief Reload counter of WDT.
 * @rmtoll EN   WDTE   LL_WDT_ReloadCounter
 * @param WDTx  WDT Instance
 * @return None
 */
__STATIC_INLINE void LL_WDT_ReloadCounter(WDT_TypeDef *WDTx)
{
    LL_WDT_WAITIDLE(WDTx);
    MODIFY_REG(WDTx->EN, WDT_EN_WDTE, LL_WDT_WDTE_RELOAD);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Read WDTE value.
 * @rmtoll EN   WDTE   LL_WDT_GetWDTE
 * @param WDTx    WDT Instance
 * @return Value of WDTE
 */
__STATIC_INLINE uint32_t LL_WDT_GetWDTE(WDT_TypeDef *WDTx)
{
    return (uint32_t)(READ_BIT(WDTx->EN, WDT_EN_WDTE));
}

/**
 * @brief Configure counter of random wake.
 * @rmtoll CTL   WAKE_CNT   LL_WDT_SetRandomWakeCounter
 * @param WDTx    WDT Instance
 * @param Counter Counter of random wake which is 19 bits, 0<=Counter<=0x7FFFF.
 * @return None
 */
__STATIC_INLINE void LL_WDT_SetRandomWakeCounter(WDT_TypeDef *WDTx, uint32_t Counter)
{
    LL_WDT_WAITIDLE(WDTx);
    MODIFY_REG(WDTx->CTL, WDT_CTL_WAKE_CNT, (Counter << WDT_CTL_WAKE_CNT_POS));
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Return the counter of random wake.
 * @rmtoll CTL   WAKE_CNT   LL_WDT_GetRandomWakeCounter
 * @param WDTx  WDT Instance
 * @return Counter
 */
__STATIC_INLINE uint32_t LL_WDT_GetRandomWakeCounter(WDT_TypeDef *WDTx)
{
    return (uint32_t)(READ_BIT(WDTx->CTL, WDT_CTL_WAKE_CNT) >> WDT_CTL_WAKE_CNT_POS);
}

/**
 * @brief Enable random wake function.
 * @rmtoll CTL   RAD_WAKE   LL_WDT_EnableRandomWake
 * @param WDTx    WDT Instance
 * @return None
 */
__STATIC_INLINE void LL_WDT_EnableRandomWake(WDT_TypeDef *WDTx)
{
    LL_WDT_WAITIDLE(WDTx);
    SET_BIT(WDTx->CTL, WDT_CTL_RAD_WAKE);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Disable random wake function.
 * @rmtoll CTL   RAD_WAKE   LL_WDT_DisableRandomWake
 * @param WDTx  WDT Instance
 * @return None
 */
__STATIC_INLINE void LL_WDT_DisableRandomWake(WDT_TypeDef *WDTx)
{
    LL_WDT_WAITIDLE(WDTx);
    CLEAR_BIT(WDTx->CTL, WDT_CTL_RAD_WAKE);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Indicates whether random wake function is enabled.
 * @rmtoll CTL   RAD_WAKE   LL_WDT_IsEnabledRandomWake
 * @param WDTx  WDT Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_WDT_IsEnabledRandomWake(WDT_TypeDef *WDTx)
{
    return (uint32_t)((READ_BIT(WDTx->CTL, WDT_CTL_RAD_WAKE) == WDT_CTL_RAD_WAKE));
}

/**
 * @brief Configure Prescaler of clock.
 * @rmtoll CTL   CLKDIV   LL_WDT_SetPrescaler
 * @param WDTx WDT Instance
 * @param Prescaler This parameter can be one of the following values:
 *         @arg @ref LL_WDT_PRESCALER_NONE
 *         @arg @ref LL_WDT_PRESCALER_2
 *         @arg @ref LL_WDT_PRESCALER_4
 *         @arg @ref LL_WDT_PRESCALER_8
 *         @arg @ref LL_WDT_PRESCALER_16
 *         @arg @ref LL_WDT_PRESCALER_32
 *         @arg @ref LL_WDT_PRESCALER_64
 *         @arg @ref LL_WDT_PRESCALER_128
 * @return None
 */
__STATIC_INLINE void LL_WDT_SetPrescaler(WDT_TypeDef *WDTx, uint32_t Prescaler)
{
    LL_WDT_WAITIDLE(WDTx);
    MODIFY_REG(WDTx->CTL, WDT_CTL_CLKDIV, Prescaler);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Return Prescaler of clock.
 * @rmtoll CTL   CLKDIV   LL_WDT_GetPrescaler
 * @param WDTx  WDT Instance
 * @return The return value can be the one of the following values:
 *         @arg @ref LL_WDT_PRESCALER_NONE
 *         @arg @ref LL_WDT_PRESCALER_2
 *         @arg @ref LL_WDT_PRESCALER_4
 *         @arg @ref LL_WDT_PRESCALER_8
 *         @arg @ref LL_WDT_PRESCALER_16
 *         @arg @ref LL_WDT_PRESCALER_32
 *         @arg @ref LL_WDT_PRESCALER_64
 *         @arg @ref LL_WDT_PRESCALER_128
 */
__STATIC_INLINE uint32_t LL_WDT_GetPrescaler(WDT_TypeDef *WDTx)
{
    return (uint32_t)(READ_BIT(WDTx->CTL, WDT_CTL_CLKDIV));
}

/**
 * @brief Enable early wakeup interrupt.
 * @note  The interrupt occurs at 75% of overflow time.
 * @rmtoll CTL   WDTINT   LL_WDT_EnableIT_EWKUP
 * @param WDTx  WDT Instance
 * @return None
 */
__STATIC_INLINE void LL_WDT_EnableIT_EWKUP(WDT_TypeDef *WDTx)
{
    LL_WDT_WAITIDLE(WDTx);
    SET_BIT(WDTx->CTL, WDT_CTL_WDTINT);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Disable early wakeup interrupt.
 * @note  The interrupt occurs at 75% of overflow time.
 * @rmtoll CTL   WDTINT   LL_WDT_DisableIT_EWKUP
 * @param WDTx  WDT Instance
 * @return None
 */
__STATIC_INLINE void LL_WDT_DisableIT_EWKUP(WDT_TypeDef *WDTx)
{
    LL_WDT_WAITIDLE(WDTx);
    CLEAR_BIT(WDTx->CTL, WDT_CTL_WDTINT);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Indicates whether the early wakeup interrupt is enabled.
 * @note  The interrupt occurs at 75% of overflow time.
 * @rmtoll CTL   WDTINT   LL_WDT_IsEnabledIT_EWKUP
 * @param WDTx  WDT Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_WDT_IsEnabledIT_EWKUP(WDT_TypeDef *WDTx)
{
    return (uint32_t)((READ_BIT(WDTx->CTL, WDT_CTL_WDTINT) == WDT_CTL_WDTINT));
}

/**
 * @brief Configure window of counter.
 * @rmtoll CTL   WINDOW   LL_WDT_SetWindow
 * @param WDTx    WDT Instance
 * @param Window This parameter can be one of the following values:
 *         @arg @ref LL_WDT_WINDOW_PERSENT_25
 *         @arg @ref LL_WDT_WINDOW_PERSENT_50
 *         @arg @ref LL_WDT_WINDOW_PERSENT_75
 *         @arg @ref LL_WDT_WINDOW_PERSENT_100
 * @return None
 */
__STATIC_INLINE void LL_WDT_SetWindow(WDT_TypeDef *WDTx, uint32_t Window)
{
    LL_WDT_WAITIDLE(WDTx);
    MODIFY_REG(WDTx->CTL, WDT_CTL_WINDOW, Window);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Return the window of counter.
 * @rmtoll CTL   WINDOW   LL_WDT_GetWindow
 * @param WDTx  WDT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_WDT_WINDOW_PERSENT_25
 *         @arg @ref LL_WDT_WINDOW_PERSENT_50
 *         @arg @ref LL_WDT_WINDOW_PERSENT_75
 *         @arg @ref LL_WDT_WINDOW_PERSENT_100
 */
__STATIC_INLINE uint32_t LL_WDT_GetWindow(WDT_TypeDef *WDTx)
{
    return (uint32_t)(READ_BIT(WDTx->CTL, WDT_CTL_WINDOW));
}

/**
 * @brief Configure overflow time of counter.
 * @rmtoll CTL   WDCS   LL_WDT_SetOverflowTime
 * @param WDTx    WDT Instance
 * @param Time This parameter can be one of the following values:
 *         @arg @ref LL_WDT_OVFTIME_15_625MS
 *         @arg @ref LL_WDT_OVFTIME_31_25MS
 *         @arg @ref LL_WDT_OVFTIME_125MS
 *         @arg @ref LL_WDT_OVFTIME_500MS
 *         @arg @ref LL_WDT_OVFTIME_1S
 *         @arg @ref LL_WDT_OVFTIME_2S
 *         @arg @ref LL_WDT_OVFTIME_4S
 *         @arg @ref LL_WDT_OVFTIME_8S
 * @return None
 */
__STATIC_INLINE void LL_WDT_SetOverflowTime(WDT_TypeDef *WDTx, uint32_t Time)
{
    LL_WDT_WAITIDLE(WDTx);
    MODIFY_REG(WDTx->CTL, WDT_CTL_WDCS, Time);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Return reload time of counter.
 * @rmtoll CTL   WDCS   LL_WDT_GetOverflowTime
 * @param WDTx  WDT Instance
 * @return Return value can be one of the following values:
 *         @arg @ref LL_WDT_OVFTIME_15_625MS
 *         @arg @ref LL_WDT_OVFTIME_31_25MS
 *         @arg @ref LL_WDT_OVFTIME_125MS
 *         @arg @ref LL_WDT_OVFTIME_500MS
 *         @arg @ref LL_WDT_OVFTIME_1S
 *         @arg @ref LL_WDT_OVFTIME_2S
 *         @arg @ref LL_WDT_OVFTIME_4S
 *         @arg @ref LL_WDT_OVFTIME_8S
 */
__STATIC_INLINE uint32_t LL_WDT_GetOverflowTime(WDT_TypeDef *WDTx)
{
    return (uint32_t)(READ_BIT(WDTx->CTL, WDT_CTL_WDCS));
}

/**
 * @brief Enable write access of WDT register.
 * @rmtoll PASS   PASS_H   LL_WDT_EnableWriteAccess
 *         PASS   PASS_L   LL_WDT_EnableWriteAccess
 * @param WDTx    WDT Instance
 * @return None
 */
__STATIC_INLINE void LL_WDT_EnableWriteAccess(WDT_TypeDef *WDTx)
{
    LL_WDT_WAITIDLE(WDTx);
    WRITE_REG(WDTx->PASS, LL_WDT_PASS_WR_ACCESS_PASSWORD1);
    WRITE_REG(WDTx->PASS, LL_WDT_PASS_WR_ACCESS_PASSWORD2);
}

/**
 * @brief Disable write access of WDT register.
 * @rmtoll PASS   PASS_H   LL_WDT_DisableWriteAccess
 *         PASS   PASS_L   LL_WDT_DisableWriteAccess
 * @param WDTx  WDT Instance
 * @return None
 */
__STATIC_INLINE void LL_WDT_DisableWriteAccess(WDT_TypeDef *WDTx)
{
    LL_WDT_WAITIDLE(WDTx);
    WRITE_REG(WDTx->PASS, LL_WDT_PASS_WR_ACCESS_DISABLE);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Indicates whether write access is enabled.
 * @rmtoll PASS   PASS   LL_WDT_DisableWriteAccess
 * @param WDTx  WDT Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_WDT_IsEnabledWriteAccess(WDT_TypeDef *WDTx)
{
    return (uint32_t)((READ_REG(WDTx->PASS) == LL_WDT_PASS_WR_ACCESS_PASSWORD2));
}

/**
 * @brief Enable HALT function.
 * @note  When CPU is in halt state, if HALT_EN = 0xA5A5A581, the WDT stops counting, otherwise continues counting.
 * @rmtoll HALT   HALT_EN   LL_WDT_EnableHalt
 * @param WDTx    WDT Instance
 * @return None
 */
__STATIC_INLINE void LL_WDT_EnableHalt(WDT_TypeDef *WDTx)
{
    LL_WDT_WAITIDLE(WDTx);
    WRITE_REG(WDTx->HALT, LL_WDT_HALT_ENABLE);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Disable HALT function.
 * @note  When CPU is in halt state, if HALT_EN = 0xA5A5A581, the WDT stops counting, otherwise continues counting.
 * @rmtoll HALT   HALT_EN   LL_WDT_DisableHalt
 * @param WDTx  WDT Instance
 * @return None
 */
__STATIC_INLINE void LL_WDT_DisableHalt(WDT_TypeDef *WDTx)
{
    LL_WDT_WAITIDLE(WDTx);
    WRITE_REG(WDTx->HALT, LL_WDT_HALT_DISABLE);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Indicates whether HALT function is enabled.
 * @note  When CPU is in halt state, if HALT_EN = 0xA5A5A581, the WDT stops counting, otherwise continues counting.
 * @rmtoll HALT   HALT_EN   LL_WDT_IsEnabledHalt
 * @param WDTx  WDT Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_WDT_IsEnabledHalt(WDT_TypeDef *WDTx)
{
    return (uint32_t)((READ_REG(WDTx->HALT) == LL_WDT_HALT_ENABLE));
}

/**
 * @brief Enable STBY function.
 * @note  When CPU is in standby state, if STBY_EN = 0xA5A5A5C7, the WDT stops counting, otherwise continues counting.
 * @rmtoll STBY   STBY_EN   LL_WDT_EnableStandby
 * @param WDTx    WDT Instance
 * @return None
 */
__STATIC_INLINE void LL_WDT_EnableStandby(WDT_TypeDef *WDTx)
{
    LL_WDT_WAITIDLE(WDTx);
    WRITE_REG(WDTx->STBY, LL_WDT_STBY_ENABLE);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Disable STBY function.
 * @note  When CPU is in standby state, if STBY_EN = 0xA5A5A5C7, the WDT stops counting, otherwise continues counting.
 * @rmtoll STBY   STBY_EN   LL_WDT_DisableStandby
 * @param WDTx  WDT Instance
 * @return None
 */
__STATIC_INLINE void LL_WDT_DisableStandby(WDT_TypeDef *WDTx)
{
    LL_WDT_WAITIDLE(WDTx);
    WRITE_REG(WDTx->STBY, LL_WDT_STBY_DISABLE);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Indicates whether STBY function is enabled.
 * @note  When CPU is in standby state, if STBY_EN = 0xA5A5A5C7, the WDT stops counting, otherwise continues counting.
 * @rmtoll STBY   STBY_EN   LL_WDT_IsEnabledStandby
 * @param WDTx  WDT Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_WDT_IsEnabledStandby(WDT_TypeDef *WDTx)
{
    return (uint32_t)((READ_REG(WDTx->STBY) == LL_WDT_STBY_ENABLE));
}

/**
 * @brief Enable STOP function.
 * @note  When CPU is in stop state, if STOP_EN = 0xAA5555AA, the WDT stops counting, otherwise continues counting.
 * @rmtoll STOP   STOP_EN   LL_WDT_EnableStop
 * @param WDTx    WDT Instance
 * @return None
 */
__STATIC_INLINE void LL_WDT_EnableStop(WDT_TypeDef *WDTx)
{
    LL_WDT_WAITIDLE(WDTx);
    WRITE_REG(WDTx->STOP, LL_WDT_STOP_ENABLE);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Disable STOP function.
 * @note  When CPU is in stop state, if STOP_EN = 0xAA5555AA, the WDT stops counting, otherwise continues counting.
 * @rmtoll STOP   STOP_EN   LL_WDT_DisableStop
 * @param WDTx  WDT Instance
 * @return None
 */
__STATIC_INLINE void LL_WDT_DisableStop(WDT_TypeDef *WDTx)
{
    LL_WDT_WAITIDLE(WDTx);
    WRITE_REG(WDTx->STOP, LL_WDT_STOP_DISABLE);
    LL_WDT_WAITIDLE(WDTx);
}

/**
 * @brief Indicates whether STOP function is enabled.
 * @note  When CPU is in stop state, if STOP_EN = 0xAA5555AA, the WDT stops counting, otherwise continues counting.
 * @rmtoll STOP   STOP_EN   LL_WDT_IsEnabledStop
 * @param WDTx  WDT Instance
 * @return State of bit, 1: enabled, 0: disabled.
 */
__STATIC_INLINE uint32_t LL_WDT_IsEnabledStop(WDT_TypeDef *WDTx)
{
    return (uint32_t)((READ_REG(WDTx->STOP) == LL_WDT_STOP_ENABLE));
}

/**
 * @brief Read couter of Watchdog.
 * @rmtoll CNT   CNT   LL_WDT_GetCounter
 * @param WDTx  WDT Instance
 * @return Couter
 */
__STATIC_INLINE uint32_t LL_WDT_GetCounter(WDT_TypeDef *WDTx)
{
    return (uint32_t)(READ_REG(WDTx->CNT));
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

#endif /* WDT */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_WDT_H */
