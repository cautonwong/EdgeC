#include "sea_test.h"

/*
 - 标准字符串输入数据(文本数据，单纯参考使用)
"key": "feffe9928665731c6d6a8f9467308308"
"pt" :
"d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39"
"aad": "feedfacedeadbeeffeedfacedeadbeefabaddad2"
"iv" : "cafebabefacedbaddecaf888"
- 标准字符串输出数据(文本数据)
"ct":"42831ec2217774244b7221b784d0d49ce3aa212f2c02a4e035c17e2329aca12e21d514b25466931c7d8f6a5aac84aa051ba30b396a0aac973d58e091"
"tag": "5bc94fbc3221a5db94fae95ae7121a47"
*/
#ifdef AES_CRYPT_ITEMS_IN_BYTES
uint8_t aes_128_gcm_pt_test0[] = {0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5, 0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a, 0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda, 0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72, 0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53, 0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25, 0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57, 0xba, 0x63, 0x7b, 0x39};
uint8_t aes_128_gcm_aad_test0[] = {0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef, 0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef, 0xab, 0xad, 0xda, 0xd2};
uint8_t aes_128_gcm_ct_test0[] = {0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24, 0x4b, 0x72, 0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c, 0xe3, 0xaa, 0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0, 0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e, 0x21, 0xd5, 0x14, 0xb2, 0x54, 0x66, 0x93, 0x1c, 0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05, 0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a, 0xac, 0x97, 0x3d, 0x58, 0xe0, 0x91};
uint8_t aes_128_gcm_iv_test0[] ={0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88};
#else
#if defined(__CC_ARM)
__align(4) uint8_t aes_128_gcm_pt_test0[] ={0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5, 0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a, 0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda, 0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72, 0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53, 0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25, 0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57, 0xba, 0x63, 0x7b, 0x39};
__align(4) uint8_t aes_128_gcm_aad_test0[] ={0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef, 0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef, 0xab, 0xad, 0xda, 0xd2};
__align(4) uint8_t aes_128_gcm_ct_test0[] ={0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24, 0x4b, 0x72, 0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c, 0xe3, 0xaa, 0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0, 0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e, 0x21, 0xd5, 0x14, 0xb2, 0x54, 0x66, 0x93, 0x1c, 0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05, 0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a, 0xac, 0x97, 0x3d, 0x58, 0xe0, 0x91};
__align(4) uint8_t aes_128_gcm_iv_test0[] ={0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88};
#else /*defined ( __ICCARM__ ) */
#pragma pack(4)
uint8_t aes_128_gcm_pt_test0[]  ={0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5, 0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a, 0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda, 0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72, 0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53, 0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25, 0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57, 0xba, 0x63, 0x7b, 0x39};
uint8_t aes_128_gcm_aad_test0[] ={0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef, 0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef, 0xab, 0xad, 0xda, 0xd2};
uint8_t aes_128_gcm_ct_test0[]  ={0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24, 0x4b, 0x72, 0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c, 0xe3, 0xaa, 0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0, 0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e, 0x21, 0xd5, 0x14, 0xb2, 0x54, 0x66, 0x93, 0x1c, 0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05, 0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a, 0xac, 0x97, 0x3d, 0x58, 0xe0, 0x91};
uint8_t aes_128_gcm_iv_test0[]  ={0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88};
#pragma pack()
#endif
#endif
uint8_t aes_128_gcm_key_test0[] ={0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c, 0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x8};
uint8_t aes_128_gcm_tag_test0[] ={0x5b, 0xc9, 0x4f, 0xbc, 0x32, 0x21, 0xa5, 0xdb, 0x94, 0xfa, 0xe9, 0x5a, 0xe7, 0x12, 0x1a, 0x47};

uint8_t tag_sign[16];             // 签名信息
uint8_t tag_verify[16];           // 解密签名认证信息
uint32_t crpyt_decrypt_data[128]; // 加解密输出缓存
AES_VERIFY_STA verify_status;     // 解密认证结果标志

