/******************************************************************************
 * @file     renergy_chip_config.h
 * @brief    renergy chip config
 * @author   Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 ******************************************************************************/
#ifndef RENERGY_CHIP_CONFIG_H_
#define RENERGY_CHIP_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * According to product functionality classification, currently 6 series products are supported.
 * Please correctly define the chip series macro definition based on the chip used;
 * If you cannot confirm the chip series macro definition, defining the chip model macro
 * is also currently supported. The mapping relationship between the two is as follows:
 *  RN821x_RN721x_SOC_D   corresponding chip model macros: RN7213_D RN8213_D RN8213B_D RN8217_D
 *  RN821x_RN721x_SOC_V2  corresponding chip model macros: RN7213_V2 RN8211B_V2 RN8213_V2 RN8213B_V2 RN8215_V2 RN8217_V2
 *  RN821x_RN721x_SOC_V3  corresponding chip model macros: RN8211B_V3 RN8213_V3
 *  RN831x_RN861x_MCU_V2  corresponding chip model macros: RN8318_V2 RN8611_V2 RN8613_V2 RN8615_V2
 *  RN831x_RN861x_MCU_V3  corresponding chip model macros: RN8610_V3 RN8612_V3 RN8613_V3
 *  RN202x_RN7326_SOC_V2  corresponding chip model macros: RN2026_V2 RN2028_V2 RN7326_V2 RN2025_V2
 */

/* TODO Please define the correct chip macro definition (recommended) or chip model macro definition here */

/* The mapping relationship between chip model macro definitions and chip series macro definitions is as follows */
#if defined(RN8213_D) || defined(RN8217_D) || defined(RN8213B_D) || defined(RN7213_D)
#define RN821x_RN721x_SOC_D
#elif defined(RN8211B_V2) || defined(RN8213_V2) || defined(RN8215_V2) || defined(RN8217_V2) || defined(RN8213B_V2) || defined(RN7213_V2)
#define RN821x_RN721x_SOC_V2
#elif defined(RN8318_V2) || defined(RN8615_V2) || defined(RN8613_V2)|| defined(RN8611_V2)
#define RN831x_RN861x_MCU_V2
#elif defined(RN8211B_V3) || defined(RN8213_V3)
#define RN821x_RN721x_SOC_V3
#elif defined(RN8613_V3) || defined(RN8610_V3) || defined(RN8612_V3)
#define RN831x_RN861x_MCU_V3
#elif defined(RN2025_V2)|| defined(RN2026_V2)|| defined(RN2028_V2)|| defined(RN7326_V2)
#define RN202x_RN7326_SOC_V2
#endif

#if (defined(RN831x_RN861x_MCU_V1) + \
     defined(RN831x_RN861x_MCU_V2) + \
     defined(RN831x_RN861x_MCU_V3) + \
     defined(RN821x_RN721x_SOC_B) + \
     defined(RN821x_RN721x_SOC_C) + \
     defined(RN821x_RN721x_SOC_D) + \
     defined(RN821x_RN721x_SOC_V2) + \
     defined(RN821x_RN721x_SOC_V3) + \
     defined(RN202x_RN7326_SOC_B) + \
     defined(RN202x_RN7326_SOC_V2)) > 1

    #if defined(RN831x_RN861x_MCU_V1)
    #warning "defined RN831x_RN861x_MCU_V1"
    #undef RN831x_RN861x_MCU_V1
    #endif
    #if defined(RN831x_RN861x_MCU_V2)
    #warning "defined RN831x_RN861x_MCU_V2"
    #undef RN831x_RN861x_MCU_V2
    #endif
    #if defined(RN831x_RN861x_MCU_V3)
    #warning "defined RN831x_RN861x_MCU_V3"
    #undef RN831x_RN861x_MCU_V3
    #endif
    #if defined(RN821x_RN721x_SOC_B)
    #warning "defined RN821x_RN721x_SOC_B"
    #undef RN821x_RN721x_SOC_B
    #endif
    #if defined(RN821x_RN721x_SOC_C)
    #warning "defined RN821x_RN721x_SOC_C"
    #undef RN821x_RN721x_SOC_C
    #endif
    #if defined(RN821x_RN721x_SOC_D)
    #warning "defined RN821x_RN721x_SOC_D"
    #undef RN821x_RN721x_SOC_D
    #endif
    #if defined(RN821x_RN721x_SOC_V2)
    #warning "defined RN821x_RN721x_SOC_V2"
    #undef RN821x_RN721x_SOC_V2
    #endif
    #if defined(RN821x_RN721x_SOC_V3)
    #warning "defined RN821x_RN721x_SOC_V3"
    #undef RN821x_RN721x_SOC_V3
    #endif
    #if defined(RN202x_RN7326_SOC_B)
    #warning "defined RN202x_RN7326_SOC_B"
    #undef RN202x_RN7326_SOC_B
    #endif
    #if defined(RN202x_RN7326_SOC_V2)
    #warning "defined RN202x_RN7326_SOC_V2"
    #undef RN202x_RN7326_SOC_V2
    #endif
    #if defined(RN831x_RN861x_MCU_V4)
    #warning "defined RN831x_RN861x_MCU_V4"
    #undef RN831x_RN861x_MCU_V4
    #endif

    #error "Multiple chip types defined!"

#elif (defined(RN831x_RN861x_MCU_V1) + \
       defined(RN831x_RN861x_MCU_V2) + \
       defined(RN831x_RN861x_MCU_V3) + \
       defined(RN821x_RN721x_SOC_B) + \
       defined(RN821x_RN721x_SOC_C) + \
       defined(RN821x_RN721x_SOC_D) + \
       defined(RN821x_RN721x_SOC_V2) + \
       defined(RN821x_RN721x_SOC_V3) + \
       defined(RN202x_RN7326_SOC_B) + \
       defined(RN202x_RN7326_SOC_V2)+ \
       defined(RN831x_RN861x_MCU_V4)) == 0

    #error "No chip type selected"

#endif

/* library code selection macro */
// #define     NVRLIB_EN

#ifdef __cplusplus
}
#endif

#endif
/* r2863 */
