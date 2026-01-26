/**
 ******************************************************************************
 * @file   port.h
 * @author Renergy Software Team
 * @brief  Partial implementation of kernel compatilbility between M0 and RISCV core.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

#ifndef _PORT_H_
#define _PORT_H_

#include "nmsis_core.h"

#if !defined(UNUSED)
#define UNUSED(X) (void)X /* To avoid gcc/g++ warnings */
#endif /* UNUSED */

#define NVIC_EnableIRQ        __ECLIC_EnableIRQ
#define NVIC_GetEnableIRQ     __ECLIC_GetEnableIRQ
#define NVIC_DisableIRQ       __ECLIC_DisableIRQ

#define NVIC_SetPendingIRQ    __ECLIC_SetPendingIRQ
#define NVIC_GetPendingIRQ    __ECLIC_GetPendingIRQ
#define NVIC_ClearPendingIRQ  __ECLIC_ClearPendingIRQ

/**
    \brief   Set Interrupt Priority
    \details Sets the priority of a device specific interrupt or a processor exception.
            The interrupt number can be positive to specify a device specific interrupt,
            or negative to specify a processor exception.
    \param [in]      IRQn  Interrupt number.
    \param [in]  priority  Priority to set.
    \note    The priority cannot be set for every processor exception.
 */
__STATIC_INLINE void NVIC_SetPriority(IRQn_Type IRQn, uint32_t priority)
{
    uint8_t cur_ctrl = __ECLIC_GetCtrlIRQ(IRQn);
    __ECLIC_SetCtrlIRQ(IRQn, (cur_ctrl | (uint8_t)priority));
}

/**
    \brief   Get Interrupt Priority
    \details Reads the priority of a device specific interrupt or a processor exception.
            The interrupt number can be positive to specify a device specific interrupt,
            or negative to specify a processor exception.
    \param [in]   IRQn  Interrupt number.
    \return             Interrupt Priority.
                        Value is aligned automatically to the implemented priority bits of the microcontroller.
 */
__STATIC_INLINE uint32_t NVIC_GetPriority(IRQn_Type IRQn)
{
    return (uint32_t)(__ECLIC_GetCtrlIRQ(IRQn) - ((1U << (8U - __ECLIC_INTCTLBITS)) - 1U)); /* only 3bits valid */
}

/**
    \brief   Encode Priority
    \details Encodes the priority for an interrupt with the given priority group,
            preemptive priority value, and subpriority value.
            In case of a conflict between priority grouping and available
            priority bits (__NVIC_PRIO_BITS), the smallest possible priority group is set.
    \param [in]     PriorityGroup  Used priority group.
    \param [in]   PreemptPriority  Preemptive priority value (starting from 0).
    \param [in]       SubPriority  Subpriority value (starting from 0).
    \return                        Encoded priority. Value can be used in the function \ref NVIC_SetPriority().
 */
__STATIC_INLINE uint32_t NVIC_EncodePriority(uint32_t PriorityGroup, uint32_t PreemptPriority, uint32_t SubPriority)
{
    uint8_t nlbits            = __ECLIC_GetCfgNlbits(); /* reg: cliccfg */
    uint8_t intctlbits        = (uint8_t)__ECLIC_INTCTLBITS; /* reg: clicinfo */
    UNUSED(PriorityGroup);
    if (nlbits == 0) {
        return 0;
    }
    if (nlbits > intctlbits) {
        nlbits = intctlbits;
    }
    uint32_t PreemptPriorityBits = nlbits; /* level */
    uint32_t SubPriorityBits     = (intctlbits - nlbits); /* priority */

    return (((PreemptPriority & (uint32_t)((1UL << (PreemptPriorityBits)) - 1UL)) << (8U - nlbits)) |
            ((SubPriority     & (uint32_t)((1UL << (SubPriorityBits    )) - 1UL)) << (8U - intctlbits)));
}

