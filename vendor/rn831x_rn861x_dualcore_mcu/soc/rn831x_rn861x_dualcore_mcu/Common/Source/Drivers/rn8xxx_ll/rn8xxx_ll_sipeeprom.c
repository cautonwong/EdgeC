/******************************************************************************
 * @file     sip_eeprom.c
 * @brief    sip eeprom driver
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) 2024, Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#include "rn8xxx_ll_utils.h"
#include "rn8xxx_ll_sipeeprom.h"

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3)

/**
 * 名称  RA9701A    RA9701B     RA9702A     RA9702B     RA9703A
 *  SDA  P141       P140        P140        P141        P74
 *  SCL  P142       P141        P142        P147        P75
 *  PWR  VCC        P142        VCC         VCC         VCC
 *
 * EEPROM Timing:
 *  Tstart_setup:    0.26us  ***
 *  Tstart_hold:     0.26us  ***
 *  Tscl_low:        0.5us   ***
 *  Tscl_high:       0.5us   ***
 *  Tscllow_dataout: 0.45us  ***
 *  Tdata_setup:     50ns
 *  Tdata_hold:      0ns
 *  Tstop_setup:     0.26us  ***
 *
 */
#define  WR_RETRY_NUM   (160U)  /* 3ms for previous eeprom writting */
#define  I2C_ADDR       (0U)
#define  DEV_ADDR       (0xa0U | ((unsigned char)I2C_ADDR << 1))

#if defined(RN_CM0_PLATFORM)
static void SET_SDA_OUTPUT(rn_chipid_t chipid)
{
    if ((chipid == CHIP_RA9701A) || (chipid == CHIP_RA9702B)) {
        MEM32(GPIO_BB_PMD_ADDR + (0x4U * 17U)) = 0U;
    } else if (((chipid == CHIP_RA9701B) || (chipid == CHIP_RA9702A))) {
        MEM32(GPIO_BB_PMD_ADDR + (0x4U * 16U)) = 0U;
    } else {
        MEM32(GPIO_BB_PMB1_ADDR + (0x4U * 20U)) = 0U;
    }
}

static void SET_SDA_INPUT(rn_chipid_t chipid)
{
    if ((chipid == CHIP_RA9701A) || (chipid == CHIP_RA9702B)) {
        MEM32(GPIO_BB_PMD_ADDR + (0x4U * 17U)) = 1U;
    } else if (((chipid == CHIP_RA9701B) || (chipid == CHIP_RA9702A))) {
        MEM32(GPIO_BB_PMD_ADDR + (0x4U * 16U)) = 1U;
    } else {
        MEM32(GPIO_BB_PMB1_ADDR + (0x4U * 20U)) = 1U;
    }
}

static void SET_SDA_HIGH(rn_chipid_t chipid)
{
    if ((chipid == CHIP_RA9701A) || (chipid == CHIP_RA9702B)) {
        MEM32(GPIO_BB_PD_ADDR + (0x4U * 17U)) = 1U;
    } else if (((chipid == CHIP_RA9701B) || (chipid == CHIP_RA9702A))) {
        MEM32(GPIO_BB_PD_ADDR + (0x4U * 16U)) = 1U;
    } else {
        MEM32(GPIO_BB_PB1_ADDR + (0x4U * 20U)) = 1U;
    }
}

static void SET_SDA_LOW(rn_chipid_t chipid)
{
    if ((chipid == CHIP_RA9701A) || (chipid == CHIP_RA9702B)) {
        MEM32(GPIO_BB_PD_ADDR + (0x4U * 17U)) = 0U;
    } else if (((chipid == CHIP_RA9701B) || (chipid == CHIP_RA9702A))) {
        MEM32(GPIO_BB_PD_ADDR + (0x4U * 16U)) = 0U;
    } else {
        MEM32(GPIO_BB_PB1_ADDR + (0x4U * 20U)) = 0U;
    }
}

static void SET_SDA(rn_chipid_t chipid, unsigned char l)
{
    if ((chipid == CHIP_RA9701A) || (chipid == CHIP_RA9702B)) {
        MEM32(GPIO_BB_PD_ADDR + (0x4U * 17U)) = l;
    } else if (((chipid == CHIP_RA9701B) || (chipid == CHIP_RA9702A))) {
        MEM32(GPIO_BB_PD_ADDR + (0x4U * 16U)) = l;
    } else {
        MEM32(GPIO_BB_PB1_ADDR + (0x4U * 20U)) = l;
    }
}

static unsigned char GET_SDA(rn_chipid_t chipid)
{
    unsigned char i;
    if ((chipid == CHIP_RA9701A) || (chipid == CHIP_RA9702B)) {
        i = (unsigned char)MEM32(GPIO_BB_PD_ADDR + (0x4U * 17U));
    } else if ((chipid == CHIP_RA9701B) || (chipid == CHIP_RA9702A)) {
        i = (unsigned char)MEM32(GPIO_BB_PD_ADDR + (0x4U * 16U));
    } else {
        i = (unsigned char)MEM32(GPIO_BB_PB1_ADDR + (0x4U * 20U));
    }

    return i & 0x1U;
}

