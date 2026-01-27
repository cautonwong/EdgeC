/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_flash.h
 * @author  Renergy Software Team
 * @brief   FLASH module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_FLASH_H
#define _RN831x_RN861x_MCU_FLASH_H

#include "rn831x_rn861x_mcu.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/*                Device Specific Peripheral registers structures             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Peripherals rn831x_rn861x_mcu Peripherals
 * @brief rn831x_rn861x_mcu Device Specific Peripheral registers structures
 * @{
 */

/* FLASH */
typedef struct
{
    __IO uint32_t RDCTRL;                    /*!< offset: 0x000 info: Read Mode Control Register */
    __IO uint32_t MAINPEA;                   /*!< offset: 0x004 info: MAIN Address Config Register */
    __IO uint32_t SINGAL_PROGRAM;            /*!< offset: 0x008 info: Byte Program Register */
    __IO uint32_t STA;                       /*!< offset: 0x00C info: Status Register */
    __IO uint32_t RECNT1;                    /*!< offset: 0x010 info: FLASH Access Counter1 */
    __IO uint32_t RECNT2;                    /*!< offset: 0x014 info: FLASH Access Counter2 */
    __IO uint32_t PSW;                       /*!< offset: 0x018 info: Password Register */
    __IO uint32_t REMAP;                     /*!< offset: 0x01C info: Address Remap Register */
    __IO uint32_t MAINPRO1;                  /*!< offset: 0x020 info: Write Protection Register1 */
    __IO uint32_t MAINPRO2;                  /*!< offset: 0x024 info: Write Protection Register2 */
    __IO uint32_t MAINPRO3;                  /*!< offset: 0x028 info: Write Protection Register3 */
    __IO uint32_t MAINPRO4;                  /*!< offset: 0x02C info: Write Protection Register4 */
    __IO uint32_t MAINPRO5;                  /*!< offset: 0x030 info: Write Protection Register5 */
    __IO uint32_t MAINPRO6;                  /*!< offset: 0x034 info: Write Protection Register6 */
    __IO uint32_t MAINPRO7;                  /*!< offset: 0x038 info: Write Protection Register7 */
    __IO uint32_t MAINPRO8;                  /*!< offset: 0x03C info: Write Protection Register8 */
    __IO uint32_t RESERVED0[48];             /*!< offset: 0x040 ~ 0FC */
    __IO uint32_t PG_DATA0;                  /*!< offset: 0x100 info: Program Data Register0 */
    __IO uint32_t PG_DATA1;                  /*!< offset: 0x104 info: Program Data Register1 */
    __IO uint32_t PG_DATA2;                  /*!< offset: 0x108 info: Program Data Register2 */
    __IO uint32_t PG_DATA3;                  /*!< offset: 0x10C info: Program Data Register3 */
    __IO uint32_t PG_DATA4;                  /*!< offset: 0x110 info: Program Data Register4 */
    __IO uint32_t PG_DATA5;                  /*!< offset: 0x114 info: Program Data Register5 */
    __IO uint32_t PG_DATA6;                  /*!< offset: 0x118 info: Program Data Register6 */
    __IO uint32_t PG_DATA7;                  /*!< offset: 0x11C info: Program Data Register7 */
    __IO uint32_t PG_DATA8;                  /*!< offset: 0x120 info: Program Data Register8 */
    __IO uint32_t PG_DATA9;                  /*!< offset: 0x124 info: Program Data Register9 */
    __IO uint32_t PG_DATA10;                 /*!< offset: 0x128 info: Program Data Register10 */
    __IO uint32_t PG_DATA11;                 /*!< offset: 0x12C info: Program Data Register11 */
    __IO uint32_t PG_DATA12;                 /*!< offset: 0x130 info: Program Data Register12 */
    __IO uint32_t PG_DATA13;                 /*!< offset: 0x134 info: Program Data Register13 */
    __IO uint32_t PG_DATA14;                 /*!< offset: 0x138 info: Program Data Register14 */
    __IO uint32_t PG_DATA15;                 /*!< offset: 0x13C info: Program Data Register15 */
    __IO uint32_t PG_DATA16;                 /*!< offset: 0x140 info: Program Data Register16 */
    __IO uint32_t PG_DATA17;                 /*!< offset: 0x144 info: Program Data Register17 */
    __IO uint32_t PG_DATA18;                 /*!< offset: 0x148 info: Program Data Register18 */
    __IO uint32_t PG_DATA19;                 /*!< offset: 0x14C info: Program Data Register19 */
    __IO uint32_t PG_DATA20;                 /*!< offset: 0x150 info: Program Data Register20 */
    __IO uint32_t PG_DATA21;                 /*!< offset: 0x154 info: Program Data Register21 */
    __IO uint32_t PG_DATA22;                 /*!< offset: 0x158 info: Program Data Register22 */
    __IO uint32_t PG_DATA23;                 /*!< offset: 0x15C info: Program Data Register23 */
    __IO uint32_t PG_DATA24;                 /*!< offset: 0x160 info: Program Data Register24 */
    __IO uint32_t PG_DATA25;                 /*!< offset: 0x164 info: Program Data Register25 */
    __IO uint32_t PG_DATA26;                 /*!< offset: 0x168 info: Program Data Register26 */
    __IO uint32_t PG_DATA27;                 /*!< offset: 0x16C info: Program Data Register27 */
    __IO uint32_t PG_DATA28;                 /*!< offset: 0x170 info: Program Data Register28 */
    __IO uint32_t PG_DATA29;                 /*!< offset: 0x174 info: Program Data Register29 */
    __IO uint32_t PG_DATA30;                 /*!< offset: 0x178 info: Program Data Register30 */
    __IO uint32_t PG_DATA31;                 /*!< offset: 0x17C info: Program Data Register31 */
    __IO uint32_t PG_DATA32;                 /*!< offset: 0x180 info: Program Data Register32 */
    __IO uint32_t PG_DATA33;                 /*!< offset: 0x184 info: Program Data Register33 */
    __IO uint32_t PG_DATA34;                 /*!< offset: 0x188 info: Program Data Register34 */
    __IO uint32_t PG_DATA35;                 /*!< offset: 0x18C info: Program Data Register35 */
    __IO uint32_t PG_DATA36;                 /*!< offset: 0x190 info: Program Data Register36 */
    __IO uint32_t PG_DATA37;                 /*!< offset: 0x194 info: Program Data Register37 */
    __IO uint32_t PG_DATA38;                 /*!< offset: 0x198 info: Program Data Register38 */
    __IO uint32_t PG_DATA39;                 /*!< offset: 0x19C info: Program Data Register39 */
    __IO uint32_t PG_DATA40;                 /*!< offset: 0x1A0 info: Program Data Register40 */
    __IO uint32_t PG_DATA41;                 /*!< offset: 0x1A4 info: Program Data Register41 */
    __IO uint32_t PG_DATA42;                 /*!< offset: 0x1A8 info: Program Data Register42 */
    __IO uint32_t PG_DATA43;                 /*!< offset: 0x1AC info: Program Data Register43 */
    __IO uint32_t PG_DATA44;                 /*!< offset: 0x1B0 info: Program Data Register44 */
    __IO uint32_t PG_DATA45;                 /*!< offset: 0x1B4 info: Program Data Register45 */
    __IO uint32_t PG_DATA46;                 /*!< offset: 0x1B8 info: Program Data Register46 */
    __IO uint32_t PG_DATA47;                 /*!< offset: 0x1BC info: Program Data Register47 */
    __IO uint32_t PG_DATA48;                 /*!< offset: 0x1C0 info: Program Data Register48 */
    __IO uint32_t PG_DATA49;                 /*!< offset: 0x1C4 info: Program Data Register49 */
    __IO uint32_t PG_DATA50;                 /*!< offset: 0x1C8 info: Program Data Register50 */
    __IO uint32_t PG_DATA51;                 /*!< offset: 0x1CC info: Program Data Register51 */
    __IO uint32_t PG_DATA52;                 /*!< offset: 0x1D0 info: Program Data Register52 */
    __IO uint32_t PG_DATA53;                 /*!< offset: 0x1D4 info: Program Data Register53 */
    __IO uint32_t PG_DATA54;                 /*!< offset: 0x1D8 info: Program Data Register54 */
    __IO uint32_t PG_DATA55;                 /*!< offset: 0x1DC info: Program Data Register55 */
    __IO uint32_t PG_DATA56;                 /*!< offset: 0x1E0 info: Program Data Register56 */
    __IO uint32_t PG_DATA57;                 /*!< offset: 0x1E4 info: Program Data Register57 */
    __IO uint32_t PG_DATA58;                 /*!< offset: 0x1E8 info: Program Data Register58 */
    __IO uint32_t PG_DATA59;                 /*!< offset: 0x1EC info: Program Data Register59 */
    __IO uint32_t PG_DATA60;                 /*!< offset: 0x1F0 info: Program Data Register60 */
    __IO uint32_t PG_DATA61;                 /*!< offset: 0x1F4 info: Program Data Register61 */
    __IO uint32_t PG_DATA62;                 /*!< offset: 0x1F8 info: Program Data Register62 */
    __IO uint32_t PG_DATA63;                 /*!< offset: 0x1FC info: Program Data Register63 */
    __IO uint32_t OPRT_MODE;                 /*!< offset: 0x200 info: Operation Mode Register */
    __IO uint32_t CMD_CTRL;                  /*!< offset: 0x204 info: Command Control Register */
    __IO uint32_t HSMCMD;                    /*!< offset: 0x208 info: HSM Command Register */
    __IO uint32_t HOSTCMD;                   /*!< offset: 0x20C info: Host Command Register */
    __IO uint32_t CTRL;                      /*!< offset: 0x210 info: Control Register */
    __IO uint32_t NVRPEA;                    /*!< offset: 0x214 info: Address Config Register */
    __IO uint32_t PRIORITY;                  /*!< offset: 0x218 info: Priority Control Register */
    __IO uint32_t TCE;                       /*!< offset: 0x21C info: Chip Erase Timing Register */
    __IO uint32_t TSE;                       /*!< offset: 0x220 info: Sector Erase Timing Register */
    __IO uint32_t TP;                        /*!< offset: 0x224 info: Program Timing Register */
    __IO uint32_t TUS;                       /*!< offset: 0x228 info: Microsecond Timing Register */
    __IO uint32_t TW;                        /*!< offset: 0x22C info: Wait Time Register */
    __IO uint32_t READONLY;                  /*!< offset: 0x230 info: Read-Only Control Register */
    __IO uint32_t TRIM;                      /*!< offset: 0x234 info: Calibration Register */
    __IO uint32_t ALLOCATE1;                 /*!< offset: 0x238 info: Region Allocation Register */
    __IO uint32_t NVRRD1;                    /*!< offset: 0x23C info: NVR Read Permission Register1 */
    __IO uint32_t NVRRD2;                    /*!< offset: 0x240 info: NVR Read Permission Register2 */
    __IO uint32_t NVRRD3;                    /*!< offset: 0x244 info: NVR Read Permission Register3 */
    __IO uint32_t NVRWR1;                    /*!< offset: 0x248 info: NVR Write Permission Register1 */
    __IO uint32_t NVRWR2;                    /*!< offset: 0x24C info: NVR Write Permission Register2 */
    __IO uint32_t NVRWR3;                    /*!< offset: 0x250 info: NVR Write Permission Register3 */
    __IO uint32_t ALLOCATE1_LOCK;            /*!< offset: 0x254 info: Allocation Lock Register */
    __IO uint32_t CHIPERASE_PSW;             /*!< offset: 0x258 info: Lifecycle Management Register */
    __IO uint32_t LIFE_MANAGE;               /*!< offset: 0x25C info: Lifecycle Lock Register */
    __IO uint32_t LIFE_MANAGE_LOCK;          /*!< offset: 0x260 info: LIFE_MANAGE_LOCK Register */
} FLASH_TypeDef;

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_Peripherals */

/******************************************************************************/
/*                         Peripheral memory map                              */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_MemoryMap rn831x_rn861x_mcu Memory Mapping
 * @{
 */

/* base address */
#define FLASH_CORE0_BASE (0x30000000U)
#define FLASH_CORE1_BASE (0xB0000000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define FLASH_CORE0 ((FLASH_TypeDef *)    FLASH_CORE0_BASE)
#define FLASH_CORE1 ((FLASH_TypeDef *)    FLASH_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define FLASH  FLASH_CORE0
#endif
#if defined(CORE1)
#define FLASH  FLASH_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** FLASH Instances *******************************/
#define IS_FLASH_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == FLASH_CORE0) || \
                                            ((INSTANCE) == FLASH_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* FLASH_RDCTRL */
#define FLASH_RDCTRL_RD32_EN                 (BIT(0))                                            /*!< 32-byte read enable */
#define FLASH_RDCTRL_RECNT_EN                (BIT(1))                                            /*!< RE_CNT operation control */

/* FLASH_MAINPEA */
#define FLASH_MAINPEA_MAIN_ROW_SEL           (BIT(0))                                            /*!< Selected sector index */
#define FLASH_MAINPEA_MAIN_SECTOR_SEL        (BITS(1,11))                                        /*!< Selected row in sector */

/* FLASH_SINGAL_PROGRAM */
#define FLASH_SINGAL_PROGRAM_SINGLE_BYTE     (BITS(0,3))                                         /*!< Byte select */
#define FLASH_SINGAL_PROGRAM_SINGLE_ADDR     (BITS(4,8))                                         /*!< Programming address (lower 5 bits of flash address) */
#define FLASH_SINGAL_PROGRAM_SINGLE_PROG_EN  (BIT(12))                                           /*!< Single-byte programming mode */

/* FLASH_STA */
#define FLASH_STA_KEYSTORE_BUSY              (BIT(0))                                            /*!< Controller busy (keystore commands) */
#define FLASH_STA_HSM_BUSY                   (BIT(1))                                            /*!< Controller busy (HSM commands) */
#define FLASH_STA_HOST_BUSY                  (BIT(2))                                            /*!< Controller busy (host commands) */
#define FLASH_STA_DEEP_STANDBY_BUSY          (BIT(3))                                            /*!< Deep standby status */
#define FLASH_STA_CMD_BUSY                   (BIT(4))                                            /*!< Command busy (FLASH_CMD) */
#define FLASH_STA_KS_CMD_BUSY                (BIT(5))                                            /*!< Keystore command busy */
#define FLASH_STA_LOCK_ACTIVATE              (BIT(6))                                            /*!< Lock mechanism status */

/* FLASH_RECNT1 */
#define FLASH_RECNT1_RECNT1                  (BITS(0,31))                                        /*!< Lower 32 bits of read counter */

/* FLASH_RECNT2 */
#define FLASH_RECNT2_RECNT2                  (BITS(0,31))                                        /*!< Upper 32 bits of read counter */

/* FLASH_PSW */
#define FLASH_PSW_PSW                        (BITS(0,15))                                        /*!< Unlock password */

/* FLASH_REMAP */
#define FLASH_REMAP_REMAP_ADDR               (BITS(0,6))                                         /*!< Remap address (8KB granularity, effective when remap_en=1) */
#define FLASH_REMAP_REMAP_KEY                (BITS(16,31))                                       /*!< Command key (effective only with valid FLASH_PSW) */

/* FLASH_MAINPRO1 */
#define FLASH_MAINPRO1_MAIN_PRO1             (BITS(0,15))                                        /*!< 8KB sector protection (bit0=sector0, bit1=sector1, etc.), 0: Protected (no erase/write), 1: Allow erase/write */
#define FLASH_MAINPRO1_MAINPRO1_KEY          (BITS(16,31))                                       /*!< Protection key (requires valid FLASH_PSW) */

/* FLASH_MAINPRO2 */
#define FLASH_MAINPRO2_MAIN_PRO2             (BITS(0,15))                                        /*!< 8KB sector protection (bit0=sector0, bit1=sector1, etc.), 0: Protected (no erase/write), 1: Allow erase/write */
#define FLASH_MAINPRO2_MAINPRO2_KEY          (BITS(16,31))                                       /*!< Protection key (requires valid FLASH_PSW) */

/* FLASH_MAINPRO3 */
#define FLASH_MAINPRO3_MAIN_PRO3             (BITS(0,15))                                        /*!< 8KB sector protection (bit0=sector0, bit1=sector1, etc.), 0: Protected (no erase/write), 1: Allow erase/write */
#define FLASH_MAINPRO3_MAINPRO3_KEY          (BITS(16,31))                                       /*!< Protection key (requires valid FLASH_PSW) */

/* FLASH_MAINPRO4 */
#define FLASH_MAINPRO4_MAIN_PRO4             (BITS(0,15))                                        /*!< 8KB sector protection (bit0=sector0, bit1=sector1, etc.), 0: Protected (no erase/write), 1: Allow erase/write */
#define FLASH_MAINPRO4_MAINPRO4_KEY          (BITS(16,31))                                       /*!< Protection key (requires valid FLASH_PSW) */

/* FLASH_MAINPRO5 */
#define FLASH_MAINPRO5_MAIN_PRO5             (BITS(0,15))                                        /*!< 8KB sector protection (bit0=sector0, bit1=sector1, etc.), 0: Protected (no erase/write), 1: Allow erase/write */
#define FLASH_MAINPRO5_MAINPRO5_KEY          (BITS(16,31))                                       /*!< Protection key (requires valid FLASH_PSW) */

/* FLASH_MAINPRO6 */
#define FLASH_MAINPRO6_MAIN_PRO6             (BITS(0,15))                                        /*!< 8KB sector protection (bit0=sector0, bit1=sector1, etc.), 0: Protected (no erase/write), 1: Allow erase/write */
#define FLASH_MAINPRO6_MAINPRO6_KEY          (BITS(16,31))                                       /*!< Protection key (requires valid FLASH_PSW) */

/* FLASH_MAINPRO7 */
#define FLASH_MAINPRO7_MAIN_PRO7             (BITS(0,15))                                        /*!< 8KB sector protection (bit0=sector0, bit1=sector1, etc.), 0: Protected (no erase/write), 1: Allow erase/write */
#define FLASH_MAINPRO7_MAINPRO7_KEY          (BITS(16,31))                                       /*!< Protection key (requires valid FLASH_PSW) */

/* FLASH_MAINPRO8 */
#define FLASH_MAINPRO8_MAIN_PRO8             (BITS(0,15))                                        /*!< 8KB sector protection (bit0=sector0, bit1=sector1, etc.), 0: Protected (no erase/write), 1: Allow erase/write */
#define FLASH_MAINPRO8_MAINPRO8_KEY          (BITS(16,31))                                       /*!< Protection key (requires valid FLASH_PSW) */

/* FLASH_PG_DATA0 */
#define FLASH_PG_DATA0_NVR_PG_DATA           (BITS(0,31))                                        /*!< Programming data buffer0, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA1 */
#define FLASH_PG_DATA1_NVR_PG_DATA           (BITS(0,31))                                        /*!< Programming data buffer1, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA2 */
#define FLASH_PG_DATA2_NVR_PG_DATA           (BITS(0,31))                                        /*!< Programming data buffer2, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA3 */
#define FLASH_PG_DATA3_NVR_PG_DATA           (BITS(0,31))                                        /*!< Programming data buffer3, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA4 */
#define FLASH_PG_DATA4_NVR_PG_DATA           (BITS(0,31))                                        /*!< Programming data buffer4, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA5 */
#define FLASH_PG_DATA5_NVR_PG_DATA           (BITS(0,31))                                        /*!< Programming data buffer5, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA6 */
#define FLASH_PG_DATA6_NVR_PG_DATA           (BITS(0,31))                                        /*!< Programming data buffer6, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA7 */
#define FLASH_PG_DATA7_NVR_PG_DATA           (BITS(0,31))                                        /*!< Programming data buffer7, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA8 */
#define FLASH_PG_DATA8_NVR_PG_DATA           (BITS(0,31))                                        /*!< Programming data buffer8, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA9 */
#define FLASH_PG_DATA9_NVR_PG_DATA           (BITS(0,31))                                        /*!< Programming data buffer9, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA10 */
#define FLASH_PG_DATA10_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer10, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA11 */
#define FLASH_PG_DATA11_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer11, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA12 */
#define FLASH_PG_DATA12_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer12, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA13 */
#define FLASH_PG_DATA13_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer13, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA14 */
#define FLASH_PG_DATA14_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer14, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA15 */
#define FLASH_PG_DATA15_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer15, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA16 */
#define FLASH_PG_DATA16_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer16, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA17 */
#define FLASH_PG_DATA17_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer17, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA18 */
#define FLASH_PG_DATA18_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer18, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA19 */
#define FLASH_PG_DATA19_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer19, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA20 */
#define FLASH_PG_DATA20_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer20, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA21 */
#define FLASH_PG_DATA21_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer21, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA22 */
#define FLASH_PG_DATA22_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer22, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA23 */
#define FLASH_PG_DATA23_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer23, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA24 */
#define FLASH_PG_DATA24_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer24, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA25 */
#define FLASH_PG_DATA25_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer25, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA26 */
#define FLASH_PG_DATA26_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer26, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA27 */
#define FLASH_PG_DATA27_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer27, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA28 */
#define FLASH_PG_DATA28_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer28, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA29 */
#define FLASH_PG_DATA29_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer29, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA30 */
#define FLASH_PG_DATA30_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer30, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA31 */
#define FLASH_PG_DATA31_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer31, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA32 */
#define FLASH_PG_DATA32_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer32, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA33 */
#define FLASH_PG_DATA33_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer33, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA34 */
#define FLASH_PG_DATA34_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer34, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA35 */
#define FLASH_PG_DATA35_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer35, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA36 */
#define FLASH_PG_DATA36_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer36, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA37 */
#define FLASH_PG_DATA37_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer37, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA38 */
#define FLASH_PG_DATA38_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer38, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA39 */
#define FLASH_PG_DATA39_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer39, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA40 */
#define FLASH_PG_DATA40_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer40, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA41 */
#define FLASH_PG_DATA41_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer41, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA42 */
#define FLASH_PG_DATA42_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer42, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA43 */
#define FLASH_PG_DATA43_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer43, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA44 */
#define FLASH_PG_DATA44_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer44, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA45 */
#define FLASH_PG_DATA45_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer45, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA46 */
#define FLASH_PG_DATA46_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer46, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA47 */
#define FLASH_PG_DATA47_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer47, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA48 */
#define FLASH_PG_DATA48_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer48, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA49 */
#define FLASH_PG_DATA49_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer49, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA50 */
#define FLASH_PG_DATA50_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer50, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA51 */
#define FLASH_PG_DATA51_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer51, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA52 */
#define FLASH_PG_DATA52_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer52, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA53 */
#define FLASH_PG_DATA53_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer53, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA54 */
#define FLASH_PG_DATA54_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer54, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA55 */
#define FLASH_PG_DATA55_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer55, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA56 */
#define FLASH_PG_DATA56_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer56, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA57 */
#define FLASH_PG_DATA57_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer57, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA58 */
#define FLASH_PG_DATA58_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer58, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA59 */
#define FLASH_PG_DATA59_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer59, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA60 */
#define FLASH_PG_DATA60_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer60, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA61 */
#define FLASH_PG_DATA61_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer61, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA62 */
#define FLASH_PG_DATA62_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer62, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_PG_DATA63 */
#define FLASH_PG_DATA63_NVR_PG_DATA          (BITS(0,31))                                        /*!< Programming data buffer63, (64-bit word = {PG_DATA[n+1], PG_DATA[n]}) */

/* FLASH_OPRT_MODE */
#define FLASH_OPRT_MODE_HOST_ENABLE_MODE     (BIT(0))                                            /*!< Host command enable mode */
#define FLASH_OPRT_MODE_MAINPRO8_KEY         (BIT(1))                                            /*!< Host operation mode */
#define FLASH_OPRT_MODE_HSM_ENABLE_MODE      (BIT(2))                                            /*!< HSM command enable mode */
#define FLASH_OPRT_MODE_HSM_OPRT_MODE        (BIT(3))                                            /*!< HSM operation mode */

/* FLASH_CMD_CTRL */
#define FLASH_CMD_CTRL_HOSTCMD_ENABLE        (BIT(0))                                            /*!< Host command enable */
#define FLASH_CMD_CTRL_HSMCMD_ENABLE         (BIT(1))                                            /*!< HSM command enable */

/* FLASH_HSMCMD */
#define FLASH_HSMCMD_HSM_CMD                 (BITS(0,2))                                         /*!< Command encoding */
#define FLASH_HSMCMD_HSMCMD_KEY              (BITS(16,31))                                       /*!< Command key (0xC362 to activate) */

/* FLASH_HOSTCMD */
#define FLASH_HOSTCMD_HOST_CMD               (BITS(0,1))                                         /*!< Command encoding */
#define FLASH_HOSTCMD_HOST_KEY               (BITS(16,31))                                       /*!< Command key (0x2ADC to activate) */

/* FLASH_CTRL */
#define FLASH_CTRL_AUTO_STANDBY              (BIT(0))                                            /*!< Standby control */
#define FLASH_CTRL_FWUP_EN                   (BIT(1))                                            /*!< Deep standby wakeup */
#define FLASH_CTRL_AUTO_NS_CNT               (BIT(2))                                            /*!< 1μs timer source */

/* FLASH_NVRPEA */
#define FLASH_NVRPEA_NVR_ROW_SEL             (BIT(0))                                            /*!< Row index in sector (0=first row, etc.) */
#define FLASH_NVRPEA_NVR_SECTOR_SEL          (BITS(1,6))                                         /*!< NVR sector index (0=first sector, etc.) */
#define FLASH_NVRPEA_NVR_SEL                 (BIT(8))                                            /*!< Flash area selection */

/* FLASH_PRIORITY */
#define FLASH_PRIORITY_PRI_HSM               (BITS(0,1))                                         /*!< HSM priority */
#define FLASH_PRIORITY_PRI_HOST              (BITS(2,3))                                         /*!< Host priority */
#define FLASH_PRIORITY_PRI_KEYSTORE          (BITS(4,5))                                         /*!< Keystore priority */
#define FLASH_PRIORITY_PRI_KEY               (BITS(16,31))                                       /*!< Configuration key (0x6F2C to activate) */

/* FLASH_TCE */
#define FLASH_TCE_T0_CHIP_ERASE              (BITS(0,2))                                         /*!< Fractional ms (0.128ms units) */
#define FLASH_TCE_T1_CHIP_ERASE              (BITS(3,8))                                         /*!< Chip erase time - integer ms */

/* FLASH_TSE */
#define FLASH_TSE_T0_SECTOR_ERASE            (BITS(0,2))                                         /*!< Fractional ms */
#define FLASH_TSE_T1_SECTOR_ERASE            (BITS(3,8))                                         /*!< Sector erase - integer ms */

/* FLASH_TP */
#define FLASH_TP_PROGRAM_NS                  (BITS(0,6))                                         /*!< Fractional μs */
#define FLASH_TP_PROGRAM_US                  (BITS(7,10))                                        /*!< Program time - integer μs */

/* FLASH_TUS */
#define FLASH_TUS_US_UNIT                    (BITS(0,6))                                         /*!< Clock cycles per 1μs */

/* FLASH_TW */
#define FLASH_TW_FLASH_RD_WAIT               (BITS(0,2))                                         /*!< Read wait cycle configuration */

/* FLASH_READONLY */
#define FLASH_READONLY_READ_ONLY_EN          (BITS(0,7))                                         /*!< Security lock control */

/* FLASH_TRIM */
#define FLASH_TRIM_TRIM_FLASH                (BITS(0,15))                                        /*!< Calibration control */
#define FLASH_TRIM_TRIM_WKEY                 (BITS(16,31))                                       /*!< Write key (0xD0A3 enables writes) */

/* FLASH_ALLOCATE1 */
#define FLASH_ALLOCATE1_HSM_ENDADDR          (BITS(0,6))                                         /*!< N200 accessible end address */
#define FLASH_ALLOCATE1_HOST_ENDADDR         (BITS(8,14))                                        /*!< N300 accessible end address */
#define FLASH_ALLOCATE1_ALLOCATE1_KEY        (BITS(16,31))                                       /*!< Config key (0xABA3 with ALLOCATE1_LOCK=0x6A) */

/* FLASH_NVRRD1 */
#define FLASH_NVRRD1_NVR_PRO_RD1             (BITS(0,15))                                        /*!< Sector 0-15 read enable */
#define FLASH_NVRRD1_NVRPRO_RD1_KEY          (BITS(16,31))                                       /*!< Key (0x63DA) */

/* FLASH_NVRRD2 */
#define FLASH_NVRRD2_NVR_PRO_RD2             (BITS(0,15))                                        /*!< Sector 0-15 read enable */
#define FLASH_NVRRD2_NVRPRO_RD2_KEY          (BITS(16,31))                                       /*!< Key (0x63DA) */

/* FLASH_NVRRD3 */
#define FLASH_NVRRD3_NVR_PRO_RD3             (BITS(0,15))                                        /*!< Sector 0-15 read enable */
#define FLASH_NVRRD3_NVRPRO_RD3_KEY          (BITS(16,31))                                       /*!< Key (0x63DA) */

/* FLASH_NVRWR1 */
#define FLASH_NVRWR1_NVR_PRO_WR1             (BITS(0,15))                                        /*!< Sector 0-15 write enable */
#define FLASH_NVRWR1_NVRPRO_WR1_KEY          (BITS(16,31))                                       /*!< Key (0x63DA) */

/* FLASH_NVRWR2 */
#define FLASH_NVRWR2_NVR_PRO_WR2             (BITS(0,15))                                        /*!< Sector 0-15 write enable */
#define FLASH_NVRWR2_NVRPRO_WR2_KEY          (BITS(16,31))                                       /*!< Key (0x63DA) */

/* FLASH_NVRWR3 */
#define FLASH_NVRWR3_NVR_PRO_WR3             (BITS(0,15))                                        /*!< Sector 0-15 write enable */
#define FLASH_NVRWR3_NVRPRO_WR3_KEY          (BITS(16,31))                                       /*!< Key (0x63DA) */

/* FLASH_ALLOCATE1_LOCK */
#define FLASH_ALLOCATE1_LOCK_ALLOCATE1_LOCK  (BITS(0,7))                                         /*!< Write any value clears to 0x00, Only writable when value=0x6A */

/* FLASH_CHIPERASE_PSW */
#define FLASH_CHIPERASE_PSW_CHIPERASE_PSW    (BITS(0,7))                                         /*!< Enable value (0x6D) for chip erase */

/* FLASH_LIFE_MANAGE */
#define FLASH_LIFE_MANAGE_FLASH_LIFE_MANAGE  (BIT(0))                                            /*!< Sector 47 control */
#define FLASH_LIFE_MANAGE_FLASH_LIFE_KEY     (BITS(16,31))                                       /*!< Key (0x12CD) */

/* FLASH_LIFE_MANAGE_LOCK */
#define FLASH_LIFE_MANAGE_LOCK_LIFE_MANAGE_LOCK  (BITS(0,7))                                         /*!< Write any value clears to 0x00, Only writable when value=0x73 */

/* constants definitions */
/* FLASH_RDCTRL_RD32_EN */
#define FLASH_RDCTRL_RD32_EN_POS             (0U)                                                /*!< FLASH_RDCTRL_RD32_EN bit offset */

/* FLASH_RDCTRL_RECNT_EN */
#define FLASH_RDCTRL_RECNT_EN_POS            (1U)                                                /*!< FLASH_RDCTRL_RECNT_EN bit offset */

/* FLASH_MAINPEA_MAIN_ROW_SEL */
#define FLASH_MAINPEA_MAIN_ROW_SEL_POS       (0U)                                                /*!< FLASH_MAINPEA_MAIN_ROW_SEL bit offset */

/* FLASH_MAINPEA_MAIN_SECTOR_SEL */
#define FLASH_MAINPEA_MAIN_SECTOR_SEL_POS    (1U)                                                /*!< FLASH_MAINPEA_MAIN_SECTOR_SEL bit offset */

/* FLASH_SINGAL_PROGRAM_SINGLE_BYTE */
#define FLASH_SINGAL_PROGRAM_SINGLE_BYTE_POS  (0U)                                                /*!< FLASH_SINGAL_PROGRAM_SINGLE_BYTE bit offset */

/* FLASH_SINGAL_PROGRAM_SINGLE_ADDR */
#define FLASH_SINGAL_PROGRAM_SINGLE_ADDR_POS  (4U)                                                /*!< FLASH_SINGAL_PROGRAM_SINGLE_ADDR bit offset */

/* FLASH_SINGAL_PROGRAM_SINGLE_PROG_EN */
#define FLASH_SINGAL_PROGRAM_SINGLE_PROG_EN_POS  (12U)                                               /*!< FLASH_SINGAL_PROGRAM_SINGLE_PROG_EN bit offset */

/* FLASH_STA_KEYSTORE_BUSY */
#define FLASH_STA_KEYSTORE_BUSY_POS          (0U)                                                /*!< FLASH_STA_KEYSTORE_BUSY bit offset */

/* FLASH_STA_HSM_BUSY */
#define FLASH_STA_HSM_BUSY_POS               (1U)                                                /*!< FLASH_STA_HSM_BUSY bit offset */

/* FLASH_STA_HOST_BUSY */
#define FLASH_STA_HOST_BUSY_POS              (2U)                                                /*!< FLASH_STA_HOST_BUSY bit offset */

/* FLASH_STA_DEEP_STANDBY_BUSY */
#define FLASH_STA_DEEP_STANDBY_BUSY_POS      (3U)                                                /*!< FLASH_STA_DEEP_STANDBY_BUSY bit offset */

/* FLASH_STA_CMD_BUSY */
#define FLASH_STA_CMD_BUSY_POS               (4U)                                                /*!< FLASH_STA_CMD_BUSY bit offset */

/* FLASH_STA_KS_CMD_BUSY */
#define FLASH_STA_KS_CMD_BUSY_POS            (5U)                                                /*!< FLASH_STA_KS_CMD_BUSY bit offset */

/* FLASH_STA_LOCK_ACTIVATE */
#define FLASH_STA_LOCK_ACTIVATE_POS          (6U)                                                /*!< FLASH_STA_LOCK_ACTIVATE bit offset */

/* FLASH_RECNT1_RECNT1 */
#define FLASH_RECNT1_RECNT1_POS              (0U)                                                /*!< FLASH_RECNT1_RECNT1 bit offset */

/* FLASH_RECNT2_RECNT2 */
#define FLASH_RECNT2_RECNT2_POS              (0U)                                                /*!< FLASH_RECNT2_RECNT2 bit offset */

/* FLASH_PSW_PSW */
#define FLASH_PSW_PSW_POS                    (0U)                                                /*!< FLASH_PSW_PSW bit offset */

/* FLASH_REMAP_REMAP_ADDR */
#define FLASH_REMAP_REMAP_ADDR_POS           (0U)                                                /*!< FLASH_REMAP_REMAP_ADDR bit offset */

/* FLASH_REMAP_REMAP_KEY */
#define FLASH_REMAP_REMAP_KEY_POS            (16U)                                               /*!< FLASH_REMAP_REMAP_KEY bit offset */

/* FLASH_MAINPRO1_MAIN_PRO1 */
#define FLASH_MAINPRO1_MAIN_PRO1_POS         (0U)                                                /*!< FLASH_MAINPRO1_MAIN_PRO1 bit offset */

/* FLASH_MAINPRO1_MAINPRO1_KEY */
#define FLASH_MAINPRO1_MAINPRO1_KEY_POS      (16U)                                               /*!< FLASH_MAINPRO1_MAINPRO1_KEY bit offset */

/* FLASH_MAINPRO2_MAIN_PRO2 */
#define FLASH_MAINPRO2_MAIN_PRO2_POS         (0U)                                                /*!< FLASH_MAINPRO2_MAIN_PRO2 bit offset */

/* FLASH_MAINPRO2_MAINPRO2_KEY */
#define FLASH_MAINPRO2_MAINPRO2_KEY_POS      (16U)                                               /*!< FLASH_MAINPRO2_MAINPRO2_KEY bit offset */

/* FLASH_MAINPRO3_MAIN_PRO3 */
#define FLASH_MAINPRO3_MAIN_PRO3_POS         (0U)                                                /*!< FLASH_MAINPRO3_MAIN_PRO3 bit offset */

/* FLASH_MAINPRO3_MAINPRO3_KEY */
#define FLASH_MAINPRO3_MAINPRO3_KEY_POS      (16U)                                               /*!< FLASH_MAINPRO3_MAINPRO3_KEY bit offset */

/* FLASH_MAINPRO4_MAIN_PRO4 */
#define FLASH_MAINPRO4_MAIN_PRO4_POS         (0U)                                                /*!< FLASH_MAINPRO4_MAIN_PRO4 bit offset */

/* FLASH_MAINPRO4_MAINPRO4_KEY */
#define FLASH_MAINPRO4_MAINPRO4_KEY_POS      (16U)                                               /*!< FLASH_MAINPRO4_MAINPRO4_KEY bit offset */

/* FLASH_MAINPRO5_MAIN_PRO5 */
#define FLASH_MAINPRO5_MAIN_PRO5_POS         (0U)                                                /*!< FLASH_MAINPRO5_MAIN_PRO5 bit offset */

/* FLASH_MAINPRO5_MAINPRO5_KEY */
#define FLASH_MAINPRO5_MAINPRO5_KEY_POS      (16U)                                               /*!< FLASH_MAINPRO5_MAINPRO5_KEY bit offset */

/* FLASH_MAINPRO6_MAIN_PRO6 */
#define FLASH_MAINPRO6_MAIN_PRO6_POS         (0U)                                                /*!< FLASH_MAINPRO6_MAIN_PRO6 bit offset */

/* FLASH_MAINPRO6_MAINPRO6_KEY */
#define FLASH_MAINPRO6_MAINPRO6_KEY_POS      (16U)                                               /*!< FLASH_MAINPRO6_MAINPRO6_KEY bit offset */

/* FLASH_MAINPRO7_MAIN_PRO7 */
#define FLASH_MAINPRO7_MAIN_PRO7_POS         (0U)                                                /*!< FLASH_MAINPRO7_MAIN_PRO7 bit offset */

/* FLASH_MAINPRO7_MAINPRO7_KEY */
#define FLASH_MAINPRO7_MAINPRO7_KEY_POS      (16U)                                               /*!< FLASH_MAINPRO7_MAINPRO7_KEY bit offset */

/* FLASH_MAINPRO8_MAIN_PRO8 */
#define FLASH_MAINPRO8_MAIN_PRO8_POS         (0U)                                                /*!< FLASH_MAINPRO8_MAIN_PRO8 bit offset */

/* FLASH_MAINPRO8_MAINPRO8_KEY */
#define FLASH_MAINPRO8_MAINPRO8_KEY_POS      (16U)                                               /*!< FLASH_MAINPRO8_MAINPRO8_KEY bit offset */

/* FLASH_PG_DATA0_NVR_PG_DATA */
#define FLASH_PG_DATA0_NVR_PG_DATA_POS       (0U)                                                /*!< FLASH_PG_DATA0_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA1_NVR_PG_DATA */
#define FLASH_PG_DATA1_NVR_PG_DATA_POS       (0U)                                                /*!< FLASH_PG_DATA1_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA2_NVR_PG_DATA */
#define FLASH_PG_DATA2_NVR_PG_DATA_POS       (0U)                                                /*!< FLASH_PG_DATA2_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA3_NVR_PG_DATA */
#define FLASH_PG_DATA3_NVR_PG_DATA_POS       (0U)                                                /*!< FLASH_PG_DATA3_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA4_NVR_PG_DATA */
#define FLASH_PG_DATA4_NVR_PG_DATA_POS       (0U)                                                /*!< FLASH_PG_DATA4_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA5_NVR_PG_DATA */
#define FLASH_PG_DATA5_NVR_PG_DATA_POS       (0U)                                                /*!< FLASH_PG_DATA5_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA6_NVR_PG_DATA */
#define FLASH_PG_DATA6_NVR_PG_DATA_POS       (0U)                                                /*!< FLASH_PG_DATA6_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA7_NVR_PG_DATA */
#define FLASH_PG_DATA7_NVR_PG_DATA_POS       (0U)                                                /*!< FLASH_PG_DATA7_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA8_NVR_PG_DATA */
#define FLASH_PG_DATA8_NVR_PG_DATA_POS       (0U)                                                /*!< FLASH_PG_DATA8_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA9_NVR_PG_DATA */
#define FLASH_PG_DATA9_NVR_PG_DATA_POS       (0U)                                                /*!< FLASH_PG_DATA9_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA10_NVR_PG_DATA */
#define FLASH_PG_DATA10_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA10_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA11_NVR_PG_DATA */
#define FLASH_PG_DATA11_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA11_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA12_NVR_PG_DATA */
#define FLASH_PG_DATA12_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA12_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA13_NVR_PG_DATA */
#define FLASH_PG_DATA13_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA13_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA14_NVR_PG_DATA */
#define FLASH_PG_DATA14_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA14_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA15_NVR_PG_DATA */
#define FLASH_PG_DATA15_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA15_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA16_NVR_PG_DATA */
#define FLASH_PG_DATA16_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA16_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA17_NVR_PG_DATA */
#define FLASH_PG_DATA17_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA17_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA18_NVR_PG_DATA */
#define FLASH_PG_DATA18_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA18_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA19_NVR_PG_DATA */
#define FLASH_PG_DATA19_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA19_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA20_NVR_PG_DATA */
#define FLASH_PG_DATA20_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA20_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA21_NVR_PG_DATA */
#define FLASH_PG_DATA21_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA21_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA22_NVR_PG_DATA */
#define FLASH_PG_DATA22_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA22_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA23_NVR_PG_DATA */
#define FLASH_PG_DATA23_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA23_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA24_NVR_PG_DATA */
#define FLASH_PG_DATA24_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA24_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA25_NVR_PG_DATA */
#define FLASH_PG_DATA25_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA25_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA26_NVR_PG_DATA */
#define FLASH_PG_DATA26_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA26_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA27_NVR_PG_DATA */
#define FLASH_PG_DATA27_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA27_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA28_NVR_PG_DATA */
#define FLASH_PG_DATA28_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA28_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA29_NVR_PG_DATA */
#define FLASH_PG_DATA29_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA29_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA30_NVR_PG_DATA */
#define FLASH_PG_DATA30_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA30_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA31_NVR_PG_DATA */
#define FLASH_PG_DATA31_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA31_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA32_NVR_PG_DATA */
#define FLASH_PG_DATA32_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA32_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA33_NVR_PG_DATA */
#define FLASH_PG_DATA33_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA33_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA34_NVR_PG_DATA */
#define FLASH_PG_DATA34_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA34_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA35_NVR_PG_DATA */
#define FLASH_PG_DATA35_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA35_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA36_NVR_PG_DATA */
#define FLASH_PG_DATA36_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA36_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA37_NVR_PG_DATA */
#define FLASH_PG_DATA37_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA37_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA38_NVR_PG_DATA */
#define FLASH_PG_DATA38_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA38_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA39_NVR_PG_DATA */
#define FLASH_PG_DATA39_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA39_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA40_NVR_PG_DATA */
#define FLASH_PG_DATA40_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA40_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA41_NVR_PG_DATA */
#define FLASH_PG_DATA41_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA41_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA42_NVR_PG_DATA */
#define FLASH_PG_DATA42_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA42_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA43_NVR_PG_DATA */
#define FLASH_PG_DATA43_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA43_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA44_NVR_PG_DATA */
#define FLASH_PG_DATA44_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA44_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA45_NVR_PG_DATA */
#define FLASH_PG_DATA45_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA45_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA46_NVR_PG_DATA */
#define FLASH_PG_DATA46_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA46_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA47_NVR_PG_DATA */
#define FLASH_PG_DATA47_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA47_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA48_NVR_PG_DATA */
#define FLASH_PG_DATA48_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA48_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA49_NVR_PG_DATA */
#define FLASH_PG_DATA49_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA49_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA50_NVR_PG_DATA */
#define FLASH_PG_DATA50_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA50_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA51_NVR_PG_DATA */
#define FLASH_PG_DATA51_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA51_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA52_NVR_PG_DATA */
#define FLASH_PG_DATA52_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA52_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA53_NVR_PG_DATA */
#define FLASH_PG_DATA53_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA53_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA54_NVR_PG_DATA */
#define FLASH_PG_DATA54_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA54_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA55_NVR_PG_DATA */
#define FLASH_PG_DATA55_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA55_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA56_NVR_PG_DATA */
#define FLASH_PG_DATA56_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA56_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA57_NVR_PG_DATA */
#define FLASH_PG_DATA57_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA57_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA58_NVR_PG_DATA */
#define FLASH_PG_DATA58_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA58_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA59_NVR_PG_DATA */
#define FLASH_PG_DATA59_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA59_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA60_NVR_PG_DATA */
#define FLASH_PG_DATA60_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA60_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA61_NVR_PG_DATA */
#define FLASH_PG_DATA61_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA61_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA62_NVR_PG_DATA */
#define FLASH_PG_DATA62_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA62_NVR_PG_DATA bit offset */

/* FLASH_PG_DATA63_NVR_PG_DATA */
#define FLASH_PG_DATA63_NVR_PG_DATA_POS      (0U)                                                /*!< FLASH_PG_DATA63_NVR_PG_DATA bit offset */

/* FLASH_OPRT_MODE_HOST_ENABLE_MODE */
#define FLASH_OPRT_MODE_HOST_ENABLE_MODE_POS  (0U)                                                /*!< FLASH_OPRT_MODE_HOST_ENABLE_MODE bit offset */

/* FLASH_OPRT_MODE_MAINPRO8_KEY */
#define FLASH_OPRT_MODE_MAINPRO8_KEY_POS     (1U)                                                /*!< FLASH_OPRT_MODE_MAINPRO8_KEY bit offset */

/* FLASH_OPRT_MODE_HSM_ENABLE_MODE */
#define FLASH_OPRT_MODE_HSM_ENABLE_MODE_POS  (2U)                                                /*!< FLASH_OPRT_MODE_HSM_ENABLE_MODE bit offset */

/* FLASH_OPRT_MODE_HSM_OPRT_MODE */
#define FLASH_OPRT_MODE_HSM_OPRT_MODE_POS    (3U)                                                /*!< FLASH_OPRT_MODE_HSM_OPRT_MODE bit offset */

/* FLASH_CMD_CTRL_HOSTCMD_ENABLE */
#define FLASH_CMD_CTRL_HOSTCMD_ENABLE_POS    (0U)                                                /*!< FLASH_CMD_CTRL_HOSTCMD_ENABLE bit offset */

/* FLASH_CMD_CTRL_HSMCMD_ENABLE */
#define FLASH_CMD_CTRL_HSMCMD_ENABLE_POS     (1U)                                                /*!< FLASH_CMD_CTRL_HSMCMD_ENABLE bit offset */

/* FLASH_HSMCMD_HSM_CMD */
#define FLASH_HSMCMD_HSM_CMD_POS             (0U)                                                /*!< FLASH_HSMCMD_HSM_CMD bit offset */

/* FLASH_HSMCMD_HSMCMD_KEY */
#define FLASH_HSMCMD_HSMCMD_KEY_POS          (16U)                                               /*!< FLASH_HSMCMD_HSMCMD_KEY bit offset */

/* FLASH_HOSTCMD_HOST_CMD */
#define FLASH_HOSTCMD_HOST_CMD_POS           (0U)                                                /*!< FLASH_HOSTCMD_HOST_CMD bit offset */

/* FLASH_HOSTCMD_HOST_KEY */
#define FLASH_HOSTCMD_HOST_KEY_POS           (16U)                                               /*!< FLASH_HOSTCMD_HOST_KEY bit offset */

/* FLASH_CTRL_AUTO_STANDBY */
#define FLASH_CTRL_AUTO_STANDBY_POS          (0U)                                                /*!< FLASH_CTRL_AUTO_STANDBY bit offset */

/* FLASH_CTRL_FWUP_EN */
#define FLASH_CTRL_FWUP_EN_POS               (1U)                                                /*!< FLASH_CTRL_FWUP_EN bit offset */

/* FLASH_CTRL_AUTO_NS_CNT */
#define FLASH_CTRL_AUTO_NS_CNT_POS           (2U)                                                /*!< FLASH_CTRL_AUTO_NS_CNT bit offset */

/* FLASH_NVRPEA_NVR_ROW_SEL */
#define FLASH_NVRPEA_NVR_ROW_SEL_POS         (0U)                                                /*!< FLASH_NVRPEA_NVR_ROW_SEL bit offset */

/* FLASH_NVRPEA_NVR_SECTOR_SEL */
#define FLASH_NVRPEA_NVR_SECTOR_SEL_POS      (1U)                                                /*!< FLASH_NVRPEA_NVR_SECTOR_SEL bit offset */

/* FLASH_NVRPEA_NVR_SEL */
#define FLASH_NVRPEA_NVR_SEL_POS             (8U)                                                /*!< FLASH_NVRPEA_NVR_SEL bit offset */

/* FLASH_PRIORITY_PRI_HSM */
#define FLASH_PRIORITY_PRI_HSM_POS           (0U)                                                /*!< FLASH_PRIORITY_PRI_HSM bit offset */

/* FLASH_PRIORITY_PRI_HOST */
#define FLASH_PRIORITY_PRI_HOST_POS          (2U)                                                /*!< FLASH_PRIORITY_PRI_HOST bit offset */

/* FLASH_PRIORITY_PRI_KEYSTORE */
#define FLASH_PRIORITY_PRI_KEYSTORE_POS      (4U)                                                /*!< FLASH_PRIORITY_PRI_KEYSTORE bit offset */

/* FLASH_PRIORITY_PRI_KEY */
#define FLASH_PRIORITY_PRI_KEY_POS           (16U)                                               /*!< FLASH_PRIORITY_PRI_KEY bit offset */

/* FLASH_TCE_T0_CHIP_ERASE */
#define FLASH_TCE_T0_CHIP_ERASE_POS          (0U)                                                /*!< FLASH_TCE_T0_CHIP_ERASE bit offset */

/* FLASH_TCE_T1_CHIP_ERASE */
#define FLASH_TCE_T1_CHIP_ERASE_POS          (3U)                                                /*!< FLASH_TCE_T1_CHIP_ERASE bit offset */

/* FLASH_TSE_T0_SECTOR_ERASE */
#define FLASH_TSE_T0_SECTOR_ERASE_POS        (0U)                                                /*!< FLASH_TSE_T0_SECTOR_ERASE bit offset */

/* FLASH_TSE_T1_SECTOR_ERASE */
#define FLASH_TSE_T1_SECTOR_ERASE_POS        (3U)                                                /*!< FLASH_TSE_T1_SECTOR_ERASE bit offset */

/* FLASH_TP_PROGRAM_NS */
#define FLASH_TP_PROGRAM_NS_POS              (0U)                                                /*!< FLASH_TP_PROGRAM_NS bit offset */

/* FLASH_TP_PROGRAM_US */
#define FLASH_TP_PROGRAM_US_POS              (7U)                                                /*!< FLASH_TP_PROGRAM_US bit offset */

/* FLASH_TUS_US_UNIT */
#define FLASH_TUS_US_UNIT_POS                (0U)                                                /*!< FLASH_TUS_US_UNIT bit offset */

/* FLASH_TW_FLASH_RD_WAIT */
#define FLASH_TW_FLASH_RD_WAIT_POS           (0U)                                                /*!< FLASH_TW_FLASH_RD_WAIT bit offset */

/* FLASH_READONLY_READ_ONLY_EN */
#define FLASH_READONLY_READ_ONLY_EN_POS      (0U)                                                /*!< FLASH_READONLY_READ_ONLY_EN bit offset */

/* FLASH_TRIM_TRIM_FLASH */
#define FLASH_TRIM_TRIM_FLASH_POS            (0U)                                                /*!< FLASH_TRIM_TRIM_FLASH bit offset */

/* FLASH_TRIM_TRIM_WKEY */
#define FLASH_TRIM_TRIM_WKEY_POS             (16U)                                               /*!< FLASH_TRIM_TRIM_WKEY bit offset */

/* FLASH_ALLOCATE1_HSM_ENDADDR */
#define FLASH_ALLOCATE1_HSM_ENDADDR_POS      (0U)                                                /*!< FLASH_ALLOCATE1_HSM_ENDADDR bit offset */

/* FLASH_ALLOCATE1_HOST_ENDADDR */
#define FLASH_ALLOCATE1_HOST_ENDADDR_POS     (8U)                                                /*!< FLASH_ALLOCATE1_HOST_ENDADDR bit offset */

/* FLASH_ALLOCATE1_ALLOCATE1_KEY */
#define FLASH_ALLOCATE1_ALLOCATE1_KEY_POS    (16U)                                               /*!< FLASH_ALLOCATE1_ALLOCATE1_KEY bit offset */

/* FLASH_NVRRD1_NVR_PRO_RD1 */
#define FLASH_NVRRD1_NVR_PRO_RD1_POS         (0U)                                                /*!< FLASH_NVRRD1_NVR_PRO_RD1 bit offset */

/* FLASH_NVRRD1_NVRPRO_RD1_KEY */
#define FLASH_NVRRD1_NVRPRO_RD1_KEY_POS      (16U)                                               /*!< FLASH_NVRRD1_NVRPRO_RD1_KEY bit offset */

/* FLASH_NVRRD2_NVR_PRO_RD2 */
#define FLASH_NVRRD2_NVR_PRO_RD2_POS         (0U)                                                /*!< FLASH_NVRRD2_NVR_PRO_RD2 bit offset */

/* FLASH_NVRRD2_NVRPRO_RD2_KEY */
#define FLASH_NVRRD2_NVRPRO_RD2_KEY_POS      (16U)                                               /*!< FLASH_NVRRD2_NVRPRO_RD2_KEY bit offset */

/* FLASH_NVRRD3_NVR_PRO_RD3 */
#define FLASH_NVRRD3_NVR_PRO_RD3_POS         (0U)                                                /*!< FLASH_NVRRD3_NVR_PRO_RD3 bit offset */

/* FLASH_NVRRD3_NVRPRO_RD3_KEY */
#define FLASH_NVRRD3_NVRPRO_RD3_KEY_POS      (16U)                                               /*!< FLASH_NVRRD3_NVRPRO_RD3_KEY bit offset */

/* FLASH_NVRWR1_NVR_PRO_WR1 */
#define FLASH_NVRWR1_NVR_PRO_WR1_POS         (0U)                                                /*!< FLASH_NVRWR1_NVR_PRO_WR1 bit offset */

/* FLASH_NVRWR1_NVRPRO_WR1_KEY */
#define FLASH_NVRWR1_NVRPRO_WR1_KEY_POS      (16U)                                               /*!< FLASH_NVRWR1_NVRPRO_WR1_KEY bit offset */

/* FLASH_NVRWR2_NVR_PRO_WR2 */
#define FLASH_NVRWR2_NVR_PRO_WR2_POS         (0U)                                                /*!< FLASH_NVRWR2_NVR_PRO_WR2 bit offset */

/* FLASH_NVRWR2_NVRPRO_WR2_KEY */
#define FLASH_NVRWR2_NVRPRO_WR2_KEY_POS      (16U)                                               /*!< FLASH_NVRWR2_NVRPRO_WR2_KEY bit offset */

/* FLASH_NVRWR3_NVR_PRO_WR3 */
#define FLASH_NVRWR3_NVR_PRO_WR3_POS         (0U)                                                /*!< FLASH_NVRWR3_NVR_PRO_WR3 bit offset */

/* FLASH_NVRWR3_NVRPRO_WR3_KEY */
#define FLASH_NVRWR3_NVRPRO_WR3_KEY_POS      (16U)                                               /*!< FLASH_NVRWR3_NVRPRO_WR3_KEY bit offset */

/* FLASH_ALLOCATE1_LOCK_ALLOCATE1_LOCK */
#define FLASH_ALLOCATE1_LOCK_ALLOCATE1_LOCK_POS  (0U)                                                /*!< FLASH_ALLOCATE1_LOCK_ALLOCATE1_LOCK bit offset */

/* FLASH_CHIPERASE_PSW_CHIPERASE_PSW */
#define FLASH_CHIPERASE_PSW_CHIPERASE_PSW_POS  (0U)                                                /*!< FLASH_CHIPERASE_PSW_CHIPERASE_PSW bit offset */

/* FLASH_LIFE_MANAGE_FLASH_LIFE_MANAGE */
#define FLASH_LIFE_MANAGE_FLASH_LIFE_MANAGE_POS  (0U)                                                /*!< FLASH_LIFE_MANAGE_FLASH_LIFE_MANAGE bit offset */

/* FLASH_LIFE_MANAGE_FLASH_LIFE_KEY */
#define FLASH_LIFE_MANAGE_FLASH_LIFE_KEY_POS  (16U)                                               /*!< FLASH_LIFE_MANAGE_FLASH_LIFE_KEY bit offset */

/* FLASH_LIFE_MANAGE_LOCK_LIFE_MANAGE_LOCK */
#define FLASH_LIFE_MANAGE_LOCK_LIFE_MANAGE_LOCK_POS  (0U)                                                /*!< FLASH_LIFE_MANAGE_LOCK_LIFE_MANAGE_LOCK bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_FLASH_H */
