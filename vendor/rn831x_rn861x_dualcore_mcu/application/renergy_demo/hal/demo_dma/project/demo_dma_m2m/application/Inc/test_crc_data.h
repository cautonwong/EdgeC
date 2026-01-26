/**
 ******************************************************************************
 * @file   test_crc_data.h
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

#ifndef __TEST_CRC_DATA_H
#define __TEST_CRC_DATA_H

#include <string.h>

typedef struct
{
    uint32_t *In;        /*!< Input data */
    uint32_t  InLen;     /*!< Input data length in byte */
    uint32_t  Out;       /*!< Verify crc result */
    uint32_t  DataWidth; /*!< Data width */
    uint32_t  PolySize;  /*!< PolySize */
    uint32_t  Poly;      /*!< Polynomial */
    uint32_t  Init;      /*!< Init value */
    uint32_t  RefIn;     /*!< Reverse input */
    uint32_t  RefOut;    /*!< Reverse output */
    uint32_t  XorOut;    /*!< Xor output */
} CRC_TestTypeDef;

const uint32_t crc_data[] =
{
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
    0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A, 0x4D3C2B1A,
};

/******************************** CRC8 ***************************************/\
// CRC-8/AUTOSAR
const CRC_TestTypeDef test_crc8_00 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x35,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x2F,                       /*!< Polynomial */
    0xFF,                       /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0xFF,                       /*!< Xor output */
};

// CRC-8/BLUETOOTH
const CRC_TestTypeDef test_crc8_01 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xBB,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0xA7,                       /*!< Polynomial */
    0x00,                       /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/CDMA2000
const CRC_TestTypeDef test_crc8_02 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x96,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x9B,                       /*!< Polynomial */
    0xFF,                       /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/DARC
const CRC_TestTypeDef test_crc8_03 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xD2,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x39,                       /*!< Polynomial */
    0x00,                       /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/DVB-S2
const CRC_TestTypeDef test_crc8_04 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x17,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0xD5,                       /*!< Polynomial */
    0x00,                       /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/GSM-A
const CRC_TestTypeDef test_crc8_05 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x0A,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x1D,                       /*!< Polynomial */
    0x00,                       /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/GSM-B
const CRC_TestTypeDef test_crc8_06 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x8B,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x49,                       /*!< Polynomial */
    0x00,                       /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0xFF,                       /*!< Xor output */
};

// CRC-8/HITAG
const CRC_TestTypeDef test_crc8_07 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xAC,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x1D,                       /*!< Polynomial */
    0xFF,                       /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/I-432-1
const CRC_TestTypeDef test_crc8_08 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x36,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x07,                       /*!< Polynomial */
    0x00,                       /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x55,                       /*!< Xor output */
};

// CRC-8/I-CODE
const CRC_TestTypeDef test_crc8_09 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x8B,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x1D,                       /*!< Polynomial */
    0xFD,                       /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/LTE
const CRC_TestTypeDef test_crc8_10 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x42,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x9B,                       /*!< Polynomial */
    0x00,                       /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/MAXIM-DOW
const CRC_TestTypeDef test_crc8_11 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x99,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x31,                       /*!< Polynomial */
    0x00,                       /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/MIFARE-MAD
const CRC_TestTypeDef test_crc8_12 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x5F,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x1D,                       /*!< Polynomial */
    0xC7,                       /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/NRSC-5
const CRC_TestTypeDef test_crc8_13 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x22,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x31,                       /*!< Polynomial */
    0xFF,                       /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/OPENSAFETY
const CRC_TestTypeDef test_crc8_14 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x14,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x2F,                       /*!< Polynomial */
    0x00,                       /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/ROHC
const CRC_TestTypeDef test_crc8_15 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x0A,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x07,                       /*!< Polynomial */
    0xFF,                       /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/SAE-J1850
const CRC_TestTypeDef test_crc8_16 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x53,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x1D,                       /*!< Polynomial */
    0xFF,                       /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0xFF,                       /*!< Xor output */
};

// CRC-8/SMBUS
const CRC_TestTypeDef test_crc8_17 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x63,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x07,                       /*!< Polynomial */
    0x00,                       /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/TECH-3250
const CRC_TestTypeDef test_crc8_18 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xAB,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x1D,                       /*!< Polynomial */
    0xFF,                       /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

// CRC-8/WCDMA
const CRC_TestTypeDef test_crc8_19 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x44,                       /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT8,          /*!< PolySize */
    0x9B,                       /*!< Polynomial */
    0x00,                       /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x00,                       /*!< Xor output */
};

