/**
 *****************************************************************************
 * @file   rn_rv_ll_qspi.h
 * @author Renergy Software Team
 * @brief  Header file of QSPI LL module.
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
#ifndef __RN_LL_QSPI_H
#define __RN_LL_QSPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_qspi.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (QSPI0) || defined (QSPI1)

/** @defgroup QSPI_LL QSPI
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/

/* Private constants --------------------------------------------------------*/
/** @defgroup QSPI_LL_Private_Constants QSPI Private Constants
 * @{
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup QSPI_LL_Private_Macros QSPI Private Macros
 * @{
 */
/*  Dummy Cycle high Position  */
#define DUMMY_CYCLE_H_POS                 (4U)
/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup QSPI_LL_ES_INIT QSPI Exported Init structure
 * @{
 */

/**
 * @brief LL QSPI Init structures definition
 */
typedef struct
{
    uint32_t Mode;                    /*!< Specifies the QSPI mode (Master/Slave).
                                            This parameter can be a value of @ref QSPI_LL_EC_MODE.

                                            This feature can be modified afterwards using unitary
                                            function @ref LL_QSPI_SetMode().*/

    uint32_t DataWidth;               /*!< Specifies the QSPI data width.
                                            This parameter can be a value of @ref QSPI_LL_EC_DATAWIDTH.

                                            This feature can be modified afterwards using unitary
                                            function @ref LL_QSPI_SetDataWidth().*/

    uint32_t ClockPolarity;           /*!< Specifies the serial clock steady state.
                                            This parameter can be a value of @ref QSPI_LL_EC_POLARITY.

                                            This feature can be modified afterwards using unitary
                                            function @ref LL_QSPI_SetClockPolarity().*/

    uint32_t ClockPhase;              /*!< Specifies the clock active edge for the bit capture.
                                            This parameter can be a value of @ref QSPI_LL_EC_PHASE.

                                            This feature can be modified afterwards using unitary
                                            function @ref LL_QSPI_SetClockPhase().*/

    uint32_t ClockDivision;           /*!< Specifies the BaudRate prescaler value which will be used
                                            to configure the transmit and receive SCK clock.
                                            This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFFF.
                                            @note The communication clock is derived from the master clock.
                                                   The slave clock does not need to be set.

                                            This feature can be modified afterwards using unitary
                                            function @ref LL_QSPI_SetBaudRatePrescaler().*/

    uint32_t BitOrder;                /*!< Specifies whether bit data transfers start from MSB or LSB bit.
                                            This parameter can be a value of @ref QSPI_LL_EC_BIT_ORDER.

                                            This feature can be modified afterwards using unitary
                                            function @ref LL_QSPI_SetTransferBitOrder().*/

    uint32_t ByteOrder;               /*!< Specifies whether Byte data transfers start from MSB or LSB bit.
                                            This parameter can be a value of @ref QSPI_LL_EC_BYTE_ORDER.

                                            This feature can be modified afterwards using unitary
                                            function @ref LL_QSPI_SetTransferByteOrder().*/

} LL_QSPI_InitTypeDef;

/**
 * @}
 */
#endif /*USE_FULL_LL_DRIVER*/

/* Exported constants -------------------------------------------------------*/
/** @defgroup QSPI_LL_Exported_Constants QSPI Exported Constants
 * @{
 */

/** @defgroup QSPI_LL_EC_FORCE_WP Force WP State
 * @{
 */
#define LL_QSPI_FORCEWP_HIGH                 (QSPI_FORCE_FORCE_WP)            /*!< Output high  */
#define LL_QSPI_FORCEWP_LOW                  0x00000000U                      /*!< Output low   */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_PHASE Clock Phase
 * @{
 */
#define LL_QSPI_PHASE_1EDGE                 0x00000000U               /*!< First clock transition is the first data capture edge  */
#define LL_QSPI_PHASE_2EDGE                 (QSPI_SCKMODE_CPHA)       /*!< Second clock transition is the first data capture edge */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_POLARITY Clock Polarity
 * @{
 */
#define LL_QSPI_POLARITY_LOW                0x00000000U               /*!< Clock to 0 when idle */
#define LL_QSPI_POLARITY_HIGH               (QSPI_SCKMODE_CPOL)       /*!< Clock to 1 when idle */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_CSID Chip Select Device ID
 * @{
 */
#define LL_QSPI_CSID_NONE                   0x00000000U               /*!< Disabled   */
#define LL_QSPI_CSID_0                      (QSPI_SCKMODE_CPOL)       /*!< Select CS0 */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_CS_IDLE_STATE Chip Select 0 Idle State
 * @{
 */
#define LL_QSPI_CS0_IDLESTATE_LOW                   0x00000000U               /*!< Disabled   */
#define LL_QSPI_CS0_IDLESTATE_HIGH                  (QSPI_CSDEF_CSDEF)        /*!< Select CS0 */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_CS_MODE CS Mode
 * @{
 */
#define LL_QSPI_CSMODE_AUTO                     (QSPI_CSMODE_CSMODE_AUTO)             /*!< Auto mode    */
#define LL_QSPI_CSMODE_HOLD                     (QSPI_CSMODE_CSMODE_HOLD)             /*!< Hold mode    */
#define LL_QSPI_CSMODE_DISABLE                  (QSPI_CSMODE_CSMODE_DISABLE)          /*!< Disable mode */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_PROTOCOLMODE Protocol Mode
 * @{
 */
#define LL_QSPI_PROTOCOLMODE_SINGLE                (QSPI_FMT_PROTO_SINGLE)                                   /*!< Single-line mode             */
#define LL_QSPI_PROTOCOLMODE_DUAL                  (QSPI_FMT_PROTO_DUAL)                                     /*!< Dual-line mode               */
#define LL_QSPI_PROTOCOLMODE_QUAD                  (QSPI_FMT_PROTO_QUAD)                                     /*!< Quad-line mode               */
#define LL_QSPI_PROTOCOLMODE_HALF_DUPLEX           (QSPI_FMT_PROTO_HALF_DUPLEX | QSPI_FMT_PROTO_HI)          /*!< Single-line Half-duplex mode */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_BIT_ORDER Transmission Bit Order
 * @{
 */
#define LL_QSPI_BIT_LSB_FIRST                   (QSPI_FMT_ENDIAN)           /*!< Bit Data is transmitted/received with the LSB first */
#define LL_QSPI_BIT_MSB_FIRST                   0x00000000U                 /*!< Bit Data is transmitted/received with the MSB first */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_DIRECTION Communication Direction
 * @{
 */
#define LL_QSPI_DIRECTION_RX                   0x00000000U                        /*!< Receiver is enabled    */
#define LL_QSPI_DIRECTION_TX                   (QSPI_FMT_DIR)                     /*!< Transmitter is enabled */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_BYTE_ORDER Transmission Bit Order
 * @{
 */
#define LL_QSPI_BYTE_LSB_FIRST                   0x00000000U                        /*!< Byte transmission order with the LSB first */
#define LL_QSPI_BYTE_MSB_FIRST                   (QSPI_FMT_ENDIAN_BYTE)             /*!< Byte transmission order with the MSB first */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_DATAWIDTH Datawidth
 * @{
 */
#define LL_QSPI_DATAWIDTH_8BIT                  (QSPI_FMT_LEN_8BIT)                      /*!< Data length for QSPI transfer: 8 bits  */
#define LL_QSPI_DATAWIDTH_16BIT                 (QSPI_FMT_LEN_16BIT)                     /*!< Data length for QSPI transfer: 16 bits */
#define LL_QSPI_DATAWIDTH_32BIT                 (QSPI_FMT_LEN_32BIT)                     /*!< Data length for QSPI transfer: 32 bits */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_TXMARK Tx FIFO Watermark
 * @{
 */
#define LL_QSPI_TXMARK_1                 (QSPI_TX_MARK_TX_MARK_1)                     /*!< 1 valid data item  */
#define LL_QSPI_TXMARK_2                 (QSPI_TX_MARK_TX_MARK_2)                     /*!< 2 valid data items */
#define LL_QSPI_TXMARK_3                 (QSPI_TX_MARK_TX_MARK_3)                     /*!< 3 valid data items */
#define LL_QSPI_TXMARK_4                 (QSPI_TX_MARK_TX_MARK_4)                     /*!< 4 valid data items */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_RXMARK Rx FIFO Watermark
 * @{
 */
#define LL_QSPI_RXMARK_0                 (QSPI_RX_MARK_RX_MARK_0)                     /*!< 0 valid data item  */
#define LL_QSPI_RXMARK_1                 (QSPI_RX_MARK_RX_MARK_1)                     /*!< 1 valid data item  */
#define LL_QSPI_RXMARK_2                 (QSPI_RX_MARK_RX_MARK_2)                     /*!< 2 valid data items */
#define LL_QSPI_RXMARK_3                 (QSPI_RX_MARK_RX_MARK_3)                     /*!< 3 valid data items */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_XIP_ADDRESSWIDTH XIP Address Width
 * @{
 */
#define LL_QSPI_XIP_ADDRESSWIDTH_NONE                  (QSPI_FFMT_ADDR_LEN_NONE)                     /*!< Skip Address phase               */
#define LL_QSPI_XIP_ADDRESSWIDTH_8BIT                  (QSPI_FFMT_ADDR_LEN_8BIT)                     /*!< Address Length 8 bits            */
#define LL_QSPI_XIP_ADDRESSWIDTH_16BIT                 (QSPI_FFMT_ADDR_LEN_16BIT)                    /*!< Address Length 16 bits           */
#define LL_QSPI_XIP_ADDRESSWIDTH_24BIT                 (QSPI_FFMT_ADDR_LEN_24BIT)                    /*!< Address Length 24 bits (default) */
#define LL_QSPI_XIP_ADDRESSWIDTH_32BIT                 (QSPI_FFMT_ADDR_LEN_32BIT)                    /*!< Address Length 32 bits           */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_XIP_COMMAND_PROTOCOLMODE XIP Command Protocol Mode
 * @{
 */
#define LL_QSPI_XIP_COMMAND_PROTOCOLMODE_SINGLE                (QSPI_FFMT_CMD_PROTO_SINGL)                                   /*!< XIP Command Single-line mode */
#define LL_QSPI_XIP_COMMAND_PROTOCOLMODE_DUAL                  (QSPI_FFMT_CMD_PROTO_DUAL)                                    /*!< XIP Command Dual-line mode   */
#define LL_QSPI_XIP_COMMAND_PROTOCOLMODE_QUAD                  (QSPI_FFMT_CMD_PROTO_QUAD)                                    /*!< XIP Command Quad-line mode   */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_XIP_ADDRESS_PROTOCOLMODE XIP Address Protocol Mode
 * @{
 */
#define LL_QSPI_XIP_ADDRESS_PROTOCOLMODE_SINGLE                (QSPI_FFMT_ADDR_PROTO_SINGL)                                   /*!< XIP Address Single-line mode */
#define LL_QSPI_XIP_ADDRESS_PROTOCOLMODE_DUAL                  (QSPI_FFMT_ADDR_PROTO_DUAL)                                    /*!< XIP Address Dual-line mode   */
#define LL_QSPI_XIP_ADDRESS_PROTOCOLMODE_QUAD                  (QSPI_FFMT_ADDR_PROTO_QUAD)                                    /*!< XIP Address Quad-line mode   */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_XIP_DATA_PROTOCOLMODE XIP Data Protocol Mode
 * @{
 */
#define LL_QSPI_XIP_DATA_PROTOCOLMODE_SINGLE                   (QSPI_FFMT_DATA_PROTO_SINGLE)                                     /*!< XIP Data Single-line mode             */
#define LL_QSPI_XIP_DATA_PROTOCOLMODE_DUAL                     (QSPI_FFMT_DATA_PROTO_DUAL)                                       /*!< XIP Data Dual-line mode               */
#define LL_QSPI_XIP_DATA_PROTOCOLMODE_QUAD                     (QSPI_FFMT_DATA_PROTO_QUAD)                                       /*!< XIP Data Quad-line mode               */
#define LL_QSPI_XIP_DATA_PROTOCOLMODE_HALF_DUPLEX              (QSPI_FFMT_DATA_PROTO_HALF_DUPLEX | QSPI_FFMT_DATA_PROTO_HI)      /*!< XIP Data Single-line Half-duplex mode */
/**
 * @}
 */


/** @defgroup QSPI_LL_EC_XIP_BIT_ORDER XIP Transmission Bit Order
 * @{
 */
#define LL_QSPI_XIP_BIT_LSB_FIRST                   (QSPI_FFMT_ENDIAN_F)           /*!< XIP Bit Data is transmitted/received with the LSB first */
#define LL_QSPI_XIP_BIT_MSB_FIRST                   0x00000000U                    /*!< XIP Bit Data is transmitted/received with the MSB first */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_XIP_DDRENABLE XIP DDR Enable
 * @{
 */
#define LL_QSPI_XIP_DDRENABLE_COMMAND                   0x00004000U                     /*!< DDR Enable bits for Instruction[14] phase. */
#define LL_QSPI_XIP_DDRENABLE_ADDRESS                   0x00008000U                     /*!< DDR Enable bits for Address[15] phase.     */
#define LL_QSPI_XIP_DDRENABLE_MODE                      0x00010000U                     /*!< DDR Enable bits for Mode[16] phase.        */
#define LL_QSPI_XIP_DDRENABLE_DATA                      0x00020000U                     /*!< DDR Enable bits for Data[17] phase.        */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_XIP_MODE_PROTOCOLMODE XIP Data Protocol Mode
 * @{
 */
#define LL_QSPI_XIP_MODE_PROTOCOLMODE_SINGLE                   (QSPI_FFMT1_MODE_PROTO_SINGLE)                                     /*!< XIP Mode Single-line mode */
#define LL_QSPI_XIP_MODE_PROTOCOLMODE_DUAL                     (QSPI_FFMT1_MODE_PROTO_DUAL)                                       /*!< XIP Mode Dual-line mode   */
#define LL_QSPI_XIP_MODE_PROTOCOLMODE_QUAD                     (QSPI_FFMT1_MODE_PROTO_QUAD)                                       /*!< XIP Mode Quad-line mode   */
/**
 * @}
 */

/** @defgroup QSPI_LL_EC_XIP_MODEWIDTH XIP Mode Width
 * @{
 */
#define LL_QSPI_XIP_MODEWIDTH_NONE                  0x00000000U                              /*!< Skip Mode phase    */
#define LL_QSPI_XIP_MODEWIDTH_8BIT                  (QSPI_FFMT1_MODE_CNT)                    /*!< Mode Length 8 bits */
/**
 * @}
 */

/** @defgroup SPI_LL_EC_MODE Operation Mode
 * @{
 */
#define LL_QSPI_MODE_MASTER                 (QSPI_CR_MSTER)            /*!< Master configuration  */
#define LL_QSPI_MODE_SLAVE                  0x00000000U                /*!< Slave configuration   */
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup QSPI_LL_Exported_Macros QSPI Exported Macros
 * @{
 */
/** @defgroup QSPI_LL_EM_WRITE_READ Common Write and read registers Macros
 * @{
 */

/**
 * @brief  Write a value in QSPI register
 * @param  __INSTANCE__ QSPI Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_QSPI_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in QSPI register
 * @param  __INSTANCE__ QSPI Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_QSPI_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)

/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/

/** @defgroup QSPI_LL_Exported_Functions QSPI Exported Functions
 * @{
 */

/** @defgroup QSPI_LL_EF_Configuration Configuration functions
 * @{
 */

/**
 * @brief  Set baud rate prescaler
 * @note   These bits should not be changed when communication is ongoing. QSPI BaudRate = fPCLK/(2*(Prescaler+1)).
 *         FSCK = FQSPI_CLK / (2*(SCKDIV + 1)).
 * @rmtoll SCKDIV          SCKDIV            LL_QSPI_SetBaudRatePrescaler
 * @param  QSPIx QSPI Instance
 * @param  ClockDivision This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetBaudRatePrescaler(QSPI_TypeDef *QSPIx, uint32_t ClockDivision)
{
    MODIFY_REG(QSPIx->SCKDIV, QSPI_SCKDIV_SCKDIV, ClockDivision << QSPI_SCKDIV_SCKDIV_POS);
}

/**
 * @brief  Get baud rate prescaler
 * @rmtoll SCKDIV          SCKDIV            LL_QSPI_GetBaudRatePrescaler
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0xFFF
 */
__STATIC_INLINE uint32_t LL_QSPI_GetBaudRatePrescaler(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->SCKDIV, QSPI_SCKDIV_SCKDIV) >> QSPI_SCKDIV_SCKDIV_POS);
}

/**
 * @brief  Set clock phase
 * @note   This bit should not be changed when communication is ongoing.
 *         In DDR mode, only CPHA=0 is supported.
 * @rmtoll SCKMODE          CPHA          LL_QSPI_SetClockPhase
 * @param  QSPIx QSPI Instance
 * @param  ClockPhase This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_PHASE_1EDGE
 *         @arg @ref LL_QSPI_PHASE_2EDGE
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetClockPhase(QSPI_TypeDef *QSPIx, uint32_t ClockPhase)
{
    MODIFY_REG(QSPIx->SCKMODE, QSPI_SCKMODE_CPHA, ClockPhase);
}

/**
 * @brief  Get clock phase
 * @rmtoll SCKMODE          CPHA          LL_QSPI_GetClockPhase
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_PHASE_1EDGE
 *         @arg @ref LL_QSPI_PHASE_2EDGE
 */
__STATIC_INLINE uint32_t LL_QSPI_GetClockPhase(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->SCKMODE, QSPI_SCKMODE_CPHA));
}

/**
 * @brief  Set clock polarity
 * @note   This bit should not be changed when communication is ongoing.
 * @rmtoll SCKMODE          CPOL          LL_QSPI_SetClockPolarity
 * @param  QSPIx QSPI Instance
 * @param  ClockPolarity This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_POLARITY_LOW
 *         @arg @ref LL_QSPI_POLARITY_HIGH
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetClockPolarity(QSPI_TypeDef *QSPIx, uint32_t ClockPolarity)
{
    MODIFY_REG(QSPIx->SCKMODE, QSPI_SCKMODE_CPOL, ClockPolarity);
}

/**
 * @brief  Get clock polarity
 * @rmtoll SCKMODE          CPOL          LL_QSPI_GetClockPolarity
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_POLARITY_LOW
 *         @arg @ref LL_QSPI_POLARITY_HIGH
 */
__STATIC_INLINE uint32_t LL_QSPI_GetClockPolarity(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->SCKMODE, QSPI_SCKMODE_CPOL));
}

/**
 * @brief  Set DDR Mode SCK sample point delay
 * @note   Sampling delay must be less than 1 QSPI_SCK period.
 * @rmtoll SCKSAMPLE          SCKSAMPLE          LL_QSPI_SetDDRSampleDelay
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetDDRSampleDelay(QSPI_TypeDef *QSPIx, uint32_t SampleDelay)
{
    MODIFY_REG(QSPIx->SCKSAMPLE, QSPI_SCKSAMPLE_SCKSMP, SampleDelay << QSPI_SCKSAMPLE_SCKSMP_POS);
}

/**
 * @brief  Get DDR Mode SCK sample point delay
 * @rmtoll SCKSAMPLE          SCKSAMPLE          LL_QSPI_GetDDRSampleDelay
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0xFFF
 */
__STATIC_INLINE uint32_t LL_QSPI_GetDDRSampleDelay(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->SCKSAMPLE, QSPI_SCKSAMPLE_SCKSMP) >> QSPI_SCKSAMPLE_SCKSMP_POS);
}

/**
 * @brief  Enable Force output enable for non-data pins in different interface mode.
 * @rmtoll FORCE          FORCE_EN           LL_QSPI_EnableForceOutput
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableForceOutput(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->FORCE, QSPI_FORCE_FORCE_EN);
}

/**
 * @brief  Disable Force output enable for non-data pins in different interface mode.
 * @rmtoll FORCE          FORCE_EN           LL_QSPI_DisableForceOutput
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableForceOutput(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->FORCE, QSPI_FORCE_FORCE_EN);
}

/**
 * @brief  Check if Force output enable for non-data pins in different interface mode is enabled
 * @rmtoll FORCE          FORCE_EN           LL_QSPI_IsEnabledForceOutput
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledForceOutput(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->FORCE, QSPI_FORCE_FORCE_EN) == (QSPI_FORCE_FORCE_EN));
}

/**
 * @brief  Set WP (IO2) pin output state
 * @note   Invalid in Quad-line mode.
 * @rmtoll FORCE          FORCE_WP          LL_QSPI_SetWPState
 * @param  QSPIx QSPI Instance
 * @param  State This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_FORCEWP_HIGH
 *         @arg @ref LL_QSPI_FORCEWP_LOW
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetWPState(QSPI_TypeDef *QSPIx, uint32_t State)
{
    MODIFY_REG(QSPIx->FORCE, QSPI_FORCE_FORCE_WP, State);
}

/**
 * @brief  Get WP (IO2) pin output state
 * @rmtoll FORCE          FORCE_WP          LL_QSPI_GetWPState
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_FORCEWP_HIGH
 *         @arg @ref LL_QSPI_FORCEWP_LOW
 */
__STATIC_INLINE uint32_t LL_QSPI_GetWPState(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FORCE, QSPI_FORCE_FORCE_WP));
}

/**
 * @brief  Get QSPI Version
 * @rmtoll VERSION          VERSION          LL_QSPI_GetVersion
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values: (0x00020100) QSPI Version 2.1.0
 */
__STATIC_INLINE uint32_t LL_QSPI_GetVersion(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_REG(QSPIx->VERSION));
}

/**
 * @brief  Set Delay cycles from CS assertion (falling edge) to first valid SCK rising edge.
 * @note   Units is 1 Bus Clock Cycle.
 *         0x00: No delay.
 * @rmtoll DELAY0          CSSCK            LL_QSPI_SetCSToSCKDelay
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetCSToSCKDelay(QSPI_TypeDef *QSPIx, uint32_t Delay)
{
    MODIFY_REG(QSPIx->DELAY0, QSPI_DELAY0_CSSCK, Delay << QSPI_DELAY0_CSSCK_POS);
}

/**
 * @brief  Get Delay cycles from CS assertion (falling edge) to first valid SCK rising edge.
 * @rmtoll DELAY0          CSSCK            LL_QSPI_GetCSToSCKDelay
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0xFF
 */
__STATIC_INLINE uint32_t LL_QSPI_GetCSToSCKDelay(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->DELAY0, QSPI_DELAY0_CSSCK) >> QSPI_DELAY0_CSSCK_POS);
}

/**
 * @brief  Set Delay cycles from last valid SCK edge to CS deassertion (rising edge).
 * @note   Units is 1 Bus Clock Cycle.
 *         0x00: No delay
 * @rmtoll DELAY0          SCKCS            LL_QSPI_SetSCKToCSDelay
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetSCKToCSDelay(QSPI_TypeDef *QSPIx, uint32_t Delay)
{
    MODIFY_REG(QSPIx->DELAY0, QSPI_DELAY0_SCKCS, Delay << QSPI_DELAY0_SCKCS_POS);
}

/**
 * @brief  Get Delay cycles from last valid SCK edge to CS deassertion (rising edge).
 * @rmtoll DELAY0          SCKCS            LL_QSPI_GetSCKToCSDelay
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0xFF
 */
__STATIC_INLINE uint32_t LL_QSPI_GetSCKToCSDelay(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->DELAY0, QSPI_DELAY0_SCKCS) >> QSPI_DELAY0_SCKCS_POS);
}

/**
 * @brief  Set Minimum delay cycles from CS rising edge to next CS falling edge.
 * @note   Units is 1 Bus Clock Cycle.
 *         0x00: No delay
 * @rmtoll DELAY1          INTERCS            LL_QSPI_SetInterCSDelay
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetInterCSDelay(QSPI_TypeDef *QSPIx, uint32_t Delay)
{
    MODIFY_REG(QSPIx->DELAY0, QSPI_DELAY1_INTERCS, Delay << QSPI_DELAY1_INTERCS_POS);
}

/**
 * @brief  Get Minimum delay cycles from CS rising edge to next CS falling edge.
 * @rmtoll DELAY1          INTERCS            LL_QSPI_GetInterCSDelay
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0xFF
 */
__STATIC_INLINE uint32_t LL_QSPI_GetInterCSDelay(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->DELAY0, QSPI_DELAY1_INTERCS) >> QSPI_DELAY1_INTERCS_POS);
}

/**
 * @brief  Set Delay cycles between two Data Frames.
 * @note   Units is 1 Bus Clock Cycle.
 *         0x00: No delay
 * @rmtoll DELAY1          INTERXFR            LL_QSPI_SetInterFrameDelay
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetInterFrameDelay(QSPI_TypeDef *QSPIx, uint32_t Delay)
{
    MODIFY_REG(QSPIx->DELAY1, QSPI_DELAY1_INTERXFR, Delay << QSPI_DELAY1_INTERXFR_POS);
}

/**
 * @brief  Get Delay cycles between two Data Frames.
 * @rmtoll DELAY1          INTERXFR            LL_QSPI_GetInterFrameDelay
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0xFF
 */
__STATIC_INLINE uint32_t LL_QSPI_GetInterFrameDelay(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->DELAY1, QSPI_DELAY1_INTERXFR) >> QSPI_DELAY1_INTERXFR_POS);
}

/**
 * @brief  Get TX FIFO valid data count (read only).
 * @rmtoll FIFO_NUM          TX_NUM            LL_QSPI_GetTxFIFODataValidCount
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0x07
 */
__STATIC_INLINE uint32_t LL_QSPI_GetTxFIFODataValidCount(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FIFO_NUM, QSPI_FIFO_NUM_TX_NUM) >> QSPI_FIFO_NUM_TX_NUM_POS);
}

