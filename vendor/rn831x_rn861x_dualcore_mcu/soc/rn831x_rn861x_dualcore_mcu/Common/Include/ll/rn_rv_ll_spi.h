/**
 *****************************************************************************
 * @file   rn_rv_ll_spi.h
 * @author Renergy Software Team
 * @brief  Header file of SPI LL module.
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
#ifndef __RN_LL_SPI_H
#define __RN_LL_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_spi.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (SPI0) || defined (SPI1) || defined (SPI2) || defined (SPI3)

/** @defgroup SPI_LL SPI
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup SPI_LL_Private_Constants SPI Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup SPI_LL_Private_Macros SPI Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup SPI_LL_ES_INIT SPI Exported Init structure
 * @{
 */

/**
 * @brief LL SPI Init structures definition
 */
typedef struct
{
    uint32_t Mode;                    /*!< Specifies the SPI mode (Master/Slave).
                                            This parameter can be a value of @ref SPI_LL_EC_MODE.

                                            This feature can be modified afterwards using unitary
                                            function @ref LL_SPI_SetMode().*/

    uint32_t DataWidth;               /*!< Specifies the SPI data width.
                                            This parameter can be a value of @ref SPI_LL_EC_DATAWIDTH.

                                            This feature can be modified afterwards using unitary
                                            function @ref LL_SPI_SetDataWidth().*/

    uint32_t ClockPolarity;           /*!< Specifies the serial clock steady state.
                                            This parameter can be a value of @ref SPI_LL_EC_POLARITY.

                                            This feature can be modified afterwards using unitary
                                            function @ref LL_SPI_SetClockPolarity().*/

    uint32_t ClockPhase;              /*!< Specifies the clock active edge for the bit capture.
                                            This parameter can be a value of @ref SPI_LL_EC_PHASE.

                                            This feature can be modified afterwards using unitary
                                            function @ref LL_SPI_SetClockPhase().*/

    uint32_t ClockDivision;           /*!< Specifies the BaudRate prescaler value which will be used
                                            to configure the transmit and receive SCK clock.
                                            This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF.
                                            @note The communication clock is derived from the master clock.
                                                   The slave clock does not need to be set.

                                            This feature can be modified afterwards using unitary
                                            function @ref LL_SPI_SetBaudRatePrescaler().*/

    uint32_t BitOrder;                /*!< Specifies whether data transfers start from MSB or LSB bit.
                                            This parameter can be a value of @ref SPI_LL_EC_BIT_ORDER.

                                            This feature can be modified afterwards using unitary
                                            function @ref LL_SPI_SetTransferBitOrder().*/

} LL_SPI_InitTypeDef;

/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/* Exported constants -------------------------------------------------------*/
/** @defgroup SPI_LL_Exported_Constants SPI Exported Constants
 * @{
 */

/** @defgroup SPI_LL_EC_MODE Operation Mode
 * @{
 */
#define LL_SPI_MODE_MASTER                 (SPI_CTRL_MAST_SLAV)           /*!< Master configuration  */
#define LL_SPI_MODE_SLAVE                  0x00000000U                    /*!< Slave configuration   */
/**
 * @}
 */

/** @defgroup SPI_LL_EC_PHASE Clock Phase
 * @{
 */
#define LL_SPI_PHASE_1EDGE                 0x00000000U                    /*!< First clock transition is the first data capture edge  */
#define LL_SPI_PHASE_2EDGE                 (SPI_CTRL_CPHA)                /*!< Second clock transition is the first data capture edge */
/**
 * @}
 */

/** @defgroup SPI_LL_EC_POLARITY Clock Polarity
 * @{
 */
#define LL_SPI_POLARITY_LOW                0x00000000U                   /*!< Clock to 0 when idle */
#define LL_SPI_POLARITY_HIGH               (SPI_CTRL_CPOL)               /*!< Clock to 1 when idle */
/**
 * @}
 */

/** @defgroup SPI_LL_EC_BIT_ORDER Transmission Bit Order
 * @{
 */
#define LL_SPI_LSB_FIRST                   (SPI_CTRL_LMSB)               /*!< Data is transmitted/received with the LSB first */
#define LL_SPI_MSB_FIRST                   0x00000000U                   /*!< Data is transmitted/received with the MSB first */
/**
 * @}
 */

/** @defgroup SPI_LL_EC_DATAWIDTH Datawidth
 * @{
 */
#define LL_SPI_DATAWIDTH_8BIT              (SPI_CTRL_WIDTH_8BIT)            /*!< Data length for SPI transfer:  8 bits */
#define LL_SPI_DATAWIDTH_16BIT             (SPI_CTRL_WIDTH_16BIT)           /*!< Data length for SPI transfer:  16 bits */
#define LL_SPI_DATAWIDTH_32BIT             (SPI_CTRL_WIDTH_32BIT)           /*!< Data length for SPI transfer:  32 bits */
/**
 * @}
 */

/** @defgroup SPI_LL_EC_DMA_DELAY DMA Delay Time
 * @{
 */
#define LL_SPI_DMADELAY_1CYCLE             (SPI_CTRL_DLY_CYC_SEL_1CYCLE)              /*!< Insert 1 SPI_CLK delay per 8-bit data */
#define LL_SPI_DMADELAY_2CYCLE             (SPI_CTRL_DLY_CYC_SEL_2CYCLE)              /*!< Insert 2 SPI_CLK delays per 8-bit data */
#define LL_SPI_DMADELAY_4CYCLE             (SPI_CTRL_DLY_CYC_SEL_4CYCLE)              /*!< Insert 4 SPI_CLK delays per 8-bit data */
#define LL_SPI_DMADELAY_8CYCLE             (SPI_CTRL_DLY_CYC_SEL_8CYCLE)              /*!< Insert 8 SPI_CLK delays per 8-bit data */
/**
 * @}
 */

/** @defgroup SPI_LL_EC_RX_SAMPLE_DELAY RX Delay Time
 * @{
 */
