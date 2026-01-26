#ifndef RPC_API_H_
#define RPC_API_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "rn_rv_hal_def.h"
#include "rn_rv_hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define CORE1_FLASH_REMAP0_BASE_ADDRESS (0x80000000U)
#define TIMEOUT_DEFAULT                 (100U)            /* Timeout 100ms */


/* Clock Mode and Frequency Config functions  **********************************************/
HAL_StatusTypeDef HAL_SYSTEM_ClockModeConfig(HAL_SYSTEM_ClockModeTypeDef ClockMode, HAL_SYSTEM_SourceDivTypeDef SourceDivison, HAL_SYSTEM_CoreDivTypeDef CoreDivison);
HAL_StatusTypeDef HAL_SYSTEM_FastClockModeConfig(HAL_SYSTEM_ClockModeTypeDef ClockMode);
HAL_StatusTypeDef HAL_SYSTEM_SetCoreDivision(HAL_SYSTEM_CoreDivTypeDef CoreDivison);
HAL_StatusTypeDef HAL_SYSTEM_PLLConfig(HAL_SYSTEM_ClockModeTypeDef PLLFrequency, HAL_SYSTEM_PLLSwitchTypeDef PLLState);

/* System Option configuration Check functions  ***********************************************/
HAL_StatusTypeDef HAL_SYSTEM_OptionConfigCheck(void);

/* RTCDota operation functions  ***********************************************/
HAL_StatusTypeDef HAL_SYSTEM_SetRTCDota(uint16_t RTCDota);
uint16_t HAL_SYSTEM_GetRTCDota(void);

/* FLASH Control functions  **********************************************/
HAL_FLASH_StatusTypeDef HAL_FLASH_Unlock(HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t ByteLength);

/* Program operation functions  ***********************************************/
HAL_FLASH_StatusTypeDef HAL_FLASH_Program(HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t SrcAddress, uint32_t ByteLength);
/* Erase operation functions  ***********************************************/
HAL_FLASH_StatusTypeDef HAL_FLASH_Erase(HAL_FLASH_IDTypeDef FlashID, uint32_t DstAddress, uint32_t ByteLength);

/* ISP operation functions  ***********************************************/
HAL_FLASH_StatusTypeDef HAL_FLASH_ClearSoftISP(uint32_t ISPProtect);
HAL_FLASH_StatusTypeDef HAL_FLASH_EnableSoftISP(uint32_t ISPCommand, uint32_t UartIOCommand);
/* CP operation functions  ***********************************************/
HAL_FLASH_StatusTypeDef HAL_FLASH_ClearCP(uint32_t JtagMode, uint32_t Core0JtagEnable, uint32_t Core1JtagEnable);

/* SEA AES operation functions  **********************************************/
HAL_SEA_StatusTypeDef HAL_SEA_AES_GF128Mul(AES_HandleTypeDef *haes, const uint8_t *pDataX, const uint8_t *pDataY, uint8_t *pDataZ);
HAL_SEA_StatusTypeDef HAL_SEA_AES_Init(AES_HandleTypeDef *haes, const AES_InitTypeDef *pConfig);
HAL_SEA_StatusTypeDef HAL_SEA_AES_DeInit(void);
HAL_SEA_StatusTypeDef HAL_SEA_AES_Encrypt(AES_HandleTypeDef *haes, const uint8_t *PlainData, uint32_t PlainLen, CYPHER_DATA_STA IsLast, uint8_t *CipherData);
HAL_SEA_StatusTypeDef HAL_SEA_AES_Decrypt(AES_HandleTypeDef *haes, const uint8_t *CipherData, uint32_t CipherLen, CYPHER_DATA_STA IsLast, uint8_t *PlainData);
HAL_SEA_StatusTypeDef HAL_SEA_AES_Gmac(AES_HandleTypeDef *haes, const uint8_t *GhashData, uint32_t GhashDataLen, CYPHER_DATA_STA IsLast);
HAL_SEA_StatusTypeDef HAL_SEA_AES_LatchProcessStatus(AES_HandleTypeDef *haes, AES_HandleTypeDef *LatchBuf);
HAL_SEA_StatusTypeDef HAL_SEA_AES_RecoverProcessStatus(AES_HandleTypeDef *RecoverBuf, AES_HandleTypeDef *haes);
HAL_SEA_StatusTypeDef HAL_SEA_AES_KeyWrapping(AES_HandleTypeDef *haes, const uint8_t *PlainData, uint32_t PlainLen, uint8_t *CipherData, uint32_t *CipherLen);
HAL_SEA_StatusTypeDef HAL_SEA_AES_KeyUnwrapping(AES_HandleTypeDef *haes, const uint8_t *CipherData, uint32_t CipherLen, uint8_t *PlainData, uint32_t *PlainLen);
HAL_SEA_StatusTypeDef HAL_SEA_AES_SetAAD(AES_HandleTypeDef *haes, const uint8_t *DataIn, uint32_t DataLen);

