/**
 *****************************************************************************
 * @file rn_rv_ll_gpio.c
 * @author Renergy Software Team
 * @brief
 *
 *****************************************************************************
 * @attention
 *
 * Copyright (c) 2025, Renergy Technology Inc.
 * All rights reserved.
 *
 *****************************************************************************
 */

#if defined(USE_FULL_LL_DRIVER)

/* Includes -----------------------------------------------------------------*/
#include "rn_rv_ll_gpio.h"
#include "rn_rv_ll_system_core0.h"

#ifdef  USE_FULL_ASSERT
#include "rn_rv_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup RN_LL_Driver
 * @{
 */

#if defined (GPIOA) || defined (GPIOB) || defined (GPIOC) || defined (GPIOD) || defined (GPIOE) || defined (GPIOF) || defined (GPIOG) || defined (GPIOH) || defined (GPIOI) || defined (GPIOJ) || defined (GPIOK) || defined (GPIO_DBGx)

/** @defgroup GPIO_LL GPIO
 * @{
 */

/* Private types ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private constants --------------------------------------------------------*/
/* Private macros -----------------------------------------------------------*/
/** @addtogroup GPIO_LL_Private_Macros
 * @{
 */
#define IS_LL_GPIO_PIN(__VALUE__)          (((0x00000000U) < (__VALUE__)) && ((__VALUE__) <= (LL_GPIO_PIN_ALL)))

#define IS_LL_GPIO_MODE(__VALUE__)         (((__VALUE__) == LL_GPIO_MODE_INPUT)     ||\
                                            ((__VALUE__) == LL_GPIO_MODE_OUTPUT)    ||\
                                            ((__VALUE__) == LL_GPIO_MODE_ALTERNATE) ||\
                                            ((__VALUE__) == LL_GPIO_MODE_ANALOG))

#define IS_LL_GPIO_OUTPUT_TYPE(__VALUE__)  (((__VALUE__) == LL_GPIO_OUTPUT_PUSHPULL)  ||\
                                            ((__VALUE__) == LL_GPIO_OUTPUT_OPENDRAIN))

#define IS_LL_GPIO_INPUT_TYPE(__VALUE__)  (((__VALUE__) == LL_GPIO_INPUT_CMOS)  ||\
                                            ((__VALUE__) == LL_GPIO_INPUT_TTL))

#define IS_LL_GPIO_SPEED(__VALUE__)        (((__VALUE__) == LL_GPIO_SPEED_FREQ_NORMAL) ||\
                                            ((__VALUE__) == LL_GPIO_SPEED_FREQ_HIGH))


#define IS_LL_GPIO_PULL(__VALUE__)         (((__VALUE__) == LL_GPIO_PULL_NO)   ||\
                                            ((__VALUE__) == LL_GPIO_PULL_UP)   ||\
                                            ((__VALUE__) == LL_GPIO_PULL_DOWN))

#define IS_LL_GPIO_ALTERNATE(__VALUE__)    (((__VALUE__) == LL_GPIO_AF_0  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_1  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_2  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_3  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_4  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_5  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_6  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_7  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_8  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_9  )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_10 )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_11 )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_12 )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_13 )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_14 )   ||\
                                            ((__VALUE__) == LL_GPIO_AF_15 ))
/**
 * @}
 */

/* Private function prototypes ----------------------------------------------*/
/* Exported functions -------------------------------------------------------*/
/** @addtogroup GPIO_LL_Exported_Functions
 * @{
 */

/** @addtogroup GPIO_LL_EF_Init
 * @{
 */

/**
 * @brief  De-initialize GPIO registers
 * @param  GPIOx GPIO port
 * @return ErrorStatus value:
 *          - SUCCESS: GPIO registers are de-initialized
 *          - ERROR:   Wrong GPIO Port
 */
ErrorStatus LL_GPIO_DeInit(GPIO_TypeDef *GPIOx)
{
    ErrorStatus status = SUCCESS;
    /* Check the parameters */
    assert_param(IS_GPIO_ALL_INSTANCE(GPIOx));

    if((GPIOx == (GPIOA)) || (GPIOx == (GPIOB)) || (GPIOx == (GPIOC)) || (GPIOx == (GPIOD)) ||
       (GPIOx == (GPIOE)) || (GPIOx == (GPIOF)) || (GPIOx == (GPIOG)) || (GPIOx == (GPIOH)) ||
       (GPIOx == (GPIOL)))
    {
        /* Force and Release reset on clock of GPIOx Port */
        LL_SYSTEM_CORE0_PMOD0_ForceReset(LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GPIO);
        LL_SYSTEM_CORE0_PMOD0_ReleaseReset(LL_SYSTEM_CORE0_PMOD0_RST_PERIPH_GPIO);
    } else {
        status = ERROR;
    }
    return (status);
}

