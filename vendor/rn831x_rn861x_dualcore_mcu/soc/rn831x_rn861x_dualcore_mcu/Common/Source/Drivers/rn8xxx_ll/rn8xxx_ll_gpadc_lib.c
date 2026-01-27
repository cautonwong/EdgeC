/******************************************************************************
 * @file     rn8xxx_ll_gpadc_lib.c
 * @brief    GPADC configuration
 * @author   Renergy Technology
 * @note
 * Copyright (C) 2024, Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#include "rn8xxx_ll_utils.h"
#include "rn8xxx_ll_gpadc_lib.h"

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B) ||\
    defined(RN202x_RN7326_SOC_V2)

#define  GPADC_TADJ_OFFSET_9303  (0x90U)
#define  GPADC_TADJ_OFFSET_9703  (0xb8U)

eGpadcLibRet_TypeDef LL_GPADC_TadjGet(unsigned int *tadj)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_GPADC_TadjGet(tadj);
#else
    eGpadcLibRet_TypeDef res = GPADC_LIB_FAIL;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif
    unsigned int nvr_base_addr, nvr_size, nvr_offset, temp;

    if (tadj != 0U) {
#if defined(RN_CM0_PLATFORM)
        if ((CHIP_RA9303B == chipid) || (CHIP_RA9703A == chipid))
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B) ||\
    defined(RN202x_RN7326_SOC_V2)
        {
#if defined(RN_CM0_PLATFORM)
            if (CHIP_RA9303B == chipid)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B)
            {
                nvr_base_addr = 0x00040000U;
                nvr_size = 256U;
                nvr_offset = GPADC_TADJ_OFFSET_9303;
            }
#endif
#if defined(RN_CM0_PLATFORM)
            else
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2)
            {
                nvr_base_addr = 0x00080000U;
                nvr_size = 512U;
                nvr_offset = GPADC_TADJ_OFFSET_9703;
            }
#endif
            if (check_spl_valid((unsigned int *)nvr_base_addr, nvr_size, 0U) == 1U) {
#if defined(RN_CM0_PLATFORM)
                if (0U == get_sysreg_reliable(chipid, nvr_base_addr + nvr_offset, &temp, 0U))
#else
                if (0U == get_sysreg_reliable(nvr_base_addr + nvr_offset, &temp, 0U))
#endif
                {
                    *tadj = temp & 0xffffU;
                    res = GPADC_LIB_PASS;
                }
            }
        }
#endif
    }

    return res;
#endif
}

#endif
/* r2989 */