#define LL_SPI_RXSAMPLDELAY_1CYCLE             (SPI_DLYSAMP_DLYSMP_SEL_1CYCLE)              /*!< Delay sampling by 1 sys_clk cycle */
#define LL_SPI_RXSAMPLDELAY_2CYCLE             (SPI_DLYSAMP_DLYSMP_SEL_2CYCLE)              /*!< Delay sampling by 2 sys_clk cycles */
#define LL_SPI_RXSAMPLDELAY_3CYCLE             (SPI_DLYSAMP_DLYSMP_SEL_3CYCLE)              /*!< Delay sampling by 3 sys_clk cycles */
#define LL_SPI_RXSAMPLDELAY_4CYCLE             (SPI_DLYSAMP_DLYSMP_SEL_4CYCLE)              /*!< Delay sampling by 4 sys_clk cycles */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup SPI_LL_Exported_Macros SPI Exported Macros
 * @{
 */
/** @defgroup SPI_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in SPI register
 * @param  __INSTANCE__ SPI Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_SPI_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in SPI register
 * @param  __INSTANCE__ SPI Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_SPI_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)

/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup SPI_LL_Exported_Functions SPI Exported Functions
 * @{
 */

/** @defgroup SPI_LL_EF_Configuration Configuration functions
 * @{
 */

/**
 * @brief  Enable SPI peripheral
 * @rmtoll CTRL          EN           LL_SPI_Enable
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_Enable(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->CTRL, SPI_CTRL_EN);
}

/**
 * @brief  Disable SPI peripheral
 * @rmtoll CTRL          EN           LL_SPI_Disable
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_Disable(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->CTRL, SPI_CTRL_EN);
}

/**
 * @brief  Check if SPI peripheral is enabled
 * @rmtoll CTRL          EN           LL_SPI_IsEnabled
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsEnabled(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->CTRL, SPI_CTRL_EN) == (SPI_CTRL_EN));
}

/**
 * @brief  Set SPI operation mode to Master or Slave
 * @note   This bit should not be changed when communication is ongoing.
 * @rmtoll CTRL          MAST_SLAV          LL_SPI_SetMode
 * @param  SPIx SPI Instance
 * @param  Mode This parameter can be one of the following values:
 *         @arg @ref LL_SPI_MODE_MASTER
 *         @arg @ref LL_SPI_MODE_SLAVE
 * @retval None
 */
__STATIC_INLINE void LL_SPI_SetMode(SPI_TypeDef *SPIx, uint32_t Mode)
{
    MODIFY_REG(SPIx->CTRL, SPI_CTRL_MAST_SLAV, Mode);
}

/**
 * @brief  Get SPI operation mode (Master or Slave)
 * @rmtoll CTRL          MAST_SLAV          LL_SPI_GetMode
 * @param  SPIx SPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SPI_MODE_MASTER
 *         @arg @ref LL_SPI_MODE_SLAVE
 */
__STATIC_INLINE uint32_t LL_SPI_GetMode(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_BIT(SPIx->CTRL, SPI_CTRL_MAST_SLAV));
}

/**
 * @brief  Set transfer bit order
 * @note   This bit should not be changed when communication is ongoing.
 * @rmtoll CTRL          LMSB      LL_SPI_SetTransferBitOrder
 * @param  SPIx SPI Instance
 * @param  BitOrder This parameter can be one of the following values:
 *         @arg @ref LL_SPI_LSB_FIRST
 *         @arg @ref LL_SPI_MSB_FIRST
 * @retval None
 */
__STATIC_INLINE void LL_SPI_SetTransferBitOrder(SPI_TypeDef *SPIx, uint32_t BitOrder)
{
    MODIFY_REG(SPIx->CTRL, SPI_CTRL_LMSB, BitOrder);
}

/**
 * @brief  Get transfer bit order
 * @rmtoll CTRL          LMSB      LL_SPI_GetTransferBitOrder
 * @param  SPIx SPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SPI_LSB_FIRST
 *         @arg @ref LL_SPI_MSB_FIRST
 */
__STATIC_INLINE uint32_t LL_SPI_GetTransferBitOrder(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_BIT(SPIx->CTRL, SPI_CTRL_LMSB));
}

/**
 * @brief  Set clock polarity
 * @note   This bit should not be changed when communication is ongoing.
 * @rmtoll CTRL          CPOL          LL_SPI_SetClockPolarity
 * @param  SPIx SPI Instance
 * @param  ClockPolarity This parameter can be one of the following values:
 *         @arg @ref LL_SPI_POLARITY_LOW
 *         @arg @ref LL_SPI_POLARITY_HIGH
 * @retval None
 */
__STATIC_INLINE void LL_SPI_SetClockPolarity(SPI_TypeDef *SPIx, uint32_t ClockPolarity)
{
    MODIFY_REG(SPIx->CTRL, SPI_CTRL_CPOL, ClockPolarity);
}

/**
 * @brief  Get clock polarity
 * @rmtoll CTRL          CPOL          LL_SPI_GetClockPolarity
 * @param  SPIx SPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SPI_POLARITY_LOW
 *         @arg @ref LL_SPI_POLARITY_HIGH
 */
__STATIC_INLINE uint32_t LL_SPI_GetClockPolarity(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_BIT(SPIx->CTRL, SPI_CTRL_CPOL));
}

/**
 * @brief  Set clock phase
 * @note   This bit should not be changed when communication is ongoing.
 * @rmtoll CTRL          CPHA          LL_SPI_SetClockPhase
 * @param  SPIx SPI Instance
 * @param  ClockPhase This parameter can be one of the following values:
 *         @arg @ref LL_SPI_PHASE_1EDGE
 *         @arg @ref LL_SPI_PHASE_2EDGE
 * @retval None
 */
__STATIC_INLINE void LL_SPI_SetClockPhase(SPI_TypeDef *SPIx, uint32_t ClockPhase)
{
    MODIFY_REG(SPIx->CTRL, SPI_CTRL_CPHA, ClockPhase);
}

/**
 * @brief  Get clock phase
 * @rmtoll CTRL          CPHA          LL_SPI_GetClockPhase
 * @param  SPIx SPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SPI_PHASE_1EDGE
 *         @arg @ref LL_SPI_PHASE_2EDGE
 */
__STATIC_INLINE uint32_t LL_SPI_GetClockPhase(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_BIT(SPIx->CTRL, SPI_CTRL_CPHA));
}

/**
 * @brief  SCSN Mode Error Detection Enable
 * @rmtoll CTRL          SCSN_EN            LL_SPI_EnableSCSNErrorDetection
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_EnableSCSNErrorDetection(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->CTRL, SPI_CTRL_SCSN_EN);
}

/**
 * @brief  SCSN Mode Error Detection Disable
 * @rmtoll CTRL          SCSN_EN            LL_SPI_DisableSCSNErrorDetection
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DisableSCSNErrorDetection(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->CTRL, SPI_CTRL_SCSN_EN);
}

/**
 * @brief  Indicate if SCSN Mode Error Detection is enabled
 * @rmtoll CTRL          SCSN_EN         LL_SPI_IsEnabledSCSNErrorDetection
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsEnabledSCSNErrorDetection(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->CTRL, SPI_CTRL_SCSN_EN) == (SPI_CTRL_SCSN_EN));
}

/**
 * @brief  Set frame data width
 * @rmtoll CTRL          WIDTH           LL_SPI_SetDataWidth
 * @param  SPIx SPI Instance
 * @param  DataWidth This parameter can be one of the following values:
 *         @arg @ref LL_SPI_DATAWIDTH_8BIT
 *         @arg @ref LL_SPI_DATAWIDTH_16BIT
 *         @arg @ref LL_SPI_DATAWIDTH_32BIT
 * @retval None
 */
__STATIC_INLINE void LL_SPI_SetDataWidth(SPI_TypeDef *SPIx, uint32_t DataWidth)
{
    MODIFY_REG(SPIx->CTRL, SPI_CTRL_WIDTH, DataWidth);
}

/**
 * @brief  Get frame data width
 * @rmtoll CTRL          WIDTH           LL_SPI_GetDataWidth
 * @param  SPIx SPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SPI_DATAWIDTH_8BIT
 *         @arg @ref LL_SPI_DATAWIDTH_16BIT
 *         @arg @ref LL_SPI_DATAWIDTH_32BIT
 */
__STATIC_INLINE uint32_t LL_SPI_GetDataWidth(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_BIT(SPIx->CTRL, SPI_CTRL_WIDTH));
}

