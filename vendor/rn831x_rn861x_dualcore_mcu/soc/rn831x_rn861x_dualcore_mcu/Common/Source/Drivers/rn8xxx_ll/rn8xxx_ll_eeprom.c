/******************************************************************************
 * @file    rn8xxx_ll_eeprom.c
 * @brief    non-valatile eeprom memory driver
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) 2017, Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#include "rn8xxx_ll_utils.h"
#include "rn8xxx_ll_eeprom.h"
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN831x_RN861x_MCU_V1)
/* Register define */
#define OSC_CTRL1_REG_REG  (*(volatile unsigned int *)(0x40034000U))
#define MOD0_EN_REG    (*(volatile unsigned int *)(0x4003401cU))

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B)
#define MOD1_EN_REG    (*(volatile unsigned int *)(0x40034020U))
#endif
#define SYS_PS_REG     (*(volatile unsigned int *)(0x40034030U))
#define SYSTC_CTRL_REG (*(volatile unsigned int *)(0x50010000U))
#define SYSTC_LOAD_REG (*(volatile unsigned int *)(0x50010004U))
#define SYSTC_VAL_REG  (*(volatile unsigned int *)(0x50010008U))
#define WDT_EN_ADDR_REG     (*(volatile unsigned int *)(0x40030000U))
#define WDT_FEED   (WDT_EN_ADDR_REG = 0xbbU)

#define EEPROM_BASE_REMAP3 (0x08000000U) /*!< (EEPROM    ) Base Address */

#define EEPROM_TP (1U)
#define EEPROM_TE (2U)
#define EEPROM_TMS_PAGE (832U)
#define EEPROM_TMS_PAGE_32K (26U)

/* ----2020-07-13 9103C_EEPROMPROGRAM----- */
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN831x_RN861x_MCU_V1)
#define EEPROM_PAGE_SIZE_C0 (128U)
#define EEPROM_SIZE_C0 (EEPROM_SECTOR_SIZE * EEPROM_SECTOR_NUM)
#define ECMD_PE_C0 (10U)
#define ECMD_PP_C0 (14U)
#endif

/* ----2020-07-13 9103C_EEPROMPROGRAM----- */

#define EEPROM_PAGE_SIZE (64U)
#define EEPROM_SECTOR_NUM (8U)
#define EEPROM_SECTOR_SIZE (EEPROM_PAGE_SIZE * 64U)
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B)
#define EEPROM_SIZE (EEPROM_SECTOR_SIZE * EEPROM_SECTOR_NUM)
#endif
#define EEPROM_SP0_BASE (0x08c08000U)
#define EEPROM_SP0_OFFSET (0x00c08000U)
#define EEPROM_CMD_OFFSET (0x00800000U)  /*EEPROM Command Register Offset Address */
#define EEPROM_PGA_OFFSET (0x00800004U)  /*EEPROM Page Address Register Offset Address */
#define EEPROM_STA_OFFSET (0x00800008U)  /*EEPROM Status Register Offset Address */
#define EEPROM_PROT_OFFSET (0x00800014U) /*EEPROM Port Register Offset Address */
#define EEPROM_TW_OFFSET (0x00800018U)   /*EEPROM Wait time Register Offset Address */
#define EEPROM_TUS_OFFSET (0x00800020U)  /*EEPROM us time Register Offset Address */
#define EEPROM_TMS_OFFSET (0x00800024U)  /*EEPROM ms time Register Offset Address */
#define EEPROM_TP_OFFSET (0x00800028U)   /*EEPROM Program time Register Offset Address */
#define EEPROM_TE_OFFSET (0x0080002cU)   /*EEPROM Erase time Register Offset Address */

#define ECMD_HVPL (4U)
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B)
#define ECMD_PE (11U)
#define ECMD_PP (15U)
#endif

#define EEPROM_BUSY_CYCLE (((unsigned int)20U << 8) | 10U) /* 4.4ms..305us*20 = 6ms.  4.4 ms (64 cycles) The maximum counter for EEPROM is */

