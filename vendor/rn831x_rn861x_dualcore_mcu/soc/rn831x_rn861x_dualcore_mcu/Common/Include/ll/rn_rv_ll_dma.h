/**
 *****************************************************************************
 * @file   rn_rv_ll_dma.h
 * @author Renergy Software Team
 * @brief  Header file of DMA LL module.
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
#ifndef __RN_LL_DMA_H
#define __RN_LL_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "rn831x_rn861x_mcu_dma.h"

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (DMA00) || defined (DMA10) || defined (DMA01) || defined (DMA11)

/** @defgroup DMA_LL DMA
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/** @defgroup DMA_Private_Constants DMA Private Constants
 * @{
 */

/** @defgroup DMA_LL_PC_NDTR_NDT NDTR Managment
 * @brief     NDTR defines which can be used with LL_DMA_SetDataLength and LL_DMA_GetDataLength functions.
 * @{
 */
#define LL_DMA_NDTR_SRAM_MASK     DMA_NDTR_NDTLOW                       /*!< When channel could access SRAM.*/
#define LL_DMA_NDTR_FLASH_MASK    (DMA_NDTR_NDTLOW | DMA_NDTR_NDTHIGH) /*!< When channel could access Flash.*/
/**
 * @}
 */

/** @defgroup DMA_LL_PC_MADR Memory Address Managment
 * @brief     MADR defines which can be used with LL_DMA_SetMemoryAddress and LL_DMA_GetMemoryAddress functions.
 * @{
 */
#define LL_DMA_MADR_SRAM_MASK     (DMA_MADR_ADDRLOW | DMA_MADR_ADDRVERYHIGH) /*!< When channel could access SRAM.*/
#define LL_DMA_MADR_FLASH_MASK    (DMA_MADR_ADDRLOW | DMA_PADR_ADDRHIGH | \
                                   DMA_MADR_ADDRVERYHIGH)                    /*!< When channel could access Flash.*/
/**
 * @}
 */

/** @defgroup DMA_LL_PC_PADR Periph Address Managment
 * @brief     PADR defines which can be used with LL_DMA_SetPeriphAddress and LL_DMA_GetPeriphAddress functions.
 * @{
 */
#define LL_DMA_PADR_SRAM_MASK     (DMA_PADR_ADDRLOW | DMA_PADR_ADDRHIGH)   /*!< When channel could access SRAM.*/
#define LL_DMA_PADR_FLASH_MASK    (DMA_PADR_ADDRLOW | DMA_PADR_ADDRHIGH)   /*!< When channel could access Flash.*/
/**
 * @}
 */

/** @defgroup DMA_LL_PC_MSADR Memory Source Address Managment
 * @brief     MSADR defines which can be used with LL_DMA_SetM2MSrcAddress and LL_DMA_GetM2MSrcAddress functions.
 * @{
 */
#define LL_DMA_MSADR_SRAM_MASK    (DMA_MSADR_ADDRLOW | DMA_MSADR_ADDRHIGH) /*!< When channel could access SRAM.*/
#define LL_DMA_MSADR_FLASH_MASK   (DMA_MSADR_ADDRLOW | DMA_MSADR_ADDRHIGH) /*!< When channel could access Flash.*/
/**
 * @}
 */

/** @defgroup DMA_LL_PC_MDADR Memory Destination Address Managment
 * @brief     MDADR defines which can be used with LL_DMA_SetM2MDstAddress and LL_DMA_GetM2MDstAddress functions.
 * @{
 */
#define LL_DMA_MDADR_SRAM_MASK    (DMA_MDADR_ADDRLOW | DMA_MDADR_ADDRHIGH) /*!< When channel could access SRAM.*/
#define LL_DMA_MDADR_FLASH_MASK   (DMA_MDADR_ADDRLOW | DMA_MDADR_ADDRHIGH) /*!< When channel could access Flash.*/
/**
 * @}
 */

/**
 * @}
 */

/* Private macros -----------------------------------------------------------*/
/** @defgroup DMA_LL_Private_Macros DMA Private Macros
 * @{
 */

/**
 * @}
 */

/* Exported types -----------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup DMA_LL_ES_INIT DMA Exported Init structure
 * @{
 */

/**
 * @brief Structure definition of some features of DMA common parameters.
 */
typedef struct
{
    uint32_t PeriphOrM2MSrcAddress;  /*!< Specifies the peripheral base address for DMA transfer
                                        or as Source base address in case of memory to memory transfer direction.

                                        This parameter must be a value between Min_Data = 0 and Max_Data = 0xFFFFFFFF. */

    uint32_t MemoryOrM2MDstAddress;  /*!< Specifies the memory base address for DMA transfer
                                        or as Destination base address in case of memory to memory transfer direction.

                                        This parameter must be a value between Min_Data = 0 and Max_Data = 0xFFFFFFFF. */

    uint32_t Mode;                   /*!< Specifies the normal or circular operation mode.
                                        This parameter can be a value of @ref DMA_LL_EC_MODE
                                        @note: The circular buffer mode cannot be used if the memory to memory
                                            data transfer direction is configured on the selected Channel

                                        This feature can be modified afterwards using unitary function @ref LL_DMA_SetMode(). */

    uint32_t Direction;              /*!< Specifies if the data will be transferred from memory to peripheral,
                                        from memory to memory or from peripheral to memory.
                                        This parameter can be a value of @ref DMA_LL_EC_DIRECTION

                                        This feature can be modified afterwards using unitary function @ref LL_DMA_SetDataTransferDirection(). */

    uint32_t PeriphDataAlignment;    /*!< Specifies the Peripheral data size alignment or Source data size alignment (byte, half word, word)
                                        in case of memory to memory transfer direction.
                                        This parameter can be a value of @ref DMA_LL_EC_SRCDATAALIGN

                                        This feature can be modified afterwards using unitary function @ref LL_DMA_SetM2MDstMemorySize(). */

    uint32_t MemDataAlignment;       /*!< Specifies the Memory data size alignment or Destination data size alignment (byte, half word, word)
                                        in case of memory to memory transfer direction.
                                        This parameter can be a value of @ref DMA_LL_EC_DSTDATAALIGN

                                        This feature can be modified afterwards using unitary function @ref LL_DMA_SetMemorySize(). */

    uint32_t NbData;                 /*!< Specifies the number of data to transfer, in data unit.
                                        The data unit is equal to the source buffer configuration set in PeripheralSize
                                        or MemorySize parameters depending in the transfer direction.
                                        This parameter must be a value between Min_Data = 0 and Max_Data = 0x0000FFFF

                                        This feature can be modified afterwards using unitary function @ref LL_DMA_SetDataLength(). */

    uint32_t Priority;               /*!< Specifies the channel priority level.
                                        This parameter can be a value of @ref DMA_LL_EC_PRIORITY

                                        This feature can be modified afterwards using unitary function @ref LL_DMA_SetChannelPriorityLevel(). */

} LL_DMA_InitTypeDef;

/**
 * @}
 */
#endif

/* Exported constants -------------------------------------------------------*/
/** @defgroup DMA_LL_Exported_Constants DMA Exported Constants
 * @{
 */

/** @defgroup DMA_LL_EC_GET_FLAG Get Flags Defines
 * @brief     Flags defines which can be used with LL_DMA_ReadReg function
 * @{
 */
#define LL_DMA_IF_HTIF0                   DMA_IF_HTIF0         /*!< Channel 0 half transfer flag     */
#define LL_DMA_IF_TCIF0                   DMA_IF_TCIF0         /*!< Channel 0 transfer complete flag */
#define LL_DMA_IF_TEIF0                   DMA_IF_TEIF0         /*!< Channel 0 transfer error flag    */
#define LL_DMA_IF_HTIF1                   DMA_IF_HTIF1         /*!< Channel 1 half transfer flag     */
#define LL_DMA_IF_TCIF1                   DMA_IF_TCIF1         /*!< Channel 1 transfer complete flag */
#define LL_DMA_IF_TEIF1                   DMA_IF_TEIF1         /*!< Channel 1 transfer error flag    */
#define LL_DMA_IF_HTIF2                   DMA_IF_HTIF2         /*!< Channel 2 half transfer flag     */
#define LL_DMA_IF_TCIF2                   DMA_IF_TCIF2         /*!< Channel 2 transfer complete flag */
#define LL_DMA_IF_TEIF2                   DMA_IF_TEIF2         /*!< Channel 2 transfer error flag    */
#define LL_DMA_IF_HTIF3                   DMA_IF_HTIF3         /*!< Channel 3 half transfer flag     */
#define LL_DMA_IF_TCIF3                   DMA_IF_TCIF3         /*!< Channel 3 transfer complete flag */
#define LL_DMA_IF_TEIF3                   DMA_IF_TEIF3         /*!< Channel 3 transfer error flag    */
#define LL_DMA_IF_HTIF4                   DMA_IF_HTIF4         /*!< Channel 4 half transfer flag     */
#define LL_DMA_IF_TCIF4                   DMA_IF_TCIF4         /*!< Channel 4 transfer complete flag */
#define LL_DMA_IF_TEIF4                   DMA_IF_TEIF4         /*!< Channel 4 transfer error flag    */

#define LL_DMA_MIF_HTIF0                  DMA_MIF_HTIF0         /*!< Channel 0 half transfer flag     */
#define LL_DMA_MIF_TCIF0                  DMA_MIF_TCIF0         /*!< Channel 0 transfer complete flag */
#define LL_DMA_MIF_TEIF0                  DMA_MIF_TEIF0         /*!< Channel 0 transfer error flag    */
#define LL_DMA_MIF_HTIF1                  DMA_MIF_HTIF1         /*!< Channel 1 half transfer flag     */
#define LL_DMA_MIF_TCIF1                  DMA_MIF_TCIF1         /*!< Channel 1 transfer complete flag */
#define LL_DMA_MIF_TEIF1                  DMA_MIF_TEIF1         /*!< Channel 1 transfer error flag    */
/**
 * @}
 */

/** @defgroup DMA_LL_EC_IT IT Defines
 * @brief     IT defines which can be used with LL_DMA_ReadReg and LL_DMA_WriteReg functions
 * @{
 */