/**
 * @brief  Set baud rate prescaler
 * @note   These bits should not be changed when communication is ongoing. SPI BaudRate = fPCLK/(2*(Prescaler+1)).
 *         SCK = APBClock/(2*(CLKDIV + 1)).
 * @rmtoll CTRL          CLKDIV            LL_SPI_SetBaudRatePrescaler
 * @param  SPIx SPI Instance
 * @param  ClockDivision This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
 * @retval None
 */
__STATIC_INLINE void LL_SPI_SetBaudRatePrescaler(SPI_TypeDef *SPIx, uint32_t ClockDivision)
{
    MODIFY_REG(SPIx->CTRL, SPI_CTRL_CLKDIV, ClockDivision << SPI_CTRL_CLKDIV_POS);
}

/**
 * @brief  Get baud rate prescaler
 * @rmtoll CTRL          CLKDIV            LL_SPI_GetBaudRatePrescaler
 * @param  SPIx SPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0xFF
 */
__STATIC_INLINE uint32_t LL_SPI_GetBaudRatePrescaler(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_BIT(SPIx->CTRL, SPI_CTRL_CLKDIV) >> SPI_CTRL_CLKDIV_POS);
}

/**
 * @brief  TX Default Data Send Enable
 * @rmtoll CTRL          TX_DFLT_EN            LL_SPI_EnableTxDefaultData
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_EnableTxDefaultData(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->CTRL, SPI_CTRL_TX_DFLT_EN);
}

/**
 * @brief  TX Default Data Send Disable
 * @rmtoll CTRL          TX_DFLT_EN            LL_SPI_DisableTxDefaultData
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DisableTxDefaultData(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->CTRL, SPI_CTRL_TX_DFLT_EN);
}

/**
 * @brief  Indicate if TX Default Data Send is enabled
 * @rmtoll CTRL          TX_DFLT_EN            LL_SPI_IsEnabledTxDefaultData
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsEnabledTxDefaultData(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->CTRL, SPI_CTRL_TX_DFLT_EN) == (SPI_CTRL_TX_DFLT_EN));
}

/**
 * @brief  Slave High-Speed Mode Enable
 * @rmtoll CTRL          SLV_TX_ADV            LL_SPI_EnableSlaveHighSpeedMode
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_EnableSlaveHighSpeedMode(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->CTRL, SPI_CTRL_SLV_TX_ADV);
}

/**
 * @brief  Slave High-Speed Mode Disable
 * @rmtoll CTRL          SLV_TX_ADV            LL_SPI_DisableSlaveHighSpeedMode
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DisableSlaveHighSpeedMode(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->CTRL, SPI_CTRL_SLV_TX_ADV);
}

/**
 * @brief  Indicate if Slave High-Speed Mode is enabled
 * @rmtoll CTRL          SLV_TX_ADV            LL_SPI_IsEnabledSlaveHighSpeedMode
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsEnabledSlaveHighSpeedMode(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->CTRL, SPI_CTRL_SLV_TX_ADV) == (SPI_CTRL_SLV_TX_ADV));
}

/**
 * @brief  Slave High-Speed Mode Enable
 * @rmtoll DLYSAMP          DLYSMP_EN            LL_SPI_EnableMasterRxSampleDelay
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_EnableMasterRxSampleDelay(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->DLYSAMP, SPI_DLYSAMP_DLYSMP_EN);
}

/**
 * @brief  Slave High-Speed Mode Disable
 * @rmtoll DLYSAMP          DLYSMP_EN            LL_SPI_DisableMasterRxSampleDelay
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DisableMasterRxSampleDelay(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->DLYSAMP, SPI_DLYSAMP_DLYSMP_EN);
}

/**
 * @brief  Indicate if Slave High-Speed Mode is enabled
 * @rmtoll DLYSAMP          DLYSMP_EN            LL_SPI_IsEnabledMasterRxSampleDelay
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsEnabledMasterRxSampleDelay(SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DLYSAMP, SPI_DLYSAMP_DLYSMP_EN) == (SPI_DLYSAMP_DLYSMP_EN));
}

/**
 * @brief  Configure Master Mode SPI Receive Data Sample Delay
 * @rmtoll DLYSAMP          DLYSMP_SEL            LL_SPI_SetMasterRxSampleDelay
 * @param  SPIx SPI Instance
 * @param  DelayTime This parameter can be one of the following values:
 *         @arg @ref LL_SPI_RXSAMPLDELAY_1CYCLE
 *         @arg @ref LL_SPI_RXSAMPLDELAY_2CYCLE
 *         @arg @ref LL_SPI_RXSAMPLDELAY_3CYCLE
 *         @arg @ref LL_SPI_RXSAMPLDELAY_4CYCLE
 * @retval None
 */
__STATIC_INLINE void LL_SPI_SetMasterRxSampleDelay(SPI_TypeDef *SPIx, uint32_t DelayTime)
{
    MODIFY_REG(SPIx->DLYSAMP, SPI_DLYSAMP_DLYSMP_SEL, DelayTime);
}

/**
 * @brief  Return Master Mode SPI Receive Data Sample Delay configuration
 * @rmtoll DLYSAMP          DLYSMP_SEL            LL_SPI_GetMasterRxSampleDelay
 * @param  SPIx SPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SPI_RXSAMPLDELAY_1CYCLE
 *         @arg @ref LL_SPI_RXSAMPLDELAY_2CYCLE
 *         @arg @ref LL_SPI_RXSAMPLDELAY_3CYCLE
 *         @arg @ref LL_SPI_RXSAMPLDELAY_4CYCLE
 */
__STATIC_INLINE uint32_t LL_SPI_GetMasterRxSampleDelay(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_BIT(SPIx->DLYSAMP, SPI_DLYSAMP_DLYSMP_SEL));
}

/**
 * @}
 */

