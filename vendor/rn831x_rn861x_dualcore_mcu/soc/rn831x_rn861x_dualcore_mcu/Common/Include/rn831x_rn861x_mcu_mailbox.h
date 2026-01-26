/**
 ******************************************************************************
 * @file    rn831x_rn861x_mcu_mailbox.h
 * @author  Renergy Software Team
 * @brief   MAILBOX module register definitions for RN831x_RN861x_MCU.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */
#ifndef _RN831x_RN861x_MCU_MAILBOX_H
#define _RN831x_RN861x_MCU_MAILBOX_H

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

/* MAILBOX */
typedef struct
{
    __IO uint32_t RESERVED0[2];              /*!< offset: 0x000 ~ 0x004 */
    __IO uint32_t CSR;                       /*!< offset: 0x008 info: Control/Status Register */
    __IO uint32_t RESET;                     /*!< offset: 0x00C info: Reset Register (only core0 can access) */
    __IO uint32_t LINKID;                    /*!< offset: 0x010 info: Connection Host ID Register */
    __IO uint32_t RESERVED1;                 /*!< offset: 0x014 */
    __IO uint32_t LOCKOUT;                   /*!< offset: 0x018 info: Lockout Register */
    __IO uint32_t RESERVED2[7];              /*!< offset: 0x01C ~ 034 */
    __IO uint32_t INTEN;                     /*!< offset: 0x038 info: CORE0/CORE1 Interrupt Enable Register */
    __IO uint32_t INTST;                     /*!< offset: 0x03C info: CORE0/CORE1 Interrupt Status Register */
    __IO uint32_t RESERVED3[7];              /*!< offset: 0x040 ~ 058 */
    __IO uint32_t OPT;                       /*!< offset: 0x05C info: Internal Configuration Status Register */
    __IO uint32_t FROZE;                     /*!< offset: 0x060 info: Freeze Host Access Configuration Register (only core0 can access) */
    __IO uint32_t ID_VAL;                    /*!< offset: 0x064 info: Accessible Host ID Register (only core0 can access) */
    __IO uint32_t RESERVED4[31];             /*!< offset: 0x068 ~ 0E0 */
    __IO uint32_t ID_MASK;                   /*!< offset: 0x0E4 info: Accessible Host ID Mask Register (only core0 can access) */
    __IO uint32_t RESERVED5[134];            /*!< offset: 0x0E8 ~ 2FC */
    __IO uint32_t MBX_KEY0;                  /*!< offset: 0x300 info: CORE0 Password Register (only core0 can access) */
    __IO uint32_t MBX_KEY1;                  /*!< offset: 0x304 info: CORE1 Password Register (only core1 can access) */
    __IO uint32_t RESERVED6[1854];           /*!< offset: 0x308 ~ 1FFC */
    __IO uint32_t MESSAGE[64];               /*!< offset: 0x2000 ~ 0x20FF info: Mailbox Message Register. 256 Bytes SRAM. */
} MAILBOX_TypeDef;

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
#define MAILBOX_CORE0_BASE (0x3000C000U)
#define MAILBOX_CORE1_BASE (0xB000C000U)

/**
 * @}
 */ /* end of group rn831x_rn861x_mcu_MemoryMap */

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup rn831x_rn861x_mcu_Declaration rn831x_rn861x_mcu Declaration
 * @{
 */
#define MAILBOX_CORE0 ((MAILBOX_TypeDef *)  MAILBOX_CORE0_BASE)
#define MAILBOX_CORE1 ((MAILBOX_TypeDef *)  MAILBOX_CORE1_BASE)

#if defined(CORE1) && defined(CORE0)
#error "Should only define one of CORE0 or CORE1 !!!"
#endif
#if defined(CORE0)
#define MAILBOX  MAILBOX_CORE0
#endif
#if defined(CORE1)
#define MAILBOX  MAILBOX_CORE1
#endif

/**
 *@}
 */ /* end of group rn831x_rn861x_mcu_Declaration */

/** @addtogroup Exported_macro
 * @{
 */
/******************************** MAILBOX Instances *******************************/
#define IS_MAILBOX_ALL_INSTANCE(INSTANCE)    (((INSTANCE) == MAILBOX_CORE0) || \
                                              ((INSTANCE) == MAILBOX_CORE1))

/**
 * @}
 */

/***************************** bitband declaration ****************************/

/* bits definitions */
/* MAILBOX_CSR */
#define MAILBOX_CSR_MBX_IN_FULL              (BIT(0))                                            /*!< Input mailbox status bit. For CORE0: Write 1 to clear input mailbox status. For CORE1: Write 1 to indicate input mailbox full, set INTST.MBX_IFRE */
#define MAILBOX_CSR_MBX_OUT_FULL             (BIT(1))                                            /*!< Output mailbox status bit.For CORE0: Write 1 to indicate output mailbox full, sets INTST.MBX_DONE. For CORE1: Write 1 to clear output mailbox status. */
#define MAILBOX_CSR_MBX_LINK                 (BIT(2))                                            /*!< Write 1 to connect mailbox channel. If successful, this bit sets automatically to 1 (only core1 can access). */
#define MAILBOX_CSR_MBX_ALB_UNLINK           (BIT(3))                                            /*!< Write 1 to disconnect from mailbox and clear CSR.MBX_LINK bit (only core1 can access). */

