/**
 ******************************************************************************
 * @file   test_dma.c
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

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "test_dma.h"
#include "unity.h"

DMA_HandleTypeDef hdma;

#ifdef QSPI_TEST_ENABLE

#define QSPI_TIMEOUT 100

#define QSPI0_CS_ON      (QSPI0->CSMODE = 0x2)
#define QSPI0_CS_OFF     (QSPI0->CSMODE = 0x3)

#define QSPI1_CS_ON      (QSPI1->CSMODE = 0x2)
#define QSPI1_CS_OFF     (QSPI1->CSMODE = 0x3)

#define PAGESIZE       0x100

void test_qspi(void)
{
    hdma.Instance = DMA00; // qspi0

    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SCU_WKEY_UNLOCK();
    __HAL_SCU_QSPI0_SET_SECURE(); // set qspi0 to core0
    __HAL_SCU_QSPI1_SET_NONSECURE(); // set qspi1 to core1
    LL_SYSTEM_CORE0_MOD2_EnableClock(LL_SYSTEM_CORE0_MOD2_PERIPH_QSPI0);
    LL_SYSTEM_CORE1_MOD2_EnableClock(LL_SYSTEM_CORE1_MOD2_PERIPH_QSPI1);
    __HAL_SYSTEM_PSW_LOCK();

    const uint32_t qspi_src_data[] = {0x4D3C2B1A, 0x04030201, 0x0D0C0B0A, 0x40302010};
    uint32_t i;

    GPIO_InitTypeDef GPIO_Init;
    /* PB00(CSN)/01(SCLK)/02(SI)/03(SO)/04(HOLDN)/PA15(WPN) QSPI0 AF1 */
    GPIO_Init.Pin       = (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4);
    GPIO_Init.Mode      = GPIO_MODE_AF;
    GPIO_Init.Alternate = GPIO_AF1_QSPI0;
    HAL_GPIO_Init(GPIOB, &GPIO_Init);
    GPIO_Init.Pin       = GPIO_PIN_15;
    GPIO_Init.Mode      = GPIO_MODE_AF;
    GPIO_Init.Alternate = GPIO_AF1_QSPI0;
    HAL_GPIO_Init(GPIOA, &GPIO_Init);

    /* PG02(CSN)/01(SCLK)/06(SI)/04(SO)/08(HOLDN)/07(WPN) QSPI1 AF2 */
    GPIO_Init.Pin       = (GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8);
    GPIO_Init.Mode      = GPIO_MODE_AF;
    GPIO_Init.Alternate = GPIO_AF2_SPI0_QSPI1;
    HAL_GPIO_Init(GPIOG, &GPIO_Init);

    // LL_QSPI_InitTypeDef qspi0_init;
    // qspi0_init.Mode = LL_QSPI_MODE_MASTER;
    // qspi0_init.DataWidth = LL_QSPI_DATAWIDTH_8BIT;
    // LL_QSPI_Init(QSPI0, &qspi0_init);

    // LL_QSPI_InitTypeDef qspi1_init;
    // qspi1_init.Mode = LL_QSPI_MODE_SLAVE;
    // qspi1_init.DataWidth = LL_QSPI_DATAWIDTH_8BIT;
    // LL_QSPI_Init(QSPI1, &qspi1_init);
    // LL_QSPI_EnableForceOutput(QSPI0);
    // LL_QSPI_XIP_Disable(QSPI0);
    QSPI0->FCTRL     = 0;
    QSPI0->FMT       = 0x20000A;

    while (1) {
        WDT->EN = 0xBB;
        QSPI0->TXDATA = 0x5A5A5A5A;
        while ((QSPI0->STA & QSPI_STA_DONE) != QSPI_STA_DONE);
        printf("QSPI0 send success!\n");
        QSPI0->STA |= QSPI_STA_DONE;
    }

    QSPI1->FCTRL     = 0;
    QSPI1->FORCE     = 0x3;
    QSPI1->CR        = 0x2001;
    QSPI1->SCKSAMPLE = 0;
    QSPI1->TSIZE     = 0xFFFFFFFF;
    QSPI1->RSIZE     = 0xFFFFFFFF;

//    for (i = 0; i < sizeof(qspi_src_data); i++) {
//        LL_QSPI_TransmitData8(QSPI0, *(uint8_t *)((uint32_t)qspi_src_data + i));
//    }