/** @defgroup SPI_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable Data Transmit Interrupt
 * @rmtoll CTRL          TR_IRQ_EN        LL_SPI_EnableIT_TX
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_EnableIT_TX(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->CTRL, SPI_CTRL_TR_IRQ_EN);
}

/**
 * @brief  Disable Data Transmit Interrupt
 * @rmtoll CTRL          TR_IRQ_EN        LL_SPI_DisableIT_TX
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DisableIT_TX(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->CTRL, SPI_CTRL_TR_IRQ_EN);
}

/**
 * @brief  Check if the SPI Data Transmit Interrupt is enabled or disabled.
 * @rmtoll CTRL          TR_IRQ_EN        LL_SPI_IsEnabledIT_TX
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsEnabledIT_TX(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->CTRL, SPI_CTRL_TR_IRQ_EN) == (SPI_CTRL_TR_IRQ_EN));
}

/**
 * @brief  Enable SCSN Mode Error Interrupt
 * @rmtoll CTRL          ERR_IRQ_EN        LL_SPI_EnableIT_SCSNERR
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_EnableIT_SCSNERR(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->CTRL, SPI_CTRL_ERR_IRQ_EN);
}

/**
 * @brief  Disable SCSN Mode Error Interrupt
 * @rmtoll CTRL          ERR_IRQ_EN        LL_SPI_DisableIT_SCSNERR
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DisableIT_SCSNERR(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->CTRL, SPI_CTRL_ERR_IRQ_EN);
}

/**
 * @brief  Check if the SPI SCSN Mode Error Interrupt is enabled or disabled.
 * @rmtoll CTRL          ERR_IRQ_EN        LL_SPI_IsEnabledIT_SCSNERR
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsEnabledIT_SCSNERR(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->CTRL, SPI_CTRL_ERR_IRQ_EN) == (SPI_CTRL_ERR_IRQ_EN));
}

/**
 * @brief  Enable RX Data Conflict Interrupt
 * @rmtoll CTRL          RXCOL_IRQ_EN        LL_SPI_EnableIT_RXCOL
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_EnableIT_RXCOL(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->CTRL, SPI_CTRL_RXCOL_IRQ_EN);
}

/**
 * @brief  Disable RX Data Conflict Interrupt
 * @rmtoll CTRL          RXCOL_IRQ_EN        LL_SPI_DisableIT_RXCOL
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DisableIT_RXCOL(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->CTRL, SPI_CTRL_RXCOL_IRQ_EN);
}

/**
 * @brief  Check if the SPI RX Data Conflict Interrupt is enabled or disabled.
 * @rmtoll CTRL          RXCOL_IRQ_EN        LL_SPI_IsEnabledIT_RXCOL
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsEnabledIT_RXCOL(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->CTRL, SPI_CTRL_RXCOL_IRQ_EN) == (SPI_CTRL_RXCOL_IRQ_EN));
}

/**
 * @brief  Enable TX Data Conflict Interrupt
 * @rmtoll CTRL          TXCOL_IRQ_EN        LL_SPI_EnableIT_TXCOL
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_EnableIT_TXCOL(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->CTRL, SPI_CTRL_TXCOL_IRQ_EN);
}

/**
 * @brief  Disable TX Data Conflict Interrupt
 * @rmtoll CTRL          TXCOL_IRQ_EN        LL_SPI_DisableIT_TXCOL
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DisableIT_TXCOL(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->CTRL, SPI_CTRL_TXCOL_IRQ_EN);
}

/**
 * @brief  Check if the SPI TX Data Conflict Interrupt is enabled or disabled.
 * @rmtoll CTRL          TXCOL_IRQ_EN        LL_SPI_IsEnabledIT_TXCOL
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsEnabledIT_TXCOL(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->CTRL, SPI_CTRL_TXCOL_IRQ_EN) == (SPI_CTRL_TXCOL_IRQ_EN));
}

/**
 * @brief  Enable TX Empty Interrupt
 * @rmtoll CTRL          TXEMPT_IRQEN        LL_SPI_EnableIT_TXEMPT
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_EnableIT_TXEMPT(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->CTRL, SPI_CTRL_TXEMPT_IRQEN);
}

/**
 * @brief  Disable TX Empty Interrupt
 * @rmtoll CTRL          TXEMPT_IRQEN        LL_SPI_DisableIT_TXEMPT
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DisableIT_TXEMPT(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->CTRL, SPI_CTRL_TXEMPT_IRQEN);
}

/**
 * @brief  Check if the SPI TX Empty Interrupt is enabled or disabled.
 * @rmtoll CTRL          TXEMPT_IRQEN        LL_SPI_IsEnabledIT_TXEMPT
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsEnabledIT_TXEMPT(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->CTRL, SPI_CTRL_TXEMPT_IRQEN) == (SPI_CTRL_TXEMPT_IRQEN));
}

/**
 * @brief  Enable SCSN Falling Edge Interrupt
 * @rmtoll CTRL          SCSN_NEG_IRQEN        LL_SPI_EnableIT_SCSNNEG
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_EnableIT_SCSNNEG(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->CTRL, SPI_CTRL_SCSN_NEG_IRQEN);
}

/**
 * @brief  Disable SCSN Falling Edge Interrupt
 * @rmtoll CTRL          SCSN_NEG_IRQEN        LL_SPI_DisableIT_SCSNNEG
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DisableIT_SCSNNEG(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->CTRL, SPI_CTRL_SCSN_NEG_IRQEN);
}

/**
 * @brief  Check if the SPI SCSN Falling Edge Interrupt is enabled or disabled.
 * @rmtoll CTRL          SCSN_NEG_IRQEN        LL_SPI_IsEnabledIT_SCSNNEG
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsEnabledIT_SCSNNEG(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->CTRL, SPI_CTRL_SCSN_NEG_IRQEN) == (SPI_CTRL_SCSN_NEG_IRQEN));
}

/**
 * @brief  Enable SCSN Rising Edge Interrupt
 * @rmtoll CTRL          SCSN_POS_IRQEN        LL_SPI_EnableIT_SCSNPOS
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_EnableIT_SCSNPOS(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->CTRL, SPI_CTRL_SCSN_POS_IRQEN);
}

/**
 * @brief  Disable SCSN Rising Edge Interrupt
 * @rmtoll CTRL          SCSN_POS_IRQEN        LL_SPI_DisableIT_SCSNPOS
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DisableIT_SCSNPOS(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->CTRL, SPI_CTRL_SCSN_POS_IRQEN);
}

/**
 * @brief  Check if the SPI SCSN Rising Edge Interrupt is enabled or disabled.
 * @rmtoll CTRL          SCSN_POS_IRQEN        LL_SPI_IsEnabledIT_SCSNPOS
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsEnabledIT_SCSNPOS(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->CTRL, SPI_CTRL_SCSN_POS_IRQEN) == (SPI_CTRL_SCSN_POS_IRQEN));
}

/**
 * @}
 */

/** @defgroup SPI_LL_EF_FLAG_management FLAG_management
 * @{
 */

