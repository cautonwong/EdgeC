/******************************************************************************
 * @file        rn8xxx_ll_sea_aes.c
 * @brief       AES functions
 * @author      Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 *****************************************************************************/
#include "rn8xxx_ll_sea_aes.h"

#if !defined(RN831x_RN861x_MCU_V4)

static ErrStatus aes_drv_KernelCalc(void);
static ErrStatus aes_drv_expand_key(const regData_t p_key[], uint32_t size, uint32_t aes_len_mod);
static ErrStatus aes_drv_ghash_h_generate(uint32_t aes_len_mod, regData_t H[4]);
static ErrStatus aes_drv_ghash_unit(const regData_t pDataIn[], uint8_t *is_start, SEA_ENDIAN_MOD little_endian);
static ErrStatus aes_drv_crypt_noRev(const regData_t  ivData[], const regData_t plainData[], regData_t cryptedData[]);
static ErrStatus aes_drv_cryptKernel_ECB(const regData_t ivData[], const regData_t plainData[], regData_t cryptedData[]);
static ErrStatus aes_drv_cryptKernel_CBC(const regData_t  ivData[], const regData_t  plainData[], regData_t  cryptedData[]);
static ErrStatus aes_drv_cryptKernel_CTR(const regData_t ivData[], const regData_t  plainData[], regData_t  cryptedData[]);
static ErrStatus aes_drv_cryptKernel_CFB128(const regData_t ivData[], const regData_t plainData[], regData_t cryptedData[]);
static ErrStatus aes_drv_cryptKernel_OFB(const regData_t ivData[], const regData_t plainData[], regData_t cryptedData[]);
static ErrStatus aes_drv_ghash_engine(const ghashData_t data_in[], uint32_t size, SEA_ENDIAN_MOD little_endian);
static ErrStatus aes_crypt_engine(const plainData_t  plainData[], uint32_t size, cryptData_t  cryptedData[]);
static void aes_ghash_checkin(const regData_t pre_data[4]);
static ErrStatus aes_crypt_gcm_iv_y0_generate(regData_t IV[4], regData_t Y[4]);
static ErrStatus aes_crypt_gcm_yfc_out_generate(uint32_t add_size, uint32_t msg_size, tag_t *pTag, uint32_t size);
static void aes_crypt_start_manager(void);

static ErrStatus aes_parameters_init(const aesCypherConfigStr*  configuration);
static ErrStatus aes_crypt_sign_engine(const plainData_t* plainData, uint32_t size, CYPHER_DATA_STA is_last, cryptData_t* cryptedData);
static ErrStatus aes_decrypt_verify_engine(const cryptData_t* cryptedData, uint32_t size, CYPHER_DATA_STA is_last,  plainData_t* plainData);

static ErrStatus aes_gmac_engine(const ghashData_t* ghashData, uint32_t size, CYPHER_DATA_STA is_last);
static ErrStatus aes_process_latch(aesProcessParameters* latch_buf);
static ErrStatus aes_process_recover(const aesProcessParameters* recover_buf);
static ErrStatus aes_drv_gf128_mult(const uint8_t *pDataX, const uint8_t *pDataY,  uint8_t *pDataZ);

static void xor_calc(uint8_t *A, uint32_t t);
static ErrStatus aes_key_wrap(const plainData_t *plainData, uint32_t in_len, cryptData_t *cryptedData, uint32_t *out_len);
static ErrStatus aes_key_wrap_with_padding(const plainData_t *plainData, uint32_t in_len, cryptData_t *cryptedData, uint32_t *out_len);
static ErrStatus aes_key_unwrap(const cryptData_t *cryptedData, uint32_t in_len, plainData_t *plainData, uint32_t *out_len, uint8_t *icvData);
static ErrStatus aes_key_unwrap_with_padding(const cryptData_t *cryptedData, uint32_t in_len, plainData_t *plainData, uint32_t *out_len);

static ErrStatus aes_aad_engine(const add_t *addData, uint32_t size);
#ifdef  RN821x_RN721x_SOC_V3
static void sea_aes_sram_ptr_init(uint8_t crypt_sel);
#endif

static aesCypherConfigStr          aes_cryptConfig   ;
static aesEngineParameters         aesInterParameters;

#ifdef  RN821x_RN721x_SOC_V3

static SEA_AES_DMA_RAM_Typedef    cypher_aes_sram_paras;

static SEA_AES_CYPHER_IN_TypeDef       * SEA_AES_CYPHER_IN    ;
static SEA_AES_CYPHER_OUT_TypeDef      * SEA_AES_CYPHER_OUT   ;
static SEA_AES_EXT_KEY_TypeDef         * SEA_AES_EXP_KEY      ;

static void sea_aes_sram_ptr_init(uint8_t crypt_sel)
{
    uint32_t  tmp_addr_cry, tmp_addr_decry;
    tmp_addr_cry    = (uint32_t)&cypher_aes_sram_paras.AES_DMA_IN  ;
    tmp_addr_decry  = (uint32_t)&cypher_aes_sram_paras.AES_DMA_OUT ;
    if(SEA_CTRL_IR_AES_Enc == crypt_sel){
        SEA_AES_CYPHER_IN  = (SEA_AES_CYPHER_IN_TypeDef  *)tmp_addr_cry  ;
        SEA_AES_CYPHER_OUT = (SEA_AES_CYPHER_OUT_TypeDef *)tmp_addr_decry;
    }else{
        SEA_AES_CYPHER_IN  = (SEA_AES_CYPHER_IN_TypeDef  *)tmp_addr_decry  ;
        SEA_AES_CYPHER_OUT = (SEA_AES_CYPHER_OUT_TypeDef *)tmp_addr_cry;
    }
    SEA_AES_EXP_KEY    = &cypher_aes_sram_paras.AES_DMA_KEY ;
    SEA_CYPHER->AES_DMA_ADDR = (uint32_t)&cypher_aes_sram_paras;
}
#endif

/*
 * Define module-level global constants
 */
static const uint8_t AES_KW_ICV1[] = {0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6}; /* default IV in RFC-3394 */
static const uint8_t AES_KW_ICV2[] = {0xA6, 0x59, 0x59, 0xA6}; /* AIV per RFC-5649 */

#define KW_SEMIBLOCK_LEN sizeof(AES_KW_ICV1)

typedef  ErrStatus (*aes_drv_cryptKernel)(const regData_t ivData[], const regData_t  plainData[], regData_t  cryptedData[]);

static const aes_drv_cryptKernel aes_drv_cryptKernel_list[5]={
     (aes_drv_cryptKernel )aes_drv_cryptKernel_ECB ,
     (aes_drv_cryptKernel )aes_drv_cryptKernel_CBC ,
     (aes_drv_cryptKernel )aes_drv_cryptKernel_CTR ,
     (aes_drv_cryptKernel )aes_drv_cryptKernel_CFB128 ,
     (aes_drv_cryptKernel )aes_drv_cryptKernel_OFB ,
    };

static ErrStatus aes_drv_KernelCalc(void)
{
    uint32_t cnt;
    ErrStatus tmp_err = SUCCESS;
    SEA_CYPHER->CTRL = ((uint32_t)aes_cryptConfig.bitWideMode   << SEA_CTRL_AES_LEN_Pos)
                    |  ((uint32_t)aesInterParameters.cryptMode  << SEA_CTRL_AES_MOD_Pos)
                    |  ((uint32_t)aesInterParameters.crypt_ir   << SEA_CTRL_IR_Pos);
    SEA_CYPHER->Status = SEA_STA_AES_Msk;
    SEA_CYPHER->START = 1U;
    cnt = 0U;
    while(0U == (SEA_CYPHER->Status & SEA_STA_AES_Msk)){
        if(cnt++ > 400U){
            tmp_err = ERROR;
            break;
        }
    }
    return tmp_err;
}

/*#define SEA_AES_CRYPT_REG_PROC(cnt)     aes_drv_KernelCalc()*/

/* this function generates the expanded AES128/192/256 Key */
static ErrStatus aes_drv_expand_key(const regData_t p_key[], uint32_t size, uint32_t aes_len_mod)
{
    uint32_t i;
    ErrStatus tmp_err = SUCCESS;

    for(i = 0U; i < 4U; i++){
        SEA_AES_CYPHER_IN->DATA_IN[i] =0U ;
        SEA_AES_IV->IV[i]  = 0U;
    }

    for(i = 0U; i < (size >> 2U) ; i++){
        SEA_AES_EXP_KEY->KEY_EXP[i] = (uint32_t)(__REV(p_key[i]));
    }

    SEA_CYPHER->CTRL = ((uint32_t)aes_len_mod << SEA_CTRL_AES_LEN_Pos)  | ((uint32_t)SEA_CTRL_IR_AES_KeyExp << SEA_CTRL_IR_Pos);
    SEA_CYPHER->Status = SEA_STA_MUX_Msk;
    SEA_CYPHER->START = 1U;

    i = 0U;
    while(0U == ((SEA_CYPHER->Status & SEA_STA_MUX_Msk))){
        if(i++ > 600U) {
            tmp_err =  ERROR; /*max 162 clk*/
            break;
        }
    }
    return tmp_err;
}

