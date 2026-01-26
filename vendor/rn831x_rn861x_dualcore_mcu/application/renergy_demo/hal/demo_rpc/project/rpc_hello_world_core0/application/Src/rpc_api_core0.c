#include "rn_rv_hal.h"
#include "rpc_api.h"

HAL_StatusTypeDef HAL_SYSTEM_ClockModeConfig_server(HAL_SYSTEM_ClockModeTypeDef ClockMode, HAL_SYSTEM_SourceDivTypeDef SourceDivison, HAL_SYSTEM_CoreDivTypeDef CoreDivison)
{
    return HAL_SYSTEM_ClockModeConfig(ClockMode, SourceDivison, CoreDivison);
}

HAL_StatusTypeDef HAL_SYSTEM_FastClockModeConfig_server(HAL_SYSTEM_ClockModeTypeDef ClockMode)
{
    return HAL_SYSTEM_FastClockModeConfig(ClockMode);
}

HAL_StatusTypeDef HAL_SYSTEM_SetCoreDivision_server(HAL_SYSTEM_CoreDivTypeDef CoreDivison)
{
    return HAL_SYSTEM_SetCoreDivision(CoreDivison);
}

HAL_StatusTypeDef HAL_SYSTEM_PLLConfig_server(HAL_SYSTEM_ClockModeTypeDef PLLFrequency, HAL_SYSTEM_PLLSwitchTypeDef PLLState)
{
    return HAL_SYSTEM_PLLConfig(PLLFrequency, PLLState);
}

HAL_StatusTypeDef HAL_SYSTEM_OptionConfigCheck_server(void)
{
    return HAL_SYSTEM_OptionConfigCheck();
}

HAL_StatusTypeDef HAL_SYSTEM_SetRTCDota_server(uint16_t RTCDota)
{
    return HAL_SYSTEM_SetRTCDota(RTCDota);
}

uint16_t HAL_SYSTEM_GetRTCDota_server(void)
{
    return HAL_SYSTEM_GetRTCDota();
}

HAL_FLASH_StatusTypeDef HAL_FLASH_Unlock_server(HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t ByteLength)
{
    uint32_t Flash1Size, Flash2Size;

    if (FlashID != HAL_FLASH_ID_FLASH1) {
        return HAL_FALSH_INVALID_PARAM;
    }

    Flash1Size = (((FLASH->ALLOCATE1 >> FLASH_ALLOCATE1_HSM_ENDADDR_POS) & 0x7fU) + 1U) * 8 * 1024;
    Flash2Size = (((FLASH->ALLOCATE1 >> FLASH_ALLOCATE1_HOST_ENDADDR_POS) & 0x7fU) + 1U) * 8 * 1024;

    if (Flash2Size > Flash1Size) {
        Flash2Size = Flash2Size - Flash1Size;
    }
    else {
        Flash2Size = 0U;
    }

    if ((DstAddress < CORE1_FLASH_REMAP0_BASE_ADDRESS) || (DstAddress > (CORE1_FLASH_REMAP0_BASE_ADDRESS + Flash2Size)) ||
        ((DstAddress + ByteLength) > (CORE1_FLASH_REMAP0_BASE_ADDRESS + Flash2Size))) {
        return HAL_FALSH_DST_ADDR_NOT_MAPPED;
    }
    return HAL_FLASH_Unlock(FlashID, DstAddress, ByteLength);
}

HAL_FLASH_StatusTypeDef HAL_FLASH_Program_server(HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t SrcAddress, uint32_t ByteLength)
{
    uint32_t Flash1Size, Flash2Size;

    if (FlashID != HAL_FLASH_ID_FLASH1) {
        return HAL_FALSH_INVALID_PARAM;
    }

    Flash1Size = (((FLASH->ALLOCATE1 >> FLASH_ALLOCATE1_HSM_ENDADDR_POS) & 0x7fU) + 1U) * 8 * 1024;
    Flash2Size = (((FLASH->ALLOCATE1 >> FLASH_ALLOCATE1_HOST_ENDADDR_POS) & 0x7fU) + 1U) * 8 * 1024;

    if (Flash2Size > Flash1Size) {
        Flash2Size = Flash2Size - Flash1Size;
    }
    else {
        Flash2Size = 0U;
    }

    if ((DstAddress < CORE1_FLASH_REMAP0_BASE_ADDRESS) || (DstAddress > (CORE1_FLASH_REMAP0_BASE_ADDRESS + Flash2Size)) ||
        ((DstAddress + ByteLength) > (CORE1_FLASH_REMAP0_BASE_ADDRESS + Flash2Size))) {
        return HAL_FALSH_DST_ADDR_NOT_MAPPED;
    }
    return HAL_FLASH_Program(FlashID, DstAddress, SrcAddress, ByteLength);
}