/**
 * @brief  Get RX FIFO valid data count (read only).
 * @rmtoll FIFO_NUM          RX_NUM            LL_QSPI_GetRxFIFODataValidCount
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0x07
 */
__STATIC_INLINE uint32_t LL_QSPI_GetRxFIFODataValidCount(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FIFO_NUM, QSPI_FIFO_NUM_RX_NUM) >> QSPI_FIFO_NUM_RX_NUM_POS);
}

/**
 * @brief  Set Interface Mode (number of data lines)
 * @note   Hardware does not support Octal and X16 modes, do not configure.
 * @rmtoll FMT          PROTO            LL_QSPI_SetProtocolWireMode
 * @param  QSPIx QSPI Instance
 * @param  WireMode This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_PROTOCOLMODE_SINGLE
 *         @arg @ref LL_QSPI_PROTOCOLMODE_DUAL
 *         @arg @ref LL_QSPI_PROTOCOLMODE_QUAD
 *         @arg @ref LL_QSPI_PROTOCOLMODE_HALF_DUPLEX
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetProtocolWireMode(QSPI_TypeDef *QSPIx, uint32_t WireMode)
{
    MODIFY_REG(QSPIx->FMT, QSPI_FMT_PROTO, WireMode);
}

/**
 * @brief  Get Interface Mode (number of data lines)
 * @rmtoll FMT          PROTO            LL_QSPI_GetProtocolWireMode
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_PROTOCOLMODE_SINGLE
 *         @arg @ref LL_QSPI_PROTOCOLMODE_DUAL
 *         @arg @ref LL_QSPI_PROTOCOLMODE_QUAD
 *         @arg @ref LL_QSPI_PROTOCOLMODE_HALF_DUPLEX
 */
