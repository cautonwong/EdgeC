/******************************************************************************
 * @file     rn8xxx_ll_flash.c
 * @brief    non-valatile flash memory driver
 * @author   Renergy Technology
 * @note
 * Copyright (C) 2008, Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#include "rn8xxx_ll_utils.h"
#include "rn8xxx_ll_flash.h"

#if defined(RN831x_RN861x_MCU_V4)
typedef struct
{
    unsigned int Reserved0;
    unsigned int (*FLASH_Unlock)       (unsigned int FlashID, unsigned int DstAddress, unsigned int ByteLength);
    unsigned int (*FLASH_Erase)        (unsigned int FlashID, unsigned int DstAddress, unsigned int ByteLength);
    unsigned int (*FLASH_Program)      (unsigned int FlashID, unsigned int DstAddress, unsigned int SrcAddress, unsigned int ByteLength);
} FLASH_ApiTypeDef;
typedef unsigned int (*pFLASH_Unlock)  (unsigned int FlashID, unsigned int DstAddress, unsigned int ByteLength);
typedef unsigned int (*pFLASH_Erase)   (unsigned int FlashID, unsigned int DstAddress, unsigned int ByteLength);
typedef unsigned int (*pFLASH_Program) (unsigned int FlashID, unsigned int DstAddress, unsigned int SrcAddress, unsigned int ByteLength);
#else
/* Register define */

/* IAP command define*/
#define FL_PAGE_ERASE 0x50U
#define FL_SECT_ERASE 0x51U
#define FL_PROG 0x58U

#define FLASH_PAGE_SIZE_NVM  (512U)
#endif

#if (defined(RN_CM0_PLATFORM) ||\
     defined(RN202x_RN7326_SOC_B) ||\
     defined(RN821x_RN721x_SOC_B) ||\
     defined(RN821x_RN721x_SOC_C) ||\
     defined(RN821x_RN721x_SOC_D) ||\
     defined(RN831x_RN861x_MCU_V1)) && !defined(NVRLIB_EN)
static void cache_swith(unsigned char flag);
#endif

eFlashRet_TypeDef LL_FLASH_PageErase(unsigned int pg)
{
#if !defined(RN831x_RN861x_MCU_V4)
#if  defined(NVRLIB_EN)
    return rn_lib->LL_FLASH_PageErase(pg);
#else
    rn_iap_t iap;
    eFlashRet_TypeDef res;
    rn_iap_entry_t ptr_union;

    ptr_union.addr = ROM_BASE_ADDRESS + IAP_OFFSET;
    if (((MEM32(OSC_CTRL1_ADDR) >> 4) & 0x1U) == 0x1U) {
        res = FLASH_CLK_ERROR;
    } else {
        iap.cmd = FL_PAGE_ERASE;
        iap.par[0] = pg;

        (void)IapMemEnable(FL_MEM_EN);
        ptr_union.iap_func(&iap.cmd, &iap.stat);
        res = (eFlashRet_TypeDef)iap.stat;
    }

    return res;
#endif
#else
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return FLASH_INVALID_COMMAND;
    }
    FLASH_ApiTypeDef *Iap_Func = (FLASH_ApiTypeDef *)iap_addr;
    pFLASH_Erase func_ptr = (pFLASH_Erase)((uint32_t)Iap_Func->FLASH_Erase + ROM_BASE_ADDR);
    return func_ptr(0U, (pg * 512U), 512U);
#endif
}

eFlashRet_TypeDef LL_FLASH_SectorErase(unsigned int sec)
{
#if !defined(RN831x_RN861x_MCU_V4)
#if  defined(NVRLIB_EN)
    return rn_lib->LL_FLASH_SectorErase(sec);
#else
    rn_iap_t iap;
    rn_iap_entry_t ptr_union;

    ptr_union.addr = ROM_BASE_ADDRESS + IAP_OFFSET;
    if (((MEM32(OSC_CTRL1_ADDR) >> 4) & 0x1U) == 0x1U) {
        iap.stat = (unsigned long)FLASH_CLK_ERROR;
    } else {
        iap.cmd = FL_SECT_ERASE;
        iap.par[0] = sec;

        (void)IapMemEnable(FL_MEM_EN);
        ptr_union.iap_func(&iap.cmd, &iap.stat);
    }
    return (eFlashRet_TypeDef)iap.stat;
#endif
#else
    unsigned int i;
    eFlashRet_TypeDef status;
    for (i = 0U; i < 32U; i++) {
    	status = LL_FLASH_PageErase((sec * 32U) + i);
        if (FLASH_CMD_SUCCESS != status) {
            break;
        }
    }
    return status;
#endif
}

