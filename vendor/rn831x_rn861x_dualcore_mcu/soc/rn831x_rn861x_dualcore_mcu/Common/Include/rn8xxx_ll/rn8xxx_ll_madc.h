/******************************************************************************
 * @file    rn8xxx_lvdcmpsar.h
 * @author  Renergy Application Team
 * @version V1.1.1
 * @date    2023-09-07
 * @brief   Header file containing functions prototypes of LvdCmpSar HAL library.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2016 Renergy.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 *
 ******************************************************************************
 * @ Revised records
 * V1.1.1   2023-09-07  Add       Add macro definition for geting register and register bit
 * V1.1.0               Initial
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef Rn8xxx_ANA_H_
#define Rn8xxx_ANA_H_

/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll_def.h"
/* Exported define ------------------------------------------------------------*/

#define LVDCMPSAR_TIMEOUT 0x3fff
#define CLRSTATUS_TIMEOUT 0x3f
/*********************************************************************************/
#define GET_REG_ANA_AD_CTRL(ana, databits) ((ana)->AD_CTRL & GET_DATA_MASK(databits))
#define GET_REG_ANA_AD_START(ana, databits) ((ana)->AD_START & GET_DATA_MASK(databits))
#define GET_REG_ANA_AD_STAT(ana, databits) ((ana)->AD_STAT & GET_DATA_MASK(databits))
#define GET_REG_ANA_AD_DATA(ana, databits) ((ana)->AD_DATA & GET_DATA_MASK(databits))

#if defined(RN831x_RN861x_MCU_V4)
#define GET_REG_ANA_LVD_CTRL(lvdcmp, databits) ((lvdcmp)->LVDCTRL & GET_DATA_MASK(databits))
#define GET_REG_ANA_LVD_STAT(lvdcmp, databits) ((lvdcmp)->IF & GET_DATA_MASK(databits))
#else
#define GET_REG_ANA_LVD_CTRL(ana, databits) ((ana)->LVD_CTRL & GET_DATA_MASK(databits))
#define GET_REG_ANA_LVD_STAT(ana, databits) ((ana)->LVD_STAT & GET_DATA_MASK(databits))
#endif

#if defined(RN821x_RN721x_SOC_V2) || \
    defined(RN821x_RN721x_SOC_V3) || \
    defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3) || \
    defined(RN831x_RN861x_MCU_V4) || \
    defined(RN202x_RN7326_SOC_V2)
#define GET_REG_ANA_SAR_CTRL1(ana, databits) ((ana)->AD_CTRL1 & GET_DATA_MASK(databits))
#define GET_REG_ANA_SAR_DATA2(ana, databits) ((ana)->AD_DATA2 & GET_DATA_MASK(databits))
#endif

#if defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V4)
#define GET_REG_ANA_ANA_PAD(ana, databits) ((ana)->ANA_PAD & GET_DATA_MASK(databits))
#define GET_REG_ANA_ANA_RST(ana, databits) ((ana)->ANA_RST & GET_DATA_MASK(databits))
#endif

#define SET_REG_ANA_AD_CTRL(ana, data) ((ana)->AD_CTRL = (data))
#define SET_REG_ANA_AD_START(ana, data) ((ana)->AD_START = (data))
#define SET_REG_ANA_AD_STAT(ana, data) ((ana)->AD_STAT = (data))
#define SET_REG_ANA_AD_DATA(ana, data) ((ana)->AD_DATA = (data))
#define SET_REG_ANA_LVD_CTRL(ana, data) ((ana)->LVD_CTRL = (data))
#define SET_REG_ANA_LVD_STAT(ana, data) ((ana)->LVD_STAT = (data))

#if defined(RN821x_RN721x_SOC_V2) || \
    defined(RN821x_RN721x_SOC_V3) || \
    defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3) || \
    defined(RN831x_RN861x_MCU_V4) || \
    defined(RN202x_RN7326_SOC_V2)
#define SET_REG_ANA_SAR_CTRL1(ana, data) ((ana)->AD_CTRL1 = (data))
#define SET_REG_ANA_SAR_DATA2(ana, data) ((ana)->AD_DATA2 = (data))
#endif

#if defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V4)
#define SET_REG_ANA_ANA_PAD(ana, data) ((ana)->ANA_PAD = (data))
#define SET_REG_ANA_ANA_RST(ana, data) ((ana)->ANA_RST = (data))
#endif

/*********************************************************************************/

#define GET_BIT_ANA_AD_CTRL(ana, bit) (((ana)->AD_CTRL & (bit)) ? (1) : (0))
#define GET_BIT_ANA_AD_START(ana, bit) (((ana)->AD_START & (bit)) ? (1) : (0))
#define GET_BIT_ANA_AD_STAT(ana, bit) (((ana)->AD_STAT & (bit)) ? (1) : (0))
#define GET_BIT_ANA_AD_DATA(ana, bit) (((ana)->AD_DATA & (bit)) ? (1) : (0))

