/**
 ******************************************************************************
 * @file   rn_rv_hal_aes.c
 * @author Renergy Software Team
 * @brief  AES HAL module driver.
 *         This file provides firmware functions to manage the following
 *         functionalities of the Advanced Encryption Standard (AES) peripheral:
 *          + Initialization and de-initialization functions
 *          + Control functions
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
#include <string.h>

/** @addtogroup RN_HAL_Driver
 * @{
 */

/** @defgroup AES AES
 * @brief     AES driver modules
 * @{
 */

#ifdef HAL_AES_MODULE_ENABLED

/* Private typedef ----------------------------------------------------------*/
/** @defgroup AES_Private_Typedef AES Private Typedef
 * @{
 */
typedef struct
{
    __IO uint32_t KEY_EXP[64];
} AES_KeyExpTypeDef;

typedef struct
{
    __IO uint32_t DATA[4];
} AES_CipherTypeDef;

typedef struct
{
    __IO uint32_t IV[4];
} AES_IVTypeDef;
/**
 * @}
 */

/* Private define -----------------------------------------------------------*/
/** @defgroup AES_Private_Constants AES Private Constants
 * @{
 */
#define MEM_U32(addr)          (*((__IOM uint32_t *)(addr)))
#define SYS_PSW_ADDR(ver)      (((ver) == AES_RUNNING_IN_CORE0) ? 0x20080000U : 0xA0080000U)
#define SYS_PMOD0_ADDR(ver)    (((ver) == AES_RUNNING_IN_CORE0) ? 0x2008012CU : 0xA008012CU)
#define SYS_MOD0_ADDR(ver)     (((ver) == AES_RUNNING_IN_CORE0) ? 0x2008013CU : 0xA008013CU)
#define SYS_MOD1_ADDR(ver)     (((ver) == AES_RUNNING_IN_CORE0) ? 0x20080140U : 0xA0080140U)
#define SYS_MOD2_ADDR(ver)     (((ver) == AES_RUNNING_IN_CORE0) ? 0x20080144U : 0xA0080144U)
#define SYS_FREQ_STA_ADDR(ver) (((ver) == AES_RUNNING_IN_CORE0) ? 0x20080170U : 0xA0080170U)
#define SYS_CLK_STA_ADDR(ver)  (((ver) == AES_RUNNING_IN_CORE0) ? 0x20080174U : 0xA0080174U)
/**
 * @}
 */

/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
/** @defgroup AES_Private_Functions AES Private Functions
 * @{
 */
static void     AES_MemsetWord(uint32_t dst[], uint32_t data, uint32_t len);
static void     AES_Memcpy(uint8_t dst[], const uint8_t src[], uint32_t n);

static uint32_t AES_EnableSCUClk(void);
static void     AES_RecoverSCUClk(uint32_t clk);
static uint32_t AES_GetSCUFlag(void);
static uint32_t AES_IsAddressValid(uint32_t address);
static void     AES_ModuleInit(AES_KernelTypeDef kernel);
static void     AES_ModuleDeInit(AES_KernelTypeDef kernel);
static void     AES_ConvertEndian(uint32_t *p_disc, const uint32_t *p_src, uint32_t size);

static uint32_t AES_Computing(AES_HandleTypeDef *pParas);
static uint32_t AES_ExpendKey(AES_HandleTypeDef *pParas, const uint32_t p_key[]);
static uint32_t AES_GHASH_CalculateH(AES_HandleTypeDef *pParas, uint32_t H[4]);
static uint32_t AES_GHASH_Computing(AES_HandleTypeDef *pParas, const uint32_t pDataIn[], uint8_t *is_start, AES_ENDIAN_MODE little_endian);
static uint32_t AES_CalculateY0(AES_HandleTypeDef *pParas, const uint32_t ivData[], const uint32_t plainData[], uint32_t cryptedData[]);
static uint32_t AES_ECB_Calculate(AES_HandleTypeDef *pParas, const uint32_t ivData[], const uint32_t plainData[], uint32_t cryptedData[]);
static uint32_t AES_CBC_Calculate(AES_HandleTypeDef *pParas, const uint32_t ivData[], const uint32_t plainData[], uint32_t cryptedData[]);
static uint32_t AES_CTR_Calculate(AES_HandleTypeDef *pParas, const uint32_t ivData[], const uint32_t plainData[], uint32_t cryptedData[]);
static uint32_t AES_CFB128_Calculate(AES_HandleTypeDef *pParas, const uint32_t ivData[], const uint32_t plainData[], uint32_t cryptedData[]);
static uint32_t AES_OFB_Calculate(AES_HandleTypeDef *pParas, const uint32_t ivData[], const uint32_t plainData[], uint32_t cryptedData[]);
static uint32_t AES_GHASH_Engine(AES_HandleTypeDef *pParas, const GHashData_t data_in[], uint32_t size, AES_ENDIAN_MODE little_endian);
static uint32_t AES_Crypt_Engine(AES_HandleTypeDef *pParas, const PlainData_t plainData[], uint32_t size, CryptData_t cryptedData[]);
static void     AES_GHASH_Checkin(AES_HandleTypeDef *pParas, const uint32_t pre_data[4]);
static uint32_t AES_GCM_CalculateIVandY0(AES_HandleTypeDef *pParas, uint32_t IV[4], uint32_t Y[4]);
static uint32_t AES_GCM_CalculateTag(AES_HandleTypeDef *pParas, uint32_t aad_size, uint32_t msg_size, aesTag_t *pTag, uint32_t size);
static void     AES_StartManager(AES_HandleTypeDef *pParas);

static uint32_t AES_SetConfig(AES_HandleTypeDef *pParas, const AES_InitTypeDef *cfg);
static uint32_t AES_Encrypt(AES_HandleTypeDef *pParas, const PlainData_t *plainData, uint32_t size, CryptData_t *cryptedData);
static uint32_t AES_Decrypt(AES_HandleTypeDef *pParas, const CryptData_t *cryptedData, uint32_t size, PlainData_t *plainData);
static uint32_t AES_Gmac(AES_HandleTypeDef *pParas, const GHashData_t *ghashData, uint32_t size);
static uint32_t AES_LatchStatus(const AES_HandleTypeDef *pParas, AES_HandleTypeDef *latch_buf);
static uint32_t AES_RecoverStatus(const AES_HandleTypeDef *recover_buf, AES_HandleTypeDef *pParas);
static uint32_t AES_GF128Mult(AES_HandleTypeDef *pParas, const uint8_t *pDataX, const uint8_t *pDataY, uint8_t *pDataZ);
static uint32_t AES_JumpMode(AES_HandleTypeDef *pParas, const uint32_t ivData[], const uint32_t plainData[], uint32_t cryptedData[]);
static uint32_t AES_SetAAD(AES_HandleTypeDef *pParas, const aesAAD_t *addData, uint32_t size);

static void     AES_Xor(uint8_t *A, uint32_t t);
static uint32_t AES_KeyWrapping(AES_HandleTypeDef *pParas, const PlainData_t *plainData, uint32_t in_len, CryptData_t *cryptedData, uint32_t *out_len);
static uint32_t AES_KeyWrappingwithPadding(AES_HandleTypeDef *pParas, const PlainData_t *plainData, uint32_t in_len, CryptData_t *cryptedData, uint32_t *out_len);
static uint32_t AES_KeyUnwrapping(AES_HandleTypeDef *pParas, const CryptData_t *cryptedData, uint32_t in_len, PlainData_t *plainData, uint32_t *out_len, uint8_t *icvData);
static uint32_t AES_KeyUnwrappingwithPadding(AES_HandleTypeDef *pParas, const CryptData_t *cryptedData, uint32_t in_len, PlainData_t *plainData, uint32_t *out_len);

#define AES_KW_SEMIBLOCK_LEN 8
static const uint8_t AES_KW_ICV1[] = {0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6}; /* ICV per RFC-3394 */
static const uint8_t AES_KW_ICV2[] = {0xA6, 0x59, 0x59, 0xA6};                         /* AIV per RFC-5649 */
/**
 * @}
 */

/* Exported functions -------------------------------------------------------*/
/** @defgroup AES_Exported_Functions AES Exported Functions
 * @{
 */

/** @defgroup AES_Exported_Functions_Group1 Initialization and de-initialization functions
 * @brief     Initialization and Configuration functions
 *
@verbatim
================================================================================
            ##### Initialization and de-initialization functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) Initialize and configure the AES.
        (+) De-initialize the AES.

@endverbatim
 * @{
 */

/**
 * @brief  Initialize the AES module and calculation.
 * @param  pParas pointer to AES processing structure.
 * @param  Configuration init configuration structure.
 * @retval AES calculation status.
 */
HAL_AES_StatusTypeDef HAL_AES_Init(AES_HandleTypeDef *pParas, const AES_InitTypeDef *Configuration)
{
    pParas->engine.kernelRegion = ((AES_GetSCUFlag()) ? AES_RUNNING_IN_CORE0 : AES_RUNNING_IN_CORE1);
    AES_ModuleInit(pParas->engine.kernelRegion);
    return AES_SetConfig(pParas, Configuration);
}

/**
 * @brief  De-Initialize the AES module and calculation.
 * @retval AES calculation status.
 */
HAL_AES_StatusTypeDef HAL_AES_DeInit(void)
{
    AES_KernelTypeDef kernel;
    kernel = ((AES_GetSCUFlag()) ? AES_RUNNING_IN_CORE0 : AES_RUNNING_IN_CORE1);
    AES_ModuleDeInit(kernel);
    return AES_SUCCESS;
}

/**
 * @}
 */

/** @defgroup AES_Exported_Functions_Group2 IO operation functions
 * @brief     IO operation functions
 *
@verbatim
================================================================================
                    ##### IO operation functions #####
================================================================================
    [..]  This section provides functions allowing to:
        (+) HAL_AES_Encrypt                 : Start the AES encryption
        (+) HAL_AES_Decrypt                 : Start the AES decryption
        (+) HAL_AES_Gmac                    : Start the AES gmac calculation
        (+) HAL_AES_GF128Mul                : Start the AES Galois Field multiplication
        (+) HAL_AES_KeyWrapping             : Start the AES keywarping
        (+) HAL_AES_KeyUnwrapping           : Start the AES keyunwarping
        (+) HAL_AES_SetAAD                  : Add the AES aad message
        (+) HAL_AES_LatchProcessStatus      : Latch the current process status
        (+) HAL_AES_RecoverProcessStatus    : Recover the last process status

@endverbatim
 * @{
 */

/**
 * @brief  128-bit Galois Field multiplication.
 * @param  pParas pointer to AES processing structure.
 * @param  pDataX Multiplicand X.
 * @param  pDataY Multiplier Y
 * @param  pDataZ Result Z.
 * @retval AES calculation status.
 */
HAL_AES_StatusTypeDef HAL_AES_GF128Mul(AES_HandleTypeDef *pParas, const uint8_t *pDataX, const uint8_t *pDataY, uint8_t *pDataZ)
{
    return AES_GF128Mult(pParas, pDataX, pDataY, pDataZ);
}

/**
 * @brief  Start AES encrypt calculation.
 * @param  pParas pointer to AES processing structure.
 * @param  PlainData pointer to plaintext buffer.
 * @param  PlainLen size of plaintext.
 * @param  CipherData pointer to ciphertext buffer.
 * @param  IsLast flag of current package.
 * @retval AES calculation status.
 */