//    hdma.Channel.Type         = HAL_DMA_CHANNEL_QSPI0_TX;
//    hdma.Init.Priority        = DMA_PRIORITY_VERY_HIGH;
//    TEST_ASSERT_EQUAL_UINT_MESSAGE( \
//        HAL_OK, \
//        HAL_DMA_Init((DMA_HandleTypeDef *)&hdma), \
//        "DMA init error!!!");
//
//    TEST_ASSERT_EQUAL_UINT_MESSAGE( \
//        HAL_OK, \
//        HAL_DMA_StartChannel_IT((DMA_HandleTypeDef *)&hdma, (uint32_t)&qspi_src_data[0], 0, sizeof(qspi_src_data)), \
//        "HAL_DMA_StartChannel_IT run error!!!");
//
//    TEST_ASSERT_EQUAL_UINT_MESSAGE( \
//        HAL_OK, \
//        HAL_DMA_PollForTransfer((DMA_HandleTypeDef *)&hdma, HAL_DMA_FULL_TRANSFER, QSPI_TIMEOUT), \
//        "QSPI0-DMA it flag is error!!!");
//
//    __HAL_SYSTEM_PSW_UNLOCK();
//    LL_QSPI_DeInit(QSPI0);
//    __HAL_SYSTEM_PSW_LOCK();
}
#endif

#ifdef USART_TEST_ENABLE

#define USART_TIMEOUT 100

void test_usart(void)
{
    hdma.Instance = DMA00; // usart0

    __HAL_SCU_WKEY_UNLOCK();
    __HAL_SCU_USART0_SET_SECURE();

    GPIO_InitTypeDef GPIO_Init;
    /* PA05(TX)/06(RX)/07(CK)/08(RTS) AF11 */
    /* use usart as uart */
    GPIO_Init.Pin       = (GPIO_PIN_5 | GPIO_PIN_6);
    GPIO_Init.Mode      = GPIO_MODE_AF;
    GPIO_Init.Alternate = GPIO_AF11_USART_TCO;
    HAL_GPIO_Init(GPIOA, &GPIO_Init);

    const uint8_t usart_src_data[] = "hello world, and i am lilei\n";
    uint32_t i;

    LL_USART_DisableCTSHWFlowCtrl(USART0);
    LL_USART_DisableRTSHWFlowCtrl(USART0);

    LL_USART_EnableIT_TX(USART0);
    LL_USART_EnableIT_RX(USART0);
    WRITE_REG(USART0->STA, 0x333F0);

    LL_USART_InitTypeDef usart_init;
    usart_init.BaudRate             = 115200;
    usart_init.DataWidth            = LL_USART_DATAWIDTH_8B;
    usart_init.TransferDirection    = LL_USART_DIRECTION_TX_RX;
    LL_USART_Init(USART0, (const LL_USART_InitTypeDef *)&usart_init);

    // USART0 normal mode
    for (i = 0; i < sizeof(usart_src_data); i++) {
        LL_USART_TransmitData(USART0, (uint8_t)usart_src_data[i]);
        __NOP();
    }

    LL_USART_SetDMAReqDirection(USART0, LL_USART_DMA_DIRECTION_TX_RX);

    // USART0 dma mode
    hdma.Channel.Type         = HAL_DMA_CHANNEL_USART0_TX;
    hdma.Init.Priority        = DMA_PRIORITY_VERY_HIGH;
    TEST_ASSERT_EQUAL_UINT_MESSAGE( \
        HAL_OK, \
        HAL_DMA_Init((DMA_HandleTypeDef *)&hdma), \
        "DMA init error!!!");

    TEST_ASSERT_EQUAL_UINT_MESSAGE( \
        HAL_OK, \
        HAL_DMA_StartChannel_IT((DMA_HandleTypeDef *)&hdma, (uint32_t)&usart_src_data[0], 0, sizeof(usart_src_data)), \
        "HAL_DMA_StartChannel_IT run error!!!");

    HAL_DMA_Abort((DMA_HandleTypeDef *)&hdma);

    TEST_ASSERT_EQUAL_UINT_MESSAGE( \
        HAL_OK, \
        HAL_DMA_StartChannel_IT((DMA_HandleTypeDef *)&hdma, (uint32_t)&usart_src_data[0], 0, sizeof(usart_src_data)), \
        "HAL_DMA_StartChannel_IT 2 run error!!!");

    TEST_ASSERT_EQUAL_UINT_MESSAGE( \
        HAL_OK, \
        HAL_DMA_PollForTransfer((DMA_HandleTypeDef *)&hdma, HAL_DMA_FULL_TRANSFER, USART_TIMEOUT), \
        "USART-DMA it flag is error!!!");

    __HAL_SYSTEM_PSW_UNLOCK();
    LL_USART_DeInit(USART0);
    __HAL_SYSTEM_PSW_LOCK();
}
#endif