/* this function generates the gloabal H parameter 128bit */
static ErrStatus aes_drv_ghash_h_generate(uint32_t aes_len_mod, regData_t H[4])
{
    uint32_t   i;
    ErrStatus tmp_err = SUCCESS;

    for(i = 0U; i < 4U; i++){
        SEA_AES_CYPHER_IN->DATA_IN[i] =0U ;
        SEA_AES_IV->IV[i]  = 0U;
    }

   SEA_CYPHER->CTRL = ((uint32_t)aes_len_mod         << SEA_CTRL_AES_LEN_Pos)
                    | ((uint32_t)SEA_AES_CTR         << SEA_CTRL_AES_MOD_Pos)
                    | ((uint32_t)SEA_CTRL_IR_AES_Enc << SEA_CTRL_IR_Pos);

    SEA_CYPHER->Status = SEA_STA_MUX_Msk;
    SEA_CYPHER->START = 1U;
    i = 0U;
    while(0U == (SEA_CYPHER->Status & SEA_STA_MUX_Msk)){
        if(i++ > 400U)/*max 70 clk*/
        {
            tmp_err =  ERROR;
            break;
        }
    }
    /*save global H*/
    for(i = 0U; i < 4U; i++) {
        H[i] = SEA_AES_CYPHER_OUT->DATA_OUT[3U-i];
    }

   return tmp_err;
}

static ErrStatus aes_drv_ghash_unit(const regData_t pDataIn[], uint8_t *is_start, SEA_ENDIAN_MOD little_endian)
{
    uint32_t i, tmp;
    ErrStatus tmp_err = SUCCESS;

    if(0U != *is_start) {
        *is_start = 0U;
        if(SEA_MSG_LITTLE_ENDIAN == little_endian){
            for(i=0U; i< 4U; i++){
                SEA_CYPHER->GHASH_Y[i] = g_cypher_paras.aes_buf.h_default[i];/*Y = H*/
                SEA_CYPHER->GHASH_X[i] = g_cypher_paras.aes_buf.h_predata[i]^((uint32_t)(__REV(pDataIn[3U-i]))); /*X = DIN*/
            }
        }
        else{
            for(i=0U; i< 4U; i++){
                SEA_CYPHER->GHASH_Y[i] = g_cypher_paras.aes_buf.h_default[i];/*Y = H*/
                SEA_CYPHER->GHASH_X[i] = g_cypher_paras.aes_buf.h_predata[i]^pDataIn[3U-i]; /*X = DIN*/
            }
        }
    }
    else{
        if(SEA_MSG_LITTLE_ENDIAN == little_endian){
            for(i=0U; i< 4U; i++){
                tmp = ((uint32_t)(__REV(pDataIn[3U-i])));/*X = X_d^DIN*/
                SEA_CYPHER->GHASH_X[i] = SEA_CYPHER->GHASH_Z[i]^tmp;
            }
        }
        else{
            for(i=0U; i< 4U; i++){
                SEA_CYPHER->GHASH_X[i] = SEA_CYPHER->GHASH_Z[i]^pDataIn[3U-i];/*X = X_d^DIN*/
            }
        }
    }

    SEA_CYPHER->CTRL =  ((uint32_t)SEA_CTRL_IR_AES_GF128 << SEA_CTRL_IR_Pos);
    SEA_CYPHER->Status = SEA_STA_MUX_Msk;
    SEA_CYPHER->START = 1U;

    i = 0U;
    while(0U == (SEA_CYPHER->Status & SEA_STA_MUX_Msk)){
        if(i++ > 400U){ /*MAX 128CLK*/
            tmp_err =  ERROR;
            break;
        }
    }

    return tmp_err;
}

/*!
    \fn ErrStatus aes_drv_gf128_mult(const uint8_t *pDataX, const uint8_t *pDataY,  uint8_t *pDataZ)
    \brief               128bit有限域乘法
    \param[in]  pDataX   乘数X
    \param[in]  pDataY   被乘数Y
    \param[out] pDataZ   结果输出
    \retval     SUCCESS/ERROR
*/
static ErrStatus aes_drv_gf128_mult(const uint8_t *pDataX, const uint8_t *pDataY,  uint8_t *pDataZ)
{
    uint32_t i;
    ErrStatus tmp_err = SUCCESS;
    uint32_t tmp_data_x[4], tmp_data_y[4];

    (void)memcpy(tmp_data_x, (const uint8_t *)pDataX, 16U);
    (void)memcpy(tmp_data_y, (const uint8_t *)pDataY, 16U);

    for(i=0U; i< 4U; i++){
        SEA_CYPHER->GHASH_X[i] = (uint32_t)(__REV(tmp_data_x[3U-i]));
        SEA_CYPHER->GHASH_Y[i] = (uint32_t)(__REV(tmp_data_y[3U-i]));
    }

    SEA_CYPHER->CTRL =  ((uint32_t)SEA_CTRL_IR_AES_GF128 << SEA_CTRL_IR_Pos);
    SEA_CYPHER->Status = SEA_STA_MUX_Msk;
    SEA_CYPHER->START = 1U;

    i = 0U;
    while(0U == (SEA_CYPHER->Status & SEA_STA_MUX_Msk)){
        if(i++ > 400U){ /*MAX 128CLK*/
            tmp_err =  ERROR;
            break;
        }
    }

    for(i=0U; i< 4U; i++){
        tmp_data_x[i] = (uint32_t)(__REV(SEA_CYPHER->GHASH_Z[3U-i]));
    }

    (void)memcpy(pDataZ, tmp_data_x, 16U);

    return tmp_err;
}

static ErrStatus aes_drv_crypt_noRev(const regData_t  ivData[], const regData_t plainData[], regData_t cryptedData[])
{
    uint32_t i ;
    ErrStatus tmp_err = SUCCESS;

    for(i=0U; i< 4U; i++){
        SEA_AES_CYPHER_IN->DATA_IN[i] =  plainData[i] ;
        SEA_AES_IV->IV[i] = ivData[i];
    }
    if(SUCCESS != aes_drv_KernelCalc()){
        tmp_err =  ERROR;
    }
    for(i=0U; i< 4U; i++){
        cryptedData[i] = SEA_AES_CYPHER_OUT->DATA_OUT[i];
    }

    return tmp_err;
}

static ErrStatus aes_drv_cryptKernel_ECB(const regData_t ivData[], const regData_t plainData[], regData_t cryptedData[])
{
    uint32_t i ;
    ErrStatus tmp_err = SUCCESS;

    SEA_AES_CYPHER_IN->DATA_IN[0]  = ivData[0];

    for(i=0U; i< 4U; i++){
        SEA_AES_CYPHER_IN->DATA_IN[i] = (uint32_t)(__REV(plainData[i]));
    }
    if(SUCCESS != aes_drv_KernelCalc()){
        tmp_err = ERROR;
    }
    for(i=0U; i< 4U; i++){
        cryptedData[i] = SEA_AES_CYPHER_OUT->DATA_OUT[i];
    }

    return tmp_err;
}

static ErrStatus aes_drv_cryptKernel_CBC(const regData_t  ivData[], const regData_t  plainData[], regData_t  cryptedData[])
{
    uint32_t i ;
    ErrStatus tmp_err = SUCCESS;

    SEA_AES_IV->IV[0]  = ivData[0];

    for(i=0U; i< 4U; i++){
        SEA_AES_CYPHER_IN->DATA_IN[i] = (uint32_t)(__REV(plainData[i]));
        SEA_AES_IV->IV[i]             = g_cypher_paras.aes_buf.cypher_predata[i];
    }
    if(SUCCESS != aes_drv_KernelCalc()){
        tmp_err = ERROR;
    }
    for(i=0U; i< 4U; i++){
        cryptedData[i]  = SEA_AES_CYPHER_OUT->DATA_OUT[i];
    }

    if(aes_cryptConfig.encrypt_sel != AES_enCrypt){
        for(i=0U; i< 4U; i++){
            g_cypher_paras.aes_buf.cypher_predata[i] = SEA_AES_CYPHER_IN->DATA_IN[i];
        }
    }else{
        for(i=0U; i< 4U; i++){
          g_cypher_paras.aes_buf.cypher_predata[i] = cryptedData[i];
        }
    }


    return tmp_err;
}

static ErrStatus aes_drv_cryptKernel_CTR(const regData_t ivData[], const regData_t  plainData[], regData_t  cryptedData[])
{
    uint32_t i ;
    ErrStatus tmp_err = SUCCESS;

    for(i=0U; i< 4U; i++){
        SEA_AES_IV->IV[i] =(uint32_t)(__REV(plainData[i]));
    }
    SEA_AES_CYPHER_IN->DATA_IN[3] =  (ivData[3]);

    if(SUCCESS != aes_drv_KernelCalc()){
        tmp_err = ERROR;
    }
    for(i=0U; i< 4U; i++){
        cryptedData[i] = SEA_AES_CYPHER_OUT->DATA_OUT[i];
    }


    return tmp_err;
}

