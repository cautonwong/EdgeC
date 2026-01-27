/**
 ******************************************************************************
 * @file   rn_rv_ll_gpio.c
 * @author Renergy Software Team
 * @brief  GPIO LL module driver
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

/** @defgroup GPIO GPIO
 * @brief GPIO HAL module driver
 * @{
 */

#ifdef HAL_GPIO_MODULE_ENABLED

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/** @addtogroup GPIO_Private_Constants GPIO Private Constants
 * @{
 */
#define GPIO_NUMBER           16U
/**
 * @}
 */

/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/* Private functions --------------------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @defgroup GPIO_Exported_Functions GPIO Exported Functions
 * @{
 */

/** @defgroup GPIO_Exported_Functions_Group1 Initialization and de-initialization functions
 *  @brief    Initialization and Configuration functions
 *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
  [..]
    This section provides functions allowing to initialize and de-initialize the GPIOs
    to be ready for use.

@endverbatim
 * @{
 */

/**
 * @brief  Initializes the GPIOx peripheral according to the specified parameters in the GPIO_Init.
 * @note   PL00~PL03 are VBAT IO, configuration priority: VBAT > GPIO, if config PL00~PL03 as VBAT funciton, the GPIO
 *         configuration will be disabled.
 * @note   PL01 (VBAT power domain) IO function priority: MADC IO Function > RTC IO function > Gernal GPIO
 * @note   GPIOH only support GPIO_PIN_0 ~ GPIO_PIN_1
 *         GPIOL only support GPIO_PIN_0 ~ GPIO_PIN_3
 * @param  GPIOx where x can be (A..H, L) to select the GPIO peripheral
 * @param  GPIO_Init pointer to a GPIO_InitTypeDef structure that contains
 *         the configuration information for the specified GPIO peripheral.
 * @retval None
 */
void HAL_GPIO_Init(GPIO_TypeDef  *GPIOx, GPIO_InitTypeDef *GPIO_Init)
{
    uint32_t position;
    uint32_t ioposition = 0x00U;
    uint32_t iocurrent = 0x00U;

    /* Check the parameters */
    assert_param(IS_GPIO_ALL_INSTANCE(GPIOx));
    assert_param(IS_GPIO_PIN(GPIO_Init->Pin));
    assert_param(IS_GPIO_MODE(GPIO_Init->Mode));

    /* Configure the port pins */
    for(position = 0U; position < GPIO_NUMBER; position++)
    {
        /* Get the IO position */
        ioposition = 0x01U << position;
        /* Get the current IO position */
        iocurrent = (uint32_t)(GPIO_Init->Pin) & ioposition;

        if(iocurrent == ioposition)
        {
            /*--------------------- GPIO Mode Configuration ------------------------*/
            /* In case of Output or Alternate function mode selection */
            if(((GPIO_Init->Mode) == GPIO_MODE_OUTPUT) || ((GPIO_Init->Mode) == GPIO_MODE_AF))
            {
                /* Check the Speed parameter */
                assert_param(IS_GPIO_SPEED(GPIO_Init->Speed));
                /* Configure the IO Speed */
                LL_GPIO_SetPinSpeed(GPIOx, iocurrent, GPIO_Init->Speed);
                /* Check the Speed parameter */
                assert_param(IS_GPIO_SPEED(GPIO_Init->OutputType));
                /* Configure the IO Output Type */
                LL_GPIO_SetPinOutputType(GPIOx, iocurrent, GPIO_Init->OutputType);
            }

            if((GPIO_Init->Mode) != GPIO_MODE_ANALOG)
            {
                /* Check the Speed parameter */
                assert_param(IS_GPIO_SPEED(GPIO_Init->InputType));
                /* Configure the IO Input Type */
                LL_GPIO_SetPinInputType(GPIOx, iocurrent, GPIO_Init->InputType);
            }

            if(((GPIO_Init->Mode) == GPIO_MODE_INPUT) || ((GPIO_Init->Mode) == GPIO_MODE_AF))
            {
                /* Check the parameters */
                assert_param(IS_GPIO_PULL(GPIO_Init->Pull));
                /* Activate the Pull-up or Pull down resistor for the current IO */
                LL_GPIO_SetPinPull(GPIOx, iocurrent, GPIO_Init->Pull);
            }

            /* In case of Alternate function mode selection */
            if((GPIO_Init->Mode) == GPIO_MODE_AF)
            {
                /* Check the Alternate function parameter */
                assert_param(IS_GPIO_AF(GPIO_Init->Alternate));
                if (position < 8U)
                {
                    /* Configure Alternate function mapped with the current IO */
                    LL_GPIO_SetAFPin_0_7(GPIOx, iocurrent, GPIO_Init->Alternate);
                }
                else
                {
                    LL_GPIO_SetAFPin_8_15(GPIOx, iocurrent, GPIO_Init->Alternate);
                }
            }

            /* Configure IO Direction mode */
            LL_GPIO_SetPinMode(GPIOx, iocurrent, GPIO_Init->Mode);
        }
    }
}