#define LL_DMA_IE_HTIE0                   DMA_IE_HTIE0          /*!< Channel 0 Half Transfer interrupt     */
#define LL_DMA_IE_TCIE0                   DMA_IE_TCIE0          /*!< Channel 0 Transfer complete interrupt */
#define LL_DMA_IE_TEIE0                   DMA_IE_TEIE0          /*!< Channel 0 Transfer error interrupt    */
#define LL_DMA_IE_HTIE1                   DMA_IE_HTIE1          /*!< Channel 1 Half Transfer interrupt     */
#define LL_DMA_IE_TCIE1                   DMA_IE_TCIE1          /*!< Channel 1 Transfer complete interrupt */
#define LL_DMA_IE_TEIE1                   DMA_IE_TEIE1          /*!< Channel 1 Transfer error interrupt    */
#define LL_DMA_IE_HTIE2                   DMA_IE_HTIE2          /*!< Channel 2 Half Transfer interrupt     */
#define LL_DMA_IE_TCIE2                   DMA_IE_TCIE2          /*!< Channel 2 Transfer complete interrupt */
#define LL_DMA_IE_TEIE2                   DMA_IE_TEIE2          /*!< Channel 2 Transfer error interrupt    */
#define LL_DMA_IE_HTIE3                   DMA_IE_HTIE3          /*!< Channel 3 Half Transfer interrupt     */
#define LL_DMA_IE_TCIE3                   DMA_IE_TCIE3          /*!< Channel 3 Transfer complete interrupt */
#define LL_DMA_IE_TEIE3                   DMA_IE_TEIE3          /*!< Channel 3 Transfer error interrupt    */
#define LL_DMA_IE_HTIE4                   DMA_IE_HTIE4          /*!< Channel 4 Half Transfer interrupt     */
#define LL_DMA_IE_TCIE4                   DMA_IE_TCIE4          /*!< Channel 4 Transfer complete interrupt */
#define LL_DMA_IE_TEIE4                   DMA_IE_TEIE4          /*!< Channel 4 Transfer error interrupt    */

#define LL_DMA_MIE_HTIE0                  DMA_MIE_HTIE0         /*!< Channel 5 Half Transfer interrupt     */
#define LL_DMA_MIE_TCIE0                  DMA_MIE_TCIE0         /*!< Channel 5 Transfer complete interrupt */
#define LL_DMA_MIE_TEIE0                  DMA_MIE_TEIE0         /*!< Channel 5 Transfer error interrupt    */
#define LL_DMA_MIE_HTIE1                  DMA_MIE_HTIE1         /*!< Channel 6 Half Transfer interrupt     */
#define LL_DMA_MIE_TCIE1                  DMA_MIE_TCIE1         /*!< Channel 6 Transfer complete interrupt */
#define LL_DMA_MIE_TEIE1                  DMA_MIE_TEIE1         /*!< Channel 6 Transfer error interrupt    */
/**
 * @}
 */

/** @defgroup DMA_LL_EC_CHANNEL CHANNEL
 * @brief     DMA00 and DMA10 support LL_DMA_CHANNEL_0-4.
 *            DMA01 and DMA11 support LL_DMA_CHANNEL_0-3 and LL_DMA_CHANNEL_5-6.
 *            LL_DMA_CHANNEL_0-3 only support accessing sram.
 *            LL_DMA_CHANNEL_4-6 support accessing flash or sram.
 * @{
 */
#define LL_DMA_CHANNEL_0                  0x00000000U           /*!< DMA Channel 0 */
#define LL_DMA_CHANNEL_1                  0x00000001U           /*!< DMA Channel 1 */
#define LL_DMA_CHANNEL_2                  0x00000002U           /*!< DMA Channel 2 */
#define LL_DMA_CHANNEL_3                  0x00000003U           /*!< DMA Channel 3 */
#define LL_DMA_CHANNEL_4                  0x00000004U           /*!< DMA Channel 4 */
#define LL_DMA_CHANNEL_5                  0x00000005U           /*!< DMA Channel 5 == DMA M2M Channel 0 */
#define LL_DMA_CHANNEL_6                  0x00000006U           /*!< DMA Channel 6 == DMA M2M Channel 1 */
/**
 * @}
 */

/** @defgroup DMA_LL_EC_DIRECTION Transfer Direction
 * @brief     DIRECTION defines which can be used with LL_DMA_SetDataTransferDirection and LL_DMA_GetDataTransferDirection functions.
 * @{
 */
#define LL_DMA_DIRECTION_PERIPH2MEMORY    0x00000000U                /*!< Peripheral to memory direction. */
#define LL_DMA_DIRECTION_MEMORY2PERIPH    DMA_CTRL_DIR               /*!< Memory to peripheral direction. */
/**
 * @}
 */

/** @defgroup DMA_LL_EC_MODE Transfer mode
 * @brief     MODE defines which can be used with LL_DMA_SetMode and LL_DMA_GetMode functions.
 * @{
 */
#define LL_DMA_MODE_NORMAL                0x00000000U                /*!< Normal Mode.*/
#define LL_DMA_MODE_CIRCULAR              DMA_CTRL_CIRC              /*!< Circular Mode.*/
/**
 * @}
 */

/** @defgroup DMA_LL_EC_DATAALIGN Memory data alignment
 * @brief     DATAALIGN defines which can be used with LL_DMA_SetMemorySize and LL_DMA_GetMemorySize functions.
 * @{
 */
#define LL_DMA_DATAALIGN_BYTE             0x00000000U               /*!< Memory data alignment : Byte.*/
#define LL_DMA_DATAALIGN_HALFWORD         DMA_CTRL_MSIZE_HWORD      /*!< Memory data alignment : HalfWord.*/
#define LL_DMA_DATAALIGN_WORD             DMA_CTRL_MSIZE_WORD       /*!< Memory data alignment : Word.*/
/**
 * @}
 */

/** @defgroup DMA_LL_EC_SRCDATAALIGN Source data alignment
 * @{
 */
#define LL_DMA_SRCDATAALIGN_BYTE          0x00000000U               /*!< Source data alignment : Byte     */
#define LL_DMA_SRCDATAALIGN_HALFWORD      DMA_MCTRL_SSIZE_HWORD     /*!< Source data alignment : HalfWord */
#define LL_DMA_SRCDATAALIGN_WORD          DMA_MCTRL_SSIZE_WORD      /*!< Source data alignment : Word     */
/**
 * @}
 */

/** @defgroup DMA_LL_EC_DSTDATAALIGN Destination data alignment
 * @{
 */
#define LL_DMA_DSTDATAALIGN_BYTE          0x00000000U               /*!< Destination data alignment : Byte     */
#define LL_DMA_DSTDATAALIGN_HALFWORD      DMA_MCTRL_DSIZE_HWORD     /*!< Destination data alignment : HalfWord */
#define LL_DMA_DSTDATAALIGN_WORD          DMA_MCTRL_DSIZE_WORD      /*!< Destination data alignment : Word     */
/**
 * @}
 */

/** @defgroup DMA_LL_EC_PRIORITY Transfer Priority level
 * @brief     PRIORITY defines which can be used with LL_DMA_SetChannelPriorityLevel and LL_DMA_GetChannelPriorityLevel functions.
 * @{
 */
#define LL_DMA_PRIORITY_LOW               0x00000000U               /*!< Priority level : Low.*/
#define LL_DMA_PRIORITY_MEDIUM            DMA_CTRL_PL_MEDIUM        /*!< Priority level : Medium.*/
#define LL_DMA_PRIORITY_HIGH              DMA_CTRL_PL_HIGH          /*!< Priority level : High.*/
#define LL_DMA_PRIORITY_VERYHIGH          DMA_CTRL_PL_VERYHIGH      /*!< Priority level : Very_High.*/
/**
 * @}
 */

/**
 * @}
 */

/* Exported macro -----------------------------------------------------------*/
/** @defgroup DMA_LL_Exported_Macros DMA Exported Macros
 * @{
 */

/** @defgroup DMA_LL_EM_WRITE_READ Common Write and read registers macros
 * @{
 */
/**
 * @brief  Write a value in DMA register
 * @param  __INSTANCE__ DMA Instance
 * @param  __REG__ Register to be written
 * @param  __VALUE__ Value to be written in the register
 * @retval None
 */
#define LL_DMA_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
 * @brief  Read a value in DMA register
 * @param  __INSTANCE__ DMA Instance
 * @param  __REG__ Register to be read
 * @retval Register value
 */
#define LL_DMA_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
 * @}
 */