static void __swDelay(unsigned int time);
static void __trimFill(unsigned int dst_addr/*, unsigned int mode*/);
static void __checkFlag(unsigned int reg_addr, unsigned short wait_val, unsigned int loop_time);

static void __swDelay(unsigned int time)
{
    unsigned int i;
    unsigned int cur_freq;

#if defined(RN_CM0_PLATFORM)
    cur_freq = currentClk(CHIP_RA9103D);
#else
    cur_freq = currentClk();
#endif

    /*if (time != 0U)*/
    {
        SYS_PS_REG = 0x82U;
        MOD0_EN_REG |= ((unsigned int)1U << 0);
        SYSTC_LOAD_REG = 0xffffffffU;
        SYSTC_CTRL_REG = 1U;

        for (i = 0U; i < (time * (cur_freq / 32768U)); i++)
        {
            if (SYSTC_VAL_REG >= (time * (cur_freq / 32768U)))
            {
                break;
            }
        }

        SYSTC_CTRL_REG = 0x0U;
    }
    return;
}

/*********************************************************************************************************
** Function name:       trimFill
** Descriptions:        Fill trim parameter to FLASH/EEPROM
** input parameters:    None
** output parameters:   result
** Returned value:      None
*********************************************************************************************************/
static void __trimFill(unsigned int dst_addr/*, unsigned int mode*/)
{
    unsigned int sysclk_state;
    const unsigned char TW_TABLE[] = {0x2U, 0x0U, 0x0U, 0x0U, 0x2U, 0x5U}; /* 7.3728, 3.6864, 1.8432, 32.768K, 14.7456M, 29.4912M */
    const unsigned char TUS_TABLE[] = {7U, 3U, 1U, 1U, 15U, 31U};

    sysclk_state = ((OSC_CTRL1_REG_REG & 0x700U) >> 8);
    sysclk_state = (sysclk_state > 0x5U) ? 0x2U : sysclk_state;

    *(volatile unsigned int *)(dst_addr + EEPROM_TW_OFFSET) = (unsigned int)TW_TABLE[sysclk_state] << 16;
    *(volatile unsigned int *)(dst_addr + EEPROM_TUS_OFFSET) = (unsigned int)TUS_TABLE[sysclk_state];

    if (sysclk_state != 0x3U)
    {
        /*if (((mode >> 4) & 0x1U) == 0x1U)
        {
            *(volatile unsigned int *)(dst_addr + EEPROM_TMS_OFFSET) = EEPROM_TMS_CHIP;
        }
        else*/
        {
            *(volatile unsigned int *)(dst_addr + EEPROM_TMS_OFFSET) = EEPROM_TMS_PAGE;
        }
    }
    else
    {
        /*if (((mode >> 4) & 0x1U) == 0x1U)
        {
            *(volatile unsigned int *)(dst_addr + EEPROM_TMS_OFFSET) = EEPROM_TMS_CHIP_32K;
        }
        else*/
        {
            *(volatile unsigned int *)(dst_addr + EEPROM_TMS_OFFSET) = EEPROM_TMS_PAGE_32K;
        }
    }

    *(volatile unsigned int *)(dst_addr + EEPROM_TP_OFFSET) = EEPROM_TP;
    *(volatile unsigned int *)(dst_addr + EEPROM_TE_OFFSET) = EEPROM_TE;
}

