#!/usr/bin/env python3
"""
Filename: rpc_api_generate.py
Authors:  chengaofei
Date:     2024/12/26
Description: 根据rpc_api.h生成rpc_api.c, 使用rpc接口实现远端调用源码
"""
#-*- codiing=utf-8 -*-

import sys, os, time
import subprocess
import argparse
import re
import shutil

def api_find(f):
    api_list = list()
    with open(f, "r", encoding="utf-8") as fp:
        comment_start = 0
        code_start = 0
        api = ""
        for l in fp:
            # if (l.find('typedef enum') != -1) or (l.find('RPC_APIUsedIDTypeDef') != -1):
            #     continue
            # elif l.startswith('{') or l.startswith('}'):
            #     continue
            # elif (l.find('_ID') != -1) and l.startswith('  '):
            #     continue
            timeout_comment = ''
            if l.find('timeout') != -1:
                if l.find('//') != -1:
                    t = l[:l.find('//')].strip()
                    timeout_comment = l[l.find('//'):].strip()
                elif l.find('/*') != -1:
                    t = l[:l.find('/*')].strip()
                    timeout_comment = l[l.find('/*'):].strip()
            else:
                t = l.strip()
                if t.find('//') != -1:
                    t = t[:l.find('//')].strip()
                elif t.find('/*') != -1 and t.endswith('*/'):
                    t = t[:l.find('/*')].strip()
            if t == '':
                continue
            elif t.startswith('#include'):
                    api = t
                    api_list.append(api)
                    api = ""
                    continue
            elif t.startswith('//'):
                continue
            elif t.startswith('/*'):
                comment_start = 1
                if t.endswith('*/'):
                    comment_start = 0
                continue
            elif t.startswith(('#ifdef', '#ifndef', '#endif', '#define', '#if')):
                # 段落结束，如果前面有未识别的代码，直接忽略
                code_start = 0
                continue
            elif t.endswith('*/'):
                comment_start = 0
                continue
            elif comment_start == 1:
                continue
            elif code_start == 0:
                if t.endswith(';'):
                    api = ""
                    if timeout_comment != '':
                        api_list.append(t + ' ' + timeout_comment)
                        timeout_comment = ''
                    else:
                        api_list.append(t)
                else:
                    api = t
                    code_start = 1
            elif t.endswith(';'):
                code_start = 0
                api = api + " " + t
                api_list.append(api)
                api = ""
            else:
                api = api + " " + t

    return api_list