static void SET_SCL_HIGH(rn_chipid_t chipid)
{
    if ((chipid == CHIP_RA9701A) || (chipid == CHIP_RA9702A)) {
        MEM32(GPIO_BB_PD_ADDR + (0x4U * 18U)) = 1U;
    } else if (chipid == CHIP_RA9701B) {
        MEM32(GPIO_BB_PD_ADDR + (0x4U * 17U)) = 1U;
    } else if (chipid == CHIP_RA9702B) {
        MEM32(GPIO_BB_PD_ADDR + (0x4U * 23U)) = 1U;
    } else {
        MEM32(GPIO_BB_PB1_ADDR + (0x4U * 21U)) = 1U;
    }
}

static void SET_SCL_LOW(rn_chipid_t chipid)
{
    if ((chipid == CHIP_RA9701A) || (chipid == CHIP_RA9702A)) {
        MEM32(GPIO_BB_PD_ADDR + (0x4U * 18U)) = 0U;
    } else if (chipid == CHIP_RA9701B) {
        MEM32(GPIO_BB_PD_ADDR + (0x4U * 17U)) = 0U;
    } else if (chipid == CHIP_RA9702B) {
        MEM32(GPIO_BB_PD_ADDR + (0x4U * 23U)) = 0U;
    } else {
        MEM32(GPIO_BB_PB1_ADDR + (0x4U * 21U)) = 0U;
    }
}

#elif defined(RN831x_RN861x_MCU_V2)
/*
#define  SET_SDA_OUTPUT() (MEM32(GPIO_BB_PMD_ADDR + 0x4U * 17U) = 0U)
#define  SET_SDA_INPUT()  (MEM32(GPIO_BB_PMD_ADDR + 0x4U * 17U) = 1U)
#define  SET_SDA_HIGH()   (MEM32(GPIO_BB_PD_ADDR + 0x4U * 17U) = 1U)
#define  SET_SDA_LOW()    (MEM32(GPIO_BB_PD_ADDR + 0x4U * 17U) = 0U)
#define  SET_SDA(l)       (MEM32(GPIO_BB_PD_ADDR + 0x4U * 17U) = (l))
#define  GET_SDA()        (MEM32(GPIO_BB_PD_ADDR + 0x4U * 17U))

#define  SET_SCL_HIGH()   (MEM32(GPIO_BB_PD_ADDR + 0x4U * 18U) = 1U)
#define  SET_SCL_LOW()    (MEM32(GPIO_BB_PD_ADDR + 0x4U * 18U) = 0U)
*/
#define  SET_SDA_OUTPUT() (MEM32(GPIO_PMD_ADDR) &= (~((unsigned int)1U << 17)))
#define  SET_SDA_INPUT()  (MEM32(GPIO_PMD_ADDR) |= ((unsigned int)1U << 17))
#define  SET_SDA_HIGH()   (MEM32(GPIO_PD_ADDR) |= ((unsigned int)1U << 17))
#define  SET_SDA_LOW()    (MEM32(GPIO_PD_ADDR) &= (~((unsigned int)1U << 17)))
#define  SET_SDA(l)       (MEM32(GPIO_PD_ADDR) = (MEM32(GPIO_PD_ADDR) & (~((unsigned int)1U << 17))) | ((unsigned int)(l) << 17))
#define  GET_SDA()        ((MEM32(GPIO_PD_ADDR) >> 17U) & 0x1U)

#define  SET_SCL_HIGH()   (MEM32(GPIO_PD_ADDR) |= ((unsigned int)1U << 18))
#define  SET_SCL_LOW()    (MEM32(GPIO_PD_ADDR) &= (~((unsigned int)1U << 18)))
#elif defined(RN831x_RN861x_MCU_V3)
/*
#define  SET_SDA_OUTPUT() (MEM32(GPIO_BB_PMD_ADDR + 0x4U * 16U) = 0U)
#define  SET_SDA_INPUT()  (MEM32(GPIO_BB_PMD_ADDR + 0x4U * 16U) = 1U)
#define  SET_SDA_HIGH()   (MEM32(GPIO_BB_PD_ADDR + 0x4U * 16U) = 1U)
#define  SET_SDA_LOW()    (MEM32(GPIO_BB_PD_ADDR + 0x4U * 16U) = 0U)
#define  SET_SDA(l)       (MEM32(GPIO_BB_PD_ADDR + 0x4U * 16U) = (l))
#define  GET_SDA()        (MEM32(GPIO_BB_PD_ADDR + 0x4U * 16U))

#define  SET_SCL_HIGH()   (MEM32(GPIO_BB_PD_ADDR + 0x4U * 17U) = 1U)
#define  SET_SCL_LOW()    (MEM32(GPIO_BB_PD_ADDR + 0x4U * 17U) = 0U)
*/
#define  SET_SDA_OUTPUT() (MEM32(GPIO_PMD_ADDR) &= (~((unsigned int)1U << 16)))
#define  SET_SDA_INPUT()  (MEM32(GPIO_PMD_ADDR) |= ((unsigned int)1U << 16))
#define  SET_SDA_HIGH()   (MEM32(GPIO_PD_ADDR) |= ((unsigned int)1U << 16))
#define  SET_SDA_LOW()    (MEM32(GPIO_PD_ADDR) &= (~((unsigned int)1U << 16)))
#define  SET_SDA(l)       (MEM32(GPIO_PD_ADDR) = (MEM32(GPIO_PD_ADDR) & (~((unsigned int)1U << 16))) | ((unsigned int)(l) << 16))
#define  GET_SDA()        ((MEM32(GPIO_PD_ADDR) >> 16U) & 0x1U)

