/**
 ******************************************************************************
 * @file   rn_rv_ll_lcd.c
 * @author Renergy Software Team
 * @brief  LCD LL module driver
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @defgroup LCD LCD
 * @brief LCD HAL module driver
 * @{
 */

#ifdef HAL_LCD_MODULE_ENABLED

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/** @defgroup LCD_Private_Functions LCD Private Functions
 * @{
 */
static uint32_t LCD_GetBiasLevel(uint32_t Vref, uint32_t Bias, uint32_t LcdVoltage);
static uint32_t LCD_GetResLdo(uint32_t LcdVoltage);
/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup LCD_Exported_Functions LCD Exported Functions
 * @{
 */

/** @defgroup LCD_Exported_Functions_Group1 Initialization and de-initialization functions
 *  @brief    Initialization and Configuration functions
 *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
  [..]
    This section provides functions allowing to initialize and de-initialize the LCD
    to be ready for use.

@endverbatim
 * @{
 */

/**
 * @brief  Initializes the LCD according to the specified parameters in the Init.
 * @note   The parameter LcdValtage in hlcd is LCD screen voltage level (unit is millivolt).
 * @note   LCD instance must be disabled before initializing.
 * @param  hlcd LCD handle
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_LCD_Init(LCD_HandleTypeDef *hlcd)
{
    uint32_t i, biasLevel, ldoVol;
    HAL_StatusTypeDef hal_status = HAL_OK;

    /* Check GPIO config array and length */
    if((hlcd->LcdGpioArray == NULL) || (hlcd->ArrayLength == 0))
    {
        return HAL_ERROR;
    }

    /* Check init parameter */
    assert_param(IS_LCD_ALL_INSTANCE(hlcd->Instance));
    assert_param(IS_LCD_PUMP(hlcd->Init.Pump));
    assert_param(IS_LCD_BIAS(hlcd->Init.Bias));
    assert_param(IS_LCD_DUTY(hlcd->Init.Duty));
    assert_param(IS_LCD_DRIVE_TYPE(hlcd->Init.DriveType));

    /* Init the low level hardware: GPIO, CLOCK, LCD, ...etc */
    /* The HAL_LCD_MspInit can be implemented in the user file */
    HAL_LCD_MspInit(hlcd);

    /* Config GPIO mode and alternate function of LCD IO */
    for(i = 0; i < hlcd->ArrayLength; i++)
    {
        LL_GPIO_SetPinMode(hlcd->LcdGpioArray[i].GPIOx, hlcd->LcdGpioArray[i].Pin, LL_GPIO_MODE_ALTERNATE);
        if(hlcd->LcdGpioArray[i].Pin < LL_GPIO_PIN_8)
        {
            LL_GPIO_SetAFPin_0_7(hlcd->LcdGpioArray[i].GPIOx, hlcd->LcdGpioArray[i].Pin, LL_GPIO_AF_15);
        }
        else
        {
            LL_GPIO_SetAFPin_8_15(hlcd->LcdGpioArray[i].GPIOx, hlcd->LcdGpioArray[i].Pin, LL_GPIO_AF_15);
        }
    }

    /* Disable LCD Module */
    LL_LCD_Disable(hlcd->Instance);
    /* Pump on-off configuration */
    LL_LCD_SetPumpOnOff(hlcd->Instance, hlcd->Init.Pump);
    /* Bias configuration */
    LL_LCD_SetBias(hlcd->Instance, hlcd->Init.Bias);
    /* Duty configuration */
    LL_LCD_SetDuty(hlcd->Instance, hlcd->Init.Duty);
    /* Drive type configuration */
    LL_LCD_SetDriveType(hlcd->Instance, hlcd->Init.DriveType);

    /* Charge pumb on, config the BIASLVL and set register RESCTL to default */
    if(hlcd->Init.Pump == LCD_PUMP_ON)
    {
        /* Config BIASLVL */
        biasLevel = LCD_GetBiasLevel(1250U, hlcd->Init.Bias, hlcd->LcdValtage);
        LL_LCD_SetPumpBiasLevel(hlcd->Instance, biasLevel);

        /* Parameters of ResCtrl_Init are set as default  */
        LL_LCD_SetOffOutputLevel(hlcd->Instance, LL_LCD_OFFLEVEL_V0);
        LL_LCD_SetOutputLDO(hlcd->Instance, LL_LCD_LDOS_3P0);
        LL_LCD_EnableFastCharge(hlcd->Instance);
        LL_LCD_SetResDriveType(hlcd->Instance, LL_LCD_RESDRIVE_ALWAYS);
        LL_LCD_SetResDriveDuration(hlcd->Instance, 0U);
        LL_LCD_SetResDriveCount(hlcd->Instance, LL_LCD_DRIVECNT_1);
        LL_LCD_SetInterResDivSignal(hlcd->Instance, LL_LCD_INTERRES_SMALL);
        LL_LCD_SetResDivMode(hlcd->Instance, LL_LCD_RESMODE_RESERVED);
    }
    /* Charge pumb off, Config parameters of register RESCTL */
    else
    {
        /* Configure LDOS */
        ldoVol = LCD_GetResLdo(hlcd->LcdValtage);
        LL_LCD_SetOutputLDO(hlcd->Instance, ldoVol);
        /* Configure fast charge fucntion */
        LL_LCD_EnableFastCharge(hlcd->Instance);
        /* Configure RES_FT */
        LL_LCD_SetResDriveCount(hlcd->Instance, LL_LCD_DRIVECNT_2);
        /* Configure RES_DT */
        LL_LCD_SetResDriveDuration(hlcd->Instance, 1U);
        /* Configure RSM */
        LL_LCD_SetInterResDivSignal(hlcd->Instance, LL_LCD_INTERRES_SMALL);
        /* Configure RES_AO */
        LL_LCD_SetResDriveType(hlcd->Instance, LL_LCD_RESDRIVE_TIMEDIV);
        /* Configure IRSN */
        LL_LCD_SetResDivMode(hlcd->Instance, LL_LCD_RESMODE_INTERRES);
    }
    /* Configure LCD clock division according to Duty, LCD scan frequence will be set to about 64Hz */
    switch(hlcd->Init.Duty)
    {
        case LL_LCD_DUTY_STATIC:
            LL_LCD_SetClockDivision(hlcd->Instance, 0xFFU);
            break;
        case LL_LCD_DUTY_1_2:
            LL_LCD_SetClockDivision(hlcd->Instance, 0x7FU);
            break;
        case LL_LCD_DUTY_1_3:
            LL_LCD_SetClockDivision(hlcd->Instance, 0x54U);
            break;
        case LL_LCD_DUTY_1_4:
            LL_LCD_SetClockDivision(hlcd->Instance, 0x3FU);
            break;
        case LL_LCD_DUTY_1_6:
            LL_LCD_SetClockDivision(hlcd->Instance, 0x2AU);
            break;
        case LL_LCD_DUTY_1_8:
            LL_LCD_SetClockDivision(hlcd->Instance, 0x1FU);
            break;
        default: break;
    }
    /* Close blink function */
    LL_LCD_SetBlinkTime(hlcd->Instance, 0U);
    LL_LCD_SetTimeOff(hlcd->Instance, 0U);
    LL_LCD_SetTimeOn(hlcd->Instance, 1U);

    /* Enable LCD Module */
    LL_LCD_Enable(hlcd->Instance);

    return hal_status;
}

