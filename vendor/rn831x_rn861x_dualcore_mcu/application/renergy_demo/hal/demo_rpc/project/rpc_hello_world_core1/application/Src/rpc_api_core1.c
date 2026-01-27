#include "rn_rv_hal.h"

#define TIMEOUT_DEFAULT 100 /* Timeout 100ms */

uint32_t test_rpc_function(uint32_t Param1, uint32_t Param2, uint32_t Param3)
{
    uint32_t api_id = RPC_MAX_USED_API_ID;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;
    api_params[1] = Param1;
    api_params[2] = Param2;
    api_params[3] = Param3;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 3, TIMEOUT_DEFAULT);
    if (HAL_OK != result) {
        return (uint32_t)result;
    }

    return api_params[1];
}

void test_rpc_helloworld_function(void)
{
    uint32_t api_id = RPC_MAX_USED_API_ID + 1;
    volatile uint32_t result;
    uint32_t api_params[9];

    api_params[0] = api_id;

    result = RPC_APICall((uint8_t)api_id, (uint32_t *)api_params, 0, TIMEOUT_DEFAULT);
}