static void __checkFlag(unsigned int reg_addr, unsigned short wait_val, unsigned int loop_time)
{
    unsigned short i;
    unsigned short temp;

    for (i = 0U; i < ((loop_time & 0xffff00U) >> 8); i++)
    {
        temp = (unsigned short)(*(volatile unsigned int *)(reg_addr)) & wait_val;
        if ((/*((loop_time & 0x80000000U) != 0x80000000U) && */(temp != wait_val))/* ||
            (((loop_time & 0x80000000U) == 0x80000000U) && (temp == wait_val))*/)
        {
            break;
        }

        __swDelay(loop_time & 0xffU);
    }

    return;
}
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_B)
static eEepromRet_TypeDef __eepromProgram(unsigned int dst_addr, unsigned int src_addr, unsigned int byte_num)
{
    eEepromRet_TypeDef result;
    unsigned char write_size;
    unsigned char page_res;
    unsigned int temp;
    unsigned int start_addr;
    unsigned int page_num;
    unsigned int page_start;
    unsigned int i;
    unsigned int j;
    unsigned int eeprom_base_addr;

    eeprom_base_addr = 0x08000000U;

    if ((byte_num == 0U) || (byte_num > EEPROM_SIZE))
    {
        result = EEPROM_COUNT_ERROR;
    }
    else if ((dst_addr < eeprom_base_addr) ||
      ((dst_addr >= (eeprom_base_addr + EEPROM_SIZE)) && (dst_addr < (eeprom_base_addr + EEPROM_SP0_OFFSET))) ||
      (((dst_addr + byte_num) > (eeprom_base_addr + EEPROM_SIZE)) && ((dst_addr + byte_num) < (eeprom_base_addr + EEPROM_SP0_OFFSET))) ||
      (dst_addr >= (((eeprom_base_addr + EEPROM_SP0_BASE) - EEPROM_BASE_REMAP3) + (2U * EEPROM_PAGE_SIZE))) ||
      ((dst_addr + byte_num) > (((eeprom_base_addr + EEPROM_SP0_BASE) - EEPROM_BASE_REMAP3) + (2U * EEPROM_PAGE_SIZE))))
    {
        result = EEPROM_DST_ADDR_NOT_MAPPED;
    }
    else
    {
        __trimFill(eeprom_base_addr);

        /* Progam EEPROOM */
        start_addr = dst_addr & (EEPROM_PAGE_SIZE - 1U);
        temp = (byte_num < (EEPROM_PAGE_SIZE - start_addr)) ? 0U : (byte_num - (EEPROM_PAGE_SIZE - start_addr));
        temp = (start_addr != 0x0U) ? temp : byte_num;
        page_num = temp / EEPROM_PAGE_SIZE;
        page_num = (start_addr != 0x0U) ? (page_num + 1U) : page_num;
        page_res = (unsigned char)(temp % EEPROM_PAGE_SIZE);
        page_num = (page_res != 0x0U) ? (page_num + 1U) : page_num;
        page_start = (dst_addr & 0xffffU) / EEPROM_PAGE_SIZE;

        /* Open Protect Bit */
        *(unsigned int *)(eeprom_base_addr + EEPROM_PROT_OFFSET) = 0xfeed0000U;

        temp = 0U;

        for (i = 0U; i < page_num; i++)
        {
            SYS_PS_REG = 0x82U;
            MOD1_EN_REG |= ((unsigned int)1U << 9);
            WDT_FEED;

            *(volatile unsigned int *)(eeprom_base_addr + EEPROM_CMD_OFFSET) = 0xfeed0000U + ((unsigned int)ECMD_HVPL << 0);

            if ((page_res != 0x0U) && (i == (page_num - 0x1U)))
            {
                write_size = page_res;
            }
            else
            {
                if ((start_addr != 0U) && (i == 0U))
                {
                    write_size = (page_num == 1U) ? (unsigned char)byte_num : (unsigned char)(EEPROM_PAGE_SIZE - start_addr);
                }
                else
                {
                    write_size = EEPROM_PAGE_SIZE;
                }
            }

            for (j = 0U; j < write_size; j++)
            {
                if (i == 0U)
                {
                    *(volatile unsigned char *)(eeprom_base_addr + start_addr + j) = *(unsigned char *)(src_addr + temp);
                }
                else
                {
                    *(volatile unsigned char *)(eeprom_base_addr + j) = *(unsigned char *)(src_addr + temp);
                }
                temp = temp + 1U;
            }

            *(volatile unsigned int *)(eeprom_base_addr + EEPROM_PGA_OFFSET) = page_start + i;
            __trimFill(eeprom_base_addr);

            *(volatile unsigned int *)(eeprom_base_addr + EEPROM_CMD_OFFSET) = 0xfeed0000U + ((unsigned int)ECMD_PE << 0);
            __checkFlag(eeprom_base_addr + EEPROM_STA_OFFSET, 0x1U, EEPROM_BUSY_CYCLE);
            __trimFill(eeprom_base_addr);

            *(volatile unsigned int *)(eeprom_base_addr + EEPROM_CMD_OFFSET) = 0xfeed0000U + ((unsigned int)ECMD_PP << 0);

            __checkFlag(eeprom_base_addr + EEPROM_STA_OFFSET, 0x1U, EEPROM_BUSY_CYCLE);
        }

        *(volatile unsigned int *)(eeprom_base_addr + EEPROM_PROT_OFFSET) = 0xfeedffffU;

        result = EEPROM_PASS;
    }

    return result;
}
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN831x_RN861x_MCU_V1)
static eEepromRet_TypeDef __eepromProgram_9103c(unsigned int dst_addr, unsigned int src_addr, unsigned int byte_num)
{
    eEepromRet_TypeDef result;
    unsigned int write_size, write_word_size;
    unsigned int page_num;
    unsigned int i;
    unsigned int page;
    unsigned int eeprom_base_addr;
    unsigned int buf[EEPROM_PAGE_SIZE_C0 / 4U];
    unsigned int head_addr, tail_addr;
    unsigned int head_bytes;
    unsigned int src_cnt = 0U;
    unsigned char *pb;

    eeprom_base_addr = 0x08000000U;

    if ((byte_num == 0U) || (byte_num > EEPROM_SIZE_C0))
    {
        result = EEPROM_COUNT_ERROR;
    }
    else if ((dst_addr < eeprom_base_addr) ||
        ((dst_addr >= (eeprom_base_addr + EEPROM_SIZE_C0)) && (dst_addr < (eeprom_base_addr + EEPROM_SP0_OFFSET))) ||
        (((dst_addr + byte_num) > (eeprom_base_addr + EEPROM_SIZE_C0)) && ((dst_addr + byte_num) < (eeprom_base_addr + EEPROM_SP0_OFFSET))) ||
        (dst_addr >= (((eeprom_base_addr + EEPROM_SP0_BASE) - EEPROM_BASE_REMAP3) + (2U * EEPROM_PAGE_SIZE_C0))) ||
        ((dst_addr + byte_num) > (((eeprom_base_addr + EEPROM_SP0_BASE) - EEPROM_BASE_REMAP3) + (2U * EEPROM_PAGE_SIZE_C0))))
    {
        result = EEPROM_DST_ADDR_NOT_MAPPED;
    }
    else
    {
        __trimFill(eeprom_base_addr);

        head_addr = dst_addr & (EEPROM_PAGE_SIZE_C0 - 1U);
        tail_addr = (dst_addr + byte_num - 1U) & (EEPROM_PAGE_SIZE_C0 - 1U);
        page_num = ((dst_addr + byte_num - 1U) & (~((unsigned int)EEPROM_PAGE_SIZE_C0 - 1U))) - (((dst_addr & (~((unsigned int)EEPROM_PAGE_SIZE_C0 - 1U)))) / EEPROM_PAGE_SIZE_C0) + 1U;
        if (page_num == 1U)
        {
            head_bytes = byte_num;
        }
        else
        {
            head_bytes = EEPROM_PAGE_SIZE_C0 - head_addr;
        }
        for (page = 0U; page < page_num; page++)
        {
            WDT_FEED;

            *(volatile unsigned int *)(eeprom_base_addr + EEPROM_CMD_OFFSET) = 0xfeed0000U + ((unsigned int)ECMD_HVPL << 0);

            if (page == 0U)
            {
                write_size = head_bytes;
                /* read back by word */
                buf[head_addr / 4U] = *(unsigned int *)(dst_addr & 0xfffffffcU);
                if (page_num == 1U)
                {
                    buf[(tail_addr / 4U)] = *(unsigned int *)((dst_addr + byte_num - 1U) & 0xfffffffcU);
                    write_word_size = ((tail_addr & (~(unsigned int)0x3U)) - (((head_addr & (~(unsigned int)0x3U))) / 4U)) + 1U;
                }
                else
                {
                    write_word_size = (((EEPROM_PAGE_SIZE_C0 - 1U) & (~(unsigned int)0x3U)) - (((head_addr & (~(unsigned int)0x3U))) / 4U)) + 1U;
                }
            }
            else if (page == (page_num - 1U))
            {
                buf[tail_addr / 4U] = *(unsigned int *)((dst_addr + byte_num - 1U) & 0xfffffffcU);
                write_size = tail_addr + 1U;
                write_word_size = (((tail_addr & (~(unsigned int)0x3U))) / 4U) + 1U;
            }
            else
            {
                write_size = EEPROM_PAGE_SIZE_C0;
                write_word_size = write_size / 4U;
            }

            for (i = 0U; i < write_size; i++)
            {
                if (page == 0U)
                {
                    pb = (unsigned char *)((unsigned int)(buf) + head_addr + i);
                }
                else
                {
                    pb = (unsigned char *)((unsigned int)(buf) + i);
                }
                *pb = (*(unsigned char *)(src_addr + src_cnt + i));
            }
            src_cnt += write_size;
            for (i = 0U; i < (write_word_size); i++)
            {
                if (page == 0U)
                {
                    *(volatile unsigned int *)(eeprom_base_addr + (head_addr & (~(unsigned int)3U)) + (4U * i)) = *(unsigned int *)((unsigned int)buf + (head_addr & (~(unsigned int)3U)) + (4U * i));
                }
                else
                {
                    *(volatile unsigned int *)(eeprom_base_addr + (4U * i)) = buf[i&((EEPROM_PAGE_SIZE_C0 / 4U) - 1U)];
                }
            }

            *(volatile unsigned int *)(eeprom_base_addr + EEPROM_PGA_OFFSET) = (dst_addr / EEPROM_PAGE_SIZE_C0) + page;
            __trimFill(eeprom_base_addr);

            *(volatile unsigned int *)(eeprom_base_addr + EEPROM_CMD_OFFSET) = 0xfeed0000U + ((unsigned int)ECMD_PE_C0 << 0U);
            __checkFlag(eeprom_base_addr + EEPROM_STA_OFFSET, 0x1U, EEPROM_BUSY_CYCLE);

            __trimFill(eeprom_base_addr);

            *(volatile unsigned int *)(eeprom_base_addr + EEPROM_CMD_OFFSET) = 0xfeed0000U + ((unsigned int)ECMD_PP_C0 << 0U);
            __checkFlag(eeprom_base_addr + EEPROM_STA_OFFSET, 0x1U, EEPROM_BUSY_CYCLE);
        }

        *(volatile unsigned int *)(eeprom_base_addr + EEPROM_PROT_OFFSET) = 0xfeedffffU;

        result = EEPROM_PASS;
    }

    return result;
}
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_C) ||\
    defined(RN821x_RN721x_SOC_B) ||\
    defined(RN831x_RN861x_MCU_V1)
