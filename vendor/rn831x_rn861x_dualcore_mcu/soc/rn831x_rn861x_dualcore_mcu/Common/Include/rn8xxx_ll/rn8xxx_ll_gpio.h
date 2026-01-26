/*******************************************************************************
 * @file    rn8xxx_gpio.h
 * @author  Renergy Application Team
 * @version V1.1.2
 * @date    2023-10-08
 * @brief   Header file containing functions prototypes of Gpio HAL library.
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
 *******************************************************************************/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef Rn8xxx_GPIO_H_
#define Rn8xxx_GPIO_H_

/* Includes ------------------------------------------------------------------*/

#if !defined(RN831x_RN861x_MCU_V4)
/* Exported define ------------------------------------------------------------*/
/*"****************************   GPIO定义    ****************************"*/
#define PIN_0_Group (0x00U)
#define PIN_1_Group (0x08U)
#define PIN_2_Group (0x10U)
#define PIN_3_Group (0x18U)
#define PIN_4_Group (0x20U)
#define PIN_5_Group (0x28U)
#define PIN_6_Group (0x30U)
#define PIN_7_Group (0x38U)
#define PIN_8_Group (0x40U)
#define PIN_9_Group (0x48U)
#define PIN_10_Group (0x50U)
#define PIN_11_Group (0x58U)
#define PIN_12_Group (0x60U)
#define PIN_13_Group (0x68U)
#define PIN_14_Group (0x70U)