/**
 * @brief  De-initializes the LCD registers to their default reset values.
 * @param  LCDx LCD Instance
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_LCD_DeInit(LCD_HandleTypeDef *hlcd)
{
    /* Check the LCD handle allocation. */
    if (hlcd == NULL) {
        return HAL_ERROR;
    }

    /* Check the parameters */
    assert_param(IS_LCD_ALL_INSTANCE(LCDx));

    /* DeInit the low level hardware: CLOCK, LCD, ...etc */
    /* The HAL_LCD_MspDeInit can be implemented in the user file */
    HAL_LCD_MspDeInit(hlcd);

    /* Return function status. */
    return HAL_OK;
}

/**
 * @brief  Initializes the LCD MSP.
 * @param  hlcd LCD handle
 * @retval None
 */
__WEAK void HAL_LCD_MspInit(LCD_HandleTypeDef *hlcd)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hlcd);

    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_LCD_MspInit can be implemented in the user file. */
}

/**
 * @brief  DeInitialize the LCD MSP.
 * @param  hlcd LCD handle
 * @retval None
 */
__WEAK void HAL_LCD_MspDeInit(LCD_HandleTypeDef *hlcd)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hlcd);

    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_LCD_MspDeInit can be implemented in the user file. */
}

#if (USE_HAL_LCD_REGISTER_CALLBACKS == 1)
/**
  * @brief  Register a User LCD Callback
  *         To be used instead of the display off /display on callback
  * @param  hlcd LCD handle
  * @param  CallbackID ID of the callback to be registered
  *         This parameter can be one of the following values:
  *           @arg @ref HAL_LCD_DOFF_CB_ID       Display off Interrupt Callback ID
  *           @arg @ref HAL_LCD_DON_CB_ID        Display on Interrupt callback ID
  *           @arg @ref HAL_LCD_MSPINIT_CB_ID    MspInit callback ID
  *           @arg @ref HAL_LCD_MSPDEINIT_CB_ID  MspDeInit callback ID
  * @param  pCallback pointer to the Callback function
  * @retval status
  */