if __name__ == "__main__":
    '''
      主入口
    '''
    # f = 'rpc_api.h'
    current_dir = os.getcwd()
    h_files = [f for f in os.listdir(current_dir)
                if f.endswith('.h') and os.path.isfile(f)]
    if 'rpc_api.h' not in h_files:
        print("\n" + "="*40)
        print("错误: 当前目录下未找到rpc_api.h文件")
        print("="*40 + "\n")
        sys.stdin.flush()
        input("按回车健退出...")
        sys.exit(1)
    else:
        f = 'rpc_api.h'
        if os.path.exists(f):
            api_list = api_find(f)
            api_id = 0
            # for HOST(core1)
            with open("rpc_api_core1.c", "w", encoding="utf-8") as fp:
                # fp.writelines('#include "rpc_api.h"\n')
                fp.writelines('#include "rn_rv_hal.h"\n')
                fp.writelines('\n#define TIMEOUT_DEFAULT 100 /* Timeout 100ms */\n')
                # fp.writelines('\nextern HAL_StatusTypeDef RPC_APICall(uint8_t API_ID, uint32_t *Params, uint32_t Size, uint32_t Timeout);\n')
                # for include in api_list:
                #     include_params = include[:include.find(' ')]
                #     if include_params == "#include":
                #         fp.writelines(include + "\n")
                #         continue
                # fp.writelines("\n")
                # fp.writelines("#define RPC_SUCCESS 0\n")
                # fp.writelines("#define RPC_ERROR 1\n")
                RPC_IAP_used_strings = ["HAL_SYSTEM_ClockModeConfig", "HAL_SYSTEM_FastClockModeConfig", "HAL_SYSTEM_SetCoreDivision", "HAL_SYSTEM_PLLConfig",
                                        "HAL_SYSTEM_OptionConfigCheck", "HAL_SYSTEM_GetRTCDota", "HAL_SYSTEM_SetRTCDota",
                                        "HAL_FLASH_Unlock", "HAL_FLASH_Erase", "HAL_FLASH_Program", "HAL_FLASH_ClearSoftISP",
                                        "HAL_FLASH_EnableSoftISP", "HAL_FLASH_ClearCP", "HAL_SEA_AES_GF128Mul",
                                        "HAL_SEA_AES_Init", "HAL_SEA_AES_DeInit", "HAL_SEA_AES_Encrypt", "HAL_SEA_AES_Decrypt",
                                        "HAL_SEA_AES_Gmac", "HAL_SEA_AES_LatchProcessStatus", "HAL_SEA_AES_RecoverProcessStatus",
                                        "HAL_SEA_AES_KeyWrapping", "HAL_SEA_AES_KeyUnwrapping", "HAL_SEA_AES_SetAAD",
                                        "HAL_SEA_HASH_Init", "HAL_SEA_HASH_DeInit", "HAL_SEA_HASH", "HAL_SEA_HASH_LatchProcessStatus",
                                        "HAL_SEA_HASH_RecoverProcessStatus", "HAL_SEA_ECC_GenerateKey", "HAL_SEA_ECC_GeneratePublicKey",
                                        "HAL_SEA_ECC_VerifyPublicKey", "HAL_SEA_ECC_GenerateDHKeyPairs", "HAL_SEA_ECC_GenerateSignature",
                                        "HAL_SEA_ECC_GenerateSignaturewithPKey", "HAL_SEA_ECC_VerifySignature", "HAL_SEA_ECC_GenerateKeySecurely",
                                        "HAL_SEA_ECC_GenerateSignatureSecurely", "HAL_SEA_ECC_VerifySignatureSecurely", "HAL_SEA_RSA_GenerateKey",
                                        "HAL_SEA_RSA_GenerateSignature", "HAL_SEA_RSA_VerifySignature", "HAL_SEA_RSA_EncryptwithPublicKey",
                                        "HAL_SEA_RSA_DecryptwithPublicKey", "HAL_SEA_RSA_EncryptwithPrivateKey", "HAL_SEA_RSA_DecryptwithPrivateKey",
                                        "HAL_SEA_TRNG_GenerateRandom", "HAL_KEYSTORE_Unlock", "HAL_KEYSTORE_ReadKey", "HAL_KEYSTORE_ReadKeyProperty",
                                        "HAL_KEYSTORE_WriteKey", "HAL_KEYSTORE_Erase", "HAL_KEYSTORE_GetState"]
                for api in api_list:
                    if (api[api.find(' ') + 1:api.find('(')] in RPC_IAP_used_strings):
                        continue
                    timeout = 'TIMEOUT_DEFAULT'
                    ret_include_params = api[:api.find(' ')]
                    if ret_include_params == "#include":
                        continue
                    fp.writelines("\n")
                    if api.find('timeout') != -1:
                        if api.find('//') != -1:
                            timeout_comment = api[api.find('//'):].strip()
                            match = re.search(r'timeout\s*=\s*(\d+)', timeout_comment)
                            if match:
                                timeout = str(match.group(1))
                        elif api.find('/*') != -1:
                            timeout_comment = api[api.find('/*'):].strip()
                            match = re.search(r'timeout\s*=\s*(\d+)', timeout_comment)
                            if match:
                                timeout = str(match.group(1))
                        fp.writelines(str(timeout_comment) + "\n")
                    # fp.writelines(api[:api.find(' ')] + " rpc" + api[api.find(' ')+1:api.find(';')] +"\n{\n")
                    # fp.writelines(api[:api.find('(')] + "_RPC" + api[api.find('('):api.find(';')] +"\n{\n")
                    fp.writelines(api[:api.find(';')] +"\n{\n")
                    params = api[api.find('(') + 1:api.find(')')]
                    params_list = [x.strip() for x in params.split(',')]
                    # fp.writelines("    while (sem_get(1) != 0) {}; /* get HSEM1*/\n")
                    if api_id == 0:
                        fp.writelines("    uint32_t api_id = RPC_MAX_USED_API_ID;\n")
                    else:
                        fp.writelines("    uint32_t api_id = RPC_MAX_USED_API_ID + " + str(api_id) + ";\n")
                    fp.writelines("    volatile uint32_t result;\n")
                    fp.writelines("    uint32_t api_params[9];\n\n")
                    fp.writelines("    api_params[0] = api_id;\n")
                    params_cnt = 0
                    for p in params_list:
                        if (p == "void"):
                            continue
                        params_cnt = params_cnt + 1
                        t0 = p.split()[-1]
                        t1 = p.split()[-2]
                        if t1.endswith("*"):
                            var = t0
                            fp.writelines("    api_params[" + str(params_cnt) + "] = (uint32_t)" + var + ";\n")
                        elif t0.startswith("*"):
                            var = t0[1:]
                            fp.writelines("    api_params[" + str(params_cnt) + "] = (uint32_t)" + var + ";\n")
                        else:
                            if t0.endswith("[]"):
                                var = t0[:-2]
                                fp.writelines("    api_params[" + str(params_cnt) + "] = (uint32_t)" + var + ";\n")
                            elif t1 != "uint32_t":
                                var = t0
                                fp.writelines("    api_params[" + str(params_cnt) + "] = (uint32_t)" + var + ";\n")
                            else:
                                var = t0
                                fp.writelines("    api_params[" + str(params_cnt) + "] = " + var + ";\n")
                        # fp.writelines("    " + p + ";\n")
                    if (ret_include_params == "void"):
                        fp.writelines("\n    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, " + str(params_cnt) + ", " + str(timeout) + ");")
                        # fp.writelines("\n    sem_release(1); /* release HSEM1*/")
                        fp.writelines("\n}\n")
                    else:
                        fp.writelines("\n    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, " + str(params_cnt) + ", " + str(timeout) + ");")
                        fp.writelines("\n    if (HAL_OK != result) {")
                        # fp.writelines("\n    {")
                        fp.writelines("\n        return (" + str(ret_include_params) +")result;")
                        fp.writelines("\n    }")
                        if (ret_include_params != "uint32_t"):
                            # fp.writelines("\n        sem_release(1); /* release HSEM1*/")
                            fp.writelines("\n\n    return " + "(" + str(ret_include_params) + ")api_params[1];")
                        else:
                            fp.writelines("\n\n    return api_params[1];")
                        # fp.writelines("\n    }")
                        # fp.writelines("\n    sem_release(1); /* release HSEM1*/")
                        # fp.writelines("\n    return " + "(" + str(ret_include_params) + ")RPC_ERROR;")
                        # fp.writelines("\n    return api_params[1];")
                        fp.writelines("\n}\n")
                    # fp.writelines("\n")
                    api_id = api_id + 1

            # for MASTER(core0)
            with open("rpc_api_core0.c", "w", encoding="utf-8") as fp:
                fp.writelines('#include "rn_rv_hal.h"\n')
                fp.writelines('#include "rpc_api.h"\n')
                # fp.writelines('\n#define CORE1_FLASH_REMAP0_BASE_ADDRESS       0x80000000U\n')
                apis = list()
                ## Generate function with the _server suffix for Master
                IAP_flash_strings = ["HAL_FLASH_Unlock", "HAL_FLASH_Program", "HAL_FLASH_Erase"]
                for api in api_list:
                    ret_include_params = api[:api.find(' ')]
                    if ret_include_params == "#include":
                        continue
                    fp.writelines("\n")
                    fp.writelines(api[:api.find('(')] + "_server" + api[api.find('('):api.find(';')] + "\n{\n")
                    params = api[api.find('(') + 1:api.find(')')]
                    params_list = [x.strip() for x in params.split(',')]
                    if (api[api.find(' ') + 1:api.find('(')] in IAP_flash_strings):
                        fp.writelines("    uint32_t Flash1Size, Flash2Size;\n")
                        fp.writelines("\n")
                        fp.writelines("    if (FlashID != HAL_FLASH_ID_FLASH1)" + " {\n")
                        fp.writelines("        return HAL_FALSH_INVALID_PARAM;\n    }\n\n" )
                        fp.writelines("    Flash1Size = (((FLASH->ALLOCATE1 >> FLASH_ALLOCATE1_HSM_ENDADDR_POS) & 0x7fU) + 1U) * 8 * 1024;\n")
                        fp.writelines("    Flash2Size = (((FLASH->ALLOCATE1 >> FLASH_ALLOCATE1_HOST_ENDADDR_POS) & 0x7fU) + 1U) * 8 * 1024;\n\n")
                        fp.writelines("    if (Flash2Size > Flash1Size) {\n")
                        fp.writelines("        Flash2Size = Flash2Size - Flash1Size;\n    }\n")
                        fp.writelines("    else {\n")
                        fp.writelines("        Flash2Size = 0U;\n    }\n\n")
                        fp.writelines("    if ((DstAddress < CORE1_FLASH_REMAP0_BASE_ADDRESS) || (DstAddress > (CORE1_FLASH_REMAP0_BASE_ADDRESS + Flash2Size)) ||\n")
                        fp.writelines("        ((DstAddress + ByteLength) > (CORE1_FLASH_REMAP0_BASE_ADDRESS + Flash2Size))) {\n")
                        fp.writelines("        return HAL_FALSH_DST_ADDR_NOT_MAPPED;\n    }\n")
                    # fp.writelines("    api_params[0] = api_id;\n")
                    params_cnt = 0
                    var_list = list()
                    for p in params_list:
                        if (p == "void"):
                            continue
                        params_cnt = params_cnt + 1
                        t0 = p.split()[-1]
                        t1 = p.split()[-2]
                        if t1.endswith("*"):
                            var = t0
                            var_list.append(var)
                            # fp.writelines("    api_params[" + str(params_cnt) + "] = (uint32_t)" + var + ";\n")
                        elif t0.startswith("*"):
                            var = t0[1:]
                            var_list.append(var)
                            # fp.writelines("    api_params[" + str(params_cnt) + "] = (uint32_t)" + var + ";\n")
                        else:
                            if t0.endswith("[]"):
                                var = t0[:-2]
                                var_list.append(var)
                                # fp.writelines("    api_params[" + str(params_cnt) + "] = (uint32_t)" + var + ";\n")
                            elif t1 != "uint32_t":
                                var = t0
                                var_list.append(var)
                                # fp.writelines("    api_params[" + str(params_cnt) + "] = (uint32_t)" + var + ";\n")
                            else:
                                var = t0
                                var_list.append(var)
                                # fp.writelines("    api_params[" + str(params_cnt) + "] = " + var + ";\n")
                        # fp.writelines("    " + p + ";\n")

                    if (ret_include_params == "void"):
                        fp.writelines("    " + api[api.find(' ') + 1:api.find('(')] + "(")
                    else:
                        fp.writelines("    return " + api[api.find(' ') + 1:api.find('(')] + "(")
                    ##call no _server function
                    var_len = var_list.__len__()
                    if (var_len == 0):
                        fp.writelines(");")
                    elif (var_len == 1):
                        fp.writelines(var_list[0] + ");")
                    else:
                        for i in range(var_len):
                            if (i == 0):
                                fp.writelines(var_list[i])
                            else:
                                fp.writelines(", " + var_list[i])
                        fp.writelines(");")
                    # fp.writelines("\n")
                    fp.writelines("\n}\n")
                    api_id = api_id + 1

                ## add api_server_list
                for api in api_list:
                    # fp.writelines("extern " + api + "\n")
                    func = ""
                    for p in api.split():
                        if p.find('(') > 0:
                            func = p[:p.find('(')]
                    if func != "":
                        apis.append(func)
                fp.writelines("\nuint32_t api_server_list[] = {\n")
                for p in apis:
                    fp.writelines("    (uint32_t)" + p + "_server" + ",\n")
                fp.writelines("};\n")