HAL_AES_StatusTypeDef HAL_AES_Encrypt(AES_HandleTypeDef *pParas, const uint8_t *PlainData, uint32_t PlainLen, uint8_t IsLast, uint8_t *CipherData)
{
    HAL_AES_StatusTypeDef tmp_err = AES_SUCCESS;
    pParas->engine.is_last = (uint8_t)IsLast;
#ifndef AES_CRYPT_ITEMS_IN_BYTES
    /* address of PlainData and CipherData must align by 32bit word */
    if (0U != (((uint32_t)PlainData | (uint32_t)CipherData) & 3U)) {
        tmp_err = AES_CRYPTKEY_ADDR_ALIGN_FAILURE;
    }
    if (AES_SUCCESS == tmp_err)
#endif
    {
        if (AES_SUCCESS != AES_Encrypt(pParas, PlainData, PlainLen, CipherData)) {
            tmp_err = AES_CRYPT_FAILURE;
        }
    }
    return tmp_err;
}

/**
 * @brief  Start AES decrypt calculation.
 * @param  pParas pointer to AES processing structure.
 * @param  CipherData pointer to ciphertext buffer.
 * @param  CipherLen size of ciphertext.
 * @param  PlainData pointer to plaintext buffer.
 * @param  IsLast flag of current package.
 * @retval AES calculation status.
 */
HAL_AES_StatusTypeDef HAL_AES_Decrypt(AES_HandleTypeDef *pParas, const uint8_t *CipherData, uint32_t CipherLen, uint8_t IsLast, uint8_t *PlainData)
{
    HAL_AES_StatusTypeDef tmp_err = AES_SUCCESS;
    pParas->engine.is_last = (uint8_t)IsLast;
#ifndef AES_CRYPT_ITEMS_IN_BYTES
    /* address of PlainData and CipherData must align by 32bit word */
    if (0U != (((uint32_t)CipherData | (uint32_t)PlainData) & 3U)) {
        tmp_err = AES_CRYPTKEY_ADDR_ALIGN_FAILURE;
    }
    if (AES_SUCCESS == tmp_err)
#endif
    {
        if (AES_SUCCESS != AES_Decrypt(pParas, CipherData, CipherLen, PlainData)) {
            tmp_err = AES_DECRYPT_FAILURE;
        }
    }
    return tmp_err;
}

/**
 * @brief  Start AES GMAC calculation.
 * @param  pParas pointer to AES processing structure.
 * @param  GhashData pointer to GHashData.
 * @param  GhashDataLen size of GHashData.
 * @param  IsLast flag of current package.
 * @retval AES calculation status.
 */
HAL_AES_StatusTypeDef HAL_AES_Gmac(AES_HandleTypeDef *pParas, const uint8_t *GhashData, uint32_t GhashDataLen, uint8_t IsLast)
{
    HAL_AES_StatusTypeDef tmp_err = AES_SUCCESS;
    pParas->engine.is_last = (uint8_t)IsLast;
#ifndef AES_CRYPT_ITEMS_IN_BYTES
    if (0U != (((uint32_t)GhashData) & 3U)) {
        tmp_err = AES_CRYPTKEY_ADDR_ALIGN_FAILURE;
    }
    if (AES_SUCCESS == tmp_err)
#endif
    {
        if (AES_SUCCESS != AES_Gmac(pParas, GhashData, GhashDataLen)) {
            tmp_err = AES_GMAC_FAILURE;
        }
    }
    return tmp_err;
}

/**
 * @brief  Save the module state and intermediate variables for this calculation.
 * @param  pParas pointer to AES processing structure.
 * @param  LatchBuf pointer to latch buffer.
 * @retval AES calculation status.
 */
HAL_AES_StatusTypeDef HAL_AES_LatchProcessStatus(AES_HandleTypeDef *pParas, AES_HandleTypeDef *LatchBuf)
{
    return AES_LatchStatus((const AES_HandleTypeDef *)pParas, LatchBuf);
}

/**
 * @brief  Recover the last calculated module state and intermediate variables.
 * @param  RecoverBuf pointer to recover buffer.
 * @param  pParas pointer to AES processing structure.
 * @retval AES calculation status.
 */
HAL_AES_StatusTypeDef HAL_AES_RecoverProcessStatus(AES_HandleTypeDef *RecoverBuf, AES_HandleTypeDef *pParas)
{
    HAL_AES_StatusTypeDef tmp_err = AES_SUCCESS;
    AES_ModuleInit(RecoverBuf->engine.kernelRegion); /* restart AES CLK */
    if (((LL_AES_IR_ENCRYPT == RecoverBuf->engine.crypt_ir) || \
        (LL_AES_IR_DECRYPT == RecoverBuf->engine.crypt_ir)) && (AES_OFB >= RecoverBuf->engine.cryptMode)) {
        tmp_err = AES_RecoverStatus((const AES_HandleTypeDef *)RecoverBuf, pParas);
    } else {
        tmp_err = AES_ERROR;
    }
    return tmp_err;
}

/**
 * @brief  Start key warpping.
 * @param  pParas pointer to AES processing structure.
 * @param  PlainData pointer to plaintext buffer.
 * @param  PlainLen size of plaintext.
 * @param  CipherData pointer to ciphertext buffer.
 * @param  CipherLen size of ciphertext.
 * @retval AES calculation status.
 */
HAL_AES_StatusTypeDef HAL_AES_KeyWrapping(AES_HandleTypeDef *pParas, const uint8_t *PlainData, uint32_t PlainLen, uint8_t *CipherData, uint32_t *CipherLen)
{
    HAL_AES_StatusTypeDef tmp_err = AES_SUCCESS;
    if ((PlainData == 0U) || (CipherData == 0U)) {
        tmp_err = AES_NULL_POINTER_FAILURE;
    } else {
        if (pParas->config.cryptMode == AES_ECB) {
            if (AES_KW_MODE_KWP == pParas->config.keyWrapMode) {
                tmp_err = AES_KeyWrappingwithPadding(pParas, PlainData, PlainLen, CipherData, CipherLen);
            } else if (AES_KW_MODE_KW == pParas->config.keyWrapMode) {
                tmp_err = AES_KeyWrapping(pParas, PlainData, PlainLen, CipherData, CipherLen);
            } else {
                tmp_err = AES_ERROR;
            }
        } else {
            tmp_err = AES_KW_MODE_FAILURE;
        }
    }
    return tmp_err;
}

/**
 * @brief  Start key upwrapping.
 * @param  pParas pointer to AES processing structure.
 * @param  CipherData pointer to ciphertext buffer.
 * @param  CipherLen size of ciphertext.
 * @param  PlainData pointer to plaintext buffer.
 * @param  PlainLen size of plaintext.
 * @retval AES calculation status.
 */
HAL_AES_StatusTypeDef HAL_AES_KeyUnwrapping(AES_HandleTypeDef *pParas, const uint8_t *CipherData, uint32_t CipherLen, uint8_t *PlainData, uint32_t *PlainLen)
{
    HAL_AES_StatusTypeDef tmp_err = AES_SUCCESS;
    if ((CipherData == 0U) || (PlainData == 0U)) {
        tmp_err = AES_NULL_POINTER_FAILURE;
    } else {
        if (pParas->config.cryptMode == AES_ECB) {
            if (AES_KW_MODE_KWP == pParas->config.keyWrapMode) {
                if ((CipherLen < (AES_KW_SEMIBLOCK_LEN * 2U)) || (0U != (CipherLen & 0x7U))) {
                    tmp_err = AES_KW_INLEN_FAILURE;
                } else {
                    tmp_err = AES_KeyUnwrappingwithPadding(pParas, CipherData, CipherLen, PlainData, PlainLen);
                }
            } else if (AES_KW_MODE_KW == pParas->config.keyWrapMode) {
                tmp_err = AES_KeyUnwrapping(pParas, CipherData, CipherLen, PlainData, PlainLen, 0U);
            } else {
                tmp_err = AES_ERROR;
            }
        } else {
            tmp_err = AES_KW_MODE_FAILURE;
        }
    }
    return tmp_err;
}

/**
 * @brief  Specify additional authenticated data.
 * @param  pParas pointer to AES processing structure.
 * @param  DataIn pointer to aad.
 * @param  DataLen size of aad.
 * @retval AES calculation status.
 */
HAL_AES_StatusTypeDef HAL_AES_SetAAD(AES_HandleTypeDef *pParas, const uint8_t *DataIn, uint32_t DataLen)
{
    HAL_AES_StatusTypeDef tmp_err = AES_SUCCESS;
    if (DataIn == 0U) {
        tmp_err = AES_NULL_POINTER_FAILURE;
    } else {
        tmp_err = AES_SetAAD(pParas, DataIn, DataLen);
    }
    return tmp_err;
}

