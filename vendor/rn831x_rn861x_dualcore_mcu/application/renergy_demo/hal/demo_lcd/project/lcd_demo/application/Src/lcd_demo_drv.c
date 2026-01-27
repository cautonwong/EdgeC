/**
  *****************************************************************************
  * @file   lcd_demo_drv.c
  * @author Renergy Software Developers
  * @brief  Test source code
  *
  *****************************************************************************
  * @attention
  *
  * Copyright (c) 2025, Renergy Technology Inc.
  * All rights reserved.
  *
  *****************************************************************************
  */
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wunused-variable"

#include "lcd_demo_drv.h"

/* LCD_BUF data storage buffer*/
uint8_t LcdDisplayBuf[LCD_SEG_NUM];
uint32_t BufSize = LCD_SEG_NUM;

/**************************** LCD Digital tube table ****************************/
/* Digital tube table, defined by user */
const uint16_t DigTubeTable[DIGTUBE_NUMBER][TUBESEGNUM]=
{
    LCD_DISPLAY_3A,    	LCD_DISPLAY_3B,    	LCD_DISPLAY_3C,    	LCD_DISPLAY_3D,    	LCD_DISPLAY_3E,    	LCD_DISPLAY_3F,    	LCD_DISPLAY_3G,
    LCD_DISPLAY_4A,    	LCD_DISPLAY_4B,    	LCD_DISPLAY_4C,    	LCD_DISPLAY_4D,    	LCD_DISPLAY_4E,    	LCD_DISPLAY_4F,    	LCD_DISPLAY_4G,
    LCD_DISPLAY_5A,    	LCD_DISPLAY_5B,    	LCD_DISPLAY_5C,    	LCD_DISPLAY_5D,    	LCD_DISPLAY_5E,    	LCD_DISPLAY_5F,    	LCD_DISPLAY_5G,
    LCD_DISPLAY_6A,    	LCD_DISPLAY_6B,    	LCD_DISPLAY_6C,    	LCD_DISPLAY_6D,    	LCD_DISPLAY_6E,    	LCD_DISPLAY_6F,    	LCD_DISPLAY_6G,
    LCD_DISPLAY_7A,    	LCD_DISPLAY_7B,    	LCD_DISPLAY_7C,    	LCD_DISPLAY_7D,    	LCD_DISPLAY_7E,    	LCD_DISPLAY_7F,    	LCD_DISPLAY_7G,
    LCD_DISPLAY_8A,    	LCD_DISPLAY_8B,    	LCD_DISPLAY_8C,    	LCD_DISPLAY_8D,    	LCD_DISPLAY_8E,    	LCD_DISPLAY_8F,    	LCD_DISPLAY_8G,
};

/**************************** LCD IO ****************************/
/* LCD IO, defined by user */
#define LCD_IO_NUM     46U
const LCD_GPIO_Config LCD_GPIO_ARRY[LCD_IO_NUM] =
{
    //LCD pin SEG
    {GPIOB, LL_GPIO_PIN_8},  {GPIOB, LL_GPIO_PIN_9},  {GPIOB, LL_GPIO_PIN_10}, {GPIOB, LL_GPIO_PIN_11}, {GPIOB, LL_GPIO_PIN_12},
    {GPIOB, LL_GPIO_PIN_14}, {GPIOB, LL_GPIO_PIN_15},
    {GPIOC, LL_GPIO_PIN_0 }, {GPIOC, LL_GPIO_PIN_1 }, {GPIOC, LL_GPIO_PIN_2 }, {GPIOC, LL_GPIO_PIN_3}, {GPIOC, LL_GPIO_PIN_4},
    {GPIOC, LL_GPIO_PIN_5 }, {GPIOC, LL_GPIO_PIN_7 }, {GPIOC, LL_GPIO_PIN_8 }, {GPIOC, LL_GPIO_PIN_9}, {GPIOC, LL_GPIO_PIN_10},
    {GPIOC, LL_GPIO_PIN_11}, {GPIOC, LL_GPIO_PIN_12}, {GPIOC, LL_GPIO_PIN_13},
    {GPIOF, LL_GPIO_PIN_1},  {GPIOF, LL_GPIO_PIN_2},  {GPIOF, LL_GPIO_PIN_3}, {GPIOF, LL_GPIO_PIN_4}, {GPIOF, LL_GPIO_PIN_5},
    {GPIOF, LL_GPIO_PIN_6},
    {GPIOE, LL_GPIO_PIN_6},  {GPIOE, LL_GPIO_PIN_7 }, {GPIOE, LL_GPIO_PIN_8 }, {GPIOE, LL_GPIO_PIN_9},
    {GPIOE, LL_GPIO_PIN_10}, {GPIOE, LL_GPIO_PIN_11},
	//COM0 ~ COM7
	{GPIOF, LL_GPIO_PIN_8}, {GPIOF, LL_GPIO_PIN_9}, {GPIOF, LL_GPIO_PIN_10}, {GPIOF, LL_GPIO_PIN_11}, {GPIOF, LL_GPIO_PIN_12},
	{GPIOF, LL_GPIO_PIN_13}, {GPIOF, LL_GPIO_PIN_14}, {GPIOF, LL_GPIO_PIN_15},
    //LCDVA~LCDVD
    {GPIOD, LL_GPIO_PIN_1}, {GPIOD, LL_GPIO_PIN_2},   {GPIOD, LL_GPIO_PIN_3}, {GPIOD, LL_GPIO_PIN_4},
    //LCDVP1, LCDVP2
    {GPIOC, LL_GPIO_PIN_15}, {GPIOD, LL_GPIO_PIN_0}
};