// 初始化
void aes_128_gcm_demo_init(aesProcessParameters *pParas, AES_ENCRYPT_SEL encrypt_sel) {
    AES_InitTypeDef cfg;

    // 1 配置参数初始化
    cfg.encrypt_sel = encrypt_sel;
    cfg.bitWideMode = AES_128;
    cfg.cryptMode = AES_GCM;
    cfg.keyLocation = (aesKey_t *)aes_128_gcm_key_test0;
    cfg.ivLocation = (aesIV_t *)aes_128_gcm_iv_test0;
    cfg.ivSize = sizeof(aes_128_gcm_iv_test0);
    cfg.aadLocation = (aesAAD_t *)aes_128_gcm_aad_test0;
    cfg.aadSize = sizeof(aes_128_gcm_aad_test0);
    cfg.tagSignBuf = (aesTag_t *)tag_sign;
    cfg.tagVerifyBuf = (aesTag_t *)tag_verify;
    cfg.tagSize = sizeof(aes_128_gcm_tag_test0);
    cfg.tagVerifySta = &verify_status;
    cfg.specialMode = AES_SPC_NONE;
#ifndef SEA_HAL_FULL_TEST
    cfg.assignedAddress = (uint32_t)&AES_GCM_DMA_TEST[0];
#else
    cfg.runningMode = AES_GENERAL_MODE_EN;
#endif
    if (AES_deCrypt == encrypt_sel) {
        memcpy(cfg.tagSignBuf, aes_128_gcm_tag_test0, cfg.tagSize);
    }

    // 2 初始化
    if (SEA_SUCCESS != LL_AES_crypt_init(pParas, (const AES_InitTypeDef *)&cfg)) {
        printf("aes_crypt_init return Fail!!!\n");
        return;
    };
}

// 单次完成完整数据的加密和签名
void aes_128_gcm_demo_crypt_sign(aesProcessParameters *pParas) {
    // 1 加密和签名
    if (SEA_SUCCESS !=
        LL_AES_crypt_sign(pParas, (const PlainData_t *)(aes_128_gcm_pt_test0), sizeof(aes_128_gcm_pt_test0), CypherDatasIsTheLast, (CryptData_t *)(crpyt_decrypt_data))) {
        printf("aes_crypt_sign return Fail!!!\n");
        return;
    };
    // 2 比对加密结果-密文
    if (0 == memcmp((const uint8_t *)aes_128_gcm_ct_test0, (const uint8_t *)crpyt_decrypt_data, sizeof(aes_128_gcm_pt_test0))) {
        printf("crypt ct compare Pass\n");
    } else {
        printf("crypt ct compare Fail!!!\n");
        printf("[log]++cryptData   :\t");
        for (int j = 0; j < sizeof(aes_128_gcm_pt_test0); j++) {
            printf("%02X ", aes_128_gcm_ct_test0[j]);
        }
        printf("\n[log]++verifyData:\t");
        for (int j = 0; j < sizeof(aes_128_gcm_pt_test0); j++) {
            printf("%02X ", *((uint8_t *)crpyt_decrypt_data + j));
        }
        printf("\n");
    }
    // 3 比对加密结果-tag
    if (0 == memcmp((const uint8_t *)aes_128_gcm_tag_test0, (const uint8_t *)tag_sign, sizeof(aes_128_gcm_tag_test0))) {
        printf("sign tag compare Pass\n");
    } else {
        printf("sign tag compare Fail!!!\n");
        printf("[log]++crypttag   :\t");
        for (int j = 0; j < sizeof(aes_128_gcm_tag_test0); j++) {
            printf("%02X ", aes_128_gcm_tag_test0[j]);
        }
        printf("\n[log]++verifytag:\t");
        for (int j = 0; j < sizeof(aes_128_gcm_tag_test0); j++) {
            printf("%02X ", *((uint8_t *)tag_sign + j));
        }
        printf("\n");
    }
}