/**
 * @brief  De-initializes the GPIOx peripheral registers to their default reset values.
 * @param  GPIOx where x can be (A..H, L) to select the GPIO peripheral.
 * @param  GPIO_Pin specifies the port bit to be written.
 *          This parameter can be one of GPIO_PIN_x where x can be (0..15).
 * @retval None
 */
void HAL_GPIO_DeInit(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin)
{
    uint32_t position;
    uint32_t ioposition = 0x0U;
    uint32_t iocurrent = 0x0U;

    /* Check the parameters */
    assert_param(IS_GPIO_ALL_INSTANCE(GPIOx));

    /* Configure the port pins */
    for(position = 0U; position < GPIO_NUMBER; position++)
    {
        /* Get the IO position */
        ioposition = 0x01U << position;
        /* Get the current IO position */
        iocurrent = (GPIO_Pin) & ioposition;

        if(iocurrent == ioposition)
        {
            /*------------------------- GPIO Mode Configuration --------------------*/
            /* Configure IO Direction in Input Floating Mode */
            LL_GPIO_SetPinMode(GPIOx, iocurrent, GPIO_MODE_ANALOG);

            /* Deactivate the Pull-up and Pull-down resistor for the current IO */
            LL_GPIO_SetPinPull(GPIOx, iocurrent, GPIO_NOPULL);

            /* Configure the default value IO Input Type */
            LL_GPIO_SetPinInputType(GPIOx, iocurrent, GPIO_INPUT_CMOS);

            /* Configure the default value IO Output Type */
            LL_GPIO_SetPinOutputType(GPIOx, iocurrent, GPIO_OUTPUT_PP);

            /* Configure the default value for IO Speed */
            LL_GPIO_SetPinSpeed(GPIOx, iocurrent, GPIO_SPEED_FREQ_NORMAL);

            /* Configure the default Alternate Function in current IO */
            if(position < 8U)
            {
                LL_GPIO_SetAFPin_0_7(GPIOx, iocurrent, GPIO_AF0_SYS);
            }
            else
            {
                LL_GPIO_SetAFPin_8_15(GPIOx, iocurrent, GPIO_AF0_SYS);
            }
        }
    }
}

/**
 * @}
 */

/** @defgroup GPIO_Exported_Functions_Group2 IO operation functions
 *  @brief   GPIO Read and Write
 *
@verbatim
 ===============================================================================
                       ##### IO operation functions #####
 ===============================================================================

@endverbatim
 * @{
 */

/**
 * @brief  Reads the specified input port pin.
 * @param  GPIOx where x can be (A..H, L) to select the GPIO peripheral.
 * @param  GPIO_Pin specifies the port bit to read.
 *         This parameter can be GPIO_PIN_x where x can be (0..15).
 * @retval The input port pin value.
 */
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    GPIO_PinState bitstatus;

    /* Check the parameters */
    assert_param(IS_GPIO_PIN(GPIO_Pin));

    if((LL_GPIO_ReadInputPort(GPIOx) & GPIO_Pin) != (uint32_t)GPIO_PIN_RESET)
    {
        bitstatus = GPIO_PIN_SET;
    }
    else
    {
        bitstatus = GPIO_PIN_RESET;
    }
    return bitstatus;
}


/**
 * @brief  Sets or clears the selected data port bit.
 * @note   This function uses GPIOx_BSRR register to allow atomic read/modify
 *         accesses. In this way, there is no risk of an IRQ occurring between
 *         the read and the modify access.
 *
 * @param  GPIOx where x can be (A..H, L) to select the GPIO peripheral.
 * @param  GPIO_Pin specifies the port bit to be written.
 *          This parameter can be one of GPIO_PIN_x where x can be (0..15).
 * @param  PinState specifies the value to be written to the selected bit.
 *          This parameter can be one of the GPIO_PinState enum values:
 *            @arg GPIO_PIN_RESET: to clear the port pin
 *            @arg GPIO_PIN_SET: to set the port pin
 * @retval None
 */
void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
    /* Check the parameters */
    assert_param(IS_GPIO_PIN(GPIO_Pin));
    assert_param(IS_GPIO_PIN_ACTION(PinState));

    if(PinState != GPIO_PIN_RESET)
    {
        LL_GPIO_SetOutputPin(GPIOx, GPIO_Pin);
    }
    else
    {
        LL_GPIO_ResetOutputPin(GPIOx, GPIO_Pin);
    }
}

/**
 * @brief  Toggles the specified GPIO pins.
 * @param  GPIOx where x can be (A..H, L) to select the GPIO peripheral.
 * @param  GPIO_Pin Specifies the pins to be toggled.
 * @retval None
 */
void HAL_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    /* Check the parameters */
    assert_param(IS_GPIO_PIN(GPIO_Pin));

    LL_GPIO_TogglePin(GPIOx, GPIO_Pin);
}

/**
 * @}
 */

/** @defgroup GPIO_Exported_Functions_Group3 enable functions
 *  @brief   GPIO Analog and force input function
 *
@verbatim
 ===============================================================================
            ##### Enable Analog or force input or JTAG functions #####
 ===============================================================================

@endverbatim
 * @{
 */