__STATIC_INLINE uint32_t LL_QSPI_GetProtocolWireMode(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FMT, QSPI_FMT_PROTO));
}

/**
 * @brief  Set transfer bit order
 * @note   This bit should not be changed when communication is ongoing.
 *         This bit does not change byte order,
 *         only the bit order within a single byte.
 * @rmtoll FMT          ENDIAN      LL_QSPI_SetTransferBitOrder
 * @param  QSPIx QSPI Instance
 * @param  BitOrder This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_BIT_LSB_FIRST
 *         @arg @ref LL_QSPI_BIT_MSB_FIRST
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetTransferBitOrder(QSPI_TypeDef *QSPIx, uint32_t BitOrder)
{
    MODIFY_REG(QSPIx->FMT, QSPI_FMT_ENDIAN, BitOrder);
}

/**
 * @brief  Get transfer bit order
 * @rmtoll FMT          ENDIAN      LL_QSPI_GetTransferBitOrder
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_BIT_LSB_FIRST
 *         @arg @ref LL_QSPI_BIT_MSB_FIRST
 */
__STATIC_INLINE uint32_t LL_QSPI_GetTransferBitOrder(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FMT, QSPI_FMT_ENDIAN));
}

/**
 * @brief  Set pin data transfer direction
 * @note   For pins with fixed direction (e.g., IO0 output, IO1 input in single-line mode),
 *         this bit is invalid.
 * @rmtoll FMT          DIR      LL_QSPI_SetTransferDirection
 * @param  QSPIx QSPI Instance
 * @param  TransferDirection This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_DIRECTION_RX
 *         @arg @ref LL_QSPI_DIRECTION_TX
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetTransferDirection(QSPI_TypeDef *QSPIx, uint32_t TransferDirection)
{
    MODIFY_REG(QSPIx->FMT, QSPI_FMT_DIR, TransferDirection);
}

/**
 * @brief  Get pin data transfer direction
 * @rmtoll FMT          DIR      LL_QSPI_GetTransferDirection
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_DIRECTION_RX
 *         @arg @ref LL_QSPI_DIRECTION_TX
 */
__STATIC_INLINE uint32_t LL_QSPI_GetTransferDirection(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FMT, QSPI_FMT_DIR));
}

/**
 * @brief  Set byte transmission order within 32-bits.
 * @rmtoll FMT          ENDIAN_BYTE      LL_QSPI_SetTransferByteOrder
 * @param  QSPIx QSPI Instance
 * @param  ByteOrder This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_BYTE_LSB_FIRST
 *         @arg @ref LL_QSPI_BYTE_MSB_FIRST
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetTransferByteOrder(QSPI_TypeDef *QSPIx, uint32_t ByteOrder)
{
    MODIFY_REG(QSPIx->FMT, QSPI_FMT_ENDIAN_BYTE, ByteOrder);
}

/**
 * @brief  Get byte transmission order within 32-bits.
 * @rmtoll FMT          ENDIAN_BYTE      LL_QSPI_GetTransferByteOrder
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_BYTE_LSB_FIRST
 *         @arg @ref LL_QSPI_BYTE_MSB_FIRST
 */
__STATIC_INLINE uint32_t LL_QSPI_GetTransferByteOrder(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FMT, QSPI_FMT_ENDIAN_BYTE));
}

/**
 * @brief  Set frame data width
 * @rmtoll FMT          WIDTH           LL_QSPI_SetDataWidth
 * @param  QSPIx QSPI Instance
 * @param  DataWidth This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_DATAWIDTH_8BIT
 *         @arg @ref LL_QSPI_DATAWIDTH_16BIT
 *         @arg @ref LL_QSPI_DATAWIDTH_32BIT
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetDataWidth(QSPI_TypeDef *QSPIx, uint32_t DataWidth)
{
    MODIFY_REG(QSPIx->FMT, QSPI_FMT_LEN, DataWidth);
}

/**
 * @brief  Get frame data width
 * @rmtoll FMT          WIDTH           LL_QSPI_GetDataWidth
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_DATAWIDTH_8BIT
 *         @arg @ref LL_QSPI_DATAWIDTH_16BIT
 *         @arg @ref LL_QSPI_DATAWIDTH_32BIT
 */
__STATIC_INLINE uint32_t LL_QSPI_GetDataWidth(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FMT, QSPI_FMT_LEN));
}

/**
 * @brief  Set Transmit FIFO threshold (Watermark)
 * @rmtoll TX_MARK          TX_MARK           LL_QSPI_SetTxFIFOWatermark
 * @param  QSPIx QSPI Instance
 * @param  Watermark This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_TXMARK_1
 *         @arg @ref LL_QSPI_TXMARK_2
 *         @arg @ref LL_QSPI_TXMARK_3
 *         @arg @ref LL_QSPI_TXMARK_4
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetTxFIFOWatermark(QSPI_TypeDef *QSPIx, uint32_t Watermark)
{
    MODIFY_REG(QSPIx->TX_MARK, QSPI_TX_MARK_TX_MARK, Watermark);
}

/**
 * @brief  Get Transmit FIFO threshold (Watermark)
 * @rmtoll TX_MARK          TX_MARK           LL_QSPI_GetTxFIFOWatermark
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_TXMARK_1
 *         @arg @ref LL_QSPI_TXMARK_2
 *         @arg @ref LL_QSPI_TXMARK_3
 *         @arg @ref LL_QSPI_TXMARK_4
 */
__STATIC_INLINE uint32_t LL_QSPI_GetTxFIFOWatermark(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->TX_MARK, QSPI_TX_MARK_TX_MARK));
}

/**
 * @brief  Set Receive FIFO threshold (Watermark)
 * @rmtoll RX_MARK          RX_MARK           LL_QSPI_SetRxFIFOWatermark
 * @param  QSPIx QSPI Instance
 * @param  Watermark This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_RXMARK_0
 *         @arg @ref LL_QSPI_RXMARK_1
 *         @arg @ref LL_QSPI_RXMARK_2
 *         @arg @ref LL_QSPI_RXMARK_3
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetRxFIFOWatermark(QSPI_TypeDef *QSPIx, uint32_t Watermark)
{
    MODIFY_REG(QSPIx->RX_MARK, QSPI_RX_MARK_RX_MARK, Watermark);
}

/**
 * @brief  Get Receive FIFO threshold (Watermark)
 * @rmtoll RX_MARK          RX_MARK           LL_QSPI_GetRxFIFOWatermark
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_RXMARK_0
 *         @arg @ref LL_QSPI_RXMARK_1
 *         @arg @ref LL_QSPI_RXMARK_2
 *         @arg @ref LL_QSPI_RXMARK_3
 */
__STATIC_INLINE uint32_t LL_QSPI_GetRxFIFOWatermark(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->RX_MARK, QSPI_RX_MARK_RX_MARK));
}

/**
 * @brief  Set SDR Mode SCK sample point delay
 * @note   Sampling delay must be less than 1 QSPI_SCK period.
 * @rmtoll SDR_SCKSAMPLE          SDR_SCKSAMPLE          LL_QSPI_SetSDRSampleDelay
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetSDRSampleDelay(QSPI_TypeDef *QSPIx, uint32_t SampleDelay)
{
    MODIFY_REG(QSPIx->SDR_SCKSAMPLE, QSPI_SDR_SCKSAMPLE_SDR_SCKSAMPLE, SampleDelay << QSPI_SDR_SCKSAMPLE_SDR_SCKSAMPLE_POS);
}

/**
 * @brief  Get SDR Mode SCK sample point delay
 * @rmtoll SDR_SCKSAMPLE          SDR_SCKSAMPLE          LL_QSPI_GetSDRSampleDelay
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0xFFF
 */
__STATIC_INLINE uint32_t LL_QSPI_GetSDRSampleDelay(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->SDR_SCKSAMPLE, QSPI_SDR_SCKSAMPLE_SDR_SCKSAMPLE) >> QSPI_SDR_SCKSAMPLE_SDR_SCKSAMPLE_POS);
}