eFlashRet_TypeDef LL_FLASH_Program(unsigned int dst_addr, unsigned int src_addr, unsigned int len)
{
#if !defined(RN831x_RN861x_MCU_V4)
#if  defined(NVRLIB_EN)
    return rn_lib->LL_FLASH_Program(dst_addr, src_addr, len);
#else
    rn_iap_t iap;
    rn_iap_entry_t ptr_union;
    unsigned char pg_buf[FLASH_PAGE_SIZE_NVM];
    unsigned int i, j;
    unsigned int page_len, page_num, wr_len;
    unsigned char page_width;
    eFlashRet_TypeDef res = FLASH_PASS;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif

    ptr_union.addr = ROM_BASE_ADDRESS + IAP_OFFSET;
    if (((MEM32(OSC_CTRL1_ADDR) >> 4) & 0x1U) == 0x1U) {
        res = FLASH_CLK_ERROR;
    } else {
#if defined(RN_CM0_PLATFORM)
        if (/*(CHIP_RA9103A == chipid) ||*/
            (CHIP_RA9103D == chipid) ||
            (CHIP_RA9103C == chipid) ||
            (CHIP_RA9103E == chipid)) {
            page_len = 128U;
            page_width = 7U;
        } else if ((CHIP_RA9103F == chipid) || (CHIP_RA9103G == chipid) || (CHIP_RA9303B == chipid)) {
            page_len = 256U;
            page_width = 8U;
        } else {
            page_len = 512U;
            page_width = 9U;
        }
#elif defined(RN821x_RN721x_SOC_B) ||\
      defined(RN821x_RN721x_SOC_C)
        page_len = 128U;
        page_width = 7U;
#elif defined(RN821x_RN721x_SOC_D) ||\
      defined(RN202x_RN7326_SOC_B)
        page_len = 256U;
        page_width = 8U;
#elif defined(RN831x_RN861x_MCU_V1)
        if (get_mcu_v1_version() == CHIP_RA9103E) {
            page_len = 128U;
            page_width = 7U;
        } else {
            page_len = 256U;
            page_width = 8U;
        }
#else
        page_len = 512U;
        page_width = 9U;
#endif

        if ((src_addr & 0x3U) != 0x0U) {
            /* src_addr not word aligned */
            page_num = len >> page_width;
            if ((len & (page_len - 1U)) != 0x0U) {
                page_num++;
            }
            wr_len = page_len;
            for (i = 0U; i < page_num; i++) {
                if ((i == (page_num - 1U)) && ((len & (page_len - 1U)) != 0x0U)) {
                    wr_len = len & (page_len - 1U);
                }
                for (j = 0U; j < wr_len; j++) {
                    pg_buf[j] = *(unsigned char *)(src_addr + (i * page_len) + j);
                }

                iap.cmd = FL_PROG;
                iap.par[0] = dst_addr;
                iap.par[1] = (unsigned int)pg_buf;
                iap.par[2] = page_len;

                (void)IapMemEnable(FL_MEM_EN);
                ptr_union.iap_func(&iap.cmd, &iap.stat);
                res = (eFlashRet_TypeDef)iap.stat;
                dst_addr += wr_len;
                if (res != FLASH_PASS) {
                    break;
                }
            }
        } else {
            if (len < page_len) {
                for (i = 0U; i < len; i++) {
                    pg_buf[i] = (*(unsigned char *)(src_addr + i));
                }
                for (i = len; i < page_len; i++) {
                    pg_buf[i] = 0xffU;
                }
                src_addr = (unsigned int)pg_buf;
                len = page_len;
            }
            iap.cmd = FL_PROG;
            iap.par[0] = dst_addr;
            iap.par[1] = src_addr;
            iap.par[2] = len;

            (void)IapMemEnable(FL_MEM_EN);
            ptr_union.iap_func(&iap.cmd, &iap.stat);
            res = (eFlashRet_TypeDef)iap.stat;
        }
    }

    return res;
#endif
#else
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return FLASH_INVALID_COMMAND;
    }
    FLASH_ApiTypeDef *Iap_Func = (FLASH_ApiTypeDef *)iap_addr;
    pFLASH_Program func_ptr = (pFLASH_Program)((uint32_t)Iap_Func->FLASH_Program + ROM_BASE_ADDR);
    return func_ptr(0U, dst_addr, src_addr, len);