/**
 * @brief  Enable analog function.
 * @param  GPIOx where x can be (A..H, L) to select the GPIO peripheral.
 * @param  GPIO_Pin specifies the port bit to read.
 *         This parameter can be GPIO_PIN_x where x can be (0..15).
 * @retval None
 */
void HAL_GPIO_EnableAnalog(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    /* Check the parameters */
    assert_param(IS_GPIO_PIN(GPIO_Pin));

    LL_GPIO_EnablePinAnalog(GPIOx, GPIO_Pin);
}

/**
 * @brief  Disable analog function.
 * @param  GPIOx where x can be (A..H, L) to select the GPIO peripheral.
 * @param  GPIO_Pin specifies the port bit to read.
 *         This parameter can be GPIO_PIN_x where x can be (0..15).
 * @retval None
 */
void HAL_GPIO_DisableAnalog(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    /* Check the parameters */
    assert_param(IS_GPIO_PIN(GPIO_Pin));

    LL_GPIO_DisablePinAnalog(GPIOx, GPIO_Pin);
}

/**
 * @brief  Enable force input function.
 * @param  GPIOx where x can be (A..H, L) to select the GPIO peripheral.
 * @param  GPIO_Pin specifies the port bit to read.
 *         This parameter can be GPIO_PIN_x where x can be (0..15).
 * @retval None
 */
void HAL_GPIO_EnableForceInput(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    /* Check the parameters */
    assert_param(IS_GPIO_PIN(GPIO_Pin));

    LL_GPIO_EnablePinForceInput(GPIOx, GPIO_Pin);
}

/**
 * @brief  Disable force input function.
 * @param  GPIOx where x can be (A..H, L) to select the GPIO peripheral.
 * @param  GPIO_Pin specifies the port bit to read.
 *         This parameter can be GPIO_PIN_x where x can be (0..15).
 * @retval None
 */
void HAL_GPIO_DisableForceInput(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    /* Check the parameters */
    assert_param(IS_GPIO_PIN(GPIO_Pin));

    LL_GPIO_DisablePinForceInput(GPIOx, GPIO_Pin);
}

/**
 * @brief  Enable JTAG pin.
 * @param  GPIO_DBGx GPIO_DBG instance.
 * @param  JTAG_PIN Select GPIO as JTAG pin.
 *         This parameter can be a combination of the GPIO_JTAG_pin_define values:
 *            @arg GPIO_JTCK: select PG14 for JTCK
 *            @arg GPIO_JTMS: select PG15 for JTMS
 *            @arg GPIO_JTDI: select PG11 for JTDI
 *            @arg GPIO_JTDO: select PG10 for JTDO
 * @retval None
 */
void HAL_GPIO_EnableJTAG(GPIO_DBG_TypeDef *GPIO_DBGx, uint32_t JTAG_PIN)
{
    /* Check the parameters */
    assert_param(IS_GPIO_JTAG_PIN(JTAG_PIN));
    /* Enable JTAG pin */
    if((JTAG_PIN & GPIO_DBG_JTCK_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_EnablePinJTCK(GPIO_DBGx);
    }
    if((JTAG_PIN & GPIO_DBG_JTMS_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_EnablePinJTMS(GPIO_DBGx);
    }
    if((JTAG_PIN & GPIO_DBG_JTDI_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_EnablePinJTDI(GPIO_DBGx);
    }
    if((JTAG_PIN & GPIO_DBG_JTDO_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_EnablePinJTDO(GPIO_DBGx);
    }
}

/**
 * @brief  Disable JTAG pin.
 * @param  GPIO_DBGx GPIO_DBG instance.
 * @param  JTAG_PIN Select GPIO as normal IO.
 *         This parameter can be a combination of the GPIO_JTAG_pin_define values:
 *            @arg GPIO_JTCK: select PG14 for JTCK
 *            @arg GPIO_JTMS: select PG15 for JTMS
 *            @arg GPIO_JTDI: select PG11 for JTDI
 *            @arg GPIO_JTDO: select PG10 for JTDO
 * @retval None
 */
void HAL_GPIO_DisableJTAG(GPIO_DBG_TypeDef *GPIO_DBGx, uint32_t JTAG_PIN)
{
    /* Check the parameters */
    assert_param(IS_GPIO_JTAG_PIN(JTAG_PIN));
    /* Disable JTAG pin */
    if((JTAG_PIN & GPIO_DBG_JTCK_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_DisablePinJTCK(GPIO_DBGx);
    }
    if((JTAG_PIN & GPIO_DBG_JTMS_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_DisablePinJTMS(GPIO_DBGx);
    }
    if((JTAG_PIN & GPIO_DBG_JTDI_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_DisablePinJTDI(GPIO_DBGx);
    }
    if((JTAG_PIN & GPIO_DBG_JTDO_EN) != 0)
    {
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD1);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD2);
        LL_GPIO_WriteDebugKey(GPIO_DBGx, LL_GPIO_DBG_ACCESS_PASSWORD3);
        LL_GPIO_DisablePinJTDO(GPIO_DBGx);
    }
}

/**
 * @}
 */

/**
 * @}
 */

#endif /* HAL_GPIO_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