/******************************** CRC16 **************************************/
// CRC-16/ARC
const CRC_TestTypeDef test_crc16_00 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x094D,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x8005,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/CDMA2000
const CRC_TestTypeDef test_crc16_01 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x2A00,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0xC867,                     /*!< Polynomial */
    0xFFFF,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/CMS
const CRC_TestTypeDef test_crc16_02 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x66CC,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x8005,                     /*!< Polynomial */
    0xFFFF,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/DDS-110
const CRC_TestTypeDef test_crc16_03 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x9518,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x8005,                     /*!< Polynomial */
    0x800D,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/DECT-R
const CRC_TestTypeDef test_crc16_04 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xA051,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x0589,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0001,                     /*!< Xor output */
};

// CRC-16/DECT-X
const CRC_TestTypeDef test_crc16_05 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xA050,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x0589,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/DNP
const CRC_TestTypeDef test_crc16_06 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x475F,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x3D65,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0xFFFF,                     /*!< Xor output */
};

// CRC-16/EN-13757
const CRC_TestTypeDef test_crc16_07 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xB7C3,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x3D65,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0xFFFF,                     /*!< Xor output */
};

// CRC-16/GENIBUS
const CRC_TestTypeDef test_crc16_08 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x7F64,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x1021,                     /*!< Polynomial */
    0xFFFF,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0xFFFF,                     /*!< Xor output */
};

// CRC-16/GSM
const CRC_TestTypeDef test_crc16_09 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xC80B,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x1021,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0xFFFF,                     /*!< Xor output */
};

// CRC-16/IBM-3740
const CRC_TestTypeDef test_crc16_10 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x809B,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x1021,                     /*!< Polynomial */
    0xFFFF,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/IBM-SDLC
const CRC_TestTypeDef test_crc16_11 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x8C67,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x1021,                     /*!< Polynomial */
    0xFFFF,                     /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0xFFFF,                     /*!< Xor output */
};

// CRC-16/ISO-IEC-14443-3-A
const CRC_TestTypeDef test_crc16_12 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xF20D,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x1021,                     /*!< Polynomial */
    0xC6C6,                     /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/KERMIT
const CRC_TestTypeDef test_crc16_13 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x8575,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x1021,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/LJ1200
const CRC_TestTypeDef test_crc16_14 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x261B,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x6F63,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/M17
const CRC_TestTypeDef test_crc16_15 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x8671,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x5935,                     /*!< Polynomial */
    0xFFFF,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/MAXIM-DOW
const CRC_TestTypeDef test_crc16_16 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xF6B2,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x8005,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0xFFFF,                     /*!< Xor output */
};

// CRC-16/MCRF4XX
const CRC_TestTypeDef test_crc16_17 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x7398,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x1021,                     /*!< Polynomial */
    0xFFFF,                     /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/MODBUS
const CRC_TestTypeDef test_crc16_18 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xDDF3,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x8005,                     /*!< Polynomial */
    0xFFFF,                     /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/NRSC-5
const CRC_TestTypeDef test_crc16_19 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x1660,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x080B,                     /*!< Polynomial */
    0xFFFF,                     /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/OPENSAFETY-A
const CRC_TestTypeDef test_crc16_20 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xF484,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x5935,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/OPENSAFETY-B
const CRC_TestTypeDef test_crc16_21 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xEEB7,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x755B,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/PROFIBUS
const CRC_TestTypeDef test_crc16_22 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x7DCC,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x1DCF,                     /*!< Polynomial */
    0xFFFF,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0xFFFF,                     /*!< Xor output */
};

// CRC-16/RIELLO
const CRC_TestTypeDef test_crc16_23 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xD71B,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x1021,                     /*!< Polynomial */
    0xB2AA,                     /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/SPI-FUJITSU
const CRC_TestTypeDef test_crc16_24 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x2D27,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x1021,                     /*!< Polynomial */
    0x1D0F,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/T10-DIF
const CRC_TestTypeDef test_crc16_25 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xC25E,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x8BB7,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/TELEDISK
const CRC_TestTypeDef test_crc16_26 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x30E2,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0xA097,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/TMS37157
const CRC_TestTypeDef test_crc16_27 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x8247,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x1021,                     /*!< Polynomial */
    0x89EC,                     /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/UMTS
const CRC_TestTypeDef test_crc16_28 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x1BE7,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x8005,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

// CRC-16/USB
const CRC_TestTypeDef test_crc16_29 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x220C,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x8005,                     /*!< Polynomial */
    0xFFFF,                     /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0xFFFF,                     /*!< Xor output */
};