#if defined(RN831x_RN861x_MCU_V4)
#define GET_BIT_ANA_LVD_CTRL(lvdcmp, bit) (((lvdcmp)->LVDCTRL & (bit)) ? (1) : (0))
#define GET_BIT_ANA_LVD_STAT(lvdcmp, bit) (((lvdcmp)->IF & (bit)) ? (1) : (0))
#else
#define GET_BIT_ANA_LVD_CTRL(ana, bit) (((ana)->LVD_CTRL & (bit)) ? (1) : (0))
#define GET_BIT_ANA_LVD_STAT(ana, bit) (((ana)->LVD_STAT & (bit)) ? (1) : (0))
#endif
#define GET_BIT_ANA_SAR_CTRL1(ana, bit) (((ana)->AD_CTRL1 & (bit)) ? (1) : (0))
#define GET_BIT_ANA_SAR_DATA2(ana, bit) (((ana)->AD_DATA2 & (bit)) ? (1) : (0))
#if defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V4)
#define GET_BIT_ANA_ANA_PAD(ana, bit) (((ana)->ANA_PAD & (bit)) ? (1) : (0))
#define GET_BIT_ANA_ANA_RST(ana, bit) (((ana)->ANA_RST & (bit)) ? (1) : (0))
#endif

#define SET_BIT_ANA_AD_CTRL(ana, bit) ((ana)->AD_CTRL |= (bit))
#define SET_BIT_ANA_AD_START(ana, bit) ((ana)->AD_START |= (bit))
#define SET_BIT_ANA_AD_STAT(ana, bit) ((ana)->AD_STAT = (bit))
#if defined(RN831x_RN861x_MCU_V4)
#define SET_BIT_ANA_LVD_CTRL(ana, bit) ((ana)->LVD_CTRL |= (bit))
#define SET_BIT_ANA_LVD_STAT(ana, bit) ((ana)->LVD_STAT = (bit))
#else
#define SET_BIT_ANA_LVD_CTRL(lvdcmp, bit) ((lvdcmp)->LVDCTRL |= (bit))
#define SET_BIT_ANA_LVD_STAT(lvdcmp, bit) ((lvdcmp)->IF = (bit))
#endif

#define SET_BIT_ANA_SAR_CTRL1(ana, bit) ((ana)->AD_CTRL1 |= (bit))
#if defined(RN831x_RN861x_MCU_V2) || defined(RN831x_RN861x_MCU_V3) || defined(RN831x_RN861x_MCU_V4)
#define SET_BIT_ANA_ANA_PAD(ana, bit) ((ana)->ANA_PAD |= (bit))
#define SET_BIT_ANA_ANA_RST(ana, bit) ((ana)->ANA_RST |= (bit))
#endif

#define CLR_BIT_ANA_AD_CTRL(ana, bit) ((ana)->AD_CTRL &= (~(bit)))
#define CLR_BIT_ANA_AD_START(ana, bit) ((ana)->AD_START &= (~(bit)))
#define CLR_BIT_ANA_AD_STAT(ana, bit) (SET_BIT_ANA_AD_STAT((ana), (bit)))
#if defined(RN831x_RN861x_MCU_V4)
#define CLR_BIT_ANA_LVD_CTRL(ana, bit) ((ana)->LVD_CTRL &= (~(bit)))
#define CLR_BIT_ANA_LVD_STAT(ana, bit) ((ana)->LVD_STAT &= (~(bit)))
#else
#define CLR_BIT_ANA_LVD_CTRL(lvdcmp, bit) ((lvdcmp)->LVDCTRL &= (~(bit)))
#define CLR_BIT_ANA_LVD_STAT(lvdcmp, bit) ((lvdcmp)->IF &= (~(bit)))
#endif

#define CLR_BIT_ANA_SAR_CTRL1(ana, bit) ((ana)->AD_CTRL1 &= (~(bit)))
#if defined(RN831x_RN861x_MCU_V2)||defined(RN831x_RN861x_MCU_V3)||defined(RN831x_RN861x_MCU_V4)
#define CLR_BIT_ANA_ANA_PAD(ana, bit) ((ana)->ANA_PAD &= (~(bit)))
#endif
/* Exported enum ------------------------------------------------------------*/

/*lvd vol 配置*/
typedef enum
{
    LVDS_VOL2P3 = 0, /*芯片电源电压与"2.3V"比较*/
    LVDS_VOL2P5 = 1, /*"2.5V"*/
    LVDS_VOL2P7 = 2, /*"2.7V"*/
    LVDS_VOL2P9 = 3, /*"2.9V"*/
    LVDS_VOL3P1 = 4, /*"3.1V"*/
    LVDS_VOL3P3 = 5, /*"3.3V"*/
    LVDS_VOL3P5 = 6, /*"3.5V"*/
    LVDS_VOL3P7 = 7, /*"3.7V"*/
    LVDS_VOL3P9 = 8, /*"3.9V"*/
    LVDS_VOL4P1 = 9,
    LVDS_VOL4P3 = 10,
    LVDS_VOL4P5 = 11,
    LVDS_VOL4P7 = 12,
    LVDS_VOL4P9 = 13,
    LVDS_LVDIN0_VOL1P25 = 14, /*"外部管脚LVDIN0输入与LBGR(1.25V)进行比较"*/
} eLVD_LVDS_Typedef;

