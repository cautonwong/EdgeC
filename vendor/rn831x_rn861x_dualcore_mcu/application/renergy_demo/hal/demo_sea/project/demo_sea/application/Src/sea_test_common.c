#include "sea_test.h"
#include <stdint.h>
#include "rn_rv_ll_system_core0.h"
#include "rn_rv_ll_wdt.h"

void str2hex(const uint8_t *input_str, uint8_t *output_hex, uint32_t byte_len) {
    const uint8_t *pChar = input_str;
    uint8_t *pNumber = output_hex;
    uint8_t HexHigh, HexLow;
    uint32_t NumberLen, len_temp = byte_len;
    for (NumberLen = 0U; NumberLen < len_temp; NumberLen++) {
        HexHigh = *pChar++;
        HexLow = *pChar++;
        if ((HexHigh >= '0') && (HexHigh <= '9')) {
            HexHigh = HexHigh - '0';
        } else if ((HexHigh >= 'A') && (HexHigh <= 'F')) {
            HexHigh = HexHigh - 'A' + 10U;
        } else if ((HexHigh >= 'a') && (HexHigh <= 'f')) {
            HexHigh = HexHigh - 'a' + 10U;
        } else {
            HexHigh = 0;
        }
        if ((HexLow >= '0') && (HexLow <= '9')) {
            HexLow = HexLow - '0';
        } else if ((HexLow >= 'A') && (HexLow <= 'F')) {
            HexLow = HexLow - 'A' + 10U;
        } else if ((HexLow >= 'a') && (HexLow <= 'f')) {
            HexLow = HexLow - 'a' + 10U;
        } else {
            HexLow = 0;
        }
        *pNumber++ = ((HexHigh << 4) & 0xF0U) | (HexLow & 0x0FU);
    }
}

void word2str(uint32_t *words, uint32_t bytelen) {
    uint32_t i, temp;
    uint8_t *s;
    s = (uint8_t *)words;
    for (i = 0; i < (bytelen >> 1U); i++) {
        temp = s[i];
        s[i] = s[bytelen - 1 - i];
        s[bytelen - 1 - i] = temp;
    }
}

uint32_t getHashOutSize(const HASH_InitTypeDef *pParas) {
    uint32_t calc_data_len;
    if (pParas->bitWideMode == SEA_SHA_1) {
        calc_data_len = NOUT_SHA_1;
    } else if (pParas->bitWideMode == SEA_SHA_224) {
        calc_data_len = NOUT_SHA_224;
    } else if (pParas->bitWideMode == SEA_SHA_256) {
        calc_data_len = NOUT_SHA_256;
    } else if (pParas->bitWideMode == SEA_SHA_384) {
        calc_data_len = NOUT_SHA_384;
    } else if (pParas->bitWideMode == SEA_SHA_512) {
        calc_data_len = NOUT_SHA_512;
    } else {
        printf("[log]Hash Mode ERROR!!!\n");
    }
    return (calc_data_len * sizeof(uint32_t));
}

uint32_t sea_update_stat(void)
{
    uint8_t modify_tag = 0U;
    uint32_t flag, sysps_bak, scu_clk_tmp = 0U;
    AES_KernelTypeDef ChipID = AES_RUNNING_IN_CORE1;
    if (1U != ((MEM_U32(SYS_PMOD0_ADDR(ChipID)) & 0x8U) >> 3U)) {
        sysps_bak = MEM_U32(SYS_PSW_ADDR(ChipID));
        MEM_U32(SYS_PSW_ADDR(ChipID)) = 0x82U;
        scu_clk_tmp = (uint32_t)MEM_U32(SYS_PMOD0_ADDR(ChipID)); /* save original SCU clock */
        MEM_U32(SYS_PMOD0_ADDR(ChipID)) |= ((uint32_t)1U << 3U); /* start SCU clock */
        modify_tag = 1U;
    }
    flag = ((MEM_U32(SCU_PERIPH_SEC_CFG2(ChipID)) & 0x4U) >> 2U);
    if (1U == modify_tag) {
        MEM_U32(SYS_PMOD0_ADDR(ChipID)) = scu_clk_tmp; /* reset SCU CLK */
        MEM_U32(SYS_PSW_ADDR(ChipID)) = sysps_bak;
    }
    return flag;
}