#define  SET_SCL_HIGH()   (MEM32(GPIO_PD_ADDR) |= ((unsigned int)1U << 17))
#define  SET_SCL_LOW()    (MEM32(GPIO_PD_ADDR) &= (~((unsigned int)1U << 17)))
#elif defined(RN821x_RN721x_SOC_V2)
/*
#define  SET_SDA_OUTPUT() (MEM32(GPIO_BB_PMD_ADDR + 0x4U * 16U) = 0U)
#define  SET_SDA_INPUT()  (MEM32(GPIO_BB_PMD_ADDR + 0x4U * 16U) = 1U)
#define  SET_SDA_HIGH()   (MEM32(GPIO_BB_PD_ADDR + 0x4U * 16U) = 1U)
#define  SET_SDA_LOW()    (MEM32(GPIO_BB_PD_ADDR + 0x4U * 16U) = 0U)
#define  SET_SDA(l)       (MEM32(GPIO_BB_PD_ADDR + 0x4U * 16U) = (l))
#define  GET_SDA()        (MEM32(GPIO_BB_PD_ADDR + 0x4U * 16U))

#define  SET_SCL_HIGH()   (MEM32(GPIO_BB_PD_ADDR + 0x4U * 18U) = 1U)
#define  SET_SCL_LOW()    (MEM32(GPIO_BB_PD_ADDR + 0x4U * 18U) = 0U)
*/
#define  SET_SDA_OUTPUT() (MEM32(GPIO_PMD_ADDR) &= (~((unsigned int)1U << 16)))
#define  SET_SDA_INPUT()  (MEM32(GPIO_PMD_ADDR) |= ((unsigned int)1U << 16))
#define  SET_SDA_HIGH()   (MEM32(GPIO_PD_ADDR) |= ((unsigned int)1U << 16))
#define  SET_SDA_LOW()    (MEM32(GPIO_PD_ADDR) &= (~((unsigned int)1U << 16)))
#define  SET_SDA(l)       (MEM32(GPIO_PD_ADDR) = (MEM32(GPIO_PD_ADDR) & (~((unsigned int)1U << 16))) | ((unsigned int)(l) << 16))
#define  GET_SDA()        ((MEM32(GPIO_PD_ADDR) >> 16) & 0x1U)

#define  SET_SCL_HIGH()   (MEM32(GPIO_PD_ADDR) |= ((unsigned int)1U << 18))
#define  SET_SCL_LOW()    (MEM32(GPIO_PD_ADDR) &= (~((unsigned int)1U << 18)))
#elif defined(RN821x_RN721x_SOC_V3)
/*
#define  SET_SDA_OUTPUT() (MEM32(GPIO_BB_PMD_ADDR + 0x4U * 17U) = 0U)
#define  SET_SDA_INPUT()  (MEM32(GPIO_BB_PMD_ADDR + 0x4U * 17U) = 1U)
#define  SET_SDA_HIGH()   (MEM32(GPIO_BB_PD_ADDR + 0x4U * 17U) = 1U)
#define  SET_SDA_LOW()    (MEM32(GPIO_BB_PD_ADDR + 0x4U * 17U) = 0U)
#define  SET_SDA(l)       (MEM32(GPIO_BB_PD_ADDR + 0x4U * 17U) = (l))
#define  GET_SDA()        (MEM32(GPIO_BB_PD_ADDR + 0x4U * 17U))

#define  SET_SCL_HIGH()   (MEM32(GPIO_BB_PD_ADDR + 0x4U * 23U) = 1U)
#define  SET_SCL_LOW()    (MEM32(GPIO_BB_PD_ADDR + 0x4U * 23U) = 0U)
*/
#define  SET_SDA_OUTPUT() (MEM32(GPIO_PMD_ADDR) &= (~((unsigned int)1U << 17)))
#define  SET_SDA_INPUT()  (MEM32(GPIO_PMD_ADDR) |= ((unsigned int)1U << 17))
#define  SET_SDA_HIGH()   (MEM32(GPIO_PD_ADDR) |= ((unsigned int)1U << 17))
#define  SET_SDA_LOW()    (MEM32(GPIO_PD_ADDR) &= (~((unsigned int)1U << 17)))
#define  SET_SDA(l)       (MEM32(GPIO_PD_ADDR) = (MEM32(GPIO_PD_ADDR) & (~((unsigned int)1U << 17))) | ((unsigned int)(l) << 17))
#define  GET_SDA()        ((MEM32(GPIO_PD_ADDR) >> 17) & 0x1U)