#define PIN_SegCom_Group (0x78U)
#define PIN_nMAX (PIN_SegCom_Group + 4U)
/*------------------------------------------------------------------------------------*/
#define PIN_0_0 (0U)    /*"	PIN_0_Group + 0x00	"*/
#define PIN_0_1 (1U)    /*"	PIN_0_Group + 0x01	"*/
#define PIN_0_2 (2U)    /*"	PIN_0_Group + 0x02	"*/
#define PIN_0_3 (3U)    /*"	PIN_0_Group + 0x03	"*/
#define PIN_0_4 (4U)    /*"	PIN_0_Group + 0x04	"*/
#define PIN_0_5 (5U)    /*"	PIN_0_Group + 0x05	"*/
#define PIN_0_6 (6U)    /*"	PIN_0_Group + 0x06	"*/
#define PIN_0_7 (7U)    /*"	PIN_0_Group + 0x07	"*/
#define PIN_1_0 (8U)    /*"	PIN_1_Group + 0x00	"*/
#define PIN_1_1 (9U)    /*"	PIN_1_Group + 0x01	"*/
#define PIN_1_2 (10U)   /*"	PIN_1_Group + 0x02	"*/
#define PIN_1_3 (11U)   /*"	PIN_1_Group + 0x03	"*/
#define PIN_1_4 (12U)   /*"	PIN_1_Group + 0x04	"*/
#define PIN_1_5 (13U)   /*"	PIN_1_Group + 0x05	"*/
#define PIN_1_6 (14U)   /*"	PIN_1_Group + 0x06	"*/
#define PIN_1_7 (15U)   /*"	PIN_1_Group + 0x07	"*/
#define PIN_2_0 (16U)   /*"	PIN_2_Group + 0x00	"*/
#define PIN_2_1 (17U)   /*"	PIN_2_Group + 0x01	"*/
#define PIN_2_2 (18U)   /*"	PIN_2_Group + 0x02	"*/
#define PIN_2_3 (19U)   /*"	PIN_2_Group + 0x03	"*/
#define PIN_2_4 (20U)   /*"	PIN_2_Group + 0x04	"*/
#define PIN_2_5 (21U)   /*"	PIN_2_Group + 0x05	"*/
#define PIN_2_6 (22U)   /*"	PIN_2_Group + 0x06	"*/
#define PIN_2_7 (23U)   /*"	PIN_2_Group + 0x07	"*/
#define PIN_3_0 (24U)   /*"	PIN_3_Group + 0x00	"*/
#define PIN_3_1 (25U)   /*"	PIN_3_Group + 0x01	"*/
#define PIN_3_2 (26U)   /*"	PIN_3_Group + 0x02	"*/
#define PIN_3_3 (27U)   /*"	PIN_3_Group + 0x03	"*/
#define PIN_3_4 (28U)   /*"	PIN_3_Group + 0x04	"*/
#define PIN_3_5 (29U)   /*"	PIN_3_Group + 0x05	"*/
#define PIN_3_6 (30U)   /*"	PIN_3_Group + 0x06	"*/
#define PIN_3_7 (31U)   /*"	PIN_3_Group + 0x07	"*/
#define PIN_4_0 (32U)   /*"	PIN_4_Group + 0x00	"*/
#define PIN_4_1 (33U)   /*"	PIN_4_Group + 0x01	"*/
#define PIN_4_2 (34U)   /*"	PIN_4_Group + 0x02	"*/
#define PIN_4_3 (35U)   /*"	PIN_4_Group + 0x03	"*/
#define PIN_4_4 (36U)   /*"	PIN_4_Group + 0x04	"*/
#define PIN_4_5 (37U)   /*"	PIN_4_Group + 0x05	"*/
#define PIN_4_6 (38U)   /*"	PIN_4_Group + 0x06	"*/
#define PIN_4_7 (39U)   /*"	PIN_4_Group + 0x07	"*/
#define PIN_5_0 (40U)   /*"	PIN_5_Group + 0x00	"*/
#define PIN_5_1 (41U)   /*"	PIN_5_Group + 0x01	"*/
#define PIN_5_2 (42U)   /*"	PIN_5_Group + 0x02	"*/
#define PIN_5_3 (43U)   /*"	PIN_5_Group + 0x03	"*/
#define PIN_5_4 (44U)   /*"	PIN_5_Group + 0x04	"*/
#define PIN_5_5 (45U)   /*"	PIN_5_Group + 0x05	"*/
#define PIN_5_6 (46U)   /*"	PIN_5_Group + 0x06 	当VCC无电，VBAT初次上电时，P56默认输出1HZ。"*/
#define PIN_5_7 (47U)   /*"	PIN_5_Group + 0x07	"*/
#define PIN_6_0 (48U)   /*"	PIN_6_Group + 0x00	"*/
#define PIN_6_1 (49U)   /*"	PIN_6_Group + 0x01	"*/
#define PIN_6_2 (50U)   /*"	PIN_6_Group + 0x02	"*/
#define PIN_6_3 (51U)   /*"	PIN_6_Group + 0x03	"*/
#define PIN_6_4 (52U)   /*"	PIN_6_Group + 0x04	"*/
#define PIN_6_5 (53U)   /*"	PIN_6_Group + 0x05	"*/
#define PIN_6_6 (54U)   /*"	PIN_6_Group + 0x06	"*/
#define PIN_6_7 (55U)   /*"	PIN_6_Group + 0x07	"*/
#define PIN_7_0 (56U)   /*"	PIN_7_Group + 0x00	"*/
#define PIN_7_1 (57U)   /*"	PIN_7_Group + 0x01	"*/
#define PIN_7_2 (58U)   /*"	PIN_7_Group + 0x02	"*/
#define PIN_7_3 (59U)   /*"	PIN_7_Group + 0x03	"*/
#define PIN_7_4 (60U)   /*"	PIN_7_Group + 0x04	"*/
#define PIN_7_5 (61U)   /*"	PIN_7_Group + 0x05	"*/
#define PIN_7_6 (62U)   /*"	PIN_7_Group + 0x06	"*/
#define PIN_7_7 (63U)   /*"	PIN_7_Group + 0x07	"*/
#define PIN_8_0 (64U)   /*"	PIN_8_Group + 0x00	"*/
#define PIN_8_1 (65U)   /*"	PIN_8_Group + 0x01	"*/
#define PIN_8_2 (66U)   /*"	PIN_8_Group + 0x02	"*/
#define PIN_8_3 (67U)   /*"	PIN_8_Group + 0x03	"*/
#define PIN_8_4 (68U)   /*"	PIN_8_Group + 0x04	"*/
#define PIN_8_5 (69U)   /*"	PIN_8_Group + 0x05	"*/
#define PIN_8_6 (70U)   /*"	PIN_8_Group + 0x06	"*/
#define PIN_8_7 (71U)   /*"	PIN_8_Group + 0x07	"*/
#define PIN_9_0 (72U)   /*"	PIN_9_Group + 0x00	"*/
#define PIN_9_1 (73U)   /*"	PIN_9_Group + 0x01	"*/
#define PIN_9_2 (74U)   /*"	PIN_9_Group + 0x02	"*/
#define PIN_9_3 (75U)   /*"	PIN_9_Group + 0x03	"*/
#define PIN_9_4 (76U)   /*"	PIN_9_Group + 0x04	"*/
#define PIN_9_5 (77U)   /*"	PIN_9_Group + 0x05	"*/
#define PIN_9_6 (78U)   /*"	PIN_9_Group + 0x06	"*/
#define PIN_9_7 (79U)   /*"	PIN_9_Group + 0x07	"*/
#define PIN_10_0 (80U)  /*"	PIN_10_Group + 0x00	"*/
#define PIN_10_1 (81U)  /*"	PIN_10_Group + 0x01	"*/
#define PIN_10_2 (82U)  /*"	PIN_10_Group + 0x02	"*/
#define PIN_10_3 (83U)  /*"	PIN_10_Group + 0x03	"*/
#define PIN_10_4 (84U)  /*"	PIN_10_Group + 0x04	"*/
#define PIN_10_5 (85U)  /*"	PIN_10_Group + 0x05	"*/
#define PIN_10_6 (86U)  /*"	PIN_10_Group + 0x06	"*/
#define PIN_10_7 (87U)  /*"	PIN_10_Group + 0x07	"*/
#define PIN_11_0 (88U)  /*"	PIN_11_Group + 0x00	"*/
#define PIN_11_1 (89U)  /*"	PIN_11_Group + 0x01	"*/
#define PIN_11_2 (90U)  /*"	PIN_11_Group + 0x02	"*/
#define PIN_11_3 (91U)  /*"	PIN_11_Group + 0x03	"*/
#define PIN_11_4 (92U)  /*"	PIN_11_Group + 0x04	"*/
#define PIN_11_5 (93U)  /*"	PIN_11_Group + 0x05	"*/
#define PIN_11_6 (94U)  /*"	PIN_11_Group + 0x06	"*/
#define PIN_11_7 (95U)  /*"	PIN_11_Group + 0x07	"*/
#define PIN_12_0 (96U)  /*"	PIN_12_Group + 0x00	"*/
#define PIN_12_1 (97U)  /*"	PIN_12_Group + 0x01	"*/
#define PIN_12_2 (98U)  /*"	PIN_12_Group + 0x02	"*/
#define PIN_12_3 (99U)  /*"	PIN_12_Group + 0x03	"*/
#define PIN_12_4 (100U) /*"	PIN_12_Group + 0x04	"*/
#define PIN_12_5 (101U) /*"	PIN_12_Group + 0x05	"*/
#define PIN_12_6 (102U) /*"	PIN_12_Group + 0x06	"*/
#define PIN_12_7 (103U) /*"	PIN_12_Group + 0x07	"*/
#define PIN_13_0 (104U) /*"	PIN_13_Group + 0x00	"*/
#define PIN_13_1 (105U) /*"	PIN_13_Group + 0x01	"*/
#define PIN_13_2 (106U) /*"	PIN_13_Group + 0x02	"*/
#define PIN_13_3 (107U) /*"	PIN_13_Group + 0x03	"*/
#define PIN_13_4 (108U) /*"	PIN_13_Group + 0x04	"*/
#define PIN_13_5 (109U) /*"	PIN_13_Group + 0x05	"*/
#define PIN_13_6 (110U) /*"	PIN_13_Group + 0x06	"*/
#define PIN_13_7 (111U) /*"	PIN_13_Group + 0x07	"*/
#define PIN_14_0 (112U) /*"	PIN_14_Group + 0x00	"*/
#define PIN_14_1 (113U) /*"	PIN_14_Group + 0x01	"*/
#define PIN_14_2 (114U) /*"	PIN_14_Group + 0x02	"*/
#define PIN_14_3 (115U) /*"	PIN_14_Group + 0x03	"*/
#define PIN_14_4 (116U) /*"	PIN_14_Group + 0x04	"*/
#define PIN_14_5 (117U) /*"	PIN_14_Group + 0x05	"*/
#define PIN_14_6 (118U) /*"	PIN_14_Group + 0x06	"*/
#define PIN_14_7 (119U) /*"	PIN_14_Group + 0x07	"*/



