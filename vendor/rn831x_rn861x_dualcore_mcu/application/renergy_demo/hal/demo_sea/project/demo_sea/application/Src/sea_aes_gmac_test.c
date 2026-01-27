#include "sea_test.h"

#define AES_GMAC_ADD_enCrypt_TEST_ENABLE
#define AES_GMAC_ADD_deCrypt_TEST_ENABLE
#define AES_GMAC_CRYPTDATA_deCrypt_TEST_ENABLE

static uint32_t aes_gmac_testunit(SEA_AES_GCM_TESTLIST *testcase, uint32_t buff_size, AES_ENCRYPT_SEL encrypt_sel, AES_SPECIAL_MOD spc_mod);

extern const SEA_AES_GCM_TESTLIST *gcm_testlist[];

const uint8_t aes_gmac_ky0[] = {"A0D83382D913895F32AFBBBFB94CA91D"};
const uint8_t aes_gmac_pt0[] = {""};
const uint8_t aes_gmac_ad0[] = {"F4594739A94AEA9E3BBA8FEDBE04DBBE5196FABF882AA0A6A16AC8E6EFC72951E229AB7FEB"
                                "4830C99EC1F7F52BD2FA47CE5FE38F"};
const uint8_t aes_gmac_iv0[] = {"5D9418"};
const uint8_t aes_gmac_ct0[] = {""};
const uint8_t aes_gmac_tg0[] = {"52FE67F75AE6BD66C73B720AC1680B"};

const uint8_t aes_gmac_ky1[] = {"1F19503EDC1C78EECBDA3906ECBC9C9F"};
const uint8_t aes_gmac_pt1[] = {""};
const uint8_t aes_gmac_ad1[] = {"F74A0CBE95B6B003059727283D57E2E556BB9BF12C96D4B7C8F99E509FFD81A6967511"};
const uint8_t aes_gmac_iv1[] = {"A08559C3386F154AA7B5F43B96E8F5E24A51CB6F09EB911758EF"};
const uint8_t aes_gmac_ct1[] = {""};
const uint8_t aes_gmac_tg1[] = {"8D636459677E544F2E78EF8FF5"};

const uint8_t aes_gmac_ky2[] = {"00000000000000000000000000000000"};
const uint8_t aes_gmac_pt2[] = {""};
const uint8_t aes_gmac_ad2[] = {""};
const uint8_t aes_gmac_iv2[] = {"000000000000000000000000"};
const uint8_t aes_gmac_ct2[] = {""};
const uint8_t aes_gmac_tg2[] = {"58e2fccefa7e3061367f1d57a4e7455a"};

const uint8_t aes_gmac_ky3[] = {"000000000000000000000000000000000000000000000000"};
const uint8_t aes_gmac_pt3[] = {""};
const uint8_t aes_gmac_ad3[] = {""};
const uint8_t aes_gmac_iv3[] = {"000000000000000000000000"};
const uint8_t aes_gmac_ct3[] = {""};
const uint8_t aes_gmac_tg3[] = {"cd33b28ac773f74ba00ed1f312572435"};

const uint8_t aes_gmac_ky4[] = {"0000000000000000000000000000000000000000000000000000000000000000"};
const uint8_t aes_gmac_pt4[] = {""};
const uint8_t aes_gmac_ad4[] = {""};
const uint8_t aes_gmac_iv4[] = {"000000000000000000000000"};
const uint8_t aes_gmac_ct4[] = {""};
const uint8_t aes_gmac_tg4[] = {"530f8afbc74536b9a963b4f1c4cb738b"};

const uint8_t aes_gmac_ky5[] = {"8344816E93546368F6BF3ACF4FE16C77"};
const uint8_t aes_gmac_pt5[] = {""};
const uint8_t aes_gmac_ad5[] = {"B4B82548569E4636BA14BBFC13BB7184CD77FDED1B9699BE91F38BB39B8359FFF3BAB853AC"
                                "95184A313C1A73BF610E4F508AF559AA9E7F6C0CD1DE6EE97873D5887A5E210BC20EC2D0A7"
                                "8C9B02A5906F62752B0F58E17A52DCC315BB58E9F2BDD81D194FD170C47C994D3B24F05B2F"
                                "228D3658DB354CF2A2CA2F92D03EA8100C"};