#endif
}

#if (defined(RN_CM0_PLATFORM) ||\
     defined(RN202x_RN7326_SOC_B) ||\
     defined(RN821x_RN721x_SOC_B) ||\
     defined(RN821x_RN721x_SOC_C) ||\
     defined(RN821x_RN721x_SOC_D) ||\
     defined(RN831x_RN861x_MCU_V1)) && !defined(NVRLIB_EN)
/* flag = 0, OFF, flag = 1, ON */
static void cache_swith(unsigned char flag)
{
    unsigned int reg;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif

#if defined(RN_CM0_PLATFORM)
    if ((chipid == CHIP_RA9103F) || (chipid == CHIP_RA9303B)) {
        reg = 0x50014060U;
    } else if (chipid == CHIP_RA9103G) {
        reg = 0x00800060U;
    } else if ((chipid == CHIP_RA9701A) || (chipid == CHIP_RA9701B) || (chipid == CHIP_RA9702B)) {
        /* 9701 has no cache, write useless STA register to keep code compability */
        reg = 0x5001401cU;
    } else if ((chipid == CHIP_RA9702A) || (chipid == CHIP_RA9703A)) {
        reg = 0x5001402cU;
    } else {
        reg = getMipAddr(0U, chipid) + 0x0080001cU;
    }
    *(volatile unsigned int *)(reg) = (flag == 0U) ? 0xfeed0000U : 0xfeed0001U;
#elif defined(RN821x_RN721x_SOC_D)
    reg = 0x00800060U;
    *(volatile unsigned int *)(reg) = (flag == 0U) ? 0xfeed0000U : 0xfeed0001U;
#elif defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_V2)
    reg = 0x5001402cU;
    *(volatile unsigned int *)(reg) = (flag == 0U) ? 0xfeed0000U : 0xfeed0001U;
#elif defined(RN831x_RN861x_MCU_V1)
    if (get_mcu_v1_version() == CHIP_RA9103E) {
        reg = 0x0080001cU;
    } else {
        reg = 0x50014060U;
    }
    *(volatile unsigned int *)(reg) = (flag == 0U) ? 0xfeed0000U : 0xfeed0001U;
#elif defined(RN821x_RN721x_SOC_B) ||\
      defined(RN821x_RN721x_SOC_C)
    reg = 0x0080001cU;
    *(volatile unsigned int *)(reg) = (flag == 0U) ? 0xfeed0000U : 0xfeed0001U;
#elif defined(RN202x_RN7326_SOC_B)
    reg = 0x50014060U;
    *(volatile unsigned int *)(reg) = (flag == 0U) ? 0xfeed0000U : 0xfeed0001U;
#endif
}

#if !(defined(RN202x_RN7326_SOC_V2) ||\
      defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN831x_RN861x_MCU_V3) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN821x_RN721x_SOC_V3))
void LL_FLASH_CacheOff(void)
{
#if  defined(NVRLIB_EN)
    rn_lib->LL_FLASH_CacheOff();
#else
    cache_swith(0U);
#endif
}

void LL_FLASH_CacheOn(void)
{
#if  defined(NVRLIB_EN)
    rn_lib->LL_FLASH_CacheOn();
#else
    cache_swith(1U);
#endif
}
#endif

#endif
/* r2989 */