#define  SET_SCL_HIGH()   (MEM32(GPIO_PD_ADDR) |= ((unsigned int)1U << 23))
#define  SET_SCL_LOW()    (MEM32(GPIO_PD_ADDR) &= (~((unsigned int)1U << 23)))
#elif defined(RN202x_RN7326_SOC_V2)
/*
#define  SET_SDA_OUTPUT() (MEM32(GPIO_BB_PMB1_ADDR + 0x4U * 20U) = 0U)
#define  SET_SDA_INPUT()  (MEM32(GPIO_BB_PMB1_ADDR + 0x4U * 20U) = 1U)
#define  SET_SDA_HIGH()   (MEM32(GPIO_BB_PB1_ADDR + 0x4U * 20U) = 1U)
#define  SET_SDA_LOW()    (MEM32(GPIO_BB_PB1_ADDR + 0x4U * 20U) = 0U)
#define  SET_SDA(l)       (MEM32(GPIO_BB_PB1_ADDR + 0x4U * 20U) = (l))
#define  GET_SDA()        (MEM32(GPIO_BB_PB1_ADDR + 0x4U * 20U))

#define  SET_SCL_HIGH()   (MEM32(GPIO_BB_PB1_ADDR + 0x4U * 21U) = 1U)
#define  SET_SCL_LOW()    (MEM32(GPIO_BB_PB1_ADDR + 0x4U * 21U) = 0U)
*/
#define  SET_SDA_OUTPUT() (MEM32(GPIO_PMB1_ADDR) &= (~((unsigned int)1U << 20)))
#define  SET_SDA_INPUT()  (MEM32(GPIO_PMB1_ADDR) |= ((unsigned int)1U << 20))
#define  SET_SDA_HIGH()   (MEM32(GPIO_PB1_ADDR) |= ((unsigned int)1U << 20))
#define  SET_SDA_LOW()    (MEM32(GPIO_PB1_ADDR) &= (~((unsigned int)1U << 20)))
#define  SET_SDA(l)       (MEM32(GPIO_PB1_ADDR) = (MEM32(GPIO_PB1_ADDR) & (~((unsigned int)1U << 20))) | ((unsigned int)(l) << 20))
#define  GET_SDA()        ((MEM32(GPIO_PB1_ADDR) >> 20) & 0x1U)

#define  SET_SCL_HIGH()   (MEM32(GPIO_PB1_ADDR) |= ((unsigned int)1U << 21))
#define  SET_SCL_LOW()    (MEM32(GPIO_PB1_ADDR) &= (~((unsigned int)1U << 21)))
#endif

#if defined(RN_CM0_PLATFORM)
static void  i2c_send_start(rn_chipid_t chipid)
#else
static void  i2c_send_start(void)
#endif
{
#if defined(RN_CM0_PLATFORM)
    SET_SDA_HIGH(chipid);
    SET_SDA_OUTPUT(chipid);
    SET_SCL_HIGH(chipid);
    SET_SDA_LOW(chipid);
#else
    SET_SDA_HIGH();
    SET_SDA_OUTPUT();
    SET_SCL_HIGH();
    SET_SDA_LOW();
#endif
}

#if defined(RN_CM0_PLATFORM)
static void  i2c_send_stop(rn_chipid_t chipid)
#else
static void  i2c_send_stop(void)
#endif
{
#if defined(RN_CM0_PLATFORM)
    SET_SDA_OUTPUT(chipid);
    SET_SDA_LOW(chipid);
    SET_SCL_HIGH(chipid);
    SET_SDA_HIGH(chipid);
#else
    SET_SDA_OUTPUT();
    SET_SDA_LOW();
    SET_SCL_HIGH();
    SET_SDA_HIGH();
#endif
}
/**
 * @brief send 1 byte as i2c protocol
 *
 * @param data send data
 * @return unsigned char ack(0=ack, 1=nak)
 */
#if defined(RN_CM0_PLATFORM)
static unsigned char i2c_send_byte(rn_chipid_t chipid, unsigned char data)
#else
static unsigned char i2c_send_byte(unsigned char data)
#endif
{
    unsigned char i;

#if defined(RN_CM0_PLATFORM)
    SET_SCL_LOW(chipid);
    SET_SDA_OUTPUT(chipid);
    for (i = 0U; i < 8U; i++) {
        SET_SCL_LOW(chipid);
        SET_SDA(chipid, ((data >> (7U-i)) & 0x1U));
        SET_SCL_HIGH(chipid);
    }
    /* receive ack */
    SET_SCL_LOW(chipid);
    SET_SDA_INPUT(chipid);
    SET_SCL_HIGH(chipid);
    i = GET_SDA(chipid);
    SET_SCL_LOW(chipid);
#else
    SET_SCL_LOW();
    SET_SDA_OUTPUT();
    for (i = 0U; i < 8U; i++) {
        SET_SCL_LOW();
        SET_SDA((unsigned char)((data >> (7U-i)) & 0x1U));
        SET_SCL_HIGH();
    }
    /* receive ack */
    SET_SCL_LOW();
    SET_SDA_INPUT();
    SET_SCL_HIGH();
    i = (unsigned char)(GET_SDA());
    SET_SCL_LOW();
#endif
    return i;
}