const uint8_t aes_gmac_iv5[] = {"CC7DF732C71DA6BB51615B4B"};
const uint8_t aes_gmac_ct5[] = {""};
const uint8_t aes_gmac_tg5[] = {"79EBE4D46CBD21DC4EEDE72922D8"};

const uint8_t aes_gmac_ky6[] = {"17979378F909FB32F6D6D8CF1DA347F6EE1D569E0A285515"};
const uint8_t aes_gmac_pt6[] = {""};
const uint8_t aes_gmac_ad6[] = {"8C62CB509144E7B2B457DA6D8C5036A6B43F3CF1FC8BD896F60AD0E939B84612A0CA24CE2C"
                                "9139D10AB5B95D6BBFEAC4FBF55B1566B44AB131F7E2CA16F2686DB4452D7614319B0F55A3"
                                "A5463C0A8DD9D5E6DCA225FB58CD413900C8A18E74B0FCF567F998BFABD7978111BE828CEA"
                                "31C084598BA365631C3FC536C009339067"};
const uint8_t aes_gmac_iv6[] = {"6127D3273C4E5D2F727F0789"};
const uint8_t aes_gmac_ct6[] = {""};
const uint8_t aes_gmac_tg6[] = {"D7ACFA6889C61B4C"};

const uint8_t aes_gmac_ky7[] = {"81E975A14A38B04EB4E1D2C9FF647886F3D050CB7D1A6FA49BB5147452D71376"};
const uint8_t aes_gmac_pt7[] = {""};
const uint8_t aes_gmac_ad7[] = {"0989FBF4832AD28D8332D5EAAF2D938E78F9A240178BFFBA8D4C74DA7D3C98852EFA6BDCC3"
                                "042331A3C1BD4F138BF11CA422D6BA"};
const uint8_t aes_gmac_iv7[] = {"3F92B17465BEF9A75C764E40"};
const uint8_t aes_gmac_ct7[] = {""};
const uint8_t aes_gmac_tg7[] = {"F3A82B6F0F154AB88EDDC4AE649B09"};

const SEA_AES_GCM_TESTLIST aes_gmac_0 = {(uint32_t *)aes_gmac_ky0,
                                         (uint32_t)sizeof(aes_gmac_ky0) >> 1,
                                         (uint32_t *)aes_gmac_pt0,
                                         (uint32_t)0,
                                         (uint32_t *)aes_gmac_ad0,
                                         (uint32_t)sizeof(aes_gmac_ad0) >> 1,
                                         (uint32_t *)aes_gmac_iv0,
                                         (uint32_t)sizeof(aes_gmac_iv0) >> 1,
                                         (uint32_t *)aes_gmac_ct0,
                                         (uint32_t *)aes_gmac_tg0,
                                         (uint32_t)sizeof(aes_gmac_tg0) >> 1,
                                         AES_128,
                                         AES_GCM};
const SEA_AES_GCM_TESTLIST aes_gmac_1 = {(uint32_t *)aes_gmac_ky1,
                                         (uint32_t)sizeof(aes_gmac_ky1) >> 1,
                                         (uint32_t *)aes_gmac_pt1,
                                         (uint32_t)0,
                                         (uint32_t *)aes_gmac_ad1,
                                         (uint32_t)sizeof(aes_gmac_ad1) >> 1,
                                         (uint32_t *)aes_gmac_iv1,
                                         (uint32_t)sizeof(aes_gmac_iv1) >> 1,
                                         (uint32_t *)aes_gmac_ct1,
                                         (uint32_t *)aes_gmac_tg1,
                                         (uint32_t)sizeof(aes_gmac_tg1) >> 1,
                                         AES_128,
                                         AES_GCM};
const SEA_AES_GCM_TESTLIST aes_gmac_2 = {(uint32_t *)aes_gmac_ky2,
                                         (uint32_t)sizeof(aes_gmac_ky2) >> 1,
                                         (uint32_t *)aes_gmac_pt2,
                                         (uint32_t)0,
                                         (uint32_t *)aes_gmac_ad2,
                                         (uint32_t)0,
                                         (uint32_t *)aes_gmac_iv2,
                                         (uint32_t)sizeof(aes_gmac_iv2) >> 1,
                                         (uint32_t *)aes_gmac_ct2,
                                         (uint32_t *)aes_gmac_tg2,
                                         (uint32_t)sizeof(aes_gmac_tg2) >> 1,
                                         AES_128,
                                         AES_GCM};