/**
 * @brief  Check if the SPI Transmit Complete Flag is set or not
 * @rmtoll STA           TRIF            LL_SPI_IsActiveFlag_TR
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsActiveFlag_TR(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->STA, SPI_STA_TRIF) == (SPI_STA_TRIF));
}

/**
 * @brief  Clear Transmit Complete Flag
 * @rmtoll STA           TRIF            LL_SPI_ClearFlag_TR
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_ClearFlag_TR(SPI_TypeDef *SPIx)
{
    WRITE_REG(SPIx->STA, SPI_STA_TRIF);
}

/**
 * @brief  Check if the SPI SCSN Mode Error Flag is set or not
 * @rmtoll STA           ERRIF            LL_SPI_IsActiveFlag_ERR
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsActiveFlag_ERR(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->STA, SPI_STA_ERRIF) == (SPI_STA_ERRIF));
}

/**
 * @brief  Clear SCSN Mode Error Flag
 * @rmtoll STA           ERRIF            LL_SPI_ClearFlag_ERR
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_ClearFlag_ERR(SPI_TypeDef *SPIx)
{
    WRITE_REG(SPIx->STA, SPI_STA_ERRIF);
}

/**
 * @brief  Check if the SPI RX Overflow Flag is set or not
 * @rmtoll STA           RXCOLIF            LL_SPI_IsActiveFlag_RXCOL
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsActiveFlag_RXCOL(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->STA, SPI_STA_RXCOLIF) == (SPI_STA_RXCOLIF));
}

/**
 * @brief  Clear RX Overflow Flag
 * @rmtoll STA           RXCOLIF            LL_SPI_ClearFlag_RXCOL
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_ClearFlag_RXCOL(SPI_TypeDef *SPIx)
{
    WRITE_REG(SPIx->STA, SPI_STA_RXCOLIF);
}

/**
 * @brief  Check if the SPI TX Conflict Flag is set or not
 * @rmtoll STA           TXCOLIF            LL_SPI_IsActiveFlag_TXCOL
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsActiveFlag_TXCOL(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->STA, SPI_STA_TXCOLIF) == (SPI_STA_TXCOLIF));
}

/**
 * @brief  Clear TX Conflict Flag
 * @rmtoll STA           TXCOLIF            LL_SPI_ClearFlag_TXCOL
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_ClearFlag_TXCOL(SPI_TypeDef *SPIx)
{
    WRITE_REG(SPIx->STA, SPI_STA_TXCOLIF);
}

/**
 * @brief  Check if the SPI TX Buffer Not Empty flag is set or not (read only)
 * @rmtoll STA           TXBUF_VLD            LL_SPI_IsActiveFlag_TXBUFVLD
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsActiveFlag_TXBUFVLD(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->STA, SPI_STA_TXBUF_VLD) == (SPI_STA_TXBUF_VLD));
}

/**
 * @brief  Check if the SPI TX Empty Conflict Flag is set or not
 * @rmtoll STA           TXEMPT            LL_SPI_IsActiveFlag_TXEMPT
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsActiveFlag_TXEMPT(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->STA, SPI_STA_TXEMPT) == (SPI_STA_TXEMPT));
}

/**
 * @brief  Clear TX Empty Conflict Flag
 * @rmtoll STA           TXEMPT            LL_SPI_ClearFlag_TXEMPT
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_ClearFlag_TXEMPT(SPI_TypeDef *SPIx)
{
    WRITE_REG(SPIx->STA, SPI_STA_TXEMPT);
}

/**
 * @brief  Check if the SPI Slave mode CSN Falling Event Flag is set or not
 * @rmtoll STA           SCSN_NEG            LL_SPI_IsActiveFlag_SCSNNEG
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsActiveFlag_SCSNNEG(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->STA, SPI_STA_SCSN_NEG) == (SPI_STA_SCSN_NEG));
}

/**
 * @brief  Clear Slave mode CSN Falling Event Flag
 * @rmtoll STA           SCSN_NEG            LL_SPI_ClearFlag_SCSNNEG
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_ClearFlag_SCSNNEG(SPI_TypeDef *SPIx)
{
    WRITE_REG(SPIx->STA, SPI_STA_SCSN_NEG);
}

/**
 * @brief  Check if the SPI Slave mode CSN Rising Event Flag is set or not
 * @rmtoll STA           SCSN_POS            LL_SPI_IsActiveFlag_SCSNPOS
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsActiveFlag_SCSNPOS(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->STA, SPI_STA_SCSN_POS) == (SPI_STA_SCSN_POS));
}

/**
 * @brief  Clear Slave mode CSN Rising Event Flag
 * @rmtoll STA           SCSN_POS            LL_SPI_ClearFlag_SCSNPOS
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_ClearFlag_SCSNPOS(SPI_TypeDef *SPIx)
{
    WRITE_REG(SPIx->STA, SPI_STA_SCSN_POS);
}

/**
 * @}
 */

/** @defgroup SPI_LL_EF_DMA_Management DMA Management
 * @{
 */