/**
 * @brief  Set QSPI operation mode to Master or Slave
 * @note   This bit should not be changed when communication is ongoing.
 * @rmtoll CR          MSTER          LL_QSPI_SetMode
 * @param  QSPIx QSPI Instance
 * @param  Mode This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_MODE_MASTER
 *         @arg @ref LL_QSPI_MODE_SLAVE
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetMode(QSPI_TypeDef *QSPIx, uint32_t Mode)
{
    MODIFY_REG(QSPIx->CR, QSPI_CR_MSTER, Mode);
}

/**
 * @brief  Get QSPI operation mode (Master or Slave)
 * @rmtoll CR          MSTER          LL_QSPI_GetMode
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_MODE_MASTER
 *         @arg @ref LL_QSPI_MODE_SLAVE
 */
__STATIC_INLINE uint32_t LL_QSPI_GetMode(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->CR, QSPI_CR_MSTER));
}

/**
 * @brief  Enable DMA Mode.
 * @rmtoll CR          DMA_EN           LL_QSPI_EnableDMAMode
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableDMAMode(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->CR, QSPI_CR_DMA_EN);
}

/**
 * @brief  Disable DMA Mode.
 * @rmtoll CR          DMA_EN           LL_QSPI_DisableDMAMode
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableDMAMode(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->CR, QSPI_CR_DMA_EN);
}

/**
 * @brief  Check if DMA Mode is enabled
 * @rmtoll CR          DMA_EN           LL_QSPI_IsEnabledDMAMode
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledDMAMode(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->CR, QSPI_CR_DMA_EN) == (QSPI_CR_DMA_EN));
}

/**
 * @brief  Enable DDR Mode.
 * @rmtoll CR          DDR_EN           LL_QSPI_EnableDDRMode
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableDDRMode(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->CR, QSPI_CR_DDR_EN);
}

/**
 * @brief  Disable DDR Mode.
 * @rmtoll CR          DDR_EN           LL_QSPI_DisableDDRMode
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableDDRMode(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->CR, QSPI_CR_DDR_EN);
}

/**
 * @brief  Check if DDR Mode is enabled
 * @rmtoll CR          DDR_EN           LL_QSPI_IsEnabledDDRMode
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledDDRMode(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->CR, QSPI_CR_DDR_EN) == (QSPI_CR_DDR_EN));
}

/**
 * @brief  Enable Software-controlled Chip.
 * @rmtoll CR          CSI           LL_QSPI_EnableSoftwareSlaveChipSelect\n
 *         CR          SSM           LL_QSPI_EnableSoftwareSlaveChipSelect
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableSoftwareSlaveChipSelect(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->CR, QSPI_CR_SSM);
    CLEAR_BIT(QSPIx->CR, QSPI_CR_CSI);
}

/**
 * @brief  Disable Software-controlled Chip.
 * @rmtoll CR          CSI           LL_QSPI_DisableSoftwareSlaveChipSelect\n
 *         CR          SSM           LL_QSPI_DisableSoftwareSlaveChipSelect
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableSoftwareSlaveChipSelect(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->CR, QSPI_CR_SSM);
}

/**
 * @brief  Check if Software-controlled Chip is enabled
 * @rmtoll CR          CSI           LL_QSPI_IsEnabledSoftwareSlaveChipSelect\n
 *         CR          SSM           LL_QSPI_IsEnabledSoftwareSlaveChipSelect
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledSoftwareSlaveChipSelect(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->CR, QSPI_CR_SSM) == (QSPI_CR_SSM));
}

/**
 * @brief  Enable Master Mode Chip Select Output
 * @rmtoll CR          CSOE           LL_QSPI_EnableMasterChipSelectOutput
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableMasterChipSelectOutput(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->CR, QSPI_CR_CSOE);
}

/**
 * @brief  Disable Master Mode Chip Select Output
 * @rmtoll CR          CSOE           LL_QSPI_DisableMasterChipSelectOutput
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableMasterChipSelectOutput(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->CR, QSPI_CR_CSOE);
}

/**
 * @brief  Check if Master Mode Chip Select Output is enabled
 * @rmtoll CR          CSOE           LL_QSPI_IsEnabledMasterChipSelectOutput
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledMasterChipSelectOutput(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->CR, QSPI_CR_CSOE) == (QSPI_CR_CSOE));
}









































/**
 * @}
 */

/** @defgroup QSPI_LL_EF_CHIP_SELECT Chip Select functions
 * @{
 */

/**
 * @brief  Set Chip Select Device ID
 * @rmtoll CSID          CSID          LL_QSPI_SetChipSelectID
 * @param  QSPIx QSPI Instance
 * @param  ChipSelectID This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_CSID_NONE
 *         @arg @ref LL_QSPI_CSID_0
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetChipSelectID(QSPI_TypeDef *QSPIx, uint32_t ChipSelectID)
{
    MODIFY_REG(QSPIx->CSID, QSPI_CSID_CSID, ChipSelectID);
}

/**
 * @brief  Get Chip Select Device ID
 * @rmtoll CSID          CSID          LL_QSPI_GetChipSelectID
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_CSID_NONE
 *         @arg @ref LL_QSPI_CSID_0
 */
__STATIC_INLINE uint32_t LL_QSPI_GetChipSelectID(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->CSID, QSPI_CSID_CSID));
}

/**
 * @brief  Set CS0 Default (IDLE) state
 * @rmtoll CSDEF          CSDEF          LL_QSPI_SetChipSelect0IdleState
 * @param  QSPIx QSPI Instance
 * @param  IdleState This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_CS0_IDLESTATE_LOW
 *         @arg @ref LL_QSPI_CS0_IDLESTATE_HIGH
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetChipSelect0IdleState(QSPI_TypeDef *QSPIx, uint32_t IdleState)
{
    MODIFY_REG(QSPIx->CSDEF, QSPI_CSDEF_CSDEF, IdleState);
}

/**
 * @brief  Get CS0 Default (IDLE) state
 * @rmtoll CSDEF          CSDEF          LL_QSPI_GetChipSelect0IdleState
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_CS0_IDLESTATE_LOW
 *         @arg @ref LL_QSPI_CS0_IDLESTATE_HIGH
 */
__STATIC_INLINE uint32_t LL_QSPI_GetChipSelect0IdleState(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->CSDEF, QSPI_CSDEF_CSDEF));
}

/**
 * @brief  Set Chip Select mode
 * @note   Use Hold mode for continuous data transfer,
 *         switch to Disable mode after transfer completes.
 * @rmtoll CSMODE          CSMODE          LL_QSPI_SetChipSelectMode
 * @param  QSPIx QSPI Instance
 * @param  CSMode This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_CSMODE_AUTO
 *         @arg @ref LL_QSPI_CSMODE_HOLD
 *         @arg @ref LL_QSPI_CSMODE_DISABLE
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_SetChipSelectMode(QSPI_TypeDef *QSPIx, uint32_t CSMode)
{
    MODIFY_REG(QSPIx->CSMODE, QSPI_CSMODE_CSMODE, CSMode);
}

/**
 * @brief  Get Chip Select mode
 * @rmtoll CSMODE          CSMODE          LL_QSPI_SetChipSelectMode
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_CSMODE_AUTO
 *         @arg @ref LL_QSPI_CSMODE_HOLD
 *         @arg @ref LL_QSPI_CSMODE_DISABLE
 */
__STATIC_INLINE uint32_t LL_QSPI_GetChipSelectMode(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->CSMODE, QSPI_CSMODE_CSMODE));
}

/**
 * @}
 */

/** @defgroup QSPI_LL_EF_XIP_Management XIP_Management
 * @{
 */