static ErrStatus aes_drv_cryptKernel_CFB128(const regData_t ivData[], const regData_t plainData[], regData_t cryptedData[])
{
    uint32_t i ;
    ErrStatus tmp_err = SUCCESS;

    SEA_AES_IV->IV[0]  = ivData[0];

    for(i=0U; i< 4U; i++){
        SEA_AES_IV->IV[i]                =  (uint32_t)(__REV(plainData[i]));
        SEA_AES_CYPHER_IN->DATA_IN[i]    =  g_cypher_paras.aes_buf.cypher_predata[i];
    }
    if(SUCCESS != aes_drv_KernelCalc()){
        tmp_err = ERROR;
    }
    for(i=0U; i< 4U; i++){
        cryptedData[i] = SEA_AES_CYPHER_OUT->DATA_OUT[i];
    }

    if(aes_cryptConfig.encrypt_sel != AES_enCrypt){
        for(i=0U; i< 4U; i++){
          g_cypher_paras.aes_buf.cypher_predata[i] = SEA_AES_IV->IV[i];
        }
    }else{
        for(i=0U; i< 4U; i++){
          g_cypher_paras.aes_buf.cypher_predata[i] = cryptedData[i];
        }
    }

    return tmp_err;
}

static ErrStatus aes_drv_cryptKernel_OFB(const regData_t ivData[], const regData_t plainData[], regData_t cryptedData[])
{
    uint32_t i ;
    ErrStatus tmp_err = SUCCESS;

    SEA_AES_IV->IV[0]  = ivData[0];

    for(i=0U; i< 4U; i++){
        SEA_AES_IV->IV[i]                =  (uint32_t)(__REV(plainData[i]));
    }


    if(SUCCESS != aes_drv_KernelCalc()){
        tmp_err =  ERROR;
    }
    for(i=0U; i< 4U; i++){
        cryptedData[i]                      = SEA_AES_CYPHER_OUT->DATA_OUT[i];
    }

    return tmp_err;
}

static ErrStatus aes_drv_ghash_engine(const ghashData_t data_in[], uint32_t size, SEA_ENDIAN_MOD little_endian)
{
    uint32_t  i, loops, data_last_size, tmp_u32;
    ErrStatus tmp_err = SUCCESS;
    regData_t  tmp_buf[4];
    loops  = size >> AES_BLOCK_BSHIFT;
    data_last_size = size & (AES_PLAIN_BSIZE - 1U);
    for(i=0U; i < loops; i++){
        #ifdef AES_CRYPT_ITEMS_IN_BYTES
        tmp_u32 = (uint32_t)&data_in[AES_REGS_DIV_DATA_ITEM*i];
        SEA_MEMCPY(tmp_buf, (const uint8_t *)tmp_u32, AES_REGS_DIV_DATA_ITEM);
        if(SUCCESS != aes_drv_ghash_unit((regData_t *)&tmp_buf[0], &aesInterParameters.is_start_ghash, little_endian)){
            tmp_err = ERROR;
            break;
        }
        #else
        tmp_u32 = (uint32_t)&data_in[4U*i];
        if(SUCCESS != aes_drv_ghash_unit((regData_t *)tmp_u32, &aesInterParameters.is_start_ghash, little_endian)){
            tmp_err = ERROR;
        }
        #endif
    }
    if(0U != data_last_size){

        /*clr buffer*/
        sea_memset_word(tmp_buf, 0U, 4U);
        #ifdef AES_CRYPT_ITEMS_IN_BYTES
        SEA_MEMCPY(tmp_buf, &data_in[AES_REGS_DIV_DATA_ITEM*loops], data_last_size);
        #else
        if(little_endian == SEA_MSG_LITTLE_ENDIAN){
            SEA_MEMCPY(tmp_buf, &data_in[AES_REGS_DIV_DATA_ITEM*loops], data_last_size);
        }
        else{
            #ifdef AES_CRYPT_ITEMS_IN_BYTES
            i = data_last_size & 0xfffffffcU ;
            SEA_MEMCPY(tmp_buf, &data_in[AES_REGS_DIV_DATA_ITEM*loops], i);
            if((data_last_size & 3u) == 1U){
                tmp_buf[data_last_size>>2U] = (uint32_t)data_in[AES_REGS_DIV_DATA_ITEM*loops + i + 3U] << 24U;
            }
            else if((data_last_size & 3u) == 2U){
                tmp_buf[data_last_size>>2] = (uint32_t)data_in[AES_REGS_DIV_DATA_ITEM*loops + i + 3U] << 24U |  (uint32_t)data_in[AES_REGS_DIV_DATA_ITEM*loops + i + 2U] << 16U ;
            }
            else if((data_last_size & 3u) == 3U){
                tmp_buf[data_last_size>>2U] = (uint32_t)data_in[AES_REGS_DIV_DATA_ITEM*loops + i + 3U] << 24U |  (uint32_t)data_in[AES_REGS_DIV_DATA_ITEM*loops + i + 2U] << 16U |  (uint32_t)data_in[AES_REGS_DIV_DATA_ITEM*loops + i + 1U] << 8U;
            }else{
              ;
            }
            #else
            for(i=0U;i<(data_last_size >>2U );i++){
                tmp_buf[i] = (uint32_t)data_in[4U*loops + i];
            }
            if((data_last_size & 3U) == 1U){
                tmp_buf[i] = (uint32_t)data_in[4U*loops + i] & 0xff000000U;
            }
            else if((data_last_size & 3U) == 2U){
                tmp_buf[i] = (uint32_t)data_in[4U*loops + i] & 0xffff0000U;
            }
            else if((data_last_size & 3U) == 3U){
                tmp_buf[i] = (uint32_t)data_in[4U*loops + i] & 0xffffff00U;
            }else{
                ;
            }
            #endif
        }
        #endif
        if(SUCCESS !=  aes_drv_ghash_unit(tmp_buf, &aesInterParameters.is_start_ghash, little_endian)){
            tmp_err = ERROR;
        }
    }
    return tmp_err;
}

static ErrStatus aes_crypt_engine(const plainData_t  plainData[],  uint32_t size, cryptData_t  cryptedData[])
{
    uint32_t  i, j, loops, data_last_size;
    ErrStatus tmp_err = SUCCESS;
    regData_t  tmp_plainBuf[4];
    regData_t  tmp_cryptBuf[4];
    #ifndef AES_CRYPT_ITEMS_IN_BYTES
    uint32_t tmp_addr_plain =   (uint32_t)plainData;
    uint32_t tmp_addr_crypt =   (uint32_t)cryptedData;
    #else
    uint32_t tmp_u32;
    #endif
    const aes_drv_cryptKernel aes_drv_cryptKernel_cur = (aes_drv_cryptKernel )aes_drv_cryptKernel_list[aesInterParameters.cryptMode];

    loops  = size >> AES_BLOCK_BSHIFT;
    data_last_size = size & (AES_PLAIN_BSIZE - 1U);
    for(i=0U; i < loops; i++){
        #ifdef AES_CRYPT_ITEMS_IN_BYTES
        tmp_u32 = (uint32_t)&plainData[AES_REGS_DIV_DATA_ITEM*i];
        SEA_MEMCPY(tmp_plainBuf, (const uint8_t *)tmp_u32, 16U);
        if(SUCCESS != aes_drv_cryptKernel_cur(g_cypher_paras.aes_buf.iv, tmp_plainBuf,  tmp_cryptBuf)){
            tmp_err = ERROR;
            break;
        }
        /*when agign by bytes, then output is little endian.so crypt datas should be endian switch*/
        for(j=0U; j < 4U; j++){
            tmp_cryptBuf[j] = (uint32_t)(__REV(tmp_cryptBuf[j]));
        }
        tmp_u32 = (uint32_t)&cryptedData[AES_REGS_DIV_DATA_ITEM*i];
        SEA_MEMCPY((uint8_t *)tmp_u32, &tmp_cryptBuf[0], 16U);
        #else
        if(SUCCESS != aes_drv_cryptKernel_cur(g_cypher_paras.aes_buf.iv, (regData_t *)tmp_addr_plain, (regData_t *)tmp_addr_crypt)){
            tmp_err = ERROR;
        }
        tmp_addr_plain +=   16U;
        tmp_addr_crypt +=   16U;
        #endif
        g_cypher_paras.aes_buf.iv[3]++ ;

        if(0U == g_cypher_paras.aes_buf.iv[3]){
            g_cypher_paras.aes_buf.iv[2]++ ;
            SEA_AES_CYPHER_IN->DATA_IN[2] = g_cypher_paras.aes_buf.iv[2];
            if(0U == g_cypher_paras.aes_buf.iv[2]){
                g_cypher_paras.aes_buf.iv[1]++ ;
                SEA_AES_CYPHER_IN->DATA_IN[1] = g_cypher_paras.aes_buf.iv[1];
                if(0U == g_cypher_paras.aes_buf.iv[1]){
                    g_cypher_paras.aes_buf.iv[0]++ ;
                    SEA_AES_CYPHER_IN->DATA_IN[0] = g_cypher_paras.aes_buf.iv[0];
                }
            }
        }
    }

    if(0U != data_last_size){

        /*copy datas*/
        sea_memset_word(tmp_plainBuf, 0U, 4U);

        SEA_MEMCPY(tmp_plainBuf, &plainData[AES_REGS_DIV_DATA_ITEM*loops], data_last_size);

        if(SUCCESS != aes_drv_cryptKernel_cur(g_cypher_paras.aes_buf.iv, tmp_plainBuf,  tmp_cryptBuf)){
            tmp_err = ERROR;
        }

        #ifdef AES_CRYPT_ITEMS_IN_BYTES
        /*when agign by bytes, then output is little endian.so crypt datas should be endian switch*/
        /*cause, crypt buffer size may be not align by 32bit.                                     */
        for(j=0U; j < 4U; j++){
            tmp_cryptBuf[j] = (uint32_t)(__REV(tmp_cryptBuf[j]));
        }
        SEA_MEMCPY(&cryptedData[AES_REGS_DIV_DATA_ITEM*loops], &tmp_cryptBuf[0], data_last_size);
        #else
        i= data_last_size >> 2;
        if(0U != (data_last_size & 0x3U)){
            i++;
        }
        for(j=0U; j < i; j++){
            cryptedData[AES_REGS_DIV_DATA_ITEM*loops + j] = tmp_cryptBuf[j];
        }
        #endif
        g_cypher_paras.aes_buf.iv[3]++;
    }



    return tmp_err;
}

