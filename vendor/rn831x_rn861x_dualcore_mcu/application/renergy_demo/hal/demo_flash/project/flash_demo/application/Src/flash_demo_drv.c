/**
  *****************************************************************************
  * @file   flash_demo_drv.c
  * @author Renergy Software Developers
  * @brief  Test source code
  *
  *****************************************************************************
  * @attention
  *
  * Copyright (c) 2025, Renergy Technology Inc.
  * All rights reserved.
  *
  *****************************************************************************
  */
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wunused-variable"

#include "flash_demo_drv.h"
#include "unity.h"

#define CORE0_FLASH_BASE_ADDRESS       0x0U
#define CORE1_FLASH_BASE_ADDRESS       0x90000000U
uint32_t progBuf[1000];

/* core0 flash program demo*/
/* Note: dst address of programming must be page-aligned (1 page = 512 bytes) */
/* Note: prog_start_addr > firmware length */
uint8_t hal_flash_program_func(void)
{
    HAL_FLASH_IDTypeDef flash_id;
    uint32_t i;
    uint32_t dst_addr, byteLength, src_addr;
    uint32_t *dst_result_ptr = NULL;
    uint32_t prog_start_addr = 0xA000U;

    for(i = 0; i < 1000U; i++){
        progBuf[i] = 0x12345678U;
    }


    /* Select core0 flash */
    flash_id = HAL_FLASH_ID_FLASH0;
    src_addr = (uint32_t)&progBuf;
    dst_addr = CORE0_FLASH_BASE_ADDRESS + prog_start_addr;
    byteLength = 1000U;
	/*flash erase before program*/
    /*unlock before HAL_FLASH_Program or HAL_FLASH_Erase*/
    if(HAL_FALSH_CMD_SUCCESS != HAL_FLASH_Unlock(flash_id, dst_addr, byteLength))
    {
    	return 1;
    }
    if(HAL_FALSH_CMD_SUCCESS != HAL_FLASH_Erase(flash_id, dst_addr, byteLength))
    {
    	return 1;
    }

    /* flash program */
    if(HAL_FALSH_CMD_SUCCESS != HAL_FLASH_Unlock(flash_id, dst_addr, byteLength))
    {
    	return 1;
    }
    if(HAL_FALSH_CMD_SUCCESS, HAL_FLASH_Program(flash_id, dst_addr, src_addr, byteLength))
    {
    	return 1;
    }

    /*check result*/
    dst_result_ptr = (uint32_t*)dst_addr;
    for(i = 0; i < (byteLength/4); i++){
        if(dst_result_ptr[i] != 0x12345678U){
            printf("fail log: Flash program err at %u, dst data is 0x12345678, program result is %x\n", i, dst_result_ptr[i]);
            return 1;
        }
    }
    return 0;
}

#pragma GCC diagnostic pop