/**
 * @brief  Set SPI Flash start address offset
 * @note   Invalid in Quad-line mode.
 * @rmtoll ADDR_WRAP          OFFSET          LL_QSPI_SetFlashStartAddressOffset
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFFFFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetFlashStartAddressOffset(QSPI_TypeDef *QSPIx, uint32_t Offset)
{
    QSPIx->ADDR_WRAP = Offset;
}

/**
 * @brief  Get SPI Flash start address offset
 * @rmtoll ADDR_WRAP          OFFSET          LL_QSPI_GetFlashStartAddressOffset
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetFlashStartAddressOffset(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_REG(QSPIx->ADDR_WRAP));
}

/**
 * @brief  Set XIP boundary address configuration
 * @note   Set to SPI FLASH address boundary-1. Enabled by QSPI_FFMT.BOUNDRY_EN bit.
 * @rmtoll BOUNDARY_CFG          BOUNDARY          LL_QSPI_XIP_SetFlashAddressBoundary
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFFFFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetFlashAddressBoundary(QSPI_TypeDef *QSPIx, uint32_t Boundary)
{
    QSPIx->BOUNDARY_CFG = Boundary;
}

/**
 * @brief  Get XIP boundary address configuration
 * @rmtoll BOUNDARY_CFG          BOUNDARY          LL_QSPI_XIP_GetFlashAddressBoundary
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetFlashAddressBoundary(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_REG(QSPIx->BOUNDARY_CFG));
}

/**
 * @brief  Enable XIP Mode
 * @rmtoll FCTRL          FLASH_EN        LL_QSPI_XIP_Enable
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_Enable(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_EN);
}

/**
 * @brief  Disable XIP Mode
 * @rmtoll FCTRL          FLASH_EN        LL_QSPI_XIP_Disable
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_Disable(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_EN);
}

/**
 * @brief  Check if the QSPI XIP Mode is enabled or disabled.
 * @rmtoll FCTRL          FLASH_EN        LL_QSPI_XIP_IsEnabled
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_IsEnabled(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_EN) == (QSPI_FCTRL_FLASH_EN));
}

/**
 * @brief  Enable XIP Mode Write Data Mask Function
 * @rmtoll FCTRL          FLASH_WMASK_EN        LL_QSPI_XIP_EnableDataMask
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_EnableDataMask(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_WMASK_EN);
}

/**
 * @brief  Disable XIP Mode Write Data Mask Function
 * @rmtoll FCTRL          FLASH_WMASK_EN        LL_QSPI_XIP_DisableDataMask
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_DisableDataMask(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_WMASK_EN);
}

/**
 * @brief  Check if the QSPI XIP Mode Write Data Mask Function is enabled or disabled.
 * @rmtoll FCTRL          FLASH_WMASK_EN        LL_QSPI_XIP_IsEnabledDataMask
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_IsEnabledDataMask(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_WMASK_EN) == (QSPI_FCTRL_FLASH_WMASK_EN));
}

/**
 * @brief  Enable XIP Mode Write
 * @rmtoll FCTRL          FLASH_WEN        LL_QSPI_XIP_EnableWrite
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_EnableWrite(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_WEN);
}

/**
 * @brief  Disable XIP Mode Write
 * @rmtoll FCTRL          FLASH_WEN        LL_QSPI_XIP_DisableWrite
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_DisableWrite(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_WEN);
}

/**
 * @brief  Check if the QSPI XIP Mode Write is enabled or disabled.
 * @rmtoll FCTRL          FLASH_WEN        LL_QSPI_XIP_IsEnabledWrite
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_IsEnabledWrite(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_WEN) == (QSPI_FCTRL_FLASH_WEN));
}

/**
 * @brief  Enable XIP Mode Burst Transfer
 * @rmtoll FCTRL          FLASH_BURST_EN        LL_QSPI_XIP_EnableBurst
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_EnableBurst(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_BURST_EN);
}

/**
 * @brief  Disable XIP Mode Burst Transfer
 * @rmtoll FCTRL          FLASH_BURST_EN        LL_QSPI_XIP_DisableBurst
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_DisableBurst(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_BURST_EN);
}

/**
 * @brief  Check if the QSPI XIP Mode Burst Transfer is enabled or disabled.
 * @rmtoll FCTRL          FLASH_BURST_EN        LL_QSPI_XIP_IsEnabledBurst
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_IsEnabledBurst(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_BURST_EN) == (QSPI_FCTRL_FLASH_BURST_EN));
}

/**
 * @brief  Enable XIP Mode WRAP Function
 * @rmtoll FCTRL          FLASH_WRAP_EN        LL_QSPI_XIP_EnableWrap
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_EnableWrap(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_WRAP_EN);
}

/**
 * @brief  Disable XIP Mode WRAP Function
 * @rmtoll FCTRL          FLASH_WRAP_EN        LL_QSPI_XIP_DisableWrap
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_DisableWrap(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_WRAP_EN);
}

/**
 * @brief  Check if the QSPI XIP Mode WRAP Function is enabled or disabled.
 * @rmtoll FCTRL          FLASH_WRAP_EN        LL_QSPI_XIP_IsEnabledWrap
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_IsEnabledWrap(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->FCTRL, QSPI_FCTRL_FLASH_WRAP_EN) == (QSPI_FCTRL_FLASH_WRAP_EN));
}

/**
 * @brief  Enable XIP Mode Boundary Address
 * @rmtoll FCTRL          BOUNDRY_EN        LL_QSPI_XIP_EnableBoundaryAddress
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_EnableBoundaryAddress(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->FCTRL, QSPI_FCTRL_BOUNDRY_EN);
}

/**
 * @brief  Disable XIP Mode Boundary Address
 * @rmtoll FCTRL          BOUNDRY_EN        LL_QSPI_XIP_DisableBoundaryAddress
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_DisableBoundaryAddress(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->FCTRL, QSPI_FCTRL_BOUNDRY_EN);
}

/**
 * @brief  Check if the QSPI XIP Mode Boundary Address is enabled or disabled.
 * @rmtoll FCTRL          BOUNDRY_EN        LL_QSPI_XIP_IsEnabledBoundaryAddress
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_IsEnabledBoundaryAddress(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->FCTRL, QSPI_FCTRL_BOUNDRY_EN) == (QSPI_FCTRL_BOUNDRY_EN));
}

/**
 * @brief  Enable Instruction Send
 * @rmtoll FFMT          CMD_EN        LL_QSPI_XIP_EnableSendCommand
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_EnableSendCommand(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->FFMT, QSPI_FFMT_CMD_EN);
}

/**
 * @brief  Disable Instruction Send
 * @rmtoll FFMT          CMD_EN        LL_QSPI_XIP_DisableSendCommand
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_DisableSendCommand(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->FFMT, QSPI_FFMT_CMD_EN);
}

/**
 * @brief  Check if the QSPI Instruction Send is enabled or disabled.
 * @rmtoll FFMT          CMD_EN        LL_QSPI_XIP_IsEnabledSendCommand
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_IsEnabledSendCommand(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->FFMT, QSPI_FFMT_CMD_EN) == (QSPI_FFMT_CMD_EN));
}

/**
 * @brief  Set XIP Address Length configuration
 * @note   Set to SPI FLASH address boundary-1. Enabled by QSPI_FFMT.BOUNDRY_EN bit.
 * @rmtoll FFMT          ADDR_LEN          LL_QSPI_XIP_SetAddressWidth
 * @param  QSPIx QSPI Instance
 * @param  AddressWidth This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_ADDRESSWIDTH_NONE
 *         @arg @ref LL_QSPI_XIP_ADDRESSWIDTH_8BIT
 *         @arg @ref LL_QSPI_XIP_ADDRESSWIDTH_16BIT
 *         @arg @ref LL_QSPI_XIP_ADDRESSWIDTH_24BIT
 *         @arg @ref LL_QSPI_XIP_ADDRESSWIDTH_32BIT
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetAddressWidth(QSPI_TypeDef *QSPIx, uint32_t AddressWidth)
{
    MODIFY_REG(QSPIx->FFMT, QSPI_FFMT_ADDR_LEN, AddressWidth);
}

/**
 * @brief  Get XIP Address Length configuration
 * @rmtoll FFMT          ADDR_LEN          LL_QSPI_XIP_GetAddressWidth
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_ADDRESSWIDTH_NONE
 *         @arg @ref LL_QSPI_XIP_ADDRESSWIDTH_8BIT
 *         @arg @ref LL_QSPI_XIP_ADDRESSWIDTH_16BIT
 *         @arg @ref LL_QSPI_XIP_ADDRESSWIDTH_24BIT
 *         @arg @ref LL_QSPI_XIP_ADDRESSWIDTH_32BIT
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetAddressWidth(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FFMT, QSPI_FFMT_ADDR_LEN));
}

/**
 * @brief  Set Read instruction dummy cycle count
 * @note   The 5th bit (MSB) of RPAD_CNT is configured in FFMT1.RPAD_CNT_H.
 * @rmtoll FFMT          RPAD_CNT            LL_QSPI_XIP_SetReadDummyCycle\n
 *         FFMT1         RPAD_CNT_H          LL_QSPI_XIP_SetReadDummyCycle
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0x1F
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetReadDummyCycle(QSPI_TypeDef *QSPIx, uint32_t DummyCycle)
{
    if (((DummyCycle >> DUMMY_CYCLE_H_POS) & 0x1) == 0) {
        MODIFY_REG(QSPIx->FFMT, QSPI_FFMT_RPAD_CNT, (DummyCycle & 0x0FU) << QSPI_FFMT_RPAD_CNT_POS);
    } else {
        MODIFY_REG(QSPIx->FFMT, QSPI_FFMT_RPAD_CNT, (DummyCycle & 0x0FU) << QSPI_FFMT_RPAD_CNT_POS);
        SET_BIT(QSPIx->FFMT1, QSPI_FFMT1_RPAD_CNT_H);
    }
}

/**
 * @brief  Get Read instruction dummy cycle count
 * @rmtoll FFMT          RPAD_CNT            LL_QSPI_XIP_GetReadDummyCycle\n
 *         FFMT1         RPAD_CNT_H          LL_QSPI_XIP_GetReadDummyCycle
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0x1F
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetReadDummyCycle(const QSPI_TypeDef *QSPIx)
{
    if (((QSPIx->FFMT1 >> QSPI_FFMT1_RPAD_CNT_H_POS) & 0x1) == 0) {
        return (uint32_t)(READ_BIT(QSPIx->FFMT, QSPI_FFMT_RPAD_CNT) >> QSPI_FFMT_RPAD_CNT_POS);
    } else {
        uint8_t DummyCycle_L = READ_BIT(QSPIx->FFMT, QSPI_FFMT_RPAD_CNT) >> QSPI_FFMT_RPAD_CNT_POS;
        return (uint32_t)((0x1 << DUMMY_CYCLE_H_POS) | DummyCycle_L);
    }
}

/**
 * @brief  Set Command Phase Interface Mode
 * @rmtoll FFMT          CMD_PROTO            LL_QSPI_XIP_SetCommandProtocolWireMode
 * @param  QSPIx QSPI Instance
 * @param  WireMode This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_COMMAND_PROTOCOLMODE_SINGLEE
 *         @arg @ref LL_QSPI_XIP_COMMAND_PROTOCOLMODE_DUAL
 *         @arg @ref LL_QSPI_XIP_COMMAND_PROTOCOLMODE_QUAD
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetCommandProtocolWireMode(QSPI_TypeDef *QSPIx, uint32_t WireMode)
{
    MODIFY_REG(QSPIx->FFMT, QSPI_FFMT_CMD_PROTO, WireMode);
}

/**
 * @brief  Get Command Phase Interface Mode
 * @rmtoll FFMT          CMD_PROTO            LL_QSPI_XIP_GetCommandProtocolWireMode
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_COMMAND_PROTOCOLMODE_SINGLE
 *         @arg @ref LL_QSPI_XIP_COMMAND_PROTOCOLMODE_DUAL
 *         @arg @ref LL_QSPI_XIP_COMMAND_PROTOCOLMODE_QUAD
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetCommandProtocolWireMode(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FFMT, QSPI_FFMT_CMD_PROTO));
}

/**
 * @brief  Set Address Phase Interface Mode
 * @rmtoll FFMT          ADDR_PROTO            LL_QSPI_XIP_SetAddressProtocolWireMode
 * @param  QSPIx QSPI Instance
 * @param  WireMode This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_ADDRESS_PROTOCOLMODE_SINGLEE
 *         @arg @ref LL_QSPI_XIP_ADDRESS_PROTOCOLMODE_DUAL
 *         @arg @ref LL_QSPI_XIP_ADDRESS_PROTOCOLMODE_QUAD
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetAddressProtocolWireMode(QSPI_TypeDef *QSPIx, uint32_t WireMode)
{
    MODIFY_REG(QSPIx->FFMT, QSPI_FFMT_ADDR_PROTO, WireMode);
}

/**
 * @brief  Get Address Phase Interface Mode
 * @rmtoll FFMT          ADDR_PROTO            LL_QSPI_XIP_GetAddressProtocolWireMode
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_ADDRESS_PROTOCOLMODE_SINGLE
 *         @arg @ref LL_QSPI_XIP_ADDRESS_PROTOCOLMODE_DUAL
 *         @arg @ref LL_QSPI_XIP_ADDRESS_PROTOCOLMODE_QUAD
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetAddressProtocolWireMode(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FFMT, QSPI_FFMT_ADDR_PROTO));
}

/**
 * @brief  Set Data Phase Interface Mode
 * @rmtoll FFMT          DATA_PROTO            LL_QSPI_XIP_SetDataProtocolWireMode\n
 *         FFMT          DATA_PROTO_HI         LL_QSPI_XIP_SetDataProtocolWireMode
 * @param  QSPIx QSPI Instance
 * @param  WireMode This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_DATA_PROTOCOLMODE_SINGLE
 *         @arg @ref LL_QSPI_XIP_DATA_PROTOCOLMODE_DUAL
 *         @arg @ref LL_QSPI_XIP_DATA_PROTOCOLMODE_QUAD
 *         @arg @ref LL_QSPI_XIP_DATA_PROTOCOLMODE_HALF_DUPLEX
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetDataProtocolWireMode(QSPI_TypeDef *QSPIx, uint32_t WireMode)
{
    MODIFY_REG(QSPIx->FFMT, QSPI_FFMT_DATA_PROTO, WireMode);
}

/**
 * @brief  Get Data Phase Interface Mode
 * @rmtoll FFMT          DATA_PROTO            LL_QSPI_XIP_GetDataProtocolWireMode\n
 *         FFMT          DATA_PROTO_HI         LL_QSPI_XIP_GetDataProtocolWireMode
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_DATA_PROTOCOLMODE_SINGLE
 *         @arg @ref LL_QSPI_XIP_DATA_PROTOCOLMODE_DUAL
 *         @arg @ref LL_QSPI_XIP_DATA_PROTOCOLMODE_QUAD
 *         @arg @ref LL_QSPI_XIP_DATA_PROTOCOLMODE_HALF_DUPLEX
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetDataProtocolWireMode(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FFMT, QSPI_FFMT_DATA_PROTO));
}

/**
 * @brief  Set transfer bit order
 * @rmtoll FFMT          ENDIAN_F            LL_QSPI_XIP_SetTransferBitOrder
 * @param  QSPIx QSPI Instance
 * @param  BitOrder This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_BIT_LSB_FIRST
 *         @arg @ref LL_QSPI_XIP_BIT_MSB_FIRST
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetTransferBitOrder(QSPI_TypeDef *QSPIx, uint32_t BitOrder)
{
    MODIFY_REG(QSPIx->FFMT, QSPI_FFMT_ENDIAN_F, BitOrder);
}

/**
 * @brief  Get transfer bit order
 * @rmtoll FFMT          ENDIAN_F            LL_QSPI_XIP_GetTransferBitOrder
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_BIT_LSB_FIRST
 *         @arg @ref LL_QSPI_XIP_BIT_MSB_FIRST
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetTransferBitOrder(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FFMT, QSPI_FFMT_ENDIAN_F));
}

/**
 * @brief  Set QSPI Flash Read Instruction Opcode.
 * @rmtoll FFMT          RCMD_CODE          LL_QSPI_XIP_SetReadCommandByte
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetReadCommandByte(QSPI_TypeDef *QSPIx, uint32_t Command)
{
    MODIFY_REG(QSPIx->FFMT, QSPI_FFMT_RCMD_CODE, Command << QSPI_FFMT_RCMD_CODE_POS);
}

/**
 * @brief  Get QSPI Flash Read Instruction Opcode.
 * @rmtoll FFMT          RCMD_CODE          LL_QSPI_XIP_GetReadCommandByte
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0xFF
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetReadCommandByte(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FFMT, QSPI_FFMT_RCMD_CODE) >> QSPI_FFMT_RCMD_CODE_POS);
}

/**
 * @brief  Set First 8 bits of data transmitted during dummy
 * @rmtoll FFMT          RCMD_CODE          LL_QSPI_XIP_SetDummyCycleFirstBits
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetDummyCycleFirstBits(QSPI_TypeDef *QSPIx, uint32_t Value)
{
    MODIFY_REG(QSPIx->FFMT, QSPI_FFMT_PAD_CODE, Value << QSPI_FFMT_PAD_CODE_POS);
}

/**
 * @brief  Get First 8 bits of data transmitted during dummy
 * @rmtoll FFMT          RCMD_CODE          LL_QSPI_XIP_GetDummyCycleFirstBits
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0xFF
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetDummyCycleFirstBits(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FFMT, QSPI_FFMT_PAD_CODE) >> QSPI_FFMT_PAD_CODE_POS);
}

/**
 * @brief  Set QSPI SPI Flash Write Instruction Opcode.
 * @rmtoll FFMT1          WCMD_CODE          LL_QSPI_XIP_SetWriteCommandByte
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetWriteCommandByte(QSPI_TypeDef *QSPIx, uint32_t Command)
{
    MODIFY_REG(QSPIx->FFMT1, QSPI_FFMT1_WCMD_CODE, Command << QSPI_FFMT1_WCMD_CODE_POS);
}

/**
 * @brief  Get QSPI SPI Flash Write Instruction Opcode.
 * @rmtoll FFMT1          WCMD_CODE          LL_QSPI_XIP_GetWriteCommandByte
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0xFF
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetWriteCommandByte(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FFMT1, QSPI_FFMT1_WCMD_CODE) >> QSPI_FFMT1_WCMD_CODE_POS);
}

/**
 * @brief  Set Write instruction dummy cycle count
 * @rmtoll FFMT1          WPAD_CNT            LL_QSPI_XIP_SetWriteDummyCycle
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0x1F
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetWriteDummyCycle(QSPI_TypeDef *QSPIx, uint32_t DummyCycle)
{
    MODIFY_REG(QSPIx->FFMT1, QSPI_FFMT1_WPAD_CNT, DummyCycle << QSPI_FFMT1_WPAD_CNT_POS);
}

/**
 * @brief  Get Write instruction dummy cycle count
 * @rmtoll FFMT1          WPAD_CNT            LL_QSPI_XIP_GetWriteDummyCycle
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0x1F
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetWriteDummyCycle(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FFMT1, QSPI_FFMT1_WPAD_CNT) >> QSPI_FFMT1_WPAD_CNT_POS);
}

/**
 * @brief  Enable DDR for different phases: Instruction[14], Address[15], Mode[16], Data[17].
 * @rmtoll FFMT1          DDR_EN        LL_QSPI_XIP_EnableDDR
 * @param  QSPIx QSPI Instance
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_EnableDDR(QSPI_TypeDef *QSPIx, uint32_t DDREnable)
{
    SET_BIT(QSPIx->FFMT1, DDREnable);
}

/**
 * @brief  Disable DDR for different phases: Instruction[14], Address[15], Mode[16], Data[17].
 * @rmtoll FFMT1          DDR_EN        LL_QSPI_XIP_DisableDDR
 * @param  BitOrder This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_DDRENABLE_COMMAND
 *         @arg @ref LL_QSPI_XIP_DDRENABLE_ADDRESS
 *         @arg @ref LL_QSPI_XIP_DDRENABLE_MODE
 *         @arg @ref LL_QSPI_XIP_DDRENABLE_DATA
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_DisableDDR(QSPI_TypeDef *QSPIx, uint32_t DDREnable)
{
    CLEAR_BIT(QSPIx->FFMT1, DDREnable);
}

/**
 * @brief  Check if the QSPI DDR for different phases is enabled or disabled.
 * @rmtoll FFMT1          DDR_EN        LL_QSPI_XIP_IsEnabledDDR
 * @param  QSPIx QSPI Instance
 * @param  BitOrder This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_DDRENABLE_COMMAND
 *         @arg @ref LL_QSPI_XIP_DDRENABLE_ADDRESS
 *         @arg @ref LL_QSPI_XIP_DDRENABLE_MODE
 *         @arg @ref LL_QSPI_XIP_DDRENABLE_DATA
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_IsEnabledDDR(const QSPI_TypeDef *QSPIx, uint32_t DDREnable)
{
    return (READ_BIT(QSPIx->FFMT1, DDREnable) == (DDREnable));
}

/**
 * @brief  Set Mode Byte Phase Interface Mode
 * @rmtoll FFMT1          MODE_PROTO            LL_QSPI_XIP_SetModeProtocolWireMode
 * @param  QSPIx QSPI Instance
 * @param  WireMode This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_MODE_PROTOCOLMODE_SINGLE
 *         @arg @ref LL_QSPI_XIP_MODE_PROTOCOLMODE_DUAL
 *         @arg @ref LL_QSPI_XIP_MODE_PROTOCOLMODE_QUAD
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetModeProtocolWireMode(QSPI_TypeDef *QSPIx, uint32_t WireMode)
{
    MODIFY_REG(QSPIx->FFMT1, QSPI_FFMT1_MODE_PROTO, WireMode);
}

/**
 * @brief  Get Mode Byte Phase Interface Mode
 * @rmtoll FFMT1          MODE_PROTO            LL_QSPI_XIP_GetModeProtocolWireMode
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_MODE_PROTOCOLMODE_SINGLE
 *         @arg @ref LL_QSPI_XIP_MODE_PROTOCOLMODE_DUAL
 *         @arg @ref LL_QSPI_XIP_MODE_PROTOCOLMODE_QUAD
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetModeProtocolWireMode(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FFMT1, QSPI_FFMT1_MODE_PROTO));
}

/**
 * @brief  Set QSPI SPI Flash Mode Byte.
 * @rmtoll FFMT1          MODE_CODE          LL_QSPI_XIP_SetModeByte
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetModeByte(QSPI_TypeDef *QSPIx, uint32_t Mode)
{
    MODIFY_REG(QSPIx->FFMT1, QSPI_FFMT1_MODE_CODE, Mode << QSPI_FFMT1_MODE_CODE_POS);
}

/**
 * @brief  Get QSPI SPI Flash Mode Byte.
 * @rmtoll FFMT1          MODE_CODE          LL_QSPI_XIP_GetModeByte
 * @param  QSPIx QSPI Instance
 * @retval Returned value is a number between Min_Data = 0x00 and Max_Data = 0xFF
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetModeByte(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FFMT1, QSPI_FFMT1_MODE_CODE) >> QSPI_FFMT1_MODE_CODE_POS);
}

/**
 * @brief  Set QSPI SPI Flash Mode Byte.
 * @rmtoll FFMT1          MODE_CNT          LL_QSPI_XIP_SetModeByteWidth
 * @param  QSPIx QSPI Instance
 * @param  Width This parameter can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_MODEWIDTH_NONE
 *         @arg @ref LL_QSPI_XIP_MODEWIDTH_8BIT
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_XIP_SetModeByteWidth(QSPI_TypeDef *QSPIx, uint32_t Width)
{
    MODIFY_REG(QSPIx->FFMT1, QSPI_FFMT1_MODE_CNT, Width << QSPI_FFMT1_MODE_CNT_POS);
}

/**
 * @brief  Get QSPI SPI Flash Mode Byte.
 * @rmtoll FFMT1          MODE_CNT          LL_QSPI_XIP_GetModeByteWidth
 * @param  QSPIx QSPI Instance
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_QSPI_XIP_MODEWIDTH_NONE
 *         @arg @ref LL_QSPI_XIP_MODEWIDTH_8BIT
 */