/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup DMA_LL_Exported_Functions DMA Exported Functions
 * @{
 */

/** @defgroup DMA_LL_EF_Configuration Configuration
 * @{
 */

/**
 * @brief  Enable DMA Channel.
 * @rmtoll CTRL         EN         LL_DMA_EnableChannel
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableChannel(DMA_TypeDef *DMAx, uint32_t Channel)
{
    SET_BIT(DMAx->CH0[Channel].CTRL, DMA_CTRL_EN);
}

/**
 * @brief  Disable DMA Channel.
 * @rmtoll CTRL         EN         LL_DMA_DisableChannel
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableChannel(DMA_TypeDef *DMAx, uint32_t Channel)
{
    CLEAR_BIT(DMAx->CH0[Channel].CTRL, DMA_CTRL_EN);
}

/**
 * @brief  Check if DMA Channel is enabled or disabled.
 * @rmtoll CTRL         EN         LL_DMA_IsEnabledChannel
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledChannel(DMA_TypeDef *DMAx, uint32_t Channel)
{
    return ((READ_BIT(DMAx->CH0[Channel].CTRL, DMA_CTRL_EN) == DMA_CTRL_EN) ? 1UL : 0UL);
}

/**
 * @brief  Configure all default parameters linked to DMA transfer.
 * @rmtoll CTRL         CIRC         LL_DMA_ConfigTransfer
 *         CTRL         PL           LL_DMA_ConfigTransfer
 *         CTRL         MSIZE        LL_DMA_ConfigTransfer
 *         CTRL         DIR          LL_DMA_ConfigTransfer
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @param  Configuration This parameter must be a combination of all the following values:
 *         @arg @ref LL_DMA_DIRECTION_PERIPH2MEMORY or @ref LL_DMA_DIRECTION_MEMORY2PERIPH
 *         @arg @ref LL_DMA_MODE_NORMAL or @ref LL_DMA_MODE_CIRCULAR
 *         @arg @ref LL_DMA_DATAALIGN_BYTE or @ref LL_DMA_DATAALIGN_HALFWORD or @ref LL_DMA_DATAALIGN_WORD
 *         @arg @ref LL_DMA_PRIORITY_LOW or @ref LL_DMA_PRIORITY_MEDIUM or @ref LL_DMA_PRIORITY_HIGH or @ref LL_DMA_PRIORITY_VERYHIGH
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ConfigTransfer(DMA_TypeDef *DMAx, uint32_t Channel, uint32_t Configuration)
{
    MODIFY_REG(DMAx->CH0[Channel].CTRL, (DMA_CTRL_CIRC | DMA_CTRL_PL | \
                DMA_CTRL_MSIZE | DMA_CTRL_DIR), Configuration);
}

/**
 * @brief  Set Data transfer direction (read from peripheral or from memory).
 * @rmtoll CTRL         DIR         LL_DMA_SetDataTransferDirection
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @param  Direction This parameter can be one of the following values:
 *         @arg @ref LL_DMA_DIRECTION_PERIPH2MEMORY
 *         @arg @ref LL_DMA_DIRECTION_MEMORY2PERIPH
 * @retval None
 */
__STATIC_INLINE void LL_DMA_SetDataTransferDirection(DMA_TypeDef *DMAx, uint32_t Channel, uint32_t Direction)
{
    MODIFY_REG(DMAx->CH0[Channel].CTRL, DMA_CTRL_DIR, Direction);
}

/**
 * @brief  Get Data transfer direction (read from peripheral or from memory).
 * @rmtoll CTRL         DIR         LL_DMA_GetDataTransferDirection
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DMA_DIRECTION_PERIPH2MEMORY
 *         @arg @ref LL_DMA_DIRECTION_MEMORY2PERIPH
 */
__STATIC_INLINE uint32_t LL_DMA_GetDataTransferDirection(DMA_TypeDef *DMAx, uint32_t Channel)
{
    return (READ_BIT(DMAx->CH0[Channel].CTRL, DMA_CTRL_DIR));
}

/**
 * @brief  Set DMA mode normal, circular or peripheral flow control.
 * @rmtoll CTRL         CIRC         LL_DMA_SetMode
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @param  Mode This parameter can be one of the following values:
 *         @arg @ref LL_DMA_MODE_NORMAL
 *         @arg @ref LL_DMA_MODE_CIRCULAR
 * @retval None
 */
__STATIC_INLINE void LL_DMA_SetMode(DMA_TypeDef *DMAx, uint32_t Channel, uint32_t Mode)
{
    MODIFY_REG(DMAx->CH0[Channel].CTRL, DMA_CTRL_CIRC, Mode);
}

/**
 * @brief  Get DMA mode normal, circular or peripheral flow control.
 * @rmtoll CTRL         CIRC         LL_DMA_GetMode
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DMA_MODE_NORMAL
 *         @arg @ref LL_DMA_MODE_CIRCULAR
 */
__STATIC_INLINE uint32_t LL_DMA_GetMode(DMA_TypeDef *DMAx, uint32_t Channel)
{
    return (READ_BIT(DMAx->CH0[Channel].CTRL, DMA_CTRL_CIRC));
}

/**
 * @brief  Set Memory size.
 * @rmtoll CTRL         MSIZE         LL_DMA_SetMemorySize
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @param  MemorySize This parameter can be one of the following values:
 *         @arg @ref LL_DMA_DATAALIGN_BYTE
 *         @arg @ref LL_DMA_DATAALIGN_HALFWORD
 *         @arg @ref LL_DMA_DATAALIGN_WORD
 * @retval None
 */
__STATIC_INLINE void LL_DMA_SetMemorySize(DMA_TypeDef *DMAx, uint32_t Channel, uint32_t MemorySize)
{
    MODIFY_REG(DMAx->CH0[Channel].CTRL, DMA_CTRL_MSIZE, MemorySize);
}

/**
 * @brief  Get Memory size.
 * @rmtoll CTRL         MSIZE         LL_DMA_GetMemorySize
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DMA_DATAALIGN_BYTE
 *         @arg @ref LL_DMA_DATAALIGN_HALFWORD
 *         @arg @ref LL_DMA_DATAALIGN_WORD
 */
__STATIC_INLINE uint32_t LL_DMA_GetMemorySize(DMA_TypeDef *DMAx, uint32_t Channel)
{
    return (READ_BIT(DMAx->CH0[Channel].CTRL, DMA_CTRL_MSIZE));
}

/**
 * @brief  Set Channel priority level.
 * @rmtoll CTRL         PL         LL_DMA_SetChannelPriorityLevel
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @param  Priority This parameter can be one of the following values:
 *         @arg @ref LL_DMA_PRIORITY_LOW
 *         @arg @ref LL_DMA_PRIORITY_MEDIUM
 *         @arg @ref LL_DMA_PRIORITY_HIGH
 *         @arg @ref LL_DMA_PRIORITY_VERYHIGH
 * @retval None
 */
__STATIC_INLINE void LL_DMA_SetChannelPriorityLevel(DMA_TypeDef *DMAx, uint32_t Channel, uint32_t Priority)
{
    MODIFY_REG(DMAx->CH0[Channel].CTRL, DMA_CTRL_PL, Priority);
}

/**
 * @brief  Get Channel priority level.
 * @rmtoll CTRL         PL         LL_DMA_GetChannelPriorityLevel
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DMA_PRIORITY_LOW
 *         @arg @ref LL_DMA_PRIORITY_MEDIUM
 *         @arg @ref LL_DMA_PRIORITY_HIGH
 *         @arg @ref LL_DMA_PRIORITY_VERYHIGH
 */
__STATIC_INLINE uint32_t LL_DMA_GetChannelPriorityLevel(DMA_TypeDef *DMAx, uint32_t Channel)
{
    return (READ_BIT(DMAx->CH0[Channel].CTRL, DMA_CTRL_PL));
}

/**
 * @brief  Set Number of data to transfer.
 * @rmtoll NDTR         NDT         LL_DMA_SetDataLength
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @param  NbData Between 0 to 0xFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_DMA_SetDataLength(DMA_TypeDef* DMAx, uint32_t Channel, uint32_t NbData)
{
    if (Channel < LL_DMA_CHANNEL_4) { /* Channel x could not access Flash */
        MODIFY_REG(DMAx->CH0[Channel].NDTR, LL_DMA_NDTR_SRAM_MASK, NbData);
    } else { /* Channel x could access Flash */
        MODIFY_REG(DMAx->CH0[Channel].NDTR, LL_DMA_NDTR_FLASH_MASK, NbData);
    }
}

/**
 * @brief  Get Number of data to transfer.
 * @rmtoll NDTR         NDT         LL_DMA_GetDataLength
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @retval Between 0 to 0xFFFFF.
 */
__STATIC_INLINE uint32_t LL_DMA_GetDataLength(DMA_TypeDef* DMAx, uint32_t Channel)
{
    uint32_t NbData;
    if (Channel < LL_DMA_CHANNEL_4) { /* Channel x could not access Flash */
        NbData = (uint32_t)READ_BIT(DMAx->CH0[Channel].NDTR, LL_DMA_NDTR_SRAM_MASK);
    } else { /* Channel x could access Flash */
        NbData = (uint32_t)READ_BIT(DMAx->CH0[Channel].NDTR, LL_DMA_NDTR_FLASH_MASK);
    }
    return NbData;
}

/**
 * @brief  Configure the Source and Destination addresses.
 * @rmtoll MADR         MADR         LL_DMA_ConfigAddresses
 *         PADR         PADR         LL_DMA_ConfigAddresses
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @param  SrcAddress Between Min_Data = 0 and Max_Data = 0xFFFFFFFF.
 * @param  DstAddress Between Min_Data = 0 and Max_Data = 0xFFFFFFFF.
 * @param  Direction This parameter can be one of the following values:
 *         @arg @ref LL_DMA_DIRECTION_PERIPH2MEMORY
 *         @arg @ref LL_DMA_DIRECTION_MEMORY2PERIPH
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ConfigAddresses(DMA_TypeDef *DMAx, uint32_t Channel, uint32_t SrcAddress, uint32_t DstAddress, uint32_t Direction)
{
    /* Direction from Memory to Periph.*/
    if (Direction == LL_DMA_DIRECTION_MEMORY2PERIPH) {
        if (Channel < LL_DMA_CHANNEL_4) {
            /* Channel could not access Flash.*/
            MODIFY_REG(DMAx->CH0[Channel].MADR, LL_DMA_MADR_SRAM_MASK, SrcAddress);
        } else {
            /* Channel could access Flash.*/
            MODIFY_REG(DMAx->CH0[Channel].MADR, LL_DMA_MADR_FLASH_MASK, SrcAddress);
        }
        MODIFY_REG(DMAx->CH0[Channel].PADR, LL_DMA_PADR_SRAM_MASK, DstAddress);
    } else {
        /* Direction from Periph to Memory.*/
        MODIFY_REG(DMAx->CH0[Channel].PADR, LL_DMA_PADR_SRAM_MASK, SrcAddress);
        if (Channel < LL_DMA_CHANNEL_4) {
            /* Channel could not access Flash.*/
            MODIFY_REG(DMAx->CH0[Channel].MADR, LL_DMA_MADR_SRAM_MASK, DstAddress);
        } else {
            /* Channel could access Flash.*/
            MODIFY_REG(DMAx->CH0[Channel].MADR, LL_DMA_MADR_FLASH_MASK, DstAddress);
        }
    }
}

/**
 * @brief  Set the Memory address.
 * @rmtoll MADR         MADR         LL_DMA_SetMemoryAddress
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @param  MemoryAddress Between 0 to 0x3FFFF.
 * @retval None
 */
__STATIC_INLINE void LL_DMA_SetMemoryAddress(DMA_TypeDef* DMAx, uint32_t Channel, uint32_t MemoryAddress)
{
    if (Channel < LL_DMA_CHANNEL_4) { /* Channel x could not access Flash */
        MODIFY_REG(DMAx->CH0[Channel].MADR, LL_DMA_MADR_SRAM_MASK, MemoryAddress);
    } else { /* Channel x could access Flash */
        MODIFY_REG(DMAx->CH0[Channel].MADR, LL_DMA_MADR_FLASH_MASK, MemoryAddress);
    }
}

/**
 * @brief  Get the Memory address.
 * @rmtoll MADR         MADR         LL_DMA_GetMemoryAddress
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @retval Between 0 to 0x3FFFF.
 */
__STATIC_INLINE uint32_t LL_DMA_GetMemoryAddress(DMA_TypeDef* DMAx, uint32_t Channel)
{
    uint32_t addr;
    if (Channel < LL_DMA_CHANNEL_4) { /* Channel x could not access Flash */
        addr = (uint32_t)READ_BIT(DMAx->CH0[Channel].MADR, LL_DMA_MADR_SRAM_MASK);
    } else { /* Channel x could access Flash */
        addr = (uint32_t)READ_BIT(DMAx->CH0[Channel].MADR, LL_DMA_MADR_FLASH_MASK);
    }
    return addr;
}

/**
 * @brief  Set the Peripheral address.
 * @rmtoll PADR         PADR         LL_DMA_SetPeriphAddress
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @param  PeriphAddress Between 0 to 0xFFFFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_DMA_SetPeriphAddress(DMA_TypeDef* DMAx, uint32_t Channel, uint32_t PeriphAddress)
{
    MODIFY_REG(DMAx->CH0[Channel].PADR, LL_DMA_PADR_SRAM_MASK, PeriphAddress);
}

/**
 * @brief  Get the Peripheral address.
 * @rmtoll PADR         PADR         LL_DMA_GetPeriphAddress
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_0
 *         @arg @ref LL_DMA_CHANNEL_1
 *         @arg @ref LL_DMA_CHANNEL_2
 *         @arg @ref LL_DMA_CHANNEL_3
 *         @arg @ref LL_DMA_CHANNEL_4
 * @retval Between 0 to 0xFFFFFFFF.
 */
__STATIC_INLINE uint32_t LL_DMA_GetPeriphAddress(DMA_TypeDef* DMAx, uint32_t Channel)
{
    return (uint32_t)(READ_BIT(DMAx->CH0[Channel].PADR, LL_DMA_PADR_SRAM_MASK));
}