const SEA_AES_GCM_TESTLIST aes_gmac_3 = {(uint32_t *)aes_gmac_ky3,
                                         (uint32_t)sizeof(aes_gmac_ky3) >> 1,
                                         (uint32_t *)aes_gmac_pt3,
                                         (uint32_t)0,
                                         (uint32_t *)aes_gmac_ad3,
                                         (uint32_t)0,
                                         (uint32_t *)aes_gmac_iv3,
                                         (uint32_t)sizeof(aes_gmac_iv3) >> 1,
                                         (uint32_t *)aes_gmac_ct3,
                                         (uint32_t *)aes_gmac_tg3,
                                         (uint32_t)sizeof(aes_gmac_tg3) >> 1,
                                         AES_192,
                                         AES_GCM};
const SEA_AES_GCM_TESTLIST aes_gmac_4 = {(uint32_t *)aes_gmac_ky4,
                                         (uint32_t)sizeof(aes_gmac_ky4) >> 1,
                                         (uint32_t *)aes_gmac_pt4,
                                         (uint32_t)0,
                                         (uint32_t *)aes_gmac_ad4,
                                         (uint32_t)0,
                                         (uint32_t *)aes_gmac_iv4,
                                         (uint32_t)sizeof(aes_gmac_iv4) >> 1,
                                         (uint32_t *)aes_gmac_ct4,
                                         (uint32_t *)aes_gmac_tg4,
                                         (uint32_t)sizeof(aes_gmac_tg4) >> 1,
                                         AES_256,
                                         AES_GCM};
const SEA_AES_GCM_TESTLIST aes_gmac_5 = {(uint32_t *)aes_gmac_ky5,
                                         (uint32_t)sizeof(aes_gmac_ky5) >> 1,
                                         (uint32_t *)aes_gmac_pt5,
                                         (uint32_t)0,
                                         (uint32_t *)aes_gmac_ad5,
                                         (uint32_t)sizeof(aes_gmac_ad5) >> 1,
                                         (uint32_t *)aes_gmac_iv5,
                                         (uint32_t)sizeof(aes_gmac_iv5) >> 1,
                                         (uint32_t *)aes_gmac_ct5,
                                         (uint32_t *)aes_gmac_tg5,
                                         (uint32_t)sizeof(aes_gmac_tg5) >> 1,
                                         AES_128,
                                         AES_GCM};
const SEA_AES_GCM_TESTLIST aes_gmac_6 = {(uint32_t *)aes_gmac_ky6,
                                         (uint32_t)sizeof(aes_gmac_ky6) >> 1,
                                         (uint32_t *)aes_gmac_pt6,
                                         (uint32_t)0,
                                         (uint32_t *)aes_gmac_ad6,
                                         (uint32_t)sizeof(aes_gmac_ad6) >> 1,
                                         (uint32_t *)aes_gmac_iv6,
                                         (uint32_t)sizeof(aes_gmac_iv6) >> 1,
                                         (uint32_t *)aes_gmac_ct6,
                                         (uint32_t *)aes_gmac_tg6,
                                         (uint32_t)sizeof(aes_gmac_tg6) >> 1,
                                         AES_192,
                                         AES_GCM};
const SEA_AES_GCM_TESTLIST aes_gmac_7 = {(uint32_t *)aes_gmac_ky7,
                                         (uint32_t)sizeof(aes_gmac_ky7) >> 1,
                                         (uint32_t *)aes_gmac_pt7,
                                         (uint32_t)0,
                                         (uint32_t *)aes_gmac_ad7,
                                         (uint32_t)sizeof(aes_gmac_ad7) >> 1,
                                         (uint32_t *)aes_gmac_iv7,
                                         (uint32_t)sizeof(aes_gmac_iv7) >> 1,
                                         (uint32_t *)aes_gmac_ct7,
                                         (uint32_t *)aes_gmac_tg7,
                                         (uint32_t)sizeof(aes_gmac_tg7) >> 1,
                                         AES_256,
                                         AES_GCM};

const SEA_AES_GCM_TESTLIST *gmac_testlist[] = {(SEA_AES_GCM_TESTLIST *)&aes_gmac_0, (SEA_AES_GCM_TESTLIST *)&aes_gmac_1, (SEA_AES_GCM_TESTLIST *)&aes_gmac_2,
                                               (SEA_AES_GCM_TESTLIST *)&aes_gmac_3, (SEA_AES_GCM_TESTLIST *)&aes_gmac_4, (SEA_AES_GCM_TESTLIST *)&aes_gmac_5,
                                               (SEA_AES_GCM_TESTLIST *)&aes_gmac_6, (SEA_AES_GCM_TESTLIST *)&aes_gmac_7};