__STATIC_INLINE uint32_t LL_QSPI_XIP_GetModeByteWidth(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->FFMT1, QSPI_FFMT1_MODE_CNT) >> QSPI_FFMT1_MODE_CNT_POS);
}





















/**
 * @}
 */

/** @defgroup QSPI_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable Transmit FIFO Threshold Interrupt
 * @rmtoll IE          TR_IRQ_EN        LL_QSPI_EnableIT_TX
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableIT_TX(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->IE, QSPI_IE_TX_IRQ_EN);
}

/**
 * @brief  Disable Transmit FIFO Threshold Interrupt
 * @rmtoll IE          TR_IRQ_EN        LL_QSPI_DisableIT_TX
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableIT_TX(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->IE, QSPI_IE_TX_IRQ_EN);
}

/**
 * @brief  Check if the QSPI Transmit FIFO Threshold Interrupt is enabled or disabled.
 * @rmtoll IE          TR_IRQ_EN        LL_QSPI_IsEnabledIT_TX
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledIT_TX(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->IE, QSPI_IE_TX_IRQ_EN) == (QSPI_IE_TX_IRQ_EN));
}

/**
 * @brief  Enable Receive FIFO Threshold Interrupt
 * @rmtoll IE          RX_IRQ_EN        LL_QSPI_EnableIT_RX
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableIT_RX(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->IE, QSPI_IE_RX_IRQ_EN);
}

/**
 * @brief  Disable Receive FIFO Threshold Interrupt
 * @rmtoll IE          RX_IRQ_EN        LL_QSPI_DisableIT_RX
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableIT_RX(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->IE, QSPI_IE_RX_IRQ_EN);
}

/**
 * @brief  Check if the QSPI Receive FIFO Threshold Interrupt is enabled or disabled.
 * @rmtoll IE          RX_IRQ_EN        LL_QSPI_IsEnabledIT_RX
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledIT_RX(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->IE, QSPI_IE_RX_IRQ_EN) == (QSPI_IE_RX_IRQ_EN));
}

/**
 * @brief  Enable Transmit FIFO Underflow Interrupt
 * @rmtoll IE          TX_UDR_EN        LL_QSPI_EnableIT_TXUDR
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableIT_TXUDR(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->IE, QSPI_IE_TX_UDR_EN);
}

/**
 * @brief  Disable Transmit FIFO Underflow Interrupt
 * @rmtoll IE          TX_UDR_EN        LL_QSPI_DisableIT_TXUDR
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableIT_TXUDR(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->IE, QSPI_IE_TX_UDR_EN);
}

/**
 * @brief  Check if the QSPI Transmit FIFO Underflow Interrupt is enabled or disabled.
 * @rmtoll IE          TX_UDR_EN        LL_QSPI_IsEnabledIT_TXUDR
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledIT_TXUDR(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->IE, QSPI_IE_TX_UDR_EN) == (QSPI_IE_TX_UDR_EN));
}

/**
 * @brief  Enable Receive FIFO Overflow Interrupt
 * @rmtoll IE          RX_OVR_EN        LL_QSPI_EnableIT_RXOVR
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableIT_RXOVR(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->IE, QSPI_IE_RX_OVR_EN);
}

/**
 * @brief  Disable Receive FIFO Overflow Interrupt
 * @rmtoll IE          RX_OVR_EN        LL_QSPI_DisableIT_RXOVR
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableIT_RXOVR(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->IE, QSPI_IE_RX_OVR_EN);
}

/**
 * @brief  Check if the QSPI Receive FIFO Overflow Interrupt is enabled or disabled.
 * @rmtoll IE          RX_OVR_EN        LL_QSPI_IsEnabledIT_RXOVR
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledIT_RXOVR(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->IE, QSPI_IE_RX_OVR_EN) == (QSPI_IE_RX_OVR_EN));
}

/**
 * @brief  Enable Receive FIFO Underflow Interrupt
 * @rmtoll IE          RX_UDR_EN        LL_QSPI_EnableIT_RXUDR
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableIT_RXUDR(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->IE, QSPI_IE_RX_UDR_EN);
}

/**
 * @brief  Disable Receive FIFO Underflow Interrupt
 * @rmtoll IE          RX_UDR_EN        LL_QSPI_DisableIT_RXUDR
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableIT_RXUDR(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->IE, QSPI_IE_RX_UDR_EN);
}

/**
 * @brief  Check if the QSPI Receive FIFO Underflow Interrupt is enabled or disabled.
 * @rmtoll IE          RX_UDR_EN        LL_QSPI_IsEnabledIT_RXUDR
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledIT_RXUDR(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->IE, QSPI_IE_RX_UDR_EN) == (QSPI_IE_RX_UDR_EN));
}

/**
 * @brief  Enable Transmit FIFO Overflow Interrupt
 * @rmtoll IE          TX_OVR_EN        LL_QSPI_EnableIT_TXOVR
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableIT_TXOVR(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->IE, QSPI_IE_TX_OVR_EN);
}

/**
 * @brief  Disable Transmit FIFO Overflow Interrupt
 * @rmtoll IE          TX_OVR_EN        LL_QSPI_DisableIT_TXOVR
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableIT_TXOVR(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->IE, QSPI_IE_TX_OVR_EN);
}

/**
 * @brief  Check if the QSPI Transmit FIFO Overflow Interrupt is enabled or disabled.
 * @rmtoll IE          TX_OVR_EN        LL_QSPI_IsEnabledIT_TXOVR
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledIT_TXOVR(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->IE, QSPI_IE_TX_OVR_EN) == (QSPI_IE_TX_OVR_EN));
}

/**
 * @brief  Enable Transfer Done Interrupt
 * @rmtoll IE          DONE_EN        LL_QSPI_EnableIT_DONE
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableIT_DONE(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->IE, QSPI_IE_DONE_EN);
}

/**
 * @brief  Disable Transfer Done Interrupt
 * @rmtoll IE          DONE_EN        LL_QSPI_DisableIT_DONE
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableIT_DONE(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->IE, QSPI_IE_DONE_EN);
}

/**
 * @brief  Check if the QSPI Transfer Done Interrupt is enabled or disabled.
 * @rmtoll IE          DONE_EN        LL_QSPI_IsEnabledIT_DONE
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledIT_DONE(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->IE, QSPI_IE_DONE_EN) == (QSPI_IE_DONE_EN));
}

/**
 * @brief  Enable Configuration Frame Error Interrupt
 * @rmtoll IE          CFG_ERR_EN        LL_QSPI_EnableIT_SCSNPOS
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableIT_CFGERR(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->IE, QSPI_IE_CFG_ERR_EN);
}

/**
 * @brief  Disable Configuration Frame Error Interrupt
 * @rmtoll IE          CFG_ERR_EN        LL_QSPI_DisableIT_CFGERR
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableIT_CFGERR(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->IE, QSPI_IE_CFG_ERR_EN);
}

/**
 * @brief  Check if the QSPI Configuration Frame Error Interrupt is enabled or disabled.
 * @rmtoll IE          CFG_ERR_EN        LL_QSPI_IsEnabledIT_CFGERR
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledIT_CFGERR(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->IE, QSPI_IE_CFG_ERR_EN) == (QSPI_IE_CFG_ERR_EN));
}

/**
 * @}
 */

