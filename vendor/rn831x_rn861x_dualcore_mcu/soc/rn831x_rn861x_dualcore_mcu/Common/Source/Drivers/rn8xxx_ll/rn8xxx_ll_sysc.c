/*******************************************************************************
  * @file    LL_SYSC.c
  * @author  Renergy Application Team
  * @version V1.1.0
  * @date    2023-09-07
  * @brief   Sysctrl driver
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 Renergy.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  *@verbatim
  ==============================================================================
                     ##### Sysctrl peripheral features #####
  ==============================================================================
  [..]

                     ##### How to use this driver #####
  ==============================================================================
    [..]


    @endverbatim
  */
/* Includes ------------------------------------------------------------------*/

#include "rn8xxx_ll.h"

#if !defined(RN831x_RN861x_MCU_V4)
#ifdef LL_SYSC_MODULE_ENABLED
/* Private define ------------------------------------------------------------*/
#define RST_REG_MASK 0X29E0UL
/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private data ---------------------------------------------------------*/

/* Exported data ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/


/*********************************************************************************************************
** Function name:     LL_SYSC_ApbClkEnable
** Descriptions:     开启模块的APB时钟
** input parameters: moduel_id:模块序号; FunctionalState: 操作方式：开启或关闭
** output parameters:  null
** Returned value:   status of switching system clock
*********************************************************************************************************/

void LL_SYSC_ApbClkCtrl(eModuleID_TypeDef moduel_id, FunctionalState cmd)
{

    volatile uint32_t *module_addr;
    uint32_t temp_moduel_id = (uint32_t)moduel_id;

    SYSCTL_ENABLE_WRITE;

    if ((temp_moduel_id > 31) && (temp_moduel_id < 64))
    {
        temp_moduel_id = temp_moduel_id - 32U;/* moduel_id - 32 */
        module_addr = &(SYSCTL->MOD1_EN);
    }
    else if ((temp_moduel_id > 63) && (temp_moduel_id < 96))
    {
        temp_moduel_id = temp_moduel_id - 64;
        module_addr = &(SYSCTL->INTC_EN);
    }
    else if (temp_moduel_id > 95)
    {
        temp_moduel_id = temp_moduel_id - 96;
        module_addr = &(SYSCTL->KBI_EN);
    }
    else
    {
        module_addr = &(SYSCTL->MOD0_EN);
    }

    if (ERN_DISABLE != cmd)
    {
        SET_REG32_BIT(*module_addr, 1UL << temp_moduel_id);
    }
    else
    {
        CLR_REG32_BIT(*module_addr, 1UL << temp_moduel_id);
    }
    SYSCTL_DISABLE_WRITE;
}

/* 模拟模块电源开关函数 （只操作模拟模块开关，延迟和电阻配置不支持）
传入参数：模拟模块序号，开关操作*/
void LL_SYSC_AnaPowerCtrl(eLL_SYSC_PDModuleID_TypeDef pdmoduel_id, FunctionalState cmd)
{
    SYSCTL_ENABLE_WRITE;
#if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3)     
    if(LL_SYSC_PD_LDO3_ID == pdmoduel_id)
    {
        if (ERN_DISABLE != cmd)
        {
            SET_REG32_BIT(SYSCTL->SYS_PD, 1UL << (uint32_t)pdmoduel_id);
        }
        else
        {
            CLR_REG32_BIT(SYSCTL->SYS_PD, 1UL << (uint32_t)pdmoduel_id);
        }
    }
    else
#endif
    {
        if (ERN_DISABLE != cmd)
        {
            CLR_REG32_BIT(SYSCTL->SYS_PD, 1UL << (uint32_t)pdmoduel_id);
        }
        else
        {
            SET_REG32_BIT(SYSCTL->SYS_PD, 1UL << (uint32_t)pdmoduel_id);

        }
    }
    SYSCTL_DISABLE_WRITE;
}