#if defined(RN_CM0_PLATFORM)
static unsigned char i2c_recv_byte(rn_chipid_t chipid, unsigned char ack)
#else
static unsigned char i2c_recv_byte(unsigned char ack)
#endif
{
    unsigned int i, res = 0U;

#if defined(RN_CM0_PLATFORM)
    SET_SCL_LOW(chipid);
    SET_SDA_INPUT(chipid);
    for (i = 0U; i < 8U; i++) {
        SET_SCL_LOW(chipid);
        res <<= 1U;
        SET_SCL_HIGH(chipid);
        res |= GET_SDA(chipid);
    }
    /* end with ack or no ack */
    SET_SCL_LOW(chipid);
    SET_SDA(chipid, ack);
    SET_SDA_OUTPUT(chipid);
    SET_SCL_HIGH(chipid);
    SET_SCL_LOW(chipid);
    SET_SDA_INPUT(chipid);
#else
    SET_SCL_LOW();
    SET_SDA_INPUT();
    for (i = 0U; i < 8U; i++) {
        SET_SCL_LOW();
        res <<= 1U;
        SET_SCL_HIGH();
        res |= (unsigned char)(GET_SDA());
    }
    /* end with ack or no ack */
    SET_SCL_LOW();
    SET_SDA(ack);
    SET_SDA_OUTPUT();
    SET_SCL_HIGH();
    SET_SCL_LOW();
    SET_SDA_INPUT();
#endif
    return (unsigned char)res;
}

typedef enum {
    I2C_WR  = 0,
    I2C_RD  = 1
} ee_rw_type;

#if defined(RN_CM0_PLATFORM)
static SIPEEPROM_RET_TYPEDEF i2c_write_addr(rn_chipid_t chipid, ee_rw_type rw, unsigned int addr)
#else
static SIPEEPROM_RET_TYPEDEF i2c_write_addr(ee_rw_type rw, unsigned int addr)
#endif
{
    SIPEEPROM_RET_TYPEDEF res;
    unsigned char retry = 0U;
    unsigned char tmp_ret[3];

    do {
        res = I2C_RET_FAIL;
#if defined(RN_CM0_PLATFORM)
        i2c_send_start(chipid);
        tmp_ret[0] = i2c_send_byte(chipid, (unsigned char)(DEV_ADDR | (unsigned char)I2C_WR));
        tmp_ret[1] = i2c_send_byte(chipid, (unsigned char)((addr >> 8U) & 0xffU));
        tmp_ret[2] = i2c_send_byte(chipid, (unsigned char)(addr & 0xffU));
#else
        i2c_send_start();
        tmp_ret[0] = i2c_send_byte((unsigned char)(DEV_ADDR | (unsigned char)I2C_WR));
        tmp_ret[1] = i2c_send_byte((unsigned char)((addr >> 8U) & 0xffU));
        tmp_ret[2] = i2c_send_byte((unsigned char)(addr & 0xffU));
#endif
        if ((tmp_ret[0] == 0U) &&
            (tmp_ret[1] == 0U) &&
            (tmp_ret[2] == 0U)) {
            if (rw == I2C_RD) {
#if defined(RN_CM0_PLATFORM)
                i2c_send_start(chipid);
                if (0U == i2c_send_byte(chipid, (unsigned char)(DEV_ADDR | (unsigned char)I2C_RD)))
#else
                i2c_send_start();
                if (0U == i2c_send_byte((unsigned char)(DEV_ADDR | (unsigned char)I2C_RD)))
#endif
                {
                    res = I2C_RET_PASS;
                }
            } else {
                res = I2C_RET_PASS;
            }
        }
        if (res == I2C_RET_PASS) {
            break;
        }
#if defined(RN_CM0_PLATFORM)
        i2c_send_stop(chipid);
#else
        i2c_send_stop();
#endif
        retry++;
    } while(retry < WR_RETRY_NUM);

    return res;
}

/**
 * @brief i2c wirte data in one page
 *   注意：这是内部函数，可以确保传参满足在一页内，无需进行参数判断
 * @param mem_id 主存储区域(0)或id页(1)
 * @param addr 地址
 * @param data 数据
 * @param len 字节长度
 * @return unsigned char 0=pass, 1=fail
 */
#if defined(RN_CM0_PLATFORM)
static unsigned char i2c_write_page(rn_chipid_t chipid, unsigned int addr, const unsigned char *data, unsigned int len)
#else
static unsigned char i2c_write_page(unsigned int addr, const unsigned char *data, unsigned int len)
#endif
{
    unsigned int i;
    unsigned char res, retry = 0U;

    do {
        res = 1U;
#if defined(RN_CM0_PLATFORM)
        if (I2C_RET_PASS == i2c_write_addr(chipid, I2C_WR, addr))
#else
        if (I2C_RET_PASS == i2c_write_addr(I2C_WR, addr))
#endif
        {
            for (i = 0U; i < len; i++) {
#if defined(RN_CM0_PLATFORM)
                res = i2c_send_byte(chipid, data[i]);
#else
                res = i2c_send_byte(data[i]);
#endif
                if (1U == res) {
                    break;
                }
            }
        }
#if defined(RN_CM0_PLATFORM)
        i2c_send_stop(chipid);
#else
        i2c_send_stop();
#endif
        retry++;
    } while((res == 1U) && (retry < WR_RETRY_NUM));

    return res;
}

/**
 * @brief i2c read data in one page
 *   注意：这是内部函数，可以确保传参满足在一页内，无需进行参数判断
 * @param mem_id 主存储区域(0)或id页(1)
 * @param addr 地址
 * @param data 数据
 * @param len 字节长度
 * @return unsigned char PASS or FAIL
 */