#ifdef SEA_TEST_ENABLE
void test_sea(void)
{
    const uint8_t sha256_msg[] = {0x2C, 0xAD, 0x21, 0xBD, 0xF7, 0x58, 0x64, 0x91, 0xF4, 0x49, 0x89, 0xAC, 0x62, 0xDD, 0x06, 0x59, 0x6A, 0x72, 0x5D, 0xF0, 0x78, 0x76,
                                0x12, 0x92, 0x1B, 0xE7, 0x68, 0x6B, 0xD2, 0xBE, 0xFE, 0xD8, 0x98, 0x52, 0x6F, 0x1A, 0x59, 0x32, 0x27, 0x21, 0x41, 0xEA, 0xA0, 0x53,
                                0xB5, 0xCD, 0xDB, 0xED, 0xC8, 0x13, 0x18, 0xBF, 0x46, 0x0A, 0x2E, 0x9B, 0xC8, 0x7B, 0xEF, 0x1E, 0x7A, 0xA6, 0x7C, 0x95, 0x9D, 0x3E,
                                0x7A, 0x3E, 0xCB, 0xCF, 0x6D, 0x83, 0x31, 0xA3, 0xFA, 0xA7, 0x12, 0xDA, 0x0D, 0x0B, 0x05, 0xE2, 0x11, 0x4E, 0x53, 0x9B, 0x17, 0x12,
                                0x90, 0x3B, 0xE9, 0xC6, 0xB5, 0x58, 0xC5, 0x3C, 0x91, 0xB2, 0x2E, 0xC3, 0xD9, 0xE1, 0xD0, 0xDA, 0x62, 0x6B, 0x5B, 0x5D, 0x1C, 0xBE,
                                0xD6, 0xB1, 0x42, 0xA5, 0x77, 0x40, 0xE3, 0x0E, 0x47, 0xCB, 0x02, 0x53, 0x1F, 0x9C, 0xA9, 0xCF, 0xEF, 0x8C}; /* 1024 bits */
    const uint8_t sha256_out[] = {0x7D, 0x8C, 0xCC, 0xAE, 0x0A, 0x2D, 0x52, 0xFF, 0x7E, 0xC3, 0xE5, 0x75, 0x6E, 0x32, 0x7D, 0x00, 0x76, 0x29, 0x81, 0x27, 0xB6, 0x1B, 0x1F, 0xBC, 0x10, 0x1D, 0x40, 0x11, 0xC6, 0x79, 0xB6, 0x7F};
    const uint8_t sha384_msg[] = {
        0xF6, 0xCB, 0xB6, 0x59, 0x6D, 0xDA, 0xEF, 0x0F, 0x72, 0x28, 0x21, 0x65, 0x33, 0x01, 0x99, 0xD8, 0xB7, 0xDF, 0x90, 0xE2, 0x38, 0x3B, 0xD2, 0xDC, 0xF3, 0x7E, 0xF5, 0x0A, 0x70,
        0xCD, 0xFB, 0x6A, 0x50, 0xA9, 0x37, 0xE0, 0x55, 0xF4, 0xF4, 0xBF, 0x70, 0xFD, 0xD4, 0x00, 0xB4, 0x57, 0x56, 0x27, 0x85, 0x02, 0xE4, 0x63, 0x57, 0xF9, 0xF7, 0x2D, 0xAC, 0x0C,
        0x4E, 0x61, 0x99, 0x5F, 0x0D, 0x46, 0x29, 0xD5, 0xBD, 0x8A, 0x28, 0x1F, 0xDA, 0x9C, 0x42, 0xE8, 0xDB, 0x30, 0x69, 0x7E, 0xE9, 0x41, 0xE0, 0x8F, 0x6A, 0x7E, 0x13, 0x24, 0x40,
        0x0D, 0x33, 0x5F, 0x00, 0x2F, 0x90, 0x37, 0xA7, 0x4B, 0x2D, 0x3D, 0x7F, 0x9D, 0x98, 0x36, 0xB7, 0xF0, 0x80, 0xF7, 0x88, 0x97, 0x06, 0x2B, 0x8D, 0xAC, 0x3C, 0xDE, 0x99, 0x41,
        0xAB, 0x3B, 0x83, 0xA9, 0x72, 0x55, 0xC1, 0xDF, 0x0F, 0x5B, 0x94, 0xCA, 0x7F, 0xE1, 0x5B, 0xF7, 0x2B, 0x25, 0x20, 0x79, 0xD1, 0x16, 0xBA, 0x97, 0xCE, 0xB2, 0x62, 0xF4, 0x3F,
        0x0F, 0x8B, 0x24, 0x02, 0x27, 0x2D, 0x88, 0x34, 0xAB, 0xC2, 0xB8, 0xB0, 0x88, 0x06, 0x3A, 0x8F, 0x56, 0x4A, 0x69, 0x1F, 0xCD, 0xBC, 0xC7, 0x03, 0x9B, 0x68, 0xD8, 0xC4, 0xB8,
        0x54, 0x35, 0x0E, 0x71, 0x67, 0x16, 0x95, 0x23, 0x20, 0xCA, 0x4B, 0x7B, 0xC4, 0xE7, 0xEC, 0x24, 0x14, 0x39, 0xEF, 0x7E, 0x4F, 0x22, 0xE8, 0x71, 0x66, 0xA7, 0x26, 0x21, 0x48,
        0x23, 0xF1, 0x78, 0x04, 0xF8, 0x92, 0xAF, 0xDC, 0xC4, 0xC1, 0xF5, 0xB9, 0xDE, 0x78, 0x32, 0xDA, 0x86, 0x74, 0xE2, 0xE8, 0x4B, 0xBE, 0x59, 0x51, 0xC8, 0xB1, 0xE6, 0xE4, 0xEB,
        0x36, 0x29, 0x0C, 0x4F, 0x02, 0x99, 0xFC, 0x26, 0x01, 0x39, 0xC9, 0xD5, 0x70, 0xD3, 0x92, 0x40, 0x47, 0x01, 0x08, 0xCC, 0x5C, 0x9C, 0xD1, 0x32}; /* 2048 bits */
    const uint8_t sha384_out[] = {0xEE, 0x47, 0x30, 0xD2, 0xFB, 0x69, 0xFA, 0xFA, 0x1F, 0xC0, 0x99, 0x63, 0x3F, 0x49, 0xB9, 0x31, 0x28, 0xBF, 0x1D, 0x1D, 0x08, 0x6E, 0xF7, 0xFC, 0x45, 0xFC, 0x82, 0xA4, 0xF8, 0x09, 0x73, 0xBB, 0x07, 0xA2, 0x3F, 0xF8, 0x12, 0xCB, 0xA4, 0x92, 0x2E, 0xCD, 0xE9, 0xD0, 0x04, 0xEB, 0xA1, 0x52};

    const HASH_InitTypeDef sha_dma_cfg0 = {SEA_SHA_256, HASH_DMA_MODE_EN,     {0U, 0U}};
    const HASH_InitTypeDef sha_dma_cfg1 = {SEA_SHA_384, HASH_DMA_MODE_EN,     {0U, 0U}};
    const HASH_InitTypeDef sha_cfg0     = {SEA_SHA_256, HASH_GENERAL_MODE_EN, {0U, 0U}};
    const HASH_InitTypeDef sha_cfg1     = {SEA_SHA_384, HASH_GENERAL_MODE_EN, {0U, 0U}};

    const HASH_TestTypedef sha_test0 = {(uint8_t *)sha256_msg, (uint8_t *)sha256_out, {0U, 0U}, (uint32_t)sizeof(sha256_msg), (HASH_InitTypeDef *)&sha_dma_cfg0};
    const HASH_TestTypedef sha_test1 = {(uint8_t *)sha256_msg, (uint8_t *)sha256_out, {0U, 0U}, (uint32_t)sizeof(sha256_msg), (HASH_InitTypeDef *)&sha_cfg0};
    const HASH_TestTypedef sha_test2 = {(uint8_t *)sha384_msg, (uint8_t *)sha384_out, {0U, 0U}, (uint32_t)sizeof(sha384_msg), (HASH_InitTypeDef *)&sha_dma_cfg1};
    const HASH_TestTypedef sha_test3 = {(uint8_t *)sha384_msg, (uint8_t *)sha384_out, {0U, 0U}, (uint32_t)sizeof(sha384_msg), (HASH_InitTypeDef *)&sha_cfg1};
    const HASH_TestTypedef *testlist[] = {
        (HASH_TestTypedef *)&sha_test0,
        (HASH_TestTypedef *)&sha_test1,
        (HASH_TestTypedef *)&sha_test2,
        (HASH_TestTypedef *)&sha_test3,
    };
    const uint8_t size[] = {32, 32, 48, 48};

    HASH_HandleTypeDef pParas;
    uint32_t check_dma_out[16] = {0};
    uint32_t check_out[16] = {0};
    uint32_t tmp_err;

    for (int i = 0; i < (sizeof(testlist) / sizeof(testlist[0])); i += 2) {
        /* HASH DMA */
        tmp_err = HAL_SEA_HASH_Init(&pParas, (const HASH_InitTypeDef *)testlist[i]->cfg);
        TEST_ASSERT_EQUAL_UINT_MESSAGE(0, tmp_err, "Hash-DMA initial error!!!");
        tmp_err = HAL_SEA_HASH(&pParas, (const uint8_t *)testlist[i]->msgin, testlist[i]->msglen, CypherDatasIsTheLast, (uint8_t *)check_dma_out);
        TEST_ASSERT_EQUAL_UINT_MESSAGE(0, tmp_err, "Hash-DMA calculate error!!!");
        (void)HAL_SEA_HASH_DeInit();
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, memcmp((const uint8_t *)testlist[i]->result, (const uint8_t *)check_dma_out, size[i]), "Hash-DMA result is wrong!!!");
        WDT_FEED;

        /* HASH GENERAL */
        tmp_err = HAL_SEA_HASH_Init(&pParas, (const HASH_InitTypeDef *)testlist[i + 1]->cfg);
        TEST_ASSERT_EQUAL_UINT_MESSAGE(0, tmp_err, "Hash initial error!!!");
        tmp_err = HAL_SEA_HASH(&pParas, (const uint8_t *)testlist[i + 1]->msgin, testlist[i + 1]->msglen, CypherDatasIsTheLast, (uint8_t *)check_out);
        TEST_ASSERT_EQUAL_UINT_MESSAGE(0, tmp_err, "Hash calculate error!!!");
        (void)HAL_SEA_HASH_DeInit();
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, memcmp((const uint8_t *)testlist[i + 1]->result, (const uint8_t *)check_out, size[i + 1]), "Hash result is wrong!!!");
        WDT_FEED;
    }
}
#endif