/**
 * @brief  Enable DMA M2M Channel.
 * @rmtoll MCTRL         EN         LL_DMA_EnableM2MChannel
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableM2MChannel(DMA_TypeDef *DMAx, uint32_t Channel)
{
    SET_BIT(DMAx->CH1[Channel - 5].MCTRL, DMA_MCTRL_EN);
}

/**
 * @brief  Disable DMA M2M Channel.
 * @rmtoll MCTRL         EN         LL_DMA_DisableM2MChannel
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableM2MChannel(DMA_TypeDef *DMAx, uint32_t Channel)
{
    CLEAR_BIT(DMAx->CH1[Channel - 5].MCTRL, DMA_MCTRL_EN);
}

/**
 * @brief  Check if DMA M2M Channel is enabled or disabled.
 * @rmtoll MCTRL         EN         LL_DMA_IsEnabledM2MChannel
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledM2MChannel(DMA_TypeDef *DMAx, uint32_t Channel)
{
    return ((READ_BIT(DMAx->CH1[Channel - 5].MCTRL, DMA_MCTRL_EN) == DMA_MCTRL_EN) ? 1UL : 0UL);
}

/**
 * @brief  Configure all default parameters linked to DMA M2M transfer.
 * @rmtoll MCTRL         CIRC         LL_DMA_ConfigM2MTransfer
 *         MCTRL         PL           LL_DMA_ConfigM2MTransfer
 *         MCTRL         DSIZE        LL_DMA_ConfigM2MTransfer
 *         MCTRL         SSIZE        LL_DMA_ConfigM2MTransfer
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @param  Configuration This parameter must be a combination of all the following values:
 *         @arg @ref LL_DMA_MODE_NORMAL or @ref LL_DMA_MODE_CIRCULAR
 *         @arg @ref LL_DMA_DATAALIGN_BYTE or @ref LL_DMA_DATAALIGN_HALFWORD or @ref LL_DMA_DATAALIGN_WORD
 *         @arg @ref LL_DMA_PRIORITY_LOW or @ref LL_DMA_PRIORITY_MEDIUM or @ref LL_DMA_PRIORITY_HIGH or @ref LL_DMA_PRIORITY_VERYHIGH
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ConfigM2MTransfer(DMA_TypeDef *DMAx, uint32_t Channel, uint32_t Configuration)
{
    MODIFY_REG(DMAx->CH1[Channel - 5].MCTRL, (DMA_MCTRL_CIRC | DMA_MCTRL_PL | \
                    DMA_MCTRL_DSIZE | DMA_MCTRL_SSIZE), Configuration);
}

/**
 * @brief  Set DMA M2M mode normal, circular or peripheral flow control.
 * @rmtoll MCTRL         CIRC         LL_DMA_SetM2MMode
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @param  Mode This parameter can be one of the following values:
 *         @arg @ref LL_DMA_MODE_NORMAL
 *         @arg @ref LL_DMA_MODE_CIRCULAR
 * @retval None
 */
__STATIC_INLINE void LL_DMA_SetM2MMode(DMA_TypeDef *DMAx, uint32_t Channel, uint32_t Mode)
{
    MODIFY_REG(DMAx->CH1[Channel - 5].MCTRL, DMA_MCTRL_CIRC, Mode);
}

/**
 * @brief  Get DMA M2M mode normal, circular or peripheral flow control.
 * @rmtoll MCTRL         CIRC         LL_DMA_GetM2MMode
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DMA_MODE_NORMAL
 *         @arg @ref LL_DMA_MODE_CIRCULAR
 */
__STATIC_INLINE uint32_t LL_DMA_GetM2MMode(DMA_TypeDef *DMAx, uint32_t Channel)
{
    return (READ_BIT(DMAx->CH1[Channel - 5].MCTRL, DMA_MCTRL_CIRC));
}

/**
 * @brief  Set M2M Channel priority level.
 * @rmtoll MCTRL         PL         LL_DMA_SetM2MChannelPriorityLevel
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @param  Priority This parameter can be one of the following values:
 *         @arg @ref LL_DMA_PRIORITY_LOW
 *         @arg @ref LL_DMA_PRIORITY_MEDIUM
 *         @arg @ref LL_DMA_PRIORITY_HIGH
 *         @arg @ref LL_DMA_PRIORITY_VERYHIGH
 * @retval None
 */
__STATIC_INLINE void LL_DMA_SetM2MChannelPriorityLevel(DMA_TypeDef *DMAx, uint32_t Channel, uint32_t Priority)
{
    MODIFY_REG(DMAx->CH1[Channel - 5].MCTRL, DMA_MCTRL_PL, Priority);
}

/**
 * @brief  Get M2M Channel priority level.
 * @rmtoll MCTRL         PL         LL_DMA_GetM2MChannelPriorityLevel
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DMA_PRIORITY_LOW
 *         @arg @ref LL_DMA_PRIORITY_MEDIUM
 *         @arg @ref LL_DMA_PRIORITY_HIGH
 *         @arg @ref LL_DMA_PRIORITY_VERYHIGH
 */
__STATIC_INLINE uint32_t LL_DMA_GetM2MChannelPriorityLevel(DMA_TypeDef *DMAx, uint32_t Channel)
{
    return (READ_BIT(DMAx->CH1[Channel - 5].MCTRL, DMA_MCTRL_PL));
}

/**
 * @brief  Set M2M Source Memory size.
 * @rmtoll MCTRL         SSIZE         LL_DMA_SetM2MSrcMemorySize
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @param  MemorySize This parameter can be one of the following values:
 *         @arg @ref LL_DMA_SRCDATAALIGN_BYTE
 *         @arg @ref LL_DMA_SRCDATAALIGN_HALFWORD
 *         @arg @ref LL_DMA_SRCDATAALIGN_WORD
 * @retval None
 */
__STATIC_INLINE void LL_DMA_SetM2MSrcMemorySize(DMA_TypeDef *DMAx, uint32_t Channel, uint32_t MemorySize)
{
    MODIFY_REG(DMAx->CH1[Channel - 5].MCTRL, DMA_MCTRL_SSIZE, MemorySize);
}

/**
 * @brief  Get M2M Source Memory size.
 * @rmtoll MCTRL         SSIZE         LL_DMA_GetM2MSrcMemorySize
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DMA_SRCDATAALIGN_BYTE
 *         @arg @ref LL_DMA_SRCDATAALIGN_HALFWORD
 *         @arg @ref LL_DMA_SRCDATAALIGN_WORD
 */
__STATIC_INLINE uint32_t LL_DMA_GetM2MSrcMemorySize(DMA_TypeDef *DMAx, uint32_t Channel)
{
    return (READ_BIT(DMAx->CH1[Channel - 5].MCTRL, DMA_MCTRL_SSIZE));
}

/**
 * @brief  Set M2M Destination Memory size.
 * @rmtoll MCTRL         MSIZE         LL_DMA_SetM2MDstMemorySize
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @param  MemorySize This parameter can be one of the following values:
 *         @arg @ref LL_DMA_DSTDATAALIGN_BYTE
 *         @arg @ref LL_DMA_DSTDATAALIGN_HALFWORD
 *         @arg @ref LL_DMA_DSTDATAALIGN_WORD
 * @retval None
 */
__STATIC_INLINE void LL_DMA_SetM2MDstMemorySize(DMA_TypeDef *DMAx, uint32_t Channel, uint32_t MemorySize)
{
    MODIFY_REG(DMAx->CH1[Channel - 5].MCTRL, DMA_MCTRL_DSIZE, MemorySize);
}

/**
 * @brief  Get M2M Destination Memory size.
 * @rmtoll MCTRL         MSIZE         LL_DMA_GetM2MDstMemorySize
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @retval Returned value can be one of the following values:
 *         @arg @ref LL_DMA_DSTDATAALIGN_BYTE
 *         @arg @ref LL_DMA_DSTDATAALIGN_HALFWORD
 *         @arg @ref LL_DMA_DSTDATAALIGN_WORD
 */
__STATIC_INLINE uint32_t LL_DMA_GetM2MDstMemorySize(DMA_TypeDef *DMAx, uint32_t Channel)
{
    return (READ_BIT(DMAx->CH1[Channel - 5].MCTRL, DMA_MCTRL_DSIZE));
}

/**
 * @brief  Set Number of data to transfer.
 * @rmtoll MNDTR         NDT         LL_DMA_SetM2MDataLength
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @param  NbData Between 0 to 0xFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_DMA_SetM2MDataLength(DMA_TypeDef* DMAx, uint32_t Channel, uint32_t NbData)
{
    MODIFY_REG(DMAx->CH1[Channel - 5].MNDTR, LL_DMA_NDTR_FLASH_MASK, NbData);
}

/**
 * @brief  Get Number of data to transfer.
 * @rmtoll MNDTR         NDT         LL_DMA_GetM2MDataLength
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @retval Between 0 to 0xFFFFF.
 */
__STATIC_INLINE uint32_t LL_DMA_GetM2MDataLength(DMA_TypeDef* DMAx, uint32_t Channel)
{
    return (READ_BIT(DMAx->CH1[Channel - 5].MNDTR, LL_DMA_NDTR_FLASH_MASK));
}

/**
 * @brief  Set the Memory to Memory Source address.
 * @rmtoll MSADR         MSADR         LL_DMA_SetM2MSrcAddress
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @param  MemoryAddress Between 0 to 0xFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_DMA_SetM2MSrcAddress(DMA_TypeDef* DMAx, uint32_t Channel, uint32_t MemoryAddress)
{
    MODIFY_REG(DMAx->CH1[Channel - 5].MSADR, LL_DMA_MSADR_FLASH_MASK, MemoryAddress);
}

/**
 * @brief  Get the Memory to Memory Source address.
 * @rmtoll MSADR         MSADR         LL_DMA_GetM2MSrcAddress
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @retval Between 0 to 0xFFFFF.
 */
__STATIC_INLINE uint32_t LL_DMA_GetM2MSrcAddress(DMA_TypeDef* DMAx, uint32_t Channel)
{
    return (uint32_t)(READ_BIT(DMAx->CH1[Channel - 5].MSADR, LL_DMA_MSADR_FLASH_MASK));
}

/**
 * @brief  Set the Memory to Memory Destination address.
 * @rmtoll MDADR         MDADR         LL_DMA_SetM2MDstAddress
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @param  MemoryAddress Between 0 to 0xFFFFF.
 * @retval None
 */
__STATIC_INLINE void LL_DMA_SetM2MDstAddress(DMA_TypeDef* DMAx, uint32_t Channel, uint32_t MemoryAddress)
{
    MODIFY_REG(DMAx->CH1[Channel - 5].MDADR, LL_DMA_MDADR_FLASH_MASK, MemoryAddress);
}

