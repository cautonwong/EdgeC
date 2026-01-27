/******************************************************************************
 * @file        rn8xxx_ll_sea_hash.c
 * @brief       HASH functions
 * @author      Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 *****************************************************************************/
#include "rn8xxx_ll_sea_hash.h"

#if !defined(RN831x_RN861x_MCU_V4)
static ErrStatus hash_engine_kernel_32bit(const regData_t pDataIn[], hashEngineParameters *pParas);
static ErrStatus hash_engine_kernel_64bit(const regData_t pDataIn[], hashEngineParameters *pParas);
static ErrStatus hash_engine(const hashData_t pDataIn[], uint8_t size, hashData_t pDataOut[], SHA_BLOCK_DATA_TPYE block_type);
static ErrStatus hash_engine_manager(const hashData_t pDataIn[], uint8_t size, uint8_t last_block, hashData_t pDataOut[]);
static ErrStatus hash_parameters_init(const hashConfigStr*  configuration);
static ErrStatus hash_proc(const hashData_t* verifyMessage, uint32_t size, CYPHER_DATA_STA is_last, hashData_t *verifyOut);
static ErrStatus hash_process_latch(hashProcessParameters* latch_buf);
static ErrStatus hash_process_recover(const hashProcessParameters* recover_buf);

#ifdef  RN821x_RN721x_SOC_V3
static void sw_sha160_transform(SW_HASH256_Engine_t *sh);
static void sw_sha256_transform(SW_HASH256_Engine_t *sh);
static void sw_sha512_transform(SW_HASH512_Engine_t *sh);
#endif

static const uint32_t hash_sha_160_256_const_iv[3][NIV_SHA_384_256] ={
    { 0x67452301U, 0x0U, 0xEFCDAB89U, 0x0U, 0x98BADCFEU, 0x0U, 0x10325476U, 0x0U, 0xC3D2E1F0U, 0x0U, 0x00000000U, 0x0U, 0x00000000U, 0x0U, 0x00000000U, 0x0U},
    { 0xC1059ED8U, 0x0U, 0x367CD507U, 0x0U, 0x3070DD17U, 0x0U, 0xF70E5939U, 0x0U, 0xFFC00B31U, 0x0U, 0x68581511U, 0x0U, 0x64F98FA7U, 0x0U, 0xBEFA4FA4U, 0x0U},
    { 0x6A09E667U, 0x0U, 0xBB67AE85U, 0x0U, 0x3C6EF372U, 0x0U, 0xA54FF53AU, 0x0U, 0x510E527FU, 0x0U, 0x9B05688CU, 0x0U, 0x1F83D9ABU, 0x0U, 0x5BE0CD19U, 0x0U},
};

static const uint32_t hash_sha_384_512_const_iv[2][NIV_SHA_384_256] ={
    {
        0xC1059ED8U, 0xCBBB9D5DU, 0x367CD507U, 0x629A292AU, 0x3070DD17U, 0x9159015AU, 0xF70E5939U, 0x152FECD8U ,
        0xFFC00B31U, 0x67332667U, 0x68581511U, 0x8EB44A87U, 0x64F98FA7U, 0xDB0C2E0DU, 0xBEFA4FA4U, 0x47B5481DU
    }
    ,
    {
        0xF3BCC908U, 0x6A09E667U, 0x84CAA73BU, 0xBB67AE85U, 0xFE94F82BU, 0x3C6EF372U, 0x5F1D36F1U, 0xA54FF53AU,
        0xADE682D1U, 0x510E527FU, 0x2B3E6C1FU, 0x9B05688CU, 0xFB41BD6BU, 0x1F83D9ABU, 0x137E2179U, 0x5BE0CD19U
    }
};

#ifdef  RN821x_RN721x_SOC_V3

#define HASH160_K0 0x5a827999U
#define HASH160_K1 0x6ed9eba1U
#define HASH160_K2 0x8f1bbcdcU
#define HASH160_K3 0xca62c1d6U

static const uint32_t HASH256_K[64]={
0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U};