/*********************************************************************************************************
** Function name:     LL_SYSC_ApbClkEnable
** Descriptions:     开启模块的APB时钟
** input parameters: moduel_id:模块序号; FunctionalState: 操作方式：开启或关闭
** output parameters:  null
** Returned value:   status of switching system clock
*********************************************************************************************************/

#if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3) ||defined(RN821x_RN721x_SOC_B)|| defined(RN821x_RN721x_SOC_C)|| defined(RN821x_RN721x_SOC_D) \
    || defined(RN202x_RN7326_SOC_B) || defined(RN202x_RN7326_SOC_V2)

/* ADC增益控制函数
传入参数：通道，放大倍数*/
void LL_SYSC_ADCPGACtrl(eLL_SYSC_ADCChannel_TypeDef adc_channel, eLL_SYSC_ADCPGA_TypeDef adc_pag)
{
    uint8_t u8FieldOfs;

    u8FieldOfs = adc_channel * 3U;
    SYSCTL_ENABLE_WRITE;
    MODIFY_REG32(SYSCTL->ADC_CTRL,(uint32_t)LL_SYSC_ADCPGA_MAX << u8FieldOfs, (uint32_t)adc_pag << u8FieldOfs);
    SYSCTL_DISABLE_WRITE;
}
#endif
/* 系统模块复位标志清除函数
传入参数：系统模块序号，
注意：具有写1清零的位的寄存器，不能用或操作原寄存器值再写入，需屏蔽写1清零的复位标志位*/

void LL_SYSC_SysRstFlagClr(eLL_SYSC_RSTFlag_TypeDef sysc_rst_id)
{
   volatile uint32_t *rst_reg_addr = NULL;

  #if defined(RN202x_RN7326_SOC_V2)
  eLL_SYSC_RSTFlag_TypeDef temp_rst_id = sysc_rst_id;   
  if(temp_rst_id>31)
  {    
    rst_reg_addr = &(SYSCTL->SYS_RST1);    
    temp_rst_id -=  32;
  }
  else
  #endif
  {
    rst_reg_addr = &(SYSCTL->SYS_RST);
  }    
  SYSCTL_ENABLE_WRITE;
  MODIFY_REG32(*rst_reg_addr,RST_REG_MASK|(1UL << (uint32_t)sysc_rst_id), 1UL << (uint32_t)sysc_rst_id);/* 屏蔽写1清零的复位标志位 */
  SYSCTL_DISABLE_WRITE;
}
/* 系统模块复位标志获取函数
传入参数：系统模块序号*/
eLL_SYSC_RST_Status_TypeDef LL_SYSC_SysRstFlagGet(eLL_SYSC_RSTFlag_TypeDef sysc_rst_id)
{
  eLL_SYSC_RST_Status_TypeDef reset_status = LL_SYSC_RST_INVALID;
  volatile const uint32_t *rst_reg_addr = NULL;
  #if defined(RN202x_RN7326_SOC_V2) 
   eLL_SYSC_RSTFlag_TypeDef temp_rst_id = sysc_rst_id;
  if(temp_rst_id>31)
  {    
    rst_reg_addr = &(SYSCTL->SYS_RST1);    
    temp_rst_id -=  32;
  }
  else
  #endif
  {
    rst_reg_addr = &(SYSCTL->SYS_RST);
  }

  if(READ_REG32_BIT(*rst_reg_addr, 1UL << (uint32_t)sysc_rst_id) != 0U)
  {
    reset_status = LL_SYSC_RST_VALID;
  }
  return(reset_status);
}
/* 系统模块复位配置函数：是否使能相应模块复位功能
传入参数：系统模块序号，ERN_DISABLE：不使能复位功能；ERN_ENABLE：使能复位功能
注意：具有写1清零的位的寄存器，不能用或操作原寄存器值再写入，需屏蔽写1清零的复位标志位
*/