/**
 * @brief  Get the Memory to Memory Destination address.
 * @rmtoll MDADR         MDADR         LL_DMA_GetM2MDstAddress
 * @param  DMAx DMA instance
 * @param  Channel This parameter can be one of the following values:
 *         @arg @ref LL_DMA_CHANNEL_5
 *         @arg @ref LL_DMA_CHANNEL_6
 * @retval Between 0 to 0xFFFFF.
 */
__STATIC_INLINE uint32_t LL_DMA_GetM2MDstAddress(DMA_TypeDef* DMAx, uint32_t Channel)
{
    return (uint32_t)(READ_BIT(DMAx->CH1[Channel - 5].MDADR, LL_DMA_MDADR_FLASH_MASK));
}

/**
 * @}
 */

/** @defgroup DMA_LL_EF_FLAG_Management FLAG_Management
 * @{
 */

/**
 * @brief  Get Channel 0 half transfer flag.
 * @rmtoll IF         HTIF0         LL_DMA_IsActiveFlag_HT0
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_HT0(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_HTIF0) == (DMA_IF_HTIF0));
}

/**
 * @brief  Get Channel 1 half transfer flag.
 * @rmtoll IF         HTIF1         LL_DMA_IsActiveFlag_HT1
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_HT1(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_HTIF1) == (DMA_IF_HTIF1));
}

/**
 * @brief  Get Channel 2 half transfer flag.
 * @rmtoll IF         HTIF2         LL_DMA_IsActiveFlag_HT2
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_HT2(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_HTIF2) == (DMA_IF_HTIF2));
}

/**
 * @brief  Get Channel 3 half transfer flag.
 * @rmtoll IF         HTIF3         LL_DMA_IsActiveFlag_HT3
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_HT3(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_HTIF3) == (DMA_IF_HTIF3));
}

/**
 * @brief  Get Channel 4 half transfer flag.
 * @rmtoll IF         HTIF4         LL_DMA_IsActiveFlag_HT4
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_HT4(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_HTIF4) == (DMA_IF_HTIF4));
}

/**
 * @brief  Get Channel 0 transfer complete flag.
 * @rmtoll IF         TCIF0         LL_DMA_IsActiveFlag_TC0
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_TC0(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_TCIF0) == (DMA_IF_TCIF0));
}

/**
 * @brief  Get Channel 1 transfer complete flag.
 * @rmtoll IF         TCIF1         LL_DMA_IsActiveFlag_TC1
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_TC1(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_TCIF1) == (DMA_IF_TCIF1));
}

/**
 * @brief  Get Channel 2 transfer complete flag.
 * @rmtoll IF         TCIF2         LL_DMA_IsActiveFlag_TC2
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_TC2(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_TCIF2) == (DMA_IF_TCIF2));
}

/**
 * @brief  Get Channel 3 transfer complete flag.
 * @rmtoll IF         TCIF3         LL_DMA_IsActiveFlag_TC3
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_TC3(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_TCIF3) == (DMA_IF_TCIF3));
}

/**
 * @brief  Get Channel 4 transfer complete flag.
 * @rmtoll IF         TCIF4         LL_DMA_IsActiveFlag_TC4
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_TC4(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_TCIF4) == (DMA_IF_TCIF4));
}

/**
 * @brief  Get Channel 0 transfer error flag.
 * @rmtoll IF         TEIF0         LL_DMA_IsActiveFlag_TE0
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_TE0(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_TEIF0) == (DMA_IF_TEIF0));
}

/**
 * @brief  Get Channel 1 transfer error flag.
 * @rmtoll IF         TEIF1         LL_DMA_IsActiveFlag_TE1
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_TE1(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_TEIF1) == (DMA_IF_TEIF1));
}

/**
 * @brief  Get Channel 2 transfer error flag.
 * @rmtoll IF         TEIF2         LL_DMA_IsActiveFlag_TE2
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_TE2(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_TEIF2) == (DMA_IF_TEIF2));
}

/**
 * @brief  Get Channel 3 transfer error flag.
 * @rmtoll IF         TEIF3         LL_DMA_IsActiveFlag_TE3
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_TE3(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_TEIF3) == (DMA_IF_TEIF3));
}

/**
 * @brief  Get Channel 4 transfer error flag.
 * @rmtoll IF         TEIF4         LL_DMA_IsActiveFlag_TE4
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag_TE4(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IF, DMA_IF_TEIF4) == (DMA_IF_TEIF4));
}

/**
 * @brief  Get M2M Channel 0 half transfer flag.
 * @rmtoll MIF         HTIF0         LL_DMA_IsActiveM2MFlag_HT0
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveM2MFlag_HT0(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->MIF, DMA_MIF_HTIF0) == (DMA_MIF_HTIF0));
}

/**
 * @brief  Get Channel 1 half transfer flag.
 * @rmtoll MIF         HTIF1         LL_DMA_IsActiveM2MFlag_HT1
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveM2MFlag_HT1(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->MIF, DMA_MIF_HTIF1) == (DMA_MIF_HTIF1));
}

/**
 * @brief  Get Channel 0 transfer complete flag.
 * @rmtoll MIF         TCIF0         LL_DMA_IsActiveM2MFlag_TC0
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveM2MFlag_TC0(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->MIF, DMA_MIF_TCIF0) == (DMA_MIF_TCIF0));
}

/**
 * @brief  Get Channel 1 transfer complete flag.
 * @rmtoll MIF         TCIF1         LL_DMA_IsActiveM2MFlag_TC1
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveM2MFlag_TC1(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->MIF, DMA_MIF_TCIF1) == (DMA_MIF_TCIF1));
}

/**
 * @brief  Get Channel 0 transfer error flag.
 * @rmtoll MIF         TEIF0         LL_DMA_IsActiveM2MFlag_TE0
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveM2MFlag_TE0(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->MIF, DMA_MIF_TEIF0) == (DMA_MIF_TEIF0));
}

/**
 * @brief  Get Channel 1 transfer error flag.
 * @rmtoll MIF         TEIF1         LL_DMA_IsActiveM2MFlag_TE1
 * @param  DMAx DMA instance
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveM2MFlag_TE1(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->MIF, DMA_MIF_TEIF1) == (DMA_MIF_TEIF1));
}

/**
 * @brief  Get Channel transfer error flag.
 * @rmtoll IF         HTIF0         LL_DMA_IsActiveFlag
 *         IF         TCIF0         LL_DMA_IsActiveFlag
 *         IF         TEIF0         LL_DMA_IsActiveFlag
 *         IF         HTIF1         LL_DMA_IsActiveFlag
 *         IF         TCIF1         LL_DMA_IsActiveFlag
 *         IF         TEIF1         LL_DMA_IsActiveFlag
 *         IF         HTIF2         LL_DMA_IsActiveFlag
 *         IF         TCIF2         LL_DMA_IsActiveFlag
 *         IF         TEIF2         LL_DMA_IsActiveFlag
 *         IF         HTIF3         LL_DMA_IsActiveFlag
 *         IF         TCIF3         LL_DMA_IsActiveFlag
 *         IF         TEIF3         LL_DMA_IsActiveFlag
 *         IF         HTIF4         LL_DMA_IsActiveFlag
 *         IF         TCIF4         LL_DMA_IsActiveFlag
 *         IF         TEIF4         LL_DMA_IsActiveFlag
 * @param  DMAx DMA instance
 * @param  Flags This parameter can be one of the following values:
 *         @arg @ref LL_DMA_IF_HTIF0
 *         @arg @ref LL_DMA_IF_TCIF0
 *         @arg @ref LL_DMA_IF_TEIF0
 *         @arg @ref LL_DMA_IF_HTIF1
 *         @arg @ref LL_DMA_IF_TCIF1
 *         @arg @ref LL_DMA_IF_TEIF1
 *         @arg @ref LL_DMA_IF_HTIF2
 *         @arg @ref LL_DMA_IF_TCIF2
 *         @arg @ref LL_DMA_IF_TEIF2
 *         @arg @ref LL_DMA_IF_HTIF3
 *         @arg @ref LL_DMA_IF_TCIF3
 *         @arg @ref LL_DMA_IF_TEIF3
 *         @arg @ref LL_DMA_IF_HTIF4
 *         @arg @ref LL_DMA_IF_TCIF4
 *         @arg @ref LL_DMA_IF_TEIF4
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveFlag(DMA_TypeDef *DMAx, uint32_t Flags)
{
    return (READ_BIT(DMAx->IF, Flags) == (Flags));
}

/**
 * @brief  Get M2M Channel half transfer flag.
 * @rmtoll MIF         HTIF0         LL_DMA_IsActiveM2MFlag
 *         MIF         TCIF0         LL_DMA_IsActiveM2MFlag
 *         MIF         TEIF0         LL_DMA_IsActiveM2MFlag
 *         MIF         TEIF1         LL_DMA_IsActiveM2MFlag
 *         MIF         TEIF1         LL_DMA_IsActiveM2MFlag
 *         MIF         TEIF1         LL_DMA_IsActiveM2MFlag
 * @param  DMAx DMA instance
 * @param  Flags This parameter can be one of the following values:
 *         @arg @ref LL_DMA_MIF_HTIF0
 *         @arg @ref LL_DMA_MIF_TCIF0
 *         @arg @ref LL_DMA_MIF_TEIF0
 *         @arg @ref LL_DMA_MIF_HTIF1
 *         @arg @ref LL_DMA_MIF_TCIF1
 *         @arg @ref LL_DMA_MIF_TEIF1
 * @retval State of bit (1 or 0).
 */
__STATIC_INLINE uint32_t LL_DMA_IsActiveM2MFlag(DMA_TypeDef *DMAx, uint32_t Flags)
{
    return (READ_BIT(DMAx->MIF, Flags) == (Flags));
}