/* Exported enum ------------------------------------------------------------*/

/*"GPIO方向选择"*/
typedef enum
{
    GPIO_MODE_OUT = 0U,
    GPIO_MODE_IN = 1U,
    GPIO_MODE_NORMAL = 2U, /*"输入不使能;不使用的IO口配置为此模式"*/
} eGPIODir_TypeDef;

/*"GPIO模式选择"*/
typedef enum
{
    NORMAL_IN = 0x00U,      /*""普通模式输入，即CMOS输入缓冲器类型"*/
    TTL_IN = 0x02U,         /*""TTL输入缓冲器类型"*/
    NORMAL_OUT = 0x03U,     /*"普通模式输出,推挽输出"*/
    N_OPENDRAIN_OUT = 0x04U /*"N通道漏极开路模式输出"*/
} eGPIOMode_TypeDef;

/*"GPIO输入模式选择"*/
typedef enum
{
    COMS_MODE = 0x00U,     /*"CMOS 缓冲器， Vil=0.3VCC Vih=0.7VCC；"*/
    TTL_MODE = 0x01U,      /*"TTL 缓冲器， Vil=0.16VCC Vih=0.4VCC；"*/
} eGPIOInputMode_TypeDef; /*"输入模式选择"*/

/*"GPIO输出模式选择"*/
typedef enum
{
    PushPll_MODE = 0x00U,   /*"推挽输出，默认模式"*/
    N_ch_MODE = 0x01U,      /*"N-ch 漏极开路模式；"*/
} eGPIOOutputMode_TypeDef; /*"输出模式选择"*/

/*"GPIO上拉选择"*/
typedef enum
{
    Pull_ON = 0x01U,  /*"选择上拉"*/
    Pull_OFF = 0x00U, /*"不加上拉"*/
} eGPIOPull_TypeDef;

/*"GPIO输出电平选择"*/
typedef enum
{
    Low_Level = 0x00U,  /*"输出低电平"*/
    High_Level = 0x01U, /*"输出高电平"*/
} eGPIOOutputLevel_TypeDef;