// CRC-16/XMODEM
const CRC_TestTypeDef test_crc16_30 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x37F4,                     /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT16,         /*!< PolySize */
    0x1021,                     /*!< Polynomial */
    0x0000,                     /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x0000,                     /*!< Xor output */
};

/******************************** CRC32 **************************************/
// CRC-32/AIXM
const CRC_TestTypeDef test_crc32_00 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x3A76EAE1,                 /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT32,         /*!< PolySize */
    0x814141AB,                 /*!< Polynomial */
    0x00000000,                 /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x00000000,                 /*!< Xor output */
};

// CRC-32/AUTOSAR
const CRC_TestTypeDef test_crc32_01 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x474C6676,                 /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT32,         /*!< PolySize */
    0xF4ACFB13,                 /*!< Polynomial */
    0xFFFFFFFF,                 /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0xFFFFFFFF,                 /*!< Xor output */
};

// CRC-32/BASE91-D
const CRC_TestTypeDef test_crc32_02 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xD4794F55,                 /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT32,         /*!< PolySize */
    0xA833982B,                 /*!< Polynomial */
    0xFFFFFFFF,                 /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0xFFFFFFFF,                 /*!< Xor output */
};

// CRC-32/BZIP2
const CRC_TestTypeDef test_crc32_03 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x3586C079,                 /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT32,         /*!< PolySize */
    0x04C11DB7,                 /*!< Polynomial */
    0xFFFFFFFF,                 /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0xFFFFFFFF,                 /*!< Xor output */
};

// CRC-32/CD-ROM-EDC
const CRC_TestTypeDef test_crc32_04 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x3C0C995C,                 /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT32,         /*!< PolySize */
    0x8001801B,                 /*!< Polynomial */
    0x00000000,                 /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x00000000,                 /*!< Xor output */
};

// CRC-32/CKSUM
const CRC_TestTypeDef test_crc32_05 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xBE8C9271,                 /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT32,         /*!< PolySize */
    0x04C11DB7,                 /*!< Polynomial */
    0x00000000,                 /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0xFFFFFFFF,                 /*!< Xor output */
};

// CRC-32/ISCSI
const CRC_TestTypeDef test_crc32_06 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x9B879658,                 /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT32,         /*!< PolySize */
    0x1EDC6F41,                 /*!< Polynomial */
    0xFFFFFFFF,                 /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0xFFFFFFFF,                 /*!< Xor output */
};

// CRC-32/ISO-HDLC
const CRC_TestTypeDef test_crc32_07 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x15501A53,                 /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT32,         /*!< PolySize */
    0x04C11DB7,                 /*!< Polynomial */
    0xFFFFFFFF,                 /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0xFFFFFFFF,                 /*!< Xor output */
};

// CRC-32/JAMCRC
const CRC_TestTypeDef test_crc32_08 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0xEAAFE5AC,                 /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT32,         /*!< PolySize */
    0x04C11DB7,                 /*!< Polynomial */
    0xFFFFFFFF,                 /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x00000000,                 /*!< Xor output */
};

// CRC-32/MEF
const CRC_TestTypeDef test_crc32_09 =
{
    (uint32_t *)&crc_data,      /*!< Input data */
    sizeof(crc_data),           /*!< Input data length in byte */
    0x5672118E,                 /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT32,         /*!< PolySize */
    0x741B8CD7,                 /*!< Polynomial */
    0xFFFFFFFF,                 /*!< Init value */
    CRC_REVERSE_IN_BYTE,        /*!< Reverse input */
    CRC_OUTPUT_REVERSE_ENABLE,  /*!< Reverse output */
    0x00000000,                 /*!< Xor output */
};

// CRC-32/MPEG-2
const CRC_TestTypeDef test_crc32_10 =
{
    (uint32_t *)&crc_data, /*!< Input data */
    sizeof(crc_data),      /*!< Input data length in byte */
    0xCA793F86,                 /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT32,         /*!< PolySize */
    0x04C11DB7,                 /*!< Polynomial */
    0xFFFFFFFF,                 /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x00000000,                 /*!< Xor output */
};