eEepromRet_TypeDef LL_EEPROM_Program(unsigned int dst_addr, unsigned int src_addr, unsigned int len)
{
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif
    eEepromRet_TypeDef res;

    if (((MEM32(OSC_CTRL1_ADDR) >> 4) & 0x1U) == 0x1U) {
        res = EEPROM_CLK_ERROR;
    } else {
#if defined(RN_CM0_PLATFORM)
        if (chipid == CHIP_RA9103D)
        {
            (void)IapMemEnable(EE_MEM_EN);
            res = __eepromProgram(dst_addr, src_addr, len);
        }
        else if ((chipid == CHIP_RA9103C) || (chipid == CHIP_RA9103E))
        {
            (void)IapMemEnable(EE_MEM_EN);
            res = __eepromProgram_9103c(dst_addr, src_addr, len);
        }
        else
        {
            res = EEPROM_SRC_ADDR_NOT_MAPPED;
        }
#elif defined(RN821x_RN721x_SOC_B)
        (void)IapMemEnable(EE_MEM_EN);
        res = __eepromProgram(dst_addr, src_addr, len);
#elif defined(RN821x_RN721x_SOC_C)
        (void)IapMemEnable(EE_MEM_EN);
        res = __eepromProgram_9103c(dst_addr, src_addr, len);
#elif defined(RN831x_RN861x_MCU_V1)
        if (get_mcu_v1_version() == CHIP_RA9103E) {
            (void)IapMemEnable(EE_MEM_EN);
            res = __eepromProgram_9103c(dst_addr, src_addr, len);
        } else {
            res = EEPROM_PASS;
        }
#else
        (void)dst_addr;
        (void)src_addr;
        (void)len;
#endif
    }

    return res;
}