/**
    \brief   Decode Priority
    \details Decodes an interrupt priority value with a given priority group to
            preemptive priority value and subpriority value.
            In case of a conflict between priority grouping and available
            priority bits (__NVIC_PRIO_BITS) the smallest possible priority group is set.
    \param [in]         Priority   Priority value, which can be retrieved with the function \ref NVIC_GetPriority().
    \param [in]     PriorityGroup  Used priority group.
    \param [out] pPreemptPriority  Preemptive priority value (starting from 0).
    \param [out]     pSubPriority  Subpriority value (starting from 0).
 */
__STATIC_INLINE void NVIC_DecodePriority(uint32_t Priority, uint32_t PriorityGroup, uint32_t* const pPreemptPriority, uint32_t* const pSubPriority)
{
    uint8_t nlbits            = __ECLIC_GetCfgNlbits(); /* reg: cliccfg */
    uint8_t intctlbits        = (uint8_t)__ECLIC_INTCTLBITS; /* reg: clicinfo */
    UNUSED(PriorityGroup);
    if (nlbits == 0) {
        return;
    }
    if (nlbits > intctlbits) {
        nlbits = intctlbits;
    }
    uint32_t PreemptPriorityBits = nlbits; /* level */
    uint32_t SubPriorityBits     = (intctlbits - nlbits); /* priority */

    *pPreemptPriority = (Priority >> (8U - nlbits)    ) & (uint32_t)((1UL << (PreemptPriorityBits)) - 1UL);
    *pSubPriority     = (Priority >> (8U - intctlbits)) & (uint32_t)((1UL << (SubPriorityBits    )) - 1UL);
}

#define NVIC_SetVector   __ECLIC_SetVector
#define NVIC_GetVector   __ECLIC_GetVector

/**
    \brief   Set Process Stack Pointer
    \details Assigns the given value to the Process Stack Pointer (PSP).
    \param [in]    topOfProcStack  Process Stack Pointer value to set
 */
__STATIC_INLINE void __set_PSP(uint32_t topOfProcStack)
{
    __ASM volatile("mv tp, %0" ::"r"(topOfProcStack));
}

/**
    \brief   Get Process Stack Pointer
    \details Returns the current value of the Process Stack Pointer (PSP).
    \return               PSP Register value
 */
__STATIC_INLINE uint32_t __get_PSP(void)
{
    register uint32_t __regProcessStackPointer;
    __ASM volatile("mv %0, tp" : "=r"(__regProcessStackPointer));
    return (__regProcessStackPointer);
}

/**
    \brief   Set Main Stack Pointer
    \details Assigns the given value to the Main Stack Pointer (MSP).
    \param [in]    topOfMainStack  Main Stack Pointer value to set
 */
__STATIC_INLINE void __set_MSP(uint32_t topOfMainStack)
{
    __ASM volatile("mv sp, %0" ::"r"(topOfMainStack));
}

/**
    \brief   Get Main Stack Pointer
    \details Returns the current value of the Main Stack Pointer (MSP).
    \return               MSP Register value
 */
__STATIC_INLINE uint32_t __get_MSP(void)
{
    register uint32_t __regMainStackPointer;
    __ASM volatile("mv %0, sp" : "=r"(__regMainStackPointer));
    return(__regMainStackPointer);
}

/**
    \brief   Set Priority Mask
    \details Assigns the given value to the Priority Mask Register.
    \param [in]    priMask  Priority Mask
 */
__STATIC_INLINE void __set_PRIMASK(uint32_t priMask)
{
    if (priMask == 0U) {
        __disable_irq();
    } else {
        __enable_irq();
    }
}

/**
    \brief   Get Priority Mask
    \details Returns the current state of the priority mask bit from the Priority Mask Register.
    \return               Priority Mask value
 */
__STATIC_INLINE uint32_t __get_PRIMASK(void)
{
    return ((__RV_CSR_READ(CSR_MSTATUS) & MSTATUS_MIE) >> 3U);
}

#define NVIC_SystemReset SysTimer_SoftwareReset

#endif