/* SEA HASH operation functions  *********************************************/
HAL_SEA_StatusTypeDef HAL_SEA_HASH_Init(HASH_HandleTypeDef *hhash, const HASH_InitTypeDef *pConfig);
HAL_SEA_StatusTypeDef HAL_SEA_HASH_DeInit(void);
HAL_SEA_StatusTypeDef HAL_SEA_HASH(HASH_HandleTypeDef *hhash, const uint8_t *Input, uint32_t InputLen, CYPHER_DATA_STA IsLast, uint8_t *Output);
HAL_SEA_StatusTypeDef HAL_SEA_HASH_LatchProcessStatus(const HASH_HandleTypeDef *hhash, HASH_HandleTypeDef *LatchBuf);
HAL_SEA_StatusTypeDef HAL_SEA_HASH_RecoverProcessStatus(const HASH_HandleTypeDef *RecoverBuf, HASH_HandleTypeDef *hhash);

/* SEA ECC operation functions  **********************************************/
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateKey(const ECC_HandleTypeDef *hecc, uint8_t *Q, uint8_t *K);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GeneratePublicKey(const ECC_HandleTypeDef *hecc, uint8_t *Q, const uint8_t *K);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_VerifyPublicKey(const ECC_HandleTypeDef *hecc, const uint8_t *Q, HAL_SEA_FlagTypeDef *pFlag);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateDHKeyPairs(const ECC_HandleTypeDef *hecc, const uint8_t *Q, const uint8_t *K, uint8_t *DHKey);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateSignature(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, uint8_t *Q, uint8_t *K, uint8_t *R, uint8_t *S);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateSignaturewithPKey(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, uint8_t *Q, const uint8_t *K, uint8_t *R, uint8_t *S);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_VerifySignature(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t *Q, const uint8_t *R, const uint8_t *S, HAL_SEA_FlagTypeDef *pFlag);
/* SI = Secure Interface *****************************************************/
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateKeySecurely(const ECC_HandleTypeDef *hecc, uint8_t PKeyIdx, uint8_t QxIdx, uint8_t QyIdx);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_GenerateSignatureSecurely(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t PKeyIdx, uint8_t *R, uint8_t *S);
HAL_SEA_StatusTypeDef HAL_SEA_ECC_VerifySignatureSecurely(const ECC_HandleTypeDef *hecc, const uint8_t *HashMessage, const uint32_t MessageLen, const uint8_t QxIdx, const uint8_t QyIdx, const uint8_t *R, const uint8_t *S, HAL_SEA_FlagTypeDef *pFlag);

/* SEA RSA operation functions  **********************************************/
HAL_SEA_StatusTypeDef HAL_SEA_RSA_GenerateKey(RSA_HandleTypeDef *hrsa);
HAL_SEA_StatusTypeDef HAL_SEA_RSA_GenerateSignature(RSA_HandleTypeDef *hrsa, const uint32_t *HashMessage, uint32_t MessageLen, uint32_t *Signature);
HAL_SEA_StatusTypeDef HAL_SEA_RSA_VerifySignature(RSA_HandleTypeDef *hrsa, const uint32_t *HashMessage, uint32_t MessageLen, uint32_t *Signature);
HAL_SEA_StatusTypeDef HAL_SEA_RSA_EncryptwithPublicKey(RSA_HandleTypeDef *hrsa, const uint32_t *PlainData, uint32_t PlainLen, uint32_t *CipherData);
HAL_SEA_StatusTypeDef HAL_SEA_RSA_DecryptwithPublicKey(RSA_HandleTypeDef *hrsa, const uint32_t *CipherData, uint32_t CipherLen, uint32_t *PlainData);
HAL_SEA_StatusTypeDef HAL_SEA_RSA_EncryptwithPrivateKey(RSA_HandleTypeDef *hrsa, const uint32_t *PlainData, uint32_t PlainLen, uint32_t *CipherData);
HAL_SEA_StatusTypeDef HAL_SEA_RSA_DecryptwithPrivateKey(RSA_HandleTypeDef *hrsa, const uint32_t *CipherData, uint32_t CipherLen, uint32_t *PlainData);

/* SEA TRNG operation functions  *********************************************/
HAL_SEA_StatusTypeDef HAL_SEA_TRNG_GenerateRandom(uint32_t *RandomData, uint8_t DataLen);

/* Keystore operation functions  *********************************************/
HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_Unlock(void);
HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_ReadKey(uint32_t *PKey, uint32_t KeyLen, const uint8_t KeyIdx);
HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_ReadKeyProperty(KEYSTORE_InitTypeDef *KEYSTORE_InitStruct, const uint8_t KeyIdx);
HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_WriteKey(KEYSTORE_InitTypeDef *KEYSTORE_InitStruct, const uint32_t *PKey, const uint8_t KeyIdx);
HAL_KEYSTORE_StatusTypeDef HAL_KEYSTORE_Erase(uint32_t Passwd);
uint32_t HAL_KEYSTORE_GetState(void);


/*NOTE: the order of above apis can not be modified */

/* ADD USER API BEGIN */


/* USER CODE BEGIN 1 */
uint32_t test_rpc_function(uint32_t Param1, uint32_t Param2, uint32_t Param3);
void test_rpc_helloworld_function(void);


/* USER CODE END 1 */


/* ADD USER API END */

#ifdef __cplusplus
}
#endif

#endif
