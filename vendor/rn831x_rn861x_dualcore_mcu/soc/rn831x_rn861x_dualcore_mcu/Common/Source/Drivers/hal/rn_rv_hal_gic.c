/**
 ******************************************************************************
 * @file   rn_rv_hal_gic.c
 * @author Renergy Software Team
 * @brief  GIC HAL module driver.
 *          This file provides firmware functions to manage the following
 *          functionalities of the internal GIC memory:
 *           + Program operations functions
 *           + Memory Control functions
 *           + Peripheral State functions
 *
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_hal.h"

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @defgroup GIC GIC
 * @brief     GIC driver modules
 * @{
 */

#ifdef HAL_GIC_MODULE_ENABLED

/* Private typedef ----------------------------------------------------------*/
/** @defgroup GIC_Private_Typedef GIC Private Typedef
 * @{
 */
/**
 * @}
 */

/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @defgroup GIC_Exported_Functions GIC Exported Functions
 * @{
 */

/** @defgroup GIC_Exported_Functions_Group1 Initialization and de-initialization functions
 *  @brief    Initialization and Configuration functions
 *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
    [..]
      This subsection provides a set of functions allowing to control the GIC
      operations.

@endverbatim
 * @{
 */

/**
 * @brief  Initializes the GIC ICI
 * @param  Shv         ECLIC_NON_VECTOR_INTERRUPT means non-vector mode, and ECLIC_VECTOR_INTERRUPT is vector mode
 * @param  TriggerMode ECLIC Trigger Enum for different Trigger Type
 * @param  Level       Interupt level
 * @param  Priority    Interrupt priority
 * @retval None
 */
HAL_StatusTypeDef HAL_GIC_ICI_Init(uint8_t Shv, ECLIC_TRIGGER_Type TriggerMode, uint8_t Level, uint8_t Priority)
{
    /* enable GIC clock */
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_GIC_CLK_ENABLE();
    __HAL_SYSTEM_PSW_LOCK();

    /* clear flags */
    __HAL_GIC_ICI_CLEAR_FLAG(GIC_ICI_FLAG_FROM_CORE0);
    __HAL_GIC_ICI_CLEAR_FLAG(GIC_ICI_FLAG_FROM_CORE1);

    /* Initialize INTERCORE interrupt */
    if (ECLIC_Register_IRQ(INTERCORE_IRQn, Shv, TriggerMode, Level, Priority, NULL) != HAL_OK) {
        return HAL_ERROR;
    }
    /* enable global interrupt */
    __enable_irq();

    return HAL_OK;
}

/**
 * @brief  DeInitialize the ICI
 *
 * @retval None
 */
void HAL_GIC_ICI_DeInit(void)
{
    /* clear flags */
    __HAL_GIC_ICI_CLEAR_FLAG(GIC_ICI_FLAG_FROM_CORE0);
    __HAL_GIC_ICI_CLEAR_FLAG(GIC_ICI_FLAG_FROM_CORE1);

    /* Disable GIC clock */
    __HAL_SYSTEM_PSW_UNLOCK();
    __HAL_SYSTEM_GIC_CLK_DISABLE();
    __HAL_SYSTEM_PSW_LOCK();

    /* Disable interrupt */
    __ECLIC_DisableIRQ(INTERCORE_IRQn);
}


/**
 * @}
 */


/** @defgroup GIC_Exported_Functions_Group2 Peripheral Control functions
 *  @brief     management functions.
 *
@verbatim
================================================================================
                    ##### Peripheral Control functions #####
================================================================================
    [..]
      This subsection provides a set of functions allowing to control the GIC
      operations.

@endverbatim
 * @{
 */

/**
 * @brief  Generate a inter-core interrupt (ICI) to the core0 or core1,
 *         and Transmit an 8bits of data.
 * @param  CoreID Specify the core0 (=0) or core1(=1) to send message.
 *         @note   Only 0 and 1.
 * @param  Data   Specify the one Byte to send.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_GIC_ICI_Transmit(uint8_t CoreID, uint8_t Data)
{
    /* Check the parameters */
    if ((CoreID != 0x0) && (CoreID != 0x1)) {
        return HAL_ERROR;
    }

    LL_ICI_GenerateInterCoreInterrupt(GIC, CoreID, Data);

    return HAL_OK;
}

