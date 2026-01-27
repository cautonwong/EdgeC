/******************************************************************************
 * @file     rn8xxx_ll_lib.c
 * @brief    renergy lib top file
 * @author   Renergy Technology
 * @note
 * Copyright (C) 2025, Renergy Technology Inc. All rights reserved.
 ******************************************************************************/

#include "rn8xxx_v2_lib.h"

#if (defined(RN831x_RN861x_MCU_V2) ||\
     defined(RN831x_RN861x_MCU_V3) ||\
     defined(RN821x_RN721x_SOC_V2) ||\
     defined(RN821x_RN721x_SOC_V3) ||\
     defined(RN202x_RN7326_SOC_V2)) && defined(NVRLIB_EN)
static unsigned int checksum_calc(unsigned int *data, unsigned int length, unsigned int flag)
{
    unsigned int cksum = 0x0U;
    unsigned int i, j;

    for (i = 0U; i < (length / 4U); i++) {
        cksum += data[i];
    }
    if (flag == 1U) {
        cksum = cksum - data[0x78] - data[0x79] - data[0x7a] - data[0x7b];
    }
    return ~cksum;
}
#endif

NvrLibValid_TypeDef LL_LIB_Check(void)
{
#if defined(NVRLIB_EN)
    NvrLibValid_TypeDef res = IS_LIB_INVALID;
    unsigned int cksum;
#if defined(RN831x_RN861x_MCU_V2)
    #define   CKSUM_V126   (0x8f8ddff1U)
    #define   CKSUM_V125   (0x60d2b683U)
    #define   CKSUM_V123   (0xb62fe7bcU)

    cksum = checksum_calc((unsigned int *)0x00080410U, 0x19e8U, 1U); /* >= v1.2.6*/
    if ((cksum == CKSUM_V126) ||
        (cksum == *(volatile unsigned int *)(0x00081df8U))) /* for future */
    {
        res = IS_LIB_VALID;
    } else {
        cksum = checksum_calc((unsigned int *)0x00080600U, 0x17f8U, 0U);
        if ((cksum == CKSUM_V125) || (cksum == CKSUM_V123)) {
            res = IS_LIB_VALID;
        }
    }
#elif defined(RN831x_RN861x_MCU_V3)
    #define   CKSUM_V126   (0x6849ab68U)
    #define   CKSUM_V125   (0x0f221d6fU)
    #define   CKSUM_V123   (0x292ed07fU)
    #define   CKSUM_V120   (0x276a063cU)

    cksum = checksum_calc((unsigned int *)0x00040410U, 0x19e8U, 1U);
    if ((cksum == CKSUM_V126) ||
        (cksum == *(volatile unsigned int *)(0x00041df8U)))
    {
        res = IS_LIB_VALID;
    } else {
        cksum = checksum_calc((unsigned int *)0x00040600U, 0x17f8U, 0U);
        if ((cksum == CKSUM_V125) || (cksum == CKSUM_V123) || (cksum == CKSUM_V120)) {
            res = IS_LIB_VALID;
        }
    }
#elif defined(RN821x_RN721x_SOC_V2)
    #define   CKSUM_V126   (0x911a365cU)
    #define   CKSUM_V125   (0x9f48f7bfU)

    cksum = checksum_calc((unsigned int *)0x00080410U, 0x19e8U, 1U);
    if ((cksum == CKSUM_V126) ||
        (cksum == *(volatile unsigned int *)(0x00081df8U)))
    {
        res = IS_LIB_VALID;
    } else {
        cksum = checksum_calc((unsigned int *)0x00080600U, 0x17f8U, 0U);
        if (cksum == CKSUM_V125) {
            res = IS_LIB_VALID;
        }
    }
#elif defined(RN821x_RN721x_SOC_V3)
    #define   CKSUM_V126   (0xfc07983cU)
    #define   CKSUM_V125   (0x5fd93bb0U)

    cksum = checksum_calc((unsigned int *)0x00040410U, 0x19e8U, 1U);
    if ((cksum == CKSUM_V126) ||
        (cksum == *(volatile unsigned int *)(0x00041df8U)))
    {
        res = IS_LIB_VALID;
    } else {
        cksum = checksum_calc((unsigned int *)0x00040600U, 0x17f8U, 0U);
        if (cksum == CKSUM_V125) {
            res = IS_LIB_VALID;
        }
    }
#elif defined(RN202x_RN7326_SOC_V2)
    #define   CKSUM_V126   (0x0879ec2eU)
    #define   CKSUM_V125   (0x49955a2dU)

    cksum = checksum_calc((unsigned int *)0x00080410U, 0x19e8U, 1U);
    if ((cksum == CKSUM_V126) ||
        (cksum == *(volatile unsigned int *)(0x00081df8U)))
    {
        res = IS_LIB_VALID;
    } else {
        cksum = checksum_calc((unsigned int *)0x00080600U, 0x17f8U, 0U);
        if (cksum == CKSUM_V125) {
            res = IS_LIB_VALID;
        }
    }
#endif

    return res;
#else
    /* Code or binary library always return valid */
    return IS_LIB_VALID;
#endif
}
/* r3009 */