void LL_EEPROM_Standby(void)
{
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
    const unsigned int reg = getMipAddr(1U, chipid);
#elif defined(RN821x_RN721x_SOC_C) ||\
      defined(RN821x_RN721x_SOC_B) ||\
      defined(RN831x_RN861x_MCU_V1)
    const unsigned int reg = EEPROM_BASE_ADDRESS;
#endif

#if defined(RN_CM0_PLATFORM)
    if ((chipid == CHIP_RA9103C) || (chipid == CHIP_RA9103E)) {
        *(volatile unsigned int *)(reg + 0x00800000U) = 0xfeed000fU;
    }
    else if (/*(chipid == CHIP_RA9103A) || */(chipid == CHIP_RA9103D)) {
        *(volatile unsigned int *)(reg + 0x00800000U) = 0xfeed0007U;
    }
    else {
      /* eeprom is not supported */
    }
#elif defined(RN821x_RN721x_SOC_C)
    *(volatile unsigned int *)(reg + 0x00800000U) = 0xfeed000fU;
#elif defined(RN821x_RN721x_SOC_B)
    *(volatile unsigned int *)(reg + 0x00800000U) = 0xfeed0007U;
#elif defined(RN831x_RN861x_MCU_V1)
    if (get_mcu_v1_version() == CHIP_RA9103E) {
        *(volatile unsigned int *)(reg + 0x00800000U) = 0xfeed000fU;
    }
#endif

}