/**
 * @brief Initialize GPIO registers according to the specified parameters in GPIO_InitStruct
 * @param GPIOx GPIO Port
 * @param GPIO_InitStruct pointer to LL_GPIO_InitTypeDef structure
 * @return ErrorStatus value:
 *          - SUCCESS: GPIO registers are initialized
 *          - ERROR:   Not applicable
 */
ErrorStatus LL_GPIO_Init(GPIO_TypeDef *GPIOx, LL_GPIO_InitTypeDef *GPIO_InitStruct)
{
    uint32_t pinpos     = 0x00000000U;
    uint32_t currentpin = 0x00000000U;

    /* Check the parameters */
    assert_param(IS_GPIO_ALL_INSTANCE(GPIOx));
    assert_param(IS_LL_GPIO_PIN(GPIO_InitStruct->Pin));
    assert_param(IS_LL_GPIO_MODE(GPIO_InitStruct->Mode));
    assert_param(IS_LL_GPIO_PULL(GPIO_InitStruct->Pull));

    /* ------------------------- Configure the port pins ---------------- */
    /* Initialize pinpos on first pin set */
    pinpos = POSITION_VAL(GPIO_InitStruct->Pin);

    /* Configure the port pins */
    while (((GPIO_InitStruct->Pin) >> pinpos) != 0x00000000U)
    {
        /* Get current io position */
        currentpin = (GPIO_InitStruct->Pin) & (0x00000001U << pinpos);

        if (currentpin)
        {
            if ((GPIO_InitStruct->Mode == LL_GPIO_MODE_OUTPUT) || (GPIO_InitStruct->Mode == LL_GPIO_MODE_ALTERNATE))
            {
                /* Check Speed mode parameters */
                assert_param(IS_LL_GPIO_SPEED(GPIO_InitStruct->Speed));
                /* Speed mode configuration */
                LL_GPIO_SetPinSpeed(GPIOx, currentpin, GPIO_InitStruct->Speed);

                /* Check Output mode parameters */
                assert_param(IS_LL_GPIO_OUTPUT_TYPE(GPIO_InitStruct->OutputType));
                /* Output mode configuration*/
                LL_GPIO_SetPinOutputType(GPIOx, currentpin, GPIO_InitStruct->OutputType);
            }

            if(GPIO_InitStruct->Mode != LL_GPIO_MODE_ANALOG)
            {
                /* Check Input mode parameters */
                assert_param(IS_LL_GPIO_INPUT_TYPE(GPIO_InitStruct->InputType));
                /* Input mode configuration*/
                LL_GPIO_SetPinInputType(GPIOx, currentpin, GPIO_InitStruct->InputType);
            }

            if((GPIO_InitStruct->Mode == LL_GPIO_MODE_INPUT) || (GPIO_InitStruct->Mode == LL_GPIO_MODE_ALTERNATE))
            {
                /* Pull up / Pull down configuration*/
                LL_GPIO_SetPinPull(GPIOx, currentpin, GPIO_InitStruct->Pull);
            }

            if (GPIO_InitStruct->Mode == LL_GPIO_MODE_ALTERNATE)
            {
                /* Check Alternate parameter */
                assert_param(IS_LL_GPIO_ALTERNATE(GPIO_InitStruct->Alternate));

                /* Alternate function configuration */
                if (POSITION_VAL(currentpin) < 0x00000008U)
                {
                    LL_GPIO_SetAFPin_0_7(GPIOx, currentpin, GPIO_InitStruct->Alternate);
                }
                else
                {
                    LL_GPIO_SetAFPin_8_15(GPIOx, currentpin, GPIO_InitStruct->Alternate);
                }
            }

            /* Pin Mode configuration */
            LL_GPIO_SetPinMode(GPIOx, currentpin, GPIO_InitStruct->Mode);
        }
        pinpos++;
    }

    return (SUCCESS);
}

/**
 * @brief Set each @ref LL_GPIO_InitTypeDef field to default value.
 * @param GPIO_InitStruct pointer to LL_GPIO_InitTypeDef structure
 * @retval None
 */
void LL_GPIO_StructInit(LL_GPIO_InitTypeDef *GPIO_InitStruct)
{
    /* Reset GPIO init structure parameters values */
    GPIO_InitStruct->Pin        = LL_GPIO_PIN_ALL;
    GPIO_InitStruct->Mode       = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct->Speed      = LL_GPIO_SPEED_FREQ_NORMAL;
    GPIO_InitStruct->OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct->InputType  = LL_GPIO_INPUT_CMOS;
    GPIO_InitStruct->Pull       = LL_GPIO_PULL_NO;
    GPIO_InitStruct->Alternate  = LL_GPIO_AF_0;
}
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#endif/* defined (GPIOA) || defined (GPIOB) || defined (GPIOC) || defined (GPIOD) || defined (GPIOE) || defined (GPIOF) || defined (GPIOG) || defined (GPIOH) || defined (GPIOI) || defined (GPIOJ) || defined (GPIOK) || define (GPIO_DBG) */

/**
 * @}
 */

#endif /* USE_FULL_LL_DRIVER */