// 单次完成完整数据的解密和认证
void aes_128_gcm_demo_decrypt_verify(aesProcessParameters *pParas) {
    // 1 加密和签名
    if (SEA_SUCCESS !=
        LL_AES_decrypt_verify(pParas, (const CryptData_t *)(aes_128_gcm_ct_test0), sizeof(aes_128_gcm_pt_test0), CypherDatasIsTheLast, (PlainData_t *)(crpyt_decrypt_data))) {
        printf("aes_decrypt_verify return Fail!!!\n");
        return;
    };
    // 2 比对解密结果-明文
    if (0 == memcmp((const uint8_t *)aes_128_gcm_pt_test0, (const uint8_t *)crpyt_decrypt_data, sizeof(aes_128_gcm_ct_test0))) {
        printf("decrypt pt compare Pass\n");
    } else {
        printf("decrypt pt compare Fail!!!\n");
        printf("[log]++cryptData   :\t");
        for (int j = 0; j < sizeof(aes_128_gcm_pt_test0); j++) {
            printf("%02X ", aes_128_gcm_pt_test0[j]);
        }
        printf("\n[log]++verifyData:\t");
        for (int j = 0; j < sizeof(aes_128_gcm_pt_test0); j++) {
            printf("%02X ", *((uint8_t *)crpyt_decrypt_data + j));
        }
        printf("\n");
    }
    // 3 比对解密结果-tag认证
    if (0 == memcmp((const uint8_t *)aes_128_gcm_tag_test0, (const uint8_t *)tag_verify, sizeof(aes_128_gcm_tag_test0))) {
        printf("sign tag compare Pass\n");
        /*状态标志位测试*/
        if (verify_status != AES_VERIFY_SUCCESS) {
            printf("sign tag flag Fail!!!\n");
        } else {
            printf("sign tag flag Pass\n");
        }
    } else {
        printf("sign tag compare Fail!!!\n");
        printf("[log]++crypttag   :\t");
        for (int j = 0; j < sizeof(aes_128_gcm_tag_test0); j++) {
            printf("%02X ", aes_128_gcm_tag_test0[j]);
        }
        printf("\n[log]++verifytag:\t");
        for (int j = 0; j < sizeof(aes_128_gcm_tag_test0); j++) {
            printf("%02X ", *((uint8_t *)tag_verify + j));
        }
        printf("\n");
        /*状态标志位*/
        if (verify_status != AES_VERIFY_FAIL) {
            printf("sign tag flag Fail!!!\n");
        } else {
            printf("sign tag flag Pass\n");
        }
    }
}

// 分多次完成完整数据的加密和签名
void aes_128_gcm_demo_crypt_sign_mulit_times(aesProcessParameters *pParas) {
    if (sizeof(aes_128_gcm_pt_test0) < 16) {
        return;
    }

    // 1 先完成一部分数据加密，16字节的倍数
    if (SEA_SUCCESS != LL_AES_crypt_sign(pParas, (const PlainData_t *)(aes_128_gcm_pt_test0), 16, CypherDatasNotTheLast, (CryptData_t *)(crpyt_decrypt_data))) {
        printf("aes_crypt_sign first return Fail!!!\n");
        return;
    };
    // 2 完成剩余数据的加密和签名
    if (SEA_SUCCESS != LL_AES_crypt_sign(pParas, (const PlainData_t *)(&aes_128_gcm_pt_test0[16]), sizeof(aes_128_gcm_pt_test0) - 16, CypherDatasIsTheLast,
                                         (CryptData_t *)(&crpyt_decrypt_data[16]))) {
        printf("aes_crypt_sign final return Fail!!!\n");
        return;
    };

    // 3 比对加密结果-密文
    if (0 == memcmp((const uint8_t *)aes_128_gcm_ct_test0, (const uint8_t *)crpyt_decrypt_data, sizeof(aes_128_gcm_pt_test0))) {
        printf("crypt ct compare Pass\n");
    } else {
        printf("crypt ct compare Fail!!!\n");
    }
    // 4 比对加密结果-tag
    if (0 == memcmp((const uint8_t *)aes_128_gcm_tag_test0, (const uint8_t *)tag_sign, sizeof(aes_128_gcm_tag_test0))) {
        printf("sign tag compare Pass\n");
    } else {
        printf("sign tag compare Fail!!!\n");
    }
}