/**
 * @brief  Delay (DMA Master Mode) Enable
 * @rmtoll CTRL          DLY_EN            LL_SPI_DMA_EnableMasterDelayTime
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_EnableMasterDelayTime(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->CTRL, SPI_CTRL_DLY_EN);
}

/**
 * @brief  Delay (DMA Master Mode) Disable
 * @rmtoll CTRL          DLY_EN            LL_SPI_DMA_DisableMasterDelayTime
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_DisableMasterDelayTime(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->CTRL, SPI_CTRL_DLY_EN);
}

/**
 * @brief  Indicate if Delay (DMA Master Mode) is enabled
 * @rmtoll CTRL          DLY_EN            LL_SPI_DMA_IsEnabledMasterDelayTime
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsEnabledMasterDelayTime(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->CTRL, SPI_CTRL_DLY_EN) == (SPI_CTRL_DLY_EN));
}

/**
 * @brief  Configure DMA Data delay Time
 * @note   Data delay selection (valid only in DMA master mode when delay is enabled)
 * @rmtoll CTRL          DLY_CYC_SEL            LL_SPI_DMA_SetMasterDelayTime
 * @param  SPIx SPI Instance
 * @param  DelayTime This parameter can be one of the following values:
 *         @arg @ref LL_SPI_DMADELAY_1CYCLE
 *         @arg @ref LL_SPI_DMADELAY_2CYCLE
 *         @arg @ref LL_SPI_DMADELAY_4CYCLE
 *         @arg @ref LL_SPI_DMADELAY_8CYCLE
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_SetMasterDelayTime(SPI_TypeDef *SPIx, uint32_t DelayTime)
{
    MODIFY_REG(SPIx->CTRL, SPI_CTRL_DLY_CYC_SEL, DelayTime);
}

/**
 * @brief  Return DMA Data delay Time
 * @rmtoll CTRL          DLY_CYC_SEL            LL_SPI_DMA_GetMasterDelayTime
 * @param  SPIx SPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_SPI_DMADELAY_1CYCLE
 *         @arg @ref LL_SPI_DMADELAY_2CYCLE
 *         @arg @ref LL_SPI_DMADELAY_4CYCLE
 *         @arg @ref LL_SPI_DMADELAY_8CYCLE
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_GetMasterDelayTime(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_BIT(SPIx->CTRL, SPI_CTRL_DLY_CYC_SEL));
}

/**
 * @brief  Enable DMA Mode for transmission
 * @rmtoll DMA_CTRL          TX_DMA_EN          LL_SPI_EnableDMAReq_TX
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_EnableDMAReq_TX(SPI_TypeDef *SPIx)
{
  ATOMIC_SET_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_TX_DMA_EN);
}

/**
 * @brief  Disable DMA Mode for transmission
 * @rmtoll DMA_CTRL          TX_DMA_EN          LL_SPI_DisableDMAReq_TX
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DisableDMAReq_TX(SPI_TypeDef *SPIx)
{
  ATOMIC_CLEAR_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_TX_DMA_EN);
}

/**
 * @brief  Check if DMA Mode is enabled for transmission
 * @rmtoll DMA_CTRL          TX_DMA_EN          LL_SPI_IsEnabledDMAReq_TX
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsEnabledDMAReq_TX(const SPI_TypeDef *SPIx)
{
  return (READ_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_TX_DMA_EN) == (SPI_DMA_CTRL_TX_DMA_EN));
}

/**
 * @brief  Enable DMA Mode for reception
 * @rmtoll DMA_CTRL          RX_DMA_EN          LL_SPI_EnableDMAReq_RX
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_EnableDMAReq_RX(SPI_TypeDef *SPIx)
{
  ATOMIC_SET_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_RX_DMA_EN);
}

/**
 * @brief  Disable DMA Mode for reception
 * @rmtoll DMA_CTRL          RX_DMA_EN          LL_SPI_DisableDMAReq_RX
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DisableDMAReq_RX(SPI_TypeDef *SPIx)
{
  ATOMIC_CLEAR_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_RX_DMA_EN);
}

/**
 * @brief  Check if DMA Mode is enabled for reception
 * @rmtoll DMA_CTRL          RX_DMA_EN          LL_SPI_IsEnabledDMAReq_RX
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_IsEnabledDMAReq_RX(const SPI_TypeDef *SPIx)
{
  return (READ_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_RX_DMA_EN) == (SPI_DMA_CTRL_RX_DMA_EN));
}

/**
 * @brief  Enable DMA Transmit Cycle Mode
 * @rmtoll DMA_CTRL          TX_CYC_MODE        LL_SPI_DMA_EnableTxCircularMode
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_EnableTxCircularMode(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_TX_CYC_MODE);
}

/**
 * @brief  Disable DMA Transmit Cycle Mode
 * @rmtoll DMA_CTRL          TX_CYC_MODE        LL_SPI_DMA_DisableTxCircularMode
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_DisableTxCircularMode(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_TX_CYC_MODE);
}

/**
 * @brief  Check if the SPI DMA Transmit Cycle Mode is enabled or disabled.
 * @rmtoll DMA_CTRL          TX_CYC_MODE        LL_SPI_DMA_IsEnabledTxCircularMode
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsEnabledTxCircularMode(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_TX_CYC_MODE) == (SPI_DMA_CTRL_TX_CYC_MODE));
}

/**
 * @brief  Enable DMA Receive Cycle Mode
 * @rmtoll DMA_CTRL          RX_CYC_MODE        LL_SPI_DMA_EnableRxCircularMode
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_EnableRxCircularMode(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_RX_CYC_MODE);
}

/**
 * @brief  Disable DMA Receive Cycle Mode
 * @rmtoll DMA_CTRL          RX_CYC_MODE        LL_SPI_DMA_DisableRxCircularMode
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_DisableRxCircularMode(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_RX_CYC_MODE);
}

/**
 * @brief  Check if the SPI DMA Receive Cycle Mode is enabled or disabled.
 * @rmtoll DMA_CTRL          RX_CYC_MODE        LL_SPI_DMA_IsEnabledRxCircularMode
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsEnabledRxCircularMode(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_RX_CYC_MODE) == (SPI_DMA_CTRL_RX_CYC_MODE));
}

/**
 * @brief  Enable Detection of SCSN High Level to turn off RX_DMA_EN
 * @rmtoll DMA_CTRL          CS_STOP_RXDMA       LL_SPI_EnableDMAReq_TX
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_EnableCSStopRxDMA(SPI_TypeDef *SPIx)
{
  SET_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_CS_STOP_RXDMA);
}

/**
 * @brief  Disable Detection of SCSN High Level to turn off RX_DMA_EN
 * @rmtoll DMA_CTRL          CS_STOP_RXDMA       LL_SPI_DMA_DisableCSStopRxDMA
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_DisableCSStopRxDMA(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_CS_STOP_RXDMA);
}

/**
 * @brief  Check if Detection of SCSN High Level to turn off RX_DMA_EN is enabled
 * @rmtoll DMA_CTRL          CS_STOP_RXDMA       LL_SPI_DMA_IsEnabledCSStopRxDMA
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsEnabledCSStopRxDMA(const SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DMA_CTRL, SPI_DMA_CTRL_CS_STOP_RXDMA) == (SPI_DMA_CTRL_CS_STOP_RXDMA));
}

/**
 * @brief  Configure DMA Transmit Base Address (byte address)
 * @rmtoll DMA_TBADR          DMA_TBADR            LL_SPI_DMA_SetTxStartAddress
 * @param  SPIx SPI Instance
 * @param  TxStartAddress between Min_Data=0x00 and Max_Data=0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_SetTxStartAddress(SPI_TypeDef *SPIx, uint32_t TxStartAddress)
{
    MODIFY_REG(SPIx->DMA_TBADR, SPI_DMA_TBADR_DMA_TBADR, TxStartAddress << SPI_DMA_TBADR_DMA_TBADR_POS);
}

/**
 * @brief  Return DMA Transmit Base Address (byte address) configuration
 * @rmtoll DMA_TBADR          DMA_TBADR            LL_SPI_DMA_GetTxStartAddress
 * @param  SPIx SPI Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_GetTxStartAddress(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_BIT(SPIx->DMA_TBADR, SPI_DMA_TBADR_DMA_TBADR) >> SPI_DMA_TBADR_DMA_TBADR_POS);
}

/**
 * @brief  Configure DMA Receive Base Address (byte address)
 * @rmtoll DMA_RBADR          DMA_RBADR            LL_SPI_DMA_SetRxStartAddress
 * @param  SPIx SPI Instance
 * @param  RxStartAddress between Min_Data=0x00 and Max_Data=0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_SetRxStartAddress(SPI_TypeDef *SPIx, uint32_t RxStartAddress)
{
    MODIFY_REG(SPIx->DMA_RBADR, SPI_DMA_RBADR_DMA_RBADR, RxStartAddress << SPI_DMA_RBADR_DMA_RBADR_POS);
}

/**
 * @brief  Return DMA Receive Base Address (byte address) configuration
 * @rmtoll DMA_RBADR          DMA_RBADR            LL_SPI_DMA_GetRxStartAddress
 * @param  SPIx SPI Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_GetRxStartAddress(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_BIT(SPIx->DMA_RBADR, SPI_DMA_RBADR_DMA_RBADR) >> SPI_DMA_RBADR_DMA_RBADR_POS);
}

/**
 * @brief  Configure DMA Transmit Length (in bytes)
 * @rmtoll DMA_TLEN          DMA_TLEN            LL_SPI_DMA_SetTxLength
 * @param  SPIx SPI Instance
 * @param  TxLength between Min_Data=0x00 and Max_Data=0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_SetTxLength(SPI_TypeDef *SPIx, uint32_t TxLength)
{
    MODIFY_REG(SPIx->DMA_TLEN, SPI_DMA_TLEN_DMA_TLEN, TxLength << SPI_DMA_TLEN_DMA_TLEN_POS);
}

/**
 * @brief  Return DMA Transmit Length (in bytes) configuration
 * @rmtoll DMA_TLEN          DMA_TLEN            LL_SPI_DMA_GetTxLength
 * @param  SPIx SPI Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_GetTxLength(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_BIT(SPIx->DMA_TLEN, SPI_DMA_TLEN_DMA_TLEN) >> SPI_DMA_TLEN_DMA_TLEN_POS);
}

/**
 * @brief  Configure DMA Receive Length (in bytes)
 * @rmtoll DMA_RLEN          DMA_RLEN            LL_SPI_DMA_SetRxLength
 * @param  SPIx SPI Instance
 * @param  RxLength between Min_Data=0x00 and Max_Data=0x3FFFF
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_SetRxLength(SPI_TypeDef * SPIx, uint32_t RxLength)
{
    MODIFY_REG(SPIx->DMA_RLEN, SPI_DMA_RLEN_DMA_RLEN, RxLength << SPI_DMA_RLEN_DMA_RLEN_POS);
}

/**
 * @brief  Return DMA Receive Length (in bytes) configuration
 * @rmtoll DMA_RLEN          DMA_RLEN            LL_SPI_DMA_GetRxLength
 * @param  SPIx SPI Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_GetRxLength(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_BIT(SPIx->DMA_RLEN, SPI_DMA_RLEN_DMA_RLEN) >> SPI_DMA_RLEN_DMA_RLEN_POS);
}

/**
 * @brief  Return Current DMA Transmit Address (byte address) configuration (read only)
 * @rmtoll DMA_TADR          DMA_TADR            LL_SPI_DMA_GetTxAddress
 * @param  SPIx SPI Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_GetTxAddress(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_BIT(SPIx->DMA_TADR, SPI_DMA_TADR_DMA_TADR) >> SPI_DMA_TADR_DMA_TADR_POS);
}

/**
 * @brief  Return Current DMA Receive Address (byte address) configuration (read only)
 * @rmtoll DMA_RADR          DMA_RADR            LL_SPI_DMA_GetRxAddress
 * @param  SPIx SPI Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0x3FFFF
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_GetRxAddress(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_BIT(SPIx->DMA_RADR, SPI_DMA_TADR_DMA_TADR) >> SPI_DMA_RADR_DMA_RADR_POS);
}

/**
 * @}
 */