#ifdef __GNUC__
static const uint64_t HASH512_K[80]=
{0x428a2f98d728ae22LL,0x7137449123ef65cdLL,0xb5c0fbcfec4d3b2fLL,0xe9b5dba58189dbbcLL,
0x3956c25bf348b538LL,0x59f111f1b605d019LL,0x923f82a4af194f9bLL,0xab1c5ed5da6d8118LL,
0xd807aa98a3030242LL,0x12835b0145706fbeLL,0x243185be4ee4b28cLL,0x550c7dc3d5ffb4e2LL,
0x72be5d74f27b896fLL,0x80deb1fe3b1696b1LL,0x9bdc06a725c71235LL,0xc19bf174cf692694LL,
0xe49b69c19ef14ad2LL,0xefbe4786384f25e3LL,0x0fc19dc68b8cd5b5LL,0x240ca1cc77ac9c65LL,
0x2de92c6f592b0275LL,0x4a7484aa6ea6e483LL,0x5cb0a9dcbd41fbd4LL,0x76f988da831153b5LL,
0x983e5152ee66dfabLL,0xa831c66d2db43210LL,0xb00327c898fb213fLL,0xbf597fc7beef0ee4LL,
0xc6e00bf33da88fc2LL,0xd5a79147930aa725LL,0x06ca6351e003826fLL,0x142929670a0e6e70LL,
0x27b70a8546d22ffcLL,0x2e1b21385c26c926LL,0x4d2c6dfc5ac42aedLL,0x53380d139d95b3dfLL,
0x650a73548baf63deLL,0x766a0abb3c77b2a8LL,0x81c2c92e47edaee6LL,0x92722c851482353bLL,
0xa2bfe8a14cf10364LL,0xa81a664bbc423001LL,0xc24b8b70d0f89791LL,0xc76c51a30654be30LL,
0xd192e819d6ef5218LL,0xd69906245565a910LL,0xf40e35855771202aLL,0x106aa07032bbd1b8LL,
0x19a4c116b8d2d0c8LL,0x1e376c085141ab53LL,0x2748774cdf8eeb99LL,0x34b0bcb5e19b48a8LL,
0x391c0cb3c5c95a63LL,0x4ed8aa4ae3418acbLL,0x5b9cca4f7763e373LL,0x682e6ff3d6b2b8a3LL,
0x748f82ee5defb2fcLL,0x78a5636f43172f60LL,0x84c87814a1f0ab72LL,0x8cc702081a6439ecLL,
0x90befffa23631e28LL,0xa4506cebde82bde9LL,0xbef9a3f7b2c67915LL,0xc67178f2e372532bLL,
0xca273eceea26619cLL,0xd186b8c721c0c207LL,0xeada7dd6cde0eb1eLL,0xf57d4f7fee6ed178LL,
0x06f067aa72176fbaLL,0x0a637dc5a2c898a6LL,0x113f9804bef90daeLL,0x1b710b35131c471bLL,
0x28db77f523047d84LL,0x32caab7b40c72493LL,0x3c9ebe0a15c9bebcLL,0x431d67c49c100d4cLL,
0x4cc5d4becb3e42b6LL,0x597f299cfc657e2aLL,0x5fcb6fab3ad6faecLL,0x6c44198c4a475817LL};
#else
static const uint64_t HASH512_K[80]=
{0x428a2f98d728ae22U,0x7137449123ef65cdU,0xb5c0fbcfec4d3b2fU,0xe9b5dba58189dbbcU,
 0x3956c25bf348b538U,0x59f111f1b605d019U,0x923f82a4af194f9bU,0xab1c5ed5da6d8118U,
 0xd807aa98a3030242U,0x12835b0145706fbeU,0x243185be4ee4b28cU,0x550c7dc3d5ffb4e2U,
 0x72be5d74f27b896fU,0x80deb1fe3b1696b1U,0x9bdc06a725c71235U,0xc19bf174cf692694U,
 0xe49b69c19ef14ad2U,0xefbe4786384f25e3U,0x0fc19dc68b8cd5b5U,0x240ca1cc77ac9c65U,
 0x2de92c6f592b0275U,0x4a7484aa6ea6e483U,0x5cb0a9dcbd41fbd4U,0x76f988da831153b5U,
 0x983e5152ee66dfabU,0xa831c66d2db43210U,0xb00327c898fb213fU,0xbf597fc7beef0ee4U,
 0xc6e00bf33da88fc2U,0xd5a79147930aa725U,0x06ca6351e003826fU,0x142929670a0e6e70U,
 0x27b70a8546d22ffcU,0x2e1b21385c26c926U,0x4d2c6dfc5ac42aedU,0x53380d139d95b3dfU,
 0x650a73548baf63deU,0x766a0abb3c77b2a8U,0x81c2c92e47edaee6U,0x92722c851482353bU,
 0xa2bfe8a14cf10364U,0xa81a664bbc423001U,0xc24b8b70d0f89791U,0xc76c51a30654be30U,
 0xd192e819d6ef5218U,0xd69906245565a910U,0xf40e35855771202aU,0x106aa07032bbd1b8U,
 0x19a4c116b8d2d0c8U,0x1e376c085141ab53U,0x2748774cdf8eeb99U,0x34b0bcb5e19b48a8U,
 0x391c0cb3c5c95a63U,0x4ed8aa4ae3418acbU,0x5b9cca4f7763e373U,0x682e6ff3d6b2b8a3U,
 0x748f82ee5defb2fcU,0x78a5636f43172f60U,0x84c87814a1f0ab72U,0x8cc702081a6439ecU,
 0x90befffa23631e28U,0xa4506cebde82bde9U,0xbef9a3f7b2c67915U,0xc67178f2e372532bU,
 0xca273eceea26619cU,0xd186b8c721c0c207U,0xeada7dd6cde0eb1eU,0xf57d4f7fee6ed178U,
 0x06f067aa72176fbaU,0x0a637dc5a2c898a6U,0x113f9804bef90daeU,0x1b710b35131c471bU,
 0x28db77f523047d84U,0x32caab7b40c72493U,0x3c9ebe0a15c9bebcU,0x431d67c49c100d4cU,
 0x4cc5d4becb3e42b6U,0x597f299cfc657e2aU,0x5fcb6fab3ad6faecU,0x6c44198c4a475817U};
#endif