void LL_SYSC_SysRstCfg(eLL_SYSC_RSTConfg_TypeDef sysc_rst_id)
{
   volatile uint32_t *rst_reg_addr = NULL;
  #if defined(RN202x_RN7326_SOC_V2) 
  eLL_SYSC_RSTConfg_TypeDef temp_rst_id = sysc_rst_id;
  if(temp_rst_id>31)
  {    
    rst_reg_addr = &(SYSCTL->SYS_RST1);    
    temp_rst_id -=  32;
  }
  else
  #endif
  {
    rst_reg_addr = &(SYSCTL->SYS_RST);
  }
  SYSCTL_ENABLE_WRITE;
  MODIFY_REG32( *rst_reg_addr,RST_REG_MASK|(1UL << (uint32_t)sysc_rst_id), 1UL << (uint32_t)sysc_rst_id);/* 屏蔽写1清零的复位标志位 */
  SYSCTL_DISABLE_WRITE;
}

/* 系统模块复位操作函数：启动复位，取消复位
传入参数：系统模块序号，ERN_DISABLE：取消复位操作；ERN_ENABLE：进行复位操作*/
void LL_SYSC_SysRstRqst(eLL_SYSC_RSTRqst_TypeDef sysc_rst_id, FunctionalState cmd)
{
  uint32_t u32cmd = 0;
  volatile uint32_t *rst_reg_addr = NULL;
  #if defined(RN202x_RN7326_SOC_V2)
   eLL_SYSC_RSTRqst_TypeDef temp_rst_id = sysc_rst_id;

  if(temp_rst_id>31)
  {     
    rst_reg_addr = &(SYSCTL->SYS_RST1);    
    temp_rst_id -=  32;
  }
  else
  #endif
  {
    rst_reg_addr = &(SYSCTL->SYS_RST);
  }
    if(ERN_DISABLE == cmd)
    {
        u32cmd = 0u;
    }
    else
    {
        u32cmd = 1u;
    }
    SYSCTL_ENABLE_WRITE;
    MODIFY_REG32(*rst_reg_addr,RST_REG_MASK|(1UL << (uint32_t)sysc_rst_id), u32cmd << (uint32_t)sysc_rst_id);
    SYSCTL_DISABLE_WRITE;
}

#if defined(RN821x_RN721x_SOC_V2) || defined(RN831x_RN861x_MCU_V2) \
    || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3) \
    || defined(RN202x_RN7326_SOC_B) || defined(RN202x_RN7326_SOC_V2) 
/* DMA优先级配置
传入参数：dam_chn：DMA通道，dma_pri：优先级*/
void LL_SYSC_DMAPriSet(eLL_SYSC_DMA_CHN_TypeDef dam_chn,eLL_SYSC_DMA_PRI_TypeDef dma_pri)
{
    uint8_t u8Fild_No = (uint8_t)dam_chn;
    volatile uint32_t *dma_pri_addr;
    dma_pri_addr = &SYSCTL->DMA_PRI;

    SYSCTL_ENABLE_WRITE;
    #if defined(RN821x_RN721x_SOC_V2) || defined(RN821x_RN721x_SOC_V3) || defined(RN831x_RN861x_MCU_V3)
    if(u8Fild_No>15u)
    {
        u8Fild_No = u8Fild_No - 16;
        dma_pri_addr = &SYSCTL->DMA_PRI2;
    }
    #endif

    u8Fild_No = (uint8_t)(u8Fild_No*2U);
    MODIFY_REG32(*dma_pri_addr, ((uint32_t)(LL_SYSC_DMA_PRI_PASSWORD)<<(28u))|((uint32_t)dma_pri<<u8Fild_No), ((uint32_t)(LL_SYSC_DMA_PRI_PASSWORD)<<(28u))|((uint32_t)dma_pri<<u8Fild_No));
    SYSCTL_DISABLE_WRITE;
}
#endif
#endif
#endif
/****************************   (C) COPYRIGHT Renergy   ****************************/
/****************************       END OF FILE     ********************************/