/** @defgroup QSPI_LL_EF_FLAG_management FLAG_management
 * @{
 */

/**
 * @brief  Check if the QSPI Transmit FIFO Threshold Interrupt Flag is set or not (read only)
 * @rmtoll IP           TX_IRQ            LL_QSPI_IsActiveFlag_TX
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsActiveFlag_TX(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->IP, QSPI_IP_TX_IRQ) == (QSPI_IP_TX_IRQ));
}

/**
 * @brief  Check if the QSPI Receive FIFO Threshold Interrupt Flag is set or not (read only)
 * @rmtoll IP           RX_IRQ            LL_QSPI_IsActiveFlag_RX
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsActiveFlag_RX(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->IP, QSPI_IP_RX_IRQ) == (QSPI_IP_RX_IRQ));
}

/**
 * @brief  Check if the QSPI Transmit Complete Flag is set or not
 * @rmtoll STA           BUSY            LL_QSPI_IsActiveFlag_BUSY
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsActiveFlag_BUSY(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->STA, QSPI_STA_BUSY) == (QSPI_STA_BUSY));
}

/**
 * @brief  Check if the QSPI Receive FIFO Overflow Error Flag is set or not
 * @rmtoll STA           RX_OVR_ERR            LL_QSPI_IsActiveFlag_RXOVR
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsActiveFlag_RXOVR(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->STA, QSPI_STA_RX_OVR_ERR) == (QSPI_STA_RX_OVR_ERR));
}

/**
 * @brief  Clear Receive FIFO Overflow Error Flag
 * @rmtoll STA           RX_OVR_ERR            LL_QSPI_ClearFlag_RXOVR
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_ClearFlag_RXOVR(QSPI_TypeDef *QSPIx)
{
    WRITE_REG(QSPIx->STA, QSPI_STA_RX_OVR_ERR);
}

/**
 * @brief  Check if the QSPI Transmit FIFO Underflow Error Flag is set or not
 * @rmtoll STA           TX_UDR_ERR            LL_QSPI_IsActiveFlag_TXUDR
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsActiveFlag_TXUDR(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->STA, QSPI_STA_TX_UDR_ERR) == (QSPI_STA_TX_UDR_ERR));
}

/**
 * @brief  Clear Transmit FIFO Underflow Error Flag
 * @rmtoll STA           TX_UDR_ERR            LL_QSPI_ClearFlag_TXUDR
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_ClearFlag_TXUDR(QSPI_TypeDef *QSPIx)
{
    WRITE_REG(QSPIx->STA, QSPI_STA_TX_UDR_ERR);
}

/**
 * @brief  Check if the QSPI Transmit FIFO Full Flag is set or not (read only)
 * @rmtoll STA           TX_FULL            LL_QSPI_IsActiveFlag_TXFIFOFULL
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsActiveFlag_TXFIFOFULL(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->STA, QSPI_STA_TX_FULL) == (QSPI_STA_TX_FULL));
}

/**
 * @brief  Check if the QSPI Receive FIFO Empty Flag is set or not (read only)
 * @rmtoll STA           RX_EMPTY            LL_QSPI_IsActiveFlag_RXFIFOEMPTY
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE uint32_t LL_QSPI_IsActiveFlag_RXFIFOEMPTY(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->STA, QSPI_STA_RX_EMPTY) == (QSPI_STA_RX_EMPTY));
}

/**
 * @brief  Check if the QSPI Receive FIFO Underflow Error Flag is set or not
 * @rmtoll STA           RX_UDR_ERR            LL_QSPI_IsActiveFlag_RXUDR
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsActiveFlag_RXUDR(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->STA, QSPI_STA_RX_UDR_ERR) == (QSPI_STA_RX_UDR_ERR));
}

/**
 * @brief  Clear Receive FIFO Underflow Error Flag
 * @rmtoll STA           RX_UDR_ERR            LL_QSPI_ClearFlag_RXUDR
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_ClearFlag_RXUDR(QSPI_TypeDef *QSPIx)
{
    WRITE_REG(QSPIx->STA, QSPI_STA_RX_UDR_ERR);
}

/**
 * @brief  Check if the QSPI Transmit FIFO Overflow Error Flag is set or not
 * @rmtoll STA           TX_OVR_ERR            LL_QSPI_IsActiveFlag_TXOVR
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsActiveFlag_TXOVR(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->STA, QSPI_STA_TX_OVR_ERR) == (QSPI_STA_TX_OVR_ERR));
}

/**
 * @brief  Clear Transmit FIFO Overflow Error Flag
 * @rmtoll STA           TX_OVR_ERR            LL_QSPI_ClearFlag_TXOVR
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_ClearFlag_TXOVR(QSPI_TypeDef *QSPIx)
{
    WRITE_REG(QSPIx->STA, QSPI_STA_TX_OVR_ERR);
}

/**
 * @brief  Check if the QSPI Transmit FIFO Empty Flag is set or not (read only)
 * @rmtoll STA           TX_FULL            LL_QSPI_IsActiveFlag_TXFIFOEMPTY
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsActiveFlag_TXFIFOEMPTY(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->STA, QSPI_STA_TX_EMPTY) == (QSPI_STA_TX_EMPTY));
}

/**
 * @brief  Check if the QSPI Receive FIFO Full Flag is set or not (read only)
 * @rmtoll STA           RX_EMPTY            LL_QSPI_IsActiveFlag_RXFIFOFULL
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE uint32_t LL_QSPI_IsActiveFlag_RXFIFOFULL(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->STA, QSPI_STA_RX_FULL) == (QSPI_STA_RX_FULL));
}

/**
 * @brief  Check if the QSPI Transfer Done Flag is set or not
 * @rmtoll STA           DONE            LL_QSPI_IsActiveFlag_DONE
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsActiveFlag_DONE(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->STA, QSPI_STA_DONE) == (QSPI_STA_DONE));
}

/**
 * @brief  Clear Transfer Done Flag
 * @rmtoll STA           DONE            LL_QSPI_ClearFlag_DONE
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_ClearFlag_DONE(QSPI_TypeDef *QSPIx)
{
    WRITE_REG(QSPIx->STA, QSPI_STA_DONE);
}

/**
 * @brief  Check if the QSPI Frame Configuration Error Flag is set or not
 * @rmtoll STA           CFG_ERR            LL_QSPI_IsActiveFlag_CFGERR
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_DMA_IsActiveFlag_CFGERR(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->STA, QSPI_STA_CFG_ERR) == (QSPI_STA_CFG_ERR));
}

/**
 * @brief  Clear Frame Configuration Error Flag
 * @rmtoll STA           CFG_ERR            LL_QSPI_ClearFlag_CFGERR
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DMA_ClearFlag_CFGERR(QSPI_TypeDef *QSPIx)
{
    WRITE_REG(QSPIx->STA, QSPI_STA_CFG_ERR);
}

/**
 * @}
 */

/** @defgroup QSPI_LL_EF_DMA_Management DMA Management
 * @{
 */