HAL_FLASH_StatusTypeDef HAL_FLASH_Erase_server(HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t ByteLength)
{
    uint32_t Flash1Size, Flash2Size;

    if (FlashID != HAL_FLASH_ID_FLASH1) {
        return HAL_FALSH_INVALID_PARAM;
    }

    Flash1Size = (((FLASH->ALLOCATE1 >> FLASH_ALLOCATE1_HSM_ENDADDR_POS) & 0x7fU) + 1U) * 8 * 1024;
    Flash2Size = (((FLASH->ALLOCATE1 >> FLASH_ALLOCATE1_HOST_ENDADDR_POS) & 0x7fU) + 1U) * 8 * 1024;

    if (Flash2Size > Flash1Size) {
        Flash2Size = Flash2Size - Flash1Size;
    }
    else {
        Flash2Size = 0U;
    }

    if ((DstAddress < CORE1_FLASH_REMAP0_BASE_ADDRESS) || (DstAddress > (CORE1_FLASH_REMAP0_BASE_ADDRESS + Flash2Size)) ||
        ((DstAddress + ByteLength) > (CORE1_FLASH_REMAP0_BASE_ADDRESS + Flash2Size))) {
        return HAL_FALSH_DST_ADDR_NOT_MAPPED;
    }
    return HAL_FLASH_Erase(FlashID, DstAddress, ByteLength);
}

HAL_FLASH_StatusTypeDef HAL_FLASH_ClearSoftISP_server(uint32_t ISPProtect)
{
    return HAL_FLASH_ClearSoftISP(ISPProtect);
}

HAL_FLASH_StatusTypeDef HAL_FLASH_EnableSoftISP_server(uint32_t ISPCommand, uint32_t UartIOCommand)
{
    return HAL_FLASH_EnableSoftISP(ISPCommand, UartIOCommand);
}

HAL_FLASH_StatusTypeDef HAL_FLASH_ClearCP_server(uint32_t JtagMode, uint32_t Core0JtagEnable, uint32_t Core1JtagEnable)
{
    return HAL_FLASH_ClearCP(JtagMode, Core0JtagEnable, Core1JtagEnable);
}

HAL_SEA_StatusTypeDef HAL_SEA_AES_GF128Mul_server(AES_HandleTypeDef *haes, const uint8_t *pDataX, const uint8_t *pDataY, uint8_t *pDataZ)
{
    return HAL_SEA_AES_GF128Mul(haes, pDataX, pDataY, pDataZ);
}

HAL_SEA_StatusTypeDef HAL_SEA_AES_Init_server(AES_HandleTypeDef *haes, const AES_InitTypeDef *pConfig)
{
    return HAL_SEA_AES_Init(haes, pConfig);
}

HAL_SEA_StatusTypeDef HAL_SEA_AES_DeInit_server(void)
{
    return HAL_SEA_AES_DeInit();
}

HAL_SEA_StatusTypeDef HAL_SEA_AES_Encrypt_server(AES_HandleTypeDef *haes, const uint8_t *PlainData, uint32_t PlainLen, CYPHER_DATA_STA IsLast, uint8_t *CipherData)
{
    return HAL_SEA_AES_Encrypt(haes, PlainData, PlainLen, IsLast, CipherData);
}

HAL_SEA_StatusTypeDef HAL_SEA_AES_Decrypt_server(AES_HandleTypeDef *haes, const uint8_t *CipherData, uint32_t CipherLen, CYPHER_DATA_STA IsLast, uint8_t *PlainData)
{
    return HAL_SEA_AES_Decrypt(haes, CipherData, CipherLen, IsLast, PlainData);
}

HAL_SEA_StatusTypeDef HAL_SEA_AES_Gmac_server(AES_HandleTypeDef *haes, const uint8_t *GhashData, uint32_t GhashDataLen, CYPHER_DATA_STA IsLast)
{
    return HAL_SEA_AES_Gmac(haes, GhashData, GhashDataLen, IsLast);
}

HAL_SEA_StatusTypeDef HAL_SEA_AES_LatchProcessStatus_server(AES_HandleTypeDef *haes, AES_HandleTypeDef *LatchBuf)
{
    return HAL_SEA_AES_LatchProcessStatus(haes, LatchBuf);
}