/**
 * @brief  Receives a inter-core interrupt (ICI) Data from the core0(=0) or core1(=1).
 * @param  CoreID Specify the core0 (=0) or core1(=1) to Receive message.
 *         @note   Only 0 and 1.
 * @param   pData Pointer to data buffer for Receive.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_GIC_ICI_Receive(uint8_t CoreID, uint8_t *pData)
{
    /* get current core is CORE0 or CORE1 */
    uint32_t currentcore = HAL_GetCoreId();

    /* Check the parameters */
    if ((CoreID != 0x0) && (CoreID != 0x1)) {
        return HAL_ERROR;
    }

    if (currentcore == 0x0U) {
        /* current core is CORE0, get message from core0 */
        if (CoreID == 0x0U){
            *pData = LL_ICI_GetCore0ToCore0Message();
        }
        /* current core is CORE0, get message from core1 */
        else {
            *pData = LL_ICI_GetCore1ToCore0Message();
        }
    } else if (currentcore == 0x1U) {
        /* current core is CORE1, get message from core0 */
        if (CoreID == 0x0U){
            *pData = LL_ICI_GetCore0ToCore1Message();
        }
        /* current core is CORE1, get message from core1 */
        else {
            *pData = LL_ICI_GetCore1ToCore1Message();
        }
    } else {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief  This function handles ICI event interrupt request.
 * @note   This API should be called under the INTER_CORE_HANDLER().
 * @retval None
 */
void HAL_GIC_ICI_IRQHandler(void)
{
#ifdef CORE0
    /* from core0 to core0 */
    if (__HAL_GIC_ICI_GET_FLAG(GIC_ICI_FLAG_FROM_CORE0) != RESET) {
        /* Core0 To Core0 ICI interrupt user callback */
        HAL_GIC_ICI_Core0ToCore0Callback();

        /* Clear Core0 To Core0 ICI pending bit */
        __HAL_GIC_ICI_CLEAR_FLAG(GIC_ICI_FLAG_FROM_CORE0);
    }

    /* from core1 to core0 */
    if (__HAL_GIC_ICI_GET_FLAG(GIC_ICI_FLAG_FROM_CORE1) != RESET) {
        /* Core1 To Core0 ICI interrupt user callback */
        HAL_GIC_ICI_Core1ToCore0Callback();

        /* Clear Core1 To Core0 ICI pending bit */
        __HAL_GIC_ICI_CLEAR_FLAG(GIC_ICI_FLAG_FROM_CORE1);
    }
#endif
#ifdef CORE1
    /* from core0 to core1 */
    if (__HAL_GIC_ICI_GET_FLAG(GIC_ICI_FLAG_FROM_CORE0) != RESET) {
        /* Core0 To Core1 ICI interrupt user callback */
        HAL_GIC_ICI_Core0ToCore1Callback();

        /* Clear Core0 To Core1 ICI pending bit */
        __HAL_GIC_ICI_CLEAR_FLAG(GIC_ICI_FLAG_FROM_CORE0);
    }

    /* from core1 to core1 */
    if (__HAL_GIC_ICI_GET_FLAG(GIC_ICI_FLAG_FROM_CORE1) != RESET) {
        /* Core1 To Core1 ICI interrupt user callback */
        HAL_GIC_ICI_Core1ToCore1Callback();

        /* Clear Core1 To Core1 ICI pending bit */
        __HAL_GIC_ICI_CLEAR_FLAG(GIC_ICI_FLAG_FROM_CORE1);
    }
#endif
}

/**
 * @brief  Core0 Send To Core0 interrupt callback
 * @retval none
 */
__WEAK void HAL_GIC_ICI_Core0ToCore0Callback(void)
{
    /* NOTE : This function Should not be modified, when the callback is needed,
        the HAL_GIC_ICI_Core0ToCore0Callback could be implemented in the user file
    */
}

/**
 * @brief  Core1 Send To Core0 interrupt callback
 * @retval none
 */
__WEAK void HAL_GIC_ICI_Core1ToCore0Callback(void)
{
    /* NOTE : This function Should not be modified, when the callback is needed,
        the HAL_GIC_ICI_Core1ToCore0Callback could be implemented in the user file
    */
}

/**
 * @brief  Core0 Send To Core1 interrupt callback
 * @retval none
 */
__WEAK void HAL_GIC_ICI_Core0ToCore1Callback(void)
{
    /* NOTE : This function Should not be modified, when the callback is needed,
        the HAL_GIC_ICI_Core0ToCore1Callback could be implemented in the user file
    */
}

/**
 * @brief  Core1 Send To Core1 interrupt callback
 * @retval none
 */
__WEAK void HAL_GIC_ICI_Core1ToCore1Callback(void)
{
    /* NOTE : This function Should not be modified, when the callback is needed,
        the HAL_GIC_ICI_Core1ToCore1Callback could be implemented in the user file
    */
}

/**
 * @}
 */


/** @defgroup GIC_Exported_Functions_Group3 Set and Get IRQ target functions
 *  @brief     Set and Get IRQ target functions
 *
@verbatim
@endverbatim
 * @{
 */

/**
 * @brief  Set the GIC's SOC_INTn_INDICATOR register for the given interrupt.
 * @param  IRQn        Interrupt ID to Set the configuration for.
 * @param  CoreTarget  Specify the core to Set Interrupt to @ref HAL_GIC_IRQTargetTypeDef.
 *
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_GIC_SetIrqTargetCore(IRQn_Type IRQn, HAL_GIC_IRQTargetTypeDef CoreTarget)
{
    if (IRQn >= IRQn_MAX) {
        return HAL_ERROR;
    }

    switch (CoreTarget)
    {
        case HAL_GIC_IRQ_TARGET_NONE:  /* not set irq to core0 or core1 */
        {
            LL_GIC_UnassignIrqToCore0((uint32_t)IRQn);
            LL_GIC_UnassignIrqToCore1((uint32_t)IRQn);
            break;
        }
        case HAL_GIC_IRQ_TARGET_CORE0:  /* set irq to core0 */
        {
            /* Only Exclusive to Core1 */
            if ((IRQn == MAILBOXHOST0_IRQn) || (IRQn == CORE1_SYSREG_IRQn) ||
                (IRQn == GDMA11_IRQn) || (IRQn == GDMA10_IRQn)) {
                return HAL_ERROR;
            }
            LL_GIC_AssignIrqToCore0((uint32_t)IRQn);
            LL_GIC_UnassignIrqToCore1((uint32_t)IRQn);
            break;
        }
        case HAL_GIC_IRQ_TARGET_CORE1:  /* set irq to core1 */
        {
            /* Only Exclusive to Core0 */
            if ((IRQn == MAILBOXINTER_IRQn) || (IRQn == CORE0_SYSREG_IRQn) || (IRQn == CORE1_RST_IRQn)
                || (IRQn == GDMA01_IRQn) || (IRQn == GDMA00_IRQn) || (IRQn == SCU_IRQn)
                || (IRQn == LCSS_IRQn)) {
                return HAL_ERROR;
            }
            LL_GIC_UnassignIrqToCore0((uint32_t)IRQn);
            LL_GIC_AssignIrqToCore1((uint32_t)IRQn);
            break;
        }
        case HAL_GIC_IRQ_TARGET_CORE0_CORE1:  /* set irq to core0 and core1 */
        {
            /* Only Exclusive to Core1 */
            if ((IRQn != MAILBOXHOST0_IRQn) && (IRQn != CORE1_SYSREG_IRQn) &&
                (IRQn != GDMA11_IRQn) && (IRQn != GDMA10_IRQn)) {
                    LL_GIC_AssignIrqToCore0((uint32_t)IRQn);
            }
            /* Only Exclusive to Core0 */
            if ((IRQn != MAILBOXINTER_IRQn) && (IRQn != CORE0_SYSREG_IRQn) && (IRQn != CORE1_RST_IRQn)
                && (IRQn != GDMA01_IRQn) && (IRQn != GDMA00_IRQn) && (IRQn != SCU_IRQn)
                && (IRQn != LCSS_IRQn)) {
                LL_GIC_AssignIrqToCore1((uint32_t)IRQn);
            }
            break;
        }
        default:  /* return 0 default */
        {
            return HAL_ERROR;
            break;
        }
    }

    return HAL_OK;
}