/* SEA被分配给CORE0/CORE1 */
void sea_init_all(AES_KernelTypeDef version) {
    unsigned int i, addr;
    // unsigned int sysps_bak = SYSTEM->SYS_PSW1;
    // SYSTEM->SYS_PSW1 = 0x82U;
    // SYSTEM->MOD2_CKE |= SYSTEM_CORE0_MOD2_CKE_SEA_BUS;
    // SYSTEM->MOD2_CKE |= SYSTEM_CORE0_MOD2_CKE_SEA_PKE;
    // SYSTEM->MOD2_CKE |= SYSTEM_CORE0_MOD2_CKE_SEA_AES;
    // SYSTEM->MOD2_CKE |= SYSTEM_CORE0_MOD2_CKE_SEA_TRNG;
    // SYSTEM->MOD2_CKE |= SYSTEM_CORE0_MOD2_CKE_SEA_SHA;
    // SYSTEM->MOD2_CKE |= SYSTEM_CORE0_MOD2_CKE_KEY_STORE;
    // SYSTEM->SYS_PSW1 = sysps_bak;
#if defined(CORE0)
    LL_SYSTEM_CORE0_MOD2_EnableClock(
#if defined(SEA_HAL_FULL_TEST)
                                    LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_BUS  | \
                                    LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_PKE  | \
                                    LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_AES  | \
                                    LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_TRNG | \
                                    LL_SYSTEM_CORE0_MOD2_PERIPH_SEA_SHA  | \
                                    LL_SYSTEM_CORE0_MOD2_PERIPH_KEY_STORE
#else
                                    LL_SYSTEM_CORE0_MOD2_PERIPH_AES
#endif
    );
#elif defined(CORE1)
    LL_SYSTEM_CORE1_MOD2_EnableClock(
#if defined(SEA_HAL_FULL_TEST)
                                    LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_BUS  | \
                                    LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_PKE  | \
                                    LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_AES  | \
                                    LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_TRNG | \
                                    LL_SYSTEM_CORE1_MOD2_PERIPH_SEA_SHA  | \
                                    LL_SYSTEM_CORE1_MOD2_PERIPH_KEY_STORE
#else
                                    LL_SYSTEM_CORE1_MOD2_PERIPH_AES
#endif
    );
#endif
    for (i = 0U; i < 1024U; i++) {
#if defined(CORE0)
        addr = 0x30009000U + (4U * i);
#elif defined(CORE1)
        addr = 0xB0009000U + (4U * i);
#endif
        *((unsigned int *)(addr)) = 0x0U;
    }
}

void sea_trng_init(void) {
    unsigned int sysps_bak;
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD2_CKE |= (SYSTEM_CORE0_MOD2_CKE_SEA_TRNG | SYSTEM_CORE0_MOD2_CKE_SEA_BUS);
    SYSTEM->SYS_PSW1 = sysps_bak;
}

void sea_ecc_init(void) {
    unsigned int sysps_bak;
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD2_CKE |= (SYSTEM_CORE0_MOD2_CKE_SEA_PKE | SYSTEM_CORE0_MOD2_CKE_SEA_BUS);
    SYSTEM->SYS_PSW1 = sysps_bak;
}

void sea_rsa_init(void) {
    unsigned int sysps_bak;
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD2_CKE |= (SYSTEM_CORE0_MOD2_CKE_SEA_PKE | SYSTEM_CORE0_MOD2_CKE_SEA_BUS);
    SYSTEM->SYS_PSW1 = sysps_bak;
}

void sea_hash_init(void) {
    unsigned int sysps_bak;
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD2_CKE |= (SYSTEM_CORE0_MOD2_CKE_SEA_SHA | SYSTEM_CORE0_MOD2_CKE_SEA_BUS);
    SYSTEM->SYS_PSW1 = sysps_bak;
}

void sea_hash_deinit(void) {
    unsigned int sysps_bak;
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD2_CKE &= (~(SYSTEM_CORE0_MOD2_CKE_SEA_SHA | SYSTEM_CORE0_MOD2_CKE_SEA_BUS));
    SYSTEM->SYS_PSW1 = sysps_bak;
}

void sea_aes_init(void) {
    unsigned int sysps_bak;
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD2_CKE |= (SYSTEM_CORE0_MOD2_CKE_SEA_AES | SYSTEM_CORE0_MOD2_CKE_SEA_BUS);
    SYSTEM->SYS_PSW1 = sysps_bak;
}

void sea_aes_deinit(void) {
    unsigned int sysps_bak;
    sysps_bak = SYSTEM->SYS_PSW1;
    SYSTEM->SYS_PSW1 = 0x82U;
    SYSTEM->MOD2_CKE &= (~(SYSTEM_CORE0_MOD2_CKE_SEA_AES | SYSTEM_CORE0_MOD2_CKE_SEA_BUS));
    SYSTEM->SYS_PSW1 = sysps_bak;
}

void sea_memset_word(uint32_t dst[], uint32_t data, uint32_t len) {
    unsigned int i;
    for (i = 0U; i < len; i++) {
        dst[i] = data;
    }
}

void sea_memset(uint8_t dst[], const uint8_t data, uint32_t len) {
    unsigned int i;
    for (i = 0U; i < len; i++) {
        dst[i] = data;
    }
}

void sea_memcpy_word(uint32_t dst[], const uint32_t src[], uint32_t len) {
    unsigned int i;
    for (i = 0U; i < len; i++) {
        dst[i] = src[i];
    }
}

void sea_memcpy(uint8_t dst[], const uint8_t src[], uint32_t len) {
    unsigned int i;
    for (i = 0U; i < len; i++) {
        dst[i] = src[i];
    }
}