/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup AES_Private_Functions
 * @{
 */

/**
 * @brief  Used for storage mode conversion.
 * @note   For example, convert big-endian data to little-endian data.
 *
 * @param  p_disc pointer to dest data buffer.
 * @param  p_src  pointer to source data buffer.
 * @param  size size of source data buffer in bytes.
 * @retval None
 */
static void AES_ConvertEndian(uint32_t *p_disc, const uint32_t *p_src, uint32_t size)
{
    typedef struct {
        uint8_t data8[0x20000];
    } AES_REGS_BYTEMSK;
    typedef struct {
        uint16_t data16[0x10000];
    } AES_REGS_HWORDMSK;
    typedef struct {
        uint32_t data32[0x8000];
    } AES_REGS_WORDMSK;
    uint32_t i, j, temp_data_u32;
    AES_REGS_BYTEMSK *tmp_datamsk;
    AES_REGS_HWORDMSK *tmp_hword_datamsk;

    const AES_REGS_WORDMSK *tmp_word_src = (const AES_REGS_WORDMSK *)(uint32_t)p_src;
    AES_REGS_WORDMSK *tmp_word_disc = (AES_REGS_WORDMSK *)(uint32_t)p_disc;
    for (i = 0U; i < (size >> 2U); i++) {
        tmp_word_disc->data32[i] = (uint32_t)(__REV(tmp_word_src->data32[i]));
    }

    j = (size & 3U); /* When size is not aligned by word.*/
    if (0U != j) {
        temp_data_u32 = (uint32_t)(__REV(tmp_word_src->data32[i]));
        tmp_datamsk = (AES_REGS_BYTEMSK *)(uint32_t)&tmp_word_disc->data32[i];
        tmp_hword_datamsk = (AES_REGS_HWORDMSK *)(uint32_t)&tmp_word_disc->data32[i];
        if (1U == j) {
            tmp_datamsk->data8[0] = (uint8_t)temp_data_u32;
        } else if (2U == j) {
            tmp_hword_datamsk->data16[0] = (uint16_t)temp_data_u32;
        } else {
            tmp_hword_datamsk->data16[0] = (uint16_t)temp_data_u32;
            tmp_datamsk->data8[2] = (uint8_t)(temp_data_u32 >> 16U);
        }
    }
}

/**
 * @brief  Set a segment of dst memory to specified data, in words.
 *
 * @param  dst pointer to destination buffer.
 * @param  data specified value.
 * @param  n number of set times.
 * @retval None
 */
static void AES_MemsetWord(uint32_t dst[], uint32_t data, uint32_t n)
{
    uint32_t i;
    for (i = 0U; i < n; i++) {
        dst[i] = data;
    }
}

/**
 * @brief  Copy n words from source address to destination address.
 *
 * @param  dst pointer to destination buffer.
 * @param  src pointer to source buffer.
 * @param  n number of copy length.
 * @retval None
 */
static void AES_Memcpy(uint8_t dst[], const uint8_t src[], uint32_t n)
{
    uint32_t i;
    for (i = 0U; i < n; i++) {
        dst[i] = src[i];
    }
}

/**
 * @brief  Enable SCU clk.
 *
 * @retval Original SCU clk.
 */
static uint32_t AES_EnableSCUClk(void)
{
    uint32_t sysps_bak, scu_clk_tmp = 0U;
    AES_KernelTypeDef kernel = AES_RUNNING_IN_CORE1;
    sysps_bak = MEM_U32(SYS_PSW_ADDR(kernel));
    MEM_U32(SYS_PSW_ADDR(kernel)) = 0x82U;
    scu_clk_tmp = (uint32_t)MEM_U32(SYS_PMOD0_ADDR(kernel)); /* save original SCU clock */
    if (1U != ((MEM_U32(SYS_PMOD0_ADDR(kernel)) & 0x8U) >> 3U)) {
        MEM_U32(SYS_PMOD0_ADDR(kernel)) |= ((uint32_t)1U << 3U); /* start SCU clock */
    }
    MEM_U32(SYS_PSW_ADDR(kernel)) = sysps_bak;
    return scu_clk_tmp;
}

/**
 * @brief  Recover the SCU clk.
 *
 * @param  clk Original SCU clk.
 * @retval None
 */
static void AES_RecoverSCUClk(uint32_t clk)
{
    uint32_t sysps_bak;
    AES_KernelTypeDef kernel = AES_RUNNING_IN_CORE1;
    sysps_bak = MEM_U32(SYS_PSW_ADDR(kernel));
    MEM_U32(SYS_PSW_ADDR(kernel)) = 0x82U;
    MEM_U32(SYS_PMOD0_ADDR(kernel)) = clk; /* reset SCU CLK */
    MEM_U32(SYS_PSW_ADDR(kernel)) = sysps_bak;
}

/**
 * @brief  Check the AES module is specified to CORE0 or CORE1.
 *
 * @retval State of bit (1 or 0).
 */
static uint32_t AES_GetSCUFlag(void)
{
    uint32_t flag, scu_clk_tmp = 0U;
    AES_KernelTypeDef kernel = AES_RUNNING_IN_CORE1;
    scu_clk_tmp = AES_EnableSCUClk();
    flag = ((MEM_U32(SCU_PERIPH_SEC_CFG2(kernel)) & 0x100U) >> 8U);
    AES_RecoverSCUClk(scu_clk_tmp);
    return flag;
}

/**
 * @brief  Check input address is valid or not.
 *
 * @retval State of bit (1 or 0).
 */
static uint32_t AES_IsAddressValid(uint32_t address)
{
    uint32_t core0_sram_size = 0, core1_sram_size = 0;
    uint32_t scu_clk_tmp = 0U, aes_assignedcore = 0U;
    uint32_t tmp_err = AES_SUCCESS;
    AES_KernelTypeDef kernel = AES_RUNNING_IN_CORE1;

    scu_clk_tmp = AES_EnableSCUClk();
    core0_sram_size = (uint32_t)((MEM_U32(SCU_SRAM_SEC_WM_CFG(kernel)) & 0xFU) * 16 * 1024U);
    core1_sram_size = (uint32_t)((0xA - (MEM_U32(SCU_SRAM_SEC_WM_CFG(kernel)) & 0xFU)) * 16 * 1024U);
    core0_sram_size -= 0x120U; /* Required SRAM size of AES Module.*/
    core1_sram_size -= 0x120U;
    aes_assignedcore = ((MEM_U32(SCU_PERIPH_SEC_CFG2(kernel)) & 0x100U) >> 8U);
    if (aes_assignedcore == 1U) { /* AES Module is assigned to Core0.*/
        if (address > (core0_sram_size + SYSCTRL_ADDR_MAP1_CORE0)) {
            tmp_err = AES_REMAP_ADDR_INVALID_FAILURE;
        }
    } else { /* AES Module is assigned to Core1.*/
        if (address > (core1_sram_size + SYSCTRL_ADDR_MAP1_CORE1)) {
            tmp_err = AES_REMAP_ADDR_INVALID_FAILURE;
        }
    }
    AES_RecoverSCUClk(scu_clk_tmp);
    return tmp_err;
}

/**
 * @brief  Initialize the AES module.
 *
 * @param  kernel input kernel type.
 * @retval None
 */
static void AES_ModuleInit(AES_KernelTypeDef kernel)
{
    unsigned int sysps_bak;
    sysps_bak = MEM_U32(SYS_PSW_ADDR(kernel));
    MEM_U32(SYS_PSW_ADDR(kernel)) = 0x82U;
    MEM_U32(SYS_MOD2_ADDR(kernel)) |= SYSCTRL_MOD2_AES_EN_MSK;
    MEM_U32(SYS_PSW_ADDR(kernel)) = sysps_bak;
}

/**
 * @brief  De-Initialize the AES module.
 *
 * @param  kernel input kernel type.
 * @retval None
 */
static void AES_ModuleDeInit(AES_KernelTypeDef kernel)
{
    unsigned int sysps_bak;
    sysps_bak = MEM_U32(SYS_PSW_ADDR(kernel));
    MEM_U32(SYS_PSW_ADDR(kernel)) = 0x82U;
    MEM_U32(SYS_MOD2_ADDR(kernel)) &= ~SYSCTRL_MOD2_AES_EN_MSK;
    MEM_U32(SYS_PSW_ADDR(kernel)) = sysps_bak;
}

/**
 * @brief  Start the AES computing.
 *
 * @param  pParas pointer to AES processing structure.
 * @retval AES calculation status.
 */
static uint32_t AES_Computing(AES_HandleTypeDef *pParas)
{
    uint32_t i;
    uint32_t tmp_err = AES_SUCCESS;
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);

    LL_AES_SetIRType(AESx, (pParas->engine.crypt_ir << AES_CTRL_IR_POS));
    LL_AES_SetKeyLength(AESx, (pParas->config.bitWideMode << AES_CTRL_SIZE_POS));
    LL_AES_SetDataStreamMode(AESx, (pParas->engine.cryptMode << AES_CTRL_MODE_POS));
    LL_AES_ClearFlag_AESDONE(AESx);
    LL_AES_Enable(AESx);
    i = 0U;
    while (!LL_AES_IsActiveFlag_AESDONE(AESx)) {
        if ((i++) > 400U) {
            tmp_err = AES_ERROR;
            break;
        }
    }

    return tmp_err;
}

/**
 * @brief  Start the AES-128/192/256 Key expansion.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  Key pointer to private key buffer.
 * @retval AES calculation status.
 */
static uint32_t AES_ExpendKey(AES_HandleTypeDef *pParas, const uint32_t p_key[])
{
    uint32_t i, key_words;
    uint32_t tmp_err = AES_SUCCESS;
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);
    AES_KeyExpTypeDef *haes_keyexp = (AES_KeyExpTypeDef *)AES_KEYEXP_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    AES_CipherTypeDef *haes_cipherin;

    if (LL_AES_IR_ENCRYPT == pParas->engine.crypt_ir) {
        haes_cipherin = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    } else {
        haes_cipherin = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    }

    for (i = 0U; i < 4U; i++) {
        haes_cipherin->DATA[i] = 0U;
        AESx->IV[i] = 0U;
    }
    key_words = ((AES_128 == (uint32_t)pParas->config.bitWideMode)
                ? 4U
                : ((AES_192 == (uint32_t)pParas->config.bitWideMode) ? 6U : 8U));
    for (i = 0U; i < key_words; i++) {
        haes_keyexp->KEY_EXP[i] = (uint32_t)(__REV(p_key[i]));
    }
    LL_AES_SetAddress(AESx, pParas->engine.dma_addr);
    LL_AES_SetIRType(AESx, (LL_AES_IR_KEYEXP << AES_CTRL_IR_POS));
    LL_AES_SetKeyLength(AESx, (pParas->config.bitWideMode << AES_CTRL_SIZE_POS));
    LL_AES_ClearFlag_CRYPTDONE(AESx);
    LL_AES_Enable(AESx);
    i = 0U;
    while (!LL_AES_IsActiveFlag_CRYPTDONE(AESx)) {
        if ((i++) > 600U) {
            tmp_err = AES_ERROR; /* Max 162 clk.*/
            break;
        }
    }

    return tmp_err;
}

/**
 * @brief  Generates the global 128-bit H parameter.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  H pointer to H value buffer.
 * @retval AES calculation status.
 */
static uint32_t AES_GHASH_CalculateH(AES_HandleTypeDef *pParas, uint32_t H[4])
{
    uint32_t i;
    uint32_t tmp_err = AES_SUCCESS;
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);
    AES_CipherTypeDef *haes_cipherin, *haes_cipherout;

    if (LL_AES_IR_ENCRYPT == pParas->engine.crypt_ir) {
        haes_cipherin = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
        haes_cipherout = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    } else {
        haes_cipherin = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
        haes_cipherout = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    }

    for (i = 0U; i < 4U; i++) {
        haes_cipherin->DATA[i] = 0U;
        AESx->IV[i] = 0U;
    }
    LL_AES_SetIRType(AESx, (LL_AES_IR_ENCRYPT << AES_CTRL_IR_POS));
    LL_AES_SetKeyLength(AESx, (pParas->config.bitWideMode << AES_CTRL_SIZE_POS));
    LL_AES_SetDataStreamMode(AESx, (AES_CTR << AES_CTRL_MODE_POS));
    LL_AES_ClearFlag_CRYPTDONE(AESx);
    LL_AES_Enable(AESx);
    i = 0U;
    while (!LL_AES_IsActiveFlag_CRYPTDONE(AESx)) {
        if ((i++) > 400U) {
            tmp_err = AES_ERROR; /* Max 70 clk.*/
            break;
        }
    }
    for (i = 0U; i < 4U; i++) {
        H[i] = haes_cipherout->DATA[3U - i]; /* Save global H.*/
    }

    return tmp_err;
}

/**
 * @brief  Start the GHASH computing.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  pDataIn pointer to input data buffer.
 * @param  IsStart running flag.
 * @param  EndianOrder Data storage order:Big-endian or Little-endian.
 * @retval AES calculation status.
 */
static uint32_t AES_GHASH_Computing(AES_HandleTypeDef *pParas, const uint32_t pDataIn[], uint8_t *is_start, AES_ENDIAN_MODE little_endian)
{
    uint32_t i, tmp;
    uint32_t tmp_err = AES_SUCCESS;
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);

    if (0U != *(is_start)) {
        *(is_start) = 0U;
        if (AES_LITTLE_ENDIAN == little_endian) {
            for (i = 0U; i < 4U; i++) {
                AESx->GHASH_Y[i] = pParas->proc.h_default[i]; /* Y = H.*/
                AESx->GHASH_X[i] = (pParas->proc.h_predata[i] ^ ((uint32_t)(__REV(pDataIn[3U - i])))); /* X = DIN.*/
            }
        } else {
            for (i = 0U; i < 4U; i++) {
                AESx->GHASH_Y[i] = pParas->proc.h_default[i]; /* Y = H.*/
                AESx->GHASH_X[i] = (pParas->proc.h_predata[i] ^ pDataIn[3U - i]); /* X = DIN.*/
            }
        }
    } else {
        if (AES_LITTLE_ENDIAN == little_endian) {
            for (i = 0U; i < 4U; i++) {
                tmp = ((uint32_t)(__REV(pDataIn[3U - i]))); /* X = X_d^DIN.*/
                AESx->GHASH_X[i] = (AESx->GHASH_Z[i] ^ tmp);
            }
        } else {
            for (i = 0U; i < 4U; i++) {
                AESx->GHASH_X[i] = (AESx->GHASH_Z[i] ^ pDataIn[3U - i]); /* X = X_d^DIN.*/
            }
        }
    }
    LL_AES_SetIRType(AESx, (LL_AES_IR_GF128 << AES_CTRL_IR_POS));
    LL_AES_ClearFlag_CRYPTDONE(AESx);
    LL_AES_Enable(AESx);
    i = 0U;
    while (!LL_AES_IsActiveFlag_CRYPTDONE(AESx)) {
        if ((i++) > 400U) {
            tmp_err = AES_ERROR; /* Max 128 clk.*/
            break;
        }
    }

    return tmp_err;
}

