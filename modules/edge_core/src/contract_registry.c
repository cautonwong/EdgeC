#include "contract_registry.h"
#include <string.h>
#include <stdio.h>

static const edge_contract_t *g_active_contract = NULL;

int contract_register(const edge_contract_t *contract) {
    if (!contract) return -1;
    g_active_contract = contract;
    printf(" 📖 [Registry] Global Contract Registered: %s (v%s)\n", contract->name, contract->version);
    return 0;
}

const edge_contract_t *contract_get_active(void) {
    return g_active_contract;
}

const driver_contract_injection_t *contract_model_find(const char *name) {
    if (!g_active_contract || !name) return NULL;

    for (uint32_t i = 0; i < g_active_contract->model_count; i++) {
        if (strcasecmp(g_active_contract->models[i].name, name) == 0) {
            return g_active_contract->models[i].injection;
        }
    }
    return NULL;
}

__attribute__((weak)) const edge_tag_meta_t *contract_tag_get_meta(edge_tag_id_t id) {
    if (!g_active_contract) return NULL;
    
    // 此处可以优化为哈希查找，暂时使用线性查找
    for (uint32_t i = 0; i < g_active_contract->tag_count; i++) {
        if (g_active_contract->tags[i].id == id) {
            return &g_active_contract->tags[i];
        }
    }
    return NULL;
}

const api_route_t *contract_get_api_routes(uint32_t *count) {
    if (!g_active_contract) {
        *count = 0;
        return NULL;
    }
    *count = g_active_contract->api_route_count;
    return g_active_contract->api_routes;
}