#define AES_GMAC_TESTTIMES 8

const uint32_t gmac_buffsize_testlist[] = {16, 32, 48, 64, 80, 96, 112, 128, 512, 1024, 2048, SEA_STACK_MAX_SIZE};

static uint32_t aes_gmac_testunit(SEA_AES_GCM_TESTLIST *testcase, uint32_t buff_size, AES_ENCRYPT_SEL encrypt_sel, AES_SPECIAL_MOD spc_mod) {
    uint32_t i;
#ifdef AES_CRYPT_ITEMS_IN_BYTES
    GHashData_t tmp_gHASHdata[SEA_STACK_MAX_SIZE];
    aesAAD_t tmp_add[SEA_STACK_MAX_SIZE];
#else
    GHashData_t gHASHdata[SEA_STACK_MAX_SIZE / 4];
    aesAAD_t aad[SEA_STACK_MAX_SIZE / 4];
#endif

    uint32_t key[32] = {0};
    uint32_t iv[32] = {0};
    uint32_t tagSign[4] = {0};
    uint32_t tagVerify[4] = {0};
    aesProcessParameters pParas;
    uint32_t tmp_err;
    uint32_t calc_data_size, loops, last_data_size, ret_data_size;
    SEA_AES_GCM_TESTLIST *pCase;
    AES_VERIFY_STA verStat;
    CYPHER_DATA_STA is_last;
#ifdef AES_CRYPT_ITEMS_IN_BYTES
    GHashData_t *gHASHdata = (GHashData_t *)&tmp_gHASHdata[2];
    aesAAD_t *aad = (GHashData_t *)&tmp_add[3];
#endif
#ifdef AES_PROCESS_LATCH_RECOVER_SUPPORT
    aesProcessParameters tmp_latch_buf;
#endif

    pCase = (SEA_AES_GCM_TESTLIST *)testcase;
    AES_InitTypeDef cfg;
    cfg.encrypt_sel = encrypt_sel;
    cfg.bitWideMode = pCase->bitWideMode;
    cfg.cryptMode = pCase->cryptMode;
    cfg.keyLocation = (aesKey_t *)&key[0];
    cfg.ivLocation = (aesIV_t *)&iv[0];
    cfg.ivSize = pCase->ivlen;
    cfg.aadLocation = (aesAAD_t *)&aad[0];
    cfg.aadSize = pCase->aadlen;
    cfg.tagSignBuf = (aesTag_t *)tagSign;
    cfg.tagVerifyBuf = (aesTag_t *)tagVerify;
    cfg.tagSize = pCase->taglen;
    cfg.tagVerifySta = &verStat;
    cfg.specialMode = spc_mod;
#ifndef SEA_HAL_FULL_TEST
    cfg.assignedAddress = (uint32_t)&AES_GCM_DMA_TEST[0];
#else
    cfg.runningMode = AES_GENERAL_MODE_EN;
#endif

    str2hex((const uint8_t *)pCase->key, (uint8_t *)key, pCase->keylen);
    str2hex((const uint8_t *)pCase->iv, (uint8_t *)iv, pCase->ivlen);
    str2hex((const uint8_t *)pCase->aad, (uint8_t *)aad, pCase->aadlen);
    if (encrypt_sel == AES_enCrypt) {
        str2hex((const uint8_t *)pCase->tag, (uint8_t *)tagVerify, pCase->taglen);
    } else {
        str2hex((const uint8_t *)pCase->tag, (uint8_t *)tagSign, pCase->taglen);
    }
    tmp_err = LL_AES_crypt_init(&pParas, (const AES_InitTypeDef *)&cfg);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

    ret_data_size = ((spc_mod == AES_SPC_GMAC_CryptData) ? pCase->ptlen : pCase->aadlen);
    loops = ((ret_data_size % buff_size) != 0U) ? ((ret_data_size / buff_size) + 1U) : (ret_data_size / buff_size);
    if (ret_data_size == 0) {
        loops = 1;
    }
    for (i = 0; i < loops; i++) {
        WDT_FEED;
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
        is_last = ((ret_data_size) ? CypherDatasNotTheLast : CypherDatasIsTheLast);

        if (spc_mod == AES_SPC_GMAC_CryptData) {
            str2hex((const uint8_t *)((uint32_t)pCase->ct + (i * buff_size * 2)), (uint8_t *)gHASHdata, calc_data_size);
            tmp_err = LL_AES_gmac(&pParas, (const GHashData_t *)((uint32_t)&gHASHdata[0]), calc_data_size, is_last);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        } else {
            str2hex((const uint8_t *)((uint32_t)pCase->aad + (i * buff_size * 2)), (uint8_t *)gHASHdata, calc_data_size);
            tmp_err = LL_AES_gmac(&pParas, (const GHashData_t *)((uint32_t)&gHASHdata[0]), calc_data_size, is_last);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        }
    }
    (void)LL_AES_crypt_deinit();

    tmp_err = memcmp((const uint8_t *)tagSign, (const uint8_t *)tagVerify, pCase->taglen);
    if (0 != tmp_err) {
        printf("[log]++tag Check Fail!!! %d\n", tmp_err);
        printf("[log]++tagSign\ttagVerify\n");
        for (i = 0; i < pCase->taglen; i++) {
            printf("\t%02X %02X\n", ((uint8_t *)tagSign)[i], ((uint8_t *)tagVerify)[i]);
        }
    }
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

    if (encrypt_sel == AES_deCrypt) {
        TEST_ASSERT_EQUAL_UINT(AES_VERIFY_SUCCESS, verStat);
    }

    return tmp_err;
}