// CRC-32/XFER
const CRC_TestTypeDef test_crc32_11 =
{
    (uint32_t *)&crc_data, /*!< Input data */
    sizeof(crc_data),      /*!< Input data length in byte */
    0x0BDFCEE1,                 /*!< Verify crc result */
    CRC_OPT_DATA_WIDTH_IN_WORD, /*!< Data width */
    CRC_POLYSIZE_BIT32,         /*!< PolySize */
    0x000000AF,                 /*!< Polynomial */
    0x00000000,                 /*!< Init value */
    CRC_NOREVERSE,              /*!< Reverse input */
    CRC_OUTPUT_REVERSE_DISABLE, /*!< Reverse output */
    0x00000000,                 /*!< Xor output */
};

const CRC_TestTypeDef *crc_testlist[] =
{
    (CRC_TestTypeDef *)&test_crc8_00,
    (CRC_TestTypeDef *)&test_crc8_01,
    (CRC_TestTypeDef *)&test_crc8_02,
    (CRC_TestTypeDef *)&test_crc8_03,
    (CRC_TestTypeDef *)&test_crc8_04,
    (CRC_TestTypeDef *)&test_crc8_05,
    (CRC_TestTypeDef *)&test_crc8_06,
    (CRC_TestTypeDef *)&test_crc8_07,
    (CRC_TestTypeDef *)&test_crc8_08,
    (CRC_TestTypeDef *)&test_crc8_09,
    (CRC_TestTypeDef *)&test_crc8_10,
    (CRC_TestTypeDef *)&test_crc8_11,
    (CRC_TestTypeDef *)&test_crc8_12,
    (CRC_TestTypeDef *)&test_crc8_13,
    (CRC_TestTypeDef *)&test_crc8_14,
    (CRC_TestTypeDef *)&test_crc8_15,
    (CRC_TestTypeDef *)&test_crc8_16,
    (CRC_TestTypeDef *)&test_crc8_17,
    (CRC_TestTypeDef *)&test_crc8_18,
    (CRC_TestTypeDef *)&test_crc8_19,
    (CRC_TestTypeDef *)&test_crc16_00,
    (CRC_TestTypeDef *)&test_crc16_01,
    (CRC_TestTypeDef *)&test_crc16_02,
    (CRC_TestTypeDef *)&test_crc16_03,
    (CRC_TestTypeDef *)&test_crc16_04,
    (CRC_TestTypeDef *)&test_crc16_05,
    (CRC_TestTypeDef *)&test_crc16_06,
    (CRC_TestTypeDef *)&test_crc16_07,
    (CRC_TestTypeDef *)&test_crc16_08,
    (CRC_TestTypeDef *)&test_crc16_09,
    (CRC_TestTypeDef *)&test_crc16_10,
    (CRC_TestTypeDef *)&test_crc16_11,
    (CRC_TestTypeDef *)&test_crc16_12,
    (CRC_TestTypeDef *)&test_crc16_13,
    (CRC_TestTypeDef *)&test_crc16_14,
    (CRC_TestTypeDef *)&test_crc16_15,
    (CRC_TestTypeDef *)&test_crc16_16,
    (CRC_TestTypeDef *)&test_crc16_17,
    (CRC_TestTypeDef *)&test_crc16_18,
    (CRC_TestTypeDef *)&test_crc16_19,
    (CRC_TestTypeDef *)&test_crc16_20,
    (CRC_TestTypeDef *)&test_crc16_21,
    (CRC_TestTypeDef *)&test_crc16_22,
    (CRC_TestTypeDef *)&test_crc16_23,
    (CRC_TestTypeDef *)&test_crc16_24,
    (CRC_TestTypeDef *)&test_crc16_25,
    (CRC_TestTypeDef *)&test_crc16_26,
    (CRC_TestTypeDef *)&test_crc16_27,
    (CRC_TestTypeDef *)&test_crc16_28,
    (CRC_TestTypeDef *)&test_crc16_29,
    (CRC_TestTypeDef *)&test_crc16_30,
    (CRC_TestTypeDef *)&test_crc32_00,
    (CRC_TestTypeDef *)&test_crc32_01,
    (CRC_TestTypeDef *)&test_crc32_02,
    (CRC_TestTypeDef *)&test_crc32_03,
    (CRC_TestTypeDef *)&test_crc32_04,
    (CRC_TestTypeDef *)&test_crc32_05,
    (CRC_TestTypeDef *)&test_crc32_06,
    (CRC_TestTypeDef *)&test_crc32_07,
    (CRC_TestTypeDef *)&test_crc32_08,
    (CRC_TestTypeDef *)&test_crc32_09,
    (CRC_TestTypeDef *)&test_crc32_10,
    (CRC_TestTypeDef *)&test_crc32_11,
};

#endif /* __TEST_CRC_DATA_H */
