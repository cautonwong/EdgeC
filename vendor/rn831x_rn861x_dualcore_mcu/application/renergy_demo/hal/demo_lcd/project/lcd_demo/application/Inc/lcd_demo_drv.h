/******************************************************************************
 * @file        lcd_demo_drv.h
 * @brief
 * @author      Renergy Technology
 * @note
 *
 * Copyright (C), Renergy Technology Inc. All rights reserved.
 *
 *****************************************************************************/


#ifndef __LCD_DEMO_DRV_H__
#define __LCD_DEMO_DRV_H__

#include "rn_rv_ll_system_core0.h"
#include "rn_rv_ll_wdt.h"
#include "rn_rv_hal_lcd.h"


/**************************** 7-Segment digital tube define ****************************/
/**** hardware define
*      a
*      __
*   f |__| b   __(g)
*   e |__| c
*      d
************/
#define TUBESEGNUM            0x7U

#define DIGTALSEGA            0x01U
#define DIGTALSEGB            0x02U
#define DIGTALSEGC            0x04U
#define DIGTALSEGD            0x08U
#define DIGTALSEGE            0x10U
#define DIGTALSEGF            0x20U
#define DIGTALSEGG            0x40U

/* Display Number 0~9 */
#define DISPLAY_0          (DIGTALSEGA | DIGTALSEGB | DIGTALSEGC | DIGTALSEGD | DIGTALSEGE | DIGTALSEGF)
#define DISPLAY_1          (DIGTALSEGB | DIGTALSEGC)
#define DISPLAY_2          (DIGTALSEGA | DIGTALSEGB | DIGTALSEGG | DIGTALSEGE | DIGTALSEGD)
#define DISPLAY_3          (DIGTALSEGA | DIGTALSEGB | DIGTALSEGC | DIGTALSEGD | DIGTALSEGG)
#define DISPLAY_4          (DIGTALSEGB | DIGTALSEGC | DIGTALSEGF | DIGTALSEGG)
#define DISPLAY_5          (DIGTALSEGA | DIGTALSEGC | DIGTALSEGD | DIGTALSEGF | DIGTALSEGG)
#define DISPLAY_6          (DIGTALSEGA | DIGTALSEGC | DIGTALSEGD | DIGTALSEGE | DIGTALSEGF | DIGTALSEGG)
#define DISPLAY_7          (DIGTALSEGA | DIGTALSEGB | DIGTALSEGC | DIGTALSEGF)
#define DISPLAY_8          (DIGTALSEGA | DIGTALSEGB | DIGTALSEGC | DIGTALSEGD | DIGTALSEGE | DIGTALSEGF | DIGTALSEGG)
#define DISPLAY_9          (DIGTALSEGA | DIGTALSEGB | DIGTALSEGC | DIGTALSEGD | DIGTALSEGF | DIGTALSEGG)
/* Display letter A~Z */
#define DISPLAY_A          (DIGTALSEGA | DIGTALSEGB | DIGTALSEGC | DIGTALSEGE | DIGTALSEGF | DIGTALSEGG)
#define DISPLAY_B          (DIGTALSEGC | DIGTALSEGD | DIGTALSEGE | DIGTALSEGF | DIGTALSEGG)
#define DISPLAY_C          (DIGTALSEGA | DIGTALSEGF | DIGTALSEGE | DIGTALSEGD)
#define DISPLAY_D          (DIGTALSEGB | DIGTALSEGC | DIGTALSEGD | DIGTALSEGE | DIGTALSEGG)
#define DISPLAY_E          (DIGTALSEGA | DIGTALSEGD | DIGTALSEGE | DIGTALSEGF | DIGTALSEGG)
#define DISPLAY_F          (DIGTALSEGA | DIGTALSEGE | DIGTALSEGF | DIGTALSEGG)
#define DISPLAY_G          (DIGTALSEGA | DIGTALSEGB | DIGTALSEGC | DIGTALSEGD | DIGTALSEGF | DIGTALSEGG)
#define DISPLAY_H          (DIGTALSEGF | DIGTALSEGG | DIGTALSEGE | DIGTALSEGC)
#define DISPLAY_I          (DIGTALSEGF | DIGTALSEGE)
#define DISPLAY_J          (DIGTALSEGB | DIGTALSEGC | DIGTALSEGD | DIGTALSEGE)
#define DISPLAY_K          (DIGTALSEGF | DIGTALSEGE | DIGTALSEGG)
#define DISPLAY_L          (DIGTALSEGF | DIGTALSEGE | DIGTALSEGD)
#define DISPLAY_M          (DIGTALSEGA | DIGTALSEGE | DIGTALSEGG | DIGTALSEGC)
#define DISPLAY_N          (DIGTALSEGE | DIGTALSEGG | DIGTALSEGC)
#define DISPLAY_O          (DIGTALSEGC | DIGTALSEGD | DIGTALSEGE | DIGTALSEGG)
#define DISPLAY_P          (DIGTALSEGF | DIGTALSEGE | DIGTALSEGA | DIGTALSEGB | DIGTALSEGG)
#define DISPLAY_Q          (DIGTALSEGA | DIGTALSEGB | DIGTALSEGC | DIGTALSEGF | DIGTALSEGG)
#define DISPLAY_R          (DIGTALSEGE | DIGTALSEGG)
#define DISPLAY_S          (DIGTALSEGA | DIGTALSEGC | DIGTALSEGD | DIGTALSEGF | DIGTALSEGG)
#define DISPLAY_T          (DIGTALSEGF | DIGTALSEGE | DIGTALSEGD | DIGTALSEGG)
#define DISPLAY_U          (DIGTALSEGF | DIGTALSEGE | DIGTALSEGD | DIGTALSEGC | DIGTALSEGB)
#define DISPLAY_V          (DIGTALSEGF | DIGTALSEGG | DIGTALSEGB)
#define DISPLAY_W          (DIGTALSEGE | DIGTALSEGD | DIGTALSEGC |DIGTALSEGA)
#define DISPLAY_X          (DIGTALSEGF | DIGTALSEGE | DIGTALSEGG | DIGTALSEGB | DIGTALSEGC)
#define DISPLAY_Y          (DIGTALSEGF | DIGTALSEGG | DIGTALSEGB | DIGTALSEGC | DIGTALSEGD)
#define DISPLAY_Z          (DIGTALSEGA | DIGTALSEGB | DIGTALSEGG | DIGTALSEGE | DIGTALSEGD)
/* Negative sign */
#define DISPLAY_NEG        (DIGTALSEGG)
/* No display */
#define DISPLAY_NULL       0