/**
 * @brief  Clear Channel 0 half transfer flag.
 * @rmtoll IF         HTIF0         LL_DMA_ClearFlag_HT0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_HT0(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_HTIF0);
}

/**
 * @brief  Clear Channel 1 half transfer flag.
 * @rmtoll IF         HTIF1         LL_DMA_ClearFlag_HT1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_HT1(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_HTIF1);
}

/**
 * @brief  Clear Channel 2 half transfer flag.
 * @rmtoll IF         HTIF2         LL_DMA_ClearFlag_HT2
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_HT2(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_HTIF2);
}

/**
 * @brief  Clear Channel 3 half transfer flag.
 * @rmtoll IF         HTIF3         LL_DMA_ClearFlag_HT3
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_HT3(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_HTIF3);
}

/**
 * @brief  Clear Channel 4 half transfer flag.
 * @rmtoll IF         HTIF4         LL_DMA_ClearFlag_HT4
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_HT4(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_HTIF4);
}

/**
 * @brief  Clear Channel 0 transfer complete flag.
 * @rmtoll IF         TCIF0         LL_DMA_ClearFlag_TC0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_TC0(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_TCIF0);
}

/**
 * @brief  Clear Channel 1 transfer complete flag.
 * @rmtoll IF         TCIF1         LL_DMA_ClearFlag_TC1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_TC1(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_TCIF1);
}

/**
 * @brief  Clear Channel 2 transfer complete flag.
 * @rmtoll IF         TCIF2         LL_DMA_ClearFlag_TC2
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_TC2(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_TCIF2);
}

/**
 * @brief  Clear Channel 3 transfer complete flag.
 * @rmtoll IF         TCIF3         LL_DMA_ClearFlag_TC3
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_TC3(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_TCIF3);
}

/**
 * @brief  Clear Channel 4 transfer complete flag.
 * @rmtoll IF         TCIF4         LL_DMA_ClearFlag_TC4
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_TC4(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_TCIF4);
}

/**
 * @brief  Clear Channel 0 transfer error flag.
 * @rmtoll IF         TEIF0         LL_DMA_ClearFlag_TE0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_TE0(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_TEIF0);
}

/**
 * @brief  Clear Channel 1 transfer error flag.
 * @rmtoll IF         TEIF1         LL_DMA_ClearFlag_TE1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_TE1(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_TEIF1);
}

/**
 * @brief  Clear Channel 2 transfer error flag.
 * @rmtoll IF         TEIF2         LL_DMA_ClearFlag_TE2
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_TE2(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_TEIF2);
}

/**
 * @brief  Clear Channel 3 transfer error flag.
 * @rmtoll IF         TEIF3         LL_DMA_ClearFlag_TE3
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_TE3(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_TEIF3);
}

/**
 * @brief  Clear Channel 4 transfer error flag.
 * @rmtoll IF         TEIF4         LL_DMA_ClearFlag_TE4
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag_TE4(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->IF, DMA_IF_TEIF4);
}

/**
 * @brief  Clear M2M Channel 0 half transfer flag.
 * @rmtoll MIF         HTIF0         LL_DMA_ClearM2MFlag_HT0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearM2MFlag_HT0(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->MIF, DMA_MIF_HTIF0);
}

/**
 * @brief  Clear M2M Channel 1 half transfer flag.
 * @rmtoll MIF         HTIF1         LL_DMA_ClearM2MFlag_HT1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearM2MFlag_HT1(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->MIF, DMA_MIF_HTIF1);
}

/**
 * @brief  Clear M2M Channel 0 transfer complete flag.
 * @rmtoll MIF         TCIF0         LL_DMA_ClearM2MFlag_TC0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearM2MFlag_TC0(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->MIF, DMA_MIF_TCIF0);
}

/**
 * @brief  Clear M2M Channel 1 transfer complete flag.
 * @rmtoll MIF         TCIF1         LL_DMA_ClearM2MFlag_TC1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearM2MFlag_TC1(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->MIF, DMA_MIF_TCIF1);
}

/**
 * @brief  Clear M2M Channel 0 transfer error flag.
 * @rmtoll MIF         TEIF0         LL_DMA_ClearM2MFlag_TE0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearM2MFlag_TE0(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->MIF, DMA_MIF_TEIF0);
}

/**
 * @brief  Clear M2M Channel 1 transfer error flag.
 * @rmtoll MIF         TEIF1         LL_DMA_ClearM2MFlag_TE1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearM2MFlag_TE1(DMA_TypeDef *DMAx)
{
    WRITE_REG(DMAx->MIF, DMA_MIF_TEIF1);
}

/**
 * @brief  Clear Channel half transfer flag, transfer complete flag and transfer error flag.
 * @rmtoll IF         HTIF0         LL_DMA_ClearFlag
 *         IF         TCIF0         LL_DMA_ClearFlag
 *         IF         TEIF0         LL_DMA_ClearFlag
 *         IF         HTIF1         LL_DMA_ClearFlag
 *         IF         TCIF1         LL_DMA_ClearFlag
 *         IF         TEIF1         LL_DMA_ClearFlag
 *         IF         HTIF2         LL_DMA_ClearFlag
 *         IF         TCIF2         LL_DMA_ClearFlag
 *         IF         TEIF2         LL_DMA_ClearFlag
 *         IF         HTIF3         LL_DMA_ClearFlag
 *         IF         TCIF3         LL_DMA_ClearFlag
 *         IF         TEIF3         LL_DMA_ClearFlag
 *         IF         HTIF4         LL_DMA_ClearFlag
 *         IF         TCIF4         LL_DMA_ClearFlag
 *         IF         TEIF4         LL_DMA_ClearFlag
 * @param  DMAx DMA instance
 * @param  Flags This parameter can be one of the following values:
 *         @arg @ref LL_DMA_IF_HTIF0
 *         @arg @ref LL_DMA_IF_TCIF0
 *         @arg @ref LL_DMA_IF_TEIF0
 *         @arg @ref LL_DMA_IF_HTIF1
 *         @arg @ref LL_DMA_IF_TCIF1
 *         @arg @ref LL_DMA_IF_TEIF1
 *         @arg @ref LL_DMA_IF_HTIF2
 *         @arg @ref LL_DMA_IF_TCIF2
 *         @arg @ref LL_DMA_IF_TEIF2
 *         @arg @ref LL_DMA_IF_HTIF3
 *         @arg @ref LL_DMA_IF_TCIF3
 *         @arg @ref LL_DMA_IF_TEIF3
 *         @arg @ref LL_DMA_IF_HTIF4
 *         @arg @ref LL_DMA_IF_TCIF4
 *         @arg @ref LL_DMA_IF_TEIF4
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearFlag(DMA_TypeDef *DMAx, uint32_t Flags)
{
    WRITE_REG(DMAx->IF, Flags);
}

/**
 * @brief  Clear M2M Channel half transfer flag, transfer complete flag and transfer error flag.
 * @rmtoll MIF         HTIF0         LL_DMA_ClearM2MFlag
 *         MIF         TCIF0         LL_DMA_ClearM2MFlag
 *         MIF         TEIF0         LL_DMA_ClearM2MFlag
 *         MIF         TEIF1         LL_DMA_ClearM2MFlag
 *         MIF         TEIF1         LL_DMA_ClearM2MFlag
 *         MIF         TEIF1         LL_DMA_ClearM2MFlag
 * @param  DMAx DMA instance
 * @param  Flags This parameter can be one of the following values:
 *         @arg @ref LL_DMA_MIF_HTIF0
 *         @arg @ref LL_DMA_MIF_TCIF0
 *         @arg @ref LL_DMA_MIF_TEIF0
 *         @arg @ref LL_DMA_MIF_HTIF1
 *         @arg @ref LL_DMA_MIF_TCIF1
 *         @arg @ref LL_DMA_MIF_TEIF1
 * @retval None
 */
__STATIC_INLINE void LL_DMA_ClearM2MFlag(DMA_TypeDef *DMAx, uint32_t Flags)
{
    WRITE_REG(DMAx->MIF, Flags);
}

/**
 * @}
 */

/** @defgroup DMA_LL_EF_IT_Management IT_Management
 * @{
 */

/**
 * @brief  Enable Channel 0 half transfer interrupt.
 * @rmtoll IE         HTIE0         LL_DMA_EnableIT_HT0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_HT0(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_HTIE0);
}

/**
 * @brief  Enable Channel 1 half transfer interrupt.
 * @rmtoll IE         HTIE1         LL_DMA_EnableIT_HT1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_HT1(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_HTIE1);
}

/**
 * @brief  Enable Channel 2 half transfer interrupt.
 * @rmtoll IE         HTIE2         LL_DMA_EnableIT_HT2
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_HT2(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_HTIE2);
}

/**
 * @brief  Enable Channel 3 half transfer interrupt.
 * @rmtoll IE         HTIE3         LL_DMA_EnableIT_HT3
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_HT3(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_HTIE3);
}

/**
 * @brief  Enable Channel 4 half transfer interrupt.
 * @rmtoll IE         HTIE4         LL_DMA_EnableIT_HT4
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_HT4(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_HTIE4);
}

/**
 * @brief  Enable Channel 0 transfer complete interrupt.
 * @rmtoll IE         TCIE0         LL_DMA_EnableIT_TC0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_TC0(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_TCIE0);
}

/**
 * @brief  Enable Channel 1 transfer complete interrupt.
 * @rmtoll IE         TCIE1         LL_DMA_EnableIT_TC1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_TC1(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_TCIE1);
}

/**
 * @brief  Enable Channel 2 transfer complete interrupt.
 * @rmtoll IE         TCIE2         LL_DMA_EnableIT_TC2
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_TC2(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_TCIE2);
}

/**
 * @brief  Enable Channel 3 transfer complete interrupt.
 * @rmtoll IE         TCIE3         LL_DMA_EnableIT_TC3
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_TC3(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_TCIE3);
}

/**
 * @brief  Enable Channel 4 transfer complete interrupt.
 * @rmtoll IE         TCIE4         LL_DMA_EnableIT_TC4
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_TC4(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_TCIE4);
}

/**
 * @brief  Enable Channel 0 transfer error interrupt.
 * @rmtoll IE         TEIE0         LL_DMA_EnableIT_TE0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_TE0(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_TEIE0);
}

/**
 * @brief  Enable Channel 1 transfer error interrupt.
 * @rmtoll IE         TEIE1         LL_DMA_EnableIT_TE1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_TE1(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_TEIE1);
}

/**
 * @brief  Enable Channel 2 transfer error interrupt.
 * @rmtoll IE         TEIE2         LL_DMA_EnableIT_TE2
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_TE2(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_TEIE2);
}

/**
 * @brief  Enable Channel 3 transfer error interrupt.
 * @rmtoll IE         TEIE3         LL_DMA_EnableIT_TE3
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_TE3(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_TEIE3);
}

/**
 * @brief  Enable Channel 4 transfer error interrupt.
 * @rmtoll IE         TEIE4         LL_DMA_EnableIT_TE4
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT_TE4(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->IE, DMA_IE_TEIE4);
}

/**
 * @brief  Enable M2M Channel 0 half transfer interrupt.
 * @rmtoll MIE         HTIE0         LL_DMA_EnableM2MIT_HT0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableM2MIT_HT0(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->MIE, DMA_MIE_HTIE0);
}

/**
 * @brief  Enable M2M Channel 1 half transfer interrupt.
 * @rmtoll MIE         HTIE1         LL_DMA_EnableM2MIT_HT1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableM2MIT_HT1(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->MIE, DMA_MIE_HTIE1);
}

/**
 * @brief  Enable M2M Channel 0 transfer complete interrupt.
 * @rmtoll MIE         TCIE0         LL_DMA_EnableM2MIT_TC0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableM2MIT_TC0(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->MIE, DMA_MIE_TCIE0);
}

/**
 * @brief  Enable M2M Channel 1 transfer complete interrupt.
 * @rmtoll MIE         TCIE1         LL_DMA_EnableM2MIT_TC1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableM2MIT_TC1(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->MIE, DMA_MIE_TCIE1);
}

/**
 * @brief  Enable M2M Channel 0 transfer error interrupt.
 * @rmtoll MIE         TEIE0         LL_DMA_EnableM2MIT_TE0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableM2MIT_TE0(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->MIE, DMA_MIE_TEIE0);
}

/**
 * @brief  Enable M2M Channel 1 transfer error interrupt.
 * @rmtoll MIE         TEIE1         LL_DMA_EnableM2MIT_TE1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableM2MIT_TE1(DMA_TypeDef *DMAx)
{
    SET_BIT(DMAx->MIE, DMA_MIE_TEIE1);
}

/**
 * @brief  Enable Channel transfer error interrupt.
 * @rmtoll IE         HTIE0         LL_DMA_EnableIT
 *         IE         TCIE0         LL_DMA_EnableIT
 *         IE         TEIE0         LL_DMA_EnableIT
 *         IE         HTIE1         LL_DMA_EnableIT
 *         IE         TCIE1         LL_DMA_EnableIT
 *         IE         TEIE1         LL_DMA_EnableIT
 *         IE         HTIE2         LL_DMA_EnableIT
 *         IE         TCIE2         LL_DMA_EnableIT
 *         IE         TEIE2         LL_DMA_EnableIT
 *         IE         HTIE3         LL_DMA_EnableIT
 *         IE         TCIE3         LL_DMA_EnableIT
 *         IE         TEIE3         LL_DMA_EnableIT
 *         IE         HTIE4         LL_DMA_EnableIT
 *         IE         TCIE4         LL_DMA_EnableIT
 *         IE         TEIE4         LL_DMA_EnableIT
 * @param  DMAx DMA instance
 * @param  Interrupts This parameter can be one of the following values:
 *         @arg @ref LL_DMA_IE_HTIE0
 *         @arg @ref LL_DMA_IE_TCIE0
 *         @arg @ref LL_DMA_IE_TEIE0
 *         @arg @ref LL_DMA_IE_HTIE1
 *         @arg @ref LL_DMA_IE_TCIE1
 *         @arg @ref LL_DMA_IE_TEIE1
 *         @arg @ref LL_DMA_IE_HTIE2
 *         @arg @ref LL_DMA_IE_TCIE2
 *         @arg @ref LL_DMA_IE_TEIE2
 *         @arg @ref LL_DMA_IE_HTIE3
 *         @arg @ref LL_DMA_IE_TCIE3
 *         @arg @ref LL_DMA_IE_TEIE3
 *         @arg @ref LL_DMA_IE_HTIE4
 *         @arg @ref LL_DMA_IE_TCIE4
 *         @arg @ref LL_DMA_IE_TEIE4
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableIT(DMA_TypeDef *DMAx, uint32_t Interrupts)
{
    SET_BIT(DMAx->IE, Interrupts);
}

/**
 * @brief  Enable M2M Channel half transfer interrupt.
 * @rmtoll MIE         HTIE0         LL_DMA_EnableM2MIT
 *         MIE         TCIE0         LL_DMA_EnableM2MIT
 *         MIE         TEIE0         LL_DMA_EnableM2MIT
 *         MIE         HTIE1         LL_DMA_EnableM2MIT
 *         MIE         TCIE1         LL_DMA_EnableM2MIT
 *         MIE         TEIE1         LL_DMA_EnableM2MIT
 * @param  DMAx DMA instance
 * @param  Interrupts This parameter can be one of the following values:
 *         @arg @ref LL_DMA_MIE_HTIE0
 *         @arg @ref LL_DMA_MIE_TCIE0
 *         @arg @ref LL_DMA_MIE_TEIE0
 *         @arg @ref LL_DMA_MIE_HTIE1
 *         @arg @ref LL_DMA_MIE_TCIE1
 *         @arg @ref LL_DMA_MIE_TEIE1
 * @retval None
 */