typedef enum
{
    _NORMALIO = 1U,

    _HOSCI, /*高频晶体管脚*/
    _HOSCO, /*高频晶体管脚*/
    _SWD,

    _UART0, /*"TX,RX组合同步设置"*/
    _UART1,
    _UART2,
    _UART3,
    _UART4,
    _UART5,
    _UART6,
    _UART7,
    _LPUART,
    _CAN,
    _CAN_RX,
    _CAN_TX,

    _IIC,/*"芯片只有一个IIC时选择,_IIC0,"*/
    _IIC0,
    _IIC1,
    _SPI,/*RN202x_RN7326_SOC_V2有组合关系的SPI，使用"_SPI"将IO口初始化为SPI功能，需要另外再调用LL_GPIO_SPISelNoInit函数确定当前组SPI的具体编号*/
    _SPI0,
    _SPI1,
    _SPI2,
    _SPI3,
    _SPI4,
    _ISO7816,

    _TC_IN,
    _TC0_N_0, /*"TCO的0通道的反向输出"*/
    _TC0_P_0, /*"TCO的0通道的正向输出"*/
    _TC0_N_1, /*"TCO的1通道的反向输出"*/
    _TC0_P_1, /*"TCO的1通道的正向输出"*/
    _TC1_N_0, /*"TC1的0通道的反向输出"*/
    _TC1_P_0,
    _TC1_N_1,
    _TC1_P_1,

    _RTC_OUT,
    _RTC_1S,

    _KEY,
    _KEY_OUT,

    _INT,

    /*"*******************计量*******************"*/
    _SF_OUT, /*"视在电能脉冲输出，脉冲转发功能"*/
    _SF,     /*""*/
    _PF,     /*"有功电能脉冲输出，脉冲转发功能"*/
    _QF,     /*"无功电能脉冲输出，脉冲转发功能"*/
    _SF1,    /*"脉冲转发功能，INT6口输入，SF1输出"*/
    _SF2,    /*"脉冲转发功能，INT7口输入，SF2输出"*/
    _CF0,    /*"CF0"*/
    _CF1,    /*"CF1"*/
    _CF2,    /*"CF2"*/
    _CF3,    /*"CF3"*/
    _CF4,    /*"CF4"*/
    _CF_OUT0,/*"电能脉冲"*/
    _CF_OUT1,
    _CF_OUT2,
    _CF_OUT3,
    _CF_OUT4,
    _D2F_OUT0,
    _D2F_OUT1,
    _D2F_OUT2,
    _D2F_OUT3,
    _D2F_OUT4,
    _IOCNT_OUT0,/*"脉冲转发"*/
    _IOCNT_OUT1,
    _IOCNT_OUT2,
    _IOCNT_OUT3,
    _IOCNT_OUT4,

    _ZXOUT,
    /*"*******************模拟*******************"*/
    _AIN, /*"SDAIN,AIN "*/
    _CMP2,
    _LVD,
    _CTSWO,
    _CTCMPA,
    _CTCMPB,
    _CTCMPC,

    _LCDV, /*"gpio配置LCDVA, LCDVB, LCDVC, LCDVD, LCDVP1, LCDVP2"*/
    _SEG,  /*"SEG/COM配置为SEG"*/
    _COM,  /*"SEG/COM配置为COM"*/

    _ADC_CLKO,
    _IA_IN,
    _IB_IN,
    _IC_IN,
    _U_IN,
    _TRIG_OUT,

} eGPIOFunction_TypeDef;
#ifdef RN202x_RN7326_SOC_V2
typedef enum
{
	GPIO_SPI_SelNo0 = 0,/*"P31~34:SPI2(高速 SPIS)； P06~P11:SPI0(SPIM/S)；      P12~P15:SPI1(SPIM/S)；"*/
	GPIO_SPI_SelNo1 = 1,/*"P31~34:SPI2(高速 SPIS)； P06~P11:SPI1(SPIM/S)；      P12~P15:SPI0(SPIM/S)；"*/
	GPIO_SPI_SelNo2 = 2,/*"P31~34:SPI1(SPIM/S)；    P06~P11:SPI0(SPIM/S)；      P12~P15:SPI2(高速 SPIS)；"*/
	GPIO_SPI_SelNo3 = 3,/*"P31~34:SPI1(SPIM/S)；    P06~P11:SPI2(高速 SPIS)；   P12~P15:SPI0(SPIM/S)；"*/
	GPIO_SPI_SelNo4 = 4,/*"P31~34:SPI0(SPIM/S)；    P06~P11:SPI2(高速 SPIS)；   P12~P15:SPI1(SPIM/S)；"*/
	GPIO_SPI_SelNo5 = 5,/*"P31~34:SPI0(SPIM/S)；    P06~P11:SPI1(SPIM/S)；      P12~P15:SPI2(高速 SPIS)；"*/
}eGPIO_SPISelNo_TypeDef;
#endif
/* Exported types ------------------------------------------------------------*/
/** @defgroup GPIO_Exported_Types GPIO Exported Types
 * @{
 */