HAL_StatusTypeDef HAL_LCD_RegisterCallback(LCD_HandleTypeDef *hlcd, HAL_LCD_CallbackIDTypeDef CallbackID,
                                             pLCD_CallbackTypeDef pCallback)
{
    HAL_StatusTypeDef status = HAL_OK;

    if (pCallback == NULL)
    {
        status = HAL_ERROR;
    }
    else
    {
        switch (CallbackID)
        {
            case HAL_LCD_DOFF_CB_ID:
                hlcd->DisplayOffCallback = pCallback;
                break;

            case HAL_LCD_DON_CB_ID:
                hlcd->DisplayOnCallback = pCallback;
                break;

            case HAL_LCD_MSPINIT_CB_ID:
                hlcd->MspInitCallback   = pCallback;

            case HAL_LCD_MSPDEINIT_CB_ID:
                hlcd->MspDeInitCallback   = pCallback;

            default:
                status = HAL_ERROR;
                break;
        }
    }

    return status;
}

/**
  * @brief  Unregister a LCD Callback
  *         LCD Callback is redirected to the display off /display onpredefined callback
  * @param  hlcd LCD handle
  * @param  CallbackID ID of the callback to be registered
  *         This parameter can be one of the following values:
  *           @arg @ref HAL_LCD_DOFF_CB_ID       Display off Interrupt Callback ID
  *           @arg @ref HAL_LCD_DON_CB_ID        Display on Interrupt callback ID
  *           @arg @ref HAL_LCD_MSPINIT_CB_ID    MspInit callback ID
  *           @arg @ref HAL_LCD_MSPDEINIT_CB_ID  MspDeInit callback ID
  * @retval status
  */
HAL_StatusTypeDef HAL_LCD_UnRegisterCallback(LCD_HandleTypeDef *hlcd, HAL_LCD_CallbackIDTypeDef CallbackID)
{
    HAL_StatusTypeDef status = HAL_OK;

    switch (CallbackID)
    {
        case HAL_LCD_DOFF_CB_ID:
            hlcd->DisplayOffCallback = HAL_LCD_DisplayOffCallback;
            break;

        case HAL_LCD_DON_CB_ID:
            hlcd->DisplayOnCallback = HAL_LCD_DisplayOnCallback;
            break;

        case HAL_LCD_MSPINIT_CB_ID:
            hlcd->MspInitCallback   = HAL_LCD_MspInit;

        case HAL_LCD_MSPDEINIT_CB_ID:
            hlcd->MspDeInitCallback   = HAL_LCD_MspDeInit;

        default:
            status = HAL_ERROR;
            break;
    }

    return status;
}
#endif /* USE_HAL_LCD_REGISTER_CALLBACKS */

