#include "sea_test.h"

typedef struct {
    aesKey_t *key;      // 指针指向秘钥
    aesIV_t *iv;        // 指针指向初始向量
    uint32_t ivSize; // 初始向量长度。单位规定为bytes
    aesTag_t *tag;      // 指针指向TAG。
                     // 1）加密时候用于结果比对；2)解密时候，作为TAG数据输入,认证比对和结果比对。
    uint32_t tagSize;
    AES_BITWIDE_MODE bitWideMode; // AES_128/AES_192/AES_256
} SEA_AES_GMAC_ADD;

/*
 - 标准字符串输入数据(文本数据，单纯参考使用)
"key": "1F19503EDC1C78EECBDA3906ECBC9C9F"
"pt" : ""
"aad": "F74A0CBE95B6B003059727283D57E2E556BB9BF12C96D4B7C8F99E509FFD81A6967511"
"iv" : "A08559C3386F154AA7B5F43B96E8F5E24A51CB6F09EB911758EF"
- 标准字符串输出数据(文本数据)
"ct":""
"tag": "8D636459677E544F2E78EF8FF5"
*/
#ifdef AES_CRYPT_ITEMS_IN_BYTES
uint8_t aes_gmac_aad_test0[] = {0xF7, 0x4A, 0x0C, 0xBE, 0x95, 0xB6, 0xB0, 0x03, 0x05, 0x97, 0x27, 0x28, 0x3D, 0x57, 0xE2, 0xE5, 0x56, 0xBB,
                                0x9B, 0xF1, 0x2C, 0x96, 0xD4, 0xB7, 0xC8, 0xF9, 0x9E, 0x50, 0x9F, 0xFD, 0x81, 0xA6, 0x96, 0x75, 0x11};
uint8_t aes_gmac_iv_test0[] = {0xA0, 0x85, 0x59, 0xC3, 0x38, 0x6F, 0x15, 0x4A, 0xA7, 0xB5, 0xF4, 0x3B, 0x96,
                               0xE8, 0xF5, 0xE2, 0x4A, 0x51, 0xCB, 0x6F, 0x09, 0xEB, 0x91, 0x17, 0x58, 0xEF};
#else
#if defined(__CC_ARM)
__align(4) uint8_t aes_gmac_aad_test0[] = {0xF7, 0x4A, 0x0C, 0xBE, 0x95, 0xB6, 0xB0, 0x03, 0x05, 0x97, 0x27, 0x28, 0x3D, 0x57, 0xE2, 0xE5, 0x56, 0xBB,
                                           0x9B, 0xF1, 0x2C, 0x96, 0xD4, 0xB7, 0xC8, 0xF9, 0x9E, 0x50, 0x9F, 0xFD, 0x81, 0xA6, 0x96, 0x75, 0x11};
__align(4) uint8_t aes_gmac_iv_test0[] = {0xA0, 0x85, 0x59, 0xC3, 0x38, 0x6F, 0x15, 0x4A, 0xA7, 0xB5, 0xF4, 0x3B, 0x96,
                                          0xE8, 0xF5, 0xE2, 0x4A, 0x51, 0xCB, 0x6F, 0x09, 0xEB, 0x91, 0x17, 0x58, 0xEF};
#else
#pragma pack(4)
uint8_t aes_gmac_aad_test0[] = {0xF7, 0x4A, 0x0C, 0xBE, 0x95, 0xB6, 0xB0, 0x03, 0x05, 0x97, 0x27, 0x28, 0x3D, 0x57, 0xE2, 0xE5, 0x56, 0xBB,
                                0x9B, 0xF1, 0x2C, 0x96, 0xD4, 0xB7, 0xC8, 0xF9, 0x9E, 0x50, 0x9F, 0xFD, 0x81, 0xA6, 0x96, 0x75, 0x11};
uint8_t aes_gmac_iv_test0[] = {0xA0, 0x85, 0x59, 0xC3, 0x38, 0x6F, 0x15, 0x4A, 0xA7, 0xB5, 0xF4, 0x3B, 0x96,
                               0xE8, 0xF5, 0xE2, 0x4A, 0x51, 0xCB, 0x6F, 0x09, 0xEB, 0x91, 0x17, 0x58, 0xEF};
#pragma pack()
#endif
#endif

uint8_t aes_gmac_key_test0[] = {0x1F, 0x19, 0x50, 0x3E, 0xDC, 0x1C, 0x78, 0xEE, 0xCB, 0xDA, 0x39, 0x06, 0xEC, 0xBC, 0x9C, 0x9F};
uint8_t aes_gmac_tag_test0[] = {0x8D, 0x63, 0x64, 0x59, 0x67, 0x7E, 0x54, 0x4F, 0x2E, 0x78, 0xEF, 0x8F, 0xF5};