/**
 * @brief   GPIO Init structure definition
 */
typedef struct
{
    uint8_t Pin;                         /*!< Specifies the GPIO pins to be configured.
                                             This parameter can be any value of @ref GPIO_pins */
    eGPIOFunction_TypeDef Mode;           /*!< 复用模式 */
    eGPIOOutputLevel_TypeDef OutputLevel; /*"输出电平选择"*/
    eGPIOPull_TypeDef Pull;               /*!< 上拉选择 */
    eGPIODir_TypeDef Dir;                 /*!< 输入输出选择 */
    eGPIOInputMode_TypeDef InputMode;     /*!< 输入模式*/
    eGPIOOutputMode_TypeDef OutputMode;   /*!< 输出模式*/
} GPIO_InitTypeDef;

/* Exported data  ----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
extern void LL_GPIO_SetPin(uint8_t pin_id, uint8_t value);
extern uint8_t LL_GPIO_ReadPin(uint8_t pin_id);
extern void LL_GPIO_OverturnPin(uint8_t pin_id);
extern void LL_GPIO_CfgPullMode(uint8_t pin_id, eGPIOPull_TypeDef mode);
extern void LL_GPIO_CfgInputMode(uint8_t pin_id, eGPIOInputMode_TypeDef mode);
extern void LL_GPIO_CfgOutputMode(uint8_t pin_id, eGPIOOutputMode_TypeDef mode);
extern void LL_GPIO_CfgDirMode(uint8_t pin_id, eGPIODir_TypeDef dir);
extern void LL_GPIO_CfgIOMode(uint8_t pin_id, eGPIODir_TypeDef dir, eGPIOMode_TypeDef mode);
extern ErrorStatus LL_GPIO_CfgFun(uint8_t pin_id, eGPIOFunction_TypeDef Fun);
extern void LL_GPIO_Init(const GPIO_InitTypeDef *GPIO_Init);
extern void LL_GPIO_CfgInit(uint8_t Pin,                         /*!< IO口ID */
                            eGPIOFunction_TypeDef Mode,           /*!< 复用模式 */
                            eGPIOOutputLevel_TypeDef OutputLevel, /*"输出电平选择"*/
                            eGPIOPull_TypeDef Pull,               /*!< 上拉选择 */
                            eGPIODir_TypeDef Dir,                 /*!< 输入输出选择 */
                            eGPIOInputMode_TypeDef InputMode,     /*!< 输入模式*/
                            eGPIOOutputMode_TypeDef OutputMode);  /*!< 输出模式*/
#ifdef RN202x_RN7326_SOC_V2
extern void LL_GPIO_SPISelNoInit(eGPIO_SPISelNo_TypeDef gpio_spi_sel);
#endif

#else
#include "rn8xxx_ll_def.h"

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/**
 * @brief  GPIO Debug Password
 */
#define LL_GPIO_DBG_ACCESS_PASSWORD1        0x55DD77EE
#define LL_GPIO_DBG_ACCESS_PASSWORD2        0x66883399
#define LL_GPIO_DBG_ACCESS_PASSWORD3        0xDDAABB11

/* Private macros -----------------------------------------------------------*/
/**
 * @brief  GPIO Private Macros
 */
#define IS_GPIO_PIN_ACTION(ACTION)      (((ACTION) == GPIO_PIN_RESET) || ((ACTION) == GPIO_PIN_SET))
#define IS_GPIO_PIN(PIN)                (((((uint32_t)PIN) & GPIO_PIN_MASK ) != 0x00U) && \
                                         ((((uint32_t)PIN) & ~GPIO_PIN_MASK) == 0x00U))
#define IS_GPIO_MODE(MODE)              (((MODE) == GPIO_MODE_INPUT)         ||\
                                         ((MODE) == GPIO_MODE_OUTPUT)        ||\
                                         ((MODE) == GPIO_MODE_AF)            ||\
                                         ((MODE) == GPIO_MODE_ANALOG))
#define IS_GPIO_OUTPUT(OUTPUT)          ((OUTPUT == GPIO_OUTPUT_PP) || (OUTPUT == GPIO_OUTPUT_OD))
#define IS_GPIO_INPUT(INPUT)            ((INPUT == GPIO_INPUT_CMOS) || (INPUT == GPIO_INPUT_TTL))
#define IS_GPIO_SPEED(SPEED)            (((SPEED) == GPIO_SPEED_FREQ_NORMAL) || ((SPEED) == GPIO_SPEED_FREQ_HIGH))
#define IS_GPIO_PULL(PULL)              (((PULL) == GPIO_NOPULL) || ((PULL) == GPIO_PULLUP) || \
                                        ((PULL) == GPIO_PULLDOWN))