#ifdef CRC_TEST_ENABLE
#include "test_crc_data.h"
CRC_HandleTypeDef hcrc;

#define CRC_TIMEOUT 100 // 100ms

static uint32_t crc_testunit(const CRC_TestTypeDef *cfg, uint8_t DmaEnable)
{
    uint8_t status = HAL_ERROR;

    hcrc.Init.InputRevMode      = cfg->RefIn;
    hcrc.Init.OutputRevEnable   = cfg->RefOut;
    hcrc.Init.XorValue          = cfg->XorOut;
    hcrc.Init.DataAlignment     = CRC_OPT_DATA_WIDTH_IN_WORD;
    hcrc.Init.PolynomialSize    = cfg->PolySize;
    hcrc.Init.InitialValue      = cfg->Init;
    hcrc.Init.Polynomial        = cfg->Poly;
    if (DmaEnable) {
        hcrc.Init.DmaMode       = CRC_DMA_MODE_CRC;
        // init crc with dma
        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            HAL_CRC_Init((CRC_HandleTypeDef *)&hcrc), \
            "HAL_CRC_Init error!!!");

        hdma.Channel.Type       = HAL_DMA_CHANNEL_CRC;
        hdma.Init.Priority      = DMA_PRIORITY_VERY_HIGH;
        // init dma
        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            HAL_DMA_Init((DMA_HandleTypeDef *)&hdma), \
            "HAL_DMA_Init error!!!");

        // start channel first
        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            HAL_DMA_StartChannel((DMA_HandleTypeDef *)&hdma, (uint32_t)cfg->In, 0, cfg->InLen), \
            "HAL_DMA_StartChannel error!!!");

        // // abort first tansfer
        // TEST_ASSERT_EQUAL_UINT_MESSAGE( \
        //     HAL_OK, \
        //     HAL_DMA_Abort((DMA_HandleTypeDef *)&hdma), \
        //     "HAL_DMA_Abort error!!!");

        // // each abort need reinit crc
        // TEST_ASSERT_EQUAL_UINT_MESSAGE( \
        //     HAL_OK, \
        //     HAL_CRC_Init((CRC_HandleTypeDef *)&hcrc), \
        //     "HAL_CRC_Init error!!!");

        // // start channel with interrupt enbaled
        // TEST_ASSERT_EQUAL_UINT_MESSAGE( \
        //     HAL_OK, \
        //     HAL_DMA_StartChannel_IT((DMA_HandleTypeDef *)&hdma, (uint32_t)cfg->In, 0, cfg->InLen), \
        //     "HAL_DMA_StartChannel_IT error!!!");

        // // abort second transfer
        // TEST_ASSERT_EQUAL_UINT_MESSAGE( \
        //     HAL_OK, \
        //     HAL_DMA_Abort_IT((DMA_HandleTypeDef *)&hdma), \
        //     "HAL_DMA_Abort_IT error!!!");

        // // each abort need reinit crc
        // TEST_ASSERT_EQUAL_UINT_MESSAGE( \
        //     HAL_OK, \
        //     HAL_CRC_Init((CRC_HandleTypeDef *)&hcrc), \
        //     "HAL_CRC_Init error!!!");

        // // start channel finally
        // TEST_ASSERT_EQUAL_UINT_MESSAGE( \
        //     HAL_OK, \
        //     HAL_DMA_StartChannel((DMA_HandleTypeDef *)&hdma, (uint32_t)cfg->In, 0, cfg->InLen), \
        //     "HAL_DMA_StartChannel error!!!");

        HAL_Delay(10);
        // check dma flag
        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            HAL_DMA_PollForTransfer((DMA_HandleTypeDef *)&hdma, HAL_DMA_FULL_TRANSFER, CRC_TIMEOUT), \
            "HAL_DMA_PollForTransfer error!!!");

        // check crc result
        TEST_ASSERT_EQUAL_HEX_MESSAGE( \
            cfg->Out, \
            LL_CRC_ReadData32(hcrc.Instance), \
            "CRC-DMA calculate error!!!");

        // deinit dma
        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            HAL_DMA_DeInit((DMA_HandleTypeDef *)&hdma), \
            "HAL_DMA_DeInit error!!!");

    } else {
        hcrc.Init.DmaMode      = CRC_DMA_DISABLE;
        // init crc first
        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            HAL_CRC_Init((CRC_HandleTypeDef *)&hcrc), \
            "HAL_CRC_Init error!!!");

        // crc running with previous crc result(default is 0)
        TEST_ASSERT_EQUAL_HEX_MESSAGE( \
            cfg->Out, \
            HAL_CRC_Accumulate((CRC_HandleTypeDef *)&hcrc, (uint8_t *)cfg->In, cfg->InLen), \
            "HAL_CRC_Accumulate result error!!!");

        // crc running with init value
        TEST_ASSERT_EQUAL_HEX_MESSAGE( \
            cfg->Out, \
            HAL_CRC_Calculate((CRC_HandleTypeDef *)&hcrc, (uint8_t *)cfg->In, cfg->InLen), \
            "HAL_CRC_Calculate result error!!!");
    }
    // deinit crc
    TEST_ASSERT_EQUAL_UINT_MESSAGE( \
        HAL_OK, \
        HAL_CRC_DeInit((CRC_HandleTypeDef *)&hcrc), \
        "HAL_CRC_DeInit error!!!");

    status = HAL_OK;
    return status;
}