static void aes_ghash_checkin(const regData_t pre_data[4])
{
  uint32_t  i;
  for(i=0U; i< 4U; i++){
      g_cypher_paras.aes_buf.h_predata[i] =  pre_data[i];
  }
}

static ErrStatus aes_crypt_gcm_iv_y0_generate(regData_t IV[4], regData_t Y[4])
{
    uint32_t  i;
    uint32_t  tmp_data[4]={0,0,0,0};
    ErrStatus tmp_err = SUCCESS;

    aes_ghash_checkin(tmp_data);

    if(aes_cryptConfig.ivSize == (96U >> 3U)){
        SEA_MEMCPY((uint8_t *)IV, (uint8_t *)aes_cryptConfig.ivLocation, (uint32_t)AES_BLOCK_BSIZE - (uint32_t)4U);
        endian_rbytes(IV, IV, 3U);
        IV[3] = 1U;
    }
    else{
        if(SUCCESS != aes_drv_ghash_engine((ghashData_t *)(uint32_t)aes_cryptConfig.ivLocation, aes_cryptConfig.ivSize, SEA_MSG_LITTLE_ENDIAN)){
            tmp_err = ERROR;
        }

        tmp_data[3] = aes_cryptConfig.ivSize << 3U; /*bitwide*/
        if(SUCCESS != aes_drv_ghash_unit(tmp_data, &aesInterParameters.is_start_ghash, SEA_MSG_BIG_ENDIAN)){
                tmp_err = ERROR;
        }
        for(i=0U; i< 4U; i++){
            IV[i] = SEA_CYPHER->GHASH_Z[3U-i];
        }
        tmp_data[3] = 0U;
    }
    /*y0*/
    if(SUCCESS != aes_drv_crypt_noRev(tmp_data, IV, Y)){
            tmp_err = ERROR;
    }

    IV[3]++;

  return tmp_err;
}

static ErrStatus aes_crypt_gcm_yfc_out_generate(uint32_t add_size, uint32_t msg_size, tag_t *pTag, uint32_t size)
{
    uint32_t  i,j;
    uint32_t  tmp_u32_buf[4];
    ErrStatus tmp_err = SUCCESS;

    tmp_u32_buf[0] = 0U;
    tmp_u32_buf[2] = 0U;
    tmp_u32_buf[1] = add_size << 3;
    tmp_u32_buf[3] = msg_size << 3;

    if(SUCCESS != aes_drv_ghash_unit(tmp_u32_buf, &aesInterParameters.is_start_ghash, SEA_MSG_BIG_ENDIAN)){
         tmp_err = ERROR;
    }
     /*when size is not align by 32bit word, pTag size must align by words*/
    j = ((size & 3U) != 0U)? ((size >> 2) + 1U) : (size >> 2U);
    if(j > 4U){
      j = 4U;
    }
    for(i=0U; i< j; i++){
        tmp_u32_buf[i] = (uint32_t)(__REV(SEA_CYPHER->GHASH_Z[3U-i]^g_cypher_paras.aes_buf.h_y0[i]));
    }
    SEA_MEMCPY(pTag, tmp_u32_buf, size);

    return tmp_err;
}

static void aes_crypt_start_manager(void)
{
    uint32_t i;

    if(aes_cryptConfig.cryptMode  == AES_GCM)
    {
        for(i=0U; i< 4U; i++){
             g_cypher_paras.aes_buf.h_predata[i]     =  g_cypher_paras.aes_buf.h_add[i] ;
             g_cypher_paras.aes_buf.iv[i]            =  g_cypher_paras.aes_buf.h_iv[i];
        }
        aesInterParameters.is_start_ghash = 1U; /*ghash init start*/
        aesInterParameters.cryptSize      = 0U;
    }
    else if(aes_cryptConfig.cryptMode  != AES_ECB){
        for(i=0U; i< 4U; i++){
            g_cypher_paras.aes_buf.iv[i]             = g_cypher_paras.aes_buf.h_iv[i];
            g_cypher_paras.aes_buf.cypher_predata[i] = g_cypher_paras.aes_buf.iv[i]  ;
            SEA_AES_CYPHER_IN->DATA_IN[i] = g_cypher_paras.aes_buf.iv[i];
            SEA_AES_IV->IV[i]             = g_cypher_paras.aes_buf.iv[i];
        }
    }
    else{
       /*not care*/
    }
}

static ErrStatus aes_parameters_init(const aesCypherConfigStr*  configuration)
{
    const uint8_t data_list_aes_key_size[3] = {AES128_KEYSIZE, AES192_KEYSIZE, AES256_KEYSIZE};
    const uint8_t data_list_aes_encrypt_ir[5] = {SEA_CTRL_IR_AES_Enc, SEA_CTRL_IR_AES_Enc, SEA_CTRL_IR_AES_Enc, SEA_CTRL_IR_AES_Enc, SEA_CTRL_IR_AES_Enc};
    const uint8_t data_list_aes_decrypt_ir[5] = {SEA_CTRL_IR_AES_Dec, SEA_CTRL_IR_AES_Dec, SEA_CTRL_IR_AES_Enc, SEA_CTRL_IR_AES_Enc, SEA_CTRL_IR_AES_Enc};
    const SEA_AES_CRYPT_MODE data_list_aes_crypt_mode[6] = {SEA_AES_ECB, SEA_AES_CBC, SEA_AES_CTR, SEA_AES_CFB128, SEA_AES_OFB, SEA_AES_CTR};

    uint32_t i;
    ErrStatus tmp_err = SUCCESS;

    aesCypherConfigStr*   const pEngine_cfg   = (aesCypherConfigStr*)&aes_cryptConfig;
    aesEngineParameters*  const pEngine_paras = (aesEngineParameters*)&aesInterParameters;

    if(configuration->bitWideMode > AES_256){
        tmp_err = ERROR;
    }

    pEngine_cfg->encrypt_sel  = configuration->encrypt_sel;
    pEngine_cfg->bitWideMode  = configuration->bitWideMode;
    pEngine_cfg->cryptMode    = configuration->cryptMode;

    SEA_MEMCPY(g_cypher_paras.aes_buf.key, configuration->keyLocation, data_list_aes_key_size[pEngine_cfg->bitWideMode]);
    pEngine_cfg->keyLocation  = (aesKey_t *)(uint32_t)g_cypher_paras.aes_buf.key;
    pEngine_cfg->keySize      = data_list_aes_key_size[pEngine_cfg->bitWideMode];

    pEngine_cfg->ivLocation   = configuration->ivLocation;

    pEngine_cfg->specialMode   = configuration->specialMode;
    pEngine_cfg->keyWarpMode   = configuration->keyWarpMode;

    /*keySize ivSize*/
    if(pEngine_cfg->cryptMode      != AES_GCM){
        pEngine_cfg->ivSize         = AES_PLAIN_BSIZE;
        if(pEngine_cfg->encrypt_sel == AES_enCrypt){
            pEngine_paras->crypt_ir = data_list_aes_encrypt_ir[pEngine_cfg->cryptMode];
        }else{
            pEngine_paras->crypt_ir = data_list_aes_decrypt_ir[pEngine_cfg->cryptMode];
        }
    }
    else{/*GCM*/

        pEngine_cfg->ivSize       = configuration->ivSize;
        pEngine_cfg->addLocation  = configuration->addLocation;
        pEngine_cfg->addSize      = configuration->addSize;
        pEngine_cfg->tagSignBuf   = configuration->tagSignBuf;
        pEngine_cfg->tagVerifyBuf = configuration->tagVerifyBuf;
        pEngine_cfg->tagSize      = configuration->tagSize;
        pEngine_cfg->tagVerifySta = configuration->tagVerifySta;

        pEngine_paras->cryptSize  = 0U;
        pEngine_paras->aadSize    = 0U;
        pEngine_paras->crypt_ir   = SEA_CTRL_IR_AES_Enc;
        pEngine_paras->crypt_kernelMode = AES_IV_PLAIN3;
    }
    pEngine_paras->cryptMode    = data_list_aes_crypt_mode[configuration->cryptMode];

#ifdef  RN821x_RN721x_SOC_V3
    sea_aes_sram_ptr_init(pEngine_paras->crypt_ir);
#endif

    /*key expand*/
    if(SUCCESS != aes_drv_expand_key((regData_t *)(uint32_t)pEngine_cfg->keyLocation, pEngine_cfg->keySize, (uint32_t)pEngine_cfg->bitWideMode)){
        tmp_err = ERROR;
    }

    if(pEngine_cfg->cryptMode   == AES_GCM){
        /*CALC H*/
        if(SUCCESS != aes_drv_ghash_h_generate((uint32_t)pEngine_cfg->bitWideMode, g_cypher_paras.aes_buf.h_default)){
            tmp_err = ERROR;
        }
        /*generate 128bit iv | counter and Y0*/
        pEngine_paras->is_start_ghash = 1U; /*ghash init start*/
        if(SUCCESS != aes_crypt_gcm_iv_y0_generate(g_cypher_paras.aes_buf.h_iv, g_cypher_paras.aes_buf.h_y0)){
            tmp_err = ERROR;
        }

        /*ghash add message, generate F*/
        if(0U != pEngine_cfg->addSize){
            pEngine_paras->is_start_ghash = 1U; /*ghash init start*/
            if(SUCCESS != aes_drv_ghash_engine((ghashData_t *)pEngine_cfg->addLocation, (uint32_t)pEngine_cfg->addSize, SEA_MSG_LITTLE_ENDIAN)){
                tmp_err = ERROR;
            }
            for(i=0U; i< 4U; i++){
                g_cypher_paras.aes_buf.h_add[i] =  SEA_CYPHER->GHASH_Z[i];
            }
        }
        else{
            for(i=0U; i< 4U; i++){
                g_cypher_paras.aes_buf.h_add[i] =  0U;
            }
        }
    }
    else if(pEngine_cfg->cryptMode  != AES_ECB){
        SEA_MEMCPY(g_cypher_paras.aes_buf.h_iv, pEngine_cfg->ivLocation, AES_BLOCK_BSIZE);
        endian_rbytes(g_cypher_paras.aes_buf.h_iv, g_cypher_paras.aes_buf.h_iv, 4U);
    }
    else{
        /*not care*/
    }

    pEngine_paras->is_start_crypt       = 1U; /*init start*/
    pEngine_paras->is_start_cryptKernel = 1U;
    pEngine_paras->is_start_ghash       = 1U;
    return tmp_err;
}