/* LCD init and display demo */
void Lcd_Demo_Fun1(void)
{
    LCD_HandleTypeDef lcd_handle;

    /* configure COM7 ~ COM5 alternate function */
    LL_SYSTEM_CORE0_UnlockSystemPSW();
    LL_SYSTEM_CORE0_MOD0_EnableClock(LL_SYSTEM_CORE0_MOD0_PERIPH_LCD);/* Enable LCD clock */
    LL_LCD_Disable(LCD);
    LL_LCD_SetAF3(LCD, LL_LCD_AF_COM);
    LL_LCD_SetAF2(LCD, LL_LCD_AF_COM);
    LL_LCD_SetAF1(LCD, LL_LCD_AF_COM);
    LL_LCD_SetAF0(LCD, LL_LCD_AF_COM);

    /* 1. Init LCD module */
    lcd_handle.Instance           = LCD;
    lcd_handle.LcdGpioArray       = LCD_GPIO_ARRY;    //LCD IO array, define by user
    lcd_handle.ArrayLength        = LCD_IO_NUM;       //LCD IO number, define by user
    lcd_handle.LcdValtage         = 4500U;            //LCD screen voltage, unit millivolt(mV)
    lcd_handle.Init.Pump          = LCD_PUMP_ON;      //If LCD screen voltage > VCC, should config Charge Pump on
    lcd_handle.Init.Bias          = LCD_BIAS_1_4;
    lcd_handle.Init.Duty          = LCD_DUTY_1_8;
    lcd_handle.Init.DriveType     = LCD_DRIVE_B;
    HAL_LCD_Init(&lcd_handle);

    /* 2. Display all screen */
    HAL_LCD_DisplayAll(LcdDisplayBuf, BufSize);

    /* 3. Clear screen */
    HAL_LCD_ClearScreen(LcdDisplayBuf, BufSize);

    /* 4. Clrae DisplayBuf */
    HAL_LCD_ClearDisplayBuf(LcdDisplayBuf, BufSize);

    /* 5. Display digital number "123456" */
    HAL_LCD_SetOneDigit(LcdDisplayBuf, DigTubeTable, 0U, DISPLAY_1);
    HAL_LCD_SetOneDigit(LcdDisplayBuf, DigTubeTable, 1U, DISPLAY_2);
    HAL_LCD_SetOneDigit(LcdDisplayBuf, DigTubeTable, 2U, DISPLAY_3);
    HAL_LCD_SetOneDigit(LcdDisplayBuf, DigTubeTable, 3U, DISPLAY_4);
    HAL_LCD_SetOneDigit(LcdDisplayBuf, DigTubeTable, 4U, DISPLAY_5);
    HAL_LCD_SetOneDigit(LcdDisplayBuf, DigTubeTable, 5U, DISPLAY_6);
    /* 6. Refresh the LCD screen, display LcdDisplayBuf data */
    HAL_LCD_RefreshScreen(LcdDisplayBuf, BufSize);

    /* 7. Display special characters */
    HAL_LCD_SetOneChar(LcdDisplayBuf, LCD_DISPLAY_LOGO_HOME, LCD_SEGMENT_ON);
    HAL_LCD_SetOneChar(LcdDisplayBuf, LCD_DISPLAY_Ua, LCD_SEGMENT_ON);
    HAL_LCD_SetOneChar(LcdDisplayBuf, LCD_DISPLAY_Ub, LCD_SEGMENT_ON);
    HAL_LCD_SetOneChar(LcdDisplayBuf, LCD_DISPLAY_Uc, LCD_SEGMENT_ON);
    //Refresh the LCD screen, display LcdDisplayBuf data
    HAL_LCD_RefreshScreen(LcdDisplayBuf, BufSize);
}

//HAL_LCD_MspInit implementation
void HAL_LCD_MspInit(LCD_HandleTypeDef *hlcd)
{
	LL_SYSTEM_CORE0_UnlockSystemPSW();
    /* Enable LCD clock */
    LL_SYSTEM_CORE0_MOD0_EnableClock(LL_SYSTEM_CORE0_MOD0_PERIPH_LCD);
	//Enable LCD GPIO clock
    LL_SYSTEM_CORE0_PMOD0_EnableClock(LL_SYSTEM_CORE0_PMOD0_CKE_PERIPH_GPIO);
}


#pragma GCC diagnostic pop