__STATIC_INLINE void LL_DMA_EnableM2MIT(DMA_TypeDef *DMAx, uint32_t Interrupts)
{
    SET_BIT(DMAx->MIE, Interrupts);
}

/**
 * @brief  Disable Channel 0 half transfer interrupt.
 * @rmtoll IE         HTIE0         LL_DMA_DisableIT_HT0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_HT0(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_HTIE0);
}

/**
 * @brief  Disable Channel 1 half transfer interrupt.
 * @rmtoll IE         HTIE1         LL_DMA_DisableIT_HT1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_HT1(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_HTIE1);
}

/**
 * @brief  Disable Channel 2 half transfer interrupt.
 * @rmtoll IE         HTIE2         LL_DMA_DisableIT_HT2
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_HT2(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_HTIE2);
}

/**
 * @brief  Disable Channel 3 half transfer interrupt.
 * @rmtoll IE         HTIE3         LL_DMA_DisableIT_HT3
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_HT3(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_HTIE3);
}

/**
 * @brief  Disable Channel 4 half transfer interrupt.
 * @rmtoll IE         HTIE4         LL_DMA_DisableIT_HT4
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_HT4(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_HTIE4);
}

/**
 * @brief  Disable Channel 0 transfer complete interrupt.
 * @rmtoll IE         TCIE0         LL_DMA_DisableIT_TC0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_TC0(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_TCIE0);
}

/**
 * @brief  Disable Channel 1 transfer complete interrupt.
 * @rmtoll IE         TCIE1         LL_DMA_DisableIT_TC1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_TC1(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_TCIE1);
}

/**
 * @brief  Disable Channel 2 transfer complete interrupt.
 * @rmtoll IE         TCIE2         LL_DMA_DisableIT_TC2
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_TC2(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_TCIE2);
}

/**
 * @brief  Disable Channel 3 transfer complete interrupt.
 * @rmtoll IE         TCIE3         LL_DMA_DisableIT_TC3
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_TC3(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_TCIE3);
}

/**
 * @brief  Disable Channel 4 transfer complete interrupt.
 * @rmtoll IE         TCIE4         LL_DMA_DisableIT_TC4
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_TC4(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_TCIE4);
}

/**
 * @brief  Disable Channel 0 transfer error interrupt.
 * @rmtoll IE         TEIE0         LL_DMA_DisableIT_TE0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_TE0(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_TEIE0);
}

/**
 * @brief  Disable Channel 1 transfer error interrupt.
 * @rmtoll IE         TEIE1         LL_DMA_DisableIT_TE1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_TE1(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_TEIE1);
}

/**
 * @brief  Disable Channel 2 transfer error interrupt.
 * @rmtoll IE         TEIE2         LL_DMA_DisableIT_TE2
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_TE2(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_TEIE2);
}

/**
 * @brief  Disable Channel 3 transfer error interrupt.
 * @rmtoll IE         TEIE3         LL_DMA_DisableIT_TE3
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_TE3(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_TEIE3);
}

/**
 * @brief  Disable Channel 4 transfer error interrupt.
 * @rmtoll IE         TEIE4         LL_DMA_DisableIT_TE4
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT_TE4(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->IE, DMA_IE_TEIE4);
}

/**
 * @brief  Disable M2M Channel 0 half transfer interrupt.
 * @rmtoll MIE         HTIE0         LL_DMA_DisableM2MIT_HT0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableM2MIT_HT0(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->MIE, DMA_MIE_HTIE0);
}

/**
 * @brief  Disable M2M Channel 1 half transfer interrupt.
 * @rmtoll MIE         HTIE1         LL_DMA_DisableM2MIT_HT1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableM2MIT_HT1(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->MIE, DMA_MIE_HTIE1);
}

/**
 * @brief  Disable M2M Channel 0 transfer complete interrupt.
 * @rmtoll MIE         TCIE0         LL_DMA_DisableM2MIT_TC0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableM2MIT_TC0(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->MIE, DMA_MIE_TCIE0);
}

/**
 * @brief  Disable M2M Channel 1 transfer complete interrupt.
 * @rmtoll MIE         TCIE1         LL_DMA_DisableM2MIT_TC1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableM2MIT_TC1(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->MIE, DMA_MIE_TCIE1);
}

/**
 * @brief  Disable M2M Channel 0 transfer error interrupt.
 * @rmtoll MIE         TEIE0         LL_DMA_DisableM2MIT_TE0
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableM2MIT_TE0(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->MIE, DMA_MIE_TEIE0);
}

/**
 * @brief  Disable M2M Channel 1 transfer error interrupt.
 * @rmtoll MIE         TEIE1         LL_DMA_DisableM2MIT_TE1
 * @param  DMAx DMA instance
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableM2MIT_TE1(DMA_TypeDef *DMAx)
{
    CLEAR_BIT(DMAx->MIE, DMA_MIE_TEIE1);
}

/**
 * @brief  Disable Channel transfer error interrupt.
 * @rmtoll IE         HTIE0         LL_DMA_DisableIT
 *         IE         TCIE0         LL_DMA_DisableIT
 *         IE         TEIE0         LL_DMA_DisableIT
 *         IE         HTIE1         LL_DMA_DisableIT
 *         IE         TCIE1         LL_DMA_DisableIT
 *         IE         TEIE1         LL_DMA_DisableIT
 *         IE         HTIE2         LL_DMA_DisableIT
 *         IE         TCIE2         LL_DMA_DisableIT
 *         IE         TEIE2         LL_DMA_DisableIT
 *         IE         HTIE3         LL_DMA_DisableIT
 *         IE         TCIE3         LL_DMA_DisableIT
 *         IE         TEIE3         LL_DMA_DisableIT
 *         IE         HTIE4         LL_DMA_DisableIT
 *         IE         TCIE4         LL_DMA_DisableIT
 *         IE         TEIE4         LL_DMA_DisableIT
 * @param  DMAx DMA instance
 * @param  Interrupts This parameter can be one of the following values:
 *         @arg @ref LL_DMA_IE_HTIE0
 *         @arg @ref LL_DMA_IE_TCIE0
 *         @arg @ref LL_DMA_IE_TEIE0
 *         @arg @ref LL_DMA_IE_HTIE1
 *         @arg @ref LL_DMA_IE_TCIE1
 *         @arg @ref LL_DMA_IE_TEIE1
 *         @arg @ref LL_DMA_IE_HTIE2
 *         @arg @ref LL_DMA_IE_TCIE2
 *         @arg @ref LL_DMA_IE_TEIE2
 *         @arg @ref LL_DMA_IE_HTIE3
 *         @arg @ref LL_DMA_IE_TCIE3
 *         @arg @ref LL_DMA_IE_TEIE3
 *         @arg @ref LL_DMA_IE_HTIE4
 *         @arg @ref LL_DMA_IE_TCIE4
 *         @arg @ref LL_DMA_IE_TEIE4
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableIT(DMA_TypeDef *DMAx, uint32_t Interrupts)
{
    CLEAR_BIT(DMAx->IE, Interrupts);
}

/**
 * @brief  Disable M2M Channel half transfer interrupt.
 * @rmtoll MIE         HTIE0         LL_DMA_DisableM2MIT
 *         MIE         TCIE0         LL_DMA_DisableM2MIT
 *         MIE         TEIE0         LL_DMA_DisableM2MIT
 *         MIE         HTIE1         LL_DMA_DisableM2MIT
 *         MIE         TCIE1         LL_DMA_DisableM2MIT
 *         MIE         TEIE1         LL_DMA_DisableM2MIT
 * @param  DMAx DMA instance
 * @param  Interrupts This parameter can be one of the following values:
 *         @arg @ref LL_DMA_MIE_HTIE0
 *         @arg @ref LL_DMA_MIE_TCIE0
 *         @arg @ref LL_DMA_MIE_TEIE0
 *         @arg @ref LL_DMA_MIE_HTIE1
 *         @arg @ref LL_DMA_MIE_TCIE1
 *         @arg @ref LL_DMA_MIE_TEIE1
 * @retval None
 */
