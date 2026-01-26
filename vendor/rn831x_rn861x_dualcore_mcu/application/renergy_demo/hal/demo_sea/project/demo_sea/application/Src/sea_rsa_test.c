#include "sea_test.h"

#define RSA512_TEST_SUPPORT
#define RSA1024_TEST_SUPPORT
#define RSA_KEYGEN_ENABLED

const static RSA_HandleTypeDef rsa512 = {
    512, /* KeyLen: use to generate key */
    512, /* nlen */
    17,  /* elen */
    512, /* dlen */
    {0xB891B3CDU, 0x02185DF8U, 0xF270408EU, 0x15DC4BF3U, 0x0D3EEACAU, 0x23C9E95BU, 0x8CF9F409U, 0x3C2D4593U, 0x8EA6700AU, 0x65D4B579U, 0xCD1E805CU, 0xD4DBC663U, 0x9CF69A53U, 0xADEF3EB2U, 0x993AA9A9U, 0xC654BA26U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* n */
    {0x10001U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* e */
    {0xA8DF4E21U, 0x11630242U, 0xF364F412U, 0x3DE4AAE2U, 0xCA577C4BU, 0xFC8C76F2U, 0x33BEF165U, 0x0B242762U, 0xCDFB6C49U, 0x1AAEF582U, 0x6F97DE4BU, 0xED825163U, 0x14522B41U, 0xAEAF1BDDU, 0xAF21C0C1U, 0xBAAED5E9U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* d */
};

const static RSA_HandleTypeDef rsa1024 = {
    1024, /* KeyLen: use to generate key */
    1024, /* nlen */
    17,   /* elen */
    1023, /* dlen */
    {0x3E7F0445U, 0xEB866AD6U, 0x18AC4E99U, 0x7E1F9757U, 0xA8386483U, 0x7E5B20A4U, 0x18DB826CU, 0xF0CC63DDU, 0xB9A3C0BEU, 0x3D1EF794U, 0x0AB866A1U, 0x916EAB5AU, 0xA463384FU, 0x403B5EF0U, 0xADA005F5U, 0xCF1C5042U, 0x512B808AU, 0x79835714U, 0x399C3239U, 0x24FF3CFFU, 0x5C1CFF61U, 0xCBC04EDAU, 0xBA60DA06U, 0xB10435C3U, 0x18083478U, 0xBFC53BCFU, 0xDCFC09ACU, 0x88BE489AU, 0xACD76CF7U, 0x2FAD9677U, 0x971BCEAEU, 0xC3ED70F1U},
    {0x10001U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
    {0x0524D4E1U, 0xB7DC1058U, 0x96603B79U, 0xE7A68E70U, 0xF7205970U, 0x3643DC30U, 0x58C486C9U, 0xA6A60420U, 0x2BB9F47EU, 0xF4286288U, 0xD011932DU, 0xCC8CB967U, 0x822D4C6AU, 0xAFCBC348U, 0xE3069F6CU, 0x138FFCDBU, 0xB3BD6A80U, 0x28A792E5U, 0xEC85CACEU, 0x76992AE3U, 0xE3B72DD8U, 0xBD54B5BEU, 0x4E966EE8U, 0xDA750F12U, 0xDDF0A15FU, 0xE6EBD4F4U, 0x3DE20577U, 0x3326134FU, 0xC5801D9BU, 0xAD17C0E0U, 0xC40B59C6U, 0x78137C25U},
};

/* DATA */
const static uint32_t msg_0bits[] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
const static uint32_t msg_1bits[] = {0x00000001U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
const static uint32_t msg_256bits[] = {0x71A07076U, 0xC7806902U, 0x7186BE7BU, 0xF6D1EEE6U, 0xECC01EA5U, 0xD2675696U, 0x8C17A444U, 0xBA62C8F4U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,};
const static uint32_t msg_424bits[] = {0x9C874F24U, 0xC1DFCEBDU, 0xFE924A25U, 0xAFF2200BU, 0x05B6962EU, 0x6A1026D6U, 0xF86D0630U, 0x8A65F928U, 0xC9077E3AU, 0x5BA5E210U, 0x8C5FF853U, 0x753F23A7U, 0x543F3972U, 0x0000008FU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
const static uint32_t msg_512bits[] = {0xB891B3CDU, 0x02185DF8U, 0xF270408EU, 0x15DC4BF3U, 0x0D3EEACAU, 0x23C9E95BU, 0x8CF9F409U, 0x3C2D4593U, 0x8EA6701AU, 0x65D4B579U, 0xCD1E805CU, 0xD4DBC663U, 0x9CF69A53U, 0xADEF3EB2U, 0x993AA9A9U, 0xC654BA26U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
const static uint32_t msg_936bits[] = {0xD994738DU, 0x5A0769CDU, 0xDB0D2144U, 0xE96B4D0EU, 0x810A4FF5U, 0x3519E373U, 0xDA7AC903U, 0x53A64A6CU, 0x044DFFEAU, 0x45CE0F95U, 0xF92C9FDAU, 0xCBA2B99FU, 0xE142BCDCU, 0x13BBF161U, 0x2E12113DU, 0xB8304E31U, 0x964555C4U, 0x97A8E154U, 0x2955F965U, 0x4A670E4EU, 0x640F8134U, 0x2BFD89D0U, 0x18BEAA43U, 0xB3D38376U, 0x9EEFFC87U, 0x0C76B57EU, 0x7F04215EU, 0x4B0B5739U, 0x860CAE75U, 0x00000081U, 0U, 0U};
const static uint32_t msg_1024bits[] = {0x4D061584U, 0x13A90035U, 0x76870542U, 0x1DF4EF01U, 0x2D4FDA7EU, 0x3D7B8A40U, 0x89620A95U, 0xD68FCE14U, 0xE3FC4FE8U, 0xCB680E23U, 0xCCE78EF0U, 0x09637FC0U, 0x42C65866U, 0xD43F99A6U, 0xF6A363B8U, 0x51D3BD76U, 0x0C25DEF5U, 0xC96B32F3U, 0xC5B189ACU, 0xF71483DCU, 0x8C8D2BBBU, 0x3DD89A6EU, 0xA032EF69U, 0x22734DEAU, 0x5D65618BU, 0x0F212FFBU, 0xC5FB1757U, 0xF7C1ECF0U, 0xE549CDA4U, 0x0B406C9BU, 0xCD9C0020U, 0xFF243959U};

const static HAL_SEA_StatusTypeDef RSA512_EN_DE_expected[] = {HAL_SEA_STA_ERROR, HAL_SEA_STA_SUCCESS, HAL_SEA_STA_SUCCESS, HAL_SEA_STA_SUCCESS, HAL_SEA_STA_ERROR, HAL_SEA_STA_ERROR};
const static HAL_SEA_StatusTypeDef RSA1024_EN_DE_expected[] = {HAL_SEA_STA_ERROR, HAL_SEA_STA_SUCCESS, HAL_SEA_STA_SUCCESS, HAL_SEA_STA_SUCCESS, HAL_SEA_STA_SUCCESS, HAL_SEA_STA_ERROR};

const static uint32_t *msg_test[] = {
    (uint32_t *)&msg_0bits,
    (uint32_t *)&msg_1bits,
    (uint32_t *)&msg_256bits,
    (uint32_t *)&msg_424bits,
    (uint32_t *)&msg_936bits,
    (uint32_t *)&msg_1024bits,
};

#define testcnt (sizeof(msg_test) / sizeof(msg_test[0]))

static uint32_t BN_getBLen(const uint32_t N[]) {
    uint32_t pos, tmp;
    int i;

    tmp = 0U;
    for (i = 31; i >= 0; i--) {
        if (0U != N[i]) {
            tmp = N[i];
            break;
        }
    }

    pos = 0U;
    while (0U != tmp) {
        tmp >>= 1U;
        pos++;
    }

    return (uint32_t)((uint32_t)(i * 32) + pos);
}

static uint32_t rsa_test_sign(RSA_HandleTypeDef *rsa, const uint32_t *msg, uint32_t msg_len) {
    uint32_t sign[32] = {0};
    HAL_SEA_StatusTypeDef tmp_err = HAL_SEA_STA_SUCCESS;
    tmp_err = LL_RSA_sign_gen(rsa, msg, msg_len, sign);
    if (tmp_err != HAL_SEA_STA_SUCCESS) {
        return tmp_err;
    }
    WDT_FEED;
    tmp_err = LL_RSA_sign_ver(rsa, msg, msg_len, sign);
    TEST_ASSERT_EQUAL_UINT(HAL_SEA_STA_SUCCESS, tmp_err);
    return tmp_err;
}

static uint32_t rsa_test_enc(RSA_HandleTypeDef *rsa, const uint32_t *msg) {
    uint32_t tgt[32] = {0};
    uint32_t res[32] = {0};
    int i;
    HAL_SEA_StatusTypeDef tmp_err = HAL_SEA_STA_SUCCESS;

    tmp_err = LL_RSA_pub_encrypt(rsa, msg, BN_getBLen(msg), tgt);
    if (tmp_err != HAL_SEA_STA_SUCCESS) {
        return tmp_err;
    }
    tmp_err = LL_RSA_pvt_decrypt(rsa, tgt, BN_getBLen(tgt), res);
    if (tmp_err != HAL_SEA_STA_SUCCESS) {
        return tmp_err;
    }

    for (i = 0U; i < 32; i++) {
        if (i == (BN_getBLen(msg) >> 5))
            break;
        if (res[i] == msg[i]) {
            continue;
        } else {
            tmp_err = HAL_SEA_STA_ERROR;
            break;
        }
    }

    return tmp_err;
}

void rsa_test(void) {
    uint32_t i;
    HAL_SEA_StatusTypeDef tmp_err = HAL_SEA_STA_SUCCESS;
    AES_KernelTypeDef version = ((sea_update_stat()) ? AES_RUNNING_IN_CORE0 : AES_RUNNING_IN_CORE1);

#ifdef RSA512_TEST_SUPPORT
    printf("</************RSA512 test start************\n");
    sea_init_all(version);

    TEST_ASSERT_EQUAL_UINT(HAL_SEA_STA_SUCCESS, rsa_test_sign((RSA_HandleTypeDef *)&rsa512, msg_256bits, 256U));
    printf("------[log]RSA512 sign with SHA-256 test Pass------\n");

    for (i = 0U; i < testcnt; i++) {
        WDT_FEED;
        TEST_ASSERT_EQUAL_UINT(RSA512_EN_DE_expected[i], rsa_test_enc((RSA_HandleTypeDef *)&rsa512, msg_test[i]));
        printf("------[log]RSA512 test %d Pass------\n", i);
    }

#ifdef RSA_KEYGEN_ENABLED
    /* test keygen */
    int flag = 0, err = 1;
    RSA_HandleTypeDef tmp1;
    tmp1.KeyLen = 512U;
    while (flag < 20) {
        WDT_FEED;
        if (HAL_SEA_STA_SUCCESS == LL_RSA_keygen((RSA_HandleTypeDef *)&tmp1)) {
            err = 0;
            printf("------[log]RSA512 keygen test Pass, run times: %d------\n", flag);
            break;
        }
        flag++;
    }
    if (err != 0) {
        printf("------[log]RSA512 keygen test Fail!!!------\n");
    }
#endif

    printf("************[log]RSA512 test end************/>\n");
#endif

#ifdef RSA1024_TEST_SUPPORT
    printf("</************RSA1024 test start************\n");

    sea_init_all(version);
    TEST_ASSERT_EQUAL_UINT(HAL_SEA_STA_SUCCESS, rsa_test_sign((RSA_HandleTypeDef *)&rsa1024, msg_256bits, 256U));
    printf("------[log]RSA1024 sign with SHA-256 test Pass------\n");
    WDT_FEED;
    TEST_ASSERT_EQUAL_UINT(HAL_SEA_STA_SUCCESS, rsa_test_sign((RSA_HandleTypeDef *)&rsa1024, msg_512bits, 512U));
    printf("------[log]RSA1024 sign with SHA-512 test Pass------\n");

    for (i = 0U; i < testcnt; i++) {
        WDT_FEED;
        TEST_ASSERT_EQUAL_UINT(RSA1024_EN_DE_expected[i], rsa_test_enc((RSA_HandleTypeDef *)&rsa1024, msg_test[i]));
        printf("------[log]RSA1024 test %d Pass------\n", i);
    }

#ifdef RSA_KEYGEN_ENABLED
    /* test keygen */
    int tmp = 0;
    RSA_HandleTypeDef tmp2;
    tmp2.KeyLen = 1024U;
    while (tmp < 2) {
        WDT_FEED;
        if (HAL_SEA_STA_SUCCESS != LL_RSA_keygen((RSA_HandleTypeDef *)&tmp2)) {
            printf("------[log]rsa1024 keygen test Pass, run times: %d------\n", tmp);
            tmp++;
            continue;
        }
        printf("------[log]rsa1024 keygen test Fail!!!------\n");
        break;
    }
#endif

    printf("************RSA1024 test end************/>\n");
#endif
}