/** @defgroup SPI_LL_EF_DMA_IT_Management DMA_IT_Management
 * @{
 */

/**
 * @brief  Enable DMA Transmit Half Interrupt
 * @rmtoll DMA_IE          TX_HIE        LL_SPI_DMA_EnableIT_TXHALF
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_EnableIT_TXHALF(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->DMA_IE, SPI_DMA_IE_TX_HIE);
}

/**
 * @brief  Disable DMA Transmit Half Interrupt
 * @rmtoll DMA_IE          TX_HIE        LL_SPI_DMA_DisableIT_TXHALF
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_DisableIT_TXHALF(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->DMA_IE, SPI_DMA_IE_TX_HIE);
}

/**
 * @brief  Check if the SPI DMA Transmit Half Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          TX_HIE        LL_SPI_DMA_IsEnabledIT_TXHALF
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsEnabledIT_TXHALF(SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DMA_IE, SPI_DMA_IE_TX_HIE) == (SPI_DMA_IE_TX_HIE));
}

/**
 * @brief  Enable DMA Transmit Full (Complete) Interrupt
 * @rmtoll DMA_IE          TX_FIE        LL_SPI_DMA_EnableIT_TXFULL
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_EnableIT_TXFULL(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->DMA_IE, SPI_DMA_IE_TX_FIE);
}

/**
 * @brief  Disable DMA Transmit Full (Complete) Interrupt
 * @rmtoll DMA_IE          TX_FIE        LL_SPI_DMA_DisableIT_TXFULL
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_DisableIT_TXFULL(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->DMA_IE, SPI_DMA_IE_TX_FIE);
}

/**
 * @brief  Check if the SPI DMA Transmit Full (Complete) Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          TX_FIE        LL_SPI_DMA_IsEnabledIT_TXFULL
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsEnabledIT_TXFULL(SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DMA_IE, SPI_DMA_IE_TX_FIE) == (SPI_DMA_IE_TX_FIE));
}

/**
 * @brief  Enable DMA Receive Half Interrupt
 * @rmtoll DMA_IE          RX_HIE        LL_SPI_DMA_EnableIT_RXHALF
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_EnableIT_RXHALF(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->DMA_IE, SPI_DMA_IE_RX_HIE);
}

/**
 * @brief  Disable DMA Receive Half Interrupt
 * @rmtoll DMA_IE          RX_HIE        LL_SPI_DMA_DisableIT_RXHALF
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_DisableIT_RXHALF(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->DMA_IE, SPI_DMA_IE_RX_HIE);
}

/**
 * @brief  Check if the SPI DMA Receive Half Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          RX_HIE        LL_SPI_DMA_IsEnabledIT_RXHALF
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsEnabledIT_RXHALF(SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DMA_IE, SPI_DMA_IE_RX_HIE) == (SPI_DMA_IE_RX_HIE));
}

/**
 * @brief  Enable DMA Receive Full (Complete) Interrupt
 * @rmtoll DMA_IE          RX_FIE        LL_SPI_DMA_EnableIT_RXFULL
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_EnableIT_RXFULL(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->DMA_IE, SPI_DMA_IE_RX_FIE);
}

/**
 * @brief  Disable DMA Receive Full (Complete) Interrupt
 * @rmtoll DMA_IE          RX_FIE        LL_SPI_DMA_DisableIT_RXFULL
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_DisableIT_RXFULL(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->DMA_IE, SPI_DMA_IE_RX_FIE);
}

/**
 * @brief  Check if the SPI DMA Receive Full (Complete) Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          RX_FIE        LL_SPI_DMA_IsEnabledIT_RXFULL
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsEnabledIT_RXFULL(SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DMA_IE, SPI_DMA_IE_RX_FIE) == (SPI_DMA_IE_RX_FIE));
}

/**
 * @brief  Enable DMA Transmit Data Error Interrupt
 * @rmtoll DMA_IE          TX_ERR_IE        LL_SPI_DMA_EnableIT_TXERR
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_EnableIT_TXERR(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->DMA_IE, SPI_DMA_IE_TX_ERR_IE);
}

/**
 * @brief  Disable DMA Transmit Data Error Interrupt
 * @rmtoll DMA_IE          TX_ERR_IE        LL_SPI_DMA_DisableIT_TXERR
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_DisableIT_TXERR(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->DMA_IE, SPI_DMA_IE_TX_ERR_IE);
}

/**
 * @brief  Check if the SPI DMA Transmit Data Error Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          TX_ERR_IE        LL_SPI_DMA_IsEnabledIT_TXERR
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsEnabledIT_TXERR(SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DMA_IE, SPI_DMA_IE_TX_ERR_IE) == (SPI_DMA_IE_TX_ERR_IE));
}

/**
 * @brief  Enable DMA Receive Data Overwrite Interrupt
 * @rmtoll DMA_IE          RX_ERR_IE        LL_SPI_DMA_EnableIT_RXERR
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_EnableIT_RXERR(SPI_TypeDef *SPIx)
{
    SET_BIT(SPIx->DMA_IE, SPI_DMA_IE_RX_ERR_IE);
}

/**
 * @brief  Disable DMA Receive Data Overwrite Interrupt
 * @rmtoll DMA_IE          RX_ERR_IE        LL_SPI_DMA_DisableIT_RXERR
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_DisableIT_RXERR(SPI_TypeDef *SPIx)
{
    CLEAR_BIT(SPIx->DMA_IE, SPI_DMA_IE_RX_ERR_IE);
}

/**
 * @brief  Check if the SPI DMA Receive Data Overwrite Interrupt is enabled or disabled.
 * @rmtoll DMA_IE          RX_ERR_IE        LL_SPI_DMA_IsEnabledIT_RXERR
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsEnabledIT_RXERR(SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DMA_IE, SPI_DMA_IE_RX_ERR_IE) == (SPI_DMA_IE_RX_ERR_IE));
}

/**
 * @}
 */

/** @defgroup SPI_LL_EF_DMA_FLAG_management DMA_FLAG_management
 * @{
 */