/**
 * @brief  Get the GIC's SOC_INTn_INDICATOR register for the given interrupt.
 * @param  IRQn    Interrupt ID to Set the configuration for.
 * @retval Interrupt target core @ref HAL_GIC_IRQTargetTypeDef
 */
HAL_GIC_IRQTargetTypeDef HAL_GIC_GetIrqTargetCore(IRQn_Type IRQn)
{
    uint32_t istocore0, istocore1;

    istocore0 = LL_GIC_IsAssignedIrqToCore0(IRQn);
    istocore1 = LL_GIC_IsAssignedIrqToCore1(GIC, IRQn);

    if ((istocore0 == 0x0U) && (istocore1 == 0x0U)) {
        return HAL_GIC_IRQ_TARGET_NONE;
    } else if ((istocore0 == 0x1U) && (istocore1 == 0x0U)) {
        return HAL_GIC_IRQ_TARGET_CORE0;
    } else if ((istocore0 == 0x0U) && (istocore1 == 0x1U)) {
        return HAL_GIC_IRQ_TARGET_CORE1;
    } else {
        return HAL_GIC_IRQ_TARGET_CORE0_CORE1;
    }
}

/**
 * @brief  Set the All GIC's SOC_INTn_INDICATOR register for the given interrupt.
 * @param  CoreTarget  Specify the core to Set Interrupt to @ref HAL_GIC_IRQTargetTypeDef.
 *
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_GIC_SetAllIrqTargetCore(HAL_GIC_IRQTargetTypeDef CoreTarget)
{
    uint32_t index;

    switch (CoreTarget)
    {
        case HAL_GIC_IRQ_TARGET_NONE:  /* not set irq to core0 or core1 */
        {
            for (index = LCD_IRQn; index < IRQn_MAX; index++) {
                LL_GIC_UnassignIrqToCore0(index);
                LL_GIC_UnassignIrqToCore1(index);
            }
            break;
        }
        case HAL_GIC_IRQ_TARGET_CORE0:  /* set irq to core0 */
        {
            for (index = LCD_IRQn; index < IRQn_MAX; index++) {
                /* Only Exclusive to Core1 */
                if ((index == MAILBOXHOST0_IRQn) || (index == CORE1_SYSREG_IRQn) ||
                (index == GDMA11_IRQn) || (index == GDMA10_IRQn)) {
                    continue;
                }
                LL_GIC_AssignIrqToCore0(index);
                LL_GIC_UnassignIrqToCore1(index);
            }
            break;
        }
        case HAL_GIC_IRQ_TARGET_CORE1:  /* set irq to core1 */
        {
            for (index = LCD_IRQn; index < IRQn_MAX; index++) {
                /* Only Exclusive to Core0 */
                if ((index == MAILBOXINTER_IRQn) || (index == CORE0_SYSREG_IRQn) || (index == CORE1_RST_IRQn)
                    || (index == GDMA01_IRQn) || (index == GDMA00_IRQn) || (index == SCU_IRQn)
                    || (index == LCSS_IRQn)) {
                    continue;
                }
                LL_GIC_UnassignIrqToCore0(index);
                LL_GIC_AssignIrqToCore1(index);
            }
            break;
        }
        case HAL_GIC_IRQ_TARGET_CORE0_CORE1:  /* set irq to core0 and core1 */
        {
            for (index = LCD_IRQn; index < IRQn_MAX; index++) {
                /* Only Exclusive to Core1 */
                if ((index != MAILBOXHOST0_IRQn) && (index != CORE1_SYSREG_IRQn) &&
                (index != GDMA11_IRQn) && (index != GDMA10_IRQn)) {
                    LL_GIC_AssignIrqToCore0(index);
                }
                /* Only Exclusive to Core0 */
                if ((index != MAILBOXINTER_IRQn) && (index != CORE0_SYSREG_IRQn) && (index != CORE1_RST_IRQn)
                    && (index != GDMA01_IRQn) && (index != GDMA00_IRQn) && (index != SCU_IRQn)
                    && (index != LCSS_IRQn)) {
                    LL_GIC_AssignIrqToCore1(index);
                }
            }
            break;
        }
        default:  /* return 0 default */
        {
            return HAL_ERROR;
            break;
        }
    }

    return HAL_OK;
}

/**
 * @}
 */

/**
 * @}
 */

#endif /* HAL_GIC_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