/**
 * @}
 */

/** @defgroup LCD_Exported_Functions_Group2 IO operation functions
 *  @brief   Screen operation
 *
@verbatim
 ===============================================================================
                       ##### Screen operation functions #####
 ===============================================================================

@endverbatim
 * @{
 */

/**
 * @brief  Clear display buffer and LCD_BUF.
 * @param  DisplayBuf Pointer to display buffer.
 * @param  BufSize    Display buffer size.
 * @retval None
 */
void HAL_LCD_ClearScreen(uint8_t *DisplayBuf, uint32_t BufSize)
{
    uint32_t segNum;
    for(segNum = 0; segNum < BufSize; segNum++)
    {
        DisplayBuf[segNum] = 0x0U;
        LL_LCD_SetLCDBuffer(LCD, segNum, 0x0U);
    }
}

/**
 * @brief  Display all characters of screen.
 * @param  DisplayBuf Pointer to display buffer.
 * @param  BufSize    Display buffer size.
 * @retval None
 */
void HAL_LCD_DisplayAll(uint8_t *DisplayBuf, uint32_t BufSize)
{
    uint32_t segNum;
    for(segNum = 0; segNum < BufSize; segNum++)
    {
        DisplayBuf[segNum] = 0xFFU;
        LL_LCD_SetLCDBuffer(LCD, segNum, 0xFFU);
    }
}

/**
 * @brief  Refresh screen according to DisplayBuf data.
 * @param  DisplayBuf Pointer to display buffer.
 * @param  BufSize    Buffer size of DisplayBuf.
 * @retval None
 */
void HAL_LCD_RefreshScreen(uint8_t *DisplayBuf, uint32_t BufSize)
{
    uint32_t segNum;
    for(segNum = 0; segNum < BufSize; segNum++)
    {
        LL_LCD_SetLCDBuffer(LCD, segNum, DisplayBuf[segNum]);
    }
}

/**
 * @brief  Clear only DisplayBuf, not LCD_BUF.
 * @param  DisplayBuf Pointer to display buffer.
 * @param  BufSize    DisplayBuf size.
 * @retval None
 */
void HAL_LCD_ClearDisplayBuf(uint8_t *DisplayBuf, uint32_t BufSize)
{
    uint32_t segNum;
    for(segNum = 0; segNum < BufSize; segNum++)
    {
        DisplayBuf[segNum] = 0x0U;
    }
}

/**
 * @brief  Turn on/off a special character, include point, unit, string, ..., etc.
 * @note   LcdCharMap should defined by user, high 8-bit are the index of DisplayBuf(SEG),
 *         low 8-bit are the data of DisplayBuf(COM).
 * @param  DisplayBuf Pointer to display buffer.
 * @param  LcdCharMap special characters map of LCD.
 * @param  TurnOn    Select turn on/off, this parameter can be @ref LCD_segopration_define:
 *                   @ref  LCD_SEGMENT_ON
 *                   @ref  LCD_SEGMENT_OFF
 * @retval None
 */
void HAL_LCD_SetOneChar(uint8_t *DisplayBuf, uint16_t LcdCharMap, uint8_t TurnOn)
{
    uint8_t lcdSeg, lcdCom;
    lcdSeg = (uint8_t)(LcdCharMap >> 8) & 0xFFU;
    lcdCom = (uint8_t)(LcdCharMap & 0xFFU);
    if(TurnOn == LCD_SEGMENT_ON)
    {
        DisplayBuf[lcdSeg] |= lcdCom;
    }
    else
    {
        DisplayBuf[lcdSeg] &= ~lcdCom;
    }
}

/**
 * @brief  Set one digital tube of LCD screen, include number, letters, negative sign, ..., etc.
 * @param  DisplayBuf     Pointer to display buffer.
 * @param  DigitalTable   The table of 7-segment digital tubes.
 * @param  DigitalPos     The postion of DigitalTable, display the number of the specified NumberPos.
 * @param  DisplayNumber  Display the number defined according to 7-segment digital tubes.
 * @retval None
 */