static SEA_HASH_TypeDef  hash_sha_sram_regs;
static SEA_HASH_TypeDef *SEA_HASH;
#endif

static const uint8_t hash_sha_bit32_const_dataOutLen[3] = {NOUT_SHA_1, NOUT_SHA_224, NOUT_SHA_256};
static const uint8_t hash_sha_bit64_const_dataOutLen[2] = {NOUT_SHA_384, NOUT_SHA_512};

static hashConfigStr            hash_VerifyConfig;
static hashEngineParameters     hashParameters   ;

#ifdef  RN821x_RN721x_SOC_V3
#define FIX

#define S160(n,x) (((x)<<(n)) | ((x)>>(32-(n))))
#define F160_0(x,y,z) ((z)^((x)&((y)^(z))))
#define F160_1(x,y,z) ((x)^(y)^(z))
#define F160_2(x,y,z) (((x)&(y)) | ((z)&((x)|(y))))
#define F160_3(x,y,z) ((x)^(y)^(z))

#define S256(n,x) (((x)>>(n)) | ((x)<<(32-(n))))
#define R256(n,x) ((x)>>(n))

#define Ch256(x,y,z)  (((x)&(y))^(~(x)&(z)))
#define Maj256(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
#define Sig256_0(x)    (S256(2,x)^S256(13,x)^S256(22,x))
#define Sig256_1(x)    (S256(6,x)^S256(11,x)^S256(25,x))
#define theta256_0(x)  (S256(7,x)^S256(18,x)^R256(3,x))
#define theta256_1(x)  (S256(17,x)^S256(19,x)^R256(10,x))

#define S512(n,x) (((x)>>(n)) | ((x)<<(64-(n))))
#define R512(n,x) ((x)>>(n))

#define Ch512(x,y,z)  (((x)&(y))^(~(x)&(z)))
#define Maj512(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
#define Sig512_0(x)    (S512(28,x)^S512(34,x)^S512(39,x))
#define Sig512_1(x)    (S512(14,x)^S512(18,x)^S512(41,x))
#define theta512_0(x)  (S512(1,x)^S512(8,x)^R512(7,x))
#define theta512_1(x)  (S512(19,x)^S512(61,x)^R512(6,x))

static void sw_sha160_transform(SW_HASH256_Engine_t *sh)
{ /* basic transformation step */
    uint32_t a,b,c,d,e,temp;
    uint32_t t;
#ifdef FIX
    for (t=16;t<80;t++){
        sh->w[t]=S160(1,sh->w[t-3]^sh->w[t-8]^sh->w[t-14]^sh->w[t-16]);
    }
#else
    for (t=16;t<80;t++)
        sh->w[t]=sh->w[t-3]^sh->w[t-8]^sh->w[t-14]^sh->w[t-16];
#endif
    a=sh->h[0]; b=sh->h[1]; c=sh->h[2]; d=sh->h[3]; e=sh->h[4];
    for (t=0;t<20;t++)
    { /* 20 times - mush it up */
        temp=HASH160_K0+F160_0(b,c,d)+S160(5,a)+e+sh->w[t];
        e=d; d=c;
        c=S160(30,b);
        b=a; a=temp;
    }
    for (t=20;t<40;t++)
    { /* 20 more times - mush it up */
        temp=HASH160_K1+F160_1(b,c,d)+S160(5,a)+e+sh->w[t];
        e=d; d=c;
        c=S160(30,b);
        b=a; a=temp;
    }
    for (t=40;t<60;t++)
    { /* 20 more times - mush it up */
        temp=HASH160_K2+F160_2(b,c,d)+S160(5,a)+e+sh->w[t];
        e=d; d=c;
        c=S160(30,b);
        b=a; a=temp;
    }
    for (t=60;t<80;t++)
    { /* 20 more times - mush it up */
        temp=HASH160_K3+F160_3(b,c,d)+S160(5,a)+e+sh->w[t];
        e=d; d=c;
        c=S160(30,b);
        b=a; a=temp;
    }
    sh->h[0]+=a; sh->h[1]+=b; sh->h[2]+=c;
    sh->h[3]+=d; sh->h[4]+=e;
}

static void sw_sha256_transform(SW_HASH256_Engine_t *sh)
{ /* basic transformation step */
    uint32_t a,b,c,d,e,f,g,h,t1,t2;
    uint32_t j;
    for (j=16U;j<64U;j++) {
        sh->w[j]=theta256_1(sh->w[j-2])+sh->w[j-7]+theta256_0(sh->w[j-15])+sh->w[j-16];
    }

    a=sh->h[0]; b=sh->h[1]; c=sh->h[2]; d=sh->h[3];
    e=sh->h[4]; f=sh->h[5]; g=sh->h[6]; h=sh->h[7];

    for (j=0;j<64;j++)
    { /* 64 times - mush it up */
        t1=h+Sig256_1(e)+Ch256(e,f,g)+HASH256_K[j]+sh->w[j];
        t2=Sig256_0(a)+Maj256(a,b,c);
        h=g; g=f; f=e;
        e=d+t1;
        d=c;
        c=b;
        b=a;
        a=t1+t2;
    }
    sh->h[0]+=a; sh->h[1]+=b; sh->h[2]+=c; sh->h[3]+=d;
    sh->h[4]+=e; sh->h[5]+=f; sh->h[6]+=g; sh->h[7]+=h;
}