#if defined(RN_CM0_PLATFORM)
static SIPEEPROM_RET_TYPEDEF i2c_read_page(rn_chipid_t chipid, unsigned int addr, unsigned char *data, unsigned int len)
#else
static SIPEEPROM_RET_TYPEDEF i2c_read_page(unsigned int addr, unsigned char *data, unsigned int len)
#endif
{
    unsigned int i;
    unsigned char retry = 0U;

    do {
#if defined(RN_CM0_PLATFORM)
        if (I2C_RET_PASS == i2c_write_addr(chipid, I2C_RD, addr)) {
            for (i = 0U; i < len; i++) {
                data[i] = i2c_recv_byte(chipid, (i == (len - 1U)) ? 1U : 0U);
            }
            i2c_send_stop(chipid);
            break;
        }
        i2c_send_stop(chipid);
#else
        if (I2C_RET_PASS == i2c_write_addr(I2C_RD, addr)) {
            for (i = 0U; i < len; i++) {
                data[i] = i2c_recv_byte((i == (len - 1U)) ? 1U : 0U);
            }
            i2c_send_stop();
            break;
        }
        i2c_send_stop();
#endif
        retry++;
    } while(retry < WR_RETRY_NUM);

    return (retry == WR_RETRY_NUM) ? I2C_RET_FAIL : I2C_RET_PASS;
}

void LL_SIPEEPROM_Init(void)
{
    unsigned int sysps_bak;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif

#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2) ||\
    defined(RN831x_RN861x_MCU_V3) ||\
    defined(RN821x_RN721x_SOC_V2) ||\
    defined(RN821x_RN721x_SOC_V3) ||\
    defined(RN202x_RN7326_SOC_V2)
#if defined(RN_CM0_PLATFORM)
    if (chipid >= CHIP_RA9701A)
#endif
    {
#if defined(RN_CM0_PLATFORM)
        sysps_bak = MEM32(SYS_PS_ADDR(chipid));
        MEM32(SYS_PS_ADDR(chipid)) = 0x82U;
        MEM32(MOD1_EN_ADDR(chipid)) |= ((unsigned int)1U << 5);
#elif defined(RN202x_RN7326_SOC_V2)
        sysps_bak = MEM32(SYS_PS_9303_ADDR);
        MEM32(SYS_PS_9303_ADDR) = 0x82U;
        MEM32(MOD1_EN_9303_ADDR) |= ((unsigned int)1U << 5);
        MEM32(GPIO_PSW0_ADDR) = 0x82U;
#else
        sysps_bak = MEM32(SYS_PS_9103_ADDR);
        MEM32(SYS_PS_9103_ADDR) = 0x82U;
        MEM32(MOD1_EN_9103_ADDR) |= ((unsigned int)1U << 5);
#endif

#if defined(RN_CM0_PLATFORM)
        if (chipid == CHIP_RA9701A)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V2)
        {
            MEM32(GPIO_PCD_ADDR) &= (~((unsigned int)0xfU << 18));   /* P141(SDA),P142(SCL) as GPIO */
            MEM32(GPIO_PUD_ADDR) &= (~((unsigned int)0x1U << 17));   /* pull-up resistor */
            MEM32(GPIO_PUD_ADDR) |= ((unsigned int)0x1U << 18);
            MEM32(GPIO_PIED_ADDR) &= (~((unsigned int)1U << 17));    /* sda input enable */
            MEM32(GPIO_PIMD_ADDR) |= ((unsigned int)1U << 1);        /* sda OD mode */
            MEM32(GPIO_BB_PD_ADDR + (0x4U * 18U)) = 1U;  /* scl high */
            MEM32(GPIO_PMD_ADDR) &= (~((unsigned int)1U << 18U));    /* scl output mode */
            MEM32(GPIO_BB_PMD_ADDR + (0x4U * 17U)) = 1U; /* sda input mode */
        }
#endif
#if defined(RN_CM0_PLATFORM)
        else if (chipid == CHIP_RA9701B)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN831x_RN861x_MCU_V3)
        {
            MEM32(GPIO_PCD_ADDR) &= (~((unsigned int)0x3fU << 16));   /* P140(SDA),P141(SCL),P142(PWR) */
            MEM32(GPIO_PUD_ADDR) &= (~((unsigned int)0x1U << 16));   /* pull-up resistor */
            MEM32(GPIO_PUD_ADDR) |= ((unsigned int)0x1U << 17);
            MEM32(GPIO_PIED_ADDR) &= (~((unsigned int)1U << 16));    /* sda input enable */
            MEM32(GPIO_PIMD_ADDR) |= 1U;               /* sda OD mode */
            MEM32(GPIO_BB_PD_ADDR + (0x4U * 18U)) = 1U;   /* pwr high */
            MEM32(GPIO_BB_PD_ADDR + (0x4U * 17U)) = 1U;   /* scl high */
            MEM32(GPIO_PMD_ADDR) &= (~((unsigned int)3U << 17));     /* scl/pwr output mode */
            MEM32(GPIO_BB_PMD_ADDR + (0x4U * 16U)) = 1U; /* sda input mode */
        }