/**
 * @brief  Configure DMA Transfer Data Length (in beats).
 * @rmtoll TSIZE          TSIZE            LL_QSPI_DMA_SetTransmitDataLength
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFFFFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DMA_SetTransmitDataLength(QSPI_TypeDef *QSPIx, uint32_t DataLength)
{
    MODIFY_REG(QSPIx->TSIZE, QSPI_TSIZE_TSIZE, DataLength);
}

/**
 * @brief  Return DMA Transfer Data Length (in beats) configuration
 * @rmtoll TSIZE          TSIZE            LL_QSPI_DMA_GetTransmitDataLength
 * @param  QSPIx QSPI Instance
 * @retval Value between Min_Data = 0x00 and Max_Data = 0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_QSPI_DMA_GetTransmitDataLength(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->TSIZE, QSPI_TSIZE_TSIZE));
}

/**
 * @brief  Configure DMA Receive Data Length (in beats).
 * @rmtoll RSIZE          RSIZE            LL_QSPI_DMA_SetReceiveDataLength
 * @param  QSPIx QSPI Instance
 * @param  This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFFFFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DMA_SetReceiveDataLength(QSPI_TypeDef *QSPIx, uint32_t DataLength)
{
    MODIFY_REG(QSPIx->RSIZE, QSPI_RSIZE_RSIZE, DataLength);
}

/**
 * @brief  Return DMA Receive Data Length (in beats). configuration
 * @rmtoll RSIZE          RSIZE            LL_QSPI_DMA_GetReceiveDataLength
 * @param  QSPIx QSPI Instance
 * @retval Value between Min_Data = 0x00 and Max_Data = 0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_QSPI_DMA_GetReceiveDataLength(QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_BIT(QSPIx->RSIZE, QSPI_RSIZE_RSIZE));
}

/**
 * @brief  Enable Transmit DMA Mode
 * @rmtoll CR          DMA_TX_EN           LL_QSPI_EnableDMAReq_TX
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableDMAReq_TX(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->CR, QSPI_CR_DMA_TX_EN);
}

/**
 * @brief  Disable Transmit DMA Mode
 * @rmtoll CR          DMA_TX_EN           LL_QSPI_DisableDMAReq_TX
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableDMAReq_TX(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->CR, QSPI_CR_DMA_TX_EN);
}

/**
 * @brief  Check if Transmit DMA Mode is enabled
 * @rmtoll CR          DMA_TX_EN           LL_QSPI_IsEnabledDMAReq_TX
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledDMAReq_TX(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->CR, QSPI_CR_DMA_TX_EN) == (QSPI_CR_DMA_TX_EN));
}

/**
 * @brief  Enable Receive DMA Mode
 * @rmtoll CR          DMA_RX_EN           LL_QSPI_EnableDMAReq_RX
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableDMAReq_RX(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->CR, QSPI_CR_DMA_RX_EN);
}

/**
 * @brief  Disable Receive DMA Mode
 * @rmtoll CR          DMA_RX_EN           LL_QSPI_DisableDMAReq_RX
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableDMAReq_RX(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->CR, QSPI_CR_DMA_RX_EN);
}

/**
 * @brief  Check if Receive DMA Mode is enabled
 * @rmtoll CR          DMA_RX_EN           LL_QSPI_IsEnabledDMAReq_RX
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledDMAReq_RX(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->CR, QSPI_CR_DMA_RX_EN) == (QSPI_CR_DMA_RX_EN));
}

/**
 * @brief  Enable DMA Continuous Receive Mode
 * @rmtoll CR          DMA_RXCT_MODE           LL_QSPI_DMA_EnableRxCircularMode
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DMA_EnableRxCircularMode(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->CR, QSPI_CR_DMA_RXCT_MODE);
}

/**
 * @brief  Disable DMA Continuous Receive Mode
 * @rmtoll CR          DMA_RXCT_MODE           LL_QSPI_DMA_DisableRxCircularMode
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DMA_DisableRxCircularMode(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->CR, QSPI_CR_DMA_RXCT_MODE);
}

/**
 * @brief  Check if DMA Continuous Receive Mode is enabled
 * @rmtoll CR          DMA_RXCT_MODE           LL_QSPI_DMA_IsEnabledRxCircularMode
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_DMA_IsEnabledRxCircularMode(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->CR, QSPI_CR_DMA_RXCT_MODE) == (QSPI_CR_DMA_RXCT_MODE));
}

/**
 * @brief  Enable DMA Continuous Transmit Mode
 * @rmtoll CR          DMA_TXCT_MODE           LL_QSPI_DMA_EnableRxCircularMode
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DMA_EnableTxCircularMode(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->CR, QSPI_CR_DMA_TXCT_MODE);
}

/**
 * @brief  Disable DMA Continuous Transmit Mode
 * @rmtoll CR          DMA_TXCT_MODE           LL_QSPI_DMA_DisableRxCircularMode
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DMA_DisableTxCircularMode(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->CR, QSPI_CR_DMA_TXCT_MODE);
}

/**
 * @brief  Check if DMA Continuous Transmit Mode is enabled
 * @rmtoll CR          DMA_TXCT_MODE           LL_QSPI_DMA_IsEnabledRxCircularMode
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_DMA_IsEnabledTxCircularMode(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->CR, QSPI_CR_DMA_TXCT_MODE) == (QSPI_CR_DMA_TXCT_MODE));
}

/**
 * @brief  Enable DMA Continuous Transmit Mode
 * @rmtoll CR          SPI_RXFIFO_EN           LL_QSPI_EnableRxFIFO
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_EnableRxFIFO(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->CR, QSPI_CR_SPI_RXFIFO_EN);
}

/**
 * @brief  Disable DMA Continuous Transmit Mode
 * @rmtoll CR          SPI_RXFIFO_EN           LL_QSPI_DisableRxFIFO
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DisableRxFIFO(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->CR, QSPI_CR_SPI_RXFIFO_EN);
}

/**
 * @brief  Check if DMA Continuous Transmit Mode is enabled
 * @rmtoll CR          SPI_RXFIFO_EN           LL_QSPI_IsEnabledRxFIFO
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_IsEnabledRxFIFO(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->CR, QSPI_CR_SPI_RXFIFO_EN) == (QSPI_CR_SPI_RXFIFO_EN));
}

/**
 * @}
 */

/** @defgroup QSPI_LL_EF_DMA_IT_Management DMA_IT_Management
 * @{
 */

/**
 * @brief  Enable Transmit Done Interrupt (DMA).
 * @rmtoll IE          TX_DONE_EN        LL_QSPI_EnableIT_TXDONE
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DMA_EnableIT_TXDONE(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->IE, QSPI_IE_TX_DONE_EN);
}

/**
 * @brief  Disable Transmit Done Interrupt (DMA).
 * @rmtoll IE          TX_DONE_EN        LL_QSPI_DisableIT_TXDONE
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DMA_DisableIT_TXDONE(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->IE, QSPI_IE_TX_DONE_EN);
}

/**
 * @brief  Check if the QSPI Transmit Done Interrupt is enabled or disabled.
 * @rmtoll IE          TX_DONE_EN        LL_QSPI_IsEnabledIT_TXDONE
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_DMA_IsEnabledIT_TXDONE(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->IE, QSPI_IE_TX_DONE_EN) == (QSPI_IE_TX_DONE_EN));
}

/**
 * @brief  Enable Receive Done Interrupt (DMA).
 * @rmtoll IE          RX_DONE_EN        LL_QSPI_EnableIT_RXDONE
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DMA_EnableIT_RXDONE(QSPI_TypeDef *QSPIx)
{
    SET_BIT(QSPIx->IE, QSPI_IE_RX_DONE_EN);
}

/**
 * @brief  Disable Receive Done Interrupt (DMA).
 * @rmtoll IE          RX_DONE_EN        LL_QSPI_DisableIT_RXDONE
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DMA_DisableIT_RXDONE(QSPI_TypeDef *QSPIx)
{
    CLEAR_BIT(QSPIx->IE, QSPI_IE_RX_DONE_EN);
}

/**
 * @brief  Check if the QSPI Receive Done Interrupt is enabled or disabled.
 * @rmtoll IE          RX_DONE_EN        LL_QSPI_IsEnabledIT_RXDONE
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_DMA_IsEnabledIT_RXDONE(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->IE, QSPI_IE_RX_DONE_EN) == (QSPI_IE_RX_DONE_EN));
}

/**
 * @}
 */

/** @defgroup QSPI_LL_EF_DMA_FLAG_management DMA_FLAG_management
 * @{
 */

/**
 * @brief  Check if the QSPI DMA Transmit Done Flag is set or not
 * @rmtoll STA           TX_DONE            LL_QSPI_DMA_IsActiveFlag_TXDONE
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_DMA_IsActiveFlag_TXDONE(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->STA, QSPI_STA_TX_DONE) == (QSPI_STA_TX_DONE));
}

/**
 * @brief  Clear DMA Transmit Done Flag
 * @rmtoll STA           TX_DONE            LL_QSPI_DMA_ClearFlag_TXDONE
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DMA_ClearFlag_TXDONE(QSPI_TypeDef *QSPIx)
{
    WRITE_REG(QSPIx->STA, QSPI_STA_TX_DONE);
}

/**
 * @brief  Check if the QSPI DMA Receive Done Flag is set or not
 * @rmtoll STA           RX_DONE            LL_QSPI_DMA_IsActiveFlag_RXDONE
 * @param  QSPIx QSPI Instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_QSPI_DMA_IsActiveFlag_RXDONE(const QSPI_TypeDef *QSPIx)
{
    return (READ_BIT(QSPIx->STA, QSPI_STA_RX_DONE) == (QSPI_STA_RX_DONE));
}

/**
 * @brief  Clear DMA Receive Done Flag
 * @rmtoll STA           RX_DONE            LL_QSPI_DMA_ClearFlag_RXDONE
 * @param  QSPIx QSPI Instance
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_DMA_ClearFlag_RXDONE(QSPI_TypeDef *QSPIx)
{
    WRITE_REG(QSPIx->STA, QSPI_STA_RX_DONE);
}

/**
 * @}
 */

/** @defgroup QSPI_LL_EF_Data_Management Data_Management
 * @{
 */

/**
 * @brief  Read 8-Bits in the data register
 * @rmtoll RXDATA           RXDATA            LL_QSPI_ReceiveData8
 * @param  QSPIx QSPI Instance
 * @retval RxData Value between Min_Data=0x00 and Max_Data=0xFF
 */
__STATIC_INLINE uint8_t LL_QSPI_ReceiveData8(const QSPI_TypeDef *QSPIx)
{
    return (uint8_t)(QSPIx->RXDATA & 0xF);
}

/**
 * @brief  Read 16-Bits in the data register
 * @rmtoll RXDATA           RXDATA            LL_QSPI_ReceiveData16
 * @param  QSPIx QSPI Instance
 * @retval RxData Value between Min_Data=0x00 and Max_Data=0xFFFF
 */
__STATIC_INLINE uint16_t LL_QSPI_ReceiveData16(const QSPI_TypeDef *QSPIx)
{
    return (uint16_t)(QSPIx->RXDATA & 0xFF);
}

/**
 * @brief  Read 32-Bits in the data register
 * @rmtoll RXDATA           RXDATA            LL_QSPI_ReceiveData32
 * @param  QSPIx QSPI Instance
 * @retval RxData Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF
 */
__STATIC_INLINE uint32_t LL_QSPI_ReceiveData32(const QSPI_TypeDef *QSPIx)
{
    return (uint32_t)(READ_REG(QSPIx->RXDATA));
}

/**
 * @brief  Write 8-Bits in the data register
 * @rmtoll TXDATA           TXDATA            LL_QSPI_TransmitData8
 * @param  QSPIx QSPI Instance
 * @param  TxData Value between Min_Data=0x00 and Max_Data=0xFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_TransmitData8(QSPI_TypeDef *QSPIx, uint8_t TxData)
{
    QSPIx->TXDATA = TxData;
}

/**
 * @brief  Write 16-Bits in the data register
 * @rmtoll TXDATA           TXDATA            LL_QSPI_TransmitData16
 * @param  QSPIx QSPI Instance
 * @param  TxData Value between Min_Data=0x00 and Max_Data=0xFFFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_TransmitData16(QSPI_TypeDef *QSPIx, uint16_t TxData)
{
    QSPIx->TXDATA = TxData;
}

/**
 * @brief  Write 32-Bits in the data register
 * @rmtoll TXDATA           TXDATA            LL_QSPI_TransmitData32
 * @param  QSPIx QSPI Instance
 * @param  TxData Value between Min_Data=0x00 and Max_Data=0xFFFFFFFF
 * @retval None
 */
__STATIC_INLINE void LL_QSPI_TransmitData32(QSPI_TypeDef *QSPIx, uint32_t TxData)
{
    QSPIx->TXDATA = TxData;
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup QSPI_LL_EF_Init Initialization and de-initialization functions
 * @{
 */

ErrorStatus LL_QSPI_Init(QSPI_TypeDef *QSPIx, const LL_QSPI_InitTypeDef *QSPI_InitStruct);
ErrorStatus LL_QSPI_DeInit(QSPI_TypeDef *QSPIx);
void        LL_QSPI_StructInit(LL_QSPI_InitTypeDef *QSPI_InitStruct); // If the module is not suitable for the default configuration, you could not defines this function.

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

#endif /* QSPI0 || QSPI1 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_QSPI_H */