/* MAILBOX_RESET */
#define MAILBOX_RESET_MBX_OUT_EMPTY          (BIT(1))                                            /*!< CORE0 writes 1 to this bit to clear CSR.MBX_OUT_FULL. */
#define MAILBOX_RESET_MBX_UNLINK             (BIT(3))                                            /*!< CORE0 writes 1 to this bit to clear CSR.MBX_LINKED bit and disconnect CORE1 from mailbox. */

/* MAILBOX_LINKID */
#define MAILBOX_LINKID_ID                    (BIT(0))                                            /*!< Read back the host ID number that has connected to the input mailbox */

/* MAILBOX_LOCKOUT */
#define MAILBOX_LOCKOUT_MBX                  (BIT(0))                                            /*!< Mailbox lock status, accessible only by CORE1 */

/* MAILBOX_INTEN */
#define MAILBOX_INTEN_MBX_IFRE               (BIT(0))                                            /*!< CORE0(CORE1) input mailbox full(empty) interrupt enable. */
#define MAILBOX_INTEN_MBX_DONE               (BIT(1))                                            /*!< CORE0(CORE1) output mailbox empty(full) interrupt enable. */
#define MAILBOX_INTEN_MBX_LINKABLE           (BIT(16))                                           /*!< CORE1 mailbox connectable interrupt enable. */

/* MAILBOX_INTST */
#define MAILBOX_INTST_MBX_IFRE               (BIT(0))                                            /*!< Input mailbox interrupt flag. */
#define MAILBOX_INTST_MBX_DONE               (BIT(1))                                            /*!< Output mailbox interrupt flag */
#define MAILBOX_INTST_MBX_LINKABLE           (BIT(16))                                           /*!< CORE1 mailbox connectable interrupt flag */

/* MAILBOX_OPT */
#define MAILBOX_OPT_MBX_NUM                  (BITS(0,3))                                         /*!< Indicates number of existing input/output mailbox pairs, Current system only has 1 mailbox pair, hardware fixed to 1 */
#define MAILBOX_OPT_MBX_SIZE                 (BITS(4,5))                                         /*!< Hardware-specified actual implementation size of each input/output mailbox */
#define MAILBOX_OPT_ACTIVE_HOST_N            (BITS(6,7))                                         /*!< Indicates number of cores that can connect to mailbox. Current system only has CORE1 */

/* MAILBOX_FROZE */
#define MAILBOX_FROZE_FROZE                  (BITS(0,31))                                        /*!< CORE0 writes 1 to lock the core number that can access MAILBOX registers */

/* MAILBOX_ID_VAL */
#define MAILBOX_ID_VAL_ID_VAL                (BIT(0))                                            /*!< Written by CORE0 with the core host_id number that can access MAILBOX registers (CORE1 core host_id is 0) */

/* MAILBOX_ID_MASK */
#define MAILBOX_ID_MASK_MASK                 (BIT(0))                                            /*!< Written by CORE0 with the core host_id mask that can access MAILBOX registers */

/* MAILBOX_MBX_KEY0 */
#define MAILBOX_MBX_KEY0_MBX_KEY0            (BITS(0,31))                                        /*!< CORE0 writes password register, password is 0xA538E827 */

/* MAILBOX_MBX_KEY1 */
#define MAILBOX_MBX_KEY1_MBX_KEY1            (BITS(0,31))                                        /*!< CORE1 writes password register, password is 0xA538EC63 */

/* MAILBOX_MESSAGE */
#define MAILBOX_MESSAGE_MSG                  (BITS(0,31))                                        /*!< 256 Bytes Mailbox Message SRAM */

/* constants definitions */
/* MAILBOX_CSR_MBX_IN_FULL */
#define MAILBOX_CSR_MBX_IN_FULL_POS          (0U)                                                /*!< MAILBOX_CSR_MBX_IN_FULL bit offset */

/* MAILBOX_CSR_MBX_OUT_FULL */
#define MAILBOX_CSR_MBX_OUT_FULL_POS         (1U)                                                /*!< MAILBOX_CSR_MBX_OUT_FULL bit offset */

/* MAILBOX_CSR_MBX_LINK */
#define MAILBOX_CSR_MBX_LINK_POS             (2U)                                                /*!< MAILBOX_CSR_MBX_LINK bit offset */

/* MAILBOX_CSR_MBX_ALB_UNLINK */
#define MAILBOX_CSR_MBX_ALB_UNLINK_POS       (3U)                                                /*!< MAILBOX_CSR_MBX_ALB_UNLINK bit offset */

/* MAILBOX_RESET_MBX_OUT_EMPTY */
#define MAILBOX_RESET_MBX_OUT_EMPTY_POS      (1U)                                                /*!< MAILBOX_RESET_MBX_OUT_EMPTY bit offset */