/**
 * @brief  Start 128-bit Galois Field multiplication.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  pDataX Multiplicand X.
 * @param  pDataY Multiplier Y.
 * @param  pDataZ Result Z.
 * @retval AES calculation status.
 */
static uint32_t AES_GF128Mult(AES_HandleTypeDef *pParas, const uint8_t *pDataX, const uint8_t *pDataY, uint8_t *pDataZ)
{
    uint32_t tmp_err = AES_SUCCESS;
    uint32_t i, tmp_data_x[4], tmp_data_y[4];
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);

    AES_Memcpy((uint8_t *)tmp_data_x, (const uint8_t *)pDataX, 16U);
    AES_Memcpy((uint8_t *)tmp_data_y, (const uint8_t *)pDataY, 16U);
    for (i = 0U; i < 4U; i++) {
        AESx->GHASH_X[i] = (uint32_t)(__REV(tmp_data_x[3U - i]));
        AESx->GHASH_Y[i] = (uint32_t)(__REV(tmp_data_y[3U - i]));
    }
    LL_AES_SetIRType(AESx, (LL_AES_IR_GF128 << AES_CTRL_IR_POS));
    LL_AES_ClearFlag_CRYPTDONE(AESx);
    LL_AES_Enable(AESx);
    i = 0U;
    while (!LL_AES_IsActiveFlag_CRYPTDONE(AESx)) {
        if ((i++) > 400U) {
            tmp_err = AES_ERROR; /* Max 128 clk.*/
            break;
        }
    }
    for (i = 0U; i < 4U; i++) {
        tmp_data_x[i] = (uint32_t)(__REV(AESx->GHASH_Z[3U - i]));
    }
    AES_Memcpy((uint8_t *)pDataZ, (const uint8_t *)tmp_data_x, 16U);

    return tmp_err;
}

/**
 * @brief  Calculates the y0 parameters.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  IVData pointer to IV buffer.
 * @param  PlainData pointer to plaintext buffer.
 * @param  CipherData pointer to ciphertext buffer.
 * @retval AES calculation status.
 */
static uint32_t AES_CalculateY0(AES_HandleTypeDef *pParas, const uint32_t ivData[], const uint32_t plainData[], uint32_t cryptedData[])
{
    uint32_t i;
    uint32_t tmp_err = AES_SUCCESS;
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);
    AES_CipherTypeDef *haes_cipherin, *haes_cipherout;

    if (LL_AES_IR_ENCRYPT == pParas->engine.crypt_ir) {
        haes_cipherin = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
        haes_cipherout = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    } else {
        haes_cipherin = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
        haes_cipherout = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    }

    for (i = 0U; i < 4U; i++) {
        haes_cipherin->DATA[i] = plainData[i];
        AESx->IV[i] = ivData[i];
    }
    tmp_err = AES_Computing(pParas);
    for (i = 0U; i < 4U; i++) {
        cryptedData[i] = haes_cipherout->DATA[i];
    }

    return tmp_err;
}

/**
 * @brief  AES computing manager in AES-ECB.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  IVData pointer to IV buffer.
 * @param  PlainData pointer to plaintext buffer.
 * @param  CipherData pointer to ciphertext buffer.
 * @retval AES calculation status.
 */
static uint32_t AES_ECB_Calculate(AES_HandleTypeDef *pParas, const uint32_t ivData[], const uint32_t plainData[], uint32_t cryptedData[])
{
    uint32_t i;
    uint32_t tmp_err = AES_SUCCESS;
    AES_CipherTypeDef *haes_cipherin, *haes_cipherout;

    if (LL_AES_IR_ENCRYPT == pParas->engine.crypt_ir) {
        haes_cipherin = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
        haes_cipherout = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    } else {
        haes_cipherin = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
        haes_cipherout = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    }

    haes_cipherin->DATA[0] = ivData[0];
    for (i = 0U; i < 4U; i++) {
        haes_cipherin->DATA[i] = (uint32_t)(__REV(plainData[i]));
    }
    tmp_err = AES_Computing(pParas);
    for (i = 0U; i < 4U; i++) {
        cryptedData[i] = haes_cipherout->DATA[i];
    }

    return tmp_err;
}

/**
 * @brief  AES computing manager in AES-CBC.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  IVData pointer to IV buffer.
 * @param  PlainData pointer to plaintext buffer.
 * @param  CipherData pointer to ciphertext buffer.
 * @retval AES calculation status.
 */
static uint32_t AES_CBC_Calculate(AES_HandleTypeDef *pParas, const uint32_t ivData[], const uint32_t plainData[], uint32_t cryptedData[])
{
    uint32_t i;
    uint32_t tmp_err = AES_SUCCESS;
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);
    AES_CipherTypeDef *haes_cipherin, *haes_cipherout;

    if (LL_AES_IR_ENCRYPT == pParas->engine.crypt_ir) {
        haes_cipherin = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
        haes_cipherout = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    } else {
        haes_cipherin = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
        haes_cipherout = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    }

    AESx->IV[0] = ivData[0];
    for (i = 0U; i < 4U; i++) {
        haes_cipherin->DATA[i] = (uint32_t)(__REV(plainData[i]));
        AESx->IV[i] = pParas->proc.cypher_predata[i];
    }
    tmp_err = AES_Computing(pParas);
    for (i = 0U; i < 4U; i++) {
        cryptedData[i] = haes_cipherout->DATA[i];
    }
    if (AES_enCrypt != pParas->config.encrypt_sel) {
        for (i = 0U; i < 4U; i++) {
            pParas->proc.cypher_predata[i] = haes_cipherin->DATA[i];
        }
    } else {
        for (i = 0U; i < 4U; i++) {
            pParas->proc.cypher_predata[i] = cryptedData[i];
        }
    }

    return tmp_err;
}

/**
 * @brief  AES computing manager in AES-CTR.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  IVData pointer to IV buffer.
 * @param  PlainData pointer to plaintext buffer.
 * @param  CipherData pointer to ciphertext buffer.
 * @retval AES calculation status.
 */
static uint32_t AES_CTR_Calculate(AES_HandleTypeDef *pParas, const uint32_t ivData[], const uint32_t plainData[], uint32_t cryptedData[])
{
    uint32_t i;
    uint32_t tmp_err = AES_SUCCESS;
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);
    AES_CipherTypeDef *haes_cipherin, *haes_cipherout;

    if (LL_AES_IR_ENCRYPT == pParas->engine.crypt_ir) {
        haes_cipherin = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
        haes_cipherout = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    } else {
        haes_cipherin = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
        haes_cipherout = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    }

    for (i = 0U; i < 4U; i++) {
        AESx->IV[i] = (uint32_t)(__REV(plainData[i]));
    }
    haes_cipherin->DATA[3] = ivData[3];
    tmp_err = AES_Computing(pParas);
    for (i = 0U; i < 4U; i++) {
        cryptedData[i] = haes_cipherout->DATA[i];
    }

    return tmp_err;
}

/**
 * @brief  AES computing manager in AES-CFB128.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  IVData pointer to IV buffer.
 * @param  PlainData pointer to plaintext buffer.
 * @param  CipherData pointer to ciphertext buffer.
 * @retval AES calculation status.
 */
static uint32_t AES_CFB128_Calculate(AES_HandleTypeDef *pParas, const uint32_t ivData[], const uint32_t plainData[], uint32_t cryptedData[])
{
    uint32_t i;
    uint32_t tmp_err = AES_SUCCESS;
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);
    AES_CipherTypeDef *haes_cipherin, *haes_cipherout;

    if (LL_AES_IR_ENCRYPT == pParas->engine.crypt_ir) {
        haes_cipherin = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
        haes_cipherout = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    } else {
        haes_cipherin = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
        haes_cipherout = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    }

    AESx->IV[0] = ivData[0];
    for (i = 0U; i < 4U; i++) {
        AESx->IV[i] = (uint32_t)(__REV(plainData[i]));
        haes_cipherin->DATA[i] = pParas->proc.cypher_predata[i];
    }
    tmp_err = AES_Computing(pParas);
    for (i = 0U; i < 4U; i++) {
        cryptedData[i] = haes_cipherout->DATA[i];
    }
    if (AES_enCrypt != pParas->config.encrypt_sel) {
        for (i = 0U; i < 4U; i++) {
            pParas->proc.cypher_predata[i] = AESx->IV[i];
        }
    } else {
        for (i = 0U; i < 4U; i++) {
            pParas->proc.cypher_predata[i] = cryptedData[i];
        }
    }

    return tmp_err;
}

/**
 * @brief  AES computing manager in AES-OFB.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  IVData pointer to IV buffer.
 * @param  PlainData pointer to plaintext buffer.
 * @param  CipherData pointer to ciphertext buffer.
 * @retval AES calculation status.
 */
static uint32_t AES_OFB_Calculate(AES_HandleTypeDef *pParas, const uint32_t ivData[], const uint32_t plainData[], uint32_t cryptedData[])
{
    uint32_t i;
    uint32_t tmp_err = AES_SUCCESS;
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);
    AES_CipherTypeDef *haes_cipherout;

    if (LL_AES_IR_ENCRYPT == pParas->engine.crypt_ir) {
        haes_cipherout = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    } else {
        haes_cipherout = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    }

    AESx->IV[0] = ivData[0];
    for (i = 0U; i < 4U; i++) {
        AESx->IV[i] = (uint32_t)(__REV(plainData[i]));
    }
    tmp_err = AES_Computing(pParas);
    for (i = 0U; i < 4U; i++) {
        cryptedData[i] = haes_cipherout->DATA[i];
    }

    return tmp_err;
}

/**
 * @brief  Loops calculation manager for GHASH.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  DataIn pointer to input buffer.
 * @param  size input data size.
 * @param  EndianOrder Data storage order:Big-endian or Little-endian.
 * @retval AES calculation status.
 */