/**
 * @brief  Check if the SPI DMA Transmit Half Interrupt Flag is set or not
 * @rmtoll DMA_FLG           TX_HDONE            LL_SPI_DMA_IsActiveFlag_TXHALF
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsActiveFlag_TXHALF(SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DMA_FLG, SPI_DMA_FLG_TX_HDONE) == (SPI_DMA_FLG_TX_HDONE));
}

/**
 * @brief  Clear DMA Transmit Half Interrupt Flag
 * @rmtoll DMA_FLG           TX_HDONE            LL_SPI_DMA_ClearFlag_TXHALF
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_ClearFlag_TXHALF(SPI_TypeDef *SPIx)
{
    WRITE_REG(SPIx->DMA_FLG, SPI_DMA_FLG_TX_HDONE);
}

/**
 * @brief  Check if the SPI DMA Transmit Full (Complete) Interrupt Flag is set or not
 * @rmtoll DMA_FLG           TX_FDONE            LL_SPI_DMA_IsActiveFlag_TXFULL
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsActiveFlag_TXFULL(SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DMA_FLG, SPI_DMA_FLG_TX_FDONE) == (SPI_DMA_FLG_TX_FDONE));
}

/**
 * @brief  Clear DMA Transmit Full (Complete) Interrupt Flag
 * @rmtoll DMA_FLG           TX_FDONE            LL_SPI_DMA_ClearFlag_TXFULL
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_ClearFlag_TXFULL(SPI_TypeDef *SPIx)
{
    WRITE_REG(SPIx->DMA_FLG, SPI_DMA_FLG_TX_FDONE);
}

/**
 * @brief  Check if the SPI DMA Receive Half Interrupt Flag is set or not
 * @rmtoll DMA_FLG           RX_HDONE            LL_SPI_DMA_IsActiveFlag_RXHALF
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsActiveFlag_RXHALF(SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DMA_FLG, SPI_DMA_FLG_RX_HDONE) == (SPI_DMA_FLG_RX_HDONE));
}

/**
 * @brief  Clear DMA Receive Half Interrupt Flag
 * @rmtoll DMA_FLG           RX_HDONE            LL_SPI_DMA_ClearFlag_RXHALF
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_ClearFlag_RXHALF(SPI_TypeDef *SPIx)
{
    WRITE_REG(SPIx->DMA_FLG, SPI_DMA_FLG_RX_HDONE);
}

/**
 * @brief  Check if the SPI DMA Receive Full (Complete) Interrupt Flag is set or not
 * @rmtoll DMA_FLG           RX_FDONE            LL_SPI_DMA_IsActiveFlag_RXFULL
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsActiveFlag_RXFULL(SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DMA_FLG, SPI_DMA_FLG_RX_FDONE) == (SPI_DMA_FLG_RX_FDONE));
}

/**
 * @brief  Clear DMA Receive Full (Complete) Interrupt Flag
 * @rmtoll DMA_FLG           RX_FDONE            LL_SPI_DMA_ClearFlag_RXFULL
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_ClearFlag_RXFULL(SPI_TypeDef *SPIx)
{
    WRITE_REG(SPIx->DMA_FLG, SPI_DMA_FLG_RX_FDONE);
}

/**
 * @brief  Check if the SPI DMA Receive Data Error Flag is set or not
 * @rmtoll DMA_FLG           RX_ERR            LL_SPI_DMA_IsActiveFlag_RXERR
 * @param  SPIx SPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_SPI_DMA_IsActiveFlag_RXERR(SPI_TypeDef *SPIx)
{
    return (READ_BIT(SPIx->DMA_FLG, SPI_DMA_FLG_RX_ERR) == (SPI_DMA_FLG_RX_ERR));
}

/**
 * @brief  Clear DMA Receive Data Error Flag
 * @rmtoll DMA_FLG           RX_ERR            LL_SPI_DMA_ClearFlag_RXERR
 * @param  SPIx SPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_SPI_DMA_ClearFlag_RXERR(SPI_TypeDef *SPIx)
{
    WRITE_REG(SPIx->DMA_FLG, SPI_DMA_FLG_RX_ERR);
}

/**
 * @}
 */

/** @defgroup SPI_LL_EF_Data_Management Data_Management
 * @{
 */

/**
 * @brief  Read 8-Bits in the data register
 * @rmtoll RXDATA           RXDATA            LL_SPI_ReceiveData8
 * @param  SPIx SPI Instance
 * @retval RxData Value between Min_Data=0x00 and Max_Data=0xFF
 */
__STATIC_INLINE uint8_t LL_SPI_ReceiveData8(const SPI_TypeDef *SPIx)
{
    return (uint8_t)(SPIx->RXDATA & 0xF);
}

/**
 * @brief  Read 16-Bits in the data register
 * @rmtoll RXDATA           RXDATA            LL_SPI_ReceiveData16
 * @param  SPIx SPI Instance
 * @retval RxData Value between Min_Data=0x00 and Max_Data=0xFFFF
 */
__STATIC_INLINE uint16_t LL_SPI_ReceiveData16(const SPI_TypeDef *SPIx)
{
    return (uint16_t)(SPIx->RXDATA & 0xFF);
}

/**
 * @brief  Read 32-Bits in the data register
 * @rmtoll RXDATA           RXDATA            LL_SPI_ReceiveData32
 * @param  SPIx SPI Instance
 * @retval RxData Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_SPI_ReceiveData32(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_REG(SPIx->RXDATA));
}

/**
 * @brief  Write 8-Bits in the data register
 * @rmtoll TXDATA           TXDATA            LL_SPI_TransmitData8
 * @param  SPIx SPI Instance
 * @param  TxData Value between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_SPI_TransmitData8(SPI_TypeDef *SPIx, uint8_t TxData)
{
    SPIx->TXDATA = TxData;
}

/**
 * @brief  Write 16-Bits in the data register
 * @rmtoll TXDATA           TXDATA            LL_SPI_TransmitData16
 * @param  SPIx SPI Instance
 * @param  TxData Value between Min_Data=0x00 and Max_Data=0xFFFF
 * @retval None
 */
__STATIC_INLINE void LL_SPI_TransmitData16(SPI_TypeDef *SPIx, uint16_t TxData)
{
    SPIx->TXDATA = TxData;
}

/**
 * @brief  Write 32-Bits in the data register
 * @rmtoll TXDATA           TXDATA            LL_SPI_TransmitData32
 * @param  SPIx SPI Instance
 * @param  TxData Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_SPI_TransmitData32(SPI_TypeDef *SPIx, uint32_t TxData)
{
    SPIx->TXDATA = TxData;
}

/**
 * @brief  Write in Default Transmit Data Register
 * @rmtoll TXDFLT           TXDFLT            LL_SPI_SetTxDefaultData
 * @param  SPIx SPI Instance
 * @param  TxDefaultData Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_SPI_SetTxDefaultData(SPI_TypeDef *SPIx, uint32_t TxDefaultData)
{
    SPIx->TXDFLT = TxDefaultData;
}

/**
 * @brief  Read Data Default Transmit Data Register
 * @rmtoll TXDFLT           TXDFLT            LL_SPI_GetTxDefaultData
 * @param  SPIx SPI Instance
 * @retval Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_SPI_GetTxDefaultData(const SPI_TypeDef *SPIx)
{
    return (uint32_t)(READ_REG(SPIx->TXDFLT));
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup SPI_LL_EF_Init Initialization and de-initialization functions
 * @{
 */

// The initialization function can have more than one argument.
ErrorStatus LL_SPI_Init(SPI_TypeDef *SPIx, const LL_SPI_InitTypeDef *SPI_InitStruct);
ErrorStatus LL_SPI_DeInit(SPI_TypeDef *SPIx);
void        LL_SPI_StructInit(LL_SPI_InitTypeDef *SPI_InitStruct); // If the module is not suitable for the default configuration, you could not defines this function.

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

#endif /* SPI0 || SPI1 || SPI2 || SPI3 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_SPI_H */
