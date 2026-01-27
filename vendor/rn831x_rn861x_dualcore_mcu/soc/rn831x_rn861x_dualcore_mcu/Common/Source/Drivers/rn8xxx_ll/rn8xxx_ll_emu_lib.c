/******************************************************************************
 * @file     rn8xxx_ll_emu_lib.c
 * @brief    emu configuration
 * @author   Renergy Technology
 * @note
 * Copyright (C) 2024, Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#include "rn8xxx_ll_utils.h"
#include "rn8xxx_ll_emu_lib.h"

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B)

eEmuLibRet_TypeDef LL_EMU_ReadUEdtVBCtlReg(unsigned int RegData , unsigned char ModeSel)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_EMU_ReadUEdtVBCtlReg(RegData, ModeSel);
#else
    unsigned int vdet, uedtvb;
    eEmuLibRet_TypeDef res = EMU_LIB_PASS;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif

#if defined(RN_CM0_PLATFORM)
    if (chipid == CHIP_RA9303B)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B)
    {
        if(ModeSel == (unsigned char)UEDTModeSel_U400) {
            vdet = MEM32(VDET_CTRL_9303_ADDR);
            uedtvb = MEM32(UEDTVB_CTRL_9303_ADDR);
            if(((vdet & 0x71cU) != ((RegData & 0x700U) | ((RegData & 0x700U) >> 6))) ||
            ((uedtvb & 0x800U) != (RegData & 0x800U))) {
                res = EMU_LIB_FAIL;
            }
        } else {
            if((MEM32(UEDTVB_CTRL_9303_ADDR) & 0xf00U) != RegData) {
                res = EMU_LIB_FAIL;
            }
        }
    }
#endif
    return res;
#endif
}

eEmuLibRet_TypeDef LL_EMU_WriteUEdtVBCtlReg(unsigned int RegData , unsigned char ModeSel)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_EMU_WriteUEdtVBCtlReg(RegData, ModeSel);
#else
    unsigned int EdtVBCtlReg, sys_ps;
    unsigned char i;
    eEmuLibRet_TypeDef res = EMU_LIB_PASS;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif

#if defined(RN_CM0_PLATFORM)
    if (chipid == CHIP_RA9303B)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B)
    {
        sys_ps = MEM32(SYS_PS_9303_ADDR);
        MEM32(SYS_PS_9303_ADDR) = 0x82U;
        if(ModeSel == (unsigned char)UEDTModeSel_U400) {
            EdtVBCtlReg = (RegData & 0x700U);
            for(i = 0U; i < 3U; i++) {
                MEM32(VDET_CTRL_9303_ADDR) = (MEM32(VDET_CTRL_9303_ADDR) & 0xfffff8e3U) | EdtVBCtlReg | (EdtVBCtlReg >> 6) | 0x1U;
                if((MEM32(VDET_CTRL_9303_ADDR) & 0x71DU) == ((EdtVBCtlReg & 0x700U) | ((EdtVBCtlReg & 0x700U) >> 6) | (0x1U))) {
                    MEM32(UEDTVB_CTRL_9303_ADDR) = (MEM32(UEDTVB_CTRL_9303_ADDR) & 0xfffff7ffU) | (RegData & 0x800U);
                    if((MEM32(UEDTVB_CTRL_9303_ADDR) & 0x800U) == (RegData & 0x800U)) {
                        break;
                    }
                }
            }
            if( i >= 3U) {
                res = EMU_LIB_FAIL;
            }
        } else {
            EdtVBCtlReg = (RegData & 0xf00U);
            for(i = 0U; i < 3U; i++) {
                MEM32(UEDTVB_CTRL_9303_ADDR) = (MEM32(UEDTVB_CTRL_9303_ADDR) & 0xfffff0ffU) | EdtVBCtlReg;
                if((MEM32(UEDTVB_CTRL_9303_ADDR) & 0xf00U) == EdtVBCtlReg) {
                    break;
                }
            }
            if(i >= 3U) {
                res = EMU_LIB_FAIL;
            }
        }
    }
    MEM32(SYS_PS_9303_ADDR) = sys_ps;
#endif

    return res;
#endif
}
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B) ||\
    defined(RN202x_RN7326_SOC_V2)
#define SYS_CFG_REG_ADDR  (0x400340bcU)
eEmuLibRet_TypeDef LL_EMU_ExtRefConf(eExtRefSel_TypeDef ref_sel)
{
#if  defined(NVRLIB_EN)
    return rn_lib->LL_EMU_ExtRefConf(ref_sel);
#else
    unsigned int sys_ps, temp;
    eEmuLibRet_TypeDef res = EMU_LIB_FAIL;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif

#if defined(RN_CM0_PLATFORM)
    if (CHIP_RA9703A == chipid)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2)
    {
        sys_ps = MEM32(SYS_PS_9303_ADDR);
        MEM32(SYS_PS_9303_ADDR) = 0x82U;
        temp = MEM32(SYS_CFG_REG_ADDR);
        if (ref_sel == EXTREF_ENABLE) {
            MEM32(SYS_CFG_REG_ADDR) = (temp & (~((unsigned int)0xffU << 0))) | 0xe0U;
            MEM32(SYS_CFG_REG_ADDR) = (temp & (~((unsigned int)0xffU << 0))) | 0xe8U;
            if ((MEM32(SYS_CFG_REG_ADDR) & 0xffU) == 0xe8U) {
                res = EMU_LIB_PASS;
            }
        } else {
            MEM32(SYS_CFG_REG_ADDR) = (temp & (~((unsigned int)0xffU << 0))) | 0xe0U;
            MEM32(SYS_CFG_REG_ADDR) = (temp & (~((unsigned int)0xffU << 0)));
            if ((MEM32(SYS_CFG_REG_ADDR) & 0xffU) == 0x0U) {
                res = EMU_LIB_PASS;
            }
        }
        MEM32(SYS_PS_9303_ADDR) = sys_ps;
    }
#endif
#if defined(RN_CM0_PLATFORM)
    else if (CHIP_RA9303B == chipid)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_B)
    {
        sys_ps = MEM32(SYS_PS_9303_ADDR);
        MEM32(SYS_PS_9303_ADDR) = 0x75U;
        temp = MEM32(ANA_BGR_9303_ADDR);
        if (ref_sel == EXTREF_ENABLE) {
            MEM32(ANA_BGR_9303_ADDR) = (temp | ((unsigned int)0x1 << 1)) | 0xa5280000U;
            if ((MEM32(ANA_BGR_9303_ADDR) & ((unsigned int)0x1 << 1)) != 0x0U) {
                res = EMU_LIB_PASS;
            }
        } else {
            MEM32(ANA_BGR_9303_ADDR) = (temp & (~((unsigned int)0x1 << 1))) | 0xa5280000U;
            if ((MEM32(ANA_BGR_9303_ADDR) & ((unsigned int)0x1 << 1)) == 0x0U) {
                res = EMU_LIB_PASS;
            }
        }
        MEM32(SYS_PS_9303_ADDR) = sys_ps;
    }
#endif
#if defined(RN_CM0_PLATFORM)
    else {
        res = EMU_LIB_FAIL;
    }
#endif

    return res;
#endif
}
#endif
/* r2989 */