HAL_SEA_StatusTypeDef HAL_SEA_AES_RecoverProcessStatus_server(AES_HandleTypeDef *RecoverBuf, AES_HandleTypeDef *haes)
{
    return HAL_SEA_AES_RecoverProcessStatus(RecoverBuf, haes);
}

HAL_SEA_StatusTypeDef HAL_SEA_AES_KeyWrapping_server(AES_HandleTypeDef *haes, const uint8_t *PlainData, uint32_t PlainLen, uint8_t *CipherData, uint32_t *CipherLen)
{
    return HAL_SEA_AES_KeyWrapping(haes, PlainData, PlainLen, CipherData, CipherLen);
}

HAL_SEA_StatusTypeDef HAL_SEA_AES_KeyUnwrapping_server(AES_HandleTypeDef *haes, const uint8_t *CipherData, uint32_t CipherLen, uint8_t *PlainData, uint32_t *PlainLen)
{
    return HAL_SEA_AES_KeyUnwrapping(haes, CipherData, CipherLen, PlainData, PlainLen);
}

HAL_SEA_StatusTypeDef HAL_SEA_AES_SetAAD_server(AES_HandleTypeDef *haes, const uint8_t *DataIn, uint32_t DataLen)
{
    return HAL_SEA_AES_SetAAD(haes, DataIn, DataLen);
}

HAL_SEA_StatusTypeDef HAL_SEA_HASH_Init_server(HASH_HandleTypeDef *hhash, const HASH_InitTypeDef *pConfig)
{
    return HAL_SEA_HASH_Init(hhash, pConfig);
}

HAL_SEA_StatusTypeDef HAL_SEA_HASH_DeInit_server(void)
{
    return HAL_SEA_HASH_DeInit();
}

HAL_SEA_StatusTypeDef HAL_SEA_HASH_server(HASH_HandleTypeDef *hhash, const uint8_t *Input, uint32_t InputLen, CYPHER_DATA_STA IsLast, uint8_t *Output)
{
    return HAL_SEA_HASH(hhash, Input, InputLen, IsLast, Output);
}

HAL_SEA_StatusTypeDef HAL_SEA_HASH_LatchProcessStatus_server(const HASH_HandleTypeDef *hhash, HASH_HandleTypeDef *LatchBuf)
{
    return HAL_SEA_HASH_LatchProcessStatus(hhash, LatchBuf);
}

HAL_SEA_StatusTypeDef HAL_SEA_HASH_RecoverProcessStatus_server(const HASH_HandleTypeDef *RecoverBuf, HASH_HandleTypeDef *hhash)
{
    return HAL_SEA_HASH_RecoverProcessStatus(RecoverBuf, hhash);
}

HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateKey_server(const ECC_HandleTypeDef *hecc, uint8_t *Q, uint8_t *K)
{
    return HAL_SEA_ECC_GenerateKey(hecc, Q, K);
}

HAL_SEA_StatusTypeDef HAL_SEA_ECC_GeneratePublicKey_server(const ECC_HandleTypeDef *hecc, uint8_t *Q, const uint8_t *K)
{
    return HAL_SEA_ECC_GeneratePublicKey(hecc, Q, K);
}

HAL_SEA_StatusTypeDef HAL_SEA_ECC_VerifyPublicKey_server(const ECC_HandleTypeDef *hecc, const uint8_t *Q, HAL_SEA_FlagTypeDef *pFlag)
{
    return HAL_SEA_ECC_VerifyPublicKey(hecc, Q, pFlag);
}

HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateDHKeyPairs_server(const ECC_HandleTypeDef *hecc, const uint8_t *Q, const uint8_t *K, uint8_t *DHKey)
{
    return HAL_SEA_ECC_GenerateDHKeyPairs(hecc, Q, K, DHKey);
}

HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateSignature_server(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, uint8_t *Q, uint8_t *K, uint8_t *R, uint8_t *S)
{
    return HAL_SEA_ECC_GenerateSignature(hecc, HashMessage, MessageLen, Q, K, R, S);
}

HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateSignaturewithPKey_server(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, uint8_t *Q, const uint8_t *K, uint8_t *R, uint8_t *S)
{
    return HAL_SEA_ECC_GenerateSignaturewithPKey(hecc, HashMessage, MessageLen, Q, K, R, S);
}

HAL_SEA_StatusTypeDef HAL_SEA_ECC_VerifySignature_server(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t *Q, const uint8_t *R, const uint8_t *S, HAL_SEA_FlagTypeDef *pFlag)
{
    return HAL_SEA_ECC_VerifySignature(hecc, HashMessage, MessageLen, Q, R, S, pFlag);
}

HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateKeySecurely_server(const ECC_HandleTypeDef *hecc, uint8_t PKeyIdx, uint8_t QxIdx, uint8_t QyIdx)
{
    return HAL_SEA_ECC_GenerateKeySecurely(hecc, PKeyIdx, QxIdx, QyIdx);
}

HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateSignatureSecurely_server(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t PKeyIdx, uint8_t *R, uint8_t *S)
{
    return HAL_SEA_ECC_GenerateSignatureSecurely(hecc, HashMessage, MessageLen, PKeyIdx, R, S);
}

HAL_SEA_StatusTypeDef HAL_SEA_ECC_VerifySignatureSecurely_server(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t QxIdx, const uint8_t QyIdx, const uint8_t *R, const uint8_t *S, HAL_SEA_FlagTypeDef *pFlag)
{
    return HAL_SEA_ECC_VerifySignatureSecurely(hecc, HashMessage, MessageLen, QxIdx, QyIdx, R, S, pFlag);
}

HAL_SEA_StatusTypeDef HAL_SEA_RSA_GenerateKey_server(RSA_HandleTypeDef *hrsa)
{
    return HAL_SEA_RSA_GenerateKey(hrsa);
}

HAL_SEA_StatusTypeDef HAL_SEA_RSA_GenerateSignature_server(RSA_HandleTypeDef *hrsa, const uint32_t *HashMessage, uint32_t MessageLen, uint32_t *Signature)
{
    return HAL_SEA_RSA_GenerateSignature(hrsa, HashMessage, MessageLen, Signature);
}

HAL_SEA_StatusTypeDef HAL_SEA_RSA_VerifySignature_server(RSA_HandleTypeDef *hrsa, const uint32_t *HashMessage, uint32_t MessageLen, uint32_t *Signature)
{
    return HAL_SEA_RSA_VerifySignature(hrsa, HashMessage, MessageLen, Signature);
}

HAL_SEA_StatusTypeDef HAL_SEA_RSA_EncryptwithPublicKey_server(RSA_HandleTypeDef *hrsa, const uint32_t *PlainData, uint32_t PlainLen, uint32_t *CipherData)
{
    return HAL_SEA_RSA_EncryptwithPublicKey(hrsa, PlainData, PlainLen, CipherData);
}

HAL_SEA_StatusTypeDef HAL_SEA_RSA_DecryptwithPublicKey_server(RSA_HandleTypeDef *hrsa, const uint32_t *CipherData, uint32_t CipherLen, uint32_t *PlainData)
{
    return HAL_SEA_RSA_DecryptwithPublicKey(hrsa, CipherData, CipherLen, PlainData);
}

HAL_SEA_StatusTypeDef HAL_SEA_RSA_EncryptwithPrivateKey_server(RSA_HandleTypeDef *hrsa, const uint32_t *PlainData, uint32_t PlainLen, uint32_t *CipherData)
{
    return HAL_SEA_RSA_EncryptwithPrivateKey(hrsa, PlainData, PlainLen, CipherData);
}

HAL_SEA_StatusTypeDef HAL_SEA_RSA_DecryptwithPrivateKey_server(RSA_HandleTypeDef *hrsa, const uint32_t *CipherData, uint32_t CipherLen, uint32_t *PlainData)
{
    return HAL_SEA_RSA_DecryptwithPrivateKey(hrsa, CipherData, CipherLen, PlainData);
}

HAL_SEA_StatusTypeDef HAL_SEA_TRNG_GenerateRandom_server(uint32_t *RandomData, uint8_t DataLen)
{
    return HAL_SEA_TRNG_GenerateRandom(RandomData, DataLen);
}

HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_Unlock_server(void)
{
    return HAL_KEYSTORE_Unlock();
}

HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_ReadKey_server(uint32_t *PKey, uint32_t KeyLen, const uint8_t KeyIdx)
{
    return HAL_KEYSTORE_ReadKey(PKey, KeyLen, KeyIdx);
}

HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_ReadKeyProperty_server(KEYSTORE_InitTypeDef *KEYSTORE_InitStruct, const uint8_t KeyIdx)
{
    return HAL_KEYSTORE_ReadKeyProperty(KEYSTORE_InitStruct, KeyIdx);
}

HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_WriteKey_server(KEYSTORE_InitTypeDef *KEYSTORE_InitStruct, const uint32_t *PKey, const uint8_t KeyIdx)
{
    return HAL_KEYSTORE_WriteKey(KEYSTORE_InitStruct, PKey, KeyIdx);
}

HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_Erase_server(uint32_t Passwd)
{
    return HAL_KEYSTORE_Erase(Passwd);
}