static ErrStatus aes_crypt_sign_engine(const plainData_t* plainData, uint32_t size, CYPHER_DATA_STA is_last, cryptData_t* cryptedData)
{
    ErrStatus tmp_err = SUCCESS;

    if(0U != aesInterParameters.is_start_crypt){
        aesInterParameters.is_start_crypt = 0U;
        aes_crypt_start_manager();
    }
    /*calc crypt data*/
    if(SUCCESS !=aes_crypt_engine(plainData, size, cryptedData)){
        tmp_err = ERROR;
    }

    if(aes_cryptConfig.cryptMode  == AES_GCM){

        aesInterParameters.cryptSize += size;
        #ifdef AES_CRYPT_ITEMS_IN_BYTES
        if(SUCCESS != aes_drv_ghash_engine((ghashData_t *)cryptedData, size, SEA_MSG_LITTLE_ENDIAN)){
            tmp_err = ERROR;
        }
        #else
        if(SUCCESS != aes_drv_ghash_engine((ghashData_t *)cryptedData, size, SEA_MSG_BIG_ENDIAN)){
            tmp_err = ERROR;
        }
        #endif

        /*calc lenf lenc*/
        if(CypherDatasIsTheLast == is_last){
            if(SUCCESS != aes_crypt_gcm_yfc_out_generate((aes_cryptConfig.addSize + aesInterParameters.aadSize), aesInterParameters.cryptSize, aes_cryptConfig.tagSignBuf, aes_cryptConfig.tagSize)){
                tmp_err = ERROR;
            }
        }
    }
    /*rev dataout*/
    #ifndef AES_CRYPT_ITEMS_IN_BYTES
    endian_rbytes_size(cryptedData, cryptedData, size);
    #endif
    /*reset sta*/
    if(CypherDatasIsTheLast == is_last){
        aesInterParameters.is_start_crypt = 1U;
    }

    return tmp_err;
}

static ErrStatus aes_decrypt_verify_engine(const cryptData_t* cryptedData, uint32_t size, CYPHER_DATA_STA is_last, plainData_t*  plainData)
{
    ErrStatus tmp_err = SUCCESS;

    if(0U != aesInterParameters.is_start_crypt){
        aesInterParameters.is_start_crypt = 0U;
        aes_crypt_start_manager();
    }
    if(aes_cryptConfig.cryptMode  == AES_GCM){
        aesInterParameters.cryptSize  += size;
        if(SUCCESS != aes_drv_ghash_engine(cryptedData, size, SEA_MSG_LITTLE_ENDIAN)){
            tmp_err = ERROR;
        }
        /*calc lenf lenc*/
        if(CypherDatasIsTheLast == is_last){
            if(SUCCESS != aes_crypt_gcm_yfc_out_generate((aes_cryptConfig.addSize + aesInterParameters.aadSize), aesInterParameters.cryptSize, aes_cryptConfig.tagVerifyBuf,  aes_cryptConfig.tagSize)){
                tmp_err = ERROR;
            }
            if(0 != memcmp(aes_cryptConfig.tagVerifyBuf, aes_cryptConfig.tagSignBuf, aes_cryptConfig.tagSize)){
                *aes_cryptConfig.tagVerifySta = AES_VERIFY_FAIL;
                /*return SUCCESS;*/
            }else{
                *aes_cryptConfig.tagVerifySta = AES_VERIFY_SUCCESS;
            }
        }
    }
    #if(AES_DECRYPT_GCM_SPECIAL_MOD_TYPE  != 0)
    if(aes_cryptConfig.specialMode != AES_SPC_GCM_VERIFY_ONLY)
    #endif
    {
        /*calc decrypt data*/
        if(SUCCESS != aes_crypt_engine(cryptedData, size, plainData)){
            tmp_err = ERROR;
        }

        #ifndef AES_CRYPT_ITEMS_IN_BYTES
        endian_rbytes_size(plainData, plainData, size);
        #endif
    }

    if(CypherDatasIsTheLast == is_last){
        aesInterParameters.is_start_crypt  = 1U;
    }
    return tmp_err;
}

/*GMAC, onlgy calc tag for crypt datas or add datas*/
static ErrStatus aes_gmac_engine(const ghashData_t* ghashData, uint32_t size, CYPHER_DATA_STA is_last)
{
    uint32_t i ;
    tag_t     *p_tag_buf;
    ErrStatus tmp_err = SUCCESS;

    if(aes_cryptConfig.cryptMode  != AES_GCM){
        tmp_err = ERROR;
    }

    if(0U != aesInterParameters.is_start_crypt){

        aesInterParameters.is_start_crypt = 0U;

        if(aes_cryptConfig.specialMode == AES_SPC_GMAC_CryptData){
            for(i=0U; i< 4U; i++){
                g_cypher_paras.aes_buf.h_predata[i]     =  g_cypher_paras.aes_buf.h_add[i] ;
            }
        }else{
            for(i=0U; i< 4U; i++){
                g_cypher_paras.aes_buf.h_predata[i]     =  0U ;
            }
        }

        aesInterParameters.is_start_ghash = 1U; /*ghash init start*/
        aesInterParameters.cryptSize  = 0U;
    }

    aesInterParameters.cryptSize  += size;

    if(SUCCESS != aes_drv_ghash_engine(ghashData, size, SEA_MSG_LITTLE_ENDIAN)){
        tmp_err = ERROR;
    }
    /*calc lenf lenc*/
    if(CypherDatasIsTheLast == is_last){
        if(aes_cryptConfig.encrypt_sel == AES_enCrypt){
            p_tag_buf = aes_cryptConfig.tagSignBuf;
        }else{
            p_tag_buf = aes_cryptConfig.tagVerifyBuf;
        }

        if(aes_cryptConfig.specialMode != AES_SPC_GMAC_CryptData){
            if(SUCCESS != aes_crypt_gcm_yfc_out_generate(aesInterParameters.cryptSize, 0U, p_tag_buf,  aes_cryptConfig.tagSize)){
                tmp_err = ERROR;
            }
        }else{
            if(SUCCESS != aes_crypt_gcm_yfc_out_generate(aes_cryptConfig.addSize, aesInterParameters.cryptSize, p_tag_buf,  aes_cryptConfig.tagSize)){
                tmp_err = ERROR;
            }
        }

        if(aes_cryptConfig.encrypt_sel == AES_deCrypt){
            if(0 != memcmp(p_tag_buf, aes_cryptConfig.tagSignBuf, aes_cryptConfig.tagSize)){
                *aes_cryptConfig.tagVerifySta = AES_VERIFY_FAIL;
            }else{
                *aes_cryptConfig.tagVerifySta = AES_VERIFY_SUCCESS;
            }
        }
        aesInterParameters.is_start_crypt  = 1U;
    }

    return tmp_err;
}

