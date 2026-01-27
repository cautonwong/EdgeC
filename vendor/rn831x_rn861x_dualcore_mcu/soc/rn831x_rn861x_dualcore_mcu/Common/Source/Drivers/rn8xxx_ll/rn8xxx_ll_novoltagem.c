/********************************************************************************
 * @file    NoVoltageM.c
 * @author  Renergy Application Team
 * @version V1.0.1
 * @date    2024.10.10  V2产品上测试代码可正常进行DCOS
 * @brief   全失压测试电流模块
 ******************************************************************************
 * @attention
 * 自动DCOS校正
 * 需求分析
 * 初始化全失压模块，跟自动DCOS校正的位置（需要使用全失压测量时进行全失压初始化；需要进行全失压自动DCOS校正的时候进行全失压自动DCOS校正）
 * 需求拆分
 * 1、全失压模块初始化
 * 开启计量模块
 * 开启全失压模块
 * 配置全失压模块（开模块、关闭高通、LS_DCOS）
 * 2、自动DCOS校正
 * 2.1、自动DCOS校正前需要对DCOS寄存器内的值清零
 * 2.2、自动DCOS校正完成后，用NVM->LS_DCOS值；
 * 2.3、需要考虑增益校正寄存器
 * 2.4、自动DCOS校正耗时2.3s
 * 2.5、全失压测量时的系统时钟，要求与自动DCOS校正时的系统时钟保持一致
 * 2.6、全失压测量时的通道增益，要求与自动DCOS校正时的通道增益保持一致
 *
 * 3、全失压测量结果的获取
 * 
 * 自动DCOS要求NVM模块开启，其高通关闭
 * 若NVM的高通开启，则全失压自动校正标志会清零，但是DCOS寄存器中不会有值
 * 自动DCOS校正时，计量的高通开启和关闭不影响自动校正的结果；但自动DCOS校正有时候会失效，如果校正后结果不合适，建议多次进行
 
 *******************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "rn8xxx_ll.h"/* lint !e766 */
#ifdef LL_NVM_MODULE_ENABLED

#if defined(RN821x_RN721x_SOC_V2)||defined(RN821x_RN721x_SOC_V3)||defined(RN821x_RN721x_SOC_D)
/* Private define ------------------------------------------------------------*/

/* Private enum -------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions ----------------------------------------------------------*/
static void Nvm_DelayUs(uint32_t i)
{
  if (((SYSCTL->OSC_CTRL1 >> 8) & 0x7u) != 0x3u) /* ????*/
  {
    for (; i > 0u; i--)
    {
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
    }
  }
  else
  {
    for (; i > 0u; i--)
    {
      __NOP();
      __NOP();
    }
  }
}

/*********************************************************************************************************
** Function name:     	LL_NVM_Init
** Descriptions:       	全失压初始化
** inputparameters:    ls_dcos：全失压DCOS值
** outputparameters:   无
** Returned value: 	无
*********************************************************************************************************/
void LL_NVM_Init(int16_t *ls_dcos, uint32_t nvm_ie)
{
  SYSCTL->SYS_PS = 0x82U;  
  SYSCTL->MOD1_EN |= 1 << 7;   // EMU_Clk_En;/*"开启EMU时钟"*/
  SYSCTL->MOD1_EN |= 1 << 8;   // NVM_Clk_En;/*"开启NVM时钟"*/
  SYSCTL->SYS_PD &= ~(1 << 0); // ADCIA_PD_PowOn;/*"开启ADC电源"*/
  SYSCTL->ADC_CTRL = 0x3;/* IA通道增益配置为16倍 ，也可修改为其他增益值*/
  SYSCTL->OSC_CTRL1 &= ~0X2;   // RC;/*"开启RC时钟"*/  
  SYSCTL->SYS_PS = 0x0U;
    
  NVM->LSCFG = 8 ; /*"配置计算时间为80ms；"*/
  #if defined(RN821x_RN721x_SOC_V3) || defined(RN821x_RN721x_SOC_D) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_B)
  NVM->LSDCOS = *ls_dcos; /*"全失压直流offset值"*/
  #elif defined(RN821x_RN721x_SOC_V2)
  NVM->LSDCOS = *ls_dcos++; /*"全失压直流offset值"*/
  NVM->LSDCOSIB = *ls_dcos; /*"IB通道全失压直流offset值"*/
  #elif defined(RN202x_RN732x_SOC_V2) || defined(RN202x_RN7326_SOC_B)
  NVM->LS_DCOS_IA = *ls_dcos; /*"全失压直流offset值"*/
  NVM->LS_DCOS_IB = *ls_dcos; /*"全失压直流offset值"*/
  NVM->LS_DCOS_IC = *ls_dcos; /*"全失压直流offset值"*/
  #endif
  
  NVM->IF = NVM->IF;
  NVM->IE = nvm_ie;   
}