uint32_t HAL_KEYSTORE_GetState_server(void)
{
    return HAL_KEYSTORE_GetState();
}

uint32_t test_rpc_function_server(uint32_t Param1, uint32_t Param2, uint32_t Param3)
{
    return test_rpc_function(Param1, Param2, Param3);
}

void test_rpc_helloworld_function_server(void)
{
    test_rpc_helloworld_function();
}

uint32_t api_server_list[] = {
    (uint32_t)HAL_SYSTEM_ClockModeConfig_server,
    (uint32_t)HAL_SYSTEM_FastClockModeConfig_server,
    (uint32_t)HAL_SYSTEM_SetCoreDivision_server,
    (uint32_t)HAL_SYSTEM_PLLConfig_server,
    (uint32_t)HAL_SYSTEM_OptionConfigCheck_server,
    (uint32_t)HAL_SYSTEM_SetRTCDota_server,
    (uint32_t)HAL_SYSTEM_GetRTCDota_server,
    (uint32_t)HAL_FLASH_Unlock_server,
    (uint32_t)HAL_FLASH_Program_server,
    (uint32_t)HAL_FLASH_Erase_server,
    (uint32_t)HAL_FLASH_ClearSoftISP_server,
    (uint32_t)HAL_FLASH_EnableSoftISP_server,
    (uint32_t)HAL_FLASH_ClearCP_server,
    (uint32_t)HAL_SEA_AES_GF128Mul_server,
    (uint32_t)HAL_SEA_AES_Init_server,
    (uint32_t)HAL_SEA_AES_DeInit_server,
    (uint32_t)HAL_SEA_AES_Encrypt_server,
    (uint32_t)HAL_SEA_AES_Decrypt_server,
    (uint32_t)HAL_SEA_AES_Gmac_server,
    (uint32_t)HAL_SEA_AES_LatchProcessStatus_server,
    (uint32_t)HAL_SEA_AES_RecoverProcessStatus_server,
    (uint32_t)HAL_SEA_AES_KeyWrapping_server,
    (uint32_t)HAL_SEA_AES_KeyUnwrapping_server,
    (uint32_t)HAL_SEA_AES_SetAAD_server,
    (uint32_t)HAL_SEA_HASH_Init_server,
    (uint32_t)HAL_SEA_HASH_DeInit_server,
    (uint32_t)HAL_SEA_HASH_server,
    (uint32_t)HAL_SEA_HASH_LatchProcessStatus_server,
    (uint32_t)HAL_SEA_HASH_RecoverProcessStatus_server,
    (uint32_t)HAL_SEA_ECC_GenerateKey_server,
    (uint32_t)HAL_SEA_ECC_GeneratePublicKey_server,
    (uint32_t)HAL_SEA_ECC_VerifyPublicKey_server,
    (uint32_t)HAL_SEA_ECC_GenerateDHKeyPairs_server,
    (uint32_t)HAL_SEA_ECC_GenerateSignature_server,
    (uint32_t)HAL_SEA_ECC_GenerateSignaturewithPKey_server,
    (uint32_t)HAL_SEA_ECC_VerifySignature_server,
    (uint32_t)HAL_SEA_ECC_GenerateKeySecurely_server,
    (uint32_t)HAL_SEA_ECC_GenerateSignatureSecurely_server,
    (uint32_t)HAL_SEA_ECC_VerifySignatureSecurely_server,
    (uint32_t)HAL_SEA_RSA_GenerateKey_server,
    (uint32_t)HAL_SEA_RSA_GenerateSignature_server,
    (uint32_t)HAL_SEA_RSA_VerifySignature_server,
    (uint32_t)HAL_SEA_RSA_EncryptwithPublicKey_server,
    (uint32_t)HAL_SEA_RSA_DecryptwithPublicKey_server,
    (uint32_t)HAL_SEA_RSA_EncryptwithPrivateKey_server,
    (uint32_t)HAL_SEA_RSA_DecryptwithPrivateKey_server,
    (uint32_t)HAL_SEA_TRNG_GenerateRandom_server,
    (uint32_t)HAL_KEYSTORE_Unlock_server,
    (uint32_t)HAL_KEYSTORE_ReadKey_server,
    (uint32_t)HAL_KEYSTORE_ReadKeyProperty_server,
    (uint32_t)HAL_KEYSTORE_WriteKey_server,
    (uint32_t)HAL_KEYSTORE_Erase_server,
    (uint32_t)HAL_KEYSTORE_GetState_server,
    (uint32_t)test_rpc_function_server,
    (uint32_t)test_rpc_helloworld_function_server,
};