__STATIC_INLINE void LL_DMA_DisableM2MIT(DMA_TypeDef *DMAx, uint32_t Interrupts)
{
    CLEAR_BIT(DMAx->MIE, Interrupts);
}

/**
 * @brief  Check if Channel 0 half transfer interrupt is enabled or not.
 * @rmtoll IE         HTIE0         LL_DMA_IsEnabledIT_HT0
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_HT0(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_HTIE0) == DMA_IE_HTIE0);
}

/**
 * @brief  Check if Channel 1 half transfer interrupt is enabled or not.
 * @rmtoll IE         HTIE1         LL_DMA_IsEnabledIT_HT1
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_HT1(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_HTIE1) == DMA_IE_HTIE1);
}

/**
 * @brief  Check if Channel 2 half transfer interrupt is enabled or not.
 * @rmtoll IE         HTIE2         LL_DMA_IsEnabledIT_HT2
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_HT2(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_HTIE2) == DMA_IE_HTIE2);
}

/**
 * @brief  Check if Channel 3 half transfer interrupt is enabled or not.
 * @rmtoll IE         HTIE3         LL_DMA_IsEnabledIT_HT3
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_HT3(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_HTIE3) == DMA_IE_HTIE3);
}

/**
 * @brief  Check if Channel 4 half transfer interrupt is enabled or not.
 * @rmtoll IE         HTIE4         LL_DMA_IsEnabledIT_HT4
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_HT4(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_HTIE4) == DMA_IE_HTIE4);
}

/**
 * @brief  Check if Channel 0 transfer complete interrupt is enabled or not.
 * @rmtoll IE         TCIE0         LL_DMA_IsEnabledIT_TC0
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_TC0(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_TCIE0) == DMA_IE_TCIE0);
}

/**
 * @brief  Check if Channel 1 transfer complete interrupt is enabled or not.
 * @rmtoll IE         TCIE1         LL_DMA_IsEnabledIT_TC1
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_TC1(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_TCIE1) == DMA_IE_TCIE1);
}

/**
 * @brief  Check if Channel 2 transfer complete interrupt is enabled or not.
 * @rmtoll IE         TCIE2         LL_DMA_IsEnabledIT_TC2
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_TC2(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_TCIE2) == DMA_IE_TCIE2);
}

/**
 * @brief  Check if Channel 3 transfer complete interrupt is enabled or not.
 * @rmtoll IE         TCIE3         LL_DMA_IsEnabledIT_TC3
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_TC3(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_TCIE3) == DMA_IE_TCIE3);
}

/**
 * @brief  Check if Channel 4 transfer complete interrupt is enabled or not.
 * @rmtoll IE         TCIE4         LL_DMA_IsEnabledIT_TC4
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_TC4(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_TCIE4) == DMA_IE_TCIE4);
}

/**
 * @brief  Check if Channel 0 transfer error interrupt is enabled or not.
 * @rmtoll IE         TEIE0         LL_DMA_IsEnabledIT_TE0
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_TE0(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_TEIE0) == DMA_IE_TEIE0);
}

/**
 * @brief  Check if Channel 1 transfer error interrupt is enabled or not.
 * @rmtoll IE         TEIE1         LL_DMA_IsEnabledIT_TE1
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_TE1(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_TEIE1) == DMA_IE_TEIE1);
}

/**
 * @brief  Check if Channel 2 transfer error interrupt is enabled or not.
 * @rmtoll IE         TEIE2         LL_DMA_IsEnabledIT_TE2
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_TE2(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_TEIE2) == DMA_IE_TEIE2);
}

/**
 * @brief  Check if Channel 3 transfer error interrupt is enabled or not.
 * @rmtoll IE         TEIE3         LL_DMA_IsEnabledIT_TE3
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_TE3(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_TEIE3) == DMA_IE_TEIE3);
}

/**
 * @brief  Check if Channel 4 transfer error interrupt is enabled or not.
 * @rmtoll IE         TEIE4         LL_DMA_IsEnabledIT_TE4
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT_TE4(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->IE, DMA_IE_TEIE4) == DMA_IE_TEIE4);
}

/**
 * @brief  Check if M2M Channel 0 half transfer interrupt is enabled or not.
 * @rmtoll MIE         HTIE0         LL_DMA_IsEnabledM2MIT_HT0
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledM2MIT_HT0(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->MIE, DMA_MIE_HTIE0) == DMA_MIE_HTIE0);
}

/**
 * @brief  Check if M2M Channel 1 half transfer interrupt is enabled or not.
 * @rmtoll MIE         HTIE1         LL_DMA_IsEnabledM2MIT_HT1
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledM2MIT_HT1(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->MIE, DMA_MIE_HTIE1) == DMA_MIE_HTIE1);
}

/**
 * @brief  Check if M2M Channel 0 transfer complete interrupt is enabled or not.
 * @rmtoll MIE         TCIE0         LL_DMA_IsEnabledM2MIT_TC0
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledM2MIT_TC0(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->MIE, DMA_MIE_TCIE0) == DMA_MIE_TCIE0);
}

/**
 * @brief  Check if M2M Channel 1 transfer complete interrupt is enabled or not.
 * @rmtoll MIE         TCIE1         LL_DMA_IsEnabledM2MIT_TC1
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledM2MIT_TC1(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->MIE, DMA_MIE_TCIE1) == DMA_MIE_TCIE1);
}

/**
 * @brief  Check if M2M Channel 0 transfer error interrupt is enabled or not.
 * @rmtoll MIE         TEIE0         LL_DMA_IsEnabledM2MIT_TE0
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledM2MIT_TE0(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->MIE, DMA_MIE_TEIE0) == DMA_MIE_TEIE0);
}

/**
 * @brief  Check if M2M Channel 1 transfer error interrupt is enabled or not.
 * @rmtoll MIE         TEIE1         LL_DMA_IsEnabledM2MIT_TE1
 * @param  DMAx DMA instance
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledM2MIT_TE1(DMA_TypeDef *DMAx)
{
    return (READ_BIT(DMAx->MIE, DMA_MIE_TEIE1) == DMA_MIE_TEIE1);
}

/**
 * @brief  Check if Channel transfer error interrupt is enabled or not.
 * @rmtoll IE         HTIE0         LL_DMA_IsEnabledIT
 *         IE         TCIE0         LL_DMA_IsEnabledIT
 *         IE         TEIE0         LL_DMA_IsEnabledIT
 *         IE         HTIE1         LL_DMA_IsEnabledIT
 *         IE         TCIE1         LL_DMA_IsEnabledIT
 *         IE         TEIE1         LL_DMA_IsEnabledIT
 *         IE         HTIE2         LL_DMA_IsEnabledIT
 *         IE         TCIE2         LL_DMA_IsEnabledIT
 *         IE         TEIE2         LL_DMA_IsEnabledIT
 *         IE         HTIE3         LL_DMA_IsEnabledIT
 *         IE         TCIE3         LL_DMA_IsEnabledIT
 *         IE         TEIE3         LL_DMA_IsEnabledIT
 *         IE         HTIE4         LL_DMA_IsEnabledIT
 *         IE         TCIE4         LL_DMA_IsEnabledIT
 *         IE         TEIE4         LL_DMA_IsEnabledIT
 * @param  DMAx DMA instance
 * @param  Interrupts This parameter can be one of the following values:
 *         @arg @ref LL_DMA_IE_HTIE0
 *         @arg @ref LL_DMA_IE_TCIE0
 *         @arg @ref LL_DMA_IE_TEIE0
 *         @arg @ref LL_DMA_IE_HTIE1
 *         @arg @ref LL_DMA_IE_TCIE1
 *         @arg @ref LL_DMA_IE_TEIE1
 *         @arg @ref LL_DMA_IE_HTIE2
 *         @arg @ref LL_DMA_IE_TCIE2
 *         @arg @ref LL_DMA_IE_TEIE2
 *         @arg @ref LL_DMA_IE_HTIE3
 *         @arg @ref LL_DMA_IE_TCIE3
 *         @arg @ref LL_DMA_IE_TEIE3
 *         @arg @ref LL_DMA_IE_HTIE4
 *         @arg @ref LL_DMA_IE_TCIE4
 *         @arg @ref LL_DMA_IE_TEIE4
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledIT(DMA_TypeDef *DMAx, uint32_t Interrupts)
{
    return (READ_BIT(DMAx->IE, Interrupts) == Interrupts);
}

/**
 * @brief  Check if M2M Channel half transfer interrupt is enabled or not.
 * @rmtoll MIE         HTIE0         LL_DMA_IsEnabledM2MIT
 *         MIE         TCIE0         LL_DMA_IsEnabledM2MIT
 *         MIE         TEIE0         LL_DMA_IsEnabledM2MIT
 *         MIE         HTIE1         LL_DMA_IsEnabledM2MIT
 *         MIE         TCIE1         LL_DMA_IsEnabledM2MIT
 *         MIE         TEIE1         LL_DMA_IsEnabledM2MIT
 * @param  DMAx DMA instance
 * @param  Interrupts This parameter can be one of the following values:
 *         @arg @ref LL_DMA_MIE_HTIE0
 *         @arg @ref LL_DMA_MIE_TCIE0
 *         @arg @ref LL_DMA_MIE_TEIE0
 *         @arg @ref LL_DMA_MIE_HTIE1
 *         @arg @ref LL_DMA_MIE_TCIE1
 *         @arg @ref LL_DMA_MIE_TEIE1
 * @retval State of bit 1 or 0.
 */
__STATIC_INLINE uint32_t LL_DMA_IsEnabledM2MIT(DMA_TypeDef *DMAx, uint32_t Interrupts)
{
    return (READ_BIT(DMAx->MIE, Interrupts) == Interrupts);
}

/**
 * @}
 */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup DMA_LL_EF_Init Initialization and de-initialization functions
 * @{
 */
ErrorStatus LL_DMA_Init(DMA_TypeDef *DMAx, uint32_t Channel, LL_DMA_InitTypeDef *DMA_InitStruct);
ErrorStatus LL_DMA_DeInit(DMA_TypeDef *DMAx);
void LL_DMA_StructInit(LL_DMA_InitTypeDef *DMA_InitStruct);
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

#endif /* DMA00 || DMA10 || DMA01 || DMA11 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RN_LL_DMA_H */