void HAL_LCD_SetOneDigit(uint8_t *DisplayBuf, const uint16_t DigitalTable[][7], uint8_t DigitalPos, uint8_t DisplayNumber)
{
    uint8_t lcdSeg, lcdCom, row;
    for(row = 0; row < 7; row++)
    {
        lcdSeg = (uint8_t)(DigitalTable[DigitalPos][row] >> 8U);
        lcdCom = (uint8_t)(DigitalTable[DigitalPos][row] & 0xFFU);
        /* Set the segment of the 7-segment digital tubes at the specified position based on DisplayNumber */
        if(DisplayNumber & (1 << row))
        {
            DisplayBuf[lcdSeg] |= lcdCom;
        }
        else
        {
            DisplayBuf[lcdSeg] &= ~lcdCom;
        }
    }
}
/**
 * @}
 */

/** @defgroup LCD_Exported_Functions_Group3 Blink configuration functions
 *  @brief    Blink configuration functions
 *
@verbatim
 ===============================================================================
            ##### Blink configuration functions #####
 ===============================================================================

@endverbatim
 * @{
 */

/**
 * @brief Set Blink time during a turn-on cycle.
 * @note  Step is 0.25s(0.25*Time), support time 0~63.75s (0<=Time<=255).
 * @note  The time for turn on and turn off is 0.25*BLINK_TIME during one TON display cycle.
 * @note  If BLINK_TIME is set to zero, LCD will remain on during the TON cycle without blink. If BLINK_TIME > 0,
 *        TON must be set to BLINK_TIME*2n.
 * @param  hlcd LCD Handle
 * @param  Time Blink time.
 * @retval None
 */
void HAL_LCD_SetBlinkTime(LCD_HandleTypeDef *hlcd, uint32_t Time)
{
    LL_LCD_SetBlinkTime(hlcd->Instance, Time);
}

/**
 * @brief Set turn off time.
 * @note  Step is 0.25s(0.25*Time), support time: 0~127.5s (0<=Time<=511). Please set to be greater than 3s.
 * @param hlcd LCD Handle
 * @param Time LCD turn off time
 * @return None
 */
void HAL_LCD_SetTurnOffTime(LCD_HandleTypeDef *hlcd, uint32_t Time)
{
    LL_LCD_SetTimeOff(hlcd->Instance, Time);
}

/**
 * @brief Set turn on time.
 * @note  Step is 0.25s(0.25*Time), support time: 0~127.5s (0<=Time<=511). Please set to be greater than 3s.
 * @param hlcd LCD Handle
 * @param Time LCD turn on time
 * @return None
 */
void HAL_LCD_SetTurnOnTime(LCD_HandleTypeDef *hlcd, uint32_t Time)
{
    LL_LCD_SetTimeOn(hlcd->Instance, Time);
}

/**
 * @brief Set LCD pump setup time.
 * @note  Setup time = 30.5us * (Time + 4), 0<=time<=65535.
 * @param hlcd LCD Handle
 * @param Time Pump setup time
 * @return None
 */
void HAL_LCD_SetPumpSetupTime(LCD_HandleTypeDef *hlcd, uint32_t Time)
{
    LL_LCD_SetPumpSetupTime(hlcd->Instance, Time);
}
/**
 * @}
 */


/** @defgroup LCD_Exported_Functions_Group4 Interrupt handle functions
 *  @brief   Interrupt handle
 *
@verbatim
 ===============================================================================
                       ##### Interrupt handle functions #####
 ===============================================================================

@endverbatim
 * @{
 */

/**
  * @brief  This function handles LCD interrupt request.
  * @param  hlcd Pointer to a LCD_HandleTypeDef structure that contains
  *                the configuration information for the specified LCD module.
  * @retval None
  */