static ErrStatus aes_process_latch(aesProcessParameters* latch_buf)
{
    uint32_t i;
    /*configer paras*/
    latch_buf->config = aes_cryptConfig;

    /*engine paras*/
    latch_buf->engine = aesInterParameters;

    if(aes_cryptConfig.cryptMode  == AES_OFB){
        for(i=0U; i< 4U; i++){
             g_cypher_paras.aes_buf.h_predata[i]     =  SEA_AES_CYPHER_IN->DATA_IN[i];
             g_cypher_paras.aes_buf.iv[i]            =  SEA_AES_IV->IV[i];
        }
    }else if(aes_cryptConfig.cryptMode  == AES_GCM){
        for(i=0U; i< 4U; i++){
            g_cypher_paras.aes_buf.h_predata[i] =  SEA_CYPHER->GHASH_Z[i];
        }
    }else{
      /*not care*/
    }
    latch_buf->proc = g_cypher_paras.aes_buf;
    return SUCCESS;
}

static ErrStatus aes_process_recover(const aesProcessParameters* recover_buf)
{
    uint32_t i;
    ErrStatus tmp_err = SUCCESS;

    /*configer paras*/
    aes_cryptConfig = recover_buf->config;

    /*engine paras*/
    aesInterParameters = recover_buf->engine;

    /*gloabal paras*/
    g_cypher_paras.aes_buf = recover_buf->proc;

#ifdef  RN821x_RN721x_SOC_V3
    /*dma addr initial*/
    sea_aes_sram_ptr_init(aesInterParameters.crypt_ir);
#endif

    /*key expand*/
    if(SUCCESS != aes_drv_expand_key((regData_t *)(uint32_t)aes_cryptConfig.keyLocation, aes_cryptConfig.keySize, (uint32_t)aes_cryptConfig.bitWideMode)){
        tmp_err = ERROR;
    }
    if((aes_cryptConfig.cryptMode  == AES_GCM) ||(aes_cryptConfig.cryptMode  == AES_CTR) ){
        for(i=0U; i< 3U; i++){
            SEA_AES_CYPHER_IN->DATA_IN[i] =  g_cypher_paras.aes_buf.h_iv[i] ;
        }

        if(aes_cryptConfig.cryptMode  == AES_GCM) {
            aes_ghash_checkin(g_cypher_paras.aes_buf.h_predata);
            aesInterParameters.is_start_ghash = 1U;
        }
    }
    else if(aes_cryptConfig.cryptMode == AES_OFB){
        for(i=0U; i< 4U; i++){
            SEA_AES_CYPHER_IN->DATA_IN[i]  =  g_cypher_paras.aes_buf.h_predata[i];
        }
    }else{
      /*not care*/
    }

    return tmp_err;
}

static void xor_calc(uint8_t *A, uint32_t t) {
    uint32_t i, shift;

    for (i = 0; i < KW_SEMIBLOCK_LEN; i++) {
        shift = (uint32_t)((KW_SEMIBLOCK_LEN - 1U - i) * 8U);
        if (32U != shift) {
            A[i] ^= (uint8_t)((t >> shift) & 0xFFU);
        }
    }
}

static ErrStatus aes_key_wrap(const plainData_t *plainData, uint32_t in_len, cryptData_t *cryptedData, uint32_t *out_len) {
    ErrStatus tmp_err = SUCCESS;
    uint32_t semiblocks, t, s;
    uint8_t in_buff[KW_SEMIBLOCK_LEN * 2], out_buff[KW_SEMIBLOCK_LEN * 2];
    CYPHER_DATA_STA is_last;
    uint8_t *R2, *A;

    if (AES_KW_MODE_KW == aes_cryptConfig.keyWarpMode) {
        sea_memcpy(cryptedData, AES_KW_ICV1, KW_SEMIBLOCK_LEN);
        sea_memcpy((cryptedData + KW_SEMIBLOCK_LEN), plainData, in_len);
    }

    semiblocks = ((in_len >> 3U) + 1U);
    s = (6U * (semiblocks - 1U));
    *out_len = 0U;

    if (semiblocks < 3U) {
        tmp_err = ERROR;
    } else {
        R2 = (cryptedData + KW_SEMIBLOCK_LEN);
        A = cryptedData;

        for (t = 1U; t <= s; t++) {
            sea_memcpy(in_buff, A, KW_SEMIBLOCK_LEN);
            sea_memcpy((in_buff + KW_SEMIBLOCK_LEN), R2, KW_SEMIBLOCK_LEN);

            is_last = ((t == s) ? CypherDatasIsTheLast : CypherDatasNotTheLast);
            if (SUCCESS != aes_crypt_sign_engine((const plainData_t *)in_buff, 16U, is_last, (cryptData_t *)out_buff)) {
                tmp_err = ERROR;
            }

            sea_memcpy(A, out_buff, KW_SEMIBLOCK_LEN);
            xor_calc(A, t);

            sea_memcpy(R2, (out_buff + KW_SEMIBLOCK_LEN), KW_SEMIBLOCK_LEN);
            R2 += KW_SEMIBLOCK_LEN;
            if (R2 >= (cryptedData + (semiblocks * KW_SEMIBLOCK_LEN))) {
                R2 = (cryptedData + KW_SEMIBLOCK_LEN);
            }
        }
        *out_len = (semiblocks * KW_SEMIBLOCK_LEN);
    }

    return tmp_err;
}

static ErrStatus aes_key_wrap_with_padding(const plainData_t *plainData, uint32_t in_len, cryptData_t *cryptedData, uint32_t *out_len) {
    ErrStatus tmp_err = SUCCESS;
    uint32_t tmp_u32;
    uint32_t plain_buf[4];
    uint32_t padding_length, plaintext_padded_length;

    sea_memcpy((uint8_t *)cryptedData, AES_KW_ICV2, (KW_SEMIBLOCK_LEN >> 1U));
    tmp_u32 = ((uint32_t)cryptedData + (KW_SEMIBLOCK_LEN >> 1U));
    *((uint32_t *)tmp_u32) = (uint32_t)(__REV(in_len & 0xFFFFFFFFU));
    sea_memcpy((uint8_t *)(cryptedData + ((KW_SEMIBLOCK_LEN >> 1U) * 2U)), (const uint8_t *)plainData, in_len);

    if (0U != (in_len & 0x7U)) {
        padding_length = (((KW_SEMIBLOCK_LEN >> 1U) * 2U) - (in_len & 0x7U));
    } else {
        padding_length = 0U;
    }
    sea_memset((uint8_t *)(cryptedData + in_len + ((KW_SEMIBLOCK_LEN >> 1U) * 2U)), 0U, padding_length);
    plaintext_padded_length = (in_len + padding_length);

    tmp_u32 = (uint32_t)cryptedData;
    if (8U == plaintext_padded_length) {
        sea_memcpy((uint8_t *)plain_buf, (uint8_t *)cryptedData, 16U);
        if (SUCCESS != aes_crypt_sign_engine((const plainData_t *)tmp_u32, 16U, CypherDatasIsTheLast, (cryptData_t *)cryptedData)) {
            tmp_err = ERROR;
        }

        *out_len = 16U;
    } else {
        if (SUCCESS != aes_key_wrap((const plainData_t *)(tmp_u32 + KW_SEMIBLOCK_LEN), plaintext_padded_length, cryptedData, out_len)) {
            tmp_err = ERROR;
        }
    }

    return tmp_err;
}

static ErrStatus aes_key_unwrap(const cryptData_t *cryptedData, uint32_t in_len, plainData_t *plainData, uint32_t *out_len, uint8_t *icvData) {
    ErrStatus tmp_err = SUCCESS;
    uint8_t *R;
    uint8_t A[KW_SEMIBLOCK_LEN];
    uint32_t semiblocks, t, s;
    uint8_t in_buff[KW_SEMIBLOCK_LEN * 2], out_buff[KW_SEMIBLOCK_LEN * 2];
    CYPHER_DATA_STA is_last;

    sea_memset_word((uint32_t *)A, 0U, 2U);
    if ((AES_KW_MODE_KW == aes_cryptConfig.keyWarpMode) && (in_len < 24U)) {
        tmp_err = ERROR;
    }

    semiblocks = (in_len >> 3U);
    s = (6 * (semiblocks - 1));
    *out_len = 0U;

    if ((semiblocks < 3U) || (tmp_err == ERROR)) {
        tmp_err = ERROR;
    } else {
        sea_memcpy(A, cryptedData, KW_SEMIBLOCK_LEN);
        sea_memcpy(plainData, (cryptedData + KW_SEMIBLOCK_LEN), ((semiblocks - 1) * KW_SEMIBLOCK_LEN));
        R = (plainData + ((semiblocks - 2U) * KW_SEMIBLOCK_LEN));

        for (t = s; t >= 1; t--) {
            xor_calc(A, t);

            sea_memcpy(in_buff, A, KW_SEMIBLOCK_LEN);
            sea_memcpy((in_buff + KW_SEMIBLOCK_LEN), R, KW_SEMIBLOCK_LEN);

            is_last = ((t == 1) ? CypherDatasIsTheLast : CypherDatasNotTheLast);
            if (SUCCESS != aes_decrypt_verify_engine((const cryptData_t *)in_buff, 16U, is_last, (plainData_t *)out_buff)) {
                tmp_err = ERROR;
            }

            sea_memcpy(A, out_buff, KW_SEMIBLOCK_LEN);
            sea_memcpy(R, (out_buff + KW_SEMIBLOCK_LEN), KW_SEMIBLOCK_LEN);
            if (R == plainData) {
                R = (plainData + ((semiblocks - 2) * KW_SEMIBLOCK_LEN));
            } else {
                R -= KW_SEMIBLOCK_LEN;
            }
        }

        *out_len = ((semiblocks - 1) * KW_SEMIBLOCK_LEN);
    }

    if (0U != icvData) {
        sea_memcpy(icvData, A, KW_SEMIBLOCK_LEN);
    } else {
        if (0 != memcmp(AES_KW_ICV1, A, KW_SEMIBLOCK_LEN)) {
            tmp_err = ERROR;
        }
    }

    return tmp_err;
}