void test_crc(void)
{
	uint32_t i;
    hcrc.Instance = CRC0;
    hdma.Instance = DMA01;
    for (i = 0U; i < (sizeof(crc_testlist) / sizeof(crc_testlist[0])); i++) {
        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            crc_testunit(crc_testlist[i], DMA_DISABLE), \
            "CRC result error!!!");
        if (i < 20) {
            printf("++ CRC8  gen testcase %ld PASS!\n", i);
        } else if (i < 51) {
            printf("++ CRC16 gen testcase %ld PASS!\n", (i - 20));
        } else {
            printf("++ CRC32 gen testcase %ld PASS!\n", (i - 51));
        }
        WDT_FEED;

        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            crc_testunit(crc_testlist[i], DMA_ENABLE), \
            "CRC-DMA result error!!!");
        if (i < 20) {
            printf("++ CRC8  dma testcase %ld PASS!\n", i);
        } else if (i < 51) {
            printf("++ CRC16 dma testcase %ld PASS!\n", (i - 20));
        } else {
            printf("++ CRC32 dma testcase %ld PASS!\n", (i - 51));
        }
        WDT_FEED;
    }
}
#endif

#ifdef M2M_TEST_ENABLE
#include "test_m2m_data.h"

#define M2M_TIMEOUT 10 // 10ms