#define IS_GPIO_AF(AF)                  (((AF) == GPIO_AF0_SYS )   ||\
                                         ((AF) == GPIO_AF1_QSPI0 )   ||\
                                         ((AF) == GPIO_AF2_SPI0_QSPI1 )   ||\
                                         ((AF) == GPIO_AF3_SPI1 )   ||\
                                         ((AF) == GPIO_AF4_SPI2_UART0_UART2 )   ||\
                                         ((AF) == GPIO_AF5_SPI3_UART1_UART3 )   ||\
                                         ((AF) == GPIO_AF6_UART0_2_4_6 )   ||\
                                         ((AF) == GPIO_AF7_UART1_3_5_7 )   ||\
                                         ((AF) == GPIO_AF8_LPUART_CAN_RTC1S )   ||\
                                         ((AF) == GPIO_AF9_I2C_ISO7816 )   ||\
                                         ((AF) == GPIO_AF10_KEY)   ||\
                                         ((AF) == GPIO_AF11_USART_TCO)   ||\
                                         ((AF) == GPIO_AF12_TCI_TCO)   ||\
                                         ((AF) == GPIO_AF13_CF_IOCNT)   ||\
                                         ((AF) == GPIO_AF14_RESERVE)   ||\
                                         ((AF) == GPIO_AF15_LCD))
#define IS_GPIO_JTAG_PIN(JTAG)          ((((JTAG) & GPIO_JTAG_MASK) != 0x0U) && \
                                         (((JTAG) & ~GPIO_JTAG_MASK) == 0x0U))

/* Exported types -----------------------------------------------------------*/
/**
 * @brief  LL GPIO Init Structure definition
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
} GPIO_InitTypeDef;

/**
 * @brief  GPIO Bit SET and Bit RESET enumeration
 */
typedef enum __attribute__((__packed__))
{
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET
}GPIO_PinState;

/* Exported constants -------------------------------------------------------*/
/**
 * @brief  GPIO PIN
 */
#define LL_GPIO_PIN_0                       (BIT(0))     /*!< Select pin 0 */
#define LL_GPIO_PIN_1                       (BIT(1))     /*!< Select pin 1 */
#define LL_GPIO_PIN_2                       (BIT(2))     /*!< Select pin 2 */
#define LL_GPIO_PIN_3                       (BIT(3))     /*!< Select pin 3 */
#define LL_GPIO_PIN_4                       (BIT(4))     /*!< Select pin 4 */
#define LL_GPIO_PIN_5                       (BIT(5))     /*!< Select pin 5 */
#define LL_GPIO_PIN_6                       (BIT(6))     /*!< Select pin 6 */
#define LL_GPIO_PIN_7                       (BIT(7))     /*!< Select pin 7 */
#define LL_GPIO_PIN_8                       (BIT(8))     /*!< Select pin 8 */
#define LL_GPIO_PIN_9                       (BIT(9))     /*!< Select pin 9 */
#define LL_GPIO_PIN_10                      (BIT(10))    /*!< Select pin 10 */
#define LL_GPIO_PIN_11                      (BIT(11))    /*!< Select pin 11 */
#define LL_GPIO_PIN_12                      (BIT(12))    /*!< Select pin 12 */
#define LL_GPIO_PIN_13                      (BIT(13))    /*!< Select pin 13 */
#define LL_GPIO_PIN_14                      (BIT(14))    /*!< Select pin 14 */
#define LL_GPIO_PIN_15                      (BIT(15))    /*!< Select pin 15 */
#define LL_GPIO_PIN_ALL                     (BITS(0,15)) /*!< Select all pins */

/**
 * @brief  GPIO Mode
 */
#define LL_GPIO_MODE_INPUT                  GPIO_MODE_MODE0_INPUT      /*!< Select input mode */
#define LL_GPIO_MODE_OUTPUT                 GPIO_MODE_MODE0_OUTPUT     /*!< Select output mode */
#define LL_GPIO_MODE_ALTERNATE              GPIO_MODE_MODE0_ALTERNATE  /*!< Select alternate function mode */
#define LL_GPIO_MODE_ANALOG                 GPIO_MODE_MODE0_ANALOG     /*!< Select analog mode */

/**
 * @brief  GPIO Output Type
 */
#define LL_GPIO_OUTPUT_PUSHPULL             GPIO_OTYPE_OTYPE0_NORMAL    /*!< Select push-pull as output type */
#define LL_GPIO_OUTPUT_OPENDRAIN            GPIO_OTYPE_OTYPE0_OPENDRAIN /*!< Select open-drain as output type */

/**
 * @brief  GPIO Input Type
 */
#define LL_GPIO_INPUT_CMOS                  GPIO_ITYPE_ITYPE0_CMOS /*!< Select CMOS buffer as input type, Vil=0.3Vcc, Vih=0.7Vcc */
#define LL_GPIO_INPUT_TTL                   GPIO_ITYPE_ITYPE0_TTL  /*!< Select TTL buffer as input type, Vil=0.16Vcc, Vih=0.4Vcc */