/**************************** Mapping relationship between LCD drawing and LCD_BUF ****************************/
/* LCD MAP define, high 8-bit are LCD SEG, low 8-bit are LCD COM*/
#define LCD_MAP(lcd_seg, lcd_com)    (((lcd_seg) << 8) | (lcd_com))

/* 1. LCD_COM, defined by user. Take the 8*40 LCD screen as an example. */
#define LCD_COM_1       LCD_BUF_BIT0
#define LCD_COM_2       LCD_BUF_BIT1
#define LCD_COM_3       LCD_BUF_BIT2
#define LCD_COM_4       LCD_BUF_BIT3
#define LCD_COM_5       LCD_BUF_BIT4
#define LCD_COM_6       LCD_BUF_BIT5
#define LCD_COM_7       LCD_BUF_BIT6
#define LCD_COM_8       LCD_BUF_BIT7

/* 2. LCD_SEG, defined by user. Take the 8*32 LCD screen as an example. */
#define LCD_SEG_NUM     32U
/* Mapping relationship between LCD pin and LCD_BUF[i]*/
#define LCD_PIN_1       LCD_BUF_12  //SEG16
#define LCD_PIN_2	    LCD_BUF_13  //SEG17
#define LCD_PIN_3	    LCD_BUF_14  //SEG18
#define LCD_PIN_4	    LCD_BUF_15  //SEG19
#define LCD_PIN_5	    LCD_BUF_16  //SEG20
#define LCD_PIN_6	    LCD_BUF_17  //SEG21
#define LCD_PIN_7	    LCD_BUF_18  //SEG22
#define LCD_PIN_8	    LCD_BUF_19  //SEG23
#define LCD_PIN_9	    LCD_BUF_20  //SEG24
#define LCD_PIN_10	    LCD_BUF_21  //SEG25
#define LCD_PIN_11	    LCD_BUF_22  //SEG26
#define LCD_PIN_12	    LCD_BUF_23  //SEG27
#define LCD_PIN_13	    LCD_BUF_24  //SEG28
#define LCD_PIN_14	    LCD_BUF_25  //SEG29
#define LCD_PIN_15	    LCD_BUF_26  //SEG30
#define LCD_PIN_16	    LCD_BUF_27  //SEG31
#define LCD_PIN_17	    LCD_BUF_28  //SEG32
#define LCD_PIN_18	    LCD_BUF_29  //SEG33
#define LCD_PIN_19	    LCD_BUF_30  //SEG34
#define LCD_PIN_20	    LCD_BUF_31  //SEG35
#define LCD_PIN_21	    LCD_BUF_0   //SEG4
#define LCD_PIN_22	    LCD_BUF_1   //SEG5
#define LCD_PIN_23	    LCD_BUF_2   //SEG6
#define LCD_PIN_24	    LCD_BUF_3   //SEG7
#define LCD_PIN_25	    LCD_BUF_4   //SEG8
#define LCD_PIN_26	    LCD_BUF_5   //SEG9
#define LCD_PIN_27	    LCD_BUF_6   //SEG10
#define LCD_PIN_28	    LCD_BUF_7   //SEG11
#define LCD_PIN_29	    LCD_BUF_8   //SEG12
#define LCD_PIN_30	    LCD_BUF_9   //SEG13
#define LCD_PIN_31	    LCD_BUF_10  //SEG14
#define LCD_PIN_32	    LCD_BUF_11  //SEG15