static uint32_t AES_GHASH_Engine(AES_HandleTypeDef *pParas, const GHashData_t data_in[], uint32_t size, AES_ENDIAN_MODE little_endian)
{
    uint32_t i, loops, data_last_size, tmp_u32;
    uint32_t tmp_err = AES_SUCCESS;
    uint32_t tmp_buf[4];

    loops = (size >> AES_BLOCK_BSHIFT);
    data_last_size = (size & (AES_PLAIN_BSIZE - 1U));
    for (i = 0U; i < loops; i++) {
#ifdef AES_CRYPT_ITEMS_IN_BYTES
        tmp_u32 = (uint32_t)&data_in[AES_REGS_DIV_DATA_ITEM * i];
        AES_Memcpy((uint8_t *)tmp_buf, (const uint8_t *)tmp_u32, AES_REGS_DIV_DATA_ITEM);
        if (AES_SUCCESS !=
            AES_GHASH_Computing(pParas, (uint32_t *)&tmp_buf[0], &pParas->engine.is_start_ghash, little_endian)) {
            tmp_err = AES_ERROR;
            break;
        }
#else
        tmp_u32 = (uint32_t)&data_in[4U * i];
        if (AES_SUCCESS !=
            AES_GHASH_Computing(pParas, (uint32_t *)tmp_u32, &pParas->engine.is_start_ghash, little_endian)) {
            tmp_err = AES_ERROR;
        }
#endif
    }

    if (0U != data_last_size) {
        AES_MemsetWord(tmp_buf, 0U, 4U); /* clear buffer */
#ifdef AES_CRYPT_ITEMS_IN_BYTES
        AES_Memcpy((uint8_t *)tmp_buf, (const uint8_t *)&data_in[AES_REGS_DIV_DATA_ITEM * loops], data_last_size);
#else
        if (little_endian == AES_LITTLE_ENDIAN) {
            AES_Memcpy((uint8_t *)tmp_buf, (const uint8_t *)&data_in[AES_REGS_DIV_DATA_ITEM * loops], data_last_size);
        } else {
#ifdef AES_CRYPT_ITEMS_IN_BYTES
            i = (data_last_size & 0xFFFFFFFCU);
            AES_Memcpy((uint8_t *)tmp_buf, (const uint8_t *)&data_in[AES_REGS_DIV_DATA_ITEM * loops], i);
            if ((data_last_size & 3U) == 1U) {
                tmp_buf[data_last_size >> 2U] =
                     ((uint32_t)data_in[AES_REGS_DIV_DATA_ITEM * loops + i + 3U] << 24U);
            } else if ((data_last_size & 3U) == 2U) {
                tmp_buf[data_last_size >> 2U] =
                    (((uint32_t)data_in[AES_REGS_DIV_DATA_ITEM * loops + i + 3U] << 24U) |
                     ((uint32_t)data_in[AES_REGS_DIV_DATA_ITEM * loops + i + 2U] << 16U));
            } else if ((data_last_size & 3U) == 3U) {
                tmp_buf[data_last_size >> 2U] =
                    (((uint32_t)data_in[AES_REGS_DIV_DATA_ITEM * loops + i + 3U] << 24U) |
                     ((uint32_t)data_in[AES_REGS_DIV_DATA_ITEM * loops + i + 2U] << 16U) |
                     ((uint32_t)data_in[AES_REGS_DIV_DATA_ITEM * loops + i + 1U] << 8U));
            } else {
                /* Not need to care this branch.*/
            }
#else
            for (i = 0U; i < (data_last_size >> 2U); i++) {
                tmp_buf[i] = (uint32_t)data_in[(4U * loops) + i];
            }
            if ((data_last_size & 3U) == 1U) {
                tmp_buf[i] = ((uint32_t)data_in[(4U * loops) + i] & 0xFF000000U);
            } else if ((data_last_size & 3U) == 2U) {
                tmp_buf[i] = ((uint32_t)data_in[(4U * loops) + i] & 0xFFFF0000U);
            } else if ((data_last_size & 3U) == 3U) {
                tmp_buf[i] = ((uint32_t)data_in[(4U * loops) + i] & 0xFFFFFF00U);
            } else {
                /* Not need to care this branch.*/
            }
#endif
        }
#endif
        if (AES_SUCCESS != AES_GHASH_Computing(pParas, tmp_buf, &pParas->engine.is_start_ghash, little_endian)) {
            tmp_err = AES_ERROR;
        }
    }

    return tmp_err;
}

/**
 * @brief  AES computing mode manager.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  IVData pointer to IV buffer.
 * @param  PlainData pointer to plaintext buffer.
 * @param  CipherData pointer to ciphertext buffer.
 * @retval AES calculation status.
 */
static uint32_t AES_JumpMode(AES_HandleTypeDef *pParas, const uint32_t ivData[], const uint32_t plainData[], uint32_t cryptedData[])
{
    uint32_t tmp_err = AES_SUCCESS;

    if (AES_ECB == pParas->engine.cryptMode) {
        tmp_err = AES_ECB_Calculate(pParas, ivData, plainData, cryptedData);
    } else if (AES_CBC == pParas->engine.cryptMode) {
        tmp_err = AES_CBC_Calculate(pParas, ivData, plainData, cryptedData);
    } else if (AES_CTR == pParas->engine.cryptMode) {
        tmp_err = AES_CTR_Calculate(pParas, ivData, plainData, cryptedData);
    } else if (AES_CFB128 == pParas->engine.cryptMode) {
        tmp_err = AES_CFB128_Calculate(pParas, ivData, plainData, cryptedData);
    } else if (AES_OFB == pParas->engine.cryptMode) {
        tmp_err = AES_OFB_Calculate(pParas, ivData, plainData, cryptedData);
    } else {
        tmp_err = AES_ERROR;
    }

    return tmp_err;
}

/**
 * @brief  Loops calculation manager for AES encrypt and decrypt.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  PlainData pointer to plaintext buffer.
 * @param  size size of plaintext.
 * @param  CipherData pointer to ciphertext buffer.
 * @retval AES calculation status.
 */
static uint32_t AES_Crypt_Engine(AES_HandleTypeDef *pParas, const PlainData_t plainData[], uint32_t size, CryptData_t cryptedData[])
{
    uint32_t i, j, loops, data_last_size;
    uint32_t tmp_err = AES_SUCCESS;
    uint32_t tmp_plainBuf[4];
    uint32_t tmp_cryptBuf[4];
#ifndef AES_CRYPT_ITEMS_IN_BYTES
    uint32_t tmp_addr_plain = (uint32_t)plainData;
    uint32_t tmp_addr_crypt = (uint32_t)cryptedData;
#else
    uint32_t tmp_u32;
#endif
    AES_CipherTypeDef *haes_cipherin;

    if (LL_AES_IR_ENCRYPT == pParas->engine.crypt_ir) {
        haes_cipherin = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    } else {
        haes_cipherin = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    }

    loops = (size >> AES_BLOCK_BSHIFT);
    data_last_size = (size & (AES_PLAIN_BSIZE - 1U));
    for (i = 0U; i < loops; i++) {
#ifdef AES_CRYPT_ITEMS_IN_BYTES
        AES_MemsetWord(tmp_plainBuf, 0U, 4U);
        AES_MemsetWord(tmp_cryptBuf, 0U, 4U);
        tmp_u32 = (uint32_t)&plainData[AES_REGS_DIV_DATA_ITEM * i];
        AES_Memcpy((uint8_t *)tmp_plainBuf, (const uint8_t *)tmp_u32, 16U);
        if (AES_SUCCESS != AES_JumpMode(pParas, pParas->proc.iv, tmp_plainBuf, tmp_cryptBuf)) {
            tmp_err = AES_ERROR;
            break;
        }
        /* when aligned by bytes, then output is little endian, so crypt data should be endian switch */
        for (j = 0U; j < 4U; j++) {
            tmp_cryptBuf[j] = (uint32_t)(__REV(tmp_cryptBuf[j]));
        }
        tmp_u32 = (uint32_t)&cryptedData[AES_REGS_DIV_DATA_ITEM * i];
        AES_Memcpy((uint8_t *)tmp_u32, (const uint8_t *)&tmp_cryptBuf[0], 16U);
#else
        if (AES_SUCCESS !=
            AES_JumpMode(pParas, pParas->proc.iv, (uint32_t *)tmp_addr_plain, (uint32_t *)tmp_addr_crypt)) {
            tmp_err = AES_ERROR;
        }
        tmp_addr_plain += 16U;
        tmp_addr_crypt += 16U;
#endif
        pParas->proc.iv[3]++;

        if (0U == pParas->proc.iv[3]) {
            pParas->proc.iv[2]++;
            haes_cipherin->DATA[2] = pParas->proc.iv[2];
            if (0U == pParas->proc.iv[2]) {
                pParas->proc.iv[1]++;
                haes_cipherin->DATA[1] = pParas->proc.iv[1];
                if (0U == pParas->proc.iv[1]) {
                    pParas->proc.iv[0]++;
                    haes_cipherin->DATA[0] = pParas->proc.iv[0];
                }
            }
        }
    }
    if (0U != data_last_size) {
        AES_MemsetWord(tmp_plainBuf, 0U, 4U); /* copy data */
        AES_Memcpy((uint8_t *)tmp_plainBuf, (const uint8_t *)&plainData[AES_REGS_DIV_DATA_ITEM * loops], data_last_size);
        if (AES_SUCCESS != AES_JumpMode(pParas, pParas->proc.iv, tmp_plainBuf, tmp_cryptBuf)) {
            tmp_err = AES_ERROR;
        }
#ifdef AES_CRYPT_ITEMS_IN_BYTES
        /* when aligned by bytes, then output is little endian. so encrypted data should be endian switch */
        /* cause, crypt buffer size may be not aligned by 32bit.*/
        for (j = 0U; j < 4U; j++) {
            tmp_cryptBuf[j] = (uint32_t)(__REV(tmp_cryptBuf[j]));
        }
        AES_Memcpy((uint8_t *)&cryptedData[AES_REGS_DIV_DATA_ITEM * loops], (const uint8_t *)&tmp_cryptBuf[0], data_last_size);
#else
        i = (data_last_size >> 2);
        if (0U != (data_last_size & 0x3U)) {
            i++;
        }
        for (j = 0U; j < i; j++) {
            cryptedData[(AES_REGS_DIV_DATA_ITEM * loops) + j] = tmp_cryptBuf[j];
        }
#endif
        pParas->proc.iv[3]++;
    }

    return tmp_err;
}

/**
 * @brief  Copy predata to h_predata in AES processing structure.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  Data pointer to predata buffer.
 */
static void AES_GHASH_Checkin(AES_HandleTypeDef *pParas, const uint32_t pre_data[4])
{
    uint32_t i;
    for (i = 0U; i < 4U; i++) {
        pParas->proc.h_predata[i] = pre_data[i];
    }
}

/**
 * @brief  Calculates IV and Y0.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  IV pointer to IV buffer.
 * @param  Y pointer to Y buffer.
 * @retval AES calculation status.
 */
static uint32_t AES_GCM_CalculateIVandY0(AES_HandleTypeDef *pParas, uint32_t IV[4], uint32_t Y[4])
{
    uint32_t tmp_err = AES_SUCCESS;
    uint32_t i;
    uint32_t tmp_data[4] = {0U, 0U, 0U, 0U};
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);

    AES_GHASH_Checkin(pParas, tmp_data);
    if (pParas->config.ivSize == (96U >> 3U)) {
        AES_Memcpy((uint8_t *)IV, (const uint8_t *)pParas->config.ivLocation, ((uint32_t)AES_BLOCK_BSIZE - (uint32_t)4U));
        AES_ConvertEndian(IV, IV, 12U);
        IV[3] = 1U;
    } else {
        if (AES_SUCCESS != AES_GHASH_Engine(pParas, (GHashData_t *)(uint32_t)pParas->config.ivLocation, pParas->config.ivSize, AES_LITTLE_ENDIAN)) {
            tmp_err = AES_ERROR;
        }
        tmp_data[3] = (pParas->config.ivSize << 3U); /* bitwide */
        if (AES_SUCCESS != AES_GHASH_Computing(pParas, tmp_data, &pParas->engine.is_start_ghash, AES_BIG_ENDIAN)) {
            tmp_err = AES_ERROR;
        }
        for (i = 0U; i < 4U; i++) {
            IV[i] = AESx->GHASH_Z[3U - i];
        }
        tmp_data[3] = 0U;
    }
    if (AES_SUCCESS != AES_CalculateY0(pParas, tmp_data, IV, Y)) {
        tmp_err = AES_ERROR;
    }
    IV[3]++;

    return tmp_err;
}

