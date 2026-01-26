
#include "sea_test.h"

static uint32_t aes_ecb_testunit(SEA_AES_ECB_TESTLIST *testList, uint32_t buff_size, AES_ENCRYPT_SEL encrypt_sel);

#define AES_ECB_CRYPT_TEST_ENABLE
#define AES_ECB_DECRYPT_TEST_ENABLE

const uint8_t aes_ecb_pt0[] = {"F34481EC3CC627BACD5DC3FB08F273E6"};
const uint8_t aes_ecb_ky0[] = {"00000000000000000000000000000000"};
const uint8_t aes_ecb_ct0[] = {"0336763E966D92595A567CC9CE537F5E"};

const uint8_t aes_ecb_pt1[] = {"9CB07E739C68AB7B13CB73982528AF25BF6EEDB9D94CA2BD3C183889107C754DC324F0590D871EC0E6E7EF9F2B8B5D82"};
const uint8_t aes_ecb_ky1[] = {"3EC87BD0DD9568A9DA2BFFC8A9E8C8AFD32C0EA3FD6E4E454940202BCB0373E5"};
const uint8_t aes_ecb_ct1[] = {"A2FE4674251244F5435DABD08F2D540E3CA71D3AF29CD8F0EEFBE8F26834F0DF56107B59A3F1B11D5C5A9A46C235A70A"};

const uint8_t aes_ecb_pt2[] = {"00000000000000000000000000000000"};
const uint8_t aes_ecb_ky2[] = {"E9F065D7C13573587F7875357DFBB16C53489F6A4BD0F7CD"};
const uint8_t aes_ecb_ct2[] = {"0956259C9CD5CFD0181CCA53380CDE06"};

const SEA_AES_ECB_TESTLIST aes_ecb_test0 = {
    (uint32_t *)aes_ecb_ky0, (uint32_t)(sizeof(aes_ecb_ky0) >> 1), (uint32_t *)aes_ecb_pt0, (uint32_t)(sizeof(aes_ecb_pt0) >> 1), (uint32_t *)aes_ecb_ct0, AES_128, AES_ECB};
const SEA_AES_ECB_TESTLIST aes_ecb_test1 = {
    (uint32_t *)aes_ecb_ky1, (uint32_t)(sizeof(aes_ecb_ky1) >> 1), (uint32_t *)aes_ecb_pt1, (uint32_t)(sizeof(aes_ecb_pt1) >> 1), (uint32_t *)aes_ecb_ct1, AES_256, AES_ECB};
const SEA_AES_ECB_TESTLIST aes_ecb_test2 = {
    (uint32_t *)aes_ecb_ky2, (uint32_t)(sizeof(aes_ecb_ky2) >> 1), (uint32_t *)aes_ecb_pt2, (uint32_t)(sizeof(aes_ecb_pt2) >> 1), (uint32_t *)aes_ecb_ct2, AES_192, AES_ECB};

const SEA_AES_ECB_TESTLIST *ecb_testlists[] = {(SEA_AES_ECB_TESTLIST *)&aes_ecb_test0, (SEA_AES_ECB_TESTLIST *)&aes_ecb_test1, (SEA_AES_ECB_TESTLIST *)&aes_ecb_test2};

#define AES_ECB_TESTTIMES 3