void test_m2m(void)
{
    uint32_t i, addr;
    M2M_HandleTypeDef hm2m;
    hm2m.Instance = M2M;

    const uint32_t m2m_src_data[] = {0x4D3C2B1A, 0x04030201, 0x0D0C0B0A, 0x40302010};
    uint32_t m2m_rst_data[20];
    for (i = 0; i < (sizeof(m2m_testlist) / sizeof(m2m_testlist[0])); i++) {
        WDT_FEED;

        for (int j = 0; j < 20; j++) {
            m2m_rst_data[j] = 0U;
        }

        hm2m.Init = *m2m_testlist[i];

        hm2m.Init.SrcAddress     = (uint32_t)&m2m_src_data[0];
        if (hm2m.Init.DstDataReverseMode >= M2M_DATA_REV_ADDRONLY) {
            hm2m.Init.DstAddress = (uint32_t)&m2m_rst_data[4] - 1;
        } else {
            hm2m.Init.DstAddress = (uint32_t)&m2m_rst_data[0];
        }
        hm2m.Init.NbData         = sizeof(m2m_src_data);
        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            HAL_M2M_Init((M2M_HandleTypeDef *)&hm2m), \
            "M2M init error!!!");
        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            HAL_M2M_Start((M2M_HandleTypeDef *)&hm2m, hm2m.Init.SrcAddress, hm2m.Init.DstAddress, hm2m.Init.NbData), \
            "M2M start error!!!");
        TEST_ASSERT_EQUAL_UINT( \
            HAL_M2M_STATE_BUSY, \
            HAL_M2M_GetState((M2M_HandleTypeDef *)&hm2m)); // check m2m state
        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            HAL_M2M_PollForTransfer((M2M_HandleTypeDef *)&hm2m, M2M_TIMEOUT), \
            "M2M flag is error!!!");
        TEST_ASSERT_EQUAL_UINT( \
            HAL_M2M_STATE_READY, \
            HAL_M2M_GetState((M2M_HandleTypeDef *)&hm2m)); // check m2m state

        WDT_FEED;

        if (hm2m.Init.DstDataReverseMode >= M2M_DATA_REV_ADDRONLY) {
            hm2m.Init.DstAddress = (uint32_t)&m2m_rst_data[8] - 1;
        } else {
            hm2m.Init.DstAddress = (uint32_t)&m2m_rst_data[4];
        }
        M2M->IF |= 0x1;
        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            HAL_M2M_Init((M2M_HandleTypeDef *)&hm2m), \
            "M2M it init error!!!");
        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            HAL_M2M_Start_IT((M2M_HandleTypeDef *)&hm2m, hm2m.Init.SrcAddress, hm2m.Init.DstAddress, hm2m.Init.NbData), \
            "M2M it start error!!!");
        TEST_ASSERT_EQUAL_UINT( \
            HAL_M2M_STATE_BUSY, \
            HAL_M2M_GetState((M2M_HandleTypeDef *)&hm2m)); // check m2m state
        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            HAL_M2M_PollForTransfer((M2M_HandleTypeDef *)&hm2m, M2M_TIMEOUT), \
            "M2M it flag is error!!!");
        TEST_ASSERT_EQUAL_UINT( \
            HAL_M2M_STATE_READY, \
            HAL_M2M_GetState((M2M_HandleTypeDef *)&hm2m)); // check m2m state

        /* Check */
        TEST_ASSERT_EQUAL_INT( \
            0, \
            memcmp((uint8_t *)((uint32_t)&m2m_rst_data[0]), (uint8_t *)((uint32_t)&m2m_rst_data[4]), sizeof(m2m_src_data)));

        TEST_ASSERT_EQUAL_UINT_MESSAGE( \
            HAL_OK, \
            HAL_M2M_DeInit((M2M_HandleTypeDef *)&hm2m), \
            "M2M deinit error!!!");
        WDT_FEED;

        printf("++ M2M case %d test PASS\n", i);
    }
    return;
}
#endif