// 分多次完成完整数据解密和认证
void aes_128_gcm_demo_decrypt_verify_multi_times(aesProcessParameters *pParas) {
    if (sizeof(aes_128_gcm_pt_test0) < 16) {
        return;
    }

    // 1 先完成一部分解密
    if (SEA_SUCCESS != LL_AES_decrypt_verify(pParas, (const CryptData_t *)(aes_128_gcm_ct_test0), 16, CypherDatasNotTheLast, (PlainData_t *)(crpyt_decrypt_data))) {
        printf("aes_decrypt_verify first return Fail!!!\n");
        return;
    };
    // 2 完成剩余数据的解密和认证
    if (SEA_SUCCESS != LL_AES_decrypt_verify(pParas, (const CryptData_t *)(&aes_128_gcm_ct_test0[16]), sizeof(aes_128_gcm_pt_test0) - 16, CypherDatasIsTheLast,
                                             (PlainData_t *)(&crpyt_decrypt_data[16]))) {
        printf("aes_decrypt_verify final return Fail!!!\n");
        return;
    };
    // 2 比对解密结果-明文
    if (0 == memcmp((const uint8_t *)aes_128_gcm_pt_test0, (const uint8_t *)crpyt_decrypt_data, sizeof(aes_128_gcm_ct_test0))) {
        printf("decrypt pt compare Pass\n");
    } else {
        printf("decrypt pt compare Fail!!!\n");
    }
    // 3 比tag认证结果
    if (0 == memcmp((const uint8_t *)aes_128_gcm_tag_test0, (const uint8_t *)tag_verify, sizeof(aes_128_gcm_tag_test0))) {
        printf("sign tag compare Pass\n");
        /*状态标志位测试*/
        if (verify_status != AES_VERIFY_SUCCESS) {
            printf("sign tag flag Fail!!!\n");
        } else {
            printf("sign tag flag Pass\n");
        }
    } else {
        printf("sign tag compare Fail!!!\n");
        /*状态标志位*/
        if (verify_status != AES_VERIFY_FAIL) {
            printf("sign tag flag Fail!!!\n");
        } else {
            printf("sign tag flag Pass\n");
        }
    }
}

void aes_128_gcm_demo_test(void) {
    WDT_FEED;
    printf("</***********aes_128_gcm_demo_test begin***********\n");
    aesProcessParameters pParas;

    /*解密场景1 单次初始化，单次解密*/
    aes_128_gcm_demo_init(&pParas, AES_deCrypt);
    aes_128_gcm_demo_decrypt_verify(&pParas);

    /*加密场景1 单次初始化，单次加密*/
    aes_128_gcm_demo_init(&pParas, AES_enCrypt);
    aes_128_gcm_demo_crypt_sign(&pParas);

    /*一组数据，分多次加密签名*/
    aes_128_gcm_demo_init(&pParas, AES_enCrypt);
    aes_128_gcm_demo_crypt_sign_mulit_times(&pParas);

    /*加密场景2 单次初始化，多次加密*/
    aes_128_gcm_demo_init(&pParas, AES_enCrypt);
    aes_128_gcm_demo_crypt_sign(&pParas);
    /*只变化加密数据，其他配置不变，可以连续进行加密*/
    aes_128_gcm_demo_crypt_sign(&pParas);

    /*一次初始化化，多次解密*/
    aes_128_gcm_demo_init(&pParas, AES_deCrypt);
    aes_128_gcm_demo_decrypt_verify(&pParas);
    aes_128_gcm_demo_decrypt_verify(&pParas);

    /*一组数据，分多次解密认证*/
    aes_128_gcm_demo_init(&pParas, AES_deCrypt);
    aes_128_gcm_demo_decrypt_verify_multi_times(&pParas);

    printf("***********aes_128_gcm_demo_test finish***********/>\n\n\n");
}