static uint32_t aes_ecb_testunit(SEA_AES_ECB_TESTLIST *testList, uint32_t buff_size, AES_ENCRYPT_SEL encrypt_sel) {
    uint32_t i;
    uint32_t plainData[256] = {0};
    uint32_t cryptData[256] = {0};
    uint32_t verifyData[256] = {0};
    uint32_t key[32] = {0};

    uint32_t loops, calc_data_size, ret_data_size, tmp_err = SEA_SUCCESS;
    aesProcessParameters pParas;
    SEA_AES_ECB_TESTLIST *pCase = testList;
#ifdef AES_PROCESS_LATCH_RECOVER_SUPPORT
    aesProcessParameters tmp_latch_buf;
#endif

    if (buff_size > sizeof(plainData)) {
        printf("[log]++stack overflow Fail!!!\n");
        return SEA_ERROR;
    }
    str2hex((const uint8_t *)pCase->pt, (uint8_t *)plainData, pCase->ptlen);
    str2hex((const uint8_t *)pCase->key, (uint8_t *)key, pCase->keylen);
    str2hex((const uint8_t *)pCase->ct, (uint8_t *)cryptData, pCase->ptlen);

    AES_InitTypeDef cfg;
    cfg.encrypt_sel = encrypt_sel;
    cfg.bitWideMode = pCase->bitWideMode;
    cfg.cryptMode = pCase->cryptMode;
    cfg.keyLocation = (aesKey_t *)&key[0];
#ifndef SEA_HAL_FULL_TEST
    cfg.assignedAddress = (uint32_t)&AES_GCM_DMA_TEST[0];
#else
    cfg.runningMode = AES_GENERAL_MODE_EN;
#endif

    tmp_err = LL_AES_crypt_init(&pParas, (const AES_InitTypeDef *)&cfg);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

    ret_data_size = pCase->ptlen;
    loops = ((ret_data_size % buff_size) != 0U) ? ((ret_data_size / buff_size) + 1U) : (ret_data_size / buff_size);
    for (i = 0; i < loops; i++) {
#ifdef AES_PROCESS_LATCH_RECOVER_SUPPORT
        if ((i == 0) || (i == 3)) {
            printf("[log]++aes_latch recover begin\n");
            LL_AES_process_status_latch(&pParas, &tmp_latch_buf);
            aes_128_gcm_demo_init(&pParas, AES_enCrypt);
            aes_128_gcm_demo_crypt_sign(&pParas);
            LL_AES_process_status_recover(&tmp_latch_buf, &pParas);
            printf("[log]++aes_latch recover finish\n");
        } else if (i == 1) {
            printf("[log]++aes_latch recover begin\n");
            LL_AES_process_status_latch(&pParas, &tmp_latch_buf);
            aes_128_gcm_demo_init(&pParas, AES_deCrypt);
            aes_128_gcm_demo_decrypt_verify(&pParas);
            LL_AES_process_status_recover(&tmp_latch_buf, &pParas);
            printf("[log]++aes_latch recover finish\n");
        }
#endif
        calc_data_size = ((ret_data_size > buff_size) ? buff_size : ret_data_size);
        ret_data_size -= calc_data_size;
        if (encrypt_sel == AES_enCrypt) {
            tmp_err = LL_AES_crypt_sign(&pParas, \
                (const PlainData_t *)((uint32_t)plainData + (i * buff_size)), \
                calc_data_size, CypherDatasNotTheLast, (CryptData_t *)((uint32_t)verifyData + (i * buff_size)));
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        } else {
            tmp_err = LL_AES_decrypt_verify(&pParas, \
                (const CryptData_t *)((uint32_t)cryptData + (i * buff_size)), \
                calc_data_size, CypherDatasNotTheLast, (PlainData_t *)((uint32_t)verifyData + (i * buff_size)));
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        }
    }
    (void)LL_AES_crypt_deinit();

    if (encrypt_sel == AES_enCrypt) {
        tmp_err = memcmp((const uint8_t *)cryptData, (const uint8_t *)verifyData, pCase->ptlen);
        if (0 != tmp_err) {
            printf("[log]++cryptData\tverifyData\n");
            for (int j = 0; j < (pCase->ptlen >> 2U); j++) {
                printf("\t%08X %08X\n", cryptData[j], verifyData[j]);
            }
        }
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    } else {
        tmp_err = memcmp((const uint8_t *)plainData, (const uint8_t *)verifyData, pCase->ptlen);
        if (0 != tmp_err) {
            printf("[log]++plainData\tverifyData\n");
            for (int j = 0; j < (pCase->ptlen >> 2U); j++) {
                printf("\t%08X %08X\n", plainData[j], verifyData[j]);
            }
        }
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    }

    return tmp_err;
}

void aes_ecb_test(void) {
    uint32_t i, j, buff_size;
    AES_KernelTypeDef version = ((sea_update_stat()) ? AES_RUNNING_IN_CORE0 : AES_RUNNING_IN_CORE1);
    uint32_t tmp_err = SEA_SUCCESS;

#ifdef AES_ECB_CRYPT_TEST_ENABLE
    printf("</***********AES ECB encrypt test start***********\n");
    sea_init_all(version);
    for (i = 0U; i < AES_ECB_TESTTIMES; i++) {
        for (j = 0U; j < 4U; j++) {
            buff_size = (16U << j); /* 通过开辟不同大小的内存，测试函数接口的单次和多次调用结果 */
            printf("[log]++list: %d max_buffer_size: %d\n", i, buff_size);
            tmp_err = aes_ecb_testunit((SEA_AES_ECB_TESTLIST *)ecb_testlists[i], buff_size, AES_enCrypt);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
            printf("------[log]AES ECB encrypt test %d Pass------\n", i);
        }
    }
    printf("***********AES ECB encrypt test Pass***********/>\n");
#endif

#ifdef AES_ECB_DECRYPT_TEST_ENABLE
    printf("</***********AES ECB decrypt test start***********\n");
    sea_init_all(version);
    for (i = 0U; i < AES_ECB_TESTTIMES; i++) {
        for (j = 0U; j < 4U; j++) {
            buff_size = (16U << j);
            printf("[log]++list: %d max_buffer_size: %d\n", i, buff_size);
            tmp_err = aes_ecb_testunit((SEA_AES_ECB_TESTLIST *)ecb_testlists[i], buff_size, AES_deCrypt);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
            printf("------[log]AES ECB decrypt test %d Pass------\n", i);
        }
    }
    printf("***********AES ECB decrypt test Pass***********/>\n");
#endif
}
