/**
 ******************************************************************************
 * @file   test_dma.h
 * @author Renergy Software Team
 * @brief
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

#ifndef __TEST_DMA_H
#define __TEST_DMA_H

#ifdef __cplusplus
"C" {
#endif

#include "rn_rv_hal.h"
#include "rn_rv_ll_qspi.h"
#include "rn_rv_ll_usart.h"
#include "rn_rv_ll_wdt.h"

// #define QSPI_TEST_ENABLE
// #define USART_TEST_ENABLE
// #define SEA_TEST_ENABLE
#define CRC_TEST_ENABLE
// #define M2M_TEST_ENABLE

/* input/output items are in bytes */
typedef struct
{
    uint8_t          *msgin;        /*!< message */
    uint8_t          *result;       /*!< hash result buffer */
    uint8_t           reserved[2];  /*!< reserved */
    uint32_t          msglen;       /*!< message length in bytes */
    HASH_InitTypeDef *cfg;          /*!< hash configure */
} HASH_TestTypedef;

#define WDT_FEED    LL_WDT_ReloadCounter(WDT)
#define RDCYCLE(a)  __ASM volatile ("rdcycle %0" : "=r"(a));

#define DMA_ENABLE  1
#define DMA_DISABLE 0

#define IT_ENABLE   1
#define IT_DISABLE  0

extern void test_qspi(void);
extern void test_usart(void);
extern void test_sea(void);
extern void test_crc(void);
extern void test_m2m(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEST_DMA_H */