static void sw_sha512_transform(SW_HASH512_Engine_t *sh)
{ /* basic transformation step */
    uint64_t a,b,c,d,e,f,g,h,t1,t2;
    uint32_t j;
    for (j=16U;j<80U;j++){
        sh->w[j]=theta512_1(sh->w[j-2])+sh->w[j-7]+theta512_0(sh->w[j-15])+sh->w[j-16];
    }
    a=sh->h[0]; b=sh->h[1]; c=sh->h[2]; d=sh->h[3];
    e=sh->h[4]; f=sh->h[5]; g=sh->h[6]; h=sh->h[7];

    for (j=0;j<80;j++)
    { /* 80 times - mush it up */
        t1=h+Sig512_1(e)+Ch512(e,f,g)+HASH512_K[j]+sh->w[j];
        t2=Sig512_0(a)+Maj512(a,b,c);
        h=g; g=f; f=e;
        e=d+t1;
        d=c;
        c=b;
        b=a;
        a=t1+t2;
    }
    sh->h[0]+=a; sh->h[1]+=b; sh->h[2]+=c; sh->h[3]+=d;
    sh->h[4]+=e; sh->h[5]+=f; sh->h[6]+=g; sh->h[7]+=h;
}
#endif

static ErrStatus hash_engine_kernel_32bit(const regData_t pDataIn[], hashEngineParameters *pParas)
{
    uint32_t i, tmp_index ;
    ErrStatus tmp_err = SUCCESS;
#ifdef  RN821x_RN721x_SOC_V3
    SW_HASH256_Engine_t tmp_engine;
#endif
    struct Hash32Iv{
      uint32_t data[16];
    }const *pHash32Iv;

    if(0U != pParas->is_start){
        pHash32Iv = (struct Hash32Iv *)(uint32_t)pParas->pIv ;
        pParas->is_start = 0U  ;
    }
    else{
        pHash32Iv = (struct Hash32Iv *)((uint32_t)SEA_HASH->HASH_DOUR);
    }

#ifdef  RN821x_RN721x_SOC_V3
    /*IV*/
    tmp_index = 0U;
    for(i=0U; i<NIV_SHA_224_256; i++){
        tmp_engine.h[i] = pHash32Iv->data[tmp_index];
        tmp_index += 2U;
    }

    /*DATA IN*/
    for(i=0U; i<((uint32_t)pParas->blockSize >> 2U); i++){
        tmp_engine.w[i] = (uint32_t)(__REV(pDataIn[i]));
    }

    if(hash_VerifyConfig.bitWideMode != SEA_SHA_1){
        sw_sha256_transform(&tmp_engine);
        for(i=0U; i< ((uint32_t)NIV_SHA_224_256); i++){
            SEA_HASH->HASH_DOUR[2U*i] = tmp_engine.h[i];
        }
    }else{
        sw_sha160_transform(&tmp_engine);
        for(i=0U; i< ((uint32_t)NIV_SHA_160); i++){
            SEA_HASH->HASH_DOUR[2U*i] = tmp_engine.h[i];
        }
    }
#else
    /*IV*/
    tmp_index = 0U;
    for(i=0U; i<pParas->ivLen; i++){
        SEA_HASH->HASH_IVR[tmp_index] = pHash32Iv->data[tmp_index];
        tmp_index += 2U;
    }
    /*DATA IN*/
    tmp_index = 0U;
    for(i=0U; i<((uint32_t)pParas->blockSize >> 2U); i++){
        SEA_HASH->HASH_DINR[tmp_index] = (uint32_t)(__REV(pDataIn[i]));
        tmp_index += 2U;
    }

    SEA_CYPHER->CTRL = ((uint32_t)hash_VerifyConfig.bitWideMode << SEA_CTRL_SHA_MOD_Pos)  | ((uint32_t)SEA_CTRL_IR_SHA0 << SEA_CTRL_IR_Pos);
    SEA_CYPHER->Status = SEA_STA_HASH_Msk;
    SEA_CYPHER->START = 1U;

    i = 0U;
    while(0U == (SEA_CYPHER->Status & SEA_STA_HASH_Msk)){
        if(i++ > 400U){
            tmp_err =    ERROR;
            break;
        }
    }
#endif

    return tmp_err;
}