const SEA_AES_GMAC_ADD aes_gmac_demo_case_0 = {
    (aesKey_t *)aes_gmac_key_test0, // uint32_t *key;
    (aesIV_t *)aes_gmac_iv_test0,   // uint32_t *iv;
    sizeof(aes_gmac_iv_test0),   // uint32_t  ivSize;
    (aesTag_t *)aes_gmac_tag_test0, // uint32_t *tag;
    sizeof(aes_gmac_tag_test0),  // uint32_t  tagSize;
    AES_128                      // AES_BITWIDE_MODE bitWideMode;
};

uint8_t gmac_demo_tag_sgin[16];         // 签名信息
uint8_t gmac_demo_tag_verify[16];       // 解密签名认证信息
AES_VERIFY_STA gmac_demo_verify_status; // 解密认证结果标志

// 初始化
uint32_t aes_gmac_demo_init(aesProcessParameters *pParas, AES_ENCRYPT_SEL encrypt_sel, const SEA_AES_GMAC_ADD *p_testcase) {
    uint32_t tmp_err = SEA_SUCCESS;
    AES_InitTypeDef cfg;

    // 1 配置参数初始化
    cfg.encrypt_sel = encrypt_sel;
    cfg.bitWideMode = p_testcase->bitWideMode;
    cfg.cryptMode = AES_GCM;
    cfg.keyLocation = (aesKey_t *)p_testcase->key;
    cfg.ivLocation = (aesIV_t *)p_testcase->iv;
    cfg.ivSize = p_testcase->ivSize;
    cfg.aadLocation = (aesAAD_t *)0;
    cfg.aadSize = 0;
    cfg.tagSignBuf = (aesTag_t *)gmac_demo_tag_sgin;
    cfg.tagVerifyBuf = (aesTag_t *)gmac_demo_tag_verify;
    cfg.tagSize = p_testcase->tagSize;
    cfg.tagVerifySta = &gmac_demo_verify_status;
    cfg.specialMode = AES_SPC_NONE;
#ifndef SEA_HAL_FULL_TEST
    cfg.assignedAddress = (uint32_t)&AES_GCM_DMA_TEST[0];
#else
    cfg.runningMode = AES_GENERAL_MODE_EN;
#endif
    if (AES_deCrypt == encrypt_sel) {
        memcpy(cfg.tagSignBuf, p_testcase->tag, cfg.tagSize);
    }

    // 2 初始化
    tmp_err = LL_AES_crypt_init(pParas, (const AES_InitTypeDef *)&cfg);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    return tmp_err;
}

// 分多次完成完整数据的签名
uint32_t aes_gmac_demo_sign_mulit_times(aesProcessParameters *pParas, uint8_t ghashData[], uint32_t size, const SEA_AES_GMAC_ADD *p_testcase) {
    uint32_t tmp_err = SEA_SUCCESS;
    // 1 先完成一部分数据加密，16字节的倍数
    tmp_err = LL_AES_gmac(pParas, (const PlainData_t *)(ghashData), 16, CypherDatasNotTheLast);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    // 2 完成剩余数据的加密和签名
    tmp_err = LL_AES_gmac(pParas, (const PlainData_t *)(&ghashData[16]), size - 16, CypherDatasIsTheLast);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    // 3 比对加密结果-tag
    tmp_err = memcmp(p_testcase->tag, (const uint8_t *)gmac_demo_tag_sgin, p_testcase->tagSize);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    return tmp_err;
}

// 分多次完成完整数据认证
uint32_t aes_gmac_demo_verify_multi_times(aesProcessParameters *pParas, uint8_t ghashData[], uint32_t size, const SEA_AES_GMAC_ADD *p_testcase) {
    uint32_t tmp_err = SEA_SUCCESS;
    // 1 先完成一部分数据加密，16字节的倍数
    tmp_err = LL_AES_gmac(pParas, (const PlainData_t *)(ghashData), 16, CypherDatasNotTheLast);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    // 2 完成剩余数据的加密和签名
    tmp_err = LL_AES_gmac(pParas, (const PlainData_t *)(&ghashData[16]), size - 16, CypherDatasIsTheLast);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    // 3 比对加密结果-tag
    tmp_err = memcmp(p_testcase->tag, (const uint8_t *)gmac_demo_tag_verify, p_testcase->tagSize);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, gmac_demo_verify_status);
    return tmp_err;
}

void aes_gmac_demo_test(void) {
    WDT_FEED;
    printf("</***********aes_gmac_demo_test begin***********\n");
    aesProcessParameters pParas;
    aes_gmac_demo_init(&pParas, AES_enCrypt, &aes_gmac_demo_case_0);
    // 分多次完成完整数据的签名
    aes_gmac_demo_sign_mulit_times(&pParas, aes_gmac_aad_test0, sizeof(aes_gmac_aad_test0), &aes_gmac_demo_case_0);

    aes_gmac_demo_init(&pParas, AES_deCrypt, &aes_gmac_demo_case_0);
    // 分多次完成完整数据认证
    aes_gmac_demo_verify_multi_times(&pParas, aes_gmac_aad_test0, sizeof(aes_gmac_aad_test0), &aes_gmac_demo_case_0);
    printf("***********aes_gmac_demo_test finish***********/>\n");
}