/* MAILBOX_RESET_MBX_UNLINK */
#define MAILBOX_RESET_MBX_UNLINK_POS         (3U)                                                /*!< MAILBOX_RESET_MBX_UNLINK bit offset */

/* MAILBOX_LINKID_ID */
#define MAILBOX_LINKID_ID_POS                (0U)                                                /*!< MAILBOX_LINKID_ID bit offset */

/* MAILBOX_LOCKOUT_MBX */
#define MAILBOX_LOCKOUT_MBX_POS              (0U)                                                /*!< MAILBOX_LOCKOUT_MBX bit offset */

/* MAILBOX_INTEN_MBX_IFRE */
#define MAILBOX_INTEN_MBX_IFRE_POS           (0U)                                                /*!< MAILBOX_INTEN_MBX_IFRE bit offset */

/* MAILBOX_INTEN_MBX_DONE */
#define MAILBOX_INTEN_MBX_DONE_POS           (1U)                                                /*!< MAILBOX_INTEN_MBX_DONE bit offset */

/* MAILBOX_INTEN_MBX_LINKABLE */
#define MAILBOX_INTEN_MBX_LINKABLE_POS       (16U)                                               /*!< MAILBOX_INTEN_MBX_LINKABLE bit offset */

/* MAILBOX_INTST_MBX_IFRE */
#define MAILBOX_INTST_MBX_IFRE_POS           (0U)                                                /*!< MAILBOX_INTST_MBX_IFRE bit offset */

/* MAILBOX_INTST_MBX_DONE */
#define MAILBOX_INTST_MBX_DONE_POS           (1U)                                                /*!< MAILBOX_INTST_MBX_DONE bit offset */

/* MAILBOX_INTST_MBX_LINKABLE */
#define MAILBOX_INTST_MBX_LINKABLE_POS       (16U)                                               /*!< MAILBOX_INTST_MBX_LINKABLE bit offset */

/* MAILBOX_OPT_MBX_NUM */
#define MAILBOX_OPT_MBX_NUM_POS              (0U)                                                /*!< MAILBOX_OPT_MBX_NUM bit offset */

/* MAILBOX_OPT_MBX_SIZE */
#define MAILBOX_OPT_MBX_SIZE_POS             (4U)                                                /*!< MAILBOX_OPT_MBX_SIZE bit offset */
#define MAILBOX_OPT_MBX_SIZE_CFG(regval)  (MAILBOX_OPT_MBX_SIZE & ((uint32_t)(regval) << MAILBOX_OPT_MBX_SIZE_POS))
#define MAILBOX_OPT_MBX_SIZE_128BYTE         (MAILBOX_OPT_MBX_SIZE_CFG(0x0))                     /*!< 128Bytes */
#define MAILBOX_OPT_MBX_SIZE_256BYTE         (MAILBOX_OPT_MBX_SIZE_CFG(0x1))                     /*!< 256Bytes (default) */
#define MAILBOX_OPT_MBX_SIZE_512BYTE         (MAILBOX_OPT_MBX_SIZE_CFG(0x2))                     /*!< 512Bytes */
#define MAILBOX_OPT_MBX_SIZE_1024BYTE        (MAILBOX_OPT_MBX_SIZE_CFG(0x3))                     /*!< 1024Bytes */

/* MAILBOX_OPT_ACTIVE_HOST_N */
#define MAILBOX_OPT_ACTIVE_HOST_N_POS        (6U)                                                /*!< MAILBOX_OPT_ACTIVE_HOST_N bit offset */

/* MAILBOX_FROZE_FROZE */
#define MAILBOX_FROZE_FROZE_POS              (0U)                                                /*!< MAILBOX_FROZE_FROZE bit offset */

/* MAILBOX_ID_VAL_ID_VAL */
#define MAILBOX_ID_VAL_ID_VAL_POS            (0U)                                                /*!< MAILBOX_ID_VAL_ID_VAL bit offset */

/* MAILBOX_ID_MASK_MASK */
#define MAILBOX_ID_MASK_MASK_POS             (0U)                                                /*!< MAILBOX_ID_MASK_MASK bit offset */

/* MAILBOX_MBX_KEY0_MBX_KEY0 */
#define MAILBOX_MBX_KEY0_MBX_KEY0_POS        (0U)                                                /*!< MAILBOX_MBX_KEY0_MBX_KEY0 bit offset */

/* MAILBOX_MBX_KEY1_MBX_KEY1 */
#define MAILBOX_MBX_KEY1_MBX_KEY1_POS        (0U)                                                /*!< MAILBOX_MBX_KEY1_MBX_KEY1 bit offset */

/* MAILBOX_MESSAGE_MSG */
#define MAILBOX_MESSAGE_MSG_POS              (0U)                                                /*!< MAILBOX_MESSAGE_MSG bit offset */

#ifdef __cplusplus
}
#endif

#endif /* _RN831x_RN861x_MCU_MAILBOX_H */