void LL_EEPROM_Wakeup(void)
{
#if defined(RN_CM0_PLATFORM)
    unsigned int i, reg;
    const rn_chipid_t chipid = getChipid();

    /* if ((chipid != CHIP_RA9103F) && (chipid != CHIP_RA9103G)) */
    if (/*(chipid == CHIP_RA9103A) || */(chipid == CHIP_RA9103D) || (chipid == CHIP_RA9103C) || (chipid == CHIP_RA9103E))
    {
        reg = getMipAddr(1U, chipid);
        *(volatile unsigned int *)(reg + 0x00800000U) = 0xfeed0004U;
        for (i = 0U; i < 10000U; i++) {
            if ((*(volatile unsigned int *)(reg + 0x00800008U) & 0x1U) == 0x0U) {
                break;
            }
        }
    }
#elif defined(RN821x_RN721x_SOC_B) ||\
      defined(RN821x_RN721x_SOC_C)
    unsigned int i, reg;
    reg = EEPROM_BASE_ADDRESS;
    *(volatile unsigned int *)(reg + 0x00800000U) = 0xfeed0004U;
    for (i = 0U; i < 10000U; i++) {
        if ((*(volatile unsigned int *)(reg + 0x00800008U) & 0x1U) == 0x0U) {
            break;
        }
    }
#elif defined(RN831x_RN861x_MCU_V1)
    unsigned int i, reg;
    if (get_mcu_v1_version() == CHIP_RA9103E) {
        reg = EEPROM_BASE_ADDRESS;
        *(volatile unsigned int *)(reg + 0x00800000U) = 0xfeed0004U;
        for (i = 0U; i < 10000U; i++) {
            if ((*(volatile unsigned int *)(reg + 0x00800008U) & 0x1U) == 0x0U) {
                break;
            }
        }
    }
#endif
}
#endif
/* r2989 */