/**
 * @brief  Calculates the AES-GCM or GMAC Tag.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  AADSize size of AAD.
 * @param  MsgSize size of plaintext.
 * @param  pTag pointer to tag buffer.
 * @param  size size of tag.
 * @retval AES calculation status.
 */
static uint32_t AES_GCM_CalculateTag(AES_HandleTypeDef *pParas, uint32_t aad_size, uint32_t msg_size, aesTag_t *pTag, uint32_t size)
{
    uint32_t i, j;
    uint32_t tmp_u32_buf[4];
    uint32_t tmp_err = AES_SUCCESS;
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);

    tmp_u32_buf[0] = 0U;
    tmp_u32_buf[2] = 0U;
    tmp_u32_buf[1] = (aad_size << 3U);
    tmp_u32_buf[3] = (msg_size << 3U);
    if (AES_SUCCESS != AES_GHASH_Computing(pParas, tmp_u32_buf, &pParas->engine.is_start_ghash, AES_BIG_ENDIAN)) {
        tmp_err = AES_ERROR;
    }
    /* When size is not aligned by words, pTag size must be aligned by words.*/
    j = (((size & 3U) != 0U) ? ((size >> 2U) + 1U) : (size >> 2U));
    if (j > 4U) {
        j = 4U;
    }
    for (i = 0U; i < j; i++) {
        tmp_u32_buf[i] = ((uint32_t)(__REV(AESx->GHASH_Z[3U - i] ^ pParas->proc.h_y0[i])));
    }
    AES_Memcpy((uint8_t *)pTag, (const uint8_t *)tmp_u32_buf, size);

    return tmp_err;
}

/**
 * @brief  Top-processing function.
 *
 * @param  pParas pointer to AES processing structure.
 * @retval None
 */
static void AES_StartManager(AES_HandleTypeDef *pParas)
{
    uint32_t i;
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);
    AES_CipherTypeDef *haes_cipherin;

    if (LL_AES_IR_ENCRYPT == pParas->engine.crypt_ir) {
        haes_cipherin = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    } else {
        haes_cipherin = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    }

    if (AES_GCM == pParas->config.cryptMode) {
        for (i = 0U; i < 4U; i++) {
            pParas->proc.h_predata[i] = pParas->proc.h_aad[i];
            pParas->proc.iv[i] = pParas->proc.h_iv[i];
        }
        pParas->engine.is_start_ghash = 1U; /* Init GHash calculation.*/
        pParas->engine.cryptSize = 0U;
    } else if (AES_ECB != pParas->config.cryptMode) {
        for (i = 0U; i < 4U; i++) {
            pParas->proc.iv[i] = pParas->proc.h_iv[i];
            pParas->proc.cypher_predata[i] = pParas->proc.iv[i];
            haes_cipherin->DATA[i] = pParas->proc.iv[i];
            AESx->IV[i] = pParas->proc.iv[i];
        }
    } else {
        AES_MemsetWord(pParas->proc.iv, 0U, 4U);
    }
}

/**
 * @brief  Initialize the AES processing structure with input configuration.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  Configuration pointer to AES configuration structure.
 * @retval AES calculation status.
 */
static uint32_t AES_SetConfig(AES_HandleTypeDef *pParas, const AES_InitTypeDef *cfg)
{
    const uint8_t data_list_aes_key_size[3] = {AES128_KEYSIZE, AES192_KEYSIZE, AES256_KEYSIZE};
    const uint8_t data_list_aes_encrypt_ir[5] = {LL_AES_IR_ENCRYPT, LL_AES_IR_ENCRYPT, LL_AES_IR_ENCRYPT, LL_AES_IR_ENCRYPT, LL_AES_IR_ENCRYPT};
    const uint8_t data_list_aes_decrypt_ir[5] = {LL_AES_IR_DECRYPT, LL_AES_IR_DECRYPT, LL_AES_IR_ENCRYPT, LL_AES_IR_ENCRYPT, LL_AES_IR_ENCRYPT};
    const AES_CRYPT_MODE data_list_aes_crypt_mode[6] = {AES_ECB, AES_CBC, AES_CTR, AES_CFB128, AES_OFB, AES_CTR};
    uint32_t i;
    uint32_t tmp_err = AES_SUCCESS;
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);

    if (AES_256 < cfg->bitWideMode) {
        tmp_err = AES_ERROR;
    }
    if (AES_KW_MODE_KW != cfg->keyWrapMode) {
        pParas->config.keyWrapMode = AES_KW_MODE_KWP;
    } else {
        pParas->config.keyWrapMode = cfg->keyWrapMode;
    }
    pParas->config.encrypt_sel = cfg->encrypt_sel;
    pParas->config.bitWideMode = cfg->bitWideMode;
    pParas->config.cryptMode = cfg->cryptMode;
    if (0U != cfg->keyLocation) {
        AES_Memcpy((uint8_t *)pParas->proc.key, (const uint8_t *)cfg->keyLocation, data_list_aes_key_size[pParas->config.bitWideMode]);
        pParas->config.keyLocation = (aesKey_t *)(uint32_t)pParas->proc.key;
    }
    pParas->config.ivLocation = cfg->ivLocation;
    pParas->config.specialMode = cfg->specialMode;
    /* check SRAM offset valid.*/
    if (AES_SUCCESS == AES_IsAddressValid(cfg->assignedAddress)) {
        /* Initialize the logical address.*/
        pParas->config.assignedAddress = cfg->assignedAddress;
        /* Initialize the physical address.*/
        pParas->engine.dma_addr = (pParas->config.assignedAddress & AES_ADDR_ADDR);
        /* Initialize the required size of SRAM space.*/
        AES_MemsetWord((uint32_t *)pParas->config.assignedAddress, 0U, 72U);
    }
    if (AES_GCM != pParas->config.cryptMode) {
        pParas->config.ivSize = AES_PLAIN_BSIZE;
        pParas->engine.crypt_ir = ((AES_enCrypt == pParas->config.encrypt_sel)
                                  ? data_list_aes_encrypt_ir[pParas->config.cryptMode]
                                  : data_list_aes_decrypt_ir[pParas->config.cryptMode]);
    } else { /* Only effective in AES_GCM.*/
        pParas->config.ivSize = cfg->ivSize;
        pParas->config.aadLocation = cfg->aadLocation;
        pParas->config.aadSize = cfg->aadSize;
        pParas->config.tagSignBuf = cfg->tagSignBuf;
        pParas->config.tagVerifyBuf = cfg->tagVerifyBuf;
        pParas->config.tagSize = cfg->tagSize;
        pParas->config.tagVerifySta = cfg->tagVerifySta;

        pParas->engine.cryptSize = 0U;
        pParas->engine.aadSize = 0U;
        pParas->engine.crypt_ir = LL_AES_IR_ENCRYPT;
        pParas->engine.crypt_kernelMode = AES_IV_PLAIN3;
    }
    pParas->engine.cryptMode = data_list_aes_crypt_mode[cfg->cryptMode];

    if (tmp_err == AES_SUCCESS) {
        /* Start AES key expansion.*/
        if (AES_SUCCESS != AES_ExpendKey(pParas, (uint32_t *)(uint32_t)pParas->config.keyLocation)) {
            tmp_err = AES_ERROR;
        }
        if (AES_GCM == pParas->config.cryptMode) { /* Only effective in AES_GCM.*/
            /* calculate H.*/
            if (AES_SUCCESS !=
                AES_GHASH_CalculateH(pParas, pParas->proc.h_default)) {
                tmp_err = AES_ERROR;
            }
            /* Generate 128bit (IV | counter) and Y0.*/
            pParas->engine.is_start_ghash = 1U; /* Init GHash calculation.*/
            if (AES_SUCCESS != AES_GCM_CalculateIVandY0(pParas, pParas->proc.h_iv, pParas->proc.h_y0)) {
                tmp_err = AES_ERROR;
            }
            /* Start GHash aad message, generate F.*/
            if (0U != pParas->config.aadSize) {
                pParas->engine.is_start_ghash = 1U; /* Init GHash calculation.*/
                if (AES_SUCCESS !=
                    AES_GHASH_Engine(pParas, (GHashData_t *)pParas->config.aadLocation, \
                                    (uint32_t)pParas->config.aadSize, AES_LITTLE_ENDIAN)) {
                    tmp_err = AES_ERROR;
                }
                for (i = 0U; i < 4U; i++) {
                    pParas->proc.h_aad[i] = AESx->GHASH_Z[i];
                }
            } else {
                for (i = 0U; i < 4U; i++) {
                    pParas->proc.h_aad[i] = 0U;
                }
            }
        } else if (AES_ECB != pParas->config.cryptMode) {
            AES_Memcpy((uint8_t *)pParas->proc.h_iv, (const uint8_t *)pParas->config.ivLocation, AES_BLOCK_BSIZE);
            AES_ConvertEndian(pParas->proc.h_iv, pParas->proc.h_iv, 16U);
        } else {
            /* Not need to care this branch.*/
        }
        pParas->engine.is_start_crypt = 1U; /* Init AES calculation start flag.*/
        pParas->engine.is_start_cryptKernel = 1U;
        pParas->engine.is_start_ghash = 1U;
    }

    return tmp_err;
}

/**
 * @brief  AES encryption function.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  PlainData pointer to plaintext buffer.
 * @param  size size of plaintext.
 * @param  CipherData pointer to ciphertext buffer.
 * @retval AES calculation status.
 */
static uint32_t AES_Encrypt(AES_HandleTypeDef *pParas, const PlainData_t *plainData, uint32_t size, CryptData_t *cryptedData)
{
    uint32_t tmp_err = AES_SUCCESS;

    if (0U != pParas->engine.is_start_crypt) {
        pParas->engine.is_start_crypt = 0U;
        AES_StartManager(pParas);
    }
    /* Start encryption.*/
    if (AES_SUCCESS != AES_Crypt_Engine(pParas, plainData, size, cryptedData)) {
        tmp_err = AES_ERROR;
    }
    if (AES_GCM == pParas->config.cryptMode) {
        pParas->engine.cryptSize += size;
#ifdef AES_CRYPT_ITEMS_IN_BYTES
        if (AES_SUCCESS != AES_GHASH_Engine(pParas, (GHashData_t *)cryptedData, size, AES_LITTLE_ENDIAN)) {
            tmp_err = AES_ERROR;
        }
#else
        if (AES_SUCCESS != AES_GHASH_Engine(pParas, (GHashData_t *)cryptedData, size, AES_BIG_ENDIAN)) {
            tmp_err = AES_ERROR;
        }
#endif
        /* Calculate the tag.*/
        if (CypherDatasIsTheLast == pParas->engine.is_last) {
            if (AES_SUCCESS !=
                AES_GCM_CalculateTag(pParas, (pParas->config.aadSize + pParas->engine.aadSize), \
                                    pParas->engine.cryptSize, pParas->config.tagSignBuf, pParas->config.tagSize)) {
                tmp_err = AES_ERROR;
            }
        }
    }
#ifndef AES_CRYPT_ITEMS_IN_BYTES
    /* Reverse source cryptdata from SRAM, if needed.*/
    AES_ConvertEndian(cryptedData, cryptedData, size);
#endif
    /* Reset status.*/
    if (CypherDatasIsTheLast == pParas->engine.is_last) {
        pParas->engine.is_start_crypt = 1U;
    }

    return tmp_err;
}