/**
 * @brief  GPIO Output Speed
 */
#define LL_GPIO_SPEED_FREQ_NORMAL           GPIO_OSPEED_OSPEED0_NORMAL    /*!< Select I/O normal output speed */
#define LL_GPIO_SPEED_FREQ_HIGH             GPIO_OSPEED_OSPEED0_HIGHSPEED /*!< Select I/O fast output speed */

/**
 * @brief  GPIO Pull Up Pull Down
 */
#define LL_GPIO_PULL_NO                     GPIO_UPDN_UPDN0_FLOATING /*!< Select I/O no pull */
#define LL_GPIO_PULL_UP                     GPIO_UPDN_UPDN0_PULLUP   /*!< Select I/O pull up */
#define LL_GPIO_PULL_DOWN                   GPIO_UPDN_UPDN0_PULLDOWN /*!< Select I/O pull down */

/**
 * @brief  GPIO Alternate Function
 */
#define LL_GPIO_AF_0                        (0x0000000U) /*!< Select alternate function 0 */
#define LL_GPIO_AF_1                        (0x0000001U) /*!< Select alternate function 1 */
#define LL_GPIO_AF_2                        (0x0000002U) /*!< Select alternate function 2 */
#define LL_GPIO_AF_3                        (0x0000003U) /*!< Select alternate function 3 */
#define LL_GPIO_AF_4                        (0x0000004U) /*!< Select alternate function 4 */
#define LL_GPIO_AF_5                        (0x0000005U) /*!< Select alternate function 5 */
#define LL_GPIO_AF_6                        (0x0000006U) /*!< Select alternate function 6 */
#define LL_GPIO_AF_7                        (0x0000007U) /*!< Select alternate function 7 */
#define LL_GPIO_AF_8                        (0x0000008U) /*!< Select alternate function 8 */
#define LL_GPIO_AF_9                        (0x0000009U) /*!< Select alternate function 9 */
#define LL_GPIO_AF_10                       (0x000000AU) /*!< Select alternate function 10 */
#define LL_GPIO_AF_11                       (0x000000BU) /*!< Select alternate function 11 */
#define LL_GPIO_AF_12                       (0x000000CU) /*!< Select alternate function 12 */
#define LL_GPIO_AF_13                       (0x000000DU) /*!< Select alternate function 13 */
#define LL_GPIO_AF_14                       (0x000000EU) /*!< Select alternate function 14 */
#define LL_GPIO_AF_15                       (0x000000FU) /*!< Select alternate function 15 */

/**
 * @brief  GPIO JTAG pin
 */
#define LL_GPIO_JTDO                        GPIO_DBG_JTDO_EN /*!< Select JTDO pin of JTAG */
#define LL_GPIO_JTDI                        GPIO_DBG_JTDI_EN /*!< Select JTDI pin of JTAG */
#define LL_GPIO_JTMS                        GPIO_DBG_JTMS_EN /*!< Select JTMS pin of JTAG */
#define LL_GPIO_JTCK                        GPIO_DBG_JTCK_EN /*!< Select JTCK pin of JTAG */

/**
 * @brief  GPIO pins define
 */
#define GPIO_PIN_0                          ((uint16_t)0x0001)  /* Pin 0 selected */
#define GPIO_PIN_1                          ((uint16_t)0x0002)  /* Pin 1 selected */
#define GPIO_PIN_2                          ((uint16_t)0x0004)  /* Pin 2 selected */
#define GPIO_PIN_3                          ((uint16_t)0x0008)  /* Pin 3 selected */
#define GPIO_PIN_4                          ((uint16_t)0x0010)  /* Pin 4 selected */
#define GPIO_PIN_5                          ((uint16_t)0x0020)  /* Pin 5 selected */
#define GPIO_PIN_6                          ((uint16_t)0x0040)  /* Pin 6 selected */
#define GPIO_PIN_7                          ((uint16_t)0x0080)  /* Pin 7 selected */
#define GPIO_PIN_8                          ((uint16_t)0x0100)  /* Pin 8 selected */
#define GPIO_PIN_9                          ((uint16_t)0x0200)  /* Pin 9 selected */
#define GPIO_PIN_10                         ((uint16_t)0x0400)  /* Pin 10 selected */
#define GPIO_PIN_11                         ((uint16_t)0x0800)  /* Pin 11 selected */
#define GPIO_PIN_12                         ((uint16_t)0x1000)  /* Pin 12 selected */
#define GPIO_PIN_13                         ((uint16_t)0x2000)  /* Pin 13 selected */
#define GPIO_PIN_14                         ((uint16_t)0x4000)  /* Pin 14 selected */
#define GPIO_PIN_15                         ((uint16_t)0x8000)  /* Pin 15 selected */
#define GPIO_PIN_All                        ((uint16_t)0xFFFF)  /* All pins selected */
#define GPIO_PIN_MASK                       0x0000FFFFU         /* PIN mask */