static ErrStatus hash_engine_kernel_64bit(const regData_t pDataIn[], hashEngineParameters *pParas)
{
    uint32_t i, tmp_index ;
    ErrStatus tmp_err = SUCCESS;
#ifdef  RN821x_RN721x_SOC_V3
    SW_HASH512_Engine_t tmp_engine;
#endif
    struct HashIv{
      uint32_t data[16];
    }const *pHashIv;

    if(0U != pParas->is_start){
        pHashIv = (struct HashIv *)(uint32_t)pParas->pIv ;
        pParas->is_start = 0U  ;
    }
    else{
        pHashIv = (struct HashIv *)((uint32_t)SEA_HASH->HASH_DOUR) ;
    }
#ifdef  RN821x_RN721x_SOC_V3
    /*IV*/
    tmp_index = 0U;
    for(i=0U; i<(NIV_SHA_384_256/2); i++){
        tmp_engine.h[i] = (uint64_t)pHashIv->data[tmp_index + 1] << 32 | pHashIv->data[tmp_index];
        tmp_index += 2U;
    }

    /*DATA IN*/
    tmp_index = 0U;
    for(i=0U; i<((uint32_t)pParas->blockSize >> 3U); i++){
        tmp_engine.w[i] = (uint64_t)(__REV(pDataIn[tmp_index])) <<32 | (uint64_t)(__REV(pDataIn[tmp_index + 1]));
        tmp_index += 2U;
    }

    sw_sha512_transform(&tmp_engine);
    tmp_index = 0U;
    for(i=0U; i< ((uint32_t)NIV_SHA_384_256/2); i++){
        SEA_HASH->HASH_DOUR[tmp_index] = (uint32_t)tmp_engine.h[i];
        SEA_HASH->HASH_DOUR[tmp_index+1] = (uint32_t)(tmp_engine.h[i] >> 32);
        tmp_index += 2U;
    }
#else
    /*IV*/
    tmp_index = 0U;
    for(i=0U; i<pParas->ivLen; i++){
        SEA_HASH->HASH_IVR[tmp_index] = pHashIv->data[tmp_index];
        tmp_index++ ;
    }
    /*DATA IN*/

    tmp_index = 0U;
    for(i=0U; i<((uint32_t)pParas->blockSize >> 3U); i++){
        SEA_HASH->HASH_DINR[tmp_index]      = (uint32_t)(__REV(pDataIn[tmp_index + 1U])); /*32bit swap, data in odd to reg even*/
        SEA_HASH->HASH_DINR[tmp_index + 1U] = (uint32_t)(__REV(pDataIn[tmp_index]));      /*32bit swap, data in even to reg odd*/
        tmp_index += 2U;
    }

    SEA_CYPHER->CTRL = ((uint32_t)hash_VerifyConfig.bitWideMode << SEA_CTRL_SHA_MOD_Pos) | ((uint32_t)SEA_CTRL_IR_SHA2 << SEA_CTRL_IR_Pos);
    SEA_CYPHER->Status = SEA_STA_HASH_Msk;
    SEA_CYPHER->START = 1U;

    i = 0U;
    while(0U == (SEA_CYPHER->Status & SEA_STA_HASH_Msk)){
        if(i++ > 400U){
            tmp_err = ERROR;
            break;
        }
    }
#endif
    return tmp_err;
}

static ErrStatus hash_engine(const hashData_t pDataIn[], uint8_t size, hashData_t pDataOut[], SHA_BLOCK_DATA_TPYE block_type)
{
    regData_t tmp_msg_u32[NBLOCK_DATA_SHA_MAX];
    uint32_t i, tmp_addr;
    ErrStatus tmp_err = SUCCESS;
    struct HashDout{
      regData_t data[16];
    }*pHashDout;
    const struct HashDataIn{
      regData_t data[NBLOCK_DATA_SHA_MAX];
    }*pHashDataIn;
    #ifdef HASH_CRYPT_ITEMS_IN_BYTES
    regData_t tmp_out_u32[NOUT_SHA_MAX];
    pHashDout =  (struct HashDout *)((uint32_t)tmp_out_u32);
    #else
    pHashDout =  (struct HashDout *)(uint32_t)pDataOut;
    #endif

    /*the last block message*/
    if(block_type  == SHA_MSG_ONLY) {
        #ifdef HASH_CRYPT_ITEMS_IN_BYTES
        pHashDataIn = (struct HashDataIn *)((uint32_t)tmp_msg_u32);
        SEA_MEMCPY(tmp_msg_u32, pDataIn, (uint32_t)size);
        #else
        pHashDataIn = (struct HashDataIn *)(uint32_t)pDataIn;
        #endif
    }
    else/* if((block_type == SHA_MSG_FILL_ADD) || (block_type == SHA_MSG_FILL) || (block_type == SHA_ZERO_ADD) )*/
    {
        pHashDataIn = (struct HashDataIn *)((uint32_t)tmp_msg_u32);
        /*clear buffer*/
        sea_memset_word(tmp_msg_u32, 0U, (uint32_t)hashParameters.blockSize >>2U);
        if(block_type != SHA_ZERO_ADD){
            /*data*/
            (void)memcpy(tmp_msg_u32, pDataIn, (uint32_t)size);
            /*fill 0x80*/
            tmp_addr = (uint32_t)tmp_msg_u32 + size;
            *((uint8_t *)tmp_addr) = 0x80U;
        }
        if(block_type != SHA_MSG_FILL){
            /*datasize*/
            tmp_msg_u32[(hashParameters.blockSize>>2) - 1U] = (uint32_t)(__REV(hashParameters.messageSize  << 3U));
            tmp_msg_u32[(hashParameters.blockSize>>2) - 2U] = (uint32_t)(__REV((hashParameters.messageSize >> 29U) & 0x7U));
            /*hash_block_data_msg_leninfo_add(&tmp_msg_u32[(hashParameters.blockSize>>2) - 1], &hash_VerifyConfig.msgSize, hashParameters.MsgAddLenSize);*/
        }
    }
#if 0
    else{
       sea_memset_word(tmp_msg_u32, 0U, (uint32_t)hashParameters.blockSize >>2U);
       pHashDataIn = (struct HashDataIn *)((uint32_t)tmp_msg_u32);
       tmp_err = ERROR;
    }
#endif

    if(SEA_CTRL_IR_SHA0 == hashParameters.ctr_ir){
      if(SUCCESS !=  hash_engine_kernel_32bit(pHashDataIn->data, &hashParameters)){
        tmp_err = ERROR;
      }
    }
    else{
        if(SUCCESS !=  hash_engine_kernel_64bit(pHashDataIn->data, &hashParameters)){
            tmp_err = ERROR;
        }
    }
    /*when the last message, verify results ouput*/
    if((block_type == SHA_MSG_FILL_ADD) || (block_type == SHA_ZERO_ADD)){
        if(SEA_CTRL_IR_SHA0 == hashParameters.ctr_ir){
            for(i=0U; i< (hashParameters.dataOutLen); i++){
                pHashDout->data[i] = (uint32_t)(__REV(SEA_HASH->HASH_DOUR[2U*i]));
            }
        }
        else{
            for(i=0U; i< ((uint32_t)hashParameters.dataOutLen >> 1U); i++){
                  pHashDout->data[2U*i]     = (uint32_t)(__REV(SEA_HASH->HASH_DOUR[(2U*i) + 1U]));
                  pHashDout->data[(2U*i)+1U]   = (uint32_t)(__REV(SEA_HASH->HASH_DOUR[(2U*i)]));
            }
        }
        #ifdef HASH_CRYPT_ITEMS_IN_BYTES
        SEA_MEMCPY(pDataOut, (uint8_t *)(uint32_t)pHashDout, (uint32_t)hashParameters.dataOutLen << 2U);
        #endif
    }

    return tmp_err;
}