/*********************************************************************************************************
** Function name:     	LL_NVM_DeInit
** Descriptions:       	全失压去初始化
** inputparameters:    无
** outputparameters:   无
** Returned value: 	无
*********************************************************************************************************/
void LL_NVM_DeInit(void)
{
  
  SYSCTL->SYS_PS = 0x82U;
  SYSCTL->OSC_CTRL1 |= 0X2;    /*"关闭RCH时钟"*/

  SYSCTL->SYS_PD |= (1 << 0);  /*"关闭ADC电源"*/
  SYSCTL->SYS_PD |= (1 << 6);  /*"开启BGR电源"*/
  SYSCTL->SYS_PD |= (1 << 17); /*"开启LDO3电源"*/

  SYSCTL->MOD1_EN &= ~(1 << 8); /*"关闭NVM时钟"*/
  SYSCTL->MOD1_EN &= ~(1 << 7); /*"关闭EMU时钟"*/

  SYSCTL->SYS_PS = 0x82U;
}
/*********************************************************************************************************
** Function name:     	LL_NVM_AutoDC
** Descriptions:       	全失压自动DCOS校正函数
** inputparameters:    无
** outputparameters:   ls_dcos：全失压DCOS校正值
** Returned value: 	无
*********************************************************************************************************/
ErrorStatus LL_NVM_AutoDC(int16_t *ls_dcos)
{
  uint16_t count = 0;

  #if defined(RN821x_RN721x_SOC_V3) || defined(RN821x_RN721x_SOC_D) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_B)
  NVM->LSDCOS = 0; /*"全失压直流offset值"*/
  NVM->LSCFG = 5;
  #elif defined(RN821x_RN721x_SOC_V2)
  NVM->LSDCOS = 0; /*"全失压直流offset值"*/
  NVM->LSDCOSIB = 0; /*"IB通道全失压直流offset值"*/
  NVM->LSCFG = 5;
  #elif defined(RN202x_RN732x_SOC_V2) || defined(RN202x_RN7326_SOC_B)
  NVM->LS_DCOS_IA = 0; /*"全失压直流offset值"*/
  NVM->LS_DCOS_IB = 0; /*"全失压直流offset值"*/
  NVM->LS_DCOS_IC = 0; /*"全失压直流offset值"*/
  NVM->LS_CFG = 5;
  #endif

  while(count++ <3000) /*"SOC V2全失压直流offset耗时约2.56S (1.3s)"*/
  {
    Nvm_DelayUs(1000);  
    WDT->EN = 0xbb;    
    #if defined(RN821x_RN721x_SOC_V3) || defined(RN821x_RN721x_SOC_D) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_B) || defined(RN821x_RN721x_SOC_V2)
    if (NVM->LSCFG & 1 == 0)
      break;
    #elif defined(RN202x_RN732x_SOC_V2) || defined(RN202x_RN7326_SOC_B)
    if (NVM->LS_CFG & 1 == 0)
      break;
    #endif
  }

  #if defined(RN821x_RN721x_SOC_V3) || defined(RN821x_RN721x_SOC_D) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_B)
  *ls_dcos = NVM->LSDCOS; /*"全失压直流offset值"*/
  #elif defined(RN821x_RN721x_SOC_V2)
  *ls_dcos++ = NVM->LSDCOS; /*"全失压直流offset值"*/
  *ls_dcos = NVM->LSDCOSIB; /*"IB通道全失压直流offset值"*/
  #elif defined(RN202x_RN732x_SOC_V2) || defined(RN202x_RN7326_SOC_B)
  *ls_dcos++ = NVM->LS_DCOS_IA; /*"全失压直流offset值"*/
  *ls_dcos++ = NVM->LS_DCOS_IB; /*"全失压直流offset值"*/
  *ls_dcos++ = NVM->LS_DCOS_IC; /*"全失压直流offset值"*/
  #endif
  //  	while(EMU->AUOTDC_EN & 0x15 )/*"等待自动DCOS校正完成"*/
  //		{
  //			fnDelayMs(500);
  //			WDT->EN = 0XBB;
  //		}
  //		if(EMU->AUOTDC_EN & 0x15) return ERN_ERROR;
  //		/*"自动DCOS校正完成后将校正值取到LS_DCOS_IA并存储EEPROM"*/
  //		*ls_dcos = EMU->DCOS_IA*2;
  //		NVM->LSDCOS = *ls_dcos;

  return ERN_SUCCESS;
}
/*********************************************************************************************************
** Function name:     	LL_NVM_StartMeasure
** Descriptions:       	全失压测量启动函数，全失压测量启动后，会按照初始化在LSCFG中配置的有效值更新时间，自动更新
** inputparameters:    无
** outputparameters:   无
** Returned value: 	无
*********************************************************************************************************/
void LL_NVM_StartMeasure(void)
{
  uint32_t status = NVM->IF;
  NVM->LSCFG &= ~BIT2;
  NVM->IF = status;
  NVM->LSCFG |= BIT2;
}
/*********************************************************************************************************
** Function name:     	LL_NVM_GetRMS
** Descriptions:       	全失压测量结果获取函数
** inputparameters:    无
** outputparameters:   *rml_lx：全失压测量结果
** Returned value: 	无
*********************************************************************************************************/
void LL_NVM_GetRMS(uint32_t *rml_lx)
{  
    #if defined(RN821x_RN721x_SOC_V3) || defined(RN821x_RN721x_SOC_D) || defined(RN821x_RN721x_SOC_C) || defined(RN821x_RN721x_SOC_B)
    *rml_lx = NVM->LSRMS;
    #elif defined(RN821x_RN721x_SOC_V2)
    *rml_lx++ = NVM->LSRMS;
    *rml_lx = NVM->LSRMSIB;
    #elif defined(RN202x_RN732x_SOC_V2) || defined(RN202x_RN7326_SOC_B)
    *rml_lx++ = NVM->RMS_LA;
    *rml_lx++ = NVM->RMS_LB;
    *rml_lx = NVM->RMS_LC;
    #endif
}

#endif
#endif
/****************************  (C) COPYRIGHT Renergy  ****************************/
/****************************          END OF FILE          ****************************/