static ErrStatus aes_key_unwrap_with_padding(const cryptData_t *cryptedData, uint32_t in_len, plainData_t *plainData, uint32_t *out_len) {
    ErrStatus tmp_err = SUCCESS;
    uint8_t icvData[8];
    uint8_t tmp_buf[16];
    const uint8_t *p;
    const uint8_t *q;
    uint32_t src_len, tmp_u32;

    if (in_len == (KW_SEMIBLOCK_LEN * 2U)) {
        sea_memset_word((uint32_t *)tmp_buf, 0U, 4U);
        if (SUCCESS != aes_decrypt_verify_engine((const cryptData_t *)cryptedData, 16U, CypherDatasIsTheLast, (plainData_t *)tmp_buf)) {
            tmp_err = ERROR;
        }

        sea_memcpy(icvData, (uint8_t *)tmp_buf, KW_SEMIBLOCK_LEN);
        sea_memcpy((uint8_t *)plainData, (uint8_t *)(tmp_buf + KW_SEMIBLOCK_LEN), KW_SEMIBLOCK_LEN);
        *out_len = KW_SEMIBLOCK_LEN;
    } else { /* in_len >= (KW_SEMIBLOCK_LEN * 3) */
        if (SUCCESS != aes_key_unwrap((const cryptData_t *)cryptedData, in_len, plainData, out_len, icvData)) {
            tmp_err = ERROR;
        }
    }

    /* check ICV */
    if (0 != memcmp(AES_KW_ICV2, icvData, (KW_SEMIBLOCK_LEN >> 1))) {
        tmp_err = ERROR;
    }

    if (tmp_err == SUCCESS) {
        /* get the plaintext source length without padding */
        tmp_u32 = ((uint32_t)icvData + (KW_SEMIBLOCK_LEN >> 1));
        src_len =  (uint32_t)(__REV(*((uint32_t *)tmp_u32)));
        if ((src_len > *out_len) || ((*out_len > KW_SEMIBLOCK_LEN) && (src_len < (*out_len - 7U)))) {
            tmp_err = ERROR;
        }

        /* check padding */
        p = (plainData + src_len);
        q = (plainData + *out_len);
        while (p < q) {
            if (*p++ != 0) {
                tmp_err = ERROR;
                break;
            }
        }

        *out_len = src_len;
    }

    return tmp_err;
}

/* only for AAD data */
static ErrStatus aes_aad_engine(const add_t *addData, uint32_t size) {
    ErrStatus tmp_err = SUCCESS;
    uint32_t i;

    if (AES_GCM != aes_cryptConfig.cryptMode) {
        tmp_err = ERROR;
    }

    if (tmp_err == SUCCESS) {
        aes_ghash_checkin(g_cypher_paras.aes_buf.h_add); /* copy h_add to h_predata */
        aesInterParameters.is_start_ghash = 1U; /* GHash init start */
        aesInterParameters.aadSize += size;
        if (SUCCESS == aes_drv_ghash_engine(addData, size, SEA_MSG_LITTLE_ENDIAN)) {
            for (i = 0U; i< 4U; i++) {
                g_cypher_paras.aes_buf.h_add[i] = SEA_CYPHER->GHASH_Z[i]; /* save GHash result in h_add */
            }
        } else {
            tmp_err = ERROR;
        }
    }

    return tmp_err;
}

/******************************************************************************
 * PUBLIC INTERFACES
 *****************************************************************************/

/*!
    \fn ErrStatus LL_AES_gf128_mult(const uint8_t *pDataX, const uint8_t *pDataY,  uint8_t *pDataZ)
    \brief               128bit有限域乘法
    \param[in]  pDataX   乘数X
    \param[in]  pDataY   被乘数Y
    \param[out] pDataZ   结果输出
    \retval     SUCCESS/ERROR
*/
ErrStatus LL_AES_gf128_mult(const uint8_t *pDataX, const uint8_t *pDataY,  uint8_t *pDataZ)
{
   return aes_drv_gf128_mult(pDataX, pDataY, pDataZ);
}

/*!
    \brief      crypt:ECB/GCM  deCrypt:ECB/GCM  Initialization
    \param[in]  configuration: pointer to  aes core instance
    \param[out] none
    \retval     ErrStatus: SUCCESS or ERROR
*/
ErrStatus LL_AES_crypt_init(const aesCypherConfigStr*  configuration)
{
    ErrStatus tmp_err = SUCCESS;
    /*REG RESET*/
    clrErrorCode(0xff);
    sea_aes_init();
    /*SET FIRST BLOCK DATA*/
    if(SUCCESS != aes_parameters_init((const aesCypherConfigStr*)configuration)){
        tmp_err =  ERROR;
    }
    return tmp_err;
}

/*!
    \brief    deInitialization, close aes.
    \param[in]  configuration: pointer of aes config
    \param[out] none
    \retval     ErrStatus: SUCCESS or ERROR
*/
ErrStatus LL_AES_crypt_deinit(void)
{
    sea_aes_deinit();
    return SUCCESS  ;
}

/*!
    \brief      AES Crypt:ECB ; Crypt and Sign: GCM
    \param[in]  plainData : pointer of messages to crypt, is specified in words align when not define AES_CRYPT_ITEMS_IN_BYTES
    \param[in]  size      : size of the plainData in bytes
    \param[in]  is_last   : 0：begin hash calc or not finish hash calc;1: when message is last, finish this  hash calc.
    \param[out] cryptedData : pointer to the output buffer, should be larger then size. is specified in words align
    \retval     ErrStatus: SUCCESS or ERROR
*/
ErrStatus LL_AES_crypt_sign(const plainData_t* plainData, uint32_t size, CYPHER_DATA_STA is_last, cryptData_t* cryptedData)
{
    ErrStatus tmp_err = SUCCESS;
    #ifndef AES_CRYPT_ITEMS_IN_BYTES
    /*****check parameters valid*****/
    /*address of plainData and cryptedData must align by 32bit word*/
    if(0U != (((uint32_t)plainData | (uint32_t)cryptedData) & 3U)){
        setErrorCode(AES_CRYPTKEY_ADDR_ALIGN_FAILURE);
        tmp_err = ERROR;
    }
    if(tmp_err == SUCCESS){
    #endif

    if(SUCCESS != aes_crypt_sign_engine(plainData, size, is_last, cryptedData)){
        setErrorCode(AES_CRYPT_FAILURE);
        tmp_err =  ERROR;
    }
    #ifndef AES_CRYPT_ITEMS_IN_BYTES
    }
    #endif
    return tmp_err;
}

/*!
    \brief      AES deCrypt:ECB ; deCrypt and Verify: GCM
    \param[in]  cryptedData : pointer of messages to decrypt, is specified in words align when not define AES_CRYPT_ITEMS_IN_BYTES
    \param[in]  size        : size of the cryptedData in bytes
    \param[in]  is_last     : 0：begin hash calc or not finish hash calc;1: when message is last, finish this  hash calc.
    \param[out] plainData   : pointer to the output buffer, should be larger then size. is specified in words align
    \retval     ErrStatus: SUCCESS or ERROR
*/
ErrStatus LL_AES_decrypt_verify(const cryptData_t* cryptedData, uint32_t size, CYPHER_DATA_STA is_last,  plainData_t* plainData)
{
    ErrStatus tmp_err = SUCCESS;
    #ifndef AES_CRYPT_ITEMS_IN_BYTES
    /*****check parameters valid*****/
    /*address of plainData and cryptedData must align by 32bit word*/
    if(0U != (((uint32_t)cryptedData | (uint32_t)plainData) & 3U)){
        setErrorCode(AES_CRYPTKEY_ADDR_ALIGN_FAILURE);
        tmp_err =  ERROR;
    }
    if(tmp_err == SUCCESS){
    #endif

    if(SUCCESS != aes_decrypt_verify_engine(cryptedData, size, is_last, plainData)){
        setErrorCode(AES_DECRYPT_FAILURE);
        tmp_err =  ERROR;
    }
    #ifndef AES_CRYPT_ITEMS_IN_BYTES
    }
    #endif
    return tmp_err;
}