#endif
#if defined(RN_CM0_PLATFORM)
        else if (chipid == CHIP_RA9702A)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_V2)
        {
            MEM32(GPIO_PCD2_ADDR) &= (~(0xfU | ((unsigned int)0xfU << 8))); /* P140(SDA),P142(SCL) */
            MEM32(GPIO_PUD_ADDR) &= (~((unsigned int)0x1U << 16));          /* pull-up resistor */
            MEM32(GPIO_PUD_ADDR) |= ((unsigned int)0x1U << 18);
            MEM32(GPIO_PIED_ADDR) &= (~((unsigned int)1U << 16));           /* sda input enable */
            MEM32(GPIO_PIMD_ADDR) |= 1U;                      /* sda OD mode */
            MEM32(GPIO_BB_PD_ADDR + (0x4U * 18U)) = 1U;         /* scl high */
            MEM32(GPIO_PMD_ADDR) &= (~((unsigned int)1U << 18));            /* scl output mode */
            MEM32(GPIO_BB_PMD_ADDR + (0x4U * 16U)) = 1U;        /* sda input mode */
        }
#endif
#if defined(RN_CM0_PLATFORM)
        else if (chipid == CHIP_RA9702B)
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN821x_RN721x_SOC_V3)
        {
            MEM32(GPIO_PCD2_ADDR) &= (~(((unsigned int)0xfU << 4) | ((unsigned int)0xfU << 28))); /* P141(SDA),P147(SCL) */
            MEM32(GPIO_PUD_ADDR) &= (~((unsigned int)0x1U << 17));                  /* pull-up resistor */
            MEM32(GPIO_PUD_ADDR) |= ((unsigned int)0x1U << 23);
            MEM32(GPIO_PIED_ADDR) &= (~((unsigned int)1U << 17));                   /* sda input enable */
            MEM32(GPIO_PIMD_ADDR) |= ((unsigned int)1U << 1);                       /* sda OD mode */
            MEM32(GPIO_BB_PD_ADDR + (0x4U * 23U)) = 1U;                 /* scl high */
            MEM32(GPIO_PMD_ADDR) &= (~((unsigned int)1U << 23));                    /* scl output mode */
            MEM32(GPIO_BB_PMD_ADDR + (0x4U * 17U)) = 1U;                /* sda input mode */
        }
#endif
#if defined(RN_CM0_PLATFORM)
        else/* if (chipid == CHIP_RA9703A)*/
#endif
#if defined(RN_CM0_PLATFORM) ||\
    defined(RN202x_RN7326_SOC_V2)
        {
            MEM32(GPIO_PCB1_ADDR) &= (~((unsigned int)0x3fU << 12)); /* P74(SDA),P75(SCL) */
            MEM32(GPIO_PUB1_ADDR) &= (~((unsigned int)1U << 20));    /* pull-up resistor */
            MEM32(GPIO_PUB1_ADDR) |= ((unsigned int)1U << 21);
            MEM32(GPIO_PIEB1_ADDR) &= (~((unsigned int)1U << 20));   /* sda input enable */
            MEM32(GPIO_PIMB2_ADDR) |= ((unsigned int)1U << 4);       /* sda OD mode */
            MEM32(GPIO_BB_PB1_ADDR + (0x4U * 21U)) = 1U; /* scl high */
            MEM32(GPIO_BB_PMB1_ADDR + (0x4U * 21U)) = 0U;/* scl output mode */
            MEM32(GPIO_BB_PMB1_ADDR + (0x4U * 20U)) = 1U;/* sda input mode */
        }
#endif
#if defined(RN_CM0_PLATFORM)
        MEM32(SYS_PS_ADDR(chipid)) = sysps_bak;
#elif defined(RN202x_RN7326_SOC_V2)
        MEM32(SYS_PS_9303_ADDR) = sysps_bak;
#else
        MEM32(SYS_PS_9103_ADDR) = sysps_bak;
#endif
    }
#endif
}

void LL_SIPEEPROM_Rst(void)
{
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
#endif
#if defined(RN_CM0_PLATFORM)
    i2c_send_start(chipid);
    (void)i2c_send_byte(chipid, 0xffU);
#else
    i2c_send_start();
    (void)i2c_send_byte(0xffU);
#endif
}

SIPEEPROM_RET_TYPEDEF LL_SIPEEPROM_Read(unsigned int addr, unsigned char *data, unsigned len)
{
    unsigned int i, pages, head, tail, dst_addr = (unsigned int)data;
    SIPEEPROM_RET_TYPEDEF res = I2C_RET_FAIL;
    unsigned int ee_chip_size;
    unsigned int ee_page_size;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
    if ((chipid == CHIP_RA9701A) || (chipid == CHIP_RA9702A) || (chipid == CHIP_RA9703A)) {
        ee_chip_size = 64U * 1024U;
        ee_page_size = 128U;
    } else {
        ee_chip_size = 32U * 1024U;
        ee_page_size = 64U;
    }
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_V2)
    ee_chip_size = 64U * 1024U;
    ee_page_size = 128U;
#else
    ee_chip_size = 32U * 1024U;
    ee_page_size = 64U;