/**
 * @brief  AES decryption function.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  CipherData pointer to ciphertext buffer.
 * @param  size size of ciphertext.
 * @param  PlainData pointer to plaintext buffer.
 * @retval AES calculation status.
 */
static uint32_t AES_Decrypt(AES_HandleTypeDef *pParas, const CryptData_t *cryptedData, uint32_t size, PlainData_t *plainData)
{
    uint32_t tmp_err = AES_SUCCESS;

    if (0U != pParas->engine.is_start_crypt) {
        pParas->engine.is_start_crypt = 0U;
        AES_StartManager(pParas);
    }
    if (AES_GCM == pParas->config.cryptMode) {
        pParas->engine.cryptSize += size;
        if (AES_SUCCESS != AES_GHASH_Engine(pParas, cryptedData, size, AES_LITTLE_ENDIAN)) {
            tmp_err = AES_ERROR;
        }
        /* Calculate the tag.*/
        if (CypherDatasIsTheLast == pParas->engine.is_last) {
            if (AES_SUCCESS !=
                AES_GCM_CalculateTag(pParas, (pParas->config.aadSize + pParas->engine.aadSize), \
                                    pParas->engine.cryptSize, pParas->config.tagVerifyBuf, pParas->config.tagSize)) {
                tmp_err = AES_ERROR;
            }
            if (0 != memcmp(pParas->config.tagVerifyBuf, pParas->config.tagSignBuf, pParas->config.tagSize)) {
                *pParas->config.tagVerifySta = AES_VERIFY_FAIL;
            } else {
                *pParas->config.tagVerifySta = AES_VERIFY_SUCCESS;
            }
        }
    }

#if (AES_DECRYPT_GCM_SPECIAL_MOD_TYPE != 0)
    if (pParas->config.specialMode != AES_SPC_GCM_VERIFY_ONLY)
#endif
    {
        /* Start decryption.*/
        if (AES_SUCCESS != AES_Crypt_Engine(pParas, cryptedData, size, plainData)) {
            tmp_err = AES_ERROR;
        }
#ifndef AES_CRYPT_ITEMS_IN_BYTES
        AES_ConvertEndian(plainData, plainData, size);
#endif
    }

    if (CypherDatasIsTheLast == pParas->engine.is_last) {
        pParas->engine.is_start_crypt = 1U;
    }

    return tmp_err;
}

/**
 * @brief  AES GMAC function.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  GHashData pointer to GHashData buffer.
 * @param  size size of GHashData.
 * @retval AES calculation status.
 */
static uint32_t AES_Gmac(AES_HandleTypeDef *pParas, const GHashData_t *ghashData, uint32_t size)
{
    uint32_t tmp_err = AES_SUCCESS;
    uint32_t i;
    aesTag_t *p_tag_buf;

    if (AES_GCM != pParas->config.cryptMode) {
        tmp_err = AES_ERROR;
    }
    if (0U != pParas->engine.is_start_crypt) {
        pParas->engine.is_start_crypt = 0U;
        if (pParas->config.specialMode == AES_SPC_GMAC_CryptData) {
            for (i = 0U; i < 4U; i++) {
                pParas->proc.h_predata[i] = pParas->proc.h_aad[i];
            }
        } else {
            for (i = 0U; i < 4U; i++) {
                pParas->proc.h_predata[i] = 0U;
            }
        }
        pParas->engine.is_start_ghash = 1U; /* Init GHash calculation.*/
        pParas->engine.cryptSize = 0U;
    }
    pParas->engine.cryptSize += size;
    if (AES_SUCCESS != AES_GHASH_Engine(pParas, ghashData, size, AES_LITTLE_ENDIAN)) {
        tmp_err = AES_ERROR;
    }
    /* Calculate the tag.*/
    if (CypherDatasIsTheLast == pParas->engine.is_last) {
        if (AES_enCrypt == pParas->config.encrypt_sel) {
            p_tag_buf = pParas->config.tagSignBuf;
        } else {
            p_tag_buf = pParas->config.tagVerifyBuf;
        }
        if (AES_SPC_GMAC_CryptData != pParas->config.specialMode) {
            if (AES_SUCCESS != AES_GCM_CalculateTag(pParas, pParas->engine.cryptSize, 0U, p_tag_buf, pParas->config.tagSize)) {
                tmp_err = AES_ERROR;
            }
        } else {
            if (AES_SUCCESS != AES_GCM_CalculateTag(pParas, pParas->config.aadSize, pParas->engine.cryptSize, p_tag_buf, pParas->config.tagSize)) {
                tmp_err = AES_ERROR;
            }
        }
        if (AES_deCrypt == pParas->config.encrypt_sel) {
            if (0 != memcmp(p_tag_buf, pParas->config.tagSignBuf, pParas->config.tagSize)) {
                *pParas->config.tagVerifySta = AES_VERIFY_FAIL;
            } else {
                *pParas->config.tagVerifySta = AES_VERIFY_SUCCESS;
            }
        }
        pParas->engine.is_start_crypt = 1U;
    }

    return tmp_err;
}

/**
 * @brief  AES setting AAD function.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  AADData pointer to AADData buffer.
 * @param  size size of AADData.
 * @retval AES calculation status.
 */
static uint32_t AES_SetAAD(AES_HandleTypeDef *pParas, const aesAAD_t *addData, uint32_t size)
{
    uint32_t tmp_err = AES_SUCCESS;
    uint32_t i;
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);

    if (AES_GCM != pParas->config.cryptMode) {
        tmp_err = AES_ERROR;
    }
    if (tmp_err == AES_SUCCESS) {
        AES_GHASH_Checkin(pParas, pParas->proc.h_aad); /* copy h_aad to h_predata */
        pParas->engine.is_start_ghash = 1U;            /* GHash init start */
        pParas->engine.aadSize += size;
        if (AES_SUCCESS == AES_GHASH_Engine(pParas, addData, size, AES_LITTLE_ENDIAN)) {
            for (i = 0U; i < 4U; i++) {
                pParas->proc.h_aad[i] = AESx->GHASH_Z[i]; /* save GHash result in h_aad */
            }
        } else {
            tmp_err = AES_ERROR;
        }
    }

    return tmp_err;
}

/**
 * @brief  Latch the current process status of the AES.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  LatchBuf pointer to latch buffer.
 * @retval AES calculation status.
 */
static uint32_t AES_LatchStatus(const AES_HandleTypeDef *pParas, AES_HandleTypeDef *latch_buf)
{
    uint32_t i;
    AES_TypeDef *AESx = (AES_TypeDef *)AES_MODULE_REMAP_ADDR(pParas->engine.kernelRegion);
    AES_CipherTypeDef *haes_cipherin;

    if (LL_AES_IR_ENCRYPT == pParas->engine.crypt_ir) {
        haes_cipherin = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    } else {
        haes_cipherin = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    }

    /* Save the AES config parameters.*/
    latch_buf->config = pParas->config;
    /* Save the AES engine parameters.*/
    latch_buf->engine = pParas->engine;
    /* Save the AES global parameters.*/
    latch_buf->proc = pParas->proc;
    if (AES_OFB == pParas->config.cryptMode) {
        for (i = 0U; i < 4U; i++) {
            latch_buf->proc.h_predata[i] = haes_cipherin->DATA[i];
            latch_buf->proc.iv[i] = AESx->IV[i];
        }
    } else if (AES_GCM == pParas->config.cryptMode) {
        for (i = 0U; i < 4U; i++) {
            latch_buf->proc.h_predata[i] = AESx->GHASH_Z[i];
        }
    } else {
        /* Not need to care this branch.*/
    }

    return AES_SUCCESS;
}

/**
 * @brief  Recover the aes process status use the latch parameters before.
 *
 * @param  RecoverBuf pointer to recover buffer.
 * @param  pParas pointer to AES processing structure.
 * @retval AES calculation status.
 */
static uint32_t AES_RecoverStatus(const AES_HandleTypeDef *recover_buf, AES_HandleTypeDef *pParas)
{
    uint32_t tmp_err = AES_SUCCESS;
    uint32_t i;
    AES_CipherTypeDef *haes_cipherin;

    /* Reset the AES config parameters.*/
    pParas->config = recover_buf->config;
    /* Reset the AES engine parameters.*/
    pParas->engine = recover_buf->engine;
    /* Reset the AES global parameters.*/
    pParas->proc = recover_buf->proc;

    if (LL_AES_IR_ENCRYPT == pParas->engine.crypt_ir) {
        haes_cipherin = (AES_CipherTypeDef *)AES_PT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    } else {
        haes_cipherin = (AES_CipherTypeDef *)AES_CT_ADDR(pParas->engine.kernelRegion, pParas->config.assignedAddress);
    }
    /* Restart the AES key expansion.*/
    if (AES_SUCCESS != AES_ExpendKey(pParas, (uint32_t *)(uint32_t)pParas->config.keyLocation)) {
        tmp_err = AES_ERROR;
    }
    if ((AES_GCM == pParas->config.cryptMode) || (AES_CTR == pParas->config.cryptMode)) {
        for (i = 0U; i < 3U; i++) {
            haes_cipherin->DATA[i] = pParas->proc.h_iv[i];
        }
        if (AES_GCM == pParas->config.cryptMode) {
            pParas->engine.is_start_ghash = 1U;
        }
    } else if (AES_OFB == pParas->config.cryptMode) {
        for (i = 0U; i < 4U; i++) {
            haes_cipherin->DATA[i] = pParas->proc.h_predata[i];
        }
    } else {
        /* Not need to care this branch.*/
    }

    return tmp_err;
}

/**
 * @brief  AES xor computing.
 *
 * @param  A pointer to input buffer.
 * @param  t input value.
 * @retval None
 */
static void AES_Xor(uint8_t *A, uint32_t t)
{
    uint32_t i, shift;
    for (i = 0; i < AES_KW_SEMIBLOCK_LEN; i++) {
        shift = (uint32_t)((AES_KW_SEMIBLOCK_LEN - 1U - i) * 8U);
        if (32U != shift) {
            A[i] ^= (uint8_t)((t >> shift) & 0xFFU);
        }
    }
}

/**
 * @brief  AES key wrapping function.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  PlainData pointer to plaintext buffer.
 * @param  PlainLen input data length.
 * @param  CipherData pointer to ciphertext buffer.
 * @param  CipherLen output data length.
 * @retval AES calculation status.
 */