void aes_gmac_test(void) {
    uint32_t i, j, buff_size, tmp_err = SEA_SUCCESS;
    AES_KernelTypeDef version = ((sea_update_stat()) ? AES_RUNNING_IN_CORE0 : AES_RUNNING_IN_CORE1);

#ifdef AES_GMAC_ADD_enCrypt_TEST_ENABLE
    printf("</***********AES GMAC ADD encrypt test start***********\n");
    sea_init_all(version);

    for (i = 0; i < AES_GMAC_TESTTIMES; i++) {
        for (j = 0; j < (sizeof(gmac_buffsize_testlist) >> 2U); j++) {
            WDT_FEED;
            buff_size = gmac_buffsize_testlist[j];
            printf("[log]++list: %d max_buffer_size: %d\n", i, buff_size);
            tmp_err = aes_gmac_testunit((SEA_AES_GCM_TESTLIST *)gmac_testlist[i], buff_size, AES_enCrypt, AES_SPC_NONE);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
            printf("------[log]AES GMAC ADD encrypt test %d Pass------\n", i);
        }
    }
    printf("***********AES GMAC test ADD encrypt Pass***********/>\n");
#endif

#ifdef AES_GMAC_ADD_deCrypt_TEST_ENABLE
    printf("</***********AES GMAC ADD decrypt test start***********\n");
    sea_init_all(version);

    for (i = 0; i < AES_GMAC_TESTTIMES; i++) {
        for (j = 0; j < (sizeof(gmac_buffsize_testlist) >> 2U); j++) {
            WDT_FEED;
            buff_size = gmac_buffsize_testlist[j];
            printf("[log]++list: %d max_buffer_size: %d\n", i, buff_size);
            tmp_err = aes_gmac_testunit((SEA_AES_GCM_TESTLIST *)gmac_testlist[i], buff_size, AES_deCrypt, AES_SPC_NONE);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
            printf("*------[log]AES GMAC ADD decrypt test %d Pass------\n", i);
        }
    }
    printf("***********AES GMAC test ADD decrypt Pass***********/>\n");
#endif

#ifdef AES_GMAC_CRYPTDATA_deCrypt_TEST_ENABLE
    printf("</***********AES GMAC CRYPTDATA decrypt test start***********\n");
    sea_init_all(version);

    for (i = 0; i < 10; i++) {
        for (j = 0; j < (sizeof(gmac_buffsize_testlist) >> 2U); j++) {
            WDT_FEED;
            buff_size = gmac_buffsize_testlist[j];
            printf("[log]++list: %d max_buffer_size: %d\n", i, buff_size);
            tmp_err = aes_gmac_testunit((SEA_AES_GCM_TESTLIST *)gcm_testlist[i], buff_size, AES_deCrypt, AES_SPC_GMAC_CryptData);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
            printf("------[log]AES GMAC CRYPTDATA decrypt test %d Pass------\n", i);
        }
    }
    printf("***********AES GMAC test CRYPTDATA decrypt Pass***********/>\n");
#endif
}