typedef enum
{
    LVDCMPSAR_IE_DISABLE = 0,
    LVDCMPSAR_IE_ENABLE = 1,
} eLVDCMPSAR_IE_TypeDef;

typedef enum
{
    CMP1_CHANNEL = 0,
    CMP2_CHANNEL = 1,
} eCMP_CH_TypeDef; /*"CMP通道"*/

typedef enum
{
    CMP_R600K_DISABLE = 0,
    CMP_R600K_ENABLE = 1,
} eCMP_R600K_TypeDef; /*"CMP2 内部 600K 电阻采样开关"*/

typedef enum
{
    CMP_HYSEN_DISABLE = 0,
    CMP_HYSEN_ENABLE = 1,
} eCMP_Hysen_TypeDef; /*"内部迟滞比较器迟滞开关"*/

typedef union
{
    struct
    {
        uint32_t Channel : 4;
        uint32_t PGA : 2;
        uint32_t IE : 1;
        uint32_t Reserved : 25;
    }bitSarCtrl;
    uint32_t SarCtrl;
} uSarCtrl_TypeDef;

typedef enum
{
    TEMPSENSOR_CHANNEL = 0,
    VBAT_CHANNEL = 1,
    AIN0_CHANNEL = 2,
    AIN1_CHANNEL = 3,
    AIN2_CHANNEL = 4,
    AIN3_CHANNEL = 5,
    AIN4_CHANNEL = 6,
    AIN5_CHANNEL = 7,
    AIN6_CHANNEL = 8,
#if defined(RN202x_RN7326_SOC_V2) || defined(RN831x_RN861x_MCU_V4)
    AIN7_CHANNEL = 9,
#endif
#if defined(RN831x_RN861x_MCU_V4)
    AIN8_CHANNEL = 10,
    AIN9_CHANNEL = 11,
#endif
} eSarCtrl_CH_TypeDef; /*"SAR ADC 通道选择"*/
typedef enum
{

    PGA_0P5 = 0,
    PGA_1P0 = 1,
    PGA_1P5 = 2,
    PGA_2P0 = 3,
} eSarCtrl_PGA_TypeDef; /*" PGA选择"*/

#if defined(RN831x_RN861x_MCU_V4)
typedef enum
{
    LVD_CH = 2,
    CMP1_CH = 0,
    CMP2_CH = 1,
} eLVDCMP_CH_TypeDef; /*"获取状态 通道选择"*/

#else
typedef enum
{
    LVD_CH = 0,
    CMP1_CH = 1,
    CMP2_CH = 2,
} eLVDCMP_CH_TypeDef; /*"获取状态 通道选择"*/
#endif
typedef enum
{
    SAR_NORMALMODE = 0,
    SAR_HIGHMODE = 1,
} eSAR_PerfMODE; /*"SAR性能模式"*/

/* Exported types ------------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
extern void LL_ANA_LVDInit(eLVD_LVDS_Typedef lvds, eLVDCMPSAR_IE_TypeDef SwitchIE);
extern void LL_ANA_LVDDisable(void);
extern void LL_ANA_CMPInit(eCMP_CH_TypeDef ch, eCMP_R600K_TypeDef SwitchR600, eCMP_Hysen_TypeDef SwitchHysen, eLVDCMPSAR_IE_TypeDef SwitchIE);
extern void LL_ANA_CMPDisable(eCMP_CH_TypeDef ch);
extern void LL_ANA_SarADCInit(eSarCtrl_CH_TypeDef ch, eSarCtrl_PGA_TypeDef pga);
extern uint32_t LL_ANA_SarADCGet(void);
extern ErrorStatus LL_ANA_LvdCmpStatCheck(eLVDCMP_CH_TypeDef ch, uint32_t checkTime);
extern ErrorStatus LL_ANA_LvdCmp_STATPowerUp(eLVDCMP_CH_TypeDef ch, uint32_t checkTime);
extern ErrorStatus LL_ANA_LvdCmp_STATPowerDown(eLVDCMP_CH_TypeDef ch, uint32_t checkTime);
#if defined(RN831x_RN861x_MCU_V2) || \
    defined(RN831x_RN861x_MCU_V3) || \
    defined(RN831x_RN861x_MCU_V4) || \
    defined(RN202x_RN7326_SOC_V2)
extern void LL_ANA_LvdCmpSarModeConfig(eSAR_PerfMODE PerfMODE);
#endif
#endif

/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