static uint32_t AES_KeyWrapping(AES_HandleTypeDef *pParas, const PlainData_t *plainData, uint32_t in_len, CryptData_t *cryptedData, uint32_t *out_len)
{
    uint32_t tmp_err = AES_SUCCESS;
    uint32_t semiblocks, t, s;
    uint8_t in_buff[AES_KW_SEMIBLOCK_LEN * 2], out_buff[AES_KW_SEMIBLOCK_LEN * 2];
    uint8_t *R2, *A;

    AES_MemsetWord((uint32_t *)in_buff, 0U, 4U);
    AES_MemsetWord((uint32_t *)out_buff, 0U, 4U);
    if (AES_KW_MODE_KW == pParas->config.keyWrapMode) {
        AES_Memcpy(cryptedData, AES_KW_ICV1, AES_KW_SEMIBLOCK_LEN);
        AES_Memcpy((cryptedData + AES_KW_SEMIBLOCK_LEN), plainData, in_len);
    }

    semiblocks = ((in_len >> 3U) + 1U);
    s = (6U * (semiblocks - 1U));
    *out_len = 0U;

    if (semiblocks < 3U) {
        tmp_err = AES_ERROR;
    } else {
        R2 = (cryptedData + AES_KW_SEMIBLOCK_LEN);
        A = cryptedData;

        for (t = 1U; t <= s; t++) {
            AES_Memcpy(in_buff, A, AES_KW_SEMIBLOCK_LEN);
            AES_Memcpy((in_buff + AES_KW_SEMIBLOCK_LEN), R2, AES_KW_SEMIBLOCK_LEN);
            pParas->engine.is_last = ((t == s) ? (uint8_t)CypherDatasIsTheLast : (uint8_t)CypherDatasNotTheLast);
            if (AES_SUCCESS != AES_Encrypt(pParas, (const PlainData_t *)in_buff, 16U, (CryptData_t *)out_buff)) {
                tmp_err = AES_ERROR;
            }
            AES_Memcpy(A, out_buff, AES_KW_SEMIBLOCK_LEN);
            AES_Xor(A, t);
            AES_Memcpy(R2, (out_buff + AES_KW_SEMIBLOCK_LEN), AES_KW_SEMIBLOCK_LEN);
            R2 += AES_KW_SEMIBLOCK_LEN;
            if (R2 >= (cryptedData + (semiblocks * AES_KW_SEMIBLOCK_LEN))) {
                R2 = (cryptedData + AES_KW_SEMIBLOCK_LEN);
            }
        }
        *out_len = (semiblocks * AES_KW_SEMIBLOCK_LEN);
    }

    return tmp_err;
}

/**
 * @brief  AES key wrapping function with padding.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  PlainData pointer to plaintext buffer.
 * @param  PlainLen input data length.
 * @param  CipherData pointer to ciphertext buffer.
 * @param  CipherLen output data length.
 * @retval AES calculation status.
 */
static uint32_t AES_KeyWrappingwithPadding(AES_HandleTypeDef *pParas, const PlainData_t *plainData, uint32_t in_len, CryptData_t *cryptedData, uint32_t *out_len)
{
    uint32_t tmp_err = AES_SUCCESS;
    uint32_t tmp_u32;
    uint32_t plain_buff[4];
    uint32_t padding_length, plaintext_padded_length;

    AES_MemsetWord(plain_buff, 0U, 4U);
    AES_Memcpy((uint8_t *)cryptedData, AES_KW_ICV2, (AES_KW_SEMIBLOCK_LEN >> 1U));
    tmp_u32 = ((uint32_t)cryptedData + (AES_KW_SEMIBLOCK_LEN >> 1U));
    *((uint32_t *)tmp_u32) = (uint32_t)(__REV(in_len & 0xFFFFFFFFU));
    AES_Memcpy((uint8_t *)(cryptedData + ((AES_KW_SEMIBLOCK_LEN >> 1U) * 2U)), (const uint8_t *)plainData, in_len);

    if (0U != (in_len & 0x7U)) {
        padding_length = (((AES_KW_SEMIBLOCK_LEN >> 1U) * 2U) - (in_len & 0x7U));
    } else {
        padding_length = 0U;
    }
    memset((uint8_t *)(cryptedData + in_len + ((AES_KW_SEMIBLOCK_LEN >> 1U) * 2U)), 0U, padding_length);
    plaintext_padded_length = (in_len + padding_length);

    tmp_u32 = (uint32_t)cryptedData;
    if (8U == plaintext_padded_length) {
        AES_Memcpy((uint8_t *)plain_buff, (uint8_t *)cryptedData, 16U);
        pParas->engine.is_last = (uint8_t)CypherDatasIsTheLast;
        if (AES_SUCCESS != AES_Encrypt(pParas, (const PlainData_t *)tmp_u32, 16U, (CryptData_t *)cryptedData)) {
            tmp_err = AES_ERROR;
        }
        *out_len = 16U;
    } else {
        if (AES_SUCCESS != AES_KeyWrapping(pParas, (const PlainData_t *)(tmp_u32 + AES_KW_SEMIBLOCK_LEN), plaintext_padded_length, cryptedData, out_len)) {
            tmp_err = AES_ERROR;
        }
    }

    return tmp_err;
}

/**
 * @brief  AES key unwrapping function.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  CipherData pointer to ciphertext buffer.
 * @param  PlainLen input data length.
 * @param  PlainData pointer to plaintext buffer.
 * @param  CipherLen output data length.
 * @param  ICVData
 * @retval AES calculation status.
 */
static uint32_t AES_KeyUnwrapping(AES_HandleTypeDef *pParas, const CryptData_t *cryptedData, uint32_t in_len, PlainData_t *plainData, uint32_t *out_len, uint8_t *icvData)
{
    uint32_t tmp_err = AES_SUCCESS;
    uint8_t *R;
    uint8_t A[AES_KW_SEMIBLOCK_LEN];
    uint32_t semiblocks, t, s;
    uint8_t in_buff[AES_KW_SEMIBLOCK_LEN * 2], out_buff[AES_KW_SEMIBLOCK_LEN * 2];

    AES_MemsetWord((uint32_t *)in_buff, 0U, 4U);
    AES_MemsetWord((uint32_t *)out_buff, 0U, 4U);
    AES_MemsetWord((uint32_t *)A, 0U, 2U);
    if ((AES_KW_MODE_KW == pParas->config.keyWrapMode) && (in_len < 24U)) {
        tmp_err = AES_ERROR;
    }

    semiblocks = (in_len >> 3U);
    s = (6 * (semiblocks - 1));
    *out_len = 0U;

    if ((semiblocks < 3U) || (tmp_err == AES_ERROR)) {
        tmp_err = AES_KW_INLEN_FAILURE;
    } else {
        AES_Memcpy(A, cryptedData, AES_KW_SEMIBLOCK_LEN);
        AES_Memcpy(plainData, (cryptedData + AES_KW_SEMIBLOCK_LEN), ((semiblocks - 1) * AES_KW_SEMIBLOCK_LEN));
        R = (plainData + ((semiblocks - 2U) * AES_KW_SEMIBLOCK_LEN));

        for (t = s; t >= 1; t--) {
            AES_Xor(A, t);

            AES_Memcpy(in_buff, A, AES_KW_SEMIBLOCK_LEN);
            AES_Memcpy((in_buff + AES_KW_SEMIBLOCK_LEN), R, AES_KW_SEMIBLOCK_LEN);

            pParas->engine.is_last = ((t == 1) ? (uint8_t)CypherDatasIsTheLast : (uint8_t)CypherDatasNotTheLast);
            if (AES_SUCCESS != AES_Decrypt(pParas, (const CryptData_t *)in_buff, 16U, (PlainData_t *)out_buff)) {
                tmp_err = AES_ERROR;
            }

            AES_Memcpy(A, out_buff, AES_KW_SEMIBLOCK_LEN);
            AES_Memcpy(R, (out_buff + AES_KW_SEMIBLOCK_LEN), AES_KW_SEMIBLOCK_LEN);
            if (R == plainData) {
                R = (plainData + ((semiblocks - 2) * AES_KW_SEMIBLOCK_LEN));
            } else {
                R -= AES_KW_SEMIBLOCK_LEN;
            }
        }

        *out_len = ((semiblocks - 1) * AES_KW_SEMIBLOCK_LEN);
    }

    if (0U != icvData) {
        AES_Memcpy(icvData, A, AES_KW_SEMIBLOCK_LEN);
    } else {
        if (0 != memcmp(AES_KW_ICV1, A, AES_KW_SEMIBLOCK_LEN)) {
            tmp_err = AES_KW_CHECK_ICV_FAILURE;
        }
    }

    return tmp_err;
}

/**
 * @brief  AES key unwarpping function with padding.
 *
 * @param  pParas pointer to AES processing structure.
 * @param  CipherData pointer to ciphertext buffer.
 * @param  PlainLen input data length.
 * @param  PlainData pointer to plaintext buffer.
 * @param  CipherLen output data length.
 * @retval AES calculation status.
 */
static uint32_t AES_KeyUnwrappingwithPadding(AES_HandleTypeDef *pParas, const CryptData_t *cryptedData, uint32_t in_len, PlainData_t *plainData, uint32_t *out_len)
{
    uint32_t tmp_err = AES_SUCCESS;
    uint8_t icvData[8], tmp_buf[16];
    uint8_t *p, *q;
    uint32_t src_len, tmp_u32;

    AES_MemsetWord((uint32_t *)icvData, 0U, 2U);
    if (in_len == (AES_KW_SEMIBLOCK_LEN * 2U)) {
        AES_MemsetWord((uint32_t *)tmp_buf, 0U, 4U);
        pParas->engine.is_last = (uint8_t)CypherDatasIsTheLast;
        if (AES_SUCCESS != AES_Decrypt(pParas, (const CryptData_t *)cryptedData, (AES_KW_SEMIBLOCK_LEN * 2U), (PlainData_t *)tmp_buf)) {
            tmp_err = AES_ERROR;
        }

        AES_Memcpy(icvData, (uint8_t *)tmp_buf, AES_KW_SEMIBLOCK_LEN);
        AES_Memcpy((uint8_t *)plainData, (uint8_t *)(tmp_buf + AES_KW_SEMIBLOCK_LEN), AES_KW_SEMIBLOCK_LEN);
        *out_len = AES_KW_SEMIBLOCK_LEN;
    } else { /* in_len >= (AES_KW_SEMIBLOCK_LEN * 3) */
        if (AES_SUCCESS != AES_KeyUnwrapping(pParas, (const CryptData_t *)cryptedData, in_len, plainData, out_len, icvData)) {
            tmp_err = AES_ERROR;
        }
    }

    /* check ICV */
    if (0 != memcmp(AES_KW_ICV2, icvData, (AES_KW_SEMIBLOCK_LEN >> 1))) {
        tmp_err = AES_KW_CHECK_ICV_FAILURE;
    }

    if (tmp_err == AES_SUCCESS) {
        /* get the plaintext source length without padding */
        tmp_u32 = ((uint32_t)icvData + (AES_KW_SEMIBLOCK_LEN >> 1));
        src_len = (uint32_t)(__REV(*((uint32_t *)tmp_u32)));
        if ((src_len > *out_len) || ((*out_len > AES_KW_SEMIBLOCK_LEN) && (src_len < (*out_len - 7U)))) {
            tmp_err = AES_ERROR;
        }

        /* check padding */
        p = (plainData + src_len);
        q = (plainData + *out_len);
        while (p < q) {
            if (*p++ != 0) {
                tmp_err = AES_KW_CHECK_PADDING_FAILURE;
                break;
            }
        }

        *out_len = src_len;
    }

    return tmp_err;
}
/**
 * @}
 */

#endif /* HAL_AES_MODULE_ENABLED */

/**
 * @}
 */

/**
 * @}
 */