static ErrStatus hash_engine_manager(const hashData_t pDataIn[], uint8_t size, uint8_t last_block, hashData_t pDataOut[])
{
    ErrStatus tmp_err = SUCCESS;
    hashParameters.messageSize += size;
    /*not the last block message*/
    if(0U == last_block){
        if(SUCCESS !=  hash_engine(pDataIn, size, pDataOut, SHA_MSG_ONLY)) {
            tmp_err =  ERROR;
        }
    }
    else {
        if(size < (hashParameters.blockSize - hashParameters.MsgAddLenSize)) {
            /*data(size = (0 : blockSize - addSize - 1) + fill + add*/
            if(SUCCESS != hash_engine(pDataIn, size, pDataOut, SHA_MSG_FILL_ADD)) {
                tmp_err =  ERROR;
            }
        }
        else{
            /*data + 0x80 + zero*/
            if(SUCCESS != hash_engine(pDataIn, size, pDataOut, SHA_MSG_FILL)) {
                tmp_err =  ERROR;
            }
            /*zero + msg_size*/
            if(SUCCESS != hash_engine(pDataIn, 0U, pDataOut, SHA_ZERO_ADD)) {
                tmp_err =  ERROR;
            }
        }
        /*next message frame  continue*/
        hashParameters.is_start = 1U;
        hashParameters.messageSize = 0U;
    }
    return tmp_err;
}

static ErrStatus hash_parameters_init(const hashConfigStr* configuration)
{
    ErrStatus tmp_err = SUCCESS;
    uint32_t tmp_addr;

    if((configuration->bitWideMode > SEA_SHA_512) || (configuration->bitWideMode == SEA_SHA_NC3) ){
        tmp_err =  ERROR;
    }

    hash_VerifyConfig.bitWideMode = configuration->bitWideMode;

    hashParameters.is_start    = 1U;
    hashParameters.messageSize = 0U;

    if(hash_VerifyConfig.bitWideMode <= SEA_SHA_256)
    {
        tmp_addr = (uint32_t)&hash_sha_160_256_const_iv[hash_VerifyConfig.bitWideMode][0];
        hashParameters.pIv = (uint32_t *)tmp_addr;
        if(hash_VerifyConfig.bitWideMode == SEA_SHA_1){
            hashParameters.ivLen =  NIV_SHA_160;
        }else{
            hashParameters.ivLen =  NIV_SHA_224_256;
        }
        hashParameters.blockSize     = (uint8_t)(NBLOCK_DATA_SHA_160_256*sizeof(uint32_t));
        hashParameters.blockShift    = BSHIFT_BLOCK_DATA_SHA_160_256;

        hashParameters.MsgAddLenSize    = (uint8_t)(2U*sizeof(uint32_t));
        hashParameters.ctr_ir           = SEA_CTRL_IR_SHA0  ;

        hashParameters.dataOutLen   = hash_sha_bit32_const_dataOutLen[hash_VerifyConfig.bitWideMode];
    }
    else
    {
        tmp_addr = (uint32_t)&hash_sha_384_512_const_iv[(uint32_t)hash_VerifyConfig.bitWideMode - (uint32_t)SEA_SHA_384][0];
        hashParameters.pIv = (uint32_t *)tmp_addr;
        hashParameters.ivLen       =  NIV_SHA_384_256;
        hashParameters.blockSize   =  (uint8_t)(NBLOCK_DATA_SHA_384_512*sizeof(uint32_t));
        hashParameters.blockShift   = BSHIFT_BLOCK_DATA_SHA_384_512;
        hashParameters.MsgAddLenSize   = (uint8_t)(4U*sizeof(uint32_t));
        hashParameters.ctr_ir          = SEA_CTRL_IR_SHA2  ;
        hashParameters.dataOutLen  = hash_sha_bit64_const_dataOutLen[(uint32_t)hash_VerifyConfig.bitWideMode - (uint32_t)SEA_SHA_384];
    }
    g_cypher_paras.hash_buf.datain.size_last = 0U;

    #ifdef  RN821x_RN721x_SOC_V3
    SEA_HASH = &hash_sha_sram_regs;
    #endif

    return tmp_err;
}