/* 3. Display characters map, user should define based on LCD drawing */
/* Digital tube map */
#define DIGTUBE_NUMBER      0x8U
#define LCD_DISPLAY_1A	    LCD_MAP(LCD_PIN_28, LCD_COM_1)
#define LCD_DISPLAY_1B	    LCD_MAP(LCD_PIN_28, LCD_COM_2)
#define LCD_DISPLAY_1C	    LCD_MAP(LCD_PIN_28, LCD_COM_3)
#define LCD_DISPLAY_1D	    LCD_MAP(LCD_PIN_29, LCD_COM_4)
#define LCD_DISPLAY_1E	    LCD_MAP(LCD_PIN_29, LCD_COM_3)
#define LCD_DISPLAY_1F	    LCD_MAP(LCD_PIN_29, LCD_COM_1)
#define LCD_DISPLAY_1G	    LCD_MAP(LCD_PIN_29, LCD_COM_2)
#define LCD_DISPLAY_2A	    LCD_MAP(LCD_PIN_31, LCD_COM_5)
#define LCD_DISPLAY_2B	    LCD_MAP(LCD_PIN_31, LCD_COM_6)
#define LCD_DISPLAY_2C	    LCD_MAP(LCD_PIN_31, LCD_COM_7)
#define LCD_DISPLAY_2D	    LCD_MAP(LCD_PIN_32, LCD_COM_8)
#define LCD_DISPLAY_2E	    LCD_MAP(LCD_PIN_32, LCD_COM_7)
#define LCD_DISPLAY_2F	    LCD_MAP(LCD_PIN_32, LCD_COM_5)
#define LCD_DISPLAY_2G	    LCD_MAP(LCD_PIN_32, LCD_COM_6)
#define LCD_DISPLAY_3A	    LCD_MAP(LCD_PIN_29, LCD_COM_5)
#define LCD_DISPLAY_3B	    LCD_MAP(LCD_PIN_29, LCD_COM_6)
#define LCD_DISPLAY_3C	    LCD_MAP(LCD_PIN_29, LCD_COM_7)
#define LCD_DISPLAY_3D	    LCD_MAP(LCD_PIN_30, LCD_COM_8)
#define LCD_DISPLAY_3E	    LCD_MAP(LCD_PIN_30, LCD_COM_7)
#define LCD_DISPLAY_3F	    LCD_MAP(LCD_PIN_30, LCD_COM_5)
#define LCD_DISPLAY_3G	    LCD_MAP(LCD_PIN_30, LCD_COM_6)
#define LCD_DISPLAY_4A	    LCD_MAP(LCD_PIN_27, LCD_COM_5)
#define LCD_DISPLAY_4B	    LCD_MAP(LCD_PIN_27, LCD_COM_6)
#define LCD_DISPLAY_4C	    LCD_MAP(LCD_PIN_27, LCD_COM_7)
#define LCD_DISPLAY_4D	    LCD_MAP(LCD_PIN_28, LCD_COM_8)
#define LCD_DISPLAY_4E	    LCD_MAP(LCD_PIN_28, LCD_COM_7)
#define LCD_DISPLAY_4F	    LCD_MAP(LCD_PIN_28, LCD_COM_5)
#define LCD_DISPLAY_4G	    LCD_MAP(LCD_PIN_28, LCD_COM_6)
#define LCD_DISPLAY_5A	    LCD_MAP(LCD_PIN_25, LCD_COM_5)
#define LCD_DISPLAY_5B	    LCD_MAP(LCD_PIN_25, LCD_COM_6)
#define LCD_DISPLAY_5C	    LCD_MAP(LCD_PIN_25, LCD_COM_7)
#define LCD_DISPLAY_5D	    LCD_MAP(LCD_PIN_26, LCD_COM_8)
#define LCD_DISPLAY_5E	    LCD_MAP(LCD_PIN_26, LCD_COM_7)
#define LCD_DISPLAY_5F	    LCD_MAP(LCD_PIN_26, LCD_COM_5)
#define LCD_DISPLAY_5G	    LCD_MAP(LCD_PIN_26, LCD_COM_6)
#define LCD_DISPLAY_6A	    LCD_MAP(LCD_PIN_23, LCD_COM_5)
#define LCD_DISPLAY_6B	    LCD_MAP(LCD_PIN_23, LCD_COM_6)
#define LCD_DISPLAY_6C	    LCD_MAP(LCD_PIN_23, LCD_COM_7)
#define LCD_DISPLAY_6D	    LCD_MAP(LCD_PIN_24, LCD_COM_8)
#define LCD_DISPLAY_6E	    LCD_MAP(LCD_PIN_24, LCD_COM_7)
#define LCD_DISPLAY_6F	    LCD_MAP(LCD_PIN_24, LCD_COM_5)
#define LCD_DISPLAY_6G	    LCD_MAP(LCD_PIN_24, LCD_COM_6)
#define LCD_DISPLAY_7A	    LCD_MAP(LCD_PIN_21, LCD_COM_5)
#define LCD_DISPLAY_7B	    LCD_MAP(LCD_PIN_21, LCD_COM_6)
#define LCD_DISPLAY_7C	    LCD_MAP(LCD_PIN_21, LCD_COM_7)
#define LCD_DISPLAY_7D	    LCD_MAP(LCD_PIN_22, LCD_COM_8)
#define LCD_DISPLAY_7E	    LCD_MAP(LCD_PIN_22, LCD_COM_7)
#define LCD_DISPLAY_7F	    LCD_MAP(LCD_PIN_22, LCD_COM_5)
#define LCD_DISPLAY_7G	    LCD_MAP(LCD_PIN_22, LCD_COM_6)
#define LCD_DISPLAY_8A	    LCD_MAP(LCD_PIN_19, LCD_COM_5)
#define LCD_DISPLAY_8B	    LCD_MAP(LCD_PIN_19, LCD_COM_6)
#define LCD_DISPLAY_8C	    LCD_MAP(LCD_PIN_19, LCD_COM_7)
#define LCD_DISPLAY_8D	    LCD_MAP(LCD_PIN_20, LCD_COM_8)
#define LCD_DISPLAY_8E	    LCD_MAP(LCD_PIN_20, LCD_COM_7)
#define LCD_DISPLAY_8F	    LCD_MAP(LCD_PIN_20, LCD_COM_5)
#define LCD_DISPLAY_8G	    LCD_MAP(LCD_PIN_20, LCD_COM_6)

/* special characters map, user should define based on LCD drawing */
#define LCD_DISPLAY_LOGO_HOME	LCD_MAP(LCD_PIN_16, LCD_COM_2)
#define LCD_DISPLAY_Ua	        LCD_MAP(LCD_PIN_15, LCD_COM_1)
#define LCD_DISPLAY_Ub	        LCD_MAP(LCD_PIN_15, LCD_COM_2)
#define LCD_DISPLAY_Uc	        LCD_MAP(LCD_PIN_15, LCD_COM_3)

/**************************** function ****************************/
void Lcd_Demo_Fun1(void);

#endif