/*!
    \brief      GMAC,when aes_crypt_init.configuration.encrypt_sel = AES_GMAC_MSG/AES_GMAC_ADD
    \param[in]  ghashData   : pointer of messages to decrypt, is specified in words align when not define AES_CRYPT_ITEMS_IN_BYTES
    \param[in]  size        : size of the cryptedData/addData in bytes
    \param[in]  is_last     : 0：begin hash calc or not finish hash calc;1: when message is last, finish this  hash calc.
    \retval     ErrStatus: SUCCESS or ERROR
*/
ErrStatus LL_AES_gmac(const ghashData_t* ghashData, uint32_t size, CYPHER_DATA_STA is_last)
{
    ErrStatus tmp_err = SUCCESS;
    #ifndef AES_CRYPT_ITEMS_IN_BYTES
    /*****check parameters valid*****/
    /*address of plainData and cryptedData must align by 32bit word*/
    if(0U != (((uint32_t)ghashData) & 3U)){
        setErrorCode(AES_CRYPTKEY_ADDR_ALIGN_FAILURE);
        tmp_err = ERROR;
    }
    if(tmp_err == SUCCESS){
    #endif

    if(SUCCESS != aes_gmac_engine(ghashData, size, is_last)){
        setErrorCode(AES_GMAC_FAILURE);
        tmp_err =  ERROR;
    }
    #ifndef AES_CRYPT_ITEMS_IN_BYTES
    }
    #endif
    return tmp_err;
}

/*!
    \brief      latch the current process status of the aes.
    \param[in]  latch_buf   :pointer to the output buffer which latch the parameters of the aes process.
    \retval     ErrStatus: SUCCESS or ERROR
*/
ErrStatus LL_AES_process_status_latch(aesProcessParameters* latch_buf)
{
    (void)aes_process_latch(latch_buf);
    return SUCCESS;
}

/*!
    \brief      recover the aes process status use the latch parameters before.
    \param[in]  recover_buf   : pointer to  aes latch parameters buffer before.
    \retval     ErrStatus: SUCCESS or ERROR
*/
ErrStatus LL_AES_process_status_recover(const aesProcessParameters* recover_buf)
{
    ErrStatus tmp_err = SUCCESS;
    if(((SEA_CTRL_IR_AES_Enc == recover_buf->engine.crypt_ir)
     || (SEA_CTRL_IR_AES_Dec == recover_buf->engine.crypt_ir))
     &&(recover_buf->engine.cryptMode <= SEA_AES_OFB)){
        if(SUCCESS != aes_process_recover(recover_buf)){
            tmp_err = ERROR;
        }
    }else{
        tmp_err = ERROR;
    }
    return tmp_err;
}

/* Reference Specification RFC-3394 and RFC-5649 */
ErrStatus LL_AES_key_wrap(const plainData_t *plainData, uint32_t in_len, cryptData_t *cryptedData, uint32_t *out_len) {
    ErrStatus tmp_err = SUCCESS;

    if ((cryptedData == 0) || (plainData == 0)) {
        tmp_err = ERROR;
    } else {
        if (aes_cryptConfig.cryptMode == AES_ECB) {
            if (AES_KW_MODE_KWP == aes_cryptConfig.keyWarpMode) {
                if (SUCCESS != aes_key_wrap_with_padding(plainData, in_len, cryptedData, out_len)) {
                    tmp_err = ERROR;
                }
            } else {
                if (SUCCESS != aes_key_wrap(plainData, in_len, cryptedData, out_len)) {
                    tmp_err = ERROR;
                }
            }
        } else {
            tmp_err = ERROR;
        }
    }

    return tmp_err;
}

ErrStatus LL_AES_key_unwrap(const cryptData_t *cryptedData, uint32_t in_len, plainData_t *plainData, uint32_t *out_len) {
    ErrStatus tmp_err = SUCCESS;

    if ((cryptedData == 0) || (plainData == 0)) {
        tmp_err = ERROR;
    } else {
        if (aes_cryptConfig.cryptMode == AES_ECB) {
            if (AES_KW_MODE_KWP == aes_cryptConfig.keyWarpMode) {
                if ((in_len < (KW_SEMIBLOCK_LEN * 2U)) || (0U != (in_len & 0x7U))) {
                    tmp_err = ERROR;
                } else {
                    if (SUCCESS != aes_key_unwrap_with_padding(cryptedData, in_len, plainData, out_len)) {
                        tmp_err = ERROR;
                    }
                }
            } else {
                if (SUCCESS != aes_key_unwrap(cryptedData, in_len, plainData, out_len, 0U)) {
                    tmp_err = ERROR;
                }
            }
        } else {
            tmp_err = ERROR;
        }
    }

    return tmp_err;
}

/*!
    \brief      update AAD data in AES_GCM
    \param[in]  aadData:   pointer to aad message
    \param[in]  size:      size of aad message in bytes
    \retval     ErrStatus: SUCCESS or ERROR
 */
ErrStatus LL_AES_update_aad(const add_t *aadData, uint32_t size) {
    ErrStatus tmp_err = SUCCESS;

    if (aadData == 0U) {
        tmp_err = ERROR;
    } else {
        if (SUCCESS != aes_aad_engine(aadData, size)) {
            tmp_err = ERROR;
        }
    }

    return tmp_err;
}

#else
ErrStatus LL_AES_gf128_mult(const uint8_t *pDataX, const uint8_t *pDataY, uint8_t *pDataZ)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_AES_gf128_mult func_ptr = (pLL_AES_gf128_mult)((uint32_t)Iap_Func->LL_AES_gf128_mult + ROM_BASE_ADDR);
    return func_ptr(&aes_params, pDataX, pDataY, pDataZ);
}

ErrStatus LL_AES_crypt_init(const aesCypherConfigStr *configuration)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_AES_crypt_init func_ptr = (pLL_AES_crypt_init)((uint32_t)Iap_Func->LL_AES_crypt_init + ROM_BASE_ADDR);
    return func_ptr(&aes_params, configuration);
}

ErrStatus LL_AES_crypt_deinit(void)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_AES_crypt_deinit func_ptr = (pLL_AES_crypt_deinit)((uint32_t)Iap_Func->LL_AES_crypt_deinit + ROM_BASE_ADDR);
    return func_ptr();
}

ErrStatus LL_AES_crypt_sign(const plainData_t *plainData, uint32_t size, CYPHER_DATA_STA is_last, cryptData_t *cryptedData)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_AES_crypt_sign func_ptr = (pLL_AES_crypt_sign)((uint32_t)Iap_Func->LL_AES_crypt_sign + ROM_BASE_ADDR);
    return func_ptr(&aes_params, plainData, size, is_last, cryptedData);
}

ErrStatus LL_AES_decrypt_verify(const cryptData_t *cryptedData, uint32_t size, CYPHER_DATA_STA is_last,  plainData_t *plainData)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_AES_decrypt_verify func_ptr = (pLL_AES_decrypt_verify)((uint32_t)Iap_Func->LL_AES_decrypt_verify + ROM_BASE_ADDR);
    return func_ptr(&aes_params, cryptedData, size, is_last, plainData);
}

ErrStatus LL_AES_gmac(const ghashData_t *ghashData, uint32_t size, CYPHER_DATA_STA is_last)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_AES_gmac func_ptr = (pLL_AES_gmac)((uint32_t)Iap_Func->LL_AES_gmac + ROM_BASE_ADDR);
    return func_ptr(&aes_params, ghashData, size, is_last);
}

ErrStatus LL_AES_process_status_latch(aesProcessParameters *latch_buf)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_AES_process_status_latch func_ptr = (pLL_AES_process_status_latch)((uint32_t)Iap_Func->LL_AES_process_status_latch + ROM_BASE_ADDR);
    return func_ptr(&aes_params, latch_buf);
}

ErrStatus LL_AES_process_status_recover(const aesProcessParameters *recover_buf)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_AES_process_status_recover func_ptr = (pLL_AES_process_status_recover)((uint32_t)Iap_Func->LL_AES_process_status_recover + ROM_BASE_ADDR);
    return func_ptr(recover_buf, &aes_params);
}

ErrStatus LL_AES_key_wrap(const plainData_t *plainData, uint32_t in_len, cryptData_t *cryptedData, uint32_t *out_len)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_AES_key_wrap func_ptr = (pLL_AES_key_wrap)((uint32_t)Iap_Func->LL_AES_key_wrap + ROM_BASE_ADDR);
    return func_ptr(&aes_params, plainData, in_len, cryptedData, out_len);
}

ErrStatus LL_AES_key_unwrap(const cryptData_t *cryptedData, uint32_t in_len, plainData_t *plainData, uint32_t *out_len)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_AES_key_unwrap func_ptr = (pLL_AES_key_unwrap)((uint32_t)Iap_Func->LL_AES_key_unwrap + ROM_BASE_ADDR);
    return func_ptr(&aes_params, cryptedData, in_len, plainData, out_len);
}

ErrStatus LL_AES_update_aad(const add_t *aadData, uint32_t size)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_AES_update_aad func_ptr = (pLL_AES_update_aad)((uint32_t)Iap_Func->LL_AES_update_aad + ROM_BASE_ADDR);
    return func_ptr(&aes_params, aadData, size);
}
#endif

/* r2894 */