static ErrStatus hash_proc(const hashData_t *verifyMessage, uint32_t size, CYPHER_DATA_STA is_last, hashData_t *verifyOut)
{
    uint32_t i, loops,  size_cur, tmp_addr, tmp_addr_merge;
    uint8_t  size_merge;
    ErrStatus tmp_err = SUCCESS;

    /*pre last hash datas*/
    if(0U != g_cypher_paras.hash_buf.datain.size_last)
    {
        if((g_cypher_paras.hash_buf.datain.size_last + size) >= hashParameters.blockSize){
            size_merge = hashParameters.blockSize - g_cypher_paras.hash_buf.datain.size_last;
            /*merging datas*/
             tmp_addr_merge = (uint32_t)g_cypher_paras.hash_buf.datain.datas + g_cypher_paras.hash_buf.datain.size_last;
             (void)memcpy((uint8_t *)tmp_addr_merge, (uint8_t *)(uint32_t)verifyMessage, (uint32_t)size_merge);
            if(SUCCESS != hash_engine_manager((const hashData_t  *)(uint32_t)g_cypher_paras.hash_buf.datain.datas, hashParameters.blockSize, 0U, (hashData_t *)verifyOut)){
                tmp_err =  ERROR;
            }
            g_cypher_paras.hash_buf.datain.size_last = 0U;
            size_cur = size - size_merge;
            tmp_addr = (uint32_t)verifyMessage ;
            tmp_addr += size_merge;
        }else{
            if(CypherDatasIsTheLast == is_last){
                /*merging datas*/
                tmp_addr_merge = (uint32_t)g_cypher_paras.hash_buf.datain.datas + g_cypher_paras.hash_buf.datain.size_last;
                (void)memcpy((uint8_t *)tmp_addr_merge, (uint8_t *)(uint32_t)verifyMessage, size);
                size_cur = g_cypher_paras.hash_buf.datain.size_last + size;
                tmp_addr = (uint32_t)g_cypher_paras.hash_buf.datain.datas;
            }else{
                size_cur = size;
                tmp_addr = (uint32_t)verifyMessage;
            }
        }
    }
    else{
        size_cur = size;
        tmp_addr = (uint32_t)verifyMessage;
    }

    /*block loops hash datas*/
    loops    = size_cur >> hashParameters.blockShift;
    for(i=0U; i<loops; i++){
        if(SUCCESS != hash_engine_manager((const hashData_t *)tmp_addr, hashParameters.blockSize, 0U, (hashData_t *)verifyOut)){
            tmp_err =  ERROR;
        }
        tmp_addr += hashParameters.blockSize ;
    }

    /*cur last hash datas*/
    size_merge = (uint8_t)(size_cur - (loops*(uint32_t)hashParameters.blockSize));

    if(CypherDatasIsTheLast == is_last){
        if(SUCCESS != hash_engine_manager((const hashData_t *)tmp_addr, size_merge, 1U, (hashData_t *)verifyOut)){
            tmp_err =  ERROR;
        }
        g_cypher_paras.hash_buf.datain.size_last = 0U;
    }else{
        tmp_addr_merge = (uint32_t)g_cypher_paras.hash_buf.datain.datas + g_cypher_paras.hash_buf.datain.size_last;
        (void)memcpy((uint8_t *)tmp_addr_merge, (const uint8_t *)tmp_addr, (uint32_t)size_merge);
        g_cypher_paras.hash_buf.datain.size_last += size_merge;
    }
    return tmp_err;
}

static ErrStatus hash_process_latch(hashProcessParameters* latch_buf)
{
    uint32_t i;

    /*configer paras*/
    latch_buf->config = hash_VerifyConfig;

    /*engine paras*/
    latch_buf->engine = hashParameters;

    if(0U == hashParameters.is_start){
        for(i=0U; i< 16U; i++){
            g_cypher_paras.hash_buf.DATA[i] =  SEA_HASH->HASH_DOUR[i] ;
        }
    }
    /*greg paras*/
    latch_buf->proc = g_cypher_paras.hash_buf;

    return SUCCESS;
}