void HAL_LCD_IRQHandler(LCD_HandleTypeDef *hlcd)
{
    /* If Display off interrupt occurred */
    if(__HAL_LCD_GET_IT(hlcd, LCD_IT_DOFF) != 0x0U)
    {
        /* Clear IT flag */
        __HAL_LCD_CLEAR_FLAG(hlcd, LCD_FLAG_IT_DOFF);

        /* Call user display off callback */
#if (USE_HAL_LCD_REGISTER_CALLBACKS)
        hlcd->DisplayOffCallback(hlcd);

#else
        /* Call legacy display off Callback */
        HAL_LCD_DisplayOffCallback(hlcd);
#endif
    }
    /* If Display on interrupt occurred */
    else if(__HAL_LCD_GET_IT(hlcd, LCD_IT_DON) != 0x0U)
    {
        /* Clear IT flag */
        __HAL_LCD_CLEAR_FLAG(hlcd, LCD_FLAG_IT_DON);

        /* Call user display on callback */
#if (USE_HAL_LCD_REGISTER_CALLBACKS)
        hlcd->DisplayOnCallback(hlcd);

#else
        /* Call legacy display on Callback */
        HAL_LCD_DisplayOnCallback(hlcd);
#endif
    }
    else
    {
        ;
    }
}

/**
  * @brief  Screen changes from on to off callbacks.
  * @param  hlcd Pointer to a LCD_HandleTypeDef structure that contains
  *                the configuration information for the specified LCD module.
  * @retval None
  */
__WEAK void HAL_LCD_DisplayOffCallback(LCD_HandleTypeDef *hlcd)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hlcd);
    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_LCD_DisplayOffCallback could be implemented in the user file
    */
}

/**
  * @brief  Screen changes from off to on callbacks.
  * @param  hlcd Pointer to a LCD_HandleTypeDef structure that contains
  *                the configuration information for the specified LCD module.
  * @retval None
  */
__WEAK void HAL_LCD_DisplayOnCallback(LCD_HandleTypeDef *hlcd)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hlcd);
    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_LCD_DisplayOnCallback could be implemented in the user file
    */
}

/**
 * @}
 */

/**
 * @}
 */


/** @addtogroup LCD_Private_Functions
 * @{
 */

/**
 * @brief  Calculate the Bias level of charge pumb.
 * @param  Vref  Voltage reference value, unit is millivolt(mV).
 * @param  Bias  LCD Bias, this parameter can be one of the following values:
 *               @arg @ref LCD_BIAS_1_3
 *               @arg @ref LCD_BIAS_1_4
 * @param  LcdVoltage LCD voltage, unit is millivolt(mV).
 * @retval None
 */
static uint32_t LCD_GetBiasLevel(uint32_t Vref, uint32_t Bias, uint32_t LcdVoltage)
{
    uint32_t biasLevel;
    uint32_t voltage = LcdVoltage*1000U;
    /* Calculate Va/Vref， Va = LcdVoltage/bias */
    if(Bias == LCD_BIAS_1_3)
    {
        voltage = ((voltage / 3U) / Vref);
    }
    else
    {
        voltage = ((voltage / 4U) / Vref);
    }
    /* Calculate bias level */
    /* BIASLVL[5] = 1 */
    if(voltage >= 1000U)
    {
        biasLevel = 0x20U + ((voltage - 1000U) * 63U)/1000U;
    }
    /* BIASLVL[5] = 0 */
    else
    {
        biasLevel = ((voltage * 63U) / 1000U) - 32U;
    }

    return biasLevel;
}

/**
 * @brief  Get LDO output voltage of resistance.
 * @param  LcdVoltage LCD voltage, unit is millivolt(mV).
 * @retval None
 */
static uint32_t LCD_GetResLdo(uint32_t LcdVoltage)
{
    uint32_t resLdo;
    if(LcdVoltage <= 2700U)
    {
        resLdo = 0x0U;
    }
    else if(LcdVoltage >= 3600U)
    {
        resLdo = 0xFU;
    }
    else
    {
        resLdo = (LcdVoltage - 2700U) / 60U;
    }
    return resLdo;
}

/**
 * @}
 */

#endif /* HAL_LCD_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
