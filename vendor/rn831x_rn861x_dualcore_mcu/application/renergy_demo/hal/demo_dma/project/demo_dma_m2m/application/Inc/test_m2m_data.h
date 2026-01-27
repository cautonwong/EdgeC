/**
 ******************************************************************************
 * @file   test_m2m_data.h
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

#ifndef __TEST_M2M_DATA_H
#define __TEST_M2M_DATA_H

#include <string.h>

const M2M_InitTypeDef test0_cfg = {0, 0, 0xF, 0xF, M2M_DATA_REV_NONE,     0,          0, 0};
const M2M_InitTypeDef test1_cfg = {0, 0, 0xF, 0xF, M2M_DATA_REV_BYTEONLY, 0,          0, 0};
const M2M_InitTypeDef test2_cfg = {0, 0, 0xF, 0xF, M2M_DATA_REV_ADDRONLY, 0,          0, 0};
const M2M_InitTypeDef test3_cfg = {0, 0, 0xF, 0xF, M2M_DATA_REV_ADDR,     0,          0, 0};
const M2M_InitTypeDef test4_cfg = {0, 0, 0xE, 0xE, M2M_DATA_REV_NONE,     1, 0xFFFFFFFF, 0};
const M2M_InitTypeDef test5_cfg = {0, 0, 0xE, 0xE, M2M_DATA_REV_BYTEONLY, 1, 0xFFFFFFFF, 0};

const M2M_InitTypeDef *m2m_testlist[] =
{
    (M2M_InitTypeDef *)&test0_cfg,
    (M2M_InitTypeDef *)&test1_cfg,
    (M2M_InitTypeDef *)&test2_cfg,
    (M2M_InitTypeDef *)&test3_cfg,
    (M2M_InitTypeDef *)&test4_cfg,
    (M2M_InitTypeDef *)&test5_cfg,
};

#endif /* __TEST_M2M_DATA_H */