static ErrStatus hash_process_recover(const hashProcessParameters* recover_buf)
{
    /*configer paras*/
    hash_VerifyConfig = recover_buf->config;

    /*engine paras*/
    hashParameters = recover_buf->engine;

    /*gloabal paras*/
    g_cypher_paras.hash_buf = recover_buf->proc;

    if(0U == hashParameters.is_start){
        hashParameters.pIv = g_cypher_paras.hash_buf.DATA;
        hashParameters.is_start = 1U;
    }

    return SUCCESS;
}

/*!
    \brief      configure  SHA1/SHA224/SHA256/SHA384/SHA512 initialization
    \param[in]  configuration: pointer to  core instance
    \param[out] none
    \retval     none
*/
ErrStatus LL_HASH_init(const hashConfigStr* configuration)
{
    ErrStatus tmp_error = SUCCESS;

    if(configuration->bitWideMode > SEA_SHA_512)
    {
        tmp_error = ERROR;
    }
    /*REG RESET*/
    clrErrorCode(0xff);
    sea_hash_init();
    /*SET FIRST BLOCK DATA*/
    if(SUCCESS != hash_parameters_init((const hashConfigStr*)configuration)){
        tmp_error = ERROR;
    }
    return tmp_error;
}

/*!
    \brief    deInitialization, close hash.
    \param[in]  configuration: pointer of hash config
    \param[out] none
    \retval     ErrStatus: SUCCESS or ERROR
*/
ErrStatus LL_HASH_deinit(void)
{
    sea_hash_deinit();

    return SUCCESS;
}

/*!
    \brief      hash SHA1/224/256/384/512 messages verify
    \param[in]  verifyMessage: pointer of messages to hash, is specified in words align
    \param[in]  size      : size of the messages in bytes
    \param[in]  is_last   : 0：begin hash calc or not finish hash calc;1: when message is last, finish this  hash calc.
    \param[out] verifyOut : pointer to then output buffer. is specified in words align.then buffer size should be larger then 16 word,when SHA512
    \retval     ErrStatus: SUCCESS or ERROR
*/
ErrStatus LL_HASH(const hashData_t* verifyMessage, uint32_t size, CYPHER_DATA_STA is_last, hashData_t *verifyOut)
{
    ErrStatus tmp_err = SUCCESS;
    #ifndef HASH_CRYPT_ITEMS_IN_BYTES
    if(0U != ((uint32_t)verifyMessage & (uint32_t)3u)){
        setErrorCode(SHA_DATA_ALIGN_FAILURE);
        tmp_err = ERROR;
    }
    if(tmp_err == SUCCESS){
    #endif

    if(SUCCESS != hash_proc((const hashData_t* )verifyMessage, size,  is_last, verifyOut)){
        setErrorCode(SHA_DATASIGN_FAILURE) ;
        tmp_err = ERROR;
    }
    #ifndef AES_CRYPT_ITEMS_IN_BYTES
    }
    #endif
    return tmp_err;
}

/*!
    \brief      latch the current process status of the hash.
    \param[in]  latch_buf   :pointer to the output buffer which latch the parameters of the hash process.
    \retval     ErrStatus: SUCCESS or ERROR
*/
ErrStatus LL_HASH_process_status_latch(hashProcessParameters* latch_buf)
{
    (void)hash_process_latch(latch_buf);
    return SUCCESS;
}

/*!
    \brief      recover the hash process status use the latch parameters before.
    \param[in]  recover_buf   : pointer to  hash latch parameters buffer before.
    \retval     ErrStatus: SUCCESS or ERROR
*/
ErrStatus LL_HASH_process_status_recover(const hashProcessParameters* recover_buf)
{
    (void)hash_process_recover(recover_buf);
    return SUCCESS;
}

#else
ErrStatus LL_HASH_init(const hashConfigStr *configuration)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_HASH_init func_ptr = (pLL_HASH_init)((uint32_t)Iap_Func->LL_HASH_init + ROM_BASE_ADDR);
    return func_ptr(&hash_params, configuration);
}

ErrStatus LL_HASH_deinit(void)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_HASH_deinit func_ptr = (pLL_HASH_deinit)((uint32_t)Iap_Func->LL_HASH_deinit + ROM_BASE_ADDR);
    return func_ptr();
}

ErrStatus LL_HASH(const hashData_t *verifyMessage, uint32_t size, CYPHER_DATA_STA is_last, hashData_t *verifyOut)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_HASH func_ptr = (pLL_HASH)((uint32_t)Iap_Func->LL_HASH + ROM_BASE_ADDR);
    return func_ptr(&hash_params, verifyMessage, size, is_last, verifyOut);
}

ErrStatus LL_HASH_process_status_latch(hashProcessParameters *latch_buf)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_HASH_process_status_latch func_ptr = (pLL_HASH_process_status_latch)((uint32_t)Iap_Func->LL_HASH_process_status_latch + ROM_BASE_ADDR);
    return func_ptr(&hash_params, latch_buf);
}

ErrStatus LL_HASH_process_status_recover(const hashProcessParameters *recover_buf)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_HASH_process_status_recover func_ptr = (pLL_HASH_process_status_recover)((uint32_t)Iap_Func->LL_HASH_process_status_recover + ROM_BASE_ADDR);
    return func_ptr(recover_buf, &hash_params);
}
#endif
/* r2893 */