#endif

    if ((len != 0U) && (addr < ee_chip_size) && ((addr + len) < (ee_chip_size+ 1U))) {
        res = I2C_RET_PASS;
        head = addr % ee_page_size;
        pages = 1U;
        if (len < (ee_page_size - head)) {
        } else {
            i = len - (ee_page_size - head);
            pages += (i / ee_page_size);
            tail = i % ee_page_size;
            if (tail != 0x0U) {
                pages++;
            }
        }
        for (i = 0U; i < pages; i++) {
            if (i == 0U) {
                /* head process */
                head = (pages == 1U) ? len : (ee_page_size - head);
#if defined(RN_CM0_PLATFORM)
                res = i2c_read_page(chipid, addr, (unsigned char *)dst_addr, head);
#else
                res = i2c_read_page(addr, (unsigned char *)dst_addr, head);
#endif
                len -= head;
                addr += head;
                dst_addr += head;
            } else if (i == (pages - 1U)) {
                /* tail process */
#if defined(RN_CM0_PLATFORM)
                res = i2c_read_page(chipid, addr, (unsigned char *)dst_addr, len);
#else
                res = i2c_read_page(addr, (unsigned char *)dst_addr, len);
#endif
            } else {
#if defined(RN_CM0_PLATFORM)
                res = i2c_read_page(chipid, addr, (unsigned char *)dst_addr, ee_page_size);
#else
                res = i2c_read_page(addr, (unsigned char *)dst_addr, ee_page_size);
#endif
                len -= ee_page_size;
                addr += ee_page_size;
                dst_addr += ee_page_size;
            }
            if (res == I2C_RET_FAIL) {
                break;
            }
        }
    }

    return res;
}

SIPEEPROM_RET_TYPEDEF LL_SIPEEPROM_Write(unsigned int addr, const unsigned char *data, unsigned int len)
{
    unsigned int i, pages, head, tail, src_addr = (unsigned int)data;
    SIPEEPROM_RET_TYPEDEF res = I2C_RET_FAIL;
    unsigned int ee_chip_size;
    unsigned int ee_page_size;
#if defined(RN_CM0_PLATFORM)
    const rn_chipid_t chipid = getChipid();
    if ((chipid == CHIP_RA9701A) || (chipid == CHIP_RA9702A) || (chipid == CHIP_RA9703A)) {
        ee_chip_size = 64U * 1024U;
        ee_page_size = 128U;
    } else {
        ee_chip_size = 32U * 1024U;
        ee_page_size = 64U;
    }
#elif defined(RN831x_RN861x_MCU_V2) ||\
      defined(RN821x_RN721x_SOC_V2) ||\
      defined(RN202x_RN7326_SOC_V2)
    ee_chip_size = 64U * 1024U;
    ee_page_size = 128U;
#else
    ee_chip_size = 32U * 1024U;
    ee_page_size = 64U;
#endif

    if ((len != 0U) && (addr < ee_chip_size) && ((addr + len) < (ee_chip_size+ 1U))) {
        head = addr % ee_page_size;
        pages = 1U;
        if (len < (ee_page_size - head)) {
        } else {
            i = len - (ee_page_size - head);
            pages += (i / ee_page_size);
            tail = i % ee_page_size;
            if (tail != 0U) {
                pages++;
            }
        }
        for (i = 0U; i < pages; i++) {
            if (i == 0U) {
                /* head process */
                head = (pages == 1U) ? len : (ee_page_size - head);
#if defined(RN_CM0_PLATFORM)
                if (0U == i2c_write_page(chipid, addr, (unsigned char *)src_addr, head)) {
                    res = I2C_RET_PASS;
                } else {
                    res = I2C_RET_FAIL;
                }
#else
                if (0U == i2c_write_page(addr, (unsigned char *)src_addr, head)) {
                    res = I2C_RET_PASS;
                } else {
                    res = I2C_RET_FAIL;
                }
#endif
                len -= head;
                addr += head;
                src_addr += head;
            } else if (i == (pages - 1U)) {
                /* tail process */
#if defined(RN_CM0_PLATFORM)
                if (0U == i2c_write_page(chipid, addr, (unsigned char *)src_addr, len)) {
                    res = I2C_RET_PASS;
                } else {
                    res = I2C_RET_FAIL;
                }
#else
                if (0U == i2c_write_page(addr, (unsigned char *)src_addr, len)) {
                    res = I2C_RET_PASS;
                } else {
                    res = I2C_RET_FAIL;
                }
#endif
            } else {
#if defined(RN_CM0_PLATFORM)
                if (0U == i2c_write_page(chipid, addr, (unsigned char *)src_addr, ee_page_size)) {
                    res = I2C_RET_PASS;
                } else {
                    res = I2C_RET_FAIL;
                }
#else
                if (0U == i2c_write_page(addr, (unsigned char *)src_addr, ee_page_size)) {
                    res = I2C_RET_PASS;
                } else {
                    res = I2C_RET_FAIL;
                }
#endif
                len -= ee_page_size;
                addr += ee_page_size;
                src_addr += ee_page_size;
            }
            if (res == I2C_RET_FAIL) {
                break;
            }
        }
        if (i == pages) {
            res = I2C_RET_PASS;
        }
    }
    return res;
}
#endif
/* r2989 */