/**
 * @brief  GPIO Configuration Mode
 */
#define GPIO_MODE_INPUT                     LL_GPIO_MODE_INPUT      /*!< Input Mode */
#define GPIO_MODE_OUTPUT                    LL_GPIO_MODE_OUTPUT     /*!< Output Mode */
#define GPIO_MODE_AF                        LL_GPIO_MODE_ALTERNATE  /*!< Alternate Function Mode */
#define GPIO_MODE_ANALOG                    LL_GPIO_MODE_ANALOG     /*!< Analog Mode */

/**
 * @brief  GPIO Output Type
 */
#define GPIO_OUTPUT_PP                      LL_GPIO_OUTPUT_PUSHPULL   /*!< Output push-pull type */
#define GPIO_OUTPUT_OD                      LL_GPIO_OUTPUT_OPENDRAIN  /*!< Output open-drain type */

/**
 * @brief  GPIO Input Type
 */
#define GPIO_INPUT_CMOS                     LL_GPIO_INPUT_CMOS  /*!< CMOS buffer type, please refer to the product datasheet */
#define GPIO_INPUT_TTL                      LL_GPIO_INPUT_TTL   /*!< TTL buffer type, please refer to the product datasheet */

/**
 * @brief  GPIO Output Maximum frequency
 */
#define GPIO_SPEED_FREQ_NORMAL              LL_GPIO_SPEED_FREQ_NORMAL  /*!< Normal output speed, please refer to the product datasheet */
#define GPIO_SPEED_FREQ_HIGH                LL_GPIO_SPEED_FREQ_HIGH    /*!< Fast output speed, please refer to the product datasheet */

/**
 * @brief  GPIO Pull-Up or Pull-Down Activation
 */
#define GPIO_NOPULL                         LL_GPIO_PULL_NO     /*!< No Pull-up or Pull-down activation */
#define GPIO_PULLUP                         LL_GPIO_PULL_UP     /*!< Pull-up activation */
#define GPIO_PULLDOWN                       LL_GPIO_PULL_DOWN   /*!< Pull-down activation */

/**
 * @brief  GPIO alternate function
 */
#define GPIO_AF0_SYS                        LL_GPIO_AF_0
#define GPIO_AF1_QSPI0                      LL_GPIO_AF_1
#define GPIO_AF2_SPI0_QSPI1                 LL_GPIO_AF_2
#define GPIO_AF3_SPI1                       LL_GPIO_AF_3
#define GPIO_AF4_SPI2_UART0_UART2           LL_GPIO_AF_4
#define GPIO_AF5_SPI3_UART1_UART3           LL_GPIO_AF_5
#define GPIO_AF6_UART0_2_4_6                LL_GPIO_AF_6
#define GPIO_AF7_UART1_3_5_7                LL_GPIO_AF_7
#define GPIO_AF8_LPUART_CAN_RTC1S           LL_GPIO_AF_8
#define GPIO_AF9_I2C_ISO7816                LL_GPIO_AF_9
#define GPIO_AF10_KEY                       LL_GPIO_AF_10
#define GPIO_AF11_USART_TCO                 LL_GPIO_AF_11
#define GPIO_AF12_TCI_TCO                   LL_GPIO_AF_12
#define GPIO_AF13_CF_IOCNT                  LL_GPIO_AF_13
#define GPIO_AF14_RESERVE                   LL_GPIO_AF_14
#define GPIO_AF15_LCD                       LL_GPIO_AF_15

/**
 * @brief  GPIO_JTAG_pin_define GPIO JTAG pin define
 */
#define GPIO_JTCK                           (0x00000001U)
#define GPIO_JTMS                           (0x00000002U)
#define GPIO_JTDI                           (0x00000004U)
#define GPIO_JTDO                           (0x00000008U)
#define GPIO_JTAG_MASK                      (0x0000000FU)

/* Exported macro -----------------------------------------------------------*/
/**
 * @brief  GPIO_LL_EM_WRITE_READ Common Write and read registers Macros
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

/* Exported functions -------------------------------------------------------*/
/**
 * @brief  GPIO_LL_EF_Configuration Configuration
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

/* Initialization and de-initialization functions ****************************/
void          LL_GPIO_Init(GPIO_TypeDef  *GPIOx, GPIO_InitTypeDef *GPIO_Init);
void          LL_GPIO_DeInit(GPIO_TypeDef  *GPIOx, uint32_t GPIO_Pin);

/* IO operation functions ****************************************************/
GPIO_PinState LL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void          LL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);

/* Enable JTAG Pin functions *************************************************/
void          LL_GPIO_EnableJTAG(GPIO_DBG_TypeDef *GPIO_DBGx, uint32_t JTAG_PIN);
void          LL_GPIO_DisableJTAG(GPIO_DBG_TypeDef *GPIO_DBGx, uint32_t JTAG_PIN);

#endif
#endif

/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
