/**
 ******************************************************************************
 * @file   uart.h
 * @author Renergy Software Team
 * @brief  This file contains all the function prototypes for the usart.c file
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __UART_H
#define __UART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include "main.h"

/* Exported types -----------------------------------------------------------*/
/* Exported constants -------------------------------------------------------*/
extern UART_HandleTypeDef huart0;

/* Exported macro -----------------------------------------------------------*/
/* Exported functions prototypes --------------------------------------------*/
void MX_UART0_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_H */